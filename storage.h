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
long Total_stored,Storage_maxitems[2];
DB_ENV *Storage;
DB *Public_dbp,*Private_dbp;

struct storage_queue_entry { uint64_t keyhash,destbits; int32_t selector; };
int db_setup(const char *home,const char *data_dir,FILE *errfp,const char *progname);

int32_t init_SuperNET_storage()
{
    static int didinit;
    int ret;
#ifdef __linux__
    //return(0);
#endif
    if ( didinit != 0 )
        return(1);
    didinit = 1;

    ensure_directory("storage");
    ensure_directory("storage/data");
    //ret = db_setup("storage","data",stderr,"SuperNET");
    //printf("db_setup returns.%d\n",ret);

    if ( 1 )
    {
        if ( (ret = db_env_create(&Storage,0)) != 0 )
        {
            fprintf(stderr,"Error creating environment handle: %s\n",db_strerror(ret));
            return(-1);
        } else printf("Storage environment created\n");
      	//(void)Storage->set_data_dir(Storage,"storage");
        if ( (ret= Storage->open(Storage,"storage",DB_CREATE|DB_INIT_LOCK|DB_INIT_LOG|DB_INIT_MPOOL|DB_INIT_TXN,0)) != 0 )
        {
            printf("error.%d opening Storage environment\n",ret);
            exit(ret);
        } else printf("Storage opened\n");
    }
    if ( (ret= db_create(&Public_dbp,Storage,0)) != 0 )
    {
        printf("error.%d creating Public_dbp database\n",ret);
        return(ret);
    } else printf("Public_dbp created\n");
    if ( (ret= db_create(&Private_dbp,Storage,0)) != 0 )
    {
        printf("error.%d creating Private_dbp database\n",ret);
        return(ret);
    } else printf("Private_dbp created\n");
    if ( (ret= Public_dbp->open(Public_dbp,NULL,"public.db",NULL,DB_HASH,DB_CREATE | DB_AUTO_COMMIT,0)) != 0 )
    {
        printf("error.%d opening Public_dbp database\n",ret);
        return(ret);
    } else printf("Public_dbp opened\n");
    if ( (ret= Private_dbp->open(Private_dbp,NULL,"private.db",NULL,DB_HASH,DB_CREATE | DB_AUTO_COMMIT,0)) != 0 )
    {
        printf("error.%d opening Private_dbp database\n",ret);
        return(ret);
    } else printf("Private_dbp opened\n");
    return(0);
}

uint32_t num_in_db(int32_t selector)
{
    return((uint32_t)Storage_maxitems[selector]);
}

DB *get_selected_database(int32_t selector)
{
   return((selector == 0) ? Public_dbp : Private_dbp);
}

void clear_pair(DBT *key,DBT *data)
{
    memset(key,0,sizeof(DBT));
    memset(data,0,sizeof(DBT));
}

struct kademlia_storage *find_storage(int32_t selector,char *keystr)
{
    DB *dbp = get_selected_database(selector);
    uint64_t keybits = calc_nxt64bits(keystr);
    DBT key,data;
    int ret;
    if ( dbp == 0 )
        return(0);
    clear_pair(&key,&data);
    key.data = &keybits;
    key.size = sizeof(keybits);
    data.flags = 0;
    if ( (ret= dbp->get(dbp,NULL,&key,&data,0)) != 0 )
    {
        if ( ret != DB_NOTFOUND )
            printf("DB get error.%d\n",ret);
        else return(0);
    }
    return((struct kademlia_storage *)data.data);
}

