
//  Created by jl777
//  MIT License
//
// garbage collect transfer args

#ifndef gateway_jl777_h
#define gateway_jl777_h

#define HARDCODED_VERSION "0.707"
//#define TIMESCRAMBLE

#define MGW0_IPADDR "209.126.70.170"
#define MGW1_IPADDR "209.126.70.156"
#define MGW2_IPADDR "209.126.70.159"

#define NXT_GENESISTIME 1385294400
#define MAX_LFACTOR 10
#define MAX_UDPLEN 1400
#define PUBADDRS_MSGDURATION (3600 * 24)
#define MAX_ONION_LAYERS 7
//#define pNXT_SIG 0x99999999
#define MAX_DROPPED_PACKETS 4
#define MAX_MULTISIG_OUTPUTS 16
#define MAX_MULTISIG_INPUTS 256
#define MULTIGATEWAY_VARIANT 3
#define MULTIGATEWAY_SYNCWITHDRAW 0

#define NXT_ASSETID ('N' + ((uint64_t)'X'<<8) + ((uint64_t)'T'<<16))    // 5527630
#define BTC_ASSETID ('B' + ((uint64_t)'T'<<8) + ((uint64_t)'C'<<16))    // 4412482
#define LTC_ASSETID ('L' + ((uint64_t)'T'<<8) + ((uint64_t)'C'<<16))
#define PPC_ASSETID ('P' + ((uint64_t)'P'<<8) + ((uint64_t)'C'<<16))
#define NMC_ASSETID ('N' + ((uint64_t)'M'<<8) + ((uint64_t)'C'<<16))
#define DASH_ASSETID ('D' + ((uint64_t)'A'<<8) + ((uint64_t)'S'<<16) + ((uint64_t)'H'<<24))
#define BTCD_ASSETID ('B' + ((uint64_t)'T'<<8) + ((uint64_t)'C'<<16) + ((uint64_t)'D'<<24))

#define USD_ASSETID ('U' + ((uint64_t)'S'<<8) + ((uint64_t)'D'<<16))
#define CNY_ASSETID ('C' + ((uint64_t)'N'<<8) + ((uint64_t)'Y'<<16))
#define EUR_ASSETID ('E' + ((uint64_t)'U'<<8) + ((uint64_t)'R'<<16))
#define RUR_ASSETID ('R' + ((uint64_t)'U'<<8) + ((uint64_t)'R'<<16))    

#define GENESIS_SECRET "It was a bright cold day in April, and the clocks were striking thirteen."
#define rand16() ((uint16_t)((rand() >> 8) & 0xffff))
#define rand32() (((uint32_t)rand16()<<16) | rand16())
#define rand64() ((long long)(((uint64_t)rand32()<<32) | rand32()))

#define MAX_PRICE 1000000.f
#define NUM_BARPRICES 16

#define BARI_FIRSTBID 0
#define BARI_FIRSTASK 1
#define BARI_LOWBID 2
#define BARI_HIGHASK 3
#define BARI_HIGHBID 4
#define BARI_LOWASK 5
#define BARI_LASTBID 6
#define BARI_LASTASK 7

#define BARI_ARBBID 8
#define BARI_ARBASK 9
#define BARI_VIRTBID 10
#define BARI_VIRTASK 11
#define BARI_AVEBID 12
#define BARI_AVEASK 13
#define BARI_MEDIAN 14
#define BARI_AVEPRICE 15

#define ILLEGAL_COIN "ERR"
#define ILLEGAL_COINASSET "666"

#define MGW_PENDING_DEPOSIT -1
#define MGW_PENDING_WITHDRAW -2
#define MGW_FUND_INSTANTDEX -3
#define MGW_INSTANTDEX_TONXTAE -4

#define NXT_COINASSET "0"

#define GENESISACCT "1739068987193023818"  // NXT-MRCC-2YLS-8M54-3CMAJ
#define GENESISBLOCK "2680262203532249785"
#define BTCD_PORT 14631

#define NUM_GATEWAYS 3
#define _NXTSERVER "requestType"
#define EMERGENCY_PUNCH_SERVER Server_ipaddrs[0]
#define POOLSERVER Server_ipaddrs[1]
#define MIXER_ADDR Server_ipaddrs[2]


