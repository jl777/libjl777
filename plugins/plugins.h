//
//  plugins.h
// 
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifndef xcode_plugins_h
#define xcode_plugins_h

#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>

#define DAEMONFREE_MARGIN_SECONDS 13
#define NUM_PLUGINTAGS 256

struct daemon_info
{
    struct queueitem DL;
    queue_t messages;
    char name[64],ipaddr[64],*cmd,*jsonargs;
    cJSON *methodsjson,*pubmethods,*authmethods;
    double lastsearch;
    union endpoints perm,wss;
    int32_t (*daemonfunc)(struct daemon_info *dp,int32_t permanentflag,char *cmd,char *jsonargs);
    uint64_t daemonid,myid,instanceids[256];
    uint64_t tags[NUM_PLUGINTAGS][3];
    uint32_t numsent,numrecv;
    int32_t lasti,finished,websocket,allowremote,bundledflag,readyflag;//,pairsocks[256];
    uint16_t port;
} *Daemoninfos[1024]; int32_t Numdaemons;
queue_t DaemonQ;

#include "transport.h"

void free_daemon_info(struct daemon_info *dp)
{
    if ( dp->cmd != 0 )
        free(dp->cmd);
    if ( dp->jsonargs != 0 )
        free(dp->jsonargs);
    if ( dp->methodsjson != 0 )
        free_json(dp->methodsjson);
    if ( dp->pubmethods != 0 )
        free_json(dp->pubmethods);
    if ( dp->authmethods != 0 )
        free_json(dp->authmethods);
    shutdown_plugsocks(&dp->perm), shutdown_plugsocks(&dp->wss);
    free(dp);
}

void update_Daemoninfos()
{
    static portable_mutex_t mutex; static int didinit;
    struct daemon_info *dp;
    double currentmilli = milliseconds();
    int32_t i,n;
    if ( didinit == 0 )
        portable_mutex_init(&mutex), didinit = 1;
    portable_mutex_lock(&mutex);
    for (i=n=0; i<Numdaemons; i++)
    {
        if ( (dp= Daemoninfos[i]) != 0 )
        {
            if ( dp->finished != 0 && currentmilli > (dp->lastsearch + DAEMONFREE_MARGIN_SECONDS*1000) )
            {
                printf("daemon.%llu finished\n",(long long)dp->daemonid);
                Daemoninfos[i] = 0;
                free_daemon_info(dp);
            }
            else
            {
                if ( dp->finished != 0 )
                    printf("waiting for DAEMONFREE_MARGIN_SECONDS for %s.%llu\n",dp->name,(long long)dp->daemonid);
                if ( i != n )
                    Daemoninfos[n] = dp;
                n++;
            }
        }
    }
    Numdaemons = n;
    while ( (dp= queue_dequeue(&DaemonQ,0)) != 0 )
    {
        printf("dequeued new daemon.(%s)\n",dp->name);
        Daemoninfos[Numdaemons++] = dp;
    }
    portable_mutex_unlock(&mutex);
}

struct daemon_info *find_daemoninfo(int32_t *indp,char *name,uint64_t daemonid,uint64_t instanceid)
{
    int32_t i,j,n;
    struct daemon_info *dp = 0;
    *indp = -1;
    if ( Numdaemons > 0 )
    {
        for (i=0; i<Numdaemons; i++)
        {
            if ( (dp= Daemoninfos[i]) != 0 && ((daemonid != 0 && dp->daemonid == daemonid) || (name != 0 && name[0] != 0 && strcmp(name,dp->name) == 0)) )
            {
                n = (sizeof(dp->instanceids)/sizeof(*dp->instanceids));
                if ( instanceid != 0 )
                {
                    for (j=0; j<n; j++)
                        if ( dp->instanceids[j] == instanceid )
                        {
                            *indp = j;
                            break;
                        }
                }
                dp->lastsearch = milliseconds();
                return(dp);
            }
        }
    }
    return(0);
}

