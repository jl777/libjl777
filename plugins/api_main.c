//
//  api_main.c
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//
#include <stdint.h>
#include "ccgi.h"
#include "nn.h"
#include "cJSON.h"
#include "pair.h"
#include "pipeline.h"
uint32_t _crc32(uint32_t crc,const void *buf,size_t size);
long _stripwhite(char *buf,int accept);
#define nn_errstr() nn_strerror(nn_errno())
#define SUPERNET_APIENDPOINT "tcp://127.0.0.1:7776"

void process_json(cJSON *json)
{
    int32_t sock,i,len,checklen,sendtimeout,recvtimeout; uint32_t tag;
    char endpoint[128],*resultstr,*jsonstr;
    jsonstr = cJSON_Print(json), _stripwhite(jsonstr,' ');
    //printf("jsonstr.(%s)\r\n",jsonstr);
    len = (int32_t)strlen(jsonstr)+1;
    tag = _crc32(0,jsonstr,len);
    sprintf(endpoint,"ipc://api.%u",tag);
    free(jsonstr);
    recvtimeout = sendtimeout = 5000;
    cJSON_AddItemToObject(json,"apitag",cJSON_CreateString(endpoint));
    cJSON_AddItemToObject(json,"timeout",cJSON_CreateNumber(recvtimeout));
    jsonstr = cJSON_Print(json), _stripwhite(jsonstr,' ');
    len = (int32_t)strlen(jsonstr)+1;
    if ( json != 0 )
    {
        if ( (sock= nn_socket(AF_SP,NN_PAIR)) >= 0 )
        {
            if ( sendtimeout > 0 && nn_setsockopt(sock,NN_SOL_SOCKET,NN_SNDTIMEO,&sendtimeout,sizeof(sendtimeout)) < 0 )
                fprintf(stderr,"error setting sendtimeout %s\n",nn_errstr());
            if ( nn_connect(sock,SUPERNET_APIENDPOINT) < 0 )
                printf("error connecting to apiendpoint sock.%d type.%d (%s) %s\r\n",sock,NN_PUSH,SUPERNET_APIENDPOINT,nn_errstr());
            else if ( (checklen= nn_send(sock,jsonstr,len,0)) != len )
                printf("checklen.%d != len.%d for nn_send to (%s)\r\n",checklen,len,SUPERNET_APIENDPOINT);
            else
            {
                if ( recvtimeout > 0 && nn_setsockopt(sock,NN_SOL_SOCKET,NN_RCVTIMEO,&recvtimeout,sizeof(recvtimeout)) < 0 )
                    fprintf(stderr,"error setting sendtimeout %s\n",nn_errstr());
                if ( nn_recv(sock,&resultstr,NN_MSG,0) > 0 )
                {
                    printf("Content-Length: %ld\r\n\r\n",strlen(resultstr)+2);
                    printf("%s\r\n",resultstr);
                    nn_freemsg(resultstr);
                } else printf("error getting results %s\r\n",nn_errstr());
            }
            nn_shutdown(sock,0);
        } else printf("error getting pushsock.%s\r\n",nn_errstr());
    }
    free(jsonstr);
}

int main(int argc, char **argv)
{
    CGI_varlist *varlist; const char *name; CGI_value  *value;  int i,j,iter; cJSON *json;
    fputs("Access-Control-Allow-Origin: null\r\n",stdout);
    fputs("Access-Control-Allow-Headers: Authorization, Content-Type\r\n",stdout);
    fputs("Access-Control-Allow-Credentials: true\r\n",stdout);
    fputs("Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n",stdout);
    fputs("Content-type: text/plain\r\n",stdout);
    json = cJSON_CreateObject();
    for (i=j=0; argv[0][i]!=0; i++)
        if ( argv[0][i] == '/' || argv[0][i] == '\\' )
            j = i+1;
    cJSON_AddItemToObject(json,"agent",cJSON_CreateString(&argv[0][j]));
    for (iter=0; iter<2; iter++)
    {
        if ( (varlist= ((iter==0) ? CGI_get_post(0,0) : CGI_get_query(0))) != 0 )
        {
            for (name=CGI_first_name(varlist); name!=0; name=CGI_next_name(varlist))
            {
                value = CGI_lookup_all(varlist,0);
                for (i=0; value[i]!=0; i++)
                {
                    //printf("%s [%d] = %s\r\n", name, i, value[i]);
                    if ( i == 0 )
                        cJSON_AddItemToObject(json,name,cJSON_CreateString(value[i]));
                }
            }
        }
        CGI_free_varlist(varlist);
    }
    process_json(json);
    return 0;
}

