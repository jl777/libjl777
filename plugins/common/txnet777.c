/******************************************************************************
 * Copyright © 2014-2015 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

//alice: "i want to mix" (public)
//bob: "i'm in with this destination - xxx" (private)
//alice: "group 2 deposits and split again into xxx and yyy and try to guess where mine is ;)" (public)
//bob: "i approve that transaction" (public)
// assets
// NXTprivacy
// rollover
// trade
// paydayloan
// connect opreturn to addunit and redeemunit
// pricefeeds
// testrun
// Within a certain time limit pick as many random transactions from unconfirmed pool as possible. Calculate weight for each of them. Pick 3 with the highest weight.

/*This scheme improves blockchain tech in the following ways:
- transaction rate limit is not set in stone and adjusts depending on market needs
- removed direct incentive to mine blocks, which solves mining centralization problem
- "layers" (PoS/PoW/PoS/PoW) allow different groups to control each other
- SPV is easier to implement because transactions belonging to the same fragment of Merkle tree are not required
- some other things*/

#ifdef DEFINES_ONLY
#ifndef txnet777_h
#define txnet777_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "../includes/cJSON.h"
#include "../utils/SaM.c"
#include "../KV/kv777.c"

#define TXNET_PUBKEYBATCH 777
#define TXNET_FUTUREGAP 3
#define TXNET_DAYSECONDS (24 * 3600)
#define TXNET_PASTGAP TXNET_DAYSECONDS

// TX
struct txnet777_header { uint64_t wtsum; uint32_t sent,numrefs; bits384 sig; }; // sig must be last, rest is for local node usage
struct txnet777_approves { uint32_t nonce,prevchainind,tbd,refstamps[3]; uint64_t reftxids[3],prevtxid; bits384 prevprivkey; }; // nonce must be first
struct txnet777_input { bits384 revealed; char name[16]; uint64_t command,assetid,senderbits,totalcost; uint32_t timestamp,chainind,allocsize; uint16_t numoutputs,leverage; };
struct txnet777_output { uint64_t subcommand,assetid,destbits,destamount; };
struct txnet777_tx { struct txnet777_header H; struct txnet777_approves approvals; struct txnet777_input in; struct txnet777_output out[1]; };

// TXNET
struct txnet777_pending { struct queueitem DL; bits384 privkey; struct txnet777_tx tx; };
struct txnet777_txfilter { struct txnet777 *TXNET; uint64_t txids[3],lastwt,maxwtsum,maxwttxid,minsenttxid,maxmetric,metrictxid; uint32_t minsent,oldest,latest,keysize,valuesize,numitems,maxitems,timestamps[3]; int32_t (*func)(struct txnet777 *TXNET,struct txnet777_tx *tx); };
struct txnet777_hardsettings { uint64_t wtsum_threshold; uint32_t tx_expiration,chainiters,leverage,futuregap,pastgap,dayseconds; };
struct txnet777_softsettings { double pinggap,retransmit; };
struct txnet777_hashchain { int32_t nextpubkey,prevchainind,firstid,pad; bits384 privkey,revealkey,pubkey,prevprivkey; uint64_t prevtxid; };
struct txnet777_state { struct txnet777_hashchain chain; double lastping,lastbroadcast; uint64_t refmillistamp; uint32_t confirmed; };
struct txnet777_acct { uint64_t nxt64bits,passbits; uint8_t NXTACCTSECRET[2048]; uint8_t mysecret[32],mypublic[32]; char NXTADDR[64]; };
struct txnet777_network
{
    struct endpoint myendpoint; char transport[16],ipaddr[64]; uint64_t ipbits; uint16_t port;
    char endpointstr[512],pubpointstr[512];
    int32_t ip6flag,subsock,pubsock,lbserver,lbclient;
};

struct txnet777
{
    char name[64],protocol[16],path[512];
    struct txnet777_hardsettings CONFIG;
    struct txnet777_softsettings TUNE;
    struct txnet777_network NET;
    struct txnet777_state STATE;
    struct txnet777_acct ACCT;
    queue_t pendQ[2],verifyQ[2],confirmQ[2];
    struct kv777 *transactions,*accounts,*passwords,*state,*approvals;
    //int32_t numtx,maxtx,numalltx,maxalltx; uint64_t *txlist; void **alltx;
    char **endpoints,**pubpoints; int32_t numendpoints,numpubpoints;
    int32_t (*validatetx)(struct txnet777 *TXNET,struct txnet777_tx *tx);
    uint8_t serbuf[0x10000]; long (*serializer)(uint8_t *serbuf,void *txoutputs,uint16_t numoutputs);
    uint8_t deserbuf[0x10000]; long (*deserializer)(uint8_t *serbuf,void *txoutputs,uint16_t numoutputs);
};

// externs
bits384 txnet777_signtx(struct txnet777 *TXNET,struct txnet777_pending *emptytx,struct txnet777_tx *tx,int32_t len,char *name,uint64_t senderbits,uint32_t timestamp);
uint64_t txnet777_broadcast(struct txnet777 *TXNET,struct txnet777_tx *tx);
int32_t txnet777_init(struct txnet777 *TXNET,cJSON *json,char *protocol,char *path,char *name,double pingmillis);
char *txnet777_endpoint(char *retbuf,long maxlen,struct txnet777 *TXNET,cJSON *json,char *jsonstr,char *tokenstr,char *forwarder,char *sender,int32_t valid);
char *txnet777_ping(char *retbuf,long maxlen,struct txnet777 *TXNET,cJSON *json,char *jsonstr,char *tokenstr,char *forwarder,char *sender,int32_t valid);

#endif
#else
#ifndef txnet777_c
#define txnet777_c

#ifndef txnet777_h
#define DEFINES_ONLY
#include "txnet777.c"
#undef DEFINES_ONLY
#endif

double Startmilli; long Totalpackets,Confirmed;

int txnet777_wtsort(const void *a,const void *b)
{
//#define wt_a (*(struct txnet777_tx **)a)->H.wtsum
//#define wt_b (*(struct txnet777_tx **)b)->H.wtsum
#define wt_a ((struct txnet777_tx *)a)->H.wtsum
#define wt_b ((struct txnet777_tx *)b)->H.wtsum
    if ( wt_a == wt_b )
        return(0);
    return(wt_a < wt_b ? -1 : 1);
#undef wt_a
#undef wt_b
}

int txnet777_wtcmp(const void *a,const void *b)
{
//#define wt_a (*(struct txnet777_tx **)a)
//#define wt_b (*(struct txnet777_tx **)b)
#define wt_a ((struct txnet777_tx *)a)
#define wt_b ((struct txnet777_tx *)b)
    uint64_t aval,bval;
    aval = wt_a->H.wtsum / (wt_a->H.sent*wt_a->H.sent*wt_a->H.sent + 1), bval = wt_b->H.wtsum / (wt_b->H.sent*wt_b->H.sent*wt_b->H.sent + 1);
    if ( aval == bval )
    {
        aval = wt_a->H.sent, bval = wt_b->H.sent;
        aval = wt_a->H.wtsum, bval = wt_b->H.wtsum;
        if ( aval == bval )
        {
            aval = wt_a->in.timestamp, bval = wt_b->in.timestamp;
            if ( aval == bval )
            {
                aval = wt_a->H.numrefs, bval = wt_b->H.numrefs;
                if ( aval == bval )
                {
                    aval = wt_a->H.sig.txid, bval = wt_b->H.sig.txid;
                    aval ^= wt_a->in.revealed.txid, bval ^= wt_b->in.revealed.txid;
                    if ( aval == bval )
                        return(0);
                    else return(aval < bval ? -1 : 1);
                } return(aval < bval ? -1 : 1);
            } return(aval < bval ? -1 : 1);
        } return(aval < bval ? -1 : 1);
    } return(aval > bval ? -1 : 1);
#undef wt_a
#undef wt_b
}

int32_t txnet777_size(struct txnet777_tx *tx)
{
    int32_t size;
    size = (int32_t)(sizeof(*tx) + (tx->in.numoutputs - 1) * sizeof(*tx->out));
    if ( tx->in.allocsize == 0 )
        return(size);
    if ( size >= tx->in.allocsize && tx->in.allocsize <= 0x10000000 )
        return(tx->in.allocsize);
    printf("illegal tx size.%d vs %d | tx.%ld\n",size,tx->in.allocsize,sizeof(*tx));
    return(-1);
}

