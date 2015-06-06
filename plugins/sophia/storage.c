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
int32_t get_NXT_coininfo(uint64_t srvbits,uint64_t nxt64bits,char *coinstr,char *acctcoinaddr,char *pubkey);
int32_t add_NXT_coininfo(uint64_t srvbits,uint64_t nxt64bits,char *coinstr,char *acctcoinaddr,char *pubkey);
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
    static struct NXT_acct N,*np;
    int32_t len = sizeof(N);
    if ( (np= db777_get(&N,&len,0,DB_NXTaccts,&nxt64bits,sizeof(nxt64bits))) == 0 )
    {
        np = calloc(1,sizeof(*np));
        np->nxt64bits = nxt64bits, expand_nxt64bits(np->NXTaddr,nxt64bits);
        db777_add(1,0,DB_NXTaccts,&nxt64bits,sizeof(nxt64bits),np,sizeof(*np));
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
    db777_add(1,0,DB_NXTaccts,&stats->nxt64bits,sizeof(stats->nxt64bits),np,sizeof(*np));
}

/*struct acct_coin2 *find_NXT_coininfo(struct NXT_acct **npp,uint64_t nxt64bits,char *coinstr)
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
}*/

#endif
#endif
