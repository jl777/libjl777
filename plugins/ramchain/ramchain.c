//
//  ramchain.c
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_ramchain_h
#define crypto777_ramchain_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"
#include "system777.c"
#include "ledger777.c"

int32_t ramchain_init(char *retbuf,int32_t maxlen,struct ramchain *ramchain,cJSON *argjson,char *coinstr,char *serverport,char *userpass,uint32_t startblocknum,uint32_t endblocknum,uint32_t minconfirms);
int32_t ramchain_update(struct ramchain *ramchain,struct ledger_info *ledger);
int32_t ramchain_func(char *retbuf,int32_t maxlen,struct ramchain *ramchain,cJSON *argjson,char *method);
int32_t ramchain_resume(char *retbuf,int32_t maxlen,struct ramchain *ramchain,cJSON *argjson,uint32_t startblocknum,uint32_t endblocknum);

#endif
#else
#ifndef crypto777_ramchain_c
#define crypto777_ramchain_c

#ifndef crypto777_ramchain_h
#define DEFINES_ONLY
#include "ramchain.c"
#undef DEFINES_ONLY
#endif

int32_t ramchain_update(struct ramchain *ramchain,struct ledger_info *ledger)
{
    struct alloc_space MEM; uint32_t blocknum; double startmilli; int32_t lag,syncflag,flag = 0;
    blocknum = ledger->blocknum;
    if ( (lag= (ramchain->RTblocknum - blocknum)) < 1000 || (blocknum % 100) == 0 )
        ramchain->RTblocknum = _get_RTheight(&ramchain->lastgetinfo,ramchain->name,ramchain->serverport,ramchain->userpass,ramchain->RTblocknum);
    if ( lag < DB777_MATRIXROW*10 && ledger->syncfreq > DB777_MATRIXROW )
        ledger->syncfreq = DB777_MATRIXROW;
    else if ( lag < DB777_MATRIXROW && ledger->syncfreq > DB777_MATRIXROW/10 )
        ledger->syncfreq = DB777_MATRIXROW/10;
    else if ( lag < DB777_MATRIXROW/10 && ledger->syncfreq > DB777_MATRIXROW/100 )
        ledger->syncfreq = DB777_MATRIXROW/100;
    else if ( strcmp(ledger->DBs.coinstr,"BTC") == 0 && lag < DB777_MATRIXROW/100 && ledger->syncfreq > DB777_MATRIXROW/1000 )
        ledger->syncfreq = DB777_MATRIXROW/1000;
    if ( ramchain->paused < 10 )
    {
        syncflag = (((blocknum % ledger->syncfreq) == 0) || (ledger->needbackup != 0) || (blocknum % 10000) == 0);
        //if ( syncflag != 0 )
        //    printf("sync.%d (%d  %d) || %d\n",syncflag,blocknum,ramchain->syncfreq,ramchain->needbackup);
        if ( blocknum >= ledger->endblocknum || ramchain->paused != 0 )
        {
            if ( blocknum >= ledger->endblocknum )
                ramchain->paused = 3, syncflag = 2;
            printf("ramchain.%s blocknum.%d <<< PAUSING paused.%d |  endblocknum.%u\n",ledger->DBs.coinstr,blocknum,ramchain->paused,ledger->endblocknum);
        }
        if ( blocknum <= (ramchain->RTblocknum - ramchain->minconfirms) )
        {
            memset(&MEM,0,sizeof(MEM)), MEM.ptr = &ramchain->DECODE, MEM.size = sizeof(ramchain->DECODE);
            startmilli = milliseconds();
            if ( rawblock_load(&ramchain->EMIT,ramchain->name,ramchain->serverport,ramchain->userpass,blocknum) > 0 )
            {
                dxblend(&ledger->load_elapsed,(milliseconds() - startmilli),.99); printf("%.3f ",ledger->load_elapsed/1000.);
                flag = ledger_update(&ramchain->EMIT,ledger,&MEM,ramchain->RTblocknum,syncflag * (blocknum != 0),ramchain->minconfirms);
            }
        }
        if ( ramchain->paused == 3 )
        {
            ledger_free(ramchain->activeledger,1), ramchain->activeledger = 0;
            printf("STOPPED\n");
        }
        if ( blocknum > ledger->endblocknum || ramchain->paused != 0 )
            ramchain->paused = 10;
    }
    return(flag);
}

