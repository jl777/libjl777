//
//  jl777.cpp
//  glue code for pNXT
//
//  Created by jl777 on 7/6/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#define SATOSHIDEN 100000000L
#define dstr(x) ((double)(x) / SATOSHIDEN)
#define pNXT_SERVERA_NXTADDR "14841113963360176068"
#define pNXT_SERVERB_NXTADDR "13682911413647488545"
#define CURRENCY_DONATIONS_ADDRESS "1Gx7pfdh8aZRUU9paTc37gcXUWbYxcqbu814DgAdHxdKGAeHLdYHKS13B5SoC9j2Zv9BvkzPik53nS5nyPiiaoDqQpSs6Z1"
#define CURRENCY_ROYALTY_ADDRESS "1JnCpSjCFwTDcDwoU3BJsqUC1kn5EChEpA6Bi5kYfd1qMPCbHddDs8FD2bd2d5BvrG6MKzXLcTQ8JdmnmZ4DaLDYL6FEHv6"

#ifdef INSIDE_CCODE
#ifdef MAINNET
#define PRIVATENXT "8149788036721522865"
#else
#define PRIVATENXT "13533482370298135570"
#endif

int lwsmain(int argc,char **argv);
void *pNXT_get_wallet(char *fname,char *password);
uint64_t pNXT_sync_wallet(void *wallet);
char *pNXT_walletaddr(char *addr,void *wallet);
int32_t pNXT_startmining(void *core,void *wallet);
uint64_t pNXT_rawbalance(void *wallet);
uint64_t pNXT_confbalance(void *wallet);
int32_t pNXT_sendmoney(void *wallet,int32_t numfakes,char *dest,uint64_t amount);
uint64_t pNXT_height(void *core); // declare the wrapper function
void p2p_glue(void *p2psrv);
void rpc_server_glue(void *rpc_server);
void upnp_glue(void *upnp);
uint64_t pNXT_submit_tx(void *m_core,void *wallet,unsigned char *txbytes,int16_t size);

struct hashtable *orderbook_txids;

#include "../NXTservices/coincache.h"
#include "../NXTservices/bitcoind.h"
#include "../NXTservices/atomic.h"
#include "../NXTservices/teleport.h"
#include "../NXTservices/orders.h"
#include "../NXTservices/tradebot.h"
//#include "packets.h"

void set_pNXT_privacyServer_NXTaddr(char *NXTaddr)
{
    if ( Global_pNXT != 0 && NXTaddr != 0 && NXTaddr[0] != 0 )
    {
        strcpy(Global_pNXT->privacyServer_NXTaddr,NXTaddr);
        printf("SETTING PRIVACY SERVER NXT ADDR.(%s) (%s)\n",Global_pNXT->privacyServer_NXTaddr,NXTaddr);
    }
}

void set_pNXT_privacyServer(uint64_t privacyServer)
{
    if ( Global_pNXT != 0 && privacyServer != 0 )
    {
        char tmp[32];
        expand_ipbits(tmp,(uint32_t)privacyServer);
        printf("SETTING PRIVACY SERVER IPADDR.(%s)\n",tmp);
        Global_pNXT->privacyServer = privacyServer;
    }
}

uint64_t get_pNXT_privacyServer(int32_t *activeflagp,char *secret)
{
    uint64_t privacyServer;
    privacyServer = calc_privacyServer(Global_pNXT->privacyServer_ipaddr,atoi(Global_pNXT->privacyServer_port));
    *activeflagp = Global_pNXT->privacyServer == privacyServer;
    char tmp[32];
    expand_ipbits(tmp,(uint32_t)privacyServer);
    strcpy(secret,Global_pNXT->NXTACCTSECRET);
    memset(Global_pNXT->NXTACCTSECRET,0,sizeof(Global_pNXT->NXTACCTSECRET));
    //printf("pNXT ipaddr.(%s) %llx %s\n",Global_pNXT->privacyServer_ipaddr,(long long)privacyServer,tmp);
    return(privacyServer);
}

char *select_privacyServer(char *ipaddr,char *portstr,char *secret)
{
    char buf[1024];
    uint16_t port;
    if ( portstr != 0 && portstr[0] != 0 )
    {
        port = atoi(portstr);
        sprintf(Global_pNXT->privacyServer_port,"%u",port);
    }
    if ( ipaddr[0] != 0 )
        strcpy(Global_pNXT->privacyServer_ipaddr,ipaddr);
    memset(Global_pNXT->NXTACCTSECRET,0,sizeof(Global_pNXT->NXTACCTSECRET));
    strcpy(Global_pNXT->NXTACCTSECRET,secret);
    sprintf(buf,"{\"privacyServer\":\"%s\",\"ipaddr\":\"%s\",\"port\":\"%s\"}",Global_pNXT->privacyServer_NXTaddr,Global_pNXT->privacyServer_ipaddr,Global_pNXT->privacyServer_port);
    return(clonestr(buf));
}

int64_t get_asset_quantity(int64_t *unconfirmedp,char *NXTaddr,char *assetidstr)
{
    char cmd[4096],assetid[512];
    union NXTtype retval;
    int32_t i,n,iter;
    cJSON *array,*item,*obj;
    int64_t quantity,qty;
    quantity = *unconfirmedp = 0;
    sprintf(cmd,"%s=getAccount&account=%s",_NXTSERVER,NXTaddr);
    retval = extract_NXTfield(0,0,cmd,0,0);
    if ( retval.json != 0 )
    {
        for (iter=0; iter<2; iter++)
        {
            qty = 0;
            array = cJSON_GetObjectItem(retval.json,iter==0?"assetBalances":"unconfirmedAssetBalances");
            if ( is_cJSON_Array(array) != 0 )
            {
                n = cJSON_GetArraySize(array);
                for (i=0; i<n; i++)
                {
                    item = cJSON_GetArrayItem(array,i);
                    obj = cJSON_GetObjectItem(item,"asset");
                    copy_cJSON(assetid,obj);
                    //printf("i.%d of %d: %s(%s)\n",i,n,assetid,cJSON_Print(item));
                    if ( strcmp(assetid,assetidstr) == 0 )
                    {
                        qty = get_cJSON_int(item,iter==0?"balanceQNT":"unconfirmedBalanceQNT");
                        break;
                    }
                }
            }
            if ( iter == 0 )
                quantity = qty;
            else *unconfirmedp = qty;
        }
    }
    return(quantity);
}

uint64_t get_privateNXT_balance(char *NXTaddr)
{
    int64_t qty,unconfirmed;
    qty = get_asset_quantity(&unconfirmed,NXTaddr,PRIVATENXT);
    return(qty * 1000000L); // assumes 2 decimal points
}

char *get_pNXT_addr()
{
    if ( Global_pNXT != 0 && Global_pNXT->walletaddr != 0 )
        return(Global_pNXT->walletaddr);
    return("<No pNXT address>");
}

