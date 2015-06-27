//
//  exchanges.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_exchanges_h
#define xcode_exchanges_h

#include "exchangepairs.h"
#include "exchangeparse.h"

cJSON *exchanges_json()
{
    struct exchange_info *exchange; int32_t exchangeid,n = 0; char api[4]; cJSON *item,*array = cJSON_CreateArray();
    for (exchangeid=0; exchangeid<MAX_EXCHANGES; exchangeid++)
    {
        item = cJSON_CreateObject();
        exchange = &Exchanges[exchangeid];
        if ( exchange->name[0] == 0 )
            break;
        cJSON_AddItemToObject(item,"name",cJSON_CreateString(exchange->name));
        memset(api,0,sizeof(api));
        if ( exchange->trade != 0 )
        {
            if ( exchange->apikey[0] != 0 )
                api[n++] = 'K';
            if ( exchange->apisecret[0] != 0 )
                api[n++] = 'S';
            if ( exchange->userid[0] != 0 )
                api[n++] = 'U';
            cJSON_AddItemToObject(item,"trade",cJSON_CreateString(api));
        }
        cJSON_AddItemToArray(array,item);
    }
    return(array);
}

struct exchange_info *find_exchange(char *exchangestr,void (*ramparse)(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui),int32_t (*ramparse_supports)(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid))
{
    int32_t exchangeid;
    struct exchange_info *exchange = 0;
    //printf("FIND.(%s)\n",exchangestr);
    for (exchangeid=0; exchangeid<MAX_EXCHANGES; exchangeid++)
    {
        exchange = &Exchanges[exchangeid];
        //printf("(%s v %s) ",exchangestr,exchange->name);
        if ( exchange->name[0] == 0 )
        {
            if ( ramparse == 0 )
                return(0);
            strcpy(exchange->name,exchangestr);
            exchange->ramparse = ramparse;
            exchange->ramsupports = ramparse_supports;
            exchange->exchangeid = exchangeid;
            exchange->nxt64bits = stringbits(exchangestr);
            printf("CREATE EXCHANGE.(%s) id.%d %llu\n",exchangestr,exchangeid,(long long)exchange->nxt64bits);
            break;
        }
        if ( strcmp(exchangestr,exchange->name) == 0 )
            break;
    }
    return(exchange);
}

int32_t is_exchange_nxt64bits(uint64_t nxt64bits)
{
    int32_t exchangeid;
    struct exchange_info *exchange = 0;
    for (exchangeid=0; exchangeid<MAX_EXCHANGES; exchangeid++)
    {
        exchange = &Exchanges[exchangeid];
        // printf("(%s).(%llu vs %llu) ",exchange->name,(long long)exchange->nxt64bits,(long long)nxt64bits);
        if ( exchange->name[0] == 0 )
            return(0);
        if ( exchange->nxt64bits == nxt64bits )
            return(1);
    }
    printf("no exchangebits match\n");
    return(0);
}

int32_t get_exchangeid(char *exchangestr)
{
    int32_t exchangeid;
    struct exchange_info *exchange = 0;
    for (exchangeid=0; exchangeid<MAX_EXCHANGES; exchangeid++)
    {
        exchange = &Exchanges[exchangeid];
        // printf("(%s).(%llu vs %llu) ",exchange->name,(long long)exchange->nxt64bits,(long long)nxt64bits);
        if ( exchange->name[0] == 0 )
            return(-1);
        if ( strcmp(exchangestr,exchange->name) == 0 )
            return(exchangeid);
    }
    printf("no get_exchangeid match.(%s)\n",exchangestr);
    return(-1);
}

/*void add_to_exchange(int32_t exchangeid,struct rambook_info *bids,struct rambook_info *asks)
{
    int32_t i,j,n;
    struct exchange_info *exchange;
    struct orderpair *pair;
    if ( exchangeid >= MAX_EXCHANGES )
    {
        printf("add_to_exchange: illegal exchangeid.%d\n",exchangeid);
        return;
    }
    exchange = &Exchanges[exchangeid];
    if ( exchange->exchangeid != exchangeid )
    {
        printf("add_to_exchange: illegal exchangeid.%d for (%s).%d\n",exchangeid,exchange->name,exchange->exchangeid);
        return;
    }
    printf("exchangeid.%d numpairs.%d\n",exchangeid,exchange->num);
    if ( (n= exchange->num) == 0 )
    {
        if ( Debuglevel > 2 )
            printf("Start monitoring (%s).%d\n",exchange->name,exchangeid);
        exchange->exchangeid = exchangeid;
        i = n;
    }
    else
    {
        for (i=0; i<n; i++)
        {
            pair = &exchange->orderpairs[i];
            for (j=0; j<3; j++)
            {
                if ( pair->bids->assetids[j] != bids->assetids[j] ||  pair->asks->assetids[j] != asks->assetids[j] )
                    break;
            }
            if ( j != 3 )
                break;
            //printf("(%s/%s) ",pair->bids->base,pair->bids->rel);
        }
        //printf(" vs (%s/%s)\n",base,rel);
    }
    if ( i == n )
    {
        pair = &exchange->orderpairs[n];
        pair->bids = bids, pair->asks = asks;
        if ( exchange->num++ >= (int32_t)(sizeof(exchange->orderpairs)/sizeof(*exchange->orderpairs)) )
        {
            printf("exchange.(%s) orderpairs hit max\n",exchange->name);
            exchange->num = ((int32_t)(sizeof(exchange->orderpairs)/sizeof(*exchange->orderpairs)) - 1);
        } else printf("exchange.(%s) orderpairs n.%d of %d\n",exchange->name,n,(int32_t)(sizeof(exchange->orderpairs)/sizeof(*exchange->orderpairs)));
    }
}*/

#define SHA512_DIGEST_SIZE (512 / 8)

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

#endif
