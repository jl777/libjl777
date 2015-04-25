//
//  msig.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_msig_h
#define crypto777_msig_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "uthash.h"
#include "cJSON.h"
#include "NXT777.c"
#include "bitcoind.c"
#include "system777.c"
#include "storage.c"
//#include "ramchain.c"

//struct storage_header { uint32_t size,createtime; uint64_t keyhash; };
struct pubkey_info { uint64_t nxt64bits; uint32_t ipbits; char pubkey[256],coinaddr[128]; };
struct multisig_addr
{
    //struct storage_header H;
    UT_hash_handle hh;
    char NXTaddr[MAX_NXTADDR_LEN],multisigaddr[MAX_COINADDR_LEN],NXTpubkey[96],redeemScript[2048],coinstr[16],email[128];
    uint64_t sender,modified;
    int32_t size,m,n,created,valid,buyNXT;
    struct pubkey_info pubkeys[];
};

int32_t update_msig_info(struct multisig_addr *msig,int32_t syncflag,char *sender);
int32_t update_MGWaddr(cJSON *argjson,char *sender);
int32_t add_MGWaddr(char *previpaddr,char *sender,int32_t valid,char *origargstr);
int32_t msigcmp(struct multisig_addr *ref,struct multisig_addr *msig);
struct multisig_addr *decode_msigjson(char *NXTaddr,cJSON *obj,char *sender);
char *create_multisig_json(struct multisig_addr *msig,int32_t truncated);
struct multisig_addr *gen_multisig_addr(char *sender,int32_t M,int32_t N,char *coinstr,char *serverport,char *userpass,int32_t use_addmultisig,char *refNXTaddr,char *userpubkey,uint64_t *srvbits);
void *extract_jsonmsig(cJSON *item,void *arg,void *arg2);
int32_t update_MGW_msig(struct multisig_addr *msig,char *sender);

int32_t jsonmsigcmp(void *ref,void *item);
int32_t jsonstrcmp(void *ref,void *item);

extern int32_t Debuglevel,Gatewayid,MGW_initdone,Numgateways;

#endif
#else
#ifndef crypto777_msig_c
#define crypto777_msig_c

#ifndef crypto777_msig_h
#define DEFINES_ONLY
#include "msig.c"
#undef DEFINES_ONLY
#endif

#define BTC_COINID 1
#define LTC_COINID 2
#define DOGE_COINID 4
#define BTCD_COINID 8


cJSON *http_search(char *destip,char *type,char *file)
{
    cJSON *json = 0;
    char url[1024],*retstr;
    sprintf(url,"http://%s/%s/%s",destip,type,file);
    if ( (retstr= issue_curl(0,url)) != 0 )
    {
        json = cJSON_Parse(retstr);
        free(retstr);
    }
    return(json);
}

