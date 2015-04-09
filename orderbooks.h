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
    struct InstantDEX_quote *quote;
    int32_t dir;
    dir = polarity;
    if ( iQ->isask != 0 )
        dir = -dir;
    if ( iter == 0 )
    {
        if ( dir > 0 )
            op->numbids++;
        else op->numasks++;
    }
    else
    {   double p,v;
        if ( dir > 0 )
            quote = &op->bids[(*numbidsp)++], *quote = *iQ, quote->isask = 0;
        else quote = &op->asks[(*numasksp)++], *quote = *iQ, quote->isask = 1;
        if ( polarity < 0 )
            quote->baseid = iQ->relid, quote->baseamount = iQ->relamount, quote->relid = iQ->baseid, quote->relamount = iQ->baseamount;
        if ( calc_quoteid(quote) != calc_quoteid(iQ) )
            printf("quoteid mismatch %llu vs %llu\n",(long long)calc_quoteid(quote),(long long)calc_quoteid(iQ)), getchar();
        if ( Debuglevel > 2 )
            p = calc_price_volume(&v,quote->baseamount,quote->relamount), printf("%c.(%f %f).%d ",'B'-quote->isask,p,v,polarity);
    }
}

void add_to_orderbook(struct orderbook *op,int32_t iter,int32_t *numbidsp,int32_t *numasksp,struct rambook_info *rb,struct InstantDEX_quote *iQ,int32_t polarity,int32_t oldest,char *gui)
{
    if ( iQ->timestamp >= oldest && iQ->closed == 0 && iQ->matched == 0 )
        update_orderbook(iter,op,numbidsp,numasksp,iQ,polarity,gui);
}

void sort_orderbook(struct orderbook *op,int32_t jumpflag)
{
    if ( op != 0 && (op->numbids > 0 || op->numasks > 0) )
    {
        if ( op->numbids > 0 )
            qsort(op->bids,op->numbids,sizeof(*op->bids),_decreasing_quotes);
        if ( op->numasks > 0 )
            qsort(op->asks,op->numasks,sizeof(*op->asks),_increasing_quotes);
    }
}

struct orderbook *merge_books(char *base,char *rel,struct orderbook *books[],int32_t n,int32_t jumpflag)
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
    sort_orderbook(op,jumpflag);
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
    if ( 0 && rawop != 0 )
    {
        for (i=0; i<rawop->numbids; i++)
            printf("%llu ",(long long)rawop->bids[i].quoteid);
        printf("rawop n.%d\n",rawop->numbids);
        for (i=0; i<rawop->numasks; i++)
            printf("%llu ",(long long)rawop->asks[i].quoteid);
        printf("rawop n.%d\n",rawop->numasks);
    }
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
                if ( Debuglevel > 2 )
                    printf("(%llu %llu, %llu %llu): ",(long long)to->baseid,(long long)to->relid,(long long)from->baseid,(long long)from->relid);
                op->bids = (struct InstantDEX_quote *)calloc(op->numbids,sizeof(*op->bids));
                n = 0;
                if ( to->numasks > 0 && from->numbids > 0 )
                {
                    for (i=0; i<to->numasks&&i<m; i++)
                        for (j=0; j<from->numbids&&j<m; j++)
                            n += make_jumpiQ(baseid,relid,0,&op->bids[n],&from->bids[j],&to->asks[i],gui,0);
                }
                if ( rawop != 0 && rawop->numbids > 0 )
                    for (i=0; i<rawop->numbids; i++)
                        op->bids[n++] = rawop->bids[i];
                op->numbids = n;
            }
            if ( (op->numasks= (from->numasks*to->numbids)+(rawop==0?0:rawop->numasks)) > 0 )
            {
                if ( Debuglevel > 2 )
                    printf("(%llu %llu, %llu %llu): ",(long long)from->baseid,(long long)from->relid,(long long)to->baseid,(long long)to->relid);
                op->asks = (struct InstantDEX_quote *)calloc(op->numasks,sizeof(*op->asks));
                n = 0;
                if ( from->numasks > 0 && to->numbids > 0 )
                {
                    for (i=0; i<from->numasks&&i<m; i++)
                        for (j=0; j<to->numbids&&j<m; j++)
                            n += make_jumpiQ(baseid,relid,1,&op->asks[n],&from->asks[i],&to->bids[j],gui,0);
                }
                if ( rawop != 0 && rawop->numasks > 0 )
                    for (i=0; i<rawop->numasks; i++)
                        op->asks[n++] = rawop->asks[i];
                op->numasks = n;
                if ( Debuglevel > 2 )
                    printf("rawop.%p numasks.%d n.%d\n",rawop,rawop!=0?rawop->numasks:0,n);
            }
        }
    }
    sort_orderbook(op,1);
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
    if ( Debuglevel > 2 )
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
    if ( Debuglevel > 2 )
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
                if ( 0 && rb->numquotes == 1 )
                {
                    double vol;
                    printf("[%d] numquotes.%d: %llu %llu | oldest.%u polarity.%d %f isask.%d quoteid.%llu\n",i,rb->numquotes,(long long)rb->assetids[0],(long long)rb->assetids[1],oldest,polarity,calc_price_volume(&vol,rb->quotes[0].baseamount,rb->quotes[0].relamount),rb->quotes[0].isask,(long long)calc_quoteid(&rb->quotes[0]));
                }
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
        } else sort_orderbook(op,0);
    }
    //printf("(%f %f %llu %u)\n",quotes->price,quotes->vol,(long long)quotes->nxt64bits,quotes->timestamp);
    if ( op != 0 && (op->numbids + op->numasks) == 0 )
        free_orderbook(op), op = 0;
    return(op);
}

