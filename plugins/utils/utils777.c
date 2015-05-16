//
//  util777.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_util777_h
#define crypto777_util777_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "bits777.c"
#include "system777.c"

#define SATOSHIDEN 100000000L
#define dstr(x) ((double)(x) / SATOSHIDEN)

struct alloc_space { void *ptr; long used,size; int32_t alignflag; };

int32_t portable_pton(int32_t af,char *src,void *dst);
int32_t portable_ntop(int32_t af,void *src,char *dst,size_t size);

void _randombytes(uint8_t *buf,int32_t n,uint32_t seed);

char *stringifyM(char *str);
#define replace_backslashquotes unstringify
char *unstringify(char *str);
int32_t safecopy(char *dest,char *src,long len);
int64_t conv_floatstr(char *numstr);

void touppercase(char *str);
void reverse_hexstr(char *str);
char hexbyte(int32_t c);
int32_t is_zeroes(char *str);
int32_t is_hexstr(char *str);
unsigned char _decode_hex(char *hex);
int32_t decode_hex(unsigned char *bytes,int32_t n,char *hex);
int32_t init_hexbytes_noT(char *hexbytes,unsigned char *message,long len);
long _stripwhite(char *buf,int accept);
char *clonestr(char *str);
uint8_t *conv_datastr(int32_t *datalenp,uint8_t *data,char *datastr);
int32_t is_decimalstr(char *str);
long stripstr(char *buf,long len);
char safechar64(int32_t x);
uint64_t stringbits(char *str);

char *_mbstr(double n);
char *_mbstr2(double n);
int32_t revsortstrs(char *buf,uint32_t num,int32_t size);
int32_t revsortfs(float *buf,uint32_t num,int32_t size);
int32_t revsortds(double *buf,uint32_t num,int32_t size);
int32_t sortds(double *buf,uint32_t num,int32_t size);
int32_t sort64s(uint64_t *buf,uint32_t num,int32_t size);
int32_t revsort64s(uint64_t *buf,uint32_t num,int32_t size);

double estimate_completion(double startmilli,int32_t processed,int32_t numleft);

void clear_alloc_space(struct alloc_space *mem,int32_t alignflag);
void *memalloc(struct alloc_space *mem,long size,int32_t clearflag);

int32_t notlocalip(char *ipaddr);
int32_t is_remote_access(char *previpaddr);

#endif
#else
#ifndef crypto777_util777_c
#define crypto777_util777_c

#ifndef crypto777_util777_h
#define DEFINES_ONLY
#include "utils777.c"
#undef DEFINES_ONLY
#endif

void _randombytes(uint8_t *buf,int32_t n,uint32_t seed) { int32_t i; if ( seed != 0 ) srand(seed); for (i=0; i<n; i++) buf[i] = (rand() >> 8); } // low entropy pseudo-random bytes

char *stringifyM(char *str)
{
    char *newstr;
    int32_t i,j,n;
    if ( str == 0 )
        return(0);
    else if ( str[0] == 0 )
        return(str);
    for (i=n=0; str[i]!=0; i++)
        n += (str[i] == '"') ? 2 : 1;
    newstr = (char *)malloc(n + 3);
    j = 0;
    newstr[j++] = '"';
    for (i=0; str[i]!=0; i++)
    {
        if ( str[i] == '"' )
        {
            newstr[j++] = '\\';
            newstr[j++] = '"';
        }
        else newstr[j++] = str[i];
    }
    newstr[j++] = '"';
    newstr[j] = 0;
    return(newstr);
}

#define replace_backslashquotes unstringify
char *unstringify(char *str)
{
    int32_t i,j,n;
    if ( str == 0 )
        return(0);
    else if ( str[0] == 0 )
        return(str);
    n = (int32_t)strlen(str);
    if ( str[0] == '"' && str[n-1] == '"' )
        str[n-1] = 0, i = 1;
    else i = 0;
    for (j=0; str[i]!=0; i++)
    {
        if ( str[i] == '\\' && (str[i+1] == 't' || str[i+1] == 'n' || str[i+1] == 'b' || str[i+1] == 'r') )
            i++;
        else if ( str[i] == '\\' && str[i+1] == '"' )
            str[j++] = '"', i++;
        else str[j++] = str[i];
    }
    str[j] = 0;
    return(str);
}

int32_t safecopy(char *dest,char *src,long len)
{
    int32_t i = -1;
    if ( dest != 0 )
        memset(dest,0,len);
    if ( src != 0 )
    {
        for (i=0; i<len&&src[i]!=0; i++)
            dest[i] = src[i];
        if ( i == len )
        {
            printf("safecopy: %s too long %ld\n",src,len);
#ifdef __APPLE__
            getchar();
#endif
            return(-1);
        }
        dest[i] = 0;
    }
    return(i);
}

int32_t is_zeroes(char *str)
{
    int32_t i;
    if ( str == 0 || str[0] == 0 )
        return(1);
    for (i=0; str[i]!=0; i++)
        if ( str[i] != '0' )
            return(0);
    return(1);
}

int64_t conv_floatstr(char *numstr)
{
    double val,corr;
    val = atof(numstr);
    corr = (val < 0.) ? -0.50000000001 : 0.50000000001;
    return((int64_t)(val * SATOSHIDEN + corr));
}