uint64_t get_pNXT_confbalance()
{
    if ( Global_pNXT != 0 && Global_pNXT->wallet != 0 )
        return(pNXT_confbalance(Global_pNXT->wallet));
    return(0);
}

uint64_t get_pNXT_rawbalance()
{
    if ( Global_pNXT != 0 && Global_pNXT->wallet != 0 )
        return(pNXT_rawbalance(Global_pNXT->wallet));
    return(0);
}

void init_pNXT(void *core,void *p2psrv,void *rpc_server,void *upnp,char *NXTACCTSECRET)
{
    //uint64_t amount = 100000000000;
    int32_t i;
    struct NXT_str *tp = 0;
    unsigned char txbytes[512];
    char secret[256];//NXTADDR[128],
    struct pNXT_info *gp;
    secret[0] = 0;
    if ( NXTACCTSECRET != 0 && NXTACCTSECRET[0] != 0 )
        strcpy(secret,NXTACCTSECRET);
    if ( secret[0] == 0 )
        strcpy(secret,"password");
    //init_MGWconf(NXTADDR,secret,Global_mp);
    if ( Global_pNXT == 0 )
    {
        Global_pNXT = calloc(1,sizeof(*Global_pNXT));
        orderbook_txids = hashtable_create("orderbook_txids",HASHTABLES_STARTSIZE,sizeof(struct NXT_str),((long)&tp->txid[0] - (long)tp),sizeof(tp->txid),((long)&tp->modified - (long)tp));
        Global_pNXT->orderbook_txidsp = &orderbook_txids;
        printf("SET ORDERBOOK HASHTABLE %p\n",orderbook_txids);
    }
    gp = Global_pNXT;
    gp->core = core; gp->p2psrv = p2psrv; gp->upnp = upnp; gp->rpc_server = rpc_server;
    if ( gp->wallet == 0 )
    {
        while ( Finished_loading == 0 )
            sleep(1);
        gp->wallet = pNXT_get_wallet("wallet.bin",secret);
    }
    printf("got gp->wallet.%p (%s)\n",gp->wallet,secret);
    if ( gp->wallet != 0 )
    {
        strcpy(gp->walletaddr,"no pNXT address");
        pNXT_walletaddr(gp->walletaddr,gp->wallet);
        printf("got walletaddr (%s)\n",gp->walletaddr);
        for (i=0; i<512; i++)
            txbytes[i] = i;
        //pNXT_submit_tx(gp->core,gp->wallet,txbytes,i);
        //printf("submit tx done\n");
        //pNXT_startmining(gp->core,gp->wallet);
        //pNXT_sendmoney(gp->wallet,0,"1Bs3GNG1ScLQ2GGoK9CMQCAxvZfiyX1JdT8cwQeHCzseSnGD5bLXGgYQkp9k3rJfhN8mJ2sVLA8zkWRoE4HSs9cJMfqxJFj",amount);
    }
}

char *orderbook_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    int32_t i,polarity,allflag;
    uint64_t obookid;
    cJSON *json,*bids,*asks,*item;
    struct orderbook *op;
    char obook[512],buf[512],assetA[64],assetB[64],*retstr = 0;
    obookid = get_API_nxt64bits(objs[1]);
    expand_nxt64bits(obook,obookid);
    polarity = get_API_int(objs[2],1);
    if ( polarity == 0 )
        polarity = 1;
    allflag = get_API_int(objs[3],0);
    if ( obookid != 0 && (op= create_orderbook(obookid,polarity,0,0)) != 0 )
    {
        if ( op->numbids == 0 && op->numasks == 0 )
            retstr = clonestr("{\"error\":\"no bids or asks\"}");
        else
        {
            json = cJSON_CreateObject();
            bids = cJSON_CreateArray();
            for (i=0; i<op->numbids; i++)
            {
                item = create_order_json(&op->bids[i],1,allflag);
                cJSON_AddItemToArray(bids,item);
            }
            asks = cJSON_CreateArray();
            for (i=0; i<op->numasks; i++)
            {
                item = create_order_json(&op->asks[i],1,allflag);
                cJSON_AddItemToArray(asks,item);
            }
            expand_nxt64bits(assetA,op->assetA);
            expand_nxt64bits(assetB,op->assetB);
            cJSON_AddItemToObject(json,"orderbook",cJSON_CreateString(obook));
            cJSON_AddItemToObject(json,"assetA",cJSON_CreateString(assetA));
            cJSON_AddItemToObject(json,"assetB",cJSON_CreateString(assetB));
            cJSON_AddItemToObject(json,"polarity",cJSON_CreateNumber(polarity));
            cJSON_AddItemToObject(json,"bids",bids);
            cJSON_AddItemToObject(json,"asks",asks);
            retstr = cJSON_Print(json);
        }
        free_orderbook(op);
    } else
    {
        sprintf(buf,"{\"error\":\"no such orderbook.(%llu)\"}",(long long)obookid);
        retstr = clonestr(buf);
    }
    return(retstr);
}

char *getorderbooks_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    cJSON *json;
    char *retstr = 0;
    json = create_orderbooks_json();
    if ( json != 0 )
    {
        retstr = cJSON_Print(json);
        free_json(json);
    }
    else retstr = clonestr("{\"error\":\"no orderbooks\"}");
    return(retstr);
}

char *selectserver_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    int32_t n = 0;
    char ipaddr[512],port[512],secret[512],*retstr = 0;
    copy_cJSON(ipaddr,objs[1]);
    copy_cJSON(port,objs[2]);
    copy_cJSON(secret,objs[3]);
    if ( sender[0] != 0 && valid != 0 )
    {
        retstr = select_privacyServer(ipaddr,port,secret);
        while ( n++ < 1000 && (retstr= queue_dequeue(&RPC_6777_response)) == 0 )
            usleep(10000);
        if ( n == 1000 )
            printf("TIMEOUT: selectserver_func no response\n");
    }
    return(retstr);
}

