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
    uint64_t actual_feetxid,fee,nxt64bits,baseid,relid,quoteid,baseamount,relamount,srcarg,srcamount,jumpasset,basemult,relmult;
    double ratio,endmilli,price,volume;
    int32_t errcode,sell,numhalves,perc,minperc;
    uint32_t expiration,triggerheight;
    struct pendingpair A,B;
    struct pendinghalf halves[4];
};
uint64_t submit_to_exchange(int32_t exchangeid,char **jsonstrp,uint64_t assetid,uint64_t qty,uint64_t priceNQT,int32_t dir,uint64_t nxt64bits,char *NXTACCTSECRET,char *triggerhash,char *comment,uint64_t otherNXT,char *base,char *rel,double price,double volume,uint32_t triggerheight);

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

// process sending
double calc_asset_QNT(struct pendinghalf *half,uint64_t nxt64bits,int32_t checkflag)
{
    char NXTaddr[64],assetidstr[64]; double ratio = 1.; int64_t unconfirmed,balance; uint64_t ap_mult;
    expand_nxt64bits(NXTaddr,nxt64bits);
    expand_nxt64bits(assetidstr,half->T.assetid);
    if ( (ap_mult= get_assetmult(half->T.assetid)) != 0 )
    {
        if ( half->T.assetid == NXT_ASSETID )
            half->T.priceNQT = 1;
        else half->T.priceNQT = (half->relamount * ap_mult + ap_mult/2) / half->baseamount;
        printf("asset.%llu: priceNQT.%llu amounts.(%llu %llu)  -> %llu | mult.%llu\n",(long long)half->T.assetid,(long long)half->T.priceNQT,(long long)half->baseamount,(long long)half->relamount,(long long)half->baseamount / ap_mult,(long long)ap_mult);
        if ( (half->T.qty= half->baseamount / ap_mult) == 0 )
            return(0);
        /*if ( srcqty != 0 && srcqty < half->T.qty )
        {
            ratio = (double)srcqty / half->T.qty;
            half->baseamount *= ratio, half->relamount *= ratio;
            half->price = prices777_price_volume(&half->vol,half->baseamount,half->relamount);
            half->T.priceNQT = (half->relamount * ap_mult + ap_mult/2) / half->baseamount;
            if ( (half->T.qty= half->baseamount / ap_mult) == 0 )
                return(0);
        }
        else if ( half->price == 0. )*/
            half->price = prices777_price_volume(&half->vol,half->baseamount,half->relamount);
        balance = get_asset_quantity(&unconfirmed,NXTaddr,assetidstr);
        printf("%s balance %.8f unconfirmed %.8f vs price %llu qty %llu for asset.%s | (%f * %f) * (%lld / %llu)\n",NXTaddr,dstr(balance),dstr(unconfirmed),(long long)half->T.priceNQT,(long long)half->T.qty,assetidstr,half->vol,half->price,(long long)SATOSHIDEN,(long long)ap_mult);
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
    printf("need_to_submithalf dir.%d (%s) T.sell.%d %llu/%llu %llu\n",dir,exchange_str(half->T.exchangeid),half->T.sell,(long long)half->T.assetid,(long long)half->T.relid,(long long)otherasset(half));
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

char *submit_respondtx(char *respondtxstr,uint64_t nxt64bits,char *NXTACCTSECRET,uint64_t dest64bits)
{
    uint32_t nonce; char destNXT[64];
    expand_nxt64bits(destNXT,dest64bits);
    printf("submit_respondtx.(%s) -> dest.%llu\n",respondtxstr,(long long)dest64bits);
    return(busdata_sync(&nonce,respondtxstr,"allnodes",destNXT));
}

int32_t submit_trade(cJSON **jsonp,char *whostr,int32_t dir,struct pendinghalf *half,struct pendinghalf *other,struct pending_offer *offer,char *NXTACCTSECRET,char *base,char *rel,double price,double volume,uint32_t triggerheight)
{
    char cmdstr[4096],numstr[64],*jsonstr,*str;
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
            cJSON_AddItemToObject(item,"action",cJSON_CreateString(exchange_str(half->T.exchangeid)));
            if ( half->T.closed == 0 )
            {
                if ( (half->T.txid= submit_to_exchange(half->T.exchangeid,&jsonstr,half->T.assetid,half->T.qty,half->T.priceNQT,dir,offer->nxt64bits,NXTACCTSECRET,offer->triggerhash,offer->comment,otherNXT(half),base,rel,price,volume,triggerheight)) == 0 )
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
            sprintf(cmdstr,"{\"method\":\"respondtx\",\"offerNXT\":\"%llu\",\"NXT\":\"%llu\",\"assetid\":\"%llu\",\"quantityQNT\":\"%llu\",\"triggerhash\":\"%s\",\"quoteid\":\"%llu\",\"sig\":\"%s\",\"data\":\"%s\"",(long long)offer->nxt64bits,(long long)offer->nxt64bits,(long long)half->T.assetid,(long long)half->T.qty,offer->triggerhash,(long long)offer->quoteid,offer->feesighash,offer->feeutxbytes);
            if ( half->T.transfer != 0 )
            {
                if ( half->T.sell == 0 )
                    sprintf(cmdstr+strlen(cmdstr),",\"cmd\":\"transfer\",\"otherassetid\":\"%llu\",\"otherqty\":\"%llu\"}",(long long)other->T.assetid,(long long)other->T.qty);
                else { printf("unexpected request for transfer with dir.%d sell.%d notxfer.%d:%d\n",dir,half->T.sell,offer->A.notxfer,offer->B.notxfer); return(-1); }
            } else sprintf(cmdstr+strlen(cmdstr),",\"cmd\":\"%s\",\"priceNQT\":\"%llu\"}",(dir > 0) ? "buy" : "sell",(long long)half->T.priceNQT);
            printf("submit_trade.(%s) to offerNXT.%llu (%s)\n",cmdstr,(long long)otherNXT(half),offer->comment);
            if ( half->T.closed == 0 )
                if ( (str= submit_respondtx(cmdstr,offer->nxt64bits,NXTACCTSECRET,otherNXT(half))) != 0 )
                    free(str);
        }
    }
    if ( jsonp != 0 && *jsonp != 0 && item != 0 )
        cJSON_AddItemToObject(*jsonp,whostr,item);
    return(0);
}

int32_t set_pendinghalf(struct pendingpair *pt,int32_t dir,struct pendinghalf *half,uint64_t assetid,uint64_t baseamount,uint64_t relid,uint64_t relamount,uint64_t quoteid,uint64_t buyer,uint64_t seller,char *exchangestr,int32_t closeflag)
{
    int32_t exchangeid; struct exchange_info *exchange;
    printf("dir.%d sethalf %llu/%llu buyer.%llu seller.%llu\n",dir,(long long)assetid,(long long)relid,(long long)buyer,(long long)seller);
    half->T.assetid = assetid, half->T.relid = relid, half->baseamount = baseamount, half->relamount = relamount, half->T.quoteid = quoteid;
    if ( dir > 0 )
        half->buyer = buyer, half->seller = seller;
    else half->buyer = seller, half->seller = buyer;
    strcpy(half->exchange,exchangestr);
    if ( (exchange= find_exchange(&exchangeid,half->exchange)) != 0 )
    {
        half->T.exchangeid = exchangeid;
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
        printf("SETERROR (%s).(%s) -> dir.%d sethalf %llu/%llu buyer.%llu seller.%llu\n",half->exchange,exchange_str(half->T.exchangeid),dir,(long long)assetid,(long long)relid,(long long)buyer,(long long)seller);
        half->T.error = 1;
        half->T.closed = 1;
    }
    return(-1);
}

char *set_buyer_seller(struct pendinghalf *seller,struct pendinghalf *buyer,struct pendingpair *pt,struct pending_offer *offer,int32_t dir)
{
    char assetidstr[64],NXTaddr[64]; uint64_t qty; int64_t balance,unconfirmed; double price,volume;
    expand_nxt64bits(NXTaddr,pt->nxt64bits);
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
        seller->T.qty = pt->baseamount / offer->basemult, buyer->T.qty = pt->relamount / offer->relmult;
        printf("prices.(%f) vol %f dir.%d pt->perc %d baseqty %d relqty %d\n",pt->price,pt->volume,dir,(int)pt->perc,(int)seller->T.qty,(int)buyer->T.qty);
        price = prices777_price_volume(&volume,seller->T.qty * offer->basemult,buyer->T.qty * offer->relmult);
        printf("ratio %f prices.(%f %f) vol %f dir.%d pt->srcqty %d baseqty %d relqty %d\n",pt->ratio,price,pt->price,volume,dir,pt->perc,(int)seller->T.qty,(int)buyer->T.qty);
        if ( price != pt->price )
        {
            if ( offer->basemult <= offer->relmult )
                seller->T.qty *= (price / pt->price), printf("b adjust %f\n",(price / pt->price));
            else buyer->T.qty *= (pt->price / price), printf("r adjust %f\n",(pt->price / price));
            price = prices777_price_volume(&volume,seller->T.qty * offer->basemult,buyer->T.qty * offer->relmult);
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

void set_basereliQ(struct InstantDEX_quote *iQ,cJSON *obj)
{
    char exchange[64]; int32_t exchangeid;
    iQ->s.baseamount = get_API_nxt64bits(cJSON_GetObjectItem(obj,"baseamount"));
    iQ->s.relamount = get_API_nxt64bits(cJSON_GetObjectItem(obj,"relamount"));
    iQ->s.quoteid = get_API_nxt64bits(cJSON_GetObjectItem(obj,"quoteid"));
    iQ->s.offerNXT = get_API_nxt64bits(cJSON_GetObjectItem(obj,"offerNXT"));
    copy_cJSON(exchange,cJSON_GetObjectItem(obj,"exchange"));
    find_exchange(&exchangeid,exchange);
    iQ->exchangeid = exchangeid;
}

// phasing! https://nxtforum.org/index.php?topic=6490.msg171048#msg171048
char *set_combohalf(struct pendingpair *pt,struct InstantDEX_quote *iQ,struct pending_offer *offer,uint64_t baseid,uint64_t relid,int32_t askoffer,int32_t dir)
{
    char *retstr;
    pt->baseamount = offer->ratio * iQ->s.baseamount;//get_API_nxt64bits(cJSON_GetObjectItem(obj,"baseamount"));
    pt->relamount = offer->ratio * iQ->s.relamount;//get_API_nxt64bits(cJSON_GetObjectItem(obj,"relamount"));
    pt->quoteid = iQ->s.quoteid;//get_API_nxt64bits(cJSON_GetObjectItem(obj,"quoteid"));
    pt->offerNXT = iQ->s.offerNXT;//get_API_nxt64bits(cJSON_GetObjectItem(obj,"offerNXT"));
    strcpy(pt->exchange,exchange_str(iQ->exchangeid));
    //copy_cJSON(pt->exchange,cJSON_GetObjectItem(obj,"exchange"));
    pt->nxt64bits = offer->nxt64bits, pt->baseid = baseid, pt->relid = relid, pt->ratio = offer->ratio;
    pt->price = prices777_price_volume(&pt->volume,pt->baseamount,pt->relamount);
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
        if ( submit_trade(&json,whostr,polarity*dir * pt->notxfer,&offer->halves[i],&offer->halves[i+1],offer,NXTACCTSECRET,offer->base,offer->rel,offer->price,offer->volume,offer->triggerheight) < 0 )
            return(pending_offer_error(offer,json));
        printf("second quarter:\n");
        sprintf(whostr,"%s%s",(polarity*dir2 > 0) ? "seller" : "buyer",(i == 0) ? "" : "2");
        if ( submit_trade(&json,whostr,polarity*dir2 * pt->notxfer,&offer->halves[i+1],&offer->halves[i],offer,NXTACCTSECRET,offer->base,offer->rel,offer->price,offer->volume,offer->triggerheight) < 0 )
            return(pending_offer_error(offer,json));
        polarity = -polarity;
        pt = &offer->B;
    }
    offer->endmilli = milliseconds() + 2. * JUMPTRADE_SECONDS * 1000;
    retstr = cJSON_Print(json), _stripwhite(retstr,' ');
    printf("endmilli %f vs %f: offer.%p (%s)\n",offer->endmilli,milliseconds(),offer,retstr);
    queue_enqueue("pending_offer",&Pending_offersQ.pingpong[0],&offer->DL);
    free_json(json);
    return(retstr);
}

void create_offer_comment(struct pending_offer *offer)
{
    sprintf(offer->comment,"{\"method\":\"makeoffer3\",\"askoffer\":\"%d\",\"NXT\":\"%llu\",\"ratio\":\"%.8f\",\"perc\":\"%d\",\"baseid\":\"%llu\",\"relid\":\"%llu\",\"baseamount\":\"%llu\",\"relamount\":\"%llu\",\"fee\":\"%llu\",\"quoteid\":\"%llu\",\"minperc\":\"%u\",\"jumpasset\":\"%llu\"}",offer->sell,(long long)offer->nxt64bits,offer->ratio,offer->perc,(long long)offer->baseid,(long long)offer->relid,(long long)offer->baseamount,(long long)offer->relamount,(long long)offer->fee,(long long)offer->quoteid,offer->minperc,(long long)offer->jumpasset);
}

char *tweak_offer(struct pending_offer *offer,int32_t dir,double refprice,double refvolume)
{
    double price,volume,bestvolume,bestprice; uint64_t baseqty,relqty,satoshis,refsatoshis,best=0,dist; int32_t i,j,besti,bestj,flag = 0;
    refsatoshis = (refprice * SATOSHIDEN);
    baseqty = offer->baseamount / offer->basemult;
    relqty = offer->relamount / offer->relmult;
    price = prices777_price_volume(&volume,baseqty * offer->basemult,relqty * offer->relmult);
    if ( fabs(price/refprice - 1.) > 0.01 || fabs(volume/refvolume - 1.) > 0.1 )
    {
        printf("refprice %.8f -> price %.8f %f, %f refvolume %.8f -> %.8f\n",refprice,price,fabs(price/refprice - 1.),fabs(volume/refvolume),refvolume,volume);
        return(clonestr("{\"error\":\"asset decimals dont allow this\"}"));
    }
    price = prices777_price_volume(&volume,offer->baseamount,offer->relamount);
    satoshis = (price * SATOSHIDEN);
    besti = bestj = 0;
    bestvolume = refvolume, bestprice = refprice;
    for (i=100; i>=-100&&i>-baseqty; i--)
    {
        for (j=100; j>=-100&&j>-relqty; j--)
        {
            price = prices777_price_volume(&volume,(baseqty + i) * offer->basemult,(relqty + j) * offer->relmult + j);
            satoshis = (price * SATOSHIDEN);
            if ( dir < 0 && satoshis > refsatoshis )
            {
                dist = (satoshis - refsatoshis);
                //printf("%llu ",(long long)dist);
                if ( best == 0 || dist < best )
                    bestprice = refprice, bestvolume = volume, best = dist, besti = i, bestj = j, flag = 1;
            }
            else if ( dir > 0 && satoshis < refsatoshis )
            {
                dist = (refsatoshis - satoshis);
                //printf("%llu ",(long long)dist);
                if ( best == 0 || dist < best )
                    bestprice = refprice, bestvolume = volume, best = dist, besti = i, bestj = j, flag = 1;
            }
            if ( flag != 0 && best == 0 )
                break;
        }
        if ( flag != 0 && best == 0 )
            break;
    }
    if ( flag != 0 )
    {
        printf("besti.%d bestj.%d ref (%f %f) -> (%f %f)\n",besti,bestj,refprice,refvolume,bestprice,bestvolume);
        offer->baseamount += besti * offer->basemult;
        offer->relamount += bestj * offer->relmult;
        offer->volume = bestvolume;
    }
    return(0);
}

char *makeoffer3(char *NXTaddr,char *NXTACCTSECRET,double price,double volume,int32_t deprecated,int32_t perc,uint64_t baseid,uint64_t relid,struct InstantDEX_quote *baseiQ,struct InstantDEX_quote *reliQ,uint64_t quoteid,int32_t askoffer,char *exchange,uint64_t baseamount,uint64_t relamount,uint64_t offerNXT,int32_t minperc,uint64_t jumpasset)
{
    struct NXT_tx T; char *retstr; int32_t dir; struct pendingpair *pt; struct pending_offer *offer = 0;
    if ( minperc == 0 )
        minperc = INSTANTDEX_MINVOL;
    if ( perc == 0 )
        perc = 100;
    else if ( perc < minperc )
        return(clonestr("{\"error\":\"perc < minperc\"}"));
    offer = calloc(1,sizeof(*offer));
    offer->minperc = minperc;
    offer->jumpasset = jumpasset;
    _set_assetname(&offer->basemult,offer->base,0,baseid);
    _set_assetname(&offer->relmult,offer->rel,0,relid);
    offer->nxt64bits = calc_nxt64bits(NXTaddr);
    if ( offer->nxt64bits == offerNXT )
        return(clonestr("{\"error\":\"cant match your own offer\"}"));
    if ( perc <= 0 || perc > 100 )
    {
        offer->ratio = 1.;
        offer->volume = volume;
        offer->baseamount = baseamount;
        offer->relamount = relamount;
    }
    else
    {
        offer->ratio = (double)perc / 100.;
        offer->volume = volume * offer->ratio;
        printf("perc.%d ratio %f volume %f -> %f\n",perc,offer->ratio,volume,offer->volume);
        volume = offer->volume;
        offer->baseamount = baseamount * offer->ratio;
        offer->relamount = relamount * offer->ratio;
    }
    strcpy(offer->exchange,exchange);
    offer->sell = askoffer;
    dir = (askoffer == 0) ? 1 : -1;
    if ( (retstr= tweak_offer(offer,dir,price,volume)) != 0 )
    {
        free(offer);
        return(retstr);
    }
    offer->baseid = baseid,  offer->relid = relid, offer->quoteid = quoteid, offer->price = price, offer->A.offerNXT = offerNXT, offer->perc = perc;
    pt = &offer->A;
    if ( baseiQ != 0 && reliQ != 0 )
    {
        if ( (retstr= set_combohalf(&offer->A,baseiQ,offer,baseid,jumpasset,askoffer,-dir)) != 0 )
            return(retstr);
        if ( (retstr= set_combohalf(&offer->B,reliQ,offer,relid,jumpasset,askoffer,-dir)) != 0 )
            return(retstr);
        offer->fee = INSTANTDEX_FEE;
        if ( offer->halves[0].T.exchangeid == INSTANTDEX_EXCHANGEID && offer->halves[2].T.exchangeid == INSTANTDEX_EXCHANGEID )
            offer->fee += INSTANTDEX_FEE;
    }
    else if ( strcmp(exchange,"nxtae") == 0 )
    {
        retstr = fill_nxtae(offer->nxt64bits,-dir,price,offer->volume,offer->baseid,offer->relid);
        free(offer);
        return(retstr);
    }
    else
    {
        strcpy(pt->exchange,offer->exchange);
        pt->nxt64bits = offer->nxt64bits, pt->baseid = offer->baseid, pt->baseamount = offer->baseamount, pt->relid = offer->relid, pt->relamount = offer->relamount, pt->quoteid = quoteid, pt->offerNXT = offerNXT, pt->perc = perc, pt->price = price, pt->volume = volume, pt->sell = offer->sell;
        offer->numhalves = 2;
        if ( (retstr= set_buyer_seller(&offer->halves[0],&offer->halves[1],&offer->A,offer,dir)) != 0 )
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
    offer->triggerheight = _get_NXTheight(0) + FINISH_HEIGHT;
    printf("GOT.(%s) triggerheight.%u\n",offer->comment,offer->triggerheight);
    set_NXTtx(offer->nxt64bits,&T,NXT_ASSETID,offer->fee,calc_nxt64bits(INSTANTDEX_ACCT),-1);
    strcpy(T.comment,offer->comment);
    if ( NXTACCTSECRET == 0 || NXTACCTSECRET[0] == 0 || (offer->feetx= sign_NXT_tx(offer->feeutxbytes,offer->feesignedtx,NXTACCTSECRET,offer->nxt64bits,&T,0,1.)) != 0 )
    {
        offer->expiration = get_txhashes(offer->feesighash,offer->triggerhash,(NXTACCTSECRET == 0 || NXTACCTSECRET[0] == 0) ? &T : offer->feetx);
        sprintf(offer->comment + strlen(offer->comment) - 1,",\"feetxid\":\"%llu\",\"triggerhash\":\"%s\",\"triggerheight\":\"%u\"}",(long long)(offer->feetx != 0 ? offer->feetx->txid : 0x1234),offer->triggerhash,offer->triggerheight);
        if ( strlen(offer->triggerhash) == 64 && (retstr= submit_trades(offer,NXTACCTSECRET)) != 0 )
            return(retstr);
    }
    return(pending_offer_error(offer,cJSON_Parse("{\"error\":\"couldnt submit fee tx\"}")));
}

// event driven respondtx
struct NXT_tx *is_valid_trigger(uint32_t *triggerheightp,uint64_t *quoteidp,cJSON *triggerjson,char *sender)
{
    char otherNXT[64]; cJSON *commentobj; struct NXT_tx *triggertx;
    if ( (triggertx= set_NXT_tx(triggerjson)) != 0 )
    {
        *triggerheightp = (uint32_t)get_API_int(cJSON_GetObjectItem(triggerjson,"phasingFinishHeight"),0);
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
    if ( (iQ= findquoteid(quoteid,0)) != 0 && iQ->s.matched == 0 )
    {
        if ( dir == 0 )
        {
            printf("need to validate iQ details\n"); // combo orderbook entries, polling, automatch
            polarity = (iQ->s.isask == 0) ? -1 : 1;
            dir = polarity;
        } else polarity = (iQ->s.isask != 0) ? -1 : 1;
        if ( Debuglevel > 1 )
            printf("found quoteid.%llu polarity.%d %llu/%llu vs %llu dir.%d\n",(long long)quoteid,polarity,(long long)iQ->s.baseid,(long long)iQ->s.relid,(long long)assetid,dir);
        //found quoteid.7555841528599494229 polarity.1 6932037131189568014/6854596569382794790 vs 6854596569382794790 dir.1
        if ( polarity*dir > 0 && ((polarity > 0 && iQ->s.baseid == assetid) || (polarity < 0 && iQ->s.relid == assetid)) )
        {
            if ( priceNQT != 0 )
                baseamount = calc_baseamount(&relamount,assetid,qty,priceNQT);
            else baseamount = qty * get_assetmult(assetid), relamount = otherqty * get_assetmult(otherassetid);
            if ( polarity > 0 )
                price = prices777_price_volume(&vol,baseamount,relamount), refprice = prices777_price_volume(&refvol,iQ->s.baseamount,iQ->s.relamount);
            else price = prices777_price_volume(&vol,relamount,baseamount), refprice = prices777_price_volume(&refvol,iQ->s.relamount,iQ->s.baseamount);
            if ( Debuglevel > 1 )
                printf("polarity.%d dir.%d (%f %f) vs ref.(%f %f)\n",polarity,dir,price,vol,refprice,refvol);
            if ( vol >= refvol*(double)iQ->s.minperc/100. && vol <= refvol )
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
    int32_t dir; uint32_t triggerheight;
    if ( strcmp(cmdstr,"transfer") == 0 )
        dir = 0;
    else dir = (strcmp(cmdstr,"sell") == 0) ? -1 : 1;
    sprintf(retbuf,"{\"error\":\"did not parse (%s)\"}",utx);
    printf("dir.%d respondtx.(%s) sig.%s full.%s from (%s)\n",dir,utx,sighash,triggerhash,sender);
    if ( (jsonstr= issue_calculateFullHash(utx,sighash)) != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            obj = cJSON_GetObjectItem(json,"fullHash");
            copy_cJSON(calchash,obj);
            if ( strcmp(calchash,triggerhash) == 0 )
            {
                if ( (parsed= issue_parseTransaction(utx)) != 0 )
                {
                    _stripwhite(parsed,' ');
                    printf("PARSED OFFER.(%s) triggerhash.(%s) (%s) offer sender.%s\n",parsed,triggerhash,calchash,sender);
                    if ( (triggerjson= cJSON_Parse(parsed)) != 0 )
                    {
                        if ( (triggertx= is_valid_trigger(&triggerheight,&checkquoteid,triggerjson,sender)) != 0 )
                        {
                            if ( checkquoteid == quoteid && (iQ= is_valid_offer(quoteid,dir,assetid,qty,priceNQT,calc_nxt64bits(sender),otherassetid,otherqty)) != 0 )
                            {
                                sprintf(retbuf,"{\"method\":\"respondtx\",\"NXT\":\"%llu\",\"qty\":\"%llu\",\"assetid\":\"%llu\",\"priceNQT\":\"%llu\",\"fee\":\"%llu\",\"quoteid\":\"%llu\",\"minperc\":\"%u\",\"otherassetid\":\"%llu\",\"otherqty\":\"%llu\"}",(long long)calc_nxt64bits(NXTaddr),(long long)qty,(long long)assetid,(long long)priceNQT,(long long)fee,(long long)quoteid,minperc,(long long)otherassetid,(long long)otherqty);
                                feetxid = send_feetx(NXT_ASSETID,fee,triggerhash,retbuf);
                                sprintf(retbuf+strlen(retbuf)-1,",\"feetxid\":\"%llu\"}",(long long)feetxid);
                                if ( (txid= submit_to_exchange(INSTANTDEX_NXTAEID,&submitstr,assetid,qty,priceNQT,dir,calc_nxt64bits(NXTaddr),NXTACCTSECRET,triggerhash,retbuf,calc_nxt64bits(sender),"base","rel",0.,0.,triggerheight)) == 0 )
                                    sprintf(retbuf,"{\"error\":[\"%s\"],\"submit_txid\":\"%llu\",\"quoteid\":\"%llu\"}",submitstr == 0 ? "submit error" : submitstr,(long long)txid,(long long)quoteid), free(submitstr);
                                else
                                {
                                    iQ->s.matched = 1;
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
char *call_makeoffer3(int32_t localaccess,char *NXTaddr,char *NXTACCTSECRET,cJSON *objs[])
{
    uint64_t quoteid,baseid,relid,baseamount,relamount,offerNXT,jumpasset; struct InstantDEX_quote baseiQ,reliQ;
    char exchange[MAX_JSON_FIELD];
    double price,volume;
    int32_t minperc,perc,flip = 0;
    baseid = get_API_nxt64bits(objs[0]);
    relid = get_API_nxt64bits(objs[1]);
    quoteid = get_API_nxt64bits(objs[2]);
    perc = get_API_int(objs[3],0);
    flip = (int32_t)get_API_int(objs[4],0);
    price = get_API_float(objs[8]);
    volume = get_API_float(objs[9]);
    copy_cJSON(exchange,objs[10]);
    baseamount = get_API_nxt64bits(objs[11]);
    relamount = get_API_nxt64bits(objs[12]);
    offerNXT = get_API_nxt64bits(objs[13]);
    minperc = (int32_t)get_API_int(objs[14],INSTANTDEX_MINVOL);
    jumpasset = get_API_nxt64bits(objs[15]);
    memset(&baseiQ,0,sizeof(baseiQ));
    memset(&reliQ,0,sizeof(reliQ));
    set_basereliQ(&baseiQ,objs[5]), set_basereliQ(&reliQ,objs[6]);
    return(makeoffer3(NXTaddr,NXTACCTSECRET,price,volume,flip,perc,baseid,relid,&baseiQ,&reliQ,quoteid,get_API_int(objs[7],0),exchange,baseamount,relamount,offerNXT,minperc,jumpasset));
}

char *makeoffer3_func(int32_t localaccess,int32_t valid,char *sender,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char *retstr = 0;
    //printf("makeoffer3 localaccess.%d\n",localaccess);
    if ( valid > 0 )
        retstr = call_makeoffer3(localaccess,SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,objs);
    return(retstr);
}

char *respondtx_func(int32_t localaccess,int32_t valid,char *sender,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char cmdstr[MAX_JSON_FIELD],triggerhash[MAX_JSON_FIELD],utx[MAX_JSON_FIELD],offerNXT[MAX_JSON_FIELD],sig[MAX_JSON_FIELD],*retstr = 0;
    uint64_t quoteid,assetid,qty,priceNQT,otherassetid,otherqty;
    int32_t minperc;
    printf("got respond_tx.(%s)\n",origargstr);
    if ( localaccess == 0 )
        return(0);
    copy_cJSON(cmdstr,objs[0]);
    assetid = get_API_nxt64bits(objs[1]);
    qty = get_API_nxt64bits(objs[2]);
    priceNQT = get_API_nxt64bits(objs[3]);
    copy_cJSON(triggerhash,objs[4]);
    quoteid = get_API_nxt64bits(objs[5]);
    copy_cJSON(sig,objs[6]);
    copy_cJSON(utx,objs[7]);
    minperc = (int32_t)get_API_int(objs[8],INSTANTDEX_MINVOL);
    //if ( localaccess != 0 )
    copy_cJSON(offerNXT,objs[9]);
    otherassetid = get_API_nxt64bits(objs[10]);
    otherqty = get_API_nxt64bits(objs[11]);
    if ( strcmp(offerNXT,SUPERNET.NXTADDR) == 0 && valid > 0 && triggerhash[0] != 0 )
        retstr = respondtx(SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,offerNXT,cmdstr,assetid,qty,priceNQT,triggerhash,quoteid,sig,utx,minperc,otherassetid,otherqty);
    //else retstr = clonestr("{\"result\":\"invalid respondtx_func request\"}");
    return(retstr);
}


#endif

//
//  subatomic.h
//  Created by jl777, April 17-22 2014
//  MIT License
//

// expire matching AM's, I also need to make sure AM offers expire pretty soon, so some forgotten trade doesnt just goes active
// fix overwriting sequenceid!

char *Signedtx; // hack for testing atomic_swap

#ifndef gateway_multiatomic_h
#define gateway_multiatomic_h

#define HALFDUPLEX
#define BITCOIN_WALLET_UNLOCKSECONDS 300
#define SUBATOMIC_PORTNUM 6777
#define SUBATOMIC_VARIANT (SUBATOMIC_PORTNUM - SERVER_PORT)
#define SUBATOMIC_SIG 0x84319574
#define SUBATOMIC_LOCKTIME (3600 * 2)
#define SUBATOMIC_DONATIONRATE .001
#define SUBATOMIC_DEFAULTINCR 100
#define SUBATOMIC_TOOLONG (300 * 1000000)
#define MAX_SUBATOMIC_OUTPUTS 4
#define MAX_SUBATOMIC_INPUTS 16
#define SUBATOMIC_STARTING_SEQUENCEID 1000
#define SUBATOMIC_CANTSEND_TOLERANCE 3
#define MAX_NXT_TXBYTES 2048

#define SUBATOMIC_TYPE 0
#define SUBATOMIC_FORNXT_TYPE 3
#define NXTFOR_SUBATOMIC_TYPE 2
#define ATOMICSWAP_TYPE 1

#define SUBATOMIC_TRADEFUNC 't'
#define SUBATOMIC_ATOMICSWAP 's'

#define SUBATOMIC_ABORTED -1
#define SUBATOMIC_HAVEREFUND 1
#define SUBATOMIC_WAITFOR_CONFIRMS 2
#define SUBATOMIC_COMPLETED 3

#define SUBATOMIC_SEND_PUBKEY 'P'
#define SUBATOMIC_REFUNDTX_NEEDSIG 'R'
#define SUBATOMIC_REFUNDTX_SIGNED 'S'
#define SUBATOMIC_FUNDINGTX 'F'
#define SUBATOMIC_SEND_MICROTX 'T'
#define SUBATOMIC_SEND_ATOMICTX 'A'

struct NXT_tx
{
    unsigned char refhash[32];
    uint64_t senderbits,recipientbits,assetidbits;
    int64_t feeNQT;
    union { int64_t amountNQT; int64_t quantityQNT; };
    int32_t deadline,type,subtype,verify;
    char comment[128];
};

struct subatomic_unspent_tx
{
    int64_t amount;    // MUST be first!
    uint32_t vout,confirmations;
    char txid[128],address[64],scriptPubKey[128],redeemScript[192];
};

struct atomic_swap
{
    char *parsed[2];
    cJSON *jsons[2];
    char NXTaddr[64],otherNXTaddr[64],otheripaddr[32];
    char txbytes[2][MAX_NXT_TXBYTES],signedtxbytes[2][MAX_NXT_TXBYTES];
    char sighash[2][68],fullhash[2][68];
    int32_t numfragis,atomictx_waiting;
    struct NXT_tx *mytx;
};

struct subatomic_rawtransaction
{
    char destaddrs[MAX_SUBATOMIC_OUTPUTS][64];
    int64_t amount,change,inputsum,destamounts[MAX_SUBATOMIC_OUTPUTS];
    int32_t numoutputs,numinputs,completed,broadcast,confirmed;
    char rawtransaction[1024],signedtransaction[1024],txid[128];
    struct subatomic_unspent_tx inputs[MAX_SUBATOMIC_INPUTS];   // must be last, could even make it variable sized
};

struct subatomic_halftx
{
    int32_t coinid,destcoinid,minconfirms;
    int64_t destamount,avail,amount,donation,myamount,otheramount;  // amount = (myamount + otheramount + donation + txfee)
    struct subatomic_rawtransaction funding,refund,micropay;
    char funding_scriptPubKey[512],countersignedrefund[1024],completedmicropay[1024];
    char *fundingtxid,*refundtxid,*micropaytxid;
    char NXTaddr[64],coinaddr[64],pubkey[128],ipaddr[32];
    char otherNXTaddr[64],destcoinaddr[64],destpubkey[128],otheripaddr[32];
    struct multisig_addr *xferaddr;
};

struct subatomic_tx_args
{
    char NXTaddr[64],otherNXTaddr[64],coinaddr[2][64],destcoinaddr[2][64],otheripaddr[32],mypubkeys[2][128];
    int64_t amount,destamount;
    double myshare;
    int32_t coinid,destcoinid,numincr,incr,otherincr;
};

struct subatomic_tx
{
    struct subatomic_tx_args ARGS,otherARGS;
    struct subatomic_halftx myhalf,otherhalf;
    struct atomic_swap swap;
    char *claimtxid;
    int64_t lastcontact,myexpectedamount,myreceived,otherexpectedamount,sent_to_other;//,recvflags;
    int32_t status,initflag,connsock,refundlockblock,cantsend,type,longerflag,tag,verified;
    int32_t txs_created,other_refundtx_done,myrefundtx_done,other_fundingtx_confirms;
    int32_t myrefund_fragi,microtx_fragi,funding_fragi,other_refund_fragi;
    int32_t other_refundtx_waiting,myrefundtx_waiting,other_fundingtx_waiting,other_micropaytx_waiting;
    unsigned char recvbufs[4][sizeof(struct subatomic_rawtransaction)];
};

struct subatomic_info
{
    char ipaddr[32],NXTADDR[32],NXTACCTSECRET[512];
    struct subatomic_tx **subatomics;
    CURL *curl_handle,*curl_handle2;
    int32_t numsubatomics,enable_bitcoin_broadcast;
} *Global_subatomic;

struct subatomic_packet
{
    struct server_request_header H;
    struct subatomic_tx_args ARGS;
    char pubkeys[2][128],retpubkeys[2][128];
    struct subatomic_rawtransaction rawtx;
#ifdef HALFDUPLEX
    // struct subatomic_rawtransaction needsig,havesig,micropay;   // jl777: hack for my broken Mac
#endif
};
int32_t subatomic_gen_pubkeys(struct subatomic_tx *atx,struct subatomic_halftx *htx);

void init_subatomic_halftx(struct subatomic_halftx *htx,struct subatomic_tx *atx)
{
    struct subatomic_info *gp = Global_subatomic;
    //htx->comms = &atx->comms;
    safecopy(htx->NXTaddr,atx->ARGS.NXTaddr,sizeof(htx->NXTaddr));
    safecopy(htx->otherNXTaddr,atx->ARGS.otherNXTaddr,sizeof(htx->otherNXTaddr));
    safecopy(htx->ipaddr,gp->ipaddr,sizeof(htx->ipaddr));
    if ( atx->ARGS.otheripaddr[0] != 0 )
        safecopy(htx->otheripaddr,atx->ARGS.otheripaddr,sizeof(htx->otheripaddr));
}

int32_t init_subatomic_tx(struct subatomic_tx *atx,int32_t flipped,int32_t type)
{
    struct daemon_info *cp,*destcp;
    if ( type == ATOMICSWAP_TYPE )
    {
        if ( atx->longerflag == 0 )
        {
            atx->longerflag = 1;
            if ( (calc_nxt64bits(atx->ARGS.NXTaddr) % 666) > (calc_nxt64bits(atx->ARGS.otherNXTaddr) % 666) )
                atx->longerflag = 2;
        }
        printf("ATOMICSWAP.(%s <-> %s) longerflag.%d\n",atx->swap.NXTaddr,atx->swap.otherNXTaddr,atx->longerflag);
        return(1 << flipped);
    }
    cp = get_daemon_info(atx->ARGS.coinid);
    destcp = get_daemon_info(atx->ARGS.destcoinid);
    if ( cp != 0 && destcp != 0 && atx->ARGS.coinaddr[flipped][0] != 0 && atx->ARGS.otherNXTaddr[0] != 0 && atx->ARGS.destcoinaddr[flipped][0] != 0 )
    {
        if ( atx->ARGS.amount != 0 && atx->ARGS.destamount != 0 && atx->ARGS.coinid != atx->ARGS.destcoinid )
        {
            if ( atx->longerflag == 0 )
            {
                atx->myhalf.minconfirms = cp->minconfirms;
                atx->otherhalf.minconfirms = destcp->minconfirms;
                atx->ARGS.numincr = SUBATOMIC_DEFAULTINCR;
                atx->longerflag = 1;
                if ( (calc_nxt64bits(atx->ARGS.NXTaddr) % 666) > (calc_nxt64bits(atx->ARGS.otherNXTaddr) % 666) )
                    atx->longerflag = 2;
            }
            init_subatomic_halftx(&atx->myhalf,atx);
            init_subatomic_halftx(&atx->otherhalf,atx);
            atx->connsock = -1;
            if ( flipped == 0 )
            {
                atx->myhalf.coinid = atx->ARGS.coinid; atx->myhalf.destcoinid = atx->ARGS.destcoinid;
                atx->myhalf.amount = atx->ARGS.amount; atx->myhalf.destamount = atx->ARGS.destamount;
                safecopy(atx->myhalf.coinaddr,atx->ARGS.coinaddr[0],sizeof(atx->myhalf.coinaddr));
                safecopy(atx->myhalf.destcoinaddr,atx->ARGS.destcoinaddr[0],sizeof(atx->myhalf.destcoinaddr));
                atx->myhalf.donation = atx->myhalf.amount * SUBATOMIC_DONATIONRATE;
                //if ( atx->myhalf.donation < cp->txfee )
                //    atx->myhalf.donation = cp->txfee;
                atx->otherexpectedamount = atx->myhalf.amount - 2*cp->txfee - 2*atx->myhalf.donation;
                subatomic_gen_pubkeys(atx,&atx->myhalf);
            }
            else
            {
                atx->otherhalf.coinid = atx->ARGS.destcoinid; atx->otherhalf.destcoinid = atx->ARGS.coinid;
                atx->otherhalf.amount = atx->ARGS.destamount; atx->otherhalf.destamount = atx->ARGS.amount;
                safecopy(atx->otherhalf.coinaddr,atx->ARGS.destcoinaddr[1],sizeof(atx->otherhalf.coinaddr));
                safecopy(atx->otherhalf.destcoinaddr,atx->ARGS.coinaddr[1],sizeof(atx->otherhalf.destcoinaddr));
                atx->otherhalf.donation = atx->otherhalf.amount * SUBATOMIC_DONATIONRATE;
                //if ( atx->otherhalf.donation < destcp->txfee )
                //    atx->otherhalf.donation = destcp->txfee;
                atx->myexpectedamount = atx->otherhalf.amount - 2*destcp->txfee - 2*atx->otherhalf.donation;
            }
            printf("%p.(%s %s %.8f -> %.8f %s <-> %s %s %.8f <- %.8f %s) myhalf.(%s %s) %.8f <-> %.8f other.(%s %s) IP.(%s)\n",atx,atx->ARGS.NXTaddr,coinid_str(atx->myhalf.coinid),dstr(atx->myhalf.amount),dstr(atx->myhalf.destamount),coinid_str(atx->myhalf.destcoinid),atx->ARGS.otherNXTaddr,coinid_str(atx->otherhalf.coinid),dstr(atx->otherhalf.amount),dstr(atx->otherhalf.destamount),coinid_str(atx->otherhalf.destcoinid),atx->myhalf.coinaddr,atx->myhalf.destcoinaddr,dstr(atx->myexpectedamount),dstr(atx->otherexpectedamount),atx->otherhalf.coinaddr,atx->otherhalf.destcoinaddr,atx->ARGS.otheripaddr);
            return(1 << flipped);
        }
    }
    return(0);
}

void calc_OP_HASH160(unsigned char hash160[20],char *msg)
{
    unsigned char sha256[32];
    hash_state md;
    
    sha256_init(&md);
    sha256_process(&md,(unsigned char *)msg,strlen(msg));
    sha256_done(&md,sha256);
    
    rmd160_init(&md);
    rmd160_process(&md,(unsigned char *)sha256,256/8);
    rmd160_done(&md,hash160);
    {
        int i;
        for (i=0; i<20; i++)
            printf("%02x",hash160[i]);
        printf("<- (%s)\n",msg);
    }
}

int32_t subatomic_gen_pubkeys(struct subatomic_tx *atx,struct subatomic_halftx *htx)
{
    char coinaddrs[3][64],pubkeys[3][128];
    struct daemon_info *cp;
    int32_t i,flag,coinid;
    char *pubkey,*coinaddr;
    if ( htx->xferaddr == 0 )
    {
        memset(coinaddrs,0,sizeof(coinaddrs));
        memset(pubkeys,0,sizeof(pubkeys));
        for (i=flag=0; i<2; i++)
        {
            if ( i == 0 )
            {
                coinid = atx->ARGS.coinid;
                pubkey = atx->myhalf.pubkey;
                coinaddr = atx->myhalf.coinaddr;
            }
            else
            {
                coinid = atx->ARGS.destcoinid;
                pubkey = atx->myhalf.destpubkey;
                coinaddr = atx->myhalf.destcoinaddr;
            }
            cp = get_daemon_info(coinid);
            if ( pubkey[0] == 0 )
            {
                if ( get_bitcoind_pubkey(pubkey,cp,coinaddr) != 0 )
                {
                    flag++;
                    safecopy(atx->ARGS.mypubkeys[i],pubkey,sizeof(atx->ARGS.mypubkeys[i]));
                    printf("gen pubkey %s (%s) for (%s)\n",coinid_str(coinid),pubkey,coinaddr);
                }
                else
                {
                    printf("cant generate %s pubkey for addr.%s\n",coinid_str(coinid),coinaddr);
                    return(-1);
                }
            }
        }
    }
    return(0);
}

int32_t subatomic_gen_multisig(struct subatomic_tx *atx,struct subatomic_halftx *htx)
{
    char coinaddrs[3][64],pubkeys[3][128];
    int32_t coinid;
    struct daemon_info *cp;
    coinid = atx->ARGS.coinid;
    cp = get_daemon_info(coinid);
    if ( cp == 0 )
        return(-1);
    if ( htx->coinaddr[0] != 0 && htx->pubkey[0] != 0 && atx->otherhalf.coinaddr[0] != 0 && atx->otherhalf.pubkey[0] != 0 )
    {
        safecopy(coinaddrs[0],htx->coinaddr,sizeof(coinaddrs[0]));
        safecopy(pubkeys[0],htx->pubkey,sizeof(pubkeys[0]));
        safecopy(coinaddrs[1],atx->otherhalf.coinaddr,sizeof(coinaddrs[1]));
        safecopy(pubkeys[1],atx->otherhalf.pubkey,sizeof(pubkeys[1]));
        htx->xferaddr = gen_multisig_addr(2,2,cp,htx->coinid,htx->NXTaddr,pubkeys,coinaddrs);
        //#ifdef DEBUG_MODE
        if ( htx->xferaddr != 0 )
            get_bitcoind_pubkey(pubkeys[2],cp,htx->xferaddr->multisigaddr);
        //#endif
        return(htx->xferaddr != 0);
    } else printf("cant gen multisig %d %d %d %d\n",htx->coinaddr[0],htx->pubkey[0],atx->otherhalf.coinaddr[0],atx->otherhalf.pubkey[0]);
    return(-1);
}

// bitcoind functions
cJSON *get_transaction_json(struct daemon_info *cp,char *txid)
{
    struct gateway_info *gp = Global_gp;
    char txidstr[512],*transaction = 0;
    cJSON *json = 0;
    int32_t coinid;
    if ( cp == 0 )
        return(0);
    coinid = cp->coinid;
    sprintf(txidstr,"\"%s\"",txid);
    transaction = bitcoind_RPC(cp->curl_handle,coinid_str(coinid),gp->serverport[coinid],gp->userpass[coinid],"gettransaction",txidstr);
    if ( transaction != 0 && transaction[0] != 0 )
    {
        //printf("got transaction.(%s)\n",transaction);
        json = cJSON_Parse(transaction);
    }
    if ( transaction != 0 )
        free(transaction);
    return(json);
}

int32_t subatomic_tx_confirmed(int32_t coinid,char *txid)
{
    cJSON *json;
    int32_t numconfirmed = -1;
    json = get_transaction_json(get_daemon_info(coinid),txid);
    if ( json != 0 )
    {
        numconfirmed = (int32_t)get_cJSON_int(json,"confirmations");
        free_json(json);
    }
    return(numconfirmed);
}

cJSON *get_decoderaw_json(struct daemon_info *cp,char *rawtransaction)
{
    struct gateway_info *gp = Global_gp;
    int32_t coinid = cp->coinid;
    char *str,*retstr;
    cJSON *json = 0;
    str = malloc(strlen(rawtransaction)+4);
    //printf("got rawtransaction.(%s)\n",rawtransaction);
    sprintf(str,"\"%s\"",rawtransaction);
    retstr = bitcoind_RPC(cp->curl_handle,coinid_str(coinid),gp->serverport[coinid],gp->userpass[coinid],"decoderawtransaction",str);
    if ( retstr != 0 && retstr[0] != 0 )
    {
        //printf("got decodetransaction.(%s)\n",retstr);
        json = cJSON_Parse(retstr);
    } else printf("error decoding.(%s)\n",str);
    if ( retstr != 0 )
        free(retstr);
    free(str);
    return(json);
}

char *subatomic_broadcasttx(struct subatomic_halftx *htx,char *bytes,int32_t myincr,int32_t lockedblock)
{
    FILE *fp;
    char fname[512],*retstr = 0;
    struct daemon_info *cp = get_daemon_info(htx->coinid);
    int32_t coinid;
    cJSON *txjson;
    if ( cp == 0 )
        return(0);
    coinid = cp->coinid;
    txjson = get_decoderaw_json(cp,bytes);
    if ( txjson != 0 )
    {
        retstr = cJSON_Print(txjson);
        printf("broadcasting is disabled for now: (%s) ->\n(%s)\n",bytes,retstr);
        sprintf(fname,"backups/%s_%lld_%s_%s_%lld.%03d_%d",coinid_str(coinid),(long long)htx->amount,htx->otherNXTaddr,coinid_str(htx->destcoinid),(long long)htx->destamount,myincr,lockedblock);
        if ( (fp=fopen(fname,"w")) != 0 )
        {
            fprintf(fp,"%s\n%s\n",bytes,retstr);
            fclose(fp);
        }
        free(retstr);
    }
    retstr = 0;
    if ( Global_subatomic->enable_bitcoin_broadcast == 666 )
    {
        retstr = bitcoind_RPC(cp->curl_handle,coinid_str(coinid),Global_gp->serverport[coinid],Global_gp->userpass[coinid],"sendrawtransaction",bytes);
        if ( retstr != 0 )
        {
            printf("sendrawtransaction returns.(%s)\n",retstr);
        }
    }
    return(retstr);
}

cJSON *get_rawtransaction_json(struct daemon_info *cp,char *txid)
{
    struct gateway_info *gp = Global_gp;
    char txidstr[512],*rawtransaction=0;
    cJSON *json = 0;
    int32_t coinid;
    if ( cp == 0 )
        return(0);
    coinid = cp->coinid;
    sprintf(txidstr,"\"%s\"",txid);
    rawtransaction = bitcoind_RPC(cp->curl_handle,coinid_str(coinid),gp->serverport[coinid],gp->userpass[coinid],"getrawtransaction",txidstr);
    if ( rawtransaction != 0 && rawtransaction[0] != 0 )
        json = get_decoderaw_json(cp,rawtransaction);
    else printf("error with getrawtransaction %s %s\n",coinid_str(coinid),txid);
    if ( rawtransaction != 0 )
        free(rawtransaction);
    return(json);
}

int32_t script_has_coinaddr(cJSON *scriptobj,char *coinaddr)
{
    int32_t i,n;
    char buf[512];
    cJSON *addresses,*addrobj;
    if ( scriptobj == 0 )
        return(0);
    addresses = cJSON_GetObjectItem(scriptobj,"addresses");
    if ( addresses != 0 )
    {
        n = cJSON_GetArraySize(addresses);
        for (i=0; i<n; i++)
        {
            addrobj = cJSON_GetArrayItem(addresses,i);
            copy_cJSON(buf,addrobj);
            if ( strcmp(buf,coinaddr) == 0 )
                return(1);
        }
    }
    return(0);
}

char *subatomic_decodetxid(int64_t *valuep,char *scriptPubKey,int32_t *locktimep,struct daemon_info *cp,char *rawtransaction,char *mycoinaddr)
{
    char txidbuf[512],checkasmstr[1024],asmstr[1024],*txid = 0;
    uint64_t value = 0;
    int32_t i,n,nval,reqSigs;
    cJSON *json,*txidjson,*scriptobj,*array,*item,*hexobj,*asmobj;
    *locktimep = -1;
    if ( (json= get_decoderaw_json(cp,rawtransaction)) != 0 )
    {
        *locktimep = (int32_t)get_cJSON_int(json,"locktime");
        txidjson = cJSON_GetObjectItem(json,"txid");
        copy_cJSON(txidbuf,txidjson);
        if ( txidbuf[0] != 0 )
            txid = clonestr(txidbuf);
        array = cJSON_GetObjectItem(json,"vout");
        if ( mycoinaddr != 0 && is_cJSON_Array(array) != 0 )
        {
            n = cJSON_GetArraySize(array);
            for (i=0; i<n; i++)
            {
                item = cJSON_GetArrayItem(array,i);
                hexobj = 0;
                scriptobj = cJSON_GetObjectItem(item,"scriptPubKey");
                if ( mycoinaddr != 0 && scriptobj != 0 && script_has_coinaddr(scriptobj,mycoinaddr) != 0 )
                {
                    nval = (int32_t)get_cJSON_int(item,"n");
                    if ( nval == i )
                    {
                        reqSigs = (int32_t)get_cJSON_int(item,"reqSigs");
                        value = conv_cJSON_float(item,"value");
                        hexobj = cJSON_GetObjectItem(scriptobj,"hex");
                        asmobj = cJSON_GetObjectItem(scriptobj,"asm");
                        if ( scriptPubKey != 0 && hexobj != 0 )
                            copy_cJSON(scriptPubKey,hexobj);
                        if ( reqSigs == 1 && hexobj != 0 && asmobj != 0 )
                        {
                            sprintf(checkasmstr,"OP_DUP OP_HASH160 %s OP_EQUALVERIFY OP_CHECKSIG","need to figure out how ot gen magic number");
                            copy_cJSON(asmstr,asmobj);
                            if ( 0 && strcmp(asmstr,checkasmstr) != 0 ) // maybe I am paranoid, but what if they fiddled the script!
                                printf("warning: (%s) != check.(%s)\n",asmstr,checkasmstr);
                        }
                    }
                }
            }
        }
    }
    if ( valuep != 0 )
        *valuep = value;
    return(txid);
}

cJSON *subatomic_vouts_json_params(struct subatomic_rawtransaction *rp)
{
    int32_t i;
    cJSON *json,*obj;
    json = cJSON_CreateObject();
    for (i=0; i<rp->numoutputs; i++)
    {
        obj = cJSON_CreateNumber((double)rp->destamounts[i]/SATOSHIDEN);
        cJSON_AddItemToObject(json,rp->destaddrs[i],obj);
    }
    // printf("numdests.%d (%s)\n",rp->numoutputs,cJSON_Print(json));
    return(json);
}

cJSON *subatomic_vins_json_params(int32_t coinid,struct subatomic_rawtransaction *rp)
{
    int32_t i;
    cJSON *json,*obj,*array;
    struct subatomic_unspent_tx *up;
    array = cJSON_CreateArray();
    for (i=0; i<rp->numinputs; i++)
    {
        up = &rp->inputs[i];
        json = cJSON_CreateObject();
        obj = cJSON_CreateString(up->txid); cJSON_AddItemToObject(json,"txid",obj);
        obj = cJSON_CreateNumber(up->vout); cJSON_AddItemToObject(json,"vout",obj);
        if ( up->scriptPubKey[0] != 0 )
        {
            obj = cJSON_CreateString(up->scriptPubKey);
            cJSON_AddItemToObject(json,"scriptPubKey",obj);
        }
        if ( up->redeemScript[0] != 0 )
        {
            obj = cJSON_CreateString(up->redeemScript);
            cJSON_AddItemToObject(json,"redeemScript",obj);
        }
        cJSON_AddItemToArray(array,json);
    }
    return(array);
}

char *subatomic_rawtxid_json(int32_t coinid,struct subatomic_rawtransaction *rp)
{
    char *paramstr = 0;
    cJSON *array,*vinsobj,*voutsobj;
    vinsobj = subatomic_vins_json_params(coinid,rp);
    if ( vinsobj != 0 )
    {
        voutsobj = subatomic_vouts_json_params(rp);
        if ( voutsobj != 0 )
        {
            array = cJSON_CreateArray();
            cJSON_AddItemToArray(array,vinsobj);
            cJSON_AddItemToArray(array,voutsobj);
            paramstr = cJSON_Print(array);
            free_json(array);   // this frees both vinsobj and voutsobj
        }
        else free_json(vinsobj);
    }
    // printf("subatomic_rawtxid_json.%s\n",paramstr);
    return(paramstr);
}

cJSON *subatomic_privkeys_json_params(int32_t coinid,char **coinaddrs,int32_t n)
{
    struct daemon_info *cp = get_daemon_info(coinid);
    char walletkey[64];
    if ( cp != 0 )
    {
        sprintf(walletkey,"[\"%s\",%d]",Global_subatomic->NXTADDR,BITCOIN_WALLET_UNLOCKSECONDS);
        // locking first avoids error, hacky but no time for wallet fiddling now
        bitcoind_RPC(cp->curl_handle,coinid_str(coinid),Global_gp->serverport[coinid],Global_gp->userpass[coinid],"walletlock",0);
        bitcoind_RPC(cp->curl_handle,coinid_str(coinid),Global_gp->serverport[coinid],Global_gp->userpass[coinid],"walletpassphrase",walletkey);
        return(create_privkeys_json_params(coinid,coinaddrs,n));
    } else return(0);
}

char *subatomic_signraw_json_params(char *skipaddr,char *coinaddr,int32_t coinid,struct subatomic_rawtransaction *rp,char *rawbytes)
{
    int32_t i,j,flag;
    char *coinaddrs[MAX_SUBATOMIC_INPUTS+1],*paramstr = 0;
    cJSON *array,*rawobj,*vinsobj,*keysobj;
    rawobj = cJSON_CreateString(rawbytes);
    if ( rawobj != 0 )
    {
        vinsobj = subatomic_vins_json_params(coinid,rp);
        if ( vinsobj != 0 )
        {
            // printf("add %d inputs skipaddr.%s coinaddr.%s\n",rp->numinputs,skipaddr,coinaddr);
            for (i=flag=j=0; i<rp->numinputs; i++)
            {
                //printf("i.%d j.%d flag.%d %s\n",i,j,flag,rp->inputs[i].address);
                if ( skipaddr != 0 && strcmp(rp->inputs[i].address,skipaddr) != 0 )
                {
                    coinaddrs[j] = rp->inputs[i].address;
                    if ( coinaddr != 0 && strcmp(coinaddrs[j],coinaddr) == 0 )
                        flag++;
                    j++;
                }
            }
            //printf("i.%d j.%d flag.%d\n",i,j,flag);
            if ( coinaddr != 0 && flag == 0 )
                coinaddrs[j++] = coinaddr;
            coinaddrs[j++] = 0;
            keysobj = subatomic_privkeys_json_params(coinid,coinaddrs,j);
            //printf("subatomic_privkeys_json_params\n");
            if ( keysobj != 0 )
            {
                array = cJSON_CreateArray();
                cJSON_AddItemToArray(array,rawobj);
                cJSON_AddItemToArray(array,vinsobj);
                cJSON_AddItemToArray(array,keysobj);
                paramstr = cJSON_Print(array);
                free_json(array);
            }
            else free_json(vinsobj);
        }
        else free_json(rawobj);
    }
    return(paramstr);
}

char *subatomic_signtx(char *skipaddr,int32_t *lockedblockp,int64_t *valuep,char *coinaddr,char *deststr,unsigned long destsize,struct daemon_info *cp,int32_t coinid,struct subatomic_rawtransaction *rp,char *rawbytes)
{
    struct gateway_info *gp = Global_gp;
    cJSON *json,*hexobj,*compobj;
    char *retstr,*signparams,*txid = 0;
    int32_t locktime = 0;
    rp->txid[0] = 0;
    deststr[0] = 0;
    rp->completed = -1;
    //printf("cp.%d vs %d: subatomic_signtx rawbytes.(%s)\n",cp->coinid,coinid,rawbytes);
    signparams = subatomic_signraw_json_params(skipaddr,coinaddr,coinid,rp,rawbytes);
    if ( signparams != 0 )
    {
        stripwhite(signparams,strlen(signparams));
        // printf("got signparams.(%s)\n",signparams);
        retstr = bitcoind_RPC(cp->curl_handle,coinid_str(coinid),gp->serverport[coinid],gp->userpass[coinid],"signrawtransaction",signparams);
        if ( retstr != 0 )
        {
            //printf("got retstr.(%s)\n",retstr);
            json = cJSON_Parse(retstr);
            if ( json != 0 )
            {
                hexobj = cJSON_GetObjectItem(json,"hex");
                compobj = cJSON_GetObjectItem(json,"complete");
                if ( compobj != 0 )
                    rp->completed = ((compobj->type&0xff) == cJSON_True);
                copy_cJSON(deststr,hexobj);
                if ( strlen(deststr) > destsize )
                    printf("sign_rawtransaction: strlen(deststr) %ld > %ld destize\n",strlen(deststr),destsize);
                else
                {
                    txid = subatomic_decodetxid(valuep,0,&locktime,cp,deststr,coinaddr);
                    if ( txid != 0 )
                    {
                        safecopy(rp->txid,txid,sizeof(rp->txid));
                        free(txid);
                        txid = rp->txid;
                    }
                    // printf("got signedtransaction -> txid.(%s) %.8f\n",rp->txid,dstr(valuep!=0?*valuep:0));
                }
                free_json(json);
            } else printf("json parse error.(%s)\n",retstr);
            free(retstr);
        } else printf("error signing rawtx\n");
        free(signparams);
    } else printf("error generating signparams\n");
    if ( lockedblockp != 0 )
        *lockedblockp = locktime;
    return(txid);
}

void subatomic_uint32_splicer(char *txbytes,int32_t offset,uint32_t spliceval)
{
    int32_t i;
    uint32_t x;
    if ( offset < 0 )
    {
        static int foo;
        if ( foo++ < 3 )
            printf("subatomic_uint32_splicer illegal offset.%d\n",offset);
        return;
    }
    for (i=0; i<4; i++)
    {
        x = spliceval & 0xff; spliceval >>= 8;
        txbytes[offset + i*2] = hexbyte((x>>4) & 0xf);
        txbytes[offset + i*2 + 1] = hexbyte(x & 0xf);
    }
}

int32_t pubkey_to_256bits(unsigned char *bytes,char *pubkey)
{
    //Bitcoin private keys are 32 bytes, but are often stored in their full OpenSSL-serialized form of 279 bytes. They are serialized as 51 base58 characters, or 64 hex characters.
    //Bitcoin public keys (traditionally) are 65 bytes (the first of which is 0x04). They are typically encoded as 130 hex characters.
    //Bitcoin compressed public keys (as of 0.6.0) are 33 bytes (the first of which is 0x02 or 0x03). They are typically encoded as 66 hex characters.
    //Bitcoin addresses are RIPEMD160(SHA256(pubkey)), 20 bytes. They are typically encoded as 34 base58 characters.
    char zpadded[65];
    int32_t i,j,n;
    if ( (n= (int32_t)strlen(pubkey)) > 66 )
    {
        printf("pubkey_to_256bits pubkey.(%s) len.%ld > 66\n",pubkey,strlen(pubkey));
        return(-1);
    }
    if ( pubkey[0] != '0' || (pubkey[1] != '2' && pubkey[1] != '3') )
    {
        printf("pubkey_to_256bits pubkey.(%s) len.%ld unexpected first byte\n",pubkey,strlen(pubkey));
        return(-1);
    }
    for (i=0; i<64; i++)
        zpadded[i] = '0';
    zpadded[64] = 0;
    for (i=66-n,j=2; i<64; i++,j++)
        zpadded[i] = pubkey[j];
    if ( pubkey[j] != 0 )
    {
        printf("pubkey_to_256bits unexpected nonzero at j.%d\n",j);
        return(-1);
    }
    printf("pubkey.(%s) -> zpadded.(%s)\n",pubkey,zpadded);
    decode_hex(bytes,32,zpadded);
    //for (i=0; i<32; i++)
    //    printf("%02x",bytes[i]);
    //printf("\n");
    return(0);
}

struct btcinput_data
{
    unsigned char txid[32];
    uint32_t vout;
    int64_t scriptlen;
    uint32_t sequenceid;
    unsigned char script[];
};

struct btcoutput_data
{
    int64_t value,pk_scriptlen;
    unsigned char pk_script[];
};

struct btcinput_data *decode_btcinput(int32_t *offsetp,char *txbytes)
{
    struct btcinput_data *ptr,I;
    int32_t i;
    memset(&I,0,sizeof(I));
    for (i=0; i<32; i++)
        I.txid[31-i] = _decode_hex(txbytes + *offsetp), (*offsetp) += 2;
    I.vout = _decode_hexint(offsetp,txbytes);
    I.scriptlen = _decode_varint(offsetp,txbytes);
    if ( I.scriptlen > 1024 )
    {
        printf("decode_btcinput: very long script starting at offset.%d of (%s)\n",*offsetp,txbytes);
        return(0);
    }
    ptr = calloc(1,sizeof(*ptr) + I.scriptlen);
    *ptr = I;
    for (i=0; i<I.scriptlen; i++)
        ptr->script[i] = _decode_hex(txbytes + *offsetp), (*offsetp) += 2;
    ptr->sequenceid = _decode_hexint(offsetp,txbytes);
    return(ptr);
}

struct btcoutput_data *decode_btcoutput(int32_t *offsetp,char *txbytes)
{
    int32_t i;
    struct btcoutput_data *ptr,btcO;
    memset(&btcO,0,sizeof(btcO));
    btcO.value = _decode_hexlong(offsetp,txbytes);
    btcO.pk_scriptlen = _decode_varint(offsetp,txbytes);
    if ( btcO.pk_scriptlen > 1024 )
    {
        printf("decode_btcoutput: very long script starting at offset.%d of (%s)\n",*offsetp,txbytes);
        return(0);
    }
    ptr = calloc(1,sizeof(*ptr) + btcO.pk_scriptlen);
    *ptr = btcO;
    for (i=0; i<btcO.pk_scriptlen; i++)
        ptr->pk_script[i] = _decode_hex(txbytes + *offsetp), (*offsetp) += 2;
    return(ptr);
}

uint32_t calc_vin0seqstart(char *txbytes)
{
    struct btcinput_data *btcinput,*firstinput = 0;
    struct btcoutput_data *btcoutput;
    char buf[4096];
    int32_t i,vin0seqstart,numoutputs,numinputs,offset = 0;
    //version = _decode_hexint(&offset,txbytes);
    numinputs = _decode_hex(&txbytes[offset]), offset += 2;
    vin0seqstart = 0;
    for (i=0; i<numinputs; i++)
    {
        btcinput = decode_btcinput(&offset,txbytes);
        if ( btcinput != 0 )
        {
            init_hexbytes(buf,btcinput->txid,32);
            // printf("(%s vout%d) ",buf,btcinput->vout);
        }
        if ( i == 0 )
        {
            firstinput = btcinput;
            vin0seqstart = offset - sizeof(int32_t)*2;
        }
        else if ( btcinput != 0 ) free(btcinput);
    }
    //printf("-> ");
    numoutputs = _decode_hex(&txbytes[offset]), offset += 2;
    for (i=0; i<numoutputs; i++)
    {
        btcoutput = decode_btcoutput(&offset,txbytes);
        if ( btcoutput != 0 )
        {
            init_hexbytes(buf,btcoutput->pk_script,btcoutput->pk_scriptlen);
            // printf("(%s %.8f) ",buf,dstr(btcoutput->value));
            free(btcoutput);
        }
    }
    //locktime =
    _decode_hexint(&offset,txbytes);
    // printf("version.%d 1st.seqid %d @ %d numinputs.%d numoutputs.%d locktime.%d\n",version,firstinput!=0?firstinput->sequenceid:0xffffffff,vin0seqstart,numinputs,numoutputs,locktime);
    //0100000001ac8511d408d35e62ccc7925ed2437022e9b7e9e731197a42a58495e4465439d10000000000ffffffff0200dc5c24020000001976a914bf685a09e61215c7e824d0b73bc6d6d3ba9d9d9688ac00c2eb0b000000001976a91414b24a5b6f8c8df0f7c9b519d362618ca211e60988ac00000000
    if ( firstinput != 0 )
        return(vin0seqstart);
    else return(-1);
}

// if signcoindaddr is non-zero then signtx and return txid, otherwise just return rawtransaction bytes (NOT allocated)
char *subatomic_gen_rawtransaction(char *skipaddr,struct daemon_info *cp,struct subatomic_rawtransaction *rp,char *signcoinaddr,uint32_t locktime,uint32_t vin0sequenceid)
{
    struct gateway_info *gp = Global_gp;
    int32_t coinid = cp->coinid;
    char *rawparams,*retstr=0;
    int64_t value;
    long len;
    rawparams = subatomic_rawtxid_json(coinid,rp);
    if ( rawparams != 0 )
    {
        stripwhite(rawparams,strlen(rawparams));
        retstr = bitcoind_RPC(cp->curl_handle,coinid_str(coinid),gp->serverport[coinid],gp->userpass[coinid],"createrawtransaction",rawparams);
        if ( retstr != 0 )
        {
            if ( retstr[0] != 0 )
            {
                // printf("calc_rawtransaction retstr.(%s)\n",retstr);
                safecopy(rp->rawtransaction,retstr,sizeof(rp->rawtransaction));
                free(retstr);
                len = strlen(rp->rawtransaction);
                if ( len < 8 )
                {
                    printf("funny rawtransactionlen %ld??\n",len);
                    free(rawparams);
                    return(0);
                }
                if ( locktime != 0 )
                {
                    subatomic_uint32_splicer(rp->rawtransaction,(int32_t)(len - sizeof(uint32_t)*2),locktime);
                    if ( 0 && vin0sequenceid != 0xffffffff )
                        subatomic_uint32_splicer(rp->rawtransaction,calc_vin0seqstart(rp->rawtransaction),vin0sequenceid);
                    printf("locktime.%d sequenceid.%d\n",locktime,vin0sequenceid);
                }
                if ( signcoinaddr != 0 )
                    retstr = subatomic_signtx(skipaddr,0,&value,signcoinaddr,rp->signedtransaction,sizeof(rp->signedtransaction),cp,coinid,rp,rp->rawtransaction);
                else retstr = rp->rawtransaction;
            } else { free(retstr); retstr = 0; };
        } else printf("error creating rawtransaction from.(%s)\n",rawparams);
        free(rawparams);
    } else printf("error creating rawparams\n");
    return(retstr);
}

struct subatomic_unspent_tx *gather_unspents(int32_t *nump,struct daemon_info *cp,char *coinaddr)
{
    struct gateway_info *gp = Global_gp;
    int32_t i,j,num,coinid = cp->coinid;
    struct subatomic_unspent_tx *ups = 0;
    char *retstr;
    cJSON *json,*item;
    //sprintf(str,"[\"minconf:%d\"]",cp->minconfirms);
    retstr = bitcoind_RPC(cp->curl_handle,coinid_str(coinid),gp->serverport[coinid],gp->userpass[coinid],"listunspent",0);
    /*{
     "txid" : "1ccd2a9d0f8d690ed13b6768fc6c041972362f5531922b6b152ed2c98d3fe113",
     "vout" : 1,
     "address" : "DK3nxu6GshBcQNDMqc66ARcwqDZ1B5TJe5",
     "scriptPubKey" : "76a9149891029995222077889b36c77e2b85690878df9088ac",
     "amount" : 2.00000000,
     "confirmations" : 72505
     },*/
    if ( retstr != 0 )
    {
        printf("unspents (%s)\n",retstr);
        if ( (json = cJSON_Parse(retstr)) != 0 )
        {
            if ( is_cJSON_Array(json) != 0 && (num= cJSON_GetArraySize(json)) > 0 )
            {
                ups = calloc(num,sizeof(struct subatomic_unspent_tx));
                for (i=j=0; i<num; i++)
                {
                    item = cJSON_GetArrayItem(json,i);
                    copy_cJSON(ups[j].address,cJSON_GetObjectItem(item,"address"));
                    if ( coinaddr == 0 || strcmp(coinaddr,ups[j].address) == 0 )
                    {
                        copy_cJSON(ups[j].txid,cJSON_GetObjectItem(item,"txid"));
                        copy_cJSON(ups[j].scriptPubKey,cJSON_GetObjectItem(item,"scriptPubKey"));
                        ups[j].vout = (int32_t)get_cJSON_int(item,"vout");
                        ups[j].amount = conv_cJSON_float(item,"amount");
                        ups[j].confirmations = (int32_t)get_cJSON_int(item,"confirmations");
                        j++;
                    }
                }
                *nump = j;
                if ( j > 1 )
                    qsort(ups,j,sizeof(*ups),_decreasing_signedint64);
            }
            free_json(json);
        }
        free(retstr);
    }
    return(ups);
}

int64_t subatomic_calc_rawinputs(struct daemon_info *cp,struct subatomic_rawtransaction *rp,uint64_t amount,struct subatomic_unspent_tx *ups,int32_t num,uint64_t donation)
{
    uint64_t sum = 0;
    struct subatomic_unspent_tx *up;
    int32_t i;
    rp->inputsum = rp->numinputs = 0;
    printf("unspent num %d, amount %.8f vs donation %.8f txfee %.8f\n",num,dstr(amount),dstr(donation),dstr(cp->txfee));
    if ( (donation + cp->txfee) > amount )
        return(0);
    for (i=0; i<num&&i<((int32_t)(sizeof(rp->inputs)/sizeof(*rp->inputs))); i++)
    {
        up = &ups[i];
        sum += up->amount;
        rp->inputs[rp->numinputs++] = *up;
        if ( sum >= amount )
        {
            rp->amount = (amount - cp->txfee);
            rp->change = (sum - amount);
            rp->inputsum = sum;
            printf("numinputs %d sum %.8f vs amount %.8f change %.8f -> txfee %.8f\n",rp->numinputs,dstr(rp->inputsum),dstr(amount),dstr(rp->change),dstr(sum - rp->change - rp->amount));
            return(rp->inputsum);
        }
    }
    printf("i.%d error numinputs %d sum %.8f\n",i,rp->numinputs,dstr(rp->inputsum));
    return(0);
}

int32_t subatomic_calc_rawoutputs(struct subatomic_halftx *htx,struct daemon_info *cp,struct subatomic_rawtransaction *rp,double myshare,char *myaddr,char *otheraddr,char *changeaddr)
{
    struct gateway_info *gp = Global_gp;
    int32_t coinid,n = 0;
    int64_t donation;
    coinid = htx->coinid;
    if ( cp == 0 || cp->coinid != coinid )
    {
        printf("subatomic_calc_rawoutputs: bad cp.%p or coinid.%d\n",cp,coinid);
        return(-1);
    }
    //printf("rp->amount %.8f, (%.8f - %.8f), change %.8f (%.8f - %.8f) donation %.8f\n",dstr(rp->amount),dstr(htx->avail),dstr(cp->txfee),dstr(rp->change),dstr(rp->inputsum),dstr(htx->avail),dstr(htx->donation));
    if ( rp->amount == (htx->avail - cp->txfee) && rp->change == (rp->inputsum - htx->avail) )
    {
        if ( changeaddr == 0 )
            changeaddr = gp->internalmarker[cp->coinid];
        if ( htx->donation != 0 && gp->internalmarker[coinid] != 0 )
            donation = htx->donation;
        else donation = 0;
        htx->myamount = (rp->amount - donation) * myshare;
        if ( htx->myamount > rp->amount )
            htx->myamount = rp->amount;
        htx->otheramount = (rp->amount - htx->myamount - donation);
        if ( htx->myamount > 0 )
        {
            safecopy(rp->destaddrs[n],myaddr,sizeof(rp->destaddrs[n]));
            rp->destamounts[n] = htx->myamount;
            n++;
        }
        if ( otheraddr == 0 )
        {
            printf("no otheraddr, boost donation by %.8f\n",dstr(htx->otheramount));
            donation += htx->otheramount;
            htx->otheramount = 0;
        }
        if ( htx->otheramount > 0 )
        {
            safecopy(rp->destaddrs[n],otheraddr,sizeof(rp->destaddrs[n]));
            rp->destamounts[n] = htx->otheramount;
            n++;
        }
        if ( changeaddr == 0 )
        {
            printf("no changeaddr, boost donation %.8f\n",dstr(rp->change));
            donation += rp->change;
            rp->change = 0;
        }
        if ( rp->change > 0 )
        {
            safecopy(rp->destaddrs[n],changeaddr,sizeof(rp->destaddrs[n]));
            rp->destamounts[n] = rp->change;
            n++;
        }
        if ( donation > 0 && gp->internalmarker[coinid] != 0 )
        {
            safecopy(rp->destaddrs[n],gp->internalmarker[coinid],sizeof(rp->destaddrs[n]));
            rp->destamounts[n] = donation;
            n++;
        }
        //printf("myshare %.6f %.8f -> %s, other %.8f -> %s, change %.8f -> %s, donation %.8f -> %s | ",myshare,dstr(htx->myamount),myaddr,dstr(htx->otheramount),otheraddr,dstr(rp->change),changeaddr,dstr(donation),gp->internalmarker[coinid]);
    }
    rp->numoutputs = n;
    //printf("numoutputs.%d\n",n);
    return(n);
}


/* https://en.bitcoin.it/wiki/Contracts#Example_7:_Rapidly-adjusted_.28micro.29payments_to_a_pre-determined_party
 1) Create a public key (K1). Request a public key from the server (K2).
 2) Create and sign but do not broadcast a transaction (T1) that sets up a payment of (for example) 10 BTC to an output requiring both the server's public key and one of your own to be used. A good way to do this is use OP_CHECKMULTISIG. The value to be used is chosen as an efficiency tradeoff.
 3) Create a refund transaction (T2) that is connected to the output of T1 which sends all the money back to yourself. It has a time lock set for some time in the future, for instance a few hours. Don't sign it, and provide the unsigned transaction to the server. By convention, the output script is "2 K1 K2 2 CHECKMULTISIG"
 4) The server signs T2 using its public key K2 and returns the signature to the client. Note that it has not seen T1 at this point, just the hash (which is in the unsigned T2).
 5) The client verifies the servers signature is correct and aborts if not.
 6) The client signs T1 and passes the signature to the server, which now broadcasts the transaction (either party can do this if they both have connectivity). This locks in the money.
 
 7) The client then creates a new transaction, T3, which connects to T1 like the refund transaction does and has two outputs. One goes to K1 and the other goes to K2. It starts out with all value allocated to the first output (K1), ie, it does the same thing as the refund transaction but is not time locked. The client signs T3 and provides the transaction and signature to the server.
 8) The server verifies the output to itself is of the expected size and verifies the client's provided signature is correct.
 9) When the client wishes to pay the server, it adjusts its copy of T3 to allocate more value to the server's output and less to its ow. It then re-signs the new T3 and sends the signature to the server. It does not have to send the whole transaction, just the signature and the amount to increment by is sufficient. The server adjusts its copy of T3 to match the new amounts, verifies the signature and continues.
 
 10) This continues until the session ends, or the 1-day period is getting close to expiry. The AP then signs and broadcasts the last transaction it saw, allocating the final amount to itself. The refund transaction is needed to handle the case where the server disappears or halts at any point, leaving the allocated value in limbo. If this happens then once the time lock has expired the client can broadcast the refund transaction and get back all the money.
 */

// subatomic logic functions
char *subatomic_create_fundingtx(struct subatomic_halftx *htx,int64_t amount)
{
    //2) Create and sign but do not broadcast a transaction (T1) that sets up a payment of amount to funding acct
    struct daemon_info *cp = get_daemon_info(htx->coinid);
    struct subatomic_unspent_tx *ups;
    char *txid,*retstr = 0;
    int32_t num,check_locktime,locktime = 0;
    if ( cp == 0 )
        return(0);
    printf("CREATE FUNDING TX\n");
    memset(&htx->funding,0,sizeof(htx->funding));
    ups = gather_unspents(&num,cp,0);//htx->coinaddr);
    if ( ups != 0 && num != 0 )
    {
        if ( subatomic_calc_rawinputs(cp,&htx->funding,amount,ups,num,htx->donation) >= amount )
        {
            htx->avail = amount;
            if ( subatomic_calc_rawoutputs(htx,cp,&htx->funding,1.,htx->xferaddr->multisigaddr,0,htx->coinaddr) > 0 )
            {
                retstr = subatomic_gen_rawtransaction(htx->xferaddr->multisigaddr,cp,&htx->funding,htx->coinaddr,locktime,0xffffffff);
                if ( retstr != 0 )
                {
                    txid = subatomic_decodetxid(0,htx->funding_scriptPubKey,&check_locktime,cp,htx->funding.rawtransaction,htx->xferaddr->multisigaddr);
                    printf("txid.%s fundingtx %.8f -> %.8f %s completed.%d locktimes %d vs %d\n",txid,dstr(amount),dstr(htx->funding.amount),retstr,htx->funding.completed,check_locktime,locktime);
                    printf("funding.(%s)\n",htx->funding.signedtransaction);
                }
            }
        }
    }
    if ( ups != 0 )
        free(ups);
    return(retstr);
}

void subatomic_set_unspent_tx0(struct subatomic_unspent_tx *up,struct subatomic_halftx *htx)
{
    memset(up,0,sizeof(*up));
    up->vout = 0;
    up->amount = htx->avail;
    safecopy(up->txid,htx->fundingtxid,sizeof(up->txid));
    safecopy(up->address,htx->xferaddr->multisigaddr,sizeof(up->address));
    safecopy(up->scriptPubKey,htx->funding_scriptPubKey,sizeof(up->scriptPubKey));
    safecopy(up->redeemScript,htx->xferaddr->redeemScript,sizeof(up->redeemScript));
}

char *subatomic_create_paytx(struct subatomic_rawtransaction *rp,char *signcoinaddr,struct subatomic_halftx *htx,char *othercoinaddr,int32_t locktime,double myshare,int32_t seqid)
{
    struct daemon_info *cp = get_daemon_info(htx->coinid);
    struct subatomic_unspent_tx U;
    int32_t check_locktime;
    int64_t value;
    char *txid = 0;
    if ( cp == 0 )
        return(0);
    printf("create paytx %s\n",coinid_str(cp->coinid));
    subatomic_set_unspent_tx0(&U,htx);
    rp->numinputs = 0;
    rp->inputs[rp->numinputs++] = U;
    rp->amount = (htx->avail - cp->txfee);
    rp->change = 0;
    rp->inputsum = htx->avail;
    // jl777: make sure sequence number is not -1!!
    if ( subatomic_calc_rawoutputs(htx,cp,rp,myshare,htx->coinaddr,othercoinaddr,0) > 0 )
    {
        subatomic_gen_rawtransaction(htx->xferaddr->multisigaddr,cp,rp,signcoinaddr,locktime,seqid);
        txid = subatomic_decodetxid(&value,0,&check_locktime,cp,rp->rawtransaction,htx->coinaddr);
        if ( check_locktime != locktime )
        {
            printf("check_locktime.%d vs locktime.%d\n",check_locktime,locktime);
            return(0);
        }
        printf("created paytx %.8f to %s value %.8f, locktime.%d\n",dstr(value),htx->coinaddr,dstr(value),locktime);
    }
    return(txid);
}

int32_t subatomic_ensure_txs(struct subatomic_tx *atx,struct subatomic_halftx *htx,int32_t locktime)
{
    struct daemon_info *cp = get_daemon_info(htx->coinid);
    int32_t blocknum = 0;
    if ( cp == 0 || cp->coinid != htx->coinid || htx->xferaddr == 0 )
    {
        printf("cant get valid daemon for %s or no xferaddr.%p\n",coinid_str(htx->coinid),htx->xferaddr);
        return(-1);
    }
    if ( locktime != 0 )
    {
        blocknum = (int32_t)get_blockheight(cp,htx->coinid);
        if ( blocknum == 0 )
        {
            printf("cant get valid blocknum for %s\n",coinid_str(htx->coinid));
            return(-1);
        }
        blocknum += (locktime/cp->estblocktime) + 1;
    }
    if ( htx->fundingtxid == 0 )
    {
        //printf("create funding TX\n");
        if ( (htx->fundingtxid= subatomic_create_fundingtx(htx,htx->amount)) == 0 )
            return(-1);
        htx->avail = htx->myamount;
    }
    if ( htx->refundtxid == 0 )
    {
        // printf("create refund TX\n");
        if ( (htx->refundtxid= subatomic_create_paytx(&htx->refund,0,htx,atx->otherhalf.coinaddr,blocknum,1.,SUBATOMIC_STARTING_SEQUENCEID-1)) == 0 )
            return(-1);
        //printf("created refundtx.(%s)\n",htx->refundtxid);
        atx->refundlockblock = blocknum;
    }
    if ( htx->micropaytxid == 0 )
    {
        //printf("create micropay TX\n");
        htx->micropaytxid = subatomic_create_paytx(&htx->micropay,htx->coinaddr,htx,atx->otherhalf.coinaddr,0,1.,SUBATOMIC_STARTING_SEQUENCEID);
        if ( htx->micropaytxid == 0 )
            return(-1);
    }
    return(0);
}

int32_t subatomic_validate_refund(struct subatomic_tx *atx,struct subatomic_halftx *htx)
{
    struct daemon_info *cp = get_daemon_info(htx->coinid);
    int64_t value;
    int32_t lockedblock;
    if ( cp == 0 )
        return(-1);
    printf("validate refund\n");
    if ( subatomic_signtx(atx->myhalf.xferaddr->multisigaddr,&lockedblock,&value,htx->coinaddr,htx->countersignedrefund,sizeof(htx->countersignedrefund),cp,cp->coinid,&htx->refund,htx->refund.signedtransaction) == 0 )
    {
        printf("error signing refund\n");
        return(-1);
    }
    printf("refund signing completed.%d\n",htx->refund.completed);
    if ( htx->refund.completed <= 0 )
        return(-1);
    printf(">>>>>>>>>>>>>>>>>>>>> refund at %d is locked! txid.%s completed %d %.8f -> %s\n",lockedblock,htx->refund.txid,htx->refund.completed,dstr(value),htx->coinaddr);
    atx->status = SUBATOMIC_HAVEREFUND;
    return(0);
}

double subatomic_calc_incr(struct subatomic_halftx *htx,int64_t value,int64_t den,int32_t numincr)
{
    //printf("value %.8f/%.8f numincr.%d -> %.6f\n",dstr(value),dstr(den),numincr,(((double)value/den) * numincr));
    return((((double)value/den) * numincr));
}

int32_t subatomic_validate_micropay(struct subatomic_tx *atx,char *skipaddr,char *destbytes,int32_t max,int64_t *valuep,struct subatomic_rawtransaction *rp,struct subatomic_halftx *htx,int32_t srccoinid,int64_t srcamount,int32_t numincr,char *refcoinaddr)
{
    struct daemon_info *cp = get_daemon_info(htx->coinid);
    int64_t value;
    int32_t lockedblock;
    if ( valuep != 0 )
        *valuep = 0;
    if ( cp == 0 )
        return(-1);
    if ( subatomic_signtx(skipaddr,&lockedblock,&value,refcoinaddr,destbytes,max,cp,cp->coinid,rp,rp->signedtransaction) == 0 )
        return(-1);
    if ( valuep != 0 )
        *valuep = value;
    if ( rp->completed <= 0 )
        return(-1);
    //printf("micropay is updated txid.%s completed %d %.8f -> %s, lockedblock.%d\n",rp->txid,rp->completed,dstr(value),htx->coinaddr,lockedblock);
    return(lockedblock);
}

int32_t process_microtx(struct subatomic_tx *atx,struct subatomic_rawtransaction *rp,int32_t incr,int32_t otherincr)
{
    int64_t value;
    if ( subatomic_validate_micropay(atx,0,atx->otherhalf.completedmicropay,(int32_t)sizeof(atx->otherhalf.completedmicropay),&value,rp,&atx->otherhalf,atx->ARGS.destcoinid,atx->ARGS.destamount,atx->ARGS.numincr,atx->myhalf.destcoinaddr) < 0 )
    {
        printf("Error validating micropay from NXT.%s %s %s\n",(atx->ARGS.otherNXTaddr),coinid_str(atx->myhalf.destcoinid),atx->myhalf.destcoinaddr);
        //subatomic_sendabort(atx);
        //atx->status = SUBATOMIC_ABORTED;
        //if ( incr > 1 )
        //    atx->claimtxid = subatomic_broadcasttx(&atx->otherhalf,atx->otherhalf.completedmicropay,0,0);
        return(-1);
    }
    else
    {
        atx->myreceived = value;
        otherincr = subatomic_calc_incr(&atx->otherhalf,value,atx->myexpectedamount,atx->ARGS.numincr);
    }
    if ( otherincr == atx->ARGS.numincr || value == atx->myhalf.destamount )
    {
        printf("TX complete!\n");
        atx->claimtxid = subatomic_broadcasttx(&atx->otherhalf,atx->otherhalf.completedmicropay,0,0);
        atx->status = SUBATOMIC_COMPLETED;
    }
    printf("[%5.2f%%] Received %12.8f of %12.8f | Sent %12.8f of %12.8f\n",100.*(double)atx->myreceived/atx->myexpectedamount,dstr(atx->myreceived),dstr(atx->myexpectedamount),dstr(atx->sent_to_other),dstr(atx->otherexpectedamount));
    
    //printf("incr.%d of %d, otherincr.%d %.8f %.8f \n",incr,atx->ARGS.numincr,otherincr,dstr(value),dstr(atx->myexpectedamount));
    return(otherincr);
}

int64_t subatomic_calc_micropay(struct subatomic_tx *atx,struct subatomic_halftx *htx,char *othercoinaddr,double myshare,int32_t seqid)
{
    struct subatomic_unspent_tx U;
    struct daemon_info *cp = get_daemon_info(htx->coinid);
    struct subatomic_rawtransaction *rp = &htx->micropay;
    if ( cp == 0 )
        return(0);
    subatomic_set_unspent_tx0(&U,htx);
    rp->numinputs = 0;
    rp->inputs[rp->numinputs++] = U;
    rp->amount = (htx->avail - cp->txfee);
    rp->change = 0;
    rp->inputsum = htx->avail;
    //printf("subatomic_sendincr myshare %f seqid.%d\n",myshare,seqid);
    if ( subatomic_calc_rawoutputs(htx,cp,rp,myshare,htx->coinaddr,othercoinaddr,Global_gp->internalmarker[cp->coinid]) > 0 )
    {
        subatomic_gen_rawtransaction(htx->xferaddr->multisigaddr,cp,rp,htx->coinaddr,0,seqid);
        return(htx->otheramount);
    }
    return(-1);
}

#ifdef deprecated
// networking functions
void subatomic_extract_pubkeys(struct subatomic_tx *atx,struct subatomic_packet *ptr,int32_t retflag)
{
    char *pubkey0,*pubkey1;
    if ( retflag == 0 )
    {
        pubkey0 = ptr->pubkeys[0];
        pubkey1 = ptr->pubkeys[1];
    }
    else
    {
        pubkey0 = ptr->retpubkeys[0];
        pubkey1 = ptr->retpubkeys[1];
    }
    printf("got pubkeys.(%s) and (%s)\n",pubkey0,pubkey1);
    if ( pubkey0[0] != 0 && pubkey1[0] != 0 )
    {
        strcpy(atx->otherhalf.destpubkey,pubkey0);
        strcpy(atx->otherhalf.pubkey,pubkey1);
        atx->otherARGS = ptr->ARGS;
    }
}

void subatomic_sendabort(struct subatomic_tx *atx)
{
    printf("subatomic_sendabort cantsend.%d\n",atx->cantsend);
    if ( atx->connsock >= 0 )
    {
        close(atx->connsock);
        atx->connsock = -1;
    }
}

int32_t subatomic_send_pubkeys(struct subatomic_tx *atx)
{
    int32_t i;
    member_t *pm;
    struct subatomic_packet P;
    //if ( atx->cantsend >= SUBATOMIC_CANTSEND_TOLERANCE )
    //    return(-1);
    //printf("subatomic_send_pubkeys cantsend.%d\n",atx->cantsend);
    memset(&P,0,sizeof(P));
    P.H.funcid = SUBATOMIC_SEND_PUBKEY;
    P.H.argsize = sizeof(P);
    P.ARGS = atx->ARGS;
    for (i=0; i<2; i++)
        strcpy(P.pubkeys[i],atx->ARGS.mypubkeys[i]);
    printf("send pubkeys %s (%s and %s) %s to %s\n",atx->ARGS.coinaddr[0],P.pubkeys[0],P.pubkeys[1],atx->ARGS.destcoinaddr[0],atx->ARGS.otheripaddr);
    pm = member_find_NXTaddr(atx->ARGS.otherNXTaddr);
    if ( pm != 0 && start_sharedmem_xfer(pm,&P,sizeof(P),3000000,100) == 0 )
    {
        if ( wait_for_pendingrecv(pm,3000000) == 0 )
        {
            memcpy(&P,pm->sharedmem,sizeof(P));
            subatomic_extract_pubkeys(atx,&P,1);
            atx->otherARGS = P.ARGS;
            atx->otherhalf.refund = P.rawtx;
            printf("got pubkeys %s and %s\n",P.retpubkeys[0],P.retpubkeys[1]);
            strcpy(atx->otherhalf.pubkey,P.retpubkeys[1]);
            strcpy(atx->otherhalf.destpubkey,P.retpubkeys[0]);
            return(0);
        }
    }
    return(-1);
}

int32_t subatomic_submit_rawtransaction(struct subatomic_tx *atx,struct subatomic_rawtransaction *rp,int32_t funcid)
{
    member_t *pm;
    struct subatomic_packet P;
    //printf("subatomic_submit_rawtransaction cantsend.%d\n",atx->cantsend);
    //if ( atx->cantsend >= SUBATOMIC_CANTSEND_TOLERANCE )
    //    return;
    memset(&P,0,sizeof(P));
    P.H.funcid = funcid;
    P.H.argsize = sizeof(P);
    P.ARGS = atx->ARGS;
    P.rawtx = *rp;
    //printf("submit rawtx.%c len.%d\n",P.H.funcid,P.H.argsize);
    pm = member_find_NXTaddr(atx->ARGS.otherNXTaddr);
    if ( pm != 0 && start_sharedmem_xfer(pm,&P,sizeof(P),3000000,100) == 0 )
    {
        if ( wait_for_pendingrecv(pm,3000000) == 0 )
        {
            memcpy(&P,pm->sharedmem,sizeof(P));
            if ( funcid == SUBATOMIC_REFUNDTX_NEEDSIG )
                strcpy(atx->myhalf.refund.signedtransaction,P.rawtx.signedtransaction);
            else if ( funcid == SUBATOMIC_REFUNDTX_SIGNED )
                atx->otherhalf.funding = P.rawtx;
            else if ( funcid == SUBATOMIC_SEND_MICROTX )
                atx->otherhalf.micropay = P.rawtx;
            return(0);
        }
    }
    return(-1);
}

int32_t process_subatomic_packet(struct subatomic_tx *atx,struct subatomic_packet *ptr)
{
    struct subatomic_halftx *htx;
    struct subatomic_rawtransaction *rp;
    int32_t lockedblock;//,len = ptr->H.argsize;
    int64_t value;
    //printf("got packet.(%c) len.%d from (%s)\n",ptr->H.funcid,len,ipaddr);
    //if ( len < 0 )
    //    printf("ipaddr.(%s) disconnected\n",ipaddr);
    //else
    {
        if ( atx != 0 )
        {
            //printf("got.(%c) packet from %s, len %d (%s vs NXT.%s) %s %s <-> otherNXT.%s\n",ptr->H.funcid,ipaddr,len,(ptr->ARGS.NXTaddr),atx->ARGS.NXTaddr,coinid_str(ptr->ARGS.coinid),ptr->ARGS.coinaddr[0],atx->ARGS.otherNXTaddr);
            atx->lastcontact = microseconds();
            htx = &atx->myhalf;
            switch ( ptr->H.funcid )
            {
                case SUBATOMIC_SEND_PUBKEY:
                    subatomic_gen_pubkeys(atx,htx);
                    subatomic_extract_pubkeys(atx,ptr,0);
                    subatomic_gen_multisig(atx,htx);
                    if ( htx->xferaddr != 0 )
                    {
                        subatomic_ensure_txs(atx,htx,(atx->longerflag * SUBATOMIC_LOCKTIME));
                        ptr->rawtx = atx->myhalf.refund;
                    }
                    ptr->ARGS = atx->ARGS;
                    strcpy(ptr->retpubkeys[0],atx->myhalf.pubkey);
                    strcpy(ptr->retpubkeys[1],atx->myhalf.destpubkey);
                    printf("set retpubkeys (%s) (%s)\n",ptr->retpubkeys[0],ptr->retpubkeys[1]);
                    break;
                case SUBATOMIC_REFUNDTX_NEEDSIG:
                    rp = &ptr->rawtx;
                    subatomic_signtx(0,&lockedblock,&value,htx->destcoinaddr,rp->signedtransaction,sizeof(rp->signedtransaction),get_daemon_info(htx->destcoinid),htx->destcoinid,rp,rp->rawtransaction);
                    break;
                case SUBATOMIC_REFUNDTX_SIGNED:
                    strcpy(htx->refund.signedtransaction,ptr->rawtx.signedtransaction);
                    //printf("SUBATOMIC_REFUNDTX_SIGNED signed.(%s)\n",htx->refund.signedtransaction);
                    if ( subatomic_validate_refund(atx,htx) < 0 )
                    {
                        memset(&ptr->rawtx,0,sizeof(ptr->rawtx));
                        printf("warning: other side NXT.%s returned invalid signed refund\n",htx->otherNXTaddr);
                    }
                    else
                    {
                        subatomic_broadcasttx(&atx->myhalf,atx->myhalf.countersignedrefund,0,atx->refundlockblock);
                        ptr->rawtx = htx->funding;
                    }
                    break;
                case SUBATOMIC_FUNDINGTX:
                    atx->otherhalf.funding = ptr->rawtx;
                    printf("got funding.(%s)\n",ptr->rawtx.signedtransaction);
                    subatomic_broadcasttx(&atx->otherhalf,atx->otherhalf.funding.signedtransaction,0,atx->refundlockblock);
                    break;
                case SUBATOMIC_SEND_MICROTX:
                    atx->otherhalf.micropay = ptr->rawtx;
                    //printf("micropay.(%s)\n",atx->otherhalf.micropay.signedtransaction);
                    atx->ARGS.myshare = ((double)atx->ARGS.numincr - ptr->ARGS.incr) / atx->ARGS.numincr;
                    atx->ARGS.incr = ptr->ARGS.incr;
                    atx->ARGS.otherincr = process_microtx(atx,&atx->otherhalf.micropay,atx->ARGS.incr,atx->ARGS.otherincr);
                    if ( atx->ARGS.otherincr < 0 )
                        break;
                    atx->sent_to_other = subatomic_calc_micropay(atx,htx,atx->otherhalf.coinaddr,atx->ARGS.myshare,SUBATOMIC_STARTING_SEQUENCEID+atx->ARGS.incr);
                    ptr->rawtx = htx->micropay;
                    break;
                default: printf("unsupported func.(%c) %d??\n",ptr->H.funcid,ptr->H.funcid); return(0); break;
            }
        }
    }
    return(sizeof(struct subatomic_packet));
}

int32_t NXTsync_dispatch(void **ptrp,void *ignore)
{
    struct subatomic_packet *ptr = *ptrp;
    struct subatomic_tx *atx;
    member_t *pm;
    //printf("NXTsync_dispatch\n");
    if ( ignore == 0 || memcmp(ignore,ptr,sizeof(*ptr)) != 0 )
    {
        pm = member_find_NXTaddr(ptr->ARGS.NXTaddr);
        if ( pm != 0 )
        {
            atx = subatomic_search_connections(pm->NXTaddr,1);
            printf("NXTsync_dispatch got packet from %s %s atx.%p\n",pm->name,pm->NXTaddr,atx);
            if ( atx != 0 )
            {
                process_subatomic_packet(atx,ptr);
                start_sharedmem_xfer(pm,ptr,sizeof(*ptr),1,100);
            }
        } else printf("cant find NXT.(%s) funcid.%d\n",ptr->ARGS.NXTaddr,ptr->H.funcid);
    }
    free(ptr);
    *ptrp = 0;
    return(1);
}
#endif

void subatomic_callback(struct NXT_acct *np,int32_t fragi,struct subatomic_tx *atx,struct json_AM *ap,cJSON *json,void *binarydata,int32_t binarylen,uint32_t *targetcrcs)
{
    void *ptr;
    uint32_t TCRC,tcrc;
    int32_t *iptr,coinid,i,j,n,starti,completed,incr,ind,funcid = (ap->funcid & 0xffff);
    cJSON *addrobj,*pubkeyobj,*txobj;
    char coinaddr[64],pubkey[128],txbytes[1024];
    if ( funcid == SUBATOMIC_SEND_ATOMICTX )
    {
        // printf("got atomictx\n");
        txobj = cJSON_GetObjectItem(json,"txbytes");
        if ( txobj != 0 )
        {
            copy_cJSON(txbytes,txobj);
            if ( strlen(txbytes) > 32 )
            {
                //printf("atx.%p TXBYTES.(%s)\n",atx,txbytes);
                safecopy(atx->swap.signedtxbytes[1],txbytes,sizeof(atx->swap.signedtxbytes[1]));
                atx->swap.atomictx_waiting = 1;
            }
        }
    }
    else if ( funcid == SUBATOMIC_SEND_PUBKEY )
    {
        if ( json != 0 )
        {
            coinid = (int32_t)get_cJSON_int(json,"coinid");
            if ( coinid != 0 )
            {
                addrobj = cJSON_GetObjectItem(json,coinid_str(coinid));
                copy_cJSON(coinaddr,addrobj);
                if ( strcmp(coinaddr,atx->otherhalf.coinaddr) == 0 )
                {
                    pubkeyobj = cJSON_GetObjectItem(json,"pubkey");
                    copy_cJSON(pubkey,pubkeyobj);
                    if ( strlen(pubkey) >= 64 )
                        strcpy(atx->otherhalf.pubkey,pubkey);
                }
            }
        }
    }
    else
    {
        starti = (int32_t)get_cJSON_int(json,"starti");
        i = (int32_t)get_cJSON_int(json,"i");
        n = (int32_t)get_cJSON_int(json,"n");
        incr = (int32_t)get_cJSON_int(json,"incr");
        TCRC = (uint32_t)get_cJSON_int(json,"TCRC");
        if ( incr == binarylen && starti >= 0 && starti < SYNC_MAXUNREPORTED && i >= 0 && i < SYNC_MAXUNREPORTED && n >= 0 && n < SYNC_MAXUNREPORTED )
        {
            completed = 0;
            for (j=starti; j<starti+n; j++)
            {
                //printf("(%08x vs %08x) ",np->memcrcs[j],targetcrcs[j]);
                if ( np->memcrcs[j] == targetcrcs[j] )
                    completed++;
            }
            ind = -1;
            ptr = 0; iptr = 0;
            switch ( funcid )
            {
                default: printf("illegal funcid.%d\n",funcid); break;
                case SUBATOMIC_REFUNDTX_NEEDSIG: ptr = &atx->otherhalf.refund; ind = 0; iptr = &atx->other_refundtx_waiting; break;
                case SUBATOMIC_REFUNDTX_SIGNED: ptr = &atx->myhalf.refund; ind = 1; iptr = &atx->myrefundtx_waiting; break;
                case SUBATOMIC_FUNDINGTX: ptr = &atx->otherhalf.funding; ind = 2; iptr = &atx->other_fundingtx_waiting; break;
                case SUBATOMIC_SEND_MICROTX: ptr = &atx->otherhalf.micropay; ind = 3; iptr = &atx->other_micropaytx_waiting; break;
            }
            if ( ind >= 0 )
            {
                memcpy(atx->recvbufs[ind]+i*incr,binarydata,binarylen);
                tcrc = _crc32(0,atx->recvbufs[ind],sizeof(struct subatomic_rawtransaction));
                if ( completed == n && TCRC == tcrc )
                {
                    //printf("completed.%d ptr.%p ind.%d i.%d binarydata.%p binarylen.%d crc.%u vs %u\n",completed,ptr,ind,i,binarydata,binarylen,tcrc,TCRC);
                    memcpy(ptr,atx->recvbufs[ind],sizeof(struct subatomic_rawtransaction));
                }
            }
            if ( iptr != 0 )
                *iptr = completed;
        }
    }
}

struct subatomic_tx *subatomic_search_connections(char *NXTaddr,int32_t otherflag,int32_t type)
{
    struct subatomic_info *gp = Global_subatomic;
    int32_t i;
    struct subatomic_tx *atx;
    for (i=0; i<gp->numsubatomics; i++)
    {
        atx = gp->subatomics[i];
        if ( type != atx->type || atx->initflag != 3 || atx->status == SUBATOMIC_COMPLETED || atx->status == SUBATOMIC_ABORTED )
            continue;
        if ( otherflag == 0 && strcmp(NXTaddr,atx->ARGS.NXTaddr) == 0 )  // assumes one IP per NXT addr
            return(atx);
        else if ( otherflag != 0 && strcmp(NXTaddr,atx->ARGS.otherNXTaddr) == 0 )  // assumes one IP per NXT addr
            return(atx);
    }
    return(0);
}

void sharedmem_callback(member_t *pm,int32_t fragi,void *ptr,uint32_t crc,uint32_t *targetcrcs)
{
    cJSON *json;
    void *binarydata;
    struct subatomic_tx *atx;
    int32_t createdflag,binarylen;
    char otherNXTaddr[64],*jsontxt = 0;
    struct NXT_acct *np;
    struct json_AM *ap = ptr;
    expand_nxt64bits(otherNXTaddr,ap->H.nxt64bits);
    if ( strcmp(pm->NXTaddr,otherNXTaddr) != 0 )
        printf("WARNING: mismatched member NXT.%s vs sender.%s\n",pm->NXTaddr,otherNXTaddr);
    binarylen = (ap->funcid>>16)&0xffff;
    if ( ap->H.size > 16 && ap->H.size+binarylen < 1024 && binarylen > 0 )
        binarydata = (void *)((long)ap + ap->H.size);
    else binarydata = 0;
    json = parse_json_AM(ap);
    if ( json != 0 )
        jsontxt = cJSON_Print(json);
    np = get_NXTacct(&createdflag,Global_mp,otherNXTaddr);
    np->recvid++;
    np->memcrcs[fragi] = crc;
    //printf("[R%d S%d] other.[R%d S%d] %x size.%d %p.[%08x].binarylen.%d %s (funcid.%d arg.%d seqid.%d flag.%d) [%s]\n",np->recvid,np->sentid,ap->gatewayid,ap->timestamp,ap->H.sig,ap->H.size,binarydata,binarydata!=0?*(int *)binarydata:0,binarylen,nxt64str(ap->H.nxt64bits),ap->funcid&0xffff,ap->gatewayid,ap->timestamp,ap->jsonflag,jsontxt!=0?jsontxt:"");
    if ( ap->H.sig == SUBATOMIC_SIG )
    {
        if ( (ap->funcid&0xffff) == SUBATOMIC_SEND_ATOMICTX )
            atx = subatomic_search_connections(otherNXTaddr,1,ATOMICSWAP_TYPE);
        else
            atx = subatomic_search_connections(otherNXTaddr,1,0);
        if ( atx != 0 )
            subatomic_callback(np,fragi,atx,ap,json,binarydata,binarylen,targetcrcs);
    }
    if ( jsontxt != 0 )
        free(jsontxt);
    if ( json != 0 )
        free_json(json);
}

int32_t share_pubkey(struct NXT_acct *np,int32_t fragi,int32_t destcoinid,char *destcoinaddr,char *destpubkey)
{
    char jsonstr[512];
    // also check other pubkey and if matches atx->otherhalf.coinaddr set atx->otherhalf.pubkey
    sprintf(jsonstr,"{\"coinid\":%d,\"%s\":\"%s\",\"pubkey\":\"%s\"}",destcoinid,coinid_str(destcoinid),destcoinaddr,destpubkey);
    send_to_NXTaddr(&np->localcrcs[fragi],np->H.NXTaddr,fragi,SUBATOMIC_SIG,SUBATOMIC_SEND_PUBKEY,jsonstr,0,0);
    return(fragi+1);
}

int32_t share_atomictx(struct NXT_acct *np,char *txbytes,int32_t fragi)
{
    char jsonstr[512];
    sprintf(jsonstr,"{\"txbytes\":\"%s\"}",txbytes);
    send_to_NXTaddr(&np->localcrcs[fragi],np->H.NXTaddr,fragi,SUBATOMIC_SIG,SUBATOMIC_SEND_ATOMICTX,jsonstr,0,0);
    return(fragi+1);
}

int32_t share_tx(struct NXT_acct *np,struct subatomic_rawtransaction *rp,int32_t startfragi,int32_t funcid)
{
    uint32_t TCRC;
    int32_t incr,size;
    char i,n,jsonstr[512];
    incr = (int32_t)(SYNC_FRAGSIZE - sizeof(struct json_AM) - 60);
    size = (sizeof(*rp) - sizeof(rp->inputs) + sizeof(rp->inputs[0])*rp->numinputs);
    n = size / incr;
    if ( (size / incr) != 0 )
        n++;
    TCRC = _crc32(0,rp,sizeof(struct subatomic_rawtransaction));
    for (i=0; i<n; i++)
    {
        sprintf(jsonstr,"{\"TCRC\":%u,\"starti\":%d,\"i\":%d,\"n\":%d,\"incr\":%d}",TCRC,startfragi,i,n,incr);
        send_to_NXTaddr(&np->localcrcs[startfragi+i],np->H.NXTaddr,startfragi+i,SUBATOMIC_SIG,funcid,jsonstr,(void *)((long)rp+i*incr),incr);
    }
    return(startfragi + n);
}

int32_t share_refundtx(struct NXT_acct *np,struct subatomic_rawtransaction *rp,int32_t startfragi)
{
    return(share_tx(np,rp,startfragi,SUBATOMIC_REFUNDTX_NEEDSIG));
}

int32_t share_other_refundtx(struct NXT_acct *np,struct subatomic_rawtransaction *rp,int32_t startfragi)
{
    return(share_tx(np,rp,startfragi,SUBATOMIC_REFUNDTX_SIGNED));
}

int32_t share_fundingtx(struct NXT_acct *np,struct subatomic_rawtransaction *rp,int32_t startfragi)
{
    return(share_tx(np,rp,startfragi,SUBATOMIC_FUNDINGTX));
}

int32_t share_micropaytx(struct NXT_acct *np,struct subatomic_rawtransaction *rp,int32_t startfragi)
{
    return(share_tx(np,rp,startfragi,SUBATOMIC_SEND_MICROTX));
}


/*{
 "sender": "8989816935121514892",
 "timestamp": 13000704,
 "referencedTransaction": "0",
 "hash": "3bfcbb9e173ab146b9538e3d606417f53885d34360c75b40d4dd2123245d6e91",
 "amountNQT": "100000000",
 "feeNQT": "100000000",
 "senderPublicKey": "25c5fed2690701cf06f267e7c227b1a3c0dfa9c6fc3cdb593b3af6f16d65302f",
 "verify": false,
 "subtype": 0,
 "type": 0,
 "deadline": 666,
 "recipient": "423766016895692955"
 }*/


int32_t NXTutxcmp(struct NXT_tx *ref,struct NXT_tx *tx,double myshare)
{
    if ( ref->senderbits == tx->senderbits && ref->recipientbits == tx->recipientbits && ref->type == tx->type && ref->subtype == tx->subtype)
    {
        if ( ref->feeNQT != tx->feeNQT || ref->deadline != tx->deadline )
            return(-1);
        if ( ref->assetidbits != 0 )
        {
            if ( ref->assetidbits == tx->assetidbits && fabs((ref->quantityQNT*myshare) - tx->quantityQNT) < 0.5 && strcmp(ref->comment,tx->comment) == 0 )
                return(0);
        }
        else
        {
            if ( fabs((ref->amountNQT*myshare) - tx->amountNQT) < 0.5 )
                return(0);
        }
    }
    return(-1);
}

cJSON *gen_NXT_tx_json(struct NXT_tx *utx,char *reftxid,double myshare)
{
    cJSON *json = 0;
    char cmd[1024],destNXTaddr[64],assetidstr[64],*retstr;
    if ( utx->senderbits == Global_mp->nxt64bits )
    {
        expand_nxt64bits(destNXTaddr,utx->recipientbits);
        cmd[0] = 0;
        if ( utx->type == 0 && utx->subtype == 0 )
            sprintf(cmd,"%s=sendMoney&amountNQT=%lld",_NXTSERVER,(long long)(utx->amountNQT*myshare));
        else if ( utx->type == 2 && utx->subtype == 1 )
        {
            expand_nxt64bits(assetidstr,utx->assetidbits);
            sprintf(cmd,"%s=transferAsset&asset=%s&quantityQNT=%lld",_NXTSERVER,assetidstr,(long long)(utx->quantityQNT*myshare));
            if ( utx->comment[0] != 0 )
                strcat(cmd,"&comment="),strcat(cmd,utx->comment);
        }
        else printf("unsupported type.%d subtype.%d\n",utx->type,utx->subtype);
        if ( reftxid != 0 && reftxid[0] != 0 && cmd[0] != 0 )
            strcat(cmd,"&referencedTransactionFullHash="),strcat(cmd,reftxid);
        if ( cmd[0] != 0 )
        {
            sprintf(cmd+strlen(cmd),"&deadline=%u&feeNQT=%lld&secretPhrase=%s&recipient=%s&broadcast=false",utx->deadline,(long long)utx->feeNQT,Global_mp->NXTACCTSECRET,destNXTaddr);
            printf("generated cmd.(%s)\n",cmd);
            retstr = issue_NXTPOST(Global_subatomic->curl_handle,cmd);
            if ( retstr != 0 )
            {
                json = cJSON_Parse(retstr);
                //if ( json != 0 )
                //    printf("Parsed.(%s)\n",cJSON_Print(json));
                free(retstr);
            }
        }
    } else printf("cant gen_NXT_txjson when sender is not me\n");
    return(json);
}

void set_NXTtx(struct NXT_tx *tx,uint64_t assetidbits,int64_t amount,uint64_t other64bits)
{
    struct NXT_tx U;
    memset(&U,0,sizeof(U));
    U.senderbits = calc_nxt64bits(Global_mp->NXTADDR);
    U.recipientbits = other64bits;
    U.assetidbits = assetidbits;
    if ( assetidbits != 0 )
    {
        U.type = 2;
        U.subtype = 1;
        U.quantityQNT = amount;
    } else U.amountNQT = amount;
    U.feeNQT = MIN_NQTFEE;
    U.deadline = DEFAULT_NXT_DEADLINE;
    *tx = U;
}

int32_t calc_raw_NXTtx(char *utxbytes,char *sighash,uint64_t assetidbits,int64_t amount,uint64_t other64bits)
{
    int32_t retval = -1;
    struct NXT_tx U;
    cJSON *json;
    long n;
    utxbytes[0] = sighash[0] = 0;
    set_NXTtx(&U,assetidbits,amount,other64bits);
    json = gen_NXT_tx_json(&U,0,1.);
    if ( json != 0 )
    {
        if ( extract_cJSON_str(utxbytes,1024,json,"transactionBytes") > 0 && extract_cJSON_str(sighash,1024,json,"signatureHash") > 0 )
        {
            n = strlen(utxbytes);
            if ( n > 128 )
                utxbytes[n-128] = 0;
            retval = 0;
            printf("generated utx.(%s) sighash.(%s)\n",utxbytes,sighash);
        }
        free_json(json);
    }
    return(retval);
}

struct NXT_tx *set_NXT_tx(cJSON *json)
{
    long size;
    int32_t n = 0;
    uint64_t assetidbits,quantity;
    cJSON *attachmentobj;
    struct NXT_tx *utx = 0;
    char sender[1024],recipient[1024],deadline[1024],feeNQT[1024],amountNQT[1024],type[1024],subtype[1024],verify[1024],referencedTransaction[1024],quantityQNT[1024],comment[1024],assetidstr[1024];
    if ( json == 0 )
        return(0);
    if ( extract_cJSON_str(sender,sizeof(sender),json,"sender") > 0 ) n++;
    if ( extract_cJSON_str(recipient,sizeof(recipient),json,"recipient") > 0 ) n++;
    if ( extract_cJSON_str(referencedTransaction,sizeof(referencedTransaction),json,"referencedTransaction") > 0 ) n++;
    if ( extract_cJSON_str(amountNQT,sizeof(amountNQT),json,"amountNQT") > 0 ) n++;
    if ( extract_cJSON_str(feeNQT,sizeof(feeNQT),json,"feeNQT") > 0 ) n++;
    if ( extract_cJSON_str(deadline,sizeof(deadline),json,"deadline") > 0 ) n++;
    if ( extract_cJSON_str(type,sizeof(type),json,"type") > 0 ) n++;
    if ( extract_cJSON_str(subtype,sizeof(subtype),json,"subtype") > 0 ) n++;
    if ( extract_cJSON_str(verify,sizeof(verify),json,"verify") > 0 ) n++;
    comment[0] = 0;
    assetidbits = quantity = 0;
    size = sizeof(*utx);
    if ( strcmp(type,"2") == 0 && strcmp(subtype,"1") == 0 )
    {
        attachmentobj = cJSON_GetObjectItem(json,"attachment");
        if ( attachmentobj != 0 )
        {
            if ( extract_cJSON_str(assetidstr,sizeof(assetidstr),attachmentobj,"asset") > 0 )
                assetidbits = calc_nxt64bits(assetidstr);
            if ( extract_cJSON_str(comment,sizeof(comment),attachmentobj,"comment") > 0 )
                size += strlen(comment);
            if ( extract_cJSON_str(quantityQNT,sizeof(quantityQNT),attachmentobj,"quantityQNT") > 0 )
                quantity = calc_nxt64bits(quantityQNT);
        }
    }
    utx = malloc(size);
    memset(utx,0,size);
    if ( strlen(referencedTransaction) == 64 )
        decode_hex(utx->refhash,32,referencedTransaction);
    utx->senderbits = calc_nxt64bits(sender);
    utx->recipientbits = calc_nxt64bits(recipient);
    utx->assetidbits = assetidbits;
    utx->feeNQT = calc_nxt64bits(feeNQT);
    if ( quantity != 0 )
        utx->quantityQNT = quantity;
    else utx->amountNQT = calc_nxt64bits(amountNQT);
    utx->deadline = myatoi(deadline,1000);
    utx->type = myatoi(type,256);
    utx->subtype = myatoi(subtype,256);
    utx->verify = (strcmp("true",verify) == 0);
    strcpy(utx->comment,comment);
    return(utx);
}

struct NXT_tx *sign_NXT_tx(CURL *curl_handle,char signedtx[1024],struct NXT_tx *utx,char *reftxid,double myshare)
{
    cJSON *refjson,*txjson;
    char *parsed;
    struct NXT_tx *refutx = 0;
    txjson = gen_NXT_tx_json(utx,reftxid,myshare);
    signedtx[0] = 0;
    if ( txjson != 0 )
    {
        if ( extract_cJSON_str(signedtx,1024,txjson,"transactionBytes") > 0 )
        {
            if ( (parsed = issue_parseTransaction(curl_handle,signedtx)) != 0 )
            {
                refjson = cJSON_Parse(parsed);
                if ( refjson != 0 )
                {
                    refutx = set_NXT_tx(refjson);
                    free_json(refjson);
                }
                free(parsed);
            }
        }
        free_json(txjson);
    }
    return(refutx);
}

int32_t update_atomic(struct NXT_acct *np,struct subatomic_tx *atx)
{
    cJSON *txjson,*json;
    int32_t j,status = 0;
    char signedtx[1024],fullhash[128],*parsed,*retstr,*padded;
    struct atomic_swap *sp;
    struct NXT_tx *utx,*refutx = 0;
    sp = &atx->swap;
    if ( atx->longerflag != 1 )
    {
        //printf("atomixtx waiting.%d atx.%p\n",sp->atomictx_waiting,atx);
        if ( sp->atomictx_waiting != 0 )
        {
            printf("GOT.(%s)\n",sp->signedtxbytes[1]);
            if ( (parsed = issue_parseTransaction(Global_subatomic->curl_handle,sp->signedtxbytes[1])) != 0 )
            {
                json = cJSON_Parse(parsed);
                if ( json != 0 )
                {
                    refutx = set_NXT_tx(sp->jsons[1]);
                    utx = set_NXT_tx(json);
                    //printf("refutx.%p utx.%p verified.%d\n",refutx,utx,utx->verify);
                    if ( utx != 0 && refutx != 0 && utx->verify != 0 )
                    {
                        if ( NXTutxcmp(refutx,utx,1.) == 0 )
                        {
                            padded = malloc(strlen(sp->txbytes[0]) + 129);
                            strcpy(padded,sp->txbytes[0]);
                            for (j=0; j<128; j++)
                                strcat(padded,"0");
                            retstr = issue_signTransaction(Global_subatomic->curl_handle,padded);
                            free(padded);
                            printf("got signed tx that matches agreement submit.(%s) (%s)\n",padded,retstr);
                            if ( retstr != 0 )
                            {
                                txjson = cJSON_Parse(retstr);
                                if ( txjson != 0 )
                                {
                                    extract_cJSON_str(sp->signedtxbytes[0],sizeof(sp->signedtxbytes[0]),txjson,"transactionBytes");
                                    if ( extract_cJSON_str(fullhash,sizeof(fullhash),txjson,"fullHash") > 0 )
                                    {
                                        if ( strcmp(fullhash,sp->fullhash[0]) == 0 )
                                        {
                                            printf("broadcast (%s) and (%s)\n",sp->signedtxbytes[0],sp->signedtxbytes[1]);
                                            status = SUBATOMIC_COMPLETED;
                                        }
                                        else printf("ERROR: can't reproduct fullhash of trigger tx %s != %s\n",fullhash,sp->fullhash[0]);
                                    }
                                    free_json(txjson);
                                }
                                free(retstr);
                            }
                        } else printf("tx compare error\n");
                    }
                    if ( utx != 0 ) free(utx);
                    if ( refutx != 0 ) free(refutx);
                    free_json(json);
                } else printf("error JSON parsing.(%s)\n",parsed);
                free(parsed);
            } else printf("error parsing (%s)\n",sp->signedtxbytes[1]);
            sp->atomictx_waiting = 0;
        }
    }
    else if ( atx->longerflag == 1 )
    {
        if ( sp->numfragis == 0 )
        {
            utx = set_NXT_tx(sp->jsons[0]);
            if ( utx != 0 )
            {
                refutx = sign_NXT_tx(Global_subatomic->curl_handle,signedtx,utx,sp->fullhash[1],1.);
                /*txjson = gen_NXT_tx_json(utx,sp->fullhash[1],1.);
                 signedtx[0] = 0;
                 if ( txjson != 0 )
                 {
                 if ( extract_cJSON_str(signedtx,sizeof(signedtx),txjson,"transactionBytes") > 0 )
                 {
                 if ( (parsed = issue_parseTransaction(signedtx)) != 0 )
                 {
                 refjson = cJSON_Parse(parsed);
                 if ( refjson != 0 )
                 {
                 refutx = set_NXT_tx(refjson);
                 free_json(refjson);
                 }
                 free(parsed);
                 }
                 }
                 free_json(txjson);
                 }*/
                if ( refutx != 0 )
                {
                    if ( NXTutxcmp(refutx,utx,1.) == 0 )
                    {
                        printf("signed and referenced tx verified\n");
                        safecopy(sp->signedtxbytes[0],signedtx,sizeof(sp->signedtxbytes[0]));
                        sp->numfragis = share_atomictx(np,sp->signedtxbytes[0],1);
                        status = SUBATOMIC_COMPLETED;
                    }
                    free(refutx);
                }
                free(utx);
            }
        }
        else
        {
            // wont get here now, eventually add checks for blockchain completion or direct xfer from other side
            share_atomictx(np,sp->signedtxbytes[0],1);
        }
    }
    return(status);
}

int32_t verify_txs_created(struct NXT_acct *np,struct subatomic_tx *atx)
{
    struct subatomic_halftx *htx = &atx->myhalf;
    char signedtx[1024];
    double myshare = .01;
    struct atomic_swap *sp;
    struct NXT_tx *utx,*refutx = 0;
    if ( atx->type == NXTFOR_SUBATOMIC_TYPE || atx->type == ATOMICSWAP_TYPE )
    {
        sp = &atx->swap;
        if ( sp->numfragis == 0 )
        {
            utx = set_NXT_tx(sp->jsons[0]);
            if ( utx != 0 )
            {
                refutx = sign_NXT_tx(Global_subatomic->curl_handle,signedtx,utx,sp->fullhash[1],myshare);
                if ( refutx != 0 )
                {
                    if ( NXTutxcmp(refutx,utx,myshare) == 0 )
                    {
                        printf("signed and referenced tx verified\n");
                        safecopy(sp->signedtxbytes[0],signedtx,sizeof(sp->signedtxbytes[0]));
                        //sp->numfragis = share_atomictx(np,sp->signedtxbytes[0],1);
                        //status = SUBATOMIC_COMPLETED;
                        sp->numfragis = 2;
                        sp->mytx = utx;
                        return(1);
                    }
                    free(refutx);
                }
                //free(utx);
            }
        }
        return(0);
    }
    if ( atx->other_refundtx_done == 0 )
        printf("[R%d S%d] multisig addrs %d %d %d %d | refundtx.%d xferaddr.%p\n",np->recvid,np->sentid,atx->myhalf.coinaddr[0],atx->myhalf.pubkey[0],atx->otherhalf.coinaddr[0],atx->otherhalf.pubkey[0],atx->other_refundtx_done,atx->myhalf.xferaddr);
    if ( atx->other_refundtx_done == 0 )
        atx->myrefund_fragi = share_pubkey(np,1,htx->destcoinid,htx->destcoinaddr,htx->destpubkey);
    if ( atx->myhalf.xferaddr == 0 )
    {
        if ( atx->otherhalf.pubkey[0] != 0 )
        {
            printf(">>>> multisig addrs %d %d %d %d\n",htx->coinaddr[0],htx->pubkey[0],atx->otherhalf.coinaddr[0],atx->otherhalf.pubkey[0]);
            subatomic_gen_multisig(atx,htx);
            if ( atx->myhalf.xferaddr != 0 )
                printf("generated multisig\n");
        }
    }
    if ( atx->myhalf.xferaddr == 0 )
    {
        return(0);
    }
    if ( atx->txs_created == 0 && subatomic_ensure_txs(atx,htx,(atx->longerflag * SUBATOMIC_LOCKTIME)) < 0 )
    {
        printf("warning: cant create required transactions, probably lack of funds\n");
        return(-1);
    }
    if ( atx->other_refund_fragi == 0 )
        atx->other_refund_fragi = share_refundtx(np,&htx->refund,atx->myrefund_fragi);
    return(1);
}

int32_t update_other_refundtxdone(struct NXT_acct *np,struct subatomic_tx *atx)
{
    struct subatomic_halftx *htx = &atx->myhalf;
    struct subatomic_rawtransaction *rp;
    int32_t lockedblock;
    int64_t value;
    if ( atx->type == SUBATOMIC_FORNXT_TYPE || atx->type == ATOMICSWAP_TYPE )
    {
        atx->other_refundtx_done = 1;
    }
    else
    {
        rp = &atx->otherhalf.refund;
        if ( atx->other_refundtx_done == 0 )
        {
            if ( atx->other_refundtx_waiting != 0 )
            {
                if ( subatomic_signtx(0,&lockedblock,&value,htx->destcoinaddr,rp->signedtransaction,sizeof(rp->signedtransaction),get_daemon_info(htx->destcoinid),htx->destcoinid,rp,rp->rawtransaction) == 0 )
                {
                    printf("warning: error signing other's NXT.%s refund\n",htx->otherNXTaddr);
                    return(0);
                }
                atx->funding_fragi = share_other_refundtx(np,rp,atx->other_refund_fragi);
                atx->other_refundtx_done = 1;
                printf("other refundtx done\n");
            }
        }
    }
    return(atx->other_refundtx_done);
}

int32_t update_my_refundtxdone(struct NXT_acct *np,struct subatomic_tx *atx)
{
    struct subatomic_halftx *htx = &atx->myhalf;
    if ( atx->type == NXTFOR_SUBATOMIC_TYPE || atx->type == ATOMICSWAP_TYPE )
    {
        atx->myrefundtx_done = 1;
    }
    else
    {
        if ( atx->myrefundtx_done == 0 )
        {
            if ( atx->myrefundtx_waiting != 0 )
            {
                if ( subatomic_validate_refund(atx,htx) < 0 )
                {
                    printf("warning: other side NXT.%s returned invalid signed refund\n",htx->otherNXTaddr);
                    return(0);
                }
                subatomic_broadcasttx(&atx->myhalf,atx->myhalf.countersignedrefund,0,atx->refundlockblock);
                atx->myrefundtx_done = 1;
                printf("myrefund done\n");
            }
        }
    }
    return(atx->myrefundtx_done);
}

int32_t update_fundingtx(struct NXT_acct *np,struct subatomic_tx *atx)
{
    struct subatomic_halftx *htx = &atx->myhalf;
    if ( atx->type == SUBATOMIC_TYPE || atx->type == SUBATOMIC_FORNXT_TYPE )
    {
        if ( atx->microtx_fragi == 0 || atx->other_fundingtx_confirms == 0 )
            atx->microtx_fragi = share_fundingtx(np,&htx->funding,atx->funding_fragi);
    }
    if ( atx->type == NXTFOR_SUBATOMIC_TYPE || atx->type == SUBATOMIC_TYPE )
    {
        if ( atx->other_fundingtx_confirms == 0 )
        {
            if ( atx->other_fundingtx_waiting != 0 )
            {
                subatomic_broadcasttx(&atx->otherhalf,atx->otherhalf.funding.signedtransaction,0,0);
                atx->other_fundingtx_confirms = atx->otherhalf.minconfirms+1;
                printf("broadcast other funding\n");
            }
        }
    }
    //else atx->other_fundingtx_confirms = get_numconfirms(&atx->otherhalf);  // jl777: critical to wait for both funding tx to get confirmed
    return(atx->other_fundingtx_confirms);
}

int32_t update_otherincr(struct NXT_acct *np,struct subatomic_tx *atx)
{
    char *parsed;
    cJSON *json;
    double myshare;
    int32_t retval = 0;
    struct NXT_tx *refutx,*utx;
    if ( atx->type == ATOMICSWAP_TYPE || atx->type == SUBATOMIC_FORNXT_TYPE )
    {
        if ( atx->swap.atomictx_waiting != 0 )
        {
            printf("GOT.(%s)\n",atx->swap.signedtxbytes[1]);
            if ( (parsed = issue_parseTransaction(Global_subatomic->curl_handle,atx->swap.signedtxbytes[1])) != 0 )
            {
                json = cJSON_Parse(parsed);
                if ( json != 0 )
                {
                    refutx = set_NXT_tx(atx->swap.jsons[1]);
                    utx = set_NXT_tx(json);
                    //printf("refutx.%p utx.%p verified.%d\n",refutx,utx,utx->verify);
                    if ( utx != 0 && refutx != 0 && utx->verify != 0 )
                    {
                        myshare = (double)utx->amountNQT / refutx->amountNQT;
                        if ( NXTutxcmp(refutx,utx,myshare) == 0 )
                        {
                            retval = myshare * atx->ARGS.numincr;
                            printf("retval %d = myshare %.4f * numincr %d\n",retval,myshare,atx->ARGS.numincr);
                        } else printf("miscompare myshare %.4f %lld/%lld\n",myshare,(long long)utx->amountNQT,(long long)refutx->amountNQT);
                    }
                }
            }
        }
    }
    else if ( atx->type == NXTFOR_SUBATOMIC_TYPE || atx->type == SUBATOMIC_TYPE )
        retval = process_microtx(atx,&atx->otherhalf.micropay,atx->ARGS.incr,atx->ARGS.otherincr);
    return(retval);
}

int32_t calc_micropay(struct NXT_acct *np,struct subatomic_tx *atx)
{
    struct NXT_tx *refutx;
    char signedtx[1024];
    if ( atx->type == ATOMICSWAP_TYPE || atx->type == NXTFOR_SUBATOMIC_TYPE )
    {
        refutx = sign_NXT_tx(Global_subatomic->curl_handle,signedtx,atx->swap.mytx,atx->swap.fullhash[1],atx->ARGS.myshare);
        if ( refutx != 0 )
        {
            if ( NXTutxcmp(refutx,atx->swap.mytx,atx->ARGS.myshare) == 0 )
            {
                safecopy(atx->swap.signedtxbytes[0],signedtx,sizeof(atx->swap.signedtxbytes[0]));
            }
        }
    }
    else
    {
        if ( (atx->sent_to_other= subatomic_calc_micropay(atx,&atx->myhalf,atx->otherhalf.coinaddr,atx->ARGS.myshare,SUBATOMIC_STARTING_SEQUENCEID+atx->ARGS.incr)) > 0 )
        {
            // printf("micropay.(%s)\n",htx->micropay.signedtransaction);
            //printf("send micropay share incr.%d otherincr.%d totalfragis.%d\n",atx->ARGS.incr,atx->ARGS.otherincr,totalfragis);
            return(0);
        }
    }
    return(-1);
}

int32_t send_micropay(struct NXT_acct *np,struct subatomic_tx *atx)
{
    if ( atx->type == ATOMICSWAP_TYPE || atx->type == NXTFOR_SUBATOMIC_TYPE )
        share_atomictx(np,atx->swap.signedtxbytes[0],1);
    else share_micropaytx(np,&atx->myhalf.micropay,atx->microtx_fragi);
    return(0);
}

void update_subatomic_transfers(char *NXTaddr)
{
    static int64_t nexttime;
    struct subatomic_info *gp = Global_subatomic;
    struct subatomic_tx *atx;
    struct NXT_acct *np;
    int32_t i,j,txcreated,createdflag,retval;
    //printf("update subatomics\n");
    if ( microseconds() < nexttime )
        return;
    for (i=0; i<gp->numsubatomics; i++)
    {
        atx = gp->subatomics[i];
        if ( atx->initflag == 3 && atx->status != SUBATOMIC_COMPLETED && atx->status != SUBATOMIC_ABORTED )
        {
            np = get_NXTacct(&createdflag,Global_mp,atx->ARGS.otherNXTaddr);
            if ( (np->recvid == 0 || np->sentid == 0) && verify_peer_link(SUBATOMIC_SIG,atx->ARGS.otherNXTaddr) != 0 )
            {
                nexttime = (microseconds() + 1000000*300);
                continue;
            }
            if ( atx->type == ATOMICSWAP_TYPE )
            {
                atx->status = update_atomic(np,atx);
                continue;
            }
            txcreated = verify_txs_created(np,atx);
            if ( txcreated < 0 )
            {
                atx->status = SUBATOMIC_ABORTED;
                continue;
            }
            if ( (atx->txs_created= txcreated) == 0 )
                continue;
            update_other_refundtxdone(np,atx);
            update_my_refundtxdone(np,atx);
            if ( atx->myrefundtx_done <= 0 || atx->other_refundtx_done <= 0 )
                continue;
            update_fundingtx(np,atx);
            if ( atx->other_fundingtx_confirms-1 < atx->otherhalf.minconfirms )
                continue;
            //printf("micropay loop: share incr.%d otherincr.%d totalfragis.%d\n",atx->ARGS.incr,atx->ARGS.otherincr,totalfragis);
            if ( atx->other_micropaytx_waiting != 0 )
            {
                retval = update_otherincr(np,atx);//process_microtx(atx,&atx->otherhalf.micropay,atx->ARGS.incr,atx->ARGS.otherincr);
                if ( retval >= 0 )
                    atx->ARGS.otherincr = retval;
                atx->other_micropaytx_waiting = 0;
            }
            if ( atx->ARGS.incr < atx->ARGS.numincr && atx->ARGS.incr <= atx->ARGS.otherincr+1 )
            {
                atx->ARGS.incr++;
                if ( atx->ARGS.incr < atx->ARGS.otherincr )
                    atx->ARGS.incr = atx->ARGS.otherincr;
                atx->ARGS.myshare = ((double)atx->ARGS.numincr - atx->ARGS.incr) / atx->ARGS.numincr;
                calc_micropay(np,atx);
            }
            if ( atx->ARGS.incr == 100 )
            {
                for (j=0; j<3; j++)
                {
                    send_micropay(np,atx);
                    sleep(3);
                }
            }
            else send_micropay(np,atx);
        }
    }
    //printf("done update subatomics\n");
}

int32_t subatomic_txcmp(struct subatomic_tx *_ref,struct subatomic_tx *_atx,int32_t flipped)
{
    struct subatomic_tx_args *ref,*atx;
    if ( _atx->type == ATOMICSWAP_TYPE )
    {
        //printf("flipped.%d (%s) vs (%s) and (%s) vs (%s)\n",flipped,_ref->swap.NXTaddr,_atx->swap.NXTaddr,_ref->swap.otherNXTaddr,_atx->swap.otherNXTaddr);
        //printf("(%s) vs (%s)\n",_ref->swap.txbytes[0],_atx->swap.txbytes[0]);
        //printf("(%s) vs (%s)\n",_ref->swap.txbytes[1],_atx->swap.txbytes[1]);
        if ( flipped == 0 )
        {
            if ( strcmp(_ref->swap.NXTaddr,_atx->swap.NXTaddr) != 0 )
                return(-1);
            if ( strcmp(_ref->swap.otherNXTaddr,_atx->swap.otherNXTaddr) != 0 )
                return(-2);
            if ( strcmp(_ref->swap.txbytes[0],_atx->swap.txbytes[0]) != 0 )
                return(-3);
            if ( strcmp(_ref->swap.txbytes[1],_atx->swap.txbytes[1]) != 0 )
                return(-4);
        }
        else
        {
            if ( strcmp(_ref->swap.NXTaddr,_atx->swap.otherNXTaddr) != 0 )
                return(-5);
            if ( strcmp(_ref->swap.otherNXTaddr,_atx->swap.NXTaddr) != 0 )
                return(-6);
            if ( strcmp(_ref->swap.txbytes[0],_atx->swap.txbytes[0]) != 0 )
                return(-7);
            if ( strcmp(_ref->swap.txbytes[1],_atx->swap.txbytes[1]) != 0 )
                return(-8);
        }
        printf("MATCHED!\n");
        return(0);
    }
    ref = &_ref->ARGS; atx = &_atx->ARGS;
    printf("%p.(%s <-> %s) vs %p.(%s <-> %s)\n",ref,ref->NXTaddr,ref->otherNXTaddr,atx, atx->NXTaddr,atx->otherNXTaddr);
    if ( flipped != 0 )
    {
        if ( strcmp(ref->NXTaddr,atx->otherNXTaddr) != 0 )
        {
            printf("%s != %s\n",ref->NXTaddr,atx->otherNXTaddr);
            return(-1);
        }
        if ( strcmp(ref->otherNXTaddr,atx->NXTaddr) != 0 )
            return(-2);
    }
    else
    {
        if ( strcmp(ref->NXTaddr,atx->NXTaddr) != 0 )
        {
            printf("%s != %s\n",ref->NXTaddr,atx->NXTaddr);
            return(-1);
        }
        if ( strcmp(ref->otherNXTaddr,atx->otherNXTaddr) != 0 )
            return(-2);
    }
    if ( flipped == 0 )
    {
        if ( ref->coinid != atx->coinid )
        {
            printf("%s != %s\n",coinid_str(ref->coinid),coinid_str(atx->coinid));
            return(-3);
        }
        if ( ref->destcoinid != atx->destcoinid )
            return(-4);
        if ( ref->destamount != atx->destamount )
            return(-5);
        if ( ref->amount != atx->amount )
            return(-6);
    }
    else
    {
        if ( ref->coinid != atx->destcoinid )
        {
            printf("%s != %s\n",coinid_str(ref->coinid),coinid_str(atx->destcoinid));
            return(-13);
        }
        if ( ref->destcoinid != atx->coinid )
            return(-14);
        if ( ref->destamount != atx->amount )
            return(-15);
        if ( ref->amount != atx->destamount )
            return(-16);
    }
    return(0);
}

int32_t set_atomic_swap(struct atomic_swap *sp,char *NXTaddr,char *mytxbytes,char *otherNXTaddr,char *othertxbytes,char *otheripaddr,int32_t flipped,char *mysighash,char *othersighash)
{
    int i,j;
    char *retstr,*padded;
    memset(sp,0,sizeof(*sp));
    for (i=0; i<2; i++)
    {
        safecopy(sp->txbytes[i],i==flipped?mytxbytes:othertxbytes,sizeof(*sp->txbytes));
        safecopy(sp->sighash[i],i==flipped?mysighash:othersighash,sizeof(*sp->sighash));
        padded = malloc(strlen(sp->txbytes[i]) + 129);
        strcpy(padded,sp->txbytes[i]);
        for (j=0; j<128; j++)
            strcat(padded,"0");
        if ( (sp->parsed[i] = issue_parseTransaction(Global_subatomic->curl_handle,padded)) != 0 )
        {
            sp->jsons[i] = cJSON_Parse(sp->parsed[i]);
            if ( sp->jsons[i] == 0 )
                return(-1);
        }
        retstr = issue_calculateFullHash(Global_subatomic->curl_handle,padded,sp->sighash[i]);
        if ( retstr != 0 )
            safecopy(sp->fullhash[i],retstr,sizeof(*sp->fullhash));
        free(padded);
    }
    safecopy(sp->NXTaddr,NXTaddr,sizeof(sp->NXTaddr));
    safecopy(sp->otherNXTaddr,otherNXTaddr,sizeof(sp->otherNXTaddr));
    if ( flipped != 0 )
        safecopy(sp->otheripaddr,otheripaddr,sizeof(sp->otheripaddr));
    printf("flipped.%d ipaddr.%s %s %s -> %s %s\n",flipped,sp->otheripaddr,sp->NXTaddr,sp->parsed[0]!=0?sp->parsed[0]:"",sp->otherNXTaddr,sp->parsed[1]!=0?sp->parsed[1]:"");
    return(0);
}

int32_t set_subatomic_trade(int type,struct subatomic_tx *_atx,char *NXTaddr,char *coin,char *amountstr,char *coinaddr,char *otherNXTaddr,char *destcoin,char *destamountstr,char *destcoinaddr,char *otheripaddr,int32_t flipped)
{
    struct subatomic_tx_args *atx = &_atx->ARGS;
    if ( type == ATOMICSWAP_TYPE )
    {
        _atx->type = type;
        return(set_atomic_swap(&_atx->swap,NXTaddr,coinaddr,otherNXTaddr,destcoinaddr,otheripaddr,flipped,amountstr,destamountstr));
    }
    memset(atx,0,sizeof(*atx));
    atx->coinid = conv_coinstr(coin);
    atx->destcoinid = conv_coinstr(destcoin);
    atx->amount = conv_floatstr(amountstr);
    atx->destamount = conv_floatstr(destamountstr);
    safecopy(atx->coinaddr[flipped],coinaddr,sizeof(atx->coinaddr[flipped]));
    safecopy(atx->coinaddr[flipped^1],destcoinaddr,sizeof(atx->coinaddr[flipped^1]));
    safecopy(atx->NXTaddr,NXTaddr,sizeof(atx->NXTaddr));
    safecopy(atx->otherNXTaddr,otherNXTaddr,sizeof(atx->otherNXTaddr));
    safecopy(atx->destcoinaddr[flipped],destcoinaddr,sizeof(atx->destcoinaddr[flipped]));
    safecopy(atx->destcoinaddr[flipped^1],coinaddr,sizeof(atx->destcoinaddr[flipped^1]));
    if ( flipped != 0 )
        safecopy(atx->otheripaddr,otheripaddr,sizeof(atx->otheripaddr));
    printf("flipped.%d ipaddr.%s %d %d -> %s %s\n",flipped,atx->otheripaddr,atx->coinid,atx->destcoinid,coin,destcoin);
    if ( atx->amount != 0 && atx->destamount != 0 && atx->coinid >= 0 && atx->destcoinid >= 0 )
        return(0);
    printf("error setting subatomic trade %lld %lld %d %d\n",(long long)atx->amount,(long long)atx->destamount,atx->coinid,atx->destcoinid);
    return(-1);
}

// this function needs to invert everything to the point of view of this acct
int32_t set_subatomic_argstrs(int32_t *flippedp,char *sender,char *receiver,cJSON **objs,char *amountstr,char *destamountstr,char *NXTaddr,char *coin,char *destcoin,char *coinaddr,char *destcoinaddr,char *otherNXTaddr,char *senderipaddr)
{
    char type[64];
    *flippedp = 0;
    copy_cJSON(NXTaddr,objs[0]);
    copy_cJSON(coin,objs[1]);
    copy_cJSON(amountstr,objs[2]);
    copy_cJSON(coinaddr,objs[3]);
    
    copy_cJSON(otherNXTaddr,objs[4]);
    copy_cJSON(destcoin,objs[5]);
    copy_cJSON(destamountstr,objs[6]);
    copy_cJSON(destcoinaddr,objs[7]);
    copy_cJSON(senderipaddr,objs[8]);
    copy_cJSON(type,objs[9]);
    if ( strcmp(coin,"NXT") == 0 && strcmp(destcoin,"NXT") == 0 )
        strcpy(type,"NXTatomicswap");
    if ( strcmp(type,"NXTatomicswap") == 0 )
    {
        if ( coinaddr[0] == 0 && objs[10] != 0 )
            copy_cJSON(coinaddr,objs[10]);
        if ( destcoinaddr[0] == 0 && objs[11] != 0 )
            copy_cJSON(destcoinaddr,objs[11]);
        if ( amountstr[0] == 0 && objs[12] != 0 )
            copy_cJSON(amountstr,objs[12]);
        if ( destamountstr[0] == 0 && objs[13] != 0 )
            copy_cJSON(destamountstr,objs[13]);
    }
    //printf("type (%s)\n",type);
    if ( strcmp(otherNXTaddr,Global_subatomic->NXTADDR) == 0 )
        *flippedp = 1;
    //printf("NXT.(%s) (%s) <-> NXT.(%s) (%s)\n",NXTaddr,coinaddr,otherNXTaddr,destcoinaddr);
    if ( strcmp(type,"NXTatomicswap") != 0 && NXTaddr[0] != 0 && coin[0] != 0 && amountstr[0] != 0 && coinaddr[0] != 0 && otherNXTaddr[0] != 0 && destcoin[0] != 0 && destamountstr[0] != 0 && destcoinaddr[0] != 0 )
    {
        //*amountp = conv_floatstr(amountstr);
        //*destamountp = conv_floatstr(destamountstr);
        //if ( *amountp == 0 || *destamountp == 0 )
        //    return(-1);
        return(0);
    }
    else if ( strcmp(type,"NXTatomicswap") == 0 && otherNXTaddr[0] != 0 && NXTaddr[0] != 0 && coinaddr[0] != 0 && destcoinaddr[0] != 0 && amountstr[0] != 0 && destamountstr[0] != 0 )
    {
        //printf("NXT Atomic swap %s.(%s).%s <-> %s.(%s).%s\n",NXTaddr,coinaddr,amountstr,otherNXTaddr,destcoinaddr,destamountstr);
        return(1);
    }
    printf("something is not right: flipped.%d %d %d %d %d %d %d %d %d | sender.%s gp.%s\n",*flippedp,NXTaddr[0],coin[0],amountstr[0],coinaddr[0],otherNXTaddr[0],destcoin[0],destamountstr[0],destcoinaddr[0],sender,Global_subatomic->NXTADDR);
    return(-1);
}

int32_t decode_subatomic_json(struct subatomic_tx *atx,cJSON *json,char *sender,char *receiver)
{
    char amountstr[128],destamountstr[128],NXTaddr[64],coin[64],destcoin[64],coinaddr[MAX_NXT_TXBYTES],destcoinaddr[MAX_NXT_TXBYTES],otherNXTaddr[64],otheripaddr[32];
    cJSON *objs[16];
    int32_t type,flipped = 0;
    objs[0] = cJSON_GetObjectItem(json,"NXT");
    objs[1] = cJSON_GetObjectItem(json,"coin");
    objs[2] = cJSON_GetObjectItem(json,"amount");
    objs[3] = cJSON_GetObjectItem(json,"coinaddr");
    objs[4] = cJSON_GetObjectItem(json,"destNXT");
    objs[5] = cJSON_GetObjectItem(json,"destcoin");
    objs[6] = cJSON_GetObjectItem(json,"destamount");
    objs[7] = cJSON_GetObjectItem(json,"destcoinaddr");
    objs[8] = cJSON_GetObjectItem(json,"senderip");
    objs[9] = cJSON_GetObjectItem(json,"type");
    objs[10] = cJSON_GetObjectItem(json,"mytxbytes");
    objs[11] = cJSON_GetObjectItem(json,"othertxbytes");
    objs[12] = cJSON_GetObjectItem(json,"mysighash");
    objs[13] = cJSON_GetObjectItem(json,"othersighash");
    if ( (type= set_subatomic_argstrs(&flipped,sender,receiver,objs,amountstr,destamountstr,NXTaddr,coin,destcoin,coinaddr,destcoinaddr,otherNXTaddr,otheripaddr)) >= 0 )
    {
        if ( set_subatomic_trade(type,atx,NXTaddr,coin,amountstr,coinaddr,otherNXTaddr,destcoin,destamountstr,destcoinaddr,otheripaddr,flipped) == 0 )
            return(flipped);
    }
    return(-1);
}

void update_subatomic_state(struct subatomic_info *gp,int32_t funcid,int32_t rating,cJSON *argjson,uint64_t nxt64bits,char *sender,char *receiver)  // the only path into the subatomics[], eg. via AM
{
    //int32_t decode_subatomic_json(struct subatomic_tx *atx,cJSON *json,char *sender,char *receiver);
    int32_t i,flipped,cmpval;
    struct subatomic_tx *atx,T;
    memset(&T,0,sizeof(T));
    //printf("parse subatomic\n");
    if ( (flipped= decode_subatomic_json(&T,argjson,sender,receiver)) >= 0 )
    {
        //printf("NXT.%s (%s %s %s %s) <-> %s (%s %s %s %s)\n",T.NXTaddr,coinid_str(T.coinid),T.coinaddr[0],coinid_str(T.destcoinid),T.destcoinaddr[0],T.otherNXTaddr,coinid_str(T.coinid),T.coinaddr[1],coinid_str(T.destcoinid),T.destcoinaddr[1]);
        atx = 0;
        for (i=0; i<gp->numsubatomics; i++)
        {
            atx = gp->subatomics[i];
            if ( (cmpval= subatomic_txcmp(atx,&T,flipped)) == 0 )
            {
                printf("%d: cmpval.%d vs %p\n",i,cmpval,atx);
                if ( T.type == SUBATOMIC_TYPE )
                {
                    strcpy(atx->ARGS.coinaddr[flipped],T.ARGS.coinaddr[flipped]);
                    strcpy(atx->ARGS.destcoinaddr[flipped],T.ARGS.destcoinaddr[flipped]);
                    if ( flipped != 0 )
                        strcpy(atx->ARGS.otheripaddr,T.ARGS.otheripaddr);
                }
                else
                {
                    if ( flipped != 0 )
                    {
                        strcpy(atx->ARGS.otheripaddr,T.swap.otheripaddr);
                        strcpy(atx->ARGS.NXTaddr,T.swap.otherNXTaddr);
                        strcpy(atx->ARGS.otherNXTaddr,T.swap.NXTaddr);
                    }
                    else
                    {
                        strcpy(atx->ARGS.NXTaddr,T.swap.NXTaddr);
                        strcpy(atx->ARGS.otherNXTaddr,T.swap.otherNXTaddr);
                    }
                }
                break;
            }
        }
        if ( i == gp->numsubatomics )
        {
            atx = malloc(sizeof(T));
            *atx = T;
            if ( T.type == SUBATOMIC_TYPE )
            {
                strcpy(atx->ARGS.coinaddr[flipped],T.ARGS.coinaddr[flipped]);
                strcpy(atx->ARGS.destcoinaddr[flipped],T.ARGS.destcoinaddr[flipped]);
                if ( flipped != 0 )
                {
                    strcpy(atx->ARGS.otheripaddr,T.ARGS.otheripaddr);
                    strcpy(atx->ARGS.NXTaddr,T.ARGS.otherNXTaddr);
                    strcpy(atx->ARGS.otherNXTaddr,T.ARGS.NXTaddr);
                    atx->ARGS.coinid = T.ARGS.destcoinid;
                    atx->ARGS.amount = T.ARGS.destamount;
                    atx->ARGS.destcoinid = T.ARGS.coinid;
                    atx->ARGS.destamount = T.ARGS.amount;
                }
                else
                {
                    atx->ARGS.coinid = T.ARGS.coinid;
                    atx->ARGS.amount = T.ARGS.amount;
                    atx->ARGS.destcoinid = T.ARGS.destcoinid;
                    atx->ARGS.destamount = T.ARGS.destamount;
                }
            }
            else
            {
                if ( flipped != 0 )
                    strcpy(atx->ARGS.otheripaddr,T.swap.otheripaddr);
                strcpy(atx->ARGS.NXTaddr,T.swap.NXTaddr);
                strcpy(atx->ARGS.otherNXTaddr,T.swap.otherNXTaddr);
            }
            printf("alloc type.%d new %p atx.%d (%s <-> %s)\n",T.type,atx,gp->numsubatomics,atx->ARGS.NXTaddr,atx->ARGS.otherNXTaddr);
            gp->subatomics = realloc(gp->subatomics,(gp->numsubatomics + 1) * sizeof(*gp->subatomics));
            gp->subatomics[gp->numsubatomics] = atx, atx->tag = gp->numsubatomics;
            gp->numsubatomics++;
        }
        atx->initflag |= init_subatomic_tx(atx,flipped,T.type);
        printf("got trade! flipped.%d | initflag.%d\n",flipped,atx->initflag);
        if ( atx->initflag == 3 )
        {
            printf("PENDING SUBATOMIC TRADE from %s %s %.8f <-> %s %s %.8f\n",atx->ARGS.NXTaddr,coinid_str(atx->ARGS.coinid),dstr(atx->ARGS.amount),atx->ARGS.otherNXTaddr,coinid_str(atx->ARGS.destcoinid),dstr(atx->ARGS.destamount));
        }
    }
}

char *AM_subatomic(int32_t func,int32_t rating,char *destaddr,char *senderNXTaddr,char *jsontxt)
{
    char AM[4096],*retstr;
    struct json_AM *ap = (struct json_AM *)AM;
    stripwhite(jsontxt,strlen(jsontxt));
    printf("sender.%s func.(%c) AM_subatomic(%s) -> NXT.(%s) rating.%d\n",senderNXTaddr,func,jsontxt,destaddr,rating);
    set_json_AM(ap,SUBATOMIC_SIG,func,senderNXTaddr,rating,jsontxt,1);
    retstr = submit_AM(Global_subatomic->curl_handle,destaddr,&ap->H,0);
    printf("AM_subatomic.(%s)\n",retstr);
    return(retstr);
}

cJSON *gen_atomicswap_json(int AMflag,struct atomic_swap *sp,char *senderip)
{
    cJSON *json;
    json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"type",cJSON_CreateString("NXTatomicswap"));
    if ( sp->NXTaddr[0] != 0 )
        cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(sp->NXTaddr));
    if ( sp->parsed[0] != 0 )
    {
        if ( AMflag == 0 )
        {
            if ( sp->signedtxbytes[0] != 0 )
                cJSON_AddItemToObject(json,"signedtxbytes",cJSON_CreateString(sp->signedtxbytes[0]));
            if ( sp->fullhash[0] != 0 )
                cJSON_AddItemToObject(json,"myfullhash",cJSON_CreateString(sp->fullhash[0]));
            if ( sp->jsons != 0 )
                cJSON_AddItemToObject(json,"myTX",sp->jsons[0]);
        }
        if ( sp->txbytes[0] != 0 )
            cJSON_AddItemToObject(json,"mytxbytes",cJSON_CreateString(sp->txbytes[0]));
        if ( sp->sighash[0] != 0 )
            cJSON_AddItemToObject(json,"mysighash",cJSON_CreateString(sp->sighash[0]));
    }
    if ( sp->otherNXTaddr[0] != 0 )
        cJSON_AddItemToObject(json,"destNXT",cJSON_CreateString(sp->otherNXTaddr));
    if ( sp->otheripaddr[0] != 0 )
        cJSON_AddItemToObject(json,"otheripaddr",cJSON_CreateString(sp->otheripaddr));
    if ( sp->parsed[0] != 0 )
    {
        if ( sp->txbytes[1] != 0 )
            cJSON_AddItemToObject(json,"othertxbytes",cJSON_CreateString(sp->txbytes[1]));
        if ( sp->sighash[1] != 0 )
            cJSON_AddItemToObject(json,"othersighash",cJSON_CreateString(sp->sighash[1]));
        if ( AMflag == 0 )
        {
            if ( sp->signedtxbytes[1] != 0 )
                cJSON_AddItemToObject(json,"othersignedtxbytes",cJSON_CreateString(sp->signedtxbytes[1]));
            if ( sp->fullhash[1] != 0 )
                cJSON_AddItemToObject(json,"otherfullhash",cJSON_CreateString(sp->fullhash[1]));
            if ( sp->jsons[1] != 0 )
                cJSON_AddItemToObject(json,"otherTX",sp->jsons[1]);
        }
    }
    
    return(json);
}

cJSON *gen_subatomic_json(int AMflag,int type,struct subatomic_tx *_atx,char *senderip)
{
    struct subatomic_tx_args *atx = &_atx->ARGS;
    struct subatomic_info *gp = Global_subatomic;
    char numstr[512];
    cJSON *json;
    json = cJSON_CreateObject();
    if ( type == ATOMICSWAP_TYPE )
        return(gen_atomicswap_json(AMflag,&_atx->swap,senderip));
    cJSON_AddItemToObject(json,"type",cJSON_CreateString(type==0?"subatomic_crypto":"NXTatomicswap"));
    if ( AMflag == 0 )
    {
        cJSON_AddItemToObject(json,"type",cJSON_CreateString(type==0?"subatomic_crypto":"NXTatomicswap"));
        if ( _atx->myexpectedamount != 0 )
            cJSON_AddItemToObject(json,"completed",cJSON_CreateNumber(dstr((double)_atx->myreceived/_atx->myexpectedamount)));
        cJSON_AddItemToObject(json,"received",cJSON_CreateNumber(dstr(_atx->myreceived)));
        cJSON_AddItemToObject(json,"expected",cJSON_CreateNumber(dstr(_atx->myexpectedamount)));
        cJSON_AddItemToObject(json,"sent",cJSON_CreateNumber(dstr(_atx->sent_to_other)));
        cJSON_AddItemToObject(json,"sending",cJSON_CreateNumber(dstr(_atx->otherexpectedamount)));
        if ( senderip == 0 && gp->ipaddr[0] != 0 )
            cJSON_AddItemToObject(json,"ipaddr",cJSON_CreateString(gp->ipaddr));
        if ( atx->coinaddr[1][0] != 0 )
            cJSON_AddItemToObject(json,"destNXTcoinaddr",cJSON_CreateString(atx->coinaddr[1]));
        if ( atx->destcoinaddr[1][0] != 0 )
            cJSON_AddItemToObject(json,"destNXTdestcoinaddr",cJSON_CreateString(atx->destcoinaddr[1]));
        if ( senderip == 0 && atx->otheripaddr[0] != 0 )
            cJSON_AddItemToObject(json,"otherip",cJSON_CreateString(atx->otheripaddr));
    }
    else cJSON_AddItemToObject(json,"senderip",cJSON_CreateString(Global_mp->ipaddr));
    if ( gp->NXTADDR[0] != 0 )
        cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(gp->NXTADDR));
    cJSON_AddItemToObject(json,"coin",cJSON_CreateString(coinid_str(atx->coinid)));
    sprintf(numstr,"%.8f",dstr(atx->amount)); cJSON_AddItemToObject(json,"amount",cJSON_CreateString(numstr));
    if ( atx->coinaddr[0][0] != 0 )
        cJSON_AddItemToObject(json,"coinaddr",cJSON_CreateString(atx->coinaddr[0]));
    cJSON_AddItemToObject(json,"destcoin",cJSON_CreateString(coinid_str(atx->destcoinid)));
    sprintf(numstr,"%.8f",dstr(atx->destamount)); cJSON_AddItemToObject(json,"destamount",cJSON_CreateString(numstr));
    if ( atx->destcoinaddr[0][0] != 0 )
        cJSON_AddItemToObject(json,"destcoinaddr",cJSON_CreateString(atx->destcoinaddr[0]));
    
    if ( atx->otherNXTaddr[0] != 0 )
        cJSON_AddItemToObject(json,"destNXT",cJSON_CreateString(atx->otherNXTaddr));
    
    return(json);
}

char *subatomic_cancel_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs)
{
    char *retstr = 0;
    retstr = clonestr("{\"result\":\"subatomic cancel trade not supported yet\"}");
    return(retstr);
}

char *subatomic_status_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs)
{
    struct subatomic_info *gp = Global_subatomic;
    int32_t i;
    cJSON *item,*array;
    char *retstr = 0;
    printf("do status\n");
    if ( gp->numsubatomics == 0 )
        retstr = clonestr("{\"result\":\"subatomic no trades pending\"}");
    else
    {
        array = cJSON_CreateArray();
        for (i=0; i<gp->numsubatomics; i++)
        {
            item = gen_subatomic_json(0,gp->subatomics[i]->type,gp->subatomics[i],0);
            if ( item != 0 )
                cJSON_AddItemToArray(array,item);
        }
        retstr = cJSON_Print(array);
        free_json(array);
    }
    return(retstr);
}

char *subatomic_trade_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs)
{
    char NXTaddr[64],amountstr[128],destamountstr[128],otheripaddr[32],coin[64],destcoin[64],coinaddr[MAX_NXT_TXBYTES],destcoinaddr[MAX_NXT_TXBYTES],otherNXTaddr[64],*retstr = 0;
    cJSON *json;
    int32_t flipped,type;
    struct subatomic_tx T;
    char *jsontxt,*AMtxid,buf[4096];
    if ( (type= set_subatomic_argstrs(&flipped,sender,0,objs,amountstr,destamountstr,NXTaddr,coin,destcoin,coinaddr,destcoinaddr,otherNXTaddr,otheripaddr)) >= 0 )
    {
        if ( set_subatomic_trade(type,&T,NXTaddr,coin,amountstr,coinaddr,otherNXTaddr,destcoin,destamountstr,destcoinaddr,otheripaddr,flipped) == 0 )
        {
            json = gen_subatomic_json(1,type,&T,otheripaddr[0]!=0?otheripaddr:0);
            if ( json != 0 )
            {
                jsontxt = cJSON_Print(json);
                AMtxid = AM_subatomic(SUBATOMIC_TRADEFUNC,0,otherNXTaddr,NXTaddr,jsontxt);
                if ( AMtxid != 0 )
                {
                    sprintf(buf,"{\"result\":\"good\",\"AMtxid\":\"%s\",\"trade\":%s}",AMtxid,jsontxt);
                    retstr = clonestr(buf);
                    free(AMtxid);
                }
                else
                {
                    sprintf(buf,"{\"result\":\"error\",\"AMtxid\":\"0\",\"trade\":%s}",jsontxt);
                    retstr = clonestr(buf);
                }
                free(jsontxt);
                free_json(json);
            }
            else retstr = clonestr("{\"result\":\"error generating json\"}");
        }
        else retstr = clonestr("{\"result\":\"error initializing subatomic trade\"}");
    }
    else retstr = clonestr("{\"result\":\"invalid trade args\"}");
    return(retstr);
}

char *subatomic_json_commands(struct subatomic_info *gp,cJSON *argjson,char *sender,int32_t valid)
{
    static char *subatomic_status[] = { (char *)subatomic_status_func, "subatomic_status", "", "NXT", 0 };
    static char *subatomic_cancel[] = { (char *)subatomic_cancel_func, "subatomic_cancel", "", "NXT", 0 };
    static char *subatomic_trade[] = { (char *)subatomic_trade_func, "subatomic_trade", "", "NXT", "coin", "amount", "coinaddr", "destNXT", "destcoin", "destamount", "destcoinaddr", "senderip", "type", "mytxbytes", "othertxbytes", "mysighash", "othersighash", 0 };
    static char **commands[] = { subatomic_status, subatomic_cancel, subatomic_trade };
    int32_t i,j;
    cJSON *obj,*nxtobj,*objs[16];
    char NXTaddr[64],command[4096],**cmdinfo;
    printf("subatomic_json_commands\n");
    memset(objs,0,sizeof(objs));
    command[0] = 0;
    memset(NXTaddr,0,sizeof(NXTaddr));
    if ( argjson != 0 )
    {
        obj = cJSON_GetObjectItem(argjson,"requestType");
        nxtobj = cJSON_GetObjectItem(argjson,"NXT");
        copy_cJSON(NXTaddr,nxtobj);
        copy_cJSON(command,obj);
        //printf("(%s) command.(%s) NXT.(%s)\n",cJSON_Print(argjson),command,NXTaddr);
    }
    //printf("multigateway_json_commands sender.(%s) valid.%d\n",sender,valid);
    for (i=0; i<(int32_t)(sizeof(commands)/sizeof(*commands)); i++)
    {
        cmdinfo = commands[i];
        //printf("needvalid.(%c) sender.(%s) valid.%d %d of %d: cmd.(%s) vs command.(%s)\n",cmdinfo[2][0],sender,valid,i,(int32_t)(sizeof(commands)/sizeof(*commands)),cmdinfo[1],command);
        if ( strcmp(cmdinfo[1],command) == 0 )
        {
            if ( cmdinfo[2][0] != 0 )
            {
                if ( sender[0] == 0 || valid != 1 || strcmp(NXTaddr,sender) != 0 )
                {
                    printf("verification valid.%d missing for %s sender.(%s) vs NXT.(%s)\n",valid,cmdinfo[1],NXTaddr,sender);
                    return(0);
                }
            }
            for (j=3; cmdinfo[j]!=0&&j<3+(int32_t)(sizeof(objs)/sizeof(*objs)); j++)
                objs[j-3] = cJSON_GetObjectItem(argjson,cmdinfo[j]);
            return((*(json_handler)cmdinfo[0])(sender,valid,objs,j-3));
        }
    }
    return(0);
}

char *subatomic_jsonhandler(cJSON *argjson)
{
    struct subatomic_info *gp = Global_subatomic;
    long len;
    int32_t valid;
    cJSON *json,*obj,*parmsobj,*tokenobj,*secondobj;
    char sender[64],*parmstxt,encoded[NXT_TOKEN_LEN],*retstr = 0;
    sender[0] = 0;
    valid = -1;
    printf("subatomic_jsonhandler argjson.%p\n",argjson);
    if ( argjson == 0 )
    {
        json = cJSON_CreateObject();
        obj = cJSON_CreateString(gp->NXTADDR); cJSON_AddItemToObject(json,"NXTaddr",obj);
        obj = cJSON_CreateString(gp->ipaddr); cJSON_AddItemToObject(json,"ipaddr",obj);
        
        obj = gen_NXTaccts_json(0);
        if ( obj != 0 )
            cJSON_AddItemToObject(json,"NXTaccts",obj);
        retstr = cJSON_Print(json);
        free_json(json);
        return(retstr);
    }
    else if ( (argjson->type&0xff) == cJSON_Array && cJSON_GetArraySize(argjson) == 2 )
    {
        parmsobj = cJSON_GetArrayItem(argjson,0);
        secondobj = cJSON_GetArrayItem(argjson,1);
        tokenobj = cJSON_GetObjectItem(secondobj,"token");
        copy_cJSON(encoded,tokenobj);
        parmstxt = cJSON_Print(parmsobj);
        len = strlen(parmstxt);
        stripwhite(parmstxt,len);
        printf("website.(%s) encoded.(%s) len.%ld\n",parmstxt,encoded,strlen(encoded));
        if ( strlen(encoded) == NXT_TOKEN_LEN )
            issue_decodeToken(Global_subatomic->curl_handle2,sender,&valid,parmstxt,encoded);
        free(parmstxt);
        argjson = parmsobj;
    }
    if ( sender[0] == 0 )
        strcpy(sender,gp->NXTADDR);
    retstr = subatomic_json_commands(gp,argjson,sender,valid);
    return(retstr);
}

void process_subatomic_AM(struct subatomic_info *dp,struct NXT_protocol_parms *parms)
{
    cJSON *argjson;
    //char *jsontxt;
    struct json_AM *ap;
    char *sender,*receiver;
    sender = parms->sender; receiver = parms->receiver; ap = parms->AMptr; //txid = parms->txid;
    if ( (argjson = parse_json_AM(ap)) != 0 && (strcmp(dp->NXTADDR,sender) == 0 || strcmp(dp->NXTADDR,receiver) == 0) )
    {
        printf("process_subatomic_AM got jsontxt.(%s)\n",ap->jsonstr);
        update_subatomic_state(dp,ap->funcid,ap->timestamp,argjson,ap->H.nxt64bits,sender,receiver);
        free_json(argjson);
    }
}

void process_subatomic_typematch(struct subatomic_info *dp,struct NXT_protocol_parms *parms)
{
    char NXTaddr[64],*sender,*receiver,*txid;
    sender = parms->sender; receiver = parms->receiver; txid = parms->txid;
    safecopy(NXTaddr,sender,sizeof(NXTaddr));
    printf("got txid.(%s) type.%d subtype.%d sender.(%s) -> (%s)\n",txid,parms->type,parms->subtype,sender,receiver);
}

void *subatomic_handler(struct NXThandler_info *mp,struct NXT_protocol_parms *parms,void *handlerdata,int32_t height)
{
    struct subatomic_info *gp = handlerdata;
    //static int32_t variant;
    cJSON *obj;
    char buf[512];
    if ( parms->txid == 0 )     // indicates non-transaction event
    {
        if ( parms->mode == NXTPROTOCOL_WEBJSON )
            return(subatomic_jsonhandler(parms->argjson));
        else if ( parms->mode == NXTPROTOCOL_NEWBLOCK )
        {
            //printf("subatomic new RTblock %d time %lld microseconds %ld\n",mp->RTflag,time(0),microseconds());
        }
        else if ( parms->mode == NXTPROTOCOL_IDLETIME )
        {
            update_subatomic_transfers(gp->NXTADDR);
            static int testhack;
            if ( testhack == 0 )
            {
#ifndef __APPLE__
                //char *teststr = "{\"requestType\":\"subatomic_trade\",\"NXT\":\"423766016895692955\",\"coin\":\"DOGE\",\"amount\":\"100\",\"coinaddr\":\"DNbAcP82bpd9xdXNA1Vtf1Vo6yqP1rZvcu\",\"senderip\":\"209.126.71.170\",\"destNXT\":\"8989816935121514892\",\"destcoin\":\"LTC\",\"destamount\":\".1\",\"destcoinaddr\":\"LLedxvb1e5aCYmQfn8PHEPFR56AsbGgbUG\"}";
                Signedtx = "00002724c6009a024e5bbad625df3d536fa90b1e6a28c3f5a56e1fcbe34132391c8d3fd7f671cb198c71b555df3ec27c00e1f5050000000000e1f50500000000000000000000000000000000000000000000000000000000000000000000000058f36933200b9766566c7c3a9250f4e7607f75011bda5b3ba40c9f820ae3a60f1471b68e203bf9897b147aced9938cfb91bdf3230a02637264e25bff85ce382a";
                char *teststr = "{\"requestType\":\"subatomic_trade\",\"type\":\"NXTatomicswap\",\"NXT\":\"423766016895692955\",\"mytxbytes\":\"00002724c6009a024e5bbad625df3d536fa90b1e6a28c3f5a56e1fcbe34132391c8d3fd7f671cb198c71b555df3ec27c00e1f5050000000000e1f505000000000000000000000000000000000000000000000000000000000000000000000000\",\"mysighash\":\"422bf4f0a389398589553995f4dcb0b2a0811641ffb7e8222ce3dc3272628065\",\"destNXT\":\"8989816935121514892\",\"othertxbytes\":\"0000be25c6009a0225c5fed2690701cf06f267e7c227b1a3c0dfa9c6fc3cdb593b3af6f16d65302f9b30f378f284e10500e1f5050000000000e1f505000000000000000000000000000000000000000000000000000000000000000000000000\",\"othersighash\":\"8e72bba1bdf481bd6c03e61bceb53dfa642be04cb71f23755b5fa939b24ffa55\"}";
#else
                //char *teststr = "{\"requestType\":\"subatomic_trade\",\"NXT\":\"8989816935121514892\",\"coin\":\"LTC\",\"amount\":\".1\",\"coinaddr\":\"LRQ3ZyrZDhvKFkcbmC6cLutvJfPbxnYUep\",\"senderip\":\"181.112.79.236\",\"destNXT\":\"423766016895692955\",\"destcoin\":\"DOGE\",\"destamount\":\"100\",\"destcoinaddr\":\"DNyEii2mnvVL1BM1kTZkmKDcf8SFyqAX4Z\"}";
                Signedtx = "0000be25c6009a0225c5fed2690701cf06f267e7c227b1a3c0dfa9c6fc3cdb593b3af6f16d65302f9b30f378f284e10500e1f5050000000000e1f50500000000000000000000000000000000000000000000000000000000000000000000000044b7df75da4a7132c3ee64bd00ad1b56f0e3be56dfefd48e7d56679dc2b857010cd7bf789e290e2c8f4c20653ccb273dce3f248d25fb44ae802289a2c95e3bbf";
                char *teststr = "{\"requestType\":\"subatomic_trade\",\"type\":\"NXTatomicswap\",\"NXT\":\"8989816935121514892\",\"mysighash\":\"8e72bba1bdf481bd6c03e61bceb53dfa642be04cb71f23755b5fa939b24ffa55\",\"mytxbytes\":\"0000be25c6009a0225c5fed2690701cf06f267e7c227b1a3c0dfa9c6fc3cdb593b3af6f16d65302f9b30f378f284e10500e1f5050000000000e1f505000000000000000000000000000000000000000000000000000000000000000000000000\",\"destNXT\":\"423766016895692955\",\"othersighash\":\"422bf4f0a389398589553995f4dcb0b2a0811641ffb7e8222ce3dc3272628065\",\"othertxbytes\":\"00002724c6009a024e5bbad625df3d536fa90b1e6a28c3f5a56e1fcbe34132391c8d3fd7f671cb198c71b555df3ec27c00e1f5050000000000e1f505000000000000000000000000000000000000000000000000000000000000000000000000\"}";
#endif
                cJSON *json = 0;
                if ( 0 && teststr != 0 )
                    json = cJSON_Parse(teststr);
                if ( json != 0 )
                {
                    subatomic_jsonhandler(json);
                    printf("HACK %s\n",cJSON_Print(json));
                    free_json(json);
                } else if ( 0 && teststr != 0 ) printf("error parsing.(%s)\n",teststr);
            }
            // printf("subatomic.%d new idletime %d time %ld microseconds %lld \n",testhack,mp->RTflag,time(0),(long long)microseconds());
            testhack++;
        }
        else if ( parms->mode == NXTPROTOCOL_INIT )
        {
            printf("subatomic NXThandler_info init %d\n",mp->RTflag);
            gp = Global_subatomic = calloc(1,sizeof(*Global_subatomic));
            gp->curl_handle = curl_easy_init();
            gp->curl_handle2 = curl_easy_init();
            strcpy(gp->ipaddr,mp->ipaddr);
            strcpy(gp->NXTADDR,mp->NXTADDR);
            strcpy(gp->NXTACCTSECRET,mp->NXTACCTSECRET);
            if ( mp->accountjson != 0 )
            {
                obj = cJSON_GetObjectItem(mp->accountjson,"enable_bitcoin_broadcast");
                copy_cJSON(buf,obj);
                gp->enable_bitcoin_broadcast = myatoi(buf,1);
                printf("enable_bitcoin_broadcast set to %d\n",gp->enable_bitcoin_broadcast);
            }
            //portnum = SUBATOMIC_PORTNUM;
            // if ( portable_thread_create(subatomic_server,&portnum) == 0 )
            //     printf("ERROR _server_loop\n");
            //variant = SUBATOMIC_VARIANT;
            /*register_variant_handler(SUBATOMIC_VARIANT,process_subatomic_packet,SUBATOMIC_SEND_PUBKEY,sizeof(struct subatomic_packet),sizeof(struct subatomic_packet),0);
             register_variant_handler(SUBATOMIC_VARIANT,process_subatomic_packet,SUBATOMIC_REFUNDTX_NEEDSIG,sizeof(struct subatomic_packet),sizeof(struct subatomic_packet),0);
             register_variant_handler(SUBATOMIC_VARIANT,process_subatomic_packet,SUBATOMIC_REFUNDTX_SIGNED,sizeof(struct subatomic_packet),sizeof(struct subatomic_packet),0);
             register_variant_handler(SUBATOMIC_VARIANT,process_subatomic_packet,SUBATOMIC_FUNDINGTX,sizeof(struct subatomic_packet),sizeof(struct subatomic_packet),0);
             register_variant_handler(SUBATOMIC_VARIANT,process_subatomic_packet,SUBATOMIC_SEND_MICROTX,sizeof(struct subatomic_packet),sizeof(struct subatomic_packet),0);
             if ( pthread_create(malloc(sizeof(pthread_t)),NULL,_server_loop,&variant) != 0 )
             printf("ERROR _server_loop\n");*/
        }
        return(gp);
    }
    else if ( parms->mode == NXTPROTOCOL_AMTXID )
        process_subatomic_AM(gp,parms);
    else if ( parms->mode == NXTPROTOCOL_TYPEMATCH )
        process_subatomic_typematch(gp,parms);
    return(gp);
}

#ifdef bitcoin_doesnt_seem_to_support_nonstandard_tx
/* https://bitcointalk.org/index.php?topic=193281.msg3315031#msg3315031
 
 jl777:
 This approach is supposed to be the "holy grail", but only one miner (elgius) even accepts the nonstandard
 transactions for bitcoin network. Probably none of the miners for any of the altcoins do, so basically it
 is a thought experiment that cant possible actually work. I realized this after coding some parts of it up...
 
 description:
 A picks a random number x
 
 A creates TX1: "Pay w BTC to <B's public key> if (x for H(x) known and signed by B) or (signed by A & B)"
 
 A creates TX2: "Pay w BTC from TX1 to <A's public key>, locked 48 hours in the future, signed by A"
 
 A sends TX2 to B
 
 B signs TX2 and returns to A
 
 1) A submits TX1 to the network
 
 B creates TX3: "Pay v alt-coins to <A-public-key> if (x for H(x) known and signed by A) or (signed by A & B)"
 
 B creates TX4: "Pay v alt-coins from TX3 to <B's public key>, locked 24 hours in the future, signed by B"
 
 B sends TX4 to A
 
 A signs TX4 and sends back to B
 
 2) B submits TX3 to the network
 
 3) A spends TX3 giving x
 
 4) B spends TX1 using x
 
 This is atomic (with timeout).  If the process is halted, it can be reversed no matter when it is stopped.
 
 Before 1: Nothing public has been broadcast, so nothing happens
 Between 1 & 2: A can use refund transaction after 48 hours to get his money back
 Between 2 & 3: B can get refund after 24 hours.  A has 24 more hours to get his refund
 After 3: Transaction is completed by 2
 - A must spend his new coin within 24 hours or B can claim the refund and keep his coins
 - B must spend his new coin within 48 hours or A can claim the refund and keep his coins
 
 For safety, both should complete the process with lots of time until the deadlines.
 locktime for A significantly longer than for B
 
 
 ************** NXT variant
 A creates any ref TX to generate fullhash, sighash, utxbytes to give to B, signature is used as x
 Pay w BTC to <B's public key> if (x for H(x) known and signed by B)
 
 B creates and broadcasts "pay NXT to A" referencing fullhash of refTX and sends unsigned txbytes to A, expires in 24 hours
 
 A broadcasts ref TX and gets paid the NXT
 B obtains signature from refTX and spends TX1
 
 
 **************
 OP_IF 0x63
 // Refund for A
 2 <pubkeyA> <pubkeyB> 2 OP_CHECKMULTISIGVERIFY 0xaf
 OP_ELSE 0x67
 // Ordinary claim for B
 OP_HASH160 0xa9 <H(x)> OP_EQUAL 0x87  <pubkeyB/pubkeyA> OP_CHECKSIGVERIFY 0xad
 OP_ENDIF 0x68
 
 0x63 2 <pubkeyA> <pubkeyB> 2 0xaf 0x67 0xa9 <H(x)> 0x87 <pubkeyB> 0xad
 */

#define SCRIPT_OP_IF 0x63
#define SCRIPT_OP_ELSE 0x67
#define SCRIPT_OP_ENDIF 0x68
#define SCRIPT_OP_EQUAL 0x87
#define SCRIPT_OP_HASH160 0xa9
#define SCRIPT_OP_CHECKSIGVERIFY 0xad
#define SCRIPT_OP_CHECKMULTISIGVERIFY 0xaf


struct atomictx_half
{
    uint64_t amount;
    int32_t vout,coinid;
    char *srctxid,*srcscriptpubkey;
    char *srcaddr,*srcpubkey,*destaddr,*destpubkey;
    char *redeemScript,*refundtx,*claimtx;
};


int32_t atomictx_addbytes(unsigned char *hex,int32_t n,unsigned char *bytes,int32_t len)
{
    int32_t i;
    for (i=0; i<len; i++)
        hex[n++] = bytes[i];
    return(n);
}

char *create_atomictx_scriptPubkey(int a_or_b,char *pubkeyA,char *pubkeyB,unsigned char *hash160)
{
    char *retstr;
    unsigned char pubkeyAbytes[33],pubkeyBbytes[33],hex[4096];
    int32_t i,n = 0;
    if ( pubkey_to_256bits(pubkeyAbytes,pubkeyA) < 0 || pubkey_to_256bits(pubkeyBbytes,pubkeyB) < 0 )
        return(0);
    hex[n++] = SCRIPT_OP_IF;
    hex[n++] = 2;
    hex[n++] = 32, memcpy(&hex[n],pubkeyAbytes,32), n += 32;
    hex[n++] = 32, memcpy(&hex[n],pubkeyBbytes,32), n += 32;
    hex[n++] = 2;
    hex[n++] = SCRIPT_OP_CHECKMULTISIGVERIFY;
    hex[n++] = SCRIPT_OP_ELSE;
    hex[n++] = SCRIPT_OP_HASH160;
    hex[n++] = 20; memcpy(&hex[n],hash160,20); n += 20;
    hex[n++] = SCRIPT_OP_EQUAL;
    if ( a_or_b == 'a' )
        hex[n++] = 32, memcpy(&hex[n],pubkeyAbytes,32), n += 32;
    else if ( a_or_b == 'b' )
        hex[n++] = 32, memcpy(&hex[n],pubkeyBbytes,32), n += 32;
    else { printf("illegal a_or_b %d\n",a_or_b); return(0); }
    hex[n++] = SCRIPT_OP_CHECKSIGVERIFY;
    hex[n++] = SCRIPT_OP_ENDIF;
    retstr = malloc(n*2+1);
    printf("pubkeyA.(%s) pubkeyB.(%s) ->\n",pubkeyA,pubkeyB);
    for (i=0; i<n; i++)
    {
        retstr[i*2] = hexbyte((hex[i]>>4) & 0xf);
        retstr[i*2 + 1] = hexbyte(hex[i] & 0xf);
        printf("%02x",hex[i]);
    }
    retstr[n*2] = 0;
    printf("-> (%s)\n",retstr);
    return(retstr);
}

void _init_atomictx_half(struct atomictx_half *atomic,int coinid,char *txid,int vout,char *scriptpubkey,uint64_t amount,char *srcaddr,char *srcpubkey,char *destaddr,char *destpubkey)
{
    atomic->amount = amount; atomic->vout = vout; atomic->coinid = coinid;
    atomic->srctxid = txid; atomic->srcscriptpubkey = scriptpubkey;
    atomic->srcaddr = srcaddr; atomic->srcpubkey = srcpubkey;
    atomic->destaddr = destaddr; atomic->destpubkey = destpubkey;
}

void gen_atomictx_initiator(struct atomictx_half *atomic)
{
    int32_t i;
    cJSON *obj,*json;
    char msg[65],txidstr[4096],*jsonstr;
    unsigned char hash160[20],x[32];
    randombytes(x,sizeof(x)/sizeof(*x));
    for (i=0; i<(int32_t)(sizeof(x)/sizeof(*x)); i++)
    {
        msg[i*2] = hexbyte((x[i]>>4) & 0xf);
        msg[i*2 + 1] = hexbyte(x[i] & 0xf);
        printf("%02x ",x[i]);
    }
    msg[64] = 0;
    printf("-> (%s) x[]\n",msg);
    calc_OP_HASH160(hash160,msg);
    atomic->redeemScript = create_atomictx_scriptPubkey('b',atomic->srcpubkey,atomic->destpubkey,hash160);
    if ( atomic->redeemScript != 0 )
    {
        json = cJSON_CreateObject();
        obj = cJSON_CreateString(atomic->srctxid); cJSON_AddItemToObject(json,"txid",obj);
        obj = cJSON_CreateNumber(atomic->vout); cJSON_AddItemToObject(json,"vout",obj);
        obj = cJSON_CreateString(atomic->srcscriptpubkey); cJSON_AddItemToObject(json,"scriptPubKey",obj);
        obj = cJSON_CreateString(atomic->redeemScript); cJSON_AddItemToObject(json,"redeemScript",obj);
        jsonstr = cJSON_Print(json);
        free_json(json);
        sprintf(txidstr,"'[%s]'  '{\"%s\":%lld}'",jsonstr,atomic->srcaddr,(long long)atomic->amount);
        stripwhite(txidstr,strlen(txidstr));
        printf("refundtx.(%s) change last 4 bytes or rawtransaction to future\n",txidstr);
        atomic->refundtx = clonestr(txidstr);
        sprintf(txidstr,"'[%s]'  '{\"%s\":%lld}'",jsonstr,atomic->destaddr,(long long)atomic->amount);
        stripwhite(txidstr,strlen(txidstr));
        printf("claimtx.(%s)\n",txidstr);
        free(jsonstr);
    }
}

void test_atomictx()
{
    struct atomictx_half initiator;
    _init_atomictx_half(&initiator,DOGE_COINID,"48ae45b3741c125ba63c8b98f2adb4dcfd050583734ca57a12d5bc874f8334bd",1,"76a914d43f07a88b9cf437314d5c281f201dd0bb5486e588ac",2*SATOSHIDEN,"DQVMJKcn3yW34nWpqheiNiRAhaY9qFeuER","0279886d0de4bfba245774d0c0a5c062578244b03eec567fe71a02c410b45ca86a","DDNhdeLp4PrwKKN1PeFnUv2pnzvhPLMgCd","03628f9fa04ed2ed83e07f8d3b73a654d1df86e670542008b96b9339a27f1905cb");
    gen_atomictx_initiator(&initiator);
}
#endif

///////////////////////
#ifdef fromlastyear
char *Signedtx; // hack for testing atomic_swap

#define HALFDUPLEX
#define BITCOIN_WALLET_UNLOCKSECONDS 300
#define SUBATOMIC_PORTNUM 6777
#define SUBATOMIC_VARIANT (SUBATOMIC_PORTNUM - SERVER_PORT)
#define SUBATOMIC_SIG 0x84319574
#define SUBATOMIC_LOCKTIME (3600 * 2)
#define SUBATOMIC_DONATIONRATE .001
#define SUBATOMIC_DEFAULTINCR 100
#define SUBATOMIC_TOOLONG (300 * 1000000)
#define MAX_SUBATOMIC_OUTPUTS 4
#define MAX_SUBATOMIC_INPUTS 16
#define SUBATOMIC_STARTING_SEQUENCEID 1000
#define SUBATOMIC_CANTSEND_TOLERANCE 3
#define MAX_NXT_TXBYTES 2048

#define SUBATOMIC_TYPE 0
#define SUBATOMIC_FORNXT_TYPE 3
#define NXTFOR_SUBATOMIC_TYPE 2
#define ATOMICSWAP_TYPE 1

#define SUBATOMIC_TRADEFUNC 't'
#define SUBATOMIC_ATOMICSWAP 's'

struct NXT_tx
{
    unsigned char refhash[32];
    uint64_t senderbits,recipientbits,assetidbits;
    int64_t feeNQT;
    union { int64_t amountNQT; int64_t quantityQNT; };
    int32_t deadline,type,subtype,verify;
    char comment[128];
};

struct atomic_swap
{
    char *parsed[2];
    cJSON *jsons[2];
    char NXTaddr[64],otherNXTaddr[64],otheripaddr[32];
    char txbytes[2][MAX_NXT_TXBYTES],signedtxbytes[2][MAX_NXT_TXBYTES];
    char sighash[2][68],fullhash[2][68];
    int32_t numfragis,atomictx_waiting;
    struct NXT_tx *mytx;
};

struct subatomic_halftx
{
    int32_t coinid,destcoinid,minconfirms;
    int64_t destamount,avail,amount,donation,myamount,otheramount;  // amount = (myamount + otheramount + donation + txfee)
    struct subatomic_rawtransaction funding,refund,micropay;
    char funding_scriptPubKey[512],countersignedrefund[1024],completedmicropay[1024];
    char *fundingtxid,*refundtxid,*micropaytxid;
    char NXTaddr[64],coinaddr[64],pubkey[128],ipaddr[32];
    char otherNXTaddr[64],destcoinaddr[64],destpubkey[128],otheripaddr[32];
    struct multisig_addr *xferaddr;
};

struct subatomic_tx_args
{
    char NXTaddr[64],otherNXTaddr[64],coinaddr[2][64],destcoinaddr[2][64],otheripaddr[32],mypubkeys[2][128];
    int64_t amount,destamount;
    double myshare;
    int32_t coinid,destcoinid,numincr,incr,otherincr;
};

struct subatomic_tx
{
    struct subatomic_tx_args ARGS,otherARGS;
    struct subatomic_halftx myhalf,otherhalf;
    struct atomic_swap swap;
    char *claimtxid;
    int64_t lastcontact,myexpectedamount,myreceived,otherexpectedamount,sent_to_other;//,recvflags;
    int32_t status,initflag,connsock,refundlockblock,cantsend,type,longerflag,tag,verified;
    int32_t txs_created,other_refundtx_done,myrefundtx_done,other_fundingtx_confirms;
    int32_t myrefund_fragi,microtx_fragi,funding_fragi,other_refund_fragi;
    int32_t other_refundtx_waiting,myrefundtx_waiting,other_fundingtx_waiting,other_micropaytx_waiting;
    unsigned char recvbufs[4][sizeof(struct subatomic_rawtransaction)];
};

struct subatomic_info
{
    char ipaddr[32],NXTADDR[32],NXTACCTSECRET[512];
    struct subatomic_tx **subatomics;
    CURL *curl_handle,*curl_handle2;
    int32_t numsubatomics,enable_bitcoin_broadcast;
} *Global_subatomic;

struct subatomic_packet
{
    //struct server_request_header H;
    struct subatomic_tx_args ARGS;
    char pubkeys[2][128],retpubkeys[2][128];
    struct subatomic_rawtransaction rawtx;
#ifdef HALFDUPLEX
    // struct subatomic_rawtransaction needsig,havesig,micropay;   // jl777: hack for my broken Mac
#endif
};
int32_t subatomic_gen_pubkeys(struct subatomic_tx *atx,struct subatomic_halftx *htx);

#ifdef test
void calc_OP_HASH160(unsigned char hash160[20],char *msg)
{
    unsigned char sha256[32];
    hash_state md;
    
    sha256_init(&md);
    sha256_process(&md,(unsigned char *)msg,strlen(msg));
    sha256_done(&md,sha256);
    
    rmd160_init(&md);
    rmd160_process(&md,(unsigned char *)sha256,256/8);
    rmd160_done(&md,hash160);
    {
        int i;
        for (i=0; i<20; i++)
            printf("%02x",hash160[i]);
        printf("<- (%s)\n",msg);
    }
}

// bitcoind functions


void subatomic_uint32_splicer(char *txbytes,int32_t offset,uint32_t spliceval)
{
    int32_t i;
    uint32_t x;
    if ( offset < 0 )
    {
        static int foo;
        if ( foo++ < 3 )
            printf("subatomic_uint32_splicer illegal offset.%d\n",offset);
        return;
    }
    for (i=0; i<4; i++)
    {
        x = spliceval & 0xff; spliceval >>= 8;
        txbytes[offset + i*2] = hexbyte((x>>4) & 0xf);
        txbytes[offset + i*2 + 1] = hexbyte(x & 0xf);
    }
}

struct btcinput_data
{
    unsigned char txid[32];
    uint32_t vout;
    int64_t scriptlen;
    uint32_t sequenceid;
    unsigned char script[];
};

struct btcoutput_data
{
    int64_t value,pk_scriptlen;
    unsigned char pk_script[];
};

struct btcinput_data *decode_btcinput(int32_t *offsetp,char *txbytes)
{
    struct btcinput_data *ptr,I;
    int32_t i;
    memset(&I,0,sizeof(I));
    for (i=0; i<32; i++)
        I.txid[31-i] = _decode_hex(txbytes + *offsetp), (*offsetp) += 2;
    I.vout = _decode_hexint(offsetp,txbytes);
    I.scriptlen = _decode_varint(offsetp,txbytes);
    if ( I.scriptlen > 1024 )
    {
        printf("decode_btcinput: very long script starting at offset.%d of (%s)\n",*offsetp,txbytes);
        return(0);
    }
    ptr = calloc(1,sizeof(*ptr) + I.scriptlen);
    *ptr = I;
    for (i=0; i<I.scriptlen; i++)
        ptr->script[i] = _decode_hex(txbytes + *offsetp), (*offsetp) += 2;
    ptr->sequenceid = _decode_hexint(offsetp,txbytes);
    return(ptr);
}

struct btcoutput_data *decode_btcoutput(int32_t *offsetp,char *txbytes)
{
    int32_t i;
    struct btcoutput_data *ptr,btcO;
    memset(&btcO,0,sizeof(btcO));
    btcO.value = _decode_hexlong(offsetp,txbytes);
    btcO.pk_scriptlen = _decode_varint(offsetp,txbytes);
    if ( btcO.pk_scriptlen > 1024 )
    {
        printf("decode_btcoutput: very long script starting at offset.%d of (%s)\n",*offsetp,txbytes);
        return(0);
    }
    ptr = calloc(1,sizeof(*ptr) + btcO.pk_scriptlen);
    *ptr = btcO;
    for (i=0; i<btcO.pk_scriptlen; i++)
        ptr->pk_script[i] = _decode_hex(txbytes + *offsetp), (*offsetp) += 2;
    return(ptr);
}

uint32_t calc_vin0seqstart(char *txbytes)
{
    struct btcinput_data *btcinput,*firstinput = 0;
    struct btcoutput_data *btcoutput;
    char buf[4096];
    int32_t i,vin0seqstart,numoutputs,numinputs,offset = 0;
    //version = _decode_hexint(&offset,txbytes);
    numinputs = _decode_hex(&txbytes[offset]), offset += 2;
    vin0seqstart = 0;
    for (i=0; i<numinputs; i++)
    {
        btcinput = decode_btcinput(&offset,txbytes);
        if ( btcinput != 0 )
        {
            init_hexbytes(buf,btcinput->txid,32);
            // printf("(%s vout%d) ",buf,btcinput->vout);
        }
        if ( i == 0 )
        {
            firstinput = btcinput;
            vin0seqstart = offset - sizeof(int32_t)*2;
        }
        else if ( btcinput != 0 ) free(btcinput);
    }
    //printf("-> ");
    numoutputs = _decode_hex(&txbytes[offset]), offset += 2;
    for (i=0; i<numoutputs; i++)
    {
        btcoutput = decode_btcoutput(&offset,txbytes);
        if ( btcoutput != 0 )
        {
            init_hexbytes(buf,btcoutput->pk_script,btcoutput->pk_scriptlen);
            // printf("(%s %.8f) ",buf,dstr(btcoutput->value));
            free(btcoutput);
        }
    }
    //locktime =
    _decode_hexint(&offset,txbytes);
    // printf("version.%d 1st.seqid %d @ %d numinputs.%d numoutputs.%d locktime.%d\n",version,firstinput!=0?firstinput->sequenceid:0xffffffff,vin0seqstart,numinputs,numoutputs,locktime);
    //0100000001ac8511d408d35e62ccc7925ed2437022e9b7e9e731197a42a58495e4465439d10000000000ffffffff0200dc5c24020000001976a914bf685a09e61215c7e824d0b73bc6d6d3ba9d9d9688ac00c2eb0b000000001976a91414b24a5b6f8c8df0f7c9b519d362618ca211e60988ac00000000
    if ( firstinput != 0 )
        return(vin0seqstart);
    else return(-1);
}


/* https://en.bitcoin.it/wiki/Contracts#Example_7:_Rapidly-adjusted_.28micro.29payments_to_a_pre-determined_party
 1) Create a public key (K1). Request a public key from the server (K2).
 2) Create and sign but do not broadcast a transaction (T1) that sets up a payment of (for example) 10 BTC to an output requiring both the server's public key and one of your own to be used. A good way to do this is use OP_CHECKMULTISIG. The value to be used is chosen as an efficiency tradeoff.
 3) Create a refund transaction (T2) that is connected to the output of T1 which sends all the money back to yourself. It has a time lock set for some time in the future, for instance a few hours. Don't sign it, and provide the unsigned transaction to the server. By convention, the output script is "2 K1 K2 2 CHECKMULTISIG"
 4) The server signs T2 using its public key K2 and returns the signature to the client. Note that it has not seen T1 at this point, just the hash (which is in the unsigned T2).
 5) The client verifies the servers signature is correct and aborts if not.
 6) The client signs T1 and passes the signature to the server, which now broadcasts the transaction (either party can do this if they both have connectivity). This locks in the money.
 
 7) The client then creates a new transaction, T3, which connects to T1 like the refund transaction does and has two outputs. One goes to K1 and the other goes to K2. It starts out with all value allocated to the first output (K1), ie, it does the same thing as the refund transaction but is not time locked. The client signs T3 and provides the transaction and signature to the server.
 8) The server verifies the output to itself is of the expected size and verifies the client's provided signature is correct.
 9) When the client wishes to pay the server, it adjusts its copy of T3 to allocate more value to the server's output and less to its ow. It then re-signs the new T3 and sends the signature to the server. It does not have to send the whole transaction, just the signature and the amount to increment by is sufficient. The server adjusts its copy of T3 to match the new amounts, verifies the signature and continues.
 
 10) This continues until the session ends, or the 1-day period is getting close to expiry. The AP then signs and broadcasts the last transaction it saw, allocating the final amount to itself. The refund transaction is needed to handle the case where the server disappears or halts at any point, leaving the allocated value in limbo. If this happens then once the time lock has expired the client can broadcast the refund transaction and get back all the money.
 */

// subatomic logic functions

void subatomic_callback(struct NXT_acct *np,int32_t fragi,struct subatomic_tx *atx,struct json_AM *ap,cJSON *json,void *binarydata,int32_t binarylen,uint32_t *targetcrcs)
{
    void *ptr;
    uint32_t TCRC,tcrc;
    int32_t *iptr,coinid,i,j,n,starti,completed,incr,ind,funcid = (ap->funcid & 0xffff);
    cJSON *addrobj,*pubkeyobj,*txobj;
    char coinaddr[64],pubkey[128],txbytes[1024];
    if ( funcid == SUBATOMIC_SEND_ATOMICTX )
    {
        // printf("got atomictx\n");
        txobj = cJSON_GetObjectItem(json,"txbytes");
        if ( txobj != 0 )
        {
            copy_cJSON(txbytes,txobj);
            if ( strlen(txbytes) > 32 )
            {
                //printf("atx.%p TXBYTES.(%s)\n",atx,txbytes);
                safecopy(atx->swap.signedtxbytes[1],txbytes,sizeof(atx->swap.signedtxbytes[1]));
                atx->swap.atomictx_waiting = 1;
            }
        }
    }
    else if ( funcid == SUBATOMIC_SEND_PUBKEY )
    {
        if ( json != 0 )
        {
            coinid = (int32_t)get_cJSON_int(json,"coinid");
            if ( coinid != 0 )
            {
                addrobj = cJSON_GetObjectItem(json,coinid_str(coinid));
                copy_cJSON(coinaddr,addrobj);
                if ( strcmp(coinaddr,atx->otherhalf.coinaddr) == 0 )
                {
                    pubkeyobj = cJSON_GetObjectItem(json,"pubkey");
                    copy_cJSON(pubkey,pubkeyobj);
                    if ( strlen(pubkey) >= 64 )
                        strcpy(atx->otherhalf.pubkey,pubkey);
                }
            }
        }
    }
    else
    {
        starti = (int32_t)get_cJSON_int(json,"starti");
        i = (int32_t)get_cJSON_int(json,"i");
        n = (int32_t)get_cJSON_int(json,"n");
        incr = (int32_t)get_cJSON_int(json,"incr");
        TCRC = (uint32_t)get_cJSON_int(json,"TCRC");
        if ( incr == binarylen && starti >= 0 && starti < SYNC_MAXUNREPORTED && i >= 0 && i < SYNC_MAXUNREPORTED && n >= 0 && n < SYNC_MAXUNREPORTED )
        {
            completed = 0;
            for (j=starti; j<starti+n; j++)
            {
                //printf("(%08x vs %08x) ",np->memcrcs[j],targetcrcs[j]);
                if ( np->memcrcs[j] == targetcrcs[j] )
                    completed++;
            }
            ind = -1;
            ptr = 0; iptr = 0;
            switch ( funcid )
            {
                default: printf("illegal funcid.%d\n",funcid); break;
                case SUBATOMIC_REFUNDTX_NEEDSIG: ptr = &atx->otherhalf.refund; ind = 0; iptr = &atx->other_refundtx_waiting; break;
                case SUBATOMIC_REFUNDTX_SIGNED: ptr = &atx->myhalf.refund; ind = 1; iptr = &atx->myrefundtx_waiting; break;
                case SUBATOMIC_FUNDINGTX: ptr = &atx->otherhalf.funding; ind = 2; iptr = &atx->other_fundingtx_waiting; break;
                case SUBATOMIC_SEND_MICROTX: ptr = &atx->otherhalf.micropay; ind = 3; iptr = &atx->other_micropaytx_waiting; break;
            }
            if ( ind >= 0 )
            {
                memcpy(atx->recvbufs[ind]+i*incr,binarydata,binarylen);
                tcrc = _crc32(0,atx->recvbufs[ind],sizeof(struct subatomic_rawtransaction));
                if ( completed == n && TCRC == tcrc )
                {
                    //printf("completed.%d ptr.%p ind.%d i.%d binarydata.%p binarylen.%d crc.%u vs %u\n",completed,ptr,ind,i,binarydata,binarylen,tcrc,TCRC);
                    memcpy(ptr,atx->recvbufs[ind],sizeof(struct subatomic_rawtransaction));
                }
            }
            if ( iptr != 0 )
                *iptr = completed;
        }
    }
}

struct subatomic_tx *subatomic_search_connections(char *NXTaddr,int32_t otherflag,int32_t type)
{
    struct subatomic_info *gp = Global_subatomic;
    int32_t i;
    struct subatomic_tx *atx;
    for (i=0; i<gp->numsubatomics; i++)
    {
        atx = gp->subatomics[i];
        if ( type != atx->type || atx->initflag != 3 || atx->status == SUBATOMIC_COMPLETED || atx->status == SUBATOMIC_ABORTED )
            continue;
        if ( otherflag == 0 && strcmp(NXTaddr,atx->ARGS.NXTaddr) == 0 )  // assumes one IP per NXT addr
            return(atx);
        else if ( otherflag != 0 && strcmp(NXTaddr,atx->ARGS.otherNXTaddr) == 0 )  // assumes one IP per NXT addr
            return(atx);
    }
    return(0);
}

void sharedmem_callback(member_t *pm,int32_t fragi,void *ptr,uint32_t crc,uint32_t *targetcrcs)
{
    cJSON *json;
    void *binarydata;
    struct subatomic_tx *atx;
    int32_t createdflag,binarylen;
    char otherNXTaddr[64],*jsontxt = 0;
    struct NXT_acct *np;
    struct json_AM *ap = ptr;
    expand_nxt64bits(otherNXTaddr,ap->H.nxt64bits);
    if ( strcmp(pm->NXTaddr,otherNXTaddr) != 0 )
        printf("WARNING: mismatched member NXT.%s vs sender.%s\n",pm->NXTaddr,otherNXTaddr);
    binarylen = (ap->funcid>>16)&0xffff;
    if ( ap->H.size > 16 && ap->H.size+binarylen < 1024 && binarylen > 0 )
        binarydata = (void *)((long)ap + ap->H.size);
    else binarydata = 0;
    json = parse_json_AM(ap);
    if ( json != 0 )
        jsontxt = cJSON_Print(json);
    np = get_NXTacct(&createdflag,Global_mp,otherNXTaddr);
    np->recvid++;
    np->memcrcs[fragi] = crc;
    //printf("[R%d S%d] other.[R%d S%d] %x size.%d %p.[%08x].binarylen.%d %s (funcid.%d arg.%d seqid.%d flag.%d) [%s]\n",np->recvid,np->sentid,ap->gatewayid,ap->timestamp,ap->H.sig,ap->H.size,binarydata,binarydata!=0?*(int *)binarydata:0,binarylen,nxt64str(ap->H.nxt64bits),ap->funcid&0xffff,ap->gatewayid,ap->timestamp,ap->jsonflag,jsontxt!=0?jsontxt:"");
    if ( ap->H.sig == SUBATOMIC_SIG )
    {
        if ( (ap->funcid&0xffff) == SUBATOMIC_SEND_ATOMICTX )
            atx = subatomic_search_connections(otherNXTaddr,1,ATOMICSWAP_TYPE);
        else
            atx = subatomic_search_connections(otherNXTaddr,1,0);
        if ( atx != 0 )
            subatomic_callback(np,fragi,atx,ap,json,binarydata,binarylen,targetcrcs);
    }
    if ( jsontxt != 0 )
        free(jsontxt);
    if ( json != 0 )
        free_json(json);
}

int32_t share_pubkey(struct NXT_acct *np,int32_t fragi,int32_t destcoinid,char *destcoinaddr,char *destpubkey)
{
    char jsonstr[512];
    // also check other pubkey and if matches atx->otherhalf.coinaddr set atx->otherhalf.pubkey
    sprintf(jsonstr,"{\"coinid\":%d,\"%s\":\"%s\",\"pubkey\":\"%s\"}",destcoinid,coinid_str(destcoinid),destcoinaddr,destpubkey);
    send_to_NXTaddr(&np->localcrcs[fragi],np->H.NXTaddr,fragi,SUBATOMIC_SIG,SUBATOMIC_SEND_PUBKEY,jsonstr,0,0);
    return(fragi+1);
}

int32_t share_atomictx(struct NXT_acct *np,char *txbytes,int32_t fragi)
{
    char jsonstr[512];
    sprintf(jsonstr,"{\"txbytes\":\"%s\"}",txbytes);
    send_to_NXTaddr(&np->localcrcs[fragi],np->H.NXTaddr,fragi,SUBATOMIC_SIG,SUBATOMIC_SEND_ATOMICTX,jsonstr,0,0);
    return(fragi+1);
}

int32_t update_atomic(struct NXT_acct *np,struct subatomic_tx *atx)
{
    cJSON *txjson,*json;
    int32_t j,status = 0;
    char signedtx[1024],fullhash[128],*parsed,*retstr,*padded;
    struct atomic_swap *sp;
    struct NXT_tx *utx,*refutx = 0;
    sp = &atx->swap;
    if ( atx->longerflag != 1 )
    {
        //printf("atomixtx waiting.%d atx.%p\n",sp->atomictx_waiting,atx);
        if ( sp->atomictx_waiting != 0 )
        {
            printf("GOT.(%s)\n",sp->signedtxbytes[1]);
            if ( (parsed = issue_parseTransaction(Global_subatomic->curl_handle,sp->signedtxbytes[1])) != 0 )
            {
                json = cJSON_Parse(parsed);
                if ( json != 0 )
                {
                    refutx = set_NXT_tx(sp->jsons[1]);
                    utx = set_NXT_tx(json);
                    //printf("refutx.%p utx.%p verified.%d\n",refutx,utx,utx->verify);
                    if ( utx != 0 && refutx != 0 && utx->verify != 0 )
                    {
                        if ( NXTutxcmp(refutx,utx,1.) == 0 )
                        {
                            padded = malloc(strlen(sp->txbytes[0]) + 129);
                            strcpy(padded,sp->txbytes[0]);
                            for (j=0; j<128; j++)
                                strcat(padded,"0");
                            retstr = issue_signTransaction(Global_subatomic->curl_handle,padded);
                            free(padded);
                            printf("got signed tx that matches agreement submit.(%s) (%s)\n",padded,retstr);
                            if ( retstr != 0 )
                            {
                                txjson = cJSON_Parse(retstr);
                                if ( txjson != 0 )
                                {
                                    extract_cJSON_str(sp->signedtxbytes[0],sizeof(sp->signedtxbytes[0]),txjson,"transactionBytes");
                                    if ( extract_cJSON_str(fullhash,sizeof(fullhash),txjson,"fullHash") > 0 )
                                    {
                                        if ( strcmp(fullhash,sp->fullhash[0]) == 0 )
                                        {
                                            printf("broadcast (%s) and (%s)\n",sp->signedtxbytes[0],sp->signedtxbytes[1]);
                                            status = SUBATOMIC_COMPLETED;
                                        }
                                        else printf("ERROR: can't reproduct fullhash of trigger tx %s != %s\n",fullhash,sp->fullhash[0]);
                                    }
                                    free_json(txjson);
                                }
                                free(retstr);
                            }
                        } else printf("tx compare error\n");
                    }
                    if ( utx != 0 ) free(utx);
                    if ( refutx != 0 ) free(refutx);
                    free_json(json);
                } else printf("error JSON parsing.(%s)\n",parsed);
                free(parsed);
            } else printf("error parsing (%s)\n",sp->signedtxbytes[1]);
            sp->atomictx_waiting = 0;
        }
    }
    else if ( atx->longerflag == 1 )
    {
        if ( sp->numfragis == 0 )
        {
            utx = set_NXT_tx(sp->jsons[0]);
            if ( utx != 0 )
            {
                refutx = sign_NXT_tx(Global_subatomic->curl_handle,signedtx,utx,sp->fullhash[1],1.);
                /*txjson = gen_NXT_tx_json(utx,sp->fullhash[1],1.);
                 signedtx[0] = 0;
                 if ( txjson != 0 )
                 {
                 if ( extract_cJSON_str(signedtx,sizeof(signedtx),txjson,"transactionBytes") > 0 )
                 {
                 if ( (parsed = issue_parseTransaction(signedtx)) != 0 )
                 {
                 refjson = cJSON_Parse(parsed);
                 if ( refjson != 0 )
                 {
                 refutx = set_NXT_tx(refjson);
                 free_json(refjson);
                 }
                 free(parsed);
                 }
                 }
                 free_json(txjson);
                 }*/
                if ( refutx != 0 )
                {
                    if ( NXTutxcmp(refutx,utx,1.) == 0 )
                    {
                        printf("signed and referenced tx verified\n");
                        safecopy(sp->signedtxbytes[0],signedtx,sizeof(sp->signedtxbytes[0]));
                        sp->numfragis = share_atomictx(np,sp->signedtxbytes[0],1);
                        status = SUBATOMIC_COMPLETED;
                    }
                    free(refutx);
                }
                free(utx);
            }
        }
        else
        {
            // wont get here now, eventually add checks for blockchain completion or direct xfer from other side
            share_atomictx(np,sp->signedtxbytes[0],1);
        }
    }
    return(status);
}

char *AM_subatomic(int32_t func,int32_t rating,char *destaddr,char *senderNXTaddr,char *jsontxt)
{
    char AM[4096],*retstr;
    struct json_AM *ap = (struct json_AM *)AM;
    stripwhite(jsontxt,strlen(jsontxt));
    printf("sender.%s func.(%c) AM_subatomic(%s) -> NXT.(%s) rating.%d\n",senderNXTaddr,func,jsontxt,destaddr,rating);
    set_json_AM(ap,SUBATOMIC_SIG,func,senderNXTaddr,rating,jsontxt,1);
    retstr = submit_AM(Global_subatomic->curl_handle,destaddr,&ap->H,0);
    printf("AM_subatomic.(%s)\n",retstr);
    return(retstr);
}

cJSON *gen_atomicswap_json(int AMflag,struct atomic_swap *sp,char *senderip)
{
    cJSON *json;
    json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"type",cJSON_CreateString("NXTatomicswap"));
    if ( sp->NXTaddr[0] != 0 )
        cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(sp->NXTaddr));
    if ( sp->parsed[0] != 0 )
    {
        if ( AMflag == 0 )
        {
            if ( sp->signedtxbytes[0] != 0 )
                cJSON_AddItemToObject(json,"signedtxbytes",cJSON_CreateString(sp->signedtxbytes[0]));
            if ( sp->fullhash[0] != 0 )
                cJSON_AddItemToObject(json,"myfullhash",cJSON_CreateString(sp->fullhash[0]));
            if ( sp->jsons != 0 )
                cJSON_AddItemToObject(json,"myTX",sp->jsons[0]);
        }
        if ( sp->txbytes[0] != 0 )
            cJSON_AddItemToObject(json,"mytxbytes",cJSON_CreateString(sp->txbytes[0]));
        if ( sp->sighash[0] != 0 )
            cJSON_AddItemToObject(json,"mysighash",cJSON_CreateString(sp->sighash[0]));
    }
    if ( sp->otherNXTaddr[0] != 0 )
        cJSON_AddItemToObject(json,"destNXT",cJSON_CreateString(sp->otherNXTaddr));
    if ( sp->otheripaddr[0] != 0 )
        cJSON_AddItemToObject(json,"otheripaddr",cJSON_CreateString(sp->otheripaddr));
    if ( sp->parsed[0] != 0 )
    {
        if ( sp->txbytes[1] != 0 )
            cJSON_AddItemToObject(json,"othertxbytes",cJSON_CreateString(sp->txbytes[1]));
        if ( sp->sighash[1] != 0 )
            cJSON_AddItemToObject(json,"othersighash",cJSON_CreateString(sp->sighash[1]));
        if ( AMflag == 0 )
        {
            if ( sp->signedtxbytes[1] != 0 )
                cJSON_AddItemToObject(json,"othersignedtxbytes",cJSON_CreateString(sp->signedtxbytes[1]));
            if ( sp->fullhash[1] != 0 )
                cJSON_AddItemToObject(json,"otherfullhash",cJSON_CreateString(sp->fullhash[1]));
            if ( sp->jsons[1] != 0 )
                cJSON_AddItemToObject(json,"otherTX",sp->jsons[1]);
        }
    }
    
    return(json);
}

cJSON *gen_subatomic_json(int AMflag,int type,struct subatomic_tx *_atx,char *senderip)
{
    struct subatomic_tx_args *atx = &_atx->ARGS;
    struct subatomic_info *gp = Global_subatomic;
    char numstr[512];
    cJSON *json;
    json = cJSON_CreateObject();
    if ( type == ATOMICSWAP_TYPE )
        return(gen_atomicswap_json(AMflag,&_atx->swap,senderip));
    cJSON_AddItemToObject(json,"type",cJSON_CreateString(type==0?"subatomic_crypto":"NXTatomicswap"));
    if ( AMflag == 0 )
    {
        cJSON_AddItemToObject(json,"type",cJSON_CreateString(type==0?"subatomic_crypto":"NXTatomicswap"));
        if ( _atx->myexpectedamount != 0 )
            cJSON_AddItemToObject(json,"completed",cJSON_CreateNumber(dstr((double)_atx->myreceived/_atx->myexpectedamount)));
        cJSON_AddItemToObject(json,"received",cJSON_CreateNumber(dstr(_atx->myreceived)));
        cJSON_AddItemToObject(json,"expected",cJSON_CreateNumber(dstr(_atx->myexpectedamount)));
        cJSON_AddItemToObject(json,"sent",cJSON_CreateNumber(dstr(_atx->sent_to_other)));
        cJSON_AddItemToObject(json,"sending",cJSON_CreateNumber(dstr(_atx->otherexpectedamount)));
        if ( senderip == 0 && gp->ipaddr[0] != 0 )
            cJSON_AddItemToObject(json,"ipaddr",cJSON_CreateString(gp->ipaddr));
        if ( atx->coinaddr[1][0] != 0 )
            cJSON_AddItemToObject(json,"destNXTcoinaddr",cJSON_CreateString(atx->coinaddr[1]));
        if ( atx->destcoinaddr[1][0] != 0 )
            cJSON_AddItemToObject(json,"destNXTdestcoinaddr",cJSON_CreateString(atx->destcoinaddr[1]));
        if ( senderip == 0 && atx->otheripaddr[0] != 0 )
            cJSON_AddItemToObject(json,"otherip",cJSON_CreateString(atx->otheripaddr));
    }
    else cJSON_AddItemToObject(json,"senderip",cJSON_CreateString(Global_mp->ipaddr));
    if ( gp->NXTADDR[0] != 0 )
        cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(gp->NXTADDR));
    cJSON_AddItemToObject(json,"coin",cJSON_CreateString(coinid_str(atx->coinid)));
    sprintf(numstr,"%.8f",dstr(atx->amount)); cJSON_AddItemToObject(json,"amount",cJSON_CreateString(numstr));
    if ( atx->coinaddr[0][0] != 0 )
        cJSON_AddItemToObject(json,"coinaddr",cJSON_CreateString(atx->coinaddr[0]));
    cJSON_AddItemToObject(json,"destcoin",cJSON_CreateString(coinid_str(atx->destcoinid)));
    sprintf(numstr,"%.8f",dstr(atx->destamount)); cJSON_AddItemToObject(json,"destamount",cJSON_CreateString(numstr));
    if ( atx->destcoinaddr[0][0] != 0 )
        cJSON_AddItemToObject(json,"destcoinaddr",cJSON_CreateString(atx->destcoinaddr[0]));
    
    if ( atx->otherNXTaddr[0] != 0 )
        cJSON_AddItemToObject(json,"destNXT",cJSON_CreateString(atx->otherNXTaddr));
    
    return(json);
}

char *subatomic_cancel_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs)
{
    char *retstr = 0;
    retstr = clonestr("{\"result\":\"subatomic cancel trade not supported yet\"}");
    return(retstr);
}

char *subatomic_status_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs)
{
    struct subatomic_info *gp = Global_subatomic;
    int32_t i;
    cJSON *item,*array;
    char *retstr = 0;
    printf("do status\n");
    if ( gp->numsubatomics == 0 )
        retstr = clonestr("{\"result\":\"subatomic no trades pending\"}");
    else
    {
        array = cJSON_CreateArray();
        for (i=0; i<gp->numsubatomics; i++)
        {
            item = gen_subatomic_json(0,gp->subatomics[i]->type,gp->subatomics[i],0);
            if ( item != 0 )
                cJSON_AddItemToArray(array,item);
        }
        retstr = cJSON_Print(array);
        free_json(array);
    }
    return(retstr);
}

char *subatomic_trade_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs)
{
    char NXTaddr[64],amountstr[128],destamountstr[128],otheripaddr[32],coin[64],destcoin[64],coinaddr[MAX_NXT_TXBYTES],destcoinaddr[MAX_NXT_TXBYTES],otherNXTaddr[64],*retstr = 0;
    cJSON *json;
    int32_t flipped,type;
    struct subatomic_tx T;
    char *jsontxt,*AMtxid,buf[4096];
    if ( (type= set_subatomic_argstrs(&flipped,sender,0,objs,amountstr,destamountstr,NXTaddr,coin,destcoin,coinaddr,destcoinaddr,otherNXTaddr,otheripaddr)) >= 0 )
    {
        if ( set_subatomic_trade(type,&T,NXTaddr,coin,amountstr,coinaddr,otherNXTaddr,destcoin,destamountstr,destcoinaddr,otheripaddr,flipped) == 0 )
        {
            json = gen_subatomic_json(1,type,&T,otheripaddr[0]!=0?otheripaddr:0);
            if ( json != 0 )
            {
                jsontxt = cJSON_Print(json);
                AMtxid = AM_subatomic(SUBATOMIC_TRADEFUNC,0,otherNXTaddr,NXTaddr,jsontxt);
                if ( AMtxid != 0 )
                {
                    sprintf(buf,"{\"result\":\"good\",\"AMtxid\":\"%s\",\"trade\":%s}",AMtxid,jsontxt);
                    retstr = clonestr(buf);
                    free(AMtxid);
                }
                else
                {
                    sprintf(buf,"{\"result\":\"error\",\"AMtxid\":\"0\",\"trade\":%s}",jsontxt);
                    retstr = clonestr(buf);
                }
                free(jsontxt);
                free_json(json);
            }
            else retstr = clonestr("{\"result\":\"error generating json\"}");
        }
        else retstr = clonestr("{\"result\":\"error initializing subatomic trade\"}");
    }
    else retstr = clonestr("{\"result\":\"invalid trade args\"}");
    return(retstr);
}

char *subatomic_json_commands(struct subatomic_info *gp,cJSON *argjson,char *sender,int32_t valid)
{
    static char *subatomic_status[] = { (char *)subatomic_status_func, "subatomic_status", "", "NXT", 0 };
    static char *subatomic_cancel[] = { (char *)subatomic_cancel_func, "subatomic_cancel", "", "NXT", 0 };
    static char *subatomic_trade[] = { (char *)subatomic_trade_func, "subatomic_trade", "", "NXT", "coin", "amount", "coinaddr", "destNXT", "destcoin", "destamount", "destcoinaddr", "senderip", "type", "mytxbytes", "othertxbytes", "mysighash", "othersighash", 0 };
    static char **commands[] = { subatomic_status, subatomic_cancel, subatomic_trade };
    int32_t i,j;
    cJSON *obj,*nxtobj,*objs[16];
    char NXTaddr[64],command[4096],**cmdinfo;
    printf("subatomic_json_commands\n");
    memset(objs,0,sizeof(objs));
    command[0] = 0;
    memset(NXTaddr,0,sizeof(NXTaddr));
    if ( argjson != 0 )
    {
        obj = cJSON_GetObjectItem(argjson,"requestType");
        nxtobj = cJSON_GetObjectItem(argjson,"NXT");
        copy_cJSON(NXTaddr,nxtobj);
        copy_cJSON(command,obj);
        //printf("(%s) command.(%s) NXT.(%s)\n",cJSON_Print(argjson),command,NXTaddr);
    }
    //printf("multigateway_json_commands sender.(%s) valid.%d\n",sender,valid);
    for (i=0; i<(int32_t)(sizeof(commands)/sizeof(*commands)); i++)
    {
        cmdinfo = commands[i];
        //printf("needvalid.(%c) sender.(%s) valid.%d %d of %d: cmd.(%s) vs command.(%s)\n",cmdinfo[2][0],sender,valid,i,(int32_t)(sizeof(commands)/sizeof(*commands)),cmdinfo[1],command);
        if ( strcmp(cmdinfo[1],command) == 0 )
        {
            if ( cmdinfo[2][0] != 0 )
            {
                if ( sender[0] == 0 || valid != 1 || strcmp(NXTaddr,sender) != 0 )
                {
                    printf("verification valid.%d missing for %s sender.(%s) vs NXT.(%s)\n",valid,cmdinfo[1],NXTaddr,sender);
                    return(0);
                }
            }
            for (j=3; cmdinfo[j]!=0&&j<3+(int32_t)(sizeof(objs)/sizeof(*objs)); j++)
                objs[j-3] = cJSON_GetObjectItem(argjson,cmdinfo[j]);
            return((*(json_handler)cmdinfo[0])(sender,valid,objs,j-3));
        }
    }
    return(0);
}

char *subatomic_jsonhandler(cJSON *argjson)
{
    struct subatomic_info *gp = Global_subatomic;
    long len;
    int32_t valid;
    cJSON *json,*obj,*parmsobj,*tokenobj,*secondobj;
    char sender[64],*parmstxt,encoded[NXT_TOKEN_LEN],*retstr = 0;
    sender[0] = 0;
    valid = -1;
    printf("subatomic_jsonhandler argjson.%p\n",argjson);
    if ( argjson == 0 )
    {
        json = cJSON_CreateObject();
        obj = cJSON_CreateString(gp->NXTADDR); cJSON_AddItemToObject(json,"NXTaddr",obj);
        obj = cJSON_CreateString(gp->ipaddr); cJSON_AddItemToObject(json,"ipaddr",obj);
        
        obj = gen_NXTaccts_json(0);
        if ( obj != 0 )
            cJSON_AddItemToObject(json,"NXTaccts",obj);
        retstr = cJSON_Print(json);
        free_json(json);
        return(retstr);
    }
    else if ( (argjson->type&0xff) == cJSON_Array && cJSON_GetArraySize(argjson) == 2 )
    {
        parmsobj = cJSON_GetArrayItem(argjson,0);
        secondobj = cJSON_GetArrayItem(argjson,1);
        tokenobj = cJSON_GetObjectItem(secondobj,"token");
        copy_cJSON(encoded,tokenobj);
        parmstxt = cJSON_Print(parmsobj);
        len = strlen(parmstxt);
        stripwhite(parmstxt,len);
        printf("website.(%s) encoded.(%s) len.%ld\n",parmstxt,encoded,strlen(encoded));
        if ( strlen(encoded) == NXT_TOKEN_LEN )
            issue_decodeToken(Global_subatomic->curl_handle2,sender,&valid,parmstxt,encoded);
        free(parmstxt);
        argjson = parmsobj;
    }
    if ( sender[0] == 0 )
        strcpy(sender,gp->NXTADDR);
    retstr = subatomic_json_commands(gp,argjson,sender,valid);
    return(retstr);
}

void process_subatomic_AM(struct subatomic_info *dp,struct NXT_protocol_parms *parms)
{
    cJSON *argjson;
    //char *jsontxt;
    struct json_AM *ap;
    char *sender,*receiver;
    sender = parms->sender; receiver = parms->receiver; ap = parms->AMptr; //txid = parms->txid;
    if ( (argjson = parse_json_AM(ap)) != 0 && (strcmp(dp->NXTADDR,sender) == 0 || strcmp(dp->NXTADDR,receiver) == 0) )
    {
        printf("process_subatomic_AM got jsontxt.(%s)\n",ap->jsonstr);
        update_subatomic_state(dp,ap->funcid,ap->timestamp,argjson,ap->H.nxt64bits,sender,receiver);
        free_json(argjson);
    }
}

void process_subatomic_typematch(struct subatomic_info *dp,struct NXT_protocol_parms *parms)
{
    char NXTaddr[64],*sender,*receiver,*txid;
    sender = parms->sender; receiver = parms->receiver; txid = parms->txid;
    safecopy(NXTaddr,sender,sizeof(NXTaddr));
    printf("got txid.(%s) type.%d subtype.%d sender.(%s) -> (%s)\n",txid,parms->type,parms->subtype,sender,receiver);
}

void *subatomic_handler(struct NXThandler_info *mp,struct NXT_protocol_parms *parms,void *handlerdata,int32_t height)
{
    struct subatomic_info *gp = handlerdata;
    //static int32_t variant;
    cJSON *obj;
    char buf[512];
    if ( parms->txid == 0 )     // indicates non-transaction event
    {
        if ( parms->mode == NXTPROTOCOL_WEBJSON )
            return(subatomic_jsonhandler(parms->argjson));
        else if ( parms->mode == NXTPROTOCOL_NEWBLOCK )
        {
            //printf("subatomic new RTblock %d time %lld microseconds %ld\n",mp->RTflag,time(0),microseconds());
        }
        else if ( parms->mode == NXTPROTOCOL_IDLETIME )
        {
            update_subatomic_transfers(gp->NXTADDR);
            static int testhack;
            if ( testhack == 0 )
            {
#ifndef __APPLE__
                //char *teststr = "{\"requestType\":\"subatomic_trade\",\"NXT\":\"423766016895692955\",\"coin\":\"DOGE\",\"amount\":\"100\",\"coinaddr\":\"DNbAcP82bpd9xdXNA1Vtf1Vo6yqP1rZvcu\",\"senderip\":\"209.126.71.170\",\"destNXT\":\"8989816935121514892\",\"destcoin\":\"LTC\",\"destamount\":\".1\",\"destcoinaddr\":\"LLedxvb1e5aCYmQfn8PHEPFR56AsbGgbUG\"}";
                Signedtx = "00002724c6009a024e5bbad625df3d536fa90b1e6a28c3f5a56e1fcbe34132391c8d3fd7f671cb198c71b555df3ec27c00e1f5050000000000e1f50500000000000000000000000000000000000000000000000000000000000000000000000058f36933200b9766566c7c3a9250f4e7607f75011bda5b3ba40c9f820ae3a60f1471b68e203bf9897b147aced9938cfb91bdf3230a02637264e25bff85ce382a";
                char *teststr = "{\"requestType\":\"subatomic_trade\",\"type\":\"NXTatomicswap\",\"NXT\":\"423766016895692955\",\"mytxbytes\":\"00002724c6009a024e5bbad625df3d536fa90b1e6a28c3f5a56e1fcbe34132391c8d3fd7f671cb198c71b555df3ec27c00e1f5050000000000e1f505000000000000000000000000000000000000000000000000000000000000000000000000\",\"mysighash\":\"422bf4f0a389398589553995f4dcb0b2a0811641ffb7e8222ce3dc3272628065\",\"destNXT\":\"8989816935121514892\",\"othertxbytes\":\"0000be25c6009a0225c5fed2690701cf06f267e7c227b1a3c0dfa9c6fc3cdb593b3af6f16d65302f9b30f378f284e10500e1f5050000000000e1f505000000000000000000000000000000000000000000000000000000000000000000000000\",\"othersighash\":\"8e72bba1bdf481bd6c03e61bceb53dfa642be04cb71f23755b5fa939b24ffa55\"}";
#else
                //char *teststr = "{\"requestType\":\"subatomic_trade\",\"NXT\":\"8989816935121514892\",\"coin\":\"LTC\",\"amount\":\".1\",\"coinaddr\":\"LRQ3ZyrZDhvKFkcbmC6cLutvJfPbxnYUep\",\"senderip\":\"181.112.79.236\",\"destNXT\":\"423766016895692955\",\"destcoin\":\"DOGE\",\"destamount\":\"100\",\"destcoinaddr\":\"DNyEii2mnvVL1BM1kTZkmKDcf8SFyqAX4Z\"}";
                Signedtx = "0000be25c6009a0225c5fed2690701cf06f267e7c227b1a3c0dfa9c6fc3cdb593b3af6f16d65302f9b30f378f284e10500e1f5050000000000e1f50500000000000000000000000000000000000000000000000000000000000000000000000044b7df75da4a7132c3ee64bd00ad1b56f0e3be56dfefd48e7d56679dc2b857010cd7bf789e290e2c8f4c20653ccb273dce3f248d25fb44ae802289a2c95e3bbf";
                char *teststr = "{\"requestType\":\"subatomic_trade\",\"type\":\"NXTatomicswap\",\"NXT\":\"8989816935121514892\",\"mysighash\":\"8e72bba1bdf481bd6c03e61bceb53dfa642be04cb71f23755b5fa939b24ffa55\",\"mytxbytes\":\"0000be25c6009a0225c5fed2690701cf06f267e7c227b1a3c0dfa9c6fc3cdb593b3af6f16d65302f9b30f378f284e10500e1f5050000000000e1f505000000000000000000000000000000000000000000000000000000000000000000000000\",\"destNXT\":\"423766016895692955\",\"othersighash\":\"422bf4f0a389398589553995f4dcb0b2a0811641ffb7e8222ce3dc3272628065\",\"othertxbytes\":\"00002724c6009a024e5bbad625df3d536fa90b1e6a28c3f5a56e1fcbe34132391c8d3fd7f671cb198c71b555df3ec27c00e1f5050000000000e1f505000000000000000000000000000000000000000000000000000000000000000000000000\"}";
#endif
                cJSON *json = 0;
                if ( 0 && teststr != 0 )
                    json = cJSON_Parse(teststr);
                if ( json != 0 )
                {
                    subatomic_jsonhandler(json);
                    printf("HACK %s\n",cJSON_Print(json));
                    free_json(json);
                } else if ( 0 && teststr != 0 ) printf("error parsing.(%s)\n",teststr);
            }
            // printf("subatomic.%d new idletime %d time %ld microseconds %lld \n",testhack,mp->RTflag,time(0),(long long)microseconds());
            testhack++;
        }
        else if ( parms->mode == NXTPROTOCOL_INIT )
        {
            printf("subatomic NXThandler_info init %d\n",mp->RTflag);
            gp = Global_subatomic = calloc(1,sizeof(*Global_subatomic));
            gp->curl_handle = curl_easy_init();
            gp->curl_handle2 = curl_easy_init();
            strcpy(gp->ipaddr,mp->ipaddr);
            strcpy(gp->NXTADDR,mp->NXTADDR);
            strcpy(gp->NXTACCTSECRET,mp->NXTACCTSECRET);
            if ( mp->accountjson != 0 )
            {
                obj = cJSON_GetObjectItem(mp->accountjson,"enable_bitcoin_broadcast");
                copy_cJSON(buf,obj);
                gp->enable_bitcoin_broadcast = myatoi(buf,2);
                printf("enable_bitcoin_broadcast set to %d\n",gp->enable_bitcoin_broadcast);
            }
            //portnum = SUBATOMIC_PORTNUM;
            // if ( portable_thread_create(subatomic_server,&portnum) == 0 )
            //     printf("ERROR _server_loop\n");
            //variant = SUBATOMIC_VARIANT;
            /*register_variant_handler(SUBATOMIC_VARIANT,process_subatomic_packet,SUBATOMIC_SEND_PUBKEY,sizeof(struct subatomic_packet),sizeof(struct subatomic_packet),0);
             register_variant_handler(SUBATOMIC_VARIANT,process_subatomic_packet,SUBATOMIC_REFUNDTX_NEEDSIG,sizeof(struct subatomic_packet),sizeof(struct subatomic_packet),0);
             register_variant_handler(SUBATOMIC_VARIANT,process_subatomic_packet,SUBATOMIC_REFUNDTX_SIGNED,sizeof(struct subatomic_packet),sizeof(struct subatomic_packet),0);
             register_variant_handler(SUBATOMIC_VARIANT,process_subatomic_packet,SUBATOMIC_FUNDINGTX,sizeof(struct subatomic_packet),sizeof(struct subatomic_packet),0);
             register_variant_handler(SUBATOMIC_VARIANT,process_subatomic_packet,SUBATOMIC_SEND_MICROTX,sizeof(struct subatomic_packet),sizeof(struct subatomic_packet),0);
             if ( pthread_create(malloc(sizeof(pthread_t)),NULL,_server_loop,&variant) != 0 )
             printf("ERROR _server_loop\n");*/
        }
        return(gp);
    }
    else if ( parms->mode == NXTPROTOCOL_AMTXID )
        process_subatomic_AM(gp,parms);
    else if ( parms->mode == NXTPROTOCOL_TYPEMATCH )
        process_subatomic_typematch(gp,parms);
    return(gp);
}

int32_t pubkey_to_256bits(uint8_t *bytes,char *pubkey)
{
    //Bitcoin private keys are 32 bytes, but are often stored in their full OpenSSL-serialized form of 279 bytes. They are serialized as 51 base58 characters, or 64 hex characters.
    //Bitcoin public keys (traditionally) are 65 bytes (the first of which is 0x04). They are typically encoded as 130 hex characters.
    //Bitcoin compressed public keys (as of 0.6.0) are 33 bytes (the first of which is 0x02 or 0x03). They are typically encoded as 66 hex characters.
    //Bitcoin addresses are RIPEMD160(SHA256(pubkey)), 20 bytes. They are typically encoded as 34 base58 characters.
    char zpadded[65]; int32_t i,j,n;
    if ( (n= (int32_t)strlen(pubkey)) > 66 )
    {
        printf("pubkey_to_256bits pubkey.(%s) len.%ld > 66\n",pubkey,strlen(pubkey));
        return(-1);
    }
    if ( pubkey[0] != '0' || (pubkey[1] != '2' && pubkey[1] != '3') )
    {
        printf("pubkey_to_256bits pubkey.(%s) len.%ld unexpected first byte\n",pubkey,strlen(pubkey));
        return(-1);
    }
    for (i=0; i<64; i++)
        zpadded[i] = '0';
    zpadded[64] = 0;
    for (i=66-n,j=2; i<64; i++,j++)
        zpadded[i] = pubkey[j];
    if ( pubkey[j] != 0 )
    {
        printf("pubkey_to_256bits unexpected nonzero at j.%d\n",j);
        return(-1);
    }
    printf("pubkey.(%s) -> zpadded.(%s)\n",pubkey,zpadded);
    decode_hex(bytes,32,zpadded);
    //for (i=0; i<32; i++)
    //    printf("%02x",bytes[i]);
    //printf("\n");
    return(0);
}

#ifdef bitcoin_doesnt_seem_to_support_nonstandard_tx
/* https://bitcointalk.org/index.php?topic=193281.msg3315031#msg3315031
 
 jl777:
 This approach is supposed to be the "holy grail", but only one miner (elgius) even accepts the nonstandard
 transactions for bitcoin network. Probably none of the miners for any of the altcoins do, so basically it
 is a thought experiment that cant possible actually work. I realized this after coding some parts of it up...
 
 description:
 A picks a random number x
 
 A creates TX1: "Pay w BTC to <B's public key> if (x for H(x) known and signed by B) or (signed by A & B)"
 
 A creates TX2: "Pay w BTC from TX1 to <A's public key>, locked 48 hours in the future, signed by A"
 
 A sends TX2 to B
 
 B signs TX2 and returns to A
 
 1) A submits TX1 to the network
 
 B creates TX3: "Pay v alt-coins to <A-public-key> if (x for H(x) known and signed by A) or (signed by A & B)"
 
 B creates TX4: "Pay v alt-coins from TX3 to <B's public key>, locked 24 hours in the future, signed by B"
 
 B sends TX4 to A
 
 A signs TX4 and sends back to B
 
 2) B submits TX3 to the network
 
 3) A spends TX3 giving x
 
 4) B spends TX1 using x
 
 This is atomic (with timeout).  If the process is halted, it can be reversed no matter when it is stopped.
 
 Before 1: Nothing public has been broadcast, so nothing happens
 Between 1 & 2: A can use refund transaction after 48 hours to get his money back
 Between 2 & 3: B can get refund after 24 hours.  A has 24 more hours to get his refund
 After 3: Transaction is completed by 2
 - A must spend his new coin within 24 hours or B can claim the refund and keep his coins
 - B must spend his new coin within 48 hours or A can claim the refund and keep his coins
 
 For safety, both should complete the process with lots of time until the deadlines.
 locktime for A significantly longer than for B
 
 
 ************** NXT variant
 A creates any ref TX to generate fullhash, sighash, utxbytes to give to B, signature is used as x
 Pay w BTC to <B's public key> if (x for H(x) known and signed by B)
 
 B creates and broadcasts "pay NXT to A" referencing fullhash of refTX and sends unsigned txbytes to A, expires in 24 hours
 
 A broadcasts ref TX and gets paid the NXT
 B obtains signature from refTX and spends TX1
 
 
 **************
 OP_IF 0x63
 // Refund for A
 2 <pubkeyA> <pubkeyB> 2 OP_CHECKMULTISIGVERIFY 0xaf
 OP_ELSE 0x67
 // Ordinary claim for B
 OP_HASH160 0xa9 <H(x)> OP_EQUAL 0x87  <pubkeyB/pubkeyA> OP_CHECKSIGVERIFY 0xad
 OP_ENDIF 0x68
 
 0x63 2 <pubkeyA> <pubkeyB> 2 0xaf 0x67 0xa9 <H(x)> 0x87 <pubkeyB> 0xad
 */

#define SCRIPT_OP_IF 0x63
#define SCRIPT_OP_ELSE 0x67
#define SCRIPT_OP_ENDIF 0x68
#define SCRIPT_OP_EQUAL 0x87
#define SCRIPT_OP_HASH160 0xa9
#define SCRIPT_OP_CHECKSIGVERIFY 0xad
#define SCRIPT_OP_CHECKMULTISIGVERIFY 0xaf

int32_t atomictx_addbytes(unsigned char *hex,int32_t n,unsigned char *bytes,int32_t len)
{
    int32_t i;
    for (i=0; i<len; i++)
        hex[n++] = bytes[i];
    return(n);
}

char *create_atomictx_scriptPubkey(int a_or_b,char *pubkeyA,char *pubkeyB,unsigned char *hash160)
{
    char *retstr;
    unsigned char pubkeyAbytes[32],pubkeyBbytes[32],hex[4096];
    int32_t i,n = 0;
    //if ( pubkey_to_256bits(pubkeyAbytes,pubkeyA) < 0 || pubkey_to_256bits(pubkeyBbytes,pubkeyB) < 0 )
    //    return(0);
    decode_hex(pubkeyAbytes,32,pubkeyA);
    decode_hex(pubkeyBbytes,32,pubkeyB);
    hex[n++] = SCRIPT_OP_IF;
    hex[n++] = 2;
    hex[n++] = 32, memcpy(&hex[n],pubkeyAbytes,32), n += 32;
    hex[n++] = 32, memcpy(&hex[n],pubkeyBbytes,32), n += 32;
    hex[n++] = 2;
    hex[n++] = SCRIPT_OP_CHECKMULTISIGVERIFY;
    hex[n++] = SCRIPT_OP_ELSE;
    hex[n++] = SCRIPT_OP_HASH160;
    hex[n++] = 20; memcpy(&hex[n],hash160,20); n += 20;
    hex[n++] = SCRIPT_OP_EQUAL;
    if ( a_or_b == 'a' )
        hex[n++] = 32, memcpy(&hex[n],pubkeyAbytes,32), n += 32;
    else if ( a_or_b == 'b' )
        hex[n++] = 32, memcpy(&hex[n],pubkeyBbytes,32), n += 32;
    else { printf("illegal a_or_b %d\n",a_or_b); return(0); }
    hex[n++] = SCRIPT_OP_CHECKSIGVERIFY;
    hex[n++] = SCRIPT_OP_ENDIF;
    retstr = malloc(n*2+1);
    printf("pubkeyA.(%s) pubkeyB.(%s) ->\n",pubkeyA,pubkeyB);
    for (i=0; i<n; i++)
    {
        retstr[i*2] = hexbyte((hex[i]>>4) & 0xf);
        retstr[i*2 + 1] = hexbyte(hex[i] & 0xf);
        printf("%02x",hex[i]);
    }
    retstr[n*2] = 0;
    printf("-> (%s)\n",retstr);
    return(retstr);
}

void _init_atomictx_half(struct atomictx_half *atomic,int coinid,char *txid,int vout,char *scriptpubkey,uint64_t amount,char *srcaddr,char *srcpubkey,char *destaddr,char *destpubkey)
{
    atomic->amount = amount; atomic->vout = vout; atomic->coinid = coinid;
    atomic->srctxid = txid; atomic->srcscriptpubkey = scriptpubkey;
    atomic->srcaddr = srcaddr; atomic->srcpubkey = srcpubkey;
    atomic->destaddr = destaddr; atomic->destpubkey = destpubkey;
}

void gen_atomictx_initiator(struct atomictx_half *atomic)
{
    int32_t i;
    cJSON *obj,*json;
    char msg[65],txidstr[4096],*jsonstr;
    unsigned char hash160[20],x[32];
    randombytes(x,sizeof(x)/sizeof(*x));
    for (i=0; i<(int32_t)(sizeof(x)/sizeof(*x)); i++)
    {
        msg[i*2] = hexbyte((x[i]>>4) & 0xf);
        msg[i*2 + 1] = hexbyte(x[i] & 0xf);
        printf("%02x ",x[i]);
    }
    msg[64] = 0;
    printf("-> (%s) x[]\n",msg);
    calc_OP_HASH160(hash160,msg);
    atomic->redeemScript = create_atomictx_scriptPubkey('b',atomic->srcpubkey,atomic->destpubkey,hash160);
    if ( atomic->redeemScript != 0 )
    {
        json = cJSON_CreateObject();
        obj = cJSON_CreateString(atomic->srctxid); cJSON_AddItemToObject(json,"txid",obj);
        obj = cJSON_CreateNumber(atomic->vout); cJSON_AddItemToObject(json,"vout",obj);
        obj = cJSON_CreateString(atomic->srcscriptpubkey); cJSON_AddItemToObject(json,"scriptPubKey",obj);
        obj = cJSON_CreateString(atomic->redeemScript); cJSON_AddItemToObject(json,"redeemScript",obj);
        jsonstr = cJSON_Print(json);
        free_json(json);
        sprintf(txidstr,"'[%s]'  '{\"%s\":%lld}'",jsonstr,atomic->srcaddr,(long long)atomic->amount);
        stripwhite(txidstr,strlen(txidstr));
        printf("refundtx.(%s) change last 4 bytes or rawtransaction to future\n",txidstr);
        atomic->refundtx = clonestr(txidstr);
        sprintf(txidstr,"'[%s]'  '{\"%s\":%lld}'",jsonstr,atomic->destaddr,(long long)atomic->amount);
        stripwhite(txidstr,strlen(txidstr));
        printf("claimtx.(%s)\n",txidstr);
        free(jsonstr);
    }
}

void test_atomictx()
{
    struct atomictx_half initiator;
    _init_atomictx_half(&initiator,DOGE_COINID,"48ae45b3741c125ba63c8b98f2adb4dcfd050583734ca57a12d5bc874f8334bd",1,"76a914d43f07a88b9cf437314d5c281f201dd0bb5486e588ac",2*SATOSHIDEN,"DQVMJKcn3yW34nWpqheiNiRAhaY9qFeuER","0279886d0de4bfba245774d0c0a5c062578244b03eec567fe71a02c410b45ca86a","DDNhdeLp4PrwKKN1PeFnUv2pnzvhPLMgCd","03628f9fa04ed2ed83e07f8d3b73a654d1df86e670542008b96b9339a27f1905cb");
    gen_atomictx_initiator(&initiator);
}
#endif

struct subatomic_tx_args
{
    char coinstr[16],destcoinstr[16],NXTaddr[64],otherNXTaddr[64],coinaddr[2][64],destcoinaddr[2][64],otheripaddr[32],mypubkeys[2][128];
    int64_t amount,destamount;
    double myshare;
    int32_t numincr,incr,otherincr;
};

struct subatomic_tx
{
    struct subatomic_tx_args ARGS,otherARGS;
    struct subatomic_halftx myhalf,otherhalf;
    char *claimtxid;
    int64_t lastcontact,myexpectedamount,myreceived,otherexpectedamount,sent_to_other;
    int32_t status,initflag,connsock,refundlockblock,cantsend,type,longerflag,tag,verified;
    int32_t txs_created,other_refundtx_done,myrefundtx_done,other_fundingtx_confirms;
    int32_t other_refundtx_waiting,myrefundtx_waiting,other_fundingtx_waiting,other_micropaytx_waiting;
    unsigned char recvbufs[4][sizeof(struct subatomic_rawtransaction)];
};

char *subatomic_broadcasttx(struct subatomic_halftx *htx,char *bytes,int32_t myincr,int32_t lockedblock)
{
    FILE *fp; char fname[512],*retstr = 0; cJSON *txjson; struct coin777 *coin;
    if ( (coin= coin777_find(htx->coinstr,0)) == 0 )
        return(0);
    txjson = get_decoderaw_json(coin,bytes);
    if ( txjson != 0 )
    {
        retstr = cJSON_Print(txjson);
        printf("broadcasting is disabled for now: (%s) ->\n(%s)\n",bytes,retstr);
        sprintf(fname,"backups/%s_%lld_%s_%s_%lld.%03d_%d",coin->name,(long long)htx->amount,htx->otherNXTaddr,htx->destcoinstr,(long long)htx->destamount,myincr,lockedblock);
        if ( (fp=fopen(fname,"w")) != 0 )
        {
            fprintf(fp,"%s\n%s\n",bytes,retstr);
            fclose(fp);
        }
        free(retstr);
    }
    retstr = 0;
    if ( 0 )
    {
        if ( (retstr= bitcoind_passthru(coin->name,coin->serverport,coin->userpass,"sendrawtransaction",bytes)) != 0 )
        {
            printf("sendrawtransaction returns.(%s)\n",retstr);
        }
    }
    return(retstr);
}

struct subatomic_halftx
{
    int32_t minconfirms;
    int64_t destamount,avail,amount,myamount,otheramount;  // amount = (myamount + otheramount + donation + txfee)
    struct subatomic_rawtransaction funding,refund,micropay;
    struct destbuf funding_scriptPubKey,multisigaddr,redeemScript;
    char fundingtxid[128],refundtxid[128],micropaytxid[128],countersignedrefund[1024],completedmicropay[1024];
    char NXTaddr[64],coinaddr[64],pubkey[128],ipaddr[32],coinstr[16],destcoinstr[16];
    char otherNXTaddr[64],destcoinaddr[64],destpubkey[128],otheripaddr[32];
};

cJSON *get_transaction_json(struct coin777 *coin,char *txid)
{
    char txidstr[512],*transaction = 0; cJSON *json = 0;
    if ( coin == 0 )
        return(0);
    sprintf(txidstr,"\"%s\"",txid);
    if ( (transaction= bitcoind_passthru(coin->name,coin->serverport,coin->userpass,"gettransaction",txidstr)) != 0 && transaction[0] != 0 )
    {
        //printf("got transaction.(%s)\n",transaction);
        json = cJSON_Parse(transaction);
    }
    if ( transaction != 0 )
        free(transaction);
    return(json);
}

int32_t subatomic_tx_confirmed(struct coin777 *coin,char *txid)
{
    cJSON *json; int32_t numconfirmed = -1;
    if ( (json= get_transaction_json(coin,txid)) != 0 )
    {
        numconfirmed = (int32_t)get_cJSON_int(json,"confirmations");
        free_json(json);
    }
    return(numconfirmed);
}

int32_t subatomic_calc_rawoutputs(struct subatomic_halftx *htx,struct coin777 *coin,struct subatomic_rawtransaction *rp,double myshare,char *myaddr,char *otheraddr,char *changeaddr,uint64_t donation)
{
    int32_t n = 0;
    printf("rp->amount %.8f vs %.8f, txfee %.8f, change %.8f vs %.8f (%.8f - %.8f) donation %.8f\n",dstr(rp->amount),dstr(htx->avail),dstr(coin->mgw.txfee),dstr(rp->change),dstr(rp->inputsum - htx->avail),dstr(rp->inputsum),dstr(htx->avail),dstr(donation));
    if ( rp->amount == htx->avail && rp->change == (rp->inputsum - htx->avail - coin->mgw.txfee - donation) )
    {
        if ( changeaddr == 0 )
            changeaddr = coin->donationaddress;
        if ( coin->donationaddress[0] == 0 )
            donation = 0;
        htx->myamount = rp->amount * myshare;
        if ( htx->myamount > rp->amount )
            htx->myamount = rp->amount;
        htx->otheramount = (rp->amount - htx->myamount);
        if ( htx->myamount > 0 && myaddr != 0 )
        {
            if ( otheraddr != 0 && strcmp(myaddr,otheraddr) == 0 )
            {
                htx->myamount += htx->otheramount;
                htx->otheramount = 0;
            }
            if ( changeaddr != 0 && strcmp(myaddr,changeaddr) == 0 )
            {
                htx->myamount += rp->change;
                rp->change = 0;
            }
            safecopy(rp->destaddrs[n],myaddr,sizeof(rp->destaddrs[n]));
            rp->destamounts[n] = htx->myamount;
            //printf("(%s) <- %.8f mine\n",myaddr,dstr(htx->myamount));
            n++;
        }
        if ( otheraddr == 0 && htx->otheramount != 0 )
        {
            printf("no otheraddr, boost donation by %.8f\n",dstr(htx->otheramount));
            donation += htx->otheramount;
            htx->otheramount = 0;
        }
        if ( htx->otheramount > 0 )
        {
            safecopy(rp->destaddrs[n],otheraddr,sizeof(rp->destaddrs[n]));
            rp->destamounts[n] = htx->otheramount;
            //printf("(%s) <- %.8f other\n",otheraddr,dstr(htx->otheramount));
            n++;
        }
        if ( changeaddr == 0 && (rp->change != 0 || strcmp(changeaddr,coin->donationaddress) == 0) )
        {
            printf("no changeaddr, boost donation %.8f\n",dstr(rp->change));
            donation += rp->change;
            rp->change = 0;
        }
        if ( rp->change > 0 )
        {
            safecopy(rp->destaddrs[n],changeaddr,sizeof(rp->destaddrs[n]));
            rp->destamounts[n] = rp->change;
            //printf("(%s) <- %.8f change\n",changeaddr,dstr(rp->change));
            n++;
        }
        if ( donation > 0 && coin->donationaddress[0] != 0 )
        {
            safecopy(rp->destaddrs[n],coin->donationaddress,sizeof(rp->destaddrs[n]));
            rp->destamounts[n] = donation;
            //printf("(%s) <- %.8f donation\n",coin->donationaddress,dstr(donation));
            n++;
        }
    }
    else printf("error myshare %.6f %.8f -> %s, other %.8f -> %s, change %.8f -> %s, donation %.8f -> %s \n",myshare,dstr(htx->myamount),myaddr,dstr(htx->otheramount),otheraddr,dstr(rp->change),changeaddr,dstr(donation),coin->donationaddress);
    rp->numoutputs = n;
    //printf("numoutputs.%d\n",n);
    return(n);
}



cJSON *get_rawtransaction_json(struct coin777 *coin,char *txid)
{
    char txidstr[512],*rawtransaction=0; cJSON *json = 0;
    if ( coin == 0 )
        return(0);
    sprintf(txidstr,"\"%s\"",txid);
    if ( (rawtransaction= bitcoind_passthru(coin->name,coin->serverport,coin->userpass,"getrawtransaction",txidstr)) != 0 && rawtransaction[0] != 0 )
        json = get_decoderaw_json(coin,rawtransaction);
    else printf("error with getrawtransaction %s %s\n",coin->name,txid);
    if ( rawtransaction != 0 )
        free(rawtransaction);
    return(json);
}

int32_t generate_multisigaddr(struct destbuf *multisigaddr,struct destbuf *redeemScript,struct coin777 *coin,char *serverport,char *userpass,int32_t addmultisig,char *params)
{
    char addr[1024],*retstr; cJSON *json,*redeemobj,*msigobj; int32_t flag = 0;
    if ( addmultisig != 0 )
    {
        if ( (retstr= bitcoind_passthru(coin->name,serverport,userpass,"addmultisigaddress",params)) != 0 )
        {
            strcpy(multisigaddr->buf,retstr);
            free(retstr);
            sprintf(addr,"\"%s\"",multisigaddr->buf);
            if ( (retstr= bitcoind_passthru(coin->name,serverport,userpass,"validateaddress",addr)) != 0 )
            {
                json = cJSON_Parse(retstr);
                if ( json == 0 ) printf("Error before: [%s]\n",cJSON_GetErrorPtr());
                else
                {
                    if ( (redeemobj= cJSON_GetObjectItem(json,"hex")) != 0 )
                    {
                        copy_cJSON(redeemScript,redeemobj);
                        flag = 1;
                    } else printf("missing redeemScript in (%s)\n",retstr);
                    free_json(json);
                }
                free(retstr);
            }
        } else printf("error creating multisig address\n");
    }
    else
    {
        if ( (retstr= bitcoind_passthru(coin->name,serverport,userpass,"createmultisig",params)) != 0 )
        {
            json = cJSON_Parse(retstr);
            if ( json == 0 ) printf("Error before: [%s]\n",cJSON_GetErrorPtr());
            else
            {
                if ( (msigobj= cJSON_GetObjectItem(json,"address")) != 0 )
                {
                    if ( (redeemobj= cJSON_GetObjectItem(json,"redeemScript")) != 0 )
                    {
                        copy_cJSON(multisigaddr,msigobj);
                        copy_cJSON(redeemScript,redeemobj);
                        flag = 1;
                    } else printf("missing redeemScript in (%s)\n",retstr);
                } else printf("multisig missing address in (%s) params.(%s)\n",retstr,params);
                free_json(json);
            }
            free(retstr);
        } else printf("error issuing createmultisig.(%s)\n",params);
    }
    return(flag);
}

char *createmultisig_json_params(struct pubkey_info *pubkeys,int32_t m,int32_t n,char *acctparm)
{
    int32_t i; char *paramstr = 0; cJSON *array,*mobj,*keys,*key;
    keys = cJSON_CreateArray();
    for (i=0; i<n; i++)
    {
        key = cJSON_CreateString(pubkeys[i].pubkey);
        cJSON_AddItemToArray(keys,key);
    }
    mobj = cJSON_CreateNumber(m);
    array = cJSON_CreateArray();
    if ( array != 0 )
    {
        cJSON_AddItemToArray(array,mobj);
        cJSON_AddItemToArray(array,keys);
        if ( acctparm != 0 )
            cJSON_AddItemToArray(array,cJSON_CreateString(acctparm));
        paramstr = cJSON_Print(array);
        _stripwhite(paramstr,' ');
        free_json(array);
    }
    printf("createmultisig_json_params.(%s)\n",paramstr);
    return(paramstr);
}

int32_t subatomic_createmultisig(struct destbuf *multisigaddr,struct destbuf *redeemScript,struct coin777 *coin,char *serverport,char *userpass,int32_t use_addmultisig,int32_t m,int32_t n,char *account,struct pubkey_info *pubkeys)
{
    int32_t flag = 0; char *params;
    if ( (params = createmultisig_json_params(pubkeys,m,n,(use_addmultisig != 0) ? account : 0)) != 0 )
    {
        flag = generate_multisigaddr(multisigaddr,redeemScript,coin,serverport,userpass,use_addmultisig,params);
        free(params);
    } else printf("error generating msig params\n");
    return(flag);
}

int32_t subatomic_gen_multisig(struct subatomic_tx *atx,struct subatomic_halftx *htx)
{
    char coinaddrs[3][64]; struct destbuf pubkeys[3]; struct coin777 *coin; struct pubkey_info pubkeydata[16];
    if ( (coin= coin777_find(htx->coinstr,0)) == 0 )
        return(-1);
    if ( htx->coinaddr[0] != 0 && htx->pubkey[0] != 0 && atx->otherhalf.coinaddr[0] != 0 && atx->otherhalf.pubkey[0] != 0 )
    {
        safecopy(coinaddrs[0],htx->coinaddr,sizeof(coinaddrs[0]));
        safecopy(pubkeys[0].buf,htx->pubkey,sizeof(pubkeys[0]));
        safecopy(coinaddrs[1],atx->otherhalf.coinaddr,sizeof(coinaddrs[1]));
        safecopy(pubkeys[1].buf,atx->otherhalf.pubkey,sizeof(pubkeys[1]));
        strcpy(pubkeydata[0].pubkey,pubkeys[0].buf);
        strcpy(pubkeydata[1].pubkey,pubkeys[1].buf);
        subatomic_createmultisig(&htx->multisigaddr,&htx->redeemScript,coin,coin->serverport,coin->userpass,coin->mgw.use_addmultisig,2,2,0,pubkeydata);
        //#ifdef DEBUG_MODE
        if ( htx->multisigaddr.buf[0] != 0 )
            get_pubkey(&pubkeys[2],coin->name,coin->serverport,coin->userpass,htx->multisigaddr.buf);
        //#endif
        return(htx->multisigaddr.buf[0] != 0);
    } else printf("cant gen multisig %d %d %d %d\n",htx->coinaddr[0],htx->pubkey[0],atx->otherhalf.coinaddr[0],atx->otherhalf.pubkey[0]);
    return(-1);
}
char *subatomic_create_fundingtx(struct subatomic_halftx *htx,int64_t amount,int32_t lockgap,char *changeaddr)
{
    //2) Create and sign but do not broadcast a transaction (T1) that sets up a payment of amount to funding acct
    struct subatomic_unspent_tx *ups; struct coin777 *coin; char *txid,*retstr = 0; uint64_t total,donation; int32_t num,check_locktime,locktime;
    if ( (coin= coin777_find(htx->coinstr,0)) == 0 )
    {
        printf("subatomic_create_fundingtx: cant find (%s)\n",htx->coinstr);
        return(0);
    }
    if ( (locktime= lockgap) != 0 )
        locktime = (uint32_t)(time(NULL) + lockgap);
    donation = subatomic_donation(coin,amount);
    printf("CREATE FUNDING TX.(%s) for %.8f -> %s locktime.%u donation %.8f\n",coin->name,dstr(amount),htx->coinaddr,locktime,dstr(donation));
    memset(&htx->funding,0,sizeof(htx->funding));
    if ( (ups= gather_unspents(&total,&num,coin,0)) && num != 0 )
    {
        if ( subatomic_calc_rawinputs(coin,&htx->funding,amount,ups,num,donation) >= amount )
        {
            htx->avail = amount;
            if ( subatomic_calc_rawoutputs(htx,coin,&htx->funding,1.,htx->multisigaddr.buf,0,changeaddr,donation) > 0 )
            {
                if ( (retstr= subatomic_gen_rawtransaction(htx->multisigaddr.buf,coin,&htx->funding,htx->coinaddr,locktime,locktime==0?0xffffffff:(uint32_t)time(NULL),0)) != 0 )
                {
                    txid = subatomic_decodetxid(0,&htx->funding_scriptPubKey,&check_locktime,coin,htx->funding.rawtransaction,htx->multisigaddr.buf);
                    printf("txid.%s fundingtx %.8f -> %.8f %s completed.%d locktimes %d vs %d\n",txid,dstr(amount),dstr(htx->funding.amount),retstr,htx->funding.completed,check_locktime,locktime);
                }
            }
        }
    }
    if ( ups != 0 )
        free(ups);
    return(retstr);
}

void subatomic_set_unspent_tx0(struct subatomic_unspent_tx *up,struct subatomic_halftx *htx)
{
    memset(up,0,sizeof(*up));
    up->vout = 0;
    up->amount = htx->avail;
    safecopy(up->txid.buf,htx->fundingtxid,sizeof(up->txid));
    safecopy(up->address.buf,htx->multisigaddr.buf,sizeof(up->address));
    safecopy(up->scriptPubKey.buf,htx->funding_scriptPubKey.buf,sizeof(up->scriptPubKey));
    safecopy(up->redeemScript.buf,htx->redeemScript.buf,sizeof(up->redeemScript));
}

char *subatomic_create_paytx(struct subatomic_rawtransaction *rp,char *signcoinaddr,struct subatomic_halftx *htx,char *othercoinaddr,int32_t locktime,double myshare,int32_t seqid)
{
    struct subatomic_unspent_tx U; int32_t check_locktime; int64_t value,donation; char *txid = 0; struct coin777 *coin = coin777_find(htx->coinstr,0);
    if ( coin == 0 )
        return(0);
    //struct subatomic_unspent_tx *gather_unspents(int32_t *nump,struct coin777 *coin,char *coinaddr)
    printf("create paytx %s\n",coin->name);
    subatomic_set_unspent_tx0(&U,htx);
    donation = subatomic_donation(coin,htx->avail);
    rp->numinputs = 0;
    rp->inputs[rp->numinputs++] = U;
    rp->amount = (htx->avail - coin->mgw.txfee - donation);
    rp->change = 0;
    rp->inputsum = htx->avail;
    // jl777: make sure sequence number is not -1!!
    if ( subatomic_calc_rawoutputs(htx,coin,rp,myshare,htx->coinaddr,othercoinaddr,0,donation) > 0 )
    {
        subatomic_gen_rawtransaction(htx->multisigaddr.buf,coin,rp,signcoinaddr,locktime,seqid,0);
        txid = subatomic_decodetxid(&value,0,&check_locktime,coin,rp->rawtransaction,htx->coinaddr);
        if ( check_locktime != locktime )
        {
            printf("check_locktime.%d vs locktime.%d\n",check_locktime,locktime);
            return(0);
        }
        printf("created paytx %.8f to %s value %.8f, locktime.%d\n",dstr(value),htx->coinaddr,dstr(value),locktime);
    }
    return(txid);
}

int32_t subatomic_gen_pubkeys(struct subatomic_tx *atx,struct subatomic_halftx *htx)
{
    char coinaddrs[3][128],pubkeys[3][256],*coinstr; int32_t i,flag=0; struct destbuf pubkey; char *coinaddr,*pubkeystr; struct coin777 *coin;
    if ( htx->multisigaddr.buf[0] == 0 )
    {
        memset(coinaddrs,0,sizeof(coinaddrs));
        memset(pubkeys,0,sizeof(pubkeys));
        for (i=0; i<2; i++)
        {
            if ( i == 0 )
            {
                pubkeystr = atx->myhalf.pubkey;
                coinaddr = atx->myhalf.coinaddr;
                coinstr = atx->ARGS.coinstr;
            }
            else
            {
                pubkeystr = atx->myhalf.destpubkey;
                coinaddr = atx->myhalf.destcoinaddr;
                coinstr = atx->ARGS.destcoinstr;
            }
            if ( pubkeystr[0] == 0 && (coin= coin777_find(coinstr,0)) != 0 )
            {
                get_pubkey(&pubkey,coin->name,coin->serverport,coin->userpass,coinaddr);
                pubkeystr = pubkey.buf;
            }
            if ( pubkeystr[0] != 0 )
            {
                flag++;
                safecopy(atx->ARGS.mypubkeys[i],pubkey.buf,sizeof(atx->ARGS.mypubkeys[i]));
                printf("i.%d gen pubkey %s (%s) for (%s)\n",i,coinstr,pubkey.buf,coinaddr);
            }
            else
            {
                printf("i.%d cant generate %s pubkey for addr.%s\n",i,coinstr,coinaddr);
                //return(-1);
            }
        }
    }
    return(flag);
}


void subatomic_set_unspent_tx0(struct subatomic_unspent_tx *up,struct subatomic_halftx *htx)
{
    memset(up,0,sizeof(*up));
    up->vout = 0;
    up->amount = htx->avail;
    safecopy(up->txid.buf,htx->fundingtxid,sizeof(up->txid));
    safecopy(up->address.buf,htx->multisigaddr.buf,sizeof(up->address));
    safecopy(up->scriptPubKey.buf,htx->funding_scriptPubKey.buf,sizeof(up->scriptPubKey));
    safecopy(up->redeemScript.buf,htx->redeemScript.buf,sizeof(up->redeemScript));
}

char *subatomic_create_paytx(struct subatomic_rawtransaction *rp,char *signcoinaddr,struct subatomic_halftx *htx,char *othercoinaddr,int32_t locktime,double myshare,int32_t seqid)
{
    struct subatomic_unspent_tx U; int32_t check_locktime; int64_t value,donation; char *txid = 0; struct coin777 *coin = coin777_find(htx->coinstr,0);
    if ( coin == 0 )
        return(0);
    //struct subatomic_unspent_tx *gather_unspents(int32_t *nump,struct coin777 *coin,char *coinaddr)
    printf("create paytx %s\n",coin->name);
    subatomic_set_unspent_tx0(&U,htx);
    donation = subatomic_donation(coin,htx->avail);
    rp->numinputs = 0;
    rp->inputs[rp->numinputs++] = U;
    rp->amount = (htx->avail - coin->mgw.txfee - donation);
    rp->change = 0;
    rp->inputsum = htx->avail;
    // jl777: make sure sequence number is not -1!!
    if ( subatomic_calc_rawoutputs(htx,coin,rp,myshare,htx->coinaddr,othercoinaddr,0,donation) > 0 )
    {
        subatomic_gen_rawtransaction(htx->multisigaddr.buf,coin,rp,signcoinaddr,locktime,seqid,0);
        txid = subatomic_decodetxid(&value,0,&check_locktime,coin,rp->rawtransaction,htx->coinaddr);
        if ( check_locktime != locktime )
        {
            printf("check_locktime.%d vs locktime.%d\n",check_locktime,locktime);
            return(0);
        }
        printf("created paytx %.8f to %s value %.8f, locktime.%d\n",dstr(value),htx->coinaddr,dstr(value),locktime);
    }
    return(txid);
}

int32_t subatomic_gen_pubkeys(struct subatomic_tx *atx,struct subatomic_halftx *htx)
{
    char coinaddrs[3][128],pubkeys[3][256],*coinstr; int32_t i,flag=0; struct destbuf pubkey; char *coinaddr,*pubkeystr; struct coin777 *coin;
    if ( htx->multisigaddr.buf[0] == 0 )
    {
        memset(coinaddrs,0,sizeof(coinaddrs));
        memset(pubkeys,0,sizeof(pubkeys));
        for (i=0; i<2; i++)
        {
            if ( i == 0 )
            {
                pubkeystr = atx->myhalf.pubkey;
                coinaddr = atx->myhalf.coinaddr;
                coinstr = atx->ARGS.coinstr;
            }
            else
            {
                pubkeystr = atx->myhalf.destpubkey;
                coinaddr = atx->myhalf.destcoinaddr;
                coinstr = atx->ARGS.destcoinstr;
            }
            if ( pubkeystr[0] == 0 && (coin= coin777_find(coinstr,0)) != 0 )
            {
                get_pubkey(&pubkey,coin->name,coin->serverport,coin->userpass,coinaddr);
                pubkeystr = pubkey.buf;
            }
            if ( pubkeystr[0] != 0 )
            {
                flag++;
                safecopy(atx->ARGS.mypubkeys[i],pubkey.buf,sizeof(atx->ARGS.mypubkeys[i]));
                printf("i.%d gen pubkey %s (%s) for (%s)\n",i,coinstr,pubkey.buf,coinaddr);
            }
            else
            {
                printf("i.%d cant generate %s pubkey for addr.%s\n",i,coinstr,coinaddr);
                //return(-1);
            }
        }
    }
    return(flag);
}


// Alice sends a future transaction to Bob's address
// Bob sends a phased tx to Alice that triggers off of his pubkey
// When Bob spends the tx, the pubkey triggers his phased tx to bob.
#define SUBATOMIC_SEND_PUBKEY 'P'
#define SUBATOMIC_REFUNDTX_NEEDSIG 'R'
#define SUBATOMIC_REFUNDTX_SIGNED 'S'
#define SUBATOMIC_FUNDINGTX 'F'
#define SUBATOMIC_SEND_MICROTX 'T'
#define SUBATOMIC_SEND_ATOMICTX 'A'

int32_t share_tx(struct subatomic_rawtransaction *rp,int32_t funcid)
{
    /*uint32_t TCRC;
     int32_t incr,size;
     char i,n,jsonstr[512];
     incr = (int32_t)(SYNC_FRAGSIZE - sizeof(struct json_AM) - 60);
     size = (sizeof(*rp) - sizeof(rp->inputs) + sizeof(rp->inputs[0])*rp->numinputs);
     n = size / incr;
     if ( (size / incr) != 0 )
     n++;
     TCRC = _crc32(0,rp,sizeof(struct subatomic_rawtransaction));
     for (i=0; i<n; i++)
     {
     sprintf(jsonstr,"{\"TCRC\":%u,\"starti\":%d,\"i\":%d,\"n\":%d,\"incr\":%d}",TCRC,startfragi,i,n,incr);
     send_to_NXTaddr(&np->localcrcs[startfragi+i],np->H.NXTaddr,startfragi+i,SUBATOMIC_SIG,funcid,jsonstr,(void *)((long)rp+i*incr),incr);
     }*/
    return(0);
}

int32_t share_refundtx(struct subatomic_rawtransaction *rp)
{
    return(share_tx(rp,SUBATOMIC_REFUNDTX_NEEDSIG));
}

int32_t share_other_refundtx(struct subatomic_rawtransaction *rp)
{
    return(share_tx(rp,SUBATOMIC_REFUNDTX_SIGNED));
}

int32_t share_fundingtx(struct subatomic_rawtransaction *rp)
{
    return(share_tx(rp,SUBATOMIC_FUNDINGTX));
}

int32_t share_micropaytx(struct subatomic_rawtransaction *rp)
{
    return(share_tx(rp,SUBATOMIC_SEND_MICROTX));
}

int32_t subatomic_ensure_txs(struct subatomic_halftx *otherhalf,struct subatomic_halftx *htx,int32_t locktime)
{
    char *fundingtxid,*refundtxid,*micropaytxid; struct coin777 *coin; int32_t blocknum = 0;
    if ( (coin= coin777_find(htx->coinstr,0)) == 0 || htx->multisigaddr.buf[0] == 0 )
    {
        printf("cant get valid daemon for %s or no xferaddr.%s\n",coin->name,htx->multisigaddr.buf);
        return(-1);
    }
    if ( locktime != 0 )
    {
        coin->ramchain.RTblocknum = _get_RTheight(&coin->ramchain.lastgetinfo,coin->name,coin->serverport,coin->userpass,coin->ramchain.RTblocknum);
        if ( (blocknum= coin->ramchain.RTblocknum) == 0 )
        {
            printf("cant get valid blocknum for %s\n",coin->name);
            return(-1);
        }
        blocknum += (locktime/coin->estblocktime) + 1;
    }
    if ( htx->fundingtxid == 0 )
    {
        //printf("create funding TX\n");
        if ( (fundingtxid= subatomic_create_fundingtx(htx,htx->amount,SUBATOMIC_LOCKTIME,htx->coinaddr)) == 0 )
            return(-1);
        safecopy(htx->fundingtxid,fundingtxid,sizeof(htx->fundingtxid));
        free(fundingtxid);
        htx->avail = htx->myamount;
    }
    if ( htx->refundtxid == 0 )
    {
        // printf("create refund TX\n");
        if ( (refundtxid= subatomic_create_paytx(&htx->refund,0,htx,otherhalf->coinaddr,blocknum,1.,SUBATOMIC_STARTING_SEQUENCEID-1)) == 0 )
            return(-1);
        safecopy(htx->refundtxid,refundtxid,sizeof(htx->refundtxid));
        free(refundtxid);
        //printf("created refundtx.(%s)\n",htx->refundtxid);
    }
    if ( htx->micropaytxid == 0 )
    {
        //printf("create micropay TX\n");
        if ( (micropaytxid= subatomic_create_paytx(&htx->micropay,htx->coinaddr,htx,otherhalf->coinaddr,0,1.,SUBATOMIC_STARTING_SEQUENCEID)) == 0 )
            return(-1);
        safecopy(htx->micropaytxid,micropaytxid,sizeof(htx->micropaytxid));
        free(micropaytxid);
    }
    return(blocknum);
}

int32_t verify_txs_created(struct subatomic_tx *atx)
{
    struct subatomic_halftx *htx = &atx->myhalf;
    if ( atx->other_refundtx_done == 0 )
        printf("multisig addrs %d %d %d %d | refundtx.%d multisigaddr.%s\n",atx->myhalf.coinaddr[0],atx->myhalf.pubkey[0],atx->otherhalf.coinaddr[0],atx->otherhalf.pubkey[0],atx->other_refundtx_done,atx->myhalf.multisigaddr.buf);
    //if ( atx->other_refundtx_done == 0 )
    //    atx->myrefund_fragi = share_pubkey(np,1,htx->destcoinid,htx->destcoinaddr,htx->destpubkey);
    if ( atx->myhalf.multisigaddr.buf[0] == 0 )
    {
        if ( atx->otherhalf.pubkey[0] != 0 )
        {
            printf(">>>> multisig addrs %d %d %d %d\n",htx->coinaddr[0],htx->pubkey[0],atx->otherhalf.coinaddr[0],atx->otherhalf.pubkey[0]);
            subatomic_gen_multisig(atx,htx);
            if ( atx->myhalf.multisigaddr.buf[0] != 0 )
                printf("generated multisig.(%s)\n",atx->myhalf.multisigaddr.buf);
        }
    }
    if ( atx->myhalf.multisigaddr.buf[0] == 0 )
        return(0);
    if ( atx->txs_created == 0 && subatomic_ensure_txs(&atx->otherhalf,htx,(atx->longerflag * SUBATOMIC_LOCKTIME)) < 0 )
    {
        printf("warning: cant create required transactions, probably lack of funds\n");
        return(-1);
    }
    //if ( atx->other_refund_fragi == 0 )
    //    atx->other_refund_fragi = share_refundtx(np,&htx->refund,atx->myrefund_fragi);
    return(1);
}

int32_t update_other_refundtxdone(struct subatomic_tx *atx)
{
    int32_t lockedblock; int64_t value; struct coin777 *coin; struct subatomic_rawtransaction *rp; struct subatomic_halftx *htx = &atx->myhalf;
    if ( (coin= coin777_find(htx->coinstr,0)) != 0 )
    {
        rp = &atx->otherhalf.refund;
        if ( atx->other_refundtx_done == 0 )
        {
            if ( atx->other_refundtx_waiting != 0 )
            {
                if ( subatomic_signtx(0,&lockedblock,&value,htx->destcoinaddr,rp->signedtransaction,sizeof(rp->signedtransaction),coin777_find(htx->destcoinstr,0),rp,rp->rawtransaction) == 0 )
                {
                    printf("warning: error signing other's NXT.%s refund\n",htx->otherNXTaddr);
                    return(0);
                }
                //atx->funding_fragi = share_other_refundtx(np,rp,atx->other_refund_fragi);
                atx->other_refundtx_done = 1;
                printf("other refundtx done\n");
            }
        }
    }
    return(atx->other_refundtx_done);
}

int32_t subatomic_validate_refund(struct subatomic_tx *atx,struct subatomic_halftx *htx)
{
    struct coin777 *coin = coin777_find(htx->coinstr,0); int64_t value; int32_t lockedblock;
    if ( coin == 0 )
        return(-1);
    printf("validate refund\n");
    if ( subatomic_signtx(atx->myhalf.multisigaddr.buf,&lockedblock,&value,htx->coinaddr,htx->countersignedrefund,sizeof(htx->countersignedrefund),coin,&htx->refund,htx->refund.signedtransaction) == 0 )
    {
        printf("error signing refund\n");
        return(-1);
    }
    printf("refund signing completed.%d\n",htx->refund.completed);
    if ( htx->refund.completed <= 0 )
        return(-1);
    printf(">>>>>>>>>>>>>>>>>>>>> refund at %d is locked! txid.%s completed %d %.8f -> %s\n",lockedblock,htx->refund.txid,htx->refund.completed,dstr(value),htx->coinaddr);
    atx->status = SUBATOMIC_HAVEREFUND;
    return(0);
}

int32_t update_my_refundtxdone(struct subatomic_tx *atx)
{
    struct subatomic_halftx *htx = &atx->myhalf;
    if ( atx->myrefundtx_done == 0 )
    {
        if ( atx->myrefundtx_waiting != 0 )
        {
            if ( subatomic_validate_refund(atx,htx) < 0 )
            {
                printf("warning: other side NXT.%s returned invalid signed refund\n",htx->otherNXTaddr);
                return(0);
            }
            subatomic_broadcasttx(&atx->myhalf,atx->myhalf.countersignedrefund,0,atx->refundlockblock);
            atx->myrefundtx_done = 1;
            printf("myrefund done\n");
        }
    }
    return(atx->myrefundtx_done);
}

int32_t update_fundingtx(struct subatomic_tx *atx)
{
    //struct subatomic_halftx *htx = &atx->myhalf;
    /*if ( atx->type == SUBATOMIC_TYPE || atx->type == SUBATOMIC_FORNXT_TYPE )
     {
     if ( atx->microtx_fragi == 0 || atx->other_fundingtx_confirms == 0 )
     atx->microtx_fragi = share_fundingtx(np,&htx->funding,atx->funding_fragi);
     }
     if ( atx->type == NXTFOR_SUBATOMIC_TYPE || atx->type == SUBATOMIC_TYPE )*/
    {
        if ( atx->other_fundingtx_confirms == 0 )
        {
            if ( atx->other_fundingtx_waiting != 0 )
            {
                subatomic_broadcasttx(&atx->otherhalf,atx->otherhalf.funding.signedtransaction,0,0);
                atx->other_fundingtx_confirms = atx->otherhalf.minconfirms+1;
                printf("broadcast other funding\n");
            }
        }
    }
    //else atx->other_fundingtx_confirms = get_numconfirms(&atx->otherhalf);  // jl777: critical to wait for both funding tx to get confirmed
    return(atx->other_fundingtx_confirms);
}

double subatomic_calc_incr(struct subatomic_halftx *htx,int64_t value,int64_t den,int32_t numincr)
{
    //printf("value %.8f/%.8f numincr.%d -> %.6f\n",dstr(value),dstr(den),numincr,(((double)value/den) * numincr));
    return((((double)value/den) * numincr));
}

int32_t subatomic_validate_micropay(struct subatomic_tx *atx,char *skipaddr,char *destbytes,int32_t max,int64_t *valuep,struct subatomic_rawtransaction *rp,struct subatomic_halftx *htx,struct coin777 *srccoin,int64_t srcamount,int32_t numincr,char *refcoinaddr)
{
    int64_t value; int32_t lockedblock; struct coin777 *coin = coin777_find(htx->coinstr,0);
    if ( valuep != 0 )
        *valuep = 0;
    if ( coin == 0 )
        return(-1);
    if ( subatomic_signtx(skipaddr,&lockedblock,&value,refcoinaddr,destbytes,max,coin,rp,rp->signedtransaction) == 0 )
        return(-1);
    if ( valuep != 0 )
        *valuep = value;
    if ( rp->completed <= 0 )
        return(-1);
    //printf("micropay is updated txid.%s completed %d %.8f -> %s, lockedblock.%d\n",rp->txid,rp->completed,dstr(value),htx->coinaddr,lockedblock);
    return(lockedblock);
}

int32_t process_microtx(struct subatomic_tx *atx,struct subatomic_rawtransaction *rp,int32_t incr,int32_t otherincr)
{
    int64_t value;
    if ( subatomic_validate_micropay(atx,0,atx->otherhalf.completedmicropay,(int32_t)sizeof(atx->otherhalf.completedmicropay),&value,rp,&atx->otherhalf,coin777_find(atx->ARGS.destcoinstr,0),atx->ARGS.destamount,atx->ARGS.numincr,atx->myhalf.destcoinaddr) < 0 )
    {
        printf("Error validating micropay from NXT.%s %s %s\n",atx->ARGS.otherNXTaddr,atx->myhalf.destcoinstr,atx->myhalf.destcoinaddr);
        //subatomic_sendabort(atx);
        //atx->status = SUBATOMIC_ABORTED;
        //if ( incr > 1 )
        //    atx->claimtxid = subatomic_broadcasttx(&atx->otherhalf,atx->otherhalf.completedmicropay,0,0);
        return(-1);
    }
    else
    {
        atx->myreceived = value;
        otherincr = subatomic_calc_incr(&atx->otherhalf,value,atx->myexpectedamount,atx->ARGS.numincr);
    }
    if ( otherincr == atx->ARGS.numincr || value == atx->myhalf.destamount )
    {
        printf("TX complete!\n");
        atx->claimtxid = subatomic_broadcasttx(&atx->otherhalf,atx->otherhalf.completedmicropay,0,0);
        atx->status = SUBATOMIC_COMPLETED;
    }
    printf("[%5.2f%%] Received %12.8f of %12.8f | Sent %12.8f of %12.8f\n",100.*(double)atx->myreceived/atx->myexpectedamount,dstr(atx->myreceived),dstr(atx->myexpectedamount),dstr(atx->sent_to_other),dstr(atx->otherexpectedamount));
    
    //printf("incr.%d of %d, otherincr.%d %.8f %.8f \n",incr,atx->ARGS.numincr,otherincr,dstr(value),dstr(atx->myexpectedamount));
    return(otherincr);
}

int64_t subatomic_calc_micropay(struct subatomic_tx *atx,struct subatomic_halftx *htx,char *othercoinaddr,double myshare,int32_t seqid)
{
    struct subatomic_unspent_tx U; uint64_t donation; struct coin777 *coin = coin777_find(htx->coinstr,0); struct subatomic_rawtransaction *rp = &htx->micropay;
    if ( coin == 0 )
        return(0);
    subatomic_set_unspent_tx0(&U,htx);
    donation = subatomic_donation(coin,htx->avail);
    rp->numinputs = 0;
    rp->inputs[rp->numinputs++] = U;
    rp->amount = (htx->avail - coin->mgw.txfee - donation);
    rp->change = 0;
    rp->inputsum = htx->avail;
    //printf("subatomic_sendincr myshare %f seqid.%d\n",myshare,seqid);
    if ( subatomic_calc_rawoutputs(htx,coin,rp,myshare,htx->coinaddr,othercoinaddr,coin->donationaddress,donation) > 0 )
    {
        subatomic_gen_rawtransaction(htx->multisigaddr.buf,coin,rp,htx->coinaddr,0,seqid,0);
        return(htx->otheramount);
    }
    return(-1);
}

int32_t calc_micropay(struct subatomic_tx *atx)
{
    if ( (atx->sent_to_other= subatomic_calc_micropay(atx,&atx->myhalf,atx->otherhalf.coinaddr,atx->ARGS.myshare,SUBATOMIC_STARTING_SEQUENCEID+atx->ARGS.incr)) > 0 )
    {
        // printf("micropay.(%s)\n",htx->micropay.signedtransaction);
        //printf("send micropay share incr.%d otherincr.%d totalfragis.%d\n",atx->ARGS.incr,atx->ARGS.otherincr,totalfragis);
        return(0);
    }
    return(-1);
}

int32_t send_micropay(struct subatomic_tx *atx)
{
    share_micropaytx(&atx->myhalf.micropay);
    return(0);
}

int32_t update_otherincr(struct subatomic_tx *atx)
{
    return(process_microtx(atx,&atx->otherhalf.micropay,atx->ARGS.incr,atx->ARGS.otherincr));
}

void init_subatomic_halftx(struct subatomic_halftx *htx,struct subatomic_tx *atx)
{
    safecopy(htx->NXTaddr,atx->ARGS.NXTaddr,sizeof(htx->NXTaddr));
    safecopy(htx->otherNXTaddr,atx->ARGS.otherNXTaddr,sizeof(htx->otherNXTaddr));
    safecopy(htx->ipaddr,SUPERNET.myipaddr,sizeof(htx->ipaddr));
    if ( atx->ARGS.otheripaddr[0] != 0 )
        safecopy(htx->otheripaddr,atx->ARGS.otheripaddr,sizeof(htx->otheripaddr));
}

int32_t init_subatomic_tx(struct subatomic_tx *atx,int32_t flipped)
{
    struct coin777 *coin,*destcoin;
    if ( (coin= coin777_find(atx->ARGS.coinstr,1)) == 0 || (destcoin= coin777_find(atx->ARGS.destcoinstr,1)) == 0 )
    {
        printf("coin.(%s) or (%s) not found\n",atx->ARGS.coinstr,atx->ARGS.destcoinstr);
        return(-1);
    }
    if ( atx->ARGS.coinaddr[flipped][0] != 0 && atx->ARGS.destcoinaddr[flipped][0] != 0 ) // atx->ARGS.otherNXTaddr[0] != 0 &&
    {
        if ( atx->ARGS.amount != 0 && atx->ARGS.destamount != 0 && strcmp(atx->ARGS.coinstr,atx->ARGS.destcoinstr) != 0 )
        {
            if ( atx->longerflag == 0 )
            {
                atx->myhalf.minconfirms = coin->minconfirms;
                atx->otherhalf.minconfirms = destcoin->minconfirms;
                atx->ARGS.numincr = SUBATOMIC_DEFAULTINCR;
                atx->longerflag = 1;
                if ( (calc_nxt64bits(atx->ARGS.NXTaddr) % 666) > (calc_nxt64bits(atx->ARGS.otherNXTaddr) % 666) )
                    atx->longerflag = 2;
            }
            init_subatomic_halftx(&atx->myhalf,atx);
            init_subatomic_halftx(&atx->otherhalf,atx);
            atx->connsock = -1;
            if ( flipped == 0 )
            {
                strcpy(atx->myhalf.coinstr,atx->ARGS.coinstr); strcpy(atx->myhalf.destcoinstr,atx->ARGS.destcoinstr);
                atx->myhalf.amount = atx->ARGS.amount; atx->myhalf.destamount = atx->ARGS.destamount;
                safecopy(atx->myhalf.coinaddr,atx->ARGS.coinaddr[0],sizeof(atx->myhalf.coinaddr));
                safecopy(atx->myhalf.destcoinaddr,atx->ARGS.destcoinaddr[0],sizeof(atx->myhalf.destcoinaddr));
                //atx->myhalf.donation = atx->myhalf.amount * SUBATOMIC_DONATIONRATE;
                //if ( atx->myhalf.donation < coin->mgw.txfee )
                //    atx->myhalf.donation = coin->mgw.txfee;
                atx->otherexpectedamount = atx->myhalf.amount;// - 2*coin->mgw.txfee - 2*atx->myhalf.donation;
                subatomic_gen_pubkeys(atx,&atx->myhalf);
            }
            else
            {
                strcpy(atx->otherhalf.coinstr,atx->ARGS.destcoinstr); strcpy(atx->otherhalf.destcoinstr,atx->ARGS.coinstr);
                atx->otherhalf.amount = atx->ARGS.destamount; atx->otherhalf.destamount = atx->ARGS.amount;
                safecopy(atx->otherhalf.coinaddr,atx->ARGS.destcoinaddr[1],sizeof(atx->otherhalf.coinaddr));
                safecopy(atx->otherhalf.destcoinaddr,atx->ARGS.coinaddr[1],sizeof(atx->otherhalf.destcoinaddr));
                //atx->otherhalf.donation = atx->otherhalf.amount * SUBATOMIC_DONATIONRATE;
                //if ( atx->otherhalf.donation < destcoin->mgw.txfee )
                //    atx->otherhalf.donation = destcoin->mgw.txfee;
                atx->myexpectedamount = atx->otherhalf.amount;// - 2*destcoin->mgw.txfee - 2*atx->otherhalf.donation;
            }
            printf("%p.(%s %s %.8f -> %.8f %s <-> %s %s %.8f <- %.8f %s) myhalf.(%s %s) %.8f <-> %.8f other.(%s %s) IP.(%s)\n",atx,atx->ARGS.NXTaddr,atx->myhalf.coinstr,dstr(atx->myhalf.amount),dstr(atx->myhalf.destamount),atx->myhalf.destcoinstr,atx->ARGS.otherNXTaddr,atx->otherhalf.coinstr,dstr(atx->otherhalf.amount),dstr(atx->otherhalf.destamount),atx->otherhalf.destcoinstr,atx->myhalf.coinaddr,atx->myhalf.destcoinaddr,dstr(atx->myexpectedamount),dstr(atx->otherexpectedamount),atx->otherhalf.coinaddr,atx->otherhalf.destcoinaddr,atx->ARGS.otheripaddr);
            return(1 << flipped);
        }
    }
    return(0);
}

int32_t subatomic_txcmp(struct subatomic_tx *_ref,struct subatomic_tx *_atx,int32_t flipped)
{
    struct subatomic_tx_args *ref,*atx;
    ref = &_ref->ARGS; atx = &_atx->ARGS;
    printf("%p.(%s <-> %s) vs %p.(%s <-> %s)\n",ref,ref->NXTaddr,ref->otherNXTaddr,atx,atx->NXTaddr,atx->otherNXTaddr);
    if ( flipped != 0 )
    {
        if ( strcmp(ref->NXTaddr,atx->otherNXTaddr) != 0 )
        {
            printf("%s != %s\n",ref->NXTaddr,atx->otherNXTaddr);
            return(-1);
        }
        if ( strcmp(ref->otherNXTaddr,atx->NXTaddr) != 0 )
            return(-2);
    }
    else
    {
        if ( strcmp(ref->NXTaddr,atx->NXTaddr) != 0 )
        {
            printf("%s != %s\n",ref->NXTaddr,atx->NXTaddr);
            return(-1);
        }
        if ( strcmp(ref->otherNXTaddr,atx->otherNXTaddr) != 0 )
            return(-2);
    }
    if ( flipped == 0 )
    {
        if ( strcmp(ref->coinstr,atx->coinstr) != 0 )
        {
            printf("%s != %s\n",ref->coinstr,atx->coinstr);
            return(-3);
        }
        if ( strcmp(ref->destcoinstr,atx->destcoinstr) != 0 )
            return(-4);
        if ( ref->destamount != atx->destamount )
            return(-5);
        if ( ref->amount != atx->amount )
            return(-6);
    }
    else
    {
        if ( strcmp(ref->coinstr,atx->destcoinstr) != 0 )
        {
            printf("%s != %s\n",ref->coinstr,atx->destcoinstr);
            return(-13);
        }
        if ( strcmp(ref->destcoinstr,atx->coinstr) != 0 )
            return(-14);
        if ( ref->destamount != atx->amount )
            return(-15);
        if ( ref->amount != atx->destamount )
            return(-16);
    }
    return(0);
}

int32_t set_subatomic_trade(struct subatomic_tx *_atx,char *NXTaddr,char *coin,char *amountstr,char *coinaddr,char *otherNXTaddr,char *destcoin,char *destamountstr,char *destcoinaddr,char *otheripaddr,int32_t flipped)
{
    struct subatomic_tx_args *atx = &_atx->ARGS;
    memset(atx,0,sizeof(*atx));
    strcpy(atx->coinstr,coin);
    strcpy(atx->destcoinstr,destcoin);
    atx->amount = conv_floatstr(amountstr);
    atx->destamount = conv_floatstr(destamountstr);
    safecopy(atx->coinaddr[flipped],coinaddr,sizeof(atx->coinaddr[flipped]));
    safecopy(atx->coinaddr[flipped^1],destcoinaddr,sizeof(atx->coinaddr[flipped^1]));
    safecopy(atx->NXTaddr,NXTaddr,sizeof(atx->NXTaddr));
    safecopy(atx->otherNXTaddr,otherNXTaddr,sizeof(atx->otherNXTaddr));
    safecopy(atx->destcoinaddr[flipped],destcoinaddr,sizeof(atx->destcoinaddr[flipped]));
    safecopy(atx->destcoinaddr[flipped^1],coinaddr,sizeof(atx->destcoinaddr[flipped^1]));
    if ( flipped != 0 )
        safecopy(atx->otheripaddr,otheripaddr,sizeof(atx->otheripaddr));
    printf("flipped.%d ipaddr.%s %s %s\n",flipped,atx->otheripaddr,coin,destcoin);
    if ( atx->amount != 0 && atx->destamount != 0 )//&& atx->coinid >= 0 && atx->destcoinid >= 0 )
        return(0);
    printf("error setting subatomic trade %lld %lld %s %s\n",(long long)atx->amount,(long long)atx->destamount,atx->coinstr,atx->destcoinstr);
    return(-1);
}

char *subatomic_txid(char *txbytes,struct coin777 *coin,char *destaddr,uint64_t amount,int32_t future)
{
    struct subatomic_halftx *H; struct subatomic_unspent_tx *utx; int32_t num; uint64_t total; char *txid = 0;
    if ( coin != 0 && destaddr != 0 && destaddr[0] != 0 )
    {
        H = calloc(1,sizeof(*H));
        strcpy(H->coinstr,coin->name);
        utx = gather_unspents(&total,&num,coin,0);
        strcpy(H->multisigaddr.buf,destaddr);
        if ( (txid= subatomic_create_fundingtx(H,amount,future,coin->changeaddr)) != 0 ) //*SUBATOMIC_LOCKTIME);
            strcpy(txbytes,H->funding.rawtransaction);
        free(H);
    }
    return(txid);
}

// this function needs to invert everything to the point of view of this acct
int32_t decode_subatomic_json(struct subatomic_tx *atx,cJSON *json,char *sender,char *receiver)
{
    struct destbuf amountstr,destamountstr,NXTaddr,coin,destcoin,coinaddr,destcoinaddr,otherNXTaddr,otheripaddr;
    int32_t flipped = 0;
    copy_cJSON(&NXTaddr,jobj(json,"NXT"));
    copy_cJSON(&coin,jobj(json,"coin"));
    copy_cJSON(&amountstr,jobj(json,"amount"));
    copy_cJSON(&coinaddr,jobj(json,"coinaddr"));
    copy_cJSON(&otherNXTaddr,jobj(json,"destNXT"));
    copy_cJSON(&destcoin,jobj(json,"destcoin"));
    copy_cJSON(&destamountstr,jobj(json,"destamount"));
    copy_cJSON(&destcoinaddr,jobj(json,"destcoinaddr"));
    copy_cJSON(&otheripaddr,jobj(json,"senderip"));
    if ( strcmp(otherNXTaddr.buf,SUPERNET.NXTADDR) == 0 )
        flipped = 1;
    if ( set_subatomic_trade(atx,NXTaddr.buf,coin.buf,amountstr.buf,coinaddr.buf,otherNXTaddr.buf,destcoin.buf,destamountstr.buf,destcoinaddr.buf,otheripaddr.buf,flipped) == 0 )
        return(flipped);
    return(-1);
}

static struct subatomic_tx **Subatomics; static int32_t Numsubatomics;
struct subatomic_tx *update_subatomic_state(cJSON *argjson,uint64_t nxt64bits,char *sender,char *receiver)  // the only path into the subatomics[], eg. via AM
{
    int32_t i,flipped,cmpval; struct subatomic_tx *atx = 0,T;
    memset(&T,0,sizeof(T));
    //printf("parse subatomic\n");
    if ( (flipped= decode_subatomic_json(&T,argjson,sender,receiver)) >= 0 )
    {
        //printf("NXT.%s (%s %s %s %s) <-> %s (%s %s %s %s)\n",T.NXTaddr,coinid_str(T.coinid),T.coinaddr[0],coinid_str(T.destcoinid),T.destcoinaddr[0],T.otherNXTaddr,coinid_str(T.coinid),T.coinaddr[1],coinid_str(T.destcoinid),T.destcoinaddr[1]);
        atx = 0;
        for (i=0; i<Numsubatomics; i++)
        {
            atx = Subatomics[i];
            if ( (cmpval= subatomic_txcmp(atx,&T,flipped)) == 0 )
            {
                printf("%d: cmpval.%d vs %p\n",i,cmpval,atx);
                if ( T.type == SUBATOMIC_TYPE )
                {
                    strcpy(atx->ARGS.coinaddr[flipped],T.ARGS.coinaddr[flipped]);
                    strcpy(atx->ARGS.destcoinaddr[flipped],T.ARGS.destcoinaddr[flipped]);
                    if ( flipped != 0 )
                        strcpy(atx->ARGS.otheripaddr,T.ARGS.otheripaddr);
                }
                /*else
                 {
                 if ( flipped != 0 )
                 {
                 strcpy(atx->ARGS.otheripaddr,T.swap.otheripaddr);
                 strcpy(atx->ARGS.NXTaddr,T.swap.otherNXTaddr);
                 strcpy(atx->ARGS.otherNXTaddr,T.swap.NXTaddr);
                 }
                 else
                 {
                 strcpy(atx->ARGS.NXTaddr,T.swap.NXTaddr);
                 strcpy(atx->ARGS.otherNXTaddr,T.swap.otherNXTaddr);
                 }
                 }*/
                break;
            }
        }
        if ( i == Numsubatomics )
        {
            atx = malloc(sizeof(T));
            *atx = T;
            if ( T.type == SUBATOMIC_TYPE )
            {
                strcpy(atx->ARGS.coinaddr[flipped],T.ARGS.coinaddr[flipped]);
                strcpy(atx->ARGS.destcoinaddr[flipped],T.ARGS.destcoinaddr[flipped]);
                if ( flipped != 0 )
                {
                    strcpy(atx->ARGS.otheripaddr,T.ARGS.otheripaddr);
                    strcpy(atx->ARGS.NXTaddr,T.ARGS.otherNXTaddr);
                    strcpy(atx->ARGS.otherNXTaddr,T.ARGS.NXTaddr);
                    strcpy(atx->ARGS.coinstr,T.ARGS.destcoinstr);
                    atx->ARGS.amount = T.ARGS.destamount;
                    strcpy(atx->ARGS.destcoinstr,T.ARGS.coinstr);
                    atx->ARGS.destamount = T.ARGS.amount;
                }
                else
                {
                    strcpy(atx->ARGS.coinstr,T.ARGS.coinstr);
                    atx->ARGS.amount = T.ARGS.amount;
                    strcpy(atx->ARGS.destcoinstr,T.ARGS.destcoinstr);
                    atx->ARGS.destamount = T.ARGS.destamount;
                }
            }
            /*else
             {
             if ( flipped != 0 )
             strcpy(atx->ARGS.otheripaddr,T.swap.otheripaddr);
             strcpy(atx->ARGS.NXTaddr,T.swap.NXTaddr);
             strcpy(atx->ARGS.otherNXTaddr,T.swap.otherNXTaddr);
             }*/
            printf("alloc type.%d new %p atx.%d (%s <-> %s)\n",T.type,atx,Numsubatomics,atx->ARGS.NXTaddr,atx->ARGS.otherNXTaddr);
            Subatomics = realloc(Subatomics,(Numsubatomics + 1) * sizeof(*Subatomics));
            Subatomics[Numsubatomics] = atx, atx->tag = Numsubatomics;
            Numsubatomics++;
        }
        atx->initflag |= init_subatomic_tx(atx,flipped);
        printf("got trade! flipped.%d | initflag.%d\n",flipped,atx->initflag);
        if ( atx->initflag == 3 )
        {
            printf("PENDING SUBATOMIC TRADE from %s %s %.8f <-> %s %s %.8f\n",atx->ARGS.NXTaddr,atx->ARGS.coinstr,dstr(atx->ARGS.amount),atx->ARGS.otherNXTaddr,atx->ARGS.destcoinstr,dstr(atx->ARGS.destamount));
        }
    }
    return(atx);
}

void update_subatomic_transfers(char *NXTaddr)
{
    static double nexttime;
    struct subatomic_tx *atx; int32_t i,j,txcreated,retval;
    //printf("update subatomics\n");
    if ( milliseconds() < nexttime )
        return;
    for (i=0; i<Numsubatomics; i++)
    {
        atx = Subatomics[i];
        if ( atx->initflag == 3 && atx->status != SUBATOMIC_COMPLETED && atx->status != SUBATOMIC_ABORTED )
        {
            /*np = get_NXTacct(&createdflag,Global_mp,atx->ARGS.otherNXTaddr);
             if ( (np->recvid == 0 || np->sentid == 0) && verify_peer_link(SUBATOMIC_SIG,atx->ARGS.otherNXTaddr) != 0 )
             {
             nexttime = (microseconds() + 1000000*300);
             continue;
             }
             if ( atx->type == ATOMICSWAP_TYPE )
             {
             atx->status = update_atomic(np,atx);
             continue;
             }*/
            txcreated = verify_txs_created(atx);
            if ( txcreated < 0 )
            {
                atx->status = SUBATOMIC_ABORTED;
                continue;
            }
            if ( (atx->txs_created= txcreated) == 0 )
                continue;
            update_other_refundtxdone(atx);
            update_my_refundtxdone(atx);
            if ( atx->myrefundtx_done <= 0 || atx->other_refundtx_done <= 0 )
                continue;
            update_fundingtx(atx);
            if ( atx->other_fundingtx_confirms-1 < atx->otherhalf.minconfirms )
                continue;
            //printf("micropay loop: share incr.%d otherincr.%d totalfragis.%d\n",atx->ARGS.incr,atx->ARGS.otherincr,totalfragis);
            if ( atx->other_micropaytx_waiting != 0 )
            {
                retval = update_otherincr(atx);//process_microtx(atx,&atx->otherhalf.micropay,atx->ARGS.incr,atx->ARGS.otherincr);
                if ( retval >= 0 )
                    atx->ARGS.otherincr = retval;
                atx->other_micropaytx_waiting = 0;
            }
            if ( atx->ARGS.incr < atx->ARGS.numincr && atx->ARGS.incr <= atx->ARGS.otherincr+1 )
            {
                atx->ARGS.incr++;
                if ( atx->ARGS.incr < atx->ARGS.otherincr )
                    atx->ARGS.incr = atx->ARGS.otherincr;
                atx->ARGS.myshare = ((double)atx->ARGS.numincr - atx->ARGS.incr) / atx->ARGS.numincr;
                calc_micropay(atx);
            }
            if ( atx->ARGS.incr == 100 )
            {
                for (j=0; j<3; j++)
                {
                    send_micropay(atx);
                    sleep(3);
                }
            }
            else send_micropay(atx);
        }
    }
    //printf("done update subatomics\n");
}


#endif

