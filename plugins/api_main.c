//
//  api_main.c
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//
#include <stdint.h>
#include "ccgi.h"
#include "nn.h"
#include "includes/cJSON.h"
#include "pair.h"
#include "pipeline.h"
#ifdef _WIN32
#define setenv(x, y, z) _putenv_s(x, y)
#endif
uint32_t _crc32(uint32_t crc,const void *buf,size_t size);
long _stripwhite(char *buf,int accept);
#define nn_errstr() nn_strerror(nn_errno())
#define SUPERNET_APIENDPOINT "tcp://127.0.0.1:7776"
char *bitcoind_RPC(char **retstrp,char *debugstr,char *url,char *userpass,char *command,char *params);
#define issue_POST(url,cmdstr) bitcoind_RPC(0,"curl",url,0,0,cmdstr)
char *os_compatible_path(char *str);

void process_json(cJSON *json,int32_t publicaccess)
{
    int32_t sock,i,len,checklen,sendtimeout,recvtimeout; uint32_t apitag; uint64_t tag;
    char endpoint[128],numstr[64],*resultstr,*jsonstr;
    jsonstr = cJSON_Print(json), _stripwhite(jsonstr,' ');
    len = (int32_t)strlen(jsonstr)+1;
    apitag = _crc32(0,jsonstr,len);
    sprintf(endpoint,"ipc://api.%u",apitag);
    free(jsonstr);
    recvtimeout = get_API_int(cJSON_GetObjectItem(json,"timeout"),30000);
    sendtimeout = 30000;
    randombytes(&tag,sizeof(tag));
    if ( cJSON_GetObjectItem(json,"tag") == 0 )
        sprintf(numstr,"%llu",(long long)tag), cJSON_AddItemToObject(json,"tag",cJSON_CreateString(numstr));
    if ( cJSON_GetObjectItem(json,"apitag") == 0 )
        cJSON_AddItemToObject(json,"apitag",cJSON_CreateString(endpoint));
    if ( publicaccess != 0 )
        cJSON_AddItemToObject(json,"broadcast",cJSON_CreateString("publicaccess"));
    jsonstr = cJSON_Print(json), _stripwhite(jsonstr,' ');
    //fprintf(stderr,"publicaccess.%d jsonstr.(%s)\r\n",publicaccess,jsonstr);
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

int32_t setnxturl(char *urlbuf)
{
    FILE *fp; cJSON *json; char confname[512],buf[65536];
    strcpy(confname,"../../SuperNET.conf"), os_compatible_path(confname);
    urlbuf[0] = 0;
    if ( (fp= fopen(confname,"rb")) != 0 )
    {
        if ( fread(buf,1,sizeof(buf),fp) > 0 )
        {
            if ( (json= cJSON_Parse(buf)) != 0 )
            {
                copy_cJSON(urlbuf,cJSON_GetObjectItem(json,"NXTAPIURL"));
fprintf(stderr,"set NXTAPIURL.(%s)\n",urlbuf);
                free_json(json);
            } else fprintf(stderr,"setnxturl parse error.(%s)\n",buf);
        } else fprintf(stderr,"setnxturl error reading.(%s)\n",confname);
        fclose(fp);
    } else fprintf(stderr,"setnxturl cant open.(%s)\n",confname);
    return((int32_t)strlen(urlbuf));
}

int main(int argc, char **argv)
{
    void portable_OS_init();
    CGI_varlist *varlist; const char *name; CGI_value  *value;  int i,j,iter,publicaccess = 0,portflag = 0; cJSON *json; long offset;
    char urlbuf[512],namebuf[512],postbuf[65536],*retstr,*delim,*url = 0;
    portable_OS_init();
    setenv("CONTENT_TYPE", "application/x-www-form-urlencoded", 1);
    json = cJSON_CreateObject();
    for (i=j=0; argv[0][i]!=0; i++)
        if ( argv[0][i] == '/' || argv[0][i] == '\\' )
            j = i+1;
    strcpy(namebuf,&argv[0][j]);
    offset = strlen(namebuf) - 4;
    if ( offset > 0 && strcmp(".exe",namebuf + offset) == 0 )
        namebuf[offset] = 0;
    if ( strcmp(namebuf,"public") == 0 )
        publicaccess = 1, strcpy(namebuf,"api");
    if ( strcmp(namebuf,"api") != 0 )
        cJSON_AddItemToObject(json,"agent",cJSON_CreateString(namebuf));
    if ( strcmp("nxt",namebuf) == 0 )
    {
fprintf(stderr,"namebuf.(%s)\n",namebuf);
        if ( setnxturl(urlbuf) != 0 )
            url = urlbuf;
        else url = "http://127.0.0.1:7876/nxt";
    }
    else if ( strcmp("nxts",namebuf) == 0 )
        url = "https://127.0.0.1:7876/nxt";
    else if ( strcmp("port",namebuf) == 0 )
        url = "http://127.0.0.1", portflag = 1;
    else if ( strcmp("ports",namebuf) == 0 )
        url = "https://127.0.0.1", portflag = 1;
    if ( url != 0 )
         postbuf[0] = 0, delim = "";
    //if ( (value= CGI_lookup_all(CGI_get_all(0),"HTTP_ORIGIN")) != 0 )
    //    printf("HTTP_ORIGIN: %s\n",value);
    for (iter=0; iter<3; iter++)
    {
        if ( (varlist= ((iter==0) ? CGI_get_post(0,0) : ((iter==1) ? CGI_get_query(0) : CGI_get_cookie(0)))) != 0 )
        {
            for (name=CGI_first_name(varlist); name!=0; name=CGI_next_name(varlist))
            {
                value = CGI_lookup_all(varlist,0);
                for (i=0; value[i]!=0; i++)
                {
                    //fprintf(stderr,"iter.%d %s [%d] = %s\r\n",iter,name,i,value[i]);
                    if ( i == 0 )
                    {
                        if ( url == 0 )
                            cJSON_AddItemToObject(json,name,cJSON_CreateString(value[i]));
                        else
                        {
                            if ( portflag != 0 && strncmp(name,"port",strlen("port")) == 0 )
                                sprintf(urlbuf,"%s:%s",url,value[i]), url = urlbuf, portflag = 0;
                            else sprintf(postbuf + strlen(postbuf),"%s%s=%s",delim,name,value[i]), delim = "&";
                        }
                    }
                }
            }
        }
        CGI_free_varlist(varlist);
    }
    fputs("Access-Control-Allow-Origin: null\r\n",stdout);
    fputs("Access-Control-Allow-Headers: Authorization, Content-Type\r\n",stdout);
    fputs("Access-Control-Allow-Credentials: true\r\n",stdout);
    fputs("Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n",stdout);
    fputs("Cache-Control: no-cache, no-store, must-revalidate\r\n",stdout);
    fputs("Content-type: text/plain\r\n",stdout);
    if ( url != 0 )
    {
fprintf(stderr,"url.(%s) (%s)\n",url,postbuf);
        if ( (retstr= issue_POST(url,postbuf)) != 0 )
        {
            //fprintf(stderr,"%s",retstr);
            printf("Content-Length: %ld\r\n\r\n",strlen(retstr)+2);
            printf("%s\r\n",retstr);
            free(retstr);
        } else printf("{\"error\":\"null return from issue_NXTPOST\"}\r\n");
    }
    else
    {
        process_json(json,publicaccess);
    }
    free_json(json);
    return 0;
}

