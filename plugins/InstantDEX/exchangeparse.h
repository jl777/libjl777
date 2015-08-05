//
//  exchangeparse.h
//
//  Created by jl777 on 13/4/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifndef xcode_exchangeparse_h
#define xcode_exchangeparse_h
#include <curl/curl.h>
#define DEFAULT_MAXDEPTH 25
void *curl_post(CURL **cHandlep,char *url,char *postfields,char *hdr0,char *hdr1,char *hdr2);
/*
cJSON *inner_json(double price,double vol,uint32_t timestamp,uint64_t quoteid,uint64_t nxt64bits)
{
    cJSON *inner = cJSON_CreateArray();
    char numstr[64];
    cJSON_AddItemToArray(inner,cJSON_CreateNumber(price));
    cJSON_AddItemToArray(inner,cJSON_CreateNumber(vol));
    cJSON_AddItemToArray(inner,cJSON_CreateNumber(timestamp));
    sprintf(numstr,"%llu",(long long)quoteid), cJSON_AddItemToArray(inner,cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)nxt64bits), cJSON_AddItemToArray(inner,cJSON_CreateString(numstr));
    return(inner);
}

void convram_NXT_quotejson(uint64_t assetid,int32_t flip,cJSON *json,char *fieldname,uint64_t ap_mult,int32_t maxdepth,char *gui)
{
    //"priceNQT": "12900",
    //"asset": "4551058913252105307",
    //"order": "8128728940342496249",
    //"quantityQNT": "20000000",
    uint32_t timestamp; int32_t i,n,dir; uint64_t baseamount,relamount; struct InstantDEX_quote iQ; cJSON *srcobj,*srcitem;
    if ( ap_mult == 0 )
        return;
    if ( flip == 0 )
        dir = 1;
    else dir = -1;
    srcobj = cJSON_GetObjectItem(json,fieldname);
    if ( srcobj != 0 )
    {
        if ( (n= cJSON_GetArraySize(srcobj)) > 0 )
        {
            for (i=0; i<n && i<maxdepth; i++)
            {
                srcitem = cJSON_GetArrayItem(srcobj,i);
                baseamount = (get_satoshi_obj(srcitem,"quantityQNT") * ap_mult);
                relamount = (get_satoshi_obj(srcitem,"quantityQNT") * get_satoshi_obj(srcitem,"priceNQT"));
                timestamp = get_blockutime((uint32_t)get_API_int(cJSON_GetObjectItem(srcitem,"height"),0));
                add_rambook_quote(INSTANTDEX_NXTAENAME,&iQ,get_API_nxt64bits(cJSON_GetObjectItem(srcitem,"account")),timestamp,dir,assetid,NXT_ASSETID,0.,0.,baseamount,relamount,gui,get_API_nxt64bits(cJSON_GetObjectItem(srcitem,"order")),0);
                if ( Debuglevel > 2 && i < 3 )
                {
                    double price,volume,tmp;
                    price = calc_price_volume(&volume,baseamount,relamount);
                    printf("%-8s %s %llu/%-5s %.8f vol %.8f | invert %.8f vol %.8f | timestmp.%u quoteid.%llu | %llu/%llu = %f\n","nxtae",dir>0?"bid":"ask",(long long)assetid,"NXT",price,volume,1./price,volume*price,timestamp,(long long)get_API_nxt64bits(cJSON_GetObjectItem(srcitem,"order")),(long long)baseamount,(long long)relamount,calc_price_volume(&tmp,baseamount,relamount));
                }
            }
        }
    }
}

void ramupdate_NXThalf(int32_t flip,uint64_t assetid,int32_t maxdepth,char *gui)
{
    char url[1024],*str,*cmd,*field; cJSON *json;
    if ( gui == 0 )
        gui = "";
    if ( flip == 0 )
        cmd = "getBidOrders", field = "bidOrders";
    else cmd = "getAskOrders", field = "askOrders";
    sprintf(url,"requestType=%s&asset=%llu&limit=%d",cmd,(long long)assetid,maxdepth);
    if ( (str= issue_NXTPOST(url)) != 0 )
    {
        //printf("flip.%d update.(%s)\n",flip,url);
        if ( (json = cJSON_Parse(str)) != 0 )
            convram_NXT_quotejson(assetid,flip,json,field,get_assetmult(assetid),maxdepth,gui), free_json(json);
    } else printf("cant get.(%s)\n",url);
    if ( str != 0 )
        free(str);
}

void ramparse_NXT(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    uint64_t assetid;
    if ( NXT_ASSETID != stringbits("NXT") )
        printf("NXT_ASSETID.%llu != %llu stringbits\n",(long long)NXT_ASSETID,(long long)stringbits("NXT"));
    if ( bids->assetids[1] != NXT_ASSETID && bids->assetids[0] != NXT_ASSETID )
        return;
    //printf("ramparse_NXT %llu/%llu\n",(long long)bids->assetids[0],(long long)bids->assetids[1]);
    if ( bids->assetids[0] != NXT_ASSETID )
        assetid = bids->assetids[0];
    else assetid = bids->assetids[1];
    convram_NXT_Uquotejson(assetid);
    if ( maxdepth == 0 || bids->numupdates == 0 || asks->numupdates == 0 )
    {
        maxdepth = 25;
        ramupdate_NXThalf(0,bids->assetids[0],maxdepth,gui);
        ramupdate_NXThalf(1,asks->assetids[0],maxdepth,gui);
    }
}

int32_t parseram_json_quotes(char *exchangestr,int32_t dir,struct rambook_info *rb,cJSON *array,int32_t maxdepth,char *pricefield,char *volfield,char *gui)
{
    cJSON *item;
    int32_t i,n,numitems;
    uint32_t reftimestamp,timestamp;
    uint64_t nxt64bits,baseamount,relamount,quoteid = 0;
    double price,volume,tmp;
    struct InstantDEX_quote iQ;
    nxt64bits = stringbits(exchangestr);
    reftimestamp = (uint32_t)time(NULL);
    n = cJSON_GetArraySize(array);
    if ( maxdepth != 0 && n > maxdepth )
        n = maxdepth;
    for (i=0; i<n; i++)
    {
        quoteid = timestamp = 0;
        item = cJSON_GetArrayItem(array,i);
        if ( pricefield != 0 && volfield != 0 )
        {
            price = get_API_float(cJSON_GetObjectItem(item,pricefield));
            volume = get_API_float(cJSON_GetObjectItem(item,volfield));
        }
        else if ( is_cJSON_Array(item) != 0 && (numitems= cJSON_GetArraySize(item)) != 0 ) // big assumptions about order within nested array!
        {
            price = get_API_float(cJSON_GetArrayItem(item,0));
            volume = get_API_float(cJSON_GetArrayItem(item,1));
        }
        else
        {
            printf("unexpected case in parseram_json_quotes\n");
            continue;
        }
        if ( timestamp == 0 )
            timestamp = reftimestamp;
        set_best_amounts(&baseamount,&relamount,price,volume);
        if ( Debuglevel > 2 && i < 3 )
            printf("%-8s %s %5s/%-5s %13.8f vol %13.8f | invert %13.8f vol %13.8f | timestmp.%u quoteid.%llu | %llu/%llu = %f\n",rb->exchange,dir>0?"bid":"ask",rb->base,rb->rel,price,volume,1./price,volume*price,timestamp,(long long)quoteid,(long long)baseamount,(long long)relamount,calc_price_volume(&tmp,baseamount,relamount));
        add_rambook_quote(exchangestr,&iQ,nxt64bits,timestamp,dir,rb->assetids[0],rb->assetids[1],0.,0.,baseamount,relamount,gui,0,0);
        add_user_order(rb,&iQ);
    }
    return(n);
}

void ramparse_json_orderbook(char *exchangestr,struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,cJSON *json,char *resultfield,char *bidfield,char *askfield,char *pricefield,char *volfield,char *gui)
{
    cJSON *obj = 0,*bidobj,*askobj;
    if ( resultfield == 0 )
        obj = json;
    if ( maxdepth == 0 )
        maxdepth = DEFAULT_MAXDEPTH;
    if ( resultfield == 0 || (obj= cJSON_GetObjectItem(json,resultfield)) != 0 )
    {
        if ( (bidobj= cJSON_GetObjectItem(obj,bidfield)) != 0 && is_cJSON_Array(bidobj) != 0 )
            parseram_json_quotes(exchangestr,1,bids,bidobj,maxdepth,pricefield,volfield,gui);
        if ( (askobj= cJSON_GetObjectItem(obj,askfield)) != 0 && is_cJSON_Array(askobj) != 0 )
            parseram_json_quotes(exchangestr,-1,asks,askobj,maxdepth,pricefield,volfield,gui);
    }
}

void ramparse_bittrex(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui) // "BTC-BTCD"
{
    cJSON *json,*obj;
    char *jsonstr,market[128];
    if ( bids->url[0] == 0 )
    {
        sprintf(market,"%s-%s",bids->rel,bids->base);
        sprintf(bids->url,"https://bittrex.com/api/v1.1/public/getorderbook?market=%s&type=both&depth=%d",market,maxdepth);
    }
    jsonstr = issue_curl(bids->url);
    if ( jsonstr != 0 )
    {
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (obj= cJSON_GetObjectItem(json,"success")) != 0 && is_cJSON_True(obj) != 0 )
                ramparse_json_orderbook("bittrex",bids,asks,maxdepth,json,"result","buy","sell","Rate","Quantity",gui);
            free_json(json);
        }
        free(jsonstr);
    }
}

void ramparse_bter(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    cJSON *json,*obj;
    char resultstr[MAX_JSON_FIELD],*jsonstr;
    if ( bids->url[0] == 0 )
        sprintf(bids->url,"http://data.bter.com/api/1/depth/%s_%s",bids->base,bids->rel);
    jsonstr = issue_curl(bids->url);
    //printf("(%s) -> (%s)\n",ep->url,jsonstr);
    //{"result":"true","asks":[["0.00008035",100],["0.00008030",2030],["0.00008024",100],["0.00008018",643.41783554],["0.00008012",100]
    if ( jsonstr != 0 )
    {
        //printf("BTER.(%s)\n",jsonstr);
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (obj= cJSON_GetObjectItem(json,"result")) != 0 )
            {
                copy_cJSON(resultstr,obj);
                if ( strcmp(resultstr,"true") == 0 )
                {
                    maxdepth = 1000; // since bter ask is wrong order, need to scan entire list
                    ramparse_json_orderbook("bter",bids,asks,maxdepth,json,0,"bids","asks",0,0,gui);
                }
            }
            free_json(json);
        }
        free(jsonstr);
    }
}

void ramparse_standard(char *exchangestr,char *url,struct rambook_info *bids,struct rambook_info *asks,char *price,char *volume,int32_t maxdepth,char *gui)
{
    //static CURL *cHandle;
    char *jsonstr; cJSON *json;
    //if ( (jsonstr= curl_getorpost(&cHandle,url,0,0)) != 0 )
    if ( (jsonstr= issue_curl(url)) != 0 )
    {
        //if ( strcmp(exchangestr,"btc38") == 0 )
        //    printf("(%s)\n",jsonstr);
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            ramparse_json_orderbook(exchangestr,bids,asks,maxdepth,json,0,"bids","asks",price,volume,gui);
            free_json(json);
        }
        free(jsonstr);
    }
}

void ramparse_poloniex(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    char market[128];
    if ( bids->url[0] == 0 )
    {
        sprintf(market,"%s_%s",bids->rel,bids->base);
        sprintf(bids->url,"https://poloniex.com/public?command=returnOrderBook&currencyPair=%s&depth=%d",market,maxdepth);
    }
    ramparse_standard("poloniex",bids->url,bids,asks,0,0,maxdepth,gui);
}

void ramparse_bitfinex(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    if ( bids->url[0] == 0 )
        sprintf(bids->url,"https://api.bitfinex.com/v1/book/%s%s",bids->base,bids->rel);
    ramparse_standard("bitfinex",bids->url,bids,asks,"price","amount",maxdepth,gui);
}

void ramparse_btce(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    if ( bids->url[0] == 0 )
        sprintf(bids->url,"https://btc-e.com/api/2/%s%s/depth",bids->lbase,bids->lrel);
    ramparse_standard("btce",bids->url,bids,asks,0,0,maxdepth,gui);
}

void ramparse_bitstamp(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    if ( bids->url[0] == 0 )
        sprintf(bids->url,"https://www.bitstamp.net/api/order_book/");
    ramparse_standard("bitstamp",bids->url,bids,asks,0,0,maxdepth,gui);
}

void ramparse_okcoin(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    if ( bids->url[0] == 0 )
        sprintf(bids->url,"https://www.okcoin.com/api/depth.do?symbol=%s%s",bids->lbase,bids->lrel);
    ramparse_standard("okcoin",bids->url,bids,asks,0,0,maxdepth,gui);
}

void ramparse_huobi(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    if ( bids->url[0] == 0 )
        sprintf(bids->url,"http://api.huobi.com/staticmarket/depth_%s_json.js ",bids->lbase);
    ramparse_standard("huobi",bids->url,bids,asks,0,0,maxdepth,gui);
}

void ramparse_bityes(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    if ( bids->url[0] == 0 )
        sprintf(bids->url,"https://market.bityes.com/%s_%s/trade_history.js?time=%ld",bids->lrel,bids->lbase,time(NULL));
    ramparse_standard("bityes",bids->url,bids,asks,0,0,maxdepth,gui);
}

void ramparse_coinbase(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    if ( bids->url[0] == 0 )
        sprintf(bids->url,"https://api.exchange.coinbase.com/products/%s-%s/book?level=2",bids->base,bids->rel);
    ramparse_standard("coinbase",bids->url,bids,asks,0,0,maxdepth,gui);
}

void ramparse_lakebtc(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    if ( bids->url[0] == 0 )
    {
        if ( strcmp(bids->rel,"USD") == 0 )
            sprintf(bids->url,"https://www.LakeBTC.com/api_v1/bcorderbook");
        else if ( strcmp(bids->rel,"CNY") == 0 )
            sprintf(bids->url,"https://www.LakeBTC.com/api_v1/bcorderbook_cny");
        else printf("illegal lakebtc pair.(%s/%s)\n",bids->base,bids->rel);
    }
    ramparse_standard("lakebtc",bids->url,bids,asks,0,0,maxdepth,gui);
}

void ramparse_exmo(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    if ( bids->url[0] == 0 )
        sprintf(bids->url,"https://api.exmo.com/api_v2/orders_book?pair=%s_%s",bids->base,bids->rel);
    ramparse_standard("exmo",bids->url,bids,asks,0,0,maxdepth,gui);
}

void ramparse_btc38(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    if ( bids->url[0] == 0 )
        sprintf(bids->url,"http://api.btc38.com/v1/depth.php?c=%s&mk_type=%s",bids->lbase,bids->lrel);
    printf("btc38.(%s)\n",bids->url);
    ramparse_standard("btc38",bids->url,bids,asks,0,0,maxdepth,gui);
}*/

/*void ramparse_kraken(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    if ( bids->url[0] == 0 )
        sprintf(bids->url,"https://api.kraken.com/0/public/Depth"); // need POST
    ramparse_standard("kraken",bids->url,bids,asks,0,0,maxdepth,gui);
}*/

/*void ramparse_itbit(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    if ( bids->url[0] == 0 )
        sprintf(bids->url,"https://www.itbit.com/%s%s",bids->base,bids->rel);
    ramparse_standard("itbit",bids->url,bids,asks,0,0,maxdepth,gui);
}*/


#endif

