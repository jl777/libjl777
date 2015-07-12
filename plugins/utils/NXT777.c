//
//  NXT777.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_NXT777_h
#define crypto777_NXT777_h
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "../includes/cJSON.h"
#include "../uthash.h"
#include "../utils/bits777.c"
#include "../utils/utils777.c"
#include "../utils/system777.c"
#include "../coins/coins777.c"

#include "tweetnacl.h"
int curve25519_donna(uint8_t *, const uint8_t *, const uint8_t *);
#define NXT_ASSETID ('N' + ((uint64_t)'X'<<8) + ((uint64_t)'T'<<16))    // 5527630
#define MAX_BUYNXT 10
#define MIN_NQTFEE 100000000
#define NXT_TOKEN_LEN 160

#define NXT_ASSETLIST_INCR 16
#define MAX_COINTXID_LEN 128
#define MAX_COINADDR_LEN 128
#define MAX_NXT_STRLEN 24
#define MAX_NXTTXID_LEN MAX_NXT_STRLEN
#define MAX_NXTADDR_LEN MAX_NXT_STRLEN

#define GENESISACCT "1739068987193023818"  // NXT-MRCC-2YLS-8M54-3CMAJ
#define GENESISBLOCK "2680262203532249785"
#define NXT_GENESISTIME 1385294400
#define GENESIS_SECRET "It was a bright cold day in April, and the clocks were striking thirteen."

#define DEFAULT_NXT_DEADLINE 720
#define issue_curl(cmdstr) bitcoind_RPC(0,"curl",cmdstr,0,0,0)
#define issue_NXT(cmdstr) bitcoind_RPC(0,"NXT",cmdstr,0,0,0)
#define issue_NXTPOST(cmdstr) bitcoind_RPC(0,"curl",SUPERNET.NXTAPIURL,0,0,cmdstr)
#define fetch_URL(url) bitcoind_RPC(0,"fetch",url,0,0,0)

#define INSTANTDEX_TRIGGERDEADLINE 15

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

union _NXT_str_buf { char txid[24]; char NXTaddr[24];  char assetid[24]; };
struct NXT_str { uint64_t modified,nxt64bits; union _NXT_str_buf U; };
union _asset_price { uint64_t assetoshis,price; };
struct assettrade { uint64_t assetid,seller,buyer,askorder,bidorder,price,amount:63,sellflag:1; uint32_t bidheight,askheight; };

/*struct NXT_assettxid
{
    UT_hash_handle hh;
    struct NXT_str H;
    uint64_t AMtxidbits,redeemtxid,assetbits,senderbits,receiverbits,quantity,convassetid;
    double minconvrate;
    union _asset_price U; // price 0 -> not buy/sell but might be deposit amount
    uint32_t coinblocknum,cointxind,coinv,height,redeemstarted;
    //void *redeemdata[3];
    char *cointxid;
    char *comment,*convwithdrawaddr,convname[16],teleport[64];
    float estNXT;
    int32_t completed,timestamp,numconfs,convexpiration,buyNXT,sentNXT;
};

struct NXT_assettxid_list { struct NXT_assettxid **txids; int32_t num,max; };
struct NXT_asset
{
    UT_hash_handle hh;
    struct NXT_str H;
    uint64_t issued,mult,assetbits,issuer;
    char *description,*name;
    //struct NXT_assettxid **txids;   // all transactions for this asset
    int32_t max,num,decimals;
    uint16_t type,subtype;
};*/

//struct storage_header { uint32_t size,createtime; uint64_t keyhash; };
struct pubkey_info { uint64_t nxt64bits; uint32_t ipbits; char pubkey[256],coinaddr[128]; };
struct multisig_addr
{
    //struct storage_header H;
    //UT_hash_handle hh;
    uint64_t sig,sender,modified;
    int32_t size,m,n,created,valid,buyNXT;
    char NXTaddr[MAX_NXTADDR_LEN],multisigaddr[MAX_COINADDR_LEN],NXTpubkey[96],redeemScript[2048],coinstr[16],email[128];
    struct pubkey_info pubkeys[];
};

struct nodestats
{
    //struct storage_header H;
    uint8_t pubkey[256>>3];
    struct nodestats *eviction;
    uint64_t nxt64bits;//,coins[4];
    double pingpongsum;
    float pingmilli,pongmilli;
    uint32_t ipbits,lastcontact,numpings,numpongs;
    uint8_t BTCD_p2p,gotencrypted,modified,expired;//,isMM;
};

struct NXT_acct
{
    UT_hash_handle hh;
    char NXTaddr[24];
    uint64_t nxt64bits;
    int32_t openorders;
    struct nodestats stats;
};

struct NXT_AMhdr { uint32_t sig; int32_t size; uint64_t nxt64bits; };
struct compressed_json { uint32_t complen,sublen,origlen,jsonlen; unsigned char encoded[128]; };
union _json_AM_data { unsigned char binarydata[sizeof(struct compressed_json)]; char jsonstr[sizeof(struct compressed_json)]; struct compressed_json jsn; };
struct json_AM { struct NXT_AMhdr H; uint32_t funcid,gatewayid,timestamp,jsonflag; union _json_AM_data U; };

struct assethash { UT_hash_handle hh; uint64_t assetid,minvol,mult; int32_t type,decimals; char name[16]; } *Allassets;
struct assethash *find_asset(uint64_t assetid);

uint64_t conv_NXTpassword(unsigned char *mysecret,unsigned char *mypublic,uint8_t *pass,int32_t passlen);
bits256 calc_sharedsecret(uint64_t *nxt64bitsp,int32_t *haspubpeyp,uint8_t *NXTACCTSECRET,int32_t secretlen,uint64_t other64bits);
int32_t curve25519_donna(uint8_t *mypublic,const uint8_t *secret,const uint8_t *basepoint);
uint64_t ram_verify_NXTtxstillthere(uint64_t ap_mult,uint64_t txidbits);
uint32_t _get_NXTheight(uint32_t *firsttimep);
char *_issue_getAsset(char *assetidstr);
char *_issue_getCurrency(char *assetidstr);
uint64_t _get_NXT_ECblock(uint32_t *ecblockp);
char *_issue_getTransaction(char *txidstr);
bits256 issue_getpubkey(int32_t *haspubkeyp,char *acct);
uint64_t conv_rsacctstr(char *rsacctstr,uint64_t nxt64bits);
uint64_t issue_transferAsset(char **retstrp,void *deprecated,char *secret,char *recipient,char *asset,int64_t quantity,int64_t feeNQT,int32_t deadline,char *comment,char *destpubkey);
uint64_t get_sender(uint64_t *amountp,char *txidstr);
uint64_t conv_acctstr(char *acctstr);
int32_t gen_randomacct(uint32_t randchars,char *NXTaddr,char *NXTsecret,char *randfilename);
void set_NXTpubkey(char *NXTpubkey,char *NXTacct);

char *NXT_assettxid(uint64_t assettxid);
uint64_t assetmult(char *assetidstr);
cJSON *NXT_convjson(cJSON *array);
char *issue_calculateFullHash(char *unsignedtxbytes,char *sighash);
char *issue_parseTransaction(char *txbytes);
int64_t get_asset_quantity(int64_t *unconfirmedp,char *NXTaddr,char *assetidstr);

uint64_t issue_broadcastTransaction(int32_t *errcodep,char **retstrp,char *txbytes,char *NXTACCTSECRET);
char *issue_signTransaction(char *txbytes,char *NXTACCTSECRET);
uint32_t get_blockutime(uint32_t blocknum);
uint64_t get_nxtlowask(uint64_t *sellvolp,uint64_t assetid);
uint64_t get_nxthighbid(uint64_t *buyvolp,uint64_t assetid);
uint64_t get_nxtlastprice(uint64_t assetid);
int32_t assetdecimals(char *assetidstr);
int32_t get_assettype(int32_t *decimals,char *assetidstr);

int32_t issue_decodeToken(char *sender,int32_t *validp,char *key,unsigned char encoded[NXT_TOKEN_LEN]);
int32_t issue_generateToken(char encoded[NXT_TOKEN_LEN],char *key,char *secret);
int32_t construct_tokenized_req(uint32_t *noncep,char *tokenized,char *cmdjson,char *NXTACCTSECRET,char *broadcastmode);
int32_t validate_token(char *forwarder,char *pubkey,char *NXTaddr,char *tokenizedtxt,int32_t strictflag);
char *cancel_orderid(char *NXTaddr,uint64_t orderid);

