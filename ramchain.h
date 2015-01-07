//
//  ramchain
//  SuperNET
//
//  by jl777 on 12/29/14.
//  MIT license

//#define HUFF_GENMODE

#ifdef INCLUDE_DEFINES
#ifndef ramchain_h
#define ramchain_h

#include <stdint.h>
#include "uthash.h"

#define SETBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] |= (1 << ((bitoffset) & 7)))
#define GETBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] & (1 << ((bitoffset) & 7)))
#define CLEARBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] &= ~(1 << ((bitoffset) & 7)))

struct huffstream { uint8_t *ptr,*buf; int32_t bitoffset,maski,endpos,allocsize; };
typedef struct huffstream HUFF;

union hufftype
{
    bits64 bits;
    void *str;
};

#ifdef HUFF_GENMODE
#define HUFF_NUMFREQS 1
#define HUFF_READONLY 0
#else
#define HUFF_READONLY 1
#define HUFF_NUMFREQS 10
#endif

#define BITSTREAM_UNIQUE (1<<0)
#define BITSTREAM_STRING (1<<1)
#define BITSTREAM_HEXSTR (1<<2)
#define BITSTREAM_COMPRESSED (1<<3)
#define BITSTREAM_STATSONLY (1<<4)
#define BITSTREAM_VALUE (1<<5)
#define BITSTREAM_SCRIPT (1<<6)
#define BITSTREAM_VINS (1<<7)
#define BITSTREAM_VOUTS (1<<8)
#define BITSTREAM_32bits (1<<9)
#define BITSTREAM_64bits (1<<10)

#define MAX_BLOCKTX 16384
struct rawvin { char txidstr[128]; int32_t vout; };
struct rawvout { char coinaddr[64],script[128]; uint64_t value; };
struct rawtx { uint16_t firstvin,numvins,firstvout,numvouts; };

struct rawblock
{
    uint32_t numtx,numrawvins,numrawvouts;
    uint32_t blocknum;
    uint64_t minted;
    struct rawtx txspace[MAX_BLOCKTX];
    struct rawvin vinspace[MAX_BLOCKTX];
    struct rawvout voutspace[MAX_BLOCKTX];
};

struct voutinfo { uint32_t tp_ind,vout,addr_ind,sp_ind; uint64_t value; };
struct address_entry { uint64_t blocknum:32,txind:15,vinflag:1,v:14,spent:1,isinternal:1; };
struct blockinfo { uint32_t firstvout,firstvin; };

struct scriptinfo { uint32_t addrind; char mode; };
struct txinfo { uint64_t blocknum:24,txind:13,numvouts:13,numvins:13; };
union huffinfo
{
    uint8_t c; uint16_t s; uint32_t i; void *ptr;
    uint64_t value;
    struct scriptinfo script;
    struct txinfo tx;
};

#define MAX_HUFFBITS 5
struct huffbits { uint64_t numbits:MAX_HUFFBITS,bits:(28-MAX_HUFFBITS),huffind:28; };

struct huffitem
{
    union huffinfo U;
    UT_hash_handle hh;
    void *ptr;
    uint64_t codebits,space;
    uint32_t huffind,fpos,freq[HUFF_NUMFREQS];
    uint16_t fullsize;
    uint8_t wt,numbits;
    struct huffbits code;
    char str[];
};

struct bitstream_file
{
    struct huffitem *dataptr,**itemptrs;
    FILE *fp;
    struct huffcode *huff;
    long itemsize;
    char fname[1024],coinstr[16],typestr[16],stringflag;
    uint32_t blocknum,ind,checkblock,refblock,mode,huffid,huffwt,maxitems,nomemstructs;
};

struct huffpair { struct huffitem *items; struct huffcode *code; int32_t maxind,nonz,count; char name[16]; };
struct huffpair_hash { UT_hash_handle hh; union huffinfo U; void *ptr; uint32_t hashind; char type;  };

struct rawtx_huffs { struct huffpair numvins,numvouts; char numvinsname[32],numvoutsname[32]; };
struct rawvin_huffs { struct huffpair txid,vout; char txidname[32],voutname[32]; };
struct rawvout_huffs { struct huffpair addr,script,value[4]; char addrname[32],scriptname[32],valuename[4][32]; };
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
    struct huffcode *numtx,*numrawvins,*numrawvouts;
    // (0) and (1) are the first and second places, (n) is the last one, (i) is everything between
    struct huffcode *tx0_numvins,*tx1_numvins,*txi_numvins;
    struct huffcode *tx0_numvouts,*tx1_numvouts,*txi_numvouts;
    
    struct huffcode *vin0_txid,*vin1_txid,*vini_txid;
    struct huffcode *vin0_vout,*vin1_vout,*vini_vout;
    
    struct huffcode *vout0_addr,*vout1_addr,*vout2_addr,*vouti_addr,*voutn_addr;
    struct huffcode *vout0_script,*vout1_script,*vout2_script,*vouti_script,*voutn_script;
    struct huffcode *vout0_value[4],*vout1_value[4],*vout2_value[4],*vouti_value[4],*voutn_value[4];
};

#define HUFF_NUMGENS 2
struct compressionvars
{
    struct rawblock raw,raws[HUFF_NUMGENS];
    struct huffpair_hash *hashptrs[0x100];
    HUFF *hps[HUFF_NUMGENS]; uint8_t *rawbits[HUFF_NUMGENS];
    FILE *rawfp,*bitfps[HUFF_NUMGENS];
    uint32_t hashinds[0x100];
    struct blockinfo prevB;
    uint32_t valuebfp,addrbfp,txidbfp,scriptbfp,voutsbfp,vinsbfp,bitstream,numbfps;
    uint32_t maxitems,maxblocknum,firstblock,blocknum,processed,firstvout,firstvin;
    struct bitstream_file *bfps[16],*numvoutsbfp,*numvinsbfp,*inblockbfp,*txinbfp,*voutbfp,*spgapbfp;
    char *disp,coinstr[64];
    double startmilli;
};

struct huffcode
{
    struct huffitem **items;
    struct compressionvars *V;
    double totalbits,totalbytes,freqsum;
    int32_t numitems,maxbits,numnodes,depth,allocsize;
    int32_t tree[];
};

void *get_compressionvars_item(void *space,int32_t *sizep,struct compressionvars *V,struct huffitem *item);

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

int32_t hflush(FILE *fp,HUFF *hp)
{
    long emit_varint(FILE *fp,uint64_t x);
    uint32_t len;
    int32_t numbytes = 0;
    if ( (numbytes= (int32_t)emit_varint(fp,hp->endpos)) < 0 )
        return(-1);
    len = hp->endpos >> 3;
    if ( (hp->endpos & 7) != 0 )
        len++;
    if ( fwrite(hp->buf,1,len,fp) != len )
        return(-1);
    fflush(fp);
    return(numbytes + len);
}

int32_t hload(HUFF *hp,FILE *fp)
{
    /*long load_varint(uint64_t *x,FILE *fp);
    uint32_t len;
    if ( emit_varint(fp,hp->endpos) < 0 )
        return(-1);
    len = hp->endpos >> 3;
    if ( (hp->endpos & 7) != 0 )
        len++;
    if ( fwrite(hp->buf,1,len,fp) != len )
        return(-1);
    fflush(fp);*/
    return(0);
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
    int32_t n = item->numbits;
    uint64_t codebits;
    if ( n != 0 )
    {
        if ( dispflag != 0 )
        {
            codebits = _reversebits(item->codebits,n);
            printf("%06d: freq.%-6d wt.%d fullsize.%d (%8s).%d\n",item->huffind,item->freq[frequi],item->wt,item->fullsize,huff_str(codebits,n),n);
        }
        return(1);
    }
    return(0);
}

