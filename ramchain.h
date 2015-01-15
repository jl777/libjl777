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
// malloc, calloc, realloc, free, gettimeofday, strcpy, strncmp, strcmp, memcpy and other standard functions
//

#ifdef INCLUDE_DEFINES
#ifndef ramchain_h
#define ramchain_h

extern struct ramchain_info *get_ramchain_info(char *coinstr);
extern void *init_mappedptr(void **ptrp,struct mappedptr *mp,uint64_t allocsize,int32_t rwflag,char *fname);
extern void close_mappedptr(struct mappedptr *mp);
extern void calc_sha256cat(unsigned char hash[256 >> 3],unsigned char *src,int32_t len,unsigned char *src2,int32_t len2);
void delete_file(char *fname,int32_t scrubflag);

// ramchain functions for external access
void process_coinblocks(char *argcoinstr);

char *ramstatus(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin);
char *ramstring(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,char *typestr,uint32_t rawind);
char *ramrawind(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,char *typestr,char *str);
char *ramblock(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,uint32_t blocknum);
char *ramcompress(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,char *ramhex);
char *ramexpand(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,char *bitstream);
char *ramscript(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,char *txidstr,int32_t tx_vout,struct address_entry *bp);
char *ramtxlist(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,char *coinaddr,int32_t unspentflag);
char *ramrichlist(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,int32_t numwhales);
char *rambalances(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,char **coins,double *rates,char ***coinaddrs,int32_t numcoins);


#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <ctype.h>

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

#define HUFF_GENMODE

#ifdef HUFF_GENMODE
#define HUFF_NUMFREQS 1
#define HUFF_READONLY 0
#else
#define HUFF_READONLY 1
#define HUFF_NUMFREQS 1
#endif

#define SETBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] |= (1 << ((bitoffset) & 7)))
#define GETBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] & (1 << ((bitoffset) & 7)))
#define CLEARBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] &= ~(1 << ((bitoffset) & 7)))

struct huffstream { uint8_t *ptr,*buf,*allocptr; int32_t bitoffset,maski,endpos,allocsize; };
typedef struct huffstream HUFF;

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

#define MAX_HUFFBITS 5
struct huffbits { uint64_t numbits:MAX_HUFFBITS,rawind:(32-MAX_HUFFBITS),bits:32; };
struct huffpayload { struct address_entry B,spentB; uint64_t value; uint32_t tx_rawind,extra; };

struct huffitem
{
    struct huffbits code;
    uint32_t freq[HUFF_NUMFREQS];
};

struct huffcode
{
    double totalbits,totalbytes,freqsum;
    int32_t numitems,maxbits,numnodes,depth,allocsize;
    int32_t tree[];
};

union data_or_ptr { uint64_t data; void *ptr; };
struct huffpair { struct huffitem *items; struct huffcode *code; int32_t maxind,nonz,count,wt; char name[16]; };
struct huffpair_hash { UT_hash_handle hh; void *payloads; uint32_t rawind,numpayloads; };
struct huffhash { char coinstr[16]; struct huffpair_hash *table; struct mappedptr M; FILE *newfp; void **ptrs; uint32_t ind,numalloc; uint8_t type; };

struct rawtx_huffs { struct huffpair numvins,numvouts,txid; char numvinsname[32],numvoutsname[32],txidname[32]; };
struct rawvin_huffs { struct huffpair txid,vout; char txidname[32],voutname[32]; };
struct rawvout_huffs { struct huffpair addr,script,value; char addrname[32],scriptname[32],valuename[32]; };
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

/*#define HUFF_NUMGENS 2
struct compressionvars
{
    struct rawblock raw,raw2,raws[HUFF_NUMGENS];
    HUFF *hps[HUFF_NUMGENS]; uint8_t *rawbits[HUFF_NUMGENS];
    FILE *rawfp,*bitfps[HUFF_NUMGENS];
    struct huffhash hash[0x100];
    uint32_t maxitems,maxblocknum,firstblock,blocknum,processed,firstvout,firstvin,currentblocknum;
    char *disp,coinstr[64];
    double startmilli;
};*/

struct mappedblocks
{
    struct ramchain_info *ram;
    struct mappedblocks *prevblocks;
    struct rawblock *R,*R2;
    HUFF **hps,*tmphp;
    uint64_t *flags;
    struct mappedptr *M;
    double sum;
    uint32_t blocknum,count,firstblock,numblocks,processed,format,shift,contiguous;
};

struct ramchain_info
{
    struct mappedblocks blocks,Vblocks,Bblocks,blocks64,blocks4096,*mappedblocks[8];
    struct huffhash addrhash,txidhash,scripthash;
    double startmilli;
    HUFF *tmphp,*tmphp2;
    char name[64],dirpath[512],myipaddr[64],srvNXTACCTSECRET[2048],srvNXTADDR[64],*userpass,*serverport;
    uint32_t lastheighttime,RTblocknum,min_confirms,estblocktime,firstiter,maxblock;
    struct rawblock *R,*R2;
};

union ramtypes { double dval; uint64_t val64; float fval; uint32_t val32; uint16_t val16; uint8_t val8,hashdata[8]; };
struct ramchain_token
{
    uint32_t numbits:15,ishuffcode:1,offset:16;
    uint32_t rawind;
    char selector,type;
    union ramtypes U;
};

int32_t extract_txvals(char *coinaddr,char *script,int32_t nohexout,cJSON *txobj);
char *get_transaction(struct ramchain_info *ram,char *txidstr);
cJSON *_get_blocktxarray(uint32_t *blockidp,int32_t *numtxp,struct ramchain_info *ram,cJSON *blockjson);
cJSON *get_blockjson(uint32_t *heightp,struct ramchain_info *ram,char *blockhashstr,uint32_t blocknum);
char *get_blockhashstr(struct ramchain_info *ram,uint32_t blocknum);

#endif
#endif


#ifdef INCLUDE_CODE
#ifndef ramchain_code_h
#define ramchain_code_h

double ram_millis(void)
{
    static struct timeval timeval,first_timeval;
    gettimeofday(&timeval,0);
    if ( first_timeval.tv_sec == 0 )
    {
        first_timeval = timeval;
        return(0);
    }
    return((timeval.tv_sec - first_timeval.tv_sec) * 1000. + (timeval.tv_usec - first_timeval.tv_usec)/1000.);
}

int32_t _ram_get_blockoffset(struct mappedblocks *blocks,uint32_t blocknum)
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
    int32_t offset = _ram_get_blockoffset(blocks,blocknum);
    return((offset < 0) ? 0 : &blocks->hps[offset]);
}

struct mappedptr *ram_get_M(struct mappedblocks *blocks,uint32_t blocknum)
{
    int32_t offset = _ram_get_blockoffset(blocks,blocknum);
    return((offset < 0) ? 0 : &blocks->M[offset]);
}

int32_t ram_setbit(struct mappedblocks *blocks,uint32_t blocknum)
{
    int32_t offset = _ram_get_blockoffset(blocks,blocknum);
    return((offset < 0) ? 0 : SETBIT(blocks->flags,offset));
}

int32_t ram_getbit(struct mappedblocks *blocks,uint32_t blocknum)
{
    int32_t offset = _ram_get_blockoffset(blocks,blocknum);
    return((offset < 0) ? -1 : GETBIT(blocks->flags,offset));
}

static uint8_t huffmasks[8] = { (1<<0), (1<<1), (1<<2), (1<<3), (1<<4), (1<<5), (1<<6), (1<<7) };
static uint8_t huffoppomasks[8] = { ~(1<<0), ~(1<<1), ~(1<<2), ~(1<<3), ~(1<<4), ~(1<<5), ~(1<<6), (uint8_t)~(1<<7) };

void _hseek(HUFF *hp)
{
    hp->ptr = &hp->buf[hp->bitoffset >> 3];
    hp->maski = (hp->bitoffset & 7);
}

void hrewind(HUFF *hp)
{
    hp->bitoffset = 0;
    _hseek(hp);
}

void hclear(HUFF *hp)
{
    hp->bitoffset = 0;
    _hseek(hp);
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
    printf("hgetbit past EOF: %d >= %d\n",hp->bitoffset,hp->endpos);
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
    if ( (hp->bitoffset>>3) >= hp->allocsize )
    {
        printf("hwrite: bitoffset.%d >= allocsize.%d\n",hp->bitoffset,hp->allocsize);
        _hseek(hp);
        return(-1);
    }
    return(0);
}

int32_t _calc_bitsize(uint32_t x)
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

int32_t emit_bits(HUFF *hp,uint32_t val,int32_t numbits)
{
    int32_t hputbit(HUFF *hp,int32_t bit);
    int32_t i;
    for (i=0; i<numbits; i++)
        hputbit(hp,(val & (1<<i)) != 0);
    return(numbits);
}

int32_t decode_bits(uint32_t *valp,HUFF *hp,int32_t numbits)
{
    uint32_t i,val;
    for (i=val=0; i<numbits; i++)
        if ( hgetbit(hp) != 0 )
            val |= (1 << i);
    *valp = val;
    // printf("{%d} ",val);
    return(numbits);
}

int32_t emit_smallbits(HUFF *hp,uint16_t val)
{
    static long sum,count;
    int32_t numbits = 0;
    //printf("smallbits.(%d)\n",val);
    if ( val >= 4 )
    {
        if ( val < (1 << 5) )
        {
            numbits += emit_bits(hp,4,3);
            numbits += emit_bits(hp,val,5);
        }
        else if ( val < (1 << 8) )
        {
            numbits += emit_bits(hp,5,3);
            numbits += emit_bits(hp,val,8);
        }
        else if ( val < (1 << 12) )
        {
            numbits += emit_bits(hp,6,3);
            numbits += emit_bits(hp,val,12);
        }
        else
        {
            numbits += emit_bits(hp,7,3);
            numbits += emit_bits(hp,val,16);
        }
    } else numbits += emit_bits(hp,val,3);
    count++, sum += numbits;
    if ( (count % 100000) == 0 )
        printf("ave smallbits %.1f after %ld samples\n",(double)sum/count,count);
    return(numbits);
}

int32_t decode_smallbits(uint16_t *valp,HUFF *hp)
{
    uint32_t s;
    int32_t val,numbits = 3;
    val = (hgetbit(hp) | (hgetbit(hp) << 1) | (hgetbit(hp) << 2));
    if ( val < 4 )
        *valp = val;
    else
    {
        if ( val == 4 )
            numbits += decode_bits(&s,hp,5);
        else if ( val == 5 )
            numbits += decode_bits(&s,hp,8);
        else if ( val == 6 )
            numbits += decode_bits(&s,hp,12);
        else numbits += decode_bits(&s,hp,16);
        *valp = s;
    }
    return(numbits);
}

int32_t emit_varbits(HUFF *hp,uint32_t val)
{
    static long sum,count;
    int32_t numbits = 0;
    if ( val >= 10 )
    {
        if ( val < (1 << 8) )
        {
            numbits += emit_bits(hp,10,4);
            numbits += emit_bits(hp,val,8);
        }
        else if ( val < (1 << 10) )
        {
            numbits += emit_bits(hp,11,4);
            numbits += emit_bits(hp,val,10);
        }
        else if ( val < (1 << 13) )
        {
            numbits += emit_bits(hp,12,4);
            numbits += emit_bits(hp,val,13);
        }
        else if ( val < (1 << 16) )
        {
            numbits += emit_bits(hp,13,4);
            numbits += emit_bits(hp,val,16);
        }
        else if ( val < (1 << 20) )
        {
            numbits += emit_bits(hp,14,4);
            numbits += emit_bits(hp,val,20);
        }
        else
        {
            numbits += emit_bits(hp,15,4);
            numbits += emit_bits(hp,val,32);
        }
    } else numbits += emit_bits(hp,val,4);
    count++, sum += numbits;
    if ( (count % 100000) == 0 )
        printf("emit_varbits.(%u) numbits.%d ave varbits %.1f after %ld samples\n",val,numbits,(double)sum/count,count);
    return(numbits);
}

int32_t decode_varbits(uint32_t *valp,HUFF *hp)
{
    int32_t val;
    val = (hgetbit(hp) | (hgetbit(hp) << 1) | (hgetbit(hp) << 2) | (hgetbit(hp) << 3));
    if ( val < 10 )
    {
        *valp = val;
        return(4);
    }
    else if ( val == 10 )
        return(4 + decode_bits(valp,hp,8));
    else if ( val == 11 )
        return(4 + decode_bits(valp,hp,10));
    else if ( val == 12 )
        return(4 + decode_bits(valp,hp,13));
    else if ( val == 13 )
        return(4 + decode_bits(valp,hp,16));
    else if ( val == 14 )
        return(4 + decode_bits(valp,hp,20));
    else return(4 + decode_bits(valp,hp,32));
}

