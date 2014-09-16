

//
//  jl777.cpp
//  glue code for pNXT
//
//  Created by jl777 on 7/6/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//
#include "jl777.h"

#define SATOSHIDEN 100000000L
#define dstr(x) ((double)(x) / SATOSHIDEN)

struct hashtable *orderbook_txids;

#include "coincache.h"
#include "bitcoind.h"
#include "atomic.h"
#include "teleport.h"
#include "orders.h"
#include "tradebot.h"

char dispstr[65536];
char testforms[1024*1024],PC_USERNAME[512],MY_IPADDR[512];
int Finished_loading,Historical_done,Finished_init;
#define pNXT_SIG 0x99999999

#include "NXTservices.c"
uv_loop_t *UV_loop;

//#include "../../NXTservices/InstantDEX/InstantDEX.h"
//#include "../../NXTservices/multigateway/multigateway.h"
//#include "../../NXTservices/NXTorrent.h"
//#include "../../NXTservices/NXTsubatomic.h"
//#include "../../NXTservices/NXTcoinsco.h"
//#include "NXTmixer.h"
//#include "../NXTservices/NXTprivacy.h"
#include "html.h"

void NXTservices_idler(uv_idle_t *handle)
{
    static int64_t nexttime;
    //void call_handlers(struct NXThandler_info *mp,int32_t mode,int32_t height);
    static uint64_t counter;
    usleep(1000);
    if ( (counter++ % 1000) == 0 && microseconds() > nexttime )
    {
        call_handlers(Global_mp,NXTPROTOCOL_IDLETIME,0);
        nexttime = (microseconds() + 1000000);
    }
}

void run_UVloop(void *arg)
{
    uv_idle_t idler;
    uv_idle_init(UV_loop,&idler);
    //uv_idle_init(UV_loop,&idler);
    uv_idle_start(&idler,NXTprivacy_idler);
    uv_idle_start(&idler,NXTservices_idler);
    uv_run(UV_loop,UV_RUN_DEFAULT);
    printf("end of uv_run\n");
}

void run_NXTservices(void *arg)
{
    void *pNXT_handler(struct NXThandler_info *mp,struct NXT_protocol_parms *parms,void *handlerdata,int32_t height);
    struct NXThandler_info *mp = arg;
    printf("inside run_NXTservices %p\n",mp);
    register_NXT_handler("pNXT",mp,2,NXTPROTOCOL_ILLEGALTYPE,pNXT_handler,pNXT_SIG,1,0,0);
    printf("NXTloop\n");
    NXTloop(mp);
    printf("NXTloop done\n");
    while ( 1 ) sleep(60);
}

void init_NXTservices(char *JSON_or_fname,char *myipaddr)
{
    struct NXThandler_info *mp = Global_mp;    // seems safest place to have main data structure
    char *ipaddr;
    UV_loop = uv_default_loop();
    portable_mutex_init(&mp->hash_mutex);
    portable_mutex_init(&mp->hashtable_queue[0].mutex);
    portable_mutex_init(&mp->hashtable_queue[1].mutex);
    
    init_NXThashtables(mp);
    init_NXTAPI(0);
    ipaddr = 0;
    if ( ipaddr != 0 )
    {
        strcpy(MY_IPADDR,get_ipaddr());
        strcpy(mp->ipaddr,MY_IPADDR);
    }
    safecopy(mp->ipaddr,MY_IPADDR,sizeof(mp->ipaddr));
    mp->upollseconds = 333333 * 0;
    mp->pollseconds = POLL_SECONDS;
    crypto_box_keypair(Global_mp->loopback_pubkey,Global_mp->loopback_privkey);
    crypto_box_keypair(Global_mp->session_pubkey,Global_mp->session_privkey);
    init_hexbytes(Global_mp->pubkeystr,Global_mp->session_pubkey,sizeof(Global_mp->session_pubkey));
    if ( portable_thread_create((void *)process_hashtablequeues,mp) == 0 )
        printf("ERROR hist process_hashtablequeues\n");
    init_MGWconf(JSON_or_fname,myipaddr);
    printf("start getNXTblocks\n");
    if ( 1 && portable_thread_create((void *)getNXTblocks,mp) == 0 )
        printf("ERROR start_Histloop\n");
    //printf("start init_NXTprivacy\n");
    //if ( 0 && portable_thread_create((void *)init_NXTprivacy,"") == 0 )
    //    printf("ERROR init_NXTprivacy\n");
    //printf("start gen_testforms\n");
    //gen_testforms(0);
    
    printf("run_NXTservices >>>>>>>>>>>>>>> %p %s: %s %s\n",mp,mp->dispname,PC_USERNAME,mp->ipaddr);
    void run_NXTservices(void *arg);
    if ( 1 && portable_thread_create((void *)run_NXTservices,mp) == 0 )
        printf("ERROR hist process_hashtablequeues\n");
}

