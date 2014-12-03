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
union _asset_price { uint64_t assetoshis,price; };

struct NXT_assettxid
{
    struct NXT_str H;
    uint64_t AMtxidbits,txidbits,assetbits,senderbits,receiverbits,quantity,redeemtxid;
    union _asset_price U; // price 0 -> not buy/sell but might be deposit amount
    //struct NXT_guid *duplicatehash;
    uint32_t coinblocknum,cointxind,coinv;
    char *cointxid;
    char *comment;//*guid,
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

struct acct_coin { char name[16],*acctcoinaddr,*pubkey; };

struct NXT_acct
{
    struct NXT_str H;
    struct NXT_asset **assets;
    uint64_t *quantities,bestbits,quantity;
    struct NXT_assettxid_list **txlists;    // one list for each asset in acct
    int32_t maxassets,numassets,bestdist,numcoins;//numcoinaccts
    int64_t buyqty,buysum,sellqty,sellsum;
    double profits;
    uint32_t timestamps[64];
    struct acct_coin *coins[64];
    //struct coin_acct *coinaccts;
    // fields for NXTorrent
    //double hisfeedbacks[6],myfb_tohim[6];    // stats on feedbacks given
    //queue_t incomingQ;
    //uint16_t udp_port;
    //uv_stream_t *tcp,*connect;
    //char dispname[128];//,udp_sender[64];//,tcp_sender[64];
    struct nodestats stats;
    char *signedtx,NXTACCTSECRET[128];
};
struct NXT_acct **get_assetaccts(int32_t *nump,char *assetidstr,int32_t maxtimestamp);


#endif
