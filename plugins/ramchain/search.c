//
//  ramchainhash.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_ramchainhash_h
#define crypto777_ramchainhash_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "uthash.h"
#include "bits777.c"
#include "system777.c"
#include "ramchain.c"

#define ram_scriptind(ram,hashstr) ram_conv_hashstr(0,1,ram,hashstr,'s')
#define ram_addrind(ram,hashstr) ram_conv_hashstr(0,1,ram,hashstr,'a')
#define ram_txidind(ram,hashstr) ram_conv_hashstr(0,1,ram,hashstr,'t')
// Make sure queries dont autocreate hashtable entries
#define ram_scriptind_RO(permindp,ram,hashstr) ram_conv_hashstr(permindp,0,ram,hashstr,'s')
#define ram_addrind_RO(permindp,ram,hashstr) ram_conv_hashstr(permindp,0,ram,hashstr,'a')
#define ram_txidind_RO(permindp,ram,hashstr) ram_conv_hashstr(permindp,0,ram,hashstr,'t')

#define ram_conv_rawind(hashstr,ram,rawind,type) ram_decode_hashdata(hashstr,type,ram_gethashdata(ram,type,rawind))
#define ram_txid(hashstr,ram,rawind) ram_conv_rawind(hashstr,ram,rawind,'t')
#define ram_addr(hashstr,ram,rawind) ram_conv_rawind(hashstr,ram,rawind,'a')
#define ram_script(hashstr,ram,rawind) ram_conv_rawind(hashstr,ram,rawind,'s')
char *ram_decode_hashdata(char *strbuf,char type,uint8_t *hashdata);

#define ram_addrpayloads(addrptrp,numpayloadsp,ram,addr) ram_payloads(addrptrp,numpayloadsp,ram,addr,'a')
#define ram_txpayloads(txptrp,numpayloadsp,ram,txidstr) ram_payloads(txptrp,numpayloadsp,ram,txidstr,'t')

#endif
#else
#ifndef crypto777_ramchainhash_c
#define crypto777_ramchainhash_c

#ifndef crypto777_ramchainhash_h
#define DEFINES_ONLY
#include __BASE_FILE__
#undef DEFINES_ONLY
#endif

void ram_sethashtype(char *str,int32_t type)
{
    switch ( type )
    {
        case 'a': strcpy(str,"addr"); break;
        case 't': strcpy(str,"txid"); break;
        case 's': strcpy(str,"script"); break;
        default: sprintf(str,"type%d",type); break;
    }
}

void huff_compressionvars_fname(int32_t readonly,char *fname,char *coinstr,char *typestr,int32_t subgroup)
{
    char *dirname = MGWROOT;
    if ( subgroup < 0 )
        sprintf(fname,"%s/ramchains/%s/%s.%s",dirname,coinstr,coinstr,typestr);
    else sprintf(fname,"%s/ramchains/%s/%s/%s.%d",dirname,coinstr,typestr,coinstr,subgroup);
}

void ram_sethashname(char fname[1024],struct ramchain_hashtable *hash,int32_t newflag)
{
    char str[128],typestr[128];
    ram_sethashtype(str,hash->type);
    if ( newflag != 0 )
        sprintf(typestr,"%s.new",str);
    else strcpy(typestr,str);
    huff_compressionvars_fname(0,fname,hash->coinstr,typestr,-1);
}

int32_t ram_addhash(struct ramchain_hashtable *hash,struct ramchain_hashptr *hp,void *ptr,int32_t datalen)
{
    hp->rawind = ++hash->ind;
    HASH_ADD_KEYPTR(hh,hash->table,ptr,datalen,hp);
    if ( 0 && hash->type == 't' )
    {
        char hexbytes[8192];
        struct ramchain_hashptr *checkhp;
        init_hexbytes_noT(hexbytes,ptr,datalen);
        HASH_FIND(hh,hash->table,ptr,datalen,checkhp);
        printf("created.(%s) ind.%d | checkhp.%p\n",hexbytes,hp->rawind,checkhp);
    }
    if ( (hash->ind + 1) >= hash->numalloc )
    {
        hash->numalloc += 512; // 4K page at a time
        hash->ptrs = realloc(hash->ptrs,sizeof(*hash->ptrs) * hash->numalloc);
        memset(&hash->ptrs[hash->ind],0,(hash->numalloc - hash->ind) * sizeof(*hash->ptrs));
    }
    hash->ptrs[hp->rawind] = hp;
    return(0);
}

