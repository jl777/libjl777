/******************************************************************************
 * Copyright © 2014-2015 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

#ifdef DEFINES_ONLY
#ifndef cards777_h
#define cards777_h

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include "../utils/bits777.c"
#include "../common/hostnet777.c"

#endif
#else
#ifndef cards777_c
#define cards777_c

#ifndef cards777_h
#define DEFINES_ONLY
#include "cards777.c"
#undef DEFINES_ONLY
#endif
#include "../utils/curve25519.h"


void calc_shares(unsigned char *shares,unsigned char *secret,int32_t size,int32_t width,int32_t M,int32_t N,unsigned char *sharenrs);
int32_t init_sharenrs(unsigned char sharenrs[255],unsigned char *orig,int32_t m,int32_t n);
void gfshare_ctx_dec_newshares(void *ctx,unsigned char *sharenrs);
void gfshare_ctx_dec_giveshare(void *ctx,unsigned char sharenr,unsigned char *share);
void gfshare_ctx_dec_extract(void *ctx,unsigned char *secretbuf);
void *gfshare_ctx_init_dec(unsigned char *sharenrs,uint32_t sharecount,uint32_t size);
void gfshare_ctx_free(void *ctx);

bits256 cards777_initcrypt(bits256 data,bits256 privkey,bits256 pubkey,int32_t invert)
{
    bits256 hash; bits320 hexp;
    hash = curve25519_shared(privkey,pubkey);
    hexp = fexpand(hash);
    if ( invert != 0 )
        hexp = crecip(hexp);
    return(fcontract(fmul(fexpand(data),hexp)));
}

bits256 cards777_cardpriv(bits256 playerpriv,bits256 *cardpubs,int32_t numcards,bits256 cipher)
{
    bits256 cardpriv,checkpub; int32_t i;
    for (i=0; i<numcards; i++)
    {
        cardpriv = cards777_initcrypt(cipher,playerpriv,cardpubs[i],1);
        //printf("(%llx %llx) ",(long long)cardpriv.txid,(long long)curve25519_shared(playerpriv,cardpubs[i]).txid);
        checkpub = curve25519(cardpriv,curve25519_basepoint9());
        if ( memcmp(checkpub.bytes,cardpubs[i].bytes,sizeof(bits256)) == 0 )
        {
            //printf("%d ",cardpriv.bytes[1]);
            //printf("decrypted card.%d %llx\n",cardpriv.bytes[1],(long long)cardpriv.txid);
            return(cardpriv);
        }
    }
    //printf("\nplayerpriv %llx cipher.%llx\n",(long long)playerpriv.txid,(long long)cipher.txid);
    memset(cardpriv.bytes,0,sizeof(cardpriv));
    return(cardpriv);
}

int32_t cards777_checkcard(bits256 *cardprivp,int32_t cardi,int32_t slot,int32_t destplayer,bits256 playerpriv,bits256 *cardpubs,int32_t numcards,bits256 card)
{
    bits256 cardpriv;
    cardpriv = cards777_cardpriv(playerpriv,cardpubs,numcards,card);
    if ( cardpriv.txid != 0 )
    {
        if ( destplayer != slot )
            printf(">>>>>>>>>>>> ERROR ");
        if ( Debuglevel > 2 )
            printf("slot.%d B DECODED cardi.%d destplayer.%d cardpriv.[%d]\n",slot,cardi,destplayer,cardpriv.bytes[1]);
        *cardprivp = cardpriv;
        return(cardpriv.bytes[1]);
    }
    memset(cardprivp,0,sizeof(*cardprivp));
    return(-1);
}

int32_t cards777_shuffle(bits256 *shuffled,bits256 *cards,int32_t numcards,int32_t N)
{
    int32_t i,j,pos,nonz,permi[CARDS777_MAXCARDS],desti[CARDS777_MAXCARDS]; uint8_t x; uint64_t mask;
    memset(desti,0,sizeof(desti));
    for (i=0; i<numcards; i++)
        desti[i] = i;
    for (i=0; i<numcards; i++)
    {
        randombytes(&x,1);
        pos = (x % ((numcards-1-i) + 1));
        //printf("%d ",pos);
        permi[i] = desti[pos];
        desti[pos] = desti[numcards-1 - i];
        desti[numcards-1 - i] = -1;
    }
    //printf("pos\n");
    for (mask=i=nonz=0; i<numcards; i++)
    {
        if ( Debuglevel > 2 )
            printf("%d ",permi[i]);
        mask |= (1LL << permi[i]);
        for (j=0; j<N; j++,nonz++)
            shuffled[nonz] = cards[permi[i]*N + j];//, printf("%llx ",(long long)shuffled[nonz].txid);
    }
    if ( Debuglevel > 2 )
        printf("shuffled mask.%llx err.%llx\n",(long long)mask,(long long)(mask ^ ((1LL<<numcards)-1)));
    return(0);
}

void cards777_layer(bits256 *layered,bits256 *xoverz,bits256 *incards,int32_t numcards,int32_t N)
{
    int32_t i,k,nonz = 0; bits320 bp,x,z,x_z,z_x;
    bp = fexpand(curve25519_basepoint9());
    for (i=nonz=0; i<numcards; i++)
    {
        for (k=0; k<N; k++,nonz++)
        {
            cmult(&x,&z,rand256(1),bp);
            x_z = fmul(x,crecip(z));
            z_x = crecip(x_z);
            layered[nonz] = fcontract(fmul(z_x,fexpand(incards[nonz])));
            xoverz[nonz] = fcontract(x_z);
            //printf("{%llx -> %llx}.%d ",(long long)incards[nonz].txid,(long long)layered[nonz].txid,nonz);
        }
        //printf("card.%d\n",i);
    }
}

int32_t cards777_calcmofn(uint8_t *allshares,uint8_t *myshares[],uint8_t *sharenrs,int32_t M,bits256 *xoverz,int32_t numcards,int32_t N)
{
    int32_t size,j;
    size = N * sizeof(bits256) * numcards;
    calc_shares(allshares,(void *)xoverz,size,size,M,N,sharenrs); // PM &allshares[playerj * size] to playerJ
    for (j=0; j<N; j++)
        myshares[j] = &allshares[j * size];
    return(size);
}

uint8_t *cards777_recover(uint8_t *shares[],uint8_t *sharenrs,int32_t M,int32_t numcards,int32_t N)
{
    void *G; int32_t i,size; uint8_t *recover,recovernrs[255];
    size = N * sizeof(bits256) * numcards;
    recover = calloc(1,size);
    memset(recovernrs,0,sizeof(recovernrs));
    for (i=0; i<N; i++)
        if ( shares[i] != 0 )
            recovernrs[i] = sharenrs[i];
    G = gfshare_ctx_init_dec(recovernrs,N,size);
    for (i=0; i<N; i++)
        if ( shares[i] != 0 )
            gfshare_ctx_dec_giveshare(G,i,shares[i]);
    gfshare_ctx_dec_newshares(G,recovernrs);
    gfshare_ctx_dec_extract(G,recover);
    gfshare_ctx_free(G);
    return(recover);
}

/*bits256 cards777_pubkeys(bits256 *pubkeys,int32_t n,int32_t numcards)
{
    int32_t i; bits256 hash,check; bits320 prod,hexp,bp;
    memset(check.bytes,0,sizeof(check));
    if ( n == numcards+1 )
    {
        bp = fexpand(curve25519_basepoint9());
        prod = fmul(bp,crecip(bp));
        for (i=0; i<numcards; i++)
        {
            vcalc_sha256(0,hash.bytes,pubkeys[i].bytes,sizeof(pubkeys[i]));
            hash.bytes[0] &= 0xf8, hash.bytes[31] &= 0x7f, hash.bytes[31] |= 64;
            hexp = fexpand(hash);
            prod = fmul(prod,hexp);
        }
        check = fcontract(prod);
        if ( memcmp(check.bytes,pubkeys[numcards].bytes,sizeof(check)) != 0 )
            printf("permicheck.%llx != prod.%llx\n",(long long)check.txid,(long long)pubkeys[numcards].txid);
        else printf("pubkeys matched %llx\n",(long long)check.txid);
    } else printf("cards777_pubkeys n.%d != numcards.%d+1\n",n,numcards);
    return(check);
}*/