uint64_t broadcast_publishpacket(uint64_t coins[4],struct NXT_acct *np,char *NXTACCTSECRET,char *srvNXTaddr,char *srvipaddr,uint16_t srvport)
{
    struct coin_info *cp;
    char cmd[1024],packet[2048],coinsjson[1024],hexstr[512],*BTCDaddr,*BTCaddr;
    int32_t len;
    init_hexbytes(hexstr,np->mypeerinfo.pubkey,sizeof(np->mypeerinfo.pubkey));
    if ( np != 0 )
    {
        BTCDaddr = np->mypeerinfo.pubBTCD;
        BTCaddr = np->mypeerinfo.pubBTC;
    } else BTCDaddr = BTCaddr = "";
    if ( (cp= get_coin_info("BTCD")) != 0 && cp->pubnxt64bits != 0 )
    {
        _coins_jsonstr(coinsjson,coins);
        sprintf(cmd,"{\"requestType\":\"publishaddrs\",\"NXT\":\"%s\",\"srvipaddr\":\"%s\",\"srvport\":\"%d\",\"srvNXTaddr\":\"%s\",\"pubkey\":\"%s\",\"pubBTCD\":\"%s\",\"pubNXT\":\"%s\",\"pubBTC\":\"%s\"%s}",np->H.NXTaddr,srvipaddr,srvport,srvNXTaddr,hexstr,BTCDaddr,np->H.NXTaddr,BTCaddr,coinsjson);
        len = construct_tokenized_req(packet,cmd,NXTACCTSECRET);
        return(call_libjl777_broadcast(packet,PUBADDRS_MSGDURATION));
    } else printf("broadcast_publishpacket error: no public nxt addr\n");
    return(-1);
}

