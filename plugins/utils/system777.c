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
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include "../includes/miniupnp/miniwget.h"
#include "../includes/miniupnp/miniupnpc.h"
#include "../includes/miniupnp/upnpcommands.h"
#include "../includes/miniupnp/upnperrors.h"
#include "cJSON.h"
#include "utils777.c"
#include "inet.c"
#include "mutex.h"
#include "utlist.h"
#include "nn.h"
#include "pubsub.h"
#include "pipeline.h"
#include "survey.h"
#include "reqrep.h"
#include "bus.h"
#include "pair.h"
#define nn_errstr() nn_strerror(nn_errno())

extern int32_t Debuglevel;

#define OFFSET_ENABLED (bundledflag == 0)
#ifndef MIN
#define MIN(x,y) (((x)<=(y)) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x,y) (((x)>=(y)) ? (x) : (y))
#endif
typedef int32_t (*ptm)(int32_t,char *args[]);
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

struct sendendpoints { int32_t push,rep,pub,survey; };
struct recvendpoints { int32_t pull,req,sub,respond; };
struct biendpoints { int32_t bus,pair; };
struct allendpoints { struct sendendpoints send; struct recvendpoints recv; struct biendpoints both; };
union endpoints { int32_t all[sizeof(struct allendpoints) / sizeof(int32_t)]; struct allendpoints socks; };

#define DEFAULT_APISLEEP 50
struct SuperNET_info
{
    char WEBSOCKETD[1024],NXTAPIURL[1024],NXTSERVER[1024],DATADIR[1024],**publications;
    char myipaddr[64],myNXTacct[64],myNXTaddr[64],NXTACCT[64],NXTADDR[64],NXTACCTSECRET[4096],userhome[512];
    uint64_t my64bits;
    int32_t usessl,ismainnet,Debuglevel,SuperNET_retval,APISLEEP,europeflag,numpubs,readyflag;
    uint16_t port;
}; extern struct SuperNET_info SUPERNET;

struct coins_info
{
    int32_t num,readyflag;
    cJSON *argjson;
    struct coin777 **LIST;
    // this will be at the end of the plugins structure and will be called with all zeros to _init
}; extern struct coins_info COINS;

struct sophia_info
{
    char PATH[1024];
    int32_t numdbs,readyflag;
    struct db777 *DBS[1024];
}; extern struct sophia_info SOPHIA;

#define MAX_MGWSERVERS 16
struct MGW_info
{
    char PATH[1024],serverips[MAX_MGWSERVERS][64],bridgeipaddr[64],bridgeacct[64];
    uint64_t srv64bits[MAX_MGWSERVERS],issuers[64];
    int32_t M,N,numgateways,gatewayid,numissuers,readyflag;
    union endpoints all;
    uint32_t numrecv,numsent;
}; extern struct MGW_info MGW;

#define MAX_RAMCHAINS 128
struct ramchain_info
{
    char PATH[1024],coins[MAX_RAMCHAINS][16];
    double lastupdate[MAX_RAMCHAINS];
    union endpoints all;
    int32_t num,readyflag;
    // this will be at the end of the plugins structure and will be called with all zeros to _init
}; extern struct ramchain_info RAMCHAINS;

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

int upnpredirect(const char* eport, const char* iport, const char* proto, const char* description);

void *aligned_alloc(uint64_t allocsize);
int32_t aligned_free(void *alignedptr);

void *portable_thread_create(void *funcp,void *argp);
void randombytes(unsigned char *x,long xlen);
double milliseconds(void);
void msleep(uint32_t milliseconds);
#define portable_sleep(n) msleep((n) * 1000)

int32_t getline777(char *line,int32_t max);
char *bitcoind_RPC(char **retstrp,char *debugstr,char *url,char *userpass,char *command,char *args);
uint16_t wait_for_myipaddr(char *ipaddr);

char *get_localtransport(int32_t bundledflag);
int32_t init_socket(char *suffix,char *typestr,int32_t type,char *_bindaddr,char *_connectaddr,int32_t timeout);
int32_t shutdown_plugsocks(union endpoints *socks);
int32_t nn_broadcast(struct allendpoints *socks,uint64_t instanceid,int32_t flags,uint8_t *retstr,int32_t len);
int32_t poll_endpoints(char *messages[],uint32_t *numrecvp,uint32_t numsent,union endpoints *socks,int32_t timeoutmillis);
int32_t get_newinput(char *messages[],uint32_t *numrecvp,uint32_t numsent,int32_t permanentflag,union endpoints *socks,int32_t timeoutmillis);
void ensure_directory(char *dirname);

uint64_t calc_ipbits(char *ipaddr);
void expand_ipbits(char *ipaddr,uint64_t ipbits);
char *ipbits_str(uint64_t ipbits);
char *ipbits_str2(uint64_t ipbits);
struct sockaddr_in conv_ipbits(uint64_t ipbits);

