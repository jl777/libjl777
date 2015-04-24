//
//  ramchaindb.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_ramchaindb_h
#define crypto777_ramchaindb_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "db.h"
#include "uthash.h"
#include "cJSON.h"
#include "bits777.c"
#include "system777.c"
#include "msig.c"

struct nodestats
{
    //struct storage_header H;
    uint8_t pubkey[256>>3];
    struct nodestats *eviction;
    uint64_t nxt64bits,coins[4];
    double pingpongsum;
    float pingmilli,pongmilli;
    uint32_t ipbits,lastcontact,numpings,numpongs;
    uint8_t BTCD_p2p,gotencrypted,modified,expired,isMM;
};

struct acct_coin { uint64_t *srvbits; char name[16],**acctcoinaddrs,**pubkeys; int32_t numsrvbits; };

struct NXT_acct
{
    UT_hash_handle hh;
    struct NXT_str H;
    struct NXT_asset **assets;
    uint64_t *quantities,bestbits,quantity;
    struct NXT_assettxid_list **txlists;    // one list for each asset in acct
    int32_t maxassets,numassets,bestdist,numcoins,pendingdeposits,openorders;
    int64_t buyqty,buysum,sellqty,sellsum;
    uint32_t timestamp;
    struct acct_coin *coins[64];
    struct nodestats stats;
    char *signedtx;
};
struct storage_header **copy_all_DBentries(int32_t *nump);
void set_MGW_depositfname(char *fname,char *NXTaddr);
int32_t update_MGW_jsonfile(void (*setfname)(char *fname,char *NXTaddr),void *(*extract_jsondata)(cJSON *item,void *arg,void *arg2),int32_t (*jsoncmp)(void *ref,void *item),char *NXTaddr,char *jsonstr,void *arg,void *arg2);
void *extract_jsonints(cJSON *item,void *arg,void *arg2);
int32_t jsonmsigcmp(void *ref,void *item);
int32_t jsonstrcmp(void *ref,void *item);
struct nodestats *get_nodestats(uint64_t nxt64bits);
void _set_RTmgwname(char *RTmgwname,char *name,char *coinstr,int32_t gatewayid,uint64_t redeemtxid);

//struct acct_coin *find_NXT_coininfo(struct NXT_acct **npp,uint64_t nxt64bits,char *coinstr);
struct acct_coin *get_NXT_coininfo(uint64_t srvbits,char *acctcoinaddr,char *pubkey,uint64_t nxt64bits,char *coinstr);
void add_NXT_coininfo(uint64_t srvbits,uint64_t nxt64bits,char *coinstr,char *acctcoinaddr,char *pubkey);
cJSON *http_search(char *destip,char *type,char *file);

extern int32_t DBSLEEP;
extern char MGWROOT[];

#endif
#else
#ifndef crypto777_ramchaindb_c
#define crypto777_ramchaindb_c


#ifndef crypto777_ramchaindb_h
#define DEFINES_ONLY
#include __BASE_FILE__
#undef DEFINES_ONLY
#endif

#ifdef bidfa
struct SuperNET_db MSIGDB;

