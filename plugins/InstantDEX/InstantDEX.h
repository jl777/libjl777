//
//  InstantDEX.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_InstantDEX_h
#define xcode_InstantDEX_h

// placeorder assetA, then for assetB causes timeouts
#define _issue_curl(curl_handle,label,url) bitcoind_RPC(curl_handle,label,url,0,0,0)

#define ORDERBOOK_EXPIRATION 3600
#define INSTANTDEX_MINVOL 75
#define INSTANTDEX_MINVOLPERC ((double)INSTANTDEX_MINVOL / 100.)
#define INSTANTDEX_PRICESLIPPAGE 0.001

#define INSTANTDEX_TRIGGERDEADLINE 15
#define JUMPTRADE_SECONDS 100
#define INSTANTDEX_ACCT "4383817337783094122"
#define INSTANTDEX_FEE ((long)(2.5 * SATOSHIDEN))

#define INSTANTDEX_NAME "InstantDEX"
#define INSTANTDEX_NXTAENAME "nxtae"
#define INSTANTDEX_NXTAEUNCONF "unconf"
#define INSTANTDEX_EXCHANGEID 0
#define INSTANTDEX_UNCONFID 1
#define INSTANTDEX_NXTAEID 2
#define MAX_EXCHANGES 64

struct InstantDEX_info INSTANTDEX;
struct pingpong_queue Pending_offersQ;

struct rambook_info
{
    UT_hash_handle hh;
    FILE *fp;
    char url[128],base[16],rel[16],lbase[16],lrel[16],exchange[32],gui[16];
    struct InstantDEX_quote **quotes;
    uint64_t assetids[3];
    uint32_t lastaccess;
    int32_t numquotes,maxquotes,numupdates;
    float lastmilli;
    uint8_t updated;
} *Rambooks;

char *exchange_str(int32_t exchangeid);
struct exchange_info *find_exchange(int32_t *exchangeidp,char *exchangestr);
void prices777_poll(uint64_t baseid,uint64_t relid);

void ramparse_stub(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    printf("unexpected call to ramparse_stub gui.%s maxdepth.%d\n",gui,maxdepth);
}

char *submit_respondtx(char *respondtxstr,uint64_t nxt64bits,char *NXTACCTSECRET,uint64_t dest64bits)
{
    uint32_t nonce; char destNXT[64];
    expand_nxt64bits(destNXT,dest64bits);
    printf("submit_respondtx.(%s) -> dest.%llu\n",respondtxstr,(long long)dest64bits);
    return(busdata_sync(&nonce,respondtxstr,"allnodes",destNXT));
}

int32_t calc_users_maxopentrades(uint64_t nxt64bits)
{
    int32_t is_exchange_nxt64bits(uint64_t nxt64bits);
    if ( is_exchange_nxt64bits(nxt64bits) != 0 )
        return(1000);
    return(13);
}

int32_t time_to_nextblock(int32_t lookahead)
{
    /*
     sha256(lastblockgensig+publickey)[0-7] / basetarget * effective balance
     first 8 bytes of sha256 to make a long
     that gives you seconds to forge block
     or you can just look if base target is below 90% and adjust accordingly
     */
    // http://jnxt.org/forge/forgers.json
    return(lookahead * 600); // clearly need to do some calcs
}

#include "assetids.h"
#include "NXT_tx.h"
#include "quotes.h"

void update_openorder(struct InstantDEX_quote *iQ,uint64_t quoteid,struct NXT_tx *txptrs[],int32_t numtx,int32_t updateNXT) // from poll_pending_offers via main
{
    char *check_ordermatch(char *NXTaddr,char *NXTACCTSECRET,struct InstantDEX_quote *refiQ);
    char *retstr;
return;
    printf("update_openorder iQ.%llu with numtx.%d updateNXT.%d | expires in %ld\n",(long long)iQ->quoteid,numtx,updateNXT,iQ->timestamp+iQ->duration-time(NULL));
    if ( (SUPERNET.automatch & 2) != 0 && (retstr= check_ordermatch(SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,iQ)) != 0 )
    {
        printf("automatched order!\n");
        free(retstr);
    }
}

char *fill_nxtae(uint64_t nxt64bits,int32_t dir,double price,double volume,uint64_t baseid,uint64_t relid)
{
    uint64_t txid,assetid,avail,qty,priceNQT,ap_mult; char retbuf[512],*errstr;
    if ( nxt64bits != calc_nxt64bits(SUPERNET.NXTADDR) )
        return(clonestr("{\"error\":\"must use your NXT address\"}"));
    else if ( baseid == NXT_ASSETID )
        dir = -dir, assetid = relid;
    else if ( relid == NXT_ASSETID )
        assetid = baseid;
    else return(clonestr("{\"error\":\"NXT AE order without NXT\"}"));
    if ( (ap_mult= get_assetmult(assetid)) == 0 )
        return(clonestr("{\"error\":\"assetid not found\"}"));
    qty = calc_asset_qty(&avail,&priceNQT,SUPERNET.NXTADDR,0,assetid,price,volume);
    txid = submit_triggered_nxtae(&errstr,0,dir > 0 ? "placeBidOrder" : "placeAskOrder",nxt64bits,SUPERNET.NXTACCTSECRET,assetid,qty,priceNQT,0,0,0,0);
    if ( errstr != 0 )
        sprintf(retbuf,"{\"error\":\"%s\"}",errstr), free(errstr);
    else sprintf(retbuf,"{\"result\":\"success\",\"txid\":\"%llu\"}",(long long)txid);
    return(clonestr(retbuf));
}

#include "rambooks.h"
//#include "exchanges.h"
#include "orderbooks.h"
#include "trades.h"
#include "atomic.h"
//#include "bars.h"
//#include "signals.h"

