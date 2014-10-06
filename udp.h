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
    //char privacyServer_NXTaddr[64],privacyServer_ipaddr[32],privacyServer_port[16];
    // uint64_t privacyServer;
    struct hashtable **orderbook_txidsp,*msg_txids;
};
struct pNXT_info *Global_pNXT;

union writeU { uv_udp_send_t ureq; uv_write_t req; };

struct write_req_t
{
    union writeU U;
    uv_buf_t buf;
    int32_t allocflag;
};

struct peer_queue_entry
{
    struct peerinfo *peer,*hop;
    float startmilli,elapsed;
    uint8_t stateid,state;
};

static long server_xferred;
int Servers_started;
queue_t P2P_Q;
struct pingpong_queue PeerQ;
struct peerinfo **Peers,**Pservers;
int32_t Numpeers,Numpservers,Num_in_whitelist;
uint32_t *SuperNET_whitelist;

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

int32_t portable_udpwrite(const struct sockaddr *addr,uv_udp_t *handle,void *buf,long len,int32_t allocflag)
{
    char ipaddr[64];
    struct nodestats *stats;
    struct pserver_info *pserver;
    int32_t r,supernet_port,createdflag;
    struct write_req_t *wr;
    wr = alloc_wr(buf,len,allocflag);
    ASSERT(wr != NULL);
    {
        supernet_port = extract_nameport(ipaddr,sizeof(ipaddr),(struct sockaddr_in *)addr);
        pserver = get_pserver(&createdflag,ipaddr,supernet_port,0);
        if ( (stats= get_nodestats(pserver->nxt64bits)) != 0 )
        {
            stats->numsent++;
            stats->sentmilli = milliseconds();
        }
        //for (i=0; i<16; i++)
        //    printf("%02x ",((unsigned char *)buf)[i]);
        printf("portable_udpwrite %ld bytes to %s/%d crx.%x\n",len,ipaddr,supernet_port,_crc32(0,buf,len));
    }
    r = uv_udp_send(&wr->U.ureq,handle,&wr->buf,1,addr,(uv_udp_send_cb)after_write);
    if ( r != 0 )
        printf("uv_udp_send error.%d %s wr.%p wreq.%p %p len.%ld\n",r,uv_err_name(r),wr,&wr->U.ureq,buf,len);
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
    char ipaddr[256],retjsonstr[4096],NXTaddr[64],srvNXTaddr[64];
    retjsonstr[0] = 0;
    if ( cp != 0 && nread > 0 )
    {
        supernet_port = extract_nameport(ipaddr,sizeof(ipaddr),(struct sockaddr_in *)addr);
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
            printf("UDP RECEIVED %ld from %s/%d crc.%x\n",nread,ipaddr,supernet_port,_crc32(0,rcvbuf->base,nread));
        }
        expand_nxt64bits(NXTaddr,cp->pubnxtbits);
        expand_nxt64bits(srvNXTaddr,cp->srvpubnxtbits);
        np = process_packet(retjsonstr,(unsigned char *)rcvbuf->base,(int32_t)nread,udp,(struct sockaddr *)addr,ipaddr,supernet_port);
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
    struct nodestats *stats;
    struct pserver_info *pserver;
    extract_nameport(ipaddr,sizeof(ipaddr),(struct sockaddr_in *)destaddr);
    if ( len <= MAX_UDPLEN && Global_mp->udp != 0 )
        portable_udpwrite(destaddr,Global_mp->udp,finalbuf,len,ALLOCWR_ALLOCFREE);
    else call_SuperNET_broadcast(get_pserver(0,ipaddr,0,0),(char *)finalbuf,len,0);
    pserver = get_pserver(0,ipaddr,0,0);
    if ( peerstats != 0 )
    {
        if ( pserver->nxt64bits == 0 )
            pserver->nxt64bits = peerstats->nxt64bits;
        peerstats->numsent++;
        peerstats->sentmilli = milliseconds();
    }
    else
    {
        if ( (stats= get_nodestats(pserver->nxt64bits)) != 0 )
        {
            stats->sentmilli++;
            stats->sentmilli = milliseconds();
        }
    }
}

static int _cmp_Uaddr(const void *a,const void *b)
{
#define val_a ((struct Uaddr *)a)->metric
#define val_b ((struct Uaddr *)b)->metric
	if ( val_b > val_a )
		return(1);
	else if ( val_b < val_a )
		return(-1);
	return(0);
#undef val_a
#undef val_b
}

double calc_Uaddr_metric(struct Uaddr *uaddr,uint32_t now)
{
    double elapsed,metric = 0.;
    if ( uaddr->lastcontact != 0 && uaddr->numrecv > 0 )
    {
        if ( uaddr->lastcontact <= now )
        {
            elapsed = ((double)(now - uaddr->lastcontact) + 1) / 60.;
            metric = (double)(uaddr->numsent + 1) / uaddr->numrecv;
            if ( metric > 1. )
                metric = 1.;
            metric /= sqrt(elapsed);
        }
    }
    return(metric);
}

