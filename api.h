//
//  api.h
//
//  Created by jl777 on 7/6/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef API_H
#define API_H
//#include "Python.h"
#ifndef _WIN32
#include "includes/libwebsockets.h"
#else
#include <libwebsockets.h>
#endif
#define MAX_LEN 40
#include "plugins/plugins.h"

int32_t is_BTCD_command(cJSON *json)
{
    // RPC.({"requestType":"BTCDjson","json":"{\"requestType\":\"telepodacct\"}"}) wsi.0x7f3650035cc0 user.0x7f3650037920
    char *BTCDcmds[] = { "maketelepods", "teleport", "telepodacct", "MGW", "startxfer" };//, "rampyramid", "ramstring", "ramblock" };
    char request[MAX_JSON_FIELD],jsonstr[MAX_JSON_FIELD];
    long i,iter;
    cJSON *json2 = 0;
    if ( extract_cJSON_str(request,sizeof(request),json,"requestType") > 0 )
    {
        for (iter=0; iter<2; iter++)
        {
            for (i=0; i<(sizeof(BTCDcmds)/sizeof(*BTCDcmds)); i++)
            {
                //printf("(%s vs %s) ",request,BTCDcmds[i]);
                if ( strcmp(request,BTCDcmds[i]) == 0 )
                {
                    //printf("%s is BTCD command\n",request);
                    return(1);
                }
            }
            if ( iter == 0 )
            {
                if ( (json= cJSON_GetObjectItem(json,"json")) != 0 )
                {
                    copy_cJSON(jsonstr,json);
                    unstringify(jsonstr);
                    if ( (json2= cJSON_Parse(jsonstr)) != 0 )
                    {
                        if ( extract_cJSON_str(request,sizeof(request),json2,"requestType") <= 0 )
                            break;
                    }
                } else break;
            } else if ( json2 != 0 ) free_json(json2);
        }
    }
    //printf("not BTCD command requestType.(%s)\n",request);
    return(0);
}

char *BTCDpoll_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    static int counter;
    int32_t duration,len;
    char ip_port[64],hexstr[8192],msg[MAX_JSON_FIELD],retbuf[MAX_JSON_FIELD*3],*ptr,*str,*msg2;
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    counter++;
    //printf("BTCDpoll.%d\n",counter);
    //BTCDpoll post_process_bitcoind_RPC.SuperNET can't parse.({"msg":"[{"requestType":"ping","NXT":"13434315136155299987","time":1414310974,"pubkey":"34b173939544eb01515119b5e0b05880eadaae3d268439c9cc1471d8681ecb6d","ipaddr":"209.126.70.159"},{"token":"im9n7c9ka58g3qq4b2oe1d8p7mndlqk0pj4jj1163pkdgs8knb0vsreb0kf6luo1bbk097buojs1k5o5c0ldn6r6aueioj8stgel1221fq40f0cvaqq0bciuniit0isi0dikd363f3bjd9ov24iltirp6h4eua0q"}]","duration":86400})
    retbuf[0] = 0;
    if ( (ptr= queue_dequeue(&BroadcastQ,1)) != 0 )
    {
        if ( Debuglevel > 1 )
            printf("Got BroadcastQ\n");
        memcpy(&len,ptr,sizeof(len));
        str = &ptr[sizeof(len) + sizeof(duration)];
        if ( len == (strlen(str) + 1) )
        {
            memcpy(&duration,&ptr[sizeof(len)],sizeof(duration));
            memcpy(msg,str,len);
            ptr[sizeof(len) + sizeof(duration) + len] = 0;
            msg2 = stringifyM(msg);
            sprintf(retbuf,"{\"msg\":%s,\"duration\":%d}",msg2,duration);
            free(msg2);
            //printf("send back broadcast.(%s)\n",retbuf);
        } else printf("BTCDpoll BroadcastQ len mismatch %d != %ld (%s)\n",len,strlen(str)+1,str);
        free_queueitem(ptr);
    }
    if ( retbuf[0] == 0 )
    {
        if ( (ptr= queue_dequeue(&NarrowQ,1)) != 0 )
        {
            if ( Debuglevel > 1 )
printf("Got NarrowQ\n");
            memcpy(&len,ptr,sizeof(len));
            if ( len < 4096 && len > 0 )
            {
                memcpy(ip_port,&ptr[sizeof(len)],64);
                memcpy(msg,&ptr[sizeof(len) + 64],len);
                init_hexbytes_noT(hexstr,(unsigned char *)msg,len);
                sprintf(retbuf,"{\"ip_port\":\"%s\",\"hex\":\"%s\",\"len\":%d}",ip_port,hexstr,len);
                //printf("send back narrow.(%s)\n",retbuf);
            } else printf("BTCDpoll NarrowQ illegal len.%d\n",len);
            free_queueitem(ptr);
        }
    }
    if ( retbuf[0] == 0 )
        strcpy(retbuf,"{\"result\":\"nothing pending\"}");
    return(clonestr(retbuf));
}

void queue_GUIpoll(struct resultsitem *_rp)
{
    uint16_t port;
    uint64_t txid;
    struct resultsitem *rp;
    char *str,*args,ipaddr[64];
    args = stringifyM(_rp->argstr);
    str = stringifyM(_rp->retstr);
    //retbuf = malloc(strlen(str) + strlen(args) + 256);
   //memcpy(retbuf,&ptrs,sizeof(ptrs));
    rp = calloc(1,strlen(str) + strlen(args) + 128);
    *rp = *_rp;
    txid = rp->txid;
    if ( ((txid >> 48) & 0xffff) == 0 )
    {
        port = (txid >> 32) & 0xffff;
        expand_ipbits(ipaddr,(uint32_t)txid);
        sprintf(rp->retbuf,"{\"result\":%s,\"from\":\"%s\",\"port\":%d,\"args\":%s}",str,ipaddr,port,args);
    }
    else sprintf(rp->retbuf,"{\"result\":%s,\"txid\":\"%llu\"}",str,(long long)txid);
    free(str); free(args);
    if ( Debuglevel > 1 )
        printf("QUEUED for GUI: (%s) -> (%s) ptrs %p %p\n",rp->argstr,rp->retbuf,rp->argstr,rp->retstr);
    queue_enqueue("resultsQ",&ResultsQ,&rp->DL);
}

char *GUIpoll_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    static int counter;
    struct resultsitem *rp;
    char retbuf[MAX_JSON_FIELD*3];
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    counter++;
    retbuf[0] = 0;
    if ( retbuf[0] == 0 )
    {
        if ( (rp= queue_dequeue(&ResultsQ,0)) != 0 )
        {
            if ( Debuglevel > 1 )
                fprintf(stderr,"Got GUI ResultsQ.(%s) ptrs.%p %p %p\n",rp->retbuf,rp,rp->argstr,rp->retstr);
            if ( rp->argstr != 0 )
                free(rp->argstr);
            if ( rp->retstr != 0 )
                free(rp->retstr);
            strcpy(retbuf,rp->retbuf);
            free(rp);
        }
    }
    if ( retbuf[0] == 0 )
        strcpy(retbuf,"{\"result\":\"nothing pending\"}");
    return(clonestr(retbuf));
}

char *sendmsg_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    static int counter;
    char nexthopNXTaddr[64],destNXTaddr[64],msg[MAX_JSON_FIELD],*retstr = 0;
    int32_t L,len;
    copy_cJSON(destNXTaddr,objs[0]);
    copy_cJSON(msg,objs[1]);
    if ( is_remote_access(previpaddr) != 0 )
    {
        printf("GOT MESSAGE.(%s) from %s\n",msg,sender);
        return(0);
    }
    L = (int32_t)get_API_int(objs[2],Global_mp->Lfactor);
    nexthopNXTaddr[0] = 0;
    //printf("sendmsg_func sender.(%s) valid.%d dest.(%s) (%s)\n",sender,valid,destNXTaddr,origargstr);
    if ( sender[0] != 0 && valid > 0 && destNXTaddr[0] != 0 )
    {
        if ( is_remote_access(previpaddr) != 0 )
        {
            //port = extract_nameport(previp,sizeof(previp),(struct sockaddr_in *)prevaddr);
            fprintf(stderr,"%d >>>>>>>>>>>>> received message.(%s) NXT.%s from hop.%s\n",counter,msg,sender,previpaddr);
            counter++;
            //retstr = clonestr("{\"result\":\"received message\"}");
        }
        else
        {
            len = (int32_t)strlen(origargstr)+1;
            stripwhite_ns(origargstr,len);
            len = (int32_t)strlen(origargstr)+1;
            retstr = sendmessage(!prevent_queueing("sendmsg"),nexthopNXTaddr,L,sender,origargstr,len,destNXTaddr,0,0);
        }
    }
    //if ( retstr == 0 )
    //    retstr = clonestr("{\"error\":\"invalid sendmessage request\"}");
    return(retstr);
}

char *sendbinary_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    static int counter;
    char nexthopNXTaddr[64],destNXTaddr[MAX_JSON_FIELD],cmdstr[MAX_JSON_FIELD],datastr[MAX_JSON_FIELD],*retstr = 0;
    int32_t L;
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    copy_cJSON(destNXTaddr,objs[0]);
    copy_cJSON(datastr,objs[1]);
    L = (int32_t)get_API_int(objs[2],Global_mp->Lfactor);
    nexthopNXTaddr[0] = 0;
    //printf("sendmsg_func sender.(%s) valid.%d dest.(%s) (%s)\n",sender,valid,destNXTaddr,origargstr);
    if ( sender[0] != 0 && valid > 0 && destNXTaddr[0] != 0 )
    {
        if ( is_remote_access(previpaddr) != 0 )
        {
            //port = extract_nameport(previp,sizeof(previp),(struct sockaddr_in *)prevaddr);
            fprintf(stderr,"%d >>>>>>>>>>>>> received binary message.(%s) NXT.%s from hop.%s\n",counter,datastr,sender,previpaddr);
            counter++;
            //retstr = clonestr("{\"result\":\"received message\"}");
        }
        else
        {
            sprintf(cmdstr,"{\"requestType\":\"sendbinary\",\"NXT\":\"%s\",\"data\":\"%s\",\"dest\":\"%s\"}",NXTaddr,datastr,destNXTaddr);
            retstr = send_tokenized_cmd(!prevent_queueing("sendbinary"),nexthopNXTaddr,L,NXTaddr,NXTACCTSECRET,cmdstr,destNXTaddr);
        }
    }
    //if ( retstr == 0 )
    //    retstr = clonestr("{\"error\":\"invalid sendmessage request\"}");
    return(retstr);
}

char *makeoffer3_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char *retstr = 0;
    printf("makeoffer3 remote.%d\n",is_remote_access(previpaddr));
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    if ( sender[0] != 0 && valid > 0 )
        retstr = call_makeoffer3(NXTaddr,NXTACCTSECRET,objs);
    else retstr = clonestr("{\"result\":\"invalid makeoffer3_func request\"}");
    return(retstr);
}