int32_t emit_valuebits(HUFF *hp,uint64_t value)
{
    static long sum,count;
    int32_t i,numbits = 0;
    for (i=0; i<2; i++,value>>=32)
        numbits += emit_varbits(hp,value & 0xffffffff);
    count++, sum += numbits;
    if ( (count % 100000) == 0 )
        printf("ave valuebits %.1f after %ld samples\n",(double)sum/count,count);
    return(numbits);
}

int32_t decode_valuebits(uint64_t *valuep,HUFF *hp)
{
    uint32_t i,tmp[2],numbits = 0;
    for (i=0; i<2; i++)
        numbits += decode_varbits(&tmp[i],hp);//, printf("($%x).%d ",tmp[i],i);
    *valuep = (tmp[0] | ((uint64_t)tmp[1] << 32));
    // printf("[%llx] ",(long long)(*valuep));
    return(numbits);
}

int32_t calc_varint(uint8_t *buf,uint64_t x)
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

long emit_varint(FILE *fp,uint64_t x)
{
    uint8_t buf[9];
    int32_t len;
    if ( fp == 0 )
        return(-1);
    long retval = -1;
    if ( (len= calc_varint(buf,x)) > 0 )
        retval = fwrite(buf,1,len,fp);
    return(retval);
}

long decode_varint(uint64_t *valp,uint8_t *ptr,long offset,long mappedsize)
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

long load_varint(uint64_t *valp,FILE *fp)
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

int32_t save_varfilestr(FILE *fp,char *str)
{
    long n,savepos,len = strlen(str) + 1;
    if ( fp == 0 )
        return(-1);
    savepos = ftell(fp);
    //printf("save.(%s) at %ld\n",str,ftell(fp));
    if ( (n= emit_varint(fp,len)) > 0 )
    {
        if ( fwrite(str,1,len,fp) != len )
            return(-1);
        fflush(fp);
        return((int32_t)(n + len));
    }
    else fseek(fp,savepos,SEEK_SET);
    return(-1);
}

long load_varfilestr(int32_t *lenp,char *str,FILE *fp,int32_t maxlen)
{
    int32_t retval;
    long savepos,fpos = 0;
    uint64_t len;
    *lenp = 0;
    if ( fp == 0 )
        return(-1);
    savepos = ftell(fp);
    if ( (retval= (int32_t)load_varint(&len,fp)) > 0 && len < maxlen )
    {
        fpos = ftell(fp);
        if ( len > 0 )
        {
            if ( fread(str,1,len,fp) != len )
            {
                printf("load_filestr: error reading len.%lld at %ld, truncate to %ld\n",(long long)len,ftell(fp),savepos);
                fseek(fp,savepos,SEEK_SET);
                return(-1);
            }
            else
            {
                //str[len] = 0;
                *lenp = (int32_t)len;
                //printf("fpos.%ld got string.(%s) len.%d\n",ftell(fp),str,(int)len);
                return(fpos);
            }
        } else return(fpos);
    } else printf("load_varint got %d at %ld: len.%lld maxlen.%d\n",retval,ftell(fp),(long long)len,maxlen);
    fseek(fp,savepos,SEEK_SET);
    return(-1);
}

// HUFF bitstream functions, uses model similar to fopen/fread/fwrite/fseek but for bitstream

void hclose(HUFF *hp)
{
    if ( hp != 0 )
    {
        if ( hp->allocptr != 0 )
            free(hp->allocptr);
        if ( hp != 0 )
            free(hp);
    }
}

void hpurge(HUFF *hps[],int32_t num)
{
    int32_t i;
    for (i=0; i<num; i++)
        if ( hps[i] != 0 )
            hclose(hps[i]), hps[i] = 0;
}

HUFF *hopen(uint8_t *bits,int32_t num,void *allocptr)
{
    HUFF *hp = calloc(1,sizeof(*hp));
    hp->ptr = hp->buf = bits;
    hp->allocsize = num;
    hp->endpos = (num << 3);
    hp->allocptr = allocptr;
    return(hp);
}

int32_t hseek(HUFF *hp,int32_t offset,int32_t mode)
{
    if ( mode == SEEK_END )
        offset += hp->endpos;
    if ( offset >= 0 && (offset>>3) <= hp->allocsize )
    {
        hp->bitoffset = offset, _hseek(hp);
        if ( hp->bitoffset > hp->endpos )
            hp->endpos = hp->bitoffset;
    }
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

int32_t conv_bitlen(uint64_t bitlen)
{
    int32_t len;
    len = (int32_t)(bitlen >> 3);
    if ( ((int32_t)bitlen & 7) != 0 )
        len++;
    return(len);
}

int32_t hflush(FILE *fp,HUFF *hp)
{
    long emit_varint(FILE *fp,uint64_t x);
    uint32_t len;
    int32_t numbytes = 0;
    if ( (numbytes= (int32_t)emit_varint(fp,hp->endpos)) < 0 )
        return(-1);
    len = conv_bitlen(hp->endpos);
    if ( fwrite(hp->buf,1,len,fp) != len )
        return(-1);
    fflush(fp);
    //printf("HFLUSH len.%d numbytes.%d | fpos.%ld\n",len,numbytes,ftell(fp));
    return(numbytes + len);
}

HUFF *hload(long *offsetp,FILE *fp,char *fname)
{
    long load_varint(uint64_t *x,FILE *fp);
    uint64_t endbitpos;
    int32_t len,flag = 0;
    uint8_t *buf;
    HUFF *hp = 0;
    if ( offsetp != 0 )
        *offsetp = -1;
    if ( fp == 0 )
        fp = fopen(fname,"rb"), flag = 1;
    if ( load_varint(&endbitpos,fp) > 0 )
    {
        len = conv_bitlen(endbitpos);
        buf = calloc(1,len);
        if ( offsetp != 0 )
            *offsetp = ftell(fp);
        if ( fread(buf,1,len,fp) != len )
            free(buf);
        else hp = hopen(buf,len,buf), hp->endpos = (int32_t)endbitpos;
        //fseek(fp,0,SEEK_END);
        //printf("HLOAD endbitpos.%d len.%d | fpos.%ld\n",(int)endbitpos,len,ftell(fp));
    }
    if ( flag != 0 && fp != 0 )
        fclose(fp);
    return(hp);
}

// misc display functions:

uint64_t _reversebits(uint64_t x,int32_t n)
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

int32_t huff_dispitem(int32_t dispflag,const struct huffitem *item,int32_t frequi)
{
    int32_t n = item->code.numbits;
    uint64_t codebits;
    if ( n != 0 )
    {
        if ( dispflag != 0 )
        {
            codebits = _reversebits(item->code.bits,n);
            printf("%06d: freq.%-6d (%8s).%d\n",item->code.rawind,item->freq[frequi],huff_str(codebits,n),n);
        }
        return(1);
    }
    return(0);
}

void huff_disp(int32_t verboseflag,struct huffcode *huff,struct huffitem *items,int32_t frequi)
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
struct huffheap *_heap_create(uint32_t s,uint32_t *f)
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

void _heap_destroy(struct huffheap *heap)
{
    free(heap->h);
    free(heap);
}

#define swap_(I,J) do { int t_; t_ = a[(I)];	\
a[(I)] = a[(J)]; a[(J)] = t_; } while(0)
void _heap_sort(struct huffheap *heap)
{
    uint32_t i=1,j=2; // gnome sort
    uint32_t *a = heap->h;
    while ( i < heap->n ) // smaller values are kept at the end
    {
        if ( heap->f[a[i-1]] >= heap->f[a[i]] )
            i = j, j++;
        else
        {
            swap_(i-1, i);
            i--;
            i = (i == 0) ? j++ : i;
        }
    }
}
#undef swap_

void _heap_add(struct huffheap *heap,uint32_t ind)
{
    //printf("add to heap ind.%d n.%d s.%d\n",ind,heap->n,heap->s);
    if ( (heap->n + 1) > heap->s )
    {
        heap->h = realloc(heap->h,heap->s + heap->cs);
        heap->s += heap->cs;
    }
    heap->h[heap->n++] = ind;
    _heap_sort(heap);
}

int32_t _heap_remove(struct huffheap *heap)
{
    if ( heap->n > 0 )
        return(heap->h[--heap->n]);
    return(-1);
}

int32_t huff_init(double *freqsump,int32_t **predsp,uint32_t **revtreep,struct huffitem **items,int32_t numitems,int32_t frequi)
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
        efreqs[i] = items[i]->freq[frequi];// * ((items[i]->wt != 0) ? items[i]->wt : 1);
    if ( (heap= _heap_create(numitems*2,efreqs)) == NULL )
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
            freqsum += items[i]->freq[frequi];
            _heap_add(heap,i);
        }
    //printf("added n.%d items: freqsum %.0f\n",n,freqsum);
    *freqsump = freqsum;
    depth = 0;
    while ( heap->n > 1 )
    {
        r1 = _heap_remove(heap);
        r2 = _heap_remove(heap);
        efreqs[extf] = (efreqs[r1] + efreqs[r2]);
        revtree[(depth << 1) + 1] = r1;
        revtree[(depth << 1) + 0] = r2;
        depth++;
        _heap_add(heap,extf);
        preds[r1] = extf;
        preds[r2] = -extf;
        //printf("n.%d: r1.%d (%d) <- %d | r2.%d (%d) <- %d\n",heap->n,r1,efreqs[r1],extf,r2,efreqs[r2],-extf);
        extf++;
    }
    r1 = _heap_remove(heap);
    preds[r1] = r1;
    _heap_destroy(heap);
    free(efreqs);
    *predsp = preds;
    *revtreep = revtree;
    return(extf);
}

