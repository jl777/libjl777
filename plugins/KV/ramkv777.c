
#ifdef DEFINES_ONLY
#ifndef ramkv777_h
#define ramkv777_h

#include <stdio.h>
#include <stdint.h>
#include <memory.h>
#include "../includes/portable777.h"
#include "../utils/bits777.c"


#endif
#else
#ifndef ramkv777_c
#define ramkv777_c

#ifndef ramkv777_h
#define DEFINES_ONLY
#include "ramkv777.c"
#undef DEFINES_ONLY
#endif

void ramkv777_lock(struct ramkv777 *kv)
{
    if ( kv->threadsafe != 0 )
        portable_mutex_lock(&kv->mutex);
}

void ramkv777_unlock(struct ramkv777 *kv)
{
    if ( kv->threadsafe != 0 )
        portable_mutex_unlock(&kv->mutex);
}

int32_t ramkv777_delete(struct ramkv777 *kv,void *key)
{
    int32_t retval = -1; struct ramkv777_item *ptr = 0;
    if ( kv == 0 )
        return(-1);
    ramkv777_lock(kv);
    HASH_FIND(hh,kv->table,key,kv->keysize,ptr);
    if ( ptr != 0 )
    {
        HASH_DELETE(hh,kv->table,ptr);
        free(ptr);
        retval = 0;
    }
    ramkv777_lock(kv);
    return(retval);
}

void *ramkv777_write(struct ramkv777 *kv,void *key,void *value,int32_t valuesize)
{
    struct ramkv777_item *item = 0; int32_t keysize = kv->keysize;
    if ( kv == 0 )
        return(0);
    ramkv777_lock(kv);
    HASH_FIND(hh,kv->table,key,keysize,item);
    if ( item != 0 )
    {
        if ( valuesize == item->valuesize )
        {
            if ( memcmp(ramkv777_itemvalue(kv,item),value,valuesize) != 0 )
            {
                update_sha256(kv->sha256.bytes,&kv->state,key,kv->keysize);
                update_sha256(kv->sha256.bytes,&kv->state,value,valuesize);
                memcpy(ramkv777_itemvalue(kv,item),value,valuesize);
            }
            ramkv777_unlock(kv);
            return(item);
        }
        HASH_DELETE(hh,kv->table,item);
        free(item);
        update_sha256(kv->sha256.bytes,&kv->state,key,kv->keysize);
    }
    item = calloc(1,ramkv777_itemsize(kv,valuesize));
    memcpy(item->keyvalue,key,kv->keysize);
    memcpy(ramkv777_itemvalue(kv,item),value,valuesize);
    item->valuesize = valuesize;
    item->rawind = (kv->numkeys++ * ACCTS777_MAXRAMKVS) | kv->kvind;
    //printf("add.(%s) kv->numkeys.%d keysize.%d valuesize.%d\n",kv->name,kv->numkeys,keysize,valuesize);
    HASH_ADD_KEYPTR(hh,kv->table,ramkv777_itemkey(item),kv->keysize,item);
    update_sha256(kv->sha256.bytes,&kv->state,key,kv->keysize);
    update_sha256(kv->sha256.bytes,&kv->state,value,valuesize);
    ramkv777_unlock(kv);
    if ( kv->dispflag != 0 )
        fprintf(stderr,"%016llx ramkv777_write numkeys.%d kv.%p table.%p write key.%08x size.%d, value.(%08x) size.%d\n",(long long)kv->sha256.txid,kv->numkeys,kv,kv->table,*(int32_t *)key,keysize,_crc32(0,value,valuesize),valuesize);
    return(ramkv777_itemvalue(kv,item));
}

void *ramkv777_read(int32_t *valuesizep,struct ramkv777 *kv,void *key)
{
    struct ramkv777_item *item = 0;  int32_t keysize = kv->keysize;
    if ( kv == 0 )
        return(0);
    ramkv777_lock(kv);
    HASH_FIND(hh,kv->table,key,keysize,item);
    ramkv777_unlock(kv);
    if ( item != 0 )
    {
        if ( valuesizep != 0 )
            *valuesizep = item->valuesize;
        return(ramkv777_itemvalue(kv,item));
    }
    if ( valuesizep != 0 )
        *valuesizep = 0;
    return(0);
}

void *ramkv777_iterate(struct ramkv777 *kv,void *args,void *(*iterator)(struct ramkv777 *kv,void *args,void *key,void *value,int32_t valuesize))
{
    struct ramkv777_item *item,*tmp; void *retval = 0;
    if ( kv == 0 )
        return(0);
    ramkv777_lock(kv);
    HASH_ITER(hh,kv->table,item,tmp)
    {
        if ( (retval= (*iterator)(kv,args!=0?args:item,item->keyvalue,ramkv777_itemvalue(kv,item),item->valuesize)) != 0 )
        {
            ramkv777_unlock(kv);
            return(retval);
        }
    }
    ramkv777_unlock(kv);
    return(0);
}

struct ramkv777 *ramkv777_init(int32_t kvind,char *name,int32_t keysize,int32_t threadsafe)
{
    struct ramkv777 *kv;
    printf("ramkv777_init.(%s)\n",name);
    kv = calloc(1,sizeof(*kv));
    strcpy(kv->name,name);
    kv->threadsafe = threadsafe, kv->keysize = keysize, kv->kvind = kvind;//, kv->dispflag = 1;
    portable_mutex_init(&kv->mutex);
    update_sha256(kv->sha256.bytes,&kv->state,0,0);
    return(kv);
}

void ramkv777_free(struct ramkv777 *kv)
{
    struct ramkv777_item *ptr,*tmp;
    if ( kv != 0 )
    {
        HASH_ITER(hh,kv->table,ptr,tmp)
        {
            HASH_DEL(kv->table,ptr);
            free(ptr);
        }
        free(kv);
    }
}

int32_t ramkv777_clone(struct ramkv777 *clone,struct ramkv777 *kv)
{
    struct ramkv777_item *item,*tmp; int32_t n = 0;
    if ( kv != 0 )
    {
        HASH_ITER(hh,kv->table,item,tmp)
        {
            ramkv777_write(clone,item->keyvalue,ramkv777_itemvalue(kv,item),item->valuesize);
            n++;
        }
    }
    return(n);
}

struct ramkv777_item *ramkv777_itemptr(struct ramkv777 *kv,void *value)
{
    struct ramkv777_item *item = 0;
    if ( kv != 0 && value != 0 )
    {
        value = (void *)((long)value - (kv)->keysize);
        item = (void *)((long)value - ((long)item->keyvalue - (long)item));
    }
    return(item);
}

#endif
#endif


