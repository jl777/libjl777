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
#include "uthash.h"
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

struct sendendpoints { int32_t push,rep,pub,survey; };
struct recvendpoints { int32_t pull,req,sub,respond; };
struct biendpoints { int32_t bus,pair; };
struct allendpoints { struct sendendpoints send; struct recvendpoints recv; struct biendpoints both; };
union endpoints { int32_t all[sizeof(struct allendpoints) / sizeof(int32_t)]; struct allendpoints socks; };
struct db777_entry { UT_hash_handle hh; uint32_t allocsize,valuelen,valuesize,keylen:30,linked:1,dirty:1; uint8_t value[]; };

struct db777
{
    void *db,*asyncdb;
    portable_mutex_t mutex;
    struct db777_entry *table;
    int32_t reqsock,valuesize,matrixentries;
    uint32_t start_RTblocknum;
    void **matrix; char *dirty;
    char compression[8],dbname[32],name[16],coinstr[16],flags;
    void *ctl,*env; char namestr[32],restoredir[512],argspecialpath[512],argsubdir[512],restorelogdir[512],argname[512],argcompression[512],backupdir[512];
    uint8_t checkbuf[10000000];
};

struct env777
{
    char coinstr[16],subdir[64];
    void *ctl,*env,*transactions;
    struct db777 dbs[16];
    int32_t numdbs,needbackup,lastbackup,currentbackup,matrixentries;
    uint32_t start_RTblocknum;
};

#define DEFAULT_APISLEEP 100  // milliseconds
struct SuperNET_info
{
    char WEBSOCKETD[1024],NXTAPIURL[1024],NXTSERVER[1024],DATADIR[1024],transport[16];
    char myipaddr[64],myNXTacct[64],myNXTaddr[64],NXTACCT[64],NXTADDR[64],NXTACCTSECRET[4096],userhome[512],hostname[512];
    uint64_t my64bits;
    int32_t usessl,ismainnet,Debuglevel,SuperNET_retval,APISLEEP,gatewayid,numgateways,readyflag,UPNP,iamrelay;
    uint16_t port;
    struct env777 DBs;
}; extern struct SuperNET_info SUPERNET;

struct coins_info
{
    int32_t num,readyflag;
    cJSON *argjson;
    struct coin777 **LIST;
    // this will be at the end of the plugins structure and will be called with all zeros to _init
}; extern struct coins_info COINS;

struct db777_info
{
    char PATH[1024],RAMDISK[1024];
    int32_t numdbs,readyflag;
    struct db777 *DBS[1024];
}; extern struct db777_info SOPHIA;

#define MAX_MGWSERVERS 16
struct MGW_info
{
    char PATH[1024],serverips[MAX_MGWSERVERS][64],bridgeipaddr[64],bridgeacct[64];
    uint64_t srv64bits[MAX_MGWSERVERS],issuers[64];
    int32_t M,numissuers,readyflag,port;
    union endpoints all;
    uint32_t numrecv,numsent;
}; extern struct MGW_info MGW;