char *check_ordermatch(char *NXTaddr,char *NXTACCTSECRET,struct InstantDEX_quote *refiQ) // called by placequote, should autofill
{
    struct orderbook *op,*obooks[32]; char base[16],rel[16]; uint64_t mult; int32_t i,besti,n=0,dir = 1; struct InstantDEX_quote *iQ,*quotes;
    uint64_t assetA,amountA,assetB,amountB; char jumpstr[1024],otherNXTaddr[64],exchange[64],*retstr = 0;
    double refprice,refvol,price,vol,metric,perc,bestmetric = 0.;
    //struct InstantDEX_quote { uint64_t nxt64bits,baseamount,relamount,type; uint32_t timestamp; char exchange[9]; uint8_t closed:1,sent:1,matched:1,isask:1; };
    //update_rambooks(refiQ->baseid,refiQ->relid,DEFAULT_MAXDEPTH,refiQ->gui,1,0);
    set_assetname(&mult,base,refiQ->baseid), set_assetname(&mult,rel,refiQ->relid);
    besti = -1;
    if ( refiQ->isask != 0 )
        dir = -1;
    if ( (refprice= calc_price_volume(&refvol,refiQ->baseamount,refiQ->relamount)) <= SMALLVAL )
        return(clonestr("{\"error\":\"invalid price volume calc\"}"));
    printf("%s dir.%d check_ordermatch(%llu %.8f | %llu %.8f) ref %.8f vol %.8f\n",NXTaddr,dir,(long long)refiQ->baseid,dstr(refiQ->baseamount),(long long)refiQ->relid,dstr(refiQ->relamount),refprice,refvol);
    if ( (op= make_orderbook(obooks,sizeof(obooks)/sizeof(*obooks),base,refiQ->baseid,rel,refiQ->relid,DEFAULT_MAXDEPTH,0,refiQ->gui,0)) != 0 )
    {
        if ( dir > 0 && (n= op->numasks) != 0 )
            quotes = op->asks;
        else if ( dir < 0 && (n= op->numbids) != 0 )
            quotes = op->bids;
        if ( n > 0 )
        {
            for (i=0; i<n; i++)
            {
                iQ = &quotes[i];
                expand_nxt64bits(otherNXTaddr,iQ->nxt64bits);
                if ( iQ->closed != 0 )
                    continue;
                if ( 0 && iQ->exchangeid == INSTANTDEX_EXCHANGEID && is_unfunded_order(iQ->nxt64bits,dir > 0 ? iQ->baseid : iQ->relid,dir > 0 ? iQ->baseamount : iQ->relamount) != 0 )
                {
                    iQ->closed = 1;
                    printf("found unfunded order!\n");
                    continue;
                }
                iQ_exchangestr(exchange,iQ);
                if ( strcmp(otherNXTaddr,NXTaddr) != 0 && iQ->matched == 0 && iQ->exchangeid == INSTANTDEX_NXTAEID )
                {
                    price = calc_price_volume(&vol,iQ->baseamount,iQ->relamount);
                    if ( vol > (refvol * INSTANTDEX_MINVOLPERC) && refvol > (vol * iQ->minperc * .01) )
                    {
                        if ( vol < refvol )
                            metric = (vol / refvol);
                        else metric = 1.;
                        if ( dir > 0 && price < (refprice * (1. + INSTANTDEX_PRICESLIPPAGE) + SMALLVAL) )
                            metric *= (1. + (refprice - price)/refprice);
                        else if ( dir < 0 && price > (refprice * (1. - INSTANTDEX_PRICESLIPPAGE) - SMALLVAL) )
                            metric *= (1. + (price - refprice)/refprice);
                        else metric = 0.;
                        if ( metric != 0. )
                        {
                            printf("matchedflag.%d exchange.(%s) %llu/%llu from (%s) | ",iQ->matched,exchange,(long long)iQ->baseamount,(long long)iQ->relamount,otherNXTaddr);
                            printf("price %.8f vol %.8f | %.8f > %.8f? %.8f > %.8f?\n",price,vol,vol,(refvol * INSTANTDEX_MINVOLPERC),refvol,(vol * INSTANTDEX_MINVOLPERC));
                            printf("price %f against %f or %f\n",price,(refprice * (1. + INSTANTDEX_PRICESLIPPAGE) + SMALLVAL),(refprice * (1. - INSTANTDEX_PRICESLIPPAGE) - SMALLVAL));
                            printf("metric %f\n",metric);
                        }
                        if ( metric > bestmetric )
                        {
                            bestmetric = metric;
                            besti = i;
                        }
                    }
                }// else printf("NXTaddr.%s: skip %s from %s\n",NXTaddr,exchange,otherNXTaddr);
            }
        }
        //printf("n.%d\n",n);
        if ( besti >= 0 )
        {
            iQ = &quotes[besti];
            jumpstr[0] = 0;
            if ( dir < 0 )
            {
                assetA = refiQ->relid;
                amountA = iQ->relamount;
                assetB = refiQ->baseid;
                amountB = iQ->baseamount;
            }
            else
            {
                assetB = refiQ->relid;
                amountB = iQ->relamount;
                assetA = refiQ->baseid;
                amountA = iQ->baseamount;
            }
            price = calc_price_volume(&vol,iQ->baseamount,iQ->relamount);
            iQ_exchangestr(exchange,iQ);
            expand_nxt64bits(otherNXTaddr,iQ->nxt64bits);
            perc = 100.;
            if ( perc >= iQ->minperc )
                retstr = makeoffer3(NXTaddr,NXTACCTSECRET,price,refvol,0,perc,refiQ->baseid,refiQ->relid,iQ->baseiQ,iQ->reliQ,iQ->quoteid,dir > 0,exchange,iQ->baseamount*refvol/vol,iQ->relamount*refvol/vol,iQ->nxt64bits,iQ->minperc,get_iQ_jumpasset(iQ));
        } else printf("besti.%d\n",besti);
        free_orderbooks(obooks,sizeof(obooks)/sizeof(*obooks),op);
    } else printf("cant make orderbook\n");
    return(retstr);
}

char *lottostats_func(int32_t localaccess,int32_t valid,char *sender,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char buf[MAX_JSON_FIELD];
    uint64_t bestMMbits;
    int32_t totaltickets=0,numtickets=0;
    uint32_t firsttimestamp;
    if ( localaccess == 0 )
        return(0);
    firsttimestamp = (uint32_t)get_API_int(objs[0],0);
    bestMMbits = 0;//find_best_market_maker(&totaltickets,&numtickets,NXTaddr,firsttimestamp);
    sprintf(buf,"{\"result\":\"lottostats\",\"totaltickets\":\"%d\",\"NXT\":\"%s\",\"numtickets\":\"%d\",\"odds\":\"%.2f\",\"topMM\":\"%llu\"}",totaltickets,SUPERNET.NXTADDR,numtickets,numtickets == 0 ? 0 : (double)totaltickets / numtickets,(long long)bestMMbits);
    return(clonestr(buf));
}

