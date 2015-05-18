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
    char bindaddr[64],connectaddr[64],ipaddr[64],name[64],NXTADDR[64];
    uint64_t daemonid,myid,nxt64bits;
    union endpoints all;
    uint32_t permanentflag,ppid,transportid,extrasize,timeout,numrecv,numsent,bundledflag,registered,sleepmillis,allowremote;
    uint16_t port;
    uint8_t pluginspace[];
};
int32_t plugin_result(char *retbuf,cJSON *json,uint64_t tag);

#endif
#else
#ifndef crypto777_plugin777_c
#define crypto777_plugin777_c

#ifndef crypto777_plugin777_h
#define DEFINES_ONLY
#include "plugin777.c"
#undef DEFINES_ONLY
#endif

static int32_t init_pluginsocks(struct plugin_info *plugin,int32_t permanentflag,char *bindaddr,char *connectaddr,uint64_t instanceid,uint64_t daemonid,int32_t timeout)
{
    int32_t errs = 0;
    struct allendpoints *socks = &plugin->all.socks;
    if ( Debuglevel > 2 )
        printf("%s.%p <<<<<<<<<<<<< init_permpairsocks bind.(%s) connect.(%s)\n",plugin->name,plugin,bindaddr,connectaddr);
    if ( (socks->both.pair= init_socket(".pair","pair",NN_PAIR,0,connectaddr,timeout)) < 0 ) errs++;
    //if ( (socks->both.bus= init_socket("","bus",NN_BUS,0,connectaddr,timeout)) < 0 ) errs++;
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

static int32_t process_json(char *retbuf,int32_t max,struct plugin_info *plugin,char *jsonargs,int32_t initflag)
{
    void *loadfile(uint64_t *allocsizep,char *fname);
    char filename[1024],*myipaddr,*jsonstr = 0;
    cJSON *obj=0,*tmp,*json = 0;
    uint64_t allocsize,nxt64bits,tag = 0;
    int32_t retval = 0;
    if ( jsonargs != 0 )
    {
        json = cJSON_Parse(jsonargs);
        if ( is_cJSON_Array(json) != 0 && cJSON_GetArraySize(json) == 2 )
            obj = cJSON_GetArrayItem(json,0);
        else obj = json;
        copy_cJSON(filename,cJSON_GetObjectItem(obj,"filename"));
        if ( filename[0] != 0 && (jsonstr= loadfile(&allocsize,filename)) != 0 )
        {
            if ( (tmp= cJSON_Parse(jsonstr)) != 0 )
                obj = tmp;
            else free(jsonstr), jsonstr = 0;
        }
        if ( jsonstr == 0 )
            jsonstr = cJSON_Print(obj);
        _stripwhite(jsonstr,' ');
    }
    if ( obj != 0 )
    {
        //printf("jsonargs.(%s)\n",jsonargs);
        if ( (nxt64bits = get_API_nxt64bits(cJSON_GetObjectItem(obj,"NXT"))) != 0 )
        {
            plugin->nxt64bits = nxt64bits;
            expand_nxt64bits(plugin->NXTADDR,plugin->nxt64bits);
        }
        tag = get_API_nxt64bits(cJSON_GetObjectItem(obj,"tag"));
        if ( initflag > 0 )
        {
            myipaddr = cJSON_str(cJSON_GetObjectItem(obj,"ipaddr"));
            if ( is_ipaddr(myipaddr) != 0 )
                strcpy(plugin->ipaddr,myipaddr);
            plugin->port = get_API_int(cJSON_GetObjectItem(obj,"port"),0);
        }
    }
    fprintf(stderr,"tag.%llu initflag.%d got jsonargs.(%p) %p %p\n",(long long)tag,initflag,jsonargs,jsonstr,obj);
    if ( jsonstr != 0 && obj != 0 )
        retval = PLUGNAME(_process_json)(plugin,tag,retbuf,max,jsonstr,obj,initflag);
    else printf("error with JSON.(%s)\n",jsonstr), getchar();
    fprintf(stderr,"done tag.%llu initflag.%d got jsonargs.(%p) %p %p\n",(long long)tag,initflag,jsonargs,jsonstr,obj);
    if ( jsonstr != 0 )
        free(jsonstr);
    if ( json != 0 )
        free_json(json);
    printf("%s\n",retbuf), fflush(stdout);
    return(retval);
}

static void append_stdfields(char *retbuf,int32_t max,struct plugin_info *plugin,uint64_t tag,int32_t allfields)
{
    char tagstr[128],*NXTaddr; cJSON *json;
    //printf("APPEND.(%s) (%s)\n",retbuf,plugin->name);
    if ( (json= cJSON_Parse(retbuf)) != 0 )
    {
        if ( tag != 0 && get_API_nxt64bits(cJSON_GetObjectItem(json,"tag")) == 0 )
            sprintf(tagstr,",\"tag\":\"%llu\"",(long long)tag);
        else tagstr[0] = 0;
        NXTaddr = cJSON_str(cJSON_GetObjectItem(json,"NXT"));
        if ( NXTaddr == 0 || NXTaddr[0] == 0 )
            NXTaddr = SUPERNET.NXTADDR;
        sprintf(retbuf+strlen(retbuf)-1,",\"NXT\":\"%s\"}",SUPERNET.NXTADDR);
        if ( allfields != 0 )
        {
             if ( SUPERNET.iamrelay != 0 )
                 sprintf(retbuf+strlen(retbuf)-1,",\"myipaddr\":\"%s\"}",plugin->ipaddr);
            sprintf(retbuf+strlen(retbuf)-1,",\"allowremote\":%d%s}",plugin->allowremote,tagstr);
            sprintf(retbuf+strlen(retbuf)-1,",\"permanentflag\":%d,\"myid\":\"%llu\",\"plugin\":\"%s\",\"endpoint\":\"%s\",\"millis\":%.2f,\"sent\":%u,\"recv\":%u}",plugin->permanentflag,(long long)plugin->myid,plugin->name,plugin->bindaddr[0]!=0?plugin->bindaddr:plugin->connectaddr,milliseconds(),plugin->numsent,plugin->numrecv);
         }
         else sprintf(retbuf+strlen(retbuf)-1,",\"allowremote\":%d%s}",plugin->allowremote,tagstr);
    }
}

static int32_t registerAPI(char *retbuf,int32_t max,struct plugin_info *plugin,cJSON *argjson)
{
    cJSON *json,*array;
    char *jsonstr;
    int32_t i;
    uint64_t disableflags = 0;
    json = cJSON_CreateObject();
    retbuf[0] = 0;
    disableflags = PLUGNAME(_register)(plugin,(void *)plugin->pluginspace,argjson);
    array = cJSON_CreateArray();
    for (i=0; i<(sizeof(PLUGNAME(_methods))/sizeof(*PLUGNAME(_methods))); i++)
    {
        if ( PLUGNAME(_methods)[i] == 0 || PLUGNAME(_methods)[i][0] == 0 )
            break;
        if ( ((1LL << i) & disableflags) == 0 )
            cJSON_AddItemToArray(array,cJSON_CreateString(PLUGNAME(_methods)[i]));
    }
    cJSON_AddItemToObject(json,"methods",array);
    array = cJSON_CreateArray();
    for (i=0; i<(sizeof(PLUGNAME(_pubmethods))/sizeof(*PLUGNAME(_pubmethods))); i++)
    {
        if ( PLUGNAME(_pubmethods)[i] == 0 || PLUGNAME(_pubmethods)[i][0] == 0 )
            break;
        if ( ((1LL << i) & disableflags) == 0 )
            cJSON_AddItemToArray(array,cJSON_CreateString(PLUGNAME(_pubmethods)[i]));
    }
    cJSON_AddItemToObject(json,"pubmethods",array);
    array = cJSON_CreateArray();
    for (i=0; i<(sizeof(PLUGNAME(_authmethods))/sizeof(*PLUGNAME(_authmethods))); i++)
    {
        if ( PLUGNAME(_authmethods)[i] == 0 || PLUGNAME(_authmethods)[i][0] == 0 )
            break;
        if ( ((1LL << i) & disableflags) == 0 )
            cJSON_AddItemToArray(array,cJSON_CreateString(PLUGNAME(_authmethods)[i]));
    }
    cJSON_AddItemToObject(json,"authmethods",array);
    cJSON_AddItemToObject(json,"pluginrequest",cJSON_CreateString("SuperNET"));
    cJSON_AddItemToObject(json,"requestType",cJSON_CreateString("register"));
    if ( plugin->sleepmillis == 0 )
        plugin->sleepmillis = get_API_int(cJSON_GetObjectItem(json,"sleepmillis"),SUPERNET.APISLEEP);
    cJSON_AddItemToObject(json,"sleepmillis",cJSON_CreateNumber(plugin->sleepmillis));
    jsonstr = cJSON_Print(json), free_json(json);
    _stripwhite(jsonstr,' ');
    strcpy(retbuf,jsonstr), free(jsonstr);
    append_stdfields(retbuf,max,plugin,0,1);
    if ( Debuglevel > 2 )
        printf(">>>>>>>>>>> ret.(%s)\n",retbuf);
    return((int32_t)strlen(retbuf));
}

static void configure_plugin(char *retbuf,int32_t max,struct plugin_info *plugin,char *jsonargs,int32_t initflag)
{
    if ( initflag != 0 && jsonargs != 0 && jsonargs[0] != 0 )
        unstringify(jsonargs);
    process_json(retbuf,max,plugin,jsonargs,initflag);
}

static void plugin_transportaddr(char *addr,char *transportstr,char *ipaddr,uint64_t num)
{
    if ( ipaddr != 0 )
        sprintf(addr,"%s://%s:%llu",transportstr,ipaddr,(long long)num);
    else sprintf(addr,"%s://%llu",transportstr,(long long)num);
} 

static int32_t process_plugin_json(char *retbuf,int32_t max,int32_t *sendflagp,struct plugin_info *plugin,int32_t permanentflag,uint64_t daemonid,uint64_t myid,char *jsonstr)
{
    int32_t len = (int32_t)strlen(jsonstr);
    cJSON *json,*obj;
    uint64_t tag = 0;
    char name[MAX_JSON_FIELD];
    retbuf[0] = *sendflagp = 0;
    if ( Debuglevel > 2 )
        printf("PLUGIN.(%s) process_plugin_json\n",plugin->name);
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        if ( is_cJSON_Array(json) != 0 )
            obj = cJSON_GetArrayItem(json,0);
        else obj = json;
        copy_cJSON(name,cJSON_GetObjectItem(obj,"plugin"));
        if ( name[0] == 0 )
            copy_cJSON(name,cJSON_GetObjectItem(obj,"destplugin"));
        tag = get_API_nxt64bits(cJSON_GetObjectItem(obj,"tag"));
        if ( strcmp(name,plugin->name) == 0 && (len= PLUGNAME(_process_json)(plugin,tag,retbuf,max,jsonstr,obj,0)) > 0 )
        {
            *sendflagp = 1;
            if ( retbuf[0] == 0 )
                sprintf(retbuf,"{\"result\":\"no response\"}");
            append_stdfields(retbuf,max,plugin,tag,0);
            printf("return.(%s)\n",retbuf);
            return((int32_t)strlen(retbuf));
        } else printf("(%s) -> no return.%d (%s) vs (%s) len.%d\n",jsonstr,strcmp(name,plugin->name),name,plugin->name,len);
    }
    else
    {
        //printf("couldnt parse.(%s)\n",jsonstr);
        if ( jsonstr[len-1] == '\r' || jsonstr[len-1] == '\n' || jsonstr[len-1] == '\t' || jsonstr[len-1] == ' ' )
            jsonstr[--len] = 0;
        sprintf(retbuf,"{\"result\":\"unparseable\",\"message\":\"%s\"}",jsonstr);
    }
    if ( *sendflagp != 0 && retbuf[0] != 0 )
        append_stdfields(retbuf,max,plugin,tag,0);
    else retbuf[0] = *sendflagp = 0;
    return((int32_t)strlen(retbuf));
}

