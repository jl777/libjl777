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
#include "../../nanomsg/src/nn.h"
#include "../../nanomsg/src/bus.h"
#include "../../nanomsg/src/pubsub.h"
#include "../../nanomsg/src/pipeline.h"
#include "../../nanomsg/src/reqrep.h"
#include "../../nanomsg/src/survey.h"
#include "../../nanomsg/src/pair.h"
#include "../../nanomsg/src/pubsub.h"
#include "../includes/cJSON.h"
#include "../includes/uthash.h"

#ifndef PLUGINMILLIS
#define PLUGINMILLIS 10000
#endif

struct protocol_info
{
    struct dKV777 *protocol_relays; //struct kv777 *coins,*accounts;
    struct endpoint connections[8192],myendpoint;
    char name[64],agent[512],path[512],transport[16],ipaddr[64],NXTADDR[64],endpointstr[512]; uint8_t NXTACCTSECRET[2048];
    int32_t ip6flag,subsock,leverage; uint64_t ipbits;
    uint16_t port;
};

struct plugin_info
{
    char bindaddr[64],connectaddr[64],ipaddr[64],name[64],NXTADDR[64],SERVICENXT[64];
    uint64_t daemonid,myid,nxt64bits;
    struct protocol_info protocol;
    int32_t pushsock,pullsock;
    uint32_t permanentflag,ppid,extrasize,timeout,numrecv,numsent,bundledflag,registered,sleepmillis,allowremote;
    uint16_t port;
    portable_mutex_t mutex;
    uint8_t pluginspace[];
};

struct nn_clock
{
    uint64_t last_tsc;
    uint64_t last_time;
};

int32_t plugin_result(char *retbuf,cJSON *json,uint64_t tag);
static int32_t plugin_copyretstr(char *retbuf,long maxlen,char *retstr);
//static struct dKV777 *agent_initprotocol(struct plugin_info *plugin,cJSON *json,char *agent,char *path,char *protocol,double pingmillis,char *nxtsecret,struct kv777 *kps[],int32_t numkps);
static char *protocol_endpoint(char *retbuf,long maxlen,struct protocol_info *prot,cJSON *json,char *jsonstr,char *tokenstr,char *forwarder,char *sender,int32_t valid);

static char *protocol_ping(char *retbuf,long maxlen,struct protocol_info *prot,cJSON *json,char *jsonstr,char *tokenstr,char *forwarder,char *sender,int32_t valid);
int32_t protocols_init(int32_t sock,struct endpoint *connections,char *protocol);
uint32_t is_ipaddr(char *str);
void randombytes(unsigned char *x,long xlen);
int32_t OS_getppid();
void portable_OS_init();
static int32_t nn_settimeouts2(int32_t sock,int32_t sendtimeout,int32_t recvtimeout);
int32_t myatoi(char *str,int32_t maxval);

#endif
#else
#ifndef crypto777_plugin777_c
#define crypto777_plugin777_c

#ifndef crypto777_plugin777_h
#define DEFINES_ONLY
#include "plugin777.c"
#include "../utils/inet.c"
#undef DEFINES_ONLY
#endif

#define nn_errstr() nn_strerror(nn_errno())
struct nn_clock Global_timer;

#define OFFSET_ENABLED (bundledflag == 0)
static char *get_localtransport(int32_t bundledflag) { return(OFFSET_ENABLED ? "ipc" : "inproc"); }

static double milliseconds2()
{
    uint64_t nn_clock_now (struct nn_clock *self);
    return(nn_clock_now(&Global_timer));
}

static void msleep2(uint32_t milliseconds)
{
    void nn_sleep (int milliseconds);
    nn_sleep(milliseconds);
}

static long stripwhite2(char *buf,int accept)
{
    int32_t i,j,c;
    if ( buf == 0 || buf[0] == 0 )
        return(0);
    for (i=j=0; buf[i]!=0; i++)
    {
        buf[j] = c = buf[i];
        if ( c == accept || (c != ' ' && c != '\n' && c != '\r' && c != '\t' && c != '\b') )
            j++;
    }
    buf[j] = 0;
    return(j);
}

