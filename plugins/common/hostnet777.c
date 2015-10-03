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
#ifndef hostnet777_h
#define hostnet777_h

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include "../utils/bits777.c"
#include "../utils/ramcoder.c"

#define HOSTNET777_MAXTIMEDIFF 10

#define CARDS777_MAXCARDS 52
#define CARDS777_MAXPLAYERS 9
#define CARDS777_ALLIN CARDS777_MAXPLAYERS
#define CARDS777_FOLD -1

struct cards777_handinfo
{
    bits256 checkprod,*cardpubs,*final;
    int64_t havemasks[CARDS777_MAXPLAYERS],betsize,lastraise,bets[CARDS777_MAXPLAYERS];
    uint32_t starttime,handmask,lastbettor,cardi,userinput_starttime,handranks[CARDS777_MAXPLAYERS];
    int8_t betstatus[CARDS777_MAXPLAYERS];
    uint8_t numactions,undergun,community[5],sharenrs[255],hands[CARDS777_MAXPLAYERS][7];
};

struct hostnet777_mtime { uint32_t starttime; int64_t millistart; double millidiff; };

struct cards777_pubdata
{
    bits256 *playerpubs; int64_t balances[CARDS777_MAXPLAYERS]; uint8_t M,N,numcards,isbot[CARDS777_MAXPLAYERS];
    uint32_t button,numhands,rakemillis,startdecktime,readymask; uint64_t hostrake,bigblind,ante,pangearake,othercardpubs[CARDS777_MAXPLAYERS];
    void *table; struct cards777_handinfo hand; char newhand[32768]; bits256 data[];
};

struct cards777_privdata
{
    bits256 holecards[2],*incards,*outcards,*xoverz;
    //,*reconstructed[CARDS777_MAXPLAYERS],*mofn[CARDS777_MAXPLAYERS][CARDS777_MAXPLAYERS];
    uint8_t *myshares[CARDS777_MAXPLAYERS],*allshares,hole[2]; bits256 data[];
};

struct hostnet777_endpoint { char endpoint[128],transport[16],ipaddr[64]; uint16_t port; };
struct hostnet777_id { bits256 pubkey; uint64_t nxt64bits; void *privdata,*pubdata; int32_t pmsock; uint32_t lastcontact; };
struct hostnet777_hdr { queue_t Q,Q2,Q3[2]; bits256 privkey,pubkey; struct hostnet777_mtime mT; void *privdata,*pubdata,*raft; uint64_t nxt64bits; uint32_t lastping; int32_t slot,done,state,ind; };
struct hostnet777_client { struct hostnet777_hdr H; int32_t pushsock,subsock; struct hostnet777_id my; };

struct hostnet777_server
{
    struct hostnet777_hdr H;
    int32_t num,max,pullsock,pubsock; struct hostnet777_endpoint ep;
    struct hostnet777_id clients[];
};
union hostnet777 { struct hostnet777_server *server; struct hostnet777_client *client; };

void hostnet777_msg(uint64_t destbits,bits256 destpub,union hostnet777 *src,int32_t blindflag,char *jsonstr,int32_t len);

int32_t cards777_init(struct hostnet777_server *srv,int32_t M,struct hostnet777_client **clients,int32_t N,int32_t numcards);
bits256 cards777_decode(bits256 *xoverz,int32_t destplayer,bits256 cipher,bits256 *outcards,int32_t numcards,int32_t N);
bits256 cards777_cardpriv(bits256 playerpriv,bits256 *cardpubs,int32_t numcards,bits256 cipher);
uint8_t *cards777_encode(bits256 *encoded,bits256 *xoverz,uint8_t *allshares,uint8_t *myshares[],uint8_t *sharenrs,int32_t M,bits256 *ciphers,int32_t numcards,int32_t N);
bits256 cards777_initdeck(bits256 *cards,bits256 *cardpubs,int32_t numcards,int32_t N,bits256 *playerpubs,bits256 *playerprivs);
int32_t init_sharenrs(unsigned char sharenrs[255],unsigned char *orig,int32_t m,int32_t n);
uint32_t set_handstr(char *handstr,uint8_t cards[7],int32_t verbose);
int32_t hostnet777_idle(union hostnet777 *hn);
void msleep(uint32_t milliseconds);
struct cards777_privdata *cards777_allocpriv(int32_t numcards,int32_t N);
struct cards777_pubdata *cards777_allocpub(int32_t M,int32_t numcards,int32_t N);
struct hostnet777_server *hostnet777_server(bits256 srvprivkey,bits256 srvpubkey,char *transport,char *ipaddr,uint16_t port,int32_t maxclients);
struct hostnet777_client *hostnet777_client(bits256 privkey,bits256 pubkey,char *srvendpoint,int32_t slot);
int32_t hostnet777_register(struct hostnet777_server *srv,bits256 clientpub,int32_t slot);
int32_t cards777_checkcard(bits256 *cardprivp,int32_t cardi,int32_t slot,int32_t destplayer,bits256 playerpriv,bits256 *cardpubs,int32_t numcards,bits256 card);
int32_t hostnet777_init(union hostnet777 *hn,bits256 *privkeys,int32_t num,int32_t launchflag);
int32_t hostnet777_sendmsg(union hostnet777 *ptr,bits256 destpub,bits256 mypriv,bits256 mypub,uint8_t *msg,int32_t len);
int64_t hostnet777_convmT(struct hostnet777_mtime *mT,int64_t othermillitime);
bits256 cards777_pubkeys(bits256 *pubkeys,int32_t numcards,bits256 cmppubkey);

extern int32_t Debuglevel;

#endif
#else
#ifndef hostnet777_c
#define hostnet777_c

#ifndef hostnet777_h
#define DEFINES_ONLY
#include "hostnet777.c"
#undef DEFINES_ONLY
#endif
#include "../includes/tweetnacl.h"
#include "../utils/curve25519.h"

int32_t nn_socket_status(int32_t sock,int32_t timeoutmillis);
int32_t nn_createsocket(char *endpoint,int32_t bindflag,char *name,int32_t type,uint16_t port,int32_t sendtimeout,int32_t recvtimeout);
void free_queueitem(void *itemptr);

static bits256 zeropoint;

int64_t hostnet777_convmT(struct hostnet777_mtime *mT,int64_t othermillitime)
{
    int64_t lag,millitime,millis = (uint64_t)milliseconds();
    if ( mT->starttime == 0 )
    {
        mT->starttime = (uint32_t)time(NULL);
        mT->millistart = millis;
        printf("set millistart.%p %lld\n",mT,(long long)millis);
    }
    //printf("%p millis.%lld - millistart.%lld = %lld\n",mT,(long long)millis,(long long)mT->millistart,(long long)(millis - mT->millistart));
    millitime = (millis - mT->millistart) + ((long long)mT->starttime * 1000);
    if ( othermillitime != 0 )
    {
        millitime += mT->millidiff;
        lag = (othermillitime - millitime);
        mT->millidiff = (mT->millidiff * .9) + (.1 * lag);
    }
    return(millitime);
}

double hostnet777_updatelag(uint64_t senderbits,int64_t millitime,int64_t now)
{
    printf("updatelag %llu: %lld\n",(long long)senderbits,(long long)(millitime - now));
    return(millitime - now);
}

