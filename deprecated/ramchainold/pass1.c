//
//  pass1.c
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_pass1_h
#define crypto777_pass1_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "coins777.c"
#include "system777.c"
#include "ledger777.c"

int32_t ledger_txidstr(struct ledger_info *ledger,char *txidstr,int32_t max,uint32_t txidind);
int32_t ledger_scriptstr(struct ledger_info *ledger,char *scriptstr,int32_t max,uint32_t scriptind);
int32_t ledger_coinaddr(struct ledger_info *ledger,char *coinaddr,int32_t max,uint32_t addrind);
int32_t ledger_unspentmap(char *txidstr,struct ledger_info *ledger,uint32_t unspentind);
int32_t ledger_spentbits(struct ledger_info *ledger,uint32_t unspentind,uint8_t state);
uint64_t ledger_unspentvalue(uint32_t *addrindp,uint32_t *scriptindp,struct ledger_info *ledger,uint32_t unspentind);

struct ledger_addrinfo *ledger_addrinfo(uint32_t *firstblocknump,struct ledger_info *ledger,char *coinaddr,uint32_t addrind);
uint64_t ledger_recalc_addrinfos(char *retbuf,int32_t maxlen,struct ledger_info *ledger,int32_t numrichlist);
uint64_t addrinfo_update(struct ledger_info *ledger,char *coinaddr,int32_t addrlen,uint64_t value,uint32_t unspentind,uint32_t addrind,uint32_t blocknum,char *txidstr,int32_t vout,char *extra,int32_t v,uint32_t scriptind);

#endif
#else
#ifndef crypto777_pass1_c
#define crypto777_pass1_c

#ifndef crypto777_pass1_h
#define DEFINES_ONLY
#include "pass1.c"
#undef DEFINES_ONLY
#endif

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

int32_t addrinfo_size(int32_t n) { return(sizeof(struct ledger_addrinfo) + (sizeof(struct unspentmap) * n)); }

struct ledger_addrinfo *addrinfo_alloc() { return(calloc(1,addrinfo_size(1))); }

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
    if ( startblocknum < 1 )
    {
        ledger->numsyncs = 1;
        return(0);
    }
    if ( (block= ledger_getblock((struct ledger_blockinfo *)ledger->getbuf,&allocsize,ledger,startblocknum)) != 0 )
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

/*
uint32_t ledger_unpacktx(uint8_t *hash,struct sha256_state *state,struct alloc_space *mem,struct ledger_txinfo *tx)
{
    int32_t allocsize;
    allocsize = sizeof(*tx) - sizeof(tx->txid) + tx->txidlen;
    memcpy(memalloc(mem,allocsize,0),tx,allocsize);
    update_sha256(hash,state,(uint8_t *)tx,allocsize);
    return(allocsize);
}

uint32_t ledger_unpackspend(uint8_t *hash,struct sha256_state *state,struct alloc_space *mem,struct ledger_spendinfo *spend)
{
    memcpy(memalloc(mem,sizeof(spend->unspentind),0),&spend->unspentind,sizeof(spend->unspentind));
    update_sha256(hash,state,(uint8_t *)&spend->unspentind,sizeof(spend->unspentind));
    return(sizeof(spend->unspentind));
}

uint32_t ledger_unpackvoutstr(struct alloc_space *mem,uint32_t rawind,int32_t newitem,uint8_t *str,uint16_t len)
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

uint32_t ledger_unpackvout(uint8_t *hash,struct sha256_state *state,struct alloc_space *mem,struct ledger_voutdata *vout)
{
    uint32_t allocsize; void *ptr;
    ptr = memalloc(mem,sizeof(vout->U.value),0);
    memcpy(ptr,&vout->U.value,sizeof(vout->U.value)), allocsize = sizeof(vout->U.value);
    allocsize += ledger_packvoutstr(mem,vout->U.ind,vout->newaddr,(uint8_t *)vout->coinaddr,vout->addrlen);
    allocsize += ledger_packvoutstr(mem,vout->scriptind,vout->newscript,vout->script,vout->scriptlen);
    update_sha256(hash,state,ptr,allocsize);
    return(allocsize);
}

struct ledger_blockinfo *ledger_block_pass1(int32_t dispflag,struct ledger_info *ledger,struct alloc_space *mem,struct rawblock *emit,uint32_t blocknum)
{
    struct ledger_blockinfo *block = 0;
    uint32_t i,txidind,txind,n;
    if ( (block= ledger_getblock(mem->ptr,(int32_t)mem->size,ledger,blocknum)) != 0 )
    {
        if ( block->numtx > 0 )
        {
            for (txind=0; txind<block->numtx; txind++)
            {
                ledger_gettx(ledger,mem,txidind,tx->txidstr,ledger->unspentmap.ind+1,tx->numvouts,ledger->spentbits.ind+1,tx->numvins,blocknum);
                if ( (n= tx->numvouts) > 0 )
                    for (i=0; i<n; i++,vo++,block->numvouts++)
                    {
                        addrinfo_update(ledger,coinaddr,vout.addrlen,value,unspentind,vout.U.ind,blocknum,txidstr,v,scriptstr,txind,vout.U.scriptind);
                        //ledger_addunspent(&block->numaddrs,&block->numscripts,ledger,mem,txidind,i,++ledger->unspentmap.ind,vo->coinaddr,vo->script,vo->value,blocknum,tx->txidstr,txind);
                    }
                if ( (n= tx->numvins) > 0 )
                    for (i=0; i<n; i++,vi++,block->numvins++)
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
                        //ledger_addspend(ledger,mem,txidind,++ledger->spentbits.ind,vi->txidstr,vi->vout,blocknum,tx->txidstr,i);
                    }
            }
        }
    }
}*/

/*struct packedvin { uint32_t txidstroffset; uint16_t vout; };
 struct packedvout { uint32_t coinaddroffset,scriptoffset; uint64_t value; };
 struct packedtx { uint16_t firstvin,numvins,firstvout,numvouts; uint32_t txidstroffset; };
 
 struct packedblock
 {
 uint16_t crc16,numtx,numrawvins,numrawvouts;
 uint64_t minted;
 uint32_t blocknum,timestamp,blockhash_offset,merkleroot_offset,txspace_offsets,vinspace_offsets,voutspace_offsets,allocsize;
 uint8_t rawdata[];
 };
 */
#endif
#endif
