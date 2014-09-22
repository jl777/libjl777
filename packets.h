//
//  packets.h
//  xcode
//
//  Created by jl777 on 7/19/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_packets_h
#define xcode_packets_h

#define ALLOCWR_DONTFREE 0
#define ALLOCWR_ALLOCFREE 1
#define ALLOCWR_FREE -1

#define PEER_SENT 1
#define PEER_RECV 2
#define PEER_PENDINGRECV 4
#define PEER_MASK (PEER_SENT|PEER_RECV|PEER_PENDINGRECV)
#define PEER_TIMEOUT 0x40
#define PEER_FINISHED 0x80
#define PEER_EXPIRATION (60 * 1000.)

struct peer_queue_entry
{
    struct peerinfo *peer,*hop;
    float startmilli,elapsed;
    uint8_t stateid,state;
};

struct pingpong_queue PeerQ;
queue_t HelloQ;
struct peerinfo **Peers,**Pservers;
int32_t Numpeers,Numpservers;
int32_t portable_udpwrite(const struct sockaddr *addr,uv_udp_t *handle,void *buf,long len,int32_t allocflag);

// single threaded peer functions
int32_t process_PeerQ(void **ptrp,void *arg) // added when inbound transporter sequence is started
{
    struct peer_queue_entry *ptr = (*ptrp);
    struct peerinfo *peer,*hop;
    int32_t stateid;
    double timediff;
    peer = ptr->peer;
    hop = ptr->hop;
    timediff = (double)(milliseconds() - ptr->startmilli);
    stateid = ptr->stateid;
    if ( peer != 0 && hop != 0 && timediff >= -3000 && stateid < NUM_PEER_STATES )
    {
        //printf("timediff.%f stateid.%d %d %d\n",timediff,stateid,peer->states[stateid],hop->states[stateid]);
        if ( timediff > PEER_EXPIRATION )
        {
            if ( (peer->states[stateid]&PEER_FINISHED) != PEER_FINISHED || (hop->states[stateid]&PEER_FINISHED) != PEER_FINISHED )
            {
                peer->states[ptr->stateid] |= PEER_TIMEOUT;
                hop->states[ptr->stateid] |= PEER_TIMEOUT;
            }
            printf("process_peerQ: %s %llu state.%d -> %llu state.%d | elapsed.%.3f timediff.%.3f stateid.%d of %d\n",(timediff > PEER_EXPIRATION)?"EXPIRED!":"COMPLETED",(long long)peer->pubnxtbits,peer->states[stateid],(long long)hop->pubnxtbits,hop->states[stateid],ptr->elapsed,timediff,stateid,NUM_PEER_STATES);
            return(1);
        } else return(0);
    }
    else
    {
        printf("process_peerQ: illegal peer.%p or hop.%p | elapsed.%.3f timediff.%.3f stateid.%d of %d\n",peer,hop,hop->elapsed[stateid],timediff,stateid,NUM_PEER_STATES);
        return(-1);
    }
}

void update_peerstate(struct peerinfo *peer,struct peerinfo *hop,int32_t stateid,int32_t state)
{
    struct peer_queue_entry *ptr;
    if ( stateid >= NUM_PEER_STATES )
    {
        printf("illegal stateid.%d of %d: state,%d\n",stateid,NUM_PEER_STATES,state);
        return;
    }
    if ( state == PEER_SENT )
    {
        peer->states[stateid] = state;
        hop->states[stateid] = PEER_PENDINGRECV;
        ptr = calloc(1,sizeof(*ptr));
        ptr->peer = peer;
        ptr->hop = hop;
        ptr->stateid = stateid;
        ptr->state = state;
        ptr->startmilli = peer->startmillis[stateid] = milliseconds();
        hop->elapsed[stateid] = 0;
        queue_enqueue(&PeerQ.pingpong[0],ptr);
    }
    else if ( state == PEER_RECV )
    {
        if ( (peer->states[stateid]&PEER_MASK) == PEER_SENT && (hop->states[stateid]&PEER_MASK) == PEER_PENDINGRECV )
        {
            peer->states[stateid] |= PEER_FINISHED;
            hop->states[stateid] |= PEER_FINISHED;
            hop->elapsed[stateid] = (double)(milliseconds() - peer->startmillis[stateid]);
        }
        else printf("unexpected states[%d] NXT.%llu %d and hopNXT.%llu %d\n",stateid,(long long)peer->pubnxtbits,peer->states[stateid],(long long)hop->pubnxtbits,hop->states[stateid]);
    }
    else printf("update_peerstate unknown state.%d\n",state);
}

struct peerinfo *get_random_pserver()
{
    if ( Numpservers != 0 )
        return(Pservers[(rand() >> 8) % Numpservers]);
    return(0);
}

