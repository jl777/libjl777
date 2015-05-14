//
//  gen1auth.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_gen1auth_h
#define crypto777_gen1auth_h
#include <stdio.h>
#include "cJSON.h"
#include "coins777.c"
#include "cointx.c"


char *dumpprivkey(char *coinstr,char *serverport,char *userpass,char *coinaddr);
char *get_acct_coinaddr(char *coinaddr,char *coinstr,char *serverport,char *userpass,char *NXTaddr);
cJSON *_get_localaddresses(char *coinstr,char *serverport,char *userpass);
int32_t get_pubkey(char pubkey[512],char *coinstr,char *serverport,char *userpass,char *coinaddr);
char *sign_rawbytes(int32_t *completedp,char *signedbytes,int32_t max,char *coinstr,char *serverport,char *userpass,char *rawbytes);
struct cointx_info *createrawtransaction(char *coinstr,char *serverport,char *userpass,char *rawparams,struct cointx_info *cointx,int32_t opreturn,uint64_t redeemtxid,int32_t gatewayid,int32_t numgateways);
int32_t cosigntransaction(char **cointxidp,char **cosignedtxp,char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx,char *txbytes,int32_t gatewayid,int32_t numgateways);
int32_t generate_multisigaddr(char *multisigaddr,char *redeemScript,char *coinstr,char *serverport,char *userpass,int32_t addmultisig,char *params);
int32_t get_redeemscript(char *redeemScript,char *normaladdr,char *coinstr,char *serverport,char *userpass,char *multisigaddr);
char *get_msig_pubkeys(char *coinstr,char *serverport,char *userpass);


#endif
#else
#ifndef crypto777_gen1auth_c
#define crypto777_gen1auth_c

#ifndef crypto777_gen1auth_h
#define DEFINES_ONLY
#include "gen1auth.c"
#undef DEFINES_ONLY
#endif


char *dumpprivkey(char *coinstr,char *serverport,char *userpass,char *coinaddr)
{
    char args[1024];
    sprintf(args,"[\"%s\"]",coinaddr);
    return(bitcoind_passthru(coinstr,serverport,userpass,"dumpprivkey",args));
}

char *get_acct_coinaddr(char *coinaddr,char *coinstr,char *serverport,char *userpass,char *NXTaddr)
{
    char addr[128],*retstr;
    coinaddr[0] = 0;
    sprintf(addr,"\"%s\"",NXTaddr);
    retstr = bitcoind_passthru(coinstr,serverport,userpass,"getaccountaddress",addr);
    printf("get_acct_coinaddr.(%s) -> (%s)\n",NXTaddr,retstr);
    if ( retstr != 0 )
    {
        strcpy(coinaddr,retstr);
        free(retstr);
        return(coinaddr);
    }
    return(0);
}

int32_t get_pubkey(char pubkey[512],char *coinstr,char *serverport,char *userpass,char *coinaddr)
{
    char quotes[512],*retstr;
    int64_t len = 0;
    cJSON *json;
    if ( coinaddr[0] != '"' )
        sprintf(quotes,"\"%s\"",coinaddr);
    else safecopy(quotes,coinaddr,sizeof(quotes));
    if ( (retstr= bitcoind_passthru(coinstr,serverport,userpass,"validateaddress",quotes)) != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            copy_cJSON(pubkey,cJSON_GetObjectItem(json,"pubkey"));
            len = (int32_t)strlen(pubkey);
            free_json(json);
        }
        //printf("get_pubkey.(%s) -> (%s)\n",retstr,pubkey);
        free(retstr);
    }
    return((int32_t)len);
}

cJSON *msig_itemjson(char *account,char *coinaddr,char *pubkey,int32_t allfields)
{
    cJSON *item = cJSON_CreateObject();
    cJSON_AddItemToObject(item,"userNXT",cJSON_CreateString(account));
    cJSON_AddItemToObject(item,"coinaddr",cJSON_CreateString(coinaddr));
    cJSON_AddItemToObject(item,"pubkey",cJSON_CreateString(pubkey));
    if ( allfields != 0 && MGW.gatewayid >= 0 )
    {
        cJSON_AddItemToObject(item,"gatewayNXT",cJSON_CreateString(SUPERNET.NXTADDR));
        cJSON_AddItemToObject(item,"gatewayid",cJSON_CreateNumber(MGW.gatewayid));
    }
    return(item);
}

