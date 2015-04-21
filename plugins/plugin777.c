//
//  plugin777.c
//  SuperNET API extension
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_plugin777_h
#define crypto777_plugin777_h

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <float.h>
#include <ctype.h>
#include "nn.h"
#include "bus.h"
#include "cJSON.h"
#include "system777.c"

struct plugin_info
{
    char bindaddr[64],connectaddr[64],ipaddr[64],name[64],retbuf[8192],registerbuf[8192];
    uint64_t daemonid,myid;
    uint32_t permanentflag,ppid,transportid,extrasize,timeout,counter;
    int32_t sock;
    uint16_t port;
    uint8_t pluginspace[];
};

#endif
#else
#ifndef crypto777_plugin777_c
#define crypto777_plugin777_c

#ifndef crypto777_plugin777_h
#define DEFINES_ONLY
#include __BASE_FILE__
#undef DEFINES_ONLY
#endif


int32_t get_socket_status(int32_t sock,int32_t timeoutmillis)
{
    struct nn_pollfd pfd;
    int32_t rc;
    pfd.fd = sock;
    pfd.events = NN_POLLIN | NN_POLLOUT;
    if ( (rc= nn_poll(&pfd,1,timeoutmillis)) == 0 )
        return(pfd.revents);
    else return(-1);
}

int32_t get_newinput(int32_t permanentflag,char *line,int32_t max,int32_t sock,int32_t timeoutmillis)
{
    int32_t rc,len;
    char *jsonstr = 0;
    line[0] = 0;
    if ( (permanentflag != 0 || ((rc= get_socket_status(sock,timeoutmillis)) > 0 && (rc & NN_POLLIN) != 0)) && (len= nn_recv(sock,&jsonstr,NN_MSG,0)) > 0 )
    {
        strncpy(line,jsonstr,max-1);
        line[max-1] = 0;
        nn_freemsg(jsonstr);
    }
    else if ( permanentflag == 0 )
        getline777(line,max);
    return((int32_t)strlen(line));
}

int32_t init_daemonsock(int32_t permanentflag,char *bindaddr,char *connectaddr,int32_t timeoutmillis)
{
    int32_t sock,err;
    if ( (sock= nn_socket(AF_SP,NN_BUS)) < 0 )
    {
        printf("error %d nn_socket err.%s\n",sock,nn_strerror(nn_errno()));
        return(-1);
    }
    if ( permanentflag != 0 )
    {
        if ( (err= nn_connect(sock,connectaddr)) < 0 )
        {
            printf("error %d nn_connect err.%s (%s to %s)\n",sock,nn_strerror(nn_errno()),permanentflag != 0 ? "PERMANENT" : "WEBSOCKET",connectaddr);
            return(-1);
        }
        printf("plugin >>>>>>>>>>>>>>> %d nn_connect (%s <-> %s)\n",sock,permanentflag != 0 ? "PERMANENT" : "WEBSOCKET",connectaddr);
    }
   // if ( permanentflag == 0 )
    {
        if ( (err= nn_bind(sock,bindaddr)) < 0 )
        {
            printf("error %d nn_bind.%d (%s) | %s\n",err,sock,bindaddr,nn_strerror(nn_errno()));
            return(-1);
        }
        printf("plugin >>>>>>>>>>>>>>> %d nn_bind %s bind.(%s) connect.(%s)\n",sock,permanentflag != 0 ? "PERMANENT" : "WEBSOCKET",bindaddr,connectaddr);
    }
    nn_setsockopt(sock,NN_SOL_SOCKET,NN_RCVTIMEO,&timeoutmillis,sizeof(timeoutmillis));
    return(sock);
}

void append_stdfields(struct plugin_info *plugin)
{
    sprintf(plugin->retbuf+strlen(plugin->retbuf)-1,",\"permanent\":%d,\"myid\":\"%llu\",\"plugin\":\"%s\",\"endpoint\":\"%s\",\"millis\":%f}",plugin->permanentflag,(long long)plugin->myid,plugin->name,plugin->bindaddr[0]!=0?plugin->bindaddr:plugin->connectaddr,milliseconds());
}

