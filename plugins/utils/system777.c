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
#include "../../includes/mutex.h"
#include "../../includes/utlist.h"
#include "nn.h"
#include "pubsub.h"

#define OFFSET_ENABLED (bundledflag == 0)

typedef int32_t (*ptm)(int32_t,char *args[]);
extern int32_t Debuglevel;
// nonportable functions needed in the OS specific directory
int32_t is_bundled_plugin(char *plugin);
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

struct queueitem { struct queueitem *next,*prev; };
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

struct sendendpoints { int32_t push,rep,pub,survey; };
struct recvendpoints { int32_t pull,req,sub,respond; };
struct biendpoints { int32_t bus,pair; };
struct allendpoints { struct sendendpoints send; struct recvendpoints recv; struct biendpoints both; };
union endpoints { int32_t all[sizeof(struct allendpoints) / sizeof(int32_t)]; struct allendpoints socks; };
char *get_localtransport(int32_t bundledflag);
int32_t init_socket(char *suffix,char *typestr,int32_t type,char *_bindaddr,char *_connectaddr,int32_t timeout);
int32_t shutdown_plugsocks(union endpoints *socks);
int32_t nn_broadcast(struct allendpoints *socks,uint64_t instanceid,int32_t flags,uint8_t *retstr,int32_t len);
int32_t poll_endpoints(char *messages[],uint32_t *numrecvp,uint32_t numsent,union endpoints *socks,int32_t timeoutmillis);
int32_t get_newinput(char *messages[],uint32_t *numrecvp,uint32_t numsent,int32_t permanentflag,union endpoints *socks,int32_t timeoutmillis);
void ensure_directory(char *dirname);
uint32_t calc_ipbits(char *ipaddr);


#define SOPHIA_USERDIR "/user"
struct db777 { void *env,*ctl,*db,*asyncdb; char dbname[96]; };
struct db777 *db777_create(char *path,char *name,char *compression);
int32_t db777_add(struct db777 *DB,char *key,char *value);
int32_t db777_find(char *retbuf,int32_t max,struct db777 *DB,char *key);
int32_t db777_close(struct db777 *DB);

#endif
#else
#ifndef crypto777_system777_c
#define crypto777_system777_c

#ifndef crypto777_system777_h
#define DEFINES_ONLY
#include "system777.c"
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
    printf("aligned_alloc(%llu) realptr.%p -> ptr.%p, diff.%ld\n",(long long)allocsize,realptr,ptr,((long)ptr - (long)realptr));
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

int32_t report_err(char *typestr,int32_t err,char *nncall,int32_t type,char *bindaddr,char *connectaddr)
{
    printf("%s error %d type.%d nn_%s err.%s bind.(%s) connect.(%s)\n",typestr,err,type,nncall,nn_strerror(nn_errno()),bindaddr,connectaddr!=0?connectaddr:"");
    return(-1);
}

char *get_localtransport(int32_t bundledflag) { return(OFFSET_ENABLED ? "ipc" : "inproc"); }

int32_t init_socket(char *suffix,char *typestr,int32_t type,char *_bindaddr,char *_connectaddr,int32_t timeout)
{
    int32_t sock,err = 0;
    char bindaddr[512],connectaddr[512];
    printf("%s.%s bind.(%s) connect.(%s)\n",typestr,suffix,_bindaddr,_connectaddr);
    bindaddr[0] = connectaddr[0] = 0;
    if ( _bindaddr != 0 && _bindaddr[0] != 0 )
        strcpy(bindaddr,_bindaddr), strcat(bindaddr,suffix);
    if ( _connectaddr != 0 && _connectaddr[0] != 0 )
        strcpy(connectaddr,_connectaddr), strcat(connectaddr,suffix);
    if ( (sock= nn_socket(AF_SP,type)) < 0 )
        return(report_err(typestr,sock,"nn_socket",type,bindaddr,connectaddr));
    if ( bindaddr != 0 && bindaddr[0] != 0 )
    {
        printf("bind\n");
        if ( (err= nn_bind(sock,bindaddr)) < 0 )
            return(report_err(typestr,err,"nn_bind",type,bindaddr,connectaddr));
        if ( timeout > 0 && nn_setsockopt(sock,NN_SOL_SOCKET,NN_RCVTIMEO,&timeout,sizeof(timeout)) < 0 )
            return(report_err(typestr,err,"nn_connect",type,bindaddr,connectaddr));
    }
    if ( connectaddr != 0 && connectaddr[0] != 0 )
    {
        printf("nn_connect\n");
        if ( (err= nn_connect(sock,connectaddr)) < 0 )
            return(report_err(typestr,err,"nn_connect",type,bindaddr,connectaddr));
        else if ( type == NN_SUB && (err= nn_setsockopt(sock,NN_SUB,NN_SUB_SUBSCRIBE,"",0)) < 0 )
            return(report_err(typestr,err,"nn_setsockopt subscribe",type,bindaddr,connectaddr));
    }
    printf("%s.%s socket.%d bind.(%s) connect.(%s)\n",typestr,suffix,sock,bindaddr,connectaddr);
    return(sock);
}

