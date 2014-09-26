//
//  orders.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_orders_h
#define xcode_orders_h


#define ORDERBOOK_SIG 0x83746783

#define ORDERBOOK_FEED 1
#define NUM_PRICEDATA_SPLINES 17
#define MAX_TRADEBOT_BARS 512

#define LEFTMARGIN 0
#define TIMEIND_PIXELS 60
#define MAX_LOOKAHEAD 60
#define NUM_ACTIVE_PIXELS ((32*24) - MAX_LOOKAHEAD)
#define MAX_ACTIVE_WIDTH (NUM_ACTIVE_PIXELS + TIMEIND_PIXELS)
#define NUM_REQFUNC_SPLINES 32
#define MAX_SCREENWIDTH 2048
#define MAX_AMPLITUDE 100.
#ifndef MIN
#define MIN(x,y) (((x)<=(y)) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x,y) (((x)>=(y)) ? (x) : (y))
#endif

//#define calc_predisplinex(startweekind,clumpsize,weekind) (((weekind) - (startweekind))/(clumpsize))
struct tradebot_ptrs
{
    char base[64],rel[64];
    uint32_t jdatetime;
    int maxbars,numbids,numasks;
    uint64_t bidnxt[MAX_TRADEBOT_BARS],asknxt[MAX_TRADEBOT_BARS];
    double bids[MAX_TRADEBOT_BARS],asks[MAX_TRADEBOT_BARS],inv_bids[MAX_TRADEBOT_BARS],inv_asks[MAX_TRADEBOT_BARS];
    double bidvols[MAX_TRADEBOT_BARS],askvols[MAX_TRADEBOT_BARS],inv_bidvols[MAX_TRADEBOT_BARS],inv_askvols[MAX_TRADEBOT_BARS];
    float m1[MAX_TRADEBOT_BARS * NUM_BARPRICES],m2[MAX_TRADEBOT_BARS * NUM_BARPRICES],m3[MAX_TRADEBOT_BARS * NUM_BARPRICES],m4[MAX_TRADEBOT_BARS * NUM_BARPRICES],m5[MAX_TRADEBOT_BARS * NUM_BARPRICES],m10[MAX_TRADEBOT_BARS * NUM_BARPRICES],m15[MAX_TRADEBOT_BARS * NUM_BARPRICES],m30[MAX_TRADEBOT_BARS * NUM_BARPRICES],h1[MAX_TRADEBOT_BARS * NUM_BARPRICES];
    float inv_m1[MAX_TRADEBOT_BARS * NUM_BARPRICES],inv_m2[MAX_TRADEBOT_BARS * NUM_BARPRICES],inv_m3[MAX_TRADEBOT_BARS * NUM_BARPRICES],inv_m4[MAX_TRADEBOT_BARS * NUM_BARPRICES],inv_m5[MAX_TRADEBOT_BARS * NUM_BARPRICES],inv_m10[MAX_TRADEBOT_BARS * NUM_BARPRICES],inv_m15[MAX_TRADEBOT_BARS * NUM_BARPRICES],inv_m30[MAX_TRADEBOT_BARS * NUM_BARPRICES],inv_h1[MAX_TRADEBOT_BARS * NUM_BARPRICES];
};

struct filtered_buf
{
	double coeffs[512],projden[512],emawts[512];
	double buf[256+256],projbuf[256+256],prevprojbuf[256+256],avebuf[(256+256)/16];
	double slopes[4];
	double emadiffsum,lastval,lastave,diffsum,Idiffsum,refdiffsum,RTsum;
	int32_t middlei,len;
};

struct price_data
{
    double lastprice;
    struct exchange_quote *allquotes;
    uint32_t *display,firstjdatetime,lastjdatetime,calctime;
    uint64_t obookid;
    char base[64],rel[64];
    struct tradebot_ptrs PTRS;
    struct filtered_buf bidfb,askfb,avefb,slopefb,accelfb;
    float *bars,avebar[NUM_BARPRICES],highbids[MAX_ACTIVE_WIDTH],lowasks[MAX_ACTIVE_WIDTH],aveprices[MAX_ACTIVE_WIDTH];
    double displine[MAX_ACTIVE_WIDTH],dispslope[MAX_ACTIVE_WIDTH],dispaccel[MAX_ACTIVE_WIDTH];
    double splineprices[MAX_ACTIVE_WIDTH],slopes[MAX_ACTIVE_WIDTH],accels[MAX_ACTIVE_WIDTH];
    double *pixeltimes,timefactor,aveprice,absslope,absaccel,avedisp,aveslope,aveaccel,bidsum,asksum,halfspread;
    double dSplines[NUM_PRICEDATA_SPLINES][4],jdatetimes[NUM_PRICEDATA_SPLINES],splinevals[NUM_PRICEDATA_SPLINES];
    int32_t numquotes,maxquotes,screenwidth,screenheight,numsplines,polarity;
};