static int32_t has_backslash2(char *str)
{
    int32_t i;
    if ( str == 0 || str[0] == 0 )
        return(0);
    for (i=0; str[i]!=0; i++)
        if ( str[i] == '\\' )
            return(1);
    return(0);
}

static int32_t nn_socket_status2(int32_t sock,int32_t timeoutmillis)
{
    struct nn_pollfd pfd;
    int32_t rc;
    pfd.fd = sock;
    pfd.events = NN_POLLIN | NN_POLLOUT;
    if ( (rc= nn_poll(&pfd,1,timeoutmillis)) == 0 )
        return(pfd.revents);
    else return(-1);
}

static int32_t nn_local_broadcast2(int32_t sock,uint64_t instanceid,int32_t flags,uint8_t *retstr,int32_t len)
{
    int32_t i,sendlen,errs = 0;
    if ( sock >= 0 )
    {
        for (i=0; i<10; i++)
            if ( (nn_socket_status2(sock,1) & NN_POLLOUT) != 0 )
                break;
        if ( (sendlen= nn_send(sock,(char *)retstr,len,0)) <= 0 )
            errs++, printf("sending to socket.%d sendlen.%d len.%d (%s) [%s]\n",sock,sendlen,len,nn_strerror(nn_errno()),retstr);
        //else if ( Debuglevel > 2 )
        //    printf("nn_local_broadcast SENT.(%s) len.%d sendlen.%d vs strlen.%ld instanceid.%llu -> sock.%d\n",retstr,len,sendlen,strlen((char *)retstr),(long long)instanceid,sock);
    }
    return(errs);
}

static int32_t nn_settimeouts2(int32_t sock,int32_t sendtimeout,int32_t recvtimeout)
{
    int32_t retrymillis,maxmillis;
    maxmillis = PLUGINMILLIS, retrymillis = maxmillis/40;
    if ( nn_setsockopt(sock,NN_SOL_SOCKET,NN_RECONNECT_IVL,&retrymillis,sizeof(retrymillis)) < 0 )
        fprintf(stderr,"error setting NN_REQ NN_RECONNECT_IVL_MAX socket %s\n",nn_errstr());
    else if ( nn_setsockopt(sock,NN_SOL_SOCKET,NN_RECONNECT_IVL_MAX,&maxmillis,sizeof(maxmillis)) < 0 )
        fprintf(stderr,"error setting NN_REQ NN_RECONNECT_IVL_MAX socket %s\n",nn_errstr());
    else if ( sendtimeout > 0 && nn_setsockopt(sock,NN_SOL_SOCKET,NN_SNDTIMEO,&sendtimeout,sizeof(sendtimeout)) < 0 )
        fprintf(stderr,"error setting sendtimeout %s\n",nn_errstr());
    else if ( recvtimeout > 0 && nn_setsockopt(sock,NN_SOL_SOCKET,NN_RCVTIMEO,&recvtimeout,sizeof(recvtimeout)) < 0 )
        fprintf(stderr,"error setting sendtimeout %s\n",nn_errstr());
    else return(0);
    return(-1);
}

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
    else if ( nn_settimeouts2(plugin->pushsock,10,1) < 0 )
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
    else if ( nn_settimeouts2(plugin->pullsock,10,1) < 0 )
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

