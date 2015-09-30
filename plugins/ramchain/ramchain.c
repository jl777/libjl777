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

#ifdef DEFINES_ONLY
#ifndef crypto777_ramchain_h
#define crypto777_ramchain_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "../includes/cJSON.h"
#include "../common/system777.c"
#include "../coins/coins777.c"
//#include "pass1.c"
//#include "ledger777.c"
#define DB777_MATRIXROW 10000

int32_t ramchain_init(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson,char *coinstr,char *serverport,char *userpass,uint32_t startblocknum,uint32_t endblocknum,uint32_t minconfirms);
int32_t ramchain_update(struct coin777 *coin,struct ramchain *ramchain);
int32_t ramchain_func(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson,char *method);
int32_t ramchain_resume(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson,uint32_t startblocknum,uint32_t endblocknum);
uint32_t ramchain_prepare(struct coin777 *coin,struct ramchain *ramchain);

#endif
#else
#ifndef crypto777_ramchain_c
#define crypto777_ramchain_c

#ifndef crypto777_ramchain_h
#define DEFINES_ONLY
#include "ramchain.c"
#undef DEFINES_ONLY
#endif

int32_t ramchain_update(struct coin777 *coin,struct ramchain *ramchain)
{
    uint32_t blocknum; int32_t lag,syncflag,flag = 0; //double startmilli; struct alloc_space MEM; 
    blocknum = ramchain->blocknum;
    if ( (lag= (ramchain->RTblocknum - blocknum)) < 1000 )
        ramchain->RTmode = 1;
    if ( ramchain->RTmode != 0 || (blocknum % 100) == 0 )
        ramchain->RTblocknum = _get_RTheight(&ramchain->lastgetinfo,coin->name,coin->serverport,coin->userpass,ramchain->RTblocknum);
    if ( lag < DB777_MATRIXROW*10 && ramchain->syncfreq > DB777_MATRIXROW )
        ramchain->syncfreq = DB777_MATRIXROW;
    else if ( lag < DB777_MATRIXROW && ramchain->syncfreq > DB777_MATRIXROW/10 )
        ramchain->syncfreq = DB777_MATRIXROW/10;
    else if ( strcmp(ramchain->DBs.coinstr,"BTC") == 0 && lag < DB777_MATRIXROW/10 && ramchain->syncfreq > DB777_MATRIXROW/100 )
        ramchain->syncfreq = DB777_MATRIXROW/100;
    if ( ramchain->paused < 10 )
    {
        syncflag = (((blocknum % ramchain->syncfreq) == 0) || (ramchain->needbackup != 0) || (blocknum % DB777_MATRIXROW) == 0);
        if ( blocknum >= ramchain->endblocknum || ramchain->paused != 0 )
        {
            if ( blocknum >= ramchain->endblocknum )
                ramchain->paused = 3, syncflag = 2;
            printf("ramchain.%s blocknum.%d <<< PAUSING paused.%d |  endblocknum.%u\n",ramchain->DBs.coinstr,blocknum,ramchain->paused,ramchain->endblocknum);
        }
        if ( coin->minconfirms == 0 )
            coin->minconfirms = (strcmp("BTC",coin->name) == 0) ? 3 : 10;
        if ( blocknum <= (ramchain->RTblocknum - coin->minconfirms) )
             flag = coin777_parse(coin,ramchain->RTblocknum,syncflag * (blocknum != 0),coin->minconfirms);
        if ( ramchain->paused == 3 )
        {
            //ledger_free(ramchain->activeledger,1), ramchain->activeledger = 0;
            printf("STOPPED\n");
        }
        if ( blocknum > ramchain->endblocknum || ramchain->paused != 0 )
            ramchain->paused = 10;
    }
    return(flag);
}

uint32_t ramchain_prepare(struct coin777 *coin,struct ramchain *ramchain)
{
    uint32_t txidind,addrind,scriptind,numrawvouts,numrawvins,timestamp,totaladdrtx; uint64_t credits,debits;
    ramchain->startmilli = milliseconds();
    if ( ramchain->DBs.ctl == 0 )
    {
        ramchain->paused = 1;
        ramchain->RTblocknum = _get_RTheight(&ramchain->lastgetinfo,coin->name,coin->serverport,coin->userpass,ramchain->RTblocknum);
        coin777_initDBenv(coin);
        ramchain->startblocknum = coin777_startblocknum(coin,-1);
        printf("startblocknum.%u\n",ramchain->startblocknum);
        if ( coin777_getinds(coin,ramchain->startblocknum,&credits,&debits,&timestamp,&txidind,&numrawvouts,&numrawvins,&addrind,&scriptind,&totaladdrtx) == 0 )
        {
            coin777_initmmap(coin,ramchain->startblocknum,txidind,addrind,scriptind,numrawvouts,numrawvins,totaladdrtx);
            printf("t%u u%u s%u a%u c%u x%u initialized in %.3f seconds\n",txidind,numrawvouts,numrawvins,addrind,scriptind,totaladdrtx,(milliseconds() - ramchain->startmilli)/1000.);
            coin->verified = !coin777_verify(coin,numrawvouts,numrawvins,credits,debits,addrind,1,&totaladdrtx);
        }
        ramchain->paused = 0;
    }
    return(ramchain->startblocknum);
}