struct multisig_addr *http_search_msig(char *external_NXTaddr,char *external_ipaddr,char *NXTaddr)
{
    int32_t i,n,len;
    cJSON *array;
    struct multisig_addr *msig = 0;
    if ( (array= http_search(external_ipaddr,"MGW/msig",NXTaddr)) != 0 )
    {
        if ( is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
        {
            for (i=0; i<n; i++)
                if ( (msig= decode_msigjson(0,cJSON_GetArrayItem(array,i),external_NXTaddr)) != 0 && (msig= find_msigaddr(&len,msig->multisigaddr)) != 0 )
                    break;
        }
        free_json(array);
    }
    return(msig);
}

void *extract_jsonkey(cJSON *item,void *arg,void *arg2)
{
    char *redeemstr = calloc(1,MAX_JSON_FIELD);
    copy_cJSON(redeemstr,cJSON_GetObjectItem(item,arg));
    return(redeemstr);
}

void *extract_jsonints(cJSON *item,void *arg,void *arg2)
{
    char argstr[MAX_JSON_FIELD],*keystr;
    cJSON *obj0=0,*obj1=0;
    if ( arg != 0 )
        obj0 = cJSON_GetObjectItem(item,arg);
    if ( arg2 != 0 )
        obj1 = cJSON_GetObjectItem(item,arg2);
    if ( obj0 != 0 && obj1 != 0 )
    {
        sprintf(argstr,"%llu.%llu",(long long)get_API_int(obj0,0),(long long)get_API_int(obj1,0));
        keystr = calloc(1,strlen(argstr)+1);
        strcpy(keystr,argstr);
        return(keystr);
    } else return(0);
}

void *extract_jsonmsig(cJSON *item,void *arg,void *arg2)
{
    char sender[MAX_JSON_FIELD];
    copy_cJSON(sender,cJSON_GetObjectItem(item,"sender"));
    return(decode_msigjson(0,item,sender));
}

int32_t jsonmsigcmp(void *ref,void *item) { return(msigcmp(ref,item)); }
int32_t jsonstrcmp(void *ref,void *item) { return(strcmp(ref,item)); }

void set_legacy_coinid(char *coinstr,int32_t legacyid)
{
    switch ( legacyid )
    {
        case BTC_COINID: strcpy(coinstr,"BTC"); return;
        case LTC_COINID: strcpy(coinstr,"LTC"); return;
        case DOGE_COINID: strcpy(coinstr,"DOGE"); return;
        case BTCD_COINID: strcpy(coinstr,"BTCD"); return;
    }
}

long calc_pubkey_jsontxt(int32_t truncated,char *jsontxt,struct pubkey_info *ptr,char *postfix)
{
    if ( truncated != 0 )
        sprintf(jsontxt,"{\"address\":\"%s\"}%s",ptr->coinaddr,postfix);
    else sprintf(jsontxt,"{\"address\":\"%s\",\"pubkey\":\"%s\",\"srv\":\"%llu\"}%s",ptr->coinaddr,ptr->pubkey,(long long)ptr->nxt64bits,postfix);
    return(strlen(jsontxt));
}

char *create_multisig_json(struct multisig_addr *msig,int32_t truncated)
{
    long i,len = 0;
    char jsontxt[65536],pubkeyjsontxt[65536],rsacct[64];
    if ( msig != 0 )
    {
        rsacct[0] = 0;
        conv_rsacctstr(rsacct,calc_nxt64bits(msig->NXTaddr));
        pubkeyjsontxt[0] = 0;
        for (i=0; i<msig->n; i++)
            len += calc_pubkey_jsontxt(truncated,pubkeyjsontxt+strlen(pubkeyjsontxt),&msig->pubkeys[i],(i<(msig->n - 1)) ? ", " : "");
        sprintf(jsontxt,"{%s\"sender\":\"%llu\",\"buyNXT\":%u,\"created\":%u,\"M\":%d,\"N\":%d,\"NXTaddr\":\"%s\",\"NXTpubkey\":\"%s\",\"RS\":\"%s\",\"address\":\"%s\",\"redeemScript\":\"%s\",\"coin\":\"%s\",\"gatewayid\":\"%d\",\"pubkey\":[%s]}",truncated==0?"\"requestType\":\"MGWaddr\",":"",(long long)msig->sender,msig->buyNXT,msig->created,msig->m,msig->n,msig->NXTaddr,msig->NXTpubkey,rsacct,msig->multisigaddr,msig->redeemScript,msig->coinstr,Gatewayid,pubkeyjsontxt);
        //if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
        //    printf("(%s) pubkeys len.%ld msigjsonlen.%ld\n",jsontxt,len,strlen(jsontxt));
        return(clonestr(jsontxt));
    }
    else return(0);
}

struct multisig_addr *alloc_multisig_addr(char *coinstr,int32_t m,int32_t n,char *NXTaddr,char *userpubkey,char *sender)
{
    struct multisig_addr *msig;
    int32_t size = (int32_t)(sizeof(*msig) + n*sizeof(struct pubkey_info));
    msig = calloc(1,size);
    msig->size = size;
    msig->n = n;
    msig->created = (uint32_t)time(NULL);
    if ( sender != 0 && sender[0] != 0 )
        msig->sender = calc_nxt64bits(sender);
    safecopy(msig->coinstr,coinstr,sizeof(msig->coinstr));
    safecopy(msig->NXTaddr,NXTaddr,sizeof(msig->NXTaddr));
    safecopy(msig->NXTpubkey,userpubkey,sizeof(msig->NXTpubkey));
    msig->m = m;
    return(msig);
}

struct multisig_addr *decode_msigjson(char *NXTaddr,cJSON *obj,char *sender)
{
    int32_t j,M,n;
    char nxtstr[512],coinstr[64],ipaddr[64],numstr[64],NXTpubkey[128];
    struct multisig_addr *msig = 0;
    cJSON *pobj,*redeemobj,*pubkeysobj,*addrobj,*nxtobj,*nameobj,*idobj;
    if ( obj == 0 )
    {
        printf("decode_msigjson cant decode null obj\n");
        return(0);
    }
    nameobj = cJSON_GetObjectItem(obj,"coin");
    copy_cJSON(coinstr,nameobj);
    if ( coinstr[0] == 0 )
    {
        if ( (idobj = cJSON_GetObjectItem(obj,"coinid")) != 0 )
        {
            copy_cJSON(numstr,idobj);
            if ( numstr[0] != 0 )
                set_legacy_coinid(coinstr,atoi(numstr));
        }
    }
    if ( coinstr[0] != 0 )
    {
        addrobj = cJSON_GetObjectItem(obj,"address");
        redeemobj = cJSON_GetObjectItem(obj,"redeemScript");
        pubkeysobj = cJSON_GetObjectItem(obj,"pubkey");
        nxtobj = cJSON_GetObjectItem(obj,"NXTaddr");
        if ( nxtobj != 0 )
        {
            copy_cJSON(nxtstr,nxtobj);
            if ( NXTaddr != 0 && strcmp(nxtstr,NXTaddr) != 0 )
                printf("WARNING: mismatched NXTaddr.%s vs %s\n",nxtstr,NXTaddr);
        }
        //printf("msig.%p %p %p %p\n",msig,addrobj,redeemobj,pubkeysobj);
        if ( nxtstr[0] != 0 && addrobj != 0 && redeemobj != 0 && pubkeysobj != 0 )
        {
            n = cJSON_GetArraySize(pubkeysobj);
            M = (int32_t)get_API_int(cJSON_GetObjectItem(obj,"M"),n-1);
            copy_cJSON(NXTpubkey,cJSON_GetObjectItem(obj,"NXTpubkey"));
            if ( NXTpubkey[0] == 0 )
                set_NXTpubkey(NXTpubkey,nxtstr);
            msig = alloc_multisig_addr(coinstr,M,n,nxtstr,NXTpubkey,sender);
            safecopy(msig->coinstr,coinstr,sizeof(msig->coinstr));
            copy_cJSON(msig->redeemScript,redeemobj);
            copy_cJSON(msig->multisigaddr,addrobj);
            msig->buyNXT = (uint32_t)get_API_int(cJSON_GetObjectItem(obj,"buyNXT"),10);
            for (j=0; j<n; j++)
            {
                pobj = cJSON_GetArrayItem(pubkeysobj,j);
                if ( pobj != 0 )
                {
                    copy_cJSON(msig->pubkeys[j].coinaddr,cJSON_GetObjectItem(pobj,"address"));
                    copy_cJSON(msig->pubkeys[j].pubkey,cJSON_GetObjectItem(pobj,"pubkey"));
                    msig->pubkeys[j].nxt64bits = get_API_nxt64bits(cJSON_GetObjectItem(pobj,"srv"));
                    copy_cJSON(ipaddr,cJSON_GetObjectItem(pobj,"ipaddr"));
                    if ( Debuglevel > 2 )
                        fprintf(stderr,"{(%s) (%s) %llu ip.(%s)}.%d ",msig->pubkeys[j].coinaddr,msig->pubkeys[j].pubkey,(long long)msig->pubkeys[j].nxt64bits,ipaddr,j);
                    //if ( ipaddr[0] == 0 && j < 3 )
                     //   strcpy(ipaddr,Server_ipaddrs[j]);
                    //msig->pubkeys[j].ipbits = calc_ipbits(ipaddr);
                } else { free(msig); msig = 0; }
            }
            //printf("NXT.%s -> (%s)\n",nxtstr,msig->multisigaddr);
            if ( Debuglevel > 3 )
                fprintf(stderr,"for msig.%s\n",msig->multisigaddr);
        } else { printf("%p %p %p\n",addrobj,redeemobj,pubkeysobj); free(msig); msig = 0; }
        //printf("return msig.%p\n",msig);
        return(msig);
    } else fprintf(stderr,"decode msig:  error parsing.(%s)\n",cJSON_Print(obj));
    return(0);
}

char *createmultisig_json_params(struct multisig_addr *msig,char *acctparm)
{
    int32_t i;
    char *paramstr = 0;
    cJSON *array,*mobj,*keys,*key;
    keys = cJSON_CreateArray();
    for (i=0; i<msig->n; i++)
    {
        key = cJSON_CreateString(msig->pubkeys[i].pubkey);
        cJSON_AddItemToArray(keys,key);
    }
    mobj = cJSON_CreateNumber(msig->m);
    array = cJSON_CreateArray();
    if ( array != 0 )
    {
        cJSON_AddItemToArray(array,mobj);
        cJSON_AddItemToArray(array,keys);
        if ( acctparm != 0 )
            cJSON_AddItemToArray(array,cJSON_CreateString(acctparm));
        paramstr = cJSON_Print(array);
        free_json(array);
    }
    //printf("createmultisig_json_params.%s\n",paramstr);
    return(paramstr);
}

int32_t issue_createmultisig(char *coinstr,char *serverport,char *userpass,int32_t use_addmultisig,struct multisig_addr *msig)
{
    int32_t flag = 0;
    char addr[256];
    cJSON *json,*msigobj,*redeemobj;
    char *params,*retstr = 0;
    params = createmultisig_json_params(msig,(use_addmultisig != 0) ? msig->NXTaddr : 0);
    flag = 0;
    if ( params != 0 )
    {
        if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
            printf("multisig params.(%s)\n",params);
        if ( use_addmultisig != 0 )
        {
            retstr = bitcoind_RPC(0,coinstr,serverport,userpass,"addmultisigaddress",params);
            if ( retstr != 0 )
            {
                strcpy(msig->multisigaddr,retstr);
                free(retstr);
                sprintf(addr,"\"%s\"",msig->multisigaddr);
                retstr = bitcoind_RPC(0,coinstr,serverport,userpass,"validateaddress",addr);
                if ( retstr != 0 )
                {
                    json = cJSON_Parse(retstr);
                    if ( json == 0 ) printf("Error before: [%s]\n",cJSON_GetErrorPtr());
                    else
                    {
                        if ( (redeemobj= cJSON_GetObjectItem(json,"hex")) != 0 )
                        {
                            copy_cJSON(msig->redeemScript,redeemobj);
                            flag = 1;
                        } else printf("missing redeemScript in (%s)\n",retstr);
                        free_json(json);
                    }
                    free(retstr);
                }
            } else printf("error creating multisig address\n");
        }
        else
        {
            retstr = bitcoind_RPC(0,coinstr,serverport,userpass,"createmultisig",params);
            if ( retstr != 0 )
            {
                json = cJSON_Parse(retstr);
                if ( json == 0 ) printf("Error before: [%s]\n",cJSON_GetErrorPtr());
                else
                {
                    if ( (msigobj= cJSON_GetObjectItem(json,"address")) != 0 )
                    {
                        if ( (redeemobj= cJSON_GetObjectItem(json,"redeemScript")) != 0 )
                        {
                            copy_cJSON(msig->multisigaddr,msigobj);
                            copy_cJSON(msig->redeemScript,redeemobj);
                            flag = 1;
                        } else printf("missing redeemScript in (%s)\n",retstr);
                    } else printf("multisig missing address in (%s) params.(%s)\n",retstr,params);
                    if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
                        printf("addmultisig.(%s)\n",retstr);
                    free_json(json);
                }
                free(retstr);
            } else printf("error issuing createmultisig.(%s)\n",params);
        }
        free(params);
    } else printf("error generating msig params\n");
    return(flag);
}

struct multisig_addr *finalize_msig(struct multisig_addr *msig,uint64_t *srvbits,uint64_t refbits)
{
    int32_t i,n;
    char acctcoinaddr[1024],pubkey[1024];
    for (i=n=0; i<msig->n; i++)
    {
        if ( srvbits[i] != 0 && refbits != 0 )
        {
            acctcoinaddr[0] = pubkey[0] = 0;
            if ( get_NXT_coininfo(srvbits[i],acctcoinaddr,pubkey,refbits,msig->coinstr) != 0 && acctcoinaddr[0] != 0 && pubkey[0] != 0 )
            {
                strcpy(msig->pubkeys[i].coinaddr,acctcoinaddr);
                strcpy(msig->pubkeys[i].pubkey,pubkey);
                msig->pubkeys[i].nxt64bits = srvbits[i];
                n++;
            }
        }
    }
    if ( n != msig->n )
        free(msig), msig = 0;
    return(msig);
}

struct multisig_addr *gen_multisig_addr(char *sender,int32_t M,int32_t N,char *coinstr,char *serverport,char *userpass,int32_t use_addmultisig,char *refNXTaddr,char *userpubkey,uint64_t *srvbits)
{
    uint64_t refbits;//,srvbits[16];
    int32_t flag = 0;
    struct multisig_addr *msig;
    refbits = calc_nxt64bits(refNXTaddr);
    msig = alloc_multisig_addr(coinstr,M,N,refNXTaddr,userpubkey,sender);
    //for (i=0; i<N; i++)
    //    srvbits[i] = contacts[i]->nxt64bits;
    if ( (msig= finalize_msig(msig,srvbits,refbits)) != 0 )
        flag = issue_createmultisig(coinstr,serverport,userpass,use_addmultisig,msig);
    if ( flag == 0 )
    {
        free(msig);
        return(0);
    }
    return(msig);
}

int32_t pubkeycmp(struct pubkey_info *ref,struct pubkey_info *cmp)
{
    if ( strcmp(ref->pubkey,cmp->pubkey) != 0 )
        return(1);
    if ( strcmp(ref->coinaddr,cmp->coinaddr) != 0 )
        return(2);
    if ( ref->nxt64bits != cmp->nxt64bits )
        return(3);
    return(0);
}

int32_t msigcmp(struct multisig_addr *ref,struct multisig_addr *msig)
{
    int32_t i,x;
    if ( ref == 0 )
        return(-1);
    if ( strcmp(ref->multisigaddr,msig->multisigaddr) != 0 || msig->m != ref->m || msig->n != ref->n )
    {
        if ( Debuglevel > 3 )
            printf("A ref.(%s) vs msig.(%s)\n",ref->multisigaddr,msig->multisigaddr);
        return(1);
    }
    if ( strcmp(ref->NXTaddr,msig->NXTaddr) != 0 )
    {
        if ( Debuglevel > 3 )
            printf("B ref.(%s) vs msig.(%s)\n",ref->NXTaddr,msig->NXTaddr);
        return(2);
    }
    if ( strcmp(ref->redeemScript,msig->redeemScript) != 0 )
    {
        if ( Debuglevel > 3 )
            printf("C ref.(%s) vs msig.(%s)\n",ref->redeemScript,msig->redeemScript);
        return(3);
    }
    for (i=0; i<ref->n; i++)
        if ( (x= pubkeycmp(&ref->pubkeys[i],&msig->pubkeys[i])) != 0 )
        {
            if ( Debuglevel > 3 )
            {
                switch ( x )
                {
                    case 1: printf("P.%d pubkey ref.(%s) vs msig.(%s)\n",x,ref->pubkeys[i].pubkey,msig->pubkeys[i].pubkey); break;
                    case 2: printf("P.%d pubkey ref.(%s) vs msig.(%s)\n",x,ref->pubkeys[i].coinaddr,msig->pubkeys[i].coinaddr); break;
                    case 3: printf("P.%d pubkey ref.(%llu) vs msig.(%llu)\n",x,(long long)ref->pubkeys[i].nxt64bits,(long long)msig->pubkeys[i].nxt64bits); break;
                    default: printf("unexpected retval.%d\n",x);
                }
            }
            return(4+i);
        }
    return(0);
}

int32_t _map_msigaddr(char *redeemScript,char *coinstr,char *serverport,char *userpass,char *normaladdr,char *msigaddr) //could map to rawind, but this is rarely called
{
    int32_t i,n,ismine,len;
    cJSON *json,*array,*json2;
    struct multisig_addr *msig;
    char addr[1024],args[1024],*retstr,*retstr2;
    redeemScript[0] = normaladdr[0] = 0;
    if ( (msig= find_msigaddr(&len,msigaddr)) == 0 )
    {
        strcpy(normaladdr,msigaddr);
        printf("cant find_msigaddr.(%s)\n",msigaddr);
        return(0);
    }
    if ( msig->redeemScript[0] != 0 && Gatewayid >= 0 && Gatewayid < Numgateways )
    {
        strcpy(normaladdr,msig->pubkeys[Gatewayid].coinaddr);
        strcpy(redeemScript,msig->redeemScript);
        printf("_map_msigaddr.(%s) -> return (%s) redeem.(%s)\n",msigaddr,normaladdr,redeemScript);
        return(1);
    }
    sprintf(args,"\"%s\"",msig->multisigaddr);
    retstr = bitcoind_RPC(0,coinstr,serverport,userpass,"validateaddress",args);
    if ( retstr != 0 )
    {
        printf("got retstr.(%s)\n",retstr);
        if ( (json = cJSON_Parse(retstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"addresses")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    ismine = 0;
                    copy_cJSON(addr,cJSON_GetArrayItem(array,i));
                    if ( addr[0] != 0 )
                    {
                        sprintf(args,"\"%s\"",addr);
                        retstr2 = bitcoind_RPC(0,coinstr,serverport,userpass,"validateaddress",args);
                        if ( retstr2 != 0 )
                        {
                            if ( (json2 = cJSON_Parse(retstr2)) != 0 )
                                ismine = is_cJSON_True(cJSON_GetObjectItem(json2,"ismine"));
                            free(retstr2);
                        }
                    }
                    if ( ismine != 0 )
                    {
                        //printf("(%s) ismine.%d\n",addr,ismine);
                        strcpy(normaladdr,addr);
                        copy_cJSON(redeemScript,cJSON_GetObjectItem(json,"hex"));
                        break;
                    }
                }
            } free_json(json);
        } free(retstr);
    }
    if ( normaladdr[0] != 0 )
        return(1);
    strcpy(normaladdr,msigaddr);
    return(-1);
}

