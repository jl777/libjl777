
#ifdef DEFINES_ONLY
#ifndef acct777_h
#define acct777_h

#include <stdio.h>
#include <stdint.h>
#include <memory.h>
#include "uthash.h"
#include "utils/bits777.c"
#include "serdes777.c"
#include "txind777.c"

struct accts777_info *accts777_init(char *dirname,struct txinds777_info *txinds);

#endif
#else
#ifndef acct777_c
#define acct777_c

/*#ifndef acct777_h
#define DEFINES_ONLY
#include "accts777.c"
#undef DEFINES_ONLY
#endif*/

#define ACCTS777_MAXRAMKVS 8
#define BTCDADDRSIZE 36

struct ramkv777_item { UT_hash_handle hh; uint16_t valuesize,tbd; uint32_t ind; uint8_t keyvalue[]; };
struct ramkv777
{
    char name[63],threadsafe;
    portable_mutex_t mutex;
    struct ramkv777_item *table;
    struct sha256_state state; bits256 sha256;
    int32_t numkeys,keysize,dispflag; uint8_t kvind;
};

struct accts777_info
{
    queue_t PaymentsQ;
    struct peggy_unit *units;
    int32_t numunits; uint8_t numkvs;
    struct ramkv777 *bets,*pricefeeds,*hashaddrs,*coinaddrs,*SaMaddrs,*nxtaddrs,*addrkvs[16];
    bits256 peggyhash;
    struct txinds777_info *txinds;
};

#define ramkv777_itemsize(kv,valuesize) (sizeof(struct ramkv777_item) + (kv)->keysize + valuesize)
#define ramkv777_itemkey(item) (item)->keyvalue
#define ramkv777_itemvalue(kv,item) (&(item)->keyvalue[(kv)->keysize])
#define ramkv777_rawind(kv,value) (*(uint32_t *)((void *)((long)(value) - (kv)->keysize - sizeof(uint32_t))))
#define acct777_getaddrkv(accts,type) (accts)->addrkvs[type]

void ramkv777_lock(struct ramkv777 *kv)
{
    if ( kv->threadsafe != 0 )
        portable_mutex_lock(&kv->mutex);
}

void ramkv777_unlock(struct ramkv777 *kv)
{
    if ( kv->threadsafe != 0 )
        portable_mutex_unlock(&kv->mutex);
}

int32_t ramkv777_delete(struct ramkv777 *kv,void *key)
{
    int32_t retval = -1; struct ramkv777_item *ptr = 0;
    if ( kv == 0 )
        return(-1);
    ramkv777_lock(kv);
    HASH_FIND(hh,kv->table,key,kv->keysize,ptr);
    if ( ptr != 0 )
    {
        HASH_DELETE(hh,kv->table,ptr);
        free(ptr);
        retval = 0;
    }
    ramkv777_lock(kv);
    return(retval);
}

void *ramkv777_write(struct ramkv777 *kv,void *key,void *value,int32_t valuesize)
{
    struct ramkv777_item *item = 0; int32_t keysize = kv->keysize;
    if ( kv == 0 )
        return(0);
    ramkv777_lock(kv);
    HASH_FIND(hh,kv->table,key,keysize,item);
    if ( item != 0 )
    {
        if ( valuesize == item->valuesize )
        {
            if ( memcmp(ramkv777_itemvalue(kv,item),value,valuesize) != 0 )
            {
                update_sha256(kv->sha256.bytes,&kv->state,key,kv->keysize);
                update_sha256(kv->sha256.bytes,&kv->state,value,valuesize);
                memcpy(ramkv777_itemvalue(kv,item),value,valuesize);
            }
            ramkv777_unlock(kv);
            return(item);
        }
        HASH_DELETE(hh,kv->table,item);
        free(item);
        update_sha256(kv->sha256.bytes,&kv->state,key,kv->keysize);
    }
    item = calloc(1,ramkv777_itemsize(kv,valuesize));
    memcpy(item->keyvalue,key,kv->keysize);
    memcpy(ramkv777_itemvalue(kv,item),value,valuesize);
    item->valuesize = valuesize;
    item->ind = (kv->numkeys++ * ACCTS777_MAXRAMKVS) | kv->kvind;
    //printf("add.(%s) kv->numkeys.%d keysize.%d valuesize.%d\n",kv->name,kv->numkeys,keysize,valuesize);
    HASH_ADD_KEYPTR(hh,kv->table,ramkv777_itemkey(item),kv->keysize,item);
    update_sha256(kv->sha256.bytes,&kv->state,key,kv->keysize);
    update_sha256(kv->sha256.bytes,&kv->state,value,valuesize);
    ramkv777_unlock(kv);
    if ( kv->dispflag != 0 )
        fprintf(stderr,"%016llx ramkv777_write numkeys.%d kv.%p table.%p write key.%08x size.%d, value.(%08x) size.%d\n",(long long)kv->sha256.txid,kv->numkeys,kv,kv->table,*(int32_t *)key,keysize,_crc32(0,value,valuesize),valuesize);
    return(ramkv777_itemvalue(kv,item));
}

