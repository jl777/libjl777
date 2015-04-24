//
//  ramchain.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_ramchain_h
#define crypto777_ramchain_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "uthash.h"
#include "cJSON.h"
#include "huffstream.c"
#include "utils777.c"
#include "system777.c"
#include "files777.c"
#include "bitcoind.c"
#include "state.c"
#include "huff.c"
#include "storage.c"

#define MIN_DEPOSIT_FACTOR 5
#define WITHRAW_ENABLE_BLOCKS 3
#define TMPALLOC_SPACE_INCR 10000000
#define PERMALLOC_SPACE_INCR (1024 * 1024 * 128)
#define MAX_PENDINGSENDS_TICKS 50
#define MAX_TRANSFER_SIZE (65536 * 16)
#define TRANSFER_BLOCKSIZE 512
#define MAX_TRANSFER_BLOCKS (MAX_TRANSFER_SIZE / TRANSFER_BLOCKSIZE)
#define NUM_GATEWAYS 3

struct transfer_args
{
    uint64_t modified;
    uint32_t timestamps[MAX_TRANSFER_BLOCKS],crcs[MAX_TRANSFER_BLOCKS],gotcrcs[MAX_TRANSFER_BLOCKS],slots[MAX_TRANSFER_BLOCKS];
    char previpaddr[64],sender[64],dest[64],name[512],hashstr[65],handler[64],pstr[MAX_TRANSFER_BLOCKS];
    uint8_t data[MAX_TRANSFER_SIZE],snapshot[MAX_TRANSFER_SIZE];
    uint32_t totalcrc,currentcrc,snapshotcrc,handlercrc,syncmem,totallen,blocksize,numfrags,completed,timeout,handlertime;
};

extern int MAP_HUFF,MGW_initdone,PERMUTE_RAWINDS,Debuglevel,MAP_HUFF,Finished_init,DBSLEEP,MAX_BUYNXT,MIN_NQTFEE,Gatewayid;
extern char Server_ipaddrs[256][MAX_JSON_FIELD],MGWROOT[256],*MGW_whitelist[256],NXTAPIURL[MAX_JSON_FIELD],DATADIR[512];
extern int Numramchains; extern struct ramchain_info *Ramchains[100];
extern cJSON *MGWconf;
//extern void *Global_mp;


/*extern struct NXT_acct *NXT_accts;

struct SuperNET_db
{
    char name[64],**privkeys;
    queue_t queue;
    long maxitems,total_stored;
    DB *dbp;
    DB_ENV *storage;
    //struct hashtable *ramtable;
    int32_t *cipherids,selector,active;
    uint32_t busy,type,flags,minsize,maxsize,duplicateflag,overlap_write;
};

struct dbreq { struct queueitem DL; struct SuperNET_db *sdb; void *cursor; DB_TXN *txn; DBT key,*data; int32_t flags,retval,funcid,doneflag; };*/

struct address_entry { uint64_t blocknum:32,txind:15,vinflag:1,v:14,spent:1,isinternal:1; };

struct rampayload { struct address_entry B,spentB; uint64_t value; uint32_t otherind:31,extra:31,pendingdeposit:1,pendingsend:1; };
struct ramchain_hashptr { int64_t unspent; UT_hash_handle hh; struct rampayload *payloads; uint32_t rawind,permind,numpayloads:29,maxpayloads:29,mine:1,multisig:1,verified:1,nonstandard:1,tbd:2; int32_t numunspent; };
struct ramchain_hashtable
{
    char coinstr[16];
    struct ramchain_hashptr *table;
    struct mappedptr M;
    FILE *newfp,*permfp;
    struct ramchain_hashptr **ptrs;
    long endpermpos;
    uint32_t ind,numalloc;
    uint8_t type;
};