#define DEFAULT_NXT_DEADLINE 720
#define SATOSHIDEN 100000000L
#define NXT_TOKEN_LEN 160
#define POLL_SECONDS 10
#define NXT_ASSETLIST_INCR 16
#define SATOSHIDEN 100000000L
#define dstr(x) ((double)(x) / SATOSHIDEN)

#define MAX_COIN_INPUTS 32
#define MAX_COIN_OUTPUTS 3
//#define MAX_PRIVKEY_SIZE 768

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
#include <pthread.h>

#ifndef WIN32
//#include <getopt.h>
//#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
//#include <sys/socket.h>
#include <sys/wait.h>
//#include <sys/time.h>


#define ENABLE_DIRENT
#ifdef ENABLE_DIRENT
#include <sys/stat.h>   // only NXTmembers.h
#include <dirent.h>     //only NXTmembers.h
#endif

#include <curl/curl.h>
#include <curl/easy.h>

#else
#include "includes/curl.h"
#include "includes/easy.h"

#include <windows.h>
//#include "utils/pthread.h"
#include "includes/gettimeofday.h"


#define DEFINES_ONLY
#include "plugins/ramchain/files777.c"
#include "plugins/ramchain/system777.c"
#include "plugins/ramchain/NXT777.c"
#include "plugins/ramchain/storage.c"
#undef DEFINES_ONLY

char *os_compatible_path(char *fname);
void sleepmillis(uint32_t milliseconds);
void portable_sleep(int32_t n)
{
    sleep(n);
}
void msleep(int32_t n)
{
    usleep(n * 1000);
}
/*FILE *jl777fopen(char *fname,char *mode)
{
    char *clonestr(char *);
    FILE *fp;
    int32_t i;
    char *name = clonestr(fname);
    for (i=0; name[i]!=0; i++)
        if ( name[i] == '/' )
            name[i] = '\\';
    fp = fopen(name,mode);
    free(name);
    return(fp);
}
#define fopen jl777fopen*/


#ifdef __MINGW32__
#elif __MINGW64__
#else
#define STDIN_FILENO 0
//void usleep(int32_t);
void msleep(int32_t);
#endif

#endif

//void portable_sleep(int32_t);


/*void *jl777malloc(size_t allocsize) { void *ptr = malloc(allocsize); if ( ptr == 0 ) { fprintf(stderr,"malloc(%ld) failed\n",allocsize); while ( 1 ) portable_sleep(60); } return(ptr); }
void *jl777calloc(size_t num,size_t allocsize) { void *ptr = calloc(num,allocsize); if ( ptr == 0 ) { fprintf(stderr,"calloc(%ld,%ld) failed\n",num,allocsize); while ( 1 ) portable_sleep(60); } return(ptr); }
long jl777strlen(const char *str) { if ( str == 0 ) { fprintf(stderr,"strlen(NULL)??\n"); return(0); } return(strlen(str)); }
#define malloc jl777malloc
#define calloc jl777calloc
#define strlen jl777strlen*/

#ifdef UDP_OLDWAY
#define portable_udp_t int32_t
#else
#define portable_udp_t uv_udp_t
#endif

#define portable_tcp_t uv_tcp_t

//#define portable_mutex_t pthread_mutex_t

// includes that include actual code
//#include "includes/crypto_box.h"
/*#include "tweetnacl.c"
#if __i686__ || __i386__
#include "curve25519-donna.c"
#else
#include "curve25519-donna-c64.c"
#endif*/
//#include "includes/randombytes.h"

//#include "utils/smoothers.h"
#include "jdatetime.h"
#include "sorts.h"
//#include "utils/kdtree.c"
//#include "bitmap.h"

#include "cJSON.h"
//#include "jl777str.h"
//#include "cJSON.c"
//#include "bitcoind_RPC.c"
#include "SuperNET.h"
//#include "jsoncodec.h"
//#include "mappedptr.h"
//#include "ramchain.h"
#include "includes/utlist.h"
struct resultsitem { struct queueitem DL; char *argstr,*retstr; uint64_t txid; char retbuf[]; };

/*#define portable_mutex_t uv_mutex_t
struct queueitem { struct queueitem *next,*prev; };

typedef struct queue
{
	struct queueitem *list;
	portable_mutex_t mutex;
    char name[31],initflag;
} queue_t;

typedef struct queue
{
#ifdef oldqueue
	void **buffer;
#else
	void *buffer[65536];
#endif
    int32_t capacity,size,in,out,initflag;
	portable_mutex_t mutex;
    char name[32];
	//pthread_cond_t cond_full;
	//pthread_cond_t cond_empty;
} queue_t;
//#define QUEUE_INITIALIZER(buffer) { buffer, sizeof(buffer) / sizeof(buffer[0]), 0, 0, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER }*/
void *queue_dequeue(queue_t *queue,int32_t offsetflag);
void queue_enqueue(char *name,queue_t *queue,struct queueitem *ptr);


