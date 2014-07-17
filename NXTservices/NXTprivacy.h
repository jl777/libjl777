//
//  NXTprivacy.h
//  gateway
//
//  Created by jl777 on 7/4/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef gateway_NXTprivacy_h
#define gateway_NXTprivacy_h


#define IS_TCP_SERVER 1
#define IS_UDP_SERVER 2
#define DEFAULT_PRIVACY_SERVER "123456789012345678"
#define DEFAULT_PRIVACY_SERVERIP "209.126.70.170"
#define INTRO_SIZE 1400

char *Server_NXTaddr,*Server_secret;
queue_t RPC_6777,RPC_6777_response,NXTsync_Q,ALL_messages,IntroQ;

typedef struct {
    union { uv_udp_send_t ureq; uv_write_t req; };
    uv_buf_t buf;
    int32_t allocflag;
} write_req_t;

struct orderBook_info
{
    uint64_t assetA,assetB,*serverNXTaddrs;
    uint32_t bookid,revbookid,numservers;
};

struct orderBook_quote
{
    uint64_t assetAfemtos,assetBfemtos,nxt64bits;
    uint32_t bookid,flags;
};

struct privacy_client
{
    uint64_t NXTaddrbits,recentBlocks[3],*balances;
    char BTC_pubkey[100];
    unsigned char pubkey[20]; //jl777 lookup
    int32_t numquotes,timestamps[3],numtrades,numpaids,numassets;
    struct orderBook_quote **quotes; // sorted by bookid, multicast
};

struct signed_trade
{
    uint64_t buyerbits,sellerbits,assetA,assetB,assetAfemto,assetBfemto; // implicit fee >>10
    unsigned char buyertoken[160],sellertoken[160];
    int32_t timestampA,timestampB;
};

struct privateInfo
{
    struct sockaddr_in addr;
    uint64_t privateNXTaddr,credits;
};

struct privacyServer
{
    int32_t numassets,numclients,timestamp,maxheight,matched_height,numforks;
    uint64_t matchedBlocks[16];
    char BTC_pubkey[100];
    unsigned char pubkey[20]; //jl777 lookup
    uint64_t *assetids;
    struct privacy_client **clients;
    struct signed_trade **pendingtrades;
} P;

struct NXTservices_Topology
{
    int32_t numbooks,numservers,timestamp,maxheight,matched_height;
    struct orderBook **books;
    struct privacyServer **servers;
} *Topology;


int32_t is_subscriber(uint64_t nxt64bits)
{
    int32_t i;
    for (i=0; i<P.numclients; i++)
        if ( P.clients[i]->NXTaddrbits == nxt64bits )
            return(i);
    return(-1);
}

void add_subscriber(uint64_t nxt64bits,unsigned char *pubkey)
{
    int32_t ind;
    struct privacy_client *cp;
    if ( (ind= is_subscriber(nxt64bits)) >= 0 )
        cp = P.clients[ind];
    else
    {
        P.clients = (struct privacy_client **)realloc(P.clients,(P.numclients+1) * sizeof(*P.clients));
        cp = (struct privacy_client *)calloc(1,sizeof(*cp));
        cp->NXTaddrbits = nxt64bits;
        P.clients[P.numclients++] = cp;
    }
    if ( memcmp(cp->pubkey,pubkey,sizeof(cp->pubkey)) != 0 )
    {
        printf("update pubkey for NXT.%llu: %llx\n",(long long)nxt64bits,*(long long *)pubkey);
        memcpy(cp->pubkey,pubkey,sizeof(cp->pubkey));
    }
}

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
        free_json(json);
    }
    if ( port != 0 && ipbits != 0 )
        privacyServer = ipbits | ((uint64_t)port << 32);
    return(privacyServer);
}

