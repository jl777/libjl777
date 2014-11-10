//"findnode","key":"4567308492595024585","data":"deadbeef"}'
//
//  packets.h
//  xcode
//
//  Created by jl777 on 7/19/14.
//  Copyright (c) 2014 jl777. MIT license.
//

#ifndef xcode_packets_h
#define xcode_packets_h

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

int32_t deonionize(unsigned char *pubkey,unsigned char *decoded,unsigned char *encoded,int32_t len,char *senderip,int32_t port)
{
    int32_t err;
    uint64_t packetdest;
    struct coin_info *cp;
    uint16_t payload_len;
    cp = get_coin_info("BTCD");
    memcpy(&packetdest,encoded,sizeof(packetdest));
    if ( packetdest == 0 || ((packetdest == cp->srvpubnxtbits && notlocalip(cp->privacyserver) == 0) || packetdest == cp->privatebits) )
    {
        encoded += sizeof(packetdest);
        memcpy(pubkey,encoded,crypto_box_PUBLICKEYBYTES);
        encoded += crypto_box_PUBLICKEYBYTES;
        memcpy(&payload_len,encoded,sizeof(payload_len));
        encoded += sizeof(payload_len);
        if ( Debuglevel > 1 )
            printf("packedest.%llu srvpub.%llu (%s:%d) payload_len.%d\n",(long long)packetdest,(long long)cp->srvpubnxtbits,senderip,port,payload_len);
        if ( payload_len > 0 && (payload_len + sizeof(payload_len) + sizeof(Global_mp->loopback_pubkey) + sizeof(packetdest)) <= len )
        {
            len = payload_len;
            if ( packetdest == 0 || packetdest == cp->srvpubnxtbits  )
            {
                err = _decode_cipher((char *)decoded,encoded,&len,pubkey,Global_mp->loopback_privkey);
                if ( err == 0 )
                {
                    if ( Debuglevel > 2 )
                        printf("srvpubnxtbits payload_len.%d err.%d new len.%d\n",payload_len,err,len);
                    return(len);
                }
            }
            err = _decode_cipher((char *)decoded,encoded,&len,pubkey,Global_mp->myprivkey.bytes);
            if ( err == 0 )
            {
                if ( Debuglevel > 2 )
                    printf("payload_len.%d err.%d new len.%d\n",payload_len,err,len);
                return(len);
            }
        } else printf("mismatched len expected %ld got %d from (%s:%d)\n",(payload_len + sizeof(payload_len) + sizeof(Global_mp->loopback_pubkey) + sizeof(packetdest)),len,senderip,port);
    }
    else printf("deonionize onion for NXT.%llu not this address.(%llu)\n",(long long)packetdest,(long long)cp->srvpubnxtbits);
    return(0);
}