int32_t is_privacyServer(struct peerinfo *peer)
{
    if ( peer->srvnxtbits == peer->pubnxtbits && peer->srvipbits != 0 && peer->srvport != 0 )
        return(1);
    return(0);
}

char *_coins_jsonstr(char *coinsjson,uint64_t coins[4])
{
    int32_t i,n = 0;
    char *str;
    if ( coins == 0 )
        return(0);
    strcpy(coinsjson,",\"coins\":[");
    for (i=0; i<4*64; i++)
        if ( (coins[i>>6] & (1L << (i&63))) != 0 )
        {
            str = coinid_str(i);
            if ( strcmp(str,ILLEGAL_COIN) != 0 )
            {
                if ( n++ != 0 )
                    strcat(coinsjson,",");
                sprintf(coinsjson+strlen(coinsjson),"\"%s\"",str);
            }
        }
    if ( n == 0 )
        coinsjson[0] = coinsjson[1] = 0;
    else strcat(coinsjson,"]");
    return(coinsjson);
}

cJSON *gen_peerinfo_json(struct peerinfo *peer)
{
    char srvipaddr[64],srvnxtaddr[64],numstr[64],pubNXT[64],hexstr[512],coinsjsonstr[1024];
    cJSON *coins,*json = cJSON_CreateObject();
    expand_ipbits(srvipaddr,peer->srvipbits);
    expand_nxt64bits(srvnxtaddr,peer->srvnxtbits);
    expand_nxt64bits(pubNXT,peer->pubnxtbits);
    if ( is_privacyServer(peer) != 0 )
    {
        cJSON_AddItemToObject(json,"is_privacyServer",cJSON_CreateNumber(1));
        cJSON_AddItemToObject(json,"pubNXT",cJSON_CreateString(srvnxtaddr));
        cJSON_AddItemToObject(json,"srvipaddr",cJSON_CreateString(srvipaddr));
        sprintf(numstr,"%d",peer->srvport);
        cJSON_AddItemToObject(json,"srvport",cJSON_CreateString(numstr));
        cJSON_AddItemToObject(json,"sent",cJSON_CreateNumber(peer->numsent));
        cJSON_AddItemToObject(json,"recv",cJSON_CreateNumber(peer->numrecv));
    }
    else cJSON_AddItemToObject(json,"pubNXT",cJSON_CreateString(pubNXT));
    init_hexbytes(hexstr,peer->pubkey,sizeof(peer->pubkey));
    cJSON_AddItemToObject(json,"pubkey",cJSON_CreateString(hexstr));
    if ( _coins_jsonstr(coinsjsonstr,peer->coins) != 0 )
    {
        //printf("got.(%s)\n",coinsjsonstr);
        coins = cJSON_Parse(coinsjsonstr+9);
        if ( coins != 0 )
        {
            //printf("coins.(%s)\n",cJSON_Print(coins));
            cJSON_AddItemToObject(json,"coins",coins);
        }
        else printf("error parsing.(%s)\n",coinsjsonstr);
    }
    return(json);
}

cJSON *gen_peers_json(int32_t only_privacyServers)
{
    int32_t i,n;
    struct peerinfo **peers;
    cJSON *json,*array;
    //printf("inside gen_peer_json.%d\n",only_privacyServers);
    json = cJSON_CreateObject();
    array = cJSON_CreateArray();
    if ( only_privacyServers != 0 )
    {
        peers = Pservers;
        n = Numpservers;
    }
    else
    {
        peers = Peers;
        n = Numpeers;
    }
    if ( peers != 0 && n > 0 )
    {
        cJSON_AddItemToObject(json,"only_privacyServers",cJSON_CreateNumber(only_privacyServers));
        cJSON_AddItemToObject(json,"num",cJSON_CreateNumber(n));
        for (i=0; i<n; i++)
        {
            if ( peers[i] != 0 )
                cJSON_AddItemToArray(array,gen_peerinfo_json(peers[i]));
        }
        cJSON_AddItemToObject(json,"peers",array);
    }
    return(json);
}

struct peerinfo *find_privacyserver(struct peerinfo *refpeer)
{
    struct peerinfo *peer = 0;
    int32_t i;
    for (i=0; i<Numpservers; i++)
    {
        if ( (peer= Pservers[i]) != 0 )
        {
            if ( refpeer->srvnxtbits == peer->srvnxtbits && refpeer->srvipbits == peer->srvipbits && refpeer->srvport == peer->srvport )
                return(peer);
        }
    }
    return(0);
}

struct peerinfo *find_peerinfo(uint64_t pubnxtbits,char *pubBTCD,char *pubBTC)
{
    struct peerinfo *peer = 0;
    int32_t i;
    for (i=0; i<Numpeers; i++)
    {
        if ( (peer= Peers[i]) != 0 )
        {
            if ( pubnxtbits != 0 && peer->pubnxtbits == pubnxtbits )
                return(peer);
            if ( pubBTCD != 0 && pubBTCD[0] != 0 && strcmp(peer->pubBTCD,pubBTCD) == 0 )
                return(peer);
            if ( pubBTC != 0 && pubBTC[0] != 0 && strcmp(peer->pubBTC,pubBTC) == 0 )
                return(peer);
        }
    }
    return(0);
}

