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
#include "includes/uv.h"

#define DEFINES_ONLY
#include "plugins/utils/system777.c"
#include "plugins/utils/utils777.c"
#undef DEFINES_ONLY

#define MAX_PUBADDR_TIME (24 * 60 * 60)
#define PUBLIC_DATA 0
#define PRIVATE_DATA 1
#define TELEPOD_DATA 2
#define DEADDROP_DATA 3
#define CONTACT_DATA 4
#define NODESTATS_DATA 5
#define INSTANTDEX_DATA 6
#define MULTISIG_DATA 7
#define ADDRESS_DATA 8
#define PRICE_DATA 9
#define NUM_SUPERNET_DBS (PRICE_DATA+1)
#define _SUPERNET_PORT 7777

#define MAX_COINTXID_LEN 128
#define MAX_COINADDR_LEN 128
#define MAX_NXT_STRLEN 24
#define MAX_NXTTXID_LEN MAX_NXT_STRLEN
#define MAX_NXTADDR_LEN MAX_NXT_STRLEN
#define MAX_TRANSFER_SIZE (65536 * 16)
#define TRANSFER_BLOCKSIZE 512
#define MAX_TRANSFER_BLOCKS (MAX_TRANSFER_SIZE / TRANSFER_BLOCKSIZE)
struct address_entry { uint64_t blocknum:32,txind:15,vinflag:1,v:14,spent:1,isinternal:1; };

struct transfer_args
{
    uint64_t modified;
    uint32_t timestamps[MAX_TRANSFER_BLOCKS],crcs[MAX_TRANSFER_BLOCKS],gotcrcs[MAX_TRANSFER_BLOCKS],slots[MAX_TRANSFER_BLOCKS];
    char previpaddr[64],sender[64],dest[64],name[512],hashstr[65],handler[64],pstr[MAX_TRANSFER_BLOCKS];
    uint8_t data[MAX_TRANSFER_SIZE],snapshot[MAX_TRANSFER_SIZE];
    uint32_t totalcrc,currentcrc,snapshotcrc,handlercrc,syncmem,totallen,blocksize,numfrags,completed,timeout,handlertime;
};

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

struct pserver_info
{
    char ipaddr[64];
    uint64_t modified,nxt64bits;
    float recvmilli,sentmilli;
    void *udps[2];
    uint32_t decrypterrs,lastcontact,numsent,numrecv;
    uint16_t p2pport,firstport,lastport,supernet_port,supernet_altport;
};

/*struct nodestats
{
    struct storage_header H;
    uint8_t pubkey[256>>3];
    struct nodestats *eviction;
    uint64_t nxt64bits,coins[4];
    double pingpongsum;
    float pingmilli,pongmilli;
    uint32_t ipbits,lastcontact,numpings,numpongs;
    uint8_t BTCD_p2p,gotencrypted,modified,expired,isMM;
};*/

struct contact_info
{
    struct storage_header H;
    bits256 pubkey,shared;
    char handle[64];
    uint64_t nxt64bits,deaddrop,mydrop;
    int32_t numsent,numrecv,lastrecv,lastsent,lastentry;
    char jsonstr[32768];
};

#define NUM_BARPRICES 16
struct displaybars
{
    uint64_t baseid,relid;
    char base[16],rel[16],exchange[16];
    int32_t resolution,width,start,end;
    float bars[4096][NUM_BARPRICES];
};

//struct combined_amounts { uint64_t frombase,fromrel,tobase,torel; };

//struct orderbook_info { uint64_t baseid,relid,obookid; };

//struct exchange_pair { struct storage_header H; char exchange[64],base[16],rel[16]; };