cJSON *_create_privkeys_json_params(char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx,char **privkeys,int32_t numinputs)
{
    int32_t allocflag,i,ret,nonz = 0;
    cJSON *array;
    char args[1024],normaladdr[1024],redeemScript[4096];
    //printf("create privkeys %p numinputs.%d\n",privkeys,numinputs);
    if ( privkeys == 0 )
    {
        privkeys = calloc(numinputs,sizeof(*privkeys));
        for (i=0; i<numinputs; i++)
        {
            if ( (ret= _map_msigaddr(redeemScript,coinstr,serverport,userpass,normaladdr,cointx->inputs[i].coinaddr)) >= 0 )
            {
                sprintf(args,"[\"%s\"]",normaladdr);
                //fprintf(stderr,"(%s) -> (%s).%d ",normaladdr,normaladdr,i);
                privkeys[i] = bitcoind_RPC(0,coinstr,serverport,userpass,"dumpprivkey",args);
            } else fprintf(stderr,"ret.%d for %d (%s)\n",ret,i,normaladdr);
        }
        allocflag = 1;
        //fprintf(stderr,"allocated\n");
    } else allocflag = 0;
    array = cJSON_CreateArray();
    for (i=0; i<numinputs; i++)
    {
        if ( privkeys[i] != 0 )
        {
            nonz++;
            //printf("(%s %s) ",privkeys[i],cointx->inputs[i].coinaddr);
            cJSON_AddItemToArray(array,cJSON_CreateString(privkeys[i]));
        }
    }
    if ( nonz == 0 )
        free_json(array), array = 0;
    // else printf("privkeys.%d of %d: %s\n",nonz,numinputs,cJSON_Print(array));
    if ( allocflag != 0 )
    {
        for (i=0; i<numinputs; i++)
            if ( privkeys[i] != 0 )
                free(privkeys[i]);
        free(privkeys);
    }
    return(array);
}

