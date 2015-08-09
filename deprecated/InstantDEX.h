//
//  InstantDEX.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_InstantDEX_h
#define xcode_InstantDEX_h

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


struct rambook_info
{
    UT_hash_handle hh;
    FILE *fp;
    char url[128],base[16],rel[16],lbase[16],lrel[16],exchange[32];
    struct InstantDEX_quote *quotes;
    uint64_t assetids[3];
    uint32_t lastaccess;
    int32_t numquotes,maxquotes;
    float lastmilli;
    uint8_t updated;
} *Rambooks;

struct exchange_info
{
    void (*ramparse)(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui);
    int32_t (*ramsupports)(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid);
    uint64_t (*trade)(char **retstrp,struct exchange_info *exchange,char *base,char *rel,int32_t dir,double price,double volume);
    uint64_t nxt64bits;
    struct libwebsocket *wsi;
    char name[16],apikey[MAX_JSON_FIELD],apisecret[MAX_JSON_FIELD];
    uint32_t num,exchangeid,lastblock,lastaccess,pollgap;
    float lastmilli;
} Exchanges[MAX_EXCHANGES];

struct exchange_info *find_exchange(char *exchangestr,void (*ramparse)(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui),int32_t (*ramparse_supports)(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid));

uint64_t find_best_market_maker(int32_t *totalticketsp,int32_t *numticketsp,char *refNXTaddr,uint32_t timestamp)
{
    char cmdstr[1024],NXTaddr[64],receiverstr[MAX_JSON_FIELD],*jsonstr;
    cJSON *json,*array,*txobj;
    int32_t i,n,createdflag,totaltickets = 0;
    struct NXT_acct *np,*maxnp = 0;
    uint64_t amount,senderbits;
    uint32_t now = (uint32_t)time(NULL);
    if ( timestamp == 0 )
        timestamp = 38785003;
    sprintf(cmdstr,"requestType=getAccountTransactions&account=%s&timestamp=%u&type=0&subtype=0",INSTANTDEX_ACCT,timestamp);
    //printf("cmd.(%s)\n",cmdstr);
    if ( (jsonstr= bitcoind_RPC(0,"curl",NXTAPIURL,0,0,cmdstr)) != 0 )
    {
        // printf("jsonstr.(%s)\n",jsonstr);
        // mm string.({"requestProcessingTime":33,"transactions":[{"fullHash":"2a2aab3b84dadf092cf4cedcd58a8b5a436968e836338e361c45651bce0ef97e","confirmations":203,"signatureHash":"52a4a43d9055fe4861b3d13fbd03a42fecb8c9ad4ac06a54da7806a8acd9c5d1","transaction":"711527527619439146","amountNQT":"1100000000","transactionIndex":2,"ecBlockHeight":360943,"block":"6797727125503999830","recipientRS":"NXT-74VC-NKPE-RYCA-5LMPT","type":0,"feeNQT":"100000000","recipient":"4383817337783094122","version":1,"sender":"423766016895692955","timestamp":38929220,"ecBlockId":"10121077683890606382","height":360949,"subtype":0,"senderPublicKey":"4e5bbad625df3d536fa90b1e6a28c3f5a56e1fcbe34132391c8d3fd7f671cb19","deadline":1440,"blockTimestamp":38929430,"senderRS":"NXT-8E6V-YBWH-5VMR-26ESD","signature":"4318f36d9cf68ef0a8f58303beb0ed836b670914065a868053da5fe8b096bc0c268e682c0274e1614fc26f81be4564ca517d922deccf169eafa249a88de58036"}]})
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"transactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    txobj = cJSON_GetArrayItem(array,i);
                    copy_cJSON(receiverstr,cJSON_GetObjectItem(txobj,"recipient"));
                    if ( strcmp(receiverstr,INSTANTDEX_ACCT) == 0 )
                    {
                        if ( (senderbits = get_API_nxt64bits(cJSON_GetObjectItem(txobj,"sender"))) != 0 )
                        {
                            expand_nxt64bits(NXTaddr,senderbits);
                            np = get_NXTacct(&createdflag,NXTaddr);
                            amount = get_API_nxt64bits(cJSON_GetObjectItem(txobj,"amountNQT"));
                            if ( np->timestamp != now )
                            {
                                np->quantity = 0;
                                np->timestamp = now;
                            }
                            if ( amount == INSTANTDEX_FEE )
                                totaltickets++;
                            else if ( amount >= 2*INSTANTDEX_FEE )
                                totaltickets += 2;
                            np->quantity += amount;
                            if ( maxnp == 0 || np->quantity > maxnp->quantity )
                                maxnp = np;
                        }
                    }
                }
            }
            free_json(json);
        }
        free(jsonstr);
    }
    if ( refNXTaddr != 0 )
    {
        np = get_NXTacct(&createdflag,refNXTaddr);
        if ( numticketsp != 0 )
            *numticketsp = (int32_t)(np->quantity / INSTANTDEX_FEE);
    }
    if ( totalticketsp != 0 )
        *totalticketsp = totaltickets;
    if ( maxnp != 0 )
    {
        printf("Best MM %llu total %.8f\n",(long long)maxnp->H.nxt64bits,dstr(maxnp->quantity));
        return(maxnp->H.nxt64bits);
    }
    return(0);
}

