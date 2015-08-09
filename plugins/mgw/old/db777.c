//
//  db777.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//
#ifdef INSIDE_MGW

#ifdef DEFINES_ONLY
#ifndef crypto777_db777_h
#define crypto777_db777_h
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "sophia.h"
#include "../includes/cJSON.h"
#include "../common/system777.c"
#include "../coins/coins777.c"
#include "../KV/kv777.c"

#define DB777_RAM 1
#define DB777_HDD 2
#define DB777_NANO 4
#define DB777_FLUSH 8
#define DB777_VOLATILE 0x10
#define DB777_KEY32 0x20
#define ENV777_BACKUP 0x40
#define DB777_MULTITHREAD 0x80
#define DB777_RAMDISK 0x100
#define DB777_MATRIXROW 10000

#define SOPHIA_USERDIR "/user"
void *db777_get(void *dest,int32_t *lenp,void *transactions,struct db777 *DB,void *key,int32_t keylen);
int32_t db777_set(int32_t flags,void *transactions,struct db777 *DB,void *key,int32_t keylen,void *value,int32_t valuelen);
char *db777_errstr(void *ctl);

uint64_t db777_ctlinfo64(void *ctl,char *field);
int32_t db777_add(int32_t forceflag,void *transactions,struct db777 *DB,void *key,int32_t keylen,void *value,int32_t len);
int32_t db777_delete(int32_t flags,void *transactions,struct db777 *DB,void *key,int32_t keylen);
int32_t db777_sync(void *transactions,struct env777 *DBs,int32_t fullbackup);
void db777_free(struct db777 *DB);
int32_t db777_matrixalloc(struct db777 *DB);

int32_t db777_addstr(struct db777 *DB,char *key,char *value);
int32_t db777_findstr(char *retbuf,int32_t max,struct db777 *DB,char *key);
int32_t db777_link(void *transactions,struct db777 *DB,struct db777 *revDB,uint32_t ind,void *value,int32_t valuelen);

int32_t db777_close(struct db777 *DB);
void **db777_copy_all(int32_t *nump,struct db777 *DB,char *field,int32_t size);
struct db777 *db777_create(char *specialpath,char *subdir,char *name,char *compression,int32_t restoreflag);

int32_t env777_start(int32_t dispflag,struct env777 *DBs,uint32_t start_RTblocknum);
char **db777_index(int32_t *nump,struct db777 *DB,int32_t max);
int32_t db777_dump(struct db777 *DB,int32_t binarykey,int32_t binaryvalue);
void *db777_read(void *dest,int32_t *lenp,void *transactions,struct db777 *DB,void *key,int32_t keylen,int32_t fillcache);
void *db777_matrixptr(int32_t *matrixindp,void *transactions,struct db777 *DB,void *key,int32_t keylen);
int32_t db777_linkDB(struct db777 *DB,struct db777 *revDB,uint32_t maxind);
void db777_path(char *path,char *coinstr,char *subdir,int32_t useramdisk);
int32_t db777_write(void *transactions,struct db777 *DB,void *key,int32_t keylen,void *value,int32_t valuelen);

extern struct db777_info SOPHIA;
extern struct db777 *DB_msigs,*DB_NXTaccts,*DB_NXTtxids,*DB_MGW,*DB_redeems,*DB_NXTtrades;

#endif
#else
#ifndef crypto777_db777_c
#define crypto777_db777_c

#ifndef crypto777_db777_h
#define DEFINES_ONLY
#include "db777.c"
#undef DEFINES_ONLY
#endif

uint32_t Duplicate,Mismatch,Added,Linked,Numgets;

void db777_lock(struct db777 *DB)
{
    if ( (DB->flags & DB777_MULTITHREAD) != 0 )
        portable_mutex_lock(&DB->mutex);
}

void db777_unlock(struct db777 *DB)
{
    if ( (DB->flags & DB777_MULTITHREAD) != 0 )
        portable_mutex_unlock(&DB->mutex);
}

