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
    struct sockaddr addr;
    uv_udp_t *udp;
    uv_buf_t buf;
    int32_t allocflag;
};

struct udp_queuecmd
{
    struct sockaddr prevaddr;
    cJSON *argjson;
    struct NXT_acct *tokenized_np;
    char *decoded;
    int32_t valid;
};

/*struct peer_queue_entry
{
    struct peerinfo *peer,*hop;
    float startmilli,elapsed;
    uint8_t stateid,state;
};*/

static long server_xferred;
int Servers_started;
queue_t P2P_Q,sendQ,JSON_Q,udp_JSON;
struct pingpong_queue PeerQ;
//struct peerinfo **Peers,**Pservers;Numpeers,Numpservers,
int32_t Num_in_whitelist;
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
        //if ( Debuglevel > 0 )
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
        queue_enqueue(&sendQ,wr);
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
        if ( strcmp("127.0.0.1",ipaddr) == 0 )
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
            if ( Debuglevel > 0 )
                printf("UDP RECEIVED %ld from %s/%d crc.%x | ",nread,ipaddr,supernet_port,_crc32(0,rcvbuf->base,nread));
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
    int32_t port;
    struct nodestats *stats;
    struct pserver_info *pserver;
    port = extract_nameport(ipaddr,sizeof(ipaddr),(struct sockaddr_in *)destaddr);
    if ( len <= MAX_UDPLEN && Global_mp->udp != 0 )
    {
        if ( port == 0 || port == BTCD_PORT )
        {
            printf("send_packet WARNING: send_packet port.%d <- %d\n",port,SUPERNET_PORT);
            port = SUPERNET_PORT;
            uv_ip4_addr(ipaddr,port,(struct sockaddr_in *)destaddr);
        }
        if ( Debuglevel > 1 )
            printf("portable_udpwrite %d to (%s:%d)\n",len,ipaddr,port);
        portable_udpwrite(1,destaddr,Global_mp->udp,finalbuf,len,ALLOCWR_ALLOCFREE);
    }
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

/*int32_t is_privacyServer(struct peerinfo *peer)
{
    if ( peer->srvnxtbits == peer->pubnxtbits && peer->srv.ipbits != 0 )//&& peer->srv.supernet_port != 0 )
        return(1);
    return(0);
}*/

uint64_t route_packet(int32_t encrypted,struct sockaddr *destaddr,char *hopNXTaddr,unsigned char *outbuf,int32_t len)
{
    unsigned char finalbuf[4096];
    char destip[64];
    struct sockaddr_in addr;
    //struct Uaddr *Uaddrs[8];
    uint64_t txid = 0;
    int32_t port,createdflag;
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
        if ( Debuglevel > 0 )
            printf("DIRECT send encrypted.%d to (%s/%d) finalbuf.%d\n",encrypted,destip,port,len);
        send_packet(0,destaddr,outbuf,len);
    }
    else
    {
        np = get_NXTacct(&createdflag,Global_mp,hopNXTaddr);
        expand_ipbits(destip,np->stats.ipbits);
        if ( np->stats.ipbits != 0 )//is_privacyServer(&np->mypeerinfo) != 0 )
        {
            if ( Debuglevel > 0 )
                printf("DIRECT udpsend {%s} to %s/%d finalbuf.%d\n",hopNXTaddr,destip,np->stats.supernet_port,len);
            uv_ip4_addr(destip,np->stats.supernet_port,&addr);
            send_packet(&np->stats,(struct sockaddr *)&addr,finalbuf,len);
        }
        /*else
        {
            memset(Uaddrs,0,sizeof(Uaddrs));
            n = sort_topaddrs(Uaddrs,(int32_t)(sizeof(Uaddrs)/sizeof(*Uaddrs)),&np->mypeerinfo);
            printf("n.%d hopNXTaddr.(%s) narrowcast %s:%d %d via p2p\n",len,hopNXTaddr,destip,np->stats.supernet_port,len);
            if ( n > 0 )
            {
                for (i=0; i<n; i++)
                {
                    Uaddrs[i]->numsent++;
                    uv_ip4_addr(destip,np->stats.supernet_port==0?SUPERNET_PORT:np->stats.supernet_port,&addr);
                    send_packet(&np->stats,(struct sockaddr *)&addr,finalbuf,len);
                }
            }
        }*/
    }
    txid = calc_txid(finalbuf,len);
    return(txid);
}

