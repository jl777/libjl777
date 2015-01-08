//
//  ramchain
//  SuperNET
//
//  by jl777 on 12/29/14.
//  MIT license

#define HUFF_GENMODE

#ifdef INCLUDE_DEFINES
#ifndef ramchain_h
#define ramchain_h

#include <stdint.h>
#include "uthash.h"

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

struct huffstream { uint8_t *ptr,*buf; int32_t bitoffset,maski,endpos,allocsize; };
typedef struct huffstream HUFF;

#define MAX_BLOCKTX 16384
struct rawvin { char txidstr[128]; uint16_t vout; };
struct rawvout { char coinaddr[64],script[128]; uint64_t value; };
struct rawtx { uint16_t firstvin,numvins,firstvout,numvouts; char txidstr[128]; };

struct rawblock
{
    uint16_t numtx,numrawvins,numrawvouts;
    uint32_t blocknum;
    uint64_t minted;
    struct rawtx txspace[MAX_BLOCKTX];
    struct rawvin vinspace[MAX_BLOCKTX];
    struct rawvout voutspace[MAX_BLOCKTX];
};

#define MAX_HUFFBITS 5
struct huffbits { uint64_t numbits:MAX_HUFFBITS,rawind:(32-MAX_HUFFBITS),bits:32; };
struct huffpayload { struct address_entry B; uint64_t value; uint32_t rawind,vout; };

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

#define HUFF_NUMGENS 2
struct compressionvars
{
    struct rawblock raw,raws[HUFF_NUMGENS];
    HUFF *hps[HUFF_NUMGENS]; uint8_t *rawbits[HUFF_NUMGENS];
    FILE *rawfp,*bitfps[HUFF_NUMGENS];
    struct huffhash hash[0x100];
    uint32_t maxitems,maxblocknum,firstblock,blocknum,processed,firstvout,firstvin;
    char *disp,coinstr[64];
    double startmilli;
};

#endif
#endif


#ifdef INCLUDE_CODE
#ifndef ramchain_code_h
#define ramchain_code_h
static uint8_t huffmasks[8] = { (1<<0), (1<<1), (1<<2), (1<<3), (1<<4), (1<<5), (1<<6), (1<<7) };
static uint8_t huffoppomasks[8] = { ~(1<<0), ~(1<<1), ~(1<<2), ~(1<<3), ~(1<<4), ~(1<<5), ~(1<<6), (uint8_t)~(1<<7) };

// HUFF bitstream functions, uses model similar to fopen/fread/fwrite/fseek but for bitstream

void hclose(HUFF *hp)
{
    if ( hp != 0 )
        free(hp);
}

HUFF *hopen(uint8_t *bits,int32_t num)
{
    HUFF *hp = calloc(1,sizeof(*hp));
    hp->ptr = hp->buf = bits;
    if ( (num & 7) != 0 )
        num++;
    hp->allocsize = num;
    return(hp);
}

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

int32_t hseek(HUFF *hp,int32_t offset,int32_t mode)
{
    if ( mode == SEEK_END )
        offset += hp->endpos;
    if ( offset >= 0 && (offset>>3) < hp->allocsize )
        hp->bitoffset = offset, _hseek(hp);
    else
    {
        printf("hseek.%d: illegal offset.%d %d >= allocsize.%d\n",mode,offset,offset>>3,hp->allocsize);
        return(-1);
    }
    return(0);
}

int32_t hgetbit(HUFF *hp)
{
    int32_t bit = 0;
    if ( hp->bitoffset < hp->endpos )
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
    return(numbits);
}

int32_t emit_varbits(HUFF *hp,uint32_t val)
{
    int32_t numbits = 0;
    if ( val >= 6 )
    {
        if ( val < 0x100 )
        {
            numbits += emit_bits(hp,6,3);
            numbits += emit_bits(hp,val,8);
        }
        else
        {
            numbits += emit_bits(hp,7,3);
            numbits += emit_bits(hp,val,16);
        }
    } else numbits += emit_bits(hp,val,3);
    return(numbits);
}

int32_t decode_varbits(uint32_t *valp,HUFF *hp)
{
    int32_t val;
    val = (hgetbit(hp) | (hgetbit(hp) << 1) | (hgetbit(hp) << 2));
    if ( val < 6 )
    {
        *valp = val;
        return(3);
    }
    else if ( val == 6 )
        return(decode_bits(valp,hp,8));
    else return(decode_bits(valp,hp,16));
}

int32_t emit_valuebits(HUFF *hp,uint64_t value)
{
    int32_t i,numbits = 0;
    for (i=0; i<4; i++,value>>=16)
        numbits += emit_varbits(hp,value & 0xffff);
    return(numbits);
}

int32_t decode_valuebits(uint64_t *valuep,HUFF *hp)
{
    uint32_t i,tmp[4],numbits = 0;
    for (i=0; i<4; i++)
        numbits += decode_varbits(&tmp[i],hp);
    *valuep = (tmp[0] | (tmp[1] << 16) | ((uint64_t)tmp[2] << 32) | ((uint64_t)tmp[3] << 48));
    return(numbits);
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
    return(numbytes + len);
}