/*static struct dKV777 *agent_initprotocol(struct plugin_info *plugin,cJSON *json,char *agent,char *path,char *protocol,double pingmillis,char *nxtsecret,struct kv777 *kps[],int32_t numkps)
{
    cJSON *argjson; char buf[8192]; int32_t n; struct protocol_info *prot = &plugin->protocol;
    if ( path == 0 )
        path = protocol;
    ensure_directory(path);
    strcpy(prot->name,protocol), strcpy(prot->agent,agent), strcpy(prot->path,path);
    if ( (argjson= jobj(json,agent)) != 0 )
    {
        if ( nxtsecret == 0 || nxtsecret[0] == 0 )
            copy_cJSON((void *)prot->NXTACCTSECRET,jobj(argjson,"secret"));
        else strcpy((void *)prot->NXTACCTSECRET,nxtsecret);
        copy_cJSON((void *)prot->transport,jobj(argjson,"transport"));
        copy_cJSON((void *)prot->ipaddr,jobj(argjson,"myipaddr"));
        prot->port = juint(argjson,"port");
        printf("found %s path.(%s) protocol.(%s) json secret.(%s) %s:port.%d\n",agent,path,protocol,(char *)prot->NXTACCTSECRET,prot->ipaddr,prot->port);
    }
    if ( prot->NXTACCTSECRET[0] == 0 )
    {
        sprintf(buf,"%s/randvals",path);
        gen_randomacct(33,prot->NXTADDR,(void *)prot->NXTACCTSECRET,buf);
    }
    if ( prot->port == 0 )
        prot->port = SUPERNET_PORT + ((((uint16_t)protocol[0] << 8) | agent[0]) % 777);
#ifndef BUNDLED
    strcpy(SUPERNET.DBPATH,path), os_compatible_path(SUPERNET.DBPATH), ensure_directory(SUPERNET.DBPATH);
#endif
    buf[0] = 0, prot->subsock = nn_createsocket(buf,0,"NN_SUB",NN_SUB,0,10,1);
    n = protocols_init(prot->subsock,prot->connections,protocol);
    prot->protocol_relays = dKV777_init("protocol",protocol,kps,numkps,0,-1,prot->subsock,prot->connections,n,(int32_t)(sizeof(prot->connections)/sizeof(*prot->connections)),prot->port,pingmillis);
    set_KV777_globals(&prot->protocol_relays,prot->transport,prot->NXTACCTSECRET,(int32_t)strlen((void *)prot->NXTACCTSECRET),plugin->SERVICENXT,KV777.relayendpoint);
    strcpy(plugin->NXTADDR,KV777.NXTADDR);
    return(prot->protocol_relays);
}

char *protocol_ping(char *retbuf,long maxlen,struct protocol_info *prot,cJSON *json,char *jsonstr,char *tokenstr,char *forwarder,char *sender,int32_t valid)
{
    return(dKV777_ping(prot->protocol_relays));
}*/

static int32_t plugin_copyretstr(char *retbuf,long maxlen,char *retstr)
{
    if ( retstr != 0 )
    {
        strncpy(retbuf,retstr,maxlen-1);
        retbuf[maxlen-1] = 0;
        free(retstr);
    }// else strcpy(retbuf,"{\"error\":\"under construction\"}");
    return((int32_t)strlen(retbuf) + (retbuf[0] != 0));
}