int32_t ramchain_resume(char *retbuf,int32_t maxlen,struct ramchain *ramchain,cJSON *argjson,uint32_t startblocknum,uint32_t endblocknum)
{
    extern uint32_t Duplicate,Mismatch,Added;
    struct ledger_info *ledger; struct alloc_space MEM; uint64_t balance; int32_t numlinks,numlinks2;
    if ( (ledger= ramchain->activeledger) == 0 )
    {
        sprintf(retbuf,"{\"error\":\"no active ledger\"}");
        return(-1);
    }
    Duplicate = Mismatch = Added = 0;
    ledger->startmilli = milliseconds();
    ledger->totalsize = 0;
    printf("resuming %u to %u\n",startblocknum,endblocknum);
    ledger->startblocknum = ledger_startblocknum(ledger,-1);
    printf("updated %u to %u\n",ledger->startblocknum,endblocknum);
    memset(&MEM,0,sizeof(MEM)), MEM.ptr = &ramchain->DECODE, MEM.size = sizeof(ramchain->DECODE);
    ledger_setblocknum(ledger,&MEM,ledger->startblocknum);
    ledger->blocknum = ledger->startblocknum + (ledger->startblocknum > 1);
    printf("set %u to %u | sizes: uthash.%ld addrinfo.%ld unspentmap.%ld txoffset.%ld db777_entry.%ld\n",ledger->startblocknum,endblocknum,sizeof(UT_hash_handle),sizeof(struct ledger_addrinfo),sizeof(struct unspentmap),sizeof(struct upair32),sizeof(struct db777_entry));
    ledger->endblocknum = (endblocknum > ledger->startblocknum) ? endblocknum : ledger->startblocknum;
    if ( 0 )
    {
        numlinks = db777_linkDB(ledger->addrs.DB,ledger->revaddrs.DB,ledger->addrs.ind);
        numlinks2 = db777_linkDB(ledger->txids.DB,ledger->revtxids.DB,ledger->txids.ind);
        printf("addrs numlinks.%d, txids numlinks.%d\n",numlinks,numlinks2);
    }
    balance = ledger_recalc_addrinfos(0,0,ledger,0);
    printf("balance recalculated %.8f\n",dstr(balance));
    sprintf(retbuf,"{\"result\":\"resumed\",\"ledgerhash\":\"%llx\",\"startblocknum\":%d,\"endblocknum\":%d,\"addrsum\":%.8f,\"ledger supply\":%.8f,\"diff\":%.8f,\"elapsed\":%.3f}",*(long long *)ledger->ledger.sha256,ledger->startblocknum,ledger->endblocknum,dstr(balance),dstr(ledger->voutsum) - dstr(ledger->spendsum),dstr(balance) - (dstr(ledger->voutsum) - dstr(ledger->spendsum)),(milliseconds() - ledger->startmilli)/1000.);
    ramchain->RTblocknum = _get_RTheight(&ramchain->lastgetinfo,ramchain->name,ramchain->serverport,ramchain->userpass,ramchain->RTblocknum);
    ledger->startmilli = milliseconds();
    ramchain->paused = 0;
    return(0);
}

int32_t ramchain_pause(char *retbuf,int32_t maxlen,struct ramchain *ramchain,cJSON *argjson)
{
    ramchain->paused = 1;
    sprintf(retbuf,"{\"result\":\"started pause sequence\"}");
    return(0);
}

int32_t ramchain_stop(char *retbuf,int32_t maxlen,struct ramchain *ramchain,cJSON *argjson)
{
    ramchain->paused = 3;
    sprintf(retbuf,"{\"result\":\"pausing then stopping ramchain\"}");
    return(0);
}