int32_t hload(HUFF *hp,FILE *fp)
{
    long load_varint(uint64_t *x,FILE *fp);
    uint64_t endbitpos;
    int32_t len;
    if ( load_varint(&endbitpos,fp) <= 0 )
        return(-1);
    if ( (endbitpos >> 3) <= hp->allocsize )
    {
        len = conv_bitlen(endbitpos);
        if ( fread(hp->buf,1,len,fp) != len )
            return(-1);
        hp->endpos = (int32_t)endbitpos;
        return(len);
    }
    printf("varlen.%lld <= hp->allocsize %u\n",(long long)endbitpos,hp->allocsize);
    return(-1);
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
            i = (uint16_t)x;
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

int32_t expand_scriptdata(char *scriptstr,uint8_t *scriptdata,int32_t datalen)
{
    char *prefix,*suffix;
    int32_t mode,n = 0;
    switch ( (mode= scriptdata[n++]) )
    {
        case 's': prefix = "76a914", suffix = "88ac"; break;
        case 'm': prefix = "a9", suffix = "ac"; break;
        case 'r': prefix = "", suffix = "ac"; break;
        case ' ': prefix = "", suffix = ""; break;
        default: printf("unexpected scriptmode.(%d)\n",mode); prefix = "", suffix = ""; break;
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
    if ( strncmp(script,"76a914",6) == 0 && strcmp(script+len-4,"88ac") == 0 )
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
        len = (int32_t)(strlen(script) >> 1);
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
            if ( vlen > 0 && retval > 0 && vlen < sizeof(vout->coinaddr)-1 && (n= fread(vout->coinaddr,1,vlen+1,fp)) == (vlen+1) )
           // if ( (fpos= load_varfilestr(&len,vout->coinaddr,fp,sizeof(vout->coinaddr)-1)) > 0 )
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
            else if ( fwrite(data,1,datalen,fp) != datalen )
                return(-3);
        }
        else if ( emit_varint(fp,0) <= 0 )
            return(-4);
        if ( emit_varint(fp,len) <= 0 )
            return(-5);
        else if ( len > 0 && fwrite(vout->coinaddr,1,len+1,fp) != len+1 )
            return(-6);
        else if ( len <= 0 )
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
        else if ( fwrite(data,1,len,fp) != len )
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
        if ( len <= 0 )
            return(-1);
        decode_hex(data,(int32_t)len,tx->txidstr);
        if ( emit_varint(fp,len) <= 0 )
            return(-1);
        else if ( fwrite(data,1,len,fp) != len )
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

long get_endofdata(long *eofposp,FILE *fp)
{
    long endpos;
    fseek(fp,0,SEEK_END);
    *eofposp = ftell(fp);
    fseek(fp,-sizeof(uint64_t),SEEK_END);
    endpos = ftell(fp);
    rewind(fp);
    return(endpos);
}

/*int32_t check_for_blockcheck(FILE *fp)
{
    long fpos;
    uint64_t blockcheck;
    fpos = ftell(fp);
    fread(&blockcheck,1,sizeof(blockcheck),fp);
    if ( (uint32_t)(blockcheck >> 32) == ~(uint32_t)blockcheck )
        return(1);
    fseek(fp,fpos,SEEK_SET);
    return(0);
}*/


uint32_t load_blockcheck(FILE *fp)
{
    long fpos;
    uint64_t blockcheck;
    uint32_t blocknum = 0;
    fseek(fp,0,SEEK_END);
    fpos = ftell(fp);
    if ( fpos >= sizeof(blockcheck) )
    {
        fpos -= sizeof(blockcheck);
        fseek(fp,fpos,SEEK_SET);
    } else rewind(fp);
    if ( fread(&blockcheck,1,sizeof(blockcheck),fp) != sizeof(blockcheck) || (uint32_t)(blockcheck >> 32) != ~(uint32_t)blockcheck )
    {
        printf(">>>>>>>>>> ERROR marker blocknum %llx -> %u endpos.%ld fpos.%ld\n",(long long)blockcheck,blocknum,fpos,ftell(fp));
        blocknum = 0;
        fseek(fp,fpos,SEEK_SET);
    }
    else
    {
        blocknum = (uint32_t)blockcheck;
        fpos = ftell(fp);
        fseek(fp,fpos-sizeof(blockcheck),SEEK_SET);
        //printf("found valid marker blocknum %llx -> %u endpos.%ld fpos.%ld\n",(long long)blockcheck,blocknum,fpos,ftell(fp));
    }
    return(blocknum);
}

long emit_blockcheck(FILE *fp,uint64_t blocknum)
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
        //printf("emit_blockcheck %d -> %ld\n",(uint32_t)blocknum,ftell(fp));
        load_blockcheck(fp);
    }
    return(retval);
}

int32_t save_rawblock(int32_t dispflag,FILE *fp,struct rawblock *raw,uint32_t blocknum)
{
    uint32_t checkblocknum = load_blockcheck(fp);
    if ( blocknum <= checkblocknum+1 && raw->numtx < MAX_BLOCKTX && raw->numrawvins < MAX_BLOCKTX  && raw->numrawvouts < MAX_BLOCKTX )
    {
        if ( fwrite(raw,sizeof(int32_t),6,fp) != 6 )
            printf("fwrite error of header for block.%d\n",raw->blocknum);
        else if ( txspace_io(dispflag,1,fp,raw->txspace,raw->numtx) != 0 )
            printf("fwrite error of txspace[%d] for block.%d\n",raw->numtx,raw->blocknum);
        else if ( vinspace_io(dispflag,1,fp,raw->vinspace,raw->numrawvins) != 0 )
            printf("fwrite error of vinspace[%d] for block.%d\n",raw->numrawvins,raw->blocknum);
        else if ( voutspace_io(dispflag,1,fp,raw->voutspace,raw->numrawvouts) != 0 )
            printf("fwrite error of voutspace[%d] for block.%d\n",raw->numrawvouts,raw->blocknum);
        else if ( emit_blockcheck(fp,raw->blocknum) <= 0 )
            printf("emit_blockcheck error for block.%d\n",raw->blocknum);
        else return(0);
    } else printf("error save_rawblock blocknum.%d vs raw.%d vs checkblock.%u\n",blocknum,raw->blocknum,checkblocknum);
    return(-1);
}

int32_t load_rawblock(int32_t dispflag,FILE *fp,struct rawblock *raw,uint32_t expectedblock,long endpos)
{
    int32_t i;
    if ( fread(raw,sizeof(int32_t),6,fp) != 6 )
        printf("fread error of header for block.%d expected.%d\n",raw->blocknum,expectedblock);
    else
    {
        if ( dispflag != 0 )
            printf("block.%d: numtx.%d numrawvins.%d numrawvouts.%d minted %.8f\n",raw->blocknum,raw->numtx,raw->numrawvins,raw->numrawvouts,dstr(raw->minted));
        if ( expectedblock == raw->blocknum )
        {
            if ( raw->numtx < MAX_BLOCKTX && txspace_io(dispflag,0,fp,raw->txspace,raw->numtx) != 0 )//fread(raw->txspace,sizeof(*raw->txspace),raw->numtx,fp) != raw->numtx )
                printf("fread error of txspace[%d] for block.%d\n",raw->numtx,raw->blocknum);
            else
            {
                if ( dispflag != 0 )
                {
                    for (i=0; i<raw->numtx; i++)
                    printf("txind.%d: vins.(%d %d) vouts.(%d %d)\n",i,raw->txspace[i].firstvin,raw->txspace[i].numvins,raw->txspace[i].firstvout,raw->txspace[i].numvouts);
                }
                if ( raw->numrawvins < MAX_BLOCKTX && vinspace_io(dispflag,0,fp,raw->vinspace,raw->numrawvins) != 0 )
                    printf("read error of vinspace[%d] for block.%d\n",raw->numrawvins,raw->blocknum);
                else if ( raw->numrawvouts < MAX_BLOCKTX && voutspace_io(dispflag,0,fp,raw->voutspace,raw->numrawvouts) != 0 )
                    printf("read error of voutspace[%d] for block.%d\n",raw->numrawvouts,raw->blocknum);
                else return(0);
            }
        } else printf("fread error block.%d expected.%d\n",raw->blocknum,expectedblock);
    }
    return(-1);
}

