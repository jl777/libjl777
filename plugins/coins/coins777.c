//
//  coins777.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_coins777_h
#define crypto777_coins777_h
#include <stdio.h>
#include "uthash.h"
#include "cJSON.h"
#include "huffstream.c"
#include "system777.c"
#include "storage.c"
#include "db777.c"
#include "ramcoder.c"
#include "utils777.c"
#include "gen1pub.c"

#define OP_RETURN_OPCODE 0x6a
#define RAMCHAIN_PTRSBUNDLE 4096

#define MAX_BLOCKTX 0xffff
struct rawvin { char txidstr[128]; uint16_t vout; };
struct rawvout { char coinaddr[128],script[1024]; uint64_t value; };
struct rawtx { uint16_t firstvin,numvins,firstvout,numvouts; char txidstr[128]; };

struct rawblock
{
    uint32_t blocknum,allocsize;
    uint16_t format,numtx,numrawvins,numrawvouts;
    uint64_t minted;
    struct rawtx txspace[MAX_BLOCKTX];
    struct rawvin vinspace[MAX_BLOCKTX];
    struct rawvout voutspace[MAX_BLOCKTX];
};

#define MAX_COINTX_INPUTS 16
#define MAX_COINTX_OUTPUTS 8
struct cointx_input { struct rawvin tx; char coinaddr[64],sigs[1024]; uint64_t value; uint32_t sequence; char used; };
struct cointx_info
{
    uint32_t crc; // MUST be first
    char coinstr[16];
    uint64_t inputsum,amount,change,redeemtxid;
    uint32_t allocsize,batchsize,batchcrc,gatewayid,isallocated;
    // bitcoin tx order
    uint32_t version,timestamp,numinputs;
    uint32_t numoutputs;
    struct cointx_input inputs[MAX_COINTX_INPUTS];
    struct rawvout outputs[MAX_COINTX_OUTPUTS];
    uint32_t nlocktime;
    // end bitcoin txcalc_nxt64bits
    char signedtx[];
};

#define value_coders8 (&coders[0])
//#define minted_coders8 (&coders[8])
#define txidind_coders4 (&coders[8])
#define scriptind_coders4 (&coders[12])
#define addrind_coders4 (&coders[16])
#define blocknum_coders4 (&coders[20])
#define vout_coders2 (&coders[24])
#define numvin_coders2 (&coders[26])
#define numvout_coders2 (&coders[28])
#define numtx_coders2 (&coders[30])
#define TOTAL_CODERS (32)

struct address_entry { uint32_t rawind:31,spent:1,blocknum,txind:15,vinflag:1,v:14,isinternal:1; };

struct ramchain_hashtable
{
    struct db777 *DB;
    char name[32];
    uint32_t ind,maxind,needreverse,type,minblocknum;
    //void **ptrs;
};

struct ramchain
{
    char name[16];
    double lastgetinfo,startmilli;
    uint32_t startblocknum,RTblocknum,blocknum,confirmednum,huffallocsize,numupdates,numDBs,readyflag;
    struct ramchain_hashtable blocks,addrs,txids,scripts,unspents,*DBs[100];
    uint8_t *huffbits,*huffbits2;
    struct rawblock EMIT,DECODE;
};

struct coin777
{
    char name[16],serverport[64],userpass[128],*jsonstr;
    cJSON *argjson,*acctpubkeyjson;
    struct ramchain ramchain;
    int32_t use_addmultisig,gatewayid,multisigchar;
};

struct block_output { uint16_t crc16,numtx,totalspends,totalvouts; uint32_t blocknum,first_txidind,first_unspentind,allocsize; };
struct txid_output { struct address_entry B; uint32_t first_unspentind; uint16_t numspends,numvouts; }; // _ key, txid_spendinds, txid_unspentinds
struct coinaddr_output { struct address_entry B; uint32_t num,unspentinds[]; };

struct unspent_output
{
    struct address_entry B;
    uint64_t value;
    uint32_t addrind,scriptind,txidind,spend_txidind;
    uint16_t vout,spend_vout;
    uint8_t tbd3:8,tbd4:5,ismine:1,ismsig:1,pending:1;
};
struct unspent_entry { uint32_t unspentind; uint64_t amount; };
struct unspent_entries { UT_hash_handle hh; uint32_t addrind,max,count; struct unspent_entry *entries; };

char *bitcoind_RPC(char **retstrp,char *debugstr,char *url,char *userpass,char *command,char *params);
char *bitcoind_passthru(char *coinstr,char *serverport,char *userpass,char *method,char *params);
struct coin777 *coin777_create(char *coinstr,char *serverport,char *userpass,cJSON *argjson);
int32_t coin777_close(char *coinstr);
struct coin777 *coin777_find(char *coinstr);
char *extract_userpass(char *userhome,char *coindir,char *confname);
void ramchain_update(struct coin777 *coin);
int32_t rawblock_load(struct rawblock *raw,char *coinstr,char *serverport,char *userpass,uint32_t blocknum);
void rawblock_patch(struct rawblock *raw);

#endif
#else
#ifndef crypto777_coins777_c
#define crypto777_coins777_c

#ifndef crypto777_coins777_h
#define DEFINES_ONLY
#include "coins777.c"
#endif

