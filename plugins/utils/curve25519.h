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
// derived from curve25519_donna

#ifndef dcnet_curve25519_h
#define dcnet_curve25519_h


// special gcc mode for 128-bit integers. It's implemented on 64-bit platforms only as far as I know.
typedef unsigned uint128_t __attribute__((mode(TI)));
#undef force_inline
#define force_inline __attribute__((always_inline))

// Sum two numbers: output += in
static inline bits320 force_inline fsum(bits320 output,bits320 in)
{
    int32_t i;
    for (i=0; i<5; i++)
        output.ulongs[i] += in.ulongs[i];
    return(output);
}

static inline void force_inline fdifference_backwards(uint64_t *out,const uint64_t *in)
{
    static const uint64_t two54m152 = (((uint64_t)1) << 54) - 152;  // 152 is 19 << 3
    static const uint64_t two54m8 = (((uint64_t)1) << 54) - 8;
    int32_t i;
    out[0] = in[0] + two54m152 - out[0];
    for (i=1; i<5; i++)
        out[i] = in[i] + two54m8 - out[i];
}

// Multiply a number by a scalar: output = in * scalar
static inline bits320 force_inline fscalar_product(const bits320 in,const uint64_t scalar)
{
    int32_t i; uint128_t a = 0; bits320 output;
    a = ((uint128_t)in.ulongs[0]) * scalar;
    output.ulongs[0] = ((uint64_t)a) & 0x7ffffffffffff;
    for (i=1; i<5; i++)
    {
        a = ((uint128_t)in.ulongs[i]) * scalar + ((uint64_t) (a >> 51));
        output.ulongs[i] = ((uint64_t)a) & 0x7ffffffffffff;
    }
    output.ulongs[0] += (a >> 51) * 19;
    return(output);
}

// Multiply two numbers: output = in2 * in
// output must be distinct to both inputs. The inputs are reduced coefficient form, the output is not.
// Assumes that in[i] < 2**55 and likewise for in2. On return, output[i] < 2**52
static inline bits320 force_inline fmul(const bits320 in2,const bits320 in)
{
    uint128_t t[5]; uint64_t r0,r1,r2,r3,r4,s0,s1,s2,s3,s4,c; bits320 out;
    r0 = in.ulongs[0], r1 = in.ulongs[1], r2 = in.ulongs[2], r3 = in.ulongs[3], r4 = in.ulongs[4];
    s0 = in2.ulongs[0], s1 = in2.ulongs[1], s2 = in2.ulongs[2], s3 = in2.ulongs[3], s4 = in2.ulongs[4];
    t[0]  =  ((uint128_t) r0) * s0;
    t[1]  =  ((uint128_t) r0) * s1 + ((uint128_t) r1) * s0;
    t[2]  =  ((uint128_t) r0) * s2 + ((uint128_t) r2) * s0 + ((uint128_t) r1) * s1;
    t[3]  =  ((uint128_t) r0) * s3 + ((uint128_t) r3) * s0 + ((uint128_t) r1) * s2 + ((uint128_t) r2) * s1;
    t[4]  =  ((uint128_t) r0) * s4 + ((uint128_t) r4) * s0 + ((uint128_t) r3) * s1 + ((uint128_t) r1) * s3 + ((uint128_t) r2) * s2;
    r4 *= 19, r1 *= 19, r2 *= 19, r3 *= 19;
    t[0] += ((uint128_t) r4) * s1 + ((uint128_t) r1) * s4 + ((uint128_t) r2) * s3 + ((uint128_t) r3) * s2;
    t[1] += ((uint128_t) r4) * s2 + ((uint128_t) r2) * s4 + ((uint128_t) r3) * s3;
    t[2] += ((uint128_t) r4) * s3 + ((uint128_t) r3) * s4;
    t[3] += ((uint128_t) r4) * s4;
    r0 = (uint64_t)t[0] & 0x7ffffffffffff; c = (uint64_t)(t[0] >> 51);
    t[1] += c;      r1 = (uint64_t)t[1] & 0x7ffffffffffff; c = (uint64_t)(t[1] >> 51);
    t[2] += c;      r2 = (uint64_t)t[2] & 0x7ffffffffffff; c = (uint64_t)(t[2] >> 51);
    t[3] += c;      r3 = (uint64_t)t[3] & 0x7ffffffffffff; c = (uint64_t)(t[3] >> 51);
    t[4] += c;      r4 = (uint64_t)t[4] & 0x7ffffffffffff; c = (uint64_t)(t[4] >> 51);
    r0 +=   c * 19; c = r0 >> 51; r0 = r0 & 0x7ffffffffffff;
    r1 +=   c;      c = r1 >> 51; r1 = r1 & 0x7ffffffffffff;
    r2 +=   c;
    out.ulongs[0] = r0, out.ulongs[1] = r1, out.ulongs[2] = r2, out.ulongs[3] = r3, out.ulongs[4] = r4;
    return(out);
}