void touppercase(char *str)
{
    int32_t i;
    if ( str == 0 || str[0] == 0 )
        return;
    for (i=0; str[i]!=0; i++)
        str[i] = toupper(str[i]);
}

void reverse_hexstr(char *str)
{
    int i,n;
    char *rev;
    n = (int32_t)strlen(str);
    rev = (char *)malloc(n + 1);
    for (i=0; i<n; i+=2)
    {
        rev[n-2-i] = str[i];
        rev[n-1-i] = str[i+1];
    }
    rev[n] = 0;
    strcpy(str,rev);
    free(rev);
}

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

unsigned char _decode_hex(char *hex) { return((unhex(hex[0])<<4) | unhex(hex[1])); }

int32_t decode_hex(unsigned char *bytes,int32_t n,char *hex)
{
    int32_t adjust,i = 0;
    if ( n == 0 || (hex[n*2+1] == 0 && hex[n*2] != 0) )
    {
        bytes[0] = unhex(hex[0]);
        printf("decode_hex n.%d hex[0] (%c) -> %d\n",n,hex[0],bytes[0]);
        //while ( 1 ) portable_sleep(1);
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

long _stripwhite(char *buf,int accept)
{
    int32_t i,j,c;
    if ( buf == 0 || buf[0] == 0 )
        return(0);
    for (i=j=0; buf[i]!=0; i++)
    {
        buf[j] = c = buf[i];
        if ( c == accept || (c != ' ' && c != '\n' && c != '\r' && c != '\t' && c != '\b') )
            j++;
    }
    buf[j] = 0;
    return(j);
}

char *clonestr(char *str)
{
    char *clone;
    if ( str == 0 || str[0] == 0 )
    {
        printf("warning cloning nullstr.%p\n",str);
#ifdef __APPLE__
        while ( 1 ) portable_sleep(1);
#endif
        str = (char *)"<nullstr>";
    }
    clone = (char *)malloc(strlen(str)+16);
    strcpy(clone,str);
    return(clone);
}

long stripstr(char *buf,long len)
{
    int32_t i,j,c;
    for (i=j=0; i<len; i++)
    {
        c = buf[i];
        //if ( c == '\\' )
        //    c = buf[i+1], i++;
        buf[j] = c;
        if ( buf[j] != ' ' && buf[j] != '\n' && buf[j] != '\r' && buf[j] != '\t' && buf[j] != '"' )
            j++;
    }
    buf[j] = 0;
    return(j);
}

char safechar64(int32_t x)
{
    x %= 64;
    if ( x < 26 )
        return(x + 'a');
    else if ( x < 52 )
        return(x + 'A' - 26);
    else if ( x < 62 )
        return(x + '0' - 52);
    else if ( x == 62 )
        return('_');
    else
        return('-');
}

uint8_t *conv_datastr(int32_t *datalenp,uint8_t *data,char *datastr)
{
    int32_t datalen;
    uint8_t *dataptr;
    dataptr = 0;
    datalen = 0;
    if ( datastr[0] != 0 && is_hexstr(datastr) )
    {
        datalen = (int32_t)strlen(datastr);
        if ( datalen > 1 && (datalen & 1) == 0 )
        {
            datalen >>= 1;
            dataptr = data;
            decode_hex(data,datalen,datastr);
        } else datalen = 0;
    }
    *datalenp = datalen;
    return(dataptr);
}

int32_t is_decimalstr(char *str)
{
    int32_t i;
    if ( str == 0 || str[0] == 0 )
        return(0);
    for (i=0; str[i]!=0; i++)
        if ( str[i] < '0' || str[i] > '9' )
            return(0);
    return(i);
}

uint64_t stringbits(char *str)
{
    uint64_t bits = 0;
    int32_t i,n = (int32_t)strlen(str);
    if ( n > 8 )
        n = 8;
    for (i=n-1; i>=0; i--)
        bits = (bits << 8) | (str[i] & 0xff);
    //printf("(%s) -> %llx %llu\n",str,(long long)bits,(long long)bits);
    return(bits);
}

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

double estimate_completion(double startmilli,int32_t processed,int32_t numleft)
{
    double elapsed,rate;
    if ( processed <= 0 )
        return(0.);
    elapsed = (milliseconds() - startmilli);
    rate = (elapsed / processed);
    if ( rate <= 0. )
        return(0.);
    //printf("numleft %d rate %f\n",numleft,rate);
    return(numleft * rate);
}

void clear_alloc_space(struct alloc_space *mem,int32_t alignflag)
{
    memset(mem->ptr,0,mem->size);
    mem->used = 0;
    mem->alignflag = alignflag;
}

void *memalloc(struct alloc_space *mem,long size,int32_t clearflag)
{
    void *ptr = 0;
    if ( (mem->used + size) > mem->size )
    {
        printf("alloc: (mem->used %ld + %ld size) %ld > %ld mem->size\n",mem->used,size,(mem->used + size),mem->size);
        while ( 1 )
            portable_sleep(1);
    }
    ptr = (void *)((long)mem->ptr + mem->used);
    mem->used += size;
    if ( clearflag != 0 )
        memset(ptr,0,size);
    if ( mem->alignflag != 0 && (mem->used & 0xf) != 0 )
        mem->used += 0x10 - (mem->used & 0xf);
    return(ptr);
}


#endif
#endif
