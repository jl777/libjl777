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
#ifndef _WIN32
#include <arpa/inet.h>
#endif
#include <sys/time.h>
#include "uthash.h"

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


#include "SuperNET.h"
#include "cJSON.h"
#define NUM_GATEWAYS 3
extern char Server_names[256][MAX_JSON_FIELD],MGWROOT[];
extern char Server_NXTaddrs[256][MAX_JSON_FIELD];
extern int32_t IS_LIBTEST,USESSL,SUPERNET_PORT,ENABLE_GUIPOLL,Debuglevel,UPNP,MULTIPORT,Finished_init;
extern cJSON *MGWconf;
#define issue_curl(curl_handle,cmdstr) bitcoind_RPC(curl_handle,"curl",cmdstr,0,0,0)
char *bitcoind_RPC(void *deprecated,char *debugstr,char *url,char *userpass,char *command,char *params);
void expand_ipbits(char *ipaddr,uint32_t ipbits);
uint64_t conv_acctstr(char *acctstr);
void calc_sha256(char hashstr[(256 >> 3) * 2 + 1],unsigned char hash[256 >> 3],unsigned char *src,int32_t len);
int32_t decode_hex(unsigned char *bytes,int32_t n,char *hex);
int32_t expand_nxt64bits(char *NXTaddr,uint64_t nxt64bits);
char *clonestr(char *);
int32_t init_hexbytes_noT(char *hexbytes,unsigned char *message,long len);
char *_mbstr(double n);
char *_mbstr2(double n);
double milliseconds();
struct coin_info *get_coin_info(char *coinstr);
uint32_t get_blockheight(struct coin_info *cp);
long stripwhite_ns(char *buf,long len);
int32_t safecopy(char *dest,char *src,long len);
double estimate_completion(char *coinstr,double startmilli,int32_t processed,int32_t numleft);


//uint32_t conv_rawind(uint32_t huffid,uint32_t rawind) { return((rawind << 4) | (huffid&0xf)); }


#define INCLUDE_CODE
#include "ramchain.h"
#undef INCLUDE_CODE


/*cJSON *gen_blockjson(struct compressionvars *V,uint32_t blocknum)
{
    cJSON *array,*item,*json = cJSON_CreateObject();
    char *txidstr,*addr,*script;
    struct blockinfo *block,*next;
    struct address_entry *vin;
    struct voutinfo *vout;
    int32_t ind;
    if ( (block= get_blockinfo(V,blocknum)) != 0 && (next= get_blockinfo(V,blocknum+1)) != 0 )
    {
        printf("block.%d: (%d %d) next.(%d %d)\n",blocknum,block->firstvin,block->firstvout,next->firstvout,next->firstvout);
        if ( block->firstvin <= next->firstvin && block->firstvout <= next->firstvout )
        {
            if ( next->firstvin > block->firstvin )
            {
                array = cJSON_CreateArray();
                for (ind= block->firstvin; ind<next->firstvin; ind++)
                {
                    if ( (vin= get_vininfo(V,ind)) != 0 )
                    {
                        item = cJSON_CreateObject();
                        cJSON_AddItemToObject(item,"block",cJSON_CreateNumber(vin->blocknum));
                        cJSON_AddItemToObject(item,"txind",cJSON_CreateNumber(vin->txind));
                        cJSON_AddItemToObject(item,"vout",cJSON_CreateNumber(vin->v));
                        cJSON_AddItemToArray(array,item);
                    }
                }
                cJSON_AddItemToObject(json,"vins",array);
            }
            if ( next->firstvout > block->firstvout )
            {
                array = cJSON_CreateArray();
                for (ind= block->firstvout; ind<next->firstvout; ind++)
                {
                    if ( (vout= get_voutinfo(V,ind)) != 0 )
                    {
                        item = cJSON_CreateObject();
                        if ( (txidstr= conv_txidind(V,vout->tp_ind)) != 0 )
                            cJSON_AddItemToObject(item,"txid",cJSON_CreateString(txidstr));
                        cJSON_AddItemToObject(item,"vout",cJSON_CreateNumber(vout->vout));
                        cJSON_AddItemToObject(item,"value",cJSON_CreateNumber(dstr(vout->value)));
                        if ( (addr= conv_addrind(V,vout->addr_ind)) != 0 )
                            cJSON_AddItemToObject(item,"addr",cJSON_CreateString(addr));
                        if ( (script= conv_scriptind(V,vout->sp_ind)) != 0 )
                            cJSON_AddItemToObject(item,"script",cJSON_CreateString(script));
                        cJSON_AddItemToArray(array,item);
                    }
                }
                cJSON_AddItemToObject(json,"vouts",array);
            }
        } else cJSON_AddItemToObject(json,"error",cJSON_CreateString("block firstvin or firstvout violation"));
    }
    return(json);
}*/

