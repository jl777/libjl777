//
//  ramchain
//  SuperNET
//
//  by jl777 on 12/29/14.
//  MIT license


// to port ramchains, the following is needed
// void init_ramchain_info(struct ramchain_info *ram,struct coin_info *cp) or the equivalent needs to create the required ramchain_info
// structures and allow for get_ramchain_info() to be implemented.
//
// need to provide external functions for:
// struct ramchain_info *get_ramchain_info(char *coinstr);
// also bitcoind_RPC.c, cJSON.h and cJSON.c are needed
//
// malloc, calloc, realloc, free, gettimeofday, strcpy, strncmp, strcmp, memcpy, mmap, munmap, msync, truncate;

//#define RAM_GENMODE

#ifdef INCLUDE_DEFINES
#ifndef ramchain_h
#define ramchain_h

#define TMPALLOC_SPACE_INCR 3000000
#define PERMALLOC_SPACE_INCR (1024 * 1024 * 128)

extern struct ramchain_info *get_ramchain_info(char *coinstr);
extern void calc_sha256cat(unsigned char hash[256 >> 3],unsigned char *src,int32_t len,unsigned char *src2,int32_t len2);

// ramchain functions for external access
void *process_ramchains(void *argcoinstr);

char *ramstatus(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin);
char *ramstring(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,char *typestr,uint32_t rawind);
char *ramrawind(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,char *typestr,char *str);
char *ramblock(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,uint32_t blocknum);
char *ramcompress(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,char *ramhex);
char *ramexpand(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,char *bitstream);
char *ramscript(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,char *txidstr,int32_t tx_vout,struct address_entry *bp);
char *ramtxlist(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,char *coinaddr,int32_t unspentflag);
char *ramrichlist(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,int32_t numwhales,int32_t recalcflag);
char *rambalances(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,char **coins,double *rates,char ***coinaddrs,int32_t numcoins);


#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <ctype.h>
#include <fcntl.h>

#include "uthash.h"
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


#ifdef RAM_GENMODE
#define HUFF_NUMFREQS 1
#else
#define HUFF_NUMFREQS 1
#endif

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

struct rampayload { struct address_entry B,spentB; uint64_t value; uint32_t otherind,extra; };
struct ramchain_hashptr { int64_t unspent; UT_hash_handle hh; struct rampayload *payloads; uint32_t rawind,numpayloads,maxpayloads; int32_t numunspent; };
struct ramchain_hashtable { char coinstr[16]; struct ramchain_hashptr *table; struct mappedptr M; FILE *newfp; struct ramchain_hashptr **ptrs; uint32_t ind,numalloc; uint8_t type; };


#define MAX_BLOCKTX 0xffff
struct rawvin { char txidstr[128]; uint16_t vout; };
struct rawvout { char coinaddr[64],script[128]; uint64_t value; };
struct rawtx { uint16_t firstvin,numvins,firstvout,numvouts; char txidstr[128]; };

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
    //uint64_t *flags;
    struct mappedptr *M;
    double sum;
    uint32_t blocknum,count,firstblock,numblocks,processed,format,shift,contiguous;
};

