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
#include "system777.c"
#include "storage.c"
#include "ramchain.c"


struct multisig_addr *find_msigaddr(char *msigaddr);
struct multisig_addr *ram_add_msigaddr(char *msigaddr,int32_t n,char *NXTaddr,char *NXTpubkey,int32_t buyNXT);
int32_t update_msig_info(struct multisig_addr *msig,int32_t syncflag,char *sender);
int32_t update_MGWaddr(cJSON *argjson,char *sender);
int32_t add_MGWaddr(char *previpaddr,char *sender,int32_t valid,char *origargstr);
int32_t msigcmp(struct multisig_addr *ref,struct multisig_addr *msig);
struct multisig_addr *decode_msigjson(char *NXTaddr,cJSON *obj,char *sender);
char *create_multisig_json(struct multisig_addr *msig,int32_t truncated);

extern int32_t Debuglevel;

#endif
#else
#ifndef crypto777_msig_c
#define crypto777_msig_c

#ifndef crypto777_msig_h
#define DEFINES_ONLY
#include __BASE_FILE__
#undef DEFINES_ONLY
#endif

#define BTC_COINID 1
#define LTC_COINID 2
#define DOGE_COINID 4
#define BTCD_COINID 8

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

struct multisig_addr *MSIG_table,**MSIGS;
portable_mutex_t MSIGmutex;
int32_t didMSIGinit,Num_MSIGS;
struct multisig_addr *find_msigaddr(char *msigaddr)
{
    int32_t i;//createdflag;
    struct multisig_addr *msig = 0;
    if ( didMSIGinit == 0 )
    {
        portable_mutex_init(&MSIGmutex);
        didMSIGinit = 1;
    }
    portable_mutex_lock(&MSIGmutex);
    //HASH_FIND(hh,MSIG_table,msigaddr,strlen(msigaddr),msig);
    for (i=0; i<Num_MSIGS; i++)
        if ( strcmp(msigaddr,MSIGS[i]->multisigaddr) == 0 )
        {
            msig = MSIGS[i];
            break;
        }
    portable_mutex_unlock(&MSIGmutex);
    return(msig);
    /*if ( MTsearch_hashtable(&SuperNET_dbs[MULTISIG_DATA].ramtable,msigaddr) == HASHSEARCH_ERROR )
     {
     printf("(%s) not MGW multisig addr\n",msigaddr);
     return(0);
     }
     printf("found (%s)\n",msigaddr);
     return(MTadd_hashtable(&createdflag,&SuperNET_dbs[MULTISIG_DATA].ramtable,msigaddr));*/ // only do this if it is already there
    //return((struct multisig_addr *)find_storage(MULTISIG_DATA,msigaddr,0));
}

struct multisig_addr *ram_add_msigaddr(char *msigaddr,int32_t n,char *NXTaddr,char *NXTpubkey,int32_t buyNXT)
{
    struct multisig_addr *msig;
    if ( (msig= find_msigaddr(msigaddr)) == 0 )
    {
        msig = calloc(1,sizeof(*msig) + n*sizeof(struct pubkey_info));
        strcpy(msig->multisigaddr,msigaddr);
        if ( NXTaddr != 0 )
            strcpy(msig->NXTaddr,NXTaddr);
        if ( NXTpubkey != 0 )
            strcpy(msig->NXTpubkey,NXTpubkey);
        if ( buyNXT >= 0 )
            msig->buyNXT = buyNXT;
        if ( didMSIGinit == 0 )
        {
            //portable_mutex_init(&MSIGmutex);
            didMSIGinit = 1;
        }
        //printf("ram_add_msigaddr MSIG[%s] NXT.%s (%s) buyNXT.%d\n",msigaddr,msig->NXTaddr,msig->NXTpubkey,msig->buyNXT);
        portable_mutex_lock(&MSIGmutex);
        MSIGS = realloc(MSIGS,(1+Num_MSIGS) * sizeof(*MSIGS));
        MSIGS[Num_MSIGS] = msig, Num_MSIGS++;
        //HASH_ADD_KEYPTR(hh,MSIG_table,clonestr(msigaddr),strlen(msigaddr),msig);
        portable_mutex_unlock(&MSIGmutex);
        //printf("done ram_add_msigaddr MSIG[%s] NXT.%s (%s) buyNXT.%d\n",msigaddr,msig->NXTaddr,msig->NXTpubkey,msig->buyNXT);
    }
    return(msig);
}

