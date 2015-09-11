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

// code goes so fast, might need to change /proc/sys/net/ipv4/tcp_tw_recycle to have a 1 in it
//
//#define INSIDE_MGW

#ifdef DEFINES_ONLY
#ifndef crypto777_coins777_h
#define crypto777_coins777_h
#include <stdio.h>
#include "../uthash.h"
#include "../cJSON.h"
#include "../utils/huffstream.c"
#include "../common/system777.c"
#include "../KV/kv777.c"
#include "../utils/files777.c"
#include "../utils/utils777.c"
#include "../utils/bits777.c"
#include "gen1.c"
#ifdef INSIDE_MGW
#include "../mgw/old/db777.c"
#endif

#define OP_RETURN_OPCODE 0x6a
#define RAMCHAIN_PTRSBUNDLE 4096

#define MAX_BLOCKTX 0xffff
struct rawvin { char txidstr[128]; uint16_t vout; };
struct rawvout { char coinaddr[128],script[2048]; uint64_t value; };
struct rawtx { uint16_t firstvin,numvins,firstvout,numvouts; char txidstr[128]; };

struct rawblock
{
    uint32_t blocknum,timestamp;
    uint16_t numtx,numrawvins,numrawvouts,pad;
    uint64_t minted;
    char blockhash[65],merkleroot[65];
    struct rawtx txspace[MAX_BLOCKTX];
    struct rawvin vinspace[MAX_BLOCKTX];
    struct rawvout voutspace[MAX_BLOCKTX];
};

#define MAX_COINTX_INPUTS 256
#define MAX_COINTX_OUTPUTS 256
struct cointx_input { struct rawvin tx; char coinaddr[64],sigs[1024]; uint64_t value; uint32_t sequence; char used; };
struct cointx_info
{
    uint32_t crc; // MUST be first
    char coinstr[16],cointxid[128];
    uint64_t inputsum,amount,change,redeemtxid;
    uint32_t allocsize,batchsize,batchcrc,gatewayid,isallocated,completed;
    // bitcoin tx order
    uint32_t version,timestamp,numinputs;
    uint32_t numoutputs;
    struct cointx_input inputs[MAX_COINTX_INPUTS];
    struct rawvout outputs[MAX_COINTX_OUTPUTS];
    uint32_t nlocktime;
    // end bitcoin txcalc_nxt64bits
    char signedtx[];
};

struct coin777_state
{
    char name[16];
    uint8_t sha256[256 >> 3];
    struct sha256_state state;
    struct db777 *DB;
    struct mappedptr M;
    struct alloc_space MEM;
    queue_t writeQ; portable_mutex_t mutex;
    void *table;
    FILE *fp;
    uint64_t maxitems;
    uint32_t itemsize,flags;
};

struct hashed_uint32 { UT_hash_handle hh; uint32_t ind; };

struct coin777_hashes { uint64_t ledgerhash,credits,debits; uint8_t sha256[12][256 >> 3]; struct sha256_state states[12]; uint32_t blocknum,numsyncs,timestamp,txidind,unspentind,numspends,addrind,scriptind,totaladdrtx; };
struct coin_offsets { bits256 blockhash,merkleroot; uint64_t credits,debits; uint32_t timestamp,txidind,unspentind,numspends,addrind,scriptind,totaladdrtx; uint8_t check[16]; };

struct unspent_info { uint64_t value; uint32_t addrind,rawind_or_blocknum:31,isblocknum:1; };
struct spend_info { uint32_t unspentind,addrind,spending_txidind; uint16_t spending_vin; };

struct oldaddrtx_info { int64_t value; uint32_t rawind,num31:31,flag:1; };
struct addrtx_info { uint32_t unspentind,spendind; };
struct oldcoin777_Lentry { struct addrtx_info *first_addrtxi; int64_t balance; uint32_t numaddrtx:30,insideA:1,pending:1,maxaddrtx:30,MGW:1,tbd:1; };
struct coin777_Lentry { int64_t balance; uint32_t first_addrtxi,numaddrtx:31,insideA:1,maxaddrtx:31,tbd:1; };
struct addrtx_linkptr { uint32_t next_addrtxi,maxunspentind; };

#ifndef ADDRINFO_SIZE
#define ADDRINFO_SIZE (168)
#endif

struct coin777_addrinfo
{
    uint32_t root_addrtxi,firstblocknum;
    int16_t scriptlen;
    uint8_t addrlen;
    char coinaddr[ADDRINFO_SIZE - 12];
};

#define COIN777_SHA256 256

struct ramchain
{
    uint64_t minted,addrsum; double calc_elapsed,startmilli,lastgetinfo;
    uint32_t latestblocknum,blocknum,numsyncs,RTblocknum,startblocknum,endblocknum,needbackup,num,syncfreq,readyflag,paused,RTmode;
    struct coin_offsets latest; long totalsize; double lastupdate;
    struct env777 DBs;
    struct coin777_state *sps[16],txidDB,addrDB,scriptDB,hashDB,ledger,addrtx,blocks,txoffsets,txidbits,unspents,spends,addrinfos;
    struct alloc_space tmpMEM;
};

struct MGWstate
{
    char name[64];
    uint64_t nxt64bits;
    int64_t MGWbalance,supply;
    uint64_t totalspends,numspends,totaloutputs,numoutputs;
    uint64_t boughtNXT,circulation,sentNXT,MGWpendingredeems,orphans,MGWunspent,MGWpendingdeposits,NXT_ECblock;
    int32_t gatewayid;
    uint32_t blocknum,RTblocknum,NXT_RTblocknum,NXTblocknum,is_realtime,NXT_is_realtime,enable_deposits,enable_withdraws,NXT_ECheight,permblocks;
};

#define NUM_GATEWAYS 3

#define MGW_ISINTERNAL 1
#define MGW_PENDINGXFER 2
#define MGW_DEPOSITDONE 4
#define MGW_PENDINGREDEEM 8
#define MGW_WITHDRAWDONE 16
#define MGW_IGNORE 128
#define MGW_ERRORSTATUS 0x8000
struct extra_info { uint64_t assetidbits,txidbits,senderbits,receiverbits,amount; int32_t ind,vout,flags; uint32_t height; char coindata[128]; };

struct mgw777
{
    char coinstr[16],assetidstr[32],assetname[32],issuer[32],marker[128],marker2[128],opreturnmarker[128];
    uint32_t marker_addrind,marker2_addrind,use_addmultisig,firstunspentind,redeemheight,numwithdraws,numunspents,oldtx_format,do_opreturn,RTNXT_height;
    uint64_t assetidbits,ap_mult,NXTfee_equiv,txfee,dust,issuerbits,circulation,unspent,withdrawsum; int64_t balance;
    cJSON *limbo,*special,*retjson;
    double lastupdate,NXTconvrate;
    struct unspent_info *unspents;
    struct MGWstate S,otherS[16],remotesrcs[16];
    struct extra_info withdraws[128];
    /*uint64_t MGWbits,NXTfee_equiv,txfee,*limboarray; char *coinstr,*serverport,*userpass,*marker,*marker2;
     int32_t numgateways,nummsigs,depositconfirms,withdrawconfirms,remotemode,numpendingsends,min_NXTconfirms,numspecials;
     uint32_t firsttime,NXTtimestamp,marker_rawind,marker2_rawind;
     double NXTconvrate;
     struct NXT_asset *ap;
     char multisigchar,*srvNXTADDR,**special_NXTaddrs,*MGWredemption,*backups,MGWsmallest[256],MGWsmallestB[256],MGWpingstr[1024],mgwstrs[3][8192];*/
};

struct coin777
{
    char name[16],serverport[512],userpass[4096],*jsonstr; cJSON *argjson;
    struct ramchain ramchain;
    struct mgw777 mgw;
    uint8_t p2shtype,addrtype,usep2sh;
    struct subatomic_rawtransaction funding; struct NXTtx trigger; char *refundtx,*signedrefund;
    int32_t minconfirms,verified,lag,estblocktime; uint64_t minoutput;
    char atomicsendpubkey[128],atomicrecvpubkey[128],atomicrecv[128],atomicsend[128],donationaddress[128],changeaddr[128];
};

char *bitcoind_RPC(char **retstrp,char *debugstr,char *url,char *userpass,char *command,char *params);
char *bitcoind_passthru(char *coinstr,char *serverport,char *userpass,char *method,char *params);
struct coin777 *coin777_create(char *coinstr,cJSON *argjson);
int32_t coin777_close(char *coinstr);
struct coin777 *coin777_find(char *coinstr,int32_t autocreate);
int32_t rawblock_load(struct rawblock *raw,char *coinstr,char *serverport,char *userpass,uint32_t blocknum);
void rawblock_patch(struct rawblock *raw);
char *subatomic_txid(char *txbytes,struct coin777 *coin,char *destaddr,uint64_t amount,int32_t future);
struct subatomic_unspent_tx *subatomic_bestfit(uint64_t *valuep,struct coin777 *coin,struct subatomic_unspent_tx *unspents,int32_t numunspents,uint64_t value,int32_t mode);
struct subatomic_unspent_tx *gather_unspents(uint64_t *totalp,int32_t *nump,struct coin777 *coin,char *skipcoinaddr);
cJSON *get_decoderaw_json(struct coin777 *coin,char *rawtransaction);
char *shuffle_signvin(char *sigstr,struct coin777 *coin,struct cointx_info *refT,int32_t redeemi);

void ram_clear_rawblock(struct rawblock *raw,int32_t totalflag);
void coin777_disprawblock(struct rawblock *raw);
uint64_t wait_for_txid(char *script,struct coin777 *coin,char *txidstr,int32_t vout,uint64_t recvamount,int32_t minconfirms,int32_t maxseconds);

int32_t coin777_parse(struct coin777 *coin,uint32_t RTblocknum,int32_t syncflag,int32_t minconfirms);
void coin777_initDBenv(struct coin777 *coin);
uint32_t coin777_startblocknum(struct coin777 *coin,uint32_t synci);
int32_t coin777_getinds(void *state,uint32_t blocknum,uint64_t *creditsp,uint64_t *debitsp,uint32_t *timestampp,uint32_t *txidindp,uint32_t *unspentindp,uint32_t *numspendsp,uint32_t *addrindp,uint32_t *scriptindp,uint32_t *totaladdrtxp);
int32_t coin777_initmmap(struct coin777 *coin,uint32_t blocknum,uint32_t txidind,uint32_t addrind,uint32_t scriptind,uint32_t unspentind,uint32_t totalspends,uint32_t totaladdrtx);
int32_t coin777_syncblocks(struct coin777_hashes *inds,int32_t max,struct coin777 *coin);
uint64_t coin777_ledgerhash(char *ledgerhash,struct coin777_hashes *H);
int32_t coin777_txidstr(struct coin777 *coin,char *txidstr,int32_t max,uint32_t txidind,uint32_t addrind);
int32_t coin777_scriptstr(struct coin777 *coin,char *scriptstr,int32_t max,uint32_t scriptind,uint32_t addrind);
int32_t coin777_coinaddr(struct coin777 *coin,char *coinaddr,int32_t max,uint32_t addrind,uint32_t addrind2);
uint32_t coin777_txidind(uint32_t *firstblocknump,struct coin777 *coin,char *txidstr);
uint32_t coin777_addrind(uint32_t *firstblocknump,struct coin777 *coin,char *coinaddr);
uint32_t coin777_scriptind(uint32_t *firstblocknump,struct coin777 *coin,char *coinaddr,char *scriptstr);
int32_t coin777_replayblocks(struct coin777 *coin,uint32_t startblocknum,uint32_t endblocknum,int32_t verifyflag);
uint64_t addrinfos_sum(struct coin777 *coin,uint32_t maxaddrind,int32_t syncflag,uint32_t maxunspentind,uint32_t maxspendind,int32_t recalcflag,uint32_t *totaladdrtxp);
int32_t coin777_verify(struct coin777 *coin,uint32_t maxunspentind,uint32_t maxspendind,uint64_t credits,uint64_t debits,uint32_t addrind,int32_t forceflag,uint32_t *totaladdrtxp);
int32_t coin777_incrbackup(struct coin777 *coin,uint32_t blocknum,int32_t prevsynci,struct coin777_hashes *H);
int32_t coin777_RWmmap(int32_t writeflag,void *value,struct coin777 *coin,struct coin777_state *sp,uint32_t rawind);
struct addrtx_info *coin777_compact(int32_t compactflag,uint64_t *balancep,int32_t *numaddrtxp,struct coin777 *coin,uint32_t addrind,struct coin777_Lentry *oldL);
uint64_t coin777_unspents(uint64_t (*unspentsfuncp)(struct coin777 *coin,void *args,uint32_t addrind,struct addrtx_info *unspents,int32_t num,uint64_t balance),struct coin777 *coin,char *coinaddr,void *args);
int32_t coin777_unspentmap(uint32_t *txidindp,char *txidstr,struct coin777 *coin,uint32_t unspentind);
uint64_t coin777_Uvalue(struct unspent_info *U,struct coin777 *coin,uint32_t unspentind);
int32_t update_NXT_assettransfers(struct mgw777 *mgw);
uint64_t calc_circulation(int32_t minconfirms,struct mgw777 *mgw,uint32_t height);
int32_t coin777_RWaddrtx(int32_t writeflag,struct coin777 *coin,uint32_t addrind,struct addrtx_info *ATX,struct coin777_Lentry *L,int32_t addrtxi);
#define coin777_scriptptr(A) ((A)->scriptlen == 0 ? 0 : (uint8_t *)&(A)->coinaddr[(A)->addrlen])
int32_t NXT_set_revassettxid(uint64_t assetidbits,uint32_t ind,struct extra_info *extra);
int32_t NXT_revassettxid(struct extra_info *extra,uint64_t assetidbits,uint32_t ind);
int32_t NXT_mark_withdrawdone(struct mgw777 *mgw,uint64_t redeemtxid);
int32_t subatomic_pubkeyhash(char *pubkeystr,char *pkhash,struct coin777 *coin,uint64_t quoteid);
char *subatomic_fundingtx(char *refredeemscript,struct subatomic_rawtransaction *funding,struct coin777 *coin,char *mypubkey,char *otherpubkey,char *pkhash,uint64_t amount,int32_t lockblocks);
char *subatomic_spendtx(struct destbuf *spendtxid,char *vintxid,char *refundsig,struct coin777 *coin,char *otherpubkey,char *mypubkey,char *onetimepubkey,uint64_t amount,char *refundtx,char *refredeemscript);
char *subatomic_validate(struct coin777 *coin,char *pubA,char *pubB,char *pkhash,char *refundtx,char *refundsig);
char *create_atomictx_scripts(uint8_t addrtype,char *scriptPubKey,char *p2shaddr,char *pubkeyA,char *pubkeyB,char *hash160str);

