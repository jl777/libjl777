//
//  orders.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_orders_h
#define xcode_orders_h

#define _obookid(baseid,relid) ((baseid) ^ (relid))
#define _iQ_price(iQ) ((double)(iQ)->relamount / (iQ)->baseamount)
#define _iQ_volume(iQ) ((double)(iQ)->baseamount / SATOSHIDEN)

char *assetmap[][2] =
{
    { "5527630", "NXT" },
    { "17554243582654188572", "BTC" },
    { "4551058913252105307", "BTC" },
    { "12659653638116877017", "BTC" },
    { "11060861818140490423", "BTCD" },
    { "6918149200730574743", "BTCD" },
    { "13120372057981370228", "BITS" },
    { "2303962892272487643", "DOGE" },
    { "16344939950195952527", "DOGE" },
    { "6775076774325697454", "OPAL" },
    { "7734432159113182240", "VPN" },
    { "9037144112883608562", "VRC" },
    { "1369181773544917037", "BBR" },
    { "17353118525598940144", "DRK" },
    { "2881764795164526882", "LTC" },
    { "7117580438310874759", "BC" },
    { "275548135983837356", "VIA" },
};

struct rambook_info
{
    UT_hash_handle hh;
    struct InstantDEX_quote *quotes;
    uint64_t baseid,relid,obookid;
    int32_t numquotes,maxquotes;
} *Rambooks;

void set_assetname(char *name,uint64_t assetbits)
{
    char assetstr[64];
    int32_t i,creatededflag;
    struct NXT_asset *ap;
    expand_nxt64bits(assetstr,assetbits);
    for (i=0; i<(int32_t)(sizeof(assetmap)/sizeof(*assetmap)); i++)
    {
        if ( strcmp(assetmap[i][0],assetstr) == 0 )
        {
            strcpy(name,assetmap[i][1]);
            return;
        }
    }
    ap = get_NXTasset(&creatededflag,Global_mp,assetstr);
    strcpy(name,ap->name);
}

cJSON *rambook_json(struct rambook_info *rb)
{
    cJSON *json = cJSON_CreateObject();
    char numstr[64],base[512],rel[512];
    set_assetname(base,rb->baseid);
    cJSON_AddItemToObject(json,"base",cJSON_CreateString(base));
    sprintf(numstr,"%llu",(long long)rb->baseid), cJSON_AddItemToObject(json,"baseid",cJSON_CreateString(numstr));
    set_assetname(rel,rb->relid);
    cJSON_AddItemToObject(json,"rel",cJSON_CreateString(rel));
    sprintf(numstr,"%llu",(long long)rb->relid), cJSON_AddItemToObject(json,"relid",cJSON_CreateString(numstr));
    cJSON_AddItemToObject(json,"numquotes",cJSON_CreateNumber(rb->numquotes));
    return(json);
}

struct rambook_info *get_rambook(uint64_t baseid,uint64_t relid)
{
    uint64_t obookid;
    uint8_t obookiddata[9];
    struct rambook_info *rb;
    obookid = _obookid(baseid,relid);
    obookiddata[8] = (baseid > relid);
    HASH_FIND(hh,Rambooks,obookiddata,sizeof(obookiddata),rb);
    if ( rb == 0 )
    {
        rb = calloc(1,sizeof(*rb));
        rb->obookid = obookid;
        rb->baseid = baseid;
        rb->relid = relid;
        HASH_ADD(hh,Rambooks,obookid,sizeof(obookid),rb);
    }
    return(rb);
}

