//
//  ramchain.c
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//


#define BUNDLED
#define PLUGINSTR "ramchain"
#define PLUGNAME(NAME) ramchain ## NAME
#define STRUCTNAME struct PLUGNAME(_info) 
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
#include "../plugin777.c"
#include "storage.c"
#include "system777.c"
#undef DEFINES_ONLY

void debugstop ()
{
#ifdef __APPLE__
    getchar();
#endif
}

STRUCTNAME RAMCHAINS;
char *PLUGNAME(_methods)[] = { "create", "backup", "pause", "resume", "stop", "notify", "richlist" }; // list of supported methods

struct ledger_inds
{
    uint64_t voutsum,spendsum;
    uint32_t blocknum,numsyncs,addrind,txidind,scriptind,unspentind,numspents,numaddrinfos,txoffsets;
};

struct ledger_blockinfo
{
    uint16_t crc16,numtx,numaddrs,numscripts,numvouts,numvins;
    uint32_t blocknum,allocsize;//,txidind,addrind,scriptind,unspentind,totalspends,allocsize,numsyncs;
    uint64_t minted;//,voutsum,spendsum;
    uint8_t transactions[];
};
struct ledger_txinfo { uint32_t firstvout,firstvin; uint16_t numvouts,numvins; uint8_t txidlen,txid[255]; };
struct ledger_spendinfo { uint32_t unspentind,spent_txidind; uint16_t spent_vout; };
struct unspentmap { uint32_t addrind; uint32_t value[2]; };
struct ledger_voutdata { struct unspentmap U; uint32_t scriptind; int32_t addrlen,scriptlen,newscript,newaddr; char coinaddr[256]; uint8_t script[256]; };

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
    //update_sha256(hash,state,(uint8_t *)tx,allocsize);
    return(allocsize);
}

uint32_t ledger_packspend(uint8_t *hash,struct sha256_state *state,struct alloc_space *mem,struct ledger_spendinfo *spend)
{
    memcpy(memalloc(mem,sizeof(spend->unspentind),0),&spend->unspentind,sizeof(spend->unspentind));
    //update_sha256(hash,state,(uint8_t *)&spend->unspentind,sizeof(spend->unspentind));
    return(sizeof(spend->unspentind));
}

