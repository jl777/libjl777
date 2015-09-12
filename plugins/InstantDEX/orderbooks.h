/******************************************************************************
 * Copyright © 2014-2015 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * Nxt software, including this file, may be copied, modified, propagated,    *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/


#ifndef xcode_orderbooks_h
#define xcode_orderbooks_h

struct prices777 *prices777_find(int32_t *invertedp,uint64_t baseid,uint64_t relid,char *exchange)
{
    int32_t i; struct prices777 *prices;
    *invertedp = 0;
    for (i=0; i<BUNDLE.num; i++)
    {
        if ( (prices= BUNDLE.ptrs[i]) != 0 && strcmp(prices->exchange,exchange) == 0 )
        {
            //printf("FOUND.(%s)\n",exchange);
            if ( prices777_equiv(prices->baseid) == prices777_equiv(baseid) && prices777_equiv(prices->relid) == prices777_equiv(relid) )
                return(prices);
            else if ( prices777_equiv(prices->relid) == prices777_equiv(baseid) && prices777_equiv(prices->baseid) == prices777_equiv(relid) )
            {
                *invertedp = 1;
                return(prices);
            }
            //else printf("(%llu/%llu) != (%llu/%llu)\n",(long long)baseid,(long long)relid,(long long)prices->baseid,(long long)prices->relid);
        } //else fprintf(stderr,"(%s).%d ",prices->exchange,i);
    }
    printf("CANTFIND.(%s) %llu/%llu\n",exchange,(long long)baseid,(long long)relid);
    return(0);
}

struct prices777 *prices777_createbasket(int32_t addbasket,char *name,char *base,char *rel,uint64_t baseid,uint64_t relid,struct prices777_basket *basket,int32_t n,char *typestr)
{
    int32_t i,j,m,iter,max = 0; double firstwt,wtsum; struct prices777 *prices,*feature;
    printf("createbasket.%s n.%d (%s/%s)\n",typestr,n,base,rel);
    prices = prices777_initpair(1,0,typestr,base,rel,0.,name,baseid,relid,n);
    for (iter=0; iter<2; iter++)
    {
        for (i=0; i<n; i++)
        {
            feature = basket[i].prices;
            if ( addbasket*iter != 0 )
            {
                feature->dependents = realloc(feature->dependents,sizeof(*feature->dependents) * (feature->numdependents + 1));
                feature->dependents[feature->numdependents++] = &prices->changed;
                printf("%p i.%d/%d addlink.%s groupid.%d wt.%f (%s.%llu/%llu -> %s.%llu/%llu).%s\n",feature,i,n,feature->exchange,basket[i].groupid,basket[i].wt,feature->contract,(long long)feature->baseid,(long long)feature->relid,prices->contract,(long long)prices->baseid,(long long)prices->relid,prices->exchange);
            }
            if ( basket[i].groupid > max )
                max = basket[i].groupid;
            if ( fabs(basket[i].wt) < SMALLVAL )
            {
                printf("all basket features.%s i.%d must have nonzero wt\n",feature->contract,i);
                free(prices);
                return(0);
            }
            if ( strcmp(feature->base,feature->rel) == 0 || feature->baseid == feature->relid )
            {
                printf("base rel cant be the same (%s %s) %llu %llu\n",feature->base,feature->rel,(long long)feature->baseid,(long long)feature->relid);
                free(prices);
                return(0);
            }
        }
        if ( (max+1) > MAX_GROUPS )
        {
            printf("baskets limited to %d, %d is too many for %s.(%s/%s)\n",MAX_GROUPS,n,name,base,rel);
            return(0);
        }
    }
    prices->numgroups = (max + 1);
    for (j=0; j<prices->numgroups; j++)
    {
        for (firstwt=i=m=0; i<n; i++)
        {
            //printf("i.%d groupid.%d wt %f m.%d\n",i,basket[i].groupid,basket[i].wt,m);
            if ( basket[i].groupid == j )
            {
                if ( firstwt == 0. )
                    prices->groupwts[j] = firstwt = basket[i].wt;
                else if ( basket[i].wt != firstwt )
                {
                    printf("warning features of same group.%d different wt: %d %f != %f\n",j,i,firstwt,basket[i].wt);
                    // free(prices);
                    // return(0);
                }
                m++;
            }
        }
        //printf("m.%d\n",m);
        for (i=0; i<n; i++)
            if ( basket[i].groupid == j )
                basket[i].groupsize = m, printf("basketsize.%d n.%d j.%d groupsize[%d] <- m.%d\n",prices->basketsize,n,j,i,m);
    }
    for (j=0; j<prices->numgroups; j++)
        for (i=0; i<n; i++)
            if ( basket[i].groupid == j )
                prices->basket[prices->basketsize++] = basket[i];
    for (i=-1; i<=1; i+=2)
    {
        for (wtsum=j=m=0; j<prices->numgroups; j++)
        {
            if ( prices->groupwts[j]*i > 0 )
                wtsum += prices->groupwts[j], m++;
        }
        if ( 0 && wtsum != 0. )
        {
            if ( wtsum < 0. )
                wtsum = -wtsum;
            for (j=0; j<prices->numgroups; j++)
                prices->groupwts[j] /= wtsum;
        }
    }
    if ( prices->numgroups == 1 )
        prices->groupwts[0] = 1.;
    for (j=0; j<prices->numgroups; j++)
        printf("%9.6f ",prices->groupwts[j]);
    printf("groupwts %s\n",typestr);
    return(prices);
}

double prices777_price_volume(double *volumep,uint64_t baseamount,uint64_t relamount)
{
    *volumep = (((double)baseamount + 0.000000009999999) / SATOSHIDEN);
    if ( baseamount > 0. )
        return((double)relamount / (double)baseamount);
    else return(0.);
}

void prices777_best_amounts(uint64_t *baseamountp,uint64_t *relamountp,double price,double volume)
{
    double checkprice,checkvol,distA,distB,metric,bestmetric = (1. / SMALLVAL);
    uint64_t baseamount,relamount,bestbaseamount = 0,bestrelamount = 0;
    int32_t i,j;
    baseamount = volume * SATOSHIDEN;
    relamount = ((price * volume) * SATOSHIDEN);
    //*baseamountp = baseamount, *relamountp = relamount;
    //return;
    for (i=-1; i<=1; i++)
        for (j=-1; j<=1; j++)
        {
            checkprice = prices777_price_volume(&checkvol,baseamount+i,relamount+j);
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

void prices777_additem(cJSON **highbidp,cJSON **lowaskp,cJSON *bids,cJSON *asks,int32_t ind,cJSON *item,int32_t bidask)
{
    if ( bidask == 0 )
    {
        cJSON_AddItemToArray(bids,item);
        if ( ind == 0 )
            *highbidp = item;
    }
    else
    {
        cJSON_AddItemToArray(asks,item);
        if ( ind == 0 )
            *lowaskp = item;
    }
}

uint64_t calc_qty(uint64_t mult,uint64_t assetid,uint64_t amount)
{
    if ( assetid != NXT_ASSETID )
        return(amount / mult);
    else return(amount);
}

int32_t verify_NXTtx(cJSON *json,uint64_t refasset,uint64_t qty,uint64_t destNXTbits)
{
    int32_t typeval,subtypeval,n = 0;
    uint64_t quantity,price,assetidbits;
    cJSON *attachmentobj;
    char sender[MAX_JSON_FIELD],recipient[MAX_JSON_FIELD],deadline[MAX_JSON_FIELD],feeNQT[MAX_JSON_FIELD],amountNQT[MAX_JSON_FIELD],type[MAX_JSON_FIELD],subtype[MAX_JSON_FIELD],verify[MAX_JSON_FIELD],referencedTransaction[MAX_JSON_FIELD],quantityQNT[MAX_JSON_FIELD],priceNQT[MAX_JSON_FIELD],assetidstr[MAX_JSON_FIELD],sighash[MAX_JSON_FIELD],fullhash[MAX_JSON_FIELD],timestamp[MAX_JSON_FIELD],transaction[MAX_JSON_FIELD];
    if ( json == 0 )
    {
        printf("verify_NXTtx cant parse json\n");
        return(-1);
    }
    if ( extract_cJSON_str(sender,sizeof(sender),json,"sender") > 0 ) n++;
    if ( extract_cJSON_str(recipient,sizeof(recipient),json,"recipient") > 0 ) n++;
    if ( extract_cJSON_str(referencedTransaction,sizeof(referencedTransaction),json,"referencedTransactionFullHash") > 0 ) n++;
    if ( extract_cJSON_str(amountNQT,sizeof(amountNQT),json,"amountNQT") > 0 ) n++;
    if ( extract_cJSON_str(feeNQT,sizeof(feeNQT),json,"feeNQT") > 0 ) n++;
    if ( extract_cJSON_str(deadline,sizeof(deadline),json,"deadline") > 0 ) n++;
    if ( extract_cJSON_str(type,sizeof(type),json,"type") > 0 ) n++;
    if ( extract_cJSON_str(subtype,sizeof(subtype),json,"subtype") > 0 ) n++;
    if ( extract_cJSON_str(verify,sizeof(verify),json,"verify") > 0 ) n++;
    if ( extract_cJSON_str(sighash,sizeof(sighash),json,"signatureHash") > 0 ) n++;
    if ( extract_cJSON_str(fullhash,sizeof(fullhash),json,"fullHash") > 0 ) n++;
    if ( extract_cJSON_str(timestamp,sizeof(timestamp),json,"timestamp") > 0 ) n++;
    if ( extract_cJSON_str(transaction,sizeof(transaction),json,"transaction") > 0 ) n++;
    if ( calc_nxt64bits(recipient) != destNXTbits )
    {
        if ( Debuglevel > 2 )
            fprintf(stderr,"recipient.%s != %llu\n",recipient,(long long)destNXTbits);
        return(-2);
    }
    typeval = myatoi(type,256), subtypeval = myatoi(subtype,256);
    if ( refasset == NXT_ASSETID )
    {
        if ( typeval != 0 || subtypeval != 0 )
        {
            fprintf(stderr,"unexpected typeval.%d subtypeval.%d\n",typeval,subtypeval);
            return(-3);
        }
        if ( qty != calc_nxt64bits(amountNQT) )
        {
            fprintf(stderr,"unexpected qty.%llu vs.%s\n",(long long)qty,amountNQT);
            return(-4);
        }
        return(0);
    }
    else
    {
        if ( typeval != 2 || subtypeval != 1 )
        {
            if ( Debuglevel > 2 )
                fprintf(stderr,"refasset.%llu qty %lld\n",(long long)refasset,(long long)qty);
            return(-11);
        }
        price = quantity = assetidbits = 0;
        attachmentobj = cJSON_GetObjectItem(json,"attachment");
        if ( attachmentobj != 0 )
        {
            if ( extract_cJSON_str(assetidstr,sizeof(assetidstr),attachmentobj,"asset") > 0 )
                assetidbits = calc_nxt64bits(assetidstr);
            //else if ( extract_cJSON_str(assetidstr,sizeof(assetidstr),attachmentobj,"currency") > 0 )
            //    assetidbits = calc_nxt64bits(assetidstr);
            if ( extract_cJSON_str(quantityQNT,sizeof(quantityQNT),attachmentobj,"quantityQNT") > 0 )
                quantity = calc_nxt64bits(quantityQNT);
            //else if ( extract_cJSON_str(quantityQNT,sizeof(quantityQNT),attachmentobj,"units") > 0 )
            //    quantity = calc_nxt64bits(quantityQNT);
            if ( extract_cJSON_str(priceNQT,sizeof(priceNQT),attachmentobj,"priceNQT") > 0 )
                price = calc_nxt64bits(priceNQT);
        }
        if ( assetidbits != refasset )
        {
            fprintf(stderr,"assetidbits %llu != %llu refasset\n",(long long)assetidbits,(long long)refasset);
            return(-12);
        }
        if ( qty != quantity )
        {
            fprintf(stderr,"qty.%llu != %llu\n",(long long)qty,(long long)quantity);
            return(-13);
        }
        return(0);
    }
    return(-1);
}

int32_t InstantDEX_verify(uint64_t destNXTaddr,uint64_t sendasset,uint64_t sendqty,cJSON *txobj,uint64_t recvasset,uint64_t recvqty)
{
    int32_t err;
    // verify recipient, amounts in txobj
     if ( (err= verify_NXTtx(txobj,recvasset,recvqty,destNXTaddr)) != 0 )
    {
        if ( Debuglevel > 2 )
            printf("InstantDEX_verify dest.(%llu) tx mismatch %d (%llu %lld) -> (%llu %lld)\n",(long long)destNXTaddr,err,(long long)sendasset,(long long)sendqty,(long long)recvasset,(long long)recvqty);
        return(-1);
    }
    return(0);
}

cJSON *wallet_swapjson(char *recv,uint64_t recvasset,char *send,uint64_t sendasset,uint64_t orderid,uint64_t quoteid)
{
    int32_t iter; uint64_t assetid; struct coin777 *coin; struct InstantDEX_quote *iQ;
    char account[128],walletstr[512],*addr,*str; cJSON *walletitem = 0;
    if ( (iQ= find_iQ(quoteid)) != 0 && iQ->s.wallet != 0 )
    {
        walletitem = cJSON_Parse(iQ->walletstr);
        //printf("start with (%s)\n",iQ->walletstr);
    }
    for (iter=0; iter<2; iter++)
    {
        addr = 0;
        str = (iter == 0) ? recv : send;
        assetid = (iter == 0) ? recvasset : sendasset;
        if ( (coin= coin777_find(str,1)) != 0 )
        {
            if ( is_NXT_native(assetid) == 0 )
            {
                if ( (walletitem= set_walletstr(walletitem,walletstr,iQ)) != 0 )
                {
                    
                }
            } // else printf("%s is NXT\n",coin->name);
            if ( is_NXT_native(assetid) != 0 )
                addr = SUPERNET.NXTADDR;
            else
            {
                addr = (iter == 0) ? coin->atomicrecv : coin->atomicsend;
                if ( addr[0] == 0 )
                    addr = get_acct_coinaddr(addr,str,coin->serverport,coin->userpass,account);
            }
            if ( addr != 0 )
            {
            } else printf("%s no addr\n",coin->name);
        } else printf("cant find coin.(%s)\n",iter == 0 ? recv : send);
    }
    if ( walletitem == 0 )
        walletitem = cJSON_CreateObject(), jaddstr(walletitem,"error","cant find local coin daemons");
    return(walletitem);
}

void _prices777_item(cJSON *item,int32_t group,struct prices777 *prices,int32_t bidask,double price,double volume,uint64_t orderid,uint64_t quoteid)
{
    uint64_t baseqty,relqty; int32_t iswallet = 0; char basec,relc; struct InstantDEX_quote *iQ;
    jaddnum(item,"group",group);
    jaddstr(item,"exchange",prices->exchange);
    jaddstr(item,"base",prices->base), jaddstr(item,"rel",prices->rel);
    if ( (iQ= find_iQ(quoteid)) != 0 )
        jadd64bits(item,"offerNXT",iQ->s.offerNXT);
    if ( strcmp(prices->exchange,"nxtae") == 0 || strcmp(prices->exchange,"unconf") == 0 || strcmp(prices->exchange,"InstantDEX") == 0 || strcmp(prices->exchange,"wallet") == 0 )
    {
        jadd64bits(item,prices->type == 5 ? "currency" : "asset",prices->baseid);
        //else if ( quoteid != 0 ) printf("cant find offerNXT.%llu\n",(long long)quoteid);
        jadd64bits(item,"baseid",prices->baseid), jadd64bits(item,"relid",prices->relid);
        iswallet = (strcmp(prices->exchange,"wallet") == 0);
        if ( strcmp(prices->exchange,"InstantDEX") == 0 || iswallet != 0 )
        {
            jaddstr(item,"trade","swap");
            baseqty = calc_qty(prices->basemult,prices->baseid,SATOSHIDEN * volume + 0.5/SATOSHIDEN);
            //printf("baseid.%llu basemult.%llu -> %llu\n",(long long)prices->baseid,(long long)prices->basemult,(long long)baseqty);
            relqty = calc_qty(prices->relmult,prices->relid,SATOSHIDEN * volume * price + 0.5/SATOSHIDEN);
            if ( bidask != 0 )
            {
                basec = '+', relc = '-';
                jadd64bits(item,"recvbase",baseqty);
                jadd64bits(item,"sendrel",relqty);
                if ( iswallet != 0 )
                    jadd(item,"wallet",wallet_swapjson(prices->base,prices->baseid,prices->rel,prices->relid,orderid,quoteid));
            }
            else
            {
                basec = '-', relc = '+';
                jadd64bits(item,"sendbase",baseqty);
                jadd64bits(item,"recvrel",relqty);
                if ( iswallet != 0 )
                    jadd(item,"wallet",wallet_swapjson(prices->rel,prices->relid,prices->base,prices->baseid,orderid,quoteid));
            }
            //printf("(%s %cbaseqty.%llu <-> %s %crelqty.%llu) basemult.%llu baseid.%llu vol %f amount %llu\n",prices->base,basec,(long long)baseqty,prices->rel,relc,(long long)relqty,(long long)prices->basemult,(long long)prices->baseid,volume,(long long)volume*SATOSHIDEN);
        }
        else
        {
            //printf("alternate path\n");
            jaddstr(item,"trade",bidask == 0 ? "sell" : "buy");
        }
    }
    else
    {
        //printf("alternate path\n");
        jaddstr(item,"trade",bidask == 0 ? "sell" : "buy");
        jaddstr(item,"name",prices->contract);
    }
    jaddnum(item,"orderprice",price);
    jaddnum(item,"ordervolume",volume);
    if ( orderid != 0 )
        jadd64bits(item,"orderid",orderid);
    if ( quoteid != 0 )
        jadd64bits(item,"quoteid",quoteid);
}

cJSON *prices777_item(int32_t rootbidask,struct prices777 *prices,int32_t group,int32_t bidask,double origprice,double origvolume,double rootwt,double groupwt,double wt,uint64_t orderid,uint64_t quoteid)
{
    cJSON *item; double price,volume,oppo = 1.;
    item = cJSON_CreateObject();
    jaddstr(item,"basket",rootbidask == 0 ? "bid":"ask");
    //jaddnum(item,"rootwt",rootwt);
    //jaddnum(item,"groupwt",groupwt);
    //jaddnum(item,"wt",wt);
    if ( wt*groupwt < 0. )
        oppo = -1.;
    if ( wt*groupwt < 0 )
    {
        volume = origprice * origvolume;
        price = 1./origprice;
    } else price = origprice, volume = origvolume;
    jaddnum(item,"price",price);
    jaddnum(item,"volume",volume);
    if ( groupwt*wt < 0 )
    {
        volume = origprice * origvolume;
        price = 1./origprice;
    } else price = origprice, volume = origvolume;
    _prices777_item(item,group,prices,bidask,price,volume,orderid,quoteid);
    return(item);
}

cJSON *prices777_tradeitem(int32_t rootbidask,struct prices777 *prices,int32_t group,int32_t bidask,int32_t slot,uint32_t timestamp,double price,double volume,double rootwt,double groupwt,double wt,uint64_t orderid,uint64_t quoteid)
{
    static uint32_t match,error;
    if ( prices->O.timestamp == timestamp )
    {
        //printf("tradeitem.(%s %f %f)\n",prices->exchange,price,volume);
        if ( bidask == 0 && prices->O.book[MAX_GROUPS][slot].bid.s.price == price && prices->O.book[MAX_GROUPS][slot].bid.s.vol == volume )
            match++;
        else if ( bidask != 0 && prices->O.book[MAX_GROUPS][slot].ask.s.price == price && prices->O.book[MAX_GROUPS][slot].ask.s.vol == volume )
            match++;
    }
    else if ( prices->O2.timestamp == timestamp )
    {
        //printf("2tradeitem.(%s %f %f)\n",prices->exchange,price,volume);
        if ( bidask == 0 && prices->O2.book[MAX_GROUPS][slot].bid.s.price == price && prices->O2.book[MAX_GROUPS][slot].bid.s.vol == volume )
            match++;
        else if ( bidask != 0 && prices->O2.book[MAX_GROUPS][slot].ask.s.price == price && prices->O2.book[MAX_GROUPS][slot].ask.s.vol == volume )
            match++;
    } else error++, printf("mismatched tradeitem error.%d match.%d\n",error,match);
    return(prices777_item(rootbidask,prices,group,bidask,price,volume,rootwt,groupwt,wt,orderid,quoteid));
}

cJSON *prices777_tradesequence(struct prices777 *prices,int32_t bidask,struct prices777_order *orders[],double rootwt,double groupwt,double wt,int32_t refgroup)
{
    int32_t i,j,srcslot,srcbidask,err = 0; cJSON *array; struct prices777_order *suborders[MAX_GROUPS];
    struct prices777_order *order; struct prices777 *src;
    array = cJSON_CreateArray();
    for (i=0; i<prices->numgroups; i++)
    {
        order = orders[i];
        groupwt = prices->groupwts[i];
        memset(suborders,0,sizeof(suborders));
        srcbidask = (order->slot_ba & 1); srcslot = order->slot_ba >> 1;
        if ( (src= order->source) != 0 )
        {
            if ( src->basketsize == 0 )
                jaddi(array,prices777_tradeitem(bidask,src,refgroup*10+i,srcbidask,srcslot,order->s.timestamp,order->s.price,order->ratio*order->s.vol,rootwt,groupwt/groupwt,order->wt,order->id,order->s.quoteid));
            else if ( src->O.timestamp == order->s.timestamp )
            {
                for (j=0; j<src->numgroups; j++)
                    suborders[j] = (srcbidask == 0) ? &src->O.book[j][srcslot].bid : &src->O.book[j][srcslot].ask;
                jaddi(array,prices777_tradesequence(src,bidask,suborders,rootwt,groupwt/groupwt,order->wt,refgroup*10 + i));
            }
            else if ( src->O2.timestamp == order->s.timestamp )
            {
                for (j=0; j<src->numgroups; j++)
                    suborders[j] = (srcbidask == 0) ? &src->O2.book[j][srcslot].bid : &src->O2.book[j][srcslot].ask;
                jaddi(array,prices777_tradesequence(src,bidask,suborders,rootwt,groupwt/groupwt,order->wt,refgroup*10 + i));
            }
            else err =  1;
        }
        if ( src == 0 || err != 0 )
        {
            //jaddi(array,prices777_item(prices,bidask,price,volume,wt,orderid));
            //printf("prices777_tradesequence warning cant match timestamp %u (%s %s/%s)\n",order->timestamp,prices->contract,prices->base,prices->rel);
        }
    }
    return(array);
}

void prices777_orderbook_item(struct prices777 *prices,int32_t bidask,struct prices777_order *suborders[],cJSON *array,int32_t invert,int32_t allflag,double origprice,double origvolume,uint64_t orderid,uint64_t quoteid)
{
    cJSON *item,*obj,*tarray; double price,volume; struct InstantDEX_quote *iQ;
    item = cJSON_CreateObject();
    if ( invert != 0 )
        volume = (origvolume * origprice), price = 1./origprice;
    else price = origprice, volume = origvolume;
    if ( strcmp(prices->exchange,"shuffle") == 0 )
    {
        jaddstr(item,"plugin","shuffle"), jaddstr(item,"method","start");
        jaddnum(item,"dotrade",1), jaddnum(item,"volume",volume);
        jaddstr(item,"base",prices->base);
        if ( (iQ= find_iQ(quoteid)) != 0 )
            jadd64bits(item,"offerNXT",iQ->s.offerNXT);
        jadd64bits(item,"quoteid",iQ->s.quoteid);
        jaddi(array,item);
        return;
    }
    jaddstr(item,"plugin","InstantDEX"), jaddstr(item,"method","tradesequence");
    jaddnum(item,"dotrade",1), jaddnum(item,"price",price), jaddnum(item,"volume",volume);
    //jaddnum(item,"invert",invert), jaddnum(item,"origprice",origprice), jaddnum(item,"origvolume",origvolume);
    //jaddstr(item,"base",prices->base), jaddstr(item,"rel",prices->rel);
    if ( allflag != 0 )
    {
        if ( prices->basketsize == 0 )
        {
            tarray = cJSON_CreateArray();
            obj = cJSON_CreateObject();
            _prices777_item(obj,0,prices,bidask,origprice,origvolume,orderid,quoteid);
            jaddi(tarray,obj);
        } else tarray = prices777_tradesequence(prices,bidask,suborders,invert!=0?-1:1.,1.,1.,0);
        jadd(item,"trades",tarray);
    }
    jaddi(array,item);
}

char *prices777_orderbook_jsonstr(int32_t invert,uint64_t nxt64bits,struct prices777 *prices,struct prices777_basketinfo *OB,int32_t maxdepth,int32_t allflag)
{
    struct prices777_orderentry *gp; struct prices777_order *suborders[MAX_GROUPS]; cJSON *json,*bids,*asks;
    int32_t i,slot; char baserel[64],base[64],rel[64],assetA[64],assetB[64],NXTaddr[64];
    if ( invert == 0 )
        sprintf(baserel,"%s/%s",prices->base,prices->rel);
    else sprintf(baserel,"%s/%s",prices->rel,prices->base);
    if ( Debuglevel > 2 )
        printf("ORDERBOOK %s/%s iQsize.%ld numbids.%d numasks.%d maxdepth.%d (%llu %llu)\n",prices->base,prices->rel,sizeof(struct InstantDEX_quote),OB->numbids,OB->numasks,maxdepth,(long long)prices->baseid,(long long)prices->relid);
    json = cJSON_CreateObject(), bids = cJSON_CreateArray(), asks = cJSON_CreateArray();
    gp = &OB->book[MAX_GROUPS][0];
    memset(suborders,0,sizeof(suborders));
    for (slot=0; (slot<OB->numbids || slot<OB->numasks) && slot<maxdepth; slot++,gp++)
    {
        //printf("slot.%d\n",slot);
        if ( slot < OB->numbids )
        {
            for (i=0; i<prices->numgroups; i++)
                suborders[i] = &OB->book[i][slot].bid;
            prices777_orderbook_item(prices,0,suborders,(invert==0) ? bids : asks,invert,allflag,gp->bid.s.price,gp->bid.s.vol,gp->bid.id,gp->bid.s.quoteid);
        }
        if ( slot < OB->numasks )
        {
            for (i=0; i<prices->numgroups; i++)
                suborders[i] = &OB->book[i][slot].ask;
            prices777_orderbook_item(prices,1,suborders,(invert==0) ? asks : bids,invert,allflag,gp->ask.s.price,gp->ask.s.vol,gp->ask.id,gp->ask.s.quoteid);
        }
    }
    expand_nxt64bits(NXTaddr,nxt64bits);
    if ( invert != 0 )
        strcpy(base,prices->rel), strcpy(rel,prices->base);
    else strcpy(base,prices->base), strcpy(rel,prices->rel);
    expand_nxt64bits(assetA,invert==0 ? prices->baseid : prices->relid);
    expand_nxt64bits(assetB,invert!=0 ? prices->baseid : prices->relid);
    cJSON_AddItemToObject(json,"exchange",cJSON_CreateString(prices->exchange));
    cJSON_AddItemToObject(json,"inverted",cJSON_CreateNumber(invert));
    cJSON_AddItemToObject(json,"contract",cJSON_CreateString(prices->contract));
    cJSON_AddItemToObject(json,"baseid",cJSON_CreateString(assetA));
    if ( assetB != 0 )
        cJSON_AddItemToObject(json,"relid",cJSON_CreateString(assetB));
    cJSON_AddItemToObject(json,"base",cJSON_CreateString(base));
    if ( rel[0] != 0 )
        cJSON_AddItemToObject(json,"rel",cJSON_CreateString(rel));
    cJSON_AddItemToObject(json,"bids",bids);
    cJSON_AddItemToObject(json,"asks",asks);
    if ( invert == 0 )
    {
        cJSON_AddItemToObject(json,"numbids",cJSON_CreateNumber(OB->numbids));
        cJSON_AddItemToObject(json,"numasks",cJSON_CreateNumber(OB->numasks));
        cJSON_AddItemToObject(json,"lastbid",cJSON_CreateNumber(prices->lastbid));
        cJSON_AddItemToObject(json,"lastask",cJSON_CreateNumber(prices->lastask));
    }
    else
    {
        cJSON_AddItemToObject(json,"numbids",cJSON_CreateNumber(OB->numasks));
        cJSON_AddItemToObject(json,"numasks",cJSON_CreateNumber(OB->numbids));
        if ( prices->lastask != 0 )
            cJSON_AddItemToObject(json,"lastbid",cJSON_CreateNumber(1. / prices->lastask));
        if ( prices->lastbid != 0 )
            cJSON_AddItemToObject(json,"lastask",cJSON_CreateNumber(1. / prices->lastbid));
    }
    cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(NXTaddr));
    cJSON_AddItemToObject(json,"timestamp",cJSON_CreateNumber(time(NULL)));
    cJSON_AddItemToObject(json,"maxdepth",cJSON_CreateNumber(maxdepth));
    return(jprint(json,1));
}

void prices777_jsonstrs(struct prices777 *prices,struct prices777_basketinfo *OB)
{
    int32_t allflag; char *strs[4];
    if ( OB->numbids == 0 && OB->numasks == 0 )
    {
        printf("warning: updating null orderbook ignored for %s (%s/%s)\n",prices->contract,prices->base,prices->rel);
        return;
    }
    for (allflag=0; allflag<4; allflag++)
    {
        strs[allflag] = prices777_orderbook_jsonstr(allflag/2,SUPERNET.my64bits,prices,OB,MAX_DEPTH,allflag%2);
        if ( Debuglevel > 2 )
            printf("strs[%d].(%s) prices.%p\n",allflag,strs[allflag],prices);
    }
    portable_mutex_lock(&prices->mutex);
    for (allflag=0; allflag<4; allflag++)
    {
        if ( prices->orderbook_jsonstrs[allflag/2][allflag%2] != 0 )
            free(prices->orderbook_jsonstrs[allflag/2][allflag%2]);
        prices->orderbook_jsonstrs[allflag/2][allflag%2] = strs[allflag];
    }
    portable_mutex_unlock(&prices->mutex);
}

void prices777_json_quotes(double *hblap,struct prices777 *prices,cJSON *bids,cJSON *asks,int32_t maxdepth,char *pricefield,char *volfield,uint32_t reftimestamp)
{
    cJSON *item; int32_t i,slot,n=0,m=0,dir,bidask,numitems; uint64_t orderid,quoteid; uint32_t timestamp; double price,volume,hbla = 0.;
    struct prices777_basketinfo OB; struct prices777_orderentry *gp; struct prices777_order *order;
    memset(&OB,0,sizeof(OB));
    if ( reftimestamp == 0 )
        reftimestamp = (uint32_t)time(NULL);
    OB.timestamp = reftimestamp;
    if ( bids != 0 )
    {
        n = cJSON_GetArraySize(bids);
        if ( maxdepth != 0 && n > maxdepth )
            n = maxdepth;
    }
    if ( asks != 0 )
    {
        m = cJSON_GetArraySize(asks);
        if ( maxdepth != 0 && m > maxdepth )
            m = maxdepth;
    }
    for (i=0; i<n||i<m; i++)
    {
        gp = &OB.book[MAX_GROUPS][i];
        gp->bid.source = gp->ask.source = prices;
        for (bidask=0; bidask<2; bidask++)
        {
            price = volume = 0.;
            orderid = quoteid = 0;
            dir = (bidask == 0) ? 1 : -1;
            if ( bidask == 0 && i >= n )
                continue;
            else if ( bidask == 1 && i >= m )
                continue;
            if ( strcmp(prices->exchange,"bter") == 0 && dir < 0 )
                slot = ((bidask==0?n:m) - 1) - i;
            else slot = i;
            timestamp = 0;
            item = jitem(bidask==0?bids:asks,slot);
            if ( pricefield != 0 && volfield != 0 )
                price = jdouble(item,pricefield), volume = jdouble(item,volfield);
            else if ( is_cJSON_Array(item) != 0 && (numitems= cJSON_GetArraySize(item)) != 0 ) // big assumptions about order within nested array!
            {
                price = jdouble(jitem(item,0),0), volume = jdouble(jitem(item,1),0);
                if ( strcmp(prices->exchange,"kraken") == 0 )
                    timestamp = juint(jitem(item,2),0);
                else orderid = j64bits(jitem(item,2),0);
            }
            else continue;
            if ( quoteid == 0 )
                quoteid = orderid;
            if ( price > SMALLVAL && volume > SMALLVAL )
            {
                if ( prices->commission != 0. )
                {
                    //printf("price %f fee %f -> ",price,prices->commission * price);
                    if ( bidask == 0 )
                        price -= prices->commission * price;
                    else price += prices->commission * price;
                    //printf("%f\n",price);
                }
                order = (bidask == 0) ? &gp->bid : &gp->ask;
                order->s.price = price, order->s.vol = volume, order->source = prices, order->s.timestamp = OB.timestamp, order->wt = 1, order->id = orderid, order->s.quoteid = quoteid;
                if ( bidask == 0 )
                    order->slot_ba = (OB.numbids++ << 1);
                else order->slot_ba = (OB.numasks++ << 1) | 1;
                if ( i == 0 )
                {
                    if ( bidask == 0 )
                        prices->lastbid = price;
                    else prices->lastask = price;
                    if ( hbla == 0. )
                        hbla = price;
                    else hbla = 0.5 * (hbla + price);
                }
                if ( Debuglevel > 2 || prices->basketsize > 0 || strcmp("unconf",prices->exchange) == 0 )
                    printf("%d,%d: %-8s %s %5s/%-5s %13.8f vol %13.8f | invert %13.8f vol %13.8f | timestamp.%u\n",OB.numbids,OB.numasks,prices->exchange,dir>0?"bid":"ask",prices->base,prices->rel,price,volume,1./price,volume*price,timestamp);
            }
        }
    }
    if ( hbla != 0. )
        *hblap = hbla;
    prices->O2 = prices->O;
    //prices->O = OB;
    for (i=0; i<MAX_GROUPS; i++)
        memcpy(prices->O.book[i],prices->O.book[i+1],sizeof(prices->O.book[i]));
    memcpy(prices->O.book[MAX_GROUPS],OB.book[MAX_GROUPS],sizeof(OB.book[MAX_GROUPS]));
    prices->O.numbids = OB.numbids, prices->O.numasks = OB.numasks, prices->O.timestamp = OB.timestamp;
}

double prices777_json_orderbook(char *exchangestr,struct prices777 *prices,int32_t maxdepth,cJSON *json,char *resultfield,char *bidfield,char *askfield,char *pricefield,char *volfield)
{
    cJSON *obj = 0,*bidobj=0,*askobj=0; double hbla = 0.; int32_t numasks=0,numbids=0;
    if ( resultfield == 0 )
        obj = json;
    if ( maxdepth == 0 )
        maxdepth = MAX_DEPTH;
    if ( resultfield == 0 || (obj= jobj(json,resultfield)) != 0 )
    {
        bidobj = jarray(&numbids,obj,bidfield);
        askobj = jarray(&numasks,obj,askfield);
        if ( bidobj != 0 || askobj != 0 )
        {
            prices777_json_quotes(&hbla,prices,bidobj,askobj,maxdepth,pricefield,volfield,0);
            prices777_jsonstrs(prices,&prices->O);
        }
    }
    return(hbla);
}

void prices777_hbla(uint64_t *bidorderid,uint64_t *askorderid,int32_t *lowaski,int32_t *highbidi,double *highbid,double *bidvol,double *lowask,double *askvol,double groupwt,int32_t i,int32_t bidask,double price,double vol,uint64_t orderid)
{
    if ( groupwt > SMALLVAL && (*lowask == 0. || price < *lowask) )
        *askorderid = orderid, *lowask = price, *askvol = vol, *lowaski = (i << 1) | bidask;
    else if ( groupwt < -SMALLVAL && (*highbid == 0. || price > *highbid) )
        *bidorderid = orderid, *highbid = price, *bidvol = vol, *highbidi = (i << 1) | bidask;
    //printf("hbla.(%f %f)\n",price,vol);
}

void prices777_setorder(struct prices777_order *order,struct prices777_basket *group,int32_t coordinate,uint64_t orderid,double refprice,double refvol)
{
    int32_t bidask; struct prices777 *prices; double price=0,vol=0;
    bidask = (coordinate & 1), coordinate >>= 1;
    prices = group[coordinate].prices;
    if ( bidask != 0 && group[coordinate].aski < prices->O.numasks )
        price = prices->O.book[MAX_GROUPS][group[coordinate].aski].ask.s.price, vol = prices->O.book[MAX_GROUPS][group[coordinate].aski].ask.s.vol, order->slot_ba = (group[coordinate].aski++ << 1) | 1;
    else if ( group[coordinate].bidi < prices->O.numbids )
        price = prices->O.book[MAX_GROUPS][group[coordinate].bidi].bid.s.price, vol = prices->O.book[MAX_GROUPS][group[coordinate].bidi].bid.s.vol,order->slot_ba = (group[coordinate].bidi++ << 1);
    else printf("illegal coordinate.%d bidask.%d when bidi.%d aski.%d numbids.%d numasks.%d\n",coordinate,bidask,group[coordinate].bidi,group[coordinate].aski,prices->O.numbids,prices->O.numasks);
    order->source = prices;
    order->wt = group[coordinate].wt;
    order->s.timestamp = prices->O.timestamp;
    order->id = orderid;
    if ( order->wt < 0. )
        vol *= price, price = (1. / price);
    if ( fabs(price - refprice) > SMALLVAL || fabs(vol - refvol) > SMALLVAL )
    {
        printf("[ERROR] ");
        printf("%s group.%d (%s/%s) bidask.%d coordinate.%d wt %f (%f %f) vs (%f %f)\n",prices->exchange,group[coordinate].groupid,prices->base,prices->rel,bidask,coordinate,order->wt,price,vol,refprice,refvol);
    }
}


int32_t prices777_groupbidasks(struct prices777_orderentry *gp,double groupwt,double minvol,struct prices777_basket *group,int32_t groupsize)
{
    int32_t i,highbidi,lowaski; double highbid,lowask,bidvol,askvol,vol,price,polarity; uint64_t bidorderid,askorderid;
    struct prices777 *feature; struct prices777_order *order;
    memset(gp,0,sizeof(*gp));
    highbidi = lowaski = -1;
    for (bidvol=askvol=highbid=lowask=bidorderid=askorderid=i=0; i<groupsize; i++)
    {
        if ( (feature= group[i].prices) != 0 )
        {
            //if ( strcmp(feature->base,group[0].rel) == 0 && strcmp(feature->rel,group[0].base) == 0 )
            //    polarity = -1.;
            //else polarity = 1.;
            //if ( group[i].wt * groupwt < 0 ) fixes supernet/BTC
            //   polarity *= -1;
            polarity = group[i].wt;// * groupwt;
            order = &feature->O.book[MAX_GROUPS][group[i].bidi].bid;
            if ( group[i].bidi < feature->O.numbids && (vol= order->s.vol) > minvol && (price= order->s.price) > SMALLVAL )
            {
                //printf("%d/%d: (%s/%s) %s bidi.%d price.%f polarity.%f groupwt.%f -> ",i,groupsize,feature->base,feature->rel,feature->exchange,group[i].bidi,price,polarity,groupwt);
                if ( polarity < 0. )
                    vol *= price, price = (1. / price);
                prices777_hbla(&bidorderid,&askorderid,&lowaski,&highbidi,&highbid,&bidvol,&lowask,&askvol,-polarity,i,0,price,vol,order->id);
            }
            order = &feature->O.book[MAX_GROUPS][group[i].aski].ask;
            if ( group[i].aski < feature->O.numasks && (vol= order->s.vol) > minvol && (price= order->s.price) > SMALLVAL )
            {
                //printf("%d/%d: (%s/%s) %s aski.%d price.%f polarity.%f groupwt.%f -> ",i,groupsize,feature->base,feature->rel,feature->exchange,group[i].aski,price,polarity,groupwt);
                if ( polarity < 0. )
                    vol *= price, price = (1. / price);
                prices777_hbla(&bidorderid,&askorderid,&lowaski,&highbidi,&highbid,&bidvol,&lowask,&askvol,polarity,i,1,price,vol,order->id);
            }
        } else printf("null feature.%p\n",feature);
    }
    gp->bid.s.price = highbid, gp->bid.s.vol = bidvol, gp->ask.s.price = lowask, gp->ask.s.vol = askvol;
    if ( highbidi >= 0 )
        prices777_setorder(&gp->bid,group,highbidi,bidorderid,highbid,bidvol);
    if ( lowaski >= 0 )
        prices777_setorder(&gp->ask,group,lowaski,askorderid,lowask,askvol);
 // if ( lowaski >= 0 && highbidi >= 0 )
//printf("groupwt %f groupsize.%d %s highbidi.%d %f %f %s lowaski.%d wts.(%f %f)\n",groupwt,groupsize,gp->bid.source->exchange,highbidi,gp->bid.s.price,gp->ask.s.price,gp->ask.source->exchange,lowaski,gp->bid.wt,gp->ask.wt);
    if ( gp->bid.s.price > SMALLVAL && gp->ask.s.price > SMALLVAL )
        return(0);
    return(-1);
}

double prices777_volcalc(double *basevols,uint64_t *baseids,uint64_t baseid,double basevol)
{
    int32_t i;
    //printf("(add %llu %f) ",(long long)baseid,basevol);
    for (i=0; i<MAX_GROUPS*2; i++)
    {
        if ( baseids[i] == baseid )
        {
            if ( basevols[i] == 0. || basevol < basevols[i] )
                basevols[i] = basevol;//, printf("set %llu <= %f ",(long long)baseid,basevol);
           // else printf("missed basevols[%d] %f, ",i,basevols[i]);
            break;
        }
    }
    return(1);
}

double prices777_volratio(double *basevols,uint64_t *baseids,uint64_t baseid,double vol)
{
    int32_t i;
    for (i=0; i<MAX_GROUPS*2; i++)
    {
        if ( baseids[i] == baseid )
        {
            if ( basevols[i] > 0. )
            {
                //printf("(vol %f vs %f) ",vol,basevols[i]);
                if ( vol > basevols[i] )
                    return(basevols[i]/vol);
                else return(1.);
            }
            printf("unexpected zero vol basevols.%d\n",i);
            return(1.);
            break;
        }
    }
    printf("unexpected cant find baseid.%llu\n",(long long)baseid);
    return(1.);
}

double prices777_basket(struct prices777 *prices,int32_t maxdepth)
{
    int32_t i,j,groupsize,slot; uint64_t baseids[MAX_GROUPS*2];
    double basevols[MAX_GROUPS*2],relvols[MAX_GROUPS*2],baseratio,relratio,a,av,b,bv,gap,bid,ask,minvol,bidvol,askvol,hbla = 0.;
    struct prices777_basketinfo OB; uint32_t timestamp; struct prices777 *feature; struct prices777_orderentry *gp;
    timestamp = (uint32_t)time(NULL);
    memset(&OB,0,sizeof(OB));
    memset(baseids,0,sizeof(baseids));
    OB.timestamp = timestamp;
    //printf("prices777_basket.(%s) %s (%s/%s) %llu/%llu basketsize.%d\n",prices->exchange,prices->contract,prices->base,prices->rel,(long long)prices->baseid,(long long)prices->relid,prices->basketsize);
    for (i=0; i<prices->basketsize; i++)
    {
        if ( 0 && strcmp(prices->exchange,"active") == 0 && prices->basket[i].prices != 0 )
            printf("%s.group.%d %10s %10s/%10s wt %3.0f %.8f %.8f\n",prices->exchange,i,prices->basket[i].prices->exchange,prices->basket[i].prices->base,prices->basket[i].rel,prices->basket[i].wt,prices->basket[i].prices->O.book[MAX_GROUPS][0].bid.s.price,prices->basket[i].prices->O.book[MAX_GROUPS][0].ask.s.price);
        if ( (feature= prices->basket[i].prices) != 0 )
        {
            if ( 0 && (gap= (prices->lastupdate - feature->lastupdate)) < 0 )
            {
                if ( prices->lastupdate != 0 )
                    printf("you can ignore this harmless warning about unexpected time traveling feature %f vs %f or laggy feature\n",prices->lastupdate,feature->lastupdate);
                return(0.);
            }
        }
        else
        {
            printf("unexpected null basket item %s[%d]\n",prices->contract,i);
            return(0.);
        }
        prices->basket[i].aski = prices->basket[i].bidi = 0;
        for (j=0; j<MAX_GROUPS*2; j++)
        {
            if ( prices->basket[i].prices == 0 || prices->basket[i].prices->baseid == baseids[j] )
                break;
            if ( baseids[j] == 0 )
            {
                baseids[j] = prices->basket[i].prices->baseid;
                break;
            }
        }
        for (j=0; j<MAX_GROUPS*2; j++)
        {
            if ( prices->basket[i].prices == 0 || prices->basket[i].prices->relid == baseids[j] )
                break;
            if ( baseids[j] == 0 )
            {
                baseids[j] = prices->basket[i].prices->relid;
                break;
            }
        }
    }
    //printf("%s basketsize.%d numgroups.%d maxdepth.%d group0size.%d\n",prices->contract,prices->basketsize,prices->numgroups,maxdepth,prices->basket[0].groupsize);
    for (slot=0; slot<maxdepth; slot++)
    {
        memset(basevols,0,sizeof(basevols));
        memset(relvols,0,sizeof(relvols));
        groupsize = prices->basket[0].groupsize;
        minvol = (1. / SATOSHIDEN);
        bid = ask = 1.; bidvol = askvol = 0.;
        for (j=i=0; j<prices->numgroups; j++,i+=groupsize)
        {
            groupsize = prices->basket[i].groupsize;
            gp = &OB.book[j][slot];
            if ( prices777_groupbidasks(gp,prices->groupwts[j],minvol,&prices->basket[i],groupsize) != 0 )
            {
                //printf("prices777_groupbidasks i.%d j.%d error\n",i,j);
                break;
            }
            //printf("%s j%d slot.%d %s numgroups.%d groupsize.%d\n",prices->exchange,j,slot,prices->contract,prices->numgroups,groupsize);
            if ( bid > SMALLVAL && (b= gp->bid.s.price) > SMALLVAL && (bv= gp->bid.s.vol) > SMALLVAL )
            {
                //if ( gp->bid.wt*prices->groupwts[j] < 0 )
                //    bid /= b;
                //else
                    bid *= b;
                prices777_volcalc(basevols,baseids,gp->bid.source->baseid,bv);
                prices777_volcalc(basevols,baseids,gp->bid.source->relid,b*bv);
                //printf("bid %f b %f bv %f %s %s %f\n",bid,b,bv,gp->bid.source->base,gp->bid.source->rel,bv*b);
            } else bid = 0.;
            if ( ask > SMALLVAL && (a= gp->ask.s.price) > SMALLVAL && (av= gp->ask.s.vol) > SMALLVAL )
            {
                //if ( gp->ask.wt*prices->groupwts[j] < 0 )
                //    ask /= a;
                //else
                    ask *= a;
                prices777_volcalc(relvols,baseids,gp->ask.source->baseid,av);
                prices777_volcalc(relvols,baseids,gp->ask.source->relid,a*av);
                //printf("ask %f b %f bv %f %s %s %f\n",ask,a,av,gp->ask.source->base,gp->ask.source->rel,av*a);
            } else ask = 0.;
            if ( Debuglevel > 2 )
                printf("%10s %10s/%10s %s (%s %s) wt:%f %2.0f/%2.0f j.%d: b %.8f %12.6f a %.8f %12.6f, bid %.8f ask %.8f inv %f %f\n",prices->exchange,gp->bid.source->exchange,gp->ask.source->exchange,prices->contract,gp->bid.source->contract,gp->ask.source->contract,prices->groupwts[j],gp->bid.wt,gp->ask.wt,j,b,bv,a,av,bid,ask,1/bid,1/ask);
        }
        for (j=0; j<prices->numgroups; j++)
        {
            gp = &OB.book[j][slot];
            if ( gp->bid.source == 0 || gp->ask.source == 0 )
            {
                //printf("%s: null source slot.%d j.%d\n",prices->exchange,slot,j);
                break;
            }
            baseratio = prices777_volratio(basevols,baseids,gp->bid.source->baseid,gp->bid.s.vol);
            relratio = prices777_volratio(basevols,baseids,gp->bid.source->relid,gp->bid.s.vol * gp->bid.s.price);
            gp->bid.ratio = (baseratio < relratio) ? baseratio : relratio;
            if ( j == 0 )
                bidvol = (gp->bid.ratio * gp->bid.s.vol);
            //printf("(%f %f) (%f %f) bid%d bidratio %f bidvol %f ",gp->bid.s.vol,baseratio,gp->bid.s.vol * gp->bid.s.price,relratio,j,gp->bid.ratio,bidvol);
            baseratio = prices777_volratio(relvols,baseids,gp->ask.source->baseid,gp->ask.s.vol);
            relratio = prices777_volratio(relvols,baseids,gp->ask.source->relid,gp->ask.s.vol * gp->ask.s.price);
            gp->ask.ratio = (baseratio < relratio) ? baseratio : relratio;
            if ( j == 0 )
                askvol = (gp->ask.ratio * gp->ask.s.vol);
        }
        if ( j != prices->numgroups )
        {
            //printf("%s: j.%d != numgroups.%d\n",prices->exchange,j,prices->numgroups);
            break;
        }
        for (j=0; j<MAX_GROUPS*2; j++)
        {
            if ( baseids[j] == 0 )
                break;
            //printf("{%llu %f %f} ",(long long)baseids[j],basevols[j],relvols[j]);
        }
        //printf("basevols bidvol %f, askvol %f\n",bidvol,askvol);
        gp = &OB.book[MAX_GROUPS][slot];
        if ( bid > SMALLVAL && bidvol > SMALLVAL )
        {
            if ( slot == 0 )
                prices->lastbid = bid;
            gp->bid.s.timestamp = OB.timestamp, gp->bid.s.price = bid, gp->bid.s.vol = bidvol, gp->bid.slot_ba = (OB.numbids++ << 1);
            gp->bid.source = prices, gp->bid.wt = prices->groupwts[j];
        }
        if ( ask > SMALLVAL && askvol > SMALLVAL )
        {
            if ( slot == 0 )
                prices->lastask = ask;
            gp->ask.s.timestamp = OB.timestamp, gp->ask.s.price = ask, gp->ask.s.vol = askvol, gp->ask.slot_ba = (OB.numasks++ << 1) | 1;
            gp->ask.source = prices, gp->ask.wt = prices->groupwts[j];
        }
        //printf("%s %s slot.%d (%.8f %.6f %.8f %.6f) (%d %d)\n",prices->exchange,prices->contract,slot,gp->bid.s.price,gp->bid.s.vol,gp->ask.s.price,gp->ask.s.vol,OB.numbids,OB.numasks);
    }
    //fprintf(stderr,"%s basket.%s slot.%d numbids.%d numasks.%d %f %f\n",prices->exchange,prices->contract,slot,prices->O.numbids,prices->O.numasks,prices->O.book[MAX_GROUPS][0].bid.s.price,prices->O.book[MAX_GROUPS][0].ask.s.price);
    if ( slot > 0 )
    {
        prices->O2 = prices->O;
        prices->O = OB;
        if ( prices->lastbid > SMALLVAL && prices->lastask > SMALLVAL )
            hbla = 0.5 * (prices->lastbid + prices->lastask);
    }
    return(hbla);
}

struct prices777 *prices777_addbundle(int32_t *validp,int32_t loadprices,struct prices777 *prices,char *exchangestr,uint64_t baseid,uint64_t relid)
{
    int32_t j; struct prices777 *ptr; struct exchange_info *exchange;
    *validp = -1;
    if ( prices != 0 )
    {
        exchangestr = prices->exchange;
        baseid = prices->baseid, relid = prices->relid;
    }
    for (j=0; j<BUNDLE.num; j++)
    {
        if ( (ptr= BUNDLE.ptrs[j]) != 0 && ((ptr->baseid == baseid && ptr->relid == relid) || (ptr->relid == baseid && ptr->baseid == relid)) && strcmp(ptr->exchange,exchangestr) == 0 )
            return(ptr);
    }
    if ( j == BUNDLE.num )
    {
        if ( prices != 0 )
        {
            exchange = &Exchanges[prices->exchangeid];
            if ( loadprices != 0 && exchange->updatefunc != 0 )
            {
                portable_mutex_lock(&exchange->mutex);
                (exchange->updatefunc)(prices,MAX_DEPTH);
                portable_mutex_unlock(&exchange->mutex);
            }
            printf("total polling.%d added.(%s)\n",BUNDLE.num,prices->contract);
            if ( exchange->polling == 0 )
            {
                printf("First pair for (%s), start polling]\n",exchange_str(prices->exchangeid));
                exchange->polling = 1;
                if ( strcmp(exchange->name,"wallet") != 0 )
                    portable_thread_create((void *)prices777_exchangeloop,&Exchanges[prices->exchangeid]);
            }
            BUNDLE.ptrs[BUNDLE.num] = prices;
            printf("prices777_addbundle.(%s) (%s/%s).%s %llu %llu\n",prices->contract,prices->base,prices->rel,prices->exchange,(long long)prices->baseid,(long long)prices->relid);
            BUNDLE.num++;
        }
        *validp = BUNDLE.num;
        return(prices);
    }
    return(0);
}

int32_t create_basketitem(struct prices777_basket *basketitem,cJSON *item,char *refbase,char *refrel,int32_t basketsize)
{
    struct destbuf exchangestr,name,base,rel; char key[512]; uint64_t tmp,baseid,relid; int32_t groupid,keysize,valid; double wt; struct prices777 *prices;
    copy_cJSON(&exchangestr,jobj(item,"exchange"));
    if ( strcmp("shuffle",exchangestr.buf) == 0 || exchange_find(exchangestr.buf) == 0 )
    {
        printf("create_basketitem: illegal exchange.%s\n",exchangestr.buf);
        return(-1);
    }
    copy_cJSON(&name,jobj(item,"name"));
    copy_cJSON(&base,jobj(item,"base"));
    copy_cJSON(&rel,jobj(item,"rel"));
    if ( (baseid= j64bits(item,"baseid")) != 0 && base.buf[0] == 0 )
    {
        _set_assetname(&tmp,base.buf,0,baseid);
        //printf("GOT.(%s) <- %llu\n",base.buf,(long long)baseid);
    }
    else if ( baseid == 0 )
        baseid = stringbits(base.buf);
    if ( (relid= j64bits(item,"relid")) != 0 && rel.buf[0] == 0 )
    {
        _set_assetname(&tmp,rel.buf,0,relid);
        //printf("GOT.(%s) <- %llu\n",rel.buf,(long long)relid);
    }
    else if ( relid == 0 )
        relid = stringbits(rel.buf);
    groupid = juint(item,"group");
    wt = jdouble(item,"wt");
    if ( wt == 0. )
        wt = 1.;
    if ( strcmp(refbase,rel.buf) == 0 || strcmp(refrel,base.buf) == 0 )
    {
        if ( wt != -1 )
        {
            printf("need to flip wt %f for (%s/%s) ref.(%s/%s)\n",wt,base.buf,rel.buf,refbase,refrel);
            wt = -1.;
        }
    }
    else if ( wt != 1. )
    {
        printf("need to flip wt %f for (%s/%s) ref.(%s/%s)\n",wt,base.buf,rel.buf,refbase,refrel);
        wt = 1.;
    }
    if ( name.buf[0] == 0 )
        sprintf(name.buf,"%s/%s",base.buf,rel.buf);
    if ( base.buf[0] == 0 )
        strcpy(base.buf,refbase);
    if ( rel.buf[0] == 0 )
        strcpy(rel.buf,refrel);
    InstantDEX_name(key,&keysize,exchangestr.buf,name.buf,base.buf,&baseid,rel.buf,&relid);
    printf(">>>>>>>>>> create basketitem.(%s) name.(%s) %s (%s/%s) %llu/%llu wt %f\n",jprint(item,0),name.buf,exchangestr.buf,base.buf,rel.buf,(long long)baseid,(long long)relid,wt);
    if ( (prices= prices777_initpair(1,0,exchangestr.buf,base.buf,rel.buf,0.,name.buf,baseid,relid,basketsize)) != 0 )
    {
        prices777_addbundle(&valid,0,prices,0,0,0);
        basketitem->prices = prices;
        basketitem->wt = wt;
        basketitem->groupid = groupid;
        strcpy(basketitem->base,base.buf);
        strcpy(basketitem->rel,rel.buf);
        return(0);
    } else printf("couldnt create basketitem\n");
    return(-1);
}

struct prices777 *prices777_makebasket(char *basketstr,cJSON *_basketjson,int32_t addbasket,char *typestr,struct prices777 *ptrs[],int32_t num)
{
    //{"name":"NXT/BTC","base":"NXT","rel":"BTC","basket":[{"exchange":"poloniex"},{"exchange":"btc38"}]}
    int32_t i,j,n,keysize,valid,basketsize,total = 0; struct destbuf refname,refbase,refrel; char key[8192]; uint64_t refbaseid=0,refrelid=0;
    struct prices777_basket *basketitem,*basket = 0; cJSON *basketjson,*array,*item; struct prices777 *prices = 0;
    if ( (basketjson= _basketjson) == 0 && (basketjson= cJSON_Parse(basketstr)) == 0 )
    {
        printf("cant parse basketstr.(%s)\n",basketstr);
        return(0);
    }
    copy_cJSON(&refname,jobj(basketjson,"name"));
    copy_cJSON(&refbase,jobj(basketjson,"base"));
    copy_cJSON(&refrel,jobj(basketjson,"rel"));
    refbaseid = j64bits(basketjson,"baseid");
    refrelid = j64bits(basketjson,"relid");
    if ( (array= jarray(&n,basketjson,"basket")) != 0 )
    {
        printf("MAKE/(%s) n.%d num.%d\n",jprint(basketjson,0),n,num);
        basketsize = (n + num);
        basket = calloc(1,sizeof(*basket) * basketsize);
        for (i=0; i<n; i++)
        {
            item = jitem(array,i);
            if ( create_basketitem(&basket[total],item,refbase.buf,refrel.buf,basketsize) < 0 )
                printf("warning: >>>>>>>>>>>> skipped create_basketitem %d of %d of %d\n",i,n,basketsize);
            else
            {
                printf("MAKE.%d: (%s) %p.%s\n",total,jprint(item,0),basket[total].prices,basket[total].prices->exchange);
                total++;
            }
        }
        if ( ptrs != 0 && num > 0 )
        {
            for (i=0; i<num; i++)
            {
                basketitem = &basket[total];
                j = 0;
                if ( total > 0 )
                {
                    for (j=0; j<n+i; j++)
                    {
                        if ( basket[j].prices == ptrs[i] )
                        {
                            printf("skip duplicate basket[%d] == ptrs[%d]\n",j,i);
                            break;
                        }
                    }
                }
                if ( j == n+i )
                {
                    basketitem->prices = ptrs[i];
                    if ( strcmp(refbase.buf,ptrs[i]->rel) == 0 || strcmp(refrel.buf,ptrs[i]->base) == 0 )
                        basketitem->wt = -1;
                    else basketitem->wt = 1;
                    basketitem->groupid = 0;
                    strcpy(basketitem->base,ptrs[i]->base);
                    strcpy(basketitem->rel,ptrs[i]->rel);
                    total++;
                    printf("extrai.%d/%d total.%d wt.%f (%s/%s).%s\n",i,num,total,basketitem->wt,ptrs[i]->base,ptrs[i]->rel,ptrs[i]->exchange);
                }
            }
        }
        printf(">>>>> addbasket.%d (%s/%s).%s %llu %llu\n",addbasket,refbase.buf,refrel.buf,typestr,(long long)refbaseid,(long long)refrelid);
        InstantDEX_name(key,&keysize,typestr,refname.buf,refbase.buf,&refbaseid,refrel.buf,&refrelid);
        printf(">>>>> addbasket.%d (%s/%s).%s %llu %llu\n",addbasket,refbase.buf,refrel.buf,typestr,(long long)refbaseid,(long long)refrelid);
        if ( addbasket != 0 )
        {
            prices777_addbundle(&valid,0,0,typestr,refbaseid,refrelid);
            printf("<<<<< created.%s valid.%d refname.(%s) (%s/%s).%s %llu %llu\n",typestr,valid,refname.buf,refbase.buf,refrel.buf,typestr,(long long)refbaseid,(long long)refrelid);
        } else valid = 0;
        if ( valid >= 0 )
        {
            if ( (prices= prices777_createbasket(addbasket,refname.buf,refbase.buf,refrel.buf,refbaseid,refrelid,basket,total,typestr)) != 0 )
            {
                if ( addbasket != 0 )
                    BUNDLE.ptrs[BUNDLE.num] = prices;
                prices->lastprice = prices777_basket(prices,MAX_DEPTH);
                //printf("C.bsize.%d total polling.%d added.(%s/%s).%s updating basket lastprice %f changed.%p %d groupsize.%d numgroups.%d %p\n",total,BUNDLE.num,prices->base,prices->rel,prices->exchange,prices->lastprice,&prices->changed,prices->changed,prices->basket[0].groupsize,prices->numgroups,&prices->basket[0].groupsize);
                BUNDLE.num++;
            }
        } else prices = 0;
        if ( basketjson != _basketjson )
            free_json(basketjson);
        free(basket);
    }
    return(prices);
}

cJSON *inner_json(double price,double vol,uint32_t timestamp,uint64_t quoteid,uint64_t nxt64bits,uint64_t qty,uint64_t pqt,uint64_t baseamount,uint64_t relamount)
{
    cJSON *inner = cJSON_CreateArray();
    char numstr[64];
    sprintf(numstr,"%.8f",price), cJSON_AddItemToArray(inner,cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",vol), cJSON_AddItemToArray(inner,cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)quoteid), cJSON_AddItemToArray(inner,cJSON_CreateString(numstr));
    cJSON_AddItemToArray(inner,cJSON_CreateNumber(timestamp));
    sprintf(numstr,"%llu",(long long)nxt64bits), cJSON_AddItemToArray(inner,cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)qty), cJSON_AddItemToArray(inner,cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)pqt), cJSON_AddItemToArray(inner,cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)baseamount), cJSON_AddItemToArray(inner,cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)relamount), cJSON_AddItemToArray(inner,cJSON_CreateString(numstr));
   // printf("(%s) ",jprint(inner,0));
    return(inner);
}

double prices777_NXT(struct prices777 *prices,int32_t maxdepth)
{
    uint32_t timestamp; int32_t flip,i,n; uint64_t baseamount,relamount,qty,pqt; char url[1024],*str,*cmd,*field;
    cJSON *json,*bids,*asks,*srcobj,*item,*array; double price,vol,hbla = 0.;
    if ( NXT_ASSETID != stringbits("NXT") || (strcmp(prices->rel,"NXT") != 0 && strcmp(prices->rel,"5527630") != 0) )
    {
        printf("NXT_ASSETID.%llu != %llu stringbits rel.%s\n",(long long)NXT_ASSETID,(long long)stringbits("NXT"),prices->rel);//, getchar();
        return(0);
    }
    bids = cJSON_CreateArray(), asks = cJSON_CreateArray();
    for (flip=0; flip<2; flip++)
    {
        /*{
            "offer": "16959774565785265980",
            "expirationHeight": 1000000,
            "accountRS": "NXT-QFAF-GR4F-RBSR-AXW2G",
            "limit": "9000000",
            "currency": "5775213290661997199",
            "supply": "0",
            "account": "9728792749189838093",
            "height": 348856,
            "rateNQT": "650"
        }*/
        if ( prices->type != 5 )
        {
            if ( flip == 0 )
                cmd = "getBidOrders", field = "bidOrders", array = bids;
            else cmd = "getAskOrders", field = "askOrders", array = asks;
            sprintf(url,"requestType=%s&asset=%llu&limit=%d",cmd,(long long)prices->baseid,maxdepth);
        }
        else
        {
            if ( flip == 0 )
                cmd = "getBuyOffers", field = "offers", array = bids;
            else cmd = "getSellOffers", field = "offers", array = asks;
            sprintf(url,"requestType=%s&currency=%llu&limit=%d",cmd,(long long)prices->baseid,maxdepth);
        }
        if ( (str= issue_NXTPOST(url)) != 0 )
        {
            //printf("{%s}\n",str);
            if ( (json= cJSON_Parse(str)) != 0 )
            {
                if ( (srcobj= jarray(&n,json,field)) != 0 )
                {
                    for (i=0; i<n && i<maxdepth; i++)
                    {
                        /*
                         "quantityQNT": "79",
                         "priceNQT": "13499000000",
                         "transactionHeight": 480173,
                         "accountRS": "NXT-FJQN-8QL2-BMY3-64VLK",
                         "transactionIndex": 1,
                         "asset": "15344649963748848799",
                         "type": "ask",
                         "account": "5245394173527769812",
                         "order": "17926122097022414596",
                         "height": 480173
                         */
                        item = cJSON_GetArrayItem(srcobj,i);
                        if ( prices->type != 5 )
                            qty = j64bits(item,"quantityQNT"), pqt = j64bits(item,"priceNQT");
                        else qty = j64bits(item,"limit"), pqt = j64bits(item,"rateNQT");
                        baseamount = (qty * prices->ap_mult), relamount = (qty * pqt);
                        price = prices777_price_volume(&vol,baseamount,relamount);
                        if ( i == 0  )
                        {
                            hbla = (hbla == 0.) ? price : 0.5 * (price + hbla);
                            if ( flip == 0 )
                                prices->lastbid = price;
                            else prices->lastask = price;
                        }
                        //printf("(%llu %llu) %f %f mult.%llu qty.%llu pqt.%llu baseamount.%lld relamount.%lld\n",(long long)prices->baseid,(long long)prices->relid,price,vol,(long long)prices->ap_mult,(long long)qty,(long long)pqt,(long long)baseamount,(long long)relamount);
                        timestamp = get_blockutime(juint(item,"height"));
                        item = inner_json(price,vol,timestamp,j64bits(item,prices->type != 5 ? "order" : "offer"),j64bits(item,"account"),qty,pqt,baseamount,relamount);
                        cJSON_AddItemToArray(array,item);
                    }
                }
                free_json(json);
            }
            free(str);
        } else printf("cant get.(%s)\n",url);
    }
    json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"bids",bids);
    cJSON_AddItemToObject(json,"asks",asks);
    if ( Debuglevel > 2 )
        printf("NXTAE.(%s)\n",jprint(json,0));
    prices777_json_orderbook("nxtae",prices,maxdepth,json,0,"bids","asks",0,0);
    free_json(json);
    return(hbla);
}

