//
//  plugins.h
//
//  Created by jl777 on 16/4/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifndef xcode_plugins_h
#define xcode_plugins_h

#include "nn.h"
#include "bus.h"
#include "pair.h"
#include "pubsub.h"

struct daemon_info
{
    queue_t messages;
    uint64_t daemonid,instanceids[256];
    int32_t finished,dereferenced,daemonsock,websocket;
    char *cmd,*arg;
    void (*daemonfunc)(int32_t websocket,char *cmd,char *arg,uint64_t daemonid);
} *Daemoninfos[1024]; int32_t Numdaemons;

struct daemon_info *find_daemoninfo(uint64_t daemonid)
{
    int32_t i;
    if ( Numdaemons > 0 )
    {
        for (i=0; i<Numdaemons; i++)
            if ( Daemoninfos[i] != 0 && Daemoninfos[i]->daemonid == daemonid )
                return(Daemoninfos[i]);
    }
    return(0);
}

int32_t add_instanceid(struct daemon_info *dp,uint64_t instanceid)
{
    int32_t i;
    for (i=0; i<(sizeof(dp->instanceids)/sizeof(*dp->instanceids)); i++)
    {
        if ( dp->instanceids[i] == 0 )
        {
            dp->instanceids[i] = instanceid;
            return(1);
        }
        if ( dp->instanceids[i] == instanceid )
            return(0);
    }
    dp->instanceids[rand() % (sizeof(dp->instanceids)/sizeof(*dp->instanceids))] = instanceid;
    return(-1);
}

cJSON *instances_json(struct daemon_info *dp)
{
    cJSON *array = cJSON_CreateArray();
    char numstr[64];
    int32_t i;
    for (i=0; i<(sizeof(dp->instanceids)/sizeof(*dp->instanceids)); i++)
    {
        if ( dp->instanceids[i] != 0 )
            sprintf(numstr,"%llu",(long long)dp->instanceids[i]), cJSON_AddItemToArray(array,cJSON_CreateString(numstr));
    }
    return(array);
}

void connect_instanceid(struct daemon_info *dp,uint64_t instanceid)
{
    int32_t err;
    cJSON *json;
    char addr[64],numstr[64],*jsonstr;
    sprintf(addr,"ipc://%llu",(long long)instanceid);
    if ( (err= nn_connect(dp->daemonsock,addr)) < 0 )
    {
        printf("error %d nn_connect err.%s (%llu to %s)\n",dp->daemonsock,nn_strerror(nn_errno()),(long long)dp->daemonid,addr);
        return;
    }
    /*if ( (err= nn_connect(dp->pubsock,addr)) < 0 )
    {
        printf("error %d nn_connect err.%s (%llu to %s)\n",dp->daemonsock,nn_strerror(nn_errno()),(long long)dp->daemonid,addr);
        return;
    }*/
    printf("connect_instanceid: %d nn_connect (%llu <-> %s)\n",dp->daemonsock,(long long)dp->daemonid,addr);
    json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"requestType",cJSON_CreateString("instances"));
    cJSON_AddItemToObject(json,"instanceids",instances_json(dp));
    sprintf(numstr,"%llu",(long long)dp->daemonid), cJSON_AddItemToObject(json,"myid",cJSON_CreateString(numstr));
    jsonstr = cJSON_Print(json);
    stripwhite(jsonstr,strlen(jsonstr));
    free_json(json);
    nn_send(dp->daemonsock,jsonstr,strlen(jsonstr) + 1,0);
    free(jsonstr);
}

int32_t init_daemonsock(int32_t *pubsockp,uint64_t daemonid)
{
    int32_t sock,err,to = 1;
    char addr[64];
    sprintf(addr,"ipc://%llu",(long long)daemonid);
    printf("init_daemonsocks %s\n",addr);
    if ( (sock= nn_socket(AF_SP,NN_BUS)) < 0 )
    {
        printf("error %d nn_socket err.%s\n",sock,nn_strerror(nn_errno()));
        return(-1);
    }
    if ( (err= nn_bind(sock,addr)) < 0 )
    {
        printf("error %d nn_bind.%d (%s) | %s\n",err,sock,addr,nn_strerror(nn_errno()));
        return(-1);
    }
    /*if ( (*pubsockp= nn_socket(AF_SP,NN_PUB)) < 0 )
    {
        printf("error %d nn_socket err.%s\n",*pubsockp,nn_strerror(nn_errno()));
        return(-1);
    }
    sprintf(addr,"ipc://%llu",(long long)daemonid+1);
    if ( (err= nn_bind(*pubsockp,addr)) < 0 )
    {
        printf("error %d nn_bind.%d (%s) | %s\n",err,*pubsockp,addr,nn_strerror(nn_errno()));
        return(-1);
    }*/
   /* if ( (err= nn_device(sock,*pubsockp)) < 0 )
    {
        printf("error %d nn_device.%d %d | %s\n",err,sock,*pubsockp,nn_strerror(nn_errno()));
        return(-1);
    }*/
    assert (nn_setsockopt(sock,NN_SOL_SOCKET,NN_RCVTIMEO,&to,sizeof (to)) >= 0);
    return(sock);
}