int32_t sort_topaddrs(struct Uaddr **Uaddrs,int32_t max,struct peerinfo *peer)
{
    int32_t i,n;
    void **ptrs;
    uint32_t now = (uint32_t)time(NULL);
    if ( peer->numUaddrs == 0 || peer->Uaddrs == 0 )
        return(0);
    n = (peer->numUaddrs > max) ? max : peer->numUaddrs;
    if ( peer->numUaddrs > 1 )
    {
        ptrs = malloc(sizeof(*Uaddrs) * peer->numUaddrs);
        for (i=0; i<peer->numUaddrs; i++)
        {
            peer->Uaddrs[i].metric = calc_Uaddr_metric(ptrs[i],now);
            ptrs[i] = &peer->Uaddrs[i];
        }
        qsort(ptrs,peer->numUaddrs,sizeof(*Uaddrs),_cmp_Uaddr);
        for (i=0; i<n; i++)
        {
            if ( ((struct Uaddr *)ptrs[i])->metric <= 0. )
            {
                n = i;
                break;
            }
            Uaddrs[i] = ptrs[i];
        }
        free(ptrs);
    }
    else Uaddrs[0] = &peer->Uaddrs[0];
    return(n);
}

int32_t is_privacyServer(struct peerinfo *peer)
{
    if ( peer->srvnxtbits == peer->pubnxtbits && peer->srv.ipbits != 0 && peer->srv.supernet_port != 0 )
        return(1);
    return(0);
}

uint64_t route_packet(int32_t encrypted,struct sockaddr *destaddr,char *hopNXTaddr,unsigned char *outbuf,int32_t len)
{
    unsigned char finalbuf[4096];
    char destip[64];
    struct sockaddr_in addr;
    struct Uaddr *Uaddrs[8];
    uint64_t txid = 0;
    int32_t port,createdflag,i,n;
    struct NXT_acct *np;
    if ( len > sizeof(finalbuf) )
    {
        printf("sendmessage: len.%d > sizeof(finalbuf) %ld\n",len,sizeof(finalbuf));
        exit(-1);
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
        //printf("DIRECT send encrypted.%d to (%s/%d) finalbuf.%d\n",encrypted,destip,port,len);
        send_packet(0,destaddr,outbuf,len);
    }
    else
    {
        np = get_NXTacct(&createdflag,Global_mp,hopNXTaddr);
        expand_ipbits(destip,np->mypeerinfo.srv.ipbits);
        if ( is_privacyServer(&np->mypeerinfo) != 0 )
        {
            printf("DIRECT udpsend {%s} to %s/%d finalbuf.%d\n",hopNXTaddr,destip,np->mypeerinfo.srv.supernet_port,len);
            uv_ip4_addr(destip,np->mypeerinfo.srv.supernet_port,&addr);
            send_packet(&np->mypeerinfo.srv,(struct sockaddr *)&addr,finalbuf,len);
        }
        else
        {
            memset(Uaddrs,0,sizeof(Uaddrs));
            n = sort_topaddrs(Uaddrs,(int32_t)(sizeof(Uaddrs)/sizeof(*Uaddrs)),&np->mypeerinfo);
            printf("n.%d hopNXTaddr.(%s) narrowcast %s:%d %d via p2p\n",len,hopNXTaddr,destip,np->mypeerinfo.srv.supernet_port,len);
            if ( n > 0 )
            {
                for (i=0; i<n; i++)
                {
                    Uaddrs[i]->numsent++;
                    uv_ip4_addr(destip,np->mypeerinfo.srv.supernet_port==0?SUPERNET_PORT:np->mypeerinfo.srv.supernet_port,&addr);
                    send_packet(&np->mypeerinfo.srv,(struct sockaddr *)&addr,finalbuf,len);
                }
            }
        }
    }
    txid = calc_txid(finalbuf,len);
    return(txid);
}

