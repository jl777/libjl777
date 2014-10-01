

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
uv_loop_t *UV_loop;

#include "NXTservices.c"

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
    uv_idle_start(&idler,teleport_idler);
    //uv_idle_start(&idler,NXTservices_idler);
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
    void *Coinloop(void *arg);
    struct NXThandler_info *mp = Global_mp;    // seems safest place to have main data structure
    printf("init_NXTservices.(%s)\n",myipaddr);
    UV_loop = uv_default_loop();
    portable_mutex_init(&mp->hash_mutex);
    portable_mutex_init(&mp->hashtable_queue[0].mutex);
    portable_mutex_init(&mp->hashtable_queue[1].mutex);
    
    init_NXThashtables(mp);
    init_NXTAPI(0);
    if ( myipaddr != 0 )
    {
        //strcpy(MY_IPADDR,get_ipaddr());
        strcpy(mp->ipaddr,myipaddr);
    }
    safecopy(mp->ipaddr,MY_IPADDR,sizeof(mp->ipaddr));
    mp->upollseconds = 333333 * 0;
    mp->pollseconds = POLL_SECONDS;
    crypto_box_keypair(Global_mp->loopback_pubkey,Global_mp->loopback_privkey);
    crypto_box_keypair(Global_mp->session_pubkey,Global_mp->session_privkey);
    init_hexbytes(Global_mp->pubkeystr,Global_mp->session_pubkey,sizeof(Global_mp->session_pubkey));
    if ( portable_thread_create((void *)process_hashtablequeues,mp) == 0 )
        printf("ERROR hist process_hashtablequeues\n");
    mp->udp = start_libuv_udpserver(4,SUPERNET_PORT,(void *)on_udprecv);
    init_MGWconf(JSON_or_fname,myipaddr);
    printf("start getNXTblocks.(%s)\n",myipaddr);
    if ( 0 && portable_thread_create((void *)getNXTblocks,mp) == 0 )
        printf("ERROR start_Histloop\n");
    //mp->udp = start_libuv_udpserver(4,NXT_PUNCH_PORT,(void *)on_udprecv);
    init_pingpong_queue(&PeerQ,"PeerQ",process_PeerQ,0,0);

    printf("run_NXTservices >>>>>>>>>>>>>>> %p %s: %s %s\n",mp,mp->dispname,PC_USERNAME,mp->ipaddr);
    void run_NXTservices(void *arg);
    if ( 0 && portable_thread_create((void *)run_NXTservices,mp) == 0 )
        printf("ERROR hist process_hashtablequeues\n");
    Finished_loading = 1;
	//while ( Finished_loading == 0 )
    //    sleep(1);
    printf("start Coinloop\n");
    if ( portable_thread_create((void *)Coinloop,Global_mp) == 0 )
        printf("ERROR Coin_genaddrloop\n");
    printf("run_UVloop\n");
    if ( portable_thread_create((void *)run_UVloop,Global_mp) == 0 )
        printf("ERROR hist process_hashtablequeues\n");
    sleep(3);
    while ( get_coin_info("BTCD") == 0 )
        sleep(1);
}

