/******************************************************************************
 * Copyright © 2014-2015 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * Nxt software, including this file, may be copied, modified, propagated,    *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

#include <stdint.h>
#include "../ccgi/ccgi.h"
#include "../includes/cJSON.h"
#include "../../nanomsg/src/nn.h"
#include "../../nanomsg/src/pair.h"
#include "../../nanomsg/src/pipeline.h"
#include "nonportable.h"
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
void randombytes(unsigned char *x,long xlen);

void process_json(cJSON *json,char *remoteaddr,int32_t localaccess)
{
    int32_t sock,len,checklen,sendtimeout,recvtimeout; uint32_t apitag; uint64_t tag;
    char endpoint[128],numstr[64],*resultstr,*jsonstr;
    jsonstr = cJSON_Print(json), _stripwhite(jsonstr,' ');
    len = (int32_t)strlen(jsonstr)+1;
    apitag = _crc32(0,jsonstr,len);
    sprintf(endpoint,"ipc://api.%u",apitag);
    free(jsonstr);
    if ( (recvtimeout= juint(json,"timeout")) == 0 )
        recvtimeout = 30000;
    sendtimeout = 30000;
    randombytes((uint8_t *)&tag,sizeof(tag));
    if ( cJSON_GetObjectItem(json,"tag") == 0 )
        sprintf(numstr,"%llu",(long long)tag), cJSON_AddItemToObject(json,"tag",cJSON_CreateString(numstr));
    if ( cJSON_GetObjectItem(json,"apitag") == 0 )
        cJSON_AddItemToObject(json,"apitag",cJSON_CreateString(endpoint));
    if ( remoteaddr != 0 )
        cJSON_AddItemToObject(json,"broadcast",cJSON_CreateString("remoteaccess"));
    if ( localaccess != 0 )
        cJSON_AddItemToObject(json,"localaccess",cJSON_CreateNumber(1));
    jsonstr = cJSON_Print(json), _stripwhite(jsonstr,' ');
    //fprintf(stderr,"localacess.%d remote.(%s) jsonstr.(%s)\r\n",localaccess,remoteaddr!=0?remoteaddr:"",jsonstr);
    len = (int32_t)strlen(jsonstr)+1;
    if ( jsonstr != 0 )
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

int32_t setnxturl(struct destbuf *urlbuf)
{
    FILE *fp; cJSON *json; char confname[512],buf[65536];
    strcpy(confname,"../../SuperNET.conf"), os_compatible_path(confname);
    urlbuf->buf[0] = 0;
    if ( (fp= fopen(confname,"rb")) != 0 )
    {
        if ( fread(buf,1,sizeof(buf),fp) > 0 )
        {
            if ( (json= cJSON_Parse(buf)) != 0 )
            {
                copy_cJSON(urlbuf,cJSON_GetObjectItem(json,"NXTAPIURL"));
fprintf(stderr,"set NXTAPIURL.(%s)\n",urlbuf->buf);
                free_json(json);
            } else fprintf(stderr,"setnxturl parse error.(%s)\n",buf);
        } else fprintf(stderr,"setnxturl error reading.(%s)\n",confname);
        fclose(fp);
    } else fprintf(stderr,"setnxturl cant open.(%s)\n",confname);
    return((int32_t)strlen(urlbuf->buf));
}

int main(int argc, char **argv)
{
    void portable_OS_init();
    CGI_varlist *varlist; const char *name; char namebuf[512],postbuf[65536],*remoteaddr,*str=0,*retstr,*delim,*url = 0;
    int i,j,iter,localaccess=0,doneflag=0,portflag = 0; cJSON *json; long offset; CGI_value  *value; struct destbuf urlbuf;
    portable_OS_init();
    setenv("CONTENT_TYPE", "application/x-www-form-urlencoded", 1);
    json = cJSON_CreateObject();
    if ( (remoteaddr= getenv("REMOTE_ADDR")) == 0 || strncmp("127.0.0.1",remoteaddr,strlen("127.0.0.1")) == 0 )
        remoteaddr = 0,localaccess = 1;
    else cJSON_AddItemToObject(json,"remoteaddr",cJSON_CreateString(remoteaddr));
    for (i=j=0; argv[0][i]!=0; i++)
        if ( argv[0][i] == '/' || argv[0][i] == '\\' )
            j = i+1;
    strcpy(namebuf,&argv[0][j]);
    offset = strlen(namebuf) - 4;
    if ( offset > 0 && strcmp(".exe",namebuf + offset) == 0 )
        namebuf[offset] = 0;
    if ( offset > 0 && strcmp(".cgi",namebuf + offset) == 0 )
        namebuf[offset] = 0;
    if ( strcmp(namebuf,"init") == 0 || strcmp(namebuf,"") == 0 || strcmp(namebuf,"index.cgi") == 0 )
    {
        // "http://178.63.60.131/init/?requestType=status&coin=VRC"
        //"http://78.47.115.250:7777/public?plugin=relay&method=busdata&servicename=MGW&serviceNXT=8119557380101451968&destplugin=MGW&submethod=status&coin=BTC"
        if ( strcmp(namebuf,"api") != 0 )
            cJSON_AddItemToObject(json,"agent",cJSON_CreateString(namebuf));
        cJSON_AddItemToObject(json,"plugin",cJSON_CreateString("relay"));
        cJSON_AddItemToObject(json,"method",cJSON_CreateString("busdata"));
        cJSON_AddItemToObject(json,"servicename",cJSON_CreateString("MGW"));
        cJSON_AddItemToObject(json,"serviceNXT",cJSON_CreateString("8119557380101451968"));
        cJSON_AddItemToObject(json,"destplugin",cJSON_CreateString("MGW"));
        if ( jstr(json,"requestType") != 0 )
            cJSON_AddItemToObject(json,"submethod",cJSON_CreateString(jstr(json,"requestType")));
    }
    if ( strcmp("nxt",namebuf) == 0 )
    {
        if ( setnxturl(&urlbuf) != 0 )
            url = urlbuf.buf;
        else url = "http://127.0.0.1:7876/nxt";
    }
    else if ( strcmp("nxts",namebuf) == 0 )
        url = "https://127.0.0.1:7876/nxt";
    else if ( strcmp("port",namebuf) == 0 )
        url = "http://127.0.0.1", portflag = 1;
    else if ( strcmp("ports",namebuf) == 0 )
        url = "https://127.0.0.1", portflag = 1;
    fprintf(stderr,"namebuf.(%s)\n",namebuf);
    if ( url != 0 )
         postbuf[0] = 0, delim = "";
    for (iter=0; iter<3; iter++)
    {
        if ( (varlist= ((iter==0) ? CGI_get_post(0,0) : ((iter==1) ? CGI_get_query(0) : CGI_get_cookie(0)))) != 0 )
        {
            for (name=CGI_first_name(varlist); name!=0&&doneflag==0; name=CGI_next_name(varlist))
            {
                value = CGI_lookup_all(varlist,0);
                for (i=0; value[i]!=0; i++)
                {
                fprintf(stderr,"iter.%d %s [%d] = %s\r\n",iter,name,i,value[i]);
                    if ( i == 0 )
                    {
                        if ( url == 0 )
                        {
                            if ( strcmp(name,"stringified") == 0 || strcmp(namebuf,"stringified") == 0 )
                            {
                                char *unstringify(char *str);
                                cJSON *obj;
                                if ( (obj= cJSON_Parse(name)) == 0 )
                                {
                                    str = malloc(strlen(value[i])+1);
                                    strcpy(str,value[i]);
                                    unstringify(str);
                                    printf("unstringify (%s) -> (%s)\n",value[i],str);
                                    obj= cJSON_Parse(str);
                                }
                                if ( obj != 0 )
                                {
                                    //unstringified ((null)) -> ({"stringified":{"method":"orderbook","baseid":"12071612744977229797","relid":"5527630","maxdepth":"1"},"agent":"InstantDEX"})
                                    free_json(json);
                                    if ( jobj(obj,"stringified") != 0 )
                                        json = cJSON_Duplicate(jobj(obj,"stringified"),1), free_json(obj);
                                    else json = obj;
                                    cJSON_AddItemToObject(json,"agent",cJSON_CreateString("InstantDEX"));
                                    if ( remoteaddr != 0 && remoteaddr[0] != 0 )
                                        cJSON_AddItemToObject(json,"remoteaddr",cJSON_CreateString(remoteaddr));
                                    fprintf(stderr,"unstringified (%s) -> (%s)\n",str!=0?str:"",jprint(json,0));
                                    if ( str != 0 )
                                        free(str);
                                    doneflag = 1;
                                    break;
                                }
                            }
                            cJSON_AddItemToObject(json,name,cJSON_CreateString(value[i]));
                        }
                        else
                        {
                            if ( portflag != 0 && strncmp(name,"port",strlen("port")) == 0 )
                                sprintf(urlbuf.buf,"%s:%s",url,value[i]), url = urlbuf.buf, portflag = 0;
                            else sprintf(postbuf + strlen(postbuf),"%s%s=%s",delim,name,value[i]), delim = "&";
                        }
                    }
                }
            }
        }
        CGI_free_varlist(varlist);
    }
    if ( localaccess == 0 )
        fputs("Access-Control-Allow-Origin: *\r\n",stdout);
    else fputs("Access-Control-Allow-Origin: null\r\n",stdout);
    fputs("Access-Control-Allow-Credentials: true\r\n",stdout);
    fputs("Access-Control-Allow-Headers: Authorization, Content-Type\r\n",stdout);
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
        if ( jobj(json,"agent") == 0 && strcmp(namebuf,"api") != 0 )
            cJSON_AddItemToObject(json,"agent",cJSON_CreateString(namebuf));
        fprintf(stderr,"PROCESS.(%s)\n",jprint(json,0));
        process_json(json,remoteaddr,localaccess);
    }
    free_json(json);
    return 0;
}