void db777_free_entry(struct db777_entry *entry)
{
    void *obj;
    if ( entry->valuesize == 0 && entry->linked == 0 )
    {
        memcpy(&obj,entry->value,sizeof(obj));
        free(obj);
    }
    free(entry);
}

void db777_free(struct db777 *DB)
{
    int32_t i; struct db777_entry *entry,*tmp;
    if ( DB->table != 0 )
    {
        db777_lock(DB);
        HASH_ITER(hh,DB->table,entry,tmp)
        {
            HASH_DEL(DB->table,entry);
            db777_free_entry(entry);
        }
        db777_unlock(DB);
    }
    db777_lock(DB);
    if ( DB->dirty != 0 )
        free(DB->dirty), DB->dirty = 0;
    if ( DB->matrix != 0 )
    {
        for (i=0; i<DB->matrixentries; i++)
            if ( DB->matrix[i] != 0 )
                free(DB->matrix[i]);
        free(DB->matrix), DB->matrix = 0;
    }
    db777_unlock(DB);
}

void *db777_matrixptr(int32_t *matrixindp,void *transactions,struct db777 *DB,void *key,int32_t keylen)
{
    uint32_t rawind;
    *matrixindp = 0;
    if ( keylen == sizeof(uint32_t) )
    {
        rawind = *(uint32_t *)key;
        *matrixindp = (rawind / DB777_MATRIXROW);
        db777_lock(DB);
        {
            if ( *matrixindp >= DB->matrixentries )
            {
                DB->matrix = realloc(DB->matrix,sizeof(*DB->matrix) * (DB->matrixentries + DB777_MATRIXROW));
                memset(&DB->matrix[DB->matrixentries],0,sizeof(*DB->matrix) * DB777_MATRIXROW);
                DB->matrixentries += DB777_MATRIXROW;
            }
            if ( DB->matrix[*matrixindp] == 0 )
            {
                printf("alloc.%d -> %s[%d]\n",DB777_MATRIXROW*DB->valuesize,DB->name,*matrixindp);
                DB->matrix[*matrixindp] = calloc(DB777_MATRIXROW,DB->valuesize);
            }
        }
        db777_unlock(DB);
        return((void *)((long)DB->matrix[*matrixindp] + ((rawind % DB777_MATRIXROW) * DB->valuesize)));
    } else printf("invalid keylen.%d for %s\n",keylen,DB->name);
    return(0);
}

int32_t db777_matrixalloc(struct db777 *DB)
{
    return(0);
    return((DB->flags & (DB777_RAM | DB777_KEY32)) == (DB777_RAM | DB777_KEY32) && DB->valuesize != 0);
}

int32_t db777_link(void *transactions,struct db777 *DB,struct db777 *revDB,uint32_t ind,void *value,int32_t valuelen)
{
    struct db777_entry *entry; void *revptr; int32_t matrixi;
    return(0);
    //if ( strcmp(DB->name,"addrinfos") != 0 )
    //    transactions = 0;
    db777_lock(DB);
    HASH_FIND(hh,DB->table,value,valuelen,entry);
    db777_unlock(DB);
    if ( entry != 0 )
    {
        if ( entry->valuesize == sizeof(uint32_t)*2 )
        {
            if ( *(uint32_t *)entry->value == ind && valuelen == revDB->valuesize && memcmp(entry->hh.key,value,valuelen) == 0 )
            {
                if ( (revptr= db777_matrixptr(&matrixi,transactions,revDB,&ind,sizeof(ind))) != 0 && memcmp(revptr,value,valuelen) == 0 )
                {
                    free(entry->hh.key);
                    entry->hh.key = revptr;
                    entry->linked = 1;
                    Linked++;
                    return(0);
                } else printf("miscompared %s vs %s\n",DB->name,revDB->name);
            } else printf("miscompared %s ind.%d vs %d\n",DB->name,ind,*(uint32_t *)entry->value);
        } else printf("unexpected nonzero valuesize.%d for %s\n",entry->valuesize,DB->name);
    } else printf("couldnt find entry for %s ind.%d: value.%x\n",DB->name,ind,*(int *)value);
    return(-1);
}

