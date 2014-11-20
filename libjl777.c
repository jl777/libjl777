

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

char *get_public_srvacctsecret()
{
    struct coin_info *cp = get_coin_info("BTCD");
    if ( cp != 0 )
        return(cp->srvNXTACCTSECRET);
    else return(GENESIS_SECRET);
}

int32_t expire_nodestats(struct nodestats *stats,uint32_t now)
{
    if ( stats->lastcontact != 0 && (now - stats->lastcontact) > NODESTATS_EXPIRATION )
    {
        char ipaddr[64];
        expand_ipbits(ipaddr,stats->ipbits);
        printf("expire_nodestats %s | %u %u %d\n",ipaddr,now,stats->lastcontact,now - stats->lastcontact);
        stats->gotencrypted = stats->modified = 0;
        stats->sentmilli = 0;
        stats->expired = 1;
        return(1);
    }
    return(0);
}

void every_minute(int32_t counter)
{
    static int broadcast_count;
    uint32_t now = (uint32_t)time(NULL);
    int32_t i,n,numnodes,len;
    char ipaddr[64],_cmd[MAX_JSON_FIELD];
    uint8_t finalbuf[MAX_JSON_FIELD];
    struct coin_info *cp;
    struct nodestats *stats,**nodes;
    struct pserver_info *pserver;
    if ( Finished_init == 0 )
        return;
    now = (uint32_t)time(NULL);
    cp = get_coin_info("BTCD");
    if ( cp == 0 )
        return;
    //printf("<<<<<<<<<<<<< EVERY_MINUTE\n");
    refresh_buckets(cp->srvNXTACCTSECRET);
    if ( broadcast_count == 0 )
    {
        p2p_publishpacket(0,0);
        update_Kbuckets(get_nodestats(cp->srvpubnxtbits),cp->srvpubnxtbits,cp->myipaddr,0,0,0);
        nodes = (struct nodestats **)copy_all_DBentries(&numnodes,NODESTATS_DATA);
        if ( nodes != 0 )
        {
            now = (uint32_t)time(NULL);
            for (i=0; i<numnodes; i++)
            {
                expand_ipbits(ipaddr,nodes[i]->ipbits);
                printf("(%llu %d %s) ",(long long)nodes[i]->nxt64bits,nodes[i]->lastcontact-now,ipaddr);
                if ( gen_pingstr(_cmd,1) > 0 )
                {
                    len = construct_tokenized_req((char *)finalbuf,_cmd,cp->srvNXTACCTSECRET);
                    send_packet(nodes[i],0,finalbuf,len);
                    pserver = get_pserver(0,ipaddr,0,0);
                    send_kademlia_cmd(0,pserver,"ping",cp->srvNXTACCTSECRET,0,0);
                    p2p_publishpacket(pserver,0);
                }
                free(nodes[i]);
            }
            free(nodes);
        }
        printf("numnodes.%d\n",numnodes);
    }
    if ( (broadcast_count % 10) == 0 )
    {
        for (i=n=0; i<Num_in_whitelist; i++)
        {
            expand_ipbits(ipaddr,SuperNET_whitelist[i]);
            pserver = get_pserver(0,ipaddr,0,0);
            if ( ismyipaddr(ipaddr) == 0 && ((stats= get_nodestats(pserver->nxt64bits)) == 0 || broadcast_count == 0 || (now - stats->lastcontact) > NODESTATS_EXPIRATION) )
                send_kademlia_cmd(0,pserver,"ping",cp->srvNXTACCTSECRET,0,0), n++;
        }
        if ( Debuglevel > 0 )
            printf("PINGED.%d\n",n);
    }
    broadcast_count++;
}