void huff_disp(int32_t verboseflag,struct huffcode *huff,int32_t frequi)
{
    int32_t i,n = 0;
    if ( huff->maxbits >= 1024 )
        printf("huff->maxbits %d >= %d sizeof(strbit)\n",huff->maxbits,1024);
    for (i=0; i<huff->numitems; i++)
        n += huff_dispitem(verboseflag,huff->items[i],frequi);
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
        efreqs[i] = items[i]->freq[frequi] * ((items[i]->wt != 0) ? items[i]->wt : 1);
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

int32_t huff_calc(int32_t dispflag,int32_t *nonzp,double *totalbytesp,double *totalbitsp,int32_t *preds,struct huffitem **items,int32_t numitems,int32_t frequi)
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
                item->codebits = _reversebits(codebits,numbits);
                item->numbits = numbits;
                item->code.numbits = numbits;
                item->code.bits = item->codebits;
                item->code.huffind = item->huffind;
                if ( item->code.numbits != numbits || item->code.bits != item->codebits || item->code.huffind != item->huffind )
                    printf("error setting code.bits: item->code.numbits %d != %d numbits || item->code.bits %u != %llu item->codebits || item->code.huffind %d != %d item->huffind\n",item->code.numbits,numbits,item->code.bits,(long long)item->codebits,item->code.huffind,item->huffind);
                totalbits += numbits * item->freq[frequi];
                totalbytes += item->wt * item->freq[frequi];
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

struct huffcode *huff_create(int32_t dispflag,struct compressionvars *V,struct huffitem **items,int32_t numitems,int32_t frequi)
{
    int32_t ix2,i,n,extf,*preds,allocsize;
    uint32_t *revtree;
    double freqsum;
    struct huffcode *huff;
    if ( (extf= huff_init(&freqsum,&preds,&revtree,items,numitems,frequi)) <= 0 )
        return(0);
    allocsize = (int32_t)(sizeof(*huff) + (sizeof(*huff->tree) * 2 * extf));
    huff = calloc(1,allocsize);
    huff->items = items;
    huff->V = V;
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
    huff->maxbits = huff_calc(dispflag,&huff->numnodes,&huff->totalbytes,&huff->totalbits,preds,items,numitems,frequi);
    free(preds);
    return(huff);
}

int32_t huff_encode(struct huffcode *huff,HUFF *hp,uint32_t *itemis,int32_t num)
{
    uint64_t codebits;
    struct huffitem *item;
    int32_t i,j,n,count = 0;
	for (i=0; i<n; i++)
    {
        item = huff->items[itemis[i]];
        codebits = item->codebits;
        n = item->numbits;
        for (j=0; j<n; j++,codebits>>=1)
            hputbit(hp,codebits & 1);
        count += n;
	}
    return(count);
}

void huff_iteminit(struct huffitem *hip,uint32_t huffind,void *ptr,long fullsize,long wt)
{
    long len;
    if ( fullsize >= (1<<16) || wt >= 0x100 )
    {
        printf("FATAL: huff_iteminit overflow size.%ld vs %d || illegal wt.%ld\n",fullsize,1<<16,wt);
        exit(-1);
        return;
    }
    if ( (hip->fullsize= (uint16_t)fullsize) == 0 )
    {
        len = strlen(ptr);
        if ( len >= (1<<16)-1 )
        {
            printf("FATAL: huff_iteminit overflow len.%ld vs %d\n",len,1<<16);
            exit(-1);
            return;
        }
        hip->ptr = ptr;
        hip->fullsize = (uint16_t)len;
    }
    hip->wt = wt;
    if ( hip->wt != wt )
        printf("huff_iteminit: warning hip->wt %d vs %ld\n",hip->wt,wt);
    hip->codebits = 0;
    hip->numbits = 0;
    hip->huffind = huffind;
}

void update_huffitem(int32_t incr,struct huffitem *hip,uint32_t huffind,void *fullitem,long fullsize,int32_t wt)
{
    int32_t i;
    if ( (huffind&0xf) == 9 )
       printf("update_huffitem.%p rawind.%d type.%d full.%p size.%ld wt.%d incr.%d\n",hip,huffind>>4,huffind&0xf,fullitem,fullsize,wt,incr);
    if ( fullitem != 0 && hip->wt == 0 )
        huff_iteminit(hip,huffind,fullitem,fullsize,wt==0?sizeof(uint32_t):wt);
    if ( incr > 0 )
    {
        for (i=0; i<(int32_t)(sizeof(hip->freq)/sizeof(*hip->freq)); i++)
            hip->freq[i] += incr;
    }
}

void *huff_getitem(void *space,struct compressionvars *V,struct huffcode *huff,int32_t *sizep,uint32_t itemi)
{
    static unsigned char defaultbytes[256];
    struct huffitem *item;
    int32_t i;
    if ( huff != 0 && (item= huff->items[itemi]) != 0 )
        return(get_compressionvars_item(space,sizep,V,item));
    if ( defaultbytes[0xff] != 0xff )
        for (i=0; i<256; i++)
            defaultbytes[i] = i;
    *sizep = 1;
    return(&defaultbytes[itemi & 0xff]);
}

int32_t huff_outputitem(struct compressionvars *V,struct huffcode *huff,uint8_t *output,int32_t num,int32_t maxlen,int32_t ind)
{
    int32_t size;
    const void *ptr;
    uint8_t space[8192];
    ptr = huff_getitem(space,V,huff,&size,ind);
    if ( num+size <= maxlen )
    {
        //printf("%d.(%d %c) ",size,ind,*(char *)ptr);
        memcpy(output + num,ptr,size);
        return(num+size);
    }
    printf("huffoutput error: num.%d size.%d > maxlen.%d\n",num,size,maxlen);
    return(-1);
}

int32_t huff_decodeitem(struct compressionvars *V,struct huffcode *huff,uint8_t *output,int32_t maxlen,HUFF *hp)
{
    int32_t i,c,*tree,ind,numitems,depth,num = 0;
    output[0] = 0;
    numitems = huff->numitems;
    depth = huff->depth;
	while ( 1 )
    {
        tree = huff->tree;
        for (i=c=0; i<depth; i++)
        {
            if ( (c= hgetbit(hp)) < 0 )
                break;
            //printf("%c",c+'0');
            if ( (ind= tree[c]) < 0 )
            {
                tree -= ind;
                continue;
            }
            if ( ind >= numitems )
            {
                printf("decode error: val.%x -> ind.%d >= numitems %d\n",c,ind,numitems);
                return(-1);
            }
            else
            {
                if ( (num= huff_outputitem(V,huff,output,num,maxlen,ind)) < 0 )
                    return(-1);
                //output[num++] = ind;
            }
            break;
        }
        if ( c < 0 )
            break;
	}
    //printf("(%s) huffdecode num.%d\n",output,num);
    return(num);
}

long emit_varint(FILE *fp,uint64_t x)
{
    uint8_t b; uint16_t s; uint32_t i;
    if ( fp == 0 )
        return(-1);
    long retval = -1;
    if ( x < 0xfd )
        b = x, retval = fwrite(&b,1,sizeof(b),fp);
    else
    {
        if ( x <= 0xffff )
        {
            fputc(0xfd,fp);
            s = (uint16_t)x, retval = fwrite(&s,1,sizeof(s),fp);
        }
        else if ( x <= 0xffffffffL )
        {
            fputc(0xfe,fp);
            i = (uint32_t)x, retval = fwrite(&i,1,sizeof(i),fp);
        }
        else
        {
            fputc(0xff,fp);
            retval = fwrite(&x,1,sizeof(x),fp);
        }
    }
    return(retval);
}

int32_t load_varint(uint64_t *valp,FILE *fp)
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
    if ( (retval= load_varint(&len,fp)) > 0 && len < maxlen )
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
    //uint32_t addrhuffind = 0;
    //for (n=0; n<4; n++)
    //    addrhuffind |= scriptdata[n], addrhuffind <<= 8;
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
        strcat(scriptstr,suffix);
    //printf("mode.%d %u (%s)\n",mode,addrhuffind,scriptstr);
    //*addrhuffindp = addrhuffind;
    return(mode);
}

int32_t calc_scriptmode(int32_t *datalenp,uint8_t scriptdata[4096],char *script,int32_t trimflag)
{
    int32_t n=0,len,mode = 0;
    len = (int32_t)strlen(script);
    *datalenp = 0;
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
        //if ( addrhuffind != 0 )
        //    printf("set addrhuffind.%d\n",addrhuffind);
        //for (n=0; n<4; n++)
        //    scriptdata[n] = (addrhuffind & 0xff), addrhuffind >>= 8;
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
            retval = load_varint(&vlen,fp);
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
            return(-7);
        //else printf("script.(%s).%d wrote (%s).%ld %.8f\n",vout->script,datalen,vout->coinaddr,len,dstr(vout->value));
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

int32_t vinspace_io(int32_t dispflag,int32_t saveflag,FILE *fp,struct rawvin *vins,int32_t numrawvins)
{
    int32_t i;
    if ( numrawvins > 0 )
    {
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
        for (i=0; i<numrawvouts; i++)
            if ( (retval= ((saveflag != 0) ? save_rawvout(dispflag,fp,&vouts[i]) : load_rawvout(dispflag,fp,&vouts[i]))) != 0 )
            {
                printf("rawvout.saveflag.%d returns %d\n",saveflag,retval);
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
    }
    return(retval);
}

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
        blocknum = 0;
    else
    {
        blocknum = (uint32_t)blockcheck;
        fpos = ftell(fp);
        fseek(fp,fpos-sizeof(blockcheck),SEEK_SET);
        //printf("found valid marker blocknum %llx -> %u endpos.%ld fpos.%ld\n",(long long)blockcheck,blocknum,fpos,ftell(fp));
    }
    return(blocknum);
}

int32_t save_rawblock(int32_t dispflag,FILE *fp,struct rawblock *raw)
{
    if ( raw->numtx < MAX_BLOCKTX && raw->numrawvins < MAX_BLOCKTX  && raw->numrawvouts < MAX_BLOCKTX )
    {
        if ( fwrite(raw,sizeof(int32_t),6,fp) != 6 )
            printf("fwrite error of header for block.%d\n",raw->blocknum);
        else if ( fwrite(raw->txspace,sizeof(*raw->txspace),raw->numtx,fp) != raw->numtx )
            printf("fwrite error of txspace[%d] for block.%d\n",raw->numtx,raw->blocknum);
        else if ( vinspace_io(dispflag,1,fp,raw->vinspace,raw->numrawvins) != 0 )
            printf("fwrite error of vinspace[%d] for block.%d\n",raw->numrawvins,raw->blocknum);
        else if ( voutspace_io(dispflag,1,fp,raw->voutspace,raw->numrawvouts) != 0 )
            printf("fwrite error of voutspace[%d] for block.%d\n",raw->numrawvouts,raw->blocknum);
        else if ( emit_blockcheck(fp,raw->blocknum) <= 0 )
            printf("emit_blockcheck error for block.%d\n",raw->blocknum);
        else return(0);
    }
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
            if ( raw->numtx < MAX_BLOCKTX && fread(raw->txspace,sizeof(*raw->txspace),raw->numtx,fp) != raw->numtx )
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
                while ( 1 ) sleep (60);
            }
            printf("created %s -> fp.%p\n",fname,fp);
            *blocknump = 0;
        }
        else
        {
            fseek(fp,0,SEEK_END);
            *blocknump = load_blockcheck(fp);
            printf("opened %s blocknum.%d\n",fname,*blocknump);
        }
    }
    return(fp);
}

