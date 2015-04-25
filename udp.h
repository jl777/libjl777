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
#define MAX_UDPQUEUE_MILLIS 7

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
    //struct queueitem DL;
    union writeU U;
    struct sockaddr addr;
    uv_udp_t *udp;
    uv_buf_t buf;
    int32_t allocflag,isbridge;
    float queuetime;
};

struct udp_entry
{
    struct queueitem DL;
    struct sockaddr addr;
    void *buf;
    uv_udp_t *udp;
    int32_t len,internalflag;
};

struct udp_queuecmd
{
    struct queueitem DL;
    cJSON *argjson;
    struct NXT_acct *tokenized_np;
    char *decoded;
    uint32_t previpbits,valid;
};

struct pserver_info *get_pserver(int32_t *createdp,char *ipaddr,uint16_t supernet_port,uint16_t p2pport)
{
    int32_t createdflag = 0;
    struct pserver_info *pserver;
    if ( createdp == 0 )
        createdp = &createdflag;
    pserver = MTadd_hashtable(createdp,Global_mp->Pservers_tablep,ipaddr);
    if ( supernet_port != 0 )
    {
        pserver->lastcontact = (uint32_t)time(NULL);
        pserver->numrecv++;
        pserver->recvmilli = milliseconds();
        if ( pserver->firstport == 0 )
            pserver->firstport = supernet_port;
        pserver->lastport = supernet_port;
        if ( *createdp != 0 || (supernet_port != 0 && supernet_port != BTCD_PORT && supernet_port != pserver->supernet_port) )
        {
            pserver->supernet_altport = 0;
            pserver->supernet_port = supernet_port;
        }
    }
    if ( *createdp != 0 || (p2pport != 0 && p2pport != SUPERNET_PORT && p2pport != pserver->p2pport) )
        pserver->p2pport = p2pport;
    return(pserver);
}

uint16_t get_SuperNET_port(char *ipaddr)
{
    uint16_t port;
    int32_t gap;
    struct coin_info *cp = get_coin_info("BTCD");
    struct pserver_info *pserver;
    pserver = get_pserver(0,ipaddr,0,0);
    gap = (int32_t)(time(NULL) - pserver->lastcontact);
    if ( (port= cp->bridgeport) == 0 || strcmp(ipaddr,cp->bridgeipaddr) != 0 )
    {
        port = pserver->supernet_port != 0 ? pserver->supernet_port : (pserver->supernet_altport != 0 ? pserver->supernet_altport : SUPERNET_PORT);
    }
    if ( gap < 60 )
    {
        if ( pserver->lastport != 0 )
            port = pserver->lastport;
    }
    else
    {
        if ( pserver->firstport != 0 )
            port = pserver->firstport;
    }
    return(port);
}

int32_t prevent_queueing(char *cmd)
{
    if ( strcmp("ping",cmd) == 0 || strcmp("pong",cmd) == 0 || strcmp("getdb",cmd) == 0 || 
        strcmp("sendfrag",cmd) == 0 || strcmp("gotfrag",cmd) == 0 || strcmp("ramchain",cmd) == 0 ||
        strcmp("genmultisig",cmd) == 0 || strcmp("getmsigpubkey",cmd) == 0 || strcmp("setmsigpubkey",cmd) == 0 ||
        0 )
        return(1);
    return(0);
}

void set_handler_fname(char *fname,char *handler,char *name)
{
    if ( strstr("../",name) != 0 || strstr("..\\",name) != 0 || name[0] == '/' || name[0] == '\\' || strcmp(name,"..") == 0  || strcmp(name,"*") == 0 )
    {
        //printf("(%s) invalid_filename.(%s) %p %p %d %d %d %d\n",handler,name,strstr("../",name),strstr("..\\",name),name[0] == '/',name[0] == '\\',strcmp(name,".."),strcmp(name,"*"));
        name = "invalid_filename";
    }
    sprintf(fname,"%s/%s/%s",DATADIR,handler,name);
}

