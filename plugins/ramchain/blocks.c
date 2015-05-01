//
//  ramchainblocks.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_ramchainblocks_h
#define crypto777_ramchainblocks_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "bits777.c"
#include "system777.c"
#include "files777.c"
#include "huff.c"
#include "init.c"
#include "gen1pub.c"
#include "gen1.c"

struct rawblock
{
    uint32_t blocknum,allocsize;
    uint16_t format,numtx,numrawvins,numrawvouts;
    uint64_t minted;
    struct rawtx txspace[MAX_BLOCKTX];
    struct rawvin vinspace[MAX_BLOCKTX];
    struct rawvout voutspace[MAX_BLOCKTX];
};
cJSON *ram_rawblock_json(struct rawblock *raw,int32_t allocsize);
void ram_clear_rawblock(struct rawblock *raw,int32_t totalflag);

#endif
#else
#ifndef crypto777_ramchainblocks_c
#define crypto777_ramchainblocks_c

#ifndef crypto777_ramchainblocks_h
#define DEFINES_ONLY
#include "blocks.c"
#undef DEFINES_ONLY
#endif

void _set_string(char type,char *dest,char *src,long max)
{
    if ( src == 0 || src[0] == 0 )
        sprintf(dest,"ffff");
    else if ( strlen(src) < max-1)
        strcpy(dest,src);
    else sprintf(dest,"nonstandard");
}

uint64_t _get_txvouts(struct rawblock *raw,struct rawtx *tx,struct ramchain_info *ram,cJSON *voutsobj)
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

int32_t _get_txvins(struct rawblock *raw,struct rawtx *tx,struct ramchain_info *ram,cJSON *vinsobj)
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

uint64_t _get_txidinfo(struct rawblock *raw,struct rawtx *tx,struct ramchain_info *ram,int32_t txind,char *txidstr)
{
    char *retstr = 0;
    cJSON *txjson;
    uint64_t total = 0;
    if ( strlen(txidstr) < sizeof(tx->txidstr)-1 )
        strcpy(tx->txidstr,txidstr);
    tx->numvouts = tx->numvins = 0;
    if ( (retstr= _get_transaction(ram->name,ram->serverport,ram->userpass,txidstr)) != 0 )
    {
        if ( (txjson= cJSON_Parse(retstr)) != 0 )
        {
            _get_txvins(raw,tx,ram,cJSON_GetObjectItem(txjson,"vin"));
            total = _get_txvouts(raw,tx,ram,cJSON_GetObjectItem(txjson,"vout"));
            free_json(txjson);
        } else printf("update_txid_infos parse error.(%s)\n",retstr);
        free(retstr);
    } else printf("error getting.(%s)\n",txidstr);
    //printf("tx.%d: (%s) numvins.%d numvouts.%d (raw %d %d)\n",txind,tx->txidstr,tx->numvins,tx->numvouts,raw->numrawvins,raw->numrawvouts);
    return(total);
}

cJSON *_get_blocktxarray(uint32_t *blockidp,int32_t *numtxp,struct ramchain_info *ram,cJSON *blockjson)
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

uint32_t _get_blockinfo(struct rawblock *raw,struct ramchain_info *ram,uint32_t blocknum)
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
    if ( (json= _get_blockjson(0,ram->name,ram->serverport,ram->userpass,0,blocknum)) != 0 )
    {
        raw->blocknum = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"height"),0);
        copy_cJSON(mintedstr,cJSON_GetObjectItem(json,"mint"));
        if ( mintedstr[0] != 0 )
            raw->minted = (uint64_t)(atof(mintedstr) * SATOSHIDEN);
        if ( (txobj= _get_blocktxarray(&blockid,&n,ram,json)) != 0 && blockid == blocknum && n < MAX_BLOCKTX )
        {
            for (txind=0; txind<n; txind++)
            {
                copy_cJSON(txidstr,cJSON_GetArrayItem(txobj,txind));
                //printf("block.%d txind.%d TXID.(%s)\n",blocknum,txind,txidstr);
                total += _get_txidinfo(raw,&raw->txspace[raw->numtx++],ram,txind,txidstr);
            }
        } else printf("error _get_blocktxarray for block.%d got %d, n.%d vs %d\n",blocknum,blockid,n,MAX_BLOCKTX);
        if ( raw->minted == 0 )
            raw->minted = total;
        free_json(json);
    } else printf("get_blockjson error parsing.(%s)\n",txidstr);
    //printf("BLOCK.%d: block.%d numtx.%d minted %.8f rawnumvins.%d rawnumvouts.%d\n",blocknum,raw->blocknum,raw->numtx,dstr(raw->minted),raw->numrawvins,raw->numrawvouts);
    return(raw->numtx);
}