int32_t db777_linkDB(struct db777 *DB,struct db777 *revDB,uint32_t maxind)
{
    uint32_t ind; void *value; int32_t matrixi;
    printf("linkDB maxind.%d\n",maxind);
    for (ind=1; ind<=maxind; ind++)
    {
        if ( (value= db777_matrixptr(&matrixi,0,revDB,&ind,sizeof(ind))) != 0 )
        {
            if ( db777_link(0,DB,revDB,ind,value,revDB->valuesize) != 0 )
            {
                printf("ind.%d linkerror\n",ind);
                break;
            }
        }
        else
        {
            printf("couldnt find ind.%d, value.%p valuesize.%d matrixi.%d\n",ind,value,revDB->valuesize,matrixi);
            break;
        }
    }
    return(ind);
}

int32_t db777_write(void *transactions,struct db777 *DB,void *key,int32_t keylen,void *value,int32_t valuelen)
{
    int32_t retval = -1; void *obj,*db;
    db = DB->asyncdb != 0 ? DB->asyncdb : DB->db;
    if ( (obj= sp_object(db)) == 0 )
        retval = -3;
    if ( sp_set(obj,"key",key,keylen) != 0 || sp_set(obj,"value",value,valuelen) != 0 )
    {
        sp_destroy(obj);
        printf("error setting key/value %s[%d]\n",DB->name,*(int *)key);
        retval = -4;
    }
    else
    {
        retval = sp_set((transactions != 0 ? transactions : db),obj);
        //if ( strcmp(DB->name,"ledger") == 0 )
        //    printf("retval.%d %s key.%u valuelen.%d\n",retval,DB->name,*(int *)key,valuelen);
    }
    return(retval);
}

void *db777_read(void *dest,int32_t *lenp,void *transactions,struct db777 *DB,void *key,int32_t keylen,int32_t fillcache)
{
    void *obj,*value,*result = 0; int32_t flag,max = *lenp;
    flag = 0;
    if ( (obj= sp_object(DB->db)) != 0 )
    {
        Numgets++;
        if ( sp_set(obj,"key",key,keylen) == 0 && (result= sp_get(transactions != 0 ? transactions : DB->db,obj)) != 0 )
        {
            value = sp_get(result,"value",lenp);
            if ( *lenp <= max )
            {
                memcpy(dest,value,*lenp);
                flag = 1;
                if ( fillcache != 0 )//(DB->flags & DB777_RAM) != 0 )
                    db777_set(DB777_RAM,transactions,DB,key,keylen,value,*lenp);
            } else dest = 0;
        }
        if ( result != 0 )
            sp_destroy(result);
    }
    return(flag == 0 ? 0 : dest);
}

