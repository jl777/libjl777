//
//  jl777str.h

//  Created by jl777
//  MIT License
//

#ifndef gateway_jl777str_h
#define gateway_jl777str_h

#define NO_DEBUG_MALLOC


#define dstr(x) ((double)(x) / SATOSHIDEN)
long MY_ALLOCATED,NUM_ALLOCATED,MAX_ALLOCATED,*MY_ALLOCSIZES; void **PTRS;

void *mymalloc(long allocsize)
{
    void *ptr;
    MY_ALLOCATED += allocsize;
    ptr = malloc(allocsize);
    memset(ptr,0,allocsize);
#ifdef NO_DEBUG_MALLOC
    return(ptr);
#endif
    if ( NUM_ALLOCATED >= MAX_ALLOCATED )
    {
        MAX_ALLOCATED += 100;
        PTRS = (void **)realloc(PTRS,MAX_ALLOCATED * sizeof(*PTRS));
        MY_ALLOCSIZES = (long *)realloc(MY_ALLOCSIZES,MAX_ALLOCATED * sizeof(*MY_ALLOCSIZES));
    }
    MY_ALLOCSIZES[NUM_ALLOCATED] = allocsize;
    printf("%ld myalloc.%p %ld | %ld\n",NUM_ALLOCATED,ptr,allocsize,MY_ALLOCATED);
    PTRS[NUM_ALLOCATED++] = ptr;
    return(ptr);
}

void myfree(void *ptr,char *str)
{
    int32_t i;
    if ( ptr == 0 )
        return;
#ifdef NO_DEBUG_MALLOC
    free(ptr);
    return;
#endif
    //printf("%s: myfree.%p\n",str,ptr);
    for (i=0; i<NUM_ALLOCATED; i++)
    {
        if ( PTRS[i] == ptr )
        {
            MY_ALLOCATED -= MY_ALLOCSIZES[i];
            printf("%s: freeing %d of %ld | %ld\n",str,i,NUM_ALLOCATED,MY_ALLOCATED);
            if ( i != NUM_ALLOCATED-1 )
            {
                MY_ALLOCSIZES[i] = MY_ALLOCSIZES[NUM_ALLOCATED-1];
                PTRS[i] = PTRS[NUM_ALLOCATED-1];
            }
            NUM_ALLOCATED--;
            free(ptr);
            return;
        }
    }
    printf("couldn't find %p in PTRS[%ld]??\n",ptr,NUM_ALLOCATED);
    while ( 1 ) sleep(1);
    free(ptr);
}


/*
int32_t expand_nxt64bits(char *NXTaddr,uint64_t nxt64bits)
{
    int32_t i,n;
    uint64_t modval;
    char rev[64];
    for (i=0; nxt64bits!=0; i++)
    {
        modval = nxt64bits % 10;
        rev[i] = (char)(modval + '0');
        nxt64bits /= 10;
    }
    n = i;
    for (i=0; i<n; i++)
        NXTaddr[i] = rev[n-1-i];
    NXTaddr[i] = 0;
    return(n);
}

char *nxt64str(uint64_t nxt64bits)
{
    static char NXTaddr[64];
    expand_nxt64bits(NXTaddr,nxt64bits);
    return(NXTaddr);
}*/


/*
int32_t cmp_nxt64bits(const char *str,uint64_t nxt64bits)
{
    char expanded[64];
    if ( str == 0 )//|| str[0] == 0 || nxt64bits == 0 )
        return(-1);
    if ( nxt64bits == 0 && str[0] == 0 )
        return(0);
    expand_nxt64bits(expanded,nxt64bits);
    return(strcmp(str,expanded));
}

uint64_t calc_nxt64bits(const char *NXTaddr)
{
    int32_t c;
    int64_t n,i;
    uint64_t lastval,mult,nxt64bits = 0;
    n = strlen(NXTaddr);
    if ( n >= 22 )
    {
        printf("calc_nxt64bits: illegal NXTaddr.(%s) too long\n",NXTaddr);
        return(0);
    }
    else if ( strcmp(NXTaddr,"0") == 0 || strcmp(NXTaddr,"false") == 0 )
    {
       // printf("zero address?\n"); getchar();
        return(0);
    }
    mult = 1;
    lastval = 0;
    for (i=n-1; i>=0; i--,mult*=10)
    {
        c = NXTaddr[i];
        if ( c < '0' || c > '9' )
        {
            printf("calc_nxt64bits: illegal char.(%c %d) in (%s).%d\n",c,c,NXTaddr,(int32_t)i);
            return(0);
        }
        nxt64bits += mult * (c - '0');
        if ( nxt64bits < lastval )
            printf("calc_nxt64bits: warning: 64bit overflow %llx < %llx\n",(long long)nxt64bits,(long long)lastval);
        lastval = nxt64bits;
    }
    if ( cmp_nxt64bits(NXTaddr,nxt64bits) != 0 )
        printf("error calculating nxt64bits: %s -> %llx -> %s\n",NXTaddr,(long long)nxt64bits,nxt64str(nxt64bits));
    return(nxt64bits);
}*/

