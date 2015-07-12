//
//  main.c
//  libtest
//
//  Created by jl777 on 8/13/14.
//  Copyright (c) 2014 jl777. MIT License.
//
#ifdef oldway
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <memory.h>
#ifndef _WIN32
#include <arpa/inet.h>
#endif
#include <sys/time.h>
#include "includes/uthash.h"

//Miniupnp code for supernet by chanc3r
#include <time.h>
#ifdef _WIN32
#include <winsock2.h>
#define snprintf _snprintf
#else
// for IPPROTO_TCP / IPPROTO_UDP
#include <netinet/in.h>
#endif
#include "miniupnpc/miniwget.h"
#include "miniupnpc/miniupnpc.h"
#include "miniupnpc/upnpcommands.h"
#include "miniupnpc/upnperrors.h"
int32_t recvsock;
#define DEFINES_ONLY
#include "plugins/utils/system777.c"
#include "plugins/utils/utils777.c"
#undef DEFINES_ONLY

#include "cJSON.h"
#define NUM_GATEWAYS 3

cJSON *SuperAPI(char *cmd,char *field0,char *arg0,char *field1,char *arg1)
{
    cJSON *json = 0;
    char params[1024],*retstr;
    if ( field0 != 0 && field0[0] != 0 )
    {
        if ( field1 != 0 && field1[0] != 0 )
            sprintf(params,"{\"requestType\":\"%s\",\"%s\":\"%s\",\"%s\":\"%s\"}",cmd,field0,arg0,field1,arg1);
        else sprintf(params,"{\"requestType\":\"%s\",\"%s\":\"%s\"}",cmd,field0,arg0);
    }
    else sprintf(params,"{\"requestType\":\"%s\"}",cmd);
    retstr = bitcoind_RPC(0,(char *)"BTCD",SuperNET_url(),(char *)"",(char *)"SuperNET",params);
    if ( retstr != 0 )
    {
        json = cJSON_Parse(retstr);
        free(retstr);
    }
    return(json);
}

char *GUIpoll(char *txidstr,char *senderipaddr,uint16_t *portp)
{
    char params[4096],buf[MAX_JSON_FIELD],buf2[MAX_JSON_FIELD],ipaddr[MAX_JSON_FIELD],args[8192],*retstr;
    int32_t port;
    cJSON *json,*argjson;
    txidstr[0] = 0;
    sprintf(params,"{\"requestType\":\"GUIpoll\"}");
    retstr = bitcoind_RPC(0,(char *)"BTCD",SuperNET_url(),(char *)"",(char *)"SuperNET",params);
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
                {
                    if ( Debuglevel > 0 )
                        fprintf(stderr,"<<<<<<<<<<< GUIpoll: (%s) for [%s]\n",buf,txidstr);
                }
                else
                {
                    copy_cJSON(ipaddr,cJSON_GetObjectItem(json,"from"));
                    copy_cJSON(args,cJSON_GetObjectItem(json,"args"));
                    port = (int32_t)get_API_int(cJSON_GetObjectItem(json,"port"),0);
                    if ( args[0] != 0 )
                    {
                        unstringify(args);
                        if ( Debuglevel > 2 )
                            printf("(%s) from (%s:%d) -> (%s) Qtxid.(%s)\n",args,ipaddr,port,buf,txidstr);
                        free(retstr);
                        retstr = clonestr(args);
                        if ( (argjson= cJSON_Parse(retstr)) != 0 )
                        {
                            copy_cJSON(buf2,cJSON_GetObjectItem(argjson,"result"));
                            if ( strcmp(buf2,"nothing pending") == 0 )
                                free(retstr), retstr = 0;
                            //else printf("RESULT.(%s)\n",buf2);
                            free_json(argjson);
                        }
                    }
                }
            }
            free_json(json);
        } else fprintf(stderr,"<<<<<<<<<<< GUI poll_for_broadcasts: PARSE_ERROR.(%s)\n",retstr);
        // free(retstr);
    } //else fprintf(stderr,"<<<<<<<<<<< BTCD poll_for_broadcasts: bitcoind_RPC returns null\n");
    return(retstr);
}

