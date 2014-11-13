//
//  dbqueue.h
//  xcode
//
//  Created by jl777 on 9/24/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//


#ifndef dbqueue_h
#define dbqueue_h

#include "db.h"

#define MAX_PRICEDBS 1000

struct SuperNET_db
{
    char name[64],**privkeys;
    queue_t queue;
    long maxitems,total_stored;
    DB *dbp;
    int32_t *cipherids,selector;
    uint32_t busy,type,flags,active,minsize,maxsize,duplicateflag;
};

struct dbreq { struct SuperNET_db *sdb; void *cursor; DB_TXN *txn; DBT key,*data; int32_t flags,retval,funcid,doneflag; };
DB_ENV *Storage;
struct SuperNET_db SuperNET_dbs[NUM_SUPERNET_DBS],Price_dbs[MAX_PRICEDBS];
long Total_stored,Num_pricedbs;

struct SuperNET_db *find_pricedb(char *dbname,int32_t createflag)
{
    int32_t i;
    struct SuperNET_db *sdb = 0;
    if ( Num_pricedbs > 0 )
    {
        for (i=0; i<Num_pricedbs; i++)
        {
            sdb = &Price_dbs[i];
            if ( strcmp(sdb->name,dbname) == 0 )
                return(sdb);
        }
    }
    if ( createflag != 0 )
    {
        sdb = &Price_dbs[Num_pricedbs++];
        memset(sdb,0,sizeof(*sdb));
    }
    return(sdb);
}

int32_t valid_SuperNET_db(char *debugstr,int32_t selector)
{
    if ( IS_LIBTEST == 0 || selector < 0 || selector > NUM_SUPERNET_DBS )
    {
        fprintf(stderr,"%s: invalid SuperNET_db selector.%d or DB disabled vi LIBTEST.%d\n",debugstr,selector,IS_LIBTEST);
        return(0);
    }
    return(1);
}

DB *get_selected_database(int32_t selector)
{
    if ( valid_SuperNET_db("get_selected_database",selector) != 0 )
        return(SuperNET_dbs[selector].dbp);
    return(0);
}

long max_in_db(int32_t selector)
{
    if ( valid_SuperNET_db("max_in_db",selector) != 0 )
        return(SuperNET_dbs[selector].maxitems);
    else return(0);
}

void set_max_in_db(int32_t selector,long num)
{
    if ( valid_SuperNET_db("set_max_in_db",selector) != 0 && num > SuperNET_dbs[selector].maxitems )
        SuperNET_dbs[selector].maxitems = num;
}

void *decondition_storage(uint32_t *lenp,struct SuperNET_db *sdb,void *data,uint32_t size)
{
    void *ptr;
    if ( data == 0 || size == 0 )
        return(0);
    if ( sdb->privkeys != 0 && sdb->cipherids != 0 )
    {
        *lenp = size;
        ptr = ciphers_codec(1,sdb->privkeys,sdb->cipherids,data,(int32_t *)lenp);
        if ( *lenp >= sdb->minsize && *lenp <= sdb->maxsize )
            return(ptr);
        if ( ptr != 0 )
            free(ptr);
        //printf("unencrypted entry size.%d\n",*lenp);
    }
    *lenp = size;
    ptr = malloc(size);
    memcpy(ptr,data,size);
    return(ptr);
}

void *condition_storage(uint32_t *lenp,struct SuperNET_db *sdb,void *data,uint32_t size)
{
    *lenp = size;
    //fprintf(stderr,"condition_storage\n");
    if ( data == 0 || size <= 0 )
        return(0);
    if ( sdb->privkeys == 0 || sdb->cipherids == 0 )
        return(data);
    else return(ciphers_codec(0,sdb->privkeys,sdb->cipherids,data,(int32_t *)lenp));
}

void clear_pair(DBT *key,DBT *data)
{
    memset(key,0,sizeof(DBT));
    memset(data,0,sizeof(DBT));
}

// < 0 if a < b, = 0 if a = b, > 0 if a > b
int db_incrdouble(DB *dbp,const DBT *a,const DBT *b,size_t *locp)
{
    int ai, bi;
    locp = NULL;
    memcpy(&ai,a->data,sizeof(double));
    memcpy(&bi,b->data,sizeof(double));
    if ( ai < (bi - SMALLVAL) ) return(-1);
    else if ( ai > (bi + SMALLVAL) ) return(1);
    else return(0);
}

int db_decrdouble(DB *dbp,const DBT *a,const DBT *b,size_t *locp)
{
    int ai, bi;
    locp = NULL;
    memcpy(&ai,a->data,sizeof(double));
    memcpy(&bi,b->data,sizeof(double));
    if ( ai < (bi - SMALLVAL) ) return(1);
    else if ( ai > (bi + SMALLVAL) ) return(-1);
    else return(0);
}