double prices777_unconfNXT(struct prices777 *prices,int32_t maxdepth)
{
    struct destbuf account,txidstr,comment,recipient; char url[1024],*str; uint32_t timestamp; int32_t type,i,subtype,n;
    cJSON *json,*bids,*asks,*array,*txobj,*attachment;
    double price,vol; uint64_t assetid,accountid,quoteid,baseamount,relamount,qty,priceNQT,amount;
    bids = cJSON_CreateArray(), asks = cJSON_CreateArray();
    prices->lastbid = prices->lastask = 0.;
    prices->O.numbids = prices->O.numasks = 0;
    sprintf(url,"requestType=getUnconfirmedTransactions");
    if ( (str= issue_NXTPOST(url)) != 0 )
    {
        //printf("{%s}\n",str);
        if ( (json= cJSON_Parse(str)) != 0 )
        {
            if ( (array= jarray(&n,json,"unconfirmedTransactions")) != 0 )
            {
                for (i=0; i<n; i++)
                {
      //{"senderPublicKey":"45c9266036e705a9559ccbd2b2c92b28ea6363d2723e8d42433b1dfaa421066c","signature":"9d6cefff4c67f8cf4e9487122e5e6b1b65725815127063df52e9061036e78c0b49ba38dbfc12f03c158697f0af5811ce9398702c4acb008323df37dc55c1b43d","feeNQT":"100000000","type":2,"fullHash":"6a2cd914b9d4a5d8ebfaecaba94ef4e7d2b681c236a4bee56023aafcecd9b704","version":1,"phased":false,"ecBlockId":"887016880740444200","signatureHash":"ba8eee4beba8edbb6973df4243a94813239bf57b91cac744cb8d6a5d032d5257","attachment":{"quantityQNT":"50","priceNQT":"18503000003","asset":"13634675574519917918","version.BidOrderPlacement":1},"senderRS":"NXT-FJQN-8QL2-BMY3-64VLK","subtype":3,"amountNQT":"0","sender":"5245394173527769812","ecBlockHeight":495983,"deadline":1440,"transaction":"15611117574733507690","timestamp":54136768,"height":2147483647},{"senderPublicKey":"c42956d0a9abc5a2e455e69c7e65ff9a53de2b697e913b25fcb06791f127af06","signature":"ca2c3f8e32d3aa003692fef423193053c751235a25eb5b67c21aefdeb7a41d0d37bc084bd2e33461606e25f09ced02d1e061420da7e688306e76de4d4cf90ae0","feeNQT":"100000000","type":2,"fullHash":"51c04de7106a5d5a2895db05305b53dd33fa8b9935d549f765aa829a23c68a6b","version":1,"phased":false,"ecBlockId":"887016880740444200","signatureHash":"d76fce4c081adc29f7e60eba2a930ab5050dd79b6a1355fae04863dddf63730c","attachment":{"version.AskOrderPlacement":1,"quantityQNT":"11570","priceNQT":"110399999","asset":"979292558519844732"},"senderRS":"NXT-ANWW-C5BZ-SGSB-8LGZY","subtype":2,"amountNQT":"0","sender":"8033808554894054300","ecBlockHeight":495983,"deadline":1440,"transaction":"6511477257080258641","timestamp":54136767,"height":2147483647}],"requestProcessingTime":0}
                    
                   /* "senderRS": "NXT-M6QF-Q5WK-2UXK-5D3HR",
                    "subtype": 0,
                    "amountNQT": "137700000000",
                    "sender": "4304363382952792781",
                    "recipientRS": "NXT-6AC7-V9BD-NL5W-5BUWF",
                    "recipient": "3959589697280418117",
                    "ecBlockHeight": 506207,
                    "deadline": 1440,
                    "transaction": "5605109208989354417",
                    "timestamp": 55276659,
                    "height": 2147483647*/
                    if ( (txobj= jitem(array,i)) == 0 )
                        continue;
                    copy_cJSON(&txidstr,cJSON_GetObjectItem(txobj,"transaction"));
                    copy_cJSON(&recipient,cJSON_GetObjectItem(txobj,"recipient"));
                    copy_cJSON(&account,cJSON_GetObjectItem(txobj,"account"));
                    if ( account.buf[0] == 0 )
                        copy_cJSON(&account,cJSON_GetObjectItem(txobj,"sender"));
                    accountid = calc_nxt64bits(account.buf);
                    type = (int32_t)get_API_int(cJSON_GetObjectItem(txobj,"type"),-1);
                    subtype = (int32_t)get_API_int(cJSON_GetObjectItem(txobj,"subtype"),-1);
                    timestamp = get_blockutime(juint(txobj,"timestamp"));
                    amount = get_API_nxt64bits(cJSON_GetObjectItem(txobj,"amountNQT"));
                    qty = amount = assetid = 0;
                    if ( (attachment= cJSON_GetObjectItem(txobj,"attachment")) != 0 )
                    {
                        assetid = get_API_nxt64bits(cJSON_GetObjectItem(attachment,"asset"));
                        comment.buf[0] = 0;
                        qty = get_API_nxt64bits(cJSON_GetObjectItem(attachment,"quantityQNT"));
                        priceNQT = get_API_nxt64bits(cJSON_GetObjectItem(attachment,"priceNQT"));
                        baseamount = (qty * prices->ap_mult), relamount = (qty * priceNQT);
                        copy_cJSON(&comment,jobj(attachment,"message"));
                        if ( comment.buf[0] != 0 )
                        {
                            int32_t match_unconfirmed(char *sender,char *hexstr,cJSON *txobj,char *txidstr,char *account,uint64_t amount,uint64_t qty,uint64_t assetid,char *recipient);
                            //printf("sender.%s -> recv.(%s)\n",account,recipient);
                            match_unconfirmed(account.buf,comment.buf,txobj,txidstr.buf,account.buf,amount,qty,assetid,recipient.buf);
                        }
                        quoteid = calc_nxt64bits(txidstr.buf);
                        price = prices777_price_volume(&vol,baseamount,relamount);
                        if ( prices->baseid == assetid )
                        {
                            if ( Debuglevel > 2 )
                                printf("unconf.%d subtype.%d %s %llu (%llu %llu) %f %f mult.%llu qty.%llu pqt.%llu baseamount.%lld relamount.%lld\n",i,subtype,txidstr.buf,(long long)prices->baseid,(long long)assetid,(long long)NXT_ASSETID,price,vol,(long long)prices->ap_mult,(long long)qty,(long long)priceNQT,(long long)baseamount,(long long)relamount);
                            if ( subtype == 2 )
                            {
                                array = bids;
                                prices->lastbid = price;
                            }
                            else if ( subtype == 3 )
                            {
                                array = asks;
                                prices->lastask = price;
                            }
                            cJSON_AddItemToArray(array,inner_json(price,vol,timestamp,quoteid,accountid,qty,priceNQT,baseamount,relamount));
                        }
                    }
                }
                free_json(json);
            }
            free(str);
        } else printf("cant get.(%s)\n",url);
    }
    json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"bids",bids);
    cJSON_AddItemToObject(json,"asks",asks);
    prices777_json_orderbook("unconf",prices,maxdepth,json,0,"bids","asks",0,0);
    if ( Debuglevel > 2 )//|| prices->O.numbids != 0 || prices->O.numasks != 0 )
        printf("%s %s/%s unconf.(%s) %f %f (%d %d)\n",prices->contract,prices->base,prices->rel,jprint(json,0),prices->lastbid,prices->lastask,prices->O.numbids,prices->O.numasks);
    free_json(json);
    return(_pairaved(prices->lastbid,prices->lastask));
}