char *process_commandline_json(cJSON *json)
{
#ifdef later
    char *inject_pushtx(char *coinstr,cJSON *json);
    bits256 issue_getpubkey(int32_t *haspubkeyp,char *acct);
    char *issue_ramstatus(char *coinstr);
    struct multisig_addr *decode_msigjson(char *NXTaddr,cJSON *obj,char *sender);
    int32_t send_email(char *email,char *destNXTaddr,char *pubkeystr,char *msg);
    void issue_genmultisig(char *coinstr,char *userNXTaddr,char *userpubkey,char *email,int32_t buyNXT);
    char txidstr[1024],senderipaddr[1024],arg[1024],cmd[2048],coin[2048],userpubkey[2048],NXTacct[2048],userNXTaddr[2048],email[2048],convertNXT[2048],retbuf[1024],buf2[1024],coinstr[1024],cmdstr[512],*retstr = 0,*waitfor = 0,errstr[2048],*str;
    bits256 pubkeybits;
    unsigned char hash[256>>3],mypublic[256>>3];
    uint16_t port;
    uint64_t nxt64bits,checkbits;//,deposit_pending = 0;
    int32_t i,n,haspubkey,iter,gatewayid;//,actionflag = 0,rescan = 1;
    uint32_t buyNXT = 0;
    cJSON *array,*argjson,*retjson,*retjsons[3];
    copy_cJSON(cmdstr,cJSON_GetObjectItem(json,"webcmd"));
    if ( strcmp(cmdstr,"SuperNET") == 0 )
    {
        str = cJSON_Print(json);
        //printf("GOT webcmd.(%s)\n",str);
        retstr = bitcoind_RPC(0,"webcmd",SuperNET_url(),(char *)"",(char *)"SuperNET",str);
        free(str);
        return(retstr);
    }
    copy_cJSON(coin,cJSON_GetObjectItem(json,"coin"));
    copy_cJSON(cmd,cJSON_GetObjectItem(json,"requestType"));
    if ( strcmp(cmd,"status") == 0 )
    {
        struct MGWstate S[3],*sp;
        char fname[1024],*buf;
        long fpos,jsonflag;
        memset(S,0,sizeof(S));
        FILE *fp;
        copy_cJSON(arg,cJSON_GetObjectItem(json,"jsonflag"));
        jsonflag = !strcmp(arg,"1");
        if ( jsonflag != 0 )
            printf("[");
        for (i=0; i<3; i++)
        {
            sp = &S[i];
            //sprintf(fname,"%s/MGW/status/%s.%s",MGWROOT,coin,Server_ipaddrs[i]);
            sprintf(fname,"%s/MGW/status/%s.%d",SUPERNET.MGWROOT,coin,i);
            if ( (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
            {
                fseek(fp,0,SEEK_END);
                fpos = ftell(fp);
                rewind(fp);
                buf = calloc(1,fpos+1);
                if ( fread(buf,1,fpos,fp) == fpos && (json= cJSON_Parse(buf)) != 0 )
                {
                    if ( jsonflag != 0 )
                    {
                         if ( i != 0 )
                            printf(", ");
                        printf("%s",buf);
                    }
                    else
                    {
                        ram_parse_MGWstate(sp,json,coin,Server_NXTaddrs[i]);
                        printf("G%d:[+%.8f %s - %.0f NXT rate %.2f] unspent %.8f circ %.8f/%.8f pend.(R%.8f D%.8f) NXT.%d %s.%d<BR>",i,dstr(sp->MGWbalance),coin,dstr(sp->sentNXT),sp->MGWbalance<=0?0:dstr(sp->sentNXT)/dstr(sp->MGWbalance),dstr(sp->MGWunspent),dstr(sp->circulation),dstr(sp->supply),dstr(sp->MGWpendingredeems),dstr(sp->MGWpendingdeposits),sp->NXT_RTblocknum,coin,sp->RTblocknum);
                    }
                    free_json(json);
                }
                free(buf);
                fclose(fp);
            }
        }
        if ( jsonflag != 0 )
            printf("]");
        return(0);
       //return(issue_ramstatus(coin));
    }
    else
    {
        if ( strcmp(cmd,"pushtx") == 0 )
            return(inject_pushtx(coin,json));
        copy_cJSON(email,cJSON_GetObjectItem(json,"email"));
        copy_cJSON(NXTacct,cJSON_GetObjectItem(json,"NXT"));
        copy_cJSON(userpubkey,cJSON_GetObjectItem(json,"pubkey"));
        if ( userpubkey[0] == 0 )
        {
            pubkeybits = issue_getpubkey(&haspubkey,NXTacct);
            if ( haspubkey != 0 )
                init_hexbytes_noT(userpubkey,pubkeybits.bytes,sizeof(pubkeybits.bytes));
        }
        copy_cJSON(convertNXT,cJSON_GetObjectItem(json,"convertNXT"));
        if ( convertNXT[0] != 0 )
            buyNXT = (uint32_t)atol(convertNXT);
        nxt64bits = conv_acctstr(NXTacct);
        expand_nxt64bits(userNXTaddr,nxt64bits);
        decode_hex(mypublic,sizeof(mypublic),userpubkey);
        calc_sha256(0,hash,mypublic,32);
        memcpy(&checkbits,hash,sizeof(checkbits));
        if ( checkbits != nxt64bits )
        {
            sprintf(retbuf,"{\"error\":\"invalid pubkey\",\"pubkey\":\"%s\",\"NXT\":\"%s\",\"checkNXT\":\"%llu\"}",userpubkey,userNXTaddr,(long long)checkbits);
            return(clonestr(retbuf));
        }
        memset(retjsons,0,sizeof(retjsons));
        cmdstr[0] = 0;
        //printf("got cmd.(%s)\n",cmd);
        if ( strcmp(cmd,"newbie") == 0 )
        {
            waitfor = "MGWaddr";
            strcpy(cmdstr,cmd);
            for (i=0; i<100; i++) // flush queue
                GUIpoll(txidstr,senderipaddr,&port);
            if ( coin[0] == 0 )
            {
                array = cJSON_GetObjectItem(MGWconf,"active");
                if ( array != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
                {
                    for (iter=0; iter<3; iter++) // give chance for servers to consensus
                    {
                        for (i=0; i<n; i++)
                        {
                            copy_cJSON(coinstr,cJSON_GetArrayItem(array,i));
                            if ( coinstr[0] != 0 )
                            {
                                issue_genmultisig(coinstr,userNXTaddr,userpubkey,email,buyNXT);
                                portable_sleep(1);
                            }
                        }
                        portable_sleep(3);
                    }
                }
            }
            else
            {
                for (iter=0; iter<3; iter++) // give chance for servers to consensus
                {
                    issue_genmultisig(coin,userNXTaddr,userpubkey,email,buyNXT);
                    portable_sleep(3);
                }
            }
        }
        else return(clonestr("{\"error\":\"only newbie command is supported now\"}"));
    }
    if ( waitfor != 0 )
    {
        for (i=0; i<3000; i++)
        {
            if ( (retstr= GUIpoll(txidstr,senderipaddr,&port)) != 0 )
            {
                if ( retstr[0] == '[' || retstr[0] == '{' )
                {
                    if ( (retjson= cJSON_Parse(retstr)) != 0 )
                    {
                        if ( is_cJSON_Array(retjson) != 0 && (n= cJSON_GetArraySize(retjson)) == 2 )
                        {
                            argjson = cJSON_GetArrayItem(retjson,0);
                            copy_cJSON(buf2,cJSON_GetObjectItem(argjson,"requestType"));
                            gatewayid = (int32_t)get_API_int(cJSON_GetObjectItem(argjson,"gatewayid"),-1);
                            if ( gatewayid >= 0 && gatewayid < 3 && retjsons[gatewayid] == 0 )
                            {
                                copy_cJSON(errstr,cJSON_GetObjectItem(argjson,"error"));
                                if ( strlen(errstr) > 0 || strcmp(buf2,waitfor) == 0 )
                                {
                                    retjsons[gatewayid] = retjson, retjson = 0;
                                    if ( retjsons[0] != 0 && retjsons[1] != 0 && retjsons[2] != 0 )
                                        break;
                                }
                            }
                        }
                        if ( retjson != 0 )
                            free_json(retjson);
                    }
                }
                //fprintf(stderr,"(%p) %s\n",retjson,retstr);
                free(retstr),retstr = 0;
            } else msleep(3);
        }
    }
    for (i=0; i<3; i++)
        if ( retjsons[i] == 0 )
            break;
    if ( i < 3 && cmdstr[0] != 0 )
    {
        for (i=0; i<3; i++)
        {
            if ( retjsons[i] == 0 && (retstr= issue_curl(0,cmdstr)) != 0 )
            {
                /*printf("(%s) -> (%s)\n",cmdstr,retstr);
                 if ( (msigjson= cJSON_Parse(retstr)) != 0 )
                 {
                 if ( is_cJSON_Array(msigjson) != 0 && (n= cJSON_GetArraySize(msigjson)) > 0 )
                 {
                 for (j=0; j<n; j++)
                 {
                 item = cJSON_GetArrayItem(msigjson,j);
                 copy_cJSON(coinstr,cJSON_GetObjectItem(item,"coin"));
                 if ( strcmp(coinstr,xxx) == 0 )
                 {
                 msig = decode_msigjson(0,item,Server_NXTaddrs[i]);
                 break;
                 }
                 }
                 }
                 else msig = decode_msigjson(0,msigjson,Server_NXTaddrs[i]);
                 if ( msig != 0 )
                 {
                 free(msig);
                 free_json(msigjson);
                 if ( email[0] != 0 )
                 send_email(email,userNXTaddr,0,retstr);
                 //printf("[%s]\n",retstr);
                 return(retstr);
                 }
                 } else printf("error parsing.(%s)\n",retstr);
                 free_json(msigjson);
                 free(retstr);*/
                if ( retstr[0] == '{' || retstr[0] == '[' )
                {
                    //if ( email[0] != 0 )
                    //    send_email(email,userNXTaddr,0,retstr);
                    //return(retstr);
                    retjsons[i] = cJSON_Parse(retstr);
                }
                free(retstr);
            } //else printf("cant find (%s)\n",cmdstr);
        }
    }
    json = cJSON_CreateArray();
    for (i=0; i<3; i++)
    {
        char *load_filestr(char *userNXTaddr,int32_t gatewayid);
        char *filestr;
        if ( retjsons[i] == 0 && userNXTaddr[0] != 0 && (filestr= load_filestr(userNXTaddr,i)) != 0 )
        {
            retjsons[i] = cJSON_Parse(filestr);
            printf(">>>>>>>>>>>>>>> load_filestr!!! %s.%d json.%p\n",userNXTaddr,i,json);
        }
        if ( retjsons[i] != 0 )
            cJSON_AddItemToArray(json,retjsons[i]);
    }
    /*if ( deposit_pending != 0 )
    {
        actionflag = 1;
        rescan = 0;
        retstr = issue_MGWstatus(1<<NUM_GATEWAYS,coin,0,0,0,rescan,actionflag);
        if ( retstr != 0 )
            free(retstr), retstr = 0;
    }*/
    retstr = cJSON_Print(json);
    free_json(json);
    if ( email[0] != 0 )
        send_email(email,userNXTaddr,0,retstr);
    for (i=0; i<1000; i++)
    {
        if ( (str= GUIpoll(txidstr,senderipaddr,&port)) != 0 )
            free(str);
        else break;
    }
    return(retstr);
#endif
    return(clonestr("later"));
}

char *load_filestr(char *userNXTaddr,int32_t gatewayid)
{
    long fpos;
    FILE *fp;
    char fname[1024],*buf=0,*retstr = 0;
    sprintf(fname,"%s/gateway%d/%s",SUPERNET.MGWROOT,gatewayid,userNXTaddr);
    if ( (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
    {
        fseek(fp,0,SEEK_END);
        fpos = ftell(fp);
        if ( fpos > 0 )
        {
            rewind(fp);
            buf = calloc(1,fpos);
            if ( fread(buf,1,fpos,fp) == fpos )
                retstr = buf, buf = 0;
        }
        fclose(fp);
        if ( buf != 0 )
            free(buf);
    }
    return(retstr);
}

void bridge_handler(struct transfer_args *args)
{
    FILE *fp;
    int32_t gatewayid = -1;
    char fname[1024],cmd[1024],*name = args->name;
    if ( strncmp(name,"MGW",3) == 0 && name[3] >= '0' && name[3] <= '2' )
    {
        gatewayid = (name[3] - '0');
        name += 5;
        sprintf(fname,"%s/gateway%d/%s",SUPERNET.MGWROOT,gatewayid,name);
        if ( (fp= fopen(os_compatible_path(fname),"wb+")) != 0 )
        {
            fwrite(args->data,1,args->totallen,fp);
            fclose(fp);
            sprintf(cmd,"chmod +r %s",fname);
            system(os_compatible_path(cmd));
        }
    }
    printf("bridge_handler.gateway%d/(%s).%d\n",gatewayid,name,args->totallen);
}

#include "nn.h"
#include "bus.h"

void *GUIpoll_loop(void *arg)
{
    cJSON *json;
    uint16_t port;
    int32_t n,sleeptime = 0;
    char txidstr[MAX_JSON_FIELD],buf[MAX_JSON_FIELD],senderipaddr[MAX_JSON_FIELD],*retstr,*msg;
    while ( 1 )
    {
        sleeptime++;
        if ( (retstr= GUIpoll(txidstr,senderipaddr,&port)) != 0 )
        {
            if ( (json= cJSON_Parse(retstr)) != 0 )
            {
                copy_cJSON(buf,cJSON_GetObjectItem(json,"result"));
                if ( strcmp(buf,"nothing pending") != 0 )
                {
                    sleeptime = 0;
                    if ( strcmp(buf,"MGWstatus") == 0 )
                    {
                        printf("sleeptime.%d (%s) (%s)\n",sleeptime,buf,retstr);
                    }
                }
                free_json(json);
            }
            free(retstr);
        }
        while ( 0 && (n= nn_recv(recvsock,&msg,NN_MSG,0)) > 0 )
        {
            printf("got inproc msg.(%s)\n",msg);
            nn_freemsg(msg);
        }// else printf("no messages\n");
        if ( sleeptime != 0 )
            portable_sleep(sleeptime);
    }
    return(0);
}

#define NXTSERVER "https://127.0.0.1:7876/nxt?requestType"
uint64_t get_NXT_forginginfo(char *gensig,uint32_t height)
{
    cJSON *json;
    uint64_t basetarget = 0;
    char cmd[4096],*jsonstr;
    sprintf(cmd,"%s=getBlock&height=%u",NXTSERVER,height);
    if ( (jsonstr= issue_curl(0,cmd)) != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            basetarget = get_API_int(cJSON_GetObjectItem(json,"baseTarget"),0);
            copy_cJSON(gensig,cJSON_GetObjectItem(json,"generationSignature"));
            free_json(json);
        }
        free(jsonstr);
    }
    return(basetarget);
}

#define calc_NXThit(NXTpubkey,gensig,len) calc_sha256cat(hithash.bytes,gensig,len,NXTpubkey.bytes,sizeof(NXTpubkey))

bits256 transparent_forging(char *nextgensig,uint32_t height,bits256 *NXTpubkeys,int32_t numaccounts)
{
    int _increasing_uint64(const void *a,const void *b);
    char gensigstr[1024]; uint8_t gensig[512];
    bits256 hithash; uint64_t basetarget,*sortbuf;
    int32_t i,len,winner;
    basetarget = get_NXT_forginginfo(gensigstr,height);
    len = (int32_t)strlen(gensigstr) >> 1;
    decode_hex(gensig,len,gensigstr);
    sortbuf = calloc(numaccounts,2 * sizeof(uint64_t));
    for (i=0; i<numaccounts; i++)
    {
        calc_NXThit(NXTpubkeys[i],gensig,len);
        sortbuf[(i<<1)] = hithash.txid;
        sortbuf[(i<<1) + 1] = i;
    }
    qsort(sortbuf,numaccounts,sizeof(uint64_t)*2,_increasing_uint64);
    winner = (int32_t)sortbuf[1];
    free(sortbuf);
    // seconds to forge from last block: hit / ( basetarget * effective balanceNXT)
    return(NXTpubkeys[winner]);
}

int main(int argc,const char *argv[])
{
    FILE *fp;
    cJSON *json = 0;
    int32_t retval = -666;
    char ipaddr[64],*oldport,*newport,portstr[64],*retstr;
    {
        int32_t err,to = 1;
        char *bindaddr = "inproc://test";
        printf("create nn_socket.(%s)\n",bindaddr);
        if ( (recvsock= nn_socket(AF_SP,NN_BUS)) < 0 )
        {
            printf("error %d nn_socket err.%s\n",recvsock,nn_strerror(nn_errno()));
            return(-1);
        }
        if ( (err= nn_bind(recvsock,bindaddr)) < 0 )
        {
            printf("error %d nn_bind.%d (%s) | %s\n",err,recvsock,bindaddr,nn_strerror(nn_errno()));
            return(-1);
        }
        nn_setsockopt(recvsock,NN_SOL_SOCKET,NN_RCVTIMEO,&to,sizeof(to));
        printf("%s bound\n",bindaddr);
    }
    IS_LIBTEST = 1;
    if ( argc > 1 && argv[1] != 0 )
    {
        char *init_MGWconf(char *JSON_or_fname,char *myipaddr);
        //printf("ARGV1.(%s)\n",argv[1]);
        if ( (json= cJSON_Parse(argv[1])) != 0 )
        {
            Debuglevel = IS_LIBTEST = -1;
            init_MGWconf(argv[2] != 0 ? (char *)argv[2] : "SuperNET.conf",0);
            if ( (retstr= process_commandline_json(json)) != 0 )
            {
                printf("%s\n",retstr);
                free(retstr);
            }
            free_json(json);
            return(0);
        }
        else strcpy(ipaddr,argv[1]);
    }
    else strcpy(ipaddr,"127.0.0.1");
    if ( retval == -666 )
        retval = SuperNET_start("SuperNET.conf",ipaddr);
    sprintf(portstr,"%d",SUPERNET_PORT);
    oldport = newport = portstr;
    if ( UPNP != 0 && upnpredirect(oldport,newport,"UDP","SuperNET_http") == 0 )
        printf("TEST ERROR: failed redirect (%s) to (%s)\n",oldport,newport);
    printf("saving retval.%x (%d usessl.%d) UPNP.%d MULTIPORT.%d\n",retval,retval>>1,retval&1,UPNP,MULTIPORT);
    if ( (fp= fopen("horrible.hack","wb+")) != 0 )
    {
        fwrite(&retval,1,sizeof(retval),fp);
        fclose(fp);
    }
    if ( Debuglevel > 0 )
        system("git log | head -n 1");
    if ( retval >= 0 && ENABLE_GUIPOLL != 0 )
        GUIpoll_loop(ipaddr);
    while ( 1 ) portable_sleep(20);
    return(0);
}
//#include "child.h"

// stubs
int32_t SuperNET_broadcast(char *msg,int32_t duration) { return(0); }
int32_t SuperNET_narrowcast(char *destip,unsigned char *msg,int32_t len) { return(0); }
#endif

#include <stdio.h>
#include <curl/curl.h>
#define DEFINES_ONLY
#include "plugins/includes/cJSON.h"
#include "plugins/utils/utils777.c"
#include "plugins/utils/files777.c"
#include "plugins/utils/system777.c"
#include "plugins/plugins.h"
#undef DEFINES_ONLY

struct pending_cgi { struct queueitem DL; char apitag[24],*jsonstr; cJSON *json; int32_t sock,retind; };

char *SuperNET_install(char *plugin,char *jsonstr,cJSON *json)
{
    char ipaddr[MAX_JSON_FIELD],path[MAX_JSON_FIELD],*str,*retstr;
    int32_t i,ind,async;
    uint16_t port,websocket;
    if ( find_daemoninfo(&ind,plugin,0,0) != 0 )
        return(clonestr("{\"error\":\"plugin already installed\"}"));
    copy_cJSON(path,cJSON_GetObjectItem(json,"path"));
    copy_cJSON(ipaddr,cJSON_GetObjectItem(json,"ipaddr"));
    port = get_API_int(cJSON_GetObjectItem(json,"port"),0);
    async = get_API_int(cJSON_GetObjectItem(json,"daemonize"),0);
    websocket = get_API_int(cJSON_GetObjectItem(json,"websocket"),0);
    str = stringifyM(jsonstr);
    retstr = language_func(plugin,ipaddr,port,websocket,async,path,str,call_system);
    for (i=0; i<3; i++)
    {
        if ( find_daemoninfo(&ind,plugin,0,0) != 0 )
            break;
        poll_daemons();
        sleep(1);
    }
    free(str);
    return(retstr);
}

int32_t got_newpeer(const char *ip_port) { if ( Debuglevel > 2 ) printf("got_newpeer.(%s)\n",ip_port); return(0); }

void *issue_cgicall(void *_ptr)
{
    char apitag[1024],plugin[1024],method[1024],*str = 0,*broadcaststr,*destNXT; uint32_t nonce; int32_t checklen,retlen,timeout; struct pending_cgi *ptr =_ptr;
    copy_cJSON(apitag,cJSON_GetObjectItem(ptr->json,"apitag"));
    safecopy(ptr->apitag,apitag,sizeof(ptr->apitag));
    copy_cJSON(plugin,cJSON_GetObjectItem(ptr->json,"agent"));
    if ( plugin[0] == 0 )
        copy_cJSON(plugin,cJSON_GetObjectItem(ptr->json,"plugin"));
    copy_cJSON(method,cJSON_GetObjectItem(ptr->json,"method"));
    timeout = get_API_int(cJSON_GetObjectItem(ptr->json,"timeout"),SUPERNET.PLUGINTIMEOUT);
    broadcaststr = cJSON_str(cJSON_GetObjectItem(ptr->json,"broadcast"));
    fprintf(stderr,"sock.%d (%s) API RECV.(%s)\n",ptr->sock,broadcaststr!=0?broadcaststr:"",ptr->jsonstr);
    if ( ptr->sock >= 0 && (ptr->retind= nn_connect(ptr->sock,apitag)) < 0 )
        fprintf(stderr,"error connecting to (%s)\n",apitag);
    else
    {
        destNXT = cJSON_str(cJSON_GetObjectItem(ptr->json,"destNXT"));
        if ( strcmp(plugin,"relay") == 0 || (broadcaststr != 0 && strcmp(broadcaststr,"publicaccess") == 0) || cJSON_str(cJSON_GetObjectItem(ptr->json,"servicename")) != 0 )
        {
            if ( Debuglevel > 1 )
                printf("call busdata_sync.(%s)\n",ptr->jsonstr);
            //printf("destNXT.(%s)\n",destNXT!=0?destNXT:"");
            str = busdata_sync(&nonce,ptr->jsonstr,broadcaststr,destNXT);
            //printf("got.(%s)\n",str);
        }
        else
        {
            if ( Debuglevel > 2 )
                fprintf(stderr,"call plugin_method.(%s)\n",ptr->jsonstr);
            str = plugin_method(ptr->sock,0,1,plugin,method,0,0,ptr->jsonstr,(int32_t)strlen(ptr->jsonstr)+1,timeout,0);
        }
        if ( str != 0 )
        {
            int32_t busdata_validate(char *forwarder,char *sender,uint32_t *timestamp,uint8_t *databuf,int32_t *datalenp,void *msg,cJSON *json);
            char forwarder[512],sender[512],methodstr[512]; uint32_t timestamp = 0; uint8_t databuf[8192]; int32_t valid=-1,datalen; cJSON *retjson,*argjson;
            forwarder[0] = sender[0] = 0;
            if ( (retjson= cJSON_Parse(str)) != 0 )
            {
                if ( is_cJSON_Array(retjson) != 0 && cJSON_GetArraySize(retjson) == 2 )
                {
                    argjson = cJSON_GetArrayItem(retjson,0);
                    copy_cJSON(methodstr,cJSON_GetObjectItem(argjson,"method"));
                    if ( strcmp(methodstr,"busdata") == 0 )
                    {
                        //fprintf(stderr,"call validate\n");
                        if ( (valid= busdata_validate(forwarder,sender,&timestamp,databuf,&datalen,str,retjson)) > 0 )
                        {
                            if ( datalen > 0 )
                            {
                                free(str);
                                str = malloc(datalen);
                                memcpy(str,databuf,datalen);
                            }
                        }
                    }
                }
                free_json(retjson);
            }
            //fprintf(stderr,"sock.%d mainstr.(%s) valid.%d sender.(%s) forwarder.(%s) time.%u\n",ptr->sock,str,valid,sender,forwarder,timestamp);
            if ( ptr->sock >= 0 )
            {
                retlen = (int32_t)strlen(str) + 1;
                if ( (checklen= nn_send(ptr->sock,str,retlen,0)) != retlen )
                    fprintf(stderr,"checklen.%d != len.%d for nn_send to (%s)\n",checklen,retlen,apitag);
                free(str), str = 0;
            }
        } else printf("null str returned\n");
    }
    if ( ptr->sock >= 0 )
    {
        //nn_freemsg(ptr->jsonstr);
        nn_shutdown(ptr->sock,ptr->retind);
        if ( str != 0 )
            free(str), str = 0;
    }
    free_json(ptr->json);
    free(ptr);
    return(str);
}

char *process_nn_message(int32_t sock,char *jsonstr)
{
    cJSON *json; struct pending_cgi *ptr; char *retstr = 0;
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        ptr = calloc(1,sizeof(*ptr));
        ptr->sock = sock;
        ptr->json = json;
        ptr->jsonstr = jsonstr;
        if ( sock >= 0 )
            portable_thread_create((void *)issue_cgicall,ptr);
        else retstr = issue_cgicall(ptr);
    } //else if ( sock >= 0 ) free(jsonstr);
    return(retstr);
}

char *process_jl777_msg(char *previpaddr,char *jsonstr,int32_t duration)
{
    char *process_user_json(char *plugin,char *method,char *cmdstr,int32_t broadcastflag,int32_t timeout);
    char buf[65536],plugin[MAX_JSON_FIELD],method[MAX_JSON_FIELD],request[MAX_JSON_FIELD],*bstr,*retstr;
    uint64_t daemonid,instanceid,tag;
    int32_t broadcastflag = 0;
    cJSON *json;
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        copy_cJSON(request,cJSON_GetObjectItem(json,"requestType"));
        copy_cJSON(plugin,cJSON_GetObjectItem(json,"plugin"));
        if ( plugin[0] == 0 )
            copy_cJSON(plugin,cJSON_GetObjectItem(json,"agent"));
        if ( strcmp(request,"install") == 0 && plugin[0] != 0 )
        {
            retstr = SuperNET_install(plugin,jsonstr,json);
            free_json(json);
            return(retstr);
        }
        tag = get_API_nxt64bits(cJSON_GetObjectItem(json,"tag"));
        daemonid = get_API_nxt64bits(cJSON_GetObjectItem(json,"daemonid"));
        instanceid = get_API_nxt64bits(cJSON_GetObjectItem(json,"instanceid"));
        copy_cJSON(method,cJSON_GetObjectItem(json,"method"));
        if ( (bstr= cJSON_str(cJSON_GetObjectItem(json,"broadcast"))) != 0 )
            broadcastflag = 1;
        else broadcastflag = 0;
        if ( method[0] == 0 )
        {
            strcpy(method,request);
            if ( plugin[0] == 0 && set_first_plugin(plugin,method) < 0 )
                return(clonestr("{\"error\":\"no method or plugin specified, search for requestType failed\"}"));
        }
        if ( strlen(jsonstr) < sizeof(buf)-1)
        {
            strcpy(buf,jsonstr);
            return(process_nn_message(-1,buf));
        }
    }
    return(clonestr("{\"error\":\"couldnt parse JSON\"}"));
}

char *SuperNET_JSON(char *jsonstr) // BTCD's entry point
{
    return(process_jl777_msg(0,jsonstr,60));
}

char *call_SuperNET_JSON(char *JSONstr) // sub-plugin's entry point
{
    char request[MAX_JSON_FIELD],name[MAX_JSON_FIELD],*retstr = 0;;
    uint64_t daemonid,instanceid;
    cJSON *json;
    if ( (json= cJSON_Parse(JSONstr)) != 0 )
    {
        copy_cJSON(request,cJSON_GetObjectItem(json,"requestType"));
        copy_cJSON(name,cJSON_GetObjectItem(json,"plugin"));
        if ( name[0] == 0 )
            copy_cJSON(name,cJSON_GetObjectItem(json,"agent"));
        if ( strcmp(request,"register") == 0 )
        {
            daemonid = get_API_nxt64bits(cJSON_GetObjectItem(json,"daemonid"));
            instanceid = get_API_nxt64bits(cJSON_GetObjectItem(json,"instanceid"));
            retstr = register_daemon(name,daemonid,instanceid,cJSON_GetObjectItem(json,"methods"),cJSON_GetObjectItem(json,"pubmethods"),cJSON_GetObjectItem(json,"authmethods"));
        } else retstr = process_jl777_msg(0,JSONstr,60);
        free_json(json);
    }
    if ( retstr == 0 )
        retstr = clonestr("{\"result\":\"call_SuperNET_JSON no response\"}");
    return(retstr);
}

char *SuperNET_url()
{
    static char url[64];
    //sprintf(urls[0],"http://127.0.0.1:%d",SUPERNET_PORT+1*0);
    sprintf(url,"https://127.0.0.1:%d",SUPERNET.port);
    return(url);
}

void SuperNET_loop(void *ipaddr)
{
    char *strs[16],jsonargs[512]; int32_t i,ind,n = 0;
    while ( SUPERNET.readyflag == 0 || find_daemoninfo(&ind,"SuperNET",0,0) == 0 )
    {
        if ( poll_daemons() > 0 )
            break;
        msleep(10);
    }
    sleep(1);
    sprintf(jsonargs,"{\"filename\":\"SuperNET.conf\"}");
    strs[n++] = language_func((char *)"kv777","",0,0,1,(char *)"kv777",jsonargs,call_system);
    while ( KV777.readyflag == 0 || find_daemoninfo(&ind,"kv777",0,0) == 0 )
         poll_daemons();
    if ( SUPERNET.gatewayid >= 0 )
    {
        strs[n++] = language_func((char *)"MGW","",0,0,1,(char *)"MGW",jsonargs,call_system);
        while ( MGW.readyflag == 0 || find_daemoninfo(&ind,"MGW",0,0) == 0 )
            poll_daemons();
    }
    strs[n++] = language_func((char *)"coins","",0,0,1,(char *)"coins",jsonargs,call_system);
    while ( COINS.readyflag == 0 || find_daemoninfo(&ind,"coins",0,0) == 0 )
        poll_daemons();
    if ( SUPERNET.gatewayid >= 0 )
    {
        strs[n++] = language_func((char *)"ramchain","",0,0,1,(char *)"ramchain",jsonargs,call_system);
        while ( RAMCHAINS.readyflag == 0 || find_daemoninfo(&ind,"ramchain",0,0) == 0 )
            poll_daemons();
    }
    strs[n++] = language_func((char *)"relay","",0,0,1,(char *)"relay",jsonargs,call_system);
    while ( RELAYS.readyflag == 0 || find_daemoninfo(&ind,"relay",0,0) == 0 )
        poll_daemons();
    /*strs[n++] = language_func((char *)"peers","",0,0,1,(char *)"peers",jsonargs,call_system);
    while ( PEERS.readyflag == 0 || find_daemoninfo(&ind,"peers",0,0) == 0 )
        poll_daemons();
    strs[n++] = language_func((char *)"subscriptions","",0,0,1,(char *)"subscriptions",jsonargs,call_system);
    while ( SUBSCRIPTIONS.readyflag == 0 || find_daemoninfo(&ind,"subscriptions",0,0) == 0 )
        poll_daemons();*/
    if ( SUPERNET.gatewayid < 0 )
    {
        strs[n++] = language_func((char *)"InstantDEX","",0,0,1,(char *)"InstantDEX",jsonargs,call_system);
        while ( INSTANTDEX.readyflag == 0 || find_daemoninfo(&ind,"InstantDEX",0,0) == 0 )
            poll_daemons();
    }
    for (i=0; i<n; i++)
    {
        printf("%s ",strs[i]);
        free(strs[i]);
    }
    printf("num builtin plugin agents.%d\n",n);
    if ( SUPERNET.gatewayid >= 0 )
        printf("MGW sock = %d\n",MGW.all.socks.both.bus);
    sleep(3);
    void serverloop(void *_args);
    serverloop(0);
}

void SuperNET_apiloop(void *ipaddr)
{
    char buf[65536],*jsonstr,*str; int32_t sock,len;
    if ( (sock= nn_socket(AF_SP,NN_PAIR)) >= 0 )
    {
        if ( nn_bind(sock,SUPERNET_APIENDPOINT) < 0 )
            fprintf(stderr,"error binding to relaypoint sock.%d type.%d (%s) %s\n",sock,NN_BUS,SUPERNET_APIENDPOINT,nn_errstr());
        else
        {
            if ( nn_settimeouts(sock,10,1) < 0 )
                fprintf(stderr,"error setting sendtimeout %s\n",nn_errstr());
            fprintf(stderr,"BIND.(%s) sock.%d\n",SUPERNET_APIENDPOINT,sock);
            while ( 1 )
            {
                if ( (len= nn_recv(sock,&jsonstr,NN_MSG,0)) > 0 )
                {
                    fprintf(stderr,"apirecv.(%s)\n",jsonstr);
                    if ( strlen(jsonstr) < sizeof(buf) )
                    {
                        strcpy(buf,jsonstr);
                        nn_freemsg(jsonstr);
                        if ( (str= process_nn_message(sock,buf)) != 0 )
                            free(str);
                    }
                }
            }
        }
        nn_shutdown(sock,0);
    }
}
    
int SuperNET_start(char *fname,char *myip)
{
    int32_t init_SUPERNET_pullsock(int32_t sendtimeout,int32_t recvtimeout);
    int32_t parse_ipaddr(char *ipaddr,char *ip_port);
    char ipaddr[256],*jsonstr = 0;
    uint64_t allocsize;
    printf("myip.(%s)\n",myip);
    portable_OS_init();
    init_SUPERNET_pullsock(10,1);
    Debuglevel = 2;
    if ( (jsonstr= loadfile(&allocsize,fname)) == 0 )
        jsonstr = clonestr("{}");
    parse_ipaddr(ipaddr,myip);
    strcpy(SUPERNET.myipaddr,ipaddr);
    printf("SuperNET_start myip.(%s) -> ipaddr.(%s)\n",myip!=0?myip:"",ipaddr);
    language_func("SuperNET","",0,0,1,"SuperNET",jsonstr,call_system);
    if ( jsonstr != 0 )
        free(jsonstr);
    portable_thread_create((void *)SuperNET_loop,myip);
    portable_thread_create((void *)SuperNET_apiloop,myip);
    return(0);
}

#ifdef STANDALONE
int32_t SuperNET_broadcast(char *msg,int32_t duration) { printf(">>>>>>>>> BROADCAST.(%s)\n",msg); return(0); }
int32_t SuperNET_narrowcast(char *destip,unsigned char *msg,int32_t len) { printf(">>>>>>>>>>> NARROWCAST.(%s) -> (%s)\n",msg,destip);  return(0); }

int main(int argc,const char *argv[])
{
    char _ipaddr[64],*jsonstr = 0,*ipaddr = "127.0.0.1:7777";
    int32_t i;
    cJSON *json = 0;
    uint64_t ipbits,allocsize;
#ifdef __APPLE__
    //void SaM_PrepareIndices();
    //int32_t SaM_test();
    //SaM_PrepareIndices();
    //SaM_test();
    //printf("finished SaM_test\n");
    void kv777_test(int32_t n);
    //kv777_test(10000);
    //getchar();
    if ( 0 )
    {
        bits128 calc_md5(char digeststr[33],void *buf,int32_t len);
        char digeststr[33],*str = "abc";
        calc_md5(digeststr,str,(int32_t)strlen(str));
        printf("(%s) -> (%s)\n",str,digeststr);
        getchar();
    }
    while ( 0 )
    {
        uint32_t nonce,failed; int32_t leverage;
        nonce = nonce_func(&leverage,"test string","allrelays",3000,0);
        failed = nonce_func(&leverage,"test string","allrelays",0,nonce);
        printf("nonce.%u failed.%u\n",nonce,failed);
    }
#endif
    if ( (jsonstr= loadfile(&allocsize,"SuperNET.conf")) == 0 )
        jsonstr = clonestr("{}");
    else if ( (json= cJSON_Parse(jsonstr)) == 0 )
    {
        printf("error parsing SuperNET.conf.(%s)\n",jsonstr);
        free(jsonstr);
        exit(-1);
    }
    else free_json(json);
    i = 1;
    if ( argc > 1 )
    {
        ipbits = calc_ipbits((char *)argv[1]);
        expand_ipbits(_ipaddr,ipbits);
        if ( strcmp(_ipaddr,argv[1]) == 0 )
            ipaddr = (char *)argv[1], i++;
    }
    ipbits = calc_ipbits(ipaddr);
    expand_ipbits(_ipaddr,ipbits);
    printf(">>>>>>>>> call SuperNET_start.(%s)\n",ipaddr);
    SuperNET_start("SuperNET.conf",ipaddr);
    printf("<<<<<<<<< back SuperNET_start\n");
    if ( i < argc )
    {
        for (; i<argc; i++)
            if ( is_bundled_plugin((char *)argv[i]) != 0 )
                language_func((char *)argv[i],"",0,0,1,(char *)argv[i],jsonstr,call_system);
    }
    sleep(60);

    while ( 1 )
    {
        char line[1024];
        if ( getline777(line,sizeof(line)-1) > 0 )
            process_userinput(line);
    }
    return(0);
}
#endif
