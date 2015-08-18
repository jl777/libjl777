//
//  exchanges_trades.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_exchanges_h
#define xcode_exchanges_h

#define SHA512_DIGEST_SIZE (512 / 8)
void *curl_post(CURL **cHandlep,char *url,char *postfields,char *hdr0,char *hdr1,char *hdr2);
char *hmac_sha512_str(char dest[SHA512_DIGEST_SIZE*2 + 1],char *key,unsigned int key_size,char *message);

int32_t flip_for_exchange(char *pairstr,char *fmt,char *refstr,int32_t dir,double *pricep,double *volumep,char *base,char *rel)
{
    if ( strcmp(rel,refstr) == 0 )
        sprintf(pairstr,fmt,rel,base);
    else if ( strcmp(base,refstr) == 0 )
    {
        sprintf(pairstr,fmt,base,rel);
        dir = -dir;
        *volumep *= *pricep;
        *pricep = (1. / *pricep);
    }
    return(dir);
}

uint64_t bittrex_trade(char **retstrp,struct exchange_info *exchange,char *base,char *rel,int32_t dir,double price,double volume)
{
    static CURL *cHandle;
 	char *sig,*data,urlbuf[2048],hdr[1024],pairstr[512],dest[SHA512_DIGEST_SIZE*2 + 1],uuidstr[512];
    uint8_t databuf[512];
    uint64_t txid = 0;
    cJSON *json,*resultobj;
    int32_t i,j,n;
    dir = flip_for_exchange(pairstr,"%s-%s","BTC",dir,&price,&volume,base,rel);
    sprintf(urlbuf,"https://bittrex.com/api/v1.1/market/%slimit?apikey=%s&nonce=%ld&currencyPair=%s&rate=%.8f&amount=%.8f",dir>0?"buy":"sell",exchange->apikey,time(NULL),pairstr,price,volume);
    if ( (sig = hmac_sha512_str(dest,exchange->apisecret,(int32_t)strlen(exchange->apisecret),urlbuf)) != 0 )
        sprintf(hdr,"apisign:%s",sig);
    else hdr[0] = 0;
    printf("cmdbuf.(%s) h1.(%s)\n",urlbuf,hdr);
    if ( (data= curl_post(&cHandle,urlbuf,0,hdr,0,0)) != 0 )
    {
        printf("cmd.(%s) [%s]\n",urlbuf,data);
        if ( (json= cJSON_Parse(data)) != 0 )
        {
            // { "success" : true, "message" : "", "result" : { "uuid" : "e606d53c-8d70-11e3-94b5-425861b86ab6"  } }
            if ( is_cJSON_True(cJSON_GetObjectItem(json,"success")) != 0 && (resultobj= cJSON_GetObjectItem(json,"result")) != 0 )
            {
                copy_cJSON(uuidstr,cJSON_GetObjectItem(resultobj,"uuid"));
                for (i=j=0; uuidstr[i]!=0; i++)
                    if ( uuidstr[i] != '-' )
                        uuidstr[j++] = uuidstr[i];
                uuidstr[j] = 0;
                n = (int32_t)strlen(uuidstr);
                printf("-> uuidstr.(%s).%d\n",uuidstr,n);
                decode_hex(databuf,n/2,uuidstr);
                if ( n >= 16 )
                    for (i=0; i<8; i++)
                        databuf[i] ^= databuf[8 + i];
                memcpy(&txid,databuf,8);
                printf("-> %llx\n",(long long)txid);
            }
            free_json(json);
        }
    }
    if ( retstrp != 0 )
        *retstrp = data;
    else if ( data != 0 )
        free(data);
    return(txid);
}