uint32_t ramchain_inithash(struct ramchain_hashtable *hash)
{
    void *key,**ptrs; struct address_entry *bp; uint8_t *setbits;
    uint32_t maxblocknum,maxrawind,rawind,blocknum; int32_t keylen,i,n,allocsize = 10000000;
    if ( hash == 0 || hash->DB == 0 || hash->DB->db == 0 )
        return(0);
    if ( strcmp(hash->name,"blocks") == 0 )
    {
        for (rawind=maxblocknum=1; rawind<0xffffffff; rawind++) // start from verified key
        {
            blocknum = 0;
            if ( (key= db777_findM((int32_t *)&keylen,hash->DB,&rawind,sizeof(rawind))) != 0 )
            {
                free(key);
                maxblocknum = rawind;
            } else break;
        }
        return(maxblocknum);
    }
    if ( (ptrs= db777_copy_all(&n,hash->DB,"key",sizeof(*bp))) != 0 )
    {
        setbits = calloc(1,allocsize);
        for (i=maxblocknum=maxrawind=0; i<n; i++)
        {
            if ( (bp= ptrs[i]) != 0 )
            {
                if ( bp->rawind > maxrawind )
                    maxrawind = bp->rawind;
                if ( bp->blocknum > maxblocknum )
                    maxblocknum = bp->blocknum;
                if ( bp->rawind < (allocsize << 3) )
                    SETBIT(setbits,bp->rawind);
                free(ptrs[i]);
            }
        }
        free(ptrs);
        for (i=1; i<maxrawind; i++)
            if ( GETBIT(setbits,i) == 0 )
                break;
        printf("%s maxrawind.%d i.%d maxblocknum.%d\n",hash->name,maxrawind,i,maxblocknum);
        free(setbits);
        hash->ind = hash->maxind = (i - 1);
        if ( (ptrs= db777_copy_all(&n,hash->DB,"key",sizeof(*bp))) != 0 )
        {
            for (i=0; i<n; i++)
            {
                if ( (bp= ptrs[i]) != 0 )
                {
                    if ( bp->rawind == hash->maxind )
                    {
                        if ( bp->blocknum < maxblocknum )
                            maxblocknum = bp->blocknum;
                    }
                    free(ptrs[i]);
                }
            }
            free(ptrs);
        }
        printf("%s NEW maxblocknum.%d\n",hash->name,maxblocknum);
        return(maxblocknum);
    }
    return(0);
}

uint32_t ramchain_setblocknum(struct ramchain_hashtable *hash,uint32_t blocknum)
{
    int32_t i,n,numpurged,keylen; void **ptrs,*key; struct address_entry *bp; uint32_t rawind;
    if ( strcmp(hash->name,"blocks") == 0 )
    {
        for (rawind=blocknum; rawind<blocknum+10000; rawind++)
            db777_delete(hash->DB,&rawind,sizeof(rawind));
        hash->ind = hash->maxind = blocknum-1;
        return(hash->ind);
    }
    hash->maxind = numpurged = 0;
    if ( (ptrs= db777_copy_all(&n,hash->DB,"key",sizeof(*bp))) != 0 )
    {
        for (i=0; i<n; i++)
        {
            if ( (bp= ptrs[i]) != 0 )
            {
                if ( bp->blocknum <= blocknum )
                {
                    if ( bp->rawind > hash->maxind )
                        hash->maxind = bp->rawind;
                }
                else
                {
                    if ( db777_delete(hash->DB,bp,sizeof(*bp)) != 0 )
                        printf("error deleting blocknum.%d when max.%d\n",bp->blocknum,blocknum);
                    numpurged++;
                }
                free(ptrs[i]);
            }
        }
        free(ptrs);
        printf("%s >>>>>>>>>>>>>>>>>>>>>>>> maxind.%d -> %d, deleted.%d\n",hash->name,hash->ind,hash->maxind,numpurged);
    }
    hash->ind = hash->maxind;
    if ( (ptrs= db777_copy_all(&n,hash->DB,"key",sizeof(uint32_t))) != 0 )
    {
        for (i=0; i<n; i++)
        {
            if ( ptrs[i] != 0 )
            {
                if ( (rawind = *(uint32_t *)ptrs[i]) > hash->maxind )
                {
                    if ( (key= db777_findM((int32_t *)&keylen,hash->DB,&rawind,sizeof(rawind))) != 0 )
                    {
                        if ( db777_delete(hash->DB,key,keylen) != 0 )
                            printf("error deleting keylen.%d rawind.%d when maxind.%d\n",keylen,rawind,hash->maxind);
                        else numpurged++;
                        free(key);
                    }
                    if ( db777_delete(hash->DB,&rawind,sizeof(rawind)) != 0 )
                        printf("error deleting rawind.%d when maxind.%d\n",rawind,hash->maxind);
                    numpurged++;
                }
                free(ptrs[i]);
            }
        }
        free(ptrs);
        printf(">>>>>>>>>>>>>>>>>>>>>>>> purged %d\n",numpurged);
    }
    return(hash->ind);
}

struct coinaddr_output *update_coinaddr_output(int32_t *lenp,struct coinaddr_output *ptr,struct unspent_output *U)
{
    if ( U->B.spent != 0 )
    {
        
    }
    else
    {
        
    }
    if ( ptr == 0 )
    {
        *lenp = sizeof(*ptr) + sizeof(*ptr->unspentinds);
       // printf("allocsize %d\n",*lenp);
        ptr = calloc(1,*lenp);
        ptr->B = U->B;
        ptr->B.rawind = U->addrind;
    }
    else
    {
        *lenp = sizeof(*ptr) + ((ptr->num +1) * sizeof(*ptr->unspentinds));
       // printf("allocsize %d : num.%d\n",*lenp,ptr->num);
        ptr = realloc(ptr,*lenp);
    }
    //printf("%p[%u] <- unspentind.%u for coinaddr.%u\n",ptr,ptr->num,U->B.rawind,ptr->B.rawind);
    ptr->unspentinds[ptr->num++] = U->B.rawind;
    return(ptr);
}

int32_t coinaddr_update(struct ramchain *ram,struct unspent_output *U)
{
    /*int32_t err,err2; void *ptr; int32_t len,valuelen;
    ptr = db777_findM(&len,coinaddrs->DB,&U->addrind,sizeof(U->addrind));
    ptr = update_coinaddr_output(&valuelen,ptr,U);
    err = db777_add(0,coinaddrs->DB,&U->addrind,sizeof(U->addrind),ptr,valuelen);
    free(ptr);
    err2 = db777_add(0,coinaddrs->DB,&U->B,sizeof(U->B),&U->addrind,sizeof(U->addrind));
    if ( err != 0 || err2 != 0 )
    {
        printf("error saving unspentind.%d: %d %d %p\n",U->addrind,err,err2,coinaddrs->DB->db);
        return(-1);
    }*/
    return(0);
}