cJSON *_create_vins_json_params(char **localcoinaddrs,char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx)
{
    int32_t i,ret;
    char normaladdr[1024],redeemScript[4096];
    cJSON *json,*array;
    struct cointx_input *vin;
    array = cJSON_CreateArray();
    for (i=0; i<cointx->numinputs; i++)
    {
        vin = &cointx->inputs[i];
        if ( localcoinaddrs != 0 )
            localcoinaddrs[i] = 0;
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json,"txid",cJSON_CreateString(vin->tx.txidstr));
        cJSON_AddItemToObject(json,"vout",cJSON_CreateNumber(vin->tx.vout));
        cJSON_AddItemToObject(json,"scriptPubKey",cJSON_CreateString(vin->sigs));
        if ( (ret= _map_msigaddr(redeemScript,coinstr,serverport,userpass,normaladdr,vin->coinaddr)) >= 0 )
            cJSON_AddItemToObject(json,"redeemScript",cJSON_CreateString(redeemScript));
        else printf("ret.%d redeemScript.(%s) (%s) for (%s)\n",ret,redeemScript,normaladdr,vin->coinaddr);
        if ( localcoinaddrs != 0 )
            localcoinaddrs[i] = vin->coinaddr;
        cJSON_AddItemToArray(array,json);
    }
    return(array);
}