int64_t get_asset_quantity(int64_t *unconfirmedp,char *NXTaddr,char *assetidstr)
{
    char cmd[2*MAX_JSON_FIELD],assetid[MAX_JSON_FIELD];
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

char *orderbook_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    int32_t i,polarity,allflag;
    uint64_t obookid;
    cJSON *json,*bids,*asks,*item;
    struct orderbook *op;
    char obook[64],buf[MAX_JSON_FIELD],assetA[64],assetB[64],*retstr = 0;
    obookid = get_API_nxt64bits(objs[0]);
    expand_nxt64bits(obook,obookid);
    polarity = get_API_int(objs[1],1);
    if ( polarity == 0 )
        polarity = 1;
    allflag = get_API_int(objs[2],0);
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

char *getorderbooks_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
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

char *placequote_func(struct sockaddr *prevaddr,int32_t dir,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    int32_t polarity;
    uint64_t obookid,nxt64bits,assetA,assetB,txid = 0;
    double price,volume;
    struct orderbook_tx tx;
    char buf[MAX_JSON_FIELD],txidstr[64],*retstr = 0;
    nxt64bits = calc_nxt64bits(sender);
    obookid = get_API_nxt64bits(objs[0]);
    polarity = get_API_int(objs[1],1);
    if ( polarity == 0 )
        polarity = 1;
    volume = get_API_float(objs[2]);
    price = get_API_float(objs[3]);
    assetA = get_API_nxt64bits(objs[4]);
    assetB = get_API_nxt64bits(objs[5]);
    printf("PLACE QUOTE polarity.%d dir.%d\n",polarity,dir);
    if ( sender[0] != 0 && find_raw_orders(obookid) != 0 && valid > 0 )
    {
        if ( price != 0. && volume != 0. && dir != 0 )
        {
            if ( dir*polarity > 0 )
                bid_orderbook_tx(&tx,0,nxt64bits,obookid,price,volume);
            else ask_orderbook_tx(&tx,0,nxt64bits,obookid,price,volume);
            printf("need to finish porting this\n");
            //len = construct_tokenized_req(tx,cmd,NXTACCTSECRET);
            //txid = call_SuperNET_broadcast((uint8_t *)packet,len,PUBADDRS_MSGDURATION);
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

char *placebid_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    return(placequote_func(prevaddr,1,sender,valid,objs,numobjs,origargstr));
}

char *placeask_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    return(placequote_func(prevaddr,-1,sender,valid,objs,numobjs,origargstr));
}

char *teleport_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    double amount;
    int32_t M,N;
    struct coin_info *cp;
    char destaddr[MAX_JSON_FIELD],minage[MAX_JSON_FIELD],coinstr[MAX_JSON_FIELD],*retstr = 0;
    if ( Historical_done == 0 )
        return(clonestr("historical processing is not done yet"));
    amount = get_API_float(objs[0]);
    copy_cJSON(destaddr,objs[1]);
    copy_cJSON(coinstr,objs[2]);
    copy_cJSON(minage,objs[3]);
    M = get_API_int(objs[4],1);
    N = get_API_int(objs[5],1);
    if ( M > N || N >= 0xff || M <= 0 )
        M = N = 1;
    printf("amount.(%.8f) minage.(%s) %d | M.%d N.%d\n",amount,minage,atoi(minage),M,N);
    cp = get_coin_info(coinstr);
    if ( cp != 0 && sender[0] != 0 && amount > 0 && valid > 0 && destaddr[0] != 0 )
        retstr = teleport(sender,NXTACCTSECRET,(uint64_t)(SATOSHIDEN * amount),destaddr,cp,atoi(minage),M,N);
    else retstr = clonestr("{\"error\":\"invalid teleport request\"}");
    return(retstr);
}

char *sendmsg_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    static int counter;
    char previp[64],nexthopNXTaddr[64],destNXTaddr[64],msg[MAX_JSON_FIELD],*retstr = 0;
    int32_t L,port;
    copy_cJSON(destNXTaddr,objs[0]);
    copy_cJSON(msg,objs[1]);
    L = (int32_t)get_API_int(objs[2],Global_mp->Lfactor);
    nexthopNXTaddr[0] = 0;
    //printf("sendmsg_func sender.(%s) valid.%d dest.(%s) (%s)\n",sender,valid,destNXTaddr,origargstr);
    if ( sender[0] != 0 && valid > 0 && destNXTaddr[0] != 0 )
    {
        if ( prevaddr != 0 )
        {
            port = extract_nameport(previp,sizeof(previp),(struct sockaddr_in *)prevaddr);
            fprintf(stderr,"%d >>>>>>>>>>>>> received message.(%s) NXT.%s from hop.%s/%d\n",counter,msg,sender,previp,port);
            counter++;
            //retstr = clonestr("{\"result\":\"received message\"}");
        }
        else retstr = sendmessage(nexthopNXTaddr,L,sender,origargstr,(int32_t)strlen(origargstr)+1,destNXTaddr,origargstr);
    }
    //if ( retstr == 0 )
    //    retstr = clonestr("{\"error\":\"invalid sendmessage request\"}");
    return(retstr);
}

char *checkmsg_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char senderNXTaddr[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(senderNXTaddr,objs[0]);
    if ( sender[0] != 0 && valid > 0 )
        retstr = checkmessages(sender,NXTACCTSECRET,senderNXTaddr);
    else retstr = clonestr("{\"result\":\"invalid checkmessages request\"}");
    return(retstr);
}

char *getpubkey_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char addr[MAX_JSON_FIELD],destcoin[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(addr,objs[0]);
    copy_cJSON(NXTACCTSECRET,objs[1]);
    copy_cJSON(destcoin,objs[2]);
    printf("getpubkey_func(sender.%s valid.%d addr.%s)\n",sender,valid,addr);
    if ( valid < 0 )
        return(0);
    if ( sender[0] != 0 && valid > 0 && addr[0] != 0 )
        retstr = getpubkey(sender,NXTACCTSECRET,addr,destcoin);
    else retstr = clonestr("{\"result\":\"invalid getpubkey request\"}");
    return(retstr);
}

char *sendpeerinfo_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    int32_t pserver_flag;
    char hopNXTaddr[64],addr[MAX_JSON_FIELD],destcoin[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(addr,objs[0]);
    copy_cJSON(destcoin,objs[1]);
    pserver_flag = (int32_t)get_API_int(objs[2],0);
    printf("sendpeerinfo_func(sender.%s valid.%d addr.%s) pserver_flag.%d\n",sender,valid,addr,pserver_flag);
    if ( valid < 0 )
        return(0);
    if ( sender[0] != 0 && valid > 0 && addr[0] != 0 )
        retstr = sendpeerinfo(pserver_flag,hopNXTaddr,sender,NXTACCTSECRET,addr,destcoin);
    else retstr = clonestr("{\"result\":\"invalid getpubkey request\"}");
    return(retstr);
}