void set_pNXT_privacyServer_NXTaddr(char *NXTaddr)
{
    uint16_t port;
    uint32_t ipbits;
    int32_t createdflag;
    char ipaddr[32],pubNXTaddr[64];
    struct NXT_acct *mynp;
    struct coin_info *cp;
    if ( Global_pNXT != 0 && NXTaddr != 0 && NXTaddr[0] != 0 )
    {
        strcpy(Global_pNXT->privacyServer_NXTaddr,NXTaddr);
        ipbits = (uint32_t)Global_pNXT->privacyServer;
        port = (uint16_t)(Global_pNXT->privacyServer >> 32);
        expand_ipbits(ipaddr,ipbits);
        cp = get_coin_info("BTCD");
        printf("SETTING PRIVACY SERVER NXT ADDR.(%s) (%s) [%s/%d] pub.(%s) privacyserver.(%s)\n",Global_pNXT->privacyServer_NXTaddr,NXTaddr,ipaddr,port,cp->pubaddr,cp->privacyserver);
        if ( cp != 0 && cp->NXTACCTSECRET[0] != 0 && cp->pubnxt64bits != 0 )
        {
            expand_nxt64bits(pubNXTaddr,cp->pubnxt64bits);
            printf("pubNXTaddr.(%s)\n",pubNXTaddr);
            mynp = get_NXTacct(&createdflag,Global_mp,pubNXTaddr);
            broadcast_publishpacket(Global_mp->coins,mynp,cp->NXTACCTSECRET,NXTaddr,ipaddr,port);
        } else printf("set_pNXT_privacyServer_NXTaddr cp.%p (%s) %llu\n",cp,cp!=0?cp->NXTACCTSECRET:"null",cp!=0?(long long)cp->pubnxt64bits:0);
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

uint64_t get_pNXT_privacyServer(int32_t *activeflagp)
{
    uint64_t privacyServer;
    privacyServer = calc_privacyServer(Global_pNXT->privacyServer_ipaddr,atoi(Global_pNXT->privacyServer_port));
    *activeflagp = Global_pNXT->privacyServer == privacyServer;
    //char tmp[32];
    //expand_ipbits(tmp,(uint32_t)privacyServer);
    //printf("pNXT ipaddr.(%s) %llx %s\n",Global_pNXT->privacyServer_ipaddr,(long long)privacyServer,tmp);
    return(privacyServer);
}

char *select_privacyServer(char *ipaddr,char *portstr)
{
    char buf[1024];
    uint16_t port;
    if ( portstr != 0 && portstr[0] != 0 )
        port = atoi(portstr);
    else port = NXT_PUNCH_PORT;
    sprintf(Global_pNXT->privacyServer_port,"%u",port);
    if ( ipaddr[0] != 0 )
        strcpy(Global_pNXT->privacyServer_ipaddr,ipaddr);
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
    if ( sender[0] != 0 && valid >= 0 )
    {
        retstr = select_privacyServer(ipaddr,port);
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
    uint64_t obookid,nxt64bits,assetA,assetB,txid = 0;
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
    if ( sender[0] != 0 && find_raw_orders(obookid) != 0 && valid >= 0 )
    {
        if ( price != 0. && volume != 0. && dir != 0 )
        {
            if ( dir*polarity > 0 )
                bid_orderbook_tx(&tx,0,nxt64bits,obookid,price,volume);
            else ask_orderbook_tx(&tx,0,nxt64bits,obookid,price,volume);
            printf("need to finish porting this\n");
            //len = construct_tokenized_req(tx,cmd,NXTACCTSECRET);
            //txid = call_libjl777_broadcast((uint8_t *)packet,len,PUBADDRS_MSGDURATION);
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
    if ( sender[0] != 0 && amount > 0 && valid >= 0 )
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
    if ( sender[0] != 0 && amount > 0 && valid >= 0 )
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
    if ( sender[0] != 0 && amount > 0 && valid >= 0 && dest[0] != 0 )
        retstr = send_pNXT(sender,NXTACCTSECRET,amount,level,dest,paymentid);
    else retstr = clonestr("{\"error\":\"invalid send request\"}");
    return(retstr);
}

char *teleport_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
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
    if ( cp != 0 && sender[0] != 0 && amount > 0 && valid >= 0 && destaddr[0] != 0 )
        retstr = teleport(sender,NXTACCTSECRET,(uint64_t)(SATOSHIDEN * amount),destaddr,cp,atoi(minage),M,N);
    else retstr = clonestr("{\"error\":\"invalid teleport request\"}");
    return(retstr);
}

char *sendmsg_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char NXTACCTSECRET[512],destNXTaddr[256],msg[1024],*retstr = 0;
    int32_t L;
    copy_cJSON(destNXTaddr,objs[1]);
    copy_cJSON(NXTACCTSECRET,objs[2]);
    copy_cJSON(msg,objs[3]);
    L = (int32_t)get_API_int(objs[4],1);
    //printf("sendmsg_func sender.(%s) valid.%d dest.(%s) (%s)\n",sender,valid,destNXTaddr,origargstr);
    if ( sender[0] != 0 && valid >= 0 && destNXTaddr[0] != 0 )
        retstr = sendmessage(L,sender,NXTACCTSECRET,msg,(int32_t)strlen(msg)+1,destNXTaddr,origargstr);
    else retstr = clonestr("{\"error\":\"invalid sendmessage request\"}");
    return(retstr);
}

char *checkmsg_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char NXTACCTSECRET[512],senderNXTaddr[256],*retstr = 0;
    copy_cJSON(senderNXTaddr,objs[1]);
    copy_cJSON(NXTACCTSECRET,objs[2]);
    if ( sender[0] != 0 && valid >= 0 )
        retstr = checkmessages(sender,NXTACCTSECRET,senderNXTaddr);
    else retstr = clonestr("{\"result\":\"invalid checkmessages request\"}");
    return(retstr);
}

char *getpubkey_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char NXTACCTSECRET[512],addr[256],destcoin[512],*retstr = 0;
    copy_cJSON(addr,objs[1]);
    copy_cJSON(NXTACCTSECRET,objs[2]);
    copy_cJSON(destcoin,objs[2]);
    printf("getpubkey_func(sender.%s valid.%d addr.%s)\n",sender,valid,addr);
    if ( valid < 0 )
        return(0);
    if ( sender[0] != 0 && valid >= 0 && addr[0] != 0 )
        retstr = getpubkey(sender,NXTACCTSECRET,addr,destcoin);
    else retstr = clonestr("{\"result\":\"invalid getpubkey request\"}");
    return(retstr);
}

char *sendpeerinfo_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char NXTACCTSECRET[512],addr[256],destcoin[512],*retstr = 0;
    copy_cJSON(addr,objs[1]);
    copy_cJSON(NXTACCTSECRET,objs[2]);
    copy_cJSON(destcoin,objs[2]);
    printf("sendpeerinfo_func(sender.%s valid.%d addr.%s)\n",sender,valid,addr);
    if ( valid < 0 )
        return(0);
    if ( sender[0] != 0 && valid >= 0 && addr[0] != 0 )
        retstr = sendpeerinfo(sender,NXTACCTSECRET,addr,destcoin);
    else retstr = clonestr("{\"result\":\"invalid getpubkey request\"}");
    return(retstr);
}

