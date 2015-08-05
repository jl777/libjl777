//
//  storage.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <memory.h>

#ifdef DEFINES_ONLY
#ifndef crypto777_storage_h
#define crypto777_storage_h
#include "mutex.h"
#include "../includes/uthash.h"
#include "../utils/NXT777.c"
#include "../utils/files777.c"
#include "../common/system777.c"
#define portable_mutex_t struct nn_mutex
#define portable_mutex_init nn_mutex_init
#define portable_mutex_lock nn_mutex_lock
#define portable_mutex_unlock nn_mutex_unlock

#define KV777_ALIGNBITS 2
#define KV777_MAXKEYSIZE 65536
#define KV777_MAXVALUESIZE (1 << 30)
#define KV777_ABORTITERATOR ((void *)((long)kv777_iterate))

struct kv777_hdditem { uint32_t crc,valuesize,keysize; uint8_t value[]; };
struct kv777_item { UT_hash_handle hh; struct kv777_item *next,*prev; long offset; uint32_t ind,itemsize; struct kv777_hdditem *item; };
struct kv777_flags { char rw,hdd,threadsafe,writebehind,mmap,tbd0,tbd1,tbd2; };
struct kv777
{
    char name[64],fname[512],path[512];
    portable_mutex_t mutex;
    struct kv777_item *table,*hddlist;
    struct kv777_flags flags;
    FILE *fp; void *fileptr; uint64_t mapsize,offset;
    long totalkeys,totalvalues;
    int32_t numkeys,netkeys,netvalues,dontrelay,dispflag;
};

void kv777_defaultflags(struct kv777_flags *flags);
struct kv777 *kv777_init(char *path,char *name,struct kv777_flags *flags); // kv777_init IS NOT THREADSAFE!
struct kv777_item *kv777_write(struct kv777 *kv,void *key,int32_t keysize,void *value,int32_t valuesize);
void *kv777_read(struct kv777 *kv,void *key,int32_t keysize,void *value,int32_t *valuesizep,int32_t retitem);
int32_t kv777_addstr(struct kv777 *kv,char *key,char *value);
char *kv777_findstr(char *retbuf,int32_t max,struct kv777 *kv,char *key);

int32_t kv777_delete(struct kv777 *kv,void *key,int32_t keysize);
int32_t kv777_purge(struct kv777 *kv);
void kv777_flush(char *);

int32_t kv777_idle();
void *kv777_iterate(struct kv777 *kv,void *args,int32_t allitems,void *(*iterator)(struct kv777 *kv,void *args,void *key,int32_t keysize,void *value,int32_t valuesize));

struct dKV_node { struct endpoint endpoint; uint64_t nxt64bits,stake; int64_t penalty; uint32_t activetime,nodei; int32_t sock; };

struct dKV777
{
    char name[64],protocol[64],NXTADDR[64],NXTACCTSECRET[2048],endpointstr[64];
    struct kv777 *approvals,*nodes,**kvs;
    uint64_t approvalfifo[256];
    struct endpoint *connections;
    double pinggap,lastping;
    portable_mutex_t mutex;
    int32_t pubsock,subsock,num,max,numkvs,fifoind,lastfifoi; uint32_t totalnodes,ind,keysize,flags; uint16_t port;
};

struct dKV777 *dKV777_init(char *name,char *protocol,struct kv777 **kvs,int32_t numkvs,uint32_t flags,int32_t pubsock,int32_t subsock,struct endpoint *connections,int32_t num,int32_t max,uint16_t port,double pinggap);
int32_t dKV777_addnode(struct dKV777 *dKV,struct endpoint *ep);
int32_t dKV777_removenode(struct dKV777 *dKV,struct endpoint *ep);
int32_t dKV777_blacklist(struct dKV777 *dKV,struct endpoint *ep,int32_t penalty);
char *dKV777_ping(struct dKV777 *dKV);

uint32_t dKV777_write(struct dKV777 *dKV,struct kv777 *kv,uint64_t ownerbits,void *key,int32_t keysize,void *value,int32_t valuesize);
void *dKV777_read(struct dKV777 *dKV,struct kv777 *kv,uint64_t ownerbits,void *key,int32_t keysize,void *value,int32_t *valuesizep);
void dKV777_deleteind(struct dKV777 *dKV,struct kv777 *kv,uint64_t ownerbits,uint32_t ind);
void dKV777_delete(struct dKV777 *dKV,struct kv777 *kv,uint64_t ownerbits,void *key,int32_t keysize);
//void dKV777_deleteowner(struct dKV777 *dKV,struct kv777 *kv,uint64_t ownerbits);

void set_KV777_globals(struct dKV777 **relaysptr,char *transport,void *NXTACCTSECRET,int32_t secretlen,char *SERVICESECRET,char *relayendpoint);

struct kv777_info
{
    char PATH[512],NXTADDR[64],SERVICENXT[64],relayendpoint[128],transport[16],protocol[16];
    uint8_t mysecret[32],mypubkey[32],servicesecret[32],servicepubkey[32],NXTACCTSECRET[2048],SERVICESECRET[2048];
    struct kv777 **KVS; struct dKV777 **dKVs,**relays_handle;
    uint64_t nxt64bits,service64bits;
    double Last_kvupdate;
    int32_t Num_kvs,Num_dKVs,mmapflag,readyflag,secretlen,servicelen;
}; extern struct kv777_info KV777;

#endif
#else
#ifndef crypto777_storage_c
#define crypto777_storage_c


struct kv777_info KV777;
uint32_t _crc32(uint32_t crc,const void *buf,size_t size);

#ifndef crypto777_storage_h
#define DEFINES_ONLY
#include "kv777.c"
#undef DEFINES_ONLY
#endif


struct kv777 *kv777_find(char *name)
{
    int32_t i;
    if ( name != 0 && KV777.Num_kvs > 0 )
    {
        for (i=0; i<KV777.Num_kvs; i++)
            if ( strcmp(KV777.KVS[i]->name,name) == 0 )
                return(KV777.KVS[i]);
    }
    return(0);
}

void kv777_defaultflags(struct kv777_flags *flags)
{
    memset(flags,0,sizeof(*flags));
    flags->rw = flags->hdd = flags->threadsafe = 1;
    flags->writebehind = 0;
    flags->mmap = 0;//KV777.mmapflag;
}

void kv777_lock(struct kv777 *kv)
{
    if ( kv->flags.threadsafe != 0 )
        portable_mutex_lock(&kv->mutex);
}

void kv777_unlock(struct kv777 *kv)
{
    if ( kv->flags.threadsafe != 0 )
        portable_mutex_unlock(&kv->mutex);
}

int32_t kv777_isdeleted(struct kv777_hdditem *item) { return(((item)->valuesize & (1 << 31)) != 0); }
void kv777_setdeleted(struct kv777_hdditem *item) { (item)->valuesize |= (1 << 31); }
uint32_t kv777_valuesize(uint32_t valuesize) { return((valuesize) & ~(1 << 31)); }