void SuperNET_idler(uv_idle_t *handle)
{
    static int counter;
    static double lastattempt,lastclock;
    double millis;
    struct udp_queuecmd *qp;
    struct write_req_t *wr,*firstwr = 0;
    int32_t r;
    char *jsonstr,*retstr,**ptrs;
    if ( Finished_init == 0 )
        return;
    millis = ((double)uv_hrtime() / 1000000);
    if ( millis > (lastattempt + 10) )
    {
        lastattempt = millis;
        r = ((rand() >> 8) % 2);
        while ( (wr= queue_dequeue(&sendQ)) != 0 )
        {
            if ( wr == firstwr )
            {
                queue_enqueue(&sendQ,wr);
                //printf("reached firstwr.%p\n",firstwr);
                break;
            }
            if ( (wr->queuetime % 2) == r )
            {
                process_sendQ_item(wr);
                // free(wr); libuv does this
                break;
            }
            if ( firstwr == 0 )
                firstwr = wr;
            queue_enqueue(&sendQ,wr);
        }
        if ( (qp= queue_dequeue(&udp_JSON)) != 0 )
        {
            //printf("process qp argjson.%p\n",qp->argjson);
            char previpaddr[64];
            expand_ipbits(previpaddr,qp->previpbits);
            jsonstr = SuperNET_json_commands(Global_mp,previpaddr,qp->argjson,qp->tokenized_np->H.U.NXTaddr,qp->valid,qp->decoded);
            //printf("free qp (%s) argjson.%p\n",jsonstr,qp->argjson);
            if ( jsonstr != 0 )
                free(jsonstr);
            free(qp->decoded);
            free_json(qp->argjson);
            free(qp);
        }
        else if ( (ptrs= queue_dequeue(&JSON_Q)) != 0 )
        {
            char *call_SuperNET_JSON(char *JSONstr);
            jsonstr = ptrs[0];
            if ( Debuglevel > 2 )
                printf("dequeue JSON_Q.(%s)\n",jsonstr);
            if ( (retstr= call_SuperNET_JSON(jsonstr)) == 0 )
                retstr = clonestr("{\"result\":null}");
            ptrs[1] = retstr;
            if ( ptrs[2] != 0 )
                queue_GUIpoll(ptrs);
        }
        if ( process_storageQ() != 0 )
        {
            //printf("processed storage\n");
        }
    }
    if ( millis > (lastclock + 1000) )
    {
        poll_pricedbs();
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
            update_Allnodes();
            poll_telepods("BTCD");
            poll_telepods("BTC");
        }
        counter++;
        lastclock = millis;
    }
    usleep(APISLEEP * 1000);
}

void run_UVloop(void *arg)
{
    uv_idle_t idler;
    uv_idle_init(UV_loop,&idler);
    //uv_async_init(UV_loop,&Tasks_async,async_handler);
    uv_idle_start(&idler,SuperNET_idler);
    uv_run(UV_loop,UV_RUN_DEFAULT);
    printf("end of uv_run\n");
}

void run_libwebsockets(void *arg)
{
    int32_t usessl = *(int32_t *)arg;
    init_API_port(usessl,APIPORT-!usessl,APISLEEP);
}

void init_NXThashtables(struct NXThandler_info *mp)
{
    struct NXT_acct *np = 0;
    struct NXT_asset *ap = 0;
    struct NXT_assettxid *tp = 0;
    struct NXT_guid *gp = 0;
    struct pserver_info *pp = 0;
    struct telepathy_entry *tel = 0;
    //struct kademlia_storage *sp = 0;
    static struct hashtable *NXTasset_txids,*NXTaddrs,*NXTassets,*NXTguids,*Pserver,*Telepathy_hash;
    if ( NXTguids == 0 )
        NXTguids = hashtable_create("NXTguids",HASHTABLES_STARTSIZE,sizeof(struct NXT_guid),((long)&gp->guid[0] - (long)gp),sizeof(gp->guid),((long)&gp->H.modified - (long)gp));
    if ( NXTasset_txids == 0 )
        NXTasset_txids = hashtable_create("NXTasset_txids",HASHTABLES_STARTSIZE,sizeof(struct NXT_assettxid),((long)&tp->H.U.txid[0] - (long)tp),sizeof(tp->H.U.txid),((long)&tp->H.modified - (long)tp));
    if ( NXTassets == 0 )
        NXTassets = hashtable_create("NXTassets",HASHTABLES_STARTSIZE,sizeof(struct NXT_asset),((long)&ap->H.U.assetid[0] - (long)ap),sizeof(ap->H.U.assetid),((long)&ap->H.modified - (long)ap));
    if ( NXTaddrs == 0 )
        NXTaddrs = hashtable_create("NXTaddrs",HASHTABLES_STARTSIZE,sizeof(struct NXT_acct),((long)&np->H.U.NXTaddr[0] - (long)np),sizeof(np->H.U.NXTaddr),((long)&np->H.modified - (long)np));
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
        mp->NXTassets_tablep = &NXTassets;
        mp->NXTasset_txids_tablep = &NXTasset_txids;
        printf("init_NXThashtables: %p %p %p %p\n",NXTguids,NXTaddrs,NXTassets,NXTasset_txids);
    }
}

