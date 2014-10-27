//
//  storage.h
//  libjl777
//
//  Created by jl777 on 10/19/14.
//  Copyright (c) 2014 jl777. MIT license
//

#ifndef libjl777_storage_h
#define libjl777_storage_h

#ifdef __APPLE__
#include "db.h"
#else
#include "db.h"
#endif

#define MAX_KADEMLIA_STORAGE (1024L * 1024L * 1024L)
#define PUBLIC_DATA 0
#define PRIVATE_DATA 1
#define TELEPOD_DATA 2
#define PRICE_DATA 3
#define DEADDROP_DATA 4

struct storage_header
{
    uint64_t modified,keyhash;
    uint32_t datalen,laststored,lastaccess,createtime;
};

struct kademlia_storage
{
    struct storage_header H;
    unsigned char data[];
};
long Total_stored,Storage_maxitems[16];
DB_ENV *Storage;
DB *Public_dbp,*Private_dbp,*Telepod_dbp,*Prices_dbp,*Deaddrops_dbp;

struct storage_queue_entry { uint64_t keyhash,destbits; int32_t selector; };
int db_setup(const char *home,const char *data_dir,FILE *errfp,const char *progname);

DB *open_database(char *fname,int32_t type,int32_t flags)
{
    int ret;
    DB *dbp;
    if ( (ret= db_create(&dbp,Storage,0)) != 0 )
    {
        printf("error.%d creating %s database\n",ret,fname);
        return(0);
    } else printf("Public_dbp created\n");
    if ( (ret= dbp->open(dbp,NULL,fname,NULL,type,flags,0)) != 0 )
    {
        printf("error.%d opening %s database\n",ret,fname);
        return(0);
    } else printf("%s opened\n",fname);
    return(dbp);
}

int32_t init_SuperNET_storage()
{
    static int didinit;
    int ret;
    if ( IS_LIBTEST == 0 )
        return(0);
    if ( didinit == 0 )
    {
        didinit = 1;
        if ( (ret = db_env_create(&Storage, 0)) != 0 )
        {
            fprintf(stderr,"Error creating environment handle: %s\n",db_strerror(ret));
            return(-1);
        }
        else if ( (ret= Storage->open(Storage,"storage",DB_CREATE|DB_INIT_LOCK|DB_INIT_LOG|DB_INIT_MPOOL|DB_INIT_TXN,0)) != 0 )
        {
            fprintf(stderr,"error.%d opening storage\n",ret);
            return(-2);
        }
        else
        {
            Public_dbp = open_database("public.db",DB_HASH,DB_CREATE | DB_AUTO_COMMIT);
            Private_dbp = open_database("private.db",DB_HASH,DB_CREATE | DB_AUTO_COMMIT);
            Telepod_dbp = open_database("telepods.db",DB_HASH,DB_CREATE | DB_AUTO_COMMIT);
            Prices_dbp = open_database("prices.db",DB_BTREE,DB_CREATE | DB_AUTO_COMMIT);
            Deaddrops_dbp = open_database("deaddrops.db",DB_HASH,DB_CREATE | DB_AUTO_COMMIT);
        }
    }
    return(0);
}

uint32_t num_in_db(int32_t selector)
{
    return((uint32_t)Storage_maxitems[selector]);
}

void set_num_in_db(int32_t selector,int32_t num)
{
    Storage_maxitems[selector] = num;
}

DB *get_selected_database(int32_t selector)
{
    switch ( selector )
    {
        case PUBLIC_DATA: return(Public_dbp);
        case PRIVATE_DATA: return(Private_dbp);
        case TELEPOD_DATA: return(Telepod_dbp);
        case PRICE_DATA: return(Prices_dbp);
        case DEADDROP_DATA: return(Deaddrops_dbp);
    }
    return(0);
}

void clear_pair(DBT *key,DBT *data)
{
    memset(key,0,sizeof(DBT));
    memset(data,0,sizeof(DBT));
}

