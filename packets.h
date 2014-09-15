//
//  packets.h
//  xcode
//
//  Created by jl777 on 7/19/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_packets_h
#define xcode_packets_h


struct peerinfo **Peers,**Pservers;
int32_t Numpeers,Numpservers;

// single threaded peer functions
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
        return("");
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
    }
    else
    {
        cJSON_AddItemToObject(json,"is_privacyServer",cJSON_CreateNumber(0));
        cJSON_AddItemToObject(json,"pubNXT",cJSON_CreateString(pubNXT));
        cJSON_AddItemToObject(json,"srvNXTaddr",cJSON_CreateString(srvnxtaddr));
    }
    cJSON_AddItemToObject(json,"srvipaddr",cJSON_CreateString(srvipaddr));
    sprintf(numstr,"%d",peer->srvport);
    cJSON_AddItemToObject(json,"srvport",cJSON_CreateString(numstr));
    cJSON_AddItemToObject(json,"sent",cJSON_CreateNumber(peer->numsent));
    cJSON_AddItemToObject(json,"recv",cJSON_CreateNumber(peer->numrecv));
    init_hexbytes(hexstr,peer->pubkey,sizeof(peer->pubkey));
    cJSON_AddItemToObject(json,"pubkey",cJSON_CreateString(hexstr));
    //init_hexbytes(hexstr,(void *)peer->udp,sizeof(peer->udp));
    cJSON_AddItemToObject(json,"udp",cJSON_CreateNumber((long long)peer->udp));
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
    char NXTaddr[64];
    int32_t createdflag,isPserver;
    struct NXT_acct *np;
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
        printf("Warning: add_peerinfo without nxtbits (%s %llu %s)\n",refpeer->pubBTCD,(long long)refpeer->pubnxtbits,refpeer->pubBTC);
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
    memcpy(peer->pubkey,pubkey,sizeof(peer->pubkey));
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
    printf("(%s) [%s] need to implement query (and propagation) mechanism for pubkeys\n",destNXTaddr,np->H.NXTaddr);
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
    //for (i=0; i<len;i++)
    //    printf("%02x",str[i]);
    // printf("Err.%d\n",err);
    *lenp = len - crypto_box_ZEROBYTES;
    return(err);
}

int32_t crcize(unsigned char *final,unsigned char *encoded,int32_t len)
{
    uint32_t crc = _crc32(0,encoded,len);
    memcpy(final,&crc,sizeof(crc));
    memcpy(final + sizeof(crc),encoded,len);
    //printf("crc.%08x for len.%d\n",crc,len);
    return(len + sizeof(crc));
}

char *send_tokenized_cmd(int32_t L,char *verifiedNXTaddr,char *NXTACCTSECRET,char *cmdstr,char *destNXTaddr)
{
    char _tokbuf[4096];
    int n = construct_tokenized_req(_tokbuf,cmdstr,NXTACCTSECRET);
    return(sendmessage(L,verifiedNXTaddr,NXTACCTSECRET,_tokbuf,(int32_t)n+1,destNXTaddr,_tokbuf));
}

