#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "lib/user/syscall.h"
void syscall_init(void);

bool sys_create(const char *, unsigned);
bool sys_remove(const char *);
unsigned tell(int fd);

#endif /* userprog/syscall.h */