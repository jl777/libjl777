//
//  exchangepairs.h
//
//  Created by jl777 on 13/4/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifndef xcode_exchangepairs_h
#define xcode_exchangepairs_h

int32_t add_exchange_assetid(uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid,int32_t exchangeid)
{
    int32_t i;
    if ( n > 0 )
    {
        for (i=0; i<n; i+=3)
        {
            if ( baseid == assetids[i] && relid == assetids[i+1] && exchangeid == assetids[i+2] )
            {
                //printf("SKIP: %llu %llu %d %s\n",(long long)baseid,(long long)relid,exchangeid,Exchanges[exchangeid].name);
                return(n);
            }
        }
    }
    if ( Debuglevel > 2 )
        printf("ADD: %llu %llu %d %s\n",(long long)baseid,(long long)relid,exchangeid,Exchanges[exchangeid].name);
    assetids[n++] = baseid, assetids[n++] = relid, assetids[n++] = exchangeid;
    return(n);
}

int32_t _add_related(uint64_t *assetids,int32_t n,uint64_t refbaseid,uint64_t refrelid,int32_t exchangeid,uint64_t baseid,uint64_t relid)
{
    if ( baseid == refbaseid || baseid == refrelid || relid == refbaseid || relid == refrelid )
        n = add_exchange_assetid(assetids,n,refbaseid,refrelid,exchangeid);
    return(n);
}

int32_t add_related(uint64_t *assetids,int32_t n,uint64_t supported[][2],long num,int32_t exchangeid,uint64_t baseid,uint64_t relid)
{
    int32_t i;
    for (i=0; i<num; i++)
        n = _add_related(assetids,n,supported[i][0],supported[i][1],exchangeid,baseid,relid);
    return(n);
}

int32_t add_exchange_symbolmap(uint64_t *assetids,int32_t n,uint64_t refbits,uint64_t assetid,int32_t exchangeid,char *symbolmap[][8],int32_t numsymbols)
{
    int32_t i,j;
    char assetidstr[64];
    uint64_t namebits;
    expand_nxt64bits(assetidstr,assetid);
    for (i=0; i<numsymbols; i++)
    {
        namebits = stringbits(symbolmap[i][0]);
        for (j=1; j<8; j++)
        {
            if ( symbolmap[i][j] == 0 || symbolmap[i][j][0] == 0 )
                break;
            //printf("{%s %s} ",symbolmap[i][j],assetidstr);
            if ( strcmp(symbolmap[i][j],assetidstr) == 0 )
                n = add_exchange_assetid(assetids,n,namebits,refbits,exchangeid);
        }
    }
    //printf("(assetsearch.%s) numsymbols.%d\n",assetidstr,numsymbols);
    return(n);
}

int32_t add_exchange_assetids(uint64_t *assetids,int32_t n,uint64_t refassetid,uint64_t baseid,uint64_t relid,int32_t exchangeid,char *symbolmap[][8],int32_t numsymbols)
{
    char name[64];
    //printf("(%s) ref.%llu (%llu/%llu) numsymbols.%d\n",Exchanges[exchangeid].name,(long long)refassetid,(long long)baseid,(long long)relid,numsymbols);
    if ( baseid != refassetid )
    {
        if ( symbolmap != 0 && numsymbols > 0 )
            n = add_exchange_symbolmap(assetids,n,refassetid,baseid,exchangeid,symbolmap,numsymbols);
        if ( is_native_crypto(name,baseid) > 0 )
            n = add_exchange_assetid(assetids,n,baseid,refassetid,exchangeid);
    }
    if ( relid != refassetid )
    {
        if ( symbolmap != 0 && numsymbols > 0 )
            n = add_exchange_symbolmap(assetids,n,refassetid,relid,exchangeid,symbolmap,numsymbols);
        if ( is_native_crypto(name,relid) > 0 )
            n = add_exchange_assetid(assetids,n,relid,refassetid,exchangeid);
    }
    return(n);
}

int32_t add_NXT_assetids(uint64_t *assetids,int32_t n,uint64_t assetid)
{
    int32_t i,m;
    uint64_t equivids[256];
    if ( (m= get_equivalent_assetids(equivids,assetid)) > 0 )
    {
        for (i=0; i<m; i++)
            n = add_exchange_assetid(assetids,n,assetid,NXT_ASSETID,INSTANTDEX_NXTAEID);
    }
    else n = add_exchange_assetid(assetids,n,assetid,NXT_ASSETID,INSTANTDEX_NXTAEID);
    return(n);
}

int32_t NXT_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    if ( baseid != NXT_ASSETID )
        n = add_NXT_assetids(assetids,n,baseid);
    if ( relid != NXT_ASSETID )
        n = add_NXT_assetids(assetids,n,relid);
    return(n);
}