char *SuperNET_url()
{
    static char urls[2][64];
    sprintf(urls[0],"http://127.0.0.1:%d",SUPERNET_PORT+1);
    sprintf(urls[1],"https://127.0.0.1:%d",SUPERNET_PORT);
    return(urls[USESSL]);
}

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
            sprintf(fname,"%s/MGW/status/%s.%s",MGWROOT,coin,Server_ipaddrs[i]);
            if ( (fp= fopen(fname,"rb")) != 0 )
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
                                sleep(1);
                            }
                        }
                        sleep(3);
                    }
                }
            }
            else
            {
                for (iter=0; iter<3; iter++) // give chance for servers to consensus
                {
                    issue_genmultisig(coin,userNXTaddr,userpubkey,email,buyNXT);
                    sleep(3);
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
            } else usleep(3000);
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
}

char *load_filestr(char *userNXTaddr,int32_t gatewayid)
{
    long fpos;
    FILE *fp;
    char fname[1024],*buf=0,*retstr = 0;
    sprintf(fname,"%s/gateway%d/%s",MGWROOT,gatewayid,userNXTaddr);
    if ( (fp= fopen(fname,"rb")) != 0 )
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
        if ( (fp= fopen(fname,"wb+")) != 0 )
        {
            fwrite(args->data,1,args->totallen,fp);
            fclose(fp);
            sprintf(cmd,"chmod +r %s",fname);
            system(cmd);
        }
    }
    printf("bridge_handler.gateway%d/(%s).%d\n",gatewayid,name,args->totallen);
}

void *GUIpoll_loop(void *arg)
{
    cJSON *json;
    uint16_t port;
    int32_t sleeptime = 0;
    char txidstr[MAX_JSON_FIELD],buf[MAX_JSON_FIELD],senderipaddr[MAX_JSON_FIELD],*retstr;
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
        if ( sleeptime != 0 )
            sleep(sleeptime);
    }
    return(0);
}