FILE *_open_varsfile(int32_t readonly,uint32_t *blocknump,char *fname,char *coinstr)
{
    FILE *fp = 0;
    if ( readonly != 0 )
    {
        if ( (fp = fopen(fname,"rb")) != 0 )
        {
            fseek(fp,0,SEEK_END);
            *blocknump = load_blockcheck(fp);
            printf("opened %s blocknum.%d\n",fname,*blocknump);
        }
    }
    else
    {
        if ( (fp = fopen(fname,"rb+")) == 0 )
        {
            if ( (fp = fopen(fname,"wb")) == 0 )
            {
                printf("couldnt create (%s)\n",fname);
                exit(-1);
            }
            printf("created %s -> fp.%p\n",fname,fp);
            *blocknump = 0;
            emit_blockcheck(fp,*blocknump);
            fclose(fp);
            fp = fopen(fname,"rb+");
        }
        fseek(fp,0,SEEK_END);
        *blocknump = load_blockcheck(fp);
        printf("opened %s blocknum.%d\n",fname,*blocknump);
    }
    return(fp);
}

int32_t rawblockcmp(struct rawblock *ref,struct rawblock *raw)
{
    int32_t i;
    if ( ref->numtx != raw->numtx )
        return(-1);
    if ( ref->numrawvins != raw->numrawvins )
        return(-2);
    if ( ref->numrawvouts != raw->numrawvouts )
        return(-3);
    if ( ref->numtx != 0 && memcmp(ref->txspace,raw->txspace,ref->numtx*sizeof(*raw->txspace)) != 0 )
    {
        struct rawtx *reftx,*rawtx;
        for (i=0; i<ref->numtx; i++)
        {
            reftx = &ref->txspace[i];
            rawtx = &raw->txspace[i];
            if ( reftx->firstvin != rawtx->firstvin || reftx->firstvout != reftx->firstvout || reftx->numvins != rawtx->numvins || reftx->numvouts != reftx->numvouts || strcmp(reftx->txidstr,rawtx->txidstr) != 0 )
            {
                printf("1st.%d %d, %d %d (%s) vs 1st %d %d, %d %d (%s)\n",reftx->firstvin,reftx->firstvout,reftx->numvins,reftx->numvouts,reftx->txidstr,reftx->firstvin,reftx->firstvout,rawtx->numvins,rawtx->numvouts,reftx->txidstr);
                break;
            }
        }
        if ( i != ref->numtx )
            return(-4);
    }
    if ( ref->numrawvins != 0 && memcmp(ref->vinspace,raw->vinspace,ref->numrawvins*sizeof(*raw->vinspace)) != 0 )
    {
        return(-5);
    }
    if ( ref->numrawvouts != 0 && memcmp(ref->voutspace,raw->voutspace,ref->numrawvouts*sizeof(*raw->voutspace)) != 0 )
    {
        struct rawvout *reftx,*rawtx;
        for (i=0; i<ref->numrawvouts; i++)
        {
            reftx = &ref->voutspace[i];
            rawtx = &raw->voutspace[i];
            printf("%d of %d: (%s) (%s) %.8f vs (%s) (%s) %.8f\n",i,ref->numrawvouts,reftx->coinaddr,reftx->script,dstr(reftx->value),rawtx->coinaddr,rawtx->script,dstr(rawtx->value));
            if ( reftx->value != rawtx->value || strcmp(reftx->coinaddr,rawtx->coinaddr) != 0 || strcmp(reftx->script,rawtx->script) != 0 )
            {
                break;
            }
        }
        if ( i != ref->numrawvouts )
            return(-6);
    }
    return(0);
}