void *db777_get(void *dest,int32_t *lenp,void *transactions,struct db777 *DB,void *key,int32_t keylen)
{
    int32_t i,c,max,matrixi; struct db777_entry *entry = 0; void *src,*value = 0; char buf[8192],_keystr[513],*keystr = _keystr;
    max = *lenp, *lenp = 0;
    //if ( strcmp(DB->name,"addrinfos") != 0 )
    //    transactions = 0;
    if ( db777_matrixalloc(DB) != 0 )
    {
        if ( (src= db777_matrixptr(&matrixi,transactions,DB,key,keylen)) != 0 )
        {
            *lenp = DB->valuesize;
            memcpy(dest,src,DB->valuesize);
            //printf("GETMATRIX %s[%d] %p -> %x\n",DB->name,*(int *)key,src,*(int *)value);
            return(dest);
        } else printf("db777_get: unexpected missing matrixi ptr for %s key.%u\n",DB->name,*(int *)key);
        return(0);
    }
    else
    {
        if ( (DB->flags & DB777_RAM) != 0 )
        {
            db777_lock(DB);
            HASH_FIND(hh,DB->table,key,keylen,entry);
            db777_unlock(DB);
            if ( entry != 0 )
            {
                *lenp = entry->valuelen;
                if ( entry->valuesize == 0 )
                    memcpy(&value,entry->value,sizeof(value));
                else value = entry->value;
                if ( entry->valuelen <= max )
                    memcpy(dest,value,entry->valuelen);
                else return(0);
                return(dest);
            }
        }
        if ( (DB->flags & DB777_HDD) != 0 )
        {
            *lenp = max;
            if ( (dest= db777_read(dest,lenp,transactions,DB,key,keylen,DB->flags & DB777_RAM)) != 0 )
                return(dest);
        }
    }
    if ( 0 && (DB->flags & DB777_NANO) != 0 && DB->reqsock != 0 )
    {
        for (i=0; i<keylen; i++)
            if ( (c= ((uint8_t *)key)[i]) < 0x20 || c >= 0x80 )
                break;
        if ( i != keylen )
        {
            if ( keylen > sizeof(_keystr)/2 )
                key = malloc((keylen << 1) + 1);
            init_hexbytes_noT(keystr,key,keylen);
        }
        else
        {
            keystr = key;
            if ( keystr[keylen] != 0 )
            {
                printf("db777_get: adding null terminator\n");
                keystr[keylen] = 0;
            }
        }
        if ( keylen < sizeof(buf)-128 )
        {
            sprintf(buf,"{\"destplugin\":\"db777\",\"method\":\"get\",\"coin\":\"%s\",\"DB\":\"%s\",\"key\":\"%s\"}",DB->coinstr,DB->name,keystr);
            nn_send(DB->reqsock,buf,(int32_t)strlen(buf)+1,0);
            printf("db777_get: sent.(%s)\n",buf);
        } else printf("db777_get: keylen.%d too big for buf\n",keylen);
    }
    return(0);
}