int32_t unspent_add(struct ramchain *ram,struct ramchain_hashtable *unspents,struct unspent_output *U)
{
    int32_t err,err2; void *ptr; int32_t len; uint32_t rawind = *(uint32_t *)U;
    if ( U->B.rawind > unspents->maxind )
        unspents->maxind = U->B.rawind;
    if ( (ptr= db777_findM(&len,unspents->DB,&rawind,sizeof(rawind))) != 0 )
    {
        free(ptr);
        return(0);
    }
    err = db777_add(0,unspents->DB,&rawind,sizeof(rawind),U,sizeof(*U));
    err2 = db777_add(0,unspents->DB,&U->B,sizeof(U->B),&rawind,sizeof(rawind));
    coinaddr_update(ram,U);
    if ( err != 0 || err2 != 0 )
    {
        printf("error saving unspentind.%d: %d %d %p\n",U->B.rawind,err,err2,unspents->DB->db);
        return(-1);
    }
    return(0);
}

int32_t txid_add(struct ramchain_hashtable *txids,struct txid_output *txid,int32_t allocsize,uint8_t *key,int32_t keylen)
{
    int32_t err,err2,err3; void *ptr; int32_t len; uint32_t rawind = txid->B.rawind;
    if ( (ptr= db777_findM(&len,txids->DB,&rawind,sizeof(rawind))) != 0 )
    {
        free(ptr);
        return(0);
    }
    if ( rawind > txids->maxind )
        txids->maxind = rawind;
    err = db777_add(0,txids->DB,key,keylen,&txid->B,sizeof(txid->B));
    if ( txids->needreverse > 0 )
    {
        err2 = db777_add(0,txids->DB,&rawind,sizeof(rawind),txid,allocsize);
        err3 = db777_add(0,txids->DB,&txid->B,sizeof(txid->B),&rawind,sizeof(rawind));
        if ( err != 0 || err2 != 0 || err3 != 0 )
        {
            printf("error saving txid.%d\n",txid->B.rawind);
            return(-1);
        }
    }
    return(0);
}

int32_t ramchain_setitem(struct ramchain_hashtable *hash,void *key,int32_t keylen,struct address_entry *B) // 'a' and 's'
{
    int32_t err,err2,err3; uint32_t rawind = B->rawind;
    if ( rawind > hash->maxind )
        hash->maxind = rawind;
    if ( 0 && strcmp(hash->name,"rawaddrs") == 0 )
        printf("CREATE[%d] <- (%s)\n",rawind,key);
    err = db777_add(0,hash->DB,key,keylen,B,sizeof(*B));
    if ( hash->needreverse > 0 )
    {
        err2 = db777_add(0,hash->DB,&rawind,sizeof(rawind),key,keylen);
        err3 = db777_add(0,hash->DB,B,sizeof(*B),&rawind,sizeof(rawind));
        if ( err != 0 || err2 != 0 || err3 != 0 )
        {
            printf("error saving ramchain_setitem.%d (%c) blocknum.%d\n",rawind,hash->type,B->blocknum);
            return(-1);
        }
    }
    return(0);
}

uint32_t incr_rawind(struct ramchain_hashtable *hash)
{
    uint32_t rawind;
    rawind = ++hash->ind;
    if ( hash->ind > hash->maxind )
        hash->maxind = hash->ind;
    return(rawind);
}

int32_t hashstr_key(char *hashstr,uint8_t *key,int32_t maxlen)
{
    int32_t keylen = 0;
    keylen = (int32_t)strlen(hashstr) >> 1;
    if ( keylen < 0xfd )
        decode_hex(key+1,keylen,hashstr), key[0] = keylen, keylen++;
    else if ( keylen < maxlen-4 )
        decode_hex(key+3,keylen,hashstr), key[0] = 0xfd, key[1] = (keylen & 0xff), key[2] = (keylen>>8) & 0xff, keylen += 3;
    else return(0);
    return(keylen);
}

int32_t hashstr_decode(char *hashstr,uint8_t *key)
{
    int32_t len;
    if ( (len= key[0]) < 0xfd )
    {
        init_hexbytes_noT(hashstr,key+1,len);
        return(0);
    }
    printf("only short varints on decode\n");
    return(-1);
}

uint32_t ramchain_rawind(struct ramchain_hashtable *hash,char *hashstr,int32_t createflag,struct address_entry *B)
{
    uint8_t keydata[8192]; void *key=0; int32_t keylen,len; struct address_entry *bp; uint32_t rawind = 0;
    if ( hashstr == 0 || hashstr[0] == 0 )
    {
        printf("ramchain_rawind: null hashstr\n");
        return(0);
    }
    if ( strcmp(hash->name,"scripts") == 0 )
        keylen = hashstr_key(hashstr,keydata,sizeof(keydata)), key = keydata;
    else if ( strcmp(hash->name,"rawaddrs") == 0 )
        key = hashstr, keylen = (int32_t)strlen(key) + 1;
    else
    {
        printf("ramchain_rawind: unsupported type.(%c)\n",hash->type);
        return(0);
    }
    if ( (bp= db777_findM(&len,hash->DB,key,keylen)) != 0 )
    {
        rawind = bp->rawind;
        free(bp);
        return(rawind);
    }
    if ( createflag != 0 )
    {
        B->rawind = incr_rawind(hash);
        if ( ramchain_setitem(hash,key,keylen,B) == 0 )
            return(B->rawind);
    }
    return(0);
}

uint16_t block_crc16(struct block_output *block)
{
    uint32_t crc32 = _crc32(0,(void *)((long)&block->crc16 + sizeof(block->crc16)),block->allocsize - sizeof(block->crc16));
    return((crc32 >> 16) ^ (crc32 & 0xffff));
}

