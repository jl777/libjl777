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

#endif
