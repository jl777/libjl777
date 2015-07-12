//
//  gen1block.c
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_gen1block_h
#define crypto777_gen1block_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "../coins/coins777.c"
#include "../utils/system777.c"
#include "../coins/pass1.c"

struct ledger_blockinfo
{
    uint16_t crc16,numtx,numaddrs,numscripts,numvouts,numvins;
    uint32_t blocknum,allocsize,timestamp;
    uint64_t minted;
    uint8_t blockhash[32];
    uint8_t transactions[];
};
struct ledger_txinfo { uint32_t firstvout,firstvin; uint16_t numvouts,numvins; uint8_t txidlen,txid[255]; };
struct ledger_spendinfo { uint32_t unspentind,spent_txidind; uint16_t spent_vout; };
struct ledger_voutdata { struct unspentmap U; uint32_t scriptind; int32_t addrlen,scriptlen,newscript,newaddr; char coinaddr[256]; uint8_t script[4096]; };

void debugstop();

// pass0
uint32_t ledger_hexind(uint32_t *firstblocknump,int32_t writeflag,void *transactions,struct ledger_state *hash,uint8_t *data,int32_t *hexlenp,char *hexstr,uint32_t blocknum);
uint32_t ledger_txidind(uint32_t *firstblocknump,struct ledger_info *ledger,char *txidstr);
uint32_t ledger_addrind(uint32_t *firstblocknump,struct ledger_info *ledger,char *coinaddr);
uint32_t ledger_scriptind(uint32_t *firstblocknump,struct ledger_info *ledger,char *scriptstr);
uint32_t ledger_firstvout(int32_t requiredflag,struct ledger_info *ledger,uint32_t txidind);
int32_t ledger_upairset(struct ledger_info *ledger,uint32_t txidind,uint32_t firstvout,uint32_t firstvin);
struct ledger_blockinfo *ledger_setblock(int32_t dispflag,struct ledger_info *ledger,struct alloc_space *mem,struct rawblock *emit,uint32_t blocknum);
uint16_t block_crc16(struct ledger_blockinfo *block);
struct ledger_blockinfo *ledger_getblock(struct ledger_blockinfo *space,int32_t *allocsizep,struct ledger_info *ledger,uint32_t blocknum);

#endif
#else
#ifndef crypto777_gen1block_c
#define crypto777_gen1block_c

#ifndef crypto777_gen1block_h
#define DEFINES_ONLY
#include "gen1block.c"
#undef DEFINES_ONLY
#endif

void debugstop()
{
    //#ifdef __APPLE__
    while ( 1 )
        sleep(60);
    //#endif
}

uint16_t block_crc16(struct ledger_blockinfo *block)
{
    uint32_t crc32 = _crc32(0,(void *)((long)&block->crc16 + sizeof(block->crc16)),block->allocsize - sizeof(block->crc16));
    return((crc32 >> 16) ^ (crc32 & 0xffff));
}

uint32_t ledger_packtx(uint8_t *hash,struct sha256_state *state,struct alloc_space *mem,struct ledger_txinfo *tx)
{
    int32_t allocsize;
    allocsize = sizeof(*tx) - sizeof(tx->txid) + tx->txidlen;
    memcpy(memalloc(mem,allocsize,0),tx,allocsize);
    update_sha256(hash,state,(uint8_t *)tx,allocsize);
    return(allocsize);
}

uint32_t ledger_packspend(uint8_t *hash,struct sha256_state *state,struct alloc_space *mem,struct ledger_spendinfo *spend)
{
    memcpy(memalloc(mem,sizeof(spend->unspentind),0),&spend->unspentind,sizeof(spend->unspentind));
    update_sha256(hash,state,(uint8_t *)&spend->unspentind,sizeof(spend->unspentind));
    return(sizeof(spend->unspentind));
}