int32_t ramchain_resume(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson,uint32_t startblocknum,uint32_t endblocknum)
{
    ramchain->RTblocknum = _get_RTheight(&ramchain->lastgetinfo,coin->name,coin->serverport,coin->userpass,ramchain->RTblocknum);
    if ( ramchain->DBs.ctl == 0 )
        ramchain_prepare(coin,ramchain);
    ramchain->readyflag = 1;
    ramchain->paused = 0;
    sprintf(retbuf,"{\"result\":\"success\"}");
    return(0);
}

int32_t ramchain_pause(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson)
{
    ramchain->paused = 1;
    sprintf(retbuf,"{\"result\":\"started pause sequence\"}");
    return(0);
}

int32_t ramchain_stop(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson)
{
    ramchain->paused = 3;
    sprintf(retbuf,"{\"result\":\"pausing then stopping ramchain\"}");
    return(0);
}

int32_t ramchain_init(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson,char *coinstr,char *serverport,char *userpass,uint32_t startblocknum,uint32_t endblocknum,uint32_t minconfirms)
{
    coin->minconfirms = minconfirms;
    printf("(%s %s %s) vs (%s %s %s)\n",coinstr,serverport,userpass,coin->name,coin->serverport,coin->userpass);
    ramchain->syncfreq = 10000;
    ramchain->startblocknum = startblocknum, ramchain->endblocknum = endblocknum;
    ramchain->RTblocknum = _get_RTheight(&ramchain->lastgetinfo,coin->name,coin->serverport,coin->userpass,ramchain->RTblocknum);
    if ( endblocknum == 0 )
        ramchain->endblocknum = endblocknum = 1000000000;
    return(ramchain_resume(retbuf,maxlen,coin,ramchain,argjson,startblocknum,endblocknum));
}

//ramchain notify {"coin":"BTCD","list":["RFKYx6N8ENFiSrC7w8BJXzwkwg8XkyWYHy"]}

int32_t ramchain_notify(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson)
{
    /*uint32_t firstblocknum; int32_t i,n,m=0; char *coinaddr; struct coin777_addrinfo *addrinfo; cJSON *list;
    if ( (list= cJSON_GetObjectItem(argjson,"list")) == 0 || (n= cJSON_GetArraySize(list)) <= 0 )
        sprintf(retbuf,"{\"error\":\"no notification list\"}");
    else
    {
        for (i=0; i<n; i++)
        {
            if ( (coinaddr= cJSON_str(cJSON_GetArrayItem(list,i))) != 0 && (addrinfo= coin777_addrinfo(&firstblocknum,ledger,coinaddr,0)) != 0 )
            {
                m++;
                addrinfo->notify = 1;
                printf("NOTIFY.(%s) firstblocknum.%u\n",coinaddr,firstblocknum);
                sprintf(retbuf,"{\"result\":\"added to notification list\",\"coinaddr\":\"%s\",\"firstblocknum\":\"%u\"}",coinaddr,firstblocknum);
            } else sprintf(retbuf,"{\"error\":\"cant find address\",\"coinaddr\":\"%s\"}",coinaddr);
        }
        sprintf(retbuf,"{\"result\":\"added to notification list\",\"num\":%d,\"from\":%d}",m,n);
    }*/
    return(0);
}

//"8131ffb0a2c945ecaf9b9063e59558784f9c3a74741ce6ae2a18d0571dac15bb",

int32_t ramchain_richlist(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson)
{
    cJSON *json; //int32_t num = get_API_int(cJSON_GetObjectItem(argjson,"num"),25);
    if ( ramchain->readyflag != 0 )
    {
        //ledger_recalc_addrinfos(retbuf,maxlen,ramchain->activeledger,num);
        if ( (json= cJSON_Parse(retbuf)) == 0 )
            printf("PARSE ERROR!\n");
        else free_json(json);
    }
    return(0);
}

