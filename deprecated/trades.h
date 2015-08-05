//
//  trades.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_trades_h
#define xcode_trades_h

struct tradehistory { uint64_t assetid,purchased,sold; };

struct tradehistory *_update_tradehistory(struct tradehistory *hist,uint64_t assetid,uint64_t purchased,uint64_t sold)
{
    int32_t i = 0;
    if ( hist == 0 )
        hist = calloc(1,sizeof(*hist));
    if ( hist[i].assetid != 0 )
    {
        for (i=0; hist[i].assetid!=0; i++)
            if ( hist[i].assetid == assetid )
                break;
    }
    if ( hist[i].assetid == 0 )
    {
        hist = realloc(hist,(i+2) * sizeof(*hist));
        memset(&hist[i],0,2 * sizeof(hist[i]));
        hist[i].assetid = assetid;
    }
    if ( hist[i].assetid == assetid )
    {
        hist[i].purchased += purchased;
        hist[i].sold += sold;
        printf("hist[%d] %llu +%llu -%llu -> (%llu %llu)\n",i,(long long)hist[i].assetid,(long long)purchased,(long long)sold,(long long)hist[i].purchased,(long long)hist[i].sold);
    } else printf("_update_tradehistory: impossible case!\n");
    return(hist);
}

struct tradehistory *update_tradehistory(struct tradehistory *hist,uint64_t srcasset,uint64_t srcamount,uint64_t destasset,uint64_t destamount)
{
    hist = _update_tradehistory(hist,srcasset,0,srcamount);
    hist = _update_tradehistory(hist,destasset,destamount,0);
    return(hist);
}

