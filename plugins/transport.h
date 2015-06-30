//
//  transport.h
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

#include "nn.h"
#include "bus.h"
#include "pubsub.h"
#include "pipeline.h"
#include "reqrep.h"
#include "survey.h"
#include "pair.h"
#include "pubsub.h"
#include "system777.c"

#define LOCALCAST 1
#define BROADCAST 2

void set_transportaddr(char *addr,char *transportstr,uint64_t daemonid,char *ipaddr,int32_t port,uint64_t selector)
{
    if ( ipaddr != 0 && ipaddr[0] != 0 && port != 0 )
        sprintf(addr,"tcp://%s:%llu",ipaddr,(long long)(port + selector));
    else sprintf(addr,"%s://%llu",transportstr,(long long)(daemonid + selector));
}

void set_bind_transport(char *bindaddr,int32_t bundledflag,int32_t permanentflag,char *ipaddr,uint16_t port,uint64_t daemonid)
{
    set_transportaddr(bindaddr,get_localtransport(bundledflag),daemonid,ipaddr,port,2*OFFSET_ENABLED);
}

void set_connect_transport(char *connectaddr,int32_t bundledflag,int32_t permanentflag,char *ipaddr,uint16_t port,uint64_t daemonid)
{
    set_transportaddr(connectaddr,get_localtransport(bundledflag),daemonid,ipaddr,port,1*OFFSET_ENABLED);
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

int32_t iteration_recvsocket(struct daemon_info *dp,int32_t counter)
{
    int32_t sock,ind;
    struct allendpoints *socks;
    ind = ((counter >> 1) % ((sizeof(dp->perm.socks.recv)/sizeof(int32_t))) + 1);
    if ( (counter & 1) == 0 )
        socks = &dp->perm.socks;
    else socks = &dp->wss.socks;
    sock = (ind == 0) ? socks->both.bus : ((int32_t *)&socks->recv)[ind - 1];
    return(sock);
}

int32_t init_pluginhostsocks(struct daemon_info *dp,int32_t permanentflag,char *bindaddr,char *connectaddr,uint64_t instanceid)
{
    int32_t errs = 0;
    struct allendpoints *socks;
    if ( permanentflag != 0 )
        socks = &dp->perm.socks;
    else socks = &dp->wss.socks;
    if ( Debuglevel > 2 )
        printf("<<<<<<<<<<<<< init_permpairsocks bind.(%s) connect.(%s)\n",bindaddr,connectaddr);
#ifdef _WIN32
    if ( (socks->both.bus= init_socket("","bus",NN_BUS,bindaddr,0,1)) < 0 ) errs++;
#else
    if ( (socks->both.pair= init_socket(".pair","pair",NN_PAIR,bindaddr,0,1)) < 0 ) errs++;
#endif
    //if ( (socks->send.push= init_socket(".pipeline","push",NN_PUSH,bindaddr,0,1)) < 0 ) errs++;
    //if ( (socks->send.rep= init_socket(".reqrep","rep",NN_REP,0,connectaddr,1)) < 0 ) errs++;
    //if ( (socks->send.pub= init_socket(".pubsub","pub",NN_PUB,bindaddr,0,1)) < 0 ) errs++;
    //if ( (socks->send.survey= init_socket(".survey","surveyor",NN_SURVEYOR,bindaddr,0,1)) < 0 ) errs++;
    //if ( (socks->recv.pull= init_socket(".pipeline","pull",NN_PULL,0,connectaddr,0)) < 0 ) errs++;
    //if ( (socks->recv.req= init_socket(".reqrep","req",NN_REQ,bindaddr,connectaddr,0)) < 0 ) errs++;
    //if ( (socks->recv.sub= init_socket(".pubsub","sub",NN_SUB,0,connectaddr,0)) < 0 ) errs++;
    //if ( (socks->recv.respond= init_socket(".survey","respondent",NN_RESPONDENT,0,connectaddr,0)) < 0 ) errs++;
    return(errs);
}

int32_t connect_instanceid(struct daemon_info *dp,uint64_t instanceid)
{
    char addr[64];
    int32_t err = -1;
    sprintf(addr,"%s://%llu",get_localtransport(dp->bundledflag),(long long)instanceid);
    printf("need to connect to (%s) efficiently\n",addr);
    return(err);
}

int32_t add_tagstr(struct daemon_info *dp,uint64_t tag,char **dest,struct relayargs *args)
{
    int32_t i;
    //printf("ADDTAG.%llu <- %p\n",(long long)tag,dest);
    for (i=0; i<NUM_PLUGINTAGS; i++)
    {
        if ( dp->tags[i][0] == 0 )
        {
            if ( Debuglevel > 2 )
                printf("slot.%d <- tag.%llu dest.%p\n",i,(long long)tag,dest);
            dp->tags[i][0] = tag, dp->tags[i][1] = (uint64_t)dest, dp->tags[i][2] = (uint64_t)args;
            return(i);
        }
    }
    printf("add_tagstr: no place for tag.%llu\n",(long long)tag);
    return(-1);
}

char **get_tagstr(struct relayargs **argsp,struct daemon_info *dp,uint64_t tag)
{
    int32_t i;
    char **dest;
    for (i=0; i<NUM_PLUGINTAGS; i++)
    {
        if ( dp->tags[i][0] == tag )
        {
            dest = (char **)dp->tags[i][1];
            if ( dp->tags[i][2] != 0 )
                *argsp = (struct relayargs *)dp->tags[i][2];
            dp->tags[i][0] = dp->tags[i][1] = dp->tags[i][2] = 0;
            if ( Debuglevel > 2 )
                printf("slot.%d found tag.%llu dest.%p\n",i,(long long)tag,dest);
            return(dest);
        }
    }
    printf("get_tagstr: cant find tag.%llu [0] %llu\n",(long long)tag,(long long)dp->tags[0][0]);
    return(0);
}

char *wait_for_daemon(char **destp,uint64_t tag,int32_t timeout,int32_t sleepmillis)
{
    int32_t poll_daemons();
    static long counter,sum;
    char retbuf[512];
    double startmilli = milliseconds();
    char *retstr;
    usleep(5);
    while ( milliseconds() < (startmilli + timeout) )
    {
        poll_daemons();
        if ( (retstr= *destp) != 0 )
        {
            counter++;
            if ( (counter % 10000) == 0 )
                printf("%ld: ave %.1f\n",counter,(double)sum/counter);
            //printf("wait_for_daemon.(%s) %p destp.%p\n",retstr,retstr,destp);
            return(retstr);
        }
        if ( sleepmillis != 0 )
            msleep(sleepmillis);
    }
    sprintf(retbuf,"{\"error\":\"timeout\",\"tag\":\"%llu\",\"elapsed\":\"%.2f\"}",(long long)tag,milliseconds() - startmilli);
    *destp = clonestr(retbuf);
    return(*destp);
}
 
uint64_t send_to_daemon(struct relayargs *args,char **retstrp,char *name,uint64_t daemonid,uint64_t instanceid,char *origjsonstr,int32_t len,int32_t localaccess)
{
    struct daemon_info *find_daemoninfo(int32_t *indp,char *name,uint64_t daemonid,uint64_t instanceid);
    struct daemon_info *dp;
    char numstr[64],*tmpstr,*jsonstr,*tokbuf,*broadcastmode; uint8_t *data; int32_t ind,datalen,tmplen,flag = 0; uint64_t tmp,tag = 0; cJSON *json;
//printf("A send_to_daemon.(%s).%d\n",origjsonstr,len);
    if ( (json= cJSON_Parse(origjsonstr)) != 0 )
    {
        jsonstr = origjsonstr;
        //if ( localaccess != 0 )
        {
            tmplen = (int32_t)strlen(origjsonstr)+1;
            if ( len > tmplen )
            {
                data = (uint8_t *)&origjsonstr[tmplen];
                datalen = (len - tmplen);
            } else data = 0, datalen = 0;
            tmp = get_API_nxt64bits(cJSON_GetObjectItem(json,"tag"));
            if ( retstrp != 0 )
            {
                if ( tmp != 0 )
                    tag = tmp, flag = 1;
                if ( tag == 0 )
                    tag = (((uint64_t)rand() << 32) | rand()), flag = 1;
//printf("tag.%llu flag.%d tmp.%llu datalen.%d\n",(long long)tag,flag,(long long)tmp,datalen);
                if ( flag != 0 )
                {
                    sprintf(numstr,"%llu",(long long)tag), ensure_jsonitem(json,"tag",numstr);
                    ensure_jsonitem(json,"NXT",SUPERNET.NXTADDR);
                    if ( localaccess != 0 && cJSON_GetObjectItem(json,"time") == 0 )
                        cJSON_AddItemToObject(json,"time",cJSON_CreateNumber(time(NULL)));
                    jsonstr = cJSON_Print(json), _stripwhite(jsonstr,' ');
                    tmplen = (int32_t)strlen(jsonstr) + 1;
                    if ( datalen != 0 )
                    {
                        tmpstr = malloc(tmplen + datalen);
                        memcpy(tmpstr,jsonstr,tmplen);
                        memcpy(&tmpstr[tmplen],data,datalen);
                        free(jsonstr), jsonstr = tmpstr, len = tmplen + datalen;
                    } else len = tmplen;
                }
            } else tag = tmp;
        }
        if ( len == 0 )
            len = (int32_t)strlen(jsonstr) + 1;
        if ( localaccess != 0 && is_cJSON_Array(json) == 0 )
        {
            tokbuf = calloc(1,len + 1024);
            //printf("jsonstr.(%s)\n",jsonstr);
            broadcastmode = get_broadcastmode(json,cJSON_str(cJSON_GetObjectItem(json,"broadcast")));
            len = construct_tokenized_req(tokbuf,jsonstr,SUPERNET.NXTACCTSECRET,broadcastmode);
            if ( flag != 0 )
                free(jsonstr);
            jsonstr = tokbuf, flag = 1;
        }
printf("localaccess.%d send_to_daemon.(%s) tag.%llu\n",localaccess,jsonstr,(long long)tag);
        free_json(json);
        if ( (dp= find_daemoninfo(&ind,name,daemonid,instanceid)) != 0 )
        {
//printf("send_to_daemon.(%s) tag.%llu dp.%p len.%d vs %ld retstrp.%p\n",jsonstr,(long long)tag,dp,len,strlen(jsonstr)+1,retstrp);
            if ( len > 0 )
            {
                if ( Debuglevel > 2 )
                    fprintf(stderr,"HAVETAG.%llu send_to_daemon(%s) args.%p\n",(long long)tag,jsonstr,args);
                if ( tag != 0 )
                    add_tagstr(dp,tag,retstrp,args);
                dp->numsent++;
                if ( nn_local_broadcast(&dp->perm.socks,instanceid,instanceid != 0 ? 0 : LOCALCAST,(uint8_t *)jsonstr,len) < 0 )
                    printf("error sending to daemon %s\n",nn_strerror(nn_errno()));
            }
            else printf("send_to_daemon: error jsonstr.(%s)\n",jsonstr);
        } else printf("cant find (%s) for.(%s)\n",name,jsonstr);
        //printf("dp.%p (%s) tag.%llu\n",dp,jsonstr,(long long)tag);
        if ( flag != 0 )
            free(jsonstr);
        return(tag);
    }
    else printf("send_to_daemon: cant parse jsonstr.(%s)\n",origjsonstr);
    return(0);
}