int32_t get_top_MMaker(struct pserver_info **pserverp)
{
    static uint64_t bestMMbits;
    struct nodestats *stats;
    char ipaddr[64];
    *pserverp = 0;
    if ( bestMMbits == 0 )
        bestMMbits = find_best_market_maker(0,0,0,38785003);
    if ( bestMMbits != 0 )
    {
        stats = get_nodestats(bestMMbits);
        expand_ipbits(ipaddr,stats->ipbits);
        (*pserverp) = get_pserver(0,ipaddr,0,0);
        return(0);
    }
    return(-1);
}

void ramparse_stub(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    printf("unexpected call to ramparse_stub gui.%s maxdepth.%d\n",gui,maxdepth);
}

void submit_quote(char *quotestr)
{
    int32_t len;
    char _tokbuf[4096];
    //struct pserver_info *pserver;
    struct coin_info *cp = get_coin_info("BTCD");
    if ( cp != 0 )
    {
        printf("submit_quote.(%s)\n",quotestr);
        len = construct_tokenized_req(_tokbuf,quotestr,cp->srvNXTACCTSECRET);
        //if ( get_top_MMaker(&pserver) == 0 )
        //    call_SuperNET_broadcast(pserver,_tokbuf,len,ORDERBOOK_EXPIRATION);
        call_SuperNET_broadcast(0,_tokbuf,len,ORDERBOOK_EXPIRATION);
    }
}