char *publishaddrs_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char pubNXT[MAX_JSON_FIELD],pubkey[MAX_JSON_FIELD],BTCDaddr[MAX_JSON_FIELD],BTCaddr[MAX_JSON_FIELD],*retstr = 0;
    char srvNXTaddr[MAX_JSON_FIELD],srvipaddr[MAX_JSON_FIELD],srvport[MAX_JSON_FIELD],coinstr[MAX_JSON_FIELD],haspservers[MAX_JSON_FIELD],xorsumstr[MAX_JSON_FIELD];
    uint64_t coins[4];
    int32_t i,m=0,coinid,n=0;
    copy_cJSON(pubNXT,objs[0]);
    copy_cJSON(pubkey,objs[1]);
    copy_cJSON(BTCDaddr,objs[2]);
    copy_cJSON(BTCaddr,objs[3]);
    copy_cJSON(srvNXTaddr,objs[4]);
    copy_cJSON(srvipaddr,objs[5]);
    copy_cJSON(srvport,objs[6]);
    memset(coins,0,sizeof(coins));
    if ( is_cJSON_Array(objs[7]) != 0 && (n= cJSON_GetArraySize(objs[7])) > 0 )
    {
        for (i=0; i<n; i++)
        {
            copy_cJSON(coinstr,cJSON_GetArrayItem(objs[7],i));
            coinid = conv_coinstr(coinstr);
            if ( coinid >= 0 && coinid < 256 )
                coins[coinid>>6] |= (1L << (coinid & 63)), m++;
            else printf("unknown.%d coind.(%s)\n",i,coinstr);
        }
    }
    copy_cJSON(haspservers,objs[8]);
    copy_cJSON(xorsumstr,objs[9]);
    if ( sender[0] != 0 && valid > 0 && pubNXT[0] != 0 )
        retstr = publishaddrs(prevaddr,m!=0?coins:0,NXTACCTSECRET,pubNXT,pubkey,BTCDaddr,BTCaddr,srvNXTaddr,srvipaddr,atoi(srvport),atoi(haspservers),(uint32_t)atol(xorsumstr));
    else retstr = clonestr("{\"result\":\"invalid publishaddrs request\"}");
    return(retstr);
}

char *makeoffer_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    uint64_t assetA,assetB;
    double qtyA,qtyB;
    int32_t type;
    char otherNXTaddr[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(otherNXTaddr,objs[0]);
    assetA = get_API_nxt64bits(objs[1]);
    qtyA = get_API_float(objs[2]);
    assetB = get_API_nxt64bits(objs[3]);
    qtyB = get_API_float(objs[4]);
    type = get_API_int(objs[5],0);
    
    if ( sender[0] != 0 && valid > 0 && otherNXTaddr[0] != 0 )//&& assetA != 0 && qtyA != 0. && assetB != 0. && qtyB != 0. )
        retstr = makeoffer(sender,NXTACCTSECRET,otherNXTaddr,assetA,qtyA,assetB,qtyB,type);
    else retstr = clonestr("{\"result\":\"invalid makeoffer_func request\"}");
    return(retstr);
}

char *processutx_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char utx[MAX_JSON_FIELD],full[MAX_JSON_FIELD],sig[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(utx,objs[0]);
    copy_cJSON(sig,objs[1]);
    copy_cJSON(full,objs[2]);
    if ( sender[0] != 0 && valid > 0 )
        retstr = processutx(sender,utx,sig,full);
    else retstr = clonestr("{\"result\":\"invalid makeoffer_func request\"}");
    return(retstr);
}

char *respondtx_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char signedtx[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(signedtx,objs[0]);
    if ( sender[0] != 0 && valid > 0 && signedtx[0] != 0 )
        retstr = respondtx(sender,signedtx);
    else retstr = clonestr("{\"result\":\"invalid makeoffer_func request\"}");
    return(retstr);
}

char *tradebot_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    static char *buf;
    static int64_t filelen,allocsize;
    long len;
    cJSON *botjson;
    char code[MAX_JSON_FIELD],retbuf[4096],*str,*retstr = 0;
    copy_cJSON(code,objs[0]);
    printf("tradebotfunc.(%s) sender.(%s) valid.%d code.(%s)\n",origargstr,sender,valid,code);
    if ( sender[0] != 0 && valid > 0 && code[0] != 0 )
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