struct mappedblocks
{
    struct ramchain_info *ram;
    struct mappedblocks *prevblocks;
    struct rawblock *R,*R2,*R3;
    HUFF **hps,*tmphp;
    struct mappedptr *M;
    double sum;
    uint32_t blocknum,count,firstblock,numblocks,processed,format,shift,contiguous;
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

struct ramsnapshot { bits256 hash; long permoffset,addroffset,txidoffset,scriptoffset; uint32_t addrind,txidind,scriptind; };

struct syncstate
{
    bits256 majority,minority;
    uint64_t requested[16];
    struct ramsnapshot snaps[16];
    struct syncstate *substate;
    uint32_t blocknum,allocsize;
    uint16_t format,pending,majoritybits,minoritybits;
};

struct ramchain_info
{
    struct mappedblocks blocks,Vblocks,Bblocks,blocks64,blocks4096,*mappedblocks[8];
    struct ramchain_hashtable addrhash,txidhash,scripthash;
    struct MGWstate S,otherS[3],remotesrcs[16];
    double startmilli;
    HUFF *tmphp,*tmphp2,*tmphp3;
    FILE *permfp;
    char name[64],permfname[512],dirpath[512],myipaddr[64],srvNXTACCTSECRET[2048],srvNXTADDR[64],*userpass,*serverport,*marker,*marker2,*opreturnmarker;
    uint32_t next_txid_permind,next_addr_permind,next_script_permind,permind_changes,withdrawconfirms,DEPOSIT_XFER_DURATION;
    uint32_t lastheighttime,min_confirms,estblocktime,firstiter,maxblock,nonzblocks,marker_rawind,marker2_rawind,lastdisp,maxind,numgateways,nummsigs,oldtx;
    uint64_t totalbits,totalbytes,txfee,dust,NXTfee_equiv,minoutput;
    struct rawblock *R,*R2,*R3;
    struct syncstate *verified;
    //struct rawblock_huffs H;
    struct alloc_space Tmp,Perm;
    uint64_t minval,maxval,minval2,maxval2,minval4,maxval4,minval8,maxval8;
    struct ramsnapshot *snapshots; bits256 *permhash4096;
    struct NXT_asset *ap;
    int32_t sock;
    uint64_t MGWbits,*limboarray;
    struct cointx_input *MGWunspents;
    uint32_t min_NXTconfirms,NXTtimestamp,MGWnumunspents,MGWmaxunspents,numspecials,depositconfirms,firsttime,firstblock,numpendingsends,pendingticks,remotemode,use_addmultisig;
    char multisigchar,**special_NXTaddrs,*MGWredemption,*backups,MGWsmallest[256],MGWsmallestB[256],MGWpingstr[1024],mgwstrs[3][8192];
    struct NXT_assettxid *pendingsends[512];
    float lastgetinfo,NXTconvrate;
};

union ramtypes { double dval; uint64_t val64; float fval; uint32_t val32; uint16_t val16; uint8_t val8,hashdata[8]; };
struct ramchain_token
{
    uint32_t numbits:15,ishuffcode:1,offset:16;
    uint32_t rawind;
    char selector,type;
    union ramtypes U;
};

cJSON *_get_blocktxarray(uint32_t *blockidp,int32_t *numtxp,struct ramchain_info *ram,cJSON *blockjson);
uint32_t _get_blockinfo(struct rawblock *raw,struct ramchain_info *ram,uint32_t blocknum);
HUFF *ram_conv_permind(HUFF *permhp,struct ramchain_info *ram,HUFF *hp,uint32_t checkblocknum);
int32_t ram_expand_bitstream(cJSON **jsonp,struct rawblock *raw,struct ramchain_info *ram,HUFF *hp);
//int32_t _verify_coinaddress(char *account,int32_t *ismultisigp,int32_t *isminep,struct ramchain_info *ram,char *coinaddr);
char *ram_searchpermind(char *permstr,struct ramchain_info *ram,char type,uint32_t permind);
void ram_setdispstr(char *buf,struct ramchain_info *ram,double startmilli);
HUFF **ram_get_hpptr(struct mappedblocks *blocks,uint32_t blocknum);
HUFF *ram_makehp(HUFF *tmphp,int32_t format,struct ramchain_info *ram,struct rawblock *tmp,int32_t blocknum);
int32_t ram_compress(HUFF *hp,struct ramchain_info *ram,uint8_t *data,int32_t datalen);
char *ram_blockstr(struct rawblock *tmp,struct ramchain_info *ram,struct rawblock *raw);
uint8_t *ram_encode_hashstr(int32_t *datalenp,uint8_t *data,char type,char *hashstr);
struct ramchain_hashptr *ram_hashdata_search(char *coinstr,struct alloc_space *mem,int32_t createflag,struct ramchain_hashtable *hash,uint8_t *hashdata,int32_t datalen);
uint32_t ram_create_block(int32_t verifyflag,struct ramchain_info *ram,struct mappedblocks *blocks,struct mappedblocks *prevblocks,uint32_t blocknum);
uint32_t _get_RTheight(struct ramchain_info *ram);
int32_t ram_addhash(struct ramchain_hashtable *hash,struct ramchain_hashptr *hp,void *ptr,int32_t datalen);
void ram_sethashtype(char *str,int32_t type);
uint32_t ram_process_blocks(struct ramchain_info *ram,struct mappedblocks *blocks,struct mappedblocks *prev,double timebudget);
void ram_sethashname(char fname[1024],struct ramchain_hashtable *hash,int32_t newflag);
int32_t ram_verify(struct ramchain_info *ram,HUFF *hp,int32_t format);
void ram_disp_status(struct ramchain_info *ram);
HUFF *hload(struct ramchain_info *ram,long *offsetp,FILE *fp,char *fname);
uint64_t ram_check_redeemcointx(int32_t *unspendablep,char *,char *script,uint32_t blocknum);
uint64_t ram_calc_unspent(uint64_t *pendingp,int32_t *calc_numunspentp,struct ramchain_hashptr **addrptrp,struct ramchain_info *ram,char *addr,int32_t MGWflag);
//char *_sign_and_sendmoney(char *cointxid,struct ramchain_info *ram,struct cointx_info *cointx,char *othersignedtx,uint64_t *redeems,uint64_t *amounts,int32_t numredeems);
struct cointx_info *_calc_cointx_withdraw(struct ramchain_info *ram,char *destaddr,uint64_t value,uint64_t redeemtxid);
struct ramchain_info *get_ramchain_info(char *coinstr);
uint64_t _calc_circulation(int32_t minconfirms,struct NXT_asset *ap,struct ramchain_info *ram);
int32_t ram_MGW_ready(struct ramchain_info *ram,uint32_t blocknum,uint32_t NXTheight,uint64_t nxt64bits,uint64_t amount);
int32_t ram_rawblock_emit(HUFF *hp,struct ramchain_info *ram,struct rawblock *raw);
int32_t ram_expand_scriptdata(char *scriptstr,uint8_t *scriptdata,int32_t datalen);
int32_t ram_calc_scriptmode(int32_t *datalenp,uint8_t scriptdata[4096],char *script,int32_t trimflag);
uint32_t ram_conv_hashstr(uint32_t *permindp,int32_t createflag,struct ramchain_info *ram,char *hashstr,char type);
void *ram_gethashdata(struct ramchain_info *ram,char type,uint32_t rawind);
struct rampayload *ram_getpayloadi(struct ramchain_hashptr **ptrp,struct ramchain_info *ram,char type,uint32_t rawind,uint32_t i);
struct ramchain_hashtable *ram_gethash(struct ramchain_info *ram,char type);
struct rampayload *ram_payloads(struct ramchain_hashptr **ptrp,int32_t *numpayloadsp,struct ramchain_info *ram,char *hashstr,char type);
void ram_addunspent(struct ramchain_info *ram,char *coinaddr,struct rampayload *txpayload,struct ramchain_hashptr *addrptr,struct rampayload *addrpayload,uint32_t addr_rawind,uint32_t ind);
int32_t ram_markspent(struct ramchain_info *ram,struct rampayload *txpayload,struct address_entry *spendbp,uint32_t txid_rawind);
struct ramchain_hashptr *ram_hashsearch(char *coinstr,struct alloc_space *mem,int32_t createflag,struct ramchain_hashtable *hash,char *hashstr,char type);
void ram_set_MGWdispbuf(char *dispbuf,struct ramchain_info *ram,int32_t selector);
void ram_get_MGWpingstr(struct ramchain_info *ram,char *MGWpingstr,int32_t selector);
uint32_t ram_setcontiguous(struct mappedblocks *blocks);
int32_t _is_limbo_redeem(struct ramchain_info *ram,uint64_t redeemtxidbits);
int32_t _valid_txamount(struct ramchain_info *ram,uint64_t value,char *coinaddr);
void _complete_assettxid(struct ramchain_info *ram,struct NXT_assettxid *tp);
void ram_init_ramchain(struct ramchain_info *ram);
uint32_t _process_NXTtransaction(int32_t confirmed,struct ramchain_info *ram,cJSON *txobj);
uint64_t ram_calc_MGWunspent(uint64_t *pendingp,struct ramchain_info *ram);
void ram_set_MGWpingstr(char *pingstr,struct ramchain_info *ram,int32_t selector);
int32_t _sign_rawtransaction(char *deststr,unsigned long destsize,char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx,char *rawbytes,char **privkeys);
char *_sign_localtx(char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx,char *rawbytes);
char *_createrawtxid_json_params(char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx);


#endif
#else
#ifndef crypto777_ramchain_c
#define crypto777_ramchain_c

#ifndef crypto777_ramchain_h
#define DEFINES_ONLY
#include __BASE_FILE__
#undef DEFINES_ONLY
#endif

int32_t ram_decode_huffcode(union ramtypes *destU,struct ramchain_info *ram,int32_t selector,int32_t offset)
{
    int32_t numbits;
    destU->val64 = 0;
    return(numbits);
}

int32_t ram_huffencode(uint64_t *outbitsp,struct ramchain_info *ram,struct ramchain_token *token,void *srcptr,int32_t numbits)
{
    int32_t outbits;
    *outbitsp = 0;
    return(outbits);
}

uint32_t _get_RTheight(struct ramchain_info *ram)
{
    char *retstr;
    cJSON *json;
    uint32_t height = 0;
    if ( milliseconds() > ram->lastgetinfo+10000 )
    {
        //printf("RTheight.(%s) (%s)\n",ram->name,ram->serverport);
        retstr = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"getinfo","");
        if ( retstr != 0 )
        {
            if ( (json= cJSON_Parse(retstr)) != 0 )
            {
                height = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"blocks"),0);
                free_json(json);
                ram->lastgetinfo = milliseconds();
            }
            free(retstr);
        }
    } else height = ram->S.RTblocknum;
    return(height);
}