char *placequote_str(struct InstantDEX_quote *iQ)
{
    char iQstr[1024],exchangestr[64],buf[MAX_JSON_FIELD];
    init_hexbytes_noT(iQstr,(uint8_t *)iQ,sizeof(*iQ));
    iQ_exchangestr(exchangestr,iQ);
    sprintf(buf,"{\"result\":\"success\",\"quoteid\":\"%llu\",\"baseid\":\"%llu\",\"baseamount\":\"%llu\",\"relid\":\"%llu\",\"relamount\":\"%llu\",\"offerNXT\":\"%llu\",\"baseiQ\":\"%llu\",\"reliQ\":\"%llu\",\"timestamp\":\"%u\",\"isask\":\"%u\",\"exchange\":\"%s\",\"gui\":\"%s\",\"iQdata\":\"%s\"}",(long long)iQ->quoteid,(long long)iQ->baseid,(long long)iQ->baseamount,(long long)iQ->relid,(long long)iQ->relamount,(long long)iQ->nxt64bits,(long long)calc_quoteid(iQ->baseiQ),(long long)calc_quoteid(iQ->reliQ),iQ->timestamp,iQ->isask,exchangestr,iQ->gui,iQstr);
    return(clonestr(buf));
}

char *submitquote_str(int32_t localaccess,struct InstantDEX_quote *iQ,uint64_t baseid,uint64_t relid)
{
    cJSON *json;
    char *jsonstr = 0;
    uint64_t basetmp,reltmp;
    if ( (json= gen_InstantDEX_json(localaccess,&basetmp,&reltmp,0,iQ->isask,iQ,baseid,relid,0)) != 0 )
    {
        ensure_jsonitem(json,"plugin","relay");
        ensure_jsonitem(json,"destplugin","InstantDEX");
        ensure_jsonitem(json,"method",(iQ->isask != 0) ? "ask" : "bid");
        jsonstr = cJSON_Print(json), _stripwhite(jsonstr,' ');
        free_json(json);
    } else printf("gen_InstantDEX_json returns null\n");
    return(jsonstr);
}

char *placequote_func(char *NXTaddr,char *NXTACCTSECRET,int32_t localaccess,int32_t dir,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    uint64_t baseamount,relamount,nxt64bits,baseid,relid,quoteid = 0; double price,volume,minbasevol,minrelvol; uint32_t timestamp,nonce;
    uint8_t minperc; struct exchange_info *xchg; struct InstantDEX_quote iQ;
    int32_t remoteflag,automatch,duration,exchangeid; struct rambook_info *rb;
    char buf[MAX_JSON_FIELD],offerNXT[MAX_JSON_FIELD],gui[MAX_JSON_FIELD],exchangestr[MAX_JSON_FIELD],base[16],rel[16],*str,*jsonstr,*retstr = 0;
    if ( (xchg= find_exchange(&exchangeid,INSTANTDEX_NAME)) == 0 || exchangeid != INSTANTDEX_EXCHANGEID )
        return(clonestr("{\"error\":\"unexpected InstantDEX exchangeid\"}"));
    remoteflag = (localaccess == 0);
    nxt64bits = calc_nxt64bits(sender);
    baseid = get_API_nxt64bits(objs[0]);
    relid = get_API_nxt64bits(objs[1]);
    if ( baseid == 0 || relid == 0 || baseid == relid )
        return(clonestr("{\"error\":\"illegal asset id\"}"));
    baseamount = get_API_nxt64bits(objs[5]);
    relamount = get_API_nxt64bits(objs[6]);
    if ( baseamount != 0 && relamount != 0 )
        price = calc_price_volume(&volume,baseamount,relamount);
    else
    {
        volume = get_API_float(objs[2]);
        price = get_API_float(objs[3]);
        set_best_amounts(&baseamount,&relamount,price,volume);
    }
    memset(&iQ,0,sizeof(iQ));
    timestamp = (uint32_t)get_API_int(objs[4],0);
    copy_cJSON(gui,objs[7]), gui[sizeof(iQ.gui)-1] = 0;
    automatch = (int32_t)get_API_int(objs[8],1);
    minperc = (int32_t)get_API_int(objs[9],0);
    duration = (int32_t)get_API_int(objs[10],ORDERBOOK_EXPIRATION);
    if ( duration <= 0 || duration > ORDERBOOK_EXPIRATION )
        duration = ORDERBOOK_EXPIRATION;
    copy_cJSON(exchangestr,objs[11]);
    copy_cJSON(offerNXT,objs[12]);
printf("placequote localaccess.%d dir.%d exchangestr.(%s)\n",localaccess,dir,exchangestr);
    if ( exchangestr[0] == 0 )
        strcpy(exchangestr,INSTANTDEX_NAME);
    else
    {
        if ( remoteflag != 0 && strcmp("InstantDEX",exchangestr) != 0 )
        {
            printf("remote node (%d) (%s) trying to place quote to exchange (%s)\n",localaccess,sender,exchangestr);
            return(clonestr("{\"error\":\"no remote exchange orders: you cannot submit an order from a remote node\"}"));
        }
        else if ( strcmp(exchangestr,"nxtae") == 0 )
            return(fill_nxtae(nxt64bits,dir,price,volume,baseid,relid));
        else if ( strcmp(exchangestr,"InstantDEX") != 0 )
        {
            char *prices777_trade(char *exchangestr,char *base,char *rel,int32_t dir,double price,double volume);
            if ( is_native_crypto(base,baseid) > 0 && is_native_crypto(rel,relid) > 0 && price > 0 && volume > 0 && dir != 0 )
                return(prices777_trade(exchangestr,base,rel,dir,price,volume));
            else return(clonestr("{\"error\":\"illegal parameter baseid or relid not crypto or invalid price\"}\n"));
        } //else printf("alternate else case.(%s)\n",exchangestr);
    }
    if ( Debuglevel > 1 )
        printf("NXT.%s t.%u placequote dir.%d sender.(%s) valid.%d price %.8f vol %.8f %llu/%llu\n",NXTaddr,timestamp,dir,sender,valid,price,volume,(long long)baseamount,(long long)relamount);
    memset(base,0,sizeof(base)), memset(rel,0,sizeof(rel)), is_native_crypto(base,baseid), is_native_crypto(rel,relid);
    prices777_poll(baseid,relid);
    minbasevol = get_minvolume(baseid), minrelvol = get_minvolume(relid);
    if ( volume < minbasevol || (volume * price) < minrelvol )
    {
        sprintf(buf,"{\"error\":\"not enough volume\",\"price\":%f,\"volume\":%f,\"minbasevol\":%f,\"minrelvol\":%f,\"relvol\":%f}",price,volume,minbasevol,minrelvol,price*volume);
        return(clonestr(buf));
    }
    if ( sender[0] != 0 && valid > 0 )
    {
        if ( price != 0. && volume != 0. && dir != 0 )
        {
            if ( (rb= add_rambook_quote(INSTANTDEX_NAME,&iQ,nxt64bits,timestamp,dir,baseid,relid,price,volume,baseamount,relamount,gui,0,duration)) != 0 )
            {
                iQ.minperc = minperc;
                if ( (quoteid= calc_quoteid(&iQ)) != 0 )
                {
                    retstr = placequote_str(&iQ);
                    if ( Debuglevel > 2 )
                        printf("placequote.(%s) remoteflag.%d\n",retstr,remoteflag);
                }
                if ( (jsonstr= submitquote_str(localaccess,&iQ,baseid,relid)) != 0 )
                {
                    printf("got submitquote_str.(%s)\n",jsonstr);
                    if ( remoteflag == 0 )
                    {
                        if ( automatch != 0 && (SUPERNET.automatch & 1) != 0 && (retstr= check_ordermatch(NXTaddr,NXTACCTSECRET,&iQ)) != 0 )
                        {
                            free(jsonstr);
                            return(retstr);
                        } else printf("skip automatch.%d %d\n",automatch,SUPERNET.automatch);
                        if ( (str= busdata_sync(&nonce,jsonstr,"allnodes",0)) != 0 )
                            free(str);
                        retstr = jsonstr;
                    } else return(clonestr("{\"result\":\"updated rambook\"}"));
                } else printf("not submitquote_str\n");
            } else return(clonestr("{\"error\":\"cant get price close enough due to limited decimals\"}"));
        }
        if ( retstr == 0 )
        {
            sprintf(buf,"{\"error submitting\":\"place%s error %llu/%llu volume %f price %f\"}",dir>0?"bid":"ask",(long long)baseid,(long long)relid,volume,price);
            retstr = clonestr(buf);
        }
    }
    else
    {
        sprintf(buf,"{\"error\":\"place%s error %llu/%llu dir.%d volume %f price %f\"}",dir>0?"bid":"ask",(long long)baseid,(long long)relid,dir,volume,price);
        retstr = clonestr(buf);
    }
//printf("placequote.(%s)\n",retstr);
    return(retstr);
}