char *telepod_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    uint64_t satoshis;
    uint32_t crc,ind,height,vout,totalcrc,sharei,M,N;
    char coinstr[MAX_JSON_FIELD],coinaddr[MAX_JSON_FIELD],receipt[MAX_JSON_FIELD],otherpubaddr[MAX_JSON_FIELD],txid[MAX_JSON_FIELD],pubkey[MAX_JSON_FIELD],privkey[MAX_JSON_FIELD],privkeyhex[MAX_JSON_FIELD],*retstr = 0;
    crc = get_API_uint(objs[0],0);
    ind = get_API_uint(objs[1],0);
    height = get_API_uint(objs[2],0);
    copy_cJSON(coinstr,objs[3]);
    satoshis = SATOSHIDEN * get_API_float(objs[4]);
    copy_cJSON(coinaddr,objs[5]);
    copy_cJSON(txid,objs[6]);
    vout = get_API_uint(objs[7],0);
    copy_cJSON(pubkey,objs[8]);
    copy_cJSON(privkeyhex,objs[9]);
    decode_hex((unsigned char *)privkey,(int32_t)strlen(privkeyhex)/2,privkeyhex);
    privkey[strlen(privkeyhex)/2] = 0;
    totalcrc = get_API_uint(objs[10],0);
    sharei = get_API_uint(objs[11],0);
    M = get_API_uint(objs[12],1);
    N = get_API_uint(objs[13],1);
    copy_cJSON(otherpubaddr,objs[14]);
    memset(receipt,0,sizeof(receipt));
    safecopy(receipt,origargstr,sizeof(receipt)-32);
    strcpy(receipt+strlen(receipt)+1,sender);
    if ( coinstr[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = telepod_received(sender,NXTACCTSECRET,coinstr,crc,ind,height,satoshis,coinaddr,txid,vout,pubkey,privkey,totalcrc,sharei,M,N,otherpubaddr,receipt);
    else retstr = clonestr("{\"error\":\"invalid telepod received\"}");
    return(retstr);
}

char *transporter_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    uint64_t value;
    uint8_t sharenrs[MAX_JSON_FIELD];
    uint32_t totalcrc,M,N,minage,height,i,n=0,*crcs = 0;
    char sharenrsbuf[MAX_JSON_FIELD],coinstr[MAX_JSON_FIELD],otherpubaddr[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(coinstr,objs[0]);
    height = get_API_uint(objs[1],0);
    minage = get_API_uint(objs[2],0);
    value = (SATOSHIDEN * get_API_float(objs[3]));
    totalcrc = get_API_uint(objs[4],0);
    if ( is_cJSON_Array(objs[5]) != 0 && (n= cJSON_GetArraySize(objs[5])) > 0 )
    {
        crcs = calloc(n,sizeof(*crcs));
        for (i=0; i<n; i++)
            crcs[i] = get_API_uint(cJSON_GetArrayItem(objs[5],i),0);
    }
    M = get_API_int(objs[6],0);
    N = get_API_int(objs[7],0);
    copy_cJSON(sharenrsbuf,objs[8]);
    memset(sharenrs,0,sizeof(sharenrs));
    if ( M <= N && N < 0xff && M > 0 )
        decode_hex(sharenrs,N,sharenrsbuf);
    else M = N = 1;
    copy_cJSON(otherpubaddr,objs[9]);
    printf("transporterstatus_func M.%d N.%d [%s] otherpubaddr.(%s)\n",M,N,sharenrsbuf,otherpubaddr);
    if ( coinstr[0] != 0 && sender[0] != 0 && valid > 0 && n > 0 )
        retstr = transporter_received(sender,NXTACCTSECRET,coinstr,totalcrc,height,value,minage,crcs,n,M,N,sharenrs,otherpubaddr);
    else retstr = clonestr("{\"error\":\"invalid incoming transporter bundle\"}");
    if ( crcs != 0 )
        free(crcs);
    return(retstr);
}

char *transporterstatus_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    uint64_t value;
    cJSON *array;
    int32_t minage;
    uint8_t sharenrs[MAX_JSON_FIELD];
    uint32_t totalcrc,status,i,sharei,M,N,height,ind,num,n=0,*crcs = 0;
    char sharenrsbuf[MAX_JSON_FIELD],otherpubaddr[MAX_JSON_FIELD],coinstr[MAX_JSON_FIELD],*retstr = 0;
    status = get_API_int(objs[0],0);
    copy_cJSON(coinstr,objs[1]);
    totalcrc = get_API_uint(objs[2],0);
    value = (SATOSHIDEN * get_API_float(objs[3]));
    num = get_API_int(objs[4],0);
    minage = get_API_int(objs[5],0);
    height = get_API_int(objs[6],0);
    array = objs[7];
    if ( is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
    {
        crcs = calloc(n,sizeof(*crcs));
        for (i=0; i<n; i++)
            crcs[i] = get_API_uint(cJSON_GetArrayItem(array,i),0);
    }
    sharei = get_API_int(objs[8],0);
    M = get_API_int(objs[9],0);
    N = get_API_int(objs[10],0);
    copy_cJSON(sharenrsbuf,objs[11]);
    memset(sharenrs,0,sizeof(sharenrs));
    if ( M <= N && N < 0xff && M > 0 )
        decode_hex(sharenrs,N,sharenrsbuf);
    else M = N = 1;
    ind = get_API_int(objs[12],0);
    copy_cJSON(otherpubaddr,objs[13]);
    //printf("transporterstatus_func sharei.%d M.%d N.%d other.(%s)\n",sharei,M,N,otherpubaddr);
    if ( coinstr[0] != 0 && sender[0] != 0 && valid > 0 && num > 0 )
        retstr = got_transporter_status(NXTACCTSECRET,sender,coinstr,status,totalcrc,value,num,crcs,ind,minage,height,sharei,M,N,sharenrs,otherpubaddr);
    else retstr = clonestr("{\"error\":\"invalid incoming transporter status\"}");
    if ( crcs != 0 )
        free(crcs);
    return(retstr);
}

char *maketelepods_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    uint64_t value;
    char coinstr[MAX_JSON_FIELD],*retstr = 0;
    value = (SATOSHIDEN * get_API_float(objs[0]));
    copy_cJSON(coinstr,objs[1]);
    //printf("transporterstatus_func sharei.%d M.%d N.%d other.(%s)\n",sharei,M,N,otherpubaddr);
    if ( coinstr[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = maketelepods(NXTACCTSECRET,sender,coinstr,value);
    else retstr = clonestr("{\"error\":\"invalid maketelepods_func arguments\"}");
    return(retstr);
}

char *getpeers_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    cJSON *json;
    char numstr[MAX_JSON_FIELD],*jsonstr = 0;
    copy_cJSON(numstr,objs[0]);
    json = gen_peers_json(atoi(numstr));
    if ( json != 0 )
    {
        jsonstr = cJSON_Print(json);
        free_json(json);
    }
    return(jsonstr);
}

char *getPservers_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    cJSON *json;
    char numstr[MAX_JSON_FIELD],*jsonstr = 0;
    copy_cJSON(numstr,objs[0]);
    json = gen_Pservers_json(atoi(numstr));
    if ( json != 0 )
    {
        jsonstr = cJSON_Print(json);
        stripwhite_ns(jsonstr,strlen(jsonstr));
        free_json(json);
    }
    return(jsonstr);
}

