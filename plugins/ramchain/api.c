//
//  ramchainapi.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_ramchainapi_h
#define crypto777_ramchainapi_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"
#include "huff.c"
#include "bitcoind.c"
#include "msig.c"
#include "ramchain.c"

char *ramstatus(char *origargstr,char *sender,char *previpaddr,char *coin);
char *rampyramid(char *NXTaddr,char *origargstr,char *sender,char *previpaddr,char *coin,uint32_t blocknum,char *typestr);
char *ramstring(char *origargstr,char *sender,char *previpaddr,char *coin,char *typestr,uint32_t rawind);
char *ramrawind(char *origargstr,char *sender,char *previpaddr,char *coin,char *typestr,char *str);
char *ramblock(char *NXTaddr,char *origargstr,char *sender,char *previpaddr,char *coin,uint32_t blocknum);
char *ramcompress(char *origargstr,char *sender,char *previpaddr,char *coin,char *ramhex);
char *ramexpand(char *origargstr,char *sender,char *previpaddr,char *coin,char *bitstream);
char *ramscript(char *origargstr,char *sender,char *previpaddr,char *coin,char *txidstr,int32_t tx_vout,struct address_entry *bp);
char *ramtxlist(char *origargstr,char *sender,char *previpaddr,char *coin,char *coinaddr,int32_t unspentflag);
char *ramrichlist(char *origargstr,char *sender,char *previpaddr,char *coin,int32_t numwhales,int32_t recalcflag);
char *rambalances(char *origargstr,char *sender,char *previpaddr,char *coin,char **coins,double *rates,char ***coinaddrs,int32_t numcoins);
char *ramaddrlist(char *origargstr,char *sender,char *previpaddr,char *coin);

struct ramchain_info *get_ramchain_info(char *coinstr);

#endif
#else
#ifndef crypto777_ramchainapi_c
#define crypto777_ramchainapi_c

#ifndef crypto777_ramchainapi_h
#define DEFINES_ONLY
#include __BASE_FILE__
#undef DEFINES_ONLY
#endif

uint64_t ram_unspent_json(cJSON **arrayp,char *destcoin,double rate,char *coin,char *addr)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    uint64_t unspent = 0;
    cJSON *item;
    if ( ram != 0 )
    {
        if ( (unspent= ram_calc_unspent(0,0,0,ram,addr,0)) != 0 && arrayp != 0 )
        {
            item = cJSON_CreateObject();
            cJSON_AddItemToObject(item,addr,cJSON_CreateNumber(dstr(unspent)));
            if ( rate != 0. )
            {
                unspent *= rate;
                cJSON_AddItemToObject(item,destcoin,cJSON_CreateNumber(dstr(unspent)));
            }
            cJSON_AddItemToArray(*arrayp,item);
        }
        else if ( rate != 0. )
            unspent *= rate;
    }
    return(unspent);
}

static int _decreasing_double_cmp(const void *a,const void *b)
{
#define double_a (*(double *)a)
#define double_b (*(double *)b)
	if ( double_b > double_a )
		return(1);
	else if ( double_b < double_a )
		return(-1);
	return(0);
#undef double_a
#undef double_b
}

/*static int _decreasing_uint64_cmp(const void *a,const void *b)
 {
 #define uint64_a (*(uint64_t *)a)
 #define uint64_b (*(uint64_t *)b)
 if ( uint64_b > uint64_a )
 return(1);
 else if ( uint64_b < uint64_a )
 return(-1);
 return(0);
 #undef uint64_a
 #undef uint64_b
 }*/

char **ram_getallstrs(int32_t *numstrsp,struct ramchain_info *ram)
{
    char **strs = 0;
    uint64_t varint;
    int32_t i = 0;
    struct ramchain_hashtable *hash;
    struct ramchain_hashptr *hp,*tmp;
    hash = ram_gethash(ram,'a');
    *numstrsp = HASH_COUNT(hash->table);
    strs = calloc(*numstrsp,sizeof(*strs));
    HASH_ITER(hh,hash->table,hp,tmp)
    strs[i++] = (char *)((long)hp->hh.key + hdecode_varint(&varint,hp->hh.key,0,9));
    if ( i != *numstrsp )
        printf("ram_getalladdrs HASH_COUNT.%d vs i.%d\n",*numstrsp,i);
    return(strs);
}

struct ramchain_hashptr **ram_getallstrptrs(int32_t *numstrsp,struct ramchain_info *ram,char type)
{
    struct ramchain_hashptr **strs = 0;
    struct ramchain_hashtable *hash;
    hash = ram_gethash(ram,type);
    *numstrsp = hash->ind;
    strs = calloc(*numstrsp+1,sizeof(*strs));
    memcpy(strs,hash->ptrs+1,(*numstrsp) * sizeof(*strs));
    return(strs);
}

char *ramaddrlist(char *origargstr,char *sender,char *previpaddr,char *coin)
{
    cJSON *json,*array,*item;
    char retbuf[1024],coinaddr[MAX_JSON_FIELD],account[MAX_JSON_FIELD],*retstr = 0;
    uint64_t value;
    uint32_t rawind;
    struct ramchain_hashtable *addrhash;
    struct ramchain_hashptr *addrptr;
    struct rampayload *payloads;
    int32_t i,j,k,z,n,m,num,numpayloads,errs,ismine,ismultisig,count = 0;
    struct ramchain_info *ram = get_ramchain_info(coin);
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    errs = 0;
    addrhash = ram_gethash(ram,'a');
    if ( (json= _get_localaddresses(ram)) != 0 )
    {
        if ( is_cJSON_Array(json) != 0 && (n= cJSON_GetArraySize(json)) > 0 )
        {
            for (i=0; i<n; i++)
            {
                if ( (array= cJSON_GetArrayItem(json,i)) != 0 && is_cJSON_Array(array) != 0 && (m= cJSON_GetArraySize(array)) > 0 )
                {
                    for (j=0; j<m; j++)
                    {
                        item = cJSON_GetArrayItem(array,j);
                        if ( (item= cJSON_GetArrayItem(array,j)) != 0 && is_cJSON_Array(item) != 0 && (num= cJSON_GetArraySize(item)) > 0 )
                        {
                            value = coinaddr[0] = account[0] = 0;
                            for (k=0; k<num; k++)
                            {
                                if ( k == 0 )
                                    copy_cJSON(coinaddr,cJSON_GetArrayItem(item,0));
                                else if ( k == 1 )
                                    value = (SATOSHIDEN * get_API_float(cJSON_GetArrayItem(item,1)));
                                else if ( k == 2 )
                                    copy_cJSON(account,cJSON_GetArrayItem(item,2));
                                else printf("ramaddrlist unexpected item array size: %d\n",num);
                            }
                            rawind = 0;
                            if ( coinaddr[0] != 0 && (rawind= ram_addrind(ram,coinaddr)) != 0 )
                            {
                                if ( (payloads= ram_addrpayloads(&addrptr,&numpayloads,ram,coinaddr)) != 0 && addrptr != 0 && numpayloads > 0 )
                                {
                                    addrptr->mine = 1;
                                    printf("%d: (%s).%-6d %.8f %.8f (%s) %s\n",count,coinaddr,rawind,dstr(addrptr->unspent),dstr(value),account,(addrptr->unspent != value) ? "STAKING": "");
                                    errs += (addrptr->unspent != value);
                                    count++;
                                } else printf("ramaddrlist error finding rawind.%d\n",rawind);
                            } else printf("ramaddrlist no coinaddr for item or null rawind.%d\n",rawind);
                        } else printf("ramaddrlist unexpected item not array\n");
                    }
                } else printf("ramaddrlist unexpected array not array\n");
            }
        } else printf("ramaddrlist unexpected json not array\n");
        free_json(json);
        n = m = 0;
        for (z=1; z<=addrhash->ind; z++)
        {
            if ( (addrptr= addrhash->ptrs[z]) != 0 && (addrptr->multisig != 0 || addrptr->mine != 0) && ram_decode_hashdata(coinaddr,'a',addrptr->hh.key) != 0 )
            {
                if ( 0 && addrptr->mine == 0 && addrptr->verified == 0 )
                {
                    addrptr->verified = _verify_coinaddress(account,&ismultisig,&ismine,ram,coinaddr);
                    if ( ismultisig != 0 )
                        addrptr->multisig = 1;
                    if ( ismine != 0 )
                        addrptr->mine = 1;
                }
                if ( addrptr->mine != 0 )
                    m++;
                if ( addrptr->multisig != 0 || addrptr->mine != 0 )
                    printf("n.%d check.(%s) account.(%s) multisig.%d nonstandard.%d mine.%d verified.%d\n",n,coinaddr,account,addrptr->multisig,addrptr->nonstandard,addrptr->mine,addrptr->verified), n++;
            }
        }
        sprintf(retbuf,"{\"result\":\"addrlist\",\"multisig\":%d,\"mine\":%d,\"total\":%d}",n,m,count);
        retstr = clonestr(retbuf);
    } else retstr = clonestr("{\"error\":\"ramaddrlist no data\"}");
    return(retstr);
}

