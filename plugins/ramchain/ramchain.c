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
#include "NXT777.c"
#include "huffstream.c"
#include "utils777.c"
#include "system777.c"
#include "files777.c"
#include "state.c"
#include "huff.c"
#include "storage.c"
#include "tokens.c"

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


struct rampayload { struct address_entry B,spentB; uint64_t value; uint32_t otherind:31,extra:31,pendingdeposit:1,pendingsend:1; };
struct ramchain_hashptr { int64_t unspent; UT_hash_handle hh; struct rampayload *payloads; uint32_t rawind,permind,numpayloads:29,maxpayloads:29,mine:1,multisig:1,verified:1,nonstandard:1,tbd:2; int32_t numunspent; };
struct ramchain_hashtable
{
    char coinstr[16];
    struct db777 *DB;
    dbobj *ptrs;
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
char *ram_searchpermind(struct ramchain_hashtable *hash,uint32_t permind);
void ram_setdispstr(char *buf,struct ramchain_info *ram,double startmilli);
HUFF **ram_get_hpptr(struct mappedblocks *blocks,uint32_t blocknum);
HUFF *ram_makehp(HUFF *tmphp,int32_t format,struct ramchain_info *ram,struct rawblock *tmp,int32_t blocknum);
int32_t ram_compress(HUFF *hp,struct ramchain_info *ram,uint8_t *data,int32_t datalen);
char *ram_blockstr(struct rawblock *tmp,struct ramchain_info *ram,struct rawblock *raw);
uint8_t *ram_encode_hashstr(int32_t *datalenp,uint8_t *data,char type,char *hashstr);
dbobj ram_hashdata_search(int32_t createflag,struct ramchain_hashtable *hash,uint8_t *hashdata,int32_t datalen);
uint32_t ram_create_block(int32_t verifyflag,struct ramchain_info *ram,struct mappedblocks *blocks,struct mappedblocks *prevblocks,uint32_t blocknum);
dbobj ram_addhash(struct ramchain_hashtable *hash,char *key,void *ptr,int32_t datalen);
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
void *ram_gethashdata(struct ramchain_info *ram,char type,uint32_t rawind);
struct rampayload *ram_getpayloadi(struct ramchain_hashptr **ptrp,struct ramchain_info *ram,char type,uint32_t rawind,uint32_t i);
struct ramchain_hashtable *ram_gethash(struct ramchain_info *ram,char type);
struct rampayload *ram_payloads(struct ramchain_hashptr **ptrp,int32_t *numpayloadsp,struct ramchain_info *ram,char *hashstr,char type);
void ram_addunspent(struct ramchain_info *ram,char *coinaddr,struct rampayload *txpayload,struct ramchain_hashptr *addrptr,struct rampayload *addrpayload,uint32_t addr_rawind,uint32_t ind);
int32_t ram_markspent(struct ramchain_info *ram,struct rampayload *txpayload,struct address_entry *spendbp,uint32_t txid_rawind);
//dbobj ram_hashsearch(char *key,int32_t createflag,struct ramchain_hashtable *hash,char *hashstr,char type);
void ram_set_MGWdispbuf(char *dispbuf,struct ramchain_info *ram,int32_t selector);
void ram_get_MGWpingstr(struct ramchain_info *ram,char *MGWpingstr,int32_t selector);
uint32_t ram_setcontiguous(struct mappedblocks *blocks);
int32_t _is_limbo_redeem(struct ramchain_info *ram,uint64_t redeemtxidbits);
int32_t _valid_txamount(struct ramchain_info *ram,uint64_t value,char *coinaddr);
void _complete_assettxid(struct NXT_assettxid *tp);
void ram_init_ramchain(struct ramchain_info *ram);
uint32_t _process_NXTtransaction(int32_t confirmed,struct ramchain_info *ram,cJSON *txobj);
uint64_t ram_calc_MGWunspent(uint64_t *pendingp,struct ramchain_info *ram);
void ram_set_MGWpingstr(char *pingstr,struct ramchain_info *ram,int32_t selector);
int32_t _sign_rawtransaction(char *deststr,unsigned long destsize,char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx,char *rawbytes,char **privkeys);
char *_sign_localtx(char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx,char *rawbytes);
int32_t ram_calcsha256(bits256 *sha,HUFF *bitstreams[],int32_t num);
void ram_purge_badblock(struct ramchain_info *ram,uint32_t blocknum);
void ram_setfname(char *fname,struct ramchain_info *ram,uint32_t blocknum,char *str);
void ram_setformatstr(char *formatstr,int32_t format);
long *ram_load_bitstreams(struct ramchain_info *ram,bits256 *sha,char *fname,HUFF *bitstreams[],int32_t *nump);
int32_t ram_save_bitstreams(bits256 *refsha,char *fname,HUFF *bitstreams[],int32_t num);
int32_t ram_map_bitstreams(int32_t verifyflag,struct ramchain_info *ram,int32_t blocknum,struct mappedptr *M,bits256 *sha,HUFF *blocks[],int32_t num,char *fname,bits256 *refsha);
uint32_t ram_update_RTblock(struct ramchain_info *ram);
int32_t ram_rawblock_update(int32_t iter,struct ramchain_info *ram,HUFF *hp,uint32_t checkblocknum);
int32_t ram_emitblock(HUFF *hp,int32_t destformat,struct ramchain_info *ram,struct rawblock *raw);
uint64_t _find_pending_transfers(uint64_t *pendingredeemsp,struct ramchain_info *ram);
int32_t _ram_update_redeembits(struct ramchain_info *ram,uint64_t redeembits,uint64_t AMtxidbits,char *cointxid,struct address_entry *bp);
int32_t ram_mark_depositcomplete(struct ramchain_info *ram,struct NXT_assettxid *tp,uint32_t blocknum);
int32_t ram_update_redeembits(struct ramchain_info *ram,cJSON *argjson,uint64_t AMtxidbits);

extern int32_t Numramchains; extern struct ramchain_info *Ramchains[];

#endif
#else
#ifndef crypto777_ramchain_c
#define crypto777_ramchain_c

#ifndef crypto777_ramchain_h
#define DEFINES_ONLY
#include "ramchain.c"
#undef DEFINES_ONLY
#endif

int32_t Numramchains; struct ramchain_info *Ramchains[100];

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

FILE *ram_permopen(char *fname,char *coinstr)
{
    FILE *fp;
    if ( 0 && strcmp(coinstr,"BTC") == 0 )
        return(0);
    if ( (fp= fopen(os_compatible_path(fname),"rb+")) == 0 )
        fp= fopen(os_compatible_path(fname),"wb+");
    if ( fp == 0 )
    {
        printf("ram_permopen couldnt create (%s)\n",fname);
        exit(-1);
    }
    return(fp);
}

void ram_sethashtype(char *str,int32_t type)
{
    switch ( type )
    {
        case 'a': strcpy(str,"addr"); break;
        case 't': strcpy(str,"txid"); break;
        case 's': strcpy(str,"script"); break;
        default: sprintf(str,"type%d",type); break;
    }
}

void huff_compressionvars_fname(int32_t readonly,char *fname,char *coinstr,char *typestr,int32_t subgroup)
{
    char *dirname = SUPERNET.MGWROOT;
    if ( subgroup < 0 )
        sprintf(fname,"%s/ramchains/%s/%s.%s",dirname,coinstr,coinstr,typestr);
    else sprintf(fname,"%s/ramchains/%s/%s/%s.%d",dirname,coinstr,typestr,coinstr,subgroup);
}

void ram_sethashname(char fname[1024],struct ramchain_hashtable *hash,int32_t newflag)
{
    char str[128],typestr[128];
    ram_sethashtype(str,hash->type);
    if ( newflag != 0 )
        sprintf(typestr,"%s.new",str);
    else strcpy(typestr,str);
    huff_compressionvars_fname(0,fname,hash->coinstr,typestr,-1);
}

struct ramchain_hashtable *ram_gethash(struct ramchain_info *ram,char type)
{
    switch ( type )
    {
        case 'a': return(&ram->addrhash); break;
        case 't': return(&ram->txidhash); break;
        case 's': return(&ram->scripthash); break;
    }
    return(0);
}

struct ramchain_hashptr *ram_gethashptr(struct ramchain_info *ram,char type,uint32_t rawind)
{
    struct ramchain_hashtable *table;
    struct ramchain_hashptr *ptr = 0;
    table = ram_gethash(ram,type);
    if ( table != 0 && rawind > 0 && rawind <= table->ind && table->ptrs != 0 )
        ptr = table->ptrs[rawind];
    return(ptr);
}

void *ram_gethashdata(struct ramchain_info *ram,char type,uint32_t rawind)
{
    struct ramchain_hashptr *ptr;
    if ( (ptr= ram_gethashptr(ram,type,rawind)) != 0 )
        return(ptr->hh.key);
    return(0);
}

int32_t ram_script_multisig(struct ramchain_info *ram,uint32_t scriptind)
{
    char *hex;
    if ( (hex= ram_gethashdata(ram,'s',scriptind)) != 0 && hex[1] == 'm' )
    {
        printf("(%02x %02x %02x (%c)) ",hex[0],hex[1],hex[2],hex[1]);
        return(1);
    }
    return(0);
}

int32_t ram_script_nonstandard(struct ramchain_info *ram,uint32_t scriptind)
{
    char *hex;
    if ( (hex= ram_gethashdata(ram,'s',scriptind)) != 0 && hex[2] != 'm' && hex[2] != 's' )
        return(1);
    return(0);
}

struct rampayload *ram_gethashpayloads(struct ramchain_info *ram,char type,uint32_t rawind)
{
    struct ramchain_hashptr *ptr;
    if ( (ptr= ram_gethashptr(ram,type,rawind)) != 0 )
        return(ptr->payloads);
    return(0);
}

struct rampayload *ram_payloads(struct ramchain_hashptr **ptrp,int32_t *numpayloadsp,struct ramchain_info *ram,char *hashstr,char type)
{
    struct ramchain_hashptr *ptr = 0;
    uint32_t rawind;
    *numpayloadsp = 0;
    if ( ptrp != 0 )
        *ptrp = 0;
    if ( (rawind= ram_conv_hashstr(0,0,ram,hashstr,type)) != 0 && (ptr= ram_gethashptr(ram,type,rawind)) != 0 )
    {
        if ( ptrp != 0 )
            *ptrp = ptr;
        *numpayloadsp = ptr->numpayloads;
        return(ptr->payloads);
    }
    else return(0);
}

struct address_entry *ram_address_entry(struct address_entry *destbp,struct ramchain_info *ram,char *txidstr,int32_t vout)
{
    int32_t numpayloads;
    struct rampayload *payloads;
    if ( (payloads= ram_txpayloads(0,&numpayloads,ram,txidstr)) != 0 )
    {
        if ( vout < payloads[0].B.v )
        {
            *destbp = payloads[vout].B;
            return(destbp);
        }
    }
    return(0);
}

struct rampayload *ram_getpayloadi(struct ramchain_hashptr **ptrp,struct ramchain_info *ram,char type,uint32_t rawind,uint32_t i)
{
    struct ramchain_hashptr *ptr;
    if ( ptrp != 0 )
        *ptrp = 0;
    if ( (ptr= ram_gethashptr(ram,type,rawind)) != 0 && i < ptr->numpayloads && ptr->payloads != 0 )
    {
        if ( ptrp != 0 )
            *ptrp = ptr;
        return(&ptr->payloads[i]);
    }
    return(0);
}

int32_t ram_init_hashtable(int32_t deletefile,uint32_t *blocknump,struct ramchain_info *ram,char type)
{
    struct ramchain_hashtable *hash;
    hash = ram_gethash(ram,type);
    if ( deletefile != 0 )
        memset(hash,0,sizeof(*hash));
    strcpy(hash->coinstr,ram->name);
    hash->type = type;
    return(1);
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
    if ( (retstr= issue_NXTPOST(cmd)) != 0 )
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

uint32_t _update_ramMGW(uint32_t *firsttimep,struct ramchain_info *ram,uint32_t mostrecent)
{
    cJSON *redemptions=0,*transfers,*array,*json;
    char cmd[1024],txid[512],*jsonstr,*txidjsonstr; //fname[512],
    struct NXT_asset *ap;
    uint32_t i,j,n,height,timestamp,oldest;
    while ( (ap= ram->ap) == 0 )
        portable_sleep(1);
    if ( ram->MGWbits == 0 )
    {
        printf("no MGWbits for %s\n",ram->name);
        return(0);
    }
    i = _get_NXTheight(&oldest);
    ram->S.NXT_ECblock = _get_NXT_ECblock(&ram->S.NXT_ECheight);
    //printf("NXTheight.%d ECblock.%d mostrecent.%d\n",i,ram->S.NXT_ECheight,mostrecent);
    if ( firsttimep != 0 )
        *firsttimep = oldest;
    if ( i != ram->S.NXT_RTblocknum )
    {
        ram->S.NXT_RTblocknum = i;
        printf("NEW NXTblocknum.%d when mostrecent.%d\n",i,mostrecent);
    }
    if ( mostrecent > 0 )
    {
        //printf("mostrecent %d <= %d (ram->S.NXT_RTblocknum %d - %d ram->min_NXTconfirms)\n", mostrecent,(ram->S.NXT_RTblocknum - ram->min_NXTconfirms),ram->S.NXT_RTblocknum,ram->min_NXTconfirms);
        while ( mostrecent <= (ram->S.NXT_RTblocknum - ram->min_NXTconfirms) )
        {
            sprintf(cmd,"requestType=getBlock&height=%u&includeTransactions=true",mostrecent);
            //printf("send cmd.(%s)\n",cmd);
            if ( (jsonstr= issue_NXTPOST(cmd)) != 0 )
            {
                // printf("getBlock.%d (%s)\n",mostrecent,jsonstr);
                if ( (json= cJSON_Parse(jsonstr)) != 0 )
                {
                    timestamp = (uint32_t)get_cJSON_int(json,"timestamp");
                    if ( timestamp != 0 && timestamp > ram->NXTtimestamp )
                    {
                        if ( ram->firsttime == 0 )
                            ram->firsttime = timestamp;
                        if ( ram->S.enable_deposits == 0 && timestamp > (ram->firsttime + (ram->DEPOSIT_XFER_DURATION+1)*60) )
                        {
                            ram->S.enable_deposits = 1;
                            printf("1st.%d ram->NXTtimestamp %d -> %d: enable_deposits.%d | %d > %d\n",ram->firsttime,ram->NXTtimestamp,timestamp,ram->S.enable_deposits,(timestamp - ram->firsttime),(ram->DEPOSIT_XFER_DURATION+1)*60);
                        }
                        ram->NXTtimestamp = timestamp;
                    }
                    if ( (array= cJSON_GetObjectItem(json,"transactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
                    {
                        for (i=0; i<n; i++)
                            _process_NXTtransaction(1,ram,cJSON_GetArrayItem(array,i));
                    }
                    free_json(json);
                } else printf("error parsing.(%s)\n",jsonstr);
                free(jsonstr);
            } else printf("error sending.(%s)\n",cmd);
            mostrecent++;
        }
        if ( ram->min_NXTconfirms == 0 )
        {
            sprintf(cmd,"requestType=getUnconfirmedTransactions");
            if ( (jsonstr= issue_NXTPOST(cmd)) != 0 )
            {
                //printf("getUnconfirmedTransactions.%d (%s)\n",mostrecent,jsonstr);
                if ( (json= cJSON_Parse(jsonstr)) != 0 )
                {
                    if ( (array= cJSON_GetObjectItem(json,"unconfirmedTransactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
                    {
                        for (i=0; i<n; i++)
                            _process_NXTtransaction(0,ram,cJSON_GetArrayItem(array,i));
                    }
                    free_json(json);
                }
                free(jsonstr);
            }
        }
    }
    else
    {
        for (j=0; j<ram->numspecials; j++)
        {
            //sprintf(fname,"%s/ramchains/NXT.%s",SUPERNET.MGWROOT,ram->special_NXTaddrs[j]);
            printf("(%s) init NXT special.%d of %d (%s)\n",ram->name,j,ram->numspecials,ram->special_NXTaddrs[j]);
            timestamp = 0;
            sprintf(cmd,"requestType=getAccountTransactions&account=%s&timestamp=%u",ram->special_NXTaddrs[j],timestamp);
            jsonstr = issue_NXTPOST(cmd);
            if ( jsonstr != 0 )
            {
                //printf("special.%d (%s) (%s)\n",j,ram->special_NXTaddrs[j],jsonstr);
                if ( (redemptions= cJSON_Parse(jsonstr)) != 0 )
                {
                    if ( (array= cJSON_GetObjectItem(redemptions,"transactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
                    {
                        for (i=0; i<n; i++)
                        {
                            //fprintf(stderr,".");
                            if ( (height= _process_NXTtransaction(1,ram,cJSON_GetArrayItem(array,i))) != 0 && height > mostrecent )
                                mostrecent = height;
                        }
                    }
                    free_json(redemptions);
                }
                free(jsonstr);
            }
        }
        //sprintf(fname,"%s/ramchains/NXTasset.%llu",SUPERNET.MGWROOT,(long long)ap->assetbits);
        sprintf(cmd,"requestType=getAssetTransfers&asset=%llu",(long long)ap->assetbits);
        jsonstr = issue_NXTPOST(cmd);
        if ( jsonstr != 0 )
        {
            if ( (transfers = cJSON_Parse(jsonstr)) != 0 )
            {
                if ( (array= cJSON_GetObjectItem(transfers,"transfers")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
                {
                    for (i=0; i<n; i++)
                    {
                        //fprintf(stderr,"t");
                        if ( (i % 10) == 9 )
                            fprintf(stderr,"%.1f%% ",100. * (double)i / n);
                        copy_cJSON(txid,cJSON_GetObjectItem(cJSON_GetArrayItem(array,i),"assetTransfer"));
                        if ( txid[0] != 0 && (txidjsonstr= _issue_getTransaction(txid)) != 0 )
                        {
                            if ( (json= cJSON_Parse(txidjsonstr)) != 0 )
                            {
                                if ( (height= _process_NXTtransaction(1,ram,json)) != 0 && height > mostrecent )
                                    mostrecent = height;
                                free_json(json);
                            }
                            free(txidjsonstr);
                        }
                    }
                }
                free_json(transfers);
            }
            free(jsonstr);
        }
    }
    ram->S.circulation = _calc_circulation(ram->min_NXTconfirms,ram->ap,ram);
    if ( ram->S.is_realtime != 0 )
        ram->S.orphans = _find_pending_transfers(&ram->S.MGWpendingredeems,ram);
    //printf("return mostrecent.%d\n",mostrecent);
    return(mostrecent);
}

uint32_t ram_update_disp(struct ramchain_info *ram)
{
    //int32_t pingall(char *coinstr,char *srvNXTACCTSECRET);
    if ( ram_update_RTblock(ram) > ram->lastdisp )
    {
        ram->blocks.blocknum = ram->blocks.contiguous = ram_setcontiguous(&ram->blocks);
        ram_disp_status(ram);
        ram->lastdisp = ram_update_RTblock(ram);
        //pingall(ram->name,ram->srvNXTACCTSECRET);
        return(ram->lastdisp);
    }
    return(0);
}

uint32_t ram_process_blocks(struct ramchain_info *ram,struct mappedblocks *blocks,struct mappedblocks *prev,double timebudget)
{
    HUFF **hpptr,*hp = 0;
    char formatstr[16];
    double estimated,startmilli = milliseconds();
    int32_t newflag,processed = 0;
    if ( ram->remotemode != 0 )
    {
        if ( blocks->format == 'B' )
        {
        }
        return(processed);
    }
    ram_setformatstr(formatstr,blocks->format);
    //printf("%s shift.%d %-5s.%d %.1f min left | [%d < %d]? %f %f timebudget %f\n",formatstr,blocks->shift,ram->name,blocks->blocknum,estimated,(blocks->blocknum >> blocks->shift),(prev->blocknum >> blocks->shift),milliseconds(),(startmilli + timebudget),timebudget);
    while ( (blocks->blocknum >> blocks->shift) < (prev->blocknum >> blocks->shift) && milliseconds() < (startmilli + timebudget) )
    {
        //printf("inside (%d) block.%d\n",blocks->format,blocks->blocknum);
        newflag = (ram->blocks.hps[blocks->blocknum] == 0);
        ram_create_block(1,ram,blocks,prev,blocks->blocknum), processed++;
        if ( (hpptr= ram_get_hpptr(blocks,blocks->blocknum)) != 0 && (hp= *hpptr) != 0 )
        {
            if ( blocks->format == 'B' && newflag != 0 )
            {
                if ( ram_rawblock_update(2,ram,hp,blocks->blocknum) < 0 )
                {
                    printf("FATAL: error updating block.%d %c\n",blocks->blocknum,blocks->format);
                    while ( 1 ) portable_sleep(1);
                }
                if ( ram->permfp != 0 )
                {
                    ram_conv_permind(ram->tmphp2,ram,hp,blocks->blocknum);
                    fflush(ram->permfp);
                }
            }
            //else printf("hpptr.%p hp.%p newflag.%d\n",hpptr,hp,newflag);
        } //else printf("ram_process_blocks: hpptr.%p hp.%p\n",hpptr,hp);
        blocks->processed += (1 << blocks->shift);
        blocks->blocknum += (1 << blocks->shift);
        estimated = estimate_completion(startmilli,blocks->processed,(int32_t)ram->S.RTblocknum-blocks->blocknum) / 60000.;
        //break;
    }
    //printf("(%d >> %d) < (%d >> %d)\n",blocks->blocknum,blocks->shift,prev->blocknum,blocks->shift);
    return(processed);
}

void *ram_process_blocks_loop(void *_blocks)
{
    struct mappedblocks *blocks = _blocks;
    printf("start _process_mappedblocks.%s format.%d\n",blocks->ram->name,blocks->format);
    while ( 1 )
    {
        ram_update_RTblock(blocks->ram);
        if ( ram_process_blocks(blocks->ram,blocks,blocks->prevblocks,1000.) == 0 )
            portable_sleep(sqrt(1 << blocks->shift));
    }
}

void *ram_process_ramchain(void *_ram)
{
    struct ramchain_info *ram = _ram;
    int32_t pass;
    ram->startmilli = milliseconds();
    for (pass=1; pass<=4; pass++)
    {
        if ( portable_thread_create((void *)ram_process_blocks_loop,ram->mappedblocks[pass]) == 0 )
            printf("ERROR _process_ramchain.%s\n",ram->name);
    }
    while ( 1 )
    {
        ram_update_disp(ram);
        portable_sleep(1);
    }
    return(0);
}

void activate_ramchain(struct ramchain_info *ram,char *name)
{
    Ramchains[Numramchains++] = ram;
    if ( Debuglevel > 0 )
        printf("ram.%p Add ramchain.(%s) (%s) Num.%d\n",ram,ram->name,name,Numramchains);
}

struct ramchain_info *get_ramchain_info(char *name)
{
    int32_t i;
    if ( Numramchains > 0 )
    {
        for (i=0; i<Numramchains; i++)
            if ( strcmp(Ramchains[i]->name,name) == 0 )
                return(Ramchains[i]);
    }
    return(0);
}

void *process_ramchains(void *_argcoinstr)
{
    //void ensure_SuperNET_dirs(char *backupdir);
    char *argcoinstr = (_argcoinstr != 0) ? ((char **)_argcoinstr)[0] : 0;
    int32_t iter,gatewayid,modval,numinterleaves;
    double startmilli;
    struct ramchain_info *ram;
    int32_t i,pass,processed = 0;
    //ensure_SuperNET_dirs("ramchains");
    startmilli = milliseconds();
    if ( _argcoinstr != 0 && ((long *)_argcoinstr)[1] != 0 && ((long *)_argcoinstr)[2] != 0 )
    {
        modval = (int32_t)((long *)_argcoinstr)[1];
        numinterleaves = (int32_t)((long *)_argcoinstr)[2];
        printf("modval.%d numinterleaves.%d\n",modval,numinterleaves);
    } else modval = 0, numinterleaves = 1;
    for (iter=0; iter<3; iter++)
    {
        for (i=0; i<Numramchains; i++)
        {
            if ( argcoinstr == 0 || strcmp(argcoinstr,Ramchains[i]->name) == 0 )
            {
                if ( iter > 1 )
                {
                    Ramchains[i]->S.NXTblocknum = _update_ramMGW(0,Ramchains[i],0);
                    if ( Ramchains[i]->S.NXTblocknum > 1000 )
                        Ramchains[i]->S.NXTblocknum -= 1000;
                    else Ramchains[i]->S.NXTblocknum = 0;
                    printf("i.%d of %d: NXTblock.%d (%s) 1sttime %d\n",i,Numramchains,Ramchains[i]->S.NXTblocknum,Ramchains[i]->name,Ramchains[i]->firsttime);
                }
                else if ( iter == 1 )
                {
                    ram_init_ramchain(Ramchains[i]);
                    Ramchains[i]->startmilli = milliseconds();
                }
                else if ( i != 0 )
                    Ramchains[i]->firsttime = Ramchains[0]->firsttime;
                else _get_NXTheight(&Ramchains[i]->firsttime);
            }
            printf("took %.1f seconds to init %s for %d coins\n",(milliseconds() - startmilli)/1000.,iter==0?"NXTheight":(iter==1)?"ramchains":"MGW",Numramchains);
        }
    }
    //MGW_initdone = 1;
    while ( processed >= 0 )
    {
        processed = 0;
        for (i=0; i<Numramchains; i++)
        {
            ram = Ramchains[i];
            if ( argcoinstr == 0 || strcmp(argcoinstr,ram->name) == 0 )
            {
                //if ( (ram->S.NXTblocknum+ram->min_NXTconfirms) < _get_NXTheight() || (ram->mappedblocks[1]->blocknum+ram->min_confirms) < _get_RTheight(&ram->lastgetinfo,ram->name,ram->serverport,ram->userpass,ram->S.RTblocknum) )
                {
                    //if ( strcmp(ram->name,"BTC") != 0 )//ram->S.is_realtime != 0 )
                    {
                        ram->S.NXTblocknum = _update_ramMGW(0,ram,ram->S.NXTblocknum);
                        if ( (ram->S.MGWpendingredeems + ram->S.MGWpendingdeposits) != 0 )
                            printf("\n");
                        ram->S.NXT_is_realtime = (ram->S.NXTblocknum >= (ram->S.NXT_RTblocknum - ram->min_NXTconfirms));
                    } //else ram->S.NXT_is_realtime = 0;
                    ram_update_RTblock(ram);
                    for (pass=1; pass<=4; pass++)
                    {
                        processed += ram_process_blocks(ram,ram->mappedblocks[pass],ram->mappedblocks[pass-1],60000.*pass*pass);
                        //if ( (ram->mappedblocks[pass]->blocknum >> ram->mappedblocks[pass]->shift) < (ram->mappedblocks[pass-1]->blocknum >> ram->mappedblocks[pass]->shift) )
                        //    break;
                    }
                    if ( ram_update_disp(ram) != 0 || 1 )
                    {
                        static char dispbuf[1000],lastdisp[1000];
                        ram->S.MGWunspent = ram_calc_MGWunspent(&ram->S.MGWpendingdeposits,ram);
                        ram->S.MGWbalance = ram->S.MGWunspent - ram->S.circulation - ram->S.MGWpendingredeems - ram->S.MGWpendingdeposits;
                        ram_set_MGWdispbuf(dispbuf,ram,-1);
                        if ( strcmp(dispbuf,lastdisp) != 0 )
                        {
                            strcpy(lastdisp,dispbuf);
                            ram_set_MGWpingstr(ram->MGWpingstr,ram,-1);
                            for (gatewayid=0; gatewayid<ram->numgateways; gatewayid++)
                            {
                                ram_set_MGWdispbuf(dispbuf,ram,gatewayid);
                                printf("G%d:%s",gatewayid,dispbuf);
                            }
                            if ( ram->pendingticks != 0 )
                            {
                                /*int32_t j;
                                 struct NXT_assettxid *tp;
                                 for (j=0; j<ram->numpendingsends; j++)
                                 {
                                 if ( (tp= ram->pendingsends[j]) != 0 )
                                 printf("(%llu %x %x %x) ",(long long)tp->redeemtxid,_extract_batchcrc(tp,0),_extract_batchcrc(tp,1),_extract_batchcrc(tp,2));
                                 printf("pendingticks.%d",ram->pendingticks);*/
                            }
                            putchar('\n');
                        }
                    }
                }
            }
        }
        for (i=0; i<Numramchains; i++)
        {
            ram = Ramchains[i];
            if ( argcoinstr == 0 || strcmp(argcoinstr,ram->name) == 0 )
                ram_update_disp(ram);
        }
        /*if ( processed == 0 )
         {
         void poll_nanomsg();
         poll_nanomsg();
         portable_sleep(1);
         }*/
        //MGW_initdone++;
    }
    printf("process_ramchains: finished launching\n");
    while ( 1 )
        portable_sleep(60);
}

int32_t set_bridge_dispbuf(char *dispbuf,char *coinstr)
{
    int32_t gatewayid;
    struct ramchain_info *ram;
    dispbuf[0] = 0;
    if ( (ram= get_ramchain_info(coinstr)) != 0 )
    {
        for (gatewayid=0; gatewayid<NUM_GATEWAYS; gatewayid++)
        {
            ram_set_MGWdispbuf(dispbuf,ram,gatewayid);
            sprintf(dispbuf+strlen(dispbuf),"G%d:%s",gatewayid,dispbuf);
        }
        printf("set_bridge_dispbuf.(%s)\n",dispbuf);
        return((int32_t)strlen(dispbuf));
    }
    return(0);
}

#endif
#endif
