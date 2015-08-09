//
//  sorts.h
//  xcode
//
//  Created by jl777 on 7/25/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_sorts_h
#define xcode_sorts_h

// theoretically optimal sorts of small arrays
// void sortnetwork_sorttype(sorttype *sortbuf,int num,int dir)

#define sorttype int8_t
#define sortnetwork sortnetwork_int8
#include "_sorts.h"
#undef sorttype
#undef sortnetwork

#define sorttype uint8_t
#define sortnetwork sortnetwork_uint8
#include "_sorts.h"
#undef sorttype
#undef sortnetwork

#define sorttype int16_t
#define sortnetwork sortnetwork_int16
#include "_sorts.h"
#undef sorttype
#undef sortnetwork

#define sorttype uint16_t
#define sortnetwork sortnetwork_uint16
#include "_sorts.h"
#undef sorttype
#undef sortnetwork

#define sorttype int32_t
#define sortnetwork sortnetwork_int32
#include "_sorts.h"
#undef sorttype
#undef sortnetwork

#define sorttype uint32_t
#define sortnetwork sortnetwork_uint32
#include "_sorts.h"
#undef sorttype
#undef sortnetwork

#define sorttype int64_t
#define sortnetwork sortnetwork_int64
#include "_sorts.h"
#undef sorttype
#undef sortnetwork

#define sorttype uint64_t
#define sortnetwork sortnetwork_uint64
#include "_sorts.h"
#undef sorttype
#undef sortnetwork

#define sorttype float
#define sortnetwork sortnetwork_float
#include "_sorts.h"
#undef sorttype
#undef sortnetwork

#define sorttype double
#define sortnetwork sortnetwork_double
#include "_sorts.h"
#undef sorttype
#undef sortnetwork

// more normal sorting stuff
/*
int _increasing_unsignedint(const void *a,const void *b)
{
#define uint_a (((unsigned int *)a)[0])
#define uint_b (((unsigned int *)b)[0])
	if ( uint_b > uint_a )
		return(-1);
	else if ( uint_b < uint_a )
		return(1);
	return(0);
#undef uint_a
#undef uint_b
}

int _increasing_float(const void *a,const void *b)
{
#define float_a (*(float *)a)
#define float_b (*(float *)b)
	if ( float_b > float_a )
		return(-1);
	else if ( float_b < float_a )
		return(1);
	return(0);
#undef float_a
#undef float_b
}

int _decreasing_float(const void *a,const void *b)
{
#define float_a (*(float *)a)
#define float_b (*(float *)b)
	if ( float_b > float_a )
		return(1);
	else if ( float_b < float_a )
		return(-1);
	return(0);
#undef float_a
#undef float_b
}

int _decreasing_unsignedint64(const void *a,const void *b)
{
#define uint_a (((uint64_t *)a)[0])
#define uint_b (((uint64_t *)b)[0])
	if ( uint_b > uint_a )
		return(1);
	else if ( uint_b < uint_a )
		return(-1);
	return(0);
#undef uint_a
#undef uint_b
}

int _decreasing_signedint64(const void *a,const void *b)
{
#define int_a (((int64_t *)a)[0])
#define int_b (((int64_t *)b)[0])
	if ( int_b > int_a )
		return(1);
	else if ( int_b < int_a )
		return(-1);
	return(0);
#undef int_a
#undef int_b
}

static int _decreasing_double(const void *a,const void *b)
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

static int _increasing_double(const void *a,const void *b)
{
#define double_a (*(double *)a)
#define double_b (*(double *)b)
	if ( double_b > double_a )
		return(-1);
	else if ( double_b < double_a )
		return(1);
	return(0);
#undef double_a
#undef double_b
}

int _increasing_uint64(const void *a,const void *b)
{
#define uint64_a (*(uint64_t *)a)
#define uint64_b (*(uint64_t *)b)
	if ( uint64_b > uint64_a )
		return(-1);
	else if ( uint64_b < uint64_a )
		return(1);
	return(0);
#undef uint64_a
#undef uint64_b
}

static int _decreasing_uint64(const void *a,const void *b)
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

static int _cmp_strings(const void *a,const void *b)
{
#define str_a ((char *)a)
#define str_b ((char *)b)
    return(strcmp(str_a,str_b));
#undef double_a
#undef double_b
}

int32_t revsortstrs(char *buf,uint32_t num,int32_t size)
{
	qsort(buf,num,size,_cmp_strings);
	return(0);
}

int32_t revsortfs(float *buf,uint32_t num,int32_t size)
{
	qsort(buf,num,size,_decreasing_float);
	return(0);
}

int32_t revsortds(double *buf,uint32_t num,int32_t size)
{
	qsort(buf,num,size,_decreasing_double);
	return(0);
}

int32_t sortds(double *buf,uint32_t num,int32_t size)
{
	qsort(buf,num,size,_increasing_double);
	return(0);
}

int32_t sort64s(uint64_t *buf,uint32_t num,int32_t size)
{
	qsort(buf,num,size,_increasing_uint64);
	return(0);
}

int32_t revsort64s(uint64_t *buf,uint32_t num,int32_t size)
{
	qsort(buf,num,size,_decreasing_uint64);
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
    // printf("{%d} ",val);
    return(numbits);
}

int32_t emit_smallbits(HUFF *hp,uint16_t val)
{
    static long sum,count;
    int32_t numbits = 0;
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
    int32_t val,numbits = 0;
    val = (hgetbit(hp) | (hgetbit(hp) << 1) | (hgetbit(hp) << 2));
    if ( val < 4 )
    {
        *valp = val;
        return(3);
    }
    else if ( val == 4 )
        numbits += decode_bits(&s,hp,5);
    else if ( val == 5 )
        numbits += decode_bits(&s,hp,8);
    else if ( val == 6 )
        numbits += decode_bits(&s,hp,12);
    else numbits += decode_bits(&s,hp,16);
    *valp = s;
    return(numbits + 3);
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
        printf("ave varbits %.1f after %ld samples\n",(double)sum/count,count);
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
*/
#endif
