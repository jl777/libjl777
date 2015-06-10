//
//  api_main.c
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//
#include <stdint.h>
#include "nn.h"
#include "cJSON.h"
#include "pipeline.h"
uint32_t _crc32(uint32_t crc,const void *buf,size_t size);
long _stripwhite(char *buf,int accept);

int main(int argc, char **argv)
{
    CGI_varlist *varlist; const char *name; CGI_value  *value; int32_t pushsock,pullsock,i,len,checklen; uint32_t tag; cJSON *json;
    char endpoint[128],*resultstr,*jsonstr,*apiendpoint = "ipc://SuperNET.api";
    fputs("Content-type: text/plain\r\n\r\n", stdout);
    if ( (varlist= CGI_get_all(0)) == 0 )
        printf("No CGI data received\r\n");
        return 0;
    else
    {
        // output all values of all variables and cookies
        for (name=CGI_first_name(varlist); name!=0; name=CGI_next_name(varlist))
        {
            value = CGI_lookup_all(varlist,0);
            ///CGI_lookup_all(varlist, name) could also be used
            for (i=0; value[i]!=0; i++)
                printf("%s [%d] = %s\r\n",name,i,value[i]);
            if ( i == 0 )
            {
                if ( (json= cJSON_Parse(name)) != 0 )
                {
                    len = (int32_t)strlen(name)+1;
                    tag = _crc32(0,name,len);
                    sprintf(endpoint,"ipc://api.%u",tag);
                    cJSON_AddItemToObject(json,"apitag",cJSON_CreateString(endpoint));
                    jsonstr = cJSON_Print(json), _stripwhite(jsonstr,' ');
                    len = (int32_t)strlen(jsonstr)+1;
                    if ( (pushsock= nn_socket(AF_SP,NN_PUSH)) >= 0 )
                    {
                        if ( nn_connect(pushsock,apiendpoint) < 0 )
                            fprintf(stderr,"error connecting to apiendpoint sock.%d type.%d (%s) %s\n",pushsock,NN_PUSH,apiendpoint,nn_errstr());
                        else if ( (checklen= nn_send(pushsock,jsonstr,len,0)) != len )
                            fprintf(stderr,"checklen.%d != len.%d for nn_send to (%s)\n",checklen,len,apiendpoint);
                        else
                        {
                            if ( (pullsock= nn_socket(AF_SP,NN_PULL)) >= 0 )
                            {
                                if ( nn_bind(pullsock,endpoint) < 0 )
                                    fprintf(stderr,"error binding to sock.%d type.%d (%s) %s\n",pullsock,NN_PULL,endpoint,nn_errstr());
                                else
                                {
                                    if ( nn_recv(pullsock,&resultstr,NN_MSG,0) > 0 )
                                    {
                                        printf("%s\n",resultstr);
                                        nn_freemsg(resultstr);
                                    }
                                }
                                nn_shutdown(pullsock,0);
                            }
                            nn_shutdown(pushsock,0);
                        }
                    free_json(json), free(jsonstr);
                } else printf("JSON parse error.(%s)\n",name);
            }
        }
        CGI_free_varlist(varlist);
    }
    return(0);
}


