/**********************************************************************************
 * The MIT License (MIT)                                                          *
 *                                                                                *
 * Copyright © 2014-2015 The SuperNET Developers.                                 *
 *                                                                                *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy  *
 *  of this software and associated documentation files (the "Software"), to deal *
 *  in the Software without restriction, including without limitation the rights  *
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell     *
 *  copies of the Software, and to permit persons to whom the Software is         *
 *  furnished to do so, subject to the following conditions:                      *
 *                                                                                *
 *  The above copyright notice and this permission notice shall be included in    *
 *  all copies or substantial portions of the Software.                           *
 *                                                                                *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR    *
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,      *
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE   *
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER        *
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, *
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN     *
 *  THE SOFTWARE.                                                                 *
 *                                                                                *
 * Removal or modification of this copyright notice is prohibited.                *
 *                                                                                *
 **********************************************************************************/

#ifndef xcode_plugins_h
#define xcode_plugins_h

#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "nonportable.h"

#define DAEMONFREE_MARGIN_SECONDS 13

struct daemon_info
{
    struct queueitem DL;
    queue_t messages;
    char name[64],ipaddr[64],*cmd,*jsonargs;
    cJSON *methodsjson,*pubmethods,*authmethods;
    double lastsearch;
    //union endpoints perm,wss;
    int32_t (*daemonfunc)(struct daemon_info *dp,int32_t permanentflag,char *cmd,char *jsonargs);
    uint64_t daemonid,myid,instanceids[256];
    uint32_t numsent,numrecv;
    int32_t lasti,finished,websocket,allowremote,bundledflag,readyflag,pushsock;//,pairsocks[256];
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
    //shutdown_plugsocks(&dp->perm), shutdown_plugsocks(&dp->wss);
    nn_shutdown(dp->pushsock,0);
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
    cJSON *json; int32_t permflag,sendlen,broadcastflag,retsock; uint64_t instanceid,tag = 0; char **dest,*retstr=0,*sendstr; struct destbuf request;
    if ( (json= cJSON_Parse(str)) != 0 )
    {
        //printf("READY.(%s) >>>>>>>>>>>>>> READY.(%s)\n",dp->name,dp->name);
        if ( juint(json,"allowremote") > 0 )
            dp->allowremote = 1;
        permflag = juint(json,"permanentflag");
        instanceid = get_API_nxt64bits(cJSON_GetObjectItem(json,"myid"));
        tag = get_API_nxt64bits(cJSON_GetObjectItem(json,"tag"));
        if ( dp->readyflag == 0 || Debuglevel > 2 )
            printf("HOST: process_plugin_message.(%s) instanceid.%llu allowremote.%d pushsock.%d\n",str,(long long)instanceid,dp->allowremote,dp->pushsock);
        dp->readyflag = 1;
        if ( permflag == 0 && instanceid != 0 )
        {
            if ( (sendstr= add_instanceid(dp,instanceid)) != 0 )
                nn_local_broadcast(dp->pushsock,instanceid,LOCALCAST,(uint8_t *)sendstr,(int32_t)strlen(sendstr)+1), dp->numsent++, free(sendstr);
        }
        copy_cJSON(&request,cJSON_GetObjectItem(json,"pluginrequest"));
        if ( strcmp(request.buf,"SuperNET") == 0 )
        {
            char *call_SuperNET_JSON(char *JSONstr);
            //fprintf(stderr,"processing pluginrequest.(%s)\n",str);
            if ( (retstr= call_SuperNET_JSON(str)) != 0 )
            {
                if ( Debuglevel > 2 )
                    fprintf(stderr,"send return from (%s) <<<<<<<<<<<<<<<<<<<<<< (%s) \n",str,retstr);
                //nn_local_broadcast(dp->pushsock,instanceid,0,(uint8_t *)retstr,(int32_t)strlen(retstr)+1), dp->numsent++;
                free(str), str = retstr, retstr = 0;
            }
        }
        else if ( instanceid != 0 && (broadcastflag= juint(json,"broadcast")) > 0 )
        {
            fprintf(stderr,"send to other <<<<<<<<<<<<<<<<<<<<< \n");
            nn_local_broadcast(dp->pushsock,instanceid,broadcastflag,(uint8_t *)str,(int32_t)strlen(str)+1), dp->numsent++;
        }
        free_json(json);
    } else printf("parse error.(%s)\n",str);
//printf("tag.%llu str.%p retstr.%p\n",(long long)tag,str,retstr);
    if ( tag != 0 )
    {
        if ( (dest= get_tagstr(&retsock,dp,tag)) != 0 )
            *dest = str;
        if ( retsock >= 0 && str != 0 )
        {
            if ( (sendlen= nn_send(retsock,str,(int32_t)strlen(str)+1,0)) != (int32_t)strlen(str)+1 )
                printf("sendlen.%d != len.%d (%s)\n",sendlen,len,str);
        }
        if ( dest == 0 )
            free(str);
    } else free(str);
}