struct pingpong_queue
{
    char *name;
    queue_t pingpong[2],*destqueue,*errorqueue;
    int32_t (*action)();
    int offset;
};

union NXTtype { uint64_t nxt64bits; uint32_t uval; int32_t val; int64_t lval; double dval; char *str; cJSON *json; };


struct NXT_protocol_parms
{
    cJSON *argjson;
    int32_t mode,type,subtype,priority,height;
    char *txid,*sender,*receiver,*argstr;
    void *AMptr;
    char *assetid,*comment;
    int64_t assetoshis;
};

#include "tweetnacl.h"

struct NXThandler_info
{
    double fractured_prob,endpuzzles;  // probability NXT network is fractured, eg. major fork or attack in progress
    int32_t upollseconds,pollseconds,firsttimestamp,timestamp,RTflag,NXTheight,hashprocessing;
    int64_t acctbalance;
    uint64_t blocks[1000 * 365 * 10]; // fix in 10 years
    portable_mutex_t hash_mutex;
    void *handlerdata;
    char *origblockidstr,lastblock[256],blockidstr[256];
    queue_t hashtable_queue[2];
    struct hashtable **Pservers_tablep,**NXTaccts_tablep,**NXTassets_tablep,**NXTasset_txids_tablep,**otheraddrs_tablep,**Telepathy_tablep,**redeemtxids,**coin_txidinds,**coin_txidmap,**pending_xfers;//,**Storage_tablep,**Private_tablep;,**NXTguid_tablep,
    cJSON *accountjson;
    uv_udp_t *udp;
    unsigned char loopback_pubkey[crypto_box_PUBLICKEYBYTES],loopback_privkey[crypto_box_SECRETKEYBYTES];
    char pubkeystr[crypto_box_PUBLICKEYBYTES*2+1],myhandle[64],*myNXTADDR,*srvNXTACCTSECRET;
    bits256 mypubkey,myprivkey;
    uint64_t nxt64bits,puzzlethreshold,*neighbors;//,coins[4];
    int32_t initassets,Lfactor,gatewayid,gensocks[256],bussock;
    int32_t height,extraconfirms,maxpopdepth,maxpopheight,lastchanged,GLEFU,numblocks,timestamps[1000 * 365 * 10];
    int32_t isudpserver,istcpserver,numPrivacyServers,isMM,iambridge,insmallworld;
    uint32_t puzzletime;
    char ipaddr[64],dispname[128],groupname[128];
};
struct NXT_acct *get_NXTacct(int32_t *createdp,char *NXTaddr);
extern struct NXThandler_info *Global_mp;

#define NXTPROTOCOL_INIT -1
#define NXTPROTOCOL_IDLETIME 0
#define NXTPROTOCOL_NEWBLOCK 1
#define NXTPROTOCOL_AMTXID 2
#define NXTPROTOCOL_TYPEMATCH 3
#define NXTPROTOCOL_ILLEGALTYPE 666

typedef void *(*NXT_handler)(struct NXThandler_info *mp,struct NXT_protocol_parms *parms,void *handlerdata,int32_t height);
typedef int32_t (*addcache_funcp)(int32_t arg,char *key,void *ptr,int32_t len);

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

//union _coin_value_ptr { char *script; char *coinbase; };

struct coin_txidind
{
    int64_t modified;
    uint64_t redeemtxid,value,unspent;
    struct address_entry entry;
    int32_t numvouts,numinputs;
    uint32_t seq0,seq1;
    char coinaddr[MAX_COINADDR_LEN],txid[MAX_COINTXID_LEN],indstr[32],*script;
};

struct coin_txidmap
{
    uint64_t modified;
    uint32_t blocknum;
    uint16_t txind,v;
    char txidmapstr[MAX_COINTXID_LEN];
};

struct unspent_info
{
    int64_t maxunspent,unspent,maxavail,minavail;
    struct coin_txidind **vps,*maxvp,*minvp;
    int32_t maxvps,num;
};

