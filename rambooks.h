//
//  rambooks.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_rambooks_h
#define xcode_rambooks_h

struct rambook_info
{
    UT_hash_handle hh;
    FILE *fp;
    char url[128],base[16],rel[16],lbase[16],lrel[16],exchange[16];
    struct InstantDEX_quote *quotes;
    uint64_t assetids[4];
    uint32_t lastaccess;
    int32_t numquotes,maxquotes;
    float lastmilli;
    uint8_t updated;
} *Rambooks;

struct rambook_info *find_rambook(uint64_t rambook_hashbits[4])
{
    struct rambook_info *rb;
    HASH_FIND(hh,Rambooks,rambook_hashbits,sizeof(rb->assetids),rb);
    return(rb);
}

uint64_t purge_oldest_order(struct rambook_info *rb,struct InstantDEX_quote *iQ) // allow one pair per orderbook
{
    char NXTaddr[64];
    struct NXT_acct *np;
    int32_t age,oldi,createdflag;
    uint64_t nxt64bits = 0;
    uint32_t now,i,oldest = 0;
    if ( rb->numquotes == 0 )
        return(0);
    oldi = -1;
    now = (uint32_t)time(NULL);
    for (i=0; i<rb->numquotes; i++)
    {
        age = (now - rb->quotes[i].timestamp);
        if ( rb->quotes[i].exchangeid == INSTANTDEX_EXCHANGEID && age >= ORDERBOOK_EXPIRATION )
        {
            if ( (iQ == 0 || rb->quotes[i].nxt64bits == iQ->nxt64bits) && (oldest == 0 || rb->quotes[i].timestamp < oldest) )
            {
                oldest = rb->quotes[i].timestamp;
                //fprintf(stderr,"(oldi.%d %u) ",j,oldest);
                nxt64bits = rb->quotes[i].nxt64bits;
                oldi = i;
            }
        }
    }
    if ( oldi >= 0 )
    {
        rb->quotes[oldi] = rb->quotes[--rb->numquotes];
        memset(&rb->quotes[rb->numquotes],0,sizeof(rb->quotes[rb->numquotes]));
        expand_nxt64bits(NXTaddr,nxt64bits);
        np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
        if ( np->openorders > 0 )
            np->openorders--;
        fprintf(stderr,"purge_oldest_order from NXT.%llu (openorders.%d) oldi.%d timestamp %u\n",(long long)nxt64bits,np->openorders,oldi,oldest);
    } //else fprintf(stderr,"no purges: numquotes.%d\n",rb->numquotes);
    return(nxt64bits);
}

struct rambook_info *get_rambook(char *_base,uint64_t baseid,char *_rel,uint64_t relid,char *exchange)
{
    char base[16],rel[16];
    uint64_t assetids[4],basemult,relmult,exchangebits;
    uint32_t i,basetype,reltype;
    struct rambook_info *rb;
    exchangebits = stringbits(exchange);
    if ( _base == 0 )
        basetype = set_assetname(&basemult,base,baseid);
    else basetype = INSTANTDEX_NATIVE, strcpy(base,_base);
    if ( _rel == 0 )
        reltype = set_assetname(&relmult,rel,relid);
    else reltype = INSTANTDEX_NATIVE, strcpy(rel,_rel);
    assetids[0] = baseid, assetids[1] = relid, assetids[2] = exchangebits, assetids[3] = (((uint64_t)basetype << 32) | reltype);
    if ( (rb= find_rambook(assetids)) == 0 )
    {
        rb = calloc(1,sizeof(*rb));
        strncpy(rb->exchange,exchange,sizeof(rb->exchange)-1);
        strcpy(rb->base,base), strcpy(rb->rel,rel);
        touppercase(rb->base), strcpy(rb->lbase,rb->base), tolowercase(rb->lbase);
        touppercase(rb->rel), strcpy(rb->lrel,rb->rel), tolowercase(rb->lrel);
        for (i=0; i<4; i++)
            rb->assetids[i] = assetids[i];
        if ( Debuglevel > 1 )
            printf("CREATE RAMBOOK.(%llu.%d -> %llu.%d) %s (%s) (%s)\n",(long long)baseid,basetype,(long long)relid,reltype,exchange,rb->base,rb->rel);
        HASH_ADD(hh,Rambooks,assetids,sizeof(rb->assetids),rb);
    }
    purge_oldest_order(rb,0);
    return(rb);
}

