//
//  db777.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_db777_h
#define crypto777_db777_h
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "sophia.h"
#include "cJSON.h"
#include "system777.c"
#include "storage.c"

struct db777 { void *env,*ctl,*db,*asyncdb; char dbname[512],name[512],backupdir[512]; };

#define SOPHIA_USERDIR "/user"
struct db777 *db777_create(char *path,char *subdir,char *name,char *compression);
//void *db777_find(struct db777 *DB,void *key,int32_t keylen);
int32_t db777_add(int32_t forceflag,struct db777 *DB,void *key,int32_t keylen,void *value,int32_t len);
int32_t db777_addstr(struct db777 *DB,char *key,char *value);
int32_t db777_delete(struct db777 *DB,void *key,int32_t keylen);
int32_t db777_findstr(char *retbuf,int32_t max,struct db777 *DB,char *key);
void *db777_findM(int32_t *lenp,struct db777 *DB,void *key,int32_t keylen);
int32_t db777_close(struct db777 *DB);
int32_t db777_free(struct db777 *DB);
void **db777_copy_all(int32_t *nump,struct db777 *DB,char *field,int32_t size);
struct db777 *db777_getDB(char *dbname);
extern struct sophia_info SOPHIA;
extern struct db777 *DB_msigs,*DB_NXTaccts;//,*DB_NXTassettx,*DB_nodestats;

#endif
#else
#ifndef crypto777_db777_c
#define crypto777_db777_c

#ifndef crypto777_db777_h
#define DEFINES_ONLY
#include "db777.c"
#undef DEFINES_ONLY
#endif

void *db777_find(struct db777 *DB,void *key,int32_t keylen)
{
    void *obj,*result = 0;
    int32_t len;
    if ( DB == 0 || DB->db == 0 || (obj= sp_object(DB->db)) == 0 )
        return(0);
    if ( sp_set(obj,"key",key,keylen) == 0 )
        result = sp_get(DB->db,obj,&len);
    else sp_destroy(obj);
    return(result);
}

int32_t db777_delete(struct db777 *DB,void *key,int32_t keylen)
{
    void *obj;
    if ( (obj= db777_find(DB,key,keylen)) != 0 )
        return(sp_delete(DB->db,obj));
    return(-1);
}

int32_t db777_add(int32_t forceflag,struct db777 *DB,void *key,int32_t keylen,void *value,int32_t len)
{
    void *obj,*val;
    int32_t allocsize;
    if ( DB == 0 || DB->db == 0 )
        return(0);
    if ( forceflag == 0 && (obj= db777_find(DB,key,keylen)) != 0 )
    {
        if ( (val= sp_get(obj,"value",&allocsize)) != 0 )
        {
            if ( allocsize == len && memcmp(val,value,len) == 0 )
            {
                sp_destroy(obj);
                return(0);
            }
        }
        sp_destroy(obj);
    }
    if ( (obj= sp_object(DB->db)) == 0 )
        return(-1);
    if ( sp_set(obj,"key",key,keylen) != 0 || sp_set(obj,"value",value,len) != 0 )
    {
        sp_destroy(obj);
        return(-1);
    }
    return(sp_set(DB->db,obj));
}

int32_t db777_addstr(struct db777 *DB,char *key,char *value)
{
    return(db777_add(0,DB,key,(int32_t)strlen(key)+1,value,(int32_t)strlen(value)+1));
}

int32_t db777_findstr(char *retbuf,int32_t max,struct db777 *DB,char *key)
{
    void *obj,*val;
    int32_t valuesize = -1;
    if ( (obj= db777_find(DB,key,(int32_t)strlen(key)+1)) != 0 )
    {
        if ( (val= sp_get(obj,"value",&valuesize)) != 0 )
        {
            max--;
            memcpy(retbuf,val,(valuesize < max) ? valuesize : max), retbuf[max] = 0;
        } else retbuf[0] = 0;
        sp_destroy(obj);
    }
    return(valuesize);
}

void *db777_findM(int32_t *lenp,struct db777 *DB,void *key,int32_t keylen)
{
    void *obj,*val,*ptr = 0;
    int32_t valuesize = -1;
    if ( (obj= db777_find(DB,key,keylen)) != 0 )
    {
        if ( (val= sp_get(obj,"value",&valuesize)) != 0 )
        {
            ptr = calloc(1,valuesize);
            memcpy(ptr,val,valuesize);
            *lenp = valuesize;
        }
        sp_destroy(obj);
    }
    return(ptr);
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
    printf("ptrs.%p [0] %p numdb.%d\n",ptrs,ptrs[0],n);
    return(ptrs);
}

#endif
#endif