char *publishaddrs_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char NXTACCTSECRET[512],pubNXT[1024],pubkey[1024],BTCDaddr[1024],BTCaddr[1024],*retstr = 0;
    char srvNXTaddr[512],srvipaddr[512],srvport[512],coinstr[512];
    uint64_t coins[4];
    int32_t i,m=0,coinid,n=0;
    struct coin_info *cp;
    copy_cJSON(NXTACCTSECRET,objs[1]);
    if ( NXTACCTSECRET[0] == 0 && (cp= get_coin_info("BTCD")) != 0 )
        safecopy(NXTACCTSECRET,cp->NXTACCTSECRET,sizeof(NXTACCTSECRET));
    copy_cJSON(pubNXT,objs[2]);
    copy_cJSON(pubkey,objs[3]);
    copy_cJSON(BTCDaddr,objs[4]);
    copy_cJSON(BTCaddr,objs[5]);
    copy_cJSON(srvNXTaddr,objs[6]);
    copy_cJSON(srvipaddr,objs[7]);
    copy_cJSON(srvport,objs[8]);
    memset(coins,0,sizeof(coins));
    if ( is_cJSON_Array(objs[9]) != 0 && (n= cJSON_GetArraySize(objs[9])) > 0 )
    {
        for (i=0; i<n; i++)
        {
            copy_cJSON(coinstr,cJSON_GetArrayItem(objs[9],i));
            coinid = conv_coinstr(coinstr);
            if ( coinid >= 0 && coinid < 256 )
                coins[coinid>>6] |= (1L << (coinid & 63)), m++;
            else printf("unknown.%d coind.(%s)\n",i,coinstr);
        }
    }
    if ( sender[0] != 0 && valid >= 0 && pubNXT[0] != 0 )
        retstr = publishaddrs(m!=0?coins:0,NXTACCTSECRET,pubNXT,pubkey,BTCDaddr,BTCaddr,srvNXTaddr,srvipaddr,atoi(srvport));
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
    
    if ( sender[0] != 0 && valid >= 0 && otherNXTaddr[0] != 0 )//&& assetA != 0 && qtyA != 0. && assetB != 0. && qtyB != 0. )
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
    if ( sender[0] != 0 && valid >= 0 )
        retstr = processutx(sender,utx,sig,full);
    else retstr = clonestr("{\"result\":\"invalid makeoffer_func request\"}");
    return(retstr);
}

char *respondtx_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char signedtx[4096],*retstr = 0;
    copy_cJSON(signedtx,objs[1]);
    if ( sender[0] != 0 && valid >= 0 && signedtx[0] != 0 )
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
    if ( sender[0] != 0 && valid >= 0 && code[0] != 0 )
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
    char NXTACCTSECRET[1024],coinstr[512],coinaddr[512],receipt[1024],otherpubaddr[512],txid[512],pubkey[512],privkey[2048],privkeyhex[2048],*retstr = 0;
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
    memset(receipt,0,sizeof(receipt));
    safecopy(receipt,origargstr,sizeof(receipt)-32);
    strcpy(receipt+strlen(receipt)+1,sender);
    if ( coinstr[0] != 0 && sender[0] != 0 && valid >= 0 )
        retstr = telepod_received(sender,NXTACCTSECRET,coinstr,crc,ind,height,satoshis,coinaddr,txid,vout,pubkey,privkey,totalcrc,sharei,M,N,otherpubaddr,receipt);
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
    if ( coinstr[0] != 0 && sender[0] != 0 && valid >= 0 && n > 0 )
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
    if ( coinstr[0] != 0 && sender[0] != 0 && valid >= 0 && num > 0 )
        retstr = got_transporter_status(NXTACCTSECRET,sender,coinstr,status,totalcrc,value,num,crcs,ind,minage,height,sharei,M,N,sharenrs,otherpubaddr);
    else retstr = clonestr("{\"error\":\"invalid incoming transporter status\"}");
    if ( crcs != 0 )
        free(crcs);
    return(retstr);
}