int32_t hostnet777_send(int32_t sock,void *ptr,int32_t len)
{
    int32_t j,sendlen = 0;
    if ( sock >= 0 )
    {
        for (j=0; j<10; j++)
            if ( (nn_socket_status(sock,10) & NN_POLLOUT) != 0 )
                break;
        for (j=0; j<10; j++)
        {
            if ( (sendlen= nn_send(sock,ptr,len,0)) == len )
                break;
            printf("retry.%d for sock.%d len.%d vs sendlen.%d\n",j,sock,len,sendlen);
            sleep(1);
        }
        //printf("hostnet777_send.%d j.%d len.%d sendlen.%d\n",sock,j,len,sendlen);
    } else printf("hostnet777_send neg socket\n");
    return(sendlen);
}

struct hostnet777_id *hostnet777_find64(struct hostnet777_server *srv,uint64_t senderbits)
{
    int32_t i;
    if ( srv->num > 0 )
    {
        for (i=0; i<srv->max; i++)
            if ( srv->clients[i].nxt64bits == senderbits )
                return(&srv->clients[i]);
    }
    return(0);
}

int32_t hostnet777_sendsock(union hostnet777 *ptr,uint64_t destbits)
{
    int32_t ind; struct hostnet777_id *client;
    if ( (ind= ptr->client->H.slot) != 0 )
    {
        //printf("client.%p ind.%d: %d %d\n",ptr->client,ind,ptr->client->pushsock,ptr->client->my.pmsock);
        if ( destbits == 0 )
            return(ptr->client->pushsock);
        else return(ptr->client->my.pmsock);
    }
    else
    {
        //printf("server.%p ind.%d: %d %d\n",ptr->server,ind,ptr->server->pullsock,ptr->server->pubsock);
        if ( destbits == 0 )
            return(ptr->server->pubsock);
        else if ( (client= hostnet777_find64(ptr->server,destbits)) != 0 )
        {
            //printf("SERVER -> ind.%d: %d %d\n",ind,ptr->server->pubsock,client->pmsock);
            return(client->pmsock);
        } else printf("error cant find %llu in server clients\n",(long long)destbits);
    }
    return(-1);
}

struct hostnet777_id *hostnet777_find(struct hostnet777_server *srv,bits256 senderpub)
{
    int32_t i; uint64_t senderbits = acct777_nxt64bits(senderpub);
    if ( srv->num > 0 )
    {
        for (i=0; i<srv->max; i++)
            if ( srv->clients[i].nxt64bits == senderbits )
                return(&srv->clients[i]);
    }
    return(0);
}

void hostnet777_lastcontact(struct hostnet777_server *srv,bits256 senderpub)
{
    struct hostnet777_id *ptr;
    if ( (ptr= hostnet777_find(srv,senderpub)) != 0 )
        ptr->lastcontact = (uint32_t)time(NULL);
}

int32_t hostnet777_copybits(int32_t reverse,uint8_t *dest,uint8_t *src,int32_t len)
{
    int32_t i; uint8_t *tmp;
    if ( reverse != 0 )
    {
        tmp = dest;
        dest = src;
        src = tmp;
    }
    memset(dest,0,len);
    len <<= 3;
    for (i=0; i<len; i++)
        if ( GETBIT(src,i) != 0 )
            SETBIT(dest,i);
    return(len >> 3);
}

int32_t hostnet777_serialize(int32_t reverse,bits256 *senderpubp,uint64_t *senderbitsp,bits256 *sigp,uint32_t *timestampp,uint64_t *destbitsp,uint8_t *buf)
{
    long extra = sizeof(bits256) + sizeof(uint64_t) + sizeof(uint64_t);
    buf += hostnet777_copybits(reverse,buf,(void *)destbitsp,sizeof(uint64_t));
    buf += hostnet777_copybits(reverse,buf,senderpubp->bytes,sizeof(bits256));
    buf += hostnet777_copybits(reverse,buf,(void *)senderbitsp,sizeof(uint64_t));
    buf += hostnet777_copybits(reverse,buf,(void *)timestampp,sizeof(uint32_t)), extra += sizeof(uint32_t);
    if ( *senderbitsp != 0 )
        buf += hostnet777_copybits(reverse,buf,sigp->bytes,sizeof(bits256)), extra += sizeof(bits256);
    else memset(sigp,0,sizeof(*sigp));
    return((int32_t)extra);
}

uint8_t *hostnet777_encode(int32_t *cipherlenp,void *str,int32_t len,bits256 destpubkey,bits256 myprivkey,bits256 mypubkey,uint64_t senderbits,bits256 sig,uint32_t timestamp)
{
    uint8_t *buf,*nonce,*cipher,*ptr; uint64_t destbits; int32_t totalsize,hdrlen; long extra = crypto_box_NONCEBYTES + crypto_box_ZEROBYTES + sizeof(sig);
    destbits = (memcmp(destpubkey.bytes,GENESIS_PUBKEY.bytes,sizeof(destpubkey)) != 0) ? acct777_nxt64bits(destpubkey) : 0;
    totalsize = (int32_t)(len + sizeof(mypubkey) + sizeof(senderbits) + sizeof(destbits) + sizeof(timestamp));
    buf = calloc(1,totalsize + extra);
    ptr = cipher = calloc(1,totalsize + extra);
    hdrlen = hostnet777_serialize(0,&mypubkey,&senderbits,&sig,&timestamp,&destbits,cipher);
    if ( senderbits != 0 )
        totalsize += sizeof(sig);//, printf("totalsize.%d extra.%ld add %ld\n",totalsize-len,extra,sizeof(sig) + sizeof(timestamp));
    if ( destbits != 0 && senderbits != 0 )
    {
        totalsize += crypto_box_NONCEBYTES + crypto_box_ZEROBYTES;//, printf("totalsize.%d extra.%ld add %d\n",totalsize-len,extra,crypto_box_NONCEBYTES + crypto_box_ZEROBYTES);
        nonce = &cipher[hdrlen];
        randombytes(nonce,crypto_box_NONCEBYTES);
        cipher = &nonce[crypto_box_NONCEBYTES];
        //printf("len.%d -> %d %d\n",len,len+crypto_box_ZEROBYTES,len + crypto_box_ZEROBYTES + crypto_box_NONCEBYTES);
        memset(cipher,0,len+crypto_box_ZEROBYTES);
        memset(buf,0,crypto_box_ZEROBYTES);
        memcpy(buf+crypto_box_ZEROBYTES,str,len);
        crypto_box(cipher,buf,len+crypto_box_ZEROBYTES,nonce,destpubkey.bytes,myprivkey.bytes);
        hdrlen += crypto_box_NONCEBYTES + crypto_box_ZEROBYTES;
    }
    else memcpy(&cipher[hdrlen],str,len);
    if ( totalsize != len+hdrlen )
        printf("unexpected totalsize.%d != len.%d + hdrlen.%d %d\n",totalsize,len,hdrlen,len+hdrlen);
    free(buf);
    *cipherlenp = totalsize;
    return(ptr);
}

int32_t hostnet777_decode(uint64_t *senderbitsp,bits256 *sigp,uint32_t *timestampp,uint64_t *destbitsp,uint8_t *str,uint8_t *cipher,int32_t *lenp,uint8_t *myprivkey)
{
    bits256 srcpubkey; uint8_t *nonce; int i,hdrlen,err=0,len = *lenp;
    hdrlen = hostnet777_serialize(1,&srcpubkey,senderbitsp,sigp,timestampp,destbitsp,cipher);
    cipher += hdrlen, len -= hdrlen;
    if ( *destbitsp != 0 && *senderbitsp != 0 )
    {
        nonce = cipher;
        cipher += crypto_box_NONCEBYTES, len -= crypto_box_NONCEBYTES;
        err = crypto_box_open((uint8_t *)str,cipher,len,nonce,srcpubkey.bytes,myprivkey);
        for (i=0; i<len-crypto_box_ZEROBYTES; i++)
            str[i] = str[i+crypto_box_ZEROBYTES];
        *lenp = len - crypto_box_ZEROBYTES;
    } else memcpy(str,cipher,len);
    return(err);
}

