//
//  atomic.h
//
//
//  Created by jl777 on 7/16/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//
// need to verify expirations (deadline + timestamp)

#ifndef xcode_atomic_h
#define xcode_atomic_h

struct tradeinfo
{
    uint64_t quoteid,assetid,relid,qty,priceNQT,txid;
    uint32_t expiration;
    uint16_t exchangeid; uint16_t sell:1,needsubmit:1,sent:1,unconf:1,closed:1,error:1;
};

struct pendinghalf
{
    char exchange[16];
    uint64_t buyer,seller,baseamount,relamount,avail,feetxid;
    double price,vol;
    struct tradeinfo T;
};

struct pending_offer
{
    char comment[MAX_JSON_FIELD],feeutxbytes[MAX_JSON_FIELD],feesignedtx[MAX_JSON_FIELD],triggerhash[65],feesighash[65];
    struct NXT_tx *feetx;
    uint64_t actual_feetxid,fee,nxt64bits,baseid,relid,srcqty,quoteid,srcarg,srcamount,baseamount,relamount,offerNXT;
    double ratio,endmilli,price,volume;
    int32_t errcode,sell,flip;
    uint32_t expiration;
    struct pendinghalf base,rel;
};

char *pending_offer_error(struct pending_offer *pt)
{
    char *jsonstr;
    jsonstr = clonestr(pt->comment);
    if ( pt->feetx != 0 )
        free(pt->feetx);
    free(pt);
    return(jsonstr);
}

int32_t scale_qty(struct pending_offer *pt,char *NXTaddr,struct pendinghalf *base,struct pendinghalf *rel)
{
    char assetidstr[64]; struct NXT_asset *ap; int32_t createdflag; double ratio = 1; uint64_t tmp,mult,assetid;
    if ( pt->sell == 0 )
        pt->srcarg = base->baseamount, assetid = base->T.assetid;
    else pt->srcarg = rel->baseamount, assetid = rel->T.assetid;
    if ( assetid == NXT_ASSETID )
        mult = 1;
    else
    {
        expand_nxt64bits(assetidstr,rel->T.assetid);
        ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
        mult = ap->mult;
    }
    printf("sell.%d srcarg %llu  srcmult.%llu\n",pt->sell,(long long)pt->srcarg,(long long)mult);
    if ( mult != 0 && pt->srcarg != 0 )
    {
        tmp = (pt->srcarg / mult);
        if ( pt->srcqty != 0 && pt->srcqty < tmp )
            pt->srcamount = mult * pt->srcqty, printf("srcamount path0: %.8f tmp %.8f\n",dstr(pt->srcamount),dstr(tmp));
        else pt->srcamount = (tmp * mult), printf("srcamount path1: %.8f tmp %.8f\n",dstr(pt->srcamount),dstr(tmp));
    } else return(0.);
    if ( pt->srcamount != 0 )
    {
        if ( pt->srcamount < pt->srcarg )
        {
            pt->ratio = ratio = ((double)pt->srcamount / pt->srcarg);
            if ( pt->sell == 0 )
                base->baseamount = pt->srcamount, base->relamount *= ratio, rel->baseamount *= ratio, rel->relamount *= ratio;
            else rel->baseamount = pt->srcamount, rel->relamount *= ratio, base->baseamount *= ratio, base->relamount *= ratio;
        }
        base->price = calc_price_volume(&base->vol,base->baseamount,base->relamount);
        printf("base: price %f vol %f amount %llu %llu\n",base->price,base->vol,(long long)base->baseamount,(long long)base->relamount);
        base->T.qty = calc_asset_qty(&base->avail,&base->T.priceNQT,NXTaddr,pt->sell==0,base->T.assetid,base->price,base->vol);
        rel->price = calc_price_volume(&rel->vol,rel->baseamount,rel->relamount);
        rel->T.qty = calc_asset_qty(&rel->avail,&rel->T.priceNQT,NXTaddr,pt->sell!=0,rel->T.assetid,rel->price,rel->vol);
        printf("qtyA %llu priceA %f volA %f, qtyB %lld priceB %f volB %f\n",(long long)base->T.qty,base->price,base->vol,(long long)rel->T.qty,rel->price,rel->vol);
        printf("ratio %f, srcamount %.8f srcarg %.8f srcqty %.8f -> (%f %f) (%f %f)\n",ratio,dstr(pt->srcamount),dstr(pt->srcarg),dstr(pt->srcqty),base->price,base->vol,rel->price,rel->vol);
        if ( pt->srcamount == 0 || base->T.qty == 0 || rel->T.qty == 0 || base->T.priceNQT == 0 || rel->T.priceNQT == 0 )
        {
            sprintf(pt->comment,"{\"error\":\"%s\",\"descr\":\"NXT.%llu makeoffer3 srcamount.%.8f qtyA %.8f assetA.%llu price %.8f for qtyB %.8f assetB.%llu price.%.8f\"}\n","illegal parameter",(long long)calc_nxt64bits(NXTaddr),dstr(pt->srcamount),dstr(base->T.qty),(long long)base->T.assetid,dstr(base->T.priceNQT),dstr(rel->T.qty),(long long)rel->T.assetid,dstr(rel->T.priceNQT));
            return(-1);
        }
        return(0);
    } else return(-1);
}