struct orderbook_tx
{
    uint32_t sig,type;
    uint64_t txid,nxt64bits,baseid,relid;
    uint64_t baseamount,relamount;
};

struct quote
{
    double price,vol;    // must be first!!
    uint32_t type;
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
    int32_t num,max;
} **Raw_orders;

struct exchange_quote { uint32_t jdatetime; float highbid,lowask; };
#define EXCHANGE_QUOTES_INCR ((int32_t)(4096L / sizeof(struct exchange_quote)))
#define INITIAL_PIXELTIME 60

struct exchange_state
{
    double bidminmax[2],askminmax[2],hbla[2];
    char name[64],url[512],url2[512],base[64],rel[64],lbase[64],lrel[64];
    int32_t updated,polarity,numbids,numasks,numbidasks;
    uint32_t type,writeflag;
    uint64_t obookid,feedid,baseid,relid,basemult,relmult;
    struct price_data P;
    FILE *fp;
    double lastmilli;
    queue_t ordersQ;
    //struct orderbook_tx **orders;
};

#define _extrapolate_Spline(Spline,gap) ((double)(Spline[0]) + ((gap) * ((double)(Spline[1]) + ((gap) * ((double)(Spline[2]) + ((gap) * (double)(Spline[3])))))))
#define _extrapolate_Slope(Spline,gap) ((double)(Spline[1]) + ((gap) * ((double)(Spline[2]) + ((gap) * (double)(Spline[3])))))
#define _extrapolate_Accel(Spline,gap) ((double)(Spline[2]) + ((gap) * ((double)(Spline[3]))))

struct madata
{
	double sum;
	double ave,slope,diff,pastanswer;
	double signchange_slope,dirchange_slope,answerchange_slope,diffchange_slope;
	double oldest,lastval,accel2,accel;
	int32_t numitems,maxitems,next,maid;
	int32_t changes,slopechanges,accelchanges,diffchanges;
	int32_t signchanges,dirchanges,answerchanges;
	char signchange,dirchange,answerchange,islogprice;
	double RTvals[20],derivs[4],derivbufs[4][12];
	struct madata **stored;
#ifdef INSIDE_OPENCL
	int32_t pad;
#endif
	double rotbuf[];
};

int Num_raw_orders,Max_raw_orders,Num_price_datas;
struct price_data **Price_datas;

#include "feeds.h"

struct price_data *get_price_data(uint64_t obookid)
{
    int32_t i = 0;
    if ( Num_price_datas > 0 )
    {
        for (i=0; i<Num_price_datas; i++)
            if ( Price_datas[i]->obookid == obookid )
                return(Price_datas[i]);
    }
    Num_price_datas++;
    Price_datas = realloc(Price_datas,sizeof(*Price_datas) * Num_price_datas);
    Price_datas[i] = calloc(1,sizeof(*Price_datas[i]));
    Price_datas[i]->obookid = obookid;
    return(Price_datas[i]);
}

void purge_price_data(struct price_data *dp)
{
    freep((void **)&dp->allquotes);
    freep((void **)&dp->display);
    freep((void **)&dp->pixeltimes);
    freep((void **)&dp->bars);
    memset(dp,0,sizeof(*dp));
}

void free_orderbook(struct orderbook *op)
{
    if ( op != 0 )
    {
        if ( op->bids != 0 )
            free(op->bids);
        if ( op->asks != 0 )
            free(op->asks);
        //purge_price_data(&op->P);
        free(op);
    }
}

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