int32_t ram_get_blockoffset(struct mappedblocks *blocks,uint32_t blocknum)
{
    int32_t offset = -1;
    if ( blocknum >= blocks->firstblock )
    {
        offset = (blocknum - blocks->firstblock);
        if ( offset >= blocks->numblocks )
        {
            printf("(%d - %d) = offset.%d >= numblocks.%d for format.%d\n",blocknum,blocks->firstblock,offset,blocks->numblocks,blocks->format);
            offset = -1;
        }
    }
    return(offset);
}

HUFF **ram_get_hpptr(struct mappedblocks *blocks,uint32_t blocknum)
{
    int32_t offset = ram_get_blockoffset(blocks,blocknum);
    return((offset < 0) ? 0 : &blocks->hps[offset]);
}

struct mappedptr *ram_get_M(struct mappedblocks *blocks,uint32_t blocknum)
{
    int32_t offset = ram_get_blockoffset(blocks,blocknum);
    return((offset < 0) ? 0 : &blocks->M[offset]);
}

HUFF *ram_makehp(HUFF *tmphp,int32_t format,struct ramchain_info *ram,struct rawblock *tmp,int32_t blocknum)
{
    int32_t datalen;
    uint8_t *block;
    HUFF *hp = 0;
    hclear(tmphp,1);
    if ( (datalen= ram_emitblock(tmphp,format,ram,tmp)) > 0 )
    {
        //printf("ram_emitblock datalen.%d (%d) bitoffset.%d\n",datalen,hconv_bitlen(tmphp->endpos),tmphp->bitoffset);
        block = permalloc(ram->name,&ram->Perm,datalen,5);
        memcpy(block,tmphp->buf,datalen);
        hp = hopen(ram->name,&ram->Perm,block,datalen,0);
        hseek(hp,0,SEEK_END);
        //printf("ram_emitblock datalen.%d bitoffset.%d endpos.%d\n",datalen,hp->bitoffset,hp->endpos);
    } else printf("error emitblock.%d\n",blocknum);
    return(hp);
}

cJSON *ram_rawvin_json(struct rawblock *raw,struct rawtx *tx,int32_t vin)
{
    struct rawvin *vi = &raw->vinspace[tx->firstvin + vin];
    cJSON *json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"txid",cJSON_CreateString(vi->txidstr));
    cJSON_AddItemToObject(json,"vout",cJSON_CreateNumber(vi->vout));
    return(json);
}

cJSON *ram_rawvout_json(struct rawblock *raw,struct rawtx *tx,int32_t vout)
{
    struct rawvout *vo = &raw->voutspace[tx->firstvout + vout];
    cJSON *item,*array,*json = cJSON_CreateObject();
    /*"scriptPubKey" : {
     "asm" : "OP_DUP OP_HASH160 b2098d38dfd1bee61b12c9abc40e988d273d90ae OP_EQUALVERIFY OP_CHECKSIG",
     "reqSigs" : 1,
     "type" : "pubkeyhash",
     "addresses" : [
     "RRWZoKdmHGDbS5vfj7KwBScK3uSTpt9pHL"
     ]
     }*/
    cJSON_AddItemToObject(json,"value",cJSON_CreateNumber(dstr(vo->value)));
    cJSON_AddItemToObject(json,"n",cJSON_CreateNumber(vout));
    item = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"hex",cJSON_CreateString(vo->script));
    array = cJSON_CreateArray();
    cJSON_AddItemToArray(array,cJSON_CreateString(vo->coinaddr));
    cJSON_AddItemToObject(item,"addresses",array);
    cJSON_AddItemToObject(json,"scriptPubKey",item);
    return(json);
}