struct storage_header *find_storage(int32_t selector,char *keystr)
{
    DB *dbp = get_selected_database(selector);
    DBT key,data;
    int ret;
    void *ptr = 0;
    struct storage_header *hp;
    if ( dbp == 0 )
        return(0);
    clear_pair(&key,&data);
    key.data = keystr;
    key.size = (int32_t)strlen(keystr) + 1;
    data.flags = 0;
    if ( (ret= dbp->get(dbp,NULL,&key,&data,0)) != 0 )
    {
        if ( ret != DB_NOTFOUND )
            printf("DB get error.%d\n",ret);
        else return(0);
    }
    hp = (struct storage_header *)data.data;
    ptr = malloc(data.size);
    memcpy(ptr,hp,data.size);
    return(ptr);
}

void add_storage(int32_t selector,char *keystr,char *datastr)
{
    DB *dbp = get_selected_database(selector);
    uint32_t now = (uint32_t)time(NULL);
    int32_t datalen,slen,ret,createdflag = 0;
    unsigned char databuf[8192],space[8192];
    uint64_t hashval = 0;
    DBT key,data;
    DB_TXN *txn = 0;
    struct kademlia_storage *sp;
    if ( dbp == 0 )
        return;
    if ( selector == PUBLIC_DATA && Total_stored > MAX_KADEMLIA_STORAGE )
    {
        printf("Total_stored %s > %s\n",_mbstr(Total_stored),_mbstr2(MAX_KADEMLIA_STORAGE));
        return;
    }
    datalen = (int32_t)strlen(datastr) / 2;
    if ( datalen > sizeof(databuf) )
        return;
    decode_hex(databuf,datalen,datastr);
    if ( (sp= (struct kademlia_storage *)find_storage(selector,keystr)) == 0 || sp->H.datalen != datalen || memcmp(sp->data,databuf,datalen) != 0 )
    {
        slen = (int32_t)strlen(keystr);
        if ( sp == 0 )
        {
            Storage_maxitems[selector]++;
            if ( selector == PUBLIC_DATA )
                Total_stored += (sizeof(*sp) + datalen);
            createdflag = 1;
            if ( is_decimalstr(keystr) && slen < MAX_NXTADDR_LEN )
                hashval = calc_nxt64bits(keystr);
            else hashval = calc_txid((uint8_t *)keystr,slen);
        }
        else
        {
            hashval = sp->H.keyhash;
            memcpy(space,sp,sizeof(*sp));
            free(sp);
        }
        sp = (struct kademlia_storage *)space;
        if ( createdflag != 0 )
            sp->H.keyhash = hashval;
        else if ( sp->H.keyhash != hashval )
            printf("ERROR: keyhash.%llu != hashval.%llu (%s)\n",(long long)sp->H.keyhash,(long long)hashval,keystr);
        memcpy(sp->data,databuf,datalen);
        sp->H.datalen = datalen;
        //printf("store datalen.%d\n",datalen);
        //if ( (ret= Storage->txn_begin(Storage,NULL,&txn,0)) == 0 )
        {
            clear_pair(&key,&data);
            key.data = keystr;
            key.size = slen + 1;
            sp->H.laststored = now;
            if ( createdflag != 0 )
                sp->H.createtime = now;
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
                    printf("created.%d DB entry for %s\n",createdflag,keystr);
                    if ( (sp= (struct kademlia_storage *)find_storage(selector,keystr)) != 0 )
                    {
                        if ( memcmp(sp->data,databuf,datalen) != 0 )
                            printf("data cmp error\n");
                        free(sp);
                    } else printf("couldnt find sp in DB that was just added\n");
                    dbp->sync(dbp,0);
                }
            }
        } //else Storage->err(Storage,ret,"Transaction begin failed.");
    }
}

void update_storage(int32_t selector,char *keystr,struct storage_header *hp)
{
    DB *dbp = get_selected_database(selector);
    DBT key,data;
    int ret;
    if ( dbp != 0 )
    {
        clear_pair(&key,&data);
        key.data = keystr;
        key.size = (uint32_t)strlen(keystr) + 1;
        hp->laststored = (uint32_t)time(NULL);
        data.data = hp;
        data.size = hp->datalen;
        if ( (ret= dbp->put(dbp,0,&key,&data,0)) != 0 )
            Storage->err(Storage,ret,"Database put failed.");
    }
}

