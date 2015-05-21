//
//  ramchain.c
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//


#define BUNDLED
#define PLUGINSTR "ramchain"
#define PLUGNAME(NAME) ramchain ## NAME
#define STRUCTNAME struct PLUGNAME(_info) 
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
#include "../plugin777.c"
#include "storage.c"
#include "system777.c"
#include "ledger777.c"
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
    struct coin777 *coin; struct ramchain *ramchain; struct ledger_info *ledger;
    for (i=0; i<COINS.num; i++)
    {
        if ( (coin= COINS.LIST[i]) != 0 )
        {
            ramchain = &coin->ramchain;
            if ( ramchain->readyflag != 0 && (ledger= ramchain->activeledger) != 0 )
                 flag += ramchain_update(ramchain,ledger);
        }
    }
    return(flag);
}

int32_t PLUGNAME(_process_json)(struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag)
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
        copy_cJSON(RAMCHAINS.pullnode,cJSON_GetObjectItem(json,"pullnode"));
        RAMCHAINS.fastmode = get_API_int(cJSON_GetObjectItem(json,"fastmode"),0);
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
        printf("RAMCHAIN.(%s) for (%s)\n",methodstr,coinstr!=0?coinstr:"");
        if ( coinstr != 0 )
            coin = coin777_find(coinstr,1);
        if ( methodstr == 0 || methodstr[0] == 0 )
        {
            printf("(%s) has not method\n",jsonstr);
            return(0);
        }
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
                if ( coin->ramchain.activeledger == 0 )
                    ramchain_init(retbuf,maxlen,&coin->ramchain,json,coinstr,coin->serverport,coin->userpass,startblocknum,endblocknum,coin->minconfirms);
                if ( coin->ramchain.activeledger != 0 && coin->ramchain.activeledger->DBs.ctl != 0 )
                {
                    db777_sync(0,&coin->ramchain.activeledger->DBs,ENV777_BACKUP);
                    strcpy(retbuf,"{\"result\":\"started backup\"}");
                } else strcpy(retbuf,"{\"error\":\"cant create ramchain when coin not ready\"}");
            }
            else if ( strcmp(methodstr,"resume") == 0 )
                ramchain_init(retbuf,maxlen,&coin->ramchain,json,coinstr,coin->serverport,coin->userpass,startblocknum,endblocknum,coin->minconfirms);
            else if ( strcmp(methodstr,"create") == 0 )
                ramchain_init(retbuf,maxlen,&coin->ramchain,json,coinstr,coin->serverport,coin->userpass,startblocknum,endblocknum,coin->minconfirms);
            else if ( coin->ramchain.activeledger != 0 )
                ramchain_func(retbuf,maxlen,&coin->ramchain,json,methodstr);
            //else sprintf(retbuf,"{\"result\":\"no active ramchain\"}");
            //printf("RAMCHAIN RETURNS.(%s)\n",retbuf);
        }
    }
    return((int32_t)strlen(retbuf));
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
#include "../plugin777.c"
