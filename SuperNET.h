//
//  SuperNET.h
//  libtest
//
//  Created by jl777 on 8/13/14.
//  Copyright (c) 2014 jl777. MIT License.
//

#ifndef libtest_libjl777_h
#define libtest_libjl777_h

#include <stdint.h>

#define MAX_PUBADDR_TIME (24 * 60 * 60)
#define PUBLIC_DATA 0
#define PRIVATE_DATA 1
#define TELEPOD_DATA 2
#define DEADDROP_DATA 3
#define CONTACT_DATA 4
#define NODESTATS_DATA 5
#define INSTANTDEX_DATA 6
#define PRICE_DATA 7
#define NUM_SUPERNET_DBS (PRICE_DATA+1)
#define SMALLVAL .000000000000001

#define MAX_COINTXID_LEN 66
#define MAX_COINADDR_LEN 66

struct storage_header
{
    uint32_t size,createtime;
    uint64_t keyhash;
};

struct SuperNET_storage // for public and private data
{
    struct storage_header H;
    unsigned char data[];
};

struct telepod
{
    struct storage_header H;
    int32_t vout;
    uint32_t crc,pad2,clonetime,cloneout,podstate,inhwm,pad;
    double evolve_amount;
    char clonetxid[MAX_COINTXID_LEN],cloneaddr[MAX_COINADDR_LEN];
    uint64_t senderbits,destbits,unspent,modified,satoshis; // everything after modified is used for crc
    char coinstr[8],txid[MAX_COINTXID_LEN],coinaddr[MAX_COINADDR_LEN],script[128];
    char privkey[];
};

union _bits256 { uint8_t bytes[32]; uint16_t ushorts[16]; uint32_t uints[8]; uint64_t ulongs[4]; uint64_t txid; };
typedef union _bits256 bits256;

struct nodestats
{
    struct storage_header H;
    uint8_t pubkey[256>>3];
    struct nodestats *eviction;
    uint64_t nxt64bits,coins[4];
    uint32_t ipbits,numsent,numrecv,lastcontact,numpings,numpongs;
    float recvmilli,sentmilli,pingmilli,pongmilli;
    double pingpongsum;
    uint16_t p2pport,supernet_port;
    uint8_t BTCD_p2p,gotencrypted,modified,expired;
};

struct contact_info
{
    struct storage_header H;
    bits256 pubkey,shared;
    char handle[64];
    uint64_t nxt64bits,deaddrop,mydrop;
    int32_t numsent,numrecv,lastrecv,lastsent,lastentry;
};

struct InstantDEX_quote { uint64_t nxt64bits,baseamount,relamount; uint32_t timestamp,type; };

struct orderbook_info { uint64_t baseid,relid,obookid; };

struct exchange_pair { struct storage_header H; char exchange[64],base[16],rel[16]; };

struct storage_header **copy_all_DBentries(int32_t *nump,int32_t selector);

void init_jl777(char *myip);
int SuperNET_start(char *JSON_or_fname,char *myip);
char *SuperNET_JSON(char *JSONstr);
char *SuperNET_gotpacket(char *msg,int32_t duration,char *from_ip_port);
int32_t SuperNET_broadcast(char *msg,int32_t duration);
int32_t SuperNET_narrowcast(char *destip,unsigned char *msg,int32_t len);
int32_t got_newpeer(const char *ip_port);

void *portable_thread_create(void *funcp,void *argp);

#endif