static inline bits320 force_inline fsquare_times(const bits320 in,uint64_t count)
{
    uint128_t t[5]; uint64_t r0,r1,r2,r3,r4,c,d0,d1,d2,d4,d419; bits320 out;
    r0 = in.ulongs[0], r1 = in.ulongs[1], r2 = in.ulongs[2], r3 = in.ulongs[3], r4 = in.ulongs[4];
    do
    {
        d0 = r0 * 2;
        d1 = r1 * 2;
        d2 = r2 * 2 * 19;
        d419 = r4 * 19;
        d4 = d419 * 2;
        t[0] = ((uint128_t) r0) * r0 + ((uint128_t) d4) * r1 + (((uint128_t) d2) * (r3     ));
        t[1] = ((uint128_t) d0) * r1 + ((uint128_t) d4) * r2 + (((uint128_t) r3) * (r3 * 19));
        t[2] = ((uint128_t) d0) * r2 + ((uint128_t) r1) * r1 + (((uint128_t) d4) * (r3     ));
        t[3] = ((uint128_t) d0) * r3 + ((uint128_t) d1) * r2 + (((uint128_t) r4) * (d419   ));
        t[4] = ((uint128_t) d0) * r4 + ((uint128_t) d1) * r3 + (((uint128_t) r2) * (r2     ));
        
        r0 = (uint64_t)t[0] & 0x7ffffffffffff; c = (uint64_t)(t[0] >> 51);
        t[1] += c;      r1 = (uint64_t)t[1] & 0x7ffffffffffff; c = (uint64_t)(t[1] >> 51);
        t[2] += c;      r2 = (uint64_t)t[2] & 0x7ffffffffffff; c = (uint64_t)(t[2] >> 51);
        t[3] += c;      r3 = (uint64_t)t[3] & 0x7ffffffffffff; c = (uint64_t)(t[3] >> 51);
        t[4] += c;      r4 = (uint64_t)t[4] & 0x7ffffffffffff; c = (uint64_t)(t[4] >> 51);
        r0 +=   c * 19; c = r0 >> 51; r0 = r0 & 0x7ffffffffffff;
        r1 +=   c;      c = r1 >> 51; r1 = r1 & 0x7ffffffffffff;
        r2 +=   c;
    } while( --count );
    out.ulongs[0] = r0, out.ulongs[1] = r1, out.ulongs[2] = r2, out.ulongs[3] = r3, out.ulongs[4] = r4;
    return(out);
}

static inline void force_inline store_limb(uint8_t *out,uint64_t in)
{
    int32_t i;
    for (i=0; i<8; i++,in>>=8)
        out[i] = (in & 0xff);
}

static inline uint64_t force_inline load_limb(uint8_t *in)
{
    return
    ((uint64_t)in[0]) |
    (((uint64_t)in[1]) << 8) |
    (((uint64_t)in[2]) << 16) |
    (((uint64_t)in[3]) << 24) |
    (((uint64_t)in[4]) << 32) |
    (((uint64_t)in[5]) << 40) |
    (((uint64_t)in[6]) << 48) |
    (((uint64_t)in[7]) << 56);
}