double calc_asset_QNT(struct pendinghalf *half,uint64_t nxt64bits,int32_t checkflag,uint64_t srcqty)
{
    char NXTaddr[64],assetidstr[64]; struct NXT_asset *ap;
    double ratio = 1.;
    int32_t createdflag;
    int64_t unconfirmed,balance;
    expand_nxt64bits(NXTaddr,nxt64bits);
    expand_nxt64bits(assetidstr,half->T.assetid);
    ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
    if ( ap->mult != 0 )
    {
        half->T.priceNQT = (half->relamount * ap->mult) / half->baseamount;
        if ( (half->T.qty= half->baseamount / ap->mult) == 0 )
            return(0);
        if ( srcqty < half->T.qty )
        {
            ratio = (double)srcqty / half->T.qty;
            half->baseamount *= ratio, half->relamount *= ratio;
            half->price = calc_price_volume(&half->vol,half->baseamount,half->relamount);
            half->T.priceNQT = (half->relamount * ap->mult) / half->baseamount;
            if ( (half->T.qty= half->baseamount / ap->mult) == 0 )
                return(0);
        }
        else if ( half->price == 0. )
            half->price = calc_price_volume(&half->vol,half->baseamount,half->relamount);
        balance = get_asset_quantity(&unconfirmed,NXTaddr,assetidstr);
        printf("%s balance %.8f unconfirmed %.8f vs price %llu qty %llu for asset.%s | (%f * %f) * (%ld / %llu)\n",NXTaddr,dstr(balance),dstr(unconfirmed),(long long)half->T.priceNQT,(long long)half->T.qty,assetidstr,half->vol,half->price,SATOSHIDEN,(long long)ap->mult);
       // getchar();
        if ( checkflag != 0 && (balance < half->T.qty || unconfirmed < half->T.qty) )
            return(0);
    } else printf("%llu null apmult\n",(long long)half->T.assetid);
    return(ratio);
}

struct InstantDEX_quote *is_valid_offer(uint64_t quoteid,int32_t dir,uint64_t assetid,uint64_t qty,uint64_t priceNQT)
{
    int32_t polarity;
    double price,vol,refprice,refvol;
    struct InstantDEX_quote *iQ;
    uint64_t baseamount,relamount;
    if ( (iQ= findquoteid(quoteid,0)) != 0 && iQ->matched == 0 )
    {
        polarity = (iQ->isask != 0) ? -1 : 1;
        if ( Debuglevel > 2 )
            printf("found quoteid.%llu polarity.%d %llu/%llu vs %llu dir.%d\n",(long long)quoteid,polarity,(long long)iQ->baseid,(long long)iQ->relid,(long long)assetid,dir);
        if ( polarity*dir > 0 && ((polarity > 0 && iQ->baseid == assetid) || (polarity < 0 && iQ->relid == assetid)) )
        {
            baseamount = calc_baseamount(&relamount,assetid,qty,priceNQT);
            if ( polarity > 0 )
                price = calc_price_volume(&vol,baseamount,relamount), refprice = calc_price_volume(&refvol,iQ->baseamount,iQ->relamount);
            else price = calc_price_volume(&vol,relamount,baseamount), refprice = calc_price_volume(&refvol,iQ->relamount,iQ->baseamount);
            if ( Debuglevel > 2 )
                printf("polarity.%d dir.%d (%f %f) vs ref.(%f %f)\n",polarity,dir,price,vol,refprice,refvol);
            if ( vol >= refvol*(double)iQ->minperc/100. && vol <= refvol )
            {
                if ( (dir > 0 && price <= (refprice * (1. + INSTANTDEX_PRICESLIPPAGE) + SMALLVAL)) || (dir < 0 && price >= (refprice / (1. + INSTANTDEX_PRICESLIPPAGE) - SMALLVAL)) )
                    return(iQ);
            }
        }
    } else printf("couldnt find quoteid.%llu\n",(long long)quoteid);
    return(0);
}

void update_openorder(struct InstantDEX_quote *iQ,uint64_t quoteid,struct NXT_tx *txptrs[],int32_t updateNXT) // autoresponse to own open orders
{
    //printf("update_openorder iQ.%llu with tx.%llu\n",(long long)iQ->quoteid,(long long)tx->txid);
    // regen orderbook and see if it crosses
    // updatestats
}

uint64_t otherNXT(struct pending_offer *pt,struct pendinghalf *half)
{
    uint64_t otherNXT;
    if ( half->T.sell == 0 )
        otherNXT = half->seller;
    else otherNXT = half->buyer;
    if ( otherNXT != pt->offerNXT )
        printf("otherNXT error %llu != %llu\n",(long long)otherNXT,(long long)pt->offerNXT);
    return(otherNXT);
}

uint64_t otherasset(struct pendinghalf *half)
{
    uint64_t otherasset;
    if ( half->T.sell == 0 )
        otherasset = half->T.relid;
    else otherasset = half->T.assetid;
    return(otherasset);
}

uint64_t myasset(struct pendinghalf *half)
{
    uint64_t otherasset;
    if ( half->T.sell == 0 )
        otherasset = half->T.assetid;
    else otherasset = half->T.relid;
    return(otherasset);
}