cJSON *ram_address_entry_json(struct address_entry *bp)
{
    cJSON *array = cJSON_CreateArray();
    cJSON_AddItemToArray(array,cJSON_CreateNumber(bp->blocknum));
    cJSON_AddItemToArray(array,cJSON_CreateNumber(bp->txind));
    cJSON_AddItemToArray(array,cJSON_CreateNumber(bp->v));
    return(array);
}

void ram_payload_json(cJSON *json,struct rampayload *payload,int32_t spentflag)
{
    cJSON_AddItemToObject(json,"n",cJSON_CreateNumber(payload->B.v));
    cJSON_AddItemToObject(json,"value",cJSON_CreateNumber(dstr(payload->value)));
    if ( payload->B.isinternal != 0 )
        cJSON_AddItemToObject(json,"MGWinternal",cJSON_CreateNumber(1));
    if ( spentflag != 0 && payload->B.spent != 0 )
        cJSON_AddItemToObject(json,"spent",ram_address_entry_json(&payload->spentB));
}

cJSON *ram_addrpayload_json(struct ramchain_info *ram,struct rampayload *payload)
{
    char txidstr[8192];
    ram_txid(txidstr,ram,payload->otherind);
    cJSON *json = cJSON_CreateObject();
    if ( payload->pendingdeposit != 0 )
        cJSON_AddItemToObject(json,"pendingdeposit",cJSON_CreateNumber(1));
    cJSON_AddItemToObject(json,"txid",cJSON_CreateString(txidstr));
    cJSON_AddItemToObject(json,"txid_rawind",cJSON_CreateNumber(payload->otherind));
    cJSON_AddItemToObject(json,"txout",ram_address_entry_json(&payload->B));
    cJSON_AddItemToObject(json,"scriptind",cJSON_CreateNumber(payload->extra));
    ram_payload_json(json,payload,1);
    return(json);
}

#define ram_rawtx(raw,txind) (((txind) < (raw)->numtx) ? &(raw)->txspace[txind] : 0)

struct rawvout *ram_rawvout(struct rawblock *raw,int32_t txind,int32_t v)
{
    struct rawtx *tx;
    if ( txind < raw->numtx )
    {
        if ( (tx= ram_rawtx(raw,txind)) != 0 )
            return(&raw->voutspace[tx->firstvout + v]);
    }
    return(0);
}

struct rawvin *ram_rawvin(struct rawblock *raw,int32_t txind,int32_t v)
{
    struct rawtx *tx;
    if ( txind < raw->numtx )
    {
        if ( (tx= ram_rawtx(raw,txind)) != 0 )
            return(&raw->vinspace[tx->firstvin + v]);
    }
    return(0);
}

cJSON *ram_txpayload_json(struct ramchain_info *ram,struct rampayload *txpayload,char *spent_txidstr,int32_t spent_vout)
{
    cJSON *item;
    HUFF *hp;
    int32_t datalen;
    char coinaddr[8192],scriptstr[8192];
    struct rawvin *vi;
    struct rawtx *tx;
    struct ramchain_hashptr *addrptr;
    struct rampayload *addrpayload;
    ram_addr(coinaddr,ram,txpayload->otherind);
    cJSON *json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"address",cJSON_CreateString(coinaddr));
    cJSON_AddItemToObject(json,"addr_rawind",cJSON_CreateNumber(txpayload->otherind));
    cJSON_AddItemToObject(json,"addr_txlisti",cJSON_CreateNumber(txpayload->extra));
    ram_payload_json(json,txpayload,0);
    if ( txpayload->spentB.spent != 0 )
    {
        item = cJSON_CreateObject();
        cJSON_AddItemToObject(item,"height",cJSON_CreateNumber(txpayload->spentB.blocknum));
        cJSON_AddItemToObject(item,"txind",cJSON_CreateNumber(txpayload->spentB.txind));
        cJSON_AddItemToObject(item,"vin",cJSON_CreateNumber(txpayload->spentB.v));
        if ( (hp= ram->blocks.hps[txpayload->spentB.blocknum]) != 0 && (datalen= ram_expand_bitstream(0,ram->R,ram,hp)) > 0 )
        {
            if ( (vi= ram_rawvin(ram->R,txpayload->spentB.txind,txpayload->spentB.v)) != 0 )
            {
                if ( strcmp(vi->txidstr,spent_txidstr) != 0 )
                    cJSON_AddItemToObject(item,"vin_txid_error",cJSON_CreateString(vi->txidstr));
                if ( vi->vout != spent_vout )
                    cJSON_AddItemToObject(item,"vin_error",cJSON_CreateNumber(spent_vout));
            }
            if ( (tx= ram_rawtx(ram->R,txpayload->spentB.txind)) != 0 )
                cJSON_AddItemToObject(item,"txid",cJSON_CreateString(tx->txidstr));
        }
        cJSON_AddItemToObject(json,"spent",item);
    }
    else
    {
        if ( (addrpayload= ram_getpayloadi(&addrptr,ram,'a',txpayload->otherind,txpayload->extra)) != 0 )
        {
            ram_script(scriptstr,ram,addrpayload->extra);
            cJSON_AddItemToObject(json,"script",cJSON_CreateString(scriptstr));
            cJSON_AddItemToObject(json,"script_rawind",cJSON_CreateNumber(addrpayload->extra));
        }
    }
    return(json);
}

cJSON *ram_coinaddr_json(struct ramchain_info *ram,char *coinaddr,int32_t unspentflag,int32_t truncateflag,int32_t searchperms)
{
    char permstr[MAX_JSON_FIELD];
    int64_t total = 0;
    cJSON *json = 0,*array = 0;
    int32_t i,n,numpayloads;
    struct ramchain_hashptr *addrptr;
    struct rampayload *payloads;
    json = cJSON_CreateObject();
    if ( (payloads= ram_addrpayloads(&addrptr,&numpayloads,ram,coinaddr)) != 0 && addrptr != 0 && numpayloads > 0 )
    {
        if ( truncateflag == 0 )
        {
            for (i=n=0; i<numpayloads; i++)
            {
                if ( unspentflag == 0 || payloads[i].B.spent == 0 )
                {
                    if ( payloads[i].B.spent == 0 && payloads[i].B.isinternal == 0 )
                    {
                        n++;
                        total += payloads[i].value;
                    }
                    if ( array == 0 )
                        array = cJSON_CreateArray();
                    cJSON_AddItemToArray(array,ram_addrpayload_json(ram,&payloads[i]));
                }
            }
            if ( array != 0 )
                cJSON_AddItemToObject(json,coinaddr,array);
            cJSON_AddItemToObject(json,"calc_numunspent",cJSON_CreateNumber(n));
            cJSON_AddItemToObject(json,"calc_unspent",cJSON_CreateNumber(dstr(total)));
        }
        cJSON_AddItemToObject(json,"numunspent",cJSON_CreateNumber(addrptr->numunspent));
        cJSON_AddItemToObject(json,"unspent",cJSON_CreateNumber(dstr(addrptr->unspent)));
    }
    cJSON_AddItemToObject(json,"numtx",cJSON_CreateNumber(numpayloads));
    cJSON_AddItemToObject(json,"rawind",cJSON_CreateNumber(addrptr->rawind));
    cJSON_AddItemToObject(json,"permind",cJSON_CreateNumber(addrptr->permind));
    if ( searchperms != 0 )
    {
        ram_searchpermind(permstr,ram,'a',addrptr->rawind);
        cJSON_AddItemToObject(json,"permstr",cJSON_CreateString(permstr));
    }
    cJSON_AddItemToObject(json,ram->name,cJSON_CreateString(coinaddr));
    cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(ram->srvNXTADDR));
    return(json);
}