void *_get_dataptr(char **strp,uint64_t *valuep,struct bitstream_file *bfp,uint32_t itemind)
{
    void *dataptr;
    struct huffitem *item = 0;
    *valuep = 0;
    *strp = 0;
    if ( bfp != 0 && itemind <= bfp->ind )
    {
        if ( (bfp->mode & BITSTREAM_UNIQUE) == 0 && (dataptr= (struct voutinfo *)bfp->dataptr) != 0 )
        {
            if ( bfp->huffwt != 0 && (bfp->mode & BITSTREAM_STATSONLY) == 0 )
                return((void *)((long)dataptr + itemind*bfp->huffwt));
            else return((void *)((long)dataptr + itemind*bfp->itemsize));
        }
        else
        {
            if ( bfp->itemptrs != 0 && (item= bfp->itemptrs[itemind]) != 0 )
            {
                if ( (bfp->mode & (BITSTREAM_STRING|BITSTREAM_HEXSTR)) == 0 )
                    *valuep = item->U.value;
                else *strp = item->str;
            }
            return(item);
        }
    }
    return(0);
}

char *_conv_rawind(struct compressionvars *V,struct bitstream_file *bfp,uint32_t rawind)
{
    char *str; uint64_t value;
    if ( _get_dataptr(&str,&value,bfp,rawind) != 0 )
        return(str);
    else return(0);
}
#define conv_addrind(V,addrind) _conv_rawind(V,(V)->bfps[(V)->addrbfp],addrind)
#define conv_txidind(V,txidind) _conv_rawind(V,(V)->bfps[(V)->txidbfp],txidind)
#define conv_scriptind(V,scriptind) _conv_rawind(V,(V)->bfps[(V)->scriptbfp],scriptind)

struct address_entry *get_vininfo(struct compressionvars *V,uint32_t vinind)
{
    char *str; uint64_t value;
    return(_get_dataptr(&str,&value,V->bfps[V->vinsbfp],vinind));
}

struct voutinfo *get_voutinfo(struct compressionvars *V,uint32_t voutind)
{
    char *str; uint64_t value;
    return(_get_dataptr(&str,&value,V->bfps[V->voutsbfp],voutind));
}

struct huffitem *get_valueinfo(struct compressionvars *V,uint64_t value)
{
    char valuestr[64];
    struct huffitem *item;
    expand_nxt64bits(valuestr,value);
    HASH_FIND_STR(V->bfps[V->valuebfp]->dataptr,valuestr,item);
    return(item);
}

struct blockinfo *get_blockinfo(struct compressionvars *V,uint32_t blocknum)
{
    char *str; uint64_t value;
    if ( blocknum > V->blocknum )
    {
        printf("get_blockinfo block.%d > V.blocknum %d\n",blocknum,V->blocknum);
        return(0);
    }
    return(_get_dataptr(&str,&value,V->bfps[0],blocknum));
}

void *get_bitstream_item(void *space,int32_t *sizep,struct bitstream_file *bfp,struct huffitem *item,uint32_t rawind)
{
    if ( item->ptr != 0 )
    {
        *sizep = item->fullsize;
        return(item->ptr);
    }
    else if ( (bfp->mode & BITSTREAM_STATSONLY) != 0 )
    {
        *sizep = (int32_t)bfp->itemsize;
        memcpy(space,&rawind,bfp->itemsize);
        return(space);
    }
    else
    {
        printf("unhandled output case: %s.rawind.%d\n",bfp->typestr,rawind);
        return(0);
    }
}

void *get_compressionvars_item(void *space,int32_t *sizep,struct compressionvars *V,struct huffitem *item)
{
    struct bitstream_file *bfp;
    uint32_t rawind,huffid;
    *sizep = 0;
    rawind = (item->huffind >> 4);
    huffid = (item->huffind & 0xf);
    if ( (bfp= V->bfps[huffid]) != 0 )
        return(get_bitstream_item(space,sizep,bfp,item,rawind));
    return(0);
}

struct huffitem *update_compressionvars_table(int32_t *createdflagp,struct bitstream_file *bfp,char *str,union huffinfo *U,long fpos)
{
    struct huffitem *item;
    int32_t len,numitems;
    HASH_FIND_STR(bfp->dataptr,str,item);
    if ( item == 0 )
    {
        len = (int32_t)strlen(str);
        item = calloc(1,sizeof(*item) + len + 1);
        strcpy(item->str,str);
        HASH_ADD_STR(bfp->dataptr,str,item);
        numitems = (++bfp->ind + 1);
        if ( bfp->nomemstructs == 0 )
        {
            bfp->itemptrs = realloc(bfp->itemptrs,sizeof(item) * numitems);
            bfp->itemptrs[bfp->ind] = item;
        }
        huff_iteminit(item,conv_rawind(bfp->huffid,bfp->ind),str,0,bfp->huffwt);
        item->U = *U;
        item->fpos = (int32_t)fpos;
        //printf("%s: add.(%s) rawind.%d huffind.%d rawind2.%d U.script.addrind %d vs %d\n",bfp->typestr,str,bfp->ind,item->huffind,item->huffind>>4,item->U.script.addrind,U->script.addrind);
        *createdflagp = 1;
    }
    else
    {
        if ( bfp->nomemstructs == 0 )
        {
            update_huffitem(1,item,item->huffind,0,0,item->wt);
            //printf("%s: incr.(%s) huffind.%d freq.%d\n",bfp->typestr,str,item->huffind,item->freq[0]);
        }
        *createdflagp = 0;
    }
    return(item);
}

struct huffitem *append_to_streamfile(struct bitstream_file *bfp,uint32_t blocknum,void *newdata,uint32_t num,int32_t flushflag)
{
    unsigned char databuf[8192];
    long n,fpos,startpos = ((long)bfp->ind * bfp->itemsize);
    void *startptr;
    FILE *fp;
    if ( (newdata == 0 || num == 0) && flushflag != 0 && (fp= bfp->fp) != 0 )
    {
        if ( blocknum != 0xffffffff )
        {
            emit_blockcheck(fp,blocknum); // will be overwritten next block, but allows resuming in case interrupted
            //printf("emit blockcheck.%d for %s from %u fpos %ld %d\n",blocknum,bfp->fname,bfp->blocknum,ftell(fp),load_blockcheck(fp));
            bfp->blocknum = blocknum;
        }
        return(0);
    }
    bfp->ind += num;
    if ( bfp->nomemstructs == 0 )
    {
        bfp->dataptr = realloc(bfp->dataptr,bfp->itemsize * bfp->ind);
        startptr = (void *)((long)bfp->dataptr + startpos);
    } else startptr = databuf;
    if ( sizeof(databuf) < num*bfp->itemsize )
    {
        printf("%s: databuf too small %ld vs %ld\n",bfp->typestr,sizeof(databuf),num*bfp->itemsize);
        exit(-1);
    }
    memcpy(startptr,newdata,num * bfp->itemsize);
    if ( blocknum == 0xffffffff )
        return(0);
    fseek(bfp->fp,startpos,SEEK_SET);
    if ( (fp= bfp->fp) != 0 && (fpos= ftell(fp)) == startpos )
    {
        if ( (n= fwrite(startptr,bfp->itemsize,num,fp)) != num )
            fprintf(stderr,"FWRITE ERROR %ld (%s) block.%u startpos.%ld num.%d itemsize.%ld\n",n,bfp->fname,blocknum,startpos,num,bfp->itemsize);
        else
        {
            if ( flushflag != 0 )
            {
                emit_blockcheck(bfp->fp,blocknum); // will be overwritten next block, but allows resuming in case interrupted
                if ( strcmp("vouts",bfp->typestr) == 0 )
                    printf("emit blockcheck.%d for %s from %u fpos %ld %d\n",blocknum,bfp->fname,bfp->blocknum,ftell(fp),load_blockcheck(fp));
                bfp->blocknum = blocknum;
            }
            return(0);
        }
    } else fprintf(stderr,"append_to_filedata.(%s) block.%u error: fp.%p fpos.%ld vs startpos.%ld\n",bfp->fname,blocknum,fp,fpos,startpos);
    while ( 1 )
        sleep(1);
    exit(-1); // to get here probably out of disk space, abort is best
}

