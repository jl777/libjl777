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

#define DEFAULT_APISLEEP 100
struct SuperNET_info
{
    char WEBSOCKETD[1024],NXTAPIURL[1024],NXTSERVER[1024],DATADIR[1024];
    char myipaddr[64],myNXTacct[64],myNXTaddr[64],NXTACCT[64],NXTADDR[64],NXTACCTSECRET[4096],userhome[512],hostname[512];
    uint64_t my64bits;
    int32_t usessl,ismainnet,Debuglevel,SuperNET_retval,APISLEEP,europeflag,readyflag,UPNP,iamrelay;
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

struct peers_info
{
    int32_t readyflag;
}; extern struct peers_info PEERS;

struct subscriptions_info
{
    char **publications;
    int32_t readyflag,numpubs;
}; extern struct subscriptions_info SUBSCRIPTIONS;

struct _relay_info { int32_t sock,num; uint64_t servers[4096]; };
struct relay_info
{
    struct _relay_info lb,peer,bus,sub;
    int32_t readyflag,pubsock,querypeers,surveymillis;
}; extern struct relay_info RELAYS;

#define MAX_SERVERNAME 128
struct relayargs
{
    char *(*commandprocessor)(struct relayargs *args,uint8_t *msg,int32_t len);
    char name[16],endpoint[MAX_SERVERNAME];
    int32_t lbsock,bussock,pubsock,subsock,peersock,sock,type,bindflag,sendtimeout,recvtimeout;
};
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
void complete_relay(struct relayargs *args,char *retstr);

int32_t getline777(char *line,int32_t max);
char *bitcoind_RPC(char **retstrp,char *debugstr,char *url,char *userpass,char *command,char *args);
uint16_t wait_for_myipaddr(char *ipaddr);

char *get_localtransport(int32_t bundledflag);
int32_t init_socket(char *suffix,char *typestr,int32_t type,char *_bindaddr,char *_connectaddr,int32_t timeout);
int32_t shutdown_plugsocks(union endpoints *socks);
int32_t nn_broadcast(struct allendpoints *socks,uint64_t instanceid,int32_t flags,uint8_t *retstr,int32_t len);
int32_t poll_endpoints(char *messages[],uint32_t *numrecvp,uint32_t numsent,union endpoints *socks,int32_t timeoutmillis);
int32_t get_newinput(char *messages[],uint32_t *numrecvp,uint32_t numsent,int32_t permanentflag,union endpoints *socks,int32_t timeoutmillis,void (*funcp)(char *line));
void ensure_directory(char *dirname);
int32_t nn_portoffset(int32_t type);
uint32_t is_ipaddr(char *str);

uint64_t calc_ipbits(char *ipaddr);
void expand_ipbits(char *ipaddr,uint64_t ipbits);
char *ipbits_str(uint64_t ipbits);
char *ipbits_str2(uint64_t ipbits);
struct sockaddr_in conv_ipbits(uint64_t ipbits);
int32_t ismyaddress(char *server);
void set_endpointaddr(char *endpoint,char *domain,uint16_t port,int32_t type);

char *plugin_method(char *previpaddr,char *plugin,char *method,uint64_t daemonid,uint64_t instanceid,char *origargstr,int32_t numiters,int32_t async);
char *nn_publish(char *publishstr,int32_t nostr);
char *nn_allpeers(int32_t peersock,char *jsonquery,int32_t timeoutmillis);
char *nn_loadbalanced(struct relayargs *args,char *requeststr);
char *nn_peers(struct relayargs *args,uint8_t *msg,int32_t len);
char *nn_relays(struct relayargs *args,uint8_t *msg,int32_t len);
char *nn_subscriptions(struct relayargs *args,uint8_t *msg,int32_t len);


#endif
#else
#ifndef crypto777_system777_c
#define crypto777_system777_c

#ifndef crypto777_system777_h
#define DEFINES_ONLY
#include "system777.c"
#undef DEFINES_ONLY
#endif

int32_t nn_typelist[] = { NN_REP, NN_REQ, NN_RESPONDENT, NN_SURVEYOR, NN_PUB, NN_SUB, NN_PULL, NN_PUSH, NN_BUS, NN_PAIR };

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
    else
    {
        if ( FD_ISSET(STDIN_FILENO,&fdset) > 0 && fgets(line,max,stdin) == line )
            line[strlen(line)-1] = 0;
    }
    return((int32_t)strlen(line));
}

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
        case NN_BUS: return(NN_BUS); break;
        case NN_PAIR: return(NN_PAIR); break;
    }
    return(-1);
}