char *init_NXTservices(char *JSON_or_fname,char *myipaddr)
{
    static int32_t zero,one = 1;
    struct NXThandler_info *mp = Global_mp;    // seems safest place to have main data structure
    printf("init_NXTservices.(%s)\n",myipaddr);
    UV_loop = uv_default_loop();
    portable_mutex_init(&mp->hash_mutex);
    portable_mutex_init(&mp->hashtable_queue[0].mutex);
    portable_mutex_init(&mp->hashtable_queue[1].mutex);
    
    init_NXThashtables(mp);
    mp->upollseconds = 333333 * 0;
    mp->pollseconds = POLL_SECONDS;
    if ( portable_thread_create((void *)process_hashtablequeues,mp) == 0 )
        printf("ERROR hist process_hashtablequeues\n");
    mp->udp = start_libuv_udpserver(4,SUPERNET_PORT,(void *)on_udprecv);
    myipaddr = init_MGWconf(JSON_or_fname,myipaddr);
    if ( myipaddr != 0 )
        strcpy(mp->ipaddr,myipaddr);
    Finished_loading = 1;
    printf("run_UVloop\n");
    if ( portable_thread_create((void *)run_UVloop,Global_mp) == 0 )
        printf("ERROR hist process_hashtablequeues\n");
    if ( portable_thread_create((void *)run_libwebsockets,&one) == 0 )
        printf("ERROR hist run_libwebsockets SSL\n");
    while ( SSL_done == 0 )
        usleep(100000);
    if ( portable_thread_create((void *)run_libwebsockets,&zero) == 0 )
        printf("ERROR hist run_libwebsockets\n");
    sleep(3);
    {
        struct coin_info *cp;
        struct nodestats *stats;
        struct pserver_info *pserver;
        while ( (cp= get_coin_info("BTCD")) == 0 )
            sleep(1);
        stats = get_nodestats(cp->srvpubnxtbits);
        stats->p2pport = parse_ipaddr(cp->myipaddr,myipaddr);
        stats->ipbits = calc_ipbits(cp->myipaddr);
        pserver = get_pserver(0,myipaddr,0,0);
        pserver->nxt64bits = cp->srvpubnxtbits;
        init_Contacts();
    }
    return(myipaddr);
}

char *call_SuperNET_JSON(char *JSONstr)
{
    cJSON *json,*array;
    int32_t valid;
    char NXTaddr[64],_tokbuf[2*MAX_JSON_FIELD],encoded[NXT_TOKEN_LEN+1],*cmdstr,*retstr = 0;
    struct coin_info *cp = get_coin_info("BTCD");
    if ( Finished_init == 0 )
    {
        printf("Finished_init still 0\n");
        return(clonestr("{\"result\":null}"));
    }
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
            //printf("made tokbuf.(%s)\n",_tokbuf);
            array = cJSON_Parse(_tokbuf);
            if ( array != 0 )
            {
                cmdstr = verify_tokenized_json(0,NXTaddr,&valid,array);
                //printf("cmdstr.%s valid.%d\n",cmdstr,valid);
                retstr = SuperNET_json_commands(Global_mp,0,array,NXTaddr,valid,_tokbuf);
                //printf("json command return.(%s)\n",retstr);
                if ( cmdstr != 0 )
                    free(cmdstr);
                free_json(array);
            } else printf("couldnt parse tokbuf.(%s)\n",_tokbuf);
        }
        free_json(json);
    } else printf("couldnt parse (%s)\n",JSONstr);
    if ( retstr == 0 )
        retstr = clonestr("{\"result\":null}");
    return(retstr);
}