char *_createsignraw_json_params(char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx,char *rawbytes,char **privkeys)
{
    char *paramstr = 0;
    cJSON *array,*rawobj,*vinsobj=0,*keysobj=0;
    rawobj = cJSON_CreateString(rawbytes);
    if ( rawobj != 0 )
    {
        vinsobj = _create_vins_json_params(0,coinstr,serverport,userpass,cointx);
        if ( vinsobj != 0 )
        {
            keysobj = _create_privkeys_json_params(coinstr,serverport,userpass,cointx,privkeys,cointx->numinputs);
            if ( keysobj != 0 )
            {
                array = cJSON_CreateArray();
                cJSON_AddItemToArray(array,rawobj);
                cJSON_AddItemToArray(array,vinsobj);
                cJSON_AddItemToArray(array,keysobj);
                paramstr = cJSON_Print(array);
                free_json(array);
            }
            else free_json(vinsobj);
        }
        else free_json(rawobj);
        //printf("vinsobj.%p keysobj.%p rawobj.%p\n",vinsobj,keysobj,rawobj);
    }
    return(paramstr);
}

int32_t _sign_rawtransaction(char *deststr,unsigned long destsize,char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx,char *rawbytes,char **privkeys)
{
    cJSON *json,*hexobj,*compobj;
    int32_t completed = -1;
    char *retstr,*signparams;
    deststr[0] = 0;
    //printf("sign_rawtransaction rawbytes.(%s) %p\n",rawbytes,privkeys);
    if ( (signparams= _createsignraw_json_params(coinstr,serverport,userpass,cointx,rawbytes,privkeys)) != 0 )
    {
        _stripwhite(signparams,0);
        printf("got signparams.(%s)\n",signparams);
        retstr = bitcoind_RPC(0,coinstr,serverport,userpass,"signrawtransaction",signparams);
        if ( retstr != 0 )
        {
            printf("got retstr.(%s)\n",retstr);
            json = cJSON_Parse(retstr);
            if ( json != 0 )
            {
                hexobj = cJSON_GetObjectItem(json,"hex");
                compobj = cJSON_GetObjectItem(json,"complete");
                if ( compobj != 0 )
                    completed = ((compobj->type&0xff) == cJSON_True);
                copy_cJSON(deststr,hexobj);
                if ( strlen(deststr) > destsize )
                    printf("sign_rawtransaction: strlen(deststr) %ld > %ld destize\n",strlen(deststr),destsize);
                //printf("got signedtransaction.(%s) ret.(%s) completed.%d\n",deststr,retstr,completed);
                free_json(json);
            } else printf("json parse error.(%s)\n",retstr);
            free(retstr);
        } else printf("error signing rawtx\n");
        free(signparams);
    } else printf("error generating signparams\n");
    return(completed);
}