#ifdef INSIDE_MGW
struct db777 *db777_open(int32_t dispflag,struct env777 *DBs,char *name,char *compression,int32_t flags,int32_t valuesize);
#endif

#endif
#else
#ifndef crypto777_coins777_c
#define crypto777_coins777_c

#ifndef crypto777_coins777_h
#define DEFINES_ONLY
#include "coins777.c"
#endif

void debugstop()
{
    //#ifdef __APPLE__
    while ( 1 )
        sleep(60);
    //#endif
}

// coin777 parse funcs
uint64_t parse_voutsobj(int32_t (*voutfuncp)(void *state,uint64_t *creditsp,uint32_t txidind,uint16_t vout,uint32_t unspentind,char *coinaddr,char *script,uint64_t value,uint32_t *addrindp,uint32_t *scriptindp,uint32_t *totaladdrtxp,uint32_t blocknum),void *state,uint64_t *creditsp,uint32_t txidind,uint32_t *firstvoutp,uint16_t *txnumvoutsp,uint32_t *numrawvoutsp,uint32_t *addrindp,uint32_t *scriptindp,uint32_t *totaladdrtxp,cJSON *voutsobj,uint32_t blocknum)
{
    struct destbuf coinaddr,script; cJSON *item; uint64_t value,total = 0; int32_t i,numvouts = 0;
    *firstvoutp = (*numrawvoutsp);
    if ( voutsobj != 0 && is_cJSON_Array(voutsobj) != 0 && (numvouts= cJSON_GetArraySize(voutsobj)) > 0 )
    {
        (*txnumvoutsp) = numvouts;
        for (i=0; i<numvouts; i++,(*numrawvoutsp)++)
        {
            item = cJSON_GetArrayItem(voutsobj,i);
            value = conv_cJSON_float(item,"value");
            total += value;
            _extract_txvals(&coinaddr,&script,1,item); // default to nohexout
            if ( (*voutfuncp)(state,creditsp,txidind,i,(*numrawvoutsp),coinaddr.buf,script.buf,value,addrindp,scriptindp,totaladdrtxp,blocknum) != 0 )
                printf("error vout.%d numrawvouts.%u\n",i,(*numrawvoutsp));
        }
    } else (*txnumvoutsp) = 0, printf("error with vouts\n");
    return(total);
}

uint64_t parse_vinsobj(uint64_t (*vinfuncp)(void *state,uint64_t *debitsp,uint32_t txidind,uint16_t vin,uint32_t totalspends,char *spendtxidstr,uint16_t spendvout,uint32_t blocknum,uint32_t *totaladdrtxp),void *state,uint64_t *debitsp,uint32_t txidind,uint32_t *firstvinp,uint16_t *txnumvinsp,uint32_t *numrawvinsp,cJSON *vinsobj,uint32_t blocknum,uint32_t *totaladdrtxp)
{
    struct destbuf txidstr,coinbase; cJSON *item; int32_t i,numvins = 0; uint64_t value,total = 0;
    *firstvinp = (*numrawvinsp);
    if ( vinsobj != 0 && is_cJSON_Array(vinsobj) != 0 && (numvins= cJSON_GetArraySize(vinsobj)) > 0 )
    {
        (*txnumvinsp) = numvins;
        for (i=0; i<numvins; i++,(*numrawvinsp)++)
        {
            item = cJSON_GetArrayItem(vinsobj,i);
            if ( numvins == 1  )
            {
                copy_cJSON(&coinbase,cJSON_GetObjectItem(item,"coinbase"));
                if ( strlen(coinbase.buf) > 1 )
                {
                    (*txnumvinsp) = 0;
                    return(0);
                }
            }
            copy_cJSON(&txidstr,cJSON_GetObjectItem(item,"txid"));
            if ( (value= (*vinfuncp)(state,debitsp,txidind,i,(*numrawvinsp),txidstr.buf,(int)get_cJSON_int(item,"vout"),blocknum,totaladdrtxp)) == 0 )
                printf("error vin.%d numrawvins.%u\n",i,(*numrawvinsp));
            total += value;
        }
    } else (*txnumvinsp) = 0, printf("error with vins\n");
    return(total);
}

int32_t parse_block(void *state,uint64_t *creditsp,uint64_t *debitsp,uint32_t *txidindp,uint32_t *numrawvoutsp,uint32_t *numrawvinsp,uint32_t *addrindp,uint32_t *scriptindp,uint32_t *totaladdrtxp,char *coinstr,char *serverport,char *userpass,uint32_t blocknum,
    int32_t (*blockfuncp)(void *state,uint32_t blocknum,char *blockhash,char *merkleroot,uint32_t timestamp,uint64_t minted,uint32_t txidind,uint32_t unspentind,uint32_t numspends,uint32_t addrind,uint32_t scriptind,uint32_t totaladdrtx,uint64_t credits,uint64_t debits),
    uint64_t (*vinfuncp)(void *state,uint64_t *debitsp,uint32_t txidind,uint16_t vin,uint32_t totalspends,char *spendtxidstr,uint16_t spendvout,uint32_t blocknum,uint32_t *totaladdrtxp),
    int32_t (*voutfuncp)(void *state,uint64_t *creditsp,uint32_t txidind,uint16_t vout,uint32_t unspentind,char *coinaddr,char *script,uint64_t value,uint32_t *addrindp,uint32_t *scriptindp,uint32_t *totaladdrtxp,uint32_t blocknum),
    int32_t (*txfuncp)(void *state,uint32_t blocknum,uint32_t txidind,char *txidstr,uint32_t firstvout,uint16_t numvouts,uint64_t total,uint32_t firstvin,uint16_t numvins))
{
    struct destbuf blockhash,merkleroot,txidstr,mintedstr; char *txidjsonstr; cJSON *json,*txarray,*txjson;
    uint32_t checkblocknum,timestamp,firstvout,firstvin; uint16_t numvins,numvouts; int32_t txind,numtx = 0; uint64_t minted,total=0,spent=0;
    minted = total = 0;
    if ( (json= _get_blockjson(0,coinstr,serverport,userpass,0,blocknum)) != 0 )
    {
        if ( juint(json,"height") == blocknum )
        {
            copy_cJSON(&blockhash,cJSON_GetObjectItem(json,"hash"));
            copy_cJSON(&merkleroot,cJSON_GetObjectItem(json,"merkleroot"));
            timestamp = (uint32_t)get_cJSON_int(cJSON_GetObjectItem(json,"time"),0);
            copy_cJSON(&mintedstr,cJSON_GetObjectItem(json,"mint"));
            if ( mintedstr.buf[0] == 0 )
                copy_cJSON(&mintedstr,cJSON_GetObjectItem(json,"newmint"));
            if ( mintedstr.buf[0] != 0 )
                minted = (uint64_t)(atof(mintedstr.buf) * SATOSHIDEN);
            if ( (txarray= _rawblock_txarray(&checkblocknum,&numtx,json)) != 0 && checkblocknum == blocknum )
            {
                if ( (*blockfuncp)(state,blocknum,blockhash.buf,merkleroot.buf,timestamp,minted,(*txidindp),(*numrawvoutsp),(*numrawvinsp),(*addrindp),(*scriptindp),(*totaladdrtxp),(*creditsp),(*debitsp)) != 0 )
                    printf("error adding blocknum.%u\n",blocknum);
                firstvout = (*numrawvoutsp), firstvin = (*numrawvinsp);
                numvouts = numvins = 0;
                for (txind=0; txind<numtx; txind++,(*txidindp)++)
                {
                    copy_cJSON(&txidstr,cJSON_GetArrayItem(txarray,txind));
                    if ( (txidjsonstr= _get_transaction(coinstr,serverport,userpass,txidstr.buf)) != 0 )
                    {
                        if ( (txjson= cJSON_Parse(txidjsonstr)) != 0 )
                        {
                            total += parse_voutsobj(voutfuncp,state,creditsp,(*txidindp),&firstvout,&numvouts,numrawvoutsp,addrindp,scriptindp,totaladdrtxp,cJSON_GetObjectItem(txjson,"vout"),blocknum);
                            spent += parse_vinsobj(vinfuncp,state,debitsp,(*txidindp),&firstvin,&numvins,numrawvinsp,cJSON_GetObjectItem(txjson,"vin"),blocknum,totaladdrtxp);
                            free_json(txjson);
                        } else printf("update_txid_infos parse error.(%s)\n",txidjsonstr);
                        free(txidjsonstr);
                    }
                    else if ( blocknum != 0 )
                        printf("error getting.(%s) blocknum.%d\n",txidstr.buf,blocknum);
                    if ( (*txfuncp)(state,blocknum,(*txidindp),txidstr.buf,firstvout,numvouts,total,firstvin,numvins) != 0 )
                        printf("error adding txidind.%u blocknum.%u txind.%d\n",(*txidindp),blocknum,txind);
                }
                if ( (*blockfuncp)(state,blocknum+1,0,0,0,0,(*txidindp),(*numrawvoutsp),(*numrawvinsp),(*addrindp),(*scriptindp),(*totaladdrtxp),(*creditsp),(*debitsp)) != 0 )
                    printf("error finishing blocknum.%u\n",blocknum);
            } else printf("error _get_blocktxarray for block.%d got %d n.%d\n",blocknum,checkblocknum,numtx);
        } else printf("blocknum.%u mismatched with %u\n",blocknum,juint(json,"height"));
        free_json(json);
    } else printf("get_blockjson error parsing.(%s)\n",txidstr.buf);
    if ( Debuglevel > 2 )
        printf("BLOCK.%d: numtx.%d minted %.8f rawnumvins.%d rawnumvouts.%d\n",blocknum,numtx,dstr(minted),(*numrawvinsp),(*numrawvoutsp));
    return(numtx);
}

#ifdef INSIDE_MGW
// coin777 DB funcs
void *coin777_getDB(void *dest,int32_t *lenp,void *transactions,struct db777 *DB,void *key,int32_t keylen)
{
    void *obj,*value,*result = 0;
    if ( (obj= sp_object(DB->db)) != 0 )
    {
        if ( sp_set(obj,"key",key,keylen) == 0 )
        {
            if ( (result= sp_get(transactions != 0 ? transactions : DB->db,obj)) != 0 )
            {
                value = sp_get(result,"value",lenp);
                memcpy(dest,value,*lenp);
                sp_destroy(result);
                return(dest);
            } //else printf("DB.%p %s no result transactions.%p key.%x\n",DB,DB->name,transactions,*(int *)key);
        } else printf("no key\n");
    } else printf("getDB no obj\n");
    return(0);
}

int32_t coin777_addDB(struct coin777 *coin,void *transactions,struct db777 *DB,void *key,int32_t keylen,void *value,int32_t valuelen)
{
    void *db,*obj; int32_t retval; extern int32_t Added;
    db = DB->asyncdb != 0 ? DB->asyncdb : DB->db;
    if ( (obj= sp_object(db)) == 0 )
        retval = -3;
    if ( sp_set(obj,"key",key,keylen) != 0 || sp_set(obj,"value",value,valuelen) != 0 )
    {
        sp_destroy(obj);
        printf("error setting key/value %s[%d]\n",DB->name,*(int *)key);
        retval = -4;
    }
    else
    {
        Added++;
        coin->ramchain.totalsize += valuelen;
        retval = sp_set((transactions != 0 ? transactions : db),obj);
        if ( 1 && valuelen < 8192 )
        {
            void *check; char dest[8192]; int32_t len = sizeof(dest);
            check = coin777_getDB(dest,&len,transactions,DB,key,keylen);
            if ( check == 0 )
                printf("cant find just added key.%x val.%d %s\n",*(int *)key,*(int *)value,db777_errstr(coin->ramchain.DBs.ctl)), debugstop();
            else if ( memcmp(dest,value,valuelen) != 0 && len == valuelen )
                printf("cmp error just added key.%x len.%d valuelen.%d val.%d %s\n",*(int *)key,len,valuelen,*(int *)value,db777_errstr(coin->ramchain.DBs.ctl)), debugstop();
            //else printf("cmp success!\n");
        }
    }
    return(retval);
}

int32_t coin777_queueDB(struct coin777 *coin,struct db777 *DB,void *key,int32_t keylen,void *value,int32_t valuelen)
{
    return(coin777_addDB(coin,coin->ramchain.DBs.transactions,DB,key,keylen,value,valuelen));
}

// coin777 MM funcs
void *coin777_ensure(struct coin777 *coin,struct coin777_state *sp,uint64_t ind)
{
    char fname[1024]; long needed,prevsize = 0; int32_t rwflag = 1;
    needed = (ind + 2) * sp->itemsize;
    if ( needed > sp->M.allocsize )
    {
        db777_path(fname,coin->name,"",0), strcat(fname,"/"), strcat(fname,sp->name), os_compatible_path(fname);
        needed = (((needed * 1.1) + 1024 * sp->itemsize) / sp->itemsize) * sp->itemsize;
        printf("REMAP.%s %llu -> %ld [%ld] (%s)\n",sp->name,(long long)sp->M.allocsize,needed,(long)(needed - sp->M.allocsize)/sp->itemsize,fname);
        if ( sp->M.fileptr != 0 )
        {
            sync_mappedptr(&sp->M,0);
            release_map_file(sp->M.fileptr,sp->M.allocsize);
            sp->M.fileptr = 0, prevsize = sp->M.allocsize;
            sp->M.allocsize = 0;
        }
        needed = ensure_filesize(fname,needed,0);
    }
    if ( sp->M.fileptr == 0 )
    {
        if ( init_mappedptr(&sp->MEM.ptr,&sp->M,0,rwflag,fname) != 0 )
        {
            sp->MEM.size = sp->M.allocsize;
            sp->maxitems = (sp->MEM.size / sp->itemsize);
            if ( 1 && prevsize > sp->MEM.size )
                memset((void *)((uint64_t)sp->M.fileptr + prevsize),0,(sp->MEM.size - prevsize));
            printf("%p %s maxitems.%llu (MEMsize.%ld / itemsize.%d) prevsize.%ld needed.%ld\n",sp->MEM.ptr,sp->name,(long long)sp->maxitems,sp->MEM.size,sp->itemsize,prevsize,needed);
        }
    }
    if ( (sp->table= sp->M.fileptr) == 0 )
        printf("couldnt map %s\n",fname);
    return(sp->table);
}

void *coin777_itemptr(struct coin777 *coin,struct coin777_state *sp,uint64_t rawind)
{
    void *ptr = sp->table;
    if ( ptr == 0 || rawind >= sp->maxitems )
    {
        sp->table = coin777_ensure(coin,sp,rawind);
        if ( (ptr= sp->table) == 0 )
        {
            printf("SECOND ERROR %s overflow? %p rawind.%llu vs max.%llu\n",sp->name,ptr,(long long)rawind,(long long)sp->maxitems);
            return(0);
        }
    }
    ptr = (void *)((uint64_t)ptr + sp->itemsize*rawind);
    return(ptr);
}

