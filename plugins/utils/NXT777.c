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
#include "bits777.c"
#include "utils777.c"
#include "system777.c"

#include "tweetnacl.h"
int curve25519_donna(uint8_t *, const uint8_t *, const uint8_t *);
#define NXT_ASSETID ('N' + ((uint64_t)'X'<<8) + ((uint64_t)'T'<<16))    // 5527630

#define NXT_ASSETLIST_INCR 16
#define MAX_COINTXID_LEN 128
#define MAX_COINADDR_LEN 128
#define MAX_NXT_STRLEN 24
#define MAX_NXTTXID_LEN MAX_NXT_STRLEN
#define MAX_NXTADDR_LEN MAX_NXT_STRLEN

#define GENESISACCT "1739068987193023818"  // NXT-MRCC-2YLS-8M54-3CMAJ
#define GENESISBLOCK "2680262203532249785"
#define DEFAULT_NXT_DEADLINE 720
//#define NXTSERVER "https://127.0.0.1:7876/nxt?requestType"
#define _NXTSERVER "requestType"
#define issue_curl(curl_handle,cmdstr) bitcoind_RPC(curl_handle,"curl",cmdstr,0,0,0)
#define issue_NXT(curl_handle,cmdstr) bitcoind_RPC(curl_handle,"NXT",cmdstr,0,0,0)
#define issue_NXTPOST(curl_handle,cmdstr) bitcoind_RPC(curl_handle,"curl",SUPERNET.NXTAPIURL,0,0,cmdstr)
#define fetch_URL(url) bitcoind_RPC(0,"fetch",url,0,0,0)
#define _issue_NXTPOST(cmdstr) bitcoind_RPC(0,"curl",SUPERNET.NXTAPIURL,0,0,cmdstr)
#define _issue_curl(cmdstr) bitcoind_RPC(0,"curl",cmdstr,0,0,0)

union _NXT_str_buf { char txid[24]; char NXTaddr[24];  char assetid[24]; };
struct NXT_str { uint64_t modified,nxt64bits; union _NXT_str_buf U; };
union _asset_price { uint64_t assetoshis,price; };

struct NXT_assettxid
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
    struct NXT_assettxid **txids;   // all transactions for this asset
    int32_t max,num,decimals;
    uint16_t type,subtype;
};
struct NXT_AMhdr { uint32_t sig; int32_t size; uint64_t nxt64bits; };
struct compressed_json { uint32_t complen,sublen,origlen,jsonlen; unsigned char encoded[128]; };
union _json_AM_data { unsigned char binarydata[sizeof(struct compressed_json)]; char jsonstr[sizeof(struct compressed_json)]; struct compressed_json jsn; };
struct json_AM { struct NXT_AMhdr H; uint32_t funcid,gatewayid,timestamp,jsonflag; union _json_AM_data U; };

struct NXT_assettxid *find_NXT_assettxid(int32_t *createdflagp,struct NXT_asset *ap,char *txid);

uint64_t conv_NXTpassword(unsigned char *mysecret,unsigned char *mypublic,uint8_t *pass,int32_t passlen);
bits256 calc_sharedsecret(uint64_t *nxt64bitsp,int32_t *haspubpeyp,uint8_t *NXTACCTSECRET,int32_t secretlen,uint64_t other64bits);
int32_t curve25519_donna(uint8_t *mypublic,const uint8_t *secret,const uint8_t *basepoint);
uint64_t ram_verify_NXTtxstillthere(uint64_t ap_mult,uint64_t txidbits);
int32_t _in_specialNXTaddrs(char **specialNXTaddrs,int32_t n,char *NXTaddr);
uint32_t _get_NXTheight(uint32_t *firsttimep);
char *_issue_getAsset(char *assetidstr);
uint64_t _get_NXT_ECblock(uint32_t *ecblockp);
char *_issue_getTransaction(char *txidstr);
bits256 issue_getpubkey(int32_t *haspubkeyp,char *acct);
uint64_t conv_rsacctstr(char *rsacctstr,uint64_t nxt64bits);
uint64_t issue_transferAsset(char **retstrp,void *deprecated,char *secret,char *recipient,char *asset,int64_t quantity,int64_t feeNQT,int32_t deadline,char *comment,char *destpubkey);
uint64_t get_sender(uint64_t *amountp,char *txidstr);
uint64_t conv_acctstr(char *acctstr);
int32_t gen_randomacct(uint32_t randchars,char *NXTaddr,char *NXTsecret,char *randfilename);


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
    jsonstr = issue_curl(0,cmd);
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
    jsonstr = issue_curl(0,cmd);
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

int32_t _in_specialNXTaddrs(char **specialNXTaddrs,int32_t n,char *NXTaddr)
{
    int32_t i;
    for (i=0; i<n; i++)
        if ( strcmp(specialNXTaddrs[i],NXTaddr) == 0 )
            return(1);
    return(0);
}

char *_issue_getAsset(char *assetidstr)
{
    char cmd[4096];
    //sprintf(cmd,"requestType=getAsset&asset=%s",assetidstr);
    sprintf(cmd,"%s=getAsset&asset=%s",SUPERNET.NXTSERVER,assetidstr);
    printf("_cmd.(%s)\n",cmd);
    return(_issue_curl(cmd));
}

uint32_t _get_NXTheight(uint32_t *firsttimep)
{
    cJSON *json;
    uint32_t height = 0;
    char cmd[256],*jsonstr;
    sprintf(cmd,"requestType=getState");
    if ( (jsonstr= _issue_NXTPOST(cmd)) != 0 )
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
    if ( (jsonstr= _issue_NXTPOST(cmd)) != 0 )
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
    return(_issue_NXTPOST(cmd));
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
        jsonstr = issue_curl(0,cmd);
    }
    else if ( rsacctstr[0] != 0 )
    {
        sprintf(cmd,"%s=rsConvert&account=%s",SUPERNET.NXTSERVER,rsacctstr);
        jsonstr = issue_curl(0,cmd);
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
        sprintf(cmd,"%s=sendMoney&amountNQT=%lld",_NXTSERVER,(long long)quantity);
    else sprintf(cmd,"%s=transferAsset&asset=%s&quantityQNT=%lld",_NXTSERVER,asset,(long long)quantity);
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
    jsontxt = issue_NXTPOST(0,cmd);
    if ( jsontxt != 0 )
    {
        printf(" transferAsset.(%s) -> %s\n",cmd,jsontxt);
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

#endif
#endif