double prices777_InstantDEX(struct prices777 *prices,int32_t maxdepth)
{
    cJSON *json; double hbla = 0.;
    if ( (json= InstantDEX_orderbook(prices)) != 0 )
    {
        //if ( Debuglevel > 2 )
            printf("InstantDEX.(%s)\n",jprint(json,0));
        prices777_json_orderbook("InstantDEX",prices,maxdepth,json,0,"bids","asks",0,0);
        free_json(json);
    }
    return(hbla);
}

#define BASE_ISNXT 1
#define BASE_ISASSET 2
#define BASE_ISNAME 4
#define BASE_ISMGW 8
#define BASE_EXCHANGEASSET 16

int32_t calc_baseflags(char *exchange,char *base,uint64_t *baseidp)
{
    char assetidstr[64],tmpstr[64],*str; uint64_t tmp; int32_t flags = 0;
    exchange[0] = 0;
    printf("calc_baseflags.(%s/%llu) ",base,(long long)*baseidp);
    if ( strcmp(base,"NXT") == 0 || *baseidp == NXT_ASSETID )
        strcpy(base,"NXT"), *baseidp = NXT_ASSETID, flags |= BASE_ISNXT;
    else
    {
        if ( *baseidp == 0 )
        {
            if ( is_decimalstr(base) != 0 )
            {
                *baseidp = calc_nxt64bits(base), flags |= BASE_ISASSET;
                unstringbits(tmpstr,*baseidp);
                if ( (tmp= is_MGWcoin(tmpstr)) != 0 )
                    *baseidp = tmp, flags |= (BASE_EXCHANGEASSET | BASE_ISMGW);
                else
                {
                    printf("set base.(%s) -> %llu\n",base,(long long)*baseidp);
                    if ( (str= is_MGWasset(&tmp,*baseidp)) != 0 )
                        strcpy(base,str), flags |= (BASE_EXCHANGEASSET | BASE_ISMGW);
                }
            }
            else
            {
                *baseidp = stringbits(base), flags |= BASE_ISNAME;
                printf("stringbits.(%s) -> %llu\n",base,(long long)*baseidp);
            }
        }
        else
        {
            if ( (str= is_MGWasset(&tmp,*baseidp)) != 0 )
            {
                printf("is MGWasset.(%s)\n",str);
                strcpy(base,str), flags |= (BASE_EXCHANGEASSET | BASE_ISMGW | BASE_ISASSET);
            }
            else
            {
                expand_nxt64bits(assetidstr,*baseidp);
                if ( (str= is_tradedasset(exchange,assetidstr)) != 0 )
                {
                    strcpy(base,str), flags |= (BASE_EXCHANGEASSET | BASE_ISASSET);
                    printf("%s is tradedasset at (%s) %llu\n",assetidstr,str,(long long)*baseidp);
                }
                else
                {
                    unstringbits(tmpstr,*baseidp);
                    if ( (tmp= is_MGWcoin(tmpstr)) != 0 )
                        strcpy(base,tmpstr), *baseidp = tmp, flags |= (BASE_EXCHANGEASSET | BASE_ISMGW | BASE_ISASSET);
                    else
                    {
                        _set_assetname(&tmp,base,0,*baseidp), flags |= BASE_ISASSET;
                        printf("_set_assetname.(%s) from %llu\n",base,(long long)*baseidp);
                    }
                }
            }
        }
        if ( (flags & (BASE_ISASSET|BASE_EXCHANGEASSET|BASE_ISMGW)) == 0 )
            *baseidp = stringbits(base);
    }
    printf("-> flags.%d (%s %llu) %s\n",flags,base,(long long)*baseidp,exchange);
    return(flags);
}

