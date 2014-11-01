//
//  main.c
//  libtest
//
//  Created by jl777 on 8/13/14.
//  Copyright (c) 2014 jl777. MIT License.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <memory.h>
#include <sys/time.h>
#include "SuperNET.h"
#include "cJSON.h"

extern int32_t IS_LIBTEST;
extern cJSON *MGWconf;
char *bitcoind_RPC(void *deprecated,char *debugstr,char *url,char *userpass,char *command,char *params);

cJSON *SuperAPI(char *cmd,char *field0,char *arg0,char *field1,char *arg1)
{
    cJSON *json;
    char params[1024],*retstr;
    if ( field0 != 0 && field0[0] != 0 )
    {
        if ( field0 != 0 && field0[0] != 0 )
            sprintf(params,"{\"requestType\":\"%s\",\"%s\":\"%s\",\"%s\":\"%s\"}",cmd,field0,arg0,field1,arg1);
        else sprintf(params,"{\"requestType\":\"%s\",\"%s\":\"%s\"}",cmd,field0,arg0);
    }
    else sprintf(params,"{\"requestType\":\"%s\"}",cmd);
    retstr = bitcoind_RPC(0,(char *)"BTCD",(char *)"https://127.0.0.1:7777",(char *)"",(char *)"SuperNET",params);
    if ( retstr != 0 )
    {
        json = cJSON_Parse(retstr);
        free(retstr);
    }
    return(json);
}

void build_topology()
{
    cJSON *array,*item,*ret;
    uint32_t now;
    int32_t i,n,numnodes,numcontacts,numipaddrs = 0;
    char ipaddr[64],**ipaddrs;
    struct nodestats **nodes;
    struct contact_info **contacts;
    array = cJSON_GetObjectItem(MGWconf,"whitelist");
    if ( array != 0 && is_cJSON_Array(array) != 0 )
    {
        int32_t add_SuperNET_whitelist(char *ipaddr);
        n = cJSON_GetArraySize(array);
        ipaddrs = calloc(n+1,sizeof(*ipaddrs));
        for (i=numipaddrs=0; i<n; i++)
        {
            if ( array == 0 || n == 0 )
                break;
            item = cJSON_GetArrayItem(array,i);
            copy_cJSON(ipaddr,item);
            if ( ipaddr[0] != 0 && (ret= SuperAPI("ping","destip",ipaddr,0,0)) != 0 )
            {
                ipaddrs[numipaddrs] = calloc(1,strlen(ipaddr)+1);
                strcpy(ipaddrs[numipaddrs++],ipaddr);
                free_json(ret);
            }
        }
    }
    if ( ipaddrs != 0 )
        for (i=0; i<numipaddrs; i++)
            printf("%s ",ipaddrs[i]);
    printf("numipaddrs.%d\n",numipaddrs);
    while ( 1 )
    {
        nodes = (struct nodestats **)copy_all_DBentries(&numnodes,NODESTATS_DATA);
        contacts = (struct contact_info **)copy_all_DBentries(&numcontacts,CONTACT_DATA);
        if ( nodes != 0 )
        {
            now = (uint32_t)time(NULL);
            for (i=0; i<numnodes; i++)
            {
                printf("(%llu %d) ",(long long)nodes[i]->nxt64bits,nodes[i]->lastcontact-now);
                free(nodes[i]);
            }
            free(nodes);
        }
        printf("numnodes.%d\n",numnodes);
        if ( contacts != 0 )
        {
            for (i=0; i<numcontacts; i++)
            {
                printf("((%s) %llu) ",contacts[i]->handle,(long long)contacts[i]->nxt64bits);
                free(contacts[i]);
            }
            free(contacts);
        }
        printf("numcontacts.%d\n",numcontacts);
        sleep(10);
    }
}