char *orderbook_jsonstr(uint64_t nxt64bits,struct orderbook *op,char *base,char *rel,int32_t maxdepth,int32_t allflag)
{
    cJSON *json,*bids,*asks,*item;
    char baserel[64],assetA[64],assetB[64],NXTaddr[64],obook[64];
    int32_t i;
    if ( op == 0 )
        return(clonestr("{\"error\":\"empty orderbook\"}"));
    strcpy(op->base,base), strcpy(op->rel,rel);
    sprintf(baserel,"%s/%s",op->base,op->rel);
    printf("ORDERBOOK %s/%s iQsize.%ld numbids.%d numasks.%d maxdepth.%d\n",op->base,op->rel,sizeof(struct InstantDEX_quote),op->numbids,op->numasks,maxdepth);
    json = cJSON_CreateObject();
    bids = cJSON_CreateArray();
    asks = cJSON_CreateArray();
    if ( op->numbids != 0 || op->numasks != 0 )
    {
        for (i=0; i<op->numbids; i++)
        {
            if ( (i < maxdepth || op->bids[i].nxt64bits == nxt64bits) && (item= gen_orderbook_item(&op->bids[i],allflag,op->baseid,op->relid,op->jumpasset)) != 0 )
                cJSON_AddItemToArray(bids,item);//, debug_json(item);
        }
        for (i=0; i<op->numasks; i++)
        {
            if ( (i < maxdepth || op->asks[i].nxt64bits == nxt64bits) && (item= gen_orderbook_item(&op->asks[i],allflag,op->baseid,op->relid,op->jumpasset)) != 0 )
                cJSON_AddItemToArray(asks,item);//, debug_json(item);
        }
    }
    expand_nxt64bits(obook,_obookid(op->baseid,op->relid));
    expand_nxt64bits(NXTaddr,nxt64bits);
    expand_nxt64bits(assetA,op->baseid);
    expand_nxt64bits(assetB,op->relid);
    cJSON_AddItemToObject(json,"pair",cJSON_CreateString(baserel));
    cJSON_AddItemToObject(json,"obookid",cJSON_CreateString(obook));
    cJSON_AddItemToObject(json,"baseid",cJSON_CreateString(assetA));
    cJSON_AddItemToObject(json,"relid",cJSON_CreateString(assetB));
    cJSON_AddItemToObject(json,"bids",bids);
    cJSON_AddItemToObject(json,"asks",asks);
    cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(NXTaddr));
    cJSON_AddItemToObject(json,"timestamp",cJSON_CreateNumber(time(NULL)));
    return(cJSON_Print(json));
}