int32_t poll_daemons()
{
    struct nn_pollfd pfd[sizeof(Daemoninfos)/sizeof(*Daemoninfos)];
    int32_t flag,len,processed=0,rc,i,n = 0;
    char request[MAX_JSON_FIELD],*retstr;
    uint64_t instanceid;
    struct daemon_info *dp;
    cJSON *json;
    char *msg;
    if ( Numdaemons > 0 )
    {
        memset(pfd,0,sizeof(pfd));
        for (i=flag=0; i<Numdaemons; i++)
        {
            if ( (dp= Daemoninfos[i]) != 0 )
            {
                if ( dp->finished != 0 )
                {
                    printf("daemon.%llu finished\n",(long long)dp->daemonid);
                    Daemoninfos[i] = 0;
                    dp->dereferenced = 1;
                    flag++;
                }
                else
                {
                    pfd[i].fd = dp->daemonsock;
                    pfd[i].events = NN_POLLIN | NN_POLLOUT;
                    n++;
                }
            }
        }
        if ( n > 0 )
        {
            if ( (rc= nn_poll(pfd,Numdaemons,1)) > 0 )
            {
                for (i=0; i<Numdaemons; i++)
                {
                    if ( (pfd[i].revents & NN_POLLIN) != 0 )
                    {
                        if ( (dp= Daemoninfos[i]) != 0 && dp->finished == 0 )
                        {
                            if ( (len= nn_recv(dp->daemonsock,&msg,NN_MSG,0)) > 0 )
                            {
                                printf(">>>>>>>>>> RECEIVED.%d i.%d/%d (%s) FROM (%s) %llu\n",n,i,Numdaemons,msg,dp->cmd,(long long)dp->daemonid);
                                nn_send(dp->daemonsock,msg,len,0);
                                if ( (json= cJSON_Parse(msg)) != 0 )
                                {
                                    if ( (instanceid= get_API_nxt64bits(cJSON_GetObjectItem(json,"myid"))) != 0 )
                                    {
                                        if ( add_instanceid(dp,instanceid) != 0 )
                                            connect_instanceid(dp,instanceid);
                                    }
                                    copy_cJSON(request,cJSON_GetObjectItem(json,"pluginrequest"));
                                    if ( strcmp(request,"SuperNET") == 0 )
                                    {
                                        char *call_SuperNET_JSON(char *JSONstr);
                                        if ( (retstr= call_SuperNET_JSON(msg)) != 0 )
                                        {
                                            nn_send(dp->daemonsock,retstr,(int32_t)strlen(retstr)+1,0);
                                            free(retstr);
                                        }
                                    }
                                } else printf("parse error.(%s)\n",msg);
                                if ( dp->websocket == 0 )
                                    queue_enqueue("daemon",&dp->messages,msg);
                            }
                            processed++;
                        }
                    }
                }
            }
            else if ( rc < 0 )
                printf("Error polling daemons.%d\n",rc);
        }
        if ( flag != 0 )
        {
            static portable_mutex_t mutex; static int didinit;
            printf("compact Daemoninfos as %d have finished\n",flag);
            if ( didinit == 0 )
                portable_mutex_init(&mutex), didinit = 1;
            portable_mutex_lock(&mutex);
            for (i=n=0; i<Numdaemons; i++)
                if ( (Daemons[n]= Daemons[i]) != 0 )
                    n++;
            Numdaemons = n;
            portable_mutex_unlock(&mutex);
        }
    }
    return(processed);
}

void *daemon_loop(void *args)
{
    struct daemon_info *dp = args;
    (*dp->daemonfunc)(dp->websocket,dp->cmd,dp->arg,dp->daemonid);
    printf("daemonid.%llu (%s %s) finished\n",(long long)dp->daemonid,dp->cmd,dp->arg!=0?dp->arg:"");
    dp->finished = 1;
    while ( dp->dereferenced == 0 )
        sleep(1);
    printf("daemonid.%llu (%s %s) dereferenced\n",(long long)dp->daemonid,dp->cmd,dp->arg!=0?dp->arg:"");
    if ( dp->daemonsock >= 0 )
        nn_shutdown(dp->daemonsock,0);
    //if ( dp->pubsock >= 0 )
    //    nn_shutdown(dp->pubsock,0);
    free(dp->cmd), free(dp->arg), free(dp);
    return(0);
}