char *ram_coinaddr_str(struct ramchain_info *ram,char *coinaddr,int32_t truncateflag,int32_t searchperms)
{
    cJSON *json;
    char retbuf[1024],*retstr;
    if ( coinaddr != 0 && coinaddr[0] != 0 && (json= ram_coinaddr_json(ram,coinaddr,1,truncateflag,searchperms)) != 0 )
    {
        retstr = cJSON_Print(json);
        free_json(json);
        return(retstr);
    }
    sprintf(retbuf,"{\"error\":\"no addr info\",\"coin\":\"%s\",\"addr\":\"%s\"}",ram->name,coinaddr);
    return(clonestr(retbuf));
}

char *ram_addr_json(struct ramchain_info *ram,uint32_t rawind,int32_t truncateflag)
{
    char hashstr[8193];
    ram_addr(hashstr,ram,rawind);
    printf("ram_addr_json(%d) -> (%s)\n",rawind,hashstr);
    return(ram_coinaddr_str(ram,hashstr,truncateflag,1));
}

char *ram_addrind_json(struct ramchain_info *ram,char *coinaddr,int32_t truncateflag)
{
    return(ram_coinaddr_str(ram,coinaddr,truncateflag,0));
}

cJSON *ram_txidstr_json(struct ramchain_info *ram,char *txidstr,int32_t truncateflag,int32_t searchperms)
{
    char permstr[MAX_JSON_FIELD];
    int64_t unspent = 0,total = 0;
    cJSON *json = 0,*array = 0;
    int32_t i,n,numpayloads;
    struct ramchain_hashptr *txptr;
    struct rampayload *txpayloads;
    json = cJSON_CreateObject();
    if ( (txpayloads= ram_txpayloads(&txptr,&numpayloads,ram,txidstr)) != 0 && txptr != 0 && numpayloads > 0 )
    {
        cJSON_AddItemToObject(json,"height",cJSON_CreateNumber(txpayloads->B.blocknum));
        cJSON_AddItemToObject(json,"txind",cJSON_CreateNumber(txpayloads->B.txind));
        cJSON_AddItemToObject(json,"numvouts",cJSON_CreateNumber(numpayloads));
        if ( truncateflag == 0 )
        {
            array = cJSON_CreateArray();
            for (i=n=0; i<numpayloads; i++)
            {
                total += txpayloads[i].value;
                if ( txpayloads[i].spentB.spent == 0 )
                    unspent += txpayloads[i].value;
                cJSON_AddItemToArray(array,ram_txpayload_json(ram,&txpayloads[i],txidstr,i));
            }
            cJSON_AddItemToObject(json,"vouts",array);
            cJSON_AddItemToObject(json,"total",cJSON_CreateNumber(dstr(total)));
            cJSON_AddItemToObject(json,"unspent",cJSON_CreateNumber(dstr(unspent)));
        }
    }
    cJSON_AddItemToObject(json,"rawind",cJSON_CreateNumber(txptr->rawind));
    cJSON_AddItemToObject(json,"permind",cJSON_CreateNumber(txptr->permind));
    cJSON_AddItemToObject(json,"txid",cJSON_CreateString(txidstr));
    if ( searchperms != 0 )
    {
        ram_searchpermind(permstr,ram,'t',txptr->rawind);
        cJSON_AddItemToObject(json,"permstr",cJSON_CreateString(permstr));
    }
    cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(ram->srvNXTADDR));
    return(json);
}

char *ram_txidstr(struct ramchain_info *ram,char *txidstr,int32_t truncateflag,int32_t searchperms)
{
    cJSON *json;
    char *retstr;
    if ( txidstr != 0 && txidstr[0] != 0 && (json= ram_txidstr_json(ram,txidstr,truncateflag,searchperms)) != 0 )
    {
        retstr = cJSON_Print(json);
        free_json(json);
        return(retstr);
    }
    return(clonestr("{\"error\":\"no txid info\"}"));
}

char *ram_txid_json(struct ramchain_info *ram,uint32_t rawind,int32_t truncateflag)
{
    char hashstr[8193];
    return(ram_txidstr(ram,ram_txid(hashstr,ram,rawind),truncateflag,1));
}

char *ram_txidind_json(struct ramchain_info *ram,char *txidstr,int32_t truncateflag)
{
    return(ram_txidstr(ram,txidstr,truncateflag,0));
}

char *ram_script_json(struct ramchain_info *ram,uint32_t rawind,int32_t truncateflag)
{
    char hashstr[8193],permstr[8193],retbuf[1024];
    ram_searchpermind(permstr,ram,'s',rawind);
    if ( ram_script(hashstr,ram,rawind) != 0 )
    {
        sprintf(retbuf,"{\"NXT\":\"%s\",\"result\":\"%u\",\"script\":\"%s\",\"rawind\":\"%u\",\"permstr\":\"%s\"}",ram->srvNXTADDR,rawind,hashstr,rawind,permstr);
        return(clonestr(retbuf));
    }
    return(clonestr("{\"error\":\"no script info\"}"));
}

char *ram_scriptind_json(struct ramchain_info *ram,char *str,int32_t truncateflag)
{
    char retbuf[1024];
    uint32_t rawind,permind;
    if ( (rawind= ram_scriptind_RO(&permind,ram,str)) != 0 )
    {
        sprintf(retbuf,"{\"NXT\":\"%s\",\"result\":\"%s\",\"rawind\":\"%u\",\"permind\":\"%u\"}",ram->srvNXTADDR,str,rawind,permind);
        return(clonestr(retbuf));
    }
    return(clonestr("{\"error\":\"no script info\"}"));
}

void ram_parse_snapshot(struct ramsnapshot *snap,cJSON *json)
{
    memset(snap,0,sizeof(*snap));
    snap->permoffset = (long)get_API_int(cJSON_GetObjectItem(json,"permoffset"),0);
    snap->addrind = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"addrind"),0);
    snap->addroffset = (long)get_API_int(cJSON_GetObjectItem(json,"addroffset"),0);
    snap->scriptind = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"scriptind"),0);
    snap->scriptoffset = (long)get_API_int(cJSON_GetObjectItem(json,"scriptoffset"),0);
    snap->txidind = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"txidind"),0);
    snap->txidoffset = (long)get_API_int(cJSON_GetObjectItem(json,"txidoffset"),0);
}

cJSON *ram_snapshot_json(struct ramsnapshot *snap)
{
    char numstr[64];
    cJSON *json = cJSON_CreateObject();
    sprintf(numstr,"%ld",snap->permoffset), cJSON_AddItemToObject(json,"permoffset",cJSON_CreateString(numstr));
    sprintf(numstr,"%u",snap->addrind), cJSON_AddItemToObject(json,"addrind",cJSON_CreateString(numstr));
    sprintf(numstr,"%ld",snap->addroffset), cJSON_AddItemToObject(json,"addroffset",cJSON_CreateString(numstr));
    sprintf(numstr,"%u",snap->scriptind), cJSON_AddItemToObject(json,"scriptind",cJSON_CreateString(numstr));
    sprintf(numstr,"%ld",snap->scriptoffset), cJSON_AddItemToObject(json,"scriptoffset",cJSON_CreateString(numstr));
    sprintf(numstr,"%u",snap->txidind), cJSON_AddItemToObject(json,"txidind",cJSON_CreateString(numstr));
    sprintf(numstr,"%ld",snap->txidoffset), cJSON_AddItemToObject(json,"txidoffset",cJSON_CreateString(numstr));
    return(json);
}

// >>>>>>>>>>>>>>  start external and API interface functions
char *ramstatus(char *origargstr,char *sender,char *previpaddr,char *coin)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    char retbuf[1024],*str;
    if ( ram == 0 || ram->MGWpingstr[0] == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    ram_setdispstr(retbuf,ram,ram->startmilli);
    str = stringifyM(retbuf);
    sprintf(retbuf,"{\"result\":\"MGWstatus\",%s\"ramchain\":%s}",ram->MGWpingstr,str);
    free(str);
    return(clonestr(retbuf));
}