struct rambook_info **get_allrambooks(int32_t *numbooksp)
{
    int32_t i = 0;
    struct rambook_info *rb,*tmp,**obooks;
    *numbooksp = HASH_COUNT(Rambooks);
    obooks = calloc(*numbooksp,sizeof(*rb));
    HASH_ITER(hh,Rambooks,rb,tmp)
        obooks[i++] = rb;
    if ( i != *numbooksp )
        printf("get_allrambooks HASH_COUNT.%d vs i.%d\n",*numbooksp,i);
    return(obooks);
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

uint64_t find_best_market_maker() // store ranked list
{
    char cmdstr[1024],NXTaddr[64],receiverstr[MAX_JSON_FIELD],*jsonstr;
    cJSON *json,*array,*txobj;
    int32_t i,n,createdflag;
    struct NXT_acct *np,*maxnp = 0;
    uint64_t amount,senderbits;
    uint32_t now = (uint32_t)time(NULL);
    sprintf(cmdstr,"requestType=getAccountTransactions&account=%s&timestamp=%u&type=0&subtype=0",INSTANTDEX_ACCT,38785003);
    if ( (jsonstr= bitcoind_RPC(0,"curl",NXTAPIURL,0,0,cmdstr)) != 0 )
    {
       // mm string.({"requestProcessingTime":33,"transactions":[{"fullHash":"2a2aab3b84dadf092cf4cedcd58a8b5a436968e836338e361c45651bce0ef97e","confirmations":203,"signatureHash":"52a4a43d9055fe4861b3d13fbd03a42fecb8c9ad4ac06a54da7806a8acd9c5d1","transaction":"711527527619439146","amountNQT":"1100000000","transactionIndex":2,"ecBlockHeight":360943,"block":"6797727125503999830","recipientRS":"NXT-74VC-NKPE-RYCA-5LMPT","type":0,"feeNQT":"100000000","recipient":"4383817337783094122","version":1,"sender":"423766016895692955","timestamp":38929220,"ecBlockId":"10121077683890606382","height":360949,"subtype":0,"senderPublicKey":"4e5bbad625df3d536fa90b1e6a28c3f5a56e1fcbe34132391c8d3fd7f671cb19","deadline":1440,"blockTimestamp":38929430,"senderRS":"NXT-8E6V-YBWH-5VMR-26ESD","signature":"4318f36d9cf68ef0a8f58303beb0ed836b670914065a868053da5fe8b096bc0c268e682c0274e1614fc26f81be4564ca517d922deccf169eafa249a88de58036"}]})
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"transactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    txobj = cJSON_GetArrayItem(array,i);
                    copy_cJSON(receiverstr,cJSON_GetObjectItem(txobj,"recipient"));
                    if ( strcmp(receiverstr,INSTANTDEX_ACCT) == 0 )
                    {
                        if ( (senderbits = get_API_nxt64bits(cJSON_GetObjectItem(txobj,"sender"))) != 0 )
                        {
                            expand_nxt64bits(NXTaddr,senderbits);
                            np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
                            amount = get_API_nxt64bits(cJSON_GetObjectItem(txobj,"amountNQT"));
                            if ( np->timestamp != now )
                            {
                                np->quantity = 0;
                                np->timestamp = now;
                            }
                            np->quantity += amount;
                            if ( maxnp == 0 || np->quantity > maxnp->quantity )
                                maxnp = np;
                        }
                    }
                }
            }
            free_json(json);
        }
        free(jsonstr);
    }
    if ( maxnp != 0 )
    {
        printf("Best MM %llu total %.8f\n",(long long)maxnp->H.nxt64bits,dstr(maxnp->quantity));
        return(maxnp->H.nxt64bits);
    }
    return(0);
}

int32_t calc_users_maxopentrades(uint64_t nxt64bits)
{
    return(13);
}

int32_t get_top_MMaker(struct pserver_info **pserverp)
{
    static uint64_t bestMMbits;
    struct nodestats *stats;
    char ipaddr[64];
    *pserverp = 0;
    if ( bestMMbits == 0 )
        bestMMbits = find_best_market_maker();
    if ( bestMMbits != 0 )
    {
        stats = get_nodestats(bestMMbits);
        expand_ipbits(ipaddr,stats->ipbits);
        (*pserverp) = get_pserver(0,ipaddr,0,0);
        return(0);
    }
    return(-1);
}

double calc_price_volume(double *volumep,uint64_t baseamount,uint64_t relamount)
{
    *volumep = ((double)baseamount / SATOSHIDEN);
    return((double)relamount / baseamount);
}

void set_best_amounts(uint64_t *baseamountp,uint64_t *relamountp,double price,double volume)
{
    double checkprice,checkvol,distA,distB,metric,bestmetric = (1. / SMALLVAL);
    uint64_t baseamount,relamount,bestbaseamount = 0,bestrelamount = 0;
    int32_t i,j;
    baseamount = volume * SATOSHIDEN;
    relamount = (price * baseamount);
//*baseamountp = baseamount, *relamountp = relamount;
//return;
    for (i=-1; i<=1; i++)
        for (j=-1; j<=1; j++)
        {
            checkprice = calc_price_volume(&checkvol,baseamount+i,relamount+j);
            distA = (checkprice - price);
            distA *= distA;
            distB = (checkvol - volume);
            distB *= distB;
            metric = sqrt(distA + distB);
            if ( metric < bestmetric )
            {
                bestmetric = metric;
                bestbaseamount = baseamount + i;
                bestrelamount = relamount + j;
                //printf("i.%d j.%d metric. %f\n",i,j,metric);
            }
        }
    *baseamountp = bestbaseamount;
    *relamountp = bestrelamount;
}