char *placequote_func(int32_t dir,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    int32_t polarity;
    uint64_t obookid,nxt64bits,assetA,assetB,txid;
    double price,volume;
    struct orderbook_tx tx;
    char buf[1024],txidstr[512],*retstr = 0;
    nxt64bits = calc_nxt64bits(sender);
    obookid = get_API_nxt64bits(objs[1]);
    polarity = get_API_int(objs[2],1);
    if ( polarity == 0 )
        polarity = 1;
    volume = get_API_float(objs[3]);
    price = get_API_float(objs[4]);
    assetA = get_API_nxt64bits(objs[5]);
    assetB = get_API_nxt64bits(objs[6]);
    printf("PLACE QUOTE polarity.%d dir.%d\n",polarity,dir);
    if ( sender[0] != 0 && find_raw_orders(obookid) != 0 && valid != 0 )
    {
        if ( price != 0. && volume != 0. && dir != 0 )
        {
            if ( dir*polarity > 0 )
                bid_orderbook_tx(&tx,0,nxt64bits,obookid,price,volume);
            else ask_orderbook_tx(&tx,0,nxt64bits,obookid,price,volume);
            txid = pNXT_submit_tx(Global_pNXT->core,Global_pNXT->wallet,(unsigned char *)&tx,sizeof(tx));
            if ( txid != 0 )
            {
                expand_nxt64bits(txidstr,txid);
                sprintf(buf,"{\"txid\":\"%s\"}",txidstr);
                retstr = clonestr(buf);
            }
        }
        if ( retstr == 0 )
        {
            sprintf(buf,"{\"error submitting\":\"place%s error obookid.%llx polarity.%d volume %f price %f\"}",dir>0?"bid":"ask",(long long)obookid,polarity,volume,price);
            retstr = clonestr(buf);
        }
    }
    else if ( assetA != 0 && assetB != 0 && assetA != assetB )
    {
        if ( (obookid= create_raw_orders(assetA,assetB)) != 0 )
        {
            sprintf(buf,"{\"obookid\":\"%llu\"}",(long long)obookid);
            retstr = clonestr(buf);
        }
        else
        {
            sprintf(buf,"{\"error\":\"couldnt create orderbook for assets %llu and %llu\"}",(long long)assetA,(long long)assetB);
            retstr = clonestr(buf);
        }
    }
    else
    {
        sprintf(buf,"{\"error\":\"place%s error obookid.%llx polarity.%d volume %f price %f\"}",polarity>0?"bid":"ask",(long long)obookid,polarity,volume,price);
        retstr = clonestr(buf);
    }
    return(retstr);
}

char *placebid_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    return(placequote_func(1,sender,valid,objs,numobjs,origargstr));
}

char *placeask_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    return(placequote_func(-1,sender,valid,objs,numobjs,origargstr));
}

char *sellpNXT_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    double amount;
    char NXTACCTSECRET[512],*retstr = 0;
    amount = get_API_float(objs[1]);
    copy_cJSON(NXTACCTSECRET,objs[2]);
    if ( sender[0] != 0 && amount > 0 && valid != 0 )
        retstr = sellpNXT(sender,NXTACCTSECRET,amount);
    else retstr = clonestr("{\"error\":\"invalid sellpNXT request\"}");
    return(retstr);
}

char *buypNXT_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    double amount;
    char NXTACCTSECRET[512],*retstr = 0;
    amount = get_API_float(objs[1]);
    copy_cJSON(NXTACCTSECRET,objs[2]);
    if ( sender[0] != 0 && amount > 0 && valid != 0 )
        retstr = buypNXT(sender,NXTACCTSECRET,amount);
    else retstr = clonestr("{\"error\":\"invalid buypNXT request\"}");
    return(retstr);
}

char *sendpNXT_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    double amount;
    int32_t level;
    char NXTACCTSECRET[512],paymentid[256],dest[256],*retstr = 0;
    amount = get_API_float(objs[1]);
    copy_cJSON(NXTACCTSECRET,objs[2]);
    copy_cJSON(dest,objs[3]);
    level = get_API_int(objs[4],0);
    copy_cJSON(paymentid,objs[5]);
    if ( sender[0] != 0 && amount > 0 && valid != 0 && dest[0] != 0 )
        retstr = send_pNXT(sender,NXTACCTSECRET,amount,level,dest,paymentid);
    else retstr = clonestr("{\"error\":\"invalid send request\"}");
    return(retstr);
}

char *teleport_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    //static char *teleport[] = { (char *)teleport_func, "teleport", "V", "NXT", "secret", "amount", "dest", "coin", "minage", 0 };
    double amount;
    int32_t M,N;
    struct coin_info *cp;
    char NXTACCTSECRET[512],destaddr[512],minage[512],coinstr[512],*retstr = 0;
    if ( Historical_done == 0 )
        return(clonestr("historical processing is not done yet"));
    copy_cJSON(NXTACCTSECRET,objs[1]);
    amount = get_API_float(objs[2]);
    copy_cJSON(destaddr,objs[3]);
    copy_cJSON(coinstr,objs[4]);
    copy_cJSON(minage,objs[5]);
    M = get_API_int(objs[6],1);
    N = get_API_int(objs[7],1);
    if ( M > N || N >= 0xff || M <= 0 )
        M = N = 1;
    printf("amount.(%.8f) minage.(%s) %d | M.%d N.%d\n",amount,minage,atoi(minage),M,N);
    cp = get_coin_info(coinstr);
    if ( cp != 0 && sender[0] != 0 && amount > 0 && valid != 0 && destaddr[0] != 0 )
        retstr = teleport(sender,NXTACCTSECRET,(uint64_t)(SATOSHIDEN * amount),destaddr,cp,atoi(minage),M,N);
    else retstr = clonestr("{\"error\":\"invalid teleport request\"}");
    return(retstr);
}

char *sendmsg_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char NXTACCTSECRET[512],destNXTaddr[256],msg[1024],*retstr = 0;
    copy_cJSON(destNXTaddr,objs[1]);
    copy_cJSON(NXTACCTSECRET,objs[2]);
    copy_cJSON(msg,objs[3]);
    //printf("sendmsg_func sender.(%s) valid.%d dest.(%s) (%s)\n",sender,valid,destNXTaddr,origargstr);
    if ( sender[0] != 0 && valid != 0 && destNXTaddr[0] != 0 )
        retstr = sendmessage(sender,NXTACCTSECRET,msg,(int32_t)strlen(msg)+1,destNXTaddr,origargstr);
    else retstr = clonestr("{\"error\":\"invalid sendmessage request\"}");
    return(retstr);
}

char *checkmsg_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char NXTACCTSECRET[512],senderNXTaddr[256],*retstr = 0;
    copy_cJSON(senderNXTaddr,objs[1]);
    copy_cJSON(NXTACCTSECRET,objs[2]);
    if ( sender[0] != 0 && valid != 0 )
        retstr = checkmessages(sender,NXTACCTSECRET,senderNXTaddr);
    else retstr = clonestr("{\"result\":\"invalid checkmessages request\"}");
    return(retstr);
}

char *getpubkey_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char NXTACCTSECRET[512],addr[256],*retstr = 0;
    copy_cJSON(addr,objs[1]);
    copy_cJSON(NXTACCTSECRET,objs[2]);
    if ( sender[0] != 0 && valid != 0 && addr[0] != 0 )
        retstr = getpubkey(sender,NXTACCTSECRET,addr);
    else retstr = clonestr("{\"result\":\"invalid getpubkey request\"}");
    return(retstr);
}

