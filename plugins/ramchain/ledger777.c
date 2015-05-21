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

int32_t ledger_ledgerhash(char *ledgerhash,struct ledger_inds *lp);
struct ledger_info *ledger_alloc(char *coinstr,char *subdir,int32_t flags);
int32_t ledger_update(struct rawblock *emit,struct ledger_info *ledger,struct alloc_space *mem,uint32_t RTblocknum,int32_t syncflag,int32_t minconfirms);
struct ledger_addrinfo *ledger_addrinfo(uint32_t *firstblocknump,struct ledger_info *ledger,char *coinaddr,uint32_t addrind);
uint64_t ledger_recalc_addrinfos(char *retbuf,int32_t maxlen,struct ledger_info *ledger,int32_t numrichlist);
//int32_t ledger_setlast(struct ledger_inds *L,struct ledger_info *ledger,uint32_t blocknum,int32_t numsyncs);
int32_t ledger_syncblocks(struct ledger_inds *inds,int32_t max,struct ledger_info *ledger);
int32_t ledger_startblocknum(struct ledger_info *ledger,uint32_t startblocknum);
struct ledger_blockinfo *ledger_setblocknum(struct ledger_info *ledger,struct alloc_space *mem,uint32_t startblocknum);
void ledger_free(struct ledger_info *ledger,int32_t closeDBflag);

uint32_t ledger_txidind(uint32_t *firstblocknump,struct ledger_info *ledger,char *txidstr);
uint32_t ledger_addrind(uint32_t *firstblocknump,struct ledger_info *ledger,char *coinaddr);
uint32_t ledger_scriptind(uint32_t *firstblocknump,struct ledger_info *ledger,char *scriptstr);
int32_t ledger_txidstr(struct ledger_info *ledger,char *txidstr,int32_t max,uint32_t txidind);
int32_t ledger_scriptstr(struct ledger_info *ledger,char *scriptstr,int32_t max,uint32_t scriptind);
int32_t ledger_coinaddr(struct ledger_info *ledger,char *coinaddr,int32_t max,uint32_t addrind);

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
    //while ( retries-- > 0 )
    {
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
            //sleep(10);
            //continue;
        }
        return(0);
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

int32_t ledger_txidstr(struct ledger_info *ledger,char *txidstr,int32_t max,uint32_t txidind)
{
    uint8_t txid[256],*ptr; int32_t size = sizeof(txid),retval = -1;
    if ( (ptr= db777_get(txid,&size,ledger->DBs.transactions,ledger->revtxids.DB,&txidind,sizeof(txidind))) != 0 )
    {
        if ( size < max/2 )
        {
            init_hexbytes_noT(txidstr,ptr,size);
            retval = 0;
        }
        else printf("txid.(%x) %d too long for %d\n",*ptr,size,max);
    }
    return(retval);
}

