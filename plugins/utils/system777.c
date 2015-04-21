//
//  system777.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_system777_h
#define crypto777_system777_h

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include "utils777.c"
#include "mutex.h"
#include "utlist.h"

typedef int32_t (*ptm)(int32_t,char *args[]);

// nonportable functions needed in the OS specific directory
ptm get_bundled_plugin(char *plugin);
int32_t portable_truncate(char *fname,long filesize);
void *map_file(char *fname,uint64_t *filesizep,int32_t enablewrite);
int32_t os_supports_mappedfiles();
char *os_compatible_path(char *str);
char *OS_rmstr();
int32_t OS_launch_process(char *args[]);
int32_t OS_getppid();
int32_t OS_waitpid(int32_t childpid,int32_t *statusp,int32_t flags);

// only OS portable functions in this file
#define portable_mutex_t struct nn_mutex
#define portable_mutex_init nn_mutex_init
#define portable_mutex_lock nn_mutex_lock
#define portable_mutex_unlock nn_mutex_unlock

struct queueitem { struct queueitem *next,*prev; void *ptr; };
typedef struct queue
{
	struct queueitem *list;
	portable_mutex_t mutex;
    char name[31],initflag;
} queue_t;

void lock_queue(queue_t *queue);
void queue_enqueue(char *name,queue_t *queue,struct queueitem *item);
void *queue_dequeue(queue_t *queue,int32_t offsetflag);
int32_t queue_size(queue_t *queue);
struct queueitem *queueitem(char *str);
void free_queueitem(void *itemptr);

void *aligned_alloc(uint64_t allocsize);
int32_t aligned_free(void *alignedptr);

void *portable_thread_create(void *funcp,void *argp);
void randombytes(unsigned char *x,long xlen);
double milliseconds(void);
void sleepmillis(uint32_t milliseconds);
#define portable_sleep(n) sleepmillis((n) * 1000)
#define msleep(n) sleepmillis(n)

int32_t getline777(char *line,int32_t max);
char *bitcoind_RPC(char **retstrp,char *debugstr,char *url,char *userpass,char *command,char *args);
uint16_t wait_for_myipaddr(char *ipaddr);

#endif
#else
#ifndef crypto777_system777_c
#define crypto777_system777_c

#ifndef crypto777_system777_h
#define DEFINES_ONLY
#include __BASE_FILE__
#undef DEFINES_ONLY
#endif

struct nn_clock
{
    uint64_t last_tsc;
    uint64_t last_time;
} Global_timer;

double milliseconds()
{
    uint64_t nn_clock_now (struct nn_clock *self);
    return(nn_clock_now(&Global_timer));
}

void sleepmillis(uint32_t milliseconds)
{
    void nn_sleep (int milliseconds);
    nn_sleep(milliseconds);
}

typedef void (portable_thread_func)(void *);
struct nn_thread
{
    portable_thread_func *routine;
    void *arg;
    void *handle;
};
void nn_thread_init (struct nn_thread *self,portable_thread_func *routine,void *arg);
void nn_thread_term (struct nn_thread *self);

void *portable_thread_create(void *funcp,void *argp)
{
    //void nn_thread_term(struct nn_thread *self);
    void nn_thread_init(struct nn_thread *self,portable_thread_func *routine,void *arg);
    struct nn_thread *ptr;
    ptr = (struct nn_thread *)malloc(sizeof(*ptr));
    nn_thread_init(ptr,(portable_thread_func *)funcp,argp);
    return(ptr);
}

struct queueitem *queueitem(char *str)
{
    struct queueitem *item = calloc(1,sizeof(struct queueitem) + strlen(str) + 1);
    strcpy((char *)((long)item + sizeof(struct queueitem)),str);
    return(item);
}

void free_queueitem(void *itemptr)
{
    free((void *)((long)itemptr - sizeof(struct queueitem)));
}

void lock_queue(queue_t *queue)
{
    if ( queue->initflag == 0 )
    {
        portable_mutex_init(&queue->mutex);
        queue->initflag = 1;
    }
	portable_mutex_lock(&queue->mutex);
}

