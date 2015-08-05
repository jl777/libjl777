//
//  jl777.cpp
//  glue code for pNXT
//
//  Created by jl777 on 7/6/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#include <stdio.h>
//
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

#define BUNDLED
#define PLUGINSTR "SuperNET"
#define PLUGNAME(NAME) SuperNET ## NAME
#define STRUCTNAME struct PLUGNAME(_info)
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
#include "plugins/agents/plugin777.c"
#include "plugins/KV/kv777.c"
#include "plugins/utils/NXT777.c"
#include "plugins/common/system777.c"
#include "plugins/utils/files777.c"
#undef DEFINES_ONLY

int32_t SuperNET_idle(struct plugin_info *plugin)
{
    return(0);
}

STRUCTNAME SUPERNET;
int32_t Debuglevel;

char *PLUGNAME(_methods)[] = { "install", "plugin", "agent" }; // list of supported methods
char *PLUGNAME(_pubmethods)[] = { "ping", "pong" }; // list of supported methods
char *PLUGNAME(_authmethods)[] = { "setpass" }; // list of supported methods

uint64_t set_account_NXTSECRET(char *NXTacct,char *NXTaddr,char *secret,int32_t max,cJSON *argjson,char *coinstr,char *serverport,char *userpass)
{
    uint64_t allocsize,nxt64bits;
    char coinaddr[MAX_JSON_FIELD],*str,*privkey;
    NXTaddr[0] = 0;
    extract_cJSON_str(secret,max,argjson,"secret");
    if ( Debuglevel > 2 )
        printf("set_account_NXTSECRET.(%s)\n",secret);
    if ( secret[0] == 0 )
    {
        extract_cJSON_str(coinaddr,sizeof(coinaddr),argjson,"privateaddr");
        if ( strcmp(coinaddr,"privateaddr") == 0 )
        {
            if ( (str= loadfile(&allocsize,"privateaddr")) != 0 )
            {
                if ( allocsize < 128 )
                    strcpy(coinaddr,str);
                free(str);
            }
        }
        if ( coinaddr[0] == 0 )
            extract_cJSON_str(coinaddr,sizeof(coinaddr),argjson,"pubsrvaddr");
        printf("coinaddr.(%s)\n",coinaddr);
        if ( coinstr == 0 || serverport == 0 || userpass == 0 || (privkey= dumpprivkey(coinstr,serverport,userpass,coinaddr)) == 0 )
            gen_randomacct(33,NXTaddr,secret,"randvals");
        else
        {
            strcpy(secret,privkey);
            free(privkey);
        }
    }
    else if ( strcmp(secret,"randvals") == 0 )
        gen_randomacct(33,NXTaddr,secret,"randvals");
    nxt64bits = conv_NXTpassword(SUPERNET.myprivkey,SUPERNET.mypubkey,(uint8_t *)secret,(int32_t)strlen(secret));
    expand_nxt64bits(NXTaddr,nxt64bits);
    if ( 1 )
        conv_rsacctstr(NXTacct,nxt64bits);
    printf("(%s) (%s) (%s)\n",NXTacct,NXTaddr,Debuglevel > 2 ? secret : "<secret>");
    return(nxt64bits);
}

