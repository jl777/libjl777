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
    uint16_t exchangeid; uint16_t sell:1,needsubmit:1,sent:1,unconf:1,closed:1,error:1,myflag:1,transfer:1;
};

struct pendinghalf
{
    char exchange[32];
    uint64_t buyer,seller,baseamount,relamount,avail,feetxid;
    double price,vol;
    struct tradeinfo T;
};

struct pendingpair
{
    char exchange[64];
    uint64_t nxt64bits,baseid,relid,quoteid,baseamount,relamount,offerNXT;
    double price,volume,ratio;
    int32_t notxfer,sell,perc;
};

struct pending_offer
{
    struct queueitem DL;
    char comment[MAX_JSON_FIELD],feeutxbytes[MAX_JSON_FIELD],feesignedtx[MAX_JSON_FIELD],exchange[64],triggerhash[65],feesighash[65],base[16],rel[16];
    struct NXT_tx *feetx;
    uint64_t actual_feetxid,fee,nxt64bits,baseid,relid,quoteid,baseamount,relamount,srcarg,srcamount,jumpasset;
    double ratio,endmilli,price,volume;
    int32_t errcode,sell,numhalves,perc,minperc;
    uint32_t expiration;
    struct pendingpair A,B;
    struct pendinghalf halves[4];
};

void free_pending_offer(struct pending_offer *offer)
{
    if ( offer->feetx != 0 )
        free(offer->feetx);
    free(offer);
}