uint32_t ledger_packvoutstr(struct alloc_space *mem,uint32_t rawind,int32_t newitem,uint8_t *str,uint16_t len)
{
    int32_t enable_stringout = 0;
    uint8_t blen,extra = 1;
    if ( newitem != 0 )
    {
        rawind |= (1 << 31);
        memcpy(memalloc(mem,sizeof(rawind),0),&rawind,sizeof(rawind));
        if ( enable_stringout != 0 )
        {
            extra = 1;
            if ( len < 0xfd )
            {
                blen = len;
                memcpy(memalloc(mem,1,0),&blen,1);
            }
            else
            {
                blen = 0xfd;
                printf("long string len.%d %llx\n",len,*(long long *)str);
                memcpy(memalloc(mem,1,0),&blen,1);
                memcpy(memalloc(mem,2,0),&len,2), extra += 2;
            }
            memcpy(memalloc(mem,len,0),str,len);
            return(sizeof(rawind) + extra + len);
        }
    }
    else memcpy(memalloc(mem,sizeof(rawind),0),&rawind,sizeof(rawind));
    return(sizeof(rawind));
}

uint32_t ledger_packvout(uint8_t *hash,struct sha256_state *state,struct alloc_space *mem,struct ledger_voutdata *vout)
{
    uint32_t allocsize; void *ptr;
    ptr = memalloc(mem,sizeof(vout->U.value),0);
    memcpy(ptr,&vout->U.value,sizeof(vout->U.value)), allocsize = sizeof(vout->U.value);
    allocsize += ledger_packvoutstr(mem,vout->U.ind,vout->newaddr,(uint8_t *)vout->coinaddr,vout->addrlen);
    allocsize += ledger_packvoutstr(mem,vout->scriptind,vout->newscript,vout->script,vout->scriptlen);
    update_sha256(hash,state,ptr,allocsize);
    return(allocsize);
}

struct ledger_blockinfo *ledger_getblock(struct ledger_blockinfo *space,int32_t *allocsizep,struct ledger_info *ledger,uint32_t blocknum)
{
    return(db777_get(space,allocsizep,ledger->DBs.transactions,ledger->blocks.DB,&blocknum,sizeof(blocknum)));
}

uint32_t ledger_rawind(uint32_t *firstblocknump,int32_t writeflag,void *transactions,struct ledger_state *hash,void *key,int32_t keylen,uint32_t blocknum)
{
    uint32_t *ptr,pair[2],rawind = 0; int32_t size = sizeof(pair);
    if ( (ptr= db777_get(pair,&size,transactions,hash->DB,key,keylen)) != 0 )
    {
        rawind = ptr[0];
        if ( firstblocknump != 0 )
            *firstblocknump = ptr[1];
        if ( (rawind - 1) == hash->ind )
            hash->ind = rawind;
        if ( writeflag > 0 )
        {
            if ( blocknum < ptr[1] )
                printf("%s writeflag.1 matched rawind.%u firstblocknum %u vs at blocknum %u\n",hash->name,rawind,ptr[1],blocknum), debugstop();
            else if ( blocknum == ptr[1] )
                update_sha256(hash->sha256,&hash->state,key,keylen);
        }
        return(rawind);
    }
    if ( writeflag > 0 )
    {
        rawind = ++hash->ind;
        pair[0] = rawind, pair[1] = blocknum;
        if ( firstblocknump != 0 )
            *firstblocknump = blocknum;
        if ( Debuglevel > 2 || blocknum == 0 )
            printf("%p add rawind.%d keylen.%d: %llx | first appearance block.%u\n",hash->DB,rawind,keylen,*(long long *)key,blocknum);
        if ( hash->fp != 0 )
        {
            //fseek(hash->fp,rawind * hash->valuesize,SEEK_SET);
            //if ( fwrite(key,1,keylen,hash->fp) != keylen )
            //    printf("error writing %s.%u to file at offset.%ld\n",hash->name,rawind,rawind * hash->valuesize);
        }
        if ( db777_add(1,transactions,hash->DB,key,keylen,pair,sizeof(pair)) != 0 )
            printf("error adding to %s DB for rawind.%d keylen.%d\n",hash->name,rawind,keylen);
        else
        {
            update_sha256(hash->sha256,&hash->state,key,keylen);
            return(rawind);
        }
    }
    else if ( writeflag == 0 )
    {
        char hexstr[8192];
        if ( strcmp(hash->DB->name,"addrs") == 0 )
            strcpy(hexstr,key);
        else init_hexbytes_noT(hexstr,key,keylen);
        printf("%s %p couldnt find expected %s keylen.%d\n",hash->DB->name,hash->DB,hexstr,keylen);
        debugstop();
    }
    printf("no more retries\n");
    debugstop(); // db777_dump(hash->DB,1,1),
    return(0);
}