int32_t create_InstantDEX_quote(struct InstantDEX_quote *iQ,uint32_t timestamp,int32_t isask,int32_t type,uint64_t nxt64bits,double price,double volume,uint64_t baseamount,uint64_t relamount)
{
    memset(iQ,0,sizeof(*iQ));
    if ( baseamount == 0 && relamount == 0 )
        set_best_amounts(&baseamount,&relamount,price,volume);
    iQ->timestamp = timestamp;
    iQ->type = type;
    iQ->isask = isask;
    iQ->nxt64bits = nxt64bits;
    iQ->baseamount = baseamount;
    iQ->relamount = relamount;
    return(0);
}

cJSON *gen_orderbook_item(struct InstantDEX_quote *iQ,int32_t allflag)
{
    char NXTaddr[64],numstr[64];
    cJSON *array = 0;
    double price,volume;
    if ( iQ->baseamount != 0 && iQ->relamount != 0 )
    {
        if ( iQ->isask != 0 )
            price = calc_price_volume(&volume,iQ->relamount,iQ->baseamount);
        else price = calc_price_volume(&volume,iQ->baseamount,iQ->relamount);
        array = cJSON_CreateArray();
        sprintf(numstr,"%.11f",price), cJSON_AddItemToArray(array,cJSON_CreateString(numstr));
        sprintf(numstr,"%.8f",volume),cJSON_AddItemToArray(array,cJSON_CreateString(numstr));
        if ( allflag != 0 )
        {
            expand_nxt64bits(NXTaddr,iQ->nxt64bits);
            cJSON_AddItemToArray(array,cJSON_CreateString(NXTaddr));
            cJSON_AddItemToArray(array,cJSON_CreateNumber(iQ->type));
            cJSON_AddItemToArray(array,cJSON_CreateNumber(iQ->timestamp));
        }
    }
    return(array);
}

cJSON *gen_InstantDEX_json(int32_t isask,struct InstantDEX_quote *iQ,uint64_t refbaseid,uint64_t refrelid)
{
    cJSON *json = cJSON_CreateObject();
    char numstr[64],base[64],rel[64];
    double price,volume;
    price = calc_price_volume(&volume,iQ->baseamount,iQ->relamount);
    cJSON_AddItemToObject(json,"requestType",cJSON_CreateString((isask != 0) ? "ask" : "bid"));
    set_assetname(base,refbaseid), cJSON_AddItemToObject(json,"base",cJSON_CreateString(base));
    set_assetname(rel,refrelid), cJSON_AddItemToObject(json,"rel",cJSON_CreateString(rel));
    cJSON_AddItemToObject(json,"price",cJSON_CreateNumber(price));
    cJSON_AddItemToObject(json,"volume",cJSON_CreateNumber(volume));
    
    cJSON_AddItemToObject(json,"timestamp",cJSON_CreateNumber(iQ->timestamp));
    cJSON_AddItemToObject(json,"age",cJSON_CreateNumber((uint32_t)time(NULL) - iQ->timestamp));
    cJSON_AddItemToObject(json,"type",cJSON_CreateNumber(iQ->type));
    sprintf(numstr,"%llu",(long long)iQ->nxt64bits), cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(numstr));
    
    sprintf(numstr,"%llu",(long long)refbaseid), cJSON_AddItemToObject(json,"baseid",cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)iQ->baseamount), cJSON_AddItemToObject(json,"baseamount",cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)refrelid), cJSON_AddItemToObject(json,"relid",cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)iQ->relamount), cJSON_AddItemToObject(json,"relamount",cJSON_CreateString(numstr));
    return(json);
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

