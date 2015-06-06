//
//  ledger777.c
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_ledger777_h
#define crypto777_ledger777_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "coins777.c"
#include "system777.c"
#include "gen1block.c"

struct ledger_inds
{
    uint64_t voutsum,spendsum;
    uint32_t blocknum,numsyncs,addrind,txidind,scriptind,unspentind,numspents,numaddrinfos,txoffsets;
    struct sha256_state hashstates[13]; uint8_t hashes[13][256 >> 3];
};

// higher level
struct ledger_info *ledger_alloc(char *coinstr,char *subdir,int32_t flags);
void ledger_free(struct ledger_info *ledger,int32_t closeDBflag);
int32_t ledger_update(struct coin777 *coin,struct ledger_info *ledger,struct alloc_space *mem,uint32_t RTblocknum,int32_t syncflag,int32_t minconfirms);
int32_t ledger_syncblocks(struct ledger_inds *inds,int32_t max,struct ledger_info *ledger);
int32_t ledger_startblocknum(struct ledger_info *ledger,uint32_t startblocknum);
struct ledger_blockinfo *ledger_setblocknum(struct ledger_info *ledger,struct alloc_space *mem,uint32_t startblocknum);
int32_t ledger_ledgerhash(char *ledgerhash,struct ledger_inds *lp);
struct ledger_blockinfo *ledger_setblocknum(struct ledger_info *ledger,struct alloc_space *mem,uint32_t startblocknum);

#endif
#else
#ifndef crypto777_ledger777_c
#define crypto777_ledger777_c

#ifndef crypto777_ledger777_h
#define DEFINES_ONLY
#include "ledger777.c"
#undef DEFINES_ONLY
#endif

void ledger_copyhash(struct ledger_inds *lp,int32_t i,struct ledger_state *sp)
{
    memcpy(&lp->hashstates[i],&sp->state,sizeof(lp->hashstates[i]));
    memcpy(lp->hashes[i],sp->sha256,sizeof(lp->hashes[i]));
}

void ledger_restorehash(struct ledger_inds *lp,int32_t i,struct ledger_state *sp)
{
    memcpy(&sp->state,&lp->hashstates[i],sizeof(lp->hashstates[i]));
    memcpy(sp->sha256,lp->hashes[i],sizeof(lp->hashes[i]));
}

int32_t ledger_copyhashes(struct ledger_inds *lp,struct ledger_info *ledger,int32_t restoreflag)
{
    int32_t i,n = 0; struct ledger_state *states[(int32_t)(sizeof(lp->hashes)/sizeof(lp->hashes))];
    states[n++] = &ledger->addrs, states[n++] = &ledger->addrinfos;
    states[n++] = &ledger->txids, states[n++] = &ledger->txoffsets, states[n++] = &ledger->unspentmap;
    states[n++] = &ledger->scripts, states[n++] = &ledger->spentbits, states[n++] = &ledger->blocks;
    if ( restoreflag == 0 )
    {
        for (i=0; i<n; i++)
            ledger_copyhash(lp,i,states[i]);
    }
    else
    {
        for (i=0; i<n; i++)
            ledger_restorehash(lp,i,states[i]);
    }
#define LEDGER_NUMHASHES 8
    if ( n != LEDGER_NUMHASHES )
        printf("mismatched LEDGER_NUMHASHES %d != %d\n",n,LEDGER_NUMHASHES);
    if ( n >= (int32_t)(sizeof(lp->hashes)/sizeof(*lp->hashes)) )
        printf("Too many hashes to save them %d vs %ld\n",n,(sizeof(lp->hashes)/sizeof(lp->hashes)));
    return(n);
}