int32_t set_first_plugin(char *plugin,char *method)
{
    int32_t i;
    struct daemon_info *dp = 0;
    if ( Numdaemons > 0 )
    {
        for (i=0; i<Numdaemons; i++)
        {
            if ( (dp= Daemoninfos[i]) != 0 && dp->methodsjson != 0 && in_jsonarray(dp->methodsjson,method) != 0 )
            {
                strcpy(plugin,dp->name);
                return(i);
            }
        }
    }
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

char *add_instanceid(struct daemon_info *dp,uint64_t instanceid)
{
    cJSON *json;
    char numstr[64],*jsonstr = 0;
    int32_t i;
    for (i=0; i<(sizeof(dp->instanceids)/sizeof(*dp->instanceids)); i++)
    {
        if ( dp->instanceids[i] == 0 )
        {
            dp->instanceids[i] = instanceid;
            break;
        }
        if ( dp->instanceids[i] == instanceid )
            return(0);
    }
    i = rand() % (sizeof(dp->instanceids)/sizeof(*dp->instanceids));
    dp->instanceids[i] = instanceid;
    json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"requestType",cJSON_CreateString("instances"));
    cJSON_AddItemToObject(json,"instanceids",instances_json(dp));
    sprintf(numstr,"%llu",(long long)dp->daemonid), cJSON_AddItemToObject(json,"daemonid",cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)instanceid), cJSON_AddItemToObject(json,"instanceid",cJSON_CreateString(numstr));
    jsonstr = cJSON_Print(json);
    _stripwhite(jsonstr,' ');
    free_json(json);
    return(jsonstr);
}

void process_plugin_message(struct daemon_info *dp,char *str,int32_t len)
{
    cJSON *json;
    struct relayargs *args = 0;
    int32_t permflag,broadcastflag;
    uint64_t instanceid,tag = 0;
    char request[8192],**dest,*retstr,*sendstr;
    if ( (json= cJSON_Parse(str)) != 0 )
    {
        //printf("READY.(%s) >>>>>>>>>>>>>> READY.(%s)\n",dp->name,dp->name);
        if ( get_API_int(cJSON_GetObjectItem(json,"allowremote"),0) > 0 )
            dp->allowremote = 1;
        permflag = get_API_int(cJSON_GetObjectItem(json,"permanentflag"),0);
        instanceid = get_API_nxt64bits(cJSON_GetObjectItem(json,"myid"));
        tag = get_API_nxt64bits(cJSON_GetObjectItem(json,"tag"));
        if ( dp->readyflag == 0 )
            printf("HOST: process_plugin_message.(%s) instanceid.%llu allowremote.%d\n",str,(long long)instanceid,dp->allowremote);
        dp->readyflag = 1;
        if ( permflag == 0 && instanceid != 0 )
        {
            if ( (sendstr= add_instanceid(dp,instanceid)) != 0 )
                nn_local_broadcast(&dp->perm.socks,instanceid,LOCALCAST,(uint8_t *)sendstr,(int32_t)strlen(sendstr)+1), dp->numsent++, free(sendstr);
        }
        copy_cJSON(request,cJSON_GetObjectItem(json,"pluginrequest"));
        if ( strcmp(request,"SuperNET") == 0 )
        {
            char *call_SuperNET_JSON(char *JSONstr);
            if ( (retstr= call_SuperNET_JSON(str)) != 0 )
            {
                if ( Debuglevel > 2 )
                    fprintf(stderr,"send return from (%s) <<<<<<<<<<<<<<<<<<<<<< \n",str);
                nn_local_broadcast(&dp->perm.socks,instanceid,0,(uint8_t *)retstr,(int32_t)strlen(retstr)+1), dp->numsent++;
                free(retstr);
            }
        }
        else if ( instanceid != 0 && (broadcastflag= get_API_int(cJSON_GetObjectItem(json,"broadcast"),0)) > 0 )
        {
            fprintf(stderr,"send to other <<<<<<<<<<<<<<<<<<<<< \n");
            nn_local_broadcast(&dp->perm.socks,instanceid,broadcastflag,(uint8_t *)str,len), dp->numsent++;
        }
        free_json(json);
    } else printf("parse error.(%s)\n",str);
    if ( tag != 0 )
    {
        if ( (dest= get_tagstr(&args,dp,tag)) != 0 )
            *dest = str;
        else
        {
            int32_t complete_relay(struct relayargs *args,char *retstr);
            if ( args != 0 )
                complete_relay(args,str);
            else printf("TAG.%llu -> no destination for.(%s)\n",(long long)tag,str);
            free(str);
        }
    }
    else if ( dp->websocket == 0 )
    {
        static FILE *fp;
        if ( fp == 0 )
            fp = fopen("msglog","wb");
        if ( fp != 0 )
        {
            fprintf(fp,"queue message.(%s)\n",str);
            fflush(fp);
            free(str);
        } else queue_enqueue("daemon",&dp->messages,queueitem(str));
    }
}