int32_t huff_calc(int32_t dispflag,int32_t *nonzp,double *totalbytesp,double *totalbitsp,int32_t *preds,struct huffitem **items,int32_t numitems,int32_t frequi,int32_t wt)
{
    int32_t i,ix,pred,numbits,nonz,maxbits = 0;
    double totalbits,totalbytes;
    struct huffitem *item;
    uint64_t codebits;
    totalbits = totalbytes = 0.;
    for (i=nonz=0; i<numitems; i++)
    {
        codebits = numbits = 0;
        if ( items[i]->freq[frequi] != 0 )
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
                //item->codebits = _reversebits(codebits,numbits);
                //item->numbits = numbits;
                item->code.numbits = numbits;
                item->code.bits = _reversebits(codebits,numbits);
                //item->code.rawind = item->rawind;
                if ( numbits > 32 || item->code.numbits != numbits || item->code.bits != _reversebits(codebits,numbits) || item->code.rawind != item->code.rawind )
                    printf("error setting code.bits: item->code.numbits %d != %d numbits || item->code.bits %u != %llu _reversebits(codebits,numbits) || item->code.rawind %d != %d item->rawind\n",item->code.numbits,numbits,item->code.bits,(long long)_reversebits(codebits,numbits),item->code.rawind,item->code.rawind);
                totalbits += numbits * item->freq[frequi];
                totalbytes += wt * item->freq[frequi];
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
int32_t decodetest(uint32_t *indp,int32_t *tree,int32_t depth,int32_t val,int32_t numitems)
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

struct huffcode *huff_create(int32_t dispflag,struct huffitem **items,int32_t numitems,int32_t frequi,int32_t wt)
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
        n = decodetest(&itemi,tree,depth,i,numitems);
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

int32_t expand_scriptdata(char *scriptstr,uint8_t *scriptdata,int32_t datalen)
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

int32_t calc_scriptmode(int32_t *datalenp,uint8_t scriptdata[4096],char *script,int32_t trimflag)
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

int32_t load_rawvout(int32_t dispflag,FILE *fp,struct rawvout *vout)
{
    long fpos,n;
    uint64_t varint,vlen;
    int32_t datalen,retval;
    uint8_t data[4096];
    if ( (n= load_varint(&varint,fp)) <= 0 )
        return(-1);
    else
    {
        //dispflag = 1;
        if ( dispflag != 0 )
            printf("loaded varint %lld with %ld bytes | fpos.%ld\n",(long long)varint,n,ftell(fp));
        vout->value = varint;
        if ( (fpos= load_varfilestr(&datalen,(char *)data,fp,sizeof(vout->script)/2-1)) > 0 )
        {
            expand_scriptdata(vout->script,data,datalen);
            //printf(">>>>>>>>>>> loadscript.(%s).(%c) <<<<<<<<<<<<<<\n",vout->script,data[0]);
            retval = (int32_t)load_varint(&vlen,fp);
            if ( dispflag != 0 )
                printf("script.(%s) datalen.%d | retval.%d vlen.%llu | fpos.%ld\n",vout->script,datalen,retval,(long long)vlen,ftell(fp));
            vout->coinaddr[0] = 0;
            //if ( vlen == 0 )
            //    return(0); vlen > 0 &&
            if (  retval > 0 && vlen < sizeof(vout->coinaddr)-1 && (n= fread(vout->coinaddr,1,vlen+1,fp)) == (vlen+1) )
            {
                if ( dispflag != 0 )
                    printf("coinaddr.(%s) len.%d fpos.%ld\n",vout->coinaddr,(int)vlen,ftell(fp));
                return(0);
            } else printf("read coinaddr.%d (%s) returns %ld fpos.%ld\n",(int)vlen,vout->coinaddr,n,ftell(fp));
        } else return(-2);
    }
    return(-3);
}

int32_t save_rawvout(int32_t dispflag,FILE *fp,struct rawvout *vout)
{
    long len;
    int32_t mode,datalen = 0;
    uint8_t data[4096];
    //printf("SAVE.(%s %s %.8f)\n",vout->coinaddr,vout->script,dstr(vout->value));
    if ( emit_varint(fp,vout->value) <= 0 )
        return(-1);
    else
    {
        len = strlen(vout->coinaddr);
        if ( vout->script[0] != 0 )
        {
            mode = calc_scriptmode(&datalen,data,vout->script,1);
            if ( emit_varint(fp,datalen) <= 0 )
                return(-2);
            else if ( datalen > 0 && fwrite(data,1,datalen,fp) != datalen )
                return(-3);
        }
        else if ( emit_varint(fp,0) <= 0 )
            return(-4);
        if ( emit_varint(fp,len) <= 0 )
            return(-5);
        else if ( len >= 0 && fwrite(vout->coinaddr,1,len+1,fp) != len+1 )
            return(-6);
        else if ( len < 0 )
        {
            printf("script.(%s).%d wrote (%s).%ld %.8f\n",vout->script,datalen,vout->coinaddr,len,dstr(vout->value));
            //while ( 1 ) sleep(1);
            return(-7);
        }
    }
    return(0);
}

int32_t load_rawvin(int32_t dispflag,FILE *fp,struct rawvin *vin)
{
    long fpos;
    uint64_t varint;
    int32_t datalen;
    uint8_t data[4096];
    if ( load_varint(&varint,fp) <= 0 )
        return(-1);
    else
    {
        vin->vout = (uint32_t)varint;
        if ( (fpos= load_varfilestr(&datalen,(char *)data,fp,sizeof(vin->txidstr)-1)) > 0 )
        {
            init_hexbytes_noT(vin->txidstr,data,datalen);
            if ( dispflag != 0 )
                printf("(%s).%d ",vin->txidstr,vin->vout);
            return(0);
        }
    }
    return(-1);
}

int32_t save_rawvin(int32_t dispflag,FILE *fp,struct rawvin *vin)
{
    long len;
    uint8_t data[512];
    //printf("SAVEVIN.(%s %d)\n",vin->txidstr,vin->vout);
    if ( emit_varint(fp,vin->vout) <= 0 )
        return(-1);
    else
    {
        len = strlen(vin->txidstr) >> 1;
        if ( len <= 0 )
            return(-1);
        decode_hex(data,(int32_t)len,vin->txidstr);
        if ( emit_varint(fp,len) <= 0 )
            return(-1);
        else if ( len > 0 && fwrite(data,1,len,fp) != len )
            return(-1);
    }
    return(0);
}

int32_t load_rawtx(int32_t dispflag,FILE *fp,struct rawtx *tx)
{
    long fpos;
    uint64_t varint,varint2;
    int32_t datalen;
    uint8_t data[4096];
    if ( load_varint(&varint,fp) <= 0 || load_varint(&varint2,fp) <= 0 )
        return(-1);
    else
    {
        tx->numvins = (uint32_t)varint;
        tx->numvouts = (uint32_t)varint2;
        if ( (fpos= load_varfilestr(&datalen,(char *)data,fp,sizeof(tx->txidstr)-1)) > 0 )
        {
            init_hexbytes_noT(tx->txidstr,data,datalen);
            if ( dispflag != 0 )
                printf("%d.(%s).%d ",tx->numvins,tx->txidstr,tx->numvouts);
            return(0);
        }
    }
    return(-1);
}

int32_t save_rawtx(int32_t dispflag,FILE *fp,struct rawtx *tx)
{
    long len;
    uint8_t data[512];
    if ( emit_varint(fp,tx->numvins) <= 0 || emit_varint(fp,tx->numvouts) <= 0 )
        return(-1);
    else
    {
        len = strlen(tx->txidstr) >> 1;
        if ( len < 0 )
            return(-1);
        decode_hex(data,(int32_t)len,tx->txidstr);
        if ( emit_varint(fp,len) <= 0 )
            return(-1);
        else if ( len > 0 && fwrite(data,1,len,fp) != len )
            return(-1);
    }
    return(0);
}

int32_t vinspace_io(int32_t dispflag,int32_t saveflag,FILE *fp,struct rawvin *vins,int32_t numrawvins)
{
    int32_t i;
    if ( numrawvins > 0 )
    {
        //printf("vinspace numrawvins.%d\n",numrawvins);
        for (i=0; i<numrawvins; i++)
            if ( ((saveflag != 0) ? save_rawvin(dispflag,fp,&vins[i]) : load_rawvin(dispflag,fp,&vins[i])) != 0 )
                return(-1);
    }
    return(0);
}

int32_t voutspace_io(int32_t dispflag,int32_t saveflag,FILE *fp,struct rawvout *vouts,int32_t numrawvouts)
{
    int32_t i,retval;
    if ( numrawvouts > 0 )
    {
        //printf("voutspace numrawvouts.%d\n",numrawvouts);
        for (i=0; i<numrawvouts; i++)
            if ( (retval= ((saveflag != 0) ? save_rawvout(dispflag,fp,&vouts[i]) : load_rawvout(dispflag,fp,&vouts[i]))) != 0 )
            {
                printf("rawvout.saveflag.%d returns %d\n",saveflag,retval);
                return(-1);
            }
    }
    return(0);
}

int32_t txspace_io(int32_t dispflag,int32_t saveflag,FILE *fp,struct rawtx *tx,int32_t numtx)
{
    int32_t i,retval;
    if ( numtx > 0 )
    {
       // printf("txspace numtx.%d\n",numtx);
        for (i=0; i<numtx; i++)
            if ( (retval= ((saveflag != 0) ? save_rawtx(dispflag,fp,&tx[i]) : load_rawtx(dispflag,fp,&tx[i]))) != 0 )
            {
                printf("rawtx.saveflag.%d returns %d\n",saveflag,retval);
                return(-1);
            }
    }
    return(0);
}

int32_t rawblockcmp(struct rawblock *ref,struct rawblock *raw)
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
            printf("%d of %d: (%s) (%s) %.8f vs (%s) (%s) %.8f\n",i,ref->numrawvouts,reftx->coinaddr,reftx->script,dstr(reftx->value),rawtx->coinaddr,rawtx->script,dstr(rawtx->value));
            if ( reftx->value != rawtx->value || strcmp(reftx->coinaddr,rawtx->coinaddr) != 0 || strcmp(reftx->script,rawtx->script) != 0 )
                err++;
        }
        if ( err != 0 )
        {
            while ( 1 )
                sleep(1);
            return(-6);
        }
    }
    return(0);
}

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

int32_t huffpair_decodeitemind(int32_t type,int32_t bitflag,uint32_t *rawindp,struct huffpair *pair,HUFF *hp)
{
    uint16_t s;
    uint32_t ind;
    int32_t i,c,*tree,tmp,numitems,numbits,depth,num = 0;
    *rawindp = 0;
    if ( bitflag == 0 )
    {
        if ( type == 2 )
        {
            numbits = decode_smallbits(&s,hp);
            *rawindp = s;
            return(numbits);
        }
        return(decode_varbits(rawindp,hp));
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
            *rawindp = pair->items[ind].code.rawind;
            return(num);
        }
        if ( c < 0 )
            break;
	}
    printf("(%s) huffpair_decodeitemind error num.%d\n",pair->name,num);
    return(-1);
}

void huffhash_setfname(char fname[1024],struct huffhash *hash,int32_t newflag)
{
    char str[128],typestr[128];
    switch ( hash->type )
    {
        case 'a': strcpy(str,"addr"); break;
        case 't': strcpy(str,"txid"); break;
        case 's': strcpy(str,"script"); break;
        default: sprintf(str,"type%d",hash->type); break;
    }
    if ( newflag != 0 )
        sprintf(typestr,"%s.new",str);
    else strcpy(typestr,str);
    set_compressionvars_fname(0,fname,hash->coinstr,typestr,-1);
}

