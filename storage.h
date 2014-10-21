//
//  storage.h
//  libjl777
//
//  Created by jl777 on 10/19/14.
//  Copyright (c) 2014 jl777. MIT license
//

#ifndef libjl777_storage_h
#define libjl777_storage_h

#include "db.h"

#define MAX_KADEMLIA_STORAGE (1024L * 1024L * 1024L)
#define PUBLIC_DATA 0
#define PRIVATE_DATA 1

struct kademlia_storage
{
    uint64_t modified,keyhash;
    char key[MAX_NXTTXID_LEN];
    uint32_t datalen,laststored,lastaccess,createtime;
    unsigned char data[];
};
long Total_stored;
DB *Public_dbp,*Private_dbp;

union _storage_type { uint64_t destbits; int32_t selector; };
struct storage_queue_entry { struct kademlia_storage *sp; union _storage_type U; };

int32_t init_storage()
{
    int ret;
    if ( (ret= db_create(&Public_dbp, NULL, 0)) != 0 )
    {
        printf("error.%d creating Public_dbp database\n",ret);
        return(ret);
    }
    if ( (ret= db_create(&Private_dbp, NULL, 0)) != 0 )
    {
        printf("error.%d creating Private_dbp database\n",ret);
        return(ret);
    }
    if ( (ret= Public_dbp->open(Public_dbp,NULL,"storage/public.db",NULL,DB_HASH,DB_CREATE,0)) != 0 )
    {
        printf("error.%d creating Public_dbp database\n",ret);
        return(ret);
    }
    if ( (ret= Private_dbp->open(Private_dbp,NULL,"storage/private.db",NULL,DB_HASH,DB_CREATE,0)) != 0 )
    {
        printf("error.%d creating Private_dbp database\n",ret);
        return(ret);
    }
    return(0);
}
    
DB *get_selected_database(int32_t selector)
{
   return((selector == 0) ? Public_dbp : Private_dbp);
}

struct kademlia_storage *find_storage(int32_t selector,char *keystr)
{
    DB *dbp = get_selected_database(selector);
    uint64_t keybits = calc_nxt64bits(keystr);
    DBT key,data;
    int ret;
    /*uint64_t hashval;
    struct hashtable **hpp = get_selected_hashtable(selector);
    hashval = MTsearch_hashtable(hpp,key);
    if ( hashval == HASHSEARCH_ERROR )
        return(0);
    else return((*hpp)->hashtable[hashval]);*/
    memset(&key,0,sizeof(DBT));
    memset(&data,0,sizeof(DBT));
    key.data = &keybits;
    key.size = sizeof(keybits);
    data.flags = 0;//DB_DBT_USERMEM;
    if ( (ret= dbp->get(dbp,NULL,&key,&data,0)) != 0 )
    {
        if ( ret != DB_NOTFOUND )
            printf("DB get error.%d\n",ret);
        else return(0);
    }
    return((struct kademlia_storage *)data.data);
    /*else
    {
        if ( data.ulen != sp->datalen || memcmp(buf,sp->data,sp->datalen) != 0 )
        {
            printf("DB get lens %d %d vs cmp.%d\n",data.ulen,sp->datalen,memcmp(buf,sp->data,sp->datalen));
        }
    }*/
}

/*int32_t set_storage_data(struct kademlia_storage *sp,char *datastr,uint8_t *cacheddata,int32_t datalen)
{
    uint8_t *olddata = sp->data;
    int32_t identical,oldlen = sp->datalen;
    sp->datalen = identical = 0;
    if ( cacheddata == 0 && is_hexstr(datastr) != 0 )
    {
        sp->datalen = (int32_t)(strlen(datastr) / 2);
        sp->data = calloc(1,sp->datalen);
        decode_hex(sp->data,sp->datalen,datastr);
    }
    else if ( cacheddata != 0 && datalen != 0 )
    {
        sp->datalen = datalen;
        sp->data = cacheddata;
    } else sp->data = 0;
    Total_stored += (sp->datalen - oldlen);
    if ( olddata != 0 && sp->data != 0 && oldlen == sp->datalen && memcmp(sp->data,olddata,oldlen) == 0 )
        identical = 1;
    if ( olddata != 0 )
        free(olddata);
    return(identical);
}*/