char *placebid_func(int32_t localaccess,int32_t valid,char *sender,cJSON **objs,int32_t numobjs,char *origargstr)
{
    return(placequote_func(SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,localaccess,1,SUPERNET.NXTADDR,valid,objs,numobjs,origargstr));
}

char *placeask_func(int32_t localaccess,int32_t valid,char *sender,cJSON **objs,int32_t numobjs,char *origargstr)
{
    return(placequote_func(SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,localaccess,-1,SUPERNET.NXTADDR,valid,objs,numobjs,origargstr));
}

/**/

char *bid_func(int32_t localaccess,int32_t valid,char *sender,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char offerNXT[MAX_JSON_FIELD];
    copy_cJSON(offerNXT,objs[12]);
    printf("bid_func %s vs offerNXT %s\n",SUPERNET.NXTADDR,offerNXT);
    if ( strcmp(SUPERNET.NXTADDR,offerNXT) != 0 )
        return(placequote_func(SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,0,1,sender,valid,objs,numobjs,origargstr));
    else return(0);
}

char *ask_func(int32_t localaccess,int32_t valid,char *sender,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char offerNXT[MAX_JSON_FIELD];
    copy_cJSON(offerNXT,objs[12]);
    printf("ask_func %s vs offerNXT %s\n",SUPERNET.NXTADDR,offerNXT);
    if ( strcmp(SUPERNET.NXTADDR,offerNXT) != 0 )
        return(placequote_func(SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,0,-1,sender,valid,objs,numobjs,origargstr));
    else return(0);
}

uint64_t PTL_placebid(char *base,char *rel,double price,double volume)
{
    printf("placebid(%s/%s %.8f vol %.6f)\n",base,rel,price,volume);
    return(0);
}

uint64_t PTL_placeask(char *base,char *rel,double price,double volume)
{
    printf("placeask(%s/%s %.8f vol %.6f)\n",base,rel,price,volume);
    return(0);
}

int32_t PTL_makeoffer(char *base,char *rel,double price,double volume,uint64_t nxtaddr)
{
    printf("makeoffer(%s/%s %.8f vol %.6f to NXT.%llu\n",base,rel,price,volume,(long long)nxtaddr);
    return(0);
}

double calc_precision(uint64_t baseid,uint64_t relid)
{
    double precision;
    int32_t i,n,basedecimals,reldecimals;
    uint64_t mult;
    if ( baseid == NXT_ASSETID )
        return(.99 / get_assetmult(relid));
    else if ( relid == NXT_ASSETID )
        return(.99 / get_assetmult(baseid));
    basedecimals = get_assetdecimals(baseid), reldecimals = get_assetdecimals(relid);
    if ( basedecimals < reldecimals )
        n = basedecimals;
    else n = reldecimals;
    mult = 1;
    for (i=0; i<n; i++)
        mult *= 10;
    precision = 1.5 / mult;
    //printf("basedecimals %u reldecimals %u -> precision %f\n",basedecimals,reldecimals,precision);
    return(precision);
}
           