char *plugin_method(char *previpaddr,char *plugin,char *method,uint64_t daemonid,uint64_t instanceid,char *origargstr,int32_t numiters,int32_t async);

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

void msleep(uint32_t milliseconds)
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

// redirect port on external upnp enabled router to port on *this* host
int upnpredirect(const char* eport, const char* iport, const char* proto, const char* description)
{
    
    //  Discovery parameters
    struct UPNPDev * devlist = 0;
    struct UPNPUrls urls;
    struct IGDdatas data;
    int i;
    char lanaddr[64];	// my ip address on the LAN
    const char* leaseDuration="0";
    
    //  Redirect & test parameters
    char intClient[40];
    char intPort[6];
    char externalIPAddress[40];
    char duration[16];
    int error=0;
    
    //  Find UPNP devices on the network
    if ((devlist=upnpDiscover(2000, 0, 0,0, 0, &error))) {
        struct UPNPDev * device = 0;
        printf("UPNP INIALISED: List of UPNP devices found on the network.\n");
        for(device = devlist; device; device = device->pNext) {
            printf("UPNP INFO: dev [%s] \n\t st [%s]\n",
                   device->descURL, device->st);
        }
    } else {
        printf("UPNP ERROR: no device found - MANUAL PORTMAP REQUIRED\n");
        return 0;
    }
    
    //  Output whether we found a good one or not.
    if((error = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr)))) {
        switch(error) {
            case 1:
                printf("UPNP OK: Found valid IGD : %s\n", urls.controlURL);
                break;
            case 2:
                printf("UPNP WARN: Found a (not connected?) IGD : %s\n", urls.controlURL);
                break;
            case 3:
                printf("UPNP WARN: UPnP device found. Is it an IGD ? : %s\n", urls.controlURL);
                break;
            default:
                printf("UPNP WARN: Found device (igd ?) : %s\n", urls.controlURL);
        }
        printf("UPNP OK: Local LAN ip address : %s\n", lanaddr);
    } else {
        printf("UPNP ERROR: no device found - MANUAL PORTMAP REQUIRED\n");
        return 0;
    }
    
    //  Get the external IP address (just because we can really...)
    if(UPNP_GetExternalIPAddress(urls.controlURL,
                                 data.first.servicetype,
                                 externalIPAddress)!=UPNPCOMMAND_SUCCESS)
        printf("UPNP WARN: GetExternalIPAddress failed.\n");
    else
        printf("UPNP OK: ExternalIPAddress = %s\n", externalIPAddress);
    
    //  Check for existing supernet mapping - from this host and another host
    //  In theory I can adapt this so multiple nodes can exist on same lan and choose a different portmap
    //  for each one :)
    //  At the moment just delete a conflicting portmap and override with the one requested.
    i=0;
    error=0;
    do {
        char index[6];
        char extPort[6];
        char desc[80];
        char enabled[6];
        char rHost[64];
        char protocol[4];
        
        snprintf(index, 6, "%d", i++);
        
        if(!(error=UPNP_GetGenericPortMappingEntry(urls.controlURL,
                                                   data.first.servicetype,
                                                   index,
                                                   extPort, intClient, intPort,
                                                   protocol, desc, enabled,
                                                   rHost, duration))) {
            // printf("%2d %s %5s->%s:%-5s '%s' '%s' %s\n",i, protocol, extPort, intClient, intPort,desc, rHost, duration);
            
            // check for an existing supernet mapping on this host
            if(!strcmp(lanaddr, intClient)) { // same host
                if(!strcmp(protocol,proto)) { //same protocol
                    if(!strcmp(intPort,iport)) { // same port
                        printf("UPNP WARN: existing mapping found (%s:%s)\n",lanaddr,iport);
                        if(!strcmp(extPort,eport)) {
                            printf("UPNP OK: exact mapping already in place (%s:%s->%s)\n", lanaddr, iport, eport);
                            FreeUPNPUrls(&urls);
                            freeUPNPDevlist(devlist);
                            return 1;
                            
                        } else { // delete old mapping
                            printf("UPNP WARN: deleting existing mapping (%s:%s->%s)\n",lanaddr, iport, extPort);
                            if(UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, extPort, proto, rHost))
                                printf("UPNP WARN: error deleting old mapping (%s:%s->%s) continuing\n", lanaddr, iport, extPort);
                            else printf("UPNP OK: old mapping deleted (%s:%s->%s)\n",lanaddr, iport, extPort);
                        }
                    }
                }
            } else { // ipaddr different - check to see if requested port is already mapped
                if(!strcmp(protocol,proto)) {
                    if(!strcmp(extPort,eport)) {
                        printf("UPNP WARN: EXT port conflict mapped to another ip (%s-> %s vs %s)\n", extPort, lanaddr, intClient);
                        if(UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, extPort, proto, rHost))
                            printf("UPNP WARN: error deleting conflict mapping (%s:%s) continuing\n", intClient, extPort);
                        else printf("UPNP OK: conflict mapping deleted (%s:%s)\n",intClient, extPort);
                    }
                }
            }
        } else
            printf("UPNP OK: GetGenericPortMappingEntry() End-of-List (%d entries) \n", i);
    } while(error==0);
    
    //  Set the requested port mapping
    if((i=UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                              eport, iport, lanaddr, description,
                              proto, 0, leaseDuration))!=UPNPCOMMAND_SUCCESS) {
        printf("UPNP ERROR: AddPortMapping(%s, %s, %s) failed with code %d (%s)\n",
               eport, iport, lanaddr, i, strupnperror(i));
        
        FreeUPNPUrls(&urls);
        freeUPNPDevlist(devlist);
        return 0; //error - adding the port map primary failure
    }
    
    if((i=UPNP_GetSpecificPortMappingEntry(urls.controlURL,
                                           data.first.servicetype,
                                           eport, proto, NULL/*remoteHost*/,
                                           intClient, intPort, NULL/*desc*/,
                                           NULL/*enabled*/, duration))!=UPNPCOMMAND_SUCCESS) {
        printf("UPNP ERROR: GetSpecificPortMappingEntry(%s, %s, %s) failed with code %d (%s)\n", eport, iport, lanaddr,
               i, strupnperror(i));
        FreeUPNPUrls(&urls);
        freeUPNPDevlist(devlist);
        return 0; //error - port map wasnt returned by query so likely failed.
    }
    else printf("UPNP OK: EXT (%s:%s) %s redirected to INT (%s:%s) (duration=%s)\n",externalIPAddress, eport, proto, intClient, intPort, duration);
    FreeUPNPUrls(&urls);
    freeUPNPDevlist(devlist);
    return 1; //ok - we are mapped:)
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