char *_sign_localtx(char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx,char *rawbytes)
{
    char *batchsigned;
    cointx->batchsize = (uint32_t)strlen(rawbytes) + 1;
    cointx->batchcrc = _crc32(0,rawbytes+12,cointx->batchsize-12); // skip past timediff
    batchsigned = malloc(cointx->batchsize + cointx->numinputs*512 + 512);
    _sign_rawtransaction(batchsigned,cointx->batchsize + cointx->numinputs*512 + 512,coinstr,serverport,userpass,cointx,rawbytes,0);
    return(batchsigned);
}

cJSON *_create_vouts_json_params(struct cointx_info *cointx)
{
    int32_t i;
    cJSON *json,*obj;
    json = cJSON_CreateObject();
    for (i=0; i<cointx->numoutputs; i++)
    {
        obj = cJSON_CreateNumber(dstr(cointx->outputs[i].value));
        if ( strcmp(cointx->outputs[i].coinaddr,"OP_RETURN") != 0 )
            cJSON_AddItemToObject(json,cointx->outputs[i].coinaddr,obj);
        else
        {
            // int32_t ram_make_OP_RETURN(char *scriptstr,uint64_t *redeems,int32_t numredeems)
            cJSON_AddItemToObject(json,cointx->outputs[0].coinaddr,obj);
        }
    }
    printf("numdests.%d (%s)\n",cointx->numoutputs,cJSON_Print(json));
    return(json);
}

char *_createrawtxid_json_params(char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx)
{
    char *paramstr = 0;
    cJSON *array,*vinsobj,*voutsobj;
    vinsobj = _create_vins_json_params(0,coinstr,serverport,userpass,cointx);
    if ( vinsobj != 0 )
    {
        voutsobj = _create_vouts_json_params(cointx);
        if ( voutsobj != 0 )
        {
            array = cJSON_CreateArray();
            cJSON_AddItemToArray(array,vinsobj);
            cJSON_AddItemToArray(array,voutsobj);
            paramstr = cJSON_Print(array);
            free_json(array);   // this frees both vinsobj and voutsobj
        }
        else free_json(vinsobj);
    } else printf("_error create_vins_json_params\n");
    //printf("_createrawtxid_json_params.%s\n",paramstr);
    return(paramstr);
}

#endif
#endif
