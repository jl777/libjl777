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

extern int32_t IS_LIBTEST,USESSL;
extern cJSON *MGWconf;
char *bitcoind_RPC(void *deprecated,char *debugstr,char *url,char *userpass,char *command,char *params);
int32_t gen_pingstr(char *cmdstr,int32_t completeflag);
void send_packet(struct nodestats *peerstats,struct sockaddr *destaddr,unsigned char *finalbuf,int32_t len);
void expand_ipbits(char *ipaddr,uint32_t ipbits);
char *get_public_srvacctsecret();

char *SuperNET_url()
{
    return((USESSL == 0) ? (char *)"http://127.0.0.1:7778" : (char *)"https://127.0.0.1:7777");
}

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
    retstr = bitcoind_RPC(0,(char *)"BTCD",SuperNET_url(),(char *)"",(char *)"SuperNET",params);
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
    int32_t i,n,len,numnodes,numcontacts,numipaddrs = 0;
    char ipaddr[64],_cmd[MAX_JSON_FIELD],**ipaddrs;
    uint8_t finalbuf[MAX_JSON_FIELD];
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
    {
        for (i=0; i<numipaddrs; i++)
        {
            printf("%s ",ipaddrs[i]);
        }
    }
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
                expand_ipbits(ipaddr,nodes[i]->ipbits);
                printf("(%llu %d %s) ",(long long)nodes[i]->nxt64bits,nodes[i]->lastcontact-now,ipaddr);
                if ( gen_pingstr(_cmd,1) > 0 )
                {
                    int32_t construct_tokenized_req(char *tokenized,char *cmdjson,char *NXTACCTSECRET);
                    len = construct_tokenized_req((char *)finalbuf,_cmd,get_public_srvacctsecret());
                    send_packet(nodes[i],0,finalbuf,len);
                }
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
        sleep(100);
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
                        fprintf(stderr,"<<<<<<<<<<< GUIpoll: (%s) for [%s]\n",buf,txidstr);
                    else
                    {
                        copy_cJSON(ipaddr,cJSON_GetObjectItem(json,"from"));
                        copy_cJSON(args,cJSON_GetObjectItem(json,"args"));
                        unstringify(args);
                        port = (int32_t)get_API_int(cJSON_GetObjectItem(json,"port"),0);
                        //if ( args[0] != 0 && Debuglevel > 2 )
                        //    printf("(%s) from (%s:%d) -> (%s) Qtxid.(%s)\n",args,ipaddr,port,buf,txidstr);
                    }
                }
                free_json(json);
            } else fprintf(stderr,"<<<<<<<<<<< GUI poll_for_broadcasts: PARSE_ERROR.(%s)\n",retstr);
            free(retstr);
        } //else fprintf(stderr,"<<<<<<<<<<< BTCD poll_for_broadcasts: bitcoind_RPC returns null\n");
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
    else {
        printf("UPNP OK: EXT (%s:%s) %s redirected to INT (%s:%s) (duration=%s)\n",
               externalIPAddress, eport, proto, intClient, intPort, duration);
    }
    
    FreeUPNPUrls(&urls);
    freeUPNPDevlist(devlist);
    return 1; //ok - we are mapped:)
}


int main(int argc,const char *argv[])
{
    FILE *fp;
    int32_t retval;
    char ipaddr[64],*oldport,*newport,portstr[64];
    extern int32_t ENABLE_GUIPOLL;
    sprintf(portstr,"%d",SUPERNET_PORT);
    oldport = newport = portstr;
    if ( upnpredirect(oldport,newport,"UDP","SuperNET Peer") == 0 )
        printf("TEST ERROR: failed redirect (%s) to (%s)\n",oldport,newport);

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
    if ( retval == 0 && ENABLE_GUIPOLL != 0 )
    {
        if ( portable_thread_create((void *)GUIpoll_loop,ipaddr) == 0 )
            printf("ERROR hist process_hashtablequeues\n");
        // else build_topology();
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
/*
int main(int argc, char ** argv)
{
    if(argc<2) {
        printf("TEST ERROR: missing params\n");
        printf("TEST usage: %s ext-port int-port\n",argv[0]);
        exit(1);
    }
    if(!upnpredirect(argv[1],argv[2],"UDP", "SuperNET Peer")) {
        printf("TEST ERROR: failed redirect\n");
        exit(1);
    }
    exit (0);
}*/

// stubs
int32_t SuperNET_broadcast(char *msg,int32_t duration) { return(0); }
int32_t SuperNET_narrowcast(char *destip,unsigned char *msg,int32_t len) { return(0); }

#ifdef chanc3r
#endif