int _encode_str(unsigned char *cipher,char *str,int size,unsigned char *destpubkey,unsigned char *myprivkey)
{
    long len;
    unsigned char buf[4096],*nonce = cipher;
    randombytes(nonce,crypto_box_NONCEBYTES);
    cipher += crypto_box_NONCEBYTES;
    len = size;//strlen(str) + 1;
    //printf("len.%ld -> %ld %ld\n",len,len+crypto_box_ZEROBYTES,len + crypto_box_ZEROBYTES + crypto_box_NONCEBYTES);
    memset(cipher,0,len+crypto_box_ZEROBYTES);
    memset(buf,0,crypto_box_ZEROBYTES);
    memcpy(buf+crypto_box_ZEROBYTES,str,len);
    /*int i;
     for (i=0; i<crypto_box_PUBLICKEYBYTES; i++)
     printf("%02x",destpubkey[i]);
     printf(" destpubkey\n");
     for (i=0; i<crypto_box_SECRETKEYBYTES; i++)
     printf("%02x",myprivkey[i]);
     printf(" myprivkey\n");
     for (i=0; i<crypto_box_NONCEBYTES; i++)
     printf("%02x",nonce[i]);
     printf(" nonce\n");*/
    crypto_box(cipher,buf,len+crypto_box_ZEROBYTES,nonce,destpubkey,myprivkey);
    return((int)len + crypto_box_ZEROBYTES + crypto_box_NONCEBYTES);
}