void *GUIpoll_loop(void *arg)
{
    void unstringify(char *);
    char params[4096],txidstr[64],buf[1024],ipaddr[64],args[8192],*retstr;
    int32_t port;
    cJSON *json;
    while ( 1 )
    {
        sleep(1);
        //continue;
        sprintf(params,"{\"requestType\":\"GUIpoll\"}");
        retstr = bitcoind_RPC(0,(char *)"BTCD",(char *)"http://127.0.0.1:7776",(char *)"",(char *)"SuperNET",params);
        //fprintf(stderr,"<<<<<<<<<<< SuperNET poll_for_broadcasts: issued bitcoind_RPC params.(%s) -> retstr.(%s)\n",params,retstr);
        if ( retstr != 0 )
        {
            //sprintf(retbuf+sizeof(ptrs),"{\"result\":%s,\"from\":\"%s\",\"port\":%d,\"args\":%s}",str,ipaddr,port,args);
            if ( (json= cJSON_Parse(retstr)) != 0 )
            {
                copy_cJSON(buf,cJSON_GetObjectItem(json,"result"));
                if ( buf[0] != 0 )
                {
                    unstringify(buf);
                    copy_cJSON(txidstr,cJSON_GetObjectItem(json,"txid"));
                    if ( txidstr[0] != 0 )
                        fprintf(stderr,"<<<<<<<<<<< GUI poll_for_broadcasts: (%s) for [%s]\n",buf,txidstr);
                    else
                    {
                        copy_cJSON(ipaddr,cJSON_GetObjectItem(json,"from"));
                        copy_cJSON(args,cJSON_GetObjectItem(json,"args"));
                        unstringify(args);
                        port = (int32_t)get_API_int(cJSON_GetObjectItem(json,"port"),0);
                        if ( args[0] != 0 )
                            printf("(%s) from (%s:%d) -> (%s)\n",args,ipaddr,port,buf);
                    }
                }
                free_json(json);
            } else fprintf(stderr,"<<<<<<<<<<< GUI poll_for_broadcasts: PARSE_ERROR.(%s)\n",retstr);
            free(retstr);
        } //else fprintf(stderr,"<<<<<<<<<<< BTCD poll_for_broadcasts: bitcoind_RPC returns null\n");
    }
    return(0);
}

int main(int argc,const char *argv[])
{
    FILE *fp;
    int32_t retval;
    char ipaddr[64];
    IS_LIBTEST = 1;
    if ( argc > 1 && argv[1] != 0 && strlen(argv[1]) < 32 )
        strcpy(ipaddr,argv[1]);
    else strcpy(ipaddr,"127.0.0.1");
    retval = SuperNET_start("SuperNET.conf",ipaddr);
    if ( (fp= fopen("horrible.hack","wb")) != 0 )
    {
        fwrite(&retval,1,sizeof(retval),fp);
        fclose(fp);
    }
    if ( retval == 0 )
    {
        if ( portable_thread_create((void *)GUIpoll_loop,ipaddr) == 0 )
            printf("ERROR hist process_hashtablequeues\n");
        else build_topology();
    }
    while ( 1 ) sleep(60);
    /*
         memset(buf,0,sizeof(buf));
        fgets(buf,sizeof(buf),stdin);
        stripwhite_ns(buf,(int32_t)strlen(buf));
        if ( strcmp("p",buf) == 0 )
            strcpy(buf2,"\"getpeers\"}'");
        else if ( strcmp("q",buf) == 0 )
            exit(0);
        else if ( buf[0] == 'P' && buf[1] == ' ' )
            sprintf(buf2,"\"ping\",\"destip\":\"%s\"}'",buf+2);
        else strcpy(buf2,buf);
        sprintf(cmdstr,"{\"requestType\":%s",buf2);
        retstr = SuperNET_JSON(cmdstr);
        printf("input.(%s) -> (%s)\n",cmdstr,retstr);
        if ( retstr != 0 )
            free(retstr);*/
    return(0);
}

// stubs
int32_t SuperNET_broadcast(char *msg,int32_t duration) { return(0); }
int32_t SuperNET_narrowcast(char *destip,unsigned char *msg,int32_t len) { return(0); }