char *publishaddrs_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char NXTACCTSECRET[512],pubNXT[1024],pubkey[1024],BTCDaddr[1024],BTCaddr[1024],pNXTaddr[1024],*retstr = 0;
    copy_cJSON(NXTACCTSECRET,objs[1]);
    copy_cJSON(pubNXT,objs[2]);
    copy_cJSON(pubkey,objs[3]);
    copy_cJSON(BTCDaddr,objs[4]);
    copy_cJSON(BTCaddr,objs[5]);
    copy_cJSON(pNXTaddr,objs[6]);
    if ( sender[0] != 0 && valid != 0 && pubNXT[0] != 0 )
        retstr = publishaddrs(NXTACCTSECRET,pubNXT,pubkey,BTCDaddr,BTCaddr,pNXTaddr);
    else retstr = clonestr("{\"result\":\"invalid publishaddrs request\"}");
    return(retstr);
}

char *makeoffer_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    uint64_t assetA,assetB;
    double qtyA,qtyB;
    int32_t type;
    char NXTACCTSECRET[512],otherNXTaddr[256],*retstr = 0;
    copy_cJSON(NXTACCTSECRET,objs[1]);
    copy_cJSON(otherNXTaddr,objs[2]);
    assetA = get_API_nxt64bits(objs[3]);
    qtyA = get_API_float(objs[4]);
    assetB = get_API_nxt64bits(objs[5]);
    qtyB = get_API_float(objs[6]);
    type = get_API_int(objs[7],0);

    if ( sender[0] != 0 && valid != 0 && otherNXTaddr[0] != 0 )//&& assetA != 0 && qtyA != 0. && assetB != 0. && qtyB != 0. )
        retstr = makeoffer(sender,NXTACCTSECRET,otherNXTaddr,assetA,qtyA,assetB,qtyB,type);
    else retstr = clonestr("{\"result\":\"invalid makeoffer_func request\"}");
    return(retstr);
}

char *processutx_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char NXTACCTSECRET[512],utx[4096],full[1024],sig[1024],*retstr = 0;
    copy_cJSON(NXTACCTSECRET,objs[1]);
    copy_cJSON(utx,objs[2]);
    copy_cJSON(sig,objs[3]);
    copy_cJSON(full,objs[4]);
    if ( sender[0] != 0 && valid != 0 )
        retstr = processutx(sender,utx,sig,full);
    else retstr = clonestr("{\"result\":\"invalid makeoffer_func request\"}");
    return(retstr);
}

char *respondtx_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char signedtx[4096],*retstr = 0;
    copy_cJSON(signedtx,objs[1]);
    if ( sender[0] != 0 && valid != 0 && signedtx[0] != 0 )
        retstr = respondtx(sender,signedtx);
    else retstr = clonestr("{\"result\":\"invalid makeoffer_func request\"}");
    return(retstr);
}

char *tradebot_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    static char *buf;
    static int64_t filelen,allocsize;
    long len;
    cJSON *botjson;
    char NXTACCTSECRET[512],code[4096],retbuf[4096],*str,*retstr = 0;
    copy_cJSON(NXTACCTSECRET,objs[1]);
    copy_cJSON(code,objs[2]);
    printf("tradebotfunc.(%s) sender.(%s) valid.%d code.(%s)\n",origargstr,sender,valid,code);
    if ( sender[0] != 0 && valid != 0 && code[0] != 0 )
    {
        len = strlen(code);
        if ( code[0] == '(' && code[len-1] == ')' )
        {
            code[len-1] = 0;
            str = load_file(code+1,&buf,&filelen,&allocsize);
            if ( str == 0 )
            {
                sprintf(retbuf,"{\"error\":\"cant open (%s)\"}",code+1);
                return(clonestr(retbuf));
            }
        }
        else
        {
            str = code;
            //printf("str is (%s)\n",str);
        }
        if ( str != 0 )
        {
            //printf("before.(%s) ",str);
            replace_singlequotes(str);
            //printf("after.(%s)\n",str);
            if ( (botjson= cJSON_Parse(str)) != 0 )
            {
                retstr = start_tradebot(sender,NXTACCTSECRET,botjson);
                free_json(botjson);
            }
            else
            {
                str[sizeof(retbuf)-128] = 0;
                sprintf(retbuf,"{\"error\":\"couldnt parse (%s)\"}",str);
                printf("%s\n",retbuf);
            }
            if ( str != code )
                free(str);
        }
    }
    else retstr = clonestr("{\"result\":\"invalid tradebot request\"}");
    return(retstr);
}

char *telepod_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    uint64_t satoshis;
    struct coin_info *cp;
    uint32_t crc,ind,height,vout,totalcrc,sharei,M,N;
    char NXTACCTSECRET[1024],coinstr[512],coinaddr[512],otherpubaddr[512],txid[512],pubkey[512],privkey[2048],privkeyhex[2048],*retstr = 0;
    copy_cJSON(NXTACCTSECRET,objs[1]);
    if ( NXTACCTSECRET[0] == 0 && (cp= get_coin_info("BTCD")) != 0 )
        safecopy(NXTACCTSECRET,cp->NXTACCTSECRET,sizeof(NXTACCTSECRET));
    crc = get_API_uint(objs[2],0);
    ind = get_API_uint(objs[3],0);
    height = get_API_uint(objs[4],0);
    copy_cJSON(coinstr,objs[5]);
    satoshis = SATOSHIDEN * get_API_float(objs[6]);
    copy_cJSON(coinaddr,objs[7]);
    copy_cJSON(txid,objs[8]);
    vout = get_API_uint(objs[9],0);
    copy_cJSON(pubkey,objs[10]);
    copy_cJSON(privkeyhex,objs[11]);
    decode_hex((unsigned char *)privkey,(int32_t)strlen(privkeyhex)/2,privkeyhex);
    privkey[strlen(privkeyhex)/2] = 0;
    totalcrc = get_API_uint(objs[12],0);
    sharei = get_API_uint(objs[13],0);
    M = get_API_uint(objs[14],1);
    N = get_API_uint(objs[15],1);
    copy_cJSON(otherpubaddr,objs[16]);
    if ( coinstr[0] != 0 && sender[0] != 0 && valid != 0 )
        retstr = telepod_received(sender,NXTACCTSECRET,coinstr,crc,ind,height,satoshis,coinaddr,txid,vout,pubkey,privkey,totalcrc,sharei,M,N,otherpubaddr);
    else retstr = clonestr("{\"error\":\"invalid telepod received\"}");
    return(retstr);
}

