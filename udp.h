//
//  NXTprivacy.h
//  gateway
//
//  Created by jl777 on 7/4/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef gateway_NXTprivacy_h
#define gateway_NXTprivacy_h


#include "includes/task.h"
#undef ARRAY_SIZE
#include <stdio.h>
#include <stdlib.h>


#define PEER_SENT 1
#define PEER_RECV 2
#define PEER_PENDINGRECV 4
#define PEER_MASK (PEER_SENT|PEER_RECV|PEER_PENDINGRECV)
#define PEER_TIMEOUT 0x40
#define PEER_FINISHED 0x80
#define PEER_EXPIRATION (60 * 1000.)


#define ALLOCWR_DONTFREE 0
#define ALLOCWR_ALLOCFREE 1
#define ALLOCWR_FREE -1

queue_t ALL_messages; //RPC_6777_response

struct pNXT_info
{
    //void **coinptrs;
    //char privacyServer_NXTaddr[64],privacyServer_ipaddr[64],privacyServer_port[16];
    // uint64_t privacyServer;
    struct hashtable **orderbook_txidsp,*msg_txids;
};
struct pNXT_info *Global_pNXT;

union writeU { uv_udp_send_t ureq; uv_write_t req; };

struct write_req_t
{
    union writeU U;
    struct sockaddr addr;
    uv_udp_t *udp;
    uv_buf_t buf;
    int32_t allocflag,queuetime;
};

struct udp_queuecmd
{
    cJSON *argjson;
    struct NXT_acct *tokenized_np;
    char *decoded;
    uint32_t previpbits,valid;
};

void update_nodestats_data(struct nodestats *stats)
{
    char NXTaddr[64],ipaddr[64];
    if ( stats->H.size == 0 )
        stats->H.size = sizeof(*stats);
    expand_ipbits(ipaddr,stats->ipbits);
    expand_nxt64bits(NXTaddr,stats->nxt64bits);
    printf("Update nodestats.%s (%s) lastcontact %u\n",NXTaddr,ipaddr,stats->lastcontact);
    update_storage(&SuperNET_dbs[NODESTATS_DATA],NXTaddr,&stats->H);
}

// helper and completion funcs
void portable_alloc(uv_handle_t *handle,size_t suggested_size,uv_buf_t *buf)
{
    buf->base = malloc(suggested_size);
    //printf("portable_alloc %p\n",buf->base);
    buf->len = suggested_size;
}

struct write_req_t *alloc_wr(void *buf,long len,int32_t allocflag)
{
    // nonzero allocflag means it will get freed on completion
    // negative allocflag means already allocated, dont allocate and copy
    void *ptr;
    struct write_req_t *wr;
    wr = calloc(1,sizeof(*wr));
    ASSERT(wr != NULL);
    wr->allocflag = allocflag;
    if ( allocflag == ALLOCWR_ALLOCFREE )
    {
        ptr = malloc(len);
        memcpy(ptr,buf,len);
        //printf("alloc_wr.%p\n",ptr);
    } else ptr = buf;
    wr->buf = uv_buf_init(ptr,(int32_t)len);
    return(wr);
}

void after_write(uv_write_t *req,int status)
{
    struct write_req_t *wr;
    // Free the read/write buffer and the request
    if ( status != 0 )
        printf("after write status.%d %s\n",status,uv_err_name(status));
    wr = (struct write_req_t *)req;
    if ( wr->allocflag != ALLOCWR_DONTFREE )
    {
        //printf("after write buf.base %p\n",wr->buf.base);
        free(wr->buf.base);
    }
    //printf("after write %p\n",wr);
    free(wr);
    if ( status == 0 )
        return;
    fprintf(stderr, "uv_write error: %d %s\n",status,uv_err_name(status));
}

int32_t process_sendQ_item(struct write_req_t *wr)
{
    char ipaddr[64];
    struct coin_info *cp = get_coin_info("BTCD");
    struct nodestats *stats;
    struct pserver_info *pserver;
    int32_t r,supernet_port,createdflag;
    {
        supernet_port = extract_nameport(ipaddr,sizeof(ipaddr),(struct sockaddr_in *)&wr->addr);
        pserver = get_pserver(&createdflag,ipaddr,0,0);
        if ( 1 && (pserver->nxt64bits == cp->privatebits || pserver->nxt64bits == cp->srvpubnxtbits) )
        {
            //printf("(%s/%d) no point to send yourself dest.%llu pub.%llu srvpub.%llu\n",ipaddr,supernet_port,(long long)pserver->nxt64bits,(long long)cp->pubnxtbits,(long long)cp->srvpubnxtbits);
            //return(0);
            strcpy(ipaddr,"127.0.0.1");
            uv_ip4_addr(ipaddr,supernet_port,(struct sockaddr_in *)&wr->addr);
        }
        if ( (stats= get_nodestats(pserver->nxt64bits)) != 0 )
        {
            stats->numsent++;
            stats->sentmilli = milliseconds();
        }
        //for (i=0; i<16; i++)
        //    printf("%02x ",((unsigned char *)buf)[i]);
        if ( Debuglevel > 1 )
            printf("uv_udp_send %ld bytes to %s/%d crx.%x\n",wr->buf.len,ipaddr,supernet_port,_crc32(0,wr->buf.base,wr->buf.len));
    }
    r = uv_udp_send(&wr->U.ureq,wr->udp,&wr->buf,1,&wr->addr,(uv_udp_send_cb)after_write);
    if ( r != 0 )
        printf("uv_udp_send error.%d %s wr.%p wreq.%p %p len.%ld\n",r,uv_err_name(r),wr,&wr->U.ureq,wr->buf.base,wr->buf.len);
    return(r);
}

