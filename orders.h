//
//  orders.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_orders_h
#define xcode_orders_h

#define _ASKMASK (1L << 0)
#define _FLIPMASK (1L << 1)
#define _TYPEMASK (~_ASKMASK)
#define _obookid(baseid,relid) ((baseid) ^ (relid))
#define _iQ_dir(iQ) ((((iQ)->type) & _ASKMASK) ? -1 : 1)
#define _iQ_type(iQ) ((iQ)->type & _TYPEMASK)
#define _iQ_price(iQ) ((double)(iQ)->relamount / (iQ)->baseamount)
#define _iQ_volume(iQ) ((double)(iQ)->baseamount / SATOSHIDEN)

void flip_iQ(struct InstantDEX_quote *iQ)
{
    uint64_t amount;
    iQ->type ^= (_ASKMASK | _FLIPMASK);
    amount = iQ->baseamount;
    iQ->baseamount = iQ->relamount;
    iQ->relamount = amount;
}

int32_t create_orderbook_tx(int32_t polarity,struct orderbook_tx *tx,int32_t type,uint64_t nxt64bits,uint64_t baseid,uint64_t relid,double price,double volume)
{
    uint64_t baseamount,relamount;
    baseamount = volume * SATOSHIDEN;
    relamount = (price * baseamount);
    memset(tx,0,sizeof(*tx));
    tx->iQ.timestamp = (uint32_t)time(NULL);
    tx->iQ.type = type;
    if ( polarity < 0 )
        tx->iQ.type |= _ASKMASK;
    if ( baseid > relid )
        tx->iQ.type |= _FLIPMASK;
    tx->iQ.nxt64bits = nxt64bits;
    tx->baseid = baseid;
    tx->relid = relid;
    tx->iQ.baseamount = baseamount;
    tx->iQ.relamount = relamount;
    return(0);
}

void free_orderbook(struct orderbook *op)
{
    if ( op != 0 )
    {
        if ( op->bids != 0 )
            free(op->bids);
        if ( op->asks != 0 )
            free(op->asks);
        free(op);
    }
}

cJSON *gen_orderbook_item(struct InstantDEX_quote *iQ,int32_t allflag)
{
    char NXTaddr[64],numstr[64];
    cJSON *array = 0;
    if ( iQ != 0 )
    {
        if ( iQ->baseamount != 0 && iQ->relamount != 0 )
        {
            array = cJSON_CreateArray();
            sprintf(numstr,"%.11f",_iQ_price(iQ));
            cJSON_AddItemToArray(array,cJSON_CreateString(numstr));
            sprintf(numstr,"%.8f",_iQ_volume(iQ));
            cJSON_AddItemToArray(array,cJSON_CreateString(numstr));
            if ( allflag != 0 )
            {
                cJSON_AddItemToArray(array,cJSON_CreateNumber(iQ->type & _TYPEMASK));
                expand_nxt64bits(NXTaddr,iQ->nxt64bits);
                cJSON_AddItemToArray(array,cJSON_CreateString(NXTaddr));
            }
        }
    }
    return(array);
}

cJSON *gen_InstantDEX_json(struct InstantDEX_quote *iQ,uint64_t refbaseid,uint64_t refrelid)
{
    cJSON *json = cJSON_CreateObject();
    char numstr[64];
    cJSON_AddItemToObject(json,"time",cJSON_CreateNumber(iQ->timestamp));
    cJSON_AddItemToObject(json,"type",cJSON_CreateNumber(_iQ_type(iQ)));
    sprintf(numstr,"%llu",(long long)iQ->nxt64bits), cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)refbaseid), cJSON_AddItemToObject(json,"base",cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)iQ->baseamount), cJSON_AddItemToObject(json,"baseamount",cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)refrelid), cJSON_AddItemToObject(json,"rel",cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)iQ->relamount), cJSON_AddItemToObject(json,"relamount",cJSON_CreateString(numstr));
    cJSON_AddItemToObject(json,"requestType",cJSON_CreateString((_iQ_dir(iQ) > 0) ? "bid" : "ask"));
    return(json);
}

double parse_InstantDEX_json(uint64_t *baseidp,uint64_t *relidp,struct InstantDEX_quote *iQ,cJSON *json)
{
    char basestr[MAX_JSON_FIELD],relstr[MAX_JSON_FIELD],nxtstr[MAX_JSON_FIELD],cmd[MAX_JSON_FIELD];
    struct orderbook_tx T;
    uint64_t nxt64bits,baseamount,relamount;
    double price,volume;
    int32_t polarity;
    uint32_t type;
    //({"requestType":"[bid|ask]","type":0,"NXT":"13434315136155299987","base":"4551058913252105307","srcvol":"1.01000000","rel":"11060861818140490423","destvol":"0.00606000"}) 0x7f24700111c0
    *baseidp = *relidp = 0;
    memset(iQ,0,sizeof(*iQ));
    if ( json != 0 )
    {
        copy_cJSON(basestr,cJSON_GetObjectItem(json,"base")), *baseidp = calc_nxt64bits(basestr);
        copy_cJSON(relstr,cJSON_GetObjectItem(json,"rel")), *relidp = calc_nxt64bits(relstr);
        copy_cJSON(basestr,cJSON_GetObjectItem(json,"baseamount")), baseamount = calc_nxt64bits(basestr);
        copy_cJSON(relstr,cJSON_GetObjectItem(json,"relamount")), relamount = calc_nxt64bits(relstr);
        copy_cJSON(cmd,cJSON_GetObjectItem(json,"requestType"));
        type = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"type"),0);
        if ( strcmp(cmd,"ask") == 0 )
        {
            polarity = -1;
            type |= _ASKMASK;
        } else polarity = 1;
        if ( *baseidp != 0 && *relidp != 0 )
        {
            if ( relamount != 0 && baseamount != 0 )
            {
                price = ((double)relamount / baseamount);
                volume = ((double)baseamount / SATOSHIDEN);
                copy_cJSON(nxtstr,cJSON_GetObjectItem(json,"NXT")), nxt64bits = calc_nxt64bits(nxtstr);
                printf("conv_InstantDEX_json: obookid.%llu base %.8f -> rel %.8f price %f vol %f\n",(long long)(*baseidp ^ *relidp),dstr(baseamount),dstr(relamount),price,volume);
                create_orderbook_tx(polarity,&T,type,nxt64bits,*baseidp,*relidp,price,volume);
                T.iQ.timestamp = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"time"),0);
                *iQ = T.iQ;
            }
        }
    }
    return(_iQ_price(iQ));
}