int32_t ramchain_init(char *retbuf,int32_t maxlen,struct ramchain *ramchain,cJSON *argjson,char *coinstr,char *serverport,char *userpass,uint32_t startblocknum,uint32_t endblocknum,uint32_t minconfirms)
{
    struct ledger_info *ledger;
    strcpy(ramchain->name,coinstr);
    strcpy(ramchain->serverport,serverport);
    strcpy(ramchain->userpass,userpass);
    ramchain->readyflag = 1;
    ramchain->minconfirms = minconfirms;
    if ( ramchain->activeledger != 0 )
    {
        ramchain_stop(retbuf,maxlen,ramchain,argjson);
        while ( ramchain->activeledger != 0 )
            sleep(1);
    }
    if ( (ramchain->activeledger= ledger_alloc(coinstr,"",0)) != 0 )
    {
        ledger = ramchain->activeledger;
        ledger->syncfreq = DB777_MATRIXROW;
        ledger->startblocknum = startblocknum, ledger->endblocknum = endblocknum;
        ramchain->RTblocknum = _get_RTheight(&ramchain->lastgetinfo,ramchain->name,ramchain->serverport,ramchain->userpass,ramchain->RTblocknum);
        env777_start(0,&ledger->DBs,ramchain->RTblocknum);
        if ( endblocknum == 0 )
            endblocknum = 1000000000;
        return(ramchain_resume(retbuf,maxlen,ramchain,argjson,startblocknum,endblocknum));
    }
    sprintf(retbuf,"{\"error\":\"no coin777\"}");
    return(-1);
}

/*struct ledger_info *ramchain_session_ledger(struct ramchain *ramchain,int32_t directind)
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
 ramchain_rawblock(retbuf,maxlen,&coin->ramchain,coin->serverport,coin->userpass,cJSON_str(cJSON_GetObjectItem(json,"mytransport")),cJSON_str(cJSON_GetObjectItem(json,"myipaddr")),get_API_int(cJSON_GetObjectItem(json,"myport"),0),get_API_int(cJSON_GetObjectItem(json,"blocknum"),0));

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
}*/

//ramchain notify {"coin":"BTCD","list":["RFKYx6N8ENFiSrC7w8BJXzwkwg8XkyWYHy"]}

int32_t ramchain_notify(char *retbuf,int32_t maxlen,struct ramchain *ramchain,cJSON *argjson)
{
    struct ledger_info *ledger; uint32_t firstblocknum; int32_t i,n,m=0; char *coinaddr; struct ledger_addrinfo *addrinfo; cJSON *list;
    if ( (ledger= ramchain->activeledger) == 0 )
    {
        sprintf(retbuf,"{\"error\":\"no active ledger\"}");
        return(-1);
    }
    if ( (list= cJSON_GetObjectItem(argjson,"list")) == 0 || (n= cJSON_GetArraySize(list)) <= 0 )
        sprintf(retbuf,"{\"error\":\"no notification list\"}");
    else
    {
        for (i=0; i<n; i++)
        {
            if ( (coinaddr= cJSON_str(cJSON_GetArrayItem(list,i))) != 0 && (addrinfo= ledger_addrinfo(&firstblocknum,ledger,coinaddr,0)) != 0 )
            {
                m++;
                addrinfo->notify = 1;
                printf("NOTIFY.(%s) firstblocknum.%u\n",coinaddr,firstblocknum);
                sprintf(retbuf,"{\"result\":\"added to notification list\",\"coinaddr\":\"%s\",\"firstblocknum\":\"%u\"}",coinaddr,firstblocknum);
            } else sprintf(retbuf,"{\"error\":\"cant find address\",\"coinaddr\":\"%s\"}",coinaddr);
        }
        sprintf(retbuf,"{\"result\":\"added to notification list\",\"num\":%d,\"from\":%d}",m,n);
    }
    return(0);
}

//"8131ffb0a2c945ecaf9b9063e59558784f9c3a74741ce6ae2a18d0571dac15bb",

int32_t ramchain_richlist(char *retbuf,int32_t maxlen,struct ramchain *ramchain,cJSON *argjson)
{
    cJSON *json; int32_t num = get_API_int(cJSON_GetObjectItem(argjson,"num"),25);
    if ( ramchain->activeledger != 0 )
    {
        ledger_recalc_addrinfos(retbuf,maxlen,ramchain->activeledger,num);
        if ( (json= cJSON_Parse(retbuf)) == 0 )
            printf("PARSE ERROR!\n");
        else free_json(json);
    }
    return(0);
}

