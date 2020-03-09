
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
struct process_file* list_search(struct list *, int);

struct process_file
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

/* Goes through a switch case of all system calls. */
static void
syscall_handler(struct intr_frame *f)
{
	int *ptr = f->esp;
	check_addr(ptr);

	int sys_call = *ptr;
	switch (sys_call)
	{
	case SYS_HALT:
		halt();
		break;

	case SYS_EXIT:
		check_addr(ptr + 1);
		exit(*(ptr + 1));
		break;

	case SYS_EXEC:
		check_addr(ptr + 1);
		check_addr(*(ptr + 1));
		f->eax = exec(*(ptr + 1));
		break;

	case SYS_WAIT:
		check_addr(ptr + 1);
		f->eax = wait(*(ptr + 1));
		break;

	case SYS_CREATE:
		check_addr(ptr + 2);
		check_addr(*(ptr + 1));
		acquire_file_lock();
		f->eax = filesys_create(*(ptr + 1), *(ptr + 2));
		release_file_lock();
		break;

	case SYS_REMOVE:
		check_addr(ptr + 1);
		check_addr(*(ptr + 1));
		acquire_file_lock();
		f->eax = remove(*(ptr + 1));
		release_file_lock();
		break;

	case SYS_OPEN:
		check_addr(ptr + 1);
		check_addr(*(ptr + 1));
		acquire_file_lock();
		struct file *file_ptr = filesys_open(*(ptr + 1));
		release_file_lock();
		f->eax = sys_open(file_ptr);
		break;

	case SYS_FILESIZE:
		check_addr(ptr + 1);
		acquire_file_lock();
		f->eax = sys_filesize(list_search(&thread_current()->all_files, *(ptr + 1))->ptr);
		release_file_lock();
		break;

	case SYS_READ:
		check_addr(ptr + 3);
		check_addr(*(ptr + 2));
		f->eax = sys_read(ptr);
		break;

	case SYS_WRITE:
		check_addr(ptr + 3);
		check_addr(*(ptr + 2));
		f->eax = sys_write(ptr);
		break;

	case SYS_SEEK:
		check_addr(ptr + 2);
		acquire_file_lock();
		seek(list_search(&thread_current()->all_files, *(ptr + 1))->ptr, *(ptr + 2));
		release_file_lock();
		break;

	case SYS_TELL:
		check_addr(ptr + 1);
		acquire_file_lock();
		f->eax = tell(list_search(&thread_current()->all_files, *(ptr + 1))->ptr);
		release_file_lock();
		break;

	case SYS_CLOSE:
		check_addr(ptr + 1);
		acquire_file_lock();
		sys_close(&thread_current()->all_files, *(ptr + 1));
		release_file_lock();
		break;

	default:
		exit(-1);
		break;
	}
}
/*
Calls the shutdown_power_off function
*/
void
halt(void)
{
	shutdown_power_off();
}


/*
Goes through a list of all the parent's childrens. If the child matches the current threads tid. Then set its error code to the current status and exit the thread.
*/
void
exit(int status)
{
	struct list_elem *e;

	/*Goes through the list of children in the parent thread*/
	for (e = list_begin(&thread_current()->parent->children);
		e != list_end(&thread_current()->parent->children);
		e = list_next(e))
	{
		struct child *c = list_entry(e, struct child, childelem);
		/*Finds the child with the same tid as the current thread. */
		if (c->tid == thread_current()->tid)
		{
			c->used = true;
			c->exit_error = status;
		}
	}

	/*We set the current threads exit error to the status*/
	thread_current()->exit_error = status;

	/*Tell the parent that children can be used again*/
	if (thread_current()->parent->tid_waiting_on == thread_current()->tid)
		sema_up(&thread_current()->parent->child_lock);

	/* Exit the thread. */
	thread_exit();
}

/*
Acquires the lock on the filesystem, uses malloc to allocate memory for a char pointer. 
It then copies the filename into memory using the strlcpy function. Then it uses strtok_r to tokenize
the filename and store it in fn_cp which is passed into filesys_open and stores the resulting file in
a new struct. If this new strcut variable still holds a null value, the lock on the filesystem is released,
and a -1 is returned. If the value id non-null, the file is closed, the lock is released and the process_execute
function from process.c is called. The result it produced is returned.
*/
pid_t
exec(const char *file_name)
{
	acquire_file_lock();
	char *fn_cp = malloc(strlen(file_name) + 1);
	strlcpy(fn_cp, file_name, strlen(file_name) + 1);

	char *save_ptr;
	fn_cp = strtok_r(fn_cp, " ", &save_ptr);

	/* Attempts to open the file*/
	struct file* f = filesys_open(fn_cp);

	if (f == NULL)
	{
		/* If the file does not exist release the lock*/
		release_file_lock();
		return -1;
	}
	else
	{
		/* Close the file and release the file lock and move on the excuting the process. */
		file_close(f);
		release_file_lock();
		return process_execute(file_name);
	}
}


/*
 Calls process wait
*/
int
wait(pid_t pid)
{
	return process_wait(pid);
}

/*
Calls filesys create
*/
bool
sys_create(const char *file_name, unsigned initial_size)
{
	return filesys_create(file_name, initial_size);
}

/*
Calls filesys remove
*/
bool
remove(const char *file_name)
{
	return filesys_remove(file_name) != NULL;
}

