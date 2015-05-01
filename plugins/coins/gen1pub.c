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
#include "gen1.c"
#include "coins777.c"


//char *_get_transaction(char *coinstr,char *serverport,char *userpass,char *txidstr);
uint64_t ram_verify_txstillthere(char *coinstr,char *serverport,char *userpass,char *txidstr,int32_t vout);
//char *_get_blockhashstr(char *coinstr,char *serverport,char *userpass,uint32_t blocknum);
//cJSON *_get_blockjson(uint32_t *heightp,char *coinstr,char *serverport,char *userpass,char *blockhashstr,uint32_t blocknum);
uint32_t _get_RTheight(double *lastmillip,char *coinstr,char *serverport,char *userpass,int32_t current_RTblocknum);

#endif
#else
#ifndef crypto777_gen1pub_c
#define crypto777_gen1pub_c

#ifndef crypto777_gen1pub_h
#define DEFINES_ONLY
#include "gen1pub.c"
#undef DEFINES_ONLY
#endif

uint32_t _get_RTheight(double *lastmillip,char *coinstr,char *serverport,char *userpass,int32_t current_RTblocknum)
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

char *_get_transaction(char *coinstr,char *serverport,char *userpass,char *txidstr)
{
    char *rawtransaction=0,txid[4096];
    sprintf(txid,"[\"%s\", 1]",txidstr);
    //printf("get_transaction.(%s)\n",txidstr);
    rawtransaction = bitcoind_passthru(coinstr,serverport,userpass,"getrawtransaction",txid);
    return(rawtransaction);
}