uint32_t get_firstinds(uint32_t *first_unspentindp,struct ramchain_hashtable *blocks,uint32_t blocknum)
{
    struct block_output *block; int32_t size,first_txidind = 0;
    if ( blocknum == 1 )
    {
        *first_unspentindp = 1000;
        return(*first_unspentindp);
    }
    else if ( blocknum == 0 )
    {
        *first_unspentindp = 1;
        return(1);
    }
    blocknum--;
    if ( (block= db777_findM(&size,blocks->DB,&blocknum,sizeof(blocknum))) != 0 )
    {
        if ( size == block->allocsize && block_crc16(block) == block->crc16 )
        {
            first_txidind = (block->first_txidind + block->numtx);
            *first_unspentindp = (block->first_unspentind + block->totalvouts);
        }
        else printf("error size.%d vs %d, crc16 %d vs %d\n",size,block->allocsize,block_crc16(block),block->crc16);
        free(block);
    } else printf("get_firstinds: cant find blocknum.%d\n",blocknum);
    return(first_txidind);
}

uint32_t txid_rawind(struct ramchain_hashtable *txids,char *txidstr)
{
    uint8_t keydata[8192]; int32_t keylen,len; struct address_entry *bp; uint32_t txidind = 0;
    keylen = hashstr_key(txidstr,keydata,sizeof(keydata));
    if ( (bp= db777_findM(&len,txids->DB,keydata,keylen)) != 0 )
        txidind = bp->rawind, free(bp);
   // printf("txid.(%s) -> %u\n",txidstr,txidind);
    return(txidind);
}

void addr_decodestr(char *coinaddr,struct ramchain_hashtable *addrs,uint32_t addrind)
{
    void *value; int32_t len;
    coinaddr[0] = 0;
    if ( addrind > 0 && addrind <= addrs->maxind && (value= db777_findM(&len,addrs->DB,&addrind,sizeof(addrind))) != 0 )
        memcpy(coinaddr,value,len), free(value);
    //printf("addr.%u -> (%s)\n",addrind,coinaddr);
}

void script_decodestr(char *script,struct ramchain_hashtable *scripts,uint32_t scriptind)
{
    void *value; int32_t len;
    script[0] = 0;
    if ( scriptind > 0 && scriptind <= scripts->maxind && (value= db777_findM(&len,scripts->DB,&scriptind,sizeof(scriptind))) != 0 )
        hashstr_decode(script,value), free(value);
    //printf("script.%u -> (%s)\n",scriptind,script);
}

void txid_decodestr(char *txidstr,struct ramchain_hashtable *txids,uint32_t txidind)
{
    struct txid_output *txid = 0; int32_t len;
    txidstr[0] = 0;
    if ( txidind > 0 && txidind <= txids->maxind && (txid= db777_findM(&len,txids->DB,&txidind,sizeof(txidind))) != 0 )
        hashstr_decode(txidstr,(uint8_t *)((long)txid + sizeof(*txid))), free(txid);
    //printf("%u -> tx.(%s)\n",txidind,txidstr);
}

int32_t txid_decode_tx(struct rawtx *tx,struct ramchain_hashtable *txids,uint32_t txidind)
{
    struct txid_output *txid = 0; int32_t len; uint8_t *key;
    if ( txidind > 0 && txidind <= txids->maxind && (txid= db777_findM(&len,txids->DB,&txidind,sizeof(txidind))) != 0 )
    {
        tx->numvouts = txid->numvouts;
        tx->numvins = txid->numspends;
        key = (uint8_t *)((long)txid + sizeof(*txid));
        hashstr_decode(tx->txidstr,key);
        //printf("decoded.(%s).%d len.%d numvouts.%d numvins.%d\n",tx->txidstr,*key,len,tx->numvouts,tx->numvins);
        free(txid);
        return(0);
    } else printf("illegal txidind.%d vs max.%d or no txid.%p\n",txidind,txids->maxind,txid);
    return(-1);
}

int32_t txid_create(struct alloc_space *mem,struct ramchain_hashtable *txids,char *txidstr,uint32_t txidind,uint32_t *spendinds,int16_t numspends,uint32_t first_unspentind,struct unspent_output *unspents,uint16_t numvouts,struct address_entry *B)
{
    uint8_t keydata[8192],*txid_key; int32_t i,keylen; struct txid_output *txid; uint32_t *txid_spendinds,*txid_unspentinds;
    keylen = hashstr_key(txidstr,keydata,sizeof(keydata));
    mem->used = 0, mem->alignflag = 0;
    txid = memalloc(mem,sizeof(*txid));
    txid_key = memalloc(mem,keylen);
    txid_spendinds = memalloc(mem,sizeof(*txid_spendinds) * numspends);
    txid_unspentinds = memalloc(mem,sizeof(*txid_unspentinds) * numvouts);
    memset(txid,0,mem->used);
    txid->B = *B, txid->B.rawind = txidind;
    if ( txidind > txids->maxind )
        txids->maxind = txidind;
    txid->numspends = numspends, txid->numvouts = numvouts, txid->first_unspentind = first_unspentind;
    memcpy(txid_key,keydata,keylen);
    memcpy(txid_spendinds,spendinds,sizeof(*txid_spendinds) * numspends);
    for (i=0; i<numvouts; i++)
        txid_unspentinds[i] = unspents[i].B.rawind;
    //printf("txidind.%u txid_create.(%s).%d %u numspends.%d numvouts.%d 1stunspent.%u\n",txidind,txidstr,*txid_key,txidind,numspends,numvouts,first_unspentind);
    return(txid_add(txids,txid,(int32_t)mem->used,keydata,keylen));
}

int32_t unspent_decode_vo(struct ramchain *ram,struct rawvout *vo,struct unspent_output U)
{
    addr_decodestr(vo->coinaddr,&ram->addrs,U.addrind);
    script_decodestr(vo->script,&ram->scripts,U.scriptind);
    vo->value = U.value;
    //printf("(%s %s %.8f) ",vo->coinaddr,vo->script,dstr(vo->value));
    return(0);
}

