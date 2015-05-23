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
int32_t ledger_update(struct rawblock *emit,struct ledger_info *ledger,struct alloc_space *mem,uint32_t RTblocknum,int32_t syncflag,int32_t minconfirms);
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

int32_t ledger_update(struct rawblock *emit,struct ledger_info *ledger,struct alloc_space *mem,uint32_t RTblocknum,int32_t syncflag,int32_t minconfirms)
{
    struct ledger_blockinfo *block; struct ledger_inds L;
    uint32_t blocknum,dispflag,ledgerhash; uint64_t supply,oldsupply; double estimate,elapsed,startmilli;
    blocknum = ledger->blocknum;
    if ( blocknum <= RTblocknum-minconfirms )
    {
        startmilli = milliseconds();
        dispflag = 1 || (blocknum > RTblocknum - 1000);
        dispflag += ((blocknum % 100) == 0);
        oldsupply = ledger->voutsum - ledger->spendsum;
        if ( ledger->DBs.transactions == 0 )
            ledger->DBs.transactions = 0;//sp_begin(ledger->DBs.env), ledger->numsyncs++;
        if ( (block= ledger_setblock(dispflag,ledger,mem,emit,blocknum)) != 0 )
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
            ledger->totalsize += block->allocsize;
            estimate = estimate_completion(ledger->startmilli,blocknum - ledger->startblocknum,RTblocknum-blocknum)/60000;
            elapsed = (milliseconds() - ledger->startmilli)/60000.;
            supply = ledger->voutsum - ledger->spendsum;
            if ( dispflag != 0 )
            {
                extern uint32_t Duplicate,Mismatch,Added,Linked,Numgets;
                printf("%.3f %-5s [lag %-5d] %-6u %.8f %.8f (%.8f) [%.8f] %13.8f | dur %.2f %.2f %.2f | len.%-5d %s %.1f | H%d E%d R%d W%d %08x\n",ledger->calc_elapsed/1000.,ledger->DBs.coinstr,RTblocknum-blocknum,blocknum,dstr(supply),dstr(ledger->addrsum),dstr(supply)-dstr(ledger->addrsum),dstr(supply)-dstr(oldsupply),dstr(block->minted),elapsed,elapsed+(RTblocknum-blocknum)*ledger->calc_elapsed/60000,elapsed+estimate,block->allocsize,_mbstr(ledger->totalsize),(double)ledger->totalsize/blocknum,Duplicate,Mismatch,Numgets,Added,ledgerhash);
            }
            ledger->blocknum++;
            return(1);
        }
        else printf("%s error processing block.%d\n",ledger->DBs.coinstr,blocknum);
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

#endif
#endif
