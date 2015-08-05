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
#include "../includes/cJSON.h"
#include "../utils/system777.c"
#include "gen1.c"
#include "coins777.c"


char *_get_transaction(char *coinstr,char *serverport,char *userpass,char *txidstr);
uint64_t ram_verify_txstillthere(char *coinstr,char *serverport,char *userpass,char *txidstr,int32_t vout);
//char *_get_blockhashstr(char *coinstr,char *serverport,char *userpass,uint32_t blocknum);
cJSON *_get_blockjson(uint32_t *heightp,char *coinstr,char *serverport,char *userpass,char *blockhashstr,uint32_t blocknum);
uint32_t _get_RTheight(double *lastmillip,char *coinstr,char *serverport,char *userpass,int32_t current_RTblocknum);
cJSON *_rawblock_txarray(uint32_t *blockidp,int32_t *numtxp,cJSON *blockjson);

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
                //printf("get_RTheight %u\n",height);
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

void ram_clear_rawblock(struct rawblock *raw,int32_t totalflag)
{
    //struct rawvin { char txidstr[128]; uint16_t vout; };
    //struct rawvout { char coinaddr[128],script[1024]; uint64_t value; };
    //struct rawtx { uint16_t firstvin,numvins,firstvout,numvouts; char txidstr[128]; };
    int32_t i; long len; struct rawtx *tx;
    if ( totalflag != 0 )
    {
        uint8_t *ptr = (uint8_t *)raw;
        len = sizeof(*raw);
        while ( len > 0 )
        {
            memset(ptr,0,len < 1024*1024 ? len : 1024*1024);
            len -= 1024 * 1024;
            ptr += 1024 * 1024;
        }
    }
    else
    {
        raw->blocknum = raw->minted = raw->numtx = raw->numrawvins = raw->numrawvouts = 0;
        tx = raw->txspace;
        for (i=0; i<MAX_BLOCKTX; i++,tx++)
        {
            tx->txidstr[0] = tx->numvins = tx->numvouts = tx->firstvout = tx->firstvin = 0;
            raw->vinspace[i].txidstr[0] = 0, raw->vinspace[i].vout = 0xffff;
            raw->voutspace[i].coinaddr[0] = raw->voutspace[i].script[0] = 0, raw->voutspace[i].value = 0;
        }
    }
}

void _set_string(char type,char *dest,char *src,long max)
{
    static uint32_t count;
    if ( src == 0 || src[0] == 0 )
        sprintf(dest,"ffff");
    else if ( strlen(src) < max-1 )
        strcpy(dest,src);
    else
    {
        count++;
        printf("count.%d >>>>>>>>>>> len.%ld > max.%ld (%s)\n",count,strlen(src),max,src);
        sprintf(dest,"nonstandard");
    }
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

uint64_t rawblock_txidinfo(struct rawblock *raw,struct rawtx *tx,char *coinstr,char *serverport,char *userpass,int32_t txind,char *txidstr,uint32_t blocknum)
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
    }
    else
    {
        if ( blocknum == 0 )
        {
            
        }
        else printf("error getting.(%s)\n",txidstr);
    }
//printf("tx.%d: (%s) numvins.%d numvouts.%d (raw %d %d)\n",txind,tx->txidstr,tx->numvins,tx->numvouts,raw->numrawvins,raw->numrawvouts);
    return(total);
}