int32_t poll_daemons()
{
    struct daemon_info *dp; int32_t ind,processed=0; char *msg; uint64_t daemonid,instanceid; cJSON *json;
    update_Daemoninfos();
    while ( nn_recv(SUPERNET.pullsock,&msg,NN_MSG,0) > 0 )
    {
        if ( (json= cJSON_Parse(msg)) != 0 )
        {
            daemonid = get_API_nxt64bits(cJSON_GetObjectItem(json,"daemonid"));
            instanceid = get_API_nxt64bits(cJSON_GetObjectItem(json,"myid"));
            if ( (dp= find_daemoninfo(&ind,0,daemonid,instanceid)) != 0 )
                process_plugin_message(dp,clonestr(msg),(int32_t)strlen(msg)+1);
            else printf("poll_daemons cant find daemonid.%llu instanceid.%llu (%s)\n",(long long)daemonid,(long long)instanceid,msg);
            free_json(json);
        } else printf("poll_daemons couldnt parse.(%s)\n",msg);
        nn_freemsg(msg);
        processed++;
    }
    return(processed);
}

int32_t call_system(struct daemon_info *dp,int32_t permanentflag,char *cmd,char *jsonargs)
{
    char *args[8],cmdstr[1024],daemonstr[64],portstr[8192],flagstr[3],pidstr[16];
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
        //args[n++] = "--sameorigin=true";
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
    sprintf(pidstr,"%d",OS_getpid());
    args[n++] = pidstr;
    args[n++] = 0;
    if ( dp->bundledflag != 0 && permanentflag != 0 && dp->websocket == 0 )
    {
        int32_t ramchain_main(int32_t,char *args[]);
        int32_t MGW_main(int32_t,char *args[]);
        int32_t kv777_main(int32_t,char *args[]);
        int32_t SuperNET_main(int32_t,char *args[]);
        int32_t coins_main(int32_t,char *args[]);
        int32_t relay_main(int32_t,char *args[]);
        int32_t InstantDEX_main(int32_t,char *args[]);
        int32_t dcnet_main(int32_t,char *args[]);
        int32_t prices_main(int32_t,char *args[]);
        int32_t shuffle_main(int32_t,char *args[]);
        //int32_t cashier_main(int32_t,char *args[]);
        if ( strcmp(dp->name,"coins") == 0 ) return(coins_main(n,args));
        else if ( strcmp(dp->name,"InstantDEX") == 0 ) return(InstantDEX_main(n,args));
        else if ( strcmp(dp->name,"prices") == 0 ) return(prices_main(n,args));
        else if ( strcmp(dp->name,"kv777") == 0 ) return(kv777_main(n,args));
        else if ( strcmp(dp->name,"relay") == 0 ) return(relay_main(n,args));
        else if ( strcmp(dp->name,"shuffle") == 0 ) return(shuffle_main(n,args));
        else if ( strcmp(dp->name,"SuperNET") == 0 ) return(SuperNET_main(n,args));
        else if ( strcmp(dp->name,"dcnet") == 0 ) return(dcnet_main(n,args));
        //else if ( strcmp(dp->name,"cashier") == 0 ) return(cashier_main(n,args));
        //else if ( strcmp(dp->name,"teleport") == 0 ) return(teleport_main(n,args));
#ifdef INSIDE_MGW
        else if ( strcmp(dp->name,"ramchain") == 0 ) return(ramchain_main(n,args));
        else if ( strcmp(dp->name,"MGW") == 0 ) return(MGW_main(n,args));
#endif
        else return(-1);
    }
    else return(OS_launch_process(args));
}

int32_t is_bundled_plugin(char *plugin)
{
    if ( strcmp(plugin,"InstantDEX") == 0 || strcmp(plugin,"SuperNET") == 0 || strcmp(plugin,"kv777") == 0 || strcmp(plugin,"coins") == 0 || strcmp(plugin,"relay") == 0 ||strcmp(plugin,"prices") == 0 || strcmp(plugin,"dcnet") == 0 || strcmp(plugin,"shuffle") == 0 ||
        //strcmp(plugin,"cashier") == 0 || strcmp(plugin,"teleport") == 0
#ifdef INSIDE_MGW
        strcmp(plugin,"ramchain") == 0 || strcmp(plugin,"MGW") == 0 ||
#endif
        0 )
        return(1);
    else return(0);
}