void set_NXTpubkey(char *NXTpubkey,char *NXTacct)
{
    static uint8_t zerokey[256>>3];
    struct nodestats *stats;
    bits256 pubkey;
    if ( NXTpubkey != 0 )
        NXTpubkey[0] = 0;
    if ( NXTacct == 0 || NXTacct[0] == 0 )
        return;
    stats = get_nodestats(calc_nxt64bits(NXTacct));
    if ( memcmp(stats->pubkey,zerokey,sizeof(stats->pubkey)) == 0 )
    {
        pubkey = issue_getpubkey(0,NXTacct);
        if ( memcmp(&pubkey,zerokey,sizeof(stats->pubkey)) != 0 )
            memcpy(stats->pubkey,&pubkey,sizeof(stats->pubkey));
    } else memcpy(&pubkey,stats->pubkey,sizeof(pubkey));
    if ( NXTpubkey != 0 )
    {
        int32_t init_hexbytes_noT(char *hexbytes,unsigned char *message,long len);
        init_hexbytes_noT(NXTpubkey,pubkey.bytes,sizeof(pubkey));
    }
}

int32_t ram_expand_scriptdata(char *scriptstr,uint8_t *scriptdata,int32_t datalen)
{
    char *prefix,*suffix;
    int32_t mode,n = 0;
    scriptstr[0] = 0;
    switch ( (mode= scriptdata[n++]) )
    {
        case 'z': prefix = "ffff", suffix = ""; break;
        case 'n': prefix = "nonstandard", suffix = ""; break;
        case 's': prefix = "76a914", suffix = "88ac"; break;
        case 'm': prefix = "a9", suffix = "ac"; break;
        case 'r': prefix = "", suffix = "ac"; break;
        case ' ': prefix = "", suffix = ""; break;
        default: printf("unexpected scriptmode.(%d) (%c)\n",mode,mode); prefix = "", suffix = ""; return(-1); break;
    }
    strcpy(scriptstr,prefix);
    init_hexbytes_noT(scriptstr+strlen(scriptstr),scriptdata+n,datalen-n);
    if ( suffix[0] != 0 )
    {
        //printf("mode.(%c) suffix.(%s) [%s]\n",mode,suffix,scriptstr);
        strcat(scriptstr,suffix);
    }
    return(mode);
}