// redirect port on external upnp enabled router to port on *this* host
int upnpredirect(const char* eport, const char* iport, const char* proto, const char* description) {
    
    //  Discovery parameters
    struct UPNPDev * devlist = 0;
    struct UPNPUrls urls;
    struct IGDdatas data;
    int i;
    char lanaddr[64];	// my ip address on the LAN
    const char* leaseDuration="0";
    
    //  Redirect & test parameters
    char intClient[40];
    char intPort[6];
    char externalIPAddress[40];
    char duration[16];
    int error=0;
    
    //  Find UPNP devices on the network
    if ((devlist=upnpDiscover(2000, 0, 0,0, 0, &error))) {
        struct UPNPDev * device = 0;
        printf("UPNP INIALISED: List of UPNP devices found on the network.\n");
        for(device = devlist; device; device = device->pNext) {
            printf("UPNP INFO: dev [%s] \n\t st [%s]\n",
                   device->descURL, device->st);
        }
    } else {
        printf("UPNP ERROR: no device found - MANUAL PORTMAP REQUIRED\n");
        return 0;
    }
    
    //  Output whether we found a good one or not.
    if((error = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr)))) {
        switch(error) {
            case 1:
                printf("UPNP OK: Found valid IGD : %s\n", urls.controlURL);
                break;
            case 2:
                printf("UPNP WARN: Found a (not connected?) IGD : %s\n", urls.controlURL);
                break;
            case 3:
                printf("UPNP WARN: UPnP device found. Is it an IGD ? : %s\n", urls.controlURL);
                break;
            default:
                printf("UPNP WARN: Found device (igd ?) : %s\n", urls.controlURL);
        }
        printf("UPNP OK: Local LAN ip address : %s\n", lanaddr);
    } else {
        printf("UPNP ERROR: no device found - MANUAL PORTMAP REQUIRED\n");
        return 0;
    }
    
    //  Get the external IP address (just because we can really...)
    if(UPNP_GetExternalIPAddress(urls.controlURL,
                                 data.first.servicetype,
                                 externalIPAddress)!=UPNPCOMMAND_SUCCESS)
        printf("UPNP WARN: GetExternalIPAddress failed.\n");
    else
        printf("UPNP OK: ExternalIPAddress = %s\n", externalIPAddress);
    
    //  Check for existing supernet mapping - from this host and another host
    //  In theory I can adapt this so multiple nodes can exist on same lan and choose a different portmap
    //  for each one :)
    //  At the moment just delete a conflicting portmap and override with the one requested.
    i=0;
    error=0;
    do {
        char index[6];
        char extPort[6];
        char desc[80];
        char enabled[6];
        char rHost[64];
        char protocol[4];
        
        snprintf(index, 6, "%d", i++);
        
        if(!(error=UPNP_GetGenericPortMappingEntry(urls.controlURL,
                                                   data.first.servicetype,
                                                   index,
                                                   extPort, intClient, intPort,
                                                   protocol, desc, enabled,
                                                   rHost, duration))) {
            // printf("%2d %s %5s->%s:%-5s '%s' '%s' %s\n",i, protocol, extPort, intClient, intPort,desc, rHost, duration);
            
            // check for an existing supernet mapping on this host
            if(!strcmp(lanaddr, intClient)) { // same host
                if(!strcmp(protocol,proto)) { //same protocol
                    if(!strcmp(intPort,iport)) { // same port
                        printf("UPNP WARN: existing mapping found (%s:%s)\n",lanaddr,iport);
                        if(!strcmp(extPort,eport)) {
                            printf("UPNP OK: exact mapping already in place (%s:%s->%s)\n", lanaddr, iport, eport);
                            FreeUPNPUrls(&urls);
                            freeUPNPDevlist(devlist);
                            return 1;
                            
                        } else { // delete old mapping
                            printf("UPNP WARN: deleting existing mapping (%s:%s->%s)\n",lanaddr, iport, extPort);
                            if(UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, extPort, proto, rHost))
                                printf("UPNP WARN: error deleting old mapping (%s:%s->%s) continuing\n", lanaddr, iport, extPort);
                            else printf("UPNP OK: old mapping deleted (%s:%s->%s)\n",lanaddr, iport, extPort);
                        }
                    }
                }
            } else { // ipaddr different - check to see if requested port is already mapped
                if(!strcmp(protocol,proto)) {
                    if(!strcmp(extPort,eport)) {
                        printf("UPNP WARN: EXT port conflict mapped to another ip (%s-> %s vs %s)\n", extPort, lanaddr, intClient);
                        if(UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, extPort, proto, rHost))
                            printf("UPNP WARN: error deleting conflict mapping (%s:%s) continuing\n", intClient, extPort);
                        else printf("UPNP OK: conflict mapping deleted (%s:%s)\n",intClient, extPort);
                    }
                }
            }
        } else
            printf("UPNP OK: GetGenericPortMappingEntry() End-of-List (%d entries) \n", i);
    } while(error==0);
    
    //  Set the requested port mapping
    if((i=UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                              eport, iport, lanaddr, description,
                              proto, 0, leaseDuration))!=UPNPCOMMAND_SUCCESS) {
        printf("UPNP ERROR: AddPortMapping(%s, %s, %s) failed with code %d (%s)\n",
               eport, iport, lanaddr, i, strupnperror(i));
        
        FreeUPNPUrls(&urls);
        freeUPNPDevlist(devlist);
        return 0; //error - adding the port map primary failure
    }
    
    if((i=UPNP_GetSpecificPortMappingEntry(urls.controlURL,
                                           data.first.servicetype,
                                           eport, proto, NULL/*remoteHost*/,
                                           intClient, intPort, NULL/*desc*/,
                                           NULL/*enabled*/, duration))!=UPNPCOMMAND_SUCCESS) {
        printf("UPNP ERROR: GetSpecificPortMappingEntry(%s, %s, %s) failed with code %d (%s)\n", eport, iport, lanaddr,
               i, strupnperror(i));
        FreeUPNPUrls(&urls);
        freeUPNPDevlist(devlist);
        return 0; //error - port map wasnt returned by query so likely failed.
    }
    else printf("UPNP OK: EXT (%s:%s) %s redirected to INT (%s:%s) (duration=%s)\n",externalIPAddress, eport, proto, intClient, intPort, duration);
    FreeUPNPUrls(&urls);
    freeUPNPDevlist(devlist);
    return 1; //ok - we are mapped:)
}
/*
#include "regex/lua-regex.h"

void luatest(char *str,char *pattern)
{
    LuaMatchState ms;
    int i,init = 0;
    printf("str.(%s) pattern.(%s): ",str,pattern);
    while ( (init= lua_find(&ms,str,strlen(str),pattern,strlen(pattern),init,0)) != 0 )
    {
        for (i=0; i<ms.level; i++)
        {
            if ( ms.capture[i].len == CAP_POSITION )
                printf("(pos %d:%d l.%d)\t",i,(int)ms.capture[i].init,ms.level);
            else
            {
                printf("[%d:%s].%ld\t",i,ms.capture[i].init,ms.capture[i].len);
            }
        }
    }
    printf("init.%d\n",init);
}
#include <regex.h>

// The following is the size of a buffer to contain any error messages encountered when the regular expression is compiled.

#define MAX_ERROR_MSG 0x1000

// Compile the regular expression described by "regex_text" into "r".

static int compile_regex (regex_t * r, const char * regex_text)
{
    int status = regcomp (r, regex_text, REG_EXTENDED|REG_NEWLINE);
    if (status != 0) {
        char error_message[MAX_ERROR_MSG];
        regerror (status, r, error_message, MAX_ERROR_MSG);
        printf ("Regex error compiling '%s': %s\n",
                regex_text, error_message);
        return 1;
    }
    return 0;
}

// Match the string in "to_match" against the compiled regular expression in "r".
 

static int match_regex (regex_t * r, const char * to_match)
{
    // "P" is a pointer into the string which points to the end of the previous match.
    const char * p = to_match;
    // "N_matches" is the maximum number of matches allowed.
    const int n_matches = 10;
    // "M" contains the matches found.
    regmatch_t m[n_matches];
    
    while (1)
    {
        int i = 0;
        int nomatch = regexec (r, p, n_matches, m, 0);
        if (nomatch) {
            printf ("No more matches.\n");
            return nomatch;
        }
        for (i = 0; i < n_matches; i++) {
            int start;
            int finish;
            if (m[i].rm_so == -1) {
                break;
            }
            start = (int)(m[i].rm_so + (p - to_match));
            finish = (int)(m[i].rm_eo + (p - to_match));
            if (i == 0) {
                printf ("$& is ");
            }
            else {
                printf ("$%d is ", i);
            }
            printf ("'%.*s' (bytes %d:%d)\n", (finish - start),
                    to_match + start, start, finish);
        }
        if ( m[0].rm_eo == 0 )
            break;
        else p += m[0].rm_eo;
    }
    return 0;
}

int regexptest()
{
    regex_t r;
    const char * regex_text;
    const char * find_text;
    //regex_text = "([[:digit:]]+)[^[:digit:]]+([[:digit:]]+)";
    regex_text = "ni.*";
    find_text = "This 1 is nice 2 so 33 for 4254";
    printf ("Trying to find '%s' in '%s'\n", regex_text, find_text);
    compile_regex(& r, regex_text);
    match_regex(& r, find_text);
    regfree (& r);
    return 0;
}


int32_t bitcoin_assembler(char *script);

void unscript()
{
    char r,i,buf[1024],*teststrs[16] = { "OP_DUP OP_HASH160 375e874aea7f0c8438282396ad09e212eed746b6 OP_EQUALVERIFY OP_CHECKSIG","OP_RETURN 0", "1 02860cc9b85cfc7f16ca855a6847aca6511c63e11c1e3c9ddbbb460e186652be6e 02c3eee9c06cd7ffa3ca45451fa65feb87914d43fbdeca7d94549a109bbc6979ac 2 OP_CHECKMULTISIG", "1 0210f4b684bdd114f4ac57bb3ab405cb0f5d6d32fc129fa24d9635dc8c010f92f8 02403fb3e7e4e8884ffcc39fd5b6d8d91fe24001f83d363fc2defe2058776759e0 02bbd5514ca46ca80b71720c5683720b9895a5d81fd3410111a8c2af4d52c034c0 3 OP_CHECKMULTISIG", "OP_SIZE OP_TUCK 32 35 OP_WITHIN OP_VERIFY OP_SHA256 197bf68fb520e8d3419dc1d4ac1eb89e7dfd7cfe561c19abf7611d7626d9f02c OP_EQUALVERIFY OP_SWAP OP_SIZE OP_TUCK 32 35 OP_WITHIN OP_VERIFY OP_SHA256 f531f3041d3136701ea09067c53e7159c8f9b2746a56c3d82966c54bbc553226 OP_EQUALVERIFY OP_ROT OP_SIZE OP_TUCK 32 35 OP_WITHIN OP_VERIFY OP_SHA256 527ccdd755dcccf03192383624e0a7d0263815ce2ecf1f69cb0423ab7e6f0f3e OP_EQUALVERIFY OP_ADD OP_ADD 96 OP_SUB OP_DUP 2 OP_GREATERTHAN OP_IF 3 OP_SUB OP_ENDIF OP_DUP 2 OP_GREATERTHAN OP_IF 3 OP_SUB OP_ENDIF 04d4bf4642f56fc7af0d2382e2cac34fa16ed3321633f91d06128f0e5c0d17479778cc1f2cc7e4a0c6f1e72d905532e8e127a031bb9794b3ef9b68b657f51cc691 04208a50909284aede02ad107bb1f52175b025cdf0453537b686433bcade6d3e210b6c82bcbdf8676b2161687e232f5d9afdaa4ed7b3e3bf9608d41b40ebde6ed4 04c9ce67ff2df2cd6be5f58345b4e311c5f10aab49d3cf3f73e8dcac1f9cd0de966e924be091e7bc854aef0d0baafa80fe5f2d6af56b1788e1e8ec8d241b41c40d 3 OP_ROLL OP_ROLL 3 OP_ROLL OP_SWAP OP_CHECKSIGVERIFY" };
    for (i=0; i<5; i++)
    {
        strcpy(buf,teststrs[i]);
        printf("(%s) ",teststrs[i]);
        r = bitcoin_assembler(buf);
        printf("i.%d r.%d {%s}\n",i,r,buf);
    }
    getchar();
}*/

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


