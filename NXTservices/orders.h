//
//  orders.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_orders_h
#define xcode_orders_h

#include "../NXTservices/atomic.h"

#define ORDERBOOK_SIG 0x83746783

struct orderbook_tx
{
    int32_t sig,type;
    uint64_t txid,nxt64bits,baseid,relid;
    uint64_t baseamount,relamount;
};

struct quote
{
    float price;    // must be first!!
    int32_t type;
    uint64_t nxt64bits,baseamount,relamount;
};

struct orderbook
{
    uint64_t assetA,assetB;
    struct quote *bids,*asks;
    int32_t numbids,numasks,polarity;
};

struct raw_orders
{
    uint64_t assetA,assetB,obookid;
    struct orderbook_tx **orders;
    int num,max;
} **Raw_orders;
int Num_raw_orders,Max_raw_orders;

void display_orderbook_tx(struct orderbook_tx *tx)
{
    printf("sig.%08x t.%d NXT.%llu baseid.%llu %.8f | relid.%llu %.8f\n",tx->sig,tx->type,(long long)tx->nxt64bits,(long long)tx->baseid,dstr(tx->baseamount),(long long)tx->relid,dstr(tx->relamount));
}

int32_t find_orderbook_tx(struct raw_orders *raw,struct orderbook_tx *tx)
{
    int32_t i;
    for (i=0; i<raw->num; i++)
        if ( raw->orders[i] == tx )
            return(i);
    return(-1);
}

struct orderbook_tx **clone_orderptrs(int32_t *nump,struct raw_orders *raw)
{
    int32_t i,m,n=0;
    struct orderbook_tx *tx,**orders = 0;
    // jl777 lock/unlock
    if ( (m= raw->num) != 0 )
    {
        orders = (struct orderbook_tx **)malloc(raw->max * sizeof(*orders));
        for (i=0; i<m; i++)
        {
            if ( (tx= raw->orders[i]) != 0 )
                orders[n++] = tx;
        }
    }
    *nump = n;
    return(orders);
}

int32_t delete_orderbook_tx(struct raw_orders *raw,struct orderbook_tx *tx)
{
    int32_t ind;
    if ( (ind= find_orderbook_tx(raw,tx)) >= 0 )
    {
        raw->orders[ind] = raw->orders[--raw->num];
        raw->orders[raw->num] = 0;
        free(tx);
        return(0);
    }
    printf("delete_orderbook_tx: MISSING TX.%p\n",tx);
    return(-1);
}

int32_t append_orderbook_tx(struct raw_orders *raw,struct orderbook_tx *tx,uint64_t txid)
{
    char txidstr[64];
    int32_t ind,createdflag;
    if ( (ind= find_orderbook_tx(raw,tx)) < 0 )
    {
        if ( raw->num >= raw->max )
        {
            raw->max = raw->num + 1;
            raw->orders = (struct orderbook_tx **)realloc(raw->orders,sizeof(*raw->orders) * raw->max);
        }
        printf("add tx.%p -> slot.%d\n",tx,raw->num);
        expand_nxt64bits(txidstr,txid);
        if ( Global_pNXT != 0 && Global_pNXT->orderbook_txidsp != 0 && *Global_pNXT->orderbook_txidsp != 0 )
            MTadd_hashtable(&createdflag,Global_pNXT->orderbook_txidsp,txidstr);
        else printf("ERROR: UNINITIALIZED orderbook hashtable\n");
        raw->orders[raw->num++] = tx;
        return(0);
    }
    printf("append_orderbook_tx: already have tx.%p txid.%llu\n",tx,(long long)txid);
    return(-1);
}

uint64_t *search_jl777_txid(uint64_t txid)
{
    char txidstr[64];
    int32_t createdflag;
    struct NXT_str *tp;
    expand_nxt64bits(txidstr,txid);
    tp = MTadd_hashtable(&createdflag,Global_pNXT->orderbook_txidsp,txidstr);
    if ( createdflag != 0 )
        return(0);
    return(&tp->modified);
}