int32_t poll_daemons() // the only thread that is allowed to modify Daemoninfos[], it is called from the main thread
{
    static portable_mutex_t mutex;
    static int didinit,counter=0;
    int32_t timeoutmillis,processed=0,i,n = 0;
    char *messages[16],*str;
    struct daemon_info *dp;
    timeoutmillis = 1;
    if ( didinit == 0 )
        portable_mutex_init(&mutex), didinit = 1;
    portable_mutex_lock(&mutex);
    if ( Numdaemons > 0 )
    {
        counter++;
        for (i=0; i<Numdaemons; i++)
        {
            if ( (dp= Daemoninfos[i]) != 0 && dp->finished == 0 )
            {
                if ( (n= poll_local_endpoints(messages,&dp->numrecv,dp->numsent,(counter&1)?&dp->perm:&dp->wss,timeoutmillis)) > 0 )
                {
                    for (i=0; i<n; i++,processed++)
                    {
                        str = messages[i];
                        if ( Debuglevel > 1 )
                            printf("(%d %d) %d %.6f HOST RECEIVED.%d i.%d/%d (%s) FROM (%s) %llu >>>>>>>>>>>>>>\n",dp->numrecv,dp->numsent,processed,milliseconds(),n,i,Numdaemons,str,dp->cmd,(long long)dp->daemonid);
                        process_plugin_message(dp,str,(int32_t)strlen(str)+1);
                        //free(str);
                    }
                }
            }
        }
    }
    update_Daemoninfos();
    portable_mutex_unlock(&mutex);
    return(processed);
}

int32_t call_system(struct daemon_info *dp,int32_t permanentflag,char *cmd,char *jsonargs)
{
    char *args[8],cmdstr[1024],daemonstr[64],portstr[8192],flagstr[3];
    int32_t i,n = 0;
    if ( dp == 0 )
    {
        if ( jsonargs != 0 && jsonargs[0] != 0 )
        {
            sprintf(cmdstr,"%s \"%s\"",cmd,jsonargs);
            if ( system(os_compatible_path(cmdstr)) != 0 )
                printf("error SYSTEM.(%s)\n",cmdstr);
        }
        else if ( system(os_compatible_path(cmd)) != 0 )
            printf("error SYSTEM.(%s) cmd\n",cmd);
        return(0);
    } else cmd = dp->cmd;
    memset(args,0,sizeof(args));
    if ( dp->websocket != 0 && permanentflag == 0 )
    {
        args[n++] = SUPERNET.WEBSOCKETD;
        sprintf(portstr,"--port=%d",dp->websocket), args[n++] = portstr;
        if ( Debuglevel > 0 )
            args[n++] = "--devconsole";
    }
    args[n++] = dp->cmd;
    for (i=1; cmd[i]!=0; i++)
        if ( cmd[i] == ' ' )
            break;
    if ( cmd[i] != 0 && cmd[i+1] != 0 )
        cmd[i] = 0, args[n++] = &cmd[i+1];
    sprintf(flagstr,"%d",permanentflag);
    args[n++] = flagstr;
    if ( dp->daemonid != 0 )
    {
        sprintf(daemonstr,"%llu",(long long)dp->daemonid);
        args[n++] = daemonstr;
    }
    if ( dp->jsonargs != 0 && dp->jsonargs[0] != 0 )
        args[n++] = dp->jsonargs;
    args[n++] = 0;
    if ( dp->bundledflag != 0 && permanentflag != 0 && dp->websocket == 0 )
    {
        int32_t ramchain_main(int32_t,char *args[]);
        int32_t MGW_main(int32_t,char *args[]);
        int32_t db777_main(int32_t,char *args[]);
        int32_t SuperNET_main(int32_t,char *args[]);
        int32_t coins_main(int32_t,char *args[]);
        int32_t peers_main(int32_t,char *args[]);
        int32_t subscriptions_main(int32_t,char *args[]);
        int32_t relay_main(int32_t,char *args[]);
        if ( strcmp(dp->name,"coins") == 0 ) return(coins_main(n,args));
        else if ( strcmp(dp->name,"db777") == 0 ) return(db777_main(n,args));
        else if ( strcmp(dp->name,"relay") == 0 ) return(relay_main(n,args));
        else if ( strcmp(dp->name,"peers") == 0 ) return(peers_main(n,args));
        else if ( strcmp(dp->name,"subscriptions") == 0 ) return(subscriptions_main(n,args));
        else if ( strcmp(dp->name,"ramchain") == 0 ) return(ramchain_main(n,args));
        else if ( strcmp(dp->name,"MGW") == 0 ) return(MGW_main(n,args));
        else if ( strcmp(dp->name,"SuperNET") == 0 ) return(SuperNET_main(n,args));
        else return(-1);
    }
    else return(OS_launch_process(args));
}