int32_t huffpair_decodeitemind(int32_t bitflag,uint32_t *rawindp,struct huffpair *pair,HUFF *hp)
{
    uint32_t ind;
    int32_t i,c,*tree,tmp,numitems,depth,num = 0;
    *rawindp = 0;
    if ( pair == 0 )
        return(-1);
    if ( bitflag == 0 )
        return(decode_varbits(rawindp,hp));
    if ( pair->code == 0 || pair->code->tree == 0 )
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

int32_t load_bitstream_block(HUFF *hp,FILE *fp)
{
    int32_t numbytes;
    if ( fp == 0 )
    {
        printf("load_bitstream_block null fp\n");
        return(-1);
    }
    numbytes = hload(hp,fp);
    return(numbytes);
}

int32_t save_bitstream_block(FILE *fp,HUFF *hp)
{
    int32_t numbytes;
    if ( fp == 0 )
    {
        printf("save_bitstream_block null fp\n");
        return(-1);
    }
    numbytes = hflush(fp,hp);
    return(numbytes);
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

void *huffhash_mapfile(struct huffhash *hash)
{
    void *init_mappedptr(void **ptrp,struct mappedptr *mp,uint64_t allocsize,int32_t rwflag,char *fname);
    char fname[1024];
    long offset;
    void *ptr;
    uint64_t datalen;
    int32_t rwflag = 0;
    uint32_t ind,duplicates = 0;
    struct huffpair_hash *hp;
    hash->ind = ind;
    huffhash_setfname(fname,hash,0);
    if ( init_mappedptr(0,&hash->M,0,rwflag,fname) == 0 )
        return(0);
    printf("%s %p %llu\n",fname,hash->M.fileptr,hash->M.allocsize);
    while ( (offset= decode_varint(&datalen,hash->M.fileptr,offset,hash->M.allocsize)) > 0 )
    {
        printf("offset.%ld datalen.%d\n",offset,(int)datalen);
        if ( (offset+datalen) > hash->M.allocsize )
        {
            printf("offset.%ld + datalen.%ld > allocsize %ld for %s\n",offset,(long)datalen,(long)hash->M.allocsize,fname);
            break;
        }
        ptr = (void *)((long)hash->M.fileptr + offset);
        HASH_FIND(hh,hash->table,ptr,datalen,hp);
        if ( hp == 0 )
            huffhash_add(hash,hp,ptr,(int32_t)datalen);
        else duplicates++;
    }
    printf("loaded %d, duplicates.%d from %s\n",hash->ind,duplicates,fname);
    return(hash->M.fileptr);
}

int32_t copyfile(FILE *destfp,FILE *srcfp) // horrible inefficient for now
{
    int c,n = 0;
    while ( (c= fgetc(srcfp)) >= 0 )
        fputc(c,destfp), n++;
    return(n);
}

FILE *huffhash_combine_files(struct huffhash *hash)
{
    FILE *newfp,*fp;
    char fname[1024],newfname[1024];
    huffhash_setfname(fname,hash,0);
    huffhash_setfname(newfname,hash,1);
    fp = fopen(fname,"rb+");
    newfp = fopen(newfname,"rb");
    if ( fp != 0 )
    {
        if ( newfp == 0 )
        {
            fclose(fp);
            printf("return A create(%s)\n",newfname);
            return(fopen(newfname,"wb"));
        }
        fseek(fp,0,SEEK_END);
        copyfile(fp,newfp);
        fclose(fp);
        fclose(newfp);
    }
    else if ( newfp != 0 ) // actually invalid case
    {
        if ( (fp= fopen(fname,"wb")) != 0 )
        {
            copyfile(fp,newfp);
            fclose(fp);
        }
        fclose(newfp);
    }
    printf("return create(%s)\n",newfname);
    return(fopen(newfname,"wb"));
}

struct huffpair_hash *huffhash_get(int32_t *createdflagp,struct huffhash *hash,uint8_t *ptr,int32_t datalen)
{
    void *newptr;
    uint8_t buf[9],varlen;
    struct huffpair_hash *hp;
    if ( hash->newfp == 0 )
    {
        if ( (hash->newfp= huffhash_combine_files(hash)) == 0 )
        {
            printf("error mapping %s type.%d hashfile  or creating newfile.%p\n",hash->coinstr,hash->type,hash->newfp);
            exit(1);
        }
        //huffhash_mapfile(hash);
    }
    varlen = calc_varint(buf,datalen);
    memcpy(&ptr[-varlen],buf,varlen);
    HASH_FIND(hh,hash->table,&ptr[-varlen],datalen+varlen,hp);
    if ( 0 )
    {
        char hexbytes[8192];
        init_hexbytes_noT(hexbytes,&ptr[-varlen],datalen+varlen);
        printf("search.(%s)| varlen.%d datalen.%d checkhp.%p\n",hexbytes,varlen,datalen,hp);
    }
    *createdflagp = 0;
    if ( hp == 0 )
    {
        hp = calloc(1,sizeof(*hp));
        newptr = malloc(varlen + datalen);
        memcpy(newptr,buf,varlen);
        memcpy((void *)((long)newptr + varlen),ptr,datalen);
        *createdflagp = 1;
        if ( hash->newfp != 0 )
        {
            if ( 0 )
            {
                char hexstr[8192];
                init_hexbytes_noT(hexstr,newptr,datalen+varlen);
                printf("save.(%s) at %ld datalen.%d varlen.%d\n",hexstr,ftell(hash->newfp),datalen,varlen);
            }
            if ( fwrite(newptr,1,datalen+varlen,hash->newfp) != (datalen+varlen) )
            {
                printf("error saving type.%d ind.%d datalen.%d varlen.%d\n",hash->type,hash->ind,datalen,varlen);
                exit(-1);
            }
            fflush(hash->newfp);
        }
        huffhash_add(hash,hp,newptr,datalen+varlen);
    }
    return(hp);
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

int32_t huffpair_gencode(struct compressionvars *V,struct huffpair *pair,int32_t frequi)
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
        set_compressionvars_fname(0,fname,V->coinstr,pair->name,-1);
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

int32_t huffpair_gentxcode(struct compressionvars *V,struct rawtx_huffs *tx,int32_t frequi)
{
    if ( huffpair_gencode(V,&tx->numvins,frequi) != 0 )
        return(-1);
    else if ( huffpair_gencode(V,&tx->numvouts,frequi) != 0 )
        return(-2);
    else if ( huffpair_gencode(V,&tx->txid,frequi) != 0 )
        return(-3);
   else return(0);
}

int32_t huffpair_genvincode(struct compressionvars *V,struct rawvin_huffs *vin,int32_t frequi)
{
    if ( huffpair_gencode(V,&vin->txid,frequi) != 0 )
        return(-1);
    else if ( huffpair_gencode(V,&vin->vout,frequi) != 0 )
        return(-2);
    else return(0);
}

int32_t huffpair_genvoutcode(struct compressionvars *V,struct rawvout_huffs *vout,int32_t frequi)
{
    if ( huffpair_gencode(V,&vout->addr,frequi) != 0 )
        return(-1);
    else if ( huffpair_gencode(V,&vout->script,frequi) != 0 )
        return(-2);
    return(0);
}

int32_t huffpair_gencodes(struct compressionvars *V,struct rawblock_huffs *H,int32_t frequi)
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
        else if ( (preds->vouti_addr= &H->vouti.addr) == 0 )
            return(-21);
        else if ( (preds->voutn_addr= &H->voutn.addr) == 0 )
            return(-22);
        else if ( (preds->vout0_script= &H->vout0.script) == 0 )
            return(-23);
        else if ( (preds->vout1_script= &H->vout1.script) == 0 )
            return(-24);
        else if ( (preds->vouti_script= &H->vouti.script) == 0 )
            return(-25);
        else if ( (preds->voutn_script= &H->voutn.script) == 0 )
            return(-26);
    }
    return(0);
}

int32_t huffpair_conv(char *strbuf,uint64_t *valp,struct compressionvars *V,char type,uint32_t rawind);
uint32_t expand_huffint(int32_t bitflag,struct compressionvars *V,struct huffpair *pair,HUFF *hp)
{
    uint32_t rawind; int32_t numbits;
    if ( (numbits= huffpair_decodeitemind(bitflag,&rawind,pair,hp)) < 0 )
        return(-1);
   // printf("%s.%d ",pair->name,rawind);
    return(rawind);
}

int32_t expand_huffstr(int32_t bitflag,char *deststr,int32_t type,struct compressionvars *V,struct huffpair *pair,HUFF *hp)
{
    uint64_t destval;
    uint32_t rawind;
    int32_t numbits;
    deststr[0] = 0;
    if ( (numbits= huffpair_decodeitemind(bitflag,&rawind,pair,hp)) < 0 )
        return(-1);
    if ( rawind == 0 )
        return(0);
    //fprintf(stderr,"bitflag.%d type.%d %s rawind.%d\n",bitflag,type,pair->name,rawind);
    huffpair_conv(deststr,&destval,V,type,rawind);
    return((uint16_t)destval);
}

int32_t expand_rawbits(int32_t bitflag,struct compressionvars *V,struct rawblock *raw,HUFF *hp,struct rawblock_preds *preds)
{
    struct huffpair *pair,*pair2,*pair3;
    int32_t txind,numrawvins,numrawvouts,vin,vout;
    struct rawtx *tx; struct rawvin *vi; struct rawvout *vo;
    memset(raw,0,sizeof(*raw));
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
                        expand_huffstr(bitflag,vo->coinaddr,'a',V,pair,hp);
                        expand_huffstr(bitflag,vo->script,'s',V,pair2,hp);
                        decode_valuebits(&vo->value,hp);
                    }
                }
            }
            else
            {
                printf("overflow during expand_bitstream: numrawvins %d <= %d raw->numrawvins && numrawvouts %d <= %d raw->numrawvouts\n",numrawvins,raw->numrawvins,numrawvouts,raw->numrawvouts);
                return(-1);
            }
        }
        if ( numrawvins != raw->numrawvins || numrawvouts != raw->numrawvouts )
        {
            printf("overflow during expand_bitstream: numrawvins %d <= %d raw->numrawvins && numrawvouts %d <= %d raw->numrawvouts\n",numrawvins,raw->numrawvins,numrawvouts,raw->numrawvouts);
            return(-1);
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
        printf("huffpair_rawind: type.%d val.%d overflow for 16 bits\n",type,val);
        exit(-1);
    }
    return(s);
}

