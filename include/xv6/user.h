#pragma once

#include <xv6/stat.h>
#include <xv6/types.h>

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int pipefd[2]);
int write(int fd, const void *buf, int n);
int read(int fd, void *buf, int n);
int close(int fd);
int kill(int pid);
int exec(char *path, char **argv); // not POSIX
int open(const char *path, int flags);
int mknod(const char *path, short major, short minor); // not POSIX
int unlink(const char *path);
int fstat(int fd, struct stat *buf);
int link(const char *oldpath, const char *newpath);
int mkdir(const char *path);
int chdir(const char *path);
int dup(int oldfd);
int getpid(void);
char *sbrk(int n);
int sleep(int seconds);
int uptime(void);

// ulib.c
int stat(const char *n, struct stat *st);
char *strcpy(char *s, const char *t);
void *memmove(void *vdst, const void *vsrc, int n);
char *strchr(const char *s, char c);
int strcmp(const char *p, const char *q);
void printf(int fd, const char *fmt, ...);
char *gets(char *buf, int max);
uint strlen(const char *s);
void *memset(void *dst, int c, uint n);
void *malloc(uint nbytes);
void free(void *ap);
int atoi(const char *s);