int32_t ledger_scriptstr(struct ledger_info *ledger,char *scriptstr,int32_t max,uint32_t scriptind)
{
    uint8_t script[4096],*ptr; int32_t size = sizeof(script),retval = -1;
    if ( (ptr= db777_get(script,&size,ledger->DBs.transactions,ledger->revscripts.DB,&scriptind,sizeof(scriptind))) != 0 )
    {
        if ( size < max/2 )
        {
            init_hexbytes_noT(scriptstr,ptr,size);
            retval = 0;
        }
        else printf("script.(%x) %d too long for %d\n",*ptr,size,max);
    }
    return(retval);
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

struct ledger_addrinfo *ledger_addrinfo(uint32_t *firstblocknump,struct ledger_info *ledger,char *coinaddr,uint32_t addrind)
{
    uint32_t pair[1]; int32_t len;
    // printf("ledger_addrinfo.%p %s\n",ledger,coinaddr);
    if ( ledger != 0 )
    {
        if ( addrind == 0 && coinaddr != 0 && coinaddr[0] != 0 )
            addrind = ledger_addrind(firstblocknump,ledger,coinaddr);
        if ( addrind != 0 )
        {
            len = sizeof(ledger->getbuf), pair[0] = addrind; //, pair[1] = ledger->numsyncs;
            return(db777_get(ledger->getbuf,&len,ledger->DBs.transactions,ledger->addrinfos.DB,pair,sizeof(pair)));
        }
    }
    return(0);
}

int32_t addrinfo_size(int32_t n) { return(sizeof(struct ledger_addrinfo) + (sizeof(struct unspentmap) * n)); }

struct ledger_addrinfo *addrinfo_alloc() { return(calloc(1,addrinfo_size(1))); }

uint64_t addrinfo_update(struct ledger_info *ledger,char *coinaddr,int32_t addrlen,uint64_t value,uint32_t unspentind,uint32_t addrind,uint32_t blocknum,char *txidstr,int32_t vout,char *extra,int32_t v,uint32_t scriptind)
{
    uint64_t balance = 0; int32_t i,n,allocflag = 0,allocsize,flag;
    char itembuf[8192],pubstr[8192],spendaddr[128]; uint32_t pair[1];
    struct ledger_addrinfo *addrinfo; struct unspentmap U;
    pair[0] = addrind; //, pair[1] = ledger->numsyncs;
    if ( Debuglevel > 2 )
        printf("addrinfo update block.%d size0 %d\n",blocknum,addrinfo_size(0));
    if ( (addrinfo= ledger_addrinfo(0,ledger,coinaddr,addrind)) == 0 )
    {
        addrinfo = addrinfo_alloc(), allocflag = 1;
        addrinfo->firstblocknum = blocknum;
        db777_set(DB777_RAM,ledger->DBs.transactions,ledger->addrinfos.DB,pair,sizeof(pair),addrinfo,addrinfo_size(0));
    }
    //addrtx[0] = addrind, addrtx[1] = ++addrinfo->txindex, pair[0] = unspentind, pair[1] = blocknum;
    //db777_add(-1,ledger->DBs.transactions,ledger->addrtx.DB,addrtx,sizeof(addrtx),pair,sizeof(pair)); // skip sha on this one to save time
    if ( (unspentind & (1 << 31)) != 0 )
    {
        unspentind &= ~(1 << 31);
        if ( (n= addrinfo->count) > 0 )
        {
            //while ( retries-- > 0 )
            {
                flag = 0;
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
                        //retries = -1;
                        flag = 1;
                        if ( addrinfo->count == 0 && balance != 0 )
                            printf("ILLEGAL: addrind.%u count.%d %.8f\n",addrind,addrinfo->count,dstr(balance)), debugstop();
                        break;
                    }
                }
                //if ( i == n )
                //    sleep(10);
            }
            if ( flag == 0 )
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
    db777_set(DB777_RAM,ledger->DBs.transactions,ledger->addrinfos.DB,pair,sizeof(pair),addrinfo,addrinfo_size(addrinfo->count));
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

int32_t ledger_unspentmap(char *txidstr,struct ledger_info *ledger,uint32_t unspentind)
{
    uint32_t floor,ceiling,probe,firstvout,lastvout;
    floor = 1, ceiling = ledger->txids.ind;
    while ( floor != ceiling )
    {
        probe = (floor + ceiling) >> 1;
        if ( (firstvout= ledger_firstvout(1,ledger,probe)) == 0 || (lastvout= ledger_firstvout(1,ledger,probe+1)) == 0 )
            break;
        //printf("search %u, probe.%u (%u %u) floor.%u ceiling.%u\n",unspentind,probe,firstvout,lastvout,floor,ceiling);
        if ( unspentind < firstvout )
            ceiling = probe;
        else if ( unspentind >= lastvout )
            floor = probe;
        else
        {
            //printf("found match! txidind.%u\n",probe);
            if ( ledger_txidstr(ledger,txidstr,255,probe) == 0 )
                return(unspentind - firstvout);
            else break;
        }
    }
    printf("end search %u, probe.%u (%u %u) floor.%u ceiling.%u\n",unspentind,probe,firstvout,lastvout,floor,ceiling);
    return(-1);
}

int32_t ledger_spentbits(struct ledger_info *ledger,uint32_t unspentind,uint8_t state)
{
    return(db777_add(-1,ledger->DBs.transactions,ledger->spentbits.DB,&unspentind,sizeof(unspentind),&state,sizeof(state)));
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
        if ( db777_add(-1,ledger->DBs.transactions,ledger->revtxids.DB,&txidind,sizeof(txidind),txid,txidlen) != 0 )
            printf("error saving txid.(%s) addrind.%u\n",txidstr,txidind);
        else db777_link(ledger->DBs.transactions,ledger->txids.DB,ledger->revtxids.DB,txidind,txid,txidlen);
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
    if ( db777_add(-1,ledger->DBs.transactions,ledger->revscripts.DB,&vout.U.scriptind,sizeof(vout.U.scriptind),vout.script,vout.scriptlen) != 0 )
        printf("error saving revscript.(%s) scriptind.%u\n",scriptstr,vout.U.scriptind);
    else db777_link(ledger->DBs.transactions,ledger->revscripts.DB,ledger->revscripts.DB,vout.U.scriptind,vout.script,vout.scriptlen);
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
        spend.unspentind = ledger_firstvout(1,ledger,spent_txidind) + spent_vout;
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

struct ledger_blockinfo *ledger_block(int32_t dispflag,struct ledger_info *ledger,struct alloc_space *mem,struct rawblock *emit,uint32_t blocknum)
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
    }
    else
    {
        for (i=0; i<n; i++)
            ledger_restorehash(lp,i,states[i]);
    }
#define LEDGER_NUMHASHES 9
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
    update_sha256(ledger->ledger.sha256,&ledger->ledger.state,0,0);
    update_sha256(ledger->ledger.sha256,&ledger->ledger.state,(void *)L,sizeof(*L));
    ledgerhash = *(uint32_t *)ledger->ledger.sha256;
    if ( numsyncs < 0 )
    {
        printf("\n");
        for (i=0; i<n; i++)
            printf("%08x ",*(int *)L->hashes[i]);
        printf(" blocknum.%u txids.%d addrs.%d scripts.%d unspents.%d supply %.8f | ",ledger->blocknum,ledger->txids.ind,ledger->addrs.ind,ledger->scripts.ind,ledger->unspentmap.ind,dstr(ledger->voutsum)-dstr(ledger->spendsum));
        printf(" %08x\n",_crc32(0,(void *)L,sizeof(*L)));
        for (i=0; i<(int)sizeof(*L); i++)
        {break;
            printf("%02x ",((uint8_t *)L)[i]);
            if ( (i % 32) == 31 && i+32<sizeof(*L) )
                printf("| %u\n",_crc32(0,&L[(i/32)*32],32));
        }
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
    struct ledger_inds *lp,L; uint32_t blocknum = 0;
    if ( (lp= ledger_getsyncdata(&L,ledger,synci)) == &L )
    {
        ledger->blocknum = blocknum = lp->blocknum, ledger->numsyncs = lp->numsyncs;
        ledger->voutsum = lp->voutsum, ledger->spendsum = lp->spendsum;
        ledger->addrs.ind = ledger->revaddrs.ind = lp->addrind;
        ledger->txids.ind = ledger->revtxids.ind = lp->txidind;
        ledger->scripts.ind = ledger->revscripts.ind = lp->scriptind;
        ledger->unspentmap.ind = lp->unspentind, ledger->spentbits.ind = lp->numspents;
        ledger->addrinfos.ind = lp->numaddrinfos, ledger->txoffsets.ind = lp->txoffsets;
        ledger_copyhashes(&L,ledger,1);
        printf("restored synci.%d: blocknum.%u txids.%d addrs.%d scripts.%d unspents.%d supply %.8f\n",synci,ledger->blocknum,ledger->txids.ind,ledger->addrs.ind,ledger->scripts.ind,ledger->unspentmap.ind,dstr(ledger->voutsum)-dstr(ledger->spendsum));//, sleep(3);
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
            ledger->DBs.transactions = sp_begin(ledger->DBs.env), ledger->numsyncs++;
        if ( (block= ledger_block(dispflag,ledger,mem,emit,blocknum)) != 0 )
        {
            ledger->addrsum = ledger_recalc_addrinfos(0,0,ledger,0);
            if ( syncflag != 0 )
            {
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
                extern uint32_t Duplicate,Mismatch,Added,Linked;
                printf("%.3f %-5s [lag %-5d] %-6u %.8f %.8f (%.8f) [%.8f] %13.8f | dur %.2f %.2f %.2f | len.%-5d %s %.1f | DMA %d %d %d %d %08x.%08x\n",ledger->calc_elapsed/1000.,ledger->DBs.coinstr,RTblocknum-blocknum,blocknum,dstr(supply),dstr(ledger->addrsum),dstr(supply)-dstr(ledger->addrsum),dstr(supply)-dstr(oldsupply),dstr(block->minted),elapsed,elapsed+(RTblocknum-blocknum)*ledger->calc_elapsed/60000,elapsed+estimate,block->allocsize,_mbstr(ledger->totalsize),(double)ledger->totalsize/blocknum,Duplicate,Mismatch,Added,Linked,*(uint32_t *)ledger->ledger.sha256,ledgerhash);
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
    ledger->DBs.transactions = (continueflag != 0) ? sp_begin(ledger->DBs.env) : 0;
    return(err);
}

uint64_t ledger_recalc_addrinfos(char *retbuf,int32_t maxlen,struct ledger_info *ledger,int32_t numrichlist)
{
    struct ledger_addrinfo *addrinfo;
    uint32_t i,n,addrind,itemlen; int32_t len; float *sortbuf; uint64_t balance,addrsum; char coinaddr[128],itembuf[128];
    addrsum = n = 0;
    if ( numrichlist == 0 || retbuf == 0 || maxlen == 0 )
    {
        for (addrind=1; addrind<=ledger->addrs.ind; addrind++)
        {
            //printf("recalc.%d of %d\n",addrind,ledger->addrs.ind);
            if ( (addrinfo= ledger_addrinfo(0,ledger,0,addrind)) != 0 )
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
            //printf("recalc.%d of %d\n",addrind,ledger->addrs.ind);
            if ( (addrinfo= ledger_addrinfo(0,ledger,0,addrind)) != 0 )
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
                    addrinfo = ledger_addrinfo(0,ledger,0,addrind);
                    ledger_coinaddr(ledger,coinaddr,sizeof(coinaddr),addrind);
                    sprintf(itembuf,"%s[\"%s\", \"%.8f\"],",i==0?"":" ",coinaddr,sortbuf[i << 1]);
                    itemlen = (int32_t)strlen(itembuf);
                    if ( len+itemlen < maxlen-32 )
                    {
                        memcpy(retbuf+len,itembuf,itemlen+1);
                        len += itemlen;
                    }
                }
            } else strcat(retbuf," ");
            strcat(retbuf + strlen(retbuf) - 1,"]}");
            //printf("(%s) top.%d of %d\n",retbuf,i,n);
        }
        free(sortbuf);
    }
    return(addrsum);
}

struct ledger_blockinfo *ledger_setblocknum(struct ledger_info *ledger,struct alloc_space *mem,uint32_t startblocknum)
{
    uint32_t addrind; int32_t modval,lastmodval,allocsize = sizeof(ledger->getbuf); uint64_t balance = 0;
    struct ledger_blockinfo *block; struct ledger_addrinfo *addrinfo;
    startblocknum = ledger_startblocknum(ledger,-1);
    //ledger->ledgerstate = ledger->ledger.state, memcpy(ledger->sha256,ledger->ledger.sha256,sizeof(ledger->sha256));
    if ( startblocknum < 1 )
    {
        ledger->numsyncs = 1;
        return(0);
    }
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
                    printf("%d%% ",modval), fflush(stdout), lastmodval = modval;
                if ( (addrind % 1000) == 0 )
                    fprintf(stderr,".");
                if ( (addrinfo= ledger_addrinfo(0,ledger,0,addrind)) != 0 )
                    balance += addrinfo->balance;
            }
            printf("balance %.8f endmilli %.0f\n",dstr(balance),milliseconds());
        } else printf("mismatched block: %u %u, crc16 %u %u\n",block->allocsize,allocsize,block_crc16(block),block->crc16), debugstop();
    } else printf("couldnt load block.%u\n",startblocknum), debugstop();
    return(block);
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
        
        ledger_stateinit(&ledger->DBs,&ledger->revaddrs,coinstr,subdir,"revaddrs","zstd",flags | DB777_KEY32,34);
        ledger_stateinit(&ledger->DBs,&ledger->revscripts,coinstr,subdir,"revscripts","zstd",flags,0);
        ledger_stateinit(&ledger->DBs,&ledger->revtxids,coinstr,subdir,"revtxids","zstd",flags | DB777_KEY32,32);
        ledger_stateinit(&ledger->DBs,&ledger->unspentmap,coinstr,subdir,"unspentmap","zstd",flags | DB777_KEY32 | RAMCHAINS.fastmode*DB777_RAM,sizeof(struct unspentmap));
        ledger_stateinit(&ledger->DBs,&ledger->txoffsets,coinstr,subdir,"txoffsets","zstd",flags | DB777_KEY32 | RAMCHAINS.fastmode*DB777_RAM,sizeof(struct upair32));
        ledger_stateinit(&ledger->DBs,&ledger->spentbits,coinstr,subdir,"spentbits","zstd",flags | DB777_KEY32,1);
        
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