int32_t huffpair_conv(char *strbuf,uint64_t *valp,struct compressionvars *V,char type,uint32_t rawind)
{
    uint8_t *data;
    uint64_t datalen;
    int32_t scriptmode;
    if ( type == 2 )
    {
        *valp = _decode_rawind(type,rawind);
        return(0);
    }
   // printf("V.%p type.%c rawind.%d ptrs.%p\n",V,type,rawind,V->hash[type].ptrs);
    if ( rawind == 0xffffffff )
        while ( 1 )
            sleep(1);
    data = V->hash[type].ptrs[rawind];
    data += decode_varint(&datalen,data,0,9);
    *valp = rawind;
    if ( type == 's' )
    {
        if ( rawind == 0 )
        {
            strbuf[0] = 0;
            *valp = 0;
            return(0);
        }
        if ( (scriptmode= expand_scriptdata(strbuf,data,(uint32_t)datalen)) < 0 )
        {
            printf("huffpair_rawind: scriptmode.%d for (%s)\n",scriptmode,strbuf);
            exit(-1);
        }
        //printf("EXPANDSCRIPT.(%c) -> [%s]\n",scriptmode,strbuf);
    }
    else if ( type == 't' )
    {
        if ( datalen > 128 )
        {
            printf("huffpair_rawind: type.%d datalen.%d > sizeof(data) %d\n",type,(int)datalen,128);
            exit(-1);
        }
        init_hexbytes_noT(strbuf,data,datalen);
    }
    else if ( type == 'a' )
        memcpy(strbuf,data,datalen);
    else
    {
        printf("huffpair_rawind: unsupported type.%d\n",type);
        exit(-1);
    }
    return(0);
}

void *update_coinaddr_unspent(uint32_t *nump,struct huffpayload *payloads,struct huffpayload *payload)
{
    int32_t i,num = *nump;
    struct address_entry *bp = &payload->B;
    if ( num > 0 )
    {
        for (i=0; i<num; i++)
            if ( memcmp(&payloads[i].B,bp,sizeof(*bp)) == 0 )
            {
                printf("harmless duplicate unspent (%d %d %d)\n",bp->blocknum,bp->txind,bp->v);
                return(payloads);
            }
    } else i = 0;
    if ( i == num )
    {
        (*nump) = ++num;
        payloads = realloc(payloads,sizeof(*payloads) * num);
        payloads[i] = *payload;
    }
    return(payloads);
}

int32_t update_payload(struct address_entry *bps,int32_t slot,struct address_entry *bp)
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
            printf("harmless duplicate spent.%d slot.%d (%d %d %d)\n",bp->spent,slot,bp->blocknum,bp->txind,bp->v);
        else printf("ERROR duplicate spent.%d slot.%d txid (%d %d %d) vs (%d %d %d)\n",bp->spent,slot,bps[slot].blocknum,bps[slot].txind,bps[slot].v,bp->blocknum,bp->txind,bp->v);
    } else printf("null bps for slot.%d (%d %d %d)\n",slot,bp->blocknum,bp->txind,bp->v);
    return(-1);
}

void *update_txid_payload(uint32_t *nump,struct address_entry *bps,struct huffpayload *payload)
{
    int32_t num = *nump;
    struct address_entry *bp = &payload->B;
    if ( num > 0 )
    {
        if ( bp->spent == 0 )
        {
            update_payload(bps,0,bp);
            return(bps);
        }
        if ( payload->vout <= bps[0].v ) // maintain array of spends
            update_payload(bps,payload->vout+1,bp);
        else printf("update_txid_payload: extra.%d >= bps[0].v %d\n",payload->vout,bps[0].v);
    }
    else
    {
        if ( bp->spent == 0 )
        {
            if ( bps == 0 )
            {
                (*nump) = (payload->vout + 1);
                bps = calloc(payload->vout+1,sizeof(*bps));
            }
            update_payload(bps,0,bp); // first slot is (block, txind, numvouts) of txid
        }
        else printf("unexpected spend for extra.%d (%d %d %d) before the unspent\n",payload->vout,bp->blocknum,bp->txind,bp->v);
    }
    return(bps);
}

uint32_t huffpair_rawind(struct compressionvars *V,char type,char *str,uint64_t val,struct huffpayload *payload)
{
    uint8_t data[4096];
    char buf[8192];
    struct huffpair_hash *hp;
    int32_t createdflag,datalen,scriptmode = 0;
    if ( type == 2 )
        return(conv_to_rawind(type,(uint32_t)val));
    else if ( type == 8 )
        return(0);
    else
    {
        if ( type == 's' )
        {
            if ( str[0] == 0 )
                return(0);
            strcpy(buf,str);
            if ( (scriptmode = calc_scriptmode(&datalen,&data[9],buf,1)) < 0 )
            {
                printf("huffpair_rawind: scriptmode.%d for (%s)\n",scriptmode,str);
                exit(-1);
            }
        }
        else if ( type == 't' )
        {
            datalen = (int32_t)(strlen(str) >> 1);
            if ( datalen > sizeof(data) )
            {
                printf("huffpair_rawind: type.%d datalen.%d > sizeof(data) %ld\n",type,(int)datalen,sizeof(data));
                exit(-1);
            }
            decode_hex(&data[9],datalen,str);
        }
        else if ( type == 'a' )
        {
            datalen = (int32_t)strlen(str) + 1;
            memcpy(&data[9],str,datalen);
        }
        else
        {
            printf("huffpair_rawind: unsupported type.%d\n",type);
            exit(-1);
        }
        hp = huffhash_get(&createdflag,&V->hash[type],&data[9],datalen);
        if ( val != 0 )
        {
            if ( type == 's' )
                hp->numpayloads = (uint32_t)val;
            else if ( type == 'a' )
                hp->payloads = update_coinaddr_unspent(&hp->numpayloads,hp->payloads,payload);
            else if ( type == 't' )
                hp->payloads = update_txid_payload(&hp->numpayloads,hp->payloads,payload);
        }
        return(hp->rawind);
    }
}