uint32_t ledger_setlast(struct ledger_inds *L,struct ledger_info *ledger,uint32_t blocknum,int32_t numsyncs)
{
    uint32_t ledgerhash; int32_t i,n,retval = 0;
    memset(L,0,sizeof(*L));
    L->blocknum = ledger->blocknum, L->numsyncs = ledger->numsyncs;
    L->voutsum = ledger->voutsum, L->spendsum = ledger->spendsum;
    L->addrind = ledger->addrs.ind, L->txidind = ledger->txids.ind, L->scriptind = ledger->scripts.ind;
    L->unspentind = ledger->unspentmap.ind, L->numspents = ledger->spentbits.ind;
    L->numaddrinfos = ledger->addrinfos.ind, L->txoffsets = ledger->txoffsets.ind;
    n = ledger_copyhashes(L,ledger,0);
    ledgerhash = 0;
    for (i=0; i<n; i++)
        ledgerhash ^= *(uint32_t *)L->hashes[i];
    if ( numsyncs < 0 )
    {
        for (i=0; i<n; i++)
            printf("%08x ",*(int *)L->hashes[i]);
        printf(" blocknum.%u txids.%d addrs.%d scripts.%d unspents.%d supply %.8f | ",ledger->blocknum,ledger->txids.ind,ledger->addrs.ind,ledger->scripts.ind,ledger->unspentmap.ind,dstr(ledger->voutsum)-dstr(ledger->spendsum));
        printf(" %08x\n",ledgerhash);
    }
    if ( numsyncs >= 0 )
    {
        printf(" synci.%d: blocknum.%u %08x txids.%d addrs.%d scripts.%d unspents.%d supply %.8f | ",numsyncs,ledger->blocknum,*(int *)ledger->ledger.sha256,ledger->txids.ind,ledger->addrs.ind,ledger->scripts.ind,ledger->unspentmap.ind,dstr(ledger->voutsum)-dstr(ledger->spendsum));
        printf("SYNCNUM.%d -> %d supply %.8f | ledgerhash %llx\n",numsyncs,blocknum,dstr(L->voutsum)-dstr(L->spendsum),*(long long *)ledger->ledger.sha256);
        if ( db777_set(DB777_HDD,ledger->DBs.transactions,ledger->ledger.DB,&numsyncs,sizeof(numsyncs),L,sizeof(*L)) != 0 )
            printf("error saving ledger\n");
        if ( numsyncs > 0 )
        {
            numsyncs = 0;
            if ( (retval = db777_set(DB777_HDD,ledger->DBs.transactions,ledger->ledger.DB,&numsyncs,sizeof(numsyncs),L,sizeof(*L))) != 0 )
                printf("error saving numsyncs.0 retval.%d\n",retval);
        }
    }
    return(ledgerhash);
}

struct ledger_inds *ledger_getsyncdata(struct ledger_inds *L,struct ledger_info *ledger,int32_t syncind)
{
    struct ledger_inds *lp; int32_t allocsize = sizeof(*L);
    if ( syncind <= 0 )
        syncind++;
    if ( (lp= db777_get(L,&allocsize,ledger->DBs.transactions,ledger->ledger.DB,&syncind,sizeof(syncind))) != 0 )
        return(lp);
    else memset(L,0,sizeof(*L));
    printf("couldnt find syncind.%d keylen.%ld\n",syncind,sizeof(syncind));
    return(0);
}

int32_t ledger_syncblocks(struct ledger_inds *inds,int32_t max,struct ledger_info *ledger)
{
    struct ledger_inds L,*lp; int32_t i,n = 0;
    if ( (lp= ledger_getsyncdata(&L,ledger,-1)) != 0 )
    {
        inds[n++] = *lp;
        for (i=lp->numsyncs; i>0&&n<max; i--)
        {
            if ( (lp= ledger_getsyncdata(&L,ledger,i)) != 0 )
                inds[n++] = *lp;
        }
    } else printf("null return from ledger_getsyncdata\n");
    return(n);
}

