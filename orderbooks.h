//
//  orderbooks.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_orderbooks_h
#define xcode_orderbooks_h

struct orderbook
{
    uint64_t baseid,relid,jumpasset;
    char base[16],rel[16],jumper[16];
    struct InstantDEX_quote *bids,*asks;
    int32_t numbids,numasks;
};

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

void update_orderbook(int32_t iter,struct orderbook *op,int32_t *numbidsp,int32_t *numasksp,struct InstantDEX_quote *iQ,int32_t polarity,char *gui)
{
    struct InstantDEX_quote *ask,*bid;
    if ( iter == 0 )
    {
        if ( polarity > 0 )//&& iQ->isask == 0) || (polarity < 0 && iQ->isask != 0) )
            op->numbids++;
        else op->numasks++;
    }
    else
    {   double p,v;
        if ( polarity > 0 )//&& iQ->isask == 0) || (polarity < 0 && iQ->isask != 0) )
        {
            bid = &op->bids[(*numbidsp)++];
            *bid = *iQ;
            bid->isask = 0;
            //if ( quoteid != 0 )
            //    bid->quoteid = quoteid;
            if ( Debuglevel > 2 )
                p = calc_price_volume(&v,iQ->baseamount,iQ->relamount), printf("B.(%f %f) ",p,v);
        }
        else
        {
            ask = &op->asks[(*numasksp)++];
            *ask = *iQ;
            ask->isask = 1;
            //if ( quoteid != 0 )
            //    ask->quoteid = quoteid;
            //ask->baseamount = iQ->relamount;
            //ask->relamount = iQ->baseamount;
            if ( Debuglevel > 2 )
                p = calc_price_volume(&v,ask->baseamount,ask->relamount), printf("A.(%f %f) ",1./p,p*v);
        }
    }
}

void add_to_orderbook(struct orderbook *op,int32_t iter,int32_t *numbidsp,int32_t *numasksp,struct rambook_info *rb,struct InstantDEX_quote *iQ,int32_t polarity,int32_t oldest,char *gui)
{
    if ( iQ->timestamp >= oldest )
        update_orderbook(iter,op,numbidsp,numasksp,iQ,polarity,gui);
}

void sort_orderbook(struct orderbook *op)
{
    if ( op != 0 && (op->numbids > 0 || op->numasks > 0) )
    {
        if ( op->numbids > 0 )
            qsort(op->bids,op->numbids,sizeof(*op->bids),_decreasing_quotes);
        if ( op->numasks > 0 )
            qsort(op->asks,op->numasks,sizeof(*op->asks),_increasing_quotes);
    }
}

struct orderbook *merge_books(char *base,char *rel,struct orderbook *books[],int32_t n)
{
    struct orderbook *op = 0;
    int32_t i,numbids,numasks;
    for (i=numbids=0; i<n; i++)
        numbids += books[i]->numbids;
    for (i=numasks=0; i<n; i++)
        numasks += books[i]->numasks;
    op = (struct orderbook *)calloc(1,sizeof(*op));
    strcpy(op->base,base), strcpy(op->rel,rel), strcpy(op->jumper,"merged");
    op->baseid = stringbits(base);
    op->relid = stringbits(rel);
    if ( (op->numbids= numbids) > 0 )
    {
        op->bids = (struct InstantDEX_quote *)calloc(op->numbids,sizeof(*op->bids));
        for (i=numbids=0; i<n; i++)
            if ( books[i]->numbids != 0 )
                memcpy(&op->bids[numbids],books[i]->bids,books[i]->numbids * sizeof(*op->bids)), numbids += books[i]->numbids;
    }
    if ( (op->numasks= numasks) > 0 )
    {
        op->asks = (struct InstantDEX_quote *)calloc(op->numasks,sizeof(*op->asks));
        for (i=numasks=0; i<n; i++)
            if ( books[i]->numasks != 0 )
                memcpy(&op->asks[numasks],books[i]->asks,books[i]->numasks * sizeof(*op->asks)), numasks += books[i]->numasks;
    }
    sort_orderbook(op);
    return(op);
}

void debug_json(cJSON *item)
{
    char *str,*str2;
    if ( Debuglevel > 1 )
    {
        str = cJSON_Print(item);
        stripwhite_ns(str,strlen(str));
        str2 = stringifyM(str);
        printf("%s\n",str2);
        free(str), free(str2);
    }
}