uint64_t tradedasset(struct pendinghalf *half)
{
    if ( half->T.assetid != NXT_ASSETID )
        return(half->T.assetid);
    else if ( half->T.relid != NXT_ASSETID )
        return(half->T.relid);
    return(0);
}

int32_t matches_halfquote(struct pending_offer *pt,struct pendinghalf *half,struct NXT_tx *tx)
{
    //printf("tx quoteid.%llu %llu\n",(long long)tx->quoteid,(long long)half->T.quoteid);
    if ( tx->quoteid != half->T.quoteid )
        return(0);
    if ( Debuglevel > 2 )
        printf("sell.%d tx type.%d subtype.%d otherNXT.%llu sender.%llu myasset.%llu txasset.%llu Uamount.%llu qty.%llu\n",half->T.sell,tx->type,tx->subtype,(long long)otherNXT(pt,half),(long long)tx->senderbits,(long long)myasset(half),(long long)tx->assetidbits,(long long)tx->U.amountNQT,(long long)half->T.qty);
    
    if ( tx->type == 0 )
    {
        if ( tx->subtype != 0 || otherNXT(pt,half) != tx->senderbits || myasset(half) != NXT_ASSETID )
            return(0);
        return(tx->U.amountNQT == half->T.qty);
    }
    else if ( tx->type == 2 && tx->U.quantityQNT == half->T.qty && otherNXT(pt,half) == tx->senderbits )
    {
        if ( tx->subtype == 1 )
            return(myasset(half) == tx->assetidbits);
        return(tx->priceNQT == half->T.priceNQT && tx->subtype == (3 - half->T.sell) && tradedasset(half) == tx->assetidbits);
    }
    return(0);
}

int32_t is_feetx(struct NXT_tx *tx)
{
    if ( tx->recipientbits == calc_nxt64bits(INSTANTDEX_ACCT) && tx->type == 0 && tx->subtype == 0 && tx->U.amountNQT >= INSTANTDEX_FEE )
        return(1);
    else return(0);
}

int32_t pendinghalf_is_complete(struct pending_offer *pt,struct pendinghalf *half,struct NXT_tx *txptrs[])
{
    int32_t i;
    bits256 refhash;
    struct NXT_tx *tx;
    //printf("check for complete asset.%llu unconf.%d closed.%d needsubmit.%d sent.%d sell.%d exchange.%d\n",(long long)half->T.assetid,half->T.unconf,half->T.closed,half->T.needsubmit,half->T.sent,half->T.sell,half->T.exchangeid);
    if ( half->T.error != 0 )
        return(0);
    if ( half->T.assetid == 0 || (half->T.unconf != 0 && half->T.txid != 0 && half->feetxid != 0) || half->T.closed != 0 )
        return(1);
    if ( half->T.txid != 0 && (tx= search_txptrs(txptrs,half->T.txid,0,0,0)) != 0 )
    {
        half->T.unconf = 1;
        return(1);
    }
    if ( half->T.needsubmit != 0 && half->T.sent != 0 )
    {
        if ( half->T.exchangeid != INSTANTDEX_EXCHANGEID )
            half->feetxid = 1;
        decode_hex(refhash.bytes,sizeof(refhash),pt->triggerhash);
        for (i=0; i<MAX_TXPTRS; i++)
        {
            if ( (tx= txptrs[i]) == 0 )
                break;
            if ( tx->senderbits == otherNXT(pt,half) && memcmp(refhash.bytes,tx->refhash.bytes,sizeof(refhash)) == 0 )
            {
                if ( half->feetxid == 0 && is_feetx(tx) != 0 )
                {
                    half->feetxid = tx->txid;
                    printf("got fee from offerNXT.%llu\n",(long long)otherNXT(pt,half));
                }
                if ( half->T.txid == 0 && half->T.quoteid != 0 )
                {
                    if ( matches_halfquote(pt,half,tx) != 0 )
                    {
                        printf("REMOTE MATCH %llu %llu %.8f\n",(long long)half->T.assetid,(long long)half->T.qty,dstr(half->T.priceNQT));
                        half->T.unconf = 1;
                        half->T.txid = tx->txid;
                        return(half->feetxid != 0);
                    } //else printf("no match\n");
                }
            }
        }
    }
    return(0);
}

uint64_t need_to_submithalf(struct pendinghalf *half,int32_t dir,struct pending_offer *pt)
{
    printf("dir.%d exch.%d T.sell.%d %llu/%llu %llu\n",dir,half->T.exchangeid,half->T.sell,(long long)half->T.assetid,(long long)half->T.relid,(long long)otherasset(half));
    if ( half->T.exchangeid == INSTANTDEX_EXCHANGEID )
        return(otherNXT(pt,half));
    else if ( half->T.exchangeid == INSTANTDEX_NXTAEID || half->T.exchangeid == INSTANTDEX_UNCONFID )
    {
        if ( (dir > 0 && half->T.sell == 0) || (dir < 0 && half->T.sell != 0) )
        {
            if ( myasset(half) != NXT_ASSETID )
                half->T.closed = 1;
            return(1);
        }
        half->T.closed = 1;
        printf("%p close half dir.%d\n",half,dir);
    }
    return(0);
}