static int32_t process_json(char *retbuf,int32_t max,struct plugin_info *plugin,char *jsonargs,int32_t initflag)
{
    void *loadfile(uint64_t *allocsizep,char *fname);
    struct destbuf tokenstr,filename;
    char *myipaddr,*jsonstr = 0;
    cJSON *obj=0,*tmp,*json = 0;
    uint64_t allocsize,nxt64bits,tag = 0;
    int32_t retval = 0;
//printf("call process_json.(%s)\n",jsonargs);
    if ( jsonargs != 0 && (json= cJSON_Parse(jsonargs)) != 0 )
    {
        if ( is_cJSON_Array(json) != 0 && cJSON_GetArraySize(json) == 2 )
        {
            obj = cJSON_GetArrayItem(json,0);
            copy_cJSON(&tokenstr,cJSON_GetArrayItem(json,1));
        }
        else obj = json, tokenstr.buf[0] = 0;
        copy_cJSON(&filename,cJSON_GetObjectItem(obj,"filename"));
        if ( filename.buf[0] != 0 && (jsonstr= loadfile(&allocsize,filename.buf)) != 0 )
        {
            if ( (tmp= cJSON_Parse(jsonstr)) != 0 )
                obj = tmp;
            else free(jsonstr), jsonstr = 0;
        }
        if ( jsonstr == 0 )
            jsonstr = cJSON_Print(obj);
        stripwhite2(jsonstr,' ');
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
            plugin->port = juint(obj,"port");
        }
    }
    //fprintf(stderr,"tag.%llu initflag.%d got jsonargs.(%s) [%s] %p\n",(long long)tag,initflag,jsonargs,jsonstr,obj);
    if ( jsonstr != 0 && obj != 0 )
        retval = PLUGNAME(_process_json)(0,0,1,plugin,tag,retbuf,max,jsonstr,obj,initflag,tokenstr.buf);
    else printf("error with JSON.(%s)\n",jsonstr);//, getchar();
    //fprintf(stderr,"done tag.%llu initflag.%d got jsonargs.(%p) %p %p\n",(long long)tag,initflag,jsonargs,jsonstr,obj);
    if ( jsonstr != 0 )
        free(jsonstr);
    if ( json != 0 )
        free_json(json);
    printf("%s\n",retbuf), fflush(stdout);
    return(retval);
}

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
            //if ( SUPERNET.iamrelay != 0 )
            //    sprintf(retbuf+strlen(retbuf)-1,",\"myipaddr\":\"%s\"}",plugin->ipaddr);
            sprintf(retbuf+strlen(retbuf)-1,",\"allowremote\":%d%s}",plugin->allowremote,tagstr);
            sprintf(retbuf+strlen(retbuf)-1,",\"permanentflag\":%d,\"daemonid\":\"%llu\",\"myid\":\"%llu\",\"plugin\":\"%s\",\"endpoint\":\"%s\",\"millis\":%.2f,\"sent\":%u,\"recv\":%u}",plugin->permanentflag,(long long)plugin->daemonid,(long long)plugin->myid,plugin->name,plugin->bindaddr[0]!=0?plugin->bindaddr:plugin->connectaddr,milliseconds2(),plugin->numsent,plugin->numrecv);
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
        plugin->sleepmillis = get_API_int(cJSON_GetObjectItem(json,"sleepmillis"),25);//SUPERNET.APISLEEP);
    cJSON_AddItemToObject(json,"sleepmillis",cJSON_CreateNumber(plugin->sleepmillis));
    if ( cJSON_GetObjectItem(json,"NXT") == 0 )
        cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(plugin->NXTADDR));
    else cJSON_ReplaceItemInObject(json,"NXT",cJSON_CreateString(plugin->NXTADDR));
    if ( cJSON_GetObjectItem(json,"serviceNXT") == 0 )
        cJSON_AddItemToObject(json,"serviceNXT",cJSON_CreateString(plugin->SERVICENXT));
    else cJSON_ReplaceItemInObject(json,"serviceNXT",cJSON_CreateString(plugin->SERVICENXT));
    jsonstr = cJSON_Print(json), free_json(json);
    stripwhite2(jsonstr,' ');
    strcpy(retbuf,jsonstr), free(jsonstr);
    append_stdfields(retbuf,max,plugin,0,1);
   //printf(">>>>>>>>>>> register return.(%s)\n",retbuf);
    return((int32_t)strlen(retbuf) + (retbuf[0] != 0));
}