uint64_t poloniex_trade(char **retstrp,struct exchange_info *exchange,char *base,char *rel,int32_t dir,double price,double volume)
{
    static CURL *cHandle;
 	char *sig,*data,cmdbuf[8192],hdr1[1024],hdr2[1024],pairstr[512],dest[SHA512_DIGEST_SIZE*2 + 1]; cJSON *json; uint64_t txid = 0;
    dir = flip_for_exchange(pairstr,"%s_%s","BTC",dir,&price,&volume,base,rel);
    sprintf(cmdbuf,"command=%s&nonce=%ld&currencyPair=%s&rate=%.8f&amount=%.8f",dir>0?"buy":"sell",time(NULL),pairstr,price,volume);
    if ( (sig = hmac_sha512_str(dest,exchange->apisecret,(int32_t)strlen(exchange->apisecret),cmdbuf)) != 0 )
        sprintf(hdr2,"Sign:%s",sig);
    else hdr2[0] = 0;
    sprintf(hdr1,"Key:%s",exchange->apikey);
    //printf("cmdbuf.(%s) h1.(%s) h2.(%s)\n",cmdbuf,hdr2,hdr1);
    if ( (data= curl_post(&cHandle,"https://poloniex.com/tradingApi",cmdbuf,hdr2,hdr1,0)) != 0 )
    {
        printf("cmd.(%s) [%s]\n",cmdbuf,data);
        if ( (json= cJSON_Parse(data)) != 0 )
        {
            txid = (get_API_nxt64bits(cJSON_GetObjectItem(json,"orderNumber")) << 32) | get_API_nxt64bits(cJSON_GetObjectItem(json,"tradeID"));
            free_json(json);
        }
    }
    if ( retstrp != 0 )
        *retstrp = data;
    else if ( data != 0 )
        free(data);
    return(txid);
}

int32_t cny_flip(char *market,char *coinname,char *base,char *rel,int32_t dir,double *pricep,double *volumep)
{
    char pairstr[512],lbase[16],lrel[16],*refstr;
    strcpy(lbase,base), tolowercase(lbase), strcpy(lrel,rel), tolowercase(lrel);
    if ( strcmp(lbase,"cny") == 0 || strcmp(lrel,"cny") == 0 )
    {
        dir = flip_for_exchange(pairstr,"%s_%s","cny",dir,pricep,volumep,lbase,lrel);
        refstr = "cny";
    }
    else if ( strcmp(lbase,"btc") == 0 || strcmp(lrel,"btc") == 0 )
    {
        dir = flip_for_exchange(pairstr,"%s_%s","btc",dir,pricep,volumep,lbase,lrel);
        refstr = "btc";
    }
    if ( market != 0 && coinname != 0 && refstr != 0 )
    {
        strcpy(market,refstr);
        if ( strcmp(lbase,"refstr") != 0 )
            strcpy(coinname,lbase);
        else strcpy(coinname,lrel);
        touppercase(coinname);
    }
    return(dir);
}

uint64_t bter_trade(char **retstrp,struct exchange_info *exchange,char *base,char *rel,int32_t dir,double price,double volume)
{
    static CURL *cHandle;
 	char *sig,*data,buf[512],cmdbuf[8192],hdr1[1024],hdr2[1024],pairstr[512],dest[SHA512_DIGEST_SIZE*2 + 1];
    cJSON *json; uint64_t txid = 0;
    dir = cny_flip(0,0,base,rel,dir,&price,&volume);
    sprintf(cmdbuf,"type=%s&nonce=%ld&pair=%s&rate=%.8f&amount=%.8f",dir>0?"BUY":"SELL",time(NULL),pairstr,price,volume);
    if ( (sig = hmac_sha512_str(dest,exchange->apisecret,(int32_t)strlen(exchange->apisecret),cmdbuf)) != 0 )
        sprintf(hdr2,"SIGN:%s",sig);
    else hdr2[0] = 0;
    sprintf(hdr1,"KEY:%s",exchange->apikey);
    printf("cmdbuf.(%s) h1.(%s) h2.(%s)\n",cmdbuf,hdr2,hdr1);
    if ( (data= curl_post(&cHandle,"https://bter.com/api/1/private/placeorder",cmdbuf,hdr2,hdr1,0)) != 0 )
    {
        printf("cmd.(%s) [%s]\n",cmdbuf,data);
        //{ "result":"true", "order_id":"123456", "msg":"Success" }
        if ( (json= cJSON_Parse(data)) != 0 )
        {
            copy_cJSON(buf,cJSON_GetObjectItem(json,"result"));
            if ( strcmp(buf,"true") != 0 )
            {
                copy_cJSON(buf,cJSON_GetObjectItem(json,"msg"));
                if ( strcmp(buf,"Success") != 0 )
                    txid = get_API_nxt64bits(cJSON_GetObjectItem(json,"order_id"));
            }
            free_json(json);
        }
    }
    if ( retstrp != 0 )
        *retstrp = data;
    else if ( data != 0 )
        free(data);
    return(txid);
}