struct rambook_info **get_allrambooks(int32_t *numbooksp)
{
    int32_t i = 0;
    struct rambook_info *rb,*tmp,**obooks;
    *numbooksp = HASH_COUNT(Rambooks);
    obooks = calloc(*numbooksp,sizeof(*rb));
    HASH_ITER(hh,Rambooks,rb,tmp)
    {
        purge_oldest_order(rb,0);
        obooks[i++] = rb;
        //printf("rambook.(%s) %s %llu.%d / %s %llu.%d\n",rb->exchange,rb->base,(long long)rb->assetids[0],(int)(rb->assetids[3]>>32),rb->rel,(long long)rb->assetids[1],(uint32_t)rb->assetids[3]);
    }
    if ( i != *numbooksp )
        printf("get_allrambooks HASH_COUNT.%d vs i.%d\n",*numbooksp,i);
    return(obooks);
}

cJSON *rambook_json(struct rambook_info *rb)
{
    cJSON *json = cJSON_CreateObject();
    char numstr[64];
    cJSON_AddItemToObject(json,"base",cJSON_CreateString(rb->base));
    sprintf(numstr,"%llu",(long long)rb->assetids[0]), cJSON_AddItemToObject(json,"baseid",cJSON_CreateString(numstr));
    cJSON_AddItemToObject(json,"rel",cJSON_CreateString(rb->rel));
    sprintf(numstr,"%llu",(long long)rb->assetids[1]), cJSON_AddItemToObject(json,"relid",cJSON_CreateString(numstr));
    cJSON_AddItemToObject(json,"numquotes",cJSON_CreateNumber(rb->numquotes));
    sprintf(numstr,"%llu",(long long)rb->assetids[3]), cJSON_AddItemToObject(json,"type",cJSON_CreateString(numstr));
    cJSON_AddItemToObject(json,"exchange",cJSON_CreateString(rb->exchange));
    return(json);
}

cJSON *all_orderbooks()
{
    cJSON *array,*json = 0;
    struct rambook_info **obooks;
    int32_t i,numbooks;
    if ( (obooks= get_allrambooks(&numbooks)) != 0 )
    {
        array = cJSON_CreateArray();
        for (i=0; i<numbooks; i++)
            cJSON_AddItemToArray(array,rambook_json(obooks[i]));
        free(obooks);
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json,"orderbooks",array);
    }
    return(json);
}

struct InstantDEX_quote *findquoteid(uint64_t quoteid,int32_t evenclosed)
{
    struct rambook_info **obooks,*rb;
    struct InstantDEX_quote *iQ;
    int32_t i,j,numbooks;
    if ( (obooks= get_allrambooks(&numbooks)) != 0 )
    {
        for (i=0; i<numbooks; i++)
        {
            rb = obooks[i];
            if ( rb->numquotes == 0 )
                continue;
            for (j=0; j<rb->numquotes; j++)
            {
                iQ = &rb->quotes[j];
                if ( (evenclosed != 0 || iQ->closed == 0) && calc_quoteid(iQ) == quoteid )
                {
                    free(obooks);
                    return(iQ);
                }
            }
        }
        free(obooks);
    }
    return(0);
}