uint64_t calc_decimals_mult(int32_t decimals);
int32_t get_assetdecimals(uint64_t assetid);
uint64_t get_assetmult(uint64_t assetid);
double get_minvolume(uint64_t assetid);
uint64_t min_asset_amount(uint64_t assetid);
uint64_t RS_decode(char *rs);
int32_t RS_encode(char *rsaddr,uint64_t id);


struct nodestats *get_nodestats(uint64_t nxt64bits);
int32_t get_NXT_coininfo(uint64_t srvbits,uint64_t nxt64bits,char *coinstr,char *acctcoinaddr,char *pubkey);
int32_t add_NXT_coininfo(uint64_t srvbits,uint64_t nxt64bits,char *coinstr,char *acctcoinaddr,char *pubkey);
cJSON *http_search(char *destip,char *type,char *file);
struct NXT_acct *get_NXTacct(int32_t *createdp,char *NXTaddr);
int32_t update_msig_info(struct multisig_addr *msig,int32_t syncflag,char *sender);
struct NXT_acct *get_nxt64bits(int32_t *createdp,uint64_t nxt64bits);
int32_t issue_generateToken(char encoded[NXT_TOKEN_LEN],char *key,char *origsecret);

#endif
#else
#ifndef crypto777_NXT777_c
#define crypto777_NXT777_c

#ifndef crypto777_NXT777_h
#define DEFINES_ONLY
#include "NXT777.c"
#undef DEFINES_ONLY
#endif
#include "tweetnacl.c"
//__i686__ || __i386__
#if __amd64__
#include "curve25519-donna-c64.c"
#else
#include "curve25519-donna.c"
#endif

bits256 curve25519(bits256 mysecret,bits256 theirpublic)
{
    bits256 rawkey;
    curve25519_donna(&rawkey.bytes[0],&mysecret.bytes[0],&theirpublic.bytes[0]);
    return(rawkey);
}

uint64_t conv_NXTpassword(unsigned char *mysecret,unsigned char *mypublic,uint8_t *pass,int32_t passlen)
{
    static uint8_t basepoint[32] = {9};
    uint64_t addr;
    uint8_t hash[32];
    calc_sha256(0,mysecret,pass,passlen);
    mysecret[0] &= 248, mysecret[31] &= 127, mysecret[31] |= 64;
    curve25519_donna(mypublic,mysecret,basepoint);
    calc_sha256(0,hash,mypublic,32);
    memcpy(&addr,hash,sizeof(addr));
    return(addr);
}

bits256 issue_getpubkey(int32_t *haspubkeyp,char *acct)
{
    cJSON *json;
    bits256 pubkey;
    char cmd[4096],pubkeystr[MAX_JSON_FIELD],*jsonstr;
    sprintf(cmd,"%s?requestType=getAccountPublicKey&account=%s",SUPERNET.NXTAPIURL,acct);
    jsonstr = issue_curl(cmd);
    pubkeystr[0] = 0;
    if ( haspubkeyp != 0 )
        *haspubkeyp = 0;
    memset(&pubkey,0,sizeof(pubkey));
    if ( jsonstr != 0 )
    {
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            copy_cJSON(pubkeystr,cJSON_GetObjectItem(json,"publicKey"));
            free_json(json);
            if ( strlen(pubkeystr) == sizeof(pubkey)*2 )
            {
                if ( haspubkeyp != 0 )
                    *haspubkeyp = 1;
                decode_hex(pubkey.bytes,sizeof(pubkey),pubkeystr);
            }
        }
        free(jsonstr);
    }
    return(pubkey);
}

bits256 issue_getpubkey2(int32_t *haspubkeyp,uint64_t nxt64bits)
{
    cJSON *json;
    bits256 pubkey;
    char cmd[4096],pubkeystr[MAX_JSON_FIELD],*jsonstr;
    sprintf(cmd,"requestType=getAccountPublicKey&account=%llu",(long long)nxt64bits);
    jsonstr = issue_NXTPOST(cmd);
    pubkeystr[0] = 0;
    if ( haspubkeyp != 0 )
        *haspubkeyp = 0;
    memset(&pubkey,0,sizeof(pubkey));
    if ( jsonstr != 0 )
    {
        //printf("(%s) -> (%s)\n",cmd,jsonstr);
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            copy_cJSON(pubkeystr,cJSON_GetObjectItem(json,"publicKey"));
            free_json(json);
            if ( strlen(pubkeystr) == sizeof(pubkey)*2 )
            {
                if ( haspubkeyp != 0 )
                    *haspubkeyp = 1;
                decode_hex(pubkey.bytes,sizeof(pubkey),pubkeystr);
            } else printf("%llu -> %s\n",(long long)nxt64bits,jsonstr);
        }
        free(jsonstr);
    }
    return(pubkey);
}

bits256 calc_sharedsecret(uint64_t *nxt64bitsp,int32_t *haspubpeyp,uint8_t *NXTACCTSECRET,int32_t secretlen,uint64_t other64bits)
{
    bits256 pubkey,mysecret,mypublic,shared;
    pubkey = issue_getpubkey2(haspubpeyp,other64bits);
    if ( pubkey.txid != 0 )
    {
        *nxt64bitsp = conv_NXTpassword(mysecret.bytes,mypublic.bytes,(uint8_t *)NXTACCTSECRET,secretlen);
        shared = curve25519(mysecret,pubkey);
    } else memset(&shared,0,sizeof(shared)), *nxt64bitsp = 0;
    return(shared);
}

char *_issue_getAsset(char *assetidstr)
{
    char cmd[4096],*jsonstr;
    //sprintf(cmd,"requestType=getAsset&asset=%s",assetidstr);
    sprintf(cmd,"requestType=getAsset&asset=%s",assetidstr);
    //printf("_cmd.(%s)\n",cmd);
    jsonstr = issue_NXTPOST(cmd);
    //printf("(%s) -> (%s)\n",cmd,jsonstr);
    return(jsonstr);
}

char *_issue_getCurrency(char *assetidstr)
{
    char cmd[4096];
    //sprintf(cmd,"requestType=getAsset&asset=%s",assetidstr);
    sprintf(cmd,"requestType=getCurrency&asset=%s",assetidstr);
    //printf("_cmd.(%s)\n",cmd);
    return(issue_NXTPOST(cmd));
}

uint64_t _get_bestassetprice(uint64_t *volp,char *assetcmd,char *arrayfield,uint64_t assetid)
{
    char cmd[4096],*jsonstr;
    cJSON *array,*json;
    uint64_t price = 0;
    int32_t n;
    sprintf(cmd,"requestType=%s&asset=%llu&firstIndex=0&lastIndex=0",assetcmd,(long long)assetid);
    if ( (jsonstr= issue_NXTPOST(cmd)) != 0 )
    {
        //printf("(%s) -> (%s)\n",cmd,jsonstr);
        //printf("cmd.(%s) -> (%s)\n",cmd,jsonstr);
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,arrayfield)) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) == 1 )
            {
                price = get_API_nxt64bits(cJSON_GetObjectItem(cJSON_GetArrayItem(array,0),"priceNQT"));
                if ( volp != 0 )
                    *volp = get_API_nxt64bits(cJSON_GetObjectItem(cJSON_GetArrayItem(array,0),"quantityQNT"));
            }
            free_json(json);
        }
        free(jsonstr);
    }
    return(price);
}

uint64_t get_nxtlowask(uint64_t *sellvolp,uint64_t assetid) { return(_get_bestassetprice(sellvolp,"getAskOrders","askOrders",assetid)); }
uint64_t get_nxthighbid(uint64_t *buyvolp,uint64_t assetid) { return(_get_bestassetprice(buyvolp,"getBidOrders","bidOrders",assetid)); }
uint64_t get_nxtlastprice(uint64_t assetid) { return(_get_bestassetprice(0,"getTrades","trades",assetid)); }

uint64_t issue_getBalance(void *deprecated,char *NXTaddr)
{
    char cmd[4096],*jsonstr; cJSON *json; uint64_t ret = 0;
    sprintf(cmd,"requestType=getBalance&account=%s",NXTaddr);
    if ( (jsonstr= issue_NXTPOST(cmd)) != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
            ret = get_API_nxt64bits(cJSON_GetObjectItem(json,"balanceNQT")), free_json(json);
        free(jsonstr);
    }
    return(ret);
}

uint32_t get_blockutime(uint32_t blocknum)
{
    cJSON *json;
    uint32_t timestamp = 0;
    char cmd[4096],*jsonstr;
    sprintf(cmd,"requestType=getBlock&height=%u",blocknum);
    if ( (jsonstr= issue_NXTPOST(cmd)) != 0 )
    {
        //printf("(%s) -> (%s)\n",cmd,jsonstr);
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (timestamp= (uint32_t)get_API_int(cJSON_GetObjectItem(json,"timestamp"),0)) != 0 )
                timestamp += NXT_GENESISTIME;
            free_json(json);
        }
        free(jsonstr);
    }
    return(timestamp);
}