double verify_orderbook_json(double prevprice,int32_t i,int32_t askflag,cJSON *item,uint64_t baseid,uint64_t relid)
{
    int32_t askoffer,errs = 0;
    double price,volume,calcprice,calcvolume,precision,change;
    uint64_t baseamount,relamount;
    if ( (baseid == NXT_ASSETID || relid == NXT_ASSETID) && (cJSON_GetObjectItem(item,"baseiQ") != 0 || cJSON_GetObjectItem(item,"reliQ") != 0) )
        errs++, printf("nonzero baseiQ or reliQ\n");
    precision = calc_precision(baseid,relid);
    baseamount = get_API_nxt64bits(cJSON_GetObjectItem(item,"baseamount")), relamount = get_API_nxt64bits(cJSON_GetObjectItem(item,"relamount"));
    calcprice = calc_price_volume(&calcvolume,baseamount,relamount);
    price = get_API_float(cJSON_GetObjectItem(item,"price"));
    volume = get_API_float(cJSON_GetObjectItem(item,"volume"));
    if ( fabs(calcprice - price)/price > precision && fabs(calcprice - price) > 1./SATOSHIDEN )//0.006 )//(.5/SATOSHIDEN) || fabs(calcvolume - volume) > (.5/SATOSHIDEN) )
        errs++, printf("precision %.15f: warning calcprice %.15f != %.15f price || calcvolume %.15f != %.15f volume | [%.15f %.15f]\n",precision,calcprice,price,calcvolume,volume,calcprice-price,calcvolume-volume);
    change = fabs(price-prevprice) / price;
    if ( prevprice != 0. && ((askflag == 0 && price > prevprice && change > 0.005) || (askflag == 1 && price < prevprice && change > 0.005)) )
        errs++, printf("warning: out of order iter.%d %f vs prev %f diff %.15f\n",askflag,price,prevprice,fabs(price-prevprice)/price);
    if ( get_API_nxt64bits(cJSON_GetObjectItem(item,"baseid")) != baseid || get_API_nxt64bits(cJSON_GetObjectItem(item,"relid")) != relid )
        errs++, printf("iter.%d i.%d price %f vol %f baseid %llu != ref %llu || relid.%llu != ref %llu\n",askflag,i,price,volume,(long long)get_API_nxt64bits(cJSON_GetObjectItem(item,"baseid")), (long long)baseid,(long long)get_API_nxt64bits(cJSON_GetObjectItem(item,"relid")),(long long)relid);
    if ( (askoffer= (int32_t)get_API_int(cJSON_GetObjectItem(item,"askoffer"),0)) != askflag )
        errs++, printf("askoffer.%d != iter.%d\n",askoffer,askflag);
    if ( errs != 0 )
    {
        printf("errs.%d\n",errs);
        return(0);
    }
    //printf("price %f vs prev %f\n",price,prevprice);
    return(price);
}