int32_t huffhash_add(struct huffhash *hash,struct huffpair_hash *hp,void *ptr,int32_t datalen)
{
    hp->rawind = ++hash->ind;
    HASH_ADD_KEYPTR(hh,hash->table,ptr,datalen,hp);
    if ( 0 )
    {
        char hexbytes[8192];
        struct huffpair_hash *checkhp;
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
    hash->ptrs[hp->rawind] = hp->hh.key;
    return(0);
}

struct huffhash *ram_gethash(struct ramchain_info *ram,int32_t type)
{
    switch ( type )
    {
        case 'a': return(&ram->addrhash); break;
        case 't': return(&ram->txidhash); break;
        case 's': return(&ram->scripthash); break;
    }
    return(0);
}

char **ram_getalladdrs(int32_t *numaddrsp,struct ramchain_info *ram)
{
    char **addrs = 0;
    uint64_t varint;
    int32_t i = 0;
    struct huffhash *hash;
    struct huffpair_hash *hp,*tmp;
    hash = ram_gethash(ram,'a');
    *numaddrsp = HASH_COUNT(hash->table);
    addrs = calloc(*numaddrsp,sizeof(*addrs));
    HASH_ITER(hh,hash->table,hp,tmp)
    addrs[i++] = (char *)((long)hp->hh.key + decode_varint(&varint,hp->hh.key,0,9));
    if ( i != *numaddrsp )
        printf("ram_getalladdrs HASH_COUNT.%d vs i.%d\n",*numaddrsp,i);
    return(addrs);
}

int32_t huff_itemsave(FILE *fp,struct huffitem *item)
{
    if ( fwrite(&item->code,1,sizeof(item->code),fp) != sizeof(uint64_t) )
        return(-1);
    return((int32_t)sizeof(uint64_t));
}

int32_t huffcode_save(FILE *fp,struct huffcode *code)
{
    if ( code != 0 )
    {
        if ( fwrite(code,1,code->allocsize,fp) != code->allocsize )
            return(-1);
        return(code->allocsize);
    }
    return(-1);
}

int32_t huffpair_save(FILE *fp,struct huffpair *pair)
{
    int32_t i;
    long val,n = 0;
    if ( fwrite(pair,1,sizeof(*pair),fp) != sizeof(*pair) )
        return(-1);
    n = sizeof(*pair);
    for (i=0; i<=pair->maxind; i++)
    {
        if ( pair->items[i].freq[0] != 0 )
        {
            if ( (val= huff_itemsave(fp,&pair->items[i])) <= 0 )
                return(-1);
            n += val;
        }
    }
    if ( (val= huffcode_save(fp,pair->code)) <= 0 )
        return(-1);
    return((int32_t)(n+val));
}

int32_t huffpair_gencode(struct ramchain_info *V,struct huffpair *pair,int32_t frequi)
{
    struct huffitem **items;
    int32_t i,n;
    char fname[1024];
    FILE *fp;
    if ( pair->code == 0 && pair->nonz > 0 )
    {
        items = calloc(pair->nonz+1,sizeof(*items));
        for (i=n=0; i<=pair->maxind; i++)
            if ( pair->items[i].freq[frequi] != 0 )
                items[n++] = &pair->items[i];
        fprintf(stderr,"%s n.%d: ",pair->name,n);
        pair->code = huff_create(0,items,n,frequi,pair->wt);
        set_compressionvars_fname(0,fname,V->name,pair->name,-1);
        if ( pair->code != 0 && (fp= fopen(fname,"wb")) != 0 )
        {
            if ( huffpair_save(fp,pair) <= 0 )
                printf("huffpair_gencode: error saving %s\n",fname);
            else fprintf(stderr,"%s saved ",fname);
            fclose(fp);
        } else printf("error creating (%s)\n",fname);
        huff_disp(0,pair->code,pair->items,frequi);
        free(items);
    }
    return(0);
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
    if ( huffpair_gencode(V,&H->numall,frequi) != 0 )
        return(-1);
    else if ( huffpair_gencode(V,&H->numtx,frequi) != 0 )
        return(-2);
    else if ( huffpair_gencode(V,&H->numrawvins,frequi) != 0 )
        return(-3);
    else if ( huffpair_gencode(V,&H->numrawvouts,frequi) != 0 )
        return(-4);
    else if ( (err= huffpair_gentxcode(V,&H->txall,frequi)) != 0 )
        return(-100 + err);
    else if ( (err= huffpair_gentxcode(V,&H->tx0,frequi)) != 0 )
        return(-110 + err);
    else if ( (err= huffpair_gentxcode(V,&H->tx1,frequi)) != 0 )
        return(-120 + err);
    else if ( (err= huffpair_gentxcode(V,&H->txi,frequi)) != 0 )
        return(-130 + err);
    else if ( (err= huffpair_genvincode(V,&H->vinall,frequi)) != 0 )
        return(-200 + err);
    else if ( (err= huffpair_genvincode(V,&H->vin0,frequi)) != 0 )
        return(-210 + err);
    else if ( (err= huffpair_genvincode(V,&H->vin1,frequi)) != 0 )
        return(-220 + err);
    else if ( (err= huffpair_genvincode(V,&H->vini,frequi)) != 0 )
        return(-230 + err);
    else if ( (err= huffpair_genvoutcode(V,&H->voutall,frequi)) != 0 )
        return(-300 + err);
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

int32_t set_rawblock_preds(struct rawblock_preds *preds,struct rawblock_huffs *H,int32_t allmode)
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

int32_t huffpair_conv(char *strbuf,uint64_t *valp,struct ramchain_info *V,char type,uint32_t rawind);
uint32_t expand_huffint(int32_t bitflag,struct ramchain_info *V,struct huffpair *pair,HUFF *hp)
{
    uint32_t rawind; int32_t numbits;
    if ( (numbits= huffpair_decodeitemind(2,bitflag,&rawind,pair,hp)) < 0 )
        return(-1);
   // printf("%s.%d ",pair->name,rawind);
    return(rawind);
}

uint32_t expand_huffstr(int32_t bitflag,char *deststr,int32_t type,struct ramchain_info *V,struct huffpair *pair,HUFF *hp)
{
    uint64_t destval;
    uint32_t rawind;
    int32_t numbits;
    deststr[0] = 0;
    if ( (numbits= huffpair_decodeitemind(0,bitflag,&rawind,pair,hp)) < 0 )
        return(-1);
    //fprintf(stderr,"expand_huffstr: bitflag.%d type.%d %s rawind.%d\n",bitflag,type,pair->name,rawind);
    if ( rawind == 0 )
        return(0);
    // map rawind
    //huffpair_conv(deststr,&destval,V,type,rawind);
    return((uint32_t)destval);
}

void clear_rawblock(struct rawblock *raw)
{
    memset(raw,0,sizeof(*raw));
}

uint32_t expand_rawbits(int32_t bitflag,struct ramchain_info *V,struct rawblock *raw,HUFF *hp,struct rawblock_preds *preds)
{
    struct huffpair *pair,*pair2,*pair3;
    int32_t txind,numrawvins,numrawvouts,vin,vout;
    struct rawtx *tx; struct rawvin *vi; struct rawvout *vo;
    clear_rawblock(raw);
    hrewind(hp);
    raw->numtx = expand_huffint(bitflag,V,preds->numtx,hp);
    raw->numrawvins = expand_huffint(bitflag,V,preds->numrawvins,hp);
    raw->numrawvouts = expand_huffint(bitflag,V,preds->numrawvouts,hp);
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
            tx->numvins = expand_huffint(bitflag,V,pair,hp), numrawvins += tx->numvins;
            tx->numvouts = expand_huffint(bitflag,V,pair2,hp), numrawvouts += tx->numvouts;
           // printf("tx->numvins %d, numvouts.%d raw (%d %d) numtx.%d\n",tx->numvins,tx->numvouts,raw->numrawvins,raw->numrawvouts,raw->numtx);
            expand_huffstr(bitflag,tx->txidstr,'t',V,pair3,hp);
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
                        expand_huffstr(bitflag,vi->txidstr,'t',V,pair,hp);
                        vi->vout = expand_huffint(bitflag,V,pair2,hp);
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
                        expand_huffstr(bitflag,vo->script,'s',V,pair2,hp);
                        expand_huffstr(bitflag,vo->coinaddr,'a',V,pair,hp);
                        decode_valuebits(&vo->value,hp);
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

#define _decode_rawind conv_to_rawind
uint32_t conv_to_rawind(int32_t type,uint32_t val)
{
    uint16_t s;
    s = val;
    if ( s != val )
    {
        printf("conv_to_rawind: type.%d (%c) val.%d overflow for 16 bits\n",type,type,val);
        exit(-1);
    }
    return(s);
}

uint8_t *encode_hashstr(int32_t *datalenp,uint8_t *data,int32_t type,char *hashstr)
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
        if ( (scriptmode = calc_scriptmode(&datalen,&data[9],buf,1)) < 0 )
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
        varlen = calc_varint(varbuf,datalen);
        memcpy(&data[9-varlen],varbuf,varlen);
        //HASH_FIND(hh,hash->table,&ptr[-varlen],datalen+varlen,hp);
        *datalenp = (datalen + varlen);
        return(&data[9-varlen]);
    }
    return(0);
}

char *decode_hashdata(char *strbuf,int32_t type,uint8_t *hashdata)
{
    uint64_t varint;
    int32_t datalen,scriptmode;
    if ( hashdata == 0 )
        return(0);
    hashdata += decode_varint(&varint,hashdata,0,9);
    datalen = (int32_t)varint;
    if ( type == 's' )
    {
        if ( (scriptmode= expand_scriptdata(strbuf,hashdata,(uint32_t)datalen)) < 0 )
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

void *update_coinaddr_unspent(int32_t iter,uint32_t blocknum,uint32_t *nump,struct huffpayload *payloads,struct huffpayload *payload)
{
    int32_t i,num = *nump;
    struct address_entry *bp = &payload->B;
    if ( num > 0 )
    {
        for (i=0; i<num; i++)
        {
            if ( memcmp(&payloads[i].B,bp,sizeof(*bp)) == 0 )
            {
                if ( iter == 0 )
                    printf("block.%d duplicate unspent (%d %d %d) script.%d tx_rawind.%d %.8f\n",blocknum,bp->blocknum,bp->txind,bp->v,payload->extra,payload->tx_rawind,dstr(payload->value));
                return(payloads);
            }
            else if ( payloads[i].B.blocknum == bp->blocknum && payloads[i].B.txind == bp->txind && payloads[i].B.v == bp->v )
            {
                payloads[i].spentB = payload->spentB;
                printf("block.%d SPENT (%d %d %d) script.%d tx_rawind.%d %.8f -> (%d %d %d)\n",blocknum,bp->blocknum,bp->txind,bp->v,payload->extra,payload->tx_rawind,dstr(payload->value),payload->spentB.blocknum,payload->spentB.txind,payload->spentB.v);
                return(payloads);
            }
        }
    } else i = 0;
    if ( i == num )
    {
        (*nump) = ++num;
        payloads = realloc(payloads,sizeof(*payloads) * num);
        printf("block.%d new UNSPENT (%d %d %d) script.%d tx_rawind.%d %.8f\n",blocknum,bp->blocknum,bp->txind,bp->v,payload->extra,payload->tx_rawind,dstr(payload->value));
    //printf("blocknum.%d <<<<<<<<<<<<<< unspent (%d %d %d) numvouts.%d tx_rawind.%d %.8f\n",blocknum,bp->blocknum,bp->txind,bp->v,payload->vout,payload->rawind,dstr(payload->value));
        payloads[i] = *payload;
    }
    return(payloads);
}

int32_t update_payload(int32_t iter,struct address_entry *bps,int32_t slot,struct address_entry *bp)
{
    static struct address_entry zero;
    if ( bps != 0 )
    {
        if ( memcmp(&bps[slot],&zero,sizeof(*bp)) == 0 )
        {
            bps[slot] = *bp;
            return(0);
        }
        else if ( memcmp(&bps[slot],bp,sizeof(*bp)) == 0 )
        {
            if ( iter == 0 )
                printf(" duplicate spent.%d slot.%d (%d %d %d)\n",bp->spent,slot,bp->blocknum,bp->txind,bp->v);
        }
        else printf("ERROR duplicate spent.%d slot.%d txid (%d %d %d) vs (%d %d %d)\n",bp->spent,slot,bps[slot].blocknum,bps[slot].txind,bps[slot].v,bp->blocknum,bp->txind,bp->v);
    } else printf("null bps for slot.%d (%d %d %d)\n",slot,bp->blocknum,bp->txind,bp->v);
    return(-1);
}

void *update_txid_payload(int32_t iter,uint32_t *nump,struct address_entry *bps,struct huffpayload *payload)
{
    int32_t num = *nump;
    struct address_entry *bp = &payload->B;
    if ( num > 0 )
    {
        if ( bp->spent == 0 )
        {
            update_payload(iter,bps,0,bp);
            return(bps);
        }
        if ( payload->extra <= bps[0].v ) // maintain array of spends
            update_payload(iter,bps,payload->extra+1,bp);
        else printf("update_txid_payload: extra.%d >= bps[0].v %d\n",payload->extra,bps[0].v);
    }
    else
    {
        if ( bp->spent == 0 )
        {
            if ( bps == 0 )
            {
                (*nump) = (payload->extra + 1);
                bps = calloc(payload->extra+1,sizeof(*bps));
            }
            update_payload(iter,bps,0,bp); // first slot is (block, txind, numvouts) of txid
        }
        else printf("unexpected spend for extra.%d (%d %d %d) before the unspent\n",payload->extra,bp->blocknum,bp->txind,bp->v);
    }
    return(bps);
}

void *ram_gethashdata(struct ramchain_info *ram,int32_t type,uint32_t rawind)
{
    struct huffhash *hash;
    // slice into 1/256'ths so each slice can be compressed independently
    if ( (hash= ram_gethash(ram,type)) != 0 && hash->ptrs != 0 && rawind > 0 && rawind <= hash->ind )
        return(hash->ptrs[rawind]);
    return(0);
}

struct huffpair_hash *ram_hashdata_search(struct huffhash *hash,uint8_t *hashdata,int32_t datalen)
{
    char fname[512];
    void *newptr;
    struct huffpair_hash *hp = 0;
    if ( hash != 0 )
    {
        HASH_FIND(hh,hash->table,hashdata,datalen,hp);
        if ( hp == 0 )
        {
            if ( hash->newfp == 0 )
            {
                huffhash_setfname(fname,hash,0);
                hash->newfp = fopen(fname,"wb");
                printf("OPENED.(%s)\n",fname);
            }
            hp = calloc(1,sizeof(*hp));
            newptr = malloc(datalen);
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
            huffhash_add(hash,hp,newptr,datalen);
        } //else printf("found %d bytes ind.%d\n",datalen,hp->rawind);
    } else printf("ram_hashdata_search null hashtable\n");
    return(hp);
}

struct huffpair_hash *ram_hashsearch(struct huffhash *hash,char *hashstr,int32_t type)
{
    uint8_t data[4097],*hashdata;
    struct huffpair_hash *hp = 0;
    int32_t datalen;
    if ( hash != 0 && (hashdata= encode_hashstr(&datalen,data,type,hashstr)) != 0 )
        hp = ram_hashdata_search(hash,hashdata,datalen);
    return(hp);
}

uint32_t ram_conv_hashstr(struct ramchain_info *ram,char *hashstr,int32_t type)
{
    char nullstr[6] = { 5, 'n', 'u', 'l', 'l', 0 };
    struct huffpair_hash *hp = 0;
    struct huffhash *hash;
    if ( hashstr == 0 || hashstr[0] == 0 )
        hashstr = nullstr;
    hash = ram_gethash(ram,type);
    if ( (hp= ram_hashsearch(hash,hashstr,type)) != 0 )
        return(hp->rawind);
    else return(0xffffffff);
}

#define ram_scriptind(ram,hashstr) ram_conv_hashstr(ram,hashstr,'s')
#define ram_addrind(ram,hashstr) ram_conv_hashstr(ram,hashstr,'a')
#define ram_txidind(ram,hashstr) ram_conv_hashstr(ram,hashstr,'t')

#define ram_conv_rawind(hashstr,ram,rawind,type) decode_hashdata(hashstr,type,ram_gethashdata(ram,type,rawind))
#define ram_txid(hashstr,ram,rawind) ram_conv_rawind(hashstr,ram,rawind,'t')
#define ram_addr(hashstr,ram,rawind) ram_conv_rawind(hashstr,ram,rawind,'a')
#define ram_script(hashstr,ram,rawind) ram_conv_rawind(hashstr,ram,rawind,'s')

void *ramchain_payloads(int32_t *numpayloadsp,struct ramchain_info *ram,char *hashstr,int32_t type)
{
    struct huffpair_hash *hp = 0;
    *numpayloadsp = 0;
    if ( (hp= ram_hashsearch(ram_gethash(ram,type),hashstr,type)) != 0 )
    {
        *numpayloadsp = hp->numpayloads;
        return(hp->payloads);
    }
    else return(0);
}

#define ramchain_addr_payloads(numpayloadsp,ram,addr) ramchain_payloads(numpayloadsp,ram,addr,'a')
#define ramchain_txid_address_entries(numpayloadsp,ram,txidstr) ramchain_payloads(numpayloadsp,ram,txidstr,'s')

struct address_entry *ram_address_entry(struct address_entry *destbp,struct ramchain_info *ram,char *txidstr,int32_t vout)
{
    int32_t numpayloads;
    struct address_entry *bps;
    if ( (bps= ramchain_txid_address_entries(&numpayloads,ram,txidstr)) != 0 )
    {
        if ( vout < bps[0].v )
        {
            *destbp = bps[vout+1];
            return(destbp);
        }
    }
    return(0);
}

uint32_t get_RTheight(struct ramchain_info *ram)
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

//// interface code
#define OP_HASH160_OPCODE 0xa9
#define OP_EQUAL_OPCODE 0x87
#define OP_DUP_OPCODE 0x76
#define OP_EQUALVERIFY_OPCODE 0x88
#define OP_CHECKSIG_OPCODE 0xac
#define OP_RETURN_OPCODE 0x6a

char _hexbyte(int32_t c)
{
    c &= 0xf;
    if ( c < 10 )
        return('0'+c);
    else if ( c < 16 )
        return('a'+c-10);
    else return(0);
}

int32_t add_opcode(char *hex,int32_t offset,int32_t opcode)
{
    hex[offset + 0] = _hexbyte((opcode >> 4) & 0xf);
    hex[offset + 1] = _hexbyte(opcode & 0xf);
    return(offset+2);
}

void calc_script(char *script,char *pubkey,int msigmode)
{
    int32_t offset,len;
    offset = 0;
    len = (int32_t)strlen(pubkey);
    offset = add_opcode(script,offset,OP_DUP_OPCODE);
    offset = add_opcode(script,offset,OP_HASH160_OPCODE);
    offset = add_opcode(script,offset,len/2);
    memcpy(script+offset,pubkey,len), offset += len;
    offset = add_opcode(script,offset,OP_EQUALVERIFY_OPCODE);
    offset = add_opcode(script,offset,OP_CHECKSIG_OPCODE);
    script[offset] = 0;
}

int32_t origconvert_to_bitcoinhex(char *scriptasm)
{
    //"asm" : "OP_HASH160 db7f9942da71fd7a28f4a4b2e8c51347240b9e2d OP_EQUAL",
    char *hex;
    int32_t middlelen,offset,len,OP_HASH160_len,OP_EQUAL_len;
    len = (int32_t)strlen(scriptasm);
    OP_HASH160_len = (int32_t)strlen("OP_HASH160");
    OP_EQUAL_len = (int32_t)strlen("OP_EQUAL");
    if ( strncmp(scriptasm,"OP_HASH160",OP_HASH160_len) == 0 && strncmp(scriptasm+len-OP_EQUAL_len,"OP_EQUAL",OP_EQUAL_len) == 0 )
    {
        hex = calloc(1,len+1);
        offset = 0;
        offset = add_opcode(hex,offset,OP_HASH160_OPCODE);
        middlelen = len - OP_HASH160_len - OP_EQUAL_len - 2;
        offset = add_opcode(hex,offset,middlelen/2);
        memcpy(hex+offset,scriptasm+OP_HASH160_len+1,middlelen);
        hex[offset+middlelen] = _hexbyte((OP_EQUAL_OPCODE >> 4) & 0xf);
        hex[offset+middlelen+1] = _hexbyte(OP_EQUAL_OPCODE & 0xf);
        hex[offset+middlelen+2] = 0;
        if ( Debuglevel > 2 )
            printf("(%s) -> (%s)\n",scriptasm,hex);
        strcpy(scriptasm,hex);
        free(hex);
        return((int32_t)(2+middlelen+2));
    }
    // printf("cant assembly anything but OP_HASH160 + <key> + OP_EQUAL (%s)\n",scriptasm);
    return(-1);
}

int32_t convert_to_bitcoinhex(char *scriptasm)
{
    char *hex,pubkey[512],*endstr;
    int32_t i,j,middlelen,len,OP_HASH160_len,OP_EQUAL_len;
    len = (int32_t)strlen(scriptasm);
    OP_HASH160_len = (int32_t)strlen("OP_DUP OP_HASH160");
    OP_EQUAL_len = (int32_t)strlen("OP_EQUALVERIFY OP_CHECKSIG");
    if ( strncmp(scriptasm,"OP_DUP OP_HASH160",OP_HASH160_len) == 0 && strncmp(scriptasm+len-OP_EQUAL_len,"OP_EQUALVERIFY OP_CHECKSIG",OP_EQUAL_len) == 0 )
    {
        middlelen = len - OP_HASH160_len - OP_EQUAL_len - 2;
        memcpy(pubkey,scriptasm+OP_HASH160_len+1,middlelen);
        pubkey[middlelen] = 0;
        
        hex = calloc(1,len+1);
        calc_script(hex,pubkey,0);
        strcpy(scriptasm,hex);
        free(hex);
        return((int32_t)(2+middlelen+2));
    }
    else
    {
        endstr = scriptasm + strlen(scriptasm) - strlen(" OP_CHECKSIG");
        if ( strcmp(endstr," OP_CHECKSIG") == 0 )
        {
            strcpy(endstr,"ac");
            //printf("NEWSCRIPT.[%s]\n",scriptasm);
            return((int32_t)strlen(scriptasm));
        }
        else if ( strncmp(scriptasm,"OP_RETURN ",strlen("OP_RETURN ")) == 0 )
        {
            //printf("OP_RETURN.(%s) -> ",scriptasm);
            add_opcode(scriptasm,0,OP_RETURN_OPCODE);
            for (i=2,j=strlen("OP_RETURN "); j<=len; j++,i++)
                scriptasm[i] = scriptasm[j];
            //printf("(%s)\n",scriptasm);
            return((int32_t)strlen(scriptasm));
        }
    }
    return(origconvert_to_bitcoinhex(scriptasm));
}

cJSON *script_has_address(int32_t *nump,cJSON *scriptobj)
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

int32_t extract_txvals(char *coinaddr,char *script,int32_t nohexout,cJSON *txobj)
{
    int32_t numaddresses;
    cJSON *scriptobj,*addrobj,*hexobj;
    scriptobj = cJSON_GetObjectItem(txobj,"scriptPubKey");
    if ( scriptobj != 0 )
    {
        addrobj = script_has_address(&numaddresses,scriptobj);
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
                convert_to_bitcoinhex(script);
        }
        return(0);
    }
    return(-1);
}

void ram_set_string(char type,char *dest,char *src,long max)
{
    if ( src == 0 || src[0] == 0 )
        sprintf(dest,"ffff");
    else if ( strlen(src) < max-1)
        strcpy(dest,src);
    else sprintf(dest,"nonstandard");
}

uint64_t _get_txvouts(struct rawblock *raw,struct rawtx *tx,struct ramchain_info *ram,cJSON *voutsobj)
{
    int32_t extract_txvals(char *coinaddr,char *script,int32_t nohexout,cJSON *txobj);
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
            extract_txvals(coinaddr,script,1,item); // default to nohexout
            ram_set_string('a',v->coinaddr,coinaddr,sizeof(v->coinaddr));
            ram_set_string('s',v->script,script,sizeof(v->script));
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
            ram_set_string('t',v->txidstr,txidstr,sizeof(v->txidstr));
        }
    } else printf("error with vins\n");
    tx->numvins = numvins;
    return(numvins);
}

