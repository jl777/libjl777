//
//  NXTprotocol.h
//  Created by jl777, March/April 2014
//  MIT License
//

#ifndef gateway_NXTprotocol_h
#define gateway_NXTprotocol_h

#define SYNC_MAXUNREPORTED 32
#define SYNC_FRAGSIZE 1024

struct NXT_guid
{
    struct NXT_str H;
    char guid[128 - sizeof(struct NXT_str)];
};

struct NXT_trade { uint64_t price,quantity,askorder,bidorder,blockbits,assetid; int32_t timestamp,decimals; };

struct NXT_assettxid
{
    struct NXT_str H;
    uint64_t AMtxidbits,txidbits,assetbits,senderbits,receiverbits,quantity;
    union { uint64_t assetoshis,price; }; // price 0 -> not buy/sell but might be deposit amount
    struct NXT_guid *duplicatehash;
    char *cointxid,*redeemtxid;
    char *guid,*comment;
    int32_t completed,timestamp,vout;
};

struct NXT_assettxid_list
{
    struct NXT_assettxid **txids;
    int32_t num,max;
};

struct NXT_asset
{
    struct NXT_str H;
    uint64_t issued,mult,assetbits,issuer;
    char *description,*name;
    struct NXT_assettxid **txids;   // all transactions for this asset
    int32_t max,num,decimals,exdiv_height;
};

struct udp_info { uint64_t nxt64bits; uint32_t ipbits; uint16_t port; };
#define SET_UDPINFO(up,bits,addr) ((up)->nxt64bits = bits, (up)->ipbits = calc_ipbits(inet_ntoa((addr)->sin_addr)), (up)->port = ntohs((addr)->sin_port))

struct other_addr
{
    uint64_t modified,nxt64bits;
    char addr[128];
};

struct NXT_acct
{
    struct NXT_str H;
    struct coin_acct *coinaccts;
    struct NXT_asset **assets;
    uint64_t *quantities;
    int64_t buyqty,buysum,sellqty,sellsum,quantity;
    struct NXT_assettxid_list **txlists;    // one list for each asset in acct
    int32_t maxassets,numassets,numcoinaccts,sentintro;
    double profits;
    // fields for NXTorrent
    double hisfeedbacks[6],myfb_tohim[6];    // stats on feedbacks given
    queue_t incomingQ;
    char *signedtx;
    
    struct sockaddr Uaddr,addr;
    uint16_t udp_port,tcp_port;
    uv_stream_t *tcp,*connect;
    struct peerinfo mypeerinfo;
    char dispname[128],NXTACCTSECRET[128],udp_sender[64],tcp_sender[64];
};
struct NXT_acct **get_assetaccts(int32_t *nump,char *assetidstr,int32_t maxtimestamp);


#endif
