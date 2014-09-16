//
//  NXTprivacy.h
//  gateway
//
//  Created by jl777 on 7/4/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef gateway_NXTprivacy_h
#define gateway_NXTprivacy_h

#define ALLOCWR_DONTFREE 0
#define ALLOCWR_ALLOCFREE 1
#define ALLOCWR_FREE -1

#define IS_TCP_SERVER 1
#define IS_UDP_SERVER 2
#define DEFAULT_PRIVACY_SERVER "123456789012345678"
#define DEFAULT_PRIVACY_SERVERIP "127.0.0.1"
#define INTRO_SIZE 1400

//char *Server_secret,*Server_NXTaddr;
queue_t RPC_6777_response,ALL_messages;
char *sendmessage(int32_t L,char *verifiedNXTaddr,char *NXTACCTSECRET,char *msg,int32_t msglen,char *destNXTaddr,char *origargstr);

struct pNXT_info
{
    void **coinptrs;
    char privacyServer_NXTaddr[64],privacyServer_ipaddr[32],privacyServer_port[16];
    uint64_t privacyServer;
    struct hashtable **orderbook_txidsp,*msg_txids;
};
struct pNXT_info *Global_pNXT;
#include "packets.h"
uint64_t broadcast_publishpacket(uint64_t corecoins[4],struct NXT_acct *np,char *NXTACCTSECRET,char *srvNXTaddr,char *srvipaddr,uint16_t srvport);

typedef struct {
    union { uv_udp_send_t ureq; uv_write_t req; };
    uv_buf_t buf;
    int32_t allocflag;
} write_req_t;

int server_address(char *server,int port,struct sockaddr_in *addr)
{
    struct hostent *hp = gethostbyname(server);
    if ( hp == NULL )
    {
        perror("gethostbyname");
        return -1;
    }
    addr->sin_port = htons(port);
    addr->sin_family = AF_INET;
    memcpy(&addr->sin_addr.s_addr, hp->h_addr, hp->h_length);
    return(0);
}

uint64_t calc_privacyServer(char *ipaddr,uint16_t port)
{
    if ( ipaddr == 0 || ipaddr[0] == 0 )
        return(0);
    if ( port == 0 )
        port = NXT_PUNCH_PORT;
    return(calc_ipbits(ipaddr) | ((uint64_t)port << 32));
}

uint16_t extract_nameport(char *name,long namesize,struct sockaddr_in *addr)
{
    int32_t r;
    uint16_t port;
    if ( (r= uv_ip4_name(addr,name,namesize)) != 0 )
    {
        fprintf(stderr,"uv_ip4_name error %d (%s)\n",r,uv_err_name(r));
        return(0);
    }
    port = ntohs(addr->sin_port);
    //printf("sender.(%s) port.%d\n",name,port);
    return(port);
}

uint64_t decode_privacyServer(char *jsonstr)
{
    cJSON *json;
    char ipaddr[32],pubkey[256];
    uint16_t port = 0;
    uint32_t ipbits = 0;
    uint64_t privacyServer = 0;
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        if ( extract_cJSON_str(pubkey,sizeof(pubkey),json,"pubkey") > 0 )
        {
            port = get_cJSON_int(json,"port");
            if ( extract_cJSON_str(ipaddr,sizeof(ipaddr),json,"ipaddr") > 0 )
                ipbits = calc_ipbits(ipaddr);
        }
        printf("decode_privacyServer free json.%p\n",json);
        free_json(json);
    }
    if ( port != 0 && ipbits != 0 )
        privacyServer = ipbits | ((uint64_t)port << 32);
    return(privacyServer);
}

/*
 udp multicast of quotes -> all nodes, clients just update books they care about, servers update global
 servers and clients sync orderbooks via crc32 retransmit if discrepancy in ping
 any pending xfer -> AE reduces effective balance
 and pending xfer -> InstantDEX takes effect when it is inside matched block
 
 stats kept on percentage of matched trades are fulfilled
 all nodes calculate virtual orderbooks using NXT arb for asset <-> asset order books
 make sure to update with NXT AE orderbooks!
 
 clients negotiate with potential opposite bidder, invoke SVM, etc, multicast if agreed, but both sides need to verify no pending trades with either party (for asset being traded) via server query. If all good, both multicast and wait for server ping to validate
 
 all nodes update asset balances from pending trades in deterministic order, we enforce 3 second timestamp margin, 1 second for servers
 
 when server gets client's data, it re-timestamps to +/- 1 second and all servers exchange new pending trades and process in timestamp order. Official timestamp for an order is the average server timestamps for that order. If tied, sort by bookid. If still tied, sort by buyerbits, then sellerbits
 
 At t-2, ledger is closed and if any change in balances or pending trades, server pings all its clients
 In server's ping, all info about pending tx and completed tx, client balance changes. So as of cutoff time, all completed trades update ledger.
 
 Servers can be queried for global state, orderbook, client specific, etc.
 Server can accept payments for clientNodes, forward messages, etc.
 
 Privatebet: automatically created virtual assets with "orderbook" display from sportsbook API's and NXTorrent privatebets category
 When user's are matched up, they create 2 of 3 multisig with random privacyServer
 In case privacyServer goes missing, they agree to split if they cant agree on outcome
 
 
 privateNXT is purchased from any server that has an inventory of privateNXT
 There can be a lock added to privateNXT, in this case, the first to provide X such that hash(X) matches lock gets the privateNXT. So the X can be passed around as cash among trusted circle.
 
 Withdraw into InstantDEX credits or deposit from incurs >>= 10 fee
 */



