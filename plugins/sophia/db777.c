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
#include "coins777.c"
#include "storage.c"


#define SOPHIA_USERDIR "/user"
uint64_t db777_ctlinfo64(void *ctl,char *field);
int32_t db777_add(int32_t forceflag,void *transactions,struct db777 *DB,void *key,int32_t keylen,void *value,int32_t len);
int32_t db777_addstr(struct db777 *DB,char *key,char *value);
int32_t db777_delete(struct db777 *DB,void *key,int32_t keylen);
int32_t db777_findstr(char *retbuf,int32_t max,struct db777 *DB,char *key);
void *db777_findM(int32_t *lenp,void *transactions,struct db777 *DB,void *key,int32_t keylen);
int32_t db777_close(struct db777 *DB);
int32_t db777_free(struct db777 *DB);
void **db777_copy_all(int32_t *nump,struct db777 *DB,char *field,int32_t size);
struct db777 *db777_getDB(char *dbname);
int32_t db777_backup(void *ctl);
void *db777_transaction(void *env,struct db777 *DB,void *transactions,void *key,int32_t keylen,void *value,int32_t len);
struct db777 *db777_create(char *specialpath,char *subdir,char *name,char *compression,int32_t restoreflag);
int32_t env777_start(int32_t dispflag,struct env777 *DBs);
char **db777_index(int32_t *nump,struct db777 *DB,int32_t max);
int32_t db777_dump(struct db777 *DB,int32_t binarykey,int32_t binaryvalue);

extern struct sophia_info SOPHIA;
extern struct db777 *DB_msigs,*DB_NXTaccts,*DB_nodestats,*DB_busdata;//,*DB_NXTassettx,;

#endif
#else
#ifndef crypto777_db777_c
#define crypto777_db777_c

#ifndef crypto777_db777_h
#define DEFINES_ONLY
#include "db777.c"
#undef DEFINES_ONLY
#endif

void *db777_find(void *transactions,struct db777 *DB,void *key,int32_t keylen)
{
    void *obj,*result = 0;
    int32_t len;
    if ( DB == 0 || DB->db == 0 || (obj= sp_object(DB->db)) == 0 )
        return(0);
    if ( sp_set(obj,"key",key,keylen) == 0 )
        result = sp_get(transactions != 0 ? transactions : DB->db,obj,&len);
    else sp_destroy(obj);
    return(result);
}

int32_t db777_delete(struct db777 *DB,void *key,int32_t keylen)
{
    void *obj;
    if ( DB == 0 || DB->db == 0 || (obj= sp_object(DB->db)) == 0 )
        return(0);
    if ( sp_set(obj,"key",key,keylen) == 0 )
        return(sp_delete(DB->db,obj));
    else sp_destroy(obj);
    return(-1);
}

void *db777_transaction(void *env,struct db777 *DB,void *transactions,void *key,int32_t keylen,void *value,int32_t len)
{
    void *obj;
    if ( transactions == 0 )
        return(sp_begin(env));
    else if ( key != 0 )
    {
        if ( (obj= sp_object(DB->db)) == 0 )
            return(0);
        if ( sp_set(obj,"key",key,keylen) != 0 || sp_set(obj,"value",value,len) != 0 )
        {
            sp_destroy(obj);
            sp_destroy(transactions);
            return(0);
        }
        if ( sp_set(transactions,obj) != 0 )
        {
            sp_destroy(transactions);
            return(0);
        }
        return(transactions);
    }
    if ( sp_commit(transactions) != 0 )
        printf("error commiting transaction\n");
    return(0);
}