#define MAX_SERVERNAME 128
int32_t nn_typelist[] = { NN_REP, NN_REQ, NN_RESPONDENT, NN_SURVEYOR, NN_PUB, NN_SUB, NN_PULL, NN_PUSH };

int32_t nn_oppotype(int32_t type)
{
    switch ( type )
    {
            // Messages that need a response from the set of peers: SURVEY
        case NN_SURVEYOR: return(NN_RESPONDENT); break;
        case NN_RESPONDENT: return(NN_SURVEYOR); break;
            // Messages that need a response, but only from one peer: REQ/REP
        case NN_REQ: return(NN_REP); break;
        case NN_REP: return(NN_REQ); break;
            // One-way messages to one peer: PUSH/PULL
        case NN_PUSH: return(NN_PULL); break;
        case NN_PULL: return(NN_PUSH); break;
            //  One-way messages to all: PUB/SUB
        case NN_PUB: return(NN_SUB); break;
        case NN_SUB: return(NN_PUB); break;
    }
    return(-1);
}

int32_t nn_portoffset(int32_t type)
{
    int32_t i;
    for (i=0; i<(int32_t)(sizeof(nn_typelist)/sizeof(*nn_typelist)); i++)
        if ( nn_typelist[i] == type )
            return(i);
    return(-1);
}

void set_endpointaddr(char *endpoint,char *domain,uint16_t port,int32_t type)
{
    sprintf(endpoint,"tcp://%s:%d",domain,port + nn_portoffset(type));
}

int32_t badass_servers(char servers[][MAX_SERVERNAME],int32_t max,int32_t port)
{
    static char *tcpformat = "instantdex%d.anonymous.supply";
    char domain[MAX_SERVERNAME];
    int32_t i,n = 0;
    for (i=1; i<=7&&n<max; i++,n++)
    {
        sprintf(domain,tcpformat,i);
        set_endpointaddr(servers[i],domain,port,NN_REP);
    }
    return(n);
}

int32_t crackfoo_servers(char servers[][MAX_SERVERNAME],int32_t max,int32_t port)
{
    static char *tcpformat = "ps%02d.bitcoindark.ca";
    char domain[MAX_SERVERNAME];
    int32_t i,n = 0;
    for (i=0; i<=20&&n<max; i++,n++)
    {
        sprintf(domain,tcpformat,i);
        set_endpointaddr(servers[i],domain,port,NN_REP);
    }
    return(n);
}

int32_t nn_addservers(int32_t priority,int32_t sock,char servers[][MAX_SERVERNAME],int32_t num)
{
    int32_t i;
    if ( num > 0 && servers != 0 && nn_setsockopt(sock,NN_SOL_SOCKET,NN_SNDPRIO,&priority,sizeof(priority)) == 0 )
    {
        for (i=0; i<num; i++)
            if ( nn_connect(sock,servers[i]) != 0 )
                printf("error connecting to (%s) (%s)\n",servers[i],nn_errstr());
        priority++;
    } else printf("error setting priority.%d (%s)\n",priority,nn_errstr());
    return(priority);
}

