//
//  huffstream.c
//  crypto777
//
//  Created by jl777 on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//


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

#endif
#else
#ifndef crypto777_huffstream_c
#define crypto777_huffstream_c

#ifndef crypto777_huffstream_h
#define DEFINES_ONLY
#include "huffstream.c"
#undef DEFINES_ONLY
#endif

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

#endif
#endif


