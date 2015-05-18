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
char *PLUGNAME(_pubmethods)[] = { "ledgerhash", "richlist", "rawblock" }; // list of public methods
char *PLUGNAME(_methods)[] = { "ledgerhash", "richlist", "rawblock", "create", "backup", "pause", "resume", "stop", "notify" }; // list of supported methods
char *PLUGNAME(_authmethods)[] = { "signrawtransaction", "dumpprivkey" }; // list of authentication methods

int32_t ramchain_idle(struct plugin_info *plugin)
{
    int32_t i,lag,syncflag,flag = 0;
    struct coin777 *coin;
    struct ramchain *ramchain;
    struct ledger_info *ledger;
    for (i=0; i<COINS.num; i++)
    {
        if ( (coin= COINS.LIST[i]) != 0  && (ledger= coin->ramchain.activeledger) != 0 )
        {
            ramchain = &coin->ramchain;
            if ( (lag= (ramchain->RTblocknum - ledger->blocknum)) < 1000 || (ledger->blocknum % 100) == 0 )
                ramchain->RTblocknum = _get_RTheight(&ramchain->lastgetinfo,ramchain->name,coin->serverport,coin->userpass,ramchain->RTblocknum);
            if ( lag < DB777_MATRIXROW*10 && ramchain->syncfreq > DB777_MATRIXROW )
                ramchain->syncfreq = DB777_MATRIXROW;
            else if ( lag < DB777_MATRIXROW && ramchain->syncfreq > DB777_MATRIXROW/10 )
                ramchain->syncfreq = DB777_MATRIXROW/10;
            else if ( lag < DB777_MATRIXROW/10 && ramchain->syncfreq > DB777_MATRIXROW/100 )
                ramchain->syncfreq = DB777_MATRIXROW/100;
            else if ( strcmp(ramchain->name,"BTC") == 0 && lag < DB777_MATRIXROW/100 && ramchain->syncfreq > DB777_MATRIXROW/1000 )
                ramchain->syncfreq = DB777_MATRIXROW/1000;
            if ( ramchain->paused < 10 )
            {
                syncflag = (((ledger->blocknum % ramchain->syncfreq) == 0) || (ramchain->needbackup != 0) || (ledger->blocknum % 10000) == 0);
                //if ( syncflag != 0 )
                //    printf("sync.%d (%d  %d) || %d\n",syncflag,ledger->blocknum,ramchain->syncfreq,ramchain->needbackup);
                if ( ledger->blocknum >= ramchain->endblocknum || ramchain->paused != 0 )
                {
                    if ( ledger->blocknum >= ramchain->endblocknum )
                        ramchain->paused = 3, syncflag = 2;
                    printf("ramchain.%s blocknum.%d <<< PAUSING paused.%d |  endblocknum.%u\n",ramchain->name,ledger->blocknum,ramchain->paused,ramchain->endblocknum);
                }
                flag = ramchain_update(ramchain,coin->serverport,coin->userpass,syncflag * (ledger->blocknum != 0));
                if ( ramchain->paused == 3 )
                {
                    ledger_free(ramchain->activeledger,1), ramchain->activeledger = 0;
                    printf("STOPPED\n");
                }
                if ( ledger->blocknum > ramchain->endblocknum || ramchain->paused != 0 )
                    ramchain->paused = 10;
            }
        }
    }
    return(flag);
}

struct ledger_info *ramchain_session_ledger(struct ramchain *ramchain,int32_t directind)
{
    struct ledger_info *ledger; char numstr[32];
    if ( (ledger= ramchain->session_ledgers[directind]) == 0 )
    {
        sprintf(numstr,"direct.%d",directind);
        ramchain->session_ledgers[directind] = ledger = ledger_alloc(ramchain->name,numstr,DB777_RAM);
        ledger->sessionid = rand();
    }
    return(ledger);
}

int32_t ramchain_rawblock(char *retbuf,int32_t maxlen,struct ramchain *ramchain,char *serverport,char *userpass,char *transport,char *ipaddr,uint16_t port,uint32_t blocknum)
{
    struct alloc_space MEM; struct endpoint epbits; int32_t retval = -1;
    struct ledger_blockinfo *block; struct ledger_info *ledger; struct rawblock *emit;
    epbits = nn_directepbits(retbuf,transport,ipaddr,port);
    if ( epbits.ipbits == 0 )
        return(-1);
    if ( blocknum == 0 )
        sprintf(retbuf,"{\"error\":\"no blocknum specified\"}");
    else if ( (ledger= ramchain_session_ledger(ramchain,epbits.directind)) == 0 )
        sprintf(retbuf,"{\"error\":\"couldnt allocate session ledger\"}");
    else
    {
        emit = malloc(sizeof(*emit));
        memset(&MEM,0,sizeof(MEM)), MEM.size = 10000000, MEM.ptr = malloc(MEM.size);
        sprintf(retbuf,"{\"result\":\"ledger_update\",\"sessionid\":%u,\"counter\":%u}",ledger->sessionid,ledger->counter);
        strcpy(memalloc(&MEM,(int32_t)strlen(retbuf)+1,0),retbuf);
        if ( (block= ledger_update(0,ledger,&MEM,ramchain->name,serverport,userpass,emit,blocknum)) != 0 )
        {
            retval = nn_directsend(epbits,MEM.ptr,(int32_t)MEM.used);
            ledger->counter++;
        }
        else sprintf(retbuf,"{\"error\":\"ramchain_blockstr null return\"}");
        free(emit), free(MEM.ptr);
    }
    return(retval);
}