// Take a little-endian, 32-byte number and expand it into polynomial form
static inline bits320 force_inline fexpand(bits256 basepoint)
{
    bits320 out;
    out.ulongs[0] = load_limb(basepoint.bytes) & 0x7ffffffffffff;
    out.ulongs[1] = (load_limb(basepoint.bytes+6) >> 3) & 0x7ffffffffffff;
    out.ulongs[2] = (load_limb(basepoint.bytes+12) >> 6) & 0x7ffffffffffff;
    out.ulongs[3] = (load_limb(basepoint.bytes+19) >> 1) & 0x7ffffffffffff;
    out.ulongs[4] = (load_limb(basepoint.bytes+24) >> 12) & 0x7ffffffffffff;
    return(out);
}

static inline void force_inline fcontract_iter(uint128_t t[5],int32_t flag)
{
    int32_t i; uint64_t mask = 0x7ffffffffffff;
    for (i=0; i<4; i++)
        t[i+1] += t[i] >> 51, t[i] &= mask;
    if ( flag != 0 )
        t[0] += 19 * (t[4] >> 51); t[4] &= mask;
}

// donna: Take a fully reduced polynomial form number and contract it into a little-endian, 32-byte array
static inline bits256 force_inline fcontract(const bits320 input)
{
    uint128_t t[5]; int32_t i; bits256 out;
    for (i=0; i<5; i++)
        t[i] = input.ulongs[i];
    fcontract_iter(t,1), fcontract_iter(t,1);
    // donna: now t is between 0 and 2^255-1, properly carried.
    // donna: case 1: between 0 and 2^255-20. case 2: between 2^255-19 and 2^255-1.
    t[0] += 19, fcontract_iter(t,1);
    // now between 19 and 2^255-1 in both cases, and offset by 19.
    t[0] += 0x8000000000000 - 19;
    for (i=1; i<5; i++)
        t[i] += 0x8000000000000 - 1;
    // now between 2^255 and 2^256-20, and offset by 2^255.
    fcontract_iter(t,0);
    store_limb(out.bytes,t[0] | (t[1] << 51));
    store_limb(out.bytes+8,(t[1] >> 13) | (t[2] << 38));
    store_limb(out.bytes+16,(t[2] >> 26) | (t[3] << 25));
    store_limb(out.bytes+24,(t[3] >> 39) | (t[4] << 12));
    return(out);
}

// Input: Q, Q', Q-Q' -> Output: 2Q, Q+Q'
// x2 z2: long form && x3 z3: long form
// x z: short form, destroyed && xprime zprime: short form, destroyed
// qmqp: short form, preserved
static inline void force_inline
fmonty(bits320 *x2, bits320 *z2, // output 2Q
       bits320 *x3, bits320 *z3, // output Q + Q'
       bits320 *x, bits320 *z,   // input Q
       bits320 *xprime, bits320 *zprime, // input Q'
       const bits320 qmqp) // input Q - Q'
{
    bits320 origx,origxprime,zzz,xx,zz,xxprime,zzprime;
    origx = *x;
    *x = fsum(*x,*z), fdifference_backwards(z->ulongs,origx.ulongs);  // does x - z
    origxprime = *xprime;
    *xprime = fsum(*xprime,*zprime), fdifference_backwards(zprime->ulongs,origxprime.ulongs);
    xxprime = fmul(*xprime,*z), zzprime = fmul(*x,*zprime);
    origxprime = xxprime;
    xxprime = fsum(xxprime,zzprime), fdifference_backwards(zzprime.ulongs,origxprime.ulongs);
    *x3 = fsquare_times(xxprime,1), *z3 = fmul(fsquare_times(zzprime,1),qmqp);
    xx = fsquare_times(*x,1), zz = fsquare_times(*z,1);
    *x2 = fmul(xx,zz);
    fdifference_backwards(zz.ulongs,xx.ulongs);  // does zz = xx - zz
    zzz = fscalar_product(zz,121665);
    *z2 = fmul(zz,fsum(zzz,xx));
}