int32_t set_pendinghalf(struct pending_offer *pt,int32_t dir,struct pendinghalf *half,uint64_t assetid,uint64_t baseamount,uint64_t relid,uint64_t relamount,uint64_t quoteid,uint64_t buyer,uint64_t seller,char *exchangestr)
{
    struct exchange_info *find_exchange(char *exchangestr,int32_t createflag);
    struct exchange_info *exchange;
    printf("dir.%d sethalf %llu/%llu buyer.%llu seller.%llu\n",dir,(long long)assetid,(long long)relid,(long long)buyer,(long long)seller);
    half->T.assetid = assetid, half->T.relid = relid, half->baseamount = baseamount, half->relamount = relamount, half->T.quoteid = quoteid;
    if ( dir > 0 )
        half->buyer = buyer, half->seller = seller;
    else half->buyer = seller, half->seller = buyer;
    strcpy(half->exchange,exchangestr);
    if ( (exchange= find_exchange(half->exchange,0)) != 0 )
    {
        if ( dir < 0 )
            half->T.sell = 1;
        half->T.exchangeid = exchange->exchangeid;
        half->T.needsubmit = need_to_submithalf(half,dir,pt);
        printf("%s exchange.%d sethalf other.%llu asset.%llu | myasset.%llu tradedasset.%llu | closed.%d needsubmit.%d\n",exchangestr,half->T.exchangeid,(long long)otherNXT(pt,half),(long long)otherasset(half),(long long)myasset(half),(long long)tradedasset(half),half->T.closed,half->T.needsubmit);
        if ( otherNXT(pt,half) != 0 && half->baseamount != 0 && half->relamount != 0 )
            return(half->T.exchangeid == INSTANTDEX_EXCHANGEID);
    }
    half->T.error = 1;
    return(-1);
}

// phasing! https://nxtforum.org/index.php?topic=6490.msg171048#msg171048
int32_t extract_pendinghalf(struct pending_offer *pt,int32_t dir,struct pendinghalf *half,cJSON *obj,uint64_t assetid,uint64_t jumpasset)
{
    char exchange[MAX_JSON_FIELD];
    uint64_t baseamount,relamount,quoteid,offerNXT;
    if ( obj != 0 )
    {
        baseamount = get_API_nxt64bits(cJSON_GetObjectItem(obj,"baseamount"));
        relamount = get_API_nxt64bits(cJSON_GetObjectItem(obj,"relamount"));
        quoteid = get_API_nxt64bits(cJSON_GetObjectItem(obj,"quoteid"));
        offerNXT = get_API_nxt64bits(cJSON_GetObjectItem(obj,"offerNXT"));
        copy_cJSON(exchange,cJSON_GetObjectItem(obj,"exchange"));
        return(set_pendinghalf(pt,dir,half,assetid,baseamount,jumpasset,relamount,quoteid,dir>0?pt->nxt64bits:offerNXT,dir<0?pt->nxt64bits:offerNXT,exchange));
    }
    return(-1);
}

/*
InstantDEX is already able to automatically combine orderbooks
so this is the hard part about conversion
just make N fiat markets against NXT and automatically there is N*N virtual currency conversion orderbooks
place an order to convert
and I can add an extra functionality so you can convert and specify a different destination account, basically you buy the destination fiat asset you want the recipient to receive
Alice has USD, wants to send EUR to Bob*/

int32_t process_Pending_offersQ(struct pending_offer **ptp,void **ptrs)
{
    char *NXTACCTSECRET; struct NXT_tx **txptrs; struct pending_offer *pt = *ptp;
    NXTACCTSECRET = ptrs[0], txptrs = ptrs[1];
    if ( milliseconds() > pt->endmilli || pt->base.T.error != 0 || pt->rel.T.error != 0 )
    {
        pt->errcode = -1;
        printf("(%f > %f) expired pending trade.(%s) || errors: base.%d rel.%d\n",milliseconds(),pt->endmilli,pt->comment,pt->base.T.error,pt->rel.T.error);
        return(-1);
    }
    if ( pendinghalf_is_complete(pt,&pt->base,txptrs) != 0 && pendinghalf_is_complete(pt,&pt->rel,txptrs) != 0 )
    {
        if ( (pt->actual_feetxid= issue_broadcastTransaction(&pt->errcode,0,pt->feesignedtx,NXTACCTSECRET)) != pt->feetx->txid )
        {
            printf("Jump trades triggered! feetxid.%llu but unexpected should have been %llu\n",(long long)pt->actual_feetxid,(long long)pt->feetx->txid);
            pt->base.T.closed = pt->rel.T.closed = 1;
            return(-1);
        }
        else
        {
            printf("Jump trades triggered! feetxid.%llu\n",(long long)pt->actual_feetxid);
            return(1);
        }
    }
    return(0);
}

