

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

//char dispstr[65536];
//char testforms[1024*1024],PC_USERNAME[512],MY_IPADDR[512];
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

uv_async_t Tasks_async;
uv_work_t Tasks;
struct task_info
{
    char name[64];
    int32_t sleepmicros,argsize;
    tfunc func;
    uint8_t args[];
};
tfunc Tfuncs[100];

void aftertask(uv_work_t *req,int status)
{
    struct task_info *task = (struct task_info *)req->data;
    if ( task != 0 )
    {
        fprintf(stderr,"req.%p task.%s complete status.%d\n",req,task->name,status);
        free(task);
    } else fprintf(stderr,"task.%p complete\n",req);
    uv_close((uv_handle_t *)&Tasks_async,NULL);
}

void Task_handler(uv_work_t *req)
{
    struct task_info *task = (struct task_info *)req->data;
    while ( task != 0 )
    {
        //fprintf(stderr,"Task.(%s) sleep.%d\n",task->name,task->sleepmicros);
        if ( task->func != 0 )
        {
            if ( (*task->func)(task->args,task->argsize) < 0 )
                break;
            if ( task->sleepmicros != 0 )
                usleep(task->sleepmicros);
        }
        else break;
    }
}

uv_work_t *start_task(tfunc func,char *name,int32_t sleepmicros,void *args,int32_t argsize)
{
    uv_work_t *work;
    struct task_info *task;
    task = calloc(1,sizeof(*task) + argsize);
    memcpy(task->args,args,argsize);
    task->func = func;
    task->argsize = argsize;
    safecopy(task->name,name,sizeof(task->name));
    task->sleepmicros = sleepmicros;
    work = calloc(1,sizeof(*work));
    work->data = task;
    uv_queue_work(UV_loop,work,Task_handler,aftertask);
    return(work);
}

/*
void async_handler(uv_async_t *handle)
{
    char *jsonstr = (char *)handle->data;
    if ( jsonstr != 0 )
    {
        fprintf(stderr,"ASYNC(%s)\n",jsonstr);
        free(jsonstr);
        handle->data = 0;
    } else fprintf(stderr,"ASYNC called\n");
}

void send_async_message(char *msg)
{
    while ( Tasks_async.data != 0 )
        usleep(1000);
    Tasks_async.data = clonestr(msg);
    uv_async_send(&Tasks_async);
}*/

void run_UVloop(void *arg)
{
    uv_idle_t idler;
    uv_idle_init(UV_loop,&idler);
    //uv_async_init(UV_loop,&Tasks_async,async_handler);
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

void init_NXThashtables(struct NXThandler_info *mp)
{
    struct other_addr *op = 0;
    struct NXT_acct *np = 0;
    struct NXT_asset *ap = 0;
    struct NXT_assettxid *tp = 0;
    struct NXT_guid *gp = 0;
    struct pserver_info *pp = 0;
    struct telepathy_entry *tel = 0;
    static struct hashtable *NXTasset_txids,*NXTaddrs,*NXTassets,*NXTguids,*otheraddrs,*Pserver,*Telepathy_hash;
    if ( Pserver == 0 )
        Pserver = hashtable_create("Pservers",HASHTABLES_STARTSIZE,sizeof(struct pserver_info),((long)&pp->ipaddr[0] - (long)pp),sizeof(pp->ipaddr),((long)&pp->modified - (long)pp));
    if ( NXTguids == 0 )
        NXTguids = hashtable_create("NXTguids",HASHTABLES_STARTSIZE,sizeof(struct NXT_guid),((long)&gp->guid[0] - (long)gp),sizeof(gp->guid),((long)&gp->H.modified - (long)gp));
    if ( NXTasset_txids == 0 )
        NXTasset_txids = hashtable_create("NXTasset_txids",HASHTABLES_STARTSIZE,sizeof(struct NXT_assettxid),((long)&tp->H.U.txid[0] - (long)tp),sizeof(tp->H.U.txid),((long)&tp->H.modified - (long)tp));
    if ( NXTassets == 0 )
        NXTassets = hashtable_create("NXTassets",HASHTABLES_STARTSIZE,sizeof(struct NXT_asset),((long)&ap->H.U.assetid[0] - (long)ap),sizeof(ap->H.U.assetid),((long)&ap->H.modified - (long)ap));
    if ( NXTaddrs == 0 )
        NXTaddrs = hashtable_create("NXTaddrs",HASHTABLES_STARTSIZE,sizeof(struct NXT_acct),((long)&np->H.U.NXTaddr[0] - (long)np),sizeof(np->H.U.NXTaddr),((long)&np->H.modified - (long)np));
    if ( otheraddrs == 0 )
        otheraddrs = hashtable_create("otheraddrs",HASHTABLES_STARTSIZE,sizeof(struct other_addr),((long)&op->addr[0] - (long)op),sizeof(op->addr),((long)&op->modified - (long)op));
    if ( Telepathy_hash == 0 )
        Telepathy_hash = hashtable_create("Telepath_hash",HASHTABLES_STARTSIZE,sizeof(struct telepathy_entry),((long)&tel->locationstr[0] - (long)tel),sizeof(tel->locationstr),((long)&tel->modified - (long)tel));
    if ( mp != 0 )
    {
        mp->Telepathy_tablep = &Telepathy_hash;
        mp->Pservers_tablep = &Pserver;
        mp->NXTguid_tablep = &NXTguids;
        mp->NXTaccts_tablep = &NXTaddrs;
        mp->otheraddrs_tablep = &otheraddrs;
        mp->NXTassets_tablep = &NXTassets;
        mp->NXTasset_txids_tablep = &NXTasset_txids;
        printf("init_NXThashtables: %p %p %p %p %p\n",NXTguids,NXTaddrs,NXTassets,NXTasset_txids,otheraddrs);
    }
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
    //init_NXTAPI(0);
    if ( myipaddr != 0 )
    {
        //strcpy(MY_IPADDR,get_ipaddr());
        strcpy(mp->ipaddr,myipaddr);
    }
    //safecopy(mp->ipaddr,MY_IPADDR,sizeof(mp->ipaddr));
    mp->upollseconds = 333333 * 0;
    mp->pollseconds = POLL_SECONDS;
    //crypto_box_keypair(Global_mp->loopback_pubkey,Global_mp->loopback_privkey);
    //crypto_box_keypair(Global_mp->session_pubkey,Global_mp->session_privkey);
    if ( portable_thread_create((void *)process_hashtablequeues,mp) == 0 )
        printf("ERROR hist process_hashtablequeues\n");
    mp->udp = start_libuv_udpserver(4,SUPERNET_PORT,(void *)on_udprecv);
    init_MGWconf(JSON_or_fname,myipaddr);
    //printf("start getNXTblocks.(%s)\n",myipaddr);
    //if ( 0 && portable_thread_create((void *)getNXTblocks,mp) == 0 )
    //    printf("ERROR start_Histloop\n");
    //mp->udp = start_libuv_udpserver(4,NXT_PUNCH_PORT,(void *)on_udprecv);
    //init_pingpong_queue(&PeerQ,"PeerQ",process_PeerQ,0,0);

    //printf("run_NXTservices >>>>>>>>>>>>>>> %p %s: %s\n",mp,mp->dispname,mp->ipaddr);
    //void run_NXTservices(void *arg);
    //if ( 0 && portable_thread_create((void *)run_NXTservices,mp) == 0 )
    //    printf("ERROR hist process_hashtablequeues\n");
    Finished_loading = 1;
	//while ( Finished_loading == 0 )
    //    sleep(1);
    //printf("start Coinloop\n");
    //if ( portable_thread_create((void *)Coinloop,Global_mp) == 0 )
    //    printf("ERROR Coin_genaddrloop\n");
    printf("run_UVloop\n");
    if ( portable_thread_create((void *)run_UVloop,Global_mp) == 0 )
        printf("ERROR hist process_hashtablequeues\n");
    sleep(3);
    while ( get_coin_info("BTCD") == 0 )
        sleep(1);
}

uint64_t get_accountid(char *buf)
{
    struct coin_info *cp = get_coin_info("BTCD");
    if ( cp != 0 )
        strcpy(buf,cp->srvNXTADDR);
    else strcpy(buf,"nobtcdsrvNXTADDR");
    return(calc_nxt64bits(buf));
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
    if ( prevaddr != 0 )
        return(0);
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
    if ( prevaddr != 0 )
        return(0);
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
    int32_t L,port,len;
    copy_cJSON(destNXTaddr,objs[0]);
    copy_cJSON(msg,objs[1]);
    if ( prevaddr != 0 )
    {
        printf("GOT MESSAGE.(%s) from %s\n",msg,sender);
        return(0);
    }
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
        else
        {
            len = (int32_t)strlen(origargstr)+1;
            stripwhite_ns(origargstr,len);
            len = (int32_t)strlen(origargstr)+1;
            retstr = sendmessage(nexthopNXTaddr,L,sender,origargstr,len,destNXTaddr,0,0);
        }
    }
    //if ( retstr == 0 )
    //    retstr = clonestr("{\"error\":\"invalid sendmessage request\"}");
    return(retstr);
}