char *respondtx_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char cmdstr[MAX_JSON_FIELD],triggerhash[MAX_JSON_FIELD],utx[MAX_JSON_FIELD],sig[MAX_JSON_FIELD],*retstr = 0;
    uint64_t quoteid,assetid,qty,priceNQT,otherassetid,otherqty;
    int32_t minperc;
    printf("got respond_tx\n");
    //if ( is_remote_access(previpaddr) == 0 )
    //    return(0);
    copy_cJSON(cmdstr,objs[0]);
    assetid = get_API_nxt64bits(objs[1]);
    qty = get_API_nxt64bits(objs[2]);
    priceNQT = get_API_nxt64bits(objs[3]);
    copy_cJSON(triggerhash,objs[4]);
    quoteid = get_API_nxt64bits(objs[5]);
    copy_cJSON(sig,objs[6]);
    copy_cJSON(utx,objs[7]);
    minperc = (int32_t)get_API_int(objs[8],INSTANTDEX_MINVOL);
    if ( is_remote_access(previpaddr) == 0 )
        copy_cJSON(sender,objs[9]);
    otherassetid = get_API_nxt64bits(objs[10]);
    otherqty = get_API_nxt64bits(objs[11]);
    if ( sender[0] != 0 && valid > 0 && triggerhash[0] != 0 )
        retstr = respondtx(NXTaddr,NXTACCTSECRET,sender,cmdstr,assetid,qty,priceNQT,triggerhash,quoteid,sig,utx,minperc,otherassetid,otherqty);
    else retstr = clonestr("{\"result\":\"invalid respondtx_func request\"}");
    return(retstr);
}

char *tradebot_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    static char *buf;
    static int64_t filelen,allocsize;
    long len;
    cJSON *botjson;
    char code[MAX_JSON_FIELD],retbuf[4096],*str,*retstr = 0;
    if ( is_remote_access(previpaddr) != 0 )
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

char *teleport_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    double amount;
    char contactstr[MAX_JSON_FIELD],minage[MAX_JSON_FIELD],coinstr[MAX_JSON_FIELD],withdrawaddr[MAX_JSON_FIELD],*retstr = 0;
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    if ( Historical_done == 0 )
        return(clonestr("{\"error\":\"historical processing is not done yet\"}"));
    amount = get_API_float(objs[0]);
    copy_cJSON(contactstr,objs[1]);
    copy_cJSON(coinstr,objs[2]);
    copy_cJSON(minage,objs[3]);
    copy_cJSON(withdrawaddr,objs[4]);
    printf("amount.(%.8f) minage.(%s) %d\n",amount,minage,atoi(minage));
    if ( sender[0] != 0 && amount > 0 && valid > 0 && contactstr[0] != 0 )
        retstr = teleport(contactstr,coinstr,(uint64_t)(SATOSHIDEN * amount),atoi(minage),withdrawaddr);
    else retstr = clonestr("{\"error\":\"invalid teleport request\"}");
    return(retstr);
}

char *telepodacct_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    double amount;
    char contactstr[MAX_JSON_FIELD],coinstr[MAX_JSON_FIELD],withdrawaddr[MAX_JSON_FIELD],comment[MAX_JSON_FIELD],cmd[MAX_JSON_FIELD],*retstr = 0;
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    if ( Historical_done == 0 )
        return(clonestr("{\"error\":\"historical processing is not done yet\"}"));
    amount = get_API_float(objs[0]);
    copy_cJSON(contactstr,objs[1]);
    copy_cJSON(coinstr,objs[2]);
    copy_cJSON(comment,objs[3]);
    copy_cJSON(cmd,objs[4]);
    copy_cJSON(withdrawaddr,objs[5]);
    if ( sender[0] != 0 && valid > 0 )
        retstr = telepodacct(contactstr,coinstr,(uint64_t)(SATOSHIDEN * amount),withdrawaddr,comment,cmd);
    else retstr = clonestr("{\"error\":\"invalid telepodacct request\"}");
    return(retstr);
}

char *maketelepods_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    uint64_t value;
    char coinstr[MAX_JSON_FIELD],*retstr = 0;
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    value = (SATOSHIDEN * get_API_float(objs[0]));
    copy_cJSON(coinstr,objs[1]);
    //printf("maketelepods.%s %.8f\n",coinstr,dstr(value));
    if ( coinstr[0] != 0 && sender[0] != 0 && valid > 0 && value > 0 )
        retstr = maketelepods(NXTACCTSECRET,sender,coinstr,value);
    else retstr = clonestr("{\"error\":\"invalid maketelepods_func arguments\"}");
    return(retstr);
}

char *getpeers_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    cJSON *json;
    int32_t scanflag;
    char *jsonstr = 0;
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    scanflag = get_API_int(objs[0],0);
    json = gen_peers_json(previpaddr,NXTaddr,NXTACCTSECRET,sender,scanflag);
    if ( json != 0 )
    {
        jsonstr = cJSON_Print(json);
        free_json(json);
    }
    return(jsonstr);
}

char *findaddress_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char txidstr[MAX_JSON_FIELD],*retstr = 0;
    cJSON *array,*item;
    struct coin_info *cp = get_coin_info("BTCD");
    int32_t targetdist,numthreads,duration,i,n = 0;
    uint64_t refaddr,*txids = 0;
    if ( is_remote_access(previpaddr) != 0 )
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
        retstr = findaddress(previpaddr,NXTaddr,NXTACCTSECRET,sender,refaddr,txids,n,targetdist,duration,numthreads);
    else retstr = clonestr("{\"error\":\"invalid findaddress_func arguments\"}");
    //if ( txids != 0 ) freed on completion
    //    free(txids);
    return(retstr);
}

/*char *sendfile_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    FILE *fp;
    int32_t L;
    char fname[MAX_JSON_FIELD],dest[MAX_JSON_FIELD],*retstr = 0;
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    copy_cJSON(fname,objs[0]);
    copy_cJSON(dest,objs[1]);
    L = get_API_int(objs[2],0);
    fp = fopen(os_compatible_path(fname),"rb");
    if ( fp != 0 && sender[0] != 0 && valid > 0 )
        retstr = onion_sendfile(L,previpaddr,NXTaddr,NXTACCTSECRET,sender,dest,fp);
    else retstr = clonestr("{\"error\":\"invalid sendfile_func arguments\"}");
    if ( fp != 0 )
        fclose(fp);
    return(retstr);
}*/

char *ping_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    int32_t port,isMM;
    char pubkey[MAX_JSON_FIELD],destip[MAX_JSON_FIELD],ipaddr[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(pubkey,objs[0]);
    copy_cJSON(ipaddr,objs[1]);
    port = get_API_int(objs[2],0);
    copy_cJSON(destip,objs[3]);
    isMM = get_API_int(objs[4],0);
    //fprintf(stderr,"ping got sender.(%s) valid.%d pubkey.(%s) ipaddr.(%s) port.%d destip.(%s)\n",sender,valid,pubkey,ipaddr,port,destip);
    if ( sender[0] != 0 && valid > 0 )
        retstr = kademlia_ping(previpaddr,NXTaddr,NXTACCTSECRET,sender,ipaddr,port,destip,origargstr,isMM);
    else retstr = clonestr("{\"error\":\"invalid ping_func arguments\"}");
    return(retstr);
}

char *pong_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char pubkey[MAX_JSON_FIELD],tag[MAX_JSON_FIELD],ipaddr[MAX_JSON_FIELD],yourip[MAX_JSON_FIELD],*retstr = 0;
    uint16_t port,yourport,isMM;
    if ( is_remote_access(previpaddr) == 0 )
        return(0);
    copy_cJSON(pubkey,objs[0]);
    copy_cJSON(ipaddr,objs[1]);
    port = get_API_int(objs[2],0);
    copy_cJSON(yourip,objs[3]);
    yourport = get_API_int(objs[4],0);
    copy_cJSON(tag,objs[5]);
    isMM = get_API_int(objs[6],0);
    //printf("pong got pubkey.(%s) ipaddr.(%s) port.%d \n",pubkey,ipaddr,port);
    if ( sender[0] != 0 && valid > 0 )
    {
        retstr = kademlia_pong(previpaddr,NXTaddr,NXTACCTSECRET,sender,ipaddr,port,yourip,yourport,tag,isMM);
    }
    else retstr = clonestr("{\"error\":\"invalid pong_func arguments\"}");
    return(retstr);
}