int32_t ledger_startblocknum(struct ledger_info *ledger,uint32_t synci)
{
    struct ledger_inds *lp,L; uint32_t i,ledgerhash,n,blocknum = 0;
    if ( (lp= ledger_getsyncdata(&L,ledger,synci)) == &L )
    {
        ledger->blocknum = blocknum = lp->blocknum, ledger->numsyncs = lp->numsyncs;
        ledger->voutsum = lp->voutsum, ledger->spendsum = lp->spendsum;
        ledger->addrs.ind = ledger->revaddrs.ind = lp->addrind;
        ledger->txids.ind = ledger->revtxids.ind = lp->txidind;
        ledger->scripts.ind = ledger->revscripts.ind = lp->scriptind;
        ledger->unspentmap.ind = lp->unspentind, ledger->spentbits.ind = lp->numspents;
        ledger->addrinfos.ind = lp->numaddrinfos, ledger->txoffsets.ind = lp->txoffsets;
        n = ledger_copyhashes(&L,ledger,1);
        ledgerhash = 0;
        for (i=0; i<n; i++)
            ledgerhash ^= *(uint32_t *)lp->hashes[i], printf("%08x ",*(uint32_t *)lp->hashes[i]);
        printf("| %08x restored synci.%d: blocknum.%u txids.%d addrs.%d scripts.%d unspents.%d supply %.8f\n",ledgerhash,synci,ledger->blocknum,ledger->txids.ind,ledger->addrs.ind,ledger->scripts.ind,ledger->unspentmap.ind,dstr(ledger->voutsum)-dstr(ledger->spendsum));
    } else printf("ledger_getnearest error getting last\n");
    return(blocknum);
}

int32_t ledger_ledgerhash(char *ledgerhash,struct ledger_inds *lp)
{
    ledgerhash[0] = 0;
    if ( lp != 0 )
        init_hexbytes_noT(ledgerhash,lp->hashes[LEDGER_NUMHASHES - 1],256 >> 3);
    else return(-1);
    return(0);
}

int32_t ledger_update(struct coin777 *coin,struct ledger_info *ledger,struct alloc_space *mem,uint32_t RTblocknum,int32_t syncflag,int32_t minconfirms)
{
    //struct ledger_blockinfo *block;
    struct ledger_inds L;
    uint32_t blocknum,dispflag,ledgerhash=0,numtx=0,allocsize; uint64_t origsize,supply,oldsupply; double estimate,elapsed,startmilli;
    blocknum = ledger->blocknum;
    if ( blocknum <= RTblocknum-minconfirms )
    {
        startmilli = milliseconds();
        dispflag = 1 || (blocknum > RTblocknum - 1000);
        dispflag += ((blocknum % 100) == 0);
        oldsupply = ledger->voutsum - ledger->spendsum;
        if ( ledger->DBs.transactions == 0 )
            ledger->DBs.transactions = 0;//sp_begin(ledger->DBs.env), ledger->numsyncs++;
        origsize = coin->totalsize;
        //if ( (numtx= ledger_setblock(dispflag,ledger,mem,&coin->ramchain.EMIT,blocknum)) != 0 )
        {
            if ( syncflag != 0 )
            {
                ledger->addrsum = ledger_recalc_addrinfos(0,0,ledger,0);
                ledgerhash = ledger_setlast(&L,ledger,ledger->blocknum,ledger->numsyncs);
                db777_sync(ledger->DBs.transactions,&ledger->DBs,DB777_FLUSH);
                ledger->DBs.transactions = 0;
            }
            else ledgerhash = ledger_setlast(&L,ledger,ledger->blocknum,-1);
            dxblend(&ledger->calc_elapsed,(milliseconds() - startmilli),.99);
            allocsize = (uint32_t)(coin->totalsize - origsize);
            ledger->totalsize += allocsize;
            estimate = estimate_completion(ledger->startmilli,blocknum - ledger->startblocknum,RTblocknum-blocknum)/60000;
            elapsed = (milliseconds() - ledger->startmilli)/60000.;
            supply = ledger->voutsum - ledger->spendsum;
            if ( dispflag != 0 )
            {
                extern uint32_t Duplicate,Mismatch,Added,Linked,Numgets;
                printf("%.3f %-5s [lag %-5d] %-6u %.8f %.8f (%.8f) [%.8f] %13.8f | dur %.2f %.2f %.2f | len.%-5d %s %.1f | H%d E%d R%d W%d %08x\n",ledger->calc_elapsed/1000.,ledger->DBs.coinstr,RTblocknum-blocknum,blocknum,dstr(supply),dstr(coin->addrsum),dstr(supply)-dstr(coin->addrsum),dstr(supply)-dstr(oldsupply),dstr(coin->minted != 0 ? coin->minted : coin->latest.total),elapsed,elapsed+(RTblocknum-blocknum)*ledger->calc_elapsed/60000,elapsed+estimate,allocsize,_mbstr(ledger->totalsize),(double)ledger->totalsize/blocknum,Duplicate,Mismatch,Numgets,Added,ledgerhash);
            }
            ledger->blocknum++;
            return(1);
        } //else printf("%s error processing block.%d numtx.%d\n",ledger->DBs.coinstr,blocknum,numtx);
    } else printf("blocknum.%d > RTblocknum.%d - minconfirms.%d\n",blocknum,RTblocknum,minconfirms);
    return(0);
}

