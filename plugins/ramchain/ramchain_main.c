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


#ifdef INSIDE_MGW

#define BUNDLED
#define PLUGINSTR "ramchain"
#define PLUGNAME(NAME) ramchain ## NAME
#define STRUCTNAME struct PLUGNAME(_info) 
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
#include "../agents/plugin777.c"
#include "../KV/kv777.c"
#include "../common/system777.c"
#include "ramchain.c"
#undef DEFINES_ONLY

STRUCTNAME RAMCHAINS;
#define PUB_METHODS "ledgerhash", "richlist", "txid", "txidind", "addr", "addrind", "script", "scriptind", "balance", "unspents", "notify"

char *PLUGNAME(_pubmethods)[] = { PUB_METHODS }; // list of public methods
char *PLUGNAME(_methods)[] = { PUB_METHODS, "create", "backup", "pause", "resume", "stop" }; // list of supported methods
char *PLUGNAME(_authmethods)[] = { PUB_METHODS, "signrawtransaction", "dumpprivkey" }; // list of authentication methods

int32_t ramchain_idle(struct plugin_info *plugin)
{
    int32_t i,flag = 0;
    struct coin777 *coin; struct ramchain *ramchain;
    for (i=0; i<COINS.num; i++)
    {
        if ( (coin= COINS.LIST[i]) != 0 )
        {
            ramchain = &coin->ramchain;
            if ( ramchain->readyflag != 0 && (SUPERNET.gatewayid >= 0 || milliseconds() > ramchain->lastupdate+6000) )
            {
                flag += ramchain_update(coin,ramchain);
                ramchain->lastupdate = milliseconds();
            }
        }
    }
    return(flag);
}

int32_t PLUGNAME(_process_json)(char *forwarder,char *sender,int32_t valid,struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag,char *tokenstr)
{
    char *coinstr,*resultstr,*methodstr;
    struct coin777 *coin = 0;
    uint32_t startblocknum,endblocknum;
    retbuf[0] = 0;
// Debuglevel = 3;
    printf("<<<<<<<<<<<< INSIDE PLUGIN! process %s\n",plugin->name);
    if ( initflag > 0 )
    {
        strcpy(retbuf,"{\"result\":\"initflag > 0\"}");
        plugin->allowremote = 1;
        RAMCHAINS.fastmode = get_API_int(cJSON_GetObjectItem(json,"fastmode"),0);
        RAMCHAINS.verifyspends = get_API_int(cJSON_GetObjectItem(json,"verifyspends"),1);
        //RAMCHAINS.fileincr = get_API_int(cJSON_GetObjectItem(json,"fileincr"),100L * 1000 * 1);
        RAMCHAINS.readyflag = 1;
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        methodstr = cJSON_str(cJSON_GetObjectItem(json,"method"));
        coinstr = cJSON_str(cJSON_GetObjectItem(json,"coin"));
        startblocknum = get_API_int(cJSON_GetObjectItem(json,"start"),0);
        endblocknum = get_API_int(cJSON_GetObjectItem(json,"end"),0);
        if ( coinstr != 0 )
            coin = coin777_find(coinstr,1);
        if ( methodstr == 0 || methodstr[0] == 0 )
        {
            printf("(%s) has not method\n",jsonstr);
            return(0);
        }
        printf("RAMCHAIN.(%s) for (%s) %p\n",methodstr,coinstr!=0?coinstr:"",coin);
        if ( coin != 0 && coinstr == 0 )
            coinstr = coin->name;
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
        else if ( coin != 0 )
        {
            if ( strcmp(methodstr,"backup") == 0 )
            {
                if ( coin->ramchain.DBs.ctl == 0 )
                    ramchain_init(retbuf,maxlen,coin,&coin->ramchain,json,coinstr,coin->serverport,coin->userpass,startblocknum,endblocknum,coin->minconfirms);
                if ( coin->ramchain.DBs.ctl != 0 )
                {
                    coin777_incrbackup(coin,0,0,0);
                    strcpy(retbuf,"{\"result\":\"started backup\"}");
                } else strcpy(retbuf,"{\"error\":\"cant create ramchain when coin not ready\"}");
            }
            else if ( strcmp(methodstr,"resume") == 0 )
                ramchain_init(retbuf,maxlen,coin,&coin->ramchain,json,coinstr,coin->serverport,coin->userpass,startblocknum,endblocknum,coin->minconfirms);
            else if ( strcmp(methodstr,"create") == 0 )
                ramchain_init(retbuf,maxlen,coin,&coin->ramchain,json,coinstr,coin->serverport,coin->userpass,startblocknum,endblocknum,coin->minconfirms);
            else ramchain_func(retbuf,maxlen,coin,&coin->ramchain,json,methodstr);
            //else sprintf(retbuf,"{\"result\":\"no active ramchain\"}");
            //printf("RAMCHAIN RETURNS.(%s)\n",retbuf);
        }
    }
    return((int32_t)strlen(retbuf) + retbuf[0] != 0);
}

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *argjson)
{
    uint64_t disableflags = 0;
    //plugin->sleepmillis = 1;
    printf("init %s size.%ld\n",plugin->name,sizeof(struct ramchain_info));
    return(disableflags); // set bits corresponding to array position in _methods[]
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    return(retcode);
}
#include "../agents/plugin777.c"

#endif
#include <stdint.h>
extern int32_t Debuglevel;
