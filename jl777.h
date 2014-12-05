
//  Created by jl777
//  MIT License
//
// clusterDefiningBlock

#ifndef gateway_jl777_h
#define gateway_jl777_h

#define HARDCODED_VERSION "0.256"

#define NXT_GENESISTIME 1385294400
#define MAX_LFACTOR 10
#define MAX_UDPLEN 1400
#define PUBADDRS_MSGDURATION (3600 * 24)
#define MAX_ONION_LAYERS 7
#define pNXT_SIG 0x99999999
#define MAX_DROPPED_PACKETS 64
#define MAX_MULTISIG_OUTPUTS 16
#define MAX_MULTISIG_INPUTS 256
#define MULTIGATEWAY_VARIANT 3
#define MULTIGATEWAY_SYNCWITHDRAW 0

#define ORDERBOOK_NXTID ('N' + ((uint64_t)'X'<<8) + ((uint64_t)'T'<<16))    // 5527630
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
#define BARI_MINBID 10
#define BARI_MAXASK 11
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

#define GENESISACCT "1739068987193023818"
#define NODECOIN_SIG 0x63968736
//#define NXTCOINSCO_PORT 8777
//#define NXTPROTOCOL_WEBJSON 7777
#define BTCD_PORT 14631

#define NUM_GATEWAYS 3
#define _NXTSERVER "requestType"
#define EMERGENCY_PUNCH_SERVER Server_names[0]
#define POOLSERVER Server_names[1]
#define MIXER_ADDR Server_names[2]

#define GENESISACCT "1739068987193023818"
#define GENESISBLOCK "2680262203532249785"

#define DEFAULT_NXT_DEADLINE 720
#define SATOSHIDEN 100000000L
#define NXT_TOKEN_LEN 160
#define POLL_SECONDS 10
#define NXT_ASSETLIST_INCR 100
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
#include <sys/mman.h>

#include <fcntl.h>

#define ENABLE_DIRENT
#ifdef ENABLE_DIRENT
#include <sys/stat.h>   // only NXTmembers.h
#include <dirent.h>     //only NXTmembers.h
#endif

#include "includes/uv.h"
#include <curl/curl.h>
#include <curl/easy.h>

#else
#include "includes/curl.h"
#include "includes/easy.h"

#include <windows.h>
//#include "utils/pthread.h"
#include "includes/uv.h"
#include "includes/gettimeofday.h"


void *jl777malloc(size_t allocsize) { void *ptr = malloc(allocsize); if ( ptr == 0 ) { fprintf(stderr,"malloc(%ld) failed\n",allocsize); while ( 1 ) sleep(60); } return(ptr); }
void *jl777calloc(size_t num,size_t allocsize) { void *ptr = calloc(num,allocsize); if ( ptr == 0 ) { fprintf(stderr,"calloc(%ld,%ld) failed\n",num,allocsize); while ( 1 ) sleep(60); } return(ptr); }
long jl777strlen(char *str) { if ( str == 0 ) { fprintf(stderr,"strlen(NULL)??\n"); return(0); } return(strlen(str)); }
#define malloc jl777malloc
#define calloc jl777calloc
#define strlen jl777strlen

#ifdef __MINGW32__
#elif __MINGW64__
#else
#define STDIN_FILENO 0
void sleep(int32_t);
void usleep(int32_t);
#endif

#endif


#ifdef UDP_OLDWAY
#define portable_udp_t int32_t
#else
#define portable_udp_t uv_udp_t
#endif

#define portable_tcp_t uv_tcp_t

//#define portable_mutex_t pthread_mutex_t
#define portable_thread_t uv_thread_t

#define portable_mutex_t uv_mutex_t

// includes that include actual code
//#include "includes/crypto_box.h"
#include "tweetnacl.c"
#ifdef WIN32
#include "curve25519-donna.c"
#else
#include "curve25519-donna-c64.c"
#endif
//#include "includes/randombytes.h"

//#include "utils/smoothers.h"
#include "jdatetime.h"
#include "mappedptr.h"
#include "sorts.h"
//#include "utils/kdtree.c"
//#include "bitmap.h"

#include "cJSON.h"
#include "jl777str.h"
#include "cJSON.c"
#include "bitcoind_RPC.c"
#include "jsoncodec.h"
#include "SuperNET.h"


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

union _json_AM_data { unsigned char binarydata[sizeof(struct compressed_json)]; char jsonstr[sizeof(struct compressed_json)]; struct compressed_json jsn; };

struct json_AM
{
    struct NXT_AMhdr H;
	uint32_t funcid,gatewayid,timestamp,jsonflag;
    union _json_AM_data U;
};

union _NXT_str_buf { char txid[MAX_NXTTXID_LEN]; char NXTaddr[MAX_NXTADDR_LEN];  char assetid[MAX_NXT_STRLEN]; };

struct NXT_str
{
    uint64_t modified,nxt64bits;
    union _NXT_str_buf U;
};

