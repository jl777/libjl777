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

//#include "SuperNET.h"
#include "cJSON.h"
#define NUM_GATEWAYS 3
/*extern char Server_names[256][MAX_JSON_FIELD],MGWROOT[];
extern char Server_NXTaddrs[256][MAX_JSON_FIELD];
extern int32_t IS_LIBTEST,USESSL,SUPERNET_PORT,ENABLE_GUIPOLL,Debuglevel,UPNP,MULTIPORT,Finished_init;
extern cJSON *MGWconf;
#define issue_curl(curl_handle,cmdstr) bitcoind_RPC(curl_handle,"curl",cmdstr,0,0,0)
void expand_ipbits(char *ipaddr,uint32_t ipbits);
uint64_t conv_acctstr(char *acctstr);
void calc_sha256(char hashstr[(256 >> 3) * 2 + 1],unsigned char hash[256 >> 3],unsigned char *src,int32_t len);
int32_t decode_hex(unsigned char *bytes,int32_t n,char *hex);
int32_t expand_nxt64bits(char *NXTaddr,uint64_t nxt64bits);
char *clonestr(char *);
int32_t init_hexbytes_noT(char *hexbytes,unsigned char *message,long len);
char *_mbstr(double n);
char *_mbstr2(double n);
struct coin_info *get_coin_info(char *coinstr);
uint32_t get_blockheight(struct coin_info *cp);
long stripwhite_ns(char *buf,long len);
int32_t safecopy(char *dest,char *src,long len);*/

#define INCLUDE_CODE
//#include "ramchain.h"
#undef INCLUDE_CODE



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
            sprintf(fname,"%s/MGW/status/%s.%d",MGWROOT,coin,i);
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
    sprintf(fname,"%s/gateway%d/%s",MGWROOT,gatewayid,userNXTaddr);
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
        sprintf(fname,"%s/gateway%d/%s",MGWROOT,gatewayid,name);
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
    if ( 0 )
    {
        struct db777 *db777_create(char *path,char *name,char *compression);
        int32_t db777_close(struct db777 *DB);
        int32_t db777_addstr(struct db777 *DB,char *key,char *value);
        int32_t db777_findstr(char *retbuf,int32_t max,struct db777 *DB,char *key);
        struct db777 *db;
        int i;
        char buf[16],field[64],retbuf[65536];
        db = db777_create("/tmp","test",0);
        for (i=0; i<100000; i++)
        {
            sprintf(field,"field.%d",i);
            db777_findstr(retbuf,sizeof(retbuf),db,field);
            printf("%s\n",retbuf);
            strcpy(buf,field);
            db777_addstr(db,field,buf);
        }
        db777_close(db);
        db = db777_create("/tmp","zstd","zstd");
        for (i=0; i<100000; i++)
        {
            sprintf(field,"field.%d",i);
            strcpy(buf,field);
            db777_addstr(db,field,buf);
        }
        db777_close(db);
        
        db = db777_create("/tmp","lz4","lz4");
        for (i=0; i<100000; i++)
        {
            sprintf(field,"field.%d",i);
            strcpy(buf,field);
            db777_addstr(db,field,buf);
        }
        db777_close(db);
        getchar();
    }
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
    if ( UPNP != 0 && upnpredirect(oldport,newport,"UDP","SuperNET_https") == 0 )
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
extern char WEBSOCKETD[];

#define DEFINES_ONLY
#include "cJSON.h"
#include "utils777.c"
#include "files777.c"
#include "plugins/plugins.h"

int SuperNET_retval,MAP_HUFF,MGW_initdone,Debuglevel,Finished_init,MIN_NQTFEE,SUPERNET_PORT,USESSL,Gatewayid = -1;
char SOPHIA_DIR[MAX_JSON_FIELD],NXT_ASSETIDSTR[MAX_JSON_FIELD],NXTSERVER[MAX_JSON_FIELD],WEBSOCKETD[MAX_JSON_FIELD],MGWROOT[MAX_JSON_FIELD],NXTAPIURL[MAX_JSON_FIELD];


int32_t got_newpeer(const char *ip_port) { printf("got_newpeer.(%s)\n",ip_port); return(0); }

//static char *plugin[] = { "plugin", "method", "daemonid", "instanceid", "tag",  "async", "iters", 0 };
//static char *install[] = "requestType":"install"  { "path", "plugin", "daemonize", "websocket", "ipaddr", "port", 0 };