int32_t coin777_RWmmap(int32_t writeflag,void *value,struct coin777 *coin,struct coin777_state *sp,uint32_t rawind)
{
    static uint8_t zeroes[4096];
    void *ptr; struct coin777_addrinfo *A; struct coin_offsets B,tmpB; int32_t i,itemsize,size,retval = 0;
    if ( (writeflag & COIN777_SHA256) != 0 )
    {
        coin->ramchain.totalsize += sp->itemsize;
        update_sha256(sp->sha256,&sp->state,value,sp->itemsize);
    }
    if ( sp->DB != 0 )
    {
        printf("unexpected DB path in coin777_RWmmap %s\n",sp->name);
        if ( writeflag != 0 )
            return(coin777_addDB(coin,coin->ramchain.DBs.transactions,sp->DB,&rawind,sizeof(rawind),value,sp->itemsize));
        else
        {
            size = sp->itemsize;
            if ( (ptr= coin777_getDB(value,&size,coin->ramchain.DBs.transactions,sp->DB,&rawind,sizeof(rawind))) == 0 || size != sp->itemsize )
                return(-1);
            return(0);
        }
    }
    else
    {
        portable_mutex_lock(&sp->mutex);
        if ( (ptr= coin777_itemptr(coin,sp,rawind)) != 0 )
        {
            if ( writeflag != 0 )
            {
                itemsize = sp->itemsize;
                if ( strcmp(sp->name,"addrinfos") == 0 )
                {
                    A = ptr;
                    itemsize = (sizeof(*A) - sizeof(A->coinaddr) + A->addrlen + A->scriptlen);
                }
                if ( writeflag == 1 && (sp->flags & DB777_VOLATILE) == 0 )
                {
                    if ( strcmp(sp->name,"blocks") == 0 )
                    {
                        memcpy(&B,ptr,sizeof(B));
                        memcpy(&tmpB,value,sizeof(tmpB));
                        if ( memcmp(&B.blockhash.bytes,zeroes,sizeof(B.blockhash)) == 0 && memcmp(&B.merkleroot.bytes,zeroes,sizeof(B.merkleroot)) == 0 )
                            B.blockhash = tmpB.blockhash, B.merkleroot = tmpB.merkleroot, ptr = &B;
                    }
                    if ( memcmp(value,ptr,itemsize) != 0 && sp->itemsize <= sizeof(zeroes) )
                    {
                        if ( memcmp(ptr,zeroes,sp->itemsize) != 0 )
                        {
                            printf("\n");
                            for (i=0; i<sp->itemsize; i++)
                                printf("%02x ",((uint8_t *)ptr)[i]);
                            printf("existing.%s %d <-- overwritten\n",sp->name,sp->itemsize);
                            for (i=0; i<sp->itemsize; i++)
                                printf("%02x ",((uint8_t *)value)[i]);
                            printf("new value.%s %d rawind.%u fileptr.%p ptr.%p\n",sp->name,sp->itemsize,rawind,sp->M.fileptr,ptr);
                        }
                    }
                }
                if ( sp->fp == 0 )
                    memcpy(ptr,value,sp->itemsize);
                else // all ready for rb+ fp and readonly mapping, but need to init properly
                {
                    fseek(sp->fp,(uint64_t)sp->itemsize * rawind,SEEK_SET);
                    fwrite(value,1,sp->itemsize,sp->fp);
                    if ( memcmp(ptr,value,sp->itemsize) != 0 )
                        printf("FATAL: write mmap error\n"), debugstop();
                }
            } else memcpy(value,ptr,sp->itemsize);
        } else retval = -2;
        portable_mutex_unlock(&sp->mutex);
    }
    return(retval);
}

// coin777 lookup funcs
void coin777_addind(struct coin777 *coin,struct coin777_state *sp,void *key,int32_t keylen,uint32_t ind)
{
    struct hashed_uint32 *entry,*table; void *ptr;
    if ( 0 && RAMCHAINS.fastmode != 0 )
    {
        ptr = tmpalloc(coin->name,&coin->ramchain.tmpMEM,keylen), memcpy(ptr,key,keylen);
        entry = tmpalloc(coin->name,&coin->ramchain.tmpMEM,sizeof(*entry)), entry->ind = ind;
        table = sp->table; HASH_ADD_KEYPTR(hh,table,ptr,keylen,entry); sp->table = table;
    }
}

uint32_t coin777_findind(struct coin777 *coin,struct coin777_state *sp,uint8_t *data,int32_t datalen)
{
    struct hashed_uint32 *entry; extern int32_t Duplicate;
    if ( 0 && RAMCHAINS.fastmode != 0 )
    {
        HASH_FIND(hh,(struct hashed_uint32 *)sp->table,data,datalen,entry);
        if ( entry != 0 )
        {
            Duplicate++;
            return(entry->ind);
        }
    }
    return(0);
}

uint32_t coin777_txidind(uint32_t *firstblocknump,struct coin777 *coin,char *txidstr)
{
    bits256 txid; uint32_t txidind = 0; int32_t tmp = sizeof(txidind);
    *firstblocknump = 0;
    if ( txidstr == 0 || txidstr[0] == 0 )
        return(0);
    memset(txid.bytes,0,sizeof(txid)), decode_hex(txid.bytes,sizeof(txid),txidstr);
    if ( (txidind= coin777_findind(coin,&coin->ramchain.txidDB,txid.bytes,sizeof(txid))) == 0 )
        coin777_getDB(&txidind,&tmp,coin->ramchain.DBs.transactions,coin->ramchain.txidDB.DB,txid.bytes,sizeof(txid));
    return(txidind);
}

uint32_t coin777_addrind(uint32_t *firstblocknump,struct coin777 *coin,char *coinaddr)
{
    uint32_t addrind; int32_t len,tmp = sizeof(addrind);
    *firstblocknump = 0;
    if ( coinaddr == 0 || coinaddr[0] == 0 )
        return(0);
    len = (int32_t)strlen(coinaddr) + 1;
    if ( (addrind= coin777_findind(coin,&coin->ramchain.addrDB,(uint8_t *)coinaddr,len)) == 0 )
        coin777_getDB(&addrind,&tmp,coin->ramchain.DBs.transactions,coin->ramchain.addrDB.DB,coinaddr,len);
    return(addrind);
}

int32_t coin777_txidstr(struct coin777 *coin,char *txidstr,int32_t max,uint32_t txidind,uint32_t addrind)
{
    bits256 txid;
    if ( coin777_RWmmap(0,&txid,coin,&coin->ramchain.txidbits,txidind) == 0 )
        init_hexbytes_noT(txidstr,txid.bytes,sizeof(txid));
    return(0);
}

int32_t coin777_unspentmap(uint32_t *txidindp,char *txidstr,struct coin777 *coin,uint32_t unspentind)
{
    uint32_t floor,ceiling,probe,firstvout,lastvout,txoffsets[2],nexttxoffsets[2];
    floor = 1, ceiling = coin->ramchain.latest.txidind;
    *txidindp = 0;
    while ( floor != ceiling )
    {
        probe = (floor + ceiling) >> 1;
        if ( coin777_RWmmap(0,txoffsets,coin,&coin->ramchain.txoffsets,probe) == 0 && coin777_RWmmap(0,nexttxoffsets,coin,&coin->ramchain.txoffsets,probe+1) == 0 )
        {
            if ( (firstvout= txoffsets[0]) == 0 || (lastvout= nexttxoffsets[0]) == 0 )
                break;
            //printf("search %u, probe.%u (%u %u) floor.%u ceiling.%u\n",unspentind,probe,firstvout,lastvout,floor,ceiling);
            if ( unspentind < firstvout )
                ceiling = probe;
            else if ( unspentind >= lastvout )
                floor = probe;
            else
            {
                *txidindp = probe;
                //printf("found match! txidind.%u\n",probe);
                if ( coin777_txidstr(coin,txidstr,255,probe,0) == 0 )
                    return(unspentind - firstvout);
                else break;
            }
        }
        else
        {
            printf("error loading txoffsets at probe.%d\n",probe);
            break;
        }
    }
    printf("end search %u, probe.%u (%u %u) floor.%u ceiling.%u\n",unspentind,probe,firstvout,lastvout,floor,ceiling);
    return(-1);
}

uint64_t coin777_Uvalue(struct unspent_info *U,struct coin777 *coin,uint32_t unspentind)
{
    if ( coin777_RWmmap(0,U,coin,&coin->ramchain.unspents,unspentind) == 0 )
        return(U->value);
    else printf("error getting unspents[%u] when %d\n",unspentind,coin->ramchain.latest.unspentind);
    return(0);
}

uint64_t coin777_Svalue(struct spend_info *S,struct coin777 *coin,uint32_t spendind)
{
    struct unspent_info U;
    if ( coin777_RWmmap(0,S,coin,&coin->ramchain.spends,spendind) == 0 )
        return(coin777_Uvalue(&U,coin,S->unspentind));
    else printf("error getting spendind[%u] when %d\n",spendind,coin->ramchain.latest.numspends);
    return(0);
}

uint64_t coin777_value(struct coin777 *coin,uint32_t *unspentindp,struct unspent_info *U,uint32_t txidind,int16_t vout)
{
    uint32_t unspentind,txoffsets[2];
    if ( coin777_RWmmap(0,txoffsets,coin,&coin->ramchain.txoffsets,txidind) == 0  )
    {
        (*unspentindp) = unspentind = txoffsets[0] + vout;
        return(coin777_Uvalue(U,coin,unspentind));
    } else printf("error getting txoffsets for txidind.%u\n",txidind);
    return(0);
}

// coin777 addrinfo funcs
#define coin777_scriptptr(A) ((A)->scriptlen == 0 ? 0 : (uint8_t *)&(A)->coinaddr[(A)->addrlen])

int32_t coin777_script0(struct coin777 *coin,uint32_t addrind,uint8_t *script,int32_t scriptlen)
{
    struct coin777_addrinfo A; uint8_t *scriptptr;
    if ( coin777_RWmmap(0,&A,coin,&coin->ramchain.addrinfos,addrind) == 0 && A.scriptlen == scriptlen && (scriptptr= coin777_scriptptr(&A)) != 0 )
        return(memcmp(script,scriptptr,scriptlen) == 0);
    return(0);
}

uint32_t coin777_scriptind(uint32_t *firstblocknump,struct coin777 *coin,char *coinaddr,char *scriptstr)
{
    uint8_t script[4096]; uint32_t addrind,scriptind; int32_t scriptlen,tmp = sizeof(scriptind);
    *firstblocknump = 0;
    if ( scriptstr == 0 || scriptstr[0] == 0 )
        return(0xffffffff);
    scriptlen = (int32_t)strlen(scriptstr) >> 1, decode_hex(script,scriptlen,scriptstr);
    if ( (addrind= coin777_addrind(firstblocknump,coin,coinaddr)) != 0 && coin777_script0(coin,addrind,script,scriptlen) != 0 )
        return(0);
    if ( (scriptind= coin777_findind(coin,&coin->ramchain.scriptDB,script,scriptlen)) == 0 )
        coin777_getDB(&scriptind,&tmp,coin->ramchain.DBs.transactions,coin->ramchain.scriptDB.DB,script,scriptlen);
    if ( scriptind == 0 )
        return(0xffffffff);
    return(scriptind);
}

int32_t coin777_scriptstr(struct coin777 *coin,char *scriptstr,int32_t max,uint32_t scriptind,uint32_t addrind)
{
    struct coin777_addrinfo A; uint8_t script[8192],*ptr,*scriptptr; int32_t scriptlen;
    scriptstr[0] = 0;
    if ( scriptind != 0 )
    {
        scriptlen = sizeof(script);
        if ( (ptr= coin777_getDB(script,&scriptlen,coin->ramchain.DBs.transactions,coin->ramchain.scriptDB.DB,&scriptind,sizeof(scriptind))) != 0 )
        {
            if ( scriptlen < max )
                init_hexbytes_noT(scriptstr,script,scriptlen);
            else printf("scriptlen.%d too big max.%d for scriptind.%u\n",scriptlen,max,scriptind);
        } else printf("couldnt find scriptind.%d for addrind.%u\n",scriptind,addrind);
    }
    else
    {
        memset(&A,0,sizeof(A));
        if ( coin777_RWmmap(0,&A,coin,&coin->ramchain.addrinfos,addrind) == 0 && (scriptptr= coin777_scriptptr(&A)) != 0 )
            init_hexbytes_noT(scriptstr,scriptptr,A.scriptlen);
    }
    return(0);
}

int32_t coin777_coinaddr(struct coin777 *coin,char *coinaddr,int32_t max,uint32_t addrind,uint32_t addrind2)
{
    struct coin777_addrinfo A;
    memset(&A,0,sizeof(A));
    if ( coin777_RWmmap(0,&A,coin,&coin->ramchain.addrinfos,addrind) == 0 )
        strcpy(coinaddr,A.coinaddr);
    return(-1);
}

void update_addrinfosha256(uint8_t *sha256,struct sha256_state *state,uint32_t blocknum,char *coinaddr,int32_t addrlen,uint8_t *script,int32_t scriptlen)
{
    update_sha256(sha256,state,(uint8_t *)&blocknum,sizeof(blocknum));
    update_sha256(sha256,state,(uint8_t *)coinaddr,addrlen);
    update_sha256(sha256,state,script,scriptlen);
}

void update_ledgersha256(uint8_t *sha256,struct sha256_state *state,int64_t value,uint32_t addrind,uint32_t blocknum)
{
    uint32_t buflen = 0; uint8_t buf[64];
    memcpy(&buf[buflen],&value,sizeof(value)), buflen += sizeof(value);
    memcpy(&buf[buflen],&addrind,sizeof(addrind)), buflen += sizeof(addrind);
    memcpy(&buf[buflen],&blocknum,sizeof(blocknum)), buflen += sizeof(blocknum);
    update_sha256(sha256,state,buf,buflen);
}

void update_blocksha256(uint8_t *sha256,struct sha256_state *state,struct coin_offsets *B)
{
    struct coin_offsets tmpB = *B;
    tmpB.totaladdrtx = 0;
    memset(tmpB.check,0,sizeof(tmpB.check));
    update_sha256(sha256,state,(uint8_t *)&tmpB,sizeof(tmpB));
}