char *launch_daemon(int32_t websocket,char *cmd,char *arg,void (*daemonfunc)(int32_t websocket,char *cmd,char *fname,uint64_t daemonid))
{
    struct daemon_info *dp;
    char retbuf[1024];
    int32_t daemonsock,pubsock;
    uint64_t daemonid;
    if ( Numdaemons >= sizeof(Daemoninfos)/sizeof(*Daemoninfos) )
        return(clonestr("{\"error\":\"too many daemons, cant create anymore\"}"));
    daemonid = (uint64_t)(milliseconds() * 1000000) & (~(uint64_t)1);
    if ( (daemonsock= init_daemonsock(&pubsock,daemonid)) >= 0 && pubsock >= 0 )
    {
        dp = calloc(1,sizeof(*dp));
        dp->cmd = clonestr(cmd);
        dp->daemonid = daemonid;
        dp->daemonsock = daemonsock;//, dp->pubsock = pubsock;
        dp->arg = (arg != 0 && arg[0] != 0) ? clonestr(arg) : 0;
        dp->daemonfunc = daemonfunc;
        dp->websocket = websocket;
        Daemoninfos[Numdaemons++] = dp;
        if ( portable_thread_create((void *)daemon_loop,dp) == 0 )
        {
            free(dp->cmd), free(dp->arg), free(dp);
            nn_shutdown(dp->daemonsock,0);
            //nn_shutdown(dp->pubsock,0);
            return(clonestr("{\"error\":\"portable_thread_create couldnt create daemon\"}"));
        }
        sprintf(retbuf,"{\"result\":\"launched\",\"daemonid\":\"%llu\"}\n",(long long)dp->daemonid);
        return(clonestr(retbuf));
    }
    return(clonestr("{\"error\":\"cant open file to launch daemon\"}"));
}

char *language_func(int32_t websocket,int32_t launchflag,char *cmd,char *fname,void (*daemonfunc)(int32_t websocket,char *cmd,char *fname,uint64_t daemonid))
{
    char buffer[MAX_LEN+1] = { 0 };
    int out_pipe[2];
    int saved_stdout;
    if ( launchflag != 0 )
        return(launch_daemon(websocket,cmd,fname,daemonfunc));
    saved_stdout = dup(STDOUT_FILENO);
    if( pipe(out_pipe) != 0 )
        return(clonestr("{\"error\":\"pipe creation error\"}"));
    dup2(out_pipe[1], STDOUT_FILENO);
    close(out_pipe[1]);
    (*daemonfunc)(websocket,cmd,fname,0);
    fflush(stdout);
    read(out_pipe[0],buffer,MAX_LEN);
    dup2(saved_stdout,STDOUT_FILENO);
    return(clonestr(buffer));
}

char *checkmessages(char *NXTaddr,char *NXTACCTSECRET,uint64_t daemonid)
{
    char *msg,*retstr = 0;
    cJSON *array = 0,*json = 0;
    struct daemon_info *dp;
    int32_t i;
    if ( (dp= find_daemoninfo(daemonid)) != 0 )
    {
        for (i=0; i<10; i++)
        {
            if ( (msg= queue_dequeue(&dp->messages)) != 0 )
            {
                if ( json == 0 )
                    json = cJSON_CreateObject(), array = cJSON_CreateArray();
                cJSON_AddItemToArray(array,cJSON_CreateString(msg));
                nn_freemsg(msg);
            }
        }
        if ( json == 0 )
            return(clonestr("{\"result\":\"no messages\",\"messages\":[]}"));
        else
        {
            cJSON_AddItemToObject(json,"result",cJSON_CreateString("daemon messages"));
            cJSON_AddItemToObject(json,"messages",array);
            retstr = cJSON_Print(json);
            free_json(json);
            return(retstr);
        }
    }
    return(clonestr("{\"error\":\"cant find daemonid\"}"));
}

int file_exists(char *filename)
{
    struct stat buffer;
    return(stat(filename,&buffer) == 0);
}

/*void call_python(int32_t websocket,char *cmd,char *fname,uint64_t daemonid)
{
    FILE *fp;
    if ( (fp= fopen(fname,"r")) != 0 )
    {
        Py_Initialize();
        PyRun_SimpleFile(fp,fname);
        Py_Finalize();
        fclose(fp);
    }
}*/

void call_system(int32_t websocket,char *cmd,char *arg,uint64_t daemonid)
{
    char cmdstr[MAX_JSON_FIELD];
    if ( websocket != 0 )
        sprintf(cmdstr,"../websocketd --port=%d %s",websocket,(Debuglevel > 0) ? "--devconsole ":"");
    else cmdstr[0] = 0;
    sprintf(cmdstr + strlen(cmdstr),"%s %llu",cmd,(long long)daemonid);//,arg!=0?arg:" ");
    printf("SYSTEM.(%s)\n",cmdstr);
    system(cmdstr);
}