cJSON *_tradehistory_json(struct tradehistory *asset)
{
    cJSON *json = cJSON_CreateObject();
    char numstr[64];
    sprintf(numstr,"%llu",(long long)asset->assetid), cJSON_AddItemToObject(json,"assetid",cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",dstr(asset->purchased)), cJSON_AddItemToObject(json,"purchased",cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",dstr(asset->sold)), cJSON_AddItemToObject(json,"sold",cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",dstr(asset->purchased) - dstr(asset->sold)), cJSON_AddItemToObject(json,"net",cJSON_CreateString(numstr));
    return(json);
}

cJSON *tradehistory_json(struct tradehistory *hist,cJSON *array)
{
    cJSON *assets,*netpos,*item,*json = cJSON_CreateObject();
    int32_t i;
    uint64_t mult;
    char assetname[64],numstr[64];
    cJSON_AddItemToObject(json,"rawtrades",array);
    assets = cJSON_CreateArray();
    netpos = cJSON_CreateArray();
    for (i=0; hist[i].assetid!=0; i++)
    {
        cJSON_AddItemToArray(assets,_tradehistory_json(&hist[i]));
        item = cJSON_CreateObject();
        set_assetname(&mult,assetname,hist[i].assetid);
        cJSON_AddItemToObject(item,"asset",cJSON_CreateString(assetname));
        sprintf(numstr,"%.8f",dstr(hist[i].purchased) - dstr(hist[i].sold)), cJSON_AddItemToObject(item,"net",cJSON_CreateString(numstr));
        cJSON_AddItemToArray(netpos,item);
    }
    cJSON_AddItemToObject(json,"assets",assets);
    cJSON_AddItemToObject(json,"netpositions",netpos);
    return(json);
}

cJSON *tabulate_trade_history(uint64_t mynxt64bits,cJSON *array)
{
    int32_t i,n;
    cJSON *item;
    long balancing;
    struct tradehistory *hist = 0;
    uint64_t src64bits,srcamount,srcasset,dest64bits,destamount,destasset,jump64bits,jumpamount,jumpasset;
    //{"requestType":"processjumptrade","NXT":"5277534112615305538","assetA":"5527630","amountA":"6700000000","other":"1510821971811852351","assetB":"12982485703607823902","amountB":"100000000","feeA":"250000000","balancing":0,"feeAtxid":"1234468909119892020","triggerhash":"34ea5aaeeeb62111a825a94c366b4ae3d12bb73f9a3413a27d1b480f6029a73c"}
    if ( array != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
    {
        for (i=0; i<n; i++)
        {
            item = cJSON_GetArrayItem(array,i);
            src64bits = get_API_nxt64bits(cJSON_GetObjectItem(item,"NXT"));
            srcamount = get_API_nxt64bits(cJSON_GetObjectItem(item,"amountA"));
            srcasset = get_API_nxt64bits(cJSON_GetObjectItem(item,"assetA"));
            dest64bits = get_API_nxt64bits(cJSON_GetObjectItem(item,"other"));
            destamount = get_API_nxt64bits(cJSON_GetObjectItem(item,"amountB"));
            destasset = get_API_nxt64bits(cJSON_GetObjectItem(item,"assetB"));
            jump64bits = get_API_nxt64bits(cJSON_GetObjectItem(item,"jumper"));
            jumpamount = get_API_nxt64bits(cJSON_GetObjectItem(item,"jumpasset"));
            jumpasset = get_API_nxt64bits(cJSON_GetObjectItem(item,"jumpamount"));
            balancing = (long)get_API_int(cJSON_GetObjectItem(item,"balancing"),0);
            if ( src64bits != 0 && srcamount != 0 && srcasset != 0 && dest64bits != 0 && destamount != 0 && destasset != 0 )
            {
                if ( src64bits == mynxt64bits )
                    hist = update_tradehistory(hist,srcasset,srcamount,destasset,destamount);
                else if ( dest64bits == mynxt64bits )
                    hist = update_tradehistory(hist,destasset,destamount,srcasset,srcamount);
                else if ( jump64bits == mynxt64bits )
                    continue;
                else printf("illegal tabulate_trade_entry %llu: (%llu -> %llu) via %llu\n",(long long)mynxt64bits,(long long)src64bits,(long long)dest64bits,(long long)jump64bits);
            } else printf("illegal tabulate_trade_entry %llu: %llu %llu %llu || %llu %llu %llu\n",(long long)mynxt64bits,(long long)src64bits,(long long)srcamount,(long long)srcasset,(long long)dest64bits,(long long)destamount,(long long)destasset);
        }
    }
    if ( hist != 0 )
    {
        array = tradehistory_json(hist,array);
        free(hist);
    }
    return(array);
}

cJSON *get_tradehistory(char *refNXTaddr,uint32_t timestamp)
{
    char cmdstr[1024],NXTaddr[64],receiverstr[MAX_JSON_FIELD],message[MAX_JSON_FIELD],newtriggerhash[MAX_JSON_FIELD],triggerhash[MAX_JSON_FIELD],*jsonstr;
    cJSON *json,*array,*txobj,*msgobj,*attachment,*retjson = 0,*histarray = 0;
    int32_t i,j,n,m,duplicates = 0;
    uint64_t senderbits;
    if ( timestamp == 0 )
        timestamp = 38785003;
    sprintf(cmdstr,"requestType=getAccountTransactions&account=%s&timestamp=%u&withMessage=true",refNXTaddr,timestamp);
    if ( (jsonstr= bitcoind_RPC(0,"curl",NXTAPIURL,0,0,cmdstr)) != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"transactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    txobj = cJSON_GetArrayItem(array,i);
                    copy_cJSON(receiverstr,cJSON_GetObjectItem(txobj,"recipient"));
                    if ( (senderbits = get_API_nxt64bits(cJSON_GetObjectItem(txobj,"sender"))) != 0 )
                    {
                        expand_nxt64bits(NXTaddr,senderbits);
                        if ( refNXTaddr != 0 && strcmp(NXTaddr,refNXTaddr) == 0 )
                        {
                            if ( (attachment= cJSON_GetObjectItem(txobj,"attachment")) != 0 && (msgobj= cJSON_GetObjectItem(attachment,"message")) != 0 )
                            {
                                copy_cJSON(message,msgobj);
                                //printf("(%s) -> ",message);
                                unstringify(message);
                                if ( (msgobj= cJSON_Parse(message)) != 0 )
                                {
                                    //printf("(%s)\n",message);
                                    if ( histarray == 0 )
                                        histarray = cJSON_CreateArray(), j = m = 0;
                                    else
                                    {
                                        copy_cJSON(newtriggerhash,cJSON_GetObjectItem(msgobj,"triggerhash"));
                                        m = cJSON_GetArraySize(histarray);
                                        for (j=0; j<m; j++)
                                        {
                                            copy_cJSON(triggerhash,cJSON_GetObjectItem(cJSON_GetArrayItem(histarray,j),"triggerhash"));
                                            if ( strcmp(triggerhash,newtriggerhash) == 0 )
                                            {
                                                duplicates++;
                                                break;
                                            }
                                        }
                                    }
                                    if ( j == m )
                                        cJSON_AddItemToArray(histarray,msgobj);
                                } else printf("parse error on.(%s)\n",message);
                            }
                        }
                    }
                }
            }
            free_json(json);
        }
        free(jsonstr);
    }
    if ( histarray != 0 )
        retjson = tabulate_trade_history(calc_nxt64bits(refNXTaddr),histarray);
    printf("duplicates.%d\n",duplicates);
    return(retjson);
}

