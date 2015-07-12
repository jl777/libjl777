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
#include <curl/curl.h>
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
#include "includes/cJSON.h"
#include "sophia/kv777.c"
#include "utils/system777.c"

struct plugin_info
{
    char bindaddr[64],connectaddr[64],ipaddr[64],name[64],NXTADDR[64],SERVICENXT[64];
    uint64_t daemonid,myid,nxt64bits;
    //union endpoints all;
    int32_t pushsock,pullsock;
    uint32_t permanentflag,ppid,extrasize,timeout,numrecv,numsent,bundledflag,registered,sleepmillis,allowremote;
    uint16_t port;
    portable_mutex_t mutex;
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

static int32_t init_pluginsocks(struct plugin_info *plugin,int32_t permanentflag,uint64_t instanceid,uint64_t daemonid,int32_t timeout)
{
//#ifdef _WIN32
//    sprintf(plugin->connectaddr,"tcp://127.0.0.1:7774");
//#endif
    if ( (plugin->pushsock= nn_socket(AF_SP,NN_PUSH)) < 0 )
    {
        printf("error creating plugin->pushsock %s\n",nn_strerror(nn_errno()));
        return(-1);
    }
    else if ( nn_settimeouts(plugin->pushsock,10,1) < 0 )
    {
        printf("error setting plugin->pushsock timeouts %s\n",nn_strerror(nn_errno()));
        return(-1);
    }
    else if ( nn_connect(plugin->pushsock,plugin->connectaddr) < 0 )
    {
        printf("error connecting plugin->pushsock.%d to %s %s\n",plugin->pushsock,plugin->connectaddr,nn_strerror(nn_errno()));
        return(-1);
    }
    if ( (plugin->pullsock= nn_socket(AF_SP,NN_BUS)) < 0 )
    {
        printf("error creating plugin->pullsock %s\n",nn_strerror(nn_errno()));
        return(-1);
    }
    else if ( nn_settimeouts(plugin->pullsock,10,1) < 0 )
    {
        printf("error setting plugin->pullsock timeouts %s\n",nn_strerror(nn_errno()));
        return(-1);
    }
    else if ( nn_bind(plugin->pullsock,plugin->bindaddr) < 0 )
    {
        printf("error connecting plugin->pullsock.%d to %s %s\n",plugin->pullsock,plugin->bindaddr,nn_strerror(nn_errno()));
        return(-1);
    }
    printf("%s bind.(%s) %d and connected.(%s) %d\n",plugin->name,plugin->bindaddr,plugin->pullsock,plugin->connectaddr,plugin->pushsock);
    return(0);
}

static int32_t process_json(char *retbuf,int32_t max,struct plugin_info *plugin,char *jsonargs,int32_t initflag)
{
    void *loadfile(uint64_t *allocsizep,char *fname);
    char filename[1024],*myipaddr,tokenstr[MAX_JSON_FIELD],*jsonstr = 0;
    cJSON *obj=0,*tmp,*json = 0;
    uint64_t allocsize,nxt64bits,tag = 0;
    int32_t retval = 0;
//printf("call process_json.(%s)\n",jsonargs);
    if ( jsonargs != 0 && (json= cJSON_Parse(jsonargs)) != 0 )
    {
        if ( is_cJSON_Array(json) != 0 && cJSON_GetArraySize(json) == 2 )
        {
            obj = cJSON_GetArrayItem(json,0);
            copy_cJSON(tokenstr,cJSON_GetArrayItem(json,1));
        }
        else obj = json, tokenstr[0] = 0;
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
        tag = get_API_nxt64bits(cJSON_GetObjectItem(obj,"tag"));
        if ( initflag > 0 )
        {
            if ( (nxt64bits= get_API_nxt64bits(cJSON_GetObjectItem(obj,"NXT"))) != 0 )
            {
                plugin->nxt64bits = nxt64bits;
                expand_nxt64bits(plugin->NXTADDR,plugin->nxt64bits);
            }
            if ( (nxt64bits= get_API_nxt64bits(cJSON_GetObjectItem(obj,"serviceNXT"))) != 0 )
                expand_nxt64bits(plugin->SERVICENXT,nxt64bits);
            myipaddr = cJSON_str(cJSON_GetObjectItem(obj,"ipaddr"));
            if ( is_ipaddr(myipaddr) != 0 )
                strcpy(plugin->ipaddr,myipaddr);
            plugin->port = get_API_int(cJSON_GetObjectItem(obj,"port"),0);
        }
    }
    //fprintf(stderr,"tag.%llu initflag.%d got jsonargs.(%s) [%s] %p\n",(long long)tag,initflag,jsonargs,jsonstr,obj);
    if ( jsonstr != 0 && obj != 0 )
        retval = PLUGNAME(_process_json)(0,0,1,plugin,tag,retbuf,max,jsonstr,obj,initflag,tokenstr);
    else printf("error with JSON.(%s)\n",jsonstr);//, getchar();
    //fprintf(stderr,"done tag.%llu initflag.%d got jsonargs.(%p) %p %p\n",(long long)tag,initflag,jsonargs,jsonstr,obj);
    if ( jsonstr != 0 )
        free(jsonstr);
    if ( json != 0 )
        free_json(json);
    printf("%s\n",retbuf), fflush(stdout);
    return(retval);
}

/*static int32_t set_nxtaddrs(char *NXTaddr,char *serviceNXT)
{
    FILE *fp; cJSON *json; char confname[512],buf[65536],secret[4096],servicesecret[4096]; uint8_t mysecret[32],mypublic[32];
    strcpy(confname,"SuperNET.conf"), os_compatible_path(confname);
    NXTaddr[0] = serviceNXT[0] = 0;
    if ( (fp= fopen(confname,"rb")) != 0 )
    {
        if ( fread(buf,1,sizeof(buf),fp) > 0 )
        {
            if ( (json= cJSON_Parse(buf)) != 0 )
            {
                copy_cJSON(secret,cJSON_GetObjectItem(json,"secret"));
                copy_cJSON(servicesecret,cJSON_GetObjectItem(json,"SERVICESECRET"));
                expand_nxt64bits(NXTaddr,conv_NXTpassword(mysecret,mypublic,(uint8_t *)secret,(int32_t)strlen(secret)));
                expand_nxt64bits(serviceNXT,conv_NXTpassword(mysecret,mypublic,(uint8_t *)servicesecret,(int32_t)strlen(servicesecret)));
                free_json(json);
            } else fprintf(stderr,"set_nxtaddrs parse error.(%s)\n",buf);
        } else fprintf(stderr,"set_nxtaddrs error reading.(%s)\n",confname);
        fclose(fp);
    } else fprintf(stderr,"set_nxtaddrs cant open.(%s)\n",confname);
    return((int32_t)strlen(NXTaddr));
}*/

static void append_stdfields(char *retbuf,int32_t max,struct plugin_info *plugin,uint64_t tag,int32_t allfields)
{
    char tagstr[512]; cJSON *json; int32_t len;
//printf("APPEND.(%s) (%s)\n",retbuf,plugin->name);
    tagstr[0] = 0;
    len = (int32_t)strlen(retbuf);
    if ( len > 4 && retbuf[len-1] != ']' && (json= cJSON_Parse(retbuf)) != 0 )
    {
        if ( tag != 0 && get_API_nxt64bits(cJSON_GetObjectItem(json,"tag")) == 0 )
            sprintf(tagstr,",\"tag\":\"%llu\"",(long long)tag);
        if ( cJSON_GetObjectItem(json,"serviceNXT") == 0 && plugin->SERVICENXT[0] != 0 )
            sprintf(tagstr+strlen(tagstr),",\"serviceNXT\":\"%s\"",plugin->SERVICENXT);
        if ( cJSON_GetObjectItem(json,"NXT") == 0 )
            sprintf(tagstr+strlen(tagstr),",\"NXT\":\"%s\"",plugin->NXTADDR);
        if ( allfields != 0 )
        {
             if ( SUPERNET.iamrelay != 0 )
                 sprintf(retbuf+strlen(retbuf)-1,",\"myipaddr\":\"%s\"}",plugin->ipaddr);
            sprintf(retbuf+strlen(retbuf)-1,",\"allowremote\":%d%s}",plugin->allowremote,tagstr);
            sprintf(retbuf+strlen(retbuf)-1,",\"permanentflag\":%d,\"daemonid\":\"%llu\",\"myid\":\"%llu\",\"plugin\":\"%s\",\"endpoint\":\"%s\",\"millis\":%.2f,\"sent\":%u,\"recv\":%u}",plugin->permanentflag,(long long)plugin->daemonid,(long long)plugin->myid,plugin->name,plugin->bindaddr[0]!=0?plugin->bindaddr:plugin->connectaddr,milliseconds(),plugin->numsent,plugin->numrecv);
         }
         else sprintf(retbuf+strlen(retbuf)-1,",\"daemonid\":\"%llu\",\"myid\":\"%llu\",\"allowremote\":%d%s}",(long long)plugin->daemonid,(long long)plugin->myid,plugin->allowremote,tagstr);
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
    /*char NXTaddr[512],serviceNXT[512],tmpstrA[512],tmpstrB[512];
    copy_cJSON(NXTaddr,cJSON_GetObjectItem(json,"NXT"));
    copy_cJSON(serviceNXT,cJSON_GetObjectItem(json,"serviceNXT"));
    if ( NXTaddr[0] == 0 || serviceNXT[0] == 0 )
    {
        set_nxtaddrs(tmpstrA,tmpstrB);
        if ( NXTaddr[0] == 0 )
            strcpy(NXTaddr,tmpstrA);
        if ( serviceNXT[0] == 0 )
            strcpy(serviceNXT,tmpstrB);
        strcpy(plugin->NXTADDR,NXTaddr);
        strcpy(plugin->SERVICENXT,serviceNXT);
    }*/
    if ( cJSON_GetObjectItem(json,"NXT") == 0 )
        cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(plugin->NXTADDR));
    else cJSON_ReplaceItemInObject(json,"NXT",cJSON_CreateString(plugin->NXTADDR));
    if ( cJSON_GetObjectItem(json,"serviceNXT") == 0 )
        cJSON_AddItemToObject(json,"serviceNXT",cJSON_CreateString(plugin->SERVICENXT));
    else cJSON_ReplaceItemInObject(json,"serviceNXT",cJSON_CreateString(plugin->SERVICENXT));
    jsonstr = cJSON_Print(json), free_json(json);
    _stripwhite(jsonstr,' ');
    strcpy(retbuf,jsonstr), free(jsonstr);
    append_stdfields(retbuf,max,plugin,0,1);
    if ( Debuglevel > 2 )
        printf(">>>>>>>>>>> register return.(%s)\n",retbuf);
    return((int32_t)strlen(retbuf));
}

static void configure_plugin(char *retbuf,int32_t max,struct plugin_info *plugin,char *jsonargs,int32_t initflag)
{
    if ( initflag != 0 && jsonargs != 0 && jsonargs[0] != 0 && has_backslash(jsonargs) != 0 )
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
    int32_t valid = -11,len = (int32_t)strlen(jsonstr); cJSON *json,*obj,*tokenobj; uint64_t tag = 0;
    char name[MAX_JSON_FIELD],destname[MAX_JSON_FIELD],forwarder[MAX_JSON_FIELD],tokenstr[MAX_JSON_FIELD],sender[MAX_JSON_FIELD];
    retbuf[0] = *sendflagp = 0;
    if ( Debuglevel > 2 )
        printf("PLUGIN.(%s) process_plugin_json (%s)\n",plugin->name,jsonstr);
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        if ( is_cJSON_Array(json) != 0 )
        {
            obj = cJSON_GetArrayItem(json,0);
            jsonstr = cJSON_Print(obj), _stripwhite(jsonstr,' ');
            tokenobj = cJSON_GetArrayItem(json,1), _stripwhite(tokenstr,' ');
            copy_cJSON(tokenstr,tokenobj);
            copy_cJSON(forwarder,cJSON_GetObjectItem(tokenobj,"forwarder"));
            copy_cJSON(sender,cJSON_GetObjectItem(tokenobj,"sender"));
            valid = get_API_int(cJSON_GetObjectItem(tokenobj,"valid"),valid);
        }
        else obj = json, tokenstr[0] = forwarder[0] = sender[0] = 0;
        copy_cJSON(name,cJSON_GetObjectItem(obj,"plugin"));
        if ( name[0] == 0 )
            copy_cJSON(name,cJSON_GetObjectItem(obj,"agent"));
        copy_cJSON(destname,cJSON_GetObjectItem(obj,"destplugin"));
        if ( destname[0] == 0 )
            copy_cJSON(destname,cJSON_GetObjectItem(obj,"destagent"));
        tag = get_API_nxt64bits(cJSON_GetObjectItem(obj,"tag"));
        if ( (strcmp(name,plugin->name) == 0 || strcmp(destname,plugin->name) == 0) && (len= PLUGNAME(_process_json)(forwarder,sender,valid,plugin,tag,retbuf,max,jsonstr,obj,0,tokenstr)) > 0 )
        {
            *sendflagp = 1;
            if ( retbuf[0] == 0 )
                sprintf(retbuf,"{\"result\":\"no response\"}");
            append_stdfields(retbuf,max,plugin,tag,0);
            if ( Debuglevel > 2 )
                printf("return.(%s)\n",retbuf);
            return((int32_t)strlen(retbuf));
        } //else printf("(%s) -> no return.%d (%s) vs (%s):(%s) len.%d\n",jsonstr,strcmp(name,plugin->name),name,destname,plugin->name,len);
    }
    else
    {
printf("process_plugin_json: couldnt parse.(%s)\n",jsonstr);
        if ( jsonstr[len-1] == '\r' || jsonstr[len-1] == '\n' || jsonstr[len-1] == '\t' || jsonstr[len-1] == ' ' )
            jsonstr[--len] = 0;
        sprintf(retbuf,"{\"result\":\"unparseable\",\"message\":\"%s\"}",jsonstr);
    }
    if ( *sendflagp != 0 && retbuf[0] != 0 )
        append_stdfields(retbuf,max,plugin,tag,0);
    else retbuf[0] = *sendflagp = 0;
    return((int32_t)strlen(retbuf));
}