/*

// seconds to forge from last block: hit / ( basetarget * effective balanceNXT)
var nbst = Math.floor((((blk.baseTarget*accum2)/60)/153722867)*100);
if(nbst < bst/2) nbst = bst/2;
if(nbst > bst*2) nbst = bst*2;
var rbst = Math.floor((((blk.baseTarget*lasttime)/60)/153722867)*100);

*/

/*A * B is calculated as A0*B0 + A1*B1

come-from-beyond [11:30 PM]
|A|*|B| is calculated as sqrt(A0*A0+A1*A1)*sqrt(B0*B0+B1*B1)

come-from-beyond [11:30 PM]
so, cosine = A*B / (|A|*|B|)*/

double cos64bits(uint64_t x,uint64_t y)
{
    static double sqrts[65],sqrtsB[64*64+1];
    int32_t i,wta,wtb,dot;
    if ( sqrts[1] == 0. )
    {
        for (i=1; i<=64; i++)
            sqrts[i] = sqrt(i);
        for (i=1; i<=64*64; i++)
            sqrtsB[i] = sqrt(i);
    }
    for (i=wta=wtb=dot=0; i<64; i++,x>>=1,y>>=1)
    {
        if ( (x & 1) != 0 )
        {
            wta++;
            if ( (y & 1) != 0 )
                wtb++, dot++;
        }
        else if ( (y & 1) != 0 )
            wtb++;
    }
    if ( wta != 0 && wtb != 0 )
    {
        static double errsum,errcount;
        double cosA,cosB;
        errcount += 1.;
        cosA = ((double)dot / (sqrts[wta] * sqrts[wtb]));
        cosB = ((double)dot / (sqrtsB[wta * wtb]));
        errsum += fabs(cosA - cosB);
        if ( fabs(cosA - cosB) > SMALLVAL )
            printf("cosA %f vs %f [%.20f] ave error [%.20f]\n",cosA,cosB,cosA-cosB,errsum/errcount);
        return(cosB);
    }
    return(0.);
}