char *sendbinary_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    static int counter;
    char previp[64],nexthopNXTaddr[64],destNXTaddr[64],cmdstr[MAX_JSON_FIELD],datastr[MAX_JSON_FIELD],*retstr = 0;
    int32_t L,port;
    if ( prevaddr != 0 )
        return(0);
    copy_cJSON(destNXTaddr,objs[0]);
    copy_cJSON(datastr,objs[1]);
    L = (int32_t)get_API_int(objs[2],Global_mp->Lfactor);
    nexthopNXTaddr[0] = 0;
    //printf("sendmsg_func sender.(%s) valid.%d dest.(%s) (%s)\n",sender,valid,destNXTaddr,origargstr);
    if ( sender[0] != 0 && valid > 0 && destNXTaddr[0] != 0 )
    {
        if ( prevaddr != 0 )
        {
            port = extract_nameport(previp,sizeof(previp),(struct sockaddr_in *)prevaddr);
            fprintf(stderr,"%d >>>>>>>>>>>>> received binary message.(%s) NXT.%s from hop.%s/%d\n",counter,datastr,sender,previp,port);
            counter++;
            //retstr = clonestr("{\"result\":\"received message\"}");
        }
        else
        {
            sprintf(cmdstr,"{\"requestType\":\"sendbinary\",\"NXT\":\"%s\",\"data\":\"%s\",\"dest\":\"%s\"}",NXTaddr,datastr,destNXTaddr);
            retstr = send_tokenized_cmd(nexthopNXTaddr,L,NXTaddr,NXTACCTSECRET,cmdstr,destNXTaddr);
        }
    }
    //if ( retstr == 0 )
    //    retstr = clonestr("{\"error\":\"invalid sendmessage request\"}");
    return(retstr);
}

char *checkmsg_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char senderNXTaddr[MAX_JSON_FIELD],*retstr = 0;
    if ( prevaddr != 0 )
        return(0);
    copy_cJSON(senderNXTaddr,objs[0]);
    if ( sender[0] != 0 && valid > 0 )
        retstr = checkmessages(sender,NXTACCTSECRET,senderNXTaddr);
    else retstr = clonestr("{\"result\":\"invalid checkmessages request\"}");
    return(retstr);
}

/*
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
}*/