int32_t coin777_RWaddrtx(int32_t writeflag,struct coin777 *coin,uint32_t addrind,struct addrtx_info *ATX,struct coin777_Lentry *L,int32_t addrtxi)
{
    struct coin777_addrinfo A; struct addrtx_info *atxA;
    if ( L->insideA != 0 )
    {
        coin777_RWmmap(0,&A,coin,&coin->ramchain.addrinfos,addrind);
        atxA = (struct addrtx_info *)((uint64_t)&A + L->first_addrtxi), atxA += addrtxi;
        if ( writeflag == 0 )
            *ATX = *atxA;
        else *atxA = *ATX, coin777_RWmmap(2,&A,coin,&coin->ramchain.addrinfos,addrind);
        return(0);
    }
    else return(coin777_RWmmap(writeflag,ATX,coin,&coin->ramchain.addrtx,L->first_addrtxi + addrtxi));
    return(-1);
}

uint32_t coin777_addrtxalloc(struct coin777 *coin,struct coin777_Lentry *L,int32_t newmax,uint32_t *totaladdrtxp)
{
    int32_t i; struct addrtx_info ATX;
    L->first_addrtxi = (*totaladdrtxp);
    memset(&ATX,0,sizeof(ATX));
    for (i=0; i<=newmax; i++)
        coin777_RWmmap(1,&ATX,coin,&coin->ramchain.addrtx,(*totaladdrtxp)++);
    L->maxaddrtx = newmax;
    L->insideA = 0;
    return((uint32_t)L->first_addrtxi);
}

struct addrtx_info *coin777_compact(int32_t compactflag,uint64_t *balancep,int32_t *numaddrtxp,struct coin777 *coin,uint32_t addrind,struct coin777_Lentry *L)
{
    int32_t i,num,addrtxi,iter; struct unspent_info U; struct addrtx_info ATX,*actives = 0; uint64_t balance = 0;
    for (iter=num=addrtxi=0; iter<2; iter++)
    {
        for (i=0; i<L->numaddrtx; i++)
        {
            coin777_RWaddrtx(0,coin,addrind,&ATX,L,i);
            if ( compactflag == 0 || ATX.spendind == 0 )
            {
                if ( iter == 0 )
                    num++;
                else
                {
                    actives[addrtxi++] = ATX;
                    if ( ATX.spendind == 0 )
                    {
                        //printf("(%d) ",ATX.unspentind);
                        balance += coin777_Uvalue(&U,coin,ATX.unspentind);
                    }
                }
            }
        }
        if ( iter == 0 && num > 0 )
            actives = calloc(num,sizeof(*actives));
    }
    if ( addrtxi != 0 && Debuglevel > 2 )
        printf("-> balance %.8f ",dstr(balance));
    //printf("-> balance %.8f numaddrtx.%d -> %d\n",dstr(balance),L->numaddrtx,addrtxi);
    *numaddrtxp = addrtxi;
    if ( balancep != 0 )
        *balancep = balance;
    return(actives);
}

struct addrtx_info *coin777_add_addrtx(struct coin777 *coin,uint32_t addrind,struct addrtx_info *atx,struct coin777_Lentry *L,uint32_t maxunspentind,uint32_t *totaladdrtxp)
{
    struct addrtx_linkptr PTR; struct coin777_Lentry oldL; struct addrtx_info ATX,*actives; struct unspent_info U;
    int32_t i,n,incr = 16; uint64_t balance,atx_value;
    if ( totaladdrtxp != 0 )
    {
        if ( L->numaddrtx >= L->maxaddrtx )
        {
            oldL = *L;
            if ( sizeof(ATX) != sizeof(PTR) )
                printf("coin777_addrtx FATAL datastructure size mismatch %ld vs %ld\n",sizeof(ATX),sizeof(PTR)), debugstop();
            if ( (L->maxaddrtx << 1) > incr )
                incr = (L->maxaddrtx << 1);
            if ( L->first_addrtxi != 0 )
            {
                memset(&PTR,0,sizeof(PTR)), PTR.maxunspentind = maxunspentind, PTR.next_addrtxi = (*totaladdrtxp);
                coin777_RWaddrtx(2,coin,addrind,(struct addrtx_info *)&PTR,&oldL,oldL.maxaddrtx);
            }
            coin777_addrtxalloc(coin,L,incr,totaladdrtxp);
            L->numaddrtx = 0;
            if ( oldL.numaddrtx > 0 )
            {
                actives = coin777_compact(0,&balance,&n,coin,addrind,&oldL);
                L->numaddrtx = n;
                if ( balance != L->balance)
                {
                    L->balance = balance, printf("coin777_addrtx A.%u num %d -> %d warning recalc unspent %llu != %llu | firsti.%u\n",addrind,oldL.numaddrtx,L->numaddrtx,(long long)balance,(long long)L->balance,L->first_addrtxi);
                }
                else
                {
                    for (i=0; i<L->numaddrtx; i++)
                        coin777_RWaddrtx(1,coin,addrind,&actives[i],L,i);
                }
                if ( actives != 0 )
                    free(actives);
            }
            if ( Debuglevel > 2 )
                printf("coin777_addrtx COMPACTED A.%u num %d/%d -> %d/%d balance %.8f with %.8f | firsti.%u\n",addrind,oldL.numaddrtx,oldL.maxaddrtx,L->numaddrtx,incr,dstr(L->balance),dstr(coin777_Uvalue(&U,coin,atx->unspentind)),L->first_addrtxi);
        }
        coin777_RWaddrtx(1,coin,addrind,atx,L,L->numaddrtx++);
        atx_value = coin777_Uvalue(&U,coin,atx->unspentind), L->balance += atx_value;
        if ( Debuglevel > 2 )
            printf("rawind.%u updated addrind.%u L->numaddrtx.%d %.8f -> %.8f num.%d of %d\n",atx->unspentind,addrind,L->numaddrtx,dstr(atx_value),dstr(L->balance),L->numaddrtx,L->maxaddrtx);
        coin777_RWmmap(1,L,coin,&coin->ramchain.ledger,addrind);
        update_ledgersha256(coin->ramchain.ledger.sha256,&coin->ramchain.ledger.state,atx_value,addrind,maxunspentind);
        update_sha256(coin->ramchain.addrtx.sha256,&coin->ramchain.addrtx.state,(uint8_t *)atx,sizeof(*atx));
    }
    else printf("coin777_add_addrtx illegal mode\n"), debugstop();
    return(atx);
}

int32_t coin777_bsearch(struct addrtx_info *atx,struct coin777 *coin,uint32_t addrind,struct coin777_Lentry *L,uint32_t unspentind,uint64_t value)
{
    static long numsearches,numprobes,rangetotal;
    uint32_t floor,ceiling,probe,lastprobe,iter,start,end; int32_t i,n = 0; uint64_t atx_value; struct unspent_info U;
    if ( L->numaddrtx == 0 )
        return(-1);
    floor = 0, ceiling = L->numaddrtx-1;
    numsearches++;
    rangetotal += L->numaddrtx;
    lastprobe = -2;
    while ( floor < ceiling && n++ < 30 )
    {
        if ( (probe= (floor + ceiling) >> 1) == lastprobe )
            probe++;
        lastprobe = probe;
        coin777_RWaddrtx(0,coin,addrind,atx,L,probe);
        if ( Debuglevel > 2 )
            printf("search %u %.8f, probe.%u u%u floor.%u ceiling.%u\n",unspentind,dstr(value),probe,atx->unspentind,floor,ceiling);
        if ( unspentind < atx->unspentind )
            ceiling = probe;
        else if ( unspentind > atx->unspentind )
            floor = probe;
        else
        {
            atx_value = coin777_Uvalue(&U,coin,atx->unspentind);
            if ( atx_value == value )
            {
                if ( Debuglevel > 2 )
                    printf("FOUND MATCH end search %u, probe.%u floor.%u ceiling.%u numsearches.%ld numprobes.%ld averange %.1f %.1f\n",unspentind,probe,floor,ceiling,numsearches,numprobes,(double)rangetotal/numsearches,(double)numprobes/numsearches);
                return(probe);
            }
            else
            {
                printf("unexpected value mismatch %.8f vs %.8f at probe.%u floor.%u ceiling.%u num.%u\n",dstr(atx_value),dstr(value),probe,floor,ceiling,L->numaddrtx);
                break;
            }
        }
    }
    if ( 1 && L->numaddrtx > 1 )
        printf("SEARCH FAILURE %u, probe.%u floor.%u ceiling.%u numsearches.%ld numprobes.%ld %.1f | numiters %d\n",unspentind,probe,floor,ceiling,numsearches,numprobes,(double)numprobes/numsearches,n);
    for (iter=0; iter<2; iter++)
    {
        if ( iter == 0 )
            start = floor, end = ceiling;
        else start = 0, end = L->numaddrtx-1;
        for (i=start; i<=end; i++)
        {
            numprobes++;
            if ( coin777_RWaddrtx(0,coin,addrind,atx,L,i) == 0 )
            {
                if ( unspentind == atx->unspentind )
                {
                    if ( (atx_value= coin777_Uvalue(&U,coin,atx->unspentind)) != value )
                    {
                        printf("coin777_bsearch %d of %d value mismatch uspentind.%u %.8f %.8f\n",i,L->numaddrtx,unspentind,dstr(value),dstr(atx_value));
                        break;
                    }
                    else
                    {
                        if ( L->numaddrtx > 1 )
                            printf("linear search found u%d in  slot.%d when bsearch missed it?\n",unspentind,i);
                        return(i);
                    }
                }
            } else break;
        }
        printf("linear search iter.%d failure\n",iter);
    }
    return(-1);
}

uint64_t coin777_recalc_addrinfo(int32_t dispflag,struct coin777 *coin,uint32_t addrind,struct coin777_Lentry *L,uint32_t *totaladdrtxp,uint32_t maxunspentind,uint32_t maxspendind)
{
    struct addrtx_info ATX; int32_t addrtxi; struct unspent_info U; int64_t balance = 0;
    for (addrtxi=0; addrtxi<L->numaddrtx; addrtxi++)
    {
        memset(&ATX,0,sizeof(ATX));
        if ( coin777_RWaddrtx(0,coin,addrind,&ATX,L,addrtxi) == 0 && ATX.unspentind < maxunspentind )
        {
            if ( dispflag != 0 )
                fprintf(stderr,"(%u %.8f).s%u ",ATX.unspentind,dstr(coin777_Uvalue(&U,coin,ATX.unspentind)),ATX.spendind);
            if ( ATX.unspentind != 0 && (ATX.spendind == 0 || ATX.spendind >= maxspendind) )
                balance += coin777_Uvalue(&U,coin,ATX.unspentind);
        }
        else
        {
            printf("cant get addrtxi.%d of num.%d max.%d unspentind.%u maxunspentind.%u maxspendind.%u balance %.8f -> %.8f\n",addrtxi,L->numaddrtx,L->maxaddrtx,ATX.unspentind,maxunspentind,maxspendind,dstr(L->balance),dstr(balance));
            break;
        }
    }
    L->numaddrtx = addrtxi;
    if ( dispflag != 0 )
        printf("-> balance %.8f numaddrtx.%d max.%d\n",dstr(balance),L->numaddrtx,L->maxaddrtx);
    return(balance);
}

int32_t coin777_update_addrinfo(struct coin777 *coin,uint32_t addrind,uint32_t unspentind,uint64_t value,uint32_t spendind,uint32_t maxunspentind,uint32_t *totaladdrtxp)
{
    int32_t i,addrtxi; struct addrtx_info ATX; struct unspent_info U; struct coin777_Lentry L;
    if ( value == 0 )
        return(0);
    if ( coin777_RWmmap(0,&L,coin,&coin->ramchain.ledger,addrind) == 0 )
    {
        if ( spendind != 0 )
        {
            if ( (addrtxi= coin777_bsearch(&ATX,coin,addrind,&L,unspentind,value)) >= 0  )
            {
                ATX.spendind = spendind;
                coin777_RWaddrtx(2,coin,addrind,&ATX,&L,addrtxi);
                L.balance -= value;
                return(coin777_RWmmap(1,&L,coin,&coin->ramchain.ledger,addrind));
            }
            else
            {
                for (i=0; i<L.numaddrtx; i++)
                    if ( coin777_RWaddrtx(0,coin,addrind,&ATX,&L,i) == 0 )
                        printf("(U%u %.8f).S%u ",ATX.unspentind,dstr(coin777_Uvalue(&U,coin,ATX.unspentind)),ATX.spendind);
                printf("coin777_update_Lentry: couldnt find unspentind.%u %.8f addrind.%u %.8f num.%d max.%d insideA.%d firsti.%u\n",unspentind,dstr(value),addrind,dstr(value),L.numaddrtx,L.maxaddrtx,L.insideA,L.first_addrtxi), debugstop();
            }
        }
        else
        {
            memset(&ATX,0,sizeof(ATX)), ATX.unspentind = unspentind;
            coin777_add_addrtx(coin,addrind,&ATX,&L,maxunspentind,totaladdrtxp);
            if ( addrind == 13045 )
                printf("addrind.%u U.%u updated %.8f -> balance %.8f num.%d max.%d\n",addrind,unspentind,dstr(value),dstr(L.balance),L.numaddrtx,L.maxaddrtx);
        }
        return(0);
    }
    else printf("coin777_unspent cant find addrinfo for addrind.%u\n",addrind);
    return(-1);
}

uint64_t addrinfos_sum(struct coin777 *coin,uint32_t maxaddrind,int32_t syncflag,uint32_t maxunspentind,uint32_t maxspendind,int32_t recalcflag,uint32_t *totaladdrtxp)
{
    struct coin777_addrinfo A; struct coin777_Lentry L;
    int64_t sum = 0; uint32_t addrind; int32_t errs = 0; int64_t calcbalance;
    for (addrind=1; addrind<maxaddrind; addrind++)
    {
        if ( coin777_RWmmap(0,&A,coin,&coin->ramchain.addrinfos,addrind) == 0 && coin777_RWmmap(0,&L,coin,&coin->ramchain.ledger,addrind) == 0 )
        {
            if ( recalcflag != 0 )
            {
                if ( (calcbalance= coin777_recalc_addrinfo(0,coin,addrind,&L,totaladdrtxp,maxunspentind,maxspendind)) != L.balance )
                {
                    coin777_recalc_addrinfo(1,coin,addrind,&L,totaladdrtxp,maxunspentind,maxspendind);
                    printf("found mismatch: (%.8f -> %.8f) addrind.%d num.%d of %d | firsti %u maxunspentind.%u maxspend.%u\n",dstr(L.balance),dstr(calcbalance),addrind,L.numaddrtx,L.maxaddrtx,L.first_addrtxi,maxunspentind,maxspendind);
                    L.balance = calcbalance;
                    errs++, coin777_RWmmap(1,&L,coin,&coin->ramchain.ledger,addrind);
                }
            }
            sum += L.balance;
        } else printf("error loading addrinfo or ledger entry for addrind.%u\n",addrind);
    }
    if ( errs != 0 || syncflag < 0 )
        printf("addrinfos_sum @ maxunspentind.%u errs.%d -> sum %.8f\n",maxunspentind,errs,dstr(sum));
    return(sum);
}

