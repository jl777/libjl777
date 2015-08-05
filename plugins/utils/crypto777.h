//
//  crypto777.h
//
//  Created by James on 3/17/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifndef crpto777_h
#define crpto777_h

#define NETWORKSIZE 64
#define MAXPEERS 3
#define MAXHOPS 6

#define MAX_CRYPTO777_BLOCKS 10000

#define NUMROUNDS_ENCRYPT 1
#define NUMROUNDS_NONCE 1
#define NUMROUNDS_CHAIN 1
#define PACKET_LEVERAGE 1
#define CRYPTO777_MAXLEVERAGE 16

#define MAX_UDPSIZE 1400
#define MAX_CRYPTO777_MSG 8192

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <curl/easy.h>
extern int32_t numpackets,numxmit,Generator,Receiver;
extern long Totalrecv,Totalxmit;
extern uint64_t MIN_NQTFEE;

#define DEFINES_ONLY

#include "../common/system777.c"
#include "bits777.c"
#include "utils777.c"
#include "NXT777.c"
#include "SaM.c"
#include "huffstream.c"
#include "ramcoder.c"
#include "cJSON.h"

struct crypto777_bits { uint32_t plaintext_timestamp,tbd; uint8_t bits[MAX_UDPSIZE - sizeof(uint32_t)*2]; };
struct crypto777_peer { char ip_port[64],type[16]; int32_t pairsock,subsock; uint16_t port; };

struct crypto777_packet
{
    struct queueitem DL;
    HUFF plainH,encryptedH;
    struct ramcoder *coder;
    bits256 coderseed;
    uint64_t threshold;
    int32_t origlen,recvlen,newlen;
    uint32_t nodeid;
    uint8_t *origmsg,*recvbuf;
    struct crypto777_bits *plaintext,*encrypted;
    uint8_t space[];
};

struct crypto777_transport
{
    char ipaddr[64],ip_port[64],type[64];
    struct crypto777_peer peers[MAXPEERS];
    int32_t insocks[MAXPEERS],bus,pub,numpairs,numpeers,numsent,numrecv,uniqs,duplicates;
    uint16_t port;
};

struct crypto777_user
{
    uint8_t prevmsg[MAX_CRYPTO777_MSG];
    struct SaM_info XORpad;
    bits384 shared_SaM;
    uint64_t nxt64bits;
    double blacklist;
    uint32_t prevlen,prevtimestamp;//,numchallenges,range,supernonce,*challenges;
};
struct crypto777_link { bits256 shared_curve25519; uint64_t my64bits; struct crypto777_user send,recv; };

struct crypto777_node
{
    struct bloombits blooms[MAXHOPS+1];
    struct crypto777_link broadcast;
    struct crypto777_node *peers[MAXPEERS];
    char ip_port[64],email[60],pad[4];
    bits384 mypassword,mypubkey,nxtpass,emailhash;
    bits256 nxtpubkey,nxtsecret;
    uint64_t nxt64bits,hit,blocktxid,blocktxid2;
    uint32_t blocknum,lastblocknum,lastblocknum2,first_timestamp,lastblocktimestamp,peerfifo[MAXPEERS+1][8];
    int32_t recvcount,pubkeychainid,revealed,nodeid,numpeers;
    queue_t recvQ;
    struct crypto777_transport transport;
    //struct consensus_model model;
};

struct crypto777_generator { bits384 hash; uint64_t metric,hit; char email[60]; uint32_t timestamp; };
struct crypto777_block
{
    bits384 hash,sig;
    uint32_t blocknum,blocksize,rawblocklen,timestamp;
    struct crypto777_generator gen;
    uint8_t rawblock[];
};

struct crypto777_entry { uint64_t amount; uint32_t itemid,accountid; };
struct crypto777_txswap { bits384 sigA,sigB; struct crypto777_entry entryA,entryB; };
struct crypto777_txsend { bits384 sig; struct crypto777_entry src; uint32_t dest; };
struct crypto777_tx { bits384 sigG; uint32_t type,size; uint8_t rawbytes[]; };
struct crypto777_ledger { bits384 sigG,item; char name[12]; uint32_t num,firstblocknum,blocknum,timestamp,itemid; uint64_t vals[NETWORKSIZE]; };
struct consensus_model
{
    uint64_t stakeid,POW_DIFF,peermetrics[MAX_CRYPTO777_BLOCKS][MAXPEERS];
    uint32_t genesis_timestamp,packet_leverage,blockduration,numledgers,peerblocknum[MAXPEERS];
    struct crypto777_block *blocks[MAX_CRYPTO777_BLOCKS];
    struct crypto777_ledger stakeledger,accts,ledgers[];
};

extern struct crypto777_node *Network[NETWORKSIZE];
#undef DEFINES_ONLY


#define MODEL_ROUNDROBIN 0xffffffff // must be 0xffffffff

struct consensus_model *create_genesis(uint64_t PoW_diff,int32_t blockduration,int32_t leverage,uint64_t *stakes,int32_t num);
void *crypto777_loop(void *args);

int32_t crypto777_link(uint64_t mynxt64bits,struct crypto777_link *conn,bits384 nxtpass,uint64_t other64bits,int32_t numchallenges,uint32_t range);

void crypto777_encrypt(HUFF *dest,HUFF *src,int32_t len,struct crypto777_user *sender,uint32_t timestamp);
struct crypto777_packet *crypto777_packet(struct crypto777_link *conn,uint32_t *timestamp,uint8_t *msg,int32_t len,int32_t leverage,int32_t maxmillis,uint64_t dest64bits);
uint32_t crypto777_broadcast(struct crypto777_node *nn,uint8_t *msg,int32_t len,int32_t leverage,uint64_t dest64bits);
int32_t crypto777_send(struct crypto777_transport *dest,uint8_t *msg,long len,uint32_t flags,struct crypto777_transport *src);
int32_t crypto777_peer(struct crypto777_node *nn,struct crypto777_node *peer);
struct crypto777_node *crypto777_findpeer(struct crypto777_node *nn,int32_t dir,char *type,char *ipaddr,uint16_t port);
struct crypto777_transport *crypto777_transport(struct crypto777_node *nn,char *ip_port,char *type);

struct crypto777_node *crypto777_findpeer(struct crypto777_node *nn,int32_t dir,char *type,char *ipaddr,uint16_t port);
void select_peers(struct crypto777_node *nn);
int32_t update_peers(struct crypto777_node *nn,struct crypto777_block *block,uint64_t peermetrics[][MAXPEERS],int32_t numpeers,int32_t packet_leverage);

void crypto777_processpacket(uint8_t *buf,int32_t len,struct crypto777_node *nn,char *type,char *senderip,uint16_t senderport);

struct crypto777_node *crypto777_node(char *busurl,char *myemail,bits384 mypassword,char *type);
struct crypto777_node *crypto777_findemail(char *email);
int32_t crypto777_addnode(struct crypto777_node *nn);

#endif