int32_t db777_set(int32_t flags,void *transactions,struct db777 *DB,void *key,int32_t keylen,void *value,int32_t valuelen)
{
    struct db777_entry *entry = 0; void *db,*newkey,*dest,*obj = 0; int32_t ismatrix,matrixi,retval = 0;
    ismatrix = db777_matrixalloc(DB);
    //if ( strcmp(DB->name,"addrinfos") != 0 )
    //    transactions = 0;
    if ( DB->valuesize != 0 && valuelen > DB->valuesize )
    {
        if ( ismatrix == 0 || (ismatrix != 0 && ((*(uint32_t *)key % DB777_MATRIXROW) != 0 || valuelen != DB777_MATRIXROW*DB->valuesize)) )
        {
            printf("%s UNEXPECTED SIZE SET.%08x keylen.%d | value %x len.%d value.%p (%s)\n",DB->name,*(int *)key,keylen,*(int *)value,valuelen,value,value);
            return(-1);
        }
    }
    if ( ((DB->flags & flags) & DB777_RAM) != 0 )
    {
        if ( ismatrix != 0 )
        {
            if ( (dest= db777_matrixptr(&matrixi,transactions,DB,key,keylen)) != 0 )
            {
                DB->dirty[matrixi] = 1;
                //printf("SETMATRIX %s[%d] %p <- %x\n",DB->name,*(int *)key,dest,*(int *)value);
                memcpy(dest,value,valuelen);
                if ( valuelen < DB->valuesize )
                    memset(&((uint8_t *)dest)[valuelen],0,DB->valuesize - valuelen);
            }
            else printf("db777_set: out of bounds rawind.%u vs. %d (width %d)\n",*(int *)key,DB->matrixentries,DB777_MATRIXROW);
        }
        else
        {
            db777_lock(DB);
            HASH_FIND(hh,DB->table,key,keylen,entry);
            if ( entry == 0 )
            {
                if ( DB->valuesize == 0 )
                {
                    entry = calloc(1,sizeof(*entry) + sizeof(void *));
                    if ( valuelen > 0 )
                    {
                        obj = malloc(valuelen);
                        memcpy(entry->value,&obj,sizeof(obj));
                    }
                }
                else if ( valuelen == DB->valuesize )
                {
                    entry = calloc(1,sizeof(*entry) + valuelen);
                    obj = entry->value;
                    entry->valuesize = DB->valuesize;
                }
                else
                {
                    printf("%s mismatched valuelen.%d vs DB->valuesize.%d\n",DB->name,valuelen,DB->valuesize);
                    db777_unlock(DB);
                    return(-1);
                }
                entry->allocsize = entry->valuelen = valuelen;
                entry->keylen = keylen;
                entry->dirty = 1;
                if ( obj != 0 )
                    memcpy(obj,value,valuelen);
                else printf("%s keylen.%d unexpected null obj\n",DB->name,keylen);
                newkey = malloc(keylen);
                memcpy(newkey,key,keylen);
                HASH_ADD_KEYPTR(hh,DB->table,newkey,keylen,entry);
                retval = 0;
            }
            else
            {
                entry->dirty = 1;
                if ( entry->valuesize == 0 )
                {
                    memcpy(&obj,entry->value,sizeof(obj));
                    if ( entry->valuelen != valuelen || memcmp(obj,value,valuelen) != 0 )
                    {
                        if ( entry->allocsize >= valuelen )
                        {
                            memcpy(obj,value,valuelen);
                            if ( valuelen < entry->allocsize )
                                memset(&((uint8_t *)obj)[valuelen],0,entry->allocsize - valuelen);
                        }
                        else
                        {
                            entry->allocsize = valuelen;
                            if ( entry->linked == 0 )
                                obj = realloc(obj,entry->allocsize);
                            else
                            {
                                printf("%s unexpected realloc of linked entry\n",DB->name);
                                obj = calloc(1,entry->allocsize);
                                entry->linked = 1;
                            }
                            memcpy(obj,value,entry->allocsize);
                            memcpy(entry->value,&obj,sizeof(obj));
                        }
                        entry->valuelen = valuelen;
                    }
                    retval = 0;
                }
                else if ( entry->valuesize != valuelen || valuelen != DB->valuesize )
                    printf("entry->valuesize.%d DB->valuesize.%d vs valuesize.%d??\n",entry->valuesize,DB->valuesize,valuelen);
                else
                {
                    if ( memcmp(entry->value,value,valuelen) != 0 )
                        memcpy(entry->value,value,valuelen);
                    retval = 0;
                }
            }
            db777_unlock(DB);
        }
    }
    else if ( ((DB->flags & flags) & DB777_HDD) != 0 )
    {
        if ( ismatrix != 0 && ((*(uint32_t *)key % DB777_MATRIXROW) != 0 || valuelen != DB777_MATRIXROW*DB->valuesize) )
        {
            printf("%s UNEXPECTED db777_set: %x keylen.%d valuelen.%d %x\n",DB->name,*(int *)key,keylen,valuelen,*(int *)value);
            return(-1);
        }
        else
        {
            db = DB->asyncdb != 0 ? DB->asyncdb : DB->db;
            if ( (obj= sp_object(db)) == 0 )
                retval = -3;
            if ( sp_set(obj,"key",key,keylen) != 0 || sp_set(obj,"value",value,valuelen) != 0 )
            {
                sp_destroy(obj);
                printf("error setting key/value %s[%d]\n",DB->name,*(int *)key);
                retval = -4;
            }
            else
            {
                Added++;
                retval = sp_set((transactions != 0 ? transactions : db),obj);
                //if ( strcmp(DB->name,"ledger") == 0 )
                //    printf("retval.%d %s key.%u valuelen.%d\n",retval,DB->name,*(int *)key,valuelen);
            }
        }
        //if ( ismatrix != 0 )
        //    printf("%s save key.%u valuelen.%d\n",DB->name,*(int *)key,valuelen);
    }
    return(retval);
}

int32_t zcmp(uint8_t *buf,int32_t len)
{
    int32_t i;
    for (i=0; i<len; i++)
        if ( buf[i] != 0 )
            return(1);
    return(0);
}

