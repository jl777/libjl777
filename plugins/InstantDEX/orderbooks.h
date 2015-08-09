//
//  orderbooks.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_orderbooks_h
#define xcode_orderbooks_h

#define DEFAULT_POLLGAP 15
#define DEFAULT_MAXDEPTH 25

struct orderbook
{
    uint64_t baseid,relid,jumpasset;
    char base[16],rel[16],jumper[16];
    struct InstantDEX_quote *bids,*asks;
    int32_t numbids,numasks;
};
struct exchange_info *get_exchange(int32_t exchangeid);
struct exchange_info *find_exchange(int32_t *exchangeidp,char *exchangestr);

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
        if ( (iQ= rb->quotes[i]) != 0 )
        {
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
                numchanges++;//, emit_iQ(rb,iQ);
        }
    }
    if ( Debuglevel > 2 )
        fprintf(stderr,"%s %s_%s NEW.%d\n\n",rb->exchange,rb->base,rb->rel,rb->numquotes);
    if ( oldquotes != 0 )
        free(oldquotes);
    return(numchanges);
}

void convram_NXT_Uquotejson(uint64_t assetid)
{
    struct NXT_tx *txptrs[MAX_TXPTRS]; struct NXT_tx *tx; int32_t i,dir,flip,numptrs; uint64_t baseamount,relamount,ap_mult;
    struct InstantDEX_quote iQ; char assetidstr[64];
    expand_nxt64bits(assetidstr,assetid);
    if ( (ap_mult= assetmult(assetidstr)) == 0 )
        return;
    memset(txptrs,0,sizeof(txptrs));
    if ( (numptrs= update_iQ_flags(txptrs,sizeof(txptrs)/sizeof(*txptrs),assetid)) == 0 )
        return;
    for (i=0; i<numptrs; i++)
    {
        tx = txptrs[i];
        for (flip=0; flip<2; flip++)
        {
            if ( tx->assetidbits == assetid && tx->type == 2 && tx->subtype == (3 - flip) && tx->refhash.txid == 0 )
            {
                if ( flip == 0 )
                    dir = 1;
                else dir = -1;
                printf("i.%d: assetid.%llu type.%d subtype.%d time.%u\n",i,(long long)tx->assetidbits,tx->type,tx->subtype,tx->timestamp);
                baseamount = (tx->U.quantityQNT * ap_mult);
                relamount = (tx->U.quantityQNT * tx->priceNQT);
                add_rambook_quote(INSTANTDEX_NXTAENAME,&iQ,tx->senderbits,tx->timestamp,dir,assetid,NXT_ASSETID,0.,0.,baseamount,relamount,0,0,0);
            }
        }
    }
    free_txptrs(txptrs,numptrs);
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

void free_orderbooks(struct orderbook *obooks[],long n,struct orderbook *op)
{
    int32_t i;
    for (i=0; i<n; i++)
    {
        if ( obooks[i] == 0 )
            break;
        if ( obooks[i] != op )
            free_orderbook(obooks[i]);
    }
    free_orderbook(op);
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
    {
        if ( dir > 0 )
            quote = &op->bids[(*numbidsp)++], *quote = *iQ, quote->isask = 0;
        else quote = &op->asks[(*numasksp)++], *quote = *iQ, quote->isask = 1;
        if ( polarity < 0 )
            quote->baseid = iQ->relid, quote->baseamount = iQ->relamount, quote->relid = iQ->baseid, quote->relamount = iQ->baseamount;
        //if ( calc_quoteid(quote) != calc_quoteid(iQ) )
        //    printf("quoteid mismatch %llu vs %llu\n",(long long)calc_quoteid(quote),(long long)calc_quoteid(iQ)), getchar();
        if ( Debuglevel > 2 )
        {
            double p,v;
            p = calc_price_volume(&v,quote->baseamount,quote->relamount);//, printf("%c.(%f %f).%d ",'B'-quote->isask,p,v,polarity);
        }
    }
}

void add_to_orderbook(struct orderbook *op,int32_t iter,int32_t *numbidsp,int32_t *numasksp,struct rambook_info *rb,struct InstantDEX_quote *iQ,int32_t polarity,int32_t oldest,char *gui)
{
    if ( iQ->timestamp >= oldest && iQ->closed == 0 && iQ->matched == 0 )
        update_orderbook(iter,op,numbidsp,numasksp,iQ,polarity,gui);
}

void sort_orderbook(struct orderbook *op)
{
    if ( op != 0 )
    {
        if ( op->numbids > 1 )
            qsort(op->bids,op->numbids,sizeof(*op->bids),_decreasing_quotes);
        if ( op->numasks > 1 )
            qsort(op->asks,op->numasks,sizeof(*op->asks),_increasing_quotes);
    }
}

void debug_json(cJSON *item)
{
    char *str;//,*str2;
    if ( Debuglevel > 1 && item != 0 )
    {
        str = cJSON_Print(item);
        _stripwhite(str,' ');
        //str2 = stringifyM(str);
        printf("./BitcoinDarkd SuperNET '%s'\n",str);
        free(str);//, free(str2);
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

struct orderbook *make_jumpbook(char *base,uint64_t baseid,uint64_t jumpasset,char *rel,uint64_t relid,struct orderbook *to,struct orderbook *from,char *gui,struct orderbook *rawop,int32_t maxdepth)
{
    struct orderbook *op = 0;
    int32_t i,j,n,m = sqrt(maxdepth);
    uint64_t mult;
    if ( m < 10 )
        m = 10;
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
        if ( (nonz_and_lesser(to->numasks,from->numbids) + nonz_and_lesser(to->numbids,from->numasks)) > 0 || (rawop != 0 && (rawop->numbids + rawop->numasks) > 0) )
        {
            op = (struct orderbook *)calloc(1,sizeof(*op));
            strcpy(op->base,base), strcpy(op->rel,rel), set_assetname(&mult,op->jumper,jumpasset);
            op->baseid = baseid;
            op->relid = relid;
            op->jumpasset = jumpasset;
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
            }
        }
    } else op = rawop;
    sort_orderbook(op);
    return(op);
}

int32_t in_list64(uint64_t *list64,int32_t n,uint64_t bits)
{
    int32_t i;
    for (i=0; i<n; i++)
        if ( list64[i] == bits )
            return(i);
    return(-1);
}

int32_t baserelcmp(char *base,uint64_t *baseequivs,int32_t numbase,char *rel,uint64_t *relequivs,int32_t numrel,struct rambook_info *rb)
{
    if ( in_list64(baseequivs,numbase,rb->assetids[0]) >= 0 && in_list64(relequivs,numrel,rb->assetids[1]) >= 0 )
        return(0);
    else if ( strcasecmp(base,rb->base) == 0 && strcasecmp(rel,rb->rel) == 0 )
        return(0);
    return(-1);
}

uint64_t _obookid(uint64_t baseid,uint64_t relid) { return(baseid ^ relid); }

struct orderbook *create_orderbook(char *base,uint64_t refbaseid,char *rel,uint64_t refrelid,uint32_t oldest,char *gui,char *exchange)
{
    int32_t i,j,iter,numbids,numasks,numbooks,polarity,numbase,numrel;
    char obookstr[64],_base[16],_rel[16];
    struct rambook_info **obooks,*rb;
    struct orderbook *op = 0;
    uint64_t basemult,relmult,baseequivs[512],relequivs[512];
    if ( Debuglevel > 2 )
        printf("create_orderbook %llu/%llu\n",(long long)refbaseid,(long long)refrelid);
    if ( refbaseid == 0 || refrelid == 0 )
        getchar();
    expand_nxt64bits(obookstr,_obookid(refbaseid,refrelid));
    _base[0] = _rel[0] = 0;
    if ( base == 0 )
        base = _base;
    if ( rel == 0 )
        rel = _rel;
    if ( refbaseid == 0 && base != 0 && base[0] != 0 )
        refbaseid = stringbits(base);
    else set_assetname(&basemult,base,refbaseid);
    if ( refrelid == 0 && rel != 0 && rel[0] != 0 )
        refrelid = stringbits(rel);
    else set_assetname(&relmult,rel,refrelid);
    if ( refbaseid == 0 || refrelid == 0 )
    {
        printf("create_orderbook: illegal assetids %llu/%llu\n",(long long)refbaseid,(long long)refrelid);
        return(0);
    }
    //if ( (numbase= get_equivalent_assetids(baseequivs,refbaseid)) <= 0 )
        baseequivs[0] = refbaseid, numbase = 1;
    //if ( (numrel= get_equivalent_assetids(relequivs,refrelid)) <= 0 )
        relequivs[0] = refrelid, numrel = 1;
    op = (struct orderbook *)calloc(1,sizeof(*op));
    strcpy(op->base,base), strcpy(op->rel,rel);
    op->baseid = refbaseid, op->relid = refrelid;
    if ( Debuglevel > 2 )
        printf("create_orderbook %s/%s\n",op->base,op->rel);
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
                if ( Debuglevel > 2 )
                    printf("%s numquotes.%d: (%s).%llu (%s).%llu | (%s).%llu (%s).%llu | equiv.(%d %d) match.%d %d\n",rb->exchange,rb->numquotes,rb->base,(long long)rb->assetids[0],rb->rel,(long long)rb->assetids[1],op->base,(long long)op->baseid,op->rel,(long long)op->relid,numbase,numrel,op->baseid == rb->assetids[0] && op->relid == rb->assetids[1],op->baseid == rb->assetids[1] && op->relid == rb->assetids[0]);
                if ( rb->numquotes == 0 || (exchange != 0 && exchange[0] != 0 && strcmp(exchange,rb->exchange) != 0) )
                {
                    continue;
                }
                /*if ( baserelcmp(op->base,baseequivs,numbase,op->rel,relequivs,numrel,rb) == 0 )
                    polarity = 1;
                else if ( baserelcmp(op->rel,relequivs,numrel,op->base,baseequivs,numbase,rb) == 0 )
                    polarity = -1;*/
                if ( op->baseid == rb->assetids[0] && op->relid == rb->assetids[1] )
                    polarity = 1;
                else if ( op->baseid == rb->assetids[1] && op->relid == rb->assetids[0] )
                    polarity = -1;
                else continue;
                if ( Debuglevel > 2 )
                    printf(">>>>>> %s numquotes.%d: (%s).%llu (%s).%llu | (%s).%llu (%s).%llu\n",rb->exchange,rb->numquotes,rb->base,(long long)rb->assetids[0],rb->rel,(long long)rb->assetids[1],op->base,(long long)op->baseid,op->rel,(long long)op->relid);
                if ( 0 && rb->numquotes == 1 )
                {
                    double vol;
                    printf("[%d] numquotes.%d: %llu %llu | oldest.%u polarity.%d %f isask.%d quoteid.%llu\n",i,rb->numquotes,(long long)rb->assetids[0],(long long)rb->assetids[1],oldest,polarity,calc_price_volume(&vol,rb->quotes[0]->baseamount,rb->quotes[0]->relamount),rb->quotes[0]->isask,(long long)calc_quoteid(rb->quotes[0]));
                }
                for (j=0; j<rb->numquotes; j++)
                    add_to_orderbook(op,iter,&numbids,&numasks,rb,rb->quotes[j],polarity,oldest,gui);
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
    if ( Debuglevel > 2 )//|| op->numbids+op->numasks > 0 )
        printf("(%s/%s) %llu/%llu numbids.%d numasks.%d\n",op->base,op->rel,(long long)op->baseid,(long long)op->relid,op->numbids,op->numasks);
    if ( op != 0 && (op->numbids + op->numasks) == 0 )
        free_orderbook(op), op = 0;
    return(op);
}

char *orderbook_jsonstr(uint64_t nxt64bits,struct orderbook *op,char *base,char *rel,int32_t maxdepth,int32_t allflag)
{
    cJSON *json,*bids,*asks,*item,*highbid=0,*lowask=0;
    char baserel[64],assetA[64],assetB[64],NXTaddr[64],obook[64];
    int32_t i;
    if ( op == 0 )
        return(clonestr("{\"error\":\"empty orderbook\"}"));
    strcpy(op->base,base), strcpy(op->rel,rel);
    sprintf(baserel,"%s/%s",op->base,op->rel);
    if ( Debuglevel > 2 )
        printf("ORDERBOOK %s/%s iQsize.%ld numbids.%d numasks.%d maxdepth.%d\n",op->base,op->rel,sizeof(struct InstantDEX_quote),op->numbids,op->numasks,maxdepth);
    json = cJSON_CreateObject();
    bids = cJSON_CreateArray();
    asks = cJSON_CreateArray();
    if ( op->numbids != 0 || op->numasks != 0 )
    {
        for (i=0; i<op->numbids; i++)
        {
            if ( (i < maxdepth || op->bids[i].nxt64bits == nxt64bits) && (item= gen_orderbook_item(&op->bids[i],allflag,op->baseid,op->relid,op->jumpasset)) != 0 )
            {
                cJSON_AddItemToArray(bids,item);
                if ( Debuglevel > 2 && i == 0 )
                    highbid = item;
            }
        }
        for (i=0; i<op->numasks; i++)
        {
            if ( (i < maxdepth || op->asks[i].nxt64bits == nxt64bits) && (item= gen_orderbook_item(&op->asks[i],allflag,op->baseid,op->relid,op->jumpasset)) != 0 )
            {
                cJSON_AddItemToArray(asks,item);
                if ( Debuglevel > 2 && i == 0 )
                    lowask = item;
            }
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
    cJSON_AddItemToObject(json,"maxdepth",cJSON_CreateNumber(maxdepth));
    if ( Debuglevel > 2 )
        debug_json(highbid), debug_json(lowask);
    return(cJSON_Print(json));
}

struct orderbook *merge_books(char *base,uint64_t refbaseid,char *rel,uint64_t refrelid,struct orderbook *books[],int32_t n)
{
    struct orderbook *op = 0;
    int32_t i,numbids,numasks;
    for (i=numbids=0; i<n; i++)
        numbids += books[i]->numbids;
    for (i=numasks=0; i<n; i++)
        numasks += books[i]->numasks;
    op = (struct orderbook *)calloc(1,sizeof(*op));
    strcpy(op->base,base), strcpy(op->rel,rel), sprintf(op->jumper,"merged.%d",n);
    op->baseid = refbaseid;
    op->relid = refrelid;
    op->jumpasset = 0;
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

struct orderbook *make_orderbook(struct orderbook *obooks[],long max,char *base,uint64_t refbaseid,char *rel,uint64_t refrelid,int32_t maxdepth,uint32_t oldest,char *gui,char *exchange)
{
    uint64_t jumpassets[] = { NXT_ASSETID, BTC_ASSETID, BTCD_ASSETID, USD_ASSETID, CNY_ASSETID };
    struct orderbook *op=0,*baseop,*relop,*rawop,*jumpop,*jumpbooks[sizeof(jumpassets)/sizeof(*jumpassets) + 1];
    uint64_t baseid,relid,baseequivs[16],relequivs[16];
    int32_t numbase,numrel,b,r,i,m = 0,n = 0;
    memset(jumpbooks,0,sizeof(jumpbooks));
    if ( (numbase= get_equivalent_assetids(baseequivs,refbaseid)) <= 0 )
        return(0);
    if ( (numrel= get_equivalent_assetids(relequivs,refrelid)) <= 0 )
        return(0);
    for (b=0; b<numbase; b++)
    {
        for (r=0; r<numrel; r++)
        {
            baseid = baseequivs[b], relid = relequivs[r];
            if ( (rawop= create_orderbook(0,baseid,0,relid,oldest,gui,exchange)) != 0 )
                jumpbooks[m++] = obooks[n++] = rawop;
            for (i=0; i<(int32_t)(sizeof(jumpassets)/sizeof(*jumpassets)); i++)
            {
                if ( baseid != jumpassets[i] && relid != jumpassets[i] )
                {
                    if ( (baseop= create_orderbook(0,baseid,0,jumpassets[i],oldest,gui,exchange)) != 0 )
                        obooks[n++] = baseop;
                    if ( (relop= create_orderbook(0,relid,0,jumpassets[i],oldest,gui,exchange)) != 0 )
                        obooks[n++] = relop;
                    if ( baseop != 0 && relop != 0 && (jumpop= make_jumpbook(base,baseid,jumpassets[i],rel,relid,relop,baseop,gui,0,maxdepth)) != 0 )
                        jumpbooks[m++] = obooks[n++] = jumpop;
                }
            }
        }
    }
    if ( n > max )
    {
        printf("make_orderbook: too many sub-orderbooks n.%d > max.%ld\n",n,max);
        return(0);
    }
    obooks[n] = 0;
    if ( m > 1 )
    {
        //printf("num jumpbooks.%d\n",m);
        op = merge_books(base,refbaseid,rel,refrelid,jumpbooks,m);
    }
    else op = jumpbooks[0];
    return(op);
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

/*void update_ramparse(struct exchange_info *exchange,struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    struct InstantDEX_quote *prevbids,*prevasks;
    int32_t numoldbids,numoldasks;
    if ( exchange->ramparse != 0 && exchange->ramparse != ramparse_stub )
    {
        prevbids = clone_quotes(&numoldbids,bids), prevasks = clone_quotes(&numoldasks,asks);
        (*exchange->ramparse)(bids,asks,maxdepth,gui);
        emit_orderbook_changes(bids,prevbids,numoldbids), emit_orderbook_changes(asks,prevasks,numoldasks);
        bids->numupdates++, asks->numupdates++;
    }
    exchange->lastaccess = (uint32_t)time(NULL);
}

void update_rambooks(uint64_t refbaseid,uint64_t refrelid,int32_t maxdepth,char *gui,int32_t showall,char *name)
{
    int32_t cleared_with_nxtae(int32_t);
    uint64_t assetids[8192];
    struct rambook_info *bids,*asks;
    uint64_t baseid,relid;
    struct exchange_info *exchange;
    int32_t i,n,exchangeid;
    if ( name == 0 )
        name = "";
    n = gen_assetpair_list(assetids,sizeof(assetids)/sizeof(*assetids),refbaseid,refrelid);
    for (i=0; i<n; i++)
    {
        baseid = assetids[i*3], relid = assetids[i*3+1], exchangeid = (int32_t)assetids[i*3+2];
        if ( maxdepth > 0 && (exchange= &Exchanges[exchangeid]) != 0 && exchangeid < MAX_EXCHANGES && exchangeid != INSTANTDEX_EXCHANGEID )
        {
            if ( (name[0] == 0 || strcmp(exchange->name,name) == 0) && (showall != 0 || (exchange->trade != 0 || cleared_with_nxtae(exchangeid) != 0)) )
            {
                bids = get_rambook(0,baseid,0,relid,(exchangeid<<1),gui);
                asks = get_rambook(0,baseid,0,relid,(exchangeid<<1) | 1,gui);
                if ( bids != 0 && asks != 0 && (bids->numupdates + asks->numupdates) == 0 )
                {
                    if ( Debuglevel > 2 )
                        fprintf(stderr,"(%llu %llu max.%d).%s ",(long long)baseid,(long long)relid,maxdepth,Exchanges[exchangeid].name);
                    if ( exchange->exchangeid == exchangeid )
                        update_ramparse(exchange,bids,asks,maxdepth,gui);
                }
            }
        }
    }
}*/

char *orderbook_func(int32_t localaccess,int32_t valid,char *sender,cJSON **objs,int32_t numobjs,char *origargstr)
{
    struct InstantDEX_quote *iQ = 0;
    struct orderbook *op,*obooks[1024];
    int32_t allflag,maxdepth,showall; uint32_t oldest;
    uint64_t mult,baseid,relid,nxt64bits = calc_nxt64bits(SUPERNET.NXTADDR);
    char gui[MAX_JSON_FIELD],base[MAX_JSON_FIELD],rel[MAX_JSON_FIELD],exchange[MAX_JSON_FIELD],*retstr = 0;
    baseid = get_API_nxt64bits(objs[0]), relid = get_API_nxt64bits(objs[1]), allflag = get_API_int(objs[2],0), oldest = get_API_int(objs[3],0);
    maxdepth = get_API_int(objs[4],DEFAULT_MAXDEPTH), copy_cJSON(base,objs[5]), copy_cJSON(rel,objs[6]);
    copy_cJSON(gui,objs[7]), gui[sizeof(iQ->gui)-1] = 0, showall = get_API_int(objs[8],1), copy_cJSON(exchange,objs[9]);
    retstr = 0;
    if ( maxdepth <= 0 )
        maxdepth = DEFAULT_MAXDEPTH;
    if ( baseid == 0 && base[0] != 0 )
        baseid = stringbits(base);
    else set_assetname(&mult,base,baseid);
    if ( relid == 0 && rel[0] != 0 )
        relid = stringbits(rel);
    else set_assetname(&mult,rel,relid);
    if ( baseid != 0 && relid != 0 )
    {
        //update_rambooks(baseid,relid,maxdepth,gui,showall,exchange);
        printf("updated\n");
        op = make_orderbook(obooks,sizeof(obooks)/sizeof(*obooks),base,baseid,rel,relid,maxdepth,oldest,gui,exchange);
        printf("made\n");
        retstr = orderbook_jsonstr(nxt64bits,op,base,rel,maxdepth,allflag);
        printf("free\n");
        free_orderbooks(obooks,sizeof(obooks)/sizeof(*obooks),op);
    }
    return(retstr);
}

#endif