struct peerinfo *add_peerinfo(struct peerinfo *refpeer)
{
    void say_hello(struct NXT_acct *np);
    char NXTaddr[64];
    int32_t createdflag,isPserver;
    struct NXT_acct *np = 0;
    struct peerinfo *peer = 0;
    if ( (peer= find_peerinfo(refpeer->pubnxtbits,refpeer->pubBTCD,refpeer->pubBTC)) != 0 )
        return(peer);
    Peers = realloc(Peers,sizeof(*Peers) * (Numpeers + 1));
    if ( refpeer->pubnxtbits != 0 )
    {
        expand_nxt64bits(NXTaddr,refpeer->pubnxtbits);
        np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
        peer = &np->mypeerinfo;
    }
    else
    {
        printf("FATAL: add_peerinfo without nxtbits (%s %llu %s)\n",refpeer->pubBTCD,(long long)refpeer->pubnxtbits,refpeer->pubBTC);
        peer = calloc(1,sizeof(*peer));
    }
    *peer = *refpeer;
    Peers[Numpeers] = peer, Numpeers++;
    if ( (isPserver= is_privacyServer(peer)) != 0 )
    {
        if ( find_privacyserver(peer) == 0 )
        {
            Pservers = realloc(Pservers,sizeof(*Pservers) * (Numpservers + 1));
            Pservers[Numpservers] = peer, Numpservers++;
            printf("ADDED privacyServer.%d\n",Numpservers);
            if ( np != 0 )
                queue_enqueue(&HelloQ,np);
        }
    }
    printf("isPserver.%d add_peerinfo Numpeers.%d added %llu srv.%llu\n",isPserver,Numpeers,(long long)refpeer->pubnxtbits,(long long)refpeer->srvnxtbits);
    return(peer);
}

struct peerinfo *update_peerinfo(int32_t *createdflagp,struct peerinfo *refpeer)
{
    static uint8_t zeropubkey[crypto_box_PUBLICKEYBYTES];
    struct peerinfo *peer = 0;
    int32_t savedNumpeers = Numpeers;
    if ( memcmp(refpeer->pubkey,zeropubkey,sizeof(refpeer->pubkey)) == 0 )
    {
        printf("ERROR: update_peerinfo null pubkey not allowed: Numpeers.%d\n",Numpeers);
        return(0);
    }
    if ( refpeer->pubnxtbits == 0 && refpeer->pubBTCD[0] == 0 && refpeer->pubBTC[0] == 0 )
    {
        printf("ERROR: update_peerinfo all zero pubaddrs not supported: Numpeers.%d\n",Numpeers);
        return(0);
    }
    if ( (peer= add_peerinfo(refpeer)) == 0 )
    {
        printf("ERROR: update_peerinfo null ptr from add_peerinfo: Numpeers.%d\n",Numpeers);
        return(0);
    }
    printf("update_peerinfo Numpeers.%d added %llu srv.%llu\n",Numpeers,(long long)refpeer->pubnxtbits,(long long)refpeer->srvnxtbits);
    *createdflagp = (Numpeers != savedNumpeers);
    return(peer);
}

int32_t set_pubpeerinfo(char *srvNXTaddr,char *srvipaddr,int32_t srvport,struct peerinfo *peer,char *pubBTCD,char *pubkey,uint64_t pubnxt64bits,char *pubBTC)
{
    memset(peer,0,sizeof(*peer));
    if ( srvNXTaddr != 0 )
        peer->srvnxtbits = calc_nxt64bits(srvNXTaddr);
    if ( srvport == 0 )
        srvport = NXT_PUNCH_PORT;
    if ( srvipaddr != 0 )
    {
        peer->srvipbits = calc_ipbits(srvipaddr);
        peer->srvport = srvport;
    }
    peer->pubnxtbits = pubnxt64bits;
    safecopy(peer->pubBTCD,pubBTCD,sizeof(peer->pubBTCD));
    safecopy(peer->pubBTC,pubBTC,sizeof(peer->pubBTC));
    if ( decode_hex(peer->pubkey,(int32_t)strlen(pubkey)/2,pubkey) != sizeof(peer->pubkey) )
    {
        printf("WARNING: (%s %llu %s) illegal pubkey.(%s)\n",pubBTCD,(long long)pubnxt64bits,pubBTC,pubkey);
        return(-1);
    }
    return(0);
}