uint64_t directsend_packet(struct pserver_info *pserver,char *origargstr,int32_t len,unsigned char *data,int32_t datalen)
{
    int32_t direct_onionize(uint64_t nxt64bits,unsigned char *destpubkey,unsigned char *maxbuf,unsigned char *encoded,unsigned char **payloadp,int32_t len);
    static unsigned char zeropubkey[crypto_box_PUBLICKEYBYTES];
    uint64_t txid = 0;
    int32_t port,encrypted = 0;
    struct sockaddr destaddr;
    struct nodestats *stats;
    unsigned char encoded[4096],*outbuf;
    memset(encoded,0,sizeof(encoded)); // encoded to dest
    if ( (stats= get_nodestats(pserver->nxt64bits)) != 0 )
        port = stats->supernet_port != 0 ? stats->supernet_port : SUPERNET_PORT;
    else port = BTCD_PORT;
    uv_ip4_addr(pserver->ipaddr,port,(struct sockaddr_in *)&destaddr);
    len = (int32_t)strlen(origargstr)+1;
    stripwhite_ns(origargstr,len);
    len = (int32_t)strlen(origargstr)+1;
    outbuf = (unsigned char *)origargstr;
    if ( data != 0 && datalen > 0 )
    {
        memcpy(outbuf+len,data,datalen);
        len += datalen;
    }
    if ( stats != 0 && memcmp(zeropubkey,stats->pubkey,sizeof(zeropubkey)) != 0 )
        len = direct_onionize(pserver->nxt64bits,stats->pubkey,encoded,0,&outbuf,len), encrypted = 1;
    //printf("directsend to %llu (%s).%d stats.%p\n",(long long)pserver->nxt64bits,pserver->ipaddr,port,stats);
    if ( len > sizeof(encoded)-1024 )
        printf("directsend_packet: payload too big %d\n",len);
    else if ( len > 0 )
    {
        txid = route_packet(encrypted,&destaddr,0,outbuf,len);
        //printf("got route_packet txid.%llu\n",(long long)txid);
    }
    else printf("directsend_packet: illegal len.%d\n",len);
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
        fprintf(stderr,"p2p_publishpacket.%p (%s)\n",pserver,cmd);
        np = get_NXTacct(&createdflag,Global_mp,cp->srvNXTADDR);
        if ( cmd == 0 )
            set_peer_json(_cmd,np->H.U.NXTaddr,&np->mypeerinfo);
        else strcpy(_cmd,cmd);
        //printf("_cmd.(%s)\n",_cmd);
        len = construct_tokenized_req(packet,_cmd,cp->srvNXTACCTSECRET);
        printf("len.%d (%s)\n",len,packet);
        return(call_SuperNET_broadcast(pserver,packet,len+1,PUBADDRS_MSGDURATION));
    }
    printf("ERROR: broadcast_publishpacket null cp\n");
    return(0);
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
        }
    }
    if ( memcmp(zeropubkey,stats->pubkey,sizeof(zeropubkey)) != 0 )
    {
        void update_Kbuckets(struct nodestats *stats,uint64_t,char *,int32_t,int32_t);
        update_Kbuckets(stats,nxt64bits,ipaddr,port,p2pflag);
    }
    return(modified);
}

int32_t update_routing_probs(char *NXTaddr,int32_t encryptedflag,int32_t p2pflag,struct peerinfo *peer,char *ipaddr,int32_t port,unsigned char pubkey[crypto_box_PUBLICKEYBYTES])
{
    uint32_t now,modified = 0;
    struct pserver_info *pserver;
    struct nodestats *stats;
    //struct Uaddr *uaddr;
    //int32_t i;
    //uint32_t ipbits;
    now = (uint32_t)time(NULL);
    if ( p2pflag != 0 )
        pserver = get_pserver(0,ipaddr,0,port);
    else pserver = get_pserver(0,ipaddr,port,0);
    if ( peer != 0 )
        modified = update_nodestats(NXTaddr,now,&peer->srv,encryptedflag,p2pflag,(is_privacyServer(peer) != 0) ? pubkey : 0,ipaddr,port);
    if ( (stats= get_nodestats(calc_nxt64bits(NXTaddr))) != 0 )
        modified |= (update_nodestats(NXTaddr,now,stats,encryptedflag,p2pflag,pubkey,ipaddr,port) << 1);
    return(modified);
    /*if ( is_privacyServer(peer) != 0 )
     {
     }
     else
     {
     i = 0;
     uaddr = 0;
     if ( peer->numUaddrs > 0 )
     {
     for (; i<peer->numUaddrs; i++)
     {
     uaddr = &peer->Uaddrs[i];
     if ( uaddr->ipbits == ipbits )
     break;
     }
     }
     if ( i == peer->numUaddrs )
     {
     peer->Uaddrs = realloc(peer->Uaddrs,sizeof(*peer->Uaddrs) * (peer->numUaddrs+1));
     uaddr = &peer->Uaddrs[peer->numUaddrs++];
     memset(uaddr,0,sizeof(*uaddr));
     uaddr->ipbits = ipbits;
     printf("add.%d Uaddr.(%s/%d) to %llu\n",peer->numUaddrs,ipaddr,port,(long long)peer->pubnxtbits);
     }
     if ( uaddr != 0 )
     {
     uaddr->numrecv++;
     uaddr->lastcontact = (uint32_t)time(NULL);
     } else printf("update_routing_probs: unexpected null uaddr for %s/%d!\n",ipaddr,port);
     }*/
}
#endif