int32_t nn_portoffset(int32_t type)
{
    int32_t i;
    for (i=0; i<(int32_t)(sizeof(nn_typelist)/sizeof(*nn_typelist)); i++)
        if ( nn_typelist[i] == type )
            return((i+10));
    return(-1);
}

void set_endpointaddr(char *endpoint,char *domain,uint16_t port,int32_t type)
{
    sprintf(endpoint,"tcp://%s:%d",domain,port + nn_portoffset(type));
}

int32_t badass_servers(char servers[][MAX_SERVERNAME],int32_t max,int32_t port)
{
    int32_t n = 0;
    strcpy(servers[n++],"89.248.160.237");
    strcpy(servers[n++],"89.248.160.238");
    strcpy(servers[n++],"89.248.160.239");
    strcpy(servers[n++],"89.248.160.240");
    strcpy(servers[n++],"89.248.160.241");
    strcpy(servers[n++],"89.248.160.242");
    strcpy(servers[n++],"89.248.160.243");
    return(n);
}

int32_t crackfoo_servers(char servers[][MAX_SERVERNAME],int32_t max,int32_t port)
{
    //static char *tcpformat = "ps%02d.bitcoindark.ca";
    int32_t n = 0;
    strcpy(servers[n++],"192.99.151.160");
    strcpy(servers[n++],"167.114.96.223");
    strcpy(servers[n++],"167.114.113.197");
    //int32_t i,n = 0;
    //for (i=0; i<=20&&n<max; i++,n++)
    //    sprintf(servers[i],tcpformat,i);
    return(n);
}