uint64_t btce_trade(char **retstrp,struct exchange_info *exchange,char *base,char *rel,int32_t dir,double price,double volume)
{
    static CURL *cHandle;
 	char *sig,*data,cmdbuf[8192],hdr1[1024],hdr2[1024],pairstr[512],dest[SHA512_DIGEST_SIZE*2 + 1]; cJSON *json,*resultobj; uint64_t txid = 0;
    dir = flip_for_exchange(pairstr,"%s_%s","BTC",dir,&price,&volume,base,rel);
    sprintf(hdr1,"Key:%s",exchange->apikey);
    if ( (sig= hmac_sha512_str(dest,exchange->apisecret,(int32_t)strlen(exchange->apisecret),hdr1)) != 0 )
        sprintf(hdr2,"Sign:%s",sig);
    else hdr2[0] = 0;
    sprintf(cmdbuf,"method=Trade&nonce=%ld&pair=%s&type=%s&rate=%.6f&amount=%.6f",time(NULL),pairstr,dir>0?"buy":"sell",price,volume);
    printf("cmdbuf.(%s) h1.(%s) h2.(%s)\n",cmdbuf,hdr2,hdr1);
    if ( (data= curl_post(&cHandle,"https://btc-e.com/tapi",cmdbuf,hdr2,hdr1,0)) != 0 )
    {
        printf("cmd.(%s) [%s]\n",cmdbuf,data);
        //{ "success":1, "return":{ "received":0.1, "remains":0, "order_id":0, "funds":{ "usd":325, "btc":2.498,  } } }
        if ( (json= cJSON_Parse(data)) != 0 )
        {
            if ( get_API_int(cJSON_GetObjectItem(json,"success"),-1) > 0 && (resultobj= cJSON_GetObjectItem(json,"return")) != 0 )
            {
                if ( (txid= get_API_nxt64bits(cJSON_GetObjectItem(resultobj,"order_id"))) == 0 )
                {
                    if ( get_API_nxt64bits(cJSON_GetObjectItem(resultobj,"remains")) == 0 )
                        txid = _crc32(0,cmdbuf,strlen(cmdbuf));
                }
            }
            free_json(json);
        }
    }
    if ( retstrp != 0 )
        *retstrp = data;
    else if ( data != 0 )
        free(data);
    return(txid);
}

