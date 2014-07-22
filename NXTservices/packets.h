//
//  packets.h
//  xcode
//
//  Created by jl777 on 7/19/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_packets_h
#define xcode_packets_h


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

int32_t crcize(unsigned char *final,unsigned char *encoded,int32_t len)
{
    uint32_t crc = _crc32(0,encoded,len);
    memcpy(final,&crc,sizeof(crc));
    memcpy(final + sizeof(crc),encoded,len);
    return(len + sizeof(crc));
}

int32_t onionize(unsigned char *encoded,char *destNXTaddr,unsigned char *payload,int32_t len)
{
    unsigned char zerokey[crypto_box_PUBLICKEYBYTES];
    uint64_t nxt64bits;
    int32_t createdflag;
    uint16_t payload_len;
    struct NXT_acct *np;
    nxt64bits = calc_nxt64bits(destNXTaddr);
    np = get_NXTacct(&createdflag,Global_mp,destNXTaddr);
    if ( memcmp(np->pubkey,zerokey,sizeof(zerokey)) == 0 )
        return(0);
    memcpy(encoded,&nxt64bits,sizeof(nxt64bits));
    encoded += sizeof(nxt64bits);
    memcpy(encoded,Global_mp->session_pubkey,sizeof(Global_mp->session_pubkey));
    encoded += sizeof(Global_mp->session_pubkey);
    payload_len = len;
    memcpy(encoded,&payload_len,sizeof(payload_len));
    encoded += sizeof(payload_len);
    
    len = _encode_str(encoded,(char *)payload,len,np->pubkey,Global_mp->session_privkey);
    return(len + sizeof(payload_len) + sizeof(Global_mp->session_pubkey) + sizeof(nxt64bits));
}