int32_t ramchain_ledgerhash(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson)
{
    struct coin777_hashes H[32]; int32_t i,n; char ledgerhash[65],*jsonstr; cJSON *item,*array,*json;
    if ( coin == 0 )
    {
        sprintf(retbuf,"{\"error\":\"null coin ptr\",\"coin\":\"%s\"}",coin->name);
        return(-1);
    }
    json = cJSON_CreateObject();
    if ( (n= coin777_syncblocks(H,(int32_t)(sizeof(H)/sizeof(*H)),coin)) > 0 )
    {
        array = cJSON_CreateArray();
        for (i=0; i<n; i++)
        {
            if ( coin777_ledgerhash(ledgerhash,&H[i]) == 0 )
            {
                item = cJSON_CreateArray();
                cJSON_AddItemToArray(item,cJSON_CreateNumber(H[i].blocknum));
                cJSON_AddItemToArray(item,cJSON_CreateString(ledgerhash));
                cJSON_AddItemToArray(array,item);
            }
        }
        cJSON_AddItemToObject(json,"result",cJSON_CreateString("success"));
        cJSON_AddItemToObject(json,"coin",cJSON_CreateString(coin->name));
        cJSON_AddItemToObject(json,"latest",cJSON_CreateNumber(ramchain->blocknum));
        cJSON_AddItemToObject(json,"ledgerhashes",array);
        jsonstr = cJSON_Print(json), free_json(json);
        _stripwhite(jsonstr,' ');
        strncpy(retbuf,jsonstr,maxlen-1), retbuf[maxlen-1] = 0;
        free(jsonstr);
        return(0);
    }
    sprintf(retbuf,"{\"error\":\"no sync data\",\"coin\":\"%s\"}",coin->name);
    return(-1);
}

int32_t ramchain_rawind(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson,char *field,char *indname,uint32_t (*coin_indfuncp)(uint32_t *firstblocknump,struct coin777 *coin,char *txidstr))
{
    char *str; uint32_t rawind,firstblocknum;
    if ( (str= cJSON_str(cJSON_GetObjectItem(argjson,field))) != 0 )
    {
        if ( (rawind= (*coin_indfuncp)(&firstblocknum,coin,str)) != 0 )
        {
            sprintf(retbuf,"{\"result\":\"success\",\"coin\":\"%s\",\"%s\":\"%s\",\"%s\":\"%u\",\"firstblocknum\":\"%u\"}",coin->name,field,str,indname,rawind,firstblocknum);
        } else sprintf(retbuf,"{\"error\":\"cant find %s\",\"coin\":\"%s\"}",field,coin->name);
    }
    if ( retbuf[0] == 0 )
        sprintf(retbuf,"{\"error\":\"no %s\",\"coin\":\"%s\"}",field,coin->name);
    return(-1);
}

int32_t ramchain_txidind(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson)
{
    return(ramchain_rawind(retbuf,maxlen,coin,ramchain,argjson,"txid","txidind",coin777_txidind));
}

int32_t ramchain_addrind(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson)
{
    return(ramchain_rawind(retbuf,maxlen,coin,ramchain,argjson,"addr","addrind",coin777_addrind));
}

int32_t ramchain_scriptind(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson)
{
    //return(ramchain_rawind(retbuf,maxlen,coin,ramchain,argjson,"script","scriptind",coin777_scriptind));
    return(0);
}

int32_t ramchain_string(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson,char *field,char *strname,int32_t (*coin_strfuncp)(struct coin777 *coin,char *str,int32_t max,uint32_t rawind,uint32_t addrind))
{
    char str[8192]; uint32_t rawind;
    if ( (rawind= (uint32_t)get_API_int(cJSON_GetObjectItem(argjson,field),0)) != 0 )
    {
        if ( (*coin_strfuncp)(coin,str,sizeof(str),rawind,(uint32_t)get_API_int(cJSON_GetObjectItem(argjson,field),0)) == 0 )
        {
            sprintf(retbuf,"{\"result\":\"success\",\"coin\":\"%s\",\"%s\":\"%u\",\"%s\":\"%s\"}",coin->name,field,rawind,strname,str);
        } else sprintf(retbuf,"{\"error\":\"cant find %s\",\"coin\":\"%s\"}",coin->name,field);
    }
    if ( retbuf[0] == 0 )
        sprintf(retbuf,"{\"error\":\"no %s\",\"coin\":\"%s\"}",field,coin->name);
    return(-1);
}

int32_t ramchain_txid(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson)
{
    return(ramchain_string(retbuf,maxlen,coin,ramchain,argjson,"txidind","txid",coin777_txidstr));
}

int32_t ramchain_coinaddr(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson)
{
    return(ramchain_string(retbuf,maxlen,coin,ramchain,argjson,"addrind","coinaddr",coin777_coinaddr));
}

int32_t ramchain_script(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson)
{
    return(ramchain_string(retbuf,maxlen,coin,ramchain,argjson,"script","script",coin777_scriptstr));
}

