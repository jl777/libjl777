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

struct ledger_inds
{
    uint64_t voutsum,spendsum;
    uint32_t blocknum,numsyncs,addrind,txidind,scriptind,unspentind,numspents,numaddrinfos,txoffsets;
    struct sha256_state hashstates[13]; uint8_t hashes[13][256 >> 3];
};

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

uint16_t block_crc16(struct ledger_blockinfo *block);
struct ledger_addrinfo *ledger_addrinfo(uint32_t *firstblocknump,struct ledger_info *ledger,char *coinaddr);
int32_t ledger_commit(struct ledger_info *ledger,int32_t continueflag);
uint64_t ledger_recalc_addrinfos(char *retbuf,int32_t maxlen,struct ledger_info *ledger,int32_t numrichlist);
int32_t ledger_setlast(struct ledger_info *ledger,uint32_t blocknum,uint32_t numsyncs);
int32_t ledger_startblocknum(struct ledger_info *ledger,uint32_t startblocknum);

struct ledger_blockinfo *ledger_update(int32_t dispflag,struct ledger_info *ledger,struct alloc_space *mem,char *name,char *serverport,char *userpass,struct rawblock *emit,uint32_t blocknum);
struct ledger_blockinfo *ledger_setblocknum(struct ledger_info *ledger,struct alloc_space *mem,uint32_t startblocknum);
int32_t ledger_ledgerhash(char *ledgerhash,struct ledger_inds *lp);
int32_t ledger_syncblocks(struct ledger_inds *inds,int32_t max,struct ledger_info *ledger);

#endif
#else
#ifndef crypto777_ledger777_c
#define crypto777_ledger777_c

#ifndef crypto777_ledger777_h
#define DEFINES_ONLY
#include "ledger777.c"
#undef DEFINES_ONLY
#endif

void debugstop ()
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
    uint8_t blen,extra = 1;
    if ( newitem != 0 )
    {
        rawind |= (1 << 31);
        memcpy(memalloc(mem,sizeof(rawind),0),&rawind,sizeof(rawind));
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
    else
    {
        memcpy(memalloc(mem,sizeof(rawind),0),&rawind,sizeof(rawind));
        return(sizeof(rawind));
    }
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
        return(rawind);
    }
    if ( writeflag != 0 )
    {
        rawind = ++hash->ind;
        pair[0] = rawind, pair[1] = blocknum;
        if ( Debuglevel > 2 || blocknum == 0 )
            printf("%p add rawind.%d keylen.%d: %llx | first appearance block.%u\n",hash->DB,rawind,keylen,*(long long *)key,blocknum);
        if ( db777_add(1,transactions,hash->DB,key,keylen,pair,sizeof(pair)) != 0 )
            printf("error adding to %s DB for rawind.%d keylen.%d\n",hash->name,rawind,keylen);
        else
        {
            update_sha256(hash->sha256,&hash->state,key,keylen);
            return(rawind);
        }
    }
    else
    {
        char hexstr[8192];
        init_hexbytes_noT(hexstr,key,keylen);
        printf("%s %p couldnt find expected %s keylen.%d\n",hash->DB->name,hash->DB,hexstr,keylen), debugstop(); // db777_dump(hash->DB,1,1),
    }
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
    if ( strcmp(coinstr,"BTC") == 0 && blocknum < 200000 )
    {
        hexlen = (int32_t)strlen(txidstr) >> 1;
        if ( hexlen < 255 )
        {
            decode_hex(data,hexlen,txidstr);
            //if ( (blocknum == 91842 && strcmp(txidstr,"d5d27987d2a3dfc724e359870c6644b40e497bdc0589a033220fe15429d88599") == 0) || (blocknum == 91880 && strcmp(txidstr,"e3bf3d07d4b0375638d5f1db5255fe07ba2c4cb067cd81b84ee974b6585fb468") == 0) )
            if ( (ptr= db777_get(&pair,&size,ledger->DBs.transactions,ledger->txids.DB,data,hexlen)) != 0 )
            {
                rawind = pair[0];
                if ( Debuglevel > 2 )
                    printf("block.%u (%s) already exists.%u\n",blocknum,txidstr,*ptr);
            }
        }
    }
    return(rawind);
}

uint32_t ledger_addrind(uint32_t *firstblocknump,struct ledger_info *ledger,char *coinaddr)
{
    return(ledger_rawind(firstblocknump,0,ledger->DBs.transactions,&ledger->addrs,coinaddr,(int32_t)strlen(coinaddr),0));
}

