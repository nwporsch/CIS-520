#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/init.h"

static void syscall_handler (struct intr_frame *);

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

}

bool
remove (const char *file)
{

}

int
open(const char *file)
{

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