void *update_bitstream_file(struct compressionvars *V,int32_t *createdflagp,struct bitstream_file *bfp,uint32_t blocknum,void *data,int32_t datalen,char *str,union huffinfo *Up,long fpos)
{
    union huffinfo U;
    struct huffitem *item = 0;
    uint32_t huffind,rawind;
    int32_t createdflag = 0;
    if ( blocknum != 0xffffffff )
        bfp->blocknum = blocknum;
    if ( (bfp->mode & BITSTREAM_UNIQUE) != 0 ) // "addr", "txid", "script", "value"
    {
        item = update_compressionvars_table(&createdflag,bfp,str,Up,fpos);
        if ( item != 0 )
        {
            if ( createdflag != 0 && blocknum != 0xffffffff && bfp->fp != 0 && (bfp->mode & BITSTREAM_STATSONLY) == 0 )
            {
                if ( data == 0 && str != 0 )
                    data = (uint8_t *)str, datalen = (int32_t)strlen(str) + 1;
                if ( data != 0 && datalen != 0 )
                {
                    //printf("%s: ",bfp->typestr);
                    if ( (bfp->mode & (BITSTREAM_STRING|BITSTREAM_HEXSTR)) != 0 )
                    {
                        //printf("%s: %d -> fpos.%ld ",bfp->typestr,datalen,ftell(bfp->fp));
                        emit_varint(bfp->fp,datalen);
                    }
                    if ( 0 )
                    {
                        char tmp[8192];
                        if ( str == 0 || str[0] == 0 )
                            init_hexbytes_noT(tmp,data,datalen);
                        else strcpy(tmp,str);
                        printf("(%s) -> fpos.%ld ",tmp,ftell(bfp->fp));
                    }
                    if ( fwrite(data,1,datalen,bfp->fp) != datalen )
                    {
                        printf("error writing %d bytes to %s\n",datalen,bfp->fname);
                        exit(-1);
                    }
                    //printf("block.%u -> fpos.%ld ",blocknum,ftell(bfp->fp));
                    emit_blockcheck(bfp->fp,blocknum);
                    //printf("curpos.%ld\n",ftell(bfp->fp));
                } else printf("warning: bfp[%d] had no data in block.%u\n",bfp->huffid,blocknum);
            }
            //else printf("%s: (%s) duplicate.%d \n",bfp->typestr,item->str,item->freq[0]);
        }
    }
    else if ( bfp->fp != 0 ) // "blocks", "vins", "vouts", "bitstream"
    {
        if ( blocknum != 0xffffffff && (bfp->mode & BITSTREAM_COMPRESSED) != 0 ) // "bitstream"
            update_bitstream(bfp,blocknum);
        else
        {
            append_to_streamfile(bfp,blocknum,data,1,1);
            if ( bfp->nomemstructs == 0 )
            {
                if ( strcmp(bfp->typestr,"blocks") == 0 )
                {
                    int32_t numvins,numvouts;
                    struct blockinfo B;
                    if ( datalen == sizeof(B) )
                    {
                        memcpy(&B,data,sizeof(B));
                        if ( (numvins= (B.firstvin - V->prevB.firstvin)) >= 0 && (numvouts= (B.firstvout - V->prevB.firstvout)) >= 0 )
                        {
                            if ( numvins < (1<<16) && numvouts < (1<<16) )
                            {
                                memset(&U,0,sizeof(U));
                                U.s = numvins, update_bitstream_file(V,&createdflag,V->numvinsbfp,blocknum,&U.s,sizeof(U.s),0,&U,-1);
                                U.s = numvouts, update_bitstream_file(V,&createdflag,V->numvoutsbfp,blocknum,&U.s,sizeof(U.s),0,&U,-1);
                            }
                            else
                            {
                                printf("block.%d: numvouts.%d or numvins.%d overflow\n",blocknum,numvouts,numvins);
                                exit(-1);
                            }
                        }
                        printf("block.%d %u %u | %d %d\n",blocknum,B.firstvin,B.firstvout,numvins,numvouts);
                        V->prevB = B;
                    }
                    else
                    {
                        printf("illegal size.%d for blockinfo.%ld\n",datalen,sizeof(B));
                        exit(-1);
                    }
                }
                else if ( strcmp(bfp->typestr,"vouts") == 0 )
                {
                    struct voutinfo v;
                    struct huffitem *item;
                    char valuestr[64];
                    int32_t diff;
                    if ( datalen == sizeof(v) )
                    {
                        memcpy(&v,data,datalen);
                        //if ( (item= V->bfps[V->txidbfp]->itemptrs[v.tp_ind]) != 0 )
                        //    update_huffitem(1,item,item->huffind,0,0,item->wt);
                        if ( (item= V->bfps[V->addrbfp]->itemptrs[v.addr_ind]) != 0 )
                            update_huffitem(1,item,item->huffind,0,0,item->wt);
                        if ( (item= V->bfps[V->scriptbfp]->itemptrs[v.sp_ind]) != 0 )
                        {
                            update_huffitem(1,item,item->huffind,0,0,item->wt);
                            diff = v.sp_ind - v.addr_ind;
                            if ( diff >= 0 && diff < V->spgapbfp->maxitems )
                            {
                                item = &V->spgapbfp->dataptr[diff];
                                update_huffitem(1,item,item->huffind,&diff,sizeof(diff),item->wt);
                            }
                            else printf("illegal diff?? sp.%d vs addr.%d %d vs %d\n",v.sp_ind,v.addr_ind,diff,V->spgapbfp->maxitems);
                        }
                        if ( (item= &V->voutbfp->dataptr[v.vout]) != 0 )
                            update_huffitem(1,item,item->huffind,0,0,item->wt);
                        expand_nxt64bits(valuestr,v.value);
                        memset(&U,0,sizeof(U));
                        U.value = v.value;
                        item = update_compressionvars_table(&createdflag,V->bfps[V->valuebfp],valuestr,&U,-1);
                        //if ( item != 0 )
                        //    update_huffitem(1,item,item->huffind,0,0,item->wt);
                        //printf("tx.%-5d:v%-3d A.%-5d S.%-5d %.8f\n",v.tp_ind,v.vout,v.addr_ind,v.sp_ind,dstr(v.value));
                    }
                    else
                    {
                        printf("illegal size.%d for voutinfo.%ld\n",datalen,sizeof(v));
                        exit(-1);
                    }
                }
            }
        }
    }
    if ( (bfp->mode & BITSTREAM_STATSONLY) != 0 )
    {
        ++bfp->ind;
        if ( bfp->nomemstructs == 0 )
        {
            uint8_t c; uint16_t s;
            rawind = 0;
            switch ( bfp->itemsize )
            {
                case sizeof(uint32_t): memcpy(&rawind,data,bfp->itemsize); break;
                case sizeof(uint16_t): memcpy(&s,data,bfp->itemsize), rawind = s; break;
                case sizeof(uint8_t): c = *(uint8_t *)data, rawind = c; break;
                default: rawind = 0; printf("illegal itemsize.%ld\n",bfp->itemsize);
            }
            if ( rawind < bfp->maxitems ) // "inblock", "intxind", "vout"
            {
                huffind = conv_rawind(bfp->huffid,rawind);
                item = &bfp->dataptr[rawind];
                if ( item->wt == 0 )
                    huff_iteminit(item,huffind,(void *)&huffind,sizeof(huffind),bfp->huffwt);
                update_huffitem(1,item,item->huffind,0,0,item->wt);
            } else printf("rawind %d too big for %s %u\n",rawind,bfp->fname,bfp->maxitems);
        }
    }
    *createdflagp = createdflag;
    return(item);
}

void update_vinsbfp(struct compressionvars *V,struct bitstream_file *bfp,struct address_entry *bp,uint32_t blocknum)
{
    static uint32_t prevblocknum;
    int32_t createdflag;
    union huffinfo U;
    memset(&U,0,sizeof(U));
    //printf("spent block.%u txind.%d vout.%d\n",bp->blocknum,bp->txind,bp->v);
    if ( blocknum != 0xffffffff )
        update_bitstream_file(V,&createdflag,bfp,blocknum,bp,sizeof(*bp),0,&U,-1);
    U.s = bp->txind, update_bitstream_file(V,&createdflag,V->txinbfp,blocknum,&U.s,sizeof(U.s),0,&U,-1);
    U.s = bp->v, update_bitstream_file(V,&createdflag,V->voutbfp,blocknum,&U.s,sizeof(U.s),0,&U,-1);
    U.i = bp->blocknum, update_bitstream_file(V,&createdflag,V->inblockbfp,blocknum,&U.i,sizeof(U.i),0,&U,-1);
    prevblocknum = bp->blocknum;
}

