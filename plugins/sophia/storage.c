//
//  ramchaindb.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_ramchaindb_h
#define crypto777_ramchaindb_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "uthash.h"
#include "cJSON.h"
#include "bits777.c"
#include "system777.c"
#include "msig.c"

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

struct acct_coin { uint64_t *srvbits; char name[16],**acctcoinaddrs,**pubkeys; int32_t numsrvbits; };

struct NXT_acct
{
    UT_hash_handle hh;
    struct NXT_str H;
    int32_t numcoins;
    struct acct_coin *coins[64];
    struct nodestats stats;
};

void **copy_all_DBentries(int32_t *nump,struct db777 *DB);
struct nodestats *get_nodestats(uint64_t nxt64bits);
struct acct_coin *get_NXT_coininfo(uint64_t srvbits,char *acctcoinaddr,char *pubkey,uint64_t nxt64bits,char *coinstr);
void add_NXT_coininfo(uint64_t srvbits,uint64_t nxt64bits,char *coinstr,char *acctcoinaddr,char *pubkey);
cJSON *http_search(char *destip,char *type,char *file);
struct NXT_acct *get_NXTacct(int32_t *createdp,char *NXTaddr);
void update_nodestats_data(struct nodestats *stats);

#define find_msigaddr(lenp,msigaddr) db777_findM(lenp,DB_MSIG,msigaddr)
#define save_msig(msig,len) db777_add(DB_MSIG,msig->multisigaddr,msig,len)
extern struct db777 *DB_MSIG,*DB_NXTaccts,*DB_NXTassettx,*DB_nodestats;

#endif
#else
#ifndef crypto777_ramchaindb_c
#define crypto777_ramchaindb_c


#ifndef crypto777_ramchaindb_h
#define DEFINES_ONLY
#include "storage.c"
#undef DEFINES_ONLY
#endif


void update_nodestats_data(struct nodestats *stats)
{
    char NXTaddr[64],ipaddr[64];
    expand_ipbits(ipaddr,stats->ipbits);
    expand_nxt64bits(NXTaddr,stats->nxt64bits);
    if ( Debuglevel > 2 )
        printf("Update nodestats.%s (%s) lastcontact %u\n",NXTaddr,ipaddr,stats->lastcontact);
    db777_add(DB_nodestats,NXTaddr,stats,sizeof(*stats));
}

struct NXT_acct *get_NXTacct(int32_t *createdp,char *NXTaddr)
{
    struct NXT_acct *np;
    int32_t len;
    if ( (np= db777_findM(&len,DB_NXTaccts,NXTaddr)) == 0 )
    {
        np = calloc(1,sizeof(*np));
        np->H.nxt64bits = calc_nxt64bits(NXTaddr);
        strcpy(np->H.U.NXTaddr,NXTaddr);
        db777_add(DB_NXTaccts,NXTaddr,np,sizeof(*np));
        *createdp = 1;
    } else *createdp = 0;
    return(np);
}

struct nodestats *get_nodestats(uint64_t nxt64bits)
{
    struct nodestats *stats = 0;
    int32_t createdflag;
    struct NXT_acct *np;
    char NXTaddr[64];
    if ( nxt64bits != 0 )
    {
        expand_nxt64bits(NXTaddr,nxt64bits);
        np = get_NXTacct(&createdflag,NXTaddr);
        stats = &np->stats;
        if ( stats->nxt64bits == 0 )
            stats->nxt64bits = nxt64bits;
    }
    return(stats);
}

struct NXT_assettxid *find_NXT_assettxid(int32_t *createdflagp,struct NXT_asset *ap,char *txid)
{
    int32_t createdflag;
    struct NXT_assettxid *tp;
    if ( createdflagp == 0 )
        createdflagp = &createdflag;
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

struct acct_coin *find_NXT_coininfo(struct NXT_acct **npp,uint64_t nxt64bits,char *coinstr)
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
            if ( np->coins[i] != 0 && strcmp(np->coins[i]->name,coinstr) == 0 )
                return(np->coins[i]);
    }
    return(0);
}

struct acct_coin *get_NXT_coininfo(uint64_t srvbits,char *acctcoinaddr,char *pubkey,uint64_t nxt64bits,char *coinstr)
{
    struct acct_coin *acp = 0;
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
    struct NXT_acct *np;
    struct acct_coin *acp;
    if ( (acp= find_NXT_coininfo(&np,nxt64bits,coinstr)) == 0 )
    {
        np->coins[np->numcoins++] = acp = calloc(1,sizeof(*acp));
        safecopy(acp->name,coinstr,sizeof(acp->name));
    }
    if ( acp->numsrvbits > 0 )
    {
        for (i=0; i<acp->numsrvbits; i++)
        {
            if ( acp->srvbits[i] == srvbits )
            {
                if ( acp->pubkeys[i] != 0 )
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
                }
                break;
            }
        }
    } else i = acp->numsrvbits;
    if ( i == acp->numsrvbits )
    {
        acp->numsrvbits++;
        acp->srvbits = realloc(acp->srvbits,sizeof(*acp->srvbits) * acp->numsrvbits);
        acp->acctcoinaddrs = realloc(acp->acctcoinaddrs,sizeof(*acp->acctcoinaddrs) * acp->numsrvbits);
        acp->pubkeys = realloc(acp->pubkeys,sizeof(*acp->pubkeys) * acp->numsrvbits);
    }
    if ( (MGW_initdone == 0 && Debuglevel > 3) || (MGW_initdone != 0 && Debuglevel > 2) )
        printf("ADDCOININFO.(%s %s) for %llu:%llu\n",acctcoinaddr,pubkey,(long long)srvbits,(long long)nxt64bits);
    acp->srvbits[i] = srvbits;
    acp->pubkeys[i] = clonestr(pubkey);
    acp->acctcoinaddrs[i] = clonestr(acctcoinaddr);
}

int32_t update_msig_info(struct multisig_addr *msig,int32_t syncflag,char *sender)
{
    int32_t i,ret = 0;
    if ( msig == 0 )
    {
        if ( syncflag != 0 )//&& sdb != 0 && sdb->storage != 0 )
        {
            //    return(dbsync(sdb,0));
        }
        return(0);
    }
    for (i=0; i<msig->n; i++)
        if ( msig->pubkeys[i].nxt64bits != 0 && msig->pubkeys[i].coinaddr[0] != 0 && msig->pubkeys[i].pubkey[0] != 0 )
            add_NXT_coininfo(msig->pubkeys[i].nxt64bits,calc_nxt64bits(msig->NXTaddr),msig->coinstr,msig->pubkeys[i].coinaddr,msig->pubkeys[i].pubkey);
    if ( msig->size == 0 )
        msig->size = sizeof(*msig) + (msig->n * sizeof(msig->pubkeys[0]));
    save_msig(msig,msig->size);
    if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
        printf("add (%s) NXTpubkey.(%s)\n",msig->multisigaddr,msig->NXTpubkey);
    return(ret);
}


#endif
#endif