char *addcontact_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char handle[MAX_JSON_FIELD],acct[MAX_JSON_FIELD],*retstr = 0;
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    copy_cJSON(handle,objs[0]);
    copy_cJSON(acct,objs[1]);
    printf("handle.(%s) acct.(%s) valid.%d\n",handle,acct,valid);
    if ( handle[0] != 0 && acct[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = addcontact(handle,acct);
    else retstr = clonestr("{\"error\":\"invalid addcontact_func arguments\"}");
    return(retstr);
}

char *removecontact_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char handle[MAX_JSON_FIELD],*retstr = 0;
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    copy_cJSON(handle,objs[0]);
    if ( handle[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = removecontact(previpaddr,NXTaddr,NXTACCTSECRET,sender,handle);
    else retstr = clonestr("{\"error\":\"invalid removecontact_func arguments\"}");
    return(retstr);
}

char *dispcontact_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char handle[MAX_JSON_FIELD],*retstr = 0;
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    copy_cJSON(handle,objs[0]);
    if ( handle[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = dispcontact(previpaddr,NXTaddr,NXTACCTSECRET,sender,handle);
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

char *findnode_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char pubkey[MAX_JSON_FIELD],key[MAX_JSON_FIELD],value[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(pubkey,objs[0]);
    set_kademlia_args(key,objs[1],objs[2]);
    copy_cJSON(value,objs[3]);
    if ( Debuglevel > 1 )
        printf("findnode.%s (%s) (%s) (%s) (%s)\n",previpaddr,sender,pubkey,key,value);
    if ( key[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = kademlia_find("findnode",previpaddr,NXTaddr,NXTACCTSECRET,sender,key,value,origargstr);
    else retstr = clonestr("{\"error\":\"invalid findnode_func arguments\"}");
    return(retstr);
}

char *findvalue_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char pubkey[MAX_JSON_FIELD],key[MAX_JSON_FIELD],value[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(pubkey,objs[0]);
    set_kademlia_args(key,objs[1],objs[2]);
    copy_cJSON(value,objs[3]);
    if ( Debuglevel > 1 )
        printf("findvalue.%s (%s) (%s) (%s)\n",previpaddr,sender,pubkey,key);
    if ( key[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = kademlia_find("findvalue",previpaddr,NXTaddr,NXTACCTSECRET,sender,key,value,origargstr);
    else retstr = clonestr("{\"error\":\"invalid findvalue_func arguments\"}");
    if ( Debuglevel > 1 )
        printf("back from findvalue\n");
    return(retstr);
}

char *havenode_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char pubkey[MAX_JSON_FIELD],key[MAX_JSON_FIELD],value[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(pubkey,objs[0]);
    set_kademlia_args(key,objs[1],objs[2]);
    copy_cJSON(value,objs[3]);
    if ( Debuglevel > 1 )
        printf("got HAVENODE.(%s) for key.(%s) from %s\n",value,key,sender);
    if ( key[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = kademlia_havenode(0,previpaddr,NXTaddr,NXTACCTSECRET,sender,key,value);
    else retstr = clonestr("{\"error\":\"invalid havenode_func arguments\"}");
    return(retstr);
}

char *havenodeB_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char pubkey[MAX_JSON_FIELD],key[MAX_JSON_FIELD],value[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(pubkey,objs[0]);
    set_kademlia_args(key,objs[1],objs[2]);
    copy_cJSON(value,objs[3]);
    if ( Debuglevel > 1 )
        printf("got HAVENODEB.(%s) for key.(%s) from %s\n",value,key,sender);
    if ( key[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = kademlia_havenode(1,previpaddr,NXTaddr,NXTACCTSECRET,sender,key,value);
    else retstr = clonestr("{\"error\":\"invalid havenodeB_func arguments\"}");
    return(retstr);
}

char *store_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char pubkey[MAX_JSON_FIELD],key[MAX_JSON_FIELD],datastr[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(pubkey,objs[0]);
    set_kademlia_args(key,objs[1],objs[2]);
    copy_cJSON(datastr,objs[3]);
    if ( key[0] != 0 && sender[0] != 0 && valid > 0 && datastr[0] != 0 )
    {
        retstr = kademlia_storedata(previpaddr,NXTaddr,NXTACCTSECRET,sender,key,datastr);
    }
    else retstr = clonestr("{\"error\":\"invalid store_func arguments\"}");
    return(retstr);
}

char *getdb_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char dirstr[MAX_JSON_FIELD],contact[MAX_JSON_FIELD],key[MAX_JSON_FIELD],destip[MAX_JSON_FIELD],*retstr = 0;
    int32_t sequenceid,dir;
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    copy_cJSON(contact,objs[0]);
    sequenceid = get_API_int(objs[1],0);
    copy_cJSON(key,objs[2]);
    copy_cJSON(dirstr,objs[3]);
    copy_cJSON(destip,objs[4]);
    if ( (contact[0] != 0 || key[0] != 0) && sender[0] != 0 && valid > 0 )
    {
        if ( strcmp(dirstr,"send") == 0 )
            dir = 1;
        else dir = -1;
        retstr = getdb(previpaddr,NXTaddr,NXTACCTSECRET,sender,dir,contact,sequenceid,key,destip);
    }
    else retstr = clonestr("{\"error\":\"invalid getdb_func arguments\"}");
    return(retstr);
}

char *cosign_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
#ifdef enablemsig
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
        init_hexbytes_noT(seedstr,seed.bytes,sizeof(seed));
    stats = get_nodestats(calc_nxt64bits(otheracctstr));
    if ( strlen(seedstr) == 64 && sender[0] != 0 && valid > 0 && stats != 0 && memcmp(stats->pubkey,zerokey,sizeof(stats->pubkey)) != 0 )
    {
        memcpy(priv.bytes,Global_mp->loopback_privkey,sizeof(priv));
        memcpy(pub.bytes,stats->pubkey,sizeof(pub));
        ret0 = curve25519(priv,pub);
        init_hexbytes_noT(ret0str,ret0.bytes,sizeof(ret0));
        ret = xor_keys(seed,ret0);
        init_hexbytes_noT(hexstr,ret.bytes,sizeof(ret));
        sprintf(retbuf,"{\"requestType\":\"cosigned\",\"seed\":\"%s\",\"result\":\"%s\",\"privacct\":\"%s\",\"pubacct\":\"%s\",\"ret0\":\"%s\"}",seedstr,ret0str,NXTaddr,otheracctstr,hexstr);
        return(clonestr(retbuf));
    }
#endif
    return(clonestr("{\"error\":\"invalid cosign_func arguments\"}"));
}

char *cosigned_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
#ifdef enablemsig
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
        init_hexbytes_noT(hexstr,ret.bytes,sizeof(ret));
        sprintf(retbuf,"{\"seed\":\"%s\",\"result\":\"%s\",\"acct\":\"%s\",\"privacct\":\"%llu\",\"pubacct\":\"%llu\",\"input\":\"%s\"}",seedstr,hexstr,NXTaddr,(long long)privacct,(long long)pubacct,resultstr);
        return(clonestr(retbuf));
    }
#endif
    return(clonestr("{\"error\":\"invalid cosigned_func arguments\"}"));
}

char *gotpacket_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char *SuperNET_gotpacket(char *msg,int32_t duration,char *ip_port);
    char msg[MAX_JSON_FIELD],ip_port[MAX_JSON_FIELD];
    int32_t duration;
    //printf("gotpacket from (%s) remote.%d\n",previpaddr,is_remote_access(previpaddr));
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    copy_cJSON(msg,objs[0]);
    unstringify(msg);
    duration = (int32_t)get_API_int(objs[1],600);
    copy_cJSON(ip_port,objs[2]);
    return(SuperNET_gotpacket(msg,duration,ip_port));
}

char *gotnewpeer_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char ip_port[MAX_JSON_FIELD];
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    copy_cJSON(ip_port,objs[0]);
    if ( ip_port[0] != 0 )
    {
        queue_enqueue("P2P_Q",&P2P_Q,queueitem(ip_port));
        return(clonestr("{\"result\":\"ip_port queued\"}"));
    }
    return(0);
}

char *lotto_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char refNXTaddr[MAX_JSON_FIELD],assetidstr[MAX_JSON_FIELD],lottoseed[MAX_JSON_FIELD],prize[MAX_JSON_FIELD];
    double prizefund = 0.;
    copy_cJSON(refNXTaddr,objs[0]);
    copy_cJSON(assetidstr,objs[1]);
    copy_cJSON(lottoseed,objs[2]);
    copy_cJSON(prize,objs[3]);
    if ( prize[0] != 0 )
        prizefund = atof(prize);
    if ( prizefund <= 0. )
        prizefund = 175000.;
    if ( refNXTaddr[0] != 0 && assetidstr[0] != 0 )
        return(update_lotto_transactions(refNXTaddr,assetidstr,lottoseed,prizefund));
    return(clonestr("{\"error\":\"illegal lotto parms\"}"));
}

char *stop_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{    
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    close_SuperNET_dbs();
    exit(0);
    return(clonestr("{\"result\":\"stopping SuperNET...\"}"));
}

char *gotjson_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char jsonstr[MAX_JSON_FIELD],*retstr = 0;
    cJSON *json;
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    copy_cJSON(jsonstr,objs[0]);
    if ( jsonstr[0] != 0 )
    {
        //if ( is_remote_access(previpaddr) != 0 )
        //    port = extract_nameport(ipaddr,sizeof(ipaddr),(struct sockaddr_in *)prevaddr);
        //else port = 0, strcpy(ipaddr,"noprevaddr");
        unstringify(jsonstr);
        printf("BTCDjson jsonstr.(%s) from (%s)\n",jsonstr,previpaddr);
        json = cJSON_Parse(jsonstr);
        if ( json != 0 )
        {
            retstr = SuperNET_json_commands(Global_mp,previpaddr,json,sender,valid,origargstr);
            free_json(json);
        } else printf("PARSE error.(%s)\n",jsonstr);
    }
    return(retstr);
}


int32_t compare_files(char *fname,char *fname2)
{
    int32_t offset,errs = 0;
    long len,len2;
    char buf[8192],buf2[8192];
    FILE *fp,*fp2;
    if ( (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
    {
        if ( (fp2= fopen(os_compatible_path(fname2),"rb")) != 0 )
        {
            while ( (len= fread(buf,1,sizeof(buf),fp)) > 0 && (len2= fread(buf2,1,sizeof(buf2),fp2)) == len )
                if ( (offset= memcmp(buf,buf2,len)) != 0 )
                    printf("compare error at offset.%d: (%s) src.%ld vs. (%s) dest.%ld\n",offset,fname,ftell(fp),fname2,ftell(fp2)), errs++;
            fclose(fp2);
        }
        fclose(fp);
    }
    return(errs);
}


long copy_file(char *src,char *dest)
{
    long len = -1;
    char buf[8192];
    FILE *srcfp,*destfp;
    if ( (srcfp= fopen(os_compatible_path(src),"rb")) != 0 )
    {
        if ( (destfp= fopen(os_compatible_path(dest),"wb")) != 0 )
        {
            while ( (len= fread(buf,1,sizeof(buf),srcfp)) > 0 )
                if ( (long)fwrite(buf,1,len,destfp) != len )
                    printf("write error at (%s) src.%ld vs. (%s) dest.%ld\n",src,ftell(srcfp),dest,ftell(destfp));
            len = ftell(destfp);
            fclose(destfp);
        }
        fclose(srcfp);
    }
    if ( len == 0 || compare_files(src,dest) != 0 )
        printf("Error copying files (%s) -> (%s)\n",src,dest), len = -1;
    return(len);
}

char *settings_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    static char *buf=0;
    static int64_t len=0,allocsize=0;
    char reinit[MAX_JSON_FIELD],field[MAX_JSON_FIELD],value[MAX_JSON_FIELD*2+1],decodedhex[MAX_JSON_FIELD*2],*str,*retstr;
    cJSON *json,*item;
    FILE *fp;
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    copy_cJSON(field,objs[0]);
    copy_cJSON(value,objs[1]);
    copy_cJSON(reinit,objs[2]);
    copy_file("SuperNET.conf.old","backups/SuperNET.conf.old");
    copy_file("SuperNET.conf","SuperNET.conf.old");
    retstr = load_file("SuperNET.conf",&buf,&len,&allocsize);
    if ( retstr != 0 )
    {
        //printf("cloning.(%s)\n",retstr);
        retstr = clonestr(retstr);
        if ( field[0] == 0 && value[0] == 0 )
            return(retstr);
    }
    if ( retstr != 0 )
    {
        fprintf(stderr,"settings: field.(%s) <- (%s)\n",field,value);
        json = cJSON_Parse(retstr);
        if ( json != 0 )
        {
            free(retstr);
            if ( field[0] != 0 )
            {
                printf("FIELD.(%s)\n",field);
                if ( value[0] == 0 )
                    cJSON_DeleteItemFromObject(json,field);
                else if ( (item= cJSON_GetObjectItem(json,field)) != 0 )
                    cJSON_ReplaceItemInObject(json,field,cJSON_CreateString(value));
                else cJSON_AddItemToObject(json,field,cJSON_CreateString(value));
                retstr = cJSON_Print(json);
            }
            else
            {
                if ( is_hexstr(value) != 0 )
                {
                    decode_hex((unsigned char *)decodedhex,(int32_t)strlen(value)/2,value);
                    retstr = clonestr(decodedhex);
                    printf("hex.(%s) -> (%s)\n",value,buf);
                }
                else
                {
                    unstringify(value);
                    printf("unstringify.(%s)\n",value);
                    retstr = clonestr(value);
                }
            }
            free_json(json);
            if ( (fp= fopen("SuperNET.conf","wb")) != 0 )
            {
                if ( fwrite(retstr,1,strlen(retstr),fp) != strlen(retstr) )
                    printf("error saving SuperNET.conf\n");
                fclose(fp);
            }
        }
        else
        {
            str = stringifyM(retstr);
            free(retstr);
            retstr = malloc(strlen(str) + 512);
            sprintf(retstr,"{\"error\":\"SuperNET.conf PARSE error\",\"settings\":%s}",str);
            free(str);
            str = 0;
        }
    } else printf("cant load SuperNET.conf\n");
    if ( retstr != 0 && strcmp(reinit,"yes") == 0 )
        init_MGWconf(retstr,0);
    return(retstr);
}

char *sendfrag_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char name[MAX_JSON_FIELD],dest[MAX_JSON_FIELD],datastr[MAX_JSON_FIELD],handler[MAX_JSON_FIELD];
    uint32_t fragi,numfrags,totalcrc,datacrc,totallen,blocksize,syncmem;
    //printf("sendfrag_func(%s)\n",origargstr);
    copy_cJSON(name,objs[1]);
    fragi = (uint32_t)get_API_int(objs[2],0);
    numfrags = (uint32_t)get_API_int(objs[3],0);
    copy_cJSON(dest,objs[4]);
    totalcrc = (uint32_t)get_API_int(objs[5],0);
    datacrc = (uint32_t)get_API_int(objs[6],0);
    copy_cJSON(datastr,objs[7]);
    totallen = (uint32_t)get_API_int(objs[8],0);
    blocksize = (uint32_t)get_API_int(objs[9],0);
    copy_cJSON(handler,objs[10]);
    syncmem = (uint32_t)get_API_int(objs[11],1);
    if ( name[0] != 0 && dest[0] != 0 && sender[0] != 0 && valid > 0 )
        return(sendfrag(previpaddr,sender,NXTaddr,NXTACCTSECRET,dest,name,fragi,numfrags,totallen,blocksize,totalcrc,datacrc,datastr,handler,syncmem));
    else printf("error sendfrag: name.(%s) dest.(%s) valid.%d sender.(%s) fragi.%d num.%d crc %u %u\n",name,dest,valid,sender,fragi,numfrags,totalcrc,datacrc);
    return(clonestr("{\"error\":\"bad sendfrag_func paramater\"}"));
}

char *gotfrag_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char name[MAX_JSON_FIELD],src[MAX_JSON_FIELD],handler[MAX_JSON_FIELD];
    uint32_t fragi,numfrags,totalcrc,datacrc,totallen,blocksize,count,syncmem,snapshotcrc;
    //printf("gotfrag_func(%s) is remote.%d\n",origargstr,is_remote_access(previpaddr));
    if ( is_remote_access(previpaddr) == 0 )
        return(0);
    copy_cJSON(name,objs[1]);
    fragi = (uint32_t)get_API_int(objs[2],0);
    numfrags = (uint32_t)get_API_int(objs[3],0);
    copy_cJSON(src,objs[4]);
    totalcrc = (uint32_t)get_API_int(objs[5],0);
    datacrc = (uint32_t)get_API_int(objs[6],0);
    totallen = (uint32_t)get_API_int(objs[7],0);
    blocksize = (uint32_t)get_API_int(objs[8],0);
    count = (uint32_t)get_API_int(objs[9],0);
    copy_cJSON(handler,objs[10]);
    syncmem = (uint32_t)get_API_int(objs[11],1);
    snapshotcrc = (uint32_t)get_API_int(objs[12],0);
    if ( name[0] != 0 && src[0] != 0 && sender[0] != 0 && valid > 0 )
        gotfrag(previpaddr,sender,NXTaddr,NXTACCTSECRET,src,name,fragi,numfrags,totallen,blocksize,totalcrc,datacrc,count,handler,syncmem,snapshotcrc);
    return(clonestr(origargstr));
}

char *startxfer_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char fname[MAX_JSON_FIELD],dest[MAX_JSON_FIELD],datastr[MAX_JSON_FIELD],handler[MAX_JSON_FIELD];
    int32_t syncmem,timeout,datalen = 0;
    uint8_t *data = 0;
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    copy_cJSON(fname,objs[0]);
    copy_cJSON(dest,objs[1]);
    copy_cJSON(datastr,objs[2]);
    timeout = (int32_t)get_API_int(objs[3],0);
    copy_cJSON(handler,objs[4]);
    syncmem = (int32_t)get_API_int(objs[5],1);
    printf("startxfer_func(%s) is remote.%d fname(%s) timeout.%d\n",origargstr,is_remote_access(previpaddr),fname,timeout);
    if ( (fname[0] != 0 || datastr[0] != 0) && dest[0] != 0 && sender[0] != 0 && valid > 0 )
    {
        if ( datastr[0] != 0 )
        {
            datalen = (int32_t)strlen(datastr) / 2;
            data = malloc(datalen);
            datalen = decode_hex(data,datalen,datastr);
        }
        return(start_transfer(previpaddr,sender,NXTaddr,NXTACCTSECRET,dest,fname,data,datalen,timeout,handler,syncmem));
    }
    return(clonestr(origargstr));
}

char *getfile_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    int32_t timeout = 60;
    char fname[MAX_JSON_FIELD],handler[MAX_JSON_FIELD],name[MAX_JSON_FIELD];
    if ( is_remote_access(previpaddr) == 0 )
        return(0);
    copy_cJSON(name,objs[0]);
    copy_cJSON(handler,objs[1]);
    if ( (fname[0] != 0 || handler[0] != 0) && sender[0] != 0 && valid > 0 )
    {
        set_handler_fname(fname,handler,name);
        if ( Global_mp->gatewayid >= 0 || Global_mp->iambridge != 0 )
            broadcastfile(NXTaddr,NXTACCTSECRET,fname);
        printf("getfile.(%s).(%s) (%s) -> (%s) (%s)\n",name,handler,fname,sender,previpaddr);
        return(start_transfer(previpaddr,sender,NXTaddr,NXTACCTSECRET,previpaddr,name,0,0,timeout,handler,0));
    }
    else return(clonestr("{\"error\":\"invalid getfile parameters\"}"));
}

#ifdef later
#define RAMAPI_ERRORSTR "{\"error\":\"invalid ramchain parameters\"}"
#define RAMAPI_ILLEGALREMOTE "{\"error\":\"invalid ramchain remote access\"}"
char *preprocess_ram_apiargs(char *coin,char *previpaddr,cJSON **objs,int32_t valid,char *origargstr,char *NXTaddr,char *NXTACCTSECRET)
{
    static bits256 zerokey;
    char hopNXTaddr[64],destNXTaddr[64],destip[MAX_JSON_FIELD],*str,*jsonstr,*retstr = 0;
    uint16_t port;
    cJSON *array,*json;
    int32_t createdflag;
    struct pserver_info *pserver;
    struct NXT_acct *destnp;
    copy_cJSON(destip,objs[0]);
    port = (uint16_t)get_API_int(objs[1],0);
    copy_cJSON(coin,objs[2]);
    if ( strcmp(Global_mp->ipaddr,destip) == 0 )
        return(0);
    //printf("myipaddr.(%s) process args (%s) (%s) port.%d\n",Global_mp->ipaddr,destip,coin,port);
    if ( coin[0] != 0 && destip[0] != 0 && valid > 0 )
    {
        if ( is_remote_access(previpaddr) == 0 )
        {
            pserver = get_pserver(0,destip,0,0);
            if ( pserver->nxt64bits != 0 )
            {
                expand_nxt64bits(destNXTaddr,pserver->nxt64bits);
                destnp = get_NXTacct(&createdflag,destNXTaddr);
                if ( (memcmp(destnp->stats.pubkey,&zerokey,sizeof(zerokey)) == 0 || port != 0) && destip[0] != 0 )
                {
                    //printf("send to ipaddr.(%s/%d)\n",destip,port);
                    send_to_ipaddr(port,0,destip,origargstr,NXTACCTSECRET);
                }
                else if ( (array= cJSON_Parse(origargstr)) != 0 )
                {
                    if ( is_cJSON_Array(array) != 0 && cJSON_GetArraySize(array) == 2 )
                    {
                        json = cJSON_GetArrayItem(array,0);
                        jsonstr = cJSON_Print(json);
                        stripwhite_ns(jsonstr,strlen(jsonstr));
                        //printf("send cmd.(%s)\n",jsonstr);
                        if ( (str = send_tokenized_cmd(!prevent_queueing("ramchain"),hopNXTaddr,0,NXTaddr,NXTACCTSECRET,jsonstr,destNXTaddr)) != 0 )
                            free(str);
                        free(jsonstr);
                    }
                    free_json(array);
                } else printf("preprocess_ram_apiargs: error parsing (%s)\n",origargstr);
                return(clonestr("{\"result\":\"sent request to destip\"}"));
            } // only path to continue sequence
        } else retstr = clonestr(RAMAPI_ERRORSTR);
    }
    if ( destip[0] != 0 )
    {
        retstr = clonestr("{\"error\":\"unvalidated path with destip\"}");
        destip[0] = 0;
    }
    return(retstr);
}

void ram_request(uint64_t nxt64bits,char *destip,struct ramchain_info *ram,char *jsonstr)
{
    static bits256 zerokey;
    char hopNXTaddr[64],destNXTaddr[64],ipaddr[64],*str;
    struct pserver_info *pserver;
    int32_t createdflag;
    struct NXT_acct *destnp;
    if ( nxt64bits == 0 )
    {
        pserver = get_pserver(0,destip,0,0);
        nxt64bits =  pserver->nxt64bits;
    }
    if ( nxt64bits != 0 && nxt64bits != Global_mp->nxt64bits )
    {
        expand_nxt64bits(destNXTaddr,nxt64bits);
        destnp = get_NXTacct(&createdflag,destNXTaddr);
        expand_ipbits(ipaddr,destnp->stats.ipbits);
        if ( 1 || memcmp(destnp->stats.pubkey,&zerokey,sizeof(zerokey)) == 0 )
        {
            //printf("send to ipaddr.(%s)\n",destip);
            send_to_ipaddr(0,1,ipaddr,jsonstr,ram->srvNXTACCTSECRET);
        }
        else if ( (str = send_tokenized_cmd(!prevent_queueing("ramchain"),hopNXTaddr,0,ram->srvNXTADDR,ram->srvNXTACCTSECRET,jsonstr,destNXTaddr)) != 0 )
            free(str);
    }
}

void ram_syncblocks(struct ramchain_info *ram,uint32_t blocknum,int32_t numblocks,uint64_t *sources,int32_t n,int32_t addshaflag)
{
    int32_t ram_perm_sha256(bits256 *hashp,struct ramchain_info *ram,uint32_t blocknum,int32_t n);
    char destip[64],jsonstr[MAX_JSON_FIELD],shastr[128],hashstr[65];
    int32_t i;
    cJSON *array;
    bits256 hash;
    if ( addshaflag != 0 && ram_perm_sha256(&hash,ram,blocknum,numblocks) == numblocks )
    {
        init_hexbytes_noT(hashstr,hash.bytes,sizeof(hash));
        sprintf(shastr,",\"mysha256\":\"%s\"",hashstr);
    }
    else shastr[0] = 0;
    sprintf(jsonstr,"{\"requestType\":\"rampyramid\",\"coin\":\"%s\",\"NXT\":\"%s\",\"blocknum\":%u,\"type\":\"B%d\"%s}",ram->name,ram->srvNXTADDR,blocknum,numblocks,shastr);
    if ( sources != 0 && n > 0 )
    {
        for (i=0; i<n; i++)
            if ( sources[i] != 0 )
                ram_request(sources[i],0,ram,jsonstr);
    }
    else
    {
        array = cJSON_GetObjectItem(MGWconf,"whitelist");
        if ( array != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
        {
            //printf("RAMSYNC.(%s)\n",jsonstr);
            for (i=0; i<n; i++)
            {
                copy_cJSON(destip,cJSON_GetArrayItem(array,i));
                ram_request(0,destip,ram,jsonstr);
            }
        }
    }
}

void ram_sendresponse(char *origcmd,char *coinstr,char *retstr,char *NXTaddr,char *NXTACCTSECRET,char *destNXTaddr,char *previpaddr)
{
    int32_t len,timeout = 300;
    cJSON *json;
    char fname[MAX_JSON_FIELD],hopNXTaddr[64],*jsonstr,*str = 0;
    if ( is_remote_access(previpaddr) != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            ensure_jsonitem(json,"coin",coinstr);
            ensure_jsonitem(json,"origcmd",origcmd);
            ensure_jsonitem(json,"requestType","ramresponse");
            jsonstr = cJSON_Print(json);
            stripwhite_ns(jsonstr,strlen(jsonstr));
            if ( (len= (int32_t)strlen(jsonstr)) < 1024 )
            {
                hopNXTaddr[0] = 0;
                str = send_tokenized_cmd(!prevent_queueing("ramchain"),hopNXTaddr,0,NXTaddr,NXTACCTSECRET,jsonstr,destNXTaddr);
            }
            else
            {
                sprintf(fname,"ramresponse.%s.%d",NXTaddr,rand());
                str = start_transfer(previpaddr,destNXTaddr,NXTaddr,NXTACCTSECRET,previpaddr,fname,(uint8_t *)jsonstr,len,timeout,"ramchain",0);
            }
            if ( str != 0 )
                free(str);
            free_json(json);
            free(jsonstr);
        }
    }
}

char *ramresponse_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    if ( is_remote_access(previpaddr) == 0 )
        return(0);
    if ( Debuglevel > 2 )
        fprintf(stderr,"ramresponse_func(%s)\n",origargstr);
    if ( sender[0] != 0 && valid > 0 )
        return(ramresponse(origargstr,sender,previpaddr,cJSON_str(objs[2])));
    else return(clonestr(RAMAPI_ERRORSTR));
}