int32_t shutdown_plugsocks(union endpoints *socks)
{
    int32_t i,errs = 0;
    for (i=0; i<sizeof(socks->all)/sizeof(*socks->all); i++)
        if ( socks->all[i] >= 0 && nn_shutdown(socks->all[i],0) != 0 )
            errs++, printf("error (%s) nn_shutdown.%d\n",nn_strerror(nn_errno()),i);
    return(errs);
}

int32_t nn_broadcast(struct allendpoints *socks,uint64_t instanceid,int32_t flags,uint8_t *retstr,int32_t len)
{
    int32_t i,sock,errs = 0;
    for (i=0; i<=(sizeof(socks->send)/sizeof(int32_t))+1; i++)
    {
        if ( i < 2 )
            sock = (i == 0) ? socks->both.bus : socks->both.pair;
        else sock = ((int32_t *)&socks->send)[i - 1];
        if ( sock >= 0 )
        {
            if ( (len= nn_send(sock,(char *)retstr,len,0)) <= 0 )
                errs++, printf("error %d sending to socket.%d send.%d len.%d (%s)\n",len,sock,i,len,nn_strerror(nn_errno()));
            //else printf("SENT.(%s) len.%d vs strlen.%ld\n",retstr,len,strlen((char *)retstr));
        }
    }
    return(errs);
}

int32_t poll_endpoints(char *messages[],uint32_t *numrecvp,uint32_t numsent,union endpoints *socks,int32_t timeoutmillis)
{
    struct nn_pollfd pfd[sizeof(struct allendpoints)/sizeof(int32_t)];
    int32_t len,sock,processed=0,rc,i,n = 0;
    char *str,*msg;
    memset(pfd,0,sizeof(pfd));
    for (i=0; i<sizeof(socks->all)/sizeof(*socks->all); i++)
    {
        if ( (pfd[i].fd= socks->all[i]) >= 0 )
        {
            pfd[i].events = NN_POLLIN | NN_POLLOUT;
            n++;
        }
    }
    if ( n > 0 )
    {
        if ( (rc= nn_poll(pfd,sizeof(socks->all)/sizeof(*socks->all),timeoutmillis)) > 0 )
        {
            for (i=0; i<sizeof(socks->all)/sizeof(*socks->all); i++)
            {
                if ( (pfd[i].revents & NN_POLLIN) != 0 && (sock= socks->all[i]) >= 0 && (len= nn_recv(sock,&msg,NN_MSG,0)) > 0 )
                {
                    (*numrecvp)++;
                    str = clonestr(msg);
                    nn_freemsg(msg);
                    messages[processed++] = str;
                    if ( Debuglevel > 2 )
                        printf("(%d %d) %d %.6f RECEIVED.%d i.%d/%ld (%s)\n",*numrecvp,numsent,processed,milliseconds(),n,i,sizeof(socks->all)/sizeof(*socks->all),str);
                }
            }
        }
        else if ( rc < 0 )
            printf("%s Error.%d polling %d daemons [0] == %d\n",nn_strerror(nn_errno()),rc,n,pfd[0].fd);
    }
    return(processed);
}

int32_t get_socket_status(int32_t sock,int32_t timeoutmillis)
{
    struct nn_pollfd pfd;
    int32_t rc;
    pfd.fd = sock;
    pfd.events = NN_POLLIN | NN_POLLOUT;
    if ( (rc= nn_poll(&pfd,1,timeoutmillis)) == 0 )
        return(pfd.revents);
    else return(-1);
}

int32_t get_newinput(char *messages[],uint32_t *numrecvp,uint32_t numsent,int32_t permanentflag,union endpoints *socks,int32_t timeoutmillis)
{
    char line[8192];
    int32_t len,n = 0;
    line[0] = 0;
    if ( (n= poll_endpoints(messages,numrecvp,numsent,socks,timeoutmillis)) <= 0 && permanentflag == 0 && getline777(line,sizeof(line)-1) > 0 )
    {
        len = (int32_t)strlen(line);
        if ( line[len-1] == '\n' )
            line[--len] = 0;
        if ( len > 0 )
            messages[0] = clonestr(line), n = 1;
    }
    return(n);
}

uint32_t calc_ipbits(char *ipaddr)
{
    printf("make portable ipbits for (%s)\n",ipaddr);
    getchar();
    return(0);
}

#endif
#endif