uint32_t Duplicate,Mismatch,Added;
int32_t db777_add(int32_t forceflag,void *transactions,struct db777 *DB,void *key,int32_t keylen,void *value,int32_t len)
{
    void *obj = 0,*val = 0;
    int32_t allocsize = 0;
    if ( DB == 0 || DB->db == 0 )
        return(-1);
    if ( forceflag <= 0 && (obj= db777_find(transactions,DB,key,keylen)) != 0 )
    {
        if ( (val= sp_get(obj,"value",&allocsize)) != 0 )
        {
            if ( allocsize == len && memcmp(val,value,len) == 0 )
            {
                if ( forceflag < 0 )
                {
                    Duplicate++;
                    //if ( (rand() % 1000) == 0 )
                    //    printf("found duplicate len.%d | duplicate.%d mismatch.%d\n",len,Duplicate,Mismatch);
                }
                sp_destroy(obj);
                return(0);
            }
        }
    }
    if ( forceflag < 0 && allocsize != 0 )
    {
        int i;
        for (i=0; i<60&&i<len; i++)
            printf("%02x ",((uint8_t *)value)[i]);
        printf("value len.%d\n",len);
        if ( val != 0 )
        {
            for (i=0; i<60&&i<allocsize; i++)
                printf("%02x ",((uint8_t *)val)[i]);
            printf("saved %d\n",allocsize);
        }
        Mismatch++, printf("duplicate.%d mismatch.%d | keylen.%d len.%d -> allocsize.%d\n",Duplicate,Mismatch,keylen,len,allocsize);
    }
    if ( obj != 0 )
        sp_destroy(obj);
    if ( (obj= sp_object(DB->asyncdb != 0 ? DB->asyncdb : DB->db)) == 0 )
        return(-3);
    if ( sp_set(obj,"key",key,keylen) != 0 || sp_set(obj,"value",value,len) != 0 )
    {
        sp_destroy(obj);
        return(-4);
    }
    if ( forceflag < 1 )
        Added++;
    //printf("DB.%p add.[%p %d] val.%p %d [crcs %u %u]\n",DB,key,keylen,value,len,_crc32(0,key,keylen),_crc32(0,value,len));
    return(sp_set(transactions != 0 ? transactions : (DB->asyncdb != 0 ? DB->asyncdb : DB->db),obj));
}

int32_t db777_addstr(struct db777 *DB,char *key,char *value)
{
    return(db777_add(1,0,DB,key,(int32_t)strlen(key)+1,value,(int32_t)strlen(value)+1));
}

int32_t db777_findstr(char *retbuf,int32_t max,struct db777 *DB,char *key)
{
    void *obj,*val;
    int32_t valuesize = -1;
    if ( (obj= db777_find(0,DB,key,(int32_t)strlen(key)+1)) != 0 )
    {
        if ( (val= sp_get(obj,"value",&valuesize)) != 0 )
        {
            max--;
            memcpy(retbuf,val,(valuesize < max) ? valuesize : max), retbuf[max] = 0;
        } else retbuf[0] = 0;
       // printf("found str.(%s) -> (%s)\n",key,retbuf);
        sp_destroy(obj);
    }
    return(valuesize);
}

void *db777_findM(int32_t *lenp,void *transactions,struct db777 *DB,void *key,int32_t keylen)
{
    void *obj,*val,*ptr = 0;
    int32_t valuesize = -1;
    if ( (obj= db777_find(transactions,DB,key,keylen)) != 0 )
    {
       //printf("found keylen.%d\n",keylen);
        if ( (val= sp_get(obj,"value",&valuesize)) != 0 )
        {
            ptr = calloc(1,valuesize+1);
            memcpy(ptr,val,valuesize);
            *lenp = valuesize;
        }
        sp_destroy(obj);
    }
    return(ptr);
}

int32_t db777_backup(void *ctl)
{
    return(sp_set(ctl,"backup.run"));
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
    printf("ptrs.%p [0] %p numdb.%d\n",ptrs,ptrs[0],n);
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
            printf("%-5d: %100s | %s\n",n,key,value);
            n++;
        }
        sp_destroy(cursor);
    }
    return(n);
}

int32_t eligible_lbserver(char *server)
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
}

#endif
#endif
