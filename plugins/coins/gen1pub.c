//
//  gen1pub.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_gen1pub_h
#define crypto777_gen1pub_h
#include <stdio.h>
#include <stdint.h>
#include "cJSON.h"
#include "system777.c"
#include "coins777.c"

char *_get_transaction(char *coinstr,char *serverport,char *userpass,char *txidstr);
char *_get_blockhashstr(char *coinstr,char *serverport,char *userpass,uint32_t blocknum);
cJSON *_get_blockjson(uint32_t *heightp,char *coinstr,char *serverport,char *userpass,char *blockhashstr,uint32_t blocknum);
uint32_t _get_RTheight(float *lastmillip,char *coinstr,char *serverport,char *userpass,int32_t current_RTblocknum);

#endif
#else
#ifndef crypto777_gen1pub_c
#define crypto777_gen1pub_c

#ifndef crypto777_gen1pub_h
#define DEFINES_ONLY
#include "gen1pub.c"
#undef DEFINES_ONLY
#endif

char *_get_transaction(char *coinstr,char *serverport,char *userpass,char *txidstr)
{
    char *rawtransaction=0,txid[4096];
    sprintf(txid,"[\"%s\", 1]",txidstr);
    //printf("get_transaction.(%s)\n",txidstr);
    rawtransaction = bitcoind_passthru(coinstr,serverport,userpass,"getrawtransaction",txid);
    return(rawtransaction);
}

char *_get_blockhashstr(char *coinstr,char *serverport,char *userpass,uint32_t blocknum)
{
    char numstr[128],*blockhashstr=0;
    sprintf(numstr,"%u",blocknum);
    blockhashstr = bitcoind_passthru(coinstr,serverport,userpass,"getblockhash",numstr);
    if ( blockhashstr == 0 || blockhashstr[0] == 0 )
    {
        printf("couldnt get blockhash for %u\n",blocknum);
        if ( blockhashstr != 0 )
            free(blockhashstr);
        return(0);
    }
    return(blockhashstr);
}

cJSON *_get_blockjson(uint32_t *heightp,char *coinstr,char *serverport,char *userpass,char *blockhashstr,uint32_t blocknum)
{
    cJSON *json = 0;
    int32_t flag = 0;
    char buf[1024],*blocktxt = 0;
    if ( blockhashstr == 0 )
        blockhashstr = _get_blockhashstr(coinstr,serverport,userpass,blocknum), flag = 1;
    if ( blockhashstr != 0 )
    {
        sprintf(buf,"\"%s\"",blockhashstr);
        //printf("get_blockjson.(%d %s)\n",blocknum,blockhashstr);
        blocktxt = bitcoind_passthru(coinstr,serverport,userpass,"getblock",buf);
        if ( blocktxt != 0 && blocktxt[0] != 0 && (json= cJSON_Parse(blocktxt)) != 0 && heightp != 0 )
            *heightp = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"height"),0xffffffff);
        if ( flag != 0 && blockhashstr != 0 )
            free(blockhashstr);
        if ( blocktxt != 0 )
            free(blocktxt);
    }
    return(json);
}

uint32_t _get_RTheight(float *lastmillip,char *coinstr,char *serverport,char *userpass,int32_t current_RTblocknum)
{
    char *retstr;
    cJSON *json;
    uint32_t height = 0;
    if ( milliseconds() > (*lastmillip + 1000) )
    {
        //printf("RTheight.(%s) (%s)\n",ram->name,ram->serverport);
        retstr = bitcoind_passthru(coinstr,serverport,userpass,"getinfo","");
        if ( retstr != 0 )
        {
            if ( (json= cJSON_Parse(retstr)) != 0 )
            {
                height = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"blocks"),0);
                free_json(json);
                *lastmillip = milliseconds();
            }
            free(retstr);
        }
    } else height = current_RTblocknum;
    return(height);
}

#endif
#endif
