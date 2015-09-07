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
#ifndef crypto777_util777_h
#define crypto777_util777_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "bits777.c"
#include "../common/system777.c"

struct alloc_space { void *ptr; long used,size; int32_t alignflag; uint8_t space[4]; };

int32_t portable_pton(int32_t af,char *src,void *dst);
int32_t portable_ntop(int32_t af,void *src,char *dst,size_t size);

void _randombytes(uint8_t *buf,int32_t n,uint32_t seed);

char *stringifyM(char *str);
#define replace_backslashquotes unstringify
char *unstringify(char *str);
int64_t conv_floatstr(char *numstr);

void touppercase(char *str);
void reverse_hexstr(char *str);
char hexbyte(int32_t c);
int32_t is_zeroes(char *str);
int32_t is_hexstr(char *str);
unsigned char _decode_hex(char *hex);

long stripstr(char *buf,long len);
char safechar64(int32_t x);
int32_t has_backslash(char *str);
void escape_code(char *escaped,char *str);
uint8_t *conv_datastr(int32_t *datalenp,uint8_t *data,char *datastr);

char *_mbstr(double n);
char *_mbstr2(double n);
int32_t revsortstrs(char *buf,uint32_t num,int32_t size);
int32_t revsortfs(float *buf,uint32_t num,int32_t size);
int32_t revsortds(double *buf,uint32_t num,int32_t size);
int32_t sortds(double *buf,uint32_t num,int32_t size);
int32_t sort64s(uint64_t *buf,uint32_t num,int32_t size);
int32_t revsort64s(uint64_t *buf,uint32_t num,int32_t size);
int32_t sort32s(uint32_t *buf,uint32_t num,int32_t size);

double estimate_completion(double startmilli,int32_t processed,int32_t numleft);

void clear_alloc_space(struct alloc_space *mem,int32_t alignflag);
void *memalloc(struct alloc_space *mem,long size,int32_t clearflag);
void rewind_alloc_space(struct alloc_space *mem,int32_t flags);
struct alloc_space *init_alloc_space(struct alloc_space *mem,void *ptr,long size,int32_t flags);

int32_t notlocalip(char *ipaddr);
int32_t is_remote_access(char *previpaddr);

float xblend(float *destp,float val,float decay);
double dxblend(double *destp,double val,double decay);
void tolowercase(char *str);

double _pairaved(double valA,double valB);
double _pairave(float valA,float valB);

int32_t conv_base32(int32_t val5);
int expand_base32(uint8_t *tokenstr,uint8_t *token,int32_t len);

int32_t find_uint64(int32_t *emptyslotp,uint64_t *nums,long max,uint64_t val);
int32_t add_uint64(uint64_t *nums,long max,uint64_t val);
int32_t remove_uint64(uint64_t *nums,long max,uint64_t val);

int32_t find_uint32(int32_t *emptyslotp,uint32_t *nums,long max,uint32_t val);
int32_t add_uint32(uint32_t *nums,long max,uint32_t val);
int32_t remove_uint32(uint32_t *nums,long max,uint32_t val);

int32_t find_uint16(int32_t *emptyslotp,uint16_t *nums,long max,uint16_t val);
int32_t add_uint16(uint16_t *nums,long max,uint16_t val);
int32_t remove_uint16(uint16_t *nums,long max,uint16_t val);


long hdecode_varint(uint64_t *valp,uint8_t *ptr,long offset,long mappedsize);
int32_t hcalc_varint(uint8_t *buf,uint64_t x);
long _emit_uint(uint8_t *data,long offset,uint64_t x,int32_t size);
long _decode_uint(void *destp,int32_t size,uint8_t *data,long offset);

int32_t is_DST(int32_t datenum);
int32_t extract_datenum(int32_t *yearp,int32_t *monthp,int32_t *dayp,int32_t datenum);
int32_t expand_datenum(char *date,int32_t datenum);
int32_t calc_datenum(int32_t year,int32_t month,int32_t day);
int32_t conv_date(int32_t *secondsp,char *date);
int32_t ecb_decrdate(int32_t *yearp,int32_t *monthp,int32_t *dayp,char *date,int32_t datenum);
extern int32_t smallprimes[168];