void setitemjson(cJSON *item,char *name,char *base,uint64_t baseid,char *rel,uint64_t relid)
{
    char numstr[64];
    jaddstr(item,"name",name), jaddstr(item,"base",base), jaddstr(item,"rel",rel);
    sprintf(numstr,"%llu",(long long)baseid), jaddstr(item,"baseid",numstr);
    sprintf(numstr,"%llu",(long long)relid), jaddstr(item,"relid",numstr);
}

int32_t nxt_basketjson(cJSON *array,int32_t groupid,int32_t polarity,char *base,uint64_t baseid,char *rel,uint64_t relid,char *refbase,char *refrel)
{
    cJSON *item,*item2,*item3; char name[64]; int32_t dir = 0;
    item = cJSON_CreateObject(), jaddstr(item,"exchange",INSTANTDEX_NXTAENAME);
    item2 = cJSON_CreateObject(), jaddstr(item2,"exchange",INSTANTDEX_NXTAEUNCONF);
    item3 = cJSON_CreateObject(), jaddstr(item3,"exchange",INSTANTDEX_NAME);
    if ( strcmp(base,"NXT") == 0 )
    {
        sprintf(name,"%s/%s",rel,"NXT");
        setitemjson(item,name,rel,relid,"NXT",NXT_ASSETID);
        setitemjson(item2,name,rel,relid,"NXT",NXT_ASSETID);
        setitemjson(item3,name,rel,relid,"NXT",NXT_ASSETID);
    }
    else if ( strcmp(rel,"NXT") == 0 )
    {
        sprintf(name,"%s/%s",base,"NXT");
        setitemjson(item,name,base,baseid,"NXT",NXT_ASSETID);
        setitemjson(item2,name,base,baseid,"NXT",NXT_ASSETID);
        setitemjson(item3,name,base,baseid,"NXT",NXT_ASSETID);
    }
    else
    {
        free_json(item);
        free_json(item2);
        free_json(item3);
        return(0);
    }
    if ( strcmp(refbase,rel) == 0 || strcmp(refrel,base) == 0 )
        dir = -1;
    else dir = 1;
    jaddnum(item,"wt",dir), jaddnum(item2,"wt",dir), jaddnum(item3,"wt",dir);
    jaddnum(item,"group",groupid), jaddnum(item2,"group",groupid), jaddnum(item3,"group",groupid);
    printf("nxt_basketjson (%s/%s) %llu/%llu ref.(%s/%s) dir.%d polarity.%d\n",base,rel,(long long)baseid,(long long)relid,refbase,refrel,dir,polarity);
    jaddi(array,item), jaddi(array,item2), jaddi(array,item3);
    return(dir * polarity);
}

