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
#include "../includes/cJSON.h"
#include "coins777.c"
#include "cointx.c"


char *dumpprivkey(char *coinstr,char *serverport,char *userpass,char *coinaddr);
char *get_acct_coinaddr(char *coinaddr,char *coinstr,char *serverport,char *userpass,char *NXTaddr);
cJSON *_get_localaddresses(char *coinstr,char *serverport,char *userpass);
int32_t get_pubkey(char pubkey[512],char *coinstr,char *serverport,char *userpass,char *coinaddr);
//char *sign_rawbytes(int32_t *completedp,char *signedbytes,int32_t max,char *coinstr,char *serverport,char *userpass,char *rawbytes);
//struct cointx_info *createrawtransaction(char *coinstr,char *serverport,char *userpass,char *rawparams,struct cointx_info *cointx,int32_t opreturn,uint64_t redeemtxid,int32_t gatewayid,int32_t numgateways);
//int32_t cosigntransaction(char **cointxidp,char **cosignedtxp,char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx,char *txbytes,int32_t gatewayid,int32_t numgateways);
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

#endif
#endif
