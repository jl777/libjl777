//
//  InstantDEX.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_InstantDEX_h
#define xcode_InstantDEX_h

#define _issue_curl(curl_handle,label,url) bitcoind_RPC(curl_handle,label,url,0,0,0)

#define ORDERBOOK_EXPIRATION 300
#define INSTANTDEX_MINVOL 75
#define INSTANTDEX_MINVOLPERC ((double)INSTANTDEX_MINVOL / 100.)
#define INSTANTDEX_PRICESLIPPAGE 0.001

#define INSTANTDEX_TRIGGERDEADLINE 15
#define JUMPTRADE_SECONDS 10
#define INSTANTDEX_ACCT "4383817337783094122"
#define INSTANTDEX_FEE ((long)(2.5 * SATOSHIDEN))

#define INSTANTDEX_NAME "InstantDEX"
#define INSTANTDEX_NXTAENAME "nxtae"
#define INSTANTDEX_NXTAEUNCONF "unconf"
#define INSTANTDEX_EXCHANGEID 0
#define INSTANTDEX_UNCONFID 1
#define INSTANTDEX_NXTAEID 2
#define MAX_EXCHANGES 64

struct orderpair { struct rambook_info *bids,*asks; void (*ramparse)(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui); float lastmilli; };
struct exchange_info { uint64_t nxt64bits; char name[16]; struct orderpair orderpairs[4096]; uint32_t num,exchangeid,lastblock; float lastmilli; } Exchanges[MAX_EXCHANGES];
struct exchange_info *find_exchange(char *exchangestr,int32_t createflag);
int32_t is_exchange_nxt64bits(uint64_t nxt64bits);
int32_t init_rambooks(char *base,char *rel,uint64_t baseid,uint64_t relid);

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
                            np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
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
        np = get_NXTacct(&createdflag,Global_mp,refNXTaddr);
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

void submit_quote(char *quotestr)
{
    int32_t len;
    char _tokbuf[4096];
    struct pserver_info *pserver;
    struct coin_info *cp = get_coin_info("BTCD");
    if ( cp != 0 )
    {
        printf("submit_quote.(%s)\n",quotestr);
        len = construct_tokenized_req(_tokbuf,quotestr,cp->srvNXTACCTSECRET);
        if ( get_top_MMaker(&pserver) == 0 )
            call_SuperNET_broadcast(pserver,_tokbuf,len,ORDERBOOK_EXPIRATION);
        call_SuperNET_broadcast(0,_tokbuf,len,ORDERBOOK_EXPIRATION);
    }
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
#include "rambooks.h"
#include "exchanges.h"
#include "orderbooks.h"
#include "trades.h"
#include "atomic.h"
#include "bars.h"
#include "signals.h"

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

char *placequote_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,int32_t dir,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    cJSON *json;
    uint64_t type,basetmp,reltmp,baseamount,relamount,nxt64bits,baseid,relid,quoteid = 0;
    double price,volume;
    uint32_t timestamp;
    uint8_t minperc;
    struct exchange_info *xchg;
    struct InstantDEX_quote iQ;
    int32_t remoteflag,automatch;
    struct rambook_info *rb;
    char buf[MAX_JSON_FIELD],txidstr[64],gui[MAX_JSON_FIELD],*jsonstr,*retstr = 0;
    if ( (xchg= find_exchange(INSTANTDEX_NAME,1)) == 0 || xchg->exchangeid != INSTANTDEX_EXCHANGEID )
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
    timestamp = (uint32_t)get_API_int(objs[4],0);
    copy_cJSON(gui,objs[7]), gui[sizeof(iQ.gui)-1] = 0;
    automatch = (int32_t)get_API_int(objs[8],0);
    minperc = (int32_t)get_API_int(objs[9],INSTANTDEX_MINVOL);
    ensure_rambook(baseid,relid);
    printf("NXT.%s t.%u placequote type.%llu dir.%d sender.(%s) valid.%d price %.8f vol %.8f %llu/%llu\n",NXTaddr,timestamp,(long long)type,dir,sender,valid,price,volume,(long long)baseamount,(long long)relamount);
    /*if ( automatch != 0 && remoteflag == 0 && (retstr= auto_makeoffer2(NXTaddr,NXTACCTSECRET,dir,baseid,baseamount,relid,relamount,gui)) != 0 )
     {
     fprintf(stderr,"got (%s) from auto_makeoffer2\n",retstr);
     return(retstr);
     }*/
    if ( sender[0] != 0 && valid > 0 )
    {
        if ( price != 0. && volume != 0. && dir != 0 )
        {
            rb = add_rambook_quote(INSTANTDEX_NAME,&iQ,nxt64bits,timestamp,dir,baseid,relid,price,volume,baseamount,relamount,gui,0);
            iQ.minperc = INSTANTDEX_MINVOL;
            if ( remoteflag == 0 && (json= gen_InstantDEX_json(&basetmp,&reltmp,0,iQ.isask,&iQ,rb->assetids[0],rb->assetids[1],0)) != 0 )
            {
                cJSON_ReplaceItemInObject(json,"requestType",cJSON_CreateString((iQ.isask != 0) ? "ask" : "bid"));
                jsonstr = cJSON_Print(json);
                stripwhite_ns(jsonstr,strlen(jsonstr));
                submit_quote(jsonstr);
                free_json(json);
                free(jsonstr);
            }
            if ( (quoteid= calc_quoteid(&iQ)) != 0 )
            {
                expand_nxt64bits(txidstr,quoteid);
                sprintf(buf,"{\"result\":\"success\",\"quoteid\":\"%s\"}",txidstr);
                retstr = clonestr(buf);
                printf("placequote.(%s)\n",buf);
            }
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

#endif