uint64_t *get_possible_privacyServers(int32_t *nump,int32_t max,char *emergency_server)
{ // get list of all tx to PRIVACY_SERVERS acct, scan AM's
    int32_t i,n,m;
    struct json_AM *hdr;
    cJSON *json,*array,*item,*txobj,*attachment,*message,*senderobj;
    char sender[1024],receiver[1024],cmd[1024],txid[128],AMstr[2048],buf[4096],*jsonstr,*txstr;
    uint64_t privacyServer,*privacyServers = calloc(1,max*sizeof(*privacyServers));
    *nump = 0;
    sprintf(cmd,"%s=getAccountTransaction&transaction=%s",NXTSERVER,DEFAULT_PRIVACY_SERVER);
    jsonstr = issue_curl(Global_mp->curl_handle,cmd);
    if ( jsonstr != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"transactions")) != 0 && is_cJSON_Array(array) != 0 )
            {
                n = cJSON_GetArraySize(array);
                for (i=0; i<n; i++)
                {
                    item = cJSON_GetArrayItem(array,i);
                    copy_cJSON(txid,item);
                    if ( txid[0] != 0 )
                    {
                        sprintf(cmd,"%s=getTransaction&transaction=%s",NXTSERVER,txid);
                        txstr = issue_curl(Global_mp->curl_handle,cmd);
                        if ( txstr != 0 )
                        {
                            printf("tx[%d] (%s)\n",i,txstr);
                            txobj = cJSON_Parse(txstr);
                            if ( txobj != 0 )
                            {
                                if ( (attachment= cJSON_GetObjectItem(txobj,"attachment")) != 0 )
                                {
                                    senderobj = cJSON_GetObjectItem(txobj,"sender");
                                    if ( senderobj == 0 )
                                        senderobj = cJSON_GetObjectItem(txobj,"accountId");
                                    else if ( senderobj == 0 )
                                        senderobj = cJSON_GetObjectItem(txobj,"account");
                                    add_NXT_acct(sender,Global_mp,senderobj);
                                    add_NXT_acct(receiver,Global_mp,cJSON_GetObjectItem(txobj,"recipient"));
                                    if ( get_cJSON_int(txobj,"type") == 1 && get_cJSON_int(txobj,"subtype") == 0 )
                                    {
                                        message = cJSON_GetObjectItem(attachment,"message");
                                        if ( message != 0 )
                                        {
                                            copy_cJSON(AMstr,message);
                                            //printf("AM message.(%s).%ld\n",AMstr,strlen(AMstr));
                                            m = (int32_t)strlen(AMstr);
                                            if ( (m&1) != 0 || m > 2000 )
                                                printf("warning: odd message len?? %ld\n",(long)m);
                                            decode_hex((void *)buf,(int32_t)(m>>1),AMstr);
                                            hdr = (struct json_AM *)buf;
                                            printf("txid.%s NXT.%s -> NXT.%s (%s)\n",txid,sender,receiver,hdr->jsonstr);
                                            if ( (privacyServer= decode_privacyServer(hdr->jsonstr)) != 0 )
                                            {
                                                privacyServers[(*nump)++] = privacyServer;
                                            }
                                        }
                                    }
                                }
                                free(txobj);
                            }
                            free(txstr);
                        }
                    }
                    if ( (*nump) >= max )
                        break;
                }
            }
            free_json(json);
        }
        free(jsonstr);
    }
    if ( *nump == 0 )
    {
        privacyServers[0] = calc_ipbits(emergency_server) | ((uint64_t)NXT_PUNCH_PORT << 32);
        *nump = 1;
    }
    privacyServers = realloc(privacyServers,sizeof(*privacyServers) * (*nump));
    return(privacyServers);
}

uint64_t filter_privacyServer(uint64_t privacyServer,char **whitelist,char **blacklist)
{
    // apply white and blacklists
    int32_t i,flag = 0;
    uint64_t nxt64bits;
    if ( whitelist != 0 && whitelist[0] != 0 )
    {
        flag = -1;
        for (i=0; whitelist[i]!=0; i++)
        {
            nxt64bits = calc_nxt64bits(whitelist[i]);
            if ( privacyServer == nxt64bits )
            {
                flag = 1;
                break;
            }
        }
    }
    if ( blacklist != 0 && blacklist[0] != 0 )
    {
        for (i=0; blacklist[i]!=0; i++)
        {
            nxt64bits = calc_nxt64bits(blacklist[i]);
            if ( privacyServer == nxt64bits )
            {
                flag = -1;
                break;
            }
        }
    }
    if ( flag < 0 )
        privacyServer = 0;
    return(privacyServer);
}