uint32_t ledger_hexind(uint32_t *firstblocknump,int32_t writeflag,void *transactions,struct ledger_state *hash,uint8_t *data,int32_t *hexlenp,char *hexstr,uint32_t blocknum)
{
    uint32_t rawind = 0;
    *hexlenp = (int32_t)strlen(hexstr) >> 1;
    if ( *hexlenp < 65000 )
    {
        decode_hex(data,*hexlenp,hexstr);
        rawind = ledger_rawind(firstblocknump,writeflag,transactions,hash,data,*hexlenp,blocknum);
        if ( *hexlenp > 0xfd )
            printf("hexlen.%d (%s) -> rawind.%u\n",*hexlenp,hexstr,rawind);
    }
    else  printf("hexlen overflow (%s) -> %d\n",hexstr,*hexlenp);
    return(rawind);
}

uint32_t has_duplicate_txid(struct ledger_info *ledger,char *coinstr,uint32_t blocknum,char *txidstr)
{
    uint8_t data[256]; uint32_t *ptr,pair[2],rawind = 0; int32_t hexlen,size = sizeof(pair);
    if ( strcmp(coinstr,"BTC") == 0 )
    {
        if ( (blocknum == 91842 && strcmp(txidstr,"d5d27987d2a3dfc724e359870c6644b40e497bdc0589a033220fe15429d88599") == 0) || (blocknum == 91880 && strcmp(txidstr,"e3bf3d07d4b0375638d5f1db5255fe07ba2c4cb067cd81b84ee974b6585fb468") == 0) )
        {
            hexlen = (int32_t)strlen(txidstr) >> 1;
            if ( hexlen < 255 )
            {
                decode_hex(data,hexlen,txidstr);
                if ( (ptr= db777_get(&pair,&size,ledger->DBs.transactions,ledger->txids.DB,data,hexlen)) != 0 )
                {
                    rawind = pair[0];
                    if ( Debuglevel > 2 )
                        printf("block.%u (%s) already exists.%u\n",blocknum,txidstr,*ptr);
                }
            }
        }
    }
    return(rawind);
}

uint32_t ledger_addrind(uint32_t *firstblocknump,struct ledger_info *ledger,char *coinaddr)
{
    return(ledger_rawind(firstblocknump,-1,ledger->DBs.transactions,&ledger->addrs,coinaddr,(int32_t)strlen(coinaddr),0));
}

uint32_t ledger_txidind(uint32_t *firstblocknump,struct ledger_info *ledger,char *txidstr)
{
    uint8_t txid[256]; int32_t txidlen;
    return(ledger_hexind(firstblocknump,-1,ledger->DBs.transactions,&ledger->txids,txid,&txidlen,txidstr,0));
}

uint32_t ledger_scriptind(uint32_t *firstblocknump,struct ledger_info *ledger,char *scriptstr)
{
    uint8_t script[4096]; int32_t scriptlen;
    return(ledger_hexind(firstblocknump,-1,ledger->DBs.transactions,&ledger->scripts,script,&scriptlen,scriptstr,0));
}

int32_t ledger_upairset(struct ledger_info *ledger,uint32_t txidind,uint32_t firstvout,uint32_t firstvin)
{
    struct upair32 firstinds;
    firstinds.firstvout = firstvout, firstinds.firstvin = firstvin;
    if ( firstvout == 0 )
        printf("illegal firstvout.0 for txidind.%d\n",txidind), debugstop();
    if ( Debuglevel > 2 || firstvout == 0 )
        printf(" db777_add txidind.%u <- SET firstvout.%d firstvin.%d\n",txidind,firstvout,firstvin);
    if ( db777_add(-1,ledger->DBs.transactions,ledger->txoffsets.DB,&txidind,sizeof(txidind),&firstinds,sizeof(firstinds)) == 0 )
        return(0);
    printf("error db777_add txidind.%u <- SET firstvout.%d\n",txidind,firstvout), debugstop();
    return(-1);
}