// higher level functions
int32_t ledger_commit(struct ledger_info *ledger,int32_t continueflag)
{
    int32_t err = -1;
    while ( ledger->DBs.transactions != 0 && (err= sp_commit(ledger->DBs.transactions)) != 0 )
    {
        printf("ledger_commit: sp_commit error.%d\n",err);
        if ( err < 0 )
            break;
        msleep(1000);
    }
    ledger->DBs.transactions = 0;//(continueflag != 0) ? sp_begin(ledger->DBs.env) : 0;
    return(err);
}

#define LEDGER_DB_CLOSE 1
#define LEDGER_DB_BACKUP 2
#define LEDGER_DB_UPDATE 3

int32_t ledger_DBopcode(void *ctl,struct db777 *DB,int32_t opcode)
{
    int32_t retval = -1;
    if ( opcode == LEDGER_DB_CLOSE )
    {
        retval = sp_destroy(DB->db);
        DB->db = DB->asyncdb = 0;
    }
    return(retval);
}

int32_t ledger_DBopcodes(struct env777 *DBs,int32_t opcode)
{
    int32_t i,numerrs = 0;
    for (i=0; i<DBs->numdbs; i++)
        numerrs += (ledger_DBopcode(DBs->ctl,&DBs->dbs[i],opcode) != 0);
    return(numerrs);
}

void ledger_free(struct ledger_info *ledger,int32_t closeDBflag)
{
    int32_t i;
    if ( ledger != 0 )
    {
        for (i=0; i<ledger->DBs.numdbs; i++)
            if ( (ledger->DBs.dbs[i].flags & DB777_RAM) != 0 )
                db777_free(&ledger->DBs.dbs[i]);
        if ( closeDBflag != 0 )
        {
            ledger_DBopcodes(&ledger->DBs,LEDGER_DB_CLOSE);
            sp_destroy(ledger->DBs.env), ledger->DBs.env = 0;
        }
        free(ledger);
    }
}

void ledger_stateinit(struct env777 *DBs,struct ledger_state *sp,char *coinstr,char *subdir,char *name,char *compression,int32_t flags,int32_t valuesize)
{
    safecopy(sp->name,name,sizeof(sp->name));
    update_sha256(sp->sha256,&sp->state,0,0);
    if ( DBs != 0 )
        sp->DB = db777_open(0,DBs,name,compression,flags,valuesize);
}