uint64_t btc38_trade(char **retstrp,struct exchange_info *exchange,char *base,char *rel,int32_t dir,double price,double volume)
{
   /* $ Stamp = $ date-> getTimestamp ();
    type, 1 for the purchase of Entry, 2 entry order to sell, can not be empty / the type of the order
   
    $ Mdt = "_ public here to write here write here to write user ID_ private _" $ stamp.;
    $ Mdt = md5 ($ mdt);
    
    $ Data = array ("key" => "here to write public", "time" => $ stamp, "md5" => $ mdt, "type" => 1, "mk_type" => "cny",
                    "Price" => "0.0001", "amount" => "100", "coinname" => "XRP");
    // $ Data_string = json_encode ($ data);
    $ Ch = curl_init ();
    curl_setopt ($ ch, CURLOPT_URL, 'http://www.btc38.com/trade/t_api/submitOrder.php');
    curl_setopt ($ ch, CURLOPT_POST, 1);
    curl_setopt ($ ch, CURLOPT_POSTFIELDS, $ data);
    curl_setopt ($ ch, CURLOPT_RETURNTRANSFER, 1);
    curl_setopt ($ ch, CURLOPT_HEADER, 0);  */
    static CURL *cHandle;
 	char *data,cmdbuf[8192],buf[512],digest[33],market[16],coinname[16],fmtstr[512],*pricefmt,*volfmt = "%.3f";
    cJSON *json,*resultobj; uint32_t stamp; uint64_t txid = 0;
    stamp = (uint32_t)time(NULL);
    if ( (dir= cny_flip(market,coinname,base,rel,dir,&price,&volume)) == 0 )
    {
        fprintf(stderr,"btc38_trade illegal base.(%s) or rel.(%s)\n",base,rel);
        return(0);
    }
    if ( strcmp(market,"cny") == 0 )
        pricefmt = "%.5f";
    else pricefmt = "%.6f";
    sprintf(buf,"%s_%s_%s_%u",exchange->apikey,exchange->userid,exchange->apisecret,stamp);
    printf("MD5.(%s)\n",buf);
    calc_md5(digest,buf,(int32_t)strlen(buf));
    sprintf(fmtstr,"key=%%s&time=%%u&md5=%%s&type=%%s&mk_type=%%s&coinname=%%s&price=%s&amount=%s",pricefmt,volfmt);
    sprintf(cmdbuf,fmtstr,exchange->apikey,stamp,digest,dir>0?"1":"2",market,coinname,price,volume);
    if ( (data= curl_post(&cHandle,"http://www.btc38.com/trade/t_api/submitOrder.php",cmdbuf,0,0,0)) != 0 )
    {
        printf("submit cmd.(%s) [%s]\n",cmdbuf,data);
        if ( (json= cJSON_Parse(data)) != 0 )
        {
            if ( get_API_int(cJSON_GetObjectItem(json,"success"),-1) > 0 && (resultobj= cJSON_GetObjectItem(json,"return")) != 0 )
            {
                if ( (txid= get_API_nxt64bits(cJSON_GetObjectItem(resultobj,"order_id"))) == 0 )
                {
                    if ( get_API_nxt64bits(cJSON_GetObjectItem(resultobj,"remains")) == 0 )
                        txid = _crc32(0,cmdbuf,strlen(cmdbuf));
                }
            }
            free_json(json);
        }
    } else fprintf(stderr,"submit err cmd.(%s)\n",cmdbuf);
    if ( retstrp != 0 )
        *retstrp = data;
    else if ( data != 0 )
        free(data);
    return(txid);
}

uint64_t submit_triggered_nxtae(char **retjsonstrp,int32_t is_MS,char *bidask,uint64_t nxt64bits,char *NXTACCTSECRET,uint64_t assetid,uint64_t qty,uint64_t NXTprice,char *triggerhash,char *comment,uint64_t otherNXT,uint32_t triggerheight)
{
    int32_t deadline = 1 + 20;
    uint64_t txid = 0;
    char cmd[4096],secret[8192],errstr[MAX_JSON_FIELD],*jsonstr;
    cJSON *json;
    if ( retjsonstrp != 0 )
        *retjsonstrp = 0;
    if ( triggerheight != 0 )
        deadline = DEFAULT_NXT_DEADLINE;
    escape_code(secret,NXTACCTSECRET);
    sprintf(cmd,"requestType=%s&secretPhrase=%s&feeNQT=%llu&deadline=%d",bidask,secret,(long long)MIN_NQTFEE,deadline);
    sprintf(cmd+strlen(cmd),"&%s=%llu&%s=%llu",is_MS!=0?"units":"quantityQNT",(long long)qty,is_MS!=0?"currency":"asset",(long long)assetid);
    if ( NXTprice != 0 )
    {
        if ( is_MS != 0 )
            sprintf(cmd+strlen(cmd),"&rateNQT=%llu",(long long)NXTprice);
        else sprintf(cmd+strlen(cmd),"&priceNQT=%llu",(long long)NXTprice);
    }
    if ( otherNXT != 0 )
        sprintf(cmd+strlen(cmd),"&recipient=%llu",(long long)otherNXT);
    if ( triggerhash != 0 && triggerhash[0] != 0 )
    {
        if ( triggerheight == 0 )
            sprintf(cmd+strlen(cmd),"&referencedTransactionFullHash=%s",triggerhash);
        else sprintf(cmd+strlen(cmd),"&referencedTransactionFullHash=%s&phased=true&phasingFinishHeight=%u&phasingVotingModel=4&phasingQuorum=1&phasingLinkedFullHash=%s",triggerhash,triggerheight,triggerhash);
    }
    if ( comment != 0 && comment[0] != 0 )
        sprintf(cmd+strlen(cmd),"&message=%s",comment);
    if ( (jsonstr= issue_NXTPOST(cmd)) != 0 )
    {
        _stripwhite(jsonstr,' ');
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            copy_cJSON(errstr,cJSON_GetObjectItem(json,"error"));
            if ( errstr[0] == 0 )
                copy_cJSON(errstr,cJSON_GetObjectItem(json,"errorDescription"));
            if ( errstr[0] != 0 )
            {
                printf("submit_triggered_bidask.(%s) -> (%s)\n",cmd,jsonstr);
                if ( retjsonstrp != 0 )
                    *retjsonstrp = clonestr(errstr);
            }
            else txid = get_API_nxt64bits(cJSON_GetObjectItem(json,"transaction"));
        }
        free(jsonstr);
    }
    return(txid);
}