cJSON *ram_rawtx_json(struct rawblock *raw,int32_t txind)
{
    struct rawtx *tx = &raw->txspace[txind];
    cJSON *array,*json = cJSON_CreateObject();
    int32_t i,numvins,numvouts;
    cJSON_AddItemToObject(json,"txid",cJSON_CreateString(tx->txidstr));
    if ( (numvins= tx->numvins) > 0 )
    {
        array = cJSON_CreateArray();
        for (i=0; i<numvins; i++)
            cJSON_AddItemToArray(array,ram_rawvin_json(raw,tx,i));
        cJSON_AddItemToObject(json,"vin",array);
    }
    if ( (numvouts= tx->numvouts) > 0 )
    {
        array = cJSON_CreateArray();
        for (i=0; i<numvouts; i++)
            cJSON_AddItemToArray(array,ram_rawvout_json(raw,tx,i));
        cJSON_AddItemToObject(json,"vout",array);
    }
    return(json);
}

cJSON *ram_rawblock_json(struct rawblock *raw,int32_t allocsize)
{
    int32_t i,n;
    cJSON *array,*json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"height",cJSON_CreateNumber(raw->blocknum));
    cJSON_AddItemToObject(json,"numtx",cJSON_CreateNumber(raw->numtx));
    cJSON_AddItemToObject(json,"mint",cJSON_CreateNumber(dstr(raw->minted)));
    if ( allocsize != 0 )
        cJSON_AddItemToObject(json,"rawsize",cJSON_CreateNumber(allocsize));
    if ( (n= raw->numtx) > 0 )
    {
        array = cJSON_CreateArray();
        for (i=0; i<n; i++)
            cJSON_AddItemToArray(array,ram_rawtx_json(raw,i));
        cJSON_AddItemToObject(json,"tx",array);
    }
    return(json);
}

int32_t raw_blockcmp(struct rawblock *ref,struct rawblock *raw)
{
    int32_t i;
    if ( ref->numtx != raw->numtx )
    {
        printf("ref numtx.%d vs %d\n",ref->numtx,raw->numtx);
        return(-1);
    }
    if ( ref->numrawvins != raw->numrawvins )
    {
        printf("numrawvouts.%d vs %d\n",ref->numrawvins,raw->numrawvins);
        return(-2);
    }
    if ( ref->numrawvouts != raw->numrawvouts )
    {
        printf("numrawvouts.%d vs %d\n",ref->numrawvouts,raw->numrawvouts);
        return(-3);
    }
    if ( ref->numtx != 0 && memcmp(ref->txspace,raw->txspace,ref->numtx*sizeof(*raw->txspace)) != 0 )
    {
        struct rawtx *reftx,*rawtx;
        int32_t flag = 0;
        for (i=0; i<ref->numtx; i++)
        {
            reftx = &ref->txspace[i];
            rawtx = &raw->txspace[i];
            printf("1st.%d %d, %d %d (%s) vs 1st %d %d, %d %d (%s)\n",reftx->firstvin,reftx->firstvout,reftx->numvins,reftx->numvouts,reftx->txidstr,rawtx->firstvin,rawtx->firstvout,rawtx->numvins,rawtx->numvouts,rawtx->txidstr);
            flag = 0;
            if ( reftx->firstvin != rawtx->firstvin )
                flag |= 1;
            if ( reftx->firstvout != rawtx->firstvout )
                flag |= 2;
            if ( reftx->numvins != rawtx->numvins )
                flag |= 4;
            if ( reftx->numvouts != rawtx->numvouts )
                flag |= 8;
            if ( strcmp(reftx->txidstr,rawtx->txidstr) != 0 )
                flag |= 16;
            if ( flag != 0 )
                break;
        }
        if ( i != ref->numtx )
        {
            printf("flag.%d numtx.%d\n",flag,ref->numtx);
            //while ( 1 )
            //    portable_sleep(1);
            return(-4);
        }
    }
    if ( ref->numrawvins != 0 && memcmp(ref->vinspace,raw->vinspace,ref->numrawvins*sizeof(*raw->vinspace)) != 0 )
    {
        return(-5);
    }
    if ( ref->numrawvouts != 0 && memcmp(ref->voutspace,raw->voutspace,ref->numrawvouts*sizeof(*raw->voutspace)) != 0 )
    {
        struct rawvout *reftx,*rawtx;
        int32_t err = 0;
        for (i=0; i<ref->numrawvouts; i++)
        {
            reftx = &ref->voutspace[i];
            rawtx = &raw->voutspace[i];
            if ( strcmp(reftx->coinaddr,rawtx->coinaddr) != 0 || strcmp(reftx->script,rawtx->script) || reftx->value != rawtx->value )
                printf("%d of %d: (%s) (%s) %.8f vs (%s) (%s) %.8f\n",i,ref->numrawvouts,reftx->coinaddr,reftx->script,dstr(reftx->value),rawtx->coinaddr,rawtx->script,dstr(rawtx->value));
            if ( reftx->value != rawtx->value || strcmp(reftx->coinaddr,rawtx->coinaddr) != 0 || strcmp(reftx->script,rawtx->script) != 0 )
                err++;
        }
        if ( err != 0 )
        {
            printf("rawblockcmp error for vouts\n");
            getchar();
            return(-6);
        }
    }
    return(0);
}