int _decode_cipher(char *str,unsigned char *cipher,int32_t *lenp,unsigned char *srcpubkey,unsigned char *myprivkey)
{
    int i,err,len = *lenp;
    unsigned char *nonce = cipher;
    cipher += crypto_box_NONCEBYTES, len -= crypto_box_NONCEBYTES;
    /*int i;
     for (i=0; i<crypto_box_PUBLICKEYBYTES; i++)
     printf("%02x",srcpubkey[i]);
     printf(" destpubkey\n");
     for (i=0; i<crypto_box_SECRETKEYBYTES; i++)
     printf("%02x",myprivkey[i]);
     printf(" myprivkey\n");
     for (i=0; i<crypto_box_NONCEBYTES; i++)
     printf("%02x",nonce[i]);
     printf(" nonce\n");*/
    err = crypto_box_open((unsigned char *)str,cipher,len,nonce,srcpubkey,myprivkey);
    for (i=0; i<len-crypto_box_ZEROBYTES; i++)
        str[i] = str[i+crypto_box_ZEROBYTES];
    //for (i=0; i<len;i++)
    //    printf("%02x",str[i]);
    // printf("Err.%d\n",err);
    *lenp = len - crypto_box_ZEROBYTES;
    return(err);
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

struct NXT_acct *process_intro(uv_stream_t *handle,char *bufbase,int32_t sendresponse)
{
    int32_t portable_tcpwrite(uv_stream_t *stream,void *buf,long len,int32_t allocflag);
    int32_t n,retcode,createdflag;
    char retbuf[1024],pubkey[256],NXTaddr[64],name[64];
    cJSON *argjson = 0;
    struct NXT_acct *np = 0;
    NXTaddr[0] = pubkey[0] = name[0] = 0;
    if ( (retcode= validate_token(0,&argjson,pubkey,bufbase,NXTaddr,name,15)) > 0 )
    {
        if ( argjson != 0 )
            free_json(argjson);
        np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
        if ( np != 0 )
        {
            safecopy(np->dispname,name,sizeof(np->dispname));
            n = decode_hex(np->pubkey,(int32_t)strlen(pubkey)/2,pubkey);
            if ( n == crypto_box_PUBLICKEYBYTES )
            {
                printf("created.%d NXT.%s pubkey.%s (len.%d) name.%s\n",createdflag,NXTaddr,pubkey,n,name);
                if ( sendresponse != 0 )
                {
                    printf("call set_intro %s.(%s)\n",Server_NXTaddr,Server_secret);
                    //gen_tokenjson(0,retbuf,Server_NXTaddr,time(NULL),Server_secret);
                    printf("got (%s)\n",retbuf);
                    if ( 1 || retbuf[0] == 0 )
                        printf("error generating intro??\n");
                    else
                       portable_tcpwrite(handle,retbuf,(int32_t)strlen(retbuf)+1,1);
                    printf("after tcpwrite to %p (%s)\n",handle,retbuf);
                }
            } else np = 0;
        }
    }
    else
    {
        if ( sendresponse != 0 )
        {
            sprintf(retbuf,"{\"error\":\"token validation error.%d\"}",retcode);
            portable_tcpwrite(handle,retbuf,(int32_t)strlen(retbuf)+1,1);
        }
        np = 0;
    }
    return(np);
}

#include "libuv/test/task.h"
#include <stdio.h>
#include <stdlib.h>

static int TCPserver_closed;
static long server_xferred;

// helper and completion funcs
void portable_alloc(uv_handle_t *handle,size_t suggested_size,uv_buf_t *buf)
{
    buf->base = malloc(suggested_size);
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
    if ( allocflag == 1 )
    {
        ptr = malloc(len);
        memcpy(ptr,buf,len);
    } else ptr = buf;
    wr->buf = uv_buf_init(ptr,(int32_t)len);
    return(wr);
}

void on_close(uv_handle_t *peer)
{
    printf("on_close %p\n",peer);
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
    free(req);
}

void after_write(uv_write_t *req,int status)
{
    write_req_t *wr;
    // Free the read/write buffer and the request
    if ( status != 0 )
        printf("after write status.%d %s\n",status,uv_err_name(status));
    wr = (write_req_t *)req;
    if ( wr->allocflag != 0 )
        free(wr->buf.base);
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
    r = uv_udp_send(&wr->ureq,handle,&wr->buf,1,addr,(void *)after_write);
    if ( r != 0 )
        printf("uv_udp_send error.%d %s wr.%p wreq.%p %p len.%ld\n",r,uv_err_name(r),wr,&wr->ureq,buf,len);
    return(r);
}

void on_udprecv(uv_udp_t *handle,ssize_t nread,const uv_buf_t *rcvbuf,const struct sockaddr *addr,unsigned flags)
{
    uint16_t port;
    char sender[256];
    struct NXT_acct *np = 0;
    if ( nread > 0 )
    {
        strcpy(sender,"unknown");
        port = extract_nameport(sender,sizeof(sender),(struct sockaddr_in *)addr);
        printf("buf.%p udp.%p on_udprecv %s/%d nread.%ld flags.%d | total %ld\n",rcvbuf->base,handle,sender,port,nread,flags,server_xferred);
        //rcvbuf->base[nread] = 0;
        if ( (np= process_intro(0,(char *)rcvbuf->base,0)) != 0 )
        {
            //printf("got np.%p\n",np);
            np->Uaddr = *addr;
            np->udp = (uv_stream_t *)handle;
        }
        //printf("send back ping\n");
        ASSERT(addr->sa_family == AF_INET);
        //ASSERT(0 == portable_udpwrite(addr,handle,"ping",5,1));
    }
    if ( rcvbuf->base != 0 )
        free(rcvbuf->base);
}

void on_client_udprecv(uv_udp_t *handle,ssize_t nread,const uv_buf_t *rcvbuf,const struct sockaddr *addr,unsigned flags)
{
    char *verify_tokenized_json(char *sender,int32_t *validp,cJSON **argjsonp,char *parmstxt);
    uint16_t port;
    int32_t valid,err,len,createdflag;
    cJSON *argjson,*msgjson;
    struct NXT_acct *np;
    unsigned char pubkey[crypto_box_PUBLICKEYBYTES];
    char sender[64],senderNXTaddr[64],message[4096],*parmstxt;
    if ( nread > 0 )
    {
        strcpy(sender,"unknown");
        port = extract_nameport(sender,sizeof(sender),(struct sockaddr_in *)addr);
        printf("on_client_udprecv %s/%d nread.%ld flags.%d | total %ld\n",sender,port,nread,flags,server_xferred);
        memset(message,0,nread);
        if ( 1 && nread > (sizeof(pubkey) + crypto_box_NONCEBYTES+1) )
        {
            memcpy(pubkey,rcvbuf->base,sizeof(pubkey));
            len = (int32_t)(nread - sizeof(pubkey));
            err = _decode_cipher(message,(void *)((long)rcvbuf->base + sizeof(pubkey)),&len,pubkey,Global_mp->session_privkey);
            if ( err == 0 )
            {
                printf("DECRYPTED.(%s)\n",message);
            } else printf("error.%d decrypting pubkey.%llx\n",err,*(long long *)pubkey);
            
            parmstxt = clonestr(message);//rcvbuf->base;
            parmstxt[nread] = 0;
            argjson = cJSON_Parse(parmstxt);
            if ( argjson != 0 )
            {
                parmstxt = verify_tokenized_json(senderNXTaddr,&valid,&argjson,parmstxt);
                if ( valid != 0 && parmstxt != 0 && parmstxt[0] != 0 )
                {
                    np = get_NXTacct(&createdflag,Global_mp,senderNXTaddr);
                    if ( memcmp(np->pubkey,pubkey,sizeof(pubkey)) != 0 )
                    {
                        printf("update pubkey for NXT.%s to %llx\n",senderNXTaddr,*(long long *)pubkey);
                        memcpy(np->pubkey,pubkey,sizeof(np->pubkey));
                    }
                    queue_enqueue(&ALL_messages,clonestr(parmstxt));
                    printf("QUEUEALLMESSAGES.(%s) size.%d\n",parmstxt,queue_size(&ALL_messages));
                    msgjson = cJSON_GetObjectItem(argjson,"msg");
                    copy_cJSON(message,msgjson);
                    if ( message[0] != 0 )
                    {
                        queue_enqueue(&np->incoming,clonestr(message));
                        printf("QUEUE MESSAGES from NXT.%s (%s) size.%d\n",np->H.NXTaddr,message,queue_size(&np->incoming));
                    }
                }
                free_json(argjson);
                printf("parmstxt.%p valid.%d sender.(%s) msg.(%s)\n",parmstxt,valid,senderNXTaddr,message);
            }
            if ( parmstxt != 0 )
                free(parmstxt);
        }
        server_xferred += nread;
        ASSERT(addr->sa_family == AF_INET);
        //ASSERT(0 == portable_udpwrite(addr,handle,rcvbuf->base,nread,1));
    }
    if ( rcvbuf->base != 0 )
        free(rcvbuf->base);
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
    r = uv_listen(srv,SOMAXCONN,handler);
    if ( r != 0 )
    {
        fprintf(stderr,"Listen error %d %s\n",r,uv_err_name(r));
        return(0);
    }
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

void after_server_read(uv_stream_t *handle,ssize_t nread,const uv_buf_t *buf)
{
    char *pNXT_jsonhandler(cJSON **argjsonp,char *argstr);
    cJSON *argjson;
    char *jsonstr;
    struct NXT_acct *np = 0;
     // uv_shutdown_t *req;
    np = handle->data;
    printf("after read %ld | handle.%p data.%p\n",nread,handle,handle->data);
    if ( nread < 0 ) // Error or EOF
    {
        printf("nread.%ld UV_EOF %d\n",nread,UV_EOF);
        if ( buf->base != 0 )
            free(buf->base);
        //req = (uv_shutdown_t *)malloc(sizeof *req); causes problems?!
        //uv_shutdown(req,handle,after_shutdown);
        uv_close((uv_handle_t *)handle,on_close);
        if ( np != 0 )
        {
            np->tcp = 0;
            np->udp = 0;
            np->connect = 0;
        }
        return;
    }
    if ( nread == 0 )
    {
        // Everything OK, but nothing read.
        free(buf->base);
        return;
    }
    printf("got %ld bytes (%s) buf.%p base.%p np.%p\n",nread,buf->base,buf,buf->base,np);
    buf->base[nread] = 0;
    if ( 1 && np == 0 )
    {
        void **ptrs;
        ptrs = malloc(sizeof(*ptrs) * 2);
        ptrs[0] = handle;
        ptrs[1] = buf->base;
        queue_enqueue(&IntroQ,ptrs);
        /*np = process_intro(handle,(char *)buf->base,0);
        printf("process_intro returns np.%p for handle.%p\n",np,handle);
        if ( np != 0 )
        {
            handle->data = np;
            np->connect = handle;
        }
        printf("after process_intro returns np.%p for handle.%p\n",np,handle);*/
    }
    else
    {
        argjson = cJSON_Parse(buf->base);
        if ( argjson != 0 )
        {
            jsonstr = pNXT_jsonhandler(&argjson,buf->base);
            if ( jsonstr == 0 )
                jsonstr = clonestr("{\"result\":null}");
            printf("tcpwrite.(%s) to NXT.%s\n",jsonstr,np!=0?np->H.NXTaddr:"unknown");
            //portable_tcpwrite(handle,jsonstr,(int32_t)strlen(jsonstr)+1,-1);
            //free(jsonstr); completion frees, dont do it here!
            free_json(argjson);
        }
        if ( buf->base != 0 )
            free(buf->base);
    }
}

void tcp_client_gotbytes(uv_stream_t *tcp,ssize_t nread,const uv_buf_t *buf)
{
    char sender[32],*str;
    struct NXT_acct *np = 0;
    uv_udp_t *udp = 0;
    uv_stream_t *connect = 0;
    struct sockaddr addr;
    int port,addrlen = sizeof(addr);
    if ( (udp= (uv_udp_t *)tcp->data) != 0 )
    {
        connect = (uv_stream_t *)udp->data;
        if ( connect != 0 )
        {
            np = connect->data;
            printf("connect.%p -> %p\n",connect,connect->data);
        }
    }
    //printf("tcp_client_gotbytes tcp.%p (tcp) data.%p (udp) -> %p (connect)\n",tcp,udp,connect);
    if ( nread < 0 ) // Error or EOF
    {
        printf("Lost contact with Server! np.%p nread.%ld\n",np,nread);
        //ASSERT(nread == UV_EOF);
        if ( buf->base != 0 )
            free(buf->base);
        if ( udp != 0 )
        {
            printf("stop and close udp %p\n",udp);
            uv_udp_recv_stop(udp);
            uv_close((uv_handle_t *)udp,on_server_close);
        }
        uv_shutdown(malloc(sizeof(uv_shutdown_t)),tcp,after_server_shutdown);
        if ( np != 0 )
        {
            printf("clear server ptrs %p %p %p\n",np->tcp,np->connect,np->udp);
            np->tcp = 0;
            np->udp = 0;
            np->connect = 0;
        }
        return;
    }
    else if ( nread == 0 )
    {
        // Everything OK, but nothing read.
    }
    else
    {
        if ( uv_tcp_getpeername((uv_tcp_t *)tcp,&addr,&addrlen) == 0 )
        {
            uv_ip4_name((struct sockaddr_in *)&addr,sender,16);
            port = ntohs(((struct sockaddr_in *)&addr)->sin_port);
            printf("udp.%p tcp.%p got bytes.%p (connect) >>>>>>>>>>>>>>>>>>> got %ld bytes at %p.(%s) from %s/%d\n",udp,tcp,connect,nread,buf->base,buf->base,sender,port);
        }
        else printf(">>>>>>>>>>>>>>>>>>> got %ld bytes at %p.(%s) from MYSTERY\n",nread,buf->base,buf->base);
        str = malloc(nread+1);
        memcpy(str,buf->base,nread);
        str[nread] = 0;
        if ( connect != 0 && connect->data == 0 )
        {
            if ( (np= process_intro(0,str,0)) != 0 )
            {
                np->connect = connect;
                np->tcp = tcp;
                np->udp = (uv_stream_t *)udp;
                if ( connect != 0 )
                    connect->data = (void *)np;
            }
        }
        queue_enqueue(&RPC_6777_response,str);
    }
    if ( buf->base != 0 )
        free(buf->base);
}

void client_connected(uv_stream_t *server,int status)
{
    struct sockaddr addr;
    char sender[32];
    int r,port,addrlen = sizeof(addr);
    uv_stream_t *stream;
    printf("on_connection %p (connect) status.%d\n",server,status);
    if ( status != 0 )
        fprintf(stderr,"Connect error %s\n",uv_err_name(status));
    ASSERT(status == 0);
    stream = malloc(sizeof(uv_tcp_t));
    ASSERT(stream != NULL);
    r = uv_tcp_init(UV_loop,(uv_tcp_t *)stream);
    ASSERT(r == 0);
    stream->data = 0;   // need to wait until we know which acct it is
    r = uv_accept(server,stream);
    ASSERT(r == 0);
    r = uv_read_start(stream,portable_alloc,after_server_read);
    ASSERT(r == 0);
    if ( uv_tcp_getpeername((uv_tcp_t *)stream,&addr,&addrlen) == 0 )
    {
        if ( (port= extract_nameport(sender,sizeof(sender),(struct sockaddr_in *)&addr)) == 0 )
        {
            ASSERT(0 == portable_tcpwrite(stream,sender,strlen(sender)+1,1));
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
    tcp = (uv_tcp_t *)connect->handle;
    if ( status < 0 )
    {
        if ( status != -61 )
            printf("got on_client_connect error status.%d (%s) tcp.%p\n",status,uv_err_name(status),tcp);
        uv_shutdown(malloc(sizeof(uv_shutdown_t)),(uv_stream_t *)tcp,after_server_shutdown);
        return;
    }
    if ( uv_tcp_getpeername(tcp,(struct sockaddr *)&addr,&addrlen) == 0 )
    {
        if ( (port= extract_nameport(servername,sizeof(servername),(struct sockaddr_in *)&addr)) == 0 )
        {
            //printf("send udp to %s/%d\n",servername,port);
            //ASSERT(0 == portable_udpwrite(&addr,tcp->data,servername,strlen(servername)+1,1));
        }
    } else port = 0;
    printf("CONNECTED to %s/%d connect.%p tcp.%p\n",servername,port,connect,tcp);
    if ( uv_read_start(connect->handle,portable_alloc,tcp_client_gotbytes) != 0 )
        printf("uv_read_start failed\n");
    else
    {
        udp = malloc(sizeof(uv_udp_t));
        r = uv_udp_init(UV_loop,udp);
        if ( r != 0 )
        {
            fprintf(stderr, "uv_udp_init: %d %s\n",r,uv_err_name(r));
            free(udp);
            udp = 0;
        }
        r = uv_udp_recv_start(udp,portable_alloc,on_client_udprecv);
        if ( r != 0 )
        {
            fprintf(stderr, "uv_udp_recv_start: %d %s\n",r,uv_err_name(r));
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
        //printf("connect to %s/%d\n",servername,port);
        tcp = calloc(1,sizeof(*tcp));
        *connectp = calloc(1,sizeof(**connectp));
        uv_tcp_init(UV_loop,tcp);
        if ( (r= uv_tcp_connect(*connectp,tcp,(struct sockaddr *)addr,connected_to_server)) != 0 )
        {
            fprintf(stderr,"no TCP connection to %s/%d err %d %s\n",servername,port,r,uv_err_name(r));
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
            *tcp = start_libuv_tcp6server(port,client_connected);
        if ( udp != 0 )
            *udp = start_libuv_udp6server(port,on_udprecv);
    }
    else
    {
        if ( tcp != 0 )
            *tcp = start_libuv_tcp4server(port,client_connected);
        if ( udp != 0 )
            *udp = start_libuv_udp4server(port,on_udprecv);
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
    uint64_t get_pNXT_privacyServer(int32_t *activeflagp,char *secret);
    static char NXTACCTSECRET[256],NXTADDR[64];
    static uv_tcp_t *tcp;
    static uv_connect_t *connect;
    static uint64_t privacyServer,pNXT_privacyServer;
    static int32_t counter,numpings,didintro;
    static struct sockaddr addr;
    static double lastping,lastattempt;
    double millis;
    uv_stream_t *udp;
    struct NXT_acct *np;
    int32_t activeflag;
    uint64_t nxt64bits;
    char *jsonstr,**whitelist,**blacklist,intro[4096],secret[1024];
    whitelist = blacklist = 0;  // eventually get from config JSON
    if ( TCPserver_closed > 0 )
    {
        //printf("idler noticed TCPserver_closed.%d!\n",TCPserver_closed);
        tcp = 0;
        didintro = 0;
        connect = 0;
        pNXT_privacyServer = privacyServer = 0;
        set_pNXT_privacyServer(0);
        TCPserver_closed = 0;
        server_xferred = 0;
        return;
    }
    millis = ((double)uv_hrtime() / 1000000);
//#ifndef __linux__
    if ( millis > (lastattempt + 500) )
    {
        if ( privacyServer == 0 )
            privacyServer = pNXT_privacyServer;// != 0) ? pNXT_privacyServer:get_random_privacyServer(whitelist,blacklist);
        if ( privacyServer != 0 && NXTACCTSECRET[0] != 0 )
        {
            //printf("pNXT %llx vs %llx\n",(long long)pNXT_privacyServer,(long long)privacyServer);
            if ( tcp == 0 || connect == 0 )
            {
                printf("attempt privacyServer %llx %s/%d connect.%p tcp.%p udp.%p\n",(long long)privacyServer,ipbits_str((uint32_t)privacyServer),(int)(privacyServer>>32),connect,tcp,tcp!=0?tcp->data:0);
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
            if ( (pNXT_privacyServer= get_pNXT_privacyServer(&activeflag,secret)) != 0 && (pNXT_privacyServer != privacyServer || activeflag == 0) )
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
                if ( didintro == 0 )
                {
                    nxt64bits = issue_getAccountId(0,NXTACCTSECRET);
                    if ( nxt64bits != 0 )
                    {
                        expand_nxt64bits(NXTADDR,nxt64bits);
                        gen_tokenjson(0,intro,NXTADDR,time(NULL),NXTACCTSECRET);
                        //if ( set_intro(intro,sizeof(intro),Global_mp->dispname,Global_mp->groupname,NXTADDR,NXTACCTSECRET) < 0 )
                        if ( intro[0] == 0 )
                        {
                            printf("connect_to_privacyServer: invalid intro.(%s), try again\n",intro);
                            memset(NXTACCTSECRET,0,sizeof(NXTACCTSECRET));
                            return;
                        }
                        portable_tcpwrite((uv_stream_t *)tcp,intro,strlen(intro)+1,1);
                        //portable_udpwrite(&addr,(uv_udp_t *)udp,intro,strlen(intro)+1,1);
                        memset(NXTACCTSECRET,0,sizeof(NXTACCTSECRET));
                        didintro = 1;
                    }
                }
                if ( (np= (void *)connect->data) != 0 )
                {
                    udp = np->udp;
                    if ( (counter++ % 10) == 0 && udp != 0 )
                    {
                        //printf("udp.%p send ping.%d total transferred.%ld\n",udp,numpings,server_xferred);
                        portable_udpwrite(&addr,(uv_udp_t *)udp,"ping",5,1);
                        numpings++;
                    }
                }
            }
            lastping = millis;
        }
    }
//#endif
    if ( tcp != 0 )
    {
        if ( (jsonstr= queue_dequeue(&RPC_6777)) != 0 )
            portable_tcpwrite((uv_stream_t *)tcp,jsonstr,strlen(jsonstr)+1,-1);
    }
    void **ptrs;
    uv_stream_t *h;
    if ( (ptrs= queue_dequeue(&IntroQ)) != 0 )
    {
        h = ptrs[0];
        np = process_intro(h,(char *)ptrs[1],1);
        printf("process_intro returns np.%p for handle.%p\n",np,h);
        if ( np != 0 )
        {
            handle->data = np;
            np->connect = h;
        }
        printf("after process_intro returns np.%p for handle.%p ptrs.%p ptrs[1] %p\n",np,h,ptrs,ptrs[1]);
        free(ptrs[1]);
        free(ptrs);
        printf("done free\n");
    }
}

void init_NXTprivacy(void *ptr)
{
    uint64_t nxt64bits;
    uv_tcp_t *tcp,**tcpptr;
    uv_udp_t *udp,**udpptr;
    if ( (Server_secret= ptr) == 0 )
        Server_secret = "password";
    nxt64bits = issue_getAccountId(0,Server_secret);
    Server_NXTaddr = calloc(1,64);
    expand_nxt64bits(Server_NXTaddr,nxt64bits);
    printf("init_NXTprivacy.(%s) -> %llu (%s)\n",(char *)ptr,(long long)nxt64bits,Server_NXTaddr);
    tcp = &Global_mp->Punch_tcp; tcpptr = &tcp;
    udp = &Global_mp->Punch_udp; udpptr = &udp;
    start_libuv_servers(tcpptr,udpptr,4,NXT_PUNCH_PORT);
}


#endif