struct kademlia_storage *add_storage(int32_t selector,char *keystr,char *datastr,uint8_t *cacheddata,int32_t datalen)
{
    DB *dbp = get_selected_database(selector);
    uint32_t now = (uint32_t)time(NULL);
    int32_t ret,createdflag = 0;
    unsigned char databuf[8192];
    DBT key,data;
    //struct storage_queue_entry *ptr;
    struct kademlia_storage *sp;
    //struct hashtable **hpp = get_selected_hashtable(selector);
    if ( Total_stored > MAX_KADEMLIA_STORAGE )
    {
        printf("Total_stored %s > %s\n",_mbstr(Total_stored),_mbstr2(MAX_KADEMLIA_STORAGE));
        return(0);
    }
    datalen = (int32_t)strlen(datastr) / 2;
    if ( datalen > sizeof(databuf) )
        return(0);
    decode_hex(databuf,datalen,datastr);
    if ( (sp= find_storage(selector,keystr)) == 0 || sp->datalen != datalen || memcmp(sp->data,databuf,datalen) != 0 )
    {
        createdflag = (sp == 0);
        sp = calloc(1,sizeof(*sp) + datalen);
        sp->keyhash = calc_nxt64bits(keystr);
        strcpy(sp->key,keystr);
        memcpy(sp->data,databuf,datalen);
        
        memset(&key,0,sizeof(DBT));
        memset(&data,0,sizeof(DBT));
        key.data = &sp->keyhash;
        key.size = sizeof(sp->keyhash);
        data.data = sp;
        data.size = (sizeof(*sp) + datalen);
        if ( (ret= dbp->put(dbp,NULL,&key,&data,0)) != 0 )
            printf("DB put error.%d\n",ret);
        else
        {
            dbp->sync(dbp,0);
            printf("created.%d DB entry for %s\n",createdflag,keystr);
        }

        if ( (sp= find_storage(selector,keystr)) != 0 )
        {
            sp->laststored = now;
            if ( createdflag != 0 )
                sp->createtime = now;
            if ( memcmp(sp->data,databuf,datalen) != 0 )
                printf("data cmp error\n");
        } else printf("couldnt find sp in DB that was just added\n");
    }
    if ( sp != 0 )
        sp->lastaccess = now;
    /*sp = MTadd_hashtable(&createdflag,hpp,key);
    set_storage_data(sp,datastr,cacheddata,datalen);
    sp->laststored = sp->lastaccess = now;
    if ( createdflag != 0 )
    {
        Total_stored += sizeof(*sp);
        sp->keyhash = calc_nxt64bits(key);
        if ( datastr != 0 )
        {
            ptr = calloc(1,sizeof(*ptr));
            ptr->U.selector = selector;
            ptr->sp = sp;
            printf("cacheQ %p",ptr);
            queue_enqueue(&cacheQ,ptr);
        }
        sp->createtime = now;
        printf("add storage (%s) %s %d bytes | Total stored.%ld %s\n",key,datastr,datalen,Total_stored,_mbstr(Total_stored));
    }
    else printf("add_storage warning: already created %s\n",key);*/
    return(sp);
}

/*int32_t add_to_storage(int32_t selector,char *key,void *data,int32_t datalen)
{
    if ( add_storage(selector,key,0,data,datalen) != 0 )
        return(0);
    else return(-1);
}*/

struct kademlia_storage *kademlia_getstored(int32_t selector,uint64_t keyhash,char *datastr)
{
    uint32_t now;
    char key[64];
    struct kademlia_storage *sp;
    expand_nxt64bits(key,keyhash);
    now = (uint32_t)time(NULL);
    if ( (sp= find_storage(selector,key)) != 0 )
        sp->lastaccess = now;
    if ( datastr == 0 )
        return(sp);
    if ( (sp= add_storage(selector,key,datastr,0,0)) != 0 )
        sp->laststored = now;
    return(sp);
}