int32_t rawblockcmp(struct rawblock *ref,struct rawblock *raw)
{
    if ( ref->numtx != raw->numtx )
        return(-1);
    else if ( ref->numrawvins != raw->numrawvins )
        return(-2);
    else if ( ref->numrawvouts != raw->numrawvouts )
        return(-3);
    else if ( ref->numtx != 0 && memcmp(ref->txspace,raw->txspace,ref->numtx*sizeof(*raw->txspace)) != 0 )
        return(-4);
    else if ( ref->numrawvins != 0 && memcmp(ref->vinspace,raw->vinspace,ref->numrawvins*sizeof(*raw->vinspace)) != 0 )
        return(-5);
    else if ( ref->numrawvouts != 0 && memcmp(ref->voutspace,raw->voutspace,ref->numrawvouts*sizeof(*raw->voutspace)) != 0 )
        return(-6);
    return(0);
}

int32_t huffpair_decodeitemind(struct huffitem **itemp,struct huffpair *pair,HUFF *hp)
{
    uint32_t ind;
    int32_t i,c,*tree,tmp,numitems,depth,num = 0;
    *itemp = 0;
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
            *itemp = &pair->items[ind];
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
    char hashstr[8192];
    uint8_t data[8192];
    uint16_t s;
    int32_t i,datalen;
    long fpos;
    if ( fp == 0 )
        return(-1);
    // load varbits
    if ( fread(&s,1,sizeof(s),fp) == sizeof(s) && s > 0 )
    {
        for (i=0; i<s; i++)
        {
            if ( (fpos= load_varfilestr(&datalen,(char *)data,fp,sizeof(hashstr))) > 0 )
            {
                // handle coinaddr, script and txid
                printf("i.%d (%s)\n",i,(char *)data);
            }
        }
    }
    return(s);
}

int32_t save_bitstream_block(int dispflag,FILE *fp,HUFF *hp,queue_t *stringQ)
{
    uint16_t s;
    int32_t numbytes,n;
    long fpos,fpos2;
    char *hashstr,mode;
    if ( fp == 0 )
    {
        printf("save_bitstream_block null fp\n");
        return(-1);
    }
    numbytes = hflush(fp,hp);
    while ( (hashstr= queue_dequeue(stringQ)) != 0 )
        ;
    return(numbytes);
    fpos = ftell(fp);
    //printf("after hflush.%ld\n",fpos);
    s = 0;
    if ( fwrite(&s,1,sizeof(s),fp) == sizeof(s) )
    {
        while ( (hashstr= queue_dequeue(stringQ)) != 0 )
        {
            mode = hashstr[-1];
            if ( dispflag != 0 )
                printf("(%s).%c ",hashstr,mode);
            if ( (n= save_varfilestr(fp,hashstr)) > 0 )
                numbytes += n, s++;
            else
            {
                printf("error save_varfilestr(%s)\n",hashstr);
                return(-1);
            }
        }
        if ( s != 0 )
        {
            fpos2 = ftell(fp);
            fseek(fp,fpos,SEEK_SET);
            fwrite(&s,1,sizeof(s),fp);
            fseek(fp,fpos2,SEEK_SET);
            if ( dispflag != 0 )
                printf("wrote %d hashstrings\n",s);
        }
    }
    return(numbytes);
}

struct huffhash { char fname[512]; struct huffpair_hash *table; FILE *fp; uint32_t ind; uint8_t type; };

uint32_t huffpair_hash(int32_t iter,struct huffhash *hash,int32_t *createdflagp,char **stableptrp,struct huffpair *pair,void *ptr,int32_t datalen,union huffinfo U)
{
    struct huffpair_hash *item;
    HASH_ADD(hh,hash->table,ptr,datalen,item);
    *createdflagp = 0;
    if ( item == 0 )
    {
        item = calloc(1,sizeof(*item));
        item->U = U;
        item->type = hash->type;
        item->ptr = ptr;
        item->hashind = ++hash->ind;
        //strcpy(item->str,srcstr);
        //HASH_ADD_STR(V->hashptrs[mode],str,item);
        //HASH_ADD_KEYPTR(hh,V->hashptrs[type],item->ptr,datalen,len);
        HASH_ADD(hh,hash->table,ptr,datalen,item);

        //printf("created.(%s) ind.%d\n",item->str,item->hashind);
        *createdflagp = 1;
        if ( iter == 0 )
        {
            static FILE *fpT,*fpA,*fpS,*fpV;
            FILE *fp,**fpp;
            char fname[512];
            if ( mode != 0 )
                sprintf(fname,"hashstrings.%c",mode);
            else sprintf(fname,"hashstrings");
            switch ( mode )
            {
                case 't': fpp = &fpT; break;
                case 'a': fpp = &fpA; break;
                case 's': fpp = &fpS; break;
                default: fpp = &fpV; break;
            }
            if ( (fp= *fpp) == 0 )
                *fpp = fp = fopen(fname,"wb");
            if ( fp != 0 )
                save_varfilestr(fp,item->str);
        }
    }
    *stableptrp = item->str;
    return(item->hashind);
}

int32_t update_huffpair(uint32_t blocknum,char mode,struct compressionvars *V,queue_t *stringQ,int32_t iter,void *ptr,int32_t size,HUFF *hp,struct huffpair *pair,char *name,void *srcptr,union huffinfo U)
{
    int32_t calciters = 2;
    struct huffitem *item;
    uint32_t rawind = 0;
    char *stableptr = 0;
    int32_t numbits,createdflag = 0;
    if ( iter <= calciters )
    {
        if ( size == 0 )
            rawind = huffpair_hash(iter,mode,V,&createdflag,&stableptr,pair,srcptr,U);
        else if ( size == 2 )
        {
            rawind = 0;
            memcpy(&rawind,srcptr,2);
        }
        else
        {
            printf("unsupported size.%d\n",size);
            return(-1);
        }
    }
    //printf("(%s[%d] %s) ",name,rawind,size==0?srcptr:"");
    if ( iter >= calciters )
    {
        if ( rawind <= pair->maxind )
        {
            if ( iter == calciters )
            {
                if ( createdflag != 0 )
                    queue_enqueue(stringQ,stableptr);
                item = &pair->items[rawind];
                if ( (numbits= hwrite(item->codebits,item->numbits,hp)) > 0 )
                    return(numbits);
                printf("update_huffpair error hwrite.(%s) rawind.%d\n",name,rawind);
            }
            else
            {
                if ( (numbits= huffpair_decodeitemind(&item,pair,hp)) < 0 )
                    return(-1);
                if ( size != 0 )
                {
                    rawind = 0;
                    memcpy(&rawind,item->ptr,size);
                    rawind += blocknum;
                    memcpy(ptr,&rawind,size);
                }
                else strcpy(ptr,item->ptr);
                return(numbits);
            }
        }
        else printf("update_huffpair.(%s) rawind.%d > maxind.%d\n",name,rawind,pair->maxind);
        return(0);
    }
    if ( iter == 0 )
    {
        //static int lastrawindA,lastrawindT,lastrawindS;
        if ( pair->name[0] == 0 )
            strcpy(pair->name,name);
        if ( rawind > pair->maxind )
            pair->maxind = rawind;
        /*switch ( mode )
        {
            case 'a': V->rawinds[V->numrawinds++] = (rawind - lastrawindA), lastrawindA = rawind; break;
            case 't': V->rawinds[V->numrawinds++] = (rawind - lastrawindT), lastrawindT = rawind; break;
            case 's': V->rawinds[V->numrawinds++] = (rawind - lastrawindS), lastrawindS = rawind; break;
        }*/
    }
    else if ( iter == 1 )
    {
        if ( pair->items == 0 )
            pair->items = calloc(pair->maxind+1,sizeof(*pair->items));
        else if ( rawind > pair->maxind )
        {
            printf("illegal case rawind.%d > maxind.%d\n",rawind,pair->maxind);
            exit(-1);
        }
        item = &pair->items[rawind];
        if ( item->wt == 0 )
        {
            if ( size == 0 )
                item->ptr = stableptr, item->fullsize = strlen(stableptr) + 1, item->wt = 4;
            else item->ptr = &item->space, item->fullsize = size, memcpy(item->ptr,&rawind,size), item->wt = size;
            item->huffind = rawind;
            if ( rawind != 0 )
                pair->nonz++;
        }
        item->freq[0]++;
        pair->count++;
    }
    else printf("unsupported iter.%d\n",iter);
    return(0);
}