struct ledger_info *ledger_alloc(char *coinstr,char *subdir,int32_t flags)
{
    struct ledger_info *ledger = 0; 
    if ( (ledger= calloc(1,sizeof(*ledger))) != 0 )
    {
       // int32_t maxblocknum = 1000;
       // coin777_ensurespace(coin777_find(coinstr,0),maxblocknum,maxblocknum*8,maxblocknum*8,maxblocknum*8,maxblocknum*8,maxblocknum*8);
       // return(ledger);
   //Debuglevel = 3;
        if ( flags == 0 )
            flags = (DB777_FLUSH | DB777_HDD | DB777_MULTITHREAD | DB777_RAMDISK);
        safecopy(ledger->DBs.coinstr,coinstr,sizeof(ledger->DBs.coinstr));
        safecopy(ledger->DBs.subdir,subdir,sizeof(ledger->DBs.subdir));
        printf("open ramchain DB files (%s) (%s)\n",coinstr,subdir);
        ledger_stateinit(&ledger->DBs,&ledger->blocks,coinstr,subdir,"blocks","zstd",flags | DB777_KEY32,0);
        ledger_stateinit(&ledger->DBs,&ledger->addrinfos,coinstr,subdir,"addrinfos","zstd",flags | DB777_RAM,0);
        ledger_stateinit(&ledger->DBs,&ledger->packed,coinstr,subdir,"packed","zstd",flags,0);
        
        ledger_stateinit(&ledger->DBs,&ledger->revaddrs,coinstr,subdir,"revaddrs","zstd",flags | DB777_KEY32,34);
        ledger_stateinit(&ledger->DBs,&ledger->revscripts,coinstr,subdir,"revscripts","zstd",flags,0);
        ledger_stateinit(&ledger->DBs,&ledger->revtxids,coinstr,subdir,"revtxids","zstd",flags | DB777_KEY32,32);
        ledger_stateinit(&ledger->DBs,&ledger->spentbits,coinstr,subdir,"spentbits","zstd",flags | DB777_KEY32,1);
        ledger_stateinit(&ledger->DBs,&ledger->unspentmap,coinstr,subdir,"unspentmap","zstd",flags | DB777_KEY32 | RAMCHAINS.fastmode*DB777_RAM,sizeof(struct unspentmap));
        ledger_stateinit(&ledger->DBs,&ledger->txoffsets,coinstr,subdir,"txoffsets","zstd",flags | DB777_KEY32 | RAMCHAINS.fastmode*DB777_RAM,sizeof(struct upair32));
        
        ledger_stateinit(&ledger->DBs,&ledger->addrs,coinstr,subdir,"addrs","zstd",flags | RAMCHAINS.fastmode*DB777_RAM,sizeof(uint32_t) * 2);
        ledger_stateinit(&ledger->DBs,&ledger->txids,coinstr,subdir,"txids",0,flags | RAMCHAINS.fastmode*DB777_RAM,sizeof(uint32_t) * 2);
        ledger_stateinit(&ledger->DBs,&ledger->scripts,coinstr,subdir,"scripts","zstd",flags | RAMCHAINS.fastmode*DB777_RAM,sizeof(uint32_t) * 2);
        ledger_stateinit(&ledger->DBs,&ledger->ledger,coinstr,subdir,"ledger","zstd",flags,sizeof(struct ledger_inds));
        ledger->blocknum = 0;
    }
    return(ledger);
}
struct packedblock *ramchain_getpackedblock(void *space,int32_t *lenp,struct ramchain *ramchain,uint32_t blocknum)
{
    struct ledger_info *ledger;
    if ( (ledger= ramchain->activeledger) != 0 )
        return(db777_get(space,lenp,ledger->DBs.transactions,ledger->packed.DB,&blocknum,sizeof(blocknum)));
    else return(0);
}

void ramchain_setpackedblock(struct ramchain *ramchain,struct packedblock *packed,uint32_t blocknum)
{
    struct ledger_info *ledger;
    if ( packed_crc16(packed) == packed->crc16 )
    {
        if ( (ledger= ramchain->activeledger) != 0 )
            db777_set(DB777_HDD,ledger->DBs.transactions,ledger->ledger.DB,&blocknum,sizeof(blocknum),packed,packed->allocsize);
        if ( RELAYS.pushsock >= 0 )
        {
            printf("PUSHED.(%d) blocknum.%u | crc.%u %d %d %d %.8f %u %u %u %u %u %u %d\n",packed->allocsize,packed->blocknum,packed->crc16,packed->numtx,packed->numrawvins,packed->numrawvouts,dstr(packed->minted),packed->timestamp,packed->blockhash_offset,packed->merkleroot_offset,packed->txspace_offsets,packed->vinspace_offsets,packed->voutspace_offsets,packed->allocsize);
            nn_send(RELAYS.pushsock,(void *)packed,packed->allocsize,0);
        }
    }
}

void coin777_pulldata(struct packedblock *packed,int32_t len)
{
    struct coin777 *coin;
    if ( len >= ((long)&packed->allocsize - (long)packed + sizeof(packed->allocsize)) && packed->allocsize == len && packed_crc16(packed) == packed->crc16 )
    {
        if ( (coin= coin777_find("BTC",0)) != 0 )
            ramchain_setpackedblock(&coin->ramchain,packed,packed->blocknum);
        printf("PULLED.(%d) blocknum.%u | %u %d %d %d %.8f %u %u %u %u %u %u %d\n",len,packed->blocknum,packed->crc16,packed->numtx,packed->numrawvins,packed->numrawvouts,dstr(packed->minted),packed->timestamp,packed->blockhash_offset,packed->merkleroot_offset,packed->txspace_offsets,packed->vinspace_offsets,packed->voutspace_offsets,packed->allocsize);
    } else printf("pulled.%d but size.%d mismatch or crc16 mismatch %d %d\n",len,packed->allocsize,packed_crc16(packed),packed->crc16);
    nn_freemsg(packed);
}