char *publishPservers_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    int32_t num,firsti,i,n = 0;
    uint32_t xorsum,*pservers = 0;
    char ipstr[MAX_JSON_FIELD],*retstr = 0;
    cJSON *array,*item;
    num = get_API_int(objs[1],0);
    firsti = get_API_int(objs[2],0);
    xorsum = get_API_int(objs[3],0);
    //printf("transporterstatus_func sharei.%d M.%d N.%d other.(%s)\n",sharei,M,N,otherpubaddr);
    array = objs[0];
    if ( is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
    {
        pservers = calloc(n,sizeof(*pservers));
        for (i=0; i<n; i++)
        {
            item = cJSON_GetArrayItem(array,i);
            copy_cJSON(ipstr,item);
            if ( ipstr[0] != 0 )
                pservers[i] = calc_ipbits(ipstr);
        }
    }
    if ( sender[0] != 0 && valid > 0 && pservers != 0 )
        retstr = publishPservers(prevaddr,NXTACCTSECRET,sender,firsti,num,pservers,n,xorsum);
    else retstr = clonestr("{\"error\":\"invalid maketelepods_func arguments\"}");
    if ( pservers != 0 )
        free(pservers);
    return(retstr);
}

char *sendfile_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    FILE *fp;
    int32_t L;
    char fname[MAX_JSON_FIELD],dest[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(fname,objs[0]);
    copy_cJSON(dest,objs[1]);
    L = get_API_int(objs[2],0);
    fp = fopen(fname,"rb");
    if ( fp != 0 && sender[0] != 0 && valid > 0 )
        retstr = onion_sendfile(L,prevaddr,NXTaddr,NXTACCTSECRET,sender,dest,fp);
    else retstr = clonestr("{\"error\":\"invalid sendfile_func arguments\"}");
    if ( fp != 0 )
        fclose(fp);
    return(retstr);
}