char *get_transaction(struct ramchain_info *ram,char *txidstr)
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
    if ( (retstr= get_transaction(ram,txidstr)) != 0 )
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

char *get_blockhashstr(struct ramchain_info *ram,uint32_t blocknum)
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

cJSON *get_blockjson(uint32_t *heightp,struct ramchain_info *ram,char *blockhashstr,uint32_t blocknum)
{
    cJSON *json = 0;
    int32_t flag = 0;
    char buf[1024],*blocktxt = 0;
    if ( blockhashstr == 0 )
        blockhashstr = get_blockhashstr(ram,blocknum), flag = 1;
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
    clear_rawblock(raw);
    //raw->blocknum = blocknum;
    //printf("_get_blockinfo.%d\n",blocknum);
    raw->minted = raw->numtx = raw->numrawvins = raw->numrawvouts = 0;
    if ( (json= get_blockjson(0,ram,0,blocknum)) != 0 )
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

cJSON *ram_rawblock_json(struct rawblock *raw)
{
    int32_t i,n;
    cJSON *array,*json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"height",cJSON_CreateNumber(raw->blocknum));
    cJSON_AddItemToObject(json,"numtx",cJSON_CreateNumber(raw->numtx));
    cJSON_AddItemToObject(json,"mint",cJSON_CreateNumber(dstr(raw->minted)));
    if ( (n= raw->numtx) > 0 )
    {
        array = cJSON_CreateArray();
        for (i=0; i<n; i++)
            cJSON_AddItemToArray(array,ram_rawtx_json(raw,i));
        cJSON_AddItemToObject(json,"tx",array);
    }
    return(json);
}

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

int32_t emit_ramchain_token(int32_t compressflag,HUFF *outbits,struct ramchain_info *ram,struct ramchain_token *token)
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
            return(emit_varbits(outbits,token->rawind));
        }
        else
        {
            if ( type == 2 )
                return(emit_smallbits(outbits,token->U.val16));
            else if ( type == 4 || type == -4 )
                return(emit_varbits(outbits,token->U.val32));
            else if ( type == 8 || type == -8 )
                return(emit_valuebits(outbits,token->U.val64));
            else return(emit_smallbits(outbits,token->U.val8));
        }
    }
    for (i=0; i<token->numbits; i++)
        hputbit(outbits,GETBIT(hashdata,i));
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
        if ( decode_hashdata(hashstr,type,hashdata) == 0 )
        {
            printf("ram_extractstring.V t.(%c) decode_hashdata error\n",type);
            return(0);
        }
        rawind = ram_conv_hashstr(ram,hashstr,type);
    }
    else
    {
        if ( format == 'B' )
            decode_varbits(&rawind,hp);
        else if ( format == '*' )
        {
            ram_decode_huffcode(&U,ram,selector,offset);
            rawind = U.val32;
        }
        else printf("ram_extractstring illegal format.%d\n",format);
        //printf("type.%c rawind.%d hashdataptr.%p\n",type,rawind,ram_gethashdata(ram,type,rawind));
        if ( (hashdataptr = ram_gethashdata(ram,type,rawind)) == 0 || decode_hashdata(hashstr,type,hashdataptr) == 0 )
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
        decode_varbits(&i,hp);
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
        decode_smallbits(&s,hp);
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
        decode_valuebits(&x,hp);
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
    struct huffpair_hash *hp;
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
            token = calloc(1,sizeof(*token));
        else if ( (hashdata= encode_hashstr(&datalen,data,type,hashstr)) != 0 )
        {
            token = calloc(1,sizeof(*token) + datalen - sizeof(token->U));
            memcpy(token->U.hashdata,hashdata,datalen);
            token->numbits = (datalen << 3);
            hp = ram_hashdata_search(ram_gethash(ram,type),hashdata,datalen);
            token->rawind = hp->rawind;
           // printf(">>>>>> rawind.%d -> %d\n",rawind,token->rawind);
        } else printf("encode_hashstr error for (%c).(%s)\n",type,hashstr);
    }
    else //if ( destformat == 'B' )
    {
        token = calloc(1,sizeof(*token));
        token->numbits = (sizeof(rawind) << 3);
        if ( hashstr != 0 && hashstr[0] != 0 )
        {
            rawind = ram_conv_hashstr(ram,hashstr,type);
            //printf("(%s) -> %d\n",hashstr,rawind);
        }
        token->rawind = rawind;
        //printf("<<<<<<<<<< rawind.%d -> %d\n",rawind,token->rawind);
    }
    /*else if ( destformat == '*' )
    {
        token = calloc(1,sizeof(*token));
        token->ishuffcode = 1;
        if ( hashstr != 0 && hashstr[0] != 0 )
            rawind = ram_conv_hashstr(ram,hashstr,type);
        token->rawind = rawind;
        token->numbits = ram_huffencode(&token->U.val64,ram,token,&U,sizeof(rawind) << 3);
    }*/
    return(token);
}

