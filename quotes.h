//
//  quotes.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_quotes_h
#define xcode_quotes_h

struct normal_fields { uint64_t nxt64bits,quoteid; struct InstantDEX_quote *baseiQ,*reliQ; };
union quotefields { struct normal_fields normal; };
struct InstantDEX_quote
{
    struct InstantDEX_quote *baseiQ,*reliQ;
    uint64_t quoteid,baseid,baseamount,relid,relamount,nxt64bits;
    uint32_t timestamp;
    uint8_t closed:1,sent:1,matched:1,isask:1,pad2:4,minperc:7;
    char exchangeid,gui[9];
};

void clear_InstantDEX_quoteflags(struct InstantDEX_quote *iQ) { iQ->closed = iQ->sent = iQ->matched = 0; }
void cancel_InstantDEX_quote(struct InstantDEX_quote *iQ) { iQ->closed = iQ->sent = iQ->matched = 1; }

uint64_t calc_quoteid(struct InstantDEX_quote *iQ)
{
    struct InstantDEX_quote Q;
    if ( iQ->quoteid == 0 )
    {
        Q = *iQ;
        clear_InstantDEX_quoteflags(&Q);
        return(calc_txid((uint8_t *)&Q,sizeof(Q)));
    } return(iQ->quoteid);
}

int _decreasing_quotes(const void *a,const void *b)
{
#define iQ_a ((struct InstantDEX_quote *)a)
#define iQ_b ((struct InstantDEX_quote *)b)
    double vala,valb,volume;
    vala = iQ_a->isask == 0 ? calc_price_volume(&volume,iQ_a->baseamount,iQ_a->relamount) : calc_price_volume(&volume,iQ_a->relamount,iQ_a->baseamount);
    valb = iQ_b->isask == 0 ? calc_price_volume(&volume,iQ_b->baseamount,iQ_b->relamount) : calc_price_volume(&volume,iQ_b->relamount,iQ_b->baseamount);
	if ( valb > vala )
		return(1);
	else if ( valb < vala )
		return(-1);
	return(0);
#undef iQ_a
#undef iQ_b
}

int _increasing_quotes(const void *a,const void *b)
{
#define iQ_a ((struct InstantDEX_quote *)a)
#define iQ_b ((struct InstantDEX_quote *)b)
    double vala,valb,volume;
    vala = iQ_a->isask == 0 ? calc_price_volume(&volume,iQ_a->baseamount,iQ_a->relamount) : calc_price_volume(&volume,iQ_a->relamount,iQ_a->baseamount);
    valb = iQ_b->isask == 0 ? calc_price_volume(&volume,iQ_b->baseamount,iQ_b->relamount) : calc_price_volume(&volume,iQ_b->relamount,iQ_b->baseamount);
    // printf("(%f %f) ",vala,valb);
	if ( valb > vala )
		return(-1);
	else if ( valb < vala )
		return(1);
	return(0);
#undef iQ_a
#undef iQ_b
}

int32_t iQcmp(struct InstantDEX_quote *iQA,struct InstantDEX_quote *iQB)
{
    if ( iQA->isask == iQB->isask && iQA->baseid == iQB->baseid && iQA->relid == iQB->relid && iQA->baseamount == iQB->baseamount && iQA->relamount == iQB->relamount )
        return(0);
    else if ( iQA->isask != iQB->isask && iQA->baseid == iQB->relid && iQA->relid == iQB->baseid && iQA->baseamount == iQB->relamount && iQA->relamount == iQB->baseamount )
        return(0);
    return(-1);
}

int32_t iQ_exchangestr(char *exchange,struct InstantDEX_quote *iQ)
{
    if ( iQ->baseiQ != 0 && iQ->reliQ != 0 )
        sprintf(exchange,"%s_%s",Exchanges[(int32_t)iQ->baseiQ->exchangeid].name,Exchanges[(int32_t)iQ->reliQ->exchangeid].name);
    else if ( iQ->exchangeid == INSTANTDEX_NXTAEID && iQ->timestamp > time(NULL) )
        strcpy(exchange,INSTANTDEX_NXTAEUNCONF);
    else strcpy(exchange,Exchanges[(int32_t)iQ->exchangeid].name);
    return(0);
}

void disp_quote(void *ptr,int32_t arg,struct InstantDEX_quote *iQ)
{
    double price,vol;
    price = calc_price_volume(&vol,iQ->baseamount,iQ->relamount);
    printf("%u: arg.%d %-6ld %12.8f %12.8f %llu/%llu\n",iQ->timestamp,arg,iQ->timestamp-time(NULL),price,vol,(long long)iQ->baseamount,(long long)iQ->relamount);
}

