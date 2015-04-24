//
//  ramchain
//  SuperNET
//
//  by jl777 on 12/29/14.
//  MIT license

#ifdef INCLUDE_DEFINES
#ifndef ramchain_h
#define ramchain_h

#ifdef _WIN32
#include "mman-win.h"
#include <io.h>
#include <share.h>
#include <errno.h>
#include <string.h>
#endif

#define MIN_DEPOSIT_FACTOR 5
#define WITHRAW_ENABLE_BLOCKS 3
#define TMPALLOC_SPACE_INCR 10000000
#define PERMALLOC_SPACE_INCR (1024 * 1024 * 128)
#define MAX_PENDINGSENDS_TICKS 50

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <ctype.h>
#include <fcntl.h>

#include "includes/uthash.h"
#ifndef _WIN32
#include <curl/curl.h>
#include <curl/easy.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>
#else
#include "includes/curl.h"
#include "includes/easy.h"
#include <windows.h>
#endif
#include "cJSON.h"

// from libtom
extern uint32_t _crc32(uint32_t crc,const void *buf,size_t size);
extern void calc_sha256cat(unsigned char hash[256 >> 3],unsigned char *src,int32_t len,unsigned char *src2,int32_t len2);

// init and setup
extern int32_t MGW_initdone,PERMUTE_RAWINDS;
extern char Server_ipaddrs[256][MAX_JSON_FIELD];
extern int32_t is_trusted_issuer(char *issuer);
extern struct ramchain_info *get_ramchain_info(char *coinstr);
int32_t bitweight(uint64_t x);

// DB functions
extern struct multisig_addr *find_msigaddr(char *msigaddr);
extern int32_t update_MGW_jsonfile(void (*setfname)(char *fname,char *NXTaddr),void *(*extract_jsondata)(cJSON *item,void *arg,void *arg2),int32_t (*jsoncmp)(void *ref,void *item),char *NXTaddr,char *jsonstr,void *arg,void *arg2);
extern int32_t update_msig_info(struct multisig_addr *msig,int32_t syncflag,char *sender);

// ramchain functions for external access
void *process_ramchains(void *argcoinstr);
char *ramresponse(char *origargstr,char *sender,char *previpaddr,char *datastr);
uint32_t _get_NXTheight(uint32_t *firsttimep);

char *ramstatus(char *origargstr,char *sender,char *previpaddr,char *coin);
char *rampyramid(char *NXTaddr,char *origargstr,char *sender,char *previpaddr,char *coin,uint32_t blocknum,char *typestr);
char *ramstring(char *origargstr,char *sender,char *previpaddr,char *coin,char *typestr,uint32_t rawind);
char *ramrawind(char *origargstr,char *sender,char *previpaddr,char *coin,char *typestr,char *str);
char *ramblock(char *NXTaddr,char *origargstr,char *sender,char *previpaddr,char *coin,uint32_t blocknum);
char *ramcompress(char *origargstr,char *sender,char *previpaddr,char *coin,char *ramhex);
char *ramexpand(char *origargstr,char *sender,char *previpaddr,char *coin,char *bitstream);
char *ramscript(char *origargstr,char *sender,char *previpaddr,char *coin,char *txidstr,int32_t tx_vout,struct address_entry *bp);
char *ramtxlist(char *origargstr,char *sender,char *previpaddr,char *coin,char *coinaddr,int32_t unspentflag);
char *ramrichlist(char *origargstr,char *sender,char *previpaddr,char *coin,int32_t numwhales,int32_t recalcflag);
char *rambalances(char *origargstr,char *sender,char *previpaddr,char *coin,char **coins,double *rates,char ***coinaddrs,int32_t numcoins);
char *ramaddrlist(char *origargstr,char *sender,char *previpaddr,char *coin);

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

struct mappedptr
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
    int32_t sock,do_opreturn;
    uint64_t MGWbits,*limboarray;
    struct cointx_input *MGWunspents;
    uint32_t min_NXTconfirms,NXTtimestamp,MGWnumunspents,MGWmaxunspents,numspecials,depositconfirms,firsttime,firstblock,numpendingsends,pendingticks,remotemode;
    char multisigchar,**special_NXTaddrs,*MGWredemption,*backups,MGWsmallest[256],MGWsmallestB[256],MGWpingstr[1024],mgwstrs[3][8192];
    struct NXT_assettxid *pendingsends[512];
    float lastgetinfo,NXTconvrate;
};

union ramtypes { double dval; uint64_t val64; float fval; uint32_t val32; uint16_t val16; uint8_t val8,hashdata[8]; };
struct ramchain_token
{
    uint32_t numbits:15,ishuffcode:1,offset:16;
    uint32_t rawind;
    char selector,type;
    union ramtypes U;
};

// Bitcoin interface functions
int32_t _extract_txvals(char *coinaddr,char *script,int32_t nohexout,cJSON *txobj);
char *_get_transaction(struct ramchain_info *ram,char *txidstr);
cJSON *_get_blocktxarray(uint32_t *blockidp,int32_t *numtxp,struct ramchain_info *ram,cJSON *blockjson);
cJSON *_get_blockjson(uint32_t *heightp,struct ramchain_info *ram,char *blockhashstr,uint32_t blocknum);
char *_get_blockhashstr(struct ramchain_info *ram,uint32_t blocknum);
void ram_clear_rawblock(struct rawblock *raw,int32_t totalflag);

// system functions
void delete_file(char *fname,int32_t scrubflag);
int32_t compare_files(char *fname,char *fname2); // OS portable
long copy_file(char *src,char *dest); // OS portable
void ensure_dir(char *dirname);
void *init_mappedptr(void **ptrp,struct mappedptr *mp,uint64_t allocsize,int32_t rwflag,char *fname);
void close_mappedptr(struct mappedptr *mp);
void *map_file(char *fname,uint64_t *filesizep,int32_t enablewrite);
int32_t release_map_file(void *ptr,uint64_t filesize);

// string functions
char hexbyte(int32_t c);
int32_t _unhex(char c);
int32_t unhex(char c);
unsigned char _decode_hex(char *hex);
int32_t is_hexstr(char *str);
int32_t decode_hex(unsigned char *bytes,int32_t n,char *hex);
int32_t init_hexbytes_noT(char *hexbytes,unsigned char *message,long len);
char *_mbstr(double n);
char *_mbstr2(double n);
extern int32_t MAP_HUFF;
struct ramchain_hashptr *ram_hashsearch(char *coinstr,struct alloc_space *mem,int32_t createflag,struct ramchain_hashtable *hash,char *hashstr,char type);

#endif
#endif


#ifdef INCLUDE_CODE
#ifndef ramchain_code_h
#define ramchain_code_h
int Numramchains; struct ramchain_info *Ramchains[100];

#define ram_scriptind(ram,hashstr) ram_conv_hashstr(0,1,ram,hashstr,'s')
#define ram_addrind(ram,hashstr) ram_conv_hashstr(0,1,ram,hashstr,'a')
#define ram_txidind(ram,hashstr) ram_conv_hashstr(0,1,ram,hashstr,'t')
// Make sure queries dont autocreate hashtable entries
#define ram_scriptind_RO(permindp,ram,hashstr) ram_conv_hashstr(permindp,0,ram,hashstr,'s')
#define ram_addrind_RO(permindp,ram,hashstr) ram_conv_hashstr(permindp,0,ram,hashstr,'a')
#define ram_txidind_RO(permindp,ram,hashstr) ram_conv_hashstr(permindp,0,ram,hashstr,'t')
uint32_t ram_conv_hashstr(uint32_t *permindp,int32_t createflag,struct ramchain_info *ram,char *hashstr,char type);

#define ram_conv_rawind(hashstr,ram,rawind,type) ram_decode_hashdata(hashstr,type,ram_gethashdata(ram,type,rawind))
#define ram_txid(hashstr,ram,rawind) ram_conv_rawind(hashstr,ram,rawind,'t')
#define ram_addr(hashstr,ram,rawind) ram_conv_rawind(hashstr,ram,rawind,'a')
#define ram_script(hashstr,ram,rawind) ram_conv_rawind(hashstr,ram,rawind,'s')
char *ram_decode_hashdata(char *strbuf,char type,uint8_t *hashdata);
void *ram_gethashdata(struct ramchain_info *ram,char type,uint32_t rawind);

#define ram_addrpayloads(addrptrp,numpayloadsp,ram,addr) ram_payloads(addrptrp,numpayloadsp,ram,addr,'a')
#define ram_txpayloads(txptrp,numpayloadsp,ram,txidstr) ram_payloads(txptrp,numpayloadsp,ram,txidstr,'t')
struct rampayload *ram_payloads(struct ramchain_hashptr **ptrp,int32_t *numpayloadsp,struct ramchain_info *ram,char *hashstr,char type);
struct rampayload *ram_getpayloadi(struct ramchain_hashptr **ptrp,struct ramchain_info *ram,char type,uint32_t rawind,uint32_t i);

#define ram_millis milliseconds

int32_t _valid_txamount(struct ramchain_info *ram,uint64_t value,char *coinaddr)
{
    if ( value >= MIN_DEPOSIT_FACTOR * (ram->txfee + ram->NXTfee_equiv) )
    {
        if ( coinaddr == 0 || (strcmp(coinaddr,ram->marker) != 0 && strcmp(coinaddr,ram->marker2) != 0) )
            return(1);
    }
    return(0);
}

// >>>>>>>>>>>>>>  start system functions

double _kb(double n) { return(n / 1024.); }
double _mb(double n) { return(n / (1024.*1024.)); }
double _gb(double n) { return(n / (1024.*1024.*1024.)); }
double _hrs(double n) { return(n / 3600.); }
double _days(double n) { return(n / (3600. * 24.)); }

char *_mbstr(double n)
{
	static char str[100];
	if ( n < 1024*1024*10 )
		sprintf(str,"%.3fkb",_kb(n));
	else if ( n < 1024*1024*1024 )
		sprintf(str,"%.1fMB",_mb(n));
	else
		sprintf(str,"%.2fGB",_gb(n));
	return(str);
}

char *_mbstr2(double n)
{
	static char str[100];
	if ( n < 1024*1024*10 )
		sprintf(str,"%.3fkb",_kb(n));
	else if ( n < 1024*1024*1024 )
		sprintf(str,"%.1fMB",_mb(n));
	else
		sprintf(str,"%.2fGB",_gb(n));
	return(str);
}

static uint64_t _align16(uint64_t ptrval) { if ( (ptrval & 15) != 0 ) ptrval += 16 - (ptrval & 15); return(ptrval); }

void *alloc_aligned_buffer(uint64_t allocsize)
{
	extern int posix_memalign (void **__memptr, size_t __alignment, size_t __size);
	if ( allocsize > ((uint64_t)192L)*1024*1024*1024 )
    { printf("%llu negative allocsize\n",(long long)allocsize); while ( 1 ) portable_sleep(666); }
	void *ptr;
	allocsize = _align16(allocsize);

	#ifndef _WIN32
	if ( posix_memalign(&ptr,16,allocsize) != 0 )
	#else
	if ( ptr = _aligned_malloc(allocsize, 16) != 0 )
	#endif
		printf("alloc_aligned_buffer can't get allocsize %llu\n",(long long)allocsize);
	if ( ((unsigned long)ptr & 15) != 0 )
    { printf("%p[%llu] alloc_aligned_buffer misaligned\n",ptr,(long long)allocsize); while ( 1 ) sleep(666); }
	if ( allocsize > 1024*1024*1024L )
	{
		void *tmp = ptr;
		long n = allocsize;
		while ( n > 1024*1024*1024L )
		{
			//printf("ptr %p %ld: tmp %p, n %ld\n",ptr,allocsize,tmp,n);
			memset(tmp,0,1024*1024*1024L);
			tmp = (void *)((long)tmp + 1024*1024*1024L);
			n -= 1024*1024*1024L;
			//printf("AFTER ptr %p %ld: tmp %p, n %ld\n",ptr,allocsize,tmp,n);
		}
		if ( n > 0 )
			memset(tmp,0,n);
	}
	else memset(ptr,0,allocsize);
	return(ptr);
}

/*void *map_file(char *fname,uint64_t *filesizep,int32_t enablewrite)
{
	void *mmap64(void *addr,size_t len,int32_t prot,int32_t flags,int32_t fildes,off_t off);
	int32_t fd,rwflags,flags = MAP_FILE|MAP_SHARED;
	uint64_t filesize;
    void *ptr = 0;
	*filesizep = 0;
#ifndef _WIN32
	if ( enablewrite != 0 )
		fd = open(fname,O_RDWR);
	else fd = open(fname,O_RDONLY);
	#else
	if ( enablewrite != 0 )
		fd = _sopen(fname, _O_RDWR | _O_BINARY, _SH_DENYNO);
	else fd = _sopen(fname, _O_RDONLY | _O_BINARY, _SH_DENYNO);
#endif
	if ( fd < 0 )
	{
		printf("map_file: error opening enablewrite.%d %s\n",enablewrite,fname);
        return(0);
	}
    if ( *filesizep == 0 )
        filesize = (uint64_t)lseek(fd,0,SEEK_END);
    else
        filesize = *filesizep;
    //if ( filesize > MAX_MAPFILE_SIZE ) filesize = MAX_MAPFILE_SIZE;
	// printf("filesize %ld vs expected %ld\n",filesize,*filesizep);
	rwflags = PROT_READ;
	if ( enablewrite != 0 )
		rwflags |= PROT_WRITE;
#if __APPLE__ || _WIN32 || __i386__
	ptr = mmap(0,filesize,rwflags,flags,fd,0);
#else
	ptr = mmap64(0,filesize,rwflags,flags,fd,0);
#endif
#ifndef _WIN32
	close(fd);
#else
	_close(fd);
#endif
    if ( ptr == 0 || ptr == MAP_FAILED )
	{
		printf("map_file.write%d: mapping %s failed? mp %p\n",enablewrite,fname,ptr);
		//fatal("FATAL ERROR");
		return(0);
	}
	//if ( 0 && MACHINEID == 2 )
	//	printf("MAPPED(%s).rw%d %lx %ld %.1fmb    | ",fname,enablewrite,filesize,filesize,(double)filesize/1000000);
	*filesizep = filesize;
	return(ptr);
}*/

int32_t release_map_file(void *ptr,uint64_t filesize)
{
	int32_t retval;
    if ( ptr == 0 )
	{
		printf("release_map_file: null ptr\n");
		return(-1);
	}
	retval = munmap(ptr,filesize);
	if ( retval != 0 )
		printf("release_map_file: munmap error %p %llu: err %d\n",ptr,(long long)filesize,retval);
	//else
	//	printf("released %p %ld\n",ptr,filesize);
	return(retval);
}

void _close_mappedptr(struct mappedptr *mp)
{
	//if ( MACHINEID == 2 && mp->actually_allocated == 0 )
	// {
	// printf("map_mappedptr warning: (%s) actually_allocated flag is 0?\n",mp->fname);
	// mp->actually_allocated = 0;
	// }
	if ( mp->actually_allocated != 0 && mp->fileptr != 0 )
		#ifndef _WIN32
		free(mp->fileptr);
		#else
		_aligned_free(mp->fileptr);
		#endif
	else if ( mp->fileptr != 0 )
		release_map_file(mp->fileptr,mp->allocsize);
	mp->fileptr = 0;
}

void close_mappedptr(struct mappedptr *mp)
{
	struct mappedptr tmp;
	tmp = *mp;
	_close_mappedptr(mp);
	memset(mp,0,sizeof(*mp));
	mp->actually_allocated = tmp.actually_allocated;
	mp->allocsize = tmp.allocsize;
	mp->rwflag = tmp.rwflag;
	strcpy(mp->fname,tmp.fname);
}

int32_t open_mappedptr(struct mappedptr *mp)
{
	uint64_t allocsize = mp->allocsize;
    if ( mp->actually_allocated != 0 )
	{
		if ( mp->fileptr == 0 )
			mp->fileptr = alloc_aligned_buffer(mp->allocsize);
		else memset(mp->fileptr,0,mp->allocsize);
		return(0);
	}
	else
	{
		if ( mp->fileptr != 0 )
		{
			//printf("opening already open mappedptr, pending %p\n",mp->pending);
			close_mappedptr(mp);
		}
        mp->allocsize = allocsize;
		// printf("calling map_file with expected %ld\n",mp->allocsize);
		mp->fileptr = map_file(mp->fname,&mp->allocsize,mp->rwflag);
		if ( mp->fileptr == 0 || mp->allocsize != allocsize )
		{
			//printf("error mapping(%s) ptr %p mapped %ld vs allocsize %ld\n",mp->fname,mp->fileptr,mp->allocsize,allocsize);
			return(-1);
		}
		//if ( 0 && MACHINEID == 2 )
		//	printf("mapped (%s) -> %p %ld %s\n",mp->fname,mp->fileptr,mp->allocsize,_mbstr(mp->allocsize));
	}
	return(0);
}

void sync_mappedptr(struct mappedptr *mp,uint64_t len)
{
    static uint64_t Sync_total;
	int32_t err;
	if ( mp->actually_allocated != 0 )
		return;
	if ( len == 0 )
		len = mp->allocsize;
	//printf("sync mp.%p len.%ld\n",mp,len);
	err = msync(mp->fileptr,len,MS_ASYNC);
	if ( err != 0 )
		printf("sync (%s) len %llu, err %d\n",mp->fname,(long long)len,err);
	else if ( 0 )
	{
		release_map_file(mp->fileptr,mp->allocsize);
		mp->fileptr = 0;
		mp->fileptr = map_file(mp->fname,&mp->allocsize,mp->rwflag);
		if ( mp->fileptr == 0 )
        { printf("couldn't get mp->fileptr after sync and close: (%s)\n",mp->fname); while ( 1 ) sleep(666); }
		Sync_total += len;
	}
}

void ensure_filesize(char *fname,long filesize)
{
    FILE *fp;
    char *zeroes;
    long i,n,allocsize = 0;
    //printf("ensure_filesize.(%s) %ld\n",fname,filesize);
    if ( (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
    {
        fseek(fp,0,SEEK_END);
        allocsize = ftell(fp);
        fclose(fp);
        //printf("(%s) exists size.%ld\n",fname,allocsize);
    }
    else
    {
        //printf("try to create.(%s)\n",fname);
        if ( (fp= fopen(os_compatible_path(fname),"wb")) != 0 )
            fclose(fp);
    }
    if ( allocsize < filesize )
    {
        //printf("filesize.%ld is less than %ld\n",filesize,allocsize);
        if ( (fp=fopen(os_compatible_path(fname),"ab")) != 0 )
        {
#ifndef _WIN32
            zeroes = valloc(16*1024*1024);
#else
			zeroes = _aligned_malloc(16*1024*1024, 16);
#endif
            memset(zeroes,0,16*1024*1024);
            n = filesize - allocsize;
            while ( n > 16*1024*1024 )
            {
                fwrite(zeroes,1,16*1024*1024,fp);
                n -= 16*1024*1024;
                fprintf(stderr,".");
            }
            for (i=0; i<n; i++)
                fputc(0,fp);
            fclose(fp);
#ifndef _WIN32
            free(zeroes);
#else
			_aligned_free(zeroes);
#endif
        }
    }
    else if ( allocsize > filesize )
        truncate(fname,filesize);
}

void *init_mappedptr(void **ptrp,struct mappedptr *mp,uint64_t allocsize,int32_t rwflag,char *fname)
{
	uint64_t filesize;
#ifdef WIN32
	mp->actually_allocated = 1;
#endif
    if ( fname != 0 )
	{
		if ( strcmp(mp->fname,fname) == 0 )
		{
			if ( mp->fileptr != 0 )
			{
				release_map_file(mp->fileptr,mp->allocsize);
				mp->fileptr = 0;
			}
			open_mappedptr(mp);
			if ( ptrp != 0 )
				(*ptrp) = mp->fileptr;
			return(mp->fileptr);
		}
		strcpy(mp->fname,fname);
	}
	else mp->actually_allocated = 1;
	mp->rwflag = rwflag;
	mp->allocsize = allocsize;
    if ( rwflag != 0 && mp->actually_allocated == 0 )
        ensure_filesize(fname,allocsize);
	if ( open_mappedptr(mp) != 0 )
	{
	    //printf("init_mappedptr %s.rwflag.%d\n",fname,rwflag);
        if ( allocsize != 0 )
			printf("error mapping(%s) rwflag.%d ptr %p mapped %llu vs allocsize %llu %s\n",fname,rwflag,mp->fileptr,(long long)mp->allocsize,(long long)allocsize,_mbstr(allocsize));
		if ( rwflag != 0 )
		{
			filesize = mp->allocsize;
			if  ( mp->fileptr != 0 )
				release_map_file(mp->fileptr,mp->allocsize);
			mp->allocsize = allocsize;
			mp->changedsize = (allocsize - filesize);
			open_mappedptr(mp);
			if ( mp->fileptr == 0 || mp->allocsize != allocsize )
				printf("SECOND error mapping(%s) ptr %p mapped %llu vs allocsize %llu\n",fname,mp->fileptr,(long long)mp->allocsize,(long long)allocsize);
		}
	}
	if ( ptrp != 0 )
		(*ptrp) = mp->fileptr;
    return(mp->fileptr);
}

void fix_windows_insanity(char *str)
{
#ifdef WIN32
    int32_t i;
    for (i=0; str[i]!=0; i++)
        if ( str[i] == '/' )
            str[i] = '\\';
#endif
}

void ensure_dir(char *dirname) // jl777: does this work in windows?
{
    FILE *fp;
    char fname[512],cmd[512];
    sprintf(fname,"%s/tmp",dirname);
    fix_windows_insanity(fname);
    //printf("ensure.(%s)\n",fname);
    if ( (fp= fopen(os_compatible_path(fname),"rb")) == 0 )
    {
        sprintf(cmd,"mkdir %s",dirname);
        fix_windows_insanity(cmd);
        if ( system(cmd) != 0 )
            printf("error making subdirectory (%s) %s (%s)\n",cmd,dirname,fname);
        fp = fopen(os_compatible_path(fname),"wb");
        if ( fp != 0 )
            fclose(fp);
        if ( (fp= fopen(os_compatible_path(fname),"rb")) == 0 )
        {
            printf("failed to create.(%s) in (%s)\n",fname,dirname);
            exit(-1);
        } else printf("ensure_dir(%s) created.(%s)\n",dirname,fname);
    }
    fclose(fp);
}

int32_t compare_files(char *fname,char *fname2) // OS portable
{
    int32_t offset,errs = 0;
    long len,len2;
    char buf[8192],buf2[8192];
    FILE *fp,*fp2;
    if ( (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
    {
        if ( (fp2= fopen(os_compatible_path(fname2),"rb")) != 0 )
        {
            while ( (len= fread(buf,1,sizeof(buf),fp)) > 0 && (len2= fread(buf2,1,sizeof(buf2),fp2)) == len )
                if ( (offset= memcmp(buf,buf2,len)) != 0 )
                    printf("compare error at offset.%d: (%s) src.%ld vs. (%s) dest.%ld\n",offset,fname,ftell(fp),fname2,ftell(fp2)), errs++;
            //while ( (c= fgetc(srcfp)) != EOF )
            //   fputc(c,destfp);
            fclose(fp2);
        }
        fclose(fp);
    }
    return(errs);
}

long copy_file(char *src,char *dest) // OS portable
{
    long len = -1;
    char buf[8192];
    FILE *srcfp,*destfp;
    if ( (srcfp= fopen(os_compatible_path(src),"rb")) != 0 )
    {
        if ( (destfp= fopen(os_compatible_path(dest),"wb")) != 0 )
        {
            while ( (len= fread(buf,1,sizeof(buf),srcfp)) > 0 )
                if ( fwrite(buf,1,len,destfp) != len )
                    printf("write error at (%s) src.%ld vs. (%s) dest.%ld\n",src,ftell(srcfp),dest,ftell(destfp));
            //while ( (c= fgetc(srcfp)) != EOF )
            //   fputc(c,destfp);
            len = ftell(destfp);
            fclose(destfp);
        }
        fclose(srcfp);
    }
    if ( len == 0 || compare_files(src,dest) != 0 )
        printf("Error copying files (%s) -> (%s)\n",src,dest), len = -1;
    return(len);
}
char *OS_rmstr();
void delete_file(char *fname,int32_t scrubflag)
{
    FILE *fp;
    char cmdstr[1024];
    long i,fpos;
    if ( (fp= fopen(os_compatible_path(fname),"rb+")) != 0 )
    {
        printf("delete(%s)\n",fname);
        if ( scrubflag != 0 )
        {
            fseek(fp,0,SEEK_END);
            fpos = ftell(fp);
            rewind(fp);
            for (i=0; i<fpos; i++)
                fputc(0xff,fp);
            fflush(fp);
        }
        fclose(fp);
        sprintf(cmdstr,"%s %s",OS_rmstr(),fname);
        fix_windows_insanity(cmdstr);
        system(os_compatible_path(cmdstr));
    }
}
/*
int32_t portable_mutex_init(portable_mutex_t *mutex)
{
    return(uv_mutex_init(mutex)); //pthread_mutex_init(mutex,NULL);
}

void portable_mutex_lock(portable_mutex_t *mutex)
{
    //printf("lock.%p\n",mutex);
    uv_mutex_lock(mutex); // pthread_mutex_lock(mutex);
}

void portable_mutex_unlock(portable_mutex_t *mutex)
{
    // printf("unlock.%p\n",mutex);
    uv_mutex_unlock(mutex); //pthread_mutex_unlock(mutex);
}

#define ram_millis milliseconds



void ram_clear_alloc_space(struct alloc_space *mem)
{
    memset(mem->ptr,0,mem->size);
    mem->used = 0;
}*/
double estimate_completion(double startmilli,int32_t processed,int32_t numleft)
{
    double elapsed,rate;
    if ( processed <= 0 )
        return(0.);
    elapsed = (ram_millis() - startmilli);
    rate = (elapsed / processed);
    if ( rate <= 0. )
        return(0.);
    //printf("numleft %d rate %f\n",numleft,rate);
    return(numleft * rate);
}

void *memalloc(struct alloc_space *mem,long size)
{
    void *ptr = 0;
    if ( (mem->used + size) > mem->size )
    {
        printf("alloc: (mem->used %ld + %ld size) %ld > %ld mem->size\n",mem->used,size,(mem->used + size),mem->size);
        while ( 1 )
            portable_sleep(1);
    }
    ptr = (void *)((long)mem->ptr + mem->used);
    mem->used += size;
    memset(ptr,0,size);
    if ( (mem->used & 0xf) != 0 )
        mem->used += 0x10 - (mem->used & 0xf);
    return(ptr);
}

static void ram_clear_alloc_space(struct alloc_space *mem)
{
    memset(mem->ptr,0,mem->size);
    mem->used = 0;
}

void *permalloc(char *coinstr,struct alloc_space *mem,long size,int32_t selector)
{
    static long counts[100],totals[sizeof(counts)/sizeof(*counts)],n;
    struct mappedptr M;
    char fname[1024];
    int32_t i;
    counts[0]++;
    totals[0] += size;
    counts[selector]++;
    totals[selector] += size;
    //printf("mem->used %ld size.%ld | size.%ld\n",mem->used,size,mem->size);
    if ( (mem->used + size) > mem->size )
    {
        for (i=0; i<(int)(sizeof(counts)/sizeof(*counts)); i++)
            if ( counts[i] != 0 )
                printf("(%s %.1f).%d ",_mbstr(totals[i]),(double)totals[i]/counts[i],i);
        printf(" | ");
        printf("permalloc new space.%ld %s | selector.%d itemsize.%ld total.%ld n.%ld ave %.1f | total %s n.%ld ave %.1f\n",mem->size,_mbstr(mem->size),selector,size,totals[selector],counts[selector],(double)totals[selector]/counts[selector],_mbstr2(totals[0]),counts[0],(double)totals[0]/counts[0]);
        memset(&M,0,sizeof(M));
        sprintf(fname,"/tmp/%s.space.%ld",coinstr,n);
        fix_windows_insanity(fname);
        // delete_file(fname,0);
        if ( size > mem->size )
            mem->size = size;
        if ( init_mappedptr(0,&M,mem->size,1,fname) == 0 )
        {
            printf("couldnt create mapped file.(%s)\n",fname);
            exit(-1);
        }
        n++;
        mem->ptr = M.fileptr;
        mem->used = 0;
        if ( mem->size == 0 )
        {
            mem->size = size;
            return(mem->ptr);
        }
    }
    return(memalloc(mem,size));
}


// >>>>>>>>>>>>>>  start varint functions
int32_t hcalc_varint(uint8_t *buf,uint64_t x)
{
    uint16_t s; uint32_t i; int32_t len = 0;
    if ( x < 0xfd )
        buf[len++] = (uint8_t)(x & 0xff);
    else
    {
        if ( x <= 0xffff )
        {
            buf[len++] = 0xfd;
            s = (uint16_t)x;
            memcpy(&buf[len],&s,sizeof(s));
            len += 2;
        }
        else if ( x <= 0xffffffffL )
        {
            buf[len++] = 0xfe;
            i = (uint32_t)x;
            memcpy(&buf[len],&i,sizeof(i));
            len += 4;
        }
        else
        {
            buf[len++] = 0xff;
            memcpy(&buf[len],&x,sizeof(x));
            len += 8;
        }
    }
    return(len);
}

long hemit_varint(FILE *fp,uint64_t x)
{
    uint8_t buf[9];
    int32_t len;
    if ( fp == 0 )
        return(-1);
    long retval = -1;
    if ( (len= hcalc_varint(buf,x)) > 0 )
        retval = fwrite(buf,1,len,fp);
    return(retval);
}

long hdecode_varint(uint64_t *valp,uint8_t *ptr,long offset,long mappedsize)
{
    uint16_t s; uint32_t i; int32_t c;
    if ( ptr == 0 )
        return(-1);
    *valp = 0;
    if ( offset < 0 || offset >= mappedsize )
        return(-1);
    c = ptr[offset++];
    switch ( c )
    {
        case 0xfd: if ( offset+sizeof(s) > mappedsize ) return(-1); memcpy(&s,&ptr[offset],sizeof(s)), *valp = s, offset += sizeof(s); break;
        case 0xfe: if ( offset+sizeof(i) > mappedsize ) return(-1); memcpy(&i,&ptr[offset],sizeof(i)), *valp = i, offset += sizeof(i); break;
        case 0xff: if ( offset+sizeof(*valp) > mappedsize ) return(-1); memcpy(valp,&ptr[offset],sizeof(*valp)), offset += sizeof(*valp); break;
        default: *valp = c; break;
    }
    return(offset);
}

long hload_varint(uint64_t *valp,FILE *fp)
{
    uint16_t s; uint32_t i; int32_t c; int32_t retval = 1;
    if ( fp == 0 )
        return(-1);
    *valp = 0;
    if ( (c= fgetc(fp)) == EOF )
        return(0);
    c &= 0xff;
    switch ( c )
    {
        case 0xfd: retval = (sizeof(s) + 1) * (fread(&s,1,sizeof(s),fp) == sizeof(s)), *valp = s; break;
        case 0xfe: retval = (sizeof(i) + 1) * (fread(&i,1,sizeof(i),fp) == sizeof(i)), *valp = i; break;
        case 0xff: retval = (sizeof(*valp) + 1) * (fread(valp,1,sizeof(*valp),fp) == sizeof(*valp)); break;
        default: *valp = c; break;
    }
    return(retval);
}

// >>>>>>>>>>>>>>  start string functions
char hexbyte(int32_t c)
{
    c &= 0xf;
    if ( c < 10 )
        return('0'+c);
    else if ( c < 16 )
        return('a'+c-10);
    else return(0);
}

int32_t _unhex(char c)
{
    if ( c >= '0' && c <= '9' )
        return(c - '0');
    else if ( c >= 'a' && c <= 'f' )
        return(c - 'a' + 10);
    return(-1);
}

int32_t is_hexstr(char *str)
{
    int32_t i;
    if ( str == 0 || str[0] == 0 )
        return(0);
    for (i=0; str[i]!=0; i++)
        if ( _unhex(str[i]) < 0 )
            return(0);
    return(1);
}

int32_t unhex(char c)
{
    int32_t hex;
    if ( (hex= _unhex(c)) < 0 )
    {
        //printf("unhex: illegal hexchar.(%c)\n",c);
    }
    return(hex);
}

unsigned char _decode_hex(char *hex)
{
    return((unhex(hex[0])<<4) | unhex(hex[1]));
}

int32_t decode_hex(unsigned char *bytes,int32_t n,char *hex)
{
    int32_t adjust,i = 0;
    if ( n == 0 || (hex[n*2+1] == 0 && hex[n*2] != 0) )
    {
        bytes[0] = unhex(hex[0]);
        printf("decode_hex n.%d hex[0] (%c) -> %d\n",n,hex[0],bytes[0]);
        //while ( 1 ) portable_sleep(1);
        bytes++;
        hex++;
        adjust = 1;
    } else adjust = 0;
    if ( n > 0 )
    {
        for (i=0; i<n; i++)
            bytes[i] = _decode_hex(&hex[i*2]);
    }
    //bytes[i] = 0;
    return(n + adjust);
}

int32_t init_hexbytes_noT(char *hexbytes,unsigned char *message,long len)
{
    int32_t i;
    if ( len == 0 )
    {
        hexbytes[0] = 0;
        return(1);
    }
    for (i=0; i<len; i++)
    {
        hexbytes[i*2] = hexbyte((message[i]>>4) & 0xf);
        hexbytes[i*2 + 1] = hexbyte(message[i] & 0xf);
        //printf("i.%d (%02x) [%c%c]\n",i,message[i],hexbytes[i*2],hexbytes[i*2+1]);
    }
    hexbytes[len*2] = 0;
    //printf("len.%ld\n",len*2+1);
    return((int32_t)len*2+1);
}

long _stripwhite(char *buf,int accept)
{
    int32_t i,j,c;
    if ( buf == 0 || buf[0] == 0 )
        return(0);
    for (i=j=0; buf[i]!=0; i++)
    {
        buf[j] = c = buf[i];
        if ( c == accept || (c != ' ' && c != '\n' && c != '\r' && c != '\t' && c != '\b') )
            j++;
    }
    buf[j] = 0;
    return(j);
}

// >>>>>>>>>>>>>>  start bitcoind_RPC interface functions
int32_t _add_opcode(char *hex,int32_t offset,int32_t opcode)
{
    hex[offset + 0] = hexbyte((opcode >> 4) & 0xf);
    hex[offset + 1] = hexbyte(opcode & 0xf);
    return(offset+2);
}

enum opcodetype
{
    // push value
    OP_0 = 0x00,
    OP_FALSE = OP_0,
    OP_PUSHDATA1 = 0x4c,
    OP_PUSHDATA2 = 0x4d,
    OP_PUSHDATA4 = 0x4e,
    OP_1NEGATE = 0x4f,
    OP_RESERVED = 0x50,
    OP_1 = 0x51,
    OP_TRUE=OP_1,
    OP_2 = 0x52,
    OP_3 = 0x53,
    OP_4 = 0x54,
    OP_5 = 0x55,
    OP_6 = 0x56,
    OP_7 = 0x57,
    OP_8 = 0x58,
    OP_9 = 0x59,
    OP_10 = 0x5a,
    OP_11 = 0x5b,
    OP_12 = 0x5c,
    OP_13 = 0x5d,
    OP_14 = 0x5e,
    OP_15 = 0x5f,
    OP_16 = 0x60,
    
    // control
    OP_NOP = 0x61,
    OP_VER = 0x62,
    OP_IF = 0x63,
    OP_NOTIF = 0x64,
    OP_VERIF = 0x65,
    OP_VERNOTIF = 0x66,
    OP_ELSE = 0x67,
    OP_ENDIF = 0x68,
    OP_VERIFY = 0x69,
#define OP_RETURN_OPCODE 0x6a
    OP_RETURN = 0x6a,
    
    // stack ops
    OP_TOALTSTACK = 0x6b,
    OP_FROMALTSTACK = 0x6c,
    OP_2DROP = 0x6d,
    OP_2DUP = 0x6e,
    OP_3DUP = 0x6f,
    OP_2OVER = 0x70,
    OP_2ROT = 0x71,
    OP_2SWAP = 0x72,
    OP_IFDUP = 0x73,
    OP_DEPTH = 0x74,
    OP_DROP = 0x75,
    OP_DUP = 0x76,
    OP_NIP = 0x77,
    OP_OVER = 0x78,
    OP_PICK = 0x79,
    OP_ROLL = 0x7a,
    OP_ROT = 0x7b,
    OP_SWAP = 0x7c,
    OP_TUCK = 0x7d,
    
    // splice ops
    OP_CAT = 0x7e,
    OP_SUBSTR = 0x7f,
    OP_LEFT = 0x80,
    OP_RIGHT = 0x81,
    OP_SIZE = 0x82,
    
    // bit logic
    OP_INVERT = 0x83,
    OP_AND = 0x84,
    OP_OR = 0x85,
    OP_XOR = 0x86,
    OP_EQUAL = 0x87,
    OP_EQUALVERIFY = 0x88,
    OP_RESERVED1 = 0x89,
    OP_RESERVED2 = 0x8a,
    
    // numeric
    OP_1ADD = 0x8b,
    OP_1SUB = 0x8c,
    OP_2MUL = 0x8d,
    OP_2DIV = 0x8e,
    OP_NEGATE = 0x8f,
    OP_ABS = 0x90,
    OP_NOT = 0x91,
    OP_0NOTEQUAL = 0x92,
    
    OP_ADD = 0x93,
    OP_SUB = 0x94,
    OP_MUL = 0x95,
    OP_DIV = 0x96,
    OP_MOD = 0x97,
    OP_LSHIFT = 0x98,
    OP_RSHIFT = 0x99,
    
    OP_BOOLAND = 0x9a,
    OP_BOOLOR = 0x9b,
    OP_NUMEQUAL = 0x9c,
    OP_NUMEQUALVERIFY = 0x9d,
    OP_NUMNOTEQUAL = 0x9e,
    OP_LESSTHAN = 0x9f,
    OP_GREATERTHAN = 0xa0,
    OP_LESSTHANOREQUAL = 0xa1,
    OP_GREATERTHANOREQUAL = 0xa2,
    OP_MIN = 0xa3,
    OP_MAX = 0xa4,
    
    OP_WITHIN = 0xa5,
    
    // crypto
    OP_RIPEMD160 = 0xa6,
    OP_SHA1 = 0xa7,
    OP_SHA256 = 0xa8,
    OP_HASH160 = 0xa9,
    OP_HASH256 = 0xaa,
    OP_CODESEPARATOR = 0xab,
    OP_CHECKSIG = 0xac,
    OP_CHECKSIGVERIFY = 0xad,
    OP_CHECKMULTISIG = 0xae,
    OP_CHECKMULTISIGVERIFY = 0xaf,
    
    // expansion
    OP_NOP1 = 0xb0,
    OP_NOP2 = 0xb1,
    OP_NOP3 = 0xb2,
    OP_NOP4 = 0xb3,
    OP_NOP5 = 0xb4,
    OP_NOP6 = 0xb5,
    OP_NOP7 = 0xb6,
    OP_NOP8 = 0xb7,
    OP_NOP9 = 0xb8,
    OP_NOP10 = 0xb9,
    
    
    
    // template matching params
    OP_SMALLINTEGER = 0xfa,
    OP_PUBKEYS = 0xfb,
    OP_PUBKEYHASH = 0xfd,
    OP_PUBKEY = 0xfe,
    
    OP_INVALIDOPCODE = 0xff,
};

const char *get_opname(enum opcodetype opcode)
{
    switch (opcode)
    {
            // push value
        case OP_0                      : return "0";
        case OP_PUSHDATA1              : return "OP_PUSHDATA1";
        case OP_PUSHDATA2              : return "OP_PUSHDATA2";
        case OP_PUSHDATA4              : return "OP_PUSHDATA4";
        case OP_1NEGATE                : return "-1";
        case OP_RESERVED               : return "OP_RESERVED";
        case OP_1                      : return "1";
        case OP_2                      : return "2";
        case OP_3                      : return "3";
        case OP_4                      : return "4";
        case OP_5                      : return "5";
        case OP_6                      : return "6";
        case OP_7                      : return "7";
        case OP_8                      : return "8";
        case OP_9                      : return "9";
        case OP_10                     : return "10";
        case OP_11                     : return "11";
        case OP_12                     : return "12";
        case OP_13                     : return "13";
        case OP_14                     : return "14";
        case OP_15                     : return "15";
        case OP_16                     : return "16";
            
            // control
        case OP_NOP                    : return "OP_NOP";
        case OP_VER                    : return "OP_VER";
        case OP_IF                     : return "OP_IF";
        case OP_NOTIF                  : return "OP_NOTIF";
        case OP_VERIF                  : return "OP_VERIF";
        case OP_VERNOTIF               : return "OP_VERNOTIF";
        case OP_ELSE                   : return "OP_ELSE";
        case OP_ENDIF                  : return "OP_ENDIF";
        case OP_VERIFY                 : return "OP_VERIFY";
        case OP_RETURN                 : return "OP_RETURN";
            
            // stack ops
        case OP_TOALTSTACK             : return "OP_TOALTSTACK";
        case OP_FROMALTSTACK           : return "OP_FROMALTSTACK";
        case OP_2DROP                  : return "OP_2DROP";
        case OP_2DUP                   : return "OP_2DUP";
        case OP_3DUP                   : return "OP_3DUP";
        case OP_2OVER                  : return "OP_2OVER";
        case OP_2ROT                   : return "OP_2ROT";
        case OP_2SWAP                  : return "OP_2SWAP";
        case OP_IFDUP                  : return "OP_IFDUP";
        case OP_DEPTH                  : return "OP_DEPTH";
        case OP_DROP                   : return "OP_DROP";
        case OP_DUP                    : return "OP_DUP";
        case OP_NIP                    : return "OP_NIP";
        case OP_OVER                   : return "OP_OVER";
        case OP_PICK                   : return "OP_PICK";
        case OP_ROLL                   : return "OP_ROLL";
        case OP_ROT                    : return "OP_ROT";
        case OP_SWAP                   : return "OP_SWAP";
        case OP_TUCK                   : return "OP_TUCK";
            
            // splice ops
        case OP_CAT                    : return "OP_CAT";
        case OP_SUBSTR                 : return "OP_SUBSTR";
        case OP_LEFT                   : return "OP_LEFT";
        case OP_RIGHT                  : return "OP_RIGHT";
        case OP_SIZE                   : return "OP_SIZE";
            
            // bit logic
        case OP_INVERT                 : return "OP_INVERT";
        case OP_AND                    : return "OP_AND";
        case OP_OR                     : return "OP_OR";
        case OP_XOR                    : return "OP_XOR";
        case OP_EQUAL                  : return "OP_EQUAL";
        case OP_EQUALVERIFY            : return "OP_EQUALVERIFY";
        case OP_RESERVED1              : return "OP_RESERVED1";
        case OP_RESERVED2              : return "OP_RESERVED2";
            
            // numeric
        case OP_1ADD                   : return "OP_1ADD";
        case OP_1SUB                   : return "OP_1SUB";
        case OP_2MUL                   : return "OP_2MUL";
        case OP_2DIV                   : return "OP_2DIV";
        case OP_NEGATE                 : return "OP_NEGATE";
        case OP_ABS                    : return "OP_ABS";
        case OP_NOT                    : return "OP_NOT";
        case OP_0NOTEQUAL              : return "OP_0NOTEQUAL";
        case OP_ADD                    : return "OP_ADD";
        case OP_SUB                    : return "OP_SUB";
        case OP_MUL                    : return "OP_MUL";
        case OP_DIV                    : return "OP_DIV";
        case OP_MOD                    : return "OP_MOD";
        case OP_LSHIFT                 : return "OP_LSHIFT";
        case OP_RSHIFT                 : return "OP_RSHIFT";
        case OP_BOOLAND                : return "OP_BOOLAND";
        case OP_BOOLOR                 : return "OP_BOOLOR";
        case OP_NUMEQUAL               : return "OP_NUMEQUAL";
        case OP_NUMEQUALVERIFY         : return "OP_NUMEQUALVERIFY";
        case OP_NUMNOTEQUAL            : return "OP_NUMNOTEQUAL";
        case OP_LESSTHAN               : return "OP_LESSTHAN";
        case OP_GREATERTHAN            : return "OP_GREATERTHAN";
        case OP_LESSTHANOREQUAL        : return "OP_LESSTHANOREQUAL";
        case OP_GREATERTHANOREQUAL     : return "OP_GREATERTHANOREQUAL";
        case OP_MIN                    : return "OP_MIN";
        case OP_MAX                    : return "OP_MAX";
        case OP_WITHIN                 : return "OP_WITHIN";
            
            // crypto
        case OP_RIPEMD160              : return "OP_RIPEMD160";
        case OP_SHA1                   : return "OP_SHA1";
        case OP_SHA256                 : return "OP_SHA256";
        case OP_HASH160                : return "OP_HASH160";
        case OP_HASH256                : return "OP_HASH256";
        case OP_CODESEPARATOR          : return "OP_CODESEPARATOR";
        case OP_CHECKSIG               : return "OP_CHECKSIG";
        case OP_CHECKSIGVERIFY         : return "OP_CHECKSIGVERIFY";
        case OP_CHECKMULTISIG          : return "OP_CHECKMULTISIG";
        case OP_CHECKMULTISIGVERIFY    : return "OP_CHECKMULTISIGVERIFY";
            
            // expanson
        case OP_NOP1                   : return "OP_NOP1";
        case OP_NOP2                   : return "OP_NOP2";
        case OP_NOP3                   : return "OP_NOP3";
        case OP_NOP4                   : return "OP_NOP4";
        case OP_NOP5                   : return "OP_NOP5";
        case OP_NOP6                   : return "OP_NOP6";
        case OP_NOP7                   : return "OP_NOP7";
        case OP_NOP8                   : return "OP_NOP8";
        case OP_NOP9                   : return "OP_NOP9";
        case OP_NOP10                  : return "OP_NOP10";
            
        case OP_INVALIDOPCODE          : return "OP_INVALIDOPCODE";
            
            // Note:
            //  The template matching params OP_SMALLDATA/etc are defined in opcodetype enum
            //  as kind of implementation hack, they are *NOT* real opcodes.  If found in real
            //  Script, just let the default: case deal with them.
            
        default:
            return "OP_UNKNOWN";
    }
}

struct bitcoin_opcode { UT_hash_handle hh; uint8_t opcode; } *optable;
int32_t bitcoin_assembler(char *script)
{
    static struct bitcoin_opcode *optable;
    char hexstr[8192],*hex,*str;
    struct bitcoin_opcode *op;
    char *opname,*invalid = "OP_UNKNOWN";
    int32_t i,j;
    long len,k;
    if ( optable == 0 )
    {
        for (i=0; i<0x100; i++)
        {
            opname = (char *)get_opname(i);
            if ( strcmp(invalid,opname) != 0 )
            {
                op = calloc(1,sizeof(*op));
                HASH_ADD_KEYPTR(hh,optable,opname,strlen(opname),op);
                //printf("{%-16s %02x} ",opname,i);
                op->opcode = i;
            }
        }
        //printf("bitcoin opcodes\n");
    }
    if ( script[0] == 0 )
    {
        strcpy(script,"ffff");
        return(-1);
    }
    len = strlen(script);
    hex = (len < sizeof(hexstr)-2) ? hexstr : calloc(1,len+1);
    strcpy(hex,"ffff");
    str = script;
    k = 0;
    script[len + 1] = 0;
    while ( *str != 0 )
    {
        //printf("k.%ld (%s)\n",k,str);
        for (j=0; str[j]!=0&&str[j]!=' '; j++)
            ;
        str[j] = 0;
        len = strlen(str);
        if ( is_hexstr(str) != 0 && (len & 1) == 0 )
        {
            k = _add_opcode(hex,(int32_t)k,(int32_t)len>>1);
            strcpy(hex+k,str);
            k += len;//, printf("%s ",str);
        }
        else
        {
            HASH_FIND(hh,optable,str,len,op);
            if ( op != 0 )
                k = _add_opcode(hex,(int32_t)k,op->opcode);
            //sprintf(hex+k,"%02x",op->opcode), k += 2, printf("{%s}.%02x ",str,op->opcode);
        }
        str += (j+1);
    }
    hex[k] = 0;
    strcpy(script,hex);
    if ( hex != hexstr )
        free(hex);
    //fprintf(stderr,"-> (%s).k%ld\n",script,k);
    return((is_hexstr(script) != 0 && (strlen(script) & 1) == 0) ? 0 : -1);
}

cJSON *_script_has_address(int32_t *nump,cJSON *scriptobj)
{
    int32_t i,n;
    cJSON *addresses,*addrobj;
    if ( scriptobj == 0 )
        return(0);
    addresses = cJSON_GetObjectItem(scriptobj,"addresses");
    *nump = 0;
    if ( addresses != 0 )
    {
        *nump = n = cJSON_GetArraySize(addresses);
        for (i=0; i<n; i++)
        {
            addrobj = cJSON_GetArrayItem(addresses,i);
            return(addrobj);
        }
    }
    return(0);
}

int32_t _extract_txvals(char *coinaddr,char *script,int32_t nohexout,cJSON *txobj)
{
    int32_t numaddresses;
    cJSON *scriptobj,*addrobj,*hexobj;
    scriptobj = cJSON_GetObjectItem(txobj,"scriptPubKey");
    if ( scriptobj != 0 )
    {
        addrobj = _script_has_address(&numaddresses,scriptobj);
        if ( coinaddr != 0 )
            copy_cJSON(coinaddr,addrobj);
        if ( nohexout != 0 )
            hexobj = cJSON_GetObjectItem(scriptobj,"asm");
        else
        {
            hexobj = cJSON_GetObjectItem(scriptobj,"hex");
            if ( hexobj == 0 )
                hexobj = cJSON_GetObjectItem(scriptobj,"asm"), nohexout = 1;
        }
        if ( script != 0 )
        {
            copy_cJSON(script,hexobj);
            if ( nohexout != 0 )
            {
                //fprintf(stderr,"{%s} ",script);
                bitcoin_assembler(script);
            }
        }
        return(0);
    }
    return(-1);
}

void _set_string(char type,char *dest,char *src,long max)
{
    if ( src == 0 || src[0] == 0 )
        sprintf(dest,"ffff");
    else if ( strlen(src) < max-1)
        strcpy(dest,src);
    else sprintf(dest,"nonstandard");
}

uint64_t _get_txvouts(struct rawblock *raw,struct rawtx *tx,struct ramchain_info *ram,cJSON *voutsobj)
{
    cJSON *item;
    uint64_t value,total = 0;
    struct rawvout *v;
    int32_t i,numvouts = 0;
    char coinaddr[8192],script[8192];
    tx->firstvout = raw->numrawvouts;
    if ( voutsobj != 0 && is_cJSON_Array(voutsobj) != 0 && (numvouts= cJSON_GetArraySize(voutsobj)) > 0 && tx->firstvout+numvouts < MAX_BLOCKTX )
    {
        for (i=0; i<numvouts; i++,raw->numrawvouts++)
        {
            item = cJSON_GetArrayItem(voutsobj,i);
            value = conv_cJSON_float(item,"value");
            v = &raw->voutspace[raw->numrawvouts];
            memset(v,0,sizeof(*v));
            v->value = value;
            total += value;
            _extract_txvals(coinaddr,script,1,item); // default to nohexout
            _set_string('a',v->coinaddr,coinaddr,sizeof(v->coinaddr));
            _set_string('s',v->script,script,sizeof(v->script));
        }
    } else printf("error with vouts\n");
    tx->numvouts = numvouts;
    return(total);
}

int32_t _get_txvins(struct rawblock *raw,struct rawtx *tx,struct ramchain_info *ram,cJSON *vinsobj)
{
    cJSON *item;
    struct rawvin *v;
    int32_t i,numvins = 0;
    char txidstr[8192],coinbase[8192];
    tx->firstvin = raw->numrawvins;
    if ( vinsobj != 0 && is_cJSON_Array(vinsobj) != 0 && (numvins= cJSON_GetArraySize(vinsobj)) > 0 && tx->firstvin+numvins < MAX_BLOCKTX )
    {
        for (i=0; i<numvins; i++,raw->numrawvins++)
        {
            item = cJSON_GetArrayItem(vinsobj,i);
            if ( numvins == 1  )
            {
                copy_cJSON(coinbase,cJSON_GetObjectItem(item,"coinbase"));
                if ( strlen(coinbase) > 1 )
                    return(0);
            }
            copy_cJSON(txidstr,cJSON_GetObjectItem(item,"txid"));
            v = &raw->vinspace[raw->numrawvins];
            memset(v,0,sizeof(*v));
            v->vout = (int)get_cJSON_int(item,"vout");
            _set_string('t',v->txidstr,txidstr,sizeof(v->txidstr));
        }
    } else printf("error with vins\n");
    tx->numvins = numvins;
    return(numvins);
}

char *_get_transaction(struct ramchain_info *ram,char *txidstr)
{
    char *rawtransaction=0,txid[4096];
    sprintf(txid,"[\"%s\", 1]",txidstr);
    //printf("get_transaction.(%s)\n",txidstr);
    rawtransaction = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"getrawtransaction",txid);
    return(rawtransaction);
}

uint64_t _get_txidinfo(struct rawblock *raw,struct rawtx *tx,struct ramchain_info *ram,int32_t txind,char *txidstr)
{
    char *retstr = 0;
    cJSON *txjson;
    uint64_t total = 0;
    if ( strlen(txidstr) < sizeof(tx->txidstr)-1 )
        strcpy(tx->txidstr,txidstr);
    tx->numvouts = tx->numvins = 0;
    if ( (retstr= _get_transaction(ram,txidstr)) != 0 )
    {
        if ( (txjson= cJSON_Parse(retstr)) != 0 )
        {
            _get_txvins(raw,tx,ram,cJSON_GetObjectItem(txjson,"vin"));
            total = _get_txvouts(raw,tx,ram,cJSON_GetObjectItem(txjson,"vout"));
            free_json(txjson);
        } else printf("update_txid_infos parse error.(%s)\n",retstr);
        free(retstr);
    } else printf("error getting.(%s)\n",txidstr);
    //printf("tx.%d: (%s) numvins.%d numvouts.%d (raw %d %d)\n",txind,tx->txidstr,tx->numvins,tx->numvouts,raw->numrawvins,raw->numrawvouts);
    return(total);
}

char *_get_blockhashstr(struct ramchain_info *ram,uint32_t blocknum)
{
    char numstr[128],*blockhashstr=0;
    sprintf(numstr,"%u",blocknum);
    blockhashstr = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"getblockhash",numstr);
    if ( blockhashstr == 0 || blockhashstr[0] == 0 )
    {
        printf("couldnt get blockhash for %u\n",blocknum);
        if ( blockhashstr != 0 )
            free(blockhashstr);
        return(0);
    }
    return(blockhashstr);
}

cJSON *_get_blockjson(uint32_t *heightp,struct ramchain_info *ram,char *blockhashstr,uint32_t blocknum)
{
    cJSON *json = 0;
    int32_t flag = 0;
    char buf[1024],*blocktxt = 0;
    if ( blockhashstr == 0 )
        blockhashstr = _get_blockhashstr(ram,blocknum), flag = 1;
    if ( blockhashstr != 0 )
    {
        sprintf(buf,"\"%s\"",blockhashstr);
        //printf("get_blockjson.(%d %s)\n",blocknum,blockhashstr);
        blocktxt = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"getblock",buf);
        if ( blocktxt != 0 && blocktxt[0] != 0 && (json= cJSON_Parse(blocktxt)) != 0 && heightp != 0 )
            *heightp = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"height"),0xffffffff);
        if ( flag != 0 && blockhashstr != 0 )
            free(blockhashstr);
        if ( blocktxt != 0 )
            free(blocktxt);
    }
    return(json);
}

cJSON *_get_blocktxarray(uint32_t *blockidp,int32_t *numtxp,struct ramchain_info *ram,cJSON *blockjson)
{
    cJSON *txarray = 0;
    if ( blockjson != 0 )
    {
        *blockidp = (uint32_t)get_API_int(cJSON_GetObjectItem(blockjson,"height"),0);
        txarray = cJSON_GetObjectItem(blockjson,"tx");
        *numtxp = cJSON_GetArraySize(txarray);
    }
    return(txarray);
}

uint32_t _get_blockinfo(struct rawblock *raw,struct ramchain_info *ram,uint32_t blocknum)
{
    char txidstr[8192],mintedstr[8192];
    cJSON *json,*txobj;
    uint32_t blockid;
    int32_t txind,n;
    uint64_t total = 0;
    ram_clear_rawblock(raw,1);
    //raw->blocknum = blocknum;
    //printf("_get_blockinfo.%d\n",blocknum);
    raw->minted = raw->numtx = raw->numrawvins = raw->numrawvouts = 0;
    if ( (json= _get_blockjson(0,ram,0,blocknum)) != 0 )
    {
        raw->blocknum = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"height"),0);
        copy_cJSON(mintedstr,cJSON_GetObjectItem(json,"mint"));
        if ( mintedstr[0] != 0 )
            raw->minted = (uint64_t)(atof(mintedstr) * SATOSHIDEN);
        if ( (txobj= _get_blocktxarray(&blockid,&n,ram,json)) != 0 && blockid == blocknum && n < MAX_BLOCKTX )
        {
            for (txind=0; txind<n; txind++)
            {
                copy_cJSON(txidstr,cJSON_GetArrayItem(txobj,txind));
                //printf("block.%d txind.%d TXID.(%s)\n",blocknum,txind,txidstr);
                total += _get_txidinfo(raw,&raw->txspace[raw->numtx++],ram,txind,txidstr);
            }
        } else printf("error _get_blocktxarray for block.%d got %d, n.%d vs %d\n",blocknum,blockid,n,MAX_BLOCKTX);
        if ( raw->minted == 0 )
            raw->minted = total;
        free_json(json);
    } else printf("get_blockjson error parsing.(%s)\n",txidstr);
    //printf("BLOCK.%d: block.%d numtx.%d minted %.8f rawnumvins.%d rawnumvouts.%d\n",blocknum,raw->blocknum,raw->numtx,dstr(raw->minted),raw->numrawvins,raw->numrawvouts);
    return(raw->numtx);
}

cJSON *_get_localaddresses(struct ramchain_info *ram)
{
    char *retstr;
    cJSON *json = 0;
    retstr = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"listaddressgroupings","");
    if ( retstr != 0 )
    {
        json = cJSON_Parse(retstr);
        free(retstr);
    }
    return(json);
}

int32_t _validate_coinaddr(char pubkey[512],struct ramchain_info *ram,char *coinaddr)
{
    char quotes[512],*retstr;
    int64_t len = 0;
    cJSON *json;
    if ( coinaddr[0] != '"' )
        sprintf(quotes,"\"%s\"",coinaddr);
    else safecopy(quotes,coinaddr,sizeof(quotes));
    if ( (retstr= bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"validateaddress",quotes)) != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            copy_cJSON(pubkey,cJSON_GetObjectItem(json,"pubkey"));
            len = (int32_t)strlen(pubkey);
            free_json(json);
        }
        free(retstr);
    }
    return((int32_t)len);
}

int32_t _verify_coinaddress(char *account,int32_t *ismultisigp,int32_t *isminep,struct ramchain_info *ram,char *coinaddr)
{
    char arg[1024],str[MAX_JSON_FIELD],addr[MAX_JSON_FIELD],*retstr;
    cJSON *json,*array;
    struct ramchain_hashptr *addrptr;
    int32_t i,n,verified = 0;
    sprintf(arg,"\"%s\"",coinaddr);
    *ismultisigp = *isminep = 0;
    retstr = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"validateaddress",arg);
    if ( retstr != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            //if ( is_cJSON_True(cJSON_GetObjectItem(json,"ismine")) != 0 )
            //    *isminep = 1;
            copy_cJSON(str,cJSON_GetObjectItem(json,"script"));
            copy_cJSON(account,cJSON_GetObjectItem(json,"account"));
            if ( strcmp(str,"multisig") == 0 )
                *ismultisigp = 1;
            if ( (array= cJSON_GetObjectItem(json,"addresses")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    copy_cJSON(addr,cJSON_GetArrayItem(array,i));
                    if ( addr[0] != 0 && (addrptr= ram_hashsearch(ram->name,0,0,&ram->addrhash,coinaddr,'a')) != 0 && addrptr->mine != 0 )
                    {
                        *isminep = 1;
                        break;
                    }
                }
            }
            verified = 1;
            free_json(json);
        }
        free(retstr);
    }
    return(verified);
}

int32_t _map_msigaddr(char *redeemScript,struct ramchain_info *ram,char *normaladdr,char *msigaddr) //could map to rawind, but this is rarely called
{
    int32_t i,n,ismine;
    cJSON *json,*array,*json2;
    struct multisig_addr *msig;
    char addr[1024],args[1024],*retstr,*retstr2;
    redeemScript[0] = normaladdr[0] = 0;
    if ( (msig= find_msigaddr(msigaddr)) == 0 )
    {
        strcpy(normaladdr,msigaddr);
        printf("cant find_msigaddr.(%s)\n",msigaddr);
        return(0);
    }
    if ( msig->redeemScript[0] != 0 && ram->S.gatewayid >= 0 && ram->S.gatewayid < NUM_GATEWAYS )
    {
        strcpy(normaladdr,msig->pubkeys[ram->S.gatewayid].coinaddr);
        strcpy(redeemScript,msig->redeemScript);
        printf("_map_msigaddr.(%s) -> return (%s) redeem.(%s)\n",msigaddr,normaladdr,redeemScript);
        return(1);
    }
    sprintf(args,"\"%s\"",msig->multisigaddr);
    retstr = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"validateaddress",args);
    if ( retstr != 0 )
    {
        printf("got retstr.(%s)\n",retstr);
        if ( (json = cJSON_Parse(retstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"addresses")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    ismine = 0;
                    copy_cJSON(addr,cJSON_GetArrayItem(array,i));
                    if ( addr[0] != 0 )
                    {
                        sprintf(args,"\"%s\"",addr);
                        retstr2 = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"validateaddress",args);
                        if ( retstr2 != 0 )
                        {
                            if ( (json2 = cJSON_Parse(retstr2)) != 0 )
                                ismine = is_cJSON_True(cJSON_GetObjectItem(json2,"ismine"));
                            free(retstr2);
                        }
                    }
                    if ( ismine != 0 )
                    {
                        //printf("(%s) ismine.%d\n",addr,ismine);
                        strcpy(normaladdr,addr);
                        copy_cJSON(redeemScript,cJSON_GetObjectItem(json,"hex"));
                        break;
                    }
                }
            } free_json(json);
        } free(retstr);
    }
    if ( normaladdr[0] != 0 )
        return(1);
    strcpy(normaladdr,msigaddr);
    return(-1);
}

cJSON *_create_privkeys_json_params(struct ramchain_info *ram,struct cointx_info *cointx,char **privkeys,int32_t numinputs)
{
    int32_t allocflag,i,ret,nonz = 0;
    cJSON *array;
    char args[1024],normaladdr[1024],redeemScript[4096];
    //printf("create privkeys %p numinputs.%d\n",privkeys,numinputs);
    if ( privkeys == 0 )
    {
        privkeys = calloc(numinputs,sizeof(*privkeys));
        for (i=0; i<numinputs; i++)
        {
            if ( (ret= _map_msigaddr(redeemScript,ram,normaladdr,cointx->inputs[i].coinaddr)) >= 0 )
            {
                sprintf(args,"[\"%s\"]",normaladdr);
                //fprintf(stderr,"(%s) -> (%s).%d ",normaladdr,normaladdr,i);
                privkeys[i] = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"dumpprivkey",args);
            } else fprintf(stderr,"ret.%d for %d (%s)\n",ret,i,normaladdr);
        }
        allocflag = 1;
        //fprintf(stderr,"allocated\n");
    } else allocflag = 0;
    array = cJSON_CreateArray();
    for (i=0; i<numinputs; i++)
    {
        if ( ram != 0 && privkeys[i] != 0 )
        {
            nonz++;
            //printf("(%s %s) ",privkeys[i],cointx->inputs[i].coinaddr);
            cJSON_AddItemToArray(array,cJSON_CreateString(privkeys[i]));
        }
    }
    if ( nonz == 0 )
        free_json(array), array = 0;
   // else printf("privkeys.%d of %d: %s\n",nonz,numinputs,cJSON_Print(array));
    if ( allocflag != 0 )
    {
        for (i=0; i<numinputs; i++)
            if ( privkeys[i] != 0 )
                free(privkeys[i]);
        free(privkeys);
    }
    return(array);
}

cJSON *_create_vins_json_params(char **localcoinaddrs,struct ramchain_info *ram,struct cointx_info *cointx)
{
    int32_t i,ret;
    char normaladdr[1024],redeemScript[4096];
    cJSON *json,*array;
    struct cointx_input *vin;
    array = cJSON_CreateArray();
    for (i=0; i<cointx->numinputs; i++)
    {
        vin = &cointx->inputs[i];
        if ( localcoinaddrs != 0 )
            localcoinaddrs[i] = 0;
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json,"txid",cJSON_CreateString(vin->tx.txidstr));
        cJSON_AddItemToObject(json,"vout",cJSON_CreateNumber(vin->tx.vout));
        cJSON_AddItemToObject(json,"scriptPubKey",cJSON_CreateString(vin->sigs));
        if ( (ret= _map_msigaddr(redeemScript,ram,normaladdr,vin->coinaddr)) >= 0 )
            cJSON_AddItemToObject(json,"redeemScript",cJSON_CreateString(redeemScript));
        else printf("ret.%d redeemScript.(%s) (%s) for (%s)\n",ret,redeemScript,normaladdr,vin->coinaddr);
        if ( localcoinaddrs != 0 )
            localcoinaddrs[i] = vin->coinaddr;
        cJSON_AddItemToArray(array,json);
    }
    return(array);
}

char *_createsignraw_json_params(struct ramchain_info *ram,struct cointx_info *cointx,char *rawbytes,char **privkeys)
{
    char *paramstr = 0;
    cJSON *array,*rawobj,*vinsobj=0,*keysobj=0;
    rawobj = cJSON_CreateString(rawbytes);
    if ( rawobj != 0 )
    {
        vinsobj = _create_vins_json_params(0,ram,cointx);
        if ( vinsobj != 0 )
        {
            keysobj = _create_privkeys_json_params(ram,cointx,privkeys,cointx->numinputs);
            if ( keysobj != 0 )
            {
                array = cJSON_CreateArray();
                cJSON_AddItemToArray(array,rawobj);
                cJSON_AddItemToArray(array,vinsobj);
                cJSON_AddItemToArray(array,keysobj);
                paramstr = cJSON_Print(array);
                free_json(array);
            }
            else free_json(vinsobj);
        }
        else free_json(rawobj);
        //printf("vinsobj.%p keysobj.%p rawobj.%p\n",vinsobj,keysobj,rawobj);
    }
    return(paramstr);
}

int32_t _sign_rawtransaction(char *deststr,unsigned long destsize,struct ramchain_info *ram,struct cointx_info *cointx,char *rawbytes,char **privkeys)
{
    cJSON *json,*hexobj,*compobj;
    int32_t completed = -1;
    char *retstr,*signparams;
    deststr[0] = 0;
    //printf("sign_rawtransaction rawbytes.(%s) %p\n",rawbytes,privkeys);
    if ( (signparams= _createsignraw_json_params(ram,cointx,rawbytes,privkeys)) != 0 )
    {
        _stripwhite(signparams,0);
        printf("got signparams.(%s)\n",signparams);
        retstr = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"signrawtransaction",signparams);
        if ( retstr != 0 )
        {
            printf("got retstr.(%s)\n",retstr);
            json = cJSON_Parse(retstr);
            if ( json != 0 )
            {
                hexobj = cJSON_GetObjectItem(json,"hex");
                compobj = cJSON_GetObjectItem(json,"complete");
                if ( compobj != 0 )
                    completed = ((compobj->type&0xff) == cJSON_True);
                copy_cJSON(deststr,hexobj);
                if ( strlen(deststr) > destsize )
                    printf("sign_rawtransaction: strlen(deststr) %ld > %ld destize\n",strlen(deststr),destsize);
                //printf("got signedtransaction.(%s) ret.(%s) completed.%d\n",deststr,retstr,completed);
                free_json(json);
            } else printf("json parse error.(%s)\n",retstr);
            free(retstr);
        } else printf("error signing rawtx\n");
        free(signparams);
    } else printf("error generating signparams\n");
    return(completed);
}

char *_submit_withdraw(struct ramchain_info *ram,struct cointx_info *cointx,char *othersignedtx)
{
    FILE *fp;
    long len;
    int32_t retval = 0;
    char fname[512],cointxid[4096],*signed2transaction,*retstr;
    len = strlen(othersignedtx);
    fprintf(stderr,"submit_withdraw.(%s) len.%ld sizeof cointx.%ld\n",othersignedtx,len,sizeof(cointx));
    signed2transaction = calloc(1,2*len);
    if ( ram->S.gatewayid >= 0 && (retval= _sign_rawtransaction(signed2transaction+2,len+4000,ram,cointx,othersignedtx,0)) > 0 )
    {
        signed2transaction[0] = '[';
        signed2transaction[1] = '"';
        strcat(signed2transaction,"\"]");
        //printf("sign2.(%s)\n",signed2transaction);
        retstr = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"sendrawtransaction",signed2transaction);
        if ( retstr != 0 )
        {
            //printf("got submitraw.(%s)\n",cointxid);
            safecopy(cointxid,retstr,sizeof(cointxid));
            free(retstr);
            if ( cointxid[0] != 0 )
            {
                sprintf(fname,"%s/%s.%s",ram->backups,cointxid,ram->name);
                if ( (fp= fopen(os_compatible_path(fname),"w")) != 0 )
                {
                    fprintf(fp,"%s\n",signed2transaction);
                    fclose(fp);
                    printf("wrote.(%s) to file.(%s)\n",signed2transaction,fname);
                }
                else printf("unexpected %s cointxid.%s already there before submit??\n",ram->name,cointxid);
                printf("rawtxid len.%ld submitted.%s\n",strlen(signed2transaction),cointxid);
                free(signed2transaction);
                return(clonestr(cointxid));
            } else printf("error null cointxid\n");
        } else printf("error submit raw.%s\n",signed2transaction);
    } else printf("error 2nd sign.%s retval.%d\n",othersignedtx,retval);
    free(signed2transaction);
    return(0);
}

char *_sign_and_sendmoney(char *cointxid,struct ramchain_info *ram,struct cointx_info *cointx,char *othersignedtx,uint64_t *redeems,uint64_t *amounts,int32_t numredeems)
{
    int32_t _ram_update_redeembits(struct ramchain_info *ram,uint64_t redeembits,uint64_t AMtxidbits,char *cointxid,struct address_entry *bp);
    uint64_t get_sender(uint64_t *amountp,char *txidstr);
    void *extract_jsonkey(cJSON *item,void *arg,void *arg2);
    void set_MGW_moneysentfname(char *fname,char *NXTaddr);
    int32_t jsonstrcmp(void *ref,void *item);
    char txidstr[64],NXTaddr[64],jsonstr[4096],*retstr = 0;
    int32_t i;
    uint64_t amount,senderbits,redeemtxid;
    fprintf(stderr,"achieved consensus and sign! (%s)\n",othersignedtx);
    if ( (retstr= _submit_withdraw(ram,cointx,othersignedtx)) != 0 )
    {
        if ( is_hexstr(retstr) != 0 )
        {
            strcpy(cointxid,retstr);
            //*AMtxidp = _broadcast_moneysentAM(ram,height);
            for (i=0; i<numredeems; i++)
            {
                if ( (redeemtxid = redeems[i]) != 0 && amounts[i] != 0 )
                {
                    printf("signed and sent.%d: %llu %.8f\n",i,(long long)redeemtxid,dstr(amounts[i]));
                    _ram_update_redeembits(ram,redeemtxid,0,cointxid,0);
                    expand_nxt64bits(txidstr,redeemtxid);
                    senderbits = get_sender(&amount,txidstr);
                    expand_nxt64bits(NXTaddr,senderbits);
                    sprintf(jsonstr,"{\"NXT\":\"%s\",\"redeemtxid\":\"%llu\",\"amount\":\"%.8f\",\"coin\":\"%s\",\"cointxid\":\"%s\",\"vout\":\"%d\"}",NXTaddr,(long long)redeemtxid,dstr(amounts[i]),ram->name,txidstr,i);
                    update_MGW_jsonfile(set_MGW_moneysentfname,extract_jsonkey,jsonstrcmp,0,jsonstr,"redeemtxid",0);
                    update_MGW_jsonfile(set_MGW_moneysentfname,extract_jsonkey,jsonstrcmp,NXTaddr,jsonstr,"redeemtxid",0);
                }
            }
            //backupwallet(cp,ram->coinid);
        }
        else
        {
            for (i=0; i<numredeems; i++)
                printf("(%llu %.8f) ",(long long)redeems[i],dstr(amounts[i]));
            printf("_sign_and_sendmoney: unexpected return.(%s)\n",retstr);
            exit(1);
        }
        return(retstr);
    }
    else printf("sign_and_sendmoney: error sending rawtransaction %s\n",othersignedtx);
    return(0);
}

struct cointx_input *_find_bestfit(struct ramchain_info *ram,uint64_t value)
{
    uint64_t above,below,gap;
    int32_t i;
    struct cointx_input *vin,*abovevin,*belowvin;
    abovevin = belowvin = 0;
    for (above=below=i=0; i<ram->MGWnumunspents; i++)
    {
        vin = &ram->MGWunspents[i];
        if ( vin->used != 0 )
            continue;
        if ( vin->value == value )
            return(vin);
        else if ( vin->value > value )
        {
            gap = (vin->value - value);
            if ( above == 0 || gap < above )
            {
                above = gap;
                abovevin = vin;
            }
        }
        else
        {
            gap = (value - vin->value);
            if ( below == 0 || gap < below )
            {
                below = gap;
                belowvin = vin;
            }
        }
    }
    return((abovevin != 0) ? abovevin : belowvin);
}

int64_t _calc_cointx_inputs(struct ramchain_info *ram,struct cointx_info *cointx,int64_t amount)
{
    int64_t remainder,sum = 0;
    int32_t i;
    struct cointx_input *vin;
    cointx->inputsum = cointx->numinputs = 0;
    remainder = amount + ram->txfee;
    for (i=0; i<ram->MGWnumunspents&&i<((int)(sizeof(cointx->inputs)/sizeof(*cointx->inputs)))-1; i++)
    {
        if ( (vin= _find_bestfit(ram,remainder)) != 0 )
        {
            sum += vin->value;
            remainder -= vin->value;
            vin->used = 1;
            cointx->inputs[cointx->numinputs++] = *vin;
            if ( sum >= (amount + ram->txfee) )
            {
                cointx->amount = amount;
                cointx->change = (sum - amount - ram->txfee);
                cointx->inputsum = sum;
                fprintf(stderr,"numinputs %d sum %.8f vs amount %.8f change %.8f -> miners %.8f\n",cointx->numinputs,dstr(cointx->inputsum),dstr(amount),dstr(cointx->change),dstr(sum - cointx->change - cointx->amount));
                return(cointx->inputsum);
            }
        } else printf("no bestfit found i.%d of %d\n",i,ram->MGWnumunspents);
    }
    fprintf(stderr,"error numinputs %d sum %.8f\n",cointx->numinputs,dstr(cointx->inputsum));
    return(0);
}

char *_sign_localtx(struct ramchain_info *ram,struct cointx_info *cointx,char *rawbytes)
{
    char *batchsigned;
    cointx->batchsize = (uint32_t)strlen(rawbytes) + 1;
    cointx->batchcrc = _crc32(0,rawbytes+12,cointx->batchsize-12); // skip past timediff
    batchsigned = malloc(cointx->batchsize + cointx->numinputs*512 + 512);
    _sign_rawtransaction(batchsigned,cointx->batchsize + cointx->numinputs*512 + 512,ram,cointx,rawbytes,0);
    return(batchsigned);
}

cJSON *_create_vouts_json_params(struct cointx_info *cointx)
{
    int32_t i;
    cJSON *json,*obj;
    json = cJSON_CreateObject();
    for (i=0; i<cointx->numoutputs; i++)
    {
        obj = cJSON_CreateNumber(dstr(cointx->outputs[i].value));
        if ( strcmp(cointx->outputs[i].coinaddr,"OP_RETURN") != 0 )
            cJSON_AddItemToObject(json,cointx->outputs[i].coinaddr,obj);
        else
        {
            // int32_t ram_make_OP_RETURN(char *scriptstr,uint64_t *redeems,int32_t numredeems)
            cJSON_AddItemToObject(json,cointx->outputs[0].coinaddr,obj);
        }
    }
    printf("numdests.%d (%s)\n",cointx->numoutputs,cJSON_Print(json));
    return(json);
}

char *_createrawtxid_json_params(struct ramchain_info *ram,struct cointx_info *cointx)
{
    char *paramstr = 0;
    cJSON *array,*vinsobj,*voutsobj;
    vinsobj = _create_vins_json_params(0,ram,cointx);
    if ( vinsobj != 0 )
    {
        voutsobj = _create_vouts_json_params(cointx);
        if ( voutsobj != 0 )
        {
            array = cJSON_CreateArray();
            cJSON_AddItemToArray(array,vinsobj);
            cJSON_AddItemToArray(array,voutsobj);
            paramstr = cJSON_Print(array);
            free_json(array);   // this frees both vinsobj and voutsobj
        }
        else free_json(vinsobj);
    } else printf("_error create_vins_json_params\n");
    //printf("_createrawtxid_json_params.%s\n",paramstr);
    return(paramstr);
}

int32_t _make_OP_RETURN(char *scriptstr,uint64_t *redeems,int32_t numredeems)
{
    long _emit_uint32(uint8_t *data,long offset,uint32_t x);
    uint8_t hashdata[256],revbuf[8];
    uint64_t redeemtxid;
    int32_t i,j,size;
    long offset;
    scriptstr[0] = 0;
    if ( numredeems >= (sizeof(hashdata)/sizeof(uint64_t))-1 )
    {
        printf("ram_make_OP_RETURN numredeems.%d is crazy\n",numredeems);
        return(-1);
    }
    hashdata[1] = OP_RETURN_OPCODE;
    hashdata[2] = 'M', hashdata[3] = 'G', hashdata[4] = 'W';
    hashdata[5] = numredeems;
    offset = 6;
    for (i=0; i<numredeems; i++)
    {
        redeemtxid = redeems[i];
        for (j=0; j<8; j++)
            revbuf[j] = ((uint8_t *)&redeemtxid)[7-j];
        memcpy(&redeemtxid,revbuf,sizeof(redeemtxid));
        offset = _emit_uint32(hashdata,offset,(uint32_t)redeemtxid);
        offset = _emit_uint32(hashdata,offset,(uint32_t)(redeemtxid >> 32));
    }
    hashdata[0] = size = (int32_t)(5 + sizeof(uint64_t)*numredeems);
    init_hexbytes_noT(scriptstr,hashdata+1,hashdata[0]);
    if ( size > 0xfc )
    {
        printf("ram_make_OP_RETURN numredeems.%d -> size.%d too big\n",numredeems,size);
        return(-1);
    }
    return(size);
}

long _emit_uint32(uint8_t *data,long offset,uint32_t x)
{
    uint32_t i;
    for (i=0; i<4; i++,x>>=8)
        data[offset + i] = (x & 0xff);
    offset += 4;
    return(offset);
}

long _decode_uint32(uint32_t *uintp,uint8_t *data,long offset,long len)
{
    uint32_t i,x = 0;
    for (i=0; i<4; i++)
        x <<= 8, x |= data[offset + 3 - i];
    offset += 4;
    *uintp = x;
    if ( offset > len )
        return(-1);
    return(offset);
}

long _decode_vin(struct cointx_input *vin,uint8_t *data,long offset,long len)
{
    uint8_t revdata[32];
    uint32_t i,vout;
    uint64_t siglen;
    memset(vin,0,sizeof(*vin));
    for (i=0; i<32; i++)
        revdata[i] = data[offset + 31 - i];
    init_hexbytes_noT(vin->tx.txidstr,revdata,32), offset += 32;
    if ( (offset= _decode_uint32(&vout,data,offset,len)) < 0 )
        return(-1);
    vin->tx.vout = vout;
    if ( (offset= hdecode_varint(&siglen,data,offset,len)) < 0 || siglen > sizeof(vin->sigs) )
        return(-1);
    init_hexbytes_noT(vin->sigs,&data[offset],(int32_t)siglen), offset += siglen;
    if ( (offset= _decode_uint32(&vin->sequence,data,offset,len)) < 0 )
        return(-1);
    //printf("txid.(%s) vout.%d siglen.%d (%s) seq.%d\n",vin->tx.txidstr,vout,(int)siglen,vin->sigs,vin->sequence);
    return(offset);
}

long _emit_cointx_input(uint8_t *data,long offset,struct cointx_input *vin)
{
    uint8_t revdata[32];
    long i,siglen;
    decode_hex(revdata,32,vin->tx.txidstr);
    for (i=0; i<32; i++)
        data[offset + 31 - i] = revdata[i];
    offset += 32;
    
    offset = _emit_uint32(data,offset,vin->tx.vout);
    siglen = strlen(vin->sigs) >> 1;
    offset += hcalc_varint(&data[offset],siglen);
    //printf("decodesiglen.%ld\n",siglen);
    if ( siglen != 0 )
        decode_hex(&data[offset],(int32_t)siglen,vin->sigs), offset += siglen;
    offset = _emit_uint32(data,offset,vin->sequence);
    return(offset);
}

long _emit_cointx_output(uint8_t *data,long offset,struct rawvout *vout)
{
    long scriptlen;
    offset = _emit_uint32(data,offset,(uint32_t)vout->value);
    offset = _emit_uint32(data,offset,(uint32_t)(vout->value >> 32));
    scriptlen = strlen(vout->script) >> 1;
    offset += hcalc_varint(&data[offset],scriptlen);
   // printf("scriptlen.%ld\n",scriptlen);
    if ( scriptlen != 0 )
        decode_hex(&data[offset],(int32_t)scriptlen,vout->script), offset += scriptlen;
    return(offset);
}

long _decode_vout(struct rawvout *vout,uint8_t *data,long offset,long len)
{
    uint32_t low,high;
    uint64_t scriptlen;
    memset(vout,0,sizeof(*vout));
    if ( (offset= _decode_uint32(&low,data,offset,len)) < 0 )
        return(-1);
    if ( (offset= _decode_uint32(&high,data,offset,len)) < 0 )
        return(-1);
    vout->value = ((uint64_t)high << 32) | low;
    if ( (offset= hdecode_varint(&scriptlen,data,offset,len)) < 0 || scriptlen > sizeof(vout->script) )
        return(-1);
    init_hexbytes_noT(vout->script,&data[offset],(int32_t)scriptlen), offset += scriptlen;
    return(offset);
}

void disp_cointx_output(struct rawvout *vout)
{
    printf("(%s %s %.8f) ",vout->coinaddr,vout->script,dstr(vout->value));
}

void disp_cointx_input(struct cointx_input *vin)
{
    printf("{ %s/v%d %s }.s%d ",vin->tx.txidstr,vin->tx.vout,vin->sigs,vin->sequence);
}

void disp_cointx(struct cointx_info *cointx)
{
    int32_t i;
    printf("version.%u timestamp.%u nlocktime.%u numinputs.%u numoutputs.%u\n",cointx->version,cointx->timestamp,cointx->nlocktime,cointx->numinputs,cointx->numoutputs);
    for (i=0; i<cointx->numinputs; i++)
        disp_cointx_input(&cointx->inputs[i]);
    printf("-> ");
    for (i=0; i<cointx->numoutputs; i++)
        disp_cointx_output(&cointx->outputs[i]);
    printf("\n");
}

int32_t _emit_cointx(char *hexstr,long len,struct cointx_info *cointx,int32_t oldtx)
{
    uint8_t *data;
    long offset = 0;
    int32_t i;
    data = calloc(1,len);
    offset = _emit_uint32(data,offset,cointx->version);
    if ( oldtx == 0 )
        offset = _emit_uint32(data,offset,cointx->timestamp);
    offset += hcalc_varint(&data[offset],cointx->numinputs);
    for (i=0; i<cointx->numinputs; i++)
        offset = _emit_cointx_input(data,offset,&cointx->inputs[i]);
    offset += hcalc_varint(&data[offset],cointx->numoutputs);
    for (i=0; i<cointx->numoutputs; i++)
        offset = _emit_cointx_output(data,offset,&cointx->outputs[i]);
    offset = _emit_uint32(data,offset,cointx->nlocktime);
    init_hexbytes_noT(hexstr,data,(int32_t)offset);
    free(data);
    return((int32_t)offset);
}

int32_t _validate_decoderawtransaction(char *hexstr,struct cointx_info *cointx,int32_t oldtx)
{
    char *checkstr;
    long len;
    int32_t retval;
    len = strlen(hexstr) * 2;
    //disp_cointx(cointx);
    checkstr = calloc(1,len + 1);
    _emit_cointx(checkstr,len,cointx,oldtx);
    if ( (retval= strcmp(checkstr,hexstr)) != 0 )
    {
        disp_cointx(cointx);
        printf("_validate_decoderawtransaction: error: \n(%s) != \n(%s)\n",hexstr,checkstr);
        //getchar();
    }
    //else printf("_validate_decoderawtransaction.(%s) validates\n",hexstr);
    free(checkstr);
    return(retval);
}

struct cointx_info *_decode_rawtransaction(char *hexstr,int32_t oldtx)
{
    uint8_t data[8192];
    long len,offset = 0;
    uint64_t numinputs,numoutputs;
    struct cointx_info *cointx;
    uint32_t vin,vout;
    if ( (len= strlen(hexstr)) >= sizeof(data)*2-1 || is_hexstr(hexstr) == 0 || (len & 1) != 0 )
    {
        printf("_decode_rawtransaction: hexstr too long %ld vs %ld || is_hexstr.%d || oddlen.%ld\n",strlen(hexstr),sizeof(data)*2-1,is_hexstr(hexstr),(len & 1));
        return(0);
    }
    //_decode_rawtransaction("0100000001a131c270d541c9d2be98b6f7a88c6cbea5d5a395ec82c9954083675226f399ee0300000000ffffffff042f7500000000000017a9140cc0def37d9682c292d18b3f579b7432adf4703187a0f70300000000001976a914e8bf7b6c41702de3451d189db054c985fe6fbbdb88ac01000000000000001976a914f9fab825f93c5f0ddcf90c4c96c371dc3dbca95788ac10eb09000000000017a914309924e8dad854d4cb8e3d6b839a932aea22590c8700000000"); getchar();
    len >>= 1;
    //printf("(%s).%ld\n",hexstr,len);
    decode_hex(data,(int32_t)len,hexstr);
    //for (int i=0; i<len; i++)
    //    printf("%02x ",data[i]);
    //printf("converted\n");
    cointx = calloc(1,sizeof(*cointx));
    offset = _decode_uint32(&cointx->version,data,offset,len);
    if ( oldtx == 0 )
        offset = _decode_uint32(&cointx->timestamp,data,offset,len);
    offset = hdecode_varint(&numinputs,data,offset,len);
    if ( numinputs > MAX_COINTX_INPUTS )
    {
        printf("_decode_rawtransaction: numinputs %lld > %d MAX_COINTX_INPUTS version.%d timestamp.%u %ld offset.%ld [%x]\n",(long long)numinputs,MAX_COINTX_INPUTS,cointx->version,cointx->timestamp,time(NULL),offset,*(int *)&data[offset]);
        return(0);
    }
    for (vin=0; vin<numinputs; vin++)
        if ( (offset= _decode_vin(&cointx->inputs[vin],data,offset,len)) < 0 )
            break;
    if ( vin != numinputs )
    {
        printf("_decode_rawtransaction: _decode_vin.%d err\n",vin);
        return(0);
    }
    offset = hdecode_varint(&numoutputs,data,offset,len);
    if ( numoutputs > MAX_COINTX_OUTPUTS )
    {
        printf("_decode_rawtransaction: numoutputs %lld > %d MAX_COINTX_INPUTS\n",(long long)numoutputs,MAX_COINTX_OUTPUTS);
        return(0);
    }
    for (vout=0; vout<numoutputs; vout++)
        if ( (offset= _decode_vout(&cointx->outputs[vout],data,offset,len)) < 0 )
            break;
    if ( vout != numoutputs )
    {
        printf("_decode_rawtransaction: _decode_vout.%d err\n",vout);
        return(0);
    }
    offset = _decode_uint32(&cointx->nlocktime,data,offset,len);
    cointx->numinputs = (uint32_t)numinputs;
    cointx->numoutputs = (uint32_t)numoutputs;
    if ( offset != len )
        printf("_decode_rawtransaction warning: offset.%ld vs len.%ld for (%s)\n",offset,len,hexstr);
    _validate_decoderawtransaction(hexstr,cointx,oldtx);
    return(cointx);
}

int32_t ram_is_MGW_OP_RETURN(uint64_t *redeemtxids,struct ramchain_info *ram,uint32_t script_rawind)
{
    static uint8_t zero12[12];
    void *ram_gethashdata(struct ramchain_info *ram,char type,uint32_t rawind);
    int32_t j,numredeems = 0;
    uint8_t *hashdata;
    uint64_t redeemtxid;
    if ( (hashdata= ram_gethashdata(ram,'s',script_rawind)) != 0 )
    {
        if ( ram->do_opreturn != 0 )
        {
            if ( (len= hashdata[0]) < 256 && hashdata[1] == OP_RETURN_OPCODE && hashdata[2] == 'M' && hashdata[3] == 'G' && hashdata[4] == 'W' )
            {
                numredeems = hashdata[5];
                if ( (numredeems*sizeof(uint64_t) + 5) == len )
                {
                    hashdata = &hashdata[6];
                    for (i=0; i<numredeems; i++)
                    {
                        for (redeemtxid=j=0; j<(int32_t)sizeof(uint64_t); j++)
                            redeemtxid <<= 8, redeemtxid |= (*hashdata++ & 0xff);
                        redeemtxids[i] = redeemtxid;
                    }
                    return(numredeems);
                } else printf("ram_is_MGW_OP_RETURN: numredeems.%d + 5 != %d len\n",numredeems,len);
            }
        }
        if ( hashdata[1] == 0x76 && hashdata[2] == 0xa9 && hashdata[3] == 0x14 && memcmp(&hashdata[12],zero12,12) == 0 )
        {
            hashdata = &hashdata[4];
            for (redeemtxid=j=0; j<(int32_t)sizeof(uint64_t); j++)
                redeemtxid <<= 8, redeemtxid |= (*hashdata++ & 0xff);
            redeemtxids[0] = redeemtxid;
            printf("FOUND HACKRETURN.(%llu)\n",(long long)redeemtxid);
            numredeems = 1;
        }
    }
    return(numredeems);
}

struct cointx_info *_calc_cointx_withdraw(struct ramchain_info *ram,char *destaddr,uint64_t value,uint64_t redeemtxid)
{
    //int64 nPayFee = nTransactionFee * (1 + (int64)nBytes / 1000);
    char *rawparams,*signedtx,*changeaddr,*with_op_return=0,*retstr = 0;
    int64_t MGWfee,sum,amount;
    int32_t allocsize,opreturn_output,numoutputs = 0;
    struct cointx_info *cointx,TX,*rettx = 0;
    cointx = &TX;
    memset(cointx,0,sizeof(*cointx));
    strcpy(cointx->coinstr,ram->name);
    cointx->redeemtxid = redeemtxid;
    cointx->gatewayid = ram->S.gatewayid;
    MGWfee = 0*(value >> 10) + (2 * (ram->txfee + ram->NXTfee_equiv)) - ram->minoutput - ram->txfee;
    if ( value <= MGWfee + ram->minoutput + ram->txfee )
    {
        printf("%s redeem.%llu withdraw %.8f < MGWfee %.8f + minoutput %.8f + txfee %.8f\n",ram->name,(long long)redeemtxid,dstr(value),dstr(MGWfee),dstr(ram->minoutput),dstr(ram->txfee));
        return(0);
    }
    strcpy(cointx->outputs[numoutputs].coinaddr,ram->marker2);
    if ( strcmp(destaddr,ram->marker2) == 0 )
        cointx->outputs[numoutputs++].value = value - ram->minoutput - ram->txfee;
    else
    {
        cointx->outputs[numoutputs++].value = MGWfee;
        strcpy(cointx->outputs[numoutputs].coinaddr,destaddr);
        cointx->outputs[numoutputs++].value = value - MGWfee - ram->minoutput - ram->txfee;
    }
    opreturn_output = numoutputs;
    strcpy(cointx->outputs[numoutputs].coinaddr,ram->opreturnmarker);
    cointx->outputs[numoutputs++].value = ram->minoutput;
    cointx->numoutputs = numoutputs;
    cointx->amount = amount = (MGWfee + value + ram->minoutput + ram->txfee);
    fprintf(stderr,"calc_withdraw.%s %llu amount %.8f -> balance %.8f\n",ram->name,(long long)redeemtxid,dstr(cointx->amount),dstr(ram->S.MGWbalance));
    if ( ram->S.MGWbalance >= 0 )
    {
        if ( (sum= _calc_cointx_inputs(ram,cointx,cointx->amount)) >= (cointx->amount + ram->txfee) )
        {
            if ( cointx->change != 0 )
            {
                changeaddr = (strcmp(ram->MGWsmallest,destaddr) != 0) ? ram->MGWsmallest : ram->MGWsmallestB;
                if ( changeaddr[0] == 0 )
                {
                    printf("Need to create more deposit addresses, need to have at least 2 available\n");
                    exit(1);
                }
                if ( strcmp(cointx->outputs[0].coinaddr,changeaddr) != 0 )
                {
                    strcpy(cointx->outputs[cointx->numoutputs].coinaddr,changeaddr);
                    cointx->outputs[cointx->numoutputs].value = cointx->change;
                    cointx->numoutputs++;
                } else cointx->outputs[0].value += cointx->change;
            }
            rawparams = _createrawtxid_json_params(ram,cointx);
            if ( rawparams != 0 )
            {
                //fprintf(stderr,"len.%ld rawparams.(%s)\n",strlen(rawparams),rawparams);
                _stripwhite(rawparams,0);
                if (  ram->S.gatewayid >= 0 )
                {
                    retstr = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"createrawtransaction",rawparams);
                    if (retstr != 0 && retstr[0] != 0 )
                    {
                        fprintf(stderr,"len.%ld calc_rawtransaction retstr.(%s)\n",strlen(retstr),retstr);
                        if ( (with_op_return= _insert_OP_RETURN(retstr,opreturn_output,&redeemtxid,1,ram->oldtx)) != 0 )
                        {
                            if ( (signedtx= _sign_localtx(ram,cointx,with_op_return)) != 0 )
                            {
                                allocsize = (int32_t)(sizeof(*rettx) + strlen(signedtx) + 1);
                                // printf("signedtx returns.(%s) allocsize.%d\n",signedtx,allocsize);
                                rettx = calloc(1,allocsize);
                                *rettx = *cointx;
                                rettx->allocsize = allocsize;
                                rettx->isallocated = allocsize;
                                strcpy(rettx->signedtx,signedtx);
                                free(signedtx);
                                cointx = 0;
                            } else printf("error _sign_localtx.(%s)\n",with_op_return);
                            free(with_op_return);
                        } else printf("error replacing with OP_RETURN\n");
                    } else fprintf(stderr,"error creating rawtransaction\n");
                }
                free(rawparams);
                if ( retstr != 0 )
                    free(retstr);
            } else fprintf(stderr,"error creating rawparams\n");
        } else fprintf(stderr,"error calculating rawinputs.%.8f or outputs.%.8f | txfee %.8f\n",dstr(sum),dstr(cointx->amount),dstr(ram->txfee));
    } else fprintf(stderr,"not enough %s balance %.8f for withdraw %.8f txfee %.8f\n",ram->name,dstr(ram->S.MGWbalance),dstr(cointx->amount),dstr(ram->txfee));
    return(rettx);
}

uint32_t _get_RTheight(struct ramchain_info *ram)
{
    char *retstr;
    cJSON *json;
    uint32_t height = 0;
    if ( ram_millis() > ram->lastgetinfo+10000 )
    {
        //printf("RTheight.(%s) (%s)\n",ram->name,ram->serverport);
        retstr = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"getinfo","");
        if ( retstr != 0 )
        {
            if ( (json= cJSON_Parse(retstr)) != 0 )
            {
                height = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"blocks"),0);
                free_json(json);
                ram->lastgetinfo = milliseconds();
            }
            free(retstr);
        }
    } else height = ram->S.RTblocknum;
    return(height);
}

// >>>>>>>>>>>>>> NXT functions
#define GET_COINDEPOSIT_ADDRESS 'g'
#define BIND_DEPOSIT_ADDRESS 'b'
#define DEPOSIT_CONFIRMED 'd'
#define MONEY_SENT 'm'
extern char NXTAPIURL[MAX_JSON_FIELD],NXTSERVER[];
#define _issue_NXTPOST(cmdstr) bitcoind_RPC(0,"curl",NXTAPIURL,0,0,cmdstr)
#define _issue_curl(cmdstr) bitcoind_RPC(0,"curl",cmdstr,0,0,0)
struct NXT_assettxid *find_NXT_assettxid(int32_t *createdflagp,struct NXT_asset *ap,char *txid);

int32_t _expand_nxt64bits(char *NXTaddr,uint64_t nxt64bits)
{
    int32_t i,n;
    uint64_t modval;
    char rev[64];
    for (i=0; nxt64bits!=0; i++)
    {
        modval = nxt64bits % 10;
        rev[i] = (char)(modval + '0');
        nxt64bits /= 10;
    }
    n = i;
    for (i=0; i<n; i++)
        NXTaddr[i] = rev[n-1-i];
    NXTaddr[i] = 0;
    return(n);
}

uint64_t _calc_nxt64bits(const char *NXTaddr)
{
    int32_t c;
    int64_t n,i;
    uint64_t lastval,mult,nxt64bits = 0;
    if ( NXTaddr == 0 )
    {
        printf("calling calc_nxt64bits with null ptr!\n");
#ifdef __APPLE__
        while ( 1 ) portable_sleep(1);
#endif
        return(0);
    }
    n = strlen(NXTaddr);
    if ( n >= 22 )
    {
        printf("calc_nxt64bits: illegal NXTaddr.(%s) too long\n",NXTaddr);
        return(0);
    }
    else if ( strcmp(NXTaddr,"0") == 0 || strcmp(NXTaddr,"false") == 0 )
        return(0);
    mult = 1;
    lastval = 0;
    for (i=n-1; i>=0; i--,mult*=10)
    {
        c = NXTaddr[i];
        if ( c < '0' || c > '9' )
        {
            printf("calc_nxt64bits: illegal char.(%c %d) in (%s).%d\n",c,c,NXTaddr,(int32_t)i);
#ifdef __APPLE__
            while ( 1 )
            {
                portable_sleep(60);
                printf("calc_nxt64bits: illegal char.(%c %d) in (%s).%d\n",c,c,NXTaddr,(int32_t)i);
            }
#endif
            return(0);
        }
        nxt64bits += mult * (c - '0');
        if ( nxt64bits < lastval )
            printf("calc_nxt64bits: warning: 64bit overflow %llx < %llx\n",(long long)nxt64bits,(long long)lastval);
        lastval = nxt64bits;
    }
    return(nxt64bits);
}

int32_t _in_specialNXTaddrs(char **specialNXTaddrs,int32_t n,char *NXTaddr)
{
    int32_t i;
    for (i=0; i<n; i++)
        if ( strcmp(specialNXTaddrs[i],NXTaddr) == 0 )
            return(1);
    return(0);
}

char *_issue_getAsset(char *assetidstr)
{
    char cmd[4096];
    //sprintf(cmd,"requestType=getAsset&asset=%s",assetidstr);
    sprintf(cmd,"%s=getAsset&asset=%s",NXTSERVER,assetidstr);
    printf("_cmd.(%s)\n",cmd);
    return(_issue_curl(cmd));
}

uint32_t _get_NXTheight(uint32_t *firsttimep)
{
    cJSON *json;
    uint32_t height = 0;
    char cmd[256],*jsonstr;
    sprintf(cmd,"requestType=getState");
    if ( (jsonstr= _issue_NXTPOST(cmd)) != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( firsttimep != 0 )
                *firsttimep = (uint32_t)get_cJSON_int(json,"time");
            height = (int32_t)get_cJSON_int(json,"numberOfBlocks");
            if ( height > 0 )
                height--;
            free_json(json);
        }
        free(jsonstr);
    }
    return(height);
}

uint64_t _get_NXT_ECblock(uint32_t *ecblockp)
{
    cJSON *json;
    uint64_t ecblock = 0;
    char cmd[256],*jsonstr;
    sprintf(cmd,"requestType=getECBlock");
    if ( (jsonstr= _issue_NXTPOST(cmd)) != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( ecblockp != 0 )
                *ecblockp = (uint32_t)get_cJSON_int(json,"ecBlockHeight");
            ecblock = get_API_nxt64bits(cJSON_GetObjectItem(json,"ecBlockId"));
            free_json(json);
        }
        free(jsonstr);
    }
    return(ecblock);
}

uint64_t _calc_circulation(int32_t minconfirms,struct NXT_asset *ap,struct ramchain_info *ram)
{
    uint64_t quantity,circulation = 0;
    char cmd[4096],acct[MAX_JSON_FIELD],*retstr = 0;
    cJSON *json,*array,*item;
    uint32_t i,n,height = 0;
    if ( ap == 0 )
        return(0);
    if ( minconfirms != 0 )
    {
        ram->S.NXT_RTblocknum = _get_NXTheight(0);
        height = ram->S.NXT_RTblocknum - minconfirms;
    }
    sprintf(cmd,"requestType=getAssetAccounts&asset=%llu",(long long)ap->assetbits);
    if ( height > 0 )
        sprintf(cmd+strlen(cmd),"&height=%u",height);
    if ( (retstr= _issue_NXTPOST(cmd)) != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"accountAssets")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    item = cJSON_GetArrayItem(array,i);
                    copy_cJSON(acct,cJSON_GetObjectItem(item,"account"));
                    if ( acct[0] == 0 )
                        continue;
                    if ( _in_specialNXTaddrs(ram->special_NXTaddrs,ram->numspecials,acct) == 0 && (quantity= get_API_nxt64bits(cJSON_GetObjectItem(item,"quantityQNT"))) != 0 )
                    {
                        //if ( quantity > 2000 )
                        //    printf("Whale %s: %.8f\n",acct,dstr(quantity*ap->mult));
                        circulation += quantity;
                    }
                }
            }
            free_json(json);
        }
        free(retstr);
    }
    return(circulation * ap->mult);
}

char *_issue_getTransaction(char *txidstr)
{
    char cmd[4096];
    sprintf(cmd,"requestType=getTransaction&transaction=%s",txidstr);
    return(_issue_NXTPOST(cmd));
}

uint64_t ram_verify_NXTtxstillthere(struct ramchain_info *ram,uint64_t txidbits)
{
    char txidstr[64],*retstr;
    cJSON *json,*attach;
    uint64_t quantity = 0;
    _expand_nxt64bits(txidstr,txidbits);
    if ( (retstr= _issue_getTransaction(txidstr)) != 0 )
    {
        //printf("verify.(%s)\n",retstr);
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            if ( (attach= cJSON_GetObjectItem(json,"attachment")) != 0 )
                quantity = get_API_nxt64bits(cJSON_GetObjectItem(attach,"quantityQNT"));
            free_json(json);
        }
        free(retstr);
    }
    //fprintf(stderr,"return %.8f\n",dstr(quantity * ram->ap->mult));
    return(quantity * ram->ap->mult);
}

char *_unstringify(char *str)
{
    int32_t i,j,n;
    if ( str == 0 )
        return(0);
    else if ( str[0] == 0 )
        return(str);
    n = (int32_t)strlen(str);
    if ( str[0] == '"' && str[n-1] == '"' )
        str[n-1] = 0, i = 1;
    else i = 0;
    for (j=0; str[i]!=0; i++)
    {
        if ( str[i] == '\\' && (str[i+1] == 't' || str[i+1] == 'n' || str[i+1] == 'b' || str[i+1] == 'r') )
            i++;
        else if ( str[i] == '\\' && str[i+1] == '"' )
            str[j++] = '"', i++;
        else str[j++] = str[i];
    }
    str[j] = 0;
    return(str);
}

int32_t _is_limbo_redeem(struct ramchain_info *ram,uint64_t redeemtxidbits)
{
    int32_t j;
    if ( ram->limboarray != 0 )
    {
        for (j=0; ram->limboarray[j]!=0; j++)
            if ( redeemtxidbits == ram->limboarray[j] )
            {
                printf("redeemtxid.%llu in slot.%d\n",(long long)redeemtxidbits,j);
                return(1);
            }
    } else printf("no limboarray for %s?\n",ram->name);
    return(0);
}

void _complete_assettxid(struct ramchain_info *ram,struct NXT_assettxid *tp)
{
    if ( tp->completed == 0 && tp->redeemstarted != 0 )
    {
        printf("pending redeem %llu %.8f completed | elapsed %d seconds\n",(long long)tp->redeemtxid,dstr(tp->U.assetoshis),(uint32_t)(time(NULL) - tp->redeemstarted));
        tp->redeemstarted = (uint32_t)(time(NULL) - tp->redeemstarted);
    }
    tp->completed = 1;
}

char *_calc_withdrawaddr(char *withdrawaddr,struct ramchain_info *ram,struct NXT_assettxid *tp,cJSON *argjson)
{
    cJSON *json;
    int32_t i,c,convert = 0;
    struct ramchain_info *newram;
    char buf[MAX_JSON_FIELD],autoconvert[MAX_JSON_FIELD],issuer[MAX_JSON_FIELD],*retstr;
    copy_cJSON(withdrawaddr,cJSON_GetObjectItem(argjson,"withdrawaddr"));
    //if ( withdrawaddr[0] != 0 )
    //    return(withdrawaddr);
    //else return(0);
    if ( tp->convname[0] != 0 )
    {
        withdrawaddr[0] = 0;
        return(0);
    }
    copy_cJSON(autoconvert,cJSON_GetObjectItem(argjson,"autoconvert"));
    copy_cJSON(buf,cJSON_GetObjectItem(argjson,"teleport")); // "send" or <emailaddr>
    safecopy(tp->teleport,buf,sizeof(tp->teleport));
    tp->convassetid = tp->assetbits;
    if ( autoconvert[0] != 0 )
    {
        if ( (newram= get_ramchain_info(autoconvert)) == 0 )
        {
            if ( (retstr= _issue_getAsset(autoconvert)) != 0 )
            {
                if ( (json= cJSON_Parse(retstr)) != 0 )
                {
                    copy_cJSON(issuer,cJSON_GetObjectItem(json,"account"));
                    if ( is_trusted_issuer(issuer) > 0 )
                    {
                        copy_cJSON(tp->convname,cJSON_GetObjectItem(json,"name"));
                        convert = 1;
                    }
                    free_json(json);
                }
            }
        }
        else
        {
            strcpy(tp->convname,newram->name);
            convert = 1;
        }
        if ( convert != 0 )
        {
            tp->minconvrate = get_API_float(cJSON_GetObjectItem(argjson,"rate"));
            tp->convexpiration = (int32_t)get_API_int(cJSON_GetObjectItem(argjson,"expiration"),0);
            if ( withdrawaddr[0] != 0 ) // no address means to create user credit
            {
                _stripwhite(withdrawaddr,0);
                tp->convwithdrawaddr = clonestr(withdrawaddr);
            }
        }
        else withdrawaddr[0] = autoconvert[0] = 0;
    }
    printf("PARSED.%s withdrawaddr.(%s) autoconvert.(%s)\n",ram->name,withdrawaddr,autoconvert);
    if ( withdrawaddr[0] == 0 || autoconvert[0] != 0 )
        return(0);
    for (i=0; withdrawaddr[i]!=0; i++)
        if ( (c= withdrawaddr[i]) < ' ' || c == '\\' || c == '"' )
            return(0);
    //printf("return.(%s)\n",withdrawaddr);
    return(withdrawaddr);
}

char *_parse_withdraw_instructions(char *destaddr,char *NXTaddr,struct ramchain_info *ram,struct NXT_assettxid *tp,struct NXT_asset *ap)
{
    char pubkey[1024],withdrawaddr[1024],*retstr = destaddr;
    int64_t amount,minwithdraw;
    cJSON *argjson = 0;
    destaddr[0] = withdrawaddr[0] = 0;
    if ( tp->redeemtxid == 0 )
    {
        printf("no redeem txid %s %s\n",ram->name,cJSON_Print(argjson));
        retstr = 0;
    }
    else
    {
        amount = tp->quantity * ap->mult;
        if ( tp->comment != 0 && (argjson= cJSON_Parse(tp->comment)) != 0 ) //(tp->comment[0] == '{' || tp->comment[0] == '[') &&
        {
            if ( _calc_withdrawaddr(withdrawaddr,ram,tp,argjson) == 0 )
            {
                printf("(%llu) no withdraw.(%s) or autoconvert.(%s)\n",(long long)tp->redeemtxid,withdrawaddr,tp->comment);
                _complete_assettxid(ram,tp);
                retstr = 0;
            }
        }
        if ( retstr != 0 )
        {
            minwithdraw = ram->txfee * MIN_DEPOSIT_FACTOR;
            if ( amount <= minwithdraw )
            {
                printf("%llu: minimum withdrawal must be more than %.8f %s\n",(long long)tp->redeemtxid,dstr(minwithdraw),ram->name);
                _complete_assettxid(ram,tp);
                retstr = 0;
            }
            else if ( withdrawaddr[0] == 0 )
            {
                printf("%llu: no withdraw address for %.8f | ",(long long)tp->redeemtxid,dstr(amount));
                _complete_assettxid(ram,tp);
                retstr = 0;
            }
            else if ( ram != 0 && _validate_coinaddr(pubkey,ram,withdrawaddr) < 0 )
            {
                printf("%llu: invalid address.(%s) for NXT.%s %.8f validate.%d\n",(long long)tp->redeemtxid,withdrawaddr,NXTaddr,dstr(amount),_validate_coinaddr(pubkey,ram,withdrawaddr));
                _complete_assettxid(ram,tp);
                retstr = 0;
            }
        }
    }
    //printf("withdraw addr.(%s) for (%s)\n",withdrawaddr,NXTaddr);
    if ( retstr != 0 )
        strcpy(retstr,withdrawaddr);
    if ( argjson != 0 )
        free_json(argjson);
    return(retstr);
}

struct MGWstate *ram_select_MGWstate(struct ramchain_info *ram,int32_t selector)
{
    struct MGWstate *sp = 0;
    if ( ram != 0 )
    {
        if ( selector < 0 )
            sp = &ram->S;
        else if ( selector < ram->numgateways )
            sp = &ram->otherS[selector];
    }
    return(sp);
}

void ram_set_MGWpingstr(char *pingstr,struct ramchain_info *ram,int32_t selector)
{
    struct MGWstate *sp = ram_select_MGWstate(ram,selector);
    if ( ram != 0 && ram->S.gatewayid >= 0 )
    {
        ram->S.supply = (ram->S.totaloutputs - ram->S.totalspends);
        ram->otherS[ram->S.gatewayid] = ram->S;
    }
    if ( sp != 0 )
    {
        sprintf(pingstr,"\"coin\":\"%s\",\"gatewayid\":\"%d\",\"balance\":\"%llu\",\"sentNXT\":\"%llu\",\"unspent\":\"%llu\",\"supply\":\"%llu\",\"circulation\":\"%llu\",\"pendingredeems\":\"%llu\",\"pendingdeposits\":\"%llu\",\"internal\":\"%llu\",\"RTNXT\":{\"height\":\"%d\",\"lag\":\"%d\",\"ECblock\":\"%llu\",\"ECheight\":\"%u\"},\"%s\":{\"permblocks\":\"%d\",\"height\":\"%d\",\"lag\":\"%d\"},",ram->name,sp->gatewayid,(long long)(sp->MGWbalance),(long long)(sp->sentNXT),(long long)(sp->MGWunspent),(long long)(sp->supply),(long long)(sp->circulation),(long long)(sp->MGWpendingredeems),(long long)(sp->MGWpendingdeposits),(long long)(sp->orphans),sp->NXT_RTblocknum,sp->NXT_RTblocknum-sp->NXTblocknum,(long long)sp->NXT_ECblock,sp->NXT_ECheight,sp->name,sp->permblocks,sp->RTblocknum,sp->RTblocknum - sp->blocknum);
    }
 }

void ram_set_MGWdispbuf(char *dispbuf,struct ramchain_info *ram,int32_t selector)
{
    struct MGWstate *sp = ram_select_MGWstate(ram,selector);
    if ( sp != 0 )
    {
        sprintf(dispbuf,"[%.8f %s - %.0f NXT rate %.2f] msigs.%d unspent %.8f circ %.8f/%.8f pend.(W%.8f D%.8f) NXT.%d %s.%d\n",dstr(sp->MGWbalance),ram->name,dstr(sp->sentNXT),sp->MGWbalance<=0?0:dstr(sp->sentNXT)/dstr(sp->MGWbalance),ram->nummsigs,dstr(sp->MGWunspent),dstr(sp->circulation),dstr(sp->supply),dstr(sp->MGWpendingredeems),dstr(sp->MGWpendingdeposits),sp->NXT_RTblocknum,ram->name,sp->RTblocknum);
    }
}

void ram_get_MGWpingstr(struct ramchain_info *ram,char *MGWpingstr,int32_t selector)
{
    MGWpingstr[0] = 0;
   // printf("get MGWpingstr\n");
    if ( Numramchains != 0 )
    {
        if ( ram == 0 )
            ram = Ramchains[rand() % Numramchains];
        if ( ram != 0 )
            ram_set_MGWpingstr(MGWpingstr,ram,selector);
    }
}

void ram_parse_MGWstate(struct MGWstate *sp,cJSON *json,char *coinstr,char *NXTaddr)
{
    cJSON *nxtobj,*coinobj;
    if ( sp == 0 || json == 0 || coinstr == 0 || coinstr[0] == 0 )
        return;
    memset(sp,0,sizeof(*sp));
    sp->nxt64bits = _calc_nxt64bits(NXTaddr);
    sp->MGWbalance = get_API_nxt64bits(cJSON_GetObjectItem(json,"balance"));
    sp->sentNXT = get_API_nxt64bits(cJSON_GetObjectItem(json,"sentNXT"));
    sp->MGWunspent = get_API_nxt64bits(cJSON_GetObjectItem(json,"unspent"));
    sp->circulation = get_API_nxt64bits(cJSON_GetObjectItem(json,"circulation"));
    sp->MGWpendingredeems = get_API_nxt64bits(cJSON_GetObjectItem(json,"pendingredeems"));
    sp->MGWpendingdeposits = get_API_nxt64bits(cJSON_GetObjectItem(json,"pendingdeposits"));
    sp->supply = get_API_nxt64bits(cJSON_GetObjectItem(json,"supply"));
    sp->orphans = get_API_nxt64bits(cJSON_GetObjectItem(json,"internal"));
    if ( (nxtobj= cJSON_GetObjectItem(json,"RTNXT")) != 0 )
    {
        sp->NXT_RTblocknum = (uint32_t)get_API_int(cJSON_GetObjectItem(nxtobj,"height"),0);
        sp->NXTblocknum = (sp->NXT_RTblocknum - (uint32_t)get_API_int(cJSON_GetObjectItem(nxtobj,"lag"),0));
        sp->NXT_ECblock = get_API_nxt64bits(cJSON_GetObjectItem(nxtobj,"ECblock"));
        sp->NXT_ECheight = (uint32_t)get_API_int(cJSON_GetObjectItem(nxtobj,"ECheight"),0);
    }
    if ( (coinobj= cJSON_GetObjectItem(json,coinstr)) != 0 )
    {
        sp->permblocks = (uint32_t)get_API_int(cJSON_GetObjectItem(coinobj,"permblocks"),0);
        sp->RTblocknum = (uint32_t)get_API_int(cJSON_GetObjectItem(coinobj,"height"),0);
        sp->blocknum = (sp->NXT_RTblocknum - (uint32_t)get_API_int(cJSON_GetObjectItem(coinobj,"lag"),0));
    }
}

void ram_update_remotesrc(struct ramchain_info *ram,struct MGWstate *sp)
{
    int32_t i,oldi = -1,oldest = -1;
    //printf("update remote %llu\n",(long long)sp->nxt64bits);
    if ( sp->nxt64bits == 0 )
        return;
    for (i=0; i<(int32_t)(sizeof(ram->remotesrcs)/sizeof(*ram->remotesrcs)); i++)
    {
        if ( ram->remotesrcs[i].nxt64bits == 0 || sp->nxt64bits == ram->remotesrcs[i].nxt64bits )
        {
            ram->remotesrcs[i] = *sp;
            //printf("set slot.%d <- permblocks.%u\n",i,sp->permblocks);
            return;
        }
        if ( oldest < 0 || (ram->remotesrcs[i].permblocks != 0 && ram->remotesrcs[i].permblocks < oldest) )
            ram->remotesrcs[i].permblocks = oldest,oldi = i;
    }
    if ( oldi >= 0 && (sp->permblocks != 0 && sp->permblocks > oldest) )
    {
        //printf("overwrite slot.%d <- permblocks.%u\n",oldi,sp->permblocks);
        ram->remotesrcs[oldi] = *sp;
    }
}

void ram_parse_MGWpingstr(struct ramchain_info *ram,char *sender,char *pingstr)
{
    extern int32_t NORAMCHAINS;
    void save_MGW_status(char *NXTaddr,char *jsonstr);
    char name[512],coinstr[MAX_JSON_FIELD],*jsonstr = 0;
    struct MGWstate S;
    int32_t gatewayid;
    cJSON *json,*array;
    if ( Finished_init == 0 )
        return;
    if ( (array= cJSON_Parse(pingstr)) != 0 && is_cJSON_Array(array) != 0 )
    {
        json = cJSON_GetArrayItem(array,0);
        if ( ram == 0 )
        {
            copy_cJSON(coinstr,cJSON_GetObjectItem(json,"coin"));
            if ( coinstr[0] != 0 )
                ram = get_ramchain_info(coinstr);
        }
        if ( Debuglevel > 2 || (ram != 0 && ram->remotemode != 0) )
            printf("[%s] parse.(%s)\n",coinstr,pingstr);
        if ( ram != 0 )
        {
            cJSON_DeleteItemFromObject(json,"ipaddr");
            if ( (gatewayid= (int32_t)get_API_int(cJSON_GetObjectItem(json,"gatewayid"),-1)) >= 0 && gatewayid < ram->numgateways )
            {
                if ( strcmp(ram->special_NXTaddrs[gatewayid],sender) == 0 )
                    ram_parse_MGWstate(&ram->otherS[gatewayid],json,ram->name,sender);
                else printf("ram_parse_MGWpingstr: got wrong address.(%s) for gatewayid.%d expected.(%s)\n",sender,gatewayid,ram->special_NXTaddrs[gatewayid]);
            }
            else
            {
                //printf("call parse.(%s)\n",cJSON_Print(json));
                ram_parse_MGWstate(&S,json,ram->name,sender);
                ram_update_remotesrc(ram,&S);
            }
            jsonstr = cJSON_Print(json);
            if ( gatewayid >= 0 && gatewayid < 3 && strcmp(ram->mgwstrs[gatewayid],jsonstr) != 0 )
            {
                safecopy(ram->mgwstrs[gatewayid],jsonstr,sizeof(ram->mgwstrs[gatewayid]));
                //sprintf(name,"%s.%s",ram->name,Server_ipaddrs[gatewayid]);
                sprintf(name,"%s.%d",ram->name,gatewayid);
                //printf("name is (%s) + (%s) -> (%s)\n",ram->name,Server_ipaddrs[gatewayid],name);
                save_MGW_status(name,jsonstr);
            }
        } else if ( Debuglevel > 1 && NORAMCHAINS == 0 && coinstr[0] != 0 ) printf("dont have ramchain_info for (%s) (%s)\n",coinstr,pingstr);
        if ( jsonstr != 0 )
            free(jsonstr);
        free_json(array);
    }
    //printf("parsed\n");
}

int32_t MGWstatecmp(struct MGWstate *spA,struct MGWstate *spB)
{
    return(memcmp((void *)((long)spA + sizeof(spA->gatewayid)),(void *)((long)spB + sizeof(spB->gatewayid)),sizeof(*spA) - sizeof(spA->gatewayid)));
}

double _enough_confirms(double redeemed,double estNXT,int32_t numconfs,int32_t minconfirms)
{
    double metric;
    if ( numconfs < minconfirms )
        return(0);
    metric = log(estNXT + sqrt(redeemed));
    return(metric - 1.);
}

int32_t ram_MGW_ready(struct ramchain_info *ram,uint32_t blocknum,uint32_t NXTheight,uint64_t nxt64bits,uint64_t amount)
{
    int32_t retval = 0;
    if ( ram->S.gatewayid >= 0 && ram->S.gatewayid < 3 && strcmp(ram->srvNXTADDR,ram->special_NXTaddrs[ram->S.gatewayid]) != 0 )
    {
        if ( Debuglevel > 2 )
            printf("mismatched gatewayid.%d\n",ram->S.gatewayid);
        return(0);
    }
    if ( ram->S.gatewayid < 0 || (nxt64bits != 0 && (nxt64bits % NUM_GATEWAYS) != ram->S.gatewayid) || ram->S.MGWbalance < 0 )
        return(0);
    else if ( blocknum != 0 && ram->S.NXT_is_realtime != 0 && (blocknum + ram->depositconfirms) <= ram->S.RTblocknum && ram->S.enable_deposits != 0 )
        retval = 1;
    else if ( NXTheight != 0 && ram->S.is_realtime != 0 && ram->S.enable_withdraws != 0 && _enough_confirms(0.,amount * ram->NXTconvrate,ram->S.NXT_RTblocknum - NXTheight,ram->withdrawconfirms) > 0. )
            retval = 1;
    if ( retval != 0 )
    {
        if ( MGWstatecmp(&ram->otherS[0],&ram->otherS[1]) != 0 || MGWstatecmp(&ram->otherS[0],&ram->otherS[2]) != 0 )
        {
            printf("MGWstatecmp failure %d, %d\n",MGWstatecmp(&ram->otherS[0],&ram->otherS[1]),MGWstatecmp(&ram->otherS[0],&ram->otherS[2]));
            //return(0);
        }
    }
    return(retval);
}

struct NXT_assettxid *_process_realtime_MGW(int32_t *sendip,struct ramchain_info **ramp,struct cointx_info *cointx,char *sender,char *recvname)
{
    uint32_t crc;
    int32_t gatewayid;
    char redeemtxidstr[64];
    struct ramchain_info *ram;
    *ramp = 0;
    *sendip = -1;
    if ( (ram= get_ramchain_info(cointx->coinstr)) == 0 )
        printf("cant find coin.(%s)\n",cointx->coinstr);
    else
    {
        *ramp = ram;
        if ( strncmp(recvname,ram->name,strlen(ram->name)) != 0 ) // + archive/RTmgw/
        {
            printf("_process_realtime_MGW: coin mismatch recvname.(%s) vs (%s).%ld\n",recvname,ram->name,strlen(ram->name));
            return(0);
        }
        crc = _crc32(0,(uint8_t *)((long)cointx+sizeof(cointx->crc)),(int32_t)(cointx->allocsize-sizeof(cointx->crc)));
        if ( crc != cointx->crc )
        {
            printf("_process_realtime_MGW: crc mismatch %x vs %x\n",crc,cointx->crc);
            return(0);
        }
        _expand_nxt64bits(redeemtxidstr,cointx->redeemtxid);
        if ( strncmp(recvname+strlen(ram->name)+1,redeemtxidstr,strlen(redeemtxidstr)) != 0 )
        {
            printf("_process_realtime_MGW: redeemtxid mismatch (%s) vs (%s)\n",recvname+strlen(ram->name)+1,redeemtxidstr);
            return(0);
        }
        gatewayid = cointx->gatewayid;
        if ( gatewayid < 0 || gatewayid >= ram->numgateways )
        {
            printf("_process_realtime_MGW: illegal gatewayid.%d\n",gatewayid);
            return(0);
        }
        if ( strcmp(ram->special_NXTaddrs[gatewayid],sender) != 0 )
        {
            printf("_process_realtime_MGW: gatewayid mismatch %d.(%s) vs %s\n",gatewayid,ram->special_NXTaddrs[gatewayid],sender);
            return(0);
        }
        //ram_add_pendingsend(0,ram,0,cointx);
        printf("GOT <<<<<<<<<<<< _process_realtime_MGW.%d coin.(%s) %.8f crc %08x redeemtxid.%llu\n",gatewayid,cointx->coinstr,dstr(cointx->amount),cointx->batchcrc,(long long)cointx->redeemtxid);
    }
    return(0);
}

int32_t cointxcmp(struct cointx_info *txA,struct cointx_info *txB)
{
    if ( txA != 0 && txB != 0 )
    {
        if ( txA->batchcrc == txB->batchcrc )
            return(0);
    }
    return(-1);
}

void _set_RTmgwname(char *RTmgwname,char *name,char *coinstr,int32_t gatewayid,uint64_t redeemtxid)
{
    void set_handler_fname(char *fname,char *handler,char *name);
    sprintf(name,"%s.%llu.g%d",coinstr,(long long)redeemtxid,gatewayid);
    set_handler_fname(RTmgwname,"RTmgw",name);
}

char *ram_check_consensus(char *txidstr,struct ramchain_info *ram,struct NXT_assettxid *tp)
{
    void *loadfile(int32_t *allocsizep,char *fname);
    uint64_t retval;
    char RTmgwname[1024],name[512],cmd[1024],hopNXTaddr[64],*cointxid,*retstr = 0;
    int32_t i,gatewayid,allocsize;
    struct cointx_info *cointxs[16],*othercointx;
    memset(cointxs,0,sizeof(cointxs));
    for (gatewayid=0; gatewayid<ram->numgateways; gatewayid++)
    {
        _set_RTmgwname(RTmgwname,name,ram->name,gatewayid,tp->redeemtxid);
        if ( (cointxs[gatewayid]= loadfile(&allocsize,RTmgwname)) == 0 )
        {
            char *send_tokenized_cmd(int32_t queueflag,char *hopNXTaddr,int32_t L,char *verifiedNXTaddr,char *NXTACCTSECRET,char *cmdstr,char *destNXTaddr);
            hopNXTaddr[0] = 0;
            sprintf(cmd,"{\"requestType\":\"getfile\",\"NXT\":\"%s\",\"timestamp\":\"%ld\",\"name\":\"%s\",\"handler\":\"RTmgw\"}",ram->srvNXTADDR,(long)time(NULL),name);
            if ( (retstr= send_tokenized_cmd(0,hopNXTaddr,0,ram->srvNXTADDR,ram->srvNXTACCTSECRET,cmd,ram->special_NXTaddrs[gatewayid])) != 0 )
                free(retstr), retstr = 0;
            printf("cant find.(%s) for %llu %.8f | sent.(%s) to %s\n",RTmgwname,(long long)tp->redeemtxid,dstr(tp->U.assetoshis),cmd,ram->special_NXTaddrs[gatewayid]);
            break;
        }
        for (i=0; i<gatewayid; i++)
            if ( cointxcmp(cointxs[i],cointxs[gatewayid]) != 0 )
            {
                printf("MGW%d %x != %x MGW%d for redeem.%llu %.8f\n",i,cointxs[i]->batchcrc,cointxs[gatewayid]->batchcrc,gatewayid,(long long)tp->redeemtxid,dstr(tp->U.assetoshis));
                break;
            }
    }
    if ( gatewayid != ram->numgateways )
    {
        for (i=0; i<=gatewayid; i++)
            free(cointxs[i]);
        return(0);
    }
    printf("got consensus for %llu %.8f\n",(long long)tp->redeemtxid,dstr(tp->U.assetoshis));
    if ( ram_MGW_ready(ram,0,tp->height,tp->senderbits,tp->U.assetoshis) > 0 )
    {
        if ( (retval= ram_verify_NXTtxstillthere(ram,tp->redeemtxid)) != tp->U.assetoshis )
        {
            fprintf(stderr,"ram_check_consensus tx gone due to a fork. NXT.%llu txid.%llu %.8f vs retval %.8f\n",(long long)tp->senderbits,(long long)tp->redeemtxid,dstr(tp->U.assetoshis),dstr(retval));
            exit(1); // seems the best thing to do
        }
        othercointx = cointxs[(ram->S.gatewayid ^ 1) % ram->numgateways];
        //printf("[%d] othercointx = %p\n",(ram->S.gatewayid ^ 1) % ram->numgateways,othercointx);
        if ( (cointxid= _sign_and_sendmoney(txidstr,ram,cointxs[ram->S.gatewayid],othercointx->signedtx,&tp->redeemtxid,&tp->U.assetoshis,1)) != 0 )
        {
            _complete_assettxid(ram,tp);
            //ram_add_pendingsend(&sendi,ram,tp,0);
            printf("completed redeem.%llu for %.8f cointxidstr.%s\n",(long long)tp->redeemtxid,dstr(tp->U.assetoshis),txidstr);
            retstr = txidstr;
        }
        else printf("ram_check_consensus error _sign_and_sendmoney for NXT.%llu redeem.%llu %.8f (%s)\n",(long long)tp->senderbits,(long long)tp->redeemtxid,dstr(tp->U.assetoshis),othercointx->signedtx);
    }
    for (gatewayid=0; gatewayid<ram->numgateways; gatewayid++)
        free(cointxs[gatewayid]);
    return(retstr);
}

void _RTmgw_handler(struct transfer_args *args)
{
    struct NXT_assettxid *tp;
    struct ramchain_info *ram;
    int32_t sendi;
    //char txidstr[512];
    printf("_RTmgw_handler(%s %d bytes)\n",args->name,args->totallen);
    if ( (tp= _process_realtime_MGW(&sendi,&ram,(struct cointx_info *)args->data,args->sender,args->name)) != 0 )
    {
        if ( sendi >= ram->numpendingsends || sendi < 0 || ram->pendingsends[sendi] != tp )
        {
            printf("FATAL: _RTmgw_handler sendi %d >= %d ram->numpendingsends || sendi %d < 0 || %p ram->pendingsends[sendi] != %ptp\n",sendi,ram->numpendingsends,sendi,ram->pendingsends[sendi],tp);
            exit(1);
        }
        //ram_check_consensus(txidstr,ram,tp);
    }
    //getchar();
}

void ram_send_cointx(struct ramchain_info *ram,struct cointx_info *cointx)
{
    char *start_transfer(char *previpaddr,char *sender,char *verifiedNXTaddr,char *NXTACCTSECRET,char *dest,char *name,uint8_t *data,int32_t totallen,int32_t timeout,char *handler,int32_t syncmem);
    char RTmgwname[512],name[512],*retstr;
    int32_t gatewayid;
    FILE *fp;
    _set_RTmgwname(RTmgwname,name,cointx->coinstr,cointx->gatewayid,cointx->redeemtxid);
    cointx->crc = _crc32(0,(uint8_t *)((long)cointx+sizeof(cointx->crc)),(int32_t)(cointx->allocsize - sizeof(cointx->crc)));
    if ( (fp= fopen(os_compatible_path(RTmgwname),"wb")) != 0 )
    {
        printf("save to (%s).%d crc.%x | batchcrc %x\n",RTmgwname,cointx->allocsize,cointx->crc,cointx->batchcrc);
        fwrite(cointx,1,cointx->allocsize,fp);
        fclose(fp);
    }
    for (gatewayid=0; gatewayid<NUM_GATEWAYS; gatewayid++)
    {
        if ( gatewayid != cointx->gatewayid )
        {
            retstr = start_transfer(0,ram->srvNXTADDR,ram->srvNXTADDR,ram->srvNXTACCTSECRET,Server_ipaddrs[gatewayid],name,(uint8_t *)cointx,cointx->allocsize,300,"RTmgw",1);
            if ( retstr != 0 )
                free(retstr);
        }
        fprintf(stderr,"got publish_withdraw_info.%d -> %d coin.(%s) %.8f crc %08x\n",ram->S.gatewayid,gatewayid,cointx->coinstr,dstr(cointx->amount),cointx->batchcrc);
    }
}

uint64_t _find_pending_transfers(uint64_t *pendingredeemsp,struct ramchain_info *ram)
{
    int32_t j,disable_newsends,specialsender,specialreceiver,numpending = 0;
    char sender[64],receiver[64],txidstr[512],withdrawaddr[512],*destaddr;
    struct NXT_assettxid *tp;
    struct NXT_asset *ap;
    struct cointx_info *cointx;
    uint64_t orphans = 0;
    *pendingredeemsp = 0;
    disable_newsends = ((ram->numpendingsends > 0) || (ram->S.gatewayid < 0));
    if ( (ap= ram->ap) == 0 )
    {
        printf("no NXT_asset for %s\n",ram->name);
        return(0);
    }
    for (j=0; j<ap->num; j++)
    {
        tp = ap->txids[j];
        //if ( strcmp(ram->name,"BITS") == 0 )
        //printf("%d of %d: check %s.%llu completed.%d\n",j,ap->num,ram->name,(long long)tp->redeemtxid,tp->completed);
        if ( tp->completed == 0 )
        {
            _expand_nxt64bits(sender,tp->senderbits);
            specialsender = _in_specialNXTaddrs(ram->special_NXTaddrs,ram->numspecials,sender);
            _expand_nxt64bits(receiver,tp->receiverbits);
            specialreceiver = _in_specialNXTaddrs(ram->special_NXTaddrs,ram->numspecials,receiver);
            if ( (specialsender ^ specialreceiver) == 0 || tp->cointxid != 0 )
            {
                printf("autocomplete: %llu cointxid.%p\n",(long long)tp->redeemtxid,tp->cointxid);
                _complete_assettxid(ram,tp);
            }
            else
            {
                if ( _is_limbo_redeem(ram,tp->redeemtxid) != 0 )
                {
                    printf("autocomplete: limbo %llu cointxid.%p\n",(long long)tp->redeemtxid,tp->cointxid);
                    _complete_assettxid(ram,tp);
                }
                //printf("receiver.%llu vs MGW.%llu\n",(long long)tp->receiverbits,(long long)ram->MGWbits);
                if ( tp->receiverbits == ram->MGWbits ) // redeem start
                {
                    destaddr = "coinaddr";
                    if ( _valid_txamount(ram,tp->U.assetoshis,0) > 0 && (tp->convwithdrawaddr != 0 || (destaddr= _parse_withdraw_instructions(withdrawaddr,sender,ram,tp,ap)) != 0) )
                    {
                        if ( tp->convwithdrawaddr == 0 )
                            tp->convwithdrawaddr = clonestr(destaddr);
                        if ( tp->redeemstarted == 0 )
                        {
                            printf("find_pending_transfers: redeem.%llu started %s %.8f for NXT.%s to %s\n",(long long)tp->redeemtxid,ram->name,dstr(tp->U.assetoshis),sender,destaddr!=0?destaddr:"no withdraw address");
                            tp->redeemstarted = (uint32_t)time(NULL);
                        }
                        else
                        {
                            int32_t i,numpayloads;
                            struct ramchain_hashptr *addrptr;
                            struct rampayload *payloads;
                            if ( (payloads= ram_addrpayloads(&addrptr,&numpayloads,ram,destaddr)) != 0 && addrptr != 0 && numpayloads > 0 )
                            {
                                for (i=0; i<numpayloads; i++)
                                    if ( (dstr(tp->U.assetoshis) - dstr(payloads[i].value)) == .0101 ) // historical BTCD parameter
                                    {
                                        printf("(autocomplete.%llu payload.i%d >>>>>>>> %.8f <<<<<<<<<) ",(long long)tp->redeemtxid,i,dstr(payloads[i].value));
                                        _complete_assettxid(ram,tp);
                                    }
                             }
                        }
                        if ( tp->completed == 0 && tp->convwithdrawaddr != 0 )
                        {
                            (*pendingredeemsp) += tp->U.assetoshis;
                            printf("%s NXT.%llu withdraw.(%llu %.8f).rt%d_%d_%d_%d.g%d -> %s elapsed %.1f minutes | pending.%d\n",ram->name,(long long)tp->senderbits,(long long)tp->redeemtxid,dstr(tp->U.assetoshis),ram->S.enable_withdraws,ram->S.is_realtime,(tp->height + ram->withdrawconfirms) <= ram->S.NXT_RTblocknum,ram->S.MGWbalance >= 0,(int32_t)(tp->senderbits % NUM_GATEWAYS),tp->convwithdrawaddr,(double)(time(NULL) - tp->redeemstarted)/60,ram->numpendingsends);
                            numpending++;
                            if ( disable_newsends == 0 )
                            {
                                if ( (cointx= _calc_cointx_withdraw(ram,tp->convwithdrawaddr,tp->U.assetoshis,tp->redeemtxid)) != 0 )
                                {
                                    if ( ram_MGW_ready(ram,0,tp->height,0,tp->U.assetoshis) > 0 )
                                    {
                                        ram_send_cointx(ram,cointx);
                                        ram->numpendingsends++;
                                        //ram_add_pendingsend(0,ram,tp,cointx);
                                        // disable_newsends = 1;
                                    } else printf("not ready to withdraw yet\n");
                                }
                                else if ( ram->S.enable_withdraws != 0 && ram->S.is_realtime != 0 && ram->S.NXT_is_realtime != 0 )
                                {
                                    //tp->completed = 1; // ignore malformed requests for now
                                }
                            }
                            if ( ram->S.gatewayid >= 0 && ram_check_consensus(txidstr,ram,tp) != 0 )
                                printf("completed redeem.%llu with cointxid.%s\n",(long long)tp->redeemtxid,txidstr);
                            //printf("(%llu %.8f).%d ",(long long)tp->redeemtxid,dstr(tp->U.assetoshis),(int32_t)(time(NULL) - tp->redeemstarted));
                        } else printf("%llu %.8f: completed.%d withdraw.%p destaddr.%p\n",(long long)tp->redeemtxid,dstr(tp->U.assetoshis),tp->completed,tp->convwithdrawaddr,destaddr);
                    }
                    else if ( tp->completed == 0 && _valid_txamount(ram,tp->U.assetoshis,0) > 0 )
                        printf("incomplete but skipped.%llu: %.8f destaddr.%s\n",(long long)tp->redeemtxid,dstr(tp->U.assetoshis),destaddr);
                    else printf("%s.%llu %.8f is too small, thank you for your donation to MGW\n",ram->name,(long long)tp->redeemtxid,dstr(tp->U.assetoshis)), tp->completed = 1;
                }
                else if ( tp->completed == 0 && specialsender != 0 ) // deposit complete w/o cointxid (shouldnt happen normally)
                {
                    orphans += tp->U.assetoshis;
                    _complete_assettxid(ram,tp);
                    printf("find_pending_transfers: internal transfer.%llu limbo.%d complete %s %.8f to NXT.%s\n",(long long)tp->redeemtxid,_is_limbo_redeem(ram,tp->redeemtxid),ram->name,dstr(tp->U.assetoshis),receiver);
                } else tp->completed = 1; // this is some independent tx
            }
        }
    }
    if ( numpending == 0 && ram->numpendingsends != 0 )
    {
        printf("All pending withdraws done!\n");
        ram->numpendingsends = 0;
    }
    return(orphans);
}

int32_t ram_mark_depositcomplete(struct ramchain_info *ram,struct NXT_assettxid *tp,uint32_t blocknum)
{ // NXT
    struct ramchain_hashptr *addrptr,*txptr;
    struct rampayload *addrpayload,*txpayload;
    int32_t numtxpayloads;
    if ( tp->cointxid != 0 )
    {
        if ( (txpayload= ram_payloads(&txptr,&numtxpayloads,ram,tp->cointxid,'t')) != 0 && tp->coinv < numtxpayloads )
        {
            txpayload = &txpayload[tp->coinv];
            if ( (addrpayload= ram_getpayloadi(&addrptr,ram,'a',txpayload->otherind,txpayload->extra)) != 0 )
            {
                if ( txptr->rawind == addrpayload->otherind && txpayload->value == addrpayload->value )
                {
                    if ( addrpayload->pendingdeposit != 0 )
                    {
                        printf("deposit complete %s.%s/v%d %.8f -> NXT.%llu txid.%llu\n",ram->name,tp->cointxid,tp->coinv,dstr(tp->U.assetoshis),(long long)tp->receiverbits,(long long)tp->redeemtxid);
                        addrpayload->pendingdeposit = 0;
                        _complete_assettxid(ram,tp);
                    }
                    else
                    {
                        if ( tp->completed == 0 )
                            printf("deposit NOT PENDING? complete %s.%s/v%d %.8f -> NXT.%llu txid.%llu\n",ram->name,tp->cointxid,tp->coinv,dstr(tp->U.assetoshis),(long long)tp->receiverbits,(long long)tp->redeemtxid);
                    }
                    return(1);
                } else printf("ram_mark_depositcomplete: mismatched rawind or value (%u vs %d) (%.8f vs %.8f)\n",txptr->rawind,addrpayload->otherind,dstr(txpayload->value),dstr(addrpayload->value));
            } else printf("ram_mark_depositcomplete: couldnt find addrpayload for %s vout.%d\n",tp->cointxid,tp->coinv);
        } else printf("ram_mark_depositcomplete: couldnt find (%s) txpayload.%p or tp->coinv.%d >= %d numtxpayloads blocknum.%d\n",tp->cointxid,txpayload,tp->coinv,numtxpayloads,blocknum);
    } else printf("ram_mark_depositcomplete: unexpected null cointxid\n");
    return(0);
}

void ram_addunspent(struct ramchain_info *ram,char *coinaddr,struct rampayload *txpayload,struct ramchain_hashptr *addrptr,struct rampayload *addrpayload,uint32_t addr_rawind,uint32_t ind)
{ // bitcoin
    // ram_addunspent() is called from the rawtx_update that creates the txpayloads, each coinaddr gets a list of addrpayloads initialized
    // it needs to update the corresponding txpayload so exact state can be quickly calculated
    //struct NXT_assettxid *tp;
    struct multisig_addr *msig;
    txpayload->B = addrpayload->B;
    txpayload->value = addrpayload->value;
    //printf("txpayload.%p addrpayload.%p set addr_rawind.%d -> %p txid_rawind.%u\n",txpayload,addrpayload,addr_rawind,txpayload,addrpayload->otherind);
    txpayload->otherind = addr_rawind;  // allows for direct lookup of coin addr payload vector
    txpayload->extra = ind;             // allows for direct lookup of the payload within the vector
    if ( addrpayload->value != 0 )
    {
        //printf("UNSPENT %.8f\n",dstr(addrpayload->value));
        ram->S.totaloutputs += addrpayload->value;
        ram->S.numoutputs++;
        addrptr->unspent += addrpayload->value;
        addrptr->numunspent++;
        if ( addrptr->multisig != 0 && (msig= find_msigaddr(coinaddr)) != 0 && _in_specialNXTaddrs(ram->special_NXTaddrs,ram->numspecials,msig->NXTaddr) == 0 )
        {
            if ( addrpayload->B.isinternal == 0 ) // all non-internal unspents could be MGW deposit or withdraw
            {
                {
                    char txidstr[256];
                    ram_txid(txidstr,ram,addrpayload->otherind);
                    printf("ram_addunspent.%s: pending deposit %s %.8f -> %s rawind %d.i%d for NXT.%s\n",txidstr,ram->name,dstr(addrpayload->value),coinaddr,addr_rawind,ind,msig->NXTaddr);
                }
                addrpayload->pendingdeposit = _valid_txamount(ram,addrpayload->value,coinaddr);
            } else if ( 0 && addrptr->multisig != 0 )
                printf("find_msigaddr: couldnt find.(%s)\n",coinaddr);
            /*else if ( (tp= _is_pending_withdraw(ram,coinaddr)) != 0 )
             {
             if ( tp->completed == 0 && _is_pending_withdrawamount(ram,tp,addrpayload->value) != 0 ) // one to many problem
             {
             printf("ram_addunspent: pending withdraw %s %.8f -> %s completed for NXT.%llu\n",ram->name,dstr(tp->U.assetoshis),coinaddr,(long long)tp->senderbits);
             tp->completed = 1;
             }
             }*/
        }
    }
}

int32_t _ram_update_redeembits(struct ramchain_info *ram,uint64_t redeembits,uint64_t AMtxidbits,char *cointxid,struct address_entry *bp)
{
    struct NXT_asset *ap = ram->ap;
    struct NXT_assettxid *tp;
    int32_t createdflag,num = 0;
    char txid[64];
    if ( ram == 0 )
        return(0);
    _expand_nxt64bits(txid,redeembits);
    tp = find_NXT_assettxid(&createdflag,ap,txid);
    tp->assetbits = ap->assetbits;
    tp->redeemtxid = redeembits;
    if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone > 1 )
        printf("_ram_update_redeembits.apnum.%d set AMtxidbits.%llu -> %s redeem (%llu) cointxid.%p tp.%p\n",ap->num,(long long)AMtxidbits,ram->name,(long long)redeembits,cointxid,tp);
    if ( tp->redeemtxid == redeembits )
    {
        if ( AMtxidbits != 0 )
            tp->AMtxidbits = AMtxidbits;
        _complete_assettxid(ram,tp);
        if ( bp != 0 && bp->blocknum != 0 )
        {
            tp->coinblocknum = bp->blocknum;
            tp->cointxind = bp->txind;
            tp->coinv = bp->v;
        }
        if ( cointxid != 0 )
        {
            if ( tp->cointxid != 0 )
            {
                if ( strcmp(tp->cointxid,cointxid) != 0 )
                {
                    printf("_ram_update_redeembits: unexpected cointxid.(%s) already there for redeem.%llu (%s)\n",tp->cointxid,(long long)redeembits,cointxid);
                    free(tp->cointxid);
                    tp->cointxid = clonestr(cointxid);
                }
            }
            else tp->cointxid = clonestr(cointxid);
        }
        num++;
    }
    if ( AMtxidbits == 0 && num == 0 )
        printf("_ram_update_redeembits: unexpected no pending redeems when AMtxidbits.0\n");
    return(num);
}

int32_t ram_update_redeembits(struct ramchain_info *ram,cJSON *argjson,uint64_t AMtxidbits)
{
    cJSON *array;
    uint64_t redeembits;
    int32_t i,n,num = 0;
    struct address_entry B;
    char coinstr[MAX_JSON_FIELD],redeemtxid[MAX_JSON_FIELD],cointxid[MAX_JSON_FIELD];
    if ( extract_cJSON_str(coinstr,sizeof(coinstr),argjson,"coin") <= 0 )
        return(0);
    if ( ram != 0 && strcmp(ram->name,coinstr) != 0 )
        return(0);
    extract_cJSON_str(cointxid,sizeof(cointxid),argjson,"cointxid");
    memset(&B,0,sizeof(B));
    B.blocknum = (uint32_t)get_cJSON_int(argjson,"coinblocknum");
    B.txind = (uint32_t)get_cJSON_int(argjson,"cointxind");
    B.v = (uint32_t)get_cJSON_int(argjson,"coinv");
    if ( B.blocknum != 0 )
        printf("(%d %d %d) -> %s\n",B.blocknum,B.txind,B.v,cointxid);
    array = cJSON_GetObjectItem(argjson,"redeems");
    if ( array != 0 && is_cJSON_Array(array) != 0 )
    {
        n = cJSON_GetArraySize(array);
        for (i=0; i<n; i++)
        {
            copy_cJSON(redeemtxid,cJSON_GetArrayItem(array,i));
            redeembits = _calc_nxt64bits(redeemtxid);
            if ( redeemtxid[0] != 0 )
                num += _ram_update_redeembits(ram,redeembits,AMtxidbits,cointxid,&B);
        }
    }
    else
    {
        if ( extract_cJSON_str(redeemtxid,sizeof(redeemtxid),argjson,"redeemtxid") > 0 )
        {
            printf("no redeems: (%s)\n",cJSON_Print(argjson));
            num += _ram_update_redeembits(ram,_calc_nxt64bits(redeemtxid),AMtxidbits,cointxid,&B);
        }
    }
    if ( 0 && num == 0 )
        printf("unexpected num.0 for ram_update_redeembits.(%s)\n",cJSON_Print(argjson));
    return(num);
}

int32_t ram_markspent(struct ramchain_info *ram,struct rampayload *txpayload,struct address_entry *spendbp,uint32_t txid_rawind)
{ // bitcoin
    struct rampayload *ram_getpayloadi(struct ramchain_hashptr **addrptrp,struct ramchain_info *ram,char type,uint32_t rawind,uint32_t i);
    struct ramchain_hashptr *addrptr;
    struct rampayload *addrpayload;
    txpayload->spentB = *spendbp; // now all fields are set for the payloads from the tx
    // now we need to update the addrpayload
    if ( (addrpayload= ram_getpayloadi(&addrptr,ram,'a',txpayload->otherind,txpayload->extra)) != 0 )
    {
        if ( txid_rawind == addrpayload->otherind && txpayload->value == addrpayload->value )
        {
            if ( txpayload->value != 0 )
            {
                ram->S.totalspends += txpayload->value;
                ram->S.numspends++;
                addrpayload->spentB = *spendbp;
                addrpayload->B.spent = 1;
                addrptr->unspent -= addrpayload->value;
                addrptr->numunspent--;
                //printf("SPENT %.8f\n",dstr(addrpayload->value));
                //printf("MATCH: spendtxid_rawind.%u == %u addrpayload->otherind for (%d %d %d)\n",txid_rawind,addrpayload->otherind,spendbp->blocknum,spendbp->txind,spendbp->v);
            }
        } else printf("FATAL: txpayload.%p[%d] addrpayload.%p spendtxid_rawind.%u != %u addrpayload->otherind for (%d %d %d)\n",txpayload,txpayload->extra,addrpayload,txid_rawind,addrpayload->otherind,spendbp->blocknum,spendbp->txind,spendbp->v);
    } else printf("FATAL: ram_markspent cant find addpayload (%d %d %d) addrind.%d i.%d | txpayload.%p txid_rawind.%u\n",spendbp->blocknum,spendbp->txind,spendbp->v,txpayload->otherind,txpayload->extra,txpayload,txid_rawind);
    if ( addrpayload->value != txpayload->value )
        printf("FATAL: addrpayload %.8f vs txpayload %.8f\n",dstr(addrpayload->value),dstr(txpayload->value));
    return(-1);
}

cJSON *_process_MGW_message(struct ramchain_info *ram,uint32_t height,int32_t funcid,cJSON *argjson,uint64_t itemid,uint64_t units,char *sender,char *receiver,char *txid)
{
    struct multisig_addr *decode_msigjson(char *NXTaddr,cJSON *obj,char *sender);
    void update_coinacct_addresses(uint64_t nxt64bits,cJSON *json,char *txid);
    uint64_t nxt64bits = _calc_nxt64bits(sender);
    struct multisig_addr *msig;
    // we can ignore height as the sender is validated or it is non-money get_coindeposit_address request
    if ( _calc_nxt64bits(receiver) != ram->MGWbits && _calc_nxt64bits(sender) != ram->MGWbits )
        return(0);
    if ( argjson != 0 )
    {
        //if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
        //    fprintf(stderr,"func.(%c) %s -> %s txid.(%s) JSON.(%s)\n",ap->funcid,sender,receiver,txid,ap->U.jsonstr);
        switch ( funcid )
        {
            case GET_COINDEPOSIT_ADDRESS:
                // start address gen
                //fprintf(stderr,"GENADDRESS: func.(%c) %s -> %s txid.(%s) JSON.(%s)\n",ap->funcid,sender,receiver,txid,ap->U.jsonstr);
                //fprintf(stderr,"g");
                update_coinacct_addresses(nxt64bits,argjson,txid);
                break;
            case BIND_DEPOSIT_ADDRESS:
                //fprintf(stderr,"b");
                if ( _in_specialNXTaddrs(ram->special_NXTaddrs,ram->numspecials,sender) != 0 && (msig= decode_msigjson(0,argjson,sender)) != 0 ) // strcmp(sender,receiver) == 0) &&
                {
                    if ( strcmp(msig->coinstr,ram->name) == 0 && find_msigaddr(msig->multisigaddr) == 0 )
                    {
                        //if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone > 1 )
                        //    fprintf(stderr,"BINDFUNC: %s func.(%c) %s -> %s txid.(%s)\n",msig->coinstr,funcid,sender,receiver,txid);
                        if ( update_msig_info(msig,1,sender) > 0 )
                        {
                            //fprintf(stderr,"%s func.(%c) %s -> %s txid.(%s)\n",msig->coinstr,funcid,sender,receiver,txid);
                        }
                    }
                    free(msig);
                } //else printf("WARNING: sender.%s == NXTaddr.%s\n",sender,NXTaddr);
                break;
            case DEPOSIT_CONFIRMED:
                // need to mark cointxid with AMtxid to prevent confirmation process generating AM each time
                /*if ( is_gateway_addr(sender) != 0 && (coinid= decode_depositconfirmed_json(argjson,txid)) >= 0 )
                 {
                 printf("deposit confirmed for coinid.%d %s\n",coinid,coinid_str(coinid));
                 }*/
                break;
            case MONEY_SENT:
                //fprintf(stderr,"m");
                //printf("MONEY_SENT.(%s)\n",cJSON_Print(argjson));
                if ( _in_specialNXTaddrs(ram->special_NXTaddrs,ram->numspecials,sender) != 0 )
                    ram_update_redeembits(ram,argjson,_calc_nxt64bits(txid));
                break;
            default: printf("funcid.(%c) not handled\n",funcid);
        }
    }
    return(argjson);
}

cJSON *_parse_json_AM(struct json_AM *ap)
{
    char *jsontxt;
    if ( ap->jsonflag != 0 )
    {
        jsontxt = (ap->jsonflag == 1) ? ap->U.jsonstr : 0;//decode_json(&ap->U.jsn,ap->jsonflag);
        if ( jsontxt != 0 )
        {
            if ( jsontxt[0] == '"' && jsontxt[strlen(jsontxt)-1] == '"' )
                _unstringify(jsontxt);
            return(cJSON_Parse(jsontxt));
        }
    }
    return(0);
}

void _process_AM_message(struct ramchain_info *ram,uint32_t height,struct json_AM *AM,char *sender,char *receiver,char *txid)
{
    cJSON *argjson;
    if ( AM == 0 )
        return;
    if ( (argjson= _parse_json_AM(AM)) != 0 )
    {
        if ( 0 && ((MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone > 1) )
            fprintf(stderr,"func.(%c) %s -> %s txid.(%s) JSON.(%s)\n",AM->funcid,sender,receiver,txid,AM->U.jsonstr);
        //else fprintf(stderr,"%c",AM->funcid);
        if ( AM->funcid > 0 )
            argjson = _process_MGW_message(ram,height,AM->funcid,argjson,0,0,sender,receiver,txid);
        if ( argjson != 0 )
            free_json(argjson);
    }
}

struct NXT_assettxid *_set_assettxid(struct ramchain_info *ram,uint32_t height,char *redeemtxidstr,uint64_t senderbits,uint64_t receiverbits,uint32_t timestamp,char *commentstr,uint64_t quantity)
{
    uint64_t redeemtxid;
    struct NXT_assettxid *tp;
    int32_t createdflag;
    struct NXT_asset *ap;
    cJSON *json,*cointxidobj,*obj;
    char sender[MAX_JSON_FIELD],cointxid[MAX_JSON_FIELD],coinstr[MAX_JSON_FIELD];
    if ( (ap= ram->ap) == 0 )
    {
        printf("no NXT_asset for %s\n",ram->name);
        return(0);
    }
    redeemtxid = _calc_nxt64bits(redeemtxidstr);
    tp = find_NXT_assettxid(&createdflag,ap,redeemtxidstr);
    tp->assetbits = ap->assetbits;
    if ( (tp->height= height) != 0 )
        tp->numconfs = (ram->S.NXT_RTblocknum - height);
    tp->redeemtxid = redeemtxid;
    if ( timestamp > tp->timestamp )
        tp->timestamp = timestamp;
    tp->quantity = quantity;
    tp->U.assetoshis = (quantity * ap->mult);
    tp->receiverbits = receiverbits;
    tp->senderbits = senderbits;
    //printf("_set_assettxid(%s)\n",commentstr);
    if ( commentstr != 0 && (json= cJSON_Parse(commentstr)) != 0 ) //(tp->comment == 0 || strcmp(tp->comment,commentstr) != 0) &&
    {
        copy_cJSON(coinstr,cJSON_GetObjectItem(json,"coin"));
        if ( coinstr[0] == 0 )
            strcpy(coinstr,ram->name);
        //printf("%s txid.(%s) (%s)\n",ram->name,redeemtxidstr,commentstr!=0?commentstr:"NULL");
        if ( strcmp(coinstr,ram->name) == 0 )
        {
            if ( tp->comment != 0 )
                free(tp->comment);
            tp->comment = clonestr(commentstr);
            _stripwhite(tp->comment,' ');
            tp->buyNXT = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"buyNXT"),0);
            tp->coinblocknum = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"coinblocknum"),0);
            tp->cointxind = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"cointxind"),0);
            if ( (obj= cJSON_GetObjectItem(json,"coinv")) == 0 )
                obj = cJSON_GetObjectItem(json,"vout");
            tp->coinv = (uint32_t)get_API_int(obj,0);
            if ( ram->NXTfee_equiv != 0 && ram->txfee != 0 )
                tp->estNXT = (((double)ram->NXTfee_equiv / ram->txfee) * tp->U.assetoshis / SATOSHIDEN);
            if ( (cointxidobj= cJSON_GetObjectItem(json,"cointxid")) != 0 )
            {
                copy_cJSON(cointxid,cointxidobj);
                if ( cointxid[0] != 0 )
                {
                    if ( tp->cointxid != 0 && strcmp(tp->cointxid,cointxid) != 0 )
                    {
                        printf("cointxid conflict for redeemtxid.%llu: (%s) != (%s)\n",(long long)redeemtxid,tp->cointxid,cointxid);
                        free(tp->cointxid);
                    }
                    tp->cointxid = clonestr(cointxid);
                    expand_nxt64bits(sender,senderbits);
                    if ( _in_specialNXTaddrs(ram->special_NXTaddrs,ram->numspecials,sender) != 0 && tp->sentNXT != tp->buyNXT )
                    {
                        //ram->boughtNXT -= tp->sentNXT;
                        //ram->boughtNXT += tp->buyNXT;
                        tp->sentNXT = tp->buyNXT;
                    }
                }
                if ( tp->completed == 0 )
                {
                    if ( ram_mark_depositcomplete(ram,tp,tp->coinblocknum) != 0 )
                        _complete_assettxid(ram,tp);
                }
                if ( Debuglevel > 2 )
                    printf("sender.%llu receiver.%llu got.(%llu) comment.(%s) (B%d t%d) cointxidstr.(%s)/v%d buyNXT.%d completed.%d\n",(long long)senderbits,(long long)receiverbits,(long long)redeemtxid,tp->comment,tp->coinblocknum,tp->cointxind,cointxid,tp->coinv,tp->buyNXT,tp->completed);
            }
            else
            {
                if ( Debuglevel > 2 )
                    printf("%s txid.(%s) got comment.(%s) gotpossibleredeem.(%d.%d.%d) %.8f/%.8f NXTequiv %.8f -> redeemtxid.%llu\n",ap->name,redeemtxidstr,tp->comment!=0?tp->comment:"",tp->coinblocknum,tp->cointxind,tp->coinv,dstr(tp->quantity * ap->mult),dstr(tp->U.assetoshis),tp->estNXT,(long long)tp->redeemtxid);
            }
        } else printf("mismatched coin.%s vs (%s) for transfer.%llu (%s)\n",coinstr,ram->name,(long long)redeemtxid,commentstr);
        free_json(json);
    } //else printf("error with (%s) tp->comment %p\n",commentstr,tp->comment);
    return(tp);
}

void _set_NXT_sender(char *sender,cJSON *txobj)
{
    cJSON *senderobj;
    senderobj = cJSON_GetObjectItem(txobj,"sender");
    if ( senderobj == 0 )
        senderobj = cJSON_GetObjectItem(txobj,"accountId");
    else if ( senderobj == 0 )
        senderobj = cJSON_GetObjectItem(txobj,"account");
    copy_cJSON(sender,senderobj);
}

void ram_gotpayment(struct ramchain_info *ram,char *comment,cJSON *commentobj)
{
    printf("ram_gotpayment.(%s) from %s\n",comment,ram->name);
}

uint32_t _process_NXTtransaction(int32_t confirmed,struct ramchain_info *ram,cJSON *txobj)
{
    char AMstr[MAX_JSON_FIELD],sender[MAX_JSON_FIELD],receiver[MAX_JSON_FIELD],assetidstr[MAX_JSON_FIELD],txid[MAX_JSON_FIELD],comment[MAX_JSON_FIELD],*commentstr = 0;
    cJSON *attachment,*message,*assetjson,*commentobj;
    unsigned char buf[4096];
    struct NXT_AMhdr *hdr;
    struct NXT_asset *ap = ram->ap;
    struct NXT_assettxid *tp;
    uint64_t units;
    uint32_t buyNXT,height = 0;
    int32_t funcid,numconfs,timestamp=0;
    int64_t type,subtype,n,satoshis,assetoshis = 0;
    if ( txobj != 0 )
    {
        hdr = 0;
        sender[0] = receiver[0] = 0;
        if ( confirmed != 0 )
        {
            height = (uint32_t)get_cJSON_int(txobj,"height");
            if ( (numconfs= (int32_t)get_API_int(cJSON_GetObjectItem(txobj,"confirmations"),0)) == 0 )
                numconfs = (ram->S.NXT_RTblocknum - height);
        } else numconfs = 0;
        copy_cJSON(txid,cJSON_GetObjectItem(txobj,"transaction"));
        //if ( strcmp(txid,"1110183143900371107") == 0 )
        //    printf("TX.(%s) %s\n",txid,cJSON_Print(txobj));
        type = get_cJSON_int(txobj,"type");
        subtype = get_cJSON_int(txobj,"subtype");
        timestamp = (int32_t)get_cJSON_int(txobj,"blockTimestamp");
        _set_NXT_sender(sender,txobj);
        copy_cJSON(receiver,cJSON_GetObjectItem(txobj,"recipient"));
        attachment = cJSON_GetObjectItem(txobj,"attachment");
        if ( attachment != 0 )
        {
            message = cJSON_GetObjectItem(attachment,"message");
            assetjson = cJSON_GetObjectItem(attachment,"asset");
            memset(comment,0,sizeof(comment));
            if ( message != 0 && type == 1 )
            {
                copy_cJSON(AMstr,message);
                n = strlen(AMstr);
                if ( is_hexstr(AMstr) != 0 )
                {
                    if ( (n&1) != 0 || n > 2000 )
                        printf("warning: odd message len?? %ld\n",(long)n);
                    decode_hex((void *)buf,(int32_t)(n>>1),AMstr);
                    buf[(n>>1)] = 0;
                    hdr = (struct NXT_AMhdr *)buf;
                    _process_AM_message(ram,height,(void *)hdr,sender,receiver,txid);
                }
            }
            else if ( assetjson != 0 && type == 2 && subtype == 1 )
            {
                commentobj = cJSON_GetObjectItem(attachment,"comment");
                if ( commentobj == 0 )
                    commentobj = message;
                copy_cJSON(comment,commentobj);
                if ( comment[0] != 0 )
                    commentstr = clonestr(_unstringify(comment));
                copy_cJSON(assetidstr,cJSON_GetObjectItem(attachment,"asset"));
                //if ( strcmp(txid,"998606823456096714") == 0 )
                //printf("Inside comment.(%s): %s cmp.%d\n",assetidstr,comment,ap->assetbits == _calc_nxt64bits(assetidstr));
                if ( assetidstr[0] != 0 && ap->assetbits == _calc_nxt64bits(assetidstr) )
                {
                    assetoshis = get_cJSON_int(attachment,"quantityQNT");
                    //fprintf(stderr,"a");
                    tp = _set_assettxid(ram,height,txid,_calc_nxt64bits(sender),_calc_nxt64bits(receiver),timestamp,commentstr,assetoshis);
                    //if ( _in_specialNXTaddrs(ram->special_NXTaddrs,ram->numspecials,sender) != 0  )
                    //    _add_pendingxfer(ram,height,1,tp->redeemtxid);
                }
            }
            else
            {
                copy_cJSON(comment,message);
                _unstringify(comment);
                commentobj = comment[0] != 0 ? cJSON_Parse(comment) : 0;
                if ( type == 5 && subtype == 3 )
                {
                    copy_cJSON(assetidstr,cJSON_GetObjectItem(attachment,"currency"));
                    units = get_API_int(cJSON_GetObjectItem(attachment,"units"),0);
                    if ( commentobj != 0 )
                    {
                        funcid = (int32_t)get_API_int(cJSON_GetObjectItem(commentobj,"funcid"),-1);
                        if ( funcid >= 0 && (commentobj= _process_MGW_message(ram,height,funcid,commentobj,_calc_nxt64bits(assetidstr),units,sender,receiver,txid)) != 0 )
                            free_json(commentobj);
                    }
                }
                else if ( type == 0 && subtype == 0 && commentobj != 0 )
                {
                    //if ( strcmp(txid,"998606823456096714") == 0 )
                    //    printf("Inside message: %s\n",comment);
                    if ( _in_specialNXTaddrs(ram->special_NXTaddrs,ram->numspecials,sender) != 0 )
                    {
                        buyNXT = get_API_int(cJSON_GetObjectItem(commentobj,"buyNXT"),0);
                        satoshis = get_API_nxt64bits(cJSON_GetObjectItem(txobj,"amountNQT"));
                        if ( buyNXT*SATOSHIDEN == satoshis )
                        {
                            ram->S.sentNXT += buyNXT * SATOSHIDEN;
                            printf("%s sent %d NXT, total sent %.0f\n",sender,buyNXT,dstr(ram->S.sentNXT));
                        }
                        else if ( buyNXT != 0 )
                            printf("unexpected QNT %.8f vs %d\n",dstr(satoshis),buyNXT);
                    }
                    if ( strcmp(sender,ram->srvNXTADDR) == 0 )
                        ram_gotpayment(ram,comment,commentobj);
                }
            }
        }
    }
    else printf("unexpected error iterating timestamp.(%d) txid.(%s)\n",timestamp,txid);
    //fprintf(stderr,"finish type.%d subtype.%d txid.(%s)\n",(int)type,(int)subtype,txid);
    return(height);
}

char *_ram_loadfp(FILE *fp)
{
    long len;
    char *jsonstr;
    fseek(fp,0,SEEK_END);
    len = ftell(fp);
    jsonstr = calloc(1,len);
    rewind(fp);
    fread(jsonstr,1,len,fp);
    return(jsonstr);
}

uint32_t _update_ramMGW(uint32_t *firsttimep,struct ramchain_info *ram,uint32_t mostrecent)
{
    FILE *fp;
    cJSON *redemptions=0,*transfers,*array,*json;
    char fname[512],cmd[1024],txid[512],*jsonstr,*txidjsonstr;
    struct NXT_asset *ap;
    uint32_t i,j,n,height,iter,timestamp,oldest;
    while ( (ap= ram->ap) == 0 )
        portable_sleep(1);
    if ( ram->MGWbits == 0 )
    {
        printf("no MGWbits for %s\n",ram->name);
        return(0);
    }
    i = _get_NXTheight(&oldest);
    ram->S.NXT_ECblock = _get_NXT_ECblock(&ram->S.NXT_ECheight);
    //printf("NXTheight.%d ECblock.%d mostrecent.%d\n",i,ram->S.NXT_ECheight,mostrecent);
    if ( firsttimep != 0 )
        *firsttimep = oldest;
    if ( i != ram->S.NXT_RTblocknum )
    {
        ram->S.NXT_RTblocknum = i;
        printf("NEW NXTblocknum.%d when mostrecent.%d\n",i,mostrecent);
    }
    if ( mostrecent > 0 )
    {
        //printf("mostrecent %d <= %d (ram->S.NXT_RTblocknum %d - %d ram->min_NXTconfirms)\n", mostrecent,(ram->S.NXT_RTblocknum - ram->min_NXTconfirms),ram->S.NXT_RTblocknum,ram->min_NXTconfirms);
        while ( mostrecent <= (ram->S.NXT_RTblocknum - ram->min_NXTconfirms) )
        {
            sprintf(cmd,"requestType=getBlock&height=%u&includeTransactions=true",mostrecent);
            //printf("send cmd.(%s)\n",cmd);
            if ( (jsonstr= _issue_NXTPOST(cmd)) != 0 )
            {
               // printf("getBlock.%d (%s)\n",mostrecent,jsonstr);
                if ( (json= cJSON_Parse(jsonstr)) != 0 )
                {
                    timestamp = (uint32_t)get_cJSON_int(json,"timestamp");
                    if ( timestamp != 0 && timestamp > ram->NXTtimestamp )
                    {
                        if ( ram->firsttime == 0 )
                            ram->firsttime = timestamp;
                        if ( ram->S.enable_deposits == 0 && timestamp > (ram->firsttime + (ram->DEPOSIT_XFER_DURATION+1)*60) )
                        {
                            ram->S.enable_deposits = 1;
                            printf("1st.%d ram->NXTtimestamp %d -> %d: enable_deposits.%d | %d > %d\n",ram->firsttime,ram->NXTtimestamp,timestamp,ram->S.enable_deposits,(timestamp - ram->firsttime),(ram->DEPOSIT_XFER_DURATION+1)*60);
                        }
                        ram->NXTtimestamp = timestamp;
                    }
                    if ( (array= cJSON_GetObjectItem(json,"transactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
                    {
                        for (i=0; i<n; i++)
                            _process_NXTtransaction(1,ram,cJSON_GetArrayItem(array,i));
                    }
                    free_json(json);
                } else printf("error parsing.(%s)\n",jsonstr);
                free(jsonstr);
            } else printf("error sending.(%s)\n",cmd);
            mostrecent++;
        }
        if ( ram->min_NXTconfirms == 0 )
        {
            sprintf(cmd,"requestType=getUnconfirmedTransactions");
            if ( (jsonstr= _issue_NXTPOST(cmd)) != 0 )
            {
                //printf("getUnconfirmedTransactions.%d (%s)\n",mostrecent,jsonstr);
                if ( (json= cJSON_Parse(jsonstr)) != 0 )
                {
                    if ( (array= cJSON_GetObjectItem(json,"unconfirmedTransactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
                    {
                        for (i=0; i<n; i++)
                            _process_NXTtransaction(0,ram,cJSON_GetArrayItem(array,i));
                    }
                    free_json(json);
                }
                free(jsonstr);
            }
        }
    }
    else
    {
        for (j=0; j<ram->numspecials; j++)
        {
            fp = 0;
            sprintf(fname,"%s/ramchains/NXT.%s",MGWROOT,ram->special_NXTaddrs[j]);
            printf("(%s) init NXT special.%d of %d (%s) [%s]\n",ram->name,j,ram->numspecials,ram->special_NXTaddrs[j],fname);
            timestamp = 0;
            for (iter=1; iter<2; iter++)
            {
                if ( iter == 0 )
                {
                    if ( (fp= fopen(os_compatible_path(fname),"rb")) == 0 )
                        continue;
                    fread(&timestamp,1,sizeof(timestamp),fp);
                    jsonstr = _ram_loadfp(fp);
                    fclose(fp);
                }
                else
                {
                    sprintf(cmd,"requestType=getAccountTransactions&account=%s&timestamp=%u",ram->special_NXTaddrs[j],timestamp);
                    jsonstr = _issue_NXTPOST(cmd);
                    if ( fp == 0 && (fp= fopen(os_compatible_path(fname),"wb")) != 0 )
                    {
                        fwrite(&oldest,1,sizeof(oldest),fp);
                        fwrite(jsonstr,1,strlen(jsonstr)+1,fp);
                        fclose(fp);
                    }
                }
                if ( jsonstr != 0 )
                {
                    //printf("special.%d (%s) (%s)\n",j,ram->special_NXTaddrs[j],jsonstr);
                    if ( (redemptions= cJSON_Parse(jsonstr)) != 0 )
                    {
                        if ( (array= cJSON_GetObjectItem(redemptions,"transactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
                        {
                            for (i=0; i<n; i++)
                            {
                                //fprintf(stderr,".");
                                if ( (height= _process_NXTtransaction(1,ram,cJSON_GetArrayItem(array,i))) != 0 && height > mostrecent )
                                    mostrecent = height;
                            }
                        }
                        free_json(redemptions);
                    }
                    free(jsonstr);
                }
            }
        }
        sprintf(fname,"%s/ramchains/NXTasset.%llu",MGWROOT,(long long)ap->assetbits);
        fp = 0;
        if ( 1 || (fp= fopen(os_compatible_path(fname),"rb")) == 0 )
        {
            sprintf(cmd,"requestType=getAssetTransfers&asset=%llu",(long long)ap->assetbits);
            jsonstr = _issue_NXTPOST(cmd);
        } else jsonstr = _ram_loadfp(fp), fclose(fp);
        if ( jsonstr != 0 )
        {
            if ( fp == 0 && (fp= fopen(os_compatible_path(fname),"wb")) != 0 )
            {
                fwrite(jsonstr,1,strlen(jsonstr)+1,fp);
                fclose(fp);
            }
            if ( (transfers = cJSON_Parse(jsonstr)) != 0 )
            {
                if ( (array= cJSON_GetObjectItem(transfers,"transfers")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
                {
                    for (i=0; i<n; i++)
                    {
                        //fprintf(stderr,"t");
                        if ( (i % 10) == 9 )
                            fprintf(stderr,"%.1f%% ",100. * (double)i / n);
                        copy_cJSON(txid,cJSON_GetObjectItem(cJSON_GetArrayItem(array,i),"assetTransfer"));
                        if ( txid[0] != 0 && (txidjsonstr= _issue_getTransaction(txid)) != 0 )
                        {
                            if ( (json= cJSON_Parse(txidjsonstr)) != 0 )
                            {
                                if ( (height= _process_NXTtransaction(1,ram,json)) != 0 && height > mostrecent )
                                    mostrecent = height;
                                free_json(json);
                            }
                            free(txidjsonstr);
                        }
                    }
                }
                free_json(transfers);
            }
            free(jsonstr);
        }
    }
    ram->S.circulation = _calc_circulation(ram->min_NXTconfirms,ram->ap,ram);
    if ( ram->S.is_realtime != 0 )
        ram->S.orphans = _find_pending_transfers(&ram->S.MGWpendingredeems,ram);
    //printf("return mostrecent.%d\n",mostrecent);
    return(mostrecent);
}

// >>>>>>>>>>>>>>  start bitstream functions
static uint8_t huffmasks[8] = { (1<<0), (1<<1), (1<<2), (1<<3), (1<<4), (1<<5), (1<<6), (1<<7) };
static uint8_t huffoppomasks[8] = { ~(1<<0), ~(1<<1), ~(1<<2), ~(1<<3), ~(1<<4), ~(1<<5), ~(1<<6), (uint8_t)~(1<<7) };

void hupdate_internals(HUFF *hp)
{
    if ( hp->bitoffset > hp->endpos )
        hp->endpos = hp->bitoffset;
    hp->ptr = &hp->buf[hp->bitoffset >> 3];
    hp->maski = (hp->bitoffset & 7);
}

void hrewind(HUFF *hp)
{
    hp->bitoffset = 0;
    hupdate_internals(hp);
}

void hclear(HUFF *hp)
{
    hp->bitoffset = 0;
    hupdate_internals(hp);
    //memset(hp->buf,0,hp->allocsize);
    hp->endpos = 0;
}

int32_t hgetbit(HUFF *hp)
{
    int32_t bit = 0;
    if ( hp->bitoffset <= hp->endpos )
    {
        if ( (*hp->ptr & huffmasks[hp->maski++]) != 0 )
            bit = 1;
        hp->bitoffset++;
        if ( hp->maski == 8 )
        {
            hp->maski = 0;
            hp->ptr++;
        }
        return(bit);
    }
    printf("hgetbit past EOF: %d >= %d\n",hp->bitoffset,hp->endpos); while ( 1 ) portable_sleep(1);
    return(-1);
}

int32_t hputbit(HUFF *hp,int32_t bit)
{
    if ( bit != 0 )
        *hp->ptr |= huffmasks[hp->maski];
    else *hp->ptr &= huffoppomasks[hp->maski];
    if ( ++hp->maski >= 8 )
    {
        hp->maski = 0;
        hp->ptr++;
    }
    if ( ++hp->bitoffset > hp->endpos )
        hp->endpos = hp->bitoffset;
    if ( (hp->bitoffset>>3) > hp->allocsize )
    {
        printf("hwrite: bitoffset.%d >= allocsize.%d\n",hp->bitoffset,hp->allocsize);
        hupdate_internals(hp);
        return(-1);
    }
    return(0);
}

int32_t hcalc_bitsize(uint32_t x)
{
    uint32_t mask = (1 << 31);
    int32_t i;
    if ( x == 0 )
        return(1);
    for (i=31; i>=0; i--,mask>>=1)
    {
        if ( (mask & x) != 0 )
            return(i+1);
    }
    return(-1);
}

int32_t hemit_bits(HUFF *hp,uint32_t val,int32_t numbits)
{
    int32_t hputbit(HUFF *hp,int32_t bit);
    int32_t i;
    for (i=0; i<numbits; i++)
        hputbit(hp,(val & (1<<i)) != 0);
    return(numbits);
}

int32_t hemit_longbits(HUFF *hp,uint64_t val)
{
    int32_t hputbit(HUFF *hp,int32_t bit);
    int32_t i;
    for (i=0; i<64; i++)
        hputbit(hp,(val & (1<<i)) != 0);
    return(64);
}

int32_t hdecode_bits(uint32_t *valp,HUFF *hp,int32_t numbits)
{
    uint32_t i,val;
    for (i=val=0; i<numbits; i++)
        if ( hgetbit(hp) != 0 )
            val |= (1 << i);
    *valp = val;
    // printf("{%d} ",val);
    return(numbits);
}

int32_t hemit_smallbits(HUFF *hp,uint16_t val)
{
    static long sum,count;
    int32_t numbits = 0;
    //printf("smallbits.(%d)\n",val);
    if ( val >= 4 )
    {
        if ( val < (1 << 5) )
        {
            numbits += hemit_bits(hp,4,3);
            numbits += hemit_bits(hp,val,5);
        }
        else if ( val < (1 << 8) )
        {
            numbits += hemit_bits(hp,5,3);
            numbits += hemit_bits(hp,val,8);
        }
        else if ( val < (1 << 12) )
        {
            numbits += hemit_bits(hp,6,3);
            numbits += hemit_bits(hp,val,12);
        }
        else
        {
            numbits += hemit_bits(hp,7,3);
            numbits += hemit_bits(hp,val,16);
        }
    } else numbits += hemit_bits(hp,val,3);
    count++, sum += numbits;
    if ( (count % 10000000) == 0 )
        printf("ave smallbits %.1f after %ld samples\n",(double)sum/count,count);
    return(numbits);
}

int32_t hdecode_smallbits(uint16_t *valp,HUFF *hp)
{
    uint32_t s;
    int32_t val,numbits = 3;
    val = (hgetbit(hp) | (hgetbit(hp) << 1) | (hgetbit(hp) << 2));
    if ( val < 4 )
        *valp = val;
    else
    {
        if ( val == 4 )
            numbits += hdecode_bits(&s,hp,5);
        else if ( val == 5 )
            numbits += hdecode_bits(&s,hp,8);
        else if ( val == 6 )
            numbits += hdecode_bits(&s,hp,12);
        else numbits += hdecode_bits(&s,hp,16);
        *valp = s;
    }
    return(numbits);
}

int32_t hemit_varbits(HUFF *hp,uint32_t val)
{
    static long sum,count;
    int32_t numbits = 0;
    if ( val >= 10 )
    {
        if ( val < (1 << 8) )
        {
            numbits += hemit_bits(hp,10,4);
            numbits += hemit_bits(hp,val,8);
        }
        else if ( val < (1 << 10) )
        {
            numbits += hemit_bits(hp,11,4);
            numbits += hemit_bits(hp,val,10);
        }
        else if ( val < (1 << 13) )
        {
            numbits += hemit_bits(hp,12,4);
            numbits += hemit_bits(hp,val,13);
        }
        else if ( val < (1 << 16) )
        {
            numbits += hemit_bits(hp,13,4);
            numbits += hemit_bits(hp,val,16);
        }
        else if ( val < (1 << 20) )
        {
            numbits += hemit_bits(hp,14,4);
            numbits += hemit_bits(hp,val,20);
        }
        else
        {
            numbits += hemit_bits(hp,15,4);
            numbits += hemit_bits(hp,val,32);
        }
    } else numbits += hemit_bits(hp,val,4);
    count++, sum += numbits;
    if ( (count % 10000000) == 0 )
        printf("hemit_varbits.(%u) numbits.%d ave varbits %.1f after %ld samples\n",val,numbits,(double)sum/count,count);
    return(numbits);
}

int32_t hdecode_varbits(uint32_t *valp,HUFF *hp)
{
    int32_t val;
    val = (hgetbit(hp) | (hgetbit(hp) << 1) | (hgetbit(hp) << 2) | (hgetbit(hp) << 3));
    if ( val < 10 )
    {
        *valp = val;
        return(4);
    }
    else if ( val == 10 )
        return(4 + hdecode_bits(valp,hp,8));
    else if ( val == 11 )
        return(4 + hdecode_bits(valp,hp,10));
    else if ( val == 12 )
        return(4 + hdecode_bits(valp,hp,13));
    else if ( val == 13 )
        return(4 + hdecode_bits(valp,hp,16));
    else if ( val == 14 )
        return(4 + hdecode_bits(valp,hp,20));
    else return(4 + hdecode_bits(valp,hp,32));
}

int32_t hemit_valuebits(HUFF *hp,uint64_t value)
{
    static long sum,count;
    int32_t i,numbits = 0;
    for (i=0; i<2; i++,value>>=32)
        numbits += hemit_varbits(hp,value & 0xffffffff);
    count++, sum += numbits;
    if ( (count % 10000000) == 0 )
        printf("ave valuebits %.1f after %ld samples\n",(double)sum/count,count);
    return(numbits);
}

int32_t hdecode_valuebits(uint64_t *valuep,HUFF *hp)
{
    uint32_t i,tmp[2],numbits = 0;
    for (i=0; i<2; i++)
        numbits += hdecode_varbits(&tmp[i],hp);//, printf("($%x).%d ",tmp[i],i);
    *valuep = (tmp[0] | ((uint64_t)tmp[1] << 32));
    // printf("[%llx] ",(long long)(*valuep));
    return(numbits);
}

// HUFF bitstream functions, uses model similar to fopen/fread/fwrite/fseek but for bitstream

void hclose(HUFF *hp)
{
    if ( hp != 0 )
    {
        if ( hp->allocated != 0 )
            free(hp->buf);
        //if ( hp != 0 )
        //    free(hp);
    }
}

void hpurge(HUFF *hps[],int32_t num)
{
    int32_t i;
    for (i=0; i<num; i++)
        if ( hps[i] != 0 )
            hclose(hps[i]), hps[i] = 0;
}

HUFF *hopen(char *coinstr,struct alloc_space *mem,uint8_t *bits,int32_t num,int32_t allocated)
{
    HUFF *hp = (MAP_HUFF != 0) ? permalloc(coinstr,mem,sizeof(*hp),1) : calloc(1,sizeof(*hp));
    hp->ptr = hp->buf = bits;
    hp->allocsize = num;
    hp->endpos = (num << 3);
    if ( allocated != 0 )
        hp->allocated = 1;
    return(hp);
}

int32_t hseek(HUFF *hp,int32_t offset,int32_t mode)
{
    if ( mode == SEEK_END )
        offset += hp->endpos;
    if ( offset >= 0 && (offset>>3) <= hp->allocsize )
        hp->bitoffset = offset, hupdate_internals(hp);
    else
    {
        printf("hseek.%d: illegal offset.%d %d >= allocsize.%d\n",mode,offset,offset>>3,hp->allocsize);
        return(-1);
    }
    return(0);
}

int32_t hmemcpy(void *dest,void *src,HUFF *hp,int32_t datalen)
{
    if ( (hp->bitoffset & 7) != 0 || ((hp->bitoffset>>3) + datalen) > hp->allocsize )
    {
        printf("misaligned hmemcpy bitoffset.%d or overflow allocsize %d vs %d\n",hp->bitoffset,hp->allocsize,((hp->bitoffset>>3) + datalen));
        while ( 1 ) portable_sleep(1);
        return(-1);
    }
    if ( dest != 0 && src == 0 )
        memcpy(dest,hp->ptr,datalen);
    else if ( dest == 0 && src != 0 )
        memcpy(hp->ptr,src,datalen);
    else
    {
        printf("invalid hmemcpy with both dest.%p and src.%p\n",dest,src);
        return(-1);
    }
    hp->ptr += datalen;
    hp->bitoffset += (datalen << 3);
    if ( hp->bitoffset > hp->endpos )
        hp->endpos = hp->bitoffset;
    return(datalen);
}

int32_t hwrite(uint64_t codebits,int32_t numbits,HUFF *hp)
{
    int32_t i;
    for (i=0; i<numbits; i++,codebits>>=1)
    {
        if ( hputbit(hp,codebits & 1) < 0 )
            return(-1);
    }
    return(numbits);
}

int32_t hconv_bitlen(uint64_t bitlen)
{
    int32_t len;
    len = (int32_t)(bitlen >> 3);
    if ( ((int32_t)bitlen & 7) != 0 )
        len++;
    return(len);
}

int32_t hflush(FILE *fp,HUFF *hp)
{
    uint32_t len;
    int32_t numbytes = 0;
    if ( (numbytes= (int32_t)hemit_varint(fp,hp->endpos)) < 0 )
        return(-1);
    len = hconv_bitlen(hp->endpos);
    if ( fwrite(hp->buf,1,len,fp) != len )
        return(-1);
    fflush(fp);
    //printf("HFLUSH len.%d numbytes.%d | fpos.%ld\n",len,numbytes,ftell(fp));
    return(numbytes + len);
}

int32_t hsync(FILE *fp,HUFF *hp,void *buf)
{
    static uint32_t counter;
    uint32_t len;
    int32_t numbytes = 0;
    uint64_t endbitpos;
    long fpos = ftell(fp);
    if ( hload_varint(&endbitpos,fp) > 0 )
    {
        len = hconv_bitlen(endbitpos);
        if ( len < 1024*1024 && fread(buf,1,len,fp) == len )
        {
            if ( memcmp(buf,hp->buf,len) == 0 )
                return(0);
            else if ( counter++ < 10 ) printf("hsync data mismatch\n");
        }
    }
    fseek(fp,fpos,SEEK_SET);
    if ( (numbytes= (int32_t)hemit_varint(fp,hp->endpos)) < 0 )
        return(-1);
    len = hconv_bitlen(hp->endpos);
    if ( fwrite(hp->buf,1,len,fp) != len )
        return(-1);
    //fflush(fp);
    return(numbytes + len);
}

HUFF *hload(struct ramchain_info *ram,long *offsetp,FILE *fp,char *fname)
{
    //long hload_varint(uint64_t *x,FILE *fp);
    uint64_t endbitpos;
    int32_t len,flag = 0;
    uint8_t *buf;
    HUFF *hp = 0;
    if ( offsetp != 0 )
        *offsetp = -1;
    if ( fp == 0 )
        fp = fopen(os_compatible_path(fname),"rb"), flag = 1;
    if ( hload_varint(&endbitpos,fp) > 0 )
    {
        len = hconv_bitlen(endbitpos);
        if ( offsetp != 0 )
        {
            *offsetp = ftell(fp);
            buf = ram->tmphp->buf;
        }
        else buf = permalloc(ram->name,&ram->Perm,len,2);
        if ( fread(buf,1,len,fp) != len )
            fprintf(stderr,"HLOAD error (%s) endbitpos.%d len.%d | fpos.%ld\n",fname!=0?fname:ram->name,(int)endbitpos,len,ftell(fp));
        else hp = hopen(ram->name,&ram->Perm,buf,len,0), hp->endpos = (int32_t)endbitpos;
    }
    if ( flag != 0 && fp != 0 )
        fclose(fp);
    return(hp);
}

// >>>>>>>>>>>>>>  start huffman functions
uint64_t huff_reversebits(uint64_t x,int32_t n)
{
    uint64_t rev = 0;
    int32_t i = 0;
    while ( n > 0 )
    {
        if ( GETBIT((void *)&x,n-1) != 0 )
            SETBIT(&rev,i);
        i++;
        n--;
    }
    return(rev);
}

uint64_t huff_convstr(char *str)
{
    uint64_t mask,codebits = 0;
    long n = strlen(str);
    mask = (1 << (n-1));
    while ( n > 0 )
    {
        if ( str[n-1] != '0' )
            codebits |= mask;
        //printf("(%c %llx m%x) ",str[n-1],(long long)codebits,(int)mask);
        mask >>= 1;
        n--;
    }
    //printf("(%s -> %llx)\n",str,(long long)codebits);
    return(codebits);
}

char *huff_str(uint64_t codebits,int32_t n)
{
    static char str[128];
    uint64_t mask = 1;
    int32_t i;
    for (i=0; i<n; i++,mask<<=1)
        str[i] = ((codebits & mask) != 0) + '0';
    str[i] = 0;
    return(str);
}

int32_t huff_dispitem(int32_t dispflag,const struct huffbits *item,int32_t frequi)
{
    int32_t n = item->numbits;
    uint64_t codebits;
    if ( n != 0 )
    {
        if ( dispflag != 0 )
        {
            codebits = huff_reversebits(item->bits,n);
            printf("%06d: freq.%-6d (%8s).%d\n",item->rawind,item->freq,huff_str(codebits,n),n);
        }
        return(1);
    }
    return(0);
}

void huff_disp(int32_t verboseflag,struct huffcode *huff,struct huffbits *items,int32_t frequi)
{
    int32_t i,n = 0;
    if ( huff->maxbits >= 1024 )
        printf("huff->maxbits %d >= %d sizeof(strbit)\n",huff->maxbits,1024);
    for (i=0; i<huff->numitems; i++)
        n += huff_dispitem(verboseflag,&items[i],frequi);
    fprintf(stderr,"n.%d huffnodes.%d bytes.%.1f -> bits.%.1f compression ratio %.3f ave %.1f bits/item\n",n,huff->numnodes,huff->totalbytes,huff->totalbits,((double)huff->totalbytes*8)/huff->totalbits,huff->totalbits/huff->freqsum);
}

// huff bitcode calculation functions

struct huffheap { uint32_t *f,*h,n,s,cs; };
struct huffheap *huff_heap_create(uint32_t s,uint32_t *f)
{
    struct huffheap *h;
    h = malloc(sizeof(struct huffheap));
    h->h = malloc(sizeof(*h->h) * s);
    // printf("_heap_create heap.%p h.%p s.%d\n",h,h->h,s);
    h->s = h->cs = s;
    h->n = 0;
    h->f = f;
    return(h);
}

void huff_heap_destroy(struct huffheap *heap)
{
    free(heap->h);
    free(heap);
}

#define huffswap_(I,J) do { int t_; t_ = a[(I)];	\
a[(I)] = a[(J)]; a[(J)] = t_; } while(0)
void huff_heap_sort(struct huffheap *heap)
{
    uint32_t i=1,j=2; // gnome sort
    uint32_t *a = heap->h;
    while ( i < heap->n ) // smaller values are kept at the end
    {
        if ( heap->f[a[i-1]] >= heap->f[a[i]] )
            i = j, j++;
        else
        {
            huffswap_(i-1, i);
            i--;
            i = (i == 0) ? j++ : i;
        }
    }
}
#undef huffswap_

void huff_heap_add(struct huffheap *heap,uint32_t ind)
{
    //printf("add to heap ind.%d n.%d s.%d\n",ind,heap->n,heap->s);
    if ( (heap->n + 1) > heap->s )
    {
        heap->h = realloc(heap->h,heap->s + heap->cs);
        heap->s += heap->cs;
    }
    heap->h[heap->n++] = ind;
    huff_heap_sort(heap);
}

int32_t huff_heap_remove(struct huffheap *heap)
{
    if ( heap->n > 0 )
        return(heap->h[--heap->n]);
    return(-1);
}

int32_t huff_init(double *freqsump,int32_t **predsp,uint32_t **revtreep,struct huffbits **items,int32_t numitems,int32_t frequi)
{
    int32_t depth,r1,r2,i,n,*preds,extf = numitems;
    struct huffheap *heap;
    double freqsum = 0.;
    uint32_t *efreqs,*revtree;
    *predsp = 0;
    *revtreep = 0;
    *freqsump = 0.;
    if ( numitems <= 0 )
        return(0);
    efreqs = calloc(2 * numitems,sizeof(*efreqs));
    for (i=0; i<numitems; i++)
        efreqs[i] = items[i]->freq;// * ((items[i]->wt != 0) ? items[i]->wt : 1);
    if ( (heap= huff_heap_create(numitems*2,efreqs)) == NULL )
    {
        printf("error _heap_create for numitems.%d\n",numitems);
        free(efreqs);
        return(0);
    }
    preds = calloc(2 * numitems,sizeof(*preds));
    revtree = calloc(2 * numitems,sizeof(*revtree));
    // printf("heap.%p h.%p s.%d numitems.%d frequi.%d\n",heap,heap->h,heap->s,numitems,frequi);
    for (i=n=0; i<numitems; i++)
        if ( efreqs[i] > 0 )
        {
            n++;
            freqsum += items[i]->freq;
            huff_heap_add(heap,i);
        }
    //printf("added n.%d items: freqsum %.0f\n",n,freqsum);
    *freqsump = freqsum;
    depth = 0;
    while ( heap->n > 1 )
    {
        r1 = huff_heap_remove(heap);
        r2 = huff_heap_remove(heap);
        efreqs[extf] = (efreqs[r1] + efreqs[r2]);
        revtree[(depth << 1) + 1] = r1;
        revtree[(depth << 1) + 0] = r2;
        depth++;
        huff_heap_add(heap,extf);
        preds[r1] = extf;
        preds[r2] = -extf;
        //printf("n.%d: r1.%d (%d) <- %d | r2.%d (%d) <- %d\n",heap->n,r1,efreqs[r1],extf,r2,efreqs[r2],-extf);
        extf++;
    }
    r1 = huff_heap_remove(heap);
    preds[r1] = r1;
    huff_heap_destroy(heap);
    free(efreqs);
    *predsp = preds;
    *revtreep = revtree;
    return(extf);
}

int32_t huff_calc(int32_t dispflag,int32_t *nonzp,double *totalbytesp,double *totalbitsp,int32_t *preds,struct huffbits **items,int32_t numitems,int32_t frequi,int32_t wt)
{
    int32_t i,ix,pred,numbits,nonz,maxbits = 0;
    double totalbits,totalbytes;
    struct huffbits *item;
    uint64_t codebits;
    totalbits = totalbytes = 0.;
    for (i=nonz=0; i<numitems; i++)
    {
        codebits = numbits = 0;
        if ( items[i]->freq != 0 )
        {
            ix = i;
            pred = preds[ix];
            while ( pred != ix && -pred != ix )
            {
                if ( pred >= 0 )
                {
                    codebits |= (1LL << numbits);
                    ix = pred;
                }
                else ix = -pred;
                pred = preds[ix];
                numbits++;
            }
            if ( numbits > maxbits )
                maxbits = numbits;
            nonz++;
            if ( (item= items[i]) != 0 )
            {
                //item->codebits = huff_reversebits(codebits,numbits);
                //item->numbits = numbits;
                item->numbits = numbits;
                item->bits = huff_reversebits(codebits,numbits);
                //item->code.rawind = item->rawind;
                if ( numbits > 32 || item->bits != huff_reversebits(codebits,numbits) )
                    printf("error setting code.bits: item->code.numbits %d != %d numbits || item->code.bits %u != %llu _reversebits(codebits,numbits) || item->code.rawind %d != %d item->rawind\n",item->numbits,numbits,item->bits,(long long)huff_reversebits(codebits,numbits),item->rawind,item->rawind);
                totalbits += numbits * item->freq;
                totalbytes += wt * item->freq;
                if ( dispflag != 0 )
                {
                    printf("Total %.0f -> %.0f: ratio %.3f ave %.1f bits/item |",totalbytes,totalbits,totalbytes*8/(totalbits+1),totalbits/nonz);
                    huff_dispitem(dispflag,items[i],frequi);
                }
            } else printf("FATAL: null item ptr.%d\n",i);
        }
    }
    *nonzp = nonz; *totalbitsp = totalbits; *totalbytesp = totalbytes;
    return(maxbits);
}

#ifdef HUFF_TESTDECODE
int32_t huff_decodetest(uint32_t *indp,int32_t *tree,int32_t depth,int32_t val,int32_t numitems)
{
    int32_t i,ind;
    *indp = -1;
    for (i=0; i<depth; )
    {
        if ( (ind= tree[((1 << i++) & val) != 0]) < 0 )
        {
            tree -= ind;
            continue;
        }
        *indp = ind;
        if ( ind >= numitems )
            printf("decode error: val.%x -> ind.%d >= numitems %d\n",val,ind,numitems);
        return(i);
    }
    printf("decodetest error: val.%d\n",val);
    return(-1);
}
#endif

// high level functions

void huff_free(struct huffcode *huff) { free(huff); }

struct huffcode *huff_create(int32_t dispflag,struct huffbits **items,int32_t numitems,int32_t frequi,int32_t wt)
{
    int32_t ix2,i,n,extf,*preds,allocsize;
    uint32_t *revtree;
    double freqsum;
    struct huffcode *huff;
    if ( (extf= huff_init(&freqsum,&preds,&revtree,items,numitems,frequi)) <= 0 )
        return(0);
    allocsize = (int32_t)(sizeof(*huff) + (sizeof(*huff->tree) * 2 * extf));
    huff = calloc(1,allocsize);
    //huff->items = items;
    //huff->V = V;
    huff->allocsize = allocsize;
    huff->numitems = numitems;
    huff->depth = (extf - numitems);
    huff->freqsum = freqsum;
    n = 0;
    for (i=huff->depth-1; i>=0; i--)
    {
        ix2 = (i << 1);
        //printf("%d.[%d:%d%s %d:%d%s] ",numitems+i,-(numitems+i-revtree[i][0])*2,revtree[i][0],revtree[i][0] < numitems ? ".*" : "  ",-(numitems+i-revtree[i][1])*2,revtree[i][1],revtree[i][1] < numitems ? ".*" : "  ");
        huff->tree[n<<1] = (revtree[ix2+0] < numitems) ? revtree[ix2+0] : -((numitems+i-revtree[ix2+0]) << 1);
        huff->tree[(n<<1) + 1] = (revtree[ix2+1] < numitems) ? revtree[ix2+1] : -((numitems+i-revtree[ix2+1]) << 1);
        n++;
    }
    free(revtree);
#ifdef HUFF_TESTDECODE
    for (i=0; i<8; i++)
    {
        int32_t itemi;
        n = huff_decodetest(&itemi,tree,depth,i,numitems);
        printf("i.%d %s -> %d bits itemi.%d\n",i,huff_str(i,n),n,itemi);
    }
#endif
    huff->maxbits = huff_calc(dispflag,&huff->numnodes,&huff->totalbytes,&huff->totalbits,preds,items,numitems,frequi,wt);
    free(preds);
    return(huff);
}

int32_t huff_encode(struct huffcode *huff,HUFF *hp,struct huffitem *items,uint32_t *itemis,int32_t num)
{
    uint64_t codebits;
    struct huffitem *item;
    int32_t i,j,n,count = 0;
	for (i=0; i<n; i++)
    {
        item = &items[itemis[i]];
        codebits = item->code.bits;
        n = item->code.numbits;
        for (j=0; j<n; j++,codebits>>=1)
            hputbit(hp,codebits & 1);
        count += n;
	}
    return(count);
}

int32_t huff_itemsave(FILE *fp,struct huffbits *item)
{
    if ( fwrite(item,1,sizeof(*item),fp) != sizeof(uint64_t) )
        return(-1);
    return((int32_t)sizeof(uint64_t));
}

int32_t huff_save(FILE *fp,struct huffcode *code)
{
    if ( code != 0 )
    {
        if ( fwrite(code,1,code->allocsize,fp) != code->allocsize )
            return(-1);
        return(code->allocsize);
    }
    return(-1);
}

// >>>>>>>>>>>>>>  start huffpair functions, eg. combined items and huffcodes
int32_t huffpair_save(FILE *fp,struct huffpair *pair)
{
    int32_t i;
    long val,n = 0;
    if ( fwrite(pair,1,sizeof(*pair),fp) != sizeof(*pair) )
        return(-1);
    n = sizeof(*pair);
    for (i=0; i<=pair->maxind; i++)
    {
        if ( pair->items[i].freq != 0 )
        {
            if ( (val= huff_itemsave(fp,&pair->items[i])) <= 0 )
                return(-1);
            n += val;
        }
    }
    if ( (val= huff_save(fp,pair->code)) <= 0 )
        return(-1);
    return((int32_t)(n+val));
}

void huff_compressionvars_fname(int32_t readonly,char *fname,char *coinstr,char *typestr,int32_t subgroup)
{
    char *dirname = MGWROOT;
    if ( subgroup < 0 )
        sprintf(fname,"%s/ramchains/%s/%s.%s",dirname,coinstr,coinstr,typestr);
    else sprintf(fname,"%s/ramchains/%s/%s/%s.%d",dirname,coinstr,typestr,coinstr,subgroup);
}

int32_t huffpair_gencode(struct ramchain_info *ram,struct huffpair *pair,int32_t frequi)
{
    struct huffbits **items;
    int32_t i,n;
    char fname[1024];
    if ( pair->code == 0 && pair->items != 0 )// && pair->nonz > 0 )
    {
        //fprintf(stderr,"%s n.%d: ",pair->name,pair->nonz+1);
        items = calloc(pair->nonz+1,sizeof(*items));
        for (i=n=0; i<=pair->maxind; i++)
            if ( pair->items[i].freq != 0 )
                items[n++] = &pair->items[i];
        //fprintf(stderr,"%s n.%d: ",pair->name,n);
        pair->code = huff_create(0,items,n,frequi,pair->wt);
        huff_compressionvars_fname(0,fname,ram->name,pair->name,-1);
        if ( pair->code != 0 )//&& (fp= fopen(os_compatible_path(fname),"wb")) != 0 )
        {
            ram->totalbits += pair->code->totalbits;
            ram->totalbytes += pair->code->totalbytes;
            //if ( huffpair_save(fp,pair) <= 0 )
            //    printf("huffpair_gencode: error saving %s\n",fname);
            //else fprintf(stderr,"%s saved ",fname);
            //fclose(fp);
        } else printf("error creating (%s)\n",fname);
        //huff_disp(0,pair->code,pair->items,frequi);
        free(items);
        huff_free(pair->code);
        pair->code = 0;
    }
    return(0);
}

void huffpair_update(char type,struct ramchain_info *ram,char *str,char *name,struct huffpair *pair,uint64_t val,int32_t size)
{
    if ( type == 'H' )
    {
        if ( pair->name[0] == 0 )
        {
            sprintf(pair->name,"%s_%s",str,name);
            pair->wt = size;
        }
        if ( pair->minval == 0 || val < pair->minval )
            pair->minval = val;
        if ( pair->maxval == 0 || val > pair->maxval )
            pair->maxval = val;
        if ( size == 4 )
        {
            if ( ram->minval4 == 0 || val < ram->minval4 )
                ram->minval4 = val;
            if ( ram->maxval4 == 0 || val > ram->maxval4 )
                ram->maxval4 = val;
        }
        else if ( size == 2 )
        {
            if ( ram->minval2 == 0 || val < ram->minval2 )
                ram->minval2 = val;
            if ( ram->maxval2 == 0 || val > ram->maxval2 )
                ram->maxval2 = val;
        }
        else if ( size == 8 )
        {
            if ( ram->minval8 == 0 || val < ram->minval8 )
                ram->minval8 = val;
            if ( ram->maxval8 == 0 || val > ram->maxval8 )
                ram->maxval8 = val;
        }
    }
    else if ( size != 8 )
    {
        if ( pair->items == 0 )
        {
            pair->maxind = pair->nonz = (int32_t)(pair->maxval - pair->minval + 1);
            pair->items = calloc(size==4?pair->maxind+2:(1<<16),sizeof(*pair->items));
        }
        if ( val == 0 )
            pair->items[0].freq++;
        else
        {
            pair->items[val - pair->minval].freq++;
            //printf("%s: %llu maxval.%llu | freq.%d\n",pair->name,(long long)val,(long long)pair->maxval,pair->items[val-pair->minval].freq);
        }
    }
}

int32_t huffpair_gentxcode(struct ramchain_info *V,struct rawtx_huffs *tx,int32_t frequi)
{
    if ( huffpair_gencode(V,&tx->numvins,frequi) != 0 )
        return(-1);
    else if ( huffpair_gencode(V,&tx->numvouts,frequi) != 0 )
        return(-2);
    else if ( huffpair_gencode(V,&tx->txid,frequi) != 0 )
        return(-3);
    else return(0);
}

int32_t huffpair_genvincode(struct ramchain_info *V,struct rawvin_huffs *vin,int32_t frequi)
{
    if ( huffpair_gencode(V,&vin->txid,frequi) != 0 )
        return(-1);
    else if ( huffpair_gencode(V,&vin->vout,frequi) != 0 )
        return(-2);
    else return(0);
}

int32_t huffpair_genvoutcode(struct ramchain_info *V,struct rawvout_huffs *vout,int32_t frequi)
{
    if ( huffpair_gencode(V,&vout->addr,frequi) != 0 )
        return(-1);
    else if ( huffpair_gencode(V,&vout->script,frequi) != 0 )
        return(-2);
    return(0);
}

int32_t huffpair_gencodes(struct ramchain_info *V,struct rawblock_huffs *H,int32_t frequi)
{
    int32_t err;
    //if ( huffpair_gencode(V,&H->numall,frequi) != 0 )
    //    return(-1);
    //else
    if ( huffpair_gencode(V,&H->numtx,frequi) != 0 )
        return(-2);
    //else if ( huffpair_gencode(V,&H->numrawvins,frequi) != 0 )
    //    return(-3);
    //else if ( huffpair_gencode(V,&H->numrawvouts,frequi) != 0 )
    //    return(-4);
    //else if ( (err= huffpair_gentxcode(V,&H->txall,frequi)) != 0 )
    //    return(-100 + err);
    else if ( (err= huffpair_gentxcode(V,&H->tx0,frequi)) != 0 )
        return(-110 + err);
    else if ( (err= huffpair_gentxcode(V,&H->tx1,frequi)) != 0 )
        return(-120 + err);
    else if ( (err= huffpair_gentxcode(V,&H->txi,frequi)) != 0 )
        return(-130 + err);
    // else if ( (err= huffpair_genvincode(V,&H->vinall,frequi)) != 0 )
    //    return(-200 + err);
    else if ( (err= huffpair_genvincode(V,&H->vin0,frequi)) != 0 )
        return(-210 + err);
    else if ( (err= huffpair_genvincode(V,&H->vin1,frequi)) != 0 )
        return(-220 + err);
    else if ( (err= huffpair_genvincode(V,&H->vini,frequi)) != 0 )
        return(-230 + err);
    //else if ( (err= huffpair_genvoutcode(V,&H->voutall,frequi)) != 0 )
    //    return(-300 + err);
    else if ( (err= huffpair_genvoutcode(V,&H->vout0,frequi)) != 0 )
        return(-310 + err);
    else if ( (err= huffpair_genvoutcode(V,&H->vout1,frequi)) != 0 )
        return(-320 + err);
    else if ( (err= huffpair_genvoutcode(V,&H->vouti,frequi)) != 0 )
        return(-330 + err);
    else if ( (err= huffpair_genvoutcode(V,&H->vout2,frequi)) != 0 )
        return(-330 + err);
    else if ( (err= huffpair_genvoutcode(V,&H->voutn,frequi)) != 0 )
        return(-330 + err);
    return(0);
}

int32_t huffpair_set_preds(struct rawblock_preds *preds,struct rawblock_huffs *H,int32_t allmode)
{
    memset(preds,0,sizeof(*preds));
    if ( allmode != 0 )
    {
        if ( (preds->numtx= &H->numall) == 0 )
            return(-1);
        else preds->numrawvins = preds->numrawvouts = preds->numtx;
        
        if ( (preds->tx0_numvins= &H->txall.numvins) == 0 )
            return(-2);
        else preds->tx1_numvins = preds->txi_numvins = preds->tx0_numvins;
        if ( (preds->tx0_numvouts= &H->txall.numvouts) == 0 )
            return(-3);
        else preds->tx1_numvouts = preds->txi_numvouts = preds->tx0_numvouts;
        if ( (preds->tx0_txid= &H->txall.txid) == 0 )
            return(-3);
        else preds->tx1_txid = preds->txi_txid = preds->tx0_txid;
        
        if ( (preds->vin0_txid= &H->vinall.txid) == 0 )
            return(-4);
        else preds->vini_txid = preds->vin0_txid;
        if ( (preds->vin0_vout= &H->vinall.vout) == 0 )
            return(-5);
        else preds->vini_vout = preds->vin0_vout;
        
        if ( (preds->vout0_addr= &H->voutall.addr) == 0 )
            return(-6);
        else preds->vout1_addr = preds->vout2_addr = preds->vouti_addr = preds->voutn_addr = preds->vout0_addr;
        if ( (preds->vout0_script= &H->voutall.script) == 0 )
            return(-7);
        else preds->vout1_script = preds->vout2_script = preds->vouti_script = preds->voutn_script = preds->vout0_script;
    }
    else
    {
        if ( (preds->numtx= &H->numtx) == 0 )
            return(-1);
        else if ( (preds->numrawvouts= &H->numrawvouts) == 0 )
            return(-2);
        else if ( (preds->numrawvins= &H->numrawvins) == 0 )
            return(-3);
        
        else if ( (preds->tx0_numvins= &H->tx0.numvins) == 0 )
            return(-4);
        else if ( (preds->tx1_numvins= &H->tx1.numvins) == 0 )
            return(-5);
        else if ( (preds->txi_numvins= &H->txi.numvins) == 0 )
            return(-6);
        else if ( (preds->tx0_numvouts= &H->tx0.numvouts) == 0 )
            return(-7);
        else if ( (preds->tx1_numvouts= &H->tx1.numvouts) == 0 )
            return(-8);
        else if ( (preds->txi_numvouts= &H->txi.numvouts) == 0 )
            return(-9);
        else if ( (preds->tx0_txid= &H->tx0.txid) == 0 )
            return(-10);
        else if ( (preds->tx1_txid= &H->tx1.txid) == 0 )
            return(-11);
        else if ( (preds->txi_txid= &H->txi.txid) == 0 )
            return(-12);
        
        else if ( (preds->vin0_txid= &H->vin0.txid) == 0 )
            return(-13);
        else if ( (preds->vin1_txid= &H->vin1.txid) == 0 )
            return(-14);
        else if ( (preds->vini_txid= &H->vini.txid) == 0 )
            return(-15);
        else if ( (preds->vin0_vout= &H->vin0.vout) == 0 )
            return(-16);
        else if ( (preds->vin1_vout= &H->vin1.vout) == 0 )
            return(-17);
        else if ( (preds->vini_vout= &H->vini.vout) == 0 )
            return(-18);
        
        else if ( (preds->vout0_addr= &H->vout0.addr) == 0 )
            return(-19);
        else if ( (preds->vout1_addr= &H->vout1.addr) == 0 )
            return(-20);
        else if ( (preds->vout2_addr= &H->vout2.addr) == 0 )
            return(-21);
        else if ( (preds->vouti_addr= &H->vouti.addr) == 0 )
            return(-22);
        else if ( (preds->voutn_addr= &H->voutn.addr) == 0 )
            return(-23);
        else if ( (preds->vout0_script= &H->vout0.script) == 0 )
            return(-24);
        else if ( (preds->vout1_script= &H->vout1.script) == 0 )
            return(-25);
        else if ( (preds->vout2_script= &H->vout2.script) == 0 )
            return(-26);
        else if ( (preds->vouti_script= &H->vouti.script) == 0 )
            return(-27);
        else if ( (preds->voutn_script= &H->voutn.script) == 0 )
            return(-28);
    }
    return(0);
}

int32_t huffpair_decodeitemind(char type,int32_t bitflag,uint32_t *rawindp,struct huffpair *pair,HUFF *hp)
{
    uint16_t s;
    uint32_t ind;
    int32_t i,c,*tree,tmp,numitems,numbits,depth,num = 0;
    *rawindp = 0;
    if ( bitflag == 0 )
    {
        if ( type == 2 )
        {
            numbits = hdecode_smallbits(&s,hp);
            *rawindp = s;
            return(numbits);
        }
        return(hdecode_varbits(rawindp,hp));
    }
    if ( pair == 0 || pair->code == 0 || pair->code->tree == 0 )
        return(-1);
    numitems = pair->code->numitems;
    depth = pair->code->depth;
	while ( 1 )
    {
        tree = pair->code->tree;
        for (i=c=0; i<depth; i++)
        {
            if ( (c= hgetbit(hp)) < 0 )
                break;
            num++;
            //printf("%c",c+'0');
            if ( (tmp= tree[c]) < 0 )
            {
                tree -= tmp;
                continue;
            }
            ind = tmp;
            if ( ind >= numitems )
            {
                printf("decode error: val.%x -> ind.%d >= numitems %d\n",c,ind,numitems);
                return(-1);
            }
            *rawindp = pair->items[ind].rawind;
            return(num);
        }
        if ( c < 0 )
            break;
	}
    printf("(%s) huffpair_decodeitemind error num.%d\n",pair->name,num);
    return(-1);
}

//int32_t huffpair_conv(char *strbuf,uint64_t *valp,struct ramchain_info *V,char type,uint32_t rawind);
uint32_t huffpair_expand_int(int32_t bitflag,struct ramchain_info *V,struct huffpair *pair,HUFF *hp)
{
    uint32_t rawind; int32_t numbits;
    if ( (numbits= huffpair_decodeitemind(2,bitflag,&rawind,pair,hp)) < 0 )
        return(-1);
    // printf("%s.%d ",pair->name,rawind);
    return(rawind);
}

uint32_t huffpair_expand_str(int32_t bitflag,char *deststr,char type,struct ramchain_info *V,struct huffpair *pair,HUFF *hp)
{
    uint64_t destval;
    uint32_t rawind;
    int32_t numbits;
    deststr[0] = 0;
    if ( (numbits= huffpair_decodeitemind(0,bitflag,&rawind,pair,hp)) < 0 )
        return(-1);
    //fprintf(stderr,"huffexpand_str: bitflag.%d type.%d %s rawind.%d\n",bitflag,type,pair->name,rawind);
    if ( rawind == 0 )
        return(0);
    // map rawind
    //huffpair_conv(deststr,&destval,V,type,rawind);
    return((uint32_t)destval);
}

uint32_t huffpair_expand_rawbits(int32_t bitflag,struct ramchain_info *V,struct rawblock *raw,HUFF *hp,struct rawblock_preds *preds)
{
    struct huffpair *pair,*pair2,*pair3;
    int32_t txind,numrawvins,numrawvouts,vin,vout;
    struct rawtx *tx; struct rawvin *vi; struct rawvout *vo;
    ram_clear_rawblock(raw,0);
    hrewind(hp);
    raw->numtx = huffpair_expand_int(bitflag,V,preds->numtx,hp);
    raw->numrawvins = huffpair_expand_int(bitflag,V,preds->numrawvins,hp);
    raw->numrawvouts = huffpair_expand_int(bitflag,V,preds->numrawvouts,hp);
    if ( raw->numtx > 0 )
    {
        numrawvins = numrawvouts = 0;
        for (txind=0; txind<raw->numtx; txind++)
        {
            tx = &raw->txspace[txind];
            tx->firstvin = numrawvins;
            tx->firstvout = numrawvouts;
            if ( txind == 0 )
                pair = preds->tx0_numvins, pair2 = preds->tx0_numvouts, pair3 = preds->tx0_txid;
            else if ( txind == 1 )
                pair = preds->tx1_numvins, pair2 = preds->tx1_numvouts, pair3 = preds->tx1_txid;
            else pair = preds->txi_numvins, pair2 = preds->txi_numvouts, pair3 = preds->txi_txid;
            tx->numvins = huffpair_expand_int(bitflag,V,pair,hp), numrawvins += tx->numvins;
            tx->numvouts = huffpair_expand_int(bitflag,V,pair2,hp), numrawvouts += tx->numvouts;
            // printf("tx->numvins %d, numvouts.%d raw (%d %d) numtx.%d\n",tx->numvins,tx->numvouts,raw->numrawvins,raw->numrawvouts,raw->numtx);
            huffpair_expand_str(bitflag,tx->txidstr,'t',V,pair3,hp);
            if ( numrawvins <= raw->numrawvins && numrawvouts <= raw->numrawvouts )
            {
                if ( tx->numvins > 0 )
                {
                    for (vin=0; vin<tx->numvins; vin++)
                    {
                        vi = &raw->vinspace[tx->firstvin + vin];
                        if ( vin == 0 )
                            pair = preds->vin0_txid, pair2 = preds->vin0_vout;
                        else if ( vin == 1 )
                            pair = preds->vin1_txid, pair2 = preds->vin1_vout;
                        else pair = preds->vini_txid, pair2 = preds->vini_vout;
                        huffpair_expand_str(bitflag,vi->txidstr,'t',V,pair,hp);
                        vi->vout = huffpair_expand_int(bitflag,V,pair2,hp);
                    }
                }
                if ( tx->numvouts > 0 )
                {
                    for (vout=0; vout<tx->numvouts; vout++)
                    {
                        vo = &raw->voutspace[tx->firstvout + vout];
                        if ( vout == 0 )
                            pair = preds->vout0_addr, pair2 = preds->vout0_script;
                        else if ( vout == 1 )
                            pair = preds->vout1_addr, pair2 = preds->vout1_script;
                        else if ( vout == 2 )
                            pair = preds->vout2_addr, pair2 = preds->vout2_script;
                        else if ( vout == tx->numvouts-1 )
                            pair = preds->voutn_addr, pair2 = preds->voutn_script;
                        else pair = preds->vouti_addr, pair2 = preds->vouti_script;
                        huffpair_expand_str(bitflag,vo->script,'s',V,pair2,hp);
                        huffpair_expand_str(bitflag,vo->coinaddr,'a',V,pair,hp);
                        hdecode_valuebits(&vo->value,hp);
                    }
                }
            }
            else
            {
                printf("overflow during expand_bitstream: numrawvins %d <= %d raw->numrawvins && numrawvouts %d <= %d raw->numrawvouts\n",numrawvins,raw->numrawvins,numrawvouts,raw->numrawvouts);
                return(0);
            }
        }
        if ( numrawvins != raw->numrawvins || numrawvouts != raw->numrawvouts )
        {
            printf("overflow during expand_bitstream: numrawvins %d <= %d raw->numrawvins && numrawvouts %d <= %d raw->numrawvouts\n",numrawvins,raw->numrawvins,numrawvouts,raw->numrawvouts);
            return(0);
        }
    }
    return(0);
}

// >>>>>>>>>>>>>>  start ramchain functions
int32_t ram_decode_huffcode(union ramtypes *destU,struct ramchain_info *ram,int32_t selector,int32_t offset)
{
    int32_t numbits;
    destU->val64 = 0;
    return(numbits);
}

int32_t ram_huffencode(uint64_t *outbitsp,struct ramchain_info *ram,struct ramchain_token *token,void *srcptr,int32_t numbits)
{
    int32_t outbits;
    *outbitsp = 0;
    return(outbits);
}

int32_t ram_get_blockoffset(struct mappedblocks *blocks,uint32_t blocknum)
{
    int32_t offset = -1;
    if ( blocknum >= blocks->firstblock )
    {
        offset = (blocknum - blocks->firstblock);
        if ( offset >= blocks->numblocks )
        {
            printf("(%d - %d) = offset.%d >= numblocks.%d for format.%d\n",blocknum,blocks->firstblock,offset,blocks->numblocks,blocks->format);
            offset = -1;
        }
    }
    return(offset);
}

HUFF **ram_get_hpptr(struct mappedblocks *blocks,uint32_t blocknum)
{
    int32_t offset = ram_get_blockoffset(blocks,blocknum);
    return((offset < 0) ? 0 : &blocks->hps[offset]);
}

struct mappedptr *ram_get_M(struct mappedblocks *blocks,uint32_t blocknum)
{
    int32_t offset = ram_get_blockoffset(blocks,blocknum);
    return((offset < 0) ? 0 : &blocks->M[offset]);
}

int32_t ram_expand_scriptdata(char *scriptstr,uint8_t *scriptdata,int32_t datalen)
{
    char *prefix,*suffix;
    int32_t mode,n = 0;
    scriptstr[0] = 0;
    switch ( (mode= scriptdata[n++]) )
    {
        case 'z': prefix = "ffff", suffix = ""; break;
        case 'n': prefix = "nonstandard", suffix = ""; break;
        case 's': prefix = "76a914", suffix = "88ac"; break;
        case 'm': prefix = "a9", suffix = "ac"; break;
        case 'r': prefix = "", suffix = "ac"; break;
        case ' ': prefix = "", suffix = ""; break;
        default: printf("unexpected scriptmode.(%d) (%c)\n",mode,mode); prefix = "", suffix = ""; return(-1); break;
    }
    strcpy(scriptstr,prefix);
    init_hexbytes_noT(scriptstr+strlen(scriptstr),scriptdata+n,datalen-n);
    if ( suffix[0] != 0 )
    {
        //printf("mode.(%c) suffix.(%s) [%s]\n",mode,suffix,scriptstr);
        strcat(scriptstr,suffix);
    }
    return(mode);
}

uint64_t ram_check_redeemcointx(int32_t *unspendablep,struct ramchain_info *ram,char *script,uint32_t blocknum)
{
    uint64_t redeemtxid = 0;
    int32_t i;
    *unspendablep = 0;
    if ( strcmp(script,"76a914000000000000000000000000000000000000000088ac") == 0 )
        *unspendablep = 1;
    if ( strcmp(script+22,"00000000000000000000000088ac") == 0 )
    {
        for (redeemtxid=i=0; i<(int32_t)sizeof(uint64_t); i++)
        {
            redeemtxid <<= 8;
            redeemtxid |= (_decode_hex(&script[6 + 14 - i*2]) & 0xff);
        }
        printf("%s >>>>>>>>>>>>>>> found MGW redeem @blocknum.%u %s -> %llu | unspendable.%d\n",ram->name,blocknum,script,(long long)redeemtxid,*unspendablep);
    }
    else if ( *unspendablep != 0 )
        printf("%s >>>>>>>>>>>>>>> found unspendable %s\n",ram->name,script);

    //else printf("(%s).%d\n",script+22,strcmp(script+16,"00000000000000000000000088ac"));
    return(redeemtxid);
}

int32_t ram_calc_scriptmode(int32_t *datalenp,uint8_t scriptdata[4096],char *script,int32_t trimflag)
{
    int32_t n=0,len,mode = 0;
    len = (int32_t)strlen(script);
    *datalenp = 0;
    if ( len >= 8191 )
    {
        printf("calc_scriptmode overflow len.%d\n",len);
        return(-1);
    }
    if ( strcmp(script,"ffff") == 0 )
    {
        mode = 'z';
        if ( trimflag != 0 )
            script[0] = 0;
    }
    else if ( strcmp(script,"nonstandard") == 0 )
    {
        if ( trimflag != 0 )
            script[0] = 0;
        mode = 'n';
    }
    else if ( strncmp(script,"76a914",6) == 0 && strcmp(script+len-4,"88ac") == 0 )
    {
        if ( trimflag != 0 )
        {
            script[len-4] = 0;
            script += 6;
        }
        mode = 's';
    }
    else if ( strcmp(script+len-2,"ac") == 0 )
    {
        if ( strncmp(script,"a9",2) == 0 )
        {
            if ( trimflag != 0 )
            {
                script[len-2] = 0;
                script += 2;
            }
            mode = 'm';
        }
        else
        {
            if ( trimflag != 0 )
                script[len-2] = 0;
            mode = 'r';
        }
    } else mode = ' ';
    if ( trimflag != 0 )
    {
        scriptdata[n++] = mode;
        if ( (len= (int32_t)(strlen(script) >> 1)) > 0 )
            decode_hex(scriptdata+n,len,script);
        (*datalenp) = (len + n);
        //printf("set pubkey.(%s).%ld <- (%s)\n",pubkeystr,strlen(pubkeystr),script);
    }
    return(mode);
}

int32_t raw_blockcmp(struct rawblock *ref,struct rawblock *raw)
{
    int32_t i;
    if ( ref->numtx != raw->numtx )
    {
        printf("ref numtx.%d vs %d\n",ref->numtx,raw->numtx);
        return(-1);
    }
    if ( ref->numrawvins != raw->numrawvins )
    {
        printf("numrawvouts.%d vs %d\n",ref->numrawvins,raw->numrawvins);
        return(-2);
    }
    if ( ref->numrawvouts != raw->numrawvouts )
    {
        printf("numrawvouts.%d vs %d\n",ref->numrawvouts,raw->numrawvouts);
        return(-3);
    }
    if ( ref->numtx != 0 && memcmp(ref->txspace,raw->txspace,ref->numtx*sizeof(*raw->txspace)) != 0 )
    {
        struct rawtx *reftx,*rawtx;
        int32_t flag = 0;
        for (i=0; i<ref->numtx; i++)
        {
            reftx = &ref->txspace[i];
            rawtx = &raw->txspace[i];
            printf("1st.%d %d, %d %d (%s) vs 1st %d %d, %d %d (%s)\n",reftx->firstvin,reftx->firstvout,reftx->numvins,reftx->numvouts,reftx->txidstr,rawtx->firstvin,rawtx->firstvout,rawtx->numvins,rawtx->numvouts,rawtx->txidstr);
            flag = 0;
            if ( reftx->firstvin != rawtx->firstvin )
                flag |= 1;
            if ( reftx->firstvout != rawtx->firstvout )
                flag |= 2;
            if ( reftx->numvins != rawtx->numvins )
                flag |= 4;
            if ( reftx->numvouts != rawtx->numvouts )
                flag |= 8;
            if ( strcmp(reftx->txidstr,rawtx->txidstr) != 0 )
                flag |= 16;
            if ( flag != 0 )
                break;
        }
        if ( i != ref->numtx )
        {
            printf("flag.%d numtx.%d\n",flag,ref->numtx);
            //while ( 1 )
            //    portable_sleep(1);
            return(-4);
        }
    }
    if ( ref->numrawvins != 0 && memcmp(ref->vinspace,raw->vinspace,ref->numrawvins*sizeof(*raw->vinspace)) != 0 )
    {
        return(-5);
    }
    if ( ref->numrawvouts != 0 && memcmp(ref->voutspace,raw->voutspace,ref->numrawvouts*sizeof(*raw->voutspace)) != 0 )
    {
        struct rawvout *reftx,*rawtx;
        int32_t err = 0;
        for (i=0; i<ref->numrawvouts; i++)
        {
            reftx = &ref->voutspace[i];
            rawtx = &raw->voutspace[i];
            if ( strcmp(reftx->coinaddr,rawtx->coinaddr) != 0 || strcmp(reftx->script,rawtx->script) || reftx->value != rawtx->value )
                printf("%d of %d: (%s) (%s) %.8f vs (%s) (%s) %.8f\n",i,ref->numrawvouts,reftx->coinaddr,reftx->script,dstr(reftx->value),rawtx->coinaddr,rawtx->script,dstr(rawtx->value));
            if ( reftx->value != rawtx->value || strcmp(reftx->coinaddr,rawtx->coinaddr) != 0 || strcmp(reftx->script,rawtx->script) != 0 )
                err++;
        }
        if ( err != 0 )
        {
            printf("rawblockcmp error for vouts\n");
            getchar();
            return(-6);
        }
    }
    return(0);
}

void ram_clear_rawblock(struct rawblock *raw,int32_t totalflag)
{
    int32_t i;
    if ( totalflag != 0 )
        memset(raw,0,sizeof(*raw));
    else
    {
        raw->numtx = raw->numrawvins = raw->numrawvouts = 0;
        for (i=0; i<MAX_BLOCKTX; i++)
        {
            raw->txspace[i].txidstr[0] = 0;
            raw->vinspace[i].txidstr[0] = 0;
            raw->voutspace[i].script[0] = 0;
            raw->voutspace[i].coinaddr[0] = 0;
        }
    }
}

void ram_sethashtype(char *str,int32_t type)
{
    switch ( type )
    {
        case 'a': strcpy(str,"addr"); break;
        case 't': strcpy(str,"txid"); break;
        case 's': strcpy(str,"script"); break;
        default: sprintf(str,"type%d",type); break;
    }
}

void ram_sethashname(char fname[1024],struct ramchain_hashtable *hash,int32_t newflag)
{
    char str[128],typestr[128];
    ram_sethashtype(str,hash->type);
    if ( newflag != 0 )
        sprintf(typestr,"%s.new",str);
    else strcpy(typestr,str);
    huff_compressionvars_fname(0,fname,hash->coinstr,typestr,-1);
}

int32_t ram_addhash(struct ramchain_hashtable *hash,struct ramchain_hashptr *hp,void *ptr,int32_t datalen)
{
    hp->rawind = ++hash->ind;
    HASH_ADD_KEYPTR(hh,hash->table,ptr,datalen,hp);
    if ( 0 && hash->type == 't' )
    {
        char hexbytes[8192];
        struct ramchain_hashptr *checkhp;
        init_hexbytes_noT(hexbytes,ptr,datalen);
        HASH_FIND(hh,hash->table,ptr,datalen,checkhp);
        printf("created.(%s) ind.%d | checkhp.%p\n",hexbytes,hp->rawind,checkhp);
    }
    if ( (hash->ind + 1) >= hash->numalloc )
    {
        hash->numalloc += 512; // 4K page at a time
        hash->ptrs = realloc(hash->ptrs,sizeof(*hash->ptrs) * hash->numalloc);
        memset(&hash->ptrs[hash->ind],0,(hash->numalloc - hash->ind) * sizeof(*hash->ptrs));
    }
    hash->ptrs[hp->rawind] = hp;
    return(0);
}

struct ramchain_hashtable *ram_gethash(struct ramchain_info *ram,char type)
{
    switch ( type )
    {
        case 'a': return(&ram->addrhash); break;
        case 't': return(&ram->txidhash); break;
        case 's': return(&ram->scripthash); break;
    }
    return(0);
}

uint8_t *ram_encode_hashstr(int32_t *datalenp,uint8_t *data,char type,char *hashstr)
{
    uint8_t varbuf[9];
    char buf[8192];
    int32_t varlen,datalen,scriptmode = 0;
    *datalenp = 0;
    if ( type == 's' )
    {
        if ( hashstr[0] == 0 )
            return(0);
        strcpy(buf,hashstr);
        if ( (scriptmode = ram_calc_scriptmode(&datalen,&data[9],buf,1)) < 0 )
        {
            printf("encode_hashstr: scriptmode.%d for (%s)\n",scriptmode,hashstr);
            exit(-1);
        }
    }
    else if ( type == 't' )
    {
        datalen = (int32_t)(strlen(hashstr) >> 1);
        if ( datalen > 4096 )
        {
            printf("encode_hashstr: type.%d (%c) datalen.%d > sizeof(data) %d\n",type,type,(int)datalen,4096);
            exit(-1);
        }
        decode_hex(&data[9],datalen,hashstr);
    }
    else if ( type == 'a' )
    {
        datalen = (int32_t)strlen(hashstr) + 1;
        memcpy(&data[9],hashstr,datalen);
    }
    else
    {
        printf("encode_hashstr: unsupported type.%d (%c)\n",type,type);
        exit(-1);
    }
    if ( datalen > 0 )
    {
        varlen = hcalc_varint(varbuf,datalen);
        memcpy(&data[9-varlen],varbuf,varlen);
        //HASH_FIND(hh,hash->table,&ptr[-varlen],datalen+varlen,hp);
        *datalenp = (datalen + varlen);
        return(&data[9-varlen]);
    }
    return(0);
}

char *ram_decode_hashdata(char *strbuf,char type,uint8_t *hashdata)
{
    uint64_t varint;
    int32_t datalen,scriptmode;
    strbuf[0] = 0;
    if ( hashdata == 0 )
        return(0);
    hashdata += hdecode_varint(&varint,hashdata,0,9);
    datalen = (int32_t)varint;
    if ( type == 's' )
    {
        if ( (scriptmode= ram_expand_scriptdata(strbuf,hashdata,(uint32_t)datalen)) < 0 )
        {
            printf("decode_hashdata: scriptmode.%d for (%s)\n",scriptmode,strbuf);
            return(0);
        }
        //printf("EXPANDSCRIPT.(%c) -> [%s]\n",scriptmode,strbuf);
    }
    else if ( type == 't' )
    {
        if ( datalen > 128 )
        {
            init_hexbytes_noT(strbuf,hashdata,64);
            printf("decode_hashdata: type.%d (%c) datalen.%d > sizeof(data) %d | (%s)\n",type,type,(int)datalen,128,strbuf);
            exit(-1);
        }
        init_hexbytes_noT(strbuf,hashdata,datalen);
    }
    else if ( type == 'a' )
        memcpy(strbuf,hashdata,datalen);
    else
    {
        printf("decode_hashdata: unsupported type.%d\n",type);
        exit(-1);
    }
    return(strbuf);
}

struct ramchain_hashptr *ram_gethashptr(struct ramchain_info *ram,char type,uint32_t rawind)
{
    struct ramchain_hashtable *table;
    struct ramchain_hashptr *ptr = 0;
    table = ram_gethash(ram,type);
    if ( table != 0 && rawind > 0 && rawind <= table->ind && table->ptrs != 0 )
        ptr = table->ptrs[rawind];
    return(ptr);
}

void *ram_gethashdata(struct ramchain_info *ram,char type,uint32_t rawind)
{
    struct ramchain_hashptr *ptr;
    if ( (ptr= ram_gethashptr(ram,type,rawind)) != 0 )
        return(ptr->hh.key);
    return(0);
}

int32_t ram_script_multisig(struct ramchain_info *ram,uint32_t scriptind)
{
    char *hex;
    if ( (hex= ram_gethashdata(ram,'s',scriptind)) != 0 && hex[1] == 'm' )
    {
        printf("(%02x %02x %02x (%c)) ",hex[0],hex[1],hex[2],hex[1]);
        return(1);
    }
    return(0);
}

int32_t ram_script_nonstandard(struct ramchain_info *ram,uint32_t scriptind)
{
    char *hex;
    if ( (hex= ram_gethashdata(ram,'s',scriptind)) != 0 && hex[2] != 'm' && hex[2] != 's' )
        return(1);
    return(0);
}

struct rampayload *ram_gethashpayloads(struct ramchain_info *ram,char type,uint32_t rawind)
{
    struct ramchain_hashptr *ptr;
    if ( (ptr= ram_gethashptr(ram,type,rawind)) != 0 )
        return(ptr->payloads);
    return(0);
}

struct ramchain_hashptr *ram_hashdata_search(char *coinstr,struct alloc_space *mem,int32_t createflag,struct ramchain_hashtable *hash,uint8_t *hashdata,int32_t datalen)
{
    struct ramchain_hashptr *ptr = 0;
    char fname[512];
    void *newptr;
    if ( hash != 0 )
    {
        HASH_FIND(hh,hash->table,hashdata,datalen,ptr);
        if ( ptr == 0 && createflag != 0 )
        {
            if ( hash->newfp == 0 )
            {
                ram_sethashname(fname,hash,0);
                hash->newfp = fopen(os_compatible_path(fname),"wb");
                if ( hash->newfp != 0 )
                {
                    printf("couldnt create (%s)\n",fname);
                    exit(-1);
                }
            }
            ptr = (MAP_HUFF != 0) ? permalloc(coinstr,mem,sizeof(*ptr),3) : calloc(1,sizeof(*ptr));
            newptr = (MAP_HUFF != 0) ? permalloc(coinstr,mem,datalen,4) : calloc(1,datalen);
            memcpy(newptr,hashdata,datalen);
            //*createdflagp = 1;
            if ( 0 )
            {
                char hexstr[8192];
                init_hexbytes_noT(hexstr,newptr,datalen);
                printf("save.(%s) at %ld datalen.%d newfp.%p\n",hexstr,ftell(hash->newfp),datalen,hash->newfp);// getchar();//while ( 1 ) sleep(1);
            }
            if ( hash->newfp != 0 )
            {
                if ( fwrite(newptr,1,datalen,hash->newfp) != datalen )
                {
                    printf("error saving type.%d ind.%d datalen.%d\n",hash->type,hash->ind,datalen);
                    exit(-1);
                }
                fflush(hash->newfp);
            }
            //printf("add %d byte entry to hashtable %c\n",datalen,hash->type);
            ram_addhash(hash,ptr,newptr,datalen);
        } //else printf("found %d bytes ind.%d\n",datalen,ptr->rawind);
    } else printf("ram_hashdata_search null hashtable\n");
    //#endif
    return(ptr);
}

struct ramchain_hashptr *ram_hashsearch(char *coinstr,struct alloc_space *mem,int32_t createflag,struct ramchain_hashtable *hash,char *hashstr,char type)
{
    uint8_t data[4097],*hashdata;
    struct ramchain_hashptr *ptr = 0;
    int32_t datalen;
    if ( hash != 0 && (hashdata= ram_encode_hashstr(&datalen,data,type,hashstr)) != 0 )
        ptr = ram_hashdata_search(coinstr,mem,createflag,hash,hashdata,datalen);
    return(ptr);
}

uint32_t ram_conv_hashstr(uint32_t *permindp,int32_t createflag,struct ramchain_info *ram,char *hashstr,char type)
{
    char nullstr[6] = { 5, 'n', 'u', 'l', 'l', 0 };
    struct ramchain_hashptr *ptr = 0;
    struct ramchain_hashtable *hash;
    if ( permindp != 0 )
        *permindp = 0;
    if ( hashstr == 0 || hashstr[0] == 0 )
        hashstr = nullstr;
    hash = ram_gethash(ram,type);
    if ( (ptr= ram_hashsearch(ram->name,&ram->Perm,createflag,hash,hashstr,type)) != 0 )
    {
        if ( (hash->ind + 1) > ram->maxind )
            ram->maxind = (hash->ind + 1);
        if ( permindp != 0 )
            *permindp = ptr->permind;
        return(ptr->rawind);
    }
    else return(0);
}

struct rampayload *ram_payloads(struct ramchain_hashptr **ptrp,int32_t *numpayloadsp,struct ramchain_info *ram,char *hashstr,char type)
{
    struct ramchain_hashptr *ptr = 0;
    uint32_t rawind;
    *numpayloadsp = 0;
    if ( ptrp != 0 )
        *ptrp = 0;
    if ( (rawind= ram_conv_hashstr(0,0,ram,hashstr,type)) != 0 && (ptr= ram_gethashptr(ram,type,rawind)) != 0 )
    {
        if ( ptrp != 0 )
            *ptrp = ptr;
        *numpayloadsp = ptr->numpayloads;
        return(ptr->payloads);
    }
    else return(0);
}

struct address_entry *ram_address_entry(struct address_entry *destbp,struct ramchain_info *ram,char *txidstr,int32_t vout)
{
    int32_t numpayloads;
    struct rampayload *payloads;
    if ( (payloads= ram_txpayloads(0,&numpayloads,ram,txidstr)) != 0 )
    {
        if ( vout < payloads[0].B.v )
        {
            *destbp = payloads[vout].B;
            return(destbp);
        }
    }
    return(0);
}

struct rampayload *ram_getpayloadi(struct ramchain_hashptr **ptrp,struct ramchain_info *ram,char type,uint32_t rawind,uint32_t i)
{
    struct ramchain_hashptr *ptr;
    if ( ptrp != 0 )
        *ptrp = 0;
    if ( (ptr= ram_gethashptr(ram,type,rawind)) != 0 && i < ptr->numpayloads && ptr->payloads != 0 )
    {
        if ( ptrp != 0 )
            *ptrp = ptr;
        return(&ptr->payloads[i]);
    }
    return(0);
}

cJSON *ram_rawvin_json(struct rawblock *raw,struct rawtx *tx,int32_t vin)
{
    struct rawvin *vi = &raw->vinspace[tx->firstvin + vin];
    cJSON *json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"txid",cJSON_CreateString(vi->txidstr));
    cJSON_AddItemToObject(json,"vout",cJSON_CreateNumber(vi->vout));
    return(json);
}

cJSON *ram_rawvout_json(struct rawblock *raw,struct rawtx *tx,int32_t vout)
{
    struct rawvout *vo = &raw->voutspace[tx->firstvout + vout];
    cJSON *item,*array,*json = cJSON_CreateObject();
    /*"scriptPubKey" : {
     "asm" : "OP_DUP OP_HASH160 b2098d38dfd1bee61b12c9abc40e988d273d90ae OP_EQUALVERIFY OP_CHECKSIG",
     "reqSigs" : 1,
     "type" : "pubkeyhash",
     "addresses" : [
     "RRWZoKdmHGDbS5vfj7KwBScK3uSTpt9pHL"
     ]
     }*/
    cJSON_AddItemToObject(json,"value",cJSON_CreateNumber(dstr(vo->value)));
    cJSON_AddItemToObject(json,"n",cJSON_CreateNumber(vout));
    item = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"hex",cJSON_CreateString(vo->script));
    array = cJSON_CreateArray();
    cJSON_AddItemToArray(array,cJSON_CreateString(vo->coinaddr));
    cJSON_AddItemToObject(item,"addresses",array);
    cJSON_AddItemToObject(json,"scriptPubKey",item);
    return(json);
}

cJSON *ram_rawtx_json(struct rawblock *raw,int32_t txind)
{
    struct rawtx *tx = &raw->txspace[txind];
    cJSON *array,*json = cJSON_CreateObject();
    int32_t i,numvins,numvouts;
    cJSON_AddItemToObject(json,"txid",cJSON_CreateString(tx->txidstr));
    if ( (numvins= tx->numvins) > 0 )
    {
        array = cJSON_CreateArray();
        for (i=0; i<numvins; i++)
            cJSON_AddItemToArray(array,ram_rawvin_json(raw,tx,i));
        cJSON_AddItemToObject(json,"vin",array);
    }
    if ( (numvouts= tx->numvouts) > 0 )
    {
        array = cJSON_CreateArray();
        for (i=0; i<numvouts; i++)
            cJSON_AddItemToArray(array,ram_rawvout_json(raw,tx,i));
        cJSON_AddItemToObject(json,"vout",array);
    }
    return(json);
}

cJSON *ram_rawblock_json(struct rawblock *raw,int32_t allocsize)
{
    int32_t i,n;
    cJSON *array,*json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"height",cJSON_CreateNumber(raw->blocknum));
    cJSON_AddItemToObject(json,"numtx",cJSON_CreateNumber(raw->numtx));
    cJSON_AddItemToObject(json,"mint",cJSON_CreateNumber(dstr(raw->minted)));
    if ( allocsize != 0 )
        cJSON_AddItemToObject(json,"rawsize",cJSON_CreateNumber(allocsize));
    if ( (n= raw->numtx) > 0 )
    {
        array = cJSON_CreateArray();
        for (i=0; i<n; i++)
            cJSON_AddItemToArray(array,ram_rawtx_json(raw,i));
        cJSON_AddItemToObject(json,"tx",array);
    }
    return(json);
}

#define ram_rawtx(raw,txind) (((txind) < (raw)->numtx) ? &(raw)->txspace[txind] : 0)

struct rawvout *ram_rawvout(struct rawblock *raw,int32_t txind,int32_t v)
{
    struct rawtx *tx;
    if ( txind < raw->numtx )
    {
        if ( (tx= ram_rawtx(raw,txind)) != 0 )
            return(&raw->voutspace[tx->firstvout + v]);
    }
    return(0);
}

struct rawvin *ram_rawvin(struct rawblock *raw,int32_t txind,int32_t v)
{
    struct rawtx *tx;
    if ( txind < raw->numtx )
    {
        if ( (tx= ram_rawtx(raw,txind)) != 0 )
            return(&raw->vinspace[tx->firstvin + v]);
    }
    return(0);
}

void *ram_getrawdest(struct rawblock *raw,struct ramchain_token *token)
{
    int32_t txind;
    struct rawtx *tx;
    switch ( token->selector )
    {
        case 'B':
            if ( token->type == 4 )
                return(&raw->blocknum);
            else if ( token->type == 2 )
                return(&raw->numtx);
            else if ( token->type == 8 )
                return(&raw->minted);
            break;
        case 'T':
            txind = (token->offset >> 1);
            tx = &raw->txspace[txind];
            if ( token->type == 't' )
                return(tx->txidstr);
            else if ( (token->offset & 1) != 0 )
                return(&tx->numvouts);
            else if ( token->type == 2 )
                return(&tx->numvins);
            break;
        case 'I':
            if ( token->type == 't' )
                return(raw->vinspace[token->offset].txidstr);
            else return(&raw->vinspace[token->offset].vout);
            break;
        case 'O':
            if ( token->type == 's' )
                return(raw->voutspace[token->offset].script);
            else if ( token->type == 'a' )
                return(raw->voutspace[token->offset].coinaddr);
            else if ( token->type == 8 )
                return(&raw->voutspace[token->offset].value);
            break;
        default: printf("illegal token selector.%d\n",token->selector); return(0); break;
    }
    return(0);
}

int32_t ram_emit_token(int32_t compressflag,HUFF *outbits,struct ramchain_info *ram,struct ramchain_token *token)
{
    uint8_t *hashdata;
    int32_t i,bitlen,type;
    hashdata = token->U.hashdata;
    type = token->type;
    //printf("emit token.(%d) datalen.%d\n",token->type,outbits->bitoffset);
    if ( compressflag != 0 && type != 16 )
    {
        if ( type == 'a' || type == 't' || type == 's' )
        {
            if ( (token->numbits & 7) != 0 )
            {
                printf("misaligned token numbits.%d\n",token->numbits);
                return(-1);
            }
            return(hemit_varbits(outbits,token->rawind));
        }
        else
        {
            if ( type == 2 )
                return(hemit_smallbits(outbits,token->U.val16));
            else if ( type == 4 || type == -4 )
                return(hemit_varbits(outbits,token->U.val32));
            else if ( type == 8 || type == -8 )
                return(hemit_valuebits(outbits,token->U.val64));
            else return(hemit_smallbits(outbits,token->U.val8));
        }
    }
    bitlen = token->numbits;
    if ( (token->numbits & 7) == 0 && (outbits->bitoffset & 7) == 0 )
    {
        memcpy(outbits->ptr,hashdata,bitlen>>3);
        outbits->bitoffset += bitlen;
        hupdate_internals(outbits);
    }
    else
    {
        for (i=0; i<bitlen; i++)
            hputbit(outbits,GETBIT(hashdata,i));
    }
    return(bitlen);
}

uint64_t ram_extract_varint(HUFF *hp)
{
    uint8_t c; uint16_t s; uint32_t i; uint64_t varint = 0;
    hmemcpy(&c,0,hp,1);
    if ( c >= 0xfd )
    {
        if ( c == 0xfd )
            hmemcpy(&s,0,hp,2), varint = s;
        else if ( c == 0xfe )
            hmemcpy(&i,0,hp,4), varint = i;
        else hmemcpy(&varint,0,hp,8);
    } else varint = c;
    //printf("c.%d -> (%llu)\n",c,(long long)varint);
    return(varint);
}

int32_t ram_extract_varstr(uint8_t *data,HUFF *hp)
{
    uint8_t c,*ptr; uint16_t s; int32_t datalen; uint64_t varint = 0;
    hmemcpy(&c,0,hp,1), data[0] = c, ptr = &data[1];
    if ( c >= 0xfd )
    {
        if ( c == 0xfd )
            hmemcpy(&s,0,hp,2), memcpy(ptr,&s,2), datalen = s, ptr += 2;
        else if ( c == 0xfe )
            hmemcpy(&datalen,0,hp,4), memcpy(ptr,&datalen,4), ptr += 4;
        else hmemcpy(&varint,0,hp,8),  memcpy(ptr,&varint,8), datalen = (uint32_t)varint, ptr += 8;
    } else datalen = c;
    hmemcpy(ptr,0,hp,datalen);
    return((int32_t)((long)ptr - (long)data));
}

uint32_t ram_extractstring(char *hashstr,char type,struct ramchain_info *ram,int32_t selector,int32_t offset,HUFF *hp,int32_t format)
{
    uint8_t hashdata[8192],*hashdataptr;
    union ramtypes U;
    uint32_t rawind = 0;
    if ( format == 'V' )
    {
        ram_extract_varstr(hashdata,hp);
        if ( ram_decode_hashdata(hashstr,type,hashdata) == 0 )
        {
            printf("ram_extractstring.V t.(%c) decode_hashdata error\n",type);
            return(0);
        }
        rawind = ram_conv_hashstr(0,0,ram,hashstr,type);
    }
    else
    {
        if ( format == 'B' )
            hdecode_varbits(&rawind,hp);
        else if ( format == 'H' )
        {
            ram_decode_huffcode(&U,ram,selector,offset);
            rawind = U.val32;
        }
        else printf("ram_extractstring illegal format.%d\n",format);
        //printf("type.%c rawind.%d hashdataptr.%p\n",type,rawind,ram_gethashdata(ram,type,rawind));
        if ( (hashdataptr = ram_gethashdata(ram,type,rawind)) == 0 || ram_decode_hashdata(hashstr,type,hashdataptr) == 0 )
            rawind = 0;
        //printf("(%d) ramextract string rawind.%d (%c) -> (%s)\n",rawind,format,type,hashstr);
    }
    //rawind = ram_conv_hashstr(ram,hashstr,type);
    //printf("ram_extractstring got rawind.%d\n",rawind);
    return(rawind);
}

uint32_t ram_extractint(struct ramchain_info *ram,int32_t selector,int32_t offset,HUFF *hp,int32_t format)
{
    uint32_t i = 0;
    union ramtypes U;
    if ( format == 'B' )
        hdecode_varbits(&i,hp);
    else if ( format == 'H' )
    {
        ram_decode_huffcode(&U,ram,selector,offset);
        i = U.val32;
    } else printf("invalid format.%d\n",format);
    return(i);
}

uint16_t ram_extractshort(struct ramchain_info *ram,int32_t selector,int32_t offset,HUFF *hp,int32_t format)
{
    uint16_t s = 0;
    union ramtypes U;
    //fprintf(stderr,"s.%d: ",hp->bitoffset);
    if ( format == 'B' )
        hdecode_smallbits(&s,hp);
    else if ( format == 'H' )
    {
        ram_decode_huffcode(&U,ram,selector,offset);
        s = U.val16;
    } else printf("invalid format.%d\n",format);
    return(s);
}

uint64_t ram_extractlong(struct ramchain_info *ram,int32_t selector,int32_t offset,HUFF *hp,int32_t format)
{
    uint64_t x = 0;
    union ramtypes U;
    if ( format == 'B' )
        hdecode_valuebits(&x,hp);
    else if ( format == 'H' )
    {
        ram_decode_huffcode(&U,ram,selector,offset);
        x = U.val64;
    } else printf("invalid format.%d\n",format);
    return(x);
}

struct ramchain_token *ram_set_token_hashdata(struct ramchain_info *ram,char type,char *hashstr,uint32_t rawind)
{
    uint8_t data[4097],*hashdata;
    char strbuf[8192];
    struct ramchain_hashptr *ptr;
    struct ramchain_token *token = 0;
    int32_t datalen;
    if ( type == 'a' || type == 't' || type == 's' )
    {
        if ( hashstr == 0 )
        {
            if ( rawind == 0 || rawind == 0xffffffff )
            {
                printf("ram_set_token_hashdata no hashstr and rawind.%d\n",rawind); while ( 1 ) sleep(1);
                return(0);
            }
            hashstr = strbuf;
            if ( ram_conv_rawind(hashstr,ram,rawind,type) == 0 )
                rawind = 0;
            token->rawind = rawind;
            printf("ram_set_token converted rawind.%d -> (%c).(%s)\n",rawind,type,hashstr);
        }
        else if ( hashstr[0] == 0 )
            token = memalloc(&ram->Tmp,sizeof(*token));
        else if ( (hashdata= ram_encode_hashstr(&datalen,data,type,hashstr)) != 0 )
        {
            token = memalloc(&ram->Tmp,sizeof(*token) + datalen - sizeof(token->U));
            memcpy(token->U.hashdata,hashdata,datalen);
            token->numbits = (datalen << 3);
            if ( (ptr= ram_hashdata_search(ram->name,&ram->Perm,1,ram_gethash(ram,type),hashdata,datalen)) != 0 )
                token->rawind = ptr->rawind;
            // printf(">>>>>> rawind.%d -> %d\n",rawind,token->rawind);
        } else printf("encode_hashstr error for (%c).(%s)\n",type,hashstr);
    }
    else //if ( destformat == 'B' )
    {
        token = memalloc(&ram->Tmp,sizeof(*token));
        token->numbits = (sizeof(rawind) << 3);
        if ( hashstr != 0 && hashstr[0] != 0 )
        {
            rawind = ram_conv_hashstr(0,0,ram,hashstr,type);
            //printf("(%s) -> %d\n",hashstr,rawind);
        }
        token->rawind = rawind;
        //printf("<<<<<<<<<< rawind.%d -> %d\n",rawind,token->rawind);
    }
    /*else if ( destformat == '*' )
     {
     token = memalloc(&ram->Tmp,sizeof(*token));
     token->ishuffcode = 1;
     if ( hashstr != 0 && hashstr[0] != 0 )
     rawind = ram_conv_hashstr(ram,hashstr,type);
     token->rawind = rawind;
     token->numbits = ram_huffencode(&token->U.val64,ram,token,&U,sizeof(rawind) << 3);
     }*/
    return(token);
}

void ram_sprintf_number(char *hashstr,union ramtypes *U,char type)
{
    switch ( type )
    {
        case 8: sprintf(hashstr,"%.8f",dstr(U->val64)); break;
        case -8: sprintf(hashstr,"%.14f",U->dval); break;
        case -4: sprintf(hashstr,"%.10f",U->fval); break;
        case 4: sprintf(hashstr,"%u",U->val32); break;
        case 2: sprintf(hashstr,"%u",U->val16); break;
        case 1: sprintf(hashstr,"%u",U->val8); break;
        default: sprintf(hashstr,"invalid type error.%d",type);
    }
}

struct ramchain_token *ram_createtoken(struct ramchain_info *ram,char selector,uint16_t offset,char type,char *hashstr,uint32_t rawind,union ramtypes *U,int32_t datalen)
{
    char strbuf[128];
    struct ramchain_token *token;
    switch ( type )
    {
        case 'a': case 't': case 's':
            //printf("%c.%d: (%c) token.(%s) rawind.%d\n",selector,offset,type,hashstr,rawind);
            token = ram_set_token_hashdata(ram,type,hashstr,rawind);
            break;
        case 16: case 8: case -8: case 4: case -4: case 2: case 1:
            token = memalloc(&ram->Tmp,sizeof(*token));
            /*if ( destformat == '*' )
             {
             token->numbits = ram_huffencode(&token->U.val64,ram,token,U,token->numbits);
             token->ishuffcode = 1;
             }
             else*/
        {
            token->U = *U;
            token->numbits = ((type >= 0) ? type : -type) << 3;
        }
            memcpy(&token->rawind,U->hashdata,sizeof(token->rawind));
            if ( hashstr == 0 )
                hashstr = strbuf;
            ram_sprintf_number(hashstr,U,type);
            //printf("%c.%d: (%d) token.%llu rawind.%d\n",selector,offset,type,(long long)token->U.val64,rawind);
            break;
        default: printf("ram_createtoken: illegal tokentype.%d\n",type); return(0); break;
    }
    if ( token != 0 )
    {
        token->selector = selector;
        token->offset = offset;
        token->type = type;
        //fprintf(stderr,"{%c.%c.%d %03u (%s)} ",token->type>16?token->type : '0'+token->type,token->selector,token->offset,token->rawind&0xffff,hashstr);
    }  //else fprintf(stderr,"{%c.%c.%d %03d ERR } ",type>16?type : '0'+type,selector,offset,rawind);
    return(token);
}

void *ram_tokenstr(void *longspacep,int32_t *datalenp,struct ramchain_info *ram,char *hashstr,struct ramchain_token *token)
{
    union ramtypes U;
    char type;
    uint32_t rawind = 0xffffffff;
    hashstr[0] = 0;
    type = token->type;
    if ( type == 'a' || type == 't' || type == 's' )
    {
        if ( token->ishuffcode != 0 )
        {
            ram_decode_huffcode(&U,ram,token->selector,token->offset);
            rawind = U.val32;
        }
        else rawind = token->rawind;
        if ( rawind != 0xffffffff && rawind != 0 && ram_conv_rawind(hashstr,ram,rawind,type) == 0 )
            rawind = 0;
        if ( ram_decode_hashdata(hashstr,token->type,token->U.hashdata) == 0 )
        {
            *datalenp = 0;
            printf("ram_expand_token decode_hashdata error\n");
            return(0);
        }
        *datalenp = (int32_t)(strlen(hashstr) + 1);
        return(hashstr);
    }
    else
    {
        if ( token->ishuffcode != 0 )
            *datalenp = ram_decode_huffcode(&U,ram,token->selector,token->offset);
        else
        {
            *datalenp = (token->numbits >> 3);
            U = token->U;
        }
        ram_sprintf_number(hashstr,&U,type);
        memcpy(longspacep,&U,*datalenp);
    }
    return(longspacep);
}

void ram_patch_rawblock(struct rawblock *raw)
{
    int32_t txind,numtx,firstvin,firstvout;
    struct rawtx *tx;
    firstvin = firstvout = 0;
    if ( (numtx= raw->numtx) != 0 )
    {
        for (txind=0; txind<numtx; txind++)
        {
            tx = &raw->txspace[txind];
            tx->firstvin = firstvin, firstvin += tx->numvins;
            tx->firstvout = firstvout, firstvout += tx->numvouts;
        }
    }
    raw->numrawvouts = firstvout;
    raw->numrawvins = firstvin;
}

#define num_rawblock_tokens(raw) 3
#define num_rawtx_tokens(raw) 3
#define num_rawvin_tokens(raw) 2
#define num_rawvout_tokens(raw) 3

#define ram_createstring(ram,selector,offset,type,str,rawind) ram_createtoken(ram,selector,offset,type,str,rawind,0,0)
#define ram_createbyte(ram,selector,offset,rawind) ram_createtoken(ram,selector,offset,1,0,rawind,&U,1)
#define ram_createshort(ram,selector,offset,rawind) ram_createtoken(ram,selector,offset,2,0,rawind,&U,2)
#define ram_createint(ram,selector,offset,rawind) ram_createtoken(ram,selector,offset,4,0,rawind,&U,4)
#define ram_createfloat(ram,selector,offset,rawind) ram_createtoken(ram,selector,offset,-4,0,rawind,&U,4)
#define ram_createdouble(ram,selector,offset,rawind) ram_createtoken(ram,selector,offset,-8,0,rawind,&U,8)
#define ram_createlong(ram,selector,offset,rawind) ram_createtoken(ram,selector,offset,8,0,rawind,&U,8)

void raw_emitstr(HUFF *hp,char type,char *hashstr)
{
    uint8_t data[8192],*hashdata;
    int32_t i,numbits,datalen = 0;
    if ( (hashdata= ram_encode_hashstr(&datalen,data,type,hashstr)) != 0 )
    {
        numbits = (datalen << 3);
        for (i=0; i<numbits; i++)
            hputbit(hp,GETBIT(hashdata,i));
    }
}

void ram_rawvin_emit(HUFF *hp,struct rawvin *vi)
{
    raw_emitstr(hp,'t',vi->txidstr);
    hemit_bits(hp,vi->vout,sizeof(vi->vout));
}

void ram_rawvout_emit(HUFF *hp,struct rawvout *vo)
{
    raw_emitstr(hp,'s',vo->script);
    raw_emitstr(hp,'a',vo->coinaddr);
    hemit_longbits(hp,vo->value);
}

int32_t ram_rawtx_emit(HUFF *hp,struct rawblock *raw,struct rawtx *tx)
{
    int32_t i,numvins,numvouts;
    hemit_bits(hp,tx->numvins,sizeof(tx->numvins));
    hemit_bits(hp,tx->numvouts,sizeof(tx->numvouts));
    raw_emitstr(hp,'t',tx->txidstr);
    if ( (numvins= tx->numvins) > 0 )
        for (i=0; i<numvins; i++)
            ram_rawvin_emit(hp,&raw->vinspace[tx->firstvin + i]);
    if ( (numvouts= tx->numvouts) > 0 )
        for (i=0; i<numvouts; i++)
            ram_rawvout_emit(hp,&raw->voutspace[tx->firstvout + i]);
    return(0);
}

int32_t ram_rawblock_emit(HUFF *hp,struct ramchain_info *ram,struct rawblock *raw)
{
    int32_t txind,n;
    ram_patch_rawblock(raw);
    hrewind(hp);
    hemit_bits(hp,'V',8);
    hemit_bits(hp,raw->blocknum,sizeof(raw->blocknum));
    hemit_bits(hp,raw->numtx,sizeof(raw->numtx));
    hemit_longbits(hp,raw->minted);
    if ( (n= raw->numtx) > 0 )
    {
        for (txind=0; txind<n; txind++)
            ram_rawtx_emit(hp,raw,&raw->txspace[txind]);
    }
    return(hconv_bitlen(hp->bitoffset));
}

int32_t ram_rawvin_scan(struct ramchain_info *ram,struct ramchain_token **tokens,int32_t numtokens,struct rawblock *raw,struct rawtx *tx,int32_t vin)
{
    struct rawvin *vi;
    union ramtypes U;
    int32_t numrawvins;
    uint32_t txid_rawind,rawind = 0;
    numrawvins = (tx->firstvin + vin);
    vi = &raw->vinspace[numrawvins];
    if ( tokens != 0 )
    {
        if ( (txid_rawind= ram_txidind(ram,vi->txidstr)) != 0 )
            tokens[numtokens++] = ram_createstring(ram,'I',numrawvins,'t',vi->txidstr,txid_rawind);
        else tokens[numtokens++] = 0;
        memset(&U,0,sizeof(U)), U.val16 = vi->vout, tokens[numtokens++] = ram_createshort(ram,'I',numrawvins,rawind);
    }
    else numtokens += num_rawvin_tokens(raw);
    return(numtokens);
}

int32_t ram_rawvout_scan(struct ramchain_info *ram,struct ramchain_token **tokens,int32_t numtokens,struct rawblock *raw,struct rawtx *tx,int32_t vout)
{
    struct rawvout *vo;
    union ramtypes U;
    int numrawvouts;
    uint32_t addrind,scriptind,rawind = 0;
    numrawvouts = (tx->firstvout + vout);
    vo = &raw->voutspace[numrawvouts];
    if ( tokens != 0 )
    {
        if ( (scriptind= ram_scriptind(ram,vo->script)) != 0 )
            tokens[numtokens++] = ram_createstring(ram,'O',numrawvouts,'s',vo->script,scriptind);
        else tokens[numtokens++] = 0;
        if ( (addrind= ram_addrind(ram,vo->coinaddr)) != 0 )
            tokens[numtokens++] = ram_createstring(ram,'O',numrawvouts,'a',vo->coinaddr,addrind);
        else tokens[numtokens++] = 0;
        U.val64 = vo->value, tokens[numtokens++] = ram_createlong(ram,'O',numrawvouts,rawind);
    } else numtokens += num_rawvout_tokens(raw);
    return(numtokens);
}

int32_t ram_rawtx_scan(struct ramchain_info *ram,struct ramchain_token **tokens,int32_t numtokens,struct rawblock *raw,int32_t txind)
{
    struct rawtx *tx;
    union ramtypes U;
    uint32_t txid_rawind,rawind = 0;
    int32_t i,numvins,numvouts;
    tx = &raw->txspace[txind];
    if ( tokens != 0 )
    {
        memset(&U,0,sizeof(U)), U.val16 = tx->numvins, tokens[numtokens++] = ram_createshort(ram,'T',(txind<<1) | 0,rawind);
        U.val16 = tx->numvouts, tokens[numtokens++] = ram_createshort(ram,'T',(txind<<1) | 1,rawind);
        if ( (txid_rawind= ram_txidind(ram,tx->txidstr)) != 0 )
            tokens[numtokens++] = ram_createstring(ram,'T',(txind<<1),'t',tx->txidstr,txid_rawind);
        else tokens[numtokens++] = 0;
    } else numtokens += num_rawtx_tokens(raw);
    if ( (numvins= tx->numvins) > 0 )
    {
        for (i=0; i<numvins; i++)
            numtokens = ram_rawvin_scan(ram,tokens,numtokens,raw,tx,i);
    }
    if ( (numvouts= tx->numvouts) > 0 )
    {
        for (i=0; i<numvouts; i++)
            numtokens = ram_rawvout_scan(ram,tokens,numtokens,raw,tx,i);
    }
    return(numtokens);
}

struct ramchain_token **ram_tokenize_rawblock(int32_t *numtokensp,struct ramchain_info *ram,struct rawblock *raw)
{ // parse structure full of gaps
    union ramtypes U;
    uint32_t rawind = 0;
    int32_t i,n,maxtokens,numtokens = 0;
    struct ramchain_token **tokens = 0;
    maxtokens = num_rawblock_tokens(raw) + (raw->numtx * num_rawtx_tokens(raw)) + (raw->numrawvins * num_rawvin_tokens(raw)) + (raw->numrawvouts * num_rawvout_tokens(raw));
    ram_clear_alloc_space(&ram->Tmp);
    tokens = memalloc(&ram->Tmp,maxtokens*sizeof(*tokens));
    ram_patch_rawblock(raw);
    if ( tokens != 0 )
    {
        memset(&U,0,sizeof(U)), U.val32 = raw->blocknum, tokens[numtokens++] = ram_createint(ram,'B',0,rawind);
        memset(&U,0,sizeof(U)), U.val16 = raw->numtx, tokens[numtokens++] = ram_createshort(ram,'B',0,rawind);
        U.val64 = raw->minted, tokens[numtokens++] = ram_createlong(ram,'B',0,rawind);
    } else numtokens += num_rawblock_tokens(raw);
    if ( (n= raw->numtx) > 0 )
    {
        for (i=0; i<n; i++)
            numtokens = ram_rawtx_scan(ram,tokens,numtokens,raw,i);
    }
    if ( numtokens > maxtokens )
        printf("numtokens.%d > maxtokens.%d\n",numtokens,maxtokens);
    *numtokensp = numtokens;
    return(tokens);
}

struct ramchain_token *ram_extract_and_tokenize(union ramtypes *Uptr,struct ramchain_info *ram,int32_t selector,int32_t offset,HUFF *hp,int32_t srcformat,int32_t size)
{
    int32_t i;
    union ramtypes U;
    memset(&U,0,sizeof(U));
    if ( srcformat == 'V' )
    {
        size <<= 3;
        if ( ((hp->bitoffset+size) >> 3) > hp->allocsize )
        {
            printf("hp->bitoffset.%d + size.%d >= allocsize.%d bytes.(%d)\n",hp->bitoffset,size,hp->allocsize<<3,hp->allocsize);
            return(0);
        }
        for (i=0; i<size; i++)
            if ( hgetbit(hp) != 0 )
                SETBIT(U.hashdata,i);
        size >>= 3;
        //fprintf(stderr,"[%llx].%d ",(long long)U.val64,hp->bitoffset);
    }
    else
    {
        if ( size == 2 )
            U.val16 = ram_extractshort(ram,selector,offset,hp,srcformat);
        else if ( size == 4 )
            U.val32 = ram_extractint(ram,selector,offset,hp,srcformat);
        else if ( size == 8 )
            U.val64 = ram_extractlong(ram,selector,offset,hp,srcformat);
        else printf("extract_and_tokenize illegalsize %d\n",size);
        //fprintf(stderr,"(%llx).%d ",(long long)U.val64,hp->bitoffset);
    }
    *Uptr = U;
    if ( size == 2 )
        return(ram_createshort(ram,selector,offset,0));
    else if ( size == 4 )
        return(ram_createint(ram,selector,offset,0));
    else if ( size == 8 )
        return(ram_createlong(ram,selector,offset,0));
    printf("bad place you are\n");
    exit(-1);
    return(0);
}

int32_t ram_rawvin_huffscan(struct ramchain_info *ram,struct ramchain_token **tokens,int32_t numtokens,HUFF *hp,int32_t format,int32_t numrawvins)
{
    char txidstr[4096];
    union ramtypes U;
    uint32_t txid_rawind,orignumtokens = numtokens;
    if ( tokens != 0 )
    {
        if ( (txid_rawind= ram_extractstring(txidstr,'t',ram,'I',numrawvins,hp,format)) != 0 )
            tokens[numtokens++] = ram_createstring(ram,'I',numrawvins,'t',txidstr,txid_rawind);
        else return(orignumtokens);
        if ( (tokens[numtokens++]= ram_extract_and_tokenize(&U,ram,'I',numrawvins,hp,format,sizeof(uint16_t))) == 0 )
        {
            printf("numrawvins.%d: txid_rawind.%d (%s) vout.%d\n",numrawvins,txid_rawind,txidstr,U.val16);
            while ( 1 )
                portable_sleep(1);
            return(orignumtokens);
        }
    }
    else numtokens += num_rawvin_tokens(raw);
    return(numtokens);
}

int32_t ram_rawvout_huffscan(struct ramchain_info *ram,struct ramchain_token **tokens,int32_t numtokens,HUFF *hp,int32_t format,int32_t numrawvouts)
{
    char scriptstr[4096],coinaddr[4096];
    uint32_t scriptind,addrind,orignumtokens = numtokens;
    union ramtypes U;
    if ( tokens != 0 )
    {
        if ( (scriptind= ram_extractstring(scriptstr,'s',ram,'O',numrawvouts,hp,format)) != 0 )
            tokens[numtokens++] = ram_createstring(ram,'O',numrawvouts,'s',scriptstr,scriptind);
        else return(orignumtokens);
        if ( (addrind= ram_extractstring(coinaddr,'a',ram,'O',numrawvouts,hp,format)) != 0 )
            tokens[numtokens++] = ram_createstring(ram,'O',numrawvouts,'a',coinaddr,addrind);
        else return(orignumtokens);
        if ( (tokens[numtokens++] = ram_extract_and_tokenize(&U,ram,'O',numrawvouts,hp,format,sizeof(uint64_t))) == 0 )
        {
            printf("numrawvouts.%d: scriptind.%d (%s) addrind.%d (%s) value.(%.8f)\n",numrawvouts,scriptind,scriptstr,addrind,coinaddr,dstr(U.val64));
            while ( 1 )
                portable_sleep(1);
            return(orignumtokens);
        }
    } else numtokens += num_rawvout_tokens(raw);
    return(numtokens);
}

int32_t ram_rawtx_huffscan(struct ramchain_info *ram,struct ramchain_token **tokens,int32_t numtokens,HUFF *hp,int32_t format,int32_t txind,int32_t *firstvinp,int32_t *firstvoutp)
{
    union ramtypes U;
    char txidstr[4096];
    uint32_t txid_rawind,lastnumtokens,orignumtokens;
    int32_t i,numvins = 0,numvouts = 0;
    orignumtokens = numtokens;
    if ( tokens != 0 )
    {
        tokens[numtokens++] = ram_extract_and_tokenize(&U,ram,'T',(txind<<1) | 0,hp,format,sizeof(uint16_t)), numvins = U.val16;
        tokens[numtokens++] = ram_extract_and_tokenize(&U,ram,'T',(txind<<1) | 1,hp,format,sizeof(uint16_t)), numvouts = U.val16;
        if ( tokens[numtokens-1] == 0 || tokens[numtokens-2] == 0 )
        {
            printf("txind.%d (%d %d) numvins.%d numvouts.%d (%s) txid_rawind.%d (%p %p)\n",txind,*firstvinp,*firstvoutp,numvins,numvouts,txidstr,txid_rawind,tokens[numtokens-1],tokens[numtokens-2]);
            while ( 1 )
                portable_sleep(1);
            return(orignumtokens);
        }
        if ( (txid_rawind= ram_extractstring(txidstr,'t',ram,'T',(txind<<1),hp,format)) != 0 )
            tokens[numtokens++] = ram_createstring(ram,'T',(txind<<1),'t',txidstr,txid_rawind);
        else
        {
            printf("error ram_extractstring(%s,'t',txid.%d,%c) hp bitoffset.%d of %d\n",txidstr,txind,format,hp->bitoffset,hp->endpos);
            return(orignumtokens);
        }
    } else numtokens += num_rawtx_tokens(raw);
    if ( numvins > 0 )
    {
        lastnumtokens = numtokens;
        for (i=0; i<numvins; i++,(*firstvinp)++,lastnumtokens=numtokens)
            if ( (numtokens= ram_rawvin_huffscan(ram,tokens,numtokens,hp,format,*firstvinp)) == lastnumtokens )
                return(orignumtokens);
    }
    if ( numvouts > 0 )
    {
        lastnumtokens = numtokens;
        for (i=0; i<numvouts; i++,(*firstvoutp)++,lastnumtokens=numtokens)
            if ( (numtokens= ram_rawvout_huffscan(ram,tokens,numtokens,hp,format,*firstvoutp)) == lastnumtokens )
                return(orignumtokens);
    }
    //printf("1st vout.%d vin.%d\n",*firstvoutp,*firstvinp);
    return(numtokens);
}

struct ramchain_token **ram_purgetokens(int32_t *numtokensp,struct ramchain_token **tokens,int32_t numtokens)
{
    if ( numtokensp != 0 )
        *numtokensp = -1;
    return(0);
}

struct ramchain_token **ram_tokenize_bitstream(uint32_t *blocknump,int32_t *numtokensp,struct ramchain_info *ram,HUFF *hp,int32_t format)
{
    // 'V' packed structure using varints and varstrs
    // 'B' bitstream using varbits and rawind substitution for strings
    // 'H' bitstream using huffman codes
    struct ramchain_token **tokens = 0;
    int32_t i,numtx = 0,firstvin,firstvout,maxtokens,numtokens = 0;
    maxtokens = MAX_BLOCKTX * 2;
    union ramtypes U;
    uint64_t minted = 0;
    uint32_t blocknum,lastnumtokens;
    *blocknump = 0;
    ram_clear_alloc_space(&ram->Tmp);
    tokens = memalloc(&ram->Tmp,maxtokens * sizeof(*tokens));
    tokens[numtokens++] = ram_extract_and_tokenize(&U,ram,'B',0,hp,format,sizeof(uint32_t)), *blocknump = blocknum = U.val32;
    tokens[numtokens++] = ram_extract_and_tokenize(&U,ram,'B',0,hp,format,sizeof(uint16_t)), numtx = U.val16;
    tokens[numtokens++] = ram_extract_and_tokenize(&U,ram,'B',0,hp,format,sizeof(uint64_t)), minted = U.val64;
    if ( tokens[numtokens-1] == 0 || tokens[numtokens-2] == 0 || tokens[numtokens-3] == 0 )
    {
        printf("blocknum.%d numt.%d minted %.8f (%p %p %p)\n",*blocknump,numtx,dstr(minted),tokens[numtokens-1],tokens[numtokens-2],tokens[numtokens-3]);
        //while ( 1 )
        //    portable_sleep(1);
        return(ram_purgetokens(numtokensp,tokens,numtokens));
    }
    if ( numtx > 0 )
    {
        lastnumtokens = numtokens;
        for (i=firstvin=firstvout=0; i<numtx; i++)
            if ( (numtokens= ram_rawtx_huffscan(ram,tokens,numtokens,hp,format,i,&firstvin,&firstvout)) == lastnumtokens )
            {
                printf("block.%d parse error at token %d of %d | firstvin.%d firstvout.%d\n",blocknum,i,numtx,firstvin,firstvout);
                return(ram_purgetokens(numtokensp,tokens,numtokens));
            }
    }
    if ( numtokens > maxtokens )
        printf("numtokens.%d > maxtokens.%d\n",numtokens,maxtokens);
    *numtokensp = numtokens;
    return(tokens);
}

int32_t ram_verify(struct ramchain_info *ram,HUFF *hp,int32_t format)
{
    uint32_t blocknum = 0;
    int32_t checkformat,numtokens = 0;
    struct ramchain_token **tokens;
    hrewind(hp);
    checkformat = hp->buf[0], hp->ptr++, hp->bitoffset = 8;
    if ( checkformat != format )
        return(0);
    tokens = ram_tokenize_bitstream(&blocknum,&numtokens,ram,hp,format);
    if ( tokens != 0 )
    {
        ram_purgetokens(0,tokens,numtokens);
        return(blocknum);
    }
    return(-1);
}

int32_t ram_expand_token(struct rawblock *raw,struct ramchain_info *ram,struct ramchain_token *token)
{
    void *hashdata,*destptr;
    char hashstr[8192];
    int32_t datalen;
    uint64_t longspace;
    if ( (hashdata= ram_tokenstr(&longspace,&datalen,ram,hashstr,token)) != 0 )
    {
        if ( raw != 0 && (destptr= ram_getrawdest(raw,token)) != 0 )
        {
            //printf("[%ld] copy %d bytes to (%c.%c.%d)\n",(long)destptr-(long)raw,datalen,token->type>16?token->type:token->type+'0',token->selector,token->offset);
            memcpy(destptr,hashdata,datalen);
            return(datalen);
        }
        else printf("ram_expand_token: error finding destptr in rawblock\n");
    } else printf("ram_expand_token: error decoding token\n");
    return(-1);
}

int32_t ram_emit_and_free(int32_t compressflag,HUFF *hp,struct ramchain_info *ram,struct ramchain_token **tokens,int32_t numtokens)
{
    int32_t i;
    for (i=0; i<numtokens; i++)
        if ( tokens[i] != 0 )
            ram_emit_token(compressflag,hp,ram,tokens[i]);
    ram_purgetokens(0,tokens,numtokens);
    if ( 0 )
    {
        for (i=0; i<(hp->bitoffset>>3)+1; i++)
            printf("%02x ",hp->buf[i]);
        printf("emit_and_free: endpos.%d\n",hp->endpos);
    }
    return(hconv_bitlen(hp->endpos));
}

int32_t ram_expand_and_free(cJSON **jsonp,struct rawblock *raw,struct ramchain_info *ram,struct ramchain_token **tokens,int32_t numtokens,int32_t allocsize)
{
    int32_t i;
    ram_clear_rawblock(raw,0);
    for (i=0; i<numtokens; i++)
        if ( tokens[i] != 0 )
            ram_expand_token(raw,ram,tokens[i]);
    ram_purgetokens(0,tokens,numtokens);
    ram_patch_rawblock(raw);
    if ( jsonp != 0 )
        (*jsonp) = ram_rawblock_json(raw,allocsize);
    return(numtokens);
}

int32_t ram_compress(HUFF *hp,struct ramchain_info *ram,uint8_t *data,int32_t datalen)
{
    struct ramchain_token **tokens;
    int32_t numtokens;
    uint32_t format,blocknum;
    HUFF *srcbits;
    if ( hp != 0 )
    {
        hrewind(hp);
        hemit_bits(hp,'H',8);
        srcbits = hopen(ram->name,&ram->Perm,data,datalen,0);
        if ( hdecode_bits(&format,srcbits,8) != 8 || format != 'B' )
            printf("error hdecode_bits in ram_expand_rawinds format.%d != (%c) %d",format,'B','B');
        else if ( (tokens= ram_tokenize_bitstream(&blocknum,&numtokens,ram,srcbits,format)) != 0 )
        {
            hclose(srcbits);
            return(ram_emit_and_free(2,hp,ram,tokens,numtokens));
        } else printf("error tokenizing blockhex\n");
        hclose(srcbits);
    }
    return(-1);
}

int32_t ram_emitblock(HUFF *hp,int32_t destformat,struct ramchain_info *ram,struct rawblock *raw)
{
    struct ramchain_token **tokens;
    int32_t numtokens;
    if ( hp != 0 )
    {
        hrewind(hp);
        hemit_bits(hp,destformat,8);
        if ( (tokens= ram_tokenize_rawblock(&numtokens,ram,raw)) != 0 )
            return(ram_emit_and_free(destformat!='V',hp,ram,tokens,numtokens));
    }
    return(-1);
}

int32_t ram_expand_bitstream(cJSON **jsonp,struct rawblock *raw,struct ramchain_info *ram,HUFF *hp)
{
    struct ramchain_token **tokens;
    int32_t numtokens;
    uint32_t format,blocknum;
    ram_clear_rawblock(raw,0);
    if ( hp != 0 )
    {
        hrewind(hp);
        format = hp->buf[0], hp->ptr++, hp->bitoffset = 8;
        if ( 0 )
        {
            int i;
            for (i=0; i<=hp->endpos>>3; i++)
                printf("%02x ",hp->buf[i]);
            printf("(%c).%d\n",format,i);
        }
        if ( format != 'B' && format != 'V' && format != 'H' )
            printf("error hdecode_bits in ram_expand_rawinds format.%d != (%c/%c/%c) %d/%d/%d\n",format,'V','B','H','V','B','H');
        else if ( (tokens= ram_tokenize_bitstream(&blocknum,&numtokens,ram,hp,format)) != 0 )
            return(ram_expand_and_free(jsonp,raw,ram,tokens,numtokens,hp->allocsize));
        else printf("error expanding bitstream\n");
    }
    return(-1);
}

char *ram_blockstr(struct rawblock *tmp,struct ramchain_info *ram,struct rawblock *raw)
{
    cJSON *json = 0;
    char *retstr = 0;
    if ( (json= ram_rawblock_json(raw,0)) != 0 )
    {
        retstr = cJSON_Print(json);
        free_json(json);
    }
    return(retstr);
}

void ram_setformatstr(char *formatstr,int32_t format)
{
    if ( format == 'V' || format == 'B' || format == 'H' )
    {
        formatstr[1] = 0;
        formatstr[0] = format;
    }
    else sprintf(formatstr,"B%d",format);
}

void ram_setdirA(char *dirA,struct ramchain_info *ram)
{
	#ifndef _WIN32
    sprintf(dirA,"%s/ramchains/%s/bitstream",ram->dirpath,ram->name);
	#else
	sprintf(dirA,"%s\\ramchains\\%s\\bitstream",ram->dirpath,ram->name);
	#endif
}

void ram_setdirB(int32_t mkdirflag,char *dirB,struct ramchain_info *ram,uint32_t blocknum)
{
    static char lastdirB[1024];
    char dirA[1024];
    int32_t i;
    blocknum %= (64 * 64 * 64);
    ram_setdirA(dirA,ram);
    i = blocknum / (64 * 64);
	#ifndef _WIN32
    sprintf(dirB,"%s/%05x_%05x",dirA,i*64*64,(i+1)*64*64-1);
	#else
    sprintf(dirB,"%s\\%05x_%05x",dirA,i*64*64,(i+1)*64*64-1);
	#endif
    if ( mkdirflag != 0 && strcmp(dirB,lastdirB) != 0 )
    {
        ensure_dir(dirB);
        //printf("DIRB: (%s)\n",dirB);
        strcpy(lastdirB,dirB);
    }
}

void ram_setdirC(int mkdirflag,char *dirC,struct ramchain_info *ram,uint32_t blocknum)
{
    static char lastdirC[1024];
    char dirB[1024];
    int32_t i,j;
    blocknum %= (64 * 64 * 64);
    ram_setdirB(mkdirflag,dirB,ram,blocknum);
    i = blocknum / (64 * 64);
    j = (blocknum - (i * 64 * 64)) / 64;
	#ifndef _WIN32
    sprintf(dirC,"%s/%05x_%05x",dirB,i*64*64 + j*64,i*64*64 + (j+1)*64 - 1);
	#else
 	sprintf(dirC,"%s\\%05x_%05x",dirB,i*64*64 + j*64,i*64*64 + (j+1)*64 - 1);
	#endif
    if ( mkdirflag != 0 && strcmp(dirC,lastdirC) != 0 )
    {
        ensure_dir(dirC);
        //printf("DIRC: (%s)\n",dirC);
        strcpy(lastdirC,dirC);
    }
}

void ram_setfname(char *fname,struct ramchain_info *ram,uint32_t blocknum,char *str)
{
    char dirC[1024];
    ram_setdirC(0,dirC,ram,blocknum);
	#ifndef _WIN32
    sprintf(fname,"%s/%u.%s",dirC,blocknum,str);
	#else
    sprintf(fname,"%s\\%u.%s",dirC,blocknum,str);
	#endif
}

void ram_purge_badblock(struct ramchain_info *ram,uint32_t blocknum)
{
    char fname[1024];
    //ram_setfname(fname,ram,blocknum,"V");
    //delete_file(fname,0);
    ram_setfname(fname,ram,blocknum,"B");
    delete_file(fname,0);
    //ram_setfname(fname,ram,blocknum,"B64");
    //delete_file(fname,0);
    //ram_setfname(fname,ram,blocknum,"B4096");
   // delete_file(fname,0);
}

HUFF *ram_makehp(HUFF *tmphp,int32_t format,struct ramchain_info *ram,struct rawblock *tmp,int32_t blocknum)
{
    int32_t datalen;
    uint8_t *block;
    HUFF *hp = 0;
    hclear(tmphp);
    if ( (datalen= ram_emitblock(tmphp,format,ram,tmp)) > 0 )
    {
        //printf("ram_emitblock datalen.%d (%d) bitoffset.%d\n",datalen,hconv_bitlen(tmphp->endpos),tmphp->bitoffset);
        block = permalloc(ram->name,&ram->Perm,datalen,5);
        memcpy(block,tmphp->buf,datalen);
        hp = hopen(ram->name,&ram->Perm,block,datalen,0);
        hseek(hp,0,SEEK_END);
        //printf("ram_emitblock datalen.%d bitoffset.%d endpos.%d\n",datalen,hp->bitoffset,hp->endpos);
    } else printf("error emitblock.%d\n",blocknum);
    return(hp);
}

HUFF *ram_genblock(HUFF *tmphp,struct rawblock *tmp,struct ramchain_info *ram,int32_t blocknum,int32_t format,HUFF **prevhpp)
{
    HUFF *hp = 0;
    int32_t regenflag = 0;
    if ( format == 0 )
        format = 'V';
    if ( 0 && format == 'B' && prevhpp != 0 && (hp= *prevhpp) != 0 && strcmp(ram->name,"BTC") == 0 )
    {
        if ( ram_expand_bitstream(0,tmp,ram,hp) <= 0 )
        {
            char fname[1024],formatstr[16];
            ram_setformatstr(formatstr,'V');
            ram_setfname(fname,ram,blocknum,formatstr);
            //delete_file(fname,0);
            ram_setformatstr(formatstr,'B');
            ram_setfname(fname,ram,blocknum,formatstr);
            //delete_file(fname,0);
            regenflag = 1;
            printf("ram_genblock fatal error generating %s blocknum.%d\n",ram->name,blocknum);
            //exit(-1);
        }
        hp = 0;
    }
    if ( hp == 0 )
    {
        if ( _get_blockinfo(tmp,ram,blocknum) > 0 )
        {
            if ( tmp->blocknum != blocknum )
            {
                printf("WARNING: genblock.%c for %d: got blocknum.%d numtx.%d minted %.8f\n",format,blocknum,tmp->blocknum,tmp->numtx,dstr(tmp->minted));
                return(0);
            }
        } else printf("error _get_blockinfo.(%u)\n",blocknum);
    }
    hp = ram_makehp(tmphp,format,ram,tmp,blocknum);
    return(hp);
}

HUFF *ram_getblock(struct ramchain_info *ram,uint32_t blocknum)
{
    HUFF **hpp;
    if ( (hpp= ram_get_hpptr(&ram->blocks,blocknum)) != 0 )
        return(*hpp);
    return(0);
}

HUFF *ram_verify_Vblock(struct ramchain_info *ram,uint32_t blocknum,HUFF *hp)
{
    int32_t datalen,err;
    HUFF **hpptr;
    if ( hp == 0 && ((hpptr= ram_get_hpptr(&ram->Vblocks,blocknum)) == 0 || (hp = *hpptr) == 0) )
    {
        printf("verify_Vblock: no hp found for hpptr.%p %d\n",hpptr,blocknum);
        return(0);
    }
    if ( (datalen= ram_expand_bitstream(0,ram->Vblocks.R,ram,hp)) > 0 )
    {
        _get_blockinfo(ram->Vblocks.R2,ram,blocknum);
        if ( (err= raw_blockcmp(ram->Vblocks.R2,ram->Vblocks.R)) == 0 )
        {
            //printf("OK ");
            return(hp);
        } else printf("verify_Vblock.%d err.%d\n",blocknum,err);
    } else printf("ram_expand_bitstream returned.%d\n",datalen);
    return(0);
}

HUFF *ram_verify_Bblock(struct ramchain_info *ram,uint32_t blocknum,HUFF *Bhp)
{
    int32_t format,i,n;
    HUFF **hpp,*hp = 0,*retval = 0;
    cJSON *json;
    char *jsonstrs[2],fname[1024],strs[2][2];
    memset(strs,0,sizeof(strs));
    n = 0;
    memset(jsonstrs,0,sizeof(jsonstrs));
    for (format='V'; format>='B'; format-=('V'-'B'),n++)
    {
        strs[n][0] = format;
        ram_setfname(fname,ram,blocknum,strs[n]);
        hpp = ram_get_hpptr((format == 'V') ? &ram->Vblocks : &ram->Bblocks,blocknum);
        if ( format == 'B' && hpp != 0 && *hpp == 0 )
            hp = Bhp;
        else if ( hpp != 0 )
            hp = *hpp;
        if ( hp != 0 )
        {
            //fprintf(stderr,"\n%c: ",format);
            json = 0;
            ram_expand_bitstream(&json,(format == 'V') ? ram->Bblocks.R : ram->Bblocks.R2,ram,hp);
            if ( json != 0 )
            {
                jsonstrs[n] = cJSON_Print(json);
                free_json(json), json = 0;
            }
            if ( format == 'B' )
                retval = hp;
        } else hp = 0;
    }
    if ( jsonstrs[0] == 0 || jsonstrs[1] == 0 || strcmp(jsonstrs[0],jsonstrs[1]) != 0 )
    {
        if ( 1 && jsonstrs[0] != 0 && jsonstrs[1] != 0 )
        {
            if ( raw_blockcmp(ram->Bblocks.R,ram->Bblocks.R2) != 0 )
            {
                printf("(%s).V vs (%s).B)\n",jsonstrs[0],jsonstrs[1]);
                retval = 0;
            }
        }
    }
    for (i=0; i<2; i++)
        if ( jsonstrs[i] != 0 )
            free(jsonstrs[i]);
    return(retval);
}

int32_t ram_calcsha256(bits256 *sha,HUFF *bitstreams[],int32_t num)
{
    int32_t i;
    bits256 tmp;
    memset(sha,0,sizeof(*sha));
    for (i=0; i<num; i++)
    {
        //printf("i.%d %p\n",i,bitstreams[i]);
        if ( bitstreams[i] != 0 && bitstreams[i]->buf != 0 )
            calc_sha256cat(tmp.bytes,sha->bytes,sizeof(*sha),bitstreams[i]->buf,hconv_bitlen(bitstreams[i]->endpos)), *sha = tmp;
        else
        {
            printf("ram_calcsha256(%d): bitstreams[%d] == 0? %p\n",num,i,bitstreams[i]);
            return(-1);
        }
    }
    return(num);
}

int32_t ram_save_bitstreams(bits256 *refsha,char *fname,HUFF *bitstreams[],int32_t num)
{
    FILE *fp;
    int32_t i,len = -1;
    if ( (fp= fopen(os_compatible_path(fname),"wb")) != 0 )
    {
        if ( ram_calcsha256(refsha,bitstreams,num) < 0 )
        {
            fclose(fp);
            return(-1);
        }
        printf("saving %s num.%d %llx\n",fname,num,(long long)refsha->txid);
        if ( fwrite(&num,1,sizeof(num),fp) == sizeof(num) )
        {
            if ( fwrite(refsha,1,sizeof(*refsha),fp) == sizeof(*refsha) )
            {
                for (i=0; i<num; i++)
                {
                    if ( bitstreams[i] != 0 )
                        hflush(fp,bitstreams[i]);
                    else
                    {
                        printf("unexpected null bitstream at %d\n",i);
                        break;
                    }
                }
                if ( i == num )
                    len = (int32_t)ftell(fp);
            }
        }
        fclose(fp);
    }
    return(len);
}

long *ram_load_bitstreams(struct ramchain_info *ram,bits256 *sha,char *fname,HUFF *bitstreams[],int32_t *nump)
{
    FILE *fp;
    long *offsets = 0;
    bits256 tmp,stored;
    int32_t i,x = -1,len = 0;
    if ( (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
    {
        memset(sha,0,sizeof(*sha));
        //fprintf(stderr,"loading %s\n",fname);
        if ( fread(&x,1,sizeof(x),fp) == sizeof(x) && ((*nump) == 0 || x == (*nump)) )
        {
            if ( (*nump) == 0 )
            {
                (*nump) = x;
                //printf("set num to %d\n",x);
            }
            offsets = calloc((*nump),sizeof(*offsets));
            if ( fread(&stored,1,sizeof(stored),fp) == sizeof(stored) )
            {
                //fprintf(stderr,"reading %s num.%d stored.%llx\n",fname,*nump,(long long)stored.txid);
                for (i=0; i<(*nump); i++)
                {
                    if ( (bitstreams[i]= hload(ram,&offsets[i],fp,0)) != 0 && bitstreams[i]->buf != 0 )
                        calc_sha256cat(tmp.bytes,sha->bytes,sizeof(*sha),bitstreams[i]->buf,bitstreams[i]->allocsize), *sha = tmp;
                    else printf("unexpected null bitstream at %d %p offset.%ld\n",i,bitstreams[i],offsets[i]);
                }
                if ( memcmp(sha,&stored,sizeof(stored)) != 0 )
                    printf("sha error %s %llx vs stored.%llx\n",fname,(long long)sha->txid,(long long)stored.txid);
            } else printf("error loading sha\n");
        } else printf("num mismatch %d != num.%d\n",x,(*nump));
        len = (int32_t)ftell(fp);
        //fprintf(stderr," len.%d ",len);
        fclose(fp);
    }
    //fprintf(stderr," return offsets.%p \n",offsets);
    return(offsets);
}

int32_t ram_map_bitstreams(int32_t verifyflag,struct ramchain_info *ram,int32_t blocknum,struct mappedptr *M,bits256 *sha,HUFF *blocks[],int32_t num,char *fname,bits256 *refsha)
{
    HUFF *hp;
    long *offsets;
    uint32_t checkblock;
    int32_t i,n,retval = 0,verified=0,rwflag = 0;
    retval = n = 0;
    if ( (offsets= ram_load_bitstreams(ram,sha,fname,blocks,&num)) != 0 )
    {
        // fprintf(stderr,"offset.%p num.%d refsha.%p sha.%p M.%p\n",offsets,num,refsha,sha,M);
        if ( refsha != 0 && memcmp(sha->bytes,refsha,sizeof(*sha)) != 0 )
        {
            fprintf(stderr,"refsha cmp error for %s %llx vs %llx\n",fname,(long long)sha->txid,(long long)refsha->txid);
            hpurge(blocks,num);
            free(offsets);
            return(0);
        }
        //fprintf(stderr,"about clear M\n");
        if ( M->fileptr != 0 )
            close_mappedptr(M);
        memset(M,0,sizeof(*M));
        //fprintf(stderr,"about to init_mappedptr\n");
        if ( init_mappedptr(0,M,0,rwflag,fname) != 0 )
        {
            //fprintf(stderr,"opened (%s) filesize.%lld\n",fname,(long long)M->allocsize);
            for (i=0; i<num; i++)
            {
                if ( i > 0 && (i % 4096) == 0 )
                    fprintf(stderr,"%.1f%% ",100.*(double)i/num);
                if ( (hp= blocks[i]) != 0 )
                {
                    hp->buf = (void *)((long)M->fileptr + offsets[i]);
                    //if ( (blocks[i]= hopen(ram->name,&ram->Perm,(void *)((long)M->fileptr + offsets[i]),hp->allocsize,0)) != 0 )
                    {
                        if ( verifyflag == 0 || (checkblock= ram_verify(ram,hp,'B')) == blocknum+i )
                            verified++;
                        else
                        {
                            printf("checkblock.%d vs %d (blocknum.%d + i.%d)\n",checkblock,blocknum+i,blocknum,i);
                            break;
                        }
                    }
                } else printf("ram_map_bitstreams: ram_map_bitstreams unexpected null hp at slot.%d\n",i);
            }
            if ( i == num )
            {
                retval = (int32_t)M->allocsize;
                //printf("loaded.%d from %d\n",num,blocknum);
                for (i=0; i<num; i++)
                    if ( (hp= blocks[i]) != 0 && ram->blocks.hps[blocknum+i] == 0 )
                        ram->blocks.hps[blocknum+i] = hp, n++;
            }
            else
            {
                printf("%s: only %d of %d blocks verified\n",fname,verified,num);
                for (i=0; i<num; i++)
                    if ( (hp= blocks[i]) != 0 )
                        hclose(hp), blocks[i] = 0;
                close_mappedptr(M);
                memset(M,0,sizeof(*M));
                delete_file(fname,0);
            }
        } else printf("Error mapping.(%s)\n",fname);
        free(offsets);
    }
    //printf("mapped.%d from %d\n",n,blocknum);
    //ram_clear_alloc_space(&ram->Tmp);
    return(n);
}

uint32_t ram_setcontiguous(struct mappedblocks *blocks)
{
    uint32_t i,n = blocks->firstblock;
    for (i=0; i<blocks->numblocks; i++)
    {
        if ( blocks->hps[i] != 0 )
            n++;
        else break;
    }
    return(n);
}

uint32_t ram_load_blocks(struct ramchain_info *ram,struct mappedblocks *blocks,uint32_t firstblock,int32_t numblocks)
{
    HUFF **hps;
    bits256 sha;
    struct mappedptr *M;
    char fname[1024],formatstr[16];
    uint32_t blocknum,i,flag,incr,n = 0;
    incr = (1 << blocks->shift);
    M = &blocks->M[0];
    ram_setformatstr(formatstr,blocks->format);
    flag = blocks->format == 'B';
    for (i=0; i<numblocks; i+=incr,M++)
    {
        blocknum = (firstblock + i);
        ram_setfname(fname,ram,blocknum,formatstr);
        //printf("loading (%s)\n",fname);
        if ( (hps= ram_get_hpptr(blocks,blocknum)) != 0 )
        {
            if ( blocks->format == 64 || blocks->format == 4096 )
            {
                if ( ram_map_bitstreams(flag,ram,blocknum,M,&sha,hps,incr,fname,0) <= 0 )
                {
                    //break;
                }
            }
            else
            {
                if ( (*hps= hload(ram,0,0,fname)) != 0 )
                {
                    if ( flag == 0 || ram_verify(ram,*hps,blocks->format) == blocknum )
                    {
                        //if ( flag != 0 )
                        //    fprintf(stderr,"=");
                        n++;
                        if ( blocks->format == 'B' && ram->blocks.hps[blocknum] == 0 )
                            ram->blocks.hps[blocknum] = *hps;
                    }
                    else hclose(*hps), *hps = 0;
                    if ( (n % 100) == 0 )
                        fprintf(stderr," total.%d loaded.(%s)\n",n,fname);
                } //else break;
            }
        }
    }
    blocks->contiguous = ram_setcontiguous(blocks);
    return(blocks->contiguous);
}

uint32_t ram_create_block(int32_t verifyflag,struct ramchain_info *ram,struct mappedblocks *blocks,struct mappedblocks *prevblocks,uint32_t blocknum)
{
    char fname[1024],formatstr[16];
    FILE *fp;
    bits256 sha,refsha;
    HUFF *hp,**hpptr,**hps,**prevhps;
    int32_t i,n,numblocks,datalen = 0;
    ram_setformatstr(formatstr,blocks->format);
    prevhps = ram_get_hpptr(prevblocks,blocknum);
    ram_setfname(fname,ram,blocknum,formatstr);
    //printf("check create.(%s)\n",fname);
    if ( blocks->format == 'V' && (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
    {
        fclose(fp);
       // if ( verifyflag == 0 )
            return(0);
    }
    if ( 0 && blocks->format == 'V' )
    {
        if ( _get_blockinfo(blocks->R,ram,blocknum) > 0 )
        {
            if ( (fp= fopen("test","wb")) != 0 )
            {
                hp = blocks->tmphp;
                if ( ram_rawblock_emit(hp,ram,blocks->R) <= 0 )
                    printf("error ram_rawblock_emit.%d\n",blocknum);
                hflush(fp,hp);
                fclose(fp);
                for (i=0; i<(hp->bitoffset>>3); i++)
                    printf("%02x ",hp->buf[i]);
                printf("ram_rawblock_emit\n");
            }
            if ( (hp= ram_genblock(blocks->tmphp,blocks->R2,ram,blocknum,blocks->format,0)) != 0 )
            {
                if ( (fp= fopen(os_compatible_path(fname),"wb")) != 0 )
                {
                    hflush(fp,hp);
                    fclose(fp);
                    compare_files("test",fname);
                    for (i=0; i<(hp->bitoffset>>3); i++)
                        printf("%02x ",hp->buf[i]);
                    printf("ram_genblock\n");
                }
                hclose(hp);
            }
        } else printf("error _get_blockinfo.%d\n",blocknum);
        return(0);
    }
    else if ( blocks->format == 'V' || blocks->format == 'B' )
    {
        //printf("create %s %d\n",formatstr,blocknum);
        if ( (hpptr= ram_get_hpptr(blocks,blocknum)) != 0 )
        {
            if ( (hp= ram_genblock(blocks->tmphp,blocks->R,ram,blocknum,blocks->format,prevhps)) != 0 )
            {
                //printf("block.%d created.%c block.%d numtx.%d minted %.8f\n",blocknum,blocks->format,blocks->R->blocknum,blocks->R->numtx,dstr(blocks->R->minted));
                if ( (fp= fopen(os_compatible_path(fname),"wb")) != 0 )
                {
                    hflush(fp,hp);
                    fclose(fp);
                }
                if ( ram_verify(ram,hp,blocks->format) == blocknum )
                {
                    if ( verifyflag != 0 && ((blocks->format == 'B') ? ram_verify_Bblock(ram,blocknum,hp) : ram_verify_Vblock(ram,blocknum,hp)) == 0 )
                    {
                        printf("error creating %cblock.%d\n",blocks->format,blocknum), datalen = 0;
                        delete_file(fname,0), hclose(hp);
                    }
                    else
                    {
                        datalen = (1 + hp->allocsize);
                        //if ( blocks->format == 'V' )
                        fprintf(stderr," %s CREATED.%c block.%d datalen.%d | RT.%u lag.%d\n",ram->name,blocks->format,blocknum,datalen+1,ram->S.RTblocknum,ram->S.RTblocknum-blocknum);
                        //else fprintf(stderr,"%s.B.%d ",ram->name,blocknum);
                        if ( 0 && *hpptr != 0 )
                        {
                            hclose(*hpptr);
                            *hpptr = 0;
                            printf("OVERWRITE.(%s) size.%ld bitoffset.%d allocsize.%d\n",fname,ftell(fp),hp->bitoffset,hp->allocsize);
                        }
                        *hpptr = hp;
                        if ( blocks->format != 'V' && ram->blocks.hps[blocknum] == 0 )
                            ram->blocks.hps[blocknum] = hp;
                    }
                } //else delete_file(fname,0), hclose(hp);
            } else printf("genblock error %s (%c) blocknum.%u\n",ram->name,blocks->format,blocknum);
        } else printf("%s.%u couldnt get hpp\n",formatstr,blocknum);
    }
    else if ( blocks->format == 64 || blocks->format == 4096 )
    {
        n = blocks->format;
        for (i=0; i<n; i++)
            if ( prevhps[i] == 0 )
                break;
        if ( i == n )
        {
            ram_setfname(fname,ram,blocknum,formatstr);
            hps = ram_get_hpptr(blocks,blocknum);
            if ( ram_save_bitstreams(&refsha,fname,prevhps,n) > 0 )
                numblocks = ram_map_bitstreams(verifyflag,ram,blocknum,&blocks->M[(blocknum-blocks->firstblock) >> blocks->shift],&sha,hps,n,fname,&refsha);
        } else printf("%s prev.%d missing blockptr at %d (%d of %d)\n",ram->name,prevblocks->format,blocknum+i,i,n);
    }
    else
    {
        printf("illegal format to blocknum.%d create.%d\n",blocknum,blocks->format);
        return(0);
    }
    if ( datalen != 0 )
    {
        blocks->sum += datalen;
        blocks->count += (1 << blocks->shift);
    }
    return(datalen);
}

FILE *ram_permopen(char *fname,char *coinstr)
{
    FILE *fp;
    if ( 0 && strcmp(coinstr,"BTC") == 0 )
        return(0);
    if ( (fp= fopen(os_compatible_path(fname),"rb+")) == 0 )
        fp= fopen(os_compatible_path(fname),"wb+");
    if ( fp == 0 )
    {
        printf("ram_permopen couldnt create (%s)\n",fname);
        exit(-1);
    }
    return(fp);
}

int32_t ram_init_hashtable(int32_t deletefile,uint32_t *blocknump,struct ramchain_info *ram,char type)
{
    long offset,len,fileptr;
    uint64_t datalen;
    char fname[1024],destfname[1024],str[64];
    uint8_t *hashdata;
    int32_t varsize,num,rwflag = 0;
    struct ramchain_hashtable *hash;
    struct ramchain_hashptr *ptr;
    hash = ram_gethash(ram,type);
    if ( deletefile != 0 )
        memset(hash,0,sizeof(*hash));
    strcpy(hash->coinstr,ram->name);
    hash->type = type;
    num = 0;
    //if ( ram->remotemode == 0 )
    {
        ram_sethashname(fname,hash,0);
        strcat(fname,".perm");
        hash->permfp = ram_permopen(fname,ram->name);
    }
    ram_sethashname(fname,hash,0);
    printf("inithashtable.(%s.%d) -> [%s]\n",ram->name,type,fname);
    if ( deletefile == 0 && (hash->newfp= fopen(os_compatible_path(fname),"rb+")) != 0 )
    {
        if ( init_mappedptr(0,&hash->M,0,rwflag,fname) == 0 )
            return(1);
        if ( hash->M.allocsize == 0 )
            return(1);
        fileptr = (long)hash->M.fileptr;
        offset = 0;
        while ( (varsize= (int32_t)hload_varint(&datalen,hash->newfp)) > 0 && (offset + datalen) <= hash->M.allocsize )
        {
            hashdata = (uint8_t *)(fileptr + offset);
            if ( num < 10 )
            {
                char hexbytes[8192];
                struct ramchain_hashptr *checkptr;
                init_hexbytes_noT(hexbytes,hashdata,varsize+datalen);
                HASH_FIND(hh,hash->table,hashdata,varsize + datalen,checkptr);
                fprintf(stderr,"%s offset %ld: varsize.%d datalen.%d created.(%s) ind.%d | checkptr.%p\n",ram->name,offset,(int)varsize,(int)datalen,(type != 'a') ? hexbytes :(char *)((long)hashdata+varsize),hash->ind+1,checkptr);
            }
            HASH_FIND(hh,hash->table,hashdata,varsize + datalen,ptr);
            if ( ptr != 0 )
            {
                printf("corrupted hashtable %s: offset.%ld\n",fname,offset);
                exit(-1);
            }
            ptr = (MAP_HUFF != 0 ) ? permalloc(ram->name,&ram->Perm,sizeof(*ptr),6) : calloc(1,sizeof(*ptr));
            ram_addhash(hash,ptr,hashdata,(int32_t)(varsize+datalen));
            offset += (varsize + datalen);
            fseek(hash->newfp,offset,SEEK_SET);
            num++;
        }
        printf("%s: loaded %d strings, ind.%d, offset.%ld allocsize.%llu %s\n",fname,num,hash->ind,offset,(long long)hash->M.allocsize,((sizeof(uint64_t)+offset) != hash->M.allocsize && offset != hash->M.allocsize) ? "ERROR":"OK");
        if ( offset != hash->M.allocsize && offset != (hash->M.allocsize-sizeof(uint64_t)) )
        {
            //*blocknump = ram_load_blockcheck(hash->newfp);
            if ( (offset+sizeof(uint64_t)) != hash->M.allocsize )
            {
                printf("offset.%ld + 8 %ld != %ld allocsize\n",offset,(offset+sizeof(uint64_t)),(long)hash->M.allocsize);
                exit(-1);
            }
        }
        if ( (hash->ind + 1) > ram->maxind )
            ram->maxind = (hash->ind + 1);
        ram_sethashtype(str,hash->type);
        sprintf(destfname,"%s/ramchains/%s.%s",MGWROOT,ram->name,str);
        if ( (len= copy_file(fname,destfname)) > 0 )
            printf("copied (%s) -> (%s) %s\n",fname,destfname,_mbstr(len));
        return(0);
    } else hash->newfp = fopen(os_compatible_path(fname),"wb");
    return(1);
}

void ram_init_directories(struct ramchain_info *ram)
{
    char dirA[1024],dirB[1024],dirC[1024];
    int32_t i,j,blocknum = 0;
    ram_setdirA(dirA,ram);
    ensure_dir(dirA);
    for (i=0; blocknum+64<=ram->maxblock; i++)
    {
        ram_setdirB(1,dirB,ram,i * 64 * 64);
        for (j=0; j<64&&blocknum+64<=ram->maxblock; j++,blocknum+=64)
            ram_setdirC(1,dirC,ram,blocknum);
    }
}

void ram_setdispstr(char *buf,struct ramchain_info *ram,double startmilli)
{
    double estimatedV,estimatedB,estsizeV,estsizeB;
    estimatedV = estimate_completion(startmilli,ram->Vblocks.processed,(int32_t)ram->S.RTblocknum-ram->Vblocks.blocknum)/60000;
    estimatedB = estimate_completion(startmilli,ram->Bblocks.processed,(int32_t)ram->S.RTblocknum-ram->Bblocks.blocknum)/60000;
    estsizeV = (ram->Vblocks.sum / (1 + ram->Vblocks.count)) * ram->S.RTblocknum;
    estsizeB = (ram->Bblocks.sum / (1 + ram->Bblocks.count)) * ram->S.RTblocknum;
    sprintf(buf,"%-5s: RT.%d nonz.%d V.%d B.%d B64.%d B4096.%d | %s %s R%.2f | minutes: V%.1f B%.1f | outputs.%llu %.8f spends.%llu %.8f -> balance: %llu %.8f ave %.8f",ram->name,ram->S.RTblocknum,ram->nonzblocks,ram->Vblocks.blocknum,ram->Bblocks.blocknum,ram->blocks64.blocknum,ram->blocks4096.blocknum,_mbstr(estsizeV),_mbstr2(estsizeB),estsizeV/(estsizeB+1),estimatedV,estimatedB,(long long)ram->S.numoutputs,dstr(ram->S.totaloutputs),(long long)ram->S.numspends,dstr(ram->S.totalspends),(long long)(ram->S.numoutputs - ram->S.numspends),dstr(ram->S.totaloutputs - ram->S.totalspends),dstr(ram->S.totaloutputs - ram->S.totalspends)/(ram->S.numoutputs - ram->S.numspends));
}

void ram_disp_status(struct ramchain_info *ram)
{
    char buf[1024];
    int32_t i,n = 0;
    for (i=0; i<ram->S.RTblocknum; i++)
    {
        if ( ram->blocks.hps[i] != 0 )
            n++;
    }
    ram->nonzblocks = n;
    ram_setdispstr(buf,ram,ram->startmilli);
    fprintf(stderr,"%s\n",buf);
}

void ram_write_permentry(struct ramchain_hashtable *table,struct ramchain_hashptr *ptr)
{
    uint8_t databuf[8192];
    int32_t datalen,noexit,varlen = 0;
    uint64_t varint;
    long fpos,len;
    if ( table->permfp != 0 )
    {
        varlen += hdecode_varint(&varint,ptr->hh.key,0,9);
        datalen = ((int32_t)varint + varlen);
        fpos = ftell(table->permfp);
        if ( table->endpermpos < (fpos + datalen) )
        {
            fseek(table->permfp,0,SEEK_END);
            table->endpermpos = ftell(table->permfp);
            fseek(table->permfp,fpos,SEEK_SET);
        }
        if ( (fpos + datalen) <= table->endpermpos )
        {
            noexit = 0;
            if ( datalen < sizeof(databuf) )
            {
                if ( (len= fread(databuf,1,datalen,table->permfp)) == datalen )
                {
                    if ( memcmp(databuf,ptr->hh.key,datalen) != 0 )
                    {
                        printf("ram_write_permentry: memcmp error in permhash datalen.%d\n",datalen);
                        fseek(table->permfp,fpos,SEEK_SET);
                        noexit = 1;
                    }
                    else return;
                } else printf("ram_write_permentry: len.%ld != datalen.%d\n",len,datalen);
            } else printf("datalen.%d too big for databuf[%ld]\n",datalen,sizeof(databuf));
            if ( noexit == 0 )
                exit(-1); // unrecoverable
        }
        if ( fwrite(ptr->hh.key,1,datalen,table->permfp) != datalen )
        {
            printf("ram_write_permentry: error saving type.%d ind.%d datalen.%d\n",table->type,ptr->permind,datalen);
            exit(-1);
        }
        fflush(table->permfp);
        table->endpermpos = ftell(table->permfp);
    }
}

int32_t ram_rawvout_update(int32_t iter,uint32_t *script_rawindp,uint32_t *addr_rawindp,struct rampayload *txpayload,struct ramchain_info *ram,HUFF *hp,uint32_t blocknum,uint16_t txind,uint16_t vout,uint16_t numvouts,uint32_t txid_rawind,int32_t isinternal)
{
    struct rampayload payload;
    struct ramchain_hashtable *table;
    struct ramchain_hashptr *addrptr,*scriptptr;
    struct rawvout_huffs *pair;
    uint32_t scriptind,addrind;
    struct address_entry B;
    char *str,coinaddr[1024],txidstr[512],scriptstr[512];
    uint64_t value;
    int32_t unspendable,numbits = 0;
    *addr_rawindp = 0;
    numbits += hdecode_varbits(&scriptind,hp);
    numbits += hdecode_varbits(&addrind,hp);
    numbits += hdecode_valuebits(&value,hp);
    if ( toupper(iter) == 'H' )
    {
        if ( vout == 0 )
            pair = &ram->H.vout0, str = "vout0";
        else if ( vout == (numvouts - 1) )
            pair = &ram->H.voutn, str = "voutn";
        else if ( vout == 1 )
            pair = &ram->H.vout1, str = "vout1";
        else if ( vout == 2 )
            pair = &ram->H.vout2, str = "vout2";
        else pair = &ram->H.vouti, str = "vouti";
        huffpair_update(iter,ram,str,"script",&pair->script,scriptind,4);
        huffpair_update(iter,ram,str,"addr",&pair->addr,addrind,4);
        huffpair_update(iter,ram,str,"value",&pair->value,value,8);
    }
    table = ram_gethash(ram,'s');
    if ( scriptind > 0 && scriptind <= table->ind && (scriptptr= table->ptrs[scriptind]) != 0 )
    {
        ram_script(scriptstr,ram,scriptind);
        scriptptr->unspent = ram_check_redeemcointx(&unspendable,ram,scriptstr,blocknum);
        if ( iter != 1 )
        {
            if ( scriptptr->unspent != 0 )  // this is MGW redeemtxid
            {
                ram_txid(txidstr,ram,txid_rawind);
                printf("coin redeemtxid.(%s) with script.(%s)\n",txidstr,scriptstr);
                memset(&B,0,sizeof(B));
                B.blocknum = blocknum, B.txind = txind, B.v = vout;
                _ram_update_redeembits(ram,scriptptr->unspent,0,txidstr,&B);
            }
            if ( scriptptr->permind == 0 )
            {
                scriptptr->permind = ++ram->next_script_permind;
                ram_write_permentry(table,scriptptr);
                if ( scriptptr->permind != scriptptr->rawind )
                    ram->permind_changes++;
            }
        }
        table = ram_gethash(ram,'a');
        if ( addrind > 0 && addrind <= table->ind && (addrptr= table->ptrs[addrind]) != 0 )
        {
            if ( iter != 1 && addrptr->permind == 0 )
            {
                addrptr->permind = ++ram->next_addr_permind;
                ram_write_permentry(table,addrptr);
                if ( addrptr->permind != addrptr->rawind )
                    ram->permind_changes++;
            }
            *addr_rawindp = addrind;
            *script_rawindp = scriptind;
            if ( txpayload == 0 )
                addrptr->maxpayloads++;
            else
            {
                if ( addrptr->payloads == 0 )
                {
                    addrptr->maxpayloads += 2;
                    //printf("ptr.%p alloc max.%d for (%d %d %d)\n",ptr,ptr->maxpayloads,blocknum,txind,vout);
                    addrptr->payloads = calloc(addrptr->maxpayloads,sizeof(struct rampayload));
                }
                if ( addrptr->numpayloads >= addrptr->maxpayloads )
                {
                    //printf("realloc max.%d for (%d %d %d) with num.%d\n",ptr->maxpayloads,blocknum,txind,vout,ptr->numpayloads);
                    addrptr->maxpayloads = (addrptr->numpayloads + 16);
                    addrptr->payloads = realloc(addrptr->payloads,addrptr->maxpayloads * sizeof(struct rampayload));
                }
                if ( iter <= 2 )
                {
                    memset(&payload,0,sizeof(payload));
                    payload.B.blocknum = blocknum, payload.B.txind = txind, payload.B.v = vout;
                    if ( value > 1 )
                        payload.B.isinternal = isinternal;
                    payload.otherind = txid_rawind, payload.extra = scriptind, payload.value = value;
                    //if ( ram_script_nonstandard(ram,scriptind) != 0 )
                    //    addrptr->nonstandard = 1;
                    //if ( ram_script_multisig(ram,scriptind) != 0 )
                    //    addrptr->multisig = 1;
                    if ( ram_addr(coinaddr,ram,addrind) != 0 && coinaddr[0] == ram->multisigchar )
                        addrptr->multisig = 1;
                    if ( unspendable == 0 )
                        ram_addunspent(ram,coinaddr,txpayload,addrptr,&payload,addrind,addrptr->numpayloads);
                    addrptr->payloads[addrptr->numpayloads++] = payload;
                }
            }
            return(numbits);
        } else printf("ram_rawvout_update block.%d txind.%d vout.%d can find addrind.%d ptr.%p\n",blocknum,txind,vout,addrind,addrptr);
    } else printf("ram_rawvout_update block.%d txind.%d vout.%d can find scriptind.%d\n",blocknum,txind,vout,scriptind);
    return(-1);
}

int32_t ram_rawvin_update(int32_t iter,struct ramchain_info *ram,HUFF *hp,uint32_t blocknum,uint16_t txind,uint16_t vin,uint16_t numvins,uint32_t spendtxid_rawind)
{
    static struct address_entry zeroB;
    struct address_entry B,*bp;
    struct ramchain_hashptr *txptr;
    struct ramchain_hashtable *table;
    struct rawvin_huffs *pair;
    char *str;
    uint32_t txid_rawind;
    int32_t numbits = 0;
    uint16_t vout;
    table = ram_gethash(ram,'t');
    numbits = hdecode_varbits(&txid_rawind,hp);
    if ( txid_rawind > 0 && txid_rawind <= table->ind )
    {
        numbits += hdecode_smallbits(&vout,hp);
        if ( iter == 0 )
            return(numbits);
        else if ( toupper(iter) == 'H' )
        {
            if ( vin == 0 )
                pair = &ram->H.vin0, str = "vin0";
            else if ( vin == 1 )
                pair = &ram->H.vin1, str = "vin1";
            else pair = &ram->H.vini, str = "vini";
            huffpair_update(iter,ram,str,"txid",&pair->txid,txid_rawind,4);
            huffpair_update(iter,ram,str,"vout",&pair->vout,vout,2);
        }
        if ( (txptr= table->ptrs[txid_rawind]) != 0 && txptr->payloads != 0 )
        {
            if ( iter != 1 && txptr->permind == 0 )
                printf("raw_rawvin_update: unexpected null permind for txid_rawind.%d in blocknum.%d txind.%d\n",txid_rawind,blocknum,txind);
            memset(&B,0,sizeof(B)), B.blocknum = blocknum, B.txind = txind, B.v = vin, B.spent = 1;
            if ( vout < txptr->numpayloads )
            {
                if ( iter <= 2 )
                {
                    bp = &txptr->payloads[vout].spentB;
                    if ( memcmp(bp,&zeroB,sizeof(zeroB)) == 0 )
                        ram_markspent(ram,&txptr->payloads[vout],&B,txid_rawind);
                    else if ( memcmp(bp,&B,sizeof(B)) == 0 )
                        printf("duplicate spentB (%d %d %d)\n",B.blocknum,B.txind,B.v);
                    else
                    {
                        printf("interloper.%u perm.%u at (blocknum.%d txind.%d vin.%d)! (%d %d %d).%d vs (%d %d %d).%d >>>>>>> delete? <<<<<<<<\n",txid_rawind,txptr->permind,blocknum,txind,vin,bp->blocknum,bp->txind,bp->v,bp->spent,B.blocknum,B.txind,B.v,B.spent);
                        //if ( getchar() == 'y' )
                        ram_purge_badblock(ram,bp->blocknum);
                        ram_purge_badblock(ram,blocknum);
                        exit(-1);
                    }
                }
                return(numbits);
            } else printf("(%d %d %d) vout.%d overflows bp->v.%d\n",blocknum,txind,vin,vout,B.v);
        } else printf("rawvin_update: unexpected null table->ptrs[%d] or no payloads.%p\n",txid_rawind,txptr->payloads);
    } else printf("txid_rawind.%u out of range %d\n",txid_rawind,table->ind);
    return(-1);
}

int32_t ram_rawtx_update(int32_t iter,struct ramchain_info *ram,HUFF *hp,uint32_t blocknum,uint16_t txind)
{
    struct rampayload payload;
    struct ramchain_hashptr *txptr;
    char *str;
    struct address_entry B;
    uint32_t addr_rawind,script_rawind,txid_rawind = 0;
    int32_t i,internalvout,retval,isinternal,numbits = 0;
    uint16_t numvins,numvouts;
    struct rawtx_huffs *pair;
    struct ramchain_hashtable *table;
    table = ram_gethash(ram,'t');
    numbits += hdecode_smallbits(&numvins,hp);
    numbits += hdecode_smallbits(&numvouts,hp);
    numbits += hdecode_varbits(&txid_rawind,hp);
    if ( toupper(iter) == 'H' )
    {
        if ( txind == 0 )
            pair = &ram->H.tx0, str = "tx0";
        else if ( txind == 1 )
            pair = &ram->H.tx1, str = "tx1";
        else pair = &ram->H.txi, str = "txi";
        huffpair_update(iter,ram,str,"numvins",&pair->numvins,numvins,2);
        huffpair_update(iter,ram,str,"numvouts",&pair->numvouts,numvouts,2);
        huffpair_update(iter,ram,str,"txid",&pair->txid,txid_rawind,4);
    }
    if ( txid_rawind > 0 && txid_rawind <= table->ind )
    {
        if ( (txptr= table->ptrs[txid_rawind]) != 0 )
        {
            if ( iter != 1 && txptr->permind == 0 )
            {
                txptr->permind = ++ram->next_txid_permind;
                ram_write_permentry(table,txptr);
                if ( txptr->permind != txptr->rawind )
                    ram->permind_changes++;
            }
            if ( iter == 0 || iter == 2 )
            {
                memset(&payload,0,sizeof(payload));
                if ( txptr->payloads != 0 )
                {
                    payload.B = txptr->payloads[0].B;
                    printf("%p txid_rawind.%d txid already there: (block.%d txind.%d)[%d] vs B.(%d %d %d)\n",txptr,txid_rawind,blocknum,txind,numvouts,payload.B.blocknum,payload.B.txind,payload.B.v);
                }
                else
                {
                    payload.B.blocknum = blocknum, payload.B.txind = txind;
                    txptr->numpayloads = numvouts;
                    //printf("%p txid_rawind.%d maxpayloads.%d numpayloads.%d (%d %d %d)\n",txptr,txid_rawind,txptr->maxpayloads,txptr->numpayloads,blocknum,txind,numvouts);
                    txptr->payloads = (MAP_HUFF != 0) ? (struct rampayload *)permalloc(ram->name,&ram->Perm,txptr->numpayloads * sizeof(*txptr->payloads),7) : calloc(1,txptr->numpayloads * sizeof(*txptr->payloads));
                    for (payload.B.v=0; payload.B.v<numvouts; payload.B.v++)
                        txptr->payloads[payload.B.v] = payload;
                }
            }
            if ( numvins > 0 ) // alloc and update payloads in iter 0, no payload operations in iter 1
            {
                for (i=0; i<numvins; i++,numbits+=retval)
                    if ( (retval= ram_rawvin_update(iter,ram,hp,blocknum,txind,i,numvins,txid_rawind)) < 0 )
                        return(-1);
            }
            if ( numvouts > 0 ) // just count number of payloads needed in iter 0, iter 1 allocates and updates
            {
                internalvout = (numvouts - 1);
                memset(&B,0,sizeof(B));
                B.blocknum = blocknum, B.txind = txind;
                for (i=isinternal=0; i<numvouts; i++,numbits+=retval,B.v++)
                {
                    if ( (retval= ram_rawvout_update(iter,&script_rawind,&addr_rawind,iter==0?0:&txptr->payloads[i],ram,hp,blocknum,txind,i,numvouts,txid_rawind,isinternal*(i == 0 || i == internalvout))) < 0 )
                        return(-2);
                    if ( i == 0 && (addr_rawind == ram->marker_rawind || addr_rawind == ram->marker2_rawind) )
                        isinternal = 1;
                    /*else if ( isinternal != 0 && (numredeems= ram_is_MGW_OP_RETURN(redeemtxids,ram,script_rawind)) != 0 )
                    {
                        ram_txid(txidstr,ram,txid_rawind);
                        printf("found OP_RETURN.(%s)\n",txidstr);
                        internalvout = (i + 1);
                        for (j=0; j<numredeems; j++)
                            _ram_update_redeembits(ram,redeemtxids[j],0,txidstr,&B);
                    }*/
                }
            }
            return(numbits);
        }
    } else printf("ram_rawtx_update: parse error\n");
    return(-3);
}


bits256 ram_snapshot(struct ramsnapshot *snap,struct ramchain_info *ram,HUFF **hps,int32_t num)
{
    bits256 hash;
    memset(snap,0,sizeof(*snap));
    snap->addrind = ram->next_addr_permind;
    if ( ram->addrhash.permfp != 0 )
        snap->addroffset = ftell(ram->addrhash.permfp);
    snap->txidind = ram->next_txid_permind;
    if ( ram->txidhash.permfp != 0 )
        snap->txidoffset = ftell(ram->txidhash.permfp);
    snap->scriptind = ram->next_script_permind;
    if ( ram->scripthash.permfp != 0 )
        snap->scriptoffset = ftell(ram->scripthash.permfp);
    if ( ram->permfp != 0 )
        snap->permoffset = ftell(ram->permfp);
    ram_calcsha256(&hash,hps,num);
    return(hash);
}

int32_t ram_rawblock_update(int32_t iter,struct ramchain_info *ram,HUFF *hp,uint32_t checkblocknum)
{
    uint16_t numtx;
    uint64_t minted;
    uint32_t blocknum;
    int32_t txind,numbits,retval,format,datalen = 0;
    hrewind(hp);
    format = hp->buf[datalen++], hp->ptr++, hp->bitoffset = 8;
    if ( format != 'B' )
    {
        printf("only format B supported for now: (%c) %d not\n",format,format);
        return(-1);
    }
    numbits = hdecode_varbits(&blocknum,hp);
    if ( blocknum != checkblocknum )
    {
        printf("ram_rawblock_update: blocknum.%d vs checkblocknum.%d\n",blocknum,checkblocknum);
        return(-1);
    }
    if ( iter != 1 )
    {
        if ( blocknum != ram->S.permblocks ) //PERMUTE_RAWINDS != 0 &&
        {
            printf("ram_rawblock_update: blocknum.%d vs ram->S.permblocks.%d\n",blocknum,ram->S.permblocks);
            return(-1);
        }
        if ( (blocknum % 64) == 63 )
        {
            ram->snapshots[blocknum >> 6].hash = ram_snapshot(&ram->snapshots[blocknum >> 6],ram,&ram->blocks.hps[(blocknum >> 6) << 6],1<<6);
            if ( (blocknum % 4096) == 4095 )
                ram_calcsha256(&ram->permhash4096[blocknum >> 12],&ram->blocks.hps[(blocknum >> 12) << 12],1 << 12);
        }
        ram->S.permblocks++;
    }
    numbits += hdecode_smallbits(&numtx,hp);
    numbits += hdecode_valuebits(&minted,hp);
    if ( toupper(iter) == 'H' )
    {
        huffpair_update(iter,ram,"","numtx",&ram->H.numtx,numtx,2);
        //huffpair_update(iter,ram,"minted",&ram->H.minted,minted,8);
    }
    if ( numtx > 0 )
    {
        for (txind=0; txind<numtx; txind++,numbits+=retval)
            if ( (retval= ram_rawtx_update(iter,ram,hp,blocknum,txind)) < 0 )
                return(-1);
    }
    datalen += hconv_bitlen(numbits);
    return(datalen);
}

int32_t ram_rawvout_conv(HUFF *permhp,struct ramchain_info *ram,HUFF *hp,uint32_t blocknum,uint16_t txind,uint16_t vout,uint16_t numvouts)
{
    struct ramchain_hashtable *table;
    struct ramchain_hashptr *addrptr,*scriptptr;
    uint32_t scriptind,addrind;
    uint64_t value;
    int32_t numbits = 0;
    numbits += hdecode_varbits(&scriptind,hp);
    numbits += hdecode_varbits(&addrind,hp);
    table = ram_gethash(ram,'s');
    if ( scriptind > 0 && scriptind <= table->ind && (scriptptr= table->ptrs[scriptind]) != 0 )
    {
        hemit_varbits(permhp,scriptptr->permind);
        table = ram_gethash(ram,'a');
        if ( addrind > 0 && addrind <= table->ind && (addrptr= table->ptrs[addrind]) != 0 )
        {
            hemit_varbits(permhp,addrptr->permind);
            numbits += hdecode_valuebits(&value,hp), hemit_valuebits(permhp,value);
            return(numbits);
        } else printf("ram_rawvout_update block.%d txind.%d vout.%d can find addrind.%d ptr.%p\n",blocknum,txind,vout,addrind,addrptr);
    } else printf("ram_rawvout_update block.%d txind.%d vout.%d can find scriptind.%d\n",blocknum,txind,vout,scriptind);
    return(-1);
}

int32_t ram_rawvin_conv(HUFF *permhp,struct ramchain_info *ram,HUFF *hp,uint32_t blocknum,uint16_t txind,uint16_t vin,uint16_t numvins)
{
    struct ramchain_hashptr *txptr;
    struct ramchain_hashtable *table;
    uint32_t txid_rawind;
    int32_t numbits = 0;
    uint16_t vout;
    table = ram_gethash(ram,'t');
    numbits = hdecode_varbits(&txid_rawind,hp);
    if ( txid_rawind > 0 && txid_rawind <= table->ind )
    {
        numbits += hdecode_smallbits(&vout,hp);
        if ( (txptr= table->ptrs[txid_rawind]) != 0 && txptr->payloads != 0 )
        {
            if ( vout < txptr->numpayloads )
            {
                hemit_varbits(permhp,txptr->permind);
                hemit_smallbits(permhp,vout);
                return(numbits);
            }
            else printf("(%d %d %d) vout.%d overflows bp->v.%d\n",blocknum,txind,vin,vout,vin);
        } else printf("rawvin_update: unexpected null table->ptrs[%d] or no payloads.%p\n",txid_rawind,txptr->payloads);
    } else printf("txid_rawind.%u out of range %d\n",txid_rawind,table->ind);
    return(-1);
}

int32_t ram_rawtx_conv(HUFF *permhp,struct ramchain_info *ram,HUFF *hp,uint32_t blocknum,uint16_t txind)
{
    struct ramchain_hashptr *txptr;
    uint32_t txid_rawind = 0;
    int32_t i,retval,numbits = 0;
    uint16_t numvins,numvouts;
    struct ramchain_hashtable *table;
    table = ram_gethash(ram,'t');
    numbits += hdecode_smallbits(&numvins,hp), hemit_smallbits(permhp,numvins);
    numbits += hdecode_smallbits(&numvouts,hp), hemit_smallbits(permhp,numvouts);
    numbits += hdecode_varbits(&txid_rawind,hp);
    if ( txid_rawind > 0 && txid_rawind <= table->ind )
    {
        if ( (txptr= table->ptrs[txid_rawind]) != 0 )
        {
            hemit_varbits(permhp,txptr->permind);
            if ( numvins > 0 ) // alloc and update payloads in iter 0, no payload operations in iter 1
            {
                for (i=0; i<numvins; i++,numbits+=retval)
                    if ( (retval= ram_rawvin_conv(permhp,ram,hp,blocknum,txind,i,numvins)) < 0 )
                        return(-1);
            }
            if ( numvouts > 0 ) // just count number of payloads needed in iter 0, iter 1 allocates and updates
            {
                for (i=0; i<numvouts; i++,numbits+=retval)
                {
                    if ( (retval= ram_rawvout_conv(permhp,ram,hp,blocknum,txind,i,numvouts)) < 0 )
                        return(-2);
                }
            }
            return(numbits);
        }
    } else printf("ram_rawtx_conv: parse error\n");
    return(-3);
}

HUFF *ram_conv_permind(HUFF *permhp,struct ramchain_info *ram,HUFF *hp,uint32_t checkblocknum)
{
    uint64_t minted; uint16_t numtx; uint32_t blocknum; int32_t txind,numbits,retval,format;
    hrewind(hp);
    hclear(permhp);
    format = hp->buf[0], hp->ptr++, hp->bitoffset = 8;
    permhp->buf[0] = format, permhp->ptr++, permhp->endpos = permhp->bitoffset = 8;
    if ( format != 'B' )
    {
        printf("only format B supported for now\n");
        return(0);
    }
    numbits = hdecode_varbits(&blocknum,hp);
    if ( blocknum != checkblocknum )
    {
        printf("ram_conv_permind: hp->blocknum.%d vs checkblocknum.%d\n",blocknum,checkblocknum);
        //return(0);
        blocknum = checkblocknum;
    }
    hemit_varbits(permhp,blocknum);
    numbits += hdecode_smallbits(&numtx,hp), hemit_smallbits(permhp,numtx);
    numbits += hdecode_valuebits(&minted,hp), hemit_valuebits(permhp,minted);
    if ( numtx > 0 )
    {
        for (txind=0; txind<numtx; txind++,numbits+=retval)
            if ( (retval= ram_rawtx_conv(permhp,ram,hp,blocknum,txind)) < 0 )
            {
                printf("ram_conv_permind: blocknum.%d txind.%d parse error\n",blocknum,txind);
                return(0);
            }
    }
    //printf("hp.%d (end.%d bit.%d) -> permhp.%d (end.%d bit.%d)\n",datalen,hp->endpos,hp->bitoffset,hconv_bitlen(permhp->bitoffset),permhp->endpos,permhp->bitoffset);
    if ( ram->permfp != 0 )
        hsync(ram->permfp,permhp,(void *)ram->R3);
    return(permhp);
}

char *ram_searchpermind(char *permstr,struct ramchain_info *ram,char type,uint32_t permind)
{
    struct ramchain_hashtable *hash = ram_gethash(ram,type);
    int32_t i;
    permstr[0] = 0;
    //printf("(%c) searchpermind.(%d) ind.%d\n",type,permind,hash->ind);
    for (i=1; i<=hash->ind; i++)
        if ( hash->ptrs[i] != 0 && hash->ptrs[i]->permind == permind )
        {
            ram_script(permstr,ram,i);
            return(permstr);
        }
    return(0);
}

void ram_update_MGWunspents(struct ramchain_info *ram,char *addr,int32_t vout,uint32_t txid_rawind,uint32_t script_rawind,uint64_t value)
{
    struct cointx_input *vin;
    if ( ram->MGWnumunspents >= ram->MGWmaxunspents )
    {
        ram->MGWmaxunspents += 512;
        ram->MGWunspents = realloc(ram->MGWunspents,sizeof(*ram->MGWunspents) * ram->MGWmaxunspents);
        memset(&ram->MGWunspents[ram->MGWnumunspents],0,(ram->MGWmaxunspents - ram->MGWnumunspents) * sizeof(*ram->MGWunspents));
    }
    vin = &ram->MGWunspents[ram->MGWnumunspents++];
    if ( ram_script(vin->sigs,ram,script_rawind) != 0 && ram_txid(vin->tx.txidstr,ram,txid_rawind) != 0 )
    {
        strcpy(vin->coinaddr,addr);
        vin->value = value;
        vin->tx.vout = vout;
    } else printf("ram_update_MGWunspents: decode error for (%s) vout.%d txid.%d script.%d (%.8f)\n",addr,vout,txid_rawind,script_rawind,dstr(value));
}

uint64_t ram_verify_txstillthere(struct ramchain_info *ram,char *txidstr,struct address_entry *bp)
{
    char *retstr = 0;
    cJSON *txjson,*voutsobj;
    int32_t numvouts;
    uint64_t value = 0;
    if ( (retstr= _get_transaction(ram,txidstr)) != 0 )
    {
        if ( (txjson= cJSON_Parse(retstr)) != 0 )
        {
            if ( (voutsobj= cJSON_GetObjectItem(txjson,"vout")) != 0 && is_cJSON_Array(voutsobj) != 0 && (numvouts= cJSON_GetArraySize(voutsobj)) > 0  && bp->v < numvouts )
                value = conv_cJSON_float(cJSON_GetArrayItem(voutsobj,bp->v),"value");
           free_json(txjson);
        } else printf("update_txid_infos parse error.(%s)\n",retstr);
        free(retstr);
    } else printf("error getting.(%s)\n",txidstr);
    return(value);
}

uint64_t calc_addr_unspent(struct ramchain_info *ram,struct multisig_addr *msig,char *addr,struct rampayload *addrpayload)
{
    uint64_t MGWtransfer_asset(cJSON **transferjsonp,int32_t forceflag,uint64_t nxt64bits,char *depositors_pubkey,struct NXT_asset *ap,uint64_t value,char *coinaddr,char *txidstr,struct address_entry *entry,uint32_t *buyNXTp,char *srvNXTADDR,char *srvNXTACCTSECRET,int32_t deadline);
    uint64_t nxt64bits,pending = 0;
    char txidstr[4096];
    struct NXT_asset *ap = ram->ap;
    struct NXT_assettxid *tp;
    int32_t j;
    if ( ap != 0 )
    {
        ram_txid(txidstr,ram,addrpayload->otherind);
        for (j=0; j<ap->num; j++)
        {
            tp = ap->txids[j];
            if ( tp->cointxid != 0 && strcmp(tp->cointxid,txidstr) == 0 )
            {
                if ( ram_mark_depositcomplete(ram,tp,tp->coinblocknum) != 0 )
                    _complete_assettxid(ram,tp);
                break;
            }
        }
        //if ( strcmp("9908a63216f866650f81949684e93d62d543bdb06a23b6e56344e1c419a70d4f",txidstr) == 0 )
        //    printf("calc_addr_unspent.(%s) j.%d of apnum.%d valid.%d msig.%p\n",txidstr,j,ap->num,_valid_txamount(ram,addrpayload->value),msig);
        if ( (addrpayload->pendingdeposit != 0 || j == ap->num) && _valid_txamount(ram,addrpayload->value,msig->multisigaddr) > 0 && msig != 0 )
        {
            //printf("addr_unspent.(%s)\n",msig->NXTaddr);
            if ( (nxt64bits= _calc_nxt64bits(msig->NXTaddr)) != 0 )
            {
                printf("deposit.(%s/%d %d,%d %s %.8f)rt%d_%d_%d_%d.g%d -> NXT.%s %d\n",txidstr,addrpayload->B.v,addrpayload->B.blocknum,addrpayload->B.txind,addr,dstr(addrpayload->value),ram->S.NXT_is_realtime,ram->S.enable_deposits,(addrpayload->B.blocknum + ram->depositconfirms) <= ram->S.RTblocknum,ram->S.MGWbalance >= 0,(int32_t)(nxt64bits % NUM_GATEWAYS),msig->NXTaddr,ram->S.NXT_is_realtime != 0 && (addrpayload->B.blocknum + ram->depositconfirms) <= ram->S.RTblocknum && ram->S.enable_deposits != 0);
                pending += addrpayload->value;
                if ( ram_MGW_ready(ram,addrpayload->B.blocknum,0,nxt64bits,0) > 0 )
                {
                    if ( ram_verify_txstillthere(ram,txidstr,&addrpayload->B) != addrpayload->value )
                    {
                        printf("ram_calc_unspent: tx gone due to a fork. (%d %d %d) txid.%s %.8f\n",addrpayload->B.blocknum,addrpayload->B.txind,addrpayload->B.v,txidstr,dstr(addrpayload->value));
                        exit(1); // seems the best thing to do
                    }
                    if ( MGWtransfer_asset(0,1,nxt64bits,msig->NXTpubkey,ram->ap,addrpayload->value,msig->multisigaddr,txidstr,&addrpayload->B,&msig->buyNXT,ram->srvNXTADDR,ram->srvNXTACCTSECRET,ram->DEPOSIT_XFER_DURATION) == addrpayload->value )
                        addrpayload->pendingdeposit = 0;
                }
            }
        }
    }
    return(pending);
}

uint64_t ram_calc_unspent(uint64_t *pendingp,int32_t *calc_numunspentp,struct ramchain_hashptr **addrptrp,struct ramchain_info *ram,char *addr,int32_t MGWflag)
{
    //char redeemScript[8192],normaladdr[8192];
    uint64_t pending,unspent = 0;
    struct multisig_addr *msig;
    int32_t i,numpayloads,n = 0;
    struct rampayload *payloads;
    if ( calc_numunspentp != 0 )
        *calc_numunspentp = 0;
    pending = 0;
    if ( (payloads= ram_addrpayloads(addrptrp,&numpayloads,ram,addr)) != 0 && numpayloads > 0 )
    {
        msig = find_msigaddr(addr);
        for (i=0; i<numpayloads; i++)
        {
            /*{
                char txidstr[512];
                ram_txid(txidstr,ram,payloads[i].otherind);
                if ( strcmp("9908a63216f866650f81949684e93d62d543bdb06a23b6e56344e1c419a70d4f",txidstr) == 0 )
                    printf("txid.(%s).%d pendingdeposit.%d %.8f\n",txidstr,payloads[i].B.v,payloads[i].pendingdeposit,dstr(payloads[i].value));
            }*/
            if ( payloads[i].B.spent == 0 )
            {
                unspent += payloads[i].value, n++;
                if ( MGWflag != 0 && payloads[i].pendingsend == 0 )
                {
                    if ( strcmp(addr,"bYjsXxENs4u7raX3EPyrgqPy46MUrq4k8h") != 0 && strcmp(addr,"bKzDfRnGGTDwqNZWGCXa42DRdumAGskqo4") != 0 ) // addresses made with different third dev MGW server for BTCD only
                        ram_update_MGWunspents(ram,addr,payloads[i].B.v,payloads[i].otherind,payloads[i].extra,payloads[i].value);
                }
            }
            if ( payloads[i].pendingdeposit != 0 )
                pending += calc_addr_unspent(ram,msig,addr,&payloads[i]);
        }
    }
    if ( calc_numunspentp != 0 )
        *calc_numunspentp = n;
    if ( pendingp != 0 )
        (*pendingp) += pending;
    return(unspent);
}

uint64_t ram_calc_MGWunspent(uint64_t *pendingp,struct ramchain_info *ram)
{
    struct multisig_addr **msigs;
    int32_t i,n,m;
    uint64_t pending,smallest,val,unspent = 0;
    pending = 0;
    ram->MGWnumunspents = 0;
    if ( ram->MGWunspents != 0 )
        memset(ram->MGWunspents,0,sizeof(*ram->MGWunspents) * ram->MGWmaxunspents);
    if ( (msigs= (struct multisig_addr **)copy_all_DBentries(&n,MULTISIG_DATA)) != 0 )
    {
        ram->nummsigs = n;
        ram->MGWsmallest[0] = ram->MGWsmallestB[0] = 0;
        for (smallest=i=m=0; i<n; i++)
        {
            if ( (val= ram_calc_unspent(&pending,0,0,ram,msigs[i]->multisigaddr,1)) != 0 )
            {
                unspent += val;
                if ( smallest == 0 || val < smallest )
                {
                    smallest = val;
                    strcpy(ram->MGWsmallestB,ram->MGWsmallest);
                    strcpy(ram->MGWsmallest,msigs[i]->multisigaddr);
                }
                else if ( ram->MGWsmallestB[0] == 0 && strcmp(ram->MGWsmallest,msigs[i]->multisigaddr) != 0 )
                    strcpy(ram->MGWsmallestB,msigs[i]->multisigaddr);
            }
            free(msigs[i]);
        }
        free(msigs);
        if ( Debuglevel > 2 )
            printf("MGWnumunspents.%d smallest (%s %.8f)\n",ram->MGWnumunspents,ram->MGWsmallest,dstr(smallest));
    }
    *pendingp = pending;
    return(unspent);
}

int32_t ram_getsources(uint64_t *sources,struct ramchain_info *ram,uint32_t blocknum,int32_t numblocks)
{
    int32_t i,n = 0;
    struct MGWstate S;
    for (i=0; i<(int)(sizeof(ram->remotesrcs)/sizeof(*ram->remotesrcs)); i++)
    {
        S = ram->remotesrcs[i];
        if ( S.nxt64bits == 0 )
            break;
        if ( S.permblocks >= (blocknum + numblocks) )
            sources[n++] = S.nxt64bits;
    }
    return(n);
}

uint32_t ram_process_blocks(struct ramchain_info *ram,struct mappedblocks *blocks,struct mappedblocks *prev,double timebudget)
{
    HUFF **hpptr,*hp = 0;
    char formatstr[16];
    double estimated,startmilli = ram_millis();
    int32_t newflag,processed = 0;
    if ( ram->remotemode != 0 )
    {
        if ( blocks->format == 'B' )
        {
        }
        return(processed);
    }
    ram_setformatstr(formatstr,blocks->format);
    //printf("%s shift.%d %-5s.%d %.1f min left | [%d < %d]? %f %f timebudget %f\n",formatstr,blocks->shift,ram->name,blocks->blocknum,estimated,(blocks->blocknum >> blocks->shift),(prev->blocknum >> blocks->shift),ram_millis(),(startmilli + timebudget),timebudget);
    while ( (blocks->blocknum >> blocks->shift) < (prev->blocknum >> blocks->shift) && ram_millis() < (startmilli + timebudget) )
    {
        //printf("inside (%d) block.%d\n",blocks->format,blocks->blocknum);
        newflag = (ram->blocks.hps[blocks->blocknum] == 0);
        ram_create_block(1,ram,blocks,prev,blocks->blocknum), processed++;
        if ( (hpptr= ram_get_hpptr(blocks,blocks->blocknum)) != 0 && (hp= *hpptr) != 0 )
        {
            if ( blocks->format == 'B' && newflag != 0 )
            {
                if ( ram_rawblock_update(2,ram,hp,blocks->blocknum) < 0 )
                {
                    printf("FATAL: error updating block.%d %c\n",blocks->blocknum,blocks->format);
                    while ( 1 ) portable_sleep(1);
                }
                if ( ram->permfp != 0 )
                {
                    ram_conv_permind(ram->tmphp2,ram,hp,blocks->blocknum);
                    fflush(ram->permfp);
                }
            }
            //else printf("hpptr.%p hp.%p newflag.%d\n",hpptr,hp,newflag);
        } //else printf("ram_process_blocks: hpptr.%p hp.%p\n",hpptr,hp);
        blocks->processed += (1 << blocks->shift);
        blocks->blocknum += (1 << blocks->shift);
        estimated = estimate_completion(startmilli,blocks->processed,(int32_t)ram->S.RTblocknum-blocks->blocknum) / 60000.;
//break;
    }
    //printf("(%d >> %d) < (%d >> %d)\n",blocks->blocknum,blocks->shift,prev->blocknum,blocks->shift);
    return(processed);
}

uint64_t ram_unspent_json(cJSON **arrayp,char *destcoin,double rate,char *coin,char *addr)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    uint64_t unspent = 0;
    cJSON *item;
    if ( ram != 0 )
    {
        if ( (unspent= ram_calc_unspent(0,0,0,ram,addr,0)) != 0 && arrayp != 0 )
        {
            item = cJSON_CreateObject();
            cJSON_AddItemToObject(item,addr,cJSON_CreateNumber(dstr(unspent)));
            if ( rate != 0. )
            {
                unspent *= rate;
                cJSON_AddItemToObject(item,destcoin,cJSON_CreateNumber(dstr(unspent)));
            }
            cJSON_AddItemToArray(*arrayp,item);
        }
        else if ( rate != 0. )
            unspent *= rate;
    }
    return(unspent);
}

static int _decreasing_double_cmp(const void *a,const void *b)
{
#define double_a (*(double *)a)
#define double_b (*(double *)b)
	if ( double_b > double_a )
		return(1);
	else if ( double_b < double_a )
		return(-1);
	return(0);
#undef double_a
#undef double_b
}

/*static int _decreasing_uint64_cmp(const void *a,const void *b)
 {
 #define uint64_a (*(uint64_t *)a)
 #define uint64_b (*(uint64_t *)b)
 if ( uint64_b > uint64_a )
 return(1);
 else if ( uint64_b < uint64_a )
 return(-1);
 return(0);
 #undef uint64_a
 #undef uint64_b
 }*/

char **ram_getallstrs(int32_t *numstrsp,struct ramchain_info *ram)
{
    char **strs = 0;
    uint64_t varint;
    int32_t i = 0;
    struct ramchain_hashtable *hash;
    struct ramchain_hashptr *hp,*tmp;
    hash = ram_gethash(ram,'a');
    *numstrsp = HASH_COUNT(hash->table);
    strs = calloc(*numstrsp,sizeof(*strs));
    HASH_ITER(hh,hash->table,hp,tmp)
    strs[i++] = (char *)((long)hp->hh.key + hdecode_varint(&varint,hp->hh.key,0,9));
    if ( i != *numstrsp )
        printf("ram_getalladdrs HASH_COUNT.%d vs i.%d\n",*numstrsp,i);
    return(strs);
}

struct ramchain_hashptr **ram_getallstrptrs(int32_t *numstrsp,struct ramchain_info *ram,char type)
{
    struct ramchain_hashptr **strs = 0;
    struct ramchain_hashtable *hash;
    hash = ram_gethash(ram,type);
    *numstrsp = hash->ind;
    strs = calloc(*numstrsp+1,sizeof(*strs));
    memcpy(strs,hash->ptrs+1,(*numstrsp) * sizeof(*strs));
    return(strs);
}

char *ramaddrlist(char *origargstr,char *sender,char *previpaddr,char *coin)
{
    cJSON *json,*array,*item;
    char retbuf[1024],coinaddr[MAX_JSON_FIELD],account[MAX_JSON_FIELD],*retstr = 0;
    uint64_t value;
    uint32_t rawind;
    struct ramchain_hashtable *addrhash;
    struct ramchain_hashptr *addrptr;
    struct rampayload *payloads;
    int32_t i,j,k,z,n,m,num,numpayloads,errs,ismine,ismultisig,count = 0;
    struct ramchain_info *ram = get_ramchain_info(coin);
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    errs = 0;
    addrhash = ram_gethash(ram,'a');
    if ( (json= _get_localaddresses(ram)) != 0 )
    {
        if ( is_cJSON_Array(json) != 0 && (n= cJSON_GetArraySize(json)) > 0 )
        {
            for (i=0; i<n; i++)
            {
                if ( (array= cJSON_GetArrayItem(json,i)) != 0 && is_cJSON_Array(array) != 0 && (m= cJSON_GetArraySize(array)) > 0 )
                {
                    for (j=0; j<m; j++)
                    {
                        item = cJSON_GetArrayItem(array,j);
                        if ( (item= cJSON_GetArrayItem(array,j)) != 0 && is_cJSON_Array(item) != 0 && (num= cJSON_GetArraySize(item)) > 0 )
                        {
                            value = coinaddr[0] = account[0] = 0;
                            for (k=0; k<num; k++)
                            {
                                if ( k == 0 )
                                    copy_cJSON(coinaddr,cJSON_GetArrayItem(item,0));
                                else if ( k == 1 )
                                    value = (SATOSHIDEN * get_API_float(cJSON_GetArrayItem(item,1)));
                                else if ( k == 2 )
                                    copy_cJSON(account,cJSON_GetArrayItem(item,2));
                                else printf("ramaddrlist unexpected item array size: %d\n",num);
                            }
                            rawind = 0;
                            if ( coinaddr[0] != 0 && (rawind= ram_addrind(ram,coinaddr)) != 0 )
                            {
                                if ( (payloads= ram_addrpayloads(&addrptr,&numpayloads,ram,coinaddr)) != 0 && addrptr != 0 && numpayloads > 0 )
                                {
                                    addrptr->mine = 1;
                                    printf("%d: (%s).%-6d %.8f %.8f (%s) %s\n",count,coinaddr,rawind,dstr(addrptr->unspent),dstr(value),account,(addrptr->unspent != value) ? "STAKING": "");
                                    errs += (addrptr->unspent != value);
                                    count++;
                                } else printf("ramaddrlist error finding rawind.%d\n",rawind);
                            } else printf("ramaddrlist no coinaddr for item or null rawind.%d\n",rawind);
                        } else printf("ramaddrlist unexpected item not array\n");
                    }
                } else printf("ramaddrlist unexpected array not array\n");
            }
        } else printf("ramaddrlist unexpected json not array\n");
        free_json(json);
        n = m = 0;
        for (z=1; z<=addrhash->ind; z++)
        {
            if ( (addrptr= addrhash->ptrs[z]) != 0 && (addrptr->multisig != 0 || addrptr->mine != 0) && ram_decode_hashdata(coinaddr,'a',addrptr->hh.key) != 0 )
            {
                if ( 0 && addrptr->mine == 0 && addrptr->verified == 0 )
                {
                    addrptr->verified = _verify_coinaddress(account,&ismultisig,&ismine,ram,coinaddr);
                    if ( ismultisig != 0 )
                        addrptr->multisig = 1;
                    if ( ismine != 0 )
                        addrptr->mine = 1;
                }
                if ( addrptr->mine != 0 )
                    m++;
                if ( addrptr->multisig != 0 || addrptr->mine != 0 )
                    printf("n.%d check.(%s) account.(%s) multisig.%d nonstandard.%d mine.%d verified.%d\n",n,coinaddr,account,addrptr->multisig,addrptr->nonstandard,addrptr->mine,addrptr->verified), n++;
            }
        }
        sprintf(retbuf,"{\"result\":\"addrlist\",\"multisig\":%d,\"mine\":%d,\"total\":%d}",n,m,count);
        retstr = clonestr(retbuf);
    } else retstr = clonestr("{\"error\":\"ramaddrlist no data\"}");
    return(retstr);
}

cJSON *ram_address_entry_json(struct address_entry *bp)
{
    cJSON *array = cJSON_CreateArray();
    cJSON_AddItemToArray(array,cJSON_CreateNumber(bp->blocknum));
    cJSON_AddItemToArray(array,cJSON_CreateNumber(bp->txind));
    cJSON_AddItemToArray(array,cJSON_CreateNumber(bp->v));
    return(array);
}

void ram_payload_json(cJSON *json,struct rampayload *payload,int32_t spentflag)
{
    cJSON_AddItemToObject(json,"n",cJSON_CreateNumber(payload->B.v));
    cJSON_AddItemToObject(json,"value",cJSON_CreateNumber(dstr(payload->value)));
    if ( payload->B.isinternal != 0 )
        cJSON_AddItemToObject(json,"MGWinternal",cJSON_CreateNumber(1));
    if ( spentflag != 0 && payload->B.spent != 0 )
        cJSON_AddItemToObject(json,"spent",ram_address_entry_json(&payload->spentB));
}

cJSON *ram_addrpayload_json(struct ramchain_info *ram,struct rampayload *payload)
{
    char txidstr[8192];
    ram_txid(txidstr,ram,payload->otherind);
    cJSON *json = cJSON_CreateObject();
    if ( payload->pendingdeposit != 0 )
        cJSON_AddItemToObject(json,"pendingdeposit",cJSON_CreateNumber(1));
    cJSON_AddItemToObject(json,"txid",cJSON_CreateString(txidstr));
    cJSON_AddItemToObject(json,"txid_rawind",cJSON_CreateNumber(payload->otherind));
    cJSON_AddItemToObject(json,"txout",ram_address_entry_json(&payload->B));
    cJSON_AddItemToObject(json,"scriptind",cJSON_CreateNumber(payload->extra));
    ram_payload_json(json,payload,1);
    return(json);
}

cJSON *ram_txpayload_json(struct ramchain_info *ram,struct rampayload *txpayload,char *spent_txidstr,int32_t spent_vout)
{
    cJSON *item;
    HUFF *hp;
    int32_t datalen;
    char coinaddr[8192],scriptstr[8192];
    struct rawvin *vi;
    struct rawtx *tx;
    struct ramchain_hashptr *addrptr;
    struct rampayload *addrpayload;
    ram_addr(coinaddr,ram,txpayload->otherind);
    cJSON *json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"address",cJSON_CreateString(coinaddr));
    cJSON_AddItemToObject(json,"addr_rawind",cJSON_CreateNumber(txpayload->otherind));
    cJSON_AddItemToObject(json,"addr_txlisti",cJSON_CreateNumber(txpayload->extra));
    ram_payload_json(json,txpayload,0);
    if ( txpayload->spentB.spent != 0 )
    {
        item = cJSON_CreateObject();
        cJSON_AddItemToObject(item,"height",cJSON_CreateNumber(txpayload->spentB.blocknum));
        cJSON_AddItemToObject(item,"txind",cJSON_CreateNumber(txpayload->spentB.txind));
        cJSON_AddItemToObject(item,"vin",cJSON_CreateNumber(txpayload->spentB.v));
        if ( (hp= ram->blocks.hps[txpayload->spentB.blocknum]) != 0 && (datalen= ram_expand_bitstream(0,ram->R,ram,hp)) > 0 )
        {
            if ( (vi= ram_rawvin(ram->R,txpayload->spentB.txind,txpayload->spentB.v)) != 0 )
            {
                if ( strcmp(vi->txidstr,spent_txidstr) != 0 )
                    cJSON_AddItemToObject(item,"vin_txid_error",cJSON_CreateString(vi->txidstr));
                if ( vi->vout != spent_vout )
                    cJSON_AddItemToObject(item,"vin_error",cJSON_CreateNumber(spent_vout));
            }
            if ( (tx= ram_rawtx(ram->R,txpayload->spentB.txind)) != 0 )
                cJSON_AddItemToObject(item,"txid",cJSON_CreateString(tx->txidstr));
        }
        cJSON_AddItemToObject(json,"spent",item);
    }
    else
    {
        if ( (addrpayload= ram_getpayloadi(&addrptr,ram,'a',txpayload->otherind,txpayload->extra)) != 0 )
        {
            ram_script(scriptstr,ram,addrpayload->extra);
            cJSON_AddItemToObject(json,"script",cJSON_CreateString(scriptstr));
            cJSON_AddItemToObject(json,"script_rawind",cJSON_CreateNumber(addrpayload->extra));
        }
    }
    return(json);
}

cJSON *ram_coinaddr_json(struct ramchain_info *ram,char *coinaddr,int32_t unspentflag,int32_t truncateflag,int32_t searchperms)
{
    char permstr[MAX_JSON_FIELD];
    int64_t total = 0;
    cJSON *json = 0,*array = 0;
    int32_t i,n,numpayloads;
    struct ramchain_hashptr *addrptr;
    struct rampayload *payloads;
    json = cJSON_CreateObject();
    if ( (payloads= ram_addrpayloads(&addrptr,&numpayloads,ram,coinaddr)) != 0 && addrptr != 0 && numpayloads > 0 )
    {
        if ( truncateflag == 0 )
        {
            for (i=n=0; i<numpayloads; i++)
            {
                if ( unspentflag == 0 || payloads[i].B.spent == 0 )
                {
                    if ( payloads[i].B.spent == 0 && payloads[i].B.isinternal == 0 )
                    {
                        n++;
                        total += payloads[i].value;
                    }
                    if ( array == 0 )
                        array = cJSON_CreateArray();
                    cJSON_AddItemToArray(array,ram_addrpayload_json(ram,&payloads[i]));
                }
            }
            if ( array != 0 )
                cJSON_AddItemToObject(json,coinaddr,array);
            cJSON_AddItemToObject(json,"calc_numunspent",cJSON_CreateNumber(n));
            cJSON_AddItemToObject(json,"calc_unspent",cJSON_CreateNumber(dstr(total)));
        }
        cJSON_AddItemToObject(json,"numunspent",cJSON_CreateNumber(addrptr->numunspent));
        cJSON_AddItemToObject(json,"unspent",cJSON_CreateNumber(dstr(addrptr->unspent)));
    }
    cJSON_AddItemToObject(json,"numtx",cJSON_CreateNumber(numpayloads));
    cJSON_AddItemToObject(json,"rawind",cJSON_CreateNumber(addrptr->rawind));
    cJSON_AddItemToObject(json,"permind",cJSON_CreateNumber(addrptr->permind));
    if ( searchperms != 0 )
    {
        ram_searchpermind(permstr,ram,'a',addrptr->rawind);
        cJSON_AddItemToObject(json,"permstr",cJSON_CreateString(permstr));
    }
    cJSON_AddItemToObject(json,ram->name,cJSON_CreateString(coinaddr));
    cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(ram->srvNXTADDR));
    return(json);
}

char *ram_coinaddr_str(struct ramchain_info *ram,char *coinaddr,int32_t truncateflag,int32_t searchperms)
{
    cJSON *json;
    char retbuf[1024],*retstr;
    if ( coinaddr != 0 && coinaddr[0] != 0 && (json= ram_coinaddr_json(ram,coinaddr,1,truncateflag,searchperms)) != 0 )
    {
        retstr = cJSON_Print(json);
        free_json(json);
        return(retstr);
    }
    sprintf(retbuf,"{\"error\":\"no addr info\",\"coin\":\"%s\",\"addr\":\"%s\"}",ram->name,coinaddr);
    return(clonestr(retbuf));
}

char *ram_addr_json(struct ramchain_info *ram,uint32_t rawind,int32_t truncateflag)
{
    char hashstr[8193];
    ram_addr(hashstr,ram,rawind);
    printf("ram_addr_json(%d) -> (%s)\n",rawind,hashstr);
    return(ram_coinaddr_str(ram,hashstr,truncateflag,1));
}

char *ram_addrind_json(struct ramchain_info *ram,char *coinaddr,int32_t truncateflag)
{
    return(ram_coinaddr_str(ram,coinaddr,truncateflag,0));
}

cJSON *ram_txidstr_json(struct ramchain_info *ram,char *txidstr,int32_t truncateflag,int32_t searchperms)
{
    char permstr[MAX_JSON_FIELD];
    int64_t unspent = 0,total = 0;
    cJSON *json = 0,*array = 0;
    int32_t i,n,numpayloads;
    struct ramchain_hashptr *txptr;
    struct rampayload *txpayloads;
    json = cJSON_CreateObject();
    if ( (txpayloads= ram_txpayloads(&txptr,&numpayloads,ram,txidstr)) != 0 && txptr != 0 && numpayloads > 0 )
    {
        cJSON_AddItemToObject(json,"height",cJSON_CreateNumber(txpayloads->B.blocknum));
        cJSON_AddItemToObject(json,"txind",cJSON_CreateNumber(txpayloads->B.txind));
        cJSON_AddItemToObject(json,"numvouts",cJSON_CreateNumber(numpayloads));
        if ( truncateflag == 0 )
        {
            array = cJSON_CreateArray();
            for (i=n=0; i<numpayloads; i++)
            {
                total += txpayloads[i].value;
                if ( txpayloads[i].spentB.spent == 0 )
                    unspent += txpayloads[i].value;
                cJSON_AddItemToArray(array,ram_txpayload_json(ram,&txpayloads[i],txidstr,i));
            }
            cJSON_AddItemToObject(json,"vouts",array);
            cJSON_AddItemToObject(json,"total",cJSON_CreateNumber(dstr(total)));
            cJSON_AddItemToObject(json,"unspent",cJSON_CreateNumber(dstr(unspent)));
        }
    }
    cJSON_AddItemToObject(json,"rawind",cJSON_CreateNumber(txptr->rawind));
    cJSON_AddItemToObject(json,"permind",cJSON_CreateNumber(txptr->permind));
    cJSON_AddItemToObject(json,"txid",cJSON_CreateString(txidstr));
    if ( searchperms != 0 )
    {
        ram_searchpermind(permstr,ram,'t',txptr->rawind);
        cJSON_AddItemToObject(json,"permstr",cJSON_CreateString(permstr));
    }
    cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(ram->srvNXTADDR));
    return(json);
}

char *ram_txidstr(struct ramchain_info *ram,char *txidstr,int32_t truncateflag,int32_t searchperms)
{
    cJSON *json;
    char *retstr;
    if ( txidstr != 0 && txidstr[0] != 0 && (json= ram_txidstr_json(ram,txidstr,truncateflag,searchperms)) != 0 )
    {
        retstr = cJSON_Print(json);
        free_json(json);
        return(retstr);
    }
    return(clonestr("{\"error\":\"no txid info\"}"));
}

char *ram_txid_json(struct ramchain_info *ram,uint32_t rawind,int32_t truncateflag)
{
    char hashstr[8193];
    return(ram_txidstr(ram,ram_txid(hashstr,ram,rawind),truncateflag,1));
}

char *ram_txidind_json(struct ramchain_info *ram,char *txidstr,int32_t truncateflag)
{
    return(ram_txidstr(ram,txidstr,truncateflag,0));
}

char *ram_script_json(struct ramchain_info *ram,uint32_t rawind,int32_t truncateflag)
{
    char hashstr[8193],permstr[8193],retbuf[1024];
    ram_searchpermind(permstr,ram,'s',rawind);
    if ( ram_script(hashstr,ram,rawind) != 0 )
    {
        sprintf(retbuf,"{\"NXT\":\"%s\",\"result\":\"%u\",\"script\":\"%s\",\"rawind\":\"%u\",\"permstr\":\"%s\"}",ram->srvNXTADDR,rawind,hashstr,rawind,permstr);
        return(clonestr(retbuf));
    }
    return(clonestr("{\"error\":\"no script info\"}"));
}

char *ram_scriptind_json(struct ramchain_info *ram,char *str,int32_t truncateflag)
{
    char retbuf[1024];
    uint32_t rawind,permind;
    if ( (rawind= ram_scriptind_RO(&permind,ram,str)) != 0 )
    {
        sprintf(retbuf,"{\"NXT\":\"%s\",\"result\":\"%s\",\"rawind\":\"%u\",\"permind\":\"%u\"}",ram->srvNXTADDR,str,rawind,permind);
        return(clonestr(retbuf));
    }
    return(clonestr("{\"error\":\"no script info\"}"));
}

void ram_parse_snapshot(struct ramsnapshot *snap,cJSON *json)
{
    memset(snap,0,sizeof(*snap));
    snap->permoffset = (long)get_API_int(cJSON_GetObjectItem(json,"permoffset"),0);
    snap->addrind = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"addrind"),0);
    snap->addroffset = (long)get_API_int(cJSON_GetObjectItem(json,"addroffset"),0);
    snap->scriptind = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"scriptind"),0);
    snap->scriptoffset = (long)get_API_int(cJSON_GetObjectItem(json,"scriptoffset"),0);
    snap->txidind = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"txidind"),0);
    snap->txidoffset = (long)get_API_int(cJSON_GetObjectItem(json,"txidoffset"),0);
}

cJSON *ram_snapshot_json(struct ramsnapshot *snap)
{
    char numstr[64];
    cJSON *json = cJSON_CreateObject();
    sprintf(numstr,"%ld",snap->permoffset), cJSON_AddItemToObject(json,"permoffset",cJSON_CreateString(numstr));
    sprintf(numstr,"%u",snap->addrind), cJSON_AddItemToObject(json,"addrind",cJSON_CreateString(numstr));
    sprintf(numstr,"%ld",snap->addroffset), cJSON_AddItemToObject(json,"addroffset",cJSON_CreateString(numstr));
    sprintf(numstr,"%u",snap->scriptind), cJSON_AddItemToObject(json,"scriptind",cJSON_CreateString(numstr));
    sprintf(numstr,"%ld",snap->scriptoffset), cJSON_AddItemToObject(json,"scriptoffset",cJSON_CreateString(numstr));
    sprintf(numstr,"%u",snap->txidind), cJSON_AddItemToObject(json,"txidind",cJSON_CreateString(numstr));
    sprintf(numstr,"%ld",snap->txidoffset), cJSON_AddItemToObject(json,"txidoffset",cJSON_CreateString(numstr));
    return(json);
}

// >>>>>>>>>>>>>>  start external and API interface functions
char *ramstatus(char *origargstr,char *sender,char *previpaddr,char *coin)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    char retbuf[1024],*str;
    if ( ram == 0 || ram->MGWpingstr[0] == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    ram_setdispstr(retbuf,ram,ram->startmilli);
    str = stringifyM(retbuf);
    sprintf(retbuf,"{\"result\":\"MGWstatus\",%s\"ramchain\":%s}",ram->MGWpingstr,str);
    free(str);
    return(clonestr(retbuf));
}

int32_t ram_perm_sha256(bits256 *hashp,struct ramchain_info *ram,uint32_t blocknum,int32_t n)
{
    bits256 tmp;
    int32_t i;
    HUFF *hp,*permhp;
    memset(hashp->bytes,0,sizeof(*hashp));
    for (i=0; i<n; i++)
    {
        if ( (hp= ram->blocks.hps[blocknum+i]) == 0 )
            break;
        if ( (permhp = ram_conv_permind(ram->tmphp3,ram,hp,blocknum+i)) == 0 )
            break;
        calc_sha256cat(tmp.bytes,hashp->bytes,sizeof(*hashp),permhp->buf,hconv_bitlen(permhp->endpos)), *hashp = tmp;
    }
    return(i);
}

char *rampyramid(char *myNXTaddr,char *origargstr,char *sender,char *previpaddr,char *coin,uint32_t blocknum,char *typestr)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    char shastr[65],*hexstr,*retstr = 0;
    bits256 hash;
    HUFF *permhp,*hp,*newhp,**hpptr;
    cJSON *json;
    int32_t size,n;
    printf("rampyramid.%s (%s).%u permblocks.%d\n",coin,typestr,blocknum,ram->S.permblocks);
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    else if ( blocknum >= ram->maxblock )
        return(clonestr("{\"error\":\"blocknum too big\"}"));
    else if ( blocknum >= ram->S.permblocks )
        return(clonestr("{\"error\":\"blocknum past permblocks\"}"));
    if ( strcmp(typestr,"B64") == 0 )
    {
        n = 64;
        if ( (blocknum % n) != 0 )
            return(clonestr("{\"error\":\"B64 blocknum misaligned\"}"));
        hash = ram->snapshots[blocknum / n].hash;
    }
    else if ( strcmp(typestr,"B4096") == 0 )
    {
        n = 4096;
        if ( (blocknum % n) != 0 )
            return(clonestr("{\"error\":\"B4096 blocknum misaligned\"}"));
        //printf("rampyramid B4096 for blocknum.%d\n",blocknum);
        hash = ram->permhash4096[blocknum / n];
    }
    else
    {
        if ( (hp= ram->blocks.hps[blocknum]) == 0 )
        {
            if ( (hpptr= ram_get_hpptr(&ram->Vblocks,blocknum)) != 0 && (hp= *hpptr) != 0 )
            {
                if ( ram_expand_bitstream(0,ram->R3,ram,hp) > 0 )
                {
                    newhp = ram_makehp(ram->tmphp,'B',ram,ram->R3,blocknum);
                    if ( (hpptr= ram_get_hpptr(&ram->Bblocks,blocknum)) != 0 )
                        hp = ram->blocks.hps[blocknum] = *hpptr = newhp;
                } else hp = 0;
            }
        }
        if ( hp != 0 )
        {
            if ( (permhp= ram_conv_permind(ram->tmphp,ram,hp,blocknum)) != 0 )
            {
                size = hconv_bitlen(permhp->endpos);
                hexstr = calloc(1,size*2+1);
                init_hexbytes_noT(hexstr,permhp->buf,size);
                retstr = calloc(1,size*2+1+512);
                sprintf(retstr,"{\"NXT\":\"%s\",\"blocknum\":\"%d\",\"size\":\"%d\",\"data\":\"%s\"}",myNXTaddr,blocknum,size,hexstr);
                free(hexstr);
                return(retstr);
            } else return(clonestr("{\"error\":\"error doing ram_conv_permind\"}"));
        } else return(clonestr("{\"error\":\"no ramchain info for blocknum\"}"));
    }
    json = ram_snapshot_json(&ram->snapshots[blocknum/64]);
    init_hexbytes_noT(shastr,hash.bytes,sizeof(hash));
    cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(myNXTaddr));
    cJSON_AddItemToObject(json,"blocknum",cJSON_CreateNumber(blocknum));
    cJSON_AddItemToObject(json,"B",cJSON_CreateNumber(n));
    cJSON_AddItemToObject(json,"sha256",cJSON_CreateString(shastr));
    retstr = cJSON_Print(json);
    free_json(json);
    return(retstr);
}

void ram_setsnapshot(struct ramchain_info *ram,struct syncstate *sync,uint32_t blocknum,struct ramsnapshot *snap,uint64_t senderbits)
{
    int32_t i,num = (int32_t)(sizeof(sync->requested)/sizeof(*sync->requested));
    bits256 majority,zerokey;
    //fprintf(stderr,"ram_setsnapshot.%d\n",blocknum);
    for (i=0; i<num&&sync->requested[i]!=0; i++)
    {
        if ( senderbits == sync->requested[i] )
        {
            if ( (blocknum % 4096) != 0 )
                sync->snaps[i] = *snap;
            break;
        }
    }
    if ( sync->requested[i] == 0 )
    {
        sync->requested[i] = senderbits;
        if ( i == 0 )
        {
            if ( (blocknum % 4096) == 0 )
                ram->permhash4096[blocknum >> 12] = snap->hash;
            sync->majority = snap->hash;
        }
    }
    majority = sync->majority;
    memset(&zerokey,0,sizeof(zerokey));
    memset(&sync->minority,0,sizeof(sync->minority));
    sync->majoritybits = sync->minoritybits = 0;
    for (i=0; i<num&&sync->requested[i]!=0; i++)
    {
        //printf("check i.%d of %d: %d %d\n",i,num,sync->majoritybits,sync->minoritybits);
        if ( memcmp(sync->snaps[i].hash.bytes,majority.bytes,sizeof(majority)) == 0 )
            sync->majoritybits |= (1 << i);
        else
        {
            sync->minoritybits |= (1 << i);
            if ( memcmp(sync->snaps[i].hash.bytes,zerokey.bytes,sizeof(zerokey)) == 0 )
                sync->minority = sync->snaps[i].hash;
            else if ( memcmp(sync->snaps[i].hash.bytes,sync->minority.bytes,sizeof(sync->minority)) != 0 )
                printf("WARNING: third different hash for blocknum.%d\n",blocknum);
        }
    }
    //fprintf(stderr,"done ram_setsnapshot.%d\n",blocknum);
}

char *ramresponse(char *origargstr,char *sender,char *senderip,char *datastr)
{
    char origcmd[MAX_JSON_FIELD],coin[MAX_JSON_FIELD],permstr[MAX_JSON_FIELD],shastr[MAX_JSON_FIELD],*snapstr,*retstr = 0;
    cJSON *array,*json,*snapjson;
    uint8_t *data;
    bits256 hash;
    struct ramchain_info *ram;
    struct ramsnapshot snap;
    uint32_t blocknum,size,permind,i;
    int32_t format = 0,type = 0;
    permstr[0] = 0;
    //fprintf(stderr,"ramresponse\n");
    if ( (array= cJSON_Parse(origargstr)) != 0 )
    {
        if ( is_cJSON_Array(array) != 0 && cJSON_GetArraySize(array) == 2 )
        {
            retstr = cJSON_Print(array);
            json = cJSON_GetArrayItem(array,0);
            if ( datastr == 0 )
                datastr = cJSON_str(cJSON_GetObjectItem(json,"data"));
            copy_cJSON(origcmd,cJSON_GetObjectItem(json,"origcmd"));
            copy_cJSON(coin,cJSON_GetObjectItem(json,"coin"));
            ram = get_ramchain_info(coin);
            //printf("orig.(%s) ram.%p %s\n",origcmd,ram,coin);
            if ( strcmp(origcmd,"rampyramid") == 0 )
            {
                blocknum = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"blocknum"),0);
                if ( (format= (int32_t)get_API_int(cJSON_GetObjectItem(json,"B"),0)) == 0 )
                {
                    size = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"size"),0);
                    if ( size > 0 )
                    {
                        if ( datastr != 0 && strlen(datastr) == size*2 )
                        {
                            printf("PYRAMID.(%u %s).%d from NXT.(%s) ip.(%s)\n",blocknum,datastr,size,sender,senderip);
                            data = calloc(1,size);
                            decode_hex(data,size,datastr);
                            // update pyramid
                            free(data);
                        } else if ( datastr != 0 ) printf("strlen(%s) is %ld not %d*2\n",datastr,strlen(datastr),size);
                    }
                }
                else
                {
                    ram_parse_snapshot(&snap,json);
                    snapjson = ram_snapshot_json(&snap);
                    snapstr = cJSON_Print(snapjson), free_json(snapjson);
                    copy_cJSON(shastr,cJSON_GetObjectItem(json,"sha256"));
                    decode_hex(hash.bytes,sizeof(hash),shastr);
                    _stripwhite(snapstr,' ');
                    // update pyramid
                    if ( ram != 0 )
                    {
                        i = (blocknum >> 6);
                        ram_setsnapshot(ram,&ram->verified[i],blocknum,&snap,_calc_nxt64bits(sender));
                    }
                    printf("PYRAMID.B%d blocknum.%u [%d] sha.(%s) (%s) from NXT.(%s) ip.(%s)\n",format,blocknum,i,shastr,snapstr,sender,senderip);
                    free(snapstr);
                }
            }
            else if ( strcmp(origcmd,"ramblock") == 0 )
            {
                if ( datastr != 0 )
                    printf("PYRAMID.B%d blocknum.%u (%s).permsize %ld from NXT.(%s) ip.(%s)\n",format,blocknum,datastr,strlen(datastr)/2,sender,senderip);
                else printf("PYRAMID.B%d blocknum.%u from NXT.(%s) ip.(%s)\n",format,blocknum,sender,senderip);
            }
            else
            {
                if ( strcmp(origcmd,"addr") == 0 )
                    type = 'a';
                else if ( strcmp(origcmd,"script") == 0 )
                    type = 's';
                else if ( strcmp(origcmd,"txid") == 0 )
                    type = 't';
                if ( type != 0 )
                {
                    copy_cJSON(permstr,cJSON_GetObjectItem(json,"permstr"));
                    permind = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"rawind"),0);
                    printf("PYRAMID.%s (permind.%d %s).%c from NXT.(%s) ip.(%s)\n",origcmd,permind,permstr,type,sender,senderip);
                    // update pyramid
                }
                else if ( format == 0 )
                    printf("RAMRESPONSE unhandled: (%s) (%s) (%s) (%s) from NXT.(%s) ip.(%s)\n",coin,origcmd,permstr,origargstr,sender,senderip);
            }
        }
        free_json(array);
    }
   // fprintf(stderr,"done ramresponse\n");
    return(retstr);
}

int32_t is_remote_access(char *previpaddr);
char *ramstring(char *origargstr,char *sender,char *previpaddr,char *coin,char *typestr,uint32_t rawind)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    int32_t truncateflag = is_remote_access(previpaddr);
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    if ( strcmp(typestr,"script") == 0 )
        return(ram_script_json(ram,rawind,truncateflag));
    else if ( strcmp(typestr,"addr") == 0 )
        return(ram_addr_json(ram,rawind,truncateflag));
    else if ( strcmp(typestr,"txid") == 0 )
        return(ram_txid_json(ram,rawind,truncateflag));
    else return(clonestr("{\"error\":\"no ramstring invalid type\"}"));
}

char *ramrawind(char *origargstr,char *sender,char *previpaddr,char *coin,char *typestr,char *str)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    int32_t truncateflag = is_remote_access(previpaddr);
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    if ( strcmp(typestr,"script") == 0 )
        return(ram_scriptind_json(ram,str,truncateflag));
    else if ( strcmp(typestr,"addr") == 0 )
        return(ram_addrind_json(ram,str,truncateflag));
    else if ( strcmp(typestr,"txid") == 0 )
        return(ram_txidind_json(ram,str,truncateflag));
    else return(clonestr("{\"error\":\"no ramrawind invalid type\"}"));
}

char *ramscript(char *origargstr,char *sender,char *previpaddr,char *coin,char *txidstr,int32_t tx_vout,struct address_entry *bp)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    char retbuf[1024],scriptstr[8192];
    struct rawvout *vo;
    HUFF *hp;
    struct ramchain_hashptr *txptr,*addrptr;
    struct rampayload *txpayloads,*txpayload,*addrpayload;
    int32_t datalen,numpayloads;
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    if ( bp == 0 )
    {
        if ( (txpayloads= ram_txpayloads(&txptr,&numpayloads,ram,txidstr)) != 0 )
        {
            if ( tx_vout < txptr->numpayloads )
            {
                txpayload = &txpayloads[tx_vout];
                if ( (addrpayload= ram_getpayloadi(&addrptr,ram,'a',txpayload->otherind,txpayload->extra)) != 0 )
                {
                    ram_script(scriptstr,ram,addrpayload->extra);
                    sprintf(retbuf,"{\"result\":\"script\",\"txid\":\"%s\",\"vout\":%u,\"script\":\"%s\"}",txidstr,tx_vout,scriptstr);
                    return(clonestr(retbuf));
                } else return(clonestr("{\"error\":\"ram_getpayloadi error\"}"));
            } else return(clonestr("{\"error\":\"tx_vout error\"}"));
        }
        return(clonestr("{\"error\":\"no ram_txpayloads info\"}"));
    }
    else if ( (hp= ram->blocks.hps[bp->blocknum]) != 0 )
    {
        if ( (datalen= ram_expand_bitstream(0,ram->R,ram,hp)) > 0 )
        {
            if ( (vo= ram_rawvout(ram->R,bp->txind,bp->v)) != 0 )
            {
                sprintf(retbuf,"{\"result\":\"script\",\"txid\":\"%s\",\"vout\":%u,\"script\":\"%s\"}",txidstr,bp->v,vo->script);
                return(clonestr(retbuf));
            } else return(clonestr("{\"error\":\"ram_rawvout error\"}"));
        } else return(clonestr("{\"error\":\"ram_expand_bitstream error\"}"));
    }
    else return(clonestr("{\"error\":\"no blocks.hps[] info\"}"));
}

char *ramtxlist(char *origargstr,char *sender,char *previpaddr,char *coin,char *coinaddr,int32_t unspentflag)
{
    cJSON *json;
    char *retstr = 0;
    struct ramchain_info *ram = get_ramchain_info(coin);
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    json = ram_coinaddr_json(ram,coinaddr,unspentflag,0,0);
    if ( json != 0 )
    {
        retstr = cJSON_Print(json);
        free_json(json);
    } else retstr = clonestr("{\"error\":\"ramtxlist no data\"}");
    return(retstr);
}

char *ramrichlist(char *origargstr,char *sender,char *previpaddr,char *coin,int32_t numwhales,int32_t recalcflag)
{
    int32_t i,ind,good,bad,numunspent,numaddrs,n = 0;
    cJSON *item,*array = 0;
    char coinaddr[1024];
    struct ramchain_hashptr **addrs,*addrptr;
    char *retstr;
    double *sortbuf,startmilli = ram_millis();
    uint64_t unspent;
    struct ramchain_info *ram = get_ramchain_info(coin);
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    if ( (addrs= ram_getallstrptrs(&numaddrs,ram,'a')) != 0 && numaddrs > 0 )
    {
        sortbuf = calloc(2*numaddrs,sizeof(*sortbuf));
        for (i=good=bad=0; i<numaddrs; i++)
        {
            ram_decode_hashdata(coinaddr,'a',addrs[i]->hh.key);
            if ( recalcflag != 0 )
                unspent = ram_calc_unspent(0,&numunspent,&addrptr,ram,coinaddr,0);
            else unspent = addrs[i]->unspent, addrptr = addrs[i], numunspent = addrptr->numunspent;
            if ( unspent != 0 )
            {
                if ( addrs[i]->unspent == unspent && addrptr->numunspent == numunspent )
                    good++;
                else bad++;
                sortbuf[n << 1] = dstr(unspent);
                memcpy(&sortbuf[(n << 1) + 1],&i,sizeof(i));
                n++;
            }
        }
        if ( n > 1 )
            qsort(sortbuf,n,sizeof(double) * 2,_decreasing_double_cmp);
        if ( n > 0 )
        {
            array = cJSON_CreateArray();
            for (i=0; i<numwhales&&i<n; i++)
            {
                item = cJSON_CreateObject();
                memcpy(&ind,&sortbuf[(i << 1) + 1],sizeof(i));
                ram_decode_hashdata(coinaddr,'a',addrs[ind]->hh.key);
                cJSON_AddItemToObject(item,coinaddr,cJSON_CreateNumber(sortbuf[i<<1]));
                cJSON_AddItemToObject(item,"unspent",cJSON_CreateNumber(dstr(addrs[ind]->unspent)));
                cJSON_AddItemToArray(array,item);
            }
            item = cJSON_CreateObject();
            if ( recalcflag != 0 )
            {
                cJSON_AddItemToObject(item,"cumulative errors",cJSON_CreateNumber(bad));
                cJSON_AddItemToObject(item,"cumulative correct",cJSON_CreateNumber(good));
            }
            cJSON_AddItemToObject(item,"milliseconds",cJSON_CreateNumber(ram_millis()-startmilli));
            cJSON_AddItemToArray(array,item);
        }
        free(sortbuf);
        free(addrs);
    }
    if ( array != 0 )
    {
        retstr = cJSON_Print(array);
        free_json(array);
    } else retstr = clonestr("{\"error\":\"ramrichlist no data\"}");
    return(retstr);
}

char *rambalances(char *origargstr,char *sender,char *previpaddr,char *coin,char **coins,double *rates,char ***coinaddrs,int32_t numcoins)
{
    uint64_t total = 0;
    char *retstr = 0;
    int32_t i,j;
    cJSON *retjson,*array;
    struct ramchain_info *ram = get_ramchain_info(coin);
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    if ( numcoins > 0 && coins != 0 && coinaddrs != 0 )
    {
        retjson = cJSON_CreateObject();
        array = cJSON_CreateArray();
        if ( numcoins == 1 && strcmp(coins[0],coin) == 0 )
        {
            for (j=0; coinaddrs[0][j]!=0; j++)
                total += ram_unspent_json(&array,coin,0.,coin,coinaddrs[0][j]);
            cJSON_AddItemToObject(retjson,"total",array);
        }
        else if ( rates != 0 )
        {
            for (i=0; i<numcoins; i++)
            {
                for (j=0; coinaddrs[i][j]!=0; j++)
                    total += ram_unspent_json(&array,coin,rates[i],coins[i],coinaddrs[i][j]);
            }
            cJSON_AddItemToObject(retjson,"subtotals",array);
            cJSON_AddItemToObject(retjson,"estimated total",cJSON_CreateString(coin));
            cJSON_AddItemToObject(retjson,coin,cJSON_CreateNumber(dstr(total)));
        } else return(clonestr("{\"error\":\"rambalances: need rates for multicoin request\"}"));
        retstr = cJSON_Print(retjson);
        free_json(retjson);
    }
    return(clonestr("{\"error\":\"rambalances: numcoins zero or bad ptr\"}"));
}

char *ramblock(char *myNXTaddr,char *origargstr,char *sender,char *previpaddr,char *coin,uint32_t blocknum)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    char hexstr[8192];
    cJSON *json = 0;
    HUFF *hp,*permhp;
    int32_t datalen;
    char *retstr = 0;
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    if ( (hp= ram->blocks.hps[blocknum]) == 0 )
    {
        _get_blockinfo(ram->R,ram,blocknum);
        json = ram_rawblock_json(ram->R,0);
    }
    else
    {
        ram_expand_bitstream(&json,ram->R,ram,hp);
        permhp = ram_conv_permind(ram->tmphp,ram,hp,blocknum);
        datalen = hconv_bitlen(permhp->endpos);
        if ( json != 0 && permhp != 0 && datalen < (sizeof(hexstr)/2-1) )
        {
            init_hexbytes_noT(hexstr,permhp->buf,datalen);
            if ( is_remote_access(previpaddr) != 0 )
            {
                free_json(json);
                json = cJSON_CreateObject();
                cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(myNXTaddr));
                cJSON_AddItemToObject(json,"blocknum",cJSON_CreateNumber(blocknum));
            }
            cJSON_AddItemToObject(json,"data",cJSON_CreateString(hexstr));
        } else printf("error getting json.%p or permhp.%p allocsize.%d\n",json,permhp,permhp != 0 ? datalen : 0);
    }
    if ( json != 0 )
    {
        retstr = cJSON_Print(json);
        free_json(json);
    }
    return(retstr);
}

char *ramcompress(char *origargstr,char *sender,char *previpaddr,char *coin,char *blockhex)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    uint8_t *data;
    cJSON *json;
    int32_t datalen,complen;
    char *retstr,*hexstr;
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    datalen = (int32_t)strlen(blockhex);
    datalen >>= 1;
    data = calloc(1,datalen);
    decode_hex(data,datalen,blockhex);
    if ( (complen= ram_compress(ram->tmphp,ram,data,datalen)) > 0 )
    {
        hexstr = calloc(1,complen*2+1);
        init_hexbytes_noT(hexstr,ram->tmphp->buf,complen);
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json,"result",cJSON_CreateString(coin));
        cJSON_AddItemToObject(json,"bitstream",cJSON_CreateString(hexstr));
        cJSON_AddItemToObject(json,"datalen",cJSON_CreateNumber(datalen));
        cJSON_AddItemToObject(json,"compressed",cJSON_CreateNumber(complen));
        retstr = cJSON_Print(json);
        free_json(json);
        free(hexstr);
    } else retstr = clonestr("{\"error\":\"no block info\"}");
    free(data);
    return(retstr);
}

char *ramexpand(char *origargstr,char *sender,char *previpaddr,char *coin,char *bitstream)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    uint8_t *data;
    HUFF *hp;
    int32_t datalen,expandlen;
    char *retstr;
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    datalen = (int32_t)strlen(bitstream);
    datalen >>= 1;
    data = calloc(1,datalen);
    decode_hex(data,datalen,bitstream);
    hp = hopen(ram->name,&ram->Perm,data,datalen,0);
    if ( (expandlen= ram_expand_bitstream(0,ram->R,ram,hp)) > 0 )
    {
        free(retstr);
        retstr = ram_blockstr(ram->R2,ram,ram->R);
    } else clonestr("{\"error\":\"no ram_expand_bitstream info\"}");
    hclose(hp);
    free(data);
    return(retstr);
}

// >>>>>>>>>>>>>>  start initialization and runloops
struct mappedblocks *ram_init_blocks(int32_t noload,HUFF **copyhps,struct ramchain_info *ram,uint32_t firstblock,struct mappedblocks *blocks,struct mappedblocks *prevblocks,int32_t format,int32_t shift)
{
    void *ptr;
    int32_t numblocks,tmpsize = TMPALLOC_SPACE_INCR;
    blocks->R = (MAP_HUFF != 0) ? permalloc(ram->name,&ram->Perm,sizeof(*blocks->R),8) : calloc(1,sizeof(*blocks->R));
    blocks->R2 = (MAP_HUFF != 0) ? permalloc(ram->name,&ram->Perm,sizeof(*blocks->R2),8) : calloc(1,sizeof(*blocks->R2));
    blocks->R3 = (MAP_HUFF != 0) ? permalloc(ram->name,&ram->Perm,sizeof(*blocks->R3),8) : calloc(1,sizeof(*blocks->R3));
    blocks->ram = ram;
    blocks->prevblocks = prevblocks;
    blocks->format = format;
    ptr = (MAP_HUFF != 0) ? permalloc(ram->name,&ram->Perm,tmpsize,8) : calloc(1,tmpsize), blocks->tmphp = hopen(ram->name,&ram->Perm,ptr,tmpsize,0);
    if ( (blocks->shift = shift) != 0 )
        firstblock &= ~((1 << shift) - 1);
    blocks->firstblock = firstblock;
    numblocks = (ram->maxblock+1) - firstblock;
    printf("initblocks.%d 1st.%d num.%d n.%d\n",format,firstblock,numblocks,numblocks>>shift);
    if ( numblocks < 0 )
    {
        printf("illegal numblocks %d with firstblock.%d vs maxblock.%d\n",blocks->numblocks,firstblock,ram->maxblock);
        exit(-1);
    }
    blocks->numblocks = numblocks;
    if ( blocks->hps == 0 )
        blocks->hps = (MAP_HUFF != 0) ? permalloc(ram->name,&ram->Perm,blocks->numblocks*sizeof(*blocks->hps),8) : calloc(1,blocks->numblocks*sizeof(*blocks->hps));
    if ( format != 0 )
    {
        blocks->M = (MAP_HUFF != 0) ? permalloc(ram->name,&ram->Perm,((blocks->numblocks >> shift) + 1)*sizeof(*blocks->M),8) : calloc(1,((blocks->numblocks >> shift) + 1)*sizeof(*blocks->M));
        if ( noload == 0 )
            blocks->blocknum = ram_load_blocks(ram,blocks,firstblock,blocks->numblocks);
    }
    else
    {
        blocks->blocknum = blocks->contiguous = ram_setcontiguous(blocks);
    }
    {
        char formatstr[16];
        ram_setformatstr(formatstr,blocks->format);
        printf("%s.%s contiguous blocks.%d | numblocks.%d\n",ram->name,formatstr,blocks->contiguous,blocks->numblocks);
    }
    return(blocks);
}

uint32_t ram_update_RTblock(struct ramchain_info *ram)
{
    ram->S.RTblocknum = _get_RTheight(ram);
    if ( ram->firstblock == 0 )
        ram->firstblock = ram->S.RTblocknum;
    else if ( (ram->S.RTblocknum - ram->firstblock) >= WITHRAW_ENABLE_BLOCKS )
        ram->S.enable_withdraws = 1;
    ram->S.blocknum = ram->blocks.blocknum = (ram->S.RTblocknum - ram->min_confirms);
    if ( ram->Bblocks.blocknum >= ram->S.RTblocknum-ram->min_confirms )
        ram->S.is_realtime = 1;
    return(ram->S.RTblocknum);
}

uint32_t ram_find_firstgap(struct ramchain_info *ram,int32_t format)
{
    char fname[1024],formatstr[15];
    uint32_t blocknum;
    FILE *fp;
    ram_setformatstr(formatstr,format);
    for (blocknum=0; blocknum<ram->S.RTblocknum; blocknum++)
    {
        ram_setfname(fname,ram,blocknum,formatstr);
        if ( (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
        {
            fclose(fp);
            continue;
        }
        break;
    }
    return(blocknum);
}

void ram_syncblocks(struct ramchain_info *ram,uint32_t blocknum,int32_t numblocks,uint64_t *sources,int32_t n,int32_t addshaflag);
int32_t ram_syncblock(struct ramchain_info *ram,struct syncstate *sync,uint32_t blocknum,int32_t log2bits)
{
    int32_t numblocks,n;
    numblocks = (1 << log2bits);
    while ( (n= ram_getsources(sync->requested,ram,blocknum,numblocks)) == 0 )
    {
        fprintf(stderr,"waiting for peers for block%d.%u of %u | peers.%d\n",numblocks,blocknum,ram->S.RTblocknum,n);
        portable_sleep(3);
    }
    //for (i=0; i<n; i++)
    ///    printf("%llu ",(long long)sync->requested[i]);
    //printf("sources for %d.%d\n",blocknum,numblocks);
    ram_syncblocks(ram,blocknum,numblocks,sync->requested,n,0);
    sync->pending = n;
    sync->blocknum = blocknum;
    sync->format = numblocks;
    ram_update_RTblock(ram);
    return((ram->S.RTblocknum >> log2bits) << log2bits);
}

void ram_selfheal(struct ramchain_info *ram,uint32_t blocknum,int32_t numblocks)
{
    int32_t i;
    for (i=0; i<numblocks; i++)
        printf("magically heal block.%d\n",blocknum + i);
}

uint32_t ram_syncblock64(struct syncstate **subsyncp,struct ramchain_info *ram,struct syncstate *sync,uint32_t blocknum)
{
    uint32_t i,j,last64,done = 0;
    struct syncstate *subsync;
    last64 = (ram->S.RTblocknum >> 6) << 6;
    //fprintf(stderr,"syncblock64 from %d: last64 %d\n",blocknum,last64);
    if ( sync->substate == 0 )
        sync->substate = calloc(64,sizeof(*sync));
    for (i=0; blocknum<=last64&&i<64; blocknum+=64,i++)
    {
        subsync = &sync->substate[i];
        if ( subsync->minoritybits != 0 )
        {
            if ( subsync->substate == 0 )
                subsync->substate = calloc(64,sizeof(*subsync->substate));
            for (j=0; j<64; j++)
                ram_syncblock(ram,&sync->substate[j],blocknum+j,0);
        }
        else if ( subsync->majoritybits == 0 || bitweight(subsync->majoritybits) < 3 )
            last64 = ram_syncblock(ram,subsync,blocknum,6);
        else done++;
    }
    if ( subsyncp != 0 )
        (*subsyncp) = &sync->substate[i];
    //fprintf(stderr,"syncblock64 from %d: %d done of %d\n",blocknum,done,i);
    return(last64);
}

void ram_init_remotemode(struct ramchain_info *ram)
{
    struct syncstate *sync,*subsync,*blocksync;
    uint64_t requested[16];
    int32_t contiguous,activeblock;
    uint32_t blocknum,i,n,last64,last4096,done = 0;
    last4096 = (ram->S.RTblocknum >> 12) << 12;
    activeblock = contiguous = -1;
    while ( done < (last4096 >> 12) )
    {
        for (i=blocknum=0; blocknum<last4096; blocknum+=4096,i++)
        {
            sync = &ram->verified[i];
            if ( sync->minoritybits != 0 )
                ram_syncblock64(0,ram,sync,blocknum);
            else if ( sync->majoritybits == 0 || bitweight(sync->majoritybits) < 3 )
                ram_syncblock(ram,sync,blocknum,12);
            else done++;
        }
        printf("block.%u last4096.%d done.%d of %d\n",blocknum,last4096,done,i);
        last64 = ((ram->S.RTblocknum >> 6) << 6);
        sync = &ram->verified[i];
        ram_syncblock64(&subsync,ram,sync,blocknum);
        if ( subsync->substate == 0 )
            subsync->substate = calloc(64,sizeof(*subsync->substate));
        for (i=0; blocknum<ram->S.RTblocknum&&i<64; blocknum++,i++)
        {
            blocksync = &sync->substate[i];
            if ( blocksync->majoritybits == 0 || bitweight(blocksync->majoritybits) < 3 )
                ram_syncblock(ram,blocksync,blocknum,0);
        }
        portable_sleep(10);
    }
    for (i=0; i<4096; i++)
    {break;
        for (blocknum=i; blocknum<ram->S.RTblocknum; blocknum+=(ram->S.RTblocknum>>12))
        {
            if ( (n= ram_getsources(requested,ram,blocknum,1)) == 0 )
            {
                fprintf(stderr,"unexpected nopeers block.%u of %u | peers.%d\n",blocknum,ram->S.RTblocknum,n);
                continue;
            }
            ram_syncblocks(ram,blocknum,1,&requested[rand() % n],1,0);
        }
        msleep(10);
    }
}

void ram_regen(struct ramchain_info *ram)
{
    uint32_t blocknums[3],pass,firstblock;
    printf("REGEN\n");
    ram->mappedblocks[4] = ram_init_blocks(1,ram->blocks.hps,ram,0,&ram->blocks4096,&ram->blocks64,4096,12);
    ram->mappedblocks[3] = ram_init_blocks(1,ram->blocks.hps,ram,0,&ram->blocks64,&ram->Bblocks,64,6);
    ram->mappedblocks[2] = ram_init_blocks(1,ram->blocks.hps,ram,0,&ram->Bblocks,&ram->Vblocks,'B',0);
    ram->mappedblocks[1] = ram_init_blocks(1,ram->blocks.hps,ram,0,&ram->Vblocks,&ram->blocks,'V',0);
    ram->mappedblocks[0] = ram_init_blocks(0,ram->blocks.hps,ram,0,&ram->blocks,0,0,0);
    ram_update_RTblock(ram);
    for (pass=1; pass<=4; pass++)
    {
        printf("pass.%d\n",pass);
        if ( 1 && pass == 2 )
        {
            if ( ram->permfp != 0 )
                fclose(ram->permfp);
            ram->permfp = ram_permopen(ram->permfname,ram->name);
            ram_init_hashtable(1,&blocknums[0],ram,'a');
            ram_init_hashtable(1,&blocknums[1],ram,'s');
            ram_init_hashtable(1,&blocknums[2],ram,'t');
        }
        else if ( pass == 1 )
        {
            firstblock = ram_find_firstgap(ram,ram->mappedblocks[pass]->format);
            printf("firstblock.%d\n",firstblock);
            if ( firstblock < 10 )
                ram->mappedblocks[pass]->blocknum = 0;
            else ram->mappedblocks[pass]->blocknum = (firstblock - 1);
            printf("firstblock.%u -> %u\n",firstblock,ram->mappedblocks[pass]->blocknum);
        }
        ram_process_blocks(ram,ram->mappedblocks[pass],ram->mappedblocks[pass-1],100000000.);
    }
    printf("FINISHED REGEN\n");
    exit(1);
}
void ram_init_tmpspace(struct ramchain_info *ram,long size)
{
    ram->Tmp.ptr = (MAP_HUFF != 0) ? permalloc(ram->name,&ram->Perm,size,8) : calloc(1,size);
    // mem->ptr = malloc(size);
    ram->Tmp.size = size;
    ram_clear_alloc_space(&ram->Tmp);
}

void ram_allocs(struct ramchain_info *ram)
{
    int32_t tmpsize = TMPALLOC_SPACE_INCR;
    void *ptr;
    permalloc(ram->name,&ram->Perm,PERMALLOC_SPACE_INCR,0);
    ram->blocks.M = permalloc(ram->name,&ram->Perm,sizeof(*ram->blocks.M),8);
    ram->snapshots = permalloc(ram->name,&ram->Perm,sizeof(*ram->snapshots) * (ram->maxblock / 64),8);
    ram->permhash4096 = permalloc(ram->name,&ram->Perm,sizeof(*ram->permhash4096) * (ram->maxblock / 4096),8);
    ram->verified = permalloc(ram->name,&ram->Perm,sizeof(*ram->verified) * (ram->maxblock / 4096),8);
    ram->blocks.hps = permalloc(ram->name,&ram->Perm,ram->maxblock*sizeof(*ram->blocks.hps),8);
    ram_init_tmpspace(ram,tmpsize);
    ptr = (MAP_HUFF != 0 ) ? permalloc(ram->name,&ram->Perm,tmpsize,8) : calloc(1,tmpsize), ram->tmphp = hopen(ram->name,&ram->Perm,ptr,tmpsize,0);
    ptr = (MAP_HUFF != 0 ) ? permalloc(ram->name,&ram->Perm,tmpsize,8) : calloc(1,tmpsize), ram->tmphp2 = hopen(ram->name,&ram->Perm,ptr,tmpsize,0);
    ptr = (MAP_HUFF != 0 ) ? permalloc(ram->name,&ram->Perm,tmpsize,8) : calloc(1,tmpsize), ram->tmphp3 = hopen(ram->name,&ram->Perm,ptr,tmpsize,0);
    ram->R = (MAP_HUFF != 0) ? permalloc(ram->name,&ram->Perm,sizeof(*ram->R),8) : calloc(1,sizeof(*ram->R));
    ram->R2 = (MAP_HUFF != 0) ? permalloc(ram->name,&ram->Perm,sizeof(*ram->R2),8) : calloc(1,sizeof(*ram->R2));
    ram->R3 = (MAP_HUFF != 0) ? permalloc(ram->name,&ram->Perm,sizeof(*ram->R3),8) : calloc(1,sizeof(*ram->R3));
}

uint32_t ram_loadblocks(struct ramchain_info *ram,double startmilli)
{
    uint32_t numblocks;
    numblocks = ram_setcontiguous(&ram->blocks);
    ram->mappedblocks[4] = ram_init_blocks(0,ram->blocks.hps,ram,(numblocks>>12)<<12,&ram->blocks4096,&ram->blocks64,4096,12);
    printf("set ramchain blocknum.%s %d (1st %d num %d) vs RT.%d %.1f seconds to init_ramchain.%s B4096\n",ram->name,ram->blocks4096.blocknum,ram->blocks4096.firstblock,ram->blocks4096.numblocks,ram->blocks.blocknum,(ram_millis() - startmilli)/1000.,ram->name);
    ram->mappedblocks[3] = ram_init_blocks(0,ram->blocks.hps,ram,ram->blocks4096.contiguous,&ram->blocks64,&ram->Bblocks,64,6);
    printf("set ramchain blocknum.%s %d vs (1st %d num %d) RT.%d %.1f seconds to init_ramchain.%s B64\n",ram->name,ram->blocks64.blocknum,ram->blocks64.firstblock,ram->blocks64.numblocks,ram->blocks.blocknum,(ram_millis() - startmilli)/1000.,ram->name);
    ram->mappedblocks[2] = ram_init_blocks(0,ram->blocks.hps,ram,ram->blocks64.contiguous,&ram->Bblocks,&ram->Vblocks,'B',0);
    printf("set ramchain blocknum.%s %d vs (1st %d num %d) RT.%d %.1f seconds to init_ramchain.%s B\n",ram->name,ram->Bblocks.blocknum,ram->Bblocks.firstblock,ram->Bblocks.numblocks,ram->blocks.blocknum,(ram_millis() - startmilli)/1000.,ram->name);
    ram->mappedblocks[1] = ram_init_blocks(0,ram->blocks.hps,ram,ram->Bblocks.contiguous,&ram->Vblocks,&ram->blocks,'V',0);
    printf("set ramchain blocknum.%s %d vs (1st %d num %d) RT.%d %.1f seconds to init_ramchain.%s V\n",ram->name,ram->Vblocks.blocknum,ram->Vblocks.firstblock,ram->Vblocks.numblocks,ram->blocks.blocknum,(ram_millis() - startmilli)/1000.,ram->name);
    //ram_process_blocks(ram,ram->mappedblocks[2],ram->mappedblocks[1],1000.*3600*24);
    ram->mappedblocks[0] = ram_init_blocks(0,ram->blocks.hps,ram,0,&ram->blocks,0,0,0);
    return(numblocks);
}

void ram_Hfiles(struct ramchain_info *ram)
{
    uint32_t blocknum,iter,i;
    HUFF *hp;
    for (blocknum=0; blocknum<ram->blocks.contiguous; blocknum+=64)
    {break;
        ram->minval = ram->maxval = ram->minval2 = ram->maxval2 = ram->minval4 = ram->maxval4 = ram->minval8 = ram->maxval8 = 0;
        memset(&ram->H,0,sizeof(ram->H));
        for (iter=0; iter<2; iter++)
            for (i=0; i<64; i++)
            {
                if ( (hp= ram->blocks.hps[blocknum+i]) != 0 )
                    ram_rawblock_update(iter==0?'H':'h',ram,hp,blocknum+i);
            }
        huffpair_gencodes(ram,&ram->H,0);
        /*free(ram->H.numtx.items);
         free(ram->H.tx0.numvins.items), free(ram->H.tx0.numvouts.items), free(ram->H.tx0.txid.items);
         free(ram->H.tx1.numvins.items), free(ram->H.tx1.numvouts.items), free(ram->H.tx1.txid.items);
         free(ram->H.txi.numvins.items), free(ram->H.txi.numvouts.items), free(ram->H.txi.txid.items);
         free(ram->H.vin0.txid.items), free(ram->H.vin0.vout.items);
         free(ram->H.vin1.txid.items), free(ram->H.vin1.vout.items);
         free(ram->H.vini.txid.items), free(ram->H.vini.vout.items);
         free(ram->H.vout0.addr.items), free(ram->H.vout0.script.items);
         free(ram->H.vout1.addr.items), free(ram->H.vout1.script.items);
         free(ram->H.vouti.addr.items), free(ram->H.vout2.script.items);
         free(ram->H.vout2.addr.items), free(ram->H.vouti.script.items);
         free(ram->H.voutn.addr.items), free(ram->H.voutn.script.items);*/
    }
    //fprintf(stderr,"totalbytes.%lld %s -> %s totalbits.%lld R%.3f\n",(long long)ram->totalbits,_mbstr((double)ram->totalbytes),_mbstr2((double)ram->totalbits/8),(long long)ram->totalbytes,(double)ram->totalbytes*8/ram->totalbits);
}

void ram_convertall(struct ramchain_info *ram)
{
    char fname[1024];
    bits256 refsha,sha;
    uint32_t blocknum,checkblocknum;
    HUFF *hp,*permhp;
    void *buf;
    for (blocknum=0; blocknum<ram->blocks.contiguous; blocknum++)
    {
        if ( (hp= ram->blocks.hps[blocknum]) != 0 )
        {
            buf = (MAP_HUFF != 0) ? permalloc(ram->name,&ram->Perm,hp->allocsize*2,9) : calloc(1,hp->allocsize*2);
            permhp = hopen(ram->name,&ram->Perm,buf,hp->allocsize*2,0);
            ram->blocks.hps[blocknum] = ram_conv_permind(permhp,ram,hp,blocknum);
            permhp->allocsize = hconv_bitlen(permhp->endpos);
            if ( 0 && (checkblocknum= ram_verify(ram,permhp,'B')) != checkblocknum )
                printf("ram_verify(%d) -> %d?\n",checkblocknum,checkblocknum);
        }
        else { printf("unexpected gap at %d\n",blocknum); exit(-1); }
    }
    sprintf(fname,"%s/ramchains/%s.perm",MGWROOT,ram->name);
    ram_save_bitstreams(&refsha,fname,ram->blocks.hps,ram->blocks.contiguous);
    ram_map_bitstreams(1,ram,0,ram->blocks.M,&sha,ram->blocks.hps,ram->blocks.contiguous,fname,&refsha);
    printf("converted to permind, please copy over files with .perm files and restart\n");
    exit(1);
}

int32_t ram_scanblocks(struct ramchain_info *ram)
{
    uint32_t blocknum,errs=0,good=0,iter;
    HUFF *hp;
    for (iter=0; iter<2; iter++)
    {
        for (errs=good=blocknum=0; blocknum<ram->blocks.contiguous; blocknum++)
        {
            if ( (blocknum % 1000) == 0 )
                fprintf(stderr,".");
            if ( (hp= ram->blocks.hps[blocknum]) != 0 )
            {
                if ( ram_rawblock_update(iter,ram,hp,blocknum) > 0 )
                {
                    if ( iter == 0 && ram->permfp != 0 )
                        ram_conv_permind(ram->tmphp2,ram,hp,blocknum);
                    good++;
                }
                else
                {
                    printf("iter.%d error on block.%d purge it\n",iter,blocknum);
                    while ( 1 ) portable_sleep(1);
                    //ram_purge_badblock(ram,blocknum);
                    errs++;
                    exit(-1);
                }
            } else errs++;
        }
        printf(">>>>>>>>>>>>> permind_changes.%d <<<<<<<<<<<<\n",ram->permind_changes);
        if ( 0 && ram->addrhash.permfp != 0 && ram->txidhash.permfp != 0 && ram->scripthash.permfp != 0 && iter == 0 && ram->permind_changes != 0 )
            ram_convertall(ram);
    }
    if ( ram->permfp != 0 )
        fflush(ram->permfp);
    fprintf(stderr,"contiguous.%d good.%d errs.%d\n",ram->blocks.contiguous,good,errs);
    ram_Hfiles(ram);
    return(errs);
}

void ram_init_ramchain(struct ramchain_info *ram)
{
    int32_t datalen,nofile,numblocks,errs;
    uint32_t blocknums[3],permind;
    bits256 refsha,sha;
    double startmilli;
    char fname[1024];
    startmilli = ram_millis();
    strcpy(ram->dirpath,MGWROOT);
    ram->S.RTblocknum = _get_RTheight(ram);
    ram->blocks.blocknum = (ram->S.RTblocknum - ram->min_confirms);
    ram->blocks.numblocks = ram->maxblock = (ram->S.RTblocknum + 10000);
    ram_allocs(ram);
    printf("[%s] ramchain.%s RT.%d %.1f seconds to init_ramchain_directories: next.(%d %d %d %d)\n",ram->dirpath,ram->name,ram->S.RTblocknum,(ram_millis() - startmilli)/1000.,ram->S.permblocks,ram->next_txid_permind,ram->next_script_permind,ram->next_addr_permind);
    memset(blocknums,0,sizeof(blocknums));
    sprintf(ram->permfname,"%s/ramchains/%s.perm",MGWROOT,ram->name);
    nofile = (ram->permfp = ram_permopen(ram->permfname,ram->name)) == 0;
    nofile += ram_init_hashtable(0,&blocknums[0],ram,'a');
    nofile += ram_init_hashtable(0,&blocknums[1],ram,'s');
    nofile += ram_init_hashtable(0,&blocknums[2],ram,'t');
    ram_update_RTblock(ram);
    if ( ram->marker != 0 && ram->marker[0] != 0 && (ram->marker_rawind= ram_addrind_RO(&permind,ram,ram->marker)) == 0 )
        printf("WARNING: MARKER.(%s) set but no rawind. need to have it appear in blockchain first\n",ram->marker);
    if ( ram->marker2 != 0 && ram->marker2[0] != 0 && (ram->marker2_rawind= ram_addrind_RO(&permind,ram,ram->marker2)) == 0 )
        printf("WARNING: MARKER2.(%s) set but no rawind. need to have it appear in blockchain first\n",ram->marker2);
    printf("%.1f seconds to init_ramchain.%s hashtables marker.(%s || %s) %u %u\n",(ram_millis() - startmilli)/1000.,ram->name,ram->marker,ram->marker2,ram->marker_rawind,ram->marker2_rawind);
    if ( ram->remotemode != 0 )
        ram_init_remotemode(ram);
    else
    {
        ram_init_directories(ram);
        if ( nofile >= 3 )
            ram_regen(ram);
    }
    sprintf(fname,"%s/ramchains/%s.blocks",MGWROOT,ram->name);
    ram_map_bitstreams(0,ram,0,ram->blocks.M,&sha,ram->blocks.hps,0,fname,0);
    numblocks = ram_loadblocks(ram,startmilli);
    errs = ram_scanblocks(ram);
    if ( numblocks == 0 && errs == 0 && ram->blocks.contiguous > 4096 )
    {
        printf("saving new %s.blocks\n",ram->name);
        datalen = -1;
        if ( ram_save_bitstreams(&refsha,fname,ram->blocks.hps,ram->blocks.contiguous) > 0 )
            datalen = ram_map_bitstreams(1,ram,0,ram->blocks.M,&sha,ram->blocks.hps,ram->blocks.contiguous,fname,&refsha);
        printf("Created.(%s) datalen.%d | please restart\n",fname,datalen);
        exit(1);
    } else printf("no need to save numblocks.%d errs.%d contiguous.%d\n",numblocks,errs,ram->blocks.contiguous);
    ram_disp_status(ram);
}

uint32_t ram_update_disp(struct ramchain_info *ram)
{
    int32_t pingall(char *coinstr,char *srvNXTACCTSECRET);
    if ( ram_update_RTblock(ram) > ram->lastdisp )
    {
        ram->blocks.blocknum = ram->blocks.contiguous = ram_setcontiguous(&ram->blocks);
        ram_disp_status(ram);
        ram->lastdisp = ram_update_RTblock(ram);
        pingall(ram->name,ram->srvNXTACCTSECRET);
        return(ram->lastdisp);
    }
    return(0);
}

void *ram_process_blocks_loop(void *_blocks)
{
    struct mappedblocks *blocks = _blocks;
    printf("start _process_mappedblocks.%s format.%d\n",blocks->ram->name,blocks->format);
    while ( 1 )
    {
        ram_update_RTblock(blocks->ram);
        if ( ram_process_blocks(blocks->ram,blocks,blocks->prevblocks,1000.) == 0 )
            portable_sleep(sqrt(1 << blocks->shift));
    }
}

void *ram_process_ramchain(void *_ram)
{
    struct ramchain_info *ram = _ram;
    int32_t pass;
    ram->startmilli = ram_millis();
    for (pass=1; pass<=4; pass++)
    {
        if ( portable_thread_create((void *)ram_process_blocks_loop,ram->mappedblocks[pass]) == 0 )
            printf("ERROR _process_ramchain.%s\n",ram->name);
    }
    while ( 1 )
    {
        ram_update_disp(ram);
        portable_sleep(1);
    }
    return(0);
}

void *portable_thread_create(void *funcp,void *argp);
void activate_ramchain(struct ramchain_info *ram,char *name)
{
    Ramchains[Numramchains++] = ram;
    if ( Debuglevel > 0 )
        printf("ram.%p Add ramchain.(%s) (%s) Num.%d\n",ram,ram->name,name,Numramchains);
}

void *process_ramchains(void *_argcoinstr)
{
    extern int32_t MULTITHREADS;
    void ensure_SuperNET_dirs(char *backupdir);
    char *argcoinstr = (_argcoinstr != 0) ? ((char **)_argcoinstr)[0] : 0;
    int32_t iter,gatewayid,modval,numinterleaves;
    double startmilli;
    struct ramchain_info *ram;
    int32_t i,pass,processed = 0;
    while ( IS_LIBTEST != 7 && Finished_init == 0 )
        portable_sleep(1);
    ensure_SuperNET_dirs("ramchains");

    startmilli = ram_millis();
    if ( _argcoinstr != 0 && ((long *)_argcoinstr)[1] != 0 && ((long *)_argcoinstr)[2] != 0 )
    {
        modval = (int32_t)((long *)_argcoinstr)[1];
        numinterleaves = (int32_t)((long *)_argcoinstr)[2];
        printf("modval.%d numinterleaves.%d\n",modval,numinterleaves);
    } else modval = 0, numinterleaves = 1;
    for (iter=0; iter<3; iter++)
    {
        for (i=0; i<Numramchains; i++)
        {
            if ( argcoinstr == 0 || strcmp(argcoinstr,Ramchains[i]->name) == 0 )
            {
                if ( iter > 1 )
                {
                    Ramchains[i]->S.NXTblocknum = _update_ramMGW(0,Ramchains[i],0);
                    if ( Ramchains[i]->S.NXTblocknum > 1000 )
                        Ramchains[i]->S.NXTblocknum -= 1000;
                    else Ramchains[i]->S.NXTblocknum = 0;
                    printf("i.%d of %d: NXTblock.%d (%s) 1sttime %d\n",i,Numramchains,Ramchains[i]->S.NXTblocknum,Ramchains[i]->name,Ramchains[i]->firsttime);
                }
                else if ( iter == 1 )
                {
                    ram_init_ramchain(Ramchains[i]);
                    Ramchains[i]->startmilli = ram_millis();
                }
                else if ( i != 0 )
                    Ramchains[i]->firsttime = Ramchains[0]->firsttime;
                else _get_NXTheight(&Ramchains[i]->firsttime);
            }
            printf("took %.1f seconds to init %s for %d coins\n",(ram_millis() - startmilli)/1000.,iter==0?"NXTheight":(iter==1)?"ramchains":"MGW",Numramchains);
        }
    }
    MGW_initdone = 1;
    while ( processed >= 0 )
    {
        processed = 0;
        for (i=0; i<Numramchains; i++)
        {
            ram = Ramchains[i];
            if ( argcoinstr == 0 || strcmp(argcoinstr,ram->name) == 0 )
            {
                if ( MULTITHREADS != 0 )
                {
                    printf("%d of %d: (%s) argcoinstr.%s\n",i,Numramchains,ram->name,argcoinstr!=0?argcoinstr:"ALL");
                    printf("call process_ramchain.(%s)\n",ram->name);
                    if ( portable_thread_create((void *)ram_process_ramchain,ram) == 0 )
                        printf("ERROR _process_ramchain.%s\n",ram->name);
                    processed--;
                }
                else //if ( (ram->S.NXTblocknum+ram->min_NXTconfirms) < _get_NXTheight() || (ram->mappedblocks[1]->blocknum+ram->min_confirms) < _get_RTheight(ram) )
                {
                    //if ( strcmp(ram->name,"BTC") != 0 )//ram->S.is_realtime != 0 )
                    {
                        ram->S.NXTblocknum = _update_ramMGW(0,ram,ram->S.NXTblocknum);
                        if ( (ram->S.MGWpendingredeems + ram->S.MGWpendingdeposits) != 0 )
                            printf("\n");
                        ram->S.NXT_is_realtime = (ram->S.NXTblocknum >= (ram->S.NXT_RTblocknum - ram->min_NXTconfirms));
                    } //else ram->S.NXT_is_realtime = 0;
                    ram_update_RTblock(ram);
                    for (pass=1; pass<=4; pass++)
                    {
                        processed += ram_process_blocks(ram,ram->mappedblocks[pass],ram->mappedblocks[pass-1],60000.*pass*pass);
                        //if ( (ram->mappedblocks[pass]->blocknum >> ram->mappedblocks[pass]->shift) < (ram->mappedblocks[pass-1]->blocknum >> ram->mappedblocks[pass]->shift) )
                        //    break;
                    }
                    if ( ram_update_disp(ram) != 0 || 1 )
                    {
                        static char dispbuf[1000],lastdisp[1000];
                        ram->S.MGWunspent = ram_calc_MGWunspent(&ram->S.MGWpendingdeposits,ram);
                        ram->S.MGWbalance = ram->S.MGWunspent - ram->S.circulation - ram->S.MGWpendingredeems - ram->S.MGWpendingdeposits;
                        ram_set_MGWdispbuf(dispbuf,ram,-1);
                        if ( strcmp(dispbuf,lastdisp) != 0 )
                        {
                            strcpy(lastdisp,dispbuf);
                            ram_set_MGWpingstr(ram->MGWpingstr,ram,-1);
                            for (gatewayid=0; gatewayid<ram->numgateways; gatewayid++)
                            {
                                ram_set_MGWdispbuf(dispbuf,ram,gatewayid);
                                printf("G%d:%s",gatewayid,dispbuf);
                            }
                            if ( ram->pendingticks != 0 )
                            {
                                /*int32_t j;
                                struct NXT_assettxid *tp;
                                for (j=0; j<ram->numpendingsends; j++)
                                {
                                    if ( (tp= ram->pendingsends[j]) != 0 )
                                        printf("(%llu %x %x %x) ",(long long)tp->redeemtxid,_extract_batchcrc(tp,0),_extract_batchcrc(tp,1),_extract_batchcrc(tp,2));
                                }*/
                                printf("pendingticks.%d",ram->pendingticks);
                            }
                            putchar('\n');
                        }
                    }
                }
            }
        }
        for (i=0; i<Numramchains; i++)
        {
            ram = Ramchains[i];
            if ( argcoinstr == 0 || strcmp(argcoinstr,ram->name) == 0 )
                ram_update_disp(ram);
        }
        if ( processed == 0 )
        {
            void poll_nanomsg();
            poll_nanomsg();
            portable_sleep(1);
        }
        MGW_initdone++;
    }
    printf("process_ramchains: finished launching\n");
    while ( 1 )
        portable_sleep(60);
}


int32_t set_bridge_dispbuf(char *dispbuf,char *coinstr)
{
    int32_t gatewayid;
    struct ramchain_info *ram;
    dispbuf[0] = 0;
    if ( (ram= get_ramchain_info(coinstr)) != 0 )
    {
        for (gatewayid=0; gatewayid<NUM_GATEWAYS; gatewayid++)
        {
            ram_set_MGWdispbuf(dispbuf,ram,gatewayid);
            sprintf(dispbuf+strlen(dispbuf),"G%d:%s",gatewayid,dispbuf);
        }
        printf("set_bridge_dispbuf.(%s)\n",dispbuf);
        return((int32_t)strlen(dispbuf));
    }
    return(0);
}

#endif
#endif
