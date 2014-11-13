//
//  storage.h
//  libjl777
//
//  Created by jl777 on 10/19/14.
//  Copyright (c) 2014 jl777. MIT license
//

#ifndef libjl777_storage_h
#define libjl777_storage_h

// all functions that issue dbp-> commands should be in this file

#define MAX_KADEMLIA_STORAGE (1024L * 1024L * 1024L)


int dbreplace_iQ(int32_t selector,char *keystr,struct InstantDEX_quote *refiQ)
{
    struct SuperNET_db *sdb;
    struct InstantDEX_quote *iQ;
    int32_t n,replaced=0,ret = -1;
    DB_TXN *txn = NULL;
    DBC *cursorp = 0;
    DBT key,data;
    if ( get_selected_database(selector) == 0 )
        return(0);
    sdb = &SuperNET_dbs[selector];
    //fprintf(stderr,"dbreplace_iQ.%d\n",selector);
    if ( (ret = Storage->txn_begin(Storage,NULL,&txn,0)) != 0 )
    {
        Storage->err(Storage,ret,"Transaction begin failed.");
        return(-1);
    }
    n = 0;
    clear_pair(&key,&data);
    key.data = keystr;
    key.size = (int32_t)(strlen(keystr) + 1);
    if ( (ret= dbcursor(sdb,txn,&cursorp,0)) != 0 )
    {
        Storage->err(Storage,ret,"Cursor open failed.");
        txn->abort(txn);
        return(-1);
    }
    if ( cursorp != 0 )
    {
        //ret = cursorp->get(cursorp,&key,&data,DB_SET);
        ret = cursorget(sdb,txn,cursorp,&key,&data,DB_SET);
        while ( ret == 0 )
        {
            iQ = data.data;
            printf("%p key.%d: %s, data.size %d: %llu %u %u | vs %llu %u %u\n",iQ,n,(char *)key.data,data.size,(long long)iQ->nxt64bits,iQ->timestamp,iQ->type,(long long)refiQ->nxt64bits,refiQ->timestamp,refiQ->type);
            if ( iQ->nxt64bits == refiQ->nxt64bits && iQ->type == refiQ->type && refiQ->timestamp > iQ->timestamp )
            {
                clear_pair(&key,&data);
                key.data = keystr;
                key.size = (int32_t)(strlen(keystr) + 1);
                data.data = refiQ;
                data.size = sizeof(*refiQ);
                //ret = cursorp->put(cursorp,&key,&data,DB_CURRENT);
                ret = cursorput(sdb,txn,cursorp,&key,&data,DB_CURRENT);
                replaced = 1;
                break;
            }
            //ret = cursorp->get(cursorp,&key,&data,DB_NEXT_DUP);
            ret = cursorget(sdb,txn,cursorp,&key,&data,DB_NEXT_DUP);
            n++;
        }
        //printf("close cursor\n");
        if ( (ret= cursorclose(sdb,cursorp)) != 0 )
        {
            Storage->err(Storage,ret,"Cursor close failed.");
            txn->abort(txn);
        }
        else if ( (ret= txn->commit(txn,0)) != 0 )
            Storage->err(Storage,ret,"Transaction commit failed.");
        if ( replaced == 0 )
        {
            //printf("call dbput\n");
            data.data = refiQ;
            data.size = sizeof(*refiQ);
            ret = dbput(sdb,0,&key,&data,0);
        }
        //printf("queue sync\n");
        dbsync(sdb,0);
    }
    //fprintf(stderr,"done dbreplace_iQ.%d\n",selector);
    return(ret);
}

struct storage_header **copy_all_DBentries(int32_t *nump,int32_t selector)
{
    struct storage_header *ptr,**ptrs = 0;
    struct SuperNET_db *sdb;
    int32_t ret,max,m,n = 0;
    DBT key,data;
    DBC *cursorp = 0;
    *nump = 0;
    if ( get_selected_database(selector) == 0 )
        return(0);
    sdb = &SuperNET_dbs[selector];
    max = (int32_t)max_in_db(selector);
    max += 100;
    m = 0;
    if ( (ret= dbcursor(sdb,NULL,&cursorp,0)) != 0 )
    {
        Storage->err(Storage,ret,"copy_all_DBentries: Cursor open failed.");
        return(0);
    }
    if ( cursorp != 0 )
    {
        clear_pair(&key,&data);
        ptrs = (struct storage_header **)calloc(sizeof(struct storage_header *),max+1);
        while ( (ret= cursorget(sdb,NULL,cursorp,&key,&data,DB_NEXT)) == 0 )
        {
            m++;
            ptr = decondition_storage(&data.size,sdb,data.data,data.size);
            ptrs[n++] = ptr;
            if ( n >= max )
            {
                max += 100;
                ptrs = (struct storage_header **)realloc(ptrs,sizeof(struct storage_header *)*(max+1));
            }
            clear_pair(&key,&data);
        }
        cursorclose(sdb,cursorp);
    }
    //fprintf(stderr,"done copy all dB.%d\n",selector);
    if ( m > max_in_db(selector) )
        set_max_in_db(selector,m);
    if ( ptrs != 0 )
        ptrs[n] = 0;
    *nump = n;
    return(ptrs);
}

