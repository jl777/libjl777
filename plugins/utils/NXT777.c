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
#include "cJSON.h"
#include "uthash.h"
#include "db777.c"
#include "bits777.c"
#include "utils777.c"
#include "system777.c"
#include "coins777.c"

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
#define _NXTSERVER "requestType"
#define issue_curl(cmdstr) bitcoind_RPC(0,"curl",cmdstr,0,0,0)
#define issue_NXT(cmdstr) bitcoind_RPC(0,"NXT",cmdstr,0,0,0)
#define issue_NXTPOST(cmdstr) bitcoind_RPC(0,"curl",SUPERNET.NXTAPIURL,0,0,cmdstr)
#define fetch_URL(url) bitcoind_RPC(0,"fetch",url,0,0,0)

#define MIN_NQTFEE 100000000
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

union NXTtype { uint64_t nxt64bits; uint32_t uval; int32_t val; int64_t lval; double dval; char *str; cJSON *json; };
union _NXT_str_buf { char txid[24]; char NXTaddr[24];  char assetid[24]; };
struct NXT_str { uint64_t modified,nxt64bits; union _NXT_str_buf U; };
union _asset_price { uint64_t assetoshis,price; };

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

//struct NXT_assettxid *find_NXT_assettxid(int32_t *createdflagp,struct NXT_asset *ap,char *txid);

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
uint64_t assetmult(char *assetname,char *assetidstr);
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
int32_t get_assettype(int32_t *subtypep,char *assetidstr);

int32_t issue_decodeToken(char *sender,int32_t *validp,char *key,unsigned char encoded[NXT_TOKEN_LEN]);
int32_t issue_generateToken(char encoded[NXT_TOKEN_LEN],char *key,char *secret);
int32_t construct_tokenized_req(char *tokenized,char *cmdjson,char *NXTACCTSECRET);
int32_t validate_token(char *pubkey,char *NXTaddr,char *tokenizedtxt,int32_t strictflag);

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
#if __i686__ || __i386__
#include "curve25519-donna.c"
#else
#include "curve25519-donna-c64.c"
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
    //sprintf(cmd,"%s=getTransaction&transaction=%s",_NXTSERVER,txidstr);
    //jsonstr = issue_NXTPOST(curl_handle,cmd);
    sprintf(cmd,"%s=getAccountPublicKey&account=%s",SUPERNET.NXTSERVER,acct);
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
    sprintf(cmd,"%s=getAccountPublicKey&account=%llu",SUPERNET.NXTSERVER,(long long)nxt64bits);
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
    char cmd[4096];
    //sprintf(cmd,"requestType=getAsset&asset=%s",assetidstr);
    sprintf(cmd,"%s=getAsset&asset=%s",SUPERNET.NXTSERVER,assetidstr);
    //printf("_cmd.(%s)\n",cmd);
    return(issue_curl(cmd));
}

char *_issue_getCurrency(char *assetidstr)
{
    char cmd[4096];
    //sprintf(cmd,"requestType=getAsset&asset=%s",assetidstr);
    sprintf(cmd,"%s=getCurrency&asset=%s",SUPERNET.NXTSERVER,assetidstr);
    //printf("_cmd.(%s)\n",cmd);
    return(issue_curl(cmd));
}