char *openorders_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    cJSON *array,*json = 0;
    struct rambook_info **obooks,*rb;
    struct InstantDEX_quote *iQ;
    int32_t i,j,numbooks,n = 0;
    char nxtaddr[64],*jsonstr;
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
                if ( strcmp(NXTaddr,nxtaddr) == 0 )
                    cJSON_AddItemToArray(array,gen_InstantDEX_json(iQ->isask,iQ,rb->baseid,rb->relid)), n++;
            }
        }
        free(obooks);
        if ( n > 0 )
        {
            json = cJSON_CreateObject();
            cJSON_AddItemToObject(json,"openorders",array);
            jsonstr = cJSON_Print(json);
            free_json(json);
            return(jsonstr);
        }
    }
    return(clonestr("{\"result\":\"no openorders\"}"));
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

void update_orderbook(int32_t iter,struct orderbook *op,int32_t *numbidsp,int32_t *numasksp,struct InstantDEX_quote *iQ,int32_t polarity)
{
    struct InstantDEX_quote *ask;
    if ( iter == 0 )
    {
        if ( polarity > 0 )
            op->numbids++;
        else op->numasks++;
    }
    else
    {
        if ( polarity > 0 )
            op->bids[(*numbidsp)++] = *iQ;
        else
        {
            ask = &op->asks[(*numasksp)++];
            *ask = *iQ;
            ask->baseamount = iQ->relamount;
            ask->relamount = iQ->baseamount;
        }
    }
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
    //printf("add_user_order i.%d numquotes.%d\n",i,rb->numquotes);
    if ( i == rb->numquotes )
    {
        if ( i >= rb->maxquotes )
        {
            rb->maxquotes += 1024;
            rb->quotes = realloc(rb->quotes,rb->maxquotes * sizeof(*rb->quotes));
            memset(&rb->quotes[i],0,1024 * sizeof(*rb->quotes));
        }
        rb->quotes[rb->numquotes++] = *iQ;
    }
}

uint64_t purge_oldest_order(struct rambook_info *rb,struct InstantDEX_quote *iQ) // allow one pair per orderbook
{
    char NXTaddr[64];
    struct NXT_acct *np;
    int32_t oldi,createdflag;
    uint64_t nxt64bits = 0;
    uint32_t i,oldest = 0;
    if ( rb->numquotes == 0 )
        return(0);
    oldi = -1;
    for (i=0; i<rb->numquotes; i++)
    {
        if ( rb->quotes[i].nxt64bits == iQ->nxt64bits && (oldest == 0 || rb->quotes[i].timestamp < oldest) )
        {
            oldest = rb->quotes[i].timestamp;
            nxt64bits = rb->quotes[i].nxt64bits;
            oldi = i;
        }
    }
    if ( oldi >= 0 )
    {
        printf("purge_oldest_order from NXT.%llu oldi.%d timestamp %u\n",(long long)iQ->nxt64bits,oldi,oldest);
        rb->quotes[oldi] = rb->quotes[--rb->numquotes];
        memset(&rb->quotes[rb->numquotes],0,sizeof(rb->quotes[rb->numquotes]));
        expand_nxt64bits(NXTaddr,nxt64bits);
        np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
        if ( np->openorders > 0 )
            np->openorders--;
    }
    return(nxt64bits);
}

void save_InstantDEX_quote(struct rambook_info *rb,struct InstantDEX_quote *iQ)
{
    char NXTaddr[64];
    uint64_t obookid;
    struct NXT_acct *np;
    int32_t createdflag,maxallowed;
    obookid = _obookid(rb->baseid,rb->relid);
    maxallowed = calc_users_maxopentrades(iQ->nxt64bits);
    expand_nxt64bits(NXTaddr,iQ->nxt64bits);
    np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
    if ( np->openorders >= maxallowed )
        purge_oldest_order(rb,iQ); // allow one pair per orderbook
    add_user_order(rb,iQ);
    np->openorders++;
}

void add_to_orderbook(struct orderbook *op,int32_t iter,int32_t *numbidsp,int32_t *numasksp,struct rambook_info *rb,struct InstantDEX_quote *iQ,int32_t polarity,int32_t oldest)
{
    uint32_t purgetime = ((uint32_t)time(NULL) - NODESTATS_EXPIRATION);
    if ( iQ->timestamp >= oldest )
        update_orderbook(iter,op,numbidsp,numasksp,iQ,polarity);
    else if ( iQ->timestamp > purgetime )
        purge_oldest_order(rb,iQ);
}