int32_t deonionize(unsigned char *decoded,unsigned char *encoded,int32_t len,uint64_t mynxtbits)
{
    int32_t err;
    uint16_t payload_len;
    unsigned char *pubkey;
    if ( memcmp(&mynxtbits,encoded,sizeof(mynxtbits)) == 0 )
    {
        encoded += sizeof(mynxtbits);
        pubkey = encoded;
        encoded += crypto_box_PUBLICKEYBYTES;
        memcpy(&payload_len,encoded,sizeof(payload_len));
        encoded += sizeof(payload_len);
        if ( (payload_len + sizeof(payload_len) + sizeof(Global_mp->session_pubkey) + sizeof(mynxtbits)) == len )
        {
            len = payload_len;
            err = _decode_cipher((char *)decoded,encoded,&len,pubkey,Global_mp->session_privkey);
            printf("payload_len.%d err.%d new len.%d\n",payload_len,err,len);
            if ( err == 0 )
                return(len);
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
    tx += sizeof(crc);
    crc = _crc32(0,tx,len);
    memcpy(&packet_crc,tx,sizeof(packet_crc));
    return(packet_crc == crc);
}

int gen_tokenjson(CURL *curl_handle,char *jsonstr,char *NXTaddr,long nonce,char *NXTACCTSECRET)
{
    char argstr[1024],pubkey[1024],token[1024];
    init_hexbytes(pubkey,Global_mp->session_pubkey,sizeof(Global_mp->session_pubkey));
    sprintf(argstr,"{\"NXT\":\"%s\",\"pubkey\":\"%s\",\"time\":%ld}",NXTaddr,pubkey,nonce);
    printf("got argstr.(%s)\n",argstr);
    issue_generateToken(curl_handle,token,argstr,NXTACCTSECRET);
    token[NXT_TOKEN_LEN] = 0;
    sprintf(jsonstr,"[%s,{\"token\":\"%s\"}]",argstr,token);
    printf("tokenized.(%s)\n",jsonstr);
    return((int)strlen(jsonstr));
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
                    if ( strcmp(sender,NXTaddr) == 0 )
                    {
                        printf("signed by valid NXT.%s valid.%d diff.%lld\n",sender,valid,(long long)diff);
                        retcode = valid;
                    }
                }
                if ( retcode < 0 )
                    printf("err: signed by invalid NXT.%s valid.%d or timediff too big diff.%lld\n",sender,valid,(long long)diff);
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
    if ( (retcode= validate_token(0,pubkey,NXTaddr,bufbase,15)) > 0 )
    {
        np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
        if ( np != 0 )
        {
            n = decode_hex(np->pubkey,(int32_t)sizeof(np->pubkey),pubkey);
            if ( n == crypto_box_PUBLICKEYBYTES )
            {
                printf("created.%d NXT.%s pubkey.%s (len.%d)\n",createdflag,NXTaddr,pubkey,n);
                if ( connect != 0 && sendresponse != 0 )
                {
                    //printf("call set_intro handle.%p %s.(%s)\n",connect,Server_NXTaddr,Server_secret);
                    //printf("got (%s)\n",retbuf);
                    init_hexbytes(pubkey,Global_mp->session_pubkey,sizeof(Global_mp->session_pubkey));
                    sprintf(argstr,"{\"NXT\":\"%s\",\"pubkey\":\"%s\",\"time\":%ld}",Server_NXTaddr,pubkey,time(NULL));
                    //printf("got argstr.(%s)\n",argstr);
                    issue_generateToken(0,token,argstr,Server_secret);
                    token[NXT_TOKEN_LEN] = 0;
                    sprintf(retbuf,"[%s,{\"token\":\"%s\"}]",argstr,token);
                    if ( retbuf[0] == 0 )
                        printf("error generating intro??\n");
                    else portable_tcpwrite(connect,clonestr(retbuf),(int32_t)strlen(retbuf)+1,ALLOCWR_FREE);
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
        np->Uaddr = *addr;
        np->udp = udp;
        np->udp_port = port;
        safecopy(np->udp_sender,sender,sizeof(np->udp_sender));
    }
    else
    {
        np->addr = *addr;
        if ( I_am_server != 0 )
            np->connect = tcp;
        else np->tcp = tcp;
        np->tcp_port = port;
        safecopy(np->tcp_sender,sender,sizeof(np->tcp_sender));
    }
}

struct NXT_acct *process_packet(char *retjsonstr,struct NXT_acct *np,int32_t I_am_server,unsigned char *recvbuf,int32_t recvlen,uv_stream_t *tcp,uv_stream_t *udp,const struct sockaddr *addr,char *sender,uint16_t port)
{
    uint64_t destbits = 0;
    struct NXT_acct *tokenized_np;
    int32_t valid,len=0,createdflag;
    cJSON *argjson,*msgjson;
    unsigned char pubkey[crypto_box_PUBLICKEYBYTES],decoded[4096],crcbuf[4096];
    char senderNXTaddr[64],destNXTaddr[64],message[4096],*parmstxt,*jsonstr;
    sprintf(retjsonstr,"{\"error\":\"unknown error processing %d bytes from %s/%d\"}",recvlen,sender,port);
    if ( is_encrypted_packet(recvbuf,recvlen) != 0 )
    {
        recvbuf += sizeof(uint32_t);
        recvlen -= sizeof(uint32_t);
        if ( (len= deonionize(decoded,recvbuf,recvlen,0)) > 0 )
        {
            memcpy(&destbits,decoded,sizeof(destbits));
            printf("decrypted len.%d dest.(%llu)\n",len,(long long)destbits);
        }
        else return(0);
    }
    else if ( np == 0 && (tcp != 0 || udp != 0) )
    {
        if ( (np= process_intro(tcp,(char *)recvbuf,(I_am_server!=0 && tcp!=0) ? 1 : 0)) != 0 )
        {
            printf("process_intro got NXT.(%s) np.%p <- I_am_server.%d UDP %p TCP.%p %s/%d\n",np->H.NXTaddr,np,I_am_server,udp,tcp,sender,port);
            update_np_connectioninfo(np,I_am_server,tcp,udp,addr,sender,port);
            retjsonstr[0] = 0;
            return(np);
        } else printf("on_udprecv: process_intro returns null?\n");
    }
    else
    {
        printf("process_packet got unexpected nonencrypted len.%d NXT.(%s) np.%p <- I_am_server.%d UDP %p TCP.%p %s/%d\n",recvlen,np!=0?np->H.NXTaddr:"noaddr",np,I_am_server,udp,tcp,sender,port);
        return(0);
    }
    memset(message,0,recvlen);
    //if ( recvlen > (sizeof(pubkey) + crypto_box_NONCEBYTES+1) )
    if ( len > 0 )
    {
        /*memcpy(pubkey,recvbuf,sizeof(pubkey));
         len = (int32_t)(recvlen - sizeof(pubkey));
         err = _decode_cipher(message,(void *)((long)recvbuf + sizeof(pubkey)),&len,pubkey,Global_mp->session_privkey);
         if ( err == 0 )
         {
         printf("DECRYPTED.(%s)\n",message);
         } else printf("error.%d decrypting pubkey.%llx\n",err,*(long long *)pubkey);
         */
        parmstxt = clonestr(message);//rcvbuf->base;
        parmstxt[recvlen] = 0;
        argjson = cJSON_Parse(parmstxt);
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
                    update_np_connectioninfo(np,I_am_server,tcp,udp,addr,sender,port);
                    if ( memcmp(np->pubkey,pubkey,sizeof(pubkey)) != 0 )
                    {
                        printf("update pubkey for NXT.%s to %llx\n",senderNXTaddr,*(long long *)pubkey);
                        memcpy(np->pubkey,pubkey,sizeof(np->pubkey));
                    }
                    if ( I_am_server == 0 )
                    {
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
                    else
                    {
                        char *pNXT_jsonhandler(cJSON **argjsonp,char *argstr,char *verifiedsender);
                        jsonstr = pNXT_jsonhandler(&argjson,parmstxt,np->H.NXTaddr);
                        if ( jsonstr == 0 )
                            strcpy(retjsonstr,"{\"result\":\"pNXT_jsonhandler returns null\"}");
                        else
                        {
                            strcpy(retjsonstr,jsonstr);
                            free(jsonstr);
                        }
                        printf("tcpwrite.(%s) to NXT.%s\n",jsonstr,np!=0?np->H.NXTaddr:"unknown");
                        //portable_tcpwrite(connect,jsonstr,(int32_t)strlen(jsonstr)+1,ALLOCWR_FREE);
                        //free(jsonstr); completion frees, dont do it here!
                        
                        //queue_enqueue(&RPC_6777_response,str);
                    }
                }
            }
            free_json(argjson);
            printf("parmstxt.%p valid.%d sender.(%s) msg.(%s)\n",parmstxt,valid,senderNXTaddr,message);
        }
        else if ( tcp != 0 || udp != 0 ) // test to make sure not p2p broadcast
        {
            // route packet
            memcpy(&destbits,parmstxt,sizeof(destbits));
            if ( destbits != 0 )
            {
                expand_nxt64bits(destNXTaddr,destbits);
                np = get_NXTacct(&createdflag,Global_mp,destNXTaddr);
                if ( np->udp != 0 )
                {
                    int32_t portable_udpwrite(const struct sockaddr *addr,uv_udp_t *handle,void *buf,long len,int32_t allocflag);
                    len = crcize(crcbuf,(unsigned char *)parmstxt,recvlen);
                    portable_udpwrite(&np->Uaddr,(uv_udp_t *)np->udp,crcbuf,len,ALLOCWR_ALLOCFREE);
                }
            }
            else sprintf(retjsonstr,"{\"error\":\"unknown dest.%llu %d bytes from %s/%d\"}",(long long)destbits,recvlen,sender,port);
        }
        if ( parmstxt != 0 )
            free(parmstxt);
    }
    else printf("process_packet got unexpected recvlen.%d NXT.(%s) np.%p <- I_am_server.%d UDP %p TCP.%p %s/%d\n",recvlen,np!=0?np->H.NXTaddr:"noaddr",np,I_am_server,udp,tcp,sender,port);
    return(0);
}

#endif