int32_t portable_udpwrite(int32_t queueflag,const struct sockaddr *addr,uv_udp_t *handle,void *buf,long len,int32_t allocflag)
{
    int32_t r=0;
    struct write_req_t *wr;
    wr = alloc_wr(buf,len,allocflag);
    ASSERT(wr != NULL);
    wr->addr = *addr;
    wr->udp = handle;
    if ( queueflag != 0 )
    {
        wr->queuetime = (uint32_t)(1000. * milliseconds());
        queue_enqueue(&sendQ,wr);
    }
    else r = process_sendQ_item(wr);
    return(r);
}

void on_udprecv(uv_udp_t *udp,ssize_t nread,const uv_buf_t *rcvbuf,const struct sockaddr *addr,unsigned flags)
{
    uint16_t supernet_port;
    int32_t createdflag;
    struct nodestats *stats;
    struct pserver_info *pserver;
    struct NXT_acct *np;
    struct coin_info *cp = get_coin_info("BTCD");
    char ipaddr[256],retjsonstr[4096],srvNXTaddr[64];
    retjsonstr[0] = 0;
    if ( cp != 0 && nread > 0 )
    {
        supernet_port = extract_nameport(ipaddr,sizeof(ipaddr),(struct sockaddr_in *)addr);
        if ( notlocalip(ipaddr) == 0 )
            strcpy(ipaddr,cp->myipaddr);
        pserver = get_pserver(&createdflag,ipaddr,supernet_port,0);
        if ( (stats= get_nodestats(pserver->nxt64bits)) != 0 )
        {
            stats->numrecv++;
            stats->recvmilli = milliseconds();
        }
        {
            //int i;
            //for (i=0; i<16; i++)
            //    printf("%02x ",((unsigned char *)rcvbuf->base)[i]);
            if ( Debuglevel > 0 || (nread > 400 && nread != MAX_UDPLEN) )
                printf("UDP RECEIVED %ld from %s/%d crc.%x\n",nread,ipaddr,supernet_port,_crc32(0,rcvbuf->base,nread));
        }
        //expand_nxt64bits(NXTaddr,cp->pubnxtbits);
        expand_nxt64bits(srvNXTaddr,cp->srvpubnxtbits);
        np = process_packet(0,retjsonstr,(unsigned char *)rcvbuf->base,(int32_t)nread,udp,(struct sockaddr *)addr,ipaddr,supernet_port);
        ASSERT(addr->sa_family == AF_INET);
        server_xferred += nread;
    }
    if ( rcvbuf->base != 0 )
    {
        //printf("on_duprecv free.%p\n",rcvbuf->base);
        free(rcvbuf->base);
    }
}

uv_udp_t *open_udp(struct sockaddr *addr)
{
    int32_t r;
    uv_udp_t *udp;
    udp = malloc(sizeof(uv_udp_t));
    r = uv_udp_init(UV_loop,udp);
    if ( r != 0 )
    {
        fprintf(stderr, "uv_udp_init: %d %s\n",r,uv_err_name(r));
        return(0);
    }
    if ( addr != 0 )
    {
        r = uv_udp_bind(udp,addr,0);
        if ( r != 0 )
        {
            fprintf(stderr,"uv_udp_bind: %d %s\n",r,uv_err_name(r));
            return(0);
        }
    }
    r = uv_udp_set_broadcast(udp,1);
    if ( r != 0 )
    {
        fprintf(stderr,"uv_udp_set_broadcast: %d %s\n",r,uv_err_name(r));
        return(0);
    }
    r = uv_udp_recv_start(udp,portable_alloc,on_udprecv);
    if ( r != 0 )
    {
        fprintf(stderr, "uv_udp_recv_start: %d %s\n",r,uv_err_name(r));
        return(0);
    }
    return(udp);
}