// -----------------------------------------------------------------------------
// Maybe swap the contents of two limb arrays (@a and @b), each @len elements
// long. Perform the swap iff @swap is non-zero.
// This function performs the swap without leaking any side-channel information.
// -----------------------------------------------------------------------------
static inline void force_inline swap_conditional(bits320 *a,bits320 *b,uint64_t iswap)
{
    int32_t i; const uint64_t swap = -iswap;
    for (i=0; i<5; ++i)
    {
        const uint64_t x = swap & (a->ulongs[i] ^ b->ulongs[i]);
        a->ulongs[i] ^= x, b->ulongs[i] ^= x;
    }
}

// Calculates nQ where Q is the x-coordinate of a point on the curve
// resultx/resultz: the x coordinate of the resulting curve point (short form)
// n: a little endian, 32-byte number
// q: a point of the curve (short form)
static inline void force_inline cmult(bits320 *resultx,bits320 *resultz,bits256 secret,const bits320 q)
{
    int32_t i,j; bits320 a,b,c,d,e,f,g,h,*t;
    bits320 Zero320bits,One320bits, *nqpqx = &a,*nqpqz = &b,*nqx = &c,*nqz = &d,*nqpqx2 = &e,*nqpqz2 = &f,*nqx2 = &g,*nqz2 = &h;
    memset(&Zero320bits,0,sizeof(Zero320bits));
    memset(&One320bits,0,sizeof(One320bits)), One320bits.ulongs[0] = 1;
    a = d = e = g = Zero320bits, b = c = f = h = One320bits;
    *nqpqx = q;
    for (i=0; i<32; i++)
    {
        uint8_t byte = secret.bytes[31 - i];
        for (j=0; j<8; j++)
        {
            const uint64_t bit = byte >> 7;
            swap_conditional(nqx,nqpqx,bit), swap_conditional(nqz,nqpqz,bit);
            fmonty(nqx2,nqz2,nqpqx2,nqpqz2,nqx,nqz,nqpqx,nqpqz,q);
            swap_conditional(nqx2,nqpqx2,bit), swap_conditional(nqz2,nqpqz2,bit);
            t = nqx, nqx = nqx2, nqx2 = t;
            t = nqz, nqz = nqz2, nqz2 = t;
            t = nqpqx, nqpqx = nqpqx2, nqpqx2 = t;
            t = nqpqz, nqpqz = nqpqz2, nqpqz2 = t;
            byte <<= 1;
        }
    }
    *resultx = *nqx, *resultz = *nqz;
}

// Shamelessly copied from donna's code that copied djb's code, changed a little
static inline bits320 force_inline crecip(const bits320 z)
{
    bits320 a,t0,b,c;
    /* 2 */ a = fsquare_times(z, 1); // a = 2
    /* 8 */ t0 = fsquare_times(a, 2);
    /* 9 */ b = fmul(t0, z); // b = 9
    /* 11 */ a = fmul(b, a); // a = 11
    /* 22 */ t0 = fsquare_times(a, 1);
    /* 2^5 - 2^0 = 31 */ b = fmul(t0, b);
    /* 2^10 - 2^5 */ t0 = fsquare_times(b, 5);
    /* 2^10 - 2^0 */ b = fmul(t0, b);
    /* 2^20 - 2^10 */ t0 = fsquare_times(b, 10);
    /* 2^20 - 2^0 */ c = fmul(t0, b);
    /* 2^40 - 2^20 */ t0 = fsquare_times(c, 20);
    /* 2^40 - 2^0 */ t0 = fmul(t0, c);
    /* 2^50 - 2^10 */ t0 = fsquare_times(t0, 10);
    /* 2^50 - 2^0 */ b = fmul(t0, b);
    /* 2^100 - 2^50 */ t0 = fsquare_times(b, 50);
    /* 2^100 - 2^0 */ c = fmul(t0, b);
    /* 2^200 - 2^100 */ t0 = fsquare_times(c, 100);
    /* 2^200 - 2^0 */ t0 = fmul(t0, c);
    /* 2^250 - 2^50 */ t0 = fsquare_times(t0, 50);
    /* 2^250 - 2^0 */ t0 = fmul(t0, b);
    /* 2^255 - 2^5 */ t0 = fsquare_times(t0, 5);
    /* 2^255 - 21 */ return(fmul(t0, a));
}

