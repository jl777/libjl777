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
struct txinfo { uint16_t txind,numvouts,numvins; };
union huffinfo
{
    uint8_t c; uint16_t s; uint32_t i; void *ptr;
    uint64_t value;
    struct scriptinfo script;
    struct txinfo tx;
};

struct huffitem
{
    union huffinfo U;
    UT_hash_handle hh;
    void *ptr;
    uint64_t codebits;
    uint32_t huffind,fpos,freq[HUFF_NUMFREQS];
    uint16_t fullsize;
    uint8_t wt,numbits;
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

struct compressionvars
{
    struct rawblock raw;
    FILE *rawfp;
    HUFF *hp;
    struct blockinfo prevB;
    uint32_t valuebfp,addrbfp,txidbfp,scriptbfp,voutsbfp,vinsbfp,bitstream,numbfps;
    uint32_t maxitems,maxblocknum,firstblock,blocknum,processed,firstvout,firstvin;
    uint8_t *buffer,*rawbits;
    struct bitstream_file *bfps[16],*numvoutsbfp,*numvinsbfp,*inblockbfp,*txinbfp,*voutbfp,*spgapbfp;
    char *disp,coinstr[64];
    double startmilli;
};

struct huffcode
{
    struct huffitem **items;
    struct compressionvars *V;
    double totalbits,totalbytes,freqsum;
    int32_t numitems,maxbits,numnodes,depth;
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
    if ( emit_varint(fp,hp->endpos) < 0 )
        return(-1);
    len = hp->endpos >> 3;
    if ( (hp->endpos & 7) != 0 )
        len++;
    if ( fwrite(hp->buf,1,len,fp) != len )
        return(-1);
    fflush(fp);
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
            printf("%06d: freq.%-6d wt.%d fullsize.%d (%8s).%d\n",item->huffind>>4,item->freq[frequi],item->wt,item->fullsize,huff_str(codebits,n),n);
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
    printf("heap.%p h.%p s.%d numitems.%d frequi.%d\n",heap,heap->h,heap->s,numitems,frequi);
    for (i=n=0; i<numitems; i++)
        if ( efreqs[i] > 0 )
        {
            n++;
            freqsum += items[i]->freq[frequi];
            _heap_add(heap,i);
        }
    printf("added n.%d items: freqsum %.0f\n",n,freqsum);
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
            if ( (item = items[i]) != 0 )
            {
                item->codebits = _reversebits(codebits,numbits);
                item->numbits = numbits;
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
    int32_t ix2,i,n,extf,*preds;
    uint32_t *revtree;
    double freqsum;
    struct huffcode *huff;
    if ( (extf= huff_init(&freqsum,&preds,&revtree,items,numitems,frequi)) <= 0 )
        return(0);
    huff = calloc(1,sizeof(*huff) + (sizeof(*huff->tree) * 2 * extf));
    huff->items = items;
    huff->V = V;
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
    long savepos,len = strlen(str);
    savepos = ftell(fp);
    //printf("save.(%s) at %ld\n",str,ftell(fp));
    if ( emit_varint(fp,len) > 0 )
    {
        fwrite(str,1,len,fp);
        fflush(fp);
        return(0);
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
                str[len] = 0;
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

int32_t load_rawvout(FILE *fp,struct rawvout *vout)
{
    long fpos,n;
    uint64_t varint,vlen;
    int32_t datalen,retval;
    uint8_t data[4096];
    if ( (n= load_varint(&varint,fp)) <= 0 )
        return(-1);
    else
    {
        //printf("loaded varint %lld with %ld bytes | fpos.%ld\n",(long long)varint,n,ftell(fp));
        vout->value = varint;
        if ( (fpos= load_varfilestr(&datalen,(char *)data,fp,sizeof(vout->script)/2-1)) > 0 )
        {
            expand_scriptdata(vout->script,data,datalen);
            retval = load_varint(&vlen,fp);
            //printf("script.(%s) datalen.%d | retval.%d vlen.%llu | fpos.%ld\n",vout->script,datalen,retval,(long long)vlen,ftell(fp));
            vout->coinaddr[0] = 0;
            if ( vlen > 0 && retval > 0 && vlen < sizeof(vout->coinaddr)-1 && (n= fread(vout->coinaddr,1,vlen+1,fp)) == (vlen+1) )
           // if ( (fpos= load_varfilestr(&len,vout->coinaddr,fp,sizeof(vout->coinaddr)-1)) > 0 )
            {
                //printf("coinaddr.(%s) len.%d fpos.%ld\n",vout->coinaddr,(int)vlen,ftell(fp));
                return(0);
            } else printf("read coinaddr.%d (%s) returns %ld fpos.%ld\n",(int)vlen,vout->coinaddr,n,ftell(fp));
        } else return(-2);
    }
    return(-3);
}

int32_t save_rawvout(FILE *fp,struct rawvout *vout)
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

int32_t load_rawvin(FILE *fp,struct rawvin *vin)
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
            return(0);
        }
    }
    return(-1);
}

int32_t save_rawvin(FILE *fp,struct rawvin *vin)
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

int32_t vinspace_io(int32_t saveflag,FILE *fp,struct rawvin *vins,int32_t numrawvins)
{
    int32_t i;
    if ( numrawvins > 0 )
    {
        for (i=0; i<numrawvins; i++)
            if ( ((saveflag != 0) ? save_rawvin(fp,&vins[i]) : load_rawvin(fp,&vins[i])) != 0 )
                return(-1);
    }
    return(0);
}

int32_t voutspace_io(int32_t saveflag,FILE *fp,struct rawvout *vouts,int32_t numrawvouts)
{
    int32_t i,retval;
    if ( numrawvouts > 0 )
    {
        for (i=0; i<numrawvouts; i++)
            if ( (retval= ((saveflag != 0) ? save_rawvout(fp,&vouts[i]) : load_rawvout(fp,&vouts[i]))) != 0 )
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

int32_t save_rawblock(FILE *fp,struct rawblock *raw)
{
    if ( raw->numtx < MAX_BLOCKTX && raw->numrawvins < MAX_BLOCKTX  && raw->numrawvouts < MAX_BLOCKTX )
    {
        if ( fwrite(raw,sizeof(int32_t),6,fp) != 6 )
            printf("fwrite error of header for block.%d\n",raw->blocknum);
        else if ( fwrite(raw->txspace,sizeof(*raw->txspace),raw->numtx,fp) != raw->numtx )
            printf("fwrite error of txspace[%d] for block.%d\n",raw->numtx,raw->blocknum);
        else if ( vinspace_io(1,fp,raw->vinspace,raw->numrawvins) != 0 )
            printf("fwrite error of vinspace[%d] for block.%d\n",raw->numrawvins,raw->blocknum);
        else if ( voutspace_io(1,fp,raw->voutspace,raw->numrawvouts) != 0 )
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
                if ( raw->numrawvins < MAX_BLOCKTX && vinspace_io(0,fp,raw->vinspace,raw->numrawvins) != 0 )
                    printf("read error of vinspace[%d] for block.%d\n",raw->numrawvins,raw->blocknum);
                else if ( raw->numrawvouts < MAX_BLOCKTX && voutspace_io(0,fp,raw->voutspace,raw->numrawvouts) != 0 )
                    printf("read error of voutspace[%d] for block.%d\n",raw->numrawvouts,raw->blocknum);
                else
                {
                    //if ( fread(&checkblock,1,sizeof(checkblock),fp) != sizeof(checkblock) )
                    //    printf("fread error of checkblock for block.%d expected.%d: %llx\n",raw->blocknum,expectedblock,(long long)checkblock);
                    //if ( dispflag != 0 )
                    //    printf("checkblock.%llx %x vs %x\n",(long long)checkblock,(uint32_t)~(checkblock>>32),(uint32_t)checkblock);
                    return(0);
                }
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
    U.i = abs(bp->blocknum - prevblocknum), update_bitstream_file(V,&createdflag,V->inblockbfp,blocknum,&U.i,sizeof(U.i),0,&U,-1);
    prevblocknum = bp->blocknum;
}

void init_bitstream(struct compressionvars *V,FILE *fp)
{
    double avesize,startmilli = milliseconds();
    uint32_t blocknum,txind,vin,vout,checkvins,checkvouts;
    struct rawtx *tx;
    struct rawvin *vi;
    struct rawvout *vo;
    struct rawblock *raw;
    long endpos,eofpos;
    if ( fp != 0 )
    {
        endpos = get_endofdata(&eofpos,fp);
        rewind(fp);
        raw = &V->raw;
        for (blocknum=1; blocknum<=V->blocknum||ftell(fp)<endpos-1024; blocknum++)
        {
            if ( load_rawblock(0,fp,raw,blocknum,endpos) != 0 )
            {
                printf("error loading block.%d\n",blocknum);
                break;
            }
            checkvins = checkvouts = 0;
            if ( raw->numtx > 0 )
            {
                for (txind=0; txind<raw->numtx; txind++)
                {
                    tx = &raw->txspace[txind];
                    if ( tx->numvins > 0 )
                    {
                        for (vin=0; vin<tx->numvins; vin++)
                        {
                            vi = &raw->vinspace[tx->firstvin + vin];
                            printf("(%s).v%d ",vi->txidstr,vi->vout);
                        }
                        checkvins += tx->numvins;
                    }
                    if ( tx->numvouts > 0 )
                    {
                        for (vout=0; vout<tx->numvouts; vout++)
                        {
                            vo = &raw->voutspace[tx->firstvout + vout];
                            printf("(%s, %s, %.8f) ",vo->coinaddr,vo->script,dstr(vo->value));
                        }
                        checkvouts += tx->numvouts;
                    }
                    printf("(vins.%d vouts.%d).txind.%d\n",tx->numvins,tx->numvouts,txind);
                }
            }
            avesize = ((double)ftell(fp) / (blocknum+1));
            if ( checkvins != raw->numrawvins || checkvouts != raw->numrawvouts )
                printf("ERROR: checkvins %d != %d raw->numrawvins || checkvouts %d != %d raw->numrawvouts\n",checkvins,raw->numrawvins,checkvouts,raw->numrawvouts);
            printf("%-5s.%u [%.1f per block: est %s] numtx.%d raw.(vins.%d vouts.%d) minted %.8f\n",V->coinstr,blocknum,avesize,_mbstr(V->blocknum * avesize),raw->numtx,raw->numrawvins,raw->numrawvouts,dstr(raw->minted));
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
    if ( V->rawbits == 0 )
    {
        strcpy(V->coinstr,coinstr);
        V->startmilli = milliseconds();
        if ( (V->maxblocknum= get_blockheight(cp)) == 0 )
            V->maxblocknum = maxblocknum;
        printf("init compression vars.%s: maxblocknum %d %d readonly.%d\n",coinstr,maxblocknum,get_blockheight(cp),readonly);
        V->disp = calloc(1,100000);
        V->buffer = calloc(1,100000);
        V->hp = hopen(V->buffer,100000);
        V->rawbits = calloc(1,100000);
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
        V->bitstream = n, V->bfps[n] = init_bitstream_file(V,n,BITSTREAM_COMPRESSED,readonly,coinstr,"bitstream",0,refblock,0), n++;
        set_compressionvars_fname(readonly,fname,coinstr,"rawblocks",-1);
#ifdef HUFF_GENMODE
        char cmdstr[512]; sprintf(cmdstr,"rm %s",fname); system(cmdstr);
        V->rawfp = _open_varsfile(readonly,&V->blocknum,fname,coinstr);
#else
        V->rawfp = _open_varsfile(readonly,&V->blocknum,fname,coinstr);
        printf("rawfp.%p readonly.%d V->blocks.%d (%s) %s\n",V->rawfp,readonly,V->blocknum,fname,coinstr);
        init_bitstream(V,V->rawfp);
#endif
    }
    if ( readonly != 0 )
        exit(1);
    return(n);
}


#endif
#endif