HUFF *ram_genblock(HUFF *tmphp,struct rawblock *tmp,struct ramchain_info *ram,int32_t blocknum,int32_t format,HUFF **prevhpp)
{
    HUFF *hp = 0;
    int32_t regenflag = 0;
    if ( format == 0 )
        format = 'V';
    if ( 0 && format == 'B' && prevhpp != 0 && (hp= *prevhpp) != 0 && strcmp(ram->name,"BTC") == 0 )
    {
        if ( ram_expand_bitstream(0,tmp,ram,hp) <= 0 )
        {
            char fname[1024],formatstr[16];
            ram_setformatstr(formatstr,'V');
            ram_setfname(fname,ram,blocknum,formatstr);
            //delete_file(fname,0);
            ram_setformatstr(formatstr,'B');
            ram_setfname(fname,ram,blocknum,formatstr);
            //delete_file(fname,0);
            regenflag = 1;
            printf("ram_genblock fatal error generating %s blocknum.%d\n",ram->name,blocknum);
            //exit(-1);
        }
        hp = 0;
    }
    if ( hp == 0 )
    {
        if ( _get_blockinfo(tmp,ram,blocknum) > 0 )
        {
            if ( tmp->blocknum != blocknum )
            {
                printf("WARNING: genblock.%c for %d: got blocknum.%d numtx.%d minted %.8f\n",format,blocknum,tmp->blocknum,tmp->numtx,dstr(tmp->minted));
                return(0);
            }
        } else printf("error _get_blockinfo.(%u)\n",blocknum);
    }
    hp = ram_makehp(tmphp,format,ram,tmp,blocknum);
    return(hp);
}

HUFF *ram_getblock(struct ramchain_info *ram,uint32_t blocknum)
{
    HUFF **hpp;
    if ( (hpp= ram_get_hpptr(&ram->blocks,blocknum)) != 0 )
        return(*hpp);
    return(0);
}