#endif
#else
#ifndef crypto777_util777_c
#define crypto777_util777_c

#ifndef crypto777_util777_h
#define DEFINES_ONLY
#include "utils777.c"
#undef DEFINES_ONLY
#endif

int32_t smallprimes[168] =
{
	2,      3,      5,      7,     11,     13,     17,     19,     23,     29,
	31,     37,     41,     43,     47,     53,     59,     61,     67,     71,
	73,     79,     83,     89,     97,    101,    103,    107,    109,    113,
	127,    131,    137,    139,    149,    151,    157,    163,    167,    173,
	179,    181,    191,    193,    197,    199,    211,    223,    227,    229,
	233,    239,    241,    251,    257,    263,    269,    271,    277,    281,
	283,    293,    307,    311,    313,    317,    331,    337,    347,    349,
	353,    359,    367,    373,    379,    383,    389,    397,    401,    409,
	419,    421,    431,    433,    439,    443,    449,    457,    461,    463,
	467,    479,    487,    491,    499,    503,    509,    521,    523,    541,
	547,    557,    563,    569,    571,    577,    587,    593,    599,    601,
	607,    613,    617,    619,    631,    641,    643,    647,    653,    659,
	661,    673,    677,    683,    691,    701,    709,    719,    727,    733,
	739,    743,    751,    757,    761,    769,    773,    787,    797,    809,
	811,    821,    823,    827,    829,    839,    853,    857,    859,    863,
	877,    881,    883,    887,    907,    911,    919,    929,    937,    941,
	947,    953,    967,    971,    977,    983,    991,    997
};

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
            //getchar();
#endif
            return(-1);
        }
        dest[i] = 0;
    }
    return(i);
}