int32_t hostnet777_decrypt(bits256 *senderpubp,uint64_t *senderbitsp,uint32_t *timestampp,bits256 mypriv,bits256 mypub,uint8_t *dest,int32_t maxlen,uint8_t *src,int32_t len)
{
    bits256 seed,sig,msgpriv; uint64_t my64bits,destbits,senderbits,sendertmp,desttmp;
    uint8_t *buf; int32_t hdrlen,i,diff,newlen = -1; HUFF H,*hp = &H; struct acct777_sig checksig;
    *senderbitsp = 0;
    my64bits = acct777_nxt64bits(mypub);
    buf = calloc(1,maxlen);
    hdrlen = hostnet777_serialize(1,senderpubp,&senderbits,&sig,timestampp,&destbits,src);
    if ( destbits != 0 && my64bits != destbits && destbits != acct777_nxt64bits(GENESIS_PUBKEY) )
    {
        printf("hostnet777_decrypt received destination packet.%llu when my64bits.%llu len.%d\n",(long long)destbits,(long long)my64bits,len);
        return(-1);
    }
    if ( memcmp(mypub.bytes,senderpubp->bytes,sizeof(mypub)) == 0 )
    {
        if ( destbits != 0 )
            printf("hostnet777: got my own msg?\n");
    }
    //printf("decrypt(%d) destbits.%llu my64.%llu mypriv.%llx mypub.%llx senderpub.%llx shared.%llx\n",len,(long long)destbits,(long long)my64bits,(long long)mypriv.txid,(long long)mypub.txid,(long long)senderpubp->txid,(long long)seed.txid);
    if ( hostnet777_decode(&sendertmp,&sig,timestampp,&desttmp,(void *)buf,src,&len,mypriv.bytes) == 0 )
    {
        if ( (diff= (*timestampp - (uint32_t)time(NULL))) < 0 )
            diff = -diff;
        if ( 0 && diff > HOSTNET777_MAXTIMEDIFF )
            printf("diff.%d > %d %u vs %u\n",diff,HOSTNET777_MAXTIMEDIFF,*timestampp,(uint32_t)time(NULL));
        else
        {
            if ( 1 )
            {
                memset(seed.bytes,0,sizeof(seed));
                for (i='0'; i<='9'; i++)
                    SETBIT(seed.bytes,i);
                for (i='a'; i<='f'; i++)
                    SETBIT(seed.bytes,i);
                _init_HUFF(hp,len,buf), hp->endpos = (len << 3);
                newlen = ramcoder_decoder(0,1,dest,maxlen,hp,&seed);
            }
            else memcpy(dest,buf,len), newlen = len;
            //printf("T%d decrypted newlen.%d\n",threadid,newlen);
            if ( senderbits != 0 && senderpubp->txid != 0 )
            {
                *senderbitsp = senderbits;
                if ( destbits == 0 )
                    msgpriv = GENESIS_PRIVKEY;
                else msgpriv = mypriv;
                acct777_sign(&checksig,msgpriv,*senderpubp,*timestampp,dest,newlen);
                if ( memcmp(checksig.sigbits.bytes,&sig,sizeof(checksig.sigbits)) != 0 )
                {
                    printf("sender.%llu sig %llx compare error vs %llx using sig->pub from %llu, broadcast.%d\n",(long long)senderbits,(long long)sig.txid,(long long)checksig.sigbits.txid,(long long)senderbits,destbits == 0);
                    //free(buf);
                    //return(0);
                } //else printf("SIG VERIFIED newlen.%d (%llu -> %llu)\n",newlen,(long long)senderbits,(long long)destbits);
            }
        }
    }
    else printf("%llu: hostnet777_decrypt skip: decode_cipher error len.%d -> newlen.%d\n",(long long)acct777_nxt64bits(mypub),len,newlen);
    free(buf);
    return(newlen);
}

void hostnet777_processmsg(uint64_t *destbitsp,bits256 *senderpubp,queue_t *Q,bits256 mypriv,bits256 mypub,uint8_t *msg,int32_t origlen,int32_t pmflag,struct hostnet777_mtime *mT)
{
    char *jsonstr = 0; bits256 sig; uint32_t timestamp; int32_t len; uint64_t senderbits,now,millitime; uint8_t *ptr; cJSON *json; long extra;
    extra = sizeof(*senderpubp) + sizeof(*destbitsp) + sizeof(sig) + sizeof(senderbits) + sizeof(timestamp);
    if ( (len= origlen) > extra )
    {
        ptr = malloc(len*4 + 8192 + sizeof(struct queueitem) - extra);
        if ( (len= hostnet777_decrypt(senderpubp,&senderbits,&timestamp,mypriv,mypub,&ptr[sizeof(struct queueitem)],len*4,msg,len)) > 1 && len < len*4 )
        {
            jsonstr = (char *)&ptr[sizeof(struct queueitem)];
            if ( (json= cJSON_Parse(jsonstr)) != 0 )
            {
                millitime = j64bits(json,"millitime");
                now = hostnet777_convmT(mT,millitime);
                //printf("now.%lld vs millitime.%lld lag.%lld\n",(long long)now,(long long)millitime,(long long)(millitime - now));
                if ( pmflag != 0 && juint(json,"timestamp") != timestamp && juint(json,"timestamp")+1 != timestamp )
                    printf("msg.(%s) timestamp.%u mismatch | now.%ld\n",jsonstr,timestamp,time(NULL)), free(ptr);
                else if ( pmflag != 0 && j64bits(json,"sender") != senderbits )
                    printf("msg.(%ld) sender.%llu mismatch vs json.%llu\n",strlen(jsonstr),(long long)senderbits,(long long)j64bits(json,"sender")), free(ptr);
                else
                {
                    //printf("%llu: QUEUE msg.%d\n",(long long)acct777_nxt64bits(mypub),len);
                    queue_enqueue("host777",Q,(void *)ptr);
                }
                free_json(json);
            } else printf("parse error.(%s)\n",jsonstr);
        } else free(ptr), printf("decrypt error len.%d origlen.%d\n",len,origlen);
    } else printf("origlen.%d\n",origlen);
    nn_freemsg(msg);
}

#define hostnet777_broadcast(ptr,mypriv,mypub,msg,len) hostnet777_sendmsg(ptr,zeropoint,mypriv,mypub,msg,len)
#define hostnet777_blindcast(ptr,msg,len) hostnet777_sendmsg(ptr,zeropoint,zeropoint,zeropoint,msg,len)
#define hostnet777_signedPM(ptr,destpub,mypriv,mypub,msg,len) hostnet777_sendmsg(ptr,destpub,mypriv,mypub,msg,len)
#define hostnet777_blindPM(ptr,destpub,msg,len) hostnet777_sendmsg(ptr,destpub,zeropoint,zeropoint,msg,len)