char *ramstring_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],typestr[MAX_JSON_FIELD],*retstr = 0;
    uint32_t rawind = 0;
    if ( (retstr= preprocess_ram_apiargs(coin,previpaddr,objs,valid,origargstr,NXTaddr,NXTACCTSECRET)) != 0 )
        return(retstr);
    copy_cJSON(typestr,objs[3]);
    rawind = (uint32_t)get_API_int(objs[4],0);
    if ( get_ramchain_info(coin) != 0 && typestr[0] != 0 && sender[0] != 0 && valid > 0 )
    {
        retstr = ramstring(origargstr,sender,previpaddr,coin,typestr,rawind);
        ram_sendresponse(typestr,coin,retstr,NXTaddr,NXTACCTSECRET,sender,previpaddr);
    }
    else retstr = clonestr(RAMAPI_ERRORSTR);
    return(retstr);
}

char *ramrawind_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],typestr[MAX_JSON_FIELD],str[MAX_JSON_FIELD],*retstr = 0;
    if ( (retstr= preprocess_ram_apiargs(coin,previpaddr,objs,valid,origargstr,NXTaddr,NXTACCTSECRET)) != 0 )
        return(retstr);
    copy_cJSON(typestr,objs[3]);
    copy_cJSON(str,objs[4]);
    if ( get_ramchain_info(coin) != 0 && typestr[0] != 0 && str[0] != 0 && sender[0] != 0 && valid > 0 )
    {
        retstr = ramrawind(origargstr,sender,previpaddr,coin,typestr,str);
        ram_sendresponse(typestr,coin,retstr,NXTaddr,NXTACCTSECRET,sender,previpaddr);
    }
    else retstr = clonestr(RAMAPI_ERRORSTR);
    return(retstr);
}