int32_t nonz_and_lesser(int32_t a,int32_t b)
{
    if ( a > 0 && b > 0 )
    {
        if ( b < a )
            a = b;
        return(a);
    }
    return(0);
}

struct orderbook *make_jumpbook(char *base,uint64_t baseid,char *jumper,char *rel,uint64_t relid,struct orderbook *to,struct orderbook *from,char *gui,struct orderbook *rawop,int32_t m)
{
    struct orderbook *op = 0;
    int32_t i,j,n,numbids,numasks;
    if ( to != 0 && from != 0 )
    {
        numbids = nonz_and_lesser(to->numasks,from->numbids);
        numasks = nonz_and_lesser(to->numbids,from->numasks);
        if ( (numbids + numasks) > 0 || (rawop != 0 && (rawop->numbids + rawop->numasks) > 0) )
        {
            op = (struct orderbook *)calloc(1,sizeof(*op));
            strcpy(op->base,base), strcpy(op->rel,rel), strcpy(op->jumper,jumper);
            op->baseid = baseid;
            op->relid = relid;
            if ( strcmp(jumper,"NXT") == 0 )
                op->jumpasset = NXT_ASSETID;
            else op->jumpasset = stringbits(jumper);
            if ( (op->numbids= (to->numasks*from->numbids)+(rawop==0?0:rawop->numbids)) > 0 )
            {
                printf("(%llu %llu, %llu %llu): ",(long long)to->baseid,(long long)to->relid,(long long)from->baseid,(long long)from->relid);
                op->bids = (struct InstantDEX_quote *)calloc(op->numbids,sizeof(*op->bids));
                n = 0;
                if ( to->numasks > 0 && from->numbids > 0 )
                {
                    for (i=0; i<to->numasks&&i<m; i++)
                        for (j=0; j<from->numbids&&j<m; j++)
                            make_jumpiQ(baseid,relid,0,&op->bids[n++],&from->bids[j],&to->asks[i],gui);
                }
                if ( rawop != 0 && rawop->numbids > 0 )
                    for (i=0; i<rawop->numbids; i++)
                        op->bids[n++] = rawop->bids[i];
                op->numbids = n;
            }
            if ( (op->numasks= (from->numasks*to->numbids)+(rawop==0?0:rawop->numasks)) > 0 )
            {
                printf("(%llu %llu, %llu %llu): ",(long long)from->baseid,(long long)from->relid,(long long)to->baseid,(long long)to->relid);
                op->asks = (struct InstantDEX_quote *)calloc(op->numasks,sizeof(*op->asks));
                n = 0;
                if ( from->numasks > 0 && to->numbids > 0 )
                {
                    for (i=0; i<from->numasks&&i<m; i++)
                        for (j=0; j<to->numbids&&j<m; j++)
                            make_jumpiQ(baseid,relid,1,&op->asks[n++],&from->asks[i],&to->bids[j],gui);
                }
                if ( rawop != 0 && rawop->numasks > 0 )
                    for (i=0; i<rawop->numasks; i++)
                        op->asks[n++] = rawop->asks[i];
                op->numasks = n;
            }
        }
    }
    sort_orderbook(op);
    return(op);
}

uint64_t _obookid(uint64_t baseid,uint64_t relid) { return(baseid ^ relid); }

