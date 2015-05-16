//
//  ramchain.c
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_ramchain_h
#define crypto777_ramchain_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"
#include "system777.c"
#include "ledger777.c"

void ledger_free(struct ledger_info *ledger,int32_t closeDBflag);
void ledger_stateinit(struct env777 *DBs,struct ledger_state *sp,char *coinstr,char *subdir,char *name,char *compression,int32_t flags,int32_t valuesize);
struct ledger_info *ledger_alloc(char *coinstr,char *subdir,int32_t flags);
int32_t ramchain_init(char *retbuf,struct coin777 *coin,char *coinstr,uint32_t startblocknum,uint32_t endblocknum);
int32_t ramchain_stop(char *retbuf,struct ramchain *ramchain);
int32_t ramchain_richlist(char *retbuf,int32_t maxlen,struct ramchain *ramchain,int32_t num);
void ramchain_update(struct ramchain *ramchain,char *serverport,char *userpass,int32_t syncflag);
int32_t ramchain_resume(char *retbuf,struct ramchain *ramchain,char *serverport,char *userpass,uint32_t startblocknum,uint32_t endblocknum);

#endif
#else
#ifndef crypto777_ramchain_c
#define crypto777_ramchain_c

#ifndef crypto777_ramchain_h
#define DEFINES_ONLY
#include "ramchain.c"
#undef DEFINES_ONLY
#endif


// env funcs
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
    //update_sha256(sp->sha256,&sp->state,0,0);
    if ( DBs != 0 )
        sp->DB = db777_open(0,DBs,name,compression,flags,valuesize);
}

struct ledger_info *ledger_alloc(char *coinstr,char *subdir,int32_t flags)
{
    struct ledger_info *ledger = 0; int32_t flagsB;
    if ( (ledger= calloc(1,sizeof(*ledger))) != 0 )
    {
        if ( flags == 0 )
            flags = (DB777_FLUSH | DB777_HDD | 0*DB777_MULTITHREAD), flagsB = flags | DB777_NANO;
        else flagsB = flags;
        safecopy(ledger->DBs.coinstr,coinstr,sizeof(ledger->DBs.coinstr));
        safecopy(ledger->DBs.subdir,subdir,sizeof(ledger->DBs.subdir));
        ledger_stateinit(&ledger->DBs,&ledger->addrinfos,coinstr,subdir,"addrinfos","zstd",flags | DB777_RAM | DB777_KEY32,0);
        ledger_stateinit(&ledger->DBs,&ledger->blocks,coinstr,subdir,"blocks","zstd",flags | DB777_KEY32,0);
        ledger_stateinit(&ledger->DBs,&ledger->revaddrs,coinstr,subdir,"revaddrs","zstd",flags | DB777_KEY32,0);
        ledger_stateinit(&ledger->DBs,&ledger->txoffsets,coinstr,subdir,"txoffsets","zstd",flags | DB777_KEY32,sizeof(struct upair32));
        ledger_stateinit(&ledger->DBs,&ledger->unspentmap,coinstr,subdir,"unspentmap","zstd",flags | DB777_KEY32,sizeof(struct unspentmap));
        ledger_stateinit(&ledger->DBs,&ledger->spentbits,coinstr,subdir,"spentbits","zstd",flags | DB777_KEY32,1);
        ledger_stateinit(&ledger->DBs,&ledger->ledger,coinstr,subdir,"ledger","zstd",flags,sizeof(struct ledger_inds));
        ledger_stateinit(&ledger->DBs,&ledger->addrs,coinstr,subdir,"addrs","zstd",flags,sizeof(uint32_t));
        ledger_stateinit(&ledger->DBs,&ledger->txids,coinstr,subdir,"txids",0,flags,sizeof(uint32_t));
        ledger_stateinit(&ledger->DBs,&ledger->scripts,coinstr,subdir,"scripts","zstd",flags,sizeof(uint32_t));
        ledger->blocknum = 0;
        
    }
    return(ledger);
}