uint64_t get_random_privacyServer(char **whitelist,char **blacklist)
{
    int32_t i,n;
    uint64_t privacyServer;
    if ( Global_mp->privacyServers != 0 )
        free(Global_mp->privacyServers);
    Global_mp->privacyServers = get_possible_privacyServers(&Global_mp->numPrivacyServers,1024,EMERGENCY_PUNCH_SERVER);
    n = Global_mp->numPrivacyServers;
    for (i=0; i<n; i++)
    {
        if ( (privacyServer= filter_privacyServer(Global_mp->privacyServers[rand() % n],whitelist,blacklist)) == 0 )
            continue;
        if ( strcmp(EMERGENCY_PUNCH_SERVER,Global_mp->ipaddr) != 0 )
            return(privacyServer);
    }
    return(0);
}

/*int set_intro(char *intro,int size,char *user,char *group,char *NXTaddr,char *NXTACCTSECRET)
{
    gen_tokenjson(0,intro,0,NXTaddr,time(NULL),0,NXTACCTSECRET);
    return(0);
}*/

#include "includes/task.h"
#include <stdio.h>
#include <stdlib.h>

static int TCPserver_closed;
static long server_xferred;

// helper and completion funcs
void portable_alloc(uv_handle_t *handle,size_t suggested_size,uv_buf_t *buf)
{
    buf->base = malloc(suggested_size);
    //printf("portable_alloc %p\n",buf->base);
    buf->len = suggested_size;
}