int32_t load_handler_fname(void *dest,int32_t len,char *handler,char *name)
{
    FILE *fp;
    int32_t retval = -1;
    char fname[1024];
    set_handler_fname(fname,handler,name);
    if ( (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
    {
        fseek(fp,0,SEEK_END);
        if ( ftell(fp) == len )
        {
            rewind(fp);
            if ( fread(dest,1,len,fp) == len )
                retval = len;
        }
        fclose(fp);
    }
    return(retval);
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

void process_udpentry(struct udp_entry *up)
{
    struct coin_info *cp = get_coin_info("BTCD");
    char ipaddr[256],retjsonstr[4096];
    uint16_t supernet_port;
    supernet_port = extract_nameport(ipaddr,sizeof(ipaddr),(struct sockaddr_in *)&up->addr);
    if ( notlocalip(ipaddr) == 0 )
        strcpy(ipaddr,cp->myipaddr);
    retjsonstr[0] = 0;
    process_packet(up->internalflag,retjsonstr,up->buf,up->len,up->udp,&up->addr,ipaddr,supernet_port);
    free(up->buf);
    free(up);
}

int32_t is_whitelisted(char *ipaddr);
void _on_udprecv(int32_t queueflag,int32_t internalflag,uv_udp_t *udp,ssize_t nread,const uv_buf_t *rcvbuf,const struct sockaddr *addr,unsigned flags)
{
    uint16_t supernet_port = 0;
    int32_t createdflag;
    struct pserver_info *pserver = 0;
    struct udp_entry *up;
    struct coin_info *cp = get_coin_info("BTCD");
    char ipaddr[256],retjsonstr[4096];
    if ( addr != 0 )
    {
        supernet_port = extract_nameport(ipaddr,sizeof(ipaddr),(struct sockaddr_in *)addr);
        //if ( SOFTWALL != 0 )
        //    printf("SOFTWALL: is_whitelisted.%d %s nread.%ld\n",is_whitelisted(ipaddr),ipaddr,nread);
        if ( SOFTWALL != 0 && is_whitelisted(ipaddr) <= 0 )
        {
            printf("SOFTWALL: blocks %s:%d %ld bytes\n",ipaddr,supernet_port,nread);
            return;
        }
        if ( notlocalip(ipaddr) == 0 )
            strcpy(ipaddr,cp->myipaddr);
        pserver = get_pserver(&createdflag,ipaddr,supernet_port,0);
    } else if ( nread > 0 ) printf("_on_udprecv without addr? nread.%ld internal.%d queue.%d\n",nread,internalflag,queueflag);
    if ( cp != 0 && nread > 0 )
    {
        
        if ( Debuglevel > 2 || nread > MAX_UDPLEN )//|| (nread > 400 && nread != MAX_UDPLEN) )
            fprintf(stderr,"UDP RECEIVED %ld from %s/%d crc.%x\n",nread,ipaddr,supernet_port,_crc32(0,rcvbuf->base,nread));
        if ( nread > MAX_UDPLEN )
            nread = MAX_UDPLEN;
        ASSERT(addr->sa_family == AF_INET);
        server_xferred += nread;
        if ( queueflag != 0 )
        {
            up = calloc(1,sizeof(*up));
            up->udp = udp;
            up->internalflag = internalflag;
            up->buf = rcvbuf->base;
            up->len = (int32_t)nread;
            if ( addr != 0 )
                up->addr = *addr;
            queue_enqueue("UDP_Q",&UDP_Q,&up->DL);
        }
        else
        {
            process_packet(internalflag,retjsonstr,(unsigned char *)rcvbuf->base,(int32_t)nread,udp,(struct sockaddr *)addr,ipaddr,supernet_port);
            free(rcvbuf->base);
        }
    }
    else if ( rcvbuf->base != 0 )
        free(rcvbuf->base);
}

void on_udprecv(uv_udp_t *udp,ssize_t nread,const uv_buf_t *rcvbuf,const struct sockaddr *addr,unsigned flags)
{
    _on_udprecv(1,0,udp,nread,rcvbuf,addr,flags);
}

void on_bridgerecv(uv_udp_t *udp,ssize_t nread,const uv_buf_t *rcvbuf,const struct sockaddr *addr,unsigned flags)
{
    _on_udprecv(0,1,udp,nread,rcvbuf,addr,flags);
}

uv_udp_t *open_udp(uint16_t port,void (*handler)(uv_udp_t *,ssize_t,const uv_buf_t *,const struct sockaddr *,uint32_t))
{
    static uv_udp_t *fixed_udp;
    int32_t r;
    uv_udp_t *udp;
    struct sockaddr_in addr;
    if (  MULTIPORT == 0 && fixed_udp != 0 )
        return(fixed_udp);
    udp = malloc(sizeof(uv_udp_t));
    r = uv_udp_init(UV_loop,udp);
    if ( r != 0 )
    {
        fprintf(stderr, "uv_udp_init: %d %s\n",r,uv_err_name(r));
        return(0);
    }
    /*uv_udp_init(loop, &recv_socket);
    struct sockaddr_in recv_addr = uv_ip4_addr("0.0.0.0", 68);
    uv_udp_bind(&recv_socket, recv_addr, 0);
    uv_udp_recv_start(&recv_socket, alloc_buffer, on_read);
    
    uv_udp_init(loop, &send_socket);
    uv_udp_bind(&send_socket, uv_ip4_addr("0.0.0.0", 0), 0);
    uv_udp_set_broadcast(&send_socket, 1);*/
    
    
    if ( port != 0 )
    {
        uv_ip4_addr("0.0.0.0",port,&addr);
        r = uv_udp_bind(udp,(struct sockaddr *)&addr,0);
        if ( r != 0 )
        {
            fprintf(stderr,"uv_udp_bind: %d %s\n",r,uv_err_name(r));
            return(0);
        }
        r = uv_udp_recv_start(udp,portable_alloc,handler);
        if ( r != 0 )
        {
            fprintf(stderr, "uv_udp_recv_start: %d %s\n",r,uv_err_name(r));
            return(0);
        }
        r = uv_udp_set_broadcast(udp,1);
        if ( r != 0 )
        {
            fprintf(stderr,"uv_udp_set_broadcast: %d %s\n",r,uv_err_name(r));
            return(0);
        }
        fixed_udp = udp;
    }
    else
    {
        uv_ip4_addr("0.0.0.0",0,&addr);
        r = uv_udp_bind(udp,(struct sockaddr *)&addr,0);
        if ( r != 0 )
        {
            fprintf(stderr,"uv_udp_bind: %d %s\n",r,uv_err_name(r));
            return(0);
        }
        r = uv_udp_recv_start(udp,portable_alloc,handler);
        if ( r != 0 )
        {
            fprintf(stderr, "uv_udp_recv_start: %d %s\n",r,uv_err_name(r));
            return(0);
        }
        r = uv_udp_set_broadcast(udp,1);
        if ( r != 0 )
        {
            fprintf(stderr,"uv_udp_set_broadcast: %d %s\n",r,uv_err_name(r));
            return(0);
        }
    }
    return(udp);
}

int32_t process_sendQ_item(struct write_req_t *wr)
{
    char ipaddr[64];
    struct coin_info *cp = get_coin_info("BTCD");
    struct pserver_info *pserver;
    int32_t r,supernet_port,createdflag;
    supernet_port = extract_nameport(ipaddr,sizeof(ipaddr),(struct sockaddr_in *)&wr->addr);
    pserver = get_pserver(&createdflag,ipaddr,0,0);
    if ( pserver->udps[wr->isbridge] == 0 )
    {
        if ( (pserver->udps[wr->isbridge]= open_udp(0,on_udprecv)) == 0 )
            return(-1);
    }
    if ( 1 && (pserver->nxt64bits == cp->privatebits || pserver->nxt64bits == cp->srvpubnxtbits) )
    {
        //printf("(%s/%d) no point to send yourself dest.%llu pub.%llu srvpub.%llu\n",ipaddr,supernet_port,(long long)pserver->nxt64bits,(long long)cp->pubnxtbits,(long long)cp->srvpubnxtbits);
        return(0);
        strcpy(ipaddr,"127.0.0.1");
        uv_ip4_addr(ipaddr,supernet_port,(struct sockaddr_in *)&wr->addr);
    }
    pserver->numsent++;
    pserver->sentmilli = milliseconds();
    //for (i=0; i<16; i++)
    //    printf("%02x ",((unsigned char *)buf)[i]);
    if ( Debuglevel > 2 || wr->buf.len > MAX_UDPLEN )
        printf("%s uv_udp_send %ld bytes to %s/%d crx.%x\n",wr->buf.len > MAX_UDPLEN?"ERROR OVERFLOW":"",wr->buf.len,ipaddr,supernet_port,_crc32(0,wr->buf.base,wr->buf.len));
    if ( SOFTWALL != 0 && is_whitelisted(ipaddr) <= 0 )
    {
        printf("SOFTWALL: blocks sending to %s:%d\n",ipaddr,supernet_port);
        return(-1);
    }
    if ( pserver->udps[wr->isbridge] != 0 && wr->buf.base != 0 && wr->buf.len > 0 )
        r = uv_udp_send(&wr->U.ureq,pserver->udps[wr->isbridge],&wr->buf,1,&wr->addr,(uv_udp_send_cb)after_write);
    else r = -123;
    //r = uv_udp_try_send(pserver->udps[wr->isbridge],&wr->buf,1,&wr->addr);
    //printf("send to.(%s:%d) retval.%d\n",ipaddr,supernet_port,r);
    if ( r < 0 )
        printf("uv_udp_send error.%d %s wr.%p wreq.%p %p len.%ld\n",r,uv_err_name(r),wr,&wr->U.ureq,wr->buf.base,wr->buf.len);
    if ( THROTTLE != 0 )
        msleep(THROTTLE);
    return(r);
}

int32_t portable_udpwrite(int32_t queueflag,const struct sockaddr *addr,int32_t isbridge,void *buf,long len,int32_t allocflag)
{
    int32_t r=0;
    struct write_req_t *wr;
    wr = alloc_wr(buf,len,allocflag);
    ASSERT(wr != NULL);
    wr->addr = *addr;
    wr->isbridge = isbridge;
   /* if ( 0 && Global_mp->isMM == 0 && FASTMODE == 0 && queueflag != 0 ) // support oversized packets?
    {
        wr->queuetime = (uint32_t)(milliseconds() + (rand() % MAX_UDPQUEUE_MILLIS));
        queue_enqueue("sendQ",&sendQ,&wr->DL);
    }
    else*/ r = process_sendQ_item(wr);
    return(r);
}

void *start_libuv_udpserver(int32_t ip4_or_ip6,uint16_t port,void (*handler)(uv_udp_t *,ssize_t,const uv_buf_t *,const struct sockaddr *,uint32_t))
{
    void *srv = 0;
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
    srv = open_udp(port,handler);
    if ( srv != 0 )
    {
        char ipaddr[64];
        uint16_t ip_port;
        ip_port = extract_nameport(ipaddr,sizeof(ipaddr),(struct sockaddr_in *)ptr);
        printf("UDP.%p server started on port %d <-> (%s:%d)\n",srv,port,ipaddr,ip_port);
    }
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

void send_packet(int32_t queueflag,uint32_t ipbits,struct sockaddr *destaddr,unsigned char *finalbuf,int32_t len)
{
    char ipaddr[64];
    struct sockaddr dest;
    int32_t port = 0;
    struct pserver_info *pserver = 0;
    if ( destaddr == 0 )
    {
        expand_ipbits(ipaddr,ipbits);
        pserver = get_pserver(0,ipaddr,0,0);
        port = get_SuperNET_port(ipaddr);
        memset(&dest,0,sizeof(dest));
        destaddr = &dest;
        uv_ip4_addr(ipaddr,port,(struct sockaddr_in *)destaddr);
    }
    port = extract_nameport(ipaddr,sizeof(ipaddr),(struct sockaddr_in *)destaddr);
    if ( strcmp(ipaddr,Global_mp->ipaddr) == 0 || strcmp(ipaddr,"127.0.0.1") == 0 )
        return;
    pserver = get_pserver(0,ipaddr,0,0);
    if ( port == 0 || port == BTCD_PORT )
    {
        port = get_SuperNET_port(ipaddr);
        uv_ip4_addr(ipaddr,port,(struct sockaddr_in *)destaddr);
    }
    //if ( len <= MAX_UDPLEN )//&& Global_mp->udp != 0 )
    {
        if ( Debuglevel > 3 )
            printf("portable_udpwrite Q.%d %d to (%s:%d)\n",queueflag,len,ipaddr,port);
        portable_udpwrite(queueflag,destaddr,0,finalbuf,len,ALLOCWR_ALLOCFREE);
    }
}

void route_packet(int32_t queueflag,int32_t encrypted,struct sockaddr *destaddr,char *hopNXTaddr,unsigned char *outbuf,int32_t len)
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
        np = get_NXTacct(&createdflag,hopNXTaddr);
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
        if ( Debuglevel > 2 )
            printf("DIRECT send encrypted.%d to (%s/%d) finalbuf.%d\n",encrypted,destip,port,len);
        send_packet(queueflag,0,destaddr,outbuf,len);
    }
    else if ( stats != 0 )
    {
        expand_ipbits(destip,stats->ipbits);
        if ( stats->ipbits != 0 )
        {
            if ( Debuglevel > 2 )
                printf("DIRECT udpsend {%s} to %s finalbuf.%d\n",hopNXTaddr,destip,len);
            port = get_SuperNET_port(destip);
            uv_ip4_addr(destip,port,&addr);
            send_packet(queueflag,stats->ipbits,(struct sockaddr *)&addr,finalbuf,len);
        }
        else { printf("cant route packet.%d without IP address to %llu\n",len,(long long)stats->nxt64bits); return; }
    } else { printf("cant route packet.%d without nodestats\n",len); return; }
}

uint64_t directsend_packet(int32_t queueflag,int32_t encrypted,struct pserver_info *pserver,char *origargstr,int32_t len,unsigned char *data,int32_t datalen)
{
    static unsigned char zeropubkey[crypto_box_PUBLICKEYBYTES];
    uint64_t txid = 0;
    int32_t port,L;
    struct sockaddr destaddr;
    struct nodestats *stats = 0;
    struct coin_info *cp = get_coin_info("BTCD");
    unsigned char *outbuf;
    if ( 0 && (pserver->nxt64bits == 0 || (stats= get_nodestats(pserver->nxt64bits)) == 0 || stats->ipbits == 0) )
    {
        printf("no nxtaddr.%llu or null stats.%p ipbits.%x\n",(long long)pserver->nxt64bits,stats,stats!=0?stats->ipbits:0);
        return(0);
    }
    if ( pserver->nxt64bits != 0 )
        stats = get_nodestats(pserver->nxt64bits);
    port = get_SuperNET_port(pserver->ipaddr);
    uv_ip4_addr(pserver->ipaddr,port,(struct sockaddr_in *)&destaddr);
    stripwhite_ns(origargstr,len);
    len = (int32_t)strlen(origargstr)+1;
    txid = calc_txid((uint8_t *)origargstr,len);
    if ( encrypted != 0 && stats != 0 && memcmp(zeropubkey,stats->pubkey,sizeof(zeropubkey)) != 0 )
    {
        char *sendmessage(int32_t queueflag,char *hopNXTaddr,int32_t L,char *verifiedNXTaddr,char *msg,int32_t msglen,char *destNXTaddr,unsigned char *data,int32_t datalen);
        char hopNXTaddr[64],destNXTaddr[64],*retstr;
        expand_nxt64bits(destNXTaddr,stats->nxt64bits);
        L = (encrypted>1 ? MAX(encrypted,Global_mp->Lfactor) : 0);
        if ( Debuglevel > 2 )
            fprintf(stderr,"direct send via sendmessage (%s) %p %d\n",origargstr,data,datalen);
        retstr = sendmessage(queueflag,hopNXTaddr,L,cp->srvNXTADDR,origargstr,len,destNXTaddr,data,datalen);
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
            route_packet(queueflag,encrypted,&destaddr,0,outbuf,len);
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
    if ( cp != 0 && Finished_init != 0 )
    {
        if ( Debuglevel > 1 )
            fprintf(stderr,"p2p_publishpacket.%p (%s)\n",pserver,cmd);
        np = get_NXTacct(&createdflag,cp->srvNXTADDR);
        if ( cmd == 0 )
        {
            int32_t gen_pingstr(char *cmdstr,int32_t completeflag,char *coinstr);
            gen_pingstr(_cmd,1,0);
            //sprintf(_cmd,"{\"requestType\":\"%s\",\"NXT\":\"%s\",\"timestamp\":%ld,\"pubkey\":\"%s\",\"ipaddr\":\"%s\"}","ping",cp->srvNXTADDR,(long)time(NULL),Global_mp->pubkeystr,cp->myipaddr);
        }
        else strcpy(_cmd,cmd);
        //printf("_cmd.(%s)\n",_cmd);
        len = construct_tokenized_req(packet,_cmd,cp->srvNXTACCTSECRET);
        if ( Debuglevel > 1 )
            printf("len.%d (%s)\n",len,packet);
        return(call_SuperNET_broadcast(pserver,packet,len,PUBADDRS_MSGDURATION));
    }
    printf("ERROR: broadcast_publishpacket null cp.%p or not finished init.%d\n",cp,Finished_init);
    return(0);
}

struct transfer_args *create_transfer_args(char *previpaddr,char *sender,char *dest,char *name,uint32_t totallen,uint32_t blocksize,uint32_t totalcrc,char *handler,int32_t syncmem)
{
    static int didinit;
    static portable_mutex_t mutex;
    uint64_t txid;
    char hashstr[4096];
    struct transfer_args *args;
    int32_t createdflag;
    if ( didinit == 0 )
    {
        portable_mutex_init(&mutex);
        didinit = 1;
    }
    sprintf(hashstr,"%s.%s.%s.%u.%u.%u",dest,sender,name,totallen,totalcrc,blocksize);
    txid = calc_txid((uint8_t *)hashstr,(int32_t)strlen(hashstr));
    //printf("hashstr.(%s) -> %llx | data.%p\n",hashstr,(long long)txid,data);
    sprintf(hashstr,"%llx",(long long)txid);
    portable_mutex_lock(&mutex);
    args = MTadd_hashtable(&createdflag,Global_mp->pending_xfers,hashstr);
    if ( createdflag != 0 || totallen != args->totallen || args->blocksize != blocksize || args->totalcrc != totalcrc )
    {
        safecopy(args->previpaddr,previpaddr,sizeof(args->previpaddr));
        safecopy(args->sender,sender,sizeof(args->sender));
        safecopy(args->dest,dest,sizeof(args->dest));
        safecopy(args->name,name,sizeof(args->name));
        safecopy(args->handler,handler,sizeof(args->handler));
        args->totallen = totallen;
        args->blocksize = blocksize;
        args->numfrags = (totallen / blocksize);
        memset(args->crcs,0,sizeof(args->crcs));
        memset(args->gotcrcs,0,sizeof(args->gotcrcs));
        if ( (totallen % blocksize) != 0 )
            args->numfrags++;
        if ( Debuglevel > 1 )
            fprintf(stderr,"(%s) <- NEW XFERARGS.(%s) numfrags.%d blocksize.%d totallen.%d (%p %p %p %p)\n",args->dest,args->name,args->numfrags,blocksize,totallen,args->timestamps,args->crcs,args->gotcrcs,args->data);
    }
    args->totalcrc = totalcrc;
    args->syncmem = syncmem;
    /*if ( args->pstr == 0 )
        args->pstr = calloc(1,args->numfrags);
    if ( args->timestamps == 0 )
        args->timestamps = calloc(args->numfrags,sizeof(*args->timestamps));
    if ( args->crcs == 0 )
        args->crcs = calloc(args->numfrags,sizeof(*args->crcs));
    if ( args->gotcrcs == 0 )
        args->gotcrcs = calloc(args->numfrags,sizeof(*args->gotcrcs));
    if ( args->data == 0 )
        args->data = calloc(1,totallen);
    if ( args->slots == 0 )
        args->slots = calloc(args->numfrags,sizeof(*args->slots));
    if ( args->snapshot == 0 )
        args->snapshot = calloc(1,totallen);*/
    portable_mutex_unlock(&mutex);
   // fprintf(stderr,"return args.%p\n",args);
    return(args);
}

void calc_crcs(uint32_t *crcs,uint8_t *data,int32_t len,int32_t num,int32_t blocksize)
{
    int32_t fragi,offset = 0;
    for (fragi=0; fragi<num-1; fragi++,offset+=blocksize)
        crcs[fragi] = _crc32(0,data + offset,blocksize);
    if ( offset < len )
        crcs[fragi] = _crc32(0,data + offset,len - offset);
    //printf("crcs.%u: %u %u %u | %d\n",_crc32(0,data,len),crcs[0],crcs[1],crcs[2],len-offset);
}

int32_t update_transfer_args(char *sender,char *senderip,struct transfer_args *args,uint32_t fragi,uint32_t numfrags,uint32_t totalcrc,uint32_t datacrc,uint8_t *data,int32_t datalen)
{
    int i,count = 0;
    uint32_t checkcrc;
    if ( data == 0 ) // sender
    {
        if ( fragi < args->numfrags )
            args->gotcrcs[fragi] = datacrc;
        for (i=0; i<args->numfrags; i++)
        {
            if ( args->crcs[i] == args->gotcrcs[i] )
                count++;
        }
        if ( count == args->numfrags )
        {
            memcpy(args->snapshot,args->data,args->totallen);
            args->snapshotcrc = _crc32(0,args->snapshot,args->totallen);
            args->completed++;
            printf(">>>>>>>>> completed send to (%s) set ack[%d] <- %u | count.%d of %d | %p\n",args->dest,fragi,datacrc,count,args->numfrags,args);
        }
        if ( Debuglevel > 2 )
            printf(">>>>>>>>> set ack[%d] <- %u | count.%d of %d | %p\n",fragi,datacrc,count,args->numfrags,args);
    }
    else if ( args->totalcrc == totalcrc ) // recipient
    {
        if ( fragi < args->numfrags && fragi*args->blocksize+datalen <= args->totallen )
        {
            memcpy(args->data + fragi*args->blocksize,data,datalen);
            args->gotcrcs[fragi] = datacrc;
        }
        //fprintf(stderr,"update_transfer_args %d of %d <- %u datalen.%d | %u %u %u (%u)\n",fragi,args->numfrags,datacrc,datalen,args->gotcrcs[0],args->gotcrcs[1],args->gotcrcs[2],_crc32(0,args->data,args->totallen));
        for (i=0; i<args->numfrags; i++)
        {
            if ( args->gotcrcs[i] == args->crcs[i] )
                count++;
            //else printf("(%d of %d) %u vs %u\n",i,args->numfrags,args->gotcrcs[i],args->crcs[i]);
        }
        if ( count == args->numfrags )
        {
            count = -1;
            checkcrc = _crc32(0,args->data,args->totallen);
            if ( checkcrc == args->totalcrc )
            {
                memcpy(args->snapshot,args->data,args->totallen);
                args->snapshotcrc = _crc32(0,args->snapshot,args->totallen);
                if ( args->snapshotcrc != args->totalcrc )
                    printf("after snapshot crc ERROR %u != %u\n",args->snapshotcrc,args->totalcrc);
                else
                {
                    calc_crcs(args->crcs,args->snapshot,args->totallen,args->numfrags,args->blocksize);
                    for (i=count=0; i<args->numfrags; i++)
                    {
                        if ( args->gotcrcs[i] == args->crcs[i] )
                            count++;
                    }
                    if ( count == args->numfrags )
                    {
                        if ( Debuglevel > 1 )
                            printf("completed.%d (%s) totallen.%d to (%s) checkcrc.%u vs totalcrc.%u\n",count,args->name,args->totallen,args->dest,checkcrc,totalcrc);
                        handler_gotfile(sender,senderip,args,args->snapshot,args->totallen,args->snapshotcrc);
                        args->completed++;
                    }
                }
            }
        }
        if ( Debuglevel > 2 )
            fprintf(stderr,"update_transfer_args return count.%d\n",count);
    }
    return(count);
}

/*void purge_transfer_args(struct transfer_args *args)
{
    if ( args->pstr != 0 )
        free(args->pstr);
    if ( args->slots != 0 )
        free(args->slots), args->slots = 0;
    if ( args->data != 0 )
        free(args->data), args->data = 0;
    if ( args->snapshot != 0 )
        free(args->snapshot), args->snapshot = 0;
    if ( args->crcs != 0 )
        free(args->crcs), args->crcs = 0;
    if ( args->gotcrcs != 0 )
        free(args->gotcrcs), args->gotcrcs = 0;
    if ( args->timestamps != 0 )
        free(args->timestamps), args->timestamps = 0;
}*/

char *sendfrag(char *previpaddr,char *sender,char *verifiedNXTaddr,char *NXTACCTSECRET,char *dest,char *name,uint32_t fragi,uint32_t numfrags,uint32_t totallen,uint32_t blocksize,uint32_t totalcrc,uint32_t checkcrc,char *datastr,char *handler,int32_t syncmem)
{
    char cmdstr[4096],_tokbuf[4096],*cmd;
    struct pserver_info *pserver;
    struct transfer_args *args = 0;
    struct coin_info *cp = get_coin_info("BTCD");
    int32_t len,count=0,datalen = 0;
    unsigned char *data = 0;
    uint32_t datacrc,snapshotcrc;
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
    sprintf(cmdstr,"{\"NXT\":\"%s\",\"pubkey\":\"%s\",\"ipaddr\":\"%s\",\"name\":\"%s\",\"timestamp\":%ld,\"fragi\":%u,\"numfrags\":%u,\"totallen\":%u,\"blocksize\":%u,\"totalcrc\":%u,\"datacrc\":%u,\"handler\":\"%s\"",verifiedNXTaddr,Global_mp->pubkeystr,cp->myipaddr,name,(long)time(NULL),fragi,numfrags,totallen,blocksize,totalcrc,datacrc,handler);
    if ( previpaddr == 0 || previpaddr[0] == 0 )
    {
        cmd = "sendfrag";
        pserver = get_pserver(0,dest,0,0);
        if ( syncmem != 0 )
            sprintf(cmdstr+strlen(cmdstr),",\"syncmem\":%d",syncmem);
        sprintf(cmdstr+strlen(cmdstr),",\"requestType\":\"%s\",\"data\":%d}",cmd,datalen);
    }
    else
    {
        cmd = "gotfrag";
        pserver = get_pserver(0,previpaddr,0,0);
        if ( checkcrc != datacrc )
            strcat(cmdstr,",\"error\":\"crcerror\"");
        else
        {
            args = create_transfer_args(previpaddr,sender,dest,name,totallen,blocksize,totalcrc,handler,syncmem);
            if ( fragi < args->numfrags )
                args->crcs[fragi] = checkcrc;
            if ( Debuglevel > 2 )
                fprintf(stderr,"GOT SENDFRAG.(%s) datalen.%d %p %p (%u %u %u)\n",cmdstr,datalen,args->data,args->gotcrcs,args->crcs[0],args->crcs[1],args->crcs[2]);
            if ( datacrc == checkcrc )
                count = update_transfer_args(sender,previpaddr,args,fragi,numfrags,totalcrc,datacrc,data,datalen);
            snapshotcrc = _crc32(0,args->snapshot,args->totallen);
            sprintf(cmdstr+strlen(cmdstr),",\"snapshotcrc\":%u",snapshotcrc);
        }
        free(data);
        data = 0;
        datalen = 0;
        sprintf(cmdstr+strlen(cmdstr),",\"requestType\":\"%s\",\"count\":\"%d\",\"checkcrc\":%u}",cmd,count,checkcrc);
    }
    len = construct_tokenized_req(_tokbuf,cmdstr,NXTACCTSECRET);
    txid = directsend_packet(!prevent_queueing(cmd),1,pserver,_tokbuf,len,data,datalen);
    if ( data != 0 )
        free(data);
    return(clonestr(_tokbuf));
}

void send_fragi(char *NXTaddr,char *NXTACCTSECRET,struct transfer_args *args,int32_t fragi)
{
    char datastr[8192],*retstr;
    int32_t datalen;
    args->timestamps[fragi] = (uint32_t)time(NULL);
    if ( fragi != (args->numfrags-1) )
        datalen = args->blocksize;
    else datalen = (args->totallen - (args->blocksize * (args->numfrags-1)));
    init_hexbytes_noT(datastr,args->data + fragi*args->blocksize,datalen);
    retstr = sendfrag(0,NXTaddr,NXTaddr,NXTACCTSECRET,args->dest,args->name,fragi,args->numfrags,args->totallen,args->blocksize,args->totalcrc,args->crcs[fragi],datastr,args->handler,args->syncmem);
    if ( retstr != 0 )
        free(retstr);
}

int32_t set_fragislots(struct transfer_args *args)
{
    int32_t i,n;
    memset(args->slots,0,args->numfrags*sizeof(*args->slots));
    for (i=n=0; i<args->numfrags; i++)
        if ( args->crcs[i] != args->gotcrcs[i] )
            args->slots[n++] = i;
    return(n);
}

char *gotfrag(char *previpaddr,char *sender,char *NXTaddr,char *NXTACCTSECRET,char *src,char *name,uint32_t fragi,uint32_t numfrags,uint32_t totallen,uint32_t blocksize,uint32_t totalcrc,uint32_t datacrc,int32_t count,char *handler,int32_t syncmem,uint32_t snapshotcrc)
{
    int32_t i,j,n,match;
    struct transfer_args *args;
    char cmdstr[MAX_JSON_FIELD*2];
    if ( blocksize == 0 )
        blocksize = TRANSFER_BLOCKSIZE;
    if ( totallen == 0 )
        totallen = numfrags * blocksize;
//fprintf(stderr,"GOTFRAG.(%s)\n",cmdstr);
    args = create_transfer_args(previpaddr,NXTaddr,src,name,totallen,blocksize,totalcrc,handler,syncmem);
    match = update_transfer_args(sender,previpaddr,args,fragi,numfrags,totalcrc,datacrc,0,0);
    j = -1;
    if ( (snapshotcrc != args->totalcrc || match < args->numfrags) && args->blocksize == blocksize && args->totallen == totallen && args->totalcrc == totalcrc )
    {
        if ( (n= set_fragislots(args)) > 0 )
        {
            j = (rand() >> 8) % n;
            send_fragi(NXTaddr,NXTACCTSECRET,args,args->slots[j]);
        }
    }
    if ( Debuglevel > 2 )
    {
        for (i=0; i<args->numfrags; i++)
            sprintf(&args->pstr[i],"%c",args->gotcrcs[i]==0?' ': ((args->crcs[i] != args->gotcrcs[i]) ? '?' : '='));
        sprintf(args->pstr+strlen(args->pstr)," count.%d vs %d | recv.%d sent.%d | %p %s %s\n",count,match,fragi,j,args,name,handler);
        fprintf(stderr,"%s",args->pstr);
    }
    sprintf(cmdstr,"{\"requestType\":\"gotfrag\",\"sender\":\"%s\",\"ipaddr\":\"%s\",\"fragi\":%u,\"numfrags\":%u,\"totallen\":%u,\"blocksize\":%u,\"totalcrc\":%u,\"snapshotcrc\":%u,\"datacrc\":%u,\"count\":%d,\"handler\":\"%s\",\"syncmem\":%d}",sender,src,fragi,numfrags,totallen,blocksize,totalcrc,snapshotcrc,datacrc,count,handler,syncmem);
    return(clonestr(cmdstr));
}

char *start_transfer(char *previpaddr,char *sender,char *verifiedNXTaddr,char *NXTACCTSECRET,char *dest,char *name,uint8_t *data,int32_t totallen,int32_t timeout,char *handler,int32_t syncmem)
{
    static char *buf;
    static int64_t allocsize;
    struct transfer_args *args;
    int64_t len;
    int32_t n,incr,fragi,totalcrc,blocksize = TRANSFER_BLOCKSIZE;
    if ( data == 0 || totallen == 0 )
    {
        data = (uint8_t *)load_file(name,&buf,&len,&allocsize);
        totallen = (int32_t)len;
    }
    if ( totallen > MAX_TRANSFER_SIZE )
        return(clonestr("{\"result\":\"start_transfer cant transfer oversized request\"}"));
    if ( data != 0 && totallen != 0 )
    {
        totalcrc = _crc32(0,data,totallen);
        args = create_transfer_args(previpaddr,verifiedNXTaddr,dest,name,totallen,blocksize,totalcrc,handler,syncmem);
        if ( timeout != 0 )
            args->timeout = (uint32_t)time(NULL) + timeout;
        memcpy(args->data,data,totallen);
        calc_crcs(args->crcs,args->data,args->totallen,args->numfrags,args->blocksize);
        if ( (n= set_fragislots(args)) > 0 )
        {
            fprintf(stderr,"(%s) completed.%d start_transfer.args.%p n.%d of %d (%s) timeout.%u (%s)\n",name,args->completed,args,n,args->numfrags,verifiedNXTaddr,args->timeout,totallen<4096?(char *)data:"");
            if ( n < 8 )
                incr = 1;
            else incr = (n >> LOG2_MAX_XFERPACKETS);
            for (fragi=0; fragi<n; fragi+=incr)
                send_fragi(verifiedNXTaddr,NXTACCTSECRET,args,fragi);
        }
        else send_fragi(verifiedNXTaddr,NXTACCTSECRET,args,0);
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
        if ( Debuglevel > 0 )
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

//////////////////////////////
// 32 byte token

#define TRIT signed char

#define TRIT_FALSE -1
#define TRIT_UNKNOWN 0
#define TRIT_TRUE 1

#define SAM_HASH_SIZE 243
#define SAM_STATE_SIZE (SAM_HASH_SIZE * 3)
#define SAM_MAGIC_NUMBER 13
#define HIT_LIMIT 7625597484987 // 3 ** 27
#define MAX_INPUT_SIZE 4096

struct identity_info { uint64_t nxt64bits; uint32_t activewt,passivewt; } Identities[1000];
#define MAX_IDENTITIES ((int32_t)(sizeof(Identities)/sizeof(*Identities)))
#define MIN_INDENTITIES 2
#define MIN_SMALLWORLDPEERS 5
#define PUZZLE_DURATION 15
#define PUZZLE_THRESHOLD (HIT_LIMIT / 1337)
// oh, just recalled, use divisors that are powers of 3

int32_t Num_identities = 0;
int SAM_INDICES[SAM_STATE_SIZE];
struct SAM_STATE { TRIT trits[SAM_STATE_SIZE]; };

TRIT SaM_Any(const TRIT a,const TRIT b) { return a == b ? a : (a + b); }
TRIT SaM_Sum(const TRIT a,const TRIT b) { return a == b ? -a : (a + b); }

int32_t ConvertToTrits(const uint8_t *input,const uint32_t inputSize,TRIT * output)
{
    int32_t i;
	for (i=0; i<(inputSize << 3); i++)
		*output++ = (input[i >> 3] & (1 << (i & 7))) != 0;
    return(i);
}

int32_t ConvertToBytes(const TRIT *input,const uint32_t inputSize,uint8_t *output)
{
    int32_t i;
	for (i=0; i<(inputSize >> 2); i++)
		*output++ = (input[i << 2] + 1) | (((input[(i << 2) + 1] + 1)) << 2) | (((input[(i << 2) + 2] + 1)) << 4) | (((input[(i << 2) + 3] + 1)) << 6);
	switch ( (inputSize & 3) )
    {
        case 0: i--;
            break;
        case 1:
            *output++ = (input[(inputSize >> 2) << 2] + 1);
            break;
        case 2:
            *output++ = (input[(inputSize >> 2) << 2] + 1) | (((input[((inputSize >> 2) << 2) + 1] + 1)) << 2);
            break;
        case 3:
            *output++ = (input[(inputSize >> 2) << 2] + 1) | (((input[((inputSize >> 2) << 2) + 1] + 1)) << 2) | (((input[((inputSize >> 2) << 2) + 2] + 1)) << 4);
            break;
	}
    return(i + 1);
}

void SaM_PrepareIndices()
{
	int32_t i,nextIndex,currentIndex = 0;
	for (i=0; i<SAM_STATE_SIZE; i++)
    {
		nextIndex = ((currentIndex + 1) * SAM_MAGIC_NUMBER) % SAM_STATE_SIZE;
		SAM_INDICES[currentIndex] = nextIndex;
		currentIndex = nextIndex;
	}
    if ( 0 )
    {
        int j;
        TRIT SAMANY[3][3],SAMSUM[3][3];
#define SAM_ANY(a,b) SAMANY[a+1][b+1]
#define SAM_SUM(a,b) SAMSUM[a+1][b+1]
        for (i=0; i<3; i++)
            for (j=0; j<3; j++)
            {
                SAMANY[i][j] = SaM_Any(i-1,j-1);
                SAMSUM[i][j] = SaM_Sum(i-1,j-1);
            }
        for (i=0; i<3; i++)
        {
            printf("{ ");
            for (j=0; j<3; j++)
                printf("%d, ",SAMANY[i][j]);
            printf("}, ");
        }
        printf(" SAMANY\n");
        for (i=0; i<3; i++)
        {
            printf("{ ");
            for (j=0; j<3; j++)
                printf("%d, ",SAMSUM[i][j]);
            printf("}, ");
        }
        printf(" SAMSUM\n");
        getchar();
    }
}

void SaM_SplitAndMerge(struct SAM_STATE *state,int32_t numrounds)
{
    static const char SAMANY[3][3] = { { -1, -1, 0, }, { -1, 0, 1, }, { 0, 1, 1, } };
    static const char SAMSUM[3][3] = { { 1, -1, 0, }, { -1, 0, 1, }, { 0, 1, -1, } };
	struct SAM_STATE leftPart,rightPart;
	int32_t i,round,nextIndex,ind,currentIndex = 0;
    if ( numrounds <= 0 )
        return;
	for (round=1; round<=numrounds; round++)
    {
		for (i=0; i<SAM_STATE_SIZE; i++)
        {
			nextIndex = SAM_INDICES[currentIndex];
			//leftPart.trits[i] = SaM_Any(state->trits[currentIndex],-state->trits[nextIndex]);
			//rightPart.trits[i] = SaM_Any(-state->trits[currentIndex],state->trits[nextIndex]);
			leftPart.trits[i] = SAMANY[state->trits[currentIndex] + 1][-state->trits[nextIndex] + 1];
			rightPart.trits[i] = SAMANY[-state->trits[currentIndex] + 1][state->trits[nextIndex] + 1];
			currentIndex = nextIndex;
		}
        ind = round;
		for (i=0; i<SAM_STATE_SIZE; i++,ind++)
        {
            if ( ind >= SAM_STATE_SIZE )
                ind = 0;
			nextIndex = SAM_INDICES[currentIndex];
			//state->trits[(i + round) % SAM_STATE_SIZE] = SaM_Sum(leftPart.trits[currentIndex],rightPart.trits[nextIndex]);
			state->trits[ind] = SAMSUM[leftPart.trits[currentIndex]+1][rightPart.trits[nextIndex]+1];
			currentIndex = SAM_INDICES[nextIndex];
		}
	}
}

void SaM_Initialize(struct SAM_STATE *state)
{
    int32_t i;
	for (i=SAM_HASH_SIZE; i<SAM_STATE_SIZE; i++)
		state->trits[i] = ((i & 1) != 0) ? TRIT_FALSE : TRIT_TRUE;
}

void SaM_Absorb(struct SAM_STATE *state,const TRIT *input,uint32_t inputSize,int32_t numrounds)
{
    int32_t i,n,offset = 0;
    if ( inputSize > MAX_INPUT_SIZE )
        inputSize = MAX_INPUT_SIZE;
    n = (inputSize / SAM_HASH_SIZE);
	for (i=0; i<n; i++,offset+=SAM_HASH_SIZE)
    {
		memcpy(state->trits,&input[offset],SAM_HASH_SIZE * sizeof(TRIT));
		SaM_SplitAndMerge(state,numrounds);
	}
	if ( (i= (inputSize % SAM_HASH_SIZE)) != 0 )
    {
		memcpy(state->trits,&input[n * SAM_HASH_SIZE],i * sizeof(TRIT));
		for (; i<SAM_HASH_SIZE; i++)
			state->trits[i] = ((i & 1) != 0) ? TRIT_FALSE : TRIT_TRUE;
		SaM_SplitAndMerge(state,numrounds);
	}
}

void SaM_Squeeze(struct SAM_STATE *state,TRIT *output,int32_t numrounds)
{
	SaM_SplitAndMerge(state,(numrounds < SAM_MAGIC_NUMBER) ? 1 : numrounds);
	memcpy(output,state->trits,SAM_HASH_SIZE * sizeof(TRIT));
	SaM_SplitAndMerge(state,(numrounds < SAM_MAGIC_NUMBER) ? 1 : numrounds);
}

uint64_t Hit(const uint8_t *input,int32_t inputSize,int32_t numrounds)
{
    TRIT inputTrits[MAX_INPUT_SIZE << 3];
  	TRIT hash[SAM_HASH_SIZE];
    struct SAM_STATE state;
    uint64_t hit = 0;
    int32_t i,numtrits;
    if ( inputSize > MAX_INPUT_SIZE )
        inputSize = MAX_INPUT_SIZE;
	numtrits = ConvertToTrits(input,inputSize,inputTrits);
	SaM_Initialize(&state);
	SaM_Absorb(&state,inputTrits,inputSize << 3,numrounds);
	SaM_Squeeze(&state,hash,numrounds);
	for (i=0; i<27; i++)
 		hit = (hit * 3 + hash[i] + 1);
	return(hit);
}

//////////////////////////////

struct identity_info *find_identity(const uint64_t nxt64bits)
{
    int32_t i;
	for (i=0; i<Num_identities; i++)
		if ( Identities[i].nxt64bits == nxt64bits )
			return(&Identities[i]);
    return(0);
}

void Add(const uint64_t nxt64bits)
{
    struct identity_info *ident;
    int32_t ind;
    if ( Global_mp->insmallworld == 0 && (ident= find_identity(nxt64bits)) == 0 )
    {
        if ( (ind= Num_identities) < MAX_IDENTITIES )
            Num_identities++;
        else ind = ((rand() >> 8) % MAX_IDENTITIES);
        ident = &Identities[ind];
        memset(ident,0,sizeof(*ident));
        ident->nxt64bits = nxt64bits;
    }
}

void Remove(const uint64_t nxt64bits)
{
    struct identity_info *ident;
    if ( (ident= find_identity(nxt64bits)) != 0 )
        *ident = Identities[--Num_identities];
}

uint32_t ActiveWeight(const uint64_t nxt64bits)
{
    struct identity_info *ident;
    if ( (ident= find_identity(nxt64bits)) != 0 )
        return(ident->activewt);
	return(0);
}

uint32_t PassiveWeight(const uint64_t nxt64bits)
{
    struct identity_info *ident;
    if ( (ident= find_identity(nxt64bits)) != 0 )
        return(ident->passivewt);
	return(0);
}

uint32_t TotalActiveWeight(uint32_t *nonzp)
{
	uint32_t i,nonz,wt,totalActiveWeight = 0;
	for (i=nonz=0; i<Num_identities; i++)
        if ( (wt= Identities[i].activewt) != 0 )
            totalActiveWeight += wt, nonz++;
    if ( nonzp != 0 )
        *nonzp = nonz;
	return(totalActiveWeight);
}

uint32_t TotalPassiveWeight(uint32_t *nonzp)
{
	uint32_t i,nonz,wt,totalPassiveWeight = 0;
	for (i=nonz=0; i<Num_identities; i++)
        if ( (wt= Identities[i].passivewt) != 0 )
            totalPassiveWeight += wt, nonz++;
    if ( nonzp != 0 )
        *nonzp = nonz;
	return(totalPassiveWeight);
}

int32_t GetNeighbors(uint64_t *neighbors,int32_t maxNeighbors)
{
	uint32_t weights[MAX_IDENTITIES],i,hit,n,nonz,totalWeight,counter = 0;
    for (i=0; i<Num_identities; i++)
        weights[i] = Identities[i].activewt;
    if ( (totalWeight= TotalActiveWeight(&nonz)) == 0 )
		for (i=0; i<Num_identities; i++)
			weights[i] = 1, totalWeight++;
    n = Num_identities;
	while ( counter < maxNeighbors && totalWeight > 0 )
    {
        hit = ((rand() >> 8) % totalWeight);
		for (i=0; i<n; i++)
        {
			if ( weights[i] > hit )
            {
				totalWeight -= weights[i];
				neighbors[counter++] = Identities[i].nxt64bits;
				weights[i] = 0;
				break;
			}
			hit -= weights[i];
		}
	}
    return(counter);
}

long set_sambuf(uint8_t *data,uint32_t puzzletime,uint64_t challenger,uint64_t destbits)
{
    long offset = 0;
    memcpy(&data[offset],&puzzletime,sizeof(puzzletime)), offset += sizeof(puzzletime);
    memcpy(&data[offset],&challenger,sizeof(challenger)), offset += sizeof(challenger);
    memcpy(&data[offset],&destbits,sizeof(destbits)), offset += sizeof(destbits);
    return(offset);
}

char *challenge_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    int32_t numrounds = SAM_MAGIC_NUMBER;
    uint32_t n,len,num,duration,reftime,now = (uint32_t)time(NULL);
    char buf[MAX_JSON_FIELD],hopNXTaddr[64],numstr[64],*jsonstr;
    uint64_t threshold,senderbits,nonce,hit;
    uint8_t data[64];
    long offset;
    cJSON *json,*array;
    struct pserver_info *pserver;
    double endmilli;
    if ( is_remote_access(previpaddr) == 0 )
        return(clonestr("{\"error\":\"puzzles have to be from remote\"}"));
    senderbits = calc_nxt64bits(sender);
    pserver = get_pserver(0,previpaddr,0,0);
    if ( pserver->nxt64bits != senderbits )
        return(clonestr("{\"error\":\"puzzletime from mismatched IP\"}"));
    reftime = (uint32_t)get_API_int(objs[0],0);
    duration = (uint32_t)get_API_int(objs[1],0);
    threshold = get_API_nxt64bits(objs[2]);
    endmilli = (milliseconds() + duration*1000);
    if ( 0 && abs(reftime - now) > 3 )
    {
        printf("sender.%s via previp.(%s) reftime.%u vs now.%u too different\n",sender,previpaddr,reftime,now);
        return(clonestr("{\"error\":\"puzzletime variance\"}"));
    }
    offset = set_sambuf(data,reftime,senderbits,calc_nxt64bits(NXTaddr));
    num = n = 0;
    array = 0;
    while ( milliseconds() < endmilli )
    {
        //for (i=0; i<8; i++)
        //    data[offset+i] = (rand() >> 8) & 0xff;
        //memcpy(&nonce,&data[offset],sizeof(nonce));
        randombytes((uint8_t *)&nonce,sizeof(nonce));
        memcpy(&data[offset],&nonce,sizeof(nonce));
        hit = Hit(data,(uint32_t)(offset + sizeof(nonce)),numrounds);
        //if ( (rand() % 1000) == 0 )
            //printf("%llu vs %llu\n",(long long)hit,(long long)threshold);
        if ( hit < threshold )
        {
            if ( array == 0 )
                array = cJSON_CreateArray();
            sprintf(numstr,"%llu",(long long)nonce);
            cJSON_AddItemToArray(array,cJSON_CreateString(numstr));
            printf("found.%d nonce.%llu hit.%llu for %llu remaining %.2f seconds\n",cJSON_GetArraySize(array),(long long)nonce,(long long)hit,(long long)sender,(endmilli - milliseconds())/1000.);
            if ( cJSON_GetArraySize(array) > 32 ) // stay within 1kb can switch to binary to get more capacity
                break;
        }
        n++;
    }
    printf("hashes.%d for duration.%d = %.1f per second\n",n,duration,(double)n/duration);
    if ( array != 0 )
    {
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json,"requestType",cJSON_CreateString("nonces"));
        cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(NXTaddr));
        cJSON_AddItemToObject(json,"reftime",cJSON_CreateNumber(reftime));
        sprintf(numstr,"%llu",(long long)threshold), cJSON_AddItemToObject(json,"threshold",cJSON_CreateString(numstr));
        cJSON_AddItemToObject(json,"nonces",array);
        jsonstr = cJSON_Print(json);
        free_json(json);
        stripwhite_ns(jsonstr,strlen(jsonstr));
        if ( (len= (int32_t)strlen(jsonstr)) < 1024 )
        {
            strcpy(buf,jsonstr);
            free(jsonstr);
            hopNXTaddr[0] = 0;
            return(send_tokenized_cmd(!prevent_queueing("nonces"),hopNXTaddr,0,NXTaddr,NXTACCTSECRET,buf,sender));
        } else printf("challeng_func: len.%d too big\n",len);
        free(jsonstr);
        return(clonestr("{\"result\":\"too many nonces\"}"));
    }
    return(clonestr("{\"result\":\"i cant get no noncifaction\"}"));
}

char *response_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    struct identity_info *ident;
    uint32_t i,n,reftime,numrounds = SAM_MAGIC_NUMBER;
    uint8_t data[64];
    char buf[1024];
    long offset = 0;
    uint64_t nonce,mybits,otherbits,threshold,hit;
    cJSON *array;
    printf("nonces: %u %llu\n",Global_mp->puzzletime,(long long)Global_mp->puzzlethreshold);
    if ( is_remote_access(previpaddr) == 0 )
        return(clonestr("{\"error\":\"nonces have to be from remote\"}"));
    if ( Global_mp->insmallworld != 0 )
        return(clonestr("{\"error\":\"already in small world\"}"));
    reftime = (uint32_t)get_API_int(objs[0],0);
    threshold = get_API_nxt64bits(objs[1]);
    //if ( Global_mp->puzzletime == reftime && Global_mp->puzzlethreshold == threshold )
    {
        array = objs[2];
        mybits = calc_nxt64bits(NXTaddr);
        otherbits = calc_nxt64bits(sender);
        if ( (ident= find_identity(otherbits)) != 0 )
        {
            ident->activewt = 0;
            offset = set_sambuf(data,reftime,mybits,otherbits);
            if ( is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    nonce = get_API_nxt64bits(cJSON_GetArrayItem(array,i));
                    memcpy(&data[offset],&nonce,sizeof(nonce));
                    hit = Hit(data,(uint32_t)(offset + sizeof(nonce)),numrounds);
                    printf("got.%d nonce.%llu hit.%llu for %llu\n",i,(long long)nonce,(long long)hit,(long long)sender);
                    if ( hit < threshold )
                        ident->activewt++;
                    else
                    {
                        sprintf(buf,"{\"error\":\"hit.%llu >= threshold.%llu from %s\"}",(long long)hit,(long long)threshold,sender);
                        ident->activewt = 0;
                        return(clonestr(buf));
                    }
                }
                return(clonestr("{\"result\":\"processed nonces\"}"));
            } else return(clonestr("{\"error\":\"no nonces\"}"));
        }
        else return(clonestr("{\"error\":\"unexpected nonces from stranger\"}"));
    }
    //else return(clonestr("{\"error\":\"unexpected nonce when not puzzling or mismatched threshold\"}"));
}