int32_t unspent_create(struct ramchain *ram,struct unspent_output *U,struct ramchain_hashtable *unspents,uint32_t unspentind,struct ramchain_hashtable *addrs,char *coinaddr,struct ramchain_hashtable *scripts,char *script,uint64_t value,uint32_t txidind,uint16_t vout,struct address_entry *B)
{
    memset(U,0,sizeof(*U));
    B->rawind = 0, B->spent = 0;
    U->txidind = txidind, U->vout = vout;
    U->addrind = ramchain_rawind(addrs,coinaddr,1,B);
    U->scriptind = ramchain_rawind(scripts,script,1,B);
    U->value = value;
    U->B = *B, U->B.rawind = unspentind;
    if ( unspentind > unspents->maxind )
        unspents->maxind = unspentind;
    //printf("create unspentind.%u txidind.%-6u.v%d addrind.%-6u scriptind.%-6u %.8f\n",unspentind,txidind,vout,U->addrind,U->scriptind,dstr(value));
    return(unspent_add(ram,unspents,U));
 }

int32_t unspent_find(struct unspent_output *U,struct ramchain_hashtable *unspents,uint32_t unspentind)
{
    struct unspent_output *up; int32_t len;
    if ( (up= db777_findM(&len,unspents->DB,&unspentind,sizeof(unspentind))) != 0 )
    {
        *U = *up;
        //printf("found unspentind.%u addrind.%u txidind.%d %.8f | spendtxind.%u v.%d\n",up->B.rawind,up->addrind,up->txidind,dstr(up->value),up->spend_txidind,up->spend_vout);
        free(up);
        return(0);
    }
    else
    {
        unspentind |= (1 << 31);
        if ( (up= db777_findM(&len,unspents->DB,&unspentind,sizeof(unspentind))) != 0 )
        {
            *U = *up;
            printf("found SPENT unspentind.%u addrind.%u txidind.%d %.8f | spendtxind.%u v.%d\n",up->B.rawind,up->addrind,up->txidind,dstr(up->value),up->spend_txidind,up->spend_vout);
            free(up);
            return(0);
        }
    }
    return(-1);
}

uint32_t unspent_spend(struct ramchain *ram,uint32_t txidind,uint16_t vout,uint32_t spend_txidind,struct address_entry *B) // UPDATE addr balance
{
    struct txid_output *txid = 0; int32_t len; struct unspent_output U; uint32_t unspentind = 0;
    if ( (txid= db777_findM(&len,ram->txids.DB,&txidind,sizeof(txidind))) != 0 )
    {
        unspentind = txid->first_unspentind + vout;
        if ( unspent_find(&U,&ram->unspents,unspentind) == 0 )
        {
            U.spend_txidind = spend_txidind, U.spend_vout = B->v, U.B.spent = 1;
            if ( unspent_add(ram,&ram->unspents,&U) != 0 )
                printf("warning: couldnt update unspent\n");
            else
            {
                if ( unspent_find(&U,&ram->unspents,unspentind) == 0 )
                    printf("after spend %u: spent.%d txidind.%u v%d %.8f -> addrind.%u | spend_txidind.%u v%d\n",U.B.rawind,U.B.spent,U.spend_txidind,U.spend_vout,dstr(U.value),U.addrind,spend_txidind,B->v);
            }
        } else printf("cant find unspendind.%d\n",unspentind);
        free(txid);
    } else printf("cant find txidind.%d\n",txidind);
    return(unspentind);
}

int32_t unspent_decode_vi(struct ramchain *ram,struct rawvin *vi,uint32_t unspentind)
{
    struct unspent_output U;
    if ( unspent_find(&U,&ram->unspents,unspentind) == 0 )
    {
        txid_decodestr(vi->txidstr,&ram->txids,U.txidind);
        vi->vout = U.vout;
        return(0);
    }
    return(-1);
}

struct block_output *ramchain_emit(struct ramchain *ram,struct alloc_space *mem,struct rawtx *tx,uint16_t numtx,struct rawvin *vi,struct rawvout *vo,struct address_entry *bp)
{
    struct block_output *block; struct unspent_output *unspents; uint32_t *spendinds; uint16_t *offsets; struct alloc_space MEM;
    uint32_t i,totalspends,totalvouts,txidind,unspentind; uint16_t voutoffset,spendoffset,n;
    for (i=totalspends=totalvouts=0; i<numtx; i++)
        totalspends += tx[i].numvins, totalvouts += tx[i].numvouts;
    mem->used = 0, mem->alignflag = 0;
    block = memalloc(mem,sizeof(struct block_output));
    offsets = memalloc(mem,sizeof(*offsets) * (numtx+1) * 2);
    spendinds = memalloc(mem,sizeof(*spendinds) * totalspends);
    unspents = memalloc(mem,sizeof(*unspents) * totalvouts);
    memset(block,0,mem->used);
    block->allocsize = (uint32_t)mem->used;
    block->blocknum = bp->blocknum;
    if ( (block->first_txidind= get_firstinds(&block->first_unspentind,&ram->blocks,bp->blocknum)) == 0 )
    {
        printf("illegal first_txidind.%d\n",block->first_txidind);
        return(0);
    }
    block->numtx = numtx, block->totalspends = totalspends, block->totalvouts = totalvouts;
    if ( numtx > 0 )
    {
        memset(&MEM,0,sizeof(MEM)); MEM.ptr = ram->huffbits2; MEM.size = ram->huffallocsize;
        unspentind = block->first_unspentind;
        //printf("block.%u 1st unspent.%u 1st txidind.%d: numtx.%d total spends.%d vouts.%d\n",block->blocknum,block->first_unspentind,block->first_txidind,block->numtx,block->totalspends,block->totalvouts);
        for (bp->txind=voutoffset=spendoffset=0; bp->txind<numtx; bp->txind++,tx++)
        {
            offsets[(bp->txind << 1) + 0] = spendoffset, offsets[(bp->txind << 1) + 1] = voutoffset;
            if ( (n= tx->numvins) > 0 )
            {
                bp->vinflag = 1;
                for (bp->v=0; bp->v<n; bp->v++,vi++)
                {
                    if ( (txidind= txid_rawind(&ram->txids,vi->txidstr)) == 0 )
                    {
                        printf("cant decode txidstr.(%s)\n",vi->txidstr);
                        return(0);
                    }
                   // printf("(%u %d) ",txidind,vi->vout);
                    spendinds[spendoffset++] = unspent_spend(ram,txidind,vi->vout,(block->first_txidind + bp->txind),bp);
                }
            }
            txidind = (block->first_txidind + bp->txind);
            if ( (n= tx->numvouts) > 0 )
            {
                bp->vinflag = 0;
                for (bp->v=0; bp->v<n; bp->v++,vo++)
                {
                    unspent_create(ram,&unspents[voutoffset++],&ram->unspents,unspentind + bp->v,&ram->addrs,vo->coinaddr,&ram->scripts,vo->script,vo->value,txidind,bp->v,bp);
                }
            }
            txid_create(&MEM,&ram->txids,tx->txidstr,txidind,&spendinds[offsets[(bp->txind << 1) + 0]],tx->numvins,unspentind,&unspents[offsets[(bp->txind << 1) + 1]],tx->numvouts,bp);
            unspentind += tx->numvouts;
        }
        offsets[(numtx << 1) + 0] = spendoffset, offsets[(numtx << 1) + 1] = voutoffset;
    }
    block->crc16 = block_crc16(block);
    //printf("created block.%u crc16.%d\n",block->blocknum,block->crc16);
    return(block);
}