int32_t db777_add(int32_t forceflag,void *transactions,struct db777 *DB,void *key,int32_t keylen,void *value,int32_t valuelen)
{
    void *val = 0;
    int32_t retval,flag = 0,allocsize = sizeof(DB->checkbuf);
    if ( DB == 0 )
        return(-1);
    //if ( strcmp(DB->name,"addrinfos") != 0 )
    //    transactions = 0;
    if ( forceflag <= 0 && (val= db777_get(DB->checkbuf,&allocsize,transactions,DB,key,keylen)) != 0 )
    {
        flag = 1;
        if ( allocsize == valuelen && memcmp(val,value,valuelen) == 0 )
        {
            if ( forceflag < 0 )
                Duplicate++;
            return(0);
        }
    }
    if ( forceflag < 0 && flag != 0 && (db777_matrixalloc(DB) == 0 || zcmp(val,allocsize) != 0) )
    {
        int i;
        for (i=0; i<60&&i<valuelen; i++)
            printf("%02x ",((uint8_t *)value)[i]);
        printf("value len.%d %s | key %x keylen.%d\n",valuelen,DB->name,*(int *)key,keylen);
        if ( val != 0 )
        {
            for (i=0; i<60&&i<allocsize; i++)
                printf("%02x ",((uint8_t *)val)[i]);
            printf("save len %d %s\n",allocsize,DB->name);
        }
        Mismatch++, printf("%s duplicate.%d mismatch.%d | keylen.%d valuelen.%d -> allocsize.%d\n",DB->name,Duplicate,Mismatch,keylen,valuelen,allocsize);
    }
    retval = db777_set((DB->flags & DB777_RAM) == 0 ? DB777_HDD : DB777_RAM,transactions,DB,key,keylen,value,valuelen);
    return(retval);
}

int32_t db777_addstr(struct db777 *DB,char *key,char *value)
{
    return(db777_write(0,DB,key,(int32_t)strlen(key)+1,value,(int32_t)strlen(value)+1));
}

int32_t db777_findstr(char *retbuf,int32_t max,struct db777 *DB,char *key)
{
    void *val;
    int32_t valuesize = max;
    retbuf[0] = 0;
    if ( key == 0 || key[0] == 0 )
        return(-1);
    if ( (val= db777_get(retbuf,&valuesize,0,DB,key,(int32_t)strlen(key)+1)) != 0 )
    {
        //max--;
        //if ( valuesize > 0 )
        //    memcpy(retbuf,val,(valuesize < max) ? valuesize : max), retbuf[max] = 0;
    }
    // printf("found str.(%s) -> (%s)\n",key,retbuf);
    return(valuesize);
}

int32_t db777_delete(int32_t flags,void *transactions,struct db777 *DB,void *key,int32_t keylen)
{
    struct db777_entry *entry; void *obj; int32_t retval = -1;
    if ( DB != 0 )
    {
        if ( db777_matrixalloc(DB) != 0 )
            printf("db777_delete %s %u: unexpected delete\n",DB->name,*(int *)key);
        else
        {
            if ( ((flags & DB->flags) & DB777_HDD) != 0 )
            {
                if ( (obj= sp_object(DB->db)) == 0 )
                {
                    if ( sp_set(obj,"key",key,keylen) == 0 )
                        retval = sp_delete((transactions != 0) ? transactions : DB->db,obj);
                    else sp_destroy(obj);
                }
            }
            if ( ((flags & DB->flags) & DB777_RAM) != 0 )
            {
                db777_lock(DB);
                HASH_FIND(hh,DB->table,key,keylen,entry);
                if ( entry != 0 )
                {
                    HASH_DEL(DB->table,entry);
                    db777_free_entry(entry);
                }
                db777_unlock(DB);
            }
        }
    }
    return(retval);
}