struct exchange_quote *get_exchange_quotes(int32_t *nump,char *dbname,int32_t oldest)
{
    struct exchange_quote Q,*quotes = 0;
    struct SuperNET_db *sdb;
    int32_t ret,max,m,n = 0;
    DBT key,data;
    DBC *cursorp = 0;
    *nump = 0;
    if ( (sdb= find_pricedb(dbname,0)) == 0 )
        return(0);
    max = 8192;
    m = 0;
    if ( (ret= dbcursor(sdb,NULL,&cursorp,0)) != 0 )
    {
        Storage->err(Storage,ret,"get_allquotes: Cursor open failed.");
        return(0);
    }
    if ( cursorp != 0 )
    {
        clear_pair(&key,&data);
        quotes = (struct exchange_quote *)calloc(sizeof(Q),max+1);
        while ( (ret= cursorget(sdb,NULL,cursorp,&key,&data,DB_NEXT)) == 0 )
        {
            m++;
            memset(&Q,0,sizeof(Q));
            if ( data.data != 0 && data.size == sizeof(Q) )
            {
                memcpy(&Q,data.data,sizeof(Q));
                //printf("m.%d [%.8f %.8f] %u\n",m,Q.highbid,Q.lowask,Q.timestamp);
                if ( Q.timestamp > oldest )
                {
                    memcpy(&quotes[n++],&Q,sizeof(Q));
                    if ( n >= max )
                    {
                        max += 8192;
                        quotes = (struct exchange_quote *)realloc(quotes,sizeof(Q)*(max+1));
                    }
                }
            }
            clear_pair(&key,&data);
        }
        cursorclose(sdb,cursorp);
    }
    *nump = n;
    if ( quotes != 0 )
        memset(&quotes[n],0,sizeof(Q));
    return(quotes);
}

void save_pricequote(struct SuperNET_db *sdb,struct exchange_quote *qp)
{
    DBT key,data;
    int32_t ret;
    //DB *dbp;
    clear_pair(&key,&data);
    key.data = &qp->timestamp;
    key.size = sizeof(qp->timestamp);
    data.data = qp;
    data.size = sizeof(*qp);
    //dbp = sdb->dbp;
    //if ( (ret= dbp->put(dbp,0,&key,&data,0)) != 0 )
    if ( (ret= dbput(sdb,0,&key,&data,0)) != 0 )
        Storage->err(Storage,ret,"Database put for quote failed.");
    else dbsync(sdb,0);
}

void set_dbname(char *dbname,char *exchange,char *base,char *rel)
{
    char lexchange[512],lbase[512],lrel[512];
    strcpy(lexchange,exchange), tolowercase(lexchange);
    strcpy(lbase,base), tolowercase(lbase);
    strcpy(lrel,rel), tolowercase(lrel);
    sprintf(dbname,"%s.%s_%s",lexchange,lbase,lrel);
}

struct storage_header *find_storage(int32_t selector,char *keystr,uint32_t bulksize)
{
    void *ptr = 0;
    DBT key,data,*retdata;
    int32_t ret,reqflags = 0;
    struct storage_header *hp;
    if ( valid_SuperNET_db("find_storage",selector) == 0 )
        return(0);
    //fprintf(stderr,"in find_storage.%d %s\n",selector,keystr);
    clear_pair(&key,&data);
    key.data = (keystr);
    key.size = (int32_t)strlen(keystr) + 1;
    if ( bulksize != 0 )
    {
        reqflags = DB_MULTIPLE;
        data.ulen = bulksize;
        data.flags = DB_DBT_USERMEM;
        data.data = ptr = valloc(data.ulen);
    }
    if ( (ret= dbget(&SuperNET_dbs[selector],NULL,&key,&data,reqflags)) != 0 || data.data == 0 || data.size < sizeof(*hp) )
    {
        if ( ret != DB_NOTFOUND )
            fprintf(stderr,"DB.%d get error.%d data.size %d\n",selector,ret,data.size);
        else
        {
            if ( ptr != 0 )
                free(ptr);
            //fprintf(stderr,"find_storage.%d %s not found\n",selector,keystr);
            return(0);
        }
    }
    //fprintf(stderr,"find_storage.%d %s found\n",selector,keystr);
    if ( bulksize != 0 )
    {
        retdata = calloc(1,sizeof(*retdata));
        *retdata = data;
        return((void *)retdata);
    }
    else return(decondition_storage(&data.size,&SuperNET_dbs[selector],data.data,data.size));
}