struct alloc_space { void *ptr; long used,size; };
struct ramchain_info
{
    struct mappedblocks blocks,Vblocks,Bblocks,blocks64,blocks4096,*mappedblocks[8];
    struct ramchain_hashtable addrhash,txidhash,scripthash;
    double startmilli;
    HUFF *tmphp,*tmphp2;
    char name[64],dirpath[512],myipaddr[64],srvNXTACCTSECRET[2048],srvNXTADDR[64],*userpass,*serverport,*marker;
    uint32_t lastheighttime,RTblocknum,min_confirms,estblocktime,firstiter,maxblock,nonzblocks,marker_rawind,lastdisp,maxind;
    uint64_t totalspends,numspends,totaloutputs,numoutputs,totalbits,totalbytes;
    struct rawblock *R,*R2,*R3;
    struct rawblock_huffs H;
    struct alloc_space Tmp,Perm;
    uint64_t minval,maxval,minval2,maxval2,minval4,maxval4,minval8,maxval8;
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

#endif
#endif


#ifdef INCLUDE_CODE
#ifndef ramchain_code_h
#define ramchain_code_h

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

uint64_t _align16(uint64_t ptrval) { if ( (ptrval & 15) != 0 ) ptrval += 16 - (ptrval & 15); return(ptrval); }

void *alloc_aligned_buffer(uint64_t allocsize)
{
	extern int posix_memalign (void **__memptr, size_t __alignment, size_t __size);
	if ( allocsize > ((uint64_t)192L)*1024*1024*1024 )
        { printf("%llu negative allocsize\n",(long long)allocsize); while ( 1 ) sleep(666); }
	void *ptr;
	allocsize = _align16(allocsize);
	if ( posix_memalign(&ptr,16,allocsize) != 0 )
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

void *map_file(char *fname,uint64_t *filesizep,int32_t enablewrite)
{
	void *mmap64(void *addr,size_t len,int32_t prot,int32_t flags,int32_t fildes,off_t off);
	int32_t fd,rwflags,flags = MAP_FILE|MAP_SHARED;
	uint64_t filesize;
    void *ptr = 0;
	*filesizep = 0;
	if ( enablewrite != 0 )
		fd = open(fname,O_RDWR);
	else fd = open(fname,O_RDONLY);
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
#ifdef __APPLE__
	ptr = mmap(0,filesize,rwflags,flags,fd,0);
#else
	ptr = mmap64(0,filesize,rwflags,flags,fd,0);
#endif
	close(fd);
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
}

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
		free(mp->fileptr);
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
    if ( (fp= fopen(fname,"rb")) != 0 )
    {
        fseek(fp,0,SEEK_END);
        allocsize = ftell(fp);
        fclose(fp);
        //printf("(%s) exists size.%ld\n",fname,allocsize);
    }
    else
    {
        //printf("try to create.(%s)\n",fname);
        if ( (fp= fopen(fname,"wb")) != 0 )
            fclose(fp);
    }
    if ( allocsize < filesize )
    {
        //printf("filesize.%ld is less than %ld\n",filesize,allocsize);
        if ( (fp=fopen(fname,"ab")) != 0 )
        {
            zeroes = malloc(16*1024*1024);
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
            free(zeroes);
        }
    }
    else if ( allocsize > filesize )
        truncate(fname,filesize);
}

void *init_mappedptr(void **ptrp,struct mappedptr *mp,uint64_t allocsize,int32_t rwflag,char *fname)
{
	uint64_t filesize;
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
    if ( rwflag != 0 )
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
    if ( (fp= fopen(fname,"rb")) == 0 )
    {
        sprintf(cmd,"mkdir %s",dirname);
        fix_windows_insanity(cmd);
        if ( system(cmd) != 0 )
            printf("error making subdirectory (%s) %s (%s)\n",cmd,dirname,fname);
        fp = fopen(fname,"wb");
        if ( fp != 0 )
            fclose(fp);
        if ( (fp= fopen(fname,"rb")) == 0 )
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
    if ( (fp= fopen(fname,"rb")) != 0 )
    {
        if ( (fp2= fopen(fname2,"rb")) != 0 )
        {
            while ( (len= fread(buf,1,sizeof(buf),fp)) > 0 && (len2= fread(buf2,1,sizeof(buf2),fp2)) == len )
                if ( (offset= memcmp(buf,buf2,len)) != 0 )
                    printf("compare error at offset.%d: (%s) src.%ld vs. (%s) dest.%ld\n",offset,fname,ftell(fp),fname2,ftell(fp2)), errs++;
            //while ( (c= fgetc(srcfp)) != EOF )
            //   fputc(c,destfp);
            fclose(fp);
        }
        fclose(fp2);
    }
    return(errs);
}

long copy_file(char *src,char *dest) // OS portable
{
    long len = -1;
    char buf[8192];
    FILE *srcfp,*destfp;
    if ( (srcfp= fopen(src,"rb")) != 0 )
    {
        if ( (destfp= fopen(dest,"wb")) != 0 )
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

void delete_file(char *fname,int32_t scrubflag)
{
    FILE *fp;
    char cmdstr[1024],*OS_rmstr;
    long i,fpos;
#ifdef WIN32
    char _fname[1024];
    OS_rmstr = "del";
    strcpy(_fname,fname);
    fix_windows_insanity(_fname);
    fname = _fname;
#else
    OS_rmstr = "rm";
#endif
    if ( (fp= fopen(fname,"rb+")) != 0 )
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
        sprintf(cmdstr,"%s %s",OS_rmstr,fname);
        fix_windows_insanity(cmdstr);
        system(cmdstr);
    }
}

#define ram_millis milliseconds
/*double ram_millis(void)
{
    static struct timeval timeval,first_timeval;
    gettimeofday(&timeval,0);
    if ( first_timeval.tv_sec == 0 )
    {
        first_timeval = timeval;
        return(0);
    }
    return((timeval.tv_sec - first_timeval.tv_sec) * 1000. + (timeval.tv_usec - first_timeval.tv_usec)/1000.);
}*/

double estimate_completion(char *coinstr,double startmilli,int32_t processed,int32_t numleft)
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

void ram_clear_alloc_space(struct alloc_space *mem)
{
    memset(mem->ptr,0,mem->size);
    mem->used = 0;
}

void ram_init_alloc_space(struct alloc_space *mem,long size)
{
    mem->ptr = malloc(size);
    mem->size = size;
    ram_clear_alloc_space(mem);
}

void *memalloc(struct alloc_space *mem,long size)
{
    void *ptr = 0;
    if ( (mem->used + size) > mem->size )
    {
        printf("alloc: (mem->used %ld + %ld size) %ld > %ld mem->size\n",mem->used,size,(mem->used + size),mem->size);
        while ( 1 )
            sleep(1);
    }
    ptr = (void *)((long)mem->ptr + mem->used);
    mem->used += size;
    memset(ptr,0,size);
    if ( (mem->used & 0xf) != 0 )
        mem->used += 0x10 - (mem->used & 0xf);
    return(ptr);
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
    if ( (mem->used + size) > mem->size )
    {
        for (i=0; i<(int)(sizeof(counts)/sizeof(*counts)); i++)
            if ( counts[i] != 0 )
                printf("(%s %.1f).%d ",_mbstr(totals[i]),(double)totals[i]/counts[i],i);
        printf(" | ");
        printf("permalloc new space.%ld %s | selector.%d itemsize.%ld total.%ld n.%ld ave %.1f | total %s n.%ld ave %.1f\n",mem->size,_mbstr(mem->size),selector,size,totals[selector],counts[selector],(double)totals[selector]/counts[selector],_mbstr2(totals[0]),counts[0],(double)totals[0]/counts[0]);
        memset(&M,0,sizeof(M));
        sprintf(fname,"ramchains/%s/bitstream/space.%ld",coinstr,n);
        fix_windows_insanity(fname);
       // delete_file(fname,0);
        if ( init_mappedptr(0,&M,mem->size != 0 ? mem->size : size,1,fname) == 0 )
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
        //while ( 1 ) sleep(1);
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

// >>>>>>>>>>>>>>  start bitcoind_RPC interface functions
#define OP_HASH160_OPCODE 0xa9
#define OP_EQUAL_OPCODE 0x87
#define OP_DUP_OPCODE 0x76
#define OP_EQUALVERIFY_OPCODE 0x88
#define OP_CHECKSIG_OPCODE 0xac
#define OP_RETURN_OPCODE 0x6a

int32_t _add_opcode(char *hex,int32_t offset,int32_t opcode)
{
    hex[offset + 0] = hexbyte((opcode >> 4) & 0xf);
    hex[offset + 1] = hexbyte(opcode & 0xf);
    return(offset+2);
}

void _calc_script(char *script,char *pubkey,int msigmode)
{
    int32_t offset,len;
    offset = 0;
    len = (int32_t)strlen(pubkey);
    offset = _add_opcode(script,offset,OP_DUP_OPCODE);
    offset = _add_opcode(script,offset,OP_HASH160_OPCODE);
    offset = _add_opcode(script,offset,len/2);
    memcpy(script+offset,pubkey,len), offset += len;
    offset = _add_opcode(script,offset,OP_EQUALVERIFY_OPCODE);
    offset = _add_opcode(script,offset,OP_CHECKSIG_OPCODE);
    script[offset] = 0;
}

int32_t _origconvert_to_bitcoinhex(char *scriptasm)
{
    //"asm" : "OP_HASH160 db7f9942da71fd7a28f4a4b2e8c51347240b9e2d OP_EQUAL",
    char *hex,strbuf[8192];
    int32_t middlelen,offset,len,OP_HASH160_len,OP_EQUAL_len;
    len = (int32_t)strlen(scriptasm);
    OP_HASH160_len = (int32_t)strlen("OP_HASH160");
    OP_EQUAL_len = (int32_t)strlen("OP_EQUAL");
    if ( strncmp(scriptasm,"OP_HASH160",OP_HASH160_len) == 0 && strncmp(scriptasm+len-OP_EQUAL_len,"OP_EQUAL",OP_EQUAL_len) == 0 )
    {
        hex = (len > sizeof(strbuf)-2) ? calloc(1,len+1) : strbuf;
        offset = 0;
        offset = _add_opcode(hex,offset,OP_HASH160_OPCODE);
        middlelen = len - OP_HASH160_len - OP_EQUAL_len - 2;
        offset = _add_opcode(hex,offset,middlelen/2);
        memcpy(hex+offset,scriptasm+OP_HASH160_len+1,middlelen);
        hex[offset+middlelen] = hexbyte((OP_EQUAL_OPCODE >> 4) & 0xf);
        hex[offset+middlelen+1] = hexbyte(OP_EQUAL_OPCODE & 0xf);
        hex[offset+middlelen+2] = 0;
        if ( Debuglevel > 2 )
            printf("(%s) -> (%s)\n",scriptasm,hex);
        strcpy(scriptasm,hex);
        if ( hex != strbuf )
            free(hex);
        if ( is_hexstr(scriptasm) != 0 )
            return((int32_t)(2+middlelen+2));
    }
    if ( scriptasm[0] != 0 )
        printf("cant assembly anything but OP_HASH160 + <key> + OP_EQUAL (%s)\n",scriptasm);
    strcpy(scriptasm,"nonstandard");
    return(-1);
}

int32_t _convert_to_bitcoinhex(char *scriptasm)
{
    char *hex,pubkey[512],*endstr,strbuf[8192];
    int32_t i,j,middlelen,len,OP_HASH160_len,OP_EQUAL_len;
    len = (int32_t)strlen(scriptasm);
    OP_HASH160_len = (int32_t)strlen("OP_DUP OP_HASH160");
    OP_EQUAL_len = (int32_t)strlen("OP_EQUALVERIFY OP_CHECKSIG");
    if ( strncmp(scriptasm,"OP_DUP OP_HASH160",OP_HASH160_len) == 0 && strncmp(scriptasm+len-OP_EQUAL_len,"OP_EQUALVERIFY OP_CHECKSIG",OP_EQUAL_len) == 0 )
    {
        middlelen = len - OP_HASH160_len - OP_EQUAL_len - 2;
        memcpy(pubkey,scriptasm+OP_HASH160_len+1,middlelen);
        pubkey[middlelen] = 0;
        
        hex = (len > sizeof(strbuf)-2) ? calloc(1,len+1) : strbuf;
        _calc_script(hex,pubkey,0);
        strcpy(scriptasm,hex);
        if ( hex != strbuf )
            free(hex);
        if ( is_hexstr(scriptasm) != 0 )
            return((int32_t)(2+middlelen+2));
    }
    else
    {
        endstr = scriptasm + strlen(scriptasm) - strlen(" OP_CHECKSIG");
        if ( strcmp(endstr," OP_CHECKSIG") == 0 )
        {
            strcpy(endstr,"ac");
            //printf("NEWSCRIPT.[%s]\n",scriptasm);
            if ( is_hexstr(scriptasm) != 0 )
                return((int32_t)strlen(scriptasm));
        }
        else if ( strncmp(scriptasm,"OP_RETURN ",strlen("OP_RETURN ")) == 0 )
        {
            //printf("OP_RETURN.(%s) -> ",scriptasm);
            _add_opcode(scriptasm,0,OP_RETURN_OPCODE);
            for (i=2,j=strlen("OP_RETURN "); j<=len; j++,i++)
                scriptasm[i] = scriptasm[j];
            //printf("(%s)\n",scriptasm);
            if ( is_hexstr(scriptasm) != 0 )
                return((int32_t)strlen(scriptasm));
        }
    }
    return(_origconvert_to_bitcoinhex(scriptasm));
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
                _convert_to_bitcoinhex(script);
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

uint32_t _get_RTheight(struct ramchain_info *ram)
{
    char *retstr;
    cJSON *json;
    uint32_t height = 0;
    // printf("RTheight.(%s) (%s) (%s)\n",ram->name,ram->serverport,ram->userpass);
    retstr = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"getinfo","");
    if ( retstr != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            height = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"blocks"),0);
            free_json(json);
        }
        free(retstr);
    }
    return(height);
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
    memset(hp->buf,0,hp->allocsize);
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
    printf("hgetbit past EOF: %d >= %d\n",hp->bitoffset,hp->endpos); while ( 1 ) sleep(1);
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
    if ( (count % 100000) == 0 )
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
    if ( (count % 100000) == 0 )
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
    if ( (count % 100000) == 0 )
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
    HUFF *hp = permalloc(coinstr,mem,sizeof(*hp),1);
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
        while ( 1 ) sleep(1);
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
        fp = fopen(fname,"rb"), flag = 1;
    if ( hload_varint(&endbitpos,fp) > 0 )
    {
        len = hconv_bitlen(endbitpos);
        buf = permalloc(ram->name,&ram->Perm,len,2);
        if ( offsetp != 0 )
            *offsetp = ftell(fp);
        if ( fread(buf,1,len,fp) != len )
            free(buf);
        else hp = hopen(ram->name,&ram->Perm,buf,len,0), hp->endpos = (int32_t)endbitpos;
        //fseek(fp,0,SEEK_END);
        //printf("HLOAD endbitpos.%d len.%d | fpos.%ld\n",(int)endbitpos,len,ftell(fp));
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
    char *dirname = (0*readonly != 0) ? "/Users/jimbolaptop/ramchains" : "ramchains";
    if ( subgroup < 0 )
        sprintf(fname,"%s/%s/%s.%s",dirname,coinstr,coinstr,typestr);
    else sprintf(fname,"%s/%s/%s/%s.%d",dirname,coinstr,typestr,coinstr,subgroup);
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
        if ( pair->code != 0 )//&& (fp= fopen(fname,"wb")) != 0 )
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
            offset = -1;
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
            //    sleep(1);
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
    if ( 0 )
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
//#ifndef RAM_GENMODE
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
                hash->newfp = fopen(fname,"wb");
                if ( hash->newfp != 0 )
                {
                    printf("couldnt create (%s)\n",fname);
                    exit(-1);
                }
            }
            ptr = permalloc(coinstr,mem,sizeof(*ptr),3);
            newptr = permalloc(coinstr,mem,datalen,4);
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

uint32_t ram_conv_hashstr(int32_t createflag,struct ramchain_info *ram,char *hashstr,char type)
{
    char nullstr[6] = { 5, 'n', 'u', 'l', 'l', 0 };
    struct ramchain_hashptr *ptr = 0;
    struct ramchain_hashtable *hash;
    if ( hashstr == 0 || hashstr[0] == 0 )
        hashstr = nullstr;
    hash = ram_gethash(ram,type);
    if ( (ptr= ram_hashsearch(ram->name,&ram->Perm,createflag,hash,hashstr,type)) != 0 )
    {
        if ( (hash->ind + 1) > ram->maxind )
            ram->maxind = (hash->ind + 1);
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
    if ( (rawind= ram_conv_hashstr(0,ram,hashstr,type)) != 0 && (ptr= ram_gethashptr(ram,type,rawind)) != 0 )
    {
        if ( ptrp != 0 )
            *ptrp = ptr;
        *numpayloadsp = ptr->numpayloads;
        return(ptr->payloads);
    }
    else return(0);
}

#define ram_addrpayloads(addrptrp,numpayloadsp,ram,addr) ram_payloads(addrptrp,numpayloadsp,ram,addr,'a')
#define ram_txpayloads(txptrp,numpayloadsp,ram,txidstr) ram_payloads(txptrp,numpayloadsp,ram,txidstr,'t')

struct address_entry *ram_address_entry(struct address_entry *destbp,struct ramchain_info *ram,char *txidstr,int32_t vout)
{
    int32_t numpayloads;
    struct rampayload *payloads;
    if ( (payloads= ram_txpayloads(0,&numpayloads,ram,txidstr)) != 0 )
    {
        if ( vout < payloads[0].B.v )
        {
            *destbp = payloads[vout+1].B;
            return(destbp);
        }
    }
    return(0);
}

struct rampayload *ram_getpayloadi(struct ramchain_hashptr **addrptrp,struct ramchain_info *ram,char type,uint32_t rawind,uint32_t i)
{
    struct ramchain_hashptr *ptr;
    if ( addrptrp != 0 )
        *addrptrp = 0;
    if ( (ptr= ram_gethashptr(ram,type,rawind)) != 0 && i < ptr->numpayloads && ptr->payloads != 0 )
    {
        if ( addrptrp != 0 )
            *addrptrp = ptr;
        return(&ptr->payloads[i]);
    }
    return(0);
}

#define ram_scriptind(ram,hashstr) ram_conv_hashstr(1,ram,hashstr,'s')
#define ram_addrind(ram,hashstr) ram_conv_hashstr(1,ram,hashstr,'a')
#define ram_txidind(ram,hashstr) ram_conv_hashstr(1,ram,hashstr,'t')
// Make sure queries dont autocreate hashtable entries
#define ram_scriptind_RO(ram,hashstr) ram_conv_hashstr(0,ram,hashstr,'s')
#define ram_addrind_RO(ram,hashstr) ram_conv_hashstr(0,ram,hashstr,'a')
#define ram_txidind_RO(ram,hashstr) ram_conv_hashstr(0,ram,hashstr,'t')

#define ram_conv_rawind(hashstr,ram,rawind,type) ram_decode_hashdata(hashstr,type,ram_gethashdata(ram,type,rawind))
#define ram_txid(hashstr,ram,rawind) ram_conv_rawind(hashstr,ram,rawind,'t')
#define ram_addr(hashstr,ram,rawind) ram_conv_rawind(hashstr,ram,rawind,'a')
#define ram_script(hashstr,ram,rawind) ram_conv_rawind(hashstr,ram,rawind,'s')

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
        cJSON_AddItemToObject(json,"allocsize",cJSON_CreateNumber(allocsize));
    if ( (n= raw->numtx) > 0 )
    {
        array = cJSON_CreateArray();
        for (i=0; i<n; i++)
            cJSON_AddItemToArray(array,ram_rawtx_json(raw,i));
        cJSON_AddItemToObject(json,"tx",array);
    }
    return(json);
}

/*cJSON *ram_blockjson(struct rawblock *tmp,struct ramchain_info *ram,struct rawblock *raw)
{
    cJSON *json = 0;
    struct ramchain_token **tokens;
    int32_t numtokens;
    if ( (tokens= ram_tokenize_rawblock(&numtokens,ram,raw)) != 0 )
        ram_expand_and_free(&json,tmp,ram,tokens,numtokens,0);
    return(json);
}*/

struct rawvout *ram_rawvout(struct rawblock *raw,int32_t txind,int32_t v)
{
    struct rawtx *tx;
    if ( txind < raw->numtx )
    {
        tx = &raw->txspace[txind];
        return(&raw->voutspace[tx->firstvout + v]);
    }
    return(0);
}

struct rawvin *ram_rawvin(struct rawblock *raw,int32_t txind,int32_t v)
{
    struct rawtx *tx;
    if ( txind < raw->numtx )
    {
        tx = &raw->txspace[txind];
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
        rawind = ram_conv_hashstr(0,ram,hashstr,type);
    }
    else
    {
        if ( format == 'B' )
            hdecode_varbits(&rawind,hp);
        else if ( format == '*' )
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
    else if ( format == '*' )
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
    else if ( format == '*' )
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
    else if ( format == '*' )
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
            rawind = ram_conv_hashstr(0,ram,hashstr,type);
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
                sleep(1);
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
                sleep(1);
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
                sleep(1);
            return(orignumtokens);
        }
        if ( (txid_rawind= ram_extractstring(txidstr,'t',ram,'T',(txind<<1),hp,format)) != 0 )
            tokens[numtokens++] = ram_createstring(ram,'T',(txind<<1),'t',txidstr,txid_rawind);
        else return(orignumtokens);
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
    /*int32_t i;
    for (i=0; i<numtokens; i++)
        if ( tokens[i] != 0 )
            free(tokens[i]);
    free(tokens);*/
    if ( numtokensp != 0 )
        *numtokensp = -1;
    return(0);
}

struct ramchain_token **ram_tokenize_bitstream(uint32_t *blocknump,int32_t *numtokensp,struct ramchain_info *ram,HUFF *hp,int32_t format)
{
    // 'V' packed structure using varints and varstrs
    // 'B' bitstream using varbits and rawind substitution for strings
    // '*' bitstream using huffman codes
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
        //    sleep(1);
        return(ram_purgetokens(numtokensp,tokens,numtokens));
    }
    if ( numtx > 0 )
    {
        lastnumtokens = numtokens;
        for (i=firstvin=firstvout=0; i<numtx; i++)
            if ( (numtokens= ram_rawtx_huffscan(ram,tokens,numtokens,hp,format,i,&firstvin,&firstvout)) == lastnumtokens )
            {
                printf("parse error at token %d of %d | firstvin.%d firstvout.%d\n",i,numtokens,firstvin,firstvout);
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
        if ( format != 'B' && format != 'V' && format != '*' )
            printf("error hdecode_bits in ram_expand_rawinds format.%d != (%c/%c/%c) %d/%d/%d\n",format,'V','B','*','V','B','*');
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
    if ( format == 'V' || format == 'B' )
    {
        formatstr[1] = 0;
        formatstr[0] = format;
    }
    else sprintf(formatstr,"B%d",format);
}

void ram_setdirA(char *dirA,struct ramchain_info *ram)
{
    sprintf(dirA,"%s/ramchains/%s/bitstream",ram->dirpath,ram->name);
}

void ram_setdirB(int32_t mkdirflag,char *dirB,struct ramchain_info *ram,uint32_t blocknum)
{
    static char lastdirB[1024];
    char dirA[1024];
    int32_t i;
    blocknum %= (64 * 64 * 64);
    ram_setdirA(dirA,ram);
    i = blocknum / (64 * 64);
    sprintf(dirB,"%s/%05x_%05x",dirA,i*64*64,(i+1)*64*64-1);
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
    sprintf(dirC,"%s/%05x_%05x",dirB,i*64*64 + j*64,i*64*64 + (j+1)*64 - 1);
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
    sprintf(fname,"%s/%u.%s",dirC,blocknum,str);
}

void ram_purge_badblock(struct ramchain_info *ram,uint32_t blocknum)
{
    char fname[1024];
    ram_setfname(fname,ram,blocknum,"V");
    delete_file(fname,0);
    ram_setfname(fname,ram,blocknum,"B");
    delete_file(fname,0);
    ram_setfname(fname,ram,blocknum,"B64");
    delete_file(fname,0);
    ram_setfname(fname,ram,blocknum,"B4096");
    delete_file(fname,0);
}

HUFF *ram_genblock(HUFF *tmphp,struct rawblock *tmp,struct ramchain_info *ram,int32_t blocknum,int32_t format,HUFF **prevhpp)
{
    HUFF *hp = 0;
    int32_t datalen,regenflag = 0;
    void *block = 0;
    if ( format == 0 )
        format = 'V';
    if ( format == 'B' && prevhpp != 0 && (hp= *prevhpp) != 0 )
    {
        if ( ram_expand_bitstream(0,tmp,ram,hp) <= 0 )
        {
            char fname[1024],formatstr[16];
            ram_setformatstr(formatstr,'V');
            ram_setfname(fname,ram,blocknum,formatstr);
            delete_file(fname,0);
            ram_setformatstr(formatstr,'B');
            ram_setfname(fname,ram,blocknum,formatstr);
            delete_file(fname,0);
            regenflag = 1;
            hp = 0;
            printf("ram_genblock fatal error generating %s blocknum.%d\n",ram->name,blocknum);
            exit(-1);
        }
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
    hp = 0;
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

HUFF *ram_getblock(struct ramchain_info *ram,uint32_t blocknum)
{
    HUFF **hpp;
    if ( (hpp= ram_get_hpptr(&ram->blocks,blocknum)) != 0 )
        return(*hpp);
    return(0);
}

char *ram_script_json(struct ramchain_info *ram,uint32_t rawind)
{
    char hashstr[8193],retbuf[1024],*str;
    if ( (str= ram_script(hashstr,ram,rawind)) != 0 )
    {
        sprintf(retbuf,"{\"result\":\"%u\",\"script\":\"%s\"}",rawind,str);
        return(clonestr(retbuf));
    }
    return(clonestr("{\"error\":\"no script info\"}"));
}

char *ram_addr_json(struct ramchain_info *ram,uint32_t rawind)
{
    char hashstr[8193],retbuf[1024],*str;
    if ( (str= ram_addr(hashstr,ram,rawind)) != 0 )
    {
        sprintf(retbuf,"{\"result\":\"%u\",\"addr\":\"%s\"}",rawind,str);
        return(clonestr(retbuf));
    }
    return(clonestr("{\"error\":\"no addr info\"}"));
}

char *ram_txid_json(struct ramchain_info *ram,uint32_t rawind)
{
    cJSON *json = cJSON_CreateObject();
    char hashstr[8193],*txidstr,*retstr;
    if ( (txidstr= ram_txid(hashstr,ram,rawind)) != 0 )
    {
        cJSON_AddItemToObject(json,"result",cJSON_CreateNumber(rawind));
        cJSON_AddItemToObject(json,"txid",cJSON_CreateString(txidstr));
        retstr = cJSON_Print(json);
        free_json(json);
        return(retstr);
    }
    return(clonestr("{\"error\":\"no txid info\"}"));
}

char *ram_addrind_json(struct ramchain_info *ram,char *str)
{
    char retbuf[1024];
    uint32_t rawind;
    if ( (rawind= ram_addrind_RO(ram,str)) != 0 )
    {
        sprintf(retbuf,"{\"result\":\"%s\",\"rawind\":\"%u\"}",str,rawind);
        return(clonestr(retbuf));
    }
    return(clonestr("{\"error\":\"no addr info\"}"));
}

char *ram_txidind_json(struct ramchain_info *ram,char *str)
{
    char retbuf[1024];
    uint32_t rawind;
    if ( (rawind= ram_txidind_RO(ram,str)) != 0 )
    {
        sprintf(retbuf,"{\"result\":\"%s\",\"rawind\":\"%u\"}",str,rawind);
        return(clonestr(retbuf));
    }
    return(clonestr("{\"error\":\"no txid info\"}"));
}

char *ram_scriptind_json(struct ramchain_info *ram,char *str)
{
    char retbuf[1024];
    uint32_t rawind;
    if ( (rawind= ram_scriptind_RO(ram,str)) != 0 )
    {
        sprintf(retbuf,"{\"result\":\"%s\",\"rawind\":\"%u\"}",str,rawind);
        return(clonestr(retbuf));
    }
    return(clonestr("{\"error\":\"no script info\"}"));
}

void ram_setdispstr(char *buf,struct ramchain_info *ram,double startmilli)
{
    double estimatedV,estimatedB,estsizeV,estsizeB;
    estimatedV = estimate_completion(ram->name,startmilli,ram->Vblocks.processed,(int32_t)ram->RTblocknum-ram->Vblocks.blocknum)/60000;
    estimatedB = estimate_completion(ram->name,startmilli,ram->Bblocks.processed,(int32_t)ram->RTblocknum-ram->Bblocks.blocknum)/60000;
    if ( ram->Vblocks.count != 0 )
        estsizeV = (ram->Vblocks.sum / ram->Vblocks.count) * ram->RTblocknum;
    if ( ram->Bblocks.count != 0 )
        estsizeB = (ram->Bblocks.sum / ram->Bblocks.count) * ram->RTblocknum;
    sprintf(buf,"%-5s: RT.%d nonz.%d V.%d B.%d B64.%d B4096.%d | %s %s R%.2f | minutes: V%.1f B%.1f | outputs.%llu %.8f spends.%llu %.8f -> balance: %llu %.8f ave %.8f",ram->name,ram->RTblocknum,ram->nonzblocks,ram->Vblocks.blocknum,ram->Bblocks.blocknum,ram->blocks64.blocknum,ram->blocks4096.blocknum,_mbstr(estsizeV),_mbstr2(estsizeB),estsizeV/(estsizeB+1),estimatedV,estimatedB,(long long)ram->numoutputs,dstr(ram->totaloutputs),(long long)ram->numspends,dstr(ram->totalspends),(long long)(ram->numoutputs - ram->numspends),dstr(ram->totaloutputs - ram->totalspends),dstr(ram->totaloutputs - ram->totalspends)/(ram->numoutputs - ram->numspends));
}

cJSON *ram_address_entry_json(struct address_entry *bp)
{
    cJSON *array = cJSON_CreateArray();
    cJSON_AddItemToArray(array,cJSON_CreateNumber(bp->blocknum));
    cJSON_AddItemToArray(array,cJSON_CreateNumber(bp->txind));
    cJSON_AddItemToArray(array,cJSON_CreateNumber(bp->v));
    return(array);
}

cJSON *ram_addrpayload_json(struct ramchain_info *ram,struct rampayload *payload)
{
    char txidstr[8192];
    ram_txid(txidstr,ram,payload->otherind);
    cJSON *json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"txid",cJSON_CreateString(txidstr));
    cJSON_AddItemToObject(json,"vout",cJSON_CreateNumber(payload->B.v));
    cJSON_AddItemToObject(json,"value",cJSON_CreateNumber(dstr(payload->value)));
    cJSON_AddItemToObject(json,"txid_rawind",cJSON_CreateNumber(payload->otherind));
    cJSON_AddItemToObject(json,"txout",ram_address_entry_json(&payload->B));
    if ( payload->B.isinternal != 0 )
        cJSON_AddItemToObject(json,"MGWinternal",cJSON_CreateNumber(1));
    if ( payload->B.spent != 0 )
        cJSON_AddItemToObject(json,"spent",ram_address_entry_json(&payload->spentB));
    cJSON_AddItemToObject(json,"scriptind",cJSON_CreateNumber(payload->extra));
    return(json);
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

int32_t ram_save_bitstreams(bits256 *refsha,char *fname,HUFF *bitstreams[],int32_t num)
{
    FILE *fp;
    bits256 tmp;
    int32_t i,len = -1;
    if ( (fp= fopen(fname,"wb")) != 0 )
    {
        memset(refsha,0,sizeof(*refsha));
        for (i=0; i<num; i++)
        {
            //printf("i.%d %p\n",i,bitstreams[i]);
            if ( bitstreams[i] != 0 && bitstreams[i]->buf != 0 )
                calc_sha256cat(tmp.bytes,refsha->bytes,sizeof(*refsha),bitstreams[i]->buf,bitstreams[i]->allocsize), *refsha = tmp;
            else
            {
                printf("bitstreams[%d] == 0? %p\n",i,bitstreams[i]);
                fclose(fp);
                return(-1);
            }
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
    if ( (fp= fopen(fname,"rb")) != 0 )
    {
        memset(sha,0,sizeof(*sha));
        //printf("loading %s\n",fname);
        if ( fread(&x,1,sizeof(x),fp) == sizeof(x) && ((*nump) == 0 || x == (*nump)) )
        {
            if ( (*nump) == 0 )
            {
                (*nump) = x;
                printf("set num to %d\n",x);
            }
            offsets = calloc((*nump),sizeof(*offsets));
            if ( fread(&stored,1,sizeof(stored),fp) == sizeof(stored) )
            {
                for (i=0; i<(*nump); i++)
                {
                    //if ( bitstreams[i] != 0 )
                    //    hclose(bitstreams[i]);
                    if ( (bitstreams[i]= hload(ram,&offsets[i],fp,0)) != 0 && bitstreams[i]->buf != 0 )
                        calc_sha256cat(tmp.bytes,sha->bytes,sizeof(*sha),bitstreams[i]->buf,bitstreams[i]->allocsize), *sha = tmp;
                    else printf("unexpected null bitstream at %d %p offset.%ld\n",i,bitstreams[i],offsets[i]);
                }
                if ( memcmp(sha,&stored,sizeof(stored)) != 0 )
                    printf("sha error %s %llx vs stored.%llx\n",fname,(long long)sha->txid,(long long)stored.txid);
            } else printf("error loading sha\n");
        } else printf("num mismatch %d != num.%d\n",x,(*nump));
        len = (int32_t)ftell(fp);
        fclose(fp);
    }
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
        if ( refsha != 0 && memcmp(sha->bytes,refsha,sizeof(*sha)) != 0 )
        {
            printf("refsha cmp error for %s %llx vs %llx\n",fname,(long long)sha->txid,(long long)refsha->txid);
            hpurge(blocks,num);
            free(offsets);
            return(0);
        }
        //if ( M->fileptr != 0 )
        //    close_mappedptr(M);
        memset(M,0,sizeof(*M));
        if ( init_mappedptr(0,M,0,rwflag,fname) != 0 )
        {
            for (i=0; i<num; i++)
            {
                if ( i > 0 && (i % 4096) == 4095 )
                    fprintf(stderr,"%.1f%% ",100.*(double)i/num);
                if ( (hp= blocks[i]) != 0 )
                {
                    if ( (blocks[i]= hopen(ram->name,&ram->Perm,(void *)((long)M->fileptr + offsets[i]),hp->allocsize,0)) != 0 )
                    {
                        if ( verifyflag == 0 || (checkblock= ram_verify(ram,blocks[i],'B')) == blocknum+i )
                        {
                            verified++;
                            hclose(hp);
                        }
                        else
                        {
                            printf("checkblock.%d vs %d (blocknum.%d + i.%d)\n",checkblock,blocknum+i,blocknum,i);
                            break;
                        }
                    } else blocks[i] = hp;
                } else printf("ram_map_bitstreams: ram_map_bitstreams unexpected null hp at slot.%d\n",i);
            }
            if ( i == num )
            {
                retval = (int32_t)M->allocsize;
                for (i=0; i<num; i++)
                    if ( (hp= blocks[i]) != 0 && ram->blocks.hps[blocknum+i] == 0 )
                        ram->blocks.hps[blocknum+i] = hp, n++;
            }
            else
            {
                for (i=0; i<num; i++)
                    if ( (hp= blocks[i]) != 0 )
                        hclose(hp), blocks[i] = 0;
                close_mappedptr(M);
                memset(M,0,sizeof(*M));
                delete_file(fname,0);
                printf("%s: only %d of %d blocks verified\n",fname,verified,num);
            }
            //close_mappedptr(&M);
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
    char fname[1024],formatstr[16];
    uint32_t blocknum,i,flag,incr,n = 0;
    incr = (1 << blocks->shift);
    ram_setformatstr(formatstr,blocks->format);
    flag = blocks->format == 'B';
    firstblock &= ~(incr - 1);
    for (i=0; i<numblocks; i+=incr)
    {
        blocknum = (firstblock + i);
        ram_setfname(fname,ram,blocknum,formatstr);
        //printf("loading (%s)\n",fname);
        if ( (hps= ram_get_hpptr(blocks,blocknum)) != 0 )
        {
            if ( blocks->format == 64 || blocks->format == 4096 )
            {
                if ( ram_map_bitstreams(flag,ram,blocknum,&blocks->M[blocknum >> blocks->shift],&sha,hps,incr,fname,0) <= 0 )
                {
                    //break;
                }
            }
            else
            {
//#ifdef RAM_GENMODE
//              if ( (*hps= hload(&ram->Tmp,0,0,fname)) != 0 )
//#else
                if ( (*hps= hload(ram,0,0,fname)) != 0 )
//#endif
                {
#ifdef RAM_GENMODE
                    if ( (*hps)->allocsize < 12 )
                        delete_file(fname,0);
                    else n++;
                    hclose(*hps);
                    ram_clear_alloc_space(&ram->Tmp);
#else
                    if ( flag == 0 || ram_verify(ram,*hps,blocks->format) == blocknum )
                    {
                        //if ( flag != 0 )
                        //    fprintf(stderr,"=");
                        n++;
                        if ( blocks->format == 'B' && ram->blocks.hps[blocknum] == 0 )
                            ram->blocks.hps[blocknum] = *hps;
                    }
                    else hclose(*hps), *hps = 0;
#endif
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
    if ( blocks->format == 'V' && (fp= fopen(fname,"rb")) != 0 )
    {
        fclose(fp);
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
                if ( (fp= fopen(fname,"wb")) != 0 )
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
            if ( *hpptr == 0 && (hp= ram_genblock(blocks->tmphp,blocks->R,ram,blocknum,blocks->format,prevhps)) != 0 )
            {
                //printf("block.%d created.%c block.%d numtx.%d minted %.8f\n",blocknum,blocks->format,blocks->R->blocknum,blocks->R->numtx,dstr(blocks->R->minted));
                if ( (fp= fopen(fname,"wb")) != 0 )
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
                            fprintf(stderr," %s CREATED.%c block.%d datalen.%d\n",ram->name,blocks->format,blocknum,datalen+1);
                        //else fprintf(stderr,"%s.B.%d ",ram->name,blocknum);
                        if ( *hpptr != 0 )
                        {
                            hclose(*hpptr);
                            *hpptr = 0;
                            printf("OVERWRITE.(%s) size.%ld bitoffset.%d allocsize.%d\n",fname,ftell(fp),hp->bitoffset,hp->allocsize);
                        }
#ifdef RAM_GENMODE
                        hclose(hp);
#else
                        *hpptr = hp;
                        if ( blocks->format != 'V' && ram->blocks.hps[blocknum] == 0 )
                            ram->blocks.hps[blocknum] = hp;
#endif
                    }
                } else delete_file(fname,0), hclose(hp);
            }
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
                numblocks = ram_map_bitstreams(verifyflag,ram,blocknum,&blocks->M[blocknum >> blocks->shift],&sha,hps,n,fname,&refsha);
        } else printf("%s prev.%d missing blockptr at %d\n",ram->name,prevblocks->format,blocknum+i);
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

long ram_emit_blockcheck(FILE *fp,uint64_t blocknum)
{
    long fpos,retval = 0;
    uint64_t blockcheck;
    if ( fp != 0 )
    {
        fpos = ftell(fp);
        blockcheck = (~blocknum << 32) | blocknum;
        retval = fwrite(&blockcheck,1,sizeof(blockcheck),fp);
        fseek(fp,fpos,SEEK_SET);
        fflush(fp);
    }
    return(retval);
}

uint32_t ram_load_blockcheck(FILE *fp)
{
    long fpos;
    uint64_t blockcheck;
    uint32_t blocknum = 0;
    fpos = ftell(fp);
    if ( fread(&blockcheck,1,sizeof(blockcheck),fp) != sizeof(blockcheck) || (uint32_t)(blockcheck >> 32) != ~(uint32_t)blockcheck )
        blocknum = 0;
    else
    {
        blocknum = (uint32_t)blockcheck;
        printf("found valid marker blocknum %llx -> %u fpos.%ld afterread.%ld\n",(long long)blockcheck,blocknum,fpos,ftell(fp));
    }
    fseek(fp,fpos,SEEK_SET);
    return(blocknum);
}

int32_t ram_init_hashtable(uint32_t *blocknump,struct ramchain_info *ram,char type)
{
    long offset,len,fileptr;
    uint64_t datalen;
    char fname[1024],destfname[1024],str[64];
    uint8_t *hashdata;
    int32_t varsize,num,rwflag = 0;
    struct ramchain_hashtable *hash;
    struct ramchain_hashptr *hp;
    hash = ram_gethash(ram,type);
    strcpy(hash->coinstr,ram->name);
    hash->type = type;
    ram_sethashname(fname,hash,0);
    num = 0;
    printf("inithashtable.(%s.%d) -> [%s]\n",ram->name,type,fname);
    if ( (hash->newfp= fopen(fname,"rb+")) != 0 )
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
                struct ramchain_hashptr *checkhp;
                init_hexbytes_noT(hexbytes,hashdata,varsize+datalen);
                HASH_FIND(hh,hash->table,hashdata,varsize + datalen,checkhp);
                fprintf(stderr,"%s offset %ld: varsize.%d datalen.%d created.(%s) ind.%d | checkhp.%p\n",ram->name,offset,(int)varsize,(int)datalen,(type != 'a') ? hexbytes :(char *)((long)hashdata+varsize),hash->ind+1,checkhp);
            }
            HASH_FIND(hh,hash->table,hashdata,varsize + datalen,hp);
            if ( hp != 0 )
            {
                printf("corrupted hashtable %s: offset.%ld\n",fname,offset);
                exit(-1);
            }
            hp = permalloc(ram->name,&ram->Perm,sizeof(*hp),6);
            ram_addhash(hash,hp,hashdata,(int32_t)(varsize+datalen));
            offset += (varsize + datalen);
            fseek(hash->newfp,offset,SEEK_SET);
            num++;
        }
        printf("%s: loaded %d strings, ind.%d, offset.%ld allocsize.%llu %s\n",fname,num,hash->ind,offset,(long long)hash->M.allocsize,((sizeof(uint64_t)+offset) != hash->M.allocsize && offset != hash->M.allocsize) ? "ERROR":"OK");
        if ( offset != hash->M.allocsize )
        {
            *blocknump = ram_load_blockcheck(hash->newfp);
            if ( (offset+sizeof(uint64_t)) != hash->M.allocsize )
                exit(-1);
        }
        if ( (hash->ind + 1) > ram->maxind )
            ram->maxind = (hash->ind + 1);
        ram_sethashtype(str,hash->type);
        sprintf(destfname,"ramchains/%s.%s",ram->name,str);
        if ( (len= copy_file(fname,destfname)) > 0 )
            printf("copied (%s) -> (%s) %s\n",fname,destfname,_mbstr(len));
        return(0);
    } else hash->newfp = fopen(fname,"wb");
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

void ram_disp_status(struct ramchain_info *ram)
{
    char buf[1024];
    int32_t i,n = 0;
    for (i=0; i<ram->RTblocknum; i++)
    {
        if ( ram->blocks.hps[i] != 0 )
            n++;
    }
    ram->nonzblocks = n;
    ram_setdispstr(buf,ram,ram->startmilli);
    fprintf(stderr,"%s\n",buf);
}

int32_t ram_markspent(struct ramchain_info *ram,struct rampayload *txpayload,struct address_entry *spendbp,uint32_t txid_rawind)
{
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
                ram->totalspends += txpayload->value;
                ram->numspends++;
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

void ram_addunspent(struct ramchain_info *ram,struct rampayload *txpayload,struct ramchain_hashptr *addrptr,struct rampayload *addrpayload,uint32_t addr_rawind,uint32_t ind)
{
    // ram_addunspent() is called from the rawtx_update that creates the txpayloads, each coinaddr gets a list of addrpayloads initialized
    // it needs to update the corresponding txpayload so exact state can be quickly calculated
    txpayload->B = addrpayload->B;
    txpayload->value = addrpayload->value;
    //printf("txpayload.%p addrpayload.%p set addr_rawind.%d -> %p txid_rawind.%u\n",txpayload,addrpayload,addr_rawind,txpayload,addrpayload->otherind);
    txpayload->otherind = addr_rawind;  // allows for direct lookup of coin addr payload vector
    txpayload->extra = ind;             // allows for direct lookup of the payload within the vector
    if ( addrpayload->value != 0 )
    {
        //printf("UNSPENT %.8f\n",dstr(addrpayload->value));
        ram->totaloutputs += addrpayload->value;
        ram->numoutputs++;
        addrptr->unspent += addrpayload->value;
        addrptr->numunspent++;
    }
}

int32_t ram_rawvout_update(int32_t iter,uint32_t *addr_rawindp,struct rampayload *txpayload,struct ramchain_info *ram,HUFF *hp,uint32_t blocknum,uint16_t txind,uint16_t vout,uint16_t numvouts,uint32_t txid_rawind,int32_t isinternal)
{
    struct rampayload payload;
    struct ramchain_hashtable *table;
    struct ramchain_hashptr *addrptr;
    struct rawvout_huffs *pair;
    uint32_t scriptind,addrind;
    char *str;
    uint64_t value;
    int32_t numbits = 0;
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
    if ( scriptind > 0 && scriptind <= table->ind )
    {
        table = ram_gethash(ram,'a');
        if ( addrind > 0 && addrind <= table->ind && (addrptr= table->ptrs[addrind]) != 0 )
        {
            *addr_rawindp = addrind;
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
                    addrptr->maxpayloads = (addrptr->numpayloads + 1);
                    addrptr->payloads = realloc(addrptr->payloads,addrptr->maxpayloads * sizeof(struct rampayload));
                }
                if ( iter <= 2 )
                {
                    memset(&payload,0,sizeof(payload));
                    payload.B.blocknum = blocknum, payload.B.txind = txind, payload.B.v = vout, payload.B.isinternal = isinternal;
                    payload.otherind = txid_rawind, payload.extra = scriptind, payload.value = value;
                    ram_addunspent(ram,txpayload,addrptr,&payload,addrind,addrptr->numpayloads);
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
    struct address_entry *bps,B,*bp;
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
            memset(&B,0,sizeof(B)), B.blocknum = blocknum, B.txind = txind, B.v = vin, B.spent = 1;
            if ( (vout+1) < txptr->numpayloads )
            {
                if ( iter <= 2 )
                {
                    bp = &txptr->payloads[vout + 1].spentB;
                    if ( memcmp(bp,&zeroB,sizeof(zeroB)) == 0 )
                        ram_markspent(ram,&txptr->payloads[vout + 1],&B,txid_rawind);
                    else if ( memcmp(bp,&B,sizeof(B)) == 0 )
                        printf("duplicate spentB (%d %d %d)\n",B.blocknum,B.txind,B.v);
                    else printf("interloper! (%d %d %d).%d vs (%d %d %d).%d\n",bp->blocknum,bp->txind,bp->v,bp->spent,B.blocknum,B.txind,B.v,B.spent);
                }
                return(numbits);
            } else printf("(%d %d %d) vout.%d overflows bp->v.%d\n",blocknum,txind,vin,vout,B.v);
        } else printf("rawvin_update: unexpected null table->ptrs[%d] or no payloads.%p\n",txid_rawind,bps);
    } else printf("txid_rawind.%u out of range %d\n",txid_rawind,table->ind);
    return(-1);
}

int32_t ram_rawtx_update(int32_t iter,struct ramchain_info *ram,HUFF *hp,uint32_t blocknum,uint16_t txind)
{
    struct rampayload payload;
    struct ramchain_hashptr *txptr;
    char *str;
    uint32_t addr_rawind,txid_rawind = 0;
    int32_t i,retval,isinternal,numbits = 0;
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
                    payload.B.blocknum = blocknum, payload.B.txind = txind, payload.B.v = numvouts;
                    txptr->numpayloads = (numvouts + 1);
                    //printf("%p txid_rawind.%d maxpayloads.%d numpayloads.%d (%d %d %d)\n",txptr,txid_rawind,txptr->maxpayloads,txptr->numpayloads,blocknum,txind,numvouts);
                    txptr->payloads = (struct rampayload *)permalloc(ram->name,&ram->Perm,txptr->numpayloads * sizeof(*txptr->payloads),7);
                    txptr->payloads[0] = payload;
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
                for (i=isinternal=0; i<numvouts; i++,numbits+=retval)
                {
                    if ( (retval= ram_rawvout_update(iter,&addr_rawind,iter==0?0:&txptr->payloads[i+1],ram,hp,blocknum,txind,i,numvouts,txid_rawind,isinternal*(i==(numvouts-1)))) < 0 )
                        return(-2);
                    if ( i == 0 && addr_rawind == ram->marker_rawind )
                        isinternal = 1;
                }
            }
            return(numbits);
        }
    } else printf("ram_rawtx_update: parse error\n");
    return(-3);
}

int32_t ram_rawblock_update(int32_t iter,struct ramchain_info *ram,HUFF *hp,uint32_t checkblocknum)
{
    uint16_t numtx;
    uint64_t minted;
    uint32_t blocknum;
    int32_t txind,numbits,retval,format,datalen = 0;
#ifdef RAM_GENMODE
    return(0);
#endif
    hrewind(hp);
    format = hp->buf[datalen++], hp->ptr++, hp->bitoffset = 8;
    if ( format != 'B' )
    {
        printf("only format B supported for now\n");
        return(-1);
    }
    numbits = hdecode_varbits(&blocknum,hp);
    if ( blocknum != checkblocknum )
    {
        printf("ram_rawblock_update: blocknum.%d vs checkblocknum.%d\n",blocknum,checkblocknum);
        return(-1);
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

uint32_t ram_process_blocks(struct ramchain_info *ram,struct mappedblocks *blocks,struct mappedblocks *prev,double timebudget)
{
    HUFF **hpptr,*hp = 0;
    char formatstr[16];
    double estimated,startmilli = ram_millis();
    int32_t newflag,processed = 0;
    ram_setformatstr(formatstr,blocks->format);
    //printf("%s shift.%d %-5s.%d %.1f min left | [%d < %d]? %f %f timebudget %f\n",formatstr,blocks->shift,ram->name,blocks->blocknum,estimated,(blocks->blocknum >> blocks->shift),(prev->blocknum >> blocks->shift),ram_millis(),(startmilli + timebudget),timebudget);
    while ( (blocks->blocknum >> blocks->shift) < (prev->blocknum >> blocks->shift) && ram_millis() < (startmilli + timebudget) )
    {
        //printf("inside\n");
        newflag = (ram->blocks.hps[blocks->blocknum] == 0);
        ram_create_block(1,ram,blocks,prev,blocks->blocknum), processed++;
        if ( (hpptr= ram_get_hpptr(blocks,blocks->blocknum)) != 0 && (hp= *hpptr) != 0 )
        {
            if ( blocks->format == 'B' && newflag != 0 )//&& ram->blocks.hps[blocks->blocknum] == 0 )
                ram_rawblock_update(2,ram,hp,blocks->blocknum);
            //else printf("hpptr.%p hp.%p newflag.%d\n",hpptr,hp,newflag);
        } //else printf("ram_process_blocks: hpptr.%p hp.%p\n",hpptr,hp);
        if ( blocks->format == 'B' && blocks->blocknum >= ram->RTblocknum-1 )
        {
            ram_emit_blockcheck(ram_gethash(ram,'a')->newfp,blocks->blocknum);
            ram_emit_blockcheck(ram_gethash(ram,'t')->newfp,blocks->blocknum);
            ram_emit_blockcheck(ram_gethash(ram,'s')->newfp,blocks->blocknum);
        }
        blocks->processed += (1 << blocks->shift);
        blocks->blocknum += (1 << blocks->shift);
        estimated = estimate_completion(ram->name,startmilli,blocks->processed,(int32_t)ram->RTblocknum-blocks->blocknum) / 60000.;
    }
    //printf("(%d >> %d) < (%d >> %d)\n",blocks->blocknum,blocks->shift,prev->blocknum,blocks->shift);
    return(processed);
}

uint64_t ram_calc_unspent(int32_t *calc_numunspentp,struct ramchain_hashptr **addrptrp,struct ramchain_info *ram,char *addr)
{
    uint64_t unspent = 0;
    int32_t i,numpayloads,n = 0;
    struct rampayload *payloads;
    if ( calc_numunspentp != 0 )
        *calc_numunspentp = 0;
    if ( (payloads= ram_addrpayloads(addrptrp,&numpayloads,ram,addr)) != 0 && numpayloads > 0 )
    {
        for (i=0; i<numpayloads; i++)
            if ( payloads[i].B.spent == 0 )
                unspent += payloads[i].value, n++;
    }
    if ( calc_numunspentp != 0 )
        *calc_numunspentp = n;
    return(unspent);
}

uint64_t ram_unspent_json(cJSON **arrayp,char *destcoin,double rate,char *coin,char *addr)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    uint64_t unspent = 0;
    cJSON *item;
    if ( ram != 0 )
    {
        if ( (unspent= ram_calc_unspent(0,0,ram,addr)) != 0 && arrayp != 0 )
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

static int _decreasing_uint64_cmp(const void *a,const void *b)
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
}

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
    //*numaddrsp = HASH_COUNT(hash->table);
    //HASH_ITER(hh,hash->table,hp,tmp)
    //    addrs[i++] = hp;
    //if ( i != *numaddrsp )
    //    printf("ram_getallstrs HASH_COUNT.%d vs i.%d\n",*numstrsp,i);
    return(strs);
}

// >>>>>>>>>>>>>>  start external and API interface functions
char *ramstatus(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    char retbuf[1024];
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    ram_setdispstr(retbuf,ram,ram->startmilli);
    return(clonestr(retbuf));
}

char *ramstring(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,char *typestr,uint32_t rawind)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    if ( strcmp(typestr,"script") == 0 )
        return(ram_script_json(ram,rawind));
    else if ( strcmp(typestr,"addr") == 0 )
        return(ram_addr_json(ram,rawind));
    else if ( strcmp(typestr,"txid") == 0 )
        return(ram_txid_json(ram,rawind));
    else return(clonestr("{\"error\":\"no ramstring invalid type\"}"));
}

char *ramrawind(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,char *typestr,char *str)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    if ( strcmp(typestr,"script") == 0 )
        return(ram_scriptind_json(ram,str));
    else if ( strcmp(typestr,"addr") == 0 )
        return(ram_addrind_json(ram,str));
    else if ( strcmp(typestr,"txid") == 0 )
        return(ram_txidind_json(ram,str));
    else return(clonestr("{\"error\":\"no ramrawind invalid type\"}"));
}

char *ramblock(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,uint32_t blocknum)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    char hexstr[8192];
    cJSON *json = 0;
    HUFF *hp;
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
        if ( json != 0 && hp->allocsize < (sizeof(hexstr)/2-1) )
        {
            init_hexbytes_noT(hexstr,hp->buf,hp->allocsize);
            cJSON_AddItemToObject(json,"data",cJSON_CreateString(hexstr));
        }
    }
    if ( json != 0 )
    {
        retstr = cJSON_Print(json);
        free_json(json);
    }
    return(retstr);
}

char *ramcompress(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,char *blockhex)
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
    } else retstr = clonestr("{\"error\":\"no block info\"}");
    free(data);
    return(retstr);
}

char *ramexpand(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,char *bitstream)
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

char *ramscript(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,char *txidstr,int32_t tx_vout,struct address_entry *bp)
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
            if ( (tx_vout+1) < txptr->numpayloads )
            {
                txpayload = &txpayloads[tx_vout+1];
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

char *ramtxlist(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,char *coinaddr,int32_t unspentflag)
{
    char *retstr = 0;
    int64_t total = 0;
    cJSON *json = 0,*array = 0;
    int32_t i,n,numpayloads;
    struct ramchain_hashptr *addrptr;
    struct rampayload *payloads;
    struct ramchain_info *ram = get_ramchain_info(coin);
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    if ( (payloads= ram_addrpayloads(&addrptr,&numpayloads,ram,coinaddr)) != 0 && addrptr != 0 && numpayloads > 0 )
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
        json = cJSON_CreateObject();
        if ( array != 0 )
            cJSON_AddItemToObject(json,coinaddr,array);
        cJSON_AddItemToObject(json,"numtx",cJSON_CreateNumber(numpayloads));
        cJSON_AddItemToObject(json,"calc_numunspent",cJSON_CreateNumber(n));
        cJSON_AddItemToObject(json,"calc_unspent",cJSON_CreateNumber(dstr(total)));
        cJSON_AddItemToObject(json,"numunspent",cJSON_CreateNumber(addrptr->numunspent));
        cJSON_AddItemToObject(json,"unspent",cJSON_CreateNumber(dstr(addrptr->unspent)));
    }
    if ( json != 0 )
    {
        retstr = cJSON_Print(json);
        free_json(json);
    } else retstr = clonestr("{\"error\":\"ramtxlist no data\"}");
    return(retstr);
}

char *ramrichlist(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,int32_t numwhales,int32_t recalcflag)
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
        if ( recalcflag != 0 )
        {
            sortbuf = calloc(2*numaddrs,sizeof(*sortbuf));
            for (i=good=bad=0; i<numaddrs; i++)
            {
                ram_decode_hashdata(coinaddr,'a',addrs[i]->hh.key);
                if ( (unspent= ram_calc_unspent(&numunspent,&addrptr,ram,coinaddr)) != 0 )
                {
                    if ( addrptr->unspent == unspent && addrptr->numunspent == numunspent )
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
                cJSON_AddItemToObject(item,"cumulative errors",cJSON_CreateNumber(bad));
                cJSON_AddItemToObject(item,"cumulative correct",cJSON_CreateNumber(good));
                cJSON_AddItemToObject(item,"calculation milliseconds",cJSON_CreateNumber(ram_millis()-startmilli));
                cJSON_AddItemToArray(array,item);
            }
            free(sortbuf);
        }
        else
        {
            if ( numaddrs > 1 )
                qsort(addrs,numaddrs,sizeof(*addrs),_decreasing_uint64_cmp);
            if ( numaddrs > 0 )
            {
                array = cJSON_CreateArray();
                for (i=0; i<numwhales&&i<numaddrs; i++)
                {
                    item = cJSON_CreateObject();
                    ram_decode_hashdata(coinaddr,'a',addrs[i]->hh.key);
                    cJSON_AddItemToObject(item,coinaddr,cJSON_CreateNumber(addrs[i]->unspent));
                    cJSON_AddItemToArray(array,item);
                }
            }
        }
        free(addrs);
    }
    if ( array != 0 )
    {
        retstr = cJSON_Print(array);
        free_json(array);
    } else retstr = clonestr("{\"error\":\"ramrichlist no data\"}");
    return(retstr);
}

char *rambalances(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,char **coins,double *rates,char ***coinaddrs,int32_t numcoins)
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

// >>>>>>>>>>>>>>  start initialization and runloops
struct mappedblocks *ram_init_blocks(int32_t noload,HUFF **copyhps,struct ramchain_info *ram,uint32_t firstblock,struct mappedblocks *blocks,struct mappedblocks *prevblocks,int32_t format,int32_t shift)
{
    void *ptr;
    int32_t numblocks,tmpsize = TMPALLOC_SPACE_INCR;
    blocks->R = calloc(1,sizeof(*blocks->R));
    blocks->R2 = calloc(1,sizeof(*blocks->R2));
    blocks->R3 = calloc(1,sizeof(*blocks->R3));
    blocks->ram = ram;
    blocks->prevblocks = prevblocks;
    blocks->format = format;
    ptr = calloc(1,tmpsize), blocks->tmphp = hopen(ram->name,&ram->Perm,ptr,tmpsize,0);
    if ( (blocks->shift = shift) != 0 )
        firstblock &= ~((1 << shift) - 1);
    blocks->firstblock = firstblock;
    numblocks = (ram->maxblock+1) - firstblock;
    if ( numblocks < 0 )
    {
        printf("illegal numblocks %d with firstblock.%d vs maxblock.%d\n",blocks->numblocks,firstblock,ram->maxblock);
        exit(-1);
    }
    blocks->numblocks = numblocks;
    if ( blocks->hps == 0 )
        blocks->hps = calloc(blocks->numblocks,sizeof(*blocks->hps));
    if ( format != 0 )
    {
#ifndef RAM_GENMODE
        //blocks->flags = calloc(blocks->numblocks >> (shift+6),sizeof(*blocks->flags));
        blocks->M = calloc(blocks->numblocks >> shift,sizeof(*blocks->M));
        if ( noload == 0 )
            blocks->blocknum = ram_load_blocks(ram,blocks,firstblock,blocks->numblocks);
#endif
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
    ram->RTblocknum = _get_RTheight(ram);
    ram->blocks.blocknum = (ram->RTblocknum - ram->min_confirms);
    return(ram->RTblocknum);
}

void ram_init_ramchain(struct ramchain_info *ram)
{
    int32_t i,datalen,nofile,pass,numblocks,tmpsize = TMPALLOC_SPACE_INCR;
    uint8_t *ptr;
    uint32_t blocknums[3],minblocknum = 0;
    bits256 refsha,sha;
    double startmilli;
    char fname[1024];
    startmilli = ram_millis();
    strcpy(ram->dirpath,".");
    ram->blocks.blocknum = ram->RTblocknum = (_get_RTheight(ram) - ram->min_confirms);
    ram->blocks.numblocks = ram->maxblock = (ram->RTblocknum + 10000);
    ram_init_directories(ram);
    ram->blocks.M = calloc(1,sizeof(*ram->blocks.M));
    ram->blocks.hps = calloc(ram->maxblock,sizeof(*ram->blocks.hps));
    printf("ramchain.%s RT.%d %.1f seconds to init_ramchain_directories\n",ram->name,ram->RTblocknum,(ram_millis() - startmilli)/1000.);
//#ifndef RAM_GENMODE
    ram_init_alloc_space(&ram->Tmp,tmpsize);
    permalloc(ram->name,&ram->Perm,PERMALLOC_SPACE_INCR,0);
    ptr = calloc(1,tmpsize), ram->tmphp = hopen(ram->name,&ram->Perm,ptr,tmpsize,0);
    ram->R = calloc(1,sizeof(*ram->R));
    ram->R2 = calloc(1,sizeof(*ram->R2));
    ram->R3 = calloc(1,sizeof(*ram->R3));
    memset(blocknums,0,sizeof(blocknums));
    nofile = ram_init_hashtable(&blocknums[0],ram,'a');
    nofile += ram_init_hashtable(&blocknums[1],ram,'s');
    nofile += ram_init_hashtable(&blocknums[2],ram,'t');
    if ( nofile == 3 )
    {
        printf("REGEN\n");
        ram->mappedblocks[4] = ram_init_blocks(1,ram->blocks.hps,ram,0,&ram->blocks4096,&ram->blocks64,4096,12);
        ram->mappedblocks[3] = ram_init_blocks(1,ram->blocks.hps,ram,0,&ram->blocks64,&ram->Bblocks,64,6);
        ram->mappedblocks[2] = ram_init_blocks(1,ram->blocks.hps,ram,0,&ram->Bblocks,&ram->Vblocks,'B',0);
        ram->mappedblocks[1] = ram_init_blocks(1,ram->blocks.hps,ram,0,&ram->Vblocks,&ram->blocks,'V',0);
        ram->mappedblocks[0] = ram_init_blocks(0,ram->blocks.hps,ram,0,&ram->blocks,0,0,0);
        ram_update_RTblock(ram);
        for (pass=1; pass<=4; pass++)
            ram_process_blocks(ram,ram->mappedblocks[pass],ram->mappedblocks[pass-1],100000000.);
        printf("FINISHED REGEN\n");
        exit(1);
    } else printf("nofile.%d\n",nofile);
    ram_update_RTblock(ram);
    if ( ram->marker != 0 && ram->marker[0] != 0 && (ram->marker_rawind= ram_addrind_RO(ram,ram->marker)) == 0 )
        printf("WARNING: MARKER.(%s) set but no rawind. need to have it appear in blockchain first\n",ram->marker);
    printf("%.1f seconds to init_ramchain.%s hashtables marker.(%s) %u\n",(ram_millis() - startmilli)/1000.,ram->name,ram->marker,ram->marker_rawind);
    sprintf(fname,"ramchains/%s.blocks",ram->name);
    minblocknum = numblocks = ram_map_bitstreams(1,ram,0,ram->blocks.M,&sha,ram->blocks.hps,0,fname,0);
    for (i=0; i<3; i++)
        if ( minblocknum == 0 || (blocknums[i] != 0 && blocknums[i] < minblocknum) )
            minblocknum = blocknums[i];
    printf("Mapped.(%s) numblocks.%d: sha %llx (%d %d %d) -> minblocknum.%u\n",fname,numblocks,(long long)sha.txid,blocknums[0],blocknums[1],blocknums[2],minblocknum);
    numblocks = ram_setcontiguous(&ram->blocks);

    ram->mappedblocks[4] = ram_init_blocks(0,ram->blocks.hps,ram,(numblocks>>12)<<12,&ram->blocks4096,&ram->blocks64,4096,12);
    printf("set ramchain blocknum.%s %d (1st %d num %d) vs RT.%d %.1f seconds to init_ramchain.%s B4096\n",ram->name,ram->blocks4096.blocknum,ram->blocks4096.firstblock,ram->blocks4096.numblocks,ram->blocks.blocknum,(ram_millis() - startmilli)/1000.,ram->name);
    ram->mappedblocks[3] = ram_init_blocks(0,ram->blocks.hps,ram,ram->blocks4096.contiguous,&ram->blocks64,&ram->Bblocks,64,6);
    printf("set ramchain blocknum.%s %d vs (1st %d num %d) RT.%d %.1f seconds to init_ramchain.%s B64\n",ram->name,ram->blocks64.blocknum,ram->blocks64.firstblock,ram->blocks64.numblocks,ram->blocks.blocknum,(ram_millis() - startmilli)/1000.,ram->name);
    ram->mappedblocks[2] = ram_init_blocks(0,ram->blocks.hps,ram,ram->blocks64.contiguous,&ram->Bblocks,&ram->Vblocks,'B',0);
    printf("set ramchain blocknum.%s %d vs (1st %d num %d) RT.%d %.1f seconds to init_ramchain.%s B\n",ram->name,ram->Bblocks.blocknum,ram->Bblocks.firstblock,ram->Bblocks.numblocks,ram->blocks.blocknum,(ram_millis() - startmilli)/1000.,ram->name);
//#endif
    ram->mappedblocks[1] = ram_init_blocks(0,ram->blocks.hps,ram,ram->Bblocks.contiguous,&ram->Vblocks,&ram->blocks,'V',0);
    printf("set ramchain blocknum.%s %d vs (1st %d num %d) RT.%d %.1f seconds to init_ramchain.%s V\n",ram->name,ram->Vblocks.blocknum,ram->Vblocks.firstblock,ram->Vblocks.numblocks,ram->blocks.blocknum,(ram_millis() - startmilli)/1000.,ram->name);
    ram->mappedblocks[0] = ram_init_blocks(0,ram->blocks.hps,ram,0,&ram->blocks,0,0,0);
#ifndef RAM_GENMODE
    if ( 1 )
    {
        HUFF *hp;
        uint32_t blocknum,errs=0,good=0,iter,i;
        for (iter=0; iter<2; iter++)
        for (errs=good=blocknum=0; blocknum<ram->blocks.contiguous; blocknum++)
        {
            if ( (blocknum % 1000) == 0 )
                fprintf(stderr,".");
            if ( (hp= ram->blocks.hps[blocknum]) != 0 )
            {
                if ( ram_rawblock_update(iter,ram,hp,blocknum) > 0 )
                {
                    good++;
                }
                else
                {
                    printf("iter.%d error on block.%d\n",iter,blocknum);
                    ram_purge_badblock(ram,blocknum);
                    errs++;
                    exit(-1);
                }
            } else errs++;
        }
        fprintf(stderr,"contiguous.%d good.%d errs.%d\n",ram->blocks.contiguous,good,errs);
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
        if ( numblocks == 0 && errs == 0 && ram->blocks.contiguous > 4096 )
        {
            datalen = -1;
            sprintf(fname,"ramchains/%s.blocks",ram->name);
            printf("&refsha.%p (%s) hps.%p cont.%d\n",&refsha,fname,ram->blocks.hps,ram->blocks.contiguous);
            if ( ram_save_bitstreams(&refsha,fname,ram->blocks.hps,ram->blocks.contiguous) > 0 )
                datalen = ram_map_bitstreams(1,ram,0,ram->blocks.M,&sha,ram->blocks.hps,ram->blocks.contiguous,fname,&refsha);
            printf("MApped.(%s) datalen.%d\n",fname,datalen);
        }
    }
#endif
    ram_disp_status(ram);
    //getchar();
}

uint32_t ram_update_disp(struct ramchain_info *ram)
{
    if ( ram_update_RTblock(ram) > ram->lastdisp )
    {
        ram->blocks.blocknum = ram->blocks.contiguous = ram_setcontiguous(&ram->blocks);
        ram_disp_status(ram);
        ram->lastdisp = ram_update_RTblock(ram);
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
            sleep(sqrt(1 << blocks->shift));
    }
}

void *ram_process_ramchain(void *_ram)
{
    struct ramchain_info *ram = _ram;
    int32_t pass;
    //init_ramchain(ram);
    ram->startmilli = ram_millis();
    for (pass=1; pass<=4; pass++)
    {
        if ( portable_thread_create((void *)ram_process_blocks_loop,ram->mappedblocks[pass]) == 0 )
            printf("ERROR _process_ramchain.%s\n",ram->name);
#ifdef RAM_GENMODE
        break;
#endif
    }
    while ( 1 )
    {
        ram_update_disp(ram);
        sleep(1);
    }
    return(0);
}

void *portable_thread_create(void *funcp,void *argp);
int Numramchains; struct ramchain_info *Ramchains[100];
void activate_ramchain(struct ramchain_info *ram,char *name)
{
    Ramchains[Numramchains++] = ram;
    printf("ram.%p Add ramchain.(%s) (%s) Num.%d\n",ram,ram->name,name,Numramchains);
}
           
void *process_ramchains(void *_argcoinstr)
{
    void ensure_SuperNET_dirs(char *backupdir);
    char *argcoinstr = (_argcoinstr != 0) ? ((char **)_argcoinstr)[0] : 0;
    int32_t modval,numinterleaves,threaded = 0;
    double startmilli;
    struct ramchain_info *ram;
    int32_t i,pass,processed = 0;
    while ( IS_LIBTEST != 7 && Finished_init == 0 )
        sleep(1);
    ensure_SuperNET_dirs("ramchains");
    startmilli = ram_millis();
    if ( _argcoinstr != 0 && ((long *)_argcoinstr)[1] != 0 && ((long *)_argcoinstr)[2] != 0 )
    {
        modval = (int32_t)((long *)_argcoinstr)[1];
        numinterleaves = (int32_t)((long *)_argcoinstr)[2];
        printf("modval.%d numinterleaves.%d\n",modval,numinterleaves);
    } else modval = 0, numinterleaves = 1;
    for (i=0; i<Numramchains; i++)
        if ( argcoinstr == 0 || strcmp(argcoinstr,Ramchains[i]->name) == 0 )
        {
            printf("i.%d of %d: argcoinstr.%p (%s)\n",i,Numramchains,argcoinstr,argcoinstr!=0?argcoinstr:"");
            ram_init_ramchain(Ramchains[i]);
            Ramchains[i]->startmilli = ram_millis();
        }
    printf("took %.1f seconds to init_ramchains %d coins argcoinstr.%p\n",(ram_millis() - startmilli)/1000.,Numramchains,argcoinstr);
    while ( processed >= 0 )
    {
        processed = 0;
        for (i=0; i<Numramchains; i++)
        {
            ram = Ramchains[i];
            if ( argcoinstr == 0 || strcmp(argcoinstr,ram->name) == 0 )
            {
                if ( threaded != 0 )
                {
                    printf("%d of %d: (%s) argcoinstr.%s\n",i,Numramchains,ram->name,argcoinstr!=0?argcoinstr:"ALL");
                    printf("call process_ramchain.(%s)\n",ram->name);
                    if ( portable_thread_create((void *)ram_process_ramchain,ram) == 0 )
                        printf("ERROR _process_ramchain.%s\n",ram->name);
                    processed--;
                }
                else
                {
                    for (pass=1; pass<=4; pass++)
                    {
                        processed += ram_process_blocks(ram,ram->mappedblocks[pass],ram->mappedblocks[pass-1],1000.);
                        ram_update_disp(ram);
#ifdef RAM_GENMODE
                        break;
#endif
                        if ( (ram->mappedblocks[pass]->blocknum >> ram->mappedblocks[pass]->shift) < (ram->mappedblocks[pass-1]->blocknum >> ram->mappedblocks[pass]->shift) )
                            break;
                   }
                }
            }
        }
        if ( processed == 0 )
        {
            for (i=0; i<Numramchains; i++)
            {
                ram = Ramchains[i];
                if ( argcoinstr == 0 || strcmp(argcoinstr,ram->name) == 0 )
                    ram_update_disp(ram);
            }
            sleep(1);
        }
    }
    printf("process_ramchains: finished launching\n");
    while ( 1 )
        sleep(60);
}

#endif
#endif