char *maketelepods_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    uint64_t value;
    struct coin_info *cp;
    char NXTACCTSECRET[1024],coinstr[512],*retstr = 0;
    copy_cJSON(NXTACCTSECRET,objs[1]);
    if ( NXTACCTSECRET[0] == 0 && (cp= get_coin_info("BTCD")) != 0 )
        safecopy(NXTACCTSECRET,cp->NXTACCTSECRET,sizeof(NXTACCTSECRET));
    value = (SATOSHIDEN * get_API_float(objs[2]));
    copy_cJSON(coinstr,objs[3]);
    //printf("transporterstatus_func sharei.%d M.%d N.%d other.(%s)\n",sharei,M,N,otherpubaddr);
    if ( coinstr[0] != 0 && sender[0] != 0 && valid >= 0 )
        retstr = maketelepods(NXTACCTSECRET,sender,coinstr,value);
    else retstr = clonestr("{\"error\":\"invalid maketelepods_func arguments\"}");
    return(retstr);
}

char *getpeers_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    cJSON *json;
    char numstr[512],*jsonstr = 0;
    copy_cJSON(numstr,objs[2]);
    json = gen_peers_json(atoi(numstr));
    if ( json != 0 )
    {
        jsonstr = cJSON_Print(json);
        free_json(json);
    }
    return(jsonstr);
}