void enter_smallworld(int32_t duration,uint64_t threshold)
{
    char hopNXTaddr[64],destNXTaddr[64],jsonstr[1024],*str;
    uint32_t i,n = Num_identities;
    Global_mp->puzzletime = (uint32_t)time(NULL);
    Global_mp->puzzlethreshold = threshold;
    sprintf(jsonstr,"{\"requestType\":\"puzzles\",\"NXT\":\"%s\",\"reftime\":\"%lu\",\"duration\":\"%d\",\"threshold\":\"%llu\"}",Global_mp->myNXTADDR,time(NULL),duration,(long long)threshold);
    for (i=0; i<n; i++)
    {
        hopNXTaddr[0] = 0;
        expand_nxt64bits(destNXTaddr,Identities[i].nxt64bits);
        if ( (str= send_tokenized_cmd(!prevent_queueing("puzzles"),hopNXTaddr,0,Global_mp->myNXTADDR,Global_mp->srvNXTACCTSECRET,jsonstr,destNXTaddr)) != 0 )
            free(str);
        msleep(25); // prevent saturating UDP output
    }
    Global_mp->endpuzzles = milliseconds() + (duration + 3) * 1000;
    printf("sent puzzles to %d nodes\n",i);
}

int32_t poll_smallworld(int32_t minpeers)
{
    uint32_t i,nonz = 0,wt = 0;
    if ( milliseconds() < Global_mp->endpuzzles )
    {
        wt = TotalActiveWeight(&nonz);
        printf("Total Active Weight: %d from %d\n",wt,nonz);
        return(-1);
    }
    Global_mp->puzzletime = 0;
    Global_mp->puzzlethreshold = 0;
    Global_mp->insmallworld = 0;
    if ( nonz >= minpeers )
    {
        if ( Global_mp->neighbors != 0 )
            free(Global_mp->neighbors);
        Global_mp->neighbors = calloc(minpeers,sizeof(*Global_mp->neighbors));
        Global_mp->insmallworld = GetNeighbors(Global_mp->neighbors,minpeers);
        for (i=0; i<Global_mp->insmallworld; i++)
            printf("%llu ",(long long)Global_mp->neighbors[i]);
        printf("small world peers.%d\n",Global_mp->insmallworld);
    }
    return(Global_mp->insmallworld);
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
    {
        Add(calc_nxt64bits(NXTaddr));
        modified = (update_nodestats(NXTaddr,now,stats,encryptedflag,p2pflag,pubkey,ipaddr,port) << 1);
    }
    return(modified);
}

