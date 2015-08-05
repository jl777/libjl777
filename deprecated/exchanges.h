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

double get_current_rate(char *base,char *rel)
{
    struct coin_info *cp;
    if ( strcmp(rel,"NXT") == 0 )
    {
        if ( (cp= get_coin_info(base)) != 0 )
        {
            if ( cp->RAM.NXTconvrate != 0. )
                return(cp->RAM.NXTconvrate);
            if ( cp->NXTfee_equiv != 0 && cp->txfee != 0 )
                return(cp->NXTfee_equiv / cp->txfee);
        }
    }
    return(1.);
}
void ensure_dir(char *dirname)
{
    FILE *fp;
    char fname[512],cmd[512];
    sprintf(fname,"%s/tmp",dirname);
    if ( (fp= fopen(os_compatible_path(fname),"rb")) == 0 )
    {
        sprintf(cmd,"mkdir %s",os_compatible_path(dirname));
        if ( system(os_compatible_path(cmd)) != 0 )
            printf("error making subdirectory (%s) %s (%s)\n",cmd,dirname,fname);
        fp = fopen(os_compatible_path(fname),"wb");
        if ( fp != 0 )
            fclose(fp);
        if ( (fp= fopen(os_compatible_path(fname),"rb")) == 0 )
        {
            printf("failed to create.(%s) in (%s)\n",fname,dirname);
            exit(-1);
        } else printf("ensure_dir(%s) created.(%s)\n",dirname,fname);
    }
    fclose(fp);
}

void set_exchange_fname(char *fname,char *exchangestr,char *base,char *rel,uint64_t baseid,uint64_t relid)
{
    char exchange[16],basestr[64],relstr[64];
    uint64_t mult;
    if ( strcmp(exchange,"iDEX") == 0 )
        strcpy(exchange,"InstantDEX");
    else strcpy(exchange,exchangestr);
    ensure_dir(PRICEDIR);
    sprintf(fname,"%s/%s",PRICEDIR,exchange);
    ensure_dir(fname);
    if ( is_cryptocoin(base) != 0 )
        strcpy(basestr,base);
    else set_assetname(&mult,basestr,baseid);
    if ( is_cryptocoin(rel) != 0 )
        strcpy(relstr,rel);
    else set_assetname(&mult,relstr,relid);
    sprintf(fname,"%s/%s/%s_%s",PRICEDIR,exchange,basestr,relstr);
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

uint64_t bter_trade(char **retstrp,struct exchange_info *exchange,char *base,char *rel,int32_t dir,double price,double volume)
{
    static CURL *cHandle;
 	char *sig,*data,buf[512],cmdbuf[8192],hdr1[1024],hdr2[1024],pairstr[512],dest[SHA512_DIGEST_SIZE*2 + 1],lbase[16],lrel[16];
    cJSON *json; uint64_t txid = 0;
    strcpy(lbase,base), tolowercase(lbase), strcpy(lrel,rel), tolowercase(lrel);
    if ( strcmp(lbase,"cny") == 0 || strcmp(lrel,"cny") == 0 )
        dir = flip_for_exchange(pairstr,"%s_%s","cny",dir,&price,&volume,lbase,lrel);
    else if ( strcmp(lbase,"btc") == 0 || strcmp(lrel,"btc") == 0 )
        dir = flip_for_exchange(pairstr,"%s_%s","btc",dir,&price,&volume,lbase,lrel);
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

uint64_t submit_to_exchange(int32_t exchangeid,char **jsonstrp,uint64_t assetid,uint64_t qty,uint64_t priceNQT,int32_t dir,uint64_t nxt64bits,char *NXTACCTSECRET,char *triggerhash,char *comment,uint64_t otherNXT,char *base,char *rel,double price,double volume)
{
    uint64_t txid = 0;
    char assetidstr[64],*cmd,*retstr = 0;
    struct NXT_asset *ap;
    int32_t createdflag;
    struct exchange_info *exchange;
    *jsonstrp = 0;
    expand_nxt64bits(assetidstr,assetid);
    ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
    if ( dir == 0 || priceNQT == 0 )
        cmd = (ap->type == 2 ? "transferAsset" : "transferCurrency"), priceNQT = 0;
    else cmd = ((dir > 0) ? (ap->type == 2 ? "placeBidOrder" : "currencyBuy") : (ap->type == 2 ? "placeAskOrder" : "currencySell")), otherNXT = 0;
    if ( exchangeid == INSTANTDEX_NXTAEID || exchangeid == INSTANTDEX_UNCONFID )
    {
        if ( assetid != NXT_ASSETID && qty != 0 && (dir == 0 || priceNQT != 0) )
        {
            printf("submit to exchange.%s (%s) dir.%d\n",Exchanges[exchangeid].name,comment,dir);
            txid = submit_triggered_nxtae(jsonstrp,ap->type == 5,cmd,nxt64bits,NXTACCTSECRET,assetid,qty,priceNQT,triggerhash,comment,otherNXT);
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