void add_nxtbtc(cJSON *array,int32_t groupid,double wt)
{
    char *btcnxt_xchgs[] = { "poloniex", "bittrex", "btc38" };
    int32_t i; cJSON *item;
    if ( wt != 0 )
    {
        printf("add NXT/BTC\n");
        for (i=0; i<sizeof(btcnxt_xchgs)/sizeof(*btcnxt_xchgs); i++)
        {
            item = cJSON_CreateObject(), jaddstr(item,"exchange",btcnxt_xchgs[i]);
            setitemjson(item,"NXT/BTC","NXT",NXT_ASSETID,"BTC",BTC_ASSETID);
            jaddnum(item,"wt",wt);
            jaddnum(item,"group",groupid);
            jaddi(array,item);
        }
    }
}

cJSON *make_arrayNXT(cJSON *directarray,cJSON **arrayBTCp,char *base,char *rel,uint64_t baseid,uint64_t relid)
{
    cJSON *item,*arrayNXT = 0; char tmpstr[64],baseexchange[64],relexchange[64],*str;
    int32_t wt,baseflags,relflags,i,j,n,m; uint64_t duplicatebases[16],duplicaterels[16];
    baseflags = calc_baseflags(baseexchange,base,&baseid);
    relflags = calc_baseflags(relexchange,rel,&relid);
    sprintf(tmpstr,"%s/%s",base,rel);
    printf("make_arrayNXT base.(%s) %llu rel.(%s) %llu baseflags.%d relflags.%d\n",base,(long long)baseid,rel,(long long)relid,baseflags,relflags);
    item = cJSON_CreateObject(), setitemjson(item,tmpstr,base,baseid,rel,relid);
    jaddstr(item,"exchange",INSTANTDEX_NAME);
    jaddnum(item,"wt",1), jaddnum(item,"group",0), jaddi(directarray,item);
    if ( ((baseflags | relflags) & BASE_ISNXT) != 0 )
    {
        printf("one is NXT\n");
        if ( strcmp(base,"NXT") == 0 )
            n = get_duplicates(duplicatebases,relid), wt = -1, str = rel;
        else n = get_duplicates(duplicatebases,baseid), wt = 1, str = base;
        sprintf(tmpstr,"%s/%s",str,"NXT");
        for (i=0; i<n; i++)
            nxt_basketjson(directarray,0,wt,str,duplicatebases[i],"NXT",NXT_ASSETID,base,rel);
    }
    else if ( (baseflags & BASE_ISASSET) != 0 && (relflags & BASE_ISASSET) != 0 )
    {
        printf("both are assets (%s/%s)\n",base,rel);
        arrayNXT = cJSON_CreateArray();
        n = get_duplicates(duplicatebases,baseid);
        for (i=0; i<n; i++)
            nxt_basketjson(arrayNXT,0,1,base,duplicatebases[i],"NXT",NXT_ASSETID,base,rel);
        if ( strcmp(base,"BTC") == 0 )
            add_nxtbtc(arrayNXT,0,-1);
        sprintf(tmpstr,"%s/%s",rel,"NXT");
        m = get_duplicates(duplicaterels,relid);
        for (j=0; j<m; j++)
            nxt_basketjson(arrayNXT,1,-1,rel,duplicaterels[j],"NXT",NXT_ASSETID,base,rel);
        if ( strcmp(rel,"BTC") == 0 )
            add_nxtbtc(arrayNXT,1,1);
    }
    if ( (baseflags & BASE_EXCHANGEASSET) != 0 || (relflags & BASE_EXCHANGEASSET) != 0 )
    {
        printf("have exchange asset %d %d\n",baseflags,relflags);
        if ( (baseflags & BASE_EXCHANGEASSET) != 0 && (relflags & BASE_EXCHANGEASSET) != 0 )
        {
            printf("both are exchange asset\n");
            if ( *arrayBTCp == 0 )
                *arrayBTCp = cJSON_CreateArray();
            if ( strcmp(base,"BTC") != 0 && strcmp(rel,"BTC") != 0 )
            {
                printf("a both are exchange asset\n");
                item = cJSON_CreateObject(), jaddstr(item,"exchange",baseexchange);
                sprintf(tmpstr,"%s/%s",base,"BTC");
                setitemjson(item,tmpstr,base,baseid,"BTC",stringbits("BTC"));
                jaddi(*arrayBTCp,item);
                item = cJSON_CreateObject(), jaddstr(item,"exchange",relexchange);
                sprintf(tmpstr,"%s/%s",rel,"BTC");
                setitemjson(item,tmpstr,rel,relid,"BTC",stringbits("BTC"));
                jaddnum(item,"wt",-1);
                jaddnum(item,"group",1);
                jaddi(*arrayBTCp,item);
            }
        }
        else if ( (baseflags & BASE_EXCHANGEASSET) != 0 )
        {
            if ( strcmp(base,"BTC") != 0 && strcmp(rel,"BTC") == 0 )
            {
                printf("base.(%s/%s) is exchangeasset\n",base,rel);
                item = cJSON_CreateObject(), jaddstr(item,"exchange",baseexchange);
                sprintf(tmpstr,"%s/%s",base,"BTC");
                setitemjson(item,tmpstr,base,baseid,"BTC",stringbits("BTC"));
                jaddi(directarray,item);
            }
        }
        else
        {
            if ( strcmp(rel,"BTC") != 0 && strcmp(base,"BTC") == 0 )
            {
                printf("rel.(%s/%s) is exchangeasset\n",base,rel);
                item = cJSON_CreateObject(), jaddstr(item,"exchange",relexchange);
                sprintf(tmpstr,"%s/%s",rel,"BTC");
                setitemjson(item,tmpstr,rel,relid,"BTC",stringbits("BTC"));
                jaddnum(item,"wt",-1);
                jaddi(directarray,item);
            }
        }
    }
    return(arrayNXT);
}