HUFF *ram_verify_Vblock(struct ramchain_info *ram,uint32_t blocknum,HUFF *hp)
{
    int32_t datalen,err;
    HUFF **hpptr;
    if ( hp == 0 && ((hpptr= ram_get_hpptr(&ram->Vblocks,blocknum)) == 0 || (hp = *hpptr) == 0) )
    {
        printf("verify_Vblock: no hp found for hpptr.%p %d\n",hpptr,blocknum);
        return(0);
    }
    if ( (datalen= ram_expand_bitstream(0,ram->Vblocks.R,ram,hp)) > 0 )
    {
        _get_blockinfo(ram->Vblocks.R2,ram,blocknum);
        if ( (err= raw_blockcmp(ram->Vblocks.R2,ram->Vblocks.R)) == 0 )
        {
            //printf("OK ");
            return(hp);
        } else printf("verify_Vblock.%d err.%d\n",blocknum,err);
    } else printf("ram_expand_bitstream returned.%d\n",datalen);
    return(0);
}

HUFF *ram_verify_Bblock(struct ramchain_info *ram,uint32_t blocknum,HUFF *Bhp)
{
    int32_t format,i,n;
    HUFF **hpp,*hp = 0,*retval = 0;
    cJSON *json;
    char *jsonstrs[2],fname[1024],strs[2][2];
    memset(strs,0,sizeof(strs));
    n = 0;
    memset(jsonstrs,0,sizeof(jsonstrs));
    for (format='V'; format>='B'; format-=('V'-'B'),n++)
    {
        strs[n][0] = format;
        ram_setfname(fname,ram,blocknum,strs[n]);
        hpp = ram_get_hpptr((format == 'V') ? &ram->Vblocks : &ram->Bblocks,blocknum);
        if ( format == 'B' && hpp != 0 && *hpp == 0 )
            hp = Bhp;
        else if ( hpp != 0 )
            hp = *hpp;
        if ( hp != 0 )
        {
            //fprintf(stderr,"\n%c: ",format);
            json = 0;
            ram_expand_bitstream(&json,(format == 'V') ? ram->Bblocks.R : ram->Bblocks.R2,ram,hp);
            if ( json != 0 )
            {
                jsonstrs[n] = cJSON_Print(json);
                free_json(json), json = 0;
            }
            if ( format == 'B' )
                retval = hp;
        } else hp = 0;
    }
    if ( jsonstrs[0] == 0 || jsonstrs[1] == 0 || strcmp(jsonstrs[0],jsonstrs[1]) != 0 )
    {
        if ( 1 && jsonstrs[0] != 0 && jsonstrs[1] != 0 )
        {
            if ( raw_blockcmp(ram->Bblocks.R,ram->Bblocks.R2) != 0 )
            {
                printf("(%s).V vs (%s).B)\n",jsonstrs[0],jsonstrs[1]);
                retval = 0;
            }
        }
    }
    for (i=0; i<2; i++)
        if ( jsonstrs[i] != 0 )
            free(jsonstrs[i]);
    return(retval);
}