int32_t listcmp(char **list,char *str)
{
    if ( list == 0 || str == 0 )
        return(-1);
    while ( *list != 0 && *list[0] != 0 )
    {
        //printf("(%s vs %s)\n",*list,str);
        if ( strcmp(*list++,str) == 0 )
            return(0);
    }
    return(1);
}

char *clonestr(char *str)
{
    char *clone;
    if ( str == 0 || str[0] == 0 )
    {
        printf("warning cloning nullstr.%p\n",str); while ( 1 ) sleep(1);
        str = (char *)"<nullstr>";
    }
    clone = (char *)mymalloc(strlen(str)+1);
    strcpy(clone,str);
    return(clone);
}

/*int32_t safecopy(char *dest,char *src,long len)
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
            printf("%s too long %ld\n",src,len);
            return(-1);
        }
        dest[i] = 0;
    }
    return(i);
}*/

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

unsigned char _decode_hex(char *hex)
{
    return((unhex(hex[0])<<4) | unhex(hex[1]));
}

uint16_t _decode_hexshort(int32_t *offsetp,char *hex)
{
    if ( offsetp != 0 )
    {
        hex += *offsetp;
        *offsetp += 4;
    }
    return(_decode_hex(&hex[0]) | (_decode_hex(&hex[2]) << 8));
}

uint32_t _decode_hexint(int32_t *offsetp,char *hex)
{
    if ( offsetp != 0 )
    {
        hex += *offsetp;
        *offsetp += 8;
    }
    return(_decode_hexshort(0,&hex[0]) | (_decode_hexshort(0,&hex[4]) << 16));
}

uint64_t _decode_hexlong(int32_t *offsetp,char *hex)
{
    if ( offsetp != 0 )
    {
        hex += *offsetp;
        *offsetp += 16;
    }
    return(_decode_hexint(0,&hex[0]) | ((uint64_t)_decode_hexint(0,&hex[8]) << 32));
}

int64_t _decode_varint(int32_t *offsetp,char *hex)
{
    unsigned char x;
    x = _decode_hex(hex + *offsetp);
    *offsetp += 2;
    if ( x < 0xfd )
        return(x);
    switch ( x )
    {
        case 0xfd: return(_decode_hexshort(offsetp,hex)); break;
        case 0xfe: return(_decode_hexint(offsetp,hex)); break;
        case 0xff: return(_decode_hexlong(offsetp,hex)); break;
        default: printf("impossible switch val %x\n",x); break;
    }
    return(0);
}