uint64_t coin777_unspents(uint64_t (*unspentsfuncp)(struct coin777 *coin,void *args,uint32_t addrind,struct addrtx_info *unspents,int32_t num,uint64_t balance),struct coin777 *coin,char *coinaddr,void *args)
{
    uint32_t addrind,firstblocknum; struct coin777_Lentry L; struct addrtx_info *unspents; uint64_t balance,sum = 0; int32_t n;
    if ( (addrind= coin777_addrind(&firstblocknum,coin,coinaddr)) != 0 )
    {
        if ( coin777_RWmmap(0,&L,coin,&coin->ramchain.ledger,addrind) == 0 && (unspents= coin777_compact(1,&balance,&n,coin,addrind,&L)) != 0 )
        {
            if ( unspentsfuncp != 0 )
                sum = (*unspentsfuncp)(coin,args,addrind,unspents,n,balance);
            else sum = balance;
            //printf("{%.8f} ",dstr(sum));
            free(unspents);
        }
    }
    return(sum);
}

// coin777 add funcs
int32_t coin777_add_addrinfo(struct coin777 *coin,uint32_t addrind,char *coinaddr,int32_t len,uint8_t *script,uint16_t scriptlen,uint32_t blocknum,uint32_t *totaladdrtxp)
{
    struct coin777_addrinfo A; struct coin777_Lentry L; uint8_t *scriptptr; uint32_t offset;
    memset(&A,0,sizeof(A));
    update_addrinfosha256(coin->ramchain.addrinfos.sha256,&coin->ramchain.addrinfos.state,blocknum,coinaddr,len,script,scriptlen);
    A.firstblocknum = blocknum;
    A.addrlen = len, memcpy(A.coinaddr,coinaddr,len);
    if ( (scriptlen + A.addrlen + sizeof(struct addrtx_info)*3) <= sizeof(A.coinaddr) )
    {
        A.scriptlen = scriptlen;
        if ( (scriptptr= coin777_scriptptr(&A)) != 0 )
            memcpy(scriptptr,script,scriptlen), len += scriptlen;
    }
    coin777_RWmmap(0,&L,coin,&coin->ramchain.ledger,addrind);
    memset(&L,0,sizeof(L));
    offset = (uint32_t)((uint64_t)&A.coinaddr[len] - (uint64_t)&A);
    if ( (offset & 3) != 0 )
        offset += 4 - (offset & 3), len += (4 - (offset & 3));
    L.first_addrtxi = offset; //(struct addrtx_info *)(long)
    if ( (L.maxaddrtx= (int32_t)((sizeof(A.coinaddr) - len) / sizeof(struct addrtx_info))) > 0 )
        L.insideA = 1;
    else
    {
        A.root_addrtxi = *totaladdrtxp;
        coin777_addrtxalloc(coin,&L,16,totaladdrtxp);
        printf("not enough space for embedded addrtx A%u \n",addrind);
    }
    coin777_RWmmap(1,&L,coin,&coin->ramchain.ledger,addrind);
    //coin777_RWmmap(0,&tmpA,coin,&coin->ramchain.addrinfos,addrind);
    //memcpy(&tmpA,&A,sizeof(A) - sizeof(A.coinaddr) + len);
    //coin777_RWmmap(1,&tmpA,coin,&coin->ramchain.addrinfos,addrind);
    coin777_RWmmap(1,&A,coin,&coin->ramchain.addrinfos,addrind);
    coin->ramchain.totalsize += sizeof(A);
    return(coin777_scriptptr(&A) != 0 );
}

uint32_t coin777_addscript(struct coin777 *coin,uint32_t *scriptindp,uint8_t *script,int32_t scriptlen,int32_t script0flag)
{
    uint32_t scriptind = 0; int32_t retval = 0;
    if ( script0flag == 0 )
    {
        scriptind = (*scriptindp)++;
        retval = coin777_addDB(coin,coin->ramchain.DBs.transactions,coin->ramchain.scriptDB.DB,script,scriptlen,&scriptind,sizeof(scriptind));
        retval += coin777_addDB(coin,coin->ramchain.DBs.transactions,coin->ramchain.scriptDB.DB,&scriptind,sizeof(scriptind),script,scriptlen);
    }
    if ( Debuglevel > 2 )
        printf("NEW SCRIPT scriptind.%u [%u] script0flag.%d\n",scriptind,(*scriptindp),script0flag);
    return(scriptind);
}

int32_t coin777_addvout(void *state,uint64_t *creditsp,uint32_t txidind,uint16_t vout,uint32_t unspentind,char *coinaddr,char *scriptstr,uint64_t value,uint32_t *addrindp,uint32_t *scriptindp,uint32_t *totaladdrtxp,uint32_t blocknum)
{
    struct coin777 *coin = state; uint32_t *ptr,addrind,scriptind = 0; int32_t newflag = 0,script0flag=0,tmp,len,scriptlen;
    uint8_t script[4096]; struct unspent_info U;
    (*creditsp) += value;
    scriptlen = (int32_t)strlen(scriptstr) >> 1, decode_hex(script,scriptlen,scriptstr);
    update_sha256(coin->ramchain.scriptDB.sha256,&coin->ramchain.scriptDB.state,(uint8_t *)scriptstr,scriptlen << 1);
    len = (int32_t)strlen(coinaddr) + 1;
    if ( Debuglevel > 2 )
        printf("addvout.%d: (%s) (%s) %.8f\n",vout,coinaddr,scriptstr,dstr(value));
    if ( (addrind= coin777_findind(coin,&coin->ramchain.addrDB,(uint8_t *)coinaddr,len)) == 0 )
    {
        tmp = sizeof(addrind);
        if ( (ptr= coin777_getDB(&addrind,&tmp,coin->ramchain.DBs.transactions,coin->ramchain.addrDB.DB,coinaddr,len)) == 0 || addrind == 0 )
        {
            //printf("ptr.%p addrind.%u (%s).%u\n",ptr,addrind,coinaddr,*addrindp);
            newflag = 1, addrind = (*addrindp)++;
            update_sha256(coin->ramchain.addrDB.sha256,&coin->ramchain.addrDB.state,(uint8_t *)coinaddr,len);
            coin777_addDB(coin,coin->ramchain.DBs.transactions,coin->ramchain.addrDB.DB,coinaddr,len,&addrind,sizeof(addrind));
            /*uint32_t checkind; tmp = sizeof(checkind);
            if ( (ptr= coin777_getDB(&checkind,&tmp,coin->ramchain.DBs.transactions,coin->ramchain.addrDB.DB,coinaddr,len)) == 0 || checkind != addrind )
                printf("ERROR ptr.%p checkind.%u vs addrind.%u\n",ptr,checkind,addrind);*/
        }
        else
        {
            if ( addrind == (*addrindp) )
            {
                update_sha256(coin->ramchain.addrDB.sha256,&coin->ramchain.addrDB.state,(uint8_t *)coinaddr,len);
                newflag = 1, (*addrindp)++;
            }
            else if ( addrind > (*addrindp) )
            {
                printf("DB returned addrind.%u vs (*addrindp).%u\n",addrind,(*addrindp)), debugstop();
                (*addrindp) = (addrind + 1);
            }
        }
        coin777_addind(coin,&coin->ramchain.addrDB,coinaddr,len,addrind);
        if ( newflag != 0 )
        {
            script0flag = coin777_add_addrinfo(coin,addrind,coinaddr,len,script,scriptlen,blocknum,totaladdrtxp);
            if ( script0flag == 0 && (scriptind= coin777_findind(coin,&coin->ramchain.scriptDB,script,scriptlen)) == 0 )
            {
                coin777_addscript(coin,scriptindp,script,scriptlen,script0flag);
                coin777_addind(coin,&coin->ramchain.scriptDB,script,scriptlen,scriptind);
            }
        }
    }
    else
    {
        if ( addrind == (*addrindp) )
        {
            update_sha256(coin->ramchain.addrDB.sha256,&coin->ramchain.addrDB.state,(uint8_t *)coinaddr,len);
            newflag = 1, (*addrindp)++;
        }
        else if ( addrind > (*addrindp) )
        {
            printf("DB returned addrind.%u vs (*addrindp).%u\n",addrind,(*addrindp)), debugstop();
            (*addrindp) = (addrind + 1);
        }
    }
    if ( (script0flag + scriptind) == 0 && (script0flag= coin777_script0(coin,addrind,script,scriptlen)) == 0 )
    {
        //printf("search for (%s)\n",scriptstr);
        if ( (scriptind= coin777_findind(coin,&coin->ramchain.scriptDB,script,scriptlen)) == 0 )
        {
            tmp = sizeof(scriptind);
            if ( (ptr= coin777_getDB(&scriptind,&tmp,coin->ramchain.DBs.transactions,coin->ramchain.scriptDB.DB,script,scriptlen)) == 0 || scriptind == 0 )
                scriptind = coin777_addscript(coin,scriptindp,script,scriptlen,0);
            else
            {
                if ( Debuglevel > 2 )
                    printf("cant find  (%s) -> scriptind.%u [%u]\n",scriptstr,scriptind,(*scriptindp));
                if ( scriptind == (*scriptindp) )
                    (*scriptindp)++;
                else if ( scriptind > (*scriptindp) )
                {
                    printf("DB returned scriptind.%u vs (*scriptindp).%u\n",scriptind,(*scriptindp)), debugstop();
                    (*scriptindp) = scriptind + 1;
                }
            }
            coin777_addind(coin,&coin->ramchain.scriptDB,script,scriptlen,scriptind);
        }
        else
        {
            if ( scriptind == (*scriptindp) )
                (*scriptindp)++;
            else if ( scriptind > (*scriptindp) )
            {
                printf("DB returned scriptind.%u vs (*scriptindp).%u\n",scriptind,(*scriptindp)), debugstop();
                (*scriptindp) = scriptind + 1;
            }
        }
    }
    if ( Debuglevel > 2 )
        printf("UNSPENT.%u addrind.%u T%u vo%-3d U%u %.8f %s %llx %s\n",unspentind,addrind,txidind,vout,unspentind,dstr(value),coinaddr,*(long long *)script,scriptstr);
    memset(&U,0,sizeof(U)), U.value = value, U.addrind = addrind;
    if ( script0flag != 0 )
        U.rawind_or_blocknum = scriptind;
    else U.rawind_or_blocknum = blocknum, U.isblocknum = 1;
    coin777_RWmmap(1 | COIN777_SHA256,&U,coin,&coin->ramchain.unspents,unspentind);
    coin->ramchain.totalsize += sizeof(U);
    return(coin777_update_addrinfo(coin,addrind,unspentind,value,0,blocknum,totaladdrtxp));
}

uint64_t coin777_addvin(void *state,uint64_t *debitsp,uint32_t txidind,uint16_t vin,uint32_t totalspends,char *spent_txidstr,uint16_t spent_vout,uint32_t blocknum,uint32_t *totaladdrtxp)
{
    struct coin777 *coin = state; bits256 txid; int32_t tmp; uint32_t *ptr,unspentind,spent_txidind; struct unspent_info U; struct spend_info S;
    memset(txid.bytes,0,sizeof(txid)), decode_hex(txid.bytes,sizeof(txid),spent_txidstr);
    if ( (spent_txidind= coin777_findind(coin,&coin->ramchain.txidDB,txid.bytes,sizeof(txid))) == 0 )
    {
        tmp = sizeof(spent_txidind);
        if ( (ptr= coin777_getDB(&spent_txidind,&tmp,coin->ramchain.DBs.transactions,coin->ramchain.txidDB.DB,txid.bytes,sizeof(txid))) == 0 || spent_txidind == 0 || tmp != sizeof(*ptr) )
        {
            printf("cant find %016llx txid.(%s) ptr.%p spent_txidind.%u spendvout.%d from len.%ld (%s)\n",(long long)txid.txid,spent_txidstr,ptr,spent_txidind,spent_vout,sizeof(txid),db777_errstr(coin->ramchain.DBs.ctl)), debugstop();
            return(-1);
        }
    }
    if ( spent_txidind > txidind )
    {
        printf("coin777_addvin txidind overflow? spent_txidind.%u vs max.%u\n",spent_txidind,txidind), debugstop();
        return(-2);
    }
    if ( coin777_value(coin,&unspentind,&U,spent_txidind,spent_vout) != 0 )
    {
        if ( Debuglevel > 2 )
            printf("SPEND T%u vi%-3d S%u %s vout.%d -> A%u %.8f\n",txidind,vin,totalspends,spent_txidstr,spent_vout,U.addrind,dstr(U.value));
        S.unspentind = unspentind, S.addrind = U.addrind, S.spending_txidind = txidind, S.spending_vin = vin;
        coin777_RWmmap(1 | COIN777_SHA256,&S,coin,&coin->ramchain.spends,totalspends);
        coin->ramchain.totalsize += sizeof(S);
        coin777_update_addrinfo(coin,U.addrind,unspentind,U.value,totalspends,blocknum,totaladdrtxp);
        (*debitsp) += U.value;
        return(U.value);
    } else printf("warning: (%s).v%d null value\n",spent_txidstr,spent_vout);
    return(0);
}

int32_t coin777_addtx(void *state,uint32_t blocknum,uint32_t txidind,char *txidstr,uint32_t firstvout,uint16_t numvouts,uint64_t total,uint32_t firstvin,uint16_t numvins)
{
    struct coin777 *coin = state; bits256 txid; uint32_t txoffsets[2];
    memset(txid.bytes,0,sizeof(txid)), decode_hex(txid.bytes,sizeof(txid),txidstr);
    coin777_RWmmap(1 | COIN777_SHA256,&txid,coin,&coin->ramchain.txidbits,txidind);
    update_sha256(coin->ramchain.txidDB.sha256,&coin->ramchain.txidDB.state,(uint8_t *)txidstr,(int32_t)sizeof(txid)*2);
    coin777_addDB(coin,coin->ramchain.DBs.transactions,coin->ramchain.txidDB.DB,txid.bytes,sizeof(txid),&txidind,sizeof(txidind));
    if ( Debuglevel > 2 )
        printf("ADDTX.%s: %x T%u U%u + numvouts.%d, S%u + numvins.%d\n",txidstr,*(int *)txid.bytes,txidind,firstvout,numvouts,firstvin,numvins);
    txoffsets[0] = firstvout, txoffsets[1] = firstvin, coin777_RWmmap(1 | COIN777_SHA256,txoffsets,coin,&coin->ramchain.txoffsets,txidind);
    txoffsets[0] += numvouts, txoffsets[1] += numvins, coin777_RWmmap(1,txoffsets,coin,&coin->ramchain.txoffsets,txidind+1);
    coin777_addind(coin,&coin->ramchain.txidDB,txid.bytes,sizeof(txid),txidind);
    coin->ramchain.totalsize += sizeof(txoffsets) + sizeof(txid);
    return(0);
}

