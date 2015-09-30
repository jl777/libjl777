/******************************************************************************
 * Copyright © 2014-2015 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/


#include "crypto777.h"

void crypto777_encrypt(HUFF *dest,HUFF *src,int32_t len,struct crypto777_user *sender,uint32_t timestamp)
{
    bits256 hash; bits384 xorpad; int32_t i; uint64_t *ptr = (uint64_t *)src->buf;
    hseek(dest,len << 3,SEEK_SET), hrewind(dest), hseek(src,len << 3,SEEK_SET), hrewind(src);
    calc_sha256cat(hash.bytes,(uint8_t *)&sender->nxt64bits,sizeof(sender->nxt64bits),(uint8_t *)&timestamp,sizeof(timestamp));
    SaM_Initialize(&sender->XORpad);
    SaM_Absorb(&sender->XORpad,hash.bytes,sizeof(hash.bytes),sender->shared_SaM.bytes,sizeof(sender->shared_SaM));
    while ( len >= sizeof(xorpad) )
    {
        SaM_emit(&sender->XORpad);
        for (i=0; i<6; i++)
            xorpad.ulongs[i] = (sender->XORpad.bits.ulongs[i] ^ ptr[i]);//, printf("%llx ",(long long)xorpad.ulongs[i]);
        ptr += 6;
        hmemcpy(0,xorpad.bytes,dest,sizeof(xorpad)), len -= sizeof(xorpad);
    }
    if ( len > 0 )
    {
        SaM_emit(&sender->XORpad);
        for (i=0; i<len; i++)
            xorpad.bytes[i] = (sender->XORpad.bits.bytes[i] ^ ((uint8_t *)ptr)[i]);//, printf("%2x ",xorpad.bytes[i]);
        hmemcpy(0,xorpad.bytes,dest,len), len -= sizeof(xorpad);
    }
    //printf(" dest %llu\n",(long long)sender->nxt64bits);
}

struct crypto777_packet *crypto777_packet(struct crypto777_link *conn,uint32_t *timestamp,uint8_t *msg,int32_t len,int32_t leverage,int32_t maxmillis,uint64_t dest64bits)
{
    static uint32_t rseed = 1234;
    uint64_t hit = SAMHIT_LIMIT;
    int32_t defnumrounds=NUMROUNDS_NONCE,numrounds,codersize,netsize,allocsize = MAX_UDPSIZE;
    struct crypto777_packet *bufs;
    struct SaMhdr *hdr;
    bits384 checksig;
    if ( *timestamp == 0 )
        *timestamp = (uint32_t)time(NULL);
    netsize = (allocsize - 2*sizeof(uint32_t));
    if ( conn->my64bits == conn->send.nxt64bits && conn->recv.nxt64bits == dest64bits && memcmp(conn->send.prevmsg,msg,len) == 0 && *timestamp == conn->send.prevtimestamp )
        while ( conn->send.prevtimestamp == time(NULL) )
            msleep(250), fprintf(stderr,". "), *timestamp = (uint32_t)time(NULL);
    if ( Debuglevel > 0 )
        printf("(%llu %llu) packet.(%s) timestamp.%u [%llx] len.%d -> %llu\n",(long long)conn->send.nxt64bits,(long long)conn->recv.nxt64bits,(conn->send.nxt64bits != dest64bits) ? (char *)&msg[sizeof(struct SaMhdr)] : "ENCRYPTED",*timestamp,*(long long *)msg,len,(long long)dest64bits);
    codersize = (sizeof(*bufs->coder) + sizeof(uint16_t)*(0x100 + 2));
    bufs = calloc(1,sizeof(struct crypto777_packet) + codersize + (2 * sizeof(struct crypto777_bits)) + (sizeof(struct SaMhdr) + len) + MAX_CRYPTO777_MSG);
    //bufs->DL.ptr = bufs;
    bufs->coder = (struct ramcoder *)bufs->space, memset(bufs->coder,0,codersize);
    bufs->plaintext = (struct crypto777_bits *)&bufs->space[codersize], _init_HUFF(&bufs->plainH,netsize,bufs->plaintext->bits);
    bufs->encrypted = &bufs->plaintext[1], _init_HUFF(&bufs->encryptedH,netsize,&bufs->encrypted->bits);
    bufs->origmsg = (uint8_t *)&bufs->plaintext[2];
    bufs->recvbuf = &bufs->origmsg[sizeof(struct SaMhdr) + len]; // must be last!
    if ( dest64bits != 0 && conn->send.nxt64bits != 0 )
        calc_sha256cat(bufs->coderseed.bytes,(uint8_t *)timestamp,sizeof(*timestamp),conn->shared_curve25519.bytes,sizeof(conn->shared_curve25519));
    bufs->encrypted->plaintext_timestamp = bufs->plaintext->plaintext_timestamp = *timestamp;
    bufs->origlen = len; //printf("hdr.%ld len.%d\n",sizeof(struct SaMhdr),len);
    if ( conn->recv.nxt64bits == dest64bits && maxmillis > 0 )
    {
        hdr = (struct SaMhdr *)bufs->origmsg;
        memcpy(&bufs->origmsg[sizeof(*hdr)],msg,len);
        hdr->timestamp = *timestamp, hdr->leverage = leverage, hdr->numrounds = defnumrounds;
        bufs->threshold = SaM_threshold(leverage);
        len += (int32_t)(sizeof(*hdr) - sizeof(hdr->sig));
        //printf("+= %d | %ld = (%ld - %ld)\n",len,sizeof(*hdr) - sizeof(hdr->sig),sizeof(*hdr),sizeof(hdr->sig));
        /*if ( (hit = SaMnonce(&hdr->sig,&hdr->nonce,&bufs->origmsg[sizeof(hdr->sig)],len,bufs->threshold,rseed,maxmillis)) == 0 )
        {
            printf("crypto777_packet cant find nonce: numrounds.%d leverage.%d\n",hdr->numrounds,leverage);
            free(bufs);
            return(0);
         }*/printf("deprecated SaMnonce\n"); while ( 1 ) sleep(60);
        if ( Debuglevel > 0 )
            printf(">>>>>>>>>>>> outbound timestamp.%u nonce.%u hit.%llu (%s) | sig.%llx\n",hdr->timestamp,hdr->nonce,(long long)hit,&bufs->origmsg[sizeof(*hdr)],(long long)hdr->sig.txid);
        rseed = (uint32_t)(hdr->sig.txid ^ hit);
        len += sizeof(hdr->sig);
        if ( dest64bits != 0 )
        {
            ramcoder_encoder(bufs->coder,1,bufs->origmsg,len,&bufs->plainH,&bufs->coderseed);
            if ( (bufs->newlen= hconv_bitlen(bufs->plainH.bitoffset)) >= bufs->plainH.allocsize )
            {
                printf("crypto777_packet overflow: newlen.%d doesnt fit into %d\n",bufs->newlen,bufs->plainH.allocsize);
                free(bufs);
                return(0);
            }
            else _randombytes(&bufs->plainH.buf[bufs->newlen],bufs->plainH.allocsize - bufs->newlen,(uint32_t)hdr->sig.txid);
            if ( dest64bits != 0 )
                crypto777_encrypt(&bufs->encryptedH,&bufs->plainH,netsize,&conn->send,*timestamp);
        }
        conn->send.prevlen = bufs->origlen, memcpy(conn->send.prevmsg,msg,bufs->origlen), conn->send.prevtimestamp = *timestamp;
    }
    else if ( conn->send.nxt64bits == dest64bits && milliseconds() > conn->recv.blacklist )
    {
        if ( dest64bits != 0 )
        {
            if ( dest64bits != 0 )
                memcpy(bufs->encryptedH.buf,msg,len), crypto777_encrypt(&bufs->plainH,&bufs->encryptedH,len,&conn->recv,*timestamp);
            else memcpy(bufs->plainH.buf,msg,len);
            hseek(&bufs->plainH,len << 3,SEEK_SET), hrewind(&bufs->plainH);
            bufs->recvlen = ramcoder_decoder(bufs->coder,1,bufs->recvbuf,len,&bufs->plainH,&bufs->coderseed);
        } else memcpy(bufs->recvbuf,msg,len), bufs->recvlen = len;
        hdr = (struct SaMhdr *)bufs->recvbuf;
        leverage = hdr->leverage, numrounds = hdr->numrounds;
        if ( Debuglevel > 0 )
            printf("<<<<<<<< nonce.%u len.%d leverage.%d numrounds.%d\n",hdr->nonce,len,leverage,(int)numrounds);
        if ( leverage < CRYPTO777_MAXLEVERAGE && numrounds == defnumrounds )
        {
            bufs->threshold = SaM_threshold(leverage);
            len = bufs->recvlen - sizeof(hdr->sig);
            hit = SaM(&checksig,&bufs->recvbuf[sizeof(hdr->sig)],len,0,0);
            len -= (sizeof(*hdr) - sizeof(hdr->sig));
            if ( hit >= bufs->threshold || memcmp(checksig.bytes,hdr->sig.bytes,sizeof(checksig)) != 0 )
            {
                conn->recv.blacklist = (milliseconds() + 10000);
                printf("crypto777_packet invalid AUTH: hit %llu >= threshold.%llu || sig mismatch.%d (%llx v %llx) from %llu leverage.%d numrounds.%d\n",(long long)hit,(long long)bufs->threshold,memcmp(checksig.bytes,hdr->sig.bytes,sizeof(checksig)),(long long)checksig.txid,(long long)hdr->sig.txid,(long long)conn->recv.nxt64bits,hdr->leverage,hdr->numrounds);
                free(bufs);
                return(0);
            } else if ( Debuglevel > 0 ) printf("AUTHENTICATED packet from %llu len.%d leverage.%d numrounds.%d | hit %.2f%%\n",(long long)conn->recv.nxt64bits,len,leverage,numrounds,100.*(double)hit/bufs->threshold);
        } else printf("AUTH failure from %llu: hit.%llu >= threshold.%llu leverage.%d numrounds.%d\n",(long long)conn->recv.nxt64bits,(long long)hit,(long long)bufs->threshold,leverage,numrounds);
    } else printf("crypto777_packet warning invalid dest64bits %llu\n",(long long)dest64bits);
    return(bufs);
}