char *makeoffer_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    uint64_t assetA,assetB;
    double qtyA,qtyB;
    int32_t type;
    char otherNXTaddr[MAX_JSON_FIELD],*retstr = 0;
    if ( prevaddr != 0 )
        return(0);
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
    if ( prevaddr != 0 )
        return(0);
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
    if ( prevaddr != 0 )
        return(0);
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
    if ( prevaddr != 0 )
        return(0);
    value = (SATOSHIDEN * get_API_float(objs[0]));
    copy_cJSON(coinstr,objs[1]);
    printf("maketelepods.%s %.8f\n",coinstr,dstr(value));
    if ( coinstr[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = maketelepods(NXTACCTSECRET,sender,coinstr,value);
    else retstr = clonestr("{\"error\":\"invalid maketelepods_func arguments\"}");
    return(retstr);
}

char *getpeers_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    cJSON *json;
    int32_t scanflag;
    char *jsonstr = 0;
    if ( prevaddr != 0 )
        return(0);
    scanflag = get_API_int(objs[0],0);
    json = gen_peers_json(prevaddr,NXTaddr,NXTACCTSECRET,sender,scanflag);
    if ( json != 0 )
    {
        jsonstr = cJSON_Print(json);
        free_json(json);
    }
    return(jsonstr);
}

/*char *getPservers_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
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
}*/

char *savefile_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    FILE *fp;
    int32_t L,M,N;
    char pin[MAX_JSON_FIELD],fname[MAX_JSON_FIELD],usbname[MAX_JSON_FIELD],password[MAX_JSON_FIELD],*retstr = 0;
    if ( prevaddr != 0 )
        return(clonestr("{\"error\":\"savefile is only for local access\"}"));
    copy_cJSON(fname,objs[0]);
    L = get_API_int(objs[1],0);
    M = get_API_int(objs[2],1);
    N = get_API_int(objs[3],1);
    if ( N < 0 )
        N = 0;
    else if ( N > 254 )
        N = 254;
    if ( M >= N )
        M = N;
    else if ( M < 1 )
        M = 1;
    copy_cJSON(usbname,objs[4]);
    copy_cJSON(password,objs[5]);
    copy_cJSON(pin,objs[5]);
    fp = fopen(fname,"rb");
    if ( fp == 0 )
        printf("cant find file (%s)\n",fname);
    if ( fp != 0 && sender[0] != 0 && valid > 0 )
        retstr = mofn_savefile(prevaddr,NXTaddr,NXTACCTSECRET,sender,pin,fp,L,M,N,usbname,password,fname);
    else retstr = clonestr("{\"error\":\"invalid savefile_func arguments\"}");
    if ( fp != 0 )
        fclose(fp);
    return(retstr);
}

char *restorefile_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    FILE *fp;
    char txidstr[MAX_JSON_FIELD];
    cJSON *array,*item;
    uint64_t *txids = 0;
    int32_t L,M,N,i,n;
    char pin[MAX_JSON_FIELD],fname[MAX_JSON_FIELD],sharenrs[MAX_JSON_FIELD],destfname[MAX_JSON_FIELD],usbname[MAX_JSON_FIELD],password[MAX_JSON_FIELD],*retstr = 0;
    if ( prevaddr != 0 )
        return(clonestr("{\"error\":\"restorefile is only for local access\"}"));
    copy_cJSON(fname,objs[0]);
    L = get_API_int(objs[1],0);
    M = get_API_int(objs[2],1);
    N = get_API_int(objs[3],1);
    if ( N < 0 )
        N = 0;
    else if ( N > 254 )
        N = 254;
    if ( M >= N )
        M = N;
    else if ( M < 1 )
        M = 1;
    copy_cJSON(usbname,objs[4]);
    copy_cJSON(password,objs[5]);
    copy_cJSON(destfname,objs[6]);
    copy_cJSON(sharenrs,objs[7]);
    array = objs[8];
    if ( is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
    {
        txids = calloc(n+1,sizeof(*txids));
        for (i=0; i<n; i++)
        {
            item = cJSON_GetArrayItem(array,i);
            copy_cJSON(txidstr,item);
            if ( txidstr[0] != 0 )
                txids[i] = calc_nxt64bits(txidstr);
        }
    }
    copy_cJSON(pin,objs[9]);
    if ( destfname[0] == 0 )
        strcpy(destfname,fname), strcat(destfname,".restore");
    if ( 0 )
    {
        fp = fopen(destfname,"rb");
        if ( fp != 0 )
        {
            fclose(fp);
            return(clonestr("{\"error\":\"destfilename is already exists\"}"));
        }
    }
    fp = fopen(destfname,"wb");
    if ( fp != 0 && sender[0] != 0 && valid > 0 && destfname[0] != 0  )
        retstr = mofn_restorefile(prevaddr,NXTaddr,NXTACCTSECRET,sender,pin,fp,L,M,N,usbname,password,fname,sharenrs,txids);
    else retstr = clonestr("{\"error\":\"invalid savefile_func arguments\"}");
    if ( fp != 0 )
        fclose(fp);
    if ( txids != 0 )
        free(txids);
    {
        char cmdstr[512];
        printf("\n*****************\ncompare (%s) vs (%s)\n",fname,destfname);
        sprintf(cmdstr,"cmp %s %s",fname,destfname);
        system(cmdstr);
        printf("done\n\n");
    }
    return(retstr);
}