void poll_pending_offers(char *NXTaddr,char *NXTACCTSECRET)
{
    static uint32_t prevNXTblock;
    struct InstantDEX_quote *iQ;
    cJSON *json,*array,*item; struct NXT_tx *txptrs[MAX_TXPTRS]; void *ptrs[2];
    int32_t i,n,numtx,NXTblock; uint64_t quoteid,baseid,relid;
    ptrs[0] = NXTACCTSECRET, ptrs[1] = txptrs;
    NXTblock = get_NXTblock(0);
    memset(txptrs,0,sizeof(txptrs));
    if ( (numtx= update_iQ_flags(txptrs,(sizeof(txptrs)/sizeof(*txptrs))-1,0)) > 0 )
    {
        if ( (json= openorders_json(NXTaddr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"openorders")) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    item = cJSON_GetArrayItem(array,i);
                    if ( (quoteid= get_API_nxt64bits(cJSON_GetObjectItem(item,"quoteid"))) != 0 )
                    {
                        baseid = get_API_nxt64bits(cJSON_GetObjectItem(item,"baseid"));
                        relid = get_API_nxt64bits(cJSON_GetObjectItem(item,"relid"));
                        iQ = (struct InstantDEX_quote *)get_API_nxt64bits(cJSON_GetObjectItem(item,"iQ"));
                        if ( iQ->closed != 0 || iQ->baseid != baseid || iQ->relid != relid || calc_quoteid(iQ) != quoteid )
                            printf("error: isclosed.%d %llu/%llu != %llu/%llu: iQ.%p quoteid.%llu vs %llu\n",iQ->closed,(long long)iQ->baseid,(long long)iQ->relid,(long long)baseid,(long long)relid,iQ,(long long)calc_quoteid(iQ),(long long)quoteid);
                        else update_openorder(iQ,quoteid,txptrs,NXTblock == prevNXTblock);
                    }
                }
            }
            free_json(json);
        }
        process_pingpong_queue(&Pending_offersQ,ptrs);
        free_txptrs(txptrs,numtx);
    }
    if ( NXTblock != prevNXTblock )
        prevNXTblock = NXTblock, printf("New NXTblock.%d\n",NXTblock);
}

int32_t submit_trade(int32_t dir,struct pendinghalf *half,struct pendinghalf *other,struct pending_offer *pt,char *NXTACCTSECRET)
{
   /* makeoffer3 7581814105672729429 offer 423766016895692955
    dir.-1 sethalf 6932037131189568014/5527630 buyer.7581814105672729429 seller.423766016895692955
    dir.-1 exch.0 T.sell.1 6932037131189568014/5527630 6932037131189568014
    InstantDEX exchange.0 sethalf other.423766016895692955 asset.6932037131189568014 | myasset.5527630 tradedasset.6932037131189568014 | closed.0 needsubmit.1
    7581814105672729429 balance 0.00000084 unconfirmed 0.00000077 vs price 288000000 qty 1 for asset.6932037131189568014 | (1.000000 * 2.880000) * (100000000 / 100000000)
        dir.1 sethalf 6932037131189568014/5527630 buyer.7581814105672729429 seller.423766016895692955
        dir.1 exch.0 T.sell.0 6932037131189568014/5527630 5527630
        InstantDEX exchange.0 sethalf other.423766016895692955 asset.5527630 | myasset.6932037131189568014 tradedasset.6932037131189568014 | closed.0 needsubmit.1
        7581814105672729429 balance 0.00000084 unconfirmed 0.00000077 vs price 288000000 qty 1 for asset.6932037131189568014 | (1.000000 * 2.880000) * (100000000 / 100000000)*/
    
    /*makeoffer3 7581814105672729429 offer 456272193289401243
     dir.-1 sethalf 6932037131189568014/5527630 buyer.7581814105672729429 seller.456272193289401243
     dir.-1 exch.2 T.sell.1 6932037131189568014/5527630 6932037131189568014
     nxtae exchange.2 sethalf other.456272193289401243 asset.6932037131189568014 | myasset.5527630 tradedasset.6932037131189568014 | closed.1 needsubmit.1
     7581814105672729429 balance 0.00000083 unconfirmed 0.00000078 vs price 283000000 qty 1 for asset.6932037131189568014 | (1.000000 * 2.830000) * (100000000 / 100000000)
     dir.1 sethalf 6932037131189568014/5527630 buyer.7581814105672729429 seller.456272193289401243
     dir.1 exch.2 T.sell.0 6932037131189568014/5527630 5527630
     nxtae exchange.2 sethalf other.456272193289401243 asset.5527630 | myasset.6932037131189568014 tradedasset.6932037131189568014 | closed.0 needsubmit.1
         
         makeoffer3 7581814105672729429 offer 16166916883665310631
         dir.1 sethalf 6932037131189568014/5527630 buyer.7581814105672729429 seller.16166916883665310631
         dir.1 exch.2 T.sell.0 6932037131189568014/5527630 5527630
         nxtae exchange.2 sethalf other.16166916883665310631 asset.5527630 | myasset.6932037131189568014 tradedasset.6932037131189568014 | closed.0 needsubmit.1
         7581814105672729429 balance 0.00000083 unconfirmed 0.00000078 vs price 289000000 qty 1 for asset.6932037131189568014 | (1.000000 * 2.890000) * (100000000 / 100000000)
             dir.-1 sethalf 6932037131189568014/5527630 buyer.7581814105672729429 seller.16166916883665310631
             dir.-1 exch.2 T.sell.1 6932037131189568014/5527630 6932037131189568014
             nxtae exchange.2 sethalf other.16166916883665310631 asset.6932037131189568014 | myasset.5527630 tradedasset.6932037131189568014 | closed.1 needsubmit.1
*/

    char _tokbuf[4096],cmdstr[4096],ipaddr[64],*jsonstr;
    int32_t len;
    struct nodestats *stats;
    struct pserver_info *pserver;
    if ( half->T.closed != 0 )
        return(0);
    half->T.sent = 1;
    if ( half->T.needsubmit != 0 )
    {
        if ( myasset(half) == NXT_ASSETID )
        {
            printf("%p SUBMIT sell.%d dir.%d closed.%d\n",half,half->T.sell,dir,half->T.closed);
            if ( (half->T.txid= submit_to_exchange(INSTANTDEX_NXTAEID,&jsonstr,half->T.assetid,half->T.qty,half->T.priceNQT,dir,pt->nxt64bits,NXTACCTSECRET,pt->triggerhash,pt->comment)) == 0 )
            {
                if ( jsonstr != 0 )
                    sprintf(pt->comment+strlen(pt->comment)-1,",\"error\":[%s]}",jsonstr!=0?jsonstr:"submit_trade failed");
                free(jsonstr);
                half->T.error = 1;
            }
        }
        else
        {
            printf("%p send sell.%d dir.%d closed.%d\n",half,half->T.sell,dir,half->T.closed);
            stats = get_nodestats(otherNXT(pt,half));
            if ( stats != 0 && stats->ipbits != 0 )
            {
                sprintf(cmdstr,"{\"requestType\":\"respondtx\",\"offerNXT\":\"%llu\",\"NXT\":\"%llu\",\"cmd\":\"%s\",\"assetid\":\"%llu\",\"quantityQNT\":\"%llu\",\"priceNQT\":\"%llu\",\"triggerhash\":\"%s\",\"quoteid\":\"%llu\",\"sig\":\"%s\",\"utx\":\"%s\"}",(long long)pt->nxt64bits,(long long)pt->nxt64bits,(dir > 0) ? "buy" : "sell",(long long)half->T.assetid,(long long)half->T.qty,(long long)half->T.priceNQT,pt->triggerhash,(long long)pt->quoteid,pt->feesighash,pt->feeutxbytes);
                printf("submit_trade.(%s) to offerNXT.%llu %x\n",cmdstr,(long long)otherNXT(pt,half),stats->ipbits);
                len = construct_tokenized_req(_tokbuf,cmdstr,NXTACCTSECRET);
                expand_ipbits(ipaddr,stats->ipbits);
                pserver = get_pserver(0,ipaddr,0,0);
                call_SuperNET_broadcast(pserver,_tokbuf,len,ORDERBOOK_EXPIRATION);
                send_packet(0,stats->ipbits,0,(uint8_t *)_tokbuf,len);
            }
        }
    }
    return(0);
}