int64_t get_asset_quantity(int64_t *unconfirmedp,char *NXTaddr,char *assetidstr)
{
    char cmd[2*MAX_JSON_FIELD],assetid[MAX_JSON_FIELD],*jsonstr;
    int32_t i,n,iter;
    cJSON *array,*item,*obj,*json;
    uint64_t assetidbits = calc_nxt64bits(assetidstr);
    int64_t quantity,qty = 0;
    quantity = *unconfirmedp = 0;
    if ( assetidbits == NXT_ASSETID )
    {
        sprintf(cmd,"requestType=getBalance&account=%s",NXTaddr);
        if ( (jsonstr= issue_NXTPOST(cmd)) != 0 )
        {
            //printf("(%s) -> (%s)\n",cmd,jsonstr);
            if ( (json= cJSON_Parse(jsonstr)) != 0 )
            {
                qty = get_API_nxt64bits(cJSON_GetObjectItem(json,"balanceNQT"));
                *unconfirmedp = get_API_nxt64bits(cJSON_GetObjectItem(json,"unconfirmedBalanceNQT"));
                printf("(%s)\n",jsonstr);
                free_json(json);
            }
            free(jsonstr);
        }
        return(qty);
    }
    sprintf(cmd,"requestType=getAccount&account=%s",NXTaddr);
    if ( (jsonstr= issue_NXTPOST(cmd)) != 0 )
    {
        //printf("(%s) -> (%s)\n",cmd,jsonstr);
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            for (iter=0; iter<2; iter++)
            {
                qty = 0;
                array = cJSON_GetObjectItem(json,iter==0?"assetBalances":"unconfirmedAssetBalances");
                if ( is_cJSON_Array(array) != 0 )
                {
                    n = cJSON_GetArraySize(array);
                    for (i=0; i<n; i++)
                    {
                        item = cJSON_GetArrayItem(array,i);
                        obj = cJSON_GetObjectItem(item,"asset");
                        copy_cJSON(assetid,obj);
                        //printf("i.%d of %d: %s(%s)\n",i,n,assetid,cJSON_Print(item));
                        if ( strcmp(assetid,assetidstr) == 0 )
                        {
                            qty = get_cJSON_int(item,iter==0?"balanceQNT":"unconfirmedBalanceQNT");
                            break;
                        }
                    }
                }
                if ( iter == 0 )
                    quantity = qty;
                else *unconfirmedp = qty;
            }
            free_json(json);
        }
        free(jsonstr);
    }
    return(quantity);
}

char *issue_calculateFullHash(char *unsignedtxbytes,char *sighash)
{
    char cmd[4096];
    sprintf(cmd,"requestType=calculateFullHash&unsignedTransactionBytes=%s&signatureHash=%s",unsignedtxbytes,sighash);
    return(issue_NXTPOST(cmd));
}

char *issue_parseTransaction(char *txbytes)
{
    char cmd[4096],*retstr = 0;
    sprintf(cmd,"requestType=parseTransaction&transactionBytes=%s",txbytes);
    retstr = issue_NXTPOST(cmd);
    //printf("issue_parseTransaction.%s %s\n",txbytes,retstr);
    if ( retstr != 0 )
    {
        //printf("(%s) -> (%s)\n",cmd,retstr);
     //retstr = parse_NXTresults(0,"sender","",results_processor,jsonstr,strlen(jsonstr));
        //free(jsonstr);
    } else printf("error getting txbytes.%s\n",txbytes);
    return(retstr);
}

uint64_t issue_broadcastTransaction(int32_t *errcodep,char **retstrp,char *txbytes,char *NXTACCTSECRET)
{
    cJSON *json,*errjson;
    uint64_t txid = 0;
    char cmd[4096],secret[8192],*retstr;
    escape_code(secret,NXTACCTSECRET);
    sprintf(cmd,"requestType=broadcastTransaction&secretPhrase=%s&transactionBytes=%s",secret,txbytes);
    retstr = issue_NXTPOST(cmd);
    *errcodep = -1;
    if ( retstrp != 0 )
        *retstrp = retstr;
    if ( retstr != 0 )
    {
        //printf("(%s) -> (%s)\n",cmd,retstr);
        //printf("broadcast got.(%s)\n",retstr);
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            errjson = cJSON_GetObjectItem(json,"errorCode");
            if ( errjson != 0 )
            {
                //printf("ERROR broadcasting.(%s)\n",retstr);
                *errcodep = (int32_t)get_cJSON_int(json,"errorCode");
            }
            else
            {
                if ( (txid = get_satoshi_obj(json,"transaction")) != 0 )
                    *errcodep = 0;
            }
        }
        if ( retstrp == 0 )
            free(retstr);
    }
    return(txid);
}

char *issue_signTransaction(char *txbytes,char *NXTACCTSECRET)
{
    char cmd[4096],secret[8192];
    escape_code(secret,NXTACCTSECRET);
    sprintf(cmd,"requestType=signTransaction&secretPhrase=%s&unsignedTransactionBytes=%s",secret,txbytes);
    return(issue_NXTPOST(cmd));
}

uint32_t _get_NXTheight(uint32_t *firsttimep)
{
    cJSON *json;
    uint32_t height = 0;
    char cmd[256],*jsonstr;
    sprintf(cmd,"requestType=getState");
    if ( (jsonstr= issue_NXTPOST(cmd)) != 0 )
    {
        //printf("(%s) -> (%s)\n",cmd,jsonstr);
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( firsttimep != 0 )
                *firsttimep = (uint32_t)get_cJSON_int(json,"time");
            height = (int32_t)get_cJSON_int(json,"numberOfBlocks");
            if ( height > 0 )
                height--;
            free_json(json);
        }
        free(jsonstr);
    }
    return(height);
}

uint64_t _get_NXT_ECblock(uint32_t *ecblockp)
{
    cJSON *json;
    uint64_t ecblock = 0;
    char cmd[256],*jsonstr;
    sprintf(cmd,"requestType=getECBlock");
    if ( (jsonstr= issue_NXTPOST(cmd)) != 0 )
    {
        //printf("(%s) -> (%s)\n",cmd,jsonstr);
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( ecblockp != 0 )
                *ecblockp = (uint32_t)get_cJSON_int(json,"ecBlockHeight");
            ecblock = get_API_nxt64bits(cJSON_GetObjectItem(json,"ecBlockId"));
            free_json(json);
        }
        free(jsonstr);
    }
    return(ecblock);
}

uint64_t _get_AEquote(char *str,uint64_t orderid)
{
    cJSON *json;
    uint64_t nxt64bits = 0;
    char cmd[256],*jsonstr;
    sprintf(cmd,"requestType=get%sOrder&order=%llu",str,(long long)orderid);
    if ( (jsonstr= issue_NXTPOST(cmd)) != 0 )
    {
        //printf("(%s) -> (%s)\n",cmd,jsonstr);
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            nxt64bits = get_API_nxt64bits(cJSON_GetObjectItem(json,"account"));
            free_json(json);
        }
        free(jsonstr);
    }
    return(nxt64bits);
}

char *cancel_orderid(char *NXTaddr,uint64_t orderid)
{
    uint64_t nxt64bits; char cmd[1025],secret[8192],*str = "Bid",*retstr = 0;
    if ( (nxt64bits= _get_AEquote(str,orderid)) == 0 )
        str = "Ask", nxt64bits = _get_AEquote(str,orderid);
    if ( nxt64bits == calc_nxt64bits(NXTaddr) )
    {
        escape_code(secret,SUPERNET.NXTACCTSECRET);
        sprintf(cmd,"requestType=cancel%sOrder&secretPhrase=%s&feeNQT=%lld&deadline=%d&order=%llu",str,secret,(long long)MIN_NQTFEE,DEFAULT_NXT_DEADLINE,(long long)orderid);
        retstr = issue_NXTPOST(cmd);
        //printf("(%s) -> (%s)\n",cmd,retstr);
    }
    return(retstr);
}

char *_issue_getTransaction(char *txidstr)
{
    char cmd[4096];
    sprintf(cmd,"requestType=getTransaction&transaction=%s",txidstr);
    return(issue_NXTPOST(cmd));
}