uint64_t leverage_to_wt(int32_t leverage)
{
    static uint64_t Leverage_to_wt[32]; uint64_t wt,i;
    if ( leverage >= 0 && leverage < sizeof(Leverage_to_wt)/sizeof(*Leverage_to_wt) )
    {
        if ( Leverage_to_wt[0] == 0 )
            for (wt=3,i=0; i<sizeof(Leverage_to_wt)/sizeof(*Leverage_to_wt); i++,wt*=3)
                Leverage_to_wt[i] = wt;
        return(Leverage_to_wt[leverage]);
    } else printf("leverage_to_wt: illegal leverage.%d\n",leverage);
    return(0);
}

int32_t txnet777_txprint(struct txnet777 *TXNET,struct txnet777_tx *tx)
{
    printf("%s %6.1f sent.%d num.%d wt.%-8llu txid.%-22llu t%u.%02d [%llu %llu %llu]\n",TXNET->path,(double)tx->H.wtsum / (tx->H.sent*tx->H.sent*tx->H.sent + 1),tx->H.sent,tx->H.numrefs,(long long)tx->H.wtsum,(long long)tx->H.sig.txid,tx->in.timestamp/60,tx->in.timestamp%60,(long long)tx->approvals.reftxids[0],(long long)tx->approvals.reftxids[1],(long long)tx->approvals.reftxids[2]);
    return(0);
}

uint64_t txnet777_reftxid(uint64_t reftxid,struct txnet777 *TXNET,uint64_t txid,int32_t ind)
{
    uint8_t key[12],dest[8]; int32_t size,keysize; uint64_t *ptr;
    memcpy(key,&txid,sizeof(txid)), memcpy(&key[sizeof(txid)],&ind,sizeof(ind)), keysize = (sizeof(txid) + sizeof(ind));
    if ( reftxid == 0 )
    {
        size = sizeof(dest);
        if ( (ptr= kv777_read(TXNET->approvals,key,sizeof(key),dest,&size,0)) != 0 && size == sizeof(dest) )
            return(*ptr);
        else printf("txnet777_reftxid error loading txid.%llu:%d size.%d\n",(long long)txid,ind,size);
            return(0);
    }
    kv777_write(TXNET->approvals,key,sizeof(key),&reftxid,sizeof(reftxid));
    return(0);
}

void *txnet777_requeue(char *destname,queue_t *destQ,queue_t pairQ[2],struct txnet777_tx *tx)
{
    int32_t iter; void *ptr; struct txnet777_tx *qtx;
    for (iter=0; iter<2; iter++)
    {
        while ( (ptr= queue_dequeue(&pairQ[iter],0)) != 0 )
        {
            qtx = (void *)((long)ptr + sizeof(struct queueitem));
            if ( qtx->in.allocsize == tx->in.allocsize && memcmp(qtx,tx,tx->in.allocsize) == 0 )
            {
                if ( destQ != 0 )
                    queue_enqueue(destname,destQ,ptr);
                return(ptr);
            } else queue_enqueue("requeue",&pairQ[iter ^ 1],ptr);
        }
    }
    if ( destQ != 0 )
        ptr = queuedata(tx,tx->in.allocsize), queue_enqueue(destname,destQ,ptr);
    return(ptr);
}

void txnet777_wtadd(int32_t sendflag,struct txnet777 *TXNET,struct txnet777_tx *tx,uint64_t reftxid,int32_t leverage)
{
    int32_t i; uint64_t wt; void *ptr;
    if ( tx->H.numrefs > 0 )
    {
        for (i=0; i<tx->H.numrefs; i++)
            if ( txnet777_reftxid(0,TXNET,tx->H.sig.txid,i) == reftxid )
            {
                tx->H.sent += sendflag;
                //printf("duplicate wtsum.%llu for txid.%llu <- %llu leverage.%d\n",(long long)tx->H.wtsum,(long long)tx->H.sig.txid,(long long)reftxid,leverage);
                return;
            }
    }
    tx->H.sent += sendflag;
    wt = leverage_to_wt(leverage);
    if ( tx->H.wtsum < TXNET->CONFIG.wtsum_threshold && (tx->H.wtsum + wt) >= TXNET->CONFIG.wtsum_threshold )
    {
        Confirmed++;
        TXNET->STATE.confirmed++;
        if ( (Confirmed % 33) == 0 )
            printf("%-8s txid.%-22llu CONFIRMED %llu >= %llu (%6d).%-6ld confirmed.%ld %.1f%% total.%ld diff.%ld %.1f/sec\n",TXNET->path,(long long)tx->H.sig.txid,(long long)tx->H.wtsum+wt,(long long)TXNET->CONFIG.wtsum_threshold,TXNET->STATE.confirmed,Confirmed/15,Confirmed,100.*Confirmed/Totalpackets,Totalpackets,Totalpackets-Confirmed,(Totalpackets+1)/((milliseconds()-Startmilli)/1000));
        if ( (ptr= txnet777_requeue("TXNET.done",0,TXNET->pendQ,tx)) != 0 || (ptr= txnet777_requeue("TXNET.done",0,TXNET->verifyQ,tx)) != 0 || (ptr= txnet777_requeue("TXNET.done",0,TXNET->confirmQ,tx)) == 0 )
            free(ptr);
    }
    tx->H.wtsum += wt;
    txnet777_reftxid(reftxid,TXNET,tx->H.sig.txid,tx->H.numrefs++);
}

struct txnet777_tx *txnet777_txid(struct txnet777 *TXNET,uint64_t txid,bits384 *sig)
{
    bits384 _sig; int32_t size = sizeof(*sig);
    if ( sig == 0 )
        sig = &_sig;
    if ( txid != 0 )
    {
        if ( kv777_read(TXNET->transactions,&txid,sizeof(txid),sig,&size,0) == 0 || size != sizeof(*sig) )
        {
            if ( Debuglevel > 2 )
                printf("txnet777_txid error loading txid.%llu size.%d\n",(long long)txid,size);
            return(0);
        }
    }
    return(kv777_read(TXNET->transactions,sig->bytes,sizeof(*sig),0,0,0));
}

void *txnet777_filteriterator(struct kv777 *kv,void *_ptr,void *key,int32_t keysize,void *value,int32_t valuesize)
{
    double metric; int32_t r; struct txnet777_txfilter *filter = _ptr; struct txnet777_tx *tx = value;
    if ( filter->func != 0 && filter->TXNET != 0 && (filter->keysize == 0 || keysize == filter->keysize) && (filter->valuesize == 0 || valuesize == filter->valuesize) )
    {
        if ( (filter->oldest == 0 || tx->in.timestamp >= filter->oldest) && (filter->latest == 0 || tx->in.timestamp < filter->latest) )
        {
            if ( filter->maxitems != 0 )
                (*filter->func)(filter->TXNET,tx);
            if ( tx->H.sent < 3 || (filter->TXNET->ACCT.nxt64bits % 3) == 0 || filter->numitems < sizeof(filter->txids)/sizeof(*filter->txids) )
                filter->txids[filter->numitems % 3] = tx->H.sig.txid, filter->timestamps[filter->numitems % 3] = tx->in.timestamp;
            filter->lastwt = tx->H.wtsum;
            if ( 0 && tx->H.wtsum < filter->TXNET->CONFIG.wtsum_threshold )
            {
                //randombytes((void *)&r,sizeof(r));
                r = (uint32_t)filter->TXNET->ACCT.nxt64bits;
                if ( tx->H.sent < filter->minsent || (r % 13) == 0 )
                    filter->minsent = tx->H.sent, filter->minsenttxid = tx->H.sig.txid;
                metric = tx->H.wtsum / (tx->H.sent + 1);
                if ( filter->maxwttxid == filter->minsenttxid || (rand() % 17) == 0 )
                    filter->maxwtsum = metric, filter->maxwttxid = tx->H.sig.txid;
                metric = tx->H.wtsum / (tx->H.sent * tx->H.sent + 1);
                if ( filter->metrictxid == filter->maxwttxid || filter->metrictxid == filter->minsenttxid || metric > filter->maxmetric ||  (r % 19) == 0  )
                    filter->maxmetric = metric, filter->metrictxid = tx->H.sig.txid;
            }
            //printf("(%llu %.1f, %llu %d, %llu %llu)\n",(long long)filter->metrictxid,metric,(long long)filter->minsenttxid,filter->minsent,(long long)filter->maxwttxid,(long long)filter->maxwtsum);
            filter->numitems++;
        }// else printf("txid.%llu t%u offset.%d failed filter oldest.%lu latest.%lu lag.%lu\n",(long long)tx->H.sig.txid,tx->in.timestamp,tx->in.timestamp - filter->oldest,time(NULL)-filter->oldest,time(NULL)-filter->latest,time(NULL)-tx->in.timestamp);
    }
    if ( filter->maxitems != 0 && filter->numitems >= filter->maxitems )
        return(KV777_ABORTITERATOR);
    return(0);
}