int32_t ramchain_decode(struct ramchain *ram,struct alloc_space *mem,struct block_output *block,struct rawtx *tx,struct rawvin *vi,struct rawvout *vo,struct address_entry *bp)
{
    struct unspent_output *unspents; uint32_t *spendinds; uint16_t *offsets;
    uint32_t checksize; uint16_t voutoffset,spendoffset,n;
    offsets = (void *)((long)block + sizeof(struct block_output));
    spendinds = (void *)((long)offsets + sizeof(*offsets) * (block->numtx+1) * 2);
    unspents = (void *)((long)spendinds + sizeof(*spendinds) * block->totalspends);
    checksize = (uint32_t)((long)unspents + (sizeof(*unspents) * block->totalvouts) - (long)block);
    if ( bp->blocknum == block->blocknum && checksize == block->allocsize && block_crc16(block) == block->crc16 )
    {
        if ( block->numtx > 0 )
        {
            for (bp->txind=voutoffset=spendoffset=0; bp->txind<block->numtx; bp->txind++,tx++)
            {
                //printf("txind.%d ",bp->txind);
                if ( spendoffset != offsets[(bp->txind << 1) + 0] || voutoffset != offsets[(bp->txind << 1) + 1] )
                {
                    printf("offset mismatch %s block.%d txind.%d (%d %d %d %d)\n",ram->name,bp->blocknum,bp->txind,spendoffset,offsets[(bp->txind << 1) + 0],voutoffset,offsets[(bp->txind << 1) + 1]);
                }
                if ( (n= (offsets[((bp->txind+1) << 1) + 0] - offsets[(bp->txind << 1) + 0])) > 0 )
                {
                   // printf("numvins.%d ",n);
                    bp->vinflag = 1;
                    for (bp->v=0; bp->v<n; bp->v++,vi++)
                        unspent_decode_vi(ram,vi,spendinds[spendoffset++]);
                }
                if ( (n= (offsets[((bp->txind+1) << 1) + 1] - offsets[(bp->txind << 1) + 1])) > 0 )
                {
                    //printf("numvouts.%d ",n);
                    bp->vinflag = 0;
                    for (bp->v=0; bp->v<n; bp->v++,vo++)
                        unspent_decode_vo(ram,vo,unspents[voutoffset++]);
                }
                txid_decode_tx(tx,&ram->txids,block->first_txidind + bp->txind);
            }
        }
    } else printf("checksize error %d vs allocsize %d | crc16 %d vs %d\n",checksize,block->allocsize,block_crc16(block),block->crc16);
    return(block->numtx);
}

int32_t ramchain_rawblock(struct ramchain *ram,struct rawblock *raw,uint32_t blocknum,int32_t emitflag)
{
    struct address_entry B; struct alloc_space MEM;
    struct block_output *block,*checkblock;
    int32_t allocsize,err;
    memset(&B,0,sizeof(B)), B.blocknum = blocknum;
    if ( emitflag != 0 )
    {
        memset(&MEM,0,sizeof(MEM)); MEM.ptr = ram->huffbits; MEM.size = ram->huffallocsize;
        if ( (block= ramchain_emit(ram,&MEM,raw->txspace,raw->numtx,raw->vinspace,raw->voutspace,&B)) != 0 )
        {
            db777_delete(ram->blocks.DB,&blocknum,(int32_t)sizeof(blocknum));
            if ( (err= db777_add(0,ram->blocks.DB,&blocknum,(int32_t)sizeof(blocknum),(void *)block,block->allocsize)) != 0 )
                printf("err.%d adding blocknum.%u\n",err,blocknum);
            else
            {
                //printf("saved blocknum.%d size.%d\n",blocknum,block->allocsize);
                if ( (checkblock= db777_findM(&allocsize,ram->blocks.DB,&blocknum,sizeof(blocknum))) != 0 )
                {
                    if ( checkblock->allocsize != block->allocsize || memcmp(block,checkblock,block->allocsize) != 0 )
                        printf("validation error blocknum.%d: %d %d %d\n",blocknum,checkblock->allocsize,block->allocsize,memcmp(block,checkblock,block->allocsize));
                    free(checkblock);
                } else printf("cant find blocknum.%d\n",blocknum);
            }
            return(block->allocsize);
        } else printf("error ramchain_emit %s.%u\n",ram->name,blocknum), getchar();
    }
    else
    {
        if ( (block= db777_findM(&allocsize,ram->blocks.DB,&blocknum,sizeof(blocknum))) != 0 )
        {
            if ( allocsize == block->allocsize )
            {
                memset(&MEM,0,sizeof(MEM)); MEM.ptr = block; MEM.size = block->allocsize;
                memset(raw,0,sizeof(*raw));
                raw->blocknum = blocknum;
                raw->numtx = ramchain_decode(ram,&MEM,block,raw->txspace,raw->vinspace,raw->voutspace,&B);
                //printf("loaded blocknum.%d numtx.%d\n",blocknum,raw->numtx);
            } else allocsize = 0, printf("allocsize mismatch %s %u %d vs %d\n",ram->name,blocknum,allocsize,block->allocsize);
            free(block);
            return(allocsize);
        } else printf("cant find %s block.%d\n",ram->name,blocknum);
    }
    return(0);
}

