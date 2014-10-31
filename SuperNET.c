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
#include "SuperNET.h"
#include "cJSON.h"

extern int32_t IS_LIBTEST;
char *bitcoind_RPC(void *deprecated,char *debugstr,char *url,char *userpass,char *command,char *params);

int main(int argc,const char *argv[])
{
    long stripwhite_ns(char *buf,long len);
    FILE *fp;
    int32_t retval;
    cJSON *json;
    int32_t duration,len;
    unsigned char data[4098];
    char params[4096],txidstr[64],buf[1024],ipaddr[64],*retstr;
    IS_LIBTEST = 1;
    if ( argc > 1 && argv[1] != 0 && strlen(argv[1]) < 32 )
        strcpy(ipaddr,argv[1]);
    else ipaddr[0] = 0;
    retval = SuperNET_start("SuperNET.conf",ipaddr);
    if ( (fp= fopen("horrible.hack","wb")) != 0 )
    {
        fwrite(&retval,1,sizeof(retval),fp);
        fclose(fp);
    }
    while ( retval == 0 )
    {
        sleep(3);
        //continue;
        sprintf(params,"{\"requestType\":\"GUIpoll\"}");
        retstr = bitcoind_RPC(0,(char *)"BTCD",(char *)"https://127.0.0.1:7777",(char *)"",(char *)"SuperNET",params);
        //fprintf(stderr,"<<<<<<<<<<< SuperNET poll_for_broadcasts: issued bitcoind_RPC params.(%s) -> retstr.(%s)\n",params,retstr);
        if ( retstr != 0 )
        {
            if ( (json= cJSON_Parse(retstr)) != 0 )
            {
                copy_cJSON(buf,cJSON_GetObjectItem(json,"result"));
                if ( buf[0] != 0 )
                {
                    unstringify(buf);
                    copy_cJSON(txidstr,cJSON_GetObjectItem(json,"txid"));
                    if ( txidstr[0] != 0 )
                        fprintf(stderr,"<<<<<<<<<<< GUI poll_for_broadcasts: (%s) for [%s]\n",buf,txidstr);
                }
                free_json(json);
            } else fprintf(stderr,"<<<<<<<<<<< GUI poll_for_broadcasts: PARSE_ERROR.(%s)\n",retstr);
            free(retstr);
        } //else fprintf(stderr,"<<<<<<<<<<< BTCD poll_for_broadcasts: bitcoind_RPC returns null\n");
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
    }
    return(0);
}

int32_t SuperNET_broadcast(char *msg,int32_t duration) { return(0); }
int32_t SuperNET_narrowcast(char *destip,unsigned char *msg,int32_t len) { return(0); }