cJSON *detach_orderbook_item(cJSON *json,uint64_t quoteid,uint64_t baseid,uint64_t relid)
{
    cJSON *item,*array,*matchitem = 0;
    int32_t iter,i,n,count = 0;
    double prevprice;
    for (iter=0; iter<2; iter++)
    {
        if ( (array= cJSON_GetObjectItem(json,iter==0?"bids":"asks")) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
        {
            for (prevprice=i=0; i<n; i++)
            {
                fprintf(stderr,"%.8f ",prevprice);
                item = cJSON_GetArrayItem(array,i);
                if ( (prevprice= verify_orderbook_json(prevprice,i,iter,item,baseid,relid)) == 0. )
                    printf("error iter.%d i.%d (%s)\n",iter,i,cJSON_Print(item));//, getchar();
                if ( get_API_nxt64bits(cJSON_GetObjectItem(item,"quoteid")) == quoteid )
                {
                    if ( matchitem != 0 )
                        free_json(matchitem);
                    matchitem = cJSON_DetachItemFromArray(array,i);
                    i--, n--;
                    count++;
                }
            }
            fprintf(stderr,"iter.%d n.%d\n",iter,n);
        }
    }
    if ( count == 1 )
        return(matchitem);
    else if ( count > 1 )
        printf("too many for quoteid.%llu | matches.%d\n",(long long)quoteid,count);
    return(0);
}

double get_median(uint64_t assetid)
{
    uint64_t sellvol,buyvol,lowask,highbid,mult;
    if ( assetid == NXT_ASSETID )
        return(1.);
    mult = get_assetmult(assetid);
    lowask = get_nxtlowask(&sellvol,assetid) * (SATOSHIDEN / mult);
    highbid = get_nxthighbid(&buyvol,assetid) * (SATOSHIDEN / mult);
    //printf("lowask %llu highbid %llu mult.%llu\n",(long long)lowask,(long long)highbid,(long long)get_assetmult(assetid));
    if ( lowask != 0 && highbid != 0 )
        return(.5*((double)lowask + highbid) / SATOSHIDEN);
    else if ( lowask == 0 )
        return(((double)highbid * 1.1) / SATOSHIDEN);
    else return(((double)lowask * .999) / SATOSHIDEN);
}

cJSON *orderbook_json(uint64_t nxt64bits,char *base,uint64_t baseid,char *rel,uint64_t relid,int32_t maxdepth,int32_t allflag,char *gui)
{
    cJSON *json = 0;
    char *retstr;
    uint32_t oldest = 0;
    struct orderbook *op,*obooks[1024];
    if ( (op = make_orderbook(obooks,sizeof(obooks)/sizeof(*obooks),base,baseid,rel,relid,maxdepth,oldest,gui,0)) != 0 )
    {
        if ( (retstr = orderbook_jsonstr(nxt64bits,op,base,rel,maxdepth,allflag)) != 0 )
        {
            json = cJSON_Parse(retstr);
            free(retstr);
        }
        free_orderbooks(obooks,sizeof(obooks)/sizeof(*obooks),op);
    }
    return(json);
}

int32_t orderbook_testpair(cJSON **items,uint64_t quoteid,uint64_t nxt64bits,char *base,uint64_t baseid,char *rel,uint64_t relid,char *gui,int32_t maxdepth)
{
    cJSON *json;
    items[0] = items[1] = 0;
    if ( (json= orderbook_json(nxt64bits,base,baseid,rel,relid,maxdepth,1,gui)) != 0 )
    {
        if ( (items[0]= detach_orderbook_item(json,quoteid,baseid,relid)) == 0 )
            printf("couldnt find quoteid.%llu\n",(long long)quoteid);
       // printf("bids.(%s)\n",cJSON_Print(json));
        free_json(json);
    } else printf("cant get orderbook.(%s/%s)\n",base,rel);
    if ( (json= orderbook_json(nxt64bits,rel,relid,base,baseid,maxdepth,1,gui)) != 0 )
    {
        if ( (items[1]= detach_orderbook_item(json,quoteid,relid,baseid)) == 0 )
            printf("couldnt find quoteid.%llu\n",(long long)quoteid);
        //printf("asks.(%s)\n",cJSON_Print(json));
        free_json(json);
    } else printf("cant get orderbook.(%s/%s)\n",rel,base);
    if ( items[0] != 0 && items[1] != 0 )
        return(0);
    if ( items[0] != 0 )
        free_json(items[0]);
    if ( items[1] != 0 )
        free_json(items[1]);
    return(-1);
}

int32_t orderbook_verifymatch(int32_t dir,uint64_t baseid,uint64_t relid,double price,double volume,cJSON *item,cJSON *invitem,cJSON *buyer,cJSON *seller)
{
    char buyerstr[64],sellerstr[64],*strs[4];
    uint64_t assetid,buyerpriceNQT,sellerpriceNQT,assetidB;
    double buyprice=0,sellprice=0;
    int64_t buyerqty,sellerqty;
    int32_t i,retval = -1;
    if ( item == 0 || invitem == 0 || buyer == 0 || seller == 0 )
    {
        printf("orderbook_verifymatch: null item %p %p %p %p\n",item,invitem,buyer,seller);
        return(-1);
    }
    strs[0] = cJSON_Print(item), strs[1] = cJSON_Print(invitem), strs[2] = cJSON_Print(buyer), strs[3] = cJSON_Print(seller);
    for (i=2; i<4; i++)
        _stripwhite(strs[i],' '), printf("(%s).%d ",strs[i],i);
    printf("\n");
    assetid = get_API_nxt64bits(cJSON_GetObjectItem(buyer,"assetid"));
    assetidB = get_API_nxt64bits(cJSON_GetObjectItem(seller,"assetid"));
    copy_cJSON(buyerstr,cJSON_GetObjectItem(buyer,"action")), buyerqty = get_API_nxt64bits(cJSON_GetObjectItem(buyer,"qty")), buyerpriceNQT = get_API_nxt64bits(cJSON_GetObjectItem(buyer,"priceNQT"));
    copy_cJSON(sellerstr,cJSON_GetObjectItem(seller,"action")), sellerqty = get_API_float(cJSON_GetObjectItem(seller,"qty")), sellerpriceNQT = get_API_nxt64bits(cJSON_GetObjectItem(seller,"priceNQT"));
    if ( assetid == assetidB )
    {
        if ( assetid == baseid )
        {
            buyprice = ((double)buyerpriceNQT / get_assetmult(assetid));
            sellprice = ((double)sellerpriceNQT / get_assetmult(assetid));
        }
        else
        {
            buyprice = (get_assetmult(assetid) / (double)buyerpriceNQT);
            sellprice = (get_assetmult(assetid) / (double)sellerpriceNQT);
        }
        if ( buyerqty != 0 && buyerqty == -sellerqty && ((strcmp(buyerstr,"transmit") == 0 && strcmp(sellerstr,"nxtae") == 0) || (strcmp(sellerstr,"transmit") == 0 && strcmp(buyerstr,"nxtae") == 0)) && buyerpriceNQT == sellerpriceNQT )
        {
            if ( sellerqty < 0 && ((dir > 0 && strcmp(buyerstr,"transmit") == 0) || (dir < 0 && strcmp(sellerstr,"transmit") == 0)) )
            {
                if ( buyerpriceNQT != 0 && sellerpriceNQT != 0 )
                {
                    if ( fabs(buyprice - price) < .95/get_assetmult(assetid) || fabs(buyprice - price)/price < calc_precision(assetid,NXT_ASSETID) )
                        retval = 0;
                    else printf("price mismatch %.15f %.15f vs %.15f [%.15f] margin %f asset.%llu,mult.%llu [%.15f] precision %.8f\n",buyprice,sellprice,price,buyprice-price,.5/get_assetmult(assetid),(long long)assetid,(long long)get_assetmult(assetid),fabs(buyprice - price)/price,calc_precision(assetid,NXT_ASSETID));
                }
                else
                {
                    printf("orderbook_verifymatch: unexpected case\n");
                }
            } else printf("internal cmp error %d (%d %d) | dir.%d %s %s\n",sellerqty < 0,(dir > 0 && strcmp(buyerstr,"transmit") == 0),(dir < 0 && strcmp(sellerstr,"transmit") == 0),dir,buyerstr,sellerstr);
        } else printf("cmp failure %d %d (%d %d) %d | %s %s\n",buyerqty != 0,buyerqty == -sellerqty,(strcmp(buyerstr,"transmit") == 0 && strcmp(sellerstr,"nxtae") == 0),(strcmp(sellerstr,"transmit") == 0 && strcmp(buyerstr,"nxtae") == 0),buyerpriceNQT == sellerpriceNQT,buyerstr,sellerstr);
    }
    else
    {
        if ( assetidB == baseid && assetid == relid && ((dir < 0 && strcmp(buyerstr,"transmit") == 0 && strcmp(sellerstr,"nxtae") == 0) || (dir > 0 && strcmp(buyerstr,"nxtae") == 0 && strcmp(sellerstr,"transmit") == 0) || (dir == 0 && strcmp(buyerstr,"InstantDEX") == 0 && strcmp(sellerstr,"transmit") == 0)) )
        {
            buyprice = sellprice = ((double)buyerqty * get_assetmult(assetid)) / (sellerqty * get_assetmult(assetidB));
            if ( fabs(buyprice - price)/price < calc_precision(assetid,assetidB) || fabs(buyprice - price)/price < 0.005 )
                retval = 0;
            printf(">>>>>>>>>> swap at price %f %f/%f (%llu * %llu) / (%llu * %llu) | precision %.15f diff %.15f\n",buyprice,((double)buyerqty*get_assetmult(assetid)),((double)sellerqty*get_assetmult(assetidB)),(long long)buyerqty,(long long)get_assetmult(assetid),(long long)sellerqty,(long long)get_assetmult(assetidB),calc_precision(assetid,assetidB),fabs(buyprice - price)/price);
        }
    }
    printf(">>>>>>>>>> %llu/%llu %f %f dir.%d  buyprice %f sellprice %f qty %lld %lld pNQT %llu %llu\n",(long long)baseid,(long long)relid,price,volume,dir,buyprice,sellprice,(long long)buyerqty,(long long)sellerqty,(long long)buyerpriceNQT,(long long)sellerpriceNQT);
    for (i=0; i<4; i++)
        free(strs[i]);
    return(retval);
}

char *call_makeoffer3(int32_t localaccess,char *NXTaddr,char *NXTACCTSECRET,cJSON *objs[])
{
    uint64_t quoteid,baseid,relid,baseamount,relamount,offerNXT,jumpasset; struct InstantDEX_quote baseiQ,reliQ;
    char exchange[MAX_JSON_FIELD];
    double price,volume;
    int32_t minperc,perc,flip = 0;
    baseid = get_API_nxt64bits(objs[0]);
    relid = get_API_nxt64bits(objs[1]);
    quoteid = get_API_nxt64bits(objs[2]);
    perc = get_API_int(objs[3],0);
    flip = (int32_t)get_API_int(objs[4],0);
    price = get_API_float(objs[8]);
    volume = get_API_float(objs[9]);
    copy_cJSON(exchange,objs[10]);
    baseamount = get_API_nxt64bits(objs[11]);
    relamount = get_API_nxt64bits(objs[12]);
    offerNXT = get_API_nxt64bits(objs[13]);
    minperc = (int32_t)get_API_int(objs[14],INSTANTDEX_MINVOL);
    jumpasset = get_API_nxt64bits(objs[15]);
    memset(&baseiQ,0,sizeof(baseiQ));
    memset(&reliQ,0,sizeof(reliQ));
    set_basereliQ(&baseiQ,objs[5]), set_basereliQ(&reliQ,objs[6]);
    return(makeoffer3(NXTaddr,NXTACCTSECRET,price,volume,flip,perc,baseid,relid,&baseiQ,&reliQ,quoteid,get_API_int(objs[7],0),exchange,baseamount,relamount,offerNXT,minperc,jumpasset));
}

char *makeoffer3_stub(int32_t localaccess,char *NXTaddr,char *NXTACCTSECRET,char *jsonstr)
{
    static char *makeoffer3_fields[] = { (char *)0, "makeoffer3", "V", "baseid", "relid", "quoteid", "perc", "flip", "baseiQ", "reliQ", "askoffer", "price", "volume", "exchange", "baseamount", "relamount", "offerNXT", "minperc", "jumpasset", 0 };
    cJSON *json,*objs[16];
    char *retstr = 0;
    int32_t j;
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        for (j=3; j<3+(int32_t)(sizeof(objs)/sizeof(*objs)); j++)
            objs[j-3] = cJSON_GetObjectItem(json,makeoffer3_fields[j]);
        retstr = call_makeoffer3(localaccess,NXTaddr,NXTACCTSECRET,objs);
        free_json(json);
    }
    return(retstr);
}