int32_t is_bundled_plugin(char *plugin)
{
    if ( strcmp(plugin,"SuperNET") == 0 || strcmp(plugin,"db777") == 0 || strcmp(plugin,"coins") == 0  || strcmp(plugin,"ramchain") == 0  || strcmp(plugin,"MGW") == 0 || strcmp(plugin,"peers") == 0 || strcmp(plugin,"relay") == 0 || strcmp(plugin,"subscriptions") == 0 )
        return(1);
    else return(0);
}

void *_daemon_loop(struct daemon_info *dp,int32_t permanentflag)
{
    char bindaddr[512],connectaddr[512];
    int32_t childpid,status;
    dp->bundledflag = is_bundled_plugin(dp->name);
    set_bind_transport(bindaddr,dp->bundledflag,permanentflag,dp->ipaddr,dp->port,dp->daemonid);
    set_connect_transport(connectaddr,dp->bundledflag,permanentflag,dp->ipaddr,dp->port,dp->daemonid);
    init_pluginhostsocks(dp,permanentflag,bindaddr,connectaddr,dp->myid);
    if ( Debuglevel > 2 )
        printf("<<<<<<<<<<<<<<<<<< %s plugin.(%s) bind.(%s) connect.(%s)\n",permanentflag!=0?"PERMANENT":"WEBSOCKETD",dp->name,bindaddr,connectaddr);
    childpid = (*dp->daemonfunc)(dp,permanentflag,0,0);
    OS_waitpid(childpid,&status,0);
    printf("daemonid.%llu (%s %s) finished child.%d status.%d\n",(long long)dp->daemonid,dp->cmd,dp->jsonargs!=0?dp->jsonargs:"",childpid,status);
    if ( permanentflag != 0 )
    {
        printf("daemonid.%llu (%s %s) finished\n",(long long)dp->daemonid,dp->cmd,dp->jsonargs!=0?dp->jsonargs:"");
        dp->finished = 1;
    }
    return(0);
}

void *daemon_loop(void *args) // launch websocketd server
{
    struct daemon_info *dp = args;
    return(_daemon_loop(dp,0));
}

void *daemon_loop2(void *args) // launch permanent plugin process
{
    struct daemon_info *dp = args;
    return(_daemon_loop(dp,1));
}