int32_t txnet777_txrefs(struct txnet777 *TXNET,struct txnet777_tx *tx)
{
    int32_t i; struct txnet777_tx *reftx;
    for (i=0; i<3; i++)
        if ( (reftx= txnet777_txid(TXNET,tx->approvals.reftxids[i],0)) != 0 )
            txnet777_wtadd(1,TXNET,reftx,tx->H.sig.txid,tx->in.leverage);
    return(0);
}

int32_t txnet777_txclear(struct txnet777 *TXNET,struct txnet777_tx *tx) { tx->H.wtsum = leverage_to_wt(tx->in.leverage); tx->H.sent = tx->H.numrefs = 0; return(0); }

int32_t txnet777_txinit(struct txnet777 *TXNET)
{
    struct txnet777_txfilter filter; uint32_t now,dayseconds,weekseconds;
    memset(&filter,0,sizeof(filter)), filter.TXNET = TXNET, filter.keysize = sizeof(bits384);
    filter.numitems = 0, filter.func = txnet777_txclear, kv777_iterate(TXNET->transactions,&filter,0,txnet777_filteriterator);
    filter.numitems = 0, filter.func = txnet777_txrefs, kv777_iterate(TXNET->transactions,&filter,0,txnet777_filteriterator);
    HASH_SORT(TXNET->transactions->table,txnet777_wtcmp);
    printf("numitems.%d lastwt.%llu\n",filter.numitems,(long long)filter.lastwt);

    now = (uint32_t)time(NULL);
    TXNET->CONFIG.leverage = 2, TXNET->CONFIG.chainiters = (TXNET_PUBKEYBATCH << 1), TXNET->CONFIG.wtsum_threshold = 600;
    TXNET->CONFIG.pastgap = TXNET_PASTGAP, TXNET->CONFIG.futuregap = TXNET_FUTUREGAP, TXNET->CONFIG.dayseconds = TXNET_DAYSECONDS;
    dayseconds = TXNET->CONFIG.dayseconds, weekseconds = (7 * dayseconds);
    filter.oldest = ((now - 2*weekseconds) / dayseconds) * dayseconds, filter.latest = ((now - weekseconds) / dayseconds) * dayseconds;
    filter.numitems = 0, filter.func = txnet777_txprint, kv777_iterate(TXNET->transactions,&filter,0,txnet777_filteriterator);
    printf("numitems.%d lastwt.%llu\n",filter.numitems,(long long)filter.lastwt);
    filter.maxitems = filter.numitems/2, filter.numitems = 0, filter.func = txnet777_txprint, kv777_iterate(TXNET->transactions,&filter,0,txnet777_filteriterator);
    //if ( filter.lastwt != 0 )
    //    TXNET->CONFIG.wtsum_threshold = ((filter.lastwt / (filter.lastwt/4)) * (filter.lastwt/4));
    printf("numitems.%d lastwt.%llu -> %llu\n",filter.numitems,(long long)filter.lastwt,(long long)TXNET->CONFIG.wtsum_threshold);
    //TXNET->CONFIG.wtsum_threshold = txnet777_networkparams(&TXNET->CONFIG.leverage,TXNET);
    return(0);
}

uint64_t txnet777_networkparams(uint32_t *leveragep,struct txnet777 *TXNET)
{
/*    struct txnet777_txfilter filter; uint64_t newwt,wt = 0; uint32_t i,dayseconds,weekseconds,now; struct txnet777_wtinfo *wtinfo = 0;
    now = (uint32_t)time(NULL);
    dayseconds = TXNET->CONFIG.dayseconds, weekseconds = (7 * dayseconds);
    memset(&filter,0,sizeof(filter)), filter.TXNET = TXNET;
    filter.oldest = ((now - 2*weekseconds) / dayseconds) * dayseconds, filter.latest = ((now - weekseconds) / dayseconds) * dayseconds;
    filter.keysize = sizeof(bits384), filter.func = txnet777_txlist;
    TXNET->numtx = 0;
    kv777_iterate(TXNET->transactions,&filter,0,txnet777_filteriterator);
    if ( TXNET->numtx != 0 )
    {
        revsort64s(TXNET->txlist,TXNET->numtx,sizeof(*TXNET->txlist) * 2);
        wt = TXNET->txlist[TXNET->numtx], newwt = ((wt / (wt/4)) * (wt/4));
        if ( (wtinfo= txnet777_txwt(0,TXNET,0,txnet777_txid(TXNET,TXNET->txlist[TXNET->numtx+1],0))) != 0 )
        {
            for (i=0; i<100; i++)
                if ( (wtinfo->numrefs * leverage_to_wt(i+1)) > newwt )
                    break;
            *leveragep = i;
            printf("numrefs.%d leverage.%d medianwt %llu -> %llu\n",wtinfo->numrefs,*leveragep,(long long)wt,(long long)((wt / (wt/4)) * (wt/4)));
        }
    }
    return(wt);*/
    return(0);
}

int32_t txnet777_savetx(struct txnet777 *TXNET,struct txnet777_tx *tx)
{
    int32_t newflag = 0; struct kv777_item *item; //struct txnet777_tx *newtx; //
    if ( txnet777_txid(TXNET,0,&tx->H.sig) == 0 )
        newflag = 1;
    if ( kv777_write(TXNET->transactions,&tx->H.sig.txid,sizeof(tx->H.sig.txid),tx->H.sig.bytes,sizeof(tx->H.sig)) != 0 )
    {
        if ( Debuglevel > 2 )
        printf("%s new.%d SAVE.%llu t%u sender.%llu total transactions.%d net.%d\n",TXNET->path,newflag,(long long)tx->H.sig.txid,tx->in.timestamp,(long long)tx->in.senderbits,TXNET->transactions->numkeys,TXNET->transactions->netkeys);
        if ( (item= kv777_write(TXNET->transactions,tx->H.sig.bytes,sizeof(tx->H.sig),tx,txnet777_size(tx))) != 0 )
        {
            //if ( newflag != 0 && (newtx= txnet777_txid(TXNET,0,&tx->H.sig)) != 0 )
            //    txnet777_txinsert(TXNET,newtx);
            txnet777_txclear(TXNET,tx);
            return(item->ind);
        }
    }
    return(-1);
}

void txnet777_rwstate(int32_t rwflag,struct txnet777 *TXNET)
{
    int32_t size = sizeof(TXNET->STATE);
    if ( TXNET->state != 0 )
    {
        if ( rwflag != 0 )
            kv777_write(TXNET->state,"state",6,&TXNET->STATE,sizeof(TXNET->STATE));
        else kv777_read(TXNET->state,"state",6,&TXNET->STATE,&size,0);
    }
}

bits384 txnet777_bits384(int32_t rwflag,struct kv777 *kv,void *key,int32_t keysize,bits384 *val)
{
    bits384 *ptr; int32_t size = sizeof(*val);
    if ( kv != 0 )
    {
        if ( rwflag != 0 )
        {
            kv777_write(kv,key,keysize,val->bytes,sizeof(*val));
            if ( (ptr= kv777_read(kv,key,keysize,val->bytes,&size,0)) != 0 && size == sizeof(*val) )
            {
                if ( memcmp(ptr,val,sizeof(*val)) != 0 )
                    printf("txnet777_val384.%s cmp error, mismatched %llx vs %llx\n",kv->name,(long long)ptr->txid,(long long)val->txid);
            } else printf("txnet777_val384.%s read error size.%d\n",kv->name,size);
        } else if ( (ptr= kv777_read(kv,key,keysize,val->bytes,&size,0)) != 0 && size == sizeof(*val) )
            *val = *ptr;
        else memset(val,0,sizeof(*val));
    } else printf("txnet777_val384.%s no kv??\n",kv->name);
    return(*val);
}