uint64_t ram_verify_NXTtxstillthere(uint64_t ap_mult,uint64_t txidbits)
{
    char txidstr[64],*retstr;
    cJSON *json,*attach;
    uint64_t quantity = 0;
    expand_nxt64bits(txidstr,txidbits);
    if ( (retstr= _issue_getTransaction(txidstr)) != 0 )
    {
        //printf("verify.(%s)\n",retstr);
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            if ( (attach= cJSON_GetObjectItem(json,"attachment")) != 0 )
                quantity = get_API_nxt64bits(cJSON_GetObjectItem(attach,"quantityQNT"));
            free_json(json);
        }
        free(retstr);
    }
    //fprintf(stderr,"return %.8f\n",dstr(quantity * ram->ap->mult));
    return(quantity * ap_mult);
}

uint64_t conv_rsacctstr(char *rsacctstr,uint64_t nxt64bits)
{
    cJSON *json;
    char field[32],cmd[4096],retstr[4096],*jsonstr = 0;
    strcpy(field,"account");
    retstr[0] = 0;
    if ( nxt64bits != 0 )
    {
        sprintf(cmd,"requestType=rsConvert&account=%llu",(long long)nxt64bits);
        strcat(field,"RS");
        jsonstr = issue_NXTPOST(cmd);
    }
    else if ( rsacctstr[0] != 0 )
    {
        sprintf(cmd,"requestType=rsConvert&account=%s",rsacctstr);
        jsonstr = issue_NXTPOST(cmd);
    }
    else printf("conv_rsacctstr: illegal parms %s %llu\n",rsacctstr,(long long)nxt64bits);
    if ( jsonstr != 0 )
    {
        //printf("(%s) -> (%s)\n",cmd,jsonstr);
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            copy_cJSON(retstr,cJSON_GetObjectItem(json,field));
            free_json(json);
        }
        free(jsonstr);
        if ( nxt64bits != 0 )
            strcpy(rsacctstr,retstr);
        else nxt64bits = calc_nxt64bits(retstr);
    }
    return(nxt64bits);
}

uint64_t issue_transferAsset(char **retstrp,void *deprecated,char *secret,char *recipient,char *asset,int64_t quantity,int64_t feeNQT,int32_t deadline,char *comment,char *destpubkey)
{
    char cmd[16384],secretstr[8192],numstr[MAX_JSON_FIELD],*jsontxt;
    uint64_t assetidbits,txid = 0;
    cJSON *json,*errjson,*txidobj;
    *retstrp = 0;
    if ( strlen(secretstr) >= sizeof(secretstr)/3-1 )
    {
        fprintf(stderr,"issue_transferAsset: secret too long!\n");
        return(0);
    }
    escape_code(secretstr,secret);
    assetidbits = calc_nxt64bits(asset);
    if ( assetidbits == NXT_ASSETID )
        sprintf(cmd,"requestType=sendMoney&amountNQT=%lld",(long long)quantity);
    else sprintf(cmd,"requestType=transferAsset&asset=%s&quantityQNT=%lld&messageIsPrunable=false",asset,(long long)quantity);
    sprintf(cmd+strlen(cmd),"&secretPhrase=%s&recipient=%s&feeNQT=%lld&deadline=%d",secretstr,recipient,(long long)feeNQT,deadline);
    if ( destpubkey != 0 )
        sprintf(cmd+strlen(cmd),"&recipientPublicKey=%s",destpubkey);
    if ( comment != 0 )
    {
        strcat(cmd,"&message=");
        strcat(cmd,comment);
    }
    //printf("would have (%s)\n",cmd);
    //return(0);
    jsontxt = issue_NXTPOST(cmd);
    if ( jsontxt != 0 )
    {
        //printf("(%s) -> (%s)\n",cmd,jsontxt);
       //printf(" transferAsset.(%s) -> %s\n",cmd,jsontxt);
        //if ( field != 0 && strcmp(field,"transactionId") == 0 )
        //    printf("jsonstr.(%s)\n",jsonstr);
        json = cJSON_Parse(jsontxt);
        if ( json != 0 )
        {
            errjson = cJSON_GetObjectItem(json,"error");
            if ( errjson != 0 )
            {
                //printf("ERROR submitting assetxfer.(%s)\n",jsontxt);
                if ( retstrp != 0 )
                    *retstrp = jsontxt;
            }
            else
            {
                txidobj = cJSON_GetObjectItem(json,"transaction");
                copy_cJSON(numstr,txidobj);
                txid = calc_nxt64bits(numstr);
                if ( txid == 0 )
                {
                    //printf("ERROR WITH ASSET TRANSFER.(%s) -> \n%s\n",cmd,jsontxt);
                    if ( retstrp != 0 )
                        *retstrp = jsontxt;
                }
            }
            free_json(json);
        } else printf("error issuing asset.(%s) -> %s\n",cmd,jsontxt);
    }
    if ( *retstrp == 0 && jsontxt != 0 )
        free(jsontxt);
    return(txid);
}

uint64_t get_sender(uint64_t *amountp,char *txidstr)
{
    cJSON *json,*attachobj;
    char *jsonstr,numstr[MAX_JSON_FIELD];
    uint64_t senderbits = 0;
    jsonstr = _issue_getTransaction(txidstr);
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        copy_cJSON(numstr,cJSON_GetObjectItem(json,"sender"));
        senderbits = calc_nxt64bits(numstr);
        if ( (attachobj= cJSON_GetObjectItem(json,"quantityQNT")) != 0 )
        {
            copy_cJSON(numstr,cJSON_GetObjectItem(attachobj,"attachment"));
            *amountp = calc_nxt64bits(numstr);
        }
        free_json(json);
    }
    return(senderbits);
}

cJSON *NXT_convjson(cJSON *array)
{
    char acctstr[1024],nxtaddr[64]; int32_t i,n; uint64_t nxt64bits; cJSON *json = cJSON_CreateArray();
    if ( is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
    {
        for (i=0; i<n; i++)
        {
            copy_cJSON(acctstr,cJSON_GetArrayItem(array,i));
            if ( acctstr[0] != 0 )
            {
                nxt64bits = conv_acctstr(acctstr);
                expand_nxt64bits(nxtaddr,nxt64bits);
                printf("%s ",nxtaddr);
                cJSON_AddItemToArray(json,cJSON_CreateString(nxtaddr));
            }
        }
        printf("converted.%d\n",n);
    }
    return(json);
}

int32_t gen_randomacct(uint32_t randchars,char *NXTaddr,char *NXTsecret,char *randfilename)
{
    uint32_t i,j,x,iter,bitwidth = 6;
    FILE *fp;
    char fname[512];
    uint8_t bits[33],mypublic[32],mysecret[32];
    NXTaddr[0] = 0;
    randchars /= 8;
    if ( randchars > (int32_t)sizeof(bits) )
        randchars = (int32_t)sizeof(bits);
    if ( randchars < 3 )
        randchars = 3;
    for (iter=0; iter<=8; iter++)
    {
        sprintf(fname,"%s.%d",randfilename,iter);
        fp = fopen(os_compatible_path(fname),"rb");
        if ( fp == 0 )
        {
            randombytes(bits,sizeof(bits));
            for (i = 0; i < sizeof(bits); i++)
                printf("%02x ", bits[i]);
            printf("write\n");
            //sprintf(buf,"dd if=/dev/random count=%d bs=1 > %s",randchars*8,fname);
            //printf("cmd.(%s)\n",buf);
            //if ( system(buf) != 0 )
            //    printf("error issuing system(%s)\n",buf);
            fp = fopen(os_compatible_path(fname),"wb");
            if ( fp != 0 )
            {
                fwrite(bits,1,sizeof(bits),fp);
                fclose(fp);
            }
            portable_sleep(3);
            fp = fopen(os_compatible_path(fname),"rb");
        }
        if ( fp != 0 )
        {
            if ( fread(bits,1,sizeof(bits),fp) == 0 )
                printf("gen_random_acct: error reading bits\n");
            for (i=0; i+bitwidth<(sizeof(bits)*8) && i/bitwidth<randchars; i+=bitwidth)
            {
                for (j=x=0; j<6; j++)
                {
                    if ( GETBIT(bits,i*bitwidth+j) != 0 )
                        x |= (1 << j);
                }
                //printf("i.%d j.%d x.%d %c\n",i,j,x,1+' '+x);
                NXTsecret[randchars*iter + i/bitwidth] = safechar64(x);
            }
            NXTsecret[randchars*iter + i/bitwidth] = 0;
            fclose(fp);
        }
    }
    expand_nxt64bits(NXTaddr,conv_NXTpassword(mysecret,mypublic,(uint8_t *)NXTsecret,(int32_t)strlen(NXTsecret)));
    if ( Debuglevel > 2 )
        printf("NXT.%s NXTsecret.(%s)\n",NXTaddr,NXTsecret);
    return(0);
}

int32_t _in_specialNXTaddrs(struct mgw777 *mgw,char *NXTaddr)
{
    //printf("%s -> %d\n",NXTaddr,in_jsonarray(mgw->special,NXTaddr));
    return(in_jsonarray(mgw->special,NXTaddr));
}

uint64_t calc_decimals_mult(int32_t decimals)
{
    int32_t i; uint64_t mult = 1;
    for (i=7-decimals; i>=0; i--)
        mult *= 10;
    return(mult);
}

int32_t _set_assetname(uint64_t *multp,char *buf,char *jsonstr)
{
    int32_t decimals = -1;
    cJSON *json;
    if ( multp != 0 )
        *multp = 0;
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        if ( get_cJSON_int(json,"errorCode") == 0 )
        {
            decimals = (int32_t)get_cJSON_int(json,"decimals");
            if ( multp != 0 && decimals >= 0 && decimals <= 8 )
                *multp = calc_decimals_mult(decimals);
            if ( extract_cJSON_str(buf,16,json,"name") <= 0 )
                decimals = -1;
        }
        free_json(json);
    }
    return(decimals);
}