int32_t ram_perm_sha256(bits256 *hashp,struct ramchain_info *ram,uint32_t blocknum,int32_t n)
{
    bits256 tmp;
    int32_t i;
    HUFF *hp,*permhp;
    memset(hashp->bytes,0,sizeof(*hashp));
    for (i=0; i<n; i++)
    {
        if ( (hp= ram->blocks.hps[blocknum+i]) == 0 )
            break;
        if ( (permhp = ram_conv_permind(ram->tmphp3,ram,hp,blocknum+i)) == 0 )
            break;
        calc_sha256cat(tmp.bytes,hashp->bytes,sizeof(*hashp),permhp->buf,hconv_bitlen(permhp->endpos)), *hashp = tmp;
    }
    return(i);
}

char *rampyramid(char *myNXTaddr,char *origargstr,char *sender,char *previpaddr,char *coin,uint32_t blocknum,char *typestr)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    char shastr[65],*hexstr,*retstr = 0;
    bits256 hash;
    HUFF *permhp,*hp,*newhp,**hpptr;
    cJSON *json;
    int32_t size,n;
    printf("rampyramid.%s (%s).%u permblocks.%d\n",coin,typestr,blocknum,ram->S.permblocks);
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    else if ( blocknum >= ram->maxblock )
        return(clonestr("{\"error\":\"blocknum too big\"}"));
    else if ( blocknum >= ram->S.permblocks )
        return(clonestr("{\"error\":\"blocknum past permblocks\"}"));
    if ( strcmp(typestr,"B64") == 0 )
    {
        n = 64;
        if ( (blocknum % n) != 0 )
            return(clonestr("{\"error\":\"B64 blocknum misaligned\"}"));
        hash = ram->snapshots[blocknum / n].hash;
    }
    else if ( strcmp(typestr,"B4096") == 0 )
    {
        n = 4096;
        if ( (blocknum % n) != 0 )
            return(clonestr("{\"error\":\"B4096 blocknum misaligned\"}"));
        //printf("rampyramid B4096 for blocknum.%d\n",blocknum);
        hash = ram->permhash4096[blocknum / n];
    }
    else
    {
        if ( (hp= ram->blocks.hps[blocknum]) == 0 )
        {
            if ( (hpptr= ram_get_hpptr(&ram->Vblocks,blocknum)) != 0 && (hp= *hpptr) != 0 )
            {
                if ( ram_expand_bitstream(0,ram->R3,ram,hp) > 0 )
                {
                    newhp = ram_makehp(ram->tmphp,'B',ram,ram->R3,blocknum);
                    if ( (hpptr= ram_get_hpptr(&ram->Bblocks,blocknum)) != 0 )
                        hp = ram->blocks.hps[blocknum] = *hpptr = newhp;
                } else hp = 0;
            }
        }
        if ( hp != 0 )
        {
            if ( (permhp= ram_conv_permind(ram->tmphp,ram,hp,blocknum)) != 0 )
            {
                size = hconv_bitlen(permhp->endpos);
                hexstr = calloc(1,size*2+1);
                init_hexbytes_noT(hexstr,permhp->buf,size);
                retstr = calloc(1,size*2+1+512);
                sprintf(retstr,"{\"NXT\":\"%s\",\"blocknum\":\"%d\",\"size\":\"%d\",\"data\":\"%s\"}",myNXTaddr,blocknum,size,hexstr);
                free(hexstr);
                return(retstr);
            } else return(clonestr("{\"error\":\"error doing ram_conv_permind\"}"));
        } else return(clonestr("{\"error\":\"no ramchain info for blocknum\"}"));
    }
    json = ram_snapshot_json(&ram->snapshots[blocknum/64]);
    init_hexbytes_noT(shastr,hash.bytes,sizeof(hash));
    cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(myNXTaddr));
    cJSON_AddItemToObject(json,"blocknum",cJSON_CreateNumber(blocknum));
    cJSON_AddItemToObject(json,"B",cJSON_CreateNumber(n));
    cJSON_AddItemToObject(json,"sha256",cJSON_CreateString(shastr));
    retstr = cJSON_Print(json);
    free_json(json);
    return(retstr);
}

void ram_setsnapshot(struct ramchain_info *ram,struct syncstate *sync,uint32_t blocknum,struct ramsnapshot *snap,uint64_t senderbits)
{
    int32_t i,num = (int32_t)(sizeof(sync->requested)/sizeof(*sync->requested));
    bits256 majority,zerokey;
    //fprintf(stderr,"ram_setsnapshot.%d\n",blocknum);
    for (i=0; i<num&&sync->requested[i]!=0; i++)
    {
        if ( senderbits == sync->requested[i] )
        {
            if ( (blocknum % 4096) != 0 )
                sync->snaps[i] = *snap;
            break;
        }
    }
    if ( sync->requested[i] == 0 )
    {
        sync->requested[i] = senderbits;
        if ( i == 0 )
        {
            if ( (blocknum % 4096) == 0 )
                ram->permhash4096[blocknum >> 12] = snap->hash;
            sync->majority = snap->hash;
        }
    }
    majority = sync->majority;
    memset(&zerokey,0,sizeof(zerokey));
    memset(&sync->minority,0,sizeof(sync->minority));
    sync->majoritybits = sync->minoritybits = 0;
    for (i=0; i<num&&sync->requested[i]!=0; i++)
    {
        //printf("check i.%d of %d: %d %d\n",i,num,sync->majoritybits,sync->minoritybits);
        if ( memcmp(sync->snaps[i].hash.bytes,majority.bytes,sizeof(majority)) == 0 )
            sync->majoritybits |= (1 << i);
        else
        {
            sync->minoritybits |= (1 << i);
            if ( memcmp(sync->snaps[i].hash.bytes,zerokey.bytes,sizeof(zerokey)) == 0 )
                sync->minority = sync->snaps[i].hash;
            else if ( memcmp(sync->snaps[i].hash.bytes,sync->minority.bytes,sizeof(sync->minority)) != 0 )
                printf("WARNING: third different hash for blocknum.%d\n",blocknum);
        }
    }
    //fprintf(stderr,"done ram_setsnapshot.%d\n",blocknum);
}

char *ramresponse(char *origargstr,char *sender,char *senderip,char *datastr)
{
    char origcmd[MAX_JSON_FIELD],coin[MAX_JSON_FIELD],permstr[MAX_JSON_FIELD],shastr[MAX_JSON_FIELD],*snapstr,*retstr = 0;
    cJSON *array,*json,*snapjson;
    uint8_t *data;
    bits256 hash;
    struct ramchain_info *ram;
    struct ramsnapshot snap;
    uint32_t blocknum,size,permind,i;
    int32_t format = 0,type = 0;
    permstr[0] = 0;
    //fprintf(stderr,"ramresponse\n");
    if ( (array= cJSON_Parse(origargstr)) != 0 )
    {
        if ( is_cJSON_Array(array) != 0 && cJSON_GetArraySize(array) == 2 )
        {
            retstr = cJSON_Print(array);
            json = cJSON_GetArrayItem(array,0);
            if ( datastr == 0 )
                datastr = cJSON_str(cJSON_GetObjectItem(json,"data"));
            copy_cJSON(origcmd,cJSON_GetObjectItem(json,"origcmd"));
            copy_cJSON(coin,cJSON_GetObjectItem(json,"coin"));
            ram = get_ramchain_info(coin);
            //printf("orig.(%s) ram.%p %s\n",origcmd,ram,coin);
            if ( strcmp(origcmd,"rampyramid") == 0 )
            {
                blocknum = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"blocknum"),0);
                if ( (format= (int32_t)get_API_int(cJSON_GetObjectItem(json,"B"),0)) == 0 )
                {
                    size = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"size"),0);
                    if ( size > 0 )
                    {
                        if ( datastr != 0 && strlen(datastr) == size*2 )
                        {
                            printf("PYRAMID.(%u %s).%d from NXT.(%s) ip.(%s)\n",blocknum,datastr,size,sender,senderip);
                            data = calloc(1,size);
                            decode_hex(data,size,datastr);
                            // update pyramid
                            free(data);
                        } else if ( datastr != 0 ) printf("strlen(%s) is %ld not %d*2\n",datastr,strlen(datastr),size);
                    }
                }
                else
                {
                    ram_parse_snapshot(&snap,json);
                    snapjson = ram_snapshot_json(&snap);
                    snapstr = cJSON_Print(snapjson), free_json(snapjson);
                    copy_cJSON(shastr,cJSON_GetObjectItem(json,"sha256"));
                    decode_hex(hash.bytes,sizeof(hash),shastr);
                    _stripwhite(snapstr,' ');
                    // update pyramid
                    if ( ram != 0 )
                    {
                        i = (blocknum >> 6);
                        ram_setsnapshot(ram,&ram->verified[i],blocknum,&snap,calc_nxt64bits(sender));
                    }
                    printf("PYRAMID.B%d blocknum.%u [%d] sha.(%s) (%s) from NXT.(%s) ip.(%s)\n",format,blocknum,i,shastr,snapstr,sender,senderip);
                    free(snapstr);
                }
            }
            else if ( strcmp(origcmd,"ramblock") == 0 )
            {
                if ( datastr != 0 )
                    printf("PYRAMID.B%d blocknum.%u (%s).permsize %ld from NXT.(%s) ip.(%s)\n",format,blocknum,datastr,strlen(datastr)/2,sender,senderip);
                else printf("PYRAMID.B%d blocknum.%u from NXT.(%s) ip.(%s)\n",format,blocknum,sender,senderip);
            }
            else
            {
                if ( strcmp(origcmd,"addr") == 0 )
                    type = 'a';
                else if ( strcmp(origcmd,"script") == 0 )
                    type = 's';
                else if ( strcmp(origcmd,"txid") == 0 )
                    type = 't';
                if ( type != 0 )
                {
                    copy_cJSON(permstr,cJSON_GetObjectItem(json,"permstr"));
                    permind = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"rawind"),0);
                    printf("PYRAMID.%s (permind.%d %s).%c from NXT.(%s) ip.(%s)\n",origcmd,permind,permstr,type,sender,senderip);
                    // update pyramid
                }
                else if ( format == 0 )
                    printf("RAMRESPONSE unhandled: (%s) (%s) (%s) (%s) from NXT.(%s) ip.(%s)\n",coin,origcmd,permstr,origargstr,sender,senderip);
            }
        }
        free_json(array);
    }
    // fprintf(stderr,"done ramresponse\n");
    return(retstr);
}