#define NUM_BLOOMPRIMES 8
#define MAXHOPS 6
#define MAXTESTPEERS 5
#define NETWORKSIZE 10000
long numxmit,foundcount;
uint64_t currentsearch;

uint64_t bloomprimes[NUM_BLOOMPRIMES] = { 79559, 79631, 79691, 79697, 79811, 79841, 79901, 79997 };
struct bloombits { uint8_t hashbits[NUM_BLOOMPRIMES][79997/8 + 1],pad[sizeof(uint64_t)]; };
struct smallworldnode { struct smallworldnode *peers[MAXTESTPEERS]; uint64_t nxt64bits,nodei; struct bloombits bloom; };

void set_bloombits(struct bloombits *bloom,uint64_t nxt64bits)
{
    int32_t i;
    for (i=0; i<NUM_BLOOMPRIMES; i++)
        SETBIT(bloom->hashbits[i],(nxt64bits % bloomprimes[i]));
}

int32_t in_bloombits(struct bloombits *bloom,uint64_t nxt64bits)
{
    int32_t i;
    for (i=0; i<NUM_BLOOMPRIMES; i++)
    {
        if ( GETBIT(bloom->hashbits[i],(nxt64bits % bloomprimes[i])) == 0 )
        {
            //printf("%llu not in bloombits.%llu\n",(long long)nxt64bits,(long long)ref64bits);
            return(0);
        }
    }
    //printf("%llu in bloombits.%llu\n",(long long)nxt64bits,(long long)ref64bits);
    return(1);
}