char *pending_offer_error(struct pending_offer *offer,cJSON *json)
{
    char *jsonstr;
    if ( offer->errcode == 0 )
        offer->errcode = -10;
    if ( json != 0 )
        jsonstr = cJSON_Print(json), free_json(json);
    else jsonstr = clonestr(offer->comment);
    return(jsonstr);
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

int32_t process_Pending_offersQ(struct pending_offer **offerp,void **ptrs)
{
    char *NXTACCTSECRET; struct NXT_tx **txptrs; struct pending_offer *offer = *offerp;
    int32_t i;
    NXTACCTSECRET = ptrs[0], txptrs = ptrs[1];
    for (i=0; i<offer->numhalves; i++)
        if ( offer->halves[i].T.error != 0 )
            break;
    if ( milliseconds() > offer->endmilli || i != offer->numhalves )
    {
        offer->errcode = -1 - (i != offer->numhalves);
        for (i=0; i<offer->numhalves; i++)
            offer->halves[i].T.closed = 1;
        printf("err.%d (%f > %f) expired pending trade.(%s) || errors: seller.%d buyer.%d seller2.%d buyer2.%d\n",offer->errcode,milliseconds(),offer->endmilli,offer->comment,offer->halves[0].T.error,offer->halves[1].T.error,offer->halves[2].T.error,offer->halves[3].T.error);
        free_pending_offer(offer);
        *offerp = 0;
        return(-1);
    }
    for (i=0; i<offer->numhalves; i++)
        if ( pendinghalf_is_complete(&offer->halves[i],offer->triggerhash,txptrs) == 0 )
            break;
    if ( i == offer->numhalves )
    {
        if ( offer->feetx != 0 && (offer->actual_feetxid= issue_broadcastTransaction(&offer->errcode,0,offer->feesignedtx,NXTACCTSECRET)) != offer->feetx->txid )
        {
            printf("Jump trades triggered! feetxid.%llu but unexpected should have been %llu\n",(long long)offer->actual_feetxid,(long long)offer->feetx->txid);
            for (i=0; i<offer->numhalves; i++)
                offer->halves[i].T.closed = 1;
            free_pending_offer(offer);
            *offerp = 0;
            return(-1);
        }
        else
        {
            printf("Jump trades triggered! feetxid.%llu\n",(long long)offer->actual_feetxid);
            free_pending_offer(offer);
            *offerp = 0;
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
double calc_asset_QNT(struct pendinghalf *half,uint64_t nxt64bits,int32_t checkflag)
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
        /*if ( srcqty != 0 && srcqty < half->T.qty )
        {
            ratio = (double)srcqty / half->T.qty;
            half->baseamount *= ratio, half->relamount *= ratio;
            half->price = calc_price_volume(&half->vol,half->baseamount,half->relamount);
            half->T.priceNQT = (half->relamount * ap->mult + ap->mult/2) / half->baseamount;
            if ( (half->T.qty= half->baseamount / ap->mult) == 0 )
                return(0);
        }
        else if ( half->price == 0. )*/
            half->price = calc_price_volume(&half->vol,half->baseamount,half->relamount);
        balance = get_asset_quantity(&unconfirmed,NXTaddr,assetidstr);
        printf("%s balance %.8f unconfirmed %.8f vs price %llu qty %llu for asset.%s | (%f * %f) * (%ld / %llu)\n",NXTaddr,dstr(balance),dstr(unconfirmed),(long long)half->T.priceNQT,(long long)half->T.qty,assetidstr,half->vol,half->price,SATOSHIDEN,(long long)ap->mult);
        // getchar();
        if ( checkflag != 0 && (balance < half->T.qty || unconfirmed < half->T.qty) )
            return(0);
    } else printf("%llu null apmult\n",(long long)half->T.assetid);
    return(ratio);
}

int32_t cleared_with_nxtae(int32_t exchangeid)
{
    if ( exchangeid == INSTANTDEX_EXCHANGEID || exchangeid == INSTANTDEX_NXTAEID || exchangeid == INSTANTDEX_UNCONFID )
        return(1);
    else return(0);
}

uint64_t need_to_submithalf(struct pendinghalf *half,int32_t dir,struct pendingpair *pt,int32_t simpleflag)
{
    printf("need_to_submithalf dir.%d (%s) T.sell.%d %llu/%llu %llu\n",dir,Exchanges[half->T.exchangeid].name,half->T.sell,(long long)half->T.assetid,(long long)half->T.relid,(long long)otherasset(half));
    if ( half->T.exchangeid == INSTANTDEX_EXCHANGEID )
    {
        if ( tradedasset(half) == half->T.assetid )
            half->T.myflag = 1, half->T.exchangeid = INSTANTDEX_NXTAEID;
        return(1);
    }
    else
    {
        if ( (dir > 0 && half->T.sell == 0) || (dir < 0 && half->T.sell != 0) )
        {
            if ( myasset(half) == NXT_ASSETID && cleared_with_nxtae(half->T.exchangeid) != 0 )
                half->T.closed = 1;
            else
            {
                half->T.myflag = 1;
                if ( half->T.exchangeid == INSTANTDEX_UNCONFID )
                    half->T.exchangeid = INSTANTDEX_NXTAEID;
            }
            return(1);
        }
        half->T.closed = 1;
        printf("%p close half mismatched dir and T.sell: dir.%d sellflag.%d\n",half,dir,half->T.sell);
        return(0);
    }
}

int32_t submit_trade(cJSON **jsonp,char *whostr,int32_t dir,struct pendinghalf *half,struct pendinghalf *other,struct pending_offer *offer,char *NXTACCTSECRET,char *base,char *rel,double price,double volume)
{
    char cmdstr[4096],numstr[64],*jsonstr;
    cJSON *item = 0;
    if ( half->T.closed != 0 )
        return(0);
    half->T.sent = 1;
    if ( half->T.needsubmit != 0 )
    {
        item = cJSON_CreateObject();
        sprintf(numstr,"%llu",(long long)half->T.assetid), cJSON_AddItemToObject(item,"assetid",cJSON_CreateString(numstr));
        sprintf(numstr,"%lld",(long long)(dir != 0 ? dir : 1) * half->T.qty), cJSON_AddItemToObject(item,"qty",cJSON_CreateString(numstr));
        sprintf(numstr,"%llu",(long long)half->T.priceNQT), cJSON_AddItemToObject(item,"priceNQT",cJSON_CreateString(numstr));
        printf("%s SUBMIT %llu sell.%d dir.%d closed.%d | qty %llu price %llu (%s)\n",half->exchange,(long long)half->T.assetid,half->T.sell,dir,half->T.closed,(long long)half->T.qty,(long long)half->T.priceNQT,offer->comment);
        if ( NXTACCTSECRET == 0  || NXTACCTSECRET[0] == 0 )
            half->T.closed = 1;
        if ( half->T.myflag != 0 )
        {
            cJSON_AddItemToObject(item,"action",cJSON_CreateString(Exchanges[half->T.exchangeid].name));
            if ( half->T.closed == 0 )
            {
                if ( (half->T.txid= submit_to_exchange(half->T.exchangeid,&jsonstr,half->T.assetid,half->T.qty,half->T.priceNQT,dir,offer->nxt64bits,NXTACCTSECRET,offer->triggerhash,offer->comment,otherNXT(half),base,rel,price,volume)) == 0 )
                {
                    if ( jsonstr != 0 )
                        sprintf(offer->comment+strlen(offer->comment)-1,",\"error\":[\"%s\"]}",jsonstr!=0?jsonstr:"submit_trade failed");
                    free(jsonstr);
                    half->T.error = 1;
                    printf("failed to submit (%s) %llu sell.%d dir.%d closed.%d | qty %llu price %llu\n",half->exchange,(long long)half->T.assetid,half->T.sell,dir,half->T.closed,(long long)half->T.qty,(long long)half->T.priceNQT);
                    return(-1);
                }
                else
                {
                    sprintf(numstr,"%llu",(long long)half->T.txid), cJSON_AddItemToObject(item,"txid",cJSON_CreateString(numstr));
                    cJSON_AddItemToObject(item,"order",cJSON_CreateString(dir>0?"buy":"sell"));
                }
            }
        }
        else
        {
            cJSON_AddItemToObject(item,"action",cJSON_CreateString("transmit"));
            sprintf(cmdstr,"{\"requestType\":\"respondtx\",\"offerNXT\":\"%llu\",\"NXT\":\"%llu\",\"assetid\":\"%llu\",\"quantityQNT\":\"%llu\",\"triggerhash\":\"%s\",\"quoteid\":\"%llu\",\"sig\":\"%s\",\"data\":\"%s\"",(long long)offer->nxt64bits,(long long)offer->nxt64bits,(long long)half->T.assetid,(long long)half->T.qty,offer->triggerhash,(long long)offer->quoteid,offer->feesighash,offer->feeutxbytes);
            if ( half->T.transfer != 0 )
            {
                if ( half->T.sell == 0 )
                    sprintf(cmdstr+strlen(cmdstr),",\"cmd\":\"transfer\",\"otherassetid\":\"%llu\",\"otherqty\":\"%llu\"}",(long long)other->T.assetid,(long long)other->T.qty);
                else { printf("unexpected request for transfer with dir.%d sell.%d notxfer.%d:%d\n",dir,half->T.sell,offer->A.notxfer,offer->B.notxfer); return(-1); }
            } else sprintf(cmdstr+strlen(cmdstr),",\"cmd\":\"%s\",\"priceNQT\":\"%llu\"}",(dir > 0) ? "buy" : "sell",(long long)half->T.priceNQT);
            printf("submit_trade.(%s) to offerNXT.%llu (%s)\n",cmdstr,(long long)otherNXT(half),offer->comment);
            if ( half->T.closed == 0 )
                submit_respondtx(cmdstr,offer->nxt64bits,NXTACCTSECRET,otherNXT(half));
        }
    }
    if ( jsonp != 0 && *jsonp != 0 && item != 0 )
        cJSON_AddItemToObject(*jsonp,whostr,item);
    return(0);
}

int32_t set_pendinghalf(struct pendingpair *pt,int32_t dir,struct pendinghalf *half,uint64_t assetid,uint64_t baseamount,uint64_t relid,uint64_t relamount,uint64_t quoteid,uint64_t buyer,uint64_t seller,char *exchangestr,int32_t closeflag)
{
    struct exchange_info *exchange;
    printf("dir.%d sethalf %llu/%llu buyer.%llu seller.%llu\n",dir,(long long)assetid,(long long)relid,(long long)buyer,(long long)seller);
    half->T.assetid = assetid, half->T.relid = relid, half->baseamount = baseamount, half->relamount = relamount, half->T.quoteid = quoteid;
    if ( dir > 0 )
        half->buyer = buyer, half->seller = seller;
    else half->buyer = seller, half->seller = buyer;
    strcpy(half->exchange,exchangestr);
    if ( (exchange= find_exchange(half->exchange,0,0)) != 0 )
    {
        half->T.exchangeid = exchange->exchangeid;
        if ( dir < 0 )
            half->T.sell = 1;
        half->T.needsubmit = need_to_submithalf(half,dir,pt,closeflag);
        if ( dir < 0 )
            half->T.sell = pt->notxfer;
        printf("%s exchange.%d sethalf other.%llu asset.%llu | myasset.%llu tradedasset.%llu | closed.%d needsubmit.%d myflag.%d\n",exchangestr,half->T.exchangeid,(long long)otherNXT(half),(long long)otherasset(half),(long long)myasset(half),(long long)tradedasset(half),half->T.closed,half->T.needsubmit,half->T.myflag);
        if ( otherNXT(half) != 0 && half->baseamount != 0 && half->relamount != 0 )
            return(half->T.exchangeid == INSTANTDEX_EXCHANGEID);
    }
    if ( half->T.closed == 0 )
    {
        printf("SETERROR (%s).(%s) -> dir.%d sethalf %llu/%llu buyer.%llu seller.%llu\n",half->exchange,Exchanges[(int32_t)half->T.exchangeid].name,dir,(long long)assetid,(long long)relid,(long long)buyer,(long long)seller);
        half->T.error = 1;
        half->T.closed = 1;
    }
    return(-1);
}

char *set_buyer_seller(struct pendinghalf *seller,struct pendinghalf *buyer,struct pendingpair *pt,struct pending_offer *offer,int32_t dir)
{
    char assetidstr[64],NXTaddr[64]; uint64_t basemult,relmult,qty; int64_t balance,unconfirmed; double price,volume;
    expand_nxt64bits(NXTaddr,pt->nxt64bits);
    basemult = get_assetmult(pt->baseid), relmult = get_assetmult(pt->relid);
    pt->ratio = offer->ratio;
    if ( pt->baseid == NXT_ASSETID )
    {
        pt->notxfer = 1;
        set_pendinghalf(pt,-dir,seller,pt->relid,pt->relamount,pt->baseid,pt->baseamount,pt->quoteid,pt->offerNXT,pt->nxt64bits,pt->exchange,1);
        calc_asset_QNT(seller,pt->nxt64bits,0);
        printf("base (%llu -> %llu) ratio.%f\n",(long long)seller->T.qty,(long long)buyer->T.qty,pt->ratio);
        set_pendinghalf(pt,dir,buyer,pt->relid,pt->relamount,pt->baseid,pt->baseamount,pt->quoteid,pt->nxt64bits,pt->offerNXT,pt->exchange,1);
        calc_asset_QNT(buyer,pt->nxt64bits,1);
        printf("rel (%llu -> %llu) ratio.%f\n",(long long)seller->T.qty,(long long)buyer->T.qty,pt->ratio);
    }
    else if ( pt->relid == NXT_ASSETID )
    {
        pt->notxfer = 1;
        set_pendinghalf(pt,-dir,seller,pt->baseid,pt->baseamount,pt->relid,pt->relamount,pt->quoteid,pt->offerNXT,pt->nxt64bits,pt->exchange,1);
        calc_asset_QNT(seller,pt->nxt64bits,1);
        printf("base (%llu -> %llu) ratio.%f\n",(long long)seller->T.qty,(long long)buyer->T.qty,pt->ratio);
        set_pendinghalf(pt,dir,buyer,pt->baseid,pt->baseamount,pt->relid,pt->relamount,pt->quoteid,pt->nxt64bits,pt->offerNXT,pt->exchange,1);
        calc_asset_QNT(buyer,pt->nxt64bits,0);
        printf("rel (%llu -> %llu) ratio.%f\n",(long long)seller->T.qty,(long long)buyer->T.qty,pt->ratio);
    }
    else if ( strcmp(pt->exchange,INSTANTDEX_NAME) == 0 )
    {
        pt->notxfer = 0;
        set_pendinghalf(pt,dir,seller,pt->baseid,pt->baseamount,pt->relid,pt->relamount,pt->quoteid,pt->offerNXT,pt->nxt64bits,pt->exchange,1);
        set_pendinghalf(pt,-dir,buyer,pt->relid,pt->relamount,pt->baseid,pt->baseamount,pt->quoteid,pt->nxt64bits,pt->offerNXT,pt->exchange,1);
        seller->T.transfer = buyer->T.transfer = 1;
        seller->T.qty = pt->baseamount / basemult, buyer->T.qty = pt->relamount / relmult;
        printf("prices.(%f) vol %f dir.%d pt->perc %d baseqty %d relqty %d\n",pt->price,pt->volume,dir,(int)pt->perc,(int)seller->T.qty,(int)buyer->T.qty);
        price = calc_price_volume(&volume,seller->T.qty * basemult,buyer->T.qty * relmult);
        printf("ratio %f prices.(%f %f) vol %f dir.%d pt->srcqty %d baseqty %d relqty %d\n",pt->ratio,price,pt->price,volume,dir,pt->perc,(int)seller->T.qty,(int)buyer->T.qty);
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
            printf("perc.%u baseqty.%llu price %f vol %f | balance %.8f unconf %.8f\n",pt->perc,(long long)qty,price,volume,dstr(balance),dstr(unconfirmed));
            return(pending_offer_error(offer,cJSON_Parse("{\"error\":\"not enough assets for swap\"}")));
        }
    } else return(pending_offer_error(offer,cJSON_Parse("{\"error\":\"unsupported orderbook entry\"}")));
    return(0);
}

// phasing! https://nxtforum.org/index.php?topic=6490.msg171048#msg171048
char *set_combohalf(struct pendingpair *pt,cJSON *obj,struct pending_offer *offer,uint64_t baseid,uint64_t relid,int32_t askoffer,int32_t dir)
{
    char *retstr;
    pt->baseamount = offer->ratio * get_API_nxt64bits(cJSON_GetObjectItem(obj,"baseamount"));
    pt->relamount = offer->ratio * get_API_nxt64bits(cJSON_GetObjectItem(obj,"relamount"));
    pt->quoteid = get_API_nxt64bits(cJSON_GetObjectItem(obj,"quoteid"));
    pt->offerNXT = get_API_nxt64bits(cJSON_GetObjectItem(obj,"offerNXT"));
    copy_cJSON(pt->exchange,cJSON_GetObjectItem(obj,"exchange"));
    pt->nxt64bits = offer->nxt64bits, pt->baseid = baseid, pt->relid = relid, pt->ratio = offer->ratio;
    pt->price = calc_price_volume(&pt->volume,pt->baseamount,pt->relamount);
    pt->sell = askoffer;
    if ( (retstr= set_buyer_seller(&offer->halves[offer->numhalves++],&offer->halves[offer->numhalves++],pt,offer,dir)) != 0 )
        return(retstr);
    return(0);
}

char *submit_trades(struct pending_offer *offer,char *NXTACCTSECRET)
{
    cJSON *json;
    char whostr[64],*retstr;
    struct pendingpair *pt;
    int32_t i,dir,dir2,polarity = 1;
    json = cJSON_Parse(offer->comment);
    pt = &offer->A;
    for (i=0; i<offer->numhalves; i+=2)
    {
        dir = -2 * offer->halves[i].T.sell + 1;
        dir2 = -2 * offer->halves[i+1].T.sell + 1;
        printf("i.%d polarity.%d T.sells: %d %d | dirs: %d %d\n",i,polarity,offer->halves[i].T.sell,offer->halves[i+1].T.sell,dir,dir2);
        if ( dir*dir2 > 0 )
        {
            cJSON_AddItemToObject(json,"error",cJSON_CreateString("both sides same direction"));
            return(pending_offer_error(offer,json));
        }
        sprintf(whostr,"%s%s",(polarity*dir < 0) ? "seller" : "buyer",(i == 0) ? "" : "2");
        if ( submit_trade(&json,whostr,polarity*dir * pt->notxfer,&offer->halves[i],&offer->halves[i+1],offer,NXTACCTSECRET,offer->base,offer->rel,offer->price,offer->volume) < 0 )
            return(pending_offer_error(offer,json));
        printf("second quarter:\n");
        sprintf(whostr,"%s%s",(polarity*dir2 > 0) ? "seller" : "buyer",(i == 0) ? "" : "2");
        if ( submit_trade(&json,whostr,polarity*dir2 * pt->notxfer,&offer->halves[i+1],&offer->halves[i],offer,NXTACCTSECRET,offer->base,offer->rel,offer->price,offer->volume) < 0 )
            return(pending_offer_error(offer,json));
        polarity = -polarity;
        pt = &offer->B;
    }
    offer->endmilli = milliseconds() + 2. * JUMPTRADE_SECONDS * 1000;
    retstr = cJSON_Print(json);
    printf("endmilli %f vs %f: offer.%p (%s)\n",offer->endmilli,milliseconds(),offer,retstr);
    queue_enqueue("pending_offer",&Pending_offersQ.pingpong[0],&offer->DL);
    free_json(json);
    return(retstr);
}

void create_offer_comment(struct pending_offer *offer)
{
    sprintf(offer->comment,"{\"requestType\":\"makeoffer3\",\"askoffer\":\"%d\",\"NXT\":\"%llu\",\"ratio\":\"%.8f\",\"perc\":\"%d\",\"baseid\":\"%llu\",\"relid\":\"%llu\",\"baseamount\":\"%llu\",\"relamount\":\"%llu\",\"fee\":\"%llu\",\"quoteid\":\"%llu\",\"minperc\":\"%u\",\"jumpasset\":\"%llu\"}",offer->sell,(long long)offer->nxt64bits,offer->ratio,offer->perc,(long long)offer->baseid,(long long)offer->relid,(long long)offer->baseamount,(long long)offer->relamount,(long long)offer->fee,(long long)offer->quoteid,offer->minperc,(long long)offer->jumpasset);
}

char *makeoffer3(char *NXTaddr,char *NXTACCTSECRET,double price,double volume,int32_t deprecated,int32_t perc,uint64_t baseid,uint64_t relid,cJSON *baseobj,cJSON *relobj,uint64_t quoteid,int32_t askoffer,char *exchange,uint64_t baseamount,uint64_t relamount,uint64_t offerNXT,int32_t minperc,uint64_t jumpasset)
{
    struct NXT_tx T; char *retstr; int32_t dir; struct pendingpair *pt; uint64_t mult; struct pending_offer *offer = 0;
    if ( minperc == 0 )
        minperc = INSTANTDEX_MINVOL;
    if ( perc == 0 )
        perc = 100;
    else if ( perc < minperc )
        return(clonestr("{\"error\":\"perc < minperc\"}"));
    offer = calloc(1,sizeof(*offer));
    offer->minperc = minperc;
    offer->jumpasset = jumpasset;
    set_assetname(&mult,offer->base,baseid);
    set_assetname(&mult,offer->rel,relid);
    offer->nxt64bits = calc_nxt64bits(NXTaddr);
    if ( offer->nxt64bits == offerNXT )
        return(clonestr("{\"error\":\"cant match your own offer\"}"));
    if ( perc <= 0 || perc >= 100 )
    {
        offer->ratio = 1.;
        offer->volume = volume;
        offer->baseamount = baseamount;
        offer->relamount = relamount;
    }
    else
    {
        offer->ratio = perc / 100.;
        offer->volume = volume * offer->ratio;
        offer->baseamount = baseamount * offer->ratio;
        offer->relamount = relamount * offer->ratio;
    }
    strcpy(offer->exchange,exchange);
    offer->sell = askoffer;
    dir = (askoffer == 0) ? 1 : -1;
    offer->baseid = baseid,  offer->relid = relid, offer->quoteid = quoteid, offer->price = price, offer->A.offerNXT = offerNXT, offer->perc = perc;
    pt = &offer->A;
    if ( baseobj != 0 && relobj != 0 )
    {
        if ( (retstr= set_combohalf(&offer->A,baseobj,offer,baseid,jumpasset,askoffer,-dir)) != 0 )
            return(retstr);
        if ( (retstr= set_combohalf(&offer->B,relobj,offer,relid,jumpasset,askoffer,-dir)) != 0 )
            return(retstr);
        offer->fee = INSTANTDEX_FEE;
        if ( offer->halves[0].T.exchangeid == INSTANTDEX_EXCHANGEID && offer->halves[2].T.exchangeid == INSTANTDEX_EXCHANGEID )
            offer->fee += INSTANTDEX_FEE;
    }
    else
    {
        strcpy(pt->exchange,offer->exchange);
        pt->nxt64bits = offer->nxt64bits, pt->baseid = offer->baseid, pt->baseamount = offer->baseamount, pt->relid = offer->relid, pt->relamount = offer->relamount, pt->quoteid = quoteid, pt->offerNXT = offerNXT, pt->perc = perc, pt->price = price, pt->volume = volume, pt->sell = offer->sell;
        if ( (retstr= set_buyer_seller(&offer->halves[offer->numhalves++],&offer->halves[offer->numhalves++],&offer->A,offer,dir)) != 0 )
            return(retstr);
        if ( offer->halves[0].T.exchangeid == INSTANTDEX_EXCHANGEID )
            offer->fee = INSTANTDEX_FEE;
        else
        {
            create_offer_comment(offer);
            if ( (retstr= submit_trades(offer,NXTACCTSECRET)) != 0 )
                return(retstr);
            else return(pending_offer_error(offer,cJSON_Parse("{\"error\":\"couldnt submit trade\"}")));
        }
    }
    create_offer_comment(offer);
    printf("GOT.(%s)\n",offer->comment);
    set_NXTtx(offer->nxt64bits,&T,NXT_ASSETID,offer->fee,calc_nxt64bits(INSTANTDEX_ACCT),-1);
    strcpy(T.comment,offer->comment);
    if ( NXTACCTSECRET == 0 || NXTACCTSECRET[0] == 0 || (offer->feetx= sign_NXT_tx(offer->feeutxbytes,offer->feesignedtx,NXTACCTSECRET,offer->nxt64bits,&T,0,1.)) != 0 )
    {
        offer->expiration = get_txhashes(offer->feesighash,offer->triggerhash,(NXTACCTSECRET == 0 || NXTACCTSECRET[0] == 0) ? &T : offer->feetx);
        sprintf(offer->comment + strlen(offer->comment) - 1,",\"feetxid\":\"%llu\",\"triggerhash\":\"%s\"}",(long long)(offer->feetx != 0 ? offer->feetx->txid : 0x1234),offer->triggerhash);
        if ( strlen(offer->triggerhash) == 64 && (retstr= submit_trades(offer,NXTACCTSECRET)) != 0 )
            return(retstr);
    }
    return(pending_offer_error(offer,cJSON_Parse("{\"error\":\"couldnt submit fee tx\"}")));
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
                                if ( (txid= submit_to_exchange(INSTANTDEX_NXTAEID,&submitstr,assetid,qty,priceNQT,dir,calc_nxt64bits(NXTaddr),NXTACCTSECRET,triggerhash,retbuf,calc_nxt64bits(sender),"base","rel",0.,0.)) == 0 )
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

