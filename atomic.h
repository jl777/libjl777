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
    uint16_t exchangeid; uint16_t sell:1,needsubmit:1,sent:1,unconf:1,closed:1,error:1,aeflag:1,transfer:1;
};

struct pendinghalf
{
    char exchange[16];
    uint64_t buyer,seller,baseamount,relamount,avail,feetxid;
    double price,vol;
    struct tradeinfo T;
};

struct pendingpair
{
    char exchange[64];
    uint64_t nxt64bits,baseid,relid,quoteid,baseamount,relamount,offerNXT,srcqty;
    double price,volume,ratio;
    int notxfer,sell;
};

struct pending_offer
{
    char comment[MAX_JSON_FIELD],feeutxbytes[MAX_JSON_FIELD],feesignedtx[MAX_JSON_FIELD],exchange[64],triggerhash[65],feesighash[65];
    struct NXT_tx *feetx;
    uint64_t actual_feetxid,fee,nxt64bits,baseid,relid,quoteid,baseamount,relamount,srcqty,srcarg,srcamount;
    double ratio,endmilli,price,volume;
    int32_t errcode,sell,numhalves;
    uint32_t expiration;
    struct pendingpair A,B;
    struct pendinghalf halves[4];
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

void update_openorder(struct InstantDEX_quote *iQ,uint64_t quoteid,struct NXT_tx *txptrs[],int32_t updateNXT) // autoresponse to own open orders
{
    //printf("update_openorder iQ.%llu with tx.%llu\n",(long long)iQ->quoteid,(long long)tx->txid);
    // regen orderbook and see if it crosses
    // updatestats
}

uint64_t otherNXT(struct pendinghalf *half)
{
    uint64_t otherNXT;
    if ( half->T.sell == 0 )
        otherNXT = half->seller;
    else otherNXT = half->buyer;
    //if ( otherNXT != pt->offerNXT )
    //    printf("otherNXT error %llu != %llu\n",(long long)otherNXT,(long long)pt->offerNXT);
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

int32_t is_feetx(struct NXT_tx *tx)
{
    if ( tx->recipientbits == calc_nxt64bits(INSTANTDEX_ACCT) && tx->type == 0 && tx->subtype == 0 && tx->U.amountNQT >= INSTANTDEX_FEE )
        return(1);
    else return(0);
}

// polling functions, mostly for sender
int32_t matches_halfquote(struct pendinghalf *half,struct NXT_tx *tx)
{
    //printf("tx quoteid.%llu %llu\n",(long long)tx->quoteid,(long long)half->T.quoteid);
    if ( tx->quoteid != half->T.quoteid )
        return(0);
    if ( Debuglevel > 1 )
        printf("sell.%d tx type.%d subtype.%d otherNXT.%llu sender.%llu myasset.%llu txasset.%llu Uamount.%llu qty.%llu\n",half->T.sell,tx->type,tx->subtype,(long long)otherNXT(half),(long long)tx->senderbits,(long long)myasset(half),(long long)tx->assetidbits,(long long)tx->U.amountNQT,(long long)half->T.qty);
    if ( tx->type == 0 )
    {
        if ( tx->subtype != 0 || otherNXT(half) != tx->senderbits || myasset(half) != NXT_ASSETID )
            return(0);
        return(tx->U.amountNQT == half->T.qty);
    }
    else if ( tx->type == 2 && tx->U.quantityQNT == half->T.qty && otherNXT(half) == tx->senderbits )
    {
        if ( tx->subtype == 1 )
            return(myasset(half) == tx->assetidbits);
        return(tx->priceNQT == half->T.priceNQT && tx->subtype == (3 - half->T.sell) && tradedasset(half) == tx->assetidbits);
    }
    return(0);
}

int32_t pendinghalf_is_complete(struct pendinghalf *half,char *triggerhash,struct NXT_tx *txptrs[])
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
        decode_hex(refhash.bytes,sizeof(refhash),triggerhash);
        for (i=0; i<MAX_TXPTRS; i++)
        {
            if ( (tx= txptrs[i]) == 0 )
                break;
            if ( tx->senderbits == otherNXT(half) && memcmp(refhash.bytes,tx->refhash.bytes,sizeof(refhash)) == 0 )
            {
                if ( half->feetxid == 0 && is_feetx(tx) != 0 )
                {
                    half->feetxid = tx->txid;
                    printf("got fee from offerNXT.%llu\n",(long long)otherNXT(half));
                }
                if ( half->T.txid == 0 && half->T.quoteid != 0 )
                {
                    if ( matches_halfquote(half,tx) != 0 )
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

int32_t process_Pending_offersQ(struct pending_offer **ptp,void **ptrs)
{
    char *NXTACCTSECRET; struct NXT_tx **txptrs; struct pending_offer *pt = *ptp;
    int32_t i;
    NXTACCTSECRET = ptrs[0], txptrs = ptrs[1];
    for (i=0; i<pt->numhalves; i++)
        if ( pt->halves[i].T.error != 0 )
            break;
    if ( milliseconds() > pt->endmilli || i != pt->numhalves )
    {
        pt->errcode = -1;
        printf("(%f > %f) expired pending trade.(%s) || errors: seller.%d buyer.%d seller2.%d buyer2.%d\n",milliseconds(),pt->endmilli,pt->comment,pt->halves[0].T.error,pt->halves[1].T.error,pt->halves[2].T.error,pt->halves[3].T.error);
        return(-1);
    }
    for (i=0; i<pt->numhalves; i++)
        if ( pendinghalf_is_complete(&pt->halves[i],pt->triggerhash,txptrs) == 0 )
            break;
    if ( i == pt->numhalves )
    {
        if ( pt->feetx != 0 && (pt->actual_feetxid= issue_broadcastTransaction(&pt->errcode,0,pt->feesignedtx,NXTACCTSECRET)) != pt->feetx->txid )
        {
            printf("Jump trades triggered! feetxid.%llu but unexpected should have been %llu\n",(long long)pt->actual_feetxid,(long long)pt->feetx->txid);
            for (i=0; i<pt->numhalves; i++)
                pt->halves[i].T.closed = 1;
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
                        /*if ( iQ->closed != 0 || iQ->baseid != baseid || iQ->relid != relid || calc_quoteid(iQ) != quoteid )
                            printf("error: isclosed.%d %llu/%llu != %llu/%llu: iQ.%p quoteid.%llu vs %llu\n",iQ->closed,(long long)iQ->baseid,(long long)iQ->relid,(long long)baseid,(long long)relid,iQ,(long long)calc_quoteid(iQ),(long long)quoteid);
                        else*/
                            update_openorder(iQ,quoteid,txptrs,NXTblock == prevNXTblock);
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

// process sending
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
        if ( half->T.assetid == NXT_ASSETID )
            half->T.priceNQT = 1;
        else half->T.priceNQT = (half->relamount * ap->mult + ap->mult/2) / half->baseamount;
        printf("asset.%llu: priceNQT.%llu amounts.(%llu %llu)  -> %llu | mult.%llu\n",(long long)half->T.assetid,(long long)half->T.priceNQT,(long long)half->baseamount,(long long)half->relamount,(long long)half->baseamount / ap->mult,(long long)ap->mult);
        if ( (half->T.qty= half->baseamount / ap->mult) == 0 )
            return(0);
        if ( srcqty != 0 && srcqty < half->T.qty )
        {
            ratio = (double)srcqty / half->T.qty;
            half->baseamount *= ratio, half->relamount *= ratio;
            half->price = calc_price_volume(&half->vol,half->baseamount,half->relamount);
            half->T.priceNQT = (half->relamount * ap->mult + ap->mult/2) / half->baseamount;
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

uint64_t need_to_submithalf(struct pendinghalf *half,int32_t dir,struct pendingpair *pt,int32_t simpleflag)
{
    printf("need_to_submithalf dir.%d exch.%d T.sell.%d %llu/%llu %llu\n",dir,half->T.exchangeid,half->T.sell,(long long)half->T.assetid,(long long)half->T.relid,(long long)otherasset(half));
    if ( half->T.exchangeid == INSTANTDEX_EXCHANGEID )
    {
        if ( dir < 0 && half->T.sell != 0 && tradedasset(half) == half->T.assetid )
            half->T.aeflag = simpleflag;
        return(otherNXT(half));
    }
    else if ( half->T.exchangeid == INSTANTDEX_NXTAEID || half->T.exchangeid == INSTANTDEX_UNCONFID )
    {
        if ( (dir > 0 && half->T.sell == 0) || (dir < 0 && half->T.sell != 0) )
        {
            if ( simpleflag != 0 )
            {
                if ( myasset(half) != NXT_ASSETID )
                    half->T.closed = 1;
                else half->T.aeflag = 1;
            } else half->T.aeflag = 1;
            return(1);
        }
        half->T.closed = 1;
        printf("%p close half dir.%d\n",half,dir);
    }
    return(0);
}

int32_t set_pendinghalf(struct pendingpair *pt,int32_t dir,struct pendinghalf *half,uint64_t assetid,uint64_t baseamount,uint64_t relid,uint64_t relamount,uint64_t quoteid,uint64_t buyer,uint64_t seller,char *exchangestr,int32_t closeflag)
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
        half->T.needsubmit = need_to_submithalf(half,dir,pt,closeflag);
        printf("%s exchange.%d sethalf other.%llu asset.%llu | myasset.%llu tradedasset.%llu | closed.%d needsubmit.%d AE.%d\n",exchangestr,half->T.exchangeid,(long long)otherNXT(half),(long long)otherasset(half),(long long)myasset(half),(long long)tradedasset(half),half->T.closed,half->T.needsubmit,half->T.aeflag);
        if ( otherNXT(half) != 0 && half->baseamount != 0 && half->relamount != 0 )
            return(half->T.exchangeid == INSTANTDEX_EXCHANGEID);
    }
    half->T.error = 1;
    return(-1);
}

char *set_buyer_seller(struct pendinghalf *seller,struct pendinghalf *buyer,struct pendingpair *pt,struct pending_offer *offer,int32_t dir,int32_t minperc,uint64_t srcqty)
{
    char assetidstr[64],NXTaddr[64]; uint64_t basemult,relmult,qty; int64_t balance,unconfirmed; double price,volume;
    expand_nxt64bits(NXTaddr,pt->nxt64bits);
    basemult = get_assetmult(pt->baseid), relmult = get_assetmult(pt->relid);
    if ( pt->baseid == NXT_ASSETID )
    {
        pt->notxfer = 1;
        set_pendinghalf(pt,-dir,seller,pt->relid,pt->relamount,pt->baseid,pt->baseamount,pt->quoteid,pt->nxt64bits,pt->offerNXT,pt->exchange,1);
        pt->ratio = calc_asset_QNT(seller,pt->nxt64bits,0,pt->srcqty);
        printf("base (%llu -> %llu) ratio.%f\n",(long long)seller->T.qty,(long long)buyer->T.qty,pt->ratio);
        set_pendinghalf(pt,dir,buyer,pt->relid,pt->relamount,pt->baseid,pt->baseamount,pt->quoteid,pt->nxt64bits,pt->offerNXT,pt->exchange,1);
        pt->ratio = calc_asset_QNT(buyer,pt->nxt64bits,1,pt->srcqty);
        printf("rel (%llu -> %llu) ratio.%f\n",(long long)seller->T.qty,(long long)buyer->T.qty,pt->ratio);
    }
    else if ( pt->relid == NXT_ASSETID )
    {
        pt->notxfer = 1;
        set_pendinghalf(pt,-dir,seller,pt->baseid,pt->baseamount,pt->relid,pt->relamount,pt->quoteid,pt->nxt64bits,pt->offerNXT,pt->exchange,1);
        pt->ratio = calc_asset_QNT(seller,pt->nxt64bits,1,pt->srcqty);
        printf("base (%llu -> %llu) ratio.%f\n",(long long)seller->T.qty,(long long)buyer->T.qty,pt->ratio);
        set_pendinghalf(pt,dir,buyer,pt->baseid,pt->baseamount,pt->relid,pt->relamount,pt->quoteid,pt->nxt64bits,pt->offerNXT,pt->exchange,1);
        pt->ratio = calc_asset_QNT(buyer,pt->nxt64bits,0,pt->srcqty);
        printf("rel (%llu -> %llu) ratio.%f\n",(long long)seller->T.qty,(long long)buyer->T.qty,pt->ratio);
    }
    else if ( strcmp(pt->exchange,INSTANTDEX_NAME) == 0 )
    {
        pt->notxfer = 0;
        set_pendinghalf(pt,dir,seller,pt->baseid,pt->baseamount,pt->relid,pt->relamount,pt->quoteid,pt->nxt64bits,pt->offerNXT,pt->exchange,1);
        set_pendinghalf(pt,-dir,buyer,pt->relid,pt->relamount,pt->baseid,pt->baseamount,pt->quoteid,pt->nxt64bits,pt->offerNXT,pt->exchange,1);
        seller->T.transfer = buyer->T.transfer = 1;
        seller->T.qty = pt->baseamount / basemult, buyer->T.qty = pt->relamount / relmult;
        printf("prices.(%f) vol %f dir.%d pt->srcqty %d baseqty %d relqty %d\n",pt->price,pt->volume,dir,(int)pt->srcqty,(int)seller->T.qty,(int)buyer->T.qty);
        pt->ratio = 1.;
        if ( (pt->srcqty= srcqty) == 0 )
            pt->srcqty = (dir < 0) ? buyer->T.qty : seller->T.qty;
        if ( dir < 0 && pt->srcqty < buyer->T.qty )
            pt->ratio = ((double)pt->srcqty / buyer->T.qty), seller->T.qty = ((double)seller->T.qty * pt->ratio), buyer->T.qty = pt->srcqty;
        else if ( dir > 0 && pt->srcqty < seller->T.qty )
            pt->ratio = ((double)pt->srcqty / seller->T.qty), buyer->T.qty = ((double)buyer->T.qty * pt->ratio), seller->T.qty = pt->srcqty;
        price = calc_price_volume(&volume,seller->T.qty * basemult,buyer->T.qty * relmult);
        printf("ratio %f prices.(%f %f) vol %f dir.%d pt->srcqty %d baseqty %d relqty %d\n",pt->ratio,price,pt->price,volume,dir,(int)pt->srcqty,(int)seller->T.qty,(int)buyer->T.qty);
        if ( price != pt->price )
        {
            if ( basemult <= relmult )
                seller->T.qty *= (price / pt->price), printf("b adjust %f\n",(price / pt->price));
            else buyer->T.qty *= (pt->price / price), printf("r adjust %f\n",(pt->price / price));
            price = calc_price_volume(&volume,seller->T.qty * basemult,buyer->T.qty * relmult);
        }
        if ( dir > 0 )
            expand_nxt64bits(assetidstr,pt->baseid), qty = seller->T.qty;
        else expand_nxt64bits(assetidstr,pt->relid), qty = buyer->T.qty;
        balance = get_asset_quantity(&unconfirmed,NXTaddr,assetidstr);
        if ( seller->T.qty == 0 || buyer->T.qty == 0 || balance < qty || unconfirmed < qty )
        {
            printf("srcqty.%llu baseqty.%llu price %f vol %f | balance %.8f unconf %.8f\n",(long long)pt->srcqty,(long long)qty,price,volume,dstr(balance),dstr(unconfirmed));
            return(clonestr("{\"error\":\"not enough assets for swap\"}"));
        }
    } else return(clonestr("{\"error\":\"unsupported orderbook entry\"}"));
    if ( (pt->ratio * 100.) < minperc && (seller->T.exchangeid == INSTANTDEX_EXCHANGEID || buyer->T.exchangeid == INSTANTDEX_EXCHANGEID) )
    {
        sprintf(offer->comment,"{\"error\":\"not enough volume\",\"minperc\":%d,\"ratio\":%.2f}",minperc,pt->ratio*100);
        return(pending_offer_error(offer));
    }
    return(0);
}

int32_t submit_trade(cJSON **jsonp,char *whostr,int32_t dir,struct pendinghalf *half,struct pendinghalf *other,struct pending_offer *pt,char *NXTACCTSECRET)
{
    char cmdstr[4096],numstr[64],*jsonstr;
    cJSON *item = 0;
    //struct nodestats *stats;
    if ( half->T.closed != 0 )
        return(0);
    half->T.sent = 1;
    if ( half->T.needsubmit != 0 )
    {
        item = cJSON_CreateObject();
        sprintf(numstr,"%llu",(long long)half->T.assetid), cJSON_AddItemToObject(item,"assetid",cJSON_CreateString(numstr));
        sprintf(numstr,"%lld",(long long)(dir != 0 ? dir : 1) * half->T.qty), cJSON_AddItemToObject(item,"qty",cJSON_CreateString(numstr));
        sprintf(numstr,"%llu",(long long)half->T.priceNQT), cJSON_AddItemToObject(item,"priceNQT",cJSON_CreateString(numstr));
        if ( half->T.aeflag != 0 )
        {
            cJSON_AddItemToObject(item,"action",cJSON_CreateString("nxtae"));
            printf("%p SUBMIT %llu sell.%d dir.%d closed.%d | qty %llu price %llu\n",half,(long long)half->T.assetid,half->T.sell,dir,half->T.closed,(long long)half->T.qty,(long long)half->T.priceNQT);
            if ( NXTACCTSECRET == 0  || NXTACCTSECRET[0] == 0 )
                half->T.closed = 1;
            else if ( (half->T.txid= submit_to_exchange(INSTANTDEX_NXTAEID,&jsonstr,half->T.assetid,half->T.qty,half->T.priceNQT,dir,pt->nxt64bits,NXTACCTSECRET,pt->triggerhash,pt->comment,otherNXT(half))) == 0 )
            {
                if ( jsonstr != 0 )
                    sprintf(pt->comment+strlen(pt->comment)-1,",\"error\":[\"%s\"]}",jsonstr!=0?jsonstr:"submit_trade failed");
                free(jsonstr);
                half->T.error = 1;
                return(-1);
            }
        }
        else
        {
            cJSON_AddItemToObject(item,"action",cJSON_CreateString("transmit"));
            printf("%p send sell.%d dir.%d closed.%d | qty.%llu price %llu\n",half,half->T.sell,dir,half->T.closed,(long long)half->T.qty,(long long)half->T.priceNQT);
            sprintf(cmdstr,"{\"requestType\":\"respondtx\",\"offerNXT\":\"%llu\",\"NXT\":\"%llu\",\"assetid\":\"%llu\",\"quantityQNT\":\"%llu\",\"triggerhash\":\"%s\",\"quoteid\":\"%llu\",\"sig\":\"%s\",\"data\":\"%s\"",(long long)pt->nxt64bits,(long long)pt->nxt64bits,(long long)half->T.assetid,(long long)half->T.qty,pt->triggerhash,(long long)pt->quoteid,pt->feesighash,pt->feeutxbytes);
            if ( half->T.transfer != 0 )
            {
                if ( half->T.sell == 0 )
                    sprintf(cmdstr+strlen(cmdstr),",\"cmd\":\"transfer\",\"otherassetid\":\"%llu\",\"otherqty\":\"%llu\"}",(long long)other->T.assetid,(long long)other->T.qty);
                else { printf("unexpected request for transfer with dir.%d sell.%d\n",dir,half->T.sell); return(-1); }
            } else sprintf(cmdstr+strlen(cmdstr),",\"cmd\":\"%s\",\"priceNQT\":\"%llu\"}",(dir > 0) ? "buy" : "sell",(long long)half->T.priceNQT);
            printf("submit_trade.(%s) to offerNXT.%llu\n",cmdstr,(long long)otherNXT(half));
            if ( NXTACCTSECRET == 0 || NXTACCTSECRET[0] == 0 )
                half->T.closed = 1;
            else submit_respondtx(cmdstr,pt->nxt64bits,NXTACCTSECRET,otherNXT(half));
        }
    }
    if ( jsonp != 0 && *jsonp != 0 && item != 0 )
        cJSON_AddItemToObject(*jsonp,whostr,item);
    return(0);
}

// phasing! https://nxtforum.org/index.php?topic=6490.msg171048#msg171048
char *set_combohalf(struct pendingpair *pt,cJSON *obj,struct pending_offer *offer,uint64_t baseid,uint64_t relid,int32_t askoffer,int32_t dir,int32_t minperc,uint64_t srcqty,double ratio)
{
    char *retstr;
    pt->baseamount = ratio * get_API_nxt64bits(cJSON_GetObjectItem(obj,"baseamount"));
    pt->relamount = ratio * get_API_nxt64bits(cJSON_GetObjectItem(obj,"relamount"));
    pt->quoteid = get_API_nxt64bits(cJSON_GetObjectItem(obj,"quoteid"));
    pt->offerNXT = get_API_nxt64bits(cJSON_GetObjectItem(obj,"offerNXT"));
    copy_cJSON(pt->exchange,cJSON_GetObjectItem(obj,"exchange"));
    pt->nxt64bits = offer->nxt64bits, pt->baseid = baseid, pt->relid = relid, pt->ratio = ratio;
    pt->price = calc_price_volume(&pt->volume,pt->baseamount,pt->relamount);
    pt->sell = askoffer;
    if ( (retstr= set_buyer_seller(&offer->halves[offer->numhalves++],&offer->halves[offer->numhalves++],pt,offer,dir,minperc,srcqty)) != 0 )
    {
        free(offer);
        return(retstr);
    }
    return(0);
}

char *makeoffer3(char *NXTaddr,char *NXTACCTSECRET,double price,double volume,int32_t deprecated,uint64_t srcqty,uint64_t baseid,uint64_t relid,cJSON *baseobj,cJSON *relobj,uint64_t quoteid,int32_t askoffer,char *exchange,uint64_t baseamount,uint64_t relamount,uint64_t offerNXT,int32_t minperc,uint64_t jumpasset)
{
    struct pending_offer *offer = calloc(1,sizeof(*offer));
    struct NXT_tx T; char whostr[64],*retstr; int32_t i,dir,polarity; cJSON *json; struct pendingpair *pt;
    offer->nxt64bits = calc_nxt64bits(NXTaddr);
    printf("makeoffer3 %llu offer %llu\n",(long long)offer->nxt64bits,(long long)offerNXT);
    if ( offer->nxt64bits == offerNXT )
        return(clonestr("{\"error\":\"cant match your own offer\"}"));
    strcpy(offer->exchange,exchange);
    if ( minperc == 0 )
        minperc = INSTANTDEX_MINVOL;
    offer->sell = askoffer;
    dir = (askoffer == 0) ? 1 : -1;
    offer->baseid = baseid, offer->baseamount = baseamount, offer->relid = relid, offer->relamount = relamount;
    offer->quoteid = quoteid, offer->price = price, offer->volume = volume, offer->A.offerNXT = offerNXT, offer->srcqty = srcqty;
    sprintf(offer->comment,"{\"requestType\":\"makeoffer3\",\"askoffer\":\"%d\",\"NXT\":\"%llu\",\"ratio\":\"%.8f\",\"srcqty\":\"%llu\",\"baseid\":\"%llu\",\"relid\":\"%llu\",\"baseamount\":\"%llu\",\"relamount\":\"%llu\",\"fee\":\"%llu\",\"quoteid\":\"%llu\",\"minperc\":\"%u\",\"jumpasset\":\"%llu\"}",askoffer,(long long)calc_nxt64bits(NXTaddr),offer->ratio,(long long)offer->srcqty,(long long)baseid,(long long)relid,(long long)baseamount,(long long)relamount,(long long)offer->fee,(long long)quoteid,minperc,(long long)jumpasset);
    printf("GOT.(%s)\n",offer->comment);
    pt = &offer->A;
    if ( baseobj != 0 && relobj != 0 )
    {
        if ( (retstr= set_combohalf(&offer->A,baseobj,offer,baseid,jumpasset,askoffer,dir,minperc,srcqty,1.)) != 0 )
        {
            free(offer);
            return(retstr);
        }
        if ( (retstr= set_combohalf(&offer->B,relobj,offer,relid,jumpasset,askoffer,dir,minperc,0,offer->A.ratio)) != 0 )
        {
            free(offer);
            return(retstr);
        }
        offer->fee = 2 * INSTANTDEX_FEE;
    }
    else
    {
        strcpy(pt->exchange,offer->exchange);
        pt->nxt64bits = offer->nxt64bits, pt->baseid = offer->baseid, pt->baseamount = offer->baseamount, pt->relid = offer->relid, pt->relamount = offer->relamount, pt->quoteid = quoteid, pt->offerNXT = offerNXT, pt->srcqty = srcqty, pt->price = price, pt->volume = volume, pt->sell = offer->sell, pt->ratio = 1.;
        if ( (retstr= set_buyer_seller(&offer->halves[offer->numhalves++],&offer->halves[offer->numhalves++],&offer->A,offer,dir,minperc,srcqty)) != 0 )
        {
            free(offer);
            return(retstr);
        }
        offer->fee = INSTANTDEX_FEE;
    }
    set_NXTtx(offer->nxt64bits,&T,NXT_ASSETID,offer->fee,calc_nxt64bits(INSTANTDEX_ACCT),-1);
    strcpy(T.comment,offer->comment);
    if ( NXTACCTSECRET == 0 || NXTACCTSECRET[0] == 0 || (offer->feetx= sign_NXT_tx(offer->feeutxbytes,offer->feesignedtx,NXTACCTSECRET,offer->nxt64bits,&T,0,1.)) != 0 )
    {
        offer->expiration = get_txhashes(offer->feesighash,offer->triggerhash,(NXTACCTSECRET == 0 || NXTACCTSECRET[0] == 0) ? &T : offer->feetx);
        sprintf(offer->comment + strlen(offer->comment) - 1,",\"feetxid\":\"%llu\",\"triggerhash\":\"%s\"}",(long long)(offer->feetx != 0 ? offer->feetx->txid : 0x1234),offer->triggerhash);
        json = cJSON_Parse(offer->comment);
        if ( strlen(offer->triggerhash) == 64 )
        {
            polarity = 1;
            pt = &offer->A;
            for (i=0; i<offer->numhalves; i+=2)
            {
                sprintf(whostr,"%s%s",(-polarity < 0) ? "seller" : "buyer",(i == 0) ? "" : "2");
                if ( submit_trade(&json,whostr,-polarity * pt->notxfer,&offer->halves[i],&offer->halves[i+1],offer,NXTACCTSECRET) < 0 )
                    return(pending_offer_error(offer));
                sprintf(whostr,"%s%s",(polarity < 0) ? "seller" : "buyer",(i == 0) ? "" : "2");
                if ( submit_trade(&json,whostr,polarity * pt->notxfer,&offer->halves[i+1],&offer->halves[i],offer,NXTACCTSECRET) < 0 )
                    return(pending_offer_error(offer));
                polarity = -polarity;
                pt = &offer->B;
            }
            offer->endmilli = milliseconds() + 2. * JUMPTRADE_SECONDS * 1000;
            queue_enqueue("pending_offer",&Pending_offersQ.pingpong[0],offer);
        } else printf("invalid triggerhash.(%s).%ld\n",offer->triggerhash,strlen(offer->triggerhash));
        return(cJSON_Print(json));
    }
    return(clonestr("{\"error\":\"couldnt submit fee tx\"}"));
}

// event driven respondtx
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

struct InstantDEX_quote *is_valid_offer(uint64_t quoteid,int32_t dir,uint64_t assetid,uint64_t qty,uint64_t priceNQT,uint64_t senderbits,uint64_t otherassetid,uint64_t otherqty)
{
    int32_t polarity;
    double price,vol,refprice,refvol;
    struct InstantDEX_quote *iQ;
    uint64_t baseamount,relamount;
    if ( (iQ= findquoteid(quoteid,0)) != 0 && iQ->matched == 0 )
    {
        if ( dir == 0 )
        {
            printf("need to validate iQ details\n"); // combo orderbook entries, polling, automatch
            polarity = (iQ->isask == 0) ? -1 : 1;
            dir = polarity;
        } else polarity = (iQ->isask != 0) ? -1 : 1;
        if ( Debuglevel > 1 )
            printf("found quoteid.%llu polarity.%d %llu/%llu vs %llu dir.%d\n",(long long)quoteid,polarity,(long long)iQ->baseid,(long long)iQ->relid,(long long)assetid,dir);
        //found quoteid.7555841528599494229 polarity.1 6932037131189568014/6854596569382794790 vs 6854596569382794790 dir.1
        if ( polarity*dir > 0 && ((polarity > 0 && iQ->baseid == assetid) || (polarity < 0 && iQ->relid == assetid)) )
        {
            if ( priceNQT != 0 )
                baseamount = calc_baseamount(&relamount,assetid,qty,priceNQT);
            else baseamount = qty * get_assetmult(assetid), relamount = otherqty * get_assetmult(otherassetid);
            if ( polarity > 0 )
                price = calc_price_volume(&vol,baseamount,relamount), refprice = calc_price_volume(&refvol,iQ->baseamount,iQ->relamount);
            else price = calc_price_volume(&vol,relamount,baseamount), refprice = calc_price_volume(&refvol,iQ->relamount,iQ->baseamount);
            if ( Debuglevel > 1 )
                printf("polarity.%d dir.%d (%f %f) vs ref.(%f %f)\n",polarity,dir,price,vol,refprice,refvol);
            if ( vol >= refvol*(double)iQ->minperc/100. && vol <= refvol )
            {
                if ( (dir > 0 && price <= (refprice * (1. + INSTANTDEX_PRICESLIPPAGE) + SMALLVAL)) || (dir < 0 && price >= (refprice / (1. + INSTANTDEX_PRICESLIPPAGE) - SMALLVAL)) )
                    return(iQ);
            }
        }
    } else printf("couldnt find quoteid.%llu iQ.%p\n",(long long)quoteid,iQ);
    return(0);
}

char *check_ordermatch(char *NXTaddr,char *NXTACCTSECRET,struct InstantDEX_quote *iQ,char *submitstr)
{
    return(submitstr);
}

char *respondtx(char *NXTaddr,char *NXTACCTSECRET,char *sender,char *cmdstr,uint64_t assetid,uint64_t qty,uint64_t priceNQT,char *triggerhash,uint64_t quoteid,char *sighash,char *utx,int32_t minperc,uint64_t otherassetid,uint64_t otherqty)
{
    //PARSED OFFER.({"sender":"8989816935121514892","timestamp":20810867,"height":2147483647,"amountNQT":"0","verify":false,"subtype":1,"attachment":{"asset":"7631394205089352260","quantityQNT":"1000","comment":"{\"assetB\":\"1639299849328439538\",\"qtyB\":\"1000000\"}"},"recipientRS":"NXT-CWEE-VXCV-697E-9YKJT","feeNQT":"100000000","senderPublicKey":"25c5fed2690701cf06f267e7c227b1a3c0dfa9c6fc3cdb593b3af6f16d65302f","type":2,"deadline":720,"senderRS":"NXT-CWEE-VXCV-697E-9YKJT","recipient":"8989816935121514892"})
    char retbuf[MAX_JSON_FIELD],calchash[MAX_JSON_FIELD],*jsonstr,*parsed,*submitstr;
    cJSON *json,*obj,*triggerjson;
    struct NXT_tx *triggertx;
    struct InstantDEX_quote *iQ;
    uint64_t checkquoteid,txid,feetxid,fee = INSTANTDEX_FEE;
    int32_t dir;
    if ( strcmp(cmdstr,"transfer") == 0 )
        dir = 0;
    else dir = (strcmp(cmdstr,"sell") == 0) ? -1 : 1;
    sprintf(retbuf,"{\"error\":\"did not parse (%s)\"}",utx);
    printf("dir.%d respondtx.(%s) sig.%s full.%s from (%s)\n",dir,utx,sighash,triggerhash,sender);
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
                            if ( checkquoteid == quoteid && (iQ= is_valid_offer(quoteid,dir,assetid,qty,priceNQT,calc_nxt64bits(sender),otherassetid,otherqty)) != 0 )
                            {
                                sprintf(retbuf,"{\"requestType\":\"respondtx\",\"NXT\":\"%llu\",\"qty\":\"%llu\",\"assetid\":\"%llu\",\"priceNQT\":\"%llu\",\"fee\":\"%llu\",\"quoteid\":\"%llu\",\"minperc\":\"%u\",\"otherassetid\":\"%llu\",\"otherqty\":\"%llu\"}",(long long)calc_nxt64bits(NXTaddr),(long long)qty,(long long)assetid,(long long)priceNQT,(long long)fee,(long long)quoteid,minperc,(long long)otherassetid,(long long)otherqty);
                                feetxid = send_feetx(NXT_ASSETID,fee,triggerhash,retbuf);
                                sprintf(retbuf+strlen(retbuf)-1,",\"feetxid\":\"%llu\"}",(long long)feetxid);
                                if ( (txid= submit_to_exchange(INSTANTDEX_NXTAEID,&submitstr,assetid,qty,priceNQT,dir,calc_nxt64bits(NXTaddr),NXTACCTSECRET,triggerhash,retbuf,calc_nxt64bits(sender))) == 0 )
                                    sprintf(retbuf,"{\"error\":[\"%s\"],\"submit_txid\":\"%llu\",\"quoteid\":\"%llu\"}",submitstr == 0 ? "submit error" : submitstr,(long long)txid,(long long)quoteid), free(submitstr);
                                else
                                {
                                    iQ->matched = 1;
                                    sprintf(retbuf,"{\"result\":\"%s\",:\"submit_txid\":\"%llu\",:\"quoteid\":\"%llu\"}",cmdstr,(long long)txid,(long long)quoteid);
                                }
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

#endif