int32_t _process_dbiter(struct SuperNET_db *sdb)
{
    int32_t n;
    DBT data;
    struct dbreq *req;
    if ( sdb == 0 || sdb->dbp == 0 )
        return(0);
    n = 0;
    if ( (req= queue_dequeue(&sdb->queue,0)) != 0 )
    {
        memset(&data,0,sizeof(data));
        if ( req->data != 0 )
            data = *req->data;
        //fprintf(stderr,"%s func.%c key.(%s)\n",sdb->name,req->funcid,req->key.data);
        switch ( req->funcid )
        {
            case 'G': req->retval = sdb->dbp->get(sdb->dbp,req->txn,&req->key,&data,req->flags); break;
            case 'P':
                req->retval = sdb->dbp->put(sdb->dbp,req->txn,&req->key,&data,req->flags);
                if ( sdb->overlap_write != 0 )
                {
                    //printf("overlapped write data.%p\n",req->data->data);
                    if ( req->data != 0 && req->data->data != 0 )
                    {
                        if ( req->key.data != 0 )
                            free(req->key.data);
                        free(req->data->data);
                        free(req->data);
                    }
                    free(req);
                    sdb->dbp->sync(sdb->dbp,0);
                    sdb->busy--;
                    n = queue_size(&sdb->queue);
                    if ( n > 0 )
                        fprintf(stderr,"%d ",n);
                    return(1);
                }
                break;
            case 'S': req->retval = sdb->dbp->sync(sdb->dbp,req->flags); break;
            case 'D': req->retval = sdb->dbp->del(sdb->dbp,req->txn,&req->key,req->flags); break;
            case 'O': req->retval = sdb->dbp->cursor(sdb->dbp,req->txn,req->cursor,req->flags); break;
            case 'C': req->retval = ((DBC *)req->cursor)->close(req->cursor); break;
            case 'g': req->retval = ((DBC *)req->cursor)->get(req->cursor,&req->key,&data,req->flags); break;
            case 'd': req->retval = ((DBC *)req->cursor)->del(req->cursor,req->flags); break;
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
    struct SuperNET_db *sdb = &MSIGDB;
    while ( sdb->active > 0 )
    {
       if ( _process_dbiter(sdb) == 0 )
            msleep(DBSLEEP);
    }
    if ( sdb->dbp != 0 )
        sdb->dbp->close(sdb->dbp,0);
    sdb->active = -1;
    /*for (i=0; i<Num_pricedbs; i++)
     {
     sdb = &Price_dbs[i];
     sdb->dbp->close(sdb->dbp,0);
     memset(sdb,0,sizeof(*sdb));
     sdb->active = -1;
     fprintf(stderr,"finished processing Price_dbs.%d\n",i);
     }*/
    return(0);
}

int32_t _block_on_dbreq(struct dbreq *req)
{
    struct SuperNET_db *sdb = req->sdb;
    int32_t busy,retval = 0;
    if ( sdb->overlap_write == 0 )
    {
        while ( req->doneflag == 0 )
            msleep(DBSLEEP); // if not done after the first context switch, likely to take a while
        retval = req->retval;
        free(req);
        sdb->busy--;
        if ( (busy= sdb->busy) != 0 ) // busy is not critical for data integrity, but helps with dbreq latency
        {
            //fprintf(stderr,"_block_on_dbreq: unlikely case of busy.%d != 0, (%d) for (%s)\n",busy,sdb->busy,sdb->name);
            sdb->busy = 0;
        }
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
        {
            req->key = *key;
            if ( sdb->overlap_write != 0 )
            {
                req->key.data = calloc(1,key->size);
                memcpy(req->key.data,key->data,key->size);
            }
        }
        req->data = data;
        req->flags = flags;
        req->cursor = cursor;
        sdb->busy++;
        queue_enqueue(sdb->name,&sdb->queue,&req->DL);
        msleep(DBSLEEP); // allow context switch so request has a chance of completing
    }
    return(req);
}

int32_t dbcmd(char *debugstr,int32_t funcid,struct SuperNET_db *sdb,DB_TXN *txn,DBT *key,DBT *data,int32_t flags,void *cursor)
{
    struct dbreq *req;
    if ( sdb->dbp == 0 )
        return(-1);
    if ( sdb->dbp != 0 && sdb->active > 0 )
    {
        if ( (req= _queue_dbreq(funcid,sdb,txn,key,data,flags,cursor)) != 0 )
            return(_block_on_dbreq(req));
    }
    return(-1);
}

int32_t dbget(struct SuperNET_db *sdb,DB_TXN *txn,DBT *key,DBT *data,int32_t flags)
{
    return(dbcmd("dbget",'G',sdb,txn,key,data,flags,0));
}

int32_t dbput(struct SuperNET_db *sdb,DB_TXN *txn,DBT *key,DBT *data,int32_t flags)
{
    if ( sdb->dbp != 0 )
        return(dbcmd("dbput",'P',sdb,txn,key,data,flags,0));
    return(-1);
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

int32_t cursordel(struct SuperNET_db *sdb,DB_TXN *txn,DBC *cursor,int32_t flags)
{
    return(dbcmd("cursordel",'d',sdb,txn,0,0,flags,cursor));
}

int32_t cursorput(struct SuperNET_db *sdb,DB_TXN *txn,DBC *cursor,DBT *key,DBT *data,int32_t flags)
{
    return(dbcmd("cursorput",'p',sdb,txn,key,data,flags,cursor));
}

int32_t cursorclose(struct SuperNET_db *sdb,DBC *cursor)
{
    return(dbcmd("cursorclose",'C',sdb,0,0,0,0,cursor));
}

void *decondition_storage(uint32_t *lenp,struct SuperNET_db *sdb,void *data,uint32_t size)
{
    void *ptr;
    if ( data == 0 || size == 0 )
        return(0);
    /*if ( sdb->privkeys != 0 && sdb->cipherids != 0 )
     {
     *lenp = size;
     ptr = ciphers_codec(1,sdb->privkeys,sdb->cipherids,data,(int32_t *)lenp);
     if ( *lenp >= sdb->minsize && *lenp <= sdb->maxsize )
     return(ptr);
     if ( ptr != 0 )
     free(ptr);
     //printf("unencrypted entry size.%d\n",*lenp);
     }*/
    *lenp = size;
    ptr = malloc(size);
    memcpy(ptr,data,size);
    return(ptr);
}

/*void *condition_storage(uint32_t *lenp,struct SuperNET_db *sdb,void *data,uint32_t size)
 {
 *lenp = size;
 //fprintf(stderr,"condition_storage\n");
 if ( data == 0 || size <= 0 )
 return(0);
 if ( sdb->privkeys == 0 || sdb->cipherids == 0 )
 return(data);
 else return(ciphers_codec(0,sdb->privkeys,sdb->cipherids,data,(int32_t *)lenp));
 }*/

void clear_pair(DBT *key,DBT *data)
{
    memset(key,0,sizeof(DBT));
    memset(data,0,sizeof(DBT));
}

struct storage_header **copy_all_DBentries(int32_t *nump)
{
    struct storage_header *ptr,**ptrs = 0;
    struct SuperNET_db *sdb;
    int32_t ret,max,m,n = 0;
    DBT key,data;
    DBC *cursorp = 0;
    *nump = 0;
    sdb = &MSIGDB;//SuperNET_dbs[selector];
    if ( sdb->active <= 0 )
        return(0);
    max = 100;
    m = 0;
    if ( (ret= dbcursor(sdb,NULL,&cursorp,0)) != 0 )
    {
        sdb->storage->err(sdb->storage,ret,"copy_all_DBentries: Cursor open failed.");
        return(0);
    }
    if ( cursorp != 0 )
    {
        clear_pair(&key,&data);
        ptrs = (struct storage_header **)calloc(sizeof(struct storage_header *),max+1);
        while ( (ret= cursorget(sdb,NULL,cursorp,&key,&data,DB_NEXT)) == 0 )//&& data.size > 0 )
        {
            m++;
            //printf("%d size.%d\n",n,data.size);
            ptr = decondition_storage(&data.size,sdb,data.data,data.size);
            ptrs[n++] = ptr;
            if ( n >= max )
            {
                max = n + 100;
                ptrs = (struct storage_header **)realloc(ptrs,sizeof(struct storage_header *)*(max+1));
            }
            clear_pair(&key,&data);
        }
        cursorclose(sdb,cursorp);
    }
    //fprintf(stderr,"done copy all dB.%d\n",selector);
    if ( ptrs != 0 )
        ptrs[n] = 0;
    *nump = n;
    return(ptrs);
}
#endif

struct multisig_addr *MSIG_table,**MSIGS;
portable_mutex_t MSIGmutex;
int32_t didMSIGinit,Num_MSIGS;
struct multisig_addr *find_msigaddr(char *msigaddr)
{
    int32_t i;//createdflag;
    struct multisig_addr *msig = 0;
    if ( didMSIGinit == 0 )
    {
        portable_mutex_init(&MSIGmutex);
        didMSIGinit = 1;
    }
    portable_mutex_lock(&MSIGmutex);
    //HASH_FIND(hh,MSIG_table,msigaddr,strlen(msigaddr),msig);
    for (i=0; i<Num_MSIGS; i++)
        if ( strcmp(msigaddr,MSIGS[i]->multisigaddr) == 0 )
        {
            msig = MSIGS[i];
            break;
        }
    portable_mutex_unlock(&MSIGmutex);
    return(msig);
    /*if ( MTsearch_hashtable(&SuperNET_dbs[MULTISIG_DATA].ramtable,msigaddr) == HASHSEARCH_ERROR )
     {
     printf("(%s) not MGW multisig addr\n",msigaddr);
     return(0);
     }
     printf("found (%s)\n",msigaddr);
     return(MTadd_hashtable(&createdflag,&SuperNET_dbs[MULTISIG_DATA].ramtable,msigaddr));*/ // only do this if it is already there
    //return((struct multisig_addr *)find_storage(MULTISIG_DATA,msigaddr,0));
}

struct multisig_addr *ram_add_msigaddr(char *msigaddr,int32_t n,char *NXTaddr,char *NXTpubkey,int32_t buyNXT)
{
    struct multisig_addr *msig;
    if ( (msig= find_msigaddr(msigaddr)) == 0 )
    {
        msig = calloc(1,sizeof(*msig) + n*sizeof(struct pubkey_info));
        strcpy(msig->multisigaddr,msigaddr);
        if ( NXTaddr != 0 )
            strcpy(msig->NXTaddr,NXTaddr);
        if ( NXTpubkey != 0 )
            strcpy(msig->NXTpubkey,NXTpubkey);
        if ( buyNXT >= 0 )
            msig->buyNXT = buyNXT;
        if ( didMSIGinit == 0 )
        {
            //portable_mutex_init(&MSIGmutex);
            didMSIGinit = 1;
        }
        //printf("ram_add_msigaddr MSIG[%s] NXT.%s (%s) buyNXT.%d\n",msigaddr,msig->NXTaddr,msig->NXTpubkey,msig->buyNXT);
        portable_mutex_lock(&MSIGmutex);
        MSIGS = realloc(MSIGS,(1+Num_MSIGS) * sizeof(*MSIGS));
        MSIGS[Num_MSIGS] = msig, Num_MSIGS++;
        //HASH_ADD_KEYPTR(hh,MSIG_table,clonestr(msigaddr),strlen(msigaddr),msig);
        portable_mutex_unlock(&MSIGmutex);
        //printf("done ram_add_msigaddr MSIG[%s] NXT.%s (%s) buyNXT.%d\n",msigaddr,msig->NXTaddr,msig->NXTpubkey,msig->buyNXT);
    }
    return(msig);
}

struct NXT_acct *get_NXTacct(int32_t *createdp,struct NXT_acct **rootp,char *NXTaddr)
{
    struct NXT_acct *np;
    //printf("NXTaccts hash %p\n",mp->NXTaccts_tablep);
    //printf("get_NXTacct.(%s)\n",NXTaddr);
    np = 0;//MTadd_hashtable(createdp,rootp,NXTaddr);
    if ( *createdp != 0 )
    {
        //queue_enqueue(&np->incoming,clonestr("testmessage"));
        np->H.nxt64bits = calc_nxt64bits(NXTaddr);
        //portable_set_illegaludp(&np->Usock);//np->Usock = -1;
    }
    return(np);
}

struct nodestats *get_nodestats(uint64_t nxt64bits)
{
    struct NXT_acct *get_NXTacct(int32_t *createdp,struct NXT_acct **rootp,char *NXTaddr);
    struct nodestats *stats = 0;
    int32_t createdflag;
    struct NXT_acct *np;
    char NXTaddr[64];
    if ( nxt64bits != 0 )
    {
        expand_nxt64bits(NXTaddr,nxt64bits);
        np = get_NXTacct(&createdflag,&NXT_accts,NXTaddr);
        stats = &np->stats;
        if ( stats->nxt64bits == 0 )
            stats->nxt64bits = nxt64bits;
    }
    return(stats);
}

struct NXT_assettxid *find_NXT_assettxid(int32_t *createdflagp,struct NXT_asset *ap,char *txid)
{
    int32_t createdflag;
    struct NXT_assettxid *tp;
    if ( createdflagp == 0 )
        createdflagp = &createdflag;
    tp = 0;//MTadd_hashtable(createdflagp,&NXT_assettxids,txid);
    if ( *createdflagp != 0 )
    {
        //tp->assetbits = ap->assetbits;
        // tp->redeemtxid = calc_nxt64bits(txid);
        // tp->timestamp = timestamp;
        //printf("%d) %s txid.%s\n",ap->num,ap->name,txid);
        if ( ap != 0 )
        {
            if ( ap->num >= ap->max )
            {
                ap->max = ap->num + NXT_ASSETLIST_INCR;
                ap->txids = realloc(ap->txids,sizeof(*ap->txids) * ap->max);
            }
            ap->txids[ap->num++] = tp;
        }
    }
    return(tp);
}

struct acct_coin *find_NXT_coininfo(struct NXT_acct **npp,uint64_t nxt64bits,char *coinstr)
{
    char NXTaddr[64];
    struct NXT_acct *np;
    int32_t i,createdflag;
    expand_nxt64bits(NXTaddr,nxt64bits);
    np = get_NXTacct(&createdflag,&NXT_accts,NXTaddr);
    if ( npp != 0 )
        (*npp) = np;
    if ( np->numcoins > 0 )
    {
        for (i=0; i<np->numcoins; i++)
            if ( np->coins[i] != 0 && strcmp(np->coins[i]->name,coinstr) == 0 )
                return(np->coins[i]);
    }
    return(0);
}

struct acct_coin *get_NXT_coininfo(uint64_t srvbits,char *acctcoinaddr,char *pubkey,uint64_t nxt64bits,char *coinstr)
{
    struct acct_coin *acp = 0;
    int32_t i;
    acctcoinaddr[0] = pubkey[0] = 0;
    if ( (acp= find_NXT_coininfo(0,nxt64bits,coinstr)) != 0 )
    {
        if ( acp->numsrvbits > 0 )
        {
            for (i=0; i<acp->numsrvbits; i++)
            {
                if ( acp->srvbits[i] == srvbits )
                {
                    if ( acp->pubkeys[i] != 0 )
                        strcpy(pubkey,acp->pubkeys[i]);
                    if ( acp->acctcoinaddrs[i] != 0 )
                        strcpy(acctcoinaddr,acp->acctcoinaddrs[i]);
                    return(acp);
                }
            }
        }
    }
    return(0);
}

void add_NXT_coininfo(uint64_t srvbits,uint64_t nxt64bits,char *coinstr,char *acctcoinaddr,char *pubkey)
{
    int32_t i;
    struct NXT_acct *np;
    struct acct_coin *acp;
    if ( (acp= find_NXT_coininfo(&np,nxt64bits,coinstr)) == 0 )
    {
        np->coins[np->numcoins++] = acp = calloc(1,sizeof(*acp));
        safecopy(acp->name,coinstr,sizeof(acp->name));
    }
    if ( acp->numsrvbits > 0 )
    {
        for (i=0; i<acp->numsrvbits; i++)
        {
            if ( acp->srvbits[i] == srvbits )
            {
                if ( acp->pubkeys[i] != 0 )
                {
                    if ( strcmp(pubkey,acp->pubkeys[i]) != 0 )
                        printf(">>>>>>>>>> WARNING ADDCOININFO.(%s -> %s) for %llu;%llu\n",acp->pubkeys[i],pubkey,(long long)srvbits,(long long)nxt64bits);
                    //else printf("MATCHED pubkey ");
                    free(acp->pubkeys[i]);
                    acp->pubkeys[i] = 0;
                }
                if ( acp->acctcoinaddrs[i] != 0 )
                {
                    if ( strcmp(acctcoinaddr,acp->acctcoinaddrs[i]) != 0 )
                        printf(">>>>>>>>>> WARNING ADDCOININFO.(%s -> %s) for %llu;%llu\n",acp->acctcoinaddrs[i],acctcoinaddr,(long long)srvbits,(long long)nxt64bits);
                    //else printf("MATCHED acctcoinaddr ");
                    free(acp->acctcoinaddrs[i]);
                    acp->acctcoinaddrs[i] = 0;
                }
                break;
            }
        }
    } else i = acp->numsrvbits;
    if ( i == acp->numsrvbits )
    {
        acp->numsrvbits++;
        acp->srvbits = realloc(acp->srvbits,sizeof(*acp->srvbits) * acp->numsrvbits);
        acp->acctcoinaddrs = realloc(acp->acctcoinaddrs,sizeof(*acp->acctcoinaddrs) * acp->numsrvbits);
        acp->pubkeys = realloc(acp->pubkeys,sizeof(*acp->pubkeys) * acp->numsrvbits);
    }
    if ( (MGW_initdone == 0 && Debuglevel > 3) || (MGW_initdone != 0 && Debuglevel > 2) )
        printf("ADDCOININFO.(%s %s) for %llu:%llu\n",acctcoinaddr,pubkey,(long long)srvbits,(long long)nxt64bits);
    acp->srvbits[i] = srvbits;
    acp->pubkeys[i] = clonestr(pubkey);
    acp->acctcoinaddrs[i] = clonestr(acctcoinaddr);
}

cJSON *http_search(char *destip,char *type,char *file)
{
    cJSON *json = 0;
    char url[1024],*retstr;
    sprintf(url,"http://%s/%s/%s",destip,type,file);
    if ( (retstr= issue_curl(0,url)) != 0 )
    {
        json = cJSON_Parse(retstr);
        free(retstr);
    }
    return(json);
}

int32_t update_msig_info(struct multisig_addr *msig,int32_t syncflag,char *sender)
{
    DBT key,data,*datap;
    int32_t i,ret = 0;
    struct multisig_addr *msigram;
    struct SuperNET_db *sdb = &MSIGDB;
    update_MGW_msig(msig,sender);
    if ( msig == 0 )
    {
        if ( syncflag != 0 && sdb != 0 && sdb->storage != 0 )
            return(dbsync(sdb,0));
        return(0);
    }
    for (i=0; i<msig->n; i++)
        if ( msig->pubkeys[i].nxt64bits != 0 && msig->pubkeys[i].coinaddr[0] != 0 && msig->pubkeys[i].pubkey[0] != 0 )
            add_NXT_coininfo(msig->pubkeys[i].nxt64bits,calc_nxt64bits(msig->NXTaddr),msig->coinstr,msig->pubkeys[i].coinaddr,msig->pubkeys[i].pubkey);
    if ( msig->H.size == 0 )
        msig->H.size = sizeof(*msig) + (msig->n * sizeof(msig->pubkeys[0]));
    msigram = ram_add_msigaddr(msig->multisigaddr,msig->n,msig->NXTaddr,msig->NXTpubkey,msig->buyNXT);//MTadd_hashtable(&createdflag,&sdb->ramtable,msig->multisigaddr);
    if ( msigram->created != 0 && msig->created != 0 )
    {
        if ( msigram->created < msig->created )
            msig->created = msigram->created;
        else msigram->created = msig->created;
    }
    else if ( msig->created == 0 )
        msig->created = msigram->created;
    if ( msigram->NXTpubkey[0] != 0 && msig->NXTpubkey[0] == 0 )
        safecopy(msig->NXTpubkey,msigram->NXTpubkey,sizeof(msig->NXTpubkey));
    //if ( msigram->sender == 0 && msig->sender != 0 )
    //    createdflag = 1;
    if ( memcmp(msigram,msig,msig->H.size) != 0 ) //createdflag != 0 ||
    {
        clear_pair(&key,&data);
        key.data = msig->multisigaddr;
        key.size = (int32_t)(strlen(msig->multisigaddr) + 1);
        data.size = msig->H.size;
        if ( sdb->overlap_write != 0 )
        {
            data.data = calloc(1,msig->H.size);
            memcpy(data.data,msig,msig->H.size);
            datap = calloc(1,sizeof(*datap));
            *datap = data;
        }
        else
        {
            data.data = msig;
            datap = &data;
        }
        if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
            printf("add (%s) NXTpubkey.(%s) sdb.%p\n",msig->multisigaddr,msig->NXTpubkey,sdb->dbp);
        if ( sdb != 0 && sdb->storage != 0 )
        {
            if (  (ret= dbput(sdb,0,&key,datap,0)) != 0 )
                sdb->storage->err(sdb->storage,ret,"Database put for quote failed.");
            else if ( syncflag != 0 ) ret = dbsync(sdb,0);
        }
        ret = 1;
    }
    return(ret);
}

struct multisig_addr *http_search_msig(char *external_NXTaddr,char *external_ipaddr,char *NXTaddr)
{
    int32_t i,n;
    cJSON *array;
    struct multisig_addr *msig = 0;
    if ( (array= http_search(external_ipaddr,"MGW/msig",NXTaddr)) != 0 )
    {
        if ( is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
        {
            for (i=0; i<n; i++)
                if ( (msig= decode_msigjson(0,cJSON_GetArrayItem(array,i),external_NXTaddr)) != 0 && (msig= find_msigaddr(msig->multisigaddr)) != 0 )
                    break;
        }
        free_json(array);
    }
    return(msig);
}


#endif
#endif