struct ledger_addrinfo *ramchain_addrinfo(char *field,char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson)
{
    char *coinaddr=0; uint32_t addrind;//,firstblocknum; struct ledger_addrinfo *addrinfo;
    if ( (addrind= (uint32_t)cJSON_GetObjectItem(argjson,"addrind")) == 0 && (coinaddr= cJSON_str(cJSON_GetObjectItem(argjson,"addr"))) != 0 )
        strcpy(field,coinaddr);
    else if ( addrind != 0 )
        sprintf(field,"addrind.%u",addrind);
    else
    {
        sprintf(retbuf,"{\"error\":\"invalid addr or addrind\",\"coin\":\"%s\"}",coin->name);
        return(0);
    }
    printf("coin_addrinfo not coded yet\n"); //getchar();
    //if ( (addrinfo= ledger_addrinfo(&firstblocknum,ramchain->activeledger,coinaddr,addrind)) == 0 )
    //    sprintf(retbuf,"{\"error\":\"cant find %s\",\"coin\":\"%s\"}",field,ramchain->name);
    return(0);//addrinfo);
}

int32_t ramchain_balance(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson)
{
    struct ledger_addrinfo *addrinfo; char field[256];
    if ( (addrinfo= ramchain_addrinfo(field,retbuf,maxlen,coin,ramchain,argjson)) == 0 )
        return(-1);
    //sprintf(retbuf,"{\"result\":\"success\",\"coin\":\"%s\",\"addr\":%s,\"balance\":%.8f}",ramchain->name,field,dstr(addrinfo->balance));
    return(0);
}

struct addrtx_info *coin777_acctunspents(uint64_t *sump,int32_t *nump,struct coin777 *coin,uint32_t *addrinds,int32_t num)
{
    uint64_t balance; int32_t i,j,n; struct coin777_Lentry L; struct addrtx_info *unspents,*allunspents = 0;
    *sump = *nump = 0;
    for (i=0; i<num; i++)
    {
        if ( coin777_RWmmap(0,&L,coin,&coin->ramchain.ledger,addrinds[i]) == 0 && (unspents= coin777_compact(1,&balance,&n,coin,addrinds[i],&L)) != 0 )
        {
            for (j=0; j<n; j++)
                unspents[j].spendind = addrinds[i];
            allunspents = realloc(allunspents,sizeof(*allunspents) * (n + (*nump)));
            memcpy(&allunspents[*nump],unspents,n * sizeof(*unspents));
            free(unspents);
            (*sump) += balance;
            (*nump) += n;
        }
    }
    return(allunspents);
}

uint32_t *conv_addrjson(int32_t *nump,struct coin777 *coin,cJSON *addrjson)
{
    uint32_t firstblocknum,*addrinds = 0; int32_t i;
    if ( is_cJSON_String(addrjson) != 0 )
    {
        *nump = 1;
        addrinds = malloc(sizeof(*addrinds));
        addrinds[0] = coin777_addrind(&firstblocknum,coin,cJSON_str(addrjson));
    }
    else if ( is_cJSON_Array(addrjson) != 0 && (*nump= cJSON_GetArraySize(addrjson)) > 0 )
    {
        addrinds = calloc(*nump,sizeof(*addrinds));
        for (i=0; i<*nump; i++)
            addrinds[i] = coin777_addrind(&firstblocknum,coin,cJSON_str(cJSON_GetArrayItem(addrjson,i)));
    }
    return(addrinds);
}

int32_t ramchain_unspents(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson)
{
    //int32_t ledger_unspentmap(char *txidstr,struct ledger_info *ledger,uint32_t unspentind);
  //struct ledger_addrinfo { uint64_t balance; uint32_t firstblocknum,count:28,notify:1,pending:1,MGW:1,dirty:1; struct unspentmap unspents[]; };
    /*struct ledger_addrinfo *addrinfo; cJSON *json,*array,*item; uint64_t sum = 0; int32_t i,vout,verbose; char *jsonstr,script[8193],txidstr[256],field[256];
    if ( (addrinfo= ramchain_addrinfo(field,retbuf,maxlen,coin,ramchain,argjson)) == 0 )
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
    strncpy(retbuf,jsonstr,maxlen-1), retbuf[maxlen-1] = 0, free(jsonstr);*/
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

int32_t ramchain_func(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson,char *method)
{
    int32_t i; void *ptr;
    int32_t (*funcp)(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson);
    for (i=0; i<(int32_t)(sizeof(ramchain_funcs)/sizeof(*ramchain_funcs)); i++)
    {
        if ( strcmp(method,ramchain_funcs[i][0]) == 0 )
        {
            ptr = (void *)ramchain_funcs[i][1];
            memcpy(&funcp,&ptr,sizeof(funcp));
            return((*funcp)(retbuf,maxlen,coin,ramchain,argjson));
        }
    }
    return(-1);
}
#endif

#endif
#endif
#include <stdint.h>
extern int32_t Debuglevel;