int32_t huff_itemsave(FILE *fp,struct huffitem *item)
{
    /*long val,n = 0;
    if ( fwrite(item,1,sizeof(*item),fp) != sizeof(*item) )
        return(-1);
    n = sizeof(*item);
    if ( (val= emit_varint(fp,item->fullsize)) <= 0 )
        return(-1);
    n += val;
    if ( (val= fwrite(item->ptr,1,item->fullsize,fp)) != item->fullsize )
        return(-1);*/
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
            if ( pair->items[i].wt != 0 )
                items[n++] = &pair->items[i];
        fprintf(stderr,"%s n.%d: ",pair->name,n);
        pair->code = huff_create(0,V,items,n,frequi);
        set_compressionvars_fname(0,fname,V->coinstr,pair->name,-1);
        if ( pair->code != 0 && (fp= fopen(fname,"wb")) != 0 )
        {
            if ( huffpair_save(fp,pair) <= 0 )
                printf("huffpair_gencode: error saving %s\n",fname);
            else fprintf(stderr,"%s saved ",fname);
            fclose(fp);
        } else printf("error creating (%s)\n",fname);
        huff_disp(0,pair->code,frequi);
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
    int32_t k;
    if ( huffpair_gencode(V,&vout->addr,frequi) != 0 )
        return(-1);
    else if ( huffpair_gencode(V,&vout->script,frequi) != 0 )
        return(-2);
    else
    {
        for (k=0; k<2; k++)
            if ( huffpair_gencode(V,&vout->value[k],frequi) != 0 )
                return(-3 - k);
    }
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
    int32_t k;
    memset(preds,0,sizeof(*preds));
    if ( allmode != 0 )
    {
        if ( (preds->numtx= H->numall.code) == 0 )
            return(-1);
        else preds->numrawvins = preds->numrawvouts = preds->numtx;
        
        if ( (preds->tx0_numvins= H->txall.numvins.code) == 0 )
            return(-2);
        else preds->tx1_numvins = preds->txi_numvins = preds->tx0_numvins;
        if ( (preds->tx0_numvouts= H->txall.numvouts.code) == 0 )
            return(-3);
        else preds->tx1_numvouts = preds->txi_numvouts = preds->tx0_numvouts;
        
        if ( (preds->vin0_txid= H->vinall.txid.code) == 0 )
            return(-4);
        else preds->vini_txid = preds->vin0_txid;
        if ( (preds->vin0_vout= H->vinall.vout.code) == 0 )
            return(-5);
        else preds->vini_vout = preds->vin0_vout;

        if ( (preds->vout0_addr= H->voutall.addr.code) == 0 )
            return(-6);
        else preds->vout1_addr = preds->vout2_addr = preds->vouti_addr = preds->voutn_addr = preds->vout0_addr;
        if ( (preds->vout0_script= H->voutall.script.code) == 0 )
            return(-7);
        else preds->vout1_script = preds->vout2_script = preds->vouti_script = preds->voutn_script = preds->vout0_script;
        for (k=0; k<2; k++)
        {
            if ( (preds->vout0_value[k]= H->voutall.value[k].code) == 0 )
                return(-8 - k);
            else preds->vout1_value[k] = preds->vout2_value[k] = preds->vouti_value[k] = preds->voutn_value[k] = preds->vout0_value[k];
        }
    }
    else
    {
        if ( (preds->numtx= H->numtx.code) == 0 )
            return(-1);
        else if ( (preds->numrawvouts= H->numrawvouts.code) == 0 )
            return(-2);
        else if ( (preds->numrawvins= H->numrawvins.code) == 0 )
            return(-3);
        
        //else if ( (preds->tx0_numvins= H->tx0.numvins.code) == 0 )
        //    return(-4);
        else if ( (preds->tx1_numvins= H->tx1.numvins.code) == 0 )
            return(-5);
        else if ( (preds->txi_numvins= H->txi.numvins.code) == 0 )
            return(-6);
        else if ( (preds->tx0_numvouts= H->tx0.numvouts.code) == 0 )
            return(-7);
        else if ( (preds->tx1_numvouts= H->tx1.numvouts.code) == 0 )
            return(-8);
        else if ( (preds->txi_numvouts= H->txi.numvouts.code) == 0 )
            return(-9);

        else if ( (preds->vin0_txid= H->vin0.txid.code) == 0 )
            return(-10);
        else if ( (preds->vin1_txid= H->vin1.txid.code) == 0 )
            return(-11);
        else if ( (preds->vini_txid= H->vini.txid.code) == 0 )
            return(-12);
        else if ( (preds->vin0_vout= H->vin0.vout.code) == 0 )
            return(-13);
        else if ( (preds->vin1_vout= H->vin1.vout.code) == 0 )
            return(-14);
        else if ( (preds->vini_vout= H->vini.vout.code) == 0 )
            return(-15);

        else if ( (preds->vout0_addr= H->vout0.addr.code) == 0 )
            return(-16);
        else if ( (preds->vout1_addr= H->vout1.addr.code) == 0 )
            return(-17);
        else if ( (preds->vouti_addr= H->vouti.addr.code) == 0 )
            return(-18);
        else if ( (preds->voutn_addr= H->voutn.addr.code) == 0 )
            return(-19);
        else if ( (preds->vout0_script= H->vout0.script.code) == 0 )
            return(-20);
        else if ( (preds->vout1_script= H->vout1.script.code) == 0 )
            return(-21);
        else if ( (preds->vouti_script= H->vouti.script.code) == 0 )
            return(-22);
        else if ( (preds->voutn_script= H->voutn.script.code) == 0 )
            return(-23);
        else
        {
            for (k=0; k<2; k++)
            {
                if ( (preds->vout0_value[k]= H->vout0.value[k].code) == 0 )
                    return(-24 - k*4);
                else if ( (preds->vout1_value[k]= H->vout1.value[k].code) == 0 )
                    return(-24 - k*4 - 1);
                else if ( (preds->vouti_value[k]= H->vouti.value[k].code) == 0 )
                    return(-24 - k*4 - 2);
                else if ( (preds->voutn_value[k]= H->voutn.value[k].code) == 0 )
                    return(-24 - k*4 - 3);
            }
        }
    }
    return(0);
}

