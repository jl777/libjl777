//
//  signals.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_signals_h
#define xcode_signals_h

void emit_iQ(struct rambook_info *rb,struct InstantDEX_quote *iQ)
{
    long offset = 0;
    double price,vol;
    uint8_t data[sizeof(uint64_t) * 2 + sizeof(uint32_t)];
    /*    char fname[1024];
     if ( rb->fp == 0 )
    {
        set_exchange_fname(fname,rb->exchange,rb->base,rb->rel,rb->assetids[0],rb->assetids[1]);
        if ( (rb->fp= fopen(os_compatible_path(fname),"rb+")) != 0 )
            fseek(rb->fp,0,SEEK_SET);
        else rb->fp = fopen(os_compatible_path(fname),"wb+");
        printf("opened.(%s) fpos.%ld\n",fname,ftell(rb->fp));
    }*/
    if ( rb->fp != 0 )
    {
        offset = 0;
        memcpy(&data[offset],&iQ->baseamount,sizeof(iQ->baseamount)), offset += sizeof(iQ->baseamount);
        memcpy(&data[offset],&iQ->relamount,sizeof(iQ->relamount)), offset += sizeof(iQ->relamount);
        memcpy(&data[offset],&iQ->timestamp,sizeof(iQ->timestamp)), offset += sizeof(iQ->timestamp);
        fwrite(data,1,offset,rb->fp);
        fflush(rb->fp);
        price = calc_price_volume(&vol,iQ->baseamount,iQ->relamount);
        // printf("emit.(%s) %12.8f %12.8f %s_%s %16llu %16llu\n",rb->exchange,price,vol,rb->base,rb->rel,(long long)iQ->baseamount,(long long)iQ->relamount);
    }
}

int32_t scan_exchange_prices(void (*process_quote)(void *ptr,int32_t arg,struct InstantDEX_quote *iQ),void *ptr,int32_t arg,char *exchange,char *base,char *rel,uint64_t baseid,uint64_t relid)
{
    int32_t n = 0;
    /*FILE *fp;
    long offset = 0;
    char fname[1024];
    struct InstantDEX_quote iQ;
    uint8_t data[sizeof(uint64_t) * 2 + sizeof(uint32_t)];
    set_exchange_fname(fname,exchange,base,rel,baseid,relid);
    if ( (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
    {
        while ( fread(data,1,sizeof(data),fp) == sizeof(data) )
        {
            memset(&iQ,0,sizeof(iQ));
            offset = 0;
            memcpy(&iQ.baseamount,&data[offset],sizeof(iQ.baseamount)), offset += sizeof(iQ.baseamount);
            memcpy(&iQ.relamount,&data[offset],sizeof(iQ.relamount)), offset += sizeof(iQ.relamount);
            memcpy(&iQ.timestamp,&data[offset],sizeof(iQ.timestamp)), offset += sizeof(iQ.timestamp);
            process_quote(ptr,arg,&iQ);
            n++;
        }
        fclose(fp);
    }*/
    return(n);
}

char *allsignals_func(int32_t localaccess,int32_t valid,char *sender,cJSON **objs,int32_t numobjs,char *origargstr)
{
    cJSON *array,*item,*json = cJSON_CreateObject();
    char *retstr;
    int32_t i;
    array = cJSON_CreateArray();
    for (i=0; i<NUM_BARPRICES; i++)
    {
        item = cJSON_CreateObject();
        cJSON_AddItemToObject(item,"signal",cJSON_CreateString(barinames[i]));
        cJSON_AddItemToObject(item,"scale",cJSON_CreateString("price"));
        cJSON_AddItemToArray(array,item);
    }
    item = cJSON_CreateObject();
    cJSON_AddItemToObject(item,"signal",cJSON_CreateString("ohlc"));
    cJSON_AddItemToObject(item,"scale",cJSON_CreateString("price"));
    cJSON_AddItemToObject(item,"n",cJSON_CreateNumber(4));
    cJSON_AddItemToArray(array,item);
    
    item = cJSON_CreateObject();
    cJSON_AddItemToObject(item,"signal",cJSON_CreateString("volume"));
    cJSON_AddItemToObject(item,"scale",cJSON_CreateString("positive"));
    cJSON_AddItemToArray(array,item);
    
    cJSON_AddItemToObject(json,"signals",array);
    retstr = cJSON_Print(json);
    free_json(json);
    return(retstr);
}

/*void update_displaybars(void *ptr,int32_t dir,struct InstantDEX_quote *iQ)
{
    struct displaybars *bars = ptr;
    double price,vol;
    int32_t ind;
    ind = (int32_t)((long)iQ->timestamp - bars->start) / bars->resolution;
    price = calc_price_volume(&vol,iQ->baseamount,iQ->relamount);
    if ( ind >= 0 && ind < bars->width )
    {
        update_bar(bars->bars[ind],dir > 0 ? price : 0,dir < 0 ? price : 0);
        //printf("ind.%d %u: arg.%d %-6ld %12.8f %12.8f %llu/%llu\n",ind,iQ->timestamp,dir,iQ->timestamp-time(NULL),price,vol,(long long)iQ->baseamount,(long long)iQ->relamount);
    }
}*/