void query_pubkey(char *destNXTaddr,char *NXTACCTSECRET)
{
    struct NXT_acct *np;
    char NXTaddr[64];
    NXTaddr[0] = 0;
    np = find_NXTacct(destNXTaddr,NXTACCTSECRET);
    memcpy(np->mypeerinfo.pubkey,Global_mp->session_pubkey,sizeof(np->mypeerinfo.pubkey));
    printf("(%s) [%s] need to implement query (and propagation) mechanism for pubkeys\n",destNXTaddr,np->H.U.NXTaddr);
}

int32_t _encode_str(unsigned char *cipher,char *str,int32_t size,unsigned char *destpubkey,unsigned char *myprivkey)
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
    return((int32_t)len + crypto_box_ZEROBYTES + crypto_box_NONCEBYTES);
}

int32_t _decode_cipher(char *str,unsigned char *cipher,int32_t *lenp,unsigned char *srcpubkey,unsigned char *myprivkey)
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
    *lenp = len - crypto_box_ZEROBYTES;
    return(err);
}

int32_t crcize(unsigned char *final,unsigned char *encoded,int32_t len)
{
    uint32_t crc = _crc32(0,encoded,len);
    memcpy(final,&crc,sizeof(crc));
    memcpy(final + sizeof(crc),encoded,len);
    return(len + sizeof(crc));
}

int32_t onionize(char *verifiedNXTaddr,unsigned char *encoded,char *destNXTaddr,unsigned char *payload,int32_t len)
{
    unsigned char onetime_pubkey[crypto_box_PUBLICKEYBYTES],onetime_privkey[crypto_box_SECRETKEYBYTES];
    uint64_t nxt64bits;
    int32_t createdflag;
    uint16_t *payload_lenp,slen;
    struct NXT_acct *np;
    nxt64bits = calc_nxt64bits(destNXTaddr);
    np = get_NXTacct(&createdflag,Global_mp,destNXTaddr);
    crypto_box_keypair(onetime_pubkey,onetime_privkey);
    memcpy(encoded,&nxt64bits,sizeof(nxt64bits));
    encoded += sizeof(nxt64bits);
    memcpy(encoded,onetime_pubkey,sizeof(onetime_pubkey));
    encoded += sizeof(onetime_pubkey);
    payload_lenp = (uint16_t *)encoded;
    encoded += sizeof(*payload_lenp);
    printf("ONIONIZE: np.%p NXT.%s %s pubkey.%llx encode len.%d -> ",np,np->H.U.NXTaddr,destNXTaddr,*(long long *)np->mypeerinfo.pubkey,len);
    len = _encode_str(encoded,(char *)payload,len,np->mypeerinfo.pubkey,onetime_privkey);
    slen = len;
    memcpy(payload_lenp,&slen,sizeof(*payload_lenp));
    printf("new len.%d + %ld = %ld\n",len,sizeof(*payload_lenp) + sizeof(onetime_pubkey) + sizeof(nxt64bits),sizeof(*payload_lenp) + sizeof(onetime_pubkey) + sizeof(nxt64bits)+len);
    return(len + sizeof(*payload_lenp) + sizeof(onetime_pubkey) + sizeof(nxt64bits));
}

int32_t add_random_onionlayers(char *hopNXTaddr,int32_t numlayers,char *verifiedNXTaddr,uint8_t *final,uint8_t *src,int32_t len)
{
    char destNXTaddr[64];
    uint8_t bufs[2][4096],*dest;
    struct peerinfo *peer;
    struct NXT_acct *np;
    if ( numlayers > 1 )
        numlayers = ((rand() >> 8) % numlayers);
    if ( numlayers <= 0 )
        numlayers = 1;
    if ( numlayers > 0 )
    {
        memset(bufs,0,sizeof(bufs));
        dest = bufs[numlayers & 1];
        memcpy(dest,src,len);
        while ( numlayers > 0 )
        {
            src = bufs[numlayers & 1];
            dest = bufs[(numlayers & 1) ^ 1];
            peer = get_random_pserver();
            if ( peer == 0 )
            {
                printf("FATAL: cant get random peer!\n");
                return(-1);
            }
            np = search_addresses(peer->pubBTCD);
            if ( np == 0 && peer->pubnxtbits != 0 )
            {
                expand_nxt64bits(destNXTaddr,peer->pubnxtbits);
                np = search_addresses(destNXTaddr);
            }
            if ( np == 0 )
                np = search_addresses(peer->pubBTC);
            if ( np == 0 )
            {
                printf("FATAL: %s %s %s is unknown account\n",peer->pubBTCD,destNXTaddr,peer->pubBTC);
                return(-1);
            }
            if ( memcmp(np->mypeerinfo.pubkey,peer->pubkey,sizeof(np->mypeerinfo.pubkey)) != 0 )
            {
                printf("Warning: (%s %s %s) pubkey updated %llx -> %llx\n",peer->pubBTCD,destNXTaddr,peer->pubBTC,*(long long *)np->mypeerinfo.pubkey,*(long long *)peer->pubkey);
                memcpy(np->mypeerinfo.pubkey,peer->pubkey,sizeof(np->mypeerinfo.pubkey));
            }
            printf("add layer %d: NXT.%s\n",numlayers,np->H.U.NXTaddr);
            len = onionize(verifiedNXTaddr,dest,np->H.U.NXTaddr,src,len);
            strcpy(hopNXTaddr,np->H.U.NXTaddr);
            if ( len > 4096 )
            {
                printf("FATAL: onion layers too big.%d\n",len);
                return(-1);
            }
            numlayers--;
        }
        src = dest;
    }
    memcpy(final,src,len);
    return(len);
}