struct kademlia_storage **find_closer_Kstored(int32_t selector,uint64_t refbits,uint64_t newbits)
{
    struct kademlia_storage *sp,**sps = 0;
    int32_t dist,refdist,i,n = 0;
    int64_t changed;
    /*struct hashtable **hpp = get_selected_hashtable(selector);
    sps = (struct kademlia_storage **)hashtable_gather_modified(&changed,(*hpp),1); // partial MT support
    if ( sps != 0 )
    {
        for (i=0; i<changed; i++)
        {
            sp = sps[i];
            refdist = bitweight(refbits ^ sp->keyhash);
            dist = bitweight(newbits ^ sp->keyhash);
            if ( dist < refdist )
            {
                if ( i != n )
                    sps[n++] = sp;
            }
        }
        sps[n] = 0;
    }*/
    return(sps);
}

int32_t kademlia_pushstore(int32_t selector,uint64_t refbits,uint64_t newbits)
{
    int32_t n = 0;
    struct storage_queue_entry *ptr;
    struct kademlia_storage **sps,*sp;
    if ( (sps= find_closer_Kstored(selector,refbits,newbits)) != 0 )
    {
        while ( (sp= sps[n++]) != 0 )
        {
            printf("queue.%d to %llu\n",n,(long long)newbits);
            ptr = calloc(1,sizeof(*ptr));
            ptr->U.destbits = newbits;
            ptr->sp = sp;
            queue_enqueue(&storageQ,ptr);
        }
        free(sps);
        if ( Debuglevel > 0 )
            printf("Queue n.%d pushstore to %llu\n",n,(long long)newbits);
    }
    return(n);
}

uint64_t process_storageQ()
{
    uint64_t send_kademlia_cmd(uint64_t nxt64bits,struct pserver_info *pserver,char *kadcmd,char *NXTACCTSECRET,char *key,char *datastr);
    struct storage_queue_entry *ptr;
    char key[64],datastr[8193];
    struct kademlia_storage *sp;
    uint64_t txid = 0;
    struct coin_info *cp = get_coin_info("BTCD");
    if ( (ptr= queue_dequeue(&storageQ)) != 0 )
    {
        printf("dequeued storageQ %p\n",ptr);
        sp = ptr->sp;
        init_hexbytes_noT(datastr,sp->data,sp->datalen);
        expand_nxt64bits(key,sp->keyhash);
        txid = send_kademlia_cmd(ptr->U.destbits,0,"store",cp->srvNXTACCTSECRET,key,datastr);
        if ( Debuglevel > 0 )
            printf("txid.%llu send queued push storage key.(%s) to %llu\n",(long long)txid,key,(long long)ptr->U.destbits);
        free(ptr);
    }
    return(txid);
}

/*int32_t process_cacheQ()
{
    struct storage_queue_entry *ptr;
    DBT key, data;
    //char key[64];
    uint8_t buf[8192];
    DB *dbp;
    int ret;
    struct kademlia_storage *sp;
    if ( (ptr= queue_dequeue(&cacheQ)) != 0 )
    {
        printf("dequeued cacheQ %p\n",ptr);
        sp = ptr->sp;
        memset(&key,0,sizeof(DBT));
        memset(&data,0,sizeof(DBT));
        key.data = &sp->keyhash;
        key.size = sizeof(sp->keyhash);
        data.data = sp->data;
        data.size = sp->datalen;
        dbp = (ptr->U.selector == 0) ? Public_dbp : Private_dbp;
        if ( (ret= dbp->put(dbp,NULL,&key,&data,0)) != 0 )
            printf("DB put error.%d\n",ret);
        
        memset(&key,0,sizeof(DBT));
        memset(&data,0,sizeof(DBT));
        key.data = &sp->keyhash;
        key.size = sizeof(sp->keyhash);
        data.data = buf;
        data.ulen = sizeof(buf);
        data.flags = DB_DBT_USERMEM;
        if ( (ret= dbp->get(dbp,NULL,&key,&data,0)) != 0 )
            printf("DB get error.%d\n",ret);
        else
        {
            if ( data.ulen != sp->datalen || memcmp(buf,sp->data,sp->datalen) != 0 )
            {
                printf("DB get lens %d %d vs cmp.%d\n",data.ulen,sp->datalen,memcmp(buf,sp->data,sp->datalen));
            }
        }
        //expand_nxt64bits(key,sp->keyhash);
        //update_coincache(Global_mp->storage_fps[ptr->U.selector],key,sp->data,sp->datalen);
        free(ptr);
    }
    return(0);
}*/
#endif