void *start_libuv_udpserver(int32_t ip4_or_ip6,uint16_t port,void *handler)
{
    void *srv;
    const struct sockaddr *ptr;
    struct sockaddr_in addr;
    struct sockaddr_in6 addr6;
    if ( ip4_or_ip6 == 4 )
    {
        ASSERT(0 == uv_ip4_addr("0.0.0.0",port,&addr));
        ptr = (const struct sockaddr *)&addr;
    }
    else if ( ip4_or_ip6 == 6 )
    {
        ASSERT(0 == uv_ip6_addr("::1", port, &addr6));
        ptr = (const struct sockaddr *)&addr6;
    }
    else { printf("illegal ip4_or_ip6 %d\n",ip4_or_ip6); return(0); }
    srv = open_udp((port > 0) ? (struct sockaddr *)ptr : 0);
    if ( srv != 0 )
        printf("UDP.%p server started on port %d\n",srv,port);
    else printf("couldnt open_udp on port.%d\n",port);
    Servers_started |= 1;

    return(srv);
}

int32_t crcize(unsigned char *final,unsigned char *encoded,int32_t len)
{
    uint32_t i,n;//,crc = 0;
    uint32_t crc = _crc32(0,encoded,len);
    memcpy(final,&crc,sizeof(crc));
    memcpy(final + sizeof(crc),encoded,len);
    return(len + sizeof(crc));
    
    if ( len + sizeof(crc) < MAX_UDPLEN )
    {
        for (i=(uint32_t)(len + sizeof(crc)); i<MAX_UDPLEN; i++)
            final[i] = (rand() >> 8);
        n = MAX_UDPLEN - sizeof(crc);
    } else n = len;
    memcpy(final + sizeof(crc),encoded,len);
    crc = _crc32(0,final + sizeof(crc),n);
    memcpy(final,&crc,sizeof(crc));
    return(n + sizeof(crc));
}

int32_t is_encrypted_packet(unsigned char *tx,int32_t len)
{
    uint32_t crc,packet_crc;
    len -= sizeof(crc);
    if ( len <= 0 )
        return(0);
    memcpy(&packet_crc,tx,sizeof(packet_crc));
    tx += sizeof(crc);
    crc = _crc32(0,tx,len);
    //printf("got crc of %08x vx packet_crc %08x\n",crc,packet_crc);
    return(packet_crc == crc);
}

void send_packet(struct nodestats *peerstats,struct sockaddr *destaddr,unsigned char *finalbuf,int32_t len)
{
    char ipaddr[64];
    int32_t port,queueflag,p2pflag = 0;
    struct nodestats *stats;
    struct pserver_info *pserver = 0;
    if ( destaddr != 0 )
    {
        port = extract_nameport(ipaddr,sizeof(ipaddr),(struct sockaddr_in *)destaddr);
        pserver = get_pserver(0,ipaddr,0,0);
        if ( peerstats != 0 && len <= MAX_UDPLEN && Global_mp->udp != 0 )
        {
            if ( port == 0 || port == BTCD_PORT )
            {
                printf("send_packet WARNING: send_packet port.%d <- %d\n",port,SUPERNET_PORT);
                port = SUPERNET_PORT;
                uv_ip4_addr(ipaddr,port,(struct sockaddr_in *)destaddr);
            }
            queueflag = 1;
            if ( Debuglevel > 1 )
                printf("portable_udpwrite Q.%d %d to (%s:%d)\n",queueflag,len,ipaddr,port);
            portable_udpwrite(queueflag,destaddr,Global_mp->udp,finalbuf,len,ALLOCWR_ALLOCFREE);
        }
        else p2pflag = 1;
        if ( peerstats != 0 && peerstats->lastcontact < (time(NULL) - 600) )
            p2pflag = 1;
        if ( p2pflag != 0 )
            call_SuperNET_broadcast(pserver,(char *)finalbuf,len,0);
    }
    else if ( peerstats != 0 && peerstats->ipbits != 0 )
    {
        expand_ipbits(ipaddr,peerstats->ipbits);
        pserver = get_pserver(0,ipaddr,0,0);
        printf("send packet to (%s) %llu len.%d\n",ipaddr,(long long)peerstats->nxt64bits,len);
        call_SuperNET_broadcast(pserver,(char *)finalbuf,len,0);
    }
    else
    {
        printf("send_packet: cant resolve path for %p %p\n",peerstats,destaddr);
        return;
    }
    if ( peerstats != 0 )
    {
        if ( pserver != 0 && pserver->nxt64bits == 0 )
            pserver->nxt64bits = peerstats->nxt64bits;
        peerstats->numsent++;
        peerstats->sentmilli = milliseconds();
    }
    else if ( pserver != 0 )
    {
        if ( (stats= get_nodestats(pserver->nxt64bits)) != 0 )
        {
            stats->sentmilli++;
            stats->sentmilli = milliseconds();
        }
    }
}