int32_t nn_loadbalanced_socket(int32_t retrymillis,char servers[][MAX_SERVERNAME],int32_t num,char backups[][MAX_SERVERNAME],int32_t numbacks,char failsafe[MAX_SERVERNAME])
{
    int32_t reqsock,priority = 1;
    if ( (reqsock= nn_socket(AF_SP,NN_REQ)) >= 0 )
    {
        if ( nn_setsockopt(reqsock,NN_REQ,NN_REQ_RESEND_IVL,&retrymillis,sizeof(retrymillis)) == 0 )
            printf("error setting NN_SOL_SOCKET socket %s\n",nn_errstr());
        priority = nn_addservers(priority,reqsock,servers,num);
        priority = nn_addservers(priority,reqsock,backups,numbacks);
        priority = nn_addservers(priority,reqsock,(char (*)[128])failsafe,1);
    } else printf("error getting req socket %s\n",nn_errstr());
    return(reqsock);
}

int32_t loadbalanced_socket(int32_t retrymillis,int32_t europeflag,int32_t port)
{
    char Cservers[32][MAX_SERVERNAME],Bservers[32][MAX_SERVERNAME],jnxtaddr[MAX_SERVERNAME];
    int32_t n,m,lbsock;
    set_endpointaddr(jnxtaddr,"jnxt.org",port,NN_REQ);
    n = crackfoo_servers(Cservers,sizeof(Cservers)/sizeof(*Cservers),port);
    m = badass_servers(Bservers,sizeof(Bservers)/sizeof(*Bservers),port);
    if ( europeflag != 0 )
        lbsock = nn_loadbalanced_socket(retrymillis,Bservers,m,Cservers,n,jnxtaddr);
    else lbsock = nn_loadbalanced_socket(retrymillis,Cservers,n,Bservers,m,jnxtaddr);
    return(lbsock);
}

void nn_shutdown_pfds(struct nn_pollfd pfds[2])
{
    if ( pfds[0].fd >= 0 )
        nn_shutdown(pfds[0].fd,0);
    if ( pfds[1].fd >= 0 )
        nn_shutdown(pfds[1].fd,0);
}

int32_t process_bridge_pfds(struct nn_pollfd pfds[2],queue_t errQs[2])
{
    int32_t i,len,sendlen,flags = 0; char *msg;
    for (i=0; i<2; i++)
    {
        flags <<= 4;
        //printf("n.%d i.%d check socket.%d:%d revents.%d\n",n,i,pfd[i].fd,socks->all[i],pfd[i].revents);
        if ( (pfds[i].revents & NN_POLLIN) != 0 && (len= nn_recv(pfds[i].fd,&msg,NN_MSG,0)) > 0 )
        {
            sendlen = -1, flags |= 1;
            if ( (pfds[i ^ 1].revents & NN_POLLOUT) == 0 || (sendlen= nn_send(pfds[i ^ 1].fd,msg,NN_MSG,0)) != len )
            {
                printf("bridging_point: socket.(%d) pollout.%d || sendlen.%d != len.%d\n",i ^ 1,pfds[i ^ 1].revents,sendlen,len);
                queue_enqueue("bridge_errQ",&errQs[i ^ 1],queueitem(msg)), flags |= 2;
            }
            // else nn_freemsg(msg); done by nn_send on success
        }
        else if ( (pfds[i].revents & NN_POLLOUT) != 0 && (msg= queue_dequeue(&errQs[i],1)) != 0 )
        {
            printf("bridging_point: socket.(%d) resending.%p\n",i,msg);
            flags |= 4;
            if ( nn_send(pfds[i].fd,msg,NN_MSG,0) <= 0 )
            {
                printf("re-queue failed resend of %p\n",msg);
                queue_enqueue("bridge_retryQ",&errQs[i],queueitem(msg)), flags |= 8;
            }
            free_queueitem(msg); // in either case need to free the queue_item wrapper
        }
    }
    return(flags);
}

void MGW_loop()
{
    int32_t poll_daemons(); char *SuperNET_JSON(char *JSONstr);
    int i,n,timeoutmillis = 10;
    char *messages[100],*msg;
    poll_daemons();
    if ( (n= poll_endpoints(messages,&MGW.numrecv,MGW.numsent,&MGW.all,timeoutmillis)) > 0 )
    {
        for (i=0; i<n; i++)
        {
            msg = SuperNET_JSON(messages[i]);
            printf("%d of %d: (%s)\n",i,n,msg);
            free(messages[i]);
        }
    }
}