// following is ported from libtom
struct sha256_vstate { uint64_t length; uint32_t state[8],curlen; uint8_t buf[64]; };

#define STORE32L(x, y)                                                                     \
{ (y)[3] = (uint8_t)(((x)>>24)&255); (y)[2] = (uint8_t)(((x)>>16)&255);   \
(y)[1] = (uint8_t)(((x)>>8)&255); (y)[0] = (uint8_t)((x)&255); }

#define LOAD32L(x, y)                            \
{ x = (uint32_t)(((uint64_t)((y)[3] & 255)<<24) | \
((uint32_t)((y)[2] & 255)<<16) | \
((uint32_t)((y)[1] & 255)<<8)  | \
((uint32_t)((y)[0] & 255))); }

#define STORE64L(x, y)                                                                     \
{ (y)[7] = (uint8_t)(((x)>>56)&255); (y)[6] = (uint8_t)(((x)>>48)&255);   \
(y)[5] = (uint8_t)(((x)>>40)&255); (y)[4] = (uint8_t)(((x)>>32)&255);   \
(y)[3] = (uint8_t)(((x)>>24)&255); (y)[2] = (uint8_t)(((x)>>16)&255);   \
(y)[1] = (uint8_t)(((x)>>8)&255); (y)[0] = (uint8_t)((x)&255); }

#define LOAD64L(x, y)                                                       \
{ x = (((uint64_t)((y)[7] & 255))<<56)|(((uint64_t)((y)[6] & 255))<<48)| \
(((uint64_t)((y)[5] & 255))<<40)|(((uint64_t)((y)[4] & 255))<<32)| \
(((uint64_t)((y)[3] & 255))<<24)|(((uint64_t)((y)[2] & 255))<<16)| \
(((uint64_t)((y)[1] & 255))<<8)|(((uint64_t)((y)[0] & 255))); }

#define STORE32H(x, y)                                                                     \
{ (y)[0] = (uint8_t)(((x)>>24)&255); (y)[1] = (uint8_t)(((x)>>16)&255);   \
(y)[2] = (uint8_t)(((x)>>8)&255); (y)[3] = (uint8_t)((x)&255); }

#define LOAD32H(x, y)                            \
{ x = (uint32_t)(((uint64_t)((y)[0] & 255)<<24) | \
((uint32_t)((y)[1] & 255)<<16) | \
((uint32_t)((y)[2] & 255)<<8)  | \
((uint32_t)((y)[3] & 255))); }

#define STORE64H(x, y)                                                                     \
{ (y)[0] = (uint8_t)(((x)>>56)&255); (y)[1] = (uint8_t)(((x)>>48)&255);     \
(y)[2] = (uint8_t)(((x)>>40)&255); (y)[3] = (uint8_t)(((x)>>32)&255);     \
(y)[4] = (uint8_t)(((x)>>24)&255); (y)[5] = (uint8_t)(((x)>>16)&255);     \
(y)[6] = (uint8_t)(((x)>>8)&255); (y)[7] = (uint8_t)((x)&255); }

#define LOAD64H(x, y)                                                      \
{ x = (((uint64_t)((y)[0] & 255))<<56)|(((uint64_t)((y)[1] & 255))<<48) | \
(((uint64_t)((y)[2] & 255))<<40)|(((uint64_t)((y)[3] & 255))<<32) | \
(((uint64_t)((y)[4] & 255))<<24)|(((uint64_t)((y)[5] & 255))<<16) | \
(((uint64_t)((y)[6] & 255))<<8)|(((uint64_t)((y)[7] & 255))); }