uint64_t ram_verify_txstillthere(char *coinstr,char *serverport,char *userpass,char *txidstr,int32_t vout)
{
    char *retstr = 0;
    cJSON *txjson,*voutsobj;
    int32_t numvouts;
    uint64_t value = 0;
    if ( (retstr= _get_transaction(coinstr,serverport,userpass,txidstr)) != 0 )
    {
        if ( (txjson= cJSON_Parse(retstr)) != 0 )
        {
            if ( (voutsobj= cJSON_GetObjectItem(txjson,"vout")) != 0 && is_cJSON_Array(voutsobj) != 0 && (numvouts= cJSON_GetArraySize(voutsobj)) > 0  && vout < numvouts )
                value = conv_cJSON_float(cJSON_GetArrayItem(voutsobj,vout),"value");
            free_json(txjson);
        } else printf("update_txid_infos parse error.(%s)\n",retstr);
        free(retstr);
    } else printf("error getting.(%s)\n",txidstr);
    return(value);
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

void _set_string(char type,char *dest,char *src,long max)
{
    if ( src == 0 || src[0] == 0 )
        sprintf(dest,"ffff");
    else if ( strlen(src) < max-1 )
        strcpy(dest,src);
    else sprintf(dest,"nonstandard");
}

uint64_t rawblock_txvouts(struct rawblock *raw,struct rawtx *tx,char *coinstr,char *serverport,char *userpass,cJSON *voutsobj)
{
    cJSON *item;
    uint64_t value,total = 0;
    struct rawvout *v;
    int32_t i,numvouts = 0;
    char coinaddr[8192],script[8192];
    tx->firstvout = raw->numrawvouts;
    if ( voutsobj != 0 && is_cJSON_Array(voutsobj) != 0 && (numvouts= cJSON_GetArraySize(voutsobj)) > 0 && tx->firstvout+numvouts < MAX_BLOCKTX )
    {
        for (i=0; i<numvouts; i++,raw->numrawvouts++)
        {
            item = cJSON_GetArrayItem(voutsobj,i);
            value = conv_cJSON_float(item,"value");
            v = &raw->voutspace[raw->numrawvouts];
            memset(v,0,sizeof(*v));
            v->value = value;
            total += value;
            _extract_txvals(coinaddr,script,1,item); // default to nohexout
            _set_string('a',v->coinaddr,coinaddr,sizeof(v->coinaddr));
            _set_string('s',v->script,script,sizeof(v->script));
        }
    } else printf("error with vouts\n");
    tx->numvouts = numvouts;
    return(total);
}

int32_t rawblock_txvins(struct rawblock *raw,struct rawtx *tx,char *coinstr,char *serverport,char *userpass,cJSON *vinsobj)
{
    cJSON *item;
    struct rawvin *v;
    int32_t i,numvins = 0;
    char txidstr[8192],coinbase[8192];
    tx->firstvin = raw->numrawvins;
    if ( vinsobj != 0 && is_cJSON_Array(vinsobj) != 0 && (numvins= cJSON_GetArraySize(vinsobj)) > 0 && tx->firstvin+numvins < MAX_BLOCKTX )
    {
        for (i=0; i<numvins; i++,raw->numrawvins++)
        {
            item = cJSON_GetArrayItem(vinsobj,i);
            if ( numvins == 1  )
            {
                copy_cJSON(coinbase,cJSON_GetObjectItem(item,"coinbase"));
                if ( strlen(coinbase) > 1 )
                    return(0);
            }
            copy_cJSON(txidstr,cJSON_GetObjectItem(item,"txid"));
            v = &raw->vinspace[raw->numrawvins];
            memset(v,0,sizeof(*v));
            v->vout = (int)get_cJSON_int(item,"vout");
            _set_string('t',v->txidstr,txidstr,sizeof(v->txidstr));
        }
    } else printf("error with vins\n");
    tx->numvins = numvins;
    return(numvins);
}

uint64_t rawblock_txidinfo(struct rawblock *raw,struct rawtx *tx,char *coinstr,char *serverport,char *userpass,int32_t txind,char *txidstr)
{
    char *retstr = 0;
    cJSON *txjson;
    uint64_t total = 0;
    if ( strlen(txidstr) < sizeof(tx->txidstr)-1 )
        strcpy(tx->txidstr,txidstr);
    tx->numvouts = tx->numvins = 0;
    if ( (retstr= _get_transaction(coinstr,serverport,userpass,txidstr)) != 0 )
    {
        if ( (txjson= cJSON_Parse(retstr)) != 0 )
        {
            rawblock_txvins(raw,tx,coinstr,serverport,userpass,cJSON_GetObjectItem(txjson,"vin"));
            total = rawblock_txvouts(raw,tx,coinstr,serverport,userpass,cJSON_GetObjectItem(txjson,"vout"));
            free_json(txjson);
        } else printf("update_txid_infos parse error.(%s)\n",retstr);
        free(retstr);
    } else printf("error getting.(%s)\n",txidstr);
    //printf("tx.%d: (%s) numvins.%d numvouts.%d (raw %d %d)\n",txind,tx->txidstr,tx->numvins,tx->numvouts,raw->numrawvins,raw->numrawvouts);
    return(total);
}

cJSON *rawblock_txarray(uint32_t *blockidp,int32_t *numtxp,cJSON *blockjson)
{
    cJSON *txarray = 0;
    if ( blockjson != 0 )
    {
        *blockidp = (uint32_t)get_API_int(cJSON_GetObjectItem(blockjson,"height"),0);
        txarray = cJSON_GetObjectItem(blockjson,"tx");
        *numtxp = cJSON_GetArraySize(txarray);
    }
    return(txarray);
}

void ram_clear_rawblock(struct rawblock *raw,int32_t totalflag)
{
    int32_t i;
    if ( totalflag != 0 )
        memset(raw,0,sizeof(*raw));
    else
    {
        raw->numtx = raw->numrawvins = raw->numrawvouts = 0;
        for (i=0; i<MAX_BLOCKTX; i++)
        {
            raw->txspace[i].txidstr[0] = 0;
            raw->vinspace[i].txidstr[0] = 0;
            raw->voutspace[i].script[0] = 0;
            raw->voutspace[i].coinaddr[0] = 0;
        }
    }
}

void rawblock_patch(struct rawblock *raw)
{
    int32_t txind,numtx,firstvin,firstvout;
    struct rawtx *tx;
    firstvin = firstvout = 0;
    if ( (numtx= raw->numtx) != 0 )
    {
        for (txind=0; txind<numtx; txind++)
        {
            tx = &raw->txspace[txind];
            tx->firstvin = firstvin, firstvin += tx->numvins;
            tx->firstvout = firstvout, firstvout += tx->numvouts;
        }
    }
    raw->numrawvouts = firstvout;
    raw->numrawvins = firstvin;
}

int32_t rawblock_load(struct rawblock *raw,char *coinstr,char *serverport,char *userpass,uint32_t blocknum)
{
    char txidstr[8192],mintedstr[8192];
    cJSON *json,*txobj;
    uint32_t blockid;
    int32_t txind,n;
    uint64_t total = 0;
    ram_clear_rawblock(raw,1);
    //raw->blocknum = blocknum;
    //printf("_get_blockinfo.%d\n",blocknum);
    raw->minted = raw->numtx = raw->numrawvins = raw->numrawvouts = 0;
    if ( (json= _get_blockjson(0,coinstr,serverport,userpass,0,blocknum)) != 0 )
    {
        raw->blocknum = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"height"),0);
        copy_cJSON(mintedstr,cJSON_GetObjectItem(json,"mint"));
        if ( mintedstr[0] != 0 )
            raw->minted = (uint64_t)(atof(mintedstr) * SATOSHIDEN);
        if ( (txobj= rawblock_txarray(&blockid,&n,json)) != 0 && blockid == blocknum && n < MAX_BLOCKTX )
        {
            for (txind=0; txind<n; txind++)
            {
                copy_cJSON(txidstr,cJSON_GetArrayItem(txobj,txind));
                //printf("block.%d txind.%d TXID.(%s)\n",blocknum,txind,txidstr);
                total += rawblock_txidinfo(raw,&raw->txspace[raw->numtx++],coinstr,serverport,userpass,txind,txidstr);
            }
        } else printf("error _get_blocktxarray for block.%d got %d, n.%d vs %d\n",blocknum,blockid,n,MAX_BLOCKTX);
        if ( raw->minted == 0 )
            raw->minted = total;
        free_json(json);
    } else printf("get_blockjson error parsing.(%s)\n",txidstr);
    //printf("BLOCK.%d: block.%d numtx.%d minted %.8f rawnumvins.%d rawnumvouts.%d\n",blocknum,raw->blocknum,raw->numtx,dstr(raw->minted),raw->numrawvins,raw->numrawvouts);
    rawblock_patch(raw);
    return(raw->numtx);
}
#endif
#endif