void queue_enqueue(char *name,queue_t *queue,struct queueitem *item)
{
    if ( queue->list == 0 && name != 0 && name[0] != 0 )
        safecopy(queue->name,name,sizeof(queue->name));
    if ( item == 0 )
    {
        printf("FATAL type error: queueing empty value\n"), getchar();
        return;
    }
    lock_queue(queue);
    DL_APPEND(queue->list,item);
    portable_mutex_unlock(&queue->mutex);
    //printf("name.(%s) append.%p list.%p\n",name,item,queue->list);
}

void *queue_dequeue(queue_t *queue,int32_t offsetflag)
{
    struct queueitem *item = 0;
    lock_queue(queue);
    if ( queue->list != 0 )
    {
        item = queue->list;
        DL_DELETE(queue->list,item);
        //printf("name.(%s) dequeue.%p list.%p\n",queue->name,item,queue->list);
    }
	portable_mutex_unlock(&queue->mutex);
    if ( item != 0 && offsetflag != 0 )
        return((void *)((long)item + sizeof(struct queueitem)));
    else return(item);
}

int32_t queue_size(queue_t *queue)
{
    int32_t count = 0;
    struct queueitem *tmp;
    lock_queue(queue);
    DL_COUNT(queue->list,tmp,count);
    portable_mutex_unlock(&queue->mutex);
	return count;
}

static uint64_t _align16(uint64_t ptrval) { if ( (ptrval & 15) != 0 ) ptrval += 16 - (ptrval & 15); return(ptrval); }

void *aligned_alloc(uint64_t allocsize)
{
    void *ptr,*realptr;
    realptr = calloc(1,allocsize + 16 + sizeof(realptr));
    ptr = (void *)_align16((uint64_t)realptr + sizeof(ptr));
    memcpy((void *)((long)ptr - sizeof(realptr)),&realptr,sizeof(realptr));
    printf("aligned_alloc(%llu) realptr.%p -> ptr.%p, diff.%ld\n",allocsize,realptr,ptr,((long)ptr - (long)realptr));
    return(ptr);
}

int32_t aligned_free(void *ptr)
{
    void *realptr;
    long diff;
    if ( ((long)ptr & 0xf) != 0 )
    {
        printf("misaligned ptr.%p being aligned_free\n",ptr);
        return(-1);
    }
    memcpy(&realptr,(void *)((long)ptr - sizeof(realptr)),sizeof(realptr));
    diff = ((long)ptr - (long)realptr);
    if ( diff < (long)sizeof(ptr) || diff > 32 )
    {
        printf("ptr %p and realptr %p too far apart %ld\n",ptr,realptr,diff);
        return(-2);
    }
    printf("aligned_free: ptr %p -> realptr %p %ld\n",ptr,realptr,diff);
    free(realptr);
    return(0);
}

int32_t getline777(char *line,int32_t max)
{
    struct timeval timeout;
    fd_set fdset;
    int32_t s;
    line[0] = 0;
    FD_ZERO(&fdset);
    FD_SET(STDIN_FILENO,&fdset);
    timeout.tv_sec = 0, timeout.tv_usec = 10000;
    if ( (s= select(1,&fdset,NULL,NULL,&timeout)) < 0 )
        fprintf(stderr,"wait_for_input: error select s.%d\n",s);
    else if ( FD_ISSET(STDIN_FILENO,&fdset) == 0 || fgets(line,max,stdin) != 0 )
        return(-1);//sprintf(retbuf,"{\"result\":\"no messages\",\"myid\":\"%llu\",\"counter\":%d}",(long long)myid,counter), retbuf[0] = 0;
    return((int32_t)strlen(line));
}

uint16_t wait_for_myipaddr(char *ipaddr)
{
    uint16_t port = 0;
    printf("need a portable way to find IP addr\n");
    getchar();
    return(port);
}

#endif
#endif