char *ramstatus_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],*retstr = 0;
    if ( (retstr= preprocess_ram_apiargs(coin,previpaddr,objs,valid,origargstr,NXTaddr,NXTACCTSECRET)) != 0 )
        return(retstr);
    printf("after process args\n");
    if ( get_ramchain_info(coin) != 0 && sender[0] != 0 && valid > 0 )
    {
        printf("calling ramstatus\n");
        retstr = ramstatus(origargstr,sender,previpaddr,coin);
        ram_sendresponse("ramstatus",coin,retstr,NXTaddr,NXTACCTSECRET,sender,previpaddr);
    }
    else retstr = clonestr(RAMAPI_ERRORSTR);
    return(retstr);
}

char *rampyramid_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],typestr[MAX_JSON_FIELD],*retstr = 0;
    uint32_t blocknum;
    if ( (retstr= preprocess_ram_apiargs(coin,previpaddr,objs,valid,origargstr,NXTaddr,NXTACCTSECRET)) != 0 )
        return(retstr);
    blocknum = (uint32_t)get_API_int(objs[3],-1);
    copy_cJSON(typestr,objs[4]);
    if ( Debuglevel > 2 )
        printf("got pyramid: (%s)\n",origargstr);
    if ( get_ramchain_info(coin) != 0 && sender[0] != 0 && valid > 0 )
    {
        retstr = rampyramid(NXTaddr,origargstr,sender,previpaddr,coin,blocknum,typestr);
        ram_sendresponse("rampyramid",coin,retstr,NXTaddr,NXTACCTSECRET,sender,previpaddr);
    }
    else retstr = clonestr(RAMAPI_ERRORSTR);
    return(retstr);
}

char *ramscript_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],*txidstr,*retstr = 0;
    int32_t vout,tx_vout,blocknum,txind,validB = 0;
    struct address_entry B;
    memset(&B,0,sizeof(B));
    if ( (retstr= preprocess_ram_apiargs(coin,previpaddr,objs,valid,origargstr,NXTaddr,NXTACCTSECRET)) != 0 )
        return(retstr);
    txidstr = cJSON_str(objs[3]);
    tx_vout = (uint32_t)get_API_int(objs[4],-1);
    B.blocknum = blocknum = (uint32_t)get_API_int(objs[5],-1);
    B.txind = txind = (uint32_t)get_API_int(objs[6],-1);
    B.v = vout = (uint32_t)get_API_int(objs[7],-1);
    if ( blocknum >= 0 && txind >= 0 && vout >= 0 && B.blocknum == blocknum && B.txind == txind && B.v == vout )
        validB = 1;
    if ( get_ramchain_info(coin) != 0 && ((txidstr != 0 && tx_vout >= 0) || validB != 0) && sender[0] != 0 && valid > 0 )
    {
        retstr = ramscript(origargstr,sender,previpaddr,coin,txidstr,tx_vout,(validB != 0) ? &B : 0);
        ram_sendresponse("ramscript",coin,retstr,NXTaddr,NXTACCTSECRET,sender,previpaddr);
    }
    else retstr = clonestr(RAMAPI_ERRORSTR);
    return(retstr);
}

char *ramblock_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],*retstr = 0;
    uint32_t blocknum;
    if ( (retstr= preprocess_ram_apiargs(coin,previpaddr,objs,valid,origargstr,NXTaddr,NXTACCTSECRET)) != 0 )
        return(retstr);
    blocknum = (uint32_t)get_API_int(objs[3],0);
    if ( get_ramchain_info(coin) != 0 && sender[0] != 0 && valid > 0 )
    {
        retstr = ramblock(NXTaddr,origargstr,sender,previpaddr,coin,blocknum);
        ram_sendresponse("ramblock",coin,retstr,NXTaddr,NXTACCTSECRET,sender,previpaddr);
    }
    else retstr = clonestr(RAMAPI_ERRORSTR);
    return(retstr);
}

// local ramchain funcs
char *ramcompress_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],*ramhex,*retstr = 0;
    if ( is_remote_access(previpaddr) != 0 )
        return(clonestr(RAMAPI_ILLEGALREMOTE));
    copy_cJSON(coin,objs[0]);
    ramhex = cJSON_str(objs[1]);
    if ( coin[0] != 0 && ramhex != 0 && sender[0] != 0 && valid > 0 )
        retstr = ramcompress(origargstr,sender,previpaddr,coin,ramhex);
    else retstr = clonestr(RAMAPI_ERRORSTR);
    return(retstr);
}

char *ramexpand_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],*bitstream,*retstr = 0;
    if ( is_remote_access(previpaddr) != 0 )
        return(clonestr(RAMAPI_ILLEGALREMOTE));
    copy_cJSON(coin,objs[0]);
    bitstream = cJSON_str(objs[1]);
    if ( coin[0] != 0 && bitstream != 0 && sender[0] != 0 && valid > 0 )
        retstr = ramexpand(origargstr,sender,previpaddr,coin,bitstream);
    else retstr = clonestr(RAMAPI_ERRORSTR);
    return(retstr);
}

char *ramaddrlist_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],*retstr = 0;
    if ( is_remote_access(previpaddr) != 0 )
        return(clonestr(RAMAPI_ILLEGALREMOTE));
    copy_cJSON(coin,objs[0]);
    if ( coin[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = ramaddrlist(origargstr,sender,previpaddr,coin);
    else retstr = clonestr(RAMAPI_ERRORSTR);
    return(retstr);
}

char *ramtxlist_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],coinaddr[MAX_JSON_FIELD],*retstr = 0;
    int32_t unspentflag;
    if ( is_remote_access(previpaddr) != 0 )
        return(clonestr(RAMAPI_ILLEGALREMOTE));
    copy_cJSON(coin,objs[0]);
    copy_cJSON(coinaddr,objs[1]);
    unspentflag = (int32_t)get_API_int(objs[2],0);
    if ( coin[0] != 0 && coinaddr[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = ramtxlist(origargstr,sender,previpaddr,coin,coinaddr,unspentflag);
    else retstr = clonestr(RAMAPI_ERRORSTR);
    return(retstr);
}

char *ramrichlist_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],*retstr = 0;
    uint32_t num;
    int32_t recalcflag;
    if ( is_remote_access(previpaddr) != 0 )
        return(clonestr(RAMAPI_ILLEGALREMOTE));
    copy_cJSON(coin,objs[0]);
    num = (uint32_t)get_API_int(objs[1],0);
    recalcflag = (uint32_t)get_API_int(objs[2],1);
    if ( coin[0] != 0 && num != 0 && sender[0] != 0 && valid > 0 )
        retstr = ramrichlist(origargstr,sender,previpaddr,coin,num,recalcflag);
    else retstr = clonestr(RAMAPI_ERRORSTR);
    return(retstr);
}

double extract_rate(cJSON *array,char *base,char *rel)
{
    int32_t i,n;
    cJSON *item;
    double rate = 0.;
    char basestr[MAX_JSON_FIELD],relstr[MAX_JSON_FIELD];
    if ( is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
    {
        for (i=0; i<n; i++)
        {
            item = cJSON_GetArrayItem(array,i);
            copy_cJSON(basestr,cJSON_GetObjectItem(item,"base"));
            copy_cJSON(relstr,cJSON_GetObjectItem(item,"rel"));
            rate = get_API_float(cJSON_GetObjectItem(item,"rate"));
            if ( strcmp(base,basestr) == 0 && strcmp(rel,relstr) == 0 )
                return(rate);
            if ( strcmp(rel,basestr) == 0 && strcmp(base,relstr) == 0 && rate != 0. )
                return(1. / rate);
        }
    }
    return(0.);
}

char *rambalances_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],srccoin[MAX_JSON_FIELD],**coins = 0,**list,***coinaddrs = 0,*retstr = 0;
    double *rates = 0;
    cJSON *item,*subarray;
    uint32_t numcoins = 0,numaddrs,i,j;
    if ( is_remote_access(previpaddr) != 0 )
        return(clonestr(RAMAPI_ILLEGALREMOTE));
    copy_cJSON(coin,objs[0]);
    if ( is_cJSON_Array(objs[1]) != 0 && (numcoins= cJSON_GetArraySize(objs[1])) > 0 )
    {
        coins = calloc(numcoins,sizeof(*coins));
        rates = calloc(numcoins,sizeof(*rates));
        coinaddrs = calloc(numcoins,sizeof(*coinaddrs));
        for (i=0; i<numcoins; i++)
        {
            item = cJSON_GetArrayItem(objs[1],i);
            coins[i] = cJSON_str(cJSON_GetObjectItem(item,"coin"));
            subarray = cJSON_GetObjectItem(item,"addrs");
            if ( srccoin[0] != 0 && subarray != 0 && is_cJSON_Array(subarray) != 0 && (numaddrs= cJSON_GetArraySize(subarray)) > 0 )
            {
                list = calloc(numaddrs+1,sizeof(*list));
                for (j=0; j<numaddrs; j++)
                    list[j] = cJSON_str(cJSON_GetArrayItem(subarray,j));
                coinaddrs[i] = list;
                rates[i] = extract_rate(objs[2],srccoin,coin);
            }
        }
    }
    if ( coin[0] != 0 && numcoins != 0 && coins != 0 && coinaddrs != 0 && rates != 0 && sender[0] != 0 && valid > 0 )
        retstr = rambalances(origargstr,sender,previpaddr,coin,coins,rates,coinaddrs,numcoins);
    else retstr = clonestr(RAMAPI_ERRORSTR);
    if ( coins != 0 )
        free(coins);
    if ( coinaddrs != 0 )
    {
        for (i=0; i<numcoins; i++)
            if ( coinaddrs[i] != 0 )
                free(coinaddrs[i]);
        free(coinaddrs);
    }
    if ( rates != 0 )
        free(rates);
    return(retstr);
}