int32_t ramchain_update(struct coin777 *coin,struct ramchain *ramchain,void *deprecated,struct packedblock *packed)
{
    uint32_t blocknum; int32_t lag,syncflag,flag = 0; //double startmilli; struct alloc_space MEM;
    blocknum = coin->blocknum;
    if ( (lag= (coin->RTblocknum - blocknum)) < 1000 || (blocknum % 100) == 0 )
        coin->RTblocknum = _get_RTheight(&coin->lastgetinfo,coin->name,coin->serverport,coin->userpass,coin->RTblocknum);
    if ( lag < DB777_MATRIXROW*10 && coin->syncfreq > DB777_MATRIXROW )
        coin->syncfreq = DB777_MATRIXROW;
    else if ( lag < DB777_MATRIXROW && coin->syncfreq > DB777_MATRIXROW/10 )
        coin->syncfreq = DB777_MATRIXROW/10;
    else if ( lag < DB777_MATRIXROW/10 && coin->syncfreq > DB777_MATRIXROW/100 )
        coin->syncfreq = DB777_MATRIXROW/100;
    else if ( strcmp(coin->DBs.coinstr,"BTC") == 0 && lag < DB777_MATRIXROW/100 && coin->syncfreq > DB777_MATRIXROW/1000 )
        coin->syncfreq = DB777_MATRIXROW/1000;
    if ( ramchain->paused < 10 )
    {
        syncflag = (((blocknum % coin->syncfreq) == 0) || (coin->needbackup != 0) || (blocknum % 10000) == 0);
        //if ( syncflag != 0 )
        //    printf("sync.%d (%d  %d) || %d\n",syncflag,blocknum,coin->syncfreq,coin->needbackup);
        if ( blocknum >= coin->endblocknum || ramchain->paused != 0 )
        {
            if ( blocknum >= coin->endblocknum )
                ramchain->paused = 3, syncflag = 2;
            printf("ramchain.%s blocknum.%d <<< PAUSING paused.%d |  endblocknum.%u\n",coin->DBs.coinstr,blocknum,ramchain->paused,coin->endblocknum);
        }
        if ( blocknum <= (coin->RTblocknum - coin->minconfirms) )
        {
            //memset(&MEM,0,sizeof(MEM)), MEM.ptr = &ramchain->DECODE, MEM.size = sizeof(ramchain->DECODE);
            //startmilli = milliseconds();
            //len = (int32_t)MEM.size;
            /*if ( (packed != 0 || (packed= ramchain_getpackedblock(MEM.ptr,&len,ramchain,blocknum)) != 0) && packed_crc16(packed) == packed->crc16 )
             {
             ram_clear_rawblock(ramchain->EMIT,0);
             coin777_unpackblock(ramchain->EMIT,packed,blocknum);
             }
             else rawblock_load(ramchain->EMIT,ramchain->name,ramchain->serverport,ramchain->userpass,blocknum);
             dxblend(&ledger->load_elapsed,(milliseconds() - startmilli),.99); printf("%.3f ",ledger->load_elapsed/1000.);*/
            flag = coin777_parse(coin,coin->RTblocknum,syncflag * (blocknum != 0),coin->minconfirms);
            // flag = ledger_update(coin,ledger,&MEM,ramchain->RTblocknum,syncflag * (blocknum != 0),ramchain->minconfirms);
        }
        if ( ramchain->paused == 3 )
        {
            //ledger_free(ramchain->activeledger,1), ramchain->activeledger = 0;
            printf("STOPPED\n");
        }
        if ( blocknum > coin->endblocknum || ramchain->paused != 0 )
            ramchain->paused = 10;
    }
    return(flag);
}

