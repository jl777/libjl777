
//  Created by jl777
//  MIT License
//
// clusterDefiningBlock

#ifndef gateway_jl777_h
#define gateway_jl777_h

//#define MAINNET
//#define DEBUG_MODE
//#define PRODUCTION

// system includes
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <memory.h>
#include <math.h>
//#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>
#include <float.h>
//#include <limits.h>
#include <zlib.h>

#ifndef WIN32
//#include <getopt.h>
//#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
//#include <sys/socket.h>
#include <sys/wait.h>
//#include <sys/time.h>
//#include <pthread.h>

#include <sys/mman.h>   // only for support of map_file and release_map_file in NXTutils.h
#include <fcntl.h>

#define ENABLE_DIRENT
#ifdef ENABLE_DIRENT
#include <sys/stat.h>   // only NXTmembers.h
#include <dirent.h>     //only NXTmembers.h
#endif

#ifdef __APPLE__
#include "libuv/include/uv.h"
#else
#include "libuv/include/uv.h"
#endif
#include <curl/curl.h>
#include <curl/easy.h>

#else
#include "utils/curl/curl.h"
#include "utils/curl/easy.h"

#include <windows.h>
//#include "utils/pthread.h"
#include "../libuv/include/uv.h"
#include "libwebsockets/win32port/win32helpers/gettimeofday.h"
#define STDIN_FILENO 0
void sleep(int);
void usleep(int);
#endif

//#define UDP_OLDWAY
#ifdef UDP_OLDWAY
#define portable_udp_t int32_t
#else
#define portable_udp_t uv_udp_t
#endif

#define portable_tcp_t uv_tcp_t

//#define portable_mutex_t pthread_mutex_t
#define portable_mutex_t uv_mutex_t
#define portable_thread_t uv_thread_t

static int32_t portable_mutex_init(portable_mutex_t *mutex)
{
    return(uv_mutex_init(mutex));
    //pthread_mutex_init(mutex,NULL);
}

static void portable_mutex_lock(portable_mutex_t *mutex)
{
    //printf("lock.%p\n",mutex);
    uv_mutex_lock(mutex);
    // pthread_mutex_lock(mutex);
}

static void portable_mutex_unlock(portable_mutex_t *mutex)
{
   // printf("unlock.%p\n",mutex);
    uv_mutex_unlock(mutex);
    //pthread_mutex_unlock(mutex);
}

static portable_thread_t *portable_thread_create(void *funcp,void *argp)
{
    portable_thread_t *ptr;
    ptr = (uv_thread_t *)malloc(sizeof(portable_thread_t));
    if ( uv_thread_create(ptr,funcp,argp) != 0 )
        //if ( pthread_create(ptr,NULL,funcp,argp) != 0 )
    {
        free(ptr);
        return(0);
    } else return(ptr);
}

// includes that include actual code
#include "nacl/crypto_box.h"
#include "nacl/randombytes.h"
void *jl777malloc(size_t allocsize) { void *ptr = malloc(allocsize); if ( ptr == 0 ) { printf("malloc(%ld) failed\n",allocsize); while ( 1 ) sleep(60); } return(ptr); }
void *jl777calloc(size_t num,size_t allocsize) { void *ptr = calloc(num,allocsize); if ( ptr == 0 ) { printf("calloc(%ld,%ld) failed\n",num,allocsize); while ( 1 ) sleep(60); } return(ptr); }
#define malloc jl777malloc
#define calloc jl777calloc

//#include "guardians.h"
#include "utils/cJSON.h"
#include "utils/jl777str.h"
#include "utils/cJSON.c"
#include "utils/bitcoind_RPC.c"
#include "utils/jsoncodec.h"
//#include "libtom/crypt_argchk.c"
//#include "libtom/sha256.c"
//#include "libtom/rmd160.c"

#define ILLEGAL_COIN "ERR"
#define ILLEGAL_COINASSET "666"

#define MGW_PENDING_DEPOSIT -1
#define MGW_PENDING_WITHDRAW -2
#define MGW_FUND_INSTANTDEX -3
#define MGW_INSTANTDEX_TONXTAE -4

#define NXT_COINASSET "0"
#define CGB_COINASSET "3033014595361865200"
#define DOGE_COINASSET "8011756047853511145"   //"17694574681003862481"
#define USD_COINASSET "4562283093369359331"
#define CNY_COINASSET "13983943517283353302"

#define MIXER_NXTADDR "1234567890123456789"
#define NODECOIN_POOLSERVER_ADDR "11445347041779652448"

#define GENESISACCT "1739068987193023818"
#define NODECOIN_SIG 0x63968736
#define NXTCOINSCO_PORT 8777
#define NXTPROTOCOL_WEBJSON 7777
#define NXT_PUNCH_PORT 6777
#define NXTSYNC_PORT 5777

