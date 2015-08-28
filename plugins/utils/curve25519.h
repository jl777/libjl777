//
//  curve25519.h
//  dcnet
//
//  Created by jimbo laptop on 8/25/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

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

static inline void force_inline fcontract_iter(uint128_t t[5])
{
    int32_t i; uint64_t mask = 0x7ffffffffffff;
    for (i=0; i<4; i++)
        t[i+1] += t[i] >> 51, t[i] &= mask;
    t[0] += 19 * (t[4] >> 51); t[4] &= mask;
}

// donna: Take a fully reduced polynomial form number and contract it into a little-endian, 32-byte array
static inline bits256 force_inline fcontract(const bits320 input)
{
    uint128_t t[5]; int32_t i; bits256 out;
    for (i=0; i<5; i++)
        t[i] = input.ulongs[i];
    fcontract_iter(t), fcontract_iter(t);
    // donna: now t is between 0 and 2^255-1, properly carried.
    // donna: case 1: between 0 and 2^255-20. case 2: between 2^255-19 and 2^255-1.
    t[0] += 19, fcontract_iter(t);
    // now between 19 and 2^255-1 in both cases, and offset by 19.
    t[0] += 0x8000000000000 - 19;
    for (i=1; i<5; i++)
        t[i] += 0x8000000000000 - 1;
    // now between 2^255 and 2^256-20, and offset by 2^255.
    fcontract_iter(t);
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

#endif
