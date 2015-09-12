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

#include "../../nanomsg/src/nn.h"
#include "../../nanomsg/src/bus.h"
#include "../../nanomsg/src/pubsub.h"
#include "../../nanomsg/src/pipeline.h"
#include "../../nanomsg/src/reqrep.h"
#include "../../nanomsg/src/survey.h"
#include "../../nanomsg/src/pair.h"
#include "../../nanomsg/src/pubsub.h"

#define LOCALCAST 1
#define BROADCAST 2
#define OFFSET_ENABLED (bundledflag == 0)
char *get_localtransport(int32_t bundledflag) { return(OFFSET_ENABLED ? "ipc" : "inproc"); }

void set_connect_transport(char *connectaddr,int32_t bundledflag,int32_t permanentflag,char *ipaddr,uint16_t port,uint64_t daemonid)
{
    //set_transportaddr(connectaddr,get_localtransport(bundledflag),daemonid,ipaddr,port,1*OFFSET_ENABLED);
    sprintf(connectaddr,"%s://%llu",get_localtransport(bundledflag),(long long)daemonid);
}

int32_t init_pluginhostsocks(struct daemon_info *dp,char *connectaddr)
{
    if ( (dp->pushsock= nn_socket(AF_SP,NN_BUS)) < 0 )
    {
        printf("error creating dp->pushsock %s\n",nn_strerror(nn_errno()));
        return(-1);
    }
    else if ( nn_settimeouts(dp->pushsock,10,1) < 0 )
    {
        printf("error setting dp->pushsock timeouts %s\n",nn_strerror(nn_errno()));
        return(-1);
    }
    else if ( nn_connect(dp->pushsock,connectaddr) < 0 )
    {
        printf("error connecting dp->pushsock.%d to %s %s\n",dp->pushsock,connectaddr,nn_strerror(nn_errno()));
        return(-1);
    }
    printf("host.%s connected.(%s) %d\n",dp->name,connectaddr,dp->pushsock);
    return(0);
}

int32_t add_tagstr(struct daemon_info *dp,uint64_t tag,char **dest,int32_t retsock)
{
    int32_t i;
    //printf("ADDTAG.%llu <- %p\n",(long long)tag,dest);
    for (i=0; i<NUM_PLUGINTAGS; i++)
    {
        if ( SUPERNET.tags[i][0] == tag )
            return(-1);
        if ( SUPERNET.tags[i][0] == 0 )
        {
            if ( Debuglevel > 2 )
                printf("dp.%p %s slot.%d <- tag.%llu dest.%p\n",dp,dp->name,i,(long long)tag,dest);
            SUPERNET.tags[i][0] = tag, SUPERNET.tags[i][1] = (uint64_t)dest, SUPERNET.tags[i][2] = (uint64_t)retsock;
            return(i);
        }
    }
    printf("add_tagstr: no place for tag.%llu\n",(long long)tag);
    return(-1);
}

char **get_tagstr(int32_t *retsockp,struct daemon_info *dp,uint64_t tag)
{
    int32_t i;
    char **dest;
    for (i=0; i<NUM_PLUGINTAGS; i++)
    {
        if ( SUPERNET.tags[i][0] == tag )
        {
            dest = (char **)SUPERNET.tags[i][1];
            if ( SUPERNET.tags[i][2] != 0 )
                *retsockp = (int32_t)SUPERNET.tags[i][2];
            SUPERNET.tags[i][0] = SUPERNET.tags[i][1] = SUPERNET.tags[i][2] = 0;
            if ( Debuglevel > 2 )
                printf("dp.%p %s slot.%d found tag.%llu dest.%p\n",dp,dp->name,i,(long long)tag,dest);
            return(dest);
        }
    }
    printf("get_tagstr: dp.%p %s cant find tag.%llu [0] %llu\n",dp,dp->name,(long long)tag,(long long)SUPERNET.tags[0][0]);
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
 
uint64_t send_to_daemon(int32_t sock,char **retstrp,char *name,uint64_t daemonid,uint64_t instanceid,char *origjsonstr,int32_t len,int32_t localaccess,char *tokenstr)
{
    struct daemon_info *find_daemoninfo(int32_t *indp,char *name,uint64_t daemonid,uint64_t instanceid);
    struct daemon_info *dp;
    char numstr[64],*tmpstr,*jsonstr; uint8_t *data; int32_t duplicateflag = 0,ind,datalen,tmplen,flag = 0; uint64_t tmp,tag = 0; cJSON *json;
    if ( Debuglevel > 2 )
        printf("A.local %d send_to_daemon.(%s).%d\n",localaccess,origjsonstr,len);
    if ( (json= cJSON_Parse(origjsonstr)) != 0 )
    {
        jsonstr = origjsonstr;
        if ( localaccess != 0 )
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
                    randombytes((void *)&tag,sizeof(tag)), flag = 1;
                if ( Debuglevel > 2 )
                    printf("tag.%llu flag.%d tmp.%llu datalen.%d\n",(long long)tag,flag,(long long)tmp,datalen);
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
        else
        {
            if ( is_cJSON_Array(json) != 0 && cJSON_GetArraySize(json) == 2 )
                tag = get_API_nxt64bits(cJSON_GetObjectItem(cJSON_GetArrayItem(json,0),"tag"));
            else tag = get_API_nxt64bits(cJSON_GetObjectItem(json,"tag"));
        }
        if ( len == 0 )
            len = (int32_t)strlen(jsonstr) + 1;
        free_json(json);
        if ( (dp= find_daemoninfo(&ind,name,daemonid,instanceid)) != 0 )
        {
            if ( Debuglevel > 2 )
                printf("after find_daemoninfo send_to_daemon.(%s) tag.%llu dp.%p len.%d vs %ld retstrp.%p\n",jsonstr,(long long)tag,dp,len,strlen(jsonstr)+1,retstrp);
            if ( len > 0 )
            {
                 if ( tag != 0 )
                    duplicateflag = add_tagstr(dp,tag,retstrp,sock) < 0;
                if ( Debuglevel > 2 )
                    fprintf(stderr,"HAVETAG.%llu send_to_daemon(%s) sock.%d duplicateflag.%d\n",(long long)tag,jsonstr,sock,duplicateflag);
                if ( duplicateflag == 0 )
                {
                    dp->numsent++;
                    if ( tokenstr != 0 )
                    {
                        tmpstr = calloc(1,len + strlen(tokenstr) + 5);
                        //fprintf(stderr,"add tokenstr.(%s)\n",tokenstr);
                        sprintf(tmpstr,"[%s, %s]",jsonstr,tokenstr);
                        len = (int32_t)strlen(tmpstr) + 1;
                        if ( flag != 0 )
                            free(jsonstr);
                        jsonstr = tmpstr, flag = 1;
                        //fprintf(stderr,"added tokenstr.(%s)\n",jsonstr);
                    }
                    if ( nn_local_broadcast(dp->pushsock,instanceid,instanceid != 0 ? 0 : LOCALCAST,(uint8_t *)jsonstr,len) < 0 )
                        printf("error sending to daemon %s\n",nn_strerror(nn_errno()));
                } //else tag = 0;
            } else printf("send_to_daemon: error jsonstr.(%s)\n",jsonstr);
        } else printf("cant find (%s) for.(%s)\n",name,jsonstr);
        //printf("dp.%p (%s) tag.%llu\n",dp,jsonstr,(long long)tag);
        if ( flag != 0 )
            free(jsonstr);
        return(tag);
    }
    else printf("send_to_daemon: cant parse jsonstr.(%s)\n",origjsonstr);
    return(0);
}