void escape_code(char *escaped,char *str)
{
    int32_t i,j,c; char esc[16];
    for (i=j=0; str[i]!=0; i++)
    {
        if ( ((c= str[i]) >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') )
            escaped[j++] = c;
        else
        {
            sprintf(esc,"%%%02X",c);
            //sprintf(esc,"\\\\%c",c);
            strcpy(escaped + j,esc);
            j += strlen(esc);
        }
    }
    escaped[j] = 0;
    //printf("escape_code: (%s) -> (%s)\n",str,escaped);
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

int32_t has_backslash(char *str)
{
    int32_t i;
    if ( str == 0 || str[0] == 0 )
        return(0);
    for (i=0; str[i]!=0; i++)
        if ( str[i] == '\\' )
            return(1);
    return(0);
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
    if ( is_hexstr(hex) == 0 )
    {
        memset(bytes,0,n);
        return(n);
    }
    if ( n == 0 || (hex[n*2+1] == 0 && hex[n*2] != 0) )
    {
        bytes[0] = unhex(hex[0]);
        printf("decode_hex n.%d hex[0] (%c) -> %d (%s)\n",n,hex[0],bytes[0],hex);
#ifdef __APPLE__
        getchar();
#endif
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

int32_t base32byte(int32_t val)
{
    if ( val < 26 )
        return('A' + val);
    else if ( val < 32 )
        return('2' + val - 26);
    else return(-1);
}

int32_t unbase32(char c)
{
    if ( c >= 'A' && c <= 'Z' )
        return(c - 'A');
    else if ( c >= '2' && c <= '7' )
        return(c - '2' + 26);
    else return(-1);
}

int init_base32(char *tokenstr,uint8_t *token,int32_t len)
{
    int32_t i,j,n,val5,offset = 0;
    for (i=n=0; i<len; i++)
    {
        for (j=val5=0; j<5; j++,offset++)
            if ( GETBIT(token,offset) != 0 )
                SETBIT(&val5,offset);
        tokenstr[n++] = base32byte(val5);
    }
    tokenstr[n] = 0;
    return(n);
}

int decode_base32(uint8_t *token,uint8_t *tokenstr,int32_t len)
{
    int32_t i,j,n,val5,offset = 0;
    for (i=n=0; i<len; i++)
    {
        if ( (val5= unbase32(tokenstr[i])) >= 0 )
        {
            for (j=val5=0; j<5; j++,offset++)
            {
                if ( GETBIT(&val5,j) != 0 )
                    SETBIT(token,offset);
                else CLEARBIT(token,offset);
            }
        } else return(-1);
    }
    while ( (offset & 7) != 0 )
    {
        CLEARBIT(token,offset);
        offset++;
    }
    return(offset);
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

void tolowercase(char *str)
{
    int32_t i;
    if ( str == 0 || str[0] == 0 )
        return;
    for (i=0; str[i]!=0; i++)
        str[i] = tolower(str[i]);
}

uint64_t stringbits(char *str)
{
    uint64_t bits = 0;
    if ( str == 0 )
        return(0);
    int32_t i,n = (int32_t)strlen(str);
    if ( n > 8 )
        n = 8;
    for (i=n-1; i>=0; i--)
        bits = (bits << 8) | (str[i] & 0xff);
    //printf("(%s) -> %llx %llu\n",str,(long long)bits,(long long)bits);
    return(bits);
}

int32_t find_uint32(int32_t *emptyslotp,uint32_t *nums,long max,uint32_t val)
{
    int32_t i;
    *emptyslotp = -1;
    for (i=0; i<max; i++)
    {
        if ( nums[i] == 0 )
        {
            *emptyslotp = i;
            break;
        } else if ( nums[i] == val )
            return(i);
    }
    return(-1);
}

int32_t add_uint32(uint32_t *nums,long max,uint32_t val)
{
    int32_t i,emptyslot;
    if ( (i= find_uint32(&emptyslot,nums,max,val)) >= 0 )
        return(i);
    else if ( emptyslot >= 0 )
    {
        nums[emptyslot] = val;
        return(emptyslot);
    } else return(-1);
}

int32_t remove_uint32(uint32_t *nums,long max,uint32_t val)
{
    int32_t i,emptyslot;
    if ( (i= find_uint32(&emptyslot,nums,max,val)) >= 0 )
    {
        nums[i] = 0;
        return(i);
    }
    return(-1);
}

int32_t find_uint16(int32_t *emptyslotp,uint16_t *nums,long max,uint16_t val)
{
    int32_t i;
    *emptyslotp = -1;
    for (i=0; i<max; i++)
    {
        if ( nums[i] == 0 )
        {
            *emptyslotp = i;
            break;
        } else if ( nums[i] == val )
            return(i);
    }
    return(-1);
}

int32_t add_uint16(uint16_t *nums,long max,uint16_t val)
{
    int32_t i,emptyslot;
    if ( (i= find_uint16(&emptyslot,nums,max,val)) >= 0 )
        return(i);
    else if ( emptyslot >= 0 )
    {
        nums[emptyslot] = val;
        return(emptyslot);
    } else return(-1);
}

int32_t remove_uint16(uint16_t *nums,long max,uint16_t val)
{
    int32_t i,emptyslot;
    if ( (i= find_uint16(&emptyslot,nums,max,val)) >= 0 )
    {
        nums[i] = 0;
        return(i);
    }
    return(-1);
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

int _increasing_uint32(const void *a,const void *b)
{
#define uint_a (((uint32_t *)a)[0])
#define uint_b (((uint32_t *)b)[0])
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

int32_t sort32s(uint32_t *buf,uint32_t num,int32_t size)
{
	qsort(buf,num,size,_increasing_uint32);
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

float _xblend(float *destp,float val,float decay)
{
    float oldval;
	if ( (oldval = *destp) != 0. )
		return((oldval * decay) + ((1. - decay) * val));
	else return(val);
}

double _dxblend(double *destp,double val,double decay)
{
    double oldval;
	if ( (oldval = *destp) != 0. )
		return((oldval * decay) + ((1. - decay) * val));
	else return(val);
}

float xblend(float *destp,float val,float decay)
{
	double newval,slope;
	if ( isnan(*destp) != 0 )
		*destp = 0.;
	if ( isnan(val) != 0 )
		return(0.);
	if ( *destp == 0 )
	{
		*destp = val;
		return(0);
	}
	newval = _xblend(destp,val,decay);
	if ( newval < SMALLVAL && newval > -SMALLVAL )
	{
		// non-zero marker for actual values close to or even equal to zero
		if ( newval < 0. )
			newval = -SMALLVAL;
		else newval = SMALLVAL;
	}
	if ( *destp != 0. && newval != 0. )
		slope = (newval - *destp);
	else slope = 0.;
	*destp = newval;
	return(slope);
}

double dxblend(double *destp,double val,double decay)
{
	double newval,slope;
	if ( isnan(*destp) != 0 )
		*destp = 0.;
	if ( isnan(val) != 0 )
		return(0.);
	if ( *destp == 0 )
	{
		*destp = val;
		return(0);
	}
	newval = _dxblend(destp,val,decay);
	if ( newval < SMALLVAL && newval > -SMALLVAL )
	{
		// non-zero marker for actual values close to or even equal to zero
		if ( newval < 0. )
			newval = -SMALLVAL;
		else newval = SMALLVAL;
	}
	if ( *destp != 0. && newval != 0. )
		slope = (newval - *destp);
	else slope = 0.;
	*destp = newval;
	return(slope);
}

double _pairaved(double valA,double valB)
{
	if ( valA != 0. && valB != 0. )
		return((valA + valB) / 2.);
	else if ( valA != 0. ) return(valA);
	else return(valB);
}

double _pairave(float valA,float valB)
{
	if ( valA != 0.f && valB != 0.f )
		return((valA + valB) / 2.);
	else if ( valA != 0.f ) return(valA);
	else return(valB);
}

long _emit_uint(uint8_t *data,long offset,uint64_t x,int32_t size)
{
    int32_t i;
    for (i=0; i<size; i++,x>>=8)
        data[offset + i] = (x & 0xff);
    offset += size;
    return(offset);
}

long _decode_uint(void *destp,int32_t size,uint8_t *data,long offset)
{
    int32_t i; uint64_t x = 0;
    if ( size == 1 )
        ((uint8_t *)destp)[0] = data[offset++];
    else
    {
        for (i=0; i<size; i++)
            x <<= 8, x |= data[offset + size - i];
        offset += size;
        switch ( size )
        {
            case sizeof(uint16_t): ((uint16_t *)destp)[0] = (uint16_t)x; break;
            case sizeof(uint32_t): ((uint32_t *)destp)[0] = (uint32_t)x; break;
            case sizeof(uint64_t): ((uint64_t *)destp)[0] = (uint64_t)x; break;
            default: return(-1);
        }
    }
    return(offset);
}

int32_t find_uint64(int32_t *emptyslotp,uint64_t *nums,long max,uint64_t val)
{
    int32_t i;
    *emptyslotp = -1;
    for (i=0; i<max; i++)
    {
        if ( nums[i] == 0 )
        {
            *emptyslotp = i;
            break;
        }
        else if ( nums[i] == val )
        {
            if ( Debuglevel > 2 )
                printf("found in slot[%d] %llx\n",i,(long long)val);
            return(i);
        }
    }
    if ( Debuglevel > 2 )
        printf("emptyslot[%d] for %llx\n",i,(long long)val);
    return(-1);
}

int32_t add_uint64(uint64_t *nums,long max,uint64_t val)
{
    int32_t i,emptyslot;
    if ( (i= find_uint64(&emptyslot,nums,max,val)) >= 0 )
        return(i);
    else if ( emptyslot >= 0 )
    {
        nums[emptyslot] = val;
        return(emptyslot);
    } else return(-1);
}

int32_t remove_uint64(uint64_t *nums,long max,uint64_t val)
{
    int32_t i,emptyslot;
    if ( (i= find_uint64(&emptyslot,nums,max,val)) >= 0 )
    {
        nums[i] = 0;
        return(i);
    }
    return(-1);
}

int32_t is_DST(int32_t datenum)
{
    int32_t year,month,day;
    year = datenum / 10000, month = (datenum / 100) % 100, day = (datenum % 100);
    if ( month >= 4 && month <= 9 )
        return(1);
    else if ( month == 3 && day >= 29 )
        return(1);
    else if ( month == 10 && day < 25 )
        return(1);
    return(0);
}

int32_t extract_datenum(int32_t *yearp,int32_t *monthp,int32_t *dayp,int32_t datenum)
{
    *yearp = datenum / 10000, *monthp = (datenum / 100) % 100, *dayp = (datenum % 100);
    if ( *yearp >= 2000 && *yearp <= 2038 && *monthp >= 1 && *monthp <= 12 && *dayp >= 1 && *dayp <= 31 )
        return(datenum);
    else return(-1);
}

int32_t expand_datenum(char *date,int32_t datenum) { int32_t year,month,day; date[0] = 0; if ( extract_datenum(&year,&month,&day,datenum) != datenum) return(-1); sprintf(date,"%d-%02d-%02d",year,month,day); return(0); }

int32_t calc_datenum(int32_t year,int32_t month,int32_t day) { return((year * 10000) + (month * 100) + day); }

int32_t conv_date(int32_t *secondsp,char *date)
{
    char origdate[64],tmpdate[64]; int32_t year,month,day,hour,min,sec,len;
    strcpy(origdate,date), strcpy(tmpdate,date), tmpdate[8 + 2] = 0;
    year = myatoi(tmpdate,3000), month = myatoi(tmpdate+5,13), day = myatoi(tmpdate+8,32);
    *secondsp = 0;
    if ( (len= (int32_t)strlen(date)) <= 10 )
        hour = min = sec = 0;
    if ( len >= 18 )
    {
        tmpdate[11 + 2] = 0, tmpdate[14 + 2] = 0, tmpdate[17 + 2] = 0;
        hour = myatoi(tmpdate+11,25), min = myatoi(tmpdate + 14,61), sec = myatoi(tmpdate+17,61);
        if ( hour >= 0 && hour < 24 && min >= 0 && min < 60 && sec >= 0 && sec < 60 )
            *secondsp = (3600*hour + 60*min + sec);
        else printf("ERROR: seconds.%d %d %d %d, len.%d\n",*secondsp,hour,min,sec,len);
    }
    sprintf(origdate,"%d-%02d-%02d",year,month,day); //2015-07-25T22:34:31Z
    if ( strcmp(tmpdate,origdate) != 0 )
    {
        printf("conv_date date conversion error (%s) -> (%s)\n",origdate,date);
        return(-1);
    }
    return((year * 10000) + (month * 100) + day);
}

int32_t ecb_decrdate(int32_t *yearp,int32_t *monthp,int32_t *dayp,char *date,int32_t datenum)
{
    static int lastday[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int32_t year,month,day;
    year = datenum / 10000, month = (datenum / 100) % 100, day = (datenum % 100);
    //printf("%d -> %d %d %d\n",datenum,year,month,day);
    if ( --day <= 0 )
    {
        if ( --month <= 0 )
        {
            if ( --year < 2000 )
            {
                printf("reached epoch start\n");
                return(-1);
            }
            month = 12;
        }
        day = lastday[month];
        if ( month == 2 && (year % 4) == 0 )
            day++;
    }
    sprintf(date,"%d-%02d-%02d",year,month,day);
    //printf("%d -> %d %d %d (%s)\n",datenum,year,month,day,date);
    *yearp = year, *monthp = month, *dayp = day;
    return((year * 10000) + (month * 100) + day);
}

double init_emamult(double *emamult,double den)
{
	int32_t j; double smoothfactor,remainder,sum,sum2;
	remainder = 1.;
	smoothfactor = exp(1) / den;
	sum = 0.;
	for (j=0; j<16; j++)
	{
		emamult[j] = remainder * smoothfactor;
		sum += emamult[j];
		printf("%9.6f ",emamult[j]);
		remainder *= (1. - smoothfactor);
	}
	printf("-> emasum %f\n",sum);
	sum2 = 0.;
	for (j=0; j<16; j++)
	{
		emamult[j] /= sum;
		printf("%11.7f ",emamult[j]);
		sum2 += emamult[j];
	}
    printf("-> emasum2 %.20f\n",sum2);
	return(sum2);
}

int32_t myatoi(char *str,int32_t range)
{
    long x; char *ptr;
    x = strtol(str,&ptr,10);
    if ( range != 0 && x >= range )
        x = (range - 1);
    return((int32_t)x);
}

#endif
#endif