char *launch_daemon(char *plugin,char *ipaddr,uint16_t port,int32_t websocket,char *cmd,char *arg,int32_t (*daemonfunc)(struct daemon_info *dp,int32_t permanentflag,char *cmd,char *jsonargs))
{
    struct daemon_info *dp;
    char retbuf[1024]; int32_t i,delim,offset=0;
    //printf("launch daemon.(%s)\n",plugin);
    if ( Numdaemons >= sizeof(Daemoninfos)/sizeof(*Daemoninfos) )
        return(clonestr("{\"error\":\"too many daemons, cant create anymore\"}"));
    dp = calloc(1,sizeof(*dp));
    if ( (dp->bundledflag= is_bundled_plugin(plugin)) != 0 && websocket != 0 )
        return(clonestr("{\"error\":\"cant websocket built in\"}"));
    dp->port = port;
    if ( ipaddr != 0 )
        strcpy(dp->ipaddr,ipaddr);
    randombytes((uint8_t *)&dp->myid,sizeof(dp->myid));
    dp->cmd = clonestr(cmd!=0&&cmd[0]!=0?cmd:plugin);
#ifdef WIN32
    delim = '\\';
#else
    delim = '/';
#endif
    if ( plugin != 0 )
    {
        for (i=offset=0; plugin[i]!=0; i++)
            if ( plugin[i] == delim )
                offset = i+1;
        strcpy(dp->name,plugin+offset);
    }
    dp->daemonid = (uint64_t)(milliseconds() * 1000000) & (~(uint64_t)3) ^ *(int32_t *)plugin;
    memset(&dp->perm,0xff,sizeof(dp->perm));
    memset(&dp->wss,0xff,sizeof(dp->wss));
    dp->jsonargs = (arg != 0 && arg[0] != 0) ? clonestr(arg) : 0;
    dp->daemonfunc = daemonfunc;
    dp->websocket = websocket;
    queue_enqueue("DaemonQ",&DaemonQ,&dp->DL);
    if ( portable_thread_create((void *)daemon_loop2,dp) == 0 || (dp->bundledflag == 0 && websocket != 0 && portable_thread_create((void *)daemon_loop,dp) == 0) )
    {
        free_daemon_info(dp);
        return(clonestr("{\"error\":\"portable_thread_create couldnt create daemon\"}"));
    }
    sprintf(retbuf,"{\"result\":\"launched\",\"daemonid\":\"%llu\"}\n",(long long)dp->daemonid);
    return(clonestr(retbuf));
 }

char *language_func(char *plugin,char *ipaddr,uint16_t port,int32_t websocket,int32_t launchflag,char *cmd,char *jsonargs,int32_t (*daemonfunc)(struct daemon_info *dp,int32_t permanentflag,char *cmd,char *jsonargs))
{
    char buffer[65536] = { 0 };
    int out_pipe[2],saved_stdout;
    if ( launchflag != 0 || websocket != 0 )
        return(launch_daemon(plugin,ipaddr,port,websocket,cmd,jsonargs,daemonfunc));
    saved_stdout = dup(STDOUT_FILENO);
    if( pipe(out_pipe) != 0 )
        return(clonestr("{\"error\":\"pipe creation error\"}"));
    dup2(out_pipe[1],STDOUT_FILENO);
    close(out_pipe[1]);
    (*daemonfunc)(0,0,cmd,jsonargs);
    fflush(stdout);
    if ( read(out_pipe[0],buffer,sizeof(buffer)-1) <= 0 )
        printf("error reading in language_function\n");
    buffer[sizeof(buffer)-1] = 0;
    dup2(saved_stdout,STDOUT_FILENO);
    return(clonestr(buffer));
}

char *register_daemon(char *plugin,uint64_t daemonid,uint64_t instanceid,cJSON *methodsjson,cJSON *pubmethods,cJSON *authmethods)
{
    struct daemon_info *dp;
    char retbuf[8192],*methodstr,*authmethodstr,*pubmethodstr;
    int32_t ind;
    printf("register.(%s)\n",plugin);
    update_Daemoninfos();
    if ( (dp= find_daemoninfo(&ind,plugin,daemonid,instanceid)) != 0 )
    {
        if ( plugin[0] == 0 || strcmp(dp->name,plugin) == 0 )
        {
            if ( methodsjson != 0 )
            {
                dp->methodsjson = cJSON_Duplicate(methodsjson,1);
                methodstr = cJSON_Print(dp->methodsjson);
            } else methodstr = clonestr("[]");
            if ( methodsjson != 0 )
            {
                dp->pubmethods = cJSON_Duplicate(pubmethods,1);
                pubmethodstr = cJSON_Print(dp->pubmethods);
            } else pubmethodstr = clonestr("[]");
            if ( authmethods != 0 )
            {
                dp->authmethods = cJSON_Duplicate(authmethods,1);
                authmethodstr = cJSON_Print(dp->authmethods);
            } else authmethodstr = clonestr("[]");
            sprintf(retbuf,"{\"result\":\"registered\",\"plugin\":\"%s\",\"daemonid\":%llu,\"instanceid\":%llu,\"allowremote\":%d,\"methods\":%s,\"pubmethods\":%s,\"authmethods\":%s}",plugin,(long long)daemonid,(long long)instanceid,dp->allowremote,methodstr,pubmethodstr,authmethodstr);
            free(methodstr), free(pubmethodstr), free(authmethodstr);
            printf("register_daemon READY.(%s) >>>>>>>>>>>>>> READY.(%s)\n",dp->name,dp->name);
            dp->readyflag = 1;
            return(clonestr(retbuf));
        } else return(clonestr("{\"error\":\"plugin name mismatch\"}"));
    }
    return(clonestr("{\"error\":\"cant register inactive plugin\"}"));
}