int32_t hostnet777_sendmsg(union hostnet777 *ptr,bits256 destpub,bits256 mypriv,bits256 mypub,uint8_t *msg,int32_t len)
{
    int32_t cipherlen,datalen,sendsock,i; bits256 seed; uint8_t *data=0,*cipher; uint64_t destbits; struct acct777_sig sig; HUFF H,*hp = &H;
    if ( destpub.txid != 0 )
        destbits = acct777_nxt64bits(destpub);
    else
    {
        destbits = 0;
        destpub = GENESIS_PUBKEY;
    }
    //printf("hostnet777_sendmsg dest.%llu destpub.%llx priv.%llx pub.%llx\n",(long long)destbits,(long long)destpub.txid,(long long)mypriv.txid,(long long)mypub.txid);
    memset(&sig,0,sizeof(sig));
    if ( mypub.txid == 0 || mypriv.txid == 0 )
        mypriv = curve25519_keypair(&mypub), sig.timestamp = (uint32_t)time(NULL);
    else acct777_sign(&sig,mypriv,destpub,(uint32_t)time(NULL),msg,len);
    if ( (sendsock= hostnet777_sendsock(ptr,mypriv.txid != 0 ? destbits : 0)) < 0 )
    {
        printf("%llu: ind.%d no sendsock for %llx -> %llu\n",(long long)ptr->client->H.nxt64bits,ptr->client->H.slot,(long long)acct777_nxt64bits(mypub),(long long)destbits);
        return(-1);
    }
    if ( 1 )
    {
        data = calloc(1,len*2);
        _init_HUFF(hp,len*2,data);
        memset(seed.bytes,0,sizeof(seed));
        for (i='0'; i<='9'; i++)
            SETBIT(seed.bytes,i);
        for (i='a'; i<='f'; i++)
            SETBIT(seed.bytes,i);
        ramcoder_encoder(0,1,msg,len,hp,&seed);
        datalen = hconv_bitlen(hp->bitoffset);
    }
    else data = msg, datalen = len;
    if ( (cipher= hostnet777_encode(&cipherlen,data,datalen,destpub,mypriv,mypub,sig.signer64bits,sig.sigbits,sig.timestamp)) != 0 )
    {
        //printf("my.(priv.%llx pub.%llx) -> dest %llu pub.%llx cipherlen.%d %llx sendsock %d linksock.%d\n",(long long)pangea_privkey(player).txid,(long long)pangea_pubkey(player).txid,(long long)destbits,(long long)destpub.txid,cipherlen,*(long long *)cipher,sendsock,linksock);
        hostnet777_send(sendsock,cipher,cipherlen);
        free(cipher);
    }
    if ( data != msg )
        free(data);
    return(cipherlen);
}

int32_t hostnet777_idle(union hostnet777 *hn)
{
    int32_t len,ind,j,sock,n = 0; bits256 senderpub,mypriv,mypub; uint64_t destbits; uint8_t *msg; long extra = sizeof(bits256)+sizeof(uint64_t);
    if ( (ind= hn->client->H.slot) != 0 )
    {
        mypriv = hn->client->H.privkey, mypub = hn->client->H.pubkey;
        if ( (sock= hn->client->subsock) >= 0 && (len= nn_recv(sock,&msg,NN_MSG,0)) > extra )
            hostnet777_processmsg(&destbits,&senderpub,&hn->client->H.Q,mypriv,mypub,msg,len,0,&hn->client->H.mT), n++;
        if ( (sock= hn->client->my.pmsock) >= 0 && (len= nn_recv(sock,&msg,NN_MSG,0)) > extra )
        {
            //printf("client got pmsock.%d\n",len);
            hostnet777_processmsg(&destbits,&senderpub,&hn->client->H.Q,mypriv,mypub,msg,len,1,&hn->client->H.mT), n++;
        }
    }
    else
    {
        mypriv = hn->server->H.privkey, mypub = hn->server->H.pubkey;
        if ( (sock= hn->server->pullsock) >= 0 && (len= nn_recv(sock,&msg,NN_MSG,0)) > extra )
        {
            hostnet777_send(hn->server->pubsock,msg,len);
            hostnet777_processmsg(&destbits,&senderpub,&hn->server->H.Q,mypriv,mypub,msg,len,0,&hn->server->H.mT), n++;
            //printf("got pullsock destbits.%llu sender.%llu\n",(long long)destbits,(long long)acct777_nxt64bits(senderpub));
            hostnet777_lastcontact(hn->server,senderpub);
        }
        for (ind=1; ind<hn->server->max; ind++)
        {
            if ( (sock= hn->server->clients[ind].pmsock) >= 0 && (len= nn_recv(sock,&msg,NN_MSG,0)) > extra )
            {
                hostnet777_copybits(1,msg,(void *)&destbits,sizeof(uint64_t));
                for (j=0; j<hn->server->max; j++)
                {
                    if ( hn->server->clients[j].nxt64bits == destbits )
                    {
                        if  ( j == 0 )
                        {
                            //printf("server got PM sock.%d\n",hn->server->clients[j].pmsock);
                            hostnet777_processmsg(&destbits,&senderpub,&hn->server->H.Q,mypriv,mypub,msg,len,1,&hn->server->H.mT);
                            hostnet777_lastcontact(hn->server,senderpub);
                        }
                        else
                        {
                            //printf("forward to slot.%d sock.%d\n",j,hn->server->clients[j].pmsock);
                            hostnet777_send(hn->server->clients[j].pmsock,msg,len);
                            nn_freemsg(msg);
                        }
                        n++;
                        break;
                    }
                }
                if ( j == hn->server->max )
                {
                    printf("got msg to %llu not a client\n",(long long)destbits);
                    nn_freemsg(msg);
                }
            }
        }
    }
    return(n);
}

int32_t hostnet777_register(struct hostnet777_server *srv,bits256 clientpub,int32_t slot)
{
    int32_t i,n; struct hostnet777_id *ptr; char endpoint[128]; uint64_t nxt64bits = acct777_nxt64bits(clientpub);
    if ( slot < 0 )
    {
        if ( (ptr= hostnet777_find(srv,clientpub)) != 0 )
        {
            slot = (int32_t)(((long)ptr - (long)srv->clients) / sizeof(*srv->clients));
            //printf("hostnet777_register: deregister slot.%d\n",slot);
            if ( ptr->pmsock >= 0 )
                nn_shutdown(ptr->pmsock,0);
            memset(ptr,0,sizeof(*ptr));
            ptr->pmsock = -1;
            srv->num--;
            return(-1);
        }
        for (slot=1; slot<srv->max; slot++)
            if ( srv->clients[slot].nxt64bits == 0 )
                break;
    }
    if ( srv->num >= srv->max )
    {
        printf("hostnet777_register: cant register anymore num.%d vs max.%d\n",srv->num,srv->max);
        return(-1);
    }
    if ( (ptr= hostnet777_find(srv,clientpub)) != 0 )
    {
        printf("hostnet777_register: cant register duplicate %llu\n",(long long)nxt64bits);
        return((int32_t)(((long)ptr - (long)srv->clients) / sizeof(*srv->clients)));
    }
    if ( slot != srv->num )
    {
        printf("hostnet777_register: cant register slot.%d vs num.%d vs max.%d\n",slot,srv->num,srv->max);
        return(-1);
    }
    sprintf(endpoint,"%s://%s:%u",srv->ep.transport,srv->ep.ipaddr,srv->ep.port + slot + 2);
    srv->clients[slot].pmsock = nn_createsocket(endpoint,1,"NN_PAIR",NN_PAIR,srv->ep.port + slot + 2,100,10);
    srv->clients[slot].pubkey = clientpub;
    srv->clients[slot].nxt64bits = nxt64bits;
    srv->clients[slot].lastcontact = (uint32_t)time(NULL);
    srv->num++;
    for (i=n=0; i<srv->max; i++)
        if ( srv->clients[i].nxt64bits != 0 )
            n++;
    if ( n != srv->num )
    {
        printf("mismatched nonz nxt64bits n.%d vs %d\n",n,srv->num);
        srv->num = n;
    }
    return(slot);
}