#define MAX_RAMCHAINS 128
struct ramchain_info
{
    char PATH[1024],coins[MAX_RAMCHAINS][16],pullnode[64];
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


#define MAX_SERVERNAME 128
struct relayargs
{
    char *(*commandprocessor)(struct relayargs *args,uint8_t *msg,int32_t len);
    char name[16],endpoint[MAX_SERVERNAME];
    int32_t lbsock,bussock,pubsock,subsock,peersock,pushsock,sock,type,bindflag,sendtimeout,recvtimeout;
};

#define CONNECTION_NUMBITS 10
struct endpoint { uint64_t ipbits:32,port:16,transport:2,nn:4,directind:CONNECTION_NUMBITS; };
struct _relay_info { int32_t sock,num,mytype,desttype; struct endpoint connections[1 << CONNECTION_NUMBITS]; };
struct direct_connection { char handler[16]; struct endpoint epbits; int32_t sock; };

struct relay_info
{
    struct relayargs args[8];
    struct _relay_info lb,peer,bus,sub,pair;
    int32_t readyflag,pubsock,querypeers,surveymillis,pushsock,pullsock;
    struct direct_connection directlinks[1 << CONNECTION_NUMBITS];
}; extern struct relay_info RELAYS;

void expand_epbits(char *endpoint,struct endpoint epbits);
struct endpoint calc_epbits(char *transport,uint32_t ipbits,uint16_t port,int32_t type);

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
void process_userinput(char *line);

char *get_localtransport(int32_t bundledflag);
int32_t init_socket(char *suffix,char *typestr,int32_t type,char *_bindaddr,char *_connectaddr,int32_t timeout);
int32_t shutdown_plugsocks(union endpoints *socks);
int32_t nn_local_broadcast(struct allendpoints *socks,uint64_t instanceid,int32_t flags,uint8_t *retstr,int32_t len);
int32_t poll_local_endpoints(char *messages[],uint32_t *numrecvp,uint32_t numsent,union endpoints *socks,int32_t timeoutmillis);
struct endpoint nn_directepbits(char *retbuf,char *transport,char *ipaddr,uint16_t port);
int32_t nn_directsend(struct endpoint epbits,uint8_t *msg,int32_t len);

void ensure_directory(char *dirname);
uint32_t is_ipaddr(char *str);

uint64_t calc_ipbits(char *ipaddr);
void expand_ipbits(char *ipaddr,uint64_t ipbits);
char *ipbits_str(uint64_t ipbits);
char *ipbits_str2(uint64_t ipbits);
struct sockaddr_in conv_ipbits(uint64_t ipbits);
uint32_t conv_domainname(char *ipaddr,char *domain);
int32_t ismyaddress(char *server);

void set_endpointaddr(char *transport,char *endpoint,char *domain,uint16_t port,int32_t type);
int32_t nn_portoffset(int32_t type);

char *plugin_method(char **retstrp,int32_t localaccess,char *plugin,char *method,uint64_t daemonid,uint64_t instanceid,char *origargstr,int32_t len,int32_t timeout);
char *nn_direct(char *ipaddr,char *publishstr);
char *nn_publish(char *publishstr,int32_t nostr);
char *nn_allpeers(char *jsonquery,int32_t timeoutmillis,char *localresult);
char *nn_loadbalanced(char *requeststr);
char *relays_jsonstr(char *jsonstr,cJSON *argjson);
struct daemon_info *find_daemoninfo(int32_t *indp,char *name,uint64_t daemonid,uint64_t instanceid);

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

void free_queueitem(void *itemptr) { free((void *)((long)itemptr - sizeof(struct queueitem))); }

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

uint16_t wait_for_myipaddr(char *ipaddr)
{
    uint16_t port = 0;
    printf("need a portable way to find IP addr\n");
    getchar();
    return(port);
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
    if ( Debuglevel > 2 )
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

int32_t nn_local_broadcast(struct allendpoints *socks,uint64_t instanceid,int32_t flags,uint8_t *retstr,int32_t len)
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
                printf("nn_local_broadcast SENT.(%s) len.%d vs strlen.%ld instanceid.%llu -> sock.%d\n",retstr,len,strlen((char *)retstr),(long long)instanceid,sock);
        }
    }
    return(errs);
}

int32_t poll_local_endpoints(char *messages[],uint32_t *numrecvp,uint32_t numsent,union endpoints *socks,int32_t timeoutmillis)
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

void MGW_loop()
{
    int32_t poll_daemons(); char *SuperNET_JSON(char *JSONstr);
    int i,n,timeoutmillis = 10;
    char *messages[100],*msg;
    poll_daemons();
    if ( (n= poll_local_endpoints(messages,&MGW.numrecv,MGW.numsent,&MGW.all,timeoutmillis)) > 0 )
    {
        for (i=0; i<n; i++)
        {
            msg = SuperNET_JSON(messages[i]);
            printf("%d of %d: (%s)\n",i,n,msg);
            free(messages[i]);
        }
    }
}

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