void orderbook_test(uint64_t nxt64bits,uint64_t refbaseid,uint64_t refrelid,int32_t maxdepth)
{
    char base[64],rel[64],NXTaddr[64],*jsonstr,*retstr,*gui = "test";
    struct InstantDEX_quote iQ;
    struct rambook_info *rb;
    uint64_t baseid,relid,quoteid,mult;
    double baseprice,relprice,testprice,price,minbasevol,minrelvol,volume = 1.;
    cJSON *items[2],*json;
    int32_t dir,iter,j;
    expand_nxt64bits(NXTaddr,nxt64bits);
    baseprice = get_median(refbaseid), relprice = get_median(refrelid);
    if ( baseprice == 0. || relprice == 0. )
    {
        printf("cant get median %llu %f || %llu %f\n",(long long)refbaseid,baseprice,(long long)refrelid,relprice);
        return;
    }
    price = (baseprice / relprice);
    //update_rambooks(refbaseid,refrelid,maxdepth,gui,1,0);
    prices777_poll(refbaseid,refrelid);
    baseid = refbaseid, relid = refrelid;
    volume = 1.;
    printf("price %f = (%f / %f) vol %f\n",price,baseprice,relprice,volume);
    for (iter=0; iter<2; iter++)
    {
        set_assetname(&mult,base,baseid);
        set_assetname(&mult,rel,relid);
        prices777_poll(baseid,relid);
        //update_rambooks(baseid,relid,maxdepth,gui,1,0);
        minbasevol = get_minvolume(baseid);
        minrelvol = get_minvolume(relid);
        printf("base.(%s %.8f) rel.(%s %.8f)\n",base,minbasevol,rel,minrelvol);
        for (dir=1; dir>=-1; dir-=2)
        for (j=1; j<=1; j++)
        {
            testprice = (price * (1. - .01*dir*j));
            volume = .5;
            if ( volume < minbasevol || (volume * testprice) < minrelvol )
                volume = MAX(minbasevol * 1.2,1.2 * minrelvol/testprice);
            printf("iter.%d dir.%d testprice %f basevol %f relvol %f\n",iter,dir,testprice,volume,volume*testprice);
            if ( (rb= add_rambook_quote(INSTANTDEX_NAME,&iQ,nxt64bits,(uint32_t)time(NULL),dir,baseid,relid,testprice,volume,0,0,gui,0,13)) == 0 )
            {
                printf("skip low resolution price point\n");
                continue;
            }
            quoteid = (long long)calc_quoteid(&iQ);
            if ( orderbook_testpair(items,quoteid,nxt64bits,base,baseid,rel,relid,gui,maxdepth) == 0 )
            {
                jsonstr = submitquote_str(1,&iQ,baseid,relid);
                if ( (retstr= makeoffer3_stub(1,"4077619696739571952",0,jsonstr)) != 0 )
                {
                    if ( (json= cJSON_Parse(retstr)) != 0 )
                    {
                        if ( orderbook_verifymatch(dir,baseid,relid,testprice,volume,items[0],items[1],cJSON_GetObjectItem(json,"buyer"),cJSON_GetObjectItem(json,"seller")) < 0 )
                        {
                            printf("orderbook_verifymatch failed dir.%d %s/%s testprice %f %llu %f || %llu %f\n(%s)",dir,base,rel,testprice,(long long)refbaseid,baseprice,(long long)refrelid,relprice,retstr);//, getchar();
                            getchar();
                        } else printf("VERIFIED <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< iter.%d dir.%d %s/%s price %f vol %f\n",iter,dir,base,rel,price,volume);
                        free_json(json);
                        if ( 0 && rb->numquotes > 1 )
                        {
                            char *jsonstr2,*retstr2; cJSON *json2;
                            if ( (jsonstr2= submitquote_str(1,rb->quotes[1],baseid,relid)) != 0 )
                            {
                                if ( (retstr2= makeoffer3_stub(1,"4077619696739571952",0,jsonstr2)) != 0 )
                                {
                                    if ( (json2= cJSON_Parse(retstr2)) != 0 )
                                    {
                                        _stripwhite(retstr2,' ');
                                        printf("NXTAE.(%s)\n",retstr2);
                                        free_json(json2);
                                    } free(retstr2);
                                } free(jsonstr2);
                            }
                        }
                    } else printf("error parsing (%s)\n",retstr);//, getchar();
                    free(retstr);
                } else printf("error makeoffer3_stub\n");//, getchar();
                printf("Q.%llu %s %llu / %s %llu price %f volume %f (%s)\n",(long long)quoteid,base,(long long)baseid,rel,(long long)relid,testprice,volume,jsonstr!=0?jsonstr:"nosubmitstr");
                free_json(items[0]), free_json(items[1]);
                if ( jsonstr != 0 )
                    free(jsonstr);
            } else printf("error getting testpair dir.%d testprice %f %llu %f || %llu %f\n",dir,testprice,(long long)refbaseid,baseprice,(long long)refrelid,relprice);//, getchar();
        }
        relid = refbaseid, baseid = refrelid;
        price = (relprice / baseprice), volume = (1. / price) / .9;
    }
    printf("--------------------------\n\n");
}