int32_t ramchain_resume(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson,uint32_t startblocknum,uint32_t endblocknum)
{
    uint32_t txidind,addrind,scriptind,numrawvouts,numrawvins,timestamp;
    /*extern uint32_t Duplicate,Mismatch,Added;
     struct ledger_info *ledger; struct alloc_space MEM; uint64_t balance; //int32_t numlinks,numlinks2;
     if ( (ledger= ramchain->activeledger) == 0 )
     {
     sprintf(retbuf,"{\"error\":\"no active ledger\"}");
     return(-1);
     }
     Duplicate = Mismatch = Added = 0;
     ledger->startmilli = milliseconds();
     ledger->totalsize = 0;
     //printf("resuming %u to %u\n",startblocknum,endblocknum);
     ledger->startblocknum = startblocknum; //ledger_startblocknum(ledger,-1);
     //printf("updated %u to %u\n",ledger->startblocknum,endblocknum);
     memset(&MEM,0,sizeof(MEM)), MEM.ptr = &ramchain->DECODE, MEM.size = sizeof(ramchain->DECODE);
     //ledger_setblocknum(ledger,&MEM,ledger->startblocknum);
     ledger->blocknum = ledger->startblocknum + (ledger->startblocknum > 1);
     printf("set %u to %u | sizes: uthash.%ld addrinfo.%ld unspentmap.%ld txoffset.%ld db777_entry.%ld\n",ledger->startblocknum,endblocknum,sizeof(UT_hash_handle),sizeof(struct ledger_addrinfo),sizeof(struct unspentmap),sizeof(struct upair32),sizeof(struct db777_entry));
     ledger->endblocknum = (endblocknum > ledger->startblocknum) ? endblocknum : ledger->startblocknum;
     if ( 0 )
     {
     numlinks = db777_linkDB(ledger->addrs.DB,ledger->revaddrs.DB,ledger->addrs.ind);
     numlinks2 = db777_linkDB(ledger->txids.DB,ledger->revtxids.DB,ledger->txids.ind);
     printf("addrs numlinks.%d, txids numlinks.%d\n",numlinks,numlinks2);
     }
     balance = 0;//ledger_recalc_addrinfos(0,0,ledger,0);
     //printf("balance recalculated %.8f\n",dstr(balance));
     sprintf(retbuf,"{\"result\":\"resumed\",\"ledgerhash\":\"%llx\",\"startblocknum\":%d,\"endblocknum\":%d,\"addrsum\":%.8f,\"ledger supply\":%.8f,\"diff\":%.8f,\"elapsed\":%.3f}",*(long long *)ledger->ledger.sha256,ledger->startblocknum,ledger->endblocknum,dstr(balance),dstr(ledger->voutsum) - dstr(ledger->spendsum),dstr(balance) - (dstr(ledger->voutsum) - dstr(ledger->spendsum)),(milliseconds() - ledger->startmilli)/1000.);
     ramchain->RTblocknum = _get_RTheight(&ramchain->lastgetinfo,ramchain->name,ramchain->serverport,ramchain->userpass,ramchain->RTblocknum);
     ledger->startmilli = milliseconds();*/
    coin->startmilli = milliseconds();
    coin->RTblocknum = _get_RTheight(&coin->lastgetinfo,coin->name,coin->serverport,coin->userpass,coin->RTblocknum);
    coin777_initDBenv(coin);
    coin->startblocknum = coin777_startblocknum(coin,-1);
    printf("startblocknum.%u\n",coin->startblocknum);
    if ( coin777_getinds(coin,coin->startblocknum,&timestamp,&txidind,&numrawvouts,&numrawvins,&addrind,&scriptind) == 0 )
        coin777_initmmap(coin,coin->startblocknum,txidind,addrind,scriptind,numrawvouts,numrawvins);
    ramchain->paused = 0;
    return(0);
}

int32_t ramchain_pause(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson)
{
    ramchain->paused = 1;
    sprintf(retbuf,"{\"result\":\"started pause sequence\"}");
    return(0);
}

int32_t ramchain_stop(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson)
{
    ramchain->paused = 3;
    sprintf(retbuf,"{\"result\":\"pausing then stopping ramchain\"}");
    return(0);
}