int32_t sendandfree_jsoncmd(int32_t L,char *sender,char *NXTACCTSECRET,cJSON *json,char *destNXTaddr)
{
    int32_t err = -1;
    cJSON *retjson;
    struct NXT_acct *np;
    char *msg,*retstr,errstr[512],verifiedNXTaddr[64];
    verifiedNXTaddr[0] = 0;
    np = find_NXTacct(verifiedNXTaddr,NXTACCTSECRET);
    msg = cJSON_Print(json);
    stripwhite_ns(msg,strlen(msg));
    retstr = send_tokenized_cmd(L,verifiedNXTaddr,NXTACCTSECRET,msg,destNXTaddr);
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

int32_t onionize(char *verifiedNXTaddr,char *NXTACCTSECRET,unsigned char *encoded,char *destNXTaddr,unsigned char *payload,int32_t len)
{
    //static unsigned char zerokey[crypto_box_PUBLICKEYBYTES];
    unsigned char onetime_pubkey[crypto_box_PUBLICKEYBYTES],onetime_privkey[crypto_box_SECRETKEYBYTES];
    uint64_t nxt64bits;
    int32_t createdflag;
    uint16_t *payload_lenp,slen;
    struct NXT_acct *np;
    nxt64bits = calc_nxt64bits(destNXTaddr);
    np = get_NXTacct(&createdflag,Global_mp,destNXTaddr);
    /*if ( 0 && memcmp(np->mypeerinfo.pubkey,zerokey,sizeof(zerokey)) == 0 )
    {
        if ( Global_pNXT->privacyServer_NXTaddr[0] != 0 )
        {
            char cmdstr[4096];
            sprintf(cmdstr,"{\"requestType\":\"getpubkey\",\"NXT\":\"%s\",\"addr\":\"%s\",\"time\":%ld}",verifiedNXTaddr,destNXTaddr,time(NULL));
            send_tokenized_cmd(Global_mp->Lfactor,verifiedNXTaddr,NXTACCTSECRET,cmdstr,Global_pNXT->privacyServer_NXTaddr);
            //int n = construct_tokenized_req(_tokbuf,cmdstr,NXTACCTSECRET);
            //sendmessage(verifiedNXTaddr,NXTACCTSECRET,_tokbuf,(int32_t)n+1,Global_pNXT->privacyServer_NXTaddr,_tokbuf);
        }
        return(0);
    }*/
    crypto_box_keypair(onetime_pubkey,onetime_privkey);
    memcpy(encoded,&nxt64bits,sizeof(nxt64bits));
    encoded += sizeof(nxt64bits);
    memcpy(encoded,onetime_pubkey,sizeof(onetime_pubkey));
    encoded += sizeof(onetime_pubkey);
    payload_lenp = (uint16_t *)encoded;
    encoded += sizeof(*payload_lenp);
    printf("ONIONIZE: np.%p npudp.%p NXT.%s %s pubkey.%llx encode len.%d -> ",np,np->mypeerinfo.udp,np->H.NXTaddr,destNXTaddr,*(long long *)np->mypeerinfo.pubkey,len);
    len = _encode_str(encoded,(char *)payload,len,np->mypeerinfo.pubkey,onetime_privkey);
    slen = len;
    memcpy(payload_lenp,&slen,sizeof(*payload_lenp));
    printf("new len.%d + %ld = %ld\n",len,sizeof(*payload_lenp) + sizeof(onetime_pubkey) + sizeof(nxt64bits),sizeof(*payload_lenp) + sizeof(onetime_pubkey) + sizeof(nxt64bits)+len);
    return(len + sizeof(*payload_lenp) + sizeof(onetime_pubkey) + sizeof(nxt64bits));
}

int32_t add_random_onionlayers(int32_t numlayers,char *verifiedNXTaddr,char *NXTACCTSECRET,uint8_t *final,uint8_t *src,int32_t len)
{
    char destNXTaddr[64];
    uint8_t bufs[2][4096],*dest;
    struct peerinfo *peer;
    struct NXT_acct *np;
    if ( numlayers > 1 )
        numlayers = ((rand() >> 8) % numlayers);
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
            printf("add layer %d: NXT.%s\n",numlayers,np->H.NXTaddr);
            len = onionize(verifiedNXTaddr,NXTACCTSECRET,dest,np->H.NXTaddr,src,len);
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
            if ( cp != 0 )//&& strcmp(cp->privacyserver,"127.0.0.1") == 0 )
            {
                len = payload_len;
                err = _decode_cipher((char *)decoded,encoded,&len,pubkey,Global_mp->loopback_privkey);
                if ( err == 0 )
                {
                    //printf("2nd payload_len.%d err.%d new len.%d\n",payload_len,err,len);
                    return(len);
                }
            }
        } else printf("mismatched len expected %ld got %d\n",(payload_len + sizeof(payload_len) + sizeof(Global_mp->session_pubkey) + sizeof(mynxtbits)),len);
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

int32_t gen_tokenjson(CURL *curl_handle,char *jsonstr,char *NXTaddr,long nonce,char *NXTACCTSECRET,char *ipaddr,uint32_t port)
{
    int32_t createdflag;
    struct NXT_acct *np;
    char argstr[1024],pubkey[1024],token[1024];
    np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
    init_hexbytes(pubkey,np->mypeerinfo.pubkey,sizeof(np->mypeerinfo.pubkey));
    sprintf(argstr,"{\"NXT\":\"%s\",\"pubkey\":\"%s\",\"time\":%ld,\"yourip\":\"%s\",\"uport\":%d}",NXTaddr,pubkey,nonce,ipaddr,port);
    //printf("got argstr.(%s)\n",argstr);
    issue_generateToken(curl_handle,token,argstr,NXTACCTSECRET);
    token[NXT_TOKEN_LEN] = 0;
    sprintf(jsonstr,"[%s,{\"token\":\"%s\"}]",argstr,token);
    //printf("tokenized.(%s)\n",jsonstr);
    return((int32_t)strlen(jsonstr));
}

int32_t validate_token(CURL *curl_handle,char *pubkey,char *NXTaddr,char *tokenizedtxt,int32_t strictflag)
{
    cJSON *array=0,*firstitem=0,*tokenobj,*obj;
    int64_t timeval,diff = 0;
    int32_t valid,retcode = -13;
    char buf[4096],sender[64],*firstjsontxt = 0;
    unsigned char encoded[4096];
    array = cJSON_Parse(tokenizedtxt);
    if ( array == 0 )
    {
        printf("couldnt validate.(%s)\n",tokenizedtxt);
        return(-2);
    }
    if ( is_cJSON_Array(array) != 0 && cJSON_GetArraySize(array) == 2 )
    {
        firstitem = cJSON_GetArrayItem(array,0);
        if ( pubkey != 0 )
        {
            obj = cJSON_GetObjectItem(firstitem,"pubkey");
            copy_cJSON(pubkey,obj);
        }
        obj = cJSON_GetObjectItem(firstitem,"NXT"), copy_cJSON(buf,obj);
        if ( NXTaddr[0] != 0 && strcmp(buf,NXTaddr) != 0 )
            retcode = -3;
        else
        {
            strcpy(NXTaddr,buf);
            if ( strictflag != 0 )
            {
                timeval = get_cJSON_int(firstitem,"time");
                diff = timeval - time(NULL);
                if ( diff < 0 )
                    diff = -diff;
                if ( diff > strictflag )
                {
                    printf("time diff %lld too big %lld vs %ld\n",(long long)diff,(long long)timeval,time(NULL));
                    retcode = -5;
                }
            }
            if ( retcode != -5 )
            {
                firstjsontxt = cJSON_Print(firstitem), stripwhite_ns(firstjsontxt,strlen(firstjsontxt));
                tokenobj = cJSON_GetArrayItem(array,1);
                obj = cJSON_GetObjectItem(tokenobj,"token");
                copy_cJSON((char *)encoded,obj);
                memset(sender,0,sizeof(sender));
                valid = -1;
                if ( issue_decodeToken(curl_handle,sender,&valid,firstjsontxt,encoded) > 0 )
                {
                    if ( NXTaddr[0] == 0 )
                        strcpy(NXTaddr,sender);
                    if ( strcmp(sender,NXTaddr) == 0 )
                    {
                        printf("signed by valid NXT.%s valid.%d diff.%lld\n",sender,valid,(long long)diff);
                        retcode = valid;
                    }
                    else
                    {
                        printf("diff sender.(%s) vs NXTaddr.(%s)\n",sender,NXTaddr);
                        if ( strcmp(NXTaddr,buf) == 0 )
                            retcode = valid;
                    }
                }
                if ( retcode < 0 )
                    printf("err: signed by invalid sender.(%s) NXT.%s valid.%d or timediff too big diff.%lld, buf.(%s)\n",sender,NXTaddr,valid,(long long)diff,buf);
                free(firstjsontxt);
            }
        }
    }
    if ( array != 0 )
        free_json(array);
    return(retcode);
}

char *verify_tokenized_json(char *sender,int32_t *validp,cJSON **argjsonp,char *parmstxt)
{
    long len;
    unsigned char encoded[NXT_TOKEN_LEN+1];
    cJSON *secondobj,*tokenobj,*parmsobj;
    if ( ((*argjsonp)->type&0xff) == cJSON_Array && cJSON_GetArraySize(*argjsonp) == 2 )
    {
        secondobj = cJSON_GetArrayItem(*argjsonp,1);
        tokenobj = cJSON_GetObjectItem(secondobj,"token");
        copy_cJSON((char *)encoded,tokenobj);
        parmsobj = cJSON_DetachItemFromArray(*argjsonp,0);
        if ( parmstxt != 0 )
            free(parmstxt);
        parmstxt = cJSON_Print(parmsobj);
        len = strlen(parmstxt);
        stripwhite_ns(parmstxt,len);
        
        //printf("website.(%s) encoded.(%s) len.%ld\n",parmstxt,encoded,strlen(encoded));
        if ( strlen((char *)encoded) == NXT_TOKEN_LEN )
            issue_decodeToken(Global_mp->curl_handle2,sender,validp,parmstxt,encoded);
        if ( *argjsonp != 0 )
            free_json(*argjsonp);
        *argjsonp = parmsobj;
        return(parmstxt);
    }
    return(0);
}

struct NXT_acct *process_intro(uv_stream_t *connect,char *bufbase,int32_t sendresponse)
{
    int32_t portable_tcpwrite(uv_stream_t *stream,void *buf,long len,int32_t allocflag);
    int32_t n,retcode,createdflag;
    char retbuf[4096],pubkey[256],NXTaddr[64],argstr[1024],token[256];
    struct NXT_acct *np = 0;
    NXTaddr[0] = pubkey[0] = 0;
    struct coin_info *cp = get_coin_info("BTCD");
    if ( cp == 0 )
        return(0);
    if ( (retcode= validate_token(0,pubkey,NXTaddr,bufbase,15)) > 0 )
    {
        np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
        if ( np != 0 )
        {
            n = decode_hex(np->mypeerinfo.pubkey,(int32_t)sizeof(np->mypeerinfo.pubkey),pubkey);
            if ( n == crypto_box_PUBLICKEYBYTES )
            {
                printf("np.%p created.%d NXT.%s pubkey.%s (len.%d) connect.%p sendresponse.%d\n",np,createdflag,NXTaddr,pubkey,n,connect,sendresponse);
                if ( connect != 0 && sendresponse != 0 )
                {
                    //printf("call set_intro handle.%p %s.(%s)\n",connect,Server_NXTaddr,Server_secret);
                    //printf("got (%s)\n",retbuf);
                    init_hexbytes(pubkey,np->mypeerinfo.pubkey,sizeof(np->mypeerinfo.pubkey));
                    sprintf(argstr,"{\"NXT\":\"%s\",\"pubkey\":\"%s\",\"time\":%ld}",np->H.NXTaddr,pubkey,time(NULL));
                    //printf("got argstr.(%s)\n",argstr);
                    char pubNXT[64];
                    expand_nxt64bits(pubNXT,np->mypeerinfo.pubnxtbits);
                    if ( strcmp(NXTaddr,pubNXT) == 0 )
                        issue_generateToken(0,token,argstr,cp->NXTACCTSECRET);
                    else issue_generateToken(0,token,argstr,cp->srvNXTACCTSECRET);
                    token[NXT_TOKEN_LEN] = 0;
                    sprintf(retbuf,"[%s,{\"token\":\"%s\"}]",argstr,token);
                    //printf("send back.(%s)\n",retbuf);
                    if ( retbuf[0] == 0 )
                        printf("error generating intro??\n");
                    else portable_tcpwrite(connect,retbuf,(int32_t)strlen(retbuf)+1,ALLOCWR_ALLOCFREE);
                    //printf("after tcpwrite to %p (%s)\n",connect,retbuf);
                }
                return(np);
            }
        }
    }
    else
    {
        printf("token validation error.%d\n",retcode);
        if ( sendresponse != 0 )
        {
            sprintf(retbuf,"{\"error\":\"token validation error.%d\"}",retcode);
            portable_tcpwrite(connect,retbuf,(int32_t)strlen(retbuf)+1,ALLOCWR_ALLOCFREE);
        }
    }
    return(0);
}
/*
void tcpclient()
{
    if ( 1 && np == 0 )
    {
        if ( (np= process_intro(connect,(char *)buf->base,1)) != 0 )
        {
            connect->data = np;
            np->connect = connect;
            printf("set np.%p connect.%p\n",np,connect);
        }
        else
        {
            sprintf(retbuf,"{\"error\":\"validate_token error.%d\"}",retcode);
            portable_tcpwrite(connect,retbuf,(int32_t)strlen(retbuf)+1,ALLOCWR_ALLOCFREE);
        }
    }
    else
    {
        argjson = cJSON_Parse(buf->base);
        if ( argjson != 0 )
        {
            jsonstr = pNXT_jsonhandler(&argjson,buf->base,np->H.NXTaddr);
            if ( jsonstr == 0 )
                jsonstr = clonestr("{\"result\":\"pNXT_jsonhandler returns null\"}");
                printf("tcpwrite.(%s) to NXT.%s\n",jsonstr,np!=0?np->H.NXTaddr:"unknown");
                portable_tcpwrite(connect,jsonstr,(int32_t)strlen(jsonstr)+1,ALLOCWR_FREE);
                //free(jsonstr); completion frees, dont do it here!
                free_json(argjson);
                }
    }
    queue_enqueue(&RPC_6777_response,str);
}*/

void update_np_connectioninfo(struct NXT_acct *np,int32_t I_am_server,uv_stream_t *tcp,uv_stream_t *udp,const struct sockaddr *addr,char *sender,uint16_t port)
{
    if ( udp != 0 )
    {
        extern int Got_Server_UDP;
        Got_Server_UDP = 1;
        if ( np->mypeerinfo.udp == 0 )
            printf("set UDP.%p %s/%d\n",udp,sender,port);
        np->Uaddr = *addr;
        np->mypeerinfo.udp = udp;
        np->udp_port = port;
        safecopy(np->udp_sender,sender,sizeof(np->udp_sender));
    }
    else
    {
        if ( np->tcp == 0 )
            printf("set TCP.%p %s/%d\n",tcp,sender,port);
        np->addr = *addr;
        if ( I_am_server != 0 )
            np->connect = tcp;
        else np->tcp = tcp;
        np->tcp_port = port;
        safecopy(np->tcp_sender,sender,sizeof(np->tcp_sender));
    }
}

void queue_message(struct NXT_acct *np,char *msg,char *origmsg)
{
    queue_enqueue(&ALL_messages,clonestr(origmsg));
    if ( msg[0] != 0 )
    {
        queue_enqueue(&np->incomingQ,clonestr(msg));
        printf("QUEUE MESSAGES from NXT.%s (%s) size.%d\n",np->H.NXTaddr,msg,queue_size(&np->incomingQ));
    }
}

struct NXT_acct *process_packet(char *retjsonstr,struct NXT_acct *np,int32_t I_am_server,unsigned char *recvbuf,int32_t recvlen,uv_stream_t *tcp,uv_stream_t *udp,const struct sockaddr *addr,char *sender,uint16_t port)
{
    uint64_t destbits = 0;
    struct NXT_acct *tokenized_np;
    int32_t valid,len=0,tmp,createdflag,decrypted=0;
    cJSON *argjson;//,*msgjson;
    struct coin_info *cp = get_coin_info("BTCD");
    unsigned char pubkey[crypto_box_PUBLICKEYBYTES],tmppubkey[crypto_box_PUBLICKEYBYTES],decoded[4096],tmpdecoded[4096];
    char senderNXTaddr[64],destNXTaddr[64],*parmstxt=0,*jsonstr;
    if ( cp == 0 )
        return(0);
    memset(decoded,0,sizeof(decoded));
    memset(tmpdecoded,0,sizeof(tmpdecoded));
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
            if ( (tmp= deonionize(tmppubkey,tmpdecoded,decoded,len,0)) > 0 )
            {
                memcpy(&destbits,decoded,sizeof(destbits));
                decrypted++;
                len = tmp;
                memcpy(decoded,tmpdecoded,len);
                memcpy(pubkey,tmppubkey,sizeof(pubkey));
                printf("decrypted2 len.%d dest.(%llu)\n",len,(long long)destbits);
            } else printf("decrypted len.%d dest.(%llu)\n",len,(long long)destbits);
        }
        else return(0);
    }
    else if ( np == 0 && (tcp != 0 || udp != 0) )
    {
        if ( (np= process_intro(tcp,(char *)recvbuf,(I_am_server!=0 && tcp!=0) ? 1 : 0)) != 0 )
        {
            printf("process_intro got NXT.(%s) np.%p <- I_am_server.%d UDP %p TCP.%p %s/%d\n",np->H.NXTaddr,np,I_am_server,udp,tcp,sender,port);
            update_np_connectioninfo(np,I_am_server,tcp,udp,addr,sender,port);
            if ( I_am_server == 0 && strcmp(cp->privacyserver,sender) == 0 )
            {
                void set_pNXT_privacyServer_NXTaddr(char *NXTaddr);
                set_pNXT_privacyServer_NXTaddr(np->H.NXTaddr);
                strcpy(retjsonstr,(char *)recvbuf);
            }
            else retjsonstr[0] = 0;
            return(np);
        } else printf("on_udprecv: process_intro returns null?\n");
    }
    else
    {
        printf("process_packet got unexpected nonencrypted len.%d NXT.(%s) np.%p <- I_am_server.%d UDP %p TCP.%p %s/%d\n",recvlen,np!=0?np->H.NXTaddr:"noaddr",np,I_am_server,udp,tcp,sender,port);
        return(0);
    }
    if ( len > 0 )
    {
        decoded[len] = 0;
        parmstxt = clonestr((char *)decoded);//rcvbuf->base;
        argjson = cJSON_Parse(parmstxt);
        printf("[%s] argjson.%p I_am_server.%d tcp.%p udp.%p\n",parmstxt,argjson,I_am_server,tcp,udp);
        if ( argjson != 0 )
        {
            parmstxt = verify_tokenized_json(senderNXTaddr,&valid,&argjson,parmstxt);
            if ( valid != 0 && parmstxt != 0 && parmstxt[0] != 0 )
            {
                tokenized_np = get_NXTacct(&createdflag,Global_mp,senderNXTaddr);
                if ( np != 0 && tokenized_np != np )
                    printf("np.%p NXT.%s != tokenized_np.%p NXT.%s\n",np,np->H.NXTaddr,tokenized_np,tokenized_np->H.NXTaddr);
                else
                {
                    np = tokenized_np;
                    if ( tcp != 0 || udp != 0 )
                        update_np_connectioninfo(np,I_am_server,tcp,udp,addr,sender,port);
                    if ( 0 && decrypted != 0 && memcmp(np->mypeerinfo.pubkey,pubkey,sizeof(pubkey)) != 0 ) // we dont do it above to prevent spoofing
                    {
                        printf("update pubkey for NXT.%s to %llx\n",senderNXTaddr,*(long long *)pubkey);
                        memcpy(np->mypeerinfo.pubkey,pubkey,sizeof(np->mypeerinfo.pubkey));
                    }
                    /*if ( I_am_server == 0 )
                    {
                        printf("QUEUEALLMESSAGES.(%s) size.%d\n",parmstxt,queue_size(&ALL_messages));
                        msgjson = cJSON_GetObjectItem(argjson,"msg");
                        copy_cJSON(msg,msgjson);
                        queue_message(np,msg,parmstxt);
                    }
                    else*/
                    {
                        char *issue_pNXT_json_commands(cJSON *argjson,char *sender,int32_t valid,char *origargstr);
                        jsonstr = issue_pNXT_json_commands(argjson,np->H.NXTaddr,valid,parmstxt);//(char *)decoded);
                        if ( jsonstr == 0 )
                        {
                            //sprintf(retjsonstr,"{\"result\":\"pNXT_jsonhandler returns null from (%s)\"}",(char *)decoded);
                        }
                        else
                        {
                            strcpy(retjsonstr,jsonstr);
                            free(jsonstr);
                        }
                        //printf("respond.(%s) to np.%p NXT.%s argjson.%p\n",retjsonstr,np,np!=0?np->H.NXTaddr:"unknown",argjson);
                    }
                }
            }
            else
            {
                printf("valid.%d routine non-tokenized message.(%s)\n",valid,decoded);
                //strcpy(retjsonstr,(char *)decoded);
            }
            free_json(argjson);
            return(np);
        }
        else if ( udp != 0 || (I_am_server != 0 && tcp != 0) ) // test to make sure we are hub and not p2p broadcast
        {
            unsigned char finalbuf[4096];
            memcpy(&destbits,decoded,sizeof(destbits));
            if ( destbits != 0 ) // route packet
            {
                expand_nxt64bits(destNXTaddr,destbits);
                np = get_NXTacct(&createdflag,Global_mp,destNXTaddr);
                printf("destaddr.(%s) np->udp %p\n",destNXTaddr,np->mypeerinfo.udp);
                if ( np->mypeerinfo.udp != 0 )
                {
                    printf("route packet to NXT.%s\n",destNXTaddr);
                    len = crcize(finalbuf,decoded,len);
                    if ( len > sizeof(finalbuf) )
                    {
                        printf("sendmessage: len.%d > sizeof(finalbuf) %ld\n",len,sizeof(finalbuf));
                        exit(-1);
                    }
                    if ( np != 0 && len < 1400 && np->mypeerinfo.udp != 0 )
                    {
                        int32_t portable_udpwrite(const struct sockaddr *addr,uv_udp_t *handle,void *buf,long len,int32_t allocflag);
                        printf("udpsend finalbuf.%d\n",len);
                        portable_udpwrite(&np->Uaddr,(uv_udp_t *)np->mypeerinfo.udp,finalbuf,len,ALLOCWR_ALLOCFREE);
                    }
                    //else sendmessage(0,cp->srvNXTADDR,cp->srvNXTACCTSECRET,(char *)decoded,len,destNXTaddr,0);
                    //int32_t portable_udpwrite(const struct sockaddr *addr,uv_udp_t *handle,void *buf,long len,int32_t allocflag);
                    //len = crcize(crcbuf,decoded,len);
                    //portable_udpwrite(&np->Uaddr,(uv_udp_t *)np->udp,crcbuf,len,ALLOCWR_ALLOCFREE);
                    //sprintf(retjsonstr,"{\"result\":\"routed packet from %s/%d to dest.%llu %d bytes from %s/%d\"}",sender,port,(long long)destbits,len,sender,port);
                }
                return(np);
            }
            else
            {
                printf("{\"error\":\"unknown dest.%llu %d bytes from %s/%d\"}",(long long)destbits,recvlen,sender,port);
            }
        }
        if ( parmstxt != 0 )
            free(parmstxt);
    }
    else printf("process_packet got unexpected recvlen.%d NXT.(%s) np.%p <- I_am_server.%d UDP %p TCP.%p %s/%d\n",recvlen,np!=0?np->H.NXTaddr:"noaddr",np,I_am_server,udp,tcp,sender,port);
    return(0);
}