char *tradehistory_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char *retstr;
    cJSON *history,*json;//,*openorders = 0;
    uint32_t firsttimestamp;
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    json = cJSON_CreateObject();
    firsttimestamp = (uint32_t)get_API_int(objs[0],0);
    history = get_tradehistory(NXTaddr,firsttimestamp);
    if ( history != 0 )
        cJSON_AddItemToObject(json,"tradehistory",history);
    //if ( (openorders= openorders_json(NXTaddr)) != 0 )
    //    cJSON_AddItemToObject(json,"openorders",openorders);
    retstr = cJSON_Print(json);
    free_json(json);
    stripwhite_ns(retstr,strlen(retstr));
    return(retstr);
}

/*
 cJSON *_tradeleg_json(struct _tradeleg *halfleg)
 {
 uint32_t set_assetname(uint64_t *multp,char *name,uint64_t assetbits);
 cJSON *json = cJSON_CreateObject();
 char numstr[64],name[64];
 uint64_t mult;
 sprintf(numstr,"%llu",(long long)halfleg->nxt64bits), cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(numstr));
 sprintf(numstr,"%llu",(long long)halfleg->assetid), cJSON_AddItemToObject(json,"assetid",cJSON_CreateString(numstr));
 set_assetname(&mult,name,halfleg->assetid), cJSON_AddItemToObject(json,"name",cJSON_CreateString(name));
 sprintf(numstr,"%.8f",dstr(halfleg->amount)), cJSON_AddItemToObject(json,"amount",cJSON_CreateString(numstr));
 return(json);
 }
 
 cJSON *tradeleg_json(struct tradeleg *leg)
 {
 cJSON *json = cJSON_CreateObject();
 char numstr[64];
 cJSON_AddItemToObject(json,"src",_tradeleg_json(&leg->src));
 cJSON_AddItemToObject(json,"dest",_tradeleg_json(&leg->dest));
 sprintf(numstr,"%llu",(long long)leg->nxt64bits), cJSON_AddItemToObject(json,"sender",cJSON_CreateString(numstr));
 if ( leg->txid != 0 )
 sprintf(numstr,"%llu",(long long)leg->txid), cJSON_AddItemToObject(json,"txid",cJSON_CreateString(numstr));
 sprintf(numstr,"%llu",(long long)leg->qty), cJSON_AddItemToObject(json,"qty",cJSON_CreateString(numstr));
 sprintf(numstr,"%.8f",dstr(leg->NXTprice)), cJSON_AddItemToObject(json,"price",cJSON_CreateString(numstr));
 if ( leg->expiration != 0 )
 cJSON_AddItemToObject(json,"expiration",cJSON_CreateNumber(leg->expiration - time(NULL)));
 return(json);
 }
 
 cJSON *jumptrade_json(struct jumptrades *jtrades)
 {
 cJSON *array = cJSON_CreateArray(),*json;
 int32_t i;
 if ( (json= cJSON_Parse(jtrades->comment)) != 0 )
 {
 for (i=0; i<jtrades->numlegs; i++)
 cJSON_AddItemToArray(array,tradeleg_json(&jtrades->legs[i]));
 cJSON_AddItemToObject(json,"legs",array);
 cJSON_AddItemToObject(json,"numlegs",cJSON_CreateNumber(jtrades->numlegs));
 cJSON_AddItemToObject(json,"state",cJSON_CreateNumber(jtrades->state));
 cJSON_AddItemToObject(json,"expiration",cJSON_CreateNumber((milliseconds() - jtrades->endmilli) / 1000.));
 } else json = cJSON_CreateObject();
 return(json);
 }*/

cJSON *jumptrades_json()
{
    cJSON *array = 0,*json = cJSON_CreateObject();
    /*struct jumptrades *jtrades = 0;
     int32_t i;
     for (i=0; i<(int32_t)(sizeof(JTRADES)/sizeof(*JTRADES)); i++)
     {
     jtrades = &JTRADES[i];
     if ( jtrades->state != 0 )
     {
     if ( array == 0 )
     array = cJSON_CreateArray();
     cJSON_AddItemToArray(array,jumptrade_json(jtrades));
     }
     }*/
    if ( array != 0 )
        cJSON_AddItemToObject(json,"jumptrades",array);
    return(json);
}

char *jumptrades_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char *retstr;
    cJSON *json;
    json = jumptrades_json();
    retstr = cJSON_Print(json);
    free_json(json);
    return(retstr);
}


#endif