int32_t _find_raw_orders(uint64_t obookid)
{
    int32_t i;
    for (i=0; i<Num_raw_orders; i++)
        if ( Raw_orders[i]->obookid == obookid )
            break;
    return(i);
}

struct raw_orders *find_raw_orders(uint64_t obookid)
{
    int32_t i;
    i = _find_raw_orders(obookid);
    if ( i == Num_raw_orders )
        return(0);
    else return(Raw_orders[i]);
}

int32_t update_orderbook_tx(int dir,uint64_t obookid,struct orderbook_tx *tx,uint64_t txid)
{
    int32_t i;
    struct raw_orders *raw;
    // jl777 need to make sure calls are serialized
    i = _find_raw_orders(obookid);
    if ( i == Num_raw_orders )
    {
        if ( Num_raw_orders >= Max_raw_orders )
        {
            Max_raw_orders = Num_raw_orders+1;
            Raw_orders = (struct raw_orders **)realloc(Raw_orders,sizeof(*Raw_orders) * Max_raw_orders);
        }
        raw = (struct raw_orders *)calloc(1,sizeof(*Raw_orders[Num_raw_orders]));
        if ( tx->baseid < tx->relid )
        {
            raw->assetA = tx->baseid;
            raw->assetB = tx->relid;
        }
        else
        {
            raw->assetA = tx->relid;
            raw->assetB = tx->baseid;
        }
        raw->obookid = obookid;
        Raw_orders[Num_raw_orders++] = raw;
    }
    else
    {
        raw = Raw_orders[i];
        if ( (raw->assetA == tx->baseid && raw->assetB == tx->relid) || (raw->assetA == tx->relid && raw->assetB == tx->baseid) )
        {
        }
        else
        {
            printf("obookid collision?? %llx: raw->baseid %llx != %llx tx->baseid || raw->relid %llx != %llx tx->relid\n",(long long)obookid,(long long)raw->assetA,(long long)tx->baseid,(long long)raw->assetB,(long long)tx->relid);
            return(-1);
        }
    }
    if ( dir > 0 )
        return(append_orderbook_tx(raw,tx,txid));
    else if ( dir < 0 )
        return(delete_orderbook_tx(raw,tx));
    else if ( dir == 0 && tx->baseamount == 0 && tx->relamount == 0 )
        return(0);
    else printf("update_orderbook_tx illegal dir.%d\n",dir);
    return(-2);
}

cJSON *create_orderbooks_json()
{
    int32_t i;
    char numstr[64];
    struct raw_orders *raw;
    cJSON *json,*item,*array = 0;
    json = cJSON_CreateObject();
    for (i=0; i<Num_raw_orders; i++)
    {
        raw = Raw_orders[i];
        if ( raw != 0 )
        {
            item = cJSON_CreateObject();
            expand_nxt64bits(numstr,raw->obookid);
            cJSON_AddItemToObject(item,"orderbookid",cJSON_CreateString(numstr));
            expand_nxt64bits(numstr,raw->assetA);
            cJSON_AddItemToObject(item,"assetA",cJSON_CreateString(numstr));
            expand_nxt64bits(numstr,raw->assetB);
            cJSON_AddItemToObject(item,"assetB",cJSON_CreateString(numstr));
            cJSON_AddItemToObject(item,"numorders",cJSON_CreateNumber(raw->num));
            cJSON_AddItemToObject(item,"maxorders",cJSON_CreateNumber(raw->max));
            if ( array == 0 )
                array = cJSON_CreateArray();
            cJSON_AddItemToArray(array,item);
        }
    }
    cJSON_AddItemToObject(json,"numbooks",cJSON_CreateNumber(Num_raw_orders));
    cJSON_AddItemToObject(json,"maxbooks",cJSON_CreateNumber(Num_raw_orders));
    if ( array != 0 )
        cJSON_AddItemToObject(json,"orderbooks",array);
    return(json);
}