int32_t direct_onionize(uint64_t nxt64bits,unsigned char *destpubkey,unsigned char *maxbuf,unsigned char *encoded,unsigned char **payloadp,int32_t len)
{
    unsigned char *origencoded,onetime_pubkey[crypto_box_PUBLICKEYBYTES],onetime_privkey[crypto_box_SECRETKEYBYTES],*payload = (*payloadp);
    uint16_t *payload_lenp,*max_lenp,slen,slen2;
    int32_t padlen,origlen,onlymax = 0;
    long hdrlen,extralen;
    memset(maxbuf,0,MAX_UDPLEN);
    origencoded = encoded;
    extralen = (sizeof(*payload_lenp) + crypto_box_PUBLICKEYBYTES + sizeof(nxt64bits));
    padlen = (int32_t)(MAX_UDPLEN - (len + crypto_box_ZEROBYTES + crypto_box_NONCEBYTES) - extralen - sizeof(uint32_t));
    if ( padlen < 0 )
        padlen = 0;
    if ( encoded == 0 )
    {
        encoded = maxbuf;
        onlymax = 1;
    }
    (*payloadp) = encoded;
    crypto_box_keypair(onetime_pubkey,onetime_privkey);
    memcpy(encoded,&nxt64bits,sizeof(nxt64bits));
    encoded += sizeof(nxt64bits);
    memcpy(encoded,onetime_pubkey,sizeof(onetime_pubkey));
    encoded += sizeof(onetime_pubkey);
    payload_lenp = (uint16_t *)encoded;
    if ( onlymax == 0 )
    {
        hdrlen = (long)(encoded - origencoded);
        memcpy(maxbuf,origencoded,hdrlen);
        maxbuf += hdrlen;
        max_lenp = (uint16_t *)maxbuf;
        maxbuf += sizeof(*payload_lenp);
    } else max_lenp = payload_lenp;
    encoded += sizeof(*payload_lenp);
    if ( Debuglevel > 1 )
    {
        char hexstr[1024];
        init_hexbytes_noT(hexstr,destpubkey,crypto_box_PUBLICKEYBYTES);
        hexstr[16] = 0;
        printf("DIRECT ONIONIZE: pubkey.%s encode len.%d padlen.%d -> dest.%llu ",hexstr,len,padlen,(long long)nxt64bits);
    }
    if ( onlymax != 0 )
    {
        len = _encode_str(encoded,(char *)payload,len+padlen,destpubkey,onetime_privkey);
        slen2 = slen = len;
        memcpy(payload_lenp,&slen,sizeof(*payload_lenp));
    }
    else
    {
        origlen = len;
        len = _encode_str(encoded,(char *)payload,len,destpubkey,onetime_privkey);
        slen = len;
        memcpy(payload_lenp,&slen,sizeof(*payload_lenp));
        if ( maxbuf != 0 )
        {
            if ( padlen > 0 )
            {
                slen2 = _encode_str(maxbuf,(char *)payload,origlen + padlen,destpubkey,onetime_privkey);
                memcpy(max_lenp,&slen2,sizeof(*max_lenp));
            }
            else memcpy(maxbuf,encoded,len);
        }
    }
    if ( Debuglevel > 1 )
        printf("new len.%d + %ld = %d (%d %d)\n",len,extralen,padlen,*payload_lenp,*max_lenp);
    return(len + (int)extralen);
}

int32_t onionize(char *hopNXTaddr,unsigned char *maxbuf,unsigned char *encoded,char *destNXTaddr,unsigned char **payloadp,int32_t len)
{
    uint64_t nxt64bits;
    int32_t createdflag;
    struct NXT_acct *np;
    strcpy(hopNXTaddr,destNXTaddr);
    nxt64bits = calc_nxt64bits(destNXTaddr);
    np = get_NXTacct(&createdflag,Global_mp,destNXTaddr);
    return(direct_onionize(nxt64bits,np->stats.pubkey,maxbuf,encoded,payloadp,len));
}

int32_t pserver_canhop(struct pserver_info *pserver,char *hopNXTaddr)
{
    int32_t createdflag,i = -1;
    uint32_t *hasips;
    struct NXT_acct *np;
    np = get_NXTacct(&createdflag,Global_mp,hopNXTaddr);
    if ( (hasips= pserver->hasips) != 0 )
    {
        for (i=0; i<pserver->numips&&i<(int)(sizeof(pserver->hasips)/sizeof(*pserver->hasips)); i++)
            if ( hasips[i] == np->stats.ipbits )
            {
                char ipaddr[16];
                expand_ipbits(ipaddr,hasips[i]);
                if ( Debuglevel > 1 )
                    printf(">>>>>>>>>>> HASIP.%s in slot %d of %d\n",ipaddr,i,pserver->numips);
                return(i);
            }
    }
    return(i);
}