uint64_t ram_check_redeemcointx(int32_t *unspendablep,char *coinstr,char *script,uint32_t blocknum)
{
    uint64_t redeemtxid = 0;
    int32_t i;
    *unspendablep = 0;
    if ( strcmp(script,"76a914000000000000000000000000000000000000000088ac") == 0 )
        *unspendablep = 1;
    if ( strcmp(script+22,"00000000000000000000000088ac") == 0 )
    {
        for (redeemtxid=i=0; i<(int32_t)sizeof(uint64_t); i++)
        {
            redeemtxid <<= 8;
            redeemtxid |= (_decode_hex(&script[6 + 14 - i*2]) & 0xff);
        }
        printf("%s >>>>>>>>>>>>>>> found MGW redeem @blocknum.%u %s -> %llu | unspendable.%d\n",coinstr,blocknum,script,(long long)redeemtxid,*unspendablep);
    }
    else if ( *unspendablep != 0 )
        printf("%s >>>>>>>>>>>>>>> found unspendable %s\n",coinstr,script);
    
    //else printf("(%s).%d\n",script+22,strcmp(script+16,"00000000000000000000000088ac"));
    return(redeemtxid);
}

int32_t ram_calc_scriptmode(int32_t *datalenp,uint8_t scriptdata[4096],char *script,int32_t trimflag)
{
    int32_t n=0,len,mode = 0;
    len = (int32_t)strlen(script);
    *datalenp = 0;
    if ( len >= 8191 )
    {
        printf("calc_scriptmode overflow len.%d\n",len);
        return(-1);
    }
    if ( strcmp(script,"ffff") == 0 )
    {
        mode = 'z';
        if ( trimflag != 0 )
            script[0] = 0;
    }
    else if ( strcmp(script,"nonstandard") == 0 )
    {
        if ( trimflag != 0 )
            script[0] = 0;
        mode = 'n';
    }
    else if ( strncmp(script,"76a914",6) == 0 && strcmp(script+len-4,"88ac") == 0 )
    {
        if ( trimflag != 0 )
        {
            script[len-4] = 0;
            script += 6;
        }
        mode = 's';
    }
    else if ( strcmp(script+len-2,"ac") == 0 )
    {
        if ( strncmp(script,"a9",2) == 0 )
        {
            if ( trimflag != 0 )
            {
                script[len-2] = 0;
                script += 2;
            }
            mode = 'm';
        }
        else
        {
            if ( trimflag != 0 )
                script[len-2] = 0;
            mode = 'r';
        }
    } else mode = ' ';
    if ( trimflag != 0 )
    {
        scriptdata[n++] = mode;
        if ( (len= (int32_t)(strlen(script) >> 1)) > 0 )
            decode_hex(scriptdata+n,len,script);
        (*datalenp) = (len + n);
        //printf("set pubkey.(%s).%ld <- (%s)\n",pubkeystr,strlen(pubkeystr),script);
    }
    return(mode);
}