//ramchain notify {"coin":"BTCD","list":["RFKYx6N8ENFiSrC7w8BJXzwkwg8XkyWYHy"]}

int32_t ramchain_notify(char *retbuf,struct ramchain *ramchain,char *endpoint,cJSON *list)
{
    uint32_t firstblocknum; int32_t i,n,m=0; char *coinaddr; struct ledger_addrinfo *addrinfo; struct ledger_info *ledger = ramchain->activeledger;
    if ( ledger == 0 )
    {
        sprintf(retbuf,"{\"error\":\"no active ledger\"}");
        return(-1);
    }
    if ( endpoint == 0 )
    {
        if ( list == 0 || (n= cJSON_GetArraySize(list)) <= 0 )
            sprintf(retbuf,"{\"error\":\"no notification list\"}");
        else
        {
            for (i=0; i<n; i++)
            {
                if ( (coinaddr= cJSON_str(cJSON_GetArrayItem(list,i))) != 0 && (addrinfo= ledger_addrinfo(&firstblocknum,ledger,coinaddr)) != 0 )
                {
                    m++;
                    addrinfo->notify = 1;
                    printf("NOTIFY.(%s) firstblocknum.%u\n",coinaddr,firstblocknum);
                    sprintf(retbuf,"{\"result\":\"added to notification list\",\"coinaddr\":\"%s\",\"firstblocknum\":\"%u\"}",coinaddr,firstblocknum);
                } else sprintf(retbuf,"{\"error\":\"cant find address\",\"coinaddr\":\"%s\"}",coinaddr);
            }
            sprintf(retbuf,"{\"result\":\"added to notification list\",\"num\":%d,\"from\":%d}",m,n);
        }
    } else sprintf(retbuf,"{\"error\":\"endpoint specified, private channel needs to be implemented\"}");
    return(0);
}

//"8131ffb0a2c945ecaf9b9063e59558784f9c3a74741ce6ae2a18d0571dac15bb",

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
                    ramchain_init(retbuf,coin,coinstr,startblocknum,endblocknum);
                if ( coin->ramchain.activeledger != 0 && coin->ramchain.activeledger->DBs.ctl != 0 )
                {
                    db777_sync(0,&coin->ramchain.activeledger->DBs,ENV777_BACKUP);
                    strcpy(retbuf,"{\"result\":\"started backup\"}");
                } else strcpy(retbuf,"{\"error\":\"cant create ramchain when coin not ready\"}");
            }
            else if ( strcmp(methodstr,"resume") == 0 )
            {
                if ( coin->ramchain.activeledger == 0 )
                    ramchain_init(retbuf,coin,coinstr,startblocknum,endblocknum);
                else
                {
                    ramchain_stop(retbuf,&coin->ramchain);
                    ramchain_init(retbuf,coin,coinstr,startblocknum,endblocknum);
                    ramchain_resume(retbuf,&coin->ramchain,coin->serverport,coin->userpass,startblocknum,endblocknum);
                }
            }
            else if ( strcmp(methodstr,"create") == 0 )
                ramchain_init(retbuf,coin,coinstr,startblocknum,endblocknum);
            else if ( coin->ramchain.activeledger != 0 )
            {
                if ( strcmp(methodstr,"pause") == 0 )
                {
                    coin->ramchain.paused = 1;
                    sprintf(retbuf,"{\"result\":\"started pause sequence\"}");
                }
                else if ( strcmp(methodstr,"rawblock") == 0 )
                {
                    ramchain_rawblock(retbuf,maxlen,&coin->ramchain,coin->serverport,coin->userpass,cJSON_str(cJSON_GetObjectItem(json,"mytransport")),cJSON_str(cJSON_GetObjectItem(json,"myipaddr")),get_API_int(cJSON_GetObjectItem(json,"myport"),0),get_API_int(cJSON_GetObjectItem(json,"blocknum"),0));
                }
                else if ( strcmp(methodstr,"ledgerhash") == 0 )
                    ramchain_ledgerhash(retbuf,maxlen,&coin->ramchain,json);
                else if ( strcmp(methodstr,"richlist") == 0 )
                    ramchain_richlist(retbuf,maxlen,&coin->ramchain,get_API_int(cJSON_GetObjectItem(json,"num"),25));
                else if ( strcmp(methodstr,"notify") == 0 )
                    ramchain_notify(retbuf,&coin->ramchain,cJSON_str(cJSON_GetObjectItem(json,"endpoint")),cJSON_GetObjectItem(json,"list"));
                else if ( strcmp(methodstr,"stop") == 0 )
                {
                    coin->ramchain.paused = 3;
                    sprintf(retbuf,"{\"result\":\"pausing then stopping ramchain\"}");
                }
            }
            else sprintf(retbuf,"{\"result\":\"no active ramchain\"}");
            printf("RAMCHAIN RETURNS.(%s)\n",retbuf);
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