int32_t coin777_addblock(void *state,uint32_t blocknum,char *blockhashstr,char *merklerootstr,uint32_t timestamp,uint64_t minted,uint32_t txidind,uint32_t unspentind,uint32_t numspends,uint32_t addrind,uint32_t scriptind,uint32_t totaladdrtx,uint64_t credits,uint64_t debits)
{
    bits256 blockhash,merkleroot; struct coin777 *coin = state; struct coin_offsets zeroB,B,tmpB,block; int32_t i,flag,err = 0;
    memset(&B,0,sizeof(B));
//Debuglevel = 3;
    if ( Debuglevel > 2 )
        printf("B.%u T.%u U.%u S.%u A.%u C.%u\n",blocknum,txidind,unspentind,numspends,addrind,scriptind);
    if ( blockhashstr != 0 ) // start of block
    {
        memset(blockhash.bytes,0,sizeof(blockhash)), decode_hex(blockhash.bytes,sizeof(blockhash),blockhashstr);
        memset(merkleroot.bytes,0,sizeof(merkleroot)), decode_hex(merkleroot.bytes,sizeof(merkleroot),merklerootstr);
        B.blockhash = blockhash, B.merkleroot = merkleroot;
    } // else end of block, but called with blocknum+1
    B.timestamp = timestamp, B.txidind = txidind, B.unspentind = unspentind, B.numspends = numspends, B.addrind = addrind, B.scriptind = scriptind, B.totaladdrtx = totaladdrtx;
    B.credits = credits, B.debits = debits;
    for (i=0; i<coin->ramchain.num; i++)
        B.check[i] = coin->ramchain.sps[i]->sha256[0];
    if ( coin777_RWmmap(0,&block,coin,&coin->ramchain.blocks,blocknum) == 0  )
    {
        if ( memcmp(&B,&block,sizeof(B)) != 0 )
        {
            memset(&zeroB,0,sizeof(zeroB));
            if ( memcmp(&B,&zeroB,sizeof(zeroB)) != 0 )
            {
                if ( block.timestamp != 0 && B.timestamp != block.timestamp )
                    err = -2, printf("nonz timestamp.%u overwrites %u\n",B.timestamp,block.timestamp);
                if ( block.txidind != 0 && B.txidind != block.txidind )
                    err = -3, printf("nonz txidind.%u overwrites %u\n",B.txidind,block.txidind);
                if ( block.unspentind != 0 && B.unspentind != block.unspentind )
                    err = -4, printf("nonz unspentind.%u overwrites %u\n",B.unspentind,block.unspentind);
                if ( block.numspends != 0 && B.numspends != block.numspends )
                    err = -5, printf("nonz numspends.%u overwrites %u\n",B.numspends,block.numspends);
                if ( block.addrind != 0 && B.addrind != block.addrind )
                    err = -6, printf("nonz addrind.%u overwrites %u\n",B.addrind,block.addrind);
                if ( block.scriptind != 0 && B.scriptind != block.scriptind )
                    err = -7, printf("nonz scriptind.%u overwrites %u\n",B.scriptind,block.scriptind);
                if ( block.credits != 0 && B.credits != 0 && B.credits != block.credits )
                    err = -8, printf("nonz total %.8f overwrites %.8f\n",dstr(B.credits),dstr(block.credits));
                if ( block.debits != 0 && B.debits != 0 && B.debits != block.debits )
                    err = -9, printf("nonz debits %.8f overwrites %.8f\n",dstr(B.debits),dstr(block.debits));
            }
        }
        coin->ramchain.latest = B, coin->ramchain.latestblocknum = blocknum;
        if ( blockhashstr != 0 )
        {
            flag = 1;
            update_blocksha256(coin->ramchain.blocks.sha256,&coin->ramchain.blocks.state,&B);
            coin->ramchain.totalsize += sizeof(B);
        }
        else
        {
            flag = 2;
            if ( coin777_RWmmap(0,&tmpB,coin,&coin->ramchain.blocks,blocknum) == 0  )
                B.blockhash = tmpB.blockhash, B.merkleroot = tmpB.merkleroot;
        }
        if ( coin777_RWmmap(flag,&B,coin,&coin->ramchain.blocks,blocknum) != 0 )
            return(-1);
    }
    return(err);
}

// coin777 sync/resume funcs
struct coin777_hashes *coin777_getsyncdata(struct coin777_hashes *H,struct coin777 *coin,int32_t synci)
{
    struct coin777_hashes *hp; int32_t allocsize = sizeof(*H);
    if ( synci <= 0 )
        synci++;
    if ( (hp= coin777_getDB(H,&allocsize,coin->ramchain.DBs.transactions,coin->ramchain.hashDB.DB,&synci,sizeof(synci))) != 0 )
        return(hp);
    else memset(H,0,sizeof(*H));
    printf("couldnt find synci.%d keylen.%ld\n",synci,sizeof(synci));
    return(0);
}

int32_t coin777_syncblocks(struct coin777_hashes *inds,int32_t max,struct coin777 *coin)
{
    struct coin777_hashes H,*hp; int32_t synci,n = 0;
    if ( (hp= coin777_getsyncdata(&H,coin,-1)) != 0 )
    {
        inds[n++] = *hp;
        for (synci=coin->ramchain.numsyncs; synci>0&&n<max; synci--)
        {
            if ( (hp= coin777_getsyncdata(&H,coin,synci)) != 0 )
                inds[n++] = *hp;
        }
    } else printf("null return from coin777_getsyncdata\n");
    return(n);
}

uint64_t coin777_ledgerhash(char *ledgerhash,struct coin777_hashes *H)
{
    bits256 hashbits;
    if ( H != 0 )
    {
        calc_sha256(0,hashbits.bytes,(uint8_t *)(void *)((uint64_t)H + sizeof(H->ledgerhash)),(int32_t)(sizeof(*H) - sizeof(H->ledgerhash)));
        H->ledgerhash = hashbits.txid;
        if ( ledgerhash != 0 )
            ledgerhash[0] = 0, init_hexbytes_noT(ledgerhash,hashbits.bytes,sizeof(hashbits));
        return(hashbits.txid);
    }
    return(0);
}

void coin777_genesishash(struct coin777 *coin,struct coin777_hashes *H)
{
    int32_t i;
    memset(H,0,sizeof(*H));
    coin777_getinds(coin,0,&H->credits,&H->debits,&H->timestamp,&H->txidind,&H->unspentind,&H->numspends,&H->addrind,&H->scriptind,&H->totaladdrtx);
    H->numsyncs = 1;
    for (i=0; i<coin->ramchain.num; i++)
        update_sha256(H->sha256[i],&H->states[i],0,0);
    H->ledgerhash = coin777_ledgerhash(0,H);
}

uint32_t coin777_hashes(int32_t *syncip,struct coin777_hashes *bestH,struct coin777 *coin,uint32_t refblocknum,int32_t lastsynci)
{
    int32_t synci,flag = 0; struct coin777_hashes *hp,H;
    *syncip = -1;
    for (synci=1; synci<=lastsynci; synci++)
    {
        if ( (hp= coin777_getsyncdata(&H,coin,synci)) == 0 || hp->blocknum > refblocknum )
            break;
        *bestH = *hp;
        *syncip = synci;
        flag = 1;
    }
    if ( flag == 0 )
        coin777_genesishash(coin,bestH);
    return(bestH->blocknum);
}

// coin777 init funcs
uint32_t coin777_startblocknum(struct coin777 *coin,uint32_t synci)
{
    struct coin777_hashes H,*hp; struct coin_offsets B; int32_t i; uint32_t blocknum = 0; uint64_t ledgerhash;
    if ( (hp= coin777_getsyncdata(&H,coin,synci)) == &H )
    {
        coin->ramchain.blocknum = blocknum = hp->blocknum, coin->ramchain.numsyncs = hp->numsyncs;
        if ( coin777_RWmmap(0,&B,coin,&coin->ramchain.blocks,blocknum) == 0  )
        {
            B.credits = hp->credits, B.debits = hp->debits;
            B.timestamp = hp->timestamp, B.txidind = hp->txidind, B.unspentind = hp->unspentind, B.numspends = hp->numspends, B.addrind = hp->addrind, B.scriptind = hp->scriptind, B.totaladdrtx = hp->totaladdrtx;
            coin777_RWmmap(1,&B,coin,&coin->ramchain.blocks,blocknum);
        }
        ledgerhash = coin777_ledgerhash(0,hp);
        for (i=0; i<coin->ramchain.num; i++)
        {
            coin->ramchain.sps[i]->state = H.states[i];
            memcpy(coin->ramchain.sps[i]->sha256,H.sha256[i],sizeof(H.sha256[i]));
            printf("%08x ",*(uint32_t *)H.sha256[i]);
        }
        printf("RESTORED.%d -> block.%u ledgerhash %08x addrsum %.8f maxaddrind.%u supply %.8f\n",synci,blocknum,(uint32_t)ledgerhash,dstr(coin->ramchain.addrsum),hp->addrind,dstr(B.credits)-dstr(B.debits));
    } else printf("ledger_getnearest error getting last\n");
    return(blocknum);// == 0 ? blocknum : blocknum - 1);
}

struct coin777_state *coin777_stateinit(struct env777 *DBs,struct coin777_state *sp,char *coinstr,char *subdir,char *name,char *compression,int32_t flags,int32_t valuesize)
{
    safecopy(sp->name,name,sizeof(sp->name));
    sp->flags = flags;
    portable_mutex_init(&sp->mutex);
    if ( DBs != 0 )
    {
        safecopy(DBs->coinstr,coinstr,sizeof(DBs->coinstr));
        safecopy(DBs->subdir,subdir,sizeof(DBs->subdir));
    }
    update_sha256(sp->sha256,&sp->state,0,0);
    sp->itemsize = valuesize;
    if ( DBs != 0 )
        sp->DB = db777_open(0,DBs,name,compression,flags,valuesize);
    return(sp);
}

#define COIN777_ADDRINFOS 0 // matches
#define COIN777_BLOCKS 1 //
#define COIN777_TXOFFSETS 2 // matches
#define COIN777_TXIDBITS 3 // matches
#define COIN777_UNSPENTS 4 // ?
#define COIN777_SPENDS 5 // matches
#define COIN777_LEDGER 6 // ?
#define COIN777_ADDRTX 7 // matches
#define COIN777_TXIDS 8 // matches
#define COIN777_ADDRS 9 // matches
#define COIN777_SCRIPTS 10 // matches

#define COIN777_HASHES 11

void coin777_initDBenv(struct coin777 *coin)
{
    char *subdir="",*coinstr = coin->name; int32_t n = 0;
    struct ramchain *ramchain = &coin->ramchain;
    if ( n == COIN777_ADDRINFOS )
        ramchain->sps[n++] = coin777_stateinit(0,&ramchain->addrinfos,coinstr,subdir,"addrinfos","zstd",0*DB777_VOLATILE,sizeof(struct coin777_addrinfo));
    if ( n == COIN777_BLOCKS )
        ramchain->sps[n++] = coin777_stateinit(0,&ramchain->blocks,coinstr,subdir,"blocks","zstd",0,sizeof(struct coin_offsets));
    if ( n == COIN777_TXOFFSETS )
        ramchain->sps[n++] = coin777_stateinit(0,&ramchain->txoffsets,coinstr,subdir,"txoffsets","zstd",0,sizeof(uint32_t) * 2);
    if ( n == COIN777_TXIDBITS )
        ramchain->sps[n++] = coin777_stateinit(0,&ramchain->txidbits,coinstr,subdir,"txidbits",0,0,sizeof(bits256));
    if ( n == COIN777_UNSPENTS )
        ramchain->sps[n++] = coin777_stateinit(0,&ramchain->unspents,coinstr,subdir,"unspents","zstd",0,sizeof(struct unspent_info));
    if ( n == COIN777_SPENDS )
        ramchain->sps[n++] = coin777_stateinit(0,&ramchain->spends,coinstr,subdir,"spends","zstd",0,sizeof(struct spend_info));
    if ( n == COIN777_LEDGER )
        ramchain->sps[n++] = coin777_stateinit(0,&ramchain->ledger,coinstr,subdir,"ledger","zstd",DB777_VOLATILE,sizeof(struct coin777_Lentry));
    if ( n == COIN777_ADDRTX )
        ramchain->sps[n++] = coin777_stateinit(0,&ramchain->addrtx,coinstr,subdir,"addrtx","zstd",0,sizeof(struct addrtx_info));
    
    if ( n == COIN777_TXIDS )
        ramchain->sps[n++] = coin777_stateinit(&ramchain->DBs,&ramchain->txidDB,coinstr,subdir,"txids",0,DB777_HDD,sizeof(uint32_t));
    if ( n == COIN777_ADDRS )
        ramchain->sps[n++] = coin777_stateinit(&ramchain->DBs,&ramchain->addrDB,coinstr,subdir,"addrs",0,DB777_HDD,sizeof(uint32_t));
    if ( n == COIN777_SCRIPTS )
        ramchain->sps[n++] = coin777_stateinit(&ramchain->DBs,&ramchain->scriptDB,coinstr,subdir,"scripts",0,DB777_HDD,sizeof(uint32_t));
    ramchain->num = n;
    if ( n == COIN777_HASHES )
        ramchain->sps[n] = coin777_stateinit(&ramchain->DBs,&ramchain->hashDB,coinstr,subdir,"hashes",0,DB777_HDD,sizeof(struct coin777_hashes));
    else printf("coin777_initDBenv mismatched COIN777_HASHES.%d vs n.%d\n",COIN777_HASHES,n), exit(-1);
    env777_start(0,&ramchain->DBs,0);
}

int32_t coin777_initmmap(struct coin777 *coin,uint32_t blocknum,uint32_t txidind,uint32_t addrind,uint32_t scriptind,uint32_t unspentind,uint32_t totalspends,uint32_t totaladdrtx)
{
    char fname[1024],srcfname[1024]; struct ramchain *ramchain = &coin->ramchain;
    db777_path(fname,coin->name,"",0), strcat(fname,"/"), strcat(fname,"addrinfos"), sprintf(srcfname,"%s.sync",fname), copy_file(srcfname,fname);
    db777_path(fname,coin->name,"",0), strcat(fname,"/"), strcat(fname,"addrtx"), sprintf(srcfname,"%s.sync",fname), copy_file(srcfname,fname);
    db777_path(fname,coin->name,"",0), strcat(fname,"/"), strcat(fname,"ledger"), sprintf(srcfname,"%s.sync",fname), copy_file(srcfname,fname);
    ramchain->blocks.table = coin777_ensure(coin,&ramchain->blocks,blocknum);
    ramchain->txoffsets.table = coin777_ensure(coin,&ramchain->txoffsets,txidind);
    ramchain->txidbits.table = coin777_ensure(coin,&ramchain->txidbits,txidind);
    ramchain->unspents.table = coin777_ensure(coin,&ramchain->unspents,unspentind);
    ramchain->addrinfos.table = coin777_ensure(coin,&ramchain->addrinfos,addrind);
    ramchain->ledger.table = coin777_ensure(coin,&ramchain->ledger,addrind);
    ramchain->spends.table = coin777_ensure(coin,&ramchain->spends,totalspends);
    ramchain->addrtx.table = coin777_ensure(coin,&ramchain->addrtx,totaladdrtx);
    return(0);
}