static int32_t get_newinput(char *messages[],uint32_t *numrecvp,uint32_t numsent,int32_t permanentflag,union endpoints *socks,int32_t timeoutmillis,void (*funcp)(char *line))
{
    char line[8192];
    int32_t len,n = 0;
    line[0] = 0;
    if ( (n= poll_local_endpoints(messages,numrecvp,numsent,socks,timeoutmillis)) <= 0 && permanentflag == 0 && getline777(line,sizeof(line)-1) > 0 )
    {
        len = (int32_t)strlen(line);
        if ( line[len-1] == '\n' )
            line[--len] = 0;
        if ( len > 0 )
        {
            if ( funcp != 0 )
                (*funcp)(line);
            else messages[0] = clonestr(line), n = 1;
        }
    }
    return(n);
}

#ifdef BUNDLED
int32_t PLUGNAME(_main)
#else
int32_t main
#endif
(int argc,const char *argv[])
{
    char *retbuf,registerbuf[MAX_JSON_FIELD];
    struct plugin_info *plugin;
    double startmilli;
    cJSON *argjson;
    int32_t i,n,bundledflag,max,sendflag,sleeptime=1,len = 0;
    char *messages[16],*line,*jsonargs,*transportstr;
    milliseconds();
    max = 1000000;
    retbuf = malloc(max+1);
    plugin = calloc(1,sizeof(*plugin) + PLUGIN_EXTRASIZE);
    plugin->extrasize = PLUGIN_EXTRASIZE;
    plugin->ppid = OS_getppid();
    strcpy(plugin->name,PLUGINSTR);
    if ( 0 )
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
        configure_plugin(retbuf,max,plugin,jsonargs,-1);
        //fprintf(stderr,"PLUGIN_RETURNS.[%s]\n",retbuf), fflush(stdout);
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
    configure_plugin(retbuf,max,plugin,jsonargs,1);
    printf("CONFIGURED.(%s) argc.%d: %s myid.%llu daemonid.%llu NXT.%s\n",plugin->name,argc,plugin->permanentflag != 0 ? "PERMANENT" : "WEBSOCKET",(long long)plugin->myid,(long long)plugin->daemonid,plugin->NXTADDR);//,jsonargs!=0?jsonargs:"");
    if ( init_pluginsocks(plugin,plugin->permanentflag,plugin->bindaddr,plugin->connectaddr,plugin->myid,plugin->daemonid,plugin->timeout) == 0 )
    {
        argjson = cJSON_Parse(jsonargs);
        if ( (len= registerAPI(registerbuf,sizeof(registerbuf)-1,plugin,argjson)) > 0 )
        {
            if ( Debuglevel > 2 )
                fprintf(stderr,">>>>>>>>>>>>>>> plugin sends REGISTER SEND.(%s)\n",registerbuf);
            nn_local_broadcast(&plugin->all.socks,0,0,(uint8_t *)registerbuf,(int32_t)strlen(registerbuf)+1), plugin->numsent++;
            //nn_send(plugin->sock,plugin->registerbuf,len+1,0); // send the null terminator too
        } else printf("error register API\n");
        if ( argjson != 0 )
            free_json(argjson);
    } else printf("error init_pluginsocks\n");
    while ( OS_getppid() == plugin->ppid )
    {
        retbuf[0] = 0;
        if ( (n= get_newinput(messages,&plugin->numrecv,plugin->numsent,plugin->permanentflag,&plugin->all,plugin->timeout,0)) > 0 )
        {
            for (i=0; i<n; i++)
            {
                line = messages[i], len = (int32_t)strlen(line);
                if ( Debuglevel > 2 )
                    printf("(s%d r%d) <<<<<<<<<<<<<< RECEIVED (%s).%d -> bind.(%s) connect.(%s) %s\n",plugin->numsent,plugin->numrecv,line,len,plugin->bindaddr,plugin->connectaddr,plugin->permanentflag != 0 ? "PERMANENT" : "WEBSOCKET"), fflush(stdout);
                if ( (len= process_plugin_json(retbuf,max,&sendflag,plugin,plugin->permanentflag,plugin->daemonid,plugin->myid,line)) > 0 )
                {
                    if ( plugin->bundledflag == 0 )
                        printf("%s\n",retbuf), fflush(stdout);
                    if ( sendflag != 0 )
                    {
                        nn_local_broadcast(&plugin->all.socks,0,0,(uint8_t *)retbuf,len+1), plugin->numsent++;
                        if ( Debuglevel > 2 )
                            fprintf(stderr,">>>>>>>>>>>>>> returned.(%s)\n",retbuf);
                        //nn_send(plugin->sock,retbuf,len+1,0); // send the null terminator too
                    }
                }
                free(line);
            }
        }
        if ( n == 0 )
        {
            startmilli = milliseconds();
            if ( PLUGNAME(_idle)(plugin) == 0 )
            {
                if ( plugin->sleepmillis != 0 )
                {
                    sleeptime = plugin->sleepmillis - (milliseconds() - startmilli);
                    //printf("%s sleepmillis.%d sleeptime.%d\n",plugin->name,plugin->sleepmillis,sleeptime);
                    if ( sleeptime > 0 )
                        msleep(sleeptime);
                }
            }
        }
    } fprintf(stderr,"ppid.%d changed to %d\n",plugin->ppid,OS_getppid());
    PLUGNAME(_shutdown)(plugin,len); // rc == 0 -> parent process died
    shutdown_plugsocks(&plugin->all);
    free(plugin);
    return(len);
}

#endif
#endif