void merge_bloombits(struct bloombits *_dest,struct bloombits *_src)
{
    int32_t i,n = (int32_t)(sizeof(*_dest) / sizeof(uint64_t));
    uint64_t *dest,*src;
    dest = (uint64_t *)&_dest->hashbits[0][0];
    src = (uint64_t *)&_src->hashbits[0][0];
    for (i=0; i<n; i++)
        dest[i] |= src[i];
}

int32_t simroute(int32_t numhops,struct smallworldnode *depthblooms[],int32_t maxhops,int32_t n,struct smallworldnode *node,uint64_t dest)
{
    int32_t i,j,hops;
    struct smallworldnode *peer,*bestpeer;
    double cosval,maxmetric;
    numxmit++;
    //printf("numhops.%d maxhops.%d simroute.%llu from %llu\n",numhops,maxhops,(long long)dest,(long long)node->nxt64bits);
    peer = &depthblooms[maxhops-1][node->nodei];
    if ( 0 && in_bloombits(&peer->bloom,dest) == 0 )
    {
        printf("simroute.%llu called without any bloombits.%llu\n",(long long)dest,(long long)node->nxt64bits);
        return(-1);
    }
    numhops++;
    if ( maxhops == 0 )
    {
        for (i=0; i<MAXTESTPEERS; i++)
        {
            //printf("%llu ",(long long)node->peers[i]->nxt64bits);
            if ( node->peers[i]->nxt64bits == dest )
            {
                if ( currentsearch == dest )
                {
                    currentsearch = 0;
                    foundcount++;
                }
                //fprintf(stderr,"%d ",numhops);
                //printf("FOUND %llu\n",(long long)dest);
                return(numhops);
            }
        }
        printf("no match against %llu\n",(long long)dest);
        return(-1);
    }
    if ( node->nxt64bits == dest )
    {
        if ( currentsearch == dest )
        {
            currentsearch = 0;
            foundcount++;
        }
        fprintf(stderr,"%d ",numhops);
        return(numhops);
    }
    bestpeer = 0;
    for (j=0; j<maxhops; j++)
    {
        bestpeer = 0;
        maxmetric = 0.;
        for (i=0; i<MAXTESTPEERS; i++)
        {
            peer = &depthblooms[j][node->peers[i]->nodei];
            if ( peer != 0 )
            {
                if ( in_bloombits(&peer->bloom,dest) != 0 )
                {
                    cosval = cos64bits(peer->nxt64bits,dest);
                    if ( cosval > maxmetric )
                    {
                        maxmetric = cosval;
                        bestpeer = &depthblooms[MAXHOPS-1][node->peers[i]->nodei];
                    }
                }
            }
        }
        if ( bestpeer != 0 )
        {
            maxhops = j+1;
            break;
        }
    }
    if ( bestpeer != 0 )
    {
        hops = simroute(numhops,depthblooms,maxhops-1,n,bestpeer,dest);
        if ( hops > 0 )
            return(hops);
        else printf("got illegal numhops.%d maxhops.%d\n",numhops,maxhops);
    }
    return(-1);
}