// coin777 block parser
int32_t coin777_getinds(void *state,uint32_t blocknum,uint64_t *creditsp,uint64_t *debitsp,uint32_t *timestampp,uint32_t *txidindp,uint32_t *unspentindp,uint32_t *numspendsp,uint32_t *addrindp,uint32_t *scriptindp,uint32_t *totaladdrtxp)
{
    struct coin777 *coin = state; struct coin_offsets block;
    if ( coin->ramchain.blocks.table == 0 ) // bootstrap requires coin_offsets DB before anything else
        coin777_stateinit(0,&coin->ramchain.blocks,coin->name,"","blocks","zstd",DB777_VOLATILE,sizeof(struct coin_offsets));
    if ( blocknum == 0 )
        *txidindp = *unspentindp = *numspendsp = *addrindp = *scriptindp = *totaladdrtxp = 1, *creditsp = *debitsp = *timestampp = 0;
    else
    {
        coin777_RWmmap(0,&block,coin,&coin->ramchain.blocks,blocknum);
        *creditsp = block.credits, *debitsp = block.debits;
        *timestampp = block.timestamp, *txidindp = block.txidind;
        *unspentindp = block.unspentind, *numspendsp = block.numspends, *addrindp = block.addrind, *scriptindp = block.scriptind, *totaladdrtxp = block.totaladdrtx;
        if ( blocknum == coin->ramchain.startblocknum )
            printf("(%.8f - %.8f) supply %.8f blocknum.%u loaded txidind.%u unspentind.%u numspends.%u addrind.%u scriptind.%u totaladdrtx %u\n",dstr(*creditsp),dstr(*debitsp),dstr(*creditsp)-dstr(*debitsp),blocknum,*txidindp,*unspentindp,*numspendsp,*addrindp,*scriptindp,*totaladdrtxp);
    }
    return(0);
}

int32_t coin777_MMbackup(char *dirname,struct coin777_state *sp,uint32_t firstind,uint32_t lastind)
{
    char fname[1024]; FILE *fp; int32_t errs = 0;
    sprintf(fname,"%s/%s",dirname,sp->name);
    if ( (fp= fopen(fname,"wb")) != 0 )
    {
        if ( firstind == 0 )
        {
            if ( fwrite(sp->M.fileptr,1,(long)lastind*sp->itemsize,fp) != lastind*sp->itemsize )
                errs++;
        }
        else
        {
            if ( fwrite(&firstind,1,sizeof(firstind),fp) != sizeof(firstind) )
                errs++;
            if ( fwrite(&lastind,1,sizeof(lastind),fp) != sizeof(lastind) )
                errs++;
            if ( fwrite((void *)((uint64_t)sp->M.fileptr + firstind*sp->itemsize),1,((long)lastind - firstind + 1)*sp->itemsize,fp) != (lastind - firstind + 1)*sp->itemsize )
                errs++;
        }
        fclose(fp);
    }
    return(-errs);
}

int32_t coin777_incrbackup(struct coin777 *coin,uint32_t blocknum,int32_t prevsynci,struct coin777_hashes *H)
{
    char fname[1024],dirname[128],destfname[1024]; int16_t scriptlen; int64_t sum; int32_t i,len,errs = 0;
    struct coin777_hashes prevH,_H; struct coin_offsets B; uint8_t script[8192]; double startmilli;
    FILE *fp;
    if ( H == 0 )
    {
        H = &_H, i = 0, len = sizeof(*H);
        if ( coin777_getDB(H,&len,coin->ramchain.DBs.transactions,coin->ramchain.hashDB.DB,&i,sizeof(i)) == 0 )
        {
            printf("cant fine latest hashes entry\n");
            return(-1);
        }
        blocknum = H->blocknum;
        printf("doing full backup to blocknum.%u\n",blocknum);
    }
    sprintf(dirname,"%s/%s",SUPERNET.BACKUPS,coin->name);
    ensure_directory(dirname);
    if ( prevsynci <= 0 || coin777_getsyncdata(&prevH,coin,prevsynci) == 0 )
    {
        coin777_genesishash(coin,&prevH);
        sprintf(dirname+strlen(dirname),"/full.%d",blocknum);
    } else sprintf(dirname+strlen(dirname),"/incr.%d_%d",prevH.blocknum,blocknum);
    ensure_directory(dirname);
    printf("start Backup.(%s)\n",dirname);
    startmilli = milliseconds();
    if ( 0 )
    {
        if ( prevH.blocknum != 0 )
        {
            sprintf(fname,"%s/blocks",dirname);
            if ( (fp= fopen(fname,"wb")) != 0 )
            {
                if ( fwrite(H,1,sizeof(*H),fp) != sizeof(*H) )
                    errs++;
                if ( fwrite(&prevH,1,sizeof(prevH),fp) != sizeof(prevH) )
                    errs++;
                for (i=prevH.blocknum; i<=H->blocknum; i++)
                {
                    coin777_RWmmap(0,&B,coin,&coin->ramchain.blocks,i);
                    if ( fwrite(&B,1,sizeof(B),fp) != sizeof(B) )
                        errs++;
                }
                fclose(fp);
            }
            if ( errs != 0 )
                printf("errs.%d after blocks\n",errs);
        }
        else errs += coin777_MMbackup(dirname,&coin->ramchain.blocks,0,H->blocknum);
        errs += coin777_MMbackup(dirname,&coin->ramchain.addrinfos,prevH.addrind,H->addrind);
        errs += coin777_MMbackup(dirname,&coin->ramchain.txoffsets,prevH.txidind,H->txidind);
        errs += coin777_MMbackup(dirname,&coin->ramchain.txidbits,prevH.txidind,H->txidind);
        errs += coin777_MMbackup(dirname,&coin->ramchain.unspents,prevH.unspentind,H->unspentind);
        errs += coin777_MMbackup(dirname,&coin->ramchain.spends,prevH.numspends,H->numspends);
        if ( errs != 0 )
            printf("errs.%d after coin777_MMbackups\n",errs);
        sprintf(fname,"%s/scripts",dirname);
        if ( (fp= fopen(fname,"wb")) != 0 )
        {
            for (i=prevH.scriptind; i<=H->scriptind; i++)
            {
                if ( coin777_getDB(script,&len,coin->ramchain.DBs.transactions,coin->ramchain.scriptDB.DB,&i,sizeof(i)) != 0 )
                {
                    scriptlen = len;
                    if ( fwrite(&scriptlen,1,sizeof(scriptlen),fp) != sizeof(scriptlen) )
                        errs++;
                    if ( fwrite(script,1,scriptlen,fp) != scriptlen )
                        errs++;
                }
                else
                {
                    scriptlen = 0;
                    if ( fwrite(&scriptlen,1,sizeof(scriptlen),fp) != sizeof(scriptlen) )
                        errs++;
                }
            }
            fclose(fp);
        }
        if ( errs != 0 )
            printf("errs.%d after scripts\n",errs);
    }
    db777_path(fname,coin->name,"",0), strcat(fname,"/ledger"), sprintf(destfname,"cp %s %s.sync",fname,fname);
    if ( system(destfname) != 0 )
        printf("error doing.(%s)\n",destfname);//copy_file(fname,destfname);
    db777_path(fname,coin->name,"",0), strcat(fname,"/addrtx"), sprintf(destfname,"cp %s %s.sync",fname,fname);
    if ( system(destfname) != 0 )
        printf("error doing.(%s)\n",destfname);//copy_file(fname,destfname);
    db777_path(fname,coin->name,"",0), strcat(fname,"/addrinfos"), sprintf(destfname,"cp %s %s.sync",fname,fname);
    if ( system(destfname) != 0 )
        printf("error doing.(%s)\n",destfname);//copy_file(fname,destfname);
    sum = addrinfos_sum(coin,H->addrind,0,H->unspentind,H->numspends,0,0);
    printf("finished Backup.(%s) supply %.8f in %.3f seconds | errs.%d\n",dirname,dstr(sum),(milliseconds() - startmilli)/1000.,errs);
    return(-errs);
}

int32_t coin777_replayblock(struct coin777_hashes *hp,struct coin777 *coin,uint32_t blocknum,int32_t synci,int32_t verifyflag)
{
    struct coin_offsets B,nextB; struct unspent_info U; struct coin777_addrinfo A; struct spend_info S;
    uint32_t txidind,spendind,unspentind,txoffsets[2],nexttxoffsets[2]; bits256 txid;
    char scriptstr[8192],txidstr[65]; uint8_t *scriptptr,script[8193]; int32_t i,scriptlen,allocsize = 0,errs = 0;
    if ( coin777_RWmmap(0,&B,coin,&coin->ramchain.blocks,blocknum) == 0 && coin777_RWmmap(0,&nextB,coin,&coin->ramchain.blocks,blocknum+1) == 0 )
    {
        for (i=0; i<coin->ramchain.num; i++)
            printf("%02x.%02x ",hp->sha256[i][0],hp->sha256[i][0] ^ B.check[i]);
        if ( B.txidind != hp->txidind || B.txidind != hp->txidind || B.txidind != hp->txidind || B.txidind != hp->txidind || B.txidind != hp->txidind || B.txidind != hp->txidind || B.credits != hp->credits || B.debits != hp->debits )
        {
            printf("coin777_replayblock.%d: ind mismatch (%u %u %u %u %u) vs (%u %u %u %u %u) || %.8f %.8f vs %.8f %.8f\n",blocknum,B.txidind,B.addrind,B.scriptind,B.unspentind,B.numspends,hp->txidind,hp->addrind,hp->scriptind,hp->unspentind,hp->numspends,dstr(B.credits),dstr(B.debits),dstr(hp->credits),dstr(hp->debits));
        }
        if ( blocknum != 0 )
            update_blocksha256(hp->sha256[COIN777_BLOCKS],&hp->states[COIN777_BLOCKS],&B);
        allocsize += sizeof(B);
        for (txidind=B.txidind; txidind<nextB.txidind; txidind++,hp->txidind++)
        {
            if ( coin777_RWmmap(0,&txid,coin,&coin->ramchain.txidbits,txidind) != 0 )
                errs++, printf("error getting txid.%u\n",txidind);
            else if ( coin777_RWmmap(0,txoffsets,coin,&coin->ramchain.txoffsets,txidind) == 0 && coin777_RWmmap(0,nexttxoffsets,coin,&coin->ramchain.txoffsets,txidind+1) == 0 )
            {
                init_hexbytes_noT(txidstr,txid.bytes,sizeof(txid));
                update_sha256(hp->sha256[COIN777_TXIDS],&hp->states[COIN777_TXIDS],(uint8_t *)txidstr,(int32_t)sizeof(txid)*2);
                update_sha256(hp->sha256[COIN777_TXIDBITS],&hp->states[COIN777_TXIDBITS],txid.bytes,sizeof(txid));
                update_sha256(hp->sha256[COIN777_TXOFFSETS],&hp->states[COIN777_TXOFFSETS],(uint8_t *)txoffsets,sizeof(txoffsets));
                allocsize += sizeof(txid)*2 + sizeof(txoffsets);
                for (unspentind=txoffsets[0]; unspentind<nexttxoffsets[0]; unspentind++,hp->unspentind++)
                {
                    if ( coin777_RWmmap(0,&U,coin,&coin->ramchain.unspents,unspentind) == 0 && coin777_RWmmap(0,&A,coin,&coin->ramchain.addrinfos,U.addrind) == 0 )
                    {
                        allocsize += sizeof(U);
                        hp->credits += U.value;
                        if ( (scriptptr= coin777_scriptptr(&A)) != 0 )
                            init_hexbytes_noT(scriptstr,scriptptr,A.scriptlen);
                        else if ( U.isblocknum == 0 )
                        {
                            coin777_scriptstr(coin,scriptstr,sizeof(scriptstr),U.rawind_or_blocknum,U.addrind);
                            if ( U.rawind_or_blocknum == hp->scriptind )
                                hp->scriptind++;
                            //printf("got long script.(%s) addrind.%u scriptind.%u\n",scriptstr,U.addrind,U.scriptind_or_blocknum);
                        } else errs++, printf("replayblock illegal case of no scriptptr blocknum.%u unspentind.%u\n",blocknum,unspentind);
                        scriptlen = ((int32_t)strlen(scriptstr) >> 1);
                        decode_hex(script,scriptlen,scriptstr);
                        update_sha256(hp->sha256[COIN777_SCRIPTS],&hp->states[COIN777_SCRIPTS],(uint8_t *)scriptstr,scriptlen<<1);
                        if ( U.addrind == hp->addrind && A.firstblocknum == blocknum )
                        {
                            hp->addrind++;
                            allocsize += sizeof(A);
                            update_sha256(hp->sha256[COIN777_ADDRS],&hp->states[COIN777_ADDRS],(uint8_t *)A.coinaddr,A.addrlen);
                            update_addrinfosha256(hp->sha256[COIN777_ADDRINFOS],&hp->states[COIN777_ADDRINFOS],blocknum,A.coinaddr,A.addrlen,script,scriptlen);
                        }
                        update_sha256(hp->sha256[COIN777_UNSPENTS],&hp->states[COIN777_UNSPENTS],(uint8_t *)&U,sizeof(U));
                        update_ledgersha256(hp->sha256[COIN777_LEDGER],&hp->states[COIN777_LEDGER],U.value,U.addrind,blocknum);
                        update_ledgersha256(hp->sha256[COIN777_ADDRTX],&hp->states[COIN777_ADDRTX],U.value,unspentind,blocknum);
                    } else errs++, printf("error getting unspendid.%u\n",unspentind);
                }
                for (spendind=txoffsets[1]; spendind<nexttxoffsets[1]; spendind++,hp->numspends++)
                {
                    if ( coin777_RWmmap(0,&S,coin,&coin->ramchain.spends,spendind) == 0 )
                    {
                        allocsize += sizeof(S);
                        update_sha256(hp->sha256[COIN777_SPENDS],&hp->states[COIN777_SPENDS],(uint8_t *)&S,sizeof(S));
                        if ( coin777_RWmmap(0,&U,coin,&coin->ramchain.unspents,S.unspentind) == 0 )
                        {
                            hp->debits += U.value;
                            update_ledgersha256(hp->sha256[COIN777_LEDGER],&hp->states[COIN777_LEDGER],-U.value,U.addrind,blocknum);
                            update_ledgersha256(hp->sha256[COIN777_ADDRTX],&hp->states[COIN777_ADDRTX],-U.value,spendind,blocknum);
                            printf("-(u%d %.8f) ",unspentind,dstr(U.value));
                        }
                        else errs++, printf("couldnt find spend ind.%u\n",unspentind);
                    }
                    else errs++, printf("error getting spendind.%u\n",spendind);
                }
                printf("numvins.%d | ",nexttxoffsets[1] - txoffsets[1]);
            }
        }
        printf("blocknum.%u supply %.8f numtx.%d allocsize.%d\n",blocknum,dstr(B.credits) - dstr(B.debits),nextB.txidind - B.txidind,allocsize);
    } else printf("Error loading blockpair %d\n",blocknum);
    hp->timestamp = B.timestamp, hp->numsyncs = synci;
    hp->ledgerhash = coin777_ledgerhash(0,hp);
    return(-errs);
}