struct loopargs { char *(*respondfunc)(int32_t type,char *); int32_t sock,type,bindflag; char endpoint[MAX_SERVERNAME]; };
void provider_respondloop(void *_args)
{
    struct loopargs *args = _args;
    int32_t len,sendlen; char *msg,*jsonstr;
    if ( args->type != NN_RESPONDENT && args->type != NN_REP && args->type != NN_PAIR )
    {
        printf("responder loop doesnt deal with type.%d\n",args->type);
        return;
    }
    if ( args->sock >= 0 )
    {
        if ( args->bindflag == 0 && nn_connect(args->sock,args->endpoint) != 0 )
            printf("error connecting to bridgepoint sock.%d type.%d to (%s) %s\n",args->sock,args->type,args->endpoint,nn_errstr());
        else if ( args->bindflag == 0 && nn_bind(args->sock,args->endpoint) != 0 )
            printf("error binding to bridgepoint sock.%d type.%d to (%s) %s\n",args->sock,args->type,args->endpoint,nn_errstr());
        else
        {
            while ( 1 )
            {
                if ( (len= nn_recv(args->sock,&msg,NN_MSG,0)) > 0 )
                {
                    if ( (jsonstr= (*args->respondfunc)(args->type,msg)) != 0 )
                    {
                        len = (int32_t)strlen(jsonstr)+1;
                        if ( (sendlen= nn_send(args->sock,jsonstr,len,0)) != len )
                            printf("warning: sendlen.%d vs %ld for (%s)\n",sendlen,strlen(jsonstr)+1,jsonstr);
                        free(jsonstr);
                    }
                    nn_freemsg(msg);
                }
            }
        }
    } else printf("error getting socket type.%d %s\n",args->type,nn_errstr());
}

char *publist_jsonstr(char *category)
{
    cJSON *json,*array = cJSON_CreateArray();
    int32_t i; char endpoint[MAX_SERVERNAME],*retstr;
    for (i=0; i<SUPERNET.numpubs; i++)
        cJSON_AddItemToArray(array,cJSON_CreateString(SUPERNET.publications[i]));
    json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"endpoint",cJSON_CreateString(endpoint));
    retstr = cJSON_Print(json);
    free_json(json);
    _stripwhite(retstr,' ');
    return(retstr);
}

cJSON *Bridges;
char *loadbalanced_response(char *jsonstr,cJSON *json)
{
    if ( Bridges != 0 )
        return(cJSON_Print(Bridges));
    else return(clonestr("{\"error\":\"no known bridges\"}"));
}

int32_t get_bridgeaddr(char *bridgeaddr,int32_t lbsock)
{
    cJSON *json,*item;
    char *msg,*request = "{\"requestType\":\"getbridges\"}";
    int32_t n,len,sendlen;
    len = (int32_t)strlen(request) + 1;
    if ( (sendlen= nn_send(lbsock,request,len,0)) == len )
    {
        if ( (len= nn_recv(lbsock,&msg,NN_MSG,0)) > 0 )
        {
            if ( (json= cJSON_Parse(msg)) != 0 )
            {
                if ( is_cJSON_Array(json) != 0 && (n= cJSON_GetArraySize(json)) > 0 )
                    item = cJSON_GetArrayItem(json,rand() % n);
                else item = cJSON_GetObjectItem(json,"endpoint");
                copy_cJSON(bridgeaddr,item);
                free_json(json);
            }
            nn_freemsg(msg);
        } else printf("get_bridgeaddr: got len %d: %s\n",len,nn_errstr());
    } else printf("got sendlen.%d instead of %d\n",sendlen,len);
    return(-1);
}

char *global_response(char *jsonstr,cJSON *json)
{
    char *request,*endpoint;
    if ( (request= cJSON_str(cJSON_GetObjectItem(json,"requestType"))) != 0 )
    {
        if ( strcmp(request,"newbridge") == 0 && (endpoint= cJSON_str(cJSON_GetObjectItem(json,"endpoint"))) != 0 )
        {
            if ( Bridges == 0 )
                Bridges = cJSON_CreateArray();
            if ( in_jsonarray(Bridges,endpoint) == 0 )
            {
                cJSON_AddItemToArray(Bridges,cJSON_CreateString(endpoint));
                return(clonestr("{\"result\":\"bridge added\"}"));
            }
            else return(clonestr("{\"result\":\"bridge already in list\"}"));
        }
        if ( strcmp(request,"servicelist") == 0 )
            return(publist_jsonstr(cJSON_str(cJSON_GetObjectItem(json,"category"))));
    }
    return(clonestr("{\"error\":\"unknown request\"}"));
}

char *process_buspacket(char *jsonstr,cJSON *json)
{
    printf("BUSPACKET.(%s)\n",jsonstr);
    return(clonestr("{\"result\":\"processed bus packet\"}"));
}

