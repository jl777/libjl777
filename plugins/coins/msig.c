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
#include "../includes/uthash.h"
#include "../includes/cJSON.h"
#include "../sophia/db777.c"
#include "../utils/bits777.c"
#include "../utils/NXT777.c"
#include "../utils/system777.c"
#include "../sophia/kv777.c"
#include "../coins/cointx.c"
#include "../coins/gen1auth.c"

struct multisig_addr *alloc_multisig_addr(char *coinstr,int32_t m,int32_t n,char *NXTaddr,char *userpubkey,char *sender);

int32_t update_MGW_jsonfile(void (*setfname)(char *fname,char *NXTaddr),void *(*extract_jsondata)(cJSON *item,void *arg,void *arg2),int32_t (*jsoncmp)(void *ref,void *item),char *NXTaddr,char *jsonstr,void *arg,void *arg2);
void set_MGW_depositfname(char *fname,char *NXTaddr);
int32_t _map_msigaddr(char *redeemScript,char *coinstr,char *serverport,char *userpass,char *normaladdr,char *msigaddr,int32_t gatewayid,int32_t numgateways); //could map to rawind, but this is rarely called
struct multisig_addr *finalize_msig(struct multisig_addr *msig,uint64_t *srvbits,uint64_t refbits);

char *genmultisig(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *coinstr,char *refacct,int32_t M,int32_t N,uint64_t *srv64bits,int32_t n,char *userpubkey,char *email,uint32_t buyNXT);
char *setmultisig(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,char *origargstr);
char *getmsigpubkey(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,char *coinstr,char *refNXTaddr,char *myacctcoinaddr,char *mypubkey);
char *setmsigpubkey(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,char *coinstr,char *refNXTaddr,char *acctcoinaddr,char *userpubkey);
struct multisig_addr *find_msigaddr(struct multisig_addr *msig,int32_t *lenp,char *coinstr,char *msigaddr);
int32_t save_msigaddr(char *coinstr,char *NXTaddr,struct multisig_addr *msig);
struct multisig_addr *gen_multisig_addr(char *sender,int32_t M,int32_t N,char *coinstr,char *serverport,char *userpass,int32_t use_addmultisig,char *refNXTaddr,char *userpubkey,uint64_t *srvbits);
int32_t add_MGWaddr(char *previpaddr,char *sender,int32_t valid,char *origargstr);

int32_t update_MGW_msig(struct multisig_addr *msig,char *sender);
int32_t MGW_publish_acctpubkeys(char *coinstr,char *str);

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
 
int32_t update_msig_info(struct multisig_addr *msig,int32_t syncflag,char *sender)
{
    int32_t i,ret = 0;
    if ( msig == 0 )
    {
        if ( syncflag != 0 )//&& sdb != 0 && sdb->storage != 0 )
        {
            // return(dbsync(sdb,0));
        }
        return(0);
    }
    if ( msig->multisigaddr[0] == 0 )
        return(-1);
    for (i=0; i<msig->n; i++)
        if ( msig->pubkeys[i].nxt64bits != 0 && msig->pubkeys[i].coinaddr[0] != 0 && msig->pubkeys[i].pubkey[0] != 0 )
            add_NXT_coininfo(msig->pubkeys[i].nxt64bits,calc_nxt64bits(msig->NXTaddr),msig->coinstr,msig->pubkeys[i].coinaddr,msig->pubkeys[i].pubkey);
    if ( msig->size == 0 )
        msig->size = sizeof(*msig) + (msig->n * sizeof(msig->pubkeys[0]));
    save_msigaddr(msig->coinstr,msig->NXTaddr,msig,msig->size);
    //if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
    printf("add (%s) NXTpubkey.(%s)\n",msig->multisigaddr,msig->NXTpubkey);
    return(ret);
}