int32_t is_remote_access(char *previpaddr);
char *ramstring(char *origargstr,char *sender,char *previpaddr,char *coin,char *typestr,uint32_t rawind)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    int32_t truncateflag = is_remote_access(previpaddr);
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    if ( strcmp(typestr,"script") == 0 )
        return(ram_script_json(ram,rawind,truncateflag));
    else if ( strcmp(typestr,"addr") == 0 )
        return(ram_addr_json(ram,rawind,truncateflag));
    else if ( strcmp(typestr,"txid") == 0 )
        return(ram_txid_json(ram,rawind,truncateflag));
    else return(clonestr("{\"error\":\"no ramstring invalid type\"}"));
}

char *ramrawind(char *origargstr,char *sender,char *previpaddr,char *coin,char *typestr,char *str)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    int32_t truncateflag = is_remote_access(previpaddr);
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    if ( strcmp(typestr,"script") == 0 )
        return(ram_scriptind_json(ram,str,truncateflag));
    else if ( strcmp(typestr,"addr") == 0 )
        return(ram_addrind_json(ram,str,truncateflag));
    else if ( strcmp(typestr,"txid") == 0 )
        return(ram_txidind_json(ram,str,truncateflag));
    else return(clonestr("{\"error\":\"no ramrawind invalid type\"}"));
}

char *ramscript(char *origargstr,char *sender,char *previpaddr,char *coin,char *txidstr,int32_t tx_vout,struct address_entry *bp)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    char retbuf[1024],scriptstr[8192];
    struct rawvout *vo;
    HUFF *hp;
    struct ramchain_hashptr *txptr,*addrptr;
    struct rampayload *txpayloads,*txpayload,*addrpayload;
    int32_t datalen,numpayloads;
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    if ( bp == 0 )
    {
        if ( (txpayloads= ram_txpayloads(&txptr,&numpayloads,ram,txidstr)) != 0 )
        {
            if ( tx_vout < txptr->numpayloads )
            {
                txpayload = &txpayloads[tx_vout];
                if ( (addrpayload= ram_getpayloadi(&addrptr,ram,'a',txpayload->otherind,txpayload->extra)) != 0 )
                {
                    ram_script(scriptstr,ram,addrpayload->extra);
                    sprintf(retbuf,"{\"result\":\"script\",\"txid\":\"%s\",\"vout\":%u,\"script\":\"%s\"}",txidstr,tx_vout,scriptstr);
                    return(clonestr(retbuf));
                } else return(clonestr("{\"error\":\"ram_getpayloadi error\"}"));
            } else return(clonestr("{\"error\":\"tx_vout error\"}"));
        }
        return(clonestr("{\"error\":\"no ram_txpayloads info\"}"));
    }
    else if ( (hp= ram->blocks.hps[bp->blocknum]) != 0 )
    {
        if ( (datalen= ram_expand_bitstream(0,ram->R,ram,hp)) > 0 )
        {
            if ( (vo= ram_rawvout(ram->R,bp->txind,bp->v)) != 0 )
            {
                sprintf(retbuf,"{\"result\":\"script\",\"txid\":\"%s\",\"vout\":%u,\"script\":\"%s\"}",txidstr,bp->v,vo->script);
                return(clonestr(retbuf));
            } else return(clonestr("{\"error\":\"ram_rawvout error\"}"));
        } else return(clonestr("{\"error\":\"ram_expand_bitstream error\"}"));
    }
    else return(clonestr("{\"error\":\"no blocks.hps[] info\"}"));
}

char *ramtxlist(char *origargstr,char *sender,char *previpaddr,char *coin,char *coinaddr,int32_t unspentflag)
{
    cJSON *json;
    char *retstr = 0;
    struct ramchain_info *ram = get_ramchain_info(coin);
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    json = ram_coinaddr_json(ram,coinaddr,unspentflag,0,0);
    if ( json != 0 )
    {
        retstr = cJSON_Print(json);
        free_json(json);
    } else retstr = clonestr("{\"error\":\"ramtxlist no data\"}");
    return(retstr);
}

char *ramrichlist(char *origargstr,char *sender,char *previpaddr,char *coin,int32_t numwhales,int32_t recalcflag)
{
    int32_t i,ind,good,bad,numunspent,numaddrs,n = 0;
    cJSON *item,*array = 0;
    char coinaddr[1024];
    struct ramchain_hashptr **addrs,*addrptr;
    char *retstr;
    double *sortbuf,startmilli = milliseconds();
    uint64_t unspent;
    struct ramchain_info *ram = get_ramchain_info(coin);
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    if ( (addrs= ram_getallstrptrs(&numaddrs,ram,'a')) != 0 && numaddrs > 0 )
    {
        sortbuf = calloc(2*numaddrs,sizeof(*sortbuf));
        for (i=good=bad=0; i<numaddrs; i++)
        {
            ram_decode_hashdata(coinaddr,'a',addrs[i]->hh.key);
            if ( recalcflag != 0 )
                unspent = ram_calc_unspent(0,&numunspent,&addrptr,ram,coinaddr,0);
            else unspent = addrs[i]->unspent, addrptr = addrs[i], numunspent = addrptr->numunspent;
            if ( unspent != 0 )
            {
                if ( addrs[i]->unspent == unspent && addrptr->numunspent == numunspent )
                    good++;
                else bad++;
                sortbuf[n << 1] = dstr(unspent);
                memcpy(&sortbuf[(n << 1) + 1],&i,sizeof(i));
                n++;
            }
        }
        if ( n > 1 )
            qsort(sortbuf,n,sizeof(double) * 2,_decreasing_double_cmp);
        if ( n > 0 )
        {
            array = cJSON_CreateArray();
            for (i=0; i<numwhales&&i<n; i++)
            {
                item = cJSON_CreateObject();
                memcpy(&ind,&sortbuf[(i << 1) + 1],sizeof(i));
                ram_decode_hashdata(coinaddr,'a',addrs[ind]->hh.key);
                cJSON_AddItemToObject(item,coinaddr,cJSON_CreateNumber(sortbuf[i<<1]));
                cJSON_AddItemToObject(item,"unspent",cJSON_CreateNumber(dstr(addrs[ind]->unspent)));
                cJSON_AddItemToArray(array,item);
            }
            item = cJSON_CreateObject();
            if ( recalcflag != 0 )
            {
                cJSON_AddItemToObject(item,"cumulative errors",cJSON_CreateNumber(bad));
                cJSON_AddItemToObject(item,"cumulative correct",cJSON_CreateNumber(good));
            }
            cJSON_AddItemToObject(item,"milliseconds",cJSON_CreateNumber(milliseconds()-startmilli));
            cJSON_AddItemToArray(array,item);
        }
        free(sortbuf);
        free(addrs);
    }
    if ( array != 0 )
    {
        retstr = cJSON_Print(array);
        free_json(array);
    } else retstr = clonestr("{\"error\":\"ramrichlist no data\"}");
    return(retstr);
}