char *checkmsg_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char *retstr = 0;
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    if ( sender[0] != 0 && valid > 0 )
        retstr = checkmessages(sender,NXTACCTSECRET,get_API_nxt64bits(objs[0]));
    else retstr = clonestr("{\"result\":\"invalid checkmessages request\"}");
    return(retstr);
}

int32_t send_to_daemon(uint64_t daemonid,char *jsonstr)
{
    int32_t len;
    cJSON *json;
    struct daemon_info *dp;
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        free_json(json);
        if ( (dp= find_daemoninfo(daemonid)) != 0 )
        {
            if ( (len= (int32_t)strlen(jsonstr)) > 0 )
                return(nn_send(dp->daemonsock,jsonstr,len + 1,0));
            else printf("send_to_daemon: error jsonstr.(%s)\n",jsonstr);
        }
    }
    else printf("send_to_daemon: cant parse jsonstr.(%s)\n",jsonstr);
    return(-1);
}

char *passthru_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char hopNXTaddr[64],tagstr[MAX_JSON_FIELD],coinstr[MAX_JSON_FIELD],method[MAX_JSON_FIELD],params[MAX_JSON_FIELD],*str2,*cmdstr,*retstr = 0;
    struct coin_info *cp = 0;
    uint64_t daemonid;
    copy_cJSON(coinstr,objs[0]);
    copy_cJSON(method,objs[1]);
    if ( is_remote_access(previpaddr) != 0 )
    {
        if ( in_jsonarray(cJSON_GetObjectItem(MGWconf,"remote"),method) == 0 && in_jsonarray(cJSON_GetObjectItem(cp->json,"remote"),method) == 0 )
            return(0);
    }
    copy_cJSON(params,objs[2]);
    unstringify(params);
    copy_cJSON(tagstr,objs[3]);
    daemonid = get_API_nxt64bits(objs[4]);
    printf("daemonid.%llu tag.(%s) passthru.(%s) %p method=%s [%s]\n",(long long)daemonid,tagstr,coinstr,cp,method,params);
    if ( sender[0] != 0 && valid > 0 )
    {
        if ( daemonid != 0 )
        {
            unstringify(params);
            send_to_daemon(daemonid,params);
            return(clonestr("{\"result\":\"unstringified params sent to daemon\"}"));
        }
        else if ( (cp= get_coin_info(coinstr)) != 0 && method[0] != 0 )
            retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,method,params);
    }
    else retstr = clonestr("{\"error\":\"invalid passthru_func arguments\"}");
    if ( is_remote_access(previpaddr) != 0 )
    {
        cmdstr = malloc(strlen(retstr)+512);
        str2 = stringifyM(retstr);
        sprintf(cmdstr,"{\"requestType\":\"remote\",\"coin\":\"%s\",\"method\":\"%s\",\"tag\":\"%s\",\"result\":\"%s\"}",coinstr,method,tagstr,str2);
        free(str2);
        hopNXTaddr[0] = 0;
        retstr = send_tokenized_cmd(!prevent_queueing("passthru"),hopNXTaddr,0,NXTaddr,NXTACCTSECRET,cmdstr,sender);
        free(cmdstr);
    }
    return(retstr);
}

char *remote_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    if ( is_remote_access(previpaddr) == 0 )
        return(clonestr("{\"error\":\"cant remote locally\"}"));
    return(clonestr(origargstr));
}

/*char *python_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char fname[MAX_JSON_FIELD],*retstr;
    int32_t launchflag,websocket;
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    copy_cJSON(fname,objs[0]);
    launchflag = get_API_int(objs[1],0);
    websocket = get_API_int(objs[2],0);
    if ( file_exists(fname) != 0 )
    {
        retstr = language_func(websocket,launchflag,"python",fname,call_python);
        if ( retstr != 0 )
            printf("(%s) -> (%s)\n",fname,retstr);
        return(retstr);
    }
    else return(clonestr("{\"error\":\"file doesn't exist\"}"));
}*/

char *syscall_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char arg[MAX_JSON_FIELD],syscall[MAX_JSON_FIELD];
    int32_t launchflag,websocket;
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    copy_cJSON(syscall,objs[0]);
    launchflag = get_API_int(objs[1],0);
    websocket = get_API_int(objs[2],0);
    copy_cJSON(arg,objs[3]);
    printf("websocket.%d launchflag.%d syscall.(%s) arg.(%s)\n",websocket,launchflag,syscall,arg!=0?arg:"");
    return(language_func(websocket,launchflag,syscall,arg,call_system));
}

#endif