struct hostnet777_client *hostnet777_client(bits256 privkey,bits256 pubkey,char *srvendpoint,int32_t slot)
{
    char endbuf[128],endbuf2[128]; uint16_t port; struct hostnet777_client *ptr;
    ptr = calloc(1,sizeof(*ptr));
    ptr->H.slot = slot;
    ptr->H.privkey = privkey, ptr->H.pubkey = ptr->my.pubkey = pubkey;
    ptr->H.nxt64bits = ptr->my.nxt64bits = acct777_nxt64bits(pubkey);
    ptr->my.lastcontact = (uint32_t)time(NULL);
    strcpy(endbuf,srvendpoint);
    endbuf[strlen(endbuf)-4] = 0;
    port = atoi(&srvendpoint[strlen(endbuf)]);
    sprintf(endbuf2,"%s%u",endbuf,port + 2 + slot);
    ptr->my.pmsock = nn_createsocket(endbuf2,0,"NN_PAIR",NN_PAIR,0,100,10);
    printf("PAIR %d from (%s) port.%d\n",ptr->my.pmsock,endbuf2,port+2+slot);
    sprintf(endbuf2,"%s%u",endbuf,port + 1);
    ptr->subsock = nn_createsocket(endbuf2,0,"NN_SUB",NN_SUB,0,100,10);
    printf("SUB %d from (%s) port.%d\n",ptr->subsock,endbuf2,port+1);
    nn_setsockopt(ptr->subsock,NN_SUB,NN_SUB_SUBSCRIBE,"",0);
    sprintf(endbuf2,"%s%u",endbuf,port);
    ptr->pushsock = nn_createsocket(endbuf2,0,"NN_PUSH",NN_PUSH,0,100,10);
    printf("PUSH %d to (%s)\n",ptr->pushsock,endbuf2);
    return(ptr);
}

void hostnet777_freeclient(struct hostnet777_client *client)
{
    client->H.done = 1;
    if ( client->subsock >= 0 )
        nn_shutdown(client->subsock,0);
    if ( client->pushsock >= 0 )
        nn_shutdown(client->pushsock,0);
    if ( client->my.pmsock >= 0 )
        nn_shutdown(client->my.pmsock,0);
}

void hostnet777_freeserver(struct hostnet777_server *srv)
{
    int32_t ind;
    srv->H.done = 1;
    if ( srv->pullsock >= 0 )
        nn_shutdown(srv->pullsock,0);
    if ( srv->pubsock >= 0 )
        nn_shutdown(srv->pubsock,0);
    for (ind=1; ind<srv->max; ind++)
    {
        if ( srv->clients[ind].pmsock >= 0 )
            nn_shutdown(srv->clients[ind].pmsock,0);
    }
}

struct hostnet777_server *hostnet777_server(bits256 srvprivkey,bits256 srvpubkey,char *transport,char *ipaddr,uint16_t port,int32_t maxclients)
{
    struct hostnet777_server *srv; struct hostnet777_endpoint *ep;
    srv = calloc(1,sizeof(*srv) + maxclients*sizeof(struct hostnet777_id));
    srv->max = maxclients;
    ep = &srv->ep;
    if ( (ep->port= port) == 0 )
        ep->port = port = 8000 + (rand() % 1000);
    if ( transport == 0 || transport[0] == 0 )
        transport = "tcp";
    if ( ipaddr == 0 || ipaddr[0] == 0 )
        ipaddr = "127.0.0.1";
    strcpy(ep->transport,transport), strcpy(ep->ipaddr,ipaddr);
    srv->clients[0].pmsock = -1;
    srv->H.privkey = srvprivkey;
    srv->H.pubkey = srv->clients[0].pubkey = srvpubkey;
    srv->H.nxt64bits = srv->clients[0].nxt64bits = acct777_nxt64bits(srvpubkey);
    sprintf(ep->endpoint,"%s://%s:%u",transport,ipaddr,port + 1);
    srv->pubsock = nn_createsocket(ep->endpoint,1,"NN_PUB",NN_PUB,port + 1,100,10);
    printf("PUB.%d to (%s)\n",srv->pubsock,ep->endpoint);
    sprintf(ep->endpoint,"%s://%s:%u",transport,ipaddr,port);
    srv->pullsock = nn_createsocket(ep->endpoint,1,"NN_PULL",NN_PULL,port,100,10);
    printf("PULL.%d from (%s)\n",srv->pullsock,ep->endpoint);
    srv->num = 1;
    return(srv);
}

void *hostnet777_idler(union hostnet777 *ptr)
{
    while ( ptr->client->H.done == 0 )
    {
        if ( hostnet777_idle(ptr) == 0 )
            msleep(1);
    }
    //printf("hostnet777_idler ind.%d done\n",ptr->client->H.slot);
    sleep(1);
    free(ptr);
    return(0);
}

void hostnet777_msg(uint64_t destbits,bits256 destpub,union hostnet777 *src,int32_t blindflag,char *jsonstr,int32_t len)
{
    if ( destbits == 0 )
    {
        //printf(">>>>>>>>> blind.%d broadcast from %llu, len.%d\n",blindflag,(long long)src->client->H.nxt64bits,len);
        if ( blindflag != 0 )
            hostnet777_blindcast(src,(uint8_t *)jsonstr,len);
        else hostnet777_broadcast(src,src->client->H.privkey,src->client->H.pubkey,(uint8_t *)jsonstr,len);
        if ( src->server->H.slot == 0 )
            queue_enqueue("loopback",&src->client->H.Q,queueitem(jsonstr));
    }
    else if ( destbits != src->client->H.nxt64bits )
    {
        //printf(">>>>>>>>> blind.%d PM from %llu to %llu\n",blindflag,(long long)src->client->H.nxt64bits,(long long)destbits);
        if ( blindflag != 0 )
            hostnet777_blindPM(src,destpub,(uint8_t *)jsonstr,len);
        else hostnet777_signedPM(src,destpub,src->client->H.privkey,src->client->H.pubkey,(uint8_t *)jsonstr,len);
    }
    else queue_enqueue("loopback",&src->client->H.Q,queueitem(jsonstr));
}