uint32_t ledger_firstvout(int32_t requiredflag,struct ledger_info *ledger,uint32_t txidind)
{
    struct upair32 firstinds,*ptr; int32_t flag,size = sizeof(firstinds); uint32_t firstvout = 0;
    if ( txidind == 1 )
        return(1);
    flag = 0;
    if ( (ptr= db777_get(&firstinds,&size,ledger->DBs.transactions,ledger->txoffsets.DB,&txidind,sizeof(txidind))) != 0 && size == sizeof(firstinds) )
        firstvout = ptr->firstvout, flag = 1;
    else if ( requiredflag != 0 )
        printf("couldnt find txoffset for txidind.%u size.%d vs %ld\n",txidind,size,sizeof(firstinds)), debugstop();
    if ( Debuglevel > 2 || (requiredflag != 0 && (firstvout == 0 || flag == 0)) )
        printf("search txidind.%u GET -> firstvout.%d, flag.%d\n",txidind,firstvout,flag);
    return(firstvout);
}

struct ledger_blockinfo *ledger_startblock(struct ledger_info *ledger,struct alloc_space *mem,uint32_t blocknum,uint64_t minted,int32_t numtx,uint32_t timestamp,char *blockhashstr)
{
    struct ledger_blockinfo *block;
    if ( ledger->blockpending != 0 )
    {
        printf("ledger_startblock: cant startblock when %s %u is pending\n",ledger->DBs.coinstr,ledger->blocknum);
        return(0);
    }
    ledger->blockpending = 1, ledger->blocknum = blocknum;
    block = memalloc(mem,sizeof(*block),1);
    block->minted = minted, block->numtx = numtx, block->blocknum = blocknum, block->timestamp = timestamp;
    if ( strlen(blockhashstr) == 64 )
        decode_hex(block->blockhash,32,blockhashstr);//, printf("blockhash.(%s) %x\n",blockhashstr,*(int *)block->blockhash);
    else printf("warning blockhash len.%ld\n",strlen(blockhashstr));
    return(block);
}

uint32_t ledger_addtx(struct ledger_info *ledger,struct alloc_space *mem,uint32_t txidind,char *txidstr,uint32_t totalvouts,uint16_t numvouts,uint32_t totalspends,uint16_t numvins,uint32_t blocknum)
{
    uint32_t checkind; uint8_t txid[256]; struct ledger_txinfo tx; int32_t txidlen;
    if ( Debuglevel > 2 )
        printf("ledger_tx txidind.%d %s vouts.%d vins.%d | ledger->txoffsets.ind %d\n",txidind,txidstr,totalvouts,totalspends,ledger->txoffsets.ind);
    if ( (checkind= ledger_hexind(0,1,ledger->DBs.transactions,&ledger->txids,txid,&txidlen,txidstr,blocknum)) == txidind )
    {
         memset(&tx,0,sizeof(tx));
        tx.firstvout = totalvouts, tx.firstvin = totalspends, tx.numvouts = numvouts, tx.numvins = numvins;
        tx.txidlen = txidlen, memcpy(tx.txid,txid,txidlen);
        ledger_upairset(ledger,txidind+1,totalvouts + numvouts,totalspends + numvins);
        return(ledger_packtx(ledger->txoffsets.sha256,&ledger->txoffsets.state,mem,&tx));
    } else printf("ledger_tx: (%s) mismatched txidind, expected %u got %u\n",txidstr,txidind,checkind), debugstop();
    return(0);
}