int32_t _txnet777_rwkey(uint64_t key[16],char *name,uint64_t account,uint32_t chainind,uint32_t numchainiters)
{
    int32_t len,keysize;
    key[0] = account;
    key[1] = ((((uint64_t)chainind / numchainiters) << 32) | (chainind % numchainiters));
    keysize = sizeof(key[0]) + sizeof(key[1]);
    if ( name != 0 )
    {
        len = (int32_t)strlen(name) + 1;
        memcpy(&key[2],name,len);
        keysize += len;
    }
    return(keysize);
}

bits384 txnet777_rwkey(int32_t rwflag,struct kv777 *kv,bits384 *val,char *name,uint64_t account,uint32_t chainind,uint32_t numchainiters)
{
    uint64_t key[16]; int32_t keysize;
    keysize = _txnet777_rwkey(key,name,account,chainind,numchainiters);
    if ( kv != 0 )
        return(txnet777_bits384(rwflag,kv,key,keysize,val));
    return(*val);
}

#define txnet777_privkey(rwflag,TXNET,chainind,privkey) txnet777_rwkey(rwflag,(TXNET)->passwords,privkey,0,(TXNET)->ACCT.nxt64bits,chainind,(TXNET)->CONFIG.chainiters)
#define txnet777_pubkey(rwflag,TXNET,name,account,chainind,pubkey) txnet777_rwkey(rwflag,(TXNET)->accounts,pubkey,name,account,chainind,(TXNET)->CONFIG.chainiters)

int32_t txnet777_updatepubkey(struct txnet777 *TXNET,char *name,uint64_t senderbits,int32_t chainind,bits384 *pubkey)
{
    uint64_t key[16]; int32_t keysize,checkind,size = sizeof(checkind);
    if ( pubkey->txid != 0 )
    {
        txnet777_pubkey(1,TXNET,name,senderbits,chainind,pubkey);
        keysize = _txnet777_rwkey(key,name,senderbits,1,TXNET->CONFIG.chainiters);
        kv777_write(TXNET->accounts,key,keysize,&chainind,sizeof(chainind));
        if ( kv777_read(TXNET->accounts,key,keysize,&checkind,&size,0) != 0 && size == sizeof(checkind) && checkind != 0 && checkind < chainind )
        {
            printf("not smallest.%d vs %s.chainind.%d\n",checkind,name,chainind);
            return(0);
        }
        if ( Debuglevel > 2 )
            printf("%s txnet777_updatepubkey %s.chainind.%d <- %llx\n",TXNET->path,name,chainind,(long long)pubkey->txid);
    }
    return(0);
}

int32_t txnet777_chain(struct txnet777 *TXNET,bits384 *privkey,bits384 *revealkey,bits384 *pubkey,bits384 seed)
{
    struct SaM_info state; int32_t i,len = (int32_t)strlen(TXNET->name)+1;
    for (i=0; i<TXNET->CONFIG.chainiters; i++)
    {
        if ( i == TXNET->CONFIG.chainiters-1 )
            *revealkey = seed;
        else if ( i == TXNET->CONFIG.chainiters-2 )
            *privkey = seed;
        txnet777_privkey(1,TXNET,TXNET->STATE.chain.firstid + i,&seed);
        SaM_Initialize(&state);
        SaM_Absorb(&state,seed.bytes,sizeof(seed),(uint8_t *)TXNET->name,len);
        seed = SaM_emit(&state);
        //printf("i.%d of %d 1st.%d key.%d.%s priv.%llx %llx %llx %llx %llx %llx\n",i+1,TXNET->CONFIG.chainiters,TXNET->STATE.chain.firstid,i + TXNET->STATE.chain.firstid,TXNET->name,(long long)seed.ulongs[0],(long long)seed.ulongs[1],(long long)seed.ulongs[2],(long long)seed.ulongs[3],(long long)seed.ulongs[4],(long long)seed.ulongs[5]);
    }
    i += TXNET->STATE.chain.firstid;
    txnet777_updatepubkey(TXNET,TXNET->name,TXNET->ACCT.nxt64bits,i,&seed);
    *pubkey = seed;
    return(i);
}

int32_t txnet777_verifykey(struct txnet777 *TXNET,bits384 revealed,char *name,uint64_t nxt64bits,int32_t pubkeyind,int32_t expected)
{
    int32_t i,n=0,m=0; bits384 pubkey,seed; struct SaM_info state;
    return(0);
    txnet777_pubkey(0,TXNET,name,nxt64bits,pubkeyind,&pubkey);
    if ( pubkey.txid == 0 )
    {
        printf("txnet777_verifykey got null pubkey at chainind.%d\n",pubkeyind);
        for (n=0,i=pubkeyind; (i%TXNET->CONFIG.chainiters)!=0; i++,n++)
        {
            txnet777_pubkey(0,TXNET,name,nxt64bits,i,&pubkey);
            if ( pubkey.txid != 0 )
                break;
        }
    }
    seed = revealed;
    for (m=0; m<TXNET->CONFIG.chainiters; m++)
    {
        SaM_Initialize(&state);
        SaM_Absorb(&state,seed.bytes,sizeof(seed),(uint8_t *)name,(int32_t)strlen(name)+1);
        seed = SaM_emit(&state);
        if ( memcmp(seed.bytes,pubkey.bytes,sizeof(seed)) == 0 )
        {
            if ( m > expected )
                printf("%s expected.%d iters.%d+%d found match with pubkeyind.%d revealed.%llx -> %llx\n",TXNET->path,expected,n,m,pubkeyind,(long long)revealed.txid,(long long)pubkey.txid);
            return(0);
        }// printf("(%llx) ",(long long)seed.txid);
    }
    if ( memcmp(seed.bytes,pubkey.bytes,sizeof(seed)) != 0 )
        printf("ERROR NXT.%llu expected.%d %d+%d %s verifykey.%s %llx -> %llx  pubkeyind.%d seed.%llx %llx %llx %llx %llx %llx\n",(long long)nxt64bits,expected,m,n,TXNET->path,name,(long long)revealed.txid,(long long)pubkey.txid,pubkeyind,(long long)seed.ulongs[0],(long long)seed.ulongs[1],(long long)seed.ulongs[2],(long long)seed.ulongs[3],(long long)seed.ulongs[4],(long long)seed.ulongs[5]);
    return(memcmp(seed.bytes,pubkey.bytes,sizeof(seed)));
}

int32_t txnet777_keypair(struct txnet777 *TXNET,bits384 *privkey,bits384 *revealkey,bits384 *pubkey)
{
    bits384 seed; struct txnet777_hashchain *chain = &TXNET->STATE.chain;
    if ( chain->nextpubkey < chain->firstid+2 )
    {
        randombytes(seed.bytes,sizeof(seed));
        chain->firstid += TXNET->CONFIG.chainiters;
        chain->nextpubkey = txnet777_chain(TXNET,&chain->privkey,&chain->revealkey,&chain->pubkey,seed);
    }
    *pubkey = chain->pubkey, *revealkey = chain->revealkey, *privkey = chain->pubkey = chain->privkey;
    txnet777_privkey(0,TXNET,--chain->nextpubkey,&chain->revealkey);
    txnet777_privkey(0,TXNET,--chain->nextpubkey,&chain->privkey);
    txnet777_rwstate(1,TXNET);
    return(chain->nextpubkey);
}

uint32_t txnet777_nonce(struct txnet777_tx *tx,uint32_t nonce)
{
    void *ptr; long offset; int32_t i,len,n = 100; uint32_t nonceerr;
    offset = sizeof(tx->H) + sizeof(tx->approvals.nonce);
    len = (int32_t)(txnet777_size(tx) - offset), ptr = (void *)((long)tx + offset);
    if ( nonce == 0 )
    {
        for (i=0; i<n; i++)
        {
            if ( (nonce= SaM_nonce(ptr,len,tx->in.leverage,1000,0)) != 0 )
                break;
            printf("txnet777 iter.%d of %d couldnt find nonce, try again\n",i,n);
        }
    }
    if ( (nonceerr= SaM_nonce(ptr,len,tx->in.leverage,0,nonce)) != 0 )
    {
        printf("error validating nonce.%u -> %u | len.%d\n",nonce,nonceerr,len);
        return(0);
    }
    return(nonce);
}

