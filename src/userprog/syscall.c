#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/init.h"
#include "filesys/filesys.h"
#include "filesys/file.h"

static void syscall_handler (struct intr_frame *);
struct lock mem_lock;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	int * esp = f->esp;

	/*We need to check to see if the system call is below the correct PHYS_BASE */

	int system_call = *esp;


	/* The list of system calls are in lib/syscall-nr.h */
	
	/*
		SYS_HALT,                    Halt the operating system. 
		SYS_EXIT,                    Terminate this process. 
		SYS_EXEC,                    Start another process. 
		SYS_WAIT,                    Wait for a child process to die. 
		SYS_CREATE,                  Create a file. 
		SYS_REMOVE,                  Delete a file. 
		SYS_OPEN,                    Open a file. 
		SYS_FILESIZE,                Obtain a file's size. 
		SYS_READ,                    Read from a file. 
		SYS_WRITE,                   Write to a file. 
		SYS_SEEK,                    Change position in a file. 
		SYS_TELL,                    Report current position in a file. 
		SYS_CLOSE,                   Close a file. 
	*/

	switch (system_call)
	{
	case SYS_HALT:
		break;
	case SYS_EXIT:
		//check_addr(esp + 1);
		exit(*(esp + 1));
		break;
	case SYS_EXEC:
		break;
	case SYS_WAIT:
		break;
	case SYS_CREATE:
		break;
	case SYS_REMOVE:
		break;
	case SYS_OPEN:
		break;
	case SYS_FILESIZE:
		break;
	case SYS_READ:
		break;
	case SYS_WRITE:
		break;
	case SYS_SEEK:
		break;
	case SYS_TELL:
		break;
	case SYS_CLOSE:
		break;
	
	}


  /*printf ("system call!\n");
  thread_exit ();
  */
}

void
halt(void)
{
  shutdown_power_off();
}

void
exit(int status)
{
  struct list_elem *e;
  for(e = list_begin(&thread_current()->parent->children); e != list_end(&thread_current()->parent->children); e = list_next(e))
  {
    struct thread *t = list_entry (e, struct thread, childelem);
    if(t->tid == thread_current()->tid)
    {
       t->used = true;
       t->exit_error = status;

    }

    thread_current()->exit_error = status;

    if(thread_current()->parent->tid_waiting_on == thread_current()->tid)
      sema_up (&thread_current()->parent->child_lock);
    thread_exit ();
  }
}

/*
pid_t
exec(const char *cmd_line)
{

}
*/
/*int
wait (pid_t pid)
{

}
*/

bool
create (const char *file, unsigned initial_size)
{
	return filesys_create(file, initial_size);
}

bool
remove (const char *file)
{
	return filesys_remove(file) != NULL;
}

int
open(const char *file)
{
	struct file * open = NULL;
	struct filedesu *fd;
	fd = palloc_get_page(0);
	
	if(file==NULL)
	{
		return -1;
	}
	else
	{
		open = filesys_open(file);
		fd->f = open;
		fd->num= fd->num + 1;
		fd->parent = thread_current();
		list_push_back(&(thread_current()->filede), &(fd->elem));
		return fd->num;
	}
}

int
filesize (int fd)
{
	struct list_elem * current_elem;
	struct filedesu * filede;
	if(!list_empty(&thread_current()->filede))
	{
		for (current_elem= list_front(&thread_current()->filede);; current_elem=list_next(current_elem))
		{
			filede = list_entry(current_elem, struct filedesu, elem);
			if(filede->num == fd)
			{
				break;
			}
			else if(current_elem == list_tail(&thread_current()->filede) && (filede->num != fd))
			{
				return -1;
			}
		}
		
		return file_length(filede->f);
	}
}

bool 
write_mem(unsigned char *addr, unsigned char byte)
{
  if(addr != NULL)
  {
    int error_code;

    asm ("movl $1f, %0; movb %b2, %1; 1:"
        : "=&a" (error_code), "=m" (*addr) : "q" (byte));
    return error_code != -1;
  }
  else
    exit(-1);
}

int
read (int fd, void *buffer, unsigned size)
{
	struct list_elem * current_elem;
	struct filedesu * filede;
	
	if(fd == 0)
	{
		for(int i = 0; i <(unsigned)size; i++)
		{
			write_mem((unsigned char *)(buffer+i), input_getc());
			return i;
		}
	}

	else if(!list_empty(&thread_current()->filede))
	{
		for (current_elem= list_front(&thread_current()->filede);; current_elem=list_next(current_elem))
		{
			filede = list_entry(current_elem, struct filedesu, elem);
			if(filede->num == fd)
			{
				break;
			}
			else if(current_elem == list_tail(&thread_current()->filede) && (filede->num != fd))
			{
				return -1;
			}
		}
		lock_acquire(&mem_lock);
		int offset = file_read(filede->f, buffer, size);
		lock_release(&mem_lock);
		
		return offset;
	}
}

void 
seek (int fd, unsigned position)
{
	file_seek(fd, position);

}

unsigned
tell (int fd)
{
	return file_tell(fd);

}

void
close (int fd)
{

	struct list_elem * current_elem;
	struct filedesu * filede;
	
	if(list_empty(&thread_current()->filede)) return;
	else{
		for(current_elem=list_front(&thread_current()->filede); ; current_elem=list_next(current_elem))
		{
			filede = list_entry(current_elem, struct filedesu, elem);
			if(filede->num == fd)
			 break;
			if(current_elem == list_tail(&thread_current()->filede) && (filede->num != fd))
			{
			  return;
			}
		}
		if(thread_current()->tid == filede->parent->tid) // check master thread.
        {
			file_close(filede->f);
			list_remove(&(filede->elem));
			palloc_free_page(filede);
        }
	}

}

/*Returns the number of bytes written to the system console.*/
int
write(int fd, const void * buffer, unsigned length) {

	/*If fd == 1 then we print to the system console using putbuf*/
	printf("MADE IT TO WRITE\n\n");
	if (fd == 1) {
		putbuf(buffer, length);
		return length;
	}


	return 0;

}