void submit_respondtx(char *respondtxstr,uint64_t nxt64bits,char *NXTACCTSECRET,uint64_t dest64bits)
{
    char _tokbuf[8192],hopNXTaddr[64],NXTaddr[64],destNXTaddr[64],*retstr;
    int32_t len;
    len = construct_tokenized_req(_tokbuf,respondtxstr,NXTACCTSECRET);
    hopNXTaddr[0] = 0;
    expand_nxt64bits(NXTaddr,nxt64bits);
    expand_nxt64bits(destNXTaddr,dest64bits);
    if ( (retstr= send_tokenized_cmd(!prevent_queueing("respondtx"),hopNXTaddr,0,NXTaddr,NXTACCTSECRET,respondtxstr,destNXTaddr)) != 0 )
        free(retstr);
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

void update_openorder(struct InstantDEX_quote *iQ,uint64_t quoteid,struct NXT_tx *txptrs[],int32_t updateNXT) // from poll_pending_offers via main
{
    //printf("update_openorder iQ.%llu with tx.%llu\n",(long long)iQ->quoteid,(long long)tx->txid);
    // regen orderbook and see if it crosses
    // updatestats
}

#include "rambooks.h"
#include "exchanges.h"
#include "orderbooks.h"
#include "trades.h"
#include "atomic.h"
#include "bars.h"
#include "signals.h"

char *check_ordermatch(char *NXTaddr,char *NXTACCTSECRET,struct InstantDEX_quote *iQ,char *submitstr) // called by placequote, should autofill
{
    return(submitstr);
}

char *lottostats_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char buf[MAX_JSON_FIELD];
    uint64_t bestMMbits;
    int32_t totaltickets,numtickets;
    uint32_t firsttimestamp;
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    firsttimestamp = (uint32_t)get_API_int(objs[0],0);
    bestMMbits = find_best_market_maker(&totaltickets,&numtickets,NXTaddr,firsttimestamp);
    sprintf(buf,"{\"result\":\"lottostats\",\"totaltickets\":\"%d\",\"NXT\":\"%s\",\"numtickets\":\"%d\",\"odds\":\"%.2f\",\"topMM\":\"%llu\"}",totaltickets,NXTaddr,numtickets,numtickets == 0 ? 0 : (double)totaltickets / numtickets,(long long)bestMMbits);
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

char *submitquote_str(struct InstantDEX_quote *iQ,uint64_t baseid,uint64_t relid)
{
    cJSON *json;
    char *jsonstr = 0;
    uint64_t basetmp,reltmp;
    if ( (json= gen_InstantDEX_json(&basetmp,&reltmp,0,iQ->isask,iQ,baseid,relid,0)) != 0 )
    {
        cJSON_ReplaceItemInObject(json,"requestType",cJSON_CreateString((iQ->isask != 0) ? "ask" : "bid"));
        jsonstr = cJSON_Print(json);
        stripwhite_ns(jsonstr,strlen(jsonstr));
        submit_quote(jsonstr);
        free_json(json);
    }
    return(jsonstr);
}

char *placequote_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,int32_t dir,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    uint64_t baseamount,relamount,nxt64bits,baseid,relid,quoteid = 0;
    double price,volume,minbasevol,minrelvol;
    uint32_t timestamp;
    uint8_t minperc;
    struct exchange_info *xchg;
    struct InstantDEX_quote iQ;
    struct exchange_info *exchange;
    int32_t remoteflag,automatch,duration;
    struct rambook_info *rb;
    char buf[MAX_JSON_FIELD],gui[MAX_JSON_FIELD],exchangestr[MAX_JSON_FIELD],base[16],rel[16],*jsonstr,*retstr = 0;
    if ( (xchg= find_exchange(INSTANTDEX_NAME,0,0)) == 0 || xchg->exchangeid != INSTANTDEX_EXCHANGEID )
        return(clonestr("{\"error\":\"unexpected InstantDEX exchangeid\"}"));
    remoteflag = (is_remote_access(previpaddr) != 0);
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
    automatch = (int32_t)get_API_int(objs[8],0);
    minperc = (int32_t)get_API_int(objs[9],0);
    duration = (int32_t)get_API_int(objs[10],ORDERBOOK_EXPIRATION);
    if ( duration <= 0 || duration > ORDERBOOK_EXPIRATION )
        duration = ORDERBOOK_EXPIRATION;
    copy_cJSON(exchangestr,objs[11]);
    if ( exchangestr[0] == 0 )
        strcpy(exchangestr,INSTANTDEX_NAME);
    else
    {
        if ( remoteflag != 0 && strcmp("InstantDEX",exchangestr) != 0 )
        {
            printf("remote node (%s) (%s) trying to place quote to exchange (%s)\n",previpaddr,sender,exchangestr);
            return(clonestr("{\"error\":\"no remote exchange orders: you cannot submit an order from a remote node\"}"));
        }
        else if ( strcmp(exchangestr,"nxtae") != 0 && strcmp(exchangestr,"unconf") != 0 && strcmp(exchangestr,"InstantDEX") != 0 )
        {
            if ( is_native_crypto(base,baseid) > 0 && is_native_crypto(rel,relid) > 0 && price > 0 && volume > 0 && dir != 0 )
            {
                if ( (exchange= find_exchange(exchangestr,0,0)) != 0 )
                {
                    if ( exchange->trade != 0 )
                    {
                        printf(" issue dir.%d %s/%s price %f vol %f -> %s\n",dir,base,rel,price,volume,exchangestr);
                        (*exchange->trade)(&retstr,exchange,base,rel,dir,price,volume);
                        return(retstr);
                    }
                    else return(clonestr("{\"error\":\"no trade function for exchange\"}\n"));
                } else return(clonestr("{\"error\":\"exchange not active, check SuperNET.conf exchanges array\"}\n"));
            } else return(clonestr("{\"error\":\"illegal parameter baseid or relid not crypto or invalid price\"}\n"));
        }
    }
    update_rambooks(baseid,relid,0,0,0);
    printf("NXT.%s t.%u placequote dir.%d sender.(%s) valid.%d price %.8f vol %.8f %llu/%llu\n",NXTaddr,timestamp,dir,sender,valid,price,volume,(long long)baseamount,(long long)relamount);
    minbasevol = get_minvolume(baseid), minrelvol = get_minvolume(relid);
    if ( volume < minbasevol || (volume * price) < minrelvol )
    {
        sprintf(buf,"{\"error\":\"not enough volume\",\"price\":%f,\"volume\":%f,\"minbasevol\":%f,\"minrelvol\":%f,\"relvol\":%f}",price,volume,minbasevol,minrelvol,price*volume);
        return(clonestr(buf));
    }

    /*if ( automatch != 0 && remoteflag == 0 && (retstr= auto_makeoffer2(NXTaddr,NXTACCTSECRET,dir,baseid,baseamount,relid,relamount,gui)) != 0 )
     {
     fprintf(stderr,"got (%s) from auto_makeoffer2\n",retstr);
     return(retstr);
     }*/
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
                        printf("placequote.(%s)\n",retstr);
                }
                if ( (jsonstr= submitquote_str(&iQ,baseid,relid)) != 0 )
                {
                    if ( remoteflag == 0 )
                    {
                        submit_quote(jsonstr);
                        //free(jsonstr);
                        retstr = jsonstr;
                    }
                    else retstr = check_ordermatch(NXTaddr,NXTACCTSECRET,&iQ,jsonstr);
                }
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
    return(retstr);
}

