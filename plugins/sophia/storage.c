//
//  storage.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_storage_h
#define crypto777_storage_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "uthash.h"
#include "cJSON.h"
#include "bits777.c"
#include "system777.c"
#include "db777.c"
#include "msig.c"

struct nodestats *get_nodestats(uint64_t nxt64bits);
struct acct_coin2 *get_NXT_coininfo(uint64_t srvbits,char *acctcoinaddr,char *pubkey,uint64_t nxt64bits,char *coinstr);
void add_NXT_coininfo(uint64_t srvbits,uint64_t nxt64bits,char *coinstr,char *acctcoinaddr,char *pubkey);
cJSON *http_search(char *destip,char *type,char *file);
struct NXT_acct *get_NXTacct(int32_t *createdp,char *NXTaddr);
void update_nodestats_data(struct nodestats *stats);
void set_NXTpubkey(char *NXTpubkey,char *NXTacct);
struct multisig_addr *find_NXT_msig(char *NXTaddr,char *coinstr,uint64_t *srv64bits,int32_t n);
int32_t update_msig_info(struct multisig_addr *msig,int32_t syncflag,char *sender);
struct NXT_acct *get_nxt64bits(int32_t *createdp,uint64_t nxt64bits);

#endif
#else
#ifndef crypto777_storage_c
#define crypto777_storage_c


#ifndef crypto777_storage_h
#define DEFINES_ONLY
#include "storage.c"
//#undef DEFINES_ONLY
#endif


struct NXT_acct *get_nxt64bits(int32_t *createdp,uint64_t nxt64bits)
{
    struct NXT_acct *np;
    int32_t len;
    if ( (np= db777_findM(&len,DB_NXTaccts,&nxt64bits,sizeof(nxt64bits))) == 0 )
    {
        np = calloc(1,sizeof(*np));
        np->nxt64bits = nxt64bits, expand_nxt64bits(np->NXTaddr,nxt64bits);
        db777_add(1,DB_NXTaccts,&nxt64bits,sizeof(nxt64bits),np,sizeof(*np));
        *createdp = 1;
    } else *createdp = 0;
    return(np);
}

struct NXT_acct *get_NXTacct(int32_t *createdp,char *NXTaddr)
{
   return(get_nxt64bits(createdp,calc_nxt64bits(NXTaddr)));
}

struct nodestats *get_nodestats(uint64_t nxt64bits)
{
    struct nodestats *stats = 0;
    int32_t createdflag;
    struct NXT_acct *np;
    if ( nxt64bits != 0 )
    {
        np = get_nxt64bits(&createdflag,nxt64bits);
        stats = &np->stats;
        if ( stats->nxt64bits == 0 )
            np->nxt64bits = nxt64bits, expand_nxt64bits(np->NXTaddr,nxt64bits);
    }
    return(stats);
}

void update_nodestats_data(struct nodestats *stats)
{
    char ipaddr[64];
    int32_t createdflag;
    struct NXT_acct *np;
    expand_ipbits(ipaddr,stats->ipbits);
    np = get_nxt64bits(&createdflag,stats->nxt64bits);
    np->stats = *stats;
    if ( Debuglevel > 2 )
        printf("Update nodestats.%llu (%s) lastcontact %u\n",(long long)stats->nxt64bits,ipaddr,stats->lastcontact);
    db777_add(1,DB_NXTaccts,&stats->nxt64bits,sizeof(stats->nxt64bits),np,sizeof(*np));
}

struct NXT_assettxid *find_NXT_assettxid(int32_t *createdflagp,struct NXT_asset *ap,char *txid)
{
    int32_t createdflag;
    struct NXT_assettxid *tp;
    if ( createdflagp == 0 )
        createdflagp = &createdflag;
    printf("port assettxid\n"); getchar();
    tp = 0;//MTadd_hashtable(createdflagp,&NXT_assettxids,txid);
    if ( *createdflagp != 0 )
    {
        //tp->assetbits = ap->assetbits;
        // tp->redeemtxid = calc_nxt64bits(txid);
        // tp->timestamp = timestamp;
        //printf("%d) %s txid.%s\n",ap->num,ap->name,txid);
        if ( ap != 0 )
        {
            if ( ap->num >= ap->max )
            {
                ap->max = ap->num + NXT_ASSETLIST_INCR;
                ap->txids = realloc(ap->txids,sizeof(*ap->txids) * ap->max);
            }
            ap->txids[ap->num++] = tp;
        }
    }
    return(tp);
}