uint32_t kv777_itemsize(uint32_t keysize,uint32_t valuesize)
{
    int32_t alignmask,alignsize; uint32_t size;
    valuesize = kv777_valuesize(valuesize);
    size = (uint32_t)(sizeof(struct kv777_hdditem) + valuesize + keysize);
    alignsize = (1 << KV777_ALIGNBITS), alignmask = (alignsize - 1);
    if ( KV777_ALIGNBITS > 0 && (valuesize & alignmask) != 0 )
        size += alignsize - (valuesize & alignmask);
    if ( KV777_ALIGNBITS > 0 && (keysize & alignmask) != 0 )
        size += alignsize - (keysize & alignmask);
    return(size);
}

void *kv777_itemkey(struct kv777_hdditem *item)
{
    int32_t alignmask,alignsize,size = item->valuesize;
    alignsize = (1 << KV777_ALIGNBITS), alignmask = (alignsize - 1);
    if ( KV777_ALIGNBITS > 0 && (size & alignmask) != 0 )
        size += alignsize - (size & alignmask);
    return(&item->value[size]);
}

struct kv777_hdditem *kv777_hdditem(uint32_t *allocsizep,void *buf,int32_t maxsize,void *key,int32_t keysize,void *value,int32_t valuesize)
{
    struct kv777_hdditem *item; uint32_t size;
    *allocsizep = size = kv777_itemsize(keysize,valuesize);
    if ( size > maxsize || buf == 0 )
        item = calloc(1,size);
    else item = (struct kv777_hdditem *)buf;
    item->valuesize = valuesize, item->keysize = keysize;
    memcpy(item->value,value,valuesize);
    memcpy(kv777_itemkey(item),key,keysize);
    item->crc = _crc32(0,(void *)((long)item + sizeof(item->crc)),size - sizeof(item->crc));
    //for (int i=0; i<size; i++)
    //    printf("%02x ",((uint8_t *)item)[i]);
    //printf("-> itemsize.%d | %p value.%p %d key.%p %d (%s %s)\n",size,item,item->value,item->valuesize,kv777_itemkey(item),item->keysize,item->value,kv777_itemkey(item));
    return(item);
}

struct kv777_hdditem *kv777_load(uint32_t *allocflagp,uint32_t *itemsizep,struct kv777 *kv)
{
    uint32_t crc,valuesize,keysize,size; long fpos; struct kv777_hdditem *item = 0;
    if ( kv->fileptr != 0 )
    {
        item = (void *)((long)kv->fileptr + kv->offset);
        *itemsizep = size = kv777_itemsize(item->keysize,item->valuesize);
        if ( (kv->offset + size) <= kv->mapsize )
        {
            *allocflagp = 0;
            kv->offset += size;
            return(item);
        }
    }
    fpos = kv->offset;
    fseek(kv->fp,fpos,SEEK_SET);
    *allocflagp = 1;
    if ( fread(&crc,1,sizeof(crc),kv->fp) == sizeof(crc) && crc != 0 )
    {
        if ( fread(&valuesize,1,sizeof(valuesize),kv->fp) != sizeof(valuesize) )
        {
            printf("valuesize read error after %d items\n",kv->numkeys);
            return(0);
        }
        if ( fread(&keysize,1,sizeof(keysize),kv->fp) != sizeof(keysize) || keysize > KV777_MAXKEYSIZE || valuesize > KV777_MAXVALUESIZE )
        {
            printf("keysize read error after %d items keysize.%u valuesize.%u\n",kv->numkeys,keysize,valuesize);
            return(0);
        }
        *itemsizep = size = kv777_itemsize(keysize,valuesize);
        item = calloc(1,size);
        item->valuesize = valuesize, item->keysize = keysize;
        if ( fread(item->value,1,size - sizeof(*item),kv->fp) != (size - sizeof(*item)) )
        {
            printf("valuesize.%d read error after %d items\n",valuesize,kv->numkeys);
            return(0);
        }
        item->crc = _crc32(0,(void *)((long)item + sizeof(item->crc)),size - sizeof(item->crc));
        if ( crc != item->crc )
        {
            uint32_t i;
            for (i=0; i<size; i++)
                printf("%02x ",((uint8_t *)item)[i]);
            printf("kv777.%s error item.%d crc.%x vs calccrc.%x valuesize.%u\n",kv->name,kv->numkeys,crc,item->crc,valuesize);
            return(0);
        }
    } else item = 0;
    kv->offset = ftell(kv->fp);
    return(item);
}

void kv777_free(struct kv777 *kv,struct kv777_item *ptr,int32_t freeall)
{
    if ( kv->fileptr == 0 || (long)ptr->item < (long)kv->fileptr || (long)ptr->item >= (long)kv->fileptr+kv->mapsize )
        free(ptr->item);
    if ( freeall != 0 )
        free(ptr);
}

int32_t kv777_update(struct kv777 *kv,struct kv777_item *ptr)
{
    struct kv777_hdditem *item; uint32_t valuesize; long savepos; int32_t retval = -1;
    if ( kv->fp == 0 )
        return(-1);
    item = (void *)ptr->item;
    savepos = ftell(kv->fp);
    if ( kv777_isdeleted(item) != 0 )
    {
        fseek(kv->fp,ptr->offset + sizeof(item->crc),SEEK_SET);
        if ( fread(&valuesize,1,sizeof(valuesize),kv->fp) == sizeof(valuesize) )
        {
            if ( kv777_isdeleted(item) == 0 )
            {
                kv777_setdeleted(item);
                fseek(kv->fp,ptr->offset + sizeof(item->crc),SEEK_SET);
                if ( fwrite(&item->valuesize,1,sizeof(item->valuesize),kv->fp) == sizeof(valuesize) )
                    retval = 0;
            }
        } else printf("kv777.%s read error at %ld\n",kv->name,savepos);
        if ( retval != 0 )
            printf("error reading valuesize at fpos.%ld\n",ftell(kv->fp));
        fseek(kv->fp,savepos,SEEK_SET);
        kv777_free(kv,ptr,1);
        return(retval);
    }
    else if ( ptr->offset < 0 )
    {
        ptr->offset = ftell(kv->fp);
        if ( fwrite(ptr->item,1,ptr->itemsize,kv->fp) != ptr->itemsize )
        {
            printf("fwrite.%s error at fpos.%ld\n",kv->name,ftell(kv->fp));
            exit(-1);
        }
        return(0);
    }
    else
    {
        fseek(kv->fp,ptr->offset,SEEK_SET);
        if ( ftell(kv->fp) != ptr->offset )
        {
            printf("kv777 seek warning %ld != %ld\n",ftell(kv->fp),ptr->offset);
            fseek(kv->fp,savepos,SEEK_SET);
            ptr->offset = savepos;
            if ( ftell(kv->fp) != savepos )
            {
                printf("kv777.%s seek error %ld != savepos.%ld\n",kv->name,ftell(kv->fp),savepos);
                exit(-1);
            }
        }
        if ( Debuglevel > 3 )
            printf("updated item.%d at %ld siz.%d\n",ptr->ind,ptr->offset,ptr->itemsize);
        if ( fwrite(ptr->item,1,ptr->itemsize,kv->fp) != ptr->itemsize )
        {
            printf("fwrite.%s error at fpos.%ld\n",kv->name,ftell(kv->fp));
            exit(-1);
        }
        if ( 0 && kv->dispflag != 0 )
        {
            for (int i=0; i<ptr->itemsize; i++)
                fprintf(stderr,"%02x ",((uint8_t *)ptr->item)[i]);
            fprintf(stderr,"(%s).(%s)\n",kv->path,kv->name);
        }
        fseek(kv->fp,savepos,SEEK_SET);
        return(0);
    }
     //printf("-> itemsize.%d | %p value.%p %d key.%p %d (%s %s)\n",ptr->itemsize,item,item->value,item->valuesize,kv777_itemkey(item),item->keysize,item->value,kv777_itemkey(item));
 }