int32_t poloniex_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    return(add_exchange_assetids(assetids,n,BTC_ASSETID,baseid,relid,exchangeid,poloassets,(int32_t)(sizeof(poloassets)/sizeof(*poloassets))));
}

int32_t bter_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    uint64_t unityid = calc_nxt64bits("12071612744977229797");
    n = add_exchange_assetids(assetids,n,BTC_ASSETID,baseid,relid,exchangeid,bterassets,(int32_t)(sizeof(bterassets)/sizeof(*bterassets)));
    if ( baseid == unityid || relid == unityid )
    {
        n = add_exchange_assetid(assetids,n,unityid,BTC_ASSETID,exchangeid);
        n = add_exchange_assetid(assetids,n,unityid,NXT_ASSETID,exchangeid);
        n = add_exchange_assetid(assetids,n,unityid,CNY_ASSETID,exchangeid);
    }
    return(n);
}

int32_t bittrex_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    return(add_exchange_assetids(assetids,n,BTC_ASSETID,baseid,relid,exchangeid,0,0));
}

int32_t btc38_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    return(add_exchange_assetids(assetids,n,BTC_ASSETID,baseid,relid,exchangeid,0,0));
    return(add_exchange_assetids(assetids,n,CNY_ASSETID,baseid,relid,exchangeid,0,0));
}

int32_t okcoin_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    uint64_t supported[][2] = { {BTC_ASSETID, USD_ASSETID}, {LTC_ASSETID, USD_ASSETID} };
    return(add_related(assetids,n,supported,sizeof(supported)/sizeof(*supported),exchangeid,baseid,relid));
}

int32_t huobi_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    return(_add_related(assetids,n,BTC_ASSETID,CNY_ASSETID,exchangeid,baseid,relid));
}

int32_t bitfinex_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    uint64_t supported[][2] = { {BTC_ASSETID, USD_ASSETID}, {LTC_ASSETID, USD_ASSETID}, {LTC_ASSETID, BTC_ASSETID}, {DASH_ASSETID, BTC_ASSETID}, {DASH_ASSETID, USD_ASSETID} };
    return(add_related(assetids,n,supported,sizeof(supported)/sizeof(*supported),exchangeid,baseid,relid));
}

int32_t btce_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    uint64_t supported[][2] = { {BTC_ASSETID, USD_ASSETID}, {LTC_ASSETID, BTC_ASSETID}, {LTC_ASSETID, USD_ASSETID}, {BTC_ASSETID, RUR_ASSETID}, {PPC_ASSETID, BTC_ASSETID}, {PPC_ASSETID, USD_ASSETID}, {LTC_ASSETID, RUR_ASSETID}, {NMC_ASSETID, BTC_ASSETID}, {NMC_ASSETID, USD_ASSETID} };
    return(add_related(assetids,n,supported,sizeof(supported)/sizeof(*supported),exchangeid,baseid,relid));
}

int32_t bityes_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    uint64_t supported[][2] = { {BTC_ASSETID, USD_ASSETID}, {LTC_ASSETID, USD_ASSETID} };
    return(add_related(assetids,n,supported,sizeof(supported)/sizeof(*supported),exchangeid,baseid,relid));
}

int32_t coinbase_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    uint64_t supported[][2] = { {BTC_ASSETID, USD_ASSETID} };
    return(add_related(assetids,n,supported,sizeof(supported)/sizeof(*supported),exchangeid,baseid,relid));
}

int32_t bitstamp_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    uint64_t supported[][2] = { {BTC_ASSETID, USD_ASSETID} };
    return(add_related(assetids,n,supported,sizeof(supported)/sizeof(*supported),exchangeid,baseid,relid));
}

int32_t lakebtc_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    uint64_t supported[][2] = { {BTC_ASSETID, USD_ASSETID} };
    return(add_related(assetids,n,supported,sizeof(supported)/sizeof(*supported),exchangeid,baseid,relid));
}

int32_t exmo_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    uint64_t supported[][2] = { {BTC_ASSETID, USD_ASSETID}, {BTC_ASSETID, EUR_ASSETID} , {BTC_ASSETID, RUR_ASSETID} };
    return(add_related(assetids,n,supported,sizeof(supported)/sizeof(*supported),exchangeid,baseid,relid));
}

/*int32_t kraken_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    uint64_t supported[][2] = { {BTC_ASSETID, USD_ASSETID}, {BTC_ASSETID, EUR_ASSETID} };
    return(add_related(assetids,n,supported,sizeof(supported)/sizeof(*supported),exchangeid,baseid,relid));
}*/

#endif