DB *open_database(int32_t selector,struct SuperNET_db *sdb,char *fname,uint32_t type,uint32_t flags,int32_t minsize,int32_t maxsize,int32_t duplicateflag)
{
    int ret;
    if ( valid_SuperNET_db("open_database",selector) == 0 )
    {
        fprintf(stderr,"open_database error illegal selector.%d for (%s)\n",selector,fname);
        return(0);
     }
    if ( (ret= db_create(&sdb->dbp,Storage,0)) != 0 )
    {
        fprintf(stderr,"open_database error.%d creating %s database\n",ret,fname);
        exit(-1);
        return(0);
    } else printf("open_database %s created\n",fname);
    if ( duplicateflag != 0 )
    {
        if ( (ret= sdb->dbp->set_flags(sdb->dbp,DB_DUP)) != 0 )
        {
            fprintf(stderr,"set_flags DB_DUPSORT error.%d %s\n",ret,fname);
            exit(-3);
            return(0);
        } else printf("set_flags DB_DUPSORT %s\n",fname);
        /*if ( (ret= sdb->dbp->set_dup_compare(sdb->dbp,(sortflag > 0) ? db_incrdouble : db_decrdouble)) != 0 )
         {
         fprintf(stderr,"set_dup_compare error.%d %s\n",ret,fname);
         exit(-4);
         return(0);
         } else printf("set_dup_compare %s\n",fname);*/
    }
    if ( (ret= sdb->dbp->open(sdb->dbp,NULL,fname,NULL,type,flags,0)) != 0 )
    {
        fprintf(stderr,"open_database error.%d opening %s database\n",ret,fname);
        exit(-2);
        return(0);
    } else printf("open_database %s opened\n",fname);
    safecopy(sdb->name,fname,sizeof(sdb->name));
    sdb->type = type;
    sdb->flags = flags;
    sdb->minsize = minsize;
    sdb->maxsize = maxsize;
    sdb->duplicateflag = duplicateflag;
    sdb->active = 1;
    sdb->selector = selector;
    return(sdb->dbp);
}

int32_t close_SuperNET_db(struct SuperNET_db *sdb,int32_t selector)
{
    if ( sdb->dbp == 0 )
        return(-1);
    if ( sdb->active > 0 )
    {
        sdb->active = 0;
        while ( sdb->active == 0 )
        {
            fprintf(stderr,".");
            usleep(100000);
            fprintf(stderr," %s selector.%d shutdown\n",sdb->name,selector);
            sdb->active = -1;
        }
    }
    return(0);
}

int32_t _process_dbiter(struct SuperNET_db *sdb)
{
    int32_t n;
    DBT data;
    struct dbreq *req;
    if ( sdb->active <= 0 )
        return(0);
    n = 0;
    while ( (req= queue_dequeue(&sdb->queue)) != 0 )
    {
        memset(&data,0,sizeof(data));
        if ( req->data != 0 )
            data = *req->data;
        //fprintf(stderr,"%s func.%c key.(%s)\n",sdb->name,req->funcid,req->key.data);
        switch ( req->funcid )
        {
            case 'G': req->retval = sdb->dbp->get(sdb->dbp,req->txn,&req->key,&data,req->flags); break;
            case 'P': req->retval = sdb->dbp->put(sdb->dbp,req->txn,&req->key,&data,req->flags); break;
            case 'S': req->retval = sdb->dbp->sync(sdb->dbp,req->flags); break;
            case 'D': req->retval = sdb->dbp->del(sdb->dbp,req->txn,&req->key,req->flags); break;
            case 'O': req->retval = sdb->dbp->cursor(sdb->dbp,req->txn,req->cursor,req->flags); break;
            case 'C': req->retval = ((DBC *)req->cursor)->close(req->cursor); break;
            case 'g': req->retval = ((DBC *)req->cursor)->get(req->cursor,&req->key,&data,req->flags); break;
            case 'p': req->retval = ((DBC *)req->cursor)->put(req->cursor,&req->key,&data,req->flags); break;
            default:
                printf("UNEXPECTED SuperNET_db funcid.(%c) %d\n",req->funcid,req->funcid);
                break;
        }
        if ( req->data != 0 )
            *req->data = data;
        req->doneflag = 1;
        n++;
    }
    return(n);
}