int32_t db777_flush(void *transactions,struct db777 *DB)
{
    struct db777_entry *entry,*tmp; void *obj; uint32_t key; int32_t valuelen,flushed = 0,i,n = 0,numerrs = 0;
    //if ( strcmp(DB->name,"addrinfos") != 0 )
    //    transactions = 0;
    if ( (DB->flags & DB777_RAM) != 0 )
    {
        db777_lock(DB);
        if ( db777_matrixalloc(DB) != 0 )
        {
            valuelen = DB->valuesize * DB777_MATRIXROW;
            for (i=0; i<DB->matrixentries; i++)
            {
                if ( DB->matrix[i] != 0 )
                {
                    if  ( DB->dirty[i] != 0 )
                    {
                        key = (i * DB777_MATRIXROW);
                        DB->dirty[i] = (db777_set(DB777_HDD,transactions,DB,&key,sizeof(key),DB->matrix[i],valuelen) != 0);
                        n++, flushed += valuelen;
                    } //else printf("db777_flush: %s not dirty row[%d]\n",DB->name,i);
                }
                else
                {
                    //printf("empty %s matrixrow[%d]\n",DB->name,i);
                    break;
                }
            }
        }
        else
        {
            HASH_ITER(hh,DB->table,entry,tmp)
            {
                if ( entry->valuesize == 0 && entry->valuelen < entry->valuesize )
                {
                    memcpy(&obj,entry->value,sizeof(obj));
                    entry->allocsize = entry->valuelen;
                    obj = realloc(obj,entry->valuelen);
                    memcpy(entry->value,&obj,sizeof(obj));
                }
                if ( entry->dirty != 0 )
                {
                    if ( (DB->flags & DB777_HDD) != 0 )
                    {
                        //db777_delete(DB777_HDD,transactions,DB,entry->hh.key,entry->keylen);
                        obj = (DB->valuesize == 0) ? *(void **)entry->value : entry->value;
                        entry->dirty = (db777_set(DB777_HDD,transactions,DB,entry->hh.key,entry->keylen,obj,entry->valuelen) != 0);
                        //if ( strcmp(DB->name,"revaddrs") == 0 )
                        //    printf("%d: (%s).%d\n",*(int32_t *)entry->hh.key,obj,entry->valuelen);
                        numerrs += entry->dirty;
                        n++, flushed += entry->valuelen;
                    }
                }
            }
        }
        db777_unlock(DB);
    }
   if ( Debuglevel > 2 )
       printf("(%s %d).%d ",DB->name,flushed,n);
    return(-numerrs);
}

int32_t db777_sync(void *transactions,struct env777 *DBs,int32_t flags)
{
    int32_t i,err = 0;
    if ( (flags & DB777_FLUSH) != 0 )
    {
        //transactions = sp_begin(DBs->env);
        for (i=0; i<DBs->numdbs; i++)
            if ( (DBs->dbs[i].flags & DB777_FLUSH) != 0 )
                db777_flush(transactions,&DBs->dbs[i]);
        if ( transactions != 0 && (err= sp_commit(transactions)) != 0 )
            printf("db777_sync err.%d\n",err);
    }
    if ( (flags & ENV777_BACKUP) != 0 )
        sp_set(DBs->ctl,"backup.run");
    return(err);
}

uint64_t db777_ctlinfo64(void *ctl,char *field)
{
	void *obj,*ptr; uint64_t val = 0;
    if ( (obj= sp_get(ctl,field)) != 0 )
    {
        if ( (ptr= sp_get(obj,"value",NULL)) != 0 )
            val = calc_nxt64bits(ptr);
        sp_destroy(obj);
    }
    return(val);
}

char *db777_errstr(void *ctl)
{
   	void *obj,*ptr; static char errstr[1024];
    errstr[0] = 0;
    if ( (obj= sp_get(ctl,"sophia.error")) != 0 )
    {
        if ( (ptr= sp_get(obj,"value",NULL)) != 0 )
            strcpy(errstr,ptr);
        sp_destroy(obj);
    }
    return(errstr);
}