struct pserver_info
{
    uint64_t modified,nxt64bits,hasnxt[64];
    //uint32_t hasips[128];//numips,hasnum,numnxt,
    char ipaddr[64];
    uint32_t decrypterrs,port;
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
    struct hashtable **Pservers_tablep,**NXTaccts_tablep,**NXTassets_tablep,**NXTasset_txids_tablep,**otheraddrs_tablep,**Telepathy_tablep,**redeemtxids,**coin_txidinds,**coin_txidmap;//,**Storage_tablep,**Private_tablep;,**NXTguid_tablep,
    cJSON *accountjson;
    //FILE *storage_fps[2];
    uv_udp_t *udp;
    unsigned char loopback_pubkey[crypto_box_PUBLICKEYBYTES],loopback_privkey[crypto_box_SECRETKEYBYTES];
    char pubkeystr[crypto_box_PUBLICKEYBYTES*2+1],myhandle[64];
    bits256 mypubkey,myprivkey;
    uint64_t coins[4];
    int32_t initassets,Lfactor,gatewayid,gensocks[256];
    int32_t height,extraconfirms,maxpopdepth,maxpopheight,lastchanged,GLEFU,numblocks,timestamps[1000 * 365 * 10];
    int32_t isudpserver,istcpserver,numPrivacyServers;
    char ipaddr[64],dispname[128],groupname[128];
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
    char coinaddr[MAX_COINADDR_LEN],txid[MAX_COINTXID_LEN],indstr[MAX_NXTADDR_LEN],*script;
};

struct coin_txidmap
{
    uint64_t modified;
    uint32_t blocknum;
    uint16_t txind,v;
    char txidmapstr[MAX_COINTXID_LEN];
};

/*struct coin_value
{
    int64_t modified,value;
    char *txid;
    struct coin_txidind *parent,*spent,*pendingspend;
    union _coin_value_ptr U;
    int32_t parent_vout,spent_vin,pending_spendvin,isconfirmed,iscoinbase,isinternal;
    char coinaddr[MAX_COINADDR_LEN];
};*/

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

struct server_request_header { int32_t retsize,argsize,variant,funcid; };

struct withdraw_info
{
    struct server_request_header H;
    uint64_t modified,AMtxidbits,approved[16];
    int64_t amount,moneysent;
    int32_t coinid,srcgateway,destgateway,twofactor,authenticated,submitted,confirmed;
    char withdrawaddr[64],NXTaddr[MAX_NXTADDR_LEN],redeemtxid[MAX_NXTADDR_LEN],comment[1024];
    //struct rawtransaction rawtx;
    char cointxid[MAX_COINTXID_LEN];
};

struct batch_info
{
    struct withdraw_info W;
    struct rawtransaction rawtx;
};

/*struct coincache_info
{
    FILE *cachefp,*blocksfp;
    struct hashtable *coin_txids;
    char **blocks,*ignorelist;
    int32_t ignoresize,lastignore,numblocks,purgedblock;
};*/

struct coin_info
{
    int32_t timestamps[100];
    struct batch_info BATCH,withdrawinfos[16];
    //struct coincache_info CACHE;
    struct unspent_info unspent;
    portable_mutex_t consensus_mutex;
    //struct pingpong_queue podQ;
    cJSON *json;
    struct hashtable *telepods; void *changepod; uint64_t min_telepod_satoshis;
    //void **logs;
    cJSON *ciphersobj;
    char privateaddr[128],privateNXTACCTSECRET[2048],coinpubkey[1024],privateNXTADDR[64];
    char srvpubaddr[128],srvNXTACCTSECRET[2048],srvcoinpubkey[1024],srvNXTADDR[64];
    