void add_user_order(struct rambook_info *rb,struct InstantDEX_quote *iQ)
{
    int32_t i;
    if ( rb->numquotes > 0 )
    {
        for (i=0; i<rb->numquotes; i++)
        {
            if ( memcmp(iQ,&rb->quotes[i],sizeof(rb->quotes[i])) == 0 )
                break;
        }
    } else i = 0;
    if ( i == rb->numquotes )
    {
        if ( i >= rb->maxquotes )
        {
            rb->maxquotes += 50;
            rb->quotes = realloc(rb->quotes,rb->maxquotes * sizeof(*rb->quotes));
            memset(&rb->quotes[i],0,50 * sizeof(*rb->quotes));
        }
        rb->quotes[rb->numquotes++] = *iQ;
    }
    rb->updated = 1;
   // printf("add_user_order i.%d numquotes.%d\n",i,rb->numquotes);
}

int32_t calc_users_maxopentrades(uint64_t nxt64bits)
{
    if ( is_exchange_nxt64bits(nxt64bits) != 0 )
        return(1000);
    return(13);
}

void save_InstantDEX_quote(struct rambook_info *rb,struct InstantDEX_quote *iQ)
{
    char NXTaddr[64];
    struct NXT_acct *np;
    int32_t createdflag,maxallowed;
    maxallowed = calc_users_maxopentrades(iQ->nxt64bits);
    expand_nxt64bits(NXTaddr,iQ->nxt64bits);
    np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
    if ( np->openorders >= maxallowed )
        purge_oldest_order(rb,iQ); // allow one pair per orderbook
    purge_oldest_order(rb,0);
    add_user_order(rb,iQ);
    np->openorders++;
}

struct rambook_info *_add_rambook_quote(struct InstantDEX_quote *iQ,struct rambook_info *rb,uint64_t nxt64bits,uint32_t timestamp,int32_t dir,double price,double volume,uint64_t baseid,uint64_t baseamount,uint64_t relid,uint64_t relamount,char *gui,uint64_t quoteid)
{
    if ( timestamp == 0 )
        timestamp = (uint32_t)time(NULL);
    if ( dir > 0 )
        create_InstantDEX_quote(iQ,timestamp,0,quoteid,price,volume,baseid,baseamount,relid,relamount,rb->exchange,nxt64bits,gui,0,0);
    else
    {
        if ( baseamount == 0 || relamount == 0 )
            set_best_amounts(&baseamount,&relamount,price,volume);
        create_InstantDEX_quote(iQ,timestamp,1,quoteid,0,0,relid,relamount,baseid,baseamount,rb->exchange,nxt64bits,gui,0,0);
    }
    return(rb);
}

struct rambook_info *add_rambook_quote(char *exchange,struct InstantDEX_quote *iQ,uint64_t nxt64bits,uint32_t timestamp,int32_t dir,uint64_t baseid,uint64_t relid,double price,double volume,uint64_t baseamount,uint64_t relamount,char *gui,uint64_t quoteid)
{
    void emit_iQ(struct rambook_info *rb,struct InstantDEX_quote *iQ);
    struct rambook_info *rb;
    memset(iQ,0,sizeof(*iQ));
    if ( timestamp == 0 )
        timestamp = (uint32_t)time(NULL);
    if ( dir > 0 )
    {
        rb = get_rambook(0,baseid,0,relid,exchange);
        _add_rambook_quote(iQ,rb,nxt64bits,timestamp,dir,price,volume,baseid,baseamount,relid,relamount,gui,quoteid);
    }
    else
    {
        rb = get_rambook(0,relid,0,baseid,exchange);
        _add_rambook_quote(iQ,rb,nxt64bits,timestamp,dir,price,volume,baseid,baseamount,relid,relamount,gui,quoteid);
    }
    save_InstantDEX_quote(rb,iQ);
    emit_iQ(rb,iQ);
    return(rb);
}