char *get_msig_pubkeys(char *coinstr,char *serverport,char *userpass)
{
    char pubkey[512],NXTaddr[64],account[512],coinaddr[512],*retstr = 0;
    cJSON *json,*item,*array = cJSON_CreateArray();
    uint64_t nxt64bits;
    int32_t i,n;
    if ( (retstr= bitcoind_passthru(coinstr,serverport,userpass,"listreceivedbyaddress","[1, true]")) != 0 )
    {
        //printf("listaccounts.(%s)\n",retstr);
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            if ( is_cJSON_Array(json) != 0 && (n= cJSON_GetArraySize(json)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    item = cJSON_GetArrayItem(json,i);
                    copy_cJSON(account,cJSON_GetObjectItem(item,"account"));
                    if ( is_decimalstr(account) > 0 )
                    {
                        nxt64bits = calc_nxt64bits(account);
                        expand_nxt64bits(NXTaddr,nxt64bits);
                        if ( strcmp(account,NXTaddr) == 0 )
                        {
                            copy_cJSON(coinaddr,cJSON_GetObjectItem(item,"address"));
                            if ( get_pubkey(pubkey,coinstr,serverport,userpass,coinaddr) != 0 )
                                cJSON_AddItemToArray(array,msig_itemjson(account,coinaddr,pubkey,1));
                        }
                        else printf("decimal.%d (%s) -> (%s)? ",is_decimalstr(account),account,NXTaddr);
                    }
                }
            }
            free_json(json);
        } else printf("couldnt parse.(%s)\n",retstr);
        free(retstr);
    }
    retstr = cJSON_Print(array);
    _stripwhite(retstr,' ');
    return(retstr);
}

cJSON *_get_localaddresses(char *coinstr,char *serverport,char *userpass)
{
    char *retstr;
    cJSON *json = 0;
    retstr = bitcoind_passthru(coinstr,serverport,userpass,"listaddressgroupings","");
    if ( retstr != 0 )
    {
        json = cJSON_Parse(retstr);
        free(retstr);
    }
    return(json);
}

char *sign_rawbytes(int32_t *completedp,char *signedbytes,int32_t max,char *coinstr,char *serverport,char *userpass,char *rawbytes)
{
    char *retstr = 0;
    cJSON *json,*hexobj,*compobj;
    if ( (retstr= bitcoind_passthru(coinstr,serverport,userpass,"signrawtransaction",rawbytes)) != 0 )
    {
        printf("got retstr.(%s)\n",retstr);
        json = cJSON_Parse(retstr);
        if ( json != 0 )
        {
            hexobj = cJSON_GetObjectItem(json,"hex");
            compobj = cJSON_GetObjectItem(json,"complete");
            if ( compobj != 0 )
                *completedp = ((compobj->type&0xff) == cJSON_True);
            copy_cJSON(signedbytes,hexobj);
            if ( strlen(signedbytes) > max )
                printf("sign_rawbytes: strlen(deststr) %ld > %d destize\n",strlen(signedbytes),max);
            free_json(json);
        } else printf("json parse error.(%s)\n",retstr);
    } else printf("error signing rawtx\n");
    return(retstr);
}

int32_t _sign_rawtransaction(char *deststr,unsigned long destsize,char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx,char *rawbytes,int32_t gatewayid,int32_t numgateways)
{
    int32_t completed = -1;
    char *retstr,*signparams;
    deststr[0] = 0;
    //printf("sign_rawtransaction rawbytes.(%s) %p\n",rawbytes,privkeys);
    if ( (signparams= _createsignraw_json_params(coinstr,serverport,userpass,cointx,rawbytes,0,gatewayid,numgateways)) != 0 )
    {
        _stripwhite(signparams,0);
        printf("got signparams.(%s)\n",signparams);
        if ( (retstr= sign_rawbytes(&completed,deststr,(int32_t)destsize,coinstr,serverport,userpass,signparams)) != 0 )
            free(retstr);
        free(signparams);
    } else printf("error generating signparams\n");
    return(completed);
}