struct rawtransaction
{
    struct coin_txidind *inputs[MAX_MULTISIG_INPUTS];
    char *destaddrs[MAX_MULTISIG_OUTPUTS],txid[MAX_COINADDR_LEN];
    uint64_t redeems[MAX_MULTISIG_OUTPUTS+MAX_MULTISIG_INPUTS];
    int64_t amount,change,inputsum,destamounts[MAX_MULTISIG_OUTPUTS];
    int32_t numoutputs,numinputs,completed,broadcast,confirmed,numredeems;
    uint32_t batchcrc;
    long batchsize;
    char *rawtxbytes,*signedtx;
    char batchsigned[56000];
};

//struct server_request_header { int32_t retsize,argsize,variant,funcid; };

struct withdraw_info
{
    //struct server_request_header H;
    uint64_t modified,AMtxidbits,approved[16];
    int64_t amount,moneysent;
    int32_t srcgateway,destgateway,twofactor,authenticated,submitted,confirmed;
    char withdrawaddr[128],NXTaddr[MAX_NXTADDR_LEN],redeemtxid[MAX_NXTADDR_LEN],comment[1024];
    char cointxid[MAX_COINTXID_LEN],coinstr[16];
};

struct consensus_info
{
    int64_t balance;
    uint64_t circulation,unspent,pendingdeposits,pendingwithdraws;
    uint32_t boughtNXT,pad0;
};

struct batch_info
{
    struct consensus_info C;
    struct withdraw_info W;
    struct rawtransaction rawtx;
};

struct coin_info
{
#ifdef soon
    struct ramchain_info RAM;
#endif
    int32_t timestamps[100];
    struct batch_info BATCH,withdrawinfos[16];
    struct unspent_info unspent;
    portable_mutex_t consensus_mutex;
    cJSON *json;
    struct huffitem *items;
    uv_udp_t *bridgeudp;
    //struct compressionvars V;
    struct hashtable *telepods,*addrs; void *changepod; uint64_t min_telepod_satoshis;
    cJSON *ciphersobj;
    char privateaddr[128],privateNXTACCTSECRET[MAX_JSON_FIELD*2+1],coinpubkey[1024],privateNXTADDR[64];
    char srvpubaddr[128],srvNXTACCTSECRET[MAX_JSON_FIELD*2+1],srvcoinpubkey[1024],srvNXTADDR[64];
    
    char name[64],backupdir[512],privacyserver[64],myipaddr[64],transporteraddr[128],bridgeipaddr[64],MGWissuer[64];
    char *userpass,*serverport,assetid[64],*marker,*marker2,*tradebotfname,*pending_ptr;
    uint64_t srvpubnxtbits,privatebits,dust,NXTfee_equiv,txfee,markeramount,lastheighttime,blockheight,RTblockheight,nxtaccts[512];
    uint32_t uptodate,boughtNXT;
    int32_t maxevolveiters,initdone,nohexout,use_addmultisig,min_confirms,minconfirms,estblocktime,forkheight,backupcount,enabled,savedtelepods,M,N,numlogs,clonesmear,pending_ptrmaxlen,srvport,numnxtaccts;
    uint16_t bridgeport;
    char multisigchar;
};


#define TRADEBOT_PRICECHANGE 1
#define TRADEBOT_NEWMINUTE 2
#define MAX_TRADEBOT_INPUTS (1024*1024)
#define MAX_BOTTYPE_BITS 30
#define MAX_BOTTYPE_VNUMBITS 11
#define MAX_BOTTYPE_ITEMBITS (MAX_BOTTYPE_BITS - 2*MAX_BOTTYPE_VNUMBITS)

struct tradebot_type { uint32_t itembits_sub1:MAX_BOTTYPE_ITEMBITS,vnum:MAX_BOTTYPE_VNUMBITS,vind:MAX_BOTTYPE_VNUMBITS,isfloat:1,hasneg:1; };

struct tradebot
{
    char *botname,**outputnames;
    void *compiled,*codestr;
    void **inputs,**outputs;
    uint32_t lastupdatetime;
    int32_t botid,numoutputs,metalevel,disabled,numinputs,bitpacked;
    struct tradebot_type *outtypes,*intypes;
    uint64_t *inconditioners;
};

struct InstantDEX_state
{
    struct price_data *dp;
    struct exchange_state *ep;
    char exchange[64],base[64],rel[64];
    uint64_t baseid,relid,changedmask;
    int32_t numexchanges,event;
    uint32_t jdatetime;
    