/*
Attempts to open the file.
*/
int
sys_open(struct file *file_ptr)
{
	/* If the file is null then it has failed to open.*/
	if (file_ptr == NULL)
		return -1;
	else
	{
		/* File exists so we create a file descriptor and pointer to the file.*/
		struct process_file *process_file = malloc(sizeof(*process_file));
		process_file->ptr = file_ptr;
		process_file->fd = thread_current()->fd_count;
		/* Add to the number of files the thread has open.*/
		thread_current()->fd_count++;
		/* Add the file  to the back of the list of files.*/
		list_push_back(&thread_current()->all_files, &process_file->elem);
		return process_file->fd;
	}
}


/*returns the length of the file*/
int
sys_filesize(struct file *file)
{
	return file_length(file);
}


/*
function to read the file. A pointer to the file is passed in as an argument. 
Checks if the spot next to this pointer in memory is free. If it is, declares a pointer called buffer at a location 2 spots below the one passed in.
It then uses a loop to read the file by calling the input_getc function and stores it in the buffer. 
If the spot is not empty, the list_search function in list.c is called to look for the file in the list of all_files of the current thread. If it wasn't found, returned -1.
Otherwise, the file was found and it must be read by first acquiring the lock to the filesystem, then calling file_read, releasing the lock and returning the results of file_read.
*/
int
sys_read(int *ptr)
{
	int i;

	/*Checks to see if the next memory space is open.*/
	if (*(ptr + 1) == 0)
	{
		/*Loop through and read from the file. */
		uint8_t *buffer = *(ptr + 2);
		for (i = 0; i < *(ptr + 3); i++)
			/* Add the file info to the buffer*/
			buffer[i] = input_getc();
		return *(ptr + 3);
	}
	else
	{

		/* We search through all files in the threads list of files for the file.*/
		struct process_file *file_ptr = list_search(&thread_current()->all_files, *(ptr + 1));
		if (file_ptr == NULL)
			/* The file was not found */
			return -1;
		else
		{
			/* The file was found and we acquire the lock*/
			int offset;
			acquire_file_lock();
			/*Read from the file */
			offset = file_read(file_ptr->ptr, *(ptr + 2), *(ptr + 3));
			/* Release the file lock */
			release_file_lock();
			return offset;
		}
	}
}

/*
Attempt to write to a file
*/
int
sys_write(int *ptr)
{
	/* If there is data in the memory space next to the file*/
	if (*(ptr + 1) == 1)
	{
		/* We move through the memory to the next block*/
		putbuf(*(ptr + 2), *(ptr + 3));
		return *(ptr + 3);
	}
	else
	{
		/* If the space was free then we search the list of open files. */
		struct process_file *file_ptr = list_search(&thread_current()->all_files, *(ptr + 1));
		/* We could not find the file. */
		if (file_ptr == NULL)
			return -1;
		else
		{
			/* Found the file and we write to the file. */
			int offset;
			acquire_file_lock();
			offset = file_write(file_ptr->ptr, *(ptr + 2), *(ptr + 3));
			release_file_lock();
			return offset;
		}
	}
}

/*calls file_seek in filesys.c*/
void
seek(int fd, unsigned position)
{
	return file_seek(fd, position);
}

/*calls file_tell in filesys.c*/
unsigned
tell(int fd)
{
	return file_tell(fd);
}


/*
Attempts to close the file.
*/
void
sys_close(struct list *all_files, int fd)
{
	/* Checks to see if the list of files is empty.*/
	if (list_empty(&all_files))
		return;


	struct process_file *f;
	
	/* If the list is not empty we look for the file. */
	f = list_search(all_files, fd);
	if (f != NULL)
	{
		/* We found the file and we remove it from the list. */
		file_close(f->ptr);
		list_remove(&f->elem);
		/* We free it from memory. */
		free(f);
	}
}


/*
  Closes all files a process has.
*/
void
close_all_files(struct list *files)
{
	struct list_elem *e;

	/* If the list is not empty we keep going and removing them from the list */
	while (!list_empty(files))
	{
		e = list_pop_front(files);
		struct process_file *f = list_entry(e, struct process_file, elem);
		/* Removes the file from the list. */
		file_close(f->ptr);
		list_remove(e);
		free(f);
	}
}
/*
Goes through the list of files to find the correct one.
*/
struct process_file*
	list_search(struct list *files, int fd)
{
	struct list_elem *e;

	/* Go through the list of files*/
	for (e = list_begin(files);
		e != list_end(files);
		e = list_next(e))

	{
		struct process_file *f = list_entry(e, struct process_file, elem);
		/* If we find the one with the correct fd return it. */
		if (f->fd == fd)
			return f;
	}

	/* We didn't find it. */
	return NULL;
}

/*
Checks to see if ths vaddr is a virtual address and if so creates a page.
*/
void*
check_addr(const void *vaddr)
{
	/*Checks to see if vaddr is a virtual address.*/
	if (!is_user_vaddr(vaddr))
	{
		/* It is not a virtual address*/
		exit(-1);
		return 0;
	}

	/* We attempt to get a page. */
	void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
	if (!ptr)
	{
		/* We could not get a page*/
		exit(-1);
		return 0;
	}

	/* We got a page. */
	return ptr;
}