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

struct db777 { void *env,*ctl,*db,*asyncdb; char dbname[96]; };

#define SOPHIA_USERDIR "/user"
struct db777 *db777_create(char *name,char *compression);
int32_t db777_add(struct db777 *DB,char *key,void *value,int32_t len);
int32_t db777_addstr(struct db777 *DB,char *key,char *value);
int32_t db777_delete(struct db777 *DB,char *key);
int32_t db777_findstr(char *retbuf,int32_t max,struct db777 *DB,char *key);
void *db777_findM(int32_t *lenp,struct db777 *DB,char *key);
int32_t db777_close(struct db777 *DB);
int32_t db777_free(struct db777 *DB);
void **db777_copy_all(int32_t *nump,struct db777 *DB);
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

int32_t db777_addstr(struct db777 *DB,char *key,char *value)
{
    void *obj;
    int32_t err;
    if ( DB == 0 || DB->db == 0 )
        return(-1);
    obj = sp_object(DB->db);
    printf("db777_addstr: (%s) < (%s)\n",key,value);
    if ( (err= sp_set(obj,"key",key,strlen(key))) != 0 || (err= sp_set(obj,"value",value,strlen(value))) != 0 )
        return(err);
    else return(sp_set(DB->db,obj));
}

int32_t db777_add(struct db777 *DB,char *key,void *value,int32_t len)
{
    void *obj;
    int32_t err;
    if ( DB == 0 || DB->db == 0 )
        return(-1);
    obj = sp_object(DB->db);
    printf("db777_add: (%s) < (%p).%d\n",key,value,len);
    if ( (err= sp_set(obj,"key",key,strlen(key))) != 0 || (err= sp_set(obj,"value",value,len)) != 0 )
        return(err);
    else return(sp_set(DB->db,obj));
}

int32_t db777_findstr(char *retbuf,int32_t max,struct db777 *DB,char *key)
{
    void *value,*result,*obj;
    int32_t err,valuesize = -1;
    if ( DB == 0 || DB->db == 0 )
        return(0);
    obj = sp_object(DB->db);
    if ( (err= sp_set(obj,"key",key,strlen(key))) != 0 )
        return(err);
    if ( (result= sp_get(DB->db,obj)) != 0 )
    {
        if ( (value= sp_get(result,"value",&valuesize)) != 0 )
        {
            if ( valuesize < max )
                memcpy(retbuf,value,valuesize);
            sp_destroy(value);
        }
        sp_destroy(result);
    }
    return(valuesize);
}

void *db777_findM(int32_t *lenp,struct db777 *DB,char *key)
{
    void *ptr=0,*value,*result,*obj;
    int32_t err,valuesize = -1;
    *lenp = 0;
    if ( DB == 0 || DB->db == 0 )
        return(0);
    obj = sp_object(DB->db);
    if ( (err= sp_set(obj,"key",key,strlen(key))) != 0 )
        return(0);
    if ( (result= sp_get(DB->db,obj)) != 0 )
    {
        if ( (value= sp_get(result,"value",&valuesize)) != 0 )
        {
            ptr = calloc(1,valuesize);
            memcpy(ptr,value,valuesize);
            *lenp = valuesize;
        }
        sp_destroy(result);
    }
    return(ptr);
}

void **db777_copy_all(int32_t *nump,struct db777 *DB)
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
            value = sp_get(obj,"value",&len);
            if ( len > 0 )
            {
                ptr = malloc(len);
                memcpy(ptr,value,len);
                printf("%p set [%d] <- %p.len %d\n",ptrs,n,ptr,len);
                ptrs[n++] = ptr;
                if ( n >= max )
                {
                    max = n + 100;
                    ptrs = (void **)realloc(ptrs,sizeof(void *)*(max+1));
                }
            } else printf("null value\n");
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