    int maxbars,numbids,numasks;
    uint64_t *bidnxt,*asknxt;
    double *bids,*asks,*inv_bids,*inv_asks;
    double *bidvols,*askvols,*inv_bidvols,*inv_askvols;
    float *m1,*m2,*m3,*m4,*m5,*m10,*m15,*m30,*h1;
    float *inv_m1,*inv_m2,*inv_m3,*inv_m4,*inv_m5,*inv_m10,*inv_m15,*inv_m30,*inv_h1;
};

struct tradebot_language
{
    char name[64];
    int32_t (*compiler_func)(int32_t *iosizep,void **inputs,struct tradebot_type *intypes,uint64_t *conditioners,int32_t *numinputsp,int32_t *metalevelp,void **compiledptr,char *retbuf,cJSON *codejson);
    int32_t (*runtime_func)(uint32_t jdatetime,struct tradebot *bot,struct InstantDEX_state *state);
    struct tradebot **tradebots;
    int32_t numtradebots,maxtradebots;
} **Languages;

#define ORDERBOOK_SIG 0x83746783

#define ORDERBOOK_FEED 1
#define NUM_PRICEDATA_SPLINES 17
#define MAX_TRADEBOT_BARS 512

#define LEFTMARGIN 0
#define TIMEIND_PIXELS 60
#define MAX_LOOKAHEAD 60
#define NUM_ACTIVE_PIXELS ((32*24) - MAX_LOOKAHEAD)
#define MAX_ACTIVE_WIDTH (NUM_ACTIVE_PIXELS + TIMEIND_PIXELS)
#define NUM_REQFUNC_SPLINES 32
#define MAX_SCREENWIDTH 2048
#define MAX_AMPLITUDE 100.
#ifndef MIN
#define MIN(x,y) (((x)<=(y)) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x,y) (((x)>=(y)) ? (x) : (y))
#endif

//#define calc_predisplinex(startweekind,clumpsize,weekind) (((weekind) - (startweekind))/(clumpsize))
struct tradebot_ptrs
{
    char base[64],rel[64];
    uint32_t jdatetime;
    int maxbars,numbids,numasks;
    uint64_t bidnxt[MAX_TRADEBOT_BARS],asknxt[MAX_TRADEBOT_BARS];
    double bids[MAX_TRADEBOT_BARS],asks[MAX_TRADEBOT_BARS],inv_bids[MAX_TRADEBOT_BARS],inv_asks[MAX_TRADEBOT_BARS];
    double bidvols[MAX_TRADEBOT_BARS],askvols[MAX_TRADEBOT_BARS],inv_bidvols[MAX_TRADEBOT_BARS],inv_askvols[MAX_TRADEBOT_BARS];
    float m1[MAX_TRADEBOT_BARS * NUM_BARPRICES],m2[MAX_TRADEBOT_BARS * NUM_BARPRICES],m3[MAX_TRADEBOT_BARS * NUM_BARPRICES],m4[MAX_TRADEBOT_BARS * NUM_BARPRICES],m5[MAX_TRADEBOT_BARS * NUM_BARPRICES],m10[MAX_TRADEBOT_BARS * NUM_BARPRICES],m15[MAX_TRADEBOT_BARS * NUM_BARPRICES],m30[MAX_TRADEBOT_BARS * NUM_BARPRICES],h1[MAX_TRADEBOT_BARS * NUM_BARPRICES];
    float inv_m1[MAX_TRADEBOT_BARS * NUM_BARPRICES],inv_m2[MAX_TRADEBOT_BARS * NUM_BARPRICES],inv_m3[MAX_TRADEBOT_BARS * NUM_BARPRICES],inv_m4[MAX_TRADEBOT_BARS * NUM_BARPRICES],inv_m5[MAX_TRADEBOT_BARS * NUM_BARPRICES],inv_m10[MAX_TRADEBOT_BARS * NUM_BARPRICES],inv_m15[MAX_TRADEBOT_BARS * NUM_BARPRICES],inv_m30[MAX_TRADEBOT_BARS * NUM_BARPRICES],inv_h1[MAX_TRADEBOT_BARS * NUM_BARPRICES];
};

struct filtered_buf
{
	double coeffs[512],projden[512],emawts[512];
	double buf[256+256],projbuf[256+256],prevprojbuf[256+256],avebuf[(256+256)/16];
	double slopes[4];
	double emadiffsum,lastval,lastave,diffsum,Idiffsum,refdiffsum,RTsum;
	int32_t middlei,len;
};