void *ramkv777_read(int32_t *valuesizep,struct ramkv777 *kv,void *key)
{
    struct ramkv777_item *item = 0;  int32_t keysize = kv->keysize;
    if ( kv == 0 )
        return(0);
    ramkv777_lock(kv);
    HASH_FIND(hh,kv->table,key,keysize,item);
    ramkv777_unlock(kv);
    if ( item != 0 )
    {
        if ( valuesizep != 0 )
            *valuesizep = item->valuesize;
        return(ramkv777_itemvalue(kv,item));
    }
    if ( valuesizep != 0 )
        *valuesizep = 0;
    return(0);
}

void *ramkv777_iterate(struct ramkv777 *kv,void *args,void *(*iterator)(struct ramkv777 *kv,void *args,void *key,void *value,int32_t valuesize))
{
    struct ramkv777_item *item,*tmp; void *retval = 0;
    if ( kv == 0 )
        return(0);
    ramkv777_lock(kv);
    HASH_ITER(hh,kv->table,item,tmp)
    {
        if ( (retval= (*iterator)(kv,args!=0?args:item,item->keyvalue,ramkv777_itemvalue(kv,item),item->valuesize)) != 0 )
        {
            ramkv777_unlock(kv);
            return(retval);
        }
    }
    ramkv777_unlock(kv);
    return(0);
}

struct ramkv777 *ramkv777_init(int32_t kvind,char *name,int32_t keysize,int32_t threadsafe)
{
    struct ramkv777 *kv;
    printf("ramkv777_init.(%s)\n",name);
    kv = calloc(1,sizeof(*kv));
    safecopy(kv->name,name,sizeof(kv->name));
    kv->threadsafe = threadsafe, kv->keysize = keysize, kv->kvind = kvind, kv->dispflag = 1;
    portable_mutex_init(&kv->mutex);
    update_sha256(kv->sha256.bytes,&kv->state,0,0);
    return(kv);
}

void ramkv777_free(struct ramkv777 *kv)
{
    struct ramkv777_item *ptr,*tmp;
    if ( kv != 0 )
    {
        HASH_ITER(hh,kv->table,ptr,tmp)
        {
            HASH_DEL(kv->table,ptr);
            free(ptr);
        }
        free(kv);
    }
}

int32_t ramkv777_clone(struct ramkv777 *clone,struct ramkv777 *kv)
{
    struct ramkv777_item *item,*tmp; int32_t n = 0;
    if ( kv != 0 )
    {
        HASH_ITER(hh,kv->table,item,tmp)
        {
            ramkv777_write(clone,item->keyvalue,ramkv777_itemvalue(kv,item),item->valuesize);
            n++;
        }
    }
    return(n);
}

struct accts777_info *accts777_init(char *dirname,struct txinds777_info *txinds)
{
    struct accts777_info *accts = calloc(1,sizeof(*accts));
    accts->hashaddrs = ramkv777_init(accts->numkvs++,"hashaddrs",sizeof(bits256),1);
    accts->coinaddrs = ramkv777_init(accts->numkvs++,"coinaddrs",BTCDADDRSIZE,1);
    accts->nxtaddrs = ramkv777_init(accts->numkvs++,"nxtaddrs",sizeof(uint64_t),1);
    accts->SaMaddrs = ramkv777_init(accts->numkvs++,"SaMaddrs",sizeof(bits384),1);
    accts->bets = ramkv777_init(accts->numkvs++,"bets",BTCDADDRSIZE,1);
    accts->pricefeeds = ramkv777_init(accts->numkvs++,"pricefeeds",sizeof(uint32_t) * 2,1);//, accts->pricefeeds->dispflag = 0;
    if ( accts->numkvs > ACCTS777_MAXRAMKVS )
    {
        printf("too many ramkvs for accts %d vs %d\n",accts->numkvs,ACCTS777_MAXRAMKVS);
        exit(-1);
    }
    accts->addrkvs[PEGGY_ADDRFUNDING] = accts->addrkvs[PEGGY_ADDRBTCD] = accts->coinaddrs;
    accts->addrkvs[PEGGY_ADDR777] = accts->SaMaddrs;
    accts->addrkvs[PEGGY_ADDRNXT] = accts->nxtaddrs;
    accts->addrkvs[PEGGY_ADDRCREATE] = accts->addrkvs[PEGGY_ADDRUNIT] = accts->addrkvs[PEGGY_ADDRPUBKEY] = accts->hashaddrs;
    if ( (accts->txinds= txinds) == 0 )
        accts->txinds = txinds777_init(dirname,"txinds");
    return(accts);
}