/*struct libwebsocket *init_exchangewss(char *addr)
{
    struct libwebsocket *libwebsocket_client_connect(struct libwebsocket_context *clients,const char *address,int port,int ssl_connection,const char *path,const char *host,const char *origin,const char *protocol,int ietf_version_or_minus_one);
    int32_t port = 80,use_ssl = 0;
    printf("init_exchangewss(%s)\n",addr);
    if ( LWScontext != 0 )
        return(libwebsocket_client_connect(LWScontext,addr,port,use_ssl,"/",addr,addr,"echo",-1));
    else return(0);
}
*/

void init_exchanges()
{
    //cJSON *exchanges;
    //int32_t i,n;
    find_exchange(0,INSTANTDEX_NAME);
    find_exchange(0,INSTANTDEX_NXTAEUNCONF);
    find_exchange(0,INSTANTDEX_NXTAENAME);
    //POLLGAP = get_API_int(cJSON_GetObjectItem(MGWconf,"POLLGAP"),10);
    //DEFAULT_MAXDEPTH = get_API_int(cJSON_GetObjectItem(MGWconf,"DEFAULT_MAXDEPTH"),DEFAULT_MAXDEPTH);
    /*if ( (exchanges= cJSON_GetObjectItem(SUPERNET.argjson,"exchanges")) != 0 && (n= cJSON_GetArraySize(exchanges)) > 0 )
    {
        for (i=0; i<n; i++)
            init_exchange(cJSON_GetArrayItem(exchanges,i));
    }*/
}

char *trollbox_func(int32_t localaccess,int32_t valid,char *sender,cJSON **objs,int32_t numobjs,char *origargstr)
{
    if ( (rand() & 1) == 0 )
        return(clonestr("{\"result\":\"buy it will go to the moon\",\"user\":\"troll\",\"whaleindex\":\"0\"}"));
    else return(clonestr("{\"result\":\"sell it will be dumped\",\"user\":\"troll\",\"whaleindex\":\"0\"}"));
}

void init_InstantDEX(uint64_t nxt64bits,int32_t testflag)
{
    int32_t a,b;
    init_pingpong_queue(&Pending_offersQ,"pending_offers",process_Pending_offersQ,0,0);
    Pending_offersQ.offset = 0;
    init_exchanges();
    find_exchange(&a,INSTANTDEX_NXTAENAME), find_exchange(&b,INSTANTDEX_NAME);
    if ( a != INSTANTDEX_NXTAEID || b != INSTANTDEX_EXCHANGEID )
        printf("invalid exchangeid %d, %d\n",a,b);
    printf("NXT-> %llu BTC -> %llu\n",(long long)stringbits("NXT"),(long long)stringbits("BTC"));
#ifdef __APPLE__
    if ( 0 && testflag != 0 )
    {
        static char *testids[] = { "6932037131189568014", "6854596569382794790", "17554243582654188572", "15344649963748848799", "8688289798928624137", "12071612744977229797" };
        long i,j;
        uint64_t assetA,assetB;
        //orderbook_test(nxt64bits,calc_nxt64bits(testids[0]),calc_nxt64bits(testids[1]),13);
        //orderbook_test(nxt64bits,calc_nxt64bits(testids[1]),calc_nxt64bits(testids[0]),13);
        for (i=0; i<(sizeof(testids)/sizeof(*testids))-1; i++)
        {
            assetA = calc_nxt64bits(testids[i]);
            for (j=i; j<(sizeof(testids)/sizeof(*testids)); j++)
            {
                assetB = calc_nxt64bits(testids[j]);
                printf("assetA.%llu assetB.%llu\n",(long long)assetA,(long long)assetB);
                if ( i != j )
                    orderbook_test(nxt64bits,assetA,assetB,13), orderbook_test(nxt64bits,assetB,assetA,13);
                else orderbook_test(nxt64bits,assetA,NXT_ASSETID,13), orderbook_test(nxt64bits,NXT_ASSETID,assetA,13);
                //getchar();
            }
        }
        printf("FINISHED tests\n");
        getchar();
    }
#endif
}
#endif