char *transporter_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    uint64_t value;
    struct coin_info *cp;
    uint8_t sharenrs[255];
    uint32_t totalcrc,M,N,minage,height,i,n=0,*crcs = 0;
    char NXTACCTSECRET[1024],sharenrsbuf[1024],coinstr[512],otherpubaddr[512],*retstr = 0;
    copy_cJSON(NXTACCTSECRET,objs[1]);
    if ( NXTACCTSECRET[0] == 0 && (cp= get_coin_info("BTCD")) != 0 )
        safecopy(NXTACCTSECRET,cp->NXTACCTSECRET,sizeof(NXTACCTSECRET));
    copy_cJSON(coinstr,objs[2]);
    height = get_API_uint(objs[3],0);
    minage = get_API_uint(objs[4],0);
    value = (SATOSHIDEN * get_API_float(objs[5]));
    totalcrc = get_API_uint(objs[6],0);
    if ( is_cJSON_Array(objs[7]) != 0 && (n= cJSON_GetArraySize(objs[7])) > 0 )
    {
        crcs = calloc(n,sizeof(*crcs));
        for (i=0; i<n; i++)
            crcs[i] = get_API_uint(cJSON_GetArrayItem(objs[7],i),0);
    }
    M = get_API_int(objs[8],0);
    N = get_API_int(objs[9],0);
    copy_cJSON(sharenrsbuf,objs[10]);
    memset(sharenrs,0,sizeof(sharenrs));
    if ( M <= N && N < 0xff && M > 0 )
        decode_hex(sharenrs,N,sharenrsbuf);
    else M = N = 1;
    copy_cJSON(otherpubaddr,objs[11]);
    printf("transporterstatus_func M.%d N.%d [%s] otherpubaddr.(%s)\n",M,N,sharenrsbuf,otherpubaddr);
    if ( coinstr[0] != 0 && sender[0] != 0 && valid != 0 && n > 0 )
        retstr = transporter_received(sender,NXTACCTSECRET,coinstr,totalcrc,height,value,minage,crcs,n,M,N,sharenrs,otherpubaddr);
    else retstr = clonestr("{\"error\":\"invalid incoming transporter bundle\"}");
    if ( crcs != 0 )
        free(crcs);
    return(retstr);
}

char *transporterstatus_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    uint64_t value;
    cJSON *array;
    struct coin_info *cp;
    int32_t minage;
    uint8_t sharenrs[255];
    uint32_t totalcrc,status,i,sharei,M,N,height,ind,num,n=0,*crcs = 0;
    char NXTACCTSECRET[1024],sharenrsbuf[1024],otherpubaddr[512],coinstr[512],*retstr = 0;
    copy_cJSON(NXTACCTSECRET,objs[1]);
    if ( NXTACCTSECRET[0] == 0 && (cp= get_coin_info("BTCD")) != 0 )
        safecopy(NXTACCTSECRET,cp->NXTACCTSECRET,sizeof(NXTACCTSECRET));
    status = get_API_int(objs[2],0);
    copy_cJSON(coinstr,objs[3]);
    totalcrc = get_API_uint(objs[4],0);
    value = (SATOSHIDEN * get_API_float(objs[5]));
    num = get_API_int(objs[6],0);
    minage = get_API_int(objs[7],0);
    height = get_API_int(objs[8],0);
    array = objs[9];
    if ( is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
    {
        crcs = calloc(n,sizeof(*crcs));
        for (i=0; i<n; i++)
            crcs[i] = get_API_uint(cJSON_GetArrayItem(array,i),0);
    }
    sharei = get_API_int(objs[9],0);
    M = get_API_int(objs[11],0);
    N = get_API_int(objs[12],0);
    copy_cJSON(sharenrsbuf,objs[13]);
    memset(sharenrs,0,sizeof(sharenrs));
    if ( M <= N && N < 0xff && M > 0 )
        decode_hex(sharenrs,N,sharenrsbuf);
    else M = N = 1;
    ind = get_API_int(objs[14],0);
    copy_cJSON(otherpubaddr,objs[15]);
    //printf("transporterstatus_func sharei.%d M.%d N.%d other.(%s)\n",sharei,M,N,otherpubaddr);
    if ( coinstr[0] != 0 && sender[0] != 0 && valid != 0 && num > 0 )
        retstr = got_transporter_status(NXTACCTSECRET,sender,coinstr,status,totalcrc,value,num,crcs,ind,minage,height,sharei,M,N,sharenrs,otherpubaddr);
    else retstr = clonestr("{\"error\":\"invalid incoming transporter status\"}");
    if ( crcs != 0 )
        free(crcs);
    return(retstr);
}

char *pNXT_json_commands(struct NXThandler_info *mp,struct pNXT_info *gp,cJSON *argjson,char *sender,int32_t valid,char *origargstr)
{
    static char *teleport[] = { (char *)teleport_func, "teleport", "V", "NXT", "secret", "amount", "dest", "coin", "minage", "M", "N", 0 };
    static char *telepod[] = { (char *)telepod_func, "telepod", "V", "NXT", "secret", "crc", "i", "h", "c", "v", "a", "t", "o", "p", "k", "L", "s", "M", "N", "D", 0 };
    static char *transporter[] = { (char *)transporter_func, "transporter", "V", "NXT", "secret", "coin", "height", "minage", "value", "totalcrc", "telepods", "M", "N", "sharenrs", "pubaddr", 0 };
    static char *transporterstatus[] = { (char *)transporterstatus_func, "transporter_status", "V", "NXT", "secret", "status", "coin", "totalcrc", "value", "num", "minage", "height", "crcs", "sharei", "M", "N", "sharenrs", "ind", "pubaddr", 0 };
    static char *tradebot[] = { (char *)tradebot_func, "tradebot", "V", "NXT", "secret", "code", 0 };
    static char *respondtx[] = { (char *)respondtx_func, "respondtx", "V", "NXT", "signedtx", 0 };
    static char *processutx[] = { (char *)processutx_func, "processutx", "V", "NXT", "secret", "utx", "sig", "full", 0 };
    static char *publishaddrs[] = { (char *)publishaddrs_func, "publishaddrs", "V", "NXT", "secret", "pubNXT", "pubkey", "BTCD", "BTC", "pNXT", 0 };
    static char *getpubkey[] = { (char *)getpubkey_func, "getpubkey", "V", "NXT", "addr", "secret", 0 };
    static char *sellp[] = { (char *)sellpNXT_func, "sellpNXT", "V", "NXT", "amount", "secret", 0 };
    static char *buyp[] = { (char *)buypNXT_func, "buypNXT", "V", "NXT", "amount", "secret", 0 };
    static char *sendmsg[] = { (char *)sendmsg_func, "sendmessage", "V", "NXT", "dest", "secret", "msg", 0 };
    static char *checkmsg[] = { (char *)checkmsg_func, "checkmessages", "V", "NXT", "sender", "secret", 0 };
    static char *send[] = { (char *)sendpNXT_func, "send", "V", "NXT", "amount", "secret", "dest", "level","paymentid", 0 };
    static char *select[] = { (char *)selectserver_func, "select", "V", "NXT", "ipaddr", "port", "secret", 0 };
    static char *orderbook[] = { (char *)orderbook_func, "orderbook", "V", "NXT", "obookid", "polarity", "allfields", "secret", 0 };
    static char *getorderbooks[] = { (char *)getorderbooks_func, "getorderbooks", "V", "NXT", "secret", 0 };
    static char *placebid[] = { (char *)placebid_func, "placebid", "V", "NXT", "obookid", "polarity", "volume", "price", "assetA", "assetB", "secret", 0 };
    static char *placeask[] = { (char *)placeask_func, "placeask", "V", "NXT", "obookid", "polarity", "volume", "price", "assetA", "assetB", "secret", 0 };
    static char *makeoffer[] = { (char *)makeoffer_func, "makeoffer", "V", "NXT", "secret", "other", "assetA", "qtyA", "assetB", "qtyB", "type", 0 };
    static char **commands[] = { getpubkey, transporterstatus, telepod, transporter, tradebot, respondtx, processutx, publishaddrs, checkmsg, placebid, placeask, makeoffer, sendmsg, orderbook, getorderbooks, sellp, buyp, send, teleport, select  };
    int32_t i,j;
    cJSON *obj,*nxtobj,*objs[64];
    char NXTaddr[64],command[4096],**cmdinfo,*retstr;
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
    printf("%llu pNXT_json_commands sender.(%s) valid.%d | size.%d | command.(%s) orig.(%s)\n",(long long)Global_pNXT->privacyServer,sender,valid,(int32_t)(sizeof(commands)/sizeof(*commands)),command,origargstr);
    for (i=0; i<(int32_t)(sizeof(commands)/sizeof(*commands)); i++)
    {
        cmdinfo = commands[i];
        //printf("needvalid.(%c) sender.(%s) valid.%d %d of %d: cmd.(%s) vs command.(%s)\n",cmdinfo[2][0],sender,valid,i,(int32_t)(sizeof(commands)/sizeof(*commands)),cmdinfo[1],command);
        if ( strcmp(cmdinfo[1],command) == 0 )
        {
            if ( cmdinfo[2][0] != 0 )
            {
                if ( NXTaddr[0] == 0 )
                    strcpy(NXTaddr,sender);
                if ( sender[0] == 0 || valid != 1 || (strcmp(NXTaddr,sender) != 0 && strcmp(sender,Global_pNXT->privacyServer_NXTaddr) != 0) )
                {
                    printf("verification valid.%d missing for %s sender.(%s) vs NXT.(%s)\n",valid,cmdinfo[1],sender,NXTaddr);
                    return(0);
                }
            }
            for (j=3; cmdinfo[j]!=0&&j<3+(int32_t)(sizeof(objs)/sizeof(*objs)); j++)
                objs[j-3] = cJSON_GetObjectItem(argjson,cmdinfo[j]);
            retstr = (*(json_handler)cmdinfo[0])(sender,valid,objs,j-3,origargstr);
            if ( 0 && retstr != 0 )
                printf("json_handler returns.(%s)\n",retstr);
            return(retstr);
        }
    }
    return(0);
}