struct orderbook_tx **clone_orderptrs(int32_t *nump,struct raw_orders *raw,struct orderbook_tx **feedorders,int32_t numfeeds)
{
    int32_t i,m,n=0;
    struct orderbook_tx *tx,**orders = 0;
    // jl777 lock/unlock
    if ( (m= raw->num) != 0 || (feedorders != 0 && numfeeds != 0) )
    {
        if ( feedorders == 0 )
            numfeeds = 0;
        orders = (struct orderbook_tx **)malloc((m + numfeeds) * sizeof(*orders));
        if ( m > 0 )
        {
            for (i=0; i<m; i++)
            {
                if ( (tx= raw->orders[i]) != 0 )
                    orders[n++] = tx;
                else printf("null tx raw->orders[%d]\n",i);
            }
        }
        if ( numfeeds > 0 )
        {
            for (i=0; i<numfeeds; i++)
            {
                if ( (tx= feedorders[i]) != 0 )
                    orders[n++] = tx;
                else printf("null tx feedorders[%d]\n",i);
            }
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
            qsort(quotes,n,sizeof(*quotes),_decreasing_double);
        else qsort(quotes,n,sizeof(*quotes),_increasing_double);
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

void sort_orderbook(struct orderbook *op,struct orderbook_tx **orders,int32_t n,struct raw_orders *raw)
{
    struct quote *bids,*asks,*qp;
    int32_t i,dir;
    struct orderbook_tx *tx;
    bids = (struct quote *)calloc(n,sizeof(*bids));
    asks = (struct quote *)calloc(n,sizeof(*asks));
    if ( n != 0 )
    {
        for (i=0; i<n; i++)
        {
            if ( (tx= orders[i]) != 0 && tx->baseamount != 0 && tx->relamount != 0 )
            {
                if ( (dir= is_orderbook_bid(op->polarity,raw,tx)) > 0 )
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
                qp->type = tx->type;
                qp->nxt64bits = tx->nxt64bits;
                qp->price = ((double)qp->relamount / qp->baseamount);
                //printf("tx.%p dir.%d base.%llu rel.%llu price %.8f vol %.8f\n",tx,dir,(long long)tx->baseid,(long long)tx->relid,qp->price,((double)qp->baseamount / SATOSHIDEN));
            }
        }
    }
    op->bids = shrink_and_sort(op->polarity,bids,op->numbids,n);
    op->asks = shrink_and_sort(-op->polarity,asks,op->numasks,n);
}

struct orderbook *create_orderbook(uint64_t obookid,int32_t polarity,struct orderbook_tx **feedorders,int32_t numfeeds)
{
    int32_t n;
    struct orderbook *op = 0;
    struct orderbook_tx **orders;
    struct raw_orders *raw;
    if ( (raw= find_raw_orders(obookid)) != 0 )
    {
        orders = clone_orderptrs(&n,raw,feedorders,numfeeds);
        if ( orders != 0 )
        {
            op = (struct orderbook *)calloc(1,sizeof(*op));
            op->assetA = raw->assetA;
            op->assetB = raw->assetB;
            op->polarity = polarity;
            sort_orderbook(op,orders,n,raw);
            free(orders);
        }
    }
    return(op);
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
    //printf("bid base.%llu rel.%llu price %.8f vol %.8f\n",(long long)baseamount,(long long)relamount,price,volume);
    return(init_orderbook_tx(1,tx,type,nxt64bits,obookid,baseamount,relamount));
}

int32_t ask_orderbook_tx(struct orderbook_tx *tx,int32_t type,uint64_t nxt64bits,uint64_t obookid,double price,double volume)
{
    uint64_t baseamount,relamount;
    baseamount = volume * SATOSHIDEN;
    relamount = (price * baseamount);
    //printf("ask base.%llu rel.%llu price %.8f vol %.8f\n",(long long)baseamount,(long long)relamount,price,volume);
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

void set_peer_json(char *buf,char *NXTaddr,struct NXT_acct *pubnp)
{
    char pubkey[128],srvipaddr[64],srvnxtaddr[64],coinsjson[1024];
    struct peerinfo *pi = &pubnp->mypeerinfo;
    expand_ipbits(srvipaddr,pi->srvipbits);
    expand_nxt64bits(srvnxtaddr,pi->srvnxtbits);
    init_hexbytes(pubkey,pi->pubkey,sizeof(pi->pubkey));
    _coins_jsonstr(coinsjson,pi->coins);
    sprintf(buf,"{\"requestType\":\"publishaddrs\",\"NXT\":\"%s\",\"pubkey\":\"%s\",\"pubNXT\":\"%s\",\"pubBTCD\":\"%s\",\"pubBTC\":\"%s\",\"time\":%ld,\"srvNXTaddr\":\"%s\",\"srvipaddr\":\"%s\",\"srvport\":\"%d\"%s}",NXTaddr,pubkey,pubnp->H.U.NXTaddr,pi->pubBTCD,pi->pubBTC,time(NULL),srvnxtaddr,srvipaddr,pi->srvport,coinsjson);
}

char *getpubkey(char *NXTaddr,char *NXTACCTSECRET,char *pubaddr,char *destcoin)
{
    char buf[4096];
    struct NXT_acct *pubnp;
    printf("in getpubkey(%s)\n",pubaddr);
    pubnp = search_addresses(pubaddr);
    if ( pubnp != 0 )
    {
        set_peer_json(buf,NXTaddr,pubnp);
        return(clonestr(buf));
    } else return(clonestr("{\"error\":\"cant find pubaddr\"}"));
}

char *sendpeerinfo(char *hopNXTaddr,char *NXTaddr,char *NXTACCTSECRET,char *destaddr,char *destcoin)
{
    char buf[4096];
    struct NXT_acct *pubnp,*destnp;
    printf("in sendpeerinfo(%s)\n",destaddr);
    pubnp = search_addresses(NXTaddr);
    destnp = search_addresses(destaddr);
    if ( pubnp != 0 && destnp != 0 )
    {
        set_peer_json(buf,NXTaddr,pubnp);
        if ( hopNXTaddr != 0 )
        {
            printf("SENDPEERINFO >>>>>>>>>> (%s)\n",buf);
            hopNXTaddr[0] = 0;
            return(send_tokenized_cmd(hopNXTaddr,0,NXTaddr,NXTACCTSECRET,buf,destnp->H.U.NXTaddr));
        } else return(0);
    } else return(clonestr("{\"error\":\"sendpeerinfo cant find pubaddr\"}"));
}

void say_hello(struct NXT_acct *np)
{
    struct coin_info *cp = get_coin_info("BTCD");
    char srvNXTaddr[64],hopNXTaddr[64],*retstr;
    struct NXT_acct *hopnp;
    int32_t createflag;
    //printf("in say_hello.cp %p\n",cp);
    expand_nxt64bits(srvNXTaddr,cp->srvpubnxtbits);
    if ( (retstr= sendpeerinfo(hopNXTaddr,srvNXTaddr,cp->srvNXTACCTSECRET,np->H.U.NXTaddr,0)) != 0 )
    {
        printf("say_hello.(%s)\n",retstr);
        if ( hopNXTaddr[0] != 0 )
        {
            hopnp = get_NXTacct(&createflag,Global_mp,hopNXTaddr);
            update_peerstate(&np->mypeerinfo,&hopnp->mypeerinfo,PEER_HELLOSTATE,PEER_SENT);
        }
        else printf("say_hello no hopNXTaddr?\n");
        free(retstr);
    } else printf("say_hello error sendpeerinfo?\n");
}

void ack_hello(struct NXT_acct *np,struct sockaddr *prevaddr)
{
    struct coin_info *cp = get_coin_info("BTCD");
    char srvNXTaddr[64];
    expand_nxt64bits(srvNXTaddr,cp->srvpubnxtbits);
    printf("ack_hello to %s\n",np->H.U.NXTaddr);
}

uint64_t broadcast_publishpacket(char *ip_port)
{
    char cmd[MAX_JSON_FIELD*4],packet[MAX_JSON_FIELD*4];
    int32_t len,createdflag;
    struct NXT_acct *np;
    struct coin_info *cp = get_coin_info("BTCD");
    if ( cp != 0 )
    {
        np = get_NXTacct(&createdflag,Global_mp,cp->srvNXTADDR);
        set_peer_json(cmd,np->H.U.NXTaddr,np);
        len = construct_tokenized_req(packet,cmd,cp->srvNXTACCTSECRET);
        return(call_SuperNET_broadcast(ip_port,packet,len+1,PUBADDRS_MSGDURATION));
    }
    else return(0);
}

char *publishaddrs(struct sockaddr *prevaddr,uint64_t coins[4],char *NXTACCTSECRET,char *pubNXT,char *pubkeystr,char *BTCDaddr,char *BTCaddr,char *srvNXTaddr,char *srvipaddr,int32_t srvport)
{
    int32_t createdflag,updatedflag = 0;
    struct NXT_acct *np;
    struct coin_info *cp;
    struct other_addr *op;
    struct peerinfo *refpeer,peer;
    char verifiedNXTaddr[64],mysrvNXTaddr[64];
    unsigned char pubkey[crypto_box_PUBLICKEYBYTES];
    uint64_t pubnxtbits;
    cp = get_coin_info("BTCD");
    np = get_NXTacct(&createdflag,Global_mp,pubNXT);
    pubnxtbits = calc_nxt64bits(np->H.U.NXTaddr);
    if ( (refpeer= find_peerinfo(pubnxtbits,BTCDaddr,BTCaddr)) != 0 )
    {
        safecopy(refpeer->pubBTCD,BTCDaddr,sizeof(refpeer->pubBTCD));
        safecopy(refpeer->pubBTC,BTCaddr,sizeof(refpeer->pubBTC));
        if ( pubkeystr != 0 && pubkeystr[0] != 0 )
        {
            memset(pubkey,0,sizeof(pubkey));
            decode_hex(pubkey,(int32_t)sizeof(pubkey),pubkeystr);
            if ( memcmp(refpeer->pubkey,pubkey,sizeof(refpeer->pubkey)) != 0 )
            {
                memcpy(refpeer->pubkey,pubkey,sizeof(refpeer->pubkey));
                updatedflag = 1;
            }
        }
        if ( srvport != 0 )
            refpeer->srvport = srvport;
        if ( srvipaddr != 0 && strcmp(srvipaddr,"127.0.0.1") != 0 )
            refpeer->srvipbits = calc_ipbits(srvipaddr);
        if ( srvNXTaddr != 0 && srvNXTaddr[0] != 0 )
            refpeer->srvnxtbits = calc_nxt64bits(srvNXTaddr);
        printf("found %s and updated.%d %s | coins.%p\n",pubNXT,updatedflag,np->H.U.NXTaddr,coins);
    }
    else
    {
        set_pubpeerinfo(srvNXTaddr,srvipaddr,srvport,&peer,BTCDaddr,pubkeystr,pubnxtbits,BTCaddr);
        refpeer = update_peerinfo(&createdflag,&peer);
        printf("created path for (%s) | coins.%p\n",pubNXT,coins);
    }
    if ( refpeer != 0 )
    {
        if ( coins != 0 )
            memcpy(refpeer->coins,coins,sizeof(refpeer->coins));
        else if ( cp != 0 && cp->pubnxtbits == refpeer->pubnxtbits )
            memcpy(refpeer->coins,Global_mp->coins,sizeof(refpeer->coins));
        printf("set coins.%llx\n",(long long)coins[0]);
    }
    np->mypeerinfo = *refpeer;
    //printf("in secret.(%s) publishaddrs.(%s) np.%p %llu\n",NXTACCTSECRET,pubNXT,np,(long long)np->H.nxt64bits);
    if ( BTCDaddr[0] != 0 )
    {
        //safecopy(np->BTCDaddr,BTCDaddr,sizeof(np->BTCDaddr));
        op = MTadd_hashtable(&createdflag,Global_mp->otheraddrs_tablep,BTCDaddr),op->nxt64bits = np->H.nxt64bits;
        printf("op.%p for %s\n",op,BTCDaddr);
    }
    if ( BTCaddr != 0 && BTCaddr[0] != 0 )
    {
        //safecopy(np->BTCaddr,BTCaddr,sizeof(np->BTCaddr));
        op = MTadd_hashtable(&createdflag,Global_mp->otheraddrs_tablep,BTCaddr),op->nxt64bits = np->H.nxt64bits;
    }
    if ( prevaddr != 0 )
    {
        if ( updatedflag != 0 )
            say_hello(np);
        return(0);
    }
    verifiedNXTaddr[0] = 0;
    np = find_NXTacct(verifiedNXTaddr,NXTACCTSECRET);
    expand_nxt64bits(mysrvNXTaddr,np->mypeerinfo.srvnxtbits);
    //if ( strcmp(np->H.U.NXTaddr,pubNXT) == 0 || strcmp(np->H.U.NXTaddr,srvNXTaddr) == 0 || strcmp(srvNXTaddr,mysrvNXTaddr) == 0 ) // this is this node
    {
        if ( strcmp(srvNXTaddr,pubNXT) == 0 )
        {
            strcpy(verifiedNXTaddr,srvNXTaddr);
            if ( cp != 0 )
                strcpy(NXTACCTSECRET,cp->srvNXTACCTSECRET);
            broadcast_publishpacket(0);
        }
    }
    return(getpubkey(verifiedNXTaddr,NXTACCTSECRET,pubNXT,0));
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
        msgs = &np->incomingQ;
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

char *processutx(char *sender,char *utx,char *sig,char *full)
{
    //PARSED OFFER.({"sender":"8989816935121514892","timestamp":20810867,"height":2147483647,"amountNQT":"0","verify":false,"subtype":1,"attachment":{"asset":"7631394205089352260","quantityQNT":"1000","comment":"{\"assetB\":\"1639299849328439538\",\"qtyB\":\"1000000\"}"},"recipientRS":"NXT-CWEE-VXCV-697E-9YKJT","feeNQT":"100000000","senderPublicKey":"25c5fed2690701cf06f267e7c227b1a3c0dfa9c6fc3cdb593b3af6f16d65302f","type":2,"deadline":720,"senderRS":"NXT-CWEE-VXCV-697E-9YKJT","recipient":"8989816935121514892"})
    struct NXT_tx T,*tx;
    cJSON *json,*obj,*offerjson,*commentobj;
    struct NXT_tx *offertx;
    struct NXT_acct *np;
    int32_t createdflag;
    double vol,price,amountB;
    uint64_t qtyB,assetB;
    char *jsonstr,*parsed,hopNXTaddr[64],buf[1024],calchash[1024],NXTaddr[64],responseutx[1024],signedtx[1024],otherNXTaddr[64];
    jsonstr = issue_calculateFullHash(0,utx,sig);
    if ( jsonstr != 0 )
    {
        json = cJSON_Parse(jsonstr);
        if ( json != 0 )
        {
            obj = cJSON_GetObjectItem(json,"fullHash");
            copy_cJSON(calchash,obj);
            if ( strcmp(calchash,full) == 0 )
            {
                if ( (parsed = issue_parseTransaction(0,utx)) != 0 )
                {
                    printf("PARSED OFFER.(%s) full.(%s) (%s)\n",parsed,full,calchash);
                    if ( (offerjson= cJSON_Parse(parsed)) != 0 )
                    {
                        offertx = set_NXT_tx(offerjson);
                        expand_nxt64bits(otherNXTaddr,offertx->senderbits);
                        vol = conv_assetoshis(offertx->assetidbits,offertx->U.quantityQNT);
                        if ( vol != 0. && offertx->comment[0] != 0 )
                        {
                            commentobj = cJSON_Parse(offertx->comment);
                            if ( commentobj != 0 )
                            {
                                assetB = get_satoshi_obj(commentobj,"assetB");
                                qtyB = get_satoshi_obj(commentobj,"qtyB");
                                amountB = conv_assetoshis(assetB,qtyB);
                                price = (amountB / vol);
                                // jl777 need to make sure we want to do this trade
                                free_json(commentobj);
                                set_NXTtx(offertx->recipientbits,&T,assetB,qtyB,offertx->senderbits);
                                sprintf(T.comment,"{\"assetA\":\"%llu\",\"qtyA\":\"%llu\"}",(long long)offertx->assetidbits,(long long)offertx->U.quantityQNT);
                                expand_nxt64bits(NXTaddr,offertx->recipientbits);
                                np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
                                if ( np->NXTACCTSECRET[0] != 0 )
                                {
                                    tx = sign_NXT_tx(responseutx,signedtx,np->NXTACCTSECRET,offertx->recipientbits,&T,full,1.);
                                    if ( tx != 0 )
                                    {
                                        sprintf(buf,"{\"requestType\":\"respondtx\",\"NXT\":\"%s\",\"signedtx\":\"%s\",\"time\":%ld}",NXTaddr,signedtx,time(NULL));
                                        send_tokenized_cmd(hopNXTaddr,0,NXTaddr,np->NXTACCTSECRET,buf,otherNXTaddr);
                                        free(tx);
                                        sprintf(buf,"{\"results\":\"utx from NXT.%llu accepted with fullhash.(%s) %.8f of %llu for %.8f of %llu -> price %.11f\"}",(long long)offertx->senderbits,full,vol,(long long)offertx->assetidbits,amountB,(long long)assetB,price);
                                    }
                                    else sprintf(buf,"{\"error\":\"from %s error signing responsutx.(%s)\"}",otherNXTaddr,NXTaddr);
                                }
                                else sprintf(buf,"{\"error\":\"cant send response to %s, no access to acct %s\"}",otherNXTaddr,np->H.U.NXTaddr);
                            }
                            else sprintf(buf,"{\"error\":\"%s error parsing comment comment.(%s)\"}",otherNXTaddr,offertx->comment);
                        }
                        else sprintf(buf,"{\"error\":\"%s missing comment.(%s) or zero vol %.8f\"}",otherNXTaddr,parsed,vol);
                        free_json(offerjson);
                    }
                    else sprintf(buf,"{\"error\":\"%s cant json parse offer.(%s)\"}",otherNXTaddr,parsed);
                    free(parsed);
                }
                else sprintf(buf,"{\"error\":\"%s cant parse processutx.(%s)\"}",otherNXTaddr,utx);
            }
            free_json(json);
        }
        else sprintf(buf,"{\"error\":\"cant parse calcfullhash results.(%s) from %s\"}",jsonstr,otherNXTaddr);
        free(jsonstr);
    }
    else sprintf(buf,"{\"error\":\"processutx cant issue calcfullhash\"}");
    return(clonestr(buf));
}

char *respondtx(char *sender,char *signedtx)
{
   // RESPONSETX.({"fullHash":"210f0b0f6e817897929dc4a0a83666246287925c742a6a8a1613626fa5662d16","signatureHash":"3f8e42ba625f78c9f741501a83d86db4ed0dba2af5ab60315a5f7b01d0f8b737","transaction":"10914616006631165729","amountNQT":"0","verify":true,"attachment":{"asset":"7631394205089352260","quantityQNT":"1000","comment":"{\"assetA\":\"7631394205089352260\",\"qtyA\":\"1000\"}"},"recipientRS":"NXT-CWEE-VXCV-697E-9YKJT","type":2,"feeNQT":"100000000","recipient":"8989816935121514892","sender":"8989816935121514892","timestamp":20877092,"height":2147483647,"subtype":1,"senderPublicKey":"25c5fed2690701cf06f267e7c227b1a3c0dfa9c6fc3cdb593b3af6f16d65302f","deadline":720,"senderRS":"NXT-CWEE-VXCV-697E-9YKJT","signature":"005b7022a385932cabb7d3dd2b2d51d585f9b8f3ece8837746209a08d623cc0fb3f3c76107ec3ce01d7bb7093befd0421756bea4b1caa075766733f9a76d4193"})
    char otherNXTaddr[64],NXTaddr[64],buf[1024],*pendingtxbytes;
    uint64_t othertxid,mytxid;
    int32_t createdflag,errcode;
    struct NXT_acct *np,*othernp;
    struct NXT_tx *pendingtx,*recvtx;
    sprintf(buf,"{\"error\":\"some error with respondtx got (%s) from NXT.%s\"}",signedtx,sender);
    recvtx = conv_txbytes(signedtx);
    if ( recvtx != 0 )
    {
        expand_nxt64bits(otherNXTaddr,recvtx->senderbits);
        othernp = get_NXTacct(&createdflag,Global_mp,otherNXTaddr);
        expand_nxt64bits(NXTaddr,recvtx->recipientbits);
        np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
        if ( (pendingtxbytes= othernp->signedtx) != 0 && np->NXTACCTSECRET[0] != 0 )
        {
            pendingtx = conv_txbytes(pendingtxbytes);
            if ( pendingtx != 0 && pendingtx->senderbits == recvtx->recipientbits && pendingtx->recipientbits == recvtx->senderbits )
            {
                if ( recvtx->verify != 0 && memcmp(pendingtx->fullhash,recvtx->refhash,sizeof(pendingtx->fullhash)) == 0 )
                {
                    if ( equiv_NXT_tx(recvtx,pendingtx->comment) == 0 && equiv_NXT_tx(pendingtx,recvtx->comment) == 0 )
                    {
                        sprintf(buf,"{\"error\":\"error broadcasting tx\"}");
                        othertxid = issue_broadcastTransaction(&errcode,0,signedtx,np->NXTACCTSECRET);
                        if ( othertxid != 0 && errcode == 0 )
                        {
                            mytxid = issue_broadcastTransaction(&errcode,0,pendingtxbytes,np->NXTACCTSECRET);
                            if ( mytxid != 0 && errcode == 0 )
                            {
                                sprintf(buf,"{\"result\":\"tradecompleted\",\"txid\":\"%llu\",\"signedtx\":\"%s\",\"othertxid\":\"%llu\"}",(long long)mytxid,pendingtxbytes,(long long)othertxid);
                                free(othernp->signedtx);
                                othernp->signedtx = 0;
                            }
                        }
                    }
                    else sprintf(buf,"{\"error\":\"pendingtx for NXT.%s (%s) doesnt match received tx (%s)\"}",otherNXTaddr,pendingtxbytes,signedtx);
                }
                else sprintf(buf,"{\"error\":\"refhash != fullhash from NXT.%s or unsigned.%d\"}",otherNXTaddr,recvtx->verify);
                free(pendingtx);
            } else sprintf(buf,"{\"error\":\"mismatched sender/recipient NXT.%s <-> NXT.%s\"}",otherNXTaddr,NXTaddr);
        } else sprintf(buf,"{\"error\":\"no pending tx with (%s) or cant access account NXT.%s\"}",otherNXTaddr,NXTaddr);
        free(recvtx);
    }
    return(clonestr(buf));
}

char *makeoffer(char *verifiedNXTaddr,char *NXTACCTSECRET,char *otherNXTaddr,uint64_t assetA,double qtyA,uint64_t assetB,double qtyB,int32_t type)
{
    char hopNXTaddr[64],buf[1024],signedtx[1024],utxbytes[1024],sighash[65],fullhash[65],_tokbuf[4096];
    struct NXT_tx T,*tx;
    struct NXT_acct *np,*othernp;
    long i,n;
    int32_t createdflag;
    uint64_t nxt64bits,other64bits,assetoshisA,assetoshisB;
    hopNXTaddr[0] = 0;
    find_NXTacct(verifiedNXTaddr,NXTACCTSECRET);
    nxt64bits = calc_nxt64bits(verifiedNXTaddr);
    other64bits = calc_nxt64bits(otherNXTaddr);
    assetoshisA = calc_assetoshis(assetA,qtyA);
    assetoshisB = calc_assetoshis(assetB,qtyB);
    othernp = get_NXTacct(&createdflag,Global_mp,otherNXTaddr);
    if ( othernp->signedtx != 0 )
    {
        printf("cancelling preexisting trade with NXT.%s (%s)\n",otherNXTaddr,othernp->signedtx);
        if ( othernp->signedtx != 0 )
            free(othernp->signedtx);
        othernp->signedtx = 0;
    }
    np = get_NXTacct(&createdflag,Global_mp,verifiedNXTaddr);
    if ( np->NXTACCTSECRET[0] == 0 )
        strcpy(np->NXTACCTSECRET,NXTACCTSECRET);
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
        othernp->signedtx = clonestr(signedtx);
        return(sendmessage(hopNXTaddr,0,NXTACCTSECRET,_tokbuf,(int32_t)n+1,otherNXTaddr,_tokbuf));
    }
    else sprintf(buf,"{\"error\":\"%s\",\"descr\":\"%s\",\"comment\":\"NXT.%llu makeoffer to NXT.%s %.8f asset.%llu for %.8f asset.%llu, type.%d\"",utxbytes,signedtx,(long long)nxt64bits,otherNXTaddr,dstr(assetoshisA),(long long)assetA,dstr(assetoshisB),(long long)assetB,type);
    return(clonestr(buf));
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

/*uint64_t add_jl777_tx(void *origptr,unsigned char *tx,int32_t size,unsigned char *hash,long hashsize)
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
        process_packet(retjsonstr,tx,size,0,0,0,0);
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
}*/

#endif