char *findaddress_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char txidstr[MAX_JSON_FIELD],*retstr = 0;
    cJSON *array,*item;
    struct coin_info *cp = get_coin_info("BTCD");
    int32_t targetdist,numthreads,duration,i,n = 0;
    uint64_t refaddr,*txids = 0;
    if ( prevaddr != 0 )
        return(clonestr("{\"error\":\"can only findaddress locally\"}"));
    refaddr = get_API_nxt64bits(objs[0]);
    array = objs[1];
    targetdist = (int32_t)get_API_int(objs[2],10);
    duration = (int32_t)get_API_int(objs[3],60);
    numthreads = (int32_t)get_API_int(objs[4],8);
    if ( is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
    {
        txids = calloc(n+1,sizeof(*txids));
        for (i=0; i<n; i++)
        {
            item = cJSON_GetArrayItem(array,i);
            copy_cJSON(txidstr,item);
            if ( txidstr[0] != 0 )
                txids[i] = calc_nxt64bits(txidstr);
        }
    }
    else if ( (n= cp->numnxtaccts) > 0 )
    {
        txids = calloc(n+1,sizeof(*txids));
        memcpy(txids,cp->nxtaccts,n*sizeof(*txids));
    }
    else
    {
        n = 512;
        txids = calloc(n+1,sizeof(*txids));
        for (i=0; i<n; i++)
            randombytes((unsigned char *)&txids[i],sizeof(txids[i]));
    }
    if ( txids != 0 && sender[0] != 0 && valid > 0 )
        retstr = findaddress(prevaddr,NXTaddr,NXTACCTSECRET,sender,refaddr,txids,n,targetdist,duration,numthreads);
    else retstr = clonestr("{\"error\":\"invalid findaddress_func arguments\"}");
    //if ( txids != 0 ) freed on completion
    //    free(txids);
    return(retstr);
}

char *sendfile_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    FILE *fp;
    int32_t L;
    char fname[MAX_JSON_FIELD],dest[MAX_JSON_FIELD],*retstr = 0;
    if ( prevaddr != 0 )
        return(0);
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

char *ping_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    int32_t port;
    char pubkey[MAX_JSON_FIELD],destip[MAX_JSON_FIELD],ipaddr[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(pubkey,objs[0]);
    copy_cJSON(ipaddr,objs[1]);
    port = get_API_int(objs[2],0);
    copy_cJSON(destip,objs[3]);
    //printf("ping got sender.(%s) valid.%d pubkey.(%s) ipaddr.(%s) port.%d destip.(%s)\n",sender,valid,pubkey,ipaddr,port,destip);
    if ( sender[0] != 0 && valid > 0 )
        retstr = kademlia_ping(prevaddr,NXTaddr,NXTACCTSECRET,sender,ipaddr,port,destip);
    else retstr = clonestr("{\"error\":\"invalid ping_func arguments\"}");
    return(retstr);
}

char *pong_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char pubkey[MAX_JSON_FIELD],ipaddr[MAX_JSON_FIELD],*retstr = 0;
    uint16_t port;
    if ( prevaddr == 0 )
        return(0);
    copy_cJSON(pubkey,objs[0]);
    copy_cJSON(ipaddr,objs[1]);
    port = get_API_int(objs[2],0);
    //printf("pong got pubkey.(%s) ipaddr.(%s) port.%d \n",pubkey,ipaddr,port);
    if ( sender[0] != 0 && valid > 0 )
        retstr = kademlia_pong(prevaddr,NXTaddr,NXTACCTSECRET,sender,ipaddr,port);
    else retstr = clonestr("{\"error\":\"invalid pong_func arguments\"}");
    return(retstr);
}