struct assethash *Allassets;
struct assethash *find_asset(uint64_t assetid)
{
    struct assethash *ap;
    HASH_FIND(hh,Allassets,&assetid,sizeof(assetid),ap);
    return(ap);
}

struct assethash *create_asset(uint64_t assetid,struct assethash *ap)
{
    struct assethash *newap;
    if ( (newap= find_asset(assetid)) != 0 )
        return(newap);
    newap = calloc(1,sizeof(*newap));
    *newap = *ap;
    HASH_ADD(hh,Allassets,assetid,sizeof(assetid),newap);
    return(newap);
}

int32_t get_assettype(int32_t *numdecimalsp,char *assetidstr)
{
    cJSON *json; char name[64],*jsonstr; uint64_t assetid; int32_t ap_type = -1; struct assethash *ap,A;
    if ( (assetid= calc_nxt64bits(assetidstr)) == NXT_ASSETID )
    {
        *numdecimalsp = 8;
        return(0);
    }
    if ( (ap= find_asset(assetid)) != 0 )
    {
        *numdecimalsp = ap->decimals;
        return(ap->type);
    }
    memset(name,0,sizeof(name));
    if ( (jsonstr= _issue_getAsset(assetidstr)) != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( get_cJSON_int(json,"errorCode") == 0 )
            {
                if ( extract_cJSON_str(name,16,json,"name") <= 0 )
                    *numdecimalsp = -1;
                else *numdecimalsp = (int32_t)get_cJSON_int(json,"decimals");
                ap_type = 2;
            }
            free_json(json);
        }
        free(jsonstr);
    }
    if ( ap_type < 0 )
    {
        if ( (jsonstr= _issue_getCurrency(assetidstr)) != 0 )
        {
            if ( (json= cJSON_Parse(jsonstr)) != 0 )
            {
                if ( get_cJSON_int(json,"errorCode") == 0 )
                {
                    if ( extract_cJSON_str(name,16,json,"name") <= 0 )
                        *numdecimalsp = -1;
                    else *numdecimalsp = (int32_t)get_cJSON_int(json,"decimals");
                    ap_type = 5;
                }
                free_json(json);
            }
            free(jsonstr);
        }
    }
    memset(&A,0,sizeof(A));
    A.assetid = assetid;
    A.minvol = A.mult = calc_decimals_mult(*numdecimalsp);
    A.decimals = *numdecimalsp;
    A.type = ap_type;
    strcpy(A.name,name);
    create_asset(assetid,&A);
    return(ap_type);
}

uint64_t assetmult(char *assetidstr)
{
    int32_t ap_type,decimals; uint64_t mult = 0;
    ap_type = get_assettype(&decimals,assetidstr);
    if ( ap_type == 0 )
        return(1);
    if ( decimals >= 0 && decimals <= 8 )
        mult = calc_decimals_mult(decimals);
    return(mult);
}

int32_t assetdecimals(char *assetidstr)
{
    int32_t ap_type,decimals = 0;
    ap_type = get_assettype(&decimals,assetidstr);
    if ( ap_type == 0 )
        return(8);
    return(decimals);
}

uint64_t min_asset_amount(uint64_t assetid)
{
    char assetidstr[64];
    if ( assetid == NXT_ASSETID )
        return(1);
    expand_nxt64bits(assetidstr,assetid);
    return(assetmult(assetidstr));
}

int32_t get_assetdecimals(uint64_t assetid)
{
    char assetidstr[64];
    if ( assetid == NXT_ASSETID )
        return(8);
    expand_nxt64bits(assetidstr,assetid);
    return(assetdecimals(assetidstr));
}

uint64_t get_assetmult(uint64_t assetid)
{
    char assetidstr[64];
    expand_nxt64bits(assetidstr,assetid);
    return(assetmult(assetidstr));
}

double get_minvolume(uint64_t assetid)
{
    return(dstr(get_assetmult(assetid)));
}

uint64_t calc_circulation(int32_t minconfirms,struct mgw777 *mgw,uint32_t height)
{
    uint64_t quantity,circulation = 0; char cmd[4096],acct[MAX_JSON_FIELD],*retstr = 0; cJSON *json,*array,*item; uint32_t i,n;
    mgw->RTNXT_height = _get_NXTheight(0);
    if ( minconfirms != 0 )
        height = mgw->RTNXT_height - minconfirms;
    sprintf(cmd,"requestType=getAssetAccounts&asset=%llu",(long long)mgw->assetidbits);
    if ( height > 0 )
        sprintf(cmd+strlen(cmd),"&height=%u",height);
    if ( (retstr= issue_NXTPOST(cmd)) != 0 )
    {
        //printf("(%s) -> (%s)\n",cmd,retstr);
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"accountAssets")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    item = cJSON_GetArrayItem(array,i);
                    copy_cJSON(acct,cJSON_GetObjectItem(item,"account"));
                    if ( acct[0] != 0 && _in_specialNXTaddrs(mgw,acct) == 0 && (quantity= get_API_nxt64bits(cJSON_GetObjectItem(item,"quantityQNT"))) != 0 )
                        circulation += quantity;
                }
            }
            free_json(json);
        }
        free(retstr);
    }
    return(circulation * mgw->ap_mult);
}

int32_t NXT_set_revassettxid(uint64_t assetidbits,uint32_t ind,struct extra_info *extra)
{
    uint64_t revkey[2]; void *obj;
    if ( (obj= sp_object(DB_NXTtxids->db)) != 0 )
    {
        revkey[0] = assetidbits, revkey[1] = ind;
        //printf("set ind.%d <- txid.%llu\n",ind,(long long)extra->txidbits);
        if ( sp_set(obj,"key",revkey,sizeof(revkey)) == 0 && sp_set(obj,"value",extra,sizeof(*extra)) == 0 )
            return(sp_set(DB_NXTtxids->db,obj));
        else
        {
            sp_destroy(obj);
            printf("error NXT_add_assettxid rev %llu ind.%d\n",(long long)extra->txidbits,ind);
        }
    }
    return(-1);
}

int32_t NXT_revassettxid(struct extra_info *extra,uint64_t assetidbits,uint32_t ind)
{
    void *obj,*result,*value; uint64_t revkey[2]; int32_t len = 0;
    memset(extra,0,sizeof(*extra));
    if ( (obj= sp_object(DB_NXTtxids->db)) != 0 )
    {
        revkey[0] = assetidbits, revkey[1] = ind;
        if ( sp_set(obj,"key",revkey,sizeof(revkey)) == 0 && (result= sp_get(DB_NXTtxids->db,obj)) != 0 )
        {
            value = sp_get(result,"value",&len);
            if ( len == sizeof(*extra) )
                memcpy(extra,value,len);
            else printf("NXT_revassettxid mismatched len.%d vs %ld\n",len,sizeof(*extra));
            sp_destroy(result);
        } //else sp_destroy(obj);
    }
    return(len);
}

int32_t NXT_add_assettxid(uint64_t assetidbits,uint64_t txidbits,void *value,int32_t valuelen,uint32_t ind,struct extra_info *extra)
{
    void *obj;
    if ( value != 0 )
    {
        if ( (obj= sp_object(DB_NXTtxids->db)) != 0 )
        {
            extra->assetidbits = assetidbits, extra->txidbits = txidbits, extra->ind = ind;
            if ( sp_set(obj,"key",&txidbits,sizeof(txidbits)) == 0 && sp_set(obj,"value",value,valuelen) == 0 )
                sp_set(DB_NXTtxids->db,obj);
            else
            {
                sp_destroy(obj);
                printf("error NXT_add_assettxid %llu ind.%d\n",(long long)txidbits,ind);
            }
        }
        NXT_set_revassettxid(assetidbits,ind,extra);
    }
    return(0);
}