struct ledger_addrinfo *ledger_addrinfo(uint32_t *firstblocknump,struct ledger_info *ledger,char *coinaddr)
{
    uint32_t addrind; int32_t len = sizeof(ledger->getbuf);
    // printf("ledger_addrinfo.%p %s\n",ledger,coinaddr);
    if ( ledger != 0 && (addrind= ledger_addrind(firstblocknump,ledger,coinaddr)) > 0 )
        return(db777_get(ledger->getbuf,&len,ledger->DBs.transactions,ledger->addrinfos.DB,&addrind,sizeof(addrind)));
    else return(0);
}

int32_t ledger_coinaddr(struct ledger_info *ledger,char *coinaddr,int32_t max,uint32_t addrind)
{
    char *ptr; int32_t size = max,retval = -1;
    if ( (ptr= db777_get(coinaddr,&size,ledger->DBs.transactions,ledger->revaddrs.DB,&addrind,sizeof(addrind))) != 0 )
    {
        ptr[size] = 0;
        if ( size < max )
            strcpy(coinaddr,ptr), retval = 0;
        else printf("coinaddr.(%s) too long for %d\n",ptr,max);
    }
    return(retval);
}

int32_t addrinfo_size(int32_t n) { return(sizeof(struct ledger_addrinfo) + (sizeof(struct unspentmap) * n)); }

struct ledger_addrinfo *addrinfo_alloc() { return(calloc(1,addrinfo_size(1))); }