struct kademlia_storage *add_storage(int32_t selector,char *keystr,char *datastr,uint8_t *cacheddata,int32_t datalen)
{
    DB *dbp = get_selected_database(selector);
    uint32_t now = (uint32_t)time(NULL);
    int32_t ret,createdflag = 0;
    unsigned char databuf[8192],space[8192];
    DBT key,data;
    DB_TXN *txn = 0;
    struct kademlia_storage *sp;
    if ( dbp == 0 )
        return(0);
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
        if ( sp == 0 )
        {
            Storage_maxitems[selector]++;
            createdflag = 1;
        }
        else memcpy(space,sp,sizeof(*sp));
        sp = (struct kademlia_storage *)space;
        sp->keyhash = calc_nxt64bits(keystr);
        strcpy(sp->key,keystr);
        memcpy(sp->data,databuf,datalen);
        sp->datalen = datalen;
        //printf("store datalen.%d\n",datalen);
        //if ( (ret= Storage->txn_begin(Storage,NULL,&txn,0)) == 0 )
        {
            clear_pair(&key,&data);
            key.data = &sp->keyhash;
            key.size = sizeof(sp->keyhash);
            data.data = sp;
            data.size = (sizeof(*sp) + datalen);
            if ( (ret= dbp->put(dbp,txn,&key,&data,0)) != 0 )
            {
                Storage->err(Storage,ret,"Database put failed.");
                txn->abort(txn);
            }
            else
            {
                //if ( (ret= txn->commit(txn,0)) != 0 )
                //    Storage->err(Storage,ret,"Transaction commit failed.");
                //else
                {
                    dbp->sync(dbp,0);
                    printf("created.%d DB entry for %s\n",createdflag,keystr);
                    if ( (sp= find_storage(selector,keystr)) != 0 )
                    {
                        sp->laststored = now;
                        if ( createdflag != 0 )
                            sp->createtime = now;
                        if ( memcmp(sp->data,databuf,datalen) != 0 )
                            printf("data cmp error\n");
                    } else printf("couldnt find sp in DB that was just added\n");
                }
            }
        } //else Storage->err(Storage,ret,"Transaction begin failed.");
    }
    if ( sp != 0 )
        sp->lastaccess = now;
    return(sp);
}

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
    DB *dbp = get_selected_database(selector);
    struct kademlia_storage *sp,**sps = 0;
    int32_t ret,dist,refdist,n = 0;
    DBT key,data;
    DBC *cursorp = 0;
    if ( dbp == 0 )
        return(0);
    printf("find_closer_Kstored max.%d\n",num_in_db(selector));
    dbp->cursor(dbp,NULL,&cursorp,0);
    if ( cursorp != 0 )
    {
        clear_pair(&key,&data);
        sps = (struct kademlia_storage **)calloc(sizeof(*sps),num_in_db(selector)+1);
        while ( (ret= cursorp->get(cursorp,&key,&data,DB_NEXT)) == 0 )
        {
            sp = data.data;
            refdist = bitweight(refbits ^ sp->keyhash);
            dist = bitweight(newbits ^ sp->keyhash);
            if ( dist < refdist )
                sps[n++] = sp;
            clear_pair(&key,&data);
        }
        cursorp->close(cursorp);
    }
    printf("find_closer_Kstored returns n.%d\n",n);
    return(sps);
}

int32_t kademlia_pushstore(int32_t selector,uint64_t refbits,uint64_t newbits)
{
    int32_t n = 0;
    struct storage_queue_entry *ptr;
    struct kademlia_storage **sps,*sp;
    fprintf(stderr,"pushstore\n");
    if ( (sps= find_closer_Kstored(selector,refbits,newbits)) != 0 )
    {
        while ( (sp= sps[n++]) != 0 )
        {
            ptr = calloc(1,sizeof(*ptr));
            ptr->destbits = newbits;
            ptr->selector = selector;
            ptr->keyhash = sp->keyhash;
            printf("%p queue.%d to %llu\n",ptr,n,(long long)newbits);
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
    uint64_t txid = 0;
    struct kademlia_storage *sp;
    struct coin_info *cp = get_coin_info("BTCD");
    if ( (ptr= queue_dequeue(&storageQ)) != 0 )
    {
        fprintf(stderr,"dequeue StorageQ %p key.(%llu) dest.(%llu) selector.%d\n",ptr,(long long)ptr->keyhash,(long long)ptr->destbits,ptr->selector);
        expand_nxt64bits(key,ptr->keyhash);
        if ( (sp= find_storage(ptr->selector,key)) != 0 )
        {
            init_hexbytes_noT(datastr,sp->data,sp->datalen);
            printf("dequeued storageQ %p: (%s) len.%d\n",ptr,datastr,sp->datalen);
            txid = send_kademlia_cmd(ptr->destbits,0,"store",cp->srvNXTACCTSECRET,key,datastr);
            if ( Debuglevel > 0 )
                printf("txid.%llu send queued push storage key.(%s) to %llu\n",(long long)txid,key,(long long)ptr->destbits);
        }
        free(ptr);
    }
    return(txid);
}
#endif