int32_t ramchain_init(char *retbuf,int32_t maxlen,struct coin777 *coin,struct ramchain *ramchain,cJSON *argjson,char *coinstr,char *serverport,char *userpass,uint32_t startblocknum,uint32_t endblocknum,uint32_t minconfirms)
{
    //struct ledger_info *ledger;
    strcpy(ramchain->name,coinstr);
    strcpy(ramchain->serverport,serverport);
    strcpy(ramchain->userpass,userpass);
    ramchain->readyflag = 1;
    ramchain->minconfirms = minconfirms;
    if ( ramchain->activeledger != 0 )
    {
        ramchain_stop(retbuf,maxlen,coin,ramchain,argjson);
        while ( ramchain->activeledger != 0 )
            sleep(1);
    }
    //if ( (ramchain->activeledger= ledger_alloc(coinstr,"",0)) != 0 )
    {
        /*ledger = ramchain->activeledger;
         ledger->syncfreq = 100;//DB777_MATRIXROW;
         ledger->startblocknum = startblocknum, ledger->endblocknum = endblocknum;
         ramchain->RTblocknum = _get_RTheight(&ramchain->lastgetinfo,ramchain->name,ramchain->serverport,ramchain->userpass,ramchain->RTblocknum);
         */
        coin->syncfreq = 1000;
        coin->startblocknum = startblocknum, coin->endblocknum = endblocknum;
        coin->RTblocknum = _get_RTheight(&coin->lastgetinfo,coin->name,coin->serverport,coin->userpass,coin->RTblocknum);
        //env777_start(0,&ledger->DBs,ramchain->RTblocknum);
        if ( endblocknum == 0 )
            endblocknum = 1000000000;
        return(ramchain_resume(retbuf,maxlen,coin,ramchain,argjson,startblocknum,endblocknum));
    }
    sprintf(retbuf,"{\"error\":\"no coin777\"}");
    return(-1);
}

/*struct ledger_info *ramchain_session_ledger(struct ramchain *ramchain,int32_t directind)
 {
 struct ledger_info *ledger; char numstr[32];
 if ( (ledger= ramchain->session_ledgers[directind]) == 0 )
 {
 sprintf(numstr,"direct.%d",directind);
 ramchain->session_ledgers[directind] = ledger = ledger_alloc(ramchain->name,numstr,DB777_RAM);
 ledger->sessionid = rand();
 }
 return(ledger);
 }
 ramchain_rawblock(retbuf,maxlen,&coin->ramchain,coin->serverport,coin->userpass,cJSON_str(cJSON_GetObjectItem(json,"mytransport")),cJSON_str(cJSON_GetObjectItem(json,"myipaddr")),get_API_int(cJSON_GetObjectItem(json,"myport"),0),get_API_int(cJSON_GetObjectItem(json,"blocknum"),0));
 
 int32_t ramchain_rawblock(char *retbuf,int32_t maxlen,struct ramchain *ramchain,char *serverport,char *userpass,char *transport,char *ipaddr,uint16_t port,uint32_t blocknum)
 {
 struct alloc_space MEM; struct endpoint epbits; int32_t retval = -1;
 struct ledger_blockinfo *block; struct ledger_info *ledger; struct rawblock *emit;
 epbits = nn_directepbits(retbuf,transport,ipaddr,port);
 if ( epbits.ipbits == 0 )
 return(-1);
 if ( blocknum == 0 )
 sprintf(retbuf,"{\"error\":\"no blocknum specified\"}");
 else if ( (ledger= ramchain_session_ledger(ramchain,epbits.directind)) == 0 )
 sprintf(retbuf,"{\"error\":\"couldnt allocate session ledger\"}");
 else
 {
 emit = malloc(sizeof(*emit));
 memset(&MEM,0,sizeof(MEM)), MEM.size = 10000000, MEM.ptr = malloc(MEM.size);
 sprintf(retbuf,"{\"result\":\"ledger_update\",\"sessionid\":%u,\"counter\":%u}",ledger->sessionid,ledger->counter);
 strcpy(memalloc(&MEM,(int32_t)strlen(retbuf)+1,0),retbuf);
 if ( (block= ledger_update(0,ledger,&MEM,ramchain->name,serverport,userpass,emit,blocknum)) != 0 )
 {
 retval = nn_directsend(epbits,MEM.ptr,(int32_t)MEM.used);
 ledger->counter++;
 }
 else sprintf(retbuf,"{\"error\":\"ramchain_blockstr null return\"}");
 free(emit), free(MEM.ptr);
 }
 return(retval);
 }*/
#endif
#endif