uint64_t addrinfo_update(struct ledger_info *ledger,char *coinaddr,int32_t addrlen,uint64_t value,uint32_t unspentind,uint32_t addrind,uint32_t blocknum,char *txidstr,int32_t vout,char *extra,int32_t v,uint32_t scriptind)
{
    uint64_t balance = 0; int32_t i,n,allocflag = 0,allocsize = sizeof(ledger->getbuf);
    char itembuf[8192],pubstr[8192],spendaddr[128]; // uint32_t addrtx[2],pair[2];
    struct ledger_addrinfo *addrinfo; struct unspentmap U;
    if ( Debuglevel > 2 )
        printf("addrinfo update block.%d size0 %d\n",blocknum,addrinfo_size(0));
    if ( (addrinfo= db777_get(ledger->getbuf,&allocsize,ledger->DBs.transactions,ledger->addrinfos.DB,&addrind,sizeof(addrind))) == 0 )
    {
        addrinfo = addrinfo_alloc(), allocflag = 1;
        addrinfo->firstblocknum = blocknum;
        db777_set(DB777_RAM,ledger->DBs.transactions,ledger->addrinfos.DB,&addrind,sizeof(addrind),addrinfo,addrinfo_size(0));
    }
    //addrtx[0] = addrind, addrtx[1] = ++addrinfo->txindex, pair[0] = unspentind, pair[1] = blocknum;
    //db777_add(-1,ledger->DBs.transactions,ledger->addrtx.DB,addrtx,sizeof(addrtx),pair,sizeof(pair)); // skip sha on this one to save time
    if ( (unspentind & (1 << 31)) != 0 )
    {
        unspentind &= ~(1 << 31);
        if ( (n= addrinfo->count) > 0 )
        {
            for (i=0; i<n; i++)
            {
                if ( unspentind == addrinfo->unspents[i].ind )
                {
                    addrinfo->balance -= value, balance = addrinfo->balance;
                    addrinfo->dirty = 1;
                    if ( Debuglevel > 2 )
                        printf("addrind.%u: i.%d count.%d remove %u -%.8f -> balace %.8f\n",addrind,i,addrinfo->count,unspentind,dstr(value),dstr(balance));
                    if ( addrinfo->notify != 0 )
                    {
                        // T balance, b blocknum, a -value, t this txidstr, v this vin, st spent_txidstr, sv spent_vout
                        ledger_coinaddr(ledger,spendaddr,sizeof(spendaddr),addrind);
                        sprintf(itembuf,"{\"%s\":\"%s\",\"T\":%.8f,\"b\":%u,\"a\":%.8f,\"t\":\"%s\",\"v\":%d,\"st\":\"%s\",\"sv\":%d,\"si\":%d}",ledger->DBs.coinstr,spendaddr,dstr(addrinfo->balance),blocknum,-dstr(value),extra,v,txidstr,vout,addrinfo->unspents[i].scriptind);
                        sprintf(pubstr,"{\"%s\":%s}","notify",itembuf);
                        //nn_publish(pubstr,1);
                    }
                    addrinfo->unspents[i] = addrinfo->unspents[--addrinfo->count];
                    memset(&addrinfo->unspents[addrinfo->count],0,sizeof(addrinfo->unspents[addrinfo->count]));
                    unspentind |= (1 << 31);
                    if ( addrinfo->count == 0 && balance != 0 )
                        printf("ILLEGAL: addrind.%u count.%d %.8f\n",addrind,addrinfo->count,dstr(balance)), debugstop();
                    break;
                }
            }
            if ( i == n )
            {
                printf("ERROR: addrind.%u cant find unspentind.%u in txlist with %d entries\n",addrind,unspentind,n), debugstop();
                return(0);
            }
        }
    }
    else
    {
        allocsize = addrinfo_size(addrinfo->count + 1);
        addrinfo->balance += value, balance = addrinfo->balance;
        if ( Debuglevel > 2 )
            printf("addrind.%u: add count.%d unspentind %u +%.8f -> balace %.8f\n",addrind,addrinfo->count,unspentind,dstr(value),dstr(balance));
        memset(&U,0,sizeof(U));
        U.value = value, U.ind = unspentind, U.scriptind = scriptind;
        addrinfo->unspents[addrinfo->count++] = U;
        addrinfo->dirty = 1;
        if ( addrinfo->notify != 0 )
        {
            // T balance, b blocknum, a value, t txidstr, v vout, s scriptstr, ti txind
            sprintf(itembuf,"{\"T\":%.8f,\"b\":%u,\"a\":%.8f,\"t\":\"%s\",\"v\":%d,\"s\":\"%s\",\"si\":%u}",dstr(*(uint64_t *)addrinfo->balance),blocknum,dstr(value),txidstr,vout,extra,scriptind);
            sprintf(pubstr,"{\"%s\":%s}",coinaddr,itembuf);
            //nn_publish(pubstr,1);
        }
    }
    update_sha256(ledger->addrinfos.sha256,&ledger->addrinfos.state,(void *)addrinfo,addrinfo_size(addrinfo->count));
    db777_set(DB777_RAM,ledger->DBs.transactions,ledger->addrinfos.DB,&addrind,sizeof(addrind),addrinfo,addrinfo_size(addrinfo->count));
    if ( allocflag != 0 )
        free(addrinfo);
    //printf("addrinfo.%p, entry.%p\n",addrinfo,entry);
    return(balance);
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

uint32_t ledger_firstvout(struct ledger_info *ledger,uint32_t txidind)
{
    struct upair32 firstinds,*ptr; int32_t size = sizeof(firstinds); uint32_t firstvout = 0;
    if ( txidind == 1 )
        return(1);
    if ( (ptr= db777_get(&firstinds,&size,ledger->DBs.transactions,ledger->txoffsets.DB,&txidind,sizeof(txidind))) != 0 && size == sizeof(firstinds) )
        firstvout = ptr->firstvout;
    else printf("couldnt find txoffset for txidind.%u size.%d vs %ld\n",txidind,size,sizeof(firstinds)), debugstop();
    if ( Debuglevel > 2 || firstvout == 0 )
        printf("search txidind.%u GET -> firstvout.%d\n",txidind,firstvout);
    return(firstvout);
}

int32_t ledger_spentbits(struct ledger_info *ledger,uint32_t unspentind,uint8_t state)
{
    return(db777_add(-1,ledger->DBs.transactions,ledger->spentbits.DB,&unspentind,sizeof(unspentind),&state,sizeof(state)));
}

// block iterators
uint32_t ledger_addtx(struct ledger_info *ledger,struct alloc_space *mem,uint32_t txidind,char *txidstr,uint32_t totalvouts,uint16_t numvouts,uint32_t totalspends,uint16_t numvins,uint32_t blocknum)
{
    uint32_t checkind; uint8_t txid[256]; struct ledger_txinfo tx; int32_t txidlen;
    if ( Debuglevel > 2 )
        printf("ledger_tx txidind.%d %s vouts.%d vins.%d | ledger->txoffsets.ind %d\n",txidind,txidstr,totalvouts,totalspends,ledger->txoffsets.ind);
    if ( (checkind= ledger_hexind(0,1,ledger->DBs.transactions,&ledger->txids,txid,&txidlen,txidstr,blocknum)) == txidind )
    {
        if ( db777_add(-1,ledger->DBs.transactions,ledger->revtxids.DB,&txidind,sizeof(txidind),txid,txidlen) != 0 )
            printf("error saving txid.(%s) addrind.%u\n",txidstr,txidind);
        else db777_link(ledger->DBs.transactions,ledger->txids.DB,ledger->revtxids.DB,txidind,txid,txidlen);
        memset(&tx,0,sizeof(tx));
        tx.firstvout = totalvouts, tx.firstvin = totalspends, tx.numvouts = numvouts, tx.numvins = numvins;
        tx.txidlen = txidlen, memcpy(tx.txid,txid,txidlen);
        ledger_upairset(ledger,txidind+1,totalvouts + numvouts,totalspends + numvins);
        return(ledger_packtx(ledger->txoffsets.sha256,&ledger->txoffsets.state,mem,&tx));
    } else printf("ledger_tx: mismatched txidind, expected %u got %u\n",txidind,checkind), debugstop();
    return(0);
}

uint64_t ledger_unspentvalue(uint32_t *addrindp,uint32_t *scriptindp,struct ledger_info *ledger,uint32_t unspentind)
{
    struct unspentmap *ptr,U; int32_t size = sizeof(U); uint64_t value = 0;
    *addrindp = 0;
    if ( (ptr= db777_get(&U,&size,ledger->DBs.transactions,ledger->unspentmap.DB,&unspentind,sizeof(unspentind))) != 0 && size == sizeof(U) )
    {
        value = ptr->value;
        *addrindp = ptr->ind;
        *scriptindp = ptr->scriptind;
        //printf("unspentmap.%u %.8f -> addrind.%u\n",unspentind,dstr(value),U->addrind);
    } else printf("unspentmap unexpectsize %d vs %ld\n",size,sizeof(U));
    return(value);
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
        ledger->unspentmap.ind = unspentind;
        if ( db777_add(-1,ledger->DBs.transactions,ledger->unspentmap.DB,&unspentind,sizeof(unspentind),&vout.U,sizeof(vout.U)) != 0 )
            printf("error saving unspentmap (%s) %u -> %u %.8f\n",ledger->DBs.coinstr,unspentind,vout.U.ind,dstr(value));
        update_sha256(ledger->unspentmap.sha256,&ledger->unspentmap.state,(void *)&vout.U,sizeof(vout.U));
        if ( vout.U.ind == ledger->addrs.ind )
        {
            vout.newaddr = 1, strcpy(vout.coinaddr,coinaddr), (*numaddrsp)++;
            if ( db777_add(-1,ledger->DBs.transactions,ledger->revaddrs.DB,&vout.U.ind,sizeof(vout.U.ind),coinaddr,vout.addrlen) != 0 )
                printf("error saving coinaddr.(%s) addrind.%u\n",coinaddr,vout.U.ind);
            else db777_link(ledger->DBs.transactions,ledger->addrs.DB,ledger->revaddrs.DB,vout.U.ind,coinaddr,vout.addrlen);
            update_sha256(ledger->revaddrs.sha256,&ledger->revaddrs.state,(void *)coinaddr,vout.addrlen);
        }
        if ( Debuglevel > 2 )
            printf("txidind.%u v.%d unspent.%d (%s).%u (%s).%u %.8f | %ld\n",txidind,v,unspentind,coinaddr,vout.U.ind,scriptstr,vout.scriptind,dstr(value),sizeof(vout.U));
        addrinfo_update(ledger,coinaddr,vout.addrlen,value,unspentind,vout.U.ind,blocknum,txidstr,v,scriptstr,txind,vout.U.scriptind);
        return(ledger_packvout(ledger->addrinfos.sha256,&ledger->addrinfos.state,mem,&vout));
    } else printf("ledger_unspent: cant find addrind.(%s)\n",coinaddr), debugstop();
    return(0);
}