bits256 cards777_pubkeys(bits256 *pubkeys,int32_t numcards,bits256 cmppubkey)
{
    int32_t i; bits256 bp,pubkey,hash,check; bits320 prod,hexp; // cJSON *array; char *hexstr;
    memset(check.bytes,0,sizeof(check));
    memset(bp.bytes,0,sizeof(bp)), bp.bytes[0] = 9;
    prod = fmul(fexpand(bp),crecip(fexpand(bp)));
    for (i=0; i<numcards; i++)
    {
        pubkey = pubkeys[i];
        vcalc_sha256(0,hash.bytes,pubkey.bytes,sizeof(pubkey));
        hash.bytes[0] &= 0xf8, hash.bytes[31] &= 0x7f, hash.bytes[31] |= 64;
        hexp = fexpand(hash);
        prod = fmul(prod,hexp);
    }
    check = fcontract(prod);
    if ( cmppubkey.txid != 0 )
    {
        if ( memcmp(check.bytes,cmppubkey.bytes,sizeof(check)) != 0 )
            printf("permicheck.%llx != prod.%llx\n",(long long)check.txid,(long long)pubkey.txid);
        else printf("pubkeys matched\n");
    }
    return(check);
}

bits256 cards777_initdeck(bits256 *cards,bits256 *cardpubs,int32_t numcards,int32_t N,bits256 *playerpubs,bits256 *playerprivs)
{
    bits256 privkey,pubkey,hash; bits320 bp,prod,hexp; int32_t i,j,nonz,num = 0; uint64_t mask = 0;
    bp = fexpand(curve25519_basepoint9());
    prod = crecip(bp);
    prod = fmul(bp,prod);
    if ( Debuglevel > 2 )
        printf("card777_initdeck unit.%llx\n",(long long)prod.txid);
    nonz = 0;
    while ( mask != (1LL << numcards)-1 )
    {
        privkey = curve25519_keypair(&pubkey);
        if ( (i=privkey.bytes[1]) < numcards && ((1LL << i) & mask) == 0 )
        {
            mask |= (1LL << i);
            cardpubs[num] = pubkey;
            if ( playerprivs != 0 )
                printf("%llx.",(long long)privkey.txid);
            for (j=0; j<N; j++,nonz++)
            {
                cards[nonz] = cards777_initcrypt(privkey,privkey,playerpubs[j],0);
                if ( playerprivs != 0 )
                    printf("[%llx * %llx -> %llx] ",(long long)cards[nonz].txid,(long long)curve25519_shared(playerprivs[j],pubkey).txid,(long long)cards777_initcrypt(cards[nonz],playerprivs[j],pubkey,1).txid);
            }
            vcalc_sha256(0,hash.bytes,pubkey.bytes,sizeof(pubkey));
            hash.bytes[0] &= 0xf8, hash.bytes[31] &= 0x7f, hash.bytes[31] |= 64;
            hexp = fexpand(hash);
            prod = fmul(prod,hexp);
            num++;
        }
    }
    if ( playerprivs != 0 )
        printf("\n%llx %llx playerprivs\n",(long long)playerprivs[0].txid,(long long)playerprivs[1].txid);
    if ( Debuglevel > 2 )
    {
        for (i=0; i<numcards; i++)
            printf("%d ",cards[i*N].bytes[1]);
        printf("init order %llx (%llx %llx)\n",(long long)prod.txid,(long long)playerpubs[0].txid,(long long)playerpubs[1].txid);
    }
    return(fcontract(prod));
}