struct kademlia_storage *kademlia_getstored(int32_t selector,uint64_t keyhash,char *datastr)
{
    uint32_t now;
    char key[64];
    struct kademlia_storage *sp;
    expand_nxt64bits(key,keyhash);
    now = (uint32_t)time(NULL);
    sp = (struct kademlia_storage *)find_storage(selector,key);
    if ( datastr == 0 )
        return(sp);
    if ( sp != 0 )
        free(sp);
    add_storage(selector,key,datastr);
    return((struct kademlia_storage *)find_storage(selector,key));
}

uint64_t *find_closer_Kstored(int32_t selector,uint64_t refbits,uint64_t newbits)
{
    DB *dbp = get_selected_database(selector);
    struct kademlia_storage *sp;
    int32_t ret,dist,max,m,refdist,n = 0;
    DBT key,data;
    uint64_t *keys = 0;
    DBC *cursorp = 0;
    if ( dbp == 0 )
        return(0);
    max = num_in_db(selector);
    //printf("find_closer_Kstored max.%d\n",max);
    max += 100;
    m = 0;
    dbp->cursor(dbp,NULL,&cursorp,0);
    if ( cursorp != 0 )
    {
        clear_pair(&key,&data);
        keys = (uint64_t *)calloc(sizeof(*keys),max);
        while ( (ret= cursorp->get(cursorp,&key,&data,DB_NEXT)) == 0 )
        {
            m++;
            sp = data.data;
            refdist = bitweight(refbits ^ sp->H.keyhash);
            dist = bitweight(newbits ^ sp->H.keyhash);
            if ( dist < refdist )
            {
                keys[n++] = sp->H.keyhash;
                if ( n >= max )
                {
                    max += 100;
                    keys = (uint64_t *)realloc(keys,sizeof(*keys)*max);
                }
            }
            clear_pair(&key,&data);
        }
        cursorp->close(cursorp);
    }
    //printf("find_closer_Kstored returns n.%d %p\n",n,sps);
    if ( m > num_in_db(selector) )
        set_num_in_db(selector,m);
    return(keys);
}

int32_t kademlia_pushstore(int32_t selector,uint64_t refbits,uint64_t newbits)
{
    int32_t n = 0;
    uint64_t *keys,key;
    struct storage_queue_entry *ptr;
    //fprintf(stderr,"pushstore\n");
    if ( (keys= find_closer_Kstored(selector,refbits,newbits)) != 0 )
    {
        while ( (key= keys[n++]) != 0 )
        {
            ptr = calloc(1,sizeof(*ptr));
            ptr->destbits = newbits;
            ptr->selector = selector;
            ptr->keyhash = key;
            //printf("%p queue.%d to %llu\n",ptr,n,(long long)newbits);
            queue_enqueue(&storageQ,ptr);
        }
        //printf("free sps.%p\n",sps);
        free(keys);
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
        //fprintf(stderr,"dequeue StorageQ %p key.(%llu) dest.(%llu) selector.%d\n",ptr,(long long)ptr->keyhash,(long long)ptr->destbits,ptr->selector);
        expand_nxt64bits(key,ptr->keyhash);
        if ( (sp= (struct kademlia_storage *)find_storage(ptr->selector,key)) != 0 )
        {
            init_hexbytes_noT(datastr,sp->data,sp->H.datalen);
            //printf("dequeued storageQ %p: (%s) len.%d\n",ptr,datastr,sp->datalen);
            txid = send_kademlia_cmd(ptr->destbits,0,"store",cp->srvNXTACCTSECRET,key,datastr);
            if ( Debuglevel > 0 )
                printf("txid.%llu send queued push storage key.(%s) to %llu\n",(long long)txid,key,(long long)ptr->destbits);
        }
        free(ptr);
    }
    return(txid);
}
#endif
