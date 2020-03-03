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

static void syscall_handler (struct intr_frame *);

struct proc_file* list_search(struct list *, int);

struct proc_file
{
  struct file *ptr;
  int fd;
  struct list_elem elem;
};

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  thread_exit ();
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
    struct child *c = list_entry (e, struct child, elem);
    if(c->tid == thread_current()->tid)
    {
       c->used = true;
       c->exit_error = status;
    }

    thread_current()->exit_error = status;

    if(thread_current()->parent->tid_waiting_on == thread_current()->tid)
      sema_up (&thread_current()->parent->child_lock);
    
    thread_exit ();
  }
}

pid_t
exec(const char *cmd_line)
{
  acquire_filesys_lock();

  char *cpy_filename = malloc(strlen(cmd_line)+1);
  strlcpy(cpy_filename, cmd_line, strlen(cmd_line)+1);
  
  char *ptr_cpy;
  cpy_filename = strtok_r (cpy_filename, " ", &ptr_cpy);

  struct file *f = filesys_open (cpy_filename);
  
  if(f == NULL)
  {
    release_filesys_lock();
    return -1;
  }
  else
  {
    file_close(f);
    release_filesys_lock();
    return process_execute(cmd_line);
  }
}

int
wait (pid_t pid)
{
  return process_wait(pid);
}

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
  if(file == NULL)
  {
    return -1;
  }
  else
  {
    struct proc_file *proc_file = malloc(sizeof(*proc_file));
    proc_file->ptr = file;
    proc_file->fd = thread_current()->fd_count;
    thread_current()->fd_count++;
    list_push_back(&thread_current()->files,&proc_file->elem);
    return proc_file->fd;
  }
}

int
filesize (int fd)
{

}

int
read (int fd, void *buffer, unsigned size)
{

}

void 
seek (int fd, unsigned position)
{

}

unsigned
tell (int fd)
{

}

void
close (int fd)
{

}