struct price_data
{
    double lastprice;
    struct exchange_quote *allquotes;
    uint32_t *display,firstjdatetime,lastjdatetime,calctime;
    uint64_t baseid,relid;
    char base[64],rel[64];
    struct tradebot_ptrs PTRS;
    struct filtered_buf bidfb,askfb,avefb,slopefb,accelfb;
    float *bars,avebar[NUM_BARPRICES],highbids[MAX_ACTIVE_WIDTH],lowasks[MAX_ACTIVE_WIDTH],aveprices[MAX_ACTIVE_WIDTH];
    double displine[MAX_ACTIVE_WIDTH],dispslope[MAX_ACTIVE_WIDTH],dispaccel[MAX_ACTIVE_WIDTH];
    double splineprices[MAX_ACTIVE_WIDTH],slopes[MAX_ACTIVE_WIDTH],accels[MAX_ACTIVE_WIDTH];
    double *pixeltimes,timefactor,aveprice,absslope,absaccel,avedisp,aveslope,aveaccel,bidsum,asksum,halfspread;
    double dSplines[NUM_PRICEDATA_SPLINES][4],jdatetimes[NUM_PRICEDATA_SPLINES],splinevals[NUM_PRICEDATA_SPLINES];
    int32_t numquotes,maxquotes,screenwidth,screenheight,numsplines;
};

/*struct quote
{
    //double price,vol;    // must be first!!
    struct InstantDEX_quote iQ;
    //uint32_t type;
    //uint64_t nxt64bits,baseamount,relamount;
};*/

/*struct orderbook_tx
{
    uint64_t baseid,relid;
    struct InstantDEX_quote iQ;
};*/


/*struct raw_orders
{
    uint64_t assetA,assetB,obookid;
    struct orderbook_tx **orders;
    int32_t num,max;
} **Raw_orders;*/

struct exchange_quote { uint32_t timestamp; float highbid,lowask; };
#define EXCHANGE_QUOTES_INCR ((int32_t)(4096L / sizeof(struct exchange_quote)))
#define INITIAL_PIXELTIME 60

struct exchange_state
{
    double bidminmax[2],askminmax[2],hbla[2];
    char name[64],url[512],url2[512],base[64],rel[64],lbase[64],lrel[64];
    int32_t updated,polarity,numbids,numasks,numbidasks;
    uint32_t type,writeflag;
    uint64_t obookid,feedid,baseid,relid,basemult,relmult;
    struct price_data P;
    FILE *fp;
    double lastmilli;
    queue_t ordersQ;
    char dbname[512];
    //struct orderbook_tx **orders;
};

#define _extrapolate_Spline(Spline,gap) ((double)(Spline[0]) + ((gap) * ((double)(Spline[1]) + ((gap) * ((double)(Spline[2]) + ((gap) * (double)(Spline[3])))))))
#define _extrapolate_Slope(Spline,gap) ((double)(Spline[1]) + ((gap) * ((double)(Spline[2]) + ((gap) * (double)(Spline[3])))))
#define _extrapolate_Accel(Spline,gap) ((double)(Spline[2]) + ((gap) * ((double)(Spline[3]))))

struct madata
{
	double sum;
	double ave,slope,diff,pastanswer;
	double signchange_slope,dirchange_slope,answerchange_slope,diffchange_slope;
	double oldest,lastval,accel2,accel;
	int32_t numitems,maxitems,next,maid;
	int32_t changes,slopechanges,accelchanges,diffchanges;
	int32_t signchanges,dirchanges,answerchanges;
	char signchange,dirchange,answerchange,islogprice;
	double RTvals[20],derivs[4],derivbufs[4][12];
	struct madata **stored;
#ifdef INSIDE_OPENCL
	int32_t pad;
#endif
	double rotbuf[];
};

#define TELEPORT_DEFAULT_SMEARTIME 3600

//#define SETBIT(bits,bitoffset) (((unsigned char *)bits)[(bitoffset) >> 3] |= (1 << ((bitoffset) & 7)))
//#define GETBIT(bits,bitoffset) (((unsigned char *)bits)[(bitoffset) >> 3] & (1 << ((bitoffset) & 7)))
//#define CLEARBIT(bits,bitoffset) (((unsigned char *)bits)[(bitoffset) >> 3] &= ~(1 << ((bitoffset) & 7)))
#ifndef MIN
#define MIN(x,y) (((x)<=(y)) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x,y) (((x)>=(y)) ? (x) : (y))
#endif
typedef char *(*json_handler)(char *verifiedNXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr);