int32_t create_InstantDEX_quote(struct InstantDEX_quote *iQ,uint32_t timestamp,int32_t isask,uint64_t quoteid,double price,double volume,uint64_t baseid,uint64_t baseamount,uint64_t relid,uint64_t relamount,char *exchange,uint64_t nxt64bits,char *gui,struct InstantDEX_quote *baseiQ,struct InstantDEX_quote *reliQ)
{
    struct exchange_info *xchg;
    memset(iQ,0,sizeof(*iQ));
    if ( baseamount == 0 && relamount == 0 )
        set_best_amounts(&baseamount,&relamount,price,volume);
    iQ->timestamp = timestamp;
    iQ->isask = isask;
    iQ->nxt64bits = nxt64bits;
    iQ->baseiQ = baseiQ;
    iQ->reliQ = reliQ;
    iQ->baseid = baseid, iQ->baseamount = baseamount;
    iQ->relid = relid, iQ->relamount = relamount;
    //printf("(%s) %f %f\n",exchange,dstr(baseamount),dstr(relamount));
    strncpy(iQ->gui,gui,sizeof(iQ->gui)-1);
    if ( baseiQ == 0 && reliQ == 0 )
    {
        if ( (xchg= find_exchange(exchange,1)) != 0 )
            iQ->exchangeid = xchg->exchangeid;
        else printf("cant find_exchange(%s)??\n",exchange);
    }
    else iQ->exchangeid = INSTANTDEX_EXCHANGEID;
    if ( (iQ->quoteid= quoteid) == 0 )
        iQ->quoteid = calc_quoteid(iQ);
    return(0);
}