char **db777_index(int32_t *nump,struct db777 *DB,int32_t max)
{
    void *obj,*cursor; uint32_t *addrindp; char *coinaddr,**coinaddrs = 0;
    int32_t addrlen,len,n = 0;
    *nump = 0;
    if ( DB == 0 || DB->db == 0 )
        return(0);
    obj = sp_object(DB->db);
    if ( (cursor= sp_cursor(DB->db,obj)) != 0 )
    {
        coinaddrs = (char **)calloc(sizeof(char *),max+1);
        while ( (obj= sp_get(cursor,obj)) != 0 )
        {
            addrindp = sp_get(obj,"value",&len);
            coinaddr = sp_get(obj,"key",&addrlen);
            if ( len == sizeof(*addrindp) && *addrindp <= max )
            {
                coinaddrs[*addrindp] = calloc(1,addrlen + 1);
                memcpy(coinaddrs[*addrindp],coinaddr,addrlen);
                n++;
             } //else printf("n.%d wrong len.%d or overflow %d vs %d\n",n,len,*addrindp,max);
        }
        sp_destroy(cursor);
    }
    *nump = n;
    return(coinaddrs);
}

void **db777_copy_all(int32_t *nump,struct db777 *DB,char *field,int32_t size)
{
    void *obj,*cursor,*value,*ptr,**ptrs = 0;
    int32_t len,max,n = 0;
    *nump = 0;
    max = 100;
    if ( DB == 0 || DB->db == 0 )
        return(0);
    obj = sp_object(DB->db);
    if ( (cursor= sp_cursor(DB->db,obj)) != 0 )
    {
        ptrs = (void **)calloc(sizeof(void *),max+1);
        while ( (obj= sp_get(cursor,obj)) != 0 )
        {
            value = sp_get(obj,field,&len);
            if ( len > 0 && (size == 0 || len == size) )
            {
                ptr = malloc(len);
                memcpy(ptr,value,len);
                //printf("%p set [%d] <- %p.len %d\n",ptrs,n,ptr,len);
                ptrs[n++] = ptr;
                if ( n >= max )
                {
                    max = n + 100;
                    ptrs = (void **)realloc(ptrs,sizeof(void *)*(max+1));
                }
            } //else printf("wrong size.%d\n",len);
        }
        sp_destroy(cursor);
    }
    if ( ptrs != 0 )
        ptrs[n] = 0;
    *nump = n;
    //printf("ptrs.%p [0] %p numdb.%d\n",ptrs,ptrs[0],n);
    return(ptrs);
}

int32_t db777_dump(struct db777 *DB,int32_t binarykey,int32_t binaryvalue)
{
    void *obj,*cursor,*value,*key; char keyhex[8192],valuehex[8192];
    int32_t keylen,len,max,n = 0;
    max = 100;
    if ( DB == 0 || DB->db == 0 )
        return(0);
    obj = sp_object(DB->db);
    if ( (cursor= sp_cursor(DB->db,obj)) != 0 )
    {
        while ( (obj= sp_get(cursor,obj)) != 0 )
        {
            value = sp_get(obj,"value",&len);
            key = sp_get(obj,"key",&keylen);
            if ( binarykey != 0 )
                init_hexbytes_noT(keyhex,key,keylen), key = keyhex;
            if ( binaryvalue != 0 )
                init_hexbytes_noT(valuehex,value,len), value = valuehex;
            printf("%-5d: %16s keylen.%d | %s len.%d\n",n,key,keylen,value,len);
            n++;
        }
        sp_destroy(cursor);
    }
    return(n);
}

/*int32_t eligible_lbserver(char *server)
{
    cJSON *json; int32_t len,keylen,retval = 1; char *jsonstr,*status,*valstr = "{\"status\":\"enabled\"}";
    if ( server == 0 || server[0] == 0 || ismyaddress(server) != 0 || is_remote_access(server) == 0 )
        return(0);
    keylen = (int32_t)strlen(server)+1;
    if ( (jsonstr= db777_findM(&len,0,DB_NXTaccts,(uint8_t *)server,keylen)) != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (status= cJSON_str(cJSON_GetObjectItem(json,"status"))) != 0 )
            {
                //printf("(%s) status.(%s)\n",server,status);
                if ( strcmp(status,"disabled") != 0 )
                    retval = 0;
            }
            free_json(json);
        }
        free(jsonstr);
    }
    else db777_add(1,0,DB_NXTaccts,server,keylen,valstr,(int32_t)strlen(valstr)+1);
    return(retval);
}*/

#endif
#endif
#endif
#include <stdint.h>
extern int32_t Debuglevel;