struct orderbook *make_orderbook(struct orderbook *obooks[3],char *base,uint64_t baseid,char *rel,uint64_t relid,int32_t maxdepth,uint32_t oldest,char *gui)
{
    struct orderbook *op=0,*toNXT=0,*fromNXT=0,*rawop=0;
    if ( baseid != NXT_ASSETID && relid != NXT_ASSETID )
    {
        rawop = create_orderbook(0,baseid,0,relid,oldest,gui);  // base/rel
        fromNXT = create_orderbook(0,baseid,0,NXT_ASSETID,oldest,gui);  // base/jump
        toNXT = create_orderbook(0,relid,0,NXT_ASSETID,oldest,gui); // rel/jump
        op = make_jumpbook(base,baseid,"NXT",rel,relid,toNXT,fromNXT,gui,rawop,maxdepth);
    }
    else op = create_orderbook(0,baseid,0,relid,oldest,gui);
    obooks[0] = toNXT, obooks[1] = fromNXT, obooks[2] = rawop;
    return(op);
}

cJSON *orderbook_json(uint64_t nxt64bits,char *base,uint64_t baseid,char *rel,uint64_t relid,int32_t maxdepth,int32_t allflag,char *gui)
{
    cJSON *json = 0;
    char *retstr;
    uint32_t oldest = 0;
    struct orderbook *op,*obooks[3];
    if ( (op = make_orderbook(obooks,base,baseid,rel,relid,maxdepth,oldest,gui)) != 0 )
    {
        if ( (retstr = orderbook_jsonstr(nxt64bits,op,base,rel,maxdepth,allflag)) != 0 )
        {
            json = cJSON_Parse(retstr);
            free(retstr);
        }
        free_orderbook(op), free_orderbook(obooks[0]), free_orderbook(obooks[1]), free_orderbook(obooks[2]);
    }
    return(json);
}

char *orderbook_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    struct InstantDEX_quote *iQ = 0;
    int32_t allflag,maxdepth;
    uint32_t oldest;
    struct orderbook *op,*obooks[3];
    uint64_t mult,baseid,relid,nxt64bits = calc_nxt64bits(NXTaddr);
    char buf[MAX_JSON_FIELD],gui[MAX_JSON_FIELD],base[MAX_JSON_FIELD],rel[MAX_JSON_FIELD],*retstr = 0;
    baseid = get_API_nxt64bits(objs[0]), relid = get_API_nxt64bits(objs[1]), allflag = get_API_int(objs[2],0), oldest = get_API_int(objs[3],0);
    maxdepth = get_API_int(objs[4],2), copy_cJSON(base,objs[5]), copy_cJSON(rel,objs[6]), copy_cJSON(gui,objs[7]), gui[sizeof(iQ->gui)-1] = 0;
    retstr = 0;
    if ( baseid != relid && ((baseid != 0 && relid != 0) || (base[0] != 0 && rel[0] != 0)) )
    {
        ensure_rambook(baseid,relid);
        if ( base[0] == 0 )
            set_assetname(&mult,base,baseid);
        if ( rel[0] == 0 )
            set_assetname(&mult,rel,relid);
        update_NXTAE_books(baseid,relid,maxdepth,gui);
        op = make_orderbook(obooks,base,baseid,rel,relid,maxdepth,oldest,gui);
        retstr = orderbook_jsonstr(nxt64bits,op,base,rel,maxdepth,allflag);
        free_orderbook(op), free_orderbook(obooks[0]), free_orderbook(obooks[1]), free_orderbook(obooks[2]);
    }
    else
    {
        sprintf(buf,"{\"error\":\"no orders for (%s)/(%s) (%llu ^ %llu)\"}",base,rel,(long long)baseid,(long long)relid);
        retstr = clonestr(buf);
    }
    return(retstr);
}

#endif