struct orderbook *create_orderbook(char *base,uint64_t refbaseid,char *rel,uint64_t refrelid,uint32_t oldest,char *gui)
{
    int32_t i,j,iter,numbids,numasks,numbooks,polarity,haveexchanges = 0;
    char obookstr[64];
    uint32_t basetype,reltype;
    struct rambook_info **obooks,*rb;
    struct orderbook *op = 0;
    uint64_t basemult,relmult;
    printf("create_orderbook %llu/%llu\n",(long long)refbaseid,(long long)refrelid);
    if ( (refbaseid != 0 && refbaseid == refrelid) || (base != 0 && rel != 0 && base[0] != 0 && strcmp(base,rel) == 0) )
        return(0);
    expand_nxt64bits(obookstr,_obookid(refbaseid,refrelid));
    op = (struct orderbook *)calloc(1,sizeof(*op));
    if ( refbaseid != refrelid && base != 0 && base[0] != 0 && rel != 0 && rel[0] != 0 )
    {
        strcpy(op->base,base), strcpy(op->rel,rel);
        op->baseid = stringbits(base);
        op->relid = stringbits(rel);
        basetype = reltype = INSTANTDEX_UNKNOWN;
    }
    else
    {
        basetype = set_assetname(&basemult,op->base,refbaseid);
        reltype = set_assetname(&relmult,op->rel,refrelid);
        op->baseid = refbaseid;
        op->relid = refrelid;
    }
    printf("create_orderbook %s/%s\n",op->base,op->rel);
    basetype &= 0xffff, reltype &= 0xffff;
    for (iter=0; iter<2; iter++)
    {
        numbids = numasks = 0;
        if ( (obooks= get_allrambooks(&numbooks)) != 0 )
        {
            if ( Debuglevel > 2 )
                printf("got %d rambooks: oldest.%u\n",numbooks,oldest);
            for (i=0; i<numbooks; i++)
            {
                rb = obooks[i];
                if ( strcmp(rb->exchange,INSTANTDEX_NAME) != 0 )
                    haveexchanges++;
                if ( Debuglevel > 2 )
                    printf("[%d] numquotes.%d: (%s).%llu (%s).%llu | (%s).%llu (%s).%llu\n",i,rb->numquotes,rb->base,(long long)rb->assetids[0],rb->rel,(long long)rb->assetids[1],op->base,(long long)op->baseid,op->rel,(long long)op->relid);
                if ( rb->numquotes == 0 )
                    continue;
                if ( (rb->assetids[0] == refbaseid && rb->assetids[1] == refrelid) || (strcmp(op->base,rb->base) == 0 && strcmp(op->rel,rb->rel) == 0) )
                    polarity = 1;
                else if ( (rb->assetids[1] == refbaseid && rb->assetids[0] == refrelid) || (strcmp(op->base,rb->rel) == 0 && strcmp(op->rel,rb->base) == 0)  )
                    polarity = -1;
                else continue;
                //printf("[%d] numquotes.%d: %llu %llu | oldest.%u\n",i,rb->numquotes,(long long)rb->assetids[0],(long long)rb->assetids[1],oldest);
                for (j=0; j<rb->numquotes; j++)
                    add_to_orderbook(op,iter,&numbids,&numasks,rb,&rb->quotes[j],polarity,oldest,gui);
            }
            free(obooks);
        }
        if ( iter == 0 )
        {
            if ( op->numbids > 0 )
                op->bids = (struct InstantDEX_quote *)calloc(op->numbids,sizeof(*op->bids));
            if ( op->numasks > 0 )
                op->asks = (struct InstantDEX_quote *)calloc(op->numasks,sizeof(*op->asks));
        } else sort_orderbook(op);
    }
    //printf("(%f %f %llu %u)\n",quotes->price,quotes->vol,(long long)quotes->nxt64bits,quotes->timestamp);
    if ( op != 0 && (op->numbids + op->numasks) == 0 )
        free_orderbook(op), op = 0;
    return(op);
}