uint64_t directsend_packet(int32_t encrypted,struct pserver_info *pserver,char *origargstr,int32_t len,unsigned char *data,int32_t datalen)
{
    //int32_t direct_onionize(uint64_t nxt64bits,unsigned char *destpubkey,unsigned char *maxbuf,unsigned char *encoded,unsigned char **payloadp,int32_t len);
    static unsigned char zeropubkey[crypto_box_PUBLICKEYBYTES];
    uint64_t txid = 0;
    int32_t port;
    struct sockaddr destaddr;
    struct nodestats *stats;
    struct coin_info *cp = get_coin_info("BTCD");
    unsigned char *outbuf;
    //memset(encoded,0,sizeof(encoded)); // encoded to dest
    if ( (stats= get_nodestats(pserver->nxt64bits)) != 0 )
        port = stats->supernet_port != 0 ? stats->supernet_port : SUPERNET_PORT;
    else port = SUPERNET_PORT;
    uv_ip4_addr(pserver->ipaddr,port,(struct sockaddr_in *)&destaddr);
    len = (int32_t)strlen(origargstr)+1;
    if ( encrypted != 0 && stats != 0 && memcmp(zeropubkey,stats->pubkey,sizeof(zeropubkey)) != 0 )
    {
        char *sendmessage(char *hopNXTaddr,int32_t L,char *verifiedNXTaddr,char *msg,int32_t msglen,char *destNXTaddr,unsigned char *data,int32_t datalen);
        char hopNXTaddr[64],destNXTaddr[64],*retstr;
        expand_nxt64bits(destNXTaddr,stats->nxt64bits);
        retstr = sendmessage(hopNXTaddr,0*(encrypted>1?Global_mp->Lfactor:0),cp->srvNXTADDR,origargstr,len,destNXTaddr,data,datalen);
        if ( retstr != 0 )
        {
            if ( Debuglevel > 0 )
                printf("direct send via sendmessage got (%s)\n",retstr);
            free(retstr);
        }
        //len = direct_onionize(pserver->nxt64bits,stats->pubkey,encoded,0,&outbuf,len);
    }
    else
    {
        encrypted = 0;
        stripwhite_ns(origargstr,len);
        len = (int32_t)strlen(origargstr)+1;
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
            //printf("route_packet encrypted.%d\n",encrypted);
            txid = route_packet(encrypted,&destaddr,0,outbuf,len);
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
        //if ( Debuglevel > 1 )
            fprintf(stderr,"p2p_publishpacket.%p (%s)\n",pserver,cmd);
        np = get_NXTacct(&createdflag,Global_mp,cp->srvNXTADDR);
        if ( cmd == 0 )
        {
            sprintf(_cmd,"{\"requestType\":\"%s\",\"NXT\":\"%s\",\"time\":%ld,\"pubkey\":\"%s\",\"ipaddr\":\"%s\"}","ping",cp->srvNXTADDR,(long)time(NULL),Global_mp->pubkeystr,cp->myipaddr);
        }
        else strcpy(_cmd,cmd);
        //printf("_cmd.(%s)\n",_cmd);
        len = construct_tokenized_req(packet,_cmd,cp->srvNXTACCTSECRET);
        //if ( Debuglevel > 1 )
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
    char ipaddr[16];
    p2pport = parse_ipaddr(ipaddr,ip_port);
    if ( p2pport == 0 )
        p2pport = BTCD_PORT;
    pserver = get_pserver(&createdflag,ipaddr,0,p2pport);
    //pserver->S.BTCD_p2p = 1;
    if ( on_SuperNET_whitelist(ipaddr) != 0 )
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