char *nn_response(int32_t type,char *jsonstr)
{
    cJSON *json; char *retstr = 0;
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        switch ( type )
        {
            case NN_REP: retstr = loadbalanced_response(jsonstr,json); break;
            case NN_RESPONDENT: retstr = global_response(jsonstr,json); break;
            case NN_BUS: retstr = process_buspacket(jsonstr,json); break;
            default: retstr = clonestr("{\"error\":\"invalid socket type\"}"); break;
        }
        free_json(json);
    } else retstr = clonestr("{\"error\":\"couldnt parse request\"}");
    return(retstr);
}

void launch_serverthread(struct loopargs *args,int32_t type,int32_t bindflag)
{
    char endpoint[MAX_SERVERNAME];
    set_endpointaddr(endpoint,"127.0.0.1",SUPERNET.port,type);
    args->type = NN_RESPONDENT, args->respondfunc = nn_response, args->bindflag = bindflag, strcpy(args->endpoint,endpoint);
    portable_thread_create(provider_respondloop,args);
}

void serverloop(void *_args)
{
    int32_t types[] = { NN_REP, NN_RESPONDENT, NN_PUB, NN_PULL };
    struct nn_pollfd pfds[4][2]; queue_t errQs[4][2]; char bindaddr[128];
    int32_t i,j,rc,n,type,portoffset,sock,numtypes,timeoutmillis,err,bindflag = 1;
    struct loopargs args[2];
    numtypes = (int32_t)(sizeof(types)/sizeof(*types));
    memset(args,0,sizeof(args));
    memset(pfds,0xff,sizeof(pfds)); memset(errQs,0,sizeof(errQs));
    timeoutmillis = 1;
    for (i=n=0; i<numtypes; i++)
    {
        printf("i.%d of numtypes.%d\n",i,numtypes);
        for (j=err=0; j<2; j++,n++)
        {
            type = (j == 0) ? types[i] : nn_oppotype(types[i]);
            if ( (portoffset= nn_portoffset(type)) != n )
                printf("FATAL mismatched portoffset %d vs %d\n",portoffset,n), getchar();
            set_endpointaddr(bindaddr,"127.0.0.1",SUPERNET.port,type);
            printf("(%d) type.%d bindaddr.%s\n",types[i],type,bindaddr);
            if ( (sock= nn_socket(AF_SP_RAW,type)) < 0 )
                break;
            printf("got socket %d\n",sock);
            if ( (err= nn_bind(sock,bindaddr)) != 0 )
                break;
            printf("nn_bind %d\n",sock);
            pfds[i][j].fd = sock;
            pfds[i][j].events = NN_POLLIN | NN_POLLOUT;
        }
        if ( j != 2 )
        {
            printf("error.%d launching bridges at i.%d of %d j.%d %s\n",err,i,numtypes,j,nn_errstr());
            break;
        }
    }
    args[0].sock = nn_socket(AF_SP,NN_REP);
    args[1].sock = nn_socket(AF_SP,NN_RESPONDENT);
    printf("launch NN_REP %d\n",args[0].sock);
    printf("launch NN_RESPONDENT %d\n",args[1].sock);
    if  ( i == numtypes )
    {
        launch_serverthread(&args[0],NN_REP,bindflag);
        launch_serverthread(&args[1],NN_RESPONDENT,bindflag);
        while ( 1 )
        {
            if ( (rc= nn_poll(&pfds[0][0],numtypes << 1,timeoutmillis)) > 0 )
            {
                for (i=0; i<numtypes; i++)
                    process_bridge_pfds(pfds[i],errQs[i]);
            }
            else if ( rc < 0 )
                printf("%s Error polling launch_bridging_point\n",nn_errstr());
            if ( MGW.gatewayid >= 0 || MGW.srv64bits[MGW.N] == SUPERNET.my64bits )
                MGW_loop();
        }
    }
    for (i=0; i<numtypes; i++)
        nn_shutdown_pfds(pfds[i]);
}

