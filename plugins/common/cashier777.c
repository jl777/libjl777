/******************************************************************************
 * Copyright © 2014-2015 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

#define BUNDLED
#define PLUGINSTR "cashier"
#define PLUGNAME(NAME) cashier ## NAME
#define STRUCTNAME struct PLUGNAME(_info) 
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
#include "../KV/kv777.c"
#include "../agents/plugin777.c"
#undef DEFINES_ONLY

STRUCTNAME CASHIER;

int32_t cashier_idle(struct plugin_info *plugin) { return(0); }

char *PLUGNAME(_methods)[] = { "update" }; // list of supported methods approved for local access
char *PLUGNAME(_pubmethods)[] = { "" }; // list of supported methods approved for public (Internet) access
char *PLUGNAME(_authmethods)[] = { "" }; // list of supported methods that require authentication

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    return(retcode);
}

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *argjson)
{
    uint64_t disableflags = 0;
    printf("init %s size.%ld\n",plugin->name,sizeof(struct cashier_info));
    plugin->allowremote = 1;
    // runtime specific state can be created and put into *data
    return(disableflags); // set bits corresponding to array position in _methods[]
}

int32_t cashier777_update(int32_t (*gatewayfunc)(uint64_t txid,uint32_t blocknum,uint32_t timestamp,uint64_t senderbits,uint64_t amount,uint64_t destbits),char *gatewayNXT,int32_t firstindex,int32_t lastindex)
{
    char cmd[1024],*jsonstr; cJSON *transfers,*array,*item; int32_t rawind,saved[2],i,status,size,n = 0;
    uint64_t txidbits,amount,senderbits,gatewaybits,recvbits,key[4];
    gatewaybits = conv_acctstr(gatewayNXT);
    sprintf(cmd,"requestType=getBlockchainTransactions&account=%s&nonPhasedOnly=true&type=0&subtype=0",gatewayNXT);
    if ( firstindex >= 0 && lastindex >= firstindex )
        sprintf(cmd + strlen(cmd),"&firstIndex=%u&lastIndex=%u",firstindex,lastindex);
    if ( (jsonstr= issue_NXTPOST(cmd)) != 0 )
    {
        //printf("(%s) -> (%s)\n",cmd,jsonstr);
        if ( (transfers= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (array= jarray(&n,transfers,"transactions")) != 0 )
            {
                for (i=0; i<n; i++)
                {
                    item = cJSON_GetArrayItem(array,i);
                    if ( (recvbits= j64bits(item,"recipient")) == gatewaybits && (txidbits= j64bits(item,"transaction")) != 0 && (amount= j64bits(item,"amountNQT")) != 0 && (senderbits= j64bits(item,"sender")) != 0 )
                    {
                        key[0] = recvbits, key[1] = txidbits, key[2] = senderbits, key[3] = amount;
                        size = sizeof(saved);
                        if ( kv777_read(SUPERNET.NXTtxids,key,sizeof(key),saved,&size,0) == 0 || size != sizeof(saved) )
                        {
                            saved[0] = 0, saved[1] = rawind;
                            rawind = SUPERNET.NXTtxids->numkeys;
                            kv777_write(SUPERNET.NXTtxids,key,sizeof(key),saved,sizeof(saved));
                        }
                        if ( kv777_read(SUPERNET.NXTtxids,key,sizeof(key),saved,&size,0) != 0 && size == sizeof(saved) )
                        {
                            if ( saved[0] < 777 && (status= (*gatewayfunc)(txidbits,juint(item,"height"),juint(item,"timestamp"),senderbits,amount,recvbits)) != saved[0] )
                            {
                                saved[0] = status;
                                kv777_write(SUPERNET.NXTtxids,key,sizeof(key),saved,sizeof(saved));
                            }
                        } else printf("kv777_read failure after kv777_write\n");
                    }
                }
            } free_json(transfers);
        } free(jsonstr);
    }
    //if ( firstindex < 0 || lastindex <= firstindex )
    //    printf("assetid.(%s) -> %d entries\n",mgw->assetidstr,n);
    return(n);
}

int32_t PLUGNAME(_process_json)(char *forwarder,char *sender,int32_t valid,struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag,char *tokenstr)
{
    char *resultstr,*methodstr;
    retbuf[0] = 0;
    plugin->allowremote = 1;
    //printf("<<<<<<<<<<<< INSIDE PLUGIN! process %s (%s)\n",plugin->name,jsonstr);
    if ( initflag > 0 )
    {
        // configure settings
        strcpy(retbuf,"{\"result\":\"cashier init\"}");
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        methodstr = cJSON_str(cJSON_GetObjectItem(json,"method"));
        retbuf[0] = 0;
        if ( methodstr == 0 || methodstr[0] == 0 )
        {
            printf("(%s) has not method\n",jsonstr);
            return(0);
        }
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
        else if ( strcmp(methodstr,"echo") == 0 )
        {
            sprintf(retbuf,"{\"result\":\"%s\"}",methodstr);
        }
    }
    return(plugin_copyretstr(retbuf,maxlen,0));
}
#include "../agents/plugin777.c"