char *rambalances(char *origargstr,char *sender,char *previpaddr,char *coin,char **coins,double *rates,char ***coinaddrs,int32_t numcoins)
{
    uint64_t total = 0;
    char *retstr = 0;
    int32_t i,j;
    cJSON *retjson,*array;
    struct ramchain_info *ram = get_ramchain_info(coin);
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    if ( numcoins > 0 && coins != 0 && coinaddrs != 0 )
    {
        retjson = cJSON_CreateObject();
        array = cJSON_CreateArray();
        if ( numcoins == 1 && strcmp(coins[0],coin) == 0 )
        {
            for (j=0; coinaddrs[0][j]!=0; j++)
                total += ram_unspent_json(&array,coin,0.,coin,coinaddrs[0][j]);
            cJSON_AddItemToObject(retjson,"total",array);
        }
        else if ( rates != 0 )
        {
            for (i=0; i<numcoins; i++)
            {
                for (j=0; coinaddrs[i][j]!=0; j++)
                    total += ram_unspent_json(&array,coin,rates[i],coins[i],coinaddrs[i][j]);
            }
            cJSON_AddItemToObject(retjson,"subtotals",array);
            cJSON_AddItemToObject(retjson,"estimated total",cJSON_CreateString(coin));
            cJSON_AddItemToObject(retjson,coin,cJSON_CreateNumber(dstr(total)));
        } else return(clonestr("{\"error\":\"rambalances: need rates for multicoin request\"}"));
        retstr = cJSON_Print(retjson);
        free_json(retjson);
    }
    return(clonestr("{\"error\":\"rambalances: numcoins zero or bad ptr\"}"));
}

char *ramblock(char *myNXTaddr,char *origargstr,char *sender,char *previpaddr,char *coin,uint32_t blocknum)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    char hexstr[8192];
    cJSON *json = 0;
    HUFF *hp,*permhp;
    int32_t datalen;
    char *retstr = 0;
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    if ( (hp= ram->blocks.hps[blocknum]) == 0 )
    {
        _get_blockinfo(ram->R,ram,blocknum);
        json = ram_rawblock_json(ram->R,0);
    }
    else
    {
        ram_expand_bitstream(&json,ram->R,ram,hp);
        permhp = ram_conv_permind(ram->tmphp,ram,hp,blocknum);
        datalen = hconv_bitlen(permhp->endpos);
        if ( json != 0 && permhp != 0 && datalen < (sizeof(hexstr)/2-1) )
        {
            init_hexbytes_noT(hexstr,permhp->buf,datalen);
            if ( is_remote_access(previpaddr) != 0 )
            {
                free_json(json);
                json = cJSON_CreateObject();
                cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(myNXTaddr));
                cJSON_AddItemToObject(json,"blocknum",cJSON_CreateNumber(blocknum));
            }
            cJSON_AddItemToObject(json,"data",cJSON_CreateString(hexstr));
        } else printf("error getting json.%p or permhp.%p allocsize.%d\n",json,permhp,permhp != 0 ? datalen : 0);
    }
    if ( json != 0 )
    {
        retstr = cJSON_Print(json);
        free_json(json);
    }
    return(retstr);
}

char *ramcompress(char *origargstr,char *sender,char *previpaddr,char *coin,char *blockhex)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    uint8_t *data;
    cJSON *json;
    int32_t datalen,complen;
    char *retstr,*hexstr;
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    datalen = (int32_t)strlen(blockhex);
    datalen >>= 1;
    data = calloc(1,datalen);
    decode_hex(data,datalen,blockhex);
    if ( (complen= ram_compress(ram->tmphp,ram,data,datalen)) > 0 )
    {
        hexstr = calloc(1,complen*2+1);
        init_hexbytes_noT(hexstr,ram->tmphp->buf,complen);
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json,"result",cJSON_CreateString(coin));
        cJSON_AddItemToObject(json,"bitstream",cJSON_CreateString(hexstr));
        cJSON_AddItemToObject(json,"datalen",cJSON_CreateNumber(datalen));
        cJSON_AddItemToObject(json,"compressed",cJSON_CreateNumber(complen));
        retstr = cJSON_Print(json);
        free_json(json);
        free(hexstr);
    } else retstr = clonestr("{\"error\":\"no block info\"}");
    free(data);
    return(retstr);
}

char *ramexpand(char *origargstr,char *sender,char *previpaddr,char *coin,char *bitstream)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    uint8_t *data;
    HUFF *hp;
    int32_t datalen,expandlen;
    char *retstr;
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    datalen = (int32_t)strlen(bitstream);
    datalen >>= 1;
    data = calloc(1,datalen);
    decode_hex(data,datalen,bitstream);
    hp = hopen(ram->name,&ram->Perm,data,datalen,0);
    if ( (expandlen= ram_expand_bitstream(0,ram->R,ram,hp)) > 0 )
    {
        free(retstr);
        retstr = ram_blockstr(ram->R2,ram,ram->R);
    } else clonestr("{\"error\":\"no ram_expand_bitstream info\"}");
    hclose(hp);
    free(data);
    return(retstr);
}

#define RAMAPI_ERRORSTR "{\"error\":\"invalid ramchain parameters\"}"
#define RAMAPI_ILLEGALREMOTE "{\"error\":\"invalid ramchain remote access\"}"
char *preprocess_ram_apiargs(char *coin,char *previpaddr,cJSON **objs,int32_t valid,char *origargstr,char *NXTaddr,char *NXTACCTSECRET)
{
 /*   static bits256 zerokey;
    char hopNXTaddr[64],destNXTaddr[64],destip[MAX_JSON_FIELD],*str,*jsonstr,*retstr = 0;
    uint16_t port;
    cJSON *array,*json;
    int32_t createdflag;
    struct pserver_info *pserver;
    struct NXT_acct *destnp;
    copy_cJSON(destip,objs[0]);
    port = (uint16_t)get_API_int(objs[1],0);
    copy_cJSON(coin,objs[2]);
    if ( strcmp(Global_mp->ipaddr,destip) == 0 )
        return(0);
    //printf("myipaddr.(%s) process args (%s) (%s) port.%d\n",Global_mp->ipaddr,destip,coin,port);
    if ( coin[0] != 0 && destip[0] != 0 && valid > 0 )
    {
        if ( is_remote_access(previpaddr) == 0 )
        {
            pserver = get_pserver(0,destip,0,0);
            if ( pserver->nxt64bits != 0 )
            {
                expand_nxt64bits(destNXTaddr,pserver->nxt64bits);
                destnp = get_NXTacct(&createdflag,Global_mp,destNXTaddr);
                if ( (memcmp(destnp->stats.pubkey,&zerokey,sizeof(zerokey)) == 0 || port != 0) && destip[0] != 0 )
                {
                    //printf("send to ipaddr.(%s/%d)\n",destip,port);
                    send_to_ipaddr(port,0,destip,origargstr,NXTACCTSECRET);
                }
                else if ( (array= cJSON_Parse(origargstr)) != 0 )
                {
                    if ( is_cJSON_Array(array) != 0 && cJSON_GetArraySize(array) == 2 )
                    {
                        json = cJSON_GetArrayItem(array,0);
                        jsonstr = cJSON_Print(json);
                        stripwhite_ns(jsonstr,strlen(jsonstr));
                        //printf("send cmd.(%s)\n",jsonstr);
                        if ( (str = send_tokenized_cmd(!prevent_queueing("ramchain"),hopNXTaddr,0,NXTaddr,NXTACCTSECRET,jsonstr,destNXTaddr)) != 0 )
                            free(str);
                        free(jsonstr);
                    }
                    free_json(array);
                } else printf("preprocess_ram_apiargs: error parsing (%s)\n",origargstr);
                return(clonestr("{\"result\":\"sent request to destip\"}"));
            } // only path to continue sequence
        } else retstr = clonestr(RAMAPI_ERRORSTR);
    }
    if ( destip[0] != 0 )
    {
        retstr = clonestr("{\"error\":\"unvalidated path with destip\"}");
        destip[0] = 0;
    }
    return(retstr);*/
    return(0);
}