int32_t hostnet777_init(union hostnet777 *hn,bits256 *privkeys,int32_t num,int32_t launchflag)
{
    bits256 pubkey; int32_t slot,threadid; struct hostnet777_server *srv;
    for (threadid=0; threadid<num; threadid++)
    {
        pubkey = acct777_pubkey(privkeys[threadid]);
        if ( threadid == 0 )
        {
            if ( (srv= hostnet777_server(privkeys[threadid],pubkey,0,0,0,num)) == 0 )
            {
                printf("cant create hostnet777 server\n");
                return(-1);
            }
            hn[0].server = srv;
            srv->H.privkey = privkeys[threadid], srv->H.pubkey = pubkey;
            if ( launchflag != 0 && portable_thread_create((void *)hostnet777_idler,&hn[0]) == 0 )
                printf("error launching server thread\n");
        }
        else
        {
            if ( (slot= hostnet777_register(srv,pubkey,-1)) >= 0 )
            {
                if ( (hn[threadid].client= hostnet777_client(privkeys[threadid],pubkey,srv->ep.endpoint,slot)) == 0 )
                    printf("error creating clients[%d]\n",threadid);
                else
                {
                    hn[threadid].client->H.privkey = privkeys[threadid], hn[threadid].client->H.pubkey = pubkey;
                    printf("slot.%d client.%p -> %llu pubkey.%llx\n",slot,hn[threadid].client,(long long)hn[threadid].client->H.nxt64bits,(long long)hn[threadid].client->H.pubkey.txid);
                    if ( launchflag != 0 && portable_thread_create((void *)hostnet777_idler,&hn[threadid]) == 0 )
                        printf("error launching clients[%d] thread\n",threadid);
                }
            }
        }
    }
    return(num);
}
     
int32_t hostnet777_block(struct hostnet777_server *srv,uint64_t *senderbitsp,uint32_t *timestampp,union hostnet777 *hn,uint8_t *data,int32_t len,uint8_t *buf,int32_t maxmicro,int32_t blind,int32_t revealed)
{
    static int32_t errs;
    char *jsonstr,*hexstr,*cmdstr,*nrs,*handstr,tmp[128]; cJSON *json; void *val; struct cards777_privdata *priv; struct cards777_pubdata *dp;
    int32_t i,j,cardi,bestj,destplayer,card,senderslot,retval = -1; bits256 cardpriv; uint32_t rank,bestrank;
    *senderbitsp = 0;
    if ( hn == 0 || hn->client == 0 )
    {
        printf("null hn.%p %p\n",hn,hn!=0?hn->client:0);
        return(-1);
    }
    dp = srv->clients[hn->client->H.slot].pubdata;
    priv = srv->clients[hn->client->H.slot].privdata;
    for (i=0; i<maxmicro; i++)
    {
        if ( (jsonstr= queue_dequeue(&hn->client->H.Q,1)) != 0 )
        {
            if ( (json= cJSON_Parse(jsonstr)) != 0 )
            {
                *senderbitsp = j64bits(json,"sender");
                *timestampp = juint(json,"timestamp");
                if ( (hexstr= jstr(json,"data")) != 0 && strlen(hexstr) == (juint(json,"n")<<1) )
                {
                    decode_hex(buf,len,hexstr);
                    if ( memcmp(buf,data,len) == 0 )
                    {
                        val = hostnet777_find64(srv,*senderbitsp);
                        //printf("blind.%d val.%p\n",blind,val);
                        if ( (blind == 0 && val != 0) || (blind != 0 && val == 0) )
                        {
                            if ( (cmdstr= jstr(json,"cmd")) != 0 )
                            {
                                cardi = juint(json,"cardi");
                                destplayer = juint(json,"dest");
                                senderslot = juint(json,"myslot");
                                if ( strcmp(cmdstr,"pubstr") == 0 )
                                {
                                    //printf("player.%d got pubstr\n",hn->client->H.slot);
                                    memcpy(dp->hand.cardpubs,buf,len);
                                    if ( (nrs= jstr(json,"sharenrs")) != 0 )
                                        decode_hex(dp->hand.sharenrs,(int32_t)strlen(nrs)>>1,nrs);
                                    memset(dp->hand.handranks,0,sizeof(dp->hand.handranks));
                                    memset(priv->hole,0,sizeof(priv->hole));
                                    memset(priv->holecards,0,sizeof(priv->holecards));
                                    memset(dp->hand.community,0,sizeof(dp->hand.community));
                                    dp->hand.handmask = 0;
                                    dp->numhands++;
                                    dp->button++;
                                    if ( dp->button >= dp->N )
                                        dp->button = 0;
                                    dp->balances[dp->button]--, dp->balances[(dp->button + 1) % dp->N] -= 2;
                                }
                                else if ( strcmp(cmdstr,"encode") == 0 )
                                {
                                    if ( Debuglevel > 2 )
                                        printf("player.%d encodes\n",hn->client->H.slot);
                                    cards777_encode(priv->outcards,priv->xoverz,priv->allshares,priv->myshares,dp->hand.sharenrs,dp->M,(void *)buf,dp->numcards,dp->N);
                                }
                                else if ( strcmp(cmdstr,"final") == 0 )
                                {
                                    memcpy(dp->hand.final,buf,sizeof(*dp->hand.final) * dp->N * dp->numcards);
                                    if ( hn->client->H.slot == dp->N-1 )
                                        memcpy(priv->incards,buf,sizeof(*priv->incards) * dp->N * dp->numcards);
                                    //printf("player.%d got final crc.%04x %llx\n",hn->client->H.slot,_crc32(0,buf,len),(long long)dp->final[1].txid);
                                }
                                else if ( strcmp(cmdstr,"decode") == 0 )
                                {
                                    if ( (card= cards777_checkcard(&cardpriv,cardi,hn->client->H.slot,destplayer,hn->client->H.privkey,dp->hand.cardpubs,dp->numcards,*(bits256 *)buf)) >= 0 )
                                        printf("ERROR: player.%d got card.[%d]\n",hn->client->H.slot,card);
                                    memcpy(&priv->incards[cardi*dp->N + destplayer],buf,sizeof(bits256));
                                }
                                else if ( strcmp(cmdstr,"card") == 0 )
                                {
                                    if ( (card= cards777_checkcard(&cardpriv,cardi,hn->client->H.slot,destplayer,hn->client->H.privkey,dp->hand.cardpubs,dp->numcards,*(bits256 *)buf)) >= 0 )
                                    {
                                        //printf("player.%d got card.[%d]\n",hn->client->H.slot,card);
                                        memcpy(&priv->incards[cardi*dp->N + destplayer],cardpriv.bytes,sizeof(bits256));
                                    }
                                    else printf("ERROR player.%d got no card\n",hn->client->H.slot);
                                }
                                else if ( strcmp(cmdstr,"facedown") == 0 )
                                {
                                    //printf("player.%d sees that destplayer.%d got card\n",hn->client->H.slot,destplayer);
                                }
                                else if ( strcmp(cmdstr,"faceup") == 0 )
                                {
                                    if ( revealed < 0 || revealed != buf[1] )
                                        printf(">>>>>>>>>>>>>>> ERROR ");
                                    //printf("player.%d was REVEALED.[%d] (%s) cardi.%d\n",hn->client->H.slot,buf[1],hexstr,cardi);
                                    dp->hand.community[cardi - 2*dp->N] = buf[1];
                                }
                                else if ( strcmp(cmdstr,"showdown") == 0 )
                                {
                                    if ( (handstr= jstr(json,"hand")) != 0 )
                                    {
                                        rank = set_handstr(tmp,buf,0);
                                        if ( strcmp(handstr,tmp) != 0 || rank != juint(json,"rank") )
                                            printf("checkhand.(%s) != (%s) || rank.%u != %u\n",tmp,handstr,rank,juint(json,"rank"));
                                        else
                                        {
                                            //printf("sender.%d (%s) (%d %d)\n",senderslot,handstr,buf[5],buf[6]);
                                            dp->hand.handranks[senderslot] = rank;
                                            memcpy(dp->hand.hands[senderslot],buf,7);
                                            dp->hand.handmask |= (1 << senderslot);
                                            if ( dp->hand.handmask == (1 << dp->N)-1 )
                                            {
                                                bestj = 0;
                                                bestrank = dp->hand.handranks[0];
                                                for (j=1; j<dp->N; j++)
                                                    if ( dp->hand.handranks[j] > bestrank )
                                                    {
                                                        bestrank = dp->hand.handranks[j];
                                                        bestj = j;
                                                    }
                                                rank = set_handstr(tmp,dp->hand.hands[bestj],0);
                                                if ( rank == bestrank )
                                                {
                                                    for (j=0; j<dp->N; j++)
                                                    {
                                                        rank = set_handstr(tmp,dp->hand.hands[j],0);
                                                        if ( tmp[strlen(tmp)-1] == ' ' )
                                                            tmp[strlen(tmp)-1] = 0;
                                                        printf("%14s|",tmp[0]!=' '?tmp:tmp+1);
                                                        //printf("(%2d %2d).%d ",dp->hands[j][5],dp->hands[j][6],(int32_t)dp->balances[j]);
                                                    }
                                                    rank = set_handstr(tmp,dp->hand.hands[bestj],0);
                                                    dp->balances[bestj] += 3;
                                                    printf("->P%d $%-5lld %s N%d p%d $%d\n",bestj,(long long)dp->balances[bestj],tmp,dp->numhands,hn->client->H.slot,(int32_t)dp->balances[hn->client->H.slot]);
                                                } else printf("bestrank.%u mismatch %u\n",bestrank,rank);
                                            }
                                            //printf("player.%d got rank %u (%s) from %d\n",hn->client->H.slot,rank,handstr,senderslot);
                                        }
                                    }
                                }
                            }
                            retval = 0;
                        }
                    } else printf("NXT.%llu data mismatch %u vs %u len.%d\n",(long long)acct777_nxt64bits(hn->client->H.pubkey),_crc32(0,data,len),_crc32(0,buf,len),len);
                } else printf("NXT.%llu invalid hexstr.%p %ld %d\n",(long long)acct777_nxt64bits(hn->client->H.pubkey),jsonstr,hexstr!=0?strlen(hexstr):0,len);
                free_json(json);
            } else printf("NXT.%llu cant parse.(%s)\n",(long long)acct777_nxt64bits(hn->client->H.pubkey),jsonstr);
            free_queueitem(jsonstr);
            break;
        }
        usleep(1);
    }
    if ( i == maxmicro )
        printf("NXT.%llu timeout.%d\n",(long long)acct777_nxt64bits(hn->client->H.pubkey),i);
    else
    {
        static uint64_t sum,count,max;
        sum += (i+1);
        count++;
        if ( i > max )
            max = i;
        if ( (count % 10000) == 9999 )
            printf("us.%-6d completed | ave %.1f %llu max.%llu errs.%d\n",i,(double)sum/count,(long long)count,(long long)max,errs);
    }
    if ( retval != 0 )
        errs++;
    return(retval);
}