uint32_t txnet777_setapprovals(struct txnet777 *TXNET,struct txnet777_pending *emptytx,struct txnet777_tx *tx,int32_t leverage)
{
    struct kv777_item *item; struct txnet777_tx *reftx; struct txnet777_txfilter filter;
    uint32_t i,nonce = 0;
    if ( (item= TXNET->transactions->table) != 0 )
    {
        //HASH_SORT(TXNET->transactions->table,txnet777_wtcmp);
        memset(&filter,0,sizeof(filter)), filter.TXNET = TXNET, filter.keysize = sizeof(bits384);
        filter.minsent = 0xffffffff, filter.maxitems = 0, filter.func = txnet777_txprint, kv777_iterate(TXNET->transactions,&filter,0,txnet777_filteriterator);
        tx->approvals.reftxids[0] = filter.minsenttxid;
        tx->approvals.reftxids[1] = filter.metrictxid;
        tx->approvals.reftxids[2] = filter.maxwttxid;
        for (i=0; i<3; i++)
        {
            //if ( emptytx != 0 && i == 2 )
            //    break;
            if ( (tx->approvals.reftxids[i]= filter.txids[i]) != 0 )
            {
                if ( (reftx= txnet777_txid(TXNET,tx->approvals.reftxids[i],0)) != 0 )
                    tx->approvals.refstamps[i] = reftx->in.timestamp, reftx->H.sent++;
                else tx->approvals.reftxids[i] = 0;
            }
            fprintf(stderr,"[%llu %u] ",(long long)tx->approvals.reftxids[i],tx->approvals.refstamps[i]);
        }
    }
    if ( 0 && emptytx != 0 )
    {
        tx->approvals.reftxids[2] = emptytx->tx.H.sig.txid, tx->approvals.refstamps[2] = emptytx->tx.in.timestamp;
        tx->approvals.prevtxid = emptytx->tx.H.sig.txid, tx->approvals.prevprivkey = emptytx->privkey, tx->approvals.prevchainind = emptytx->tx.in.chainind;
    }
    else
    {
        tx->approvals.prevtxid = TXNET->STATE.chain.prevtxid, tx->approvals.prevprivkey = TXNET->STATE.chain.prevprivkey, tx->approvals.prevchainind = TXNET->STATE.chain.prevchainind;
        TXNET->STATE.chain.prevtxid = 0, memset(TXNET->STATE.chain.prevprivkey.bytes,0,sizeof(TXNET->STATE.chain.prevprivkey));
    }
    txnet777_rwstate(1,TXNET);
    nonce = txnet777_nonce(tx,0);
    return(nonce);
}

void txnet777_setinput(struct txnet777_input *in,char *name,int32_t chainind,uint32_t timestamp,uint64_t senderbits,uint8_t leverage,int32_t len)
{
    strncpy(in->name,name,sizeof(in->name)-1), in->name[sizeof(in->name)-1] = 0;
    in->chainind = chainind, in->timestamp = timestamp, in->senderbits = senderbits;
    in->leverage = leverage, in->allocsize = len;
    //printf("txnet777_setinput.%u len.%d\n",timestamp,len);
}

int32_t txnet777_checksig(int32_t updateflag,struct txnet777 *TXNET,struct txnet777_tx *tx,bits384 privkey,int32_t chainind)
{
    bits384 txid; struct txnet777_tx *checktx;
    txid = SaM_encrypt(0,(void *)((long)tx + sizeof(tx->H)),(int32_t)(tx->in.allocsize - sizeof(tx->H)),privkey,tx->in.timestamp);
    if ( memcmp(txid.bytes,tx->H.sig.bytes,sizeof(txid)) == 0 )
    {
        if ( updateflag != 0 )
        {
            txnet777_savetx(TXNET,tx);
            if ( (checktx= txnet777_txid(TXNET,0,&tx->H.sig)) == 0 || memcmp(checktx,tx,tx->in.allocsize) != 0 )
                printf("txnet777_verifytx cant find just saved tx.%llu\n",(long long)tx->H.sig.txid);
            else if ( (checktx= txnet777_txid(TXNET,tx->H.sig.txid,0)) == 0 || memcmp(checktx,tx,tx->in.allocsize) != 0 )
                printf("txnet777_verifytx B cant find just saved tx.%llu\n",(long long)tx->H.sig.txid);
            if ( Debuglevel > 2 )
                printf("verified.%s txid.%llu prevtxid.%llu\n",tx->in.name,(long long)tx->in.senderbits,(long long)tx->approvals.prevtxid);
        }
        return(0);
    }
    else if ( Debuglevel > 2 )
    {
        int32_t i; uint64_t diff = 0;
        for (i=0; i<6; i++)
            printf("%016llx ",(long long)privkey.ulongs[i]);
        printf("privkey.%d\n",chainind);
        for (i=0; i<6; i++)
            printf("%016llx ",(long long)txid.ulongs[i]);
        printf("checksig\n");
        for (i=0; i<6; i++)
        {
            printf("%016llx ",(long long)tx->H.sig.ulongs[i]);
            diff |= (txid.ulongs[i] ^ tx->H.sig.ulongs[i]);
        }
        printf("mismatched txsig.%llu diff.%llx bits.%d %s crc.%u\n",(long long)tx->H.sig.txid,(long long)diff,bitweight(diff),TXNET->path,_crc32(0,(void *)((long)tx + sizeof(tx->H)),tx->in.allocsize-sizeof(tx->H)));
    }
    return(-2);
}

bits384 txnet777_signtx(struct txnet777 *TXNET,struct txnet777_pending *emptytx,struct txnet777_tx *tx,int32_t len,char *name,uint64_t senderbits,uint32_t timestamp)
{
    int32_t chainind = -1; bits384 privkey,pubkey;
    //memset(&tx->H.sig,0,sizeof(tx->H.sig));
    if ( len <= 0 )
        len = txnet777_size(tx);
    //printf("signtx.%u\n",timestamp);
    if ( len < 0x10000 && (chainind= txnet777_keypair(TXNET,&privkey,&tx->in.revealed,&pubkey)) >= 0 )
    {
        txnet777_setinput(&tx->in,name,chainind,timestamp,senderbits,TXNET->CONFIG.leverage,len);
        tx->approvals.nonce = txnet777_setapprovals(TXNET,emptytx,tx,tx->in.leverage);
        tx->H.sig = SaM_encrypt(0,(void *)((long)tx + sizeof(tx->H)),(int32_t)(len - sizeof(tx->H)),privkey,timestamp);
        if ( Debuglevel > 2 )
        {
            int32_t i;
            printf("\n");
            for (i=0; i<6; i++)
                printf("%016llx ",(long long)privkey.ulongs[i]);
            printf("privkey.%d\n",chainind);
            for (i=0; i<6; i++)
                printf("%016llx ",(long long)tx->in.revealed.ulongs[i]);
            printf("reveal.%d\n",chainind+1);
            for (i=0; i<6; i++)
                printf("%016llx ",(long long)pubkey.ulongs[i]);
            printf("pubkey.%d\n",chainind+2);
            for (i=0; i<6; i++)
                printf("%016llx ",(long long)tx->H.sig.ulongs[i]);
            printf("signed txsig.%llu %s allocsize.%d crc.%u\n",(long long)tx->H.sig.txid,TXNET->path,len,_crc32(0,(void *)((long)tx + sizeof(tx->H)),len-sizeof(tx->H)));
        }
        TXNET->STATE.chain.prevprivkey = privkey, TXNET->STATE.chain.prevtxid = tx->H.sig.txid, TXNET->STATE.chain.prevchainind = chainind;
        txnet777_rwstate(1,TXNET);
    } else memset(privkey.bytes,0,sizeof(privkey)), printf("ERROR cant get keypair NXT.%llu %s chainind.%d len.%d\n",(long long)senderbits,name,chainind,len);
    return(privkey);
}

