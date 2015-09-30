/******************************************************************************
 * Copyright © 2014-2015 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

//  ramcoder based on arcode.c from Michael Dipperstein

#ifdef DEFINES_ONLY
#ifndef crypto777_ramcoder_h
#define crypto777_ramcoder_h
#include <stdint.h>
#include "bits777.c"
#include "huffstream.c"

#define RAMMASK_BIT(x) ((uint16_t)(1 << ((8 * sizeof(uint16_t)) - (1 + (x)))))
#define RAMCODER_FINALIZE 1
#define RAMCODER_PUTBITS 2
#define RAMCODER_MAXSYMBOLS 0x100

struct ramcoder
{
    uint32_t cumulativeProb;
    uint16_t lower,upper,code,underflowBits,lastsymbol,upper_lastsymbol,counter;
    uint16_t ranges[];
};
int32_t ramcoder_decode(struct ramcoder *coder,int32_t updateprobs,HUFF *hp);
int32_t ramcoder_decoder(struct ramcoder *coder,int32_t updateprobs,uint8_t *buf,int32_t maxlen,HUFF *hp,bits256 *seed);
#define ramcoder_encode(val,coder,hp) ramcoder_update(val,coder,1,RAMCODER_PUTBITS,hp)
bits256 ramcoder_encoder(struct ramcoder *coder,int32_t updateprobs,uint8_t *buf,int32_t len,HUFF *hp,bits256 *seed);
int32_t ramcoder_update(int symbol,struct ramcoder *coder,int32_t updateprobs,int32_t putflags,HUFF *hp);
int32_t init_ramcoder(struct ramcoder *coder,HUFF *hp,bits256 *seed);
int32_t ramcoder_emit(HUFF *hp,struct ramcoder *coder,int32_t updateprobs,uint8_t *buf,int32_t len);

#endif
#else
#ifndef crypto777_ramcoder_c
#define crypto777_ramcoder_c

#ifndef crypto777_ramcoder_h
#define DEFINES_ONLY
#include "ramcoder.c"
#undef DEFINES_ONLY
#endif

int32_t init_ramcoder(struct ramcoder *coder,HUFF *hp,bits256 *seed)
{
    int32_t i,precision,numbits = 0;
    if ( coder->lastsymbol == 0 )
        coder->lastsymbol = RAMCODER_MAXSYMBOLS, coder->upper_lastsymbol = (coder->lastsymbol + 1);
    coder->cumulativeProb = coder->lower = coder->code = coder->underflowBits = coder->ranges[0] = 0;
    for (i=1; i<=coder->upper_lastsymbol; i++)
    {
        coder->ranges[i] = coder->ranges[i - 1] + 1 + 256*((i <= sizeof(seed)*8) ? (GETBIT(seed->bytes,i-1) != 0) : 0);
        //printf("%d ",coder->ranges[i]);
    }
    for (i=1; i<=coder->upper_lastsymbol; i++)
        coder->cumulativeProb += (coder->ranges[i] - coder->ranges[i - 1]);
    //printf("cumulative.%d\n",coder->cumulativeProb);
    precision = (8 * sizeof(uint16_t));
    coder->upper = (1LL << precision) - 1;
    if ( hp != 0 )
    {
        for (i=0; i<precision; i++)
            coder->code = (coder->code << 1) | hgetbit(hp);
        //coder->code = hread(&numbits,precision,hp), coder->code <<= (precision - numbits);
        //printf("set code %x\n",coder->code);
    }
    return(numbits);
}

int32_t ramcoder_state(struct ramcoder *coder)
{
    if ( (coder->upper & RAMMASK_BIT(0)) == (coder->lower & RAMMASK_BIT(0)) )
        return(0);
    else if ( (coder->lower & RAMMASK_BIT(1)) && (coder->upper & RAMMASK_BIT(1)) == 0 )
        return(1);
    else return(-1);
}

void ramcoder_normalize(struct ramcoder *coder) { coder->lower &= ~(RAMMASK_BIT(0) | RAMMASK_BIT(1)), coder->upper |= RAMMASK_BIT(1); }

void ramcoder_shiftbits(struct ramcoder *coder) { coder->lower <<= 1, coder->upper <<= 1, coder->upper |= 1; }

int32_t ramcoder_putbits(HUFF *hp,struct ramcoder *coder,int32_t flushflag)
{
    int32_t numbits = 0;
    while ( 1 )
    {
        switch ( ramcoder_state(coder) )
        {
            case 1:  coder->underflowBits++, ramcoder_normalize(coder); break;
            case 0:
                hputbit(hp,(coder->upper & RAMMASK_BIT(0)) != 0), numbits++;
                //printf("%d> ",(coder->upper & RAMMASK_BIT(0)) != 0);
                while ( coder->underflowBits > 0 )
                {
                    hputbit(hp,(coder->upper & RAMMASK_BIT(0)) == 0), numbits++;
                    //printf("%d> ",(coder->upper & RAMMASK_BIT(0)) == 0);
                    coder->underflowBits--;
                }
                break;
            default:
                if ( flushflag != 0 )
                {
                    hputbit(hp,(coder->lower & RAMMASK_BIT(1)) != 0), numbits++;
                    for (coder->underflowBits++; coder->underflowBits>0; coder->underflowBits--)
                        hputbit(hp,(coder->lower & RAMMASK_BIT(1)) == 0), numbits++;
                }
                return(numbits);
                break;
        }
        ramcoder_shiftbits(coder);
    }
}

int32_t ramcoder_getbits(HUFF *hp,struct ramcoder *coder)
{
    int32_t nextBit,numbits = 0;
    while ( 1 )
    {
        switch ( ramcoder_state(coder) )
        {
            case 0: break; // MSBs match, allow them to be shifted out
            case 1: ramcoder_normalize(coder), coder->code ^= RAMMASK_BIT(1); break;
            default:  return(numbits); break;
        }
        ramcoder_shiftbits(coder);
        coder->code <<= 1;
        if ( (nextBit= hgetbit(hp)) >= 0 )
            coder->code |= nextBit; //, printf("<%c",'0'+nextBit);
        else return(numbits);
        numbits++;
    }
}

int32_t ramdecoder_bsearch(uint16_t probability,struct ramcoder *coder)
{
    int32_t last,middle,first = 0;
    last = coder->upper_lastsymbol;
    while ( last >= first )
    {
        middle = first + ((last - first) / 2);
        //printf("[%d %d] ",coder->ranges[middle],coder->ranges[middle+1]);
        if ( probability < coder->ranges[middle] )
            last = middle - 1;
        else if ( probability >= coder->ranges[middle + 1] )
            first = middle + 1;
        else return(middle);
    }
    printf("Unknown Symbol: %llu (max: %llu)\n",(long long)probability,(long long)coder->ranges[coder->upper_lastsymbol]);
    return(-1);
}

int32_t ramcoder_update(int symbol,struct ramcoder *coder,int32_t updateprobs,int32_t putflags,HUFF *hp)
{
    uint32_t range; uint16_t i,original,delta;
//printf("putflags.%d %p: upper %llu lower %llu code.%x cumulative.%d | symbol.%d\n",putflags,coder,(long long)coder->upper,(long long)coder->lower,coder->code,coder->cumulativeProb,symbol);
    range = (uint32_t)(coder->upper - coder->lower) + 1;
    coder->upper = coder->lower + (uint16_t)(((uint32_t)coder->ranges[symbol + 1] * range)/ coder->cumulativeProb) - 1;
    coder->lower = coder->lower + (uint16_t)(((uint32_t)coder->ranges[symbol] * range) / coder->cumulativeProb);
    if ( updateprobs != 0 )
    {
        coder->cumulativeProb++;
        for (i=(symbol+1); i<=coder->upper_lastsymbol; i++)
            coder->ranges[i]++;
        if ( coder->cumulativeProb >= (1 << ((8 * sizeof(uint16_t)) - 2)) )
        {
            original = coder->cumulativeProb = 0;
            for (i=1; i<=coder->upper_lastsymbol; i++)
            {
                delta = coder->ranges[i] - original, original = coder->ranges[i];
                if ( delta <= 2 )
                    coder->ranges[i] = coder->ranges[i - 1] + 1;
                else coder->ranges[i] = coder->ranges[i - 1] + (delta / 2);
                coder->cumulativeProb += (coder->ranges[i] - coder->ranges[i - 1]);
            }
        }
        coder->counter++;
    } else printf("unexpected non-update ramcoder\n");
    if ( coder->lower > coder->upper )
        printf("ramcoderupdate: coder->lower %llu > %llu coder->upper\n",(long long)coder->lower,(long long)coder->upper);
    return((putflags != 0) ? ramcoder_putbits(hp,coder,putflags & RAMCODER_FINALIZE) : ramcoder_getbits(hp,coder));
}

int32_t ramcoder_emit(HUFF *hp,struct ramcoder *coder,int32_t updateprobs,uint8_t *buf,int32_t len)
{
    int32_t i,numbits = 0;
    for (i=0; i<len; i++)
    {
        numbits += ramcoder_update(buf[i],coder,updateprobs,RAMCODER_PUTBITS,hp);//, printf("->%02x ",buf[i]);
        //numbits += ramcoder_update((buf[i]>>4)&0xf,coder,updateprobs,RAMCODER_PUTBITS,hp);//, printf("->%02x ",buf[i]);
    }
    return(numbits);
}

bits256 ramcoder_encoder(struct ramcoder *coder,int32_t updateprobs,uint8_t *buf,int32_t len,HUFF *hp,bits256 *seed)
{
    bits256 newseed; int32_t i,threshold; uint8_t _coder[sizeof(*coder) + (RAMCODER_MAXSYMBOLS+2)*sizeof(coder->ranges[0])];
    if ( coder == 0 )
    {
        memset(_coder,0,sizeof(_coder));
        hrewind(hp);
        coder = (struct ramcoder *)_coder, init_ramcoder(coder,0,seed);
        ramcoder_emit(hp,coder,updateprobs,buf,len);
        ramcoder_update(coder->lastsymbol,coder,updateprobs,RAMCODER_PUTBITS,hp);
        ramcoder_update(coder->lastsymbol,coder,updateprobs,RAMCODER_PUTBITS|RAMCODER_FINALIZE,hp);
    }
    else ramcoder_emit(hp,coder,updateprobs,buf,len);
    memset(newseed.bytes,0,sizeof(newseed));
    threshold = coder->cumulativeProb / coder->upper_lastsymbol;
    for (i=1; i<=coder->upper_lastsymbol; i++)
        if ( (coder->ranges[i] - coder->ranges[i - 1]) > threshold )
            SETBIT(newseed.bytes,i-1);
    return(newseed);
}

int32_t ramcoder_decode(struct ramcoder *coder,int32_t updateprobs,HUFF *hp)
{
    int32_t ind;
#define RAMDECODER_UNSCALED(coder) ((((uint32_t)coder->code - coder->lower) + 1) * (uint32_t)coder->cumulativeProb - 1) / (((uint32_t)coder->upper - coder->lower) + 1)
    if ( (ind= ramdecoder_bsearch(RAMDECODER_UNSCALED(coder),coder)) < 0 || ind == coder->lastsymbol )
        return(-1);
    //fprintf(stderr,"output.%02x ",ind);
    ramcoder_update(ind,coder,updateprobs,0,hp);
    return(ind);
}

int32_t ramcoder_decoder(struct ramcoder *coder,int32_t updateprobs,uint8_t *buf,int32_t maxlen,HUFF *hp,bits256 *seed)
{
    uint8_t _coder[sizeof(*coder) + (RAMCODER_MAXSYMBOLS+2)*sizeof(coder->ranges[0])];
    int32_t val,n = 0,numbits = 0;
    if ( coder == 0 )
        memset(_coder,0,sizeof(_coder)), coder = (struct ramcoder *)_coder, hrewind(hp), numbits = init_ramcoder(coder,hp,seed);
    while ( n < maxlen )
    {
        if ( (val= ramcoder_decode(coder,updateprobs,hp)) < 0 )
            break;
        buf[n] = val;
        //if ( (val= ramcoder_decode(coder,updateprobs,hp)) < 0 )
        //    break;
        //buf[n] |= (val << 4);
        n++;
    }
    return(n);
}

#endif
#endif