int32_t coin777_replayblocks(struct coin777 *coin,uint32_t startblocknum,uint32_t endblocknum,int32_t verifyflag)
{
    struct coin777_hashes H,endH; uint32_t blocknum; int32_t startsynci,endsynci,errs = 0;
    if ( (blocknum= coin777_hashes(&startsynci,&H,coin,startblocknum,100000)) != startblocknum )
        errs = -1, printf("cant find hashes for startblocknum.%u closest is %u\n",startblocknum,blocknum);
    else if ( (blocknum= coin777_hashes(&endsynci,&endH,coin,endblocknum,100000)) != startblocknum )
        errs = -2, printf("cant find hashes for endblocknum.%u closest is %u\n",endblocknum,blocknum);
    else
    {
        for (blocknum=startblocknum; blocknum<endblocknum; blocknum++)
            if ( coin777_replayblock(&H,coin,blocknum,endsynci,verifyflag) != 0 )
            {
                printf("coin777_replayblocks error on blocknum.%u\n",blocknum);
                return(-3);
            }
    }
    return(0);
}

int32_t coin777_verify(struct coin777 *coin,uint32_t maxunspentind,uint32_t totalspends,uint64_t credits,uint64_t debits,uint32_t addrind,int32_t forceflag,uint32_t *totaladdrtxp)
{
    struct coin777_Lentry L; struct unspent_info U; struct spend_info S; struct addrtx_info ATX; double startmilli;
    int32_t errs = 0; uint32_t unspentind,spendind; uint64_t Ucredits,Udebits; int64_t correction = 0;
    if ( maxunspentind > 1 )
    {
        coin->ramchain.addrsum = addrinfos_sum(coin,addrind,0,maxunspentind,totalspends,forceflag,totaladdrtxp);
        if ( forceflag != 0 || coin->ramchain.addrsum != (credits - debits) )
        {
            if ( RAMCHAINS.fastmode == 0 || coin->ramchain.addrsum != (credits - debits) )
            {
                startmilli = milliseconds();
                fprintf(stderr,"Verify unspents: ");
                Ucredits = Udebits = 0;
                for (unspentind=1; unspentind<maxunspentind; unspentind++)
                {
                    if ( (unspentind % 1000000) == 0 )
                        fprintf(stderr,".");
                    Ucredits += coin777_Uvalue(&U,coin,unspentind);
                    if ( U.value != 0 )
                    {
                        coin777_RWmmap(0,&L,coin,&coin->ramchain.ledger,U.addrind);
                        if ( coin777_bsearch(&ATX,coin,U.addrind,&L,unspentind,U.value) < 0  )
                        {
                            correction += U.value;
                            printf("U cant find addrind.%u U.%u %.8f | correction %.8f\n",U.addrind,unspentind,dstr(U.value),dstr(correction));
                        }
                    }
                }
                fprintf(stderr,"\nVerify spends: ");
                for (spendind=1; spendind<totalspends; spendind++)
                {
                    if ( (spendind % 1000000) == 0 )
                        fprintf(stderr,".");
                    Udebits += coin777_Svalue(&S,coin,spendind);
                    coin777_Uvalue(&U,coin,S.unspentind);
                    if ( U.value != 0 )
                    {
                        coin777_RWmmap(0,&L,coin,&coin->ramchain.ledger,U.addrind);
                        if ( coin777_bsearch(&ATX,coin,U.addrind,&L,S.unspentind,U.value) < 0 || ATX.spendind != spendind )
                        {
                            correction -= U.value;
                            printf("S cant find addrind.%u U.%u %.8f || spendind mismatch %u vs %u | correction %.8f\n",U.addrind,S.unspentind,dstr(U.value),ATX.spendind,spendind,dstr(correction));
                        }
                    }
                }
                printf("\nVERIFY maxunspentind.%u Usum %.8f (%.8f - %.8f) correction %.8f | elapsed %.3f seconds\n",maxunspentind,dstr(Ucredits)-dstr(Udebits),dstr(Ucredits),dstr(Udebits),dstr(correction),(milliseconds() - startmilli)/1000.);
            }
            if ( coin->ramchain.addrsum != (credits - debits) )
                printf("addrinfos_sum %.8f != supply %.8f (%.8f - %.8f) -> recalc\n",dstr(coin->ramchain.addrsum),dstr(credits)-dstr(debits),dstr(credits),dstr(debits));
            if ( forceflag == 0 )
                coin->ramchain.addrsum = addrinfos_sum(coin,addrind,0,maxunspentind,totalspends,1,totaladdrtxp);
            if ( forceflag != 0 || coin->ramchain.addrsum != (credits - debits) )
            {
                if ( coin->ramchain.addrsum != (credits - debits) )
                    errs++, printf("ERROR recalc did not fix discrepancy %.8f != supply %.8f (%.8f - %.8f) -> Lchain recovery maxunspentind.%u\n",dstr(coin->ramchain.addrsum),dstr(credits)-dstr(debits),dstr(credits),dstr(debits),maxunspentind);
                //errs = coin777_replayblocks(coin,coin777_latestledger(coin),blocknum,1);
            } else printf("recalc resolved discrepancies: supply %.8f addrsum %.8f\n",dstr(credits)-dstr(debits),dstr(coin->ramchain.addrsum));
        }
    }
    return(-errs);
}

uint64_t coin777_flush(struct coin777 *coin,uint32_t blocknum,int32_t numsyncs,uint64_t credits,uint64_t debits,uint32_t timestamp,uint32_t txidind,uint32_t numrawvouts,uint32_t numrawvins,uint32_t addrind,uint32_t scriptind,uint32_t *totaladdrtxp)
{
    int32_t i,retval = 0; struct coin777_hashes H;
    if ( numsyncs > 0 )
    {
        if ( coin777_verify(coin,numrawvouts,numrawvins,credits,debits,addrind,1,totaladdrtxp) != 0 )
            printf("cant verify at block.%u\n",blocknum), debugstop();
    }
    else if ( (coin->ramchain.RTblocknum - blocknum) < coin->minconfirms*2 )
        coin->ramchain.addrsum = addrinfos_sum(coin,addrind,0,numrawvouts,numrawvins,1,totaladdrtxp);
    memset(&H,0,sizeof(H)); H.blocknum = blocknum, H.numsyncs = numsyncs, H.credits = credits, H.debits = debits;
    H.timestamp = timestamp, H.txidind = txidind, H.unspentind = numrawvouts, H.numspends = numrawvins, H.addrind = addrind, H.scriptind = scriptind, H.totaladdrtx = *totaladdrtxp;
    if ( numsyncs >= 0 )
        coin->ramchain.addrsum = addrinfos_sum(coin,addrind,1,numrawvouts,numrawvins,0,totaladdrtxp);
    for (i=0; i<=coin->ramchain.num; i++)
    {
        if ( numsyncs >= 0 )
        {
            if ( coin->ramchain.sps[i]->M.fileptr != 0 )
                sync_mappedptr(&coin->ramchain.sps[i]->M,0);
            if ( coin->ramchain.sps[i]->fp != 0 )
                fflush(coin->ramchain.sps[i]->fp);
        }
        if ( i < coin->ramchain.num )
        {
            H.states[i] = coin->ramchain.sps[i]->state;
            memcpy(H.sha256[i],coin->ramchain.sps[i]->sha256,sizeof(H.sha256[i]));
        }
    }
    H.ledgerhash = coin777_ledgerhash(0,&H);
    if ( numsyncs < 0 )
    {
        for (i=0; i<coin->ramchain.num; i++)
            printf("%08x ",*(int *)H.sha256[i]);
    }
    if ( numsyncs >= 0 )
    {
        printf("SYNCNUM.%d (%ld %ld %ld %ld %ld) -> %d addrsum %.8f addrind.%u supply %.8f | txids.%u addrs.%u scripts.%u unspents.%u spends.%u totaladdrtx %u ledgerhash %08x\n",numsyncs,sizeof(struct unspent_info),sizeof(struct spend_info),sizeof(struct hashed_uint32),sizeof(struct coin777_Lentry),sizeof(struct addrtx_info),blocknum,dstr(coin->ramchain.addrsum),addrind,dstr(credits)-dstr(debits),coin->ramchain.latest.txidind,coin->ramchain.latest.addrind,coin->ramchain.latest.scriptind,coin->ramchain.latest.unspentind,coin->ramchain.latest.numspends,coin->ramchain.latest.totaladdrtx,(uint32_t)H.ledgerhash);
        if ( coin777_addDB(coin,coin->ramchain.DBs.transactions,coin->ramchain.hashDB.DB,&numsyncs,sizeof(numsyncs),&H,sizeof(H)) != 0 )
            printf("error saving numsyncs.0 retval.%d %s\n",retval,db777_errstr(coin->ramchain.DBs.ctl)), sleep(30);
        if ( numsyncs > 0 )
        {
            coin777_incrbackup(coin,blocknum,numsyncs-1,&H);
            numsyncs = 0;
            if ( (retval = coin777_addDB(coin,coin->ramchain.DBs.transactions,coin->ramchain.hashDB.DB,&numsyncs,sizeof(numsyncs),&H,sizeof(H))) != 0 )
                printf("error saving numsyncs.0 retval.%d %s\n",retval,db777_errstr(coin->ramchain.DBs.ctl)), sleep(30);
        }
    }
    return(H.ledgerhash);
}

int32_t coin777_parse(struct coin777 *coin,uint32_t RTblocknum,int32_t syncflag,int32_t minconfirms)
{
    uint32_t blocknum,dispflag,ledgerhash=0,allocsize,timestamp,txidind,numrawvouts,numrawvins,addrind,scriptind,totaladdrtx; int32_t numtx,err;
    uint64_t origsize,supply,oldsupply,credits,debits; double estimate,elapsed,startmilli;
    blocknum = coin->ramchain.blocknum;
    if ( blocknum <= (RTblocknum - minconfirms) )
    {
        startmilli = milliseconds();
        dispflag = 1 || (blocknum > RTblocknum - 1000);
        dispflag += ((blocknum % 100) == 0);
        if ( coin777_getinds(coin,blocknum,&credits,&debits,&timestamp,&txidind,&numrawvouts,&numrawvins,&addrind,&scriptind,&totaladdrtx) == 0 )
        {
            if ( coin->ramchain.DBs.transactions == 0 )
                coin->ramchain.DBs.transactions = 0;//sp_begin(coin->DBs.env);
            supply = (credits - debits), origsize = coin->ramchain.totalsize;
            oldsupply = supply;
            if ( syncflag != 0 && blocknum != coin->ramchain.startblocknum )
                ledgerhash = (uint32_t)coin777_flush(coin,blocknum,++coin->ramchain.numsyncs,credits,debits,timestamp,txidind,numrawvouts,numrawvins,addrind,scriptind,&totaladdrtx);
            else ledgerhash = (uint32_t)coin777_flush(coin,blocknum,-1,credits,debits,timestamp,txidind,numrawvouts,numrawvins,addrind,scriptind,&totaladdrtx);
            numtx = parse_block(coin,&credits,&debits,&txidind,&numrawvouts,&numrawvins,&addrind,&scriptind,&totaladdrtx,coin->name,coin->serverport,coin->userpass,blocknum,coin777_addblock,coin777_addvin,coin777_addvout,coin777_addtx);
            if ( coin->ramchain.DBs.transactions != 0 )
            {
                while ( (err= sp_commit(coin->ramchain.DBs.transactions)) != 0 )
                {
                    printf("ledger_commit: sp_commit error.%d\n",err);
                    if ( err < 0 )
                        break;
                    msleep(1000);
                }
                coin->ramchain.DBs.transactions = 0;
            }
            supply = (credits - debits);
            dxblend(&coin->ramchain.calc_elapsed,(milliseconds() - startmilli),.99);
            allocsize = (uint32_t)(coin->ramchain.totalsize - origsize);
            estimate = estimate_completion(coin->ramchain.startmilli,blocknum - coin->ramchain.startblocknum,RTblocknum-blocknum)/60000;
            elapsed = (milliseconds() - coin->ramchain.startmilli)/60000.;
            if ( dispflag != 0 )
            {
                extern int32_t Duplicate,Mismatch,Added,Numgets;
                coin->lag = RTblocknum - blocknum;
                printf("%.3f %-5s [lag %-5d] %-6u %.8f %.8f (%.8f) [%.8f] %13.8f | dur %.2f %.2f %.2f | len.%-5d %s %.1f | H%d E%d R%d W%d %08x\n",coin->ramchain.calc_elapsed/1000.,coin->name,coin->lag,blocknum,dstr(oldsupply),dstr(coin->ramchain.addrsum),dstr(oldsupply)-dstr(coin->ramchain.addrsum),dstr(supply)-dstr(oldsupply),dstr(coin->ramchain.minted != 0 ? coin->ramchain.minted : (supply - oldsupply)),elapsed,elapsed+(RTblocknum-blocknum)*coin->ramchain.calc_elapsed/60000,elapsed+estimate,allocsize,_mbstr(coin->ramchain.totalsize),(double)coin->ramchain.totalsize/blocknum,Duplicate,Mismatch,Numgets,Added,ledgerhash);
            }
            coin->ramchain.blocknum++;
            return(1);
        }
        else
        {
            printf("coin777 error getting inds for blocknum%u\n",blocknum);
            return(0);
        }
    } else printf("blocknum.%d > RTblocknum.%d - minconfirms.%d\n",blocknum,RTblocknum,minconfirms);
    return(0);
}
#endif

#endif
#endif
