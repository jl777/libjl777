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
#include "pubsub.h"
#include "pipeline.h"
#include "reqrep.h"
#include "survey.h"
#include "pair.h"
#include "pubsub.h"

#include "cJSON.h"
#include "system777.c"

struct plugin_info
{
    char bindaddr[64],connectaddr[64],ipaddr[64],name[64],retbuf[8192],registerbuf[8192];
    uint64_t daemonid,myid;
    union endpoints all;
    uint32_t permanentflag,ppid,transportid,extrasize,timeout,numrecv,numsent,bundledflag,registered;
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


static int32_t init_pluginsocks(struct plugin_info *plugin,int32_t permanentflag,char *bindaddr,char *connectaddr,uint64_t instanceid,uint64_t daemonid,int32_t timeout)
{
    int32_t errs = 0;
    struct allendpoints *socks = &plugin->all.socks;
    printf("<<<<<<<<<<<<< init_permpairsocks bind.(%s) connect.(%s)\n",bindaddr,connectaddr);
    //if ( plugin->bundledflag != 0 && (socks->both.pair= init_socket(".pair","pair",NN_PAIR,0,connectaddr,timeout)) < 0 ) errs++;
    if ( plugin->bundledflag != 0 && (socks->both.bus= init_socket("","bus",NN_BUS,0,connectaddr,timeout)) < 0 ) errs++;
    //if ( (socks->send.push= init_socket(".pipeline","push",NN_PUSH,bindaddr,0,timeout)) < 0 ) errs++;
    //if ( (socks->send.rep= init_socket(".reqrep","rep",NN_REP,bindaddr,connectaddr,timeout)) < 0 ) errs++;
    //if ( (socks->send.pub= init_socket(".pubsub","pub",NN_PUB,bindaddr,0,timeout)) < 0 ) errs++;
    //if ( (socks->send.survey= init_socket(".survey","surveyor",NN_SURVEYOR,bindaddr,0,timeout)) < 0 ) errs++;
    //if ( (socks->recv.pull= init_socket(".pipeline","pull",NN_PULL,0,connectaddr,0)) < 0 ) errs++;
    //if ( (socks->recv.req= init_socket(".reqrep","req",NN_REQ,0,connectaddr,timeout)) < 0 ) errs++;
    //if ( (socks->recv.sub= init_socket(".pubsub","sub",NN_SUB,0,connectaddr,0)) < 0 ) errs++;
    //if ( (socks->recv.respond= init_socket(".survey","respondent",NN_RESPONDENT,0,connectaddr,0)) < 0 ) errs++;
    return(errs);
}

static int32_t process_json(struct plugin_info *plugin,char *jsonargs,int32_t initflag)
{
    char ipaddr[MAX_JSON_FIELD],*jsonstr = 0;
    cJSON *obj=0,*json = 0;
    uint16_t port;
    uint64_t tag = 0;
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
    if ( obj != 0 )
    {
        tag = get_API_nxt64bits(cJSON_GetObjectItem(obj,"port"));
        if ( initflag > 0 )
        {
            if ( (port = get_API_int(cJSON_GetObjectItem(obj,"port"),0)) != 0 )
            {
                copy_cJSON(ipaddr,cJSON_GetObjectItem(obj,"ipaddr"));
                if ( ipaddr[0] != 0 )
                    strcpy(plugin->ipaddr,ipaddr), plugin->port = port;
                fprintf(stderr,"Set ipaddr (%s:%d)\n",plugin->ipaddr,plugin->port);
            }
        }
    }
    fprintf(stderr,"tag.%llu initflag.%d got jsonargs.(%p) %p %p\n",(long long)tag,initflag,jsonargs,jsonstr,obj);
    if ( jsonstr != 0 && obj != 0 )
        retval = PLUGNAME(_process_json)(plugin,tag,plugin->retbuf,sizeof(plugin->retbuf)-1,jsonstr,obj,initflag);
    if ( jsonstr != 0 )
        free(jsonstr);
    if ( json != 0 )
        free_json(json);
    printf("%s\n",plugin->retbuf), fflush(stdout);
    return(retval);
}

static void append_stdfields(struct plugin_info *plugin,uint64_t tag)
{
    char tagstr[64];
    if ( tag != 0 )
        sprintf(tagstr,",\"tag\":%llu",(long long)tag);
    else tagstr[0] = 0;
    sprintf(plugin->retbuf+strlen(plugin->retbuf)-1,",\"permanentflag\":%d,\"myid\":\"%llu\",\"plugin\":\"%s\",\"endpoint\":\"%s\",\"millis\":%.2f,\"sent\":%u,\"recv\":%u%s}",plugin->permanentflag,(long long)plugin->myid,plugin->name,plugin->bindaddr[0]!=0?plugin->bindaddr:plugin->connectaddr,milliseconds(),plugin->numsent,plugin->numrecv,tagstr);
}

static int32_t registerAPI(struct plugin_info *plugin)
{
    cJSON *json,*array;
    char numstr[64],*jsonstr;
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
    sprintf(numstr,"%llu",plugin->myid), cJSON_AddItemToObject(json,"myid",cJSON_CreateString(numstr));
    cJSON_AddItemToObject(json,"methods",array);
    jsonstr = cJSON_Print(json), free_json(json);
    _stripwhite(jsonstr,' ');
    strcpy(plugin->retbuf,jsonstr), free(jsonstr);
    append_stdfields(plugin,0);
    printf(">>>>>>>>>>> ret.(%s)\n",plugin->retbuf);
    return((int32_t)strlen(plugin->retbuf));
}

static void configure_plugin(struct plugin_info *plugin,char *jsonargs,int32_t initflag)
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

static void plugin_transportaddr(char *addr,char *transportstr,char *ipaddr,uint64_t num)
{
    if ( ipaddr != 0 )
        sprintf(addr,"%s://%s:%llu",transportstr,ipaddr,(long long)num);
    else sprintf(addr,"%s://%llu",transportstr,(long long)num);
}

static int32_t process_plugin_json(int32_t *sendflagp,struct plugin_info *plugin,int32_t permanentflag,uint64_t daemonid,uint64_t myid,char *jsonstr)
{
    int32_t len = (int32_t)strlen(jsonstr);
    cJSON *json,*obj;
    uint64_t tag = 0;
    char name[MAX_JSON_FIELD];
    plugin->retbuf[0] = *sendflagp = 0;
    //printf("process_plugin_json.(%s)\n",plugin->name);
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        if ( is_cJSON_Array(json) != 0 )
            obj = cJSON_GetArrayItem(json,0);
        else obj = json;
        copy_cJSON(name,cJSON_GetObjectItem(obj,"plugin"));
        tag = get_API_nxt64bits(cJSON_GetObjectItem(obj,"tag"));
        if ( strcmp(name,plugin->name) == 0 && (len= PLUGNAME(_process_json)(plugin,tag,plugin->retbuf,(int32_t)sizeof(plugin->retbuf)-1,jsonstr,obj,0)) > 0 )
        {
            *sendflagp = 1;
            return(len);
        } else printf("no return.%d (%s)\n",strcmp(name,plugin->name),name);
    }
    else
    {
        //printf("couldnt parse.(%s)\n",jsonstr);
        if ( jsonstr[len-1] == '\r' || jsonstr[len-1] == '\n' || jsonstr[len-1] == '\t' || jsonstr[len-1] == ' ' )
            jsonstr[--len] = 0;
        if ( strcmp(jsonstr,"getpeers") == 0 )
            sprintf(plugin->retbuf,"{\"pluginrequest\":\"SuperNET\",\"requestType\":\"getpeers\"}"), *sendflagp = 1;
        else sprintf(plugin->retbuf,"{\"result\":\"unparseable\",\"message\":\"%s\"}",jsonstr);
    }
    if ( *sendflagp != 0 && plugin->retbuf[0] != 0 )
        append_stdfields(plugin,tag);
    else plugin->retbuf[0] = *sendflagp = 0;
    return((int32_t)strlen(plugin->retbuf));
}