uint8_t *cards777_encode(bits256 *encoded,bits256 *xoverz,uint8_t *allshares,uint8_t *myshares[],uint8_t *sharenrs,int32_t M,bits256 *ciphers,int32_t numcards,int32_t N)
{
    bits256 shuffled[CARDS777_MAXCARDS * CARDS777_MAXPLAYERS];
    cards777_shuffle(shuffled,ciphers,numcards,N);
    cards777_layer(encoded,xoverz,shuffled,numcards,N);
    cards777_calcmofn(allshares,myshares,sharenrs,M,xoverz,numcards,N);
    memcpy(ciphers,shuffled,numcards * N * sizeof(bits256));
    if ( 0 )
    {
        int32_t i,j,m,size; uint8_t *recover,*testshares[CARDS777_MAXPLAYERS],testnrs[255];
        size = N * sizeof(bits256) * numcards;
        for (j=0; j<1; j++)
        {
            memset(testnrs,0,sizeof(testnrs));
            memset(testshares,0,sizeof(testshares));
            m = (rand() % N) + 1;
            if ( m < M )
                m = M;
            if ( init_sharenrs(testnrs,sharenrs,m,N) < 0 )
            {
                printf("iter.%d error init_sharenrs(m.%d of n.%d)\n",j,m,N);
                return(0);
            }
            for (i=0; i<N; i++)
                if ( testnrs[i] == sharenrs[i] )
                    testshares[i] = myshares[i];
            if ( (recover= cards777_recover(testshares,sharenrs,M,numcards,N)) != 0 )
            {
                if ( memcmp(xoverz,recover,size) != 0 )
                    fprintf(stderr,"(ERROR m.%d M.%d N.%d)\n",m,M,N);
                //else fprintf(stderr,"reconstructed with m.%d M.%d N.%d\n",m,M,N);
                free(recover);
            }
        }
    }
    return(allshares);
}