int32_t ramchain_processblock(struct coin777 *coin,uint32_t blocknum,uint32_t RTblocknum)
{
    struct ramchain *ram = &coin->ramchain;
    int32_t len;
    memset(&ram->EMIT,0,sizeof(ram->EMIT)), memset(&ram->DECODE,0,sizeof(ram->DECODE));
    if ( rawblock_load(&ram->EMIT,coin->name,coin->serverport,coin->userpass,blocknum) > 0 )
    {
        ram->RTblocknum = _get_RTheight(&ram->lastgetinfo,coin->name,coin->serverport,coin->userpass,ram->RTblocknum);
        len = ramchain_rawblock(ram,&ram->EMIT,blocknum,1), memset(ram->huffbits,0,ram->huffallocsize);
        ramchain_rawblock(ram,&ram->DECODE,blocknum,0);
        printf("%-4s [lag %-5d]    RTblock.%-6u    blocknum.%-6u  len.%-5d   minutes %.2f\n",coin->name,RTblocknum-blocknum,RTblocknum,blocknum,len,estimate_completion(ram->startmilli,blocknum-ram->startblocknum,RTblocknum-blocknum)/60000);
        rawblock_patch(&ram->EMIT), rawblock_patch(&ram->DECODE);
        ram->DECODE.minted = ram->EMIT.minted = 0;
        if ( (len= memcmp(&ram->EMIT,&ram->DECODE,sizeof(ram->EMIT))) != 0 )
        {
            int i,n = 0;
            for (i=0; i<sizeof(ram->DECODE); i++)
                if ( ((char *)&ram->EMIT)[i] != ((char *)&ram->DECODE)[i] )
                    printf("(%02x v %02x).%d ",((uint8_t *)&ram->EMIT)[i],((uint8_t *)&ram->DECODE)[i],i),n++;
            printf("COMPARE ERROR at %d | numdiffs.%d size.%ld\n",len,n,sizeof(ram->DECODE));
        }
        return(0);
    } else printf("ramchain_processblock error rawblock_load.%u\n",blocknum);
    return(-1);
}

int32_t ramchain_checkblock(struct coin777 *coin,uint32_t blocknum)
{
    struct ramchain *ram = &coin->ramchain;
    HUFF H;
    memset(&H,0,sizeof(H)), _init_HUFF(&H,ram->huffallocsize,ram->huffbits), hrewind(&H);
    if ( 1 || ramchain_rawblock(ram,&ram->EMIT,blocknum,0) <= 0 )
    {
        //if ( rawblock_load(&ram->RAW,coin->name,coin->serverport,coin->userpass,blocknum) > 0 )
        {
            
        }
    }
    return(-1);
}

void ramchain_syncDB(struct ramchain_hashtable *hash)
{
    //int32_t bval,cval;
    //bval = sp_set(hash->DB->ctl,"backup.run");
    //cval = sp_set(hash->DB->ctl,"scheduler.checkpoint");
    //printf("hash.(%c) bval.%d cval.%d\n",hash->type,bval,cval);
}

void ramchain_syncDBs(struct ramchain *ram)
{
    int32_t i;
    for (i=0; i<ram->numDBs; i++)
        ramchain_syncDB(ram->DBs[i]);
}

void ramchain_setblocknums(struct ramchain *ram,uint32_t minblocknum)
{
    int32_t i;
    for (i=0; i<ram->numDBs; i++)
        ramchain_setblocknum(ram->DBs[i],minblocknum);
}

uint32_t init_hashDBs(struct ramchain *ram,char *coinstr,struct ramchain_hashtable *hash,char *name,char *compression,int32_t numptrs)
{
    if ( hash->DB == 0 )
    {
        hash->DB = db777_create("ramchains",coinstr,name,compression);
        hash->type = name[0];
        strcpy(hash->name,name);
        hash->needreverse = numptrs;
        hash->minblocknum = ramchain_inithash(hash);
        ram->DBs[ram->numDBs++] = hash;
    }
    return(0);
}

void delete_unspent_entries(struct unspent_entries *unspents)
{
    struct unspent_entries *current,*tmp;
    HASH_ITER(hh,unspents,current,tmp)
    {
        HASH_DEL(unspents,current);
        if ( current->entries != 0 )
            free(current->entries);
        free(current);
    }
}