char *addcontact_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char handle[MAX_JSON_FIELD],acct[MAX_JSON_FIELD],*retstr = 0;
    if ( prevaddr != 0 )
        return(0);
    copy_cJSON(handle,objs[0]);
    copy_cJSON(acct,objs[1]);
    printf("handle.(%s) acct.(%s) valid.%d\n",handle,acct,valid);
    if ( handle[0] != 0 && acct[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = addcontact(prevaddr,NXTaddr,NXTACCTSECRET,sender,handle,acct);
    else retstr = clonestr("{\"error\":\"invalid addcontact_func arguments\"}");
    return(retstr);
}

char *removecontact_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char handle[MAX_JSON_FIELD],*retstr = 0;
    if ( prevaddr != 0 )
        return(0);
    copy_cJSON(handle,objs[0]);
    if ( handle[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = removecontact(prevaddr,NXTaddr,NXTACCTSECRET,sender,handle);
    else retstr = clonestr("{\"error\":\"invalid removecontact_func arguments\"}");
    return(retstr);
}

char *dispcontact_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char handle[MAX_JSON_FIELD],*retstr = 0;
    if ( prevaddr != 0 )
        return(0);
    copy_cJSON(handle,objs[0]);
    if ( handle[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = dispcontact(prevaddr,NXTaddr,NXTACCTSECRET,sender,handle);
    else retstr = clonestr("{\"error\":\"invalid dispcontact arguments\"}");
    return(retstr);
}

void set_kademlia_args(char *key,cJSON *keyobj,cJSON *nameobj)
{
    uint64_t hash;
    long len;
    char name[MAX_JSON_FIELD];
    key[0] = 0;
    copy_cJSON(name,nameobj);
    if ( name[0] != 0 )
    {
        len = strlen(name);
        if ( len < 64 )
        {
            hash = calc_txid((unsigned char *)name,(int32_t)strlen(name));
            expand_nxt64bits(key,hash);
        }
    }
    else copy_cJSON(key,keyobj);
}

char *findnode_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char pubkey[MAX_JSON_FIELD],key[MAX_JSON_FIELD],value[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(pubkey,objs[0]);
    set_kademlia_args(key,objs[1],objs[2]);
    copy_cJSON(value,objs[3]);
    if ( Debuglevel > 1 )
        printf("findnode.%p (%s) (%s) (%s) (%s)\n",prevaddr,sender,pubkey,key,value);
    if ( key[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = kademlia_find("findnode",prevaddr,NXTaddr,NXTACCTSECRET,sender,key,value,origargstr);
    else retstr = clonestr("{\"error\":\"invalid findnode_func arguments\"}");
    return(retstr);
}

char *findvalue_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char pubkey[MAX_JSON_FIELD],key[MAX_JSON_FIELD],value[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(pubkey,objs[0]);
    set_kademlia_args(key,objs[1],objs[2]);
    copy_cJSON(value,objs[3]);
    if ( Debuglevel > 1 )
        printf("findvalue.%p (%s) (%s) (%s)\n",prevaddr,sender,pubkey,key);
    if ( key[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = kademlia_find("findvalue",prevaddr,NXTaddr,NXTACCTSECRET,sender,key,value,origargstr);
    else retstr = clonestr("{\"error\":\"invalid findvalue_func arguments\"}");
    if ( Debuglevel > 1 )
        printf("back from findvalue\n");
    return(retstr);
}

char *havenode_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char pubkey[MAX_JSON_FIELD],key[MAX_JSON_FIELD],value[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(pubkey,objs[0]);
    set_kademlia_args(key,objs[1],objs[2]);
    copy_cJSON(value,objs[3]);
    if ( key[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = kademlia_havenode(0,prevaddr,NXTaddr,NXTACCTSECRET,sender,key,value);
    else retstr = clonestr("{\"error\":\"invalid havenode_func arguments\"}");
    return(retstr);
}

char *havenodeB_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char pubkey[MAX_JSON_FIELD],key[MAX_JSON_FIELD],value[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(pubkey,objs[0]);
    set_kademlia_args(key,objs[1],objs[2]);
    copy_cJSON(value,objs[3]);
    if ( key[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = kademlia_havenode(1,prevaddr,NXTaddr,NXTACCTSECRET,sender,key,value);
    else retstr = clonestr("{\"error\":\"invalid havenode_func arguments\"}");
    return(retstr);
}

char *store_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char pubkey[MAX_JSON_FIELD],key[MAX_JSON_FIELD],datastr[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(pubkey,objs[0]);
    set_kademlia_args(key,objs[1],objs[2]);
    copy_cJSON(datastr,objs[3]);
    if ( key[0] != 0 && sender[0] != 0 && valid > 0 && datastr[0] != 0 )
    {
        retstr = kademlia_storedata(prevaddr,NXTaddr,NXTACCTSECRET,sender,key,datastr);
    }
    else retstr = clonestr("{\"error\":\"invalid store_func arguments\"}");
    return(retstr);
}

char *cosign_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    static unsigned char zerokey[32];
    char retbuf[MAX_JSON_FIELD],plaintext[MAX_JSON_FIELD],seedstr[MAX_JSON_FIELD],otheracctstr[MAX_JSON_FIELD],hexstr[65],ret0str[65];
    bits256 ret,ret0,seed,priv,pub,sha;
    struct nodestats *stats;
    // WARNING: if this is being remotely invoked, make sure you trust the requestor as the rawkey is being sent
    
    copy_cJSON(otheracctstr,objs[0]);
    copy_cJSON(seedstr,objs[1]);
    copy_cJSON(plaintext,objs[2]);
    if ( seedstr[0] != 0 )
        decode_hex(seed.bytes,sizeof(seed),seedstr);
    if ( plaintext[0] != 0 )
    {
        calc_sha256(0,sha.bytes,(unsigned char *)plaintext,(int32_t)strlen(plaintext));
        if ( seedstr[0] != 0 && memcmp(seed.bytes,sha.bytes,sizeof(seed)) != 0 )
            printf("cosign_func: error comparing seed %llx with sha256 %llx?n",(long long)seed.ulongs[0],(long long)sha.ulongs[0]);
        seed = sha;
    }
    if ( seedstr[0] == 0 )
        init_hexbytes(seedstr,seed.bytes,sizeof(seed));
    stats = get_nodestats(calc_nxt64bits(otheracctstr));
    if ( strlen(seedstr) == 64 && sender[0] != 0 && valid > 0 && stats != 0 && memcmp(stats->pubkey,zerokey,sizeof(stats->pubkey)) != 0 )
    {
        memcpy(priv.bytes,Global_mp->loopback_privkey,sizeof(priv));
        memcpy(pub.bytes,stats->pubkey,sizeof(pub));
        ret0 = curve25519(priv,pub);
        init_hexbytes(ret0str,ret0.bytes,sizeof(ret0));
        ret = xor_keys(seed,ret0);
        init_hexbytes(hexstr,ret.bytes,sizeof(ret));
        sprintf(retbuf,"{\"requestType\":\"cosigned\",\"seed\":\"%s\",\"result\":\"%s\",\"privacct\":\"%s\",\"pubacct\":\"%s\",\"ret0\":\"%s\"}",seedstr,ret0str,NXTaddr,otheracctstr,hexstr);
        return(clonestr(retbuf));
    }
    return(clonestr("{\"error\":\"invalid cosign_func arguments\"}"));
}

char *cosigned_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char retbuf[MAX_JSON_FIELD],resultstr[MAX_JSON_FIELD],seedstr[MAX_JSON_FIELD],hexstr[65];
    bits256 ret,seed,priv,val;
    uint64_t privacct,pubacct;
    // 0 ABc sha256_key(xor_keys(seed,curve25519(A,curve25519(B,c))))
    // 2 AbC sha256_key(xor_keys(seed,curve25519(A,curve25519(C,b))))
    // 4 ABc sha256_key(xor_keys(seed,curve25519(B,curve25519(A,c))))
    // 6 aBC sha256_key(xor_keys(seed,curve25519(B,curve25519(C,a))))
    // 8 AbC sha256_key(xor_keys(seed,curve25519(C,curve25519(A,b))))
    // 10 aBC sha256_key(xor_keys(seed,curve25519(C,curve25519(B,a))))
    copy_cJSON(seedstr,objs[0]);
    copy_cJSON(resultstr,objs[1]);
    privacct = get_API_nxt64bits(objs[2]);
    pubacct = get_API_nxt64bits(objs[3]);
    if ( strlen(seedstr) == 64 && sender[0] != 0 && valid > 0 )
    {
        decode_hex(seed.bytes,sizeof(seed),seedstr);
        decode_hex(val.bytes,sizeof(val),resultstr);
        memcpy(priv.bytes,Global_mp->loopback_privkey,sizeof(priv));
        ret = sha256_key(xor_keys(seed,curve25519(priv,val)));
        init_hexbytes(hexstr,ret.bytes,sizeof(ret));
        sprintf(retbuf,"{\"seed\":\"%s\",\"result\":\"%s\",\"acct\":\"%s\",\"privacct\":\"%llu\",\"pubacct\":\"%llu\",\"input\":\"%s\"}",seedstr,hexstr,NXTaddr,(long long)privacct,(long long)pubacct,resultstr);
        return(clonestr(retbuf));
    }
    return(clonestr("{\"error\":\"invalid cosigned_func arguments\"}"));
}

char *pNXT_json_commands(struct NXThandler_info *mp,struct sockaddr *prevaddr,cJSON *origargjson,char *sender,int32_t valid,char *origargstr)
{
    // multisig
    static char *cosign[] = { (char *)cosign_func, "cosign", "V", "otheracct", "seed", "text", 0 };
    static char *cosigned[] = { (char *)cosigned_func, "cosigned", "V", "seed", "result", "privacct", "pubacct", 0 };

    // Kademlia DHT
    static char *ping[] = { (char *)ping_func, "ping", "V", "pubkey", "ipaddr", "port", "destip", 0 };
    static char *pong[] = { (char *)pong_func, "pong", "V", "pubkey", "ipaddr", "port", 0 };
    static char *store[] = { (char *)store_func, "store", "V", "pubkey", "key", "name", "data", 0 };
    static char *findvalue[] = { (char *)findvalue_func, "findvalue", "V", "pubkey", "key", "name", "data", 0 };
    static char *findnode[] = { (char *)findnode_func, "findnode", "V", "pubkey", "key", "name", "data", 0 };
    static char *havenode[] = { (char *)havenode_func, "havenode", "V", "pubkey", "key", "name", "data", 0 };
    static char *havenodeB[] = { (char *)havenodeB_func, "havenodeB", "V", "pubkey", "key", "name", "data", 0 };
    static char *findaddress[] = { (char *)findaddress_func, "findaddress", "V", "refaddr", "list", "dist", "duration", "numthreads", 0 };

    // MofNfs
    static char *savefile[] = { (char *)savefile_func, "savefile", "V", "filename", "L", "M", "N", "backup", "password", "pin", 0 };
    static char *restorefile[] = { (char *)restorefile_func, "restorefile", "V", "filename", "L", "M", "N", "backup", "password", "destfile", "sharenrs", "txids", "pin", 0 };
    static char *sendfile[] = { (char *)sendfile_func, "sendfile", "V", "filename", "dest", "L", 0 };
    
    // Telepathy
    static char *getpeers[] = { (char *)getpeers_func, "getpeers", "V",  "scan", 0 };
    static char *addcontact[] = { (char *)addcontact_func, "addcontact", "V",  "handle", "acct", 0 };
    static char *removecontact[] = { (char *)removecontact_func, "removecontact", "V",  "contact", 0 };
    static char *dispcontact[] = { (char *)dispcontact_func, "dispcontact", "V",  "contact", 0 };
    static char *telepathy[] = { (char *)telepathy_func, "telepathy", "V",  "contact", "msg", "id", 0 };
    static char *sendmsg[] = { (char *)sendmsg_func, "sendmessage", "V", "dest", "msg", "L", 0 };
    static char *sendbinary[] = { (char *)sendbinary_func, "sendbinary", "V", "dest", "data", "L", 0 };
    static char *checkmsg[] = { (char *)checkmsg_func, "checkmessages", "V", "sender", 0 };

    // Teleport
    static char *maketelepods[] = { (char *)maketelepods_func, "maketelepods", "V", "amount", "coin", 0 };
    static char *teleport[] = { (char *)teleport_func, "teleport", "V", "amount", "dest", "coin", "minage", "M", "N", 0 };
    static char *telepod[] = { (char *)telepod_func, "telepod", "V", "crc", "i", "h", "c", "v", "a", "t", "o", "p", "k", "L", "s", "M", "N", "D", 0 };
    static char *transporter[] = { (char *)transporter_func, "transporter", "V", "coin", "height", "minage", "value", "totalcrc", "telepods", "M", "N", "sharenrs", "pubaddr", 0 };
    static char *transporterstatus[] = { (char *)transporterstatus_func, "transporter_status", "V", "status", "coin", "totalcrc", "value", "num", "minage", "height", "crcs", "sharei", "M", "N", "sharenrs", "ind", "pubaddr", 0 };
    
   // InstantDEX
    static char *respondtx[] = { (char *)respondtx_func, "respondtx", "V", "signedtx", 0 };
    static char *processutx[] = { (char *)processutx_func, "processutx", "V", "utx", "sig", "full", 0 };
    static char *orderbook[] = { (char *)orderbook_func, "orderbook", "V", "obookid", "polarity", "allfields", 0 };
    static char *getorderbooks[] = { (char *)getorderbooks_func, "getorderbooks", "V", 0 };
    static char *placebid[] = { (char *)placebid_func, "placebid", "V", "obookid", "polarity", "volume", "price", "assetA", "assetB", 0 };
    static char *placeask[] = { (char *)placeask_func, "placeask", "V", "obookid", "polarity", "volume", "price", "assetA", "assetB", 0 };
    static char *makeoffer[] = { (char *)makeoffer_func, "makeoffer", "V", "other", "assetA", "qtyA", "assetB", "qtyB", "type", 0 };
    
    // Tradebot
    static char *tradebot[] = { (char *)tradebot_func, "tradebot", "V", "code", 0 };

     static char **commands[] = { cosign, cosigned, telepathy, addcontact, dispcontact, removecontact, findaddress, ping, pong, store, findnode, havenode, havenodeB, findvalue, sendfile, getpeers, maketelepods, transporterstatus, telepod, transporter, tradebot, respondtx, processutx, checkmsg, placebid, placeask, makeoffer, sendmsg, sendbinary, orderbook, getorderbooks, teleport, savefile, restorefile  };
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
            if ( prevaddr == 0 || strcmp(command,"findnode") == 0 )
            {
                if ( 1 || strcmp("127.0.0.1",cp->privacyserver) == 0 )
                {
                    safecopy(NXTACCTSECRET,cp->srvNXTACCTSECRET,sizeof(NXTACCTSECRET));
                    expand_nxt64bits(NXTaddr,cp->srvpubnxtbits);
                    //printf("use localserver NXT.%s to send command\n",NXTaddr);
                }
                else
                {
                    safecopy(NXTACCTSECRET,cp->privateNXTACCTSECRET,sizeof(NXTACCTSECRET));
                    expand_nxt64bits(NXTaddr,cp->privatebits);
                    //printf("use NXT.%s to send command\n",NXTaddr);
                }
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

char *call_SuperNET_JSON(char *JSONstr)
{
    cJSON *json,*array;
    int32_t valid;
    char NXTaddr[64],_tokbuf[2*MAX_JSON_FIELD],encoded[NXT_TOKEN_LEN+1],*cmdstr,*retstr = 0;
    struct coin_info *cp = get_coin_info("BTCD");
    if ( Finished_init == 0 )
        return(0);
    //printf("got call_SuperNET_JSON.(%s)\n",JSONstr);
    if ( cp != 0 && (json= cJSON_Parse(JSONstr)) != 0 )
    {
        expand_nxt64bits(NXTaddr,cp->srvpubnxtbits);
        cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(NXTaddr));
        cmdstr = cJSON_Print(json);
        if ( cmdstr != 0 )
        {
            stripwhite_ns(cmdstr,strlen(cmdstr));
            issue_generateToken(0,encoded,cmdstr,cp->srvNXTACCTSECRET);
            encoded[NXT_TOKEN_LEN] = 0;
            sprintf(_tokbuf,"[%s,{\"token\":\"%s\"}]",cmdstr,encoded);
            free(cmdstr);
            array = cJSON_Parse(_tokbuf);
            if ( array != 0 )
            {
                cmdstr = verify_tokenized_json(0,NXTaddr,&valid,array);
                retstr = pNXT_json_commands(Global_mp,0,array,NXTaddr,valid,_tokbuf);
                if ( cmdstr != 0 )
                    free(cmdstr);
                free_json(array);
            }
        }
        free_json(json);
    } else printf("couldnt parse (%s)\n",JSONstr);
    if ( retstr == 0 )
        retstr = clonestr("{\"result\":null}");
    return(retstr);
}

int32_t is_BTCD_command(cJSON *json)
{
    char *BTCDcmds[] = { "maketelepods", "teleport", "telepod", "transporter", "transporter_status" };
    char request[MAX_JSON_FIELD];
    long i;
    if ( extract_cJSON_str(request,sizeof(request),json,"requestType") > 0 )
    {
        for (i=0; i<(sizeof(BTCDcmds)/sizeof(*BTCDcmds)); i++)
        {
            //printf("(%s vs %s) ",request,BTCDcmds[i]);
            if ( strcmp(request,BTCDcmds[i]) == 0 )
                return(1);
        }
    }
    return(0);
}
    
char *SuperNET_JSON(char *JSONstr)
{
    char *retstr = 0;
    struct coin_info *cp = get_coin_info("BTCD");
    cJSON *json;
    if ( Finished_init == 0 )
        return(0);
    if ( Debuglevel > 0 )
        printf("got JSON.(%s)\n",JSONstr);
    if ( cp != 0 && (json= cJSON_Parse(JSONstr)) != 0 )
    {
        if ( is_BTCD_command(json) != 0 ) // deadlocks as the SuperNET API came from locked BTCD RPC
        {
            if ( Debuglevel > 1 )
                printf("is_BTCD_command\n");
            queue_enqueue(&JSON_Q,clonestr(JSONstr));
            return(clonestr("{\"result\":\"SuperNET BTCD command queued\"}"));
        } else retstr = call_SuperNET_JSON(JSONstr);
        free_json(json);
    } else printf("couldnt parse (%s)\n",JSONstr);
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
                fatal("pNXT_handler: NO GLOBALS!!!");
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

uint64_t call_SuperNET_broadcast(struct pserver_info *pserver,char *msg,int32_t len,int32_t duration)
{
    int32_t SuperNET_broadcast(char *msg,int32_t duration);
    int32_t SuperNET_narrowcast(char *destip,unsigned char *msg,int32_t len);
    char ip_port[64],ipaddr[64];
    struct nodestats *stats;
    uint64_t txid = 0;
    int32_t port;
    if ( Debuglevel > 1 )
        printf("call_SuperNET_broadcast.%p %p len.%d\n",pserver,msg,len);
    txid = calc_txid((uint8_t *)msg,(int32_t)strlen(msg));
    if ( pserver != 0 )
    {
        if ( (stats= get_nodestats(pserver->nxt64bits)) != 0 )
            port = (stats->p2pport == 0) ? BTCD_PORT : stats->p2pport;
        else port = BTCD_PORT;
        sprintf(ip_port,"%s:%d",pserver->ipaddr,port);
        txid ^= calc_ipbits(ipaddr);
        if ( Debuglevel > 1 )
            printf("%s NARROWCAST.(%s) txid.%llu (%s)\n",pserver->ipaddr,msg,(long long)txid,ip_port);
        if ( SuperNET_narrowcast(ip_port,(unsigned char *)msg,len) == 0 )
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
            cmdstr = verify_tokenized_json(0,NXTaddr,&valid,array);
            if ( cmdstr != 0 )
                free(cmdstr);
            free_json(array);
            if ( Debuglevel > 1 )
                printf("BROADCAST parms.(%s) valid.%d duration.%d txid.%llu\n",msg,valid,duration,(long long)txid);
            if ( SuperNET_broadcast(msg,duration) == 0 )
                return(txid);
        } else printf("cant broadcast non-JSON.(%s)\n",msg);
    }
    return(txid);
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
    uint16_t p2pport;
    struct pserver_info *pserver;
    uint64_t txid,obookid;
    struct sockaddr prevaddr;
    int32_t len,createdflag,valid;
    unsigned char packet[2*MAX_JSON_FIELD];
    char ipaddr[64],txidstr[64],retjsonstr[2*MAX_JSON_FIELD],verifiedNXTaddr[64],*cmdstr,*retstr;
    if ( Debuglevel > 0 )
        printf("gotpacket.(%s) duration.%d from (%s)\n",msg,duration,ip_port);
    strcpy(retjsonstr,"{\"result\":null}");
    if ( Finished_loading == 0 )
    {
        if ( is_hexstr(msg) == 0 )
        {
            printf("QUEUE.(%s)\n",msg);
            queue_enqueue(&JSON_Q,clonestr(msg));
        }
        return(clonestr(retjsonstr));
    }
    p2pport = parse_ipaddr(ipaddr,ip_port);
    uv_ip4_addr(ipaddr,p2pport,(struct sockaddr_in *)&prevaddr);
    pserver = get_pserver(0,ipaddr,0,p2pport);
    len = (int32_t)strlen(msg);
    if ( is_hexstr(msg) != 0 )
    {
        len >>= 1;
        decode_hex(packet,len,msg);
        txid = calc_txid(packet,len);//hash,sizeof(hash));
        sprintf(txidstr,"%llu",(long long)txid);
        MTadd_hashtable(&createdflag,&Global_pNXT->msg_txids,txidstr);
        if ( createdflag == 0 )
        {
            duplicates++;
            return(clonestr("{\"error\":\"duplicate msg\"}"));
        }
        if ( (len<<1) == 30 ) // hack against flood
            flood++;
        if ( Debuglevel > 0 )
            printf("gotpacket.(%s) %d | Finished_loading.%d | flood.%d duplicates.%d\n",msg,duration,Finished_loading,flood,duplicates);
        if ( is_encrypted_packet(packet,len) != 0 )
            process_packet(0,retjsonstr,packet,len,0,&prevaddr,ipaddr,0);
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
        //calc_sha256(0,hash,(uint8_t *)msg,len);
        txid = calc_txid((uint8_t *)msg,len);//hash,sizeof(hash));
        sprintf(txidstr,"%llu",(long long)txid);
        MTadd_hashtable(&createdflag,&Global_pNXT->msg_txids,txidstr);
        if ( createdflag == 0 )
        {
            duplicates++;
            return(clonestr("{\"error\":\"duplicate msg\"}"));
        }
        if ( len == 30 ) // hack against flood
            flood++;
        if ( Debuglevel > 0 )
            printf("C SuperNET_gotpacket.(%s) from %s:%d size.%d ascii txid.%llu | flood.%d\n",msg,ipaddr,p2pport,len,(long long)txid,flood);
        if ( (json= cJSON_Parse((char *)msg)) != 0 )
        {
            cJSON *argjson;
            char pubkeystr[MAX_JSON_FIELD];
            cmdstr = verify_tokenized_json(0,verifiedNXTaddr,&valid,json);
            pubkeystr[0] = 0;
            if ( (argjson= cJSON_Parse(cmdstr)) != 0 )
            {
                copy_cJSON(pubkeystr,cJSON_GetObjectItem(argjson,"pubkey"));
                if ( pubkeystr[0] != 0 )
                    add_new_node(calc_nxt64bits(verifiedNXTaddr));
                free_json(argjson);
            }
            //printf("update (%s) (%s:%d) %s\n",verifiedNXTaddr,ipaddr,p2pport,pubkeystr);
            kademlia_update_info(verifiedNXTaddr,ipaddr,p2pport,pubkeystr,(int32_t)time(NULL),1);
            retstr = pNXT_json_commands(Global_mp,&prevaddr,json,verifiedNXTaddr,valid,(char *)msg);
            if ( cmdstr != 0 )
                free(cmdstr);
            free_json(json);
            if ( retstr == 0 )
                retstr = clonestr("{\"result\":null}");
            return(retstr);
        }
        else printf("cJSON_Parse error.(%s)\n",msg);
    }
    return(clonestr(retjsonstr));
}

int SuperNET_start(char *JSON_or_fname,char *myipaddr)
{
    FILE *fp = 0;
    struct coin_info *cp;
    struct NXT_str *tp = 0;
    printf("SuperNET_start(%s) %p ipaddr.(%s)\n",JSON_or_fname,myipaddr,myipaddr);
    if ( JSON_or_fname != 0 && JSON_or_fname[0] != '{' )
    {
        fp = fopen(JSON_or_fname,"rb");
        if ( fp == 0 )
            return(-1);
        fclose(fp);
    }
    portable_mutex_init(&Contacts_mutex);
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
    free(myipaddr);
    p2p_publishpacket(0,0);
    if ( (cp= get_coin_info("BTCD")) == 0 || cp->srvNXTACCTSECRET[0] == 0 || cp->srvNXTADDR[0] == 0 )
    {
        fprintf(stderr,"need to have BTCD active and also srvpubaddr\n");
        exit(-1);
    }
    Finished_init = 1;
    return(0);
}