#define NUM_GATEWAYS 3
#ifdef PRODUCTION
#define MIN_NXTCONFIRMS 10
#define SERVER_NAMEA "65.49.77.102"
#define SERVER_NAMEB "65.49.77.114"
#define SERVER_NAMEC "65.49.77.115"
#define EMERGENCY_PUNCH_SERVER "65.49.77.103"
#define POOLSERVER "65.49.77.104"
#define MIXER_ADDR "65.49.77.105"

#define NXTACCTA "3144696369927305525"
#define NXTACCTB "17410682079219394914"
#define NXTACCTC "14311766766716340607"
#define NXTACCTD "14311766766716340607"
#define NXTACCTE "14311766766716340607"
#else

#define MIN_NXTCONFIRMS 5
#define SERVER_NAMEA "209.126.71.170"
#define SERVER_NAMEB "209.126.73.156"
#define SERVER_NAMEC "209.126.73.158"

#define EMERGENCY_PUNCH_SERVER SERVER_NAMEA 
#define POOLSERVER SERVER_NAMEB
#define MIXER_ADDR SERVER_NAMEC

#define NXTACCTA "423766016895692955"
#define NXTACCTB "12240549928875772593"
#define NXTACCTC "8279528579993996036"
#define NXTACCTD "5723512772332443130"
#define NXTACCTE "15740288657919119263"
#endif


#define _NXTSERVER "requestType"
#ifdef MAINNET
#define NXT_FORKHEIGHT 173271
#define NXTSERVER "http://127.0.0.1:7876/nxt?requestType"
#define NXTAPIURL "http://127.0.0.1:7876/nxt"
#define ORIGBLOCK "14398161661982498695"    //"91889681853055765";//"16787696303645624065";

#define BTC_COINASSET "4551058913252105307"
#define LTC_COINASSET "2881764795164526882"
#define DRK_COINASSET "17353118525598940144"

#define NXTISSUERACCT "7117166754336896747"     // multigateway MGW is the issuer
#define NODECOIN "11749590149008849562"
#else
#define NXT_FORKHEIGHT 0

#define NXTISSUERACCT "18232225178877143084"
#define NODECOIN "9096665501521699628"

#define NXTSERVER "http://127.0.0.1:6876/nxt?requestType"
#define NXTAPIURL "http://127.0.0.1:6876/nxt"
#define ORIGBLOCK "16787696303645624065"    //"91889681853055765";//"16787696303645624065";

#define BTC_COINASSET "1639299849328439538"
#define LTC_COINASSET "1994770251775275406"
#define DRK_COINASSET "4731882869050825869"
#endif

#define USD "3759130218572630531"
#define CNY "17293030412654616962"
#define DEFAULT_NXT_DEADLINE 720

#define SERVER_PORT 3013
#define SERVER_PORTSTR "3013"

#define SATOSHIDEN 100000000L
#define MIN_NQTFEE SATOSHIDEN
#define NXT_TOKEN_LEN 160
#define MAX_NXT_STRLEN 24
#define MAX_NXTTXID_LEN MAX_NXT_STRLEN
#define MAX_NXTADDR_LEN MAX_NXT_STRLEN
#define POLL_SECONDS 10
#define NXT_ASSETLIST_INCR 100

typedef struct queue
{
	void **buffer;
    int32_t capacity,size,in,out,initflag;
	portable_mutex_t mutex;
	//pthread_cond_t cond_full;
	//pthread_cond_t cond_empty;
} queue_t;
//#define QUEUE_INITIALIZER(buffer) { buffer, sizeof(buffer) / sizeof(buffer[0]), 0, 0, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER }

struct pingpong_queue
{
    char *name;
    queue_t pingpong[2],*destqueue,*errorqueue;
    int32_t (*action)();
};

union NXTtype { uint64_t nxt64bits; uint32_t uval; int32_t val; int64_t lval; double dval; char *str; cJSON *json; };

struct NXT_AMhdr
{
    uint32_t sig;
    int32_t size;
    uint64_t nxt64bits;
};

struct json_AM
{
    struct NXT_AMhdr H;
	uint32_t funcid,gatewayid,timestamp,jsonflag;
    union { unsigned char binarydata[sizeof(struct compressed_json)]; char jsonstr[sizeof(struct compressed_json)]; struct compressed_json jsn; };
};

struct NXT_str
{
    uint64_t modified,nxt64bits;
    union { char txid[MAX_NXTTXID_LEN]; char NXTaddr[MAX_NXTADDR_LEN];  char assetid[MAX_NXT_STRLEN]; };
};