uint32_t ledger_addunspent(uint16_t *numaddrsp,uint16_t *numscriptsp,struct ledger_info *ledger,struct alloc_space *mem,uint32_t txidind,uint16_t v,uint32_t unspentind,char *coinaddr,char *scriptstr,uint64_t value,uint32_t blocknum,char *txidstr,int32_t txind)
{
    struct ledger_voutdata vout; uint32_t firstblocknum;
    memset(&vout,0,sizeof(vout));
    vout.U.value = value;
    ledger->voutsum += value;
    //printf("%.8f ",dstr(value));
    if ( (vout.U.scriptind= ledger_hexind(0,1,ledger->DBs.transactions,&ledger->scripts,vout.script,&vout.scriptlen,scriptstr,blocknum)) == 0 )
    {
        printf("ledger_unspent: error getting scriptind.(%s)\n",scriptstr);
        return(0);
    }
    vout.newscript = (vout.scriptind == ledger->scripts.ind);
    (*numscriptsp) += vout.newscript;
    vout.addrlen = (int32_t)strlen(coinaddr);
    if ( vout.addrlen < ledger->revaddrs.DB->valuesize )
    {
        memset(&coinaddr[vout.addrlen],0,ledger->revaddrs.DB->valuesize - vout.addrlen);
        vout.addrlen = ledger->revaddrs.DB->valuesize;
    }
    if ( (vout.U.ind= ledger_rawind(&firstblocknum,1,ledger->DBs.transactions,&ledger->addrs,coinaddr,vout.addrlen,blocknum)) != 0 )
    {
        if ( vout.U.ind == ledger->addrs.ind )
            vout.newaddr = 1, strcpy(vout.coinaddr,coinaddr), (*numaddrsp)++;
        update_sha256(ledger->unspentmap.sha256,&ledger->unspentmap.state,(void *)&vout.U,sizeof(vout.U));
        ledger->unspentmap.ind = unspentind;
        if ( db777_add(-1,ledger->DBs.transactions,ledger->unspentmap.DB,&unspentind,sizeof(unspentind),&vout.U,sizeof(vout.U)) != 0 )
            printf("error saving unspentmap (%s) %u -> %u %.8f\n",ledger->DBs.coinstr,unspentind,vout.U.ind,dstr(value));
        //if ( pass == 1 )
        {
            if ( Debuglevel > 2 )
                printf("txidind.%u v.%d unspent.%d (%s).%u (%s).%u %.8f | %ld\n",txidind,v,unspentind,coinaddr,vout.U.ind,scriptstr,vout.scriptind,dstr(value),sizeof(vout.U));
            addrinfo_update(ledger,coinaddr,vout.addrlen,value,unspentind,vout.U.ind,blocknum,txidstr,v,scriptstr,txind,vout.U.scriptind);
        }
        return(ledger_packvout(ledger->addrinfos.sha256,&ledger->addrinfos.state,mem,&vout));
    } else printf("ledger_unspent: cant find addrind.(%s)\n",coinaddr), debugstop();
    return(0);
}

uint32_t ledger_addspend(struct ledger_info *ledger,struct alloc_space *mem,uint32_t txidind,uint32_t totalspends,char *spent_txidstr,uint16_t spent_vout,uint32_t blocknum,char *txidstr,int32_t v)
{
    struct ledger_spendinfo spend;
    int32_t txidlen; uint8_t txid[256]; uint32_t spent_txidind,addrind,scriptind; uint64_t value,balance;
    if ( Debuglevel > 2 )
        printf("txidind.%d totalspends.%d (%s).v%d\n",txidind,totalspends,spent_txidstr,spent_vout);
    if ( (spent_txidind= ledger_hexind(0,0,ledger->DBs.transactions,&ledger->txids,txid,&txidlen,spent_txidstr,0)) != 0 )
    {
        memset(&spend,0,sizeof(spend));
        spend.spent_txidind = spent_txidind, spend.spent_vout = spent_vout;
        spend.unspentind = ledger_firstvout(1,ledger,spent_txidind) + spent_vout;
        //if ( pass == 1 )
        {
            ledger_spentbits(ledger,spend.unspentind,1);
            if ( Debuglevel > 2 )
                printf("spent_txidstr.(%s) -> spent_txidind.%u firstvout.%d\n",spent_txidstr,spent_txidind,spend.unspentind-spent_vout);
            if ( (value= ledger_unspentvalue(&addrind,&scriptind,ledger,spend.unspentind)) != 0 && addrind > 0 )
            {
                ledger->spendsum += value;
                balance = addrinfo_update(ledger,0,0,value,spend.unspentind | (1 << 31),addrind,blocknum,spent_txidstr,spent_vout,txidstr,v,scriptind);
                if ( Debuglevel > 2 )
                    printf("addrind.%u %.8f\n",addrind,dstr(balance));
            } else printf("error getting unspentmap for unspentind.%u (%s).v%d\n",spend.unspentind,spent_txidstr,spent_vout);
        }
        return(ledger_packspend(ledger->spentbits.sha256,&ledger->spentbits.state,mem,&spend));
    } else printf("ledger_spend: cant find txidind for (%s).v%d\n",spent_txidstr,spent_vout), debugstop();
    return(0);
}