void every_second(int32_t counter)
{
    static int sendsock = -100;
    uint64_t send_kademlia_cmd(uint64_t nxt64bits,struct pserver_info *pserver,char *kadcmd,char *NXTACCTSECRET,char *key,char *datastr);
    static double firstmilli;
    char *ip_port;
    struct coin_info *cp = get_coin_info("BTCD");
    struct pserver_info *pserver;
    int32_t i,gatewayid,err;
    if ( sendsock == -100 )
    {
        char *testaddr = "inproc://test";
        if ( (sendsock= nn_socket(AF_SP,NN_BUS)) < 0 )
            printf("error %d nn_socket err.%s\n",sendsock,nn_strerror(nn_errno()));
        else if ( (err= nn_connect(sendsock,testaddr)) < 0 )
            printf("error %d nn_connect err.%s (%s)\n",sendsock,nn_strerror(nn_errno()),testaddr);
        else printf("test >>>>>>>>>>>>>>> %d nn_connect (%s)\n",sendsock,testaddr);
    }
    if ( 0 && sendsock >= 0 )
    {
        printf("send\n");
        nn_send(sendsock,"hello",6,0);
    }
    if ( Finished_init == 0 )
        return;
    if ( firstmilli == 0 )
        firstmilli = milliseconds();
    if ( milliseconds() < firstmilli+5000 )
        return;
    if ( (ip_port= queue_dequeue(&P2P_Q,1)) != 0 )
    {
        add_SuperNET_peer(ip_port);
        free_queueitem(ip_port);
    }
    if ( (counter % 10) == 0 && Global_mp->gatewayid >= 0 )
    {
        if ( 0 && Global_mp->insmallworld == 0 && Num_identities >= MIN_INDENTITIES )
            enter_smallworld(PUZZLE_DURATION,PUZZLE_THRESHOLD);
        else if ( Global_mp->puzzletime != 0 )
            poll_smallworld(MIN_SMALLWORLDPEERS);
        if ( Global_mp->gatewayid < 0 )
        {
            for (i=0; i<Num_in_whitelist; i++)
            {
                expand_ipbits(ip_port,SuperNET_whitelist[i]);
                pserver = get_pserver(0,ip_port,0,0);
                send_kademlia_cmd(0,pserver,"ping",cp->srvNXTACCTSECRET,0,0);
            }
        }
        else
        {
            for (gatewayid=0; gatewayid<=NUM_GATEWAYS; gatewayid++)
                if ( gatewayid != Global_mp->gatewayid )
                {
                    if ( Server_ipaddrs[gatewayid][0] != 0 )
                    {
                        pserver = get_pserver(0,Server_ipaddrs[gatewayid],0,0);
                        send_kademlia_cmd(0,pserver,"ping",cp->srvNXTACCTSECRET,0,0);
                    }
                }
        }
    }
}

#endif