int32_t complete_dbput(struct SuperNET_db *sdb,char *keystr,void *databuf,int32_t datalen,int32_t bulksize)
{
    struct SuperNET_storage *sp;
    if ( (sp= (struct SuperNET_storage *)find_storage(sdb->selector,keystr,bulksize)) != 0 )
    {
        if ( memcmp(sp,databuf,datalen) != 0 )
            fprintf(stderr,"(%s) data.%s cmp error datalen.%d\n",keystr,sdb->name,datalen);
        //else fprintf(stderr,"DB.%d (%s) %d verified\n",selector,keystr,datalen);
        free(sp);
    } else { fprintf(stderr,"couldnt find sp in DB.%s that was just added.(%s)\n",sdb->name,keystr); return(-1); }
    return(dbsync(sdb,0));
}

void add_pricedb(char *exchange,char *base,char *rel)
{
    char dbname[MAX_JSON_FIELD];
    struct exchange_pair P;
    DBT key,data;
    int ret;
    set_dbname(dbname,exchange,base,rel);
    clear_pair(&key,&data);
    memset(&P,0,sizeof(P));
    strcpy(P.exchange,exchange), tolowercase(P.exchange);
    strcpy(P.base,base), tolowercase(P.base);
    strcpy(P.rel,rel), tolowercase(P.rel);
    P.H.size = sizeof(P);
    key.data = dbname;
    key.size = (uint32_t)strlen(dbname) + 1;
    data.data = &P;
    data.size = sizeof(P);
    if ( (ret= dbput(&SuperNET_dbs[PRICE_DATA],0,&key,&data,0)) != 0 )
        Storage->err(Storage,ret,"Database put failed.");
    else if ( complete_dbput(&SuperNET_dbs[PRICE_DATA],dbname,&P,sizeof(P),0) == 0 && Debuglevel > 1 )
        fprintf(stderr,"updated.(%s)\n",dbname);
}

int32_t delete_storage(struct SuperNET_db *sdb,char *keystr)
{
    //if ( valid_SuperNET_db("delete_storage",selector) == 0 )
    //    return(-1);
    DBT key;
    int32_t ret;
    memset(&key,0,sizeof(key));
    key.data = keystr;
    key.size = (int32_t)strlen(keystr) + 1;
    fprintf(stderr,"delete_storage.%d\n",sdb->selector);
    if ( (ret= dbdel(sdb,0,&key,0,0)) != 0 )
        fprintf(stderr,"error deleting (%s) from DB.%d\n",keystr,sdb->selector);
    else return(dbsync(sdb,0));
    return(-1);
}

void update_storage(struct SuperNET_db *sdb,char *keystr,struct storage_header *hp)
{
    DBT key,data;
    int ret;
    if ( hp->size == 0 )
    {
        printf("update_storage.%s zero datalen for (%s)\n",sdb->name,keystr);
        return;
    }
    //if ( valid_SuperNET_db("update_storage",selector) != 0 )
    {
        clear_pair(&key,&data);
        key.data = (keystr);
        key.size = (uint32_t)strlen(keystr) + 1;
        if ( hp->createtime == 0 )
        {
            sdb->maxitems++;
            hp->createtime = (uint32_t)time(NULL);
        }
        data.data = condition_storage(&data.size,sdb,hp,hp->size);
        //fprintf(stderr,"updateDB.%d entry.(%s) datalen.%d -> %d | hp %p, data.data %p\n",selector,keystr,hp->size,data.size,hp,data.data);
        if ( (ret= dbput(sdb,0,&key,&data,0)) != 0 )
            Storage->err(Storage,ret,"Database put failed.");
        else if ( complete_dbput(sdb,keystr,hp,hp->size,0) == 0 && Debuglevel > 1 )
            fprintf(stderr,"updated.%s (%s) hp.%p data.data %p\n",sdb->name,keystr,hp,data.data);
        if ( data.data != hp && data.data != 0 )
            free(data.data);
    }
}