char *bridge_test(int32_t sendflag,char *NXTACCTSECRET,char *destip,uint16_t bridgeport,char *origargstr)
{
    struct coin_info *cp = get_coin_info("BTCD");
    char retbuf[1024],*str = "";
    retbuf[0] = 0;
    if ( strcmp(cp->myipaddr,destip) == 0 )
    {
        if ( (bridgeport= cp->bridgeport) != 0 && cp->bridgeipaddr[0] != 0 )
            strcpy(destip,cp->bridgeipaddr);
        else bridgeport = destip[0] = 0;
        //printf("my bridgetest (%s:%d)\n",destip,bridgeport);
    }
    if ( destip[0] != 0 )
    {
        if ( Debuglevel > 2 )
            printf("bridgetest.(%s) illegal.%d\n",destip,is_illegal_ipaddr(destip));
        if ( is_illegal_ipaddr(destip) == 0 )
        {
            if ( sendflag != 0 )
                send_to_ipaddr(bridgeport,0,destip,origargstr,NXTACCTSECRET);
            else str = "would have ";
            if ( bridgeport != 0 )
                sprintf(retbuf,"{\"result\":\"%sbridged\"}",str);
            else sprintf(retbuf,"{\"result\":\"%sforwarded\"}",str);
        } else sprintf(retbuf,"{\"error\":\"%sillegal destip\"}",str);
    }
    if ( retbuf[0] != 0 )
        return(clonestr(retbuf));
    else return(0);
}

char *genmultisig_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char refacct[MAX_JSON_FIELD],coin[MAX_JSON_FIELD],destip[MAX_JSON_FIELD],userpubkey[MAX_JSON_FIELD],email[MAX_JSON_FIELD],*retstr = 0;
    cJSON *json;
    uint16_t bridgeport;
    int32_t M,N,buyNXT,noerror,n = 0;
    struct multisig_addr *msig;
    struct contact_info **contacts = 0;
    copy_cJSON(userpubkey,objs[0]);
    copy_cJSON(coin,objs[1]);
    copy_cJSON(refacct,objs[2]);
    M = (int32_t)get_API_int(objs[3],1);
    N = (int32_t)get_API_int(objs[4],1);
    contacts = conv_contacts_json(&n,objs[5]);
    copy_cJSON(destip,objs[6]);
    bridgeport = (uint16_t)get_API_int(objs[7],0);
    copy_cJSON(email,objs[8]);
    buyNXT = (int32_t)get_API_int(objs[9],0);
    printf("genmultisig_func coin.(%s) (%s)\n",coin,origargstr);
    if ( coin[0] != 0 && refacct[0] != 0 && sender[0] != 0 && valid > 0 )
    {
        if ( (retstr= bridge_test(0,NXTACCTSECRET,destip,bridgeport,origargstr)) != 0 )
        {
            printf("sender.(%s) bridgetest returns.(%s)\n",sender,retstr);
            free(retstr);
            noerror = 0;
            if ( (msig= find_NXT_msig(1,sender,coin,contacts,n)) != 0 )
            {
                retstr = create_multisig_json(msig,0);
                printf("MSIGreturns.(%s)\n",retstr);
                if ( (json= cJSON_Parse(retstr)) != 0 )
                {
                    if ( cJSON_GetObjectItem(json,"error") == 0 )
                        noerror = 1;
                    free_json(json);
                }
                free(msig);
                if ( noerror != 0 )
                    return(retstr);
                else free(retstr);
            }
            return(bridge_test(1,NXTACCTSECRET,destip,bridgeport,origargstr));
        }
        retstr = genmultisig(NXTaddr,NXTACCTSECRET,previpaddr,coin,refacct,M,N,contacts,n,userpubkey,email,buyNXT);
    }
    free_contacts(contacts,n);
    if ( retstr != 0 )
        return(retstr);
    return(clonestr("{\"error\":\"bad genmultisig_func paramater\"}"));
}

void issue_genmultisig(char *coinstr,char *userNXTaddr,char *userpubkey,char *email,int32_t buyNXT)
{
    char *SuperNET_url();
    struct coin_info *refcp = get_coin_info("BTCD");
    char params[4096],*retstr;
    int32_t gatewayid;
    for (gatewayid=0; gatewayid<NUM_GATEWAYS; gatewayid++)
    {
        sprintf(params,"{\"requestType\":\"genmultisig\",\"email\":\"%s\",\"buyNXT\":%d,\"destip\":\"%s\",\"destport\":%d,\"refcontact\":\"%s\",\"userpubkey\":\"%s\",\"coin\":\"%s\",\"contacts\":[\"%s\", \"%s\", \"%s\"],\"M\":%d,\"N\":%d}",email,buyNXT,Server_ipaddrs[gatewayid],refcp->bridgeport,userNXTaddr,userpubkey,coinstr,Server_NXTaddrs[0],Server_NXTaddrs[1],Server_NXTaddrs[2],NUM_GATEWAYS-1,NUM_GATEWAYS);
        retstr = bitcoind_RPC(0,(char *)"BTCD",SuperNET_url(),(char *)"",(char *)"SuperNET",params);
        if ( Debuglevel > 0 )
            printf("issue.(%s) -> (%s)\n",params,retstr);
        if ( retstr != 0 )
            free(retstr);
    }
}

char *getmsigpubkey_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    struct coin_info *cp;
    char hopNXTaddr[64],refNXTaddr[MAX_JSON_FIELD],coin[MAX_JSON_FIELD],myacctcoinaddr[MAX_JSON_FIELD],mypubkey[MAX_JSON_FIELD],acctcoinaddr[MAX_JSON_FIELD],pubkey[MAX_JSON_FIELD],cmdstr[MAX_JSON_FIELD];
    //if ( is_remote_access(previpaddr) == 0 )
    //    return(0);
    copy_cJSON(coin,objs[0]);
    copy_cJSON(refNXTaddr,objs[1]);
    copy_cJSON(myacctcoinaddr,objs[2]);
    copy_cJSON(mypubkey,objs[3]);
    if ( coin[0] != 0 && refNXTaddr[0] != 0 && sender[0] != 0 && valid > 0 )
    {
        cp = get_coin_info(coin);
        if ( cp != 0 )
        {
            if ( myacctcoinaddr[0] != 0 && mypubkey[0] != 0 )
                add_NXT_coininfo(calc_nxt64bits(sender),conv_acctstr(refNXTaddr),cp->name,myacctcoinaddr,mypubkey);
            if ( get_acct_coinaddr(acctcoinaddr,cp,refNXTaddr) != 0 && get_bitcoind_pubkey(pubkey,cp,acctcoinaddr) != 0 )
            {
                sprintf(cmdstr,"{\"requestType\":\"setmsigpubkey\",\"NXT\":\"%s\",\"coin\":\"%s\",\"refNXTaddr\":\"%s\",\"addr\":\"%s\",\"userpubkey\":\"%s\"}",NXTaddr,coin,refNXTaddr,acctcoinaddr,pubkey);
                return(send_tokenized_cmd(!prevent_queueing("setmsigpubkey"),hopNXTaddr,0,NXTaddr,NXTACCTSECRET,cmdstr,sender));
            }
        }
    }
    return(clonestr("{\"error\":\"bad getmsigpubkey_func paramater\"}"));
}

char *setmsigpubkey_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char refNXTaddr[MAX_JSON_FIELD],coin[MAX_JSON_FIELD],acctcoinaddr[MAX_JSON_FIELD],userpubkey[MAX_JSON_FIELD];
    //struct contact_info *contact;
    struct coin_info *cp;
    uint64_t nxt64bits;
    if ( (MGW_initdone == 0 && Debuglevel > 3) || (MGW_initdone != 0 && Debuglevel > 2) )
        printf("setmsigpubkey(%s)\n",previpaddr);
    if ( is_remote_access(previpaddr) == 0 )
        return(0);
    copy_cJSON(coin,objs[0]);
    copy_cJSON(refNXTaddr,objs[1]);
    copy_cJSON(acctcoinaddr,objs[2]);
    copy_cJSON(userpubkey,objs[3]);
    cp = get_coin_info(coin);
    if ( (MGW_initdone == 0 && Debuglevel > 3) || (MGW_initdone != 0 && Debuglevel > 2) )
        printf("coin.(%s) %p ref.(%s) acc.(%s) pub.(%s)\n",coin,cp,refNXTaddr,acctcoinaddr,userpubkey);
    if ( cp != 0 && refNXTaddr[0] != 0 && acctcoinaddr[0] != 0 && userpubkey[0] != 0 && sender[0] != 0 && valid > 0 )
    {
        if ( (nxt64bits= conv_acctstr(refNXTaddr)) != 0 )
        {
            add_NXT_coininfo(calc_nxt64bits(sender),nxt64bits,coin,acctcoinaddr,userpubkey);
            //replace_msig_json(1,refNXTaddr,acctcoinaddr,pubkey,coin,contact->jsonstr);
            //update_contact_info(contact);
            //free(contact);
            return(clonestr("{\"result\":\"setmsigpubkey added coininfo\"}"));
        }
        return(clonestr("{\"error\":\"setmsigpubkey_func couldnt convert refNXTaddr\"}"));
    }
    return(clonestr("{\"error\":\"bad setmsigpubkey_func paramater\"}"));
}

char *MGWaddr_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    if ( Debuglevel > 0 )
        printf("MGWaddr_func(%s)\n",origargstr);
    if ( is_remote_access(previpaddr) == 0 )
        return(0);
    if ( sender[0] != 0 && valid > 0 )
        add_MGWaddr(previpaddr,sender,valid,origargstr);
    return(clonestr(origargstr));
}