cJSON *gen_InstantDEX_json(uint64_t *baseamountp,uint64_t *relamountp,int32_t depth,int32_t flip,struct InstantDEX_quote *iQ,uint64_t refbaseid,uint64_t refrelid,uint64_t jumpasset)
{
    cJSON *json = cJSON_CreateObject();
    char numstr[64],base[64],rel[64],exchange[64];
    uint64_t baseamount,relamount,frombase,fromrel,tobase,torel;
    double price,volume,ratio;
    cJSON *relobj=0,*baseobj=0;
    struct InstantDEX_quote *baseiQ,*reliQ;
    uint64_t mult;
    if ( iQ->isask == 0 )
        baseamount = iQ->baseamount, relamount = iQ->relamount;
    else relamount = iQ->baseamount, baseamount = iQ->relamount;
    baseiQ = iQ->baseiQ, reliQ = iQ->reliQ;
    if ( depth == 0 )
        *baseamountp = baseamount, *relamountp = relamount;
    if ( baseiQ != 0 && reliQ != 0 )
    {
        if ( baseiQ->isask == 0 )
            frombase = baseiQ->baseamount, fromrel = baseiQ->relamount;
        else fromrel = baseiQ->baseamount, frombase = baseiQ->relamount;
        if ( reliQ->isask == 0 )
            tobase = reliQ->baseamount, torel = reliQ->relamount;
        else torel = reliQ->baseamount, tobase = reliQ->relamount;
        make_jumpquote(baseamountp,relamountp,&frombase,&fromrel,&tobase,&torel);
    } else frombase = fromrel = tobase = torel = 0;
    if ( Debuglevel > 2 )
        printf("%p depth.%d %p %p %.8f %.8f: %.8f %.8f %.8f %.8f\n",iQ,depth,baseiQ,reliQ,dstr(*baseamountp),dstr(*relamountp),dstr(frombase),dstr(fromrel),dstr(tobase),dstr(torel));
    if ( depth == 0 )
    {
        cJSON_AddItemToObject(json,"requestType",cJSON_CreateString("makeoffer3"));
        //cJSON_AddItemToObject(json,"flip",cJSON_CreateNumber(flip));
        if ( iQ->isask != 0 ) //iQ->askoffer != 0 ||
            cJSON_AddItemToObject(json,"askoffer",cJSON_CreateNumber(1));
        set_assetname(&mult,base,refbaseid), cJSON_AddItemToObject(json,"base",cJSON_CreateString(base));
        set_assetname(&mult,rel,refrelid), cJSON_AddItemToObject(json,"rel",cJSON_CreateString(rel));
        
        cJSON_AddItemToObject(json,"timestamp",cJSON_CreateNumber(iQ->timestamp));
        cJSON_AddItemToObject(json,"age",cJSON_CreateNumber((uint32_t)time(NULL) - iQ->timestamp));
        if ( iQ->matched != 0 )
            cJSON_AddItemToObject(json,"matched",cJSON_CreateNumber(1));
        if ( iQ->sent != 0 )
            cJSON_AddItemToObject(json,"sent",cJSON_CreateNumber(1));
        if ( iQ->closed != 0 )
            cJSON_AddItemToObject(json,"closed",cJSON_CreateNumber(1));
        iQ_exchangestr(exchange,iQ), cJSON_AddItemToObject(json,"exchange",cJSON_CreateString(exchange));
        if ( iQ->nxt64bits != 0 )
            sprintf(numstr,"%llu",(long long)iQ->nxt64bits), cJSON_AddItemToObject(json,"offerNXT",cJSON_CreateString(numstr));
        sprintf(numstr,"%llu",(long long)refbaseid), cJSON_AddItemToObject(json,"baseid",cJSON_CreateString(numstr));
        sprintf(numstr,"%llu",(long long)refrelid), cJSON_AddItemToObject(json,"relid",cJSON_CreateString(numstr));
        if ( jumpasset != 0 )
            sprintf(numstr,"%llu",(long long)jumpasset), cJSON_AddItemToObject(json,"jumpasset",cJSON_CreateString(numstr));
        if ( iQ->baseiQ != 0 && iQ->reliQ != 0 )
        {
            baseamount = frombase, relamount = fromrel;
            baseobj = gen_InstantDEX_json(&baseamount,&relamount,depth+1,0,iQ->baseiQ,refbaseid,jumpasset,0);
            *baseamountp = baseamount;
            if ( (ratio= check_ratios(baseamount,relamount,frombase,fromrel)) < .999 || ratio > 1.001 )
                printf("WARNING: baseiQ ratio %f (%llu/%llu) -> (%llu/%llu)\n",ratio,(long long)baseamount,(long long)relamount,(long long)frombase,(long long)fromrel);
            baseamount = tobase, relamount = torel;
            relobj = gen_InstantDEX_json(&baseamount,&relamount,depth+1,0,iQ->reliQ,refrelid,jumpasset,0);
            *relamountp = baseamount;
            if ( (ratio= check_ratios(baseamount,relamount,tobase,torel)) < .999 || ratio > 1.001 )
                printf("WARNING: reliQ ratio %f (%llu/%llu) -> (%llu/%llu)\n",ratio,(long long)baseamount,(long long)relamount,(long long)tobase,(long long)torel);
        }
        price = calc_price_volume(&volume,*baseamountp,*relamountp);
        cJSON_AddItemToObject(json,"price",cJSON_CreateNumber(price));
        cJSON_AddItemToObject(json,"volume",cJSON_CreateNumber(volume));
        sprintf(numstr,"%llu",(long long)*baseamountp), cJSON_AddItemToObject(json,"baseamount",cJSON_CreateString(numstr));
        sprintf(numstr,"%llu",(long long)*relamountp), cJSON_AddItemToObject(json,"relamount",cJSON_CreateString(numstr));
        sprintf(numstr,"%llu",(long long)calc_quoteid(iQ)), cJSON_AddItemToObject(json,"quoteid",cJSON_CreateString(numstr));
        if ( iQ->gui[0] != 0 )
            cJSON_AddItemToObject(json,"gui",cJSON_CreateString(iQ->gui));
        if ( baseobj != 0 )
            cJSON_AddItemToObject(json,"baseiQ",baseobj);
        if ( relobj != 0 )
            cJSON_AddItemToObject(json,"reliQ",relobj);
        cJSON_AddItemToObject(json,"minperc",cJSON_CreateNumber(iQ->minperc));
    }
    else
    {
        price = calc_price_volume(&volume,*baseamountp,*relamountp);
        iQ_exchangestr(exchange,iQ);
        cJSON_AddItemToObject(json,"exchange",cJSON_CreateString(exchange));
        if ( iQ->nxt64bits != 0 )
            sprintf(numstr,"%llu",(long long)iQ->nxt64bits), cJSON_AddItemToObject(json,"offerNXT",cJSON_CreateString(numstr));
        sprintf(numstr,"%llu",(long long)calc_quoteid(iQ)), cJSON_AddItemToObject(json,"quoteid",cJSON_CreateString(numstr));
        sprintf(numstr,"%llu",(long long)*baseamountp), cJSON_AddItemToObject(json,"baseamount",cJSON_CreateString(numstr));
        sprintf(numstr,"%llu",(long long)*relamountp), cJSON_AddItemToObject(json,"relamount",cJSON_CreateString(numstr));
    }
    if ( *baseamountp < min_asset_amount(refbaseid) || *relamountp < min_asset_amount(refrelid) )
    {
        if ( Debuglevel > 2 )
            printf("%.8f < %.8f || rel %.8f < %.8f\n",dstr(*baseamountp),dstr(min_asset_amount(refbaseid)),dstr(*relamountp),dstr(min_asset_amount(refrelid)));
        if ( *baseamountp < min_asset_amount(refbaseid) )
            sprintf(numstr,"%llu",(long long)min_asset_amount(refbaseid)), cJSON_AddItemToObject(json,"minbase_error",cJSON_CreateString(numstr));
       if ( *relamountp < min_asset_amount(refrelid) )
           sprintf(numstr,"%llu",(long long)min_asset_amount(refrelid)), cJSON_AddItemToObject(json,"minrel_error",cJSON_CreateString(numstr));
    }
    return(json);
}