int32_t centralexchange_items(int32_t group,double wt,cJSON *array,char *_base,char *_rel,int32_t tradeable,char *refbase,char *refrel)
{
    int32_t exchangeid,inverted,n = 0; char base[64],rel[64],name[64]; cJSON *item;
    for (exchangeid=FIRST_EXTERNAL; exchangeid<MAX_EXCHANGES; exchangeid++)
    {
        strcpy(base,_base), strcpy(rel,_rel);
        if ( Exchanges[exchangeid].name[0] == 0 )
            break;
        //printf("check %s for (%s/%s) group.%d wt.%f\n",Exchanges[exchangeid].name,base,rel,group,wt);
        if ( Exchanges[exchangeid].supports != 0 && (inverted= (*Exchanges[exchangeid].supports)(base,rel)) != 0 && (tradeable == 0 || Exchanges[exchangeid].apikey[0] != 0) )
        {
            if ( array != 0 )
            {
                item = cJSON_CreateObject(), jaddstr(item,"exchange",Exchanges[exchangeid].name);
                //printf("ref.(%s/%s) vs (%s/%s) inverted.%d flipped.%d\n",refbase,refrel,base,rel,inverted,strcmp(refbase,rel) == 0 || strcmp(refrel,base) == 0);
                if ( inverted < 0 )
                    jaddstr(item,"base",rel), jaddstr(item,"rel",base), sprintf(name,"%s/%s",rel,base);
                else jaddstr(item,"base",base), jaddstr(item,"rel",rel), sprintf(name,"%s/%s",base,rel);
                if ( strcmp(refbase,rel) == 0 || strcmp(refrel,base) == 0 )
                    jaddnum(item,"wt",-inverted);
                else jaddnum(item,"wt",inverted);
                jaddstr(item,"name",name), jaddnum(item,"group",group);
                printf("ADDED.%s inverted.%d (%s) ref.(%s/%s)\n",Exchanges[exchangeid].name,inverted,name,refbase,refrel);
                jaddi(array,item);
            }
            n++;
        }
    }
    return(n);
}