char *make_globalrequest(int32_t retrymillis,char *jsonquery,int32_t timeoutmillis)
{
    static char bridgeaddr[MAX_SERVERNAME];
    static int32_t lbsock = -1;
    cJSON *item,*array = cJSON_CreateArray();
    int32_t n,len,surveysock;
    char *msg,*retstr;
    if ( timeoutmillis <= 0 )
        timeoutmillis = 1000;
    if ( lbsock < 0 )
        lbsock = loadbalanced_socket(retrymillis,SUPERNET.europeflag,SUPERNET.port);
    if ( lbsock < 0 )
        return(clonestr("{\"error\":\"getting loadbalanced socket\"}"));
    if ( bridgeaddr[0] == 0 && get_bridgeaddr(bridgeaddr,lbsock) != 0 )
        return(clonestr("{\"error\":\"getting bridgeaddr\"}"));
    if ( (surveysock= nn_socket(AF_SP,NN_SURVEYOR)) < 0 )
        return(clonestr("{\"error\":\"getting surveysocket\"}"));
    else if ( nn_connect(surveysock,bridgeaddr) != 0 )
    {
        nn_shutdown(surveysock,0);
        return(clonestr("{\"error\":\"connecting to bridgepoint\"}"));
    }
    else if ( nn_setsockopt(surveysock,NN_SURVEYOR,NN_SURVEYOR_DEADLINE,&timeoutmillis,sizeof(timeoutmillis)) != 0 )
    {
        nn_shutdown(surveysock,0);
        return(clonestr("{\"error\":\"setting timeout\"}"));
    }
    _stripwhite(jsonquery,' ');
    if ( (len= nn_send(surveysock,jsonquery,(int32_t)strlen(jsonquery)+1,0)) > 0 )
    {
        n = 0;
        while ( (len= nn_recv(surveysock,&msg,NN_MSG,0)) > 0 )
        {
            if ( (item= cJSON_Parse(msg)) != 0 )
                cJSON_AddItemToArray(array,item), n++;
            nn_freemsg(msg);
        }
    }
    else
    {
        nn_shutdown(surveysock,0);
        return(clonestr("{\"error\":\"sending out query\"}"));
    }
    nn_shutdown(surveysock,0);
    if ( n == 0 )
    {
        bridgeaddr[0] = 0;
        free_json(array);
        return(clonestr("{\"error\":\"no responses\n"));
    }
    retstr = cJSON_Print(array);
    printf("globalrequest(%s) via bridge.(%s) returned (%s) from n.%d respondents\n",jsonquery,bridgeaddr,retstr,n);
    free_json(array);
    return(retstr);
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
    //printf("%s.%s bind.(%s) connect.(%s)\n",typestr,suffix,_bindaddr,_connectaddr);
    bindaddr[0] = connectaddr[0] = 0;
    if ( _bindaddr != 0 && _bindaddr[0] != 0 )
        strcpy(bindaddr,_bindaddr), strcat(bindaddr,suffix);
    if ( _connectaddr != 0 && _connectaddr[0] != 0 )
        strcpy(connectaddr,_connectaddr), strcat(connectaddr,suffix);
    if ( (sock= nn_socket(AF_SP,type)) < 0 )
        return(report_err(typestr,sock,"nn_socket",type,bindaddr,connectaddr));
    if ( bindaddr != 0 && bindaddr[0] != 0 )
    {
        //printf("bind\n");
        if ( (err= nn_bind(sock,bindaddr)) < 0 )
            return(report_err(typestr,err,"nn_bind",type,bindaddr,connectaddr));
        if ( timeout > 0 && nn_setsockopt(sock,NN_SOL_SOCKET,NN_RCVTIMEO,&timeout,sizeof(timeout)) < 0 )
            return(report_err(typestr,err,"nn_connect",type,bindaddr,connectaddr));
    }
    if ( connectaddr != 0 && connectaddr[0] != 0 )
    {
        //printf("nn_connect\n");
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
    for (i=0; i<(int32_t)(sizeof(socks->all)/sizeof(*socks->all)); i++)
        if ( socks->all[i] >= 0 && nn_shutdown(socks->all[i],0) != 0 )
            errs++, printf("error (%s) nn_shutdown.%d\n",nn_strerror(nn_errno()),i);
    return(errs);
}

int32_t nn_broadcast(struct allendpoints *socks,uint64_t instanceid,int32_t flags,uint8_t *retstr,int32_t len)
{
    int32_t i,sock,errs = 0;
    for (i=0; i<=(int32_t)(sizeof(socks->send)/sizeof(int32_t))+1; i++)
    {
        if ( i < 2 )
            sock = (i == 0) ? socks->both.bus : socks->both.pair;
        else sock = ((int32_t *)&socks->send)[i - 1];
        if ( sock >= 0 )
        {
            if ( (len= nn_send(sock,(char *)retstr,len,0)) <= 0 )
                errs++, printf("error %d sending to socket.%d send.%d len.%d (%s)\n",len,sock,i,len,nn_strerror(nn_errno()));
            else if ( Debuglevel > 2 )
                printf("SENT.(%s) len.%d vs strlen.%ld instanceid.%llu\n",retstr,len,strlen((char *)retstr),(long long)instanceid);
        }
    }
    return(errs);
}

int32_t poll_endpoints(char *messages[],uint32_t *numrecvp,uint32_t numsent,union endpoints *socks,int32_t timeoutmillis)
{
    struct nn_pollfd pfd[sizeof(struct allendpoints)/sizeof(int32_t)];
    int32_t len,received=0,rc,i,n = 0;
    char *str,*msg;
    memset(pfd,0,sizeof(pfd));
    for (i=0; i<(int32_t)(sizeof(socks->all)/sizeof(*socks->all)); i++)
    {
        if ( (pfd[i].fd= socks->all[i]) >= 0 )
        {
            pfd[i].events = NN_POLLIN | NN_POLLOUT;
            n++;
        }
    }
    if ( n > 0 )
    {
        if ( (rc= nn_poll(pfd,n,timeoutmillis)) > 0 )
        {
            for (i=0; i<n; i++)
            {
                //printf("n.%d i.%d check socket.%d:%d revents.%d\n",n,i,pfd[i].fd,socks->all[i],pfd[i].revents);
                if ( (pfd[i].revents & NN_POLLIN) != 0 && (len= nn_recv(pfd[i].fd,&msg,NN_MSG,0)) > 0 )
                {
                    (*numrecvp)++;
                    str = clonestr(msg);
                    nn_freemsg(msg);
                    messages[received++] = str;
                    if ( Debuglevel > 1 )
                        printf("(%d %d) %d %.6f RECEIVED.%d i.%d/%ld (%s)\n",*numrecvp,numsent,received,milliseconds(),n,i,sizeof(socks->all)/sizeof(*socks->all),str);
                }
            }
        }
        else if ( rc < 0 )
            printf("%s Error.%d polling %d daemons [0] == %d\n",nn_strerror(nn_errno()),rc,n,pfd[0].fd);
    }
    //if ( received != 0 )
    //    printf("received.%d\n",received);
    return(received);
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

int32_t parse_ipaddr(char *ipaddr,char *ip_port)
{
    int32_t j,port = 0;
    if ( ip_port != 0 && ip_port[0] != 0 )
    {
		strcpy(ipaddr, ip_port);
        for (j=0; ipaddr[j]!=0&&j<60; j++)
            if ( ipaddr[j] == ':' )
            {
                port = atoi(ipaddr+j+1);
                break;
            }
        ipaddr[j] = 0;
        printf("(%s) -> (%s:%d)\n",ip_port,ipaddr,port);
    } else strcpy(ipaddr,"127.0.0.1");
    return(port);
}

uint64_t calc_ipbits(char *ip_port)
{
    int32_t port;
    char ipaddr[64];
    struct sockaddr_in addr;
    port = parse_ipaddr(ipaddr,ip_port);
    memset(&addr,0,sizeof(addr));
    portable_pton(ip_port[0] == '[' ? AF_INET6 : AF_INET,ipaddr,&addr);
    {
        int i;
        for (i=0; i<16; i++)
            printf("%02x ",((uint8_t *)&addr)[i]);
        printf("<- %s %x\n",ip_port,*(uint32_t *)&addr);
    }
    return(*(uint32_t *)&addr | ((uint64_t)port << 32));
}

void expand_ipbits(char *ipaddr,uint64_t ipbits)
{
    uint16_t port;
    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    *(uint32_t *)&addr = (uint32_t)ipbits;
    portable_ntop(AF_INET,&addr,ipaddr,64);
    if ( (port= (uint16_t)(ipbits>>32)) != 0 )
        sprintf(ipaddr + strlen(ipaddr),":%d",port);
    //sprintf(ipaddr,"%d.%d.%d.%d",(ipbits>>24)&0xff,(ipbits>>16)&0xff,(ipbits>>8)&0xff,(ipbits&0xff));
}

char *ipbits_str(uint64_t ipbits)
{
    static char ipaddr[64];
    expand_ipbits(ipaddr,ipbits);
    return(ipaddr);
}

char *ipbits_str2(uint64_t ipbits)
{
    static char ipaddr[64];
    expand_ipbits(ipaddr,ipbits);
    return(ipaddr);
}

/*struct sockaddr_in conv_ipbits(uint64_t ipbits)
{
    char ipaddr[64];
    uint16_t port;
    struct hostent *host;
    struct sockaddr_in server_addr;
    port = (uint16_t)(ipbits>>32);
    ipbits = (uint32_t)ipbits;
    expand_ipbits(ipaddr,ipbits);
    host = (struct hostent *)gethostbyname(ipaddr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    memset(&(server_addr.sin_zero),0,8);
    return(server_addr);
}*/
int32_t plugin_result(char *retbuf,cJSON *json,uint64_t tag)
{
    char *error,*result;
    error = cJSON_str(cJSON_GetObjectItem(json,"error"));
    result = cJSON_str(cJSON_GetObjectItem(json,"result"));
    if ( error != 0 || result != 0 )
    {
        sprintf(retbuf,"{\"result\":\"completed\",\"tag\":\"%llu\"}",(long long)tag);
        return(1);
    }
    return(0);
}



#endif
#endif