int32_t map_msigaddr(char *redeemScript,struct ramchain_info *cp,char *normaladdr,char *msigaddr)
{
    struct ramchain_info *refcp = get_ramchain_info("BTCD");
    int32_t i,n,ismine;
    cJSON *json,*array,*json2;
    struct multisig_addr *msig;
    char addr[1024],args[1024],*retstr,*retstr2;
    redeemScript[0] = normaladdr[0] = 0;
    if ( cp == 0 || refcp == 0 || (msig= find_msigaddr(msigaddr)) == 0 )
    {
        strcpy(normaladdr,msigaddr);
        return(0);
    }
    /* {
     "isvalid" : true,
     "address" : "bUNry9zFx9EQnukpUNDgHRsw6zy3eUs8yR",
     "ismine" : true,
     "isscript" : true,
     "script" : "multisig",
     "hex" : "522103a07d28c8d4eaa7e90dc34133fec204f9cf7740d5fd21acc00f9b0552e6bd721e21036d2b86cb74aaeaa94bb82549c4b6dd9666355241d37c371b1e0a17d060dad1c82103ceac7876e4655cf4e39021cf34b7228e1d961a2bcc1f8e36047b40149f3730ff53ae",
     "addresses" : [
     "RGjegNGJDniYFeY584Adfgr8pX2uQegfoj",
     "RQWB6GWe67EHCYurSiffYbyZPi7RGcrZa2",
     "RWVebRCCVMz3YWrZEA9Lc3VWKH9kog5wYg"
     ],
     "sigsrequired" : 2,
     "account" : ""
     }
     */
    sprintf(args,"\"%s\"",msig->multisigaddr);
    retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"validateaddress",args);
    if ( retstr != 0 )
    {
        //printf("got retstr.(%s)\n",retstr);
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
                        retstr2 = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"validateaddress",args);
                        if ( retstr2 != 0 )
                        {
                            if ( (json2 = cJSON_Parse(retstr2)) != 0 )
                                ismine = is_cJSON_True(cJSON_GetObjectItem(json2,"ismine"));
                            free(retstr2);
                        }
                    }
                    if ( ismine != 0 )
                    {
                        printf("(%s) ismine.%d\n",addr,ismine);
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

struct multisig_addr *alloc_multisig_addr(char *coinstr,int32_t m,int32_t n,char *NXTaddr,char *userpubkey,char *sender)
{
    struct multisig_addr *msig;
    int32_t size = (int32_t)(sizeof(*msig) + n*sizeof(struct pubkey_info));
    msig = calloc(1,size);
    msig->H.size = size;
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

uint32_t calc_ipbits(char *ipaddr)
{
    printf("make portable ipbits for (%s)\n",ipaddr);
    getchar();
    return(0);
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
                    if ( ipaddr[0] == 0 && j < 3 )
                        strcpy(ipaddr,Server_ipaddrs[j]);
                    msig->pubkeys[j].ipbits = calc_ipbits(ipaddr);
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

int32_t issue_createmultisig(struct ramchain_info *cp,struct multisig_addr *msig)
{
    int32_t flag = 0;
    char addr[256];
    cJSON *json,*msigobj,*redeemobj;
    char *params,*retstr = 0;
    params = createmultisig_json_params(msig,(cp->use_addmultisig != 0) ? msig->NXTaddr : 0);
    flag = 0;
    if ( params != 0 )
    {
        if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
            printf("multisig params.(%s)\n",params);
        if ( cp->use_addmultisig != 0 )
        {
            retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"addmultisigaddress",params);
            if ( retstr != 0 )
            {
                strcpy(msig->multisigaddr,retstr);
                free(retstr);
                sprintf(addr,"\"%s\"",msig->multisigaddr);
                retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"validateaddress",addr);
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
            retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"createmultisig",params);
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

struct multisig_addr *gen_multisig_addr(char *sender,int32_t M,int32_t N,struct ramchain_info *cp,char *refNXTaddr,char *userpubkey,uint64_t *srvbits)
{
    uint64_t refbits;//,srvbits[16];
    int32_t flag = 0;
    struct multisig_addr *msig;
    if ( cp == 0 )
        return(0);
    refbits = calc_nxt64bits(refNXTaddr);
    msig = alloc_multisig_addr(cp->name,M,N,refNXTaddr,userpubkey,sender);
    //for (i=0; i<N; i++)
    //    srvbits[i] = contacts[i]->nxt64bits;
    if ( (msig= finalize_msig(msig,srvbits,refbits)) != 0 )
        flag = issue_createmultisig(cp,msig);
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

struct multisig_addr *http_search_msig(char *external_NXTaddr,char *external_ipaddr,char *NXTaddr)
{
    int32_t i,n;
    cJSON *array;
    struct multisig_addr *msig = 0;
    if ( (array= http_search(external_ipaddr,"MGW/msig",NXTaddr)) != 0 )
    {
        if ( is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
        {
            for (i=0; i<n; i++)
                if ( (msig= decode_msigjson(0,cJSON_GetArrayItem(array,i),external_NXTaddr)) != 0 && (msig= find_msigaddr(msig->multisigaddr)) != 0 )
                    break;
        }
        free_json(array);
    }
    return(msig);
}

#endif
#endif
