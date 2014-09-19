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

static long server_xferred;
int Servers_started;

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

// UDP funcs
int32_t portable_udpwrite(const struct sockaddr *addr,uv_udp_t *handle,void *buf,long len,int32_t allocflag)
{
    int32_t r;
    struct write_req_t *wr;
    wr = alloc_wr(buf,len,allocflag);
    ASSERT(wr != NULL);
    {
        char destip[64]; int32_t port;
        port = extract_nameport(destip,sizeof(destip),(struct sockaddr_in *)addr);
        printf("portable_udpwrite %ld bytes to %s/%d\n",len,destip,port);
    }
    r = uv_udp_send(&wr->U.ureq,handle,&wr->buf,1,addr,(uv_udp_send_cb)after_write);
    if ( r != 0 )
        printf("uv_udp_send error.%d %s wr.%p wreq.%p %p len.%ld\n",r,uv_err_name(r),wr,&wr->U.ureq,buf,len);
    return(r);
}

void on_udprecv(uv_udp_t *udp,ssize_t nread,const uv_buf_t *rcvbuf,const struct sockaddr *addr,unsigned flags)
{
    uint16_t port;
    struct NXT_acct *np;
    struct coin_info *cp = get_coin_info("BTCD");
    char sender[256],retjsonstr[4096],NXTaddr[64],*retstr;
    retjsonstr[0] = 0;
    printf("UDP RECEIVED\n");
    if ( cp != 0 && nread > 0 )
    {
        expand_nxt64bits(NXTaddr,cp->pubnxt64bits);
        strcpy(sender,"unknown");
        port = extract_nameport(sender,sizeof(sender),(struct sockaddr_in *)addr);
        np = process_packet(retjsonstr,(unsigned char *)rcvbuf->base,(int32_t)nread,udp,(struct sockaddr *)addr,sender,port);
        ASSERT(addr->sa_family == AF_INET);
        if ( np != 0 )
        {
            if ( retjsonstr[0] != 0 )
            {
                printf("%s send tokenized.(%s) to %s\n",NXTaddr,retjsonstr,np->H.NXTaddr);
                if ( (retstr= send_tokenized_cmd(Global_mp->Lfactor,NXTaddr,cp->NXTACCTSECRET,retjsonstr,np->H.NXTaddr)) != 0 )
                {
                    printf("sent back via UDP.(%s) got (%s) free.%p\n",retjsonstr,retstr,retstr);
                    free(retstr);
                }
            }
         }
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

#endif