struct acct_coin2 *find_NXT_coininfo(struct NXT_acct **npp,uint64_t nxt64bits,char *coinstr)
{
    char NXTaddr[64];
    struct NXT_acct *np;
    int32_t i,createdflag;
    expand_nxt64bits(NXTaddr,nxt64bits);
    np = get_NXTacct(&createdflag,NXTaddr);
    if ( npp != 0 )
        (*npp) = np;
    if ( np->numcoins > 0 )
    {
        for (i=0; i<np->numcoins; i++)
            if ( strcmp(np->coins[i].name,coinstr) == 0 ) //np->coins[i] != 0 &&
                return(&np->coins[i]);
    }
    return(0);
}

struct acct_coin2 *get_NXT_coininfo(uint64_t srvbits,char *acctcoinaddr,char *pubkey,uint64_t nxt64bits,char *coinstr)
{
    struct acct_coin2 *acp = 0;
    int32_t i;
    acctcoinaddr[0] = pubkey[0] = 0;
    if ( (acp= find_NXT_coininfo(0,nxt64bits,coinstr)) != 0 )
    {
        if ( acp->numsrvbits > 0 )
        {
            for (i=0; i<acp->numsrvbits; i++)
            {
                if ( acp->srvbits[i] == srvbits )
                {
                    if ( acp->pubkeys[i] != 0 )
                        strcpy(pubkey,acp->pubkeys[i]);
                    if ( acp->acctcoinaddrs[i] != 0 )
                        strcpy(acctcoinaddr,acp->acctcoinaddrs[i]);
                    return(acp);
                }
            }
        }
    }
    return(0);
}

void add_NXT_coininfo(uint64_t srvbits,uint64_t nxt64bits,char *coinstr,char *acctcoinaddr,char *pubkey)
{
    int32_t i;
    struct NXT_acct *np = 0;
    struct acct_coin2 *acp;
    printf("add_NXT_coininfo\n");
    if ( (acp= find_NXT_coininfo(&np,nxt64bits,coinstr)) == 0 )
    {
        //np->coins[np->numcoins++] = acp = calloc(1,sizeof(*acp));
        //safecopy(acp->name,coinstr,sizeof(acp->name));
        acp = &np->coins[np->numcoins];
        memset(acp,0,sizeof(*acp));
        strcpy(acp->name,coinstr);
        np->numcoins++;
    }
    if ( acp->numsrvbits > 0 )
    {
        for (i=0; i<acp->numsrvbits; i++)
        {
            if ( acp->srvbits[i] == srvbits )
            {
                /*if ( acp->pubkeys[i] != 0 )
                {
                    if ( strcmp(pubkey,acp->pubkeys[i]) != 0 )
                        printf(">>>>>>>>>> WARNING ADDCOININFO.(%s -> %s) for %llu;%llu\n",acp->pubkeys[i],pubkey,(long long)srvbits,(long long)nxt64bits);
                    //else printf("MATCHED pubkey ");
                    free(acp->pubkeys[i]);
                    acp->pubkeys[i] = 0;
                }
                if ( acp->acctcoinaddrs[i] != 0 )
                {
                    if ( strcmp(acctcoinaddr,acp->acctcoinaddrs[i]) != 0 )
                        printf(">>>>>>>>>> WARNING ADDCOININFO.(%s -> %s) for %llu;%llu\n",acp->acctcoinaddrs[i],acctcoinaddr,(long long)srvbits,(long long)nxt64bits);
                    //else printf("MATCHED acctcoinaddr ");
                    free(acp->acctcoinaddrs[i]);
                    acp->acctcoinaddrs[i] = 0;
                }*/
                break;
            }
        }
    } else i = acp->numsrvbits;
    if ( i == acp->numsrvbits )
    {
        acp->numsrvbits++;
        //acp->srvbits = realloc(acp->srvbits,sizeof(*acp->srvbits) * acp->numsrvbits);
        //acp->acctcoinaddrs = realloc(acp->acctcoinaddrs,sizeof(*acp->acctcoinaddrs) * acp->numsrvbits);
        //acp->pubkeys = realloc(acp->pubkeys,sizeof(*acp->pubkeys) * acp->numsrvbits);
    }
    //if ( (MGW_initdone == 0 && Debuglevel > 3) || (MGW_initdone != 0 && Debuglevel > 2) )
        printf("ADDCOININFO.(%s %s) for %llu:%llu\n",acctcoinaddr,pubkey,(long long)srvbits,(long long)nxt64bits);
    acp->srvbits[i] = srvbits;
    strcpy(acp->pubkeys[i],pubkey);// = clonestr(pubkey);
    strcpy(acp->acctcoinaddrs[i],acctcoinaddr);// = clonestr(acctcoinaddr);
    if ( np != 0 )
        db777_add(1,DB_NXTaccts,&nxt64bits,sizeof(nxt64bits),np,sizeof(*np));
}


#endif
#endif