void ram_request(uint64_t nxt64bits,char *destip,struct ramchain_info *ram,char *jsonstr)
{
   /* static bits256 zerokey;
    char hopNXTaddr[64],destNXTaddr[64],ipaddr[64],*str;
    struct pserver_info *pserver;
    int32_t createdflag;
    struct NXT_acct *destnp;
    if ( nxt64bits == 0 )
    {
        pserver = get_pserver(0,destip,0,0);
        nxt64bits =  pserver->nxt64bits;
    }
    if ( nxt64bits != 0 && nxt64bits != Global_mp->nxt64bits )
    {
        expand_nxt64bits(destNXTaddr,nxt64bits);
        destnp = get_NXTacct(&createdflag,Global_mp,destNXTaddr);
        expand_ipbits(ipaddr,destnp->stats.ipbits);
        if ( 1 || memcmp(destnp->stats.pubkey,&zerokey,sizeof(zerokey)) == 0 )
        {
            //printf("send to ipaddr.(%s)\n",destip);
            send_to_ipaddr(0,1,ipaddr,jsonstr,ram->srvNXTACCTSECRET);
        }
        else if ( (str = send_tokenized_cmd(!prevent_queueing("ramchain"),hopNXTaddr,0,ram->srvNXTADDR,ram->srvNXTACCTSECRET,jsonstr,destNXTaddr)) != 0 )
            free(str);
    }*/
}

void ram_syncblocks(struct ramchain_info *ram,uint32_t blocknum,int32_t numblocks,uint64_t *sources,int32_t n,int32_t addshaflag)
{
    int32_t ram_perm_sha256(bits256 *hashp,struct ramchain_info *ram,uint32_t blocknum,int32_t n);
    char destip[64],jsonstr[MAX_JSON_FIELD],shastr[128],hashstr[65];
    int32_t i;
    cJSON *array;
    bits256 hash;
    if ( addshaflag != 0 && ram_perm_sha256(&hash,ram,blocknum,numblocks) == numblocks )
    {
        init_hexbytes_noT(hashstr,hash.bytes,sizeof(hash));
        sprintf(shastr,",\"mysha256\":\"%s\"",hashstr);
    }
    else shastr[0] = 0;
    sprintf(jsonstr,"{\"requestType\":\"rampyramid\",\"coin\":\"%s\",\"NXT\":\"%s\",\"blocknum\":%u,\"type\":\"B%d\"%s}",ram->name,ram->srvNXTADDR,blocknum,numblocks,shastr);
    if ( sources != 0 && n > 0 )
    {
        for (i=0; i<n; i++)
            if ( sources[i] != 0 )
                ram_request(sources[i],0,ram,jsonstr);
    }
    else
    {
        array = cJSON_GetObjectItem(MGWconf,"whitelist");
        if ( array != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
        {
            //printf("RAMSYNC.(%s)\n",jsonstr);
            for (i=0; i<n; i++)
            {
                copy_cJSON(destip,cJSON_GetArrayItem(array,i));
                ram_request(0,destip,ram,jsonstr);
            }
        }
    }
}

void ram_sendresponse(char *origcmd,char *coinstr,char *retstr,char *NXTaddr,char *NXTACCTSECRET,char *destNXTaddr,char *previpaddr)
{
    int32_t len,timeout = 300;
    cJSON *json;
    char fname[MAX_JSON_FIELD],hopNXTaddr[64],*jsonstr,*str = 0;
    if ( is_remote_access(previpaddr) != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            ensure_jsonitem(json,"coin",coinstr);
            ensure_jsonitem(json,"origcmd",origcmd);
            ensure_jsonitem(json,"requestType","ramresponse");
            jsonstr = cJSON_Print(json);
            _stripwhite(jsonstr,' ');
            if ( (len= (int32_t)strlen(jsonstr)) < 1024 )
            {
                hopNXTaddr[0] = 0;
                str = send_tokenized_cmd(!prevent_queueing("ramchain"),hopNXTaddr,0,NXTaddr,NXTACCTSECRET,jsonstr,destNXTaddr);
            }
            else
            {
                sprintf(fname,"ramresponse.%s.%d",NXTaddr,rand());
                str = start_transfer(previpaddr,destNXTaddr,NXTaddr,NXTACCTSECRET,previpaddr,fname,(uint8_t *)jsonstr,len,timeout,"ramchain",0);
            }
            if ( str != 0 )
                free(str);
            free_json(json);
            free(jsonstr);
        }
    }
}

char *ramresponse_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    if ( is_remote_access(previpaddr) == 0 )
        return(0);
    if ( Debuglevel > 2 )
        fprintf(stderr,"ramresponse_func(%s)\n",origargstr);
    if ( sender[0] != 0 && valid > 0 )
        return(ramresponse(origargstr,sender,previpaddr,cJSON_str(objs[2])));
    else return(clonestr(RAMAPI_ERRORSTR));
}

char *ramstring_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],typestr[MAX_JSON_FIELD],*retstr = 0;
    uint32_t rawind = 0;
    if ( (retstr= preprocess_ram_apiargs(coin,previpaddr,objs,valid,origargstr,NXTaddr,NXTACCTSECRET)) != 0 )
        return(retstr);
    copy_cJSON(typestr,objs[3]);
    rawind = (uint32_t)get_API_int(objs[4],0);
    if ( get_ramchain_info(coin) != 0 && typestr[0] != 0 && sender[0] != 0 && valid > 0 )
    {
        retstr = ramstring(origargstr,sender,previpaddr,coin,typestr,rawind);
        ram_sendresponse(typestr,coin,retstr,NXTaddr,NXTACCTSECRET,sender,previpaddr);
    }
    else retstr = clonestr(RAMAPI_ERRORSTR);
    return(retstr);
}

char *ramrawind_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],typestr[MAX_JSON_FIELD],str[MAX_JSON_FIELD],*retstr = 0;
    if ( (retstr= preprocess_ram_apiargs(coin,previpaddr,objs,valid,origargstr,NXTaddr,NXTACCTSECRET)) != 0 )
        return(retstr);
    copy_cJSON(typestr,objs[3]);
    copy_cJSON(str,objs[4]);
    if ( get_ramchain_info(coin) != 0 && typestr[0] != 0 && str[0] != 0 && sender[0] != 0 && valid > 0 )
    {
        retstr = ramrawind(origargstr,sender,previpaddr,coin,typestr,str);
        ram_sendresponse(typestr,coin,retstr,NXTaddr,NXTACCTSECRET,sender,previpaddr);
    }
    else retstr = clonestr(RAMAPI_ERRORSTR);
    return(retstr);
}

char *ramstatus_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],*retstr = 0;
    if ( (retstr= preprocess_ram_apiargs(coin,previpaddr,objs,valid,origargstr,NXTaddr,NXTACCTSECRET)) != 0 )
        return(retstr);
    printf("after process args\n");
    if ( get_ramchain_info(coin) != 0 && sender[0] != 0 && valid > 0 )
    {
        printf("calling ramstatus\n");
        retstr = ramstatus(origargstr,sender,previpaddr,coin);
        ram_sendresponse("ramstatus",coin,retstr,NXTaddr,NXTACCTSECRET,sender,previpaddr);
    }
    else retstr = clonestr(RAMAPI_ERRORSTR);
    return(retstr);
}

char *rampyramid_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],typestr[MAX_JSON_FIELD],*retstr = 0;
    uint32_t blocknum;
    if ( (retstr= preprocess_ram_apiargs(coin,previpaddr,objs,valid,origargstr,NXTaddr,NXTACCTSECRET)) != 0 )
        return(retstr);
    blocknum = (uint32_t)get_API_int(objs[3],-1);
    copy_cJSON(typestr,objs[4]);
    if ( Debuglevel > 2 )
        printf("got pyramid: (%s)\n",origargstr);
    if ( get_ramchain_info(coin) != 0 && sender[0] != 0 && valid > 0 )
    {
        retstr = rampyramid(NXTaddr,origargstr,sender,previpaddr,coin,blocknum,typestr);
        ram_sendresponse("rampyramid",coin,retstr,NXTaddr,NXTACCTSECRET,sender,previpaddr);
    }
    else retstr = clonestr(RAMAPI_ERRORSTR);
    return(retstr);
}