int32_t decode_hex(unsigned char *bytes,int32_t n,char *hex)
{
    int32_t i;
    for (i=0; i<n; i++)
        bytes[i] = _decode_hex(&hex[i*2]);
    //bytes[i] = 0;
    return(n);
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

int32_t init_hexbytes(char *hexbytes,unsigned char *message,long len)
{
    int32_t i,lastnonz = -1;
    for (i=0; i<len; i++)
    {
        if ( message[i] != 0 )
        {
            lastnonz = i;
            hexbytes[i*2] = hexbyte((message[i]>>4) & 0xf);
            hexbytes[i*2 + 1] = hexbyte(message[i] & 0xf);
        }
        else hexbytes[i*2] = hexbytes[i*2+1] = '0';
        //printf("i.%d (%02x) [%c%c] last.%d\n",i,message[i],hexbytes[i*2],hexbytes[i*2+1],lastnonz);
    }
    lastnonz++;
    hexbytes[lastnonz*2] = 0;
    return(lastnonz*2+1);
}

int32_t init_hexbytes_noT(char *hexbytes,unsigned char *message,long len)
{
    int32_t i;
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

void zero_last128(char *dest,char *src)
{
    int32_t i,n;
    n = (int32_t)strlen(src);
    strcpy(dest,src);
    if ( n < 128 )
    {
        for (i=0; i<n; i++)
            dest[i] = '0';
    }
    else
    {
        for (i=n-128; i<n; i++)
            dest[i] = '0';
    }
}

void tolowercase(char *str)
{
    int32_t i;
    if ( str == 0 || str[0] == 0 )
        return;
    for (i=0; str[i]!=0; i++)
        str[i] = tolower(str[i]);
}

void freep(void **ptrp)
{
	if ( *ptrp != 0 )
	{
		free(*ptrp);
		*ptrp = 0;
	}
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

char *replacequotes(char *str)
{
    char *newstr;
    int32_t i,j,n;
    for (i=n=0; str[i]!=0; i++)
        n += (str[i] == '"') ? 3 : 1;
    newstr = (char *)malloc(n + 1);
    for (i=j=0; str[i]!=0; i++)
    {
        if ( str[i] == '"' )
        {
            newstr[j++] = '%';
            newstr[j++] = '2';
            newstr[j++] = '2';
        }
        else newstr[j++] = str[i];
    }
    newstr[j] = 0;
    free(str);
    return(newstr);
}

char *convert_percent22(char *str)
{
    int32_t i,j;
    for (i=j=0; str[i]!=0; i++)
    {
        if ( str[i] == '%' && str[i+1] != 0 && str[i+1] == '2' && str[i+2] == '2' )
            str[j++] = '"', i += 2;
        else str[j++] = str[i];
    }
    str[j] = 0;
    return(str);
}

char *replace_singlequotes(char *str)
{
    int32_t i;
    if ( str == 0 || str[0] == 0 )
        return(0);
    for (i=0; str[i]!=0; i++)
    {
        if ( str[i] == '\'' )
        {
            if ( str[i+1] == '\'' )
                str[i] = '\\', str[i+1] = '"', i++;
            else
                str[i] = '"';
        }
    }
    return(str);
}

char *replace_backslashquotes(char *str)
{
    int32_t i,j,n;
    if ( str == 0 )
        return(0);
    n = (int32_t)strlen(str);
    if ( str[0] == '"' && str[n-1] == '"' )
        str[n-1] = 0, i = 1;
    else i = 0;
    for (j=0; str[i]!=0; i++)
    {
        if ( str[i] == '\\' && str[i+1] == '"' )
            str[j++] = '"', i++;
        else str[j++] = str[i];
    }
    str[j] = 0;
    return(str);
}

long stripwhite(char *buf,long len)
{
    int32_t i,j,c;
    for (i=j=0; i<len; i++)
    {
        c = buf[i];
        //if ( c == '\\' )
        //    c = buf[i+1], i++;
        buf[j] = c;
        if ( buf[j] != ' ' && buf[j] != '\n' && buf[j] != '\r' && buf[j] != '\t' )
            j++;
    }
    buf[j] = 0;
    return(j);
}

long stripwhite_ns(char *buf,long len)
{
    int32_t i,j,c;
    for (i=j=0; i<len; i++)
    {
        c = buf[i];
        buf[j] = c;
        if ( buf[j] != '\n' && buf[j] != '\r' && buf[j] != '\t' )
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

char *lastpart(char *fname)
{
    long i,n = strlen(fname);
    for (i=n-1; i>=0; i--)
        if ( fname[i] == '/' )
            return(&fname[i]);
    return(fname);
}
/*-
 *  COPYRIGHT (C) 1986 Gary S. Brown.  You may use this program, or
 *  code or tables extracted from it, as desired without restriction.
 *
 *  First, the polynomial itself and its table of feedback terms.  The
 *  polynomial is
 *  X^32+X^26+X^23+X^22+X^16+X^12+X^11+X^10+X^8+X^7+X^5+X^4+X^2+X^1+X^0
 *
 *  Note that we take it "backwards" and put the highest-order term in
 *  the lowest-order bit.  The X^32 term is "implied"; the LSB is the
 *  X^31 term, etc.  The X^0 term (usually shown as "+1") results in
 *  the MSB being 1
 *
 *  Note that the usual hardware shift register implementation, which
 *  is what we're using (we're merely optimizing it by doing eight-bit
 *  chunks at a time) shifts bits into the lowest-order term.  In our
 *  implementation, that means shifting towards the right.  Why do we
 *  do it this way?  Because the calculated CRC must be transmitted in
 *  order from highest-order term to lowest-order term.  UARTs transmit
 *  characters in order from LSB to MSB.  By storing the CRC this way
 *  we hand it to the UART in the order low-byte to high-byte; the UART
 *  sends each low-bit to hight-bit; and the result is transmission bit
 *  by bit from highest- to lowest-order term without requiring any bit
 *  shuffling on our part.  Reception works similarly
 *
 *  The feedback terms table consists of 256, 32-bit entries.  Notes
 *
 *      The table can be generated at runtime if desired; code to do so
 *      is shown later.  It might not be obvious, but the feedback
 *      terms simply represent the results of eight shift/xor opera
 *      tions for all combinations of data and CRC register values
 *
 *      The values must be right-shifted by eight bits by the "updcrc
 *      logic; the shift must be unsigned (bring in zeroes).  On some
 *      hardware you could probably optimize the shift in assembler by
 *      using byte-swap instructions
 *      polynomial $edb88320
 *
 *
 * CRC32 code derived from work by Gary S. Brown.
 */

//#include <sys/param.h>
//#include <sys/system.h>

static uint32_t crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t _crc32(uint32_t crc, const void *buf, size_t size)
{
	const uint8_t *p;
    
	p = (const uint8_t *)buf;
	crc = crc ^ ~0U;
    
	while (size--)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    
	return crc ^ ~0U;
}

#ifndef __linux__
char *strsep(char **stringp,const char *delim)
{
	char *s;
	const char *spanp;
	int c, sc;
	char *tok;
	if ((s = *stringp) == NULL)
		return (NULL);
	for (tok = s;;) {
		c = *s++;
		spanp = delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
                    else
                        s[-1] = 0;
                        *stringp = s;
                        return (tok);
			}
		} while (sc != 0);
	}
}
#endif

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
            return(-1);
    return(i);
}

#endif