int32_t parseram_json_quotes(int32_t dir,struct rambook_info *rb,cJSON *array,int32_t maxdepth,char *pricefield,char *volfield,char *gui)
{
    cJSON *item;
    int32_t i,n,numitems;
    uint32_t reftimestamp,timestamp;
    uint64_t nxt64bits,quoteid = 0;
    double price,volume;
    struct InstantDEX_quote iQ;
    reftimestamp = (uint32_t)time(NULL);
    n = cJSON_GetArraySize(array);
    if ( maxdepth != 0 && n > maxdepth )
        n = maxdepth;
    for (i=0; i<n; i++)
    {
        nxt64bits = quoteid = timestamp = 0;
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
            timestamp = (uint32_t)get_API_int(cJSON_GetArrayItem(item,2),0);
            quoteid = get_API_nxt64bits(cJSON_GetArrayItem(item,3));
            nxt64bits = get_API_nxt64bits(cJSON_GetArrayItem(item,4));
        }
        else
        {
            printf("unexpected case in parseram_json_quotes\n");
            continue;
        }
        if ( timestamp == 0 )
            timestamp = reftimestamp;
        if ( Debuglevel > 1 )
            printf("%-8s %s %5s/%-5s %13.8f vol %13.8f | invert %13.8f vol %13.8f | timestmp.%u quoteid.%llu\n",rb->exchange,dir>0?"bid":"ask",rb->base,rb->rel,price,volume,1./price,volume*price,timestamp,(long long)quoteid);
        if ( _add_rambook_quote(&iQ,rb,nxt64bits,timestamp,dir,price,volume,rb->assetids[0],0,rb->assetids[1],0,gui,quoteid) != rb )
            printf("ERROR: rambook mismatch for %s/%s dir.%d price %.8f vol %.8f\n",rb->base,rb->rel,dir,price,volume);
        add_user_order(rb,&iQ);
    }
    return(n);
}

void ramparse_json_orderbook(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,cJSON *json,char *resultfield,char *bidfield,char *askfield,char *pricefield,char *volfield,char *gui)
{
    cJSON *obj = 0,*bidobj,*askobj;
    if ( resultfield == 0 )
        obj = json;
    if ( maxdepth == 0 )
        maxdepth = 10;
    if ( resultfield == 0 || (obj= cJSON_GetObjectItem(json,resultfield)) != 0 )
    {
        if ( (bidobj= cJSON_GetObjectItem(obj,bidfield)) != 0 && is_cJSON_Array(bidobj) != 0 )
            parseram_json_quotes(1,bids,bidobj,maxdepth,pricefield,volfield,gui);
        if ( (askobj= cJSON_GetObjectItem(obj,askfield)) != 0 && is_cJSON_Array(askobj) != 0 )
            parseram_json_quotes(-1,asks,askobj,maxdepth,pricefield,volfield,gui);
    }
}

struct InstantDEX_quote *clone_quotes(int32_t *nump,struct rambook_info *rb)
{
    struct InstantDEX_quote *quotes = 0;
    if ( (*nump= rb->numquotes) != 0 )
    {
        quotes = calloc(rb->numquotes,sizeof(*rb->quotes));
        memcpy(quotes,rb->quotes,rb->numquotes * sizeof(*rb->quotes));
        memset(rb->quotes,0,rb->numquotes * sizeof(*rb->quotes));
    }
    rb->numquotes = 0;
    return(quotes);
}

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

int32_t match_unconfirmed(struct rambook_info **obooks,int32_t numbooks,char *account,uint64_t quoteid)
{
    int32_t j,k;
    struct rambook_info *rb;
    struct InstantDEX_quote *iQ;
    if ( strcmp(account,INSTANTDEX_ACCT) == 0 && quoteid != 0 )
    {
        for (j=0; j<numbooks; j++)
        {
            rb = obooks[j];
            if ( strcmp(INSTANTDEX_NAME,rb->exchange) == 0 )
            {
                for (k=0; k<rb->numquotes; k++)
                {
                    iQ = &rb->quotes[k];
                    if ( calc_quoteid(iQ) == quoteid )
                    {
                        if ( iQ->matched == 0 )
                        {
                            iQ->matched = 1;
                            printf("MARK MATCHED TRADE FROM UNCONFIRMED\n");
                            return(1);
                        } else return(0);
                    }
                }
            }
        }
    }
    return(-1);
}

