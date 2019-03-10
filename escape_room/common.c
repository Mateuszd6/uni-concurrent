#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>        /* For mode constants */
#include <sys/mman.h>
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>
#include <semaphore.h>

#include "common.h"
#include "barrier.c"

#define SHM_NAME ("md394171_pw_shm")

#define SHM_BUFF_SIZE (sizeof(mem_data))

extern int sys_nerr;
void syserr(const char *fmt, ...)
{
    va_list fmt_args;

    fprintf(stderr, "ERROR: ");
    va_start(fmt_args, fmt);
    vfprintf(stderr, fmt, fmt_args);
    va_end(fmt_args);
    fprintf(stderr, " (%d; %s)\n", errno, strerror(errno));
    exit(1);
}

void print_table(char* t, int len)
{
    int i;
    (void)(len);

    LOG("Proces %d, tablica pod adresem %p:\n", getpid(), t);
    for(i = 0; i < 32; ++i)
        LOG("|%x", t[i]);
    LOG("|\n");
}

char *load_shared_mem(int create)
{
    int fd_memory = -1;

    if(create)
        fd_memory = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    else
        fd_memory = shm_open(SHM_NAME, O_RDWR, S_IRUSR | S_IWUSR);

    if(fd_memory == -1)
        syserr("shm_open");

    if (create && ftruncate(fd_memory, SHM_BUFF_SIZE) == -1)
        syserr("truncate");

    int prot = PROT_READ | PROT_WRITE;
    int flags = MAP_SHARED;
    char* mapped_mem = (char *)mmap(NULL, SHM_BUFF_SIZE, prot, flags, fd_memory, 0);
    close(fd_memory);

    if(mapped_mem == MAP_FAILED)
        syserr("mmap");

    return mapped_mem;
}

void close_shared_mem(char* memory, int delete)
{
    munmap(memory, SHM_BUFF_SIZE);

    if(delete)
        shm_unlink(SHM_NAME);
}

void suggestion_queue_init(suggestion_queue* queue)
{
    queue->head_idx = 0;
    queue->tail_idx = 0;
}

int suggestion_queue_take(suggestion_queue* queue, suggestion* res)
{
    if(queue->head_idx != queue->tail_idx)
    {
        (* res) = queue->buffer[queue->tail_idx++];
        if(queue->tail_idx >= MAX_PLAYERS)
            queue->tail_idx = 0;

        return 1;
    }
    else
        return 0;
}

void suggestion_queue_push(suggestion_queue* queue, suggestion value)
{
    queue->buffer[queue->head_idx++] = value;
    if(queue->head_idx >= MAX_PLAYERS)
        queue->head_idx = 0;
}