void accts777_free(struct accts777_info *accts)
{
    int32_t i;
    queue_free(&accts->PaymentsQ);
    for (i=0; i<sizeof(accts->addrkvs)/sizeof(*accts->addrkvs); i++)
        if ( accts->addrkvs[i] != 0 )
            ramkv777_free(accts->addrkvs[i]);
    free(accts);
}

struct accts777_info *accts777_clone(char *path,struct accts777_info *accts)
{
    struct accts777_info *clone;
    clone = accts777_init(path,accts->txinds);
    queue_clone(&clone->PaymentsQ,&accts->PaymentsQ,sizeof(struct opreturn_payment));
    if ( accts->numunits > 0 && accts->units != 0 )
    {
        clone->units = calloc(accts->numunits,sizeof(*accts->units));
        memcpy(clone->units,accts->units,accts->numunits * sizeof(*accts->units));
    }
    clone->peggyhash = accts->peggyhash;
    ramkv777_clone(clone->bets,accts->bets);
    ramkv777_clone(clone->pricefeeds,accts->pricefeeds);
    ramkv777_clone(clone->hashaddrs,accts->hashaddrs);
    ramkv777_clone(clone->coinaddrs,accts->coinaddrs);
    ramkv777_clone(clone->SaMaddrs,accts->SaMaddrs);
    ramkv777_clone(clone->nxtaddrs,accts->nxtaddrs);
    return(clone);
}

void *accts777_key(union peggy_addr *addr,int32_t type)
{
    void *key;
    switch ( type )
    {
        case PEGGY_ADDRFUNDING: case PEGGY_ADDRBTCD: key = addr->coinaddr; break;
        case PEGGY_ADDRNXT: key = &addr->nxt64bits; break;
        case PEGGY_ADDR777: key = &addr->SaMbits; break;
        case PEGGY_ADDRCREATE: key = &addr->newunit.sha256; break;
        case PEGGY_ADDRUNIT: key = &addr->sha256; break;
        case PEGGY_ADDRPUBKEY: key = &addr->sha256; break;
    }
    return(key);
}

struct acct777 *accts777_find(int32_t *valuesizep,struct accts777_info *accts,union peggy_addr *addr,int32_t type)
{
    void *key;
    if ( (key= accts777_key(addr,type)) != 0 )
        return(ramkv777_read(valuesizep,accts->addrkvs[type],key));
    else
    {
        if ( valuesizep != 0 )
            *valuesizep = 0;
        return(0);
    }
}

struct acct777 *accts777_create(struct accts777_info *accts,union peggy_addr *addr,int32_t type,uint32_t blocknum,uint32_t blocktimestamp)
{
    struct acct777 *acct,A;
    if ( (acct= accts777_find(0,accts,addr,type)) == 0 )
    {
        memset(&A,0,sizeof(A));
        A.firstblocknum = blocknum, A.firsttimestamp = blocktimestamp;
        acct = ramkv777_write(accts->addrkvs[type],accts777_key(addr,type),&A,sizeof(A));
    }
    else if ( blocknum < acct->firstblocknum || blocktimestamp < acct->firsttimestamp )
    {
        printf("accts777_create: already exists but with an earlier block/timestamp? %u:%u vs %u:%u\n",blocknum,acct->firstblocknum,blocktimestamp,acct->firsttimestamp);
        return(0);
    }
    return(acct);
}

void peggy_delete(struct accts777_info *accts,struct peggy_unit *U,int32_t reason)
{
    memcpy(U,&accts->units[--accts->numunits],sizeof(struct peggy_unit));
    //U->redeemed = (uint32_t)time(NULL);
    //if ( PEGS->lockhashes != 0 )
    //    kv777_delete(PEGS->lockhashes,U->lockPeriodHash,HASH_SIZE);
}

int32_t peggy_addunit(struct accts777_info *accts,struct peggy_unit *U,bits256 lockhash)
{
    U->lockhash = lockhash;
    accts->units = realloc(accts->units,sizeof(*accts->units) * (accts->numunits + 1));
    accts->units[accts->numunits] = *U;
    //if ( PEGS->lockhashes != 0 )
    //    kv777_write(PEGS->lockhashes,lockPeriodHash,HASH_SIZE,U,sizeof(*U));
    return(accts->numunits++);
}