uint32_t crypto777_broadcast(struct crypto777_node *nn,uint8_t *msg,int32_t len,int32_t leverage,uint64_t dest64bits)
{
    uint32_t timestamp,crc = 0;
    struct SaMhdr *hdr;
    struct crypto777_packet *send;
    timestamp = 0;
    for (; len<SAM_HASH_SIZE; len++)
        msg[len] = 0;
    if ( (send = crypto777_packet(&nn->broadcast,&timestamp,(uint8_t *)msg,len,leverage,1000,dest64bits)) != 0 )
    {
        hdr = (struct SaMhdr *)send->origmsg;
        crc = _crc32(0,send->origmsg,send->origlen+sizeof(struct SaMhdr));
        //printf("crypto777_broadcast crc.%u send time.%u nonce.%u leverage.%d numrounds.%d\n",crc,hdr->timestamp,hdr->nonce,hdr->leverage,hdr->numrounds);
        //usleep((rand() % 1000000) + 100000);
        crypto777_send(0,send->origmsg,send->origlen+sizeof(struct SaMhdr),0,&nn->transport);
        free(send);
    } else crc = 0;//, printf("error broadcasting packet from %s\n",nn->email);
    return(crc);
}

void crypto777_processpacket(uint8_t *buf,int32_t len,struct crypto777_node *nn,char *type,char *senderip,uint16_t senderport)
{
    uint32_t timestamp = 0;
    struct crypto777_node *peer;
    struct crypto777_packet *recv;
    if ( nn != 0 )
    {
        if ( (peer= crypto777_findpeer(nn,-1,type,senderip,senderport)) == 0 )
        {
            numpackets++, Totalrecv++;
            nn->recvcount++;
            //SETBIT(Recvmasks[nn->nodeid],peer->nodeid);
            if ( (recv= crypto777_packet(&nn->broadcast,&timestamp,buf,len,0,0,0)) != 0 )
            {
                recv->nodeid = peer->nodeid;
                printf("(%s): broadcast RECEIVED %d | (%s) recv.%d total.%ld\n",nn->email,len,&buf[sizeof(struct SaMhdr)],recv->recvlen,Totalrecv);
                queue_enqueue("recvQ",&nn->recvQ,&recv->DL);
            }
            else printf("ERROR: (%s): recv.%d total.%ld\n",nn->email,len,Totalrecv);
        }
    } else printf("crypto777_processpacket got null nn: %s:%d len.%d\n",senderip,senderport,len);
}