void update_pair_fields(int32_t iter,int32_t type,struct huffpair *pair,char *name,uint32_t rawind)
{
    if ( iter == 0 ) // just get max ranges, but hash tables are also being created
    {
        if ( pair->name[0] == 0 )
            strcpy(pair->name,name);
        if ( pair->wt == 0 )
            pair->wt = (type == 0) ? 4 : type;
        if ( rawind > pair->maxind )
            pair->maxind = rawind;
    }
}

int32_t update_huffpair(struct compressionvars *V,int32_t iter,char type,char *destbuf,uint64_t *destvalp,HUFF *hp,struct huffpair *pair,char *name,char *str,uint64_t val,struct huffpayload *payload)
{
    int32_t calciters = 2;
    struct huffitem *item;
    uint32_t rawind = 0;
    int32_t numbits = 0;
    *destvalp = 0;
    if ( type == 8 )
    {
        update_pair_fields(iter,type,pair,name,rawind);
        if ( iter <= calciters )
            return(emit_valuebits(hp,val));
        else return(decode_valuebits(destvalp,hp));
    }
    else if ( type == 2 )
        rawind = (val & 0xffff);
    else rawind = huffpair_rawind(V,type,str,val,payload);
    update_pair_fields(iter,type,pair,name,rawind);
    if ( iter < calciters )
        numbits = emit_varbits(hp,rawind);
    if ( iter == 1 ) // initialize and create huffitems
    {
        if ( pair->items == 0 )
            pair->items = calloc(pair->maxind+1,sizeof(*pair->items));
        else if ( rawind > pair->maxind )
        {
            printf("illegal case rawind.%d > maxind.%d\n",rawind,pair->maxind);
            exit(-1);
        }
        item = &pair->items[rawind];
        if ( item->freq[0]++ == 0 )
        {
            item->code.rawind = rawind;
            pair->nonz++;
        }
    }
    else if ( iter >= calciters )
    {
        if ( rawind <= pair->maxind )
        {
            if ( iter == calciters ) // emit bitstream
            {
                item = &pair->items[rawind];
                if ( (numbits= hwrite(item->code.bits,item->code.numbits,hp)) > 0 )
                {
                    huffpair_conv(destbuf,destvalp,V,type,item->code.rawind);
                    return(numbits);
                }
                printf("update_huffpair error hwrite.(%s) rawind.%d\n",name,rawind);
            }
            else // decode bitstream
            {
                if ( (numbits= huffpair_decodeitemind(1,&rawind,pair,hp)) < 0 )
                    return(-1);
                huffpair_conv(destbuf,destvalp,V,type,rawind);
                return(numbits);
            }
        }
        else printf("update_huffpair.(%s) rawind.%d > maxind.%d\n",name,rawind,pair->maxind);
    }
    return(0);
}