char *bitcoind_RPC(char **retstrp,char *debugstr,char *url,char *userpass,char *command,char *args);
#define issue_curl(curl_handle,cmdstr) bitcoind_RPC(curl_handle,"curl",cmdstr,0,0,0)
#define issue_NXT(curl_handle,cmdstr) bitcoind_RPC(curl_handle,"NXT",cmdstr,0,0,0)
#define issue_NXTPOST(curl_handle,cmdstr) bitcoind_RPC(curl_handle,"curl",NXTAPIURL,0,0,cmdstr)
#define fetch_URL(curl_handle,cmdstr) bitcoind_RPC(curl_handle,"fetch",cmdstr,0,0,0)

extern uv_loop_t *UV_loop;
extern struct pingpong_queue Pending_offersQ;

char Server_ipaddrs[256][MAX_JSON_FIELD],DATADIR[MAX_JSON_FIELD],PRICEDIR[MAX_JSON_FIELD],WEBSOCKETD[MAX_JSON_FIELD];
char Server_NXTaddrs[256][MAX_JSON_FIELD],SERVER_PORTSTR[MAX_JSON_FIELD],SOPHIA_DIR[MAX_JSON_FIELD];
char *MGW_blacklist[256],*MGW_whitelist[256],ORIGBLOCK[MAX_JSON_FIELD],NXTISSUERACCT[MAX_JSON_FIELD];
cJSON *MGWconf,**MGWcoins;
uint64_t MIN_NQTFEE = SATOSHIDEN;
int32_t PERMUTE_RAWINDS,SOFTWALL,MAP_HUFF,OLDTX,DEFAULT_MAXDEPTH = 10,SUPERNET_PORT = 7777;
int32_t FASTMODE,SERVER_PORT,MIN_NXTCONFIRMS = 10;
uint32_t GATEWAY_SIG,FIRST_NXTBLOCK,FIRST_NXTTIMESTAMP,UPNP,MULTIPORT,QUOTE_SLEEP,EXCHANGE_SLEEP,ENABLE_EXTERNALACCESS;   // 3134975738 = 0xbadbeefa;
int32_t MULTITHREADS,DGSBLOCK = 213000;
int32_t MAX_BUYNXT,DBSLEEP,NXT_FORKHEIGHT,Finished_init,Finished_loading,Historical_done,NORAMCHAINS,Debuglevel = 0;
char NXTSERVER[MAX_JSON_FIELD],NXTAPIURL[MAX_JSON_FIELD],NXT_ASSETIDSTR[64],MGWROOT[1024];

struct hashtable *orderbook_txids;
uv_loop_t *UV_loop;
static long server_xferred;
int Servers_started;
queue_t P2P_Q,sendQ,JSON_Q,udp_JSON,storageQ,cacheQ,BroadcastQ,NarrowQ,ResultsQ,UDP_Q,DepositQ,WithdrawQ;
int32_t Num_in_whitelist,IS_LIBTEST,APIPORT,APISLEEP,USESSL,ENABLE_GUIPOLL,POLLGAP,LOG2_MAX_XFERPACKETS = 3;
uint32_t *SuperNET_whitelist;
int32_t Historical_done,MGW_initdone,THROTTLE;
struct NXThandler_info *Global_mp;
struct libwebsocket_context *LWScontext;