int32_t add_random_onionlayers(char *hopNXTaddr,int32_t numlayers,uint8_t *maxbuf,uint8_t *final,uint8_t **srcp,int32_t len)
{
    char ipaddr[64],NXTaddr[64];
    int32_t tmp,origlen=0,maxlen = 0;
    uint8_t dest[4096],srcbuf[4096],*src = srcbuf;
    struct nodestats *stats;
    struct pserver_info *pserver;
//return(0);
    if ( numlayers > 1 )
    {
        tmp = ((rand() >> 8) % numlayers);
        if ( numlayers > 3 && tmp < 3 )
            tmp = 3;
        numlayers = tmp;
    }
    if ( numlayers > 0 )
    {
        if ( numlayers > MAX_ONION_LAYERS )
            numlayers = MAX_ONION_LAYERS;
        if ( Debuglevel > 0 )
            printf("add_random_onionlayers %d of %d *srcp %p\n",numlayers,Global_mp->Lfactor,*srcp);
        memset(dest,0,sizeof(dest));
        memcpy(srcbuf,*srcp,len);
        while ( numlayers > 0 )
        {
            stats = get_random_node();
            if ( stats == 0 )
            {
                printf("FATAL: cant get random node!\n");
                return(maxlen);
            }
            expand_ipbits(ipaddr,stats->ipbits);
            if ( stats->ipbits == 0 || stats->nxt64bits == 0 || (pserver= get_pserver(0,ipaddr,0,0)) == 0 || pserver_canhop(pserver,hopNXTaddr) < 0 )
                continue;
            expand_nxt64bits(NXTaddr,stats->nxt64bits);
            if ( strcmp(hopNXTaddr,NXTaddr) != 0 )
            {
                if ( Debuglevel > 0 )
                    printf("add layer %d: NXT.(%s) -> (%s) [%s] len.%d origlen.%d maxlen.%d\n",numlayers,NXTaddr,hopNXTaddr,ipaddr,len,origlen,maxlen);
                origlen = len;
                src = srcbuf;
                len = onionize(hopNXTaddr,maxbuf,dest,NXTaddr,&src,len);
                memcpy(srcbuf,dest,len);
                memcpy(final,dest,len);
                
                if ( 0 )
                {
                    src = srcbuf;
                    maxlen = onionize(hopNXTaddr,maxbuf,0,NXTaddr,&src,origlen);
                    *srcp = maxbuf;
                }
                else if ( final != 0 )
                    *srcp = final;
                else *srcp = maxbuf;
                if ( final == 0 )
                    break;
                if ( len > 4096 )
                {
                    printf("FATAL: onion layers too big.%d\n",len);
                    return(-1);
                }
                else if ( len > MAX_UDPLEN-128 )
                    break;
            }
            numlayers--;
        }
    }
    if ( 0 ) return(maxlen);
    return(len);
}

int32_t has_privacyServer(struct NXT_acct *np)
{
    if ( np->stats.ipbits != 0 && np->stats.nxt64bits != 0 )
        return(1);
    else return(0);
}