/*struct pubkey_info { uint64_t nxt64bits; uint32_t ipbits; char pubkey[256],coinaddr[128]; };

union _NXT_str_buf { char txid[MAX_NXTTXID_LEN]; char NXTaddr[MAX_NXTADDR_LEN];  char assetid[MAX_NXT_STRLEN]; };

struct NXT_str
{
    uint64_t modified,nxt64bits;
    union _NXT_str_buf U;
};

union _asset_price { uint64_t assetoshis,price; };

struct NXT_assettxid
{
    struct NXT_str H;
    uint64_t AMtxidbits,redeemtxid,assetbits,senderbits,receiverbits,quantity,convassetid;
    double minconvrate;
    union _asset_price U; // price 0 -> not buy/sell but might be deposit amount
    uint32_t coinblocknum,cointxind,coinv,height,redeemstarted;
    //void *redeemdata[3];
    char *cointxid;
    char *comment,*convwithdrawaddr,convname[16],teleport[64];
    float estNXT;
    int32_t completed,timestamp,numconfs,convexpiration,buyNXT,sentNXT;
};

struct NXT_AMhdr
{
    uint32_t sig;
    int32_t size;
    uint64_t nxt64bits;
};

struct compressed_json { uint32_t complen,sublen,origlen,jsonlen; unsigned char encoded[128]; };
union _json_AM_data { unsigned char binarydata[sizeof(struct compressed_json)]; char jsonstr[sizeof(struct compressed_json)]; struct compressed_json jsn; };

struct json_AM
{
    struct NXT_AMhdr H;
	uint32_t funcid,gatewayid,timestamp,jsonflag;
    union _json_AM_data U;
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
    int32_t max,num,decimals;
    uint16_t type,subtype;
};*/

int32_t portable_truncate(char *fname,long filesize);
void *map_file(char *fname,uint64_t *filesizep,int32_t enablewrite);
int32_t os_supports_mappedfiles();
char *os_compatible_path(char *str);
char *OS_rmstr();
int32_t OS_launch_process(char *args[]);
int32_t OS_getppid();
int32_t OS_waitpid(int32_t childpid,int32_t *statusp,int32_t flags);
int32_t is_bundled_plugin(char *plugin);
//typedef int32_t (*ptm)(int32_t,char *args[]);

#define INCLUDE_DEFINES
//#include "ramchain.h"
#undef INCLUDE_DEFINES
#include "includes/uthash.h"
#define HUFF_NUMFREQS 1
#define SETBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] |= (1 << ((bitoffset) & 7)))
#define GETBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] & (1 << ((bitoffset) & 7)))
#define CLEARBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] &= ~(1 << ((bitoffset) & 7)))

struct huffstream { uint8_t *ptr,*buf; uint32_t bitoffset,maski,endpos; uint32_t allocsize:31,allocated:1; };
typedef struct huffstream HUFF;

struct huffbits { uint64_t numbits:5,rawind:23,bits:20,freq:16; };
struct huffitem { struct huffbits code; uint32_t freq[HUFF_NUMFREQS]; };

struct huffcode
{
    double totalbits,totalbytes,freqsum;
    int32_t numitems,maxbits,numnodes,depth,allocsize;
    int32_t tree[];
};

struct huffpair { struct huffbits *items; struct huffcode *code; int32_t maxind,nonz,count,wt; uint64_t minval,maxval; char name[16]; };
struct rawtx_huffs { struct huffpair numvins,numvouts,txid; };
struct rawvin_huffs { struct huffpair txid,vout; };
struct rawvout_huffs { struct huffpair addr,script,value; };
struct rawblock_huffs
{
    struct huffpair numall,numtx,numrawvins,numrawvouts;
    // (0) and (1) are the first and second places, (n) is the last one, (i) is everything between
    struct rawtx_huffs txall,tx0,tx1,txi;
    struct rawvin_huffs vinall,vin0,vin1,vini;
    struct rawvout_huffs voutall,vout0,vout1,vout2,vouti,voutn;
};

