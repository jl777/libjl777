/******************************************************************************
 * Copyright © 2014-2015 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * Nxt software, including this file, may be copied, modified, propagated,    *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/


#ifdef DEFINES_ONLY
#ifndef crypto777_huffstream_h
#define crypto777_huffstream_h
#include <stdio.h>
#include <memory.h>
#include <stdint.h>

struct huffstream { uint8_t *ptr,*buf; uint32_t bitoffset,maski,endpos; uint32_t allocsize:31,allocated:1; };
typedef struct huffstream HUFF;

void _init_HUFF(HUFF *hp,int32_t allocsize,void *buf);
int32_t hupdate_internals(HUFF *hp);
#define hrewind(hp) hseek(hp,0,SEEK_SET)
int32_t hseek(HUFF *hp,int32_t offset,int32_t mode);
void hclear(HUFF *hp,int32_t clearbuf);
int32_t hputbit(HUFF *hp,int32_t bit);
int32_t hgetbit(HUFF *hp);
int32_t hwrite(uint64_t codebits,int32_t numbits,HUFF *hp);
uint64_t hread(int32_t *numbitsp,int32_t numbits,HUFF *hp);
int32_t hmemcpy(void *dest,void *src,HUFF *hp,int32_t datalen);
int32_t hcalc_bitsize(uint64_t x);
int32_t hconv_bitlen(uint32_t bitlen);
int32_t huffcompress(uint8_t *dest,int32_t maxlen,uint8_t *src,int32_t len,int32_t maxpatlen);
int32_t huffexpand(uint8_t *destbuf,int32_t maxlen,uint8_t *src,int32_t len);
void *huff_hexcode(uint8_t *data,int32_t datalen);

#endif
#else
#ifndef crypto777_huffstream_c
#define crypto777_huffstream_c

#ifndef crypto777_huffstream_h
#define DEFINES_ONLY
#include "bits777.c"
#include "huffstream.c"
#undef DEFINES_ONLY
#endif
extern int32_t Debuglevel;

static const uint8_t huffmasks[8] = { (1<<0), (1<<1), (1<<2), (1<<3), (1<<4), (1<<5), (1<<6), (1<<7) };
static const uint8_t huffoppomasks[8] = { ~(1<<0), ~(1<<1), ~(1<<2), ~(1<<3), ~(1<<4), ~(1<<5), ~(1<<6), (uint8_t)~(1<<7) };

void _init_HUFF(HUFF *hp,int32_t allocsize,void *buf) {  hp->buf = hp->ptr = buf, hp->allocsize = allocsize, hp->bitoffset = 0; }

int32_t hconv_bitlen(uint32_t bitlen)
{
    int32_t len;
    len = (int32_t)(bitlen >> 3);
    if ( ((int32_t)bitlen & 7) != 0 )
        len++;
    return(len);
}

int32_t hupdate_internals(HUFF *hp)
{
    int32_t retval = 0;
    if ( (hp->bitoffset >> 3) > hp->allocsize )
    {
        printf("hupdate_internals: ERROR: bitoffset.%d -> %d >= allocsize.%d\n",hp->bitoffset,hp->bitoffset>>3,hp->allocsize);
        //getchar();
        hp->bitoffset = (hp->allocsize << 3) - 1;
        retval = -1;
    }
    if ( hp->bitoffset > hp->endpos )
        hp->endpos = hp->bitoffset;
    hp->ptr = &hp->buf[hp->bitoffset >> 3];
    hp->maski = (hp->bitoffset & 7);
    return(retval);
}

int32_t hseek(HUFF *hp,int32_t offset,int32_t mode)
{
    if ( mode == SEEK_END )
        hp->bitoffset = (offset + hp->endpos);
    else if ( mode == SEEK_SET )
        hp->bitoffset = offset;
    else hp->bitoffset += offset;
    if ( hupdate_internals(hp) < 0 )
    {
        printf("hseek.%d: illegal offset.%d %d >= allocsize.%d\n",mode,offset,offset>>3,hp->allocsize);
        return(-1);
    }
    return(0);
}

void hclear(HUFF *hp,int32_t clearbuf)
{
    hp->bitoffset = 0;
    hupdate_internals(hp);
    hp->endpos = 0;
    if ( clearbuf != 0 )
        memset(hp->buf,0,hp->allocsize);
}

int32_t hgetbit(HUFF *hp)
{
    int32_t bit = 0;
    //printf("hp.%p ptr.%ld buf.%ld maski.%d\n",hp,(long)hp->ptr-(long)hp->buf,(long)hp->buf-(long)hp,hp->maski);
    if ( hp->bitoffset < hp->endpos )
    {
        if ( (*hp->ptr & huffmasks[hp->maski++]) != 0 )
            bit = 1;
        hp->bitoffset++;
        if ( hp->maski == 8 )
            hp->maski = 0, hp->ptr++;
        //fprintf(stderr,"<-%d ",bit);
        return(bit);
    }
    printf("hgetbit past EOF: %d >= %d\n",hp->bitoffset,hp->endpos);//, getchar();
    return(-1);
}

int32_t hputbit(HUFF *hp,int32_t bit)
{
    // printf("->%d ",bit);
    if ( bit != 0 )
        *hp->ptr |= huffmasks[hp->maski];
    else *hp->ptr &= huffoppomasks[hp->maski];
    if ( ++hp->maski >= 8 )
        hp->maski = 0, hp->ptr++;
    if ( ++hp->bitoffset > hp->endpos )
        hp->endpos = hp->bitoffset;
    if ( (hp->bitoffset>>3) >= hp->allocsize )
    {
        printf("hwrite: bitoffset.%d >= allocsize.%d\n",hp->bitoffset,hp->allocsize);
        hp->bitoffset--;
        hupdate_internals(hp);
        return(-1);
    }
    return(0);
}

int32_t hwrite(uint64_t codebits,int32_t numbits,HUFF *hp)
{
    int32_t i;
    for (i=0; i<numbits; i++,codebits>>=1)
        if ( hputbit(hp,codebits & 1) < 0 )
            return(-1);
    return(numbits);
}

uint64_t hread(int32_t *numbitsp,int32_t numbits,HUFF *hp)
{
    int32_t i,bit; uint64_t codebits = 0;
    for (i=0; i<numbits; i++)
    {
        codebits <<= 1;
        if ( (bit= hgetbit(hp)) < 0 )
            break;
        codebits |= bit;
    }
    *numbitsp = i;
    return(codebits);
}

int32_t hmemcpy(void *dest,void *src,HUFF *hp,int32_t datalen)
{
    if ( (hp->bitoffset & 7) != 0 || ((hp->bitoffset>>3) + datalen) > hp->allocsize )
    {
        printf("misaligned hmemcpy bitoffset.%d or overflow allocsize %d vs %d\n",hp->bitoffset,hp->allocsize,((hp->bitoffset>>3) + datalen));
        getchar();
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

int32_t hcalc_bitsize(uint64_t x)
{
    uint64_t mask = ((uint64_t)1 << 63);
    int32_t i;
    if ( x == 0 )
        return(1);
    for (i=63; i>=0; i--,mask>>=1)
    {
        if ( (mask & x) != 0 )
            return(i+1);
    }
    return(-1);
}
#define MAX_PATTERNLEN 30
#define MAX_HUFFRECURSIONS 1

struct huffnumbers { uint32_t altsize,maxitems,nonz,biggest,diff; int32_t *vals,*treemap,freq[]; };

struct huffbits { uint32_t numbits:5,bits:22,patlen:5; };
struct huffitem { struct huffbits code; uint32_t freq; uint8_t pattern[MAX_PATTERNLEN+1]; };
struct huffpair { struct huffbits *items; struct huffcode *code; int32_t maxind,nonz,count,wt; uint64_t minval,maxval; char name[16]; };

struct huffcode
{
    double totalbits,totalbytes,freqsum;
    int32_t numitems,maxbits,numnodes,depth,allocsize;
    int32_t tree[];
};

struct huffstats
{
    uint8_t *ptrs[MAX_PATTERNLEN+1][65536],checkbuf[65536];
    uint32_t freq[MAX_PATTERNLEN+1][65536];
    uint32_t marked[65536],numpats[MAX_PATTERNLEN+1];
};

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

int32_t hcalc_smallsize(uint16_t val)
{
    if ( val >= 4 )
    {
        if ( val < (1 << 5) )
            return(8);
        else if ( val < (1 << 8) )
            return(11);
        else if ( val < (1 << 12) )
            return(15);
        else return(19);
    } else return(3);
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
    //printf("decoded smallbits.(%d)\n",*valp);
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

int32_t hemit_svarbits(HUFF *hp,uint32_t val)
{
    if ( val < (1 << 16) )
    {
        hputbit(hp,0);
        return(hemit_smallbits(hp,val));
    }
    else
    {
        hputbit(hp,1);
        return(hemit_varbits(hp,val));
    }
}

int32_t hdecode_svarbits(uint32_t *valp,HUFF *hp)
{
    int32_t retval; uint16_t sval;
    if ( hgetbit(hp) == 0 )
    {
        retval = hdecode_smallbits(&sval,hp);
        *valp = sval;
        return(retval);
    }
    else return(hdecode_varbits(valp,hp));
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

/*HUFF *hopen(char *coinstr,struct alloc_space *mem,uint8_t *bits,int32_t num,int32_t allocated)
{
    HUFF *hp = (SUPERNET.MAP_HUFF != 0) ? permalloc(coinstr,mem,sizeof(*hp),1) : calloc(1,sizeof(*hp));
    hp->ptr = hp->buf = bits;
    hp->allocsize = num;
    hp->endpos = (num << 3);
    if ( allocated != 0 )
        hp->allocated = 1;
    return(hp);
}*/

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