char *NXT_assettxid(uint64_t assettxid)
{
    void *obj,*result,*value; int32_t len; char *retstr = 0;
    if ( (obj= sp_object(DB_NXTtxids->db)) != 0 )
    {
        if ( sp_set(obj,"key",&assettxid,sizeof(assettxid)) == 0 && (result= sp_get(DB_NXTtxids->db,obj)) != 0 )
        {
            value = sp_get(result,"value",&len);
            retstr = clonestr(value);
            sp_destroy(result);
        }// else sp_destroy(obj);
    }
    return(retstr);
}

uint64_t _set_NXT_sender(char *sender,cJSON *txobj)
{
    cJSON *senderobj;
    senderobj = cJSON_GetObjectItem(txobj,"sender");
    if ( senderobj == 0 )
        senderobj = cJSON_GetObjectItem(txobj,"accountId");
    else if ( senderobj == 0 )
        senderobj = cJSON_GetObjectItem(txobj,"account");
    copy_cJSON(sender,senderobj);
    if ( sender[0] != 0 )
        return(calc_nxt64bits(sender));
    else return(0);
}

int32_t process_assettransfer(uint32_t *heightp,uint64_t *senderbitsp,uint64_t *receiverbitsp,uint64_t *amountp,int32_t *flagp,char *coindata,int32_t confirmed,struct mgw777 *mgw,cJSON *txobj)
{
    char AMstr[MAX_JSON_FIELD],coinstr[MAX_JSON_FIELD],sender[MAX_JSON_FIELD],receiver[MAX_JSON_FIELD],assetidstr[MAX_JSON_FIELD],txid[MAX_JSON_FIELD],comment[MAX_JSON_FIELD],buf[MAX_JSON_FIELD];
    cJSON *attachment,*message,*assetjson,*commentobj,*json = 0,*obj; struct NXT_AMhdr *hdr;
    uint64_t units,estNXT; uint32_t buyNXT,height = 0; int32_t funcid,numconfs,coinv = -1,timestamp=0;
    int64_t type,subtype,n,satoshis,assetoshis = 0;
    *flagp = MGW_IGNORE, *amountp = *senderbitsp = *receiverbitsp = *heightp = 0;
    if ( txobj != 0 )
    {
        hdr = 0, sender[0] = receiver[0] = 0;
        *heightp = height = (uint32_t)get_cJSON_int(txobj,"height");
        if ( confirmed != 0 )
        {
            if ( (numconfs= (int32_t)get_API_int(cJSON_GetObjectItem(txobj,"confirmations"),0)) == 0 )
                numconfs = (_get_NXTheight(0) - height);
        } else numconfs = 0;
        copy_cJSON(txid,cJSON_GetObjectItem(txobj,"transaction"));
        type = get_cJSON_int(txobj,"type");
        subtype = get_cJSON_int(txobj,"subtype");
        timestamp = (int32_t)get_cJSON_int(txobj,"blockTimestamp");
        *senderbitsp = _set_NXT_sender(sender,txobj);
        copy_cJSON(receiver,cJSON_GetObjectItem(txobj,"recipient"));
        if ( receiver[0] != 0 )
            *receiverbitsp = calc_nxt64bits(receiver);
        attachment = cJSON_GetObjectItem(txobj,"attachment");
        if ( attachment != 0 )
        {
            message = cJSON_GetObjectItem(attachment,"message");
            assetjson = cJSON_GetObjectItem(attachment,"asset");
            memset(comment,0,sizeof(comment));
            if ( message != 0 && type == 1 )
            {
                copy_cJSON(AMstr,message);
                n = strlen(AMstr);
                if ( is_hexstr(AMstr) != 0 )
                {
                    if ( (n&1) != 0 || n > 2000 )
                        printf("warning: odd message len?? %ld\n",(long)n);
                    decode_hex((void *)buf,(int32_t)(n>>1),AMstr);
                    buf[(n>>1)] = 0;
                    hdr = (struct NXT_AMhdr *)buf;
                    //_process_AM_message(mgw,height,(void *)hdr,sender,receiver,txid);
                }
            }
            else if ( assetjson != 0 && type == 2 && subtype == 1 )
            {
                commentobj = cJSON_GetObjectItem(attachment,"comment");
                if ( commentobj == 0 )
                    commentobj = message;
                copy_cJSON(comment,commentobj);
                if ( comment[0] != 0 )
                {
                    unstringify(comment);
                    json = cJSON_Parse(comment);
                }
                copy_cJSON(assetidstr,cJSON_GetObjectItem(attachment,"asset"));
                assetoshis = get_cJSON_int(attachment,"quantityQNT");
                if ( mgw->NXTfee_equiv != 0 && mgw->txfee != 0 )
                    estNXT = (((double)mgw->NXTfee_equiv / mgw->txfee) * assetoshis / SATOSHIDEN);
                else estNXT = 0;
                *amountp = assetoshis * mgw->ap_mult;
                //printf("%s [%s] vs [%s] txid.(%s) (%s) -> %.8f estNXT %.8f json.%p\n",mgw->coinstr,mgw->assetidstr,assetidstr,txid,comment,dstr(assetoshis * mgw->ap_mult),dstr(estNXT),json);
                if ( assetidstr[0] != 0 && strcmp(mgw->assetidstr,assetidstr) == 0 )
                {
                    if ( json != 0 )
                    {
                        copy_cJSON(coinstr,cJSON_GetObjectItem(json,"coin"));
                        copy_cJSON(coindata,cJSON_GetObjectItem(json,"withdrawaddr"));
                        if ( coindata[0] != 0 )
                        {
                            if ( *receiverbitsp == mgw->issuerbits )
                                *flagp = MGW_PENDINGREDEEM;
                            else printf("%llu != issuer.%llu ",(long long)*receiverbitsp,(long long)mgw->issuerbits);
                        }
                        else
                        {
                            if ( (obj= cJSON_GetObjectItem(json,"coinv")) == 0 )
                                obj = cJSON_GetObjectItem(json,"vout");
                            coinv = (uint32_t)get_API_int(obj,-1);
                            copy_cJSON(coindata,cJSON_GetObjectItem(json,"cointxid"));
                            if ( coindata[0] != 0 )
                                *flagp = MGW_DEPOSITDONE;
                        }
                        free_json(json);
                        if ( coindata[0] != 0 )
                            unstringify(coindata);
                    }
                }
            }
            else
            {
                copy_cJSON(comment,message);
                unstringify(comment);
                commentobj = comment[0] != 0 ? cJSON_Parse(comment) : 0;
                if ( type == 5 && subtype == 3 )
                {
                    copy_cJSON(assetidstr,cJSON_GetObjectItem(attachment,"currency"));
                    units = get_API_int(cJSON_GetObjectItem(attachment,"units"),0);
                    if ( commentobj != 0 )
                    {
                        funcid = (int32_t)get_API_int(cJSON_GetObjectItem(commentobj,"funcid"),-1);
                        //if ( funcid >= 0 && (commentobj= _process_MGW_message(mgw,height,funcid,commentobj,calc_nxt64bits(assetidstr),units,sender,receiver,txid)) != 0 )
                        //    free_json(commentobj);
                    }
                }
                else if ( type == 0 && subtype == 0 && commentobj != 0 )
                {
                    if ( _in_specialNXTaddrs(mgw,sender) != 0 )
                    {
                        buyNXT = get_API_int(cJSON_GetObjectItem(commentobj,"buyNXT"),0);
                        satoshis = get_API_nxt64bits(cJSON_GetObjectItem(txobj,"amountNQT"));
                        if ( buyNXT*SATOSHIDEN == satoshis )
                        {
                            mgw->S.sentNXT += buyNXT * SATOSHIDEN;
                            printf("%s sent %d NXT, total sent %.0f\n",sender,buyNXT,dstr(mgw->S.sentNXT));
                        }
                        else if ( buyNXT != 0 )
                            printf("unexpected QNT %.8f vs %d\n",dstr(satoshis),buyNXT);
                    }
                    //if ( strcmp(sender,mgw->srvNXTADDR) == 0 )
                    //    ram_gotpayment(mgw,comment,commentobj);
                }
            }
        }
    }
    else printf("unexpected error iterating timestamp.(%d) txid.(%s)\n",timestamp,txid);
    //fprintf(stderr,"finish type.%d subtype.%d txid.(%s)\n",(int)type,(int)subtype,txid);
    return(coinv);
}

