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
#include <arpa/inet.h>
#include <sys/time.h>

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

char *SuperNET_url()
{
    static char urls[2][64];
    sprintf(urls[0],"http://127.0.0.1:%d",SUPERNET_PORT+1);
    sprintf(urls[1],"https://127.0.0.1:%d",SUPERNET_PORT);
    return(urls[USESSL]);
}

cJSON *SuperAPI(char *cmd,char *field0,char *arg0,char *field1,char *arg1)
{
    cJSON *json;
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
    void unstringify(char *);
    char params[4096],buf[1024],buf2[1024],ipaddr[64],args[8192],*retstr;
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
    int32_t init_hexbytes_noT(char *hexbytes,unsigned char *message,long len);
    bits256 issue_getpubkey(int32_t *haspubkeyp,char *acct);
    char *issue_MGWstatus(int32_t mask,char *coinstr,char *userNXTaddr,char *userpubkey,char *email,int32_t rescan,int32_t actionflag);
    struct multisig_addr *decode_msigjson(char *NXTaddr,cJSON *obj,char *sender);
    int32_t send_email(char *email,char *destNXTaddr,char *pubkeystr,char *msg);
    void issue_genmultisig(char *coinstr,char *userNXTaddr,char *userpubkey,char *email,int32_t buyNXT);
    char txidstr[1024],senderipaddr[1024],cmd[2048],coin[2048],userpubkey[2048],NXTacct[2048],userNXTaddr[2048],email[2048],convertNXT[2048],retbuf[1024],buf2[1024],coinstr[1024],cmdstr[512],*retstr = 0,*waitfor = 0,errstr[2048],*str;
    bits256 pubkeybits;
    unsigned char hash[256>>3],mypublic[256>>3];
    uint16_t port;
    uint64_t nxt64bits,checkbits,deposit_pending = 0;
    int32_t i,n,haspubkey,iter,gatewayid,actionflag = 0,rescan = 1;
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
        sprintf(cmdstr,"http://%s/MGW/msig/%s",Server_names[i],userNXTaddr);
        array = cJSON_GetObjectItem(MGWconf,"active");
        if ( array != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
        {
            for (i=0; i<100; i++) // flush queue
                GUIpoll(txidstr,senderipaddr,&port);
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
                sleep(1);
            }
        }
    }
    else if ( strcmp(cmd,"status") == 0 )
    {
        waitfor = "MGWresponse";
        sprintf(cmdstr,"http://%s/MGW/status/%s",Server_names[i],userNXTaddr);
        //printf("cmdstr.(%s) waitfor.(%s)\n",cmdstr,waitfor);
        retstr = issue_MGWstatus((1<<NUM_GATEWAYS)-1,coin,userNXTaddr,userpubkey,0,rescan,actionflag);
        if ( retstr != 0 )
            free(retstr), retstr = 0;
    }
    else return(clonestr("{\"error\":\"only newbie command is supported now\"}"));
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
    if ( deposit_pending != 0 )
    {
        actionflag = 1;
        rescan = 0;
        retstr = issue_MGWstatus(1<<NUM_GATEWAYS,coin,0,0,0,rescan,actionflag);
        if ( retstr != 0 )
            free(retstr), retstr = 0;
    }
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
    char fname[1024],*buf,*retstr = 0;
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
    int32_t gatewayid;
    char fname[1024],cmd[1024],*name = args->name;
    if ( strncmp(name,"MGW",3) == 0 && name[3] >= '0' && name[3] <= '2' )
    {
        gatewayid = (name[3] - '0');
        name += 5;
        sprintf(fname,"%s/gateway%d/%s",MGWROOT,gatewayid,name);
        if ( (fp= fopen(fname,"wb")) != 0 )
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
    uint16_t port;
    char txidstr[1024],senderipaddr[1024],*retstr;
    while ( 1 )
    {
        sleep(1);
        if ( (retstr= GUIpoll(txidstr,senderipaddr,&port)) != 0 )
            free(retstr);
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

struct hashtable *hashtable_create(char *name,int64_t hashsize,long structsize,long keyoffset,long keysize,long modifiedoffset);
void *add_hashtable(int32_t *createdflagp,struct hashtable **hp_ptr,char *key);
void **hashtable_gather_modified(int64_t *changedp,struct hashtable *hp,int32_t forceflag);
struct huffcode *huff_create(const struct huffitem **items,int32_t numinds,int32_t frequi);
void huff_iteminit(struct huffitem *hip,void *ptr,long size,long wt,int32_t ishexstr);
void huff_free(struct huffcode *huff);
void huff_clearstats(struct huffcode *huff);

uint32_t BITSTREAM_GROUPSIZE(char *coinstr)
{
    if ( strcmp(coinstr,"BTC") == 0 )
        return(1000);
    else return(10000);
}

int32_t calc_frequi(char *coinstr,uint32_t blocknum)
{
    int32_t slice,incr;
    incr = 1000;//BITSTREAM_GROUPSIZE(coinstr);
    slice = (incr / HUFF_NUMFREQS);
    return((blocknum / slice) % HUFF_NUMFREQS);
}

struct coinaddr
{
    uint32_t ind,numentries,allocsize;
    struct huffitem item;
    char addr[35];
    uint8_t pubkey[33];
    struct address_entry *entries;
};

struct valueinfo
{
    uint32_t ind;
    uint64_t value;
    struct huffitem item;
    char valuestr[24];
};

struct scriptinfo
{
    uint32_t ind,mode;
    struct huffitem item;
    uint8_t script[];
};

struct txinfo
{
    uint32_t ind,allocsize,numentries;
    uint16_t numvouts,numvins;
    struct huffitem item;
    uint8_t txidstr[128];
    struct address_entry entries[];
};

void clear_compressionvars(struct compressionvars *V,int32_t clearstats,int32_t frequi)
{
    int32_t i;
    //int64_t changed;
    struct scriptinfo *sp;
    struct txinfo *tp;
    struct coinaddr *ap;
    struct valueinfo *valp;
    V->maxitems = 0;
    memset(V->rawdata,0,sizeof(*V->rawdata));
    if ( clearstats != 0 )
    {
        for (i=0; i<V->values->hashsize; i++)
        {
            if ( (valp= V->values->hashtable[i]) != 0 )
                valp->item.freq[frequi] = 0;
        }
        for (i=0; i<V->addrs->hashsize; i++)
        {
            if ( (ap= V->addrs->hashtable[i]) != 0 )
                ap->item.freq[frequi] = 0;
        }
        for (i=0; i<V->txids->hashsize; i++)
        {
            if ( (tp= V->txids->hashtable[i]) != 0 )
                tp->item.freq[frequi] = 0;
        }
        for (i=0; i<V->scripts->hashsize; i++)
        {
            if ( (sp= V->addrs->hashtable[i]) != 0 )
                sp->item.freq[frequi] = 0;
        }
    }
    /*if ( (addrs= (struct coinaddr **)hashtable_gather_modified(&changed,V->addrs,1)) != 0 )
    {
        for (i=0; i<changed; i++)
            if ( addrs[i] != 0 )
                memset(&addrs[i]->item,0,sizeof(addrs[i]->item));
        free(addrs);
    }
    if ( (txids= (struct txinfo **)hashtable_gather_modified(&changed,V->txids,1)) != 0 )
    {
        for (i=0; i<changed; i++)
            if ( txids[i] != 0 )
                memset(&txids[i]->item,0,sizeof(txids[i]->item));
        free(txids);
    }
    if ( (scripts= (struct scriptinfo **)hashtable_gather_modified(&changed,V->scripts,1)) != 0 )
    {
        for (i=0; i<changed; i++)
            if ( scripts[i] != 0 )
                memset(&scripts[i]->item,0,sizeof(scripts[i]->item));
        free(scripts);
    }*/
}

void update_huffitem(struct huffitem *hip,uint32_t huffind,int32_t wt)
{
    int32_t i;
    if ( hip->size == 0 )
        huff_iteminit(hip,&huffind,sizeof(huffind),wt,0);
    for (i=0; i<(int32_t)(sizeof(hip->freq)/sizeof(*hip->freq)); i++)
        hip->freq[i]++;
}

/*void incr_valueitem(struct huffitem *valueitems,int32_t maxitems,uint64_t value,int32_t frequi)
{
    int32_t i;
    for (i=0; i<maxitems; i++)
    {
        if ( valueitems[i].U.bits.txid == value )
        {
            update_huffitem(&valueitems[i],(i << 3) | 0,64);
            return;
        }
        if ( valueitems[i].U.bits.txid == 0 )
        {
            update_huffitem(&valueitems[i],(i << 3) | 0,64);
            valueitems[i].U.bits.txid = value;
            return;
        }
    }
    printf("FATAL: incr_valueitem valueitems full?\n");
    exit(-1);
}*/

void emit_compressed_block(struct compressionvars *V,char *coinstr,uint32_t blocknum,int32_t frequi)
{
    uint8_t *ptr = V->rawbits;
    long incr,size = 0;
    int32_t i,numvins,numvouts;
    uint32_t numhuffinds = 0;
    uint16_t vout;
    struct scriptinfo *sp;
    struct txinfo *tp;
    struct coinaddr *ap;
    struct valueinfo *valp;
    struct huffcode *huff;
    struct huffitem **items,*blockitems,*voutitems,*valueitems,*txinditems;
    //struct rawblock_voutdata *vp;//{ uint32_t tp_ind,vout,addr_ind,sp_ind; uint64_t value; };
    struct address_entry *bp;//{ uint64_t blocknum:32,txind:15,vinflag:1,v:14,spent:1,isinternal:1; };
    numvins = V->rawdata->numvins;
    numvouts = V->rawdata->numvouts;
    incr = sizeof(blocknum), memcpy(&ptr[size],&blocknum,incr), size += incr;
    incr = sizeof(numvins), memcpy(&ptr[size],&numvins,incr), size += incr;
    incr = sizeof(numvouts), memcpy(&ptr[size],&numvouts,incr), size += incr;
    incr = sizeof(V->rawdata->vins[0]) * numvins, memcpy(&ptr[size],&V->rawdata->vins[0],incr), size += incr;
    incr = sizeof(V->rawdata->vouts[0]) * numvouts, memcpy(&ptr[size],&V->rawdata->vouts[0],incr), size += incr;
    fwrite(V->rawbits,1,size,V->fp), fflush(V->fp);
   // printf("size.%ld numvins.%d numvouts.%d\n",size,numvins,numvouts);
//return;
    blockitems = calloc(blocknum+1,sizeof(*blockitems));
    voutitems = calloc(1<<16,sizeof(*voutitems));
    txinditems = calloc(1<<16,sizeof(*txinditems));
    valueitems = calloc(V->maxitems,sizeof(*valueitems));
    items = calloc(1000000,sizeof(*items));
    for (i=0; i<numvins; i++)
    {
        bp = &V->rawdata->vins[i];
        update_huffitem(&blockitems[bp->blocknum],(bp->blocknum << 3) | 1,32);
        update_huffitem(&txinditems[bp->txind],(bp->txind << 3) | 2,15);
        update_huffitem(&voutitems[bp->v],(bp->v << 3) | 3,15);
        
        /*if ( blockitems[bp->blocknum].freq[frequi]++ == 0 )
            huffind = ((bp->blocknum << 3) | 1), huff_iteminit(&blockitems[bp->blocknum],&huffind,sizeof(huffind),32,0);
        if ( txinditems[bp->txind].freq++ == 0 )
            huffind = ((bp->txind << 3) | 2), huff_iteminit(&txinditems[bp->txind],&huffind,sizeof(huffind),15,0);
        if ( voutitems[bp->v].freq++ == 0 )
            huffind = ((bp->v << 3) | 3), huff_iteminit(&voutitems[bp->v],&huffind,sizeof(huffind),15,0);*/
    }
    for (i=0; i<numvouts; i++)
    {
        vout = V->rawdata->vouts[i].vout;
        update_huffitem(&voutitems[vout],(vout << 3) | 4,15);
        //if ( voutitems[vout].freq++ == 0 )
        //    huffind = ((vout << 3) | 4), huff_iteminit(&voutitems[vout],&huffind,sizeof(huffind),15,0);
        //incr_valueitem(valueitems,V->maxitems,V->rawdata->vouts[i].value,frequi);
    }
    for (i=0; i<=blocknum; i++)
    {
        if ( blockitems[i].freq[frequi] != 0 )
            items[numhuffinds++] = &blockitems[i];
    }
    /*for (i=0; i<V->maxitems; i++)
    {
        if ( valueitems[i].freq[frequi] != 0 )
            items[numhuffinds++] = &valueitems[i];
    }*/
    for (i=0; i<(1<<16); i++)
    {
        if ( voutitems[i].freq[frequi] != 0 )
            items[numhuffinds++] = &voutitems[i];
        if ( txinditems[i].freq[frequi] != 0 )
            items[numhuffinds++] = &txinditems[i];
    }
    for (i=0; i<V->addrs->hashsize; i++)
    {
        if ( (ap= V->addrs->hashtable[i]) != 0 && ap->item.freq[frequi] != 0 )
            items[numhuffinds++] = &ap->item;
    }
    for (i=0; i<V->values->hashsize; i++)
    {
        if ( (valp= V->values->hashtable[i]) != 0 && valp->item.freq[frequi] != 0 )
            items[numhuffinds++] = &valp->item;
    }
    for (i=0; i<V->txids->hashsize; i++)
    {
        if ( (tp= V->txids->hashtable[i]) != 0 && tp->item.freq[frequi] != 0 )
            items[numhuffinds++] = &tp->item;
    }
    for (i=0; i<V->scripts->hashsize; i++)
    {
        if ( (sp= V->addrs->hashtable[i]) != 0 && sp->item.freq[frequi] != 0 )
            items[numhuffinds++] = &sp->item;
    }

     //emit_varbits(V->hp,V->numentries);
    
    if ( 0 && numhuffinds > 0 && (huff= huff_create((const struct huffitem **)items,numhuffinds,frequi)) != 0 )
    {
        /*for (i=num=0; i<len; i++)
         {
         huffind;
         num += hwrite(huff->items[c].codebits,huff->items[c].numbits,hp);
         }*/
        printf("numhuffinds.%d size.%ld starting bits.%.0f -> %.0f [%.3f]\n",numhuffinds,size,huff->totalbytes/8,huff->totalbits/8,(double)huff->totalbytes/(huff->totalbits+1));
        //hrewind(hp);
        //dlen = huff_decode(huff,output,(int32_t)sizeof(output),hp);
        //output[num] = 0;

        hflush(V->fp,V->hp);
        hclear(V->hp);
        huff_clearstats(huff);
        huff_free(huff);
    } //else printf("error from huff_create %s.%u\n",coinstr,blocknum);
    free(blockitems); free(voutitems); free(txinditems); free(valueitems); free(items);
}

void update_coinaddr_entries(struct coinaddr *addrp,struct address_entry *entry)
{
    addrp->allocsize = sizeof(*addrp) + ((addrp->numentries+1) * sizeof(*entry));
    addrp->entries = realloc(addrp->entries,addrp->allocsize);
    addrp->entries[addrp->numentries++] = *entry;
}

void add_entry_to_tx(struct txinfo *tp,struct address_entry *entry)
{
    tp->allocsize = sizeof(*tp) + ((tp->numentries+1) * sizeof(*entry));
    tp = realloc(tp,tp->allocsize);
    tp->entries[tp->numentries++] = *entry;
}

int32_t _calc_bitsize(uint32_t x)
{
    uint32_t mask = (1 << 31);
    int32_t i;
    if ( x == 0 )
        return(0);
    for (i=31; i>=0; i--)
    {
        if ( (mask & x) != 0 )
            return(i);
    }
    return(-1);
}

int32_t emit_varbits(HUFF *hp,uint8_t val)
{
    int i,valsize = _calc_bitsize(val);
    for (i=0; i<3; i++)
        hputbit(hp,(valsize & (1<<i)) != 0);
    for (i=0; i<valsize; i++)
        hputbit(hp,(val & (1<<i)) != 0);
    return(valsize + 3);
}

int32_t emit_valuebits(HUFF *hp,uint8_t value)
{
    int32_t i,num,valsize,lsb = 0;
    uint64_t mask;
    mask = (1L << 63);
    for (i=63; i>=0; i--,mask>>=1)
        if ( (value & mask) != 0 )
            break;
    mask = 1;
    for (lsb=0; lsb<i; lsb++,mask<<=1)
        if ( (value & mask) != 0 )
            break;
    value >>= lsb;
    valsize = (i - lsb);
    num = emit_varbits(hp,lsb);
    num += emit_varbits(hp,valsize);
    mask = 1;
    for (i=0; i<valsize; i++,mask<<=1)
        hputbit(hp,(value & mask) != 0);
    //printf("%d ",num+valsize);
    return(num + valsize);
}

int32_t choose_varbits(HUFF *hp,uint32_t val,int32_t diff)
{
    int valsize,diffsize,i,num = 0;
    valsize = _calc_bitsize(val);
    diffsize = _calc_bitsize(diff < 0 ? -diff : diff);
    if ( valsize < diffsize )
    {
        hputbit(hp,0);
        hputbit(hp,1);
        num = 2 + valsize + emit_varbits(hp,valsize);
        for (i=0; i<valsize; i++)
            hputbit(hp,(val & (1<<i)) != 0);
    }
    else
    {
        num = 1;
        if ( diff < 0 )
        {
            hputbit(hp,0);
            hputbit(hp,0);
            num++;
        }
        else hputbit(hp,1);
        num += emit_varbits(hp,diffsize) + diffsize;
        for (i=0; i<diffsize; i++)
            hputbit(hp,(diff & (1<<i)) != 0);
    }
    return(num);
}

long emit_varint(FILE *fp,uint64_t x)
{
    uint8_t b; uint16_t s; uint32_t i;
    long retval = -1;
    if ( x < 0xfd )
        b = x, retval = fwrite(&b,1,sizeof(b),fp);
    else
    {
        if ( x <= 0xffff )
        {
            fputc(0xfd,fp);
            s = (uint16_t)x, retval = fwrite(&s,1,sizeof(s),fp);
        }
        else if ( x <= 0xffffffffL )
        {
            fputc(0xfe,fp);
            i = (uint32_t)x, retval = fwrite(&i,1,sizeof(i),fp);
        }
        else
        {
            fputc(0xff,fp);
            retval = fwrite(&x,1,sizeof(x),fp);
        }
    }
    return(retval);
}

int32_t load_varint(uint64_t *valp,FILE *fp)
{
    uint16_t s; uint32_t i; int32_t c; int32_t retval = 1;
    *valp = 0;
    if ( (c= fgetc(fp)) == EOF )
        return(0);
    c &= 0xff;
    switch ( c )
    {
        case 0xfd: retval = (sizeof(s) + 1) * (fread(&s,1,sizeof(s),fp) == sizeof(s)), *valp = s; break;
        case 0xfe: retval = (sizeof(i) + 1) * (fread(&i,1,sizeof(i),fp) == sizeof(i)), *valp = i; break;
        case 0xff: retval = (sizeof(*valp) + 1) * (fread(valp,1,sizeof(*valp),fp) == sizeof(*valp)); break;
        default: *valp = c; break;
    }
    return(retval);
}

int32_t save_filestr(FILE *fp,char *str)
{
    long savepos,len = strlen(str);
    savepos = ftell(fp);
    if ( emit_varint(fp,len) > 0 )
    {
        fwrite(str,1,len,fp);
        fflush(fp);
        return(0);
    }
    else fseek(fp,savepos,SEEK_SET);
    return(-1);
}

int32_t load_cfilestr(int32_t *lenp,char *str,FILE *fp)
{
    long savepos;
    uint64_t len;
    *lenp = 0;
    savepos = ftell(fp);
    if ( load_varint(&len,fp) > 0 )
    {
        if ( fread(str,1,len,fp) != len )
        {
            printf("load_filestr: error reading len.%lld at %ld, truncate to %ld\n",(long long)len,ftell(fp),savepos);
            fseek(fp,savepos,SEEK_SET);
            return(-1);
        }
        else
        {
            str[len] = 0;
            *lenp = (int32_t)len;
            return(0);
        }
    }
    fseek(fp,savepos,SEEK_SET);
    return(-1);
}

long emit_blockcheck(FILE *fp,uint64_t blocknum,int32_t restorepos)
{
    long retval,fpos;
    uint64_t blockcheck;
    fpos = ftell(fp);
    blockcheck = (~blocknum << 32) | blocknum;
    retval = fwrite(&blockcheck,1,sizeof(blockcheck),fp);
    if ( restorepos != 0 )
        fseek(fp,fpos,SEEK_SET);
    fflush(fp);
    return(retval);
}

uint32_t load_blockcheck(FILE *fp,int32_t depth,char *coinstr)
{
    uint64_t blockcheck;
    uint32_t blocknum = 0;
    if ( ftell(fp) > sizeof(uint64_t) )
    {
        fseek(fp,-sizeof(uint64_t) * depth,SEEK_END);
        if ( fread(&blockcheck,1,sizeof(uint64_t),fp) != sizeof(blockcheck) || (uint32_t)(blockcheck >> 32) != ~(uint32_t)blockcheck )
            blocknum = 0;
        else
        {
            blocknum = (uint32_t)blockcheck;
            printf("found valid marker.%s blocknum %llx\n",coinstr,(long long)blockcheck);
        }
        fseek(fp,-sizeof(uint64_t) * depth,SEEK_END);
    }
    return(blocknum);
}

FILE *_open_varsfile(uint32_t *blocknump,char *fname,char *coinstr)
{
    FILE *fp;
    if ( (fp = fopen(fname,"rb+")) == 0 )
    {
        fp = fopen(fname,"wb");
        printf("created %s\n",fname);
        *blocknump = 0;
    }
    else
    {
        *blocknump = load_blockcheck(fp,1,coinstr);
        printf("opened %s blocknum.%d\n",fname,*blocknump);
    }
    return(fp);
}

void set_commpressionvars_fname(char *fname,char *coinstr,char *typestr,int32_t subgroup)
{
    if ( subgroup < 0 )
        sprintf(fname,"address/%s/%s.%s",coinstr,coinstr,typestr);
    else sprintf(fname,"address/%s/%s/%s.%d",coinstr,typestr,coinstr,subgroup);
}

FILE *open_commpresionvars_file(uint32_t *checkpoints,struct hashtable *table,uint32_t *countp,uint32_t *blocknump,char *coinstr,char *typestr)
{
    char fname[1024],str[8192];
    uint32_t tmpblocknum,*ptr,groupsize = BITSTREAM_GROUPSIZE(coinstr);
    FILE *fp,*tmpfp = 0;
    int32_t i,len,createdflag,count = 0;
    *blocknump = -1;
    if ( checkpoints != 0 )
        for (i=0; i<3; i++)
            checkpoints[i] = -1;
    *countp = 0;
    if ( 1 )
    {
        set_commpressionvars_fname(fname,coinstr,typestr,-1 + (strcmp(typestr,"bitstream") == 0));
        fp = fopen(fname,"wb");
        printf("created (%s) %p\n",fname,fp);
        return(fp);
    }
    if ( strcmp(typestr,"bitstream") == 0 )
    {
        fp = 0;
        for (i=0; i<10000; i++)
        {
            set_commpressionvars_fname(fname,coinstr,typestr,i*groupsize);
            if ( (tmpfp= _open_varsfile(&tmpblocknum,fname,coinstr)) == 0 )
            {
                printf("error opening.(%s) tmpblocknum.%u\n",fname,tmpblocknum);
                if ( fp != 0 )
                    fclose(fp), fp = 0;
                *blocknump = -1;
                break;
            }
            else if ( tmpblocknum == 0 )
            {
                *blocknump = (i * groupsize) - 1;
                printf("opening.(%s) has blocknum of 0, set to %d\n",fname,*blocknump);
                if ( fp != 0 )
                    fclose(fp);
                fp = tmpfp, tmpfp = 0;
                break;
            }
            else
            {
                *blocknump = tmpblocknum;
                printf("opening.(%s) has blocknum of %d\n",fname,*blocknump);
                if ( fp != 0 )
                    fclose(fp);
                fp = tmpfp, tmpfp = 0;
                if ( checkpoints != 0 )
                    for (i=0; i<3; i++)
                        checkpoints[i] = load_blockcheck(fp,i+2,coinstr);
                if ( tmpblocknum != (i*groupsize + groupsize-1) )
                {
                    printf("not expected %d, break\n",(i*groupsize + groupsize-1));
                    break;
                }
            }
        }
        if ( i < 10000 )
            count = i;
        else if ( tmpfp != 0 )
            fclose(tmpfp), tmpfp = 0;
    }
    else set_commpressionvars_fname(fname,coinstr,typestr,-1);
    if ( fp != 0 && table != 0 )
    {
        while ( load_cfilestr(&len,str,fp) > 0 )
        {
            ptr = add_hashtable(&createdflag,&table,str);
            if ( createdflag != 0 )
                *ptr = ++count;
            else printf("FATAL: redundant entry in (%s).%d [%s]?\n",fname,count,str), exit(-1);
        }
    }
    *countp = count;
    printf("(%s) count.%d blocknum.%d fp.%p\n",fname,count,*blocknump,fp);
    return(fp);
}

void compressionvars_add_txout(struct rawblockdata *rp,char *coinstr,uint32_t tp_ind,uint32_t vout,uint32_t addr_ind,uint64_t value,uint32_t sp_ind)
{
    struct rawblock_voutdata *vp;
    vp = &rp->vouts[rp->numvouts++];
    vp->tp_ind = tp_ind;
    vp->vout = vout;
    vp->addr_ind = addr_ind;
    vp->value = value;
    vp->sp_ind = sp_ind;
}

void compressionvars_add_txin(struct rawblockdata *rp,char *coinstr,struct address_entry *bp)
{
    rp->vins[rp->numvins++] = *bp;
}

uint32_t flush_compressionvars(struct compressionvars *V,char *coinstr,uint32_t prevblocknum,uint32_t newblocknum,int32_t frequi)
{
    char fname[1024];
    uint32_t tmpblocknum;
    if ( prevblocknum == 0xffffffff )
        return(0);
    emit_compressed_block(V,coinstr,prevblocknum,frequi);
    emit_blockcheck(V->fp,ftell(V->sfp),0);
    emit_blockcheck(V->fp,ftell(V->tfp),0);
    emit_blockcheck(V->fp,ftell(V->afp),0);
    emit_blockcheck(V->fp,prevblocknum,1); // overwrite with bits from next block

    emit_blockcheck(V->afp,prevblocknum,1);
    emit_blockcheck(V->tfp,prevblocknum,1);
    emit_blockcheck(V->sfp,prevblocknum,1);
    if ( V->disp != 0 )
    {
        sprintf(V->disp+strlen(V->disp),"-> max.%d %.1f %.1f\n%s F.%d NEWBLOCK.%u A%u T%u S%u |",V->maxitems,(double)(ftell(V->fp)+ftell(V->afp)+ftell(V->tfp)+ftell(V->sfp))/(prevblocknum+1),(double)ftell(V->fp)/(prevblocknum+1),coinstr,frequi,prevblocknum,V->addrind,V->txind,V->scriptind);
        printf("%s",V->disp);
        V->disp[0] = 0;
    }
    if ( 0 && (newblocknum % BITSTREAM_GROUPSIZE(coinstr)) == 0 )
    {
        fclose(V->fp);
        set_commpressionvars_fname(fname,coinstr,"bitstream",newblocknum);
        if ( (V->fp= _open_varsfile(&tmpblocknum,fname,coinstr)) == 0 )
        {
            printf("couldnt open (%s) at newblocknum.%d\n",fname,newblocknum);
            exit(-1);
        }
    }
    clear_compressionvars(V,(newblocknum % 100) == 0,frequi);
    V->prevblock = newblocknum;
    return(newblocknum);
}

void init_compressionvars(struct compressionvars *V,char *coinstr)
{
    struct coinaddr *addrp = 0;
    struct txinfo *tp = 0;
    struct scriptinfo *sp = 0;
    struct valueinfo *valp = 0;
    uint32_t blocknums[4],i,checkpoints[3];
    if ( V->addrs == 0 )
    {
        printf("init compression vars\n");
        V->buffer = calloc(1,1000000);
        V->disp = calloc(1,1000000);
        V->rawbits = calloc(1,1000000);
        V->rawdata = calloc(1,sizeof(*V->rawdata));
        V->hp = hopen(V->buffer,1000000);
        V->values = hashtable_create("values",1000000,sizeof(*valp),((long)&valp->valuestr[0] - (long)valp),sizeof(valp->valuestr),-1);
        V->addrs = hashtable_create("addrs",1000000,sizeof(*addrp),((long)&addrp->addr[0] - (long)addrp),sizeof(addrp->addr),-1);
        V->txids = hashtable_create("txids",1000000,sizeof(*tp),((long)&tp->txidstr[0] - (long)tp),sizeof(tp->txidstr),-1);
        //V->scripts = hashtable_create("scripts",100,sizeof(*sp),((long)&sp->script[0] - (long)sp),sizeof(sp->script),-1);
        V->scripts = hashtable_create("scripts",1000000,sizeof(*sp),sizeof(*sp),0,-1);
        V->fp = open_commpresionvars_file(checkpoints,0,&V->filecount,&blocknums[0],coinstr,"bitstream");
        V->afp = open_commpresionvars_file(0,V->addrs,&V->addrind,&blocknums[1],coinstr,"addrs");
        V->tfp = open_commpresionvars_file(0,V->txids,&V->txind,&blocknums[2],coinstr,"txids");
        V->sfp = open_commpresionvars_file(0,V->scripts,&V->scriptind,&blocknums[3],coinstr,"scripts");
        for (i=1; i<4; i++)
        {
            printf("blocknum.%u vs %u | checkpoint %u\n",blocknums[i],blocknums[0],checkpoints[i-1]);
            if ( blocknums[i] != blocknums[0] )
                break;
        }
        if ( i != 4 )
        {
            printf("mismatched blocknums in critical %s files. need to repair them\n",coinstr);
            fseek(V->afp,checkpoints[0],SEEK_SET);
            fseek(V->tfp,checkpoints[1],SEEK_SET);
            fseek(V->sfp,checkpoints[2],SEEK_SET);
        }
        V->prevblock = blocknums[0];
    }
}

void *update_compressionvars_table(FILE *fp,uint32_t *indp,struct hashtable *table,char *str)
{
    uint32_t *ptr;
    int32_t createdflag;
    ptr = add_hashtable(&createdflag,&table,str);
    if ( createdflag != 0 )
    {
        (*ptr) = ++(*indp);
        if ( fp != 0 && save_filestr(fp,str) < 0 )
            printf("save_filestr error for (%s)\n",str);
    }
    return(ptr);
}

int32_t calc_scriptmode(uint8_t pubkey[33],char *script,int32_t trimflag)
{
    char pubkeystr[256];
    int32_t len,mode = 0;
    len = (int32_t)strlen(script);
    if ( strncmp(script,"76a914",6) == 0 && strcmp(script+len-4,"88ac") == 0 )
    {
        strcpy(pubkeystr,script+6);
        pubkeystr[strlen(pubkeystr) - 4] = 0;
        if ( strlen(pubkeystr) < 66 )
        {
            decode_hex(pubkey,(int32_t)strlen(pubkeystr)/2,pubkeystr);
            //printf("set pubkey.(%s).%ld <- (%s)\n",pubkeystr,strlen(pubkeystr),script);
        }
        if ( trimflag != 0 )
        {
            script[len-4] = 0;
            script += 6;
        }
         mode = 's';
    }
    else if ( strcmp(script+len-2,"ac") == 0 )
    {
        if ( strncmp(script,"a9",2) == 0 )
        {
            if ( trimflag != 0 )
            {
                script[len-2] = 0;
                script += 2;
            }
            mode = 'm';
        }
        else
        {
            if ( trimflag != 0 )
                script[len-2] = 0;
            mode = 'r';
        }
    }
    return(mode);
}

void update_ramchain(struct compressionvars *V,char *coinstr,char *addr,struct address_entry *bp,uint64_t value,char *txidstr,char *script)
{
    struct hashtable *hashtable_create(char *name,int64_t hashsize,long structsize,long keyoffset,long keysize,long modifiedoffset);
    uint32_t huffind;
    char fname[512],valuestr[64];
    int32_t frequi;
    uint8_t pubkey[128];
    struct coinaddr *addrp = 0;
    struct txinfo *tp = 0;
    struct scriptinfo *sp = 0;
    struct valueinfo *valp = 0;
   // printf("update ramchain.(%s) addr.(%s) block.%d vin.%d %p %p\n",coinstr,addr,bp->blocknum,bp->vinflag,txidstr,script);
    if ( V->fp == 0 )
    {
        if ( IS_LIBTEST != 7 )
        {
            sprintf(fname,"address/%s/%s",coinstr,addr);
            if ( (V->fp= fopen(fname,"rb+")) == 0 )
                V->fp = fopen(fname,"wb");
            else fseek(V->fp,0,SEEK_END);
        }
        else init_compressionvars(V,coinstr);
    }
    if ( V->fp != 0 )
    {
        if ( bp->vinflag == 0 )
        {
            addrp = update_compressionvars_table(V->afp,&V->addrind,V->addrs,addr);
            if ( txidstr != 0 && script != 0 )
            {
                frequi = calc_frequi(coinstr,V->prevblock);
                if ( V->prevblock != bp->blocknum )
                    V->prevblock = flush_compressionvars(V,coinstr,V->prevblock,bp->blocknum,frequi);
            //printf("txid.(%s) %s\n",txidstr,script);
                if ( value != 0 )
                {
                    expand_nxt64bits(valuestr,value);
                    valp = update_compressionvars_table(0,&V->valueind,V->values,valuestr);
                    if ( valp->ind == V->valueind ) // indicates just created
                    {
                        huffind = (valp->ind << 3) | 0, huff_iteminit(&valp->item,&huffind,sizeof(huffind),64,0);
                        valp->item.U.bits.txid = value;
                    }
                }
                tp = update_compressionvars_table(V->tfp,&V->txind,V->txids,txidstr);
                if ( tp->ind == V->txind ) // indicates just created
                    huffind = (tp->ind << 3) | 5, huff_iteminit(&tp->item,&huffind,sizeof(huffind),15,0);
                sp = update_compressionvars_table(V->sfp,&V->scriptind,V->scripts,script);
                memset(pubkey,0,sizeof(pubkey));
                if ( sp->ind == V->scriptind ) // indicates just created
                {
                    sp->mode = calc_scriptmode(pubkey,script,0);
                    huffind = (sp->ind << 3) | 6, huff_iteminit(&sp->item,&huffind,sizeof(huffind),32,0);
                }
                if ( addrp->ind == V->addrind ) // indicates just created
                {
                    memcpy(addrp->pubkey,pubkey,sizeof(addrp->pubkey));
                    huffind = (addrp->ind << 3) | 7, huff_iteminit(&addrp->item,&huffind,sizeof(huffind),32,0);
                }
                update_huffitem(&tp->item,(tp->ind << 3) | 5,15);
                update_huffitem(&sp->item,(sp->ind << 3) | 6,32);
                update_huffitem(&addrp->item,(addrp->ind << 3) | 7,32);
                if ( 0 && V->disp != 0 )
                    sprintf(V->disp+strlen(V->disp),"{A%d T%d.%d S%d %.8f} ",V->addrind,V->txind,bp->v,V->scriptind,dstr(value));
                compressionvars_add_txout(V->rawdata,coinstr,tp->ind,bp->v,addrp->ind,value,sp->ind);
            }
            else //if ( bp->vinflag == 0 ) // dereferenced (blocknum, txind, v)
            {
                if ( 0 && V->disp != 0 )
                    sprintf(V->disp+strlen(V->disp),"[%d %d %d] ",bp->blocknum,bp->txind,bp->v);
                compressionvars_add_txin(V->rawdata,coinstr,bp);
                // ????? add_entry_to_tx(tp,bp);
            }
            V->maxitems++;
            update_coinaddr_entries(addrp,bp);
            //add_entry_to_tx(tp,bp);
        }
        else
        {
            // vin txid:vin is dereferenced above
            //sprintf(V->disp+strlen(V->disp),"(%d %d %d) ",bp->blocknum,bp->txind,bp->v);
        }
        if ( IS_LIBTEST != 7 )
            fclose(V->fp);
    }
}

int main(int argc,const char *argv[])
{
    FILE *fp;
    cJSON *json = 0;
    int32_t retval;
    char ipaddr[64],*oldport,*newport,portstr[64],*retstr;
   // if ( Debuglevel > 0 )
    if ( argc > 1 && strcmp(argv[1],"genfiles") == 0 )
    {
        uint32_t process_coinblocks(char *coinstr);
        retval = SuperNET_start("SuperNET.conf","127.0.0.1");
        printf("process coinblocks\n");
        process_coinblocks((char *)argv[2]);
        getchar();
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
        getchar();
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
                init_MGWconf("SuperNET.conf",0);
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
    if ( (fp= fopen("horrible.hack","wb")) != 0 )
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
        sleep(60);
    return(0);
}


// stubs
int32_t SuperNET_broadcast(char *msg,int32_t duration) { return(0); }
int32_t SuperNET_narrowcast(char *destip,unsigned char *msg,int32_t len) { return(0); }

#ifdef chanc3r
#endif