int32_t KV777_idle(char *protocol)
{
    double gap; struct kv777_item *ptr; struct kv777 *kv; int32_t i,n = 0;
    gap = (milliseconds() - KV777.Last_kvupdate);
    if ( KV777.Num_kvs > 0 && (gap < 100 || gap > 1000) )
    {
        for (i=0; i<KV777.Num_kvs; i++)
        {
            if ( strcmp(KV777.protocol,protocol) == 0 )
            {
                kv = KV777.KVS[i];
                kv777_lock(kv);
                if ( (ptr= kv->hddlist) != 0 )
                    DL_DELETE(kv->hddlist,ptr);
                kv777_unlock(kv);
                if ( ptr != 0 )
                {
                    kv777_update(kv,ptr);
                    n++;
                    KV777.Last_kvupdate = milliseconds();
                }
            }
        }
    }
    return(n);
}

int32_t kv777_idle()
{
    return(KV777_idle("*"));
}

void kv777_flush(char *protocol)
{
    int32_t i; struct kv777 *kv;
    if ( KV777.Num_kvs > 0 )
    {
        while ( KV777_idle(protocol) > 0 )
            ;
        for (i=0; i<KV777.Num_kvs; i++)
        {
            kv = KV777.KVS[i];
            if ( kv->fp != 0 )
                fflush(kv->fp);
#ifndef _WIN32
            if ( kv->fileptr != 0 && kv->mapsize != 0 )
                msync(kv->fileptr,kv->mapsize,MS_SYNC);
#endif
        }
    }
}

void kv777_counters(struct kv777 *kv,int32_t polarity,uint32_t keysize,uint32_t valuesize,struct kv777_item *ptr)
{
    kv->totalvalues += polarity * kv777_valuesize(valuesize), kv->netvalues += polarity;
    kv->totalkeys += polarity *  keysize, kv->netkeys += polarity;
    if ( ptr != 0 && kv->flags.hdd != 0 && kv->flags.rw != 0 )
    {
        if ( kv->flags.writebehind != 0 )
            DL_APPEND(kv->hddlist,ptr);
        else if ( kv->fp != 0 )
        {
            kv777_update(kv,ptr);
            fflush(kv->fp);
            //printf("flushed.(%s).(%s)\n",kv->path,kv->name);
        }
    }
}

void kv777_clear(struct kv777 *kv)
{
    kv->totalkeys = kv->totalvalues = kv->netkeys = kv->netvalues = 0;
    if ( kv->fp != 0 )
        fclose(kv->fp), kv->fp = 0;
    if ( kv->fileptr != 0 )
        release_map_file(kv->fileptr,kv->mapsize), kv->fileptr = 0, kv->mapsize = 0;
    delete_file(kv->fname,0);
}

void kv777_add(struct kv777 *kv,struct kv777_item *ptr,int32_t updatehdd)
{
    if ( kv->dispflag != 0 )
        printf("add.(%s).(%s) kv->numkeys.%d\n",kv->path,kv->name,kv->numkeys);
    kv->numkeys++;
    HASH_ADD_KEYPTR(hh,kv->table,kv777_itemkey(ptr->item),ptr->item->keysize,ptr);
    kv777_counters(kv,1,ptr->item->keysize,ptr->item->valuesize,updatehdd != 0 ? ptr : 0);
}

int32_t kv777_delete(struct kv777 *kv,void *key,int32_t keysize)
{
    void *itemkey; int32_t retval = -1; struct kv777_item *ptr = 0;
    if ( kv == 0 )
        return(-1);
    kv777_lock(kv);
    HASH_FIND(hh,kv->table,key,keysize,ptr);
    if ( ptr != 0 )
    {
        itemkey = kv777_itemkey(ptr->item);
        fprintf(stderr,"%d kv777_delete.%p val.%s %s vs %s val.%s\n",kv->netkeys,ptr,ptr->item->value,itemkey,key,ptr->item->value);
        HASH_DELETE(hh,kv->table,ptr);
        kv777_setdeleted(ptr->item);
        kv777_counters(kv,-1,ptr->item->keysize,ptr->item->valuesize,ptr);
        retval = 0;
    }
    kv777_lock(kv);
    return(retval);
}

struct kv777_item *kv777_write(struct kv777 *kv,void *key,int32_t keysize,void *value,int32_t valuesize)
{
    struct kv777_item *ptr = 0;
    if ( kv == 0 )
        return(0);
    if ( kv->dispflag != 0 )
        fprintf(stderr,"kv777_write kv.%p table.%p write key.%s size.%d, value.(%llx) size.%d\n",kv,kv->table,key,keysize,*(long long *)value,valuesize);
    kv777_lock(kv);
    HASH_FIND(hh,kv->table,key,keysize,ptr);
    if ( ptr != 0 )
    {
        if ( ptr->item != 0 )
        {
            if ( valuesize == ptr->item->valuesize )
            {
                if ( memcmp(ptr->item->value,value,valuesize) != 0 )
                {
                    memcpy(ptr->item->value,value,valuesize);
                    ptr->item->crc = _crc32(0,(void *)((long)ptr->item + sizeof(ptr->item->crc)),kv777_itemsize(keysize,valuesize) - sizeof(ptr->item->crc));
                    if ( kv->fp != 0 )
                        kv777_update(kv,ptr), fflush(kv->fp);
#ifndef _WIN32
                    else if ( kv->fileptr != 0 && kv->mapsize != 0 )
                            msync(kv->fileptr,kv->mapsize,MS_SYNC);
#endif
                } else if ( Debuglevel > 1 )
                    fprintf(stderr,"%d IDENTICAL.%p val.%s %s vs %s\n",kv->netkeys,ptr,ptr->item->value,kv777_itemkey(ptr->item),key);
                kv777_unlock(kv);
                return(ptr);
            }
            else if ( Debuglevel > 3 || kv->dispflag != 0 )
                printf("kv777_write (%llx) != (%llx)\n",*(long long *)ptr->item->value,*(long long *)value);
            kv777_counters(kv,-1,ptr->item->keysize,ptr->item->valuesize,0);
            kv777_free(kv,ptr,0);
        } else printf("kv777_write: null item?\n");
        if ( kv777_itemsize(keysize,valuesize) > ptr->itemsize )
        {
            printf("DOESNT FIT: itemsize.%d > existing.%d offset.%ld -> -1\n",kv777_itemsize(keysize,valuesize),ptr->itemsize,ptr->offset);
            ptr->offset = -1;
        }
        if ( (ptr->item= kv777_hdditem(&ptr->itemsize,0,0,key,keysize,value,valuesize)) != 0 )
            kv777_counters(kv,1,keysize,valuesize,ptr);
        else if ( Debuglevel > 3 )
            printf("kv777_write: couldnt create item.(%s) %s ind.%d offset.%ld\n",key,value,kv->numkeys,ftell(kv->fp));
    }
    else
    {
        ptr = calloc(1,sizeof(struct kv777_item));
        ptr->ind = kv->numkeys, ptr->offset = -1;
        if ( (ptr->item= kv777_hdditem(&ptr->itemsize,0,0,key,keysize,value,valuesize)) == 0 )
        {
            printf("kv777_write: couldnt create item.(%s) %s ind.%d offset.%ld\n",key,value,kv->numkeys,ftell(kv->fp));
            free(ptr), ptr = 0;
        }
        else  kv777_add(kv,ptr,1);
        if ( Debuglevel > 3 )
            fprintf(stderr,"%d CREATE.%p val.%s %s vs %s val.%s\n",kv->netkeys,ptr,ptr->item->value,kv777_itemkey(ptr->item),key,value);
    }
    kv777_unlock(kv);
    return(ptr);
}