struct multisig_addr *find_NXT_msig(char *NXTaddr,char *coinstr,uint64_t *srv64bits,int32_t n)
{
    struct multisig_addr **msigs,*retmsig = 0;
    int32_t i,j,nummsigs;
    uint64_t nxt64bits;
    printf("find_NXT_msig(%s,%s,%p,%d)\n",NXTaddr,coinstr,srv64bits,n);
    if ( (msigs= (struct multisig_addr **)db777_copy_all(&nummsigs,DB_msigs,"value",0)) != 0 )
    {
        nxt64bits = (NXTaddr != 0) ? calc_nxt64bits(NXTaddr) : 0;
        for (i=0; i<nummsigs; i++)
        {
            printf("i.%d of nummsigs.%d: %p srvbits.%p n.(%d %d) %p\n",i,nummsigs,msigs[i],srv64bits,msigs[i]->valid,msigs[i]->n,msigs[i]);
            if ( msigs[i]->valid != msigs[i]->n && msigs[i]->valid < n && msigs[i]->n == n )
            {
                if ( finalize_msig(msigs[i],srv64bits,nxt64bits) == 0 )
                    continue;
                //printf("FIXED %llu -> %s\n",(long long)nxt64bits,msigs[i]->multisigaddr);
                //update_msig_info(msigs[i],1,0);
                //update_MGW_msig(msigs[i],0);
            }
            if ( nxt64bits != 0 && strcmp(coinstr,msigs[i]->coinstr) == 0 && strcmp(NXTaddr,msigs[i]->NXTaddr) == 0 )
            {
                for (j=0; j<n; j++)
                    if ( srv64bits[j] != msigs[i]->pubkeys[j].nxt64bits )
                        break;
                if ( j == n )
                {
                    if ( retmsig != 0 )
                        free(retmsig);
                    retmsig = msigs[i];
                }
            }
            if ( msigs[i] != retmsig )
                free(msigs[i]);
        }
        free(msigs);
    }
    return(retmsig);
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

void *extract_jsonmsig(cJSON *item,void *arg,void *arg2)
{
    char sender[MAX_JSON_FIELD];
    copy_cJSON(sender,cJSON_GetObjectItem(item,"sender"));
    return(decode_msigjson(0,item,sender));
}

int32_t jsonmsigcmp(void *ref,void *item) { return(msigcmp(ref,item)); }
int32_t jsonstrcmp(void *ref,void *item) { return(strcmp(ref,item)); }

cJSON *http_search(char *destip,char *type,char *file)
{
    cJSON *json = 0;
    char url[1024],*retstr;
    sprintf(url,"http://%s/%s/%s",destip,type,file);
    if ( (retstr= issue_curl(url)) != 0 )
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
                if ( (msig= decode_msigjson(0,cJSON_GetArrayItem(array,i),external_NXTaddr)) != 0 && (msig= find_msigaddr(&len,msig->coinstr,NXTaddr,msig->multisigaddr)) != 0 )
                    break;
        }
        free_json(array);
    }
    return(msig);
}

int32_t update_MGW_msig(struct multisig_addr *msig,char *sender)
{
    char *create_multisig_jsonstr(struct multisig_addr *msig,int32_t truncated);
    char *jsonstr;
    int32_t appendflag = 0;
    if ( msig != 0 )
    {
        jsonstr = create_multisig_jsonstr(msig,0);
        if ( jsonstr != 0 )
        {
            //if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
            //   printf("add_MGWaddr(%s) from (%s)\n",jsonstr,sender!=0?sender:"");
            //broadcast_bindAM(msig->NXTaddr,msig,origargstr);
            //update_MGW_msigfile(0,msig,jsonstr);
            // update_MGW_msigfile(msig->NXTaddr,msig,jsonstr);
            update_MGW_jsonfile(set_MGW_msigfname,extract_jsonmsig,jsonmsigcmp,0,jsonstr,0,0);
            appendflag = update_MGW_jsonfile(set_MGW_msigfname,extract_jsonmsig,jsonmsigcmp,msig->NXTaddr,jsonstr,0,0);
            free(jsonstr);
        }
    }
    return(appendflag);
}