void route_packet(int32_t encrypted,struct sockaddr *destaddr,char *hopNXTaddr,unsigned char *outbuf,int32_t len)
{
    unsigned char finalbuf[4096];
    char destip[64];
    struct sockaddr_in addr;
    int32_t port,createdflag;
    struct NXT_acct *np;
    struct nodestats *stats = 0;
    if ( len > sizeof(finalbuf) )
    {
        fprintf(stderr,"sendmessage: len.%d > sizeof(finalbuf) %ld\n",len,sizeof(finalbuf));
        return;
    }
    if ( hopNXTaddr != 0 && hopNXTaddr[0] != 0 )
    {
        np = get_NXTacct(&createdflag,Global_mp,hopNXTaddr);
        stats = &np->stats;
    }
    if ( encrypted != 0 )
    {
        memset(finalbuf,0,sizeof(finalbuf));
        len = crcize(finalbuf,outbuf,len);
        outbuf = finalbuf;
    }
    if ( destaddr != 0 )
    {
        port = extract_nameport(destip,sizeof(destip),(struct sockaddr_in *)destaddr);
        if ( Debuglevel > 0 )
            printf("DIRECT send encrypted.%d to (%s/%d) finalbuf.%d\n",encrypted,destip,port,len);
        send_packet(stats,destaddr,outbuf,len);
    }
    else if ( stats != 0 )
    {
        expand_ipbits(destip,stats->ipbits);
        if ( stats->ipbits != 0 )
        {
            if ( Debuglevel > 0 )
                printf("DIRECT udpsend {%s} to %s/%d finalbuf.%d\n",hopNXTaddr,destip,stats->supernet_port,len);
            uv_ip4_addr(destip,stats->supernet_port!=0?stats->supernet_port:SUPERNET_PORT,&addr);
            send_packet(stats,(struct sockaddr *)&addr,finalbuf,len);
        }
        else { printf("cant route packet.%d without IP address to %llu\n",len,(long long)stats->nxt64bits); return; }
    } else { printf("cant route packet.%d without nodestats\n",len); return; }
}

uint64_t directsend_packet(int32_t encrypted,struct pserver_info *pserver,char *origargstr,int32_t len,unsigned char *data,int32_t datalen)
{
    static unsigned char zeropubkey[crypto_box_PUBLICKEYBYTES];
    uint64_t txid = 0;
    int32_t port,L;
    struct sockaddr destaddr;
    struct nodestats *stats = 0;
    struct coin_info *cp = get_coin_info("BTCD");
    unsigned char *outbuf;
    if ( pserver->nxt64bits == 0 || (stats= get_nodestats(pserver->nxt64bits)) == 0 || stats->ipbits == 0 )
    {
        printf("no nxtaddr.%llu or null stats.%p ipbits.%x\n",(long long)pserver->nxt64bits,stats,stats!=0?stats->ipbits:0);
        return(0);
    }
    if ( pserver->nxt64bits != 0 )
        stats = get_nodestats(pserver->nxt64bits);
    if ( stats != 0 )
        port = stats->supernet_port != 0 ? stats->supernet_port : SUPERNET_PORT;
    else port = SUPERNET_PORT;
    uv_ip4_addr(pserver->ipaddr,port,(struct sockaddr_in *)&destaddr);
    stripwhite_ns(origargstr,len);
    len = (int32_t)strlen(origargstr)+1;
    txid = calc_txid((uint8_t *)origargstr,len);
    if ( encrypted != 0 && stats != 0 && memcmp(zeropubkey,stats->pubkey,sizeof(zeropubkey)) != 0 )
    {
        char *sendmessage(char *hopNXTaddr,int32_t L,char *verifiedNXTaddr,char *msg,int32_t msglen,char *destNXTaddr,unsigned char *data,int32_t datalen);
        char hopNXTaddr[64],destNXTaddr[64],*retstr;
        expand_nxt64bits(destNXTaddr,stats->nxt64bits);
        L = (encrypted>1 ? MAX(encrypted,Global_mp->Lfactor) : 0);
        if ( Debuglevel > 2 )
            fprintf(stderr,"direct send via sendmessage (%s) %p %d\n",origargstr,data,datalen);
        retstr = sendmessage(hopNXTaddr,L,cp->srvNXTADDR,origargstr,len,destNXTaddr,data,datalen);
        if ( retstr != 0 )
        {
            if ( Debuglevel > 2 )
                fprintf(stderr,"direct send via sendmessage got (%s)\n",retstr);
            free(retstr);
        }
    }
    else if ( origargstr != 0 )
    {
        encrypted = 0;
        outbuf = (unsigned char *)origargstr;
        if ( data != 0 && datalen > 0 )
        {
            memcpy(outbuf+len,data,datalen);
            len += datalen;
        }
        //init_jsoncodec((char *)outbuf,len);
        //printf("directsend to %llu (%s).%d stats.%p\n",(long long)pserver->nxt64bits,pserver->ipaddr,port,stats);
        if ( len > MAX_UDPLEN-56 )
            printf("directsend_packet: payload too big %d\n",len);
        else if ( len > 0 )
        {
            if ( Debuglevel > 0 )
                fprintf(stderr,"route_packet encrypted.%d\n",encrypted);
            route_packet(encrypted,&destaddr,0,outbuf,len);
            //printf("got route_packet txid.%llu\n",(long long)txid);
        }
        else printf("directsend_packet: illegal len.%d\n",len);
    }
    return(txid);
}