void init_bitstream(int32_t dispflag,struct compressionvars *V,FILE *fp,int32_t frequi)
{
    long endpos,eofpos; double avesize,avebits[HUFF_NUMGENS],startmilli = milliseconds();
    int32_t numbits[HUFF_NUMGENS],n[HUFF_NUMGENS],allbits[HUFF_NUMGENS],j,err;
    uint32_t blocknum,txind,vin,vout,checkvins,checkvouts,iter,calciters,geniters,tx_rawind,addr_rawind; uint64_t destval;
    struct rawtx *tx,*tx2; struct rawvin *vi,*vi2; struct rawvout *vo,*vo2; struct rawblock *raw;
    char *str,destbuf[8192];
    struct huffpair *pair; struct rawtx_huffs *txpairs; struct rawvin_huffs *vinpairs; struct rawvout_huffs *voutpairs;
    struct huffpayload payload; struct rawblock_huffs H; struct rawblock_preds P[HUFF_NUMGENS];
    memset(&H,0,sizeof(H));
    memset(P,0,sizeof(P));
    memset(&allbits,0,sizeof(allbits));
    if ( fp != 0 )
    {
        calciters = 2;
        geniters = 1;//HUFF_NUMGENS;
        endpos = get_endofdata(&eofpos,fp);
        for (iter=0; iter<calciters+1; iter++)//calciters+geniters; iter++)
        {
            //dispflag = iter>1;
            rewind(fp);
            raw = &V->raw;
            if ( iter == calciters )
            {
                if ( (err= huffpair_gencodes(V,&H,frequi)) != 0 )
                    printf("error setting gencodes iter.%d err.%d\n",iter,err);
                //getchar();
            }
            for (j=0; j<geniters; j++)
            {
                if ( (err= set_rawblock_preds(&P[j],&H,j)) < 0 )
                    printf("error setting preds iter.%d j.%d err.%d\n",iter,j,err);
            }
            for (j=0; j<geniters; j++)
            {
                if ( V->bitfps[j] != 0 )
                    rewind(V->bitfps[j]);
            }
            for (blocknum=1; blocknum<=V->blocknum; blocknum++)
            {
                if ( dispflag == 0 && (blocknum % 1000) == 0 )
                    fprintf(stderr,".");
                //printf("load block.%d\n",blocknum);
                memset(raw,0,sizeof(*raw));
                if ( load_rawblock(dispflag,fp,raw,blocknum,endpos) != 0 )
                {
                    printf("error loading block.%d\n",blocknum);
                    break;
                }
                if ( dispflag != 0 )
                    printf(">>>>>>>>>>>>> block.%d loaded (T%-2d vi%-2d vo%-2d) { %s.v%d }-> [%s %.8f]\n",raw->blocknum,raw->numtx,raw->numrawvins,raw->numrawvouts,raw->txspace[0].numvins==0?"coinbase":raw->vinspace[raw->txspace[0].firstvin].txidstr,raw->vinspace[raw->txspace[0].firstvin].vout,raw->voutspace[raw->txspace[0].firstvout].coinaddr,dstr(raw->voutspace[raw->txspace[0].firstvout].value));
                for (j=0; j<geniters; j++)
                {
                    hclear(V->hps[j]);
                    numbits[j] = n[j] = 0;
                    if ( iter > calciters ) // calciters is to output to bitfps
                    {
                        memset(&V->raws[j],0,sizeof(V->raws[j]));
                        load_bitstream_block(V->hps[j],V->bitfps[j]);
                    }
                }
                checkvins = checkvouts = 0;
                for (j=0; j<geniters; j++)
                {
                    if ( j == 0 )
                        str = "numtx", pair = &H.numtx;
                    else str = "numall", pair = &H.numall;
                    n[j]++, numbits[j] += update_huffpair(V,iter,2,destbuf,&destval,V->hps[j],pair,str,0,raw->numtx,0), V->raws[j].numtx = (uint32_t)destval;

                    if ( j == 0 )
                        str = "numrawvins", pair = &H.numrawvins;
                    else str = "numall", pair = &H.numall;
                    n[j]++, numbits[j] += update_huffpair(V,iter,2,destbuf,&destval,V->hps[j],pair,str,0,raw->numrawvins,0), V->raws[j].numrawvins = (uint32_t)destval;

                    if ( j == 0 )
                        str = "numrawvouts", pair = &H.numrawvouts;
                    else str = "numall", pair = &H.numall;
                    n[j]++, numbits[j] += update_huffpair(V,iter,2,destbuf,&destval,V->hps[j],pair,str,0,raw->numrawvouts,0), V->raws[j].numrawvouts = (uint32_t)destval;
                }
                if ( raw->numtx > 0 )
                {
                    for (txind=0; txind<raw->numtx; txind++)
                    {
                        tx = &raw->txspace[txind];
                        tx_rawind = 0;
                        for (j=0; j<geniters; j++)
                        {
                            tx2 = &V->raws[j].txspace[txind];
                            if ( j == 0 )
                            {
                                if ( txind == 0 )
                                    str = "tx0", txpairs = &H.tx0;
                                else if ( txind == 1 )
                                    str = "tx1", txpairs = &H.tx1;
                                else str = "txi", txpairs = &H.txi;
                            }
                            else str = "txall", txpairs = &H.txall;
                            if ( txpairs->numvinsname[0] == 0 )
                            {
                                sprintf(txpairs->numvinsname,"%s.numvins",str);
                                sprintf(txpairs->numvoutsname,"%s.numvouts",str);
                                sprintf(txpairs->txidname,"%s.txid",str);
                            }
                            n[j]++, numbits[j] += update_huffpair(V,iter,2,destbuf,&destval,V->hps[j],&txpairs->numvins,txpairs->numvinsname,0,tx->numvins,0), tx2->numvins = (uint32_t)destval;
                            n[j]++, numbits[j] += update_huffpair(V,iter,2,destbuf,&destval,V->hps[j],&txpairs->numvouts,txpairs->numvoutsname,0,tx->numvouts,0), tx2->numvouts = (uint32_t)destval;
                            memset(&payload,0,sizeof(payload)), payload.B.blocknum = blocknum, payload.B.txind = txind, payload.B.v = tx->numvouts;
                            payload.vout = tx->numvouts;
                            n[j]++, numbits[j] += update_huffpair(V,iter,'t',tx2->txidstr,&destval,V->hps[j],&txpairs->txid,txpairs->txidname,tx->txidstr,0,&payload);
                            tx_rawind = (uint32_t)destval;
                        }
                        if ( tx->numvins > 0 )
                        {
                            for (vin=0; vin<tx->numvins; vin++)
                            {
                                vi = &raw->vinspace[tx->firstvin + vin];
                                if ( dispflag != 0 )
                                    printf("{ %s, %d } ",vi->txidstr,vi->vout);
                                for (j=0; j<geniters; j++)
                                {
                                    vi2 = &V->raws[j].vinspace[tx->firstvin + vin];
                                    if ( j == 0 )
                                    {
                                        if ( vin == 0 )
                                            str = "vin0", vinpairs = &H.vin0;
                                        else if ( vin == 1 )
                                            str = "vin1", vinpairs = &H.vin1;
                                        else str = "vini", vinpairs = &H.vini;
                                    } else str = "vinall", vinpairs = &H.vinall;
                                    if ( vinpairs->txidname[0] == 0 )
                                    {
                                        sprintf(vinpairs->txidname,"%s.txid",str);
                                        sprintf(vinpairs->voutname,"%s.vout",str);
                                    }
                                    memset(&payload,0,sizeof(payload));
                                    payload.B.blocknum = blocknum, payload.B.txind = txind, payload.B.v = vin, payload.B.spent = 1;
                                    payload.vout = vi->vout, payload.rawind = tx_rawind;
                                    n[j]++, numbits[j] += update_huffpair(V,iter,'t',vi2->txidstr,&destval,V->hps[j],&vinpairs->txid,vinpairs->txidname,vi->txidstr,0,&payload);
                                    
                                    n[j]++, numbits[j] += update_huffpair(V,iter,2,destbuf,&destval,V->hps[j],&vinpairs->vout,vinpairs->voutname,0,vi->vout,0), vi2->vout = (uint32_t)destval;
                                }
                            }
                            checkvins += tx->numvins;
                        }
                        if ( tx->numvouts > 0 )
                        {
                            for (vout=0; vout<tx->numvouts; vout++)
                            {
                                vo = &raw->voutspace[tx->firstvout + vout];
                                if ( dispflag != 0 )
                                    printf("%s[%s, %s, %.8f] ",vout==0?" -> ":"",vo->coinaddr,vo->script,dstr(vo->value));
                                for (j=0; j<geniters; j++)
                                {
                                    vo2 = &V->raws[j].voutspace[tx->firstvout + vout];
                                    if ( j == 0 )
                                    {
                                        if ( vout == 0 )
                                            str = "vout0", voutpairs = &H.vout0;
                                        else if ( vout == 1 )
                                            str = "vout1", voutpairs = &H.vout1;
                                        else if ( vout == 2 )
                                            str = "vout2", voutpairs = &H.vout2;
                                        else if ( vout == tx->numvouts-1 )
                                            str = "voutn", voutpairs = &H.voutn;
                                        else str = "vouti", voutpairs = &H.vouti;
                                    } else str = "voutall", voutpairs = &H.voutall;
                                    if ( voutpairs->addrname[0] == 0 )
                                    {
                                        sprintf(voutpairs->addrname,"%s.addr",str);
                                        sprintf(voutpairs->scriptname,"%s.script",str);
                                        sprintf(voutpairs->valuename,"%s.value",str);
                                    }
                                    memset(&payload,0,sizeof(payload));
                                    payload.B.blocknum = blocknum, payload.B.txind = txind, payload.B.v = vout;
                                    payload.vout = vout, payload.rawind = tx_rawind, payload.value = vo->value;
                                    n[j]++, numbits[j] += update_huffpair(V,iter,'a',vo2->coinaddr,&destval,V->hps[j],&voutpairs->addr,voutpairs->addrname,vo->coinaddr,0,&payload);
                                    addr_rawind = (uint32_t)destval;
                                    n[j]++, numbits[j] += update_huffpair(V,iter,'s',vo2->script,&destval,V->hps[j],&voutpairs->script,voutpairs->scriptname,vo->script,addr_rawind,0);
                                    n[j]++, numbits[j] += update_huffpair(V,iter,8,destbuf,&vo2->value,V->hps[j],&voutpairs->value,voutpairs->valuename,0,vo->value,0);
                                }
                            }
                            checkvouts += tx->numvouts;
                        }
                        if ( dispflag != 0 )
                            printf("(vi.%d vo.%d) | ",tx->numvins,tx->numvouts);
                    }
                }
                if ( checkvins != raw->numrawvins || checkvouts < raw->numrawvouts )
                    printf("ERROR: checkvins %d != %d raw->numrawvins || checkvouts %d != %d raw->numrawvouts\n",checkvins,raw->numrawvins,checkvouts,raw->numrawvouts);
                for (j=0; j<geniters; j++)
                {
                    allbits[j] += numbits[j];
                    avebits[j] = ((double)allbits[j] / (blocknum+1));
                    if ( iter >= calciters )
                    {
                        if ( iter > calciters )
                        {
                            if ( (err= rawblockcmp(raw,&V->raws[j])) != 0 )
                                printf("iter.%d rawblockcmp error block.%d gen.%d err.%d\n",iter,blocknum,j,err);
                        } else save_bitstream_block(V->bitfps[j],V->hps[j]);
                        expand_rawbits(1,V,&V->raws[j],V->hps[j],&P[j]);
                        if ( rawblockcmp(raw,&V->raws[j]) != 0 )
                            printf("iter.%d rawblockcmp2 error block.%d gen.%d\n",iter,blocknum,j);
                    }
                    else if ( iter == 0 )
                    {
                        save_bitstream_block(V->bitfps[j],V->hps[j]);
                        
                        //printf("\n\n>>>>>>>>>>>>>>> start expansion\n");
                        expand_rawbits(0,V,&V->raws[j],V->hps[j],&P[j]);
                        if ( (err= rawblockcmp(raw,&V->raws[j])) != 0 )
                            printf("iter.%d rawblockcmp2 error.%d block.%d gen.%d\n",iter,err,blocknum,j);
                        else printf("block.%d compares\n",blocknum);
                        //printf("<<<<<<<<<<<<<< finished expansion\n\n");
                    }
                }
                avesize = ((double)ftell(fp) / (blocknum+1));
                if ( (blocknum % 100) == 0 || dispflag != 0 )
                {
                    printf("%-5s.%u [%.1f per block est %s] -> ",V->coinstr,blocknum,avesize,_mbstr(V->blocknum * avesize));
                    for (j=0; j<geniters; j++)
                        printf("(%u += %d/%d %.1f R%.2f) ",allbits[j],numbits[j],n[j],(double)numbits[j]/n[j],((double)ftell(fp) / ftell(V->bitfps[j])));
                    printf("\n");
                }
            }
        }
    }
    printf("%.1f seconds to init_bitstream.%s\n",(milliseconds() - startmilli)/1000.,V->coinstr);
#ifndef HUFF_GENMODE
    getchar();
#endif
}