char *block_on_SuperNET(int32_t blockflag,char *JSONstr)
{
    char **ptrs,*retstr,retbuf[1024];
    uint64_t txid = 0;
    ptrs = calloc(3,sizeof(*ptrs));
    ptrs[0] = clonestr(JSONstr);
    if ( blockflag == 0 )
    {
        txid = calc_txid((uint8_t *)JSONstr,(int32_t)strlen(JSONstr));
        ptrs[2] = (char *)txid;
    }
    if ( Debuglevel > 2 )
        printf("block.%d QUEUE.(%s)\n",blockflag,JSONstr);
    queue_enqueue(&JSON_Q,ptrs);
    if ( blockflag != 0 )
    {
        while ( (retstr= ptrs[1]) == 0 )
            usleep(1000);
        if ( ptrs[0] != 0 )
            free(ptrs[0]);
        free(ptrs);
        //printf("block.%d returned.(%s)\n",blockflag,retstr);
        return(retstr);
    }
    else
    {
        sprintf(retbuf,"{\"result\":\"pending SuperNET API call\",\"txid\":\"%llu\"}",(long long)txid);
        //printf("queue.%d returned.(%s)\n",blockflag,retbuf);
        return(clonestr(retbuf));
    }
}

char *oldblock_on_SuperNET(int32_t blockflag,char *JSONstr)
{
    char **ptrs,*retstr;
    ptrs = calloc(2,sizeof(*ptrs));
    ptrs[0] = clonestr(JSONstr);
    //printf("QUEUE.(%s)\n",JSONstr);
    queue_enqueue(&JSON_Q,ptrs);
    if ( blockflag != 0 )
    {
        while ( ptrs[1] == 0 )
            usleep(1000);
    } else ptrs[1] = clonestr("{\"result\":\"pending SuperNET API call\"}");
    retstr = ptrs[1];
    free(ptrs);
    //printf("block returned.(%s)\n",retstr);
    return(retstr);
}

char *SuperNET_JSON(char *JSONstr)
{
    char *retstr = 0;
    struct coin_info *cp = get_coin_info("BTCD");
    cJSON *json;
    if ( Finished_init == 0 )
        return(0);
    if ( Debuglevel > 1 )
        printf("got JSON.(%s)\n",JSONstr);
    if ( cp != 0 && (json= cJSON_Parse(JSONstr)) != 0 )
    {
        if ( 1 && is_BTCD_command(json) != 0 ) // deadlocks as the SuperNET API came from locked BTCD RPC
        {
            //if ( Debuglevel > 1 )
            //    printf("is_BTCD_command\n");
            return(block_on_SuperNET(0,JSONstr));
        }
        else retstr = block_on_SuperNET(1,JSONstr);
        free_json(json);
    } else printf("couldnt parse (%s)\n",JSONstr);
    if ( retstr == 0 )
        retstr = clonestr("{\"result\":null}");
    return(retstr);
}

uint64_t call_SuperNET_broadcast(struct pserver_info *pserver,char *msg,int32_t len,int32_t duration)
{
    int32_t SuperNET_broadcast(char *msg,int32_t duration);
    int32_t SuperNET_narrowcast(char *destip,unsigned char *msg,int32_t len);
    char ip_port[64],*ptr;
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
        //fprintf(stderr,"port.%d\n",port);
        sprintf(ip_port,"%s:%d",pserver->ipaddr,port);
        txid ^= calc_ipbits(pserver->ipaddr);
        if ( Debuglevel > 1 )
            fprintf(stderr,"%s NARROWCAST.(%s) txid.%llu (%s)\n",pserver->ipaddr,msg,(long long)txid,ip_port);
        ptr = calloc(1,64 + sizeof(len) + len + 1);
        memcpy(ptr,&len,sizeof(len));
        memcpy(&ptr[sizeof(len)],ip_port,strlen(ip_port));
        memcpy(&ptr[sizeof(len) + 64],msg,len);
        queue_enqueue(&NarrowQ,ptr);
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
                printf("BROADCAST parms.(%s) valid.%d duration.%d txid.%llu len.%d\n",msg,valid,duration,(long long)txid,len);
            ptr = calloc(1,sizeof(len) + sizeof(duration) + len);
            memcpy(ptr,&len,sizeof(len));
            memcpy(&ptr[sizeof(len)],&duration,sizeof(duration));
            memcpy(&ptr[sizeof(len) + sizeof(duration)],msg,len);
            ptr[sizeof(len) + sizeof(duration) + len] = 0;
            queue_enqueue(&BroadcastQ,ptr);
            return(txid);
        } else printf("cant broadcast non-JSON.(%s)\n",msg);
    }
    return(txid);
}