char *sendmessage(int32_t L,char *verifiedNXTaddr,char *NXTACCTSECRET,char *msg,int32_t msglen,char *destNXTaddr,char *origargstr)
{
    uint64_t txid;
    char buf[4096];
    unsigned char encodedD[4096],encodedL[4096],encodedP[4096],finalbuf[4096],*outbuf;
    int32_t len,createdflag;
    struct NXT_acct *np = 0,*destnp;
    struct coin_info *cp = get_coin_info("BTCD");
    
    if ( cp == 0 )
        return(clonestr("\"error\":\"no cp for sendmessage\"}"));
    //printf("sendmessage.(%s) -> NXT.(%s) (%s) (%s)\n",NXTaddr,destNXTaddr,msg,origargstr);
    /*if ( Server_NXTaddr == 0 )
    {
        if ( Global_pNXT->privacyServer_NXTaddr[0] == 0 )
        {
            sprintf(buf,"{\"error\":\"%s cant sendmessage.(%s) to null privacyServer\"}",verifiedNXTaddr,msg);
            return(clonestr(buf));
        }
        np = get_NXTacct(&createdflag,Global_mp,Global_pNXT->privacyServer_NXTaddr);
        printf("set np <- NXT.%s\n",Global_pNXT->privacyServer_NXTaddr);
    }
    else*/
    {
        np = get_NXTacct(&createdflag,Global_mp,verifiedNXTaddr);
        /*if ( strcmp(cp->srvNXTaddr,destNXTaddr) == 0 )
        {
            queue_message(np,msg,origargstr);
            sprintf(buf,"{\"result\":\"msg.(%s) from NXT.%s queued\"}",msg,verifiedNXTaddr);
            return(clonestr(buf));
        }*/
    }
    destnp = get_NXTacct(&createdflag,Global_mp,destNXTaddr);
    memset(finalbuf,0,sizeof(finalbuf));
    memset(encodedD,0,sizeof(encodedD)); // encoded to dest
    memset(encodedL,0,sizeof(encodedL)); // encoded to max L onion layers
    memset(encodedP,0,sizeof(encodedP)); // encoded to privacyserver
    if ( origargstr != 0 )
        len = onionize(verifiedNXTaddr,NXTACCTSECRET,encodedD,destNXTaddr,(unsigned char *)origargstr,(int32_t)strlen(origargstr)+1);
    else
    {
        len = onionize(verifiedNXTaddr,NXTACCTSECRET,encodedD,destNXTaddr,(unsigned char *)msg,msglen);
        msg = origargstr = "<encrypted>";
    }
    printf("sendmessage (%s) len.%d to %s\n",origargstr,msglen,destNXTaddr);
    if ( len > sizeof(finalbuf)-256 )
    {
        printf("sendmessage, payload too big %d\n",len);
        sprintf(buf,"{\"error\":\"%s cant sendmessage.(%s) to %s too long.%d\"}",verifiedNXTaddr,msg,destNXTaddr,len);
    }
    else if ( len > 0 )
    {
        outbuf = encodedD;
        printf("np.%p NXT.%s np->udp %p | destnp.%p destnp_udp.%p\n",np,np!=0?np->H.NXTaddr:"no np",np!=0?np->mypeerinfo.udp:0,destnp,destnp!=0?destnp->mypeerinfo.udp:0);
        if ( np != 0 && np->mypeerinfo.udp != 0 )
        {
            if ( L > 0 )
            {
                len = add_random_onionlayers(L,verifiedNXTaddr,NXTACCTSECRET,encodedL,outbuf,len);
                outbuf = encodedL;
            }
            len = onionize(verifiedNXTaddr,NXTACCTSECRET,encodedP,cp->srvNXTADDR,outbuf,len);
            outbuf = encodedP;
            sprintf(buf,"{\"status\":\"%s sends via %s encrypted sendmessage to %s pending\"}",verifiedNXTaddr,cp->srvNXTADDR,destNXTaddr);
        }
        else if ( cp->srvNXTADDR[0] != 0 && destnp->mypeerinfo.udp != 0 )
        {
            printf("can do direct!\n");
            np = destnp;
            sprintf(buf,"{\"status\":\"%s sends direct encrypted sendmessage to %s pending\"}",verifiedNXTaddr,destNXTaddr);
        } else np = 0;  // have to use p2p network
        if ( len > 0 && len < sizeof(finalbuf) )
        {
            len = crcize(finalbuf,outbuf,len);
            if ( len > sizeof(finalbuf) )
            {
                printf("sendmessage: len.%d > sizeof(finalbuf) %ld\n",len,sizeof(finalbuf));
                exit(-1);
            }
            if ( np != 0 && len < 1400 )
            {
                int32_t portable_udpwrite(const struct sockaddr *addr,uv_udp_t *handle,void *buf,long len,int32_t allocflag);
                printf("udpsend finalbuf.%d\n",len);
                portable_udpwrite(&np->Uaddr,(uv_udp_t *)np->mypeerinfo.udp,finalbuf,len,ALLOCWR_ALLOCFREE);
            }
            else if ( cp->srvNXTADDR[0] != 0 ) // test to verify this is hub
            {
                printf("len.%d Server_NXTaddr.(%s) broadcast %d via p2p\n",len,cp->srvNXTADDR,len);
                txid = call_libjl777_broadcast((char *)finalbuf,600);
                if ( txid == 0 )
                {
                    sprintf(buf,"{\"error\":\"%s cant send via p2p sendmessage.(%s) [%s] to %s pending\"}",verifiedNXTaddr,origargstr,msg,destNXTaddr);
                }
                else
                {
                    sprintf(buf,"{\"status\":\"%s sends via p2p encrypted sendmessage to %s pending\"}",verifiedNXTaddr,destNXTaddr);
                }
            }
            else sprintf(buf,"{\"error\":\"%s cant sendmessage.(%s) to %s unexpected case\"}",verifiedNXTaddr,msg,destNXTaddr);
        } else sprintf(buf,"{\"error\":\"%s cant sendmessage.(%s) to %s error encoding layer, len.%d\"}",verifiedNXTaddr,msg,destNXTaddr,len);
    } else sprintf(buf,"{\"error\":\"%s cant sendmessage.(%s) to %s probably no pubkey\"}",verifiedNXTaddr,msg,destNXTaddr);
    return(clonestr(buf));
}

#endif