write_req_t *alloc_wr(void *buf,long len,int32_t allocflag)
{
    // nonzero allocflag means it will get freed on completion
    // negative allocflag means already allocated, dont allocate and copy
    void *ptr;
    write_req_t *wr;
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

void on_close(uv_handle_t *peer)
{
    //printf("on_close free %p\n",peer);
    free(peer);
}

void on_server_close(uv_handle_t *handle)
{
    if ( server_xferred != 0 )
        printf("on_server_close TCPserver_closed.%d %p after xfer %ld\n",TCPserver_closed,handle,server_xferred);
    TCPserver_closed++;
}

void after_server_shutdown(uv_shutdown_t *req,int status)
{
    if ( status != -57 )
        printf("after_server_shutdown %p status.%d %s\n",req->handle,status,uv_err_name(status));
    uv_close((uv_handle_t *)req->handle,on_server_close);
    printf("after_server_shutdown free.%p\n",req);
    free(req);
}

void after_write(uv_write_t *req,int status)
{
    write_req_t *wr;
    // Free the read/write buffer and the request
    if ( status != 0 )
        printf("after write status.%d %s\n",status,uv_err_name(status));
    wr = (write_req_t *)req;
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
    write_req_t *wr;
    wr = alloc_wr(buf,len,allocflag);
    ASSERT(wr != NULL);
    r = uv_udp_send(&wr->ureq,handle,&wr->buf,1,addr,(uv_udp_send_cb)after_write);
    if ( r != 0 )
        printf("uv_udp_send error.%d %s wr.%p wreq.%p %p len.%ld\n",r,uv_err_name(r),wr,&wr->ureq,buf,len);
    return(r);
}

void on_udprecv(uv_udp_t *udp,ssize_t nread,const uv_buf_t *rcvbuf,const struct sockaddr *addr,unsigned flags)
{
    uint16_t port;
    struct NXT_acct *np;
    struct coin_info *cp = get_coin_info("BTCD");
    char sender[256],retjsonstr[4096],intro[4096],*retstr;
    if ( nread > 0 )
    {
        strcpy(sender,"unknown");
        port = extract_nameport(sender,sizeof(sender),(struct sockaddr_in *)addr);
        //sprintf(buf,"buf.%p udp.%p on_udprecv %s/%d nread.%ld flags.%d | total %ld\n",rcvbuf->base,udp,sender,port,nread,flags,server_xferred);
        np = process_packet(retjsonstr,0,1,(unsigned char *)rcvbuf->base,(int32_t)nread,0,(uv_stream_t *)udp,addr,sender,port);
        printf("after process_packet np.%p %p %p sentintro.%d\n",np,cp->srvNXTADDR,cp->srvNXTACCTSECRET,np!=0?np->sentintro:0);
        ASSERT(addr->sa_family == AF_INET);
        if ( cp != 0 && np != 0 && cp->srvNXTADDR[0] != 0 && cp->srvNXTACCTSECRET[0] != 0 )
        {
            if ( np->sentintro == 0 )
            {
                gen_tokenjson(0,intro,cp->srvNXTADDR,time(NULL),cp->srvNXTACCTSECRET,sender,port);
                printf(">>>>>>>> send UDP intro to %s/%d\n",sender,port);
                ASSERT(0 == portable_udpwrite(addr,udp,intro,strlen(intro)+1,ALLOCWR_ALLOCFREE));
                np->sentintro = 1;
            }
            else if ( retjsonstr[0] != 0 )
            {
                printf("%s.%s send tokenized.(%s) to %s\n",cp->srvNXTADDR,cp->srvNXTACCTSECRET,retjsonstr,np->H.NXTaddr);
                if ( 1 && (retstr= send_tokenized_cmd(Global_mp->Lfactor,cp->srvNXTADDR,cp->srvNXTACCTSECRET,retjsonstr,np->H.NXTaddr)) != 0 )//sendmessage(Server_NXTaddr,cp->srvNXTACCTSECRET,retjsonstr,(int32_t)strlen(retjsonstr)+1,np->H.NXTaddr,retjsonstr)) != 0 )
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

void on_client_udprecv(uv_udp_t *udp,ssize_t nread,const uv_buf_t *rcvbuf,const struct sockaddr *addr,unsigned flags)
{
    uint16_t port;
    char sender[64],retjsonstr[4096];
    if ( nread > 0 )
    {
        strcpy(sender,"unknown");
        port = extract_nameport(sender,sizeof(sender),(struct sockaddr_in *)addr);
        //if ( server_xferred == 0 )
            printf("on_client_udprecv %s/%d nread.%ld flags.%d | total %ld\n",sender,port,nread,flags,server_xferred);
        process_packet(retjsonstr,0,0,(unsigned char *)rcvbuf->base,(int32_t)nread,0,(uv_stream_t *)udp,addr,sender,port);
        server_xferred += nread;
        ASSERT(addr->sa_family == AF_INET);
        //ASSERT(0 == portable_udpwrite(addr,handle,rcvbuf->base,nread,ALLOCWR_ALLOCFREE));
    }
    if ( rcvbuf->base != 0 )
    {
        //printf("on_client_udprecv free %p\n",rcvbuf->base);
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
    /*r = uv_udp_set_broadcast(udp,1);
     if ( r != 0 )
     {
     fprintf(stderr,"uv_udp_set_broadcast: %d %s\n",r,uv_err_name(r));
     return(0);
     }*/
    r = uv_udp_recv_start(udp,portable_alloc,on_udprecv);
    if ( r != 0 )
    {
        fprintf(stderr, "uv_udp_recv_start: %d %s\n",r,uv_err_name(r));
        return(0);
    }
    return(udp);
}

int Got_Server_UDP,Servers_started;
void *start_libuv_server(int32_t ip4_or_ip6,int port,void *handler)
{
    void *srv;
    const struct sockaddr *ptr;
    struct sockaddr_in addr;
    struct sockaddr_in6 addr6;
    int r,udpflag = 0;
    if ( ip4_or_ip6 < 0 )
    {
        udpflag = 1;
        ip4_or_ip6 = -ip4_or_ip6;
    }
    ASSERT(0 == uv_ip4_addr("0.0.0.0",port,&addr));
    ASSERT(0 == uv_ip6_addr("::1", port, &addr6));
    if ( ip4_or_ip6 == 4 )
        ptr = (const struct sockaddr *)&addr;
    else if ( ip4_or_ip6 == 6 )
        ptr = (const struct sockaddr *)&addr6;
    else { printf("illegal ip4_or_ip6 %d\n",ip4_or_ip6); return(0); }
    if ( udpflag != 0 )
    {
        srv = open_udp((port > 0) ? (struct sockaddr *)ptr : 0);
        if ( srv != 0 )
            printf("UDP.%p server started on port %d\n",srv,port);
        else printf("couldnt open_udp on port.%d\n",port);
        Servers_started |= 2;
        return(srv);
    }
    srv = malloc(sizeof(uv_tcp_t));
    r = uv_tcp_init(UV_loop,srv);
    if ( r != 0 )
    {
        fprintf(stderr,"Socket creation error %d (%s)\n",r,uv_err_name(r));
        return(0);
    }
    r = uv_tcp_bind(srv,ptr,0);
    if ( r != 0 )
    {
        fprintf(stderr,"Bind error %d (%s)\n",r,uv_err_name(r));
        return(0);
    }
    r = uv_listen(srv,SOMAXCONN,(uv_connection_cb)handler);
    if ( r != 0 )
    {
        fprintf(stderr,"Listen error %d %s\n",r,uv_err_name(r));
        return(0);
    }
    Servers_started |= 1;
    printf("TCP server started on port %d\n",port);
    return(srv);
}

// begin TCP specific
int32_t portable_tcpwrite(uv_stream_t *stream,void *buf,long len,int32_t allocflag)
{
    int32_t r;
    write_req_t *wr;
    wr = alloc_wr(buf,len,allocflag);
    r = uv_write(&wr->req,stream,&wr->buf,1,after_write);
    if ( r != 0 )
        fprintf(stderr,"portable_write error %d %s\n",r,uv_err_name(r));
    //else printf("portable_write.%d %p %ld (%s)\n",allocflag,buf,len,(char *)buf);
    return(r);
}

uint16_t set_peerinfo(struct sockaddr *addr,char *sender,uv_stream_t *tcp)
{
    uint16_t port;
    int32_t addrlen = sizeof(*addr);
    if ( uv_tcp_getpeername((uv_tcp_t *)tcp,addr,&addrlen) == 0 )
    {
        uv_ip4_name((struct sockaddr_in *)addr,sender,16);
        port = ntohs(((struct sockaddr_in *)addr)->sin_port);
        return(port);
    }
    return(0);
}

void after_server_read(uv_stream_t *connect,ssize_t nread,const uv_buf_t *buf)
{
    char sender[64],retjsonstr[4096];
    struct sockaddr addr;
    int32_t port;
    struct NXT_acct *np = 0;
     // uv_shutdown_t *req;
    np = connect->data;
    printf("after read %ld | connect.%p np.%p\n",nread,connect,np);
    if ( nread < 0 ) // Error or EOF
    {
        printf("nread.%ld UV_EOF %d\n",nread,UV_EOF);
        if ( buf->base != 0 )
        {
            printf("after serverread %p\n",buf->base);
            free(buf->base);
        }
        //req = (uv_shutdown_t *)malloc(sizeof *req); causes problems?!
        //uv_shutdown(req,connect,after_shutdown);
        uv_close((uv_handle_t *)connect,on_close);
        clear_NXT_networkinfo(np);
        return;
    }
    if ( nread == 0 )
    {
        // Everything OK, but nothing read.
        printf("after serverreadB %p\n",buf->base);
        free(buf->base);
        return;
    }
    port = set_peerinfo(&addr,sender,connect);
    printf(">>>>>>>>>>>>>>>>>>> got %ld bytes at %p.(%s) from %s/%d\n",nread,buf->base,buf->base,sender,port);
    if ( (np= process_packet(retjsonstr,np,1,(unsigned char *)buf->base,(int32_t)nread,connect,0,&addr,sender,port)) != 0 )
    {
        if ( connect->data == 0 )
            np->sentintro = 0;
        connect->data = np;
    }
    if ( retjsonstr[0] != 0 )
        portable_tcpwrite(connect,retjsonstr,(int32_t)strlen(retjsonstr)+1,ALLOCWR_ALLOCFREE);
    if ( buf->base != 0 )
    {
        printf("after serverreadC %p\n",buf->base);
        free(buf->base);
    }
}

void tcp_client_gotbytes(uv_stream_t *tcp,ssize_t nread,const uv_buf_t *buf)
{
    char sender[32],retjsonstr[4096];
    struct sockaddr addr;
    int32_t addrlen = sizeof(addr);
    struct NXT_acct *np = 0;
    uv_udp_t *udp = 0;
    uv_stream_t *connect = 0;
    uint16_t port = 0;
    if ( (udp= (uv_udp_t *)tcp->data) != 0 )
    {
        connect = (uv_stream_t *)udp->data;
        if ( connect != 0 )
        {
            np = connect->data;
            printf("connect.%p -> %p\n",connect,connect->data);
        }
    }
printf("tcp_client_gotbytes tcp.%p (tcp) data.%p (udp) -> %p (connect)\n",tcp,udp,connect);
    if ( nread < 0 ) // Error or EOF
    {
        printf("Lost contact with Server! np.%p nread.%ld\n",np,nread);
        //ASSERT(nread == UV_EOF);
        if ( buf->base != 0 )
        {
            printf("tcp_client_gotbytes dontfree.%p\n",buf->base);
            //free(buf->base);
        }
        if ( udp != 0 )
        {
            printf("stop and close udp %p\n",udp);
            uv_udp_recv_stop(udp);
            uv_close((uv_handle_t *)udp,on_server_close);
        }
        uv_shutdown(malloc(sizeof(uv_shutdown_t)),tcp,after_server_shutdown);
        clear_NXT_networkinfo(np);
        return;
    }
    else if ( nread == 0 )
    {
        // Everything OK, but nothing read.
    }
    else if ( connect != 0 )
    {
        //port = set_peerinfo(&addr,sender,connect);
        //printf(">>>>>>>>>>>>>>>>>>> got %ld bytes at %p.(%s) from %s/%d\n",nread,buf->base,buf->base,sender,port);
        if ( uv_tcp_getpeername((uv_tcp_t *)tcp,&addr,&addrlen) == 0 )
        {
            uv_ip4_name((struct sockaddr_in *)&addr,sender,16);
            port = ntohs(((struct sockaddr_in *)&addr)->sin_port);
            printf("udp.%p tcp.%p got bytes.%p (connect) >>>>>>>>>>>>>>>>>>> got %ld bytes at %p.(%s) from %s/%d\n",udp,tcp,connect,nread,buf->base,buf->base,sender,port);
        }
        else printf(">>>>>>>>>>>>>>>>>>> got %ld bytes at %p.(%s) from MYSTERY\n",nread,buf->base,buf->base);
        if ( (np= process_packet(retjsonstr,np,0,(unsigned char *)buf->base,(int32_t)nread,tcp,0,&addr,sender,port)) != 0 )
            connect->data = np;
        if ( retjsonstr[0] == 0 )
            sprintf(retjsonstr,"{\"error\":\"no response from process packet\"}");
        queue_enqueue(&RPC_6777_response,clonestr(retjsonstr));
        /*str = malloc(nread+1);
        memcpy(str,buf->base,nread);
        str[nread] = 0;
        if ( connect != 0 && connect->data == 0 )
        {
            if ( (np= process_intro(0,str,0)) != 0 )
            {
                np->connect = connect;
                np->tcp = tcp;
                np->udp = (uv_stream_t *)udp;
                connect->data = (void *)np;
            }
        }
        queue_enqueue(&RPC_6777_response,str);
         */
    }
    if ( buf->base != 0 )
    {
        //printf("tcp_client_gotbytes: free bufbase %p\n",buf->base);
        free(buf->base);
    }
}

void client_connected(uv_stream_t *tcp,int status)
{
    struct sockaddr addr;
    char sender[32];
    int r,port,addrlen = sizeof(addr);
    uv_stream_t *connect;
fprintf(stderr,"client_connected status.%d\n",status);
    if ( status != 0 )
        fprintf(stderr,"Connect error %s\n",uv_err_name(status));
    ASSERT(status == 0);
    connect = calloc(1,sizeof(uv_tcp_t));
    fprintf(stderr,"on_connection %p (connect) status.%d | connect.%p\n",tcp,status,connect);
    ASSERT(connect != NULL);
    r = uv_tcp_init(UV_loop,(uv_tcp_t *)connect);
    ASSERT(r == 0);
    connect->data = 0;
    r = uv_accept(tcp,connect);
    ASSERT(r == 0);
    r = uv_read_start(connect,portable_alloc,after_server_read);
    ASSERT(r == 0);
    if ( 0 && uv_tcp_getpeername((uv_tcp_t *)connect,&addr,&addrlen) == 0 )
    {
        if ( (port= extract_nameport(sender,sizeof(sender),(struct sockaddr_in *)&addr)) == 0 )
        {
            //ASSERT(0 == portable_tcpwrite(connect,sender,strlen(sender)+1,ALLOCWR_ALLOCFREE));
        }
    }
}

void connected_to_server(uv_connect_t *connect,int status)
{
    uv_tcp_t *tcp;
    uv_udp_t *udp;
    uint16_t port;
    struct sockaddr addr;
    int32_t r,addrlen = sizeof(addr);
    char servername[32];
    servername[0] = 0;
    printf("connected_to_server %p tcp.%p\n",connect,connect!=0?connect->handle:0);
    tcp = (uv_tcp_t *)connect->handle;
    if ( status < 0 )
    {
        if ( status != -61 )
            printf("got on_client_connect error status.%d (%s) tcp.%p\n",status,uv_err_name(status),tcp);
        if ( tcp != 0 )
            uv_shutdown(malloc(sizeof(uv_shutdown_t)),(uv_stream_t *)tcp,after_server_shutdown);
        return;
    }
    if ( tcp != 0 && uv_tcp_getpeername(tcp,(struct sockaddr *)&addr,&addrlen) == 0 )
    {
        if ( (port= extract_nameport(servername,sizeof(servername),(struct sockaddr_in *)&addr)) == 0 )
        {
            //printf("send udp to %s/%d\n",servername,port);
            //ASSERT(0 == portable_udpwrite(&addr,tcp->data,servername,strlen(servername)+1,ALLOCWR_ALLOCFREE));
        }
    } else port = 0;
    printf("CONNECTED to %s/%d connect.%p tcp.%p\n",servername,port,connect,tcp);
    if ( tcp != 0 && uv_read_start((void *)tcp,portable_alloc,tcp_client_gotbytes) != 0 )
        printf("uv_read_start failed\n");
    else if ( tcp != 0 && tcp->data == 0 )
    {
        udp = malloc(sizeof(uv_udp_t));
        r = uv_udp_init(UV_loop,udp);
        if ( r != 0 )
        {
            fprintf(stderr, "uv_udp_init: %d %s\n",r,uv_err_name(r));
            printf("connected_to_server: free udp.%p\n",udp);
            free(udp);
            udp = 0;
        }
        r = uv_udp_recv_start(udp,portable_alloc,on_client_udprecv);
        if ( r != 0 )
        {
            fprintf(stderr, "uv_udp_recv_start: %d %s\n",r,uv_err_name(r));
            printf("connected_to_server 2: free udp.%p\n",udp);
            free(udp);
            udp = 0;
        }
        tcp->data = udp;
        udp->data = connect;
    }
}

uv_tcp_t *connect_to_privacyServer(struct sockaddr_in *addr,uv_connect_t **connectp,uint64_t privacyServer)
{
    uint16_t port;
    int32_t r;
    uint32_t ipbits;
    char servername[32];
    uv_tcp_t *tcp = 0;
    ipbits = (uint32_t)privacyServer;
    port = (uint16_t)(privacyServer >> 32);
    expand_ipbits(servername,ipbits);
    if ( server_address(servername,port,addr) < 0 )
        fprintf(stderr, "no address\n");
    else
    {
printf("connect to (%s/%d)\n",servername,port);
        tcp = calloc(1,sizeof(*tcp));
        *connectp = calloc(1,sizeof(**connectp));
        uv_tcp_init(UV_loop,tcp);
        (*connectp)->handle = (uv_stream_t *)tcp;
        if ( (r= uv_tcp_connect(*connectp,tcp,(struct sockaddr *)addr,connected_to_server)) != 0 )
        {
            fprintf(stderr,"no TCP connection to %s/%d err %d %s\n",servername,port,r,uv_err_name(r));
            printf("connected_to_privacyserver: free tcp.%p, connect.%p\n",tcp,*connectp);
            free(tcp);
            free(*connectp);
            tcp = 0;
            *connectp = 0;
        }
    }
    return(tcp);
}

uv_tcp_t *start_libuv_tcp4server(int port,void *onconnection) { return(start_libuv_server(4,port,onconnection)); }
uv_tcp_t *start_libuv_tcp6server(int port,void *onconnection) { return(start_libuv_server(6,port,onconnection)); }
uv_udp_t *start_libuv_udp4server(int port,void *onrecv) { return(start_libuv_server(-4,port,onrecv)); }
uv_udp_t *start_libuv_udp6server(int port,void *onrecv) { return(start_libuv_server(-6,port,onrecv)); }

int32_t start_libuv_servers(uv_tcp_t **tcp,uv_udp_t **udp,int32_t ip4_or_ip6,int32_t port)
{
    if ( ip4_or_ip6 == 6 )
    {
        if ( tcp != 0 )
            *tcp = start_libuv_tcp6server(port,(void *)client_connected);
        if ( udp != 0 )
            *udp = start_libuv_udp6server(port,(void *)on_udprecv);
    }
    else
    {
        if ( tcp != 0 )
            *tcp = start_libuv_tcp4server(port,(void *)client_connected);
        if ( udp != 0 )
            *udp = start_libuv_udp4server(port,(void *)on_udprecv);
    }
    if ( (tcp != 0 && *tcp == 0) || (udp != 0 && *udp == 0) )
    {
        printf("error starting libuv servers %p %p || %p %p\n",tcp,*tcp,udp,*udp);
        return(-1);
    }
    else if ( tcp != 0 && *tcp != 0 && udp != 0 && *udp != 0 )
    {
        (*tcp)->data = *udp;
        (*udp)->data = *tcp;
    }
    return(0);
}

void NXTprivacy_idler(uv_idle_t *handle)
{
    void set_pNXT_privacyServer(uint64_t privacyServer);
    uint64_t get_pNXT_privacyServer(int32_t *activeflagp);
    static char NXTACCTSECRET[256],NXTADDR[64],secret[1024],intro[4096];
    static uv_tcp_t *tcp;
    static uv_connect_t *connect;
    static uint64_t privacyServer,pNXT_privacyServer;
    static int32_t didintro; // numpings,counter
    static struct sockaddr addr;
    static double lastping,lastattempt;
    double millis;
    uv_stream_t *udp;
    struct NXT_acct *np;
    int32_t activeflag;
    uint32_t ipbits,port;
    char servername[64];
    uint64_t nxt64bits;
    char *jsonstr,**whitelist,**blacklist;
    whitelist = blacklist = 0;  // eventually get from config JSON
    void teleport_idler();
    if ( Servers_started != 3 )
        return;
    usleep(5000);
    if ( TCPserver_closed > 0 )
    {
        //printf("idler noticed TCPserver_closed.%d!\n",TCPserver_closed);
        tcp = 0;
        didintro = 0;
        connect = 0;
        Got_Server_UDP = 0;
        pNXT_privacyServer = privacyServer = 0;
        set_pNXT_privacyServer(0);
        TCPserver_closed = 0;
        server_xferred = 0;
        memset(intro,0,sizeof(intro));
        return;
    }
    if ( NXTACCTSECRET[0] == 0 )
    {
        struct coin_info *cp = get_coin_info("BTCD");
        if ( cp != 0 )
            safecopy(NXTACCTSECRET,cp->srvNXTACCTSECRET,sizeof(NXTACCTSECRET));
    }
    millis = ((double)uv_hrtime() / 1000000);
//#ifndef __linux__
    if ( millis > (lastattempt + 500) )
    {
        //printf("Servers_started.%d\n",Servers_started);
        teleport_idler();
        if ( privacyServer == 0 )
            privacyServer = pNXT_privacyServer;// != 0) ? pNXT_privacyServer:get_random_privacyServer(whitelist,blacklist);
        if ( privacyServer != 0 && NXTACCTSECRET[0] != 0 )
        {
            //printf("pNXT %llx vs %llx\n",(long long)pNXT_privacyServer,(long long)privacyServer);
            if ( tcp == 0 || connect == 0 )
            {
                printf("attempt privacyServer %llx %s/%d connect.%p tcp.%p udp.%p\n",(long long)privacyServer,ipbits_str((uint32_t)privacyServer),(int32_t)(privacyServer>>32),connect,tcp,tcp!=0?tcp->data:0);
                tcp = connect_to_privacyServer((struct sockaddr_in *)&addr,&connect,privacyServer);
                if ( tcp == 0 || connect == 0 )
                {
                    tcp = 0;
                    memset(&addr,0,sizeof(addr));
                    connect = 0;
                    privacyServer = 0;
                    return;
                }
                else set_pNXT_privacyServer(privacyServer);
            }
        }
        lastattempt = millis;
        return;
    }
    else
    {
        if ( millis > (lastping+1000) )
        {
            if ( (pNXT_privacyServer= get_pNXT_privacyServer(&activeflag)) != 0 && (pNXT_privacyServer != privacyServer || activeflag == 0) )
            {
                if ( privacyServer != 0 )
                {
                    if ( privacyServer != pNXT_privacyServer )
                    {
                        if ( tcp != 0 )
                        {
                            printf("shutdown wrong server! %llx vs %llx\n",(long long)pNXT_privacyServer,(long long)privacyServer);
                            uv_shutdown(malloc(sizeof(uv_shutdown_t)),(uv_stream_t *)tcp,after_server_shutdown);
                        }
                    }
                }
            }
            if ( secret[0] != 0 )
                strcpy(NXTACCTSECRET,secret);
            if ( tcp != 0 && (udp= tcp->data) != 0 )
            {
                if ( didintro == 0 && NXTACCTSECRET[0] != 0 )
                {
                    nxt64bits = issue_getAccountId(0,NXTACCTSECRET);
                    printf("nxt.%llu\n",(long long)nxt64bits);
                    if ( nxt64bits != 0 )
                    {
                        ipbits = (uint32_t)privacyServer;
                        port = (uint16_t)(privacyServer >> 32);
                        expand_ipbits(servername,ipbits);
                        expand_nxt64bits(NXTADDR,nxt64bits);
                        gen_tokenjson(0,intro,NXTADDR,time(NULL),NXTACCTSECRET,servername,port);
                        memset(NXTACCTSECRET,0,sizeof(NXTACCTSECRET));
                        if ( intro[0] == 0 )
                        {
                            printf("connect_to_privacyServer: invalid intro.(%s), try again\n",intro);
                            return;
                        }
                        portable_tcpwrite((uv_stream_t *)tcp,intro,strlen(intro)+1,ALLOCWR_ALLOCFREE);
                        Got_Server_UDP = 0;
                        portable_udpwrite(&addr,(uv_udp_t *)udp,intro,strlen(intro)+1,ALLOCWR_ALLOCFREE);
                        didintro = 1;
                    }
                }
                if ( (np= (void *)connect->data) != 0 )
                {
                    udp = np->mypeerinfo.udp;
                    if ( udp != 0 )
                    {
                        if ( Got_Server_UDP == 0 )
                            portable_udpwrite(&addr,(uv_udp_t *)udp,intro,strlen(intro)+1,ALLOCWR_ALLOCFREE);
                        //printf("udp.%p send ping.%d total transferred.%ld\n",udp,numpings,server_xferred);
                        //portable_udpwrite(&addr,(uv_udp_t *)udp,"ping",5,ALLOCWR_ALLOCFREE);
                        //numpings++;
                    }
                }
            }
            lastping = millis;
        }
    }
//#endif
    if ( tcp != 0 ) // for clients to send to server
    {// deprecated
        /*if ( (jsonstr= queue_dequeue(&RPC_6777)) != 0 )
        {
            // onionize jl777
            portable_tcpwrite((uv_stream_t *)tcp,jsonstr,strlen(jsonstr)+1,ALLOCWR_FREE);
        }*/
    }
    else
    {
        if ( (jsonstr= queue_dequeue(&ALL_messages)) != 0 )
        {
            printf("SERVER GOTMSG.(%s)\n",jsonstr);
            free(jsonstr);
        }
    }
}

void init_NXTprivacy(void *ptr)
{
    uv_tcp_t *tcp,**tcpptr;
    uv_udp_t *udp,**udpptr;
//#ifdef __linux__
    //uint64_t nxt64bits;
    //if ( (Server_secret= ptr) == 0 )
    //    Server_secret = "password";
    //nxt64bits = issue_getAccountId(0,Server_secret);
    //Server_NXTaddr = calloc(1,64);
    //expand_nxt64bits(Server_NXTaddr,nxt64bits);
    //printf("init_NXTprivacy.(%s) -> %llu (%s)\n",(char *)ptr,(long long)nxt64bits,Server_NXTaddr);
//#endif
    tcp = &Global_mp->Punch_tcp; tcpptr = &tcp;
    udp = &Global_mp->Punch_udp; udpptr = &udp;
    start_libuv_servers(tcpptr,udpptr,4,NXT_PUNCH_PORT);
}


#endif