char *SuperNET_gotpacket(char *msg,int32_t duration,char *ip_port)
{
    static int flood,duplicates;
    cJSON *json;
    uint16_t p2pport;
    struct pserver_info *pserver;
    uint64_t txid;
    struct sockaddr prevaddr;
    int32_t len,createdflag,valid;
    unsigned char packet[2*MAX_JSON_FIELD];
    char ipaddr[64],txidstr[64],retjsonstr[2*MAX_JSON_FIELD],verifiedNXTaddr[64],*cmdstr,*retstr;
    if ( Debuglevel > 2 )
        printf("gotpacket.(%s) duration.%d from (%s)\n",msg,duration,ip_port);
    strcpy(retjsonstr,"{\"result\":null}");
    if ( Finished_loading == 0 )
    {
        if ( is_hexstr(msg) == 0 )
        {
            //printf("QUEUE.(%s)\n",msg);
            return(block_on_SuperNET(0,msg));
        }
        return(clonestr(retjsonstr));
    }
    p2pport = parse_ipaddr(ipaddr,ip_port);
    uv_ip4_addr(ipaddr,0,(struct sockaddr_in *)&prevaddr);
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
        /*else if ( (obookid= is_orderbook_tx(packet,len)) != 0 )
        {
            if ( update_orderbook_tx(1,obookid,(struct orderbook_tx *)packet,txid) == 0 )
            {
                ((struct orderbook_tx *)packet)->txid = txid;
                sprintf(retjsonstr,"{\"result\":\"SuperNET_gotpacket got obbokid.%llu packet txid.%llu\"}",(long long)obookid,(long long)txid);
            } else sprintf(retjsonstr,"{\"result\":\"SuperNET_gotpacket error updating obookid.%llu\"}",(long long)obookid);
        }*/ else sprintf(retjsonstr,"{\"error\":\"SuperNET_gotpacket cant find obookid\"}");
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
        if ( Debuglevel > 2 )
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
            retstr = SuperNET_json_commands(Global_mp,ipaddr,json,verifiedNXTaddr,valid,(char *)msg);
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
    //""
    //myipaddr = clonestr("[2607:5300:100:200::b1d]:14631");
    //myipaddr = clonestr("[2001:16d8:dd24:0:86c9:681e:f931:256]");
    if ( myipaddr[0] == '[' )
    {
        myipaddr = clonestr(conv_ipv6(myipaddr));
        //myipaddr[strlen(myipaddr)-1] = 0;
    }
    if ( myipaddr != 0 )
        myipaddr = clonestr(myipaddr);
    printf("SuperNET_start(%s) %p ipaddr.(%s)\n",JSON_or_fname,myipaddr,myipaddr);
    //getchar();
    if ( JSON_or_fname != 0 && JSON_or_fname[0] != '{' )
    {
        fp = fopen(JSON_or_fname,"rb");
        if ( fp == 0 )
            return(-1);
        fclose(fp);
    }
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
    printf("call init_NXTservices (%s)\n",myipaddr);
    myipaddr = init_NXTservices(JSON_or_fname,myipaddr);
    printf("back from init_NXTservices (%s)\n",myipaddr);
    p2p_publishpacket(0,0);
    if ( (cp= get_coin_info("BTCD")) == 0 || cp->srvNXTACCTSECRET[0] == 0 || cp->srvNXTADDR[0] == 0 )
    {
        fprintf(stderr,"need to have BTCD active and also srvpubaddr\n");
        exit(-1);
    }
    Historical_done = 1;
    Finished_init = 1;
    printf("add myhandle\n");
    addcontact(Global_mp->myhandle,cp->privateNXTADDR);
    printf("add mypublic\n");
    addcontact("mypublic",cp->srvNXTADDR);
    printf("finished addcontact\n");
    return(0);
}