uint8_t *ram_encode_hashstr(int32_t *datalenp,uint8_t *data,char type,char *hashstr)
{
    uint8_t varbuf[9];
    char buf[8192];
    int32_t varlen,datalen,scriptmode = 0;
    *datalenp = 0;
    if ( type == 's' )
    {
        if ( hashstr[0] == 0 )
            return(0);
        strcpy(buf,hashstr);
        if ( (scriptmode = ram_calc_scriptmode(&datalen,&data[9],buf,1)) < 0 )
        {
            printf("encode_hashstr: scriptmode.%d for (%s)\n",scriptmode,hashstr);
            exit(-1);
        }
    }
    else if ( type == 't' )
    {
        datalen = (int32_t)(strlen(hashstr) >> 1);
        if ( datalen > 4096 )
        {
            printf("encode_hashstr: type.%d (%c) datalen.%d > sizeof(data) %d\n",type,type,(int)datalen,4096);
            exit(-1);
        }
        decode_hex(&data[9],datalen,hashstr);
    }
    else if ( type == 'a' )
    {
        datalen = (int32_t)strlen(hashstr) + 1;
        memcpy(&data[9],hashstr,datalen);
    }
    else
    {
        printf("encode_hashstr: unsupported type.%d (%c)\n",type,type);
        exit(-1);
    }
    if ( datalen > 0 )
    {
        varlen = hcalc_varint(varbuf,datalen);
        memcpy(&data[9-varlen],varbuf,varlen);
        //HASH_FIND(hh,hash->table,&ptr[-varlen],datalen+varlen,hp);
        *datalenp = (datalen + varlen);
        return(&data[9-varlen]);
    }
    return(0);
}

char *ram_decode_hashdata(char *strbuf,char type,uint8_t *hashdata)
{
    uint64_t varint;
    int32_t datalen,scriptmode;
    strbuf[0] = 0;
    if ( hashdata == 0 )
        return(0);
    hashdata += hdecode_varint(&varint,hashdata,0,9);
    datalen = (int32_t)varint;
    if ( type == 's' )
    {
        if ( (scriptmode= ram_expand_scriptdata(strbuf,hashdata,(uint32_t)datalen)) < 0 )
        {
            printf("decode_hashdata: scriptmode.%d for (%s)\n",scriptmode,strbuf);
            return(0);
        }
        //printf("EXPANDSCRIPT.(%c) -> [%s]\n",scriptmode,strbuf);
    }
    else if ( type == 't' )
    {
        if ( datalen > 128 )
        {
            init_hexbytes_noT(strbuf,hashdata,64);
            printf("decode_hashdata: type.%d (%c) datalen.%d > sizeof(data) %d | (%s)\n",type,type,(int)datalen,128,strbuf);
            exit(-1);
        }
        init_hexbytes_noT(strbuf,hashdata,datalen);
    }
    else if ( type == 'a' )
        memcpy(strbuf,hashdata,datalen);
    else
    {
        printf("decode_hashdata: unsupported type.%d\n",type);
        exit(-1);
    }
    return(strbuf);
}

struct ramchain_hashtable *ram_gethash(struct ramchain_info *ram,char type)
{
    switch ( type )
    {
        case 'a': return(&ram->addrhash); break;
        case 't': return(&ram->txidhash); break;
        case 's': return(&ram->scripthash); break;
    }
    return(0);
}

struct ramchain_hashptr *ram_gethashptr(struct ramchain_info *ram,char type,uint32_t rawind)
{
    struct ramchain_hashtable *table;
    struct ramchain_hashptr *ptr = 0;
    table = ram_gethash(ram,type);
    if ( table != 0 && rawind > 0 && rawind <= table->ind && table->ptrs != 0 )
        ptr = table->ptrs[rawind];
    return(ptr);
}

void *ram_gethashdata(struct ramchain_info *ram,char type,uint32_t rawind)
{
    struct ramchain_hashptr *ptr;
    if ( (ptr= ram_gethashptr(ram,type,rawind)) != 0 )
        return(ptr->hh.key);
    return(0);
}

int32_t ram_script_multisig(struct ramchain_info *ram,uint32_t scriptind)
{
    char *hex;
    if ( (hex= ram_gethashdata(ram,'s',scriptind)) != 0 && hex[1] == 'm' )
    {
        printf("(%02x %02x %02x (%c)) ",hex[0],hex[1],hex[2],hex[1]);
        return(1);
    }
    return(0);
}

int32_t ram_script_nonstandard(struct ramchain_info *ram,uint32_t scriptind)
{
    char *hex;
    if ( (hex= ram_gethashdata(ram,'s',scriptind)) != 0 && hex[2] != 'm' && hex[2] != 's' )
        return(1);
    return(0);
}

struct rampayload *ram_gethashpayloads(struct ramchain_info *ram,char type,uint32_t rawind)
{
    struct ramchain_hashptr *ptr;
    if ( (ptr= ram_gethashptr(ram,type,rawind)) != 0 )
        return(ptr->payloads);
    return(0);
}

