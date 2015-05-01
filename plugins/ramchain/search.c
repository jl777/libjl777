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
#include "cointx.c"
#include "system777.c"


struct address_entry { uint64_t blocknum:32,txind:15,vinflag:1,v:14,spent:1,isinternal:1; };

struct ramchain_hashtable
{
    char coinstr[16];
    struct db777 *DB;
    dbobj *ptrs;
    long endpermpos;
    uint32_t ind,numalloc,maxind;
    uint8_t type;
};


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
#include "cointx.c"
#include "search.c"
#include "db777.c"
#undef DEFINES_ONLY
#endif

char *ram_searchpermind(struct ramchain_hashtable *hash,uint32_t permind)
{
    int32_t i,len;
    dbobj obj;
    //printf("(%c) searchpermind.(%d) ind.%d\n",type,permind,hash->ind);
    for (i=1; i<=hash->ind; i++)
        if ( (obj= hash->ptrs[i]) != 0 && db777_ind(hash->DB,obj,"p") == permind )
            return(sp_get(obj,"value",&len));
    return(0);
}

dbobj ram_addhash(struct ramchain_hashtable *hash,char *key,void *value,int32_t len)
{
    dbobj obj = 0;
    char numstr[16];
    int32_t rawind;
    db777_add(hash->DB,key,value,len);
    obj = db777_find(hash->DB,key);
    rawind = ++hash->ind;
    sprintf(numstr,"%u",rawind);
    if ( sp_set(obj,"i",numstr,strlen(numstr)) != 0 )
    {
        printf("ram_addhash: error adding field to object\n");
        sp_destroy(obj);
        return(0);
    }
    if ( (hash->ind + 1) >= hash->numalloc )
    {
        hash->numalloc += 512; // 4K page at a time
        hash->ptrs = realloc(hash->ptrs,sizeof(*hash->ptrs) * hash->numalloc);
        memset(&hash->ptrs[hash->ind],0,(hash->numalloc - hash->ind) * sizeof(*hash->ptrs));
    }
    hash->ptrs[rawind] = obj;
    return(obj);
}

dbobj ram_hashdata_search(int32_t createflag,struct ramchain_hashtable *hash,uint8_t *hashdata,int32_t datalen)
{
    char _key[8192],*key;
    dbobj obj = 0;
    if ( datalen > sizeof(key) )
        key = malloc(datalen+1);
    else key = _key;
    init_hexbytes_noT(key,hashdata,datalen);
    if ( hash != 0 && (obj= db777_find(hash->DB,key)) == 0 && createflag != 0 )
        obj = ram_addhash(hash,key,hashdata,datalen);
    if ( key != _key )
        free(key);
    return(obj);
}

/*dbobj ram_hashsearch(int32_t createflag,struct ramchain_hashtable *hash,char *hashstr,char type)
{
    uint8_t data[4097],*hashdata;
    dbobj obj = 0;
    int32_t datalen;
    if ( hash != 0 && (hashdata= ram_encode_hashstr(&datalen,data,type,hashstr)) != 0 )
        obj = ram_hashdata_search(createflag,hash,hashdata,datalen);
    return(obj);
}*/

uint32_t ram_conv_hashstr(uint32_t *permindp,int32_t createflag,struct ramchain_hashtable *hash,char *hashstr)
{
    //static char nullstr[6] = { 5, 'n', 'u', 'l', 'l', 0 };
    ;
    dbobj *obj = 0;
    if ( permindp != 0 )
        *permindp = 0;
    //if ( hashstr == 0 || hashstr[0] == 0 )
    //    hashstr = nullstr;
    if ( (obj= db777_find(hash->DB,hashstr)) != 0 )
    {
        if ( (hash->ind + 1) > hash->maxind )
            hash->maxind = (hash->ind + 1);
        if ( permindp != 0 )
            *permindp = db777_ind(hash->DB,obj,"p");
        return(db777_ind(hash->DB,obj,"i"));
    }
    else return(0);
}

#endif
#endif