char *MGW_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],asset[MAX_JSON_FIELD],NXT0[MAX_JSON_FIELD],NXT1[MAX_JSON_FIELD],NXT2[MAX_JSON_FIELD],ip0[MAX_JSON_FIELD],ip1[MAX_JSON_FIELD],ip2[MAX_JSON_FIELD],specialNXT[MAX_JSON_FIELD],exclude0[MAX_JSON_FIELD],exclude1[MAX_JSON_FIELD],exclude2[MAX_JSON_FIELD],destip[MAX_JSON_FIELD],pubkey[MAX_JSON_FIELD],email[MAX_JSON_FIELD],destNXT[MAX_JSON_FIELD],hopNXTaddr[64];
    int32_t rescan,actionflag;
    char *retstr,*str = 0;
    uint16_t bridgeport;
    if ( Global_mp->gatewayid < 0 && previpaddr != 0 )
        return(clonestr(origargstr));
    copy_cJSON(destip,objs[14]);
    bridgeport = (uint16_t)get_API_int(objs[15],0);
    if ( (retstr= bridge_test(1,NXTACCTSECRET,destip,bridgeport,origargstr)) != 0 )
    {
        printf("MGW bridge.(%s)\n",retstr);
        return(retstr);
    }
    copy_cJSON(NXT0,objs[0]);
    copy_cJSON(NXT1,objs[1]);
    copy_cJSON(NXT2,objs[2]);
    copy_cJSON(ip0,objs[3]);
    copy_cJSON(ip1,objs[4]);
    copy_cJSON(ip2,objs[5]);
    copy_cJSON(coin,objs[6]);
    copy_cJSON(asset,objs[7]);
    rescan = (int32_t)get_API_int(objs[8],0);
    actionflag = (int32_t)get_API_int(objs[9],0);
    copy_cJSON(specialNXT,objs[10]);
    copy_cJSON(exclude0,objs[11]);
    copy_cJSON(exclude1,objs[12]);
    copy_cJSON(exclude2,objs[13]);
    copy_cJSON(pubkey,objs[16]);
    copy_cJSON(email,objs[17]);
    copy_cJSON(destNXT,objs[18]);
    if ( sender[0] != 0 )
    {
        retstr = MGW(specialNXT,rescan,actionflag,coin,asset,NXT0,NXT1,NXT2,ip0,ip1,ip2,exclude0,exclude1,exclude2,destNXT,pubkey);
        if ( (MGW_initdone == 0 && Debuglevel > 3) || (MGW_initdone != 0 && Debuglevel > 2) )
            printf("got retstr.(%s)\n",retstr);
        if ( previpaddr != 0 )
        {
            if ( email[0] != 0 )
                send_email(email,sender,0,retstr);
            if ( strlen(retstr) < 1024 )
            {
                hopNXTaddr[0] = 0;
                str = send_tokenized_cmd(!prevent_queueing("MGWstatus"),hopNXTaddr,0,NXTaddr,NXTACCTSECRET,retstr,sender);
            }
            else
            {
                char fname[512];
                int32_t timeout = 300;
                //datalen = (int32_t)strlen(retstr) / 2;
                //data = malloc(datalen);
                //datalen = decode_hex(data,datalen,retstr);
                printf("start_transfer\n");
                sprintf(fname,"MGW%d.%s",Global_mp->gatewayid,(destNXT[0] == 0) ? "ALL" : destNXT);
                str = start_transfer(previpaddr,sender,NXTaddr,NXTACCTSECRET,previpaddr,fname,(uint8_t *)retstr,(int32_t)strlen(retstr),timeout,"bridge",0);
            }
            if ( str != 0 )
                free(str);
        }
        return(retstr);
    }
    return(clonestr("{\"error\":\"bad MGW_func paramater\"}"));
}

char *MGWresponse_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    if ( Debuglevel > 0 )
        printf("MGWresponse_func(%s)\n",origargstr);
    if ( is_remote_access(previpaddr) == 0 )
        return(0);
    return(clonestr(origargstr));
}

char *issue_ramstatus(char *coinstr)
{
    static char *Deposit_server = Server_ipaddrs[NUM_GATEWAYS-1]; // change this
    char *SuperNET_url();
    struct coin_info *cp;//,*refcp = get_coin_info("BTCD");
    char params[4096],retbuf[8192],*retstr,*serverip;
    int32_t gatewayid,nonz = 0;
    if ( (cp= get_coin_info(coinstr)) == 0 )
        return(clonestr("{\"error\":\"unsupported coin\"}"));
    strcpy(retbuf,"[");
    for (gatewayid=0; gatewayid<NUM_GATEWAYS; gatewayid++)
    {
        serverip = (gatewayid < NUM_GATEWAYS) ? Server_ipaddrs[gatewayid] : Deposit_server;
        //curl -k --data-binary '{"jsonrpc": "1.0", "id":"curltest", "method": "SuperNET", "params": ["{\"requestType\":\"MGW\",\"rescan\":\"1\",\"actionflag\":\"0\",\"handler\":\"mgw\",\"destip\":\"209.126.70.159\",\"destport\":\"4000\",\"specialNXT\":\"7117166754336896747\",\"coin\":\"BTCD\",\"asset\":\"11060861818140490423\",\"exclude0\":\"7581814105672729429\",\"destNXT\":\"NXT-BAD7-238Z-2SEX-2TJ2S\"}"]  }' -H 'content-type: text/plain;' https://127.0.0.1:7777/

        //sprintf(params,"{\"requestType\":\"MGW\",\"specialNXT\":\"%s\",\"destip\":\"%s\",\"destport\":%d,\"rescan\":%d,\"actionflag\":%d,\"refcontact\":\"%s\",\"userpubkey\":\"%s\",\"coin\":\"%s\",\"asset\":\"%s\",\"exclude0\":\"7581814105672729429\"}",cp->MGWissuer,Server_ipaddrs[gatewayid],refcp->bridgeport,rescan,actionflag,userNXTaddr,userpubkey,coinstr,cp->assetid);
        //sprintf(params,"{\"requestType\":\"MGW\",\"handler\":\"mgw\",\"destip\":\"%s\",\"destport\":%d,\"rescan\":%d,\"actionflag\":%d,\"destNXT\":\"%s\",\"userpubkey\":\"%s\",\"coin\":\"%s\",\"asset\":\"%s\",\"exclude0\":\"7581814105672729429\"}",serverip,refcp->bridgeport,rescan,actionflag,userNXTaddr,userpubkey,coinstr,cp->assetid);
        //printf("issue (%s)\n",params);
        sprintf(params,"{\"requestType\":\"ramstatus\",\"destip\":\"%s\",\"coin\":\"%s\"}",serverip,coinstr);
        retstr = bitcoind_RPC(0,(char *)"BTCD",SuperNET_url(),(char *)"",(char *)"SuperNET",params);
        if ( retstr != 0 )
        {
            if ( nonz++ != 0 )
                strcat(retbuf,", ");
            strcat(retbuf,retstr);
            if ( Debuglevel > 0 )
                printf("issue.(%s) -> (%s)\n",params,retstr);
           free(retstr);
        }
    }
    strcat(retbuf,"]");
    return(clonestr(retbuf));
}
#endif

#include "pluginapi.h"

