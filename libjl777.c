

//
//  jl777.cpp
//  glue code for pNXT
//
//  Created by jl777 on 7/6/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#include "jl777.h"

uv_async_t Tasks_async;
uv_work_t Tasks;
struct task_info
{
    char name[64];
    int32_t sleepmicros,argsize;
    tfunc func;
    uint8_t args[];
};

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

void SuperNET_idler(uv_idle_t *handle)
{
    static int counter;
    static double lastattempt;
    double millis;
    struct udp_queuecmd *qp;
    void *wr;
    char *jsonstr,*retstr;
    if ( Finished_init == 0 )
        return;
    millis = ((double)uv_hrtime() / 1000000);
    if ( millis > (lastattempt + 10) && (wr= queue_dequeue(&sendQ)) != 0 )
    {
        if ( ((rand()>>8) % 100) < 50 )
        {
            //printf("skip packet\n");
            queue_enqueue(&sendQ,wr);
            wr = queue_dequeue(&sendQ);
        }
        if ( wr != 0 )
        {
            process_sendQ_item(wr);
            lastattempt = millis;
        }
        // free(wr); libuv does this
    }
    if ( millis > (lastattempt + 10) && (qp= queue_dequeue(&udp_JSON)) != 0 )
    {
        //printf("process qp argjson.%p\n",qp->argjson);
        jsonstr = SuperNET_json_commands(Global_mp,&qp->prevaddr,qp->argjson,qp->tokenized_np->H.U.NXTaddr,qp->valid,qp->decoded);
        //printf("free qp (%s) argjson.%p\n",jsonstr,qp->argjson);
        if ( jsonstr != 0 )
            free(jsonstr);
        free(qp->decoded);
        free_json(qp->argjson);
        free(qp);
        lastattempt = millis;
    }
    if ( millis > (lastattempt + 1000) )
    {
        process_storageQ();
        every_second(counter);
        retstr = findaddress(0,0,0,0,0,0,0,0,0,0);
        if ( retstr != 0 )
        {
            printf("findaddress completed (%s)\n",retstr);
            free(retstr);
        }
        if ( (counter % 60) == 17 )
        {
            every_minute(counter/60);
        }
        counter++;
        process_pingpong_queue(&PeerQ,0);
        process_pingpong_queue(&Transporter_sendQ,0);
        process_pingpong_queue(&Transporter_recvQ,0);
        process_pingpong_queue(&CloneQ,0);
        lastattempt = millis;
    }
    else if ( (jsonstr= queue_dequeue(&JSON_Q)) != 0 )
    {
        char *call_SuperNET_JSON(char *JSONstr);
        if ( (retstr= call_SuperNET_JSON(jsonstr)) != 0 )
        {
            printf("(%s) -> (%s)\n",jsonstr,retstr);
            free(retstr);
        }
        free(jsonstr);
    }
    usleep(1000);
}

void run_UVloop(void *arg)
{
    uv_idle_t idler;
    uv_idle_init(UV_loop,&idler);
    //uv_async_init(UV_loop,&Tasks_async,async_handler);
    uv_idle_start(&idler,SuperNET_idler);
    //uv_idle_start(&idler,NXTservices_idler);
    uv_run(UV_loop,UV_RUN_DEFAULT);
    printf("end of uv_run\n");
}

/*void run_NXTservices(void *arg)
{
    void *pNXT_handler(struct NXThandler_info *mp,struct NXT_protocol_parms *parms,void *handlerdata,int32_t height);
    struct NXThandler_info *mp = arg;
    printf("inside run_NXTservices %p\n",mp);
    register_NXT_handler("pNXT",mp,2,NXTPROTOCOL_ILLEGALTYPE,pNXT_handler,pNXT_SIG,1,0,0);
    printf("NXTloop\n");
    NXTloop(mp);
    printf("NXTloop done\n");
    while ( 1 ) sleep(60);
}*/

void init_NXThashtables(struct NXThandler_info *mp)
{
    struct other_addr *op = 0;
    struct NXT_acct *np = 0;
    struct NXT_asset *ap = 0;
    struct NXT_assettxid *tp = 0;
    struct NXT_guid *gp = 0;
    struct pserver_info *pp = 0;
    struct telepathy_entry *tel = 0;
    //struct kademlia_storage *sp = 0;
    static struct hashtable *NXTasset_txids,*NXTaddrs,*NXTassets,*NXTguids,*otheraddrs,*Pserver,*Telepathy_hash;
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
    if ( Pserver == 0 )
        Pserver = hashtable_create("Pservers",HASHTABLES_STARTSIZE,sizeof(struct pserver_info),((long)&pp->ipaddr[0] - (long)pp),sizeof(pp->ipaddr),((long)&pp->modified - (long)pp));
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

/*void process_pNXT_AM(struct pNXT_info *dp,struct NXT_protocol_parms *parms)
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
}*/

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
                retstr = SuperNET_json_commands(Global_mp,0,array,NXTaddr,valid,_tokbuf);
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

/*void *pNXT_handler(struct NXThandler_info *mp,struct NXT_protocol_parms *parms,void *handlerdata,int32_t height)
{
    struct pNXT_info *gp = handlerdata;
    if ( parms->txid == 0 )     // indicates non-transaction event
    {
        //if ( parms->mode == NXTPROTOCOL_WEBJSON )
        //{
           // printf(":7777 interface deprecated\n");
            //return(pNXT_jsonhandler(&parms->argjson,parms->argstr,0));
        //}
        //else
        if ( parms->mode == NXTPROTOCOL_NEWBLOCK )
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
}*/

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
            retstr = SuperNET_json_commands(Global_mp,&prevaddr,json,verifiedNXTaddr,valid,(char *)msg);
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
    init_storage();
    Finished_init = 1;
    return(0);
}