bits256 cards777_decode(bits256 *xoverz,int32_t destplayer,bits256 cipher,bits256 *outcards,int32_t numcards,int32_t N)
{
    int32_t i,ind;
    for (i=0; i<numcards; i++)
    {
        ind = i*N + destplayer;
        //printf("[%llx] ",(long long)outcards[ind].txid);
        if ( memcmp(outcards[ind].bytes,cipher.bytes,32) == 0 )
        {
            cipher = fcontract(fmul(fexpand(xoverz[ind]),fexpand(cipher)));
            //printf("matched %d -> %llx\n",i,(long long)cipher.txid);
            return(cipher);
        }
    }
    if ( i == numcards )
    {
        printf("decryption error %llx: destplayer.%d no match\n",(long long)cipher.txid,destplayer);
        memset(cipher.bytes,0,sizeof(cipher));
        //cipher = cards777_cardpriv(playerpriv,cardpubs,numcards,cipher);
    }
    return(cipher);
}

struct cards777_privdata *cards777_allocpriv(int32_t numcards,int32_t N)
{
    struct cards777_privdata *priv;
    priv = calloc(1,sizeof(*priv) + sizeof(bits256) * ((N+3) * N * numcards));
    priv->incards = &priv->data[0];
    priv->outcards = &priv->incards[N * numcards];
    priv->xoverz = &priv->outcards[N * numcards];
    priv->allshares = (void *)&priv->xoverz[N * numcards];
    return(priv);
}

struct cards777_pubdata *cards777_allocpub(int32_t M,int32_t numcards,int32_t N)
{
    struct cards777_pubdata *dp;
    dp = calloc(1,sizeof(*dp) + sizeof(bits256) * ((N + numcards + 1) + (N * numcards)));
    dp->M = M, dp->N = N, dp->numcards = numcards;
    dp->playerpubs = &dp->data[0];
    dp->hand.cardpubs = &dp->playerpubs[N];
    dp->hand.final = &dp->hand.cardpubs[numcards + 1];
    return(dp);
}