uint32_t ensure_ramchain_DBs(struct ramchain *ram)
{
    uint32_t i,j,minblocknum,numaddrs;  struct unspent_entries *unspents,*ptr; uint64_t sum,total;
    init_hashDBs(ram,ram->name,&ram->unspents,"unspents","lz4",RAMCHAIN_PTRSBUNDLE);
    if ( (unspents= db777_extract_unspents(&numaddrs,ram->unspents.DB)) != 0 )
    {
        total = i = 0;
        for (ptr=unspents; ptr!=NULL; ptr=ptr->hh.next,i++)
        {
            for (sum=j=0; j<ptr->count; j++)
                sum += ptr->entries[j].amount;
            total += sum;
            if ( ptr->count > 10 )
                printf("i.%d of %d: addrind.%u count.%d max.%d | %.8f total %.8f\n",i,numaddrs,ptr->addrind,ptr->count,ptr->max,dstr(sum),dstr(total));
        }
        delete_unspent_entries(unspents);
        printf("numaddrs.%d total %.8f\n",numaddrs,dstr(total));
    }

    init_hashDBs(ram,ram->name,&ram->blocks,"blocks","lz4",0);
    init_hashDBs(ram,ram->name,&ram->addrs,"rawaddrs","lz4",RAMCHAIN_PTRSBUNDLE);
    init_hashDBs(ram,ram->name,&ram->txids,"txids","lz4",RAMCHAIN_PTRSBUNDLE);
    init_hashDBs(ram,ram->name,&ram->scripts,"scripts","lz4",RAMCHAIN_PTRSBUNDLE);
    minblocknum = 0xffffffff;
    for (i=0; i<ram->numDBs; i++)
    {
        if ( ram->DBs[i]->minblocknum < minblocknum )
            minblocknum = ram->DBs[i]->minblocknum;
        printf("%u ",ram->DBs[i]->minblocknum);
    }
    printf("minblocknums -> %d\n",minblocknum);
    ramchain_setblocknums(ram,minblocknum);
    printf("finished setblocknums\n");
    return(minblocknum);
}

void ramchain_update(struct coin777 *coin)
{
    uint32_t blocknum;
    struct address_entry B;
    //printf("%s ramchain_update: ready.%d\n",coin->name,coin->ramchain.readyflag);
    if ( coin->ramchain.readyflag == 0 )
        return;
    if ( (blocknum= coin->ramchain.blocknum) < coin->ramchain.RTblocknum )
    {
        if ( blocknum == 0 )
            blocknum = 1;
        memset(&B,0,sizeof(B));
        B.blocknum = blocknum;
        if ( 0 && ramchain_checkblock(coin,blocknum) == 0 )
            coin->ramchain.blocknum++;
        else if ( ramchain_processblock(coin,blocknum,coin->ramchain.RTblocknum) == 0 )
        {
            coin->ramchain.blocknum++;
            if ( 0 && coin->ramchain.numupdates++ > 10000 )
            {
                ramchain_syncDBs(&coin->ramchain);
                printf("Start backups\n");// getchar();
                coin->ramchain.numupdates = 0;
            }
        }
        else printf("%s error processing block.%d\n",coin->name,B.blocknum);
    }
}

int32_t init_ramchain(struct coin777 *coin,char *coinstr)
{
    struct ramchain *ram = &coin->ramchain;
    ram->startmilli = milliseconds();
    strcpy(ram->name,coinstr);
    ram->blocknum = ram->startblocknum = ensure_ramchain_DBs(ram);
    ram->huffallocsize = sizeof(struct rawblock)/10, ram->huffbits = calloc(1,ram->huffallocsize), ram->huffbits2 = calloc(1,ram->huffallocsize);
    ram->RTblocknum = _get_RTheight(&ram->lastgetinfo,coinstr,coin->serverport,coin->userpass,ram->RTblocknum);
    ramchain_syncDBs(ram);
    coin->ramchain.readyflag = 1;
    return(0);
}

char *bitcoind_passthru(char *coinstr,char *serverport,char *userpass,char *method,char *params)
{
    return(bitcoind_RPC(0,coinstr,serverport,userpass,method,params));
}

char *parse_conf_line(char *line,char *field)
{
    line += strlen(field);
    for (; *line!='='&&*line!=0; line++)
        break;
    if ( *line == 0 )
        return(0);
    if ( *line == '=' )
        line++;
    stripstr(line,strlen(line));
    if ( Debuglevel > 0 )
        printf("[%s]\n",line);
    return(clonestr(line));
}

char *extract_userpass(char *userhome,char *coindir,char *confname)
{
    FILE *fp;
    char fname[2048],line[1024],userpass[1024],*rpcuser,*rpcpassword,*str;
    userpass[0] = 0;
    sprintf(fname,"%s/%s/%s",userhome,coindir,confname);
    if ( (fp= fopen(os_compatible_path(fname),"r")) != 0 )
    {
        if ( Debuglevel > 0 )
            printf("extract_userpass from (%s)\n",fname);
        rpcuser = rpcpassword = 0;
        while ( fgets(line,sizeof(line),fp) != 0 )
        {
            if ( line[0] == '#' )
                continue;
            //printf("line.(%s) %p %p\n",line,strstr(line,"rpcuser"),strstr(line,"rpcpassword"));
            if ( (str= strstr(line,"rpcuser")) != 0 )
                rpcuser = parse_conf_line(str,"rpcuser");
            else if ( (str= strstr(line,"rpcpassword")) != 0 )
                rpcpassword = parse_conf_line(str,"rpcpassword");
        }
        if ( rpcuser != 0 && rpcpassword != 0 )
            sprintf(userpass,"%s:%s",rpcuser,rpcpassword);
        else userpass[0] = 0;
        if ( Debuglevel > 0 )
            printf("-> (%s):(%s) userpass.(%s)\n",rpcuser,rpcpassword,userpass);
        if ( rpcuser != 0 )
            free(rpcuser);
        if ( rpcpassword != 0 )
            free(rpcpassword);
        fclose(fp);
    }
    else
    {
        printf("extract_userpass cant open.(%s)\n",fname);
        return(0);
    }
    if ( userpass[0] != 0 )
        return(clonestr(userpass));
    return(0);
}

#undef value_coders8
#undef minted_coders8
#undef txidind_coders6
#undef scriptind_coders6
#undef addrind_coders6
#undef blocknum_coders4
#undef vout_coders2
#undef numvin_coders2
#undef numvout_coders2
#undef numtx_coders2

#endif
#endif