double picoc(int argc,char **argv,char *codestr);
int32_t init_sharenrs(unsigned char sharenrs[255],unsigned char *orig,int32_t m,int32_t n);
uint64_t call_SuperNET_broadcast(struct pserver_info *pserver,char *msg,int32_t len,int32_t duration);
void calc_sha256(char hashstr[(256 >> 3) * 2 + 1],unsigned char hash[256 >> 3],unsigned char *src,int32_t len);
void calc_sha256cat(unsigned char hash[256 >> 3],unsigned char *src,int32_t len,unsigned char *src2,int32_t len2);
struct NXT_acct *process_packet(int32_t internalflag,char *retjsonstr,unsigned char *recvbuf,int32_t recvlen,uv_udp_t *udp,struct sockaddr *addr,char *sender,uint16_t port);
char *send_tokenized_cmd(int32_t queueflag,char *hopNXTaddr,int32_t L,char *verifiedNXTaddr,char *NXTACCTSECRET,char *cmdstr,char *destNXTaddr);
typedef int32_t (*tfunc)(void *,int32_t argsize);
uv_work_t *start_task(tfunc func,char *name,int32_t sleepmicros,void *args,int32_t argsize);
char *addcontact(char *handle,char *acct);
char *SuperNET_json_commands(struct NXThandler_info *mp,char *previpaddr,cJSON *argjson,char *sender,int32_t valid,char *origargstr);
void handler_gotfile(char *sender,char *senderip,struct transfer_args *args,uint8_t *data,int32_t len,uint32_t crc);
char *call_SuperNET_JSON(char *JSONstr);
int curve25519_donna(uint8_t *, const uint8_t *, const uint8_t *);

bits256 curve25519(bits256 mysecret,bits256 theirpublic)
{
    bits256 rawkey;
    curve25519_donna(&rawkey.bytes[0],&mysecret.bytes[0],&theirpublic.bytes[0]);
    return(rawkey);
}

bits256 sha256_key(bits256 a)
{
    bits256 hash;
    calc_sha256(0,hash.bytes,a.bytes,256>>3);
    return(hash);
}

bits256 gen_sharedkey(bits256 mysecret,bits256 theirpublic)
{
    return(sha256_key(curve25519(mysecret,theirpublic)));
}

bits256 xor_keys(bits256 a,bits256 b)
{
    bits256 xor;
    xor.ulongs[0] = a.ulongs[0] ^ b.ulongs[0];
    xor.ulongs[1] = a.ulongs[1] ^ b.ulongs[1];
    xor.ulongs[2] = a.ulongs[2] ^ b.ulongs[2];
    xor.ulongs[3] = a.ulongs[3] ^ b.ulongs[3];
    return(xor);
}

bits256 gen_password()
{
    bits256 pass;
    int32_t i;
    memset(&pass,0,sizeof(pass));
    randombytes(pass.bytes,sizeof(pass));
    for (i=0; i<sizeof(pass)-1; i++)
    {
        if ( pass.bytes[i] == 0 )
            pass.bytes[i] = ((rand() >> 8) % 254) + 1;
    }
    pass.bytes[i] = 0;
    return(pass);
}

uint64_t conv_NXTpassword(unsigned char *mysecret,unsigned char *mypublic,uint8_t *pass,int32_t passlen);
/*uint64_t conv_NXTpassword(unsigned char *mysecret,unsigned char *mypublic,char *pass)
{
    static uint8_t basepoint[32] = {9};
    uint64_t addr;
    uint8_t hash[32];
    calc_sha256(0,mysecret,(unsigned char *)pass,(int32_t)strlen(pass));
    mysecret[0] &= 248, mysecret[31] &= 127, mysecret[31] |= 64;
    curve25519_donna(mypublic,mysecret,basepoint);
    calc_sha256(0,hash,mypublic,32);
    memcpy(&addr,hash,sizeof(addr));
    return(addr);
}*/

double _pairaved(double valA,double valB)
{
	if ( valA != 0. && valB != 0. )
		return((valA + valB) / 2.);
	else if ( valA != 0. ) return(valA);
	else return(valB);
}

double _pairave(float valA,float valB)
{
	if ( valA != 0.f && valB != 0.f )
		return((valA + valB) / 2.);
	else if ( valA != 0.f ) return(valA);
	else return(valB);
}
int32_t queue_size(queue_t *queue);
struct queueitem *queueitem(char *str);
void free_queueitem(void *itemptr);
void fatal(char *);

//#include "NXTservices.h"
#include "jl777hash.h"
//#include "NXTutils.h"
#include "ciphers.h"
#include "coins.h"
//#include "dbqueue.h"
//#include "storage.h"
#include "udp.h"
//#include "coincache.h"
#include "kademlia.h"
#include "packets.h"
//#include "mofnfs.h"
#include "contacts.h"
#include "deaddrop.h"
#include "telepathy.h"
//#include "NXTsock.h"
#include "bitcoind.h"
//#include "atomic.h"

//#include "feeds.h"
//#include "bars.h"
#include "InstantDEX.h"
#include "teleport.h"
//#include "mgw.h"
#include "tradebot.h"
#include "lotto.h"
//#include "NXTservices.c"
#include "api.h"

#endif