cJSON *external_combo(char *base,char *rel,char *coinstr,int32_t tradeable)
{
    cJSON *array = 0;
    printf("check central jumper.(%s) for (%s/%s)\n",coinstr,base,rel);
    //if ( (baseflags & (BASE_ISNAME|BASE_ISMGW)) != 0 && (relflags & (BASE_ISNAME|BASE_ISMGW)) != 0 )
    {
        if ( strcmp(base,coinstr) != 0 && strcmp(rel,coinstr) != 0 && centralexchange_items(0,1,0,base,coinstr,tradeable,base,rel) > 0 && centralexchange_items(0,1,0,rel,coinstr,tradeable,base,rel) > 0 )
        {
            array = cJSON_CreateArray();
            printf("add central jumper.(%s) for (%s/%s)\n",coinstr,base,rel);
            centralexchange_items(0,1,array,base,coinstr,tradeable,base,rel);
            centralexchange_items(1,-1,array,rel,coinstr,tradeable,base,rel);
        }
    }
    return(array);
}

int32_t make_subactive(struct prices777 *baskets[],int32_t n,cJSON *array,char *prefix,char *base,char *rel,uint64_t baseid,uint64_t relid,int32_t maxdepth)
{
    char tmpstr[64],typestr[64]; struct prices777 *basket; cJSON *basketjson;
    basketjson = cJSON_CreateObject();
    sprintf(tmpstr,"%s/%s",base,rel);
    setitemjson(basketjson,tmpstr,base,baseid,rel,relid);
    jadd(basketjson,"basket",array);
    printf("%s BASKETMAKE.(%s)\n",prefix,jprint(basketjson,0));
    sprintf(typestr,"basket%s",prefix);
    if ( (basket= prices777_makebasket(0,basketjson,1,typestr,0,0)) != 0 )
    {
        prices777_basket(basket,maxdepth);
        prices777_jsonstrs(basket,&basket->O);
        printf("add to baskets[%d].%s (%s/%s) (%s)\n",n,basket->exchange,basket->base,basket->rel,basket->contract);
        baskets[n++] = basket;
    }
    free_json(basketjson);
    return(n);
}

