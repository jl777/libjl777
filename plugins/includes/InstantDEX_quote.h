//
//  sha256.h
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifndef crypto777_InstantDEX_quote_h
#define crypto777_InstantDEX_quote_h

#include <stdint.h>
#include "uthash.h"

struct InstantDEX_shared { double price,vol; uint64_t quoteid,offerNXT,basebits,relbits,baseid,relid; int64_t baseamount,relamount; uint32_t timestamp; uint16_t duration:14,wallet:1,a:1,isask:1,expired:1,closed:1,swap:1,responded:1,matched:1,feepaid:1,automatch:1,pending:1,minperc:7; uint16_t minbuyin,maxbuyin; };

struct InstantDEX_quote
{
    UT_hash_handle hh;
    struct InstantDEX_shared s; // must be here
    char exchangeid,gui[9];
    char walletstr[];
};

struct InstantDEX_quote *delete_iQ(uint64_t quoteid);
struct InstantDEX_quote *find_iQ(uint64_t quoteid);
struct InstantDEX_quote *create_iQ(struct InstantDEX_quote *iQ,char *walletstr);
uint64_t calc_quoteid(struct InstantDEX_quote *iQ);
cJSON *set_walletstr(cJSON *walletitem,char *walletstr,struct InstantDEX_quote *iQ);
cJSON *InstantDEX_specialorders(uint64_t *quoteidp,uint64_t nxt64bits,char *base,char *special,uint64_t baseamount,int32_t addrtype);
int32_t bidask_parse(int32_t localaccess,struct destbuf *exchangestr,struct destbuf *name,struct destbuf *base,struct destbuf *rel,struct destbuf *gui,struct InstantDEX_quote *iQ,cJSON *json);

int32_t coin777_addrtype(uint8_t *p2shtypep,char *coinstr);

#endif