int32_t txnet777_processtx(int32_t updateflag,struct txnet777 *TXNET,struct txnet777_tx *tx,int32_t len)
{
    int32_t i; struct txnet777_tx *reftx,*prevtx = 0; void *ptr;
    if ( tx->in.allocsize == len && txnet777_nonce(tx,tx->approvals.nonce) == tx->approvals.nonce )
    {
        if ( tx->in.timestamp >= TXNET->CONFIG.tx_expiration && (TXNET->validatetx == 0 || (*TXNET->validatetx)(TXNET,tx) == 0) )
        {
            if ( tx->approvals.prevchainind != 0 )
                 txnet777_updatepubkey(TXNET,tx->in.name,tx->in.senderbits,tx->approvals.prevchainind,&tx->approvals.prevprivkey);
            txnet777_updatepubkey(TXNET,tx->in.name,tx->in.senderbits,tx->in.chainind+1,&tx->in.revealed);
            if ( txnet777_verifykey(TXNET,tx->in.revealed,tx->in.name,tx->in.senderbits,tx->in.chainind+2,1) == 0 )
            {
                txnet777_savetx(TXNET,tx);
                for (i=0; i<3; i++)
                {
                    if ( (reftx= txnet777_txid(TXNET,tx->approvals.reftxids[i],0)) != 0 && reftx->in.timestamp == tx->approvals.refstamps[i] )
                        txnet777_wtadd(0,TXNET,reftx,tx->H.sig.txid,tx->in.leverage);
                    else printf("%s processtx ref.%llu reftx.%p not found or bad timestamp %u %d\n",TXNET->path,(long long)tx->approvals.reftxids[i],reftx,reftx!=0?reftx->in.timestamp:0,tx->approvals.refstamps[i]);
                }
                queue_enqueue("TXNET.verify",&TXNET->verifyQ[0],queuedata(tx,len));
            } else printf("txid.%llu %llx revealed.%d did not verify\n",(long long)tx->H.sig.txid,(long long)tx->in.revealed.txid,tx->in.chainind+2);
            if ( tx->approvals.prevtxid != 0 && (prevtx= txnet777_txid(TXNET,tx->approvals.prevtxid,0)) != 0 && prevtx->in.senderbits == tx->in.senderbits )
            {
                if ( txnet777_checksig(updateflag,TXNET,prevtx,tx->approvals.prevprivkey,tx->approvals.prevchainind) == 0 )
                {
                    if ( txnet777_verifykey(TXNET,tx->approvals.prevprivkey,tx->in.name,tx->in.senderbits,tx->approvals.prevchainind+2,2) == 0 )
                    {
                        txnet777_wtadd(0,TXNET,prevtx,tx->H.sig.txid,tx->in.leverage);
                        //if ( Debuglevel > 2 )
                            printf("%s prevtxid.%llu validated %llu wtsum %llu\n",prevtx->in.name,(long long)tx->approvals.prevtxid,(long long)prevtx->H.sig.txid,(long long)prevtx->H.wtsum);
                        if ( (ptr= txnet777_requeue("TXNET.confirm",&TXNET->confirmQ[0],TXNET->verifyQ,prevtx)) == 0 )
                            printf("couldnt dequeue prevtx.%llu\n",(long long)prevtx->H.sig.txid);
                    } else printf("%s prevtxid.%llu didnt verifytx\n",prevtx->in.name,(long long)prevtx->approvals.prevtxid);
                } else printf("%s prevtxid.%llu didnt checksig\n",TXNET->path,(long long)prevtx->approvals.prevtxid);
            } else printf("prevtxid.%llu prevtx.%p\n",(long long)tx->approvals.prevtxid,prevtx);
        } else printf("tx timestamp.%u expired.%u\n",tx->in.timestamp,TXNET->CONFIG.tx_expiration);
    } else printf("%llx revealed hash no nonce\n",(long long)tx->in.revealed.txid);
    return(-1);
}

char *txnet777_ping(char *retbuf,long maxlen,struct txnet777 *TXNET,cJSON *json,char *jsonstr,char *tokenstr,char *forwarder,char *sender,int32_t valid)
{
    return(0);
}

void txnet777_addendpoint(struct txnet777 *TXNET,char *endpoint,char *pubpoint)
{
    int32_t i;
    if ( endpoint != 0 )
    {
        for (i=0; i<TXNET->numendpoints; i++)
            if ( strcmp(endpoint,TXNET->endpoints[i]) == 0 )
                break;
        if ( i == TXNET->numendpoints )
        {
            TXNET->endpoints = realloc(TXNET->endpoints,sizeof(*TXNET->endpoints) * (TXNET->numendpoints + 1));
            TXNET->endpoints[TXNET->numendpoints++] = clonestr(endpoint);
            //if ( strcmp(endpoint,TXNET->endpoint0) == 0 )
            nn_connect(TXNET->NET.lbclient,endpoint);
            printf("%s adding endpoint.(%s) num.%d p%d\n",TXNET->path,endpoint,TXNET->numendpoints,TXNET->numpubpoints);
        }
    }
    if ( pubpoint != 0 )
    {
        for (i=0; i<TXNET->numpubpoints; i++)
            if ( strcmp(pubpoint,TXNET->pubpoints[i]) == 0 )
                break;
        if ( i == TXNET->numpubpoints )
        {
            TXNET->pubpoints = realloc(TXNET->pubpoints,sizeof(*TXNET->pubpoints) * (TXNET->numpubpoints + 1));
            TXNET->pubpoints[TXNET->numpubpoints++] = clonestr(pubpoint);
            //if ( strcmp(pubpoint,TXNET->pubpoint0) == 0 )
            nn_connect(TXNET->NET.subsock,pubpoint);
            printf("%s adding pubpoint.(%s) num.%d e%d\n",TXNET->path,pubpoint,TXNET->numpubpoints,TXNET->numendpoints);
        }
    }
}

char *txnet777_endpoints(struct txnet777 *TXNET)
{
    int32_t i; cJSON *json,*arrayP,*array;
    array = cJSON_CreateArray(), arrayP = cJSON_CreateArray(), json = cJSON_CreateObject();
    for (i=0; i<TXNET->numendpoints; i++)
        jaddistr(array,TXNET->endpoints[i]);
    for (i=0; i<TXNET->numpubpoints; i++)
        jaddistr(arrayP,TXNET->pubpoints[i]);
    jaddstr(json,"T",TXNET->path);
    jaddnum(json,"numendpoints",TXNET->numendpoints);
    jaddnum(json,"numpubpoints",TXNET->numpubpoints);
    jadd(json,"endpoints",array);
    jadd(json,"pubpoints",arrayP);
    return(jprint(json,1));
}

void txnet777_parse(struct txnet777 *TXNET,cJSON *json)
{
    int32_t i,n; cJSON *arrayP,*array;
    if ( (arrayP= jobj(json,"pubpoints")) != 0 && (array= jobj(json,"endpoints")) != 0 )
    {
        n = cJSON_GetArraySize(arrayP);
        for (i=0; i<n; i++)
            txnet777_addendpoint(TXNET,jstri(arrayP,i),0);
        n = cJSON_GetArraySize(array);
        for (i=0; i<n; i++)
            txnet777_addendpoint(TXNET,jstri(array,i),0);
    }
}

long serialize_bits384(uint8_t *dest,bits384 *hash,int32_t n)
{
    int32_t i;
    for (i=0; i<n; i++)
        *dest++ = hash->bytes[i];
    return(n);
}

uint8_t *txnet777_serialize(struct txnet777 *TXNET,uint8_t *serbuf,int32_t max,struct txnet777_tx *tx)
{
    long i,offset = 0; int32_t size = txnet777_size(tx);
    return(0);
    if ( size > max )
        serbuf = calloc(1,size);
    struct txnet777_output { uint64_t subcommand,assetid,destbits,destamount; };
    offset = sizeof(struct txnet777_header) - sizeof(tx->H.sig);
#define SERIALIZE(item) offset = serialize_bits384(&serbuf[offset],(bits384 *)&(item),sizeof(item))
    SERIALIZE(tx->H.sig);
    SERIALIZE(tx->approvals.nonce), SERIALIZE(tx->approvals.prevchainind), SERIALIZE(tx->approvals.tbd);
    SERIALIZE(tx->approvals.refstamps[0]), SERIALIZE(tx->approvals.refstamps[1]), SERIALIZE(tx->approvals.refstamps[2]);
    SERIALIZE(tx->approvals.reftxids[0]), SERIALIZE(tx->approvals.reftxids[1]), SERIALIZE(tx->approvals.reftxids[2]);
    SERIALIZE(tx->approvals.prevtxid), SERIALIZE(tx->approvals.prevprivkey);
    SERIALIZE(tx->in.revealed), SERIALIZE(tx->in.name);
    SERIALIZE(tx->in.command), SERIALIZE(tx->in.assetid), SERIALIZE(tx->in.senderbits), SERIALIZE(tx->in.totalcost);
    SERIALIZE(tx->in.timestamp), SERIALIZE(tx->in.chainind), SERIALIZE(tx->in.allocsize), SERIALIZE(tx->in.numoutputs), SERIALIZE(tx->in.leverage);
    if ( TXNET->serializer == 0 )
    {
        for (i=0; i<tx->in.numoutputs; i++)
            SERIALIZE(tx->out[i].subcommand), SERIALIZE(tx->out[i].assetid), SERIALIZE(tx->out[i].destbits), SERIALIZE(tx->out[i].destamount);
    } else offset = (*TXNET->serializer)(&serbuf[offset],tx->out,tx->in.numoutputs);
    return((uint8_t *)tx);
    return(serbuf);
#undef SERIALIZE
}