cJSON *create_order_json(struct quote *qp,int32_t polarity,int32_t allflag)
{
    char NXTaddr[64],numstr[64];
    cJSON *array = 0;
    double price,vol;
    uint64_t baseamount,relamount;
    if ( qp != 0 )
    {
        if ( polarity > 0 )
        {
            baseamount = qp->baseamount;
            relamount = qp->relamount;
        }
        else
        {
            baseamount = qp->relamount;
            relamount = qp->baseamount;
        }
        if ( baseamount != 0 && relamount != 0 )
        {
            vol = (double)baseamount / SATOSHIDEN;
            price = (double)relamount / baseamount;
            sprintf(numstr,"%.11f",price);
            array = cJSON_CreateArray();
            cJSON_AddItemToArray(array,cJSON_CreateString(numstr));
            sprintf(numstr,"%.8f",vol);
            cJSON_AddItemToArray(array,cJSON_CreateString(numstr));
            if ( allflag != 0 )
            {
                cJSON_AddItemToArray(array,cJSON_CreateNumber(qp->type));
                expand_nxt64bits(NXTaddr,qp->nxt64bits);
                cJSON_AddItemToArray(array,cJSON_CreateString(NXTaddr));
            }
        }
    }
    return(array);
}

struct quote *shrink_and_sort(int32_t polarity,struct quote *quotes,int32_t n,int32_t max)
{
    if ( n != 0 )
    {
        quotes = (struct quote *)realloc(quotes,n * sizeof(*quotes));
        if ( polarity > 0 )
            qsort(quotes,n,sizeof(*quotes),_decreasing_float);
        else qsort(quotes,n,sizeof(*quotes),_increasing_float);
    }
    else free(quotes), quotes = 0;
    return(quotes);
}

int32_t is_orderbook_bid(int32_t polarity,struct raw_orders *raw,struct orderbook_tx *tx)
{
    int32_t dir;
    if ( raw->assetA == tx->baseid && raw->assetB == tx->relid )
        dir = 1;
    else if ( raw->assetA == tx->relid && raw->assetB == tx->baseid )
        dir = -1;
    else dir = 0;
    return(dir * polarity);
}