uint64_t p2p_publishpacket(struct pserver_info *pserver,char *cmd)
{
    char _cmd[MAX_JSON_FIELD*4],packet[MAX_JSON_FIELD*4];
    int32_t len,createdflag;
    struct NXT_acct *np;
    struct coin_info *cp = get_coin_info("BTCD");
    if ( cp != 0 )
    {
        if ( Debuglevel > 1 )
            fprintf(stderr,"p2p_publishpacket.%p (%s)\n",pserver,cmd);
        np = get_NXTacct(&createdflag,Global_mp,cp->srvNXTADDR);
        if ( cmd == 0 )
        {
            int32_t gen_pingstr(char *cmdstr,int32_t completeflag);
            gen_pingstr(_cmd,1);
            //sprintf(_cmd,"{\"requestType\":\"%s\",\"NXT\":\"%s\",\"time\":%ld,\"pubkey\":\"%s\",\"ipaddr\":\"%s\"}","ping",cp->srvNXTADDR,(long)time(NULL),Global_mp->pubkeystr,cp->myipaddr);
        }
        else strcpy(_cmd,cmd);
        //printf("_cmd.(%s)\n",_cmd);
        len = construct_tokenized_req(packet,_cmd,cp->srvNXTACCTSECRET);
        if ( Debuglevel > 1 )
            printf("len.%d (%s)\n",len,packet);
        return(call_SuperNET_broadcast(pserver,packet,len,PUBADDRS_MSGDURATION));
    }
    printf("ERROR: broadcast_publishpacket null cp\n");
    return(0);
}

struct transfer_args *create_transfer_args(char *previpaddr,char *sender,char *dest,char *name,uint32_t totallen,uint32_t blocksize,uint32_t totalcrc)
{
    uint64_t txid;
    char hashstr[4096];
    struct transfer_args *args;
    int32_t createdflag;
    sprintf(hashstr,"%s.%s.%u.%u.%u",sender,name,totallen,totalcrc,blocksize);
    txid = calc_txid((uint8_t *)hashstr,(int32_t)strlen(hashstr));
    //printf("hashstr.(%s) -> %llx | data.%p\n",hashstr,(long long)txid,data);
    sprintf(hashstr,"%llx",(long long)txid);
    args = MTadd_hashtable(&createdflag,Global_mp->pending_xfers,hashstr);
    if ( createdflag != 0 )
    {
        safecopy(args->previpaddr,previpaddr,sizeof(args->previpaddr));
        safecopy(args->sender,sender,sizeof(args->sender));
        safecopy(args->dest,dest,sizeof(args->dest));
        safecopy(args->name,name,sizeof(args->name));
        args->totallen = totallen;
        args->blocksize = blocksize;
        args->totalcrc = totalcrc;
        args->numblocks = (totallen / blocksize);
        if ( (totallen % blocksize) != 0 )
            args->numblocks++;
    }
    if ( args->timestamps == 0 )
        args->timestamps = calloc(args->numblocks,sizeof(*args->timestamps));
    if ( args->crcs == 0 )
        args->crcs = calloc(args->numblocks,sizeof(*args->crcs));
    if ( args->ackcrcs == 0 )
        args->ackcrcs = calloc(args->numblocks,sizeof(*args->ackcrcs));
    if ( args->data == 0 )
        args->data = calloc(1,totallen);
    return(args);
}

int32_t update_transfer_args(struct transfer_args *args,uint32_t fragi,uint32_t numfrags,uint32_t totalcrc,uint32_t datacrc,uint8_t *data,int32_t datalen)
{
    int i,count = 0;
    uint32_t checkcrc;
    if ( data == 0 ) // sender
    {
        if ( fragi < args->numblocks )
            args->ackcrcs[fragi] = datacrc;
        for (i=0; i<args->numblocks; i++)
        {
            if ( args->crcs[i] == args->ackcrcs[i] )
                count++;
        }
        if ( Debuglevel > 2 )
            printf(">>>>>>>>> set ack[%d] <- %u | count.%d of %d | %p\n",fragi,datacrc,count,args->numblocks,args);
    }
    else // recipient
    {
        if ( fragi < args->numblocks )
            args->crcs[fragi] = datacrc;
        for (i=0; i<args->numblocks; i++)
        {
            if ( args->crcs[i] != 0 )
                count++;
        }
        if ( count == args->numblocks )
        {
            checkcrc = _crc32(0,args->data,args->totallen);
            if ( checkcrc != args->totalcrc )
                printf("totalcrc ERROR %u != %u\n",checkcrc,args->totalcrc);
        }
    }
    return(count);
}