struct rawblock_preds
{
    struct huffpair *numtx,*numrawvins,*numrawvouts;
    // (0) and (1) are the first and second places, (n) is the last one, (i) is everything between
    struct huffpair *tx0_numvins,*tx1_numvins,*txi_numvins;
    struct huffpair *tx0_numvouts,*tx1_numvouts,*txi_numvouts;
    struct huffpair *tx0_txid,*tx1_txid,*txi_txid;
    
    struct huffpair *vin0_txid,*vin1_txid,*vini_txid;
    struct huffpair *vin0_vout,*vin1_vout,*vini_vout;
    
    struct huffpair *vout0_addr,*vout1_addr,*vout2_addr,*vouti_addr,*voutn_addr;
    struct huffpair *vout0_script,*vout1_script,*vout2_script,*vouti_script,*voutn_script;
    struct huffpair *vout0_value,*vout1_value,*vout2_value,*vouti_value,*voutn_value;
};

/*struct mappedptr
{
	char fname[512];
	void *fileptr,*pending;
	uint64_t allocsize,changedsize;
	int32_t rwflag,actually_allocated;
};

struct ramsnapshot { bits256 hash; long permoffset,addroffset,txidoffset,scriptoffset; uint32_t addrind,txidind,scriptind; };
struct rampayload { struct address_entry B,spentB; uint64_t value; uint32_t otherind:31,extra:31,pendingdeposit:1,pendingsend:1; };
struct ramchain_hashptr { int64_t unspent; UT_hash_handle hh; struct rampayload *payloads; uint32_t rawind,permind,numpayloads:29,maxpayloads:29,mine:1,multisig:1,verified:1,nonstandard:1,tbd:2; int32_t numunspent; };
struct ramchain_hashtable
{
    char coinstr[16];
    struct ramchain_hashptr *table;
    struct mappedptr M;
    FILE *newfp,*permfp;
    struct ramchain_hashptr **ptrs;
    long endpermpos;
    uint32_t ind,numalloc;
    uint8_t type;
};

#define MAX_BLOCKTX 0xffff
struct rawvin { char txidstr[128]; uint16_t vout; };
struct rawvout { char coinaddr[64],script[256]; uint64_t value; };
struct rawtx { uint16_t firstvin,numvins,firstvout,numvouts; char txidstr[128]; };

#define MAX_COINTX_INPUTS 16
#define MAX_COINTX_OUTPUTS 8
struct cointx_input { struct rawvin tx; char coinaddr[64],sigs[1024]; uint64_t value; uint32_t sequence; char used; };
struct cointx_info
{
    uint32_t crc; // MUST be first
    char coinstr[16];
    uint64_t inputsum,amount,change,redeemtxid;
    uint32_t allocsize,batchsize,batchcrc,gatewayid,isallocated;
    // bitcoin tx order
    uint32_t version,timestamp,numinputs;
    uint32_t numoutputs;
    struct cointx_input inputs[MAX_COINTX_INPUTS];
    struct rawvout outputs[MAX_COINTX_OUTPUTS];
    uint32_t nlocktime;
    // end bitcoin txcalc_nxt64bits
    char signedtx[];
};

struct rawblock
{
    uint32_t blocknum,allocsize;
    uint16_t format,numtx,numrawvins,numrawvouts;
    uint64_t minted;
    struct rawtx txspace[MAX_BLOCKTX];
    struct rawvin vinspace[MAX_BLOCKTX];
    struct rawvout voutspace[MAX_BLOCKTX];
};

struct mappedblocks
{
    struct ramchain_info *ram;
    struct mappedblocks *prevblocks;
    struct rawblock *R,*R2,*R3;
    HUFF **hps,*tmphp;
    struct mappedptr *M;
    double sum;
    uint32_t blocknum,count,firstblock,numblocks,processed,format,shift,contiguous;
};

struct MGWstate
{
    char name[64];
    uint64_t nxt64bits;
    int64_t MGWbalance,supply;
    uint64_t totalspends,numspends,totaloutputs,numoutputs;
    uint64_t boughtNXT,circulation,sentNXT,MGWpendingredeems,orphans,MGWunspent,MGWpendingdeposits,NXT_ECblock;
    int32_t gatewayid;
    uint32_t blocknum,RTblocknum,NXT_RTblocknum,NXTblocknum,is_realtime,NXT_is_realtime,enable_deposits,enable_withdraws,NXT_ECheight,permblocks;
};

struct syncstate
{
    bits256 majority,minority;
    uint64_t requested[16];
    struct ramsnapshot snaps[16];
    struct syncstate *substate;
    uint32_t blocknum,allocsize;
    uint16_t format,pending,majoritybits,minoritybits;
};

struct ramchain_info
{
    struct mappedblocks blocks,Vblocks,Bblocks,blocks64,blocks4096,*mappedblocks[8];
    struct ramchain_hashtable addrhash,txidhash,scripthash;
    struct MGWstate S,otherS[3],remotesrcs[16];
    double startmilli;
    HUFF *tmphp,*tmphp2,*tmphp3;
    FILE *permfp;
    char name[64],permfname[512],dirpath[512],myipaddr[64],srvNXTACCTSECRET[2048],srvNXTADDR[64],*userpass,*serverport,*marker,*marker2,*opreturnmarker;
    uint32_t next_txid_permind,next_addr_permind,next_script_permind,permind_changes,withdrawconfirms,DEPOSIT_XFER_DURATION;
    uint32_t lastheighttime,min_confirms,estblocktime,firstiter,maxblock,nonzblocks,marker_rawind,marker2_rawind,lastdisp,maxind,numgateways,nummsigs,oldtx;
    uint64_t totalbits,totalbytes,txfee,dust,NXTfee_equiv,minoutput;
    struct rawblock *R,*R2,*R3;
    struct syncstate *verified;
    struct rawblock_huffs H;
    struct alloc_space Tmp,Perm;
    uint64_t minval,maxval,minval2,maxval2,minval4,maxval4,minval8,maxval8;
    struct ramsnapshot *snapshots; bits256 *permhash4096;
    struct NXT_asset *ap;
    int32_t sock;
    uint64_t MGWbits,*limboarray;
    struct cointx_input *MGWunspents;
    uint32_t min_NXTconfirms,NXTtimestamp,MGWnumunspents,MGWmaxunspents,numspecials,depositconfirms,firsttime,firstblock,numpendingsends,pendingticks,remotemode;
    char multisigchar,**special_NXTaddrs,*MGWredemption,*backups,MGWsmallest[256],MGWsmallestB[256],MGWpingstr[1024],mgwstrs[3][8192];
    struct NXT_assettxid *pendingsends[512];
    float lastgetinfo,NXTconvrate;
};

struct multisig_addr
{
    struct storage_header H;
    UT_hash_handle hh;
    char NXTaddr[MAX_NXTADDR_LEN],multisigaddr[MAX_COINADDR_LEN],NXTpubkey[96],redeemScript[2048],coinstr[16],email[128];
    uint64_t sender,modified;
    uint32_t m,n,created,valid,buyNXT;
    struct pubkey_info pubkeys[];
};*/

struct hashtable
{
    char *name;//[128];
    void **hashtable;
    uint64_t hashsize,numsearches,numiterations,numitems;
    long keyoffset,keysize,modifiedoffset,structsize;
};


//struct storage_header **copy_all_DBentries(int32_t *nump,int32_t selector);

void init_jl777(char *myip);
int SuperNET_start(char *JSON_or_fname,char *myip);
char *SuperNET_JSON(char *JSONstr);
char *SuperNET_gotpacket(char *msg,int32_t duration,char *from_ip_port);
int32_t SuperNET_broadcast(char *msg,int32_t duration);
int32_t SuperNET_narrowcast(char *destip,unsigned char *msg,int32_t len);
int32_t got_newpeer(const char *ip_port);

void *portable_thread_create(void *funcp,void *argp);

#endif