cJSON *gen_orderbook_item(struct InstantDEX_quote *iQ,int32_t allflag,uint64_t baseid,uint64_t relid,uint64_t jumpasset)
{
    char offerstr[MAX_JSON_FIELD];
    uint64_t baseamount=0,relamount=0;
    double price,volume;
    cJSON *json = 0;
    baseamount = iQ->baseamount, relamount = iQ->relamount;
    if ( (iQ->isask == 0 && (baseid != iQ->baseid || relid != iQ->relid)) || (iQ->isask != 0 && (baseid != iQ->relid || relid != iQ->baseid)) )
        printf("gen_orderbook_item: isask.%d %llu/%llu != %llu/%llu\n",iQ->isask,(long long)iQ->baseid,(long long)iQ->relid,(long long)baseid,(long long)relid), getchar();
    if ( (json= gen_InstantDEX_json(&baseamount,&relamount,0,iQ->isask,iQ,baseid,relid,jumpasset)) != 0 )
    {
        if ( cJSON_GetObjectItem(json,"minbase_error") != 0 || cJSON_GetObjectItem(json,"minrel_error") != 0 )
        {
            //printf("gen_orderbook_item has error\n");
            free_json(json);
            return(0);
        }
        if ( allflag == 0 )
        {
            price = calc_price_volume(&volume,baseamount,relamount);
            sprintf(offerstr,"{\"price\":\"%.8f\",\"volume\":\"%.8f\"}",price,volume);
            free_json(json);
            return(cJSON_Parse(offerstr));
        }
    }
    return(json);
}

int32_t make_jumpiQ(uint64_t refbaseid,uint64_t refrelid,int32_t flip,struct InstantDEX_quote *iQ,struct InstantDEX_quote *fromiQ,struct InstantDEX_quote *toiQ,char *gui)
{
    uint64_t baseamount,relamount,frombase,fromrel,tobase,torel;
    double vol;
    char exchange[64];
    uint32_t timestamp;
    if ( fromiQ->isask == 0 )
        frombase = fromiQ->baseamount, fromrel = fromiQ->relamount;
    else fromrel = fromiQ->baseamount, frombase = fromiQ->relamount;
    if ( toiQ->isask == 0 )
        tobase = toiQ->baseamount, torel = toiQ->relamount;
    else torel = toiQ->baseamount, tobase = toiQ->relamount;
    make_jumpquote(&baseamount,&relamount,&frombase,&fromrel,&tobase,&torel);
    if ( (timestamp= toiQ->timestamp) > fromiQ->timestamp )
        timestamp = fromiQ->timestamp;
    iQ_exchangestr(exchange,iQ);
    create_InstantDEX_quote(iQ,timestamp,0,calc_quoteid(fromiQ) ^ calc_quoteid(toiQ),0.,0.,refbaseid,baseamount,refrelid,relamount,exchange,0,gui,fromiQ,toiQ);
    if ( Debuglevel > 2 )
        printf("%p jumpASK: %f (%llu/%llu) %llu %llu %llu %llu\n",iQ,calc_price_volume(&vol,iQ->baseamount,iQ->relamount),(long long)baseamount,(long long)relamount,(long long)frombase,(long long)fromrel,(long long)tobase,(long long)torel);
    iQ->isask = flip;
    //iQ->askoffer = flip;
    return(1);
}

#endif