// I will add support in sendmsg so you can send to NXT addr (numerical/RS), contact handle, IP addr, maybe even NXT alias
char *sendmessage(char *hopNXTaddr,int32_t L,char *verifiedNXTaddr,char *msg,int32_t msglen,char *destNXTaddr,unsigned char *data,int32_t datalen)
{
    uint64_t txid;
    char buf[4096],destsrvNXTaddr[64],srvNXTaddr[64],_hopNXTaddr[64];
    unsigned char maxbuf[4096],encodedD[4096],encoded[4096],encodedL[4096],encodedF[4096],*outbuf;
    int32_t len,createdflag;//,maxlen;
    struct NXT_acct *np,*destnp;
    np = get_NXTacct(&createdflag,Global_mp,verifiedNXTaddr);
    expand_nxt64bits(srvNXTaddr,np->stats.nxt64bits);
    destnp = get_NXTacct(&createdflag,Global_mp,destNXTaddr);
    if ( hopNXTaddr == 0 )
        hopNXTaddr = _hopNXTaddr, hopNXTaddr[0] = 0;
    if ( np == 0 || destnp == 0 || Global_mp->udp == 0 || destnp->stats.nxt64bits == 0 )
    {
        sprintf(buf,"\"error\":\"no np.%p or global udp.%p for sendmessage || %s destnp->stats.nxtbits %llu == 0\"}",np,Global_mp->udp,destNXTaddr,(long long)destnp->stats.nxt64bits);
        return(clonestr(buf));
    }
    expand_nxt64bits(destsrvNXTaddr,destnp->stats.nxt64bits);
    memset(maxbuf,0,sizeof(maxbuf)); // always the same size
    memset(encoded,0,sizeof(encoded)); // always the same size
    memset(encodedD,0,sizeof(encodedD)); // encoded to dest
    memset(encodedL,0,sizeof(encodedL)); // encoded to onion layers
    memset(encodedF,0,sizeof(encodedF)); // encoded to prefinal
    memcpy(encoded,msg,msglen);
    outbuf = encoded;//(unsigned char *)msg;
    len = msglen;
    if ( data != 0 && datalen > 0 ) // must properly handle "data" field, eg. set it to "data":%d <- datalen
    {
        memcpy(outbuf+msglen,data,datalen);
        len += datalen;
    }
    txid = calc_txid(outbuf,len);
    //init_jsoncodec((char *)outbuf,msglen);
    if ( Debuglevel > 1 )
        printf("\nsendmessage (%s) len.%d to %s crc.%x\n",msg,msglen,destNXTaddr,_crc32(0,outbuf,len));
    if ( len > sizeof(maxbuf)-1024 )
    {
        printf("sendmessage, payload too big %d\n",len);
        sprintf(buf,"{\"error\":\"%s cant sendmessage.(%s) to %s too long.%d\"}",verifiedNXTaddr,msg,destNXTaddr,len);
    }
    else if ( len > 0 )
    {
        if ( 0*L > 0 )
        {
            len = onionize(hopNXTaddr,maxbuf,encodedD,destNXTaddr,&outbuf,len);
            if ( (len= add_random_onionlayers(hopNXTaddr,L,maxbuf,encodedL,&outbuf,len)) == 0 )
            {
                outbuf = (unsigned char *)msg;
                //len = onionize(hopNXTaddr,maxbuf,0,destNXTaddr,&outbuf,msglen);
                len = onionize(hopNXTaddr,maxbuf,encodedF,destNXTaddr,&outbuf,msglen);
            }
        }
        else
        {
           // len = onionize(hopNXTaddr,maxbuf,0,destNXTaddr,&outbuf,len);
            len = onionize(hopNXTaddr,maxbuf,encodedF,destNXTaddr,&outbuf,len);
        }
        len = onionize(hopNXTaddr,maxbuf,0,destNXTaddr,&outbuf,len);
        route_packet(1,0,hopNXTaddr,outbuf,len);
        if ( txid == 0 )
            sprintf(buf,"{\"error\":\"%s cant sendmessage.(%s) to %s, len.%d\"}",verifiedNXTaddr,msg,destNXTaddr,len);
        else sprintf(buf,"{\"status\":\"%s sends encrypted sendmessage to %s pending via.(%s), len.%d\"}",verifiedNXTaddr,destNXTaddr,hopNXTaddr,len);
    }
    else sprintf(buf,"{\"error\":\"%s cant sendmessage.(%s) to %s illegal len\"}",verifiedNXTaddr,msg,destNXTaddr);
    return(clonestr(buf));
}