void purge_transfer_args(struct transfer_args *args)
{
    args->completed = 1;
    if ( args->data != 0 )
        free(args->data), args->data = 0;
    if ( args->crcs != 0 )
        free(args->crcs), args->crcs = 0;
    if ( args->ackcrcs != 0 )
        free(args->ackcrcs), args->ackcrcs = 0;
    if ( args->timestamps != 0 )
        free(args->timestamps), args->timestamps = 0;
}

char *sendfrag(char *previpaddr,char *sender,char *verifiedNXTaddr,char *NXTACCTSECRET,char *dest,char *name,uint32_t fragi,uint32_t numfrags,uint32_t totallen,uint32_t blocksize,uint32_t totalcrc,uint32_t checkcrc,char *datastr)
{
    char cmdstr[4096],_tokbuf[4096];
    struct pserver_info *pserver;
    struct transfer_args *args;
    struct coin_info *cp = get_coin_info("BTCD");
    int32_t len,count=0,datalen = 0;
    unsigned char *data = 0;
    uint32_t datacrc;
    uint64_t txid;
    if ( cp == 0 || datastr == 0 || datastr[0] == 0 )
    {
        printf("sendfrag err cp.%p datastr.%p\n",cp,datastr);
        return(clonestr("{\"error\":\"no BTCD coin info\"}"));
    }
    datalen = (int32_t)strlen(datastr)/2;
    data = malloc(datalen);
    datalen = decode_hex(data,datalen,datastr);
    datacrc = _crc32(0,data,datalen);
    sprintf(cmdstr,"{\"NXT\":\"%s\",\"pubkey\":\"%s\",\"ipaddr\":\"%s\",\"name\":\"%s\",\"time\":%ld,\"fragi\":%u,\"numfrags\":%u,\"totallen\":%u,\"blocksize\":%u,\"totalcrc\":%u,\"datacrc\":%u",verifiedNXTaddr,Global_mp->pubkeystr,cp->myipaddr,name,(long)time(NULL),fragi,numfrags,totallen,blocksize,totalcrc,datacrc);
    if ( previpaddr == 0 || previpaddr[0] == 0 )
    {
        pserver = get_pserver(0,dest,0,0);
        sprintf(cmdstr+strlen(cmdstr),",\"requestType\":\"sendfrag\",\"data\":%d}",datalen);
    }
    else
    {
        pserver = get_pserver(0,previpaddr,0,0);
        if ( checkcrc != datacrc )
            strcat(cmdstr,",\"error\":\"crcerror\"");
        else
        {
            args = create_transfer_args(previpaddr,sender,dest,name,totallen,blocksize,totalcrc);
            memcpy(args->data + fragi*args->blocksize,data,datalen);
            if ( (count= update_transfer_args(args,fragi,numfrags,totalcrc,datacrc,data,datalen)) == args->numblocks )
            {
                checkcrc = _crc32(0,args->data,args->totallen);
                printf("completed.%d (%s) totallen.%d to (%s) checkcrc.%u vs totalcrc.%u\n",count,args->name,args->totallen,dest,checkcrc,totalcrc);
                if ( checkcrc == args->totalcrc )
                {
                    FILE *fp;
                    char buf[512];
                    sprintf(buf,"archive/%s",args->name);
                    if ( (fp= fopen(buf,"wb")) != 0 )
                    {
                        fwrite(args->data,1,args->totallen,fp);
                        fclose(fp);
                    }
                }
                args->completed = 1;
                purge_transfer_args(args);
            }
        }
        free(data);
        data = 0;
        sprintf(cmdstr+strlen(cmdstr),",\"requestType\":\"gotfrag\",\"count\":\"%d\",\"checkcrc\":%u}",count,checkcrc);
    }
    len = construct_tokenized_req(_tokbuf,cmdstr,NXTACCTSECRET);
    txid = directsend_packet(1,pserver,_tokbuf,len,data,datalen);
    if ( Debuglevel > 2 )
        printf("send back (%s) len.%d datalen.%d\n",cmdstr,len,datalen);
    if ( data != 0 )
        free(data);
    return(clonestr(_tokbuf));
}