int32_t ramchain_ledgerhash(char *retbuf,int32_t maxlen,struct ramchain *ramchain,cJSON *argjson)
{
    struct ledger_inds L[32]; int32_t i,n; char ledgerhash[65],*jsonstr; struct ledger_info *ledger = ramchain->activeledger;
    cJSON *item,*array,*json;
    if ( ledger == 0 )
    {
        sprintf(retbuf,"{\"error\":\"no active ledger\",\"coin\":\"%s\"}",ramchain->name);
        return(-1);
    }
    json = cJSON_CreateObject();
    if ( (n= ledger_syncblocks(L,(int32_t)(sizeof(L)/sizeof(*L)),ledger)) > 0 )
    {
        array = cJSON_CreateArray();
        for (i=0; i<n; i++)
        {
            if ( ledger_ledgerhash(ledgerhash,&L[i]) == 0 )
            {
                item = cJSON_CreateArray();
                cJSON_AddItemToArray(item,cJSON_CreateNumber(L[i].blocknum));
                cJSON_AddItemToArray(item,cJSON_CreateString(ledgerhash));
                cJSON_AddItemToArray(array,item);
            }
        }
        cJSON_AddItemToObject(json,"result",cJSON_CreateString("success"));
        cJSON_AddItemToObject(json,"coin",cJSON_CreateString(ramchain->name));
        cJSON_AddItemToObject(json,"latest",cJSON_CreateNumber(ledger->blocknum));
        cJSON_AddItemToObject(json,"ledgerhashes",array);
        jsonstr = cJSON_Print(json), free_json(json);
        _stripwhite(jsonstr,' ');
        strncpy(retbuf,jsonstr,maxlen-1), retbuf[maxlen-1] = 0;
        free(jsonstr);
        return(0);
    }
    sprintf(retbuf,"{\"error\":\"no sync data\",\"coin\":\"%s\"}",ramchain->name);
    return(-1);
}

int32_t ramchain_rawind(char *retbuf,int32_t maxlen,struct ramchain *ramchain,cJSON *argjson,char *field,char *indname,uint32_t (*ledger_indfuncp)(uint32_t *firstblocknump,struct ledger_info *ledger,char *txidstr))
{
    char *str; uint32_t rawind,firstblocknum;
    if ( (str= cJSON_str(cJSON_GetObjectItem(argjson,field))) != 0 )
    {
        if ( (rawind= (*ledger_indfuncp)(&firstblocknum,ramchain->activeledger,str)) != 0 )
        {
            sprintf(retbuf,"{\"result\":\"success\",\"coin\":\"%s\",\"%s\":\"%s\",\"%s\":\"%u\",\"firstblocknum\":\"%u\"}",ramchain->name,field,str,indname,rawind,firstblocknum);
        } else sprintf(retbuf,"{\"error\":\"cant find %s\",\"coin\":\"%s\"}",field,ramchain->name);
    }
    if ( retbuf[0] == 0 )
        sprintf(retbuf,"{\"error\":\"no %s\",\"coin\":\"%s\"}",field,ramchain->name);
    return(-1);
}

int32_t ramchain_txidind(char *retbuf,int32_t maxlen,struct ramchain *ramchain,cJSON *argjson)
{
    return(ramchain_rawind(retbuf,maxlen,ramchain,argjson,"txid","txidind",ledger_txidind));
}

int32_t ramchain_addrind(char *retbuf,int32_t maxlen,struct ramchain *ramchain,cJSON *argjson)
{
    return(ramchain_rawind(retbuf,maxlen,ramchain,argjson,"addr","addrind",ledger_addrind));
}

int32_t ramchain_scriptind(char *retbuf,int32_t maxlen,struct ramchain *ramchain,cJSON *argjson)
{
    return(ramchain_rawind(retbuf,maxlen,ramchain,argjson,"script","scriptind",ledger_scriptind));
}