void sim()
{
    void randombytes(uint8_t *x,uint64_t xlen);
    int32_t hist[100],i,j,k,n,skip,routefails,searches,nonz,iter,ind,hops,maxhops,sumhops,numpeers = MAXTESTPEERS;
    struct smallworldnode *network,*tmpnetworks[MAXHOPS],*node;
    struct bloombits allbits;
    memset(hist,0,sizeof(hist));
    n = NETWORKSIZE;
    network = calloc(n,sizeof(*network));
    for (i=0; i<MAXHOPS-1; i++)
        tmpnetworks[i] = calloc(n,sizeof(*network));
    printf("network allocated\n");
    for (i=0; i<n; i++)
        randombytes((uint8_t *)&network[i].nxt64bits,sizeof(network[i].nxt64bits));
    printf("nodes set\n");
    memset(&allbits,0,sizeof(allbits));
    for (i=0; i<n; i++)
    {
        network[i].nodei = i;
        for (j=0; j<numpeers; j++)
        {
            while ( 1 )
            {
                ind = (rand() % n);
                if ( ind == i )
                    continue;
                if ( j > 0 )
                {
                    for (k=0; k<j; k++)
                        if ( network[i].peers[k]->nodei == ind )
                            break;
                } else k = 0;
                if ( k == j )
                    break;
            }
            network[i].peers[j] = &network[ind];
            //printf("nodei.%d set bloom.%llu <- %llu nodej.%d\n",i,(long long)network[i].nxt64bits,(long long)network[ind].nxt64bits,ind);
            set_bloombits(&network[i].bloom,network[ind].nxt64bits);
            set_bloombits(&allbits,network[ind].nxt64bits);
        }
        if ( 0 && i == 0 )
        {
            for (j=nonz=0; j<n; j++)
                if ( in_bloombits(&network[i].bloom,network[j].nxt64bits) != 0 )
                    printf("%llu ",network[j].nxt64bits), nonz++;
            printf("in bloom for node.%d %llu nonz.%d\n",i,(long long)network[i].nxt64bits,nonz);
        }
    }
    printf("bloombits set\n");
    for (iter=1; iter<MAXHOPS; iter++)
    {
        memcpy(tmpnetworks[iter-1],network,sizeof(*network) * n);
        for (i=0; i<n; i++)
        {
            node = &network[i];
            for (j=0; j<numpeers; j++)
                merge_bloombits(&node->bloom,&tmpnetworks[iter-1][node->peers[j]->nodei].bloom);
            if ( 0 && i == 0 )
            {
                for (j=nonz=0; j<n; j++)
                    if ( in_bloombits(&network[i].bloom,network[j].nxt64bits) != 0 )
                        printf("%llu ",network[j].nxt64bits), nonz++;
                printf("iter.%d in bloom for node.%d %llu nonz.%d\n",iter,i,(long long)network[i].nxt64bits,nonz);
            }
            if ( (i % 100) == 0 )
                fprintf(stderr,".");
        }
    }
    tmpnetworks[MAXHOPS-1] = network;
    printf("bloombits merged\n");
    for (i=sumhops=maxhops=skip=searches=0; i<n; i++)
    {
        currentsearch = network[rand() % n].nxt64bits;
        node = &network[(rand() % n)];
        if ( in_bloombits(&node->bloom,currentsearch) != 0 )
        {
            //printf("search for %llu from %llu\n",(long long)currentsearch,(long long)node->nxt64bits);
            searches++;
            if ( (hops= simroute(0,tmpnetworks,MAXHOPS,n,node,currentsearch)) >= 0 )
            {
                sumhops += hops;
                if ( hops > maxhops )
                    maxhops = hops;
            } else routefails++;
            //break;
        } else skip++;
    }
    printf("avehops %.1f maxhops.%d | numxmit.%ld ave %.1f | skip.%d fails.%d foundcount.%ld %.2f%%\n",(double)sumhops/foundcount,maxhops,numxmit,(double)numxmit/searches,skip,routefails,foundcount,100.*(double)foundcount/searches);
    getchar();
    uint64_t x,y;
    double cosval,sum,total,minval = 0,maxval = 0.;
    for (total=i=0; i<n; i++)
    {
        x = network[i].nxt64bits;
        for (sum=j=0; j<n; j++)
        {
            if ( i == j )
                continue;
            y = network[j].nxt64bits;
            cosval = cos64bits(x,y);
            sum += cosval;
            total += cosval;
            hist[(int)(cosval * 100)]++;
            if ( cosval > maxval )
            {
                maxval = cosval;
                fprintf(stderr,"[%.6f> ",cosval);
            }
            if ( minval == 0. || cosval < minval )
            {
                minval = cosval;
                fprintf(stderr,"<%.6f] ",cosval);
            }
        }
        fprintf(stderr,"%.3f ",sum/n);
    }
    printf("\nn.%d numpeers.%d minval %.8f maxval %.8f total %f\n",n,numpeers,minval,maxval,total/(n*n));
    for (i=0; i<100; i++)
    {
        printf("%.6f ",(double)hist[i]/(n*n));
        if ( (i % 10) == 9 )
            printf("i.%d\n",i);
    }
    printf("histogram\n");
}