char *allorderbooks_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    cJSON *json;
    char *jsonstr;
    if ( (json= all_orderbooks()) != 0 )
    {
        jsonstr = cJSON_Print(json);
        free_json(json);
        return(jsonstr);
    }
    return(clonestr("{\"error\":\"no orderbooks\"}"));
}

cJSON *openorders_json(char *NXTaddr)
{
    cJSON *array,*item,*json = 0;
    struct rambook_info **obooks,*rb;
    struct InstantDEX_quote *iQ;
    int32_t i,j,numbooks,n = 0;
    uint64_t baseamount,relamount,ptr;
    char nxtaddr[64],numstr[64];
   // update_iQ_flags(0,0,0,0);
    if ( (obooks= get_allrambooks(&numbooks)) != 0 )
    {
        array = cJSON_CreateArray();
        for (i=0; i<numbooks; i++)
        {
            rb = obooks[i];
            if ( rb->numquotes == 0 )
                continue;
            for (j=0; j<rb->numquotes; j++)
            {
                iQ = &rb->quotes[j];
                expand_nxt64bits(nxtaddr,iQ->nxt64bits);
                if ( strcmp(NXTaddr,nxtaddr) == 0 && iQ->closed == 0 )
                {
                    baseamount = iQ->baseamount, relamount = iQ->relamount;
                    if ( (item= gen_InstantDEX_json(&baseamount,&relamount,0,iQ->isask,iQ,rb->assetids[0],rb->assetids[1],0)) != 0 )
                    {
                        ptr = (uint64_t)iQ;
                        sprintf(numstr,"%llu",(long long)ptr), cJSON_AddItemToObject(item,"iQ",cJSON_CreateString(numstr));
                        cJSON_AddItemToArray(array,item);
                        n++;
                    }
                }
            }
        }
        free(obooks);
        if ( n > 0 )
        {
            json = cJSON_CreateObject();
            cJSON_AddItemToObject(json,"openorders",array);
            return(json);
        }
    }
    return(json);
}

char *openorders_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    cJSON *json = 0;
    char *jsonstr;
    if ( (json= openorders_json(NXTaddr)) != 0 )
    {
        jsonstr = cJSON_Print(json);
        free_json(json);
        return(jsonstr);
    }
    return(clonestr("{\"error\":\"no openorders\"}"));
}

int32_t cancelquote(char *NXTaddr,uint64_t quoteid)
{
    struct InstantDEX_quote *iQ;
    char nxtaddr[64];
    if ( (iQ= findquoteid(quoteid,0)) != 0 )
    {
        expand_nxt64bits(nxtaddr,iQ->nxt64bits);
        if ( strcmp(NXTaddr,nxtaddr) == 0 && calc_quoteid(iQ) == quoteid )
        {
            cancel_InstantDEX_quote(iQ);
            return(1);
        }
    }
    return(0);
}

char *cancelquote_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    uint64_t quoteid;
    printf("inside cancelquote\n");
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    quoteid = get_API_nxt64bits(objs[0]);
    printf("cancelquote %llu\n",(long long)quoteid);
    if ( cancelquote(NXTaddr,quoteid) > 0 )
        return(clonestr("{\"result\":\"quote cancelled\"}"));
    else return(clonestr("{\"result\":\"no quote to cancel\"}"));
}

void ensure_rambook(uint64_t baseid,uint64_t relid)
{
    if ( baseid != NXT_ASSETID && relid != NXT_ASSETID )
    {
        init_rambooks(0,0,baseid,relid);
        init_rambooks(0,0,baseid,NXT_ASSETID);
        init_rambooks(0,0,relid,NXT_ASSETID);
    }
    else if ( baseid != NXT_ASSETID )
        init_rambooks(0,0,baseid,NXT_ASSETID);
    else if ( relid != NXT_ASSETID )
        init_rambooks(0,0,relid,NXT_ASSETID);
    else printf("ILLEGAL CASE for ORDERBOOK %llu/%llu\n",(long long)baseid,(long long)relid);
}
#endif