struct peggy_unit *peggy_match(struct accts777_info *accts,int32_t peg,uint64_t nxt64bits,bits256 lockhash,uint16_t lockdays)
{
    int32_t i,size; struct peggy_unit *U;
    if ( accts->hashaddrs == 0 )
    {
        for (i=0,U=&accts->units[0]; i<accts->numunits; i++,U++)
        {
            //if ( U->nxt64bits == 0 || U->nxt64bits == nxt64bits )
            {
                if ( U->lock.peg == peg && lockdays >= U->lock.minlockdays && lockdays <= U->lock.maxlockdays )
                {
                    if ( memcmp(lockhash.bytes,U->lockhash.bytes,sizeof(lockhash)) == 0 )
                        return(U);
                    return(0);
                }
            }
        }
    }
    else
    {
        size = sizeof(*U);
        if ( (U= ramkv777_read(&size,accts->hashaddrs,lockhash.bytes)) != 0 && size == sizeof(U) )
        {
            //if ( U->nxt64bits == 0 || U->nxt64bits == nxt64bits )
            {
                if ( U->lock.peg == peg && lockdays >= U->lock.minlockdays && lockdays <= U->lock.maxlockdays )
                    return(U);
            }
        }
    }
    return(0);
}

int32_t peggy_swap(struct accts777_info *accts,uint64_t signerA,uint64_t signerB,bits256 hashA,bits256 hashB)
{
    struct peggy_unit *U,*U2; int32_t size; uint64_t nxtA,nxtB;
    size = sizeof(*U);
    if ( (U= ramkv777_read(&size,accts->hashaddrs,hashA.bytes)) != 0 && size == sizeof(U) )
    {
        if ( (U2= ramkv777_read(&size,accts->hashaddrs,hashB.bytes)) != 0 && size == sizeof(U2) )
        {
            nxtA = acct777_nxt64bits(hashA), nxtB = acct777_nxt64bits(hashB);
            if ( (nxtA == signerA && nxtB == signerB) || (nxtA == signerB && nxtB == signerA) )
            {
                // need to verify ownership
                U2->lockhash = hashA, U->lockhash = hashB;
                return(0);
            }
        }
    }
    return(-1);
}

int32_t acct777_pay(struct accts777_info *accts,struct acct777 *srcacct,struct acct777 *acct,int64_t value,uint32_t blocknum,uint32_t blocktimestamp)
{
    if ( srcacct != 0 )
    {
        if ( srcacct->balance < value )
            return(-1);
        srcacct->balance -= value;
    }
    acct->balance += value;
    return(0);
}

int64_t acct777_balance(struct accts777_info *accts,uint32_t blocknum,uint32_t blocktimestamp,union peggy_addr *addr,int32_t type)
{
    int64_t balance = 0;
    struct acct777 *acct;
    if ( (acct= accts777_find(0,accts,addr,type)) != 0 )
        balance = acct->balance;
    return(balance);
}

int32_t peggy_flush(void *_PEGS,uint32_t currentblocknum,uint32_t blocknum,uint32_t blocktimestamp)
{
    struct peggy_info *PEGS = _PEGS;
    if ( PEGS != 0 && PEGS->accts != 0 )
        return(txinds777_flush(PEGS->accts->txinds,blocknum,blocktimestamp));
    else return(-1);
}

int32_t peggy_payments(queue_t *PaymentsQ,struct opreturn_payment *payments,int32_t max,uint32_t currentblocknum,uint32_t blocknum,uint32_t blocktimestamp)
{
    struct opreturn_payment *payment; int32_t n = 0;
    while ( max > 0 && (payment= queue_dequeue(PaymentsQ,0)) != 0 )
    {
        if ( payment->value != 0 && payment->coinaddr[0] != 0 )
            *payments++ = *payment;
        free(payment);
        n++;
    }
    return(n);
}

int32_t peggy_emit(void *_PEGS,uint8_t opreturndata[MAX_OPRETURNSIZE],struct opreturn_payment *payments,int32_t max,uint32_t currentblocknum,uint32_t blocknum,uint32_t blocktimestamp)
{
    char *opreturnstr; int32_t nonz,len = 0; struct peggy_info *PEGS = _PEGS;
    if ( payments != 0 && max > 1 && PEGS->accts != 0 && peggy_payments(&PEGS->accts->PaymentsQ,payments,max,currentblocknum,blocknum,blocktimestamp) < 0 )
        return(-1);
    if ( opreturndata != 0 && (opreturnstr= peggy_emitprices(&nonz,PEGS,blocknum,blocktimestamp,0)) != 0 )
    {
        memset(opreturndata,0,MAX_OPRETURNSIZE);
        len = (int32_t)strlen(opreturnstr) / 2;
        decode_hex(opreturndata,len,opreturnstr);
        free(opreturnstr);
    }
    return(len);
}

uint32_t peggy_clone(char *path,void *dest,void *src)
{
    struct peggy_info *destPEGS,*srcPEGS;
    destPEGS = dest, srcPEGS = src;
    *destPEGS = *srcPEGS;
    destPEGS->accts = accts777_clone(path,srcPEGS->accts);
    return(0);
}
#endif
#endif