struct NXT_tx *is_valid_trigger(uint64_t *quoteidp,cJSON *triggerjson,char *sender)
{
    char otherNXT[64]; cJSON *commentobj; struct NXT_tx *triggertx;
    if ( (triggertx= set_NXT_tx(triggerjson)) != 0 )
    {
        expand_nxt64bits(otherNXT,triggertx->senderbits);
        if ( strcmp(otherNXT,sender) == 0 && is_feetx(triggertx) != 0 && triggertx->comment[0] != 0 )
        {
            if ( (commentobj= cJSON_Parse(triggertx->comment)) != 0 )
            {
                *quoteidp = get_satoshi_obj(commentobj,"quoteid");
                free_json(commentobj);
                return(triggertx);
            } else printf("couldnt parse triggertx->comment.(%s)\n",triggertx->comment);
        } else printf("otherNXT.%s vs sender.%s is_fee.%d comment[0].%d\n",otherNXT,sender,is_feetx(triggertx),triggertx->comment[0]);
        free(triggertx);
    } else printf("couldnt set_NXT_tx\n");
    return(0);
}

char *respondtx(char *NXTaddr,char *NXTACCTSECRET,char *sender,char *cmdstr,uint64_t assetid,uint64_t qty,uint64_t priceNQT,char *triggerhash,uint64_t quoteid,char *sighash,char *utx,int32_t minperc)
{
    //PARSED OFFER.({"sender":"8989816935121514892","timestamp":20810867,"height":2147483647,"amountNQT":"0","verify":false,"subtype":1,"attachment":{"asset":"7631394205089352260","quantityQNT":"1000","comment":"{\"assetB\":\"1639299849328439538\",\"qtyB\":\"1000000\"}"},"recipientRS":"NXT-CWEE-VXCV-697E-9YKJT","feeNQT":"100000000","senderPublicKey":"25c5fed2690701cf06f267e7c227b1a3c0dfa9c6fc3cdb593b3af6f16d65302f","type":2,"deadline":720,"senderRS":"NXT-CWEE-VXCV-697E-9YKJT","recipient":"8989816935121514892"})
    char retbuf[MAX_JSON_FIELD],calchash[MAX_JSON_FIELD],*jsonstr,*parsed,*submitstr;
    cJSON *json,*obj,*triggerjson;
    struct NXT_tx *triggertx;
    struct InstantDEX_quote *iQ;
    uint64_t checkquoteid,txid,feetxid,fee = INSTANTDEX_FEE;
    int32_t dir = (strcmp(cmdstr,"sell") == 0) ? -1 : 1;
    sprintf(retbuf,"{\"error\":\"did not parse (%s)\"}",utx);
    printf("respondtx.(%s) sig.%s full.%s from (%s)\n",utx,sighash,triggerhash,sender);
    if ( (jsonstr= issue_calculateFullHash(0,utx,sighash)) != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            obj = cJSON_GetObjectItem(json,"fullHash");
            copy_cJSON(calchash,obj);
            if ( strcmp(calchash,triggerhash) == 0 )
            {
                if ( (parsed= issue_parseTransaction(0,utx)) != 0 )
                {
                    stripwhite_ns(parsed,strlen(parsed));
                    printf("PARSED OFFER.(%s) triggerhash.(%s) (%s) offer sender.%s\n",parsed,triggerhash,calchash,sender);
                    if ( (triggerjson= cJSON_Parse(parsed)) != 0 )
                    {
                        if ( (triggertx= is_valid_trigger(&checkquoteid,triggerjson,sender)) != 0 )
                        {
                            if ( checkquoteid == quoteid && (iQ= is_valid_offer(quoteid,dir,assetid,qty,priceNQT)) != 0 )
                            {
                                iQ->matched = 1;
                                sprintf(retbuf,"{\"requestType\":\"respondtx\",\"NXT\":\"%llu\",\"qty\":\"%llu\",\"assetid\":\"%llu\",\"priceNQT\":\"%llu\",\"fee\":\"%llu\",\"quoteid\":\"%llu\",\"minperc\":\"%u\"}",(long long)calc_nxt64bits(NXTaddr),(long long)qty,(long long)assetid,(long long)priceNQT,(long long)fee,(long long)quoteid,minperc);
                                feetxid = send_feetx(NXT_ASSETID,fee,triggerhash,retbuf);
                                sprintf(retbuf+strlen(retbuf)-1,",\"feetxid\":\"%llu\"}",(long long)feetxid);
                                if ( (txid= submit_to_exchange(INSTANTDEX_NXTAEID,&submitstr,assetid,qty,priceNQT,dir,calc_nxt64bits(NXTaddr),NXTACCTSECRET,triggerhash,retbuf)) == 0 )
                                    sprintf(retbuf,"{\"error\":[%s],\"submit_txid\":\"%llu\",\"quoteid\":\"%llu\"}",submitstr == 0 ? "submit error" : submitstr,(long long)txid,(long long)quoteid), free(submitstr);
                                else sprintf(retbuf,"{\"result\":\"%s\",:\"submit_txid\":\"%llu\",:\"quoteid\":\"%llu\"}",cmdstr,(long long)txid,(long long)quoteid);
                            } else printf("invalid offer or quoteid mismatch %llu vs %llu\n",(long long)checkquoteid,(long long)quoteid);
                            free(triggertx);
                        } else printf("invalid trigger\n");
                        free_json(triggerjson);
                    } free(parsed);
                } else printf("couldnt parse.(%s)\n",utx);
            } else printf("triggerhash mismatch (%s) != (%s)\n",calchash,triggerhash);
            free_json(json);
        } free(jsonstr);
    }
    return(clonestr(retbuf));
}