cJSON *ohlc_json(float bar[NUM_BARPRICES])
{
    cJSON *array = cJSON_CreateArray();
    double prices[4];
    int32_t i;
    memset(prices,0,sizeof(prices));
    if ( bar[BARI_FIRSTBID] != 0.f && bar[BARI_FIRSTASK] != 0.f )
        prices[0] = (bar[BARI_FIRSTBID] + bar[BARI_FIRSTASK]) / 2.f;
    if ( bar[BARI_HIGHBID] != 0.f && bar[BARI_LOWASK] != 0.f )
    {
        if ( bar[BARI_HIGHBID] < bar[BARI_LOWASK] )
            prices[1] = bar[BARI_LOWASK], prices[2] = bar[BARI_HIGHBID];
        else prices[2] = bar[BARI_LOWASK], prices[1] = bar[BARI_HIGHBID];
    }
    if ( bar[BARI_LASTBID] != 0.f && bar[BARI_LASTASK] != 0.f )
        prices[3] = (bar[BARI_LASTBID] + bar[BARI_LASTASK]) / 2.f;
    //printf("ohlc_json %f %f %f %f\n",prices[0],prices[1],prices[2],prices[3]);
    for (i=0; i<4; i++)
        cJSON_AddItemToArray(array,cJSON_CreateNumber(prices[i]));
    return(array);
}

/*int32_t finalize_displaybars(struct displaybars *bars)
{
    int32_t ind,nonz = 0;
    float *bar,aveprice;
    for (ind=0; ind<bars->width; ind++)
    {
        bar = bars->bars[ind];
        if ( (aveprice= calc_barprice_aves(bar)) != 0.f )
            nonz++;
    }
    return(nonz);
}*/

int32_t conv_sigstr(char *sigstr)
{
    int32_t bari;
    if ( strcmp(sigstr,"ohlc") == 0 )
        return(NUM_BARPRICES);
    for (bari=0; bari<NUM_BARPRICES; bari++)
        if ( strcmp(barinames[bari],sigstr) == 0 )
            return(bari);
    return(-1);
}

char *getsignal_func(int32_t localaccess,int32_t valid,char *sender,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char sigstr[MAX_JSON_FIELD],base[MAX_JSON_FIELD],rel[MAX_JSON_FIELD],exchange[MAX_JSON_FIELD],*retstr;
    uint32_t width,resolution,now = (uint32_t)time(NULL);
    // struct InstantDEX_quote *iQ = 0;
    int32_t i,start;//,sigid,numbids,numasks = 0;
    uint64_t baseid,relid;
    struct displaybars *bars = 0;
    cJSON *json=0,*array=0;
    copy_cJSON(sigstr,objs[0]);
    start = (int32_t)get_API_int(objs[1],0);
    if ( start < 0 )
        start += now;
    width = (uint32_t)get_API_int(objs[2],now);
    if ( width < 1 || width >= 4096 )
        return(clonestr("{\"error\":\"too wide\"}"));
    resolution = (uint32_t)get_API_int(objs[3],60);
    baseid = get_API_nxt64bits(objs[4]);
    relid = get_API_nxt64bits(objs[5]);
    copy_cJSON(base,objs[6]), base[15] = 0;
    copy_cJSON(rel,objs[7]), rel[15] = 0;
    copy_cJSON(exchange,objs[8]);
    if ( strcmp(sigstr,"volume") == 0 )
    {
        json = cJSON_CreateObject();
        array = cJSON_CreateArray();
        for (i=0; i<width; i++)
            cJSON_AddItemToArray(array,cJSON_CreateNumber(rand() % 100000));
    }
    else
    {
       /* bars = calloc(1,sizeof(*bars));
        bars->baseid = baseid, bars->relid = relid, bars->resolution = resolution, bars->width = width, bars->start = start;
        bars->end = start + width*resolution;
        printf("now %ld start.%u end.%u res.%d width.%d\n",time(NULL),start,bars->end,resolution,width);
        if ( bars->end > time(NULL)+100*resolution )
            return(clonestr("{\"error\":\"too far in future\"}"));
        strcpy(bars->base,base), strcpy(bars->rel,rel), strcpy(bars->exchange,exchange);
        numbids = scan_exchange_prices(update_displaybars,bars,1,exchange,base,rel,baseid,relid);
        numasks = scan_exchange_prices(update_displaybars,bars,-1,exchange,base,rel,baseid,relid);
        if ( numbids == 0 && numasks == 0)
            return(clonestr("{\"error\":\"no data\"}"));
        if ( finalize_displaybars(bars) > 0 && (sigid= conv_sigstr(sigstr)) >= 0 )
        {
            printf("sigid.%d now %ld start.%u end.%u res.%d width.%d | numbids.%d numasks.%d\n",sigid,time(NULL),bars->start,bars->end,bars->resolution,bars->width,numbids,numasks);
            json = cJSON_CreateObject();
            array = cJSON_CreateArray();
            for (i=0; i<bars->width; i++)
            {
                if ( sigid < NUM_BARPRICES )
                    cJSON_AddItemToArray(array,cJSON_CreateNumber(bars->bars[i][sigid]));
                else cJSON_AddItemToArray(array,ohlc_json(bars->bars[i]));
            }
        } else free(bars);*/
    }
    if ( json != 0 && array != 0 )
    {
        cJSON_AddItemToObject(json,sigstr,array);
        retstr = cJSON_Print(json);
        free_json(json);
        if ( bars != 0 )
            free(bars);
        return(retstr);
    }
    return(clonestr("{\"error\":\"no data\"}"));
}

#endif
