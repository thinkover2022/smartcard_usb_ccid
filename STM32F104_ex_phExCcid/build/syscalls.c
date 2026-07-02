/* ============================================================================
 *  syscalls.c - minimal newlib (nano) stubs for STM32F1 bare metal.
 *  _sbrk provides the heap (FreeRTOS heap_3 / malloc). Heap grows up from the
 *  linker 'end' symbol toward the stack; the rest are harmless stubs.
 * ==========================================================================*/
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stddef.h>

extern char end;            /* start of heap, from linker (._user_heap_stack) */
extern char _estack;        /* top of RAM / stack base, from linker           */
#define STACK_HEADROOM  0x400

void *_sbrk(int incr)
{
    static char *heap = NULL;
    char *prev;
    char *limit = &_estack - STACK_HEADROOM;

    if (heap == NULL) heap = &end;
    if (heap + incr > limit) { errno = ENOMEM; return (void *)-1; }
    prev = heap;
    heap += incr;
    return (void *)prev;
}

int  _write (int fd, const void *buf, size_t n) { (void)fd; (void)buf; return (int)n; }
int  _read  (int fd, void *buf, size_t n)       { (void)fd; (void)buf; (void)n; return 0; }
int  _close (int fd)                            { (void)fd; return -1; }
int  _lseek (int fd, int off, int whence)       { (void)fd; (void)off; (void)whence; return 0; }
int  _isatty(int fd)                            { (void)fd; return 1; }
int  _fstat (int fd, struct stat *st)           { (void)fd; st->st_mode = S_IFCHR; return 0; }
int  _kill  (int pid, int sig)                  { (void)pid; (void)sig; errno = EINVAL; return -1; }
int  _getpid(void)                              { return 1; }
void _exit  (int code)                          { (void)code; for (;;) { } }