char *process_jl777_msg(char *previpaddr,char *jsonstr,int32_t duration)
{
    char plugin[MAX_JSON_FIELD],method[MAX_JSON_FIELD],request[MAX_JSON_FIELD],ipaddr[MAX_JSON_FIELD],path[MAX_JSON_FIELD];
    uint64_t daemonid,instanceid,tag;
    int32_t ind,async,n = 1;
    uint16_t port,websocket;
    cJSON *json;
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        copy_cJSON(request,cJSON_GetObjectItem(json,"requestType"));
        copy_cJSON(plugin,cJSON_GetObjectItem(json,"plugin"));
        if ( strcmp(request,"install") == 0 && plugin[0] != 0 )
        {
            if ( find_daemoninfo(&ind,plugin,0,0) != 0 )
                return(clonestr("{\"error\":\"plugin already installed\"}"));
            copy_cJSON(path,cJSON_GetObjectItem(json,"path"));
            copy_cJSON(ipaddr,cJSON_GetObjectItem(json,"ipaddr"));
            port = get_API_int(cJSON_GetObjectItem(json,"port"),0);
            async = get_API_int(cJSON_GetObjectItem(json,"daemonize"),0);
            websocket = get_API_int(cJSON_GetObjectItem(json,"websocket"),0);
            return(language_func(plugin,ipaddr,port,websocket,async,path,jsonstr,call_system));
        }
        tag = get_API_nxt64bits(cJSON_GetObjectItem(json,"daemonid"));
        daemonid = get_API_nxt64bits(cJSON_GetObjectItem(json,"daemonid"));
        instanceid = get_API_nxt64bits(cJSON_GetObjectItem(json,"instanceid"));
        copy_cJSON(method,cJSON_GetObjectItem(json,"method"));
        if ( method[0] == 0 )
        {
            strcpy(method,request);
            if ( plugin[0] == 0 && set_first_plugin(plugin,method) < 0 )
                return(clonestr("{\"error\":\"no method or plugin specified, search for requestType failed\"}"));
        }
        n = get_API_int(cJSON_GetObjectItem(json,"iters"),1);
        async = get_API_int(cJSON_GetObjectItem(json,"async"),0);
        return(plugin_method(previpaddr,plugin,method,daemonid,instanceid,tag!=0?tag:rand(),jsonstr,n,async));
    } else return(clonestr("{\"error\":\"couldnt parse JSON\"}"));
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
        if ( strcmp(request,"register") == 0 )
        {
            daemonid = get_API_nxt64bits(cJSON_GetObjectItem(json,"daemonid"));
            instanceid = get_API_nxt64bits(cJSON_GetObjectItem(json,"instanceid"));
            retstr = register_daemon(name,daemonid,instanceid,cJSON_GetObjectItem(json,"methods"));
        }
        free_json(json);
    } else retstr = process_jl777_msg(0,JSONstr,60);
    if ( retstr == 0 )
        retstr = clonestr("{\"result\":\"call_SuperNET_JSON no response\"}");
    return(retstr);
}

int SuperNET_start(char *jsonstr,char *myip)
{
    cJSON *json = cJSON_Parse(jsonstr);
    FILE *fp;
    Debuglevel = 2;
    SUPERNET_PORT = 7777;
    if ( (fp= fopen("libs/websocketd","rb")) != 0 )
    {
        fclose(fp);
        strcpy(WEBSOCKETD,"libs/websocketd");
    }
    else strcpy(WEBSOCKETD,"websocketd");
    strcpy(SOPHIA_DIR,"./DB");
    if ( json != 0 )
    {
        extract_cJSON_str(SOPHIA_DIR,sizeof(SOPHIA_DIR),json,"SOPHIA_DIR");
    }
    os_compatible_path(SOPHIA_DIR);
    language_func("SuperNET","",0,0,1,"SuperNET","{}",call_system);
    return(0);
}

char *SuperNET_url()
{
    static char urls[2][64];
    sprintf(urls[0],"http://127.0.0.1:%d",SUPERNET_PORT+1*0);
    sprintf(urls[1],"https://127.0.0.1:%d",SUPERNET_PORT);
    return(urls[USESSL]);
}

void SuperNET_loop(void *ipaddr)
{
    int32_t i;
    printf("start SuperNET.(%s)\n",ipaddr);
    SuperNET_start("SuperNET.conf",ipaddr);
    while ( 1 )
    {
        for (i=0; i<1000; i++)
            if ( poll_daemons() <= 0 )
                break;
        msleep(100);
        //fprintf(stderr,".");
    }
}

int32_t launch_SuperNET(char *ipaddr)
{
    portable_thread_create((void *)SuperNET_loop,ipaddr);
    return(0);
}

#ifdef STANDALONE
int32_t SuperNET_broadcast(char *msg,int32_t duration) { printf(">>>>>>>>> BROADCAST.(%s)\n",msg); return(0); }
int32_t SuperNET_narrowcast(char *destip,unsigned char *msg,int32_t len) { printf(">>>>>>>>>>> NARROWCAST.(%s) -> (%s)\n",msg,destip);  return(0); }

int main(int argc,const char *argv[])
{
    char _ipaddr[64],*jsonstr = 0,*ipaddr = "127.0.0.1:7777";//,*ipaddr6 = "[2001:16d8:dd24:0:86c9:681e:f931:256]";
    int32_t i;
    cJSON *json = 0;
    uint64_t ipbits,allocsize;
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
    printf("(%s -> %s)\n",ipaddr,_ipaddr);
    SuperNET_start("SuperNET.conf",ipaddr);
    if ( i < argc )
    {
        for (; i<argc; i++)
            if ( is_bundled_plugin((char *)argv[i]) != 0 )
                language_func((char *)argv[i],"",0,0,1,(char *)argv[i],jsonstr,call_system);
    }
    while ( 1 )
    {
        for (i=0; i<1000; i++)
            if ( poll_daemons() <= 0 )
                break;
        sleep(10);
    }
    free(jsonstr);
    return(0);
}
#endif