uint32_t ledger_addspend(struct ledger_info *ledger,struct alloc_space *mem,uint32_t txidind,uint32_t totalspends,char *spent_txidstr,uint16_t spent_vout,uint32_t blocknum,char *txidstr,int32_t v)
{
    struct ledger_spendinfo spend;
    int32_t txidlen; uint64_t value,balance; uint8_t txid[256]; uint32_t spent_txidind,addrind,scriptind;
    if ( Debuglevel > 2 )
        printf("txidind.%d totalspends.%d (%s).v%d\n",txidind,totalspends,spent_txidstr,spent_vout);
    if ( (spent_txidind= ledger_hexind(0,0,ledger->DBs.transactions,&ledger->txids,txid,&txidlen,spent_txidstr,0)) != 0 )
    {
        memset(&spend,0,sizeof(spend));
        spend.spent_txidind = spent_txidind, spend.spent_vout = spent_vout;
        spend.unspentind = ledger_firstvout(ledger,spent_txidind) + spent_vout;
        ledger_spentbits(ledger,spend.unspentind,1);
        if ( Debuglevel > 2 )
            printf("spent_txidstr.(%s) -> spent_txidind.%u firstvout.%d\n",spent_txidstr,spent_txidind,spend.unspentind-spent_vout);
        if ( (value= ledger_unspentvalue(&addrind,&scriptind,ledger,spend.unspentind)) != 0 && addrind > 0 )
        {
            ledger->spendsum += value;
            balance = addrinfo_update(ledger,0,0,value,spend.unspentind | (1 << 31),addrind,blocknum,spent_txidstr,spent_vout,txidstr,v,scriptind);
            if ( Debuglevel > 2 )
                printf("addrind.%u %.8f\n",addrind,dstr(balance));
            return(ledger_packspend(ledger->spentbits.sha256,&ledger->spentbits.state,mem,&spend));
        } else printf("error getting unspentmap for unspentind.%u (%s).v%d\n",spend.unspentind,spent_txidstr,spent_vout);
    } else printf("ledger_spend: cant find txidind for (%s).v%d\n",spent_txidstr,spent_vout), debugstop();
    return(0);
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
        printf("error saving blocks %s %u\n",ledger->DBs.coinstr,block->blocknum);
        return(0);
    }
    update_sha256(ledger->blocks.sha256,&ledger->blocks.state,(void *)block,block->allocsize);
    return(block->allocsize);
}