char *pNXT_json_commands(struct NXThandler_info *mp,struct sockaddr *prevaddr,cJSON *origargjson,char *sender,int32_t valid,char *origargstr)
{

    static char *getpeers[] = { (char *)getpeers_func, "getpeers", "V",  "only_privacyServer", 0 };
    static char *getPservers[] = { (char *)getPservers_func, "getPservers", "V",  "firsti", 0 };
    static char *publishPservers[] = { (char *)publishPservers_func, "publishPservers", "V", "Pservers", "Numpservers", "firstPserver", "xorsum", 0 };
    static char *maketelepods[] = { (char *)maketelepods_func, "maketelepods", "V", "amount", "coin", 0 };
    static char *teleport[] = { (char *)teleport_func, "teleport", "V", "amount", "dest", "coin", "minage", "M", "N", 0 };
    static char *telepod[] = { (char *)telepod_func, "telepod", "V", "crc", "i", "h", "c", "v", "a", "t", "o", "p", "k", "L", "s", "M", "N", "D", 0 };
    static char *transporter[] = { (char *)transporter_func, "transporter", "V", "coin", "height", "minage", "value", "totalcrc", "telepods", "M", "N", "sharenrs", "pubaddr", 0 };
    static char *transporterstatus[] = { (char *)transporterstatus_func, "transporter_status", "V", "status", "coin", "totalcrc", "value", "num", "minage", "height", "crcs", "sharei", "M", "N", "sharenrs", "ind", "pubaddr", 0 };
    static char *tradebot[] = { (char *)tradebot_func, "tradebot", "V", "code", 0 };
    static char *respondtx[] = { (char *)respondtx_func, "respondtx", "V", "signedtx", 0 };
    static char *processutx[] = { (char *)processutx_func, "processutx", "V", "utx", "sig", "full", 0 };
    static char *publishaddrs[] = { (char *)publishaddrs_func, "publishaddrs", "V", "pubNXT", "pubkey", "BTCD", "BTC", "srvNXTaddr", "srvipaddr", "srvport", "coins", "Numpservers", "xorsum", 0 };
    static char *getpubkey[] = { (char *)getpubkey_func, "getpubkey", "V", "addr", "destcoin", 0 };
    static char *sendpeerinfo[] = { (char *)sendpeerinfo_func, "sendpeerinfo", "V", "addr", "destcoin", "pserver_flag", 0 };
    static char *sendmsg[] = { (char *)sendmsg_func, "sendmessage", "V", "dest", "msg", "L", 0 };
    static char *checkmsg[] = { (char *)checkmsg_func, "checkmessages", "V", "sender", 0 };
    static char *orderbook[] = { (char *)orderbook_func, "orderbook", "V", "obookid", "polarity", "allfields", 0 };
    static char *getorderbooks[] = { (char *)getorderbooks_func, "getorderbooks", "V", 0 };
    static char *placebid[] = { (char *)placebid_func, "placebid", "V", "obookid", "polarity", "volume", "price", "assetA", "assetB", 0 };
    static char *placeask[] = { (char *)placeask_func, "placeask", "V", "obookid", "polarity", "volume", "price", "assetA", "assetB", 0 };
    static char *makeoffer[] = { (char *)makeoffer_func, "makeoffer", "V", "other", "assetA", "qtyA", "assetB", "qtyB", "type", 0 };
    static char *sendfile[] = { (char *)sendfile_func, "sendfile", "V", "filename", "dest", "L", 0 };
    static char **commands[] = { sendfile, publishPservers, sendpeerinfo, getPservers, getpubkey, getpeers, maketelepods, transporterstatus, telepod, transporter, tradebot, respondtx, processutx, publishaddrs, checkmsg, placebid, placeask, makeoffer, sendmsg, orderbook, getorderbooks, teleport  };
    int32_t i,j;
    struct coin_info *cp;
    cJSON *argjson,*obj,*nxtobj,*secretobj,*objs[64];
    char NXTaddr[MAX_JSON_FIELD],NXTACCTSECRET[MAX_JSON_FIELD],command[MAX_JSON_FIELD],**cmdinfo,*retstr;
    memset(objs,0,sizeof(objs));
    command[0] = 0;
    memset(NXTaddr,0,sizeof(NXTaddr));
    if ( is_cJSON_Array(origargjson) != 0 )
        argjson = cJSON_GetArrayItem(origargjson,0);
    else argjson = origargjson;
    NXTACCTSECRET[0] = 0;
    if ( argjson != 0 )
    {
        obj = cJSON_GetObjectItem(argjson,"requestType");
        nxtobj = cJSON_GetObjectItem(argjson,"NXT");
        secretobj = cJSON_GetObjectItem(argjson,"secret");
        copy_cJSON(NXTaddr,nxtobj);
        copy_cJSON(command,obj);
        copy_cJSON(NXTACCTSECRET,secretobj);
        if ( NXTACCTSECRET[0] == 0 && (cp= get_coin_info("BTCD")) != 0 )
        {
            if ( strcmp("127.0.0.1",cp->privacyserver) == 0 )
            {
                safecopy(NXTACCTSECRET,cp->srvNXTACCTSECRET,sizeof(NXTACCTSECRET));
                expand_nxt64bits(NXTaddr,cp->srvpubnxtbits);
                //printf("use localserver NXT.%s to send command\n",NXTaddr);
            }
            else
            {
                safecopy(NXTACCTSECRET,cp->NXTACCTSECRET,sizeof(NXTACCTSECRET));
                expand_nxt64bits(NXTaddr,cp->pubnxtbits);
                //printf("use NXT.%s to send command\n",NXTaddr);
            }
        }
        //printf("(%s) command.(%s) NXT.(%s)\n",cJSON_Print(argjson),command,NXTaddr);
    }
    //printf("pNXT_json_commands sender.(%s) valid.%d | size.%d | command.(%s) orig.(%s)\n",sender,valid,(int32_t)(sizeof(commands)/sizeof(*commands)),command,origargstr);
    for (i=0; i<(int32_t)(sizeof(commands)/sizeof(*commands)); i++)
    {
        cmdinfo = commands[i];
        //printf("needvalid.(%c) sender.(%s) valid.%d %d of %d: cmd.(%s) vs command.(%s)\n",cmdinfo[2][0],sender,valid,i,(int32_t)(sizeof(commands)/sizeof(*commands)),cmdinfo[1],command);
        if ( strcmp(cmdinfo[1],command) == 0 )
        {
            if ( cmdinfo[2][0] != 0 && valid <= 0 )
                return(0);
            for (j=3; cmdinfo[j]!=0&&j<3+(int32_t)(sizeof(objs)/sizeof(*objs)); j++)
                objs[j-3] = cJSON_GetObjectItem(argjson,cmdinfo[j]);
            retstr = (*(json_handler)cmdinfo[0])(NXTaddr,NXTACCTSECRET,prevaddr,sender,valid,objs,j-3,origargstr);
            //if ( retstr == 0 )
            //    retstr = clonestr("{\"result\":null}");
            //if ( 0 && retstr != 0 )
            //    printf("json_handler returns.(%s)\n",retstr);
            return(retstr);
        }
    }
    return(0);
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
        printf("process_pNXT_AM got jsontxt.(%s)\n",ap->U.jsonstr);
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

char *SuperNET_JSON(char *JSONstr)
{
    cJSON *json,*array;
    int32_t valid;
    char NXTaddr[64],_tokbuf[2*MAX_JSON_FIELD],encoded[NXT_TOKEN_LEN+1],*cmdstr,*retstr = 0;
    struct coin_info *cp = get_coin_info("BTCD");
    if ( Finished_init == 0 )
        return(0);
    //printf("got JSON.(%s)\n",JSONstr);
    if ( cp != 0 && (json= cJSON_Parse(JSONstr)) != 0 )
    {
        expand_nxt64bits(NXTaddr,cp->pubnxtbits);
        cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(NXTaddr));
        cmdstr = cJSON_Print(json);
        if ( cmdstr != 0 )
        {
            stripwhite_ns(cmdstr,strlen(cmdstr));
            issue_generateToken(0,encoded,cmdstr,cp->NXTACCTSECRET);
            encoded[NXT_TOKEN_LEN] = 0;
            sprintf(_tokbuf,"[%s,{\"token\":\"%s\"}]",cmdstr,encoded);
            free(cmdstr);
            array = cJSON_Parse(_tokbuf);
            if ( array != 0 )
            {
                cmdstr = verify_tokenized_json(NXTaddr,&valid,array);
                retstr = pNXT_json_commands(Global_mp,0,array,NXTaddr,valid,_tokbuf);
                if ( cmdstr != 0 )
                {
                    //printf("parms.(%s) valid.%d\n",cmdstr,valid);
                    free(cmdstr);
                }
                free_json(array);
            }
        }
        free_json(json);
    }
    if ( retstr == 0 )
        retstr = clonestr("{\"result\":null}");
    return(retstr);
}