char *prices777_activebooks(char *name,char *_base,char *_rel,uint64_t baseid,uint64_t relid,int32_t maxdepth,int32_t allflag,int32_t tradeable)
{
    cJSON *array,*arrayNXT,*arrayBTC,*arrayUSD,*arrayCNY,*basketjson; struct prices777 *active,*basket,*baskets[64];
    int32_t inverted,keysize,baseflags,relflags,n = 0; char tmpstr[64],base[64],rel[64],bexchange[64],rexchange[64],key[512],*retstr = 0;
    memset(baskets,0,sizeof(baskets));
    strcpy(base,_base), strcpy(rel,_rel);
    baseflags = calc_baseflags(bexchange,base,&baseid);
    relflags = calc_baseflags(rexchange,rel,&relid);
    InstantDEX_name(key,&keysize,"active",name,base,&baseid,rel,&relid);
    printf("activebooks (%s/%s) (%llu/%llu)\n",base,rel,(long long)baseid,(long long)relid);
    if ( (active= prices777_find(&inverted,baseid,relid,"active")) == 0 )
    {
        if ( ((baseflags & BASE_ISMGW) != 0 || (baseflags & BASE_ISASSET) == 0) && ((relflags & BASE_ISMGW) != 0 || (relflags & BASE_ISASSET) == 0) )
        {
            if ( (arrayUSD= external_combo(base,rel,"USD",tradeable)) != 0 )
                n = make_subactive(baskets,n,arrayUSD,"USD",base,rel,baseid,relid,maxdepth);
            if ( (arrayCNY= external_combo(base,rel,"CNY",tradeable)) != 0 )
                n = make_subactive(baskets,n,arrayCNY,"CNY",base,rel,baseid,relid,maxdepth);
        }
        arrayBTC = external_combo(base,rel,"BTC",tradeable);
        basketjson = cJSON_CreateObject(), array = cJSON_CreateArray();
        sprintf(tmpstr,"%s/%s",base,rel);
        setitemjson(basketjson,tmpstr,base,baseid,rel,relid);
        //if ( baseflags != BASE_ISASSET && relflags != BASE_ISASSET )
            centralexchange_items(0,1,array,base,rel,tradeable,base,rel);
        if ( (arrayNXT= make_arrayNXT(array,&arrayBTC,base,rel,baseid,relid)) != 0 )
            n = make_subactive(baskets,n,arrayNXT,"NXT",base,rel,baseid,relid,maxdepth);
        if ( arrayBTC != 0 )
            n = make_subactive(baskets,n,arrayBTC,"BTC",base,rel,baseid,relid,maxdepth);
        if ( (basket= prices777_find(&inverted,baseid,relid,"basket")) != 0 )
            baskets[n++] = basket;
        jadd(basketjson,"basket",array);
        printf(" ACTIVE MAKE.(%s)\n",jprint(basketjson,0));
        if ( (active= prices777_makebasket(0,basketjson,1,"active",baskets,n)) != 0 )
        {
            prices777_basket(active,maxdepth);
            prices777_jsonstrs(active,&active->O);
        }
        free_json(array);
    }
    if ( active != 0 && retstr == 0 )
    {
        prices777_basket(active,maxdepth);
        prices777_jsonstrs(active,&active->O);
        if ( (retstr= active->orderbook_jsonstrs[inverted][allflag]) != 0 )
            retstr = clonestr(retstr);
    }
    if ( retstr == 0 )
        retstr = clonestr("{\"error\":\"null active orderbook\"}");
    return(retstr);
}

#endif