    char name[64],backupdir[512],privacyserver[64],myipaddr[64],transporteraddr[128];
    char *userpass,*serverport,assetid[64],*marker,*tradebotfname,*pending_ptr;
    uint64_t *limboarray,srvpubnxtbits,privatebits,dust,NXTfee_equiv,txfee,markeramount,lastheighttime,blockheight,RTblockheight,nxtaccts[512];
    int32_t coinid,maxevolveiters,initdone,nohexout,use_addmultisig,min_confirms,minconfirms,estblocktime,forkheight,backupcount,enabled,savedtelepods,M,N,numlogs,clonesmear,pending_ptrmaxlen,srvport,numnxtaccts;
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

struct orderbook_tx
{
    uint64_t baseid,relid;
    struct InstantDEX_quote iQ;
    //uint32_t timestamp,type;
    //uint64_t txid,nxt64bits,baseid,relid;
    //uint64_t baseamount,relamount;
};

struct orderbook
{
    uint64_t baseid,relid;
    struct InstantDEX_quote *bids,*asks;
    int32_t numbids,numasks;
};

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

#define SETBIT(bits,bitoffset) (((unsigned char *)bits)[(bitoffset) >> 3] |= (1 << ((bitoffset) & 7)))
#define GETBIT(bits,bitoffset) (((unsigned char *)bits)[(bitoffset) >> 3] & (1 << ((bitoffset) & 7)))
#define CLEARBIT(bits,bitoffset) (((unsigned char *)bits)[(bitoffset) >> 3] &= ~(1 << ((bitoffset) & 7)))
#ifndef MIN
#define MIN(x,y) (((x)<=(y)) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x,y) (((x)>=(y)) ? (x) : (y))
#endif
typedef char *(*json_handler)(char *verifiedNXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr);

char *bitcoind_RPC(CURL *curl_handle,char *debugstr,char *url,char *userpass,char *command,char *args);
#define issue_curl(curl_handle,cmdstr) bitcoind_RPC(curl_handle,"curl",cmdstr,0,0,0)
#define issue_NXT(curl_handle,cmdstr) bitcoind_RPC(curl_handle,"NXT",cmdstr,0,0,0)
#define issue_NXTPOST(curl_handle,cmdstr) bitcoind_RPC(curl_handle,"curl",NXTAPIURL,0,0,cmdstr)
#define fetch_URL(curl_handle,cmdstr) bitcoind_RPC(curl_handle,"fetch",cmdstr,0,0,0)
//void gen_testforms(char *secret);
extern uv_loop_t *UV_loop;
char Server_names[NUM_GATEWAYS+1][MAX_JSON_FIELD];
char Server_NXTaddrs[256][MAX_JSON_FIELD],SERVER_PORTSTR[MAX_JSON_FIELD];
char *MGW_blacklist[256],*MGW_whitelist[256],ORIGBLOCK[MAX_JSON_FIELD],NXTISSUERACCT[MAX_JSON_FIELD];
cJSON *MGWconf,**MGWcoins;
uint64_t MIN_NQTFEE = SATOSHIDEN;
int32_t SERVER_PORT,MIN_NXTCONFIRMS = 10;
uint32_t GATEWAY_SIG;   // 3134975738 = 0xbadbeefa;
int32_t DGSBLOCK = 213000;
int32_t NXT_FORKHEIGHT,Finished_init,Finished_loading,Historical_done,Debuglevel = 0;
char NXTSERVER[MAX_JSON_FIELD],NXTAPIURL[MAX_JSON_FIELD];

struct hashtable *orderbook_txids;

//char dispstr[65536];
//char testforms[1024*1024],PC_USERNAME[512],MY_IPADDR[512];
uv_loop_t *UV_loop;
static long server_xferred;
int Servers_started;
queue_t P2P_Q,sendQ,JSON_Q,udp_JSON,storageQ,cacheQ,BroadcastQ,NarrowQ,ResultsQ;
//struct pingpong_queue PeerQ;
int32_t Num_in_whitelist,IS_LIBTEST,APIPORT,APISLEEP,USESSL,ENABLE_GUIPOLL;
uint32_t *SuperNET_whitelist;
int32_t Historical_done;
struct NXThandler_info *Global_mp;

double picoc(int argc,char **argv,char *codestr);
int32_t init_sharenrs(unsigned char sharenrs[255],unsigned char *orig,int32_t m,int32_t n);
uint64_t call_SuperNET_broadcast(struct pserver_info *pserver,char *msg,int32_t len,int32_t duration);
void calc_sha256(char hashstr[(256 >> 3) * 2 + 1],unsigned char hash[256 >> 3],unsigned char *src,int32_t len);
void calc_sha256cat(unsigned char hash[256 >> 3],unsigned char *src,int32_t len,unsigned char *src2,int32_t len2);
struct NXT_acct *process_packet(int32_t internalflag,char *retjsonstr,unsigned char *recvbuf,int32_t recvlen,uv_udp_t *udp,struct sockaddr *addr,char *sender,uint16_t port);
char *send_tokenized_cmd(char *hopNXTaddr,int32_t L,char *verifiedNXTaddr,char *NXTACCTSECRET,char *cmdstr,char *destNXTaddr);
typedef int32_t (*tfunc)(void *,int32_t argsize);
uv_work_t *start_task(tfunc func,char *name,int32_t sleepmicros,void *args,int32_t argsize);
char *addcontact(char *handle,char *acct);
char *SuperNET_json_commands(struct NXThandler_info *mp,char *previpaddr,cJSON *argjson,char *sender,int32_t valid,char *origargstr);

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

uint64_t conv_NXTpassword(unsigned char *mysecret,unsigned char *mypublic,char *pass)
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
}

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

#include "NXTservices.h"
#include "jl777hash.h"
#include "NXTutils.h"
#include "ciphers.h"
#include "coins.h"
#include "dbqueue.h"
#include "storage.h"
#include "udp.h"
//#include "coincache.h"
#include "kademlia.h"
#include "packets.h"
#include "mofnfs.h"
#include "contacts.h"
#include "deaddrop.h"
#include "telepathy.h"
#include "NXTsock.h"
#include "bitcoind.h"
#include "atomic.h"
#include "teleport.h"
#include "mgw.h"

#include "feeds.h"
#include "orders.h"
#include "bars.h"
#include "tradebot.h"
//#include "NXTservices.c"
#include "api.h"

#endif