static void configure_plugin(char *retbuf,int32_t max,struct plugin_info *plugin,char *jsonargs,int32_t initflag)
{
    if ( initflag != 0 && jsonargs != 0 && jsonargs[0] != 0 && has_backslash2(jsonargs) != 0 )
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
    struct destbuf name,destname,forwarder,tokenstr,sender;
    retbuf[0] = *sendflagp = 0;
    //printf("PLUGIN.(%s) process_plugin_json (%s)\n",plugin->name,jsonstr);
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        if ( is_cJSON_Array(json) != 0 )
        {
            obj = cJSON_GetArrayItem(json,0);
            jsonstr = cJSON_Print(obj), stripwhite2(jsonstr,' ');
            tokenobj = cJSON_GetArrayItem(json,1), stripwhite2(tokenstr.buf,' ');
            copy_cJSON(&tokenstr,tokenobj);
            copy_cJSON(&forwarder,cJSON_GetObjectItem(tokenobj,"forwarder"));
            copy_cJSON(&sender,cJSON_GetObjectItem(tokenobj,"sender"));
            valid = get_API_int(cJSON_GetObjectItem(tokenobj,"valid"),valid);
        }
        else obj = json, tokenstr.buf[0] = forwarder.buf[0] = sender.buf[0] = 0;
        copy_cJSON(&name,cJSON_GetObjectItem(obj,"plugin"));
        if ( name.buf[0] == 0 )
            copy_cJSON(&name,cJSON_GetObjectItem(obj,"agent"));
        copy_cJSON(&destname,cJSON_GetObjectItem(obj,"destplugin"));
        if ( destname.buf[0] == 0 )
            copy_cJSON(&destname,cJSON_GetObjectItem(obj,"destagent"));
        tag = get_API_nxt64bits(cJSON_GetObjectItem(obj,"tag"));
        if ( (strcmp(name.buf,plugin->name) == 0 || strcmp(destname.buf,plugin->name) == 0) && (len= PLUGNAME(_process_json)(forwarder.buf,sender.buf,valid,plugin,tag,retbuf,max,jsonstr,obj,0,tokenstr.buf)) > 0 )
        {
            *sendflagp = 1;
            if ( retbuf[0] == 0 )
                sprintf(retbuf,"{\"result\":\"no response\"}");
            append_stdfields(retbuf,max,plugin,tag,0);
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
    int32_t max,sendflag,sleeptime=1,len = 0;
#ifndef BUNDLED
    portable_OS_init();
#endif
    milliseconds2();
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
    plugin->permanentflag = myatoi((char *)argv[1],2);
    plugin->daemonid = calc_nxt64bits(argv[2]);
#ifdef BUNDLED
    plugin->bundledflag = 1;
#endif
    transportstr = get_localtransport(plugin->bundledflag);
    sprintf(plugin->connectaddr,"%s://SuperNET.agents",transportstr);
    sprintf(plugin->bindaddr,"%s://%llu",transportstr,(long long)plugin->daemonid);
    jsonargs = (argc >= 3) ? ((char *)argv[3]) : 0;
    configure_plugin(retbuf,max,plugin,jsonargs,1);
    printf("CONFIGURED.(%s) argc.%d: %s myid.%llu daemonid.%llu NXT.%s serviceNXT.%s\n",plugin->name,argc,plugin->permanentflag != 0 ? "PERMANENT" : "WEBSOCKET",(long long)plugin->myid,(long long)plugin->daemonid,plugin->NXTADDR,plugin->SERVICENXT);//,jsonargs!=0?jsonargs:"");
    if ( init_pluginsocks(plugin,plugin->permanentflag,plugin->myid,plugin->daemonid,plugin->timeout) == 0 )
    {
        argjson = cJSON_Parse(jsonargs);
        if ( (len= registerAPI(registerbuf,sizeof(registerbuf)-1,plugin,argjson)) > 0 )
        {
            //fprintf(stderr,">>>>>>>>>>>>>>> plugin.(%s) sends REGISTER SEND.(%s)\n",plugin->name,registerbuf);
            nn_local_broadcast2(plugin->pushsock,0,0,(uint8_t *)registerbuf,(int32_t)strlen(registerbuf)+1), plugin->numsent++;
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
            //printf("(s%d r%d) <<<<<<<<<<<<<< %s.RECEIVED (%s).%d -> bind.(%s) connect.(%s) %s\n",plugin->numsent,plugin->numrecv,plugin->name,line,len,plugin->bindaddr,plugin->connectaddr,plugin->permanentflag != 0 ? "PERMANENT" : "WEBSOCKET"), fflush(stdout);
            if ( (len= process_plugin_json(retbuf,max,&sendflag,plugin,plugin->permanentflag,plugin->daemonid,plugin->myid,line)) > 0 )
            {
                if ( plugin->bundledflag == 0 )
                    printf("%s\n",retbuf), fflush(stdout);
                if ( sendflag != 0 )
                {
                    nn_local_broadcast2(plugin->pushsock,0,0,(uint8_t *)retbuf,(int32_t)strlen(retbuf)+1), plugin->numsent++;
                    //fprintf(stderr,">>>>>>>>>>>>>> returned.(%s)\n",retbuf);
                }
            } //else printf("null return from process_plugin_json\n");
            nn_freemsg(line);
        }
        else
        {
            startmilli = milliseconds2();
            if ( PLUGNAME(_idle)(plugin) == 0 )
            {
                if ( plugin->sleepmillis != 0 )
                {
                    sleeptime = plugin->sleepmillis - (milliseconds2() - startmilli);
                    //printf("%s sleepmillis.%d sleeptime.%d\n",plugin->name,plugin->sleepmillis,sleeptime);
                    if ( sleeptime > 0 )
                        msleep2(sleeptime);
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