// add create telepod
char *pNXT_json_commands(struct NXThandler_info *mp,struct pNXT_info *gp,cJSON *argjson,char *sender,int32_t valid,char *origargstr)
{
    static char *getpeers[] = { (char *)getpeers_func, "getpeers", "V", "NXT", "secret", "only_privacyServer", 0 };
    static char *maketelepods[] = { (char *)maketelepods_func, "maketelepods", "V", "NXT", "secret", "amount", "coin", 0 };
    static char *teleport[] = { (char *)teleport_func, "teleport", "V", "NXT", "secret", "amount", "dest", "coin", "minage", "M", "N", 0 };
    static char *telepod[] = { (char *)telepod_func, "telepod", "V", "NXT", "secret", "crc", "i", "h", "c", "v", "a", "t", "o", "p", "k", "L", "s", "M", "N", "D", 0 };
    static char *transporter[] = { (char *)transporter_func, "transporter", "V", "NXT", "secret", "coin", "height", "minage", "value", "totalcrc", "telepods", "M", "N", "sharenrs", "pubaddr", 0 };
    static char *transporterstatus[] = { (char *)transporterstatus_func, "transporter_status", "V", "NXT", "secret", "status", "coin", "totalcrc", "value", "num", "minage", "height", "crcs", "sharei", "M", "N", "sharenrs", "ind", "pubaddr", 0 };
    static char *tradebot[] = { (char *)tradebot_func, "tradebot", "V", "NXT", "secret", "code", 0 };
    static char *respondtx[] = { (char *)respondtx_func, "respondtx", "V", "NXT", "signedtx", 0 };
    static char *processutx[] = { (char *)processutx_func, "processutx", "V", "NXT", "secret", "utx", "sig", "full", 0 };
    static char *publishaddrs[] = { (char *)publishaddrs_func, "publishaddrs", "V", "NXT", "secret", "pubNXT", "pubkey", "BTCD", "BTC", "srvNXTaddr", "srvipaddr", "srvport", "coins", 0 };
    static char *getpubkey[] = { (char *)getpubkey_func, "getpubkey", "V", "NXT", "addr", "secret", "destcoin", 0 };
    static char *sendpeerinfo[] = { (char *)sendpeerinfo_func, "sendpeerinfo", "V", "NXT", "addr", "secret", "destcoin", 0 };
    static char *sellp[] = { (char *)sellpNXT_func, "sellpNXT", "V", "NXT", "amount", "secret", 0 };
    static char *buyp[] = { (char *)buypNXT_func, "buypNXT", "V", "NXT", "amount", "secret", 0 };
    static char *sendmsg[] = { (char *)sendmsg_func, "sendmessage", "V", "NXT", "dest", "secret", "msg", "L", 0 };
    static char *checkmsg[] = { (char *)checkmsg_func, "checkmessages", "V", "NXT", "sender", "secret", 0 };
    static char *send[] = { (char *)sendpNXT_func, "send", "V", "NXT", "amount", "secret", "dest", "level","paymentid", 0 };
    static char *select[] = { (char *)selectserver_func, "select", "V", "NXT", "ipaddr", "port", "secret", 0 };
    static char *orderbook[] = { (char *)orderbook_func, "orderbook", "V", "NXT", "obookid", "polarity", "allfields", "secret", 0 };
    static char *getorderbooks[] = { (char *)getorderbooks_func, "getorderbooks", "V", "NXT", "secret", 0 };
    static char *placebid[] = { (char *)placebid_func, "placebid", "V", "NXT", "obookid", "polarity", "volume", "price", "assetA", "assetB", "secret", 0 };
    static char *placeask[] = { (char *)placeask_func, "placeask", "V", "NXT", "obookid", "polarity", "volume", "price", "assetA", "assetB", "secret", 0 };
    static char *makeoffer[] = { (char *)makeoffer_func, "makeoffer", "V", "NXT", "secret", "other", "assetA", "qtyA", "assetB", "qtyB", "type", 0 };
    static char **commands[] = { sendpeerinfo, getpubkey, getpeers, maketelepods, transporterstatus, telepod, transporter, tradebot, respondtx, processutx, publishaddrs, checkmsg, placebid, placeask, makeoffer, sendmsg, orderbook, getorderbooks, sellp, buyp, send, teleport, select  };
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
                if ( sender[0] == 0 || valid == 0 || (strcmp(NXTaddr,sender) != 0 && strcmp(sender,Global_pNXT->privacyServer_NXTaddr) != 0) )
                {
                    if ( strcmp(NXTaddr,sender) != 0 )
                    {
                        printf("verification valid.%d missing for %s sender.(%s) vs NXT.(%s)\n",valid,cmdinfo[1],sender,NXTaddr);
                        return(0);
                    }
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
        struct coin_info *cp = get_coin_info("BTCD");
        char _tokbuf[4096],srvNXTaddr[64],NXTaddr[64],buf[1024];//,*str;
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
        if ( strcmp(buf,"getpeers") != 0 && strcmp(buf,"maketelepods") != 0 && strcmp(buf,"teleport") != 0 && strcmp(buf,"tradebot") != 0 && strcmp(buf,"makeoffer") != 0 && strcmp(buf,"select") != 0 && strcmp(buf,"checkmessages") != 0 && cp != 0 )
        {
            parmstxt = remove_secret(argjsonp,parmstxt);
            issue_generateToken(0,encoded,parmstxt,NXTACCTSECRET);
            encoded[NXT_TOKEN_LEN] = 0;
            sprintf(_tokbuf,"[%s,{\"token\":\"%s\"}]",parmstxt,encoded);
            expand_nxt64bits(srvNXTaddr,cp->srvpubnxt64bits);
            retstr = sendmessage(Global_mp->Lfactor,NXTaddr,NXTACCTSECRET,_tokbuf,(int32_t)strlen(_tokbuf)+1,srvNXTaddr,_tokbuf);
        }
        else
            //#endif
        {
            issue_generateToken(0,encoded,origparmstxt,NXTACCTSECRET);
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

char *libjl777_JSON(char *JSONstr)
{
    cJSON *json;
    char NXTaddr[64],*cmdstr,*retstr = 0;
    struct coin_info *cp = get_coin_info("BTCD");
    printf("got JSON.(%s)\n",JSONstr);
    if ( cp != 0 && (json= cJSON_Parse(JSONstr)) != 0 )
    {
        expand_nxt64bits(NXTaddr,cp->pubnxt64bits);
        cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(NXTaddr));
        cmdstr = cJSON_Print(json);
        if ( cmdstr != 0 )
        {
            retstr = pNXT_jsonhandler(&json,cmdstr,NXTaddr);
            free(cmdstr);
        }
        free_json(json);
    }
    if ( retstr == 0 )
        retstr = clonestr("{\"result\":null}");
    return(retstr);
}

void *pNXT_handler(struct NXThandler_info *mp,struct NXT_protocol_parms *parms,void *handlerdata,int32_t height)
{
    int32_t i,createdflag;
    struct NXT_acct *np;
    uint64_t privacyServer;
    struct coin_info *cp = get_coin_info("BTCD");
    char destNXTaddr[64],servername[64],intro[4096];
    struct pNXT_info *gp = handlerdata;
    uv_connect_t **connectp;
    if ( parms->txid == 0 )     // indicates non-transaction event
    {
        if ( parms->mode == NXTPROTOCOL_WEBJSON )
            return(pNXT_jsonhandler(&parms->argjson,parms->argstr,0));
        else if ( parms->mode == NXTPROTOCOL_NEWBLOCK )
        {
            //printf("pNXT new RTblock %d time %ld microseconds %lld\n",mp->RTflag,time(0),(long long)microseconds());
            for (i=0; i<Numpeers; i++)
            {
                break;
                if ( cp != 0 && is_privacyServer(Peers[i]) != 0 && Peers[i]->udp == 0 && Peers[i]->numsent < 3 )
                {
                    expand_nxt64bits(destNXTaddr,Peers[i]->pubnxtbits);
                    np = get_NXTacct(&createdflag,mp,destNXTaddr);
                    if ( &np->mypeerinfo == Peers[i] )
                    {
                    //sprintf(jsonstr,"{\"requestType\":\"sendpeerinfo\",\"addr\":\"%s\"}",destNXTaddr);
                    //libjl777_JSON(jsonstr);
                        Peers[i]->numsent++;
                        privacyServer = (((long long)Peers[i]->srvport << 32) | Peers[i]->srvipbits);
                        connectp = (uv_connect_t **)&np->connect;
                        np->tcp = (uv_stream_t *)connect_to_privacyServer((struct sockaddr_in *)&np->addr,connectp,privacyServer);
                        
                        expand_ipbits(servername,Peers[i]->srvipbits);
                        gen_tokenjson(0,intro,destNXTaddr,time(NULL),cp->NXTACCTSECRET,servername,Peers[i]->srvport);
                        if ( intro[0] != 0 )
                        {
                            printf("send intro to %s\n",destNXTaddr);
                            portable_tcpwrite((uv_stream_t *)np->tcp,intro,strlen(intro)+1,ALLOCWR_ALLOCFREE);
                            if ( np->tcp->data != 0 )
                                portable_udpwrite(&np->Uaddr,(uv_udp_t *)np->tcp->data,intro,strlen(intro)+1,ALLOCWR_ALLOCFREE);
                        }
                     }
                }
            }
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
                /*struct NXT_str *tp = 0;
                 Global_pNXT = calloc(1,sizeof(*Global_pNXT));
                 orderbook_txids = hashtable_create("orderbook_txids",HASHTABLES_STARTSIZE,sizeof(struct NXT_str),((long)&tp->txid[0] - (long)tp),sizeof(tp->txid),((long)&tp->modified - (long)tp));
                 Global_pNXT->orderbook_txidsp = &orderbook_txids;
                 printf("SET ORDERBOOK HASHTABLE %p\n",orderbook_txids);*/
                fatal("pNXT_handler: NO GLOBALS!!!");
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

uint64_t call_libjl777_broadcast(char *msg,int32_t duration)
{
    int32_t libjl777_broadcast(char *msg,int32_t duration);
    unsigned char hash[256>>3];
    uint64_t txid;
    calc_sha256(0,hash,(uint8_t *)msg,(int32_t)strlen(msg));
    txid = calc_txid(hash,sizeof(hash));
    printf("BROADCAST.(%s) txid.%llu\n",msg,(long long)txid);
    if ( libjl777_broadcast(msg,duration) == 0 )
        return(txid);
    else return(0);
}

char *libjl777_gotpacket(char *msg,int32_t duration)
{
    static int flood,duplicates;
    cJSON *json;
    uint64_t txid;
    int32_t len,createdflag;
    unsigned char packet[4096],hash[256>>3];
    char txidstr[64];
    char retjsonstr[4096],*retstr;
    uint64_t obookid;
    printf("gotpacket\n");
    //for (i=0; i<len; i++)
    //    printf("%02x ",packet[i]);
    strcpy(retjsonstr,"{\"result\":null}");
//return(clonestr(retjsonstr));
    if ( Finished_loading == 0 )
    {
        printf("QUEUE.(%s)\n",msg);
        return(clonestr(retjsonstr));
    }
    len = (int32_t)strlen(msg);
    if ( is_hexstr(msg) != 0 )
    {
        len >>= 1;
        decode_hex(packet,len,msg);
        calc_sha256(0,hash,packet,len);
        txid = calc_txid(hash,sizeof(hash));
        sprintf(txidstr,"%llu",(long long)txid);
        MTadd_hashtable(&createdflag,&Global_pNXT->msg_txids,txidstr);
        if ( createdflag == 0 )
        {
            duplicates++;
            return(clonestr("{\"error\":\"duplicate msg\"}"));
        }
        if ( (len<<1) != 30 ) // hack against flood
            printf("C libjl777_gotpacket.%s size.%d txid.%llu | >> flood.%d\n",msg,len<<1,(long long)txid,flood);
        else flood++;
        printf("gotpacket.(%s) %d | Finished_loading.%d | flood.%d duplicates.%d\n",msg,duration,Finished_loading,flood,duplicates);
        if ( is_encrypted_packet(packet,len) != 0 )
            process_packet(retjsonstr,0,0,packet,len,0,0,0,0,0);
        else if ( (obookid= is_orderbook_tx(packet,len)) != 0 )
        {
            if ( update_orderbook_tx(1,obookid,(struct orderbook_tx *)packet,txid) == 0 )
            {
                ((struct orderbook_tx *)packet)->txid = txid;
                sprintf(retjsonstr,"{\"result\":\"libjl777_gotpacket got obbokid.%llu packet txid.%llu\"}",(long long)obookid,(long long)txid);
            } else sprintf(retjsonstr,"{\"result\":\"libjl777_gotpacket error updating obookid.%llu\"}",(long long)obookid);
        } else sprintf(retjsonstr,"{\"error\":\"libjl777_gotpacket cant find obookid\"}");
    }
    else
    {
        calc_sha256(0,hash,(uint8_t *)msg,len);
        txid = calc_txid(hash,sizeof(hash));
        sprintf(txidstr,"%llu",(long long)txid);
        MTadd_hashtable(&createdflag,&Global_pNXT->msg_txids,txidstr);
        if ( createdflag == 0 )
        {
            duplicates++;
            return(clonestr("{\"error\":\"duplicate msg\"}"));
        }
        if ( len != 30 ) // hack against flood
            printf("C libjl777_gotpacket.(%s) size.%d ascii txid.%llu | flood.%d\n",msg,len,(long long)txid,flood);
        else flood++;
        printf("gotpacket.(%s) %d | Finished_loading.%d | flood.%d duplicates.%d\n",msg,duration,Finished_loading,flood,duplicates);
        if ( (json= cJSON_Parse((char *)msg)) != 0 )
        {
            retstr = pNXT_jsonhandler(&json,(char *)msg,0);
            free_json(json);
            if ( retstr != 0 )
                return(retstr);
            else printf("pNXT_jsonhandler returns null\n");
        } printf("cJSON_Parse error.(%s)\n",msg);
    }
    return(clonestr(retjsonstr));
}

int libjl777_start(char *JSON_or_fname)
{
    char *myipaddr = 0;
    struct NXT_str *tp = 0;
    Global_mp = calloc(1,sizeof(*Global_mp));
    printf("libjl777_start(%s)\n",JSON_or_fname);
    
//return(0);
    curl_global_init(CURL_GLOBAL_ALL); //init the curl session
    if ( Global_pNXT == 0 )
    {
        Global_pNXT = calloc(1,sizeof(*Global_pNXT));
        orderbook_txids = hashtable_create("orderbook_txids",HASHTABLES_STARTSIZE,sizeof(struct NXT_str),((long)&tp->txid[0] - (long)tp),sizeof(tp->txid),((long)&tp->modified - (long)tp));
        Global_pNXT->orderbook_txidsp = &orderbook_txids;
        Global_pNXT->msg_txids = hashtable_create("msg_txids",HASHTABLES_STARTSIZE,sizeof(struct NXT_str),((long)&tp->txid[0] - (long)tp),sizeof(tp->txid),((long)&tp->modified - (long)tp));
        printf("SET ORDERBOOK HASHTABLE %p\n",orderbook_txids);
    }
    printf("call init_NXTservices\n");
    init_NXTservices(JSON_or_fname,myipaddr);
    printf("back from init_NXTservices\n");
	while ( Finished_loading == 0 )
        sleep(1);
    void *Coinloop(void *arg);
    printf("start Coinloop\n");
    if ( portable_thread_create((void *)Coinloop,Global_mp) == 0 )
        printf("ERROR Coin_genaddrloop\n");
    printf("run_UVloop\n");
    if ( portable_thread_create((void *)run_UVloop,Global_mp) == 0 )
        printf("ERROR hist process_hashtablequeues\n");
    sleep(3);
    while ( get_coin_info("BTCD") == 0 )
        sleep(1);
    init_NXTprivacy("");
    Finished_init = 1;
    return(0);
}