void *_daemon_loop(struct daemon_info *dp,int32_t permanentflag)
{
    char connectaddr[512];
    int32_t childpid,status;
    dp->bundledflag = is_bundled_plugin(dp->name);
    set_connect_transport(connectaddr,dp->bundledflag,permanentflag,dp->ipaddr,dp->port,dp->daemonid);
    init_pluginhostsocks(dp,connectaddr);
    if ( Debuglevel > 2 )
        printf("<<<<<<<<<<<<<<<<<< %s plugin.(%s) connect.(%s)\n",permanentflag!=0?"PERMANENT":"WEBSOCKETD",dp->name,connectaddr);
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
    //memset(&dp->perm,0xff,sizeof(dp->perm));
    //memset(&dp->wss,0xff,sizeof(dp->wss));
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
    FILE *fp;
    char buffer[65536] = { 0 };
    int out_pipe[2],saved_stdout;
    if ( is_bundled_plugin(plugin) == 0 )
    {
        if ( (fp= fopen(cmd,"rb")) == 0 )
            return(clonestr("{\"error\":\"no agent file\"}"));
        else fclose(fp);
    }
    printf("found file.(%s)\n",cmd);
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

char *plugin_method(int32_t sock,char **retstrp,int32_t localaccess,char *plugin,char *method,uint64_t daemonid,uint64_t instanceid,char *origargstr,int32_t len,int32_t timeout,char *tokenstr)
{
    struct daemon_info *dp; char retbuf[8192],*str,*methodsstr,*retstr; uint64_t tag; cJSON *json; int32_t ind,async;
    if ( Debuglevel > 2 )
        printf("inside plugin_method: localaccess.%d origargstr.(%s).%d retstrp.%p token.(%s)\n",localaccess,origargstr,len,retstrp,tokenstr!=0?tokenstr:"");
    async = (timeout == 0 || retstrp != 0);
    if ( retstrp == 0 )
        retstrp = &retstr;
    *retstrp = 0;
    if ( (dp= find_daemoninfo(&ind,plugin,daemonid,instanceid)) == 0 )
    {
        if ( is_bundled_plugin(plugin) != 0 )
        {
            if ( SUPERNET.iamrelay <= 1 )
                language_func((char *)plugin,"",0,0,1,(char *)plugin,origargstr,call_system);
            return(clonestr("{\"error\":\"cant find plugin, AUTOLOAD\"}"));
        }
        fprintf(stderr,"cant find.(%s)\n",plugin);
        return(clonestr("{\"error\":\"cant find plugin\"}"));
    }
    else
    {
        if ( Debuglevel > 2 )
            fprintf(stderr,">>>>>>> PLUGINMETHOD.(%s) for (%s) bundled.%d ready.%d allowremote.%d localaccess.%d retstrp.%p\n",method,plugin,is_bundled_plugin(plugin),dp->readyflag,dp->allowremote,localaccess,retstrp);
        if ( dp->readyflag == 0 )
        {
            fprintf(stderr,"%s readyflag.%d\n",dp->name,dp->readyflag);
            sprintf(retbuf,"{\"error\":\"plugin not ready\",\"daemonid\":\"%llu\",\"myid\":\"%llu\"}",(long long)dp->daemonid,(long long)dp->myid);
            return(clonestr(retbuf));
        }
        if ( localaccess == 0 && dp->allowremote == 0 )
        {
            fprintf(stderr,"allowremote.%d isremote.%d\n",dp->allowremote,!localaccess);
            sprintf(retbuf,"{\"error\":\"cant remote call plugin\",\"ipaddr\":\"%s\",\"plugin\":\"%s\",\"daemonid\":\"%llu\",\"myid\":\"%llu\"}",SUPERNET.myipaddr,plugin,(long long)dp->daemonid,(long long)dp->myid);
            return(clonestr(retbuf));
        }
        else if ( in_jsonarray(localaccess != 0 ? dp->methodsjson : dp->pubmethods,method) == 0 )
        {
            static uint32_t counter;
            methodsstr = cJSON_Print(localaccess != 0 ? dp->methodsjson : dp->pubmethods);
            if ( Debuglevel > 2 || counter++ < 3 )
                fprintf(stderr,"available.%s methods.(%s) vs (%s) orig.(%s)\n",plugin,methodsstr,method,origargstr);//, getchar();
            sprintf(retbuf,"{\"error\":\"method not allowed\",\"plugin\":\"%s\",\"%s\":\"%s\",\"daemonid\":\"%llu\",\"myid\":\"%llu\"}",plugin,method,methodsstr,(long long)dp->daemonid,(long long)dp->myid);
            free(methodsstr);
            return(clonestr(retbuf));
        }
        else
        {
            if ( Debuglevel > 2 )
                fprintf(stderr,"B send_to_daemon.(%s) sock.%d (%s).%d\n",dp->name,sock,origargstr,len);
            if ( (tag= send_to_daemon(sock,retstrp,dp->name,daemonid,instanceid,origargstr,len,localaccess,tokenstr)) == 0 )
            {
fprintf(stderr,"null tag from send_to_daemon\n");
                *retstrp = clonestr("{\"error\":\"null tag from send_to_daemon\"}");
                return(*retstrp);
            }
            else if ( async != 0 )
                return(0);
//fprintf(stderr,"wait_for_daemon retstrp.%p\n",retstrp);
            if ( ((*retstrp)= wait_for_daemon(retstrp,tag,timeout,10)) == 0 || (*retstrp)[0] == 0 )
            {
                if ( (json= cJSON_Parse(origargstr)) != 0 )
                {
                    cJSON_AddItemToObject(json,"result",cJSON_CreateString("submitted"));
                    str = cJSON_Print(json), _stripwhite(str,' ');
                    printf("timedout.(%s)\n",str);
                    free_json(json);
                    *retstrp = str;
                } else *retstrp = clonestr("{\"error\":\"cant parse command\"}");
            }
        }
        //fprintf(stderr,"return.(%s)\n",*retstrp!=0?*retstrp:"");
        return(*retstrp);
    }
}

#endif