#ifdef BUNDLED
int32_t PLUGNAME(_main)
#else
int32_t main
#endif
(int argc,const char *argv[])
{
    char *retbuf,*line,*jsonargs,*transportstr,registerbuf[MAX_JSON_FIELD];
    struct plugin_info *plugin; double startmilli; cJSON *argjson;
    int32_t bundledflag,max,sendflag,sleeptime=1,len = 0;
#ifndef BUNDLED
    portable_OS_init();
#endif
    milliseconds();
    max = 1000000;
    retbuf = malloc(max+1);
    plugin = calloc(1,sizeof(*plugin));// + PLUGIN_EXTRASIZE);
    //plugin->extrasize = PLUGIN_EXTRASIZE;
    plugin->ppid = OS_getppid();
    strcpy(plugin->name,PLUGINSTR);
    if ( 0 )
    {
        int32_t i;
        for (i=0; i<argc; i++)
            printf("(%s) ",argv[i]);
        printf("argc.%d\n",argc);
    }
    //printf("%s (%s).argc%d parent PID.%d\n",plugin->name,argv[0],argc,plugin->ppid);
    plugin->timeout = 1;
    if ( argc <= 2 )
    {
        jsonargs = (argc > 1) ? stringifyM((char *)argv[1]) : stringifyM("{}");
        configure_plugin(retbuf,max,plugin,jsonargs,-1);
        free(jsonargs);
//fprintf(stderr,"PLUGIN_RETURNS.[%s]\n",retbuf), fflush(stdout);
        return(0);
    }
    randombytes((uint8_t *)&plugin->myid,sizeof(plugin->myid));
    plugin->permanentflag = atoi(argv[1]);
    plugin->daemonid = calc_nxt64bits(argv[2]);
    plugin->bundledflag = bundledflag = is_bundled_plugin(plugin->name);
    transportstr = get_localtransport(plugin->bundledflag);
    sprintf(plugin->connectaddr,"%s://SuperNET",transportstr);
    sprintf(plugin->bindaddr,"%s://%llu",transportstr,(long long)plugin->daemonid);
    jsonargs = (argc >= 3) ? ((char *)argv[3]) : 0;
    configure_plugin(retbuf,max,plugin,jsonargs,1);
    printf("CONFIGURED.(%s) argc.%d: %s myid.%llu daemonid.%llu NXT.%s serviceNXT.%s\n",plugin->name,argc,plugin->permanentflag != 0 ? "PERMANENT" : "WEBSOCKET",(long long)plugin->myid,(long long)plugin->daemonid,plugin->NXTADDR,plugin->SERVICENXT);//,jsonargs!=0?jsonargs:"");
    if ( init_pluginsocks(plugin,plugin->permanentflag,plugin->myid,plugin->daemonid,plugin->timeout) == 0 )
    {
        argjson = cJSON_Parse(jsonargs);
        if ( (len= registerAPI(registerbuf,sizeof(registerbuf)-1,plugin,argjson)) > 0 )
        {
            if ( Debuglevel > 2 )
                fprintf(stderr,">>>>>>>>>>>>>>> plugin.(%s) sends REGISTER SEND.(%s)\n",plugin->name,registerbuf);
            nn_local_broadcast(plugin->pushsock,0,0,(uint8_t *)registerbuf,(int32_t)strlen(registerbuf)+1), plugin->numsent++;
        } else printf("error register API\n");
        if ( argjson != 0 )
            free_json(argjson);
    } else printf("error init_pluginsocks\n");
    while ( OS_getppid() == plugin->ppid )
    {
        retbuf[0] = 0;
        if ( (len= nn_recv(plugin->pullsock,&line,NN_MSG,0)) > 0 )
        {
            len = (int32_t)strlen(line);
            if ( Debuglevel > 2 )
                printf("(s%d r%d) <<<<<<<<<<<<<< %s.RECEIVED (%s).%d -> bind.(%s) connect.(%s) %s\n",plugin->numsent,plugin->numrecv,plugin->name,line,len,plugin->bindaddr,plugin->connectaddr,plugin->permanentflag != 0 ? "PERMANENT" : "WEBSOCKET"), fflush(stdout);
            if ( (len= process_plugin_json(retbuf,max,&sendflag,plugin,plugin->permanentflag,plugin->daemonid,plugin->myid,line)) > 0 )
            {
                if ( plugin->bundledflag == 0 )
                    printf("%s\n",retbuf), fflush(stdout);
                if ( sendflag != 0 )
                {
                    nn_local_broadcast(plugin->pushsock,0,0,(uint8_t *)retbuf,(int32_t)strlen(retbuf)+1), plugin->numsent++;
                    if ( Debuglevel > 2 )
                        fprintf(stderr,">>>>>>>>>>>>>> returned.(%s)\n",retbuf);
                }
            } //else printf("null return from process_plugin_json\n");
            nn_freemsg(line);
        }
        else
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
    nn_shutdown(plugin->pushsock,0);
    if ( plugin->pushsock != plugin->pullsock )
        nn_shutdown(plugin->pullsock,0);
    free(plugin);
    return(len);
}

#endif
#endif