struct ledger_blockinfo *ledger_update(int32_t dispflag,struct ledger_info *ledger,struct alloc_space *mem,char *name,char *serverport,char *userpass,struct rawblock *emit,uint32_t blocknum)
{
    struct rawtx *tx; struct rawvin *vi; struct rawvout *vo; struct ledger_blockinfo *block = 0;
    uint32_t i,txidind,txind,n;
    if ( rawblock_load(emit,name,serverport,userpass,blocknum) > 0 )
    {
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
            printf("error updating %s block.%u\n",name,blocknum);
    } else printf("error loading %s block.%u\n",ledger->DBs.coinstr,blocknum);
    return(block);
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
    ledger->DBs.transactions = (continueflag != 0) ? sp_begin(ledger->DBs.env) : 0;
    return(err);
}

uint64_t ledger_recalc_addrinfos(char *retbuf,int32_t maxlen,struct ledger_info *ledger,int32_t numrichlist)
{
    struct ledger_addrinfo *addrinfo;
    uint32_t i,n,addrind,itemlen; int32_t len,allocsize; float *sortbuf; uint64_t balance,addrsum; char coinaddr[128],itembuf[128];
    addrsum = n = 0;
    if ( numrichlist == 0 || retbuf == 0 || maxlen == 0 )
    {
        for (addrind=1; addrind<=ledger->addrs.ind; addrind++)
        {
            len = sizeof(ledger->getbuf);
            if ( (addrinfo= db777_get(ledger->getbuf,&len,ledger->DBs.transactions,ledger->addrinfos.DB,&addrind,sizeof(addrind))) != 0 )
            {
                if ( addrinfo->notify != 0 )
                    printf("notification active addind.%d count.%d size.%d\n",addrind,addrinfo->count,addrinfo_size(addrinfo->count));                addrsum += addrinfo->balance;
            }
        }
    }
    else
    {
        sortbuf = calloc(ledger->addrs.ind,sizeof(float)+sizeof(uint32_t));
        for (addrind=1; addrind<=ledger->addrs.ind; addrind++)
        {
            len = sizeof(ledger->getbuf);
            if ( (addrinfo= db777_get(ledger->getbuf,&len,ledger->DBs.transactions,ledger->addrinfos.DB,&addrind,sizeof(addrind))) != 0 )
            {
                balance = addrinfo->balance;
                addrsum += balance;
                sortbuf[n << 1] = dstr(balance);
                memcpy(&sortbuf[(n << 1) + 1],&addrind,sizeof(addrind));
                n++;
            }
        }
        if ( n > 0 )
        {
            revsortfs(sortbuf,n,sizeof(*sortbuf) * 2);
            sprintf(retbuf,"{\"supply\":%.8f,\"richlist\":[",dstr(addrsum)), len = (int32_t)strlen(retbuf);
            if ( numrichlist > 0 && n > 0 )
            {
                for (i=0; i<numrichlist&&i<n; i++)
                {
                    memcpy(&addrind,&sortbuf[(i << 1) + 1],sizeof(addrind));
                    allocsize = sizeof(ledger->getbuf);
                    addrinfo = db777_get(ledger->getbuf,&allocsize,ledger->DBs.transactions,ledger->addrinfos.DB,&addrind,sizeof(addrind));
                    ledger_coinaddr(ledger,coinaddr,sizeof(coinaddr),addrind);
                    sprintf(itembuf,"%s[\"%s\", \"%.8f\"],",i==0?"":" ",coinaddr,sortbuf[i << 1]);
                    itemlen = (int32_t)strlen(itembuf);
                    if ( len+itemlen < maxlen-32 )
                    {
                        memcpy(retbuf+len,itembuf,itemlen+1);
                        len += itemlen;
                    }
                }
            } else strcat(retbuf,"]}");
            strcat(retbuf + len - 1,"]}");
            //printf("(%s) top.%d of %d\n",retbuf,i,n);
        }
        free(sortbuf);
    }
    return(addrsum);
}

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
    states[n++] = &ledger->addrs, states[n++] = &ledger->revaddrs, states[n++] = &ledger->addrinfos;
    states[n++] = &ledger->txids, states[n++] = &ledger->txoffsets, states[n++] = &ledger->unspentmap;
    states[n++] = &ledger->scripts, states[n++] = &ledger->spentbits, states[n++] = &ledger->blocks;
    if ( restoreflag == 0 )
    {
        for (i=0; i<n; i++)
            ledger_copyhash(lp,i,states[i]);
        update_sha256(ledger->ledger.sha256,&ledger->ledger.state,(void *)lp,sizeof(*lp));
        ledger_copyhash(lp,n++,&ledger->ledger);
    }
    else
    {
        for (i=0; i<n; i++)
            ledger_restorehash(lp,i,states[i]);
        ledger_restorehash(lp,n++,&ledger->ledger);
    }