struct NXThandler_info
{
    double fractured_prob;  // probability NXT network is fractured, eg. major fork or attack in progress
    int32_t upollseconds,pollseconds,firsttimestamp,timestamp,RTflag,NXTheight,hashprocessing;
    int64_t acctbalance;
    uint64_t blocks[1000 * 365 * 10]; // fix in 10 years
    portable_mutex_t hash_mutex;
    void *handlerdata;
    char *origblockidstr,lastblock[256],blockidstr[256];
    queue_t hashtable_queue[2];
    struct hashtable **NXTaccts_tablep,**NXTassets_tablep,**NXTasset_txids_tablep,**NXTguid_tablep,**NXTsyncaddrs_tablep;
    cJSON *accountjson;
    unsigned char session_pubkey[crypto_box_PUBLICKEYBYTES],session_privkey[crypto_box_SECRETKEYBYTES];
    char pubkeystr[crypto_box_PUBLICKEYBYTES*2+1];
    void *pm;
    uint64_t nxt64bits,*privacyServers;
    uv_tty_t *stdoutput;
    CURL *curl_handle,*curl_handle2,*curl_handle3;
    portable_tcp_t Punch_tcp;
    uv_udp_t Punch_udp;
    uv_connect_t Punch_connect;
    int32_t waitforloading,initassets;
    int32_t UDPserver,extraconfirms,maxpopdepth,maxpopheight,lastchanged,GLEFU,numPrivacyServers,msigs_processed;
    int32_t numblocks,numforging,timestamps[1000 * 365 * 10];//,timestampfifo[MIN_NXTCONFIRMS*100];
    int32_t isudpserver,istcpserver,corresponding,myind,NXTsync_sock,height,prevheight,hackindex;
    char ipaddr[64],NXTAPISERVER[128],NXTADDR[64],NXTACCTSECRET[256],dispname[128],groupname[128];
    char NXTURL[128],Punch_servername[128],Punch_connect_id[512],otherNXTaddr[MAX_NXTADDR_LEN],terminationblock[128];
};
struct NXT_acct *get_NXTacct(int32_t *createdp,struct NXThandler_info *mp,char *NXTaddr);
extern struct NXThandler_info *Global_mp;

#define NXTPROTOCOL_INIT -1
#define NXTPROTOCOL_IDLETIME 0
#define NXTPROTOCOL_NEWBLOCK 1
#define NXTPROTOCOL_AMTXID 2
#define NXTPROTOCOL_TYPEMATCH 3
#define NXTPROTOCOL_ILLEGALTYPE 666

struct NXT_protocol_parms
{
    cJSON *argjson;
    int32_t mode,type,subtype,priority,height;
    char *txid,*sender,*receiver,*argstr;
    void *AMptr;
    char *assetid,*comment;
    int64_t assetoshis;
};

typedef void *(*NXT_handler)(struct NXThandler_info *mp,struct NXT_protocol_parms *parms,void *handlerdata,int32_t height);

struct NXT_protocol
{
    int32_t type,subtype,priority;
    uint32_t AMsigfilter;
    void *handlerdata;
    char **assetlist,**whitelist;
    NXT_handler NXT_handler;
    char name[64];
    char *retjsontxt;
    long retjsonsize;
};

struct NXT_protocol *NXThandlers[1000]; int Num_NXThandlers;

#define SETBIT(bits,bitoffset) (((unsigned char *)bits)[(bitoffset) >> 3] |= (1 << ((bitoffset) & 7)))
#define GETBIT(bits,bitoffset) (((unsigned char *)bits)[(bitoffset) >> 3] & (1 << ((bitoffset) & 7)))
#define CLEARBIT(bits,bitoffset) (((unsigned char *)bits)[(bitoffset) >> 3] &= ~(1 << ((bitoffset) & 7)))
#ifndef MIN
#define MIN(x,y) (((x)<=(y)) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x,y) (((x)>=(y)) ? (x) : (y))
#endif
typedef char *(*json_handler)(char *NXTaddr,int32_t valid,cJSON **objs,int32_t numobjs);

char *bitcoind_RPC(CURL *curl_handle,char *debugstr,char *url,char *userpass,char *command,char *args);
#define issue_curl(curl_handle,cmdstr) bitcoind_RPC(curl_handle,"curl",cmdstr,0,0,0)
#define issue_NXT(curl_handle,cmdstr) bitcoind_RPC(curl_handle,"NXT",cmdstr,0,0,0)
#define issue_NXTPOST(curl_handle,cmdstr) bitcoind_RPC(curl_handle,"curl",NXTAPIURL,0,0,cmdstr)
#define fetch_URL(curl_handle,cmdstr) bitcoind_RPC(curl_handle,"fetch",cmdstr,0,0,0)
void gen_testforms();
#include "NXTservices.h"
#include "utils/jl777hash.h"
//#include "crossplatform.h"

#include "utils/NXTutils.h"
//#include "utils/NXTsock.h"
//#include "punch.h"
//#include "NXTprivacy.h"

#endif