char *issue_pNXT_json_commands(cJSON *argjson,char *sender,int32_t valid,char *origargstr)
{
    return(pNXT_json_commands(Global_mp,Global_pNXT,argjson,sender,valid,origargstr));
}

char *remove_secret(cJSON **argjsonp,char *parmstxt)
{
    long len;
    cJSON_DeleteItemFromObject(*argjsonp,"secret");
    if ( parmstxt != 0 )
        free(parmstxt);
    parmstxt = cJSON_Print(*argjsonp);
    len = strlen(parmstxt);
    stripwhite_ns(parmstxt,len);
    return(parmstxt);
}

char *pNXT_jsonhandler(cJSON **argjsonp,char *argstr,char *verifiedsender)
{
    struct NXThandler_info *mp = Global_mp;
    long len;
    struct coin_info *cp;
    int32_t valid,firsttime = 1;
    cJSON *secretobj = 0,*json;
    char NXTACCTSECRET[1024],sender[64],*origparmstxt,*parmstxt=0,encoded[NXT_TOKEN_LEN+1],*retstr = 0;
again:
    sender[0] = 0;
    if ( verifiedsender != 0 && verifiedsender[0] != 0 )
        safecopy(sender,verifiedsender,sizeof(sender));
    valid = -1;
    //printf("pNXT_jsonhandler argjson.%p\n",*argjsonp);
    if ( *argjsonp != 0 )
    {
        secretobj = cJSON_GetObjectItem(*argjsonp,"secret");
        copy_cJSON(NXTACCTSECRET,secretobj);
        if ( NXTACCTSECRET[0] == 0 && (cp= get_coin_info("BTCD")) != 0 )
        {
            safecopy(NXTACCTSECRET,cp->NXTACCTSECRET,sizeof(NXTACCTSECRET));
            cJSON_ReplaceItemInObject(*argjsonp,"secret",cJSON_CreateString(NXTACCTSECRET));
            //printf("got cp.%p for BTCD (%s) (%s)\n",cp,cp->NXTACCTSECRET,cJSON_Print(*argjsonp));
        }
        parmstxt = cJSON_Print(*argjsonp);
        len = strlen(parmstxt);
        stripwhite_ns(parmstxt,len);
        //printf("parmstxt.(%s)\n",parmstxt);
    }
    if ( *argjsonp == 0 )
    {
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json,"error",cJSON_CreateString("cant parse"));
        cJSON_AddItemToObject(json,"argstr",cJSON_CreateString(argstr));
        retstr = cJSON_Print(json);
        free_json(json);
        return(retstr);
    }
    else
    {
        origparmstxt = parmstxt;
        parmstxt = verify_tokenized_json(sender,&valid,argjsonp,parmstxt);
    }
    retstr = pNXT_json_commands(mp,Global_pNXT,*argjsonp,sender,valid,argstr);
    //printf("back from pNXT_json_commands\n");
    if ( firsttime != 0 && retstr == 0 && *argjsonp != 0 && parmstxt == 0 )
    {
        cJSON *reqobj;
        uint64_t nxt64bits;
        char _tokbuf[4096],NXTaddr[64],buf[1024];//,*str;
        firsttime = 0;
        reqobj = cJSON_GetObjectItem(*argjsonp,"requestType");
        copy_cJSON(buf,reqobj);
        if ( cJSON_GetObjectItem(*argjsonp,"requestType") != 0 )
            cJSON_ReplaceItemInObject(*argjsonp,"time",cJSON_CreateNumber(time(NULL)));
        else cJSON_AddItemToObject(*argjsonp,"time",cJSON_CreateNumber(time(NULL)));
        nxt64bits = issue_getAccountId(0,NXTACCTSECRET);
        expand_nxt64bits(NXTaddr,nxt64bits);
        cJSON_ReplaceItemInObject(*argjsonp,"NXT",cJSON_CreateString(NXTaddr));
        printf("replace NXT.(%s)\n",NXTaddr);
//#ifndef __linux__
        if ( strcmp(buf,"teleport") != 0 && strcmp(buf,"tradebot") != 0 && strcmp(buf,"makeoffer") != 0 && strcmp(buf,"select") != 0 && strcmp(buf,"checkmessages") != 0 && Global_pNXT->privacyServer != 0 )
        {
            parmstxt = remove_secret(argjsonp,parmstxt);
            issue_generateToken(mp->curl_handle2,encoded,parmstxt,NXTACCTSECRET);
            encoded[NXT_TOKEN_LEN] = 0;
            sprintf(_tokbuf,"[%s,{\"token\":\"%s\"}]",parmstxt,encoded);
            retstr = sendmessage(NXTaddr,NXTACCTSECRET,_tokbuf,(int32_t)strlen(_tokbuf)+1,Global_pNXT->privacyServer_NXTaddr,_tokbuf);
        }
        else
//#endif
        {
            issue_generateToken(mp->curl_handle2,encoded,origparmstxt,NXTACCTSECRET);
            encoded[NXT_TOKEN_LEN] = 0;
            sprintf(_tokbuf,"[%s,{\"token\":\"%s\"}]",origparmstxt,encoded);
            if ( *argjsonp != 0 )
                free_json(*argjsonp);
            *argjsonp = cJSON_Parse(_tokbuf);
            if ( origparmstxt != 0 )
                free(origparmstxt), origparmstxt = 0;
            goto again;
        }
    }
    //printf("free parmstxt.%p, argjson.%p\n",parmstxt,*argjsonp);
    if ( parmstxt != 0 )
        free(parmstxt);
    return(retstr);
}