int32_t ledger_finishblock(struct ledger_info *ledger,struct alloc_space *mem,struct ledger_blockinfo *block)
{
    uint32_t tmp;
    if ( ledger->blockpending == 0 || ledger->blocknum != block->blocknum )
    {
        printf("ledger_finishblock: mismatched parameter pending.%d (%d %d)\n",ledger->blockpending,ledger->blocknum,block->blocknum);
        return(0);
    }
    block->allocsize = (uint32_t)mem->used;
    block->crc16 = block_crc16(block);
    if ( Debuglevel > 2 )
        printf("block.%u mem.%p size.%d crc.%u\n",block->blocknum,mem,block->allocsize,block->crc16);
    tmp = block->blocknum + 1;
    ledger->blockpending = 0;
    if ( db777_add(-1,ledger->DBs.transactions,ledger->blocks.DB,&tmp,sizeof(tmp),block,block->allocsize) != 0 )
    {
        printf("error saving blocks %s %u %s\n",ledger->DBs.coinstr,block->blocknum,db777_errstr(ledger->DBs.ctl));
        return(0);
    }
    update_sha256(ledger->blocks.sha256,&ledger->blocks.state,(void *)block,block->allocsize);
    return(block->allocsize);
}

struct ledger_blockinfo *ledger_setblock(int32_t dispflag,struct ledger_info *ledger,struct alloc_space *mem,struct rawblock *emit,uint32_t blocknum)
{
    struct rawtx *tx; struct rawvin *vi; struct rawvout *vo; struct ledger_blockinfo *block = 0;
    uint32_t i,txidind,txind,n;
    tx = emit->txspace, vi = emit->vinspace, vo = emit->voutspace;
    block = ledger_startblock(ledger,mem,blocknum,emit->minted,emit->numtx,emit->timestamp,emit->blockhash);
    if ( block->numtx > 0 )
    {
        for (txind=0; txind<block->numtx; txind++,tx++)
        {
            if ( (txidind= has_duplicate_txid(ledger,ledger->DBs.coinstr,blocknum,tx->txidstr)) == 0 )
                txidind = ledger->txids.ind + 1;
            ledger_addtx(ledger,mem,txidind,tx->txidstr,ledger->unspentmap.ind+1,tx->numvouts,ledger->spentbits.ind+1,tx->numvins,blocknum);
            if ( (n= tx->numvouts) > 0 )
                for (i=0; i<n; i++,vo++,block->numvouts++)
                    ledger_addunspent(&block->numaddrs,&block->numscripts,ledger,mem,txidind,i,++ledger->unspentmap.ind,vo->coinaddr,vo->script,vo->value,blocknum,tx->txidstr,txind);
            if ( (n= tx->numvins) > 0 )
                for (i=0; i<n; i++,vi++,block->numvins++)
                    ledger_addspend(ledger,mem,txidind,++ledger->spentbits.ind,vi->txidstr,vi->vout,blocknum,tx->txidstr,i);
        }
    }
    if ( ledger_finishblock(ledger,mem,block) <= 0 )
        printf("error updating %s block.%u\n",ledger->DBs.coinstr,blocknum);
    return(block);
}

/*int32_t ledger_setblock(int32_t dispflag,struct ledger_info *ledger,struct alloc_space *mem,struct rawblock *emit,uint32_t blocknum)
{
    struct coin777 *coin = coin777_find(ledger->DBs.coinstr,0);
    if ( coin != 0 )
        return(coin777_parse(coin,coin,blocknum));
    return(0);
}*/

#endif
#endif