// redirect port on external upnp enabled router to port on *this* host
int upnpredirect(const char* eport, const char* iport, const char* proto, const char* description)
{
    
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

int32_t PLUGNAME(_process_json)(char *forwarder,char *sender,int32_t valid,struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag,char *tokenstr)
{
    char *SuperNET_install(char *plugin,char *jsonstr,cJSON *json);
    char *retstr,*resultstr,*methodstr,*destplugin,myipaddr[512];
    uint8_t mysecret[32],mypublic[32];
    FILE *fp;
    int32_t len;
    retbuf[0] = 0;
    //printf("<<<<<<<<<<<< INSIDE PLUGIN.(%s)! (%s) initflag.%d process %s\n",plugin->name,jsonstr,initflag,plugin->name);
    if ( initflag > 0 )
    {
        SUPERNET.ppid = OS_getppid();
        SUPERNET.argjson = cJSON_Duplicate(json,1);
        SUPERNET.disableNXT = get_API_int(cJSON_GetObjectItem(json,"disableNXT"),0);
        SUPERNET.ismainnet = get_API_int(cJSON_GetObjectItem(json,"MAINNET"),1);
        SUPERNET.usessl = get_API_int(cJSON_GetObjectItem(json,"USESSL"),0);
        SUPERNET.NXTconfirms = get_API_int(cJSON_GetObjectItem(json,"NXTconfirms"),10);
        copy_cJSON(SUPERNET.NXTAPIURL,cJSON_GetObjectItem(json,"NXTAPIURL"));
        if ( SUPERNET.NXTAPIURL[0] == 0 )
        {
            if ( SUPERNET.usessl == 0 )
                strcpy(SUPERNET.NXTAPIURL,"http://127.0.0.1:");
            else strcpy(SUPERNET.NXTAPIURL,"https://127.0.0.1:");
            if ( SUPERNET.ismainnet != 0 )
                strcat(SUPERNET.NXTAPIURL,"7876/nxt");
            else strcat(SUPERNET.NXTAPIURL,"6876/nxt");
        }
        copy_cJSON(SUPERNET.userhome,cJSON_GetObjectItem(json,"userdir"));
        if ( SUPERNET.userhome[0] == 0 )
            strcpy(SUPERNET.userhome,"/root");
        strcpy(SUPERNET.NXTSERVER,SUPERNET.NXTAPIURL);
        strcat(SUPERNET.NXTSERVER,"?requestType");
        copy_cJSON(SUPERNET.myNXTacct,cJSON_GetObjectItem(json,"myNXTacct"));
        if ( SUPERNET.disableNXT == 0 )
            set_account_NXTSECRET(SUPERNET.NXTACCT,SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,sizeof(SUPERNET.NXTACCTSECRET)-1,json,0,0,0);
        else strcpy(SUPERNET.NXTADDR,SUPERNET.myNXTacct);
        SUPERNET.my64bits = conv_acctstr(SUPERNET.NXTADDR);
        copy_cJSON(myipaddr,cJSON_GetObjectItem(json,"myipaddr"));
        if ( myipaddr[0] != 0 || SUPERNET.myipaddr[0] == 0 )
            strcpy(SUPERNET.myipaddr,myipaddr);
        if ( SUPERNET.myipaddr[0] != 0 )
            SUPERNET.myipbits = (uint32_t)calc_ipbits(SUPERNET.myipaddr);
        KV777.mmapflag = get_API_int(cJSON_GetObjectItem(json,"mmapflag"),0);
        //if ( strncmp(SUPERNET.myipaddr,"89.248",5) == 0 )
        //    SUPERNET.iamrelay = get_API_int(cJSON_GetObjectItem(json,"iamrelay"),1*0);
        //else
            SUPERNET.iamrelay = get_API_int(cJSON_GetObjectItem(json,"iamrelay"),0);
        copy_cJSON(SUPERNET.hostname,cJSON_GetObjectItem(json,"hostname"));
        SUPERNET.port = get_API_int(cJSON_GetObjectItem(json,"SUPERNET_PORT"),SUPERNET_PORT);
        SUPERNET.serviceport = get_API_int(cJSON_GetObjectItem(json,"serviceport"),SUPERNET_PORT - 2);
        copy_cJSON(SUPERNET.transport,cJSON_GetObjectItem(json,"transport"));
        if ( SUPERNET.transport[0] == 0 )
            strcpy(SUPERNET.transport,SUPERNET.UPNP == 0 ? "tcp" : "ws");
        sprintf(SUPERNET.lbendpoint,"%s://%s:%u",SUPERNET.transport,SUPERNET.myipaddr,SUPERNET.port + LB_OFFSET);
        sprintf(SUPERNET.relayendpoint,"%s://%s:%u",SUPERNET.transport,SUPERNET.myipaddr,SUPERNET.port + PUBRELAYS_OFFSET);
        sprintf(SUPERNET.globalendpoint,"%s://%s:%u",SUPERNET.transport,SUPERNET.myipaddr,SUPERNET.port + PUBGLOBALS_OFFSET);
        copy_cJSON(SUPERNET.SERVICESECRET,cJSON_GetObjectItem(json,"SERVICESECRET"));
        expand_nxt64bits(SUPERNET.SERVICENXT,conv_NXTpassword(mysecret,mypublic,(uint8_t *)SUPERNET.SERVICESECRET,(int32_t)strlen(SUPERNET.SERVICESECRET)));
        printf("SERVICENXT.%s\n",SUPERNET.SERVICENXT);
        SUPERNET.automatch = get_API_int(cJSON_GetObjectItem(json,"automatch"),3);
#ifndef __linux__
        SUPERNET.UPNP = 1;
#endif
        SUPERNET.telepathicdelay = get_API_int(cJSON_GetObjectItem(json,"telepathicdelay"),1000);
        SUPERNET.gatewayid = get_API_int(cJSON_GetObjectItem(json,"gatewayid"),-1);
        SUPERNET.numgateways = get_API_int(cJSON_GetObjectItem(json,"numgateways"),3);
        SUPERNET.UPNP = get_API_int(cJSON_GetObjectItem(json,"UPNP"),SUPERNET.UPNP);
        SUPERNET.APISLEEP = get_API_int(cJSON_GetObjectItem(json,"APISLEEP"),DEFAULT_APISLEEP);
        SUPERNET.PLUGINTIMEOUT = get_API_int(cJSON_GetObjectItem(json,"PLUGINTIMEOUT"),10000);
        if ( SUPERNET.APISLEEP <= 1 )
            SUPERNET.APISLEEP = 1;
        copy_cJSON(SUPERNET.DATADIR,cJSON_GetObjectItem(json,"DATADIR"));
        if ( SUPERNET.DATADIR[0] == 0 )
            strcpy(SUPERNET.DATADIR,"archive");
        Debuglevel = get_API_int(cJSON_GetObjectItem(json,"debug"),Debuglevel);
        if ( (fp= fopen("libs/websocketd","rb")) != 0 )
        {
            fclose(fp);
            strcpy(SUPERNET.WEBSOCKETD,"libs/websocketd");
        }
        else strcpy(SUPERNET.WEBSOCKETD,"websocketd");
        copy_cJSON(SUPERNET.BACKUPS,cJSON_GetObjectItem(json,"backups"));
        if ( SUPERNET.BACKUPS[0] == 0 )
            strcpy(SUPERNET.BACKUPS,"/tmp");
        copy_cJSON(KV777.PATH,cJSON_GetObjectItem(json,"KV777"));
        if ( KV777.PATH[0] == 0 )
            strcpy(KV777.PATH,"./DB");
        os_compatible_path(KV777.PATH), ensure_directory(KV777.PATH);
#ifdef INSIDE_MGW
        copy_cJSON(RAMCHAINS.pullnode,cJSON_GetObjectItem(json,"pullnode"));
        copy_cJSON(SOPHIA.PATH,cJSON_GetObjectItem(json,"SOPHIA"));
        copy_cJSON(SOPHIA.RAMDISK,cJSON_GetObjectItem(json,"RAMDISK"));
        if ( SOPHIA.PATH[0] == 0 )
            strcpy(SOPHIA.PATH,"./DB");
        os_compatible_path(SOPHIA.PATH), ensure_directory(SOPHIA.PATH);
        MGW.port = get_API_int(cJSON_GetObjectItem(json,"MGWport"),7643);
        copy_cJSON(MGW.PATH,cJSON_GetObjectItem(json,"MGWPATH"));
        if ( MGW.PATH[0] == 0 )
            strcpy(MGW.PATH,"/var/www/html/MGW");
        ensure_directory(MGW.PATH);
        sprintf(buf,"%s/msig",MGW.PATH), ensure_directory(buf);
        sprintf(buf,"%s/status",MGW.PATH), ensure_directory(buf);
        sprintf(buf,"%s/sent",MGW.PATH), ensure_directory(buf);
        sprintf(buf,"%s/deposit",MGW.PATH), ensure_directory(buf);
        printf("MGWport.%u >>>>>>>>>>>>>>>>>>> INIT ********************** (%s) (%s) (%s) SUPERNET.port %d UPNP.%d NXT.%s ip.(%s) iamrelay.%d pullnode.(%s)\n",MGW.port,SOPHIA.PATH,MGW.PATH,SUPERNET.NXTSERVER,SUPERNET.port,SUPERNET.UPNP,SUPERNET.NXTADDR,SUPERNET.myipaddr,SUPERNET.iamrelay,RAMCHAINS.pullnode);
        if ( SUPERNET.gatewayid >= 0 )
        {
            if ( DB_NXTaccts == 0 )
                DB_NXTaccts = db777_create(0,0,"NXTaccts",0,0);
            //if ( DB_nodestats == 0 )
            //    DB_nodestats = db777_create(0,0,"nodestats",0,0);
            //if ( DB_busdata == 0 )
            //    DB_busdata = db777_create(0,0,"busdata",0,0);
            if ( DB_NXTtxids == 0 )
                DB_NXTtxids = db777_create(0,0,"NXT_txids",0,0);
            if ( DB_redeems == 0 )
                DB_redeems = db777_create(0,0,"redeems",0,0);
            if ( DB_MGW == 0 )
                DB_MGW = db777_create(0,0,"MGW",0,0);
            if ( DB_msigs == 0 )
                DB_msigs = db777_create(0,0,"msigs",0,0);
            if ( DB_NXTtrades == 0 )
                DB_NXTtrades = db777_create(0,0,"NXT_trades",0,0);
            //if ( DB_services == 0 )
            //    DB_services = db777_create(0,0,"services",0,0);
        }
        else
#else
        {
            extern struct ramkv777 *DB_NXTtrades,*DB_revNXTtrades;
            if ( DB_NXTtrades == 0 )
                DB_NXTtrades = ramkv777_init(0xff,"NXT_trades",sizeof(uint64_t)*2,1);
            if ( DB_revNXTtrades == 0 )
                DB_revNXTtrades = ramkv777_init(0xfe,"revNXT_trades",sizeof(uint32_t),1);
            SUPERNET.NXTaccts = kv777_init(KV777.PATH,"NXTaccts",0);
            SUPERNET.NXTtxids = kv777_init(KV777.PATH,"NXT_txids",0);
        }
#endif
        SUPERNET.PM = kv777_init(KV777.PATH,"PM",0);
        SUPERNET.alias = kv777_init(KV777.PATH,"alias",0);
        SUPERNET.protocols = kv777_init(KV777.PATH,"protocols",0);
        SUPERNET.rawPM = kv777_init(KV777.PATH,"rawPM",0);
        SUPERNET.services = kv777_init(KV777.PATH,"services",0);
        SUPERNET.invoices = kv777_init(KV777.PATH,"invoices",0);
        SUPERNET.readyflag = 1;
        if ( SUPERNET.UPNP != 0 )
        {
            char portstr[64];
            sprintf(portstr,"%d",SUPERNET.serviceport), upnpredirect(portstr,portstr,"TCP","SuperNET");
            sprintf(portstr,"%d",SUPERNET.port + LB_OFFSET), upnpredirect(portstr,portstr,"TCP","SuperNET");
            sprintf(portstr,"%d",SUPERNET.port + PUBGLOBALS_OFFSET), upnpredirect(portstr,portstr,"TCP","SuperNET");
            sprintf(portstr,"%d",SUPERNET.port + PUBRELAYS_OFFSET), upnpredirect(portstr,portstr,"TCP","SuperNET");
        }
        
        //void kv777_test(); kv777_test();
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        methodstr = cJSON_str(cJSON_GetObjectItem(json,"method"));
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        if ( (destplugin= cJSON_str(cJSON_GetObjectItem(json,"name"))) == 0 )
            destplugin = cJSON_str(cJSON_GetObjectItem(json,"path"));
        printf("SUPERNET\n");
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            retstr = "return registered";
        }
        else if ( methodstr != 0 && strcmp(methodstr,"install") == 0 && destplugin != 0 && destplugin[0] != 0 )
            retstr = SuperNET_install(destplugin,jsonstr,json);
        else retstr = "return JSON result";
        strcpy(retbuf,retstr);
        len = (int32_t)strlen(retbuf);
        while ( --len > 0 )
            if ( retbuf[len] == '}' )
                break;
        sprintf(retbuf + len,",\"debug\":%d,\"USESSL\":%d,\"MAINNET\":%d,\"DATADIR\":\"%s\",\"NXTAPI\":\"%s\",\"WEBSOCKETD\":\"%s\",\"SUPERNET_PORT\":%d,\"APISLEEP\":%d,\"domain\":\"%s\"}",Debuglevel,SUPERNET.usessl,SUPERNET.ismainnet,SUPERNET.DATADIR,SUPERNET.NXTAPIURL,SUPERNET.WEBSOCKETD,SUPERNET.port,SUPERNET.APISLEEP,SUPERNET.hostname);
    }
    return((int32_t)strlen(retbuf));
}

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *Globals,cJSON *json)
{
    uint64_t disableflags = 0;
   //ensure_directory(SUPERNET.SOPHIA_DIR);
    //Globals->Gatewayid = -1, Globals->Numgateways = 3;
    //expand_nxt64bits(Globals->NXT_ASSETIDSTR,NXT_ASSETID);
    //init_InstantDEX(calc_nxt64bits(Global_mp->myNXTADDR),1);
    //SaM_PrepareIndices();
    printf("SuperNET init %s size.%ld\n",plugin->name,sizeof(struct SuperNET_info));
    return(disableflags); // set bits corresponding to array position in _methods[]
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    return(retcode);
}
#include "plugins/agents/plugin777.c"