void process_pNXT_AM(struct pNXT_info *dp,struct NXT_protocol_parms *parms)
{
    cJSON *argjson;
    struct json_AM *ap;
    char NXTaddr[64],*sender,*receiver;
    sender = parms->sender; receiver = parms->receiver; ap = parms->AMptr;
    expand_nxt64bits(NXTaddr,ap->H.nxt64bits);
    if ( strcmp(NXTaddr,sender) != 0 )
    {
        printf("unexpected NXTaddr %s != sender.%s when receiver.%s\n",NXTaddr,sender,receiver);
        return;
    }
    if ( (argjson = parse_json_AM(ap)) != 0 )
    {
        printf("process_pNXT_AM got jsontxt.(%s)\n",ap->jsonstr);
        free_json(argjson);
    }
}

void process_pNXT_typematch(struct pNXT_info *dp,struct NXT_protocol_parms *parms)
{
    char NXTaddr[64],*sender,*receiver,*txid;
    sender = parms->sender; receiver = parms->receiver; txid = parms->txid;
    safecopy(NXTaddr,sender,sizeof(NXTaddr));
    printf("got txid.(%s) type.%d subtype.%d sender.(%s) -> (%s)\n",txid,parms->type,parms->subtype,sender,receiver);
}

void *pNXT_handler(struct NXThandler_info *mp,struct NXT_protocol_parms *parms,void *handlerdata,int32_t height)
{
    struct pNXT_info *gp = handlerdata;
    if ( parms->txid == 0 )     // indicates non-transaction event
    {
        if ( parms->mode == NXTPROTOCOL_WEBJSON )
            return(pNXT_jsonhandler(&parms->argjson,parms->argstr,0));
        else if ( parms->mode == NXTPROTOCOL_NEWBLOCK )
        {
            if ( gp->wallet != 0 && gp->core != 0 )
            {
                printf("pNXT Height: %lld | %s raw %.8f conf %.8f |",(long long)pNXT_height(gp->core),gp->walletaddr!=0?gp->walletaddr:"no wallet address",dstr(pNXT_rawbalance(gp->wallet)),dstr(pNXT_confbalance(gp->wallet)));
                //pNXT_sendmoney(gp->wallet,0,gp->walletaddr,12345678);
            }
            printf("pNXT new RTblock %d time %ld microseconds %lld\n",mp->RTflag,time(0),(long long)microseconds());
        }
        else if ( parms->mode == NXTPROTOCOL_IDLETIME )
        {
            //printf("pNXT new idletime %d time %ld microseconds %lld \n",mp->RTflag,time(0),(long long)microseconds());
        }
        else if ( parms->mode == NXTPROTOCOL_INIT )
        {
            printf("pNXT NXThandler_info init %d\n",mp->RTflag);
            if ( Global_pNXT == 0 )
            {
                struct NXT_str *tp = 0;
                Global_pNXT = calloc(1,sizeof(*Global_pNXT));
                orderbook_txids = hashtable_create("orderbook_txids",HASHTABLES_STARTSIZE,sizeof(struct NXT_str),((long)&tp->txid[0] - (long)tp),sizeof(tp->txid),((long)&tp->modified - (long)tp));
                Global_pNXT->orderbook_txidsp = &orderbook_txids;
                printf("SET ORDERBOOK HASHTABLE %p\n",orderbook_txids);
            }
            gp = Global_pNXT;
            printf("return gp.%p\n",gp);
        }
        return(gp);
    }
    else if ( parms->mode == NXTPROTOCOL_AMTXID )
        process_pNXT_AM(gp,parms);
    else if ( parms->mode == NXTPROTOCOL_TYPEMATCH )
        process_pNXT_typematch(gp,parms);
    return(gp);
}

#ifdef FROM_pNXT
void _init_lws(void *arg)
{
    char *secret;
    void *core,*p2psrv,*rpc_server,*upnp,**ptrs = (void **)arg;
    sleep(3);
    core = ptrs[0]; p2psrv = ptrs[1]; rpc_server = ptrs[2]; upnp = ptrs[3]; secret = (char *)ptrs[4];
    init_pNXT(core,p2psrv,rpc_server,upnp,secret==0?"password":secret);
    p2p_glue(p2psrv);
    rpc_server_glue(rpc_server);
    upnp_glue(upnp);
    printf("finished call lwsmain pNXT.(%p) height.%lld | %p %p %p\n",ptrs[0],(long long)pNXT_height(core),p2psrv,rpc_server,upnp);
}

void _init_lws2(void *arg)
{
    char *argv[2];
    argv[0] = arg;
    argv[1] = 0;
    printf("call lwsmain\n");
    lwsmain(1,argv);
}

void init_lws(void *core,void *p2p,void *rpc_server,void *upnp,char *secret)
{
    static void *ptrs[5];
    if ( strcmp(secret,"exchanges") == 0 )
    {
        init_pNXT(0,0,0,0,secret);
        exit(0);
    }
    curl_global_init(CURL_GLOBAL_ALL); //init the curl session
    ptrs[0] = core; ptrs[1] = p2p; ptrs[2] = rpc_server; ptrs[3] = upnp; ptrs[4] = (void *)secret;
    printf("init_lws(%p %p %p %p)\n",core,p2p,rpc_server,upnp);
    if ( portable_thread_create(_init_lws2,secret) == 0 )
        printf("ERROR launching _init_lws2\n");
    printf("done init_lws2()\n");
    if ( portable_thread_create(_init_lws,ptrs) == 0 )
        printf("ERROR launching _init_lws\n");
    printf("done init_lws()\n");
}