char *NXT_txidstr(struct mgw777 *mgw,char *txid,int32_t writeflag,uint32_t ind)
{
    void *obj,*value,*result = 0; int32_t slen,len,flag; uint64_t txidbits,savedbits; struct extra_info extra; char *txidjsonstr = 0; cJSON *json,*txobj;
    printf("NXT_txidstr.(%s) write.%d ind.%d\n",txid,writeflag,ind);
    if ( txid[0] != 0 && (txidjsonstr= _issue_getTransaction(txid)) != 0 )
    {
        flag = writeflag;
        if ( (json= cJSON_Parse(txidjsonstr)) != 0 )
        {
            free(txidjsonstr);
            cJSON_DeleteItemFromObject(json,"requestProcessingTime");
            cJSON_DeleteItemFromObject(json,"confirmations");
            cJSON_DeleteItemFromObject(json,"transactionIndex");
            txidjsonstr = cJSON_Print(json);
            free_json(json);
        } else printf("PARSE ERROR.(%s)\n",txidjsonstr);
        _stripwhite(txidjsonstr,' ');
        slen = (int32_t)strlen(txidjsonstr)+1;
        txidbits = calc_nxt64bits(txid);
        if ( (obj= sp_object(DB_NXTtxids->db)) != 0 )
        {
            if ( sp_set(obj,"key",&txidbits,sizeof(txidbits)) == 0 && (result= sp_get(DB_NXTtxids->db,obj)) != 0 )
            {
                value = sp_get(result,"value",&len);
                if ( value != 0 )
                {
                    if ( len != slen || strcmp(value,txidjsonstr) != 0 )
                        printf("mismatched NXT_txidstr ind.%d for %llu: lens %d vs %d (%s) vs (%s)\n",ind,(long long)txidbits,slen,len,txidjsonstr,value);
                    else flag = 0;
                }
                sp_destroy(result);
            } //else sp_destroy(obj);
        }
        if ( flag != 0 )
        {
            int32_t mgw_markunspent(char *txidstr,int32_t vout,int32_t status);
            NXT_revassettxid(&extra,mgw->assetidbits,ind);
            savedbits = extra.txidbits;
            memset(&extra,0,sizeof(extra));
            if ( (txobj= cJSON_Parse(txidjsonstr)) != 0 )
            {
                extra.vout = process_assettransfer(&extra.height,&extra.senderbits,&extra.receiverbits,&extra.amount,&extra.flags,extra.coindata,0,mgw,txobj);
                free_json(txobj);
                if ( extra.vout >= 0 )
                {
                    mgw_markunspent(extra.coindata,extra.vout,MGW_DEPOSITDONE);
                    printf("MARK DEPOSITDONE %llu.%d oldval.%llu -> newval flags.%d %llu (%s v%d %.8f)\n",(long long)mgw->assetidbits,ind,(long long)savedbits,extra.flags,(long long)txidbits,extra.coindata,extra.vout,dstr(extra.amount));
                }
            } else extra.vout = -1;
            printf("for %llu.%d oldval.%llu -> newval flags.%d %llu (%s v%d %.8f)\n",(long long)mgw->assetidbits,ind,(long long)savedbits,extra.flags,(long long)txidbits,extra.coindata,extra.vout,dstr(extra.amount));
            NXT_add_assettxid(mgw->assetidbits,txidbits,txidjsonstr,slen,ind,&extra);
        }
    }
    return(txidjsonstr);
}

int32_t NXT_assettransfers(struct mgw777 *mgw,uint64_t *txids,long max,int32_t firstindex,int32_t lastindex)
{
    char cmd[1024],txid[64],*jsonstr,*txidstr; cJSON *transfers,*array;
    int32_t i,n = 0; uint64_t txidbits,revkey[2];
    sprintf(cmd,"requestType=getAssetTransfers&asset=%s",mgw->assetidstr);
    if ( firstindex >= 0 && lastindex >= firstindex )
        sprintf(cmd + strlen(cmd),"&firstIndex=%u&lastIndex=%u",firstindex,lastindex);
    revkey[0] = mgw->assetidbits;
    //printf("issue.(%s) max.%ld\n",cmd,max);
    if ( (jsonstr= issue_NXTPOST(cmd)) != 0 )
    {
        //printf("(%s) -> (%s)\n",cmd,jsonstr);
        if ( (transfers = cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(transfers,"transfers")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    copy_cJSON(txid,cJSON_GetObjectItem(cJSON_GetArrayItem(array,i),"assetTransfer"));
                    if ( txid[0] != 0 && (txidbits= calc_nxt64bits(txid)) != 0 )
                    {
                        if ( i < max )
                            txids[i] = txidbits;
                        if ( firstindex < 0 && lastindex <= firstindex )
                        {
                            if ( (txidstr= NXT_txidstr(mgw,txid,1,n - i)) != 0 )
                                free(txidstr);
                        }
                    }
                }
            } free_json(transfers);
        } free(jsonstr);
    }
    //if ( firstindex < 0 || lastindex <= firstindex )
    //    printf("assetid.(%s) -> %d entries\n",mgw->assetidstr,n);
    return(n);
}

int32_t NXT_mark_withdrawdone(struct mgw777 *mgw,uint64_t redeemtxid)
{
    int32_t i,count; struct extra_info extra;
    if ( NXT_revassettxid(&extra,mgw->assetidbits,0) == sizeof(extra) )
    {
        //printf("got extra ind.%d\n",extra.ind);
        count = extra.ind;
        for (i=1; i<=count; i++)
        {
            NXT_revassettxid(&extra,mgw->assetidbits,i);
            if ( extra.txidbits == redeemtxid != 0 && (extra.flags & MGW_PENDINGREDEEM) != 0 && (extra.flags & MGW_WITHDRAWDONE) == 0 )
            {
                extra.flags |= MGW_WITHDRAWDONE;
                printf("NXT_mark_withdrawdone %s.%llu %.8f\n",mgw->coinstr,(long long)redeemtxid,dstr(extra.amount));
                NXT_set_revassettxid(mgw->assetidbits,i,&extra);
                return(i);
            }
            //fprintf(stderr,"%llu.%d ",(long long)extra.txidbits,extra.flags);
        }
    }
    return(-1);
}

int32_t update_NXT_assettransfers(struct mgw777 *mgw)
{
    int32_t len,verifyflag = 0;
    uint64_t txids[100],mostrecent; int32_t i,count = 0; char txidstr[128],nxt_txid[64],*txidjsonstr; struct extra_info extra;
    mgw->assetidbits = calc_nxt64bits(mgw->assetidstr);
    mgw->withdrawsum = mgw->numwithdraws = 0;
    if ( (len= NXT_revassettxid(&extra,mgw->assetidbits,0)) == sizeof(extra) )
    {
        //printf("got extra ind.%d\n",extra.ind);
        count = extra.ind;
        for (i=1; i<=count; i++)
        {
            NXT_revassettxid(&extra,mgw->assetidbits,i);
            if ( (extra.flags & MGW_PENDINGREDEEM) != 0 && (extra.flags & MGW_WITHDRAWDONE) == 0 )
            {
                int32_t mgw_update_redeem(struct mgw777 *mgw,struct extra_info *extra);
                expand_nxt64bits(nxt_txid,extra.txidbits);
                if ( in_jsonarray(mgw->limbo,nxt_txid) != 0 || mgw_update_redeem(mgw,&extra) != 0 )
                {
                    extra.flags |= MGW_WITHDRAWDONE;
                    NXT_set_revassettxid(mgw->assetidbits,i,&extra);
                }
            }
            //fprintf(stderr,"%llu.%d ",(long long)extra.txidbits,extra.flags);
        }
        //fprintf(stderr,"sequential tx.%d\n",count);
        NXT_revassettxid(&extra,mgw->assetidbits,count);
        mostrecent = extra.txidbits;
        //NXT_revassettxid(&extra,mgw->assetidbits,count+1);
        //if ( extra.txidbits != 0 )
        //    mostrecent = extra.txidbits;
        //printf("mostrecent.%llu count.%d\n",(long long)mostrecent,count);
        for (i=0; i<sizeof(txids)/sizeof(*txids); i++)
        {
            if ( NXT_assettransfers(mgw,&txids[i],1,i,i) == 1 && txids[i] == mostrecent )
            {
                if ( i != 0 )
                    printf("asset.(%s) count.%d i.%d mostrecent.%llu vs %llu\n",mgw->assetidstr,count,i,(long long)mostrecent,(long long)txids[i]);
                while ( i-- > 0 )
                {
                    expand_nxt64bits(txidstr,txids[i]);
                    if ( (txidjsonstr= NXT_txidstr(mgw,txidstr,1,++count)) != 0 )
                        free(txidjsonstr);
                }
                break;
            }
        }
        if ( i == 100 )
            count = 0;
    } else printf("cant get count len.%d\n",len);
    if ( count == 0 )
        count = NXT_assettransfers(mgw,txids,sizeof(txids)/sizeof(*txids) - 1,-1,-1);
    if ( NXT_revassettxid(&extra,mgw->assetidbits,0) != sizeof(extra) || extra.ind != count )
    {
        memset(&extra,0,sizeof(extra));
        extra.ind = count;
        NXT_set_revassettxid(mgw->assetidbits,0,&extra);
    }
    if ( verifyflag != 0 )
        NXT_assettransfers(mgw,txids,sizeof(txids)/sizeof(*txids) - 1,-1,-1);
    return(count);
}