char *plugin_method(char **retstrp,int32_t localaccess,char *plugin,char *method,uint64_t daemonid,uint64_t instanceid,char *origargstr,int32_t len,int32_t timeout)
{
    struct daemon_info *dp;
    char retbuf[8192],methodbuf[1024],*str,*methodsstr,*retstr = 0;
    uint64_t tag;
    cJSON *json;
    struct relayargs *args = 0;
    int32_t ind,async;
    async = (timeout == 0 || retstrp != 0);
    if ( retstrp == 0 )
        retstrp = &retstr;
    if ( localaccess < 0 )// && strcmp(previpaddr,"remote") == 0 )
    {
        localaccess = 0;
        args = (struct relayargs *)method, method = 0, methodbuf[0] = 0;
        if ( (json= cJSON_Parse(origargstr)) != 0 )
            copy_cJSON(methodbuf,cJSON_GetObjectItem(json,"method"));
        method = methodbuf;
    }
    if ( (dp= find_daemoninfo(&ind,plugin,daemonid,instanceid)) == 0 )
    {
        if ( is_bundled_plugin(plugin) != 0 )
        {
            language_func((char *)plugin,"",0,0,1,(char *)plugin,origargstr,call_system);
            return(clonestr("{\"error\":\"cant find plugin, AUTOLOAD\"}"));
        }
        return(clonestr("{\"error\":\"cant find plugin\"}"));
    }
    else
    {
        fprintf(stderr,"PLUGINMETHOD.(%s) for (%s) bundled.%d ready.%d allowremote.%d localaccess.%d\n",method,plugin,is_bundled_plugin(plugin),dp->readyflag,dp->allowremote,localaccess);
        if ( dp->readyflag == 0 )
        {
            printf("readyflag.%d\n",dp->readyflag);
            return(clonestr("{\"error\":\"plugin not ready\"}"));
        }
        if ( localaccess == 0 && dp->allowremote == 0 )
        {
            printf("allowremote.%d isremote.%d\n",dp->allowremote,!localaccess);
            sprintf(retbuf,"{\"error\":\"cant remote call plugin\",\"ipaddr\":\"%s\",\"plugin\":\"%s\"}",SUPERNET.myipaddr,plugin);
            return(clonestr(retbuf));
        }
        else if ( in_jsonarray(localaccess != 0 ? dp->methodsjson : dp->pubmethods,method) == 0 )
        {
            methodsstr = cJSON_Print(localaccess != 0 ? dp->methodsjson : dp->pubmethods);
           // if ( Debuglevel > 2 )
                fprintf(stderr,"available methods.(%s)\n",methodsstr);
            sprintf(retbuf,"{\"error\":\"method not allowed\",\"plugin\":\"%s\",\"%s\":\"%s\"}",plugin,method,methodsstr);
            free(methodsstr);
            return(clonestr(retbuf));
        }
        else
        {
            *retstrp = 0;
            if ( (tag= send_to_daemon(args,async==0?retstrp:0,dp->name,daemonid,instanceid,origargstr,len)) == 0 )
            {
                printf("null tag from send_to_daemon\n");
                return(clonestr("{\"error\":\"null tag from send_to_daemon\"}"));
            }
            else if ( async != 0 )
                return(0);//override == 0 ? clonestr("{\"error\":\"request sent to plugin async\"}") : 0);
            if ( ((*retstrp)= wait_for_daemon(retstrp,tag,timeout,10)) == 0 || (*retstrp)[0] == 0 )
            {
                str = stringifyM(origargstr);
                sprintf(retbuf,"{\"error\":\"\",\"args\":%s}",str);
                free(str);
                return(clonestr(retbuf));
            }
        }
        return(*retstrp);
    }
}

#endif