uint64_t _get_bestassetprice(uint64_t *volp,char *assetcmd,char *arrayfield,uint64_t assetid)
{
    char cmd[4096],*jsonstr;
    cJSON *array,*json;
    uint64_t price = 0;
    int32_t n;
    sprintf(cmd,"%s=%s&asset=%llu&firstIndex=0&lastIndex=0",SUPERNET.NXTSERVER,assetcmd,(long long)assetid);
    if ( (jsonstr= issue_curl(cmd)) != 0 )
    {
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

union NXTtype extract_NXTfield(void *deprecated,char *origoutput,char *cmd,char *field,int32_t type)
{
    char *jsonstr,*output,NXTaddr[MAX_NXTADDR_LEN]; cJSON *json,*obj,*errobj;  union NXTtype retval;
    retval.nxt64bits = 0;
    if ( origoutput == 0 )
        output = NXTaddr;
    else output = origoutput;
        jsonstr = issue_NXTPOST(cmd);
    if ( jsonstr != 0 )
    {
        if ( field != 0 && strcmp(field,"transactionId") == 0 )
            printf("jsonstr.(%s)\n",jsonstr);
        json = cJSON_Parse(jsonstr);
        if ( json == 0 ) printf("Error before: (%s) -> [%s]\n",jsonstr,cJSON_GetErrorPtr());
        else
        {
            errobj = cJSON_GetObjectItem(json,"errorCode");
            if ( errobj != 0 )
            {
                printf("cmd.(%s) -> %s\n",cmd,jsonstr);
            }
            if ( field == 0 )
            {
                if ( origoutput == 0 )
                    retval.json = json;
                else
                {
                    copy_cJSON(origoutput,json);
                    retval.str = origoutput;
                    free_json(json);
                }
            }
            else
            {
                obj = cJSON_GetObjectItem(json,field);
                if ( obj != 0 )
                {
                    copy_cJSON(output,obj);
                    //if ( strcmp(field,"transactionId") == 0 )
                    //    printf("obj.(%s) type.%d\n",output,type);
                    switch ( type )
                    {
                        case sizeof(double):
                            retval.dval = atof(output);
                            break;
                        case sizeof(uint32_t):
                            retval.uval = atoi(output);
                            break;
                        case -(int32_t)sizeof(int32_t):
                            retval.val = atoi(output);
                            break;
                        case -(int32_t)sizeof(int64_t):
                            retval.lval = calc_nxt64bits(output);
                            break;
                        case 64:
                            retval.nxt64bits = calc_nxt64bits(output);
                            //if ( strcmp(field,"transactionId") == 0 )
                            //    printf("transactionId.%s\n",nxt64str(retval.nxt64bits));
                            break;
                        case 0:
                            if ( origoutput != 0 )
                                retval.str = origoutput;
                            else retval.str = 0;
                            break;
                        default: printf("extract_NXTfield: warning unknown type.%d\n",type);
                    }
                }
                free_json(json);
            }
        }
        free(jsonstr);
    }
    else printf("ERROR submitting cmd.(%s)\n",cmd);
        return(retval);
}

uint64_t issue_getBalance(void *deprecated,char *NXTaddr)
{
    char cmd[4096];
    union NXTtype ret;
    sprintf(cmd,"%s=getBalance&account=%s",SUPERNET.NXTSERVER,NXTaddr);
    ret = extract_NXTfield(deprecated,0,cmd,"balanceNQT",64);
    return(ret.nxt64bits);
}

uint32_t get_blockutime(uint32_t blocknum)
{
    cJSON *json;
    uint32_t timestamp = 0;
    char cmd[4096],*jsonstr;
    sprintf(cmd,"%s=getBlock&height=%u",SUPERNET.NXTSERVER,blocknum);
    if ( (jsonstr= issue_curl(cmd)) != 0 )
    {
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
    union NXTtype retval;
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
    sprintf(cmd,"%s=getAccount&account=%s",SUPERNET.NXTSERVER,NXTaddr);
    retval = extract_NXTfield(0,0,cmd,0,0);
    if ( retval.json != 0 )
    {
        for (iter=0; iter<2; iter++)
        {
            qty = 0;
            array = cJSON_GetObjectItem(retval.json,iter==0?"assetBalances":"unconfirmedAssetBalances");
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
    }
    return(quantity);
}

char *issue_calculateFullHash(char *unsignedtxbytes,char *sighash)
{
    char cmd[4096];
    sprintf(cmd,"%s=calculateFullHash&unsignedTransactionBytes=%s&signatureHash=%s",SUPERNET.NXTSERVER,unsignedtxbytes,sighash);
    return(issue_NXTPOST(cmd));
}

char *issue_parseTransaction(char *txbytes)
{
    char cmd[4096],*retstr = 0;
    sprintf(cmd,"%s=parseTransaction&transactionBytes=%s",SUPERNET.NXTSERVER,txbytes);
    retstr = issue_NXTPOST(cmd);
    //printf("issue_parseTransaction.%s %s\n",txbytes,retstr);
    if ( retstr != 0 )
    {
        //retstr = parse_NXTresults(0,"sender","",results_processor,jsonstr,strlen(jsonstr));
        //free(jsonstr);
    } else printf("error getting txbytes.%s\n",txbytes);
    return(retstr);
}

uint64_t issue_broadcastTransaction(int32_t *errcodep,char **retstrp,char *txbytes,char *NXTACCTSECRET)
{
    cJSON *json,*errjson;
    uint64_t txid = 0;
    char cmd[4096],*retstr;
    sprintf(cmd,"%s=broadcastTransaction&secretPhrase=%s&transactionBytes=%s",SUPERNET.NXTSERVER,NXTACCTSECRET,txbytes);
    retstr = issue_NXTPOST(cmd);
    *errcodep = -1;
    if ( retstrp != 0 )
        *retstrp = retstr;
    if ( retstr != 0 )
    {
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
    char cmd[4096];
    sprintf(cmd,"%s=signTransaction&secretPhrase=%s&unsignedTransactionBytes=%s",SUPERNET.NXTSERVER,NXTACCTSECRET,txbytes);
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
        sprintf(cmd,"%s=rsConvert&account=%llu",SUPERNET.NXTSERVER,(long long)nxt64bits);
        strcat(field,"RS");
        jsonstr = issue_curl(cmd);
    }
    else if ( rsacctstr[0] != 0 )
    {
        sprintf(cmd,"%s=rsConvert&account=%s",SUPERNET.NXTSERVER,rsacctstr);
        jsonstr = issue_curl(cmd);
    }
    else printf("conv_rsacctstr: illegal parms %s %llu\n",rsacctstr,(long long)nxt64bits);
    if ( jsonstr != 0 )
    {
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
    char cmd[4096],numstr[MAX_JSON_FIELD],*jsontxt;
    uint64_t assetidbits,txid = 0;
    cJSON *json,*errjson,*txidobj;
    *retstrp = 0;
    assetidbits = calc_nxt64bits(asset);
    if ( assetidbits == NXT_ASSETID )
        sprintf(cmd,"%s=sendMoney&amountNQT=%lld",SUPERNET.NXTSERVER,(long long)quantity);
    else sprintf(cmd,"%s=transferAsset&asset=%s&quantityQNT=%lld&messageIsPrunable=false",SUPERNET.NXTSERVER,asset,(long long)quantity);
    sprintf(cmd+strlen(cmd),"&secretPhrase=%s&recipient=%s&feeNQT=%lld&deadline=%d",secret,recipient,(long long)feeNQT,deadline);
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

uint64_t conv_acctstr(char *acctstr)
{
    uint64_t nxt64bits = 0;
    int32_t len;
    if ( (len= is_decimalstr(acctstr)) > 0 && len < 24 )
        nxt64bits = calc_nxt64bits(acctstr);
    else if ( strncmp("NXT-",acctstr,4) == 0 )
        nxt64bits = conv_rsacctstr(acctstr,0);
    return(nxt64bits);
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
    db777_add(0,0,DB_NXTaccts,&nxt64bits,sizeof(nxt64bits),np,sizeof(*np));
    if ( NXTpubkey != 0 )
    {
        int32_t init_hexbytes_noT(char *hexbytes,unsigned char *message,long len);
        init_hexbytes_noT(NXTpubkey,pubkey.bytes,sizeof(pubkey));
    }
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

uint64_t assetmult(char *assetname,char *assetidstr)
{
    cJSON *json; char _assetname[16],*jsonstr; int32_t decimals; uint64_t mult = 0;
    if ( calc_nxt64bits(assetidstr) == NXT_ASSETID )
        return(1);
    if ( (jsonstr= _issue_getAsset(assetidstr)) != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( get_cJSON_int(json,"errorCode") == 0 )
            {
                decimals = (int32_t)get_cJSON_int(json,"decimals");
                if ( decimals >= 0 && decimals <= 8 )
                    mult = calc_decimals_mult(decimals);
                if ( extract_cJSON_str(assetname!=0?assetname:_assetname,sizeof(_assetname),json,"name") <= 0 )
                    decimals = -1;
            }
            free_json(json);
        }
        free(jsonstr);
    }
    return(mult);
}

int32_t assetdecimals(char *assetidstr)
{
    cJSON *json; char *jsonstr; int32_t decimals = 0;
    if ( calc_nxt64bits(assetidstr) == NXT_ASSETID )
        return(8);
    if ( (jsonstr= _issue_getAsset(assetidstr)) != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( get_cJSON_int(json,"errorCode") == 0 )
                decimals = (int32_t)get_cJSON_int(json,"decimals");
            free_json(json);
        }
        free(jsonstr);
    }
    return(decimals);
}

int32_t get_assettype(int32_t *subtypep,char *assetidstr)
{
    cJSON *json; char *jsonstr; int32_t ap_type = -1;
    if ( calc_nxt64bits(assetidstr) == NXT_ASSETID )
        return(8);
    if ( (jsonstr= _issue_getAsset(assetidstr)) != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( get_cJSON_int(json,"errorCode") == 0 )
            {
                ap_type = (int32_t)get_cJSON_int(json,"type");
                *subtypep = (int32_t)get_cJSON_int(json,"subtype");
            }
            free_json(json);
        }
        free(jsonstr);
    }
    return(ap_type);
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

int32_t issue_generateToken(char encoded[NXT_TOKEN_LEN],char *key,char *secret)
{
    char cmd[4096],token[MAX_JSON_FIELD+2*NXT_TOKEN_LEN+1];
    cJSON *tokenobj;
    union NXTtype retval;
    encoded[0] = 0;
    sprintf(cmd,"%s=generateToken&website=%s&secretPhrase=%s",SUPERNET.NXTSERVER,key,secret);
    // printf("cmd.(%s)\n",cmd);
    retval = extract_NXTfield(0,0,cmd,0,0);
    if ( retval.json != 0 )
    {
        //printf("token.(%s)\n",cJSON_Print(retval.json));
        tokenobj = cJSON_GetObjectItem(retval.json,"token");
        memset(token,0,sizeof(token));
        copy_cJSON(token,tokenobj);
        free_json(retval.json);
        if ( token[0] != 0 )
        {
            memcpy(encoded,token,NXT_TOKEN_LEN);
            return(0);
        }
    }
    return(-1);
}

int32_t construct_tokenized_req(char *tokenized,char *cmdjson,char *NXTACCTSECRET)
{
    char encoded[NXT_TOKEN_LEN+1];
    _stripwhite(cmdjson,' ');
    issue_generateToken(encoded,cmdjson,NXTACCTSECRET);
    encoded[NXT_TOKEN_LEN] = 0;
    sprintf(tokenized,"[%s,{\"token\":\"%s\"}]",cmdjson,encoded);
    return((int32_t)strlen(tokenized)+1);
    // printf("(%s) -> (%s) _tokbuf.[%s]\n",NXTaddr,otherNXTaddr,_tokbuf);
}

int32_t issue_decodeToken(char *sender,int32_t *validp,char *key,unsigned char encoded[NXT_TOKEN_LEN])
{
    char cmd[4096],token[MAX_JSON_FIELD+2*NXT_TOKEN_LEN+1];
    cJSON *nxtobj,*validobj;
    union NXTtype retval;
    *validp = -1;
    sender[0] = 0;
    memcpy(token,encoded,NXT_TOKEN_LEN);
    token[NXT_TOKEN_LEN] = 0;
    sprintf(cmd,"%s=decodeToken&website=%s&token=%s",SUPERNET.NXTSERVER,key,token);
    //printf("cmd.(%s)\n",cmd);
    retval = extract_NXTfield(0,0,cmd,0,0);
    if ( retval.json != 0 )
    {
        validobj = cJSON_GetObjectItem(retval.json,"valid");
        if ( validobj != 0 )
            *validp = ((validobj->type&0xff) == cJSON_True) ? 1 : 0;
        nxtobj = cJSON_GetObjectItem(retval.json,"account");
        copy_cJSON(sender,nxtobj);
        free_json(retval.json);
        //printf("decoded valid.%d NXT.%s len.%d\n",*validp,sender,(int32_t)strlen(sender));
        if ( sender[0] != 0 )
            return((int32_t)strlen(sender));
        else return(0);
    }
    return(-1);
}

int32_t validate_token(char *pubkey,char *NXTaddr,char *tokenizedtxt,int32_t strictflag)
{
    cJSON *array=0,*firstitem=0,*tokenobj,*obj;
    int64_t timeval,diff = 0;
    int32_t valid,retcode = -13;
    char buf[MAX_JSON_FIELD],sender[MAX_JSON_FIELD],*firstjsontxt = 0;
    unsigned char encoded[4096];
    array = cJSON_Parse(tokenizedtxt);
    if ( array == 0 )
    {
        printf("couldnt validate.(%s)\n",tokenizedtxt);
        return(-2);
    }
    if ( is_cJSON_Array(array) != 0 && cJSON_GetArraySize(array) == 2 )
    {
        firstitem = cJSON_GetArrayItem(array,0);
        if ( pubkey != 0 )
        {
            obj = cJSON_GetObjectItem(firstitem,"pubkey");
            copy_cJSON(pubkey,obj);
        }
        obj = cJSON_GetObjectItem(firstitem,"NXT"), copy_cJSON(buf,obj);
        if ( NXTaddr[0] != 0 && strcmp(buf,NXTaddr) != 0 )
            retcode = -3;
        else
        {
            strcpy(NXTaddr,buf);
            if ( strictflag != 0 )
            {
                timeval = get_cJSON_int(firstitem,"time");
                diff = timeval - time(NULL);
                if ( diff < 0 )
                    diff = -diff;
                if ( diff > strictflag )
                {
                    printf("time diff %lld too big %lld vs %ld\n",(long long)diff,(long long)timeval,time(NULL));
                    retcode = -5;
                }
            }
            if ( retcode != -5 )
            {
                firstjsontxt = cJSON_Print(firstitem), _stripwhite(firstjsontxt,' ');
                tokenobj = cJSON_GetArrayItem(array,1);
                obj = cJSON_GetObjectItem(tokenobj,"token");
                copy_cJSON((char *)encoded,obj);
                memset(sender,0,sizeof(sender));
                valid = -1;
                if ( issue_decodeToken(sender,&valid,firstjsontxt,encoded) > 0 )
                {
                    if ( NXTaddr[0] == 0 )
                        strcpy(NXTaddr,sender);
                    if ( strcmp(sender,NXTaddr) == 0 )
                    {
                        printf("signed by valid NXT.%s valid.%d diff.%lld\n",sender,valid,(long long)diff);
                        retcode = valid;
                    }
                    else
                    {
                        printf("diff sender.(%s) vs NXTaddr.(%s)\n",sender,NXTaddr);
                        if ( strcmp(NXTaddr,buf) == 0 )
                            retcode = valid;
                    }
                }
                if ( retcode < 0 )
                    printf("err: signed by invalid sender.(%s) NXT.%s valid.%d or timediff too big diff.%lld, buf.(%s)\n",sender,NXTaddr,valid,(long long)diff,buf);
                free(firstjsontxt);
            }
        }
    }
    if ( array != 0 )
        free_json(array);
    return(retcode);
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

#endif
#endif