int32_t huff_dispitem(int32_t dispflag,const struct huffitem *item)
{
    int32_t n = item->code.numbits;
    uint64_t codebits;
    if ( n != 0 )
    {
        if ( dispflag != 0 )
        {
            codebits = huff_reversebits(item->code.bits,n);
            printf("%06llx: freq.%-6d (%8s).%d\n",*(uint64_t *)item->pattern,item->freq,huff_str(codebits,n),n);
        }
        return(1);
    }
    return(0);
}

void huff_disp(int32_t verboseflag,struct huffcode *huff,struct huffitem *items)
{
    int32_t i,n = 0;
    if ( huff->maxbits >= 1024 )
        printf("huff->maxbits %d >= %d sizeof(strbit)\n",huff->maxbits,1024);
    for (i=0; i<huff->numitems; i++)
        n += huff_dispitem(verboseflag,&items[i]);
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

int32_t huff_init(double *freqsump,int32_t **predsp,uint32_t **revtreep,struct huffitem *items,int32_t numitems)
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
        efreqs[i] = items[i].freq;// * ((items[i]->wt != 0) ? items[i]->wt : 1);
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
            freqsum += items[i].freq;
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

int32_t huff_calc(int32_t dispflag,int32_t *nonzp,double *totalbytesp,double *totalbitsp,int32_t *preds,struct huffitem *items,int32_t numitems)
{
    int32_t i,ix,pred,numbits,nonz,maxbits = 0;
    double totalbits,totalbytes;
    struct huffitem *item;
    uint64_t codebits;
    totalbits = totalbytes = 0.;
    for (i=nonz=0; i<numitems; i++)
    {
        codebits = numbits = 0;
        if ( items[i].freq != 0 )
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
            if ( (item= &items[i]) != 0 )
            {
                //item->codebits = huff_reversebits(codebits,numbits);
                //item->numbits = numbits;
                item->code.numbits = numbits;
                item->code.bits = (uint32_t)huff_reversebits(codebits,numbits);
                //item->code.rawind = item->rawind;
                if ( numbits > 32 || item->code.bits != huff_reversebits(codebits,numbits) )
                    printf("error setting code.bits: item->code.numbits %d != %d numbits || item->code.bits %u != %llu _reversebits(codebits,numbits) || pattern %06llx\n",item->code.numbits,numbits,item->code.bits,(long long)huff_reversebits(codebits,numbits),(long long)*(int64_t *)item->pattern);
                totalbits += numbits * item->freq;
                totalbytes += item->code.patlen * item->freq;
                if ( dispflag != 0 )
                {
                    printf("Total %.0f -> %.0f: ratio %.3f ave %.1f bits/item |",totalbytes,totalbits,totalbytes*8/(totalbits+1),totalbits/nonz);
                    huff_dispitem(dispflag,&items[i]);
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

struct huffcode *huff_create(int32_t dispflag,struct huffitem *items,int32_t numitems)
{
    int32_t ix2,i,n,extf,*preds,allocsize;
    uint32_t *revtree;
    double freqsum;
    struct huffcode *huff;
    if ( (extf= huff_init(&freqsum,&preds,&revtree,items,numitems)) <= 0 )
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
    huff->maxbits = huff_calc(dispflag,&huff->numnodes,&huff->totalbytes,&huff->totalbits,preds,items,numitems);
    free(preds);
    return(huff);
}

void *huff_hexcode(uint8_t *data,int32_t datalen)
{
    int32_t i,histo[16]; struct huffitem items[16];
    memset(histo,0,sizeof(histo));
    memset(items,0,sizeof(items));
    for (i=0; i<datalen; i++)
        histo[data[i]&0xf]++, histo[(data[i]>>4)&0xff]++;
    for (i=0; i<16; i++)
    {
        items[i].code.patlen = 4;
        items[i].freq = histo[i];
        printf("%4d ",histo[i]);
    }
    printf("cipherhisto\n");
    return(huff_create(1,items,16));
}

int32_t huff_encode(struct huffcode *huff,HUFF *hp,struct huffitem *items,uint32_t *itemis,int32_t num)
{
    uint64_t codebits;
    struct huffitem *item;
    int32_t i,j,n,count = 0;
	for (i=0; i<num; i++)
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

struct huffitem *huff_decodeitem(int32_t *hufftree,int32_t depth,int32_t numitems,struct huffitem *items,HUFF *hp)
{
    uint32_t ind;
    int32_t i,c,*tree,tmp,num = 0;
    if ( items == 0 )
        return(0);
	while ( 1 )
    {
        tree = hufftree;
        for (i=c=0; i<depth; i++)
        {
            if ( (c= hgetbit(hp)) < 0 )
                return(0);
            num++;
            if ( Debuglevel > 1 )
                printf("[%c %d]",c+'0',hp->bitoffset);
            if ( (tmp= tree[c]) < 0 )
            {
                tree -= tmp;
                continue;
            }
            ind = tmp;
            if ( ind >= numitems )
            {
                printf("decode error: val.%x -> ind.%d >= numitems %d\n",c,ind,numitems);
                return(0);
            }
            if ( Debuglevel > 1 )
                printf("{%d} ",ind);
            return(&items[ind]);
        }
	}
    printf("huffpair_decodeitemind error num.%d\n",num);
    return(0);
}

struct huffnumbers *huffnumbers_init(int32_t numvals,int32_t maxval,int32_t diff)
{
    struct huffnumbers *hn;
    int32_t allocitems;
    allocitems = ((maxval+1) << diff);
    hn = calloc(1,sizeof(*hn) + sizeof(*hn->freq)*allocitems + numvals*2*sizeof(int32_t));
    hn->maxitems = allocitems;
    hn->diff = diff;
    hn->treemap = &hn->freq[allocitems];
    hn->vals = &hn->treemap[numvals];
    return(hn);
}

struct huffitem *huffnumbers_items(struct huffnumbers *hn)
{
    struct huffitem *item,*items = 0;
    uint32_t i,n = 0;
    if ( hn->nonz != 0 )
    {
        items = calloc(hn->nonz,sizeof(*items));
        for (i=0; i<=hn->biggest; i++)
        {
            if ( hn->freq[i] != 0 )
            {
                item = &items[n];
                item->code.patlen = sizeof(uint32_t);
                item->freq = hn->freq[i];
                hn->treemap[i] = n;
                memcpy(item->pattern,&i,item->code.patlen);
                n++;
            }
        }
        if ( n != hn->nonz )
        {
            printf("huffnumbers_items: FATAL n.%d != hn->nonz %d\n",n,hn->nonz);
            free(items);
            items = 0;
        }
    }
    return(items);
}

void hdecode_numbers(int32_t iter,int32_t dispflag,struct huffcode *huff,HUFF *hp)
{
    struct huffcode *hdecode_header(int32_t iter,int32_t dispflag,struct huffitem **itemsp,HUFF *hp);
    int16_t val;
    int32_t i,diff,minval;
    struct huffcode *treehuff;
    struct huffitem *treeitems,*item;
    iter++;
    diff = hgetbit(hp);
    hdecode_smallbits((uint16_t *)&val,hp), minval = val;
    if ( hgetbit(hp) == 0 )
    {
        printf("diff.%d simple path minval.%d\n",diff,minval);
        for (i=0; i<(huff->depth << 1); i++)
        {
            hdecode_smallbits((uint16_t *)&val,hp);
            huff->tree[i] = (val + minval);
            if ( Debuglevel > 0 )
                printf("%d ",huff->tree[i]);
        }
        if ( Debuglevel > 0 )
            printf("\n");
    }
    else
    {
        printf("diff.%d complex path minval.%d\n",diff,minval);
        if ( (treehuff= hdecode_header(iter,dispflag,&treeitems,hp)) != 0 )
        {
            for (i=0; i<(huff->depth << 1); i++)
            {
                if ( (item= huff_decodeitem(treehuff->tree,treehuff->depth,treehuff->numitems,treeitems,hp)) != 0 )
                {
                    memcpy(&huff->tree[i],item->pattern,item->code.patlen);
                    printf("numbits.%d %llx).%d ",item->code.numbits,(long long)item->code.bits,huff->tree[i]);
                }
            }
            free(treehuff);
            free(treeitems);
        }
    }
    if ( diff != 0 )
    {
        for (i=1; i<(huff->depth << 1); i++)
        {
            huff->tree[i] += huff->tree[i - 1];
            printf("%d ",huff->tree[i]);
        }
    }
}

int32_t hemit_numbers(int32_t iter,int32_t dispflag,HUFF *hp,int32_t *rawvals,int32_t numvals,int32_t maxval,int32_t diff)
{
    int32_t hemit_header(int32_t iter,int32_t dispflag,HUFF *hp,struct huffcode *huff,struct huffitem *items,int32_t numitems);
    struct huffnumbers *hn = 0;
    struct huffitem *item,*items = 0;
    struct huffcode *huff = 0;
    int32_t i,j,n,minval,val,origbitoffset,numbits = 2;
    uint64_t codebits;
    if ( Debuglevel > 0 )
        printf("emitnumbers iter.%d numvals.%d maxval.%d diff.%d\n",iter,numvals,maxval,diff);
    hputbit(hp,diff != 0);
    origbitoffset = hp->bitoffset;
    if ( (hn= huffnumbers_init(numvals,maxval,diff)) != 0 )
    {
        for (i=minval=0; i<numvals; i++)
        {
            hn->vals[i] = val = (diff == 1 && i > 0) ? rawvals[i] - rawvals[i-1] : rawvals[i];
            if ( val < minval )
                minval = val;
        }
        if ( Debuglevel > 0 )
            printf("MINVAL.%d\n",minval);
        hemit_smallbits(hp,minval);
        origbitoffset = hp->bitoffset;
        for (i=0; i<numvals; i++)
        {
            hn->vals[i] = val = (hn->vals[i] - minval);
            if ( val >= hn->maxitems || val < 0 )
            {
                fprintf(stderr,"illegal rawals[%d] %d >= %d\n",i,val,maxval);
                getchar();
                return(-1);
            }
            if ( hn->freq[val]++ == 0 )
                hn->nonz++;
            if ( val > hn->biggest )
                hn->biggest = val;
            hn->altsize += hcalc_smallsize(val);
            if ( Debuglevel > 0 )
                fprintf(stderr,"iter.%d i.%d val %d -> %d\n",iter,i,rawvals[i],val);
        }
        if ( iter < MAX_HUFFRECURSIONS && (items= huffnumbers_items(hn)) != 0 )
        {
            if ( Debuglevel > 0 )
                printf("COMPLEX\n");
            hputbit(hp,1);
            if ( (huff= huff_create(dispflag,items,hn->nonz)) != 0 )
            {
                if ( (n= hemit_header(iter,dispflag,hp,huff,items,hn->nonz)) > 0 )
                {
                    numbits += n;
                    for (i=0; i<numvals; i++)
                    {
                        if ( (item= &items[hn->vals[i]]) != 0 )
                        {
                            n = item->code.numbits;
                            codebits = item->code.bits;
                            numbits += n + hemit_smallbits(hp,n);
                            for (j=0; j<n; j++,codebits>>=1)
                                hputbit(hp,codebits & 1);
                        }
                    }
                }
                free(huff);
            }
            free(items);
        } else if ( iter < MAX_HUFFRECURSIONS ) printf("huffsetheader: cant make treeitems!\n");
        if ( Debuglevel > 0 )
            printf("iter.%d numbits.%d altsize.%d vs %d (%d - %d)\n",iter,hn->altsize,numbits,hp->bitoffset - origbitoffset,hp->bitoffset,origbitoffset);
        if ( iter >= MAX_HUFFRECURSIONS || hn->altsize < (hp->bitoffset - origbitoffset) )
        {
            hseek(hp,origbitoffset,SEEK_SET);
            hputbit(hp,0);
            numbits = 2;
            for (i=0; i<numvals; i++)
                numbits += hemit_smallbits(hp,hn->vals[i]);
            printf("smallbits -> %d vs altsize.%d\n",numbits,hn->altsize);
        }
        free(hn);
    } else printf("hemit_numbers error\n");
    return(numbits);
}

uint8_t *hcheckpoint(int32_t *savedsizep,HUFF *hp,int32_t bitoffset)
{
    uint8_t *savedbuf;
    *savedsizep = hconv_bitlen(hp->bitoffset);
    savedbuf = malloc(*savedsizep);
    memcpy(savedbuf,hp->buf,*savedsizep);
    if ( bitoffset != hp->bitoffset )
        hseek(hp,bitoffset,SEEK_SET), hp->endpos = hp->bitoffset;
    return(savedbuf);
}

void hrestore(HUFF *hp,uint8_t *savedbuf,int32_t savedsize,int32_t restoreoffset)
{
    memcpy(hp->buf,savedbuf,savedsize);
    hseek(hp,restoreoffset,SEEK_SET);
    free(savedbuf);
}

int32_t huff_emitbestdiff(int32_t iter,int32_t dispflag,HUFF *hp,int32_t *rawvals,int32_t numvals,int32_t maxval)
{
    int32_t savedsize,normalsize,numbits = 0,origbitoffset = hp->bitoffset;
    uint8_t *savedbuf = 0;
    hemit_numbers(iter,dispflag,hp,rawvals,numvals,maxval,0);
    normalsize = (hp->bitoffset - origbitoffset);
    if ( 0 )
    {
        savedbuf = hcheckpoint(&savedsize,hp,origbitoffset);
        hemit_numbers(iter,dispflag,hp,rawvals,numvals,maxval,1);
        numbits = (hp->bitoffset - origbitoffset);
        printf("iter.%d normalsize.%d vs diffsize.%d | numvals.%d\n",iter,normalsize,numbits,numvals);
        if ( numbits > normalsize )
            hrestore(hp,savedbuf,savedsize,origbitoffset+normalsize), numbits = normalsize;
        else free(savedbuf);
    }
    return(numbits);
}

int32_t hchoose(HUFF *hp,int32_t origbitoffset,uint8_t *savedbuf,int32_t savedsize,int32_t numbits)
{
    int32_t simplesize;
    simplesize = (hp->bitoffset - origbitoffset);
    if ( simplesize != 0 && savedbuf != 0 && simplesize > numbits )
        hrestore(hp,savedbuf,savedsize,origbitoffset + numbits);
    else if ( savedbuf != 0 )
        free(savedbuf);
    return(hp->bitoffset - origbitoffset);
}

int32_t huffitems_emit(int32_t iter,int32_t dispflag,HUFF *hp,struct huffitem *items,int32_t numitems)
{
    //struct huffbits { uint32_t numbits:5,bits:22,patlen:5; };
    int32_t i,j,origbitoffset,maxval,numbits = 0,*rawvals,savedsize = 0;
    uint8_t *savedbuf = 0;
    struct huffitem *item;
    origbitoffset = hp->bitoffset;
    if ( 0 && iter < MAX_HUFFRECURSIONS )
    {
        rawvals = calloc(numitems,sizeof(*rawvals));
        hputbit(hp,1);
        for (i=maxval=0; i<numitems; i++)
            if ( (rawvals[i]= items[i].code.numbits) > maxval )
                maxval = rawvals[i];
        //fprintf(stderr,"numbits\n");
        huff_emitbestdiff(MAX_HUFFRECURSIONS,dispflag,hp,rawvals,numitems,maxval+2);
        for (i=maxval=0; i<numitems; i++)
            if ( (rawvals[i]= items[i].code.bits) > maxval )
                maxval = rawvals[i];
        //fprintf(stderr,"codebits\n");
        huff_emitbestdiff(MAX_HUFFRECURSIONS,dispflag,hp,rawvals,numitems,maxval+2);
        for (i=maxval=0; i<numitems; i++)
            if ( (rawvals[i]= items[i].code.patlen) > maxval )
                maxval = rawvals[i];
        //fprintf(stderr,"patlen\n");
        huff_emitbestdiff(MAX_HUFFRECURSIONS,dispflag,hp,rawvals,numitems,maxval+2);
        numbits = (hp->bitoffset - origbitoffset);
        savedbuf = hcheckpoint(&savedsize,hp,origbitoffset);
        free(rawvals);
    }
    hputbit(hp,0); // smallbits
    for (i=0; i<numitems; i++)
    {
        item = &items[i];
        hemit_smallbits(hp,item->code.numbits);
        hemit_bits(hp,item->code.bits,item->code.numbits);
        hemit_smallbits(hp,item->code.patlen);
        for (j=0; j<item->code.patlen; j++)
            hemit_smallbits(hp,item->pattern[j]);
    }
    return(hchoose(hp,origbitoffset,savedbuf,savedsize,numbits));
}

void hdecode_items(int32_t dispflag,struct huffitem *items,int32_t numitems,struct huffcode *huff,HUFF *hp)
{
    int32_t i,j;
    uint16_t sval;
    uint32_t val;
    struct huffitem *item;
    if ( hgetbit(hp) == 0 )
    {
        for (i=0; i<numitems; i++)
        {
            item = &items[i];
            hdecode_smallbits(&sval,hp), item->code.numbits = sval;
            hdecode_bits(&val,hp,sval), item->code.bits = val;
            hdecode_smallbits(&sval,hp), item->code.patlen = sval;
            for (j=0; j<item->code.patlen; j++)
                hdecode_smallbits(&sval,hp), item->pattern[j] = sval;
        }
    }
    else
    {
        printf("hdecode_items complex case\n");
    }
}

int32_t hemit_header(int32_t iter,int32_t dispflag,HUFF *hp,struct huffcode *huff,struct huffitem *items,int32_t numitems)
{
    int32_t i,origbitoffset,minval,normalsize = 0,savedsize = 0,numbits = 0;
    uint8_t *savedbuf = 0;
    if ( Debuglevel > 0 )
        printf(">>>>>>>>>>>>> EMITHDR.%d numitems.%d depth.%d\n",iter,numitems,huff->depth);
    iter++;
    numbits += hemit_svarbits(hp,numitems);
    numbits += hemit_svarbits(hp,huff->depth);
    origbitoffset = hp->bitoffset;
    if ( 0 && iter < MAX_HUFFRECURSIONS )
    {
        normalsize = huff_emitbestdiff(iter,dispflag,hp,huff->tree,huff->depth<<1,(huff->depth<<2)+1);
        savedbuf = hcheckpoint(&savedsize,hp,origbitoffset);
    }
    if ( 1 )
    {
        minval = 0;
        hputbit(hp,0); // no diff
        hemit_smallbits(hp,minval);
        hputbit(hp,0); // smallbits
        for (i=0; i<(huff->depth<<1); i++)
            hemit_smallbits(hp,huff->tree[i]-minval);
        if ( Debuglevel > 0 )
            printf("hchoose at bitoffset.%d\n",hp->bitoffset);
    }
    hchoose(hp,origbitoffset,savedbuf,savedsize,normalsize);
    huffitems_emit(iter,dispflag,hp,items,numitems);
    numbits = (hp->bitoffset - origbitoffset);
    if ( Debuglevel > 0 )
        printf("iter.%d numitems.%d depth.%d huffsize %d normalsize.%d -> numbits.%d\n",iter,numitems,huff->depth,hp->bitoffset-origbitoffset,normalsize,numbits);
    return(numbits);
}

struct huffcode *hdecode_header(int32_t iter,int32_t dispflag,struct huffitem **itemsp,HUFF *hp)
{
    uint32_t numitems,depth;
    struct huffcode *huff = 0;
    struct huffitem *items;//,*item;
    hdecode_svarbits(&numitems,hp);
    hdecode_svarbits(&depth,hp);
    if ( Debuglevel > 0 )
        printf("numitems.%d depth.%d\n",numitems,depth);
    if ( depth < (1 << 16) )
    {
        huff = calloc(1,sizeof(*huff) + depth*sizeof(*huff->tree)*2);
        huff->depth = depth;
        huff->numitems = numitems;
        if ( numitems < (1 << 16) )
        {
            (*itemsp) = items = calloc(numitems,sizeof(struct huffitem));
            hdecode_numbers(iter,dispflag,huff,hp);
            hdecode_items(dispflag,items,numitems,huff,hp);
        } else *itemsp = 0, printf("hdecode_header: ERROR numitems.%d depth.%d\n",numitems,depth);
    } else printf("hdecode_header: ERROR2 numitems.%d depth.%d\n",numitems,depth);
    return(huff);
}

#define ALPHABET_LEN 256
#define NOT_FOUND patlen
#define max(a, b) ((a < b) ? b : a)

void make_delta1(int32_t *delta1,uint8_t *pat,int32_t patlen)
{
    int32_t i;
    for (i=0; i<ALPHABET_LEN; i++)
        delta1[i] = NOT_FOUND;
    for (i=0; i<patlen-1; i++)
        delta1[pat[i]] = patlen-1 - i;
}

int32_t is_prefix(uint8_t *word,int32_t wordlen,int32_t pos)
{
    int32_t i,suffixlen = wordlen - pos;
    for (i=0; i<suffixlen; i++)
        if ( word[i] != word[pos+i] )
            return(0);
    return(1);
}

int suffix_length(uint8_t *word,int32_t wordlen,int32_t pos)
{
    int32_t i;
    for (i=0; (word[pos-i] == word[wordlen-1-i]) && (i < pos); i++)
        ;
    return(i);
}

void make_delta2(int32_t *delta2,uint8_t *pat,int32_t patlen)
{
    int32_t p,slen,last_prefix_index = patlen-1;
    for (p=patlen-1; p>=0; p--)
    {
        if ( is_prefix(pat, patlen,p+1))
            last_prefix_index = p+1;
        delta2[p] = last_prefix_index + (patlen-1 - p);
    }
    for (p=0; p < patlen-1; p++)
    {
        slen = suffix_length(pat,patlen,p);
        if ( pat[p - slen] != pat[patlen-1 - slen] )
            delta2[patlen-1 - slen] = patlen-1 - p + slen;
    }
}

uint8_t *boyer_moore(uint8_t *string,uint32_t stringlen,uint8_t *pat,uint32_t patlen)
{
    int32_t i,j,delta1[ALPHABET_LEN],delta2[8192];
    if ( patlen > (uint32_t)(sizeof(delta2)/sizeof(*delta2)) )
        return(0);
    make_delta1(delta1,pat,patlen);
    make_delta2(delta2,pat,patlen);
    if ( patlen == 0 )
        return(string);
    i = patlen - 1;
    while ( i < stringlen )
    {
        j = patlen-1;
        while ( j >= 0 && (string[i] == pat[j]) )
            --i, --j;
        if ( j < 0 )
            return(string + i+1);
        i += max(delta1[string[i]],delta2[j]);
    }
    return(NULL);
}

int32_t _srchpats(uint8_t *ptrs[],uint8_t numpats,uint8_t *pat,int32_t patlen)
{
    int32_t i;
    for (i=0; i<numpats; i++)
        if ( memcmp(ptrs[i],pat,patlen) == 0 )
            return(i);
    return(-1);
}

int32_t huff_findpatterns(struct huffstats *hstats,int32_t maxpatlen,uint8_t *src,int32_t len)
{
    int32_t i,patlen,n = 0;
    uint8_t *ptr;
    for (i=0; i<len; i++)
    {
        if ( Debuglevel > 0 )
            printf("%d ",src[i]);
        for (patlen=1; patlen<=maxpatlen; patlen++)
        {
            if ( (ptr= boyer_moore(&src[i],len-i,&src[i],patlen)) != 0 )
            {
                //printf("{%d}.%d ",src[i],i);
                if ( _srchpats(hstats->ptrs[patlen],hstats->numpats[patlen],&src[i],patlen) < 0 )
                    hstats->ptrs[patlen][hstats->numpats[patlen]++] = ptr, n++;
            }
        }
    }
    if ( Debuglevel > 0 )
    {
        for (patlen=1; patlen<=maxpatlen; patlen++)
            printf("%d ",hstats->numpats[patlen]);
        printf("numpatterns\n");
    }
    return(n);
}

int32_t huff_setmarked(struct huffstats *hstats,int32_t maxpatlen,uint8_t *src,int32_t len)
{
    int32_t patlen,i,j,n,offset,tablesize,nonz,total,numitems,firstoffset;
    uint32_t *marked = hstats->marked;
    uint8_t *srcptr,*ptr;
    tablesize = nonz = 0;
    for (patlen=maxpatlen; patlen>=1; patlen--)
    {
        if ( (n= hstats->numpats[patlen]) > 0 )
        {
            for (i=0; i<n; i++)
            {
                srcptr = src;
                firstoffset = -1;
                while ( (ptr= boyer_moore(srcptr,(int32_t)((long)&src[len] - (long)srcptr),hstats->ptrs[patlen][i],patlen)) != 0 )
                {
                    offset = (int32_t)((long)ptr - (long)src);
                    if ( hstats->freq[patlen][i] == 0 )
                        firstoffset = offset;
                    for (j=0; j<patlen; j++)
                        if ( marked[offset+j] != 0 )
                            break;
                    //printf("(%d) ",offset);
                    if ( j == patlen )
                    {
                        for (j=0; j<patlen; j++,offset++)
                        {
                            if ( marked[offset] != 0 )
                                printf("unexpected entry at %d %x\n",offset,marked[offset]);
                            marked[offset] = (i << 16) | (patlen << 8) | (j << 1) | (hstats->freq[patlen][i] == 0);
                            //printf("%d ",offset);
                            nonz++;
                        }
                        if ( hstats->freq[patlen][i]++ == 0 )
                        {
                            tablesize += patlen;
                            if ( Debuglevel > 0 )
                                printf("[%d] ",hstats->ptrs[patlen][i][0]);
                        }
                    } else offset++;// += patlen;
                    for (j=offset; j<len; j++)
                        if ( marked[j] == 0 )
                            break;
                    srcptr = &src[j];
                }
                if ( 1 && patlen > 1 && hstats->freq[patlen][i] == 1 )
                {
                    for (j=0; j<patlen; j++)
                    {
                        marked[firstoffset+j] = 0;
                        if ( Debuglevel > 0 )
                            printf("-%d ",firstoffset+j);
                    }
                    hstats->freq[patlen][i] = 0;
                    nonz -= patlen;
                    tablesize -= patlen;
                }
            }
        }
    }
    if ( Debuglevel > 0 )
        printf("marked\n");
    numitems = total = 0;
    for (patlen=1; patlen<=maxpatlen; patlen++)
    {
        n = hstats->numpats[patlen];
        for (i=0; i<n; i++)
            if ( hstats->freq[patlen][i] != 0 )
                numitems++, total += hstats->freq[patlen][i]; //printf("(%d %d) ",i,freq[patlen][i]),
    }
    if ( Debuglevel > 0 )
        printf("numitems.%d tokens.%d | tablesize.%d nonz.%d\n",numitems,total,tablesize,nonz);
    return(numitems);
}

HUFF *huffsetitems(uint8_t *dest,int32_t maxlen,struct huffstats *hstats,struct huffitem *items,int32_t numitems,int32_t maxpatlen)
{
    HUFF *hp = calloc(1,sizeof(*hp));
    uint16_t slen;
    int32_t newlen,patlen,i,n,nonz = 0;
    struct huffitem *item;
    _init_HUFF(hp,maxlen,dest);
    //hp = hopen(dest,maxlen,0);
    for (nonz=0,patlen=1; patlen<=maxpatlen; patlen++)
    {
        n = hstats->numpats[patlen];
        for (i=0; i<n; i++)
            if ( hstats->freq[patlen][i] != 0 )
            {
                item = &items[nonz];
                item->freq = hstats->freq[patlen][i] * patlen;
                item->code.patlen = patlen;
                memset(item->pattern,0,sizeof(item->pattern));
                memcpy(item->pattern,hstats->ptrs[patlen][i],patlen);
                if ( hp->buf[0] == 0 )
                {
                    hp->buf[0] = 'R';
                    slen = hstats->freq[patlen][i];
                    memcpy(&hp->buf[1],&slen,sizeof(slen));
                    hp->buf[1 + sizeof(slen)] = patlen;
                    //printf("patlen.%d -> %p\n",patlen,&dest[1 + sizeof(slen)]);
                    memcpy(&hp->buf[1 + sizeof(slen) + 1],hstats->ptrs[patlen][i],patlen);
                    newlen = (int32_t)(1 + sizeof(slen) + 1 + patlen);
                    hp->bitoffset = 8 * newlen;
                    hupdate_internals(hp);
                }
                nonz++;
                hstats->freq[patlen][i] = nonz;
            }
    }
    if ( Debuglevel > 0 )
        printf("patterns\n");
    return(hp);
}

int32_t _huffexpand(uint8_t *destbuf,HUFF *hp,uint8_t *src,int32_t len)
{
    struct huffcode *hdecode_header(int32_t iter,int32_t dispflag,struct huffitem **itemsp,HUFF *hp);
    int32_t i,j,patlen,offset,newlen,err = 0;
    struct huffitem *item,*items = 0;
    struct huffcode *huff = 0;
    uint8_t pattern[MAX_PATTERNLEN+1];
    uint16_t slen;
    j = i = 0;
    newlen = hconv_bitlen(hp->endpos);
    memcpy(&slen,&hp->buf[1],sizeof(slen));
    if ( hp->buf[0] == 'H' )
    {
        huff = hdecode_header(0,Debuglevel,&items,hp);
        //printf("expand loop depth.%d numitems.%d\n",huff->depth,huff->numitems);
        while ( (item= huff_decodeitem(huff->tree,huff->depth,huff->numitems,items,hp)) != 0 )
        {
            //printf("%s ",huff_str(item->code.bits,item->code.numbits));
            //printf("(%ld).%d ",((long)item-(long)items)/sizeof(*item),hp->bitoffset);
            if ( Debuglevel > 0 )
                printf("[%d %d].%d ",item->pattern[0],j,i);
            memcpy(&destbuf[j],item->pattern,item->code.patlen);
            j += item->code.patlen;
            i++;
        }
        free(huff);
    }
    else if ( hp->buf[0] == 'R' )
    {
        patlen = hp->buf[1 + sizeof(slen)];
        printf("patlen.%d <- %p\n",patlen,&hp->buf[1 + sizeof(slen)]);
        if ( patlen < MAX_PATTERNLEN )
        {
            memcpy(pattern,&hp->buf[1 + sizeof(slen) + 1],patlen);
            newlen = (int32_t)(1 + sizeof(slen) + 1 + patlen);
            for (i=offset=0; i<slen; i++,offset+=patlen)
                memcpy(&destbuf[offset],pattern,patlen);
            //printf("newlen.%d patlen.%d run.%d %llx\n",newlen,patlen,slen,(long long)pattern);
        } else offset = 0;
        j = offset;
    }
    else if ( hp->buf[0] == 'P' )
        memcpy(destbuf,&hp->buf[1+sizeof(slen)],slen), newlen = slen;
    else printf("huffexpand unknown type in [0] %d (%c)\n",hp->buf[0],hp->buf[0]);
    if ( src != 0 && len > 0 && (memcmp(src,destbuf,len) != 0 || len != j) )
    {
        printf(" j.%d vs hp.%d\n",j,hp->bitoffset);
        for (i=0; i<len; i++)
            if ( src[i] != destbuf[i] )
            {
                if ( err++ < 100 )
                    printf("(%d %d).%d ",src[i],destbuf[i],i);
            }
        printf("len.%d checklen.%d newlen.%d | errs.%d\n",len,j,newlen,err);
        getchar();
    }
    if ( err == 0 )
        return(newlen);
    return(-err);
}

int32_t huffexpand(uint8_t *destbuf,int32_t maxlen,uint8_t *data,int32_t complen)
{
    HUFF H,*hp = &H; int32_t newlen_offset = 8;
    _init_HUFF(hp,complen,data);
    hp->endpos = (complen << 3);//hp->bitoffset;
    hseek(hp,newlen_offset,SEEK_SET);
    hemit_bits(hp,complen,16);
    if ( Debuglevel > 0 )
    {
        int32_t i;
        printf("\nDECODE hp.(end %d pos %d [%d %d] %p): newlen.%d\n",hp->endpos,hp->bitoffset,hp->ptr[0],hp->buf[0],hp->ptr,complen);
        for (i=0; i<complen; i++)
            printf("%02x ",hp->buf[i]);
        printf("hp->buf\n");
    }
    hrewind(hp);
    hseek(hp,8*(1 + sizeof(uint16_t)),SEEK_CUR);
    _huffexpand(destbuf,hp,0,0);
    return(0);
}

int32_t huffencode(int32_t dispflag,HUFF *hp,struct huffstats *hstats,struct huffcode *huff,struct huffitem *items,int32_t numitems,uint8_t *src,int32_t len)
{
    uint32_t ind,*marked = hstats->marked;
    int32_t i,j,patlen,offset,newlen,newlen_offset;
    struct huffitem *item;
    hclear(hp,0);
    newlen_offset = hemit_bits(hp,'H',8);
    hseek(hp,16,SEEK_CUR);
    hemit_header(0,dispflag,hp,huff,items,numitems);
    for (i=0; i<len; i++)
    {
        if ( marked[i] == 0 )
            printf("unexpected unmarked at %d of %d\n",i,len);
        else
        {
            if ( (j= ((marked[i] >> 1) & 0x7f)) == 0 )
            {
                patlen = (marked[i] >> 8) & 0xff;
                ind = (marked[i] >> 16) & 0xffff;
                if ( hstats->freq[patlen][ind] != 0 )
                {
                    ind = hstats->freq[patlen][ind] - 1;
                    if ( Debuglevel > 0 )
                        printf("{%d.%d %d} ",ind,patlen,ind);
                    item = &items[ind];
                    offset = huff_encode(huff,hp,items,&ind,1);
                    //printf("(%d).%d ",ptrs[patlen][ind][0],hp->bitoffset);
                    //printf("%s ",huff_str(item->code.bits,item->code.numbits));
                }
            }
        }
    }
    newlen = hconv_bitlen(hp->bitoffset);
    if ( Debuglevel > 0 )
        printf("<<<<<<<<<<<<<<< huffencode len.%d bitoffset.%d newlen.%d [0] %d (%c)\n",len,hp->bitoffset,newlen,hp->buf[0],hp->buf[0]);
    if ( huff->depth > 0 )
    {
        hp->endpos = hp->bitoffset;
        hseek(hp,newlen_offset,SEEK_SET);
        hemit_bits(hp,newlen,16);
        if ( Debuglevel > 0 )
        {
            printf("\nDECODE hp.(end %d pos %d [%d %d] %p): newlen.%d\n",hp->endpos,hp->bitoffset,hp->ptr[0],hp->buf[0],hp->ptr,newlen);
            for (i=0; i<newlen; i++)
                printf("%02x ",hp->buf[i]);
            printf("hp->buf\n");
        }
        hrewind(hp);
        hseek(hp,8*(1 + sizeof(uint16_t)),SEEK_CUR);
        _huffexpand(hstats->checkbuf,hp,src,len);
    } else printf("huffencode invalid depth.%d?\n",huff->depth);
    return(newlen);
}

int32_t huffcompress(uint8_t *dest,int32_t maxlen,uint8_t *src,int32_t len,int32_t maxpatlen)
{
    int32_t i,numitems,nonz,tablesize,newlen = 0;
    uint16_t slen;
    struct huffstats *hstats = 0;
    HUFF *hp = 0;
    struct huffcode *huff = 0;
    struct huffitem *items = 0;
    hstats = calloc(1,sizeof(*hstats));
    if ( maxpatlen < 1 )
        maxpatlen = 1;
    if ( hstats == 0 || (newlen= len) > sizeof(hstats->marked) )
    {
        printf("huffcompress: overflow len.%d > %ld\n",len,sizeof(hstats->marked));
        return(-1);
    }
    memset(dest,0,maxlen);
    tablesize = nonz = 0;
    huff_findpatterns(hstats,maxpatlen,src,len);
    numitems = huff_setmarked(hstats,maxpatlen,src,len);
    if ( numitems == 0 )
    {
        printf("huffcompress: no items?\n");
        free(hstats);
        return(-1);
    }
    items = calloc(numitems,sizeof(*items));
    hp = huffsetitems(dest,maxlen,hstats,items,numitems,maxpatlen);
    if ( numitems > 1 && (huff= huff_create(Debuglevel,items,numitems)) != 0 )
    {
        if ( Debuglevel > 0 )
        {
            for (i=0; i<huff->depth; i++)
                printf("%d %d, ",huff->tree[i<<1],huff->tree[(i<<1)+1]);
            printf("emit the compressed bits totalbytes %.0f -> (bits %.0f + table %d + tree %d code %d) ratio %.3f | maxpatlen.%d\n",huff->totalbytes,huff->totalbits,tablesize*8,huff->depth*16,numitems*(32 +maxpatlen*8),huff->totalbytes*8./(huff->totalbits + huff->depth*16 + numitems*(32 +maxpatlen*8)),maxpatlen);
        }
        newlen = huffencode(Debuglevel,hp,hstats,huff,items,numitems,src,len);
        //printf("emit the compressed bits totalbytes %.0f -> (bits %.0f + table %d + tree %d code %d) ratio %.3f | maxpatlen.%d\n",huff->totalbytes,huff->totalbits,tablesize*8,huff->depth*16,numitems*(32 +maxpatlen*8),huff->totalbytes*8./(huff->totalbits + huff->depth*16 + numitems*(32 +maxpatlen*8)),maxpatlen);
    } else newlen = hconv_bitlen(hp->bitoffset);
    if ( newlen >= len )
    {
        slen = len;
        dest[0] = 'P'; // passthrough
        memcpy(&dest[1],&slen,sizeof(slen));
        memcpy(&dest[1+sizeof(slen)],src,len);
        newlen = (len + 1 + sizeof(slen));
    }
    if ( huff != 0 )
        free(huff);
    if ( hp != 0 )
        free(hp);
    if ( items != 0 )
        free(items);
    free(hstats);
    return(newlen);
}

/*int32_t hwrite(uint64_t codebits,int32_t numbits,HUFF *hp)
{
    int32_t i;
    for (i=0; i<numbits; i++,codebits>>=1)
    {
        if ( hputbit(hp,codebits & 1) < 0 )
            return(-1);
    }
    return(numbits);
}*/

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
        len = hconv_bitlen((uint32_t)endbitpos);
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

/*HUFF *hload(struct ramchain_info *ram,long *offsetp,FILE *fp,char *fname)
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
}*/

#endif
#endif