int32_t parse_ipaddr(char *ipaddr,char *ip_port)
{
    int32_t j,port = 0;
    if ( ip_port != 0 && ip_port[0] != 0 )
    {
		strcpy(ipaddr,ip_port);
        for (j=0; ipaddr[j]!=0&&j<60; j++)
            if ( ipaddr[j] == ':' )
            {
                port = atoi(ipaddr+j+1);
                break;
            }
        ipaddr[j] = 0;
        //printf("(%s) -> (%s:%d)\n",ip_port,ipaddr,port);
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
    if ( 0 )
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

uint32_t is_ipaddr(char *str)
{
    uint64_t ipbits; char ipaddr[64];
    if ( str != 0 && str[0] != 0 && (ipbits= calc_ipbits(str)) != 0 )
    {
        expand_ipbits(ipaddr,(uint32_t)ipbits);
        if ( strncmp(ipaddr,str,strlen(ipaddr)) == 0 )
            return((uint32_t)ipbits);
    }
   // printf("(%s) is not ipaddr\n",str);
    return(0);
}

uint32_t conv_domainname(char *ipaddr,char *domain)
{
    int32_t conv_domain(struct sockaddr_storage *ss,const char *addr,int32_t ipv4only);
    int32_t ipv4only = 1;
    uint32_t ipbits;
    struct sockaddr_in ss;
    if ( conv_domain((struct sockaddr_storage *)&ss,(const char *)domain,ipv4only) == 0 )
    {
        ipbits = *(uint32_t *)&ss.sin_addr;
        expand_ipbits(ipaddr,ipbits);
        if ( (uint32_t)calc_ipbits(ipaddr) == ipbits )
            return(ipbits);
        //printf("conv_domainname (%s) -> (%s)\n",domain,ipaddr);
    } //else printf("error conv_domain.(%s)\n",domain);
    return(0);
}

int32_t ismyaddress(char *server)
{
    char ipaddr[64]; uint32_t ipbits;
    if ( strncmp(server,"tcp://",6) == 0 )
        server += 6;
    else if ( strncmp(server,"ws://",5) == 0 )
        server += 5;
    if ( (ipbits= is_ipaddr(server)) != 0 )
    {
        if ( strcmp(server,SUPERNET.myipaddr) == 0 || calc_ipbits(SUPERNET.myipaddr) == ipbits )
        {
            printf("(%s) MATCHES me (%s)\n",server,SUPERNET.myipaddr);
            return(1);
        }
    }
    else
    {
        if ( SUPERNET.hostname[0] != 0 && strcmp(SUPERNET.hostname,server) == 0 )
            return(1);
        if ( conv_domainname(ipaddr,server) == 0 && (strcmp(SUPERNET.myipaddr,ipaddr) == 0 || strcmp(SUPERNET.hostname,ipaddr) == 0) )
            return(1);
    }
    //printf("(%s) is not me (%s)\n",server,SUPERNET.myipaddr);
    return(0);
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

int32_t nn_addservers(int32_t priority,int32_t sock,char servers[][MAX_SERVERNAME],int32_t num)
{
    int32_t add_relay(struct _relay_info *list,uint64_t ipbits);
    int32_t i; char endpoint[512];
    if ( num > 0 && servers != 0 && nn_setsockopt(sock,NN_SOL_SOCKET,NN_SNDPRIO,&priority,sizeof(priority)) >= 0 )
    {
        for (i=0; i<num; i++)
        {
            set_endpointaddr(endpoint,servers[i],SUPERNET.port,NN_REP);
            if ( ismyaddress(servers[i]) == 0 && nn_connect(sock,endpoint) >= 0 )
            {
                printf("+%s ",endpoint);
                add_relay(&RELAYS.lb,calc_ipbits(servers[i]));
            }
        }
        priority++;
    } else printf("error setting priority.%d (%s)\n",priority,nn_errstr());
    return(priority);
}

int32_t nn_loadbalanced_socket(int32_t retrymillis,char servers[][MAX_SERVERNAME],int32_t num,char backups[][MAX_SERVERNAME],int32_t numbacks,char failsafes[][MAX_SERVERNAME],int32_t numfailsafes)
{
    int32_t lbsock,timeout,priority = 1; //char *fallback = "tcp://209.126.70.170:4010";
    if ( (lbsock= nn_socket(AF_SP,NN_REQ)) >= 0 )
    {
        //printf("!!!!!!!!!!!! lbsock.%d !!!!!!!!!!!\n",lbsock);
        if ( nn_setsockopt(lbsock,NN_SOL_SOCKET,NN_RECONNECT_IVL_MAX,&retrymillis,sizeof(retrymillis)) < 0 )
            printf("error setting NN_REQ NN_RECONNECT_IVL_MAX socket %s\n",nn_errstr());
        timeout = 1000;
        if ( nn_setsockopt(lbsock,NN_SOL_SOCKET,NN_RCVTIMEO,&timeout,sizeof(timeout)) < 0 )
            printf("error setting NN_SOL_SOCKET NN_RCVTIMEO socket %s\n",nn_errstr());
        timeout = 10;
        if ( nn_setsockopt(lbsock,NN_SOL_SOCKET,NN_SNDTIMEO,&timeout,sizeof(timeout)) < 0 )
            printf("error setting NN_SOL_SOCKET NN_SNDTIMEO socket %s\n",nn_errstr());
        priority = nn_addservers(priority,lbsock,servers,num);
        priority = nn_addservers(priority,lbsock,backups,numbacks);
        priority = nn_addservers(priority,lbsock,failsafes,numfailsafes);
    } else printf("error getting req socket %s\n",nn_errstr());
    return(lbsock);
}

int32_t loadbalanced_socket(int32_t retrymillis,int32_t port)
{
    char Cservers[32][MAX_SERVERNAME],Bservers[32][MAX_SERVERNAME],failsafes[4][MAX_SERVERNAME];
    int32_t n,m,lbsock,numfailsafes = 0;
    strcpy(failsafes[numfailsafes++],"jnxt.org");
    strcpy(failsafes[numfailsafes++],"209.126.70.156");
    strcpy(failsafes[numfailsafes++],"209.126.70.159");
    strcpy(failsafes[numfailsafes++],"209.126.70.170");
    n = crackfoo_servers(Cservers,sizeof(Cservers)/sizeof(*Cservers),port);
    m = badass_servers(Bservers,sizeof(Bservers)/sizeof(*Bservers),port);
    //if ( europeflag != 0 )
    //    lbsock = nn_loadbalanced_socket(retrymillis,Bservers,m,Cservers,n,failsafes,numfailsafes);
    //else lbsock = nn_loadbalanced_socket(retrymillis,Cservers,n,Bservers,m,failsafes,numfailsafes);
    lbsock = nn_loadbalanced_socket(retrymillis,Bservers,m,Cservers,n,failsafes,numfailsafes);
    return(lbsock);
}

int32_t nn_createsocket(char *endpoint,int32_t bindflag,char *name,int32_t type,uint16_t port,int32_t sendtimeout,int32_t recvtimeout)
{
    int32_t sock;
    set_endpointaddr(endpoint,"*",SUPERNET.port,type);
    if ( (sock= nn_socket(AF_SP,type)) < 0 )
        printf("error getting socket %s\n",nn_errstr());
    if ( bindflag != 0 && nn_bind(sock,endpoint) < 0 )
        printf("error binding to relaypoint sock.%d type.%d to (%s) (%s) %s\n",sock,type,name,endpoint,nn_errstr());
    //else if ( bindflag == 0 && nn_connect(sock,endpoint) < 0 )
    //    printf("error connecting to relaypoint sock.%d type.%d to (%s) (%s) %s\n",sock,type,name,endpoint,nn_errstr());
    else
    {
        if ( sendtimeout > 0 && nn_setsockopt(sock,NN_SOL_SOCKET,NN_SNDTIMEO,&sendtimeout,sizeof(sendtimeout)) < 0 )
            printf("error setting sendtimeout %s\n",nn_errstr());
        else if ( recvtimeout > 0 && nn_setsockopt(sock,NN_SOL_SOCKET,NN_RCVTIMEO,&recvtimeout,sizeof(recvtimeout)) < 0 )
            printf("error setting sendtimeout %s\n",nn_errstr());
        else
        {
            printf("nn_createsocket.(%s) %d\n",name,sock);
            return(sock);
        }
    }
    return(-1);
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

void run_device(void *_args)
{
    struct nn_pollfd *pfds = _args;
    printf(">>>>>>>>>>>>>> RUNDEVICE.(%d <-> %d)\n",pfds[0].fd,pfds[1].fd);
    nn_device(pfds[0].fd,pfds[1].fd);
}

int32_t start_devices(int32_t type)
{
    int32_t i,j,n,err,numtypes,sock,portoffset,devicetypes[] = { NN_RESPONDENT }; //NN_REP, NN_PUB, NN_PULL };
    static struct nn_pollfd pfds[4][2];
    char bindaddr[128];
    numtypes = (int32_t)(sizeof(devicetypes)/sizeof(*devicetypes));
    memset(pfds,0xff,sizeof(pfds));
    devicetypes[0] = type;
    for (i=n=0; i<1; i++)
    {
        for (j=err=0; j<2; j++,n++)
        {
            type = (j == 0) ? devicetypes[i] : nn_oppotype(devicetypes[i]);
            portoffset = nn_portoffset(type);
            set_endpointaddr(bindaddr,"*",SUPERNET.port,type);
            if ( (sock= nn_socket(AF_SP_RAW,type)) < 0 )
                break;
            if ( (err= nn_bind(sock,bindaddr)) < 0 )
                break;
            printf("(%d) type.%d bindaddr.(%s) sock.%d\n",devicetypes[i],type,bindaddr,sock);
            pfds[i][j].fd = sock;
            pfds[i][j].events = NN_POLLIN | NN_POLLOUT;
        }
        portable_thread_create((void *)run_device,&pfds[i][0]);
        if ( j != 2 )
        {
            printf("error.%d launching relays at i.%d of %d j.%d %s\n",err,i,numtypes,j,nn_errstr());
            break;
        }
    }
    return(pfds[0][0].fd);
}

void add_standard_fields(char *request)
{
    sprintf(request + strlen(request) - 1,",\"NXT\":\"%s\",\"tag\":\"%llu\"}",SUPERNET.NXTADDR,(((long long)rand() << 32) | (uint32_t)rand()));
    if ( SUPERNET.iamrelay != 0 && (SUPERNET.hostname[0] != 0 || SUPERNET.myipaddr[0] != 0) )
        sprintf(request + strlen(request) - 1,",\"iamrelay\":\"%s\"}",SUPERNET.hostname[0]!=0?SUPERNET.hostname:SUPERNET.myipaddr);
}

char *nn_loadbalanced(struct relayargs *args,char *_request)
{
    char *msg,*request,*jsonstr = 0;
    int32_t len,sendlen,i,recvlen = 0;
    if ( args->lbsock < 0 )
        return(clonestr("{\"error\":\"invalid load balanced socket\"}"));
    request = malloc(strlen(_request) + 512);
    strcpy(request,_request);
    add_standard_fields(request);
    len = (int32_t)strlen(request) + 1;
    for (i=0; i<1000; i++)
        if ( (get_socket_status(args->lbsock,1) & NN_POLLOUT) != 0 )
            break;
    if ( (sendlen= nn_send(args->lbsock,request,len,0)) == len )
    {
        for (i=0; i<1000; i++)
            if ( (get_socket_status(args->lbsock,1) & NN_POLLIN) != 0 )
                break;
        if ( (recvlen= nn_recv(args->lbsock,&msg,NN_MSG,0)) > 0 )
        {
            jsonstr = clonestr((char *)msg);//(*args->commandprocessor)(args,(uint8_t *)msg,len);
            nn_freemsg(msg);
        }
        else
        {
            printf("got recvlen.%d %s\n",recvlen,nn_errstr());
            jsonstr = clonestr("{\"error\":\"lb send error\"}");
        }
    } else printf("got sendlen.%d instead of %d %s\n",sendlen,len,nn_errstr()), jsonstr = clonestr("{\"error\":\"lb send error\"}");
    free(request);
    return(jsonstr);
}

char *nn_allpeers(int32_t peersock,char *_request,int32_t timeoutmillis)
{
    cJSON *item,*json,*array = 0;
    int32_t i,sendlen,len,n = 0;
    double startmilli;
    char *request;
    char *msg,*retstr;
    if ( timeoutmillis == 0 )
        timeoutmillis = 500;
    if ( peersock < 0 )
        return(clonestr("{\"error\":\"invalid peers socket\"}"));
    if ( nn_setsockopt(peersock,NN_SURVEYOR,NN_SURVEYOR_DEADLINE,&timeoutmillis,sizeof(timeoutmillis)) < 0 )
    {
        printf("error nn_setsockopt %d %s\n",peersock,nn_errstr());
        return(clonestr("{\"error\":\"setting NN_SURVEYOR_DEADLINE\"}"));
    }
    request = malloc(strlen(_request) + 512);
    strcpy(request,_request);
    add_standard_fields(request);
    printf("request_allpeers.(%s)\n",request);
    for (i=0; i<1000; i++)
        if ( (get_socket_status(peersock,1) & NN_POLLOUT) != 0 )
            break;
    len = (int32_t)strlen(request) + 1;
    startmilli = milliseconds();
    if ( (sendlen= nn_send(peersock,request,len,0)) == len )
    {
        for (i=0; i<1000; i++)
            if ( (get_socket_status(peersock,1) & NN_POLLIN) != 0 )
                break;
        while ( (len= nn_recv(peersock,&msg,NN_MSG,0)) > 0 )
        {
            if ( (item= cJSON_Parse(msg)) != 0 )
            {
                if ( array == 0 )
                    array = cJSON_CreateArray();
                cJSON_AddItemToObject(item,"lag",cJSON_CreateNumber(milliseconds()-startmilli));
                cJSON_AddItemToArray(array,item);
                n++;
            }
            nn_freemsg(msg);
        }
    } else printf("nn_allpeers: sendlen.%d vs len.%d\n",sendlen,len);
    if ( n == 0 )
    {
        free_json(array);
        free(request);
        return(clonestr("{\"error\":\"no responses\"}"));
    }
    json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"responses",array);
    cJSON_AddItemToObject(json,"n",cJSON_CreateNumber(n));
    retstr = cJSON_Print(json);
    _stripwhite(retstr,' ');
    //printf("globalrequest(%s) returned (%s) from n.%d respondents\n",request,retstr,n);
    free_json(json);
    free(request);
    return(retstr);
}

char *nn_relays(struct relayargs *args,uint8_t *msg,int32_t len)
{
    cJSON *json; char *jsonstr,*plugin,*retstr = 0;
    jsonstr = (char *)msg;
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        if ( (plugin= cJSON_str(cJSON_GetObjectItem(json,"plugin"))) != 0 )
        {
            if ( strcmp(plugin,"subscriptions") == 0 )
                retstr = nn_subscriptions(args,msg,len);
            else if ( strcmp(plugin,"peers") == 0 )
                retstr = nn_peers(args,msg,len);
            else retstr = plugin_method("remote",plugin,(char *)args,0,0,(char *)msg,len,1000);
        }
        else
        {
            retstr = plugin_method("remote","relay",(char *)args,0,0,(char *)msg,len,1000);
            printf("returnpath.(%s) (%s) -> (%s)\n",args->name,jsonstr,retstr);
        }
        free_json(json);
    } else retstr = clonestr("{\"error\":\"couldnt parse request\"}");
    return(retstr);
}

char *nn_subscriptions(struct relayargs *args,uint8_t *msg,int32_t len)
{
    cJSON *json; char *plugin,*retstr = 0;
    if ( (json= cJSON_Parse((char *)msg)) != 0 )
    {
        if ( (plugin= cJSON_str(cJSON_GetObjectItem(json,"plugin"))) != 0 )
        {
            if ( strcmp(plugin,"relay") == 0 )
                retstr = nn_relays(args,msg,len);
            else if ( strcmp(plugin,"peers") == 0 )
                retstr = nn_peers(args,msg,len);
            else retstr = plugin_method("remote",plugin,(char *)args,0,0,(char *)msg,len,1000);
        }
        retstr = plugin_method("remote",plugin==0?"subscriptions":plugin,(char *)args,0,0,(char *)msg,len,1000);
        free_json(json);
    } else retstr = clonestr((char *)msg);
    return(retstr);
}

char *nn_peers(struct relayargs *args,uint8_t *msg,int32_t len)
{
    cJSON *json; char *plugin,*retstr = 0;
    if ( (json= cJSON_Parse((char *)msg)) != 0 )
    {
        if ( (plugin= cJSON_str(cJSON_GetObjectItem(json,"plugin"))) != 0 )
        {
            if ( strcmp(plugin,"subscriptions") == 0 )
                retstr = nn_subscriptions(args,msg,len);
            else retstr = plugin_method("remote",plugin==0?"peers":plugin,(char *)args,0,0,(char *)msg,len,1000);
        }
        free_json(json);
    } else retstr = clonestr("{\"error\":\"couldnt parse request\"}");
    return(retstr);
}

void complete_relay(struct relayargs *args,char *retstr)
{
    int32_t len,sendlen;
    _stripwhite(retstr,' ');
    len = (int32_t)strlen(retstr)+1;
    if ( args->type != NN_SUB && (sendlen= nn_send(args->sock,retstr,len,0)) != len )
        printf("complete_relay.%s warning: send.%d vs %d for (%s) sock.%d %s\n",args->name,sendlen,len,retstr,args->sock,nn_errstr());
    //else printf("SUCCESS complete_relay.(%s) -> sock.%d %s\n",retstr,args->sock,args->name);
}

char *nn_publish(char *publishstr,int32_t nostr)
{
    int32_t len,sendlen = -1;
    char retbuf[1024];
    if ( publishstr != 0 )
    {
        len = (int32_t)strlen(publishstr) + 1;
        if ( (sendlen= nn_send(RELAYS.pubsock,publishstr,len,0)) != len )
            printf("add_connections warning: send.%d vs %d for (%s) sock.%d %s\n",sendlen,len,publishstr,RELAYS.pubsock,nn_errstr());
        else printf("published.(%s)\n",publishstr);
        sprintf(retbuf,"{\"result\":\"published\",\"len\":%d,\"sendlen\":%d,\"crc\":%u}",len,sendlen,_crc32(0,publishstr,(int32_t)strlen(publishstr)));
        if ( nostr != 0 )
            return(0);
    }
    else if ( nostr == 0 )
        strcpy(retbuf,"{\"error\":\"null publishstr\"}");
    return(clonestr(retbuf));
}

void responseloop(void *_args)
{
    struct relayargs *args = _args;
    int32_t len; char *msg,*retstr,*broadcaststr; cJSON *json;
    if ( args->sock >= 0 )
    {
        printf("respondloop.%s %d type.%d <- (%s).%d\n",args->name,args->sock,args->type,args->endpoint,nn_oppotype(args->type));
        while ( 1 )
        {
            if ( (len= nn_recv(args->sock,&msg,NN_MSG,0)) > 0 )
            {
                retstr = 0;
                //if ( Debuglevel > 1 )
                    printf("RECV.%s (%s)\n",args->name,msg);
                if ( (json= cJSON_Parse((char *)msg)) != 0 )
                {
                    broadcaststr = cJSON_str(cJSON_GetObjectItem(json,"broadcast"));
                    if ( broadcaststr != 0 && strcmp(broadcaststr,"allpeers") == 0 )
                        retstr = nn_allpeers(RELAYS.querypeers,(char *)msg,3000);
                    free_json(json);
                }
                if ( retstr == 0 && (retstr= (*args->commandprocessor)(args,(uint8_t *)msg,len)) != 0 )
                {
                    complete_relay(args,retstr);
                    free(retstr);
                }
                nn_freemsg(msg);
            }// else fprintf(stderr,".");
        }
    } else printf("error getting socket type.%d %s\n",args->type,nn_errstr());
}

int32_t launch_responseloop(struct relayargs *args,char *name,int32_t type,int32_t bindflag,char *(*commandprocessor)(struct relayargs *,uint8_t *msg,int32_t len))
{
    if ( type != NN_RESPONDENT && type != NN_REP && type != NN_SUB && type != NN_BUS && type != NN_PULL )
    {
        printf("responder loop doesnt deal with type.%d %s\n",type,name);
        return(-1);
    }
    strcpy(args->name,name), args->type = type, args->commandprocessor = commandprocessor, args->bindflag = bindflag;
    if ( args->sendtimeout == 0 )
        args->sendtimeout = 10;
    if ( args->recvtimeout == 0 )
        args->recvtimeout = 10000;
    if ( (args->sock= nn_createsocket(args->endpoint,bindflag,name,type,SUPERNET.port,args->sendtimeout,args->recvtimeout)) >= 0 )
        portable_thread_create((void *)responseloop,args);
    else printf("error getting nn_createsocket.%d %s %s\n",type,name,nn_errstr());
    return(args->sock);
}

void process_userinput(struct relayargs *lbargs,char *line)
{
    char plugin[512],method[512],*str,*cmdstr,*retstr,*pubstr; cJSON *json; int i,j,broadcastflag = 0;
    printf("[%s]\n",line);
    if ( line[0] == '!' )
        broadcastflag = 1, line++;
    for (i=0; i<512&&line[i]!=' '&&line[i]!=0; i++)
        plugin[i] = line[i];
    plugin[i] = 0;
    pubstr = line;
    if ( strcmp(plugin,"pub") == 0 )
        strcpy(plugin,"subscriptions"), strcpy(method,"publish"), pubstr += 4;
    else if ( line[i+1] != 0 )
    {
        for (++i,j=0; i<512&&line[i]!=' '&&line[i]!=0; i++,j++)
            method[j] = line[i];
        method[j] = 0;
    } else method[0] = 0;
    if ( (json= cJSON_Parse(line+i+1)) == 0 )
    {
        json = cJSON_CreateObject();
        if ( line[i+1] != 0 )
        {
            str = stringifyM(&line[i+1]);
            cJSON_AddItemToObject(json,"content",cJSON_CreateString(pubstr));
            free(str);
        }
    }
    if ( json != 0 )
    {
        struct daemon_info *find_daemoninfo(int32_t *indp,char *name,uint64_t daemonid,uint64_t instanceid);
        if ( plugin[0] == 0 )
            strcpy(plugin,"relay");
        cJSON_AddItemToObject(json,"plugin",cJSON_CreateString(plugin));
        if ( method[0] == 0 )
            strcpy(method,"help");
        cJSON_AddItemToObject(json,"method",cJSON_CreateString(method));
        if ( broadcastflag != 0 )
            cJSON_AddItemToObject(json,"broadcast",cJSON_CreateString("allpeers"));
        cmdstr = cJSON_Print(json);
        _stripwhite(cmdstr,' ');
        if ( broadcastflag != 0 || strcmp(plugin,"relay") == 0 )
            retstr = nn_loadbalanced(lbargs,cmdstr);
        else if ( strcmp(plugin,"peers") == 0 )
            retstr = nn_allpeers(RELAYS.querypeers,cmdstr,RELAYS.surveymillis);
        else if ( find_daemoninfo(&j,plugin,0,0) != 0 )
            retstr = plugin_method(0,plugin,method,0,0,cmdstr,(int32_t)strlen(cmdstr),1000);
        else retstr = nn_publish(pubstr,0);
        printf("(%s) -> (%s) -> (%s)\n",line,cmdstr,retstr);
        free(cmdstr);
        free_json(json);
    } else printf("cant create json object for (%s)\n",line);
}

void serverloop(void *_args)
{
    static struct relayargs args[8];
    struct relayargs *peerargs,*lbargs,*arg;
    char endpoint[128],request[1024],ipaddr[64],*retstr;
    int32_t i,sendtimeout,recvtimeout,lbsock,bussock,pubsock,peersock,n = 0;
    memset(args,0,sizeof(args));
    //start_devices(NN_RESPONDENT);
    sendtimeout = 10, recvtimeout = 10000;
    RELAYS.querypeers = peersock = nn_createsocket(endpoint,1,"NN_SURVEYOR",NN_SURVEYOR,SUPERNET.port,sendtimeout,recvtimeout);
    peerargs = &args[n++], RELAYS.peer.sock = launch_responseloop(peerargs,"NN_RESPONDENT",NN_RESPONDENT,0,nn_peers);
    pubsock = nn_createsocket(endpoint,1,"NN_PUB",NN_PUB,SUPERNET.port,sendtimeout,-1);
    RELAYS.sub.sock = launch_responseloop(&args[n++],"NN_SUB",NN_SUB,0,nn_subscriptions);
    lbsock = loadbalanced_socket(3000,SUPERNET.port); // NN_REQ
    lbargs = &args[n++];
    if ( SUPERNET.iamrelay != 0 )
    {
        launch_responseloop(lbargs,"NN_REP",NN_REP,1,nn_relays);
        bussock = launch_responseloop(&args[n++],"NN_BUS",NN_BUS,1,nn_relays);
    } else bussock = -1, lbargs->commandprocessor = nn_relays, lbargs->sock = lbsock;
    RELAYS.lb.sock = lbsock, RELAYS.bus.sock = bussock, RELAYS.pubsock = pubsock;
    for (i=0; i<n; i++)
    {
        arg = &args[i];
        arg->lbsock = lbsock;
        arg->bussock = bussock;
        arg->pubsock = pubsock;
        arg->peersock = peersock;
    }
    int32_t add_connections(char *server,int32_t skiplb);
    for (i=0; i<RELAYS.lb.num; i++)
    {
        expand_ipbits(ipaddr,(uint32_t)RELAYS.lb.servers[i]);
        add_connections(ipaddr,1);
    }
    if ( SUPERNET.iamrelay != 0 )
    {
        if ( SUPERNET.hostname[0] != 0 || SUPERNET.myipaddr[0] != 0 )
        {
            sprintf(request,"{\"plugin\":\"relay\",\"method\":\"add\",\"hostnames\":[\"%s\"]}",SUPERNET.hostname[0]!=0?SUPERNET.hostname:SUPERNET.myipaddr);
            if ( (retstr= nn_loadbalanced(lbargs,request)) != 0 )
            {
                printf("LB_RESPONSE.(%s)\n",retstr);
                free(retstr);
            }
        }
    }
    while ( 1 )
    {
#ifdef __APPLE__
        char line[1024];
        if ( getline777(line,sizeof(line)-1) > 0 )
            process_userinput(lbargs,line);
#endif
        int32_t poll_daemons();
        poll_daemons();
        if ( SUPERNET.APISLEEP > 0 )
            msleep(SUPERNET.APISLEEP);
    }
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
    if ( bindaddr[0] != 0 )
    {
        //printf("bind\n");
        if ( (err= nn_bind(sock,bindaddr)) < 0 )
            return(report_err(typestr,err,"nn_bind",type,bindaddr,connectaddr));
        if ( timeout > 0 && nn_setsockopt(sock,NN_SOL_SOCKET,NN_SNDTIMEO,&timeout,sizeof(timeout)) < 0 )
            return(report_err(typestr,err,"nn_connect",type,bindaddr,connectaddr));
        if ( timeout > 0 && nn_setsockopt(sock,NN_SOL_SOCKET,NN_RCVTIMEO,&timeout,sizeof(timeout)) < 0 )
            return(report_err(typestr,err,"nn_connect",type,bindaddr,connectaddr));
    }
    if ( connectaddr[0] != 0 )
    {
        //printf("nn_connect\n");
        if ( (err= nn_connect(sock,connectaddr)) < 0 )
            return(report_err(typestr,err,"nn_connect",type,bindaddr,connectaddr));
        else if ( type == NN_SUB && (err= nn_setsockopt(sock,NN_SUB,NN_SUB_SUBSCRIBE,"",0)) < 0 )
            return(report_err(typestr,err,"nn_setsockopt subscribe",type,bindaddr,connectaddr));
    }
    //if ( Debuglevel > 2 )
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
    for (i=0; i<(int32_t)(sizeof(socks->send)/sizeof(int32_t))+2; i++)
    {
        if ( i < 2 )
            sock = (i == 0) ? socks->both.bus : socks->both.pair;
        else sock = ((int32_t *)&socks->send)[i - 2];
        if ( sock >= 0 )
        {
            if ( (len= nn_send(sock,(char *)retstr,len,0)) <= 0 )
                errs++, printf("error %d sending to socket.%d send.%d len.%d (%s)\n",len,sock,i,len,nn_strerror(nn_errno()));
            else if ( Debuglevel > 2 )
                printf("nn_broadcast SENT.(%s) len.%d vs strlen.%ld instanceid.%llu -> sock.%d\n",retstr,len,strlen((char *)retstr),(long long)instanceid,sock);
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
        if ( (pfd[n].fd= socks->all[i]) >= 0 )
            pfd[n++].events = NN_POLLIN | NN_POLLOUT;
    }
    if ( n > 0 )
    {
        if ( (rc= nn_poll(pfd,n,timeoutmillis)) > 0 )
        {
            for (i=0; i<n; i++)
            {
               // printf("n.%d i.%d check socket.%d:%d revents.%d\n",n,i,pfd[i].fd,socks->all[i],pfd[i].revents);
                if ( (pfd[i].revents & NN_POLLIN) != 0 && (len= nn_recv(pfd[i].fd,&msg,NN_MSG,0)) > 0 )
                {
                    (*numrecvp)++;
                    str = clonestr(msg);
                    nn_freemsg(msg);
                    messages[received++] = str;
                    if ( Debuglevel > 2 )
                        printf("(%d %d) %d %.6f RECEIVED.%d i.%d/%ld (%s)\n",*numrecvp,numsent,received,milliseconds(),n,i,sizeof(socks->all)/sizeof(*socks->all),str);
                }
            }
        }
        else if ( rc < 0 )
            printf("%s Error.%d polling %d daemons [0] == %d\n",nn_strerror(nn_errno()),rc,n,pfd[0].fd);
    }
    //if ( n != 0 || received != 0 )
    //   printf("n.%d: received.%d\n",n,received);
    return(received);
}

int32_t get_newinput(char *messages[],uint32_t *numrecvp,uint32_t numsent,int32_t permanentflag,union endpoints *socks,int32_t timeoutmillis,void (*funcp)(char *line))
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
        {
            if ( funcp != 0 )
                (*funcp)(line);
            else messages[0] = clonestr(line), n = 1;
        }
    }
    return(n);
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