// Various logical functions
#define RORc(x, y) ( ((((uint32_t)(x)&0xFFFFFFFFUL)>>(uint32_t)((y)&31)) | ((uint32_t)(x)<<(uint32_t)(32-((y)&31)))) & 0xFFFFFFFFUL)
#define Ch(x,y,z)       (z ^ (x & (y ^ z)))
#define Maj(x,y,z)      (((x | y) & z) | (x & y))
#define S(x, n)         RORc((x),(n))
#define R(x, n)         (((x)&0xFFFFFFFFUL)>>(n))
#define Sigma0(x)       (S(x, 2) ^ S(x, 13) ^ S(x, 22))
#define Sigma1(x)       (S(x, 6) ^ S(x, 11) ^ S(x, 25))
#define Gamma0(x)       (S(x, 7) ^ S(x, 18) ^ R(x, 3))
#define Gamma1(x)       (S(x, 17) ^ S(x, 19) ^ R(x, 10))
#define MIN(x, y) ( ((x)<(y))?(x):(y) )

static inline int32_t sha256_vcompress(struct sha256_vstate * md,uint8_t *buf)
{
    uint32_t S[8],W[64],t0,t1,i;
    for (i=0; i<8; i++) // copy state into S
        S[i] = md->state[i];
    for (i=0; i<16; i++) // copy the state into 512-bits into W[0..15]
        LOAD32H(W[i],buf + (4*i));
    for (i=16; i<64; i++) // fill W[16..63]
        W[i] = Gamma1(W[i - 2]) + W[i - 7] + Gamma0(W[i - 15]) + W[i - 16];
    
#define RND(a,b,c,d,e,f,g,h,i,ki)                    \
t0 = h + Sigma1(e) + Ch(e, f, g) + ki + W[i];   \
t1 = Sigma0(a) + Maj(a, b, c);                  \
d += t0;                                        \
h  = t0 + t1;
    
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],0,0x428a2f98);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],1,0x71374491);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],2,0xb5c0fbcf);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],3,0xe9b5dba5);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],4,0x3956c25b);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],5,0x59f111f1);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],6,0x923f82a4);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],7,0xab1c5ed5);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],8,0xd807aa98);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],9,0x12835b01);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],10,0x243185be);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],11,0x550c7dc3);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],12,0x72be5d74);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],13,0x80deb1fe);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],14,0x9bdc06a7);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],15,0xc19bf174);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],16,0xe49b69c1);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],17,0xefbe4786);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],18,0x0fc19dc6);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],19,0x240ca1cc);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],20,0x2de92c6f);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],21,0x4a7484aa);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],22,0x5cb0a9dc);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],23,0x76f988da);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],24,0x983e5152);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],25,0xa831c66d);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],26,0xb00327c8);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],27,0xbf597fc7);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],28,0xc6e00bf3);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],29,0xd5a79147);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],30,0x06ca6351);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],31,0x14292967);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],32,0x27b70a85);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],33,0x2e1b2138);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],34,0x4d2c6dfc);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],35,0x53380d13);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],36,0x650a7354);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],37,0x766a0abb);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],38,0x81c2c92e);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],39,0x92722c85);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],40,0xa2bfe8a1);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],41,0xa81a664b);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],42,0xc24b8b70);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],43,0xc76c51a3);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],44,0xd192e819);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],45,0xd6990624);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],46,0xf40e3585);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],47,0x106aa070);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],48,0x19a4c116);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],49,0x1e376c08);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],50,0x2748774c);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],51,0x34b0bcb5);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],52,0x391c0cb3);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],53,0x4ed8aa4a);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],54,0x5b9cca4f);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],55,0x682e6ff3);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],56,0x748f82ee);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],57,0x78a5636f);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],58,0x84c87814);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],59,0x8cc70208);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],60,0x90befffa);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],61,0xa4506ceb);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],62,0xbef9a3f7);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],63,0xc67178f2);
#undef RND
    for (i=0; i<8; i++) // feedback
        md->state[i] = md->state[i] + S[i];
    return(0);
}