int32_t txnet777_incoming(int32_t rtflag,struct txnet777 *TXNET,struct txnet777_tx *tx,int32_t recvlen)
{
    uint32_t i,now,nonceerr = 0;
    if ( 0 && strcmp(TXNET->path,"RPS/T1") == 0 )
    {
        for (i=0; i<recvlen; i++)
            fprintf(stderr,"%02x ",((uint8_t *)tx)[i]);
        fprintf(stderr,"%s received.%d %x\n",TXNET->path,recvlen,recvlen);
    }
    now = (uint32_t)time(NULL);
    if ( Debuglevel > 2 )
        printf("%s (%s) received.%d vs %d <<<<<<<<<<< %llu [%llu %llu %llu] total.%ld %.1f/sec\n",TXNET->path,tx->in.name,recvlen,tx->in.allocsize,(long long)tx->H.sig.txid,(long long)tx->approvals.reftxids[0],(long long)tx->approvals.reftxids[1],(long long)tx->approvals.reftxids[2],Totalpackets,(Totalpackets+1)/((milliseconds()-Startmilli)/1000));
    if ( rtflag != 0 && (tx->in.timestamp > (now + TXNET->CONFIG.futuregap) || tx->in.timestamp < (now - TXNET->CONFIG.pastgap)) )
        printf("??????????? illegal timestamp %u when now.%u\n",now,tx->in.timestamp);
    else if ( recvlen == tx->in.allocsize && (nonceerr= txnet777_nonce(tx,tx->approvals.nonce)) != 0 )
    {
        Totalpackets++;
        txnet777_savetx(TXNET,tx);
        txnet777_processtx(1,TXNET,tx,recvlen);
        return(0);
    }
    else printf("len mismatch (%d vs %d) error validating nonce.%u -> %u\n",recvlen,tx->in.allocsize,tx->approvals.nonce,nonceerr);
    if ( tx->approvals.reftxids[0] != 0 )
        printf("%s txnet777_poll.(%s) sub RECV.%d | nonceerr.%u %llx %llx %llx\n",TXNET->path,TXNET->name,recvlen,nonceerr,(long long)tx->approvals.reftxids[0],(long long)tx->approvals.reftxids[1],(long long)tx->approvals.reftxids[2]);
    return(-1);
}

uint64_t txnet777_broadcast(struct txnet777 *TXNET,struct txnet777_tx *tx)
{
    int32_t i,len,sendlen; //uint8_t *ptr;
    txnet777_savetx(TXNET,tx);
    if ( TXNET->NET.pubsock >= 0 )
    {
        /*if ( (ptr= txnet777_serialize(TXNET,TXNET->serbuf,sizeof(TXNET->serbuf),tx)) != 0 )
        {
            nn_send(TXNET->NET.pubsock,ptr,txnet777_size(tx),0);
            if ( ptr != TXNET->serbuf && ptr != (void *)tx )
                free(ptr);
        }
        else*/
        len = txnet777_size(tx);
        sendlen = nn_send(TXNET->NET.pubsock,tx,len,0);
        if ( 0 && strcmp(TXNET->path,"RPS/T0") == 0 )
        {
            for (i=0; i<len; i++)
                fprintf(stderr,"%02x ",((uint8_t *)tx)[i]);
            printf("%s sent.%d %x allocsize.%d offset.%ld\n",TXNET->path,len,len,tx->in.allocsize,(long)&tx->in.allocsize-(long)tx);
        }
        TXNET->STATE.lastbroadcast = milliseconds();
        txnet777_incoming(1,TXNET,tx,sendlen);
    }
    return(tx->H.sig.txid);
}

void txnet777_sendtx(struct txnet777 *TXNET,struct txnet777_tx *tx)
{
    struct txnet777_pending *pendtx; uint32_t nonceerr;
    pendtx = calloc(1,sizeof(*pendtx) + txnet777_size(tx));
    memset(pendtx,0,sizeof(*pendtx));
    pendtx->privkey = txnet777_signtx(TXNET,0,tx,txnet777_size(tx),TXNET->name,TXNET->ACCT.nxt64bits,(uint32_t)time(NULL));
    nonceerr = txnet777_nonce(tx,tx->approvals.nonce);
    memcpy(&pendtx->tx,tx,txnet777_size(tx));
    if ( tx->approvals.nonce != nonceerr )
        printf("newpubkey.%llx for timestamp.%u nonce.%d err.%u\n",pendtx->privkey.txid,tx->in.timestamp,tx->approvals.nonce,nonceerr);
    txnet777_broadcast(TXNET,tx);
    txnet777_processtx(1,TXNET,tx,tx->in.allocsize);
    if ( Debuglevel > 2 )
        printf(">>>>>>>>>>>>>>>>>> %s sendtx.%llu t%u sender.%llu\n",TXNET->path,(long long)tx->H.sig.txid,tx->in.timestamp,(long long)tx->H.sig.txid);
    queue_enqueue("pendQ",&TXNET->pendQ[0],&pendtx->DL);
}

void txnet777_test0(struct txnet777 *TXNET)
{
    struct txnet777_tx *tx; uint8_t buf[65536];
    tx = (void *)buf;
    memset(tx,0,sizeof(*tx)*2);
    txnet777_sendtx(TXNET,tx);
}

void txnet777_poll(void *_args)
{
    char *msg; cJSON *json; int32_t recvlen; struct txnet777 *TXNET = _args;
    while ( 1 )
    {
        if ( milliseconds() > (TXNET->STATE.lastping + TXNET->TUNE.pinggap) )
        {
            TXNET->CONFIG.tx_expiration = (uint32_t)time(NULL) - 24*3600;
            if ( TXNET->NET.subsock >= 0 && (recvlen= nn_recv(TXNET->NET.subsock,&msg,NN_MSG,0)) > 0 )
            {
                if ( 0 && (json= cJSON_Parse(msg)) != 0 )
                {
                    printf("%s RECV.(%s)\n",TXNET->path,msg);
                    txnet777_parse(TXNET,json);
                    free_json(json);
                }
                else txnet777_incoming(1,TXNET,(struct txnet777_tx *)msg,recvlen);
                nn_freemsg(msg);
            }
            else if ( 0 && milliseconds() > (TXNET->STATE.lastbroadcast + TXNET->TUNE.retransmit) )
            {
                int32_t iter,flag=0; uint32_t now; struct txnet777_pending *pendtx; struct txnet777_tx tx;
                now = (uint32_t)time(NULL);
                for (iter=0; iter<2; iter++)
                {
                    while ( (pendtx= queue_dequeue(&TXNET->pendQ[iter],0)) != 0 )
                    {
                        flag++;
                        //printf("now.%u vs t%u retransmit.%3f\n",now,pendtx->tx.in.timestamp,TXNET->TUNE.retransmit/1000);
                        if ( now > (pendtx->tx.in.timestamp + TXNET->TUNE.retransmit/1000) )
                        {
                            memset(&tx,0,sizeof(tx));
                            txnet777_signtx(TXNET,pendtx,&tx,txnet777_size(&tx),TXNET->name,TXNET->ACCT.nxt64bits,(uint32_t)time(NULL));
                            printf("%s retransmit >>>>>>>>>>>>>> tx.%llu wtsum %llu\n",TXNET->path,(long long)pendtx->tx.H.sig.txid,(long long)pendtx->tx.H.wtsum);
                            txnet777_broadcast(TXNET,&tx);
                            if ( now > (pendtx->tx.in.timestamp + 2*TXNET->TUNE.retransmit/1000) )
                                queue_enqueue("requeue",&TXNET->pendQ[iter ^ 1],&pendtx->DL);
                            else free(pendtx), printf("abandon tx.%llu\n",(long long)pendtx->tx.H.sig.txid);
                        } else queue_enqueue("requeue",&TXNET->pendQ[iter ^ 1],&pendtx->DL);
                    }
                }
               // if ( flag == 0 )//&& strcmp(TXNET->path,"RPS/T0") == 0 )
                    txnet777_test0(TXNET);
            }
            if ( Totalpackets < 1000 || (rand() % 50) == 0 )
                txnet777_test0(TXNET);
            TXNET->STATE.lastping = milliseconds();
        } else msleep(1);
    }
}

