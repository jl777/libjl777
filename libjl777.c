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
    char *retstr,*resultstr,*methodstr,*destplugin;
    int32_t len;
    retbuf[0] = 0;
    //printf("<<<<<<<<<<<< INSIDE PLUGIN.(%s)! (%s) initflag.%d process %s\n",plugin->name,jsonstr,initflag,plugin->name);
    if ( initflag > 0 )
    {
        SUPERNET.ppid = OS_getppid();
        SUPERNET.argjson = cJSON_Duplicate(json,1);
        SUPERNET.readyflag = 1;
        if ( SUPERNET.UPNP != 0 )
        {
            char portstr[64];
            sprintf(portstr,"%d",SUPERNET.serviceport), upnpredirect(portstr,portstr,"TCP","SuperNET");
            sprintf(portstr,"%d",SUPERNET.port + LB_OFFSET), upnpredirect(portstr,portstr,"TCP","SuperNET");
            sprintf(portstr,"%d",SUPERNET.port + PUBGLOBALS_OFFSET), upnpredirect(portstr,portstr,"TCP","SuperNET");
            sprintf(portstr,"%d",SUPERNET.port + PUBRELAYS_OFFSET), upnpredirect(portstr,portstr,"TCP","SuperNET");
        }
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