int32_t hostnet777_testresult(struct hostnet777_server *srv,struct hostnet777_client **clients,int32_t numclients,union hostnet777 *src,union hostnet777 *dest,int32_t blind,uint8_t *data,int32_t len,void *buf,int32_t revealed)
{
    uint64_t senderbits; uint32_t timestamp,maxmicro = 100000; int32_t i,n,retval = -1;
    if ( dest != 0 && dest->client != 0 )
    {
        //printf("PM call block on %d %llu\n",dest->client->H.slot,(long long)dest->client->H.nxt64bits);
        if ( hostnet777_block(srv,&senderbits,&timestamp,dest,data,len,buf,maxmicro,blind,revealed) == 0 )
            retval = 0;
    }
    else if ( dest == 0 || dest->client == 0 )
    {
        for (i=n=0; i<numclients; i++)
        {
            if ( i == 0 )
                dest->server = srv;
            else dest->client = clients[i];
            //printf("broadcast call block on %d %llu\n",i,(long long)dest->client->H.nxt64bits);
            if ( hostnet777_block(srv,&senderbits,&timestamp,dest,data,len,buf,maxmicro,blind,revealed) == 0 )
                n++;//, printf("verified.%d\n",i);
        }
        if ( n == numclients )
            retval = 0;
    }
    if ( retval != 0 )
    {
        for (i=1; i<numclients; i++)
            printf("%llu ",(long long)clients[i]->H.nxt64bits);
        printf("<<<<<<<<<<<<<<< srv.%llu ERROR\n\n",(long long)srv->H.nxt64bits);
    }
    //else
    //    printf("<<<<<<<<<<<<<<< PASS\n\n");
    return(retval);
}