char *txnet777_endpoint(char *retbuf,long maxlen,struct txnet777 *TXNET,cJSON *json,char *jsonstr,char *tokenstr,char *forwarder,char *sender,int32_t valid)
{
    char cmd[1024],*endpoint; uint32_t nonce; struct txnet777_network *net = &TXNET->NET;
    if ( (endpoint= jstr(json,"endpoint")) == 0 || endpoint[0] == 0 || strcmp(endpoint,"disconnect") == 0 )
    {
        memset(net->endpointstr,0,sizeof(net->endpointstr));
        memset(&net->myendpoint,0,sizeof(net->myendpoint));
        endpoint = "disconnect";
    }
    else
    {
        net->port = parse_endpoint(&net->ip6flag,net->transport,net->ipaddr,retbuf,jstr(json,"endpoint"),net->port);
        net->ipbits = calc_ipbits(net->ipaddr) | ((uint64_t)net->port << 32);
        net->myendpoint = calc_epbits(net->transport,(uint32_t)net->ipbits,net->port,NN_PUB);
        expand_epbits(net->endpointstr,net->myendpoint);
    }
    sprintf(cmd,"{\"method\":\"busdata\",\"plugin\":\"relay\",\"submethod\":\"protocol\",\"protocol\":\"%s\",\"endpoint\":\"%s\"}",TXNET->protocol,endpoint);
    return(busdata_sync(&nonce,cmd,"allrelays",0));
}

int32_t txnet777_init(struct txnet777 *TXNET,cJSON *json,char *protocol,char *path,char *name,double pingmillis)
{
    int32_t protocols_init(int32_t sock,struct endpoint *connections,char *protocol);
    cJSON *argjson; char buf[512]; uint16_t port; int32_t sendtimeout = 10,recvtimeout = 1;
    safecopy(TXNET->name,name,sizeof(TXNET->name)), strcpy(TXNET->protocol,protocol), strcpy(TXNET->path,path);
    TXNET->TUNE.pinggap = pingmillis, TXNET->TUNE.retransmit = pingmillis;
    if ( path == 0 )
        path = protocol;
    strcpy(TXNET->path,path);
    ensure_directory(TXNET->path);
    TXNET->transactions = kv777_init(path,"transactions",0);
    TXNET->accounts = kv777_init(path,"accounts",0);
    TXNET->passwords = kv777_init(path,"password",0);
    TXNET->approvals = kv777_init(path,"approvals",0);
    TXNET->state = kv777_init(path,"state",0);
    txnet777_rwstate(0,TXNET);
    printf("%s CHAINID.%d\n",TXNET->path,TXNET->STATE.chain.nextpubkey);
    if ( (argjson= jobj(json,name)) != 0 )
    {
        copy_cJSON((void *)TXNET->ACCT.NXTACCTSECRET,jobj(argjson,"secret"));
        copy_cJSON((void *)TXNET->NET.transport,jobj(argjson,"transport"));
        copy_cJSON((void *)TXNET->NET.ipaddr,jobj(argjson,"myipaddr"));
        TXNET->NET.port = juint(argjson,"port");
    }
    if ( TXNET->ACCT.NXTACCTSECRET[0] == 0 )
    {
        sprintf(buf,"%s/randvals",path);
        gen_randomacct(33,TXNET->ACCT.NXTADDR,(void *)TXNET->ACCT.NXTACCTSECRET,buf);
    }
    if ( TXNET->NET.port == 0 )
    {
        port = (uint16_t)stringbits(protocol);
        if ( port < SUPERNET_PORT )
            port = SUPERNET_PORT + ((((uint16_t)protocol[0] << 8) | name[0]) % 777);
        TXNET->NET.port = port;
    }
    sprintf(TXNET->NET.endpointstr,"%s://%s:%u",TXNET->NET.transport,TXNET->NET.ipaddr,TXNET->NET.port + LB_OFFSET);
    sprintf(TXNET->NET.pubpointstr,"%s://%s:%u",TXNET->NET.transport,TXNET->NET.ipaddr,TXNET->NET.port + PUBRELAYS_OFFSET);
    if ( (TXNET->NET.lbclient= nn_createsocket(TXNET->NET.endpointstr,0,"NN_REQ",NN_REQ,TXNET->NET.port + LB_OFFSET,sendtimeout,recvtimeout)) < 0 )
        printf("error creating lbclient\n");
    if ( (TXNET->NET.lbserver= nn_createsocket(TXNET->NET.endpointstr,1,"NN_REP",NN_REP,TXNET->NET.port + LB_OFFSET,sendtimeout,recvtimeout)) < 0 )
        printf("error creating lbserver\n");
    printf("found %s path.(%s) protocol.(%s) json secret.(%s) %s:port.%d\n",name,path,protocol,(void *)TXNET->ACCT.NXTACCTSECRET,TXNET->NET.ipaddr,TXNET->NET.port);
#ifndef BUNDLED
    os_compatible_path(TXNET->path), ensure_directory(TXNET->path);
#endif
    buf[0] = 0, TXNET->NET.subsock = nn_createsocket(buf,0,"NN_SUB",NN_SUB,0,10,1), nn_setsockopt(TXNET->NET.subsock,NN_SUB,NN_SUB_SUBSCRIBE,"",0);
    TXNET->NET.pubsock = nn_createsocket(TXNET->NET.pubpointstr,1,"NN_PUB",NN_PUB,0,10,1);
    set_KV777_globals(0,TXNET->NET.transport,TXNET->ACCT.NXTACCTSECRET,(int32_t)strlen((void *)TXNET->ACCT.NXTACCTSECRET),0,0);
    txnet777_txinit(TXNET);
    msleep((rand()%100) + 1);
    return(0);
}

void txnet777_test(char *protocol,char *path,char *agent)
{
    struct txnet777 *txnet,*TXNETS[1]; char buf[1024],secret[64],threadpath[100]; uint64_t refmillistamp; cJSON *json; double pingmillis = 100;
    uint32_t i,j,numthreads = (uint32_t)(sizeof(TXNETS)/sizeof(*TXNETS));
    portable_OS_init();
   // Debuglevel = 3;
    refmillistamp = millistamp();
    msleep(100);
    ensure_directory(path);
    for (i=0; i<numthreads; i++)
    {
        msleep(10 + (rand() % 10));
        TXNETS[i] = txnet = calloc(1,sizeof(*txnet));
        _randombytes((void *)&txnet->ACCT.passbits,sizeof(txnet->ACCT.passbits),0);
        init_hexbytes_noT(buf,(void *)&txnet->ACCT.passbits,sizeof(txnet->ACCT.passbits));
        txnet->ACCT.nxt64bits = conv_NXTpassword(txnet->ACCT.mysecret,txnet->ACCT.mypublic,(void *)buf,(int32_t)strlen(buf));
    }
    for (i=0; i<numthreads; i++)
    {
        txnet = TXNETS[i];
        init_hexbytes_noT(secret,(void *)&txnet->ACCT.passbits,sizeof(txnet->ACCT.passbits));
        txnet->NET.ipbits = rand(), expand_ipbits(txnet->NET.ipaddr,txnet->NET.ipbits);
        sprintf(buf,"{\"rps\":{\"secret\":\"%s\",\"transport\":\"inproc\",\"myipaddr\":\"%s\"}}",secret,txnet->NET.ipaddr);
        printf("T.%d %s\n",i,buf);
        json = cJSON_Parse(buf);
        sprintf(threadpath,"%s/T%d",path,i);
        txnet777_init(txnet,json,protocol,threadpath,agent,pingmillis);
        txnet->STATE.refmillistamp = millistamp();
        free_json(json);
    }
    for (i=0; i<numthreads; i++)
        for (j=0; j<numthreads; j++)
            if ( i != j )
                txnet777_addendpoint(TXNETS[i],TXNETS[j]->NET.endpointstr,TXNETS[j]->NET.pubpointstr);
    for (i=0; i<numthreads; i++)
        portable_thread_create((void *)txnet777_poll,TXNETS[i]);
    Startmilli = milliseconds();
   // getchar();
    
}
#endif
#endif