void init_bitstream(int32_t dispflag,struct compressionvars *V,FILE *fp,int32_t frequi)
{
    double avesize,avebits[HUFF_NUMGENS],startmilli = milliseconds();
    int32_t numbits[HUFF_NUMGENS],n[HUFF_NUMGENS],allbits[HUFF_NUMGENS],j,k,err;
    uint32_t blocknum,txind,vin,vout,checkvins,checkvouts,iter,calciters,geniters;
    uint16_t s;
    struct rawtx *tx,*tx2;
    struct rawvin *vi,*vi2;
    struct rawvout *vo,*vo2;
    struct rawblock *raw;
    long endpos,eofpos;
    char *str,fracstr[64],intstr[64],valuestr[64];
    struct huffpair *pair;
    struct rawtx_huffs *txpairs;
    struct rawvin_huffs *vinpairs;
    struct rawvout_huffs *voutpairs;
    struct rawblock_huffs H;
    queue_t stringQ;
    struct rawblock_preds P[HUFF_NUMGENS],*preds;
    memset(&H,0,sizeof(H));
    memset(P,0,sizeof(P));
    memset(&stringQ,0,sizeof(stringQ));
    memset(&allbits,0,sizeof(allbits));
    if ( fp != 0 )
    {
        calciters = 2;
        geniters = 1;//HUFF_NUMGENS;
        endpos = get_endofdata(&eofpos,fp);
        for (iter=0; iter<calciters+geniters; iter++)
        {
            //dispflag = iter>1;
            rewind(fp);
            raw = &V->raw;
            if ( iter == calciters )
            {
                if ( (err= huffpair_gencodes(V,&H,frequi)) != 0 )
                    printf("error setting gencodes iter.%d err.%d\n",iter,err);
                for (j=0; j<geniters; j++)
                {
                    if ( (err= set_rawblock_preds(&P[j],&H,j)) < 0 )
                        printf("error setting preds iter.%d j.%d err.%d\n",iter,j,err);
                }
                //getchar();
            }
            //huffpair_clearhash(V);
            for (j=0; j<geniters; j++)
            {
                if ( V->bitfps[j] != 0 )
                    rewind(V->bitfps[j]);
            }
            while ( (str= queue_dequeue(&stringQ)) != 0 )
                ;
            for (blocknum=1; blocknum<=V->blocknum/1; blocknum++)
            {
                if ( dispflag == 0 && (blocknum % 1000) == 0 )
                    fprintf(stderr,".");
                //printf("load block.%d\n",blocknum);
                memset(raw,0,sizeof(*raw));
                //memset(V->rawinds,0,sizeof(V->rawinds));
                //V->numrawinds = 0;
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
                    memset(&V->raws[j],0,sizeof(V->raws[j]));
                    if ( iter > calciters ) // calciters is to output to bitfps
                        load_bitstream_block(V->hps[j],V->bitfps[j]);
                }
                preds = (iter >= calciters) ? &P[iter - calciters] : 0;
                checkvins = checkvouts = 0;
                for (j=0; j<geniters; j++)
                {
                    if ( j == 0 )
                        str = "numtx", pair = &H.numtx;
                    else str = "numall", pair = &H.numall;
                    s = raw->numtx;
                    n[j]++, numbits[j] += update_huffpair(blocknum,0,V,0,iter,&V->raws[j].numtx,2,V->hps[j],pair,str,&s);

                    if ( j == 0 )
                        str = "numrawvins", pair = &H.numrawvins;
                    else str = "numall", pair = &H.numall;
                    s = raw->numrawvins;
                    n[j]++, numbits[j] += update_huffpair(blocknum,0,V,0,iter,&V->raws[j].numrawvins,2,V->hps[j],pair,str,&s);

                    if ( j == 0 )
                        str = "numrawvouts", pair = &H.numrawvouts;
                    else str = "numall", pair = &H.numall;
                    s = raw->numrawvouts;
                    n[j]++, numbits[j] += update_huffpair(blocknum,0,V,0,iter,&V->raws[j].numrawvouts,2,V->hps[j],pair,str,&s);
                }
                if ( raw->numtx > 0 )
                {
                    for (txind=0; txind<raw->numtx; txind++)
                    {
                        tx = &raw->txspace[txind];
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
                            }
                            s = tx->numvins;
                            if ( txind > 0 || s > 0 )
                                n[j]++, numbits[j] += update_huffpair(blocknum,0,V,0,iter,&tx2->numvins,2,V->hps[j],&txpairs->numvins,txpairs->numvinsname,&s);
                            s = tx->numvouts;
                            n[j]++, numbits[j] += update_huffpair(blocknum,0,V,0,iter,&tx2->numvouts,2,V->hps[j],&txpairs->numvouts,txpairs->numvoutsname,&s);
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
                                    n[j]++, numbits[j] += update_huffpair(blocknum,'t',V,&stringQ,iter,vi2->txidstr,0,V->hps[j],&vinpairs->txid,vinpairs->txidname,vi->txidstr);
                                    s = vi->vout;
                                    n[j]++, numbits[j] += update_huffpair(blocknum,0,V,0,iter,&vi2->vout,2,V->hps[j],&vinpairs->vout,vinpairs->voutname,&s);
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
                                        //for (k=0; k<4; k++)
                                        //    sprintf(voutpairs->valuename[k],"%s.value%d",str,k);
                                        sprintf(voutpairs->valuename[0],"%s.valuefrac",str);
                                        sprintf(voutpairs->valuename[1],"%s.valueint",str);
                                        //sprintf(voutpairs->valuename[0],"%s.value",str);
                                    }
                                    n[j]++, numbits[j] += update_huffpair(blocknum,'a',V,&stringQ,iter,vo2->coinaddr,0,V->hps[j],&voutpairs->addr,voutpairs->addrname,vo->coinaddr);
                                    n[j]++, numbits[j] += update_huffpair(blocknum,'s',V,&stringQ,iter,vo2->script,0,V->hps[j],&voutpairs->script,voutpairs->scriptname,vo->script);
                                    /*for (k=0; k<4; k++)
                                    {
                                        s = ((vo->value >> (k*16)) & 0xffff);
                                        n[j]++, numbits[j] += update_huffpair(0,V,0,iter,(void *)((long)&vo2->value+k*2),2,V->hps[j],&voutpairs->value[k],voutpairs->valuename[k],&s);
                                    }*/
                                    expand_nxt64bits(valuestr,vo->value);
                                    sprintf(intstr,"%1.8f",dstr(vo->value));
                                    for (k=0; intstr[k]!=0; k++)
                                        if ( intstr[k] == '.' )
                                            break;
                                    intstr[k] = 0;
                                    strcpy(fracstr,intstr+k+1);
                                    //printf("(%s) (%.8f) -> (%s).(%s)\n",valuestr,dstr(vo->value),intstr,fracstr);
                                    k = 0;
                                    //n[j]++, numbits[j] += update_huffpair(0,V,0,iter,&vo2->value,0,V->hps[j],&voutpairs->value[k],voutpairs->valuename[k],valuestr), k++;
                                    n[j]++, numbits[j] += update_huffpair(blocknum,'f',V,&stringQ,iter,&vo2->value,0,V->hps[j],&voutpairs->value[k],voutpairs->valuename[k],fracstr), k++;
                                    n[j]++, numbits[j] += update_huffpair(blocknum,'i',V,&stringQ,iter,&vo2->value,0,V->hps[j],&voutpairs->value[k],voutpairs->valuename[k],intstr), k++;
                                }
                            }
                            checkvouts += tx->numvouts;
                        }
                        if ( dispflag != 0 )
                            printf("(vi.%d vo.%d) | ",tx->numvins,tx->numvouts);
                    }
                }
                if ( checkvins != raw->numrawvins || checkvouts != raw->numrawvouts )
                    printf("ERROR: checkvins %d != %d raw->numrawvins || checkvouts %d != %d raw->numrawvouts\n",checkvins,raw->numrawvins,checkvouts,raw->numrawvouts);
                /*if ( V->numrawinds > 0 )
                {
                    for (j=0; j<V->numrawinds; j++)
                        printf("%d ",V->rawinds[j]);
                    printf(" -> numrawinds.%d block.%d\n",V->numrawinds,blocknum);
                }*/
                for (j=0; j<geniters; j++)
                {
                    allbits[j] += numbits[j];
                    avebits[j] = ((double)allbits[j] / (blocknum+1));
                    if ( iter >= calciters )
                    {
                        if ( iter == calciters )
                        {
                            save_bitstream_block(dispflag,V->bitfps[j],V->hps[j],&stringQ);
                             //expand_rawbits(&V->raws[j],V->hps[j],preds);
                            //if ( rawblockcmp(raw,&V->raws[j]) == 0 )
                                //else printf("iter.%d rawblockcmp error block.%d gen.%d\n",iter,blocknum,j);
                        }
                        else
                        {
                            if ( (err= rawblockcmp(raw,&V->raws[j])) != 0 )
                                printf("iter.%d rawblockcmp error block.%d gen.%d err.%d\n",iter,blocknum,j,err);
                            //expand_rawbits(&V->raws[j],V->hps[j],preds);
                            //if ( rawblockcmp(raw,&V->raws[j]) != 0 )
                            //    printf("iter.%d rawblockcmp2 error block.%d gen.%d\n",iter,blocknum,j);
                        }
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

int32_t load_reference_strings(struct compressionvars *V,struct bitstream_file *bfp,int32_t isbinary)
{
    FILE *fp;
    char str[65536];
    uint8_t data[32768];
    struct huffitem *item;
    union huffinfo U;
    long remaining,eofpos,fpos,endpos = 0;
    int32_t len,maxlen,createdflag,n,count = 0;
    n = 0;
    if ( (fp= bfp->fp) != 0 )
    {
        endpos = get_endofdata(&eofpos,fp);
        maxlen = (int32_t)sizeof(data)-1;
        while ( ftell(fp) < endpos && (fpos= load_varfilestr(&len,(isbinary != 0) ? (char *)data : str,fp,maxlen)) > 0 )
        {
            memset(&U,0,sizeof(U));
            //printf("isbinary.%d: len.%d\n",isbinary,len);
            if ( isbinary != 0 )
            {
                if ( (bfp->mode & BITSTREAM_SCRIPT) != 0 )
                    U.script.mode = expand_scriptdata(str,data,len);
                else if ( (bfp->mode & BITSTREAM_32bits) != 0 )
                {
                    memcpy(&U.i,data,sizeof(uint32_t));
                    init_hexbytes_noT(str,data+sizeof(uint32_t),len-sizeof(uint32_t));
                }
                else if ( (bfp->mode & BITSTREAM_64bits) != 0 )
                    memcpy(&U.value,data,sizeof(uint64_t));
                else init_hexbytes_noT(str,data,len);
            }
            if ( str[0] != 0 )
            {
                if ( isbinary != 0 )
                    item = update_bitstream_file(V,&createdflag,bfp,0xffffffff,data,len,str,&U,fpos);
                else item = update_bitstream_file(V,&createdflag,bfp,0xffffffff,0,0,str,&U,fpos);
                if ( (bfp->mode & BITSTREAM_SCRIPT) != 0 )
                {
                    //printf("add.(%s)\n",str);
                    //sp = (struct scriptinfo *)item;
                    //sp->mode = mode;
                    //sp->addrhuffind = addrhuffind;
                    //if ( addrhuffind != 0 )
                    //    printf("set addrhuffind.%d < (%s)\n",addrhuffind,str);
                }
                if ( createdflag == 0 )
                    printf("WARNING: redundant entry in (%s).%d [%s]?\n",bfp->fname,count,str);
                count++;
            }
            remaining = (endpos - ftell(fp));
            if ( remaining < sizeof(data) )
                maxlen = (int32_t)(endpos - ftell(fp));
        }
    }
    bfp->checkblock = load_blockcheck(bfp->fp);
    printf("loaded %d to block.%u from hashtable.(%s) fpos.%ld vs endpos.%ld | numblockchecks.%d\n",count,bfp->checkblock,bfp->fname,ftell(fp),eofpos,n);
    return(count);
}

int32_t load_fixed_fields(struct compressionvars *V,struct bitstream_file *bfp)
{
    void *ptr;
    char hexstr[16385];
    uint8_t data[8192];
    union huffinfo U;
    int32_t createdflag,count = 0;
    long eofpos,endpos,itemsize,fpos;
    endpos = get_endofdata(&eofpos,bfp->fp);
    if ( bfp->itemsize >= sizeof(data) )
    {
        printf("bfp.%s itemsize.%ld too big\n",bfp->typestr,bfp->itemsize);
        exit(-1);
    }
    if ( (bfp->mode & BITSTREAM_VALUE) != 0 )
        itemsize = sizeof(uint64_t);
    else itemsize = bfp->itemsize;
    fpos = ftell(bfp->fp);
    while ( (ftell(bfp->fp)+bfp->itemsize) <= endpos && fread(data,1,itemsize,bfp->fp) == itemsize )
    {
        memset(&U,0,sizeof(U));
        init_hexbytes_noT(hexstr,data,itemsize);
        ptr = update_bitstream_file(V,&createdflag,bfp,0xffffffff,data,(int32_t)itemsize,hexstr,&U,fpos);
        //if ( (bfp->mode & BITSTREAM_VALUE) != 0 )
        //    memcpy(&((struct valueinfo *)ptr)->value,data,itemsize);
        //else
        if ( (bfp->mode & BITSTREAM_VINS) != 0 )
            update_vinsbfp(V,bfp,(void *)data,0xffffffff);
        fpos = ftell(bfp->fp);
    }
    return(count);
}

struct bitstream_file *init_bitstream_file(struct compressionvars *V,int32_t huffid,int32_t mode,int32_t readonly,char *coinstr,char *typestr,long itemsize,uint32_t refblock,long huffwt)
{
    struct bitstream_file *bfp = calloc(1,sizeof(*bfp));
    int32_t numitems;
    bfp->huffid = huffid;
    bfp->nomemstructs = !readonly;
    bfp->mode = mode;
    bfp->huffwt = (uint32_t)huffwt;
    bfp->itemsize = itemsize;
    bfp->refblock = refblock;
    strcpy(bfp->coinstr,coinstr);
    strcpy(bfp->typestr,typestr);
    if ( (mode & BITSTREAM_STATSONLY) != 0 ) // needs to be unique filtered, so use hashtable
    {
        bfp->maxitems = refblock;
        if ( bfp->nomemstructs == 0 )
        {
            if ( itemsize == sizeof(uint32_t) )
                numitems = bfp->maxitems;
            else if ( itemsize == sizeof(int16_t) )
                numitems = (1<<16);
            else { printf("unsupported itemsize of statsonly: %ld\n",itemsize); exit(-1); }
            bfp->dataptr = calloc(numitems,sizeof(struct huffitem));
        }
        return(bfp);
    }
    set_compressionvars_fname(readonly,bfp->fname,coinstr,typestr,-1);
    bfp->fp = _open_varsfile(readonly,&bfp->blocknum,bfp->fname,coinstr);
    if ( bfp->fp != 0 ) // needs to be unique filtered, so use hashtable
    {
        rewind(bfp->fp);
        if ( (bfp->mode & (BITSTREAM_STRING|BITSTREAM_HEXSTR)) != 0 )
            load_reference_strings(V,bfp,bfp->mode & BITSTREAM_HEXSTR);
        else if ( bfp->itemsize != 0 )
            load_fixed_fields(V,bfp);
        //else load_bitstream(V,bfp);
        bfp->blocknum = load_blockcheck(bfp->fp);
    }
    if (  refblock != 0xffffffff && bfp->blocknum != refblock )
    {
        printf("%s bfp->blocknum %u != refblock.%u mismatch FATAL if less than\n",typestr,bfp->blocknum,refblock);
        if ( bfp->blocknum < refblock )
            exit(-1);
    }
    //printf("%-8s mode.%d %s itemsize.%ld numitems.%d blocknum.%u refblock.%u\n",typestr,mode,coinstr,itemsize,bfp->ind,bfp->blocknum,refblock);
    return(bfp);
}

int32_t init_compressionvars(int32_t readonly,struct compressionvars *V,char *coinstr,int32_t maxblocknum)
{
    struct coin_info *cp = get_coin_info(coinstr);
    struct huffitem *addrp=0,*tp=0,*sp=0,*valp=0;
    struct blockinfo *blockp = 0;
    uint32_t n=0,refblock;
    char fname[512];
    //if ( V->rawbits == 0 )
    {
        strcpy(V->coinstr,coinstr);
        V->startmilli = milliseconds();
        if ( (V->maxblocknum= get_blockheight(cp)) == 0 )
            V->maxblocknum = maxblocknum;
        printf("init compression vars.%s: maxblocknum %d %d readonly.%d\n",coinstr,maxblocknum,get_blockheight(cp),readonly);
        V->disp = calloc(1,100000);
        V->spgapbfp = init_bitstream_file(V,-6,BITSTREAM_STATSONLY,readonly,coinstr,"spgap",sizeof(uint32_t),V->maxblocknum,sizeof(uint32_t));
        V->numvinsbfp = init_bitstream_file(V,-5,BITSTREAM_STATSONLY,readonly,coinstr,"numvins",sizeof(uint16_t),1<<16,sizeof(uint16_t));
        V->numvoutsbfp = init_bitstream_file(V,-4,BITSTREAM_STATSONLY,readonly,coinstr,"numvouts",sizeof(uint16_t),1<<16,sizeof(uint16_t));
        V->inblockbfp = init_bitstream_file(V,-3,BITSTREAM_STATSONLY,readonly,coinstr,"inblock",sizeof(uint32_t),V->maxblocknum,sizeof(uint32_t));
        V->txinbfp = init_bitstream_file(V,-2,BITSTREAM_STATSONLY,readonly,coinstr,"intxind",sizeof(uint16_t),1<<16,sizeof(uint16_t));
        V->voutbfp = init_bitstream_file(V,-1,BITSTREAM_STATSONLY,readonly,coinstr,"vout",sizeof(uint16_t),1<<16,sizeof(uint16_t));
        V->bfps[V->spgapbfp->huffid & 0xf] = V->spgapbfp;
        V->bfps[V->numvinsbfp->huffid & 0xf] = V->numvinsbfp;
        V->bfps[V->numvoutsbfp->huffid & 0xf] = V->numvoutsbfp;
        V->bfps[V->inblockbfp->huffid & 0xf] = V->inblockbfp;
        V->bfps[V->txinbfp->huffid & 0xf] = V->txinbfp;
        V->bfps[V->voutbfp->huffid & 0xf] = V->voutbfp;
        n = 0;
        V->bfps[n] = init_bitstream_file(V,n,0,readonly,coinstr,"blocks",sizeof(*blockp),0xffffffff,0), n++;
        V->firstblock = refblock = V->bfps[0]->blocknum;
        V->valuebfp = n, V->bfps[n] = init_bitstream_file(V,n,BITSTREAM_UNIQUE|BITSTREAM_VALUE,readonly,coinstr,"values",sizeof(*valp),refblock,sizeof(uint64_t)), n++;
        V->addrbfp = n, V->bfps[n] = init_bitstream_file(V,n,BITSTREAM_UNIQUE|BITSTREAM_STRING,readonly,coinstr,"addrs",sizeof(*addrp),refblock,sizeof(uint32_t)), n++;
        V->txidbfp = n, V->bfps[n] = init_bitstream_file(V,n,BITSTREAM_UNIQUE|BITSTREAM_HEXSTR,readonly,coinstr,"txids",sizeof(*tp),refblock,sizeof(uint32_t)), n++;
        V->scriptbfp = n, V->bfps[n] = init_bitstream_file(V,n,BITSTREAM_UNIQUE|BITSTREAM_HEXSTR|BITSTREAM_SCRIPT,readonly,coinstr,"scripts",sizeof(*sp),refblock,sizeof(uint32_t)), n++;
        V->voutsbfp = n, V->bfps[n] = init_bitstream_file(V,n,BITSTREAM_VOUTS,readonly,coinstr,"vouts",sizeof(struct voutinfo),refblock,sizeof(struct voutinfo)), n++;
        V->vinsbfp = n, V->bfps[n] = init_bitstream_file(V,n,BITSTREAM_VINS,readonly,coinstr,"vins",sizeof(struct address_entry),refblock,sizeof(struct address_entry)), n++;
        //V->bitstream = n, V->bfps[n] = init_bitstream_file(V,n,BITSTREAM_COMPRESSED,readonly,coinstr,"bitstream",0,refblock,0), n++;
        set_compressionvars_fname(readonly,fname,coinstr,"rawblocks",-1);
#ifdef HUFF_GENMODE
        char cmdstr[512]; sprintf(cmdstr,"rm %s",fname); system(cmdstr);
        V->rawfp = _open_varsfile(readonly,&V->blocknum,fname,coinstr);
#else
        V->rawfp = _open_varsfile(readonly,&V->blocknum,fname,coinstr);
        int j;
        for (j=0; j<HUFF_NUMGENS; j++)
        {
            V->rawbits[j] = calloc(1,1000000);
            V->hps[j] = hopen(V->rawbits[j],1000000);
            set_compressionvars_fname(readonly,fname,coinstr,"bitstream",j);
            V->bitfps[j] = _open_varsfile(0,&V->blocknum,fname,coinstr);
        }
        if ( V->blocknum == 0 )
            V->blocknum = get_blockheight(get_coin_info(coinstr));
        printf("rawfp.%p readonly.%d V->blocks.%d (%s) %s bitfps %p %p\n",V->rawfp,readonly,V->blocknum,fname,coinstr,V->bitfps[0],V->bitfps[1]);
        init_bitstream(0,V,V->rawfp,0);
#endif
    }
    if ( readonly != 0 )
        exit(1);
    return(n);
}


#endif
#endif