#define LEDGER_NUMHASHES 10
    if ( n != LEDGER_NUMHASHES )
        printf("mismatched LEDGER_NUMHASHES %d != %d\n",n,LEDGER_NUMHASHES);
    if ( n > (int32_t)(sizeof(lp->hashes)/sizeof(*lp->hashes)) )
        printf("Too many hashes to save them %d vs %ld\n",n,(sizeof(lp->hashes)/sizeof(lp->hashes)));
    return(n);
}

int32_t ledger_setlast(struct ledger_info *ledger,uint32_t blocknum,uint32_t numsyncs)
{
    struct ledger_inds L; int32_t retval;
    memset(&L,0,sizeof(L));
    L.blocknum = ledger->blocknum, L.numsyncs = ledger->numsyncs;
    L.voutsum = ledger->voutsum, L.spendsum = ledger->spendsum;
    L.addrind = ledger->addrs.ind, L.txidind = ledger->txids.ind, L.scriptind = ledger->scripts.ind;
    L.unspentind = ledger->unspentmap.ind, L.numspents = ledger->spentbits.ind;
    L.numaddrinfos = ledger->addrinfos.ind, L.txoffsets = ledger->txoffsets.ind;
    ledger_copyhashes(&L,ledger,0);
    printf("SYNCNUM.%d -> %d supply %.8f | ledgerhash %llx\n",numsyncs,blocknum,dstr(L.voutsum)-dstr(L.spendsum),*(long long *)ledger->ledger.sha256);
    if ( numsyncs > 0 )
    {
        if ( db777_set(DB777_HDD,ledger->DBs.transactions,ledger->ledger.DB,&numsyncs,sizeof(numsyncs),&L,sizeof(L)) != 0 )
            printf("error saving ledger\n");
        //allocsize = sizeof(checkL);
        //if ( (ptr= db777_get(&checkL,&allocsize,ledger->DBs.transactions,ledger->ledger.DB,&numsyncs,sizeof(numsyncs))) != 0 )
        //    printf("got checkval.%d/%d vs numsyncs.%d\n",checkL.blocknum,blocknum,numsyncs);
        //else printf("ptr.%p error getting ledger.DB numsync.%d checkL.blocknum %d allocsize.%d | transactions.%p\n",ptr,numsyncs,checkL.blocknum,allocsize,ledger->DBs.transactions);
    }
    numsyncs = 0;
    if ( (retval = db777_set(DB777_HDD,ledger->DBs.transactions,ledger->ledger.DB,&numsyncs,sizeof(numsyncs),&L,sizeof(L))) != 0 )
        printf("error saving numsyncs.0 retval.%d\n",retval);
    //allocsize = sizeof(checkL);
    //if ( (ptr= db777_get(&checkL,&allocsize,ledger->DBs.transactions,ledger->ledger.DB,&numsyncs,sizeof(numsyncs))) != 0 )
    //    printf("got checkval.%d/%d vs numsyncs.%d\n",L.blocknum,blocknum,numsyncs);
    //else printf("ptr.%p errorB getting ledger.DB numsync.%d checkL.blocknum %d allocsize.%d | transactions.%p\n",ptr,numsyncs,checkL.blocknum,allocsize,ledger->DBs.transactions);
    return(retval);
}