void *kv777_read(struct kv777 *kv,void *key,int32_t keysize,void *value,int32_t *valuesizep,int32_t retitem)
{
    struct kv777_hdditem *item = 0; struct kv777_item *ptr = 0;
    if ( kv == 0 )
        return(0);
    kv777_lock(kv);
    HASH_FIND(hh,kv->table,key,keysize,ptr);
    kv777_unlock(kv);
    if ( ptr != 0 && (item= ptr->item) != 0 && kv777_isdeleted(item) == 0 )
    {
        if ( valuesizep != 0 )
        {
            if ( value != 0 && item->valuesize <= *valuesizep )
                memcpy(value,item->value,item->valuesize);
            *valuesizep = item->valuesize;
        }
        if ( retitem != 0 )
            return(item);
        else return(item->value);
    }
    if ( Debuglevel > 3 )
        printf("kv777_read ptr.%p item.%p key.%s keysize.%d\n",ptr,item,key,keysize);
    if ( valuesizep != 0 )
        *valuesizep = 0;
    return(0);
}

int32_t kv777_addstr(struct kv777 *kv,char *key,char *value)
{
    struct kv777_item *ptr;
    if ( (ptr= kv777_write(kv,key,(int32_t)strlen(key)+1,value,(int32_t)strlen(value)+1)) != 0 )
        return(ptr->ind);
    return(-1);
}

char *kv777_findstr(char *retbuf,int32_t max,struct kv777 *kv,char *key) { return(kv777_read(kv,key,(int32_t)strlen(key)+1,retbuf,&max,0)); }

void *kv777_iterate(struct kv777 *kv,void *args,int32_t allitems,void *(*iterator)(struct kv777 *kv,void *args,void *key,int32_t keysize,void *value,int32_t valuesize))
{
    struct kv777_item *ptr,*tmp; void *retval = 0;
    if ( kv == 0 )
        return(0);
    //kv777_lock(kv);
    HASH_ITER(hh,kv->table,ptr,tmp)
    {
        if ( (allitems != 0 || kv777_isdeleted(ptr->item) == 0) && (retval= (*iterator)(kv,args!=0?args:ptr,kv777_itemkey(ptr->item),ptr->item->keysize,ptr->item->value,ptr->item->valuesize)) != 0 )
        {
            //kv777_unlock(kv);
            return(retval);
        }
    }
    //kv777_unlock(kv);
    return(0);
}

void *kv777_deliterator(struct kv777 *kv,void *_ptr,void *key,int32_t keysize,void *value,int32_t valuesize)
{
    struct kv777_item *ptr = _ptr;
    HASH_DEL(kv->table,ptr);
    kv777_free(kv,ptr,1);
    return(0);
}

int32_t kv777_purge(struct kv777 *kv)
{
    struct kv777_item *ptr = 0;
    kv777_lock(kv);
    while ( kv->hddlist != 0 )
        DL_DELETE(kv->hddlist,ptr);
    kv777_unlock(kv);
    kv777_iterate(kv,0,1,kv777_deliterator);
    kv777_clear(kv);
    return(0);
}

struct kv777 *kv777_init(char *path,char *name,struct kv777_flags *flags) // kv777_init IS NOT THREADSAFE!
{
    long offset = 0; struct kv777_item *checkitem; struct kv777_hdditem *item; uint32_t i,itemsize,allocflag; struct kv777_flags F;
    struct kv777_item *ptr; struct kv777 *kv;
    ensure_directory(path);
    printf("kv777_init.(%s/%s)\n",path,name);
    if ( flags == 0 )
        flags = &F, kv777_defaultflags(flags);
#ifdef _WIN32
    flags->mmap = 0;
#endif
    if ( KV777.Num_kvs > 0 )
    {
        for (i=0; i<KV777.Num_kvs; i++)
            if ( strcmp(KV777.KVS[i]->name,name) == 0 && strcmp(KV777.KVS[i]->path,path) == 0 )
                return(KV777.KVS[i]);
    }
    kv = calloc(1,sizeof(*kv));
    //if ( strcmp(name,"state") == 0 )
    //    kv->dispflag = 1;
    safecopy(kv->path,path,sizeof(kv->path));
    safecopy(kv->name,name,sizeof(kv->name));
    portable_mutex_init(&kv->mutex);
    kv->flags = *flags, kv->flags.mmap *= KV777.mmapflag, kv->flags.threadsafe = 0;
    sprintf(kv->fname,"%s/%s",path,kv->name), os_compatible_path(kv->fname);
    if ( flags->hdd != 0 && (kv->fp= fopen(kv->fname,"rb+")) == 0 )
        kv->fp = fopen(kv->fname,"wb+");
    if ( kv->fp == 0 )
        printf("kv->fp.%p for %s: RAM only mode hdd.%d\n",kv->fp,kv->fname,flags->hdd), flags->hdd = 0;
    if ( kv->fp != 0 )
    {
        if ( kv->flags.mmap != 0 )
        {
            fseek(kv->fp,0,SEEK_END);
            kv->mapsize = ftell(kv->fp);
            kv->fileptr = map_file(kv->fname,&kv->mapsize,1);
        }
        rewind(kv->fp);
        while ( (item= kv777_load(&allocflag,&itemsize,kv)) != 0 )
        {
            if ( kv->dispflag != 0 )
                printf("%d: item.%p itemsize.%d\n",kv->numkeys,item,itemsize);
            if ( kv777_isdeleted(item) != 0 && allocflag != 0 )
                free(item);
            else
            {
                if ( (checkitem= kv777_read(kv,kv777_itemkey(item),item->keysize,0,0,1)) != 0 )
                {
                    printf("init.(%s).(%s): duplicate key.[%llx] found\n",kv->path,kv->name,*(long long *)kv777_itemkey(item));
                    kv777_delete(kv,kv777_itemkey(item),item->keysize);
                }
                ptr = calloc(1,sizeof(*ptr));
                ptr->itemsize = itemsize, ptr->item = item, ptr->offset = offset, ptr->ind = kv->numkeys;
                kv777_add(kv,ptr,0);
                if ( kv->dispflag != 0 )
                    fprintf(stderr,"%p [%llx] add item.%d crc.%u valuesize.%d keysize.%d [%s]\n",item->value,*(long long *)item->value,ptr->ind,item->crc,item->valuesize,item->keysize,kv777_itemkey(item));
            }
            offset = kv->offset; //ftell(kv->fp);
        }
    }
    printf("%s/kv777.%s fpos.%ld -> goodpos.%ld fileptr.%p mapsize.%ld | numkeys.%d netkeys.%d netvalues.%d\n",kv->path,kv->name,kv->fp != 0 ? ftell(kv->fp) : 0,offset,kv->fileptr,(long)kv->mapsize,kv->numkeys,kv->netkeys,kv->netvalues);
    if ( kv->fp != 0 && offset != ftell(kv->fp) )
    {
        printf("strange position?, seek\n");
        fseek(kv->fp,offset,SEEK_SET);
    }
    kv->flags.threadsafe = flags->threadsafe;
    KV777.KVS = realloc(KV777.KVS,sizeof(*KV777.KVS) * (KV777.Num_kvs + 1));
    KV777.KVS[KV777.Num_kvs++] = kv;
    return(kv);
}

