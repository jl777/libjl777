//
//  exchangeparse.h
//
//  Created by jl777 on 13/4/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifndef xcode_exchangeparse_h
#define xcode_exchangeparse_h

/*struct MemoryStruct { char *memory; size_t size; };

static size_t WriteMemoryCallback(void *ptr,size_t size,size_t nmemb,void *data)
{
    size_t realsize = (size * nmemb);
    struct MemoryStruct *mem = (struct MemoryStruct *)data;
    mem->memory = (ptr != 0) ? realloc(mem->memory,mem->size + realsize + 1) : malloc(mem->size + realsize + 1);
    if ( mem->memory != 0 )
    {
        memcpy(&(mem->memory[mem->size]),ptr,realsize);
        mem->size += realsize;
        mem->memory[mem->size] = 0;
    }
    return(realsize);
}

void *curl_post(CURL **cHandlep,char *url,char *postfields,char *hdr0,char *hdr1,char *hdr2)
{
    struct MemoryStruct chunk; CURL *cHandle; long code; struct curl_slist *headers = 0;
    if ( (cHandle= *cHandlep) == NULL )
		*cHandlep = cHandle = curl_easy_init();
    else curl_easy_reset(cHandle);
#ifdef DEBUG
	//curl_easy_setopt(cHandle,CURLOPT_VERBOSE, 1);
#endif
	curl_easy_setopt(cHandle,CURLOPT_USERAGENT,"Mozilla/4.0 (compatible; )");
	curl_easy_setopt(cHandle,CURLOPT_SSL_VERIFYPEER,0);
	curl_easy_setopt(cHandle,CURLOPT_URL,url);
	if ( postfields != NULL )
		curl_easy_setopt(cHandle,CURLOPT_POSTFIELDS,postfields);
    if ( hdr0 != NULL )
    {
        headers = curl_slist_append(headers,hdr0);
        if ( hdr1 != 0 )
            headers = curl_slist_append(headers,hdr1);
        if ( hdr2 != 0 )
            headers = curl_slist_append(headers,hdr2);
        if ( headers != 0 )
            curl_easy_setopt(cHandle,CURLOPT_HTTPHEADER,headers);
    }
    //res = curl_easy_perform(cHandle);
    memset(&chunk,0,sizeof(chunk));
    curl_easy_setopt(cHandle,CURLOPT_WRITEFUNCTION,WriteMemoryCallback);
    curl_easy_setopt(cHandle,CURLOPT_WRITEDATA,(void *)&chunk);
    curl_easy_perform(cHandle);
    curl_easy_getinfo(cHandle,CURLINFO_RESPONSE_CODE,&code);
    if ( code != 200 )
        printf("error: (%s) server responded with code %ld\n",url,code);
    if ( headers != 0 )
        curl_slist_free_all(headers);
    return(chunk.memory);
}*/

int32_t emit_orderbook_changes(struct rambook_info *rb,struct InstantDEX_quote *oldquotes,int32_t numold)
{
    void emit_iQ(struct rambook_info *rb,struct InstantDEX_quote *iQ);
    double vol;
    int32_t i,j,numchanges = 0;
    struct InstantDEX_quote *iQ;
    if ( rb->numquotes == 0 || numold == 0 )
        return(0);
    if ( 0 && numold != 0 )
    {
        for (j=0; j<numold; j++)
            printf("%llu ",(long long)oldquotes[j].baseamount);
        printf("%s %s_%s OLD.%d\n",rb->exchange,rb->base,rb->rel,numold);
    }
    for (i=0; i<rb->numquotes; i++)
    {
        iQ = &rb->quotes[i];
        if ( Debuglevel > 2 )
            fprintf(stderr,"(%llu/%llu %.8f) ",(long long)iQ->baseamount,(long long)iQ->relamount,calc_price_volume(&vol,iQ->baseamount,iQ->relamount));
        if ( numold > 0 )
        {
            for (j=0; j<numold; j++)
            {
                //printf("%s %s_%s %d of %d: %llu/%llu vs %llu/%llu\n",rb->exchange,rb->base,rb->rel,j,numold,(long long)iQ->baseamount,(long long)iQ->relamount,(long long)oldquotes[j].baseamount,(long long)oldquotes[j].relamount);
                if ( iQcmp(iQ,&oldquotes[j]) == 0 )
                    break;
            }
        } else j = 0;
        if ( j == numold )
            emit_iQ(rb,iQ), numchanges++;
    }
    if ( Debuglevel > 2 )
        fprintf(stderr,"%s %s_%s NEW.%d\n\n",rb->exchange,rb->base,rb->rel,rb->numquotes);
    if ( oldquotes != 0 )
        free(oldquotes);
    return(numchanges);
}

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