struct ledger_inds *ledger_getsyncdata(struct ledger_inds *L,struct ledger_info *ledger,uint32_t syncind)
{
    struct ledger_inds *lp; int32_t allocsize = sizeof(*L);
    if ( (lp= db777_get(L,&allocsize,ledger->DBs.transactions,ledger->ledger.DB,&syncind,sizeof(syncind))) != 0 )
        return(lp);
    else memset(L,0,sizeof(*L));
    printf("couldnt find syncind.%d keylen.%ld\n",syncind,sizeof(syncind));
    return(0);
}

int32_t ledger_startblocknum(struct ledger_info *ledger,uint32_t startblocknum)
{
    struct ledger_inds *lp,L; uint32_t blocknum = 0;
    if ( (lp= ledger_getsyncdata(&L,ledger,0)) == &L )
    {
        ledger->blocknum = blocknum = lp->blocknum, ledger->numsyncs = lp->numsyncs;
        ledger->voutsum = lp->voutsum, ledger->spendsum = lp->spendsum;
        ledger->addrs.ind = ledger->revaddrs.ind = lp->addrind;
        ledger->txids.ind = ledger->revtxids.ind = lp->txidind;
        ledger->scripts.ind = lp->scriptind;
        ledger->unspentmap.ind = lp->unspentind, ledger->spentbits.ind = lp->numspents;
        ledger->addrinfos.ind = lp->numaddrinfos, ledger->txoffsets.ind = lp->txoffsets;
        ledger_copyhashes(&L,ledger,1);
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

int32_t ledger_syncblocks(struct ledger_inds *inds,int32_t max,struct ledger_info *ledger)
{
    struct ledger_inds L,*lp; uint32_t numsyncs,i,n = 0;
    if ( (lp= ledger_getsyncdata(&L,ledger,0)) != 0 )
    {
        inds[n++] = *lp;
        numsyncs = lp->numsyncs;
        for (i=numsyncs-1; i>0; i--)
        {
            if ( (lp= ledger_getsyncdata(&L,ledger,i)) != 0 )
                inds[n++] = *lp;
        }
    } else printf("null return from ledger_getsyncdata\n");
    return(n);
}

#ifdef ENABLE_RECONSTRUCTION
struct ledger_addrinfo *ledger_reconstruct_addrinfo(struct ledger_info *ledger,struct alloc_space *mem,uint32_t addrind,uint32_t startblocknum)
{
    uint32_t *ptr,addrtx[2],pair[2],unspentind,blocknum,checkind; struct ledger_addrinfo *addrinfo;
    int32_t strange,i,n = 0,len; uint64_t value; int64_t balance;
    addrtx[0] = addrind, addrtx[1] = 0;
    mem->used = 0;
    addrinfo = memalloc(mem,addrinfo_size(0),1);
    strange = 0;
    for (addrtx[1]=1; addrtx[1]<=ledger->unspentmap.ind; addrtx[1]++)
    {
        len = sizeof(pair);
        if ( (ptr= db777_get(pair,&len,0,ledger->ledger.DB,addrtx,sizeof(addrtx))) != 0 && len == sizeof(pair) )
        {
            unspentind = ptr[0], blocknum = ptr[1];
            if ( blocknum < startblocknum )
            {
                if ( (unspentind & (1 << 31)) != 0 )
                {
                    unspentind &= ~(1 << 31);
                    if ( n > 0 )
                    {
                        for (i=0; i<n; i++)
                        {
                            if ( unspentind == addrinfo->unspents[i] )
                            {
                                //printf("-%u ",unspentind);
                                addrinfo->unspents[i] = addrinfo->unspents[--n];
                                addrinfo->unspents[n] = 0;
                                mem->used -= sizeof(unspentind);
                                //balance -= value;
                                break;
                            }
                        }
                    } else i = 0;
                    if ( i == n && n > 2 )
                        strange++;//, printf("addrind.%d txindex.%d couldnt find unspentind.%d out of %d unspents\n",addrind,addrtx[1],unspentind,n);
                }
                else mem->used += sizeof(unspentind), addrinfo->unspentinds[n++] = unspentind;
                //printf("addrind.%u %s txindex.%d unspentind.%d %s %.8f blocknum.%u\n",addrind,addrinfo->coinaddr,addrtx[1],unspentind & ~(1<<31),(unspentind & (1<<31))!=0?"SPEND":"",dstr(value),blocknum);
            }
            else
            {
                //printf("addrind.%d addrtx.%d blocknum.%u >= startblocknum.%u\n",addrind,addrtx[1],blocknum,startblocknum);
                break;
            }
        }
        else
        {
            //printf("numtx.%d\n",addrtx[1]);
            break;
        }
    }
    /*balance = 0;
    for (i=0; i<n; i++)
    {
        value = ledger_unspentvalue(&checkind,ledger,addrinfo->unspentinds[i]);
        if ( checkind == addrind )
            balance += value;//, printf("%.8f ",dstr(value));
        else printf("checkind.%d mismatch to addrind.%d with %.8f\n",checkind,addrind,dstr(value));
    }
    addrinfo->txindex = addrtx[1] - 1;
    memcpy(addrinfo->balance,&balance,sizeof(balance));*/
    addrinfo->balance = balance;
    addrinfo->count = n;
    if ( strange != 0 )
        fprintf(stderr,"%d ",strange);
    //printf("-> balance %.8f for %s\n",dstr(balance),addrinfo->coinaddr);
    return(addrinfo);
}
#endif

struct ledger_blockinfo *ledger_setblocknum(struct ledger_info *ledger,struct alloc_space *mem,uint32_t startblocknum)
{
    uint32_t addrind; int32_t modval,lastmodval,allocsize = sizeof(ledger->getbuf); uint64_t balance = 0;
    struct ledger_blockinfo *block; struct ledger_addrinfo *addrinfo;
    startblocknum = ledger_startblocknum(ledger,0);
    if ( startblocknum < 1 )
        return(0);
    if ( (block= db777_get(ledger->getbuf,&allocsize,0,ledger->blocks.DB,&startblocknum,sizeof(startblocknum))) != 0 )
    {
        if ( block->allocsize == allocsize && block_crc16(block) == block->crc16 )
        {
            printf("%.8f startmilli %.0f start.%u block.%u ledger block.%u, inds.(txid %d addrs %d scripts %d vouts %d vins %d)\n",dstr(ledger->voutsum)-dstr(ledger->spendsum),milliseconds(),startblocknum,block->blocknum,ledger->blocknum,ledger->txids.ind,ledger->addrs.ind,ledger->scripts.ind,ledger->unspentmap.ind,ledger->spentbits.ind);
            lastmodval = -1;
            for (addrind=1; addrind<=ledger->addrs.ind; addrind++)
            {
                modval = ((100. * addrind) / (ledger->addrs.ind + 1));
                if ( modval != lastmodval )
                    fprintf(stderr,"%d%% ",modval), lastmodval = modval;
                allocsize = sizeof(ledger->getbuf);
                if ( (addrinfo= db777_get(ledger->getbuf,&allocsize,0,ledger->addrinfos.DB,&addrind,sizeof(addrind))) != 0 )
                    balance += addrinfo->balance;
            }
            printf("balance %.8f endmilli %.0f\n",dstr(balance),milliseconds());
        } else printf("mismatched block: %u %u, crc16 %u %u\n",block->allocsize,allocsize,block_crc16(block),block->crc16);
    } else printf("couldnt load block.%u\n",startblocknum);
    return(block);
}

#endif
#endif