void *_process_SuperNET_dbqueue(void *unused) // serialize dbreq functions
{
    int32_t i,n,selector,numactive = 1;
    struct SuperNET_db *sdb;
    while ( numactive > 0 )
    {
        n = numactive = 0;
        for (selector=0; selector<NUM_SUPERNET_DBS; selector++)
        {
            sdb = &SuperNET_dbs[selector];
            //fprintf(stderr,"(%d %d) ",selector,sdb->active);
            if ( sdb->active > 0 )
            {
                numactive++;
                n += _process_dbiter(sdb);
            }
        }
        //fprintf(stderr,"n.%d numactive.%d\n",n,numactive);
        if ( n == 0 )
        {
            for (i=0; i<Num_pricedbs; i++)
                n += _process_dbiter(&Price_dbs[i]);
            if ( n == 0 )
                usleep(5000);
        }
    }
    for (selector=0; selector<NUM_SUPERNET_DBS; selector++)
    {
        sdb = &SuperNET_dbs[selector];
        sdb->dbp->close(sdb->dbp,0);
        sdb->active = -1;
        fprintf(stderr,"finished processing process_SuperNET_dbqueue.%d\n",selector);
    }
    return(0);
}

int32_t _block_on_dbreq(struct dbreq *req)
{
    struct SuperNET_db *sdb = req->sdb;//&SuperNET_dbs[req->selector];
    int32_t retval,busy;
    while ( req->doneflag == 0 )
        usleep(5000); // if not done after the first context switch, likely to take a while
    retval = req->retval;
    free(req);
    sdb->busy--;
    if ( (busy= sdb->busy) != 0 ) // busy is not critical for data integrity, but helps with dbreq latency
    {
        fprintf(stderr,"_block_on_dbreq: unlikely case of busy.%d != 0, (%d) for (%s)\n",busy,sdb->busy,sdb->name);
        sdb->busy = 0;
    }
    return(retval);
}

struct dbreq *_queue_dbreq(int32_t funcid,struct SuperNET_db *sdb,DB_TXN *txn,DBT *key,DBT *data,int32_t flags,void *cursor)
{
    struct dbreq *req = 0;
    if ( sdb->active > 0 )
    {
        req = calloc(1,sizeof(*req));
        req->funcid = funcid;
        req->sdb = sdb;
        req->txn = txn;
        if ( key != 0 )
            req->key = *key;
        req->data = data;
        req->flags = flags;
        req->cursor = cursor;
        sdb->busy++;
        queue_enqueue(&sdb->queue,req);
        usleep(10); // allow context switch so request has a chance of completing
    }
    return(req);
}

int32_t dbcmd(char *debugstr,int32_t funcid,struct SuperNET_db *sdb,DB_TXN *txn,DBT *key,DBT *data,int32_t flags,void *cursor)
{
    struct dbreq *req;
    if ( sdb->active > 0 )
    {
        if ( (req= _queue_dbreq(funcid,sdb,txn,key,data,flags,cursor)) != 0 )//&& sdb->selector != NUM_SUPERNET_DBS )
            return(_block_on_dbreq(req));
    }
    return(-1);
    //return(sdb->selector == NUM_SUPERNET_DBS ? 0 : -1);
}

int32_t dbget(struct SuperNET_db *sdb,DB_TXN *txn,DBT *key,DBT *data,int32_t flags)
{
    return(dbcmd("dbget",'G',sdb,txn,key,data,flags,0));
}

int32_t dbput(struct SuperNET_db *sdb,DB_TXN *txn,DBT *key,DBT *data,int32_t flags)
{
    //return(sdb->dbp->put(sdb->dbp,txn,key,data,flags));
    return(dbcmd("dbput",'P',sdb,txn,key,data,flags,0));
}

int32_t dbdel(struct SuperNET_db *sdb,DB_TXN *txn,DBT *key,DBT *data,int32_t flags)
{
    return(dbcmd("dbdel",'D',sdb,txn,key,data,flags,0));
}

int32_t dbsync(struct SuperNET_db *sdb,int32_t flags)
{
    return(dbcmd("dbsync",'S',sdb,0,0,0,flags,0));
}

int32_t dbcursor(struct SuperNET_db *sdb,DB_TXN *txn,DBC **cursor,int32_t flags)
{
    return(dbcmd("dbcursor",'O',sdb,txn,0,0,flags,cursor));
}

int32_t cursorget(struct SuperNET_db *sdb,DB_TXN *txn,DBC *cursor,DBT *key,DBT *data,int32_t flags)
{
    return(dbcmd("cursorget",'g',sdb,txn,key,data,flags,cursor));
}

int32_t cursorput(struct SuperNET_db *sdb,DB_TXN *txn,DBC *cursor,DBT *key,DBT *data,int32_t flags)
{
    return(dbcmd("cursorput",'p',sdb,txn,key,data,flags,cursor));
}

int32_t cursorclose(struct SuperNET_db *sdb,DBC *cursor)
{
    return(dbcmd("cursorclose",'C',sdb,0,0,0,0,cursor));
}

#endif