int _decreasing_quotes(const void *a,const void *b)
{
#define iQ_a ((struct InstantDEX_quote *)a)
#define iQ_b ((struct InstantDEX_quote *)b)
    double vala,valb;
    vala = _iQ_price(iQ_a);
    valb = _iQ_price(iQ_b);
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
    double vala,valb;
    vala = _iQ_price(iQ_a);
    valb = _iQ_price(iQ_b);
	if ( valb > vala )
		return(-1);
	else if ( valb < vala )
		return(1);
	return(0);
#undef iQ_a
#undef iQ_b
}

void update_orderbook(int32_t iter,struct orderbook *op,int32_t *numbidsp,int32_t *numasksp,struct InstantDEX_quote *iQ)
{
    if ( iter == 0 )
    {
        if ( _iQ_dir(iQ) > 0 )
            op->numbids++;
        else op->numasks++;
    }
    else
    {
        if ( _iQ_dir(iQ) > 0 )
            op->bids[(*numbidsp)++] = *iQ;
        else op->asks[(*numasksp)++] = *iQ;
    }
}

struct orderbook *create_orderbook(uint64_t refbaseid,uint64_t refrelid,struct orderbook_tx **feedorders,int32_t numfeeds)
{
    struct orderbook_tx T;
    int32_t i,iter,numbids,numasks,refflipped,flipped;
    size_t retdlen = 0;
    char obookstr[64];
    struct orderbook *op = 0;
    void *retdata,*p;
    DBT *origdata,*data = 0;
    expand_nxt64bits(obookstr,refbaseid ^ refrelid);
    op = (struct orderbook *)calloc(1,sizeof(*op));
    op->baseid = refbaseid;
    op->relid = refrelid;
    if ( refbaseid < refrelid ) refflipped = 0;
    else refflipped = _FLIPMASK;
    origdata = (DBT *)find_storage(INSTANTDEX_DATA,obookstr,65536);
    for (iter=numbids=numasks=0; iter<2; iter++)
    {
        if ( numfeeds > 0 && feedorders != 0 )
        {
            for (i=0; i<numfeeds; i++)
            {
                T = *feedorders[i];
                if ( T.baseid < T.relid ) flipped = 0;
                else flipped = _FLIPMASK;
                if ( flipped != refflipped )
                    flip_iQ(&T.iQ);
                if ( (T.iQ.type & _FLIPMASK) != refflipped )
                    flip_iQ(&T.iQ);
                update_orderbook(iter,op,&numbids,&numasks,&T.iQ);
            }
        }
        if ( (data= origdata) != 0 )
        {
            for (DB_MULTIPLE_INIT(p,data); ;)
            {
                DB_MULTIPLE_NEXT(p,data,retdata,retdlen);
                if ( p == NULL )
                    break;
                T.iQ = *(struct InstantDEX_quote *)retdata;
                if ( (T.iQ.type & _FLIPMASK) != refflipped )
                    flip_iQ(&T.iQ);
                T.baseid = refbaseid;
                T.relid = refrelid;
                if ( iter == 0 )
                {
                    for (i=0; i<retdlen; i++)
                        printf("%02x ",((uint8_t *)retdata)[i]);
                    printf("%p %p: %d\n",p,retdata,(int)retdlen);
                    printf("Q: %llu -> %llu NXT.%llu %u type.%d\n",(long long)T.iQ.baseamount,(long long)T.iQ.relamount,(long long)T.iQ.nxt64bits,T.iQ.timestamp,T.iQ.type);
                }
                update_orderbook(iter,op,&numbids,&numasks,&T.iQ);
            }
        }
        if ( iter == 0 )
        {
            if ( op->numbids > 0 )
                op->bids = (struct InstantDEX_quote *)calloc(op->numbids,sizeof(*op->bids));
            if ( op->numasks > 0 )
                op->asks = (struct InstantDEX_quote *)calloc(op->numasks,sizeof(*op->asks));
        }
        else
        {
            if ( op->numbids > 0 || op->numasks > 0 )
            {
                if ( op->numbids > 0 )
                    qsort(op->bids,op->numbids,sizeof(*op->bids),_decreasing_quotes);
                if ( op->numasks > 0 )
                    qsort(op->asks,op->numasks,sizeof(*op->asks),_increasing_quotes);
            }
            else free(op), op = 0;
        }
    }
    //printf("(%f %f %llu %u)\n",quotes->price,quotes->vol,(long long)quotes->nxt64bits,quotes->timestamp);
    if ( origdata != 0 )
    {
        if ( origdata->data != 0 )
            free(origdata->data);
        free(origdata);
    }
    return(op);
}

#undef _ASKMASK
#undef _TYPEMASK
#undef _obookid
#undef _iQ_dir
#undef _iQ_type
#undef _iQ_price
#undef _iQ_volume

#endif
