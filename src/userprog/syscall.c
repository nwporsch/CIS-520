#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "list.h"
#include "process.h"
#include "threads/vaddr.h"
#include "lib/user/syscall.h"
#include "filesys/filesys.h"

static void syscall_handler(struct intr_frame *);
void* check_addr(const void*);
struct proc_file* list_search(struct list *, int);

struct proc_file
{
	struct file *ptr;
	int fd;
	struct list_elem elem;
};

void
syscall_init(void)
{
	intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler(struct intr_frame *f)
{
	int *esp = f->esp;
	check_addr(esp);
	int sys_call = *esp;
	switch (sys_call)
	{
	case SYS_HALT:
		halt();
		break;

	case SYS_EXIT:
		check_addr(esp + 1);
		exit(*(esp + 1));
		break;

	case SYS_EXEC:
		check_addr(esp + 1);
		check_addr(*(esp + 1));
		f->eax = exec(*(esp + 1));
		break;

	case SYS_WAIT:
		check_addr(esp + 1);
		f->eax = wait(*(esp + 1));
		break;

	case SYS_CREATE:
		check_addr(esp + 2);
		check_addr(*(esp + 1));
		acquire_file_lock();
		f->eax = filesys_create(*(esp + 1), *(esp + 2));
		release_file_lock();
		break;

	case SYS_REMOVE:
		check_addr(esp + 1);
		check_addr(*(esp + 1));
		acquire_file_lock();
		f->eax = remove(*(esp + 1));
		release_file_lock();
		break;

	case SYS_OPEN:
		check_addr(esp + 1);
		check_addr(*(esp + 1));
		acquire_file_lock();
		f->eax = open(*(esp + 1));
		release_file_lock();
		break;

	case SYS_FILESIZE:
		check_addr(esp + 1);
		acquire_file_lock();
		f->eax = filesize(list_search(&thread_current()->all_files, *(esp + 1))->ptr);
		release_file_lock();
		break;

	case SYS_READ:
		check_addr(esp + 3);
		check_addr(*(esp + 2));
		f->eax = write (*(esp + 1), (void *) *(esp + 2), *(esp+ 3));
		break;

	case SYS_WRITE:
		check_addr(esp + 3);
		check_addr(*(esp + 2));
		f->eax = write (*(esp + 1), (void *) *(esp + 2), *(esp + 3));
		break;

	case SYS_SEEK:
		check_addr(esp + 2);
		acquire_file_lock();
		seek (list_search (&thread_current ()->all_files, *(esp+1))->ptr, *(esp+2));
		release_file_lock();
		break;

	case SYS_TELL:
		check_addr(esp + 1);
		acquire_file_lock();
		f->eax = tell(list_search(&thread_current()->all_files, *(esp + 1))->ptr);
		release_file_lock();
		break;

	case SYS_CLOSE:
		check_addr(esp + 1);
		acquire_file_lock();
		close(*(esp + 1));
		release_file_lock();
		break;

	default:
		exit(-1);
		break;
	}
}
/*
Terminates Pintos by calling shutdown_power_off()(declared in threads/init.h). This should be  seldom used, because you lose some information about possible deadlock situations, etc. 
*/
void
halt(void)
{
	shutdown_power_off();
}


/*
Terminates the current user program, returning statusto the kernel. If the process's parent waits for it (see below), this is the status that will be returned. Conventionally, a statusof 0 indicates success and nonzero values indicate errors. 
*/
void
exit(int status)
{
	struct list_elem *e;

	for (e = list_begin(&thread_current()->parent->children);
		e != list_end(&thread_current()->parent->children);
		e = list_next(e))
	{
		struct child *c = list_entry(e, struct child, childelem);
		if (c->tid == thread_current()->tid)
		{
			c->used = true;
			c->exit_error = status;
		}
	}

	thread_current()->exit_error = status;

	if (thread_current()->parent->tid_waiting_on == thread_current()->tid)
		sema_up(&thread_current()->parent->child_lock);
	thread_exit();
}

/*
Runs the  executable  whose name  is given in cmd_line, passing any given arguments, and returns the new process's  program  id (pid).  Must  return  pid -1,  which  otherwise  should  not  be  a  valid  pid,  if  the  program cannot  load  or  run  for  any  reason.  Thus,  the  parent  process  cannot  return  from  the execuntil  it  knows whether the child process successfully loaded its executable. You must use appropriate synchronization to ensure this. 
*/
pid_t
exec(const char *cmd_line)
{
	acquire_file_lock();
	char *fn_cp = malloc(strlen(cmd_line) + 1);
	strlcpy(fn_cp, cmd_line, strlen(cmd_line) + 1);

	char *save_ptr;
	fn_cp = strtok_r(fn_cp, " ", &save_ptr);

	struct file* f = filesys_open(fn_cp);

	if (f == NULL)
	{
		release_file_lock();
		return -1;
	}
	else
	{
		file_close(f);
		release_file_lock();
		return process_execute(cmd_line);
	}
}


/*
aits for a child process pidand retrieves the child's exit status. If pidis still alive, waits until it terminates. Then, returns the  status that pidpassed to exit.If piddid not call exit(), but was terminated by the kernel (e.g. killed due to an exception), wait(pid)must return -1. It is perfectly legal for a parent process to wait for child processes that have already terminated by the time the parent calls wait, but the kernel must still allow the parent to retrieve its child's exit status, or learn that the child was terminated by the kernel. 
*/
int
wait(pid_t pid)
{
	return process_wait(pid);
}

/*
Creates a  new file called fileinitially initial_sizebytes in size. Returns true  if successful,  false otherwise. Creating  a  new  file  does  not  open  it:  opening  the  new  file  is  a  separate  operation  which  would  require  a opensystem call. 
*/
bool
create(const char *file_name, unsigned initial_size)
{
	return filesys_create(file_name, initial_size);
}

/*
Opens  the  file  called file.  Returns  a  nonnegative  integer  handle  called  a  "file  descriptor"  (fd),  or -1  if  the file could not be opened. File descriptors numbered 0 and 1 are reserved for the console: fd 0 (STDIN_FILENO) is standard input, fd  1  (STDOUT_FILENO)  is  standard  output.  The opensystem  call  will  never  return  either  of  these  file descriptors, which are valid as system call arguments only as explicitly described below. Each process has an independent set of file descriptors. File descriptors are not inherited by child processes. When a single file is opened more than once, whether by a single process or different processes, each openreturns a new file descriptor. Different file descriptors for a single file are closed independently in separate calls to closeand they do not share a file position.
*/
bool
remove(const char *file_name)
{
	return filesys_remove(file_name) != NULL;
}

/*
Opens  the  file  called file.  Returns  a  nonnegative  integer  handle  called  a  "file  descriptor"  (fd),  or -1  if  the file could not be opened. File descriptors numbered 0 and 1 are reserved for the console: fd 0 (STDIN_FILENO) is standard input, fd  1  (STDOUT_FILENO)  is  standard  output.  The opensystem  call  will  never  return  either  of  these  file descriptors, which are valid as system call arguments only as explicitly described below. Each process has an independent set of file descriptors. File descriptors are not inherited by child processes. When a single file is opened more than once, whether by a single process or different processes, each openreturns a new file descriptor. Different file descriptors for a single file are closed independently in separate calls to closeand they do not share a file position. 
*/
int
open(const char *file)
{
	struct file *file_ptr = filesys_open (file);
	if (file_ptr == NULL)
		return -1;
	else
	{
		struct file *cur_file = filesys_open(file);
		struct proc_file *proc_file = malloc(sizeof(*proc_file));
		proc_file->ptr = file_ptr;
		proc_file->fd = thread_current()->fd_count;
		thread_current()->fd_count++;
		list_push_back(&thread_current()->all_files, &proc_file->elem);
		return proc_file->fd;
	}
}


/*
Returns the size, in bytes, of the file open as fd. 
*/
int
filesize(int fd)
{
	struct list_elem *temp;
	for (temp = list_front(&thread_current()->filede); temp != NULL; temp = temp->next)
	{
		struct proc_file *t = list_entry (temp, struct proc_file, elem);
		if (t->fd == fd)
		{
			return (int) file_length(t->ptr);
		}	
	}
	return -1;
}


/*
Reads sizebytes from the file open as fd into buffer. Returns the number of bytes actually read (0 at end of file),  or -1  if  the  file  could  not  be  read  (due  to  a  condition  other  than  end  of  file).  Fd  0  reads  from  the keyboard using input_getc(). 
*/
int
read(int fd, void* buffer, unsigned size)
{
	int i;
  if (fd == 0)
  {
  	uint8_t *f = buffer;
  	for (i = 0; i < size; i++)
  		f[i] = input_getc ();
  	return size;
  }
  else
  {
  	struct proc_file *file_ptr = list_search (&thread_current()->all_files, fd);
  	if (file_ptr == NULL)
  		return -1;
  	else
  	{
  		int offset;
  		acquire_file_lock ();
  		offset = file_read (file_ptr->ptr,buffer, size);
  		release_file_lock ();
  		return offset;
  	}
  }
}

/*
Writes sizebytes from bufferto the open file fd. Returns the number of bytes actually written, which may be less than sizeif some bytes could not be written. 
*/
int write (int fd, const void *buffer, unsigned length)
{
 	if (fd == 1)
	{
		putbuf (buffer, length);
		return length;
	}
	else
	{
		struct proc_file *file_ptr = list_search (&thread_current ()->all_files, fd);
		if (file_ptr == NULL)
			return -1;
		else
		{
			int offset;
			acquire_file_lock ();
			offset = file_write (file_ptr->ptr, buffer, length);
			release_file_lock ();
			return offset;
		}
	}
}

/*Changes  the  next  byte  to  be read  or  written  in  open  file fdto position,  expressed  in  bytes  from  the beginning of the file. (Thus, a positionof 0 is the file's start.) A  seek past the  current end of a  file is not an error. A  later read obtains 0 bytes,  indicating end of file. A later  write  extends  the  file,  filling  any  unwritten  gap  with  zeros.  (However,  in  Pintos  files  have  a  fixed length  until  project  4  is  complete,  so  writes  past  end  of  file  will  return  an  error.)  These  semantics  are implemented in the file system and do not require any special effort in system call implementation. */
void
seek(int fd, unsigned position)
{
	return file_seek(fd, position);
}

/*Returns  the  position  of  the  next  byte  to  be  read  or  written  in  open  file fd,  expressed  in  bytes  from  the beginning of the file.*/
unsigned
tell(int fd)
{
	return file_tell(fd);
}


/*
Closes file descriptor fd. Exiting or terminating a process implicitly closes all its open file descriptors, as if by calling this function for each one. 
*/
void
close(int fd)
{
	if (list_empty(&thread_current()->all_files)) return;
	struct proc_file *f;
	f = list_search(&thread_current()->all_files, fd);
	if (f != NULL)
	{
		file_close(f->ptr);
		list_remove(&f->elem);
		free(f);
	}
}


/*
Closes all the files in the provided list. 
*/
void
close_all_files(struct list *files)
{
	struct list_elem *e;

	while (!list_empty(files))
	{
		e = list_pop_front(files);
		struct proc_file *f = list_entry(e, struct proc_file, elem);
		file_close(f->ptr);
		list_remove(e);
		free(f);
	}
}

/*
Looks through a list to see if the fd is inside.
*/
struct proc_file*
	list_search(struct list *files, int fd)
{
	struct list_elem *e;
	for (e = list_begin(files);
		e != list_end(files);
		e = list_next(e))

	{
		struct proc_file *f = list_entry(e, struct proc_file, elem);
		if (f->fd == fd)
			return f;
	}

	return NULL;
}

/*
Checks if the adress passed in is in virtual memory. If its not we get the page and return that. 
*/
void*
check_addr(const void *vaddr)
{
	if (!is_user_vaddr(vaddr))
	{
		exit(-1);
		return 0;
	}
	void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
	if (!ptr)
	{
		exit(-1);
		return 0;
	}
	return ptr;
}