int32_t ramchain_string(char *retbuf,int32_t maxlen,struct ramchain *ramchain,cJSON *argjson,char *field,char *strname,int32_t (*ledger_strfuncp)(struct ledger_info *ledger,char *str,int32_t max,uint32_t rawind))
{
    char str[8192]; uint32_t rawind;
    if ( (rawind= (uint32_t)get_API_int(cJSON_GetObjectItem(argjson,field),0)) != 0 )
    {
        if ( (*ledger_strfuncp)(ramchain->activeledger,str,sizeof(str),rawind) == 0 )
        {
            sprintf(retbuf,"{\"result\":\"success\",\"coin\":\"%s\",\"%s\":\"%u\",\"%s\":\"%s\"}",ramchain->name,field,rawind,strname,str);
        } else sprintf(retbuf,"{\"error\":\"cant find %s\",\"coin\":\"%s\"}",ramchain->name,field);
    }
    if ( retbuf[0] == 0 )
        sprintf(retbuf,"{\"error\":\"no %s\",\"coin\":\"%s\"}",field,ramchain->name);
    return(-1);
}

int32_t ramchain_txid(char *retbuf,int32_t maxlen,struct ramchain *ramchain,cJSON *argjson)
{
    return(ramchain_string(retbuf,maxlen,ramchain,argjson,"txidind","txid",ledger_txidstr));
}

int32_t ramchain_coinaddr(char *retbuf,int32_t maxlen,struct ramchain *ramchain,cJSON *argjson)
{
    return(ramchain_string(retbuf,maxlen,ramchain,argjson,"addrind","coinaddr",ledger_coinaddr));
}

int32_t ramchain_script(char *retbuf,int32_t maxlen,struct ramchain *ramchain,cJSON *argjson)
{
    return(ramchain_string(retbuf,maxlen,ramchain,argjson,"script","script",ledger_scriptstr));
}

struct ledger_addrinfo *ramchain_addrinfo(char *field,char *retbuf,int32_t maxlen,struct ramchain *ramchain,cJSON *argjson)
{
    char *coinaddr=0; uint32_t addrind,firstblocknum; struct ledger_addrinfo *addrinfo;
    if ( (addrind= (uint32_t)cJSON_GetObjectItem(argjson,"addrind")) == 0 && (coinaddr= cJSON_str(cJSON_GetObjectItem(argjson,"addr"))) != 0 )
        strcpy(field,coinaddr);
    else if ( addrind != 0 )
        sprintf(field,"addrind.%u",addrind);
    else
    {
        sprintf(retbuf,"{\"error\":\"invalid addr or addrind\",\"coin\":\"%s\"}",ramchain->name);
        return(0);
    }
    if ( (addrinfo= ledger_addrinfo(&firstblocknum,ramchain->activeledger,coinaddr,addrind)) == 0 )
        sprintf(retbuf,"{\"error\":\"cant find %s\",\"coin\":\"%s\"}",field,ramchain->name);
    return(addrinfo);
}

int32_t ramchain_balance(char *retbuf,int32_t maxlen,struct ramchain *ramchain,cJSON *argjson)
{
    struct ledger_addrinfo *addrinfo; char field[256];
    if ( (addrinfo= ramchain_addrinfo(field,retbuf,maxlen,ramchain,argjson)) == 0 )
        return(-1);
    sprintf(retbuf,"{\"result\":\"success\",\"coin\":\"%s\",\"addr\":%s,\"balance\":%.8f}",ramchain->name,field,dstr(addrinfo->balance));
    return(0);
}