int main(int argc,const char *argv[])
{
    FILE *fp;
    cJSON *json = 0;
    int32_t retval = -666;
    char ipaddr[64],*oldport,*newport,portstr[64],*retstr;
#ifdef __APPLE__
    sim(), getchar();
#else
    if ( 1 && argc > 1 && strcmp(argv[1],"genfiles") == 0 )
#endif
    {
        void *process_ramchains(void *argcoinstr);
        void *args[4];
        retval = SuperNET_start("SuperNET.conf","127.0.0.1");
        memset(args,0,sizeof(args));
#ifdef __APPLE__
        args[0] = "BTCD";
        *(long *)&args[1] = 0;
        *(long *)&args[2] = 2;
#else
        if ( argc > 2 )
             args[0] = (char *)argv[2];
        else args[0] = 0;
        if ( argc > 4 )
        {
            *(long *)&args[1] = atol(argv[3]);
            *(long *)&args[2] = atol(argv[4]);
        }
#endif
        if ( IS_LIBTEST == 7 )
        {
            printf(">>>>>>>>>>>>> process coinblocks.(%s)\n",(char *)args[0]);
            process_ramchains(args);
            printf("finished genfiles.(%s)\n",(char *)(args[0]!=0?args[0]:""));
            getchar();
        }
    }
#ifdef fortesting
    if ( 0 )
    {
        void huff_iteminit(struct huffitem *hip,void *ptr,int32_t size,int32_t isptr,int32_t ishex);
        char *p,buff[1024];//,*str = "this is an example for huffman encoding";
        int i,c,n,numinds = 256;
        int probs[256];
        //struct huffcode *huff;
        struct huffitem *items = calloc(numinds,sizeof(*items));
        int testhuffcode(char *str,struct huffitem *freqs,int32_t numinds);
        for (i=0; i<numinds; i++)
            huff_iteminit(&items[i],&i,1,0,0);
        while ( 1 )
        {
            for (i=0; i<256; i++)
                probs[i] = ((rand()>>8) % 1000);
            for (i=n=0; i<128; i++)
            {
                c = (rand() >> 8) & 0xff;
                while ( c > 0 && ((rand()>>8) % 1000) > probs[c] )
                {
                    buff[n++] = (c % 64) + ' ';
                    if ( n >= sizeof(buff)-1 )
                        break;
                }
            }
            buff[n] = 0;
            for (i=0; i<numinds; i++)
                items[i].freq = 0;
            p = buff;
            while ( *p != '\0' )
                items[*p++].freq++;
            testhuffcode(0,items,numinds);
            fprintf(stderr,"*");
        }
        //getchar();
    }
#endif
    IS_LIBTEST = 1;
    if ( argc > 1 && argv[1] != 0 )
    {
        char *init_MGWconf(char *JSON_or_fname,char *myipaddr);
        //printf("ARGV1.(%s)\n",argv[1]);
        if ( (argv[1][0] == '{' || argv[1][0] == '[') )
        {
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
    //sprintf(portstr,"%d",SUPERNET_PORT+1);
    //oldport = newport = portstr;
    //if ( upnpredirect(oldport,newport,"UDP","SuperNET_http") == 0 )
    //    printf("TEST ERROR: failed redirect (%s) to (%s)\n",oldport,newport);
    printf("saving retval.%x (%d usessl.%d) UPNP.%d MULTIPORT.%d\n",retval,retval>>1,retval&1,UPNP,MULTIPORT);
    if ( (fp= fopen("horrible.hack","wb+")) != 0 )
    {
        fwrite(&retval,1,sizeof(retval),fp);
        fclose(fp);
    }
    if ( Debuglevel > 0 )
        system("git log | head -n 1");
    if ( retval >= 0 && ENABLE_GUIPOLL != 0 )
    {
        GUIpoll_loop(ipaddr);
        //if ( portable_thread_create((void *)GUIpoll_loop,ipaddr) == 0 )
        //    printf("ERROR hist process_hashtablequeues\n");
    }
    while ( 1 )
    {
        //extern void do_bridge_things();
        //do_bridge_things();
        sleep(20);
    }
    return(0);
}


// stubs
int32_t SuperNET_broadcast(char *msg,int32_t duration) { return(0); }
int32_t SuperNET_narrowcast(char *destip,unsigned char *msg,int32_t len) { return(0); }

#ifdef chanc3r
#endif
