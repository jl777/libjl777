/******************************************************************************
 * Copyright © 2014-2015 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * Nxt software, including this file, may be copied, modified, propagated,    *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/


#ifndef crypto777_portable777_h
#define crypto777_portable777_h

#include <stdint.h>
#include "mutex.h"
#include "nn.h"
#include "pubsub.h"
#include "pipeline.h"
#include "survey.h"
#include "reqrep.h"
#include "bus.h"
#include "pair.h"
#include "sha256.h"
#include "cJSON.h"
#include "uthash.h"

#define OP_RETURN_OPCODE 0x6a
#define calc_predisplinex(startweekind,clumpsize,weekind) (((weekind) - (startweekind))/(clumpsize))
#define _extrapolate_Spline(Splines,gap) ((double)(Splines)[0] + ((gap) * ((double)(Splines)[1] + ((gap) * ((double)(Splines)[2] + ((gap) * (double)(Splines)[3]))))))
#define _extrapolate_Slope(Splines,gap) ((double)(Splines)[1] + ((gap) * ((double)(Splines)[2] + ((gap) * (double)(Splines)[3]))))

#define PRICE_BLEND(oldval,newval,decay,oppodecay) ((oldval == 0.) ? newval : ((oldval * decay) + (oppodecay * newval)))
#define PRICE_BLEND64(oldval,newval,decay,oppodecay) ((oldval == 0) ? newval : ((oldval * decay) + (oppodecay * newval) + 0.499))

int32_t myatoi(char *str,int32_t maxval);

int32_t portable_truncate(char *fname,long filesize);
void *map_file(char *fname,uint64_t *filesizep,int32_t enablewrite);
int32_t os_supports_mappedfiles();
char *os_compatible_path(char *str);
char *OS_rmstr();
int32_t OS_launch_process(char *args[]);
int32_t OS_getppid();
int32_t OS_getpid();
int32_t OS_waitpid(int32_t childpid,int32_t *statusp,int32_t flags);
int32_t OS_conv_unixtime(int32_t *secondsp,time_t timestamp);
uint32_t OS_conv_datenum(int32_t datenum,int32_t hour,int32_t minute,int32_t second);

char *nn_typestr(int32_t type);

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

struct pingpong_queue
{
    char *name;
    queue_t pingpong[2],*destqueue,*errorqueue;
    int32_t (*action)();
    int offset;
};

#define ACCTS777_MAXRAMKVS 8
#define BTCDADDRSIZE 36
union _bits128 { uint8_t bytes[16]; uint16_t ushorts[8]; uint32_t uints[4]; uint64_t ulongs[2]; uint64_t txid; };
typedef union _bits128 bits128;
union _bits256 { uint8_t bytes[32]; uint16_t ushorts[16]; uint32_t uints[8]; uint64_t ulongs[4]; uint64_t txid; };
typedef union _bits256 bits256;
union _bits320 { uint8_t bytes[40]; uint16_t ushorts[20]; uint32_t uints[10]; uint64_t ulongs[5]; uint64_t txid; };
typedef union _bits320 bits320;
union _bits384 { bits256 sig; uint8_t bytes[48]; uint16_t ushorts[24]; uint32_t uints[12]; uint64_t ulongs[6]; uint64_t txid; };
typedef union _bits384 bits384;

struct ramkv777_item { UT_hash_handle hh; uint16_t valuesize,tbd; uint32_t rawind; uint8_t keyvalue[]; };

struct ramkv777
{
    char name[63],threadsafe;
    portable_mutex_t mutex;
    struct ramkv777_item *table;
    struct sha256_state state; bits256 sha256;
    int32_t numkeys,keysize,dispflag; uint8_t kvind;
};

#define ramkv777_itemsize(kv,valuesize) (sizeof(struct ramkv777_item) + (kv)->keysize + valuesize)
#define ramkv777_itemkey(item) (item)->keyvalue
#define ramkv777_itemvalue(kv,item) (&(item)->keyvalue[(kv)->keysize])

struct ramkv777_item *ramkv777_itemptr(struct ramkv777 *kv,void *value);
int32_t ramkv777_clone(struct ramkv777 *clone,struct ramkv777 *kv);
void ramkv777_free(struct ramkv777 *kv);

int32_t ramkv777_delete(struct ramkv777 *kv,void *key);
void *ramkv777_write(struct ramkv777 *kv,void *key,void *value,int32_t valuesize);
void *ramkv777_read(int32_t *valuesizep,struct ramkv777 *kv,void *key);
void *ramkv777_iterate(struct ramkv777 *kv,void *args,void *(*iterator)(struct ramkv777 *kv,void *args,void *key,void *value,int32_t valuesize));
struct ramkv777 *ramkv777_init(int32_t kvind,char *name,int32_t keysize,int32_t threadsafe);


void lock_queue(queue_t *queue);
void queue_enqueue(char *name,queue_t *queue,struct queueitem *item);
void *queue_dequeue(queue_t *queue,int32_t offsetflag);
int32_t queue_size(queue_t *queue);
struct queueitem *queueitem(char *str);
struct queueitem *queuedata(void *data,int32_t datalen);
void free_queueitem(void *itemptr);
void *queue_delete(queue_t *queue,struct queueitem *copy,int32_t copysize);
void *queue_free(queue_t *queue);
void *queue_clone(queue_t *clone,queue_t *queue,int32_t size);

void calc_sha256(char hashstr[(256 >> 3) * 2 + 1],uint8_t hash[256 >> 3],uint8_t *src,int32_t len);
void calc_sha256cat(uint8_t hash[256 >> 3],uint8_t *src,int32_t len,uint8_t *src2,int32_t len2);
void update_sha256(uint8_t hash[256 >> 3],struct sha256_state *state,uint8_t *src,int32_t len);

int32_t init_hexbytes_noT(char *hexbytes,uint8_t *message,long len);
int32_t decode_hex(unsigned char *bytes,int32_t n,char *hex);
int init_base32(char *tokenstr,uint8_t *token,int32_t len);
int decode_base32(uint8_t *token,uint8_t *tokenstr,int32_t len);
uint64_t stringbits(char *str);

long _stripwhite(char *buf,int accept);
char *clonestr(char *str);
int32_t is_decimalstr(char *str);
int32_t safecopy(char *dest,char *src,long len);

int32_t hcalc_varint(uint8_t *buf,uint64_t x);
long hdecode_varint(uint64_t *valp,uint8_t *ptr,long offset,long mappedsize);

double milliseconds();
uint64_t peggy_basebits(char *name);
uint64_t peggy_relbits(char *name);
//uint32_t set_assetname(uint64_t *multp,char *name,uint64_t assetbits);
int32_t _set_assetname(uint64_t *multp,char *buf,char *jsonstr,uint64_t assetid);
struct prices777 *prices777_poll(char *exchangestr,char *name,char *base,uint64_t refbaseid,char *rel,uint64_t refrelid);
int32_t is_native_crypto(char *name,uint64_t bits);
uint64_t InstantDEX_name(char *key,int32_t *keysizep,char *exchange,char *name,char *base,uint64_t *baseidp,char *rel,uint64_t *relidp);
uint64_t is_MGWcoin(char *name);
char *is_MGWasset(uint64_t *multp,uint64_t assetid);
int32_t unstringbits(char *buf,uint64_t bits);
int32_t get_assetname(char *name,uint64_t assetid);
void calc_OP_HASH160(char hexstr[41],uint8_t hash160[20],char *pubkey);
int32_t decode_hex(unsigned char *bytes,int32_t n,char *hex);
int32_t decode_cipher(uint8_t *str,uint8_t *cipher,int32_t *lenp,uint8_t *myprivkey);

int32_t parse_ipaddr(char *ipaddr,char *ip_port);
int32_t gen_randomacct(uint32_t randchars,char *NXTaddr,char *NXTsecret,char *randfilename);
char *dumpprivkey(char *coinstr,char *serverport,char *userpass,char *coinaddr);
uint64_t conv_NXTpassword(unsigned char *mysecret,unsigned char *mypublic,uint8_t *pass,int32_t passlen);
uint64_t conv_rsacctstr(char *rsacctstr,uint64_t nxt64bits);
uint64_t conv_NXTpassword(unsigned char *mysecret,unsigned char *mypublic,uint8_t *pass,int32_t passlen);
void set_best_amounts(int64_t *baseamountp,int64_t *relamountp,double price,double volume);
int32_t is_mscoin(char *assetidstr);
uint32_t issue_getTime();
cJSON *privatemessage_encrypt(uint64_t destbits,void *pmstr,int32_t len);
void telepathic_PM(char *destNXT,char *PM);

#define SIGHASH_ALL 1
#define SIGHASH_NONE 2
#define SIGHASH_SINGLE 3
#define SIGHASH_ANYONECANPAY 0x80

#define _MAX_DEPTH 100
extern uint32_t MAX_DEPTH;
#define MINUTES_FIFO (1024)
#define HOURS_FIFO (64)
#define DAYS_FIFO (512)
#define INSTANTDEX_MINVOL 75
#define INSTANTDEX_ACCT "4383817337783094122"
#define MAX_TXPTRS 1024

#define INSTANTDEX_NAME "InstantDEX"
#define INSTANTDEX_NXTAENAME "nxtae"
#define INSTANTDEX_NXTAEUNCONF "unconf"
#define INSTANTDEX_BASKETNAME "basket"
#define INSTANTDEX_ACTIVENAME "active"
#define INSTANTDEX_EXCHANGEID 0
#define INSTANTDEX_UNCONFID 1
#define INSTANTDEX_NXTAEID 2
#define MAX_EXCHANGES 64
#define ORDERBOOK_EXPIRATION 3600

struct NXTtx { uint64_t txid; char fullhash[MAX_JSON_FIELD],utxbytes[MAX_JSON_FIELD],utxbytes2[MAX_JSON_FIELD],txbytes[MAX_JSON_FIELD],sighash[MAX_JSON_FIELD]; };

struct InstantDEX_shared { double price,vol; uint64_t quoteid,offerNXT,basebits,relbits,baseid,relid; int64_t baseamount,relamount; uint32_t timestamp; uint16_t duration:14,wallet:1,a:1,isask:1,expired:1,closed:1,swap:1,responded:1,matched:1,feepaid:1,automatch:1,pending:1,minperc:7; };
struct InstantDEX_quote
{
    UT_hash_handle hh;
    struct InstantDEX_shared s; // must be here
    char exchangeid,gui[9];
    char walletstr[];
};

struct prices777_order { struct InstantDEX_shared s; struct prices777 *source; uint64_t id; double wt,ratio; uint16_t slot_ba; };
struct prices777_basket { struct prices777 *prices; double wt; int32_t groupid,groupsize,aski,bidi; char base[64],rel[64]; };
struct prices777_orderentry { struct prices777_order bid,ask; };
#define MAX_GROUPS 8

struct prices777_basketinfo
{
    int32_t numbids,numasks; uint32_t timestamp;
    struct prices777_orderentry book[MAX_GROUPS+1][_MAX_DEPTH];
};

struct pending_trade { struct queueitem DL; struct NXTtx trigger; struct prices777_order order; uint64_t triggertxid,txid,quoteid,orderid; struct prices777 *prices; char *triggertx,*txbytes; cJSON *tradesjson; double price,volume; uint32_t timestamp; int32_t dir,type,version,size; };

struct prices777
{
    char url[512],exchange[64],base[64],rel[64],lbase[64],lrel[64],key[512],oppokey[512],contract[64],origbase[64],origrel[64];
    uint64_t contractnum,ap_mult,baseid,relid,basemult,relmult; double lastupdate,decay,oppodecay,lastprice,lastbid,lastask;
    uint32_t pollnxtblock,exchangeid,numquotes,updated,lasttimestamp,RTflag,disabled,dirty; int32_t keysize,oppokeysize;
    portable_mutex_t mutex;
    char *orderbook_jsonstrs[2][2];
    struct prices777_basketinfo O,O2; double groupwts[MAX_GROUPS + 1];
    uint8_t changed,type; uint8_t **dependents; int32_t numdependents,numgroups,basketsize; double commission;
    struct prices777_basket basket[];
};

struct exchange_info
{
    double (*updatefunc)(struct prices777 *prices,int32_t maxdepth);
    char *(*coinbalance)(struct exchange_info *exchange,double *balancep,char *coinstr);
    int32_t (*supports)(char *base,char *rel);
    uint64_t (*trade)(char **retstrp,struct exchange_info *exchange,char *base,char *rel,int32_t dir,double price,double volume);
    char name[16],apikey[MAX_JSON_FIELD],apisecret[MAX_JSON_FIELD],userid[MAX_JSON_FIELD]; cJSON *balancejson;
    uint32_t num,exchangeid,pollgap,refcount,polling,lastbalancetime; uint64_t nxt64bits; double lastupdate,commission;
    portable_mutex_t mutex;
};
extern uint32_t FIRST_EXTERNAL;

uint64_t gen_NXTtx(struct NXTtx *tx,uint64_t dest64bits,uint64_t assetidbits,uint64_t qty,uint64_t orderid,uint64_t quoteid,int32_t deadline,char *reftx,char *phaselink,uint32_t finishheight,char *phasesecret);
int32_t InstantDEX_verify(uint64_t destNXTaddr,uint64_t sendasset,uint64_t sendqty,cJSON *txobj,uint64_t recvasset,uint64_t recvqty);
int32_t verify_NXTtx(cJSON *json,uint64_t refasset,uint64_t qty,uint64_t destNXTbits);
cJSON *exchanges_json();
struct InstantDEX_quote *delete_iQ(uint64_t quoteid);
char *is_tradedasset(char *exchange,char *assetidstr);
int32_t supported_exchange(char *exchangestr);
struct NXTtx *fee_triggerhash(char *triggerhash,uint64_t orderid,uint64_t quoteid,int32_t deadline);

struct exchange_info *get_exchange(int32_t exchangeid);
char *exchange_str(int32_t exchangeid);
struct exchange_info *find_exchange(int32_t *exchangeidp,char *exchangestr);
struct exchange_info *exchange_find(char *exchangestr);
void prices777_exchangeloop(void *ptr);
char *fill_nxtae(uint64_t *txidp,uint64_t nxt64bits,char *secret,int32_t dir,double price,double volume,uint64_t baseid,uint64_t relid);
uint64_t prices777_equiv(uint64_t assetid);
void prices777_jsonstrs(struct prices777 *prices,struct prices777_basketinfo *OB);
char *prices777_activebooks(char *name,char *_base,char *_rel,uint64_t baseid,uint64_t relid,int32_t maxdepth,int32_t allflag,int32_t tradeable);
char *prices777_orderbook_jsonstr(int32_t invert,uint64_t nxt64bits,struct prices777 *prices,struct prices777_basketinfo *OB,int32_t maxdepth,int32_t allflag);
int32_t prices777_getmatrix(double *basevals,double *btcusdp,double *btcdbtcp,double Hmatrix[32][32],double *RTprices,char *contracts[],int32_t num,uint32_t timestamp);
struct InstantDEX_quote *find_iQ(uint64_t quoteid);
int32_t bidask_parse(struct destbuf *exchangestr,struct destbuf *name,struct destbuf *base,struct destbuf *rel,struct destbuf *gui,struct InstantDEX_quote *iQ,cJSON *json);
struct InstantDEX_quote *create_iQ(struct InstantDEX_quote *iQ,char *walletstr);
double prices777_InstantDEX(struct prices777 *prices,int32_t maxdepth);
char *hmac_sha1_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_md2_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_md4_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_md5_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_sha224_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_sha256_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_sha384_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_sha512_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_rmd128_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_rmd160_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_rmd256_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_rmd320_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_tiger_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_whirlpool_str(char *dest,char *key,int32_t key_size,char *message);
int nn_base64_encode(const uint8_t *in,size_t in_len,char *out,size_t out_len);
int nn_base64_decode(const char *in,size_t in_len,uint8_t *out,size_t out_len);
uint64_t is_NXT_native(uint64_t assetid);
cJSON *set_walletstr(cJSON *walletitem,char *walletstr,struct InstantDEX_quote *iQ);
cJSON *InstantDEX_shuffleorders(uint64_t *quoteidp,uint64_t nxt64bits,char *base);
extern queue_t InstantDEXQ;

struct prices777 *prices777_initpair(int32_t needfunc,double (*updatefunc)(struct prices777 *prices,int32_t maxdepth),char *exchange,char *base,char *rel,double decay,char *name,uint64_t baseid,uint64_t relid,int32_t basketsize);
double prices777_price_volume(double *volumep,uint64_t baseamount,uint64_t relamount);
struct prices777 *prices777_makebasket(char *basketstr,cJSON *_basketjson,int32_t addbasket,char *typestr,struct prices777 *baskets[],int32_t num);
char *InstantDEX(char *jsonstr,char *remoteaddr,int32_t localaccess);
//cJSON *prices777_InstantDEX_json(char *_base,char *_rel,int32_t depth,int32_t invert,int32_t localaccess,uint64_t *baseamountp,uint64_t *relamountp,struct InstantDEX_quote *iQ,uint64_t refbaseid,uint64_t refrelid,uint64_t jumpasset);
uint64_t calc_baseamount(uint64_t *relamountp,uint64_t assetid,uint64_t qty,uint64_t priceNQT);
uint64_t calc_asset_qty(uint64_t *availp,uint64_t *priceNQTp,char *NXTaddr,int32_t checkflag,uint64_t assetid,double price,double vol);
cJSON *InstantDEX_orderbook(struct prices777 *prices);
struct prices777 *prices777_find(int32_t *invertedp,uint64_t baseid,uint64_t relid,char *exchange);
int32_t get_duplicates(uint64_t *duplicates,uint64_t baseid);

uint64_t calc_quoteid(struct InstantDEX_quote *iQ);
double check_ratios(uint64_t baseamount,uint64_t relamount,uint64_t baseamount2,uint64_t relamount2);
double make_jumpquote(uint64_t baseid,uint64_t relid,uint64_t *baseamountp,uint64_t *relamountp,uint64_t *frombasep,uint64_t *fromrelp,uint64_t *tobasep,uint64_t *torelp);

extern queue_t PendingQ;
char *peggyrates(uint32_t timestamp);
#define MAX_SUBATOMIC_OUTPUTS 4
#define MAX_SUBATOMIC_INPUTS 16
#define SUBATOMIC_STARTING_SEQUENCEID 1000
#define SUBATOMIC_LOCKTIME (3600 * 2)
#define SUBATOMIC_DONATIONRATE .001
#define SUBATOMIC_DEFAULTINCR 100
#define SUBATOMIC_TYPE 0

struct subatomic_unspent_tx
{
    int64_t amount;    // MUST be first!
    uint32_t vout,confirmations;
    struct destbuf txid,address,scriptPubKey,redeemScript;
};

struct subatomic_rawtransaction
{
    char destaddrs[MAX_SUBATOMIC_OUTPUTS][64];
    int64_t amount,change,inputsum,destamounts[MAX_SUBATOMIC_OUTPUTS];
    int32_t numoutputs,numinputs,completed,broadcast,confirmed;
    char rawtransaction[1024],signedtransaction[1024],txid[128];
    struct subatomic_unspent_tx inputs[MAX_SUBATOMIC_INPUTS];   // must be last, could even make it variable sized
};

#endif