void kv777_test(int32_t n)
{
    struct kv777 *kv; void *rval; int32_t errors,iter,i=1,j,len,keylen,valuesize; uint8_t key[32],value[32],result[1024]; double startmilli;
    struct kv777_flags flags;
    kv777_defaultflags(&flags);
    KV777.mmapflag = 1;
    //Debuglevel = 3;
    for (iter=errors=0; iter<3; iter++)
    {
        startmilli = milliseconds();
        if ( (kv= kv777_init("DB","test",&flags)) != 0 )
        {
            srand(777);
            for (i=0; i<n; i++)
            {
                //printf("i.%d of n.%d\n",i,n);
                valuesize = (rand() % (sizeof(value)-1)) + 1;
                keylen = (rand() % (sizeof(key)-8)) + 8;
                memset(key,0,sizeof(key));
                for (j=0; j<keylen; j++)
                    key[j] = safechar64(rand());
                sprintf((void *)key,"%d",i);
                keylen = (int32_t)strlen((void *)key);
                for (j=0; j<valuesize; j++)
                    value[j] = safechar64(rand());
                if ( 1 && iter != 0 && (i % 1000) == 0 )
                    value[0] ^= 0xff;
                kv777_write(kv,key,keylen,value,valuesize);
                if ( (rval= kv777_read(kv,key,keylen,result,&len,0)) != 0 )
                {
                    if ( len != valuesize || memcmp(value,rval,valuesize) != 0 )
                        errors++, printf("len.%d vs valuesize.%d or data mismatch\n",len,valuesize);
                } else errors++, printf("kv777_read error i.%d cant find key added, len.%d, valuesize.%d\n",i,len,valuesize);
            }
        }
        printf("iter.%d fileptr.%p finished kv777_test %d iterations, %.4f millis ave -> %.1f seconds\n",iter,kv->fileptr,i,(milliseconds() - startmilli) / i,.001*(milliseconds() - startmilli));
    }
    kv777_flush("*");
    printf("errors.%d finished kv777_test %d iterations, %.4f millis ave -> %.1f seconds after flush\n",errors,i,(milliseconds() - startmilli) / i,.001*(milliseconds() - startmilli));
}

cJSON *kv_json(struct kv777 *kv)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"name",cJSON_CreateString(kv->name));
    cJSON_AddItemToObject(json,"numkeys",cJSON_CreateNumber(kv->numkeys));
    cJSON_AddItemToObject(json,"netkeys",cJSON_CreateNumber(kv->netkeys));
    cJSON_AddItemToObject(json,"keysizes",cJSON_CreateNumber(kv->totalkeys));
    cJSON_AddItemToObject(json,"netvalues",cJSON_CreateNumber(kv->netvalues));
    cJSON_AddItemToObject(json,"valuesizes",cJSON_CreateNumber(kv->totalvalues));
    return(json);
}

cJSON *dKV_json(struct dKV777 *dKV)
{
    int32_t i; cJSON *array = cJSON_CreateArray();
    cJSON_AddItemToArray(array,kv_json(dKV->nodes));
    if ( dKV->numkvs > 0 )
    {
        for (i=0; i<dKV->numkvs; i++)
            cJSON_AddItemToArray(array,kv_json(dKV->kvs[i]));
    }
    return(array);
}

cJSON *dKV_approvals_json(struct dKV777 *dKV)
{
    char numstr[64]; int32_t i,ind,fifosize; uint64_t txid; cJSON *array = cJSON_CreateArray();
    fifosize = (sizeof(dKV->approvalfifo)/sizeof(*dKV->approvalfifo));
    for (i=0; i<fifosize; i++)
    {
        ind = (dKV->lastfifoi + i) % fifosize;
        if ( (dKV->lastfifoi % fifosize) == (dKV->fifoind % fifosize) || (txid= dKV->approvalfifo[ind]) == 0 )
            break;
        sprintf(numstr,"%llu",(long long)txid), cJSON_AddItemToArray(array,cJSON_CreateString(numstr));
        dKV->approvalfifo[ind] = 0;
    }
    return(array);
}

void dKV777_gotapproval(struct dKV777 *dKV,uint64_t senderbits,uint64_t txid)
{
    uint32_t count,paircount; int32_t size; uint64_t key[2];
    key[0] = txid, key[1] = senderbits;
    if ( dKV != 0 )
    {
        size = sizeof(paircount);
        if ( kv777_read(dKV->approvals,(void *)key,sizeof(key),(void *)&paircount,&size,0) == 0 )
        {
            size = sizeof(count);
            if ( kv777_read(dKV->approvals,(void *)&txid,sizeof(txid),&count,&size,0) == 0 || size != sizeof(count) )
            {
                count = 1;
                if ( kv777_write(dKV->approvals,(void *)&txid,sizeof(txid),&count,sizeof(count)) == 0 )
                    printf("dKV777_gotapproval error writing approval txid.%llu count.%u\n",(long long)txid,count);
                else printf("dKV777_gotapproval created txid.%llu\n",(long long)txid);
            }
            else
            {
                count++;
                if ( kv777_write(dKV->approvals,(void *)&txid,sizeof(txid),&count,sizeof(count)) == 0 )
                    printf("dKV777_gotapproval error writing approval txid.%llu update count.%u\n",(long long)txid,count);
                else printf("dKV777_gotapproval updated txid.%llu count.%u\n",(long long)txid,count);
            }
            size = sizeof(count);
            if ( kv777_read(dKV->approvals,(void *)&senderbits,sizeof(senderbits),&count,&size,0) == 0 || size != sizeof(count) )
            {
                count = 1;
                if ( kv777_write(dKV->approvals,(void *)&senderbits,sizeof(senderbits),&count,sizeof(count)) == 0 )
                    printf("dKV777_gotapproval error writing approval senderbits.%llu count.%u\n",(long long)senderbits,count);
                else printf("dKV777_gotapproval created senderbits.%llu\n",(long long)senderbits);
            }
            else
            {
                count++;
                if ( kv777_write(dKV->approvals,(void *)&senderbits,sizeof(senderbits),&count,sizeof(count)) == 0 )
                    printf("dKV777_gotapproval error writing approval senderbits.%llu update count.%u\n",(long long)senderbits,count);
                else printf("dKV777_gotapproval updated senderbits.%llu count.%u\n",(long long)senderbits,count);
            }
            paircount++;
        } else paircount = 1;
        if ( kv777_write(dKV->approvals,(void *)key,sizeof(key),&paircount,sizeof(paircount)) == 0 )
            printf("dKV777_gotapproval error writing approval txid.%llu for (NXT.%llu) paircount.%d\n",(long long)txid,(long long)senderbits,paircount);
    }
}