char *ramscript_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],*txidstr,*retstr = 0;
    int32_t vout,tx_vout,blocknum,txind,validB = 0;
    struct address_entry B;
    memset(&B,0,sizeof(B));
    if ( (retstr= preprocess_ram_apiargs(coin,previpaddr,objs,valid,origargstr,NXTaddr,NXTACCTSECRET)) != 0 )
        return(retstr);
    txidstr = cJSON_str(objs[3]);
    tx_vout = (uint32_t)get_API_int(objs[4],-1);
    B.blocknum = blocknum = (uint32_t)get_API_int(objs[5],-1);
    B.txind = txind = (uint32_t)get_API_int(objs[6],-1);
    B.v = vout = (uint32_t)get_API_int(objs[7],-1);
    if ( blocknum >= 0 && txind >= 0 && vout >= 0 && B.blocknum == blocknum && B.txind == txind && B.v == vout )
        validB = 1;
    if ( get_ramchain_info(coin) != 0 && ((txidstr != 0 && tx_vout >= 0) || validB != 0) && sender[0] != 0 && valid > 0 )
    {
        retstr = ramscript(origargstr,sender,previpaddr,coin,txidstr,tx_vout,(validB != 0) ? &B : 0);
        ram_sendresponse("ramscript",coin,retstr,NXTaddr,NXTACCTSECRET,sender,previpaddr);
    }
    else retstr = clonestr(RAMAPI_ERRORSTR);
    return(retstr);
}

char *ramblock_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],*retstr = 0;
    uint32_t blocknum;
    if ( (retstr= preprocess_ram_apiargs(coin,previpaddr,objs,valid,origargstr,NXTaddr,NXTACCTSECRET)) != 0 )
        return(retstr);
    blocknum = (uint32_t)get_API_int(objs[3],0);
    if ( get_ramchain_info(coin) != 0 && sender[0] != 0 && valid > 0 )
    {
        retstr = ramblock(NXTaddr,origargstr,sender,previpaddr,coin,blocknum);
        ram_sendresponse("ramblock",coin,retstr,NXTaddr,NXTACCTSECRET,sender,previpaddr);
    }
    else retstr = clonestr(RAMAPI_ERRORSTR);
    return(retstr);
}

// local ramchain funcs
char *ramcompress_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],*ramhex,*retstr = 0;
    if ( is_remote_access(previpaddr) != 0 )
        return(clonestr(RAMAPI_ILLEGALREMOTE));
    copy_cJSON(coin,objs[0]);
    ramhex = cJSON_str(objs[1]);
    if ( coin[0] != 0 && ramhex != 0 && sender[0] != 0 && valid > 0 )
        retstr = ramcompress(origargstr,sender,previpaddr,coin,ramhex);
    else retstr = clonestr(RAMAPI_ERRORSTR);
    return(retstr);
}

char *ramexpand_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],*bitstream,*retstr = 0;
    if ( is_remote_access(previpaddr) != 0 )
        return(clonestr(RAMAPI_ILLEGALREMOTE));
    copy_cJSON(coin,objs[0]);
    bitstream = cJSON_str(objs[1]);
    if ( coin[0] != 0 && bitstream != 0 && sender[0] != 0 && valid > 0 )
        retstr = ramexpand(origargstr,sender,previpaddr,coin,bitstream);
    else retstr = clonestr(RAMAPI_ERRORSTR);
    return(retstr);
}

char *ramaddrlist_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],*retstr = 0;
    if ( is_remote_access(previpaddr) != 0 )
        return(clonestr(RAMAPI_ILLEGALREMOTE));
    copy_cJSON(coin,objs[0]);
    if ( coin[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = ramaddrlist(origargstr,sender,previpaddr,coin);
    else retstr = clonestr(RAMAPI_ERRORSTR);
    return(retstr);
}

char *ramtxlist_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],coinaddr[MAX_JSON_FIELD],*retstr = 0;
    int32_t unspentflag;
    if ( is_remote_access(previpaddr) != 0 )
        return(clonestr(RAMAPI_ILLEGALREMOTE));
    copy_cJSON(coin,objs[0]);
    copy_cJSON(coinaddr,objs[1]);
    unspentflag = (int32_t)get_API_int(objs[2],0);
    if ( coin[0] != 0 && coinaddr[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = ramtxlist(origargstr,sender,previpaddr,coin,coinaddr,unspentflag);
    else retstr = clonestr(RAMAPI_ERRORSTR);
    return(retstr);
}

char *ramrichlist_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],*retstr = 0;
    uint32_t num;
    int32_t recalcflag;
    if ( is_remote_access(previpaddr) != 0 )
        return(clonestr(RAMAPI_ILLEGALREMOTE));
    copy_cJSON(coin,objs[0]);
    num = (uint32_t)get_API_int(objs[1],0);
    recalcflag = (uint32_t)get_API_int(objs[2],1);
    if ( coin[0] != 0 && num != 0 && sender[0] != 0 && valid > 0 )
        retstr = ramrichlist(origargstr,sender,previpaddr,coin,num,recalcflag);
    else retstr = clonestr(RAMAPI_ERRORSTR);
    return(retstr);
}

double extract_rate(cJSON *array,char *base,char *rel)
{
    int32_t i,n;
    cJSON *item;
    double rate = 0.;
    char basestr[MAX_JSON_FIELD],relstr[MAX_JSON_FIELD];
    if ( is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
    {
        for (i=0; i<n; i++)
        {
            item = cJSON_GetArrayItem(array,i);
            copy_cJSON(basestr,cJSON_GetObjectItem(item,"base"));
            copy_cJSON(relstr,cJSON_GetObjectItem(item,"rel"));
            rate = get_API_float(cJSON_GetObjectItem(item,"rate"));
            if ( strcmp(base,basestr) == 0 && strcmp(rel,relstr) == 0 )
                return(rate);
            if ( strcmp(rel,basestr) == 0 && strcmp(base,relstr) == 0 && rate != 0. )
                return(1. / rate);
        }
    }
    return(0.);
}

char *rambalances_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char coin[MAX_JSON_FIELD],srccoin[MAX_JSON_FIELD],**coins = 0,**list,***coinaddrs = 0,*retstr = 0;
    double *rates = 0;
    cJSON *item,*subarray;
    uint32_t numcoins = 0,numaddrs,i,j;
    if ( is_remote_access(previpaddr) != 0 )
        return(clonestr(RAMAPI_ILLEGALREMOTE));
    copy_cJSON(coin,objs[0]);
    if ( is_cJSON_Array(objs[1]) != 0 && (numcoins= cJSON_GetArraySize(objs[1])) > 0 )
    {
        coins = calloc(numcoins,sizeof(*coins));
        rates = calloc(numcoins,sizeof(*rates));
        coinaddrs = calloc(numcoins,sizeof(*coinaddrs));
        for (i=0; i<numcoins; i++)
        {
            item = cJSON_GetArrayItem(objs[1],i);
            coins[i] = cJSON_str(cJSON_GetObjectItem(item,"coin"));
            subarray = cJSON_GetObjectItem(item,"addrs");
            if ( srccoin[0] != 0 && subarray != 0 && is_cJSON_Array(subarray) != 0 && (numaddrs= cJSON_GetArraySize(subarray)) > 0 )
            {
                list = calloc(numaddrs+1,sizeof(*list));
                for (j=0; j<numaddrs; j++)
                    list[j] = cJSON_str(cJSON_GetArrayItem(subarray,j));
                coinaddrs[i] = list;
                rates[i] = extract_rate(objs[2],srccoin,coin);
            }
        }
    }
    if ( coin[0] != 0 && numcoins != 0 && coins != 0 && coinaddrs != 0 && rates != 0 && sender[0] != 0 && valid > 0 )
        retstr = rambalances(origargstr,sender,previpaddr,coin,coins,rates,coinaddrs,numcoins);
    else retstr = clonestr(RAMAPI_ERRORSTR);
    if ( coins != 0 )
        free(coins);
    if ( coinaddrs != 0 )
    {
        for (i=0; i<numcoins; i++)
            if ( coinaddrs[i] != 0 )
                free(coinaddrs[i]);
        free(coinaddrs);
    }
    if ( rates != 0 )
        free(rates);
    return(retstr);
}

#endif
#endif