char *makeoffer3(char *NXTaddr,char *NXTACCTSECRET,double price,double volume,int32_t flip,uint64_t srcqty,uint64_t baseid,uint64_t relid,cJSON *baseobj,cJSON *relobj,uint64_t quoteid,int32_t askoffer,char *exchange,uint64_t baseamount,uint64_t relamount,uint64_t offerNXT,int32_t minperc,uint64_t jumpasset)
{
    struct pending_offer *pt = calloc(1,sizeof(*pt));
    struct pendinghalf *seller,*buyer,*base,*rel; struct NXT_tx T; char buf[MAX_JSON_FIELD]; int32_t dir;
    pt->nxt64bits = calc_nxt64bits(NXTaddr);
    printf("makeoffer3 %llu offer %llu\n",(long long)pt->nxt64bits,(long long)offerNXT);
    if ( pt->nxt64bits == offerNXT )
        return(clonestr("{\"error\":\"cant match your own offer\"}"));
    if ( minperc == 0 )
        minperc = INSTANTDEX_MINVOL;
    if ( flip != 0 )
        askoffer = 1;
    pt->sell = askoffer, pt->flip = flip;
    dir = (askoffer == 0) ? 1 : -1;
    pt->baseid = baseid, pt->baseamount = baseamount, pt->relid = relid, pt->relamount = relamount, pt->quoteid = quoteid, pt->srcqty = srcqty, pt->price = price, pt->volume = volume, pt->offerNXT = offerNXT;
    base = &pt->base, rel = &pt->rel;
    if ( baseobj != 0 && relobj != 0 )
    {
        if ( extract_pendinghalf(pt,dir,base,baseobj,baseid,jumpasset) > 0 )
            pt->fee += INSTANTDEX_FEE;
        if ( extract_pendinghalf(pt,-dir,rel,relobj,relid,jumpasset) > 0 )
            pt->fee += INSTANTDEX_FEE;
        if ( pt->fee < INSTANTDEX_FEE )
            pt->fee = INSTANTDEX_FEE;
        if ( scale_qty(pt,NXTaddr,base,rel) < 0 )
            return(pending_offer_error(pt));
    }
    else
    {
        pt->fee = INSTANTDEX_FEE;
        if ( baseid == NXT_ASSETID )
        {
            dir = -dir;
            set_pendinghalf(pt,-dir,base,relid,relamount,baseid,baseamount,quoteid,pt->nxt64bits,offerNXT,exchange), pt->ratio = calc_asset_QNT(base,pt->nxt64bits,1,pt->srcqty);
            set_pendinghalf(pt,dir,rel,relid,relamount,baseid,baseamount,quoteid,pt->nxt64bits,offerNXT,exchange), pt->ratio = calc_asset_QNT(rel,pt->nxt64bits,1,pt->srcqty);
        }
        else
        {
            set_pendinghalf(pt,-dir,base,baseid,baseamount,relid,relamount,quoteid,pt->nxt64bits,offerNXT,exchange), pt->ratio = calc_asset_QNT(base,pt->nxt64bits,1,pt->srcqty);
            set_pendinghalf(pt,dir,rel,baseid,baseamount,relid,relamount,quoteid,pt->nxt64bits,offerNXT,exchange), pt->ratio = calc_asset_QNT(rel,pt->nxt64bits,1,pt->srcqty);
        }
    }
    if ( (pt->ratio * 100.) < minperc && (base->T.exchangeid == INSTANTDEX_EXCHANGEID || rel->T.exchangeid == INSTANTDEX_EXCHANGEID) )
    {
        sprintf(pt->comment,"{\"error\":\"not enough volume\",\"minperc\":%d,\"ratio\":%.2f}",minperc,pt->ratio*100);
        return(pending_offer_error(pt));
    }
    sprintf(pt->comment,"{\"requestType\":\"makeoffer3\",\"NXT\":\"%llu\",\"ratio\":\"%.8f\",\"srcqty\":\"%llu\",\"baseid\":\"%llu\",\"relid\":\"%llu\",\"frombase\":\"%llu\",\"fromrel\":\"%llu\",\"tobase\":\"%llu\",\"torel\":%llu,\"qtyA\":\"%llu\",\"priceNQTA\":\"%llu\",\"qtyB\":\"%llu\",\"priceNQTB\":\"%llu\",\"fee\":\"%llu\",\"quoteid\":\"%llu\",\"minperc\":\"%u\",\"jumpasset\":\"%llu\"}",(long long)calc_nxt64bits(NXTaddr),pt->ratio,(long long)pt->srcqty,(long long)pt->baseid,(long long)pt->relid,(long long)base->baseamount,(long long)base->relamount,(long long)rel->baseamount,(long long)rel->relamount,(long long)base->T.qty,(long long)base->T.priceNQT,(long long)rel->T.qty,(long long)rel->T.priceNQT,(long long)pt->fee,(long long)pt->quoteid,minperc,(long long)jumpasset);
    printf("GOT.(%s)\n",pt->comment);
    set_NXTtx(pt->nxt64bits,&T,NXT_ASSETID,pt->fee,calc_nxt64bits(INSTANTDEX_ACCT),-1);
    strcpy(T.comment,pt->comment);
    if ( (pt->feetx= sign_NXT_tx(pt->feeutxbytes,pt->feesignedtx,NXTACCTSECRET,pt->nxt64bits,&T,0,1.)) != 0 )
    {
        get_txhashes(pt->feesighash,pt->triggerhash,pt->feetx);
        pt->expiration = calc_expiration(pt->feetx);
        sprintf(pt->comment + strlen(pt->comment) - 1,",\"feetxid\":\"%llu\",\"triggerhash\":\"%s\"}",(long long)pt->feetx->txid,pt->triggerhash);
        strcpy(buf,pt->comment);
        if ( strlen(pt->triggerhash) == 64 )
        {
            if ( dir > 0 )
                seller = &pt->base, buyer = &pt->rel;
            else seller = &pt->rel, buyer = &pt->base;
            if ( submit_trade(-1,seller,buyer,pt,NXTACCTSECRET) < 0 )
                return(pending_offer_error(pt));
            if ( submit_trade(1,buyer,seller,pt,NXTACCTSECRET) < 0 )
                return(pending_offer_error(pt));
            pt->endmilli = milliseconds() + 2. * JUMPTRADE_SECONDS * 1000;
            //printf("queue pt.%p expiration %f feetxid.%llu\n",pt,pt->expiration,pt->feetx->txid);
            queue_enqueue("pending_offer",&Pending_offersQ.pingpong[0],pt);
        } else printf("invalid triggerhash.(%s).%ld\n",pt->triggerhash,strlen(pt->triggerhash));
        return(clonestr(pt->comment));
    }
    return(clonestr("{\"error\":\"couldnt submit fee tx\"}"));
}

#endif