void ram_sprintf_number(char *hashstr,union ramtypes *U,int32_t type)
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

struct ramchain_token *ram_createtoken(struct ramchain_info *ram,char selector,uint16_t offset,int32_t type,char *hashstr,uint32_t rawind,union ramtypes *U,int32_t datalen)
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
            token = calloc(1,sizeof(*token));
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
    int32_t type;
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
        if ( decode_hashdata(hashstr,token->type,token->U.hashdata) == 0 )
        {
            *datalenp = 0;
            printf("expand_ramchain_token decode_hashdata error\n");
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

struct ramchain_token **ram_tokenize_rawblock(int32_t *numtokensp,struct ramchain_info *ram,struct rawblock *raw)
{ // parse structure full of gaps
    union ramtypes U;
    uint32_t rawind = 0;
    int32_t i,n,maxtokens,numtokens = 0;
    struct ramchain_token **tokens = 0;
    maxtokens = num_rawblock_tokens(raw) + (raw->numtx * num_rawtx_tokens(raw)) + (raw->numrawvins * num_rawvin_tokens(raw)) + (raw->numrawvouts * num_rawvout_tokens(raw));
    tokens = calloc(maxtokens,sizeof(*tokens));
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

struct ramchain_token *extract_and_tokenize(union ramtypes *Uptr,struct ramchain_info *ram,int32_t selector,int32_t offset,HUFF *hp,int32_t srcformat,int32_t size)
{
    int32_t i;
    union ramtypes U;
    memset(&U,0,sizeof(U));
    if ( srcformat == 'V' )
    {
        size <<= 3;
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
        tokens[numtokens++] = extract_and_tokenize(&U,ram,'I',numrawvins,hp,format,sizeof(uint16_t));
        //printf("numrawvins.%d: txid_rawind.%d (%s) vout.%d\n",numrawvins,txid_rawind,txidstr,U.val16);
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
        tokens[numtokens++] = extract_and_tokenize(&U,ram,'O',numrawvouts,hp,format,sizeof(uint64_t));
        //printf("numrawvouts.%d: scriptind.%d (%s) addrind.%d (%s) value.(%.8f)\n",numrawvouts,scriptind,scriptstr,addrind,coinaddr,dstr(U.val64));
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
        tokens[numtokens++] = extract_and_tokenize(&U,ram,'T',(txind<<1) | 0,hp,format,sizeof(uint16_t)), numvins = U.val16;
        tokens[numtokens++] = extract_and_tokenize(&U,ram,'T',(txind<<1) | 1,hp,format,sizeof(uint16_t)), numvouts = U.val16;
        if ( (txid_rawind= ram_extractstring(txidstr,'t',ram,'T',(txind<<1),hp,format)) != 0 )
            tokens[numtokens++] = ram_createstring(ram,'T',(txind<<1),'t',txidstr,txid_rawind);
        else return(orignumtokens);
        //printf("txind.%d (%d %d) numvins.%d numvouts.%d (%s) txid_rawind.%d\n",txind,*firstvinp,*firstvoutp,numvins,numvouts,txidstr,txid_rawind);
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

struct ramchain_token **ram_tokenize_bitstream(uint32_t *blocknump,int32_t *numtokensp,struct ramchain_info *ram,HUFF *hp,int32_t format)
{
    // 'V' packed structure using varints and varstrs
    // 'B' bitstream using varbits and rawind substitution for strings
    // '*' bitstream using huffman codes
    struct ramchain_token **tokens = 0;
    int32_t i,numtx,firstvin,firstvout,maxtokens,numtokens = 0;
    maxtokens = 65536 * 2;
    union ramtypes U;
    uint64_t minted;
    uint32_t blocknum,lastnumtokens;
    tokens = calloc(maxtokens,sizeof(*tokens));
    if ( tokens != 0 )
    {
        tokens[numtokens++] = extract_and_tokenize(&U,ram,'B',0,hp,format,sizeof(uint32_t)), *blocknump = blocknum = U.val32;
        tokens[numtokens++] = extract_and_tokenize(&U,ram,'B',0,hp,format,sizeof(uint16_t)), numtx = U.val16;
        tokens[numtokens++] = extract_and_tokenize(&U,ram,'B',0,hp,format,sizeof(uint64_t)), minted = U.val64;
    } else numtokens += num_rawblock_tokens(raw), *blocknump = 0;
    //printf("blocknum.%d numt.%d minted %.8f\n",blocknum,numtx,dstr(minted));
    if ( numtx > 0 )
    {
        lastnumtokens = numtokens;
        for (i=firstvin=firstvout=0; i<numtx; i++)
            if ( (numtokens= ram_rawtx_huffscan(ram,tokens,numtokens,hp,format,i,&firstvin,&firstvout)) == lastnumtokens )
            {
                printf("parse error at token %d of %d | firstvin.%d firstvout.%d\n",i,numtokens,firstvin,firstvout);
                for (i=0; i<numtokens; i++)
                    if ( tokens[i] != 0 )
                        free(tokens[i]);
                free(tokens);
                *numtokensp = -1;
                return(0);
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
    int32_t i,checkformat,numtokens = 0;
    struct ramchain_token **tokens;
    hrewind(hp);
    checkformat = hp->buf[0], hp->ptr++, hp->bitoffset = 8;
    if ( checkformat != format )
    {
        //printf("got checkformat.%d vs %d\n",checkformat,format);
        return(0);
    }
    tokens = ram_tokenize_bitstream(&blocknum,&numtokens,ram,hp,format);
    if ( tokens != 0 )
    {
        //printf("got blocknum.%d numtokens.%d\n",blocknum,numtokens);
        for (i=0; i<numtokens; i++)
            if ( tokens[i] != 0 )
                   free(tokens[i]);
        free(tokens);
        return(blocknum);
    }
    return(-1);
}

int32_t expand_ramchain_token(struct rawblock *raw,struct ramchain_info *ram,struct ramchain_token *token)
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
        else printf("expand_ramchain_token: error finding destptr in rawblock\n");
    } else printf("expand_ramchain_token: error decoding token\n");
    return(-1);
}

int32_t emit_and_free(int32_t compressflag,HUFF *hp,struct ramchain_info *ram,struct ramchain_token **tokens,int32_t numtokens)
{
    int32_t i;
    for (i=0; i<numtokens; i++)
        if ( tokens[i] != 0 )
        {
            emit_ramchain_token(compressflag,hp,ram,tokens[i]);
            free(tokens[i]);
        }
    free(tokens);
    if ( 0 )
    {
        for (i=0; i<(hp->bitoffset>>3)+1; i++)
            printf("%02x ",hp->buf[i]);
        printf("emit_and_free\n");
    }
    return(conv_bitlen(hp->endpos));
}

int32_t expand_and_free(cJSON **jsonp,struct rawblock *raw,struct ramchain_info *ram,struct ramchain_token **tokens,int32_t numtokens)
{
    int32_t i;
    clear_rawblock(raw);
    for (i=0; i<numtokens; i++)
        if ( tokens[i] != 0 )
        {
            expand_ramchain_token(raw,ram,tokens[i]);
            free(tokens[i]);
        }
    ram_patch_rawblock(raw);
    {
        //cJSON *json = ram_rawblock_json(raw);
        //printf("%s\n",cJSON_Print(json));
        //printf("raw numtx.%d numvouts.%d numvins.%d | tx0.(%d %d)\n",raw->numtx,raw->numrawvouts,raw->numrawvins,raw->txspace[0].numvouts,raw->txspace[0].numvins);
    }
    if ( jsonp != 0 )
        (*jsonp) = ram_rawblock_json(raw);
    free(tokens);
    return(numtokens);
}

int32_t ram_compress_blockhex(HUFF *hp,struct ramchain_info *ram,uint8_t *data,int32_t datalen)
{
    struct ramchain_token **tokens;
    int32_t numtokens;
    uint32_t format,blocknum;
    HUFF *srcbits;
    if ( hp != 0 )
    {
        hrewind(hp);
        emit_bits(hp,'B',8);
        srcbits = hopen(data,datalen,0);
        if ( decode_bits(&format,srcbits,8) != 8 || format != 'V' )
            printf("error decode_bits in ram_expand_rawinds format.%d != (%c) %d",format,'V','V');
        else if ( (tokens= ram_tokenize_bitstream(&blocknum,&numtokens,ram,srcbits,format)) != 0 )
        {
            hclose(srcbits);
            return(emit_and_free(1,hp,ram,tokens,numtokens));
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
        emit_bits(hp,destformat,8);
        if ( (tokens= ram_tokenize_rawblock(&numtokens,ram,raw)) != 0 )
            return(emit_and_free(destformat!='V',hp,ram,tokens,numtokens));
    }
    return(-1);
}

int32_t ram_expand_bitstream(cJSON **jsonp,struct rawblock *raw,struct ramchain_info *ram,HUFF *hp)
{
    struct ramchain_token **tokens;
    int32_t numtokens;
    uint32_t format,blocknum;
    clear_rawblock(raw);
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
        if ( format == 'C' )
            format = 'B';
        if ( format != 'B' && format != 'V' && format != '*' )
            printf("error decode_bits in ram_expand_rawinds format.%d != (%c/%c/%c) %d/%d/%d\n",format,'V','B','*','V','B','*');
        else if ( (tokens= ram_tokenize_bitstream(&blocknum,&numtokens,ram,hp,format)) != 0 )
            return(expand_and_free(jsonp,raw,ram,tokens,numtokens));
        else printf("error expanding bitstream\n");
    }
    return(-1);
}

char *ram_blockstr(struct rawblock *tmp,struct ramchain_info *ram,struct rawblock *raw)
{
    cJSON *json = 0;
    struct ramchain_token **tokens;
    int32_t numtokens;
    char *retstr;
    if ( (tokens= ram_tokenize_rawblock(&numtokens,ram,raw)) != 0 )
        expand_and_free(&json,tmp,ram,tokens,numtokens);
    retstr = cJSON_Print(json);
    free_json(json);
    return(retstr);
}

HUFF *ram_genblock(HUFF *tmphp,struct rawblock *tmp,struct ramchain_info *ram,int32_t blocknum,int32_t format,HUFF **prevhpp)
{
    HUFF *hp = 0;
    int32_t datalen;
    void *block = 0;
    if ( format == 0 )
        format = 'V';
    if ( format == 'B' && prevhpp != 0 && (hp= *prevhpp) != 0 )
    {
        if ( ram_expand_bitstream(0,tmp,ram,hp) <= 0 )
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
    hp = 0;
    hclear(tmphp);
    if ( (datalen= ram_emitblock(tmphp,format,ram,tmp)) > 0 )
    {
        //printf("ram_emitblock datalen.%d bitoffset.%d\n",datalen,hp->bitoffset);
        block = calloc(1,datalen);
        memcpy(block,tmphp->buf,datalen);
        hp = hopen(block,datalen,block);
        hseek(hp,datalen << 3,SEEK_SET);
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
    char hashstr[8193],retbuf[1024],*str;
    if ( (str= ram_txid(hashstr,ram,rawind)) != 0 )
    {
        sprintf(retbuf,"{\"result\":\"%u\",\"txid\":\"%s\"}",rawind,str);
        return(clonestr(retbuf));
    }
    return(clonestr("{\"error\":\"no txid info\"}"));
}

char *ram_addrind_json(struct ramchain_info *ram,char *str)
{
    char retbuf[1024];
    uint32_t rawind;
    if ( (rawind= ram_addrind(ram,str)) != 0xffffffff )
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
    if ( (rawind= ram_txidind(ram,str)) != 0xffffffff )
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
    if ( (rawind= ram_scriptind(ram,str)) != 0xffffffff )
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
    sprintf(buf,"%-5s: RT.%d V.%d B.%d B64.%d B4096.%d | %s %s R%.2f | estimated completion V%.1f B%.1f\n",ram->name,ram->RTblocknum,ram->Vblocks.blocknum,ram->Bblocks.blocknum,ram->blocks64.blocknum,ram->blocks4096.blocknum,_mbstr(estsizeV),_mbstr2(estsizeB),estsizeV/(estsizeB+1),estimatedV,estimatedB);
}

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

char *ramscript(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,char *txidstr,int32_t tx_vout,struct address_entry *bp)
{
    char retbuf[1024];
    struct address_entry B;
    struct rawvout *vo;
    struct rawblock *raw;
    HUFF *block;
    int32_t datalen;
    struct ramchain_info *ram = get_ramchain_info(coin);
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    if ( bp == 0 )
    {
        bp = &B;
        ram_address_entry(bp,ram,txidstr,tx_vout);
    }
    if ( (block= ram_getblock(ram,bp->blocknum)) != 0 )
    {
        raw = ram->R;
        if ( (datalen= ram_expand_bitstream(0,raw,ram,block)) > 0 )
        {
            if ( (vo= ram_rawvout(raw,bp->txind,bp->v)) != 0 )
            {
                sprintf(retbuf,"{\"result\":\"script\",\"txid\":\"%s\",\"vout\":%u,\"hex\":\"%s\"}",txidstr,bp->v,vo->script);
                free(block->buf), hclose(block);
                return(clonestr(retbuf));
            }
        }
    }
    return(clonestr("{\"error\":\"no ramchain info\"}"));
}

char *ramblock(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,uint32_t blocknum)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    int32_t numtx,datalen;
    cJSON *json;
    char *hexstr,*retstr;
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    numtx = _get_blockinfo(ram->R,ram,blocknum);
    if ( (datalen= ram_emitblock(ram->tmphp,'V',ram,ram->R)) > 0 )
    {
        hexstr = calloc(1,datalen*2+1);
        init_hexbytes_noT(hexstr,ram->tmphp->buf,datalen);
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json,"result",cJSON_CreateString(coin));
        cJSON_AddItemToObject(json,"block",cJSON_CreateNumber(blocknum));
        cJSON_AddItemToObject(json,"blockhex",cJSON_CreateString(hexstr));
        cJSON_AddItemToObject(json,"datalen",cJSON_CreateNumber(datalen));
        retstr = cJSON_Print(json);
        free_json(json);
    } else retstr = clonestr("{\"error\":\"no block info\"}");
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
    if ( (complen= ram_compress_blockhex(ram->tmphp,ram,data,datalen)) > 0 )
    {
        hexstr = calloc(1,complen*2+1);
        init_hexbytes_noT(hexstr,ram->tmphp->buf,complen);
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json,"result",cJSON_CreateString(coin));
        cJSON_AddItemToObject(json,"bitstream",cJSON_CreateString(hexstr));
        cJSON_AddItemToObject(json,"datalen",cJSON_CreateNumber(complen));
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
    hp = hopen(data,datalen,0);
    if ( (expandlen= ram_expand_bitstream(0,ram->R,ram,hp)) > 0 )
    {
        free(retstr);
        retstr = ram_blockstr(ram->R2,ram,ram->R);
    } else clonestr("{\"error\":\"no ram_expand_bitstream info\"}");
    hclose(hp);
    free(data);
    return(retstr);
}

int32_t _is_unspent(struct huffpayload *payload)
{
    struct address_entry *spentB = &payload->B;
    if ( spentB->blocknum != 0 || spentB->txind != 0 || spentB->v != 0 )
        return(0);
    else return(1);
}

cJSON *gen_address_entry_json(struct address_entry *bp)
{
    cJSON *array = cJSON_CreateArray();
    cJSON_AddItemToArray(array,cJSON_CreateNumber(bp->blocknum));
    cJSON_AddItemToArray(array,cJSON_CreateNumber(bp->txind));
    cJSON_AddItemToArray(array,cJSON_CreateNumber(bp->v));
    return(array);
}

cJSON *gen_addrpayload_json(struct huffpayload *payload)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"tx_rawind",cJSON_CreateNumber(payload->tx_rawind));
    cJSON_AddItemToObject(json,"script",cJSON_CreateNumber(payload->extra));
    cJSON_AddItemToObject(json,"tx",gen_address_entry_json(&payload->B));
    if ( _is_unspent(payload) != 0 )
        cJSON_AddItemToObject(json,"spent",gen_address_entry_json(&payload->spentB));
    cJSON_AddItemToObject(json,"value",cJSON_CreateNumber(payload->value));
    return(json);
}

uint64_t calc_unspent_outputs(struct ramchain_info *ram,char *addr)
{
    uint64_t unspent = 0;
    int32_t i,numpayloads;
    struct huffpayload *payloads;
    if ( (payloads= ramchain_addr_payloads(&numpayloads,ram,addr)) != 0 && numpayloads > 0 )
    {
        for (i=0; i<numpayloads; i++)
            if ( _is_unspent(&payloads[i]) != 0 )
                unspent += payloads[i].value;
    }
    return(unspent);
}

uint64_t ram_calcunspent(cJSON **arrayp,char *destcoin,double rate,char *coin,char *addr)
{
    struct ramchain_info *ram = get_ramchain_info(coin);
    uint64_t unspent = 0;
    cJSON *item;
    if ( ram != 0 )
    {
        if ( (unspent= calc_unspent_outputs(ram,addr)) != 0 && arrayp != 0 )
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

char *ramtxlist(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,char *coinaddr,int32_t unspentflag)
{
    char *retstr = 0;
    cJSON *json = 0,*array = 0;
    int32_t i,numpayloads;
    struct huffpayload *payloads;
    struct ramchain_info *ram = get_ramchain_info(coin);
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    if ( (payloads= ramchain_addr_payloads(&numpayloads,ram,coinaddr)) != 0 && numpayloads > 0 )
    {
        for (i=0; i<numpayloads; i++)
        {
            if ( unspentflag == 0 || _is_unspent(&payloads[i]) != 0 )
                cJSON_AddItemToArray(array,gen_addrpayload_json(&payloads[i]));
        }
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json,coinaddr,array);
    }
    if ( json != 0 )
    {
        retstr = cJSON_Print(json);
        free_json(json);
    } else retstr = clonestr("{\"error\":\"ramtxlist no data\"}");
    return(retstr);
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

char *ramrichlist(char *origargstr,char *sender,char *previpaddr,char *destip,char *coin,int32_t numwhales)
{
    int32_t i,ind,numaddrs,n = 0;
    cJSON *item,*array = 0;
    char **addrs,*retstr;
    double *sortbuf;
    uint64_t unspent;
    struct ramchain_info *ram = get_ramchain_info(coin);
    if ( ram == 0 )
        return(clonestr("{\"error\":\"no ramchain info\"}"));
    if ( (addrs= ram_getalladdrs(&numaddrs,ram)) != 0 && numaddrs > 0 )
    {
        sortbuf = calloc(2*numaddrs,sizeof(*sortbuf));
        for (i=0; i<numaddrs; i++)
        {
            if ( (unspent= calc_unspent_outputs(ram,addrs[i])) != 0 )
            {
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
                cJSON_AddItemToObject(item,addrs[ind],cJSON_CreateNumber(sortbuf[i<<1]));
                cJSON_AddItemToArray(array,item);
            }
        }
        free(addrs);
        free(sortbuf);
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
                total += ram_calcunspent(&array,coin,0.,coin,coinaddrs[0][j]);
            cJSON_AddItemToObject(retjson,"total",array);
        }
        else if ( rates != 0 )
        {
            for (i=0; i<numcoins; i++)
            {
                for (j=0; coinaddrs[i][j]!=0; j++)
                    total += ram_calcunspent(&array,coin,rates[i],coins[i],coinaddrs[i][j]);
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

void ensure_dir(char *dirname) // jl777: does this work in windows?
{
    FILE *fp;
    char fname[512],cmd[512];
    sprintf(fname,"%s/tmp",dirname);
    if ( (fp= fopen(fname,"rb")) == 0 )
    {
        sprintf(cmd,"mkdir %s",dirname);
        if ( system(cmd) != 0 )
            printf("error making subdirectory (%s) %s (%s)\n",cmd,dirname,fname);
        fp = fopen(fname,"wb");
        if ( fp != 0 )
            fclose(fp);
    }
    else fclose(fp);
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
       // printf("DIRB: (%s)\n",dirB);
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

HUFF *verify_Vblock(struct ramchain_info *ram,uint32_t blocknum,HUFF *hp)
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
        if ( (err= rawblockcmp(ram->Vblocks.R2,ram->Vblocks.R)) == 0 )
        {
            printf("OK ");
            return(hp);
        } else printf("verify_Vblock.%d err.%d\n",blocknum,err);
    } else printf("ram_expand_bitstream returned.%d\n",datalen);
    return(0);
}

HUFF *verify_Bblock(struct ramchain_info *ram,uint32_t blocknum,HUFF *Bhp)
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
            if ( rawblockcmp(ram->Bblocks.R,ram->Bblocks.R2) != 0 )
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

long *ram_load_bitstreams(bits256 *sha,char *fname,HUFF *bitstreams[],int32_t num)
{
    FILE *fp;
    long *offsets = 0;
    bits256 tmp,stored;
    int32_t i,x = -1,len = 0;
    if ( (fp= fopen(fname,"rb")) != 0 )
    {
        offsets = calloc(num,sizeof(*offsets));
        memset(sha,0,sizeof(*sha));
        //printf("loading %s\n",fname);
        if ( fread(&x,1,sizeof(x),fp) == sizeof(x) && x == num )
        {
            if ( fread(&stored,1,sizeof(stored),fp) == sizeof(stored) )
            {
                for (i=0; i<num; i++)
                {
                    if ( bitstreams[i] != 0 )
                        hclose(bitstreams[i]);
                    if ( (bitstreams[i]= hload(&offsets[i],fp,0)) != 0 && bitstreams[i]->buf != 0 )
                        calc_sha256cat(tmp.bytes,sha->bytes,sizeof(*sha),bitstreams[i]->buf,bitstreams[i]->allocsize), *sha = tmp;
                    else printf("unexpected null bitstream at %d %p offset.%ld\n",i,bitstreams[i],offsets[i]);
                }
                if ( memcmp(sha,&stored,sizeof(stored)) != 0 )
                    printf("sha error %s %llx vs stored.%llx\n",fname,(long long)sha->txid,(long long)stored.txid);
            } else printf("error loading sha\n");
        } else printf("num mismatch %d != num.%d\n",x,num);
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
    int32_t i,retval = 0,verified=0,rwflag = 0;
    retval = 0;
    if ( (offsets= ram_load_bitstreams(sha,fname,blocks,num)) != 0 )
    {
        if ( refsha != 0 && memcmp(sha->bytes,refsha,sizeof(*sha)) != 0 )
        {
            printf("refsha cmp error for %s %llx vs %llx\n",fname,(long long)sha->txid,(long long)refsha->txid);
            hpurge(blocks,num);
            free(offsets);
            return(0);
        }
        if ( M->fileptr != 0 )
            close_mappedptr(M);
        memset(M,0,sizeof(*M));
        if ( init_mappedptr(0,M,0,rwflag,fname) != 0 )
        {
            for (i=0; i<num; i++)
            {
                if ( (hp= blocks[i]) != 0 )
                {
                    if ( (blocks[i]= hopen((void *)((long)M->fileptr + offsets[i]),hp->allocsize,0)) != 0 )
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
                        ram->blocks.hps[blocknum+i] = hp;
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
    return(retval);
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

uint32_t load_ram_blocks(struct ramchain_info *ram,struct mappedblocks *blocks,uint32_t firstblock,int32_t numblocks)
{
    HUFF **hps;
    bits256 sha;
    char fname[1024],formatstr[16];
    uint32_t blocknum,i,flag,incr,n = 0;
    incr = (1 << blocks->shift);
    ram_setformatstr(formatstr,blocks->format);
    flag = blocks->format != 'V';
    for (i=0; i<numblocks; i+=incr)
    {
        blocknum = (firstblock + i);
        ram_setfname(fname,ram,blocknum,formatstr);
        //printf("loading (%s)\n",fname);
        if ( (hps= ram_get_hpptr(blocks,blocknum)) != 0 )
        {
            if ( blocks->format == 64 || blocks->format == 4096 )
            {
                if ( ram_map_bitstreams(1,ram,blocknum,&blocks->M[blocknum >> blocks->shift],&sha,hps,blocks->format,fname,0) <= 0 )
                {
                    //break;
                }
            }
            else
            {
                if ( (*hps= hload(0,0,fname)) != 0 )
                {
                    if ( flag == 0 || ram_verify(ram,*hps,blocks->format) == blocknum )
                    {
                        //if ( flag != 0 )
                        //    fprintf(stderr,"=");
                        if ( (++n % 100) == 0 )
                            fprintf(stderr," total.%d loaded.(%s)\n",n,fname);
                    }
                    else hclose(*hps), *hps = 0;
                } //else break;
            }
        }
    }
    blocks->contiguous = ram_setcontiguous(blocks);
    return(blocks->contiguous);
}

uint32_t create_ramchain_block(int32_t verifyflag,struct ramchain_info *ram,struct mappedblocks *blocks,struct mappedblocks *prevblocks,uint32_t blocknum)
{
    char fname[1024],formatstr[16];
    FILE *fp;
    bits256 sha,refsha;
    HUFF *hp,**hpptr,**hps,**prevhps;
    int32_t i,n,datalen = 0;
    ram_setformatstr(formatstr,blocks->format);
    prevhps = ram_get_hpptr(prevblocks,blocknum);
    if ( blocks->format == 'V' || blocks->format == 'B' )
    {
        if ( (hpptr= ram_get_hpptr(blocks,blocknum)) != 0 )
        {
            //printf("create %s %d\n",formatstr,blocknum);
            if ( *hpptr == 0 && (hp= ram_genblock(blocks->tmphp,blocks->R,ram,blocknum,blocks->format,prevhps)) != 0 )
            {
                //printf("block.%d created.%c block.%d numtx.%d minted %.8f\n",blocknum,blocks->format,blocks->R.blocknum,blocks->R.numtx,dstr(blocks->R.minted));
                ram_setfname(fname,ram,blocknum,formatstr);
                if ( (fp= fopen(fname,"wb")) != 0 )
                {
                    hflush(fp,hp);
                    fclose(fp);
                }
                if ( *hpptr != 0 )
                {
                    hclose(*hpptr);
                    *hpptr = 0;
                    printf("OVERWRITE.(%s) size.%ld bitoffset.%d allocsize.%d\n",fname,ftell(fp),hp->bitoffset,hp->allocsize);
                }
                if ( ram_verify(ram,hp,blocks->format) == blocknum )
                {
                    if ( verifyflag != 0 && ((blocks->format == 'B') ? verify_Bblock(ram,blocknum,hp) : verify_Vblock(ram,blocknum,hp)) == 0 )
                    {
                        printf("error creating %cblock.%d\n",blocks->format,blocknum), datalen = 0;
                        delete_file(fname,0), hclose(hp);
                    }
                    else
                    {
                        datalen = (1 + hp->allocsize);
                        if ( blocks->format == 'V' )
                            fprintf(stderr," %s CREATED.%c block.%d datalen.%d\n",ram->name,blocks->format,blocknum,datalen+1);
                        //else fprintf(stderr,"%s.B.%d ",ram->name,blocknum);
                        *hpptr = hp;
                        if ( ram->blocks.hps[blocknum] == 0 )
                            ram->blocks.hps[blocknum] = hp;
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
                datalen = ram_map_bitstreams(verifyflag,ram,blocknum,&blocks->M[blocknum >> blocks->shift],&sha,hps,n,fname,&refsha);
        } else printf("prev.%d missing blockptr at %d\n",prevblocks->format,blocknum+i);
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

int32_t init_hashtable(struct ramchain_info *ram,char type)
{
    long offset,fileptr;
    uint64_t datalen;
    char fname[1024];
    uint8_t *hashdata;
    int32_t varsize,num,rwflag = 0;
    struct huffhash *hash;
    struct huffpair_hash *hp;
    hash = ram_gethash(ram,type);
    strcpy(hash->coinstr,ram->name);
    hash->type = type;
    huffhash_setfname(fname,hash,0);
    num = 0;
    if ( (hash->newfp= fopen(fname,"rb+")) != 0 )
    {
        if ( init_mappedptr(0,&hash->M,0,rwflag,fname) == 0 )
            return(0);
        fileptr = (long)hash->M.fileptr;
        offset = 0;
        while ( (varsize= (int32_t)load_varint(&datalen,hash->newfp)) > 0 && (offset + datalen) <= hash->M.allocsize )
        {
            hashdata = (uint8_t *)(fileptr + offset);
            if ( num < 10 )
            {
                char hexbytes[8192];
                struct huffpair_hash *checkhp;
                init_hexbytes_noT(hexbytes,hashdata,varsize+datalen);
                HASH_FIND(hh,hash->table,hashdata,varsize + datalen,checkhp);
                printf("offset %ld: varsize.%d datalen.%d created.(%s) ind.%d | checkhp.%p\n",offset,(int)varsize,(int)datalen,(type != 'a') ? hexbytes :(void *)((long)hashdata+varsize),hash->ind+1,checkhp);
            }
            HASH_FIND(hh,hash->table,hashdata,varsize + datalen,hp);
            if ( hp != 0 )
            {
                printf("corrupted hashtable %s: offset.%ld\n",fname,offset);
                exit(-1);
            }
            hp = calloc(1,sizeof(*hp));
            huffhash_add(hash,hp,hashdata,(int32_t)(varsize+datalen));
            offset += (varsize + datalen);
            fseek(hash->newfp,offset,SEEK_SET);
            num++;
        }
        printf("%s: loaded %d strings, ind.%d, offset.%ld allocsize.%llu %s\n",fname,num,hash->ind,offset,(long long)hash->M.allocsize,(offset != hash->M.allocsize) ? "ERROR":"OK");
        if ( offset != hash->M.allocsize )
            exit(-1);
    } else hash->newfp = fopen(fname,"wb");
    return(num);
}

void init_ramchain_directories(struct ramchain_info *ram)
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
    ram->blocks.blocknum = ram->RTblocknum = (get_RTheight(ram) - ram->min_confirms);
    ram_setdispstr(buf,ram,ram->startmilli);
    printf("%s\n",buf);
}

uint32_t process_ramchain(struct ramchain_info *ram,struct mappedblocks *blocks,struct mappedblocks *prev,double timebudget,double startmilli)
{
    char formatstr[16];
    double estimated;
    int32_t processed = 0;
    ram_setformatstr(formatstr,blocks->format);
    //printf("%s shift.%d %-5s.%d %.1f min left | [%d < %d]?\n",formatstr,blocks->shift,ram->name,blocks->blocknum,estimated,(blocks->blocknum >> blocks->shift),(prev->blocknum >> blocks->shift));
    while ( (blocks->blocknum >> blocks->shift) < (prev->blocknum >> blocks->shift) && ram_millis() < (startmilli + timebudget) )
    {
        create_ramchain_block(1,ram,blocks,prev,blocks->blocknum), processed++;
        blocks->processed += (1 << blocks->shift);
        blocks->blocknum += (1 << blocks->shift);
        estimated = estimate_completion(ram->name,startmilli,blocks->processed,(int32_t)ram->RTblocknum-blocks->blocknum) / 60000.;
    }
    //printf("(%d >> %d) < (%d >> %d)\n",blocks->blocknum,blocks->shift,prev->blocknum,blocks->shift);
    return(processed);
}

struct mappedblocks *init_ram_blocks(HUFF **copyhps,struct ramchain_info *ram,uint32_t firstblock,struct mappedblocks *blocks,struct mappedblocks *prevblocks,int32_t format,int32_t shift)
{
    void *ptr;
    int32_t numblocks,tmpsize = 1000000;
    blocks->R = calloc(1,sizeof(*blocks->R));
    blocks->R2 = calloc(1,sizeof(*blocks->R2));
    blocks->ram = ram;
    blocks->prevblocks = prevblocks;
    blocks->format = format;
    ptr = calloc(1,tmpsize), blocks->tmphp = hopen(ptr,tmpsize,ptr);
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
    blocks->hps = calloc(blocks->numblocks,sizeof(*blocks->hps));
    if ( format != 0 )
    {
        blocks->flags = calloc(blocks->numblocks >> (shift+6),sizeof(*blocks->flags));
        blocks->M = calloc(blocks->numblocks >> shift,sizeof(*blocks->M));
        blocks->blocknum = load_ram_blocks(ram,blocks,firstblock,blocks->numblocks);
    } else blocks->blocknum = blocks->contiguous = ram_setcontiguous(blocks);
    {
        char formatstr[16];
        ram_setformatstr(formatstr,blocks->format);
        printf("%s.%s contiguous blocks.%d | numblocks.%d\n",ram->name,formatstr,blocks->contiguous,blocks->numblocks);
    }
    return(blocks);
}

void init_ramchain(struct ramchain_info *ram)
{
    int32_t tmpsize = 1000000;
    uint8_t *ptr;
    double startmilli;
    startmilli = ram_millis();
    ptr = calloc(1,tmpsize), ram->tmphp = hopen(ptr,tmpsize,ptr);
    ram->R = calloc(1,sizeof(*ram->R));
    ram->R2 = calloc(1,sizeof(*ram->R2));
    init_hashtable(ram,'a'), init_hashtable(ram,'s'), init_hashtable(ram,'t');
    printf("%.1f seconds to init_ramchain.%s hashtables\n",(ram_millis() - startmilli)/1000.,ram->name);
    strcpy(ram->dirpath,".");
    init_ramchain_directories(ram);
    printf("set ramchain blocknum.%s %d vs RT.%d %.1f seconds to init_ramchain.%s directories\n",ram->name,ram->Vblocks.blocknum,ram->blocks.blocknum,(ram_millis() - startmilli)/1000.,ram->name);
    ram->blocks.blocknum = ram->RTblocknum = (get_RTheight(ram) - ram->min_confirms);
    ram->maxblock = (ram->RTblocknum + 10000);
    ram->blocks.hps = calloc(ram->maxblock,sizeof(*ram->blocks.hps));
    ram->mappedblocks[4] = init_ram_blocks(ram->blocks.hps,ram,0,&ram->blocks4096,&ram->blocks64,4096,12);
    ram->mappedblocks[3] = init_ram_blocks(ram->blocks.hps,ram,ram->blocks4096.contiguous,&ram->blocks64,&ram->Bblocks,64,6);
    ram->mappedblocks[2] = init_ram_blocks(ram->blocks.hps,ram,ram->blocks64.contiguous,&ram->Bblocks,&ram->Vblocks,'B',0);
    ram->mappedblocks[1] = init_ram_blocks(ram->blocks.hps,ram,ram->Bblocks.contiguous,&ram->Vblocks,&ram->blocks,'V',0);
    ram->mappedblocks[0] = init_ram_blocks(ram->blocks.hps,ram,0,&ram->blocks,0,0,0);
    ram_disp_status(ram);
    //getchar();
}

void *_process_mappedblocks(void *_blocks)
{
    struct mappedblocks *blocks = _blocks;
    printf("start _process_mappedblocks.%s format.%d\n",blocks->ram->name,blocks->format);
    while ( 1 )
    {
        if ( process_ramchain(blocks->ram,blocks,blocks->prevblocks,1000.,ram_millis()) == 0 )
            sleep(sqrt(1 << blocks->shift));
    }
}

void *_process_ramchain(void *_ram)
{
    struct ramchain_info *ram = _ram;
    int32_t pass;
    init_ramchain(ram);
    ram->startmilli = ram_millis();
    for (pass=1; pass<=4; pass++)
        if ( portable_thread_create((void *)_process_mappedblocks,ram->mappedblocks[pass]) == 0 )
            printf("ERROR _process_ramchain.%s\n",ram->name);
    while ( 1 )
    {
        ram->blocks.blocknum = ram->blocks.contiguous = ram_setcontiguous(&ram->blocks);
        ram_disp_status(ram);
        sleep(10);
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
           
void process_coinblocks(char *argcoinstr)
{
    int32_t threaded = 1;
    int32_t i,pass,processed = 0;
    //argcoinstr = "DOGE";
    if ( threaded == 0 )
        for (i=0; i<Numramchains; i++)
        if ( argcoinstr == 0 || strcmp(argcoinstr,Ramchains[i]->name) == 0 )
        {
            init_ramchain(Ramchains[i]);
            Ramchains[i]->startmilli = ram_millis();
        }
    while ( processed >= 0 )
    {
        processed = 0;
        for (i=0; i<Numramchains; i++)
        {
            if ( argcoinstr == 0 || strcmp(argcoinstr,Ramchains[i]->name) == 0 )
            {
                if ( threaded != 0 )
                {
                    printf("%d of %d: (%s) argcoinstr.%s\n",i,Numramchains,Ramchains[i]->name,argcoinstr!=0?argcoinstr:"ALL");
                    printf("call process_ramchain.(%s)\n",Ramchains[i]->name);
                    if ( portable_thread_create((void *)_process_ramchain,Ramchains[i]) == 0 )
                        printf("ERROR _process_ramchain.%s\n",Ramchains[i]->name);
                    processed--;
                }
                else
                {
                    for (pass=1; pass<=4; pass++)
                        processed += process_ramchain(Ramchains[i],Ramchains[i]->mappedblocks[pass],Ramchains[i]->mappedblocks[pass-1],1000.,ram_millis());
                }
            }
        }
        if ( processed == 0 )
        {
            for (i=0; i<Numramchains; i++)
                if ( argcoinstr == 0 || strcmp(argcoinstr,Ramchains[i]->name) == 0 )
                {
                    Ramchains[i]->blocks.blocknum = Ramchains[i]->blocks.contiguous = ram_setcontiguous(&Ramchains[i]->blocks);
                    ram_disp_status(Ramchains[i]);
                }
            sleep(10);
        }
    }
    printf("process_coinblocks: finished launching\n");
    while ( 1 )
        sleep(60);
}

#endif
#endif