char *placebid_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    return(placequote_func(NXTaddr,NXTACCTSECRET,previpaddr,1,sender,valid,objs,numobjs,origargstr));
}

char *placeask_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    return(placequote_func(NXTaddr,NXTACCTSECRET,previpaddr,-1,sender,valid,objs,numobjs,origargstr));
}

char *bid_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    return(placequote_func(NXTaddr,NXTACCTSECRET,previpaddr,1,sender,valid,objs,numobjs,origargstr));
}

char *ask_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    return(placequote_func(NXTaddr,NXTACCTSECRET,previpaddr,-1,sender,valid,objs,numobjs,origargstr));
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
    if ( (op = make_orderbook(obooks,sizeof(obooks)/sizeof(*obooks),base,baseid,rel,relid,maxdepth,oldest,gui)) != 0 )
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
        stripwhite_ns(strs[i],strlen(strs[i])), printf("(%s).%d ",strs[i],i);
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

char *call_makeoffer3(char *NXTaddr,char *NXTACCTSECRET,cJSON *objs[])
{
    uint64_t quoteid,baseid,relid,baseamount,relamount,offerNXT,jumpasset;
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
    return(makeoffer3(NXTaddr,NXTACCTSECRET,price,volume,flip,perc,baseid,relid,objs[5],objs[6],quoteid,get_API_int(objs[7],0),exchange,baseamount,relamount,offerNXT,minperc,jumpasset));
}