int32_t init_compressionvars(int32_t readonly,struct compressionvars *V,char *coinstr,uint32_t maxblocknum)
{
    struct coin_info *cp = get_coin_info(coinstr);
    uint32_t j,n = 0;
    char fname[512];
    if ( V->rawfp == 0 )
    {
        strcpy(V->coinstr,coinstr);
        V->startmilli = milliseconds();
        if ( (V->maxblocknum= get_blockheight(cp)) == 0 )
            V->maxblocknum = maxblocknum;
        printf("init compression vars.%s: maxblocknum %d %d readonly.%d\n",coinstr,maxblocknum,get_blockheight(cp),readonly);
        V->disp = calloc(1,100000);
        for (j=0; j<0x100; j++)
            strcpy(V->hash[j].coinstr,coinstr), V->hash[j].type = j;
        set_compressionvars_fname(readonly,fname,coinstr,"rawblocks",-1);
//#ifdef HUFF_GENMODE
//        V->rawfp = _open_varsfile(readonly,&V->blocknum,fname,coinstr);
//#else
        V->rawfp = _open_varsfile(readonly,&V->blocknum,fname,coinstr);
        for (j=0; j<HUFF_NUMGENS; j++)
        {
            V->rawbits[j] = calloc(1,1000000);
            V->hps[j] = hopen(V->rawbits[j],1000000);
            set_compressionvars_fname(readonly,fname,coinstr,"bitstream",j);
            V->bitfps[j] = fopen(fname,"wb");//_open_varsfile(0,&blocknum,fname,coinstr);
            printf("(%s).%p ",fname,V->bitfps[j]);
        }
        //if ( V->blocknum == 0 )
        //    V->blocknum = get_blockheight(get_coin_info(coinstr));
        uint32_t decodebits,numbits,tmp,errs = 0;
        uint64_t checkx,x;
        double sum,sum2;
        sum = sum2 = 0;
        for (j=0; j<0x1000; j++)
        {
            hclear(V->hps[0]);
            numbits = emit_varbits(V->hps[0],j);
            sum += numbits;
            hrewind(V->hps[0]);
            decodebits = decode_varbits(&tmp,V->hps[0]);
            if ( tmp != j )
                printf("x.%d numbits.%d -> %d numbits.%d\n",j,numbits,tmp,decodebits), errs++;
      
            x = rand();
            x <<= 32; x ^= rand();
            hclear(V->hps[0]);
            numbits = emit_valuebits(V->hps[0],x);
            sum2 += numbits;
            hrewind(V->hps[0]);
            decodebits = decode_valuebits(&checkx,V->hps[0]);
            if ( checkx != x )
                printf("x.%llx numbits.%d -> %llx numbits.%d\n",(long long)x,numbits,(long long)checkx,decodebits), errs++;
        }
        printf("\nemitbit stats errs.%d | ave %.1f %.1f\n",errs,sum/j,sum2/j);
#ifndef HUFF_GENMODE
        init_bitstream(0,V,V->rawfp,0);
#endif
        printf("rawfp.%p readonly.%d V->blocks.%d (%s) %s bitfps %p %p\n",V->rawfp,readonly,V->blocknum,fname,coinstr,V->bitfps[0],V->bitfps[1]);
    }
    if ( readonly != 0 )
        exit(1);
    return(n);
}


#endif
#endif