char *fill_nxtae(uint64_t *txidp,uint64_t nxt64bits,int32_t dir,double price,double volume,uint64_t baseid,uint64_t relid)
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
    if ( txidp != 0 )
        *txidp = txid;
    return(clonestr(retbuf));
}

uint64_t submit_to_exchange(int32_t exchangeid,char **jsonstrp,uint64_t assetid,uint64_t qty,uint64_t priceNQT,int32_t dir,uint64_t nxt64bits,char *NXTACCTSECRET,char *triggerhash,char *comment,uint64_t otherNXT,char *base,char *rel,double price,double volume,uint32_t triggerheight)
{
    uint64_t txid = 0;
    char assetidstr[64],*cmd,*retstr = 0;
    int32_t ap_type,decimals;
    struct exchange_info *exchange;
    *jsonstrp = 0;
    expand_nxt64bits(assetidstr,assetid);
    ap_type = get_assettype(&decimals,assetidstr);
    if ( dir == 0 || priceNQT == 0 )
        cmd = (ap_type == 2 ? "transferAsset" : "transferCurrency"), priceNQT = 0;
    else cmd = ((dir > 0) ? (ap_type == 2 ? "placeBidOrder" : "currencyBuy") : (ap_type == 2 ? "placeAskOrder" : "currencySell")), otherNXT = 0;
    if ( exchangeid == INSTANTDEX_NXTAEID || exchangeid == INSTANTDEX_UNCONFID )
    {
        if ( assetid != NXT_ASSETID && qty != 0 && (dir == 0 || priceNQT != 0) )
        {
            printf("submit to exchange.%s (%s) dir.%d\n",Exchanges[exchangeid].name,comment,dir);
            txid = submit_triggered_nxtae(jsonstrp,ap_type == 5,cmd,nxt64bits,NXTACCTSECRET,assetid,qty,priceNQT,triggerhash,comment,otherNXT,triggerheight);
            if ( *jsonstrp != 0 )
                txid = 0;
        }
    }
    else if ( exchangeid < MAX_EXCHANGES && (exchange= &Exchanges[exchangeid]) != 0 && exchange->exchangeid == exchangeid && exchange->trade != 0 )
    {
        printf("submit_to_exchange.(%d) dir.%d price %f vol %f | inv %f %f (%s)\n",exchangeid,dir,price,volume,1./price,price*volume,comment);
        if ( (txid= (*exchange->trade)(&retstr,exchange,base,rel,dir,price,volume)) == 0 )
            printf("illegal combo (%s/%s) ret.(%s)\n",base,rel,retstr!=0?retstr:"");
    }
    return(txid);
}

uint64_t InstantDEX_tradestub(char **retstrp,struct exchange_info *exchange,char *base,char *rel,int32_t dir,double price,double volume)
{
    printf("this is just a InstantDEX_tradestub\n");
    return(0);
}

uint64_t NXT_tradestub(char **retstrp,struct exchange_info *exchange,char *base,char *rel,int32_t dir,double price,double volume)
{
    printf("this is just a NXT_tradestub\n");
    return(0);
}

#endif