struct ramchain_hashptr *ram_hashdata_search(char *coinstr,struct alloc_space *mem,int32_t createflag,struct ramchain_hashtable *hash,uint8_t *hashdata,int32_t datalen)
{
    struct ramchain_hashptr *ptr = 0;
    char fname[512];
    void *newptr;
    if ( hash != 0 )
    {
        HASH_FIND(hh,hash->table,hashdata,datalen,ptr);
        if ( ptr == 0 && createflag != 0 )
        {
            if ( hash->newfp == 0 )
            {
                ram_sethashname(fname,hash,0);
                hash->newfp = fopen(os_compatible_path(fname),"wb");
                if ( hash->newfp != 0 )
                {
                    printf("couldnt create (%s)\n",fname);
                    exit(-1);
                }
            }
            ptr = (MAP_HUFF != 0) ? permalloc(coinstr,mem,sizeof(*ptr),3) : calloc(1,sizeof(*ptr));
            newptr = (MAP_HUFF != 0) ? permalloc(coinstr,mem,datalen,4) : calloc(1,datalen);
            memcpy(newptr,hashdata,datalen);
            //*createdflagp = 1;
            if ( 0 )
            {
                char hexstr[8192];
                init_hexbytes_noT(hexstr,newptr,datalen);
                printf("save.(%s) at %ld datalen.%d newfp.%p\n",hexstr,ftell(hash->newfp),datalen,hash->newfp);// getchar();//while ( 1 ) sleep(1);
            }
            if ( hash->newfp != 0 )
            {
                if ( fwrite(newptr,1,datalen,hash->newfp) != datalen )
                {
                    printf("error saving type.%d ind.%d datalen.%d\n",hash->type,hash->ind,datalen);
                    exit(-1);
                }
                fflush(hash->newfp);
            }
            //printf("add %d byte entry to hashtable %c\n",datalen,hash->type);
            ram_addhash(hash,ptr,newptr,datalen);
        } //else printf("found %d bytes ind.%d\n",datalen,ptr->rawind);
    } else printf("ram_hashdata_search null hashtable\n");
    //#endif
    return(ptr);
}

struct ramchain_hashptr *ram_hashsearch(char *coinstr,struct alloc_space *mem,int32_t createflag,struct ramchain_hashtable *hash,char *hashstr,char type)
{
    uint8_t data[4097],*hashdata;
    struct ramchain_hashptr *ptr = 0;
    int32_t datalen;
    if ( hash != 0 && (hashdata= ram_encode_hashstr(&datalen,data,type,hashstr)) != 0 )
        ptr = ram_hashdata_search(coinstr,mem,createflag,hash,hashdata,datalen);
    return(ptr);
}

uint32_t ram_conv_hashstr(uint32_t *permindp,int32_t createflag,struct ramchain_info *ram,char *hashstr,char type)
{
    char nullstr[6] = { 5, 'n', 'u', 'l', 'l', 0 };
    struct ramchain_hashptr *ptr = 0;
    struct ramchain_hashtable *hash;
    if ( permindp != 0 )
        *permindp = 0;
    if ( hashstr == 0 || hashstr[0] == 0 )
        hashstr = nullstr;
    hash = ram_gethash(ram,type);
    if ( (ptr= ram_hashsearch(ram->name,&ram->Perm,createflag,hash,hashstr,type)) != 0 )
    {
        if ( (hash->ind + 1) > ram->maxind )
            ram->maxind = (hash->ind + 1);
        if ( permindp != 0 )
            *permindp = ptr->permind;
        return(ptr->rawind);
    }
    else return(0);
}

struct rampayload *ram_payloads(struct ramchain_hashptr **ptrp,int32_t *numpayloadsp,struct ramchain_info *ram,char *hashstr,char type)
{
    struct ramchain_hashptr *ptr = 0;
    uint32_t rawind;
    *numpayloadsp = 0;
    if ( ptrp != 0 )
        *ptrp = 0;
    if ( (rawind= ram_conv_hashstr(0,0,ram,hashstr,type)) != 0 && (ptr= ram_gethashptr(ram,type,rawind)) != 0 )
    {
        if ( ptrp != 0 )
            *ptrp = ptr;
        *numpayloadsp = ptr->numpayloads;
        return(ptr->payloads);
    }
    else return(0);
}

struct address_entry *ram_address_entry(struct address_entry *destbp,struct ramchain_info *ram,char *txidstr,int32_t vout)
{
    int32_t numpayloads;
    struct rampayload *payloads;
    if ( (payloads= ram_txpayloads(0,&numpayloads,ram,txidstr)) != 0 )
    {
        if ( vout < payloads[0].B.v )
        {
            *destbp = payloads[vout].B;
            return(destbp);
        }
    }
    return(0);
}

struct rampayload *ram_getpayloadi(struct ramchain_hashptr **ptrp,struct ramchain_info *ram,char type,uint32_t rawind,uint32_t i)
{
    struct ramchain_hashptr *ptr;
    if ( ptrp != 0 )
        *ptrp = 0;
    if ( (ptr= ram_gethashptr(ram,type,rawind)) != 0 && i < ptr->numpayloads && ptr->payloads != 0 )
    {
        if ( ptrp != 0 )
            *ptrp = ptr;
        return(&ptr->payloads[i]);
    }
    return(0);
}

#endif
#endif