struct orderbook *create_orderbook(uint32_t oldest,uint64_t refbaseid,uint64_t refrelid)
{
    int32_t i,j,iter,numbids,numasks,numbooks,polarity;
    char obookstr[64];
    struct rambook_info **obooks,*rb;
    struct orderbook *op = 0;
    expand_nxt64bits(obookstr,_obookid(refbaseid,refrelid));
    op = (struct orderbook *)calloc(1,sizeof(*op));
    op->baseid = refbaseid;
    op->relid = refrelid;
    for (iter=0; iter<2; iter++)
    {
        numbids = numasks = 0;
        if ( (obooks= get_allrambooks(&numbooks)) != 0 )
        {
            for (i=0; i<numbooks; i++)
            {
                rb = obooks[i];
                if ( rb->numquotes == 0 )
                    continue;
                if ( rb->baseid == refbaseid && rb->relid == refrelid )
                    polarity = 1;
                else if ( rb->relid == refbaseid && rb->baseid == refrelid )
                    polarity = -1;
                else continue;
                for (j=0; j<rb->numquotes; j++)
                    add_to_orderbook(op,iter,&numbids,&numasks,rb,&rb->quotes[j],polarity,oldest);
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
    return(op);
}

char *orderbook_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    uint32_t oldest;
    int32_t i,allflag;
    uint64_t baseid,relid;
    cJSON *json,*bids,*asks,*item;
    struct orderbook *op;
    char obook[64],buf[MAX_JSON_FIELD],base[64],rel[64],baserel[128],datastr[MAX_JSON_FIELD],assetA[64],assetB[64],*retstr = 0;
    baseid = get_API_nxt64bits(objs[0]);
    relid = get_API_nxt64bits(objs[1]);
    allflag = get_API_int(objs[2],0);
    oldest = get_API_int(objs[3],0);
    expand_nxt64bits(obook,_obookid(baseid,relid));
    sprintf(buf,"{\"baseid\":\"%llu\",\"relid\":\"%llu\",\"oldest\":%u}",(long long)baseid,(long long)relid,oldest);
    init_hexbytes_noT(datastr,(uint8_t *)buf,strlen(buf));
    //printf("ORDERBOOK.(%s)\n",buf);
    retstr = 0;
    if ( baseid != 0 && relid != 0 && (op= create_orderbook(oldest,baseid,relid)) != 0 )
    {
        if ( op->numbids == 0 && op->numasks == 0 )
            retstr = clonestr("{\"error\":\"no bids or asks\"}");
        else
        {
            json = cJSON_CreateObject();
            bids = cJSON_CreateArray();
            for (i=0; i<op->numbids; i++)
            {
                if ( (item= gen_orderbook_item(&op->bids[i],allflag)) != 0 )
                    cJSON_AddItemToArray(bids,item);
            }
            asks = cJSON_CreateArray();
            for (i=0; i<op->numasks; i++)
            {
                if ( (item= gen_orderbook_item(&op->asks[i],allflag)) != 0 )
                    cJSON_AddItemToArray(asks,item);
            }
            expand_nxt64bits(assetA,op->baseid);
            expand_nxt64bits(assetB,op->relid);
            set_assetname(base,op->baseid);
            set_assetname(rel,op->relid);
            sprintf(baserel,"%s/%s",base,rel);
            cJSON_AddItemToObject(json,"pair",cJSON_CreateString(baserel));
            cJSON_AddItemToObject(json,"obookid",cJSON_CreateString(obook));
            cJSON_AddItemToObject(json,"baseid",cJSON_CreateString(assetA));
            cJSON_AddItemToObject(json,"relid",cJSON_CreateString(assetB));
            cJSON_AddItemToObject(json,"bids",bids);
            cJSON_AddItemToObject(json,"asks",asks);
            cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(NXTaddr));
            retstr = cJSON_Print(json);
        }
        free_orderbook(op);
    }
    else
    {
        sprintf(buf,"{\"error\":\"no such orderbook.(%llu ^ %llu)\"}",(long long)baseid,(long long)relid);
        retstr = clonestr(buf);
    }
    return(retstr);
}

void submit_quote(char *quotestr)
{
    int32_t len;
    char _tokbuf[4096];
    struct pserver_info *pserver;
    struct coin_info *cp = get_coin_info("BTCD");
    if ( cp != 0 )
    {
        printf("submit_quote.(%s)\n",quotestr);
        len = construct_tokenized_req(_tokbuf,quotestr,cp->srvNXTACCTSECRET);
        if ( get_top_MMaker(&pserver) == 0 )
            call_SuperNET_broadcast(pserver,_tokbuf,len,300);
        call_SuperNET_broadcast(0,_tokbuf,len,300);
    }
}

char *placequote_func(char *previpaddr,int32_t dir,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    cJSON *json;
    uint64_t baseamount,relamount,nxt64bits,baseid,relid,txid = 0;
    double price,volume;
    uint32_t timestamp;
    int32_t remoteflag,type;
    struct rambook_info *rb;
    struct InstantDEX_quote iQ;
    char buf[MAX_JSON_FIELD],txidstr[64],*jsonstr,*retstr = 0;
    remoteflag = (is_remote_access(previpaddr) != 0);
    nxt64bits = calc_nxt64bits(sender);
    baseid = get_API_nxt64bits(objs[0]);
    relid = get_API_nxt64bits(objs[1]);
    if ( baseid == 0 || relid == 0 )
        return(clonestr("{\"error\":\"illegal asset id\"}"));
    baseamount = get_API_nxt64bits(objs[5]);
    relamount = get_API_nxt64bits(objs[6]);
    if ( baseamount != 0 && relamount != 0 )
        price = calc_price_volume(&volume,baseamount,relamount);
    else
    {
        volume = get_API_float(objs[2]);
        price = get_API_float(objs[3]);
    }
    type = (int32_t)get_API_int(objs[7],0);
    if ( (timestamp= (uint32_t)get_API_int(objs[4],0)) == 0 )
        timestamp = (uint32_t)time(NULL);
    printf("t.%u placequote type.%d dir.%d sender.(%s) valid.%d price %.11f vol %.8f\n",timestamp,type,dir,sender,valid,price,volume);
    if ( sender[0] != 0 && valid > 0 )
    {
        if ( price != 0. && volume != 0. && dir != 0 )
        {
            if ( dir > 0 )
            {
                rb = get_rambook(baseid,relid);
                create_InstantDEX_quote(&iQ,timestamp,0,type,nxt64bits,price,volume,baseamount,relamount);
            }
            else
            {
                rb = get_rambook(relid,baseid);
                create_InstantDEX_quote(&iQ,timestamp,0,type,nxt64bits,price,volume,relamount,baseamount);
            }
            save_InstantDEX_quote(rb,&iQ);
            if ( remoteflag == 0 && (json= gen_InstantDEX_json(dir<0,&iQ,baseid,relid)) != 0 )
            {
                jsonstr = cJSON_Print(json);
                stripwhite_ns(jsonstr,strlen(jsonstr));
                submit_quote(jsonstr);
                free_json(json);
                free(jsonstr);
            }
            txid = calc_txid((uint8_t *)&iQ,sizeof(iQ));
            if ( txid != 0 )
            {
                expand_nxt64bits(txidstr,txid);
                sprintf(buf,"{\"result\":\"success\",\"txid\":\"%s\"}",txidstr);
                retstr = clonestr(buf);
                printf("placequote.(%s)\n",buf);
            }
        }
        if ( retstr == 0 )
        {
            sprintf(buf,"{\"error submitting\":\"place%s error %llu/%llu volume %f price %f\"}",dir>0?"bid":"ask",(long long)baseid,(long long)relid,volume,price);
            retstr = clonestr(buf);
        }
    }
    else
    {
        sprintf(buf,"{\"error\":\"place%s error %llu/%llu dir.%d volume %f price %f\"}",dir>0?"bid":"ask",(long long)baseid,(long long)relid,dir,volume,price);
        retstr = clonestr(buf);
    }
    return(retstr);
}

char *placebid_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    return(placequote_func(previpaddr,1,sender,valid,objs,numobjs,origargstr));
}

char *placeask_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    return(placequote_func(previpaddr,-1,sender,valid,objs,numobjs,origargstr));
}

char *bid_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    return(placequote_func(previpaddr,1,sender,valid,objs,numobjs,origargstr));
}

char *ask_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    return(placequote_func(previpaddr,-1,sender,valid,objs,numobjs,origargstr));
}

#undef _ASKMASK
#undef _TYPEMASK
#undef _obookid
#undef _iQ_dir
#undef _iQ_type
#undef _iQ_price
#undef _iQ_volume

#endif