uint64_t _calc_circulation(int32_t minconfirms,struct NXT_asset *ap,struct ramchain_info *ram)
{
    uint64_t quantity,circulation = 0;
    char cmd[4096],acct[MAX_JSON_FIELD],*retstr = 0;
    cJSON *json,*array,*item;
    uint32_t i,n,height = 0;
    if ( ap == 0 )
        return(0);
    if ( minconfirms != 0 )
    {
        ram->S.NXT_RTblocknum = _get_NXTheight(0);
        height = ram->S.NXT_RTblocknum - minconfirms;
    }
    sprintf(cmd,"requestType=getAssetAccounts&asset=%llu",(long long)ap->assetbits);
    if ( height > 0 )
        sprintf(cmd+strlen(cmd),"&height=%u",height);
    if ( (retstr= _issue_NXTPOST(cmd)) != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"accountAssets")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    item = cJSON_GetArrayItem(array,i);
                    copy_cJSON(acct,cJSON_GetObjectItem(item,"account"));
                    if ( acct[0] == 0 )
                        continue;
                    if ( _in_specialNXTaddrs(ram->special_NXTaddrs,ram->numspecials,acct) == 0 && (quantity= get_API_nxt64bits(cJSON_GetObjectItem(item,"quantityQNT"))) != 0 )
                    {
                        //if ( quantity > 2000 )
                        //    printf("Whale %s: %.8f\n",acct,dstr(quantity*ap->mult));
                        circulation += quantity;
                    }
                }
            }
            free_json(json);
        }
        free(retstr);
    }
    return(circulation * ap->mult);
}

#endif
#endif