char *_sign_localtx(char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx,char *rawbytes,int32_t gatewayid,int32_t numgateways)
{
    char *batchsigned;
    cointx->batchsize = (uint32_t)strlen(rawbytes) + 1;
    cointx->batchcrc = _crc32(0,rawbytes+12,cointx->batchsize-12); // skip past timediff
    batchsigned = malloc(cointx->batchsize + cointx->numinputs*512 + 512);
    _sign_rawtransaction(batchsigned,cointx->batchsize + cointx->numinputs*512 + 512,coinstr,serverport,userpass,cointx,rawbytes,gatewayid,numgateways);
    return(batchsigned);
}

int32_t cosigntransaction(char **cointxidp,char **cosignedtxp,char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx,char *txbytes,int32_t gatewayid,int32_t numgateways)
{
    char *signed2transaction;
    int32_t completed,len;
    len = (int32_t)strlen(txbytes);
    fprintf(stderr,"submit_withdraw.(%s) len.%d sizeof cointx.%ld\n",txbytes,len,sizeof(cointx));
    *cosignedtxp = *cointxidp = 0;
    signed2transaction = calloc(1,2*len);
    if ( (completed= _sign_rawtransaction(signed2transaction+2,len+4000,coinstr,serverport,userpass,cointx,txbytes,gatewayid,numgateways)) > 0 )
    {
        signed2transaction[0] = '[';
        signed2transaction[1] = '"';
        strcat(signed2transaction,"\"]");
        //printf("sign2.(%s)\n",signed2transaction);
        *cointxidp = bitcoind_passthru(coinstr,serverport,userpass,"sendrawtransaction",signed2transaction);
        *cosignedtxp = signed2transaction;
    }
    return(completed);
}

struct cointx_info *createrawtransaction(char *coinstr,char *serverport,char *userpass,char *rawparams,struct cointx_info *cointx,int32_t opreturn,uint64_t redeemtxid,int32_t gatewayid,int32_t numgateways)
{
    struct cointx_info *rettx = 0;
    char *txbytes,*signedtx,*txbytes2;
    int32_t allocsize,isBTC;
    if ( (txbytes= bitcoind_passthru(coinstr,serverport,userpass,"createrawtransaction",rawparams)) != 0 )
    {
        fprintf(stderr,"len.%ld calc_rawtransaction retstr.(%s)\n",strlen(txbytes),txbytes);
        if ( opreturn >= 0 )
        {
            isBTC = (strcmp("BTC",coinstr) == 0);
            if ( (txbytes2= _insert_OP_RETURN(txbytes,isBTC,opreturn,&redeemtxid,1,isBTC)) == 0 )
            {
                fprintf(stderr,"error replacing with OP_RETURN.%s txout.%d (%s)\n",coinstr,opreturn,txbytes);
                free(txbytes);
                return(0);
            }
            free(txbytes);
            txbytes = txbytes2;
        }
        if ( (signedtx= _sign_localtx(coinstr,serverport,userpass,cointx,txbytes,gatewayid,numgateways)) != 0 )
        {
            allocsize = (int32_t)(sizeof(*rettx) + strlen(signedtx) + 1);
            // printf("signedtx returns.(%s) allocsize.%d\n",signedtx,allocsize);
            rettx = calloc(1,allocsize);
            *rettx = *cointx;
            rettx->allocsize = allocsize;
            rettx->isallocated = allocsize;
            strcpy(rettx->signedtx,signedtx);
            free(signedtx);
            cointx = 0;
        } else fprintf(stderr,"error _sign_localtx.(%s)\n",txbytes);
        free(txbytes);
    } else fprintf(stderr,"error creating rawtransaction\n");
    return(rettx);
}

#endif
#endif