void convram_NXT_quotejson(struct NXT_tx *txptrs[],int32_t numptrs,uint64_t assetid,int32_t flip,cJSON *json,char *fieldname,uint64_t ap_mult,int32_t maxdepth,char *gui)
{
    //"priceNQT": "12900",
    //"asset": "4551058913252105307",
    //"order": "8128728940342496249",
    //"quantityQNT": "20000000",
    struct NXT_tx *tx;
    uint32_t timestamp;
    int32_t i,n,dir;
    uint64_t baseamount,relamount;
    struct InstantDEX_quote iQ;
    cJSON *srcobj,*srcitem;
    if ( ap_mult == 0 )
        return;
    if ( flip == 0 )
        dir = 1;
    else dir = -1;
    for (i=0; i<numptrs; i++)
    {
        tx = txptrs[i];
        if ( tx->assetidbits == assetid && tx->type == 2 && tx->subtype == (3 - flip) && tx->refhash.txid == 0 )
        {
            printf("i.%d: assetid.%llu type.%d subtype.%d time.%u\n",i,(long long)tx->assetidbits,tx->type,tx->subtype,tx->timestamp);
            baseamount = (tx->U.quantityQNT * ap_mult);
            relamount = (tx->U.quantityQNT * tx->priceNQT);
            add_rambook_quote(INSTANTDEX_NXTAENAME,&iQ,tx->senderbits,tx->timestamp,dir,assetid,NXT_ASSETID,0.,0.,baseamount,relamount,gui,0,0);
        }
    }
    srcobj = cJSON_GetObjectItem(json,fieldname);
    if ( srcobj != 0 )
    {
        if ( (n= cJSON_GetArraySize(srcobj)) > 0 )
        {
            for (i=0; i<n&&i<maxdepth; i++)
            {
                srcitem = cJSON_GetArrayItem(srcobj,i);
                baseamount = (get_satoshi_obj(srcitem,"quantityQNT") * ap_mult);
                relamount = (get_satoshi_obj(srcitem,"quantityQNT") * get_satoshi_obj(srcitem,"priceNQT"));
                timestamp = get_blockutime((uint32_t)get_API_int(cJSON_GetObjectItem(srcitem,"height"),0));
                add_rambook_quote(INSTANTDEX_NXTAENAME,&iQ,get_API_nxt64bits(cJSON_GetObjectItem(srcitem,"account")),timestamp,dir,assetid,NXT_ASSETID,0.,0.,baseamount,relamount,gui,get_API_nxt64bits(cJSON_GetObjectItem(srcitem,"order")),0);
                if ( Debuglevel > 1 && i < 3 )
                {
                    double price,volume,tmp;
                    price = calc_price_volume(&volume,baseamount,relamount);
                    printf("%-8s %s %llu/%-5s %.8f vol %.8f | invert %.8f vol %.8f | timestmp.%u quoteid.%llu | %llu/%llu = %f\n","nxtae",dir>0?"bid":"ask",(long long)assetid,"NXT",price,volume,1./price,volume*price,timestamp,(long long)get_API_nxt64bits(cJSON_GetObjectItem(srcitem,"order")),(long long)baseamount,(long long)relamount,calc_price_volume(&tmp,baseamount,relamount));
                }
            }
        }
    }
}

void ramparse_NXT(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    struct NXT_tx *txptrs[MAX_TXPTRS];
    cJSON *json,*bidobj,*askobj;
    char *buystr,*sellstr;
    struct NXT_asset *ap;
    int32_t createdflag,flip,numptrs;
    char assetidstr[64],bidurl[1024],askurl[1024];
    uint64_t basemult,relmult,assetid;
    if ( NXT_ASSETID != stringbits("NXT") )
        printf("NXT_ASSETID.%llu != %llu stringbits\n",(long long)NXT_ASSETID,(long long)stringbits("NXT"));
    if ( (bids->assetids[1] != NXT_ASSETID && bids->assetids[0] != NXT_ASSETID) || time(NULL) < bids->lastaccess+10 || time(NULL) < asks->lastaccess+10 )
    {
        return;
    }
    printf("ramparse_NXT %llu/%llu\n",(long long)bids->assetids[0],(long long)bids->assetids[1]);
    memset(txptrs,0,sizeof(txptrs));
    basemult = relmult = 1;
    if ( bids->assetids[0] != NXT_ASSETID )
    {
        expand_nxt64bits(assetidstr,bids->assetids[0]);
        ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
        basemult = ap->mult;
        assetid = bids->assetids[0];
        flip = 0;
    }
    else
    {
        expand_nxt64bits(assetidstr,bids->assetids[1]);
        ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
        relmult = ap->mult;
        assetid = bids->assetids[1];
        flip = 1;
    }
    sprintf(bidurl,"%s=getBidOrders&asset=%llu&limit=%d",NXTSERVER,(long long)bids->assetids[0],maxdepth);
    sprintf(askurl,"%s=getAskOrders&asset=%llu&limit=%d",NXTSERVER,(long long)asks->assetids[0],maxdepth);
    buystr = _issue_curl(0,"ramparse",bidurl);
    sellstr = _issue_curl(0,"ramparse",askurl);
    //printf("(%s) (%s)\n",buystr,sellstr);
    if ( buystr != 0 && sellstr != 0 )
    {
        bidobj = askobj = 0;
        numptrs = update_iQ_flags(txptrs,sizeof(txptrs)/sizeof(*txptrs),assetid);
        if ( (json = cJSON_Parse(buystr)) != 0 )
            convram_NXT_quotejson(txptrs,numptrs,assetid,0,json,"bidOrders",ap->mult,maxdepth,gui), free_json(json);
        if ( (json = cJSON_Parse(sellstr)) != 0 )
            convram_NXT_quotejson(txptrs,numptrs,assetid,1,json,"askOrders",ap->mult,maxdepth,gui), free_json(json);
        free_txptrs(txptrs,numptrs);
    }
    if ( buystr != 0 )
        free(buystr);
    if ( sellstr != 0 )
        free(sellstr);
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
        if ( Debuglevel > 1 && i < 3 )
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
    jsonstr = _issue_curl(0,"trex",bids->url);
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
    jsonstr = _issue_curl(0,"bter",bids->url);
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
    if ( (jsonstr= _issue_curl(0,exchangestr,url)) != 0 )
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
}

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