int32_t hostnet777_testiter(struct hostnet777_server *srv,struct hostnet777_client **clients,int32_t numclients,int32_t mode,int32_t iter)
{
    int32_t s,d,blindflag,len,n,i,j,k,hexlen,cardi,destplayer,revealed; uint32_t rank; cJSON *json; union hostnet777 src,dest; uint64_t srcbits;
    char *cmdstr,hex[32768+128],pubstr[52*9*64+1],nrs[512],handstr[128]; uint8_t data[16384]; struct cards777_privdata *priv; struct cards777_pubdata *dp; bits256 destpub,card;
    revealed = -1;
    rank = pubstr[0] = nrs[0] = handstr[0] = 0;
    if ( mode == 0 )
    {
        cmdstr = "test";
        cardi = destplayer = -1;
        if ( (s= (rand() % numclients)) == 0 )
            src.server = srv;
        else src.client = clients[s];
        i = s;
        srcbits = src.client->H.nxt64bits;
        if ( (d= (rand() % (numclients+1))) == 0 )
            dest.server = srv, destpub = srv->H.pubkey;
        else if ( d < numclients )
            dest.client = clients[d], destpub = clients[d]->H.pubkey;
        else dest.client = 0;
        if ( (blindflag = ((rand() & 256) != 0)) != 0 )
            srcbits = 0;
        len = (rand() % (sizeof(data)-10)) + 10;
        randombytes(data,len);
    }
    else
    {
        blindflag = 0;
        cardi = destplayer = -1;
        if ( (i= iter) < numclients )
        {
            dp = srv->clients[i].pubdata;
            priv = srv->clients[i].privdata;
            if ( i < numclients-1 )
            {
                if ( iter == 0 )
                {
                    memset(dp->hand.sharenrs,0,sizeof(dp->hand.sharenrs));
                    init_sharenrs(dp->hand.sharenrs,0,dp->N,dp->N);
                    dp->hand.checkprod = cards777_initdeck(priv->outcards,dp->hand.cardpubs,dp->numcards,dp->N,dp->playerpubs,0);
                    init_hexbytes_noT(nrs,dp->hand.sharenrs,dp->N);
                    cmdstr = "pubstr";
                    srcbits = srv->H.nxt64bits;
                    len = dp->numcards*sizeof(bits256);
                    sprintf(hex,"{\"cmd\":\"%s\",\"cardi\":%d,\"dest\":%d,\"sender\":\"%llu\",\"timestamp\":\"%lu\",\"sharenrs\":\"%s\",\"n\":%u,\"data\":\"",cmdstr,cardi,destplayer,(long long)srcbits,time(NULL),nrs,len);
                    n = (int32_t)strlen(hex);
                    memcpy(data,dp->hand.cardpubs,len);
                    init_hexbytes_noT(&hex[n],data,len);
                    strcat(hex,"\"}");
                    hexlen = (int32_t)strlen(hex)+1;
                    dest.client = 0, memset(destpub.bytes,0,sizeof(destpub));
                    src.server = srv;
                    hostnet777_msg(0,destpub,&src,blindflag,hex,hexlen);
                    hostnet777_testresult(srv,clients,numclients,&src,&dest,blindflag,data,len,hex,revealed);
                }
                j = i+1, cmdstr = "encode";
            }
            else j = -1, cmdstr = "final";
            len = sizeof(bits256) * dp->N * dp->numcards;
            memcpy(data,priv->outcards,len);
        }
        else
        {
            cardi = (iter / numclients) - 1;
            dp = srv->clients[0].pubdata;
            destplayer = ((cardi + dp->button) % numclients);
            if ( cardi < numclients*2 + 5 )
            {
                i = (numclients - 1) - (iter % numclients);
                if ( i > 1 )
                    j = i - 1, cmdstr = "decode";
                else if ( i == 1 )
                    j = destplayer, cmdstr = "card";
                else //if ( i == 0 )
                {
                    j = -1;
                    i = destplayer;
                    if ( cardi < numclients*2 )
                        cmdstr = "facedown";
                    else cmdstr = "faceup";
                }
            }
            else
            {
                j = -1;
                i = (iter % numclients);
                cmdstr = "showdown";
            }
            dp = srv->clients[i].pubdata;
            priv = srv->clients[i].privdata;
            if ( strcmp(cmdstr,"showdown") == 0 )
            {
                len = 7;
                for (k=0; k<5; k++)
                    data[k] = dp->hand.community[k];
                data[k++] = priv->hole[0];
                data[k++] = priv->hole[1];
                rank = set_handstr(handstr,data,0);
            }
            else
            {
                card = priv->incards[cardi*numclients + destplayer];
                if ( j >= 0 )
                    card = cards777_decode(priv->xoverz,destplayer,card,priv->outcards,dp->numcards,numclients);
                else
                {
                    if ( strcmp(cmdstr,"facedown") == 0 )
                    {
                        priv->hole[cardi / numclients] = card.bytes[1];
                        priv->holecards[cardi / numclients] = card;
                        memset(card.bytes,0,sizeof(card));
                    }
                    else
                    {
                        revealed = card.bytes[1];
                        //printf("cmd.%s player.%d %llx (cardi.%d destplayer.%d) card.[%d]\n",cmdstr,i,(long long)card.txid,cardi,destplayer,card.bytes[1]);
                    }
                }
                len = sizeof(bits256);
                memcpy(data,card.bytes,len);
            }
        }
        //printf("iter.%d i.%d cardi.%d destplayer.%d j.%d\n",iter,i,cardi,destplayer,j);
        if ( i == 0 )
            src.server = srv;
        else src.client = clients[i];
        dp = srv->clients[i].pubdata;
        priv = srv->clients[i].privdata;
        if ( j < 0 )
            dest.client = 0;
        else if ( j == 0 )
            dest.server = srv, destpub = srv->H.pubkey;
        else dest.client = clients[j], destpub = clients[j]->H.pubkey;
        srcbits = src.client->H.nxt64bits;
    }
    sprintf(hex,"{\"cmd\":\"%s\",\"myslot\":%d,\"hand\":\"%s\",\"rank\":%u,\"cardi\":%d,\"dest\":%d,\"sender\":\"%llu\",\"timestamp\":\"%lu\",\"pubstr\":\"%s\",\"nrs\":\"%s\",\"n\":%u,\"data\":\"",cmdstr,i,handstr,rank,cardi,destplayer,(long long)srcbits,time(NULL),pubstr,nrs,len);
    n = (int32_t)strlen(hex);
    init_hexbytes_noT(&hex[n],data,len);
    strcat(hex,"\"}");
    //printf("HEX.[%s]\n",hex);
    if ( (json= cJSON_Parse(hex)) == 0 )
    {
        printf("error creating json\n");
        return(-1);
    }
    free_json(json);
    hexlen = (int32_t)strlen(hex)+1;
    hostnet777_msg(dest.client == 0 ? 0 : dest.client->H.nxt64bits,destpub,&src,blindflag,hex,hexlen);
    //printf("d.%d %p, s.%d %p len.%d blind.%d | dest.%p src.%p srv.%p\n",d,dest.client,s,src.client,len,blind,&dest,&src,srv);
    return(hostnet777_testresult(srv,clients,numclients,&src,&dest,blindflag,data,len,hex,revealed));
}

void hostnet777_test(int32_t numclients,int32_t numiters,int32_t mode)
{
    void *portable_thread_create(void *funcp,void *argp);
    int32_t i,slot,modval,errs = 0; union hostnet777 *hn; struct hostnet777_server *srv; bits256 srvpubkey,srvprivkey,pubkey,privkey;
    struct hostnet777_client **clients; uint32_t starttime;
    srvprivkey = curve25519_keypair(&srvpubkey);
    if ( (srv= hostnet777_server(srvprivkey,srvpubkey,0,0,0,numclients)) == 0 )
    {
        printf("cant create hostnet777 server\n");
        return;
    }
    hn = calloc(1,sizeof(*hn));
    hn->server = srv;
    if ( portable_thread_create((void *)hostnet777_idler,hn) == 0 )
        printf("error launching server thread\n");
    clients = calloc(numclients+1,sizeof(*clients));
    for (i=1; i<=numclients; i++) // generate one error
    {
        privkey = curve25519_keypair(&pubkey);
        if ( (slot= hostnet777_register(srv,pubkey,-1)) >= 0 )
        {
            if ( (clients[i]= hostnet777_client(privkey,pubkey,srv->ep.endpoint,slot)) == 0 )
                printf("error creating clients[%d]\n",i);
            else
            {
                hn = calloc(1,sizeof(*hn));
                hn->client = clients[i];
                printf("slot.%d client.%p -> hn.%p %llu pubkey.%llx\n",slot,clients[i],hn,(long long)clients[i]->H.nxt64bits,(long long)clients[i]->H.pubkey.txid);
                if ( portable_thread_create((void *)hostnet777_idler,hn) == 0 )
                    printf("error launching clients[%d] thread\n",i);
            }
        }
        else
        {
            printf("hostnet777_test: error creating client.%d\n",i);
            break;
        }
        //printf("iter.%d server.%p: %d %d\n",i,srv,srv->pullsock,srv->pubsock);
        //printf("client sendmsg.%d [%p] (%d %d %d)\n",clients[i]->H.slot,clients[i],clients[i]->pushsock,clients[i]->subsock,clients[i]->my.pmsock);
    }
    if ( mode != 0 )
        cards777_init(srv,numclients/2+1,clients,numclients,52);
    printf("srv.%p %llu M.%d N.%d\n",srv,(long long)srv->H.nxt64bits,numclients/2+1,numclients);
    if ( i >= numclients )
    {
        starttime = (uint32_t)time(NULL);
        modval = (numclients + numclients * (numclients*2 + 5 + 1));
        for (i=0; i<numiters; i++)
            errs += hostnet777_testiter(srv,clients,numclients,mode,i % modval);
        printf("hostnet777 numerrs %d of %d | %ld seconds, ave %.3f millis\n",errs,numiters,time(NULL) - starttime,1000. * (double)(time(NULL) - starttime)/numiters);
    }
    for (slot=1; slot<numclients; slot++)
    {
        if ( clients[slot] != 0 )
        {
            hostnet777_register(srv,clients[slot]->H.pubkey,-1);
            hostnet777_freeclient(clients[slot]);
        }
    }
    free(clients);
    hostnet777_freeserver(srv);
}

#endif
#endif