char *makeoffer3_stub(char *NXTaddr,char *NXTACCTSECRET,char *jsonstr)
{
    static char *makeoffer3_fields[] = { (char *)0, "makeoffer3", "V", "baseid", "relid", "quoteid", "perc", "flip", "baseiQ", "reliQ", "askoffer", "price", "volume", "exchange", "baseamount", "relamount", "offerNXT", "minperc", "jumpasset", 0 };
    cJSON *json,*objs[16];
    char *retstr = 0;
    int32_t j;
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        for (j=3; j<3+(int32_t)(sizeof(objs)/sizeof(*objs)); j++)
            objs[j-3] = cJSON_GetObjectItem(json,makeoffer3_fields[j]);
        retstr = call_makeoffer3(NXTaddr,NXTACCTSECRET,objs);
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
    update_rambooks(refbaseid,refrelid,maxdepth,gui,1);
    baseid = refbaseid, relid = refrelid;
    volume = 1.;
    printf("price %f = (%f / %f) vol %f\n",price,baseprice,relprice,volume);
    for (iter=0; iter<2; iter++)
    {
        set_assetname(&mult,base,baseid);
        set_assetname(&mult,rel,relid);
        update_rambooks(baseid,relid,maxdepth,gui,1);
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
                jsonstr = submitquote_str(&iQ,baseid,relid);
                if ( (retstr= makeoffer3_stub("4077619696739571952",0,jsonstr)) != 0 )
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
                            if ( (jsonstr2= submitquote_str(&rb->quotes[1],baseid,relid)) != 0 )
                            {
                                if ( (retstr2= makeoffer3_stub("4077619696739571952",0,jsonstr2)) != 0 )
                                {
                                    if ( (json2= cJSON_Parse(retstr2)) != 0 )
                                    {
                                        stripwhite_ns(retstr2,strlen(retstr2));
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

struct libwebsocket *init_exchangewss(char *addr)
{
    struct libwebsocket *libwebsocket_client_connect(struct libwebsocket_context *clients,const char *address,int port,int ssl_connection,const char *path,const char *host,const char *origin,const char *protocol,int ietf_version_or_minus_one);
    int32_t port = 80,use_ssl = 0;
    printf("init_exchangewss(%s)\n",addr);
    if ( LWScontext != 0 )
        return(libwebsocket_client_connect(LWScontext,addr,port,use_ssl,"/",addr,addr,"echo",-1));
    else return(0);
}

void init_exchange(cJSON *json)
{
    static void *exchangeptrs[][5] =
    {
        { (void *)"poloniex", (void *)ramparse_poloniex, (void *)poloniex_supports, (void *)poloniex_trade, },
        { (void *)"bittrex", (void *)ramparse_bittrex, (void *)bittrex_supports, (void *)bittrex_trade },
        { (void *)"bter", (void *)ramparse_bter, (void *)bter_supports, (void *)bter_trade },
        { (void *)"btce", (void *)ramparse_btce, (void *)btce_supports, (void *)btce_trade },
        { (void *)"bitfinex", (void *)ramparse_bitfinex, (void *)bitfinex_supports, 0 },
        { (void *)"bitstamp", (void *)ramparse_bitstamp, (void *)bitstamp_supports, 0 },
        { (void *)"okcoin", (void *)ramparse_okcoin, (void *)okcoin_supports, 0 },
        { (void *)"huobi", (void *)ramparse_huobi, (void *)huobi_supports, 0 },
        { (void *)"bityes", (void *)ramparse_bityes, (void *)bityes_supports, 0 },
        { (void *)"lakebtc", (void *)ramparse_lakebtc, (void *)lakebtc_supports, 0 },
        { (void *)"exmo", (void *)ramparse_exmo, (void *)exmo_supports, 0 },
        { (void *)"btc38", (void *)ramparse_btc38, (void *)btc38_supports, 0 },
    };
    struct exchange_info *exchange;
    char name[MAX_JSON_FIELD];
    void *parse=0,*supports=0,*trade=0;
    int32_t i;
    extract_cJSON_str(name,sizeof(name),json,"name");
    if ( name[0] != 0 && strlen(name) < 16 )
    {
        for (i=0; i<(int32_t)(sizeof(exchangeptrs)/sizeof(*exchangeptrs)); i++)
        {
            if ( strcmp(exchangeptrs[i][0],name) == 0 )
            {
                *(void **)&parse = exchangeptrs[i][1];
                *(void **)&supports = exchangeptrs[i][2];
                *(void **)&trade = exchangeptrs[i][3];
                break;
            }
        }
        if ( parse != 0 && (exchange= find_exchange(name,(void (*)(struct rambook_info *, struct rambook_info *, int32_t, char *))parse,(int32_t (*)(int32_t, uint64_t *, int32_t, uint64_t, uint64_t))supports)) != 0 )
        {
            exchange->pollgap = get_API_int(cJSON_GetObjectItem(json,"pollgap"),POLLGAP);
            extract_cJSON_str(exchange->apikey,sizeof(exchange->apikey),json,"key");
            extract_cJSON_str(exchange->apisecret,sizeof(exchange->apisecret),json,"secret");
            *(void **)&exchange->trade = trade;
            /*if ( exchangeptrs[i][4] != 0 )
            {
                int libwebsocket_rx_flow_control(struct libwebsocket *wsi,int enable);
                int err;
                if ( (exchange->wsi= init_exchangewss(exchangeptrs[i][4])) != 0 )
                {
                    err = libwebsocket_rx_flow_control(exchange->wsi,1);
                }
                printf("got wsi.%p | err.%d\n",exchange->wsi,err); getchar();
            }*/
        }
    }
}

void init_exchanges()
{
    cJSON *exchanges;
    int32_t i,n;
    find_exchange(INSTANTDEX_NAME,ramparse_stub,0);
    find_exchange(INSTANTDEX_NXTAEUNCONF,ramparse_stub,0);
    find_exchange(INSTANTDEX_NXTAENAME,ramparse_NXT,NXT_supports);
    POLLGAP = get_API_int(cJSON_GetObjectItem(MGWconf,"POLLGAP"),10);
    DEFAULT_MAXDEPTH = get_API_int(cJSON_GetObjectItem(MGWconf,"DEFAULT_MAXDEPTH"),DEFAULT_MAXDEPTH);
    if ( (exchanges= cJSON_GetObjectItem(MGWconf,"exchanges")) != 0 && (n= cJSON_GetArraySize(exchanges)) > 0 )
    {
        for (i=0; i<n; i++)
            init_exchange(cJSON_GetArrayItem(exchanges,i));
    }
}

char *trollbox_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    if ( (rand() & 1) == 0 )
        return(clonestr("{\"result\":\"buy it will go to the moon\",\"user\":\"troll\",\"whaleindex\":\"0\"}"));
    else return(clonestr("{\"result\":\"sell it will be dumped\",\"user\":\"troll\",\"whaleindex\":\"0\"}"));
}

void init_InstantDEX(uint64_t nxt64bits,int32_t testflag)
{
    //printf("NXT-> %llu BTC -> %llu\n",(long long)stringbits("NXT"),(long long)stringbits("BTC")); getchar();
    init_pingpong_queue(&Pending_offersQ,"pending_offers",process_Pending_offersQ,0,0);
    Pending_offersQ.offset = 0;
    init_exchanges();
    if ( find_exchange(INSTANTDEX_NXTAENAME,0,0)->exchangeid != INSTANTDEX_NXTAEID || find_exchange(INSTANTDEX_NAME,0,0)->exchangeid != INSTANTDEX_EXCHANGEID )
        printf("invalid exchangeid %d, %d\n",find_exchange(INSTANTDEX_NXTAENAME,0,0)->exchangeid,find_exchange(INSTANTDEX_NAME,0,0)->exchangeid);
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