struct dKV777 *dKV777_find(char *name,char *protocol)
{
    int32_t i;
    if ( name != 0 && KV777.Num_dKVs > 0 )
    {
        for (i=0; i<KV777.Num_dKVs; i++)
            if ( strcmp(KV777.dKVs[i]->name,name) == 0 && (protocol == 0 || strcmp(protocol,KV777.dKVs[i]->protocol) == 0) )
                return(KV777.dKVs[i]);
    }
    return(0);
}

struct kv777 *dKV777_findkv(struct dKV777 *dKV,struct kv777 *kv)
{
    int32_t i;
    if ( dKV->numkvs > 0 )
    {
        for (i=0; i<dKV->numkvs; i++)
            if ( dKV->kvs[i] == kv )
                return(kv);
    }
    return(0);
}

bits256 dKV777_approval(struct dKV777 *dKV,uint64_t nxt64bits,void *ptr,int32_t len)
{
    char hexstr[512]; bits256 hash;
    calc_sha256(hexstr,(void *)&hash.bytes,ptr,len);
    if ( dKV != 0 && kv777_read(dKV->approvals,(void *)&hash.bytes,sizeof(hash.bytes),0,0,0) == 0 )
    {
        if ( kv777_write(dKV->approvals,(void *)&hash.bytes,sizeof(hash.bytes),ptr,len) == 0 )
            printf("dKV777_approval error writing approval for (%s)\n",ptr);
        else dKV->approvalfifo[dKV->fifoind++ % (sizeof(dKV->approvalfifo)/sizeof(*dKV->approvalfifo))] = hash.txid;
        printf("NEW.");
    }
    printf("[%s approve %s].%llu ",dKV->name,hexstr,(long long)hash.txid);
    return(hash);
}

void *dKV777_bigkey(int32_t *bigkeysizep,uint64_t ownerbits,void *key,int32_t keysize)
{
    uint8_t *bigkey = malloc(keysize + sizeof(ownerbits));
    memcpy(bigkey,&ownerbits,sizeof(ownerbits));
    memcpy(&bigkey[sizeof(ownerbits)],key,keysize);
    *bigkeysizep = (int32_t)(keysize + sizeof(ownerbits));
    return(bigkey);
}

void *dKV777_bigvalue(int32_t *bigvaluesizep,uint32_t rawind,void *value,int32_t valuesize)
{
    uint8_t *bigvalue = malloc(valuesize + sizeof(rawind));
    memcpy(bigvalue,&rawind,sizeof(rawind));
    memcpy(&bigvalue[sizeof(rawind)],value,valuesize);
    *bigvaluesizep = (int32_t)(valuesize + sizeof(rawind));
    return(bigvalue);
}

struct key_hash { uint64_t key[2]; bits256 hash;  };
uint32_t dKV777_write(struct dKV777 *dKV,struct kv777 *kv,uint64_t ownerbits,void *key,int32_t keysize,void *value,int32_t valuesize)
{
    struct kv777_item *valueptr; struct key_hash KH; int32_t bigkeysize,bigvaluesize,savedsize; uint32_t rawind = 0; uint8_t *bigkey,*bigvalue;
    memset(&KH,0,sizeof(KH));
    if ( dKV777_findkv(dKV,kv) == kv )
    {
        bigkey = dKV777_bigkey(&bigkeysize,ownerbits,key,keysize);
        if ( (valueptr= kv777_read(kv,bigkey,bigkeysize,0,&savedsize,1)) != 0 )
        {
            memcpy(&rawind,valueptr->item->value,sizeof(valueptr->ind));
            KH.key[0] = ownerbits, KH.key[1] = valueptr->ind;
            kv777_write(kv,KH.key,sizeof(KH.key),key,keysize);
            bigvalue = dKV777_bigvalue(&bigvaluesize,valueptr->ind,value,valuesize);
        }
        else
        {
            if ( kv->dontrelay == 0 && dKV->pubsock >= 0 )
                nn_send(dKV->pubsock,value,valuesize,0);
            KH.key[0] = ownerbits, KH.key[1] = rawind = kv->numkeys;
            kv777_write(kv,KH.key,sizeof(KH.key),key,keysize);
            bigvalue = dKV777_bigvalue(&bigvaluesize,rawind,value,valuesize);
            kv777_write(kv,bigkey,bigkeysize,bigvalue,bigvaluesize);
        }
        KH.hash = dKV777_approval(dKV,ownerbits,bigvalue,bigvaluesize);
        free(bigkey), free(bigvalue);
    }
    return(rawind);
}

void *dKV777_read(struct dKV777 *dKV,struct kv777 *kv,uint64_t ownerbits,void *key,int32_t keysize,void *value,int32_t *valuesizep)
{
    int32_t bigkeysize; uint8_t *bigkey; void *valueptr = 0;
    if ( dKV777_findkv(dKV,kv) == kv )
    {
        bigkey = dKV777_bigkey(&bigkeysize,ownerbits,key,keysize);
        valueptr = kv777_read(kv,bigkey,bigkeysize,value,valuesizep,0);
        free(bigkey);
    }
    return(valueptr);
}

void dKV777_deleteind(struct dKV777 *dKV,struct kv777 *kv,uint64_t ownerbits,uint32_t rawind)
{
    struct key_hash KH; uint8_t *key,*bigkey,*bigvalue; int32_t keysize,bigvaluesize,bigkeysize;
    KH.key[0] = ownerbits, KH.key[1] = rawind;
    if ( dKV777_findkv(dKV,kv) == kv )
    {
        if ( (key= kv777_read(kv,KH.key,sizeof(KH.key),0,&keysize,1)) != 0 )
        {
            bigkey = dKV777_bigkey(&bigkeysize,ownerbits,key,keysize);
            bigvalue = kv777_read(kv,bigkey,bigkeysize,0,&bigvaluesize,1);
            KH.hash = dKV777_approval(dKV,ownerbits,&bigvalue[sizeof(rawind)],(int32_t)(bigvaluesize-sizeof(rawind)));
            kv777_delete(kv,KH.key,sizeof(KH.key));
            kv777_delete(kv,bigkey,bigkeysize);
            kv777_delete(dKV->approvals,(void *)&KH.hash,sizeof(KH.hash));
            free(bigkey);
        }
    }
}