int32_t deonionize(unsigned char *pubkey,unsigned char *decoded,unsigned char *encoded,int32_t len,uint64_t mynxtbits)
{
    //void *origencoded = encoded;
    int32_t err;
    struct coin_info *cp;
    uint16_t payload_len;
    if ( mynxtbits == 0 || memcmp(&mynxtbits,encoded,sizeof(mynxtbits)) == 0 )
    {
        encoded += sizeof(mynxtbits);
        memcpy(pubkey,encoded,crypto_box_PUBLICKEYBYTES);
        encoded += crypto_box_PUBLICKEYBYTES;
        memcpy(&payload_len,encoded,sizeof(payload_len));
        //printf("deonionize >>>>> pubkey.%llx vs mypubkey.%llx (%llx) -> %d %2x\n",*(long long *)pubkey,*(long long *)Global_mp->session_pubkey,*(long long *)Global_mp->loopback_pubkey,payload_len,payload_len);
        encoded += sizeof(payload_len);
        if ( (payload_len + sizeof(payload_len) + sizeof(Global_mp->session_pubkey) + sizeof(mynxtbits)) == len )
        {
            len = payload_len;
            err = _decode_cipher((char *)decoded,encoded,&len,pubkey,Global_mp->session_privkey);
            if ( err == 0 )
            {
                //printf("payload_len.%d err.%d new len.%d\n",payload_len,err,len);
                return(len);
            }
            cp = get_coin_info("BTCD");
            if ( cp != 0 && strcmp(cp->privacyserver,"127.0.0.1") == 0 ) // might have been encrypted to the loopback
            {
                len = payload_len;
                err = _decode_cipher((char *)decoded,encoded,&len,pubkey,Global_mp->loopback_privkey);
                if ( err == 0 )
                {
                    //printf("2nd payload_len.%d err.%d new len.%d\n",payload_len,err,len);
                    return(len);
                }
            }
        } //else printf("mismatched len expected %ld got %d\n",(payload_len + sizeof(payload_len) + sizeof(Global_mp->session_pubkey) + sizeof(mynxtbits)),len);
    }
    else
    {
        uint64_t destbits;
        memcpy(&destbits,encoded,sizeof(destbits));
        printf("deonionize onion for NXT.%llu not this address.(%llu)\n",(long long)destbits,(long long)mynxtbits);
    }
    return(0);
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

void update_routing_probs(struct peerinfo *peer,struct sockaddr *addr)
{
    struct Uaddr *uaddr;
    int32_t i,port;
    char sender[64],srvip[64];
    port = extract_nameport(sender,sizeof(sender),(struct sockaddr_in *)addr);
    expand_ipbits(srvip,peer->srvipbits);
    peer->numrecv++;
    if ( is_privacyServer(peer) != 0 && strcmp(sender,srvip) == 0 && port == peer->srvport )
    {
        printf("direct comm from privacy server %s/%d vs (%s/%d) NXT.%llu | recv.%d send.%d\n",sender,port,srvip,peer->srvport,(long long)peer->srvnxtbits,peer->numrecv,peer->numsent);
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
                if ( memcmp(&uaddr->addr,addr,sizeof(uaddr->addr)) == 0 )
                    break;
            }
        }
        if ( i == peer->numUaddrs )
        {
            peer->Uaddrs = realloc(peer->Uaddrs,sizeof(*peer->Uaddrs) * (peer->numUaddrs+1));
            uaddr = &peer->Uaddrs[peer->numUaddrs++];
            memset(uaddr,0,sizeof(*uaddr));
            uaddr->addr = *addr;
            printf("add.%d Uaddr.(%s/%d) to %llu\n",peer->numUaddrs,sender,port,(long long)peer->pubnxtbits);
        }
        if ( uaddr != 0 )
        {
            uaddr->numrecv++;
            uaddr->lastcontact = (uint32_t)time(NULL);
        } else printf("update_routing_probs: unexpected null uaddr for %s/%d!\n",sender,port);
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
    n = (peer->numUaddrs > max) ? max : peer->numUaddrs;
    if ( peer->numUaddrs == 0 || peer->Uaddrs == 0 )
        return(0);
    else if ( peer->numUaddrs > 1 )
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

uint64_t route_packet(uv_udp_t *udp,char *hopNXTaddr,unsigned char *outbuf,int32_t len)
{
    unsigned char finalbuf[4096],hash[256>>3];
    char destip[64];
    struct sockaddr_in addr;
    struct Uaddr *Uaddrs[8];
    uint64_t txid = 0;
    int32_t createdflag,i,n;
    struct NXT_acct *np;
    memset(finalbuf,0,sizeof(finalbuf));
    np = get_NXTacct(&createdflag,Global_mp,hopNXTaddr);
    expand_ipbits(destip,np->mypeerinfo.srvipbits);
    len = crcize(finalbuf,outbuf,len);
    if ( len > sizeof(finalbuf) )
    {
        printf("sendmessage: len.%d > sizeof(finalbuf) %ld\n",len,sizeof(finalbuf));
        exit(-1);
    }
    if ( is_privacyServer(&np->mypeerinfo) != 0 )
    {
        printf("DIRECT udpsend to %s/%d finalbuf.%d\n",destip,np->mypeerinfo.srvport,len);
        np->mypeerinfo.numsent++;
        if ( len < 1400 )
        {
            uv_ip4_addr(destip,np->mypeerinfo.srvport,&addr);
            portable_udpwrite((struct sockaddr *)&addr,udp,finalbuf,len,ALLOCWR_ALLOCFREE);
        }
        else call_libjl777_broadcast(destip,(char *)finalbuf,len,0);
    }
    else
    {
        memset(Uaddrs,0,sizeof(Uaddrs));
        n = sort_topaddrs(Uaddrs,(int32_t)(sizeof(Uaddrs)/sizeof(*Uaddrs)),&np->mypeerinfo);
        printf("n.%d hopNXTaddr.(%s) narrowcast %s:%d %d via p2p\n",len,hopNXTaddr,destip,np->mypeerinfo.srvport,len);
        if ( n > 0 )
        {
            for (i=0; i<n; i++)
            {
                np->mypeerinfo.numsent++;
                Uaddrs[i]->numsent++;
                if ( len < 1400 )
                    portable_udpwrite((struct sockaddr *)&Uaddrs[i]->addr,udp,finalbuf,len,ALLOCWR_ALLOCFREE);
                else
                {
                    extract_nameport(destip,sizeof(destip),(struct sockaddr_in *)&Uaddrs[i]->addr);
                    call_libjl777_broadcast(destip,(char *)finalbuf,len,0);
                }
            }
        }
    }
    calc_sha256(0,hash,finalbuf,len);
    txid = calc_txid(hash,sizeof(hash));
    return(txid);
}

struct NXT_acct *process_packet(char *retjsonstr,unsigned char *recvbuf,int32_t recvlen,uv_udp_t *udp,struct sockaddr *addr,char *sender,uint16_t port)
{
    uint64_t destbits = 0;
    struct NXT_acct *tokenized_np = 0;
    int32_t valid,len=0,tmp,createdflag,decrypted=0;
    cJSON *argjson;
    unsigned char pubkey[crypto_box_PUBLICKEYBYTES],tmppubkey[crypto_box_PUBLICKEYBYTES],decoded[4096],tmpbuf[4096];
    char senderNXTaddr[64],hopNXTaddr[64],*parmstxt=0,*jsonstr;
    memset(decoded,0,sizeof(decoded));
    memset(tmpbuf,0,sizeof(tmpbuf));
    retjsonstr[0] = 0;
    //sprintf(retjsonstr,"{\"error\":\"unknown error processing %d bytes from %s/%d\"}",recvlen,sender,port);
    if ( is_encrypted_packet(recvbuf,recvlen) != 0 )
    {
        recvbuf += sizeof(uint32_t);
        recvlen -= sizeof(uint32_t);
        if ( (len= deonionize(pubkey,decoded,recvbuf,recvlen,0)) > 0 )
        {
            memcpy(&destbits,decoded,sizeof(destbits));
            decrypted++;
            if ( (tmp= deonionize(tmppubkey,tmpbuf,decoded,len,0)) > 0 )
            {
                memcpy(&destbits,decoded,sizeof(destbits));
                decrypted++;
                len = tmp;
                memcpy(decoded,tmpbuf,len);
                memcpy(pubkey,tmppubkey,sizeof(pubkey));
                printf("decrypted2 len.%d dest.(%llu)\n",len,(long long)destbits);
            } else printf("decrypted len.%d dest.(%llu)\n",len,(long long)destbits);
        }
        else return(0);
    }
    else
    {
        printf("process_packet got unexpected nonencrypted len.%d %s/%d\n",recvlen,sender,port);
        return(0);
    }
    if ( len > 0 )
    {
        decoded[len] = 0;
        parmstxt = clonestr((char *)decoded);
        argjson = cJSON_Parse(parmstxt);
        free(parmstxt);
        parmstxt = 0;
        //printf("[%s] argjson.%p udp.%p\n",parmstxt,argjson,udp);
        if ( argjson != 0 ) // if it parses, we must have been the ultimate destination
        {
            parmstxt = verify_tokenized_json(senderNXTaddr,&valid,argjson);
            if ( valid != 0 && parmstxt != 0 && parmstxt[0] != 0 )
            {
                tokenized_np = get_NXTacct(&createdflag,Global_mp,senderNXTaddr);
                char *pNXT_json_commands(struct NXThandler_info *mp,struct sockaddr *prevaddr,cJSON *argjson,char *sender,int32_t valid,char *origargstr);
                jsonstr = pNXT_json_commands(Global_mp,addr,argjson,tokenized_np->H.U.NXTaddr,valid,(char *)decoded);
                if ( jsonstr != 0 )
                {
                    strcpy(retjsonstr,jsonstr);
                    free(jsonstr);
                }
                update_routing_probs(&tokenized_np->mypeerinfo,addr);
            }
            else printf("valid.%d unexpected non-tokenized message.(%s)\n",valid,decoded);
            free_json(argjson);
            return(tokenized_np);
        }
        else
        {
            memcpy(&destbits,decoded,sizeof(destbits));
            if ( destbits != 0 ) // route packet
            {
                expand_nxt64bits(hopNXTaddr,destbits);
                printf("FORWARD PACKET from %s/%d to %s\n",sender,port,hopNXTaddr);
                route_packet(udp,hopNXTaddr,decoded,len);
                return(0);
            }
        }
        if ( parmstxt != 0 )
            free(parmstxt);
    }
    else printf("process_packet got unexpected recvlen.%d %s/%d\n",recvlen,sender,port);
    return(0);
}

int32_t has_privacyServer(struct NXT_acct *np)
{
    if ( np->mypeerinfo.srvipbits != 0 && np->mypeerinfo.pubnxtbits != np->mypeerinfo.srvnxtbits )
        return(1);
    else return(0);
}

char *sendmessage(char *hopNXTaddr,int32_t L,char *verifiedNXTaddr,char *msg,int32_t msglen,char *destNXTaddr,char *origargstr)
{
    uint64_t txid;
    char buf[4096],destsrvNXTaddr[64],srvNXTaddr[64];
    unsigned char encodedsrvD[4096],encodedD[4096],encodedL[4096],encodedP[4096],*outbuf;
    int32_t len,createdflag;
    struct NXT_acct *np,*destnp;    
    np = get_NXTacct(&createdflag,Global_mp,verifiedNXTaddr);
    destnp = get_NXTacct(&createdflag,Global_mp,destNXTaddr);
    if ( np == 0 || destnp == 0 || Global_mp->udp == 0 || destnp->mypeerinfo.srvnxtbits == 0 )
    {
        printf("np.%p destnp.%p udp.%p nxtbits.%llu\n",np,destnp,Global_mp->udp,(long long)destnp->mypeerinfo.srvnxtbits);
        return(clonestr("\"error\":\"no np or global udp for sendmessage || destnp->mypeerinfo.srvnxtbits == 0\"}"));
    }
    expand_nxt64bits(srvNXTaddr,np->mypeerinfo.srvnxtbits);
    expand_nxt64bits(destsrvNXTaddr,destnp->mypeerinfo.srvnxtbits);
    memset(encodedD,0,sizeof(encodedD)); // encoded to dest
    memset(encodedsrvD,0,sizeof(encodedsrvD)); // encoded to privacyServer of dest
    memset(encodedL,0,sizeof(encodedL)); // encoded to max L onion layers
    memset(encodedP,0,sizeof(encodedP)); // encoded to privacyserver
    len = onionize(verifiedNXTaddr,encodedD,destNXTaddr,(unsigned char *)origargstr,(int32_t)strlen(origargstr)+1);
    printf("sendmessage (%s) len.%d to %s\n",origargstr,msglen,destNXTaddr);
    if ( len > sizeof(encodedP)-1024 ) // 1024 should be plenty of room, especially since 1400 is UDP limit used
    {
        printf("sendmessage, payload too big %d\n",len);
        sprintf(buf,"{\"error\":\"%s cant sendmessage.(%s) to %s too long.%d\"}",verifiedNXTaddr,msg,destNXTaddr,len);
    }
    else if ( len > 0 )
    {
        outbuf = encodedD;
        if ( 1 && strcmp(verifiedNXTaddr,destNXTaddr) == 0 ) // chanc3r's boomerang onion
        {
            if ( hopNXTaddr[0] != 0 && L == 0 )
            {
                len = onionize(verifiedNXTaddr,encodedsrvD,srvNXTaddr,outbuf,len);
                outbuf = encodedsrvD;
                len = onionize(verifiedNXTaddr,encodedL,hopNXTaddr,outbuf,len);
                outbuf = encodedL;
                // we now have [hopNXTaddr [our privacyServer [this node]]]
            }
            else
            {
                printf("boomerang needs a hopNXTaddr! and L.%d == 0\n",L);
                return(clonestr("\"error\":\"boomerang needs a hopNXTaddr!\"}"));
            }
        }
        else
        {
            strcpy(hopNXTaddr,destNXTaddr);
            if ( has_privacyServer(destnp) != 0 ) // build onion in reverse order, privacyServer for dest is 2nd
            {
                len = onionize(verifiedNXTaddr,encodedsrvD,destsrvNXTaddr,outbuf,len);
                outbuf = encodedsrvD,strcpy(hopNXTaddr,destsrvNXTaddr);
                // we now have [dest privacyServer [dest]]
            }
            if ( 0 && L > 0 )
            {
                len = add_random_onionlayers(hopNXTaddr,L,verifiedNXTaddr,encodedL,outbuf,len);
                outbuf = encodedL;
                // we now have [L onion layers [dest privacyServer [dest]]]
            }
        }
        if ( has_privacyServer(np) != 0 ) // send via privacy server to protect our IP
        {
            len = onionize(verifiedNXTaddr,encodedP,srvNXTaddr,outbuf,len);
            outbuf = encodedP,strcpy(hopNXTaddr,srvNXTaddr);
        }
        sprintf(buf,"{\"status\":\"%s sends via %s encrypted sendmessage to %s pending\"}",verifiedNXTaddr,srvNXTaddr,destNXTaddr);
        txid = route_packet(Global_mp->udp,hopNXTaddr,outbuf,len);
        if ( txid == 0 )
        {
            sprintf(buf,"{\"error\":\"%s cant send via p2p sendmessage.(%s) [%s] to %s\"}",verifiedNXTaddr,origargstr,msg,destNXTaddr);
        }
        else
        {
            sprintf(buf,"{\"status\":\"%s sends via p2p encrypted sendmessage to %s pending\"}",verifiedNXTaddr,destNXTaddr);
        }
    }
    else sprintf(buf,"{\"error\":\"%s cant sendmessage.(%s) to %s illegal len\"}",verifiedNXTaddr,msg,destNXTaddr);
    return(clonestr(buf));
}

char *send_tokenized_cmd(char *hopNXTaddr,int32_t L,char *verifiedNXTaddr,char *NXTACCTSECRET,char *cmdstr,char *destNXTaddr)
{
    int n;
    char _tokbuf[4096];
    if ( strcmp("{\"result\":null}",cmdstr) == 0 )
    {
        printf("no need to send null JSON to %s\n",destNXTaddr);
        return(0);
    }
    n = construct_tokenized_req(_tokbuf,cmdstr,NXTACCTSECRET);
    if ( strcmp(verifiedNXTaddr,destNXTaddr) != 0 )
        hopNXTaddr[0] = 0;
    else if ( L != 0 )
        printf("send_tokenized_cmd L must be 0 for chanc3r boomerangs\n"), L = 0;
    return(sendmessage(hopNXTaddr,L,verifiedNXTaddr,_tokbuf,(int32_t)n+1,destNXTaddr,_tokbuf));
}

int32_t sendandfree_jsoncmd(int32_t L,char *sender,char *NXTACCTSECRET,cJSON *json,char *destNXTaddr)
{
    int32_t err = -1;
    cJSON *retjson;
    struct NXT_acct *np;
    char *msg,*retstr,errstr[512],hopNXTaddr[64],verifiedNXTaddr[64];
    verifiedNXTaddr[0] = 0;
    np = find_NXTacct(verifiedNXTaddr,NXTACCTSECRET);
    msg = cJSON_Print(json);
    stripwhite(msg,strlen(msg));
    retstr = send_tokenized_cmd(hopNXTaddr,L,verifiedNXTaddr,NXTACCTSECRET,msg,destNXTaddr);
    if ( retstr != 0 )
    {
        // printf("sendandfree_jsoncmd.(%s)\n",retstr);
        retjson = cJSON_Parse(retstr);
        if ( retjson != 0 )
        {
            if ( extract_cJSON_str(errstr,sizeof(errstr),retjson,"error") > 0 )
            {
                printf("error.(%s) sending (%s)\n",errstr,msg);
                err = -2;
            } else err = 0;
            free_json(retjson);
        } else printf("sendandfree_jsoncmd: cant parse (%s) from (%s)\n",retstr,msg);
        free(retstr);
    } else printf("sendandfree_jsoncmd: no retstr from (%s)\n",msg);
    free_json(json);
    free(msg);
    return(err);
}
#endif
