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
    char NXTaddr[24];
    uint64_t nxt64bits;
    int32_t numcoins;
    struct acct_coin *coins[64];
    struct nodestats stats;
};

struct nodestats *get_nodestats(uint64_t nxt64bits);
struct acct_coin *get_NXT_coininfo(uint64_t srvbits,char *acctcoinaddr,char *pubkey,uint64_t nxt64bits,char *coinstr);
void add_NXT_coininfo(uint64_t srvbits,uint64_t nxt64bits,char *coinstr,char *acctcoinaddr,char *pubkey);
cJSON *http_search(char *destip,char *type,char *file);
struct NXT_acct *get_NXTacct(int32_t *createdp,char *NXTaddr);
void update_nodestats_data(struct nodestats *stats);
void set_NXTpubkey(char *NXTpubkey,char *NXTacct);
struct multisig_addr *find_NXT_msig(char *NXTaddr,char *coinstr,uint64_t *srv64bits,int32_t n);
int32_t update_msig_info(struct multisig_addr *msig,int32_t syncflag,char *sender);

#endif
#else
#ifndef crypto777_storage_c
#define crypto777_storage_c


#ifndef crypto777_storage_h
#define DEFINES_ONLY
#include "storage.c"
#undef DEFINES_ONLY
#endif

struct NXT_acct *get_NXTacct(int32_t *createdp,char *NXTaddr)
{
    struct NXT_acct *np;
    int32_t len;
    if ( (np= db777_findM(&len,DB_NXTaccts,NXTaddr)) == 0 )
    {
        np = calloc(1,sizeof(*np));
        np->nxt64bits = calc_nxt64bits(NXTaddr);
        strcpy(np->NXTaddr,NXTaddr);
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

void update_nodestats_data(struct nodestats *stats)
{
    char NXTaddr[64],ipaddr[64];
    int32_t createdflag;
    struct NXT_acct *np;
    expand_ipbits(ipaddr,stats->ipbits);
    expand_nxt64bits(NXTaddr,stats->nxt64bits);
    np = get_NXTacct(&createdflag,NXTaddr);
    np->stats = *stats;
    if ( Debuglevel > 2 )
        printf("Update nodestats.%s (%s) lastcontact %u\n",NXTaddr,ipaddr,stats->lastcontact);
    db777_add(DB_NXTaccts,NXTaddr,np,sizeof(*np));
}

void set_NXTpubkey(char *NXTpubkey,char *NXTacct)
{
    static uint8_t zerokey[256>>3];
    struct nodestats *stats;
    char NXTaddr[64];
    uint64_t nxt64bits;
    int32_t createdflag;
    struct NXT_acct *np;
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
    db777_add(DB_NXTaccts,NXTaddr,np,sizeof(*np));
    if ( NXTpubkey != 0 )
    {
        int32_t init_hexbytes_noT(char *hexbytes,unsigned char *message,long len);
        init_hexbytes_noT(NXTpubkey,pubkey.bytes,sizeof(pubkey));
    }
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
    struct NXT_acct *np = 0;
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
    //if ( (MGW_initdone == 0 && Debuglevel > 3) || (MGW_initdone != 0 && Debuglevel > 2) )
        printf("ADDCOININFO.(%s %s) for %llu:%llu\n",acctcoinaddr,pubkey,(long long)srvbits,(long long)nxt64bits);
    acp->srvbits[i] = srvbits;
    acp->pubkeys[i] = clonestr(pubkey);
    acp->acctcoinaddrs[i] = clonestr(acctcoinaddr);
    if ( np != 0 )
        db777_add(DB_NXTaccts,np->NXTaddr,np,sizeof(*np));
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
    if ( msig->multisigaddr[0] == 0 )
        return(-1);
    for (i=0; i<msig->n; i++)
        if ( msig->pubkeys[i].nxt64bits != 0 && msig->pubkeys[i].coinaddr[0] != 0 && msig->pubkeys[i].pubkey[0] != 0 )
            add_NXT_coininfo(msig->pubkeys[i].nxt64bits,calc_nxt64bits(msig->NXTaddr),msig->coinstr,msig->pubkeys[i].coinaddr,msig->pubkeys[i].pubkey);
    if ( msig->size == 0 )
        msig->size = sizeof(*msig) + (msig->n * sizeof(msig->pubkeys[0]));
    int32_t save_msigaddr(char *coinstr,char *NXTaddr,struct multisig_addr *msig,int32_t len);
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
    if ( (msigs= (struct multisig_addr **)db777_copy_all(&nummsigs,DB_msigs)) != 0 )
    {
        nxt64bits = (NXTaddr != 0) ? calc_nxt64bits(NXTaddr) : 0;
        for (i=0; i<nummsigs; i++)
        {
            printf("i.%d of nummsigs.%d: %p srvbits.%p n.(%d %d) %p\n",i,nummsigs,msigs[i],srv64bits,msigs[i]->valid,msigs[i]->n,msigs[i]);
            if ( msigs[i]->valid != msigs[i]->n && msigs[i]->valid < n && msigs[i]->n == n )
            {
                struct multisig_addr *finalize_msig(struct multisig_addr *msig,uint64_t *srvbits,uint64_t refbits);
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

#endif
#endif