int32_t cards777_init(struct hostnet777_server *srv,int32_t M,struct hostnet777_client **clients,int32_t N,int32_t numcards)
{
    int32_t i,j; uint8_t sharenrs[255]; //,destplayer,cardibits256 *ciphers,cardpriv,card; uint64_t mask = 0;
    struct cards777_pubdata *dp; struct cards777_privdata *priv;
    if ( srv->num != N )
    {
        printf("srv->num.%d != N.%d\n",srv->num,N);
        return(-1);
    }
    memset(sharenrs,0,sizeof(sharenrs));
    init_sharenrs(sharenrs,0,N,N); // this needs to be done to start a hand
    for (i=0; i<N; i++)
    {
        dp = srv->clients[i].pubdata = cards777_allocpub(M,numcards,N);
        memcpy(dp->hand.sharenrs,sharenrs,dp->N);
        for (j=0; j<N; j++)
            dp->playerpubs[j] = srv->clients[j].pubkey;
        for (j=0; j<N; j++)
            dp->balances[j] = 100;
        priv = srv->clients[i].privdata = cards777_allocpriv(numcards,N);
        //priv->privkey = (i == 0) ? srv->H.privkey : clients[i]->H.privkey;
        /*if ( i == 0 )
            dp->checkprod = cards777_initdeck(priv->outcards,dp->cardpubs,numcards,N,dp->playerpubs), refdp = dp;
        else memcpy(dp->cardpubs,refdp->cardpubs,sizeof(*dp->cardpubs) * numcards);*/
    }
    return(0);
    /*priv = srv->clients[0].privdata;
    ciphers = priv->outcards;
    for (i=1; i<N; i++)
    {
        dp = srv->clients[i].pubdata;
        priv = srv->clients[i].privdata;
        cards777_encode(priv->outcards,priv->xoverz,priv->allshares,priv->myshares,dp->sharenrs,dp->M,ciphers,dp->numcards,dp->N);
        ciphers = priv->outcards;
    }
    for (cardi=0; cardi<dp->numcards; cardi++)
    {
        for (destplayer=0; destplayer<dp->N; destplayer++)
        {
            priv = srv->clients[dp->N - 1].privdata;
            card = priv->outcards[cardi*dp->N + destplayer];
            for (i=N-1; i>=0; i--)
            {
                j = (i > 0) ? i : destplayer;
                //printf("cardi.%d destplayer.%d i.%d j.%d\n",cardi,destplayer,i,j);
                dp = srv->clients[j].pubdata;
                priv = srv->clients[j].privdata;
                cardpriv = cards777_cardpriv(priv->privkey,dp->cardpubs,dp->numcards,card);
                if ( cardpriv.txid != 0 )
                {
                    mask |= (1LL << cardpriv.bytes[1]);
                    if ( destplayer != j )
                        printf(">>>>>>>>>>>> ERROR ");
                    printf("i.%d j.%d A DECODED cardi.%d destplayer.%d cardpriv.[%d] mask.%llx\n",i,j,cardi,destplayer,cardpriv.bytes[1],(long long)mask);
                    break;
                }
                card = cards777_decode(priv->xoverz,destplayer,card,priv->outcards,dp->numcards,dp->N);
                cardpriv = cards777_cardpriv(priv->privkey,dp->cardpubs,dp->numcards,card);
                if ( cardpriv.txid != 0 )
                {
                    mask |= (1LL << cardpriv.bytes[1]);
                    if ( destplayer != j )
                        printf(">>>>>>>>>>>> ERROR ");
                    printf("i.%d j.%d B DECODED cardi.%d destplayer.%d cardpriv.[%d] mask.%llx\n",i,j,cardi,destplayer,cardpriv.bytes[1],(long long)mask);
                    break;
                }
            }
        }
        printf("cardi.%d\n\n",cardi);
    }*/
    return(0);
}

#endif
#endif