int32_t process_plugin_json(struct plugin_info *plugin,int32_t permanentflag,uint64_t daemonid,int32_t sock,uint64_t myid,char *jsonstr)
{
    int32_t len = (int32_t)strlen(jsonstr);
    cJSON *json;
    uint64_t sender;
    plugin->retbuf[0] = 0;
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        if ( (sender= get_API_nxt64bits(cJSON_GetObjectItem(json,"myid"))) != myid )
        {
            fprintf(stderr,"sender.%llu vs myid.%llu daemon.%llu\n",(long long)sender,(long long)myid,(long long)daemonid);
            if ( sender == daemonid || daemonid == plugin->daemonid )
            {
                if ( (len= PLUGNAME(_process_json)(plugin,plugin->retbuf,(int32_t)sizeof(plugin->retbuf)-1,jsonstr,json,0)) < 0 )
                    return(len);
            }
            else if ( sender != 0 )
                printf("process message from %llu: (%s)\n",(long long)sender,jsonstr), fflush(stdout);
        } else printf("gotack.(%s) %f\n",jsonstr,milliseconds()), fflush(stdout);
    }
    else
    {
        if ( jsonstr[len-1] == '\r' || jsonstr[len-1] == '\n' || jsonstr[len-1] == '\t' || jsonstr[len-1] == ' ' )
            jsonstr[--len] = 0;
        if ( strcmp(jsonstr,"getpeers") == 0 )
            sprintf(plugin->retbuf,"{\"pluginrequest\":\"SuperNET\",\"requestType\":\"getpeers\"}");
        else sprintf(plugin->retbuf,"{\"result\":\"unparseable\",\"message\":\"%s\"}",jsonstr);
    }
    if ( plugin->retbuf[0] != 0 )
        append_stdfields(plugin);
    return((int32_t)strlen(plugin->retbuf));
}

int32_t process_json(struct plugin_info *plugin,char *jsonargs,int32_t initflag)
{
    char ipaddr[MAX_JSON_FIELD],*jsonstr = 0;
    cJSON *obj,*json = 0;
    uint16_t port;
    int32_t retval = 0;
    if ( jsonargs != 0 )
    {
        json = cJSON_Parse(jsonargs);
        if ( is_cJSON_Array(json) != 0 && cJSON_GetArraySize(json) == 2 )
            obj = cJSON_GetArrayItem(json,0);
        else obj = json;
        jsonstr = cJSON_Print(obj);
        _stripwhite(jsonstr,' ');
    }
    if ( initflag > 0 && obj != 0 )
    {
        if ( (port = get_API_int(cJSON_GetObjectItem(obj,"port"),0)) != 0 )
        {
            copy_cJSON(ipaddr,cJSON_GetObjectItem(obj,"ipaddr"));
            if ( ipaddr[0] != 0 )
                strcpy(plugin->ipaddr,ipaddr), plugin->port = port;
            fprintf(stderr,"Set ipaddr (%s:%d)\n",plugin->ipaddr,plugin->port);
        }
    }
    fprintf(stderr,"initflag.%d got jsonargs.(%p) %p %p\n",initflag,jsonargs,jsonstr,obj);
    if ( jsonstr != 0 && obj != 0 )
        retval = PLUGNAME(_process_json)(plugin,plugin->retbuf,sizeof(plugin->retbuf)-1,jsonstr,obj,initflag);
    if ( jsonstr != 0 )
        free(jsonstr);
    if ( json != 0 )
        free_json(json);
    printf("%s\n",plugin->retbuf), fflush(stdout);
    return(retval);
}

int32_t registerAPI(struct plugin_info *plugin)
{
    cJSON *json,*array;
    char *jsonstr;
    int32_t i;
    uint64_t disableflags = 0;
    json = cJSON_CreateObject(), array = cJSON_CreateArray();
    plugin->retbuf[0] = 0;
    disableflags = PLUGNAME(_init)(plugin,(void *)plugin->pluginspace);
    for (i=0; i<(sizeof(PLUGNAME(_methods))/sizeof(*PLUGNAME(_methods))); i++)
    {
        if ( PLUGNAME(_methods)[i] == 0 || PLUGNAME(_methods)[i][0] == 0 )
            break;
        if ( ((1LL << i) & disableflags) == 0 )
            cJSON_AddItemToArray(array,cJSON_CreateString(PLUGNAME(_methods)[i]));
    }
    cJSON_AddItemToObject(json,"pluginrequest",cJSON_CreateString("SuperNET"));
    cJSON_AddItemToObject(json,"requestType",cJSON_CreateString("register"));
    cJSON_AddItemToObject(json,"methods",array);
    jsonstr = cJSON_Print(json), free_json(json);
    _stripwhite(jsonstr,' ');
    strcpy(plugin->retbuf,jsonstr), free(jsonstr);
    append_stdfields(plugin);
    printf(">>>>>>>>>>> ret.(%s)\n",plugin->retbuf);
    return((int32_t)strlen(plugin->retbuf));
}

void configure_plugin(struct plugin_info *plugin,char *jsonargs,int32_t initflag)
{
    if ( (plugin->extrasize= PLUGIN_EXTRASIZE) > 0 )
    {
        plugin = realloc(plugin,sizeof(*plugin) + plugin->extrasize);
        memset(plugin->pluginspace,0,plugin->extrasize);
    }
    if ( initflag != 0 )
        unstringify(jsonargs);
    process_json(plugin,jsonargs,initflag);
}