char *send_tokenized_cmd(char *hopNXTaddr,int32_t L,char *verifiedNXTaddr,char *NXTACCTSECRET,char *cmdstr,char *destNXTaddr)
{
    char _tokbuf[4096],datastr[MAX_JSON_FIELD],*cmd = cmdstr;
    unsigned char databuf[4096],*data = 0;
    int32_t n,len,datalen = 0;
    cJSON *json;
    if ( strcmp("{\"result\":null}",cmdstr) == 0 )
    {
        printf("no need to send null JSON to %s\n",destNXTaddr);
        return(0);
    }
    if ( (json= cJSON_Parse(cmdstr)) != 0 )
    {
        copy_cJSON(datastr,cJSON_GetObjectItem(json,"data"));
        len = (int32_t)strlen(datastr);
        if ( len >= 2 && (len & 1) == 0 && is_hexstr(datastr) != 0 )
        {
            datalen = (len >> 1);
            data = databuf;
            decode_hex(data,datalen,datastr);
            cJSON_ReplaceItemInObject(json,"data",cJSON_CreateNumber(datalen));
            cmd = cJSON_Print(json);
            stripwhite_ns(cmd,strlen(cmd));
            printf("cmdstr.(%s) -> (%s)\n",cmdstr,cmd);
        }
        free_json(json);
    }
    memset(_tokbuf,0,sizeof(_tokbuf));
    n = construct_tokenized_req(_tokbuf,cmd,NXTACCTSECRET);
    if ( cmd != cmdstr )
        free(cmd), cmd = cmdstr;
    {
        char sender[64],*parmstxt;
        int32_t valid;
        cJSON *json = cJSON_Parse(_tokbuf);
        if ( json != 0 )
        {
            parmstxt = verify_tokenized_json(0,sender,&valid,json);
            if ( valid <= 0 )
                printf("_tokbuf.%s valid.%d sender.(%s)\n",_tokbuf,valid,sender);
            if ( parmstxt != 0 )
                free(parmstxt);
            free_json(json);
        }
    }
    return(sendmessage(hopNXTaddr,L,verifiedNXTaddr,_tokbuf,(int32_t)n+1,destNXTaddr,data,datalen));
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
    stripwhite_ns(msg,strlen(msg));
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


struct NXT_acct *process_packet(int32_t internalflag,char *retjsonstr,unsigned char *recvbuf,int32_t recvlen,uv_udp_t *udp,struct sockaddr *prevaddr,char *sender,uint16_t port)
{
    struct coin_info *cp = get_coin_info("BTCD");
    uint64_t destbits = 0;
    struct NXT_acct *tokenized_np = 0;
    int32_t valid,len=0,len2,createdflag,datalen,parmslen,encrypted = 1;
    cJSON *tmpjson,*valueobj,*argjson = 0;
    unsigned char pubkey[crypto_box_PUBLICKEYBYTES],pubkey2[crypto_box_PUBLICKEYBYTES],decoded[4096],decoded2[4096],tmpbuf[4096],maxbuf[4096],*outbuf;
    char senderNXTaddr[64],datastr[4096],hopNXTaddr[64],destNXTaddr[64],checkstr[MAX_JSON_FIELD],datalenstr[MAX_JSON_FIELD];
    char *parmstxt=0,*jsonstr;
    struct pserver_info *pserver;
    pserver = get_pserver(0,sender,0,0);
    memset(decoded,0,sizeof(decoded));
    memset(tmpbuf,0,sizeof(tmpbuf));
    retjsonstr[0] = 0;
    //sprintf(retjsonstr,"{\"error\":\"unknown error processing %d bytes from %s/%d\"}",recvlen,sender,port);
    if ( is_encrypted_packet(recvbuf,recvlen) != 0 )
    {
        recvbuf += sizeof(uint32_t);
        recvlen -= sizeof(uint32_t);
        if ( (len= deonionize(pubkey,decoded,recvbuf,recvlen,sender,port)) > 0 )
        {
            memcpy(&destbits,decoded,sizeof(destbits));
            while ( cp != 0 && (destbits == 0 || destbits == cp->privatebits || destbits == cp->srvpubnxtbits) )
            {
                memset(decoded2,0,sizeof(decoded2));
                if ( (len2= deonionize(pubkey2,decoded2,decoded,len,sender,port)) > 0 )
                {
                    memcpy(&destbits,decoded2,sizeof(destbits));
                    if ( Debuglevel > 1 )
                        printf("decrypted2 len2.%d dest2.(%llu)\n",len2,(long long)destbits);
                    len = len2;
                    memcpy(decoded,decoded2,len);
                    memcpy(pubkey,pubkey2,sizeof(pubkey));
                }
                else
                {
                    if ( Debuglevel > 1 )
                        printf("couldnt decrypt2 packet len.%d to %llu\n",len,(long long)destbits);
                    if ( destbits == cp->privatebits || destbits == cp->srvpubnxtbits )
                        return(0);
                    break;
                }
            }
            pserver->decrypterrs = 0;
        }
        else
        {
            if ( (++pserver->decrypterrs % 10) == 0 && pserver->decrypterrs < 100 )
                send_kademlia_cmd(0,pserver,"ping",cp->srvNXTACCTSECRET,0,0);
            if ( Debuglevel > 0 )
                printf("couldnt decrypt packet len.%d from (%s)\n",recvlen,sender);
            return(0);
        }
    }
    else if ( internalflag == 0 )
    {
        if ( Debuglevel > 1 )
            printf("process_packet internalflag.%d got nonencrypted len.%d %s/%d (%s)\n",internalflag,recvlen,sender,port,recvbuf);
        len = recvlen;
        memcpy(decoded,recvbuf,recvlen);
        encrypted = 0;
        //return(0);
    }
    else return(0); // if from data field, must decrypt or it is ignored
    if ( len > 0 )
    {
        //decoded[len] = 0;
        parmslen = (int32_t)strlen((char *)decoded) + 1;
        if ( len > parmslen )
            datalen = (len - parmslen);
        else datalen = 0;
        if ( decoded[0] == '{' || decoded[0] == '[' )
        {
            parmstxt = clonestr((char *)decoded);
            argjson = cJSON_Parse(parmstxt);
            free(parmstxt), parmstxt = 0;
        }
        if ( argjson != 0 ) // if it parses, we must have been the ultimate destination
        {
            senderNXTaddr[0] = 0;
            memset(pubkey,0,sizeof(pubkey));
            parmstxt = verify_tokenized_json(pubkey,senderNXTaddr,&valid,argjson);
            if ( Debuglevel > 1 )
                printf("len.%d parmslen.%d datalen.%d (%s) valid.%d\n",len,parmslen,datalen,parmstxt,valid);
            if ( valid > 0 && parmstxt != 0 && parmstxt[0] != 0 )
            {
                tokenized_np = get_NXTacct(&createdflag,Global_mp,senderNXTaddr);
                tmpjson = cJSON_Parse(parmstxt);
                if ( tmpjson != 0 )
                {
                    char nxtip[64];
                    uint16_t nxtport,dontupdate = 0;
                    copy_cJSON(checkstr,cJSON_GetObjectItem(tmpjson,"requestType"));
                    copy_cJSON(nxtip,cJSON_GetObjectItem(tmpjson,"ipaddr"));
                    if ( is_illegal_ipaddr(nxtip) != 0 || notlocalip(nxtip) == 0 )
                        strcpy(nxtip,sender);
                    printf("nxtip.(%s) %s\n",nxtip,parmstxt);
                    nxtport = (int32_t)get_API_int(cJSON_GetObjectItem(tmpjson,"port"),0);
                    if ( strcmp(nxtip,sender) == 0 )
                        nxtport = port;
                    if ( encrypted == 0 )
                    {
                        if ( strcmp("ping",checkstr) == 0 && internalflag == 0 && dontupdate == 0 )
                            update_routing_probs(tokenized_np->H.U.NXTaddr,1,udp == 0,&tokenized_np->stats,nxtip,nxtport,pubkey);
                        if ( strcmp("ping",checkstr) == 0 || strcmp("getdb",checkstr) == 0 )
                            strcpy(checkstr,"valid");
                    }
                    else
                    {
                        if ( strcmp("pong",checkstr) == 0 && internalflag == 0 && dontupdate == 0 )
                            update_routing_probs(tokenized_np->H.U.NXTaddr,1,udp == 0,&tokenized_np->stats,nxtip,nxtport,pubkey);
                        strcpy(checkstr,"valid");
                    }
                    valueobj = cJSON_GetObjectItem(tmpjson,"data");
                    if ( is_cJSON_Number(valueobj) != 0 )
                    {
                        copy_cJSON(datalenstr,valueobj);
                        if ( datalen > 0 && datalen >= atoi(datalenstr) )
                        {
                            init_hexbytes_noT(datastr,decoded + parmslen,atoi(datalenstr));
                            cJSON_ReplaceItemInObject(tmpjson,"data",cJSON_CreateString(datastr));
                            free(parmstxt);
                            parmstxt = cJSON_Print(tmpjson);
                            stripwhite_ns(parmstxt,strlen(parmstxt));
                            free_json(argjson);
                            argjson = cJSON_Parse(parmstxt);
                            if ( Debuglevel > 0 )
                                printf("replace data.%s with (%s) (%s)\n",datalenstr,datastr,parmstxt);
                        }
                        else printf("datalen.%d mismatch.(%s) -> %d [%x]\n",datalen,datalenstr,atoi(datalenstr),*(int *)(decoded+parmslen));
                    }
                    free_json(tmpjson);
                    if ( strcmp(checkstr,"valid") == 0 )
                    {
                        char previpaddr[64];
                        struct udp_queuecmd *qp;
                        if ( prevaddr != 0 )
                            extract_nameport(previpaddr,sizeof(previpaddr),(struct sockaddr_in *)prevaddr);
                        else previpaddr[0] = 0;
                        //printf("GOT.(%s)\n",parmstxt);
                        if ( 1 )
                        {
                            qp = calloc(1,sizeof(*qp));
                            if ( previpaddr[0] != 0 )
                                qp->previpbits = calc_ipbits(previpaddr);
                            qp->argjson = argjson;
                            qp->valid = valid;
                            qp->tokenized_np = tokenized_np;
                            qp->decoded = clonestr((char *)decoded);
                            //printf("queue argjson.%p\n",argjson);
                            queue_enqueue(&udp_JSON,qp);
                            argjson = 0;
                        }
                        else
                        {
                            jsonstr = SuperNET_json_commands(Global_mp,previpaddr,argjson,tokenized_np->H.U.NXTaddr,valid,(char *)decoded);
                            if ( jsonstr != 0 )
                                free(jsonstr);
                        }
                    }
                    else printf("encrypted.%d: checkstr.(%s) for (%s)\n",encrypted,checkstr,parmstxt);
                }
            }
            else printf("valid.%d | unexpected non-tokenized message.(%s)\n",valid,decoded);
            if ( argjson != 0 )
                free_json(argjson);
            if ( parmstxt != 0 )
                free(parmstxt);
            return(tokenized_np);
        }
        else
        {
            memcpy(&destbits,decoded,sizeof(destbits));
            if ( destbits != 0 ) // route packet
            {
                static uint8_t zerokey[256>>3];
                struct nodestats *stats;
                stats = find_nodestats(destbits);
                expand_nxt64bits(destNXTaddr,destbits);
                if ( Debuglevel > 0 )
                    printf("Route to {%s} %p\n",destNXTaddr,stats);
                if ( stats != 0 && stats->ipbits != 0 && memcmp(stats->pubkey,zerokey,sizeof(stats->pubkey)) != 0 )
                {
                    outbuf = decoded;
                    len = onionize(hopNXTaddr,maxbuf,0,destNXTaddr,&outbuf,len);
                    route_packet(1,0,hopNXTaddr,outbuf,len);
                } else if ( Debuglevel > 0 ) printf("JSON didnt parse and no nodestats.%p %x %llx\n",stats,stats==0?0:stats->ipbits,stats==0?0:*(long long *)stats->pubkey);
                return(0);
            } else if ( Debuglevel > 0 ) printf("JSON didnt parse and no destination to forward to\n");
        }
    }
    else printf("process_packet got unexpected recvlen.%d %s/%d\n",recvlen,sender,port);
    return(0);
}

#endif