int32_t Do_transfers(void *_args,int32_t argsize)
{
    struct transfer_args *args = *(struct transfer_args **)_args;
    char datastr[4096],*retstr;
    struct coin_info *cp = get_coin_info("BTCD");
    int32_t i,remains,num,retval = -1,finished = 0;
    uint32_t now = (uint32_t)time(NULL);
    //printf("Do_transfers.args.%p\n",args);
    if ( cp != 0 )
    {
        retval = 0;
        num = 0;
        remains = args->totallen;
        for (i=0; i<args->numblocks; i++)
        {
            if ( Debuglevel > 1 )
                printf("crc.(%u vs %u) ",args->ackcrcs[i],args->crcs[i]);
            if ( args->ackcrcs[i] != args->crcs[i] )
            {
                if ( (now - args->timestamps[i]) > 1 )
                {
                    init_hexbytes_noT(datastr,args->data + i*args->blocksize,(remains < args->blocksize) ? remains : args->blocksize);
                    retstr = sendfrag(0,cp->srvNXTADDR,cp->srvNXTADDR,cp->srvNXTACCTSECRET,args->dest,args->name,i,args->numblocks,args->totallen,args->blocksize,args->totalcrc,args->crcs[i],datastr);
                    num++;
                    if ( retstr != 0 )
                        free(retstr);
                    args->timestamps[i] = now;
                }
            } else finished++;
            remains -= args->blocksize;
            //if ( num > 16 )
                break;
        }
        if ( finished == args->numblocks )
            retval = -1;
        if ( Debuglevel > 1 )
            printf("finished %d of %d len.%d | %s %u to %s\n",finished,args->numblocks,args->totallen,args->name,args->totalcrc,args->dest);
    }
    if ( args->timeout != 0 && now > args->timeout )
    {
        printf("TIMEOUT %u for %s %u len.%d to %s\n",args->timeout,args->name,args->totalcrc,args->totallen,args->dest);
        args->completed = 1;
        retval = -1;
    }
    if ( retval < 0 )
    {
        args->completed = 1;
        purge_transfer_args(args);
    }
    return(retval);
}

char *gotfrag(char *previpaddr,char *sender,char *NXTaddr,char *NXTACCTSECRET,char *src,char *name,uint32_t fragi,uint32_t numfrags,uint32_t totallen,uint32_t blocksize,uint32_t totalcrc,uint32_t datacrc,int32_t count)
{
    struct transfer_args *args;
    char cmdstr[4096];
    if ( blocksize == 0 )
        blocksize = 512;
    if ( totallen == 0 )
        totallen = numfrags * blocksize;
    sprintf(cmdstr,"{\"requestType\":\"gotfrag\",\"sender\":\"%s\",\"src\":\"%s\",\"fragi\":%u,\"numfrags\":%u,\"totallen\":%u,\"blocksize\":%u,\"totalcrc\":%u,\"datacrc\":%u,\"count\":%d}",sender,src,fragi,numfrags,totallen,blocksize,totalcrc,datacrc,count);
    args = create_transfer_args(previpaddr,NXTaddr,src,name,totallen,blocksize,totalcrc);
    update_transfer_args(args,fragi,numfrags,totalcrc,datacrc,0,0);
    return(clonestr(cmdstr));
}

char *start_transfer(char *previpaddr,char *sender,char *verifiedNXTaddr,char *NXTACCTSECRET,char *dest,char *name,uint8_t *data,int32_t totallen,int32_t timeout)
{
    static char *buf;
    static int64_t allocsize=0;
    struct transfer_args *args;
    int64_t len;
    int32_t i,remains,totalcrc,blocksize = 512;
    if ( data == 0 || totallen == 0 )
    {
        data = (uint8_t *)load_file(name,&buf,&len,&allocsize);
        totallen = (int32_t)len;
    }
    if ( data != 0 && totallen != 0 )
    {
        totalcrc = _crc32(0,data,totallen);
        args = create_transfer_args(previpaddr,verifiedNXTaddr,dest,name,totallen,blocksize,totalcrc);
        if ( timeout != 0 )
            args->timeout = (uint32_t)time(NULL) + timeout;
        printf("start_transfer.args.%p (%s) timeout.%u\n",args,verifiedNXTaddr,args->timeout);
        memcpy(args->data,data,totallen);
        data = args->data;
        remains = totallen;
        for (i=0; i<args->numblocks; i++)
        {
            args->crcs[i] = _crc32(0,data + i*blocksize,(remains < blocksize) ? remains : blocksize);
            printf("CRC[%d] <- %u offset %d len.%d\n",i,args->crcs[i],i*blocksize,(remains < blocksize) ? remains : blocksize);
            remains -= blocksize;
        }
        start_task(Do_transfers,"transfer",100000,(void *)&args,sizeof(args));
        return(clonestr("{\"result\":\"start_transfer pending\"}"));
    } else return(clonestr("{\"error\":\"start_transfer: cant start_transfer\"}"));
}