void dKV777_delete(struct dKV777 *dKV,struct kv777 *kv,uint64_t ownerbits,void *key,int32_t keysize)
{
    struct key_hash KH; uint8_t *bigkey; struct kv777_item *valueptr; int32_t bigvaluesize,bigkeysize; uint32_t rawind;
    if ( dKV777_findkv(dKV,kv) == kv )
    {
        bigkey = dKV777_bigkey(&bigkeysize,ownerbits,key,keysize);
        if ( (valueptr= kv777_read(kv,bigkey,bigkeysize,0,&bigvaluesize,1)) != 0 )
        {
            memcpy(&rawind,valueptr->item->value,sizeof(valueptr->ind));
            KH.key[0] = ownerbits, KH.key[1] = rawind;
            KH.hash = dKV777_approval(dKV,ownerbits,&valueptr->item->value[sizeof(rawind)],(int32_t)(bigvaluesize-sizeof(rawind)));
            kv777_delete(kv,KH.key,sizeof(KH.key));
            kv777_delete(kv,bigkey,bigkeysize);
            kv777_delete(dKV->approvals,(void *)&KH.hash,sizeof(KH.hash));
        }
        free(bigkey);
    }
}

/*int32_t dKV777_poll()
{
    int32_t i,j,recvlen,n = 0; struct dKV777 *dKV; char *msg,*retstr,*str;
    if ( KV777.Num_dKVs > 0 )
    {
        //portable_mutex_lock(&dKV->mutex);
        for (i=0; i<KV777.Num_dKVs; i++)
        {
            if ( (dKV= KV777.dKVs[i]) != 0 )
            {
                for (j=0; j<dKV->numkvs; j++)
                {
                    if ( dKV->subsock != RELAYS.pubrelays && dKV->subsock >= 0 && (recvlen= nn_recv(dKV->subsock,&msg,NN_MSG,0)) > 0 )
                    {
                        printf("dKV777_poll.(%s).%d RECV.(%s).%d\n",dKV->name,j,msg,recvlen);
                        retstr = nn_busdata_processor((uint8_t *)msg,recvlen);
                        if ( retstr != 0 )
                        {
                            printf("retstr.(%s)\n",retstr);
                            free(retstr);
                        }
                        n++;
                        nn_freemsg(msg);
                    }
                }
            }
        }
        if ( milliseconds() > (dKV->lastping + dKV->pinggap) )
        {
            if ( (str= dKV777_ping(dKV)) != 0 )
                free(str);
            dKV->lastping = milliseconds();
        }
        //portable_mutex_unlock(&dKV->mutex);
    }
    return(n);
}*/

int32_t dKV777_connect(struct dKV777 *dKV,struct endpoint *ep)
{
    int32_t j; char endpoint[512];
    if ( dKV == 0 || dKV->subsock < 0 )
        return(-1);
    for (j=0; j<dKV->num; j++)
        if ( memcmp(ep,&dKV->connections[j],sizeof(*ep)) == 0 )
            return(0);
    if ( j == dKV->num )
    {
        expand_epbits(endpoint,*ep);
        printf("%p connect (%s) to (%s)\n",dKV,dKV->name,endpoint);
        if ( nn_connect(dKV->subsock,endpoint) < 0 )
            printf("KV777_init warning: error connecting to (%s) %s\n",endpoint,nn_errstr());
        else
        {
            if ( dKV->num >= dKV->max )
                printf("KV777_init warning: num.%d > max.%d (%s)\n",dKV->num,dKV->max,endpoint);
            dKV->connections[dKV->num++ % dKV->max] = *ep;
            return(1);
        }
    }
    return(-1);
}

int32_t dKV777_addnode(struct dKV777 *dKV,struct endpoint *ep)
{
    struct dKV_node node; uint32_t ind = dKV->nodes->numkeys;
    printf("addnode %x\n",ep->ipbits);
    memset(&node,0,sizeof(node));
    node.endpoint = *ep;
    if ( kv777_write(dKV->nodes,ep,sizeof(*ep),&node,sizeof(node)) == 0 || kv777_write(dKV->nodes,&ind,sizeof(ind),ep,sizeof(*ep)) == 0 )
        return(-1);
    return(0);
}