uint32_t ram_create_block(int32_t verifyflag,struct ramchain_info *ram,struct mappedblocks *blocks,struct mappedblocks *prevblocks,uint32_t blocknum)
{
    char fname[1024],formatstr[16];
    FILE *fp;
    bits256 sha,refsha;
    HUFF *hp,**hpptr,**hps,**prevhps;
    int32_t i,n,numblocks,datalen = 0;
    ram_setformatstr(formatstr,blocks->format);
    prevhps = ram_get_hpptr(prevblocks,blocknum);
    ram_setfname(fname,ram,blocknum,formatstr);
    //printf("check create.(%s)\n",fname);
    if ( blocks->format == 'V' && (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
    {
        fclose(fp);
        // if ( verifyflag == 0 )
        return(0);
    }
    if ( 0 && blocks->format == 'V' )
    {
        if ( _get_blockinfo(blocks->R,ram,blocknum) > 0 )
        {
            if ( (fp= fopen("test","wb")) != 0 )
            {
                hp = blocks->tmphp;
                if ( ram_rawblock_emit(hp,ram,blocks->R) <= 0 )
                    printf("error ram_rawblock_emit.%d\n",blocknum);
                hflush(fp,hp);
                fclose(fp);
                for (i=0; i<(hp->bitoffset>>3); i++)
                    printf("%02x ",hp->buf[i]);
                printf("ram_rawblock_emit\n");
            }
            if ( (hp= ram_genblock(blocks->tmphp,blocks->R2,ram,blocknum,blocks->format,0)) != 0 )
            {
                if ( (fp= fopen(os_compatible_path(fname),"wb")) != 0 )
                {
                    hflush(fp,hp);
                    fclose(fp);
                    compare_files("test",fname);
                    for (i=0; i<(hp->bitoffset>>3); i++)
                        printf("%02x ",hp->buf[i]);
                    printf("ram_genblock\n");
                }
                hclose(hp);
            }
        } else printf("error _get_blockinfo.%d\n",blocknum);
        return(0);
    }
    else if ( blocks->format == 'V' || blocks->format == 'B' )
    {
        //printf("create %s %d\n",formatstr,blocknum);
        if ( (hpptr= ram_get_hpptr(blocks,blocknum)) != 0 )
        {
            if ( (hp= ram_genblock(blocks->tmphp,blocks->R,ram,blocknum,blocks->format,prevhps)) != 0 )
            {
                //printf("block.%d created.%c block.%d numtx.%d minted %.8f\n",blocknum,blocks->format,blocks->R->blocknum,blocks->R->numtx,dstr(blocks->R->minted));
                if ( (fp= fopen(os_compatible_path(fname),"wb")) != 0 )
                {
                    hflush(fp,hp);
                    fclose(fp);
                }
                if ( ram_verify(ram,hp,blocks->format) == blocknum )
                {
                    if ( verifyflag != 0 && ((blocks->format == 'B') ? ram_verify_Bblock(ram,blocknum,hp) : ram_verify_Vblock(ram,blocknum,hp)) == 0 )
                    {
                        printf("error creating %cblock.%d\n",blocks->format,blocknum), datalen = 0;
                        delete_file(fname,0), hclose(hp);
                    }
                    else
                    {
                        datalen = (1 + hp->allocsize);
                        //if ( blocks->format == 'V' )
                        fprintf(stderr," %s CREATED.%c block.%d datalen.%d | RT.%u lag.%d\n",ram->name,blocks->format,blocknum,datalen+1,ram->S.RTblocknum,ram->S.RTblocknum-blocknum);
                        //else fprintf(stderr,"%s.B.%d ",ram->name,blocknum);
                        if ( 0 && *hpptr != 0 )
                        {
                            hclose(*hpptr);
                            *hpptr = 0;
                            printf("OVERWRITE.(%s) size.%ld bitoffset.%d allocsize.%d\n",fname,ftell(fp),hp->bitoffset,hp->allocsize);
                        }
                        *hpptr = hp;
                        if ( blocks->format != 'V' && ram->blocks.hps[blocknum] == 0 )
                            ram->blocks.hps[blocknum] = hp;
                    }
                } //else delete_file(fname,0), hclose(hp);
            } else printf("genblock error %s (%c) blocknum.%u\n",ram->name,blocks->format,blocknum);
        } else printf("%s.%u couldnt get hpp\n",formatstr,blocknum);
    }
    else if ( blocks->format == 64 || blocks->format == 4096 )
    {
        n = blocks->format;
        for (i=0; i<n; i++)
            if ( prevhps[i] == 0 )
                break;
        if ( i == n )
        {
            ram_setfname(fname,ram,blocknum,formatstr);
            hps = ram_get_hpptr(blocks,blocknum);
            if ( ram_save_bitstreams(&refsha,fname,prevhps,n) > 0 )
                numblocks = ram_map_bitstreams(verifyflag,ram,blocknum,&blocks->M[(blocknum-blocks->firstblock) >> blocks->shift],&sha,hps,n,fname,&refsha);
        } else printf("%s prev.%d missing blockptr at %d (%d of %d)\n",ram->name,prevblocks->format,blocknum+i,i,n);
    }
    else
    {
        printf("illegal format to blocknum.%d create.%d\n",blocknum,blocks->format);
        return(0);
    }
    if ( datalen != 0 )
    {
        blocks->sum += datalen;
        blocks->count += (1 << blocks->shift);
    }
    return(datalen);
}

#endif
#endif