char *SuperNET_json_commands(struct NXThandler_info *mp,char *previpaddr,cJSON *origargjson,char *sender,int32_t valid,char *origargstr)
{
    // local glue
    static char *gotjson[] = { (char *)gotjson_func, "BTCDjson", "V", "json", 0 };
    static char *gotpacket[] = { (char *)gotpacket_func, "gotpacket", "V", "msg", "dur", "ip_port", 0 };
    static char *gotnewpeer[] = { (char *)gotnewpeer_func, "gotnewpeer", "V", "ip_port", 0 };
    static char *BTCDpoll[] = { (char *)BTCDpoll_func, "BTCDpoll", "V", 0 };
    static char *GUIpoll[] = { (char *)GUIpoll_func, "GUIpoll", "V", 0 };
    static char *stop[] = { (char *)stop_func, "stop", "V", 0 };
    static char *settings[] = { (char *)settings_func, "settings", "V", "field", "value", "reinit", 0 };
    
#ifdef later
    // remotable ramchains
    static char *rampyramid[] = { (char *)rampyramid_func, "rampyramid", "V", "destip", "port", "coin", "blocknum", "type", 0 };
    static char *ramstatus[] = { (char *)ramstatus_func, "ramstatus", "V", "destip", "port", "coin", 0 };
    static char *ramstring[] = { (char *)ramstring_func, "ramstring", "V", "destip", "port", "coin", "type", "rawind", 0 };
    static char *ramrawind[] = { (char *)ramrawind_func, "ramrawind", "V", "destip", "port", "coin", "type", "string", 0 };
    static char *ramscript[] = { (char *)ramscript_func, "ramscript", "V", "destip", "port", "coin", "txid", "vout", "blocknum", "txind", "v", 0 };
    static char *ramblock[] = { (char *)ramblock_func, "ramblock", "V", "destip", "port", "coin", "blocknum", 0 };
    static char *ramresponse[] = { (char *)ramresponse_func, "ramresponse", "V", "coin", "origcmd", "data", 0 };
    // local ramchains
    static char *ramtxlist[] = { (char *)ramtxlist_func, "ramtxlist", "V", "coin", "address", "unspent", 0 };
    static char *ramrichlist[] = { (char *)ramrichlist_func, "ramrichlist", "V", "coin", "numwhales", "recalc", 0 };
    static char *ramaddrlist[] = { (char *)ramaddrlist_func, "ramaddrlist", "V", "coin", 0 };
    static char *rambalances[] = { (char *)rambalances_func, "rambalances", "V", "coin", "coins", "rates", 0 };
    static char *ramcompress[] = { (char *)ramcompress_func, "ramcompress", "V", "coin", "data", 0 };
    static char *ramexpand[] = { (char *)ramexpand_func, "ramexpand", "V", "coin", "data", 0 };

    // MGW
    static char *genmultisig[] = { (char *)genmultisig_func, "genmultisig", "", "userpubkey", "coin", "refcontact", "M", "N", "contacts", "destip", "destport", "email", "buyNXT", 0 };
    static char *getmsigpubkey[] = { (char *)getmsigpubkey_func, "getmsigpubkey", "V", "coin", "refNXTaddr", "myaddr", "mypubkey", 0 };
    static char *MGWaddr[] = { (char *)MGWaddr_func, "MGWaddr", "V", 0 };
    static char *MGWresponse[] = { (char *)MGWresponse_func, "MGWresponse", "V", 0 };
    static char *setmsigpubkey[] = { (char *)setmsigpubkey_func, "setmsigpubkey", "V", "coin", "refNXTaddr", "addr", "userpubkey", 0 };
    //static char *MGW[] = { (char *)MGW_func, "MGW", "", "NXT0", "NXT1", "NXT2", "ip0", "ip1", "ip2", "coin", "asset", "rescan", "actionflag", "specialNXT", "exclude0", "exclude1", "exclude2", "destip", "destport", "userpubkey", "email", "destNXT", 0 };
    static char *cosign[] = { (char *)cosign_func, "cosign", "V", "otheracct", "seed", "text", 0 };
    static char *cosigned[] = { (char *)cosigned_func, "cosigned", "V", "seed", "result", "privacct", "pubacct", 0 };
    static char *sendfrag[] = { (char *)sendfrag_func, "sendfrag", "V", "pubkey", "name", "fragi", "numfrags", "ipaddr", "totalcrc", "datacrc", "data", "totallen", "blocksize", "handler", "syncmem", 0 };
    static char *gotfrag[] = { (char *)gotfrag_func, "gotfrag", "V", "pubkey", "name", "fragi", "numfrags", "ipaddr", "totalcrc", "datacrc", "totallen", "blocksize", "count", "handler", "syncmem", "snapshotcrc", 0 };
    static char *startxfer[] = { (char *)startxfer_func, "startxfer", "V", "fname", "dest", "data", "timeout", "handler", "syncmem", 0 };
    // MofNfs
    static char *savefile[] = { (char *)savefile_func, "savefile", "V", "fname", "L", "M", "N", "backup", "password", "pin", 0 };
    static char *restorefile[] = { (char *)restorefile_func, "restorefile", "V", RESTORE_ARGS, 0 };
    static char *publish[] = { (char *)publish_func, "publish", "V", "files", "L", "M", "N", "backup", "password", "pin", 0  };
    // Privatbet
    static char *lotto[] = { (char *)lotto_func, "lotto", "V", "refacct", "asset", "lottoseed", "prizefund", 0 };
#endif
    // IP comms
    static char *ping[] = { (char *)ping_func, "ping", "V", "pubkey", "ipaddr", "port", "destip", "MMatrix", 0 };
    static char *pong[] = { (char *)pong_func, "pong", "V", "pubkey", "ipaddr", "port", "yourip", "yourport", "tag", "MMatrix", 0 };
     static char *getfile[] = { (char *)getfile_func, "getfile", "V", "name", "handler", 0 };

    // Kademlia DHT
    static char *puzzles[] = { (char *)challenge_func, "puzzles", "V", "reftime", "duration", "threshold", 0 };
    static char *nonces[] = { (char *)response_func, "nonces", "V", "reftime", "threshold", "nonces", 0 };
    static char *store[] = { (char *)store_func, "store", "V", "pubkey", "key", "name", "data", 0 };
    static char *findvalue[] = { (char *)findvalue_func, "findvalue", "V", "pubkey", "key", "name", "data", 0 };
    static char *findnode[] = { (char *)findnode_func, "findnode", "V", "pubkey", "key", "name", "data", 0 };
    static char *havenode[] = { (char *)havenode_func, "havenode", "V", "pubkey", "key", "name", "data", 0 };
    static char *havenodeB[] = { (char *)havenodeB_func, "havenodeB", "V", "pubkey", "key", "name", "data", 0 };
    static char *findaddress[] = { (char *)findaddress_func, "findaddress", "V", "refaddr", "list", "dist", "duration", "numthreads", 0 };

    
    // Telepathy
    static char *getpeers[] = { (char *)getpeers_func, "getpeers", "V",  "scan", 0 };
    static char *addcontact[] = { (char *)addcontact_func, "addcontact", "V",  "handle", "acct", 0 };
    static char *removecontact[] = { (char *)removecontact_func, "removecontact", "V",  "contact", 0 };
    static char *dispcontact[] = { (char *)dispcontact_func, "dispcontact", "V",  "contact", 0 };
    static char *telepathy[] = { (char *)telepathy_func, "telepathy", "V",  "contact", "id", "type", "attach", 0 };
    static char *getdb[] = { (char *)getdb_func, "getdb", "V",  "contact", "id", "key", "dir", "destip", 0 };
    static char *sendmsg[] = { (char *)sendmsg_func, "sendmessage", "V", "dest", "msg", "L", 0 };
    static char *sendbinary[] = { (char *)sendbinary_func, "sendbinary", "V", "dest", "data", "L", 0 };

    // Teleport
    static char *maketelepods[] = { (char *)maketelepods_func, "maketelepods", "V", "amount", "coin", 0 };
    static char *telepodacct[] = { (char *)telepodacct_func, "telepodacct", "V", "amount", "contact", "coin", "comment", "cmd", "withdraw", 0 };
    static char *teleport[] = { (char *)teleport_func, "teleport", "V", "amount", "contact", "coin", "minage", "withdraw", 0 };
    
    // InstantDEX
    static char *trollbox[] = { (char *)trollbox_func, "trollbox", "V", "post", "whaleindex", 0 };
    static char *allorderbooks[] = { (char *)allorderbooks_func, "allorderbooks", "V", 0 };
    static char *orderbook[] = { (char *)orderbook_func, "orderbook", "V", "baseid", "relid", "allfields", "oldest", "maxdepth", "base", "rel", "gui", "showall", 0 };
    static char *lottostats[] = { (char *)lottostats_func, "lottostats", "V", "timestamp", 0 };
    static char *cancelquote[] = { (char *)cancelquote_func, "cancelquote", "V", "quoteid", 0 };
    static char *openorders[] = { (char *)openorders_func, "openorders", "V", 0 };
    static char *placebid[] = { (char *)placebid_func, "placebid", "V", "baseid", "relid", "volume", "price", "timestamp", "baseamount", "relamount", "gui", "automatch", "minperc", "duration", "exchange", 0 };
    static char *placeask[] = { (char *)placeask_func, "placeask", "V", "baseid", "relid", "volume", "price", "timestamp", "baseamount", "relamount", ",gui", "automatch", "minperc", "duration", "exchange", 0 };
    static char *bid[] = { (char *)bid_func, "bid", "V", "baseid", "relid", "volume", "price", "timestamp", "baseamount", "relamount", "gui", "automatch", "minperc", "duration", "exchange", 0 };
    static char *ask[] = { (char *)ask_func, "ask", "V", "baseid", "relid", "volume", "price", "timestamp", "baseamount", "relamount", "gui", "automatch", "minperc", "duration", "exchange", 0 };
    static char *makeoffer3[] = { (char *)makeoffer3_func, "makeoffer3", "V", "baseid", "relid", "quoteid", "perc", "deprecated", "baseiQ", "reliQ", "askoffer", "price", "volume", "exchange", "baseamount", "relamount", "offerNXT", "minperc", "jumpasset", 0 };
    static char *respondtx[] = { (char *)respondtx_func, "respondtx", "V", "cmd", "assetid", "quantityQNT", "priceNQT", "triggerhash", "quoteid", "sig", "data", "minperc", "offerNXT", "otherassetid", "otherqty", 0 };
    static char *jumptrades[] = { (char *)jumptrades_func, "jumptrades", "V", 0 };
    static char *tradehistory[] = { (char *)tradehistory_func, "tradehistory", "V", "timestamp", 0 };
  //static char *processjumptrade[] = { (char *)processjumptrade_func, "processjumptrade", "V", "assetA", "amountA", "other", "assetB", "amountB", "feeA", "feeAtxid", "triggerhash", "jumper", "jumpasset", "jumpamount", "balancing", "balancetxid", "gui", "quoteid", 0 };
    //static char *processutx[] = { (char *)processutx_func, "processutx", "V", "utx", "sig", "full", "feeAtxid", "quoteid", 0 };
    //static char *makeoffer[] = { (char *)makeoffer_func, "makeoffer", "V", "baseid", "relid", "baseamount", "relamount", "other", "type", "quoteid", 0 };
    //static char *makeoffer2[] = { (char *)makeoffer2_func, "makeoffer2", "V", "baseid", "baseamount", "jumpaddr", "jumpasset", "jumpamount", "other", "relid", "relamount", "gui", "quoteid", 0 };

    // Tradebot
    static char *allsignals[] = { (char *)allsignals_func, "allsignals", "V", 0 };
    static char *getsignal[] = { (char *)getsignal_func, "getsignal", "V", "signal", "start", "width", "resolution", "baseid", "relid", "base", "rel", "exchange", 0 };
    //static char *pricedb[] = { (char *)pricedb_func, "pricedb", "V", "exchange", "base", "rel", "stop", 0 };
    //static char *getquotes[] = { (char *)getquotes_func, "getquotes", "V", "exchange", "base", "rel", "oldest", 0 };
    static char *tradebot[] = { (char *)tradebot_func, "tradebot", "V", "code", 0 };


    // plugins
    static char *passthru[] = { (char *)passthru_func, "passthru", "V", "coin", "method", "params", "tag", "plugin", "daemonid", "instanceid", 0 };
    static char *plugin[] = { (char *)plugin_func, "plugin", "V", "name", "daemonid", "instanceid", "method", "tag", "iters", "timeout", 0 };
    static char *registerplugin[] = { (char *)register_func, "register", "V", "plugin", "daemonid", "instanceid", "methods", 0 };
    static char *remote[] = { (char *)remote_func, "remote", "V",  "coin", "method", "result", "tag", 0 };
    //static char *python[] = { (char *)python_func, "python", "V",  "name", "launch", "websocket", 0 };
    static char *syscall[] = { (char *)syscall_func, "syscall", "V", "name", "daemonize", "websocket", "jsonargs", "plugin", "ipaddr", "port", 0 };
    static char *checkmsg[] = { (char *)checkmsg_func, "checkmessages", "V", "daemonid", 0 };

    static char **commands[] = { registerplugin, plugin, stop, GUIpoll, BTCDpoll, settings, gotjson, gotpacket, gotnewpeer, getdb, telepathy, addcontact, dispcontact, removecontact, findaddress, puzzles, nonces, ping, pong, store, findnode, havenode, havenodeB, findvalue, syscall, getpeers, maketelepods, tradebot, respondtx, checkmsg, openorders, allorderbooks, placebid, bid, placeask, ask, sendmsg, sendbinary, orderbook, teleport, telepodacct, passthru, remote,
#ifdef later
         cosign, cosigned, genmultisig, getmsigpubkey, setmsigpubkey, MGWaddr, MGWresponse, sendfrag, gotfrag, startxfer, lotto, ramstring, ramrawind, ramblock, ramcompress, ramexpand, ramscript, ramtxlist, ramrichlist, rambalances, ramstatus, ramaddrlist, rampyramid, ramresponse, publish,  savefile, restorefile, 
#endif
        getfile, allsignals, getsignal, jumptrades, cancelquote, lottostats, tradehistory, makeoffer3, trollbox };
    int32_t i,j;
    struct coin_info *cp;
    cJSON *argjson,*obj,*nxtobj,*secretobj,*objs[64];
    char NXTaddr[MAX_JSON_FIELD],NXTACCTSECRET[MAX_JSON_FIELD],command[MAX_JSON_FIELD],**cmdinfo,*retstr=0;
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
        if ( is_remote_access(previpaddr) == 0 && strcmp(NXTaddr,Global_mp->myNXTADDR) != 0 )// && strcmp(NXTaddr,Global_mp->privateNXTADDR) != 0 )
        {
            strcpy(NXTaddr,Global_mp->myNXTADDR);
            ensure_jsonitem(argjson,"NXT",NXTaddr);
            valid = 1;
            printf("subsititute NXT.%s\n",NXTaddr);
        }
        copy_cJSON(command,obj);
        if ( is_enabled_command(command) == 0 )
            return(clonestr("{\"error\":\"command disabled\"}"));
        copy_cJSON(NXTACCTSECRET,secretobj);
        if ( NXTACCTSECRET[0] == 0 && (cp= get_coin_info("BTCD")) != 0 )
        {
            if ( is_remote_access(previpaddr) == 0 || strcmp(command,"findnode") != 0 )
            {
                if ( 1 || notlocalip(cp->privacyserver) == 0 )
                {
                    safecopy(NXTACCTSECRET,Global_mp->srvNXTACCTSECRET,sizeof(NXTACCTSECRET));
                    strcpy(NXTaddr,Global_mp->myNXTADDR);
                    //expand_nxt64bits(NXTaddr,cp->srvpubnxtbits);
                    //printf("use localserver NXT.%s to send command (cp %llu)\n",NXTaddr,(long long)cp->srvpubnxtbits);
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
        //fprintf(stderr,"SuperNET_json_commands sender.(%s) valid.%d | size.%d | command.(%s) orig.(%s)\n",sender,valid,(int32_t)(sizeof(commands)/sizeof(*commands)),command,origargstr);
        for (i=0; i<(int32_t)(sizeof(commands)/sizeof(*commands)); i++)
        {
            cmdinfo = commands[i];
            //printf("needvalid.(%c) sender.(%s) valid.%d %d of %d: cmd.(%s) vs command.(%s)\n",cmdinfo[2][0],sender,valid,i,(int32_t)(sizeof(commands)/sizeof(*commands)),cmdinfo[1],command);
            if ( strcmp(cmdinfo[1],command) == 0 )
            {
                //printf("%d %d\n",cmdinfo[2][0],valid);
                if ( cmdinfo[2][0] != 0 && valid <= 0 )
                    return(0);
                for (j=3; cmdinfo[j]!=0&&j<3+(int32_t)(sizeof(objs)/sizeof(*objs)); j++)
                    objs[j-3] = cJSON_GetObjectItem(argjson,cmdinfo[j]);
                retstr = (*(json_handler)cmdinfo[0])(NXTaddr,NXTACCTSECRET,previpaddr,sender,valid,objs,j-3,origargstr);
                if ( is_remote_access(previpaddr) != 0 )
                {
                    struct resultsitem *rp = calloc(1,sizeof(*rp));
                    if ( origargstr != 0 )
                        rp->argstr = clonestr(origargstr);
                    else rp->argstr = clonestr("{}");
                    if ( retstr != 0 )
                        rp->retstr = clonestr(retstr);
                    else rp->retstr = clonestr("{}");
                    rp->txid = calc_ipbits(previpaddr);
                    queue_GUIpoll(rp);
                }
                break;
            }
        }
    } else printf("not JSON to parse?\n");
    return(retstr);
}


#endif