uint32_t ledger_packvoutstr(struct alloc_space *mem,uint32_t rawind,int32_t newitem,uint8_t *str,uint8_t len)
{
    if ( newitem != 0 )
    {
        rawind |= (1 << 31);
        memcpy(memalloc(mem,sizeof(rawind),0),&rawind,sizeof(rawind));
        memcpy(memalloc(mem,sizeof(len),0),&len,sizeof(len));
        memcpy(memalloc(mem,len,0),str,len);
        return(sizeof(rawind) + sizeof(len) + len);
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
    allocsize += ledger_packvoutstr(mem,vout->U.addrind,vout->newaddr,(uint8_t *)vout->coinaddr,vout->addrlen);
    allocsize += ledger_packvoutstr(mem,vout->scriptind,vout->newscript,vout->script,vout->scriptlen);
    //update_sha256(hash,state,ptr,allocsize);
    return(allocsize);
}

uint32_t ledger_rawind(int32_t writeflag,void *transactions,struct ledger_state *hash,void *key,int32_t keylen)
{
    int32_t size; uint32_t *ptr,rawind = 0;
    if ( (ptr= db777_findM(&size,transactions,hash->D.DB,key,keylen)) != 0 )
    {
        if ( size == sizeof(uint32_t) )
        {
            rawind = *ptr;
            if ( (rawind - 1) == hash->ind )
                hash->ind = rawind;
        }
        else printf("error unexpected size.%d for (%s) keylen.%d\n",size,hash->name,keylen);
        free(ptr);
        return(rawind);
    }
    if ( writeflag != 0 )
    {
        rawind = ++hash->ind;
        //printf("add rawind.%d keylen.%d\n",rawind,keylen);
        if ( db777_add(-1,transactions,hash->D.DB,key,keylen,&rawind,sizeof(rawind)) != 0 )
            printf("error adding to %s DB for rawind.%d keylen.%d\n",hash->name,rawind,keylen);
        else
        {
            //update_sha256(hash->sha256,&hash->state,key,keylen);
            return(rawind);
        }
    }
    else printf("couldnt find expected %llx keylen.%d\n",*(long long *)key,keylen), debugstop();
    return(0);
}

uint32_t ledger_hexind(int32_t writeflag,void *transactions,struct ledger_state *hash,uint8_t *data,int32_t *hexlenp,char *hexstr)
{
    uint32_t rawind = 0;
    *hexlenp = (int32_t)strlen(hexstr) >> 1;
    if ( *hexlenp < 255 )
    {
        decode_hex(data,*hexlenp,hexstr);
        rawind = ledger_rawind(writeflag,transactions,hash,data,*hexlenp);
        //printf("hexlen.%d (%s) -> rawind.%u\n",hexlen,hexstr,rawind);
    }
    else  printf("hexlen overflow (%s) -> %d\n",hexstr,*hexlenp);
    return(rawind);
}

uint32_t has_duplicate_txid(struct ledger_info *ledger,char *coinstr,uint32_t blocknum,char *txidstr)
{
    int32_t hexlen,size; uint8_t data[256]; uint32_t *ptr,rawind = 0;
    if ( strcmp(coinstr,"BTC") == 0 && blocknum < 200000 )
    {
        hexlen = (int32_t)strlen(txidstr) >> 1;
        if ( hexlen < 255 )
        {
            decode_hex(data,hexlen,txidstr);
            //if ( (blocknum == 91842 && strcmp(txidstr,"d5d27987d2a3dfc724e359870c6644b40e497bdc0589a033220fe15429d88599") == 0) || (blocknum == 91880 && strcmp(txidstr,"e3bf3d07d4b0375638d5f1db5255fe07ba2c4cb067cd81b84ee974b6585fb468") == 0) )
            if ( (ptr= db777_findM(&size,ledger->DBs.transactions,ledger->txids.D.DB,data,hexlen)) != 0 )
            {
                if ( Debuglevel > 2 )
                    printf("block.%u (%s) already exists.%u\n",blocknum,txidstr,*ptr);
                if ( size == sizeof(uint32_t) )
                    rawind = *ptr;
                free(ptr);
            }
        }
    }
    return(rawind);
}

uint32_t ledger_addrind(struct ledger_info *ledger,char *coinaddr)
{
    return(ledger_rawind(0,ledger->DBs.transactions,&ledger->addrs,coinaddr,(int32_t)strlen(coinaddr)));
}

struct ledger_addrinfo *ledger_addrinfo(struct ledger_info *ledger,char *coinaddr)
{
    uint32_t addrind;
    printf("ledger_addrinfo.%p %s\n",ledger,coinaddr);
    if ( ledger != 0 && (addrind= ledger_addrind(ledger,coinaddr)) > 0 )
        return(ledger->addrinfos.D.table[addrind]);
    else return(0);
}

int32_t ledger_coinaddr(struct ledger_info *ledger,char *coinaddr,int32_t max,uint32_t addrind)
{
    char *ptr; int32_t size,retval = -1;
    if ( (ptr= db777_findM(&size,ledger->DBs.transactions,ledger->addrs.D.DB,&addrind,sizeof(addrind))) != 0 )
    {
        if ( size < max )
            strcpy(coinaddr,ptr), retval = 0;
        else printf("coinaddr.(%s) too long for %d\n",ptr,max);
        free(ptr);
    }
    return(retval);
}

int32_t addrinfo_size(int32_t n) { return(sizeof(struct ledger_addrinfo) + (sizeof(uint32_t) * n)); }

struct ledger_addrinfo *addrinfo_alloc() { return(calloc(1,addrinfo_size(0))); }

struct ledger_addrinfo *addrinfo_update(struct ledger_info *ledger,struct ledger_addrinfo *addrinfo,char *coinaddr,int32_t addrlen,uint64_t value,uint32_t unspentind,uint32_t addrind,uint32_t blocknum,char *txidstr,int32_t vout,char *extra,int32_t v)
{
    int32_t i,n; char itembuf[8192],pubstr[8192],addr[128];
    if ( addrinfo == 0 )
        addrinfo = addrinfo_alloc();
    if ( (unspentind & (1 << 31)) != 0 )
    {
        unspentind &= ~(1 << 31);
        if ( (n= addrinfo->count) > 0 )
        {
            for (i=0; i<n; i++)
            {
                if ( unspentind == addrinfo->unspentinds[i] )
                {
                    *(int64_t *)addrinfo->balance -= value;
                    addrinfo->dirty = 1;
                    addrinfo->unspentinds[i] = addrinfo->unspentinds[--addrinfo->count];
                    addrinfo->unspentinds[addrinfo->count] = 0;
                    ledger->addrinfos.D.table[addrind] = addrinfo = realloc(addrinfo,addrinfo_size(addrinfo->count));
                    /*if ( addrinfo->notify != 0 )
                    {
                        // T balance, b blocknum, a -value, t this txidstr, v this vin, st spent_txidstr, sv spent_vout
                        sprintf(itembuf,"{\"T\":%.8f,\"b\":%u,\"a\":%.8f,\"t\":\"%s\",\"v\":%d,\"st\":\"%s\",\"sv\":%d}",dstr(*(uint64_t *)addrinfo->balance),blocknum,-dstr(value),extra,v,txidstr,vout);
                        ledger_coinaddr(ledger,addr,sizeof(addr),addrind);
                        sprintf(pubstr,"{\"%s\":%s}","notify",itembuf);
                        //nn_publish(pubstr,1);
                    }*/
                    unspentind |= (1 << 31);
                    if ( addrinfo->count == 0 && *(int64_t *)addrinfo->balance != 0 )
                        printf("ILLEGAL: addrind.%u count.%d %.8f\n",addrind,addrinfo->count,dstr(*(int64_t *)addrinfo->balance)), debugstop();
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
        addrinfo = realloc(addrinfo,addrinfo_size(addrinfo->count + 1));
        *(int64_t *)addrinfo->balance += value;
        addrinfo->dirty = 1;
        addrinfo->unspentinds[addrinfo->count++] = unspentind;
        /*if ( addrinfo->notify != 0 )
        {
            // T balance, b blocknum, a value, t txidstr, v vout, s scriptstr, ti txind
            sprintf(itembuf,"{\"T\":%.8f,\"b\":%u,\"a\":%.8f,\"t\":\"%s\",\"v\":%d,\"s\":\"%s\",\"ti\":%d}",dstr(*(uint64_t *)addrinfo->balance),blocknum,dstr(value),txidstr,vout,extra,v);
            sprintf(pubstr,"{\"%s\":%s}",coinaddr,itembuf);
            //nn_publish(pubstr,1);
        }*/
    }
    return(addrinfo);
}

struct ledger_addrinfo *ledger_ensureaddrinfos(struct ledger_info *ledger,uint32_t addrind)
{
    int32_t n,width = 4096;
    if ( (addrind+1) >= ledger->addrinfos.ind )
    {
        n = (addrind + 1 + width);
        //if ( Debuglevel > 2 )
            printf("realloc addrinfos[%u] %d -> %d | ledger->addrinfos.D.table %p n.%d\n",addrind,ledger->addrinfos.ind,n,ledger->addrinfos.D.table,n);
        if ( ledger->addrinfos.D.table != 0 )
        {
            ledger->addrinfos.D.table = realloc(ledger->addrinfos.D.table,sizeof(*ledger->addrinfos.D.table) * n);
            memset(&ledger->addrinfos.D.table[ledger->addrinfos.ind],0,sizeof(*ledger->addrinfos.D.table) * (n - ledger->addrinfos.ind));
        }
        else ledger->addrinfos.D.table = calloc(n,sizeof(*ledger->addrinfos.D.table));
        ledger->addrinfos.ind = n;
    }
    return(ledger->addrinfos.D.table[addrind]);
}

uint32_t ledger_firstvout(struct ledger_info *ledger,uint32_t txidind)
{
    int32_t size = -1; uint32_t firstvout = 0; struct upair32 *firstinds;
    if ( txidind == 1 )
        return(1);
    if ( (firstinds= db777_findM(&size,ledger->DBs.transactions,ledger->txoffsets.D.DB,&txidind,sizeof(txidind))) != 0 && size == sizeof(*firstinds) )
    {
        firstvout = firstinds->firstvout;
        free(firstinds);
    } else printf("couldnt find txoffset for txidind.%u size.%d vs %ld\n",txidind,size,sizeof(*firstinds));
    return(firstvout);
}

int32_t ledger_upairset(struct ledger_info *ledger,uint32_t txidind,uint32_t firstvout,uint32_t firstvin)
{
    struct upair32 firstinds;
    firstinds.firstvout = firstvout, firstinds.firstvin = firstvin;
    if ( db777_add(-1,ledger->DBs.transactions,ledger->txoffsets.D.DB,&txidind,sizeof(txidind),&firstinds,sizeof(firstinds)) == 0 )
        return(0);
    printf("error db777_add txidind.%u <- SET firstvout.%d\n",txidind,firstvout);
    return(-1);
}

int32_t ledger_spentbits(struct ledger_info *ledger,uint32_t unspentind,uint8_t state)
{
    return(db777_add(-1,ledger->DBs.transactions,ledger->spentbits.D.DB,&unspentind,sizeof(unspentind),&state,sizeof(state)));
}

int32_t ledger_setlast(struct ledger_info *ledger,uint32_t blocknum,uint32_t numsyncs)
{
    struct ledger_inds L;
    memset(&L,0,sizeof(L));
    uint16_t key = numsyncs;
    L.blocknum = ledger->blocknum, L.numsyncs = ledger->numsyncs;
    L.voutsum = ledger->voutsum, L.spendsum = ledger->spendsum;
    L.addrind = ledger->addrs.ind, L.txidind = ledger->txids.ind, L.scriptind = ledger->scripts.ind;
    L.unspentind = ledger->unspentmap.ind, L.numspents = ledger->spentbits.ind;
    L.numaddrinfos = ledger->addrinfos.ind, L.txoffsets = ledger->txoffsets.ind;
    if ( numsyncs > 0 )
    {
        printf("SYNCNUM.%d -> %d supply %.8f\n",numsyncs,blocknum,dstr(L.voutsum)-dstr(L.spendsum));
        db777_add(1,ledger->DBs.transactions,ledger->ledger.D.DB,&key,sizeof(key),&L,sizeof(L));
    }
    return(db777_add(2,ledger->DBs.transactions,ledger->ledger.D.DB,"last",strlen("last"),&L,sizeof(L)));
}

int32_t ledger_startblocknum(struct ledger_info *ledger,uint32_t startblocknum)
{
    struct ledger_inds *lp;
    int32_t size; uint32_t blocknum = 1;
    if ( (lp= db777_findM(&size,ledger->DBs.transactions,ledger->ledger.D.DB,"last",strlen("last"))) != 0 )
    {
        if ( size == sizeof(*lp) )
        {
            ledger->blocknum = blocknum = lp->blocknum, ledger->numsyncs = lp->numsyncs;
            ledger->voutsum = lp->voutsum, ledger->spendsum = lp->spendsum;
            ledger->addrs.ind = lp->addrind, ledger->txids.ind = lp->txidind, ledger->scripts.ind = lp->scriptind;
            ledger->unspentmap.ind = lp->unspentind, ledger->spentbits.ind = lp->numspents;
            ledger->txoffsets.ind = lp->numaddrinfos, ledger->txoffsets.ind = lp->numaddrinfos;
        } else printf("size mismatch %d vs %ld\n",size,sizeof(*lp));
        free(lp);
    } else printf("ledger_getnearest error getting last\n");
    return(blocknum);
}

uint64_t ledger_unspentvalue(uint32_t *addrindp,struct ledger_info *ledger,uint32_t unspentind)
{
    struct unspentmap *U; int32_t size; uint64_t value = 0;
    *addrindp = 0;
    if ( (U= db777_findM(&size,ledger->DBs.transactions,ledger->unspentmap.D.DB,&unspentind,sizeof(unspentind))) != 0 )
    {
        if ( size != sizeof(*U) )
            printf("unspentmap unexpectsize %d vs %ld\n",size,sizeof(*U));
        else
        {
            memcpy(&value,U->value,sizeof(value));
            *addrindp = U->addrind;
            //printf("unspentmap.%u %.8f -> addrind.%u\n",unspentind,dstr(value),U->addrind);
        }
        free(U);
    }
    else printf("error loading unspentmap (%s) unspentind.%u\n",ledger->DBs.coinstr,unspentind), debugstop();
    return(value);
}

uint64_t ledger_recalc_addrinfos(char *retbuf,int32_t maxlen,struct ledger_info *ledger,int32_t numrichlist)
{
    struct ledger_addrinfo *addrinfo;
    uint32_t i,n,addrind,itemlen,len; float *sortbuf; uint64_t balance,addrsum; char coinaddr[128],itembuf[128];
    addrsum = n = 0;
    if ( ledger->addrinfos.D.table == 0 )
        return(0);
    if ( numrichlist == 0 || retbuf == 0 || maxlen == 0 )
    {
        for (i=1; i<=ledger->addrs.ind; i++)
            if ( (addrinfo= ledger->addrinfos.D.table[i]) != 0 && (balance= *(int64_t *)addrinfo->balance) != 0 )
            {
                //if ( addrinfo->notify != 0 )
                //    printf("notification active addind.%d count.%d size.%d\n",addrind,addrinfo->count,addrinfo_size(addrinfo->count)), getchar();
                addrsum += balance;
            }
    }
    else
    {
        sortbuf = calloc(ledger->addrs.ind,sizeof(float)+sizeof(uint32_t));
        for (i=1; i<=ledger->addrs.ind; i++)
            if ( (addrinfo= ledger->addrinfos.D.table[i]) != 0 && (balance= *(int64_t *)addrinfo->balance) != 0 )
            {
                addrsum += balance;
                sortbuf[n << 1] = dstr(balance);
                memcpy(&sortbuf[(n << 1) + 1],&i,sizeof(i));
                n++;
            }
        if ( n > 0 )
        {
            revsortfs(sortbuf,n,sizeof(*sortbuf) * 2);
            strcpy(retbuf,"{\"richlist\":["), len = (int32_t)strlen(retbuf);
            for (i=0; i<numrichlist&&i<n; i++)
            {
                memcpy(&addrind,&sortbuf[(i << 1) + 1],sizeof(addrind));
                addrinfo = ledger->addrinfos.D.table[addrind];
                ledger_coinaddr(ledger,coinaddr,sizeof(coinaddr),addrind);
                sprintf(itembuf,"%s[\"%s\", \"%.8f\"],",i==0?"":" ",coinaddr,sortbuf[i << 1]);
                itemlen = (int32_t)strlen(itembuf);
                if ( len+itemlen < maxlen-32 )
                {
                    memcpy(retbuf+len,itembuf,itemlen+1);
                    len += itemlen;
                }
            }
            sprintf(retbuf + len - 1,"],\"supply\":%.8f}",dstr(addrsum));
            //printf("(%s) top.%d of %d\n",retbuf,i,n);
        }
        free(sortbuf);
    }
    return(addrsum);
}

// block iterators
uint32_t ledger_addtx(struct ledger_info *ledger,struct alloc_space *mem,uint32_t txidind,char *txidstr,uint32_t totalvouts,uint16_t numvouts,uint32_t totalspends,uint16_t numvins)
{
    uint32_t checkind; uint8_t txid[256]; struct ledger_txinfo tx; int32_t txidlen;
    if ( Debuglevel > 2 )
        printf("ledger_tx txidind.%d %s vouts.%d vins.%d | ledger->txoffsets.ind %d\n",txidind,txidstr,totalvouts,totalspends,ledger->txoffsets.ind);
    if ( (checkind= ledger_hexind(1,ledger->DBs.transactions,&ledger->txids,txid,&txidlen,txidstr)) == txidind )
    {
        memset(&tx,0,sizeof(tx));
        tx.firstvout = totalvouts, tx.firstvin = totalspends, tx.numvouts = numvouts, tx.numvins = numvins;
        tx.txidlen = txidlen, memcpy(tx.txid,txid,txidlen);
        ledger_upairset(ledger,txidind+1,totalvouts + numvouts,totalspends + numvins);
        return(ledger_packtx(ledger->txoffsets.sha256,&ledger->txoffsets.state,mem,&tx));
    } else printf("ledger_tx: mismatched txidind, expected %u got %u\n",txidind,checkind), debugstop();
    return(0);
}

uint32_t ledger_addunspent(uint16_t *numaddrsp,uint16_t *numscriptsp,struct ledger_info *ledger,struct alloc_space *mem,uint32_t txidind,uint16_t v,uint32_t unspentind,char *coinaddr,char *scriptstr,uint64_t value,uint32_t blocknum,char *txidstr,int32_t txind)
{
    struct ledger_voutdata vout;
    memset(&vout,0,sizeof(vout));
    memcpy(vout.U.value,&value,sizeof(vout.U.value));
    ledger->voutsum += value;
    //printf("%.8f ",dstr(value));
    if ( (vout.scriptind= ledger_hexind(1,ledger->DBs.transactions,&ledger->scripts,vout.script,&vout.scriptlen,scriptstr)) == 0 )
    {
        printf("ledger_unspent: error getting scriptind.(%s)\n",scriptstr);
        return(0);
    }
    vout.newscript = (vout.scriptind == ledger->scripts.ind);
    (*numscriptsp) += vout.newscript;
    vout.addrlen = (int32_t)strlen(coinaddr);
    if ( (vout.U.addrind= ledger_rawind(1,ledger->DBs.transactions,&ledger->addrs,coinaddr,vout.addrlen)) != 0 )
    {
        ledger->unspentmap.ind = unspentind;
        if ( db777_add(-1,ledger->DBs.transactions,ledger->unspentmap.D.DB,&unspentind,sizeof(unspentind),&vout.U,sizeof(vout.U)) != 0 )
            printf("error saving unspentmap (%s) %u -> %u %.8f\n",ledger->DBs.coinstr,unspentind,vout.U.addrind,dstr(value));
        if ( vout.U.addrind == ledger->addrs.ind )
        {
            vout.newaddr = 1, strcpy(vout.coinaddr,coinaddr), (*numaddrsp)++;
            if ( db777_add(-1,ledger->DBs.transactions,ledger->addrs.D.DB,&vout.U.addrind,sizeof(vout.U.addrind),coinaddr,vout.addrlen) != 0 )
                printf("error saving coinaddr.(%s) addrind.%u\n",coinaddr,vout.U.addrind);
        }
        if ( Debuglevel > 2 )
            printf("txidind.%u v.%d unspent.%d (%s).%u (%s).%u %.8f | %ld\n",txidind,v,unspentind,coinaddr,vout.U.addrind,scriptstr,vout.scriptind,dstr(value),sizeof(vout.U));
        ledger_ensureaddrinfos(ledger,vout.U.addrind);
        ledger->addrinfos.D.table[vout.U.addrind] = addrinfo_update(ledger,ledger->addrinfos.D.table[vout.U.addrind],coinaddr,vout.addrlen,value,unspentind,vout.U.addrind,blocknum,txidstr,v,scriptstr,txind);
        //ledger->addrinfos.D.table[vout.U.addrind]->notify = 1;
        return(ledger_packvout(ledger->addrinfos.sha256,&ledger->addrinfos.state,mem,&vout));
    } else printf("ledger_unspent: cant find addrind.(%s)\n",coinaddr);
    return(0);
}

uint32_t ledger_addspend(struct ledger_info *ledger,struct alloc_space *mem,uint32_t txidind,uint32_t totalspends,char *spent_txidstr,uint16_t spent_vout,uint32_t blocknum,char *txidstr,int32_t v)
{
    struct ledger_spendinfo spend; struct ledger_addrinfo *addrinfo;
    int32_t txidlen; uint64_t value; uint8_t txid[256]; uint32_t spent_txidind,addrind;
    if ( Debuglevel > 2 )
        printf("txidind.%d totalspends.%d (%s).v%d\n",txidind,totalspends,spent_txidstr,spent_vout);
    if ( (spent_txidind= ledger_hexind(0,ledger->DBs.transactions,&ledger->txids,txid,&txidlen,spent_txidstr)) != 0 )
    {
        memset(&spend,0,sizeof(spend));
        spend.spent_txidind = spent_txidind, spend.spent_vout = spent_vout;
#ifdef USEMEM
        spend.unspentind = ledger->txoffsets.D.upairs[spent_txidind].firstvout + spent_vout;
        SETBIT(ledger->spentbits.D.bits,spend.unspentind);
#else
        spend.unspentind = ledger_firstvout(ledger,spent_txidind) + spent_vout;
        ledger_spentbits(ledger,spend.unspentind,1);
#endif
        if ( Debuglevel > 2 )
            printf("spent_txidstr.(%s) -> spent_txidind.%u firstvout.%d\n",spent_txidstr,spent_txidind,spend.unspentind-spent_vout);
        if ( (value= ledger_unspentvalue(&addrind,ledger,spend.unspentind)) != 0 && addrind > 0 )
        {
            ledger->spendsum += value;
            if ( (addrinfo= ledger_ensureaddrinfos(ledger,addrind)) != 0 )
                ledger->addrinfos.D.table[addrind] = addrinfo_update(ledger,addrinfo,0,0,value,spend.unspentind | (1 << 31),addrind,blocknum,spent_txidstr,spent_vout,txidstr,v);
            else printf("null addrinfo for addrind.%d max.%d, unspentind.%d %.8f\n",addrind,ledger->addrs.ind,spend.unspentind,dstr(value));
            if ( Debuglevel > 2 )
                printf("addrind.%u count.%d %.8f\n",addrind,addrinfo->count,dstr(*(int64_t *)addrinfo->balance));
            return(ledger_packspend(ledger->spentbits.sha256,&ledger->spentbits.state,mem,&spend));
        } else printf("error getting unspentmap for unspentind.%u (%s).v%d\n",spend.unspentind,spent_txidstr,spent_vout);
    } else printf("ledger_spend: cant find txidind for (%s).v%d\n",spent_txidstr,spent_vout);
    return(0);
}

struct ledger_blockinfo *ledger_startblock(struct ledger_info *ledger,struct alloc_space *mem,uint32_t blocknum,uint64_t minted,int32_t numtx)
{
    struct ledger_blockinfo *block;
    if ( ledger->blockpending != 0 )
    {
        printf("ledger_startblock: cant startblock when %s %u is pending\n",ledger->DBs.coinstr,ledger->blocknum);
        return(0);
    }
    ledger->blockpending = 1, ledger->blocknum = blocknum;
    block = memalloc(mem,sizeof(*block),1);
    block->minted = minted, block->numtx = numtx, block->blocknum = blocknum;
    //ledger_copyinds(block,ledger,1);
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
    if ( db777_add(-1,ledger->DBs.transactions,ledger->blocks.D.DB,&tmp,sizeof(tmp),block,block->allocsize) != 0 )
    {
        printf("error saving blocks %s %u\n",ledger->DBs.coinstr,block->blocknum);
        return(0);
    }
    ledger->blockpending = 0;
    return(block->allocsize);
}

struct ledger_blockinfo *ledger_update(int32_t dispflag,struct ledger_info *ledger,struct alloc_space *mem,char *name,char *serverport,char *userpass,struct rawblock *emit,uint32_t blocknum)
{
    struct rawtx *tx; struct rawvin *vi; struct rawvout *vo; struct ledger_blockinfo *block = 0;
    uint32_t i,txidind,txind,n;
    if ( rawblock_load(emit,name,serverport,userpass,blocknum) > 0 )
    {
        tx = emit->txspace, vi = emit->vinspace, vo = emit->voutspace;
        block = ledger_startblock(ledger,mem,blocknum,emit->minted,emit->numtx);
        if ( block->numtx > 0 )
        {
            for (txind=0; txind<block->numtx; txind++,tx++)
            {
                if ( (txidind= has_duplicate_txid(ledger,ledger->DBs.coinstr,blocknum,tx->txidstr)) == 0 )
                    txidind = ledger->txids.ind + 1;
                //printf("expect txidind.%d unspentind.%d totalspends.%d\n",txidind,block->unspentind+1,block->totalspends);
                ledger_addtx(ledger,mem,txidind,tx->txidstr,ledger->unspentmap.ind+1,tx->numvouts,ledger->spentbits.ind+1,tx->numvins);
                if ( (n= tx->numvouts) > 0 )
                    for (i=0; i<n; i++,vo++,block->numvouts++)
                        ledger_addunspent(&block->numaddrs,&block->numscripts,ledger,mem,txidind,i,++ledger->unspentmap.ind,vo->coinaddr,vo->script,vo->value,blocknum,tx->txidstr,txind);
                if ( (n= tx->numvins) > 0 )
                    for (i=0; i<n; i++,vi++,block->numvins++)
                        ledger_addspend(ledger,mem,txidind,++ledger->spentbits.ind,vi->txidstr,vi->vout,blocknum,tx->txidstr,i);
            }
        }
    } else printf("error loading %s block.%u\n",ledger->DBs.coinstr,blocknum);
    return(block);
}

int32_t ledger_sync(struct ledger_info *ledger)
{
    uint32_t addrind,allocsize,dirty = 0; struct ledger_addrinfo *addrinfo;
    if ( ledger->addrinfos.D.table == 0 )
    {
        printf("uninitialized pointer %p\n",ledger->addrinfos.D.table);
        return(-1);
    }
    allocsize = 0;
    for (addrind=1; addrind<=ledger->addrs.ind; addrind++)
    {
        if ( (addrinfo= ledger->addrinfos.D.table[addrind]) != 0 && addrinfo->dirty != 0 )
        {
            dirty++;
            addrinfo->dirty = 0;
            if ( db777_add(1,ledger->DBs.transactions,ledger->ledger.D.DB,&addrind,sizeof(addrind),addrinfo,addrinfo_size(addrinfo->count)) != 0 )
            {
                printf("error saving addrinfo[%u]\n",addrind);
                return(-1);
            }
            else allocsize += addrinfo_size(addrinfo->count);
        }
    }
    printf("[%-3d addrs %8s] ",dirty,_mbstr(allocsize));
    return(dirty);
}

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

void ramchain_update(struct ramchain *ramchain,char *serverport,char *userpass,int32_t syncflag)
{
    void ledger_free(struct ledger_info *ledger,int32_t closeDBflag);
    struct alloc_space MEM; struct ledger_info *ledger; struct ledger_blockinfo *block;
    int32_t allocsize; uint32_t blocknum,dispflag; uint64_t supply,oldsupply; double estimate,elapsed;
    if ( ramchain->readyflag == 0 || (ledger= ramchain->activeledger) == 0 )
        return;
    blocknum = ledger->blocknum;
    if ( blocknum < ramchain->RTblocknum )
    {
        if ( blocknum == 0 )
            ledger->blocknum = blocknum = 1;
        dispflag = 1 || (blocknum > ramchain->RTblocknum - 1000);
        dispflag += ((blocknum % 100) == 0);
        oldsupply = ledger->voutsum - ledger->spendsum;
        memset(&MEM,0,sizeof(MEM)), MEM.ptr = &ramchain->DECODE, MEM.size = sizeof(ramchain->DECODE);
        if ( ledger->DBs.transactions == 0 )
            ledger->DBs.transactions = sp_begin(ledger->DBs.env);
        if ( (block= ledger_update(dispflag,ledger,&MEM,ramchain->name,serverport,userpass,&ramchain->EMIT,blocknum)) != 0 )
        {
            if ( (allocsize= ledger_finishblock(ledger,&MEM,block)) <= 0 )
                printf("error updating %s block.%u\n",ramchain->name,blocknum);
            if ( syncflag != 0 )
            {
                ledger_setlast(ledger,ledger->blocknum,++ledger->numsyncs);
                ledger_commit(ledger,0);
                ledger_sync(ledger);
                ledger_commit(ledger,syncflag == 1);
            }
            ramchain->addrsum = ledger_recalc_addrinfos(0,0,ledger,0);//dispflag - 1);
            ramchain->totalsize += block->allocsize;
            estimate = estimate_completion(ramchain->startmilli,blocknum - ramchain->startblocknum,ramchain->RTblocknum-blocknum)/60000;
            elapsed = (milliseconds() - ramchain->startmilli)/60000.;
            supply = ledger->voutsum - ledger->spendsum;
            if ( dispflag != 0 )
            {
                extern uint32_t Duplicate,Mismatch,Added;
                printf("%-5s [lag %-5d] %-6u supply %.8f %.8f (%.8f) [%.8f] %13.8f | dur %.2f %.2f %.2f | len.%-5d %s %.1f | DMA %d ?.%d %d\n",ramchain->name,ramchain->RTblocknum-blocknum,blocknum,dstr(supply),dstr(ramchain->addrsum),dstr(supply)-dstr(ramchain->addrsum),dstr(supply)-dstr(oldsupply),dstr(ramchain->EMIT.minted),elapsed,estimate,elapsed+estimate,block->allocsize,_mbstr(ramchain->totalsize),(double)ramchain->totalsize/blocknum,Duplicate,Mismatch,Added);
            }
            ledger->blocknum++;
        }
        else printf("%s error processing block.%d\n",ramchain->name,blocknum);
    }
}

// init funcs
struct ledger_blockinfo *ledger_setblocknum(struct ledger_info *ledger,struct alloc_space *mem,uint32_t startblocknum)
{
    uint32_t addrind; int32_t allocsize,empty,modval,lastmodval,extra; uint64_t balance = 0;
    struct ledger_blockinfo *block; struct ledger_addrinfo *addrinfo;
    if ( startblocknum < 1 )
        startblocknum = 1;
    startblocknum = ledger_startblocknum(ledger,0);
    if ( (block= db777_findM(&allocsize,0,ledger->blocks.D.DB,&startblocknum,sizeof(startblocknum))) != 0 )
    {
        if ( block->allocsize == allocsize && block_crc16(block) == block->crc16 )
        {
            //ledger_copyinds(block,ledger,0);
            printf("%.8f startmilli %.0f start.%u block.%u ledger block.%u, inds.(txid %d addrs %d scripts %d vouts %d vins %d)\n",dstr(ledger->voutsum)-dstr(ledger->spendsum),milliseconds(),startblocknum,block->blocknum,ledger->blocknum,ledger->txids.ind,ledger->addrs.ind,ledger->scripts.ind,ledger->unspentmap.ind,ledger->spentbits.ind);
            ledger_ensureaddrinfos(ledger,ledger->addrs.ind);
            extra = empty = 0;
            lastmodval = -1;
            for (addrind=1; addrind<=ledger->addrs.ind; addrind++)
            {
                modval = ((100. * addrind) / (ledger->addrs.ind + 1));
                if ( modval != lastmodval )
                    fprintf(stderr,"%d%% ",modval), lastmodval = modval;
                addrinfo = db777_findM(&allocsize,0,ledger->ledger.D.DB,&addrind,sizeof(addrind));
                if ( addrinfo != 0 )
                {
                    ledger->addrinfos.D.table[addrind] = addrinfo;
                    balance += *(int64_t *)addrinfo->balance;
                }
                else printf("error loading addrind.%u addrinfo\n",addrind);
            }
            for (; addrind<ledger->addrinfos.ind; addrind++)
                if ( ledger->addrinfos.D.table[addrind] != 0 )
                    free(ledger->addrinfos.D.table[addrind]), ledger->addrinfos.D.table[addrind] = 0, extra++;
            printf(" addrinds empty.%d and extra.%d\n",empty,extra);
            printf("balance %.8f endmilli %.0f\n",dstr(balance),milliseconds());
        } else printf("mismatched block: %u %u, crc16 %u %u\n",block->allocsize,allocsize,block_crc16(block),block->crc16);
    } else printf("couldnt load block.%u\n",startblocknum);
    return(block);
}

int32_t ramchain_resume(char *retbuf,struct ramchain *ramchain,uint32_t startblocknum,uint32_t endblocknum)
{
    extern uint32_t Duplicate,Mismatch,Added;
    struct ledger_info *ledger; struct ledger_blockinfo *block; struct alloc_space MEM; uint64_t balance;
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
    if ( ramchain->startblocknum > 0 && (block= ledger_setblocknum(ledger,&MEM,ramchain->startblocknum)) != 0 )
        free(block);
    else ramchain->startblocknum = 0;
    ledger->blocknum = ramchain->startblocknum + 1;
    ramchain->endblocknum = (endblocknum > ramchain->startblocknum) ? endblocknum : ramchain->startblocknum;
    balance = ledger_recalc_addrinfos(0,0,ledger,0);
    sprintf(retbuf,"{\"result\":\"resumed\",\"startblocknum\":%d,\"endblocknum\":%d,\"addrsum\":%.8f,\"ledger supply\":%.8f,\"diff\":%.8f,\"elapsed\":%.3f}",ramchain->startblocknum,ramchain->endblocknum,dstr(balance),dstr(ledger->voutsum) - dstr(ledger->spendsum),dstr(balance) - (dstr(ledger->voutsum) - dstr(ledger->spendsum)),(milliseconds() - ramchain->startmilli)/1000.);
    ramchain->startmilli = milliseconds();
    ramchain->paused = 0;
    return(0);
}

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
    if ( ledger != 0 )
    {
#ifdef USEMEM
        if ( ledger->txoffsets.D.upairs != 0 )
            free(ledger->txoffsets.D.upairs);
        if ( ledger->spentbits.D.bits != 0 )
            free(ledger->spentbits.D.bits);
#endif
        if ( ledger->addrinfos.D.table != 0 )
        {
            int32_t i;
            for (i=0; i<ledger->addrinfos.ind; i++)
                if ( ledger->addrinfos.D.table[i] != 0 )
                    free(ledger->addrinfos.D.table[i]);
            free(ledger->addrinfos.D.table);
        }
        if ( closeDBflag != 0 )
        {
            ledger_DBopcodes(&ledger->DBs,LEDGER_DB_CLOSE);
            sp_destroy(ledger->DBs.env), ledger->DBs.env = 0;
        }
        free(ledger);
    }
}

void ledger_stateinit(struct env777 *DBs,struct ledger_state *sp,char *coinstr,char *subdir,char *name,char *compression)
{
    safecopy(sp->name,name,sizeof(sp->name));
    //update_sha256(sp->sha256,&sp->state,0,0);
    if ( DBs != 0 )
        sp->D.DB = db777_open(0,DBs,name,compression);
}

struct ledger_info *ledger_alloc(char *coinstr,char *subdir)
{
    struct ledger_info *ledger = 0;
    if ( (ledger= calloc(1,sizeof(*ledger))) != 0 )
    {
        safecopy(ledger->DBs.coinstr,coinstr,sizeof(ledger->DBs.coinstr));
        safecopy(ledger->DBs.subdir,subdir,sizeof(ledger->DBs.subdir));
        ledger_stateinit(0,&ledger->addrinfos,coinstr,subdir,"addrinfos",0);
        ledger_stateinit(&ledger->DBs,&ledger->spentbits,coinstr,subdir,"spentbits","zstd");
        ledger_stateinit(&ledger->DBs,&ledger->txoffsets,coinstr,subdir,"txoffsets","zstd");
        ledger_stateinit(&ledger->DBs,&ledger->blocks,coinstr,subdir,"blocks","zstd");
        ledger_stateinit(&ledger->DBs,&ledger->ledger,coinstr,subdir,"ledger","zstd");
        ledger_stateinit(&ledger->DBs,&ledger->addrs,coinstr,subdir,"addrs","zstd");
        ledger_stateinit(&ledger->DBs,&ledger->txids,coinstr,subdir,"txids",0);
        ledger_stateinit(&ledger->DBs,&ledger->scripts,coinstr,subdir,"scripts","zstd");
        ledger_stateinit(&ledger->DBs,&ledger->unspentmap,coinstr,subdir,"unspentmap","zstd");
        ledger->blocknum = 1;
    }
    return(ledger);
}

int32_t ramchain_init(char *retbuf,struct coin777 *coin,char *coinstr,uint32_t startblocknum,uint32_t endblocknum)
{
    struct ramchain *ramchain = &coin->ramchain;
    ramchain->syncfreq = 1000;
    strcpy(ramchain->name,coinstr);
    ramchain->RTblocknum = _get_RTheight(&ramchain->lastgetinfo,coinstr,coin->serverport,coin->userpass,ramchain->RTblocknum);
    ramchain->readyflag = 1;
    if ( (ramchain->activeledger= ledger_alloc(coinstr,"")) != 0 )
    {
        env777_start(0,&ramchain->activeledger->DBs);
        if ( endblocknum == 0 )
            endblocknum = 1000000000;
        return(ramchain_resume(retbuf,ramchain,startblocknum,endblocknum));
    }
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

void ramchain_idle(struct plugin_info *plugin)
{
    int32_t i,lag,syncflag;
    struct coin777 *coin;
    struct ramchain *ramchain;
    struct ledger_info *ledger;
    for (i=0; i<COINS.num; i++)
    {
        if ( (coin= COINS.LIST[i]) != 0  && (ledger= coin->ramchain.activeledger) != 0 )
        {
            ramchain = &coin->ramchain;
            if ( (lag= (ramchain->RTblocknum - ledger->blocknum)) < 1000 || (ledger->blocknum % 1000) == 0 )
                ramchain->RTblocknum = _get_RTheight(&ramchain->lastgetinfo,ramchain->name,coin->serverport,coin->userpass,ramchain->RTblocknum);
            if ( lag < 100000 && ramchain->syncfreq > 50000 )
                ramchain->syncfreq = 50000;
            else if ( lag < 50000 && ramchain->syncfreq > 10000 )
                ramchain->syncfreq = 10000;
            else if ( lag < 10000 && ramchain->syncfreq > 1000 )
                ramchain->syncfreq = 1000;
            else if ( lag < 1000 && ramchain->syncfreq > 100 )
                ramchain->syncfreq = 100;
            else if ( strcmp(ramchain->name,"BTC") == 0 && lag < 10 && ramchain->syncfreq > 10 )
                ramchain->syncfreq = 10;
            if ( ramchain->paused < 10 )
            {
                syncflag = (((ledger->blocknum % ramchain->syncfreq) == 0) || (ramchain->needbackup != 0));
                if ( ledger->blocknum >= ramchain->endblocknum || ramchain->paused != 0 )
                {
                    if ( ledger->blocknum >= ramchain->endblocknum )
                        ramchain->paused = 3, syncflag = 2;
                    printf("ramchain.%s blocknum.%d <<< PAUSING paused.%d |  endblocknum.%u\n",ramchain->name,ledger->blocknum,ramchain->paused,ramchain->endblocknum);
                }
                ramchain_update(ramchain,coin->serverport,coin->userpass,syncflag);
                if ( ramchain->paused == 3 )
                {
                    ledger_free(ramchain->activeledger,1), ramchain->activeledger = 0;
                    printf("STOPPED\n");
                }
                if ( ledger->blocknum > ramchain->endblocknum || ramchain->paused != 0 )
                    ramchain->paused = 10;
            }
        }
    }
}

int32_t ramchain_notify(char *retbuf,struct ramchain *ramchain,char *endpoint,cJSON *list)
{
    int32_t i,n,m=0; char *coinaddr; struct ledger_addrinfo *addrinfo; struct ledger_info *ledger = ramchain->activeledger;
    if ( ledger == 0 )
    {
        sprintf(retbuf,"{\"error\":\"no active ledger\"}");
        return(-1);
    }
    if ( endpoint == 0 )
    {
        if ( list == 0 || (n= cJSON_GetArraySize(list)) <= 0 )
            sprintf(retbuf,"{\"error\":\"no notification list\"}");
        else
        {
            for (i=0; i<n; i++)
            {
                if ( (coinaddr= cJSON_str(cJSON_GetArrayItem(list,i))) != 0 && (addrinfo= ledger_addrinfo(ledger,coinaddr)) != 0 )
                {
                    m++;
                    //addrinfo->notify = 1;
                    printf("NOTIFY.(%s)\n",coinaddr);
                    sprintf(retbuf,"{\"result\":\"added to notification list\",\"coinaddr\":\"%s\"}",coinaddr);
                } else sprintf(retbuf,"{\"error\":\"cant find address\",\"coinaddr\":\"%s\"}",coinaddr);
            }
            sprintf(retbuf,"{\"result\":\"added to notification list\",\"num\":%d,\"from\":%d}",m,n);
        }
    } else sprintf(retbuf,"{\"error\":\"endpoint specified, private channel needs to be implemented\"}");
    return(0);
}

int32_t PLUGNAME(_process_json)(struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag)
{
    char *coinstr,*resultstr,*methodstr;
    struct coin777 *coin = 0;
    uint32_t startblocknum,endblocknum;
    retbuf[0] = 0;
   //Debuglevel = 3;
    printf("<<<<<<<<<<<< INSIDE PLUGIN! process %s\n",plugin->name);
    if ( initflag > 0 )
    {
        strcpy(retbuf,"{\"result\":\"initflag > 0\"}");
        plugin->allowremote = 0;
        copy_cJSON(RAMCHAINS.pullnode,cJSON_GetObjectItem(json,"pullnode"));
        RAMCHAINS.readyflag = 1;
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        methodstr = cJSON_str(cJSON_GetObjectItem(json,"method"));
        coinstr = cJSON_str(cJSON_GetObjectItem(json,"coin"));
        startblocknum = get_API_int(cJSON_GetObjectItem(json,"start"),0);
        endblocknum = get_API_int(cJSON_GetObjectItem(json,"end"),0);
        printf("RAMCHAIN.(%s) for (%s)\n",methodstr,coinstr!=0?coinstr:"");
        if ( coinstr != 0 )
            coin = coin777_find(coinstr);
        if ( methodstr == 0 || methodstr[0] == 0 )
        {
            printf("(%s) has not method\n",jsonstr);
            return(0);
        }
        if ( coin != 0 && coinstr == 0 )
            coinstr = coin->name;
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
        else
        {
            if ( strcmp(methodstr,"backup") == 0 )
            {
                if ( coin != 0 )
                {
                    if ( coin->ramchain.activeledger == 0 )
                        ramchain_init(retbuf,coin,coinstr,startblocknum,endblocknum);
                    if ( coin->ramchain.readyflag != 0 && coin->ramchain.activeledger != 0 && coin->ramchain.activeledger->DBs.ctl != 0 )
                    {
                        db777_backup(coin->ramchain.activeledger->DBs.ctl);
                        strcpy(retbuf,"{\"result\":\"started backup\"}");
                    } else strcpy(retbuf,"{\"error\":\"cant create ramchain when coin not ready\"}");
                }
                else strcpy(retbuf,"{\"error\":\"cant find coin\"}");
            }
            else if ( strcmp(methodstr,"pause") == 0 )
            {
                if ( coin != 0 )
                    coin->ramchain.paused = 1;
            }
            else if ( strcmp(methodstr,"richlist") == 0 )
            {
                if ( coin != 0 )
                {
                    if ( coin->ramchain.activeledger == 0 )
                    {
                        ramchain_init(retbuf,coin,coinstr,startblocknum,endblocknum);
                        coin->ramchain.paused = 1;
                    }
                    if ( coin->ramchain.activeledger != 0 )
                        ramchain_richlist(retbuf,maxlen,&coin->ramchain,get_API_int(cJSON_GetObjectItem(json,"num"),25));
                }
            }
            else if ( strcmp(methodstr,"notify") == 0 )
            {
                if ( coin != 0 )
                {
                    if ( coin->ramchain.activeledger == 0 )
                    {
                        ramchain_init(retbuf,coin,coinstr,startblocknum,endblocknum);
                        coin->ramchain.paused = 1;
                    }
                    if ( coin->ramchain.activeledger != 0 )
                        ramchain_notify(retbuf,&coin->ramchain,cJSON_str(cJSON_GetObjectItem(json,"endpoint")),cJSON_GetObjectItem(json,"list"));
                }
            }
            else if ( strcmp(methodstr,"stop") == 0 )
            {
                if ( coin != 0 && coin->ramchain.activeledger != 0 )
                {
                    coin->ramchain.paused = 3;
                    sprintf(retbuf,"{\"result\":\"pausing and stopping ramchain\"}");
                } else sprintf(retbuf,"{\"result\":\"no active ramchain to stop\"}");
            }
            else if ( strcmp(methodstr,"resume") == 0 )
            {
                if ( coin != 0 )
                {
                    if ( coin->ramchain.activeledger == 0 )
                        ramchain_init(retbuf,coin,coinstr,startblocknum,endblocknum);
                    else
                    {
                        ramchain_stop(retbuf,&coin->ramchain);
                        ramchain_init(retbuf,coin,coinstr,startblocknum,endblocknum);
                        ramchain_resume(retbuf,&coin->ramchain,startblocknum,endblocknum);
                    }
                }
            }
            else if ( strcmp(methodstr,"create") == 0 )
                ramchain_init(retbuf,coin,coinstr,startblocknum,endblocknum);
        }
    }
    return((int32_t)strlen(retbuf));
}

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *argjson)
{
    uint64_t disableflags = 0;
    plugin->sleepmillis = 1;
    printf("init %s size.%ld\n",plugin->name,sizeof(struct ramchain_info));
    return(disableflags); // set bits corresponding to array position in _methods[]
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    return(retcode);
}
#include "../plugin777.c"