#ifdef BUNDLED
int32_t PLUGNAME(_main)
#else
int32_t main
#endif
(int argc,const char *argv[])
{
    struct plugin_info *plugin = calloc(1,sizeof(*plugin));
    int32_t i,n,bundledflag,sendflag,len = 0;
    char *messages[16],*line,*jsonargs,*transportstr;
    milliseconds();
    plugin->ppid = OS_getppid();
    strcpy(plugin->name,PLUGINSTR);
    {
        int32_t i;
        for (i=0; i<argc; i++)
            printf("(%s) ",argv[i]);
        printf("argc.%d\n",argc);
    }
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
    memset(&plugin->all,0xff,sizeof(plugin->all));
    plugin->bundledflag = bundledflag = is_bundled_plugin(plugin->name);
    transportstr = get_localtransport(plugin->bundledflag);
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
            plugin_transportaddr(plugin->bindaddr,"tcp",plugin->ipaddr,plugin->port + 1*OFFSET_ENABLED);
        } else plugin_transportaddr(plugin->bindaddr,transportstr,0,plugin->daemonid + 1*OFFSET_ENABLED);
    } else plugin_transportaddr(plugin->bindaddr,transportstr,0,plugin->daemonid + 1*OFFSET_ENABLED);
    plugin_transportaddr(plugin->connectaddr,transportstr,0,plugin->daemonid+2*OFFSET_ENABLED);
    jsonargs = (argc >= 3) ? (char *)argv[3] : 0;
    configure_plugin(plugin,jsonargs,1);
    printf("argc.%d: %s myid.%llu daemonid.%llu args.(%s)\n",argc,plugin->permanentflag != 0 ? "PERMANENT" : "WEBSOCKET",(long long)plugin->myid,(long long)plugin->daemonid,jsonargs!=0?jsonargs:"");
    if ( init_pluginsocks(plugin,plugin->permanentflag,plugin->bindaddr,plugin->connectaddr,plugin->myid,plugin->daemonid,plugin->timeout) == 0 )
        if ( (len= registerAPI(plugin)) > 0 )
        {
            strcpy(plugin->registerbuf,plugin->retbuf);
            if ( Debuglevel > 2 )
                fprintf(stderr,">>>>>>>>>>>>>>> plugin sends REGISTER SEND.(%s)\n",plugin->registerbuf);
            nn_broadcast(&plugin->all.socks,0,0,(uint8_t *)plugin->registerbuf,(int32_t)strlen(plugin->registerbuf)+1), plugin->numsent++;
            //nn_send(plugin->sock,plugin->registerbuf,len+1,0); // send the null terminator too
        }
    while ( OS_getppid() == plugin->ppid )
    {
        plugin->retbuf[0] = 0;
        if ( (n= get_newinput(messages,&plugin->numrecv,plugin->numsent,plugin->permanentflag,&plugin->all,plugin->timeout)) > 0 )
        {
            for (i=0; i<n; i++)
            {
                line = messages[i], len = (int32_t)strlen(line);
                if ( Debuglevel > 2 )
                    printf("(s%d r%d) <<<<<<<<<<<<<< RECEIVED (%s).%d -> bind.(%s) connect.(%s) %s\n",plugin->numsent,plugin->numrecv,line,len,plugin->bindaddr,plugin->connectaddr,plugin->permanentflag != 0 ? "PERMANENT" : "WEBSOCKET"), fflush(stdout);
                if ( (len= process_plugin_json(&sendflag,plugin,plugin->permanentflag,plugin->daemonid,plugin->myid,line)) > 0 )
                {
                    if ( plugin->bundledflag == 0 )
                        printf("%s\n",plugin->retbuf), fflush(stdout);
                    if ( sendflag != 0 )
                    {
                        nn_broadcast(&plugin->all.socks,0,0,(uint8_t *)plugin->retbuf,len+1), plugin->numsent++;
                        if ( Debuglevel > 2 )
                            fprintf(stderr,">>>>>>>>>>>>>> returned.(%s)\n",plugin->retbuf);
                        //nn_send(plugin->sock,plugin->retbuf,len+1,0); // send the null terminator too
                    }
                }
                free(line);
            }
        }
        usleep(10);
    } fprintf(stderr,"ppid.%d changed to %d\n",plugin->ppid,OS_getppid());
    PLUGNAME(_shutdown)(plugin,len); // rc == 0 -> parent process died
    shutdown_plugsocks(&plugin->all);
    free(plugin);
    return(len);
}

#endif
#endif