static inline void sha256_vinit(struct sha256_vstate * md)
{
    md->curlen = 0;
    md->length = 0;
    md->state[0] = 0x6A09E667UL;
    md->state[1] = 0xBB67AE85UL;
    md->state[2] = 0x3C6EF372UL;
    md->state[3] = 0xA54FF53AUL;
    md->state[4] = 0x510E527FUL;
    md->state[5] = 0x9B05688CUL;
    md->state[6] = 0x1F83D9ABUL;
    md->state[7] = 0x5BE0CD19UL;
}

static inline int32_t sha256_vprocess(struct sha256_vstate *md,const uint8_t *in,uint64_t inlen)
{
    uint64_t n; int32_t err;
    if ( md->curlen > sizeof(md->buf) )
        return(-1);
    while ( inlen > 0 )
    {
        if ( md->curlen == 0 && inlen >= 64 )
        {
            if ( (err= sha256_vcompress(md,(uint8_t *)in)) != 0 )
                return(err);
            md->length += 64 * 8, in += 64, inlen -= 64;
        }
        else
        {
            n = MIN(inlen,64 - md->curlen);
            memcpy(md->buf + md->curlen,in,(size_t)n);
            md->curlen += n, in += n, inlen -= n;
            if ( md->curlen == 64 )
            {
                if ( (err= sha256_vcompress(md,md->buf)) != 0 )
                    return(err);
                md->length += 8*64;
                md->curlen = 0;
            }
        }
    }
    return(0);
}

static inline int32_t sha256_vdone(struct sha256_vstate *md,uint8_t *out)
{
    int32_t i;
    if ( md->curlen >= sizeof(md->buf) )
        return(-1);
    md->length += md->curlen * 8; // increase the length of the message
    md->buf[md->curlen++] = (uint8_t)0x80; // append the '1' bit
    // if len > 56 bytes we append zeros then compress.  Then we can fall back to padding zeros and length encoding like normal.
    if ( md->curlen > 56 )
    {
        while ( md->curlen < 64 )
            md->buf[md->curlen++] = (uint8_t)0;
        sha256_vcompress(md,md->buf);
        md->curlen = 0;
    }
    while ( md->curlen < 56 ) // pad upto 56 bytes of zeroes
        md->buf[md->curlen++] = (uint8_t)0;
    STORE64H(md->length,md->buf+56); // store length
    sha256_vcompress(md,md->buf);
    for (i=0; i<8; i++) // copy output
        STORE32H(md->state[i],out+(4*i));
    return(0);
}

void vcalc_sha256(char hashstr[(256 >> 3) * 2 + 1],uint8_t hash[256 >> 3],uint8_t *src,int32_t len)
{
    struct sha256_vstate md;
    sha256_vinit(&md);
    sha256_vprocess(&md,src,len);
    sha256_vdone(&md,hash);
    if ( hashstr != 0 )
    {
        int32_t init_hexbytes_noT(char *hexbytes,uint8_t *message,long len);
        init_hexbytes_noT(hashstr,hash,256 >> 3);
    }
}

void vcalc_sha256cat(uint8_t hash[256 >> 3],uint8_t *src,int32_t len,uint8_t *src2,int32_t len2)
{
    struct sha256_vstate md;
    sha256_vinit(&md);
    sha256_vprocess(&md,src,len);
    if ( src2 != 0 )
        sha256_vprocess(&md,src2,len2);
    sha256_vdone(&md,hash);
}

void vupdate_sha256(uint8_t hash[256 >> 3],struct sha256_vstate *state,uint8_t *src,int32_t len)
{
    struct sha256_vstate md;
    memset(&md,0,sizeof(md));
    if ( src == 0 )
        sha256_vinit(&md);
    else
    {
        md = *state;
        sha256_vprocess(&md,src,len);
    }
    *state = md;
    sha256_vdone(&md,hash);
}

#endif
