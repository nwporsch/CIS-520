#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
int write(int fd, const void * buffer, unsigned length);
#endif /* userprog/syscall.h */