void plugin_transportaddr(char *addr,char *transportstr,char *ipaddr,uint64_t num)
{
    if ( ipaddr != 0 )
        sprintf(addr,"%s://%s:%llu",transportstr,ipaddr,(long long)num);
    else sprintf(addr,"%s://%llu",transportstr,(long long)num);
}

#ifdef BUNDLED
int32_t PLUGNAME(_main)
#else
int32_t main
#endif
(int argc,const char *argv[])
{
    struct plugin_info *plugin = calloc(1,sizeof(*plugin));
    int32_t len = 0;
    char line[8192],*jsonargs,*transportstr;
    milliseconds();
    plugin->ppid = OS_getppid();
    strcpy(plugin->name,PLUGINSTR);
    printf("%s (%s).argc%d parent PID.%d\n",plugin->name,argv[0],argc,plugin->ppid);
    plugin->timeout = 1;
    if ( argc <= 2 )
    {
        jsonargs = (argc > 1) ? (char *)argv[1]:"{}";
        configure_plugin(plugin,jsonargs,-1);
        //fprintf(stderr,"PLUGIN_RETURNS.[%s]\n",plugin->retbuf), fflush(stdout);
        return(0);
    }
    randombytes((uint8_t *)&plugin->myid,sizeof(plugin->myid));
    plugin->permanentflag = atoi(argv[1]);
    plugin->daemonid = atol(argv[2]);
    transportstr = get_bundled_plugin(plugin->name) != 0 ? "inproc" : "ipc";
    if ( plugin->permanentflag != 0 )
    {
        if ( plugin->ipaddr[0] != 0 || plugin->port != 0 )
        {
            if ( plugin->ipaddr[0] == 0 || plugin->port == 0 )
                plugin->port = wait_for_myipaddr(plugin->ipaddr);
        }
        if ( plugin->ipaddr[0] != 0 && plugin->port != 0 )
        {
            plugin->transportid = 'G';
            plugin_transportaddr(plugin->bindaddr,"tcp",plugin->ipaddr,plugin->port + 1);
        } else plugin_transportaddr(plugin->bindaddr,transportstr,0,plugin->daemonid + 1);
    } else plugin_transportaddr(plugin->bindaddr,transportstr,0,plugin->daemonid + 1);
    plugin_transportaddr(plugin->connectaddr,transportstr,0,plugin->daemonid);
    jsonargs = (argc >= 3) ? (char *)argv[3] : 0;
    configure_plugin(plugin,jsonargs,1);
    printf("argc.%d: %s myid.%llu daemonid.%llu args.(%s)\n",argc,plugin->permanentflag != 0 ? "PERMANENT" : "WEBSOCKET",(long long)plugin->myid,(long long)plugin->daemonid,jsonargs!=0?jsonargs:"");
    if ( (plugin->sock= init_daemonsock(plugin->permanentflag,plugin->bindaddr,plugin->connectaddr,plugin->timeout)) >= 0 )
    {
        if ( (len= registerAPI(plugin)) > 0 )//&& plugin->permanentflag != 0 ) // register supported API
        {
            strcpy(plugin->registerbuf,plugin->retbuf);
            nn_send(plugin->sock,plugin->retbuf,len+1,0); // send the null terminator too
        }
        while ( OS_getppid() == plugin->ppid )
        {
            if ( (len= get_newinput(plugin->permanentflag,line,sizeof(line),plugin->sock,plugin->timeout)) > 0 )
            {
                if ( line[len-1] == '\n' )
                    line[--len] = 0;
                plugin->counter++;
                printf("%d <<<<<<<<<<<<<< RECEIVED (%s).%d -> bind.(%s) connect.(%s) %s json.%p\n",plugin->counter,line,len,plugin->bindaddr,plugin->connectaddr,plugin->permanentflag != 0 ? "PERMANENT" : "WEBSOCKET",cJSON_Parse(line)), fflush(stdout);
                if ( (len= process_plugin_json(plugin,plugin->permanentflag,plugin->daemonid,plugin->sock,plugin->myid,line)) > 0 )
                {
                    printf("%s\n",plugin->retbuf), fflush(stdout);
                    fprintf(stderr,"return.(%s)\n",plugin->retbuf);
                    nn_send(plugin->sock,plugin->retbuf,len+1,0); // send the null terminator too
                } else if ( len < 0 )
                    break;
            }
            usleep(10);
        } fprintf(stderr,"ppid.%d changed to %d\n",plugin->ppid,OS_getppid());
        PLUGNAME(_shutdown)(plugin,len); // rc == 0 -> parent process died
        nn_shutdown(plugin->sock,0);
        free(plugin);
        return(len);
    } else printf("{\"error\":\"couldnt create socket\"}"), fflush(stdout);
    free(plugin);
    return(-1);
}

#endif
#endif