uint64_t calc_txid(unsigned char *buf,int32_t len)
{
    uint64_t txid,hash[4];
    calc_sha256(0,(unsigned char *)&hash[0],buf,len);
    if ( sizeof(hash) >= sizeof(txid) )
        memcpy(&txid,hash,sizeof(txid));
    else memcpy(&txid,hash,sizeof(hash));
    //printf("calc_txid.(%llu)\n",(long long)txid);
    //return(hash[0] ^ hash[1] ^ hash[2] ^ hash[3]);
    return(txid);
}

/*
 NXT address converter,
 Ported from original javascript (nxtchg)
 To C by Jones
 */

int32_t gexp[] = {1, 2, 4, 8, 16, 5, 10, 20, 13, 26, 17, 7, 14, 28, 29, 31, 27, 19, 3, 6, 12, 24, 21, 15, 30, 25, 23, 11, 22, 9, 18, 1};
int32_t glog[] = {0, 0, 1, 18, 2, 5, 19, 11, 3, 29, 6, 27, 20, 8, 12, 23, 4, 10, 30, 17, 7, 22, 28, 26, 21, 25, 9, 16, 13, 14, 24, 15};
int32_t cwmap[] = {3, 2, 1, 0, 7, 6, 5, 4, 13, 14, 15, 16, 12, 8, 9, 10, 11};
char alphabet[] = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";

int32_t gmult(int32_t a,int32_t b)
{
    if ( a == 0 || b == 0 )
        return 0;
    int32_t idx = (glog[a] + glog[b]) % 31;
    return gexp[idx];
}

int32_t letterval(char letter)
{
    int32_t ret = 0;
    if ( letter < '9' )
        ret = letter - '2';
    else
    {
        ret = letter - 'A' + 8;
        if ( letter > 'I' )
            ret--;
        if ( letter > 'O' )
            ret--;
    }
    return ret;
}

uint64_t RS_decode(char *rs)
{
    int32_t code[] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int32_t i,p = 4;
    if ( strncmp("NXT-",rs,4) != 0 )
        return(0);
    for (i=0; i<17; i++)
    {
        code[cwmap[i]] = letterval(rs[p]);
        p++;
        if ( rs[p] == '-' )
            p++;
    }
    uint64_t out = 0;
    for (i=12; i>=0; i--)
        out = out * 32 + code[i];
    return out;
}

int32_t RS_encode(char *rsaddr,uint64_t id)
{
    int32_t a,code[] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int32_t inp[32],out[32],i,pos = 0,len = 0;
    char acc[64];
    sprintf(acc,"%lld",(long long)id);
    for (a=0; *(acc+a) != '\0'; a++)
        len ++;
    if ( len == 20 && *acc != '1' )
    {
        strcpy(rsaddr,"error");
        return(-1);
    }
    for (i=0; i<len; i++)
        inp[i] = (int32_t)*(acc+i) - (int32_t)'0';
    int32_t divide = 0;
    int32_t newlen = 0;
    do // base 10 to base 32 conversion
    {
        divide = 0;
        newlen = 0;
        for (i=0; i<len; i++)
        {
            divide = divide * 10 + inp[i];
            if (divide >= 32)
            {
                inp[newlen++] = divide >> 5;
                divide &= 31;
            }
            else if ( newlen > 0 )
                inp[newlen++] = 0;
        }
        len = newlen;
        out[pos++] = divide;
    } while ( newlen != 0 );
    for (i=0; i<13; i++) // copy to code in reverse, pad with 0's
        code[i] = (--pos >= 0 ? out[i] : 0);
    int32_t p[] = {0, 0, 0, 0};
    for (i=12; i>=0; i--)
    {
        int32_t fb = code[i] ^ p[3];
        p[3] = p[2] ^ gmult(30, fb);
        p[2] = p[1] ^ gmult(6, fb);
        p[1] = p[0] ^ gmult(9, fb);
        p[0] = gmult(17, fb);
    }
    code[13] = p[0];
    code[14] = p[1];
    code[15] = p[2];
    code[16] = p[3];
    strcpy(rsaddr,"NXT-");
    int32_t j=4;
    for (i=0; i<17; i++)
    {
        rsaddr[j++] = alphabet[code[cwmap[i]]];
        if ( (j % 5) == 3 && j < 20 )
            rsaddr[j++] = '-';
    }
    rsaddr[j] = 0;
    return(0);
}

uint64_t conv_acctstr(char *acctstr)
{
    uint64_t nxt64bits = 0;
    int32_t len;
    if ( acctstr != 0 )
    {
        if ( (len= is_decimalstr(acctstr)) > 0 && len < 24 )
            nxt64bits = calc_nxt64bits(acctstr);
        else if ( strncmp("NXT-",acctstr,4) == 0 )
            nxt64bits = RS_decode(acctstr);
    }
    return(nxt64bits);
}

void set_NXTpubkey(char *NXTpubkey,char *NXTacct)
{
    static uint8_t zerokey[256>>3];
    struct nodestats *stats;
    struct NXT_acct *np;
    char NXTaddr[64];
    uint64_t nxt64bits;
    int32_t createdflag;
    bits256 pubkey;
    if ( NXTpubkey != 0 )
        NXTpubkey[0] = 0;
    if ( NXTacct == 0 || NXTacct[0] == 0 )
        return;
    nxt64bits = conv_rsacctstr(NXTacct,0);
    expand_nxt64bits(NXTaddr,nxt64bits);
    np = get_NXTacct(&createdflag,NXTaddr);
    stats = &np->stats;
    if ( memcmp(stats->pubkey,zerokey,sizeof(stats->pubkey)) == 0 )
    {
        pubkey = issue_getpubkey(0,NXTacct);
        if ( memcmp(&pubkey,zerokey,sizeof(stats->pubkey)) != 0 )
            memcpy(stats->pubkey,&pubkey,sizeof(stats->pubkey));
    } else memcpy(&pubkey,stats->pubkey,sizeof(pubkey));
    if ( DB_NXTaccts != 0 )
        db777_add(0,0,DB_NXTaccts,&nxt64bits,sizeof(nxt64bits),np,sizeof(*np));
    else kv777_write(SUPERNET.NXTaccts,&nxt64bits,sizeof(nxt64bits),np,sizeof(*np));
    if ( NXTpubkey != 0 )
    {
        int32_t init_hexbytes_noT(char *hexbytes,unsigned char *message,long len);
        init_hexbytes_noT(NXTpubkey,pubkey.bytes,sizeof(pubkey));
    }
}

struct NXT_acct *get_nxt64bits(int32_t *createdp,uint64_t nxt64bits)
{
    static struct NXT_acct N,*np;
    int32_t len = sizeof(N);
    if ( DB_NXTaccts != 0 )
        np = db777_get(&N,&len,0,DB_NXTaccts,&nxt64bits,sizeof(nxt64bits));
    else np = kv777_read(SUPERNET.NXTaccts,&nxt64bits,sizeof(nxt64bits),&N,&len);
    if ( np == 0 )
    {
        np = calloc(1,sizeof(*np));
        np->nxt64bits = nxt64bits, expand_nxt64bits(np->NXTaddr,nxt64bits);
        if ( DB_NXTaccts != 0 )
            db777_add(1,0,DB_NXTaccts,&nxt64bits,sizeof(nxt64bits),np,sizeof(*np));
        else kv777_write(SUPERNET.NXTaccts,&nxt64bits,sizeof(nxt64bits),&N,sizeof(*np));
       *createdp = 1;
    } else *createdp = 0;
    return(np);
}

struct NXT_acct *get_NXTacct(int32_t *createdp,char *NXTaddr)
{
    return(get_nxt64bits(createdp,calc_nxt64bits(NXTaddr)));
}

#endif
#endif