char *orderbook_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    int32_t i,allflag,maxdepth;
    uint32_t oldest;
    struct InstantDEX_quote *iQ = 0;
    uint64_t baseid,relid,nxt64bits = calc_nxt64bits(NXTaddr);
    struct exchange_info *exchange;
    struct orderpair *pair;
    cJSON *json,*bids,*asks,*item;
    struct orderbook *op=0,*toNXT=0,*fromNXT=0,*rawop=0;
    char obook[64],buf[MAX_JSON_FIELD],baserel[1024],gui[MAX_JSON_FIELD],base[MAX_JSON_FIELD],rel[MAX_JSON_FIELD],assetA[64],assetB[64],*retstr = 0;
    baseid = get_API_nxt64bits(objs[0]), relid = get_API_nxt64bits(objs[1]), allflag = get_API_int(objs[2],0), oldest = get_API_int(objs[3],0);
    maxdepth = get_API_int(objs[4],13), copy_cJSON(base,objs[5]), copy_cJSON(rel,objs[6]), copy_cJSON(gui,objs[7]), gui[sizeof(iQ->gui)-1] = 0;
    expand_nxt64bits(obook,_obookid(baseid,relid));
    sprintf(buf,"{\"baseid\":\"%llu\",\"relid\":\"%llu\",\"oldest\":%u}",(long long)baseid,(long long)relid,oldest);
    retstr = 0;
    if ( baseid != relid && ((baseid != 0 && relid != 0) || (base[0] != 0 && rel[0] != 0)) )
    {
        retstr = clonestr("{\"error\":\"no bids or asks\"}");
        expand_nxt64bits(assetA,baseid);
        expand_nxt64bits(assetB,relid);
        toNXT = fromNXT = 0;
        ensure_rambook(baseid,relid);
        if ( (exchange= find_exchange(INSTANTDEX_NXTAENAME,0)) != 0 )
        {
            for (i=0; i<exchange->num; i++)
            {
                pair = &exchange->orderpairs[i];
                if ( pair->bids->assetids[0] == baseid || pair->bids->assetids[0] == relid || pair->bids->assetids[1] == baseid || pair->bids->assetids[1] == relid || pair->asks->assetids[0] == baseid || pair->asks->assetids[0] == relid || pair->asks->assetids[1] == baseid || pair->asks->assetids[1] == relid )
                {
                    ramparse_NXT(pair->bids,pair->asks,maxdepth*3,gui);
                    if ( pair->bids->assetids[0] == baseid && pair->bids->assetids[1] == relid )
                        strcpy(base,pair->bids->base), strcpy(rel,pair->asks->base);
                }
            }
        }
        if ( baseid != NXT_ASSETID && relid != NXT_ASSETID )
        {
            rawop = create_orderbook(0,baseid,0,relid,oldest,gui);  // base/rel
            fromNXT = create_orderbook(0,baseid,0,NXT_ASSETID,oldest,gui);  // base/jump
            toNXT = create_orderbook(0,relid,0,NXT_ASSETID,oldest,gui); // rel/jump
            op = make_jumpbook(base,baseid,"NXT",rel,relid,toNXT,fromNXT,gui,rawop,sqrt(maxdepth)+1);
        }
        else
        {
            if ( baseid != NXT_ASSETID )
                op = create_orderbook(0,baseid,0,NXT_ASSETID,oldest,gui);  // base/jump
            else if ( relid != NXT_ASSETID )
                op = create_orderbook(0,NXT_ASSETID,0,relid,oldest,gui); // rel/jump
        }
        if ( op != 0 )
        {
            strcpy(op->base,base), strcpy(op->rel,rel);
            sprintf(baserel,"%s/%s",op->base,op->rel);
            printf("ORDERBOOK.(%s) %s/%s iQsize.%ld\n",buf,op->base,op->rel,sizeof(struct InstantDEX_quote));
            json = cJSON_CreateObject();
            bids = cJSON_CreateArray();
            asks = cJSON_CreateArray();
            if ( op->numbids != 0 || op->numasks != 0 )
            {
                for (i=0; i<op->numbids; i++)
                {
                    if ( (i < maxdepth || op->bids[i].nxt64bits == nxt64bits) && (item= gen_orderbook_item(&op->bids[i],allflag,baseid,relid,op->jumpasset)) != 0 )
                        cJSON_AddItemToArray(bids,item), debug_json(item);
                }
                for (i=0; i<op->numasks; i++)
                {
                    if ( (i < maxdepth || op->asks[i].nxt64bits == nxt64bits) && (item= gen_orderbook_item(&op->asks[i],allflag,baseid,relid,op->jumpasset)) != 0 )
                        cJSON_AddItemToArray(asks,item), debug_json(item);
                }
            }
            cJSON_AddItemToObject(json,"pair",cJSON_CreateString(baserel));
            cJSON_AddItemToObject(json,"obookid",cJSON_CreateString(obook));
            cJSON_AddItemToObject(json,"baseid",cJSON_CreateString(assetA));
            cJSON_AddItemToObject(json,"relid",cJSON_CreateString(assetB));
            cJSON_AddItemToObject(json,"bids",bids);
            cJSON_AddItemToObject(json,"asks",asks);
            cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(NXTaddr));
            cJSON_AddItemToObject(json,"timestamp",cJSON_CreateNumber(time(NULL)));
            retstr = cJSON_Print(json);
            //stripwhite_ns(retstr,strlen(retstr));
        } else printf("null op\n");
    }
    else
    {
        sprintf(buf,"{\"error\":\"no orders for (%s)/(%s) (%llu ^ %llu)\"}",base,rel,(long long)baseid,(long long)relid);
        retstr = clonestr(buf);
    }
    free_orderbook(op), free_orderbook(toNXT), free_orderbook(fromNXT), free_orderbook(rawop);
    return(retstr);
}

#endif