char *dKV777_processping(cJSON *json,char *origjsonstr,char *sender,char *tokenstr)
{
    cJSON *array; uint64_t nxt64bits,senderbits; int32_t i,j,n,size; uint16_t port; struct endpoint endpoint,*ep; struct dKV777 *dKV,*relays;
    char ipaddr[64],buf[512],*endpointstr,*nxtaddr,*peerstr = 0;
    if ( KV777.relays_handle == 0 || (relays= *KV777.relays_handle) == 0 )
        return(clonestr("{\"error\":\"no relays KV777\"}"));
    if ( (dKV= dKV777_find(jstr(json,"dKVname"),jstr(json,"protocol"))) == 0 )
        return(clonestr("{\"error\":\"unsupported protocol or cant find dKV\"}"));
    senderbits = conv_acctstr(sender);
    if ( dKV != 0 && senderbits != 0 && (array= cJSON_GetObjectItem(json,"approvals")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
    {
        for (i=0; i<n; i++)
            dKV777_gotapproval(dKV,senderbits,get_API_nxt64bits(cJSON_GetArrayItem(array,i)));
    }
    if ( (array= cJSON_GetObjectItem(json,"peers")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
    {
        peerstr = cJSON_Print(array), _stripwhite(peerstr,' ');
        for (i=0; i<n; i++)
        {
            if ( (endpointstr= cJSON_str(cJSON_GetArrayItem(array,i))) != 0 )
            {
                for (j=0; j<relays->nodes->numkeys; j++)
                {
                    size = sizeof(endpoint);
                    if ( (ep= kv777_read(relays->nodes,&j,sizeof(j),&endpoint,&size,0)) != 0 && size == sizeof(endpoint) )
                    {
                        expand_epbits(buf,*ep);
                        if ( strcmp(buf,endpointstr) == 0 )
                            break;
                    }
                }
                if ( j == relays->nodes->numkeys && strcmp(endpointstr,KV777.relayendpoint) != 0 )
                {
                    port = parse_ipaddr(ipaddr,endpointstr+6);
                    printf("ipaddr.(%s):%d\n",ipaddr,port);
                    endpoint = calc_epbits(KV777.transport,(uint32_t)calc_ipbits(ipaddr),port,NN_PUB);
                    dKV777_connect(relays,&endpoint), dKV777_addnode(relays,&endpoint);
                }
            }
        }
    }
    if ( dKV != 0 && senderbits != 0 && peerstr != 0 && (nxtaddr= cJSON_str(cJSON_GetObjectItem(json,"NXT"))) != 0 && (nxt64bits= conv_acctstr(nxtaddr)) == senderbits )
        dKV777_approval(dKV,nxt64bits,peerstr,(int32_t)strlen(peerstr)+1);
    printf("KV777 GOT.(%s) from.(%s) [%s]\n",origjsonstr,sender,tokenstr);
    if ( peerstr != 0 )
        free(peerstr);
    return(clonestr("{\"result\":\"success\"}"));
}

char *dKV777_ping(struct dKV777 *dKV)
{
    uint32_t i,nonce; int32_t size; struct endpoint endpoint,*ep; char tokenstr[512],buf[512],*retstr,*jsonstr; cJSON *array,*json;
    if ( dKV->endpointstr[0] == 0 )
        return(clonestr("{\"error\":\"no endpoint\"}"));
    json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"agent",cJSON_CreateString(dKV->name));
    cJSON_AddItemToObject(json,"dKVname",cJSON_CreateString(dKV->name));
    if ( dKV->protocol != 0 && dKV->protocol[0] != 0 )
        cJSON_AddItemToObject(json,"protocol",cJSON_CreateString(dKV->protocol));
    cJSON_AddItemToObject(json,"method",cJSON_CreateString("ping"));
    cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(dKV->NXTADDR));
    cJSON_AddItemToObject(json,"rand",cJSON_CreateNumber(rand()));
    cJSON_AddItemToObject(json,"unixtime",cJSON_CreateNumber(time(NULL)));
    cJSON_AddItemToObject(json,"myendpoint",cJSON_CreateString(dKV->endpointstr));
    array = cJSON_CreateArray();
    cJSON_AddItemToArray(array,cJSON_CreateString(dKV->endpointstr));
    for (i=0; i<dKV->nodes->numkeys; i++)
    {
        size = sizeof(endpoint);
        if ( (ep= kv777_read(dKV->nodes,&i,sizeof(i),&endpoint,&size,0)) != 0 && size == sizeof(endpoint) )
        {
            expand_epbits(buf,*ep);
            cJSON_AddItemToArray(array,cJSON_CreateString(buf));
        }
    }
    cJSON_AddItemToObject(json,"peers",array);
    cJSON_AddItemToObject(json,"approvals",dKV_approvals_json(dKV));
    cJSON_AddItemToObject(json,"kvs",dKV_json(dKV));
    jsonstr = cJSON_Print(json), _stripwhite(jsonstr,' '), free_json(json);
    if ( (retstr= busdata_sync(&nonce,jsonstr,"allrelays",0)) != 0 )
    {
        printf("KV777_ping.(%s)\n",jsonstr);
        free(retstr);
    }
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        issue_generateToken(tokenstr,jsonstr,(void *)dKV->NXTACCTSECRET);
        tokenstr[NXT_TOKEN_LEN] = 0;
        if ( (retstr= dKV777_processping(json,jsonstr,dKV->NXTADDR,tokenstr)) != 0 )
            free(retstr);
        free_json(json);
    }
    return(jsonstr);
}

struct dKV777 *dKV777_init(char *name,char *protocol,struct kv777 **kvs,int32_t numkvs,uint32_t flags,int32_t pubsock,int32_t subsock,struct endpoint *connections,int32_t num,int32_t max,uint16_t port,double pinggap)
{
    struct dKV777 *dKV = calloc(1,sizeof(*dKV));
    struct endpoint endpoint,*ep; char buf[512]; int32_t i,size,sendtimeout=10,recvtimeout=1;
    if ( protocol == 0 || protocol[0] == 0 )
        protocol = "*";
    if ( KV777.protocol[0] == 0 )
        strcpy(KV777.protocol,protocol);
    portable_mutex_init(&dKV->mutex);
    dKV->port = port; dKV->connections = connections, dKV->num = num, dKV->max = max, dKV->flags = flags, dKV->kvs = kvs, dKV->numkvs = numkvs;
    safecopy(dKV->name,name,sizeof(dKV->name));
    safecopy(dKV->NXTADDR,KV777.NXTADDR,sizeof(dKV->NXTADDR));
    memcpy(dKV->NXTACCTSECRET,(void *)KV777.NXTACCTSECRET,KV777.secretlen);
    safecopy(dKV->protocol,protocol,sizeof(dKV->protocol));
    if ( (dKV->pinggap= pinggap) == 0. )
        dKV->pinggap = 600000;
    buf[0] = 0;
    if ( (dKV->pubsock= pubsock) < 0 && (dKV->pubsock= nn_createsocket(buf,1,"NN_PUB",NN_SUB,port,sendtimeout,recvtimeout)) < 0 )
    {
        printf("KV777_init pubsocket failure %d %s\n",dKV->pubsock,buf);
        free(dKV);
        return(0);
    }
    buf[0] = 0;
    if ( (dKV->subsock= subsock) < 0 && (dKV->subsock= nn_createsocket(buf,0,"NN_SUB",NN_SUB,0,sendtimeout,recvtimeout)) >= 0 )
        nn_setsockopt(dKV->subsock,NN_SUB,NN_SUB_SUBSCRIBE,"",0);
    if ( dKV->subsock < 0 )
    {
        printf("KV777_init subsocket failure %d\n",dKV->subsock);
        nn_shutdown(dKV->pubsock,0);
        free(dKV);
        return(0);
    }
    sprintf(buf,"%s.nodes",name), dKV->nodes = kv777_init(KV777.PATH,buf,0);
    sprintf(buf,"%s.approvals",name), dKV->approvals = kv777_init(KV777.PATH,buf,0);
    for (i=0; i<dKV->nodes->numkeys; i++) // connect all nodes in DB that are not already connected
    {
        printf("dKV.%d check\n",i);
        size = sizeof(endpoint);
        if ( (ep= kv777_read(dKV->nodes,&i,sizeof(i),&endpoint,&size,0)) != 0 && size == sizeof(endpoint) )
            dKV777_connect(dKV,ep);
    }
    for (i=0; i<num; i++) // add all nodes not in DB to DB
    {
        printf("CONN.%d check\n",i);
        expand_epbits(buf,connections[i]);
        if ( kv777_read(dKV->nodes,&connections[i],sizeof(connections[i]),0,0,0) == 0 )
        {
            if ( dKV777_addnode(dKV,&connections[i]) != 0 )
                printf("KV777_init warning: error adding node to (%s)\n",buf);
        }
    }
    KV777.dKVs = realloc(KV777.dKVs,sizeof(*KV777.dKVs) * (KV777.Num_dKVs + 1));
    KV777.dKVs[KV777.Num_dKVs] = dKV, KV777.Num_dKVs++;
    return(dKV);
}

void set_KV777_globals(struct dKV777 **relays_handle,char *transport,void *NXTACCTSECRET,int32_t secretlen,char *SERVICENXT,char *relayendpoint)
{
    int32_t i;
    memcpy(KV777.NXTACCTSECRET,NXTACCTSECRET,secretlen), KV777.secretlen = secretlen;
    KV777.nxt64bits = conv_NXTpassword(KV777.mysecret,KV777.mypubkey,NXTACCTSECRET,secretlen), expand_nxt64bits(KV777.NXTADDR,KV777.nxt64bits);
    if ( SERVICENXT != 0 && SERVICENXT[0] != 0 )
        KV777.service64bits = calc_nxt64bits(KV777.SERVICENXT);
    if ( transport != 0 )
    {
        for (i=0; i<transport[i]!=0; i++)
            KV777.transport[i] = transport[i];
    }
    if ( relayendpoint != 0 )
    {
        for (i=0; i<relayendpoint[i]!=0; i++)
            KV777.relayendpoint[i] = relayendpoint[i];
    }
    KV777.relays_handle = relays_handle;
}
#endif
#endif
