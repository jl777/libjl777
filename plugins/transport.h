//
//  transport.h
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

#include "nn.h"
#include "bus.h"
#include "pair.h"
#include "pubsub.h"

void set_transportaddr(char *addr,char *transportstr,uint64_t daemonid,char *ipaddr,int32_t port,uint64_t selector)
{
    if ( ipaddr != 0 && ipaddr[0] != 0 && port != 0 )
        sprintf(addr,"tcp://%s:%llu",ipaddr,(long long)(port + selector));
    else sprintf(addr,"%s://%llu",transportstr,(long long)(daemonid + selector));
}

char *get_localtransport(int32_t bundledflag) { return((bundledflag != 0) ? "inproc" : "ipc"); }

void set_bind_transport(char *bindaddr,int32_t bundledflag,int32_t permanentflag,char *ipaddr,uint16_t port,uint64_t daemonid)
{
    set_transportaddr(bindaddr,get_localtransport(bundledflag),daemonid,ipaddr,port,2);
}

void set_connect_transport(char *connectaddr,int32_t bundledflag,int32_t permanentflag,char *ipaddr,uint16_t port,uint64_t daemonid)
{
    set_transportaddr(connectaddr,get_localtransport(bundledflag),daemonid,ipaddr,port,1);
}

void set_pair_bindconnect(char *bindaddr,char *connectaddr,int32_t bundledflag,int32_t permanentflag,uint64_t myid,uint64_t instanceid)
{
    uint64_t xored;
    char *transportstr;
    connectaddr[0] = bindaddr[0] = 0;
    if ( myid != instanceid )
    {
        xored = (myid ^ instanceid);
        transportstr = get_localtransport(bundledflag);
        if ( permanentflag != 0 )
            sprintf(bindaddr,"%s://%llu",transportstr,(long long)xored);
        else sprintf(connectaddr,"%s://%llu",transportstr,(long long)xored);
    }
}

int32_t init_permpairsock(int32_t permanentflag,char *bindaddr,char *connectaddr,uint64_t instanceid)
{
    int32_t sock,err,to = 1;
    printf("<<<<<<<<<<<<< init_permpairsocks bind.(%s) connect.(%s)\n",bindaddr,connectaddr);
    if ( (sock= nn_socket(AF_SP,(permanentflag == 0) ? NN_BUS : NN_BUS)) < 0 )
    {
        printf("error %d nn_socket err.%s\n",sock,nn_strerror(nn_errno()));
        return(-1);
    }
    if ( (err= nn_bind(sock,bindaddr)) < 0 )
    {
        printf("error %d nn_bind.%d (%s) | %s\n",err,sock,bindaddr,nn_strerror(nn_errno()));
        return(-1);
    }
    //if ( permanentflag != 0 )
    {
        if ( (err= nn_connect(sock,connectaddr)) < 0 )
        {
            printf("error %d nn_connect err.%s (%s)\n",err,nn_strerror(nn_errno()),connectaddr);
            return(-1);
        }
    }
    assert(nn_setsockopt(sock,NN_SOL_SOCKET,NN_RCVTIMEO,&to,sizeof (to)) >= 0);
    return(sock);
}

int32_t choose_socket(struct daemon_info *dp,int32_t permanentflag,int32_t ind,uint64_t instanceid)
{
    char bindaddr[64],connectaddr[64];
    int32_t sock,err;
    if ( instanceid == 0 || ind < 0 )
        return(dp->permsock);
    else if ( (sock= dp->pairsocks[ind]) < 0 )
    {
        if ( (sock= nn_socket(AF_SP,NN_PAIR)) < 0 )
        {
            printf("error %d PAIR.%d nn_socket err.%s\n",sock,ind,nn_strerror(nn_errno()));
            return(-1);
        }
        dp->pairsocks[ind] = sock;
    }
    set_pair_bindconnect(bindaddr,connectaddr,dp->main != 0,permanentflag,dp->myid,instanceid);
    if ( bindaddr[0] != 0 )
    {
        if ( (err= nn_bind(sock,bindaddr)) < 0 )
        {
            printf("error %d nn_bind.%d (%s) | %s\n",err,sock,bindaddr,nn_strerror(nn_errno()));
            return(-1);
        }
    }
    if ( connectaddr[0] != 0 )
    {
        if ( (err= nn_connect(sock,connectaddr)) < 0 )
        {
            printf("error %d nn_connect err.%s (%s)\n",err,nn_strerror(nn_errno()),connectaddr);
            return(-1);
        }
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
    i = rand() % (sizeof(dp->instanceids)/sizeof(*dp->instanceids));
    if ( dp->pairsocks[i] >= 0 )
        nn_shutdown(dp->pairsocks[i],0);
    dp->instanceids[i] = instanceid;
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

void connect_instanceid(struct daemon_info *dp,uint64_t instanceid,int32_t permanentflag)
{
    int32_t err;
    cJSON *json;
    char addr[64],numstr[64],*jsonstr;
    if ( permanentflag == 0 )
    {
        sprintf(addr,"ipc://%llu",(long long)instanceid);
        if ( (err= nn_connect(dp->permsock,addr)) < 0 )
        {
            printf("error %d nn_connect err.%s (%llu to %s)\n",dp->permsock,nn_strerror(nn_errno()),(long long)dp->daemonid,addr);
            return;
        }
        printf("connect_instanceid: %d nn_connect (%llu <-> %s)\n",dp->permsock,(long long)dp->daemonid,addr);
    }
    json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"requestType",cJSON_CreateString("instances"));
    cJSON_AddItemToObject(json,"instanceids",instances_json(dp));
    sprintf(numstr,"%llu",(long long)dp->daemonid), cJSON_AddItemToObject(json,"myid",cJSON_CreateString(numstr));
    jsonstr = cJSON_Print(json);
    stripwhite(jsonstr,strlen(jsonstr));
    free_json(json);
    nn_send(dp->permsock,jsonstr,strlen(jsonstr) + 1,0);
    free(jsonstr);
}

int32_t send_to_daemon(char *name,uint64_t daemonid,uint64_t instanceid,char *jsonstr)
{
    struct daemon_info *find_daemoninfo(int32_t *indp,char *name,uint64_t daemonid,uint64_t instanceid);
    struct daemon_info *dp;
    int32_t len,ind,permanentflag = 1;
    cJSON *json;
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        free_json(json);
        if ( (dp= find_daemoninfo(&ind,name,daemonid,instanceid)) != 0 )
        {
            if ( (len= (int32_t)strlen(jsonstr)) > 0 )
                 return(nn_send(choose_socket(dp,permanentflag,ind,instanceid),jsonstr,len + 1,0));
            else printf("send_to_daemon: error jsonstr.(%s)\n",jsonstr);
        }
    }
    else printf("send_to_daemon: cant parse jsonstr.(%s)\n",jsonstr);
    return(-1);
}