void add_storage(int32_t selector,char *keystr,char *datastr)
{
    int32_t datalen,slen,createdflag = 0;
    unsigned char databuf[8192],space[8192];
    uint64_t hashval = 0;
    struct SuperNET_db *sdb;
    struct SuperNET_storage *sp;
    if ( valid_SuperNET_db("add_storage",selector) == 0 )
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
    sdb = &SuperNET_dbs[selector];
    //fprintf(stderr,"add_storage.%d\n",selector);
    if ( (sp= (struct SuperNET_storage *)find_storage(sdb->selector,keystr,0)) == 0 || (sp->H.size-sizeof(*sp)) != datalen || memcmp(sp->data,databuf,datalen) != 0 )
    {
        slen = (int32_t)strlen(keystr);
        if ( sp == 0 )
        {
            SuperNET_dbs[selector].maxitems++;
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
        sp = (struct SuperNET_storage *)space;
        if ( createdflag != 0 )
            sp->H.keyhash = hashval;
        else if ( sp->H.keyhash != hashval )
            fprintf(stderr,"ERROR: keyhash.%llu != hashval.%llu (%s)\n",(long long)sp->H.keyhash,(long long)hashval,keystr);
        memcpy(sp->data,databuf,datalen);
        sp->H.size = (sizeof(*sp) + datalen);
        update_storage(sdb,keystr,&sp->H);
    } else fprintf(stderr,"(%s) <- (%s) already there\n",keystr,datastr);
}

int32_t init_SuperNET_storage()
{
    static int didinit;
    int ret;
    struct coin_info *cp = get_coin_info("BTCD");
    struct SuperNET_db *sdb;
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
        else if ( (ret= Storage->open(Storage,"storage",DB_CREATE|DB_INIT_LOG|DB_INIT_MPOOL|DB_INIT_TXN,0)) != 0 ) //
        {
            fprintf(stderr,"error.%d opening storage\n",ret);
            return(-2);
        }
        else
        {
            open_database(PUBLIC_DATA,&SuperNET_dbs[PUBLIC_DATA],"public.db",DB_HASH,DB_CREATE | DB_AUTO_COMMIT,sizeof(struct storage_header),4096,0);
            open_database(PRIVATE_DATA,&SuperNET_dbs[PRIVATE_DATA],"private.db",DB_HASH,DB_CREATE | DB_AUTO_COMMIT,sizeof(struct storage_header),4096,0);
            open_database(TELEPOD_DATA,&SuperNET_dbs[TELEPOD_DATA],"telepods.db",DB_HASH,DB_CREATE | DB_AUTO_COMMIT,sizeof(struct storage_header),4096,0);
            open_database(DEADDROP_DATA,&SuperNET_dbs[DEADDROP_DATA],"deaddrops.db",DB_HASH,DB_CREATE | DB_AUTO_COMMIT,sizeof(struct storage_header),4096,0);
            open_database(CONTACT_DATA,&SuperNET_dbs[CONTACT_DATA],"contacts.db",DB_HASH,DB_CREATE | DB_AUTO_COMMIT,sizeof(struct contact_info),sizeof(struct contact_info),0);
            open_database(NODESTATS_DATA,&SuperNET_dbs[NODESTATS_DATA],"nodestats.db",DB_HASH,DB_CREATE | DB_AUTO_COMMIT,sizeof(struct nodestats),sizeof(struct nodestats),0);
            open_database(INSTANTDEX_DATA,&SuperNET_dbs[INSTANTDEX_DATA],"InstantDEX.db",DB_HASH,DB_CREATE | DB_AUTO_COMMIT,sizeof(struct InstantDEX_quote),sizeof(struct InstantDEX_quote),1);
            open_database(PRICE_DATA,&SuperNET_dbs[PRICE_DATA],"prices.db",DB_HASH,DB_CREATE | DB_AUTO_COMMIT,sizeof(struct exchange_pair),sizeof(struct exchange_pair),0);
            if ( cp != 0 ) // encrypted dbs
            {
                sdb = &SuperNET_dbs[TELEPOD_DATA];
                sdb->privkeys = validate_ciphers(&sdb->cipherids,cp,cp->ciphersobj);
                sdb = &SuperNET_dbs[CONTACT_DATA];
                sdb->privkeys = validate_ciphers(&sdb->cipherids,cp,cp->ciphersobj);
            }
            if ( portable_thread_create((void *)_process_SuperNET_dbqueue,0) == 0 )
                printf("ERROR hist process_hashtablequeues\n");
        }
    }
    return(0);
}

void close_SuperNET_dbs()
{
    int32_t i,selector;
    for (selector=0; selector<NUM_SUPERNET_DBS; selector++)
        close_SuperNET_db(&SuperNET_dbs[selector],selector);
    if ( Num_pricedbs != 0 && Price_dbs != 0 )
    {
        for (i=0; i<Num_pricedbs; i++)
        {
            if ( close_SuperNET_db(&Price_dbs[i],i) == 0 )
                Price_dbs[i].dbp->close(Price_dbs[i].dbp,0);
        }
    }
    Num_pricedbs = 0;
    //free(Price_dbs);
    //Price_dbs = 0;
    memset(Price_dbs,0,sizeof(Price_dbs));
    memset(SuperNET_dbs,0,sizeof(SuperNET_dbs));
}


#endif