int32_t update_nodestats(char *NXTaddr,uint32_t now,struct nodestats *stats,int32_t encryptedflag,int32_t p2pflag,unsigned char pubkey[crypto_box_PUBLICKEYBYTES],char *ipaddr,int32_t port)
{
    static unsigned char zeropubkey[crypto_box_PUBLICKEYBYTES];
    int32_t modified = 0;
    uint64_t nxt64bits = calc_nxt64bits(NXTaddr);
    if ( stats->ipbits == 0 )
        stats->ipbits = calc_ipbits(ipaddr);
    if ( encryptedflag != 0 )
    {
        stats->modified &= ~(1 << p2pflag);
        stats->gotencrypted = 1;
    }
    if ( p2pflag != 0 )
    {
        stats->lastcontact = now;
        stats->BTCD_p2p = 1;
    }
    stats->numrecv++;
    if ( stats->nxt64bits == 0 )
        stats->nxt64bits = nxt64bits;
    else if ( stats->nxt64bits != nxt64bits )
    {
        printf("WARNING: update_nodestats %llu != %llu\n",(long long)stats->nxt64bits,(long long)nxt64bits);
        stats->nxt64bits = nxt64bits;
    }
    if ( pubkey != 0 && memcmp(zeropubkey,pubkey,sizeof(zeropubkey)) != 0 )
    {
        if ( memcmp(stats->pubkey,pubkey,sizeof(stats->pubkey)) != 0 )
        {
            char ipaddr[64];
            expand_ipbits(ipaddr,stats->ipbits);
            printf("pubkey.%llu updated from pserver %s/%d:%d | recv.%d send.%d\n",(long long)stats->nxt64bits,ipaddr,stats->supernet_port,stats->p2pport,stats->numrecv,stats->numsent);
            memcpy(stats->pubkey,pubkey,sizeof(stats->pubkey));
            modified = (1 << p2pflag);
            stats->modified |= modified;
            stats->lastcontact = now;
            update_nodestats_data(stats);
        }
    }
    if ( memcmp(zeropubkey,stats->pubkey,sizeof(zeropubkey)) != 0 )
    {
        void update_Kbuckets(struct nodestats *stats,uint64_t,char *,int32_t,int32_t,int32_t lastcontact);
        update_Kbuckets(stats,nxt64bits,ipaddr,port,p2pflag,now);
    }
    return(modified);
}

int32_t update_routing_probs(char *NXTaddr,int32_t encryptedflag,int32_t p2pflag,struct nodestats *stats,char *ipaddr,int32_t port,unsigned char pubkey[crypto_box_PUBLICKEYBYTES])
{
    uint32_t now,modified = 0;
    struct pserver_info *pserver;
    now = (uint32_t)time(NULL);
    if ( p2pflag != 0 )
        pserver = get_pserver(0,ipaddr,0,port);
    else pserver = get_pserver(0,ipaddr,port,0);
    if ( stats != 0 )
        modified = (update_nodestats(NXTaddr,now,stats,encryptedflag,p2pflag,pubkey,ipaddr,port) << 1);
    return(modified);
}

int32_t on_SuperNET_whitelist(char *ipaddr)
{
    uint32_t i,ipbits;
    if ( Num_in_whitelist > 0 && SuperNET_whitelist != 0 )
    {
        ipbits = calc_ipbits(ipaddr);
        for (i=0; i<Num_in_whitelist; i++)
            if ( SuperNET_whitelist[i] == ipbits )
                return(1);
    }
    return(0);
}

int32_t add_SuperNET_whitelist(char *ipaddr)
{
    if ( on_SuperNET_whitelist(ipaddr) == 0 )
    {
        SuperNET_whitelist = realloc(SuperNET_whitelist,sizeof(*SuperNET_whitelist) * (Num_in_whitelist + 1));
        SuperNET_whitelist[Num_in_whitelist] = calc_ipbits(ipaddr);
        printf(">>>>>>>>>>>>>>>>>> add to SuperNET_whitelist[%d] (%s)\n",Num_in_whitelist,ipaddr);
        Num_in_whitelist++;
        return(1);
    }
    return(0);
}

void add_SuperNET_peer(char *ip_port)
{
    struct pserver_info *pserver;
    int32_t createdflag,p2pport;
    char ipaddr[64];
    p2pport = parse_ipaddr(ipaddr,ip_port);
    if ( p2pport == 0 )
        p2pport = BTCD_PORT;
    pserver = get_pserver(&createdflag,ipaddr,0,p2pport);
    //pserver->S.BTCD_p2p = 1;
    //if ( on_SuperNET_whitelist(ipaddr) != 0 )
    {
        printf("got_newpeer called. Now connected to.(%s) [%s/%d]\n",ip_port,ipaddr,p2pport);
        p2p_publishpacket(pserver,0);
    }
}

void every_second(int32_t counter)
{
    static double firstmilli;
    char *ip_port;
    if ( Finished_init == 0 )
        return;
    if ( firstmilli == 0 )
        firstmilli = milliseconds();
    if ( milliseconds() < firstmilli+5000 )
        return;
    if ( (ip_port= queue_dequeue(&P2P_Q)) != 0 )
    {
        add_SuperNET_peer(ip_port);
        free(ip_port);
    }
}

#endif