struct orderbook *create_orderbook(uint64_t obookid,int32_t polarity)
{
    int32_t i,n,dir;
    struct quote *bids,*asks,*qp;
    struct orderbook *op = 0;
    struct orderbook_tx **orders,*tx;
    struct raw_orders *raw;
    if ( (raw= find_raw_orders(obookid)) != 0 )
    {
        orders = clone_orderptrs(&n,raw);
        if ( orders != 0 )
        {
            op = (struct orderbook *)calloc(1,sizeof(*op));
            op->assetA = raw->assetA;
            op->assetB = raw->assetB;
            op->polarity = polarity;
            bids = (struct quote *)calloc(n,sizeof(*bids));
            asks = (struct quote *)calloc(n,sizeof(*asks));
            if ( n != 0 )
            {
                for (i=0; i<n; i++)
                {
                    if ( (tx= orders[i]) != 0 && tx->baseamount != 0 && tx->relamount != 0 )
                    {
                        if ( (dir= is_orderbook_bid(polarity,raw,tx)) > 0 )
                        {
                            qp = &bids[op->numbids++];
                            qp->baseamount = tx->baseamount;
                            qp->relamount = tx->relamount;
                        }
                        else if ( dir < 0 )
                        {
                            qp = &asks[op->numasks++];
                            qp->baseamount = tx->relamount;
                            qp->relamount = tx->baseamount;
                        }
                        else
                        {
                            display_orderbook_tx(tx);
                            printf("create_orderbook: unexpected non-dir quote.%p (%llu) base.%llu | (%llu) rel.%llu\n",tx,(long long)tx->baseid,(long long)tx->baseamount,(long long)tx->relid,(long long)tx->relamount);
                            continue;
                        }
                        printf("tx.%p dir.%d base.%llu rel.%llu\n",tx,dir,(long long)tx->baseid,(long long)tx->relid);
                        qp->type = tx->type;
                        qp->nxt64bits = tx->nxt64bits;
                        qp->price = (float)((double)qp->relamount / qp->baseamount);
                    }
                }
            }
            free(orders);
            op->bids = shrink_and_sort(1,bids,op->numbids,n);
            op->asks = shrink_and_sort(-1,asks,op->numasks,n);
        }
    }
    return(op);
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

int32_t init_orderbook_tx(int32_t polarity,struct orderbook_tx *tx,int32_t type,uint64_t nxt64bits,uint64_t obookid,uint64_t baseamount,uint64_t relamount)
{
    struct raw_orders *raw;
    memset(tx,0,sizeof(*tx));
    tx->sig = ORDERBOOK_SIG;
    tx->type = type;
    tx->nxt64bits = nxt64bits;
    if ( (raw= find_raw_orders(obookid)) != 0 )
    {
        if ( polarity > 0 )
        {
            tx->baseid = raw->assetA;
            tx->relid = raw->assetB;
            tx->baseamount = baseamount;
            tx->relamount = relamount;
        }
        else
        {
            tx->baseamount = relamount;
            tx->relamount = baseamount;
            tx->baseid = raw->assetB;
            tx->relid = raw->assetA;
        }
        return(0);
    }
    return(-1);
}

//{"success":true,"message":"","result":{"buy":[{"Quantity":48.06620614,"Rate":0.00055003},{"Quantity":280.91122887,"Rate":0.00055000},{"Quantity":500.00000000,"Rate":0.00054626},{"Quantity":70.26866440,"Rate":0.00054620},{"Quantity":65.00000000,"Rate":0.00054400}],"sell":[{"Quantity":9.76044214,"Rate":0.00057051},{"Quantity":19.63144898,"Rate":0.00057054},{"Quantity":1.00000000,"Rate":0.00057455},{"Quantity":1.00000000,"Rate":0.00057469},{"Quantity":1.00000000,"Rate":0.00057489}]}}

int32_t bid_orderbook_tx(struct orderbook_tx *tx,int32_t type,uint64_t nxt64bits,uint64_t obookid,double price,double volume)
{
    uint64_t baseamount,relamount;
    baseamount = volume * SATOSHIDEN;
    relamount = (price * baseamount);
    printf("bid base.%llu rel.%llu\n",(long long)baseamount,(long long)relamount);
    return(init_orderbook_tx(1,tx,type,nxt64bits,obookid,baseamount,relamount));
}

int32_t ask_orderbook_tx(struct orderbook_tx *tx,int32_t type,uint64_t nxt64bits,uint64_t obookid,double price,double volume)
{
    uint64_t baseamount,relamount;
    baseamount = volume * SATOSHIDEN;
    relamount = (price * baseamount);
    printf("ask base.%llu rel.%llu\n",(long long)baseamount,(long long)relamount);
    return(init_orderbook_tx(-1,tx,type,nxt64bits,obookid,baseamount,relamount));
}

uint64_t create_raw_orders(uint64_t assetA,uint64_t assetB)
{
    uint64_t obookid;
    struct orderbook_tx tx;
    obookid = assetA ^ assetB;
    init_orderbook_tx(1,&tx,0,0,obookid,0,0);
    tx.baseid = assetA;
    tx.relid = assetB;
    update_orderbook_tx(0,obookid,&tx,0);
    return(obookid);
}

char *sellpNXT(char *NXTaddr,char *NXTACCTSECRET,double amount)
{
    char buf[1024];
    uint64_t satoshis;
    struct NXT_acct *np;
    np = find_NXTacct(NXTaddr,NXTACCTSECRET);
    satoshis = (amount * SATOSHIDEN);
    sprintf(buf,"NXT.%s sell %.8f NXT for pNXT",NXTaddr,dstr(satoshis));
    return(clonestr(buf));
}

char *buypNXT(char *NXTaddr,char *NXTACCTSECRET,double amount)
{
    char buf[1024];
    uint64_t satoshis;
    struct NXT_acct *np;
    np = find_NXTacct(NXTaddr,NXTACCTSECRET);
    satoshis = (amount * SATOSHIDEN);
    sprintf(buf,"NXT.%s buy %.8f pNXT for NXT",NXTaddr,dstr(satoshis));
    return(clonestr(buf));
}

char *send_pNXT(char *NXTaddr,char *NXTACCTSECRET,double amount,int32_t level,char *dest,char *paymentid)
{
    char buf[1024];
    uint64_t satoshis;
    struct NXT_acct *np;
    np = find_NXTacct(NXTaddr,NXTACCTSECRET);
    satoshis = (amount * SATOSHIDEN);
    sprintf(buf,"send %.8f pNXT from NXT.%s to %s paymentid.(%s) using level.%d",dstr(satoshis),NXTaddr,dest,paymentid,level);
    return(clonestr(buf));
}

char *privatesend(char *NXTaddr,char *NXTACCTSECRET,double amount,char *dest,char *coinstr)
{
    char buf[1024];
    uint64_t satoshis;
    struct NXT_acct *np;
    np = find_NXTacct(NXTaddr,NXTACCTSECRET);
    satoshis = (amount * SATOSHIDEN);
    sprintf(buf,"privatesend %.8f NXT from NXT.%s to NXT.%s",dstr(satoshis),NXTaddr,dest);
    return(clonestr(buf));
}

char *getpubkey(char *addr)
{
    char buf[4096],pubkey[128];
    struct NXT_acct *pubnp;
    pubnp = search_addresses(addr);
    init_hexbytes(pubkey,pubnp->pubkey,sizeof(pubnp->pubkey));
    sprintf(buf,"{\"pubkey\":\"%s\",\"NXT\":\"%s\",\"BTCD\":\"%s\",\"pNXT\":\"%s\",\"BTC\":\"%s\"}",pubkey,pubnp->H.NXTaddr,pubnp->BTCDaddr,pubnp->pNXTaddr,pubnp->BTCaddr);
    return(clonestr(buf));
}

char *publishaddrs(char *NXTaddr,char *NXTACCTSECRET,char *pubkey,char *BTCDaddr,char *BTCaddr,char *pNXTaddr)
{
    int32_t createdflag;
    struct NXT_acct *np;
    struct other_addr *op;
    np = find_NXTacct(NXTaddr,NXTACCTSECRET);
    if ( pubkey != 0 && pubkey[0] != 0 )
        decode_hex(np->pubkey,(int32_t)sizeof(np->pubkey),pubkey);
    if ( BTCDaddr[0] != 0 )
    {
        safecopy(np->BTCDaddr,BTCDaddr,sizeof(np->BTCDaddr));
        op = MTadd_hashtable(&createdflag,Global_mp->otheraddrs_tablep,BTCDaddr),op->nxt64bits = np->H.nxt64bits;
    }
    if ( BTCaddr[0] != 0 )
    {
        safecopy(np->BTCaddr,BTCaddr,sizeof(np->BTCaddr));
        op = MTadd_hashtable(&createdflag,Global_mp->otheraddrs_tablep,BTCaddr),op->nxt64bits = np->H.nxt64bits;
    }
    if ( pNXTaddr[0] != 0 )
    {
        safecopy(np->pNXTaddr,pNXTaddr,sizeof(np->pNXTaddr));
        op = MTadd_hashtable(&createdflag,Global_mp->otheraddrs_tablep,pNXTaddr),op->nxt64bits = np->H.nxt64bits;
    }
    return(getpubkey(NXTaddr));
}

char *checkmessages(char *NXTaddr,char *NXTACCTSECRET,char *senderNXTaddr)
{
    char *str;
    struct NXT_acct *np;
    queue_t *msgs;
    cJSON *json,*array = 0;
    json = cJSON_CreateObject();
    if ( senderNXTaddr != 0 && senderNXTaddr[0] != 0 )
    {
        np = find_NXTacct(NXTaddr,NXTACCTSECRET);
        msgs = &np->incoming;
    }
    else msgs = &ALL_messages;
    while ( (str= queue_dequeue(msgs)) != 0 ) //queue_size(msgs) > 1 &&
    {
        if ( array == 0 )
            array = cJSON_CreateArray();
        //printf("add str.(%s) size.%d\n",str,queue_size(msgs));
        cJSON_AddItemToArray(array,cJSON_CreateString(str));
        free(str);
        str = 0;
    }
    if ( array != 0 )
        cJSON_AddItemToObject(json,"messages",array);
    str = cJSON_Print(json);
    free_json(json);
    return(str);
}

char *sendmessage(char *verifiedNXTaddr,char *NXTACCTSECRET,char *msg,int32_t msglen,char *destNXTaddr,char *origargstr)
{
    char buf[4096];
    unsigned char encoded[2048],encoded2[2048],finalbuf[2048],*outbuf;
    int32_t len,createdflag;
    struct NXT_acct *np = 0,*destnp;
    //printf("sendmessage.(%s) -> NXT.(%s) (%s) (%s)\n",NXTaddr,destNXTaddr,msg,origargstr);
    if ( Server_NXTaddr == 0 )
    {
        if ( Global_pNXT->privacyServer_NXTaddr[0] == 0 )
        {
            sprintf(buf,"{\"error\":\"%s cant sendmessage.(%s) to null privacyServer\"}",verifiedNXTaddr,msg);
            return(clonestr(buf));
        }
        np = get_NXTacct(&createdflag,Global_mp,Global_pNXT->privacyServer_NXTaddr);
    }
    else if ( strcmp(Server_NXTaddr,destNXTaddr) == 0 )
    {
        np = get_NXTacct(&createdflag,Global_mp,verifiedNXTaddr);
        queue_message(np,msg,origargstr);
        sprintf(buf,"{\"result\":\"msg.(%s) from NXT.%s queued\"}",msg,verifiedNXTaddr);
        return(clonestr(buf));
    }
    destnp = get_NXTacct(&createdflag,Global_mp,destNXTaddr);
    memset(finalbuf,0,sizeof(finalbuf));
    memset(encoded,0,sizeof(encoded));
    memset(encoded2,0,sizeof(encoded2));
    if ( origargstr != 0 )
        len = onionize(verifiedNXTaddr,NXTACCTSECRET,encoded,destNXTaddr,(unsigned char *)origargstr,(int32_t)strlen(origargstr)+1);
    else
    {
        len = onionize(verifiedNXTaddr,NXTACCTSECRET,encoded,destNXTaddr,(unsigned char *)msg,msglen);
        msg = origargstr = "<encrypted>";
    }
    printf("sendmessage (%s) len.%d to %s\n",origargstr,msglen,destNXTaddr);
    if ( len > sizeof(finalbuf)-256 )
    {
        printf("sendmessage, payload too big %d\n",len);
        sprintf(buf,"{\"error\":\"%s cant sendmessage.(%s) to %s too long.%d\"}",verifiedNXTaddr,msg,destNXTaddr,len);
    }
    else if ( len > 0 )
    {
        outbuf = encoded;
        printf("np.%p np->udp %p destnpudp.%p\n",np,np!=0?np->udp:0,destnp->udp);
        if ( np != 0 && np->udp != 0 && destnp->udp == 0 )
        {
            printf("Must use indirection\n");
            len = onionize(verifiedNXTaddr,NXTACCTSECRET,encoded2,Global_pNXT->privacyServer_NXTaddr,encoded,len);
            outbuf = encoded2;
            sprintf(buf,"{\"status\":\"%s sends via %s encrypted sendmessage.(%s) [%s] to %s pending\"}",verifiedNXTaddr,Global_pNXT->privacyServer_NXTaddr,origargstr,msg,destNXTaddr);
        }
        else if ( destnp->udp != 0 )
        {
            printf("can do direct!\n");
            np = destnp;
            sprintf(buf,"{\"status\":\"%s sends direct encrypted sendmessage.(%s) [%s] to %s pending\"}",verifiedNXTaddr,origargstr,msg,destNXTaddr);
        } else np = 0;  // have to use p2p network
        if ( len > 0 )
        {
            len = crcize(finalbuf,outbuf,len);
            if ( len > sizeof(finalbuf) )
            {
                printf("sendmessage: len.%d > sizeof(finalbuf) %ld\n",len,sizeof(finalbuf));
                exit(-1);
            }
            if ( np != 0 )
            {
                printf("udpsend finalbuf.%d\n",len);
                portable_udpwrite(&np->Uaddr,(uv_udp_t *)np->udp,finalbuf,len,ALLOCWR_ALLOCFREE);
            }
            else if ( Server_NXTaddr != 0 ) // test to verify this is hub
            {
                printf("Server_NXTaddr.(%s) broadcast %d via p2p\n",Server_NXTaddr,len);
                if ( pNXT_submit_tx(Global_pNXT->core,Global_pNXT->wallet,finalbuf,len) == 0 )
                {
                    sprintf(buf,"{\"error\":\"%s cant send via p2p sendmessage.(%s) [%s] to %s pending\"}",verifiedNXTaddr,origargstr,msg,destNXTaddr);
                }
                else
                {
                    sprintf(buf,"{\"status\":\"%s sends via p2p encrypted sendmessage.(%s) [%s] to %s pending\"}",verifiedNXTaddr,origargstr,msg,destNXTaddr);
                }
            }
            else sprintf(buf,"{\"error\":\"%s cant sendmessage.(%s) to %s unexpected case\"}",verifiedNXTaddr,msg,destNXTaddr);
        } else sprintf(buf,"{\"error\":\"%s cant sendmessage.(%s) to %s error encoding 2nd layer\"}",verifiedNXTaddr,msg,destNXTaddr);
    } else sprintf(buf,"{\"error\":\"%s cant sendmessage.(%s) to %s probably no pubkey\"}",verifiedNXTaddr,msg,destNXTaddr);
    return(clonestr(buf));
}

char *processutx(char *sender,char *utx,char *sig,char *full)
{
    char *parsed,buf[1024];
    if ( (parsed = issue_parseTransaction(0,utx)) != 0 )
    {
        
    }
    sprintf(buf,"{\"error\":\"%s cant parse processutx.(%s)\"}",sender,utx);
    return(0);
}

char *makeoffer(char *verifiedNXTaddr,char *NXTACCTSECRET,char *otherNXTaddr,uint64_t assetA,double qtyA,uint64_t assetB,double qtyB,int32_t type)
{
    char buf[1024],signedtx[1024],utxbytes[1024],sighash[65],fullhash[65],_tokbuf[4096];
    struct NXT_tx T,*tx;
    long i,n;
    uint64_t nxt64bits,other64bits,assetoshisA,assetoshisB;
    find_NXTacct(verifiedNXTaddr,NXTACCTSECRET);
    nxt64bits = calc_nxt64bits(verifiedNXTaddr);
    other64bits = calc_nxt64bits(otherNXTaddr);
    assetoshisA = calc_assetoshis(assetA,qtyA);
    assetoshisB = calc_assetoshis(assetB,qtyB);
    //printf("assetoshis A %llu B %llu\n",(long long)assetoshisA,(long long)assetoshisB);
    if ( assetoshisA == 0 || assetoshisB == 0 || assetA == assetB )
    {
        sprintf(buf,"{\"error\":\"%s\",\"descr\":\"NXT.%llu makeoffer to NXT.%s %.8f asset.%llu for %.8f asset.%llu, type.%d\"","illegal parameter",(long long)nxt64bits,otherNXTaddr,dstr(assetoshisA),(long long)assetA,dstr(assetoshisB),(long long)assetB,type);
        return(clonestr(buf));
    }
    set_NXTtx(nxt64bits,&T,assetA,assetoshisA,other64bits);
    sprintf(T.comment,"{\"assetB\":\"%llu\",\"qtyB\":\"%llu\"}",(long long)assetB,(long long)assetoshisB);
    tx = sign_NXT_tx(utxbytes,signedtx,NXTACCTSECRET,nxt64bits,&T,0,1.);
    if ( tx != 0 )
    {
        init_hexbytes(sighash,tx->sighash,sizeof(tx->sighash));
        init_hexbytes(fullhash,tx->fullhash,sizeof(tx->fullhash));
        sprintf(buf,"{\"requestType\":\"processutx\",\"NXT\":\"%s\",\"utx\":\"%s\",\"sig\":\"%s\",\"full\":\"%s\",\"time\":%ld}",verifiedNXTaddr,utxbytes,sighash,fullhash,time(NULL));
        free(tx);
        if ( 0 )
        {
            n = strlen(utxbytes);
            for (i=n; i<n+128; i++)
                utxbytes[i] = '0';
            utxbytes[i] = 0;
            char *tmp = issue_parseTransaction(0,utxbytes);
            printf("(%s)\n",tmp);
            free(tmp);
        }
        n = construct_tokenized_req(_tokbuf,buf,NXTACCTSECRET);
        //stripwhite_ns(buf,strlen(buf));
        //issue_generateToken(0,encoded,buf,NXTACCTSECRET);
        //encoded[NXT_TOKEN_LEN] = 0;
        //sprintf(_tokbuf,"[%s,{\"token\":\"%s\"}]",buf,encoded);
       // printf("(%s) -> (%s) _tokbuf.[%s]\n",NXTaddr,otherNXTaddr,_tokbuf);
        return(sendmessage(verifiedNXTaddr,NXTACCTSECRET,_tokbuf,(int32_t)n+1,otherNXTaddr,buf));
    }
    else sprintf(buf,"{\"error\":\"%s\",\"descr\":\"%s\",\"comment\":\"NXT.%llu makeoffer to NXT.%s %.8f asset.%llu for %.8f asset.%llu, type.%d\"",utxbytes,signedtx,(long long)nxt64bits,otherNXTaddr,dstr(assetoshisA),(long long)assetA,dstr(assetoshisB),(long long)assetB,type);
    return(clonestr(buf));
}

uint64_t calc_txid(unsigned char *hash,long hashsize)
{
    uint64_t txid = 0;
    if ( hashsize >= sizeof(txid) )
        memcpy(&txid,hash,sizeof(txid));
    else memcpy(&txid,hash,hashsize);
    printf("calc_txid.(%llu)\n",(long long)txid);
    return(txid);
}

uint64_t is_orderbook_tx(unsigned char *tx,int32_t size)
{
    struct orderbook_tx *otx = (struct orderbook_tx *)tx;
    if ( size == sizeof(*otx) && otx->sig == ORDERBOOK_SIG )
    {
        if ( otx->baseid != otx->relid )
            return(otx->baseid ^ otx->relid);
    }
    return(0);
}

uint64_t add_jl777_tx(void *origptr,unsigned char *tx,int32_t size,unsigned char *hash,long hashsize)
{
    int i;
    char retjsonstr[4096];
    uint64_t txid = 0;
    uint64_t obookid;
    display_orderbook_tx((struct orderbook_tx *)tx);
    for (i=0; i<size; i++)
        printf("%02x ",tx[i]);
    printf("C add_jl777_tx.%p size.%d hashsize.%ld NXT.%llu\n",tx,size,hashsize,(long long)ORDERBOOK_NXTID);
    txid = calc_txid(hash,hashsize);
    if ( is_encrypted_packet(tx,size) != 0 )
    {
        process_packet(retjsonstr,0,0,tx,size,0,0,0,0,0);
    }
    else
    {
        if ( (obookid= is_orderbook_tx(tx,size)) != 0 )
        {
            if ( update_orderbook_tx(1,obookid,(struct orderbook_tx *)tx,txid) == 0 )
                ((struct orderbook_tx *)tx)->txid = txid;
        }
    }
    return(txid);
}

void remove_jl777_tx(void *tx,int32_t size)
{
    uint64_t obookid;
    printf("C remove_jl777_tx.%p size.%d\n",tx,size);
    if ( (obookid= is_orderbook_tx(tx,size)) != 0 )
        update_orderbook_tx(-1,obookid,(struct orderbook_tx *)tx,0);
}

#endif