struct multisig_addr *finalize_msig(struct multisig_addr *msig,uint64_t *srvbits,uint64_t refbits)
{
    int32_t i,n;
    char acctcoinaddr[1024],pubkey[1024];
    for (i=n=0; i<msig->n; i++)
    {
        printf("i.%d n.%d msig->n.%d NXT.(%s) msig.(%s) %p\n",i,n,msig->n,msig->NXTaddr,msig->multisigaddr,msig);
        if ( srvbits[i] != 0 && refbits != 0 )
        {
            acctcoinaddr[0] = pubkey[0] = 0;
            if ( get_NXT_coininfo(srvbits[i],refbits,msig->coinstr,acctcoinaddr,pubkey) != 0 && acctcoinaddr[0] != 0 && pubkey[0] != 0 )
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
    int32_t issue_createmultisig(char *multisigaddr,char *redeemScript,char *coinstr,char *serverport,char *userpass,int32_t use_addmultisig,struct multisig_addr *msig);
    uint64_t refbits;
    int32_t flag = 0;
    struct multisig_addr *msig;
    refbits = calc_nxt64bits(refNXTaddr);
    msig = alloc_multisig_addr(coinstr,M,N,refNXTaddr,userpubkey,sender);
    if ( (msig= finalize_msig(msig,srvbits,refbits)) != 0 )
        flag = issue_createmultisig(msig->multisigaddr,msig->redeemScript,coinstr,serverport,userpass,use_addmultisig,msig);
    if ( flag == 0 )
    {
        free(msig);
        return(0);
    }
    return(msig);
}

int32_t update_MGWaddr(cJSON *argjson,char *sender)
{
    int32_t i,retval = 0;
    uint64_t senderbits;
    struct multisig_addr *msig;
    if  ( (msig= decode_msigjson(0,argjson,sender)) != 0 )
    {
        senderbits = calc_nxt64bits(sender);
        for (i=0; i<msig->n; i++)
        {
            if ( msig->pubkeys[i].nxt64bits == senderbits )
            {
                update_msig_info(msig,1,sender);
                update_MGW_msig(msig,sender);
                retval = 1;
                break;
            }
        }
        free(msig);
    }
    return(retval);
}

int32_t add_MGWaddr(char *previpaddr,char *sender,int32_t valid,char *origargstr)
{
    cJSON *origargjson,*argjson;
    if ( valid > 0 && (origargjson= cJSON_Parse(origargstr)) != 0 )
    {
        if ( is_cJSON_Array(origargjson) != 0 )
            argjson = cJSON_GetArrayItem(origargjson,0);
        else argjson = origargjson;
        return(update_MGWaddr(argjson,sender));
    }
    return(0);
}

char *genmultisig(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *coinstr,char *refacct,int32_t M,int32_t N,uint64_t *srv64bits,int32_t n,char *userpubkey,char *email,uint32_t buyNXT)
{
    struct coin777 *coin;
    uint32_t crc,lastcrc = 0;
    struct multisig_addr *msig;
    char refNXTaddr[64],destNXTaddr[64],mypubkey[1024],myacctcoinaddr[1024],pubkey[1024],acctcoinaddr[1024],buf[1024],*retstr = 0;
    uint64_t my64bits,nxt64bits,refbits = 0;
    int32_t i,iter,len,flag,valid = 0;
    if ( (coin= coin777_find(coinstr)) == 0 )
        return(0);
    my64bits = calc_nxt64bits(NXTaddr);
    refbits = conv_acctstr(refacct);
    expand_nxt64bits(refNXTaddr,refbits);
    //if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
    printf("GENMULTISIG.%d from (%s) for %s refacct.(%s) %llu %s email.(%s) buyNXT.%u userpub.%s\n",N,previpaddr,coinstr,refacct,(long long)refbits,refNXTaddr,email,buyNXT,userpubkey);// getchar();
    if ( refNXTaddr[0] == 0 )
        return(clonestr("\"error\":\"genmultisig couldnt find refcontact\"}"));
    if ( userpubkey[0] == 0 )
        set_NXTpubkey(userpubkey,refNXTaddr);
    flag = 0;
    myacctcoinaddr[0] = mypubkey[0] = 0;
    for (iter=0; iter<2; iter++)
        for (i=0; i<n; i++)
        {
            //fprintf(stderr,"iter.%d i.%d\n",iter,i);
            if ( (nxt64bits= srv64bits[i]) != 0 )
            {
                if ( iter == 0 && my64bits == nxt64bits )
                {
                    myacctcoinaddr[0] = mypubkey[0] = 0;
                    if ( get_acct_coinaddr(myacctcoinaddr,coinstr,coin->serverport,coin->userpass,refNXTaddr) != 0 && get_pubkey(mypubkey,coinstr,coin->serverport,coin->userpass,myacctcoinaddr) != 0 && myacctcoinaddr[0] != 0 && mypubkey[0] != 0 )
                    {
                        flag++;
                        add_NXT_coininfo(nxt64bits,refbits,coinstr,myacctcoinaddr,mypubkey);
                        valid++;
                    }
                    else printf("error getting msigaddr for (%s) ref.(%s) addr.(%s) pubkey.(%s)\n",coinstr,refNXTaddr,myacctcoinaddr,mypubkey);
                }
                else if ( iter == 1 && my64bits != nxt64bits )
                {
                    acctcoinaddr[0] = pubkey[0] = 0;
                    if ( get_NXT_coininfo(nxt64bits,refbits,coinstr,acctcoinaddr,pubkey) == 0 || acctcoinaddr[0] == 0 || pubkey[0] == 0 )
                    {
                        sprintf(buf,"{\"requestType\":\"plugin\",\"plugin\":\"coins\",\"method\":\"getmsigpubkey\",\"NXT\":\"%s\",\"coin\":\"%s\",\"refNXTaddr\":\"%s\",\"userpubkey\":\"%s\"",NXTaddr,coinstr,refNXTaddr,userpubkey);
                        if ( myacctcoinaddr[0] != 0 && mypubkey[0] != 0 )
                            sprintf(buf+strlen(buf),",\"myaddr\":\"%s\",\"mypubkey\":\"%s\"",myacctcoinaddr,mypubkey);
                        if ( Debuglevel > 2 )
                            printf("SENDREQ.(%s)\n",buf);
                        expand_nxt64bits(destNXTaddr,nxt64bits);
                        if ( (crc= _crc32(0,buf,strlen(buf))) != lastcrc )
                        {
                            sprintf(buf+strlen(buf),",\"tag\":\"%u\"}",rand());
                            if ( (len= nn_send(MGW.all.socks.both.bus,buf,(int32_t)strlen(buf)+1,0)) <= 0 )
                                printf("error sending (%s)\n",buf);
                            else printf("sent.(%s).%d\n",buf,len);
                            lastcrc = crc;
                        }
                        /*if ( (str= plugin_method(0,0,"coins","getmsigpubkey",0,milliseconds(),buf,0,1,10000)) != 0 )
                        {
                            printf("GENMULTISIG sent to MGW bus (%s)\n",buf);
                            free(str);
                        }*/
                    }
                    else
                    {
                        //printf("already have %llu:%llu (%s %s)\n",(long long)contact->nxt64bits,(long long)refbits,acctcoinaddr,pubkey);
                        valid++;
                    }
                }
            }
        }
    if ( (msig= find_NXT_msig(NXTaddr,coinstr,srv64bits,N)) == 0 )
    {
        if ( (msig= gen_multisig_addr(NXTaddr,M,N,coinstr,coin->serverport,coin->userpass,coin->use_addmultisig,refNXTaddr,userpubkey,srv64bits)) != 0 )
        {
            msig->valid = valid;
            safecopy(msig->email,email,sizeof(msig->email));
            msig->buyNXT = buyNXT;
            update_msig_info(msig,1,NXTaddr);
            update_MGW_msig(msig,NXTaddr);
        }
    } else valid = N;
    if ( valid == N && msig != 0 )
    {
        char *create_multisig_jsonstr(struct multisig_addr *msig,int32_t truncated);
        if ( (retstr= create_multisig_jsonstr(msig,0)) != 0 )
        {
            if ( retstr != 0 )//&& previpaddr != 0 && previpaddr[0] != 0 )
            {
                if ( (len= nn_send(MGW.all.socks.both.bus,retstr,(int32_t)strlen(retstr)+1,0)) <= 0 )
                    printf("error sending (%s)\n",retstr);
                else printf("sent.(%s).%d\n",retstr,len);
            }
        }
    }
    if ( msig != 0 )
        free(msig);
    if ( valid != N || retstr == 0 )
    {
        sprintf(buf,"{\"error\":\"missing msig info\",\"refacct\":\"%s\",\"coin\":\"%s\",\"M\":%d,\"N\":%d,\"valid\":%d}",refacct,coinstr,M,N,valid);
        retstr = clonestr(buf);
        //printf("%s\n",buf);
    }
    return(retstr);
}

char *getmsigpubkey(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,char *coinstr,char *refNXTaddr,char *myacctcoinaddr,char *mypubkey)
{
    struct coin777 *coin;
    char acctcoinaddr[MAX_JSON_FIELD],pubkey[MAX_JSON_FIELD],buf[MAX_JSON_FIELD];
    coin = coin777_find(coinstr);
    printf("GETMSIGPUBKEY from sender.(%s) coin.(%s).%p ref.(%s) myacct.(%s) mypub.(%s)\n",sender,coinstr,coin,refNXTaddr,myacctcoinaddr,mypubkey);
    if ( refNXTaddr[0] != 0 && coin != 0 )
    {
        get_acct_coinaddr(acctcoinaddr,coinstr,coin->serverport,coin->userpass,refNXTaddr);
        get_pubkey(pubkey,coinstr,coin->serverport,coin->userpass,acctcoinaddr);
        if ( myacctcoinaddr != 0 && myacctcoinaddr[0] != 0 && mypubkey != 0 && mypubkey[0] != 0 )
            add_NXT_coininfo(calc_nxt64bits(sender),conv_acctstr(refNXTaddr),coinstr,myacctcoinaddr,mypubkey);
        if ( pubkey[0] != 0 && acctcoinaddr[0] != 0 )
        {
            sprintf(buf,"{\"requestType\":\"plugin\",\"plugin\":\"coins\",\"method\":\"setmsigpubkey\",\"NXT\":\"%s\",\"coin\":\"%s\",\"refNXTaddr\":\"%s\",\"addr\":\"%s\",\"userpubkey\":\"%s\",\"tag\":\"%u\"}",NXTaddr,coinstr,refNXTaddr,acctcoinaddr,pubkey,rand());
            if ( nn_send(MGW.all.socks.both.bus,buf,(int32_t)strlen(buf)+1,0) <= 0 )
                printf("error sending (%s)\n",buf);
            return(clonestr(buf));
            /*if ( (str= plugin_method(0,previpaddr,"coins","setmsigpubkey",0,milliseconds(),buf,0,1,10000)) != 0 )
            {
                printf("GETMSIG sent to MGW bus (%s)\n",buf);
                free(str);
            }*/
        } else sprintf(buf,"{\"result\":\"setmsigpubkey\",\"NXT\":\"%s\",\"coin\":\"%s\",\"refNXTaddr\":\"%s\",\"addr\":\"%s\",\"userpubkey\":\"%s\"}",NXTaddr,coinstr,refNXTaddr,acctcoinaddr,pubkey);
    }
    return(clonestr("{\"error\":\"bad getmsigpubkey_func paramater\"}"));
}

char *setmsigpubkey(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,char *coinstr,char *refNXTaddr,char *acctcoinaddr,char *userpubkey)
{
    struct coin777 *coin;
    uint64_t nxt64bits;
    if ( sender[0] != 0 && refNXTaddr[0] != 0 && acctcoinaddr[0] != 0 && userpubkey[0] != 0 && (coin= coin777_find(coinstr)) != 0 )
    {
        if ( (nxt64bits= conv_acctstr(refNXTaddr)) != 0 )
        {
            add_NXT_coininfo(calc_nxt64bits(sender),nxt64bits,coinstr,acctcoinaddr,userpubkey);
            return(clonestr("{\"result\":\"setmsigpubkey added coininfo\"}"));
        }
        return(clonestr("{\"error\":\"setmsigpubkey_func couldnt convert refNXTaddr\"}"));
    }
    return(clonestr("{\"error\":\"bad setmsigpubkey_func paramater\"}"));
}

char *setmultisig(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,char *origargstr)
{
    static uint32_t lastcrc = 0;
    uint32_t crc;
    if ( Debuglevel > 0 )
        printf("MGWaddr_func(%s)\n",origargstr);
    if ( sender[0] != 0 && origargstr[0] != 0 )
    {
        if ( (crc= _crc32(0,origargstr,strlen(origargstr))) != lastcrc )
        {
            if ( nn_send(MGW.all.socks.both.bus,origargstr,(int32_t)strlen(origargstr)+1,0) <= 0 )
                printf("error sending (%s)\n",origargstr);
            lastcrc = crc;
        }
        add_MGWaddr(previpaddr,sender,1,origargstr);
    }
    return(clonestr(origargstr));
}

#include <curl/curl.h>
#include <curl/easy.h>
int32_t init_public_msigs()
{
    void *curl_post(CURL **cHandlep,char *url,char *userpass,char *postfields,char *hdr0,char *hdr1,char *hdr2);
    static void *cHandle;
    char Server_NXTaddr[64],url[1024],*retstr;
    struct multisig_addr *msig;
    cJSON *json;
    int32_t len,i,j,n,added = 0;
    printf("init_public_msigs numgateways.%d gatewayid.%d\n",MGW.numgateways,MGW.gatewayid);
    if ( MGW.gatewayid < 0 || MGW.numgateways <= 0 )
        return(-1);
    for (j=0; j<MGW.numgateways; j++)
    {
        expand_nxt64bits(Server_NXTaddr,MGW.srv64bits[j]);
        sprintf(url,"http://%s/MGW/msig/ALL",MGW.serverips[j]);
        printf("issue.(%s)\n",url);
        if ( (retstr= curl_post(&cHandle,url,0,0,"",0,0)) != 0 )
        {
            printf("got.(%s)\n",retstr);
            if ( (json= cJSON_Parse(retstr)) != 0 )
            {
                if ( is_cJSON_Array(json) != 0 && (n= cJSON_GetArraySize(json)) > 0 )
                {
                    for (i=0; i<n; i++)
                        if ( (msig= decode_msigjson(0,cJSON_GetArrayItem(json,i),Server_NXTaddr)) != 0 )
                        {
                            if ( find_msigaddr(&len,msig->coinstr,msig->NXTaddr,msig->multisigaddr) == 0 )
                            {
                                printf("ADD.%d %s.(%s) NXT.(%s) NXTpubkey.(%s) (%s)\n",added,msig->coinstr,msig->multisigaddr,msig->NXTaddr,msig->NXTpubkey,msig->pubkeys[0].coinaddr);
                                if ( is_zeroes(msig->NXTpubkey) != 0 )
                                {
                                    set_NXTpubkey(msig->NXTpubkey,msig->NXTaddr);
                                    printf("FIX (%s) NXT.(%s) NXTpubkey.(%s)\n",msig->multisigaddr,msig->NXTaddr,msig->NXTpubkey);
                                }
                                update_msig_info(msig,i == n-1,Server_NXTaddr), added++;
                            }
                        }
                }
                free_json(json);
            }
            free(retstr);
        }
    }
    printf("added.%d multisig addrs\n",added);
    return(added);
}

#endif
#endif