void ramchain_update(struct ramchain *ramchain,char *serverport,char *userpass,int32_t syncflag)
{
    void ledger_free(struct ledger_info *ledger,int32_t closeDBflag);
    struct alloc_space MEM; struct ledger_info *ledger; struct ledger_blockinfo *block;
    uint32_t blocknum,dispflag; uint64_t supply,oldsupply; double estimate,elapsed,startmilli,diff;
    if ( ramchain->readyflag == 0 || (ledger= ramchain->activeledger) == 0 )
        return;
    blocknum = ledger->blocknum;
    if ( blocknum < ramchain->RTblocknum )
    {
        startmilli = milliseconds();
        dispflag = 1 || (blocknum > ramchain->RTblocknum - 1000);
        dispflag += ((blocknum % 100) == 0);
        oldsupply = ledger->voutsum - ledger->spendsum;
        memset(&MEM,0,sizeof(MEM)), MEM.ptr = &ramchain->DECODE, MEM.size = sizeof(ramchain->DECODE);
        if ( ledger->DBs.transactions == 0 )
            ledger->DBs.transactions = sp_begin(ledger->DBs.env);
        if ( (block= ledger_update(dispflag,ledger,&MEM,ramchain->name,serverport,userpass,&ramchain->EMIT,blocknum)) != 0 )
        {
            if ( syncflag != 0 )
            {
                ledger_setlast(ledger,ledger->blocknum,++ledger->numsyncs);
                db777_sync(ledger->DBs.transactions,&ledger->DBs,DB777_FLUSH);
                ledger->DBs.transactions = 0;
            }
            ramchain->addrsum = ledger_recalc_addrinfos(0,0,ledger,0);
            diff = (milliseconds() - startmilli);
            ramchain->totalsize += block->allocsize;
            estimate = estimate_completion(ramchain->startmilli,blocknum - ramchain->startblocknum,ramchain->RTblocknum-blocknum)/60000;
            elapsed = (milliseconds() - ramchain->startmilli)/60000.;
            supply = ledger->voutsum - ledger->spendsum;
            if ( dispflag != 0 )
            {
                extern uint32_t Duplicate,Mismatch,Added;
                printf("%-5s [lag %-5d] %-6u supply %.8f %.8f (%.8f) [%.8f] %13.8f | dur %.2f %.2f %.2f | len.%-5d %s %.1f | DMA %d ?.%d %d\n",ramchain->name,ramchain->RTblocknum-blocknum,blocknum,dstr(supply),dstr(ramchain->addrsum),dstr(supply)-dstr(ramchain->addrsum),dstr(supply)-dstr(oldsupply),dstr(ramchain->EMIT.minted),elapsed,elapsed+(ramchain->RTblocknum-blocknum)*diff/60000,elapsed+estimate,block->allocsize,_mbstr(ramchain->totalsize),(double)ramchain->totalsize/blocknum,Duplicate,Mismatch,Added);
            }
            ledger->blocknum++;
        }
        else printf("%s error processing block.%d\n",ramchain->name,blocknum);
    }
}

int32_t ramchain_resume(char *retbuf,struct ramchain *ramchain,char *serverport,char *userpass,uint32_t startblocknum,uint32_t endblocknum)
{
    extern uint32_t Duplicate,Mismatch,Added;
    struct ledger_info *ledger; struct alloc_space MEM; uint64_t balance;
    if ( (ledger= ramchain->activeledger) == 0 )
    {
        sprintf(retbuf,"{\"error\":\"no active ledger\"}");
        return(-1);
    }
    Duplicate = Mismatch = Added = 0;
    ramchain->startmilli = milliseconds();
    ramchain->totalsize = 0;
    ramchain->startblocknum = ledger_startblocknum(ledger,0);
    memset(&MEM,0,sizeof(MEM)), MEM.ptr = &ramchain->DECODE, MEM.size = sizeof(ramchain->DECODE);
    if ( ramchain->startblocknum > 0 )
        ledger_setblocknum(ledger,&MEM,ramchain->startblocknum);
    else ramchain->startblocknum = 0;
    ledger->blocknum = ramchain->startblocknum + (ramchain->startblocknum != 0);
    ramchain->endblocknum = (endblocknum > ramchain->startblocknum) ? endblocknum : ramchain->startblocknum;
    balance = ledger_recalc_addrinfos(0,0,ledger,0);
    sprintf(retbuf,"{\"result\":\"resumed\",\"startblocknum\":%d,\"endblocknum\":%d,\"addrsum\":%.8f,\"ledger supply\":%.8f,\"diff\":%.8f,\"elapsed\":%.3f}",ramchain->startblocknum,ramchain->endblocknum,dstr(balance),dstr(ledger->voutsum) - dstr(ledger->spendsum),dstr(balance) - (dstr(ledger->voutsum) - dstr(ledger->spendsum)),(milliseconds() - ramchain->startmilli)/1000.);
    ramchain->RTblocknum = _get_RTheight(&ramchain->lastgetinfo,ramchain->name,serverport,userpass,ramchain->RTblocknum);
    ramchain->startmilli = milliseconds();
    ramchain->paused = 0;
    return(0);
}

int32_t ramchain_init(char *retbuf,struct coin777 *coin,char *coinstr,uint32_t startblocknum,uint32_t endblocknum)
{
    struct ramchain *ramchain = &coin->ramchain;
    if ( coin != 0 )
    {
        ramchain->syncfreq = 10000;
        strcpy(ramchain->name,coinstr);
        ramchain->readyflag = 1;
        if ( (ramchain->activeledger= ledger_alloc(coinstr,"",0)) != 0 )
        {
            env777_start(0,&ramchain->activeledger->DBs);
            if ( endblocknum == 0 )
                endblocknum = 1000000000;
            return(ramchain_resume(retbuf,ramchain,coin->serverport,coin->userpass,startblocknum,endblocknum));
        }
    }
    sprintf(retbuf,"{\"error\":\"no coin777\"}");
    return(-1);
}

int32_t ramchain_stop(char *retbuf,struct ramchain *ramchain)
{
    ramchain->paused = 2;
    sprintf(retbuf,"{\"result\":\"ramchain stopping\"}");
    return(0);
}

int32_t ramchain_richlist(char *retbuf,int32_t maxlen,struct ramchain *ramchain,int32_t num)
{
    cJSON *json;
    if ( ramchain->activeledger != 0 )
    {
        ledger_recalc_addrinfos(retbuf,maxlen,ramchain->activeledger,num);
        if ( (json= cJSON_Parse(retbuf)) == 0 )
            printf("PARSE ERROR!\n");
        else free_json(json);
    }
    return(0);
}

#endif
#endif