cJSON *_rawblock_txarray(uint32_t *blockidp,int32_t *numtxp,cJSON *blockjson)
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
    ram_clear_rawblock(raw,0);
    //raw->blocknum = blocknum;
    //printf("_get_blockinfo.%d\n",blocknum);
    raw->minted = raw->numtx = raw->numrawvins = raw->numrawvouts = 0;
    if ( (json= _get_blockjson(0,coinstr,serverport,userpass,0,blocknum)) != 0 )
    {
        raw->blocknum = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"height"),0);
        copy_cJSON(raw->blockhash,cJSON_GetObjectItem(json,"hash"));
        //fprintf(stderr,"%u: blockhash.[%s] ",blocknum,raw->blockhash);
        copy_cJSON(raw->merkleroot,cJSON_GetObjectItem(json,"merkleroot"));
        //fprintf(stderr,"raw->merkleroot.[%s]\n",raw->merkleroot);
        raw->timestamp = (uint32_t)get_cJSON_int(cJSON_GetObjectItem(json,"time"),0);
        copy_cJSON(mintedstr,cJSON_GetObjectItem(json,"mint"));
        if ( mintedstr[0] == 0 )
            copy_cJSON(mintedstr,cJSON_GetObjectItem(json,"newmint"));
        if ( mintedstr[0] != 0 )
            raw->minted = (uint64_t)(atof(mintedstr) * SATOSHIDEN);
        if ( (txobj= _rawblock_txarray(&blockid,&n,json)) != 0 && blockid == blocknum && n < MAX_BLOCKTX )
        {
            for (txind=0; txind<n; txind++)
            {
                copy_cJSON(txidstr,cJSON_GetArrayItem(txobj,txind));
                //printf("block.%d txind.%d TXID.(%s)\n",blocknum,txind,txidstr);
                total += rawblock_txidinfo(raw,&raw->txspace[raw->numtx++],coinstr,serverport,userpass,txind,txidstr,blocknum);
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

#ifdef notneeded
uint32_t coin777_packedoffset(struct alloc_space *mem,char *str,int32_t convflag)
{
    uint32_t offset; uint16_t len; uint8_t _hex[255],blen,*hex = _hex;
    offset = (uint32_t)mem->used;
    len = (uint16_t)strlen(str);
    if ( convflag != 0 )
    {
        len >>= 1;
        if ( len >= sizeof(_hex) )
        {
            printf("coin777_packedoffset: extreme len.%d for (%s)\n",len,str);
            hex = malloc(len);
        }
        decode_hex(hex,len,str);
        if ( len < 0xfd )
        {
            blen = len;
            memcpy(memalloc(mem,1,0),&blen,1);
        }
        else
        {
            blen = 0xfd;
            printf("long string len.%d %llx\n",len,*(long long *)hex);
            memcpy(memalloc(mem,1,0),&blen,1);
            memcpy(memalloc(mem,2,0),&len,2);
        }
        memcpy(memalloc(mem,len,0),hex,len);
        if ( hex != _hex )
            free(hex);
    } else len++, memcpy(memalloc(mem,len,0),str,len);
    return(offset);
}

int32_t coin777_unpackoffset(struct alloc_space *mem,char *str,int32_t convflag,uint32_t offset)
{
    int16_t len; uint8_t blen; char *ptr;
    if ( convflag != 0 )
    {
        memcpy(&blen,memalloc(mem,1,0),1);
        if ( blen == 0xfd )
            memcpy(&len,memalloc(mem,2,0),2);
        else len = blen;
        init_hexbytes_noT(str,memalloc(mem,len,0),len);
        len = (len << 1) + 1;
    }
    else
    {
        ptr = (char *)((long)mem->ptr + mem->used);
        len = (int16_t)strlen(ptr) + 1;
        memcpy(str,memalloc(mem,len,0),len);
    }
    return(len);
}

void coin777_packtx(struct alloc_space *mem,struct packedtx *ptx,struct rawtx *tx)
{
//printf("packtx.(%s) numvins.%d numvouts.%d\n",tx->txidstr,tx->numvins,tx->numvouts);
    ptx->firstvin = tx->firstvin, ptx->numvins = tx->numvins, ptx->firstvout = tx->firstvout, ptx->numvouts = tx->numvouts;
    ptx->txidstroffset = coin777_packedoffset(mem,tx->txidstr,1);
}

void coin777_packvout(struct alloc_space *mem,struct packedvout *pvo,struct rawvout *vo)
{
//printf("packvout.(%s) (%s) %.8f\n",vo->coinaddr,vo->script,dstr(vo->value));
    pvo->value = vo->value;
    pvo->coinaddroffset = coin777_packedoffset(mem,vo->coinaddr,0);
    pvo->scriptoffset = coin777_packedoffset(mem,vo->script,1);
}

void coin777_packvin(struct alloc_space *mem,struct packedvin *pvi,struct rawvin *vi)
{
//printf("packvin.(%s) vout.%d\n",vi->txidstr,vi->vout);
    pvi->vout = vi->vout;
    pvi->txidstroffset = coin777_packedoffset(mem,vi->txidstr,1);
}

void coin777_unpacktx(struct alloc_space *mem,struct packedtx *ptx,struct rawtx *tx)
{
    tx->firstvin = ptx->firstvin, tx->numvins = ptx->numvins, tx->firstvout = ptx->firstvout, tx->numvouts = ptx->numvouts;
    coin777_unpackoffset(mem,tx->txidstr,1,ptx->txidstroffset);
    //printf("unpackedtx.(%s) numvins.%d numvouts.%d\n",tx->txidstr,tx->numvins,tx->numvouts);
}

void coin777_unpackvout(struct alloc_space *mem,struct packedvout *pvo,struct rawvout *vo)
{
    vo->value = pvo->value;
    coin777_unpackoffset(mem,vo->coinaddr,0,pvo->coinaddroffset);
    coin777_unpackoffset(mem,vo->script,1,pvo->scriptoffset);
    //printf("unpackedvout.(%s) (%s) %.8f\n",vo->coinaddr,vo->script,dstr(vo->value));
}

void coin777_unpackvin(struct alloc_space *mem,struct packedvin *pvi,struct rawvin *vi)
{
    vi->vout = pvi->vout;
    coin777_unpackoffset(mem,vi->txidstr,1,pvi->txidstroffset);
   // printf("unpackedvin.(%s) vout.%d\n",vi->txidstr,vi->vout);
}

uint16_t packed_crc16(struct packedblock *packed)
{
    uint32_t crc;
    crc = _crc32(0,&((uint8_t *)packed)[sizeof(packed->crc16)],(int32_t)(packed->allocsize - sizeof(packed->crc16)));
    return((((crc >> 16) & 0xffff) ^ (uint16_t)crc));
}

struct packedblock *coin777_packrawblock(struct coin777 *coin,struct rawblock *raw)
{
    static long totalsizes,totalpacked;
    struct rawtx *tx; struct rawvin *vi; struct rawvout *vo; struct alloc_space MEM,*mem = &MEM;
    struct packedtx *ptx; struct packedvin *pvi; struct packedvout *pvo; struct packedblock *packed = 0;
    uint32_t i,txind,n;
    mem = init_alloc_space(0,0,256 + raw->numtx*sizeof(struct rawtx) + raw->numrawvouts*sizeof(struct rawvout) + raw->numrawvins*sizeof(struct rawvin),0);
    packed = memalloc(mem,sizeof(*packed),1);
    packed->numtx = raw->numtx, packed->numrawvins = raw->numrawvins, packed->numrawvouts = raw->numrawvouts;
    packed->blocknum = raw->blocknum, packed->timestamp = raw->timestamp, packed->minted = raw->minted;
    packed->blockhash_offset = coin777_packedoffset(mem,raw->blockhash,1);
    packed->merkleroot_offset = coin777_packedoffset(mem,raw->merkleroot,1);
    packed->txspace_offsets = (uint32_t)mem->used, ptx = memalloc(mem,raw->numtx*sizeof(struct packedtx),0);
    packed->voutspace_offsets = (uint32_t)mem->used, pvo = memalloc(mem,raw->numrawvouts*sizeof(struct packedvout),0);
    packed->vinspace_offsets = (uint32_t)mem->used, pvi = memalloc(mem,raw->numrawvins*sizeof(struct packedvin),0);
    tx = raw->txspace, vi = raw->vinspace, vo = raw->voutspace;
//printf("blocknum.%u numtx.%u numvouts.%d numvins.%d | %s\n",raw->blocknum,raw->numtx,raw->numrawvouts,raw->numrawvins,tx[0].txidstr);
    if ( raw->numtx > 0 )
    {
        for (txind=0; txind<raw->numtx; txind++,tx++,ptx++)
        {
            coin777_packtx(mem,ptx,tx);
            if ( (n= tx->numvouts) > 0 )
                for (i=0; i<n; i++,vo++,pvo++)
                    coin777_packvout(mem,pvo,vo);
            if ( (n= tx->numvins) > 0 )
                for (i=0; i<n; i++,vi++,pvi++)
                    coin777_packvin(mem,pvi,vi);
        }
    }
    packed->allocsize = (uint32_t)mem->used;
    packed->crc16 = packed_crc16(packed);
    totalsizes += mem->size, totalpacked += mem->used;
    //for (i=0; i<mem->used; i++)
    //    printf("%02x ",((uint8_t *)mem->ptr)[i]);
    printf("block.%u packed sizes: block.%ld tx.%ld vin.%ld vout.%ld | crc.%-5u mem->size %ld -> %5d %8s vs %8s [%.3fx]\n",raw->blocknum,sizeof(struct packedblock),sizeof(struct packedtx),sizeof(struct packedvout),sizeof(struct packedvin),packed->crc16,mem->size,packed->allocsize,_mbstr(totalsizes),_mbstr2(totalpacked),(double)totalsizes / totalpacked);
    packed = malloc(mem->used), memcpy(packed,mem->ptr,mem->used), free(mem);
    ramchain_setpackedblock(&coin->ramchain,packed,packed->blocknum);
    return(packed);
}

int32_t coin777_unpackblock(struct rawblock *raw,struct packedblock *packed,uint32_t blocknum)
{
    struct rawtx *tx; struct rawvin *vi; struct rawvout *vo; struct alloc_space MEM,*mem = &MEM;
    struct packedtx *ptx; struct packedvin *pvi; struct packedvout *pvo;
    uint32_t i,txind,n; int32_t retval = -1;
    //printf("***************\n\n");
    //for (i=0; i<packed->allocsize; i++)
    //    printf("%02x ",((uint8_t *)packed)[i]);
    //printf("unpack %d\n",packed->allocsize);
    if ( packed->crc16 != packed_crc16(packed) || packed->blocknum != blocknum )
    {
        printf("allocsize.%d crc16 mismatch %u vs %u | blocknum.%u mismatch %u\n",packed->allocsize,packed->crc16,packed_crc16(packed),blocknum,packed->blocknum);
        return(-6);
    }
    mem = init_alloc_space(mem,packed,packed->allocsize,0);
    memalloc(mem,sizeof(*packed),0);
    raw->numtx = packed->numtx, raw->numrawvins = packed->numrawvins, raw->numrawvouts = packed->numrawvouts;
    raw->blocknum = packed->blocknum, raw->timestamp = packed->timestamp, raw->minted = packed->minted;
    coin777_unpackoffset(mem,raw->blockhash,1,packed->blockhash_offset);
    coin777_unpackoffset(mem,raw->merkleroot,1,packed->merkleroot_offset);
    //printf("blocknum.%u numtx.%u numvouts.%d numvins.%d %.8f t%u | %s %s\n",raw->blocknum,raw->numtx,raw->numrawvouts,raw->numrawvins,dstr(raw->minted),raw->timestamp,raw->blockhash,raw->merkleroot);
    if ( packed->txspace_offsets != mem->used )
        retval = -2, printf("mismatched txspace offset %d vs %ld\n",packed->txspace_offsets,mem->used);
    else
    {
        ptx = memalloc(mem,packed->numtx * sizeof(struct packedtx),0);
        if ( packed->voutspace_offsets != mem->used )
            retval = -3, printf("mismatched voutspace_offsets %d vs %ld\n",packed->voutspace_offsets,mem->used);
         else
        {
            pvo = memalloc(mem,packed->numrawvouts * sizeof(struct packedvout),0);
            if ( packed->vinspace_offsets != mem->used )
                retval = -4, printf("mismatched vinspace_offsets %d vs %ld\n",packed->vinspace_offsets,mem->used);
            else
            {
                pvi = memalloc(mem,packed->numrawvins * sizeof(struct packedvin),0);
                tx = raw->txspace, vi = raw->vinspace, vo = raw->voutspace;
                if ( packed->numtx > 0 )
                {
                    for (txind=0; txind<packed->numtx; txind++,tx++,ptx++)
                    {
                        coin777_unpacktx(mem,ptx,tx);
                        if ( (n= tx->numvouts) > 0 )
                            for (i=0; i<n; i++,vo++,pvo++)
                                coin777_unpackvout(mem,pvo,vo);
                        if ( (n= tx->numvins) > 0 )
                            for (i=0; i<n; i++,vi++,pvi++)
                                coin777_unpackvin(mem,pvi,vi);
                    }
                }
            }
        }
    }
    if ( packed->allocsize != mem->used )
        retval = -5, printf("allocsize error.%d != %ld\n",packed->allocsize,mem->used);
    return(retval);
}

void coin777_disprawblock(struct rawblock *raw)
{
    struct rawtx *tx; struct rawvin *vi; struct rawvout *vo; uint32_t i,txind,n;
    tx = raw->txspace, vi = raw->vinspace, vo = raw->voutspace;
    printf("blocknum.%u numtx.%u numvouts.%d numvins.%d | %s %s\n",raw->blocknum,raw->numtx,raw->numrawvouts,raw->numrawvins,raw->blockhash,raw->merkleroot);
    if ( raw->numtx > 0 )
    {
        for (txind=0; txind<raw->numtx; txind++,tx++)
        {
            printf("(%s) numvouts.%d numvins.%d\n",tx->txidstr,tx->numvouts,tx->numvins);
            if ( (n= tx->numvouts) > 0 )
                for (i=0; i<n; i++,vo++)
                    printf("(%s) (%s) %.8f\n",vo->coinaddr,vo->script,dstr(vo->value));
            if ( (n= tx->numvins) > 0 )
                for (i=0; i<n; i++,vi++)
                    printf("(%s).v%d ",vi->txidstr,vi->vout);
        }
    }
}
void coins_verify(struct coin777 *coin,struct packedblock *packed,uint32_t blocknum)
{
    int32_t i;
    ram_clear_rawblock(coin->P.DECODE,1);
    coin777_unpackblock(coin->P.DECODE,packed,blocknum);
    if ( memcmp(coin->P.DECODE,coin->P.EMIT,sizeof(*coin->P.DECODE)) != 0 )
    {
        for (i=0; i<sizeof(coin->P.DECODE); i++)
            if ( ((uint8_t *)coin->P.DECODE)[i] != ((uint8_t *)coin->P.DECODE)[i] )
                break;
        printf("packblock decode error blocknum.%u\n",coin->P.readahead);
        coin777_disprawblock(coin->P.EMIT);
        printf("----> \n");
        coin777_disprawblock(coin->P.DECODE);
        printf("mismatch\n");
        while ( 1 ) sleep(1);
    } else printf("COMPARED! ");
}

int32_t coins_idle(struct plugin_info *plugin)
{
    int32_t i,len,flag = 0; uint32_t width = 10000;
    struct coin777 *coin; struct ledger_info *ledger; struct packedblock *packed;
    for (i=0; i<COINS.num; i++)
    {
        if ( 0 && (coin= COINS.LIST[i]) != 0 )//&& coin->P.packed != 0 )
        {
            //if ( coin777_processQs(coin) != 0 )
            //    return(1);
            //else continue;
            coin->P.RTblocknum = _get_RTheight(&coin->lastgetinfo,coin->name,coin->serverport,coin->userpass,coin->P.RTblocknum);
            while ( coin->P.packedblocknum <= coin->P.RTblocknum && coin->P.packedblocknum < coin->P.packedend )
            {
                len = (int32_t)sizeof(*coin->P.EMIT);
                if ( (packed= ramchain_getpackedblock(coin->P.EMIT,&len,&coin->ramchain,coin->P.packedblocknum)) == 0 || packed_crc16(packed) != packed->crc16 )
                {
                    ram_clear_rawblock(coin->P.EMIT,1);
                    if ( rawblock_load(coin->P.EMIT,coin->name,coin->serverport,coin->userpass,coin->P.packedblocknum) > 0 )
                    {
                        if ( coin->P.packed[coin->P.packedblocknum] == 0 )
                        {
                            if ( (coin->P.packed[coin->P.packedblocknum]= coin777_packrawblock(coin,coin->P.EMIT)) != 0 )
                            {
                                //ramchain_setpackedblock(&coin->ramchain,coin->packed[coin->packedblocknum],coin->packedblocknum);
                                //coins_verify(coin,coin->packed[coin->packedblocknum],coin->packedblocknum);
                                coin->P.packedblocknum += coin->P.packedincr;
                            }
                            flag = 1;
                            return(1);
                        }
                    } else break;
                }
                else if ( RELAYS.pushsock >= 0 )
                {
                    printf("PUSHED.(%d) blocknum.%u | crc.%u %d %d %d %.8f %u %u %u %u %u %u %d\n",packed->allocsize,packed->blocknum,packed->crc16,packed->numtx,packed->numrawvins,packed->numrawvouts,dstr(packed->minted),packed->timestamp,packed->blockhash_offset,packed->merkleroot_offset,packed->txspace_offsets,packed->vinspace_offsets,packed->voutspace_offsets,packed->allocsize);
                    nn_send(RELAYS.pushsock,(void *)packed,packed->allocsize,0);
                }
                coin->P.packedblocknum += coin->P.packedincr;
            }
            if ( 1 && flag == 0 && (ledger= coin->ramchain.activeledger) != 0 )
            {
                printf("readahead.%d vs blocknum.%u\n",coin->P.readahead,ledger->blocknum);
                if ( coin->P.readahead <= ledger->blocknum )
                    coin->P.readahead = ledger->blocknum;
                while ( coin->P.readahead <= ledger->blocknum+width )
                {
                    //printf("readahead.%u %p\n",coin->readahead++,coin->packed[coin->readahead]);
                    if ( coin->P.packed[coin->P.readahead] == 0 )
                    {
                        ram_clear_rawblock(coin->P.EMIT,1);
                        if ( rawblock_load(coin->P.EMIT,coin->name,coin->serverport,coin->userpass,coin->P.readahead) > 0 )
                        {
                            if ( (coin->P.packed[coin->P.readahead]= coin777_packrawblock(coin,coin->P.EMIT)) != 0 )
                            {
                                //ramchain_setpackedblock(&coin->ramchain,coin->packed[coin->readahead],coin->readahead);
                                //coins_verify(coin,coin->packed[coin->readahead],coin->readahead);
                                width++;
                                if ( coin->P.readahead > width && coin->P.readahead-width > ledger->blocknum && coin->P.packed[coin->P.readahead-width] != 0 )
                                {
                                    printf("purge.%u\n",coin->P.readahead-width);
                                    free(coin->P.packed[coin->P.readahead-width]), coin->P.packed[coin->P.readahead-width] = 0;
                                }
                                
                                flag = 1;
                                break;
                            }
                        } else printf("error rawblock_load.%u\n",coin->P.readahead);
                    } else coin->P.readahead++;
                }
            }
        }
    }
    return(flag);
}

void ensure_packedptrs(struct coin777 *coin)
{
    uint32_t newmax;
    coin->P.RTblocknum = _get_RTheight(&coin->lastgetinfo,coin->name,coin->serverport,coin->userpass,coin->P.RTblocknum);
    newmax = (uint32_t)(coin->P.RTblocknum * 1.1);
    if ( coin->P.maxpackedblocks < newmax )
    {
        coin->P.packed = realloc(coin->P.packed,sizeof(*coin->P.packed) * newmax);
        memset(&coin->P.packed[coin->P.maxpackedblocks],0,sizeof(*coin->P.packed) * (newmax - coin->P.maxpackedblocks));
        coin->P.maxpackedblocks = newmax;
    }
}

void coins_verify(struct coin777 *coin,struct packedblock *packed,uint32_t blocknum)
{
    int32_t i;
    ram_clear_rawblock(coin->P.DECODE,1);
    coin777_unpackblock(coin->P.DECODE,packed,blocknum);
    if ( memcmp(coin->P.DECODE,coin->P.EMIT,sizeof(*coin->P.DECODE)) != 0 )
    {
        for (i=0; i<sizeof(coin->P.DECODE); i++)
            if ( ((uint8_t *)coin->P.DECODE)[i] != ((uint8_t *)coin->P.DECODE)[i] )
                break;
        printf("packblock decode error blocknum.%u\n",coin->P.readahead);
        coin777_disprawblock(coin->P.EMIT);
        printf("----> \n");
        coin777_disprawblock(coin->P.DECODE);
        printf("mismatch\n");
        while ( 1 ) sleep(1);
    } else printf("COMPARED! ");
}
else if ( strcmp(methodstr,"packblocks") == 0 )
{
    coin->P.packedblocknum = coin->P.packedstart = get_API_int(cJSON_GetObjectItem(json,"start"),0) + COINS.slicei;
    coin->P.packedend = get_API_int(cJSON_GetObjectItem(json,"end"),1000000000);
    coin->P.packedincr = get_API_int(cJSON_GetObjectItem(json,"incr"),1);
    ensure_packedptrs(coin);
    sprintf(retbuf,"{\"result\":\"packblocks\",\"start\":\"%u\",\"end\":\"%u\",\"incr\":\"%u\",\"RTblocknum\":\"%u\"}",coin->P.packedstart,coin->P.packedend,coin->P.packedincr,coin->P.RTblocknum);
}

#endif

#endif
#endif