#else
void *pNXT_get_wallet(char *fname,char *password){return(0);}
uint64_t pNXT_sync_wallet(void *wallet){return(0);}
char *pNXT_walletaddr(char *addr,void *wallet){return(0);}
int32_t pNXT_startmining(void *core,void *wallet){return(0);}
uint64_t pNXT_rawbalance(void *wallet){return(0);}
uint64_t pNXT_confbalance(void *wallet){return(0);}
int32_t pNXT_sendmoney(void *wallet,int32_t numfakes,char *dest,uint64_t amount){return(0);}
uint64_t pNXT_height(void *core){return(0);}
uint64_t pNXT_submit_tx(void *m_core,void *wallet,unsigned char *txbytes,int16_t size)
{
    void *tx = malloc(size);
    memcpy(tx,txbytes,size);
    return(add_jl777_tx(0,tx,size,txbytes,8));
}

#endif

#else

#define INSIDE_DAEMON
#include "simplewallet/password_container.cpp"
#include "simplewallet/simplewallet.cpp"
extern "C" void init_lws(currency::core *,void *,void *,void *,char *);
extern "C" uint64_t calc_txid(unsigned char *hash,long hashsize);

extern "C" currency::simple_wallet *pNXT_get_wallet(char *fname,char *password)
{
    currency::simple_wallet *wallet = new(currency::simple_wallet);
    if ( wallet->open_wallet(fname,password) == 0 )
        if ( wallet->new_wallet(fname,password) == 0 )
            free(wallet), wallet = 0;
    if ( wallet != 0 )
        wallet->load_blocks();
    return(wallet);
}

extern "C" void pNXT_sync_wallet(currency::simple_wallet *wallet)
{
    wallet->sync_wallet();
}

extern "C" char *pNXT_walletaddr(char *walletaddr,currency::simple_wallet *wallet)
{
    std::string addr;
    currency::account_public_address acct;
    addr = wallet->get_address(acct);
    printf("inside got wallet addr.(%s)\n",addr.c_str());
    strcpy(walletaddr,addr.c_str());
    return(walletaddr);
}

extern "C" int32_t pNXT_startmining(currency::core *core,currency::simple_wallet *wallet)
{
    int numthreads = 1;
    std::string addr;
    currency::account_public_address acct;
    wallet->sync_wallet();
    addr = wallet->get_address(acct);
    if ( core->get_miner().start(acct,numthreads) == 0 )
    {
        printf("Failed, mining not started for (%s)\n",addr.c_str());
        return(-1);
    }
    else
    {
        wallet->show_balance();
    }
    return(0);
}

extern "C" uint64_t pNXT_rawbalance(currency::simple_wallet *wallet)
{
    return(wallet->get_rawbalance());
}

extern "C" uint64_t pNXT_confbalance(currency::simple_wallet *wallet)
{
    return(wallet->get_confbalance());
}

extern "C" bool pNXT_sendmoney(currency::simple_wallet *wallet,int32_t numfakes,char *dest,uint64_t amount)
{
    std::vector<std::string> args;
    char buf[512];
    args.reserve(3);
    wallet->sync_wallet();
    printf("sending %.8f pNXT to (%s)  ",dstr(amount),dest);
    wallet->show_balance();
    sprintf(buf,"%d",numfakes);
    args.push_back(buf);
    args.push_back(dest);
    sprintf(buf,"%.8f",(double)amount/SATOSHIDEN);
    args.push_back(buf);
    return(wallet->transfer(args));
}

extern "C" uint64_t pNXT_height(currency::core *m)
{
    return(m->get_current_blockchain_height());
}

extern "C" void p2p_glue(nodetool::node_server<currency::t_currency_protocol_handler<currency::core> >* p2psrv)
{
    printf("p2p_glue.%p port.%d\n",p2psrv,p2psrv->get_this_peer_port());
}

extern "C" void rpc_server_glue(currency::core_rpc_server *rpc_server)
{
    printf("rpc_server_glue.%p\n",rpc_server);
}

extern "C" void upnp_glue(tools::miniupnp_helper *upnp)
{
    printf("upnp_glue.%p: lan_addr.(%s)\n",upnp,upnp->pub_lanaddr);
}

int32_t add_byte(transaction *tx,txin_to_key *txin,int32_t offset,unsigned char x)
{
    if ( offset == 0 )
    {
        txin->amount = 0;
        memset(&txin->k_image,0,sizeof(txin->k_image));
    }
    if ( offset < 8 )
        ((unsigned char *)&txin->amount)[offset] = x;
    else ((unsigned char *)&txin->k_image)[offset - 8] = x;
    offset++;
    if ( offset >= 40 )
    {
        tx->vin.push_back(*txin);
        offset = 0;
    }
    return(offset);
}

extern "C" uint64_t pNXT_submit_tx(currency::core *m_core,currency::simple_wallet *wallet,unsigned char *txbytes,int16_t size)
{
    int i,j;
    crypto::hash h;
    uint64_t txid = 0;
    blobdata txb,b;
    transaction tx = AUTO_VAL_INIT(tx);
    txin_to_key input_to_key = AUTO_VAL_INIT(input_to_key);
    NOTIFY_NEW_TRANSACTIONS::request req;
    currency_connection_context fake_context = AUTO_VAL_INIT(fake_context);
    tx_verification_context tvc = AUTO_VAL_INIT(tvc);
    if ( m_core == 0 || wallet == 0 )
    {
        printf("pNXT_submit_tx missing m_core.%p or wallet.%p\n",m_core,wallet);
        return(0);
    }
    tx.vin.clear();
    tx.vout.clear();
    tx.signatures.clear();
    keypair txkey = keypair::generate();
    add_tx_pub_key_to_extra(tx, txkey.pub);
    if ( sizeof(input_to_key.k_image) != 32 )
    {
        printf("FATAL: expected sizeof(input_to_key.k_image) to be 32!\n");
        return(0);
    }
    j = add_byte(&tx,&input_to_key,0,size&0xff);
    j = add_byte(&tx,&input_to_key,j,(size>>8)&0xff);
    for (i=0; i<size; i++)
        j = add_byte(&tx,&input_to_key,j,txbytes[i]);
    if ( j != 0 )
        tx.vin.push_back(input_to_key);
    tx.version = 0;
    txb = tx_to_blob(tx);
    printf("FROM submit jl777\n");
    if ( !m_core->handle_incoming_tx(txb,tvc,false) )
    {
        LOG_PRINT_L0("[on_send_raw_tx]: Failed to process tx");
        return(0);
    }
    if ( tvc.m_verifivation_failed )
    {
        LOG_PRINT_L0("[on_send_raw_tx]: tx verification failed");
        return(0);
    }
    if( !tvc.m_should_be_relayed )
    {
        LOG_PRINT_L0("[on_send_raw_tx]: tx accepted, but not relayed");
        return(0);
    }
    req.txs.push_back(txb);
    m_core->get_protocol()->relay_transactions(req,fake_context);
    get_transaction_hash(tx,h);
    txid = calc_txid((unsigned char *)&h,sizeof(h));
    return(txid);
}


#endif
