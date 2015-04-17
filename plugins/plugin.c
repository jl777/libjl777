//
//  plugins.c
//  SuperNET API extension
//

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <float.h>
#include <ctype.h>
#include "nn.h"
#include "bus.h"
#include "pair.h"
#include "pubsub.h"
#include "cJSON.c"

void randombytes(uint8_t *x,uint64_t xlen)
{
    static int fd = -1;
    int32_t i;
    if (fd == -1) {
        for (;;) {
            fd = open("/dev/urandom",O_RDONLY);
            if (fd != -1) break;
            sleep(1);
        }
    }
    while (xlen > 0) {
        if (xlen < 1048576) i = (int32_t)xlen; else i = 1048576;
        i = (int32_t)read(fd,x,i);
        if (i < 1) {
            sleep(1);
            continue;
        }
        x += i;
        xlen -= i;
    }
}

int32_t mygetline(char *line,int32_t max)
{
    struct timeval timeout;
    fd_set fdset;
    int32_t s,len = 0;
    line[0] = 0;
    FD_ZERO(&fdset);
    FD_SET(STDIN_FILENO,&fdset);
    timeout.tv_sec = 0, timeout.tv_usec = 100000;
    if ( (s= select(1,&fdset,NULL,NULL,&timeout)) < 0 )
        fprintf(stderr,"wait_for_input: error select s.%d\n",s);
    else if ( FD_ISSET(STDIN_FILENO,&fdset) == 0 || (len= (int32_t)fgets(line,max,stdin)) <= 0 )
        return(-1);//sprintf(retbuf,"{\"result\":\"no messages\",\"myid\":\"%llu\",\"counter\":%d}",(long long)myid,counter), retbuf[0] = 0;
    return(len);
}

int32_t get_newinput(char *line,int32_t max,int32_t sock,int32_t timeoutmillis)
{
    struct nn_pollfd pfd;
    int32_t rc,len;
    char *jsonstr = 0;
    line[0] = 0;
    pfd.fd = sock;
    pfd.events = NN_POLLIN | NN_POLLOUT;
    if ( (rc= nn_poll(&pfd,1,timeoutmillis)) > 0 && (pfd.revents & NN_POLLIN) != 0 && (len= nn_recv(pfd.fd,&jsonstr,NN_MSG,0)) > 0 )
    {
        strncpy(line,jsonstr,max-1);
        line[max-1] = 0;
        nn_freemsg(jsonstr);
    }
    else mygetline(line,max);
    return((int32_t)strlen(line));
}

int32_t init_daemonsock(uint64_t myid,uint64_t daemonid,int32_t timeoutmillis)
{
    int32_t sock,err;
    char addr[64];
    sprintf(addr,"ipc://%llu",(long long)myid);
    if ( (sock= nn_socket(AF_SP,NN_BUS)) < 0 )
    {
        printf("error %d nn_socket err.%s\n",sock,nn_strerror(nn_errno()));
        return(-1);
    }
    sprintf(addr,"ipc://%llu",(long long)daemonid);
    if ( (err= nn_connect(sock,addr)) < 0 )
    {
        printf("error %d nn_connect err.%s (%llu to %s)\n",sock,nn_strerror(nn_errno()),(long long)daemonid,addr);
        return(-1);
    }
    nn_setsockopt(sock,NN_SOL_SOCKET,NN_RCVTIMEO,&timeoutmillis,sizeof(timeoutmillis));
    //printf("daemonsock: %d nn_connect (%llu <-> %s)\n",sock,(long long)daemonid,addr);
    return(sock);
}

void process_daemon_json(cJSON *json)
{
    
}

int32_t process_plugin_json(uint64_t daemonid,int32_t sock,uint64_t myid,char *retbuf,long max,char *jsonstr)
{
    int32_t err,i,n,len = (int32_t)strlen(jsonstr);
    cJSON *json,*array;
    uint64_t instanceid,sender;
    char addr[64];
    retbuf[0] = 0;
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        if ( (sender= get_API_nxt64bits(cJSON_GetObjectItem(json,"myid"))) != myid )
        {
            if ( sender == daemonid )
                process_daemon_json(json);
            else printf("process message from %llu: (%s)\n",(long long)sender,jsonstr), fflush(stdout);
        }
    }
    else
    {
        if ( jsonstr[len-1] == '\r' || jsonstr[len-1] == '\n' || jsonstr[len-1] == '\t' || jsonstr[len-1] == ' ' )
            jsonstr[--len] = 0;
        if ( strcmp(jsonstr,"getpeers") == 0 )
            sprintf(retbuf,"{\"pluginrequest\":\"SuperNET\",\"requestType\":\"getpeers\"}");
        else sprintf(retbuf,"{\"result\":\"echo\",\"myid\":\"%llu\",\"message\":\"%s\"}",(long long)myid,jsonstr);
    }
    return(strlen(retbuf));
}

int main(int argc,const char *argv[])
{
    uint64_t daemonid,myid;
    int32_t rc,sock,len,counter = 0;
    char line[8192],retbuf[8192],*retstr;
    if ( argc < 2 )
    {
        printf("usage: %s <daemonid>\n",argv[0]), fflush(stdout);
        return(-1);
    }
    daemonid = atol(argv[1]);
    randombytes((uint8_t *)&myid,sizeof(myid));
    if ( (sock= init_daemonsock(myid,daemonid,10)) >= 0 )
    {
        while ( 1 )
        {
            if ( (len= get_newinput(line,sizeof(line),sock,10)) > 0 )
            {
                if ( line[len-1] == '\n' )
                    line[--len] = 0;
                printf("<<<<<<<<<<<<<< RECEIVED (%s).%d -> daemonid.%llu\n",line,len,(long long)daemonid), fflush(stdout);
                if ( (len= process_plugin_json(daemonid,sock,myid,retbuf,sizeof(retbuf),line)) > 1 )
                {
                    printf("%s\n",retbuf), fflush(stdout);
                    nn_send(sock,retbuf,len+1,0); // send the null terminator too
                }
            }
        }
        nn_shutdown(sock,0);
    }
    printf("plugin.(%s) exiting\n",argv[0]), fflush(stdout);
    return(0);
}