void *pNXT_handler(struct NXThandler_info *mp,struct NXT_protocol_parms *parms,void *handlerdata,int32_t height)
{
    struct pNXT_info *gp = handlerdata;
    if ( parms->txid == 0 )     // indicates non-transaction event
    {
        /*if ( parms->mode == NXTPROTOCOL_WEBJSON )
        {
            printf(":7777 interface deprecated\n");
            //return(pNXT_jsonhandler(&parms->argjson,parms->argstr,0));
        }
        else*/ if ( parms->mode == NXTPROTOCOL_NEWBLOCK )
        {
            //printf("pNXT new RTblock %d time %ld microseconds %lld\n",mp->RTflag,time(0),(long long)microseconds());
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

uint64_t call_SuperNET_broadcast(char *destip,char *msg,int32_t len,int32_t duration)
{
    int32_t SuperNET_broadcast(char *msg,int32_t duration);
    int32_t SuperNET_narrowcast(char *destip,unsigned char *msg,int32_t len);
    unsigned char hash[256>>3];
    char ip_port[64];
    uint64_t txid;
    int i;
    calc_sha256(0,hash,(uint8_t *)msg,(int32_t)strlen(msg));
    txid = calc_txid(hash,sizeof(hash));
    if ( destip != 0 )
    {
        strcpy(ip_port,destip);
        for (i=0; destip[i]!=0; i++)
            if ( destip[i] == ':' )
                break;
        if ( destip[i] != ':' )
            strcat(ip_port,":14631");
        txid ^= calc_ipbits(destip);
        printf("%s NARROWCAST.(%s) txid.%llu destbits.%x\n",destip,msg,(long long)txid,calc_ipbits(destip));
        if ( SuperNET_narrowcast(destip,(unsigned char *)msg,len) == 0 )
            return(txid);
    }
    else
    {
        char *cmdstr,NXTaddr[64];
        cJSON *array;
        int32_t valid;
        array = cJSON_Parse(msg);
        if ( array != 0 )
        {
            cmdstr = verify_tokenized_json(NXTaddr,&valid,array);
            if ( cmdstr != 0 )
                free(cmdstr);
            free_json(array);
            printf("BROADCAST parms.(%s) valid.%d txid.%llu\n",msg,valid,(long long)txid);
            if ( SuperNET_broadcast(msg,duration) == 0 )
                return(txid);
        } else printf("cant broadcast non-JSON.(%s)\n",msg);
    }
    return(0);
}

int32_t got_newpeer(char *ip_port)
{
    queue_enqueue(&P2P_Q,clonestr(ip_port));
	return(0);
}

char *SuperNET_gotpacket(char *msg,int32_t duration,char *ip_port)
{
    static int flood,duplicates;
    cJSON *json;
    uint64_t txid;
    struct sockaddr prevaddr;
    int32_t len,createdflag;
    unsigned char packet[2*MAX_JSON_FIELD],hash[256>>3];
    char txidstr[64];
    char retjsonstr[2*MAX_JSON_FIELD],*retstr;
    uint64_t obookid;
    //printf("gotpacket\n");
    //for (i=0; i<len; i++)
    //    printf("%02x ",packet[i]);
    if ( Finished_init == 0 )
        return(0);
    strcpy(retjsonstr,"{\"result\":null}");
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
        if ( (len<<1) == 30 ) // hack against flood
            flood++;
        printf("gotpacket.(%s) %d | Finished_loading.%d | flood.%d duplicates.%d\n",msg,duration,Finished_loading,flood,duplicates);
        if ( is_encrypted_packet(packet,len) != 0 )
            process_packet(retjsonstr,packet,len,0,0,0,0);
        else if ( (obookid= is_orderbook_tx(packet,len)) != 0 )
        {
            if ( update_orderbook_tx(1,obookid,(struct orderbook_tx *)packet,txid) == 0 )
            {
                ((struct orderbook_tx *)packet)->txid = txid;
                sprintf(retjsonstr,"{\"result\":\"SuperNET_gotpacket got obbokid.%llu packet txid.%llu\"}",(long long)obookid,(long long)txid);
            } else sprintf(retjsonstr,"{\"result\":\"SuperNET_gotpacket error updating obookid.%llu\"}",(long long)obookid);
        } else sprintf(retjsonstr,"{\"error\":\"SuperNET_gotpacket cant find obookid\"}");
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
        if ( len == 30 ) // hack against flood
            flood++;
        printf("C SuperNET_gotpacket.(%s) size.%d ascii txid.%llu | flood.%d\n",msg,len,(long long)txid,flood);
        if ( (json= cJSON_Parse((char *)msg)) != 0 )
        {
            int32_t valid,port;
            char ipaddr[64],verifiedNXTaddr[64],*cmdstr;
            cmdstr = verify_tokenized_json(verifiedNXTaddr,&valid,json);
            port = parse_ipaddr(ipaddr,ip_port);
            uv_ip4_addr(ipaddr,port,(struct sockaddr_in *)&prevaddr);
            retstr = pNXT_json_commands(Global_mp,&prevaddr,json,verifiedNXTaddr,valid,(char *)msg);
            if ( cmdstr != 0 )
            {
                //printf("got parms.(%s) valid.%d\n",(char *)msg,valid);
                free(cmdstr);
            }
            free_json(json);
            if ( retstr == 0 )
                retstr = clonestr("{\"result\":null}");
                return(retstr);
        } printf("cJSON_Parse error.(%s)\n",msg);
    }
    return(clonestr(retjsonstr));
}

int SuperNET_start(char *JSON_or_fname,char *myipaddr)
{
    FILE *fp = 0;
    struct NXT_str *tp = 0;
    if ( JSON_or_fname != 0 && JSON_or_fname[0] != '{' )
        fp = fopen(JSON_or_fname,"rb");
    printf("SuperNET_start(%s) %p ipaddr.(%s) fp.%p\n",JSON_or_fname,myipaddr,myipaddr,fp);
    if ( fp == 0 )
        return(-1);
    fclose(fp);
    myipaddr = clonestr(myipaddr);
    Global_mp = calloc(1,sizeof(*Global_mp));
    curl_global_init(CURL_GLOBAL_ALL); //init the curl session
    if ( Global_pNXT == 0 )
    {
        Global_pNXT = calloc(1,sizeof(*Global_pNXT));
        orderbook_txids = hashtable_create("orderbook_txids",HASHTABLES_STARTSIZE,sizeof(struct NXT_str),((long)&tp->U.txid[0] - (long)tp),sizeof(tp->U.txid),((long)&tp->modified - (long)tp));
        Global_pNXT->orderbook_txidsp = &orderbook_txids;
        Global_pNXT->msg_txids = hashtable_create("msg_txids",HASHTABLES_STARTSIZE,sizeof(struct NXT_str),((long)&tp->U.txid[0] - (long)tp),sizeof(tp->U.txid),((long)&tp->modified - (long)tp));
        printf("SET ORDERBOOK HASHTABLE %p\n",orderbook_txids);
    }
    printf("call init_NXTservices.(%s)\n",myipaddr);
    init_NXTservices(JSON_or_fname,myipaddr);
    printf("back from init_NXTservices\n");
    Finished_init = 1;
    free(myipaddr);
    broadcast_publishpacket(0);

    return(0);
}