int32_t ramchain_unspents(char *retbuf,int32_t maxlen,struct ramchain *ramchain,cJSON *argjson)
{
    int32_t ledger_unspentmap(char *txidstr,struct ledger_info *ledger,uint32_t unspentind);
  //struct ledger_addrinfo { uint64_t balance; uint32_t firstblocknum,count:28,notify:1,pending:1,MGW:1,dirty:1; struct unspentmap unspents[]; };
    struct ledger_addrinfo *addrinfo; cJSON *json,*array,*item; uint64_t sum = 0; int32_t i,vout,verbose; char *jsonstr,script[8193],txidstr[256],field[256];
    if ( (addrinfo= ramchain_addrinfo(field,retbuf,maxlen,ramchain,argjson)) == 0 )
        return(-1);
    verbose = get_API_int(cJSON_GetObjectItem(argjson,"verbose"),0);
    json = cJSON_CreateObject(), array = cJSON_CreateArray();
    for (i=0; i<addrinfo->count; i++)
    {
        sum += addrinfo->unspents[i].value;
        if ( verbose == 0 )
        {
            item = cJSON_CreateArray();
            cJSON_AddItemToArray(item,cJSON_CreateNumber(dstr(addrinfo->unspents[i].value)));
            cJSON_AddItemToArray(item,cJSON_CreateNumber(addrinfo->unspents[i].ind));
            cJSON_AddItemToArray(item,cJSON_CreateNumber(addrinfo->unspents[i].scriptind));
        }
        else
        {
            item = cJSON_CreateObject();
            cJSON_AddItemToObject(item,"value",cJSON_CreateNumber(dstr(addrinfo->unspents[i].value)));
            if ( (vout= ledger_unspentmap(txidstr,ramchain->activeledger,addrinfo->unspents[i].ind)) >= 0 )
            {
                cJSON_AddItemToObject(item,"txid",cJSON_CreateString(txidstr));
                cJSON_AddItemToObject(item,"vout",cJSON_CreateNumber(vout));
            }
            cJSON_AddItemToObject(item,"unspentind",cJSON_CreateNumber(addrinfo->unspents[i].ind));
            ledger_scriptstr(ramchain->activeledger,script,sizeof(script),addrinfo->unspents[i].scriptind);
            cJSON_AddItemToObject(item,"script",cJSON_CreateString(script));
        }
        cJSON_AddItemToArray(array,item);
    }
    cJSON_AddItemToObject(json,"unspents",array);
    cJSON_AddItemToObject(json,"count",cJSON_CreateNumber(addrinfo->count));
    cJSON_AddItemToObject(json,"addr",cJSON_CreateString(field));
    cJSON_AddItemToObject(json,"balance",cJSON_CreateNumber(dstr(addrinfo->balance)));
    cJSON_AddItemToObject(json,"sum",cJSON_CreateNumber(dstr(sum)));
    if ( addrinfo->notify != 0 )
        cJSON_AddItemToObject(json,"notify",cJSON_CreateNumber(1));
    if ( addrinfo->pending != 0 )
        cJSON_AddItemToObject(json,"pending",cJSON_CreateNumber(1));
    if ( addrinfo->MGW != 0 )
        cJSON_AddItemToObject(json,"MGW",cJSON_CreateNumber(1));
    jsonstr = cJSON_Print(json), free_json(json);
    _stripwhite(jsonstr,' ');
    strncpy(retbuf,jsonstr,maxlen-1), retbuf[maxlen-1] = 0, free(jsonstr);
    return(0);
}

char *ramchain_funcs[][2] =
{
    { "stop", (char *)ramchain_stop }, { "pause", (char *)ramchain_pause },
    { "ledgerhash", (char *)ramchain_ledgerhash }, { "richlist", (char *)ramchain_richlist },
    { "txid", (char *)ramchain_txid }, { "addr", (char *)ramchain_coinaddr }, { "script", (char *)ramchain_script },
    { "txidind", (char *)ramchain_txidind }, { "addrind", (char *)ramchain_addrind }, { "scriptind", (char *)ramchain_scriptind },
    { "balance", (char *)ramchain_balance }, { "unspents", (char *)ramchain_unspents },
    { "notify", (char *)ramchain_notify },
};

int32_t ramchain_func(char *retbuf,int32_t maxlen,struct ramchain *ramchain,cJSON *argjson,char *method)
{
    int32_t i; void *ptr;
    int32_t (*funcp)(char *retbuf,int32_t maxlen,struct ramchain *ramchain,cJSON *argjson);
    for (i=0; i<(int32_t)(sizeof(ramchain_funcs)/sizeof(*ramchain_funcs)); i++)
    {
        if ( strcmp(method,ramchain_funcs[i][0]) == 0 )
        {
            ptr = (void *)ramchain_funcs[i][1];
            memcpy(&funcp,&ptr,sizeof(funcp));
            return((*funcp)(retbuf,maxlen,ramchain,argjson));
        }
    }
    return(-1);
}

#endif
#endif
