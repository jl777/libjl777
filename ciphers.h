//
//  ciphers.h
//  xcode
//
//  Created by jl777 on 8/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
// this file is mostly cut and pasted from libtom files. Tom has great code!!
//

#ifndef xcode_ciphers_h
#define xcode_ciphers_h

#include "libtom/tomcrypt.h"

struct ltc_hash_descriptor hash_descriptor[TAB_SIZE] =
{
    { NULL, 0, 0, 0, { 0 }, 0, NULL, NULL, NULL, NULL, NULL }
};

struct ltc_cipher_descriptor cipher_descriptor[TAB_SIZE] =
{
    { NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

struct ltc_prng_descriptor prng_descriptor[TAB_SIZE] =
{
    { NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

char *Cipherstrs[] =
{
    "aes","blowfish","xtea","rc5","rc6","saferp","twofish","safer_k64","safer_sk64","safer_k128",
    "safer_sk128","rc2","des3","cast5","noekeon","skipjack","khazad","anubis","rijndael"
};
#define NUM_CIPHERS ((int)(sizeof(Cipherstrs)/sizeof(*Cipherstrs)))
const struct ltc_cipher_descriptor *Ciphers[NUM_CIPHERS];

int32_t myregister_cipher(int32_t cipherid,const struct ltc_cipher_descriptor *cipher)
{
    int32_t x;
    LTC_ARGCHK(cipher != NULL);
    // is it already registered?
    //LTC_MUTEX_LOCK(&ltc_cipher_mutex);
    for (x=0; x<TAB_SIZE; x++)
    {
        if ( cipher_descriptor[x].name != NULL && cipher_descriptor[x].ID == cipher->ID )
        {
            //LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
            if ( cipherid >= 0 && Ciphers[cipherid] == 0 )
                Ciphers[cipherid] = cipher;
            return(x);
        }
    }
    // find a blank spot
    for (x=0; x<TAB_SIZE; x++)
    {
        if ( cipher_descriptor[x].name == NULL )
        {
            memcpy(&cipher_descriptor[x],cipher,sizeof(struct ltc_cipher_descriptor));
            if ( cipherid >= 0 && Ciphers[cipherid] == 0 )
                Ciphers[cipherid] = cipher;
            //LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
            return(x);
        }
    }
    //LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
    return(-1);
}

int32_t find_cipher(const char *name)
{
    int32_t x;
    LTC_ARGCHK(name != NULL);
    //LTC_MUTEX_LOCK(&ltc_cipher_mutex);
    for (x=0; x<TAB_SIZE; x++)
    {
        if (cipher_descriptor[x].name != NULL && !XSTRCMP(cipher_descriptor[x].name, name))
        {
            //LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
            return x;
        }
    }
    //LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
    return -1;
}

int32_t myregister_hash(const struct ltc_hash_descriptor *hash)
{
    int32_t x;
    LTC_ARGCHK(hash != NULL);
    // is it already registered?
    //LTC_MUTEX_LOCK(&ltc_hash_mutex);
    for (x=0; x<TAB_SIZE; x++)
    {
        if ( XMEMCMP(&hash_descriptor[x],hash,sizeof(struct ltc_hash_descriptor)) == 0 )
        {
            //LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
            return x;
        }
    }
    // find a blank spot
    for (x=0; x<TAB_SIZE; x++)
    {
        if ( hash_descriptor[x].name == NULL )
        {
            memcpy(&hash_descriptor[x],hash,sizeof(struct ltc_hash_descriptor));
            //LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
            return(x);
        }
    }
    // no spot
    //LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
    return(-1);
}

int32_t find_hash(const char *name)
{
    int32_t x;
    LTC_ARGCHK(name != NULL);
    //LTC_MUTEX_LOCK(&ltc_hash_mutex);
    for (x=0; x<TAB_SIZE; x++)
    {
        if (hash_descriptor[x].name != NULL && XSTRCMP(hash_descriptor[x].name, name) == 0)
        {
            //LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
            return x;
        }
    }
    //LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
    return -1;
}

/**
 Register a PRNG with the descriptor table
 @param prng   The PRNG you wish to register
 @return value >= 0 if successfully added (or already present), -1 if unsuccessful
 */
int32_t myregister_prng(const struct ltc_prng_descriptor *prng)
{
    int32_t x;
    LTC_ARGCHK(prng != NULL);
    // is it already registered?
    //LTC_MUTEX_LOCK(&ltc_prng_mutex);
    for (x=0; x<TAB_SIZE; x++)
    {
        if ( XMEMCMP(&prng_descriptor[x],prng,sizeof(struct ltc_prng_descriptor)) == 0 )
        {
            //LTC_MUTEX_UNLOCK(&ltc_prng_mutex);
            return x;
        }
    }
    /* find a blank spot */
    for (x=0; x<TAB_SIZE; x++)
    {
        if ( prng_descriptor[x].name == NULL )
        {
            XMEMCPY(&prng_descriptor[x],prng,sizeof(struct ltc_prng_descriptor));
            //LTC_MUTEX_UNLOCK(&ltc_prng_mutex);
            return x;
        }
    }
    //LTC_MUTEX_UNLOCK(&ltc_prng_mutex);
    return -1;
}

// portable way to get secure random bits to feed a PRNG  (Tom St Denis)
/**
 Create a PRNG from a RNG
 @param bits     Number of bits of entropy desired (64 ... 1024)
 @param wprng    Index of which PRNG to setup
 @param prng     [out] PRNG state to initialize
 @param callback A pointer to a void function for when the RNG is slow, this can be NULL
 @return CRYPT_OK if successful
 */
int32_t rng_make_prng(int32_t bits,int32_t wprng,prng_state *prng,void (*callback)(void))
{
    unsigned char buf[256];
    int32_t err;
    LTC_ARGCHK( prng != NULL );
    if ( (err= prng_is_valid(wprng)) != CRYPT_OK )
        return(err);
    if ( bits < 64 || bits > 1024 )
        return(CRYPT_INVALID_PRNGSIZE);
    if ( (err= prng_descriptor[wprng].start(prng)) != CRYPT_OK )
        return(err);
    bits = ((bits/8)+((bits&7)!=0?1:0)) * 2;
    if ( rng_get_bytes(buf,(unsigned long)bits,callback) != (unsigned long)bits )
        return CRYPT_ERROR_READPRNG;
    if ( (err= prng_descriptor[wprng].add_entropy(buf,(unsigned long)bits,prng)) != CRYPT_OK )
        return(err);
    if ( (err= prng_descriptor[wprng].ready(prng)) != CRYPT_OK )
        return(err);
#ifdef LTC_CLEAN_STACK
    zeromem(buf,sizeof(buf));
#endif
    return(CRYPT_OK);
}

#ifdef LTC_DEVRANDOM
/* on *NIX read /dev/random */
static unsigned long rng_nix(unsigned char *buf, unsigned long len,void (*callback)(void))
{
#ifdef LTC_NO_FILE
    return 0;
#else
    FILE *f;
    unsigned long x;
#ifdef TRY_URANDOM_FIRST
    f = fopen("/dev/urandom", "rb");
    if (f == NULL)
#endif /* TRY_URANDOM_FIRST */
        f = fopen("/dev/random", "rb");
    
    if (f == NULL) {
        return 0;
    }
    
    /* disable buffering */
    if (setvbuf(f, NULL, _IONBF, 0) != 0) {
        fclose(f);
        return 0;
    }
    
    x = (unsigned long)fread(buf, 1, (size_t)len, f);
    fclose(f);
    return x;
#endif /* LTC_NO_FILE */
}

#endif /* LTC_DEVRANDOM */

/* on ANSI C platforms with 100 < CLOCKS_PER_SEC < 10000 */
#if defined(CLOCKS_PER_SEC) && !defined(WINCE)

#define ANSI_RNG

static unsigned long rng_ansic(unsigned char *buf, unsigned long len,
                               void (*callback)(void))
{
    clock_t t1;
    int l, acc, bits, a, b;
    
    if (XCLOCKS_PER_SEC < 100 || XCLOCKS_PER_SEC > 10000) {
        return 0;
    }
    
    l = (int)len;
    bits = 8;
    acc  = a = b = 0;
    while (len--) {
        if (callback != NULL) callback();
        while (bits--) {
            do {
                t1 = XCLOCK(); while (t1 == XCLOCK()) a ^= 1;
                t1 = XCLOCK(); while (t1 == XCLOCK()) b ^= 1;
            } while (a == b);
            acc = (acc << 1) | a;
        }
        *buf++ = acc;
        acc  = 0;
        bits = 8;
    }
    acc = bits = a = b = 0;
    return l;
}

#endif

/* Try the Microsoft CSP */
#if defined(WIN32) || defined(WINCE)
#define _WIN32_WINNT 0x0400
#ifdef WINCE
#define UNDER_CE
#define ARM
#endif
#include <windows.h>
#include <wincrypt.h>

static unsigned long rng_win32(unsigned char *buf, unsigned long len,
                               void (*callback)(void))
{
    HCRYPTPROV hProv = 0;
    if (!CryptAcquireContext(&hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL,
                             (CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET)) &&
        !CryptAcquireContext (&hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL,
                              CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET | CRYPT_NEWKEYSET))
        return 0;
    
    if (CryptGenRandom(hProv, len, buf) == TRUE) {
        CryptReleaseContext(hProv, 0);
        return len;
    } else {
        CryptReleaseContext(hProv, 0);
        return 0;
    }
}

#endif /* WIN32 */
/**
 Read the system RNG
 @param out       Destination
 @param outlen    Length desired (octets)
 @param callback  Pointer to void function to act as "callback" when RNG is slow.  This can be NULL
 @return Number of octets read
 */
unsigned long rng_get_bytes(unsigned char *out, unsigned long outlen,void (*callback)(void))
{
    unsigned long x;
    LTC_ARGCHK(out != NULL);
#if defined(LTC_DEVRANDOM)
    x = rng_nix(out, outlen, callback);   if (x != 0) { return x; }
#endif
#ifdef WIN32
    x = rng_win32(out, outlen, callback); if (x != 0) { return x; }
#endif
#ifdef ANSI_RNG
    x = rng_ansic(out, outlen, callback); if (x != 0) { return x; }
#endif
    return 0;
}

int32_t find_prng(const char *name)
{
    int32_t x;
    LTC_ARGCHK(name != NULL);
    //LTC_MUTEX_LOCK(&ltc_prng_mutex);
    for (x=0; x<TAB_SIZE; x++)
    {
        if ( prng_descriptor[x].name != NULL && XSTRCMP(prng_descriptor[x].name, name) == 0 )
        {
            //LTC_MUTEX_UNLOCK(&ltc_prng_mutex);
            return x;
        }
    }
    //LTC_MUTEX_UNLOCK(&ltc_prng_mutex);
    return -1;
}

int32_t prng_is_valid(int32_t idx)
{
    //LTC_MUTEX_LOCK(&ltc_prng_mutex);
    if ( idx < 0 || idx >= TAB_SIZE || prng_descriptor[idx].name == NULL )
    {
        //LTC_MUTEX_UNLOCK(&ltc_prng_mutex);
        return CRYPT_INVALID_PRNG;
    }
    //LTC_MUTEX_UNLOCK(&ltc_prng_mutex);
    return CRYPT_OK;
}

int32_t register_ciphers()
{
    int32_t i,n = 0;
    myregister_cipher(n++,&aes_enc_desc);
    myregister_cipher(n++,&blowfish_desc);
    myregister_cipher(n++,&xtea_desc);
    myregister_cipher(n++,&rc5_desc);
    myregister_cipher(n++,&rc6_desc);
    myregister_cipher(n++,&saferp_desc);
    myregister_cipher(n++,&twofish_desc);
    myregister_cipher(n++,&safer_k64_desc);
    myregister_cipher(n++,&safer_sk64_desc);
    myregister_cipher(n++,&safer_k128_desc);
    myregister_cipher(n++,&safer_sk128_desc);
    myregister_cipher(n++,&rc2_desc);
    //myregister_cipher(n++,&des_desc);
    myregister_cipher(n++,&des3_desc);
    myregister_cipher(n++,&cast5_desc);
    myregister_cipher(n++,&noekeon_desc);
    myregister_cipher(n++,&skipjack_desc);
    myregister_cipher(n++,&khazad_desc);
    myregister_cipher(n++,&anubis_desc);
    myregister_cipher(n++,&rijndael_enc_desc);
    for (i=0; i<n; i++)
        if ( Ciphers[i] == 0 )
        {
            fprintf(stderr,"FATAL error Cipher[%d] not found??\n",i);
            exit(-2);
        }
    myregister_hash(&sha256_desc);
    myregister_hash(&rmd160_desc);
    myregister_prng(&yarrow_desc);
    return(n);
}

int32_t hash_is_valid(int32_t idx)
{
    //LTC_MUTEX_LOCK(&ltc_hash_mutex);
    if ( idx < 0 || idx >= TAB_SIZE || hash_descriptor[idx].name == NULL )
    {
        //LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
        return CRYPT_INVALID_HASH;
    }
    //LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
    return CRYPT_OK;
}

int32_t cipher_is_valid(int32_t idx)
{
    //LTC_MUTEX_LOCK(&ltc_cipher_mutex);
    if ( idx < 0 || idx >= TAB_SIZE || cipher_descriptor[idx].name == NULL )
    {
        //LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
        return CRYPT_INVALID_CIPHER;
    }
    //LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
    return CRYPT_OK;
}

/**
 Hash a block of memory and store the digest.
 @param hash   The index of the hash you wish to use
 @param in     The data you wish to hash
 @param inlen  The length of the data to hash (octets)
 @param out    [out] Where to store the digest
 @param outlen [in/out] Max size and resulting size of the digest
 @return CRYPT_OK if successful
 */
int32_t hash_memory(int32_t hash, const unsigned char *in, unsigned long inlen, unsigned char *out, unsigned long *outlen)
{
    hash_state *md;
    int32_t err;
    LTC_ARGCHK(in     != NULL);
    LTC_ARGCHK(out    != NULL);
    LTC_ARGCHK(outlen != NULL);
    if ( (err= hash_is_valid(hash)) != CRYPT_OK )
        return err;
    if ( *outlen < hash_descriptor[hash].hashsize )
    {
        *outlen = hash_descriptor[hash].hashsize;
        return CRYPT_BUFFER_OVERFLOW;
    }
    md = XMALLOC(sizeof(hash_state));
    if (md == NULL)
        return CRYPT_MEM;
    if ( (err= hash_descriptor[hash].init(md)) != CRYPT_OK )
        goto LBL_ERR;
    if ( (err= hash_descriptor[hash].process(md,in,inlen)) != CRYPT_OK )
        goto LBL_ERR;
    err = hash_descriptor[hash].done(md,out);
    *outlen = hash_descriptor[hash].hashsize;
LBL_ERR:
#ifdef LTC_CLEAN_STACK
    zeromem(md, sizeof(hash_state));
#endif
    XFREE(md);
    return err;
}

/**
 Initialize a CTR context
 @param cipher      The index of the cipher desired
 @param IV          The initial vector
 @param key         The secret key
 @param keylen      The length of the secret key (octets)
 @param num_rounds  Number of rounds in the cipher desired (0 for default)
 @param ctr_mode    The counter mode (CTR_COUNTER_LITTLE_ENDIAN or CTR_COUNTER_BIG_ENDIAN)
 @param ctr         The CTR state to initialize
 @return CRYPT_OK if successful
 */
int32_t ctr_start(int32_t cipher,const unsigned char *IV,const unsigned char *key,int32_t keylen,int32_t  num_rounds,int32_t ctr_mode,symmetric_CTR *ctr)
{
    int x, err;
    LTC_ARGCHK(IV  != NULL);
    LTC_ARGCHK(key != NULL);
    LTC_ARGCHK(ctr != NULL);
    // bad param?
    if ((err = cipher_is_valid(cipher)) != CRYPT_OK)
        return err;
    // ctrlen == counter width
    ctr->ctrlen   = (ctr_mode & 255) ? (ctr_mode & 255) : cipher_descriptor[cipher].block_length;
    if (ctr->ctrlen > cipher_descriptor[cipher].block_length)
        return CRYPT_INVALID_ARG;
    if ((ctr_mode & 0x1000) == CTR_COUNTER_BIG_ENDIAN)
        ctr->ctrlen = cipher_descriptor[cipher].block_length - ctr->ctrlen;
    // setup cipher
    if ((err = cipher_descriptor[cipher].setup(key, keylen, num_rounds, &ctr->key)) != CRYPT_OK)
        return err;
    // copy ctr
    ctr->blocklen = cipher_descriptor[cipher].block_length;
    ctr->cipher   = cipher;
    ctr->padlen   = 0;
    ctr->mode     = ctr_mode & 0x1000;
    for (x = 0; x < ctr->blocklen; x++)
        ctr->ctr[x] = IV[x];
    if (ctr_mode & LTC_CTR_RFC3686)
    {
        // increment the IV as per RFC 3686
        if (ctr->mode == CTR_COUNTER_LITTLE_ENDIAN)
        {
            // little-endian
            for (x = 0; x < ctr->ctrlen; x++)
            {
                ctr->ctr[x] = (ctr->ctr[x] + (unsigned char)1) & (unsigned char)255;
                if (ctr->ctr[x] != (unsigned char)0)
                    break;
            }
        }
        else
        {
            // big-endian
            for (x = ctr->blocklen-1; x >= ctr->ctrlen; x--)
            {
                ctr->ctr[x] = (ctr->ctr[x] + (unsigned char)1) & (unsigned char)255;
                if (ctr->ctr[x] != (unsigned char)0)
                    break;
            }
        }
    }
    return cipher_descriptor[ctr->cipher].ecb_encrypt(ctr->ctr, ctr->pad, &ctr->key);
}

/**
 CTR encrypt
 @param pt     Plaintext
 @param ct     [out] Ciphertext
 @param len    Length of plaintext (octets)
 @param ctr    CTR state
 @return CRYPT_OK if successful
 */
int32_t ctr_encrypt(const unsigned char *pt,unsigned char *ct,unsigned long len,symmetric_CTR *ctr)
{
    int32_t x, err;
    LTC_ARGCHK(pt != NULL);
    LTC_ARGCHK(ct != NULL);
    LTC_ARGCHK(ctr != NULL);
    if ((err = cipher_is_valid(ctr->cipher)) != CRYPT_OK)
        return err;
    // is blocklen/padlen valid?
    if ( ctr->blocklen < 1 || ctr->blocklen > (int)sizeof(ctr->ctr) || ctr->padlen   < 0 || ctr->padlen > (int32_t)sizeof(ctr->pad))
        return CRYPT_INVALID_ARG;
#ifdef LTC_FAST
    if ( ctr->blocklen % sizeof(LTC_FAST_TYPE) )
        return CRYPT_INVALID_ARG;
#endif
    // handle acceleration only if pad is empty, accelerator is present and length is >= a block size
    if ( (ctr->padlen == ctr->blocklen) && cipher_descriptor[ctr->cipher].accel_ctr_encrypt != NULL && (len >= (unsigned long)ctr->blocklen) )
    {
        if ( (err = cipher_descriptor[ctr->cipher].accel_ctr_encrypt(pt,ct,len/ctr->blocklen,ctr->ctr, ctr->mode, &ctr->key)) != CRYPT_OK )
            return err;
        len %= ctr->blocklen;
    }
    while ( len != 0 )
    {
        // is the pad empty?
        if ( ctr->padlen == ctr->blocklen )
        {
            // increment counter
            if ( ctr->mode == CTR_COUNTER_LITTLE_ENDIAN )
            {
                // little-endian
                for (x = 0; x < ctr->ctrlen; x++)
                {
                    ctr->ctr[x] = (ctr->ctr[x] + (unsigned char)1) & (unsigned char)255;
                    if (ctr->ctr[x] != (unsigned char)0)
                        break;
                }
            }
            else
            {
                // big-endian
                for (x = ctr->blocklen-1; x >= ctr->ctrlen; x--)
                {
                    ctr->ctr[x] = (ctr->ctr[x] + (unsigned char)1) & (unsigned char)255;
                    if (ctr->ctr[x] != (unsigned char)0)
                        break;
                }
            }
            // encrypt it
            if ( (err= cipher_descriptor[ctr->cipher].ecb_encrypt(ctr->ctr, ctr->pad, &ctr->key)) != CRYPT_OK )
                return err;
            ctr->padlen = 0;
        }
#ifdef LTC_FAST
        if ( ctr->padlen == 0 && len >= (unsigned long)ctr->blocklen )
        {
            for (x = 0; x < ctr->blocklen; x += sizeof(LTC_FAST_TYPE))
            {
                *((LTC_FAST_TYPE*)((unsigned char *)ct + x)) = *((LTC_FAST_TYPE*)((unsigned char *)pt + x)) ^
                *((LTC_FAST_TYPE*)((unsigned char *)ctr->pad + x));
            }
            pt         += ctr->blocklen;
            ct         += ctr->blocklen;
            len        -= ctr->blocklen;
            ctr->padlen = ctr->blocklen;
            continue;
        }
#endif    
        *ct++ = *pt++ ^ ctr->pad[ctr->padlen++];
        --len;
    }
    return CRYPT_OK;
}

int32_t ctr_decrypt(const unsigned char *ct,unsigned char *pt,unsigned long len,symmetric_CTR *ctr)
{
    LTC_ARGCHK(pt != NULL);
    LTC_ARGCHK(ct != NULL);
    LTC_ARGCHK(ctr != NULL);
    return ctr_encrypt(ct,pt,len,ctr);
}

// Terminate the chain @param ctr    The CTR chain to terminate @return CRYPT_OK on success
int32_t ctr_done(symmetric_CTR *ctr)
{
    int32_t err;
    LTC_ARGCHK(ctr != NULL);
    if ( (err= cipher_is_valid(ctr->cipher) ) != CRYPT_OK )
        return(err);
    cipher_descriptor[ctr->cipher].done(&ctr->key);
    return(CRYPT_OK);
}

static const char *err_2_str[] =
{
    "CRYPT_OK",
    "CRYPT_ERROR",
    "Non-fatal 'no-operation' requested.",
    
    "Invalid keysize for block cipher.",
    "Invalid number of rounds for block cipher.",
    "Algorithm failed test vectors.",
    
    "Buffer overflow.",
    "Invalid input packet.",
    
    "Invalid number of bits for a PRNG.",
    "Error reading the PRNG.",
    
    "Invalid cipher specified.",
    "Invalid hash specified.",
    "Invalid PRNG specified.",
    
    "Out of memory.",
    
    "Invalid PK key or key type specified for function.",
    "A private PK key is required.",
    
    "Invalid argument provided.",
    "File Not Found",
    
    "Invalid PK type.",
    "Invalid PK system.",
    "Duplicate PK key found on keyring.",
    "Key not found in keyring.",
    "Invalid sized parameter.",
    
    "Invalid size for prime.",
    
};
const char *error_to_string(int32_t err)
{
    if (err < 0 || err >= (int32_t)(sizeof(err_2_str)/sizeof(err_2_str[0])))
        return "Invalid error code.";
    else return err_2_str[err];
}

uint8_t *decrypt_cipher(int32_t cipher_idx,uint8_t *key,int32_t ks,int32_t ivsize,uint8_t *data,int32_t *lenp)
{
    int32_t len = *lenp;
    symmetric_CTR ctr;
    uint8_t IV[MAXBLOCKSIZE],*plaintext;
    memcpy(IV,data,ivsize); // Need to read in IV
    data += ivsize;
    len -= ivsize;
    if ( (errno= ctr_start(cipher_idx,IV,key,ks,0,CTR_COUNTER_LITTLE_ENDIAN,&ctr)) != CRYPT_OK)
    {
        printf("ctr_start error: %s\n",error_to_string(errno));
        return(0);
    }
    *lenp = len;
    if ( len <= 0 )
        return(0);
    plaintext = calloc(1,len);
    if ( (errno= ctr_decrypt(data,plaintext,len,&ctr)) != CRYPT_OK )
    {
        printf("decrypt_cipher error: %s\n",error_to_string(errno));
        free(plaintext);
        return(0);
    }
    return(plaintext);
}

uint8_t *encrypt_cipher(int32_t cipher_idx,uint8_t *key,int32_t ks,int32_t ivsize,uint8_t *data,int32_t *lenp)
{
    uint8_t IV[MAXBLOCKSIZE],*encoded;
    int32_t len = *lenp;
    prng_state prng;
    symmetric_CTR ctr;
    // Setup yarrow for random bytes for IV
    *lenp = 0;
    if ( (errno= rng_make_prng(128,find_prng("yarrow"),&prng,NULL) ) != CRYPT_OK )
    {
        printf("ciphers_codec: Error setting up PRNG, %s\n", error_to_string(errno));
        return(0);
    }
    if ( yarrow_read(IV,ivsize,&prng) != ivsize )
    {
        printf("Error reading PRNG for IV required.\n");
        return(0);
    }
    encoded = calloc(1,len + ivsize);
    memcpy(encoded,IV,ivsize);
    if ( (errno= ctr_start(cipher_idx,IV,key,ks,0,CTR_COUNTER_LITTLE_ENDIAN,&ctr)) != CRYPT_OK)
    {
        printf("encrypt_cipher ctr_start error: %s\n",error_to_string(errno));
        free(encoded);
        return(0);
    }
    if ( (errno= ctr_encrypt(data,encoded+ivsize,len,&ctr)) != CRYPT_OK )
    {
        printf("encrypt_cipher ctr_encrypt error: %s\n",error_to_string(errno));
        free(encoded);
        return(0);
    }
    *lenp = len + ivsize;
    //printf("encrypt cipher.%p\n",encoded);
    return(encoded);
}

uint8_t *ciphers_codec(int32_t decrypt,char **privkeys,int32_t *cipherids,uint8_t *data,int32_t *lenp)
{
    static int32_t hash_idx = -1;
    uint8_t key[MAXBLOCKSIZE],*tmpptr = 0,*ptr = 0;
    unsigned long outlen,ivsize;
    int32_t cipher_idx,ks,i,j,n,origlen,len = *lenp;
    if ( hash_idx < 0 )
        hash_idx = find_hash("sha256");
    if ( hash_idx < 0 )
    {
        printf("ciphers_codec: cant find sha256???\n");
        return(0);
    }
    *lenp = 0;
    for (n=0; privkeys[n]!=0; n++)
        ;
    for (j=0; j<n; j++)
    {
        i = (decrypt != 0) ? (n-1-j) : j;
        cipher_idx = (cipherids[i] % NUM_CIPHERS);
        ivsize = cipher_descriptor[cipher_idx].block_length;
        ks = (int32_t)hash_descriptor[hash_idx].hashsize;
        if ( cipher_descriptor[cipher_idx].keysize(&ks) != CRYPT_OK )
        {
            printf("ciphers_codec: Invalid keysize???\n");
            return(0);
        }
        outlen = sizeof(key);
        if ( (errno= hash_memory(hash_idx,(uint8_t *)privkeys[i],strlen(privkeys[i]),key,&outlen)) != CRYPT_OK )
        {
            printf("ciphers_codec: Error hashing key: %s\n",error_to_string(errno));
            return(0);
        }
        origlen = len;
        if ( decrypt != 0 )
            ptr = decrypt_cipher(cipher_idx,key,ks,(int32_t)ivsize,data,&len);
        else
        {
            ptr = encrypt_cipher(cipher_idx,key,ks,(int32_t)ivsize,data,&len);
            if ( 0 )
            {
                int32_t tmplen = len;
                uint8_t *tmp;
                tmp = decrypt_cipher(cipher_idx,key,ks,(int32_t)ivsize,ptr,&tmplen);
                printf("i.%d %08x %08x len.%d tmplen.%d origlen.%d decrypt.%d\n",i,_crc32(0,tmp,tmplen),_crc32(0,data,origlen),len,tmplen,origlen,decrypt);
            }
        }
        if ( tmpptr != 0 )
            free(tmpptr);
        tmpptr = data = ptr;
    }
    if ( (*lenp= len) == 0 )
    {
        if ( tmpptr != 0 )
            free(tmpptr);
        return(0);
    }
    return(data);
}

void free_cipherptrs(cJSON *ciphersobj,char **privkeys,int32_t *cipherids)
{
    if ( privkeys != 0 )
        free(privkeys);
    if ( ciphersobj != 0 )
        free_json(ciphersobj);
    if ( cipherids != 0 )
        free(cipherids);
}

char **validate_ciphers(int32_t **cipheridsp,struct coin_info *cp,cJSON *ciphersobj)
{
    char *get_telepod_privkey(char **podaddrp,char *pubkey,struct coin_info *cp);
    static int32_t didinit;
    int32_t i,cipherid,n;
    cJSON *item,*obj;
    char keyaddr[128],pubkey[1024],*coinaddr,*privkey,**privkeys = 0;
    *cipheridsp = 0;
    if ( didinit == 0 )
    {
        register_ciphers();
        didinit = 1;
    }
    if ( is_cJSON_Array(ciphersobj) == 0 || (n= cJSON_GetArraySize(ciphersobj)) <= 0 )
        return(0);
    privkeys = calloc(n+1,sizeof(*privkeys));
    (*cipheridsp) = calloc(n+1,sizeof(*cipheridsp));
    //printf("set cipheridsp %p\n",(*cipheridsp));
    for (i=0; i<n; i++)
    {
        item = cJSON_GetArrayItem(ciphersobj,i);
        for (cipherid=0; cipherid<NUM_CIPHERS; cipherid++)
        {
            //printf("%s ",Cipherstrs[cipherid]);
            if ( (obj= cJSON_GetObjectItem(item,Cipherstrs[cipherid])) != 0 )
            {
                // printf("MATCH!\n");
                copy_cJSON(keyaddr,obj);
                if ( keyaddr[0] != 0 )
                {
                    coinaddr = keyaddr;
                    if ( (privkey= get_telepod_privkey(&coinaddr,pubkey,cp)) != 0 )
                    {
                        //printf("(%d (%s) %s).i%d ",cipherid,coinaddr,privkey,i);
                        privkeys[i] = privkey;
                        (*cipheridsp)[i] = cipherid;
                        break;
                    }
                    else
                    {
                        printf("cant get privkey for cipherid.%d %s.(%s)\n",cipherid,cp->name,coinaddr);
                        free(privkeys);
                        free(*cipheridsp);
                        (*cipheridsp) = 0;
                        return(0);
                    }
                }
                else
                {
                    printf("cant get address for cipherid.%d %s\n",cipherid,cp->name);
                    free(privkeys);
                    free(*cipheridsp);
                    (*cipheridsp) = 0;
                    return(0);
                }
            }
        }
        if ( cipherid == NUM_CIPHERS )
        {
            printf("cant find cipherid.%d for %s in array item.%d (%s)\n",cipherid,cp->name,i,cJSON_Print(ciphersobj));
            free(privkeys);
            free(*cipheridsp);
            (*cipheridsp) = 0;
            return(0);
        }
    }
    return(privkeys);
}

int32_t _save_encrypted(char *fname,uint8_t *encoded,int32_t len)
{
    struct coin_info *get_coin_info(char *coinstr);
    char *mofn_savefile(char *previpaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,char *pin,FILE *fp,int32_t L,int32_t M,int32_t N,char *usbdir,char *password,char *filename);
    FILE *fp;
    cJSON *json,*txidsobj;
    int M=2,N=3,result;
    char sharenrs[512],*retstr,*pin = "";
    struct coin_info *cp = get_coin_info("BTCD");
    if ( encoded != 0 )
    {
        if ( (fp= fopen(fname,"wb")) != 0 )
        {
            if ( fwrite(encoded,1,len,fp) != len )
            {
                printf("error saving.(%s) encrypted data %d\n",fname,len);
                strcpy(fname,"error");
                fclose(fp);
                return(-1);
            }
            else
            {
                fclose(fp);
                if ( (fp= fopen(fname,"rb")) != 0 )
                {
                    retstr = mofn_savefile(0,cp->srvNXTADDR,cp->srvNXTACCTSECRET,cp->srvNXTADDR,pin,fp,0,M,N,0,cp->privateNXTACCTSECRET,fname);
                    if ( retstr != 0 )
                    {
                        if ( (json= cJSON_Parse(retstr)) != 0 )
                        {
                            if ( (result= get_API_int(cJSON_GetObjectItem(json,"result"),-1)) == 0 )
                            {
                                copy_cJSON(sharenrs,cJSON_GetObjectItem(json,"sharenrs"));
                                txidsobj = cJSON_GetObjectItem(json,"txids");
                                if ( is_cJSON_Array(txidsobj) != 0 && cJSON_GetArraySize(txidsobj) == N )
                                {
                                    // extract N uint64_t, save with sharenrs in binary file, keyed to filename
                                } else printf("ERROR: unexpected array size.%d vs N.%d (%s)\n",cJSON_GetArraySize(txidsobj),N,retstr);
                            } else printf("ERROR: result.%d from (%s)\n",result,retstr);
                            free_json(json);
                        }
                        printf("save.(%s) -> (%s)\n",fname,retstr);
                        free(retstr);
                    }
                    fclose(fp);
                }
                return(len);
            }
        }
    }
    return(0);
}

uint8_t *save_encrypted(char *fname,struct coin_info *cp,uint8_t *data,int32_t *lenp)
{
    //FILE *fp;
    uint8_t *encoded = 0;
    char **privkeys;
    int32_t *cipherids,newlen = *lenp;
    if ( (privkeys= validate_ciphers(&cipherids,cp,cp->ciphersobj)) != 0 )
    {
        encoded = ciphers_codec(0,privkeys,cipherids,data,&newlen);
        free_cipherptrs(0,privkeys,cipherids);
        *lenp = _save_encrypted(fname,encoded,newlen);
    }
    return(encoded);
}

uint8_t *load_encrypted(int32_t *lenp,char *fname,struct coin_info *cp)
{
    FILE *fp;
    long fsize;
    uint8_t *encrypted=0,*decoded = 0;
    char **privkeys;
    int32_t *cipherids,newlen;
    *lenp = 0;
    //printf("load_encrypted.(%s)\n",fname);
    if ( (fp= fopen(fname,"rb")) != 0 )
    {
        fseek(fp,0,SEEK_END);
        fsize = ftell(fp);
        rewind(fp);
        encrypted = calloc(1,fsize);
        if ( fread(encrypted,1,fsize,fp) != fsize )
            printf("error loading.(%s) encrypted data len.%ld\n",fname,fsize);
        else
        {
            if ( (privkeys= validate_ciphers(&cipherids,cp,cp->ciphersobj)) != 0 )
            {
                newlen = (int32_t)fsize;
                decoded = ciphers_codec(1,privkeys,cipherids,encrypted,&newlen);
                //printf("free_cipherptrs %p %p decoded.%p\n",privkeys,cipherids,decoded);
                free_cipherptrs(0,privkeys,cipherids);
                if ( newlen != 0 )
                    *lenp = newlen;
                else if ( decoded != 0 )
                    free(decoded);
            }
        }
        fclose(fp);
        //printf("free encrypted.%p\n",encrypted);
        free(encrypted);
    }
    //printf("encrypted.%p fp.%p decoded.%p\n",encrypted,fp,decoded);
    return(decoded);
}

int test_ciphers_codec(int maxlen,int maxiters)
{
    struct coin_info *get_coin_info(char *);
    struct coin_info *cp;
    int i,j,len,encryptlen,decryptlen,err,errs = 0;
    uint8_t *data,*encrypted,*decrypted;
    cp = get_coin_info("BTCD");
    if ( cp == 0 )
        fatal("cant get BTCD info");
    printf("cp.%p\n",cp);
    for (i=0; i<maxiters; i++)
    {
        len = ((rand()>>8) % maxlen) + 16;
        data = malloc(len);
        randombytes(data,len);
        err = 1;
        encryptlen = len, encrypted = save_encrypted("/tmp/test",cp,data,&encryptlen);
        if ( encrypted != 0 )
        {
            decryptlen = encryptlen, decrypted = load_encrypted(&decryptlen,"/tmp/test",cp);
            if ( decrypted != 0 )
            {
                if ( decryptlen != len )
                    err++, printf("decryptlen %d != %d\n",decryptlen,len);
                else if ( memcmp(decrypted,data,len) != 0 )
                {
                    for (j=0; j<len; j++)
                    {
                        printf("%02x ",data[j]);
                        if ( data[j] != encrypted[j] )
                            break;
                    }
                    printf("data\n");
                    for (j=0; j<len; j++)
                    {
                        printf("%02x ",encrypted[j]);
                        if ( data[j] != encrypted[j] )
                            break;
                    }
                    printf("decrypted %d of %d\n",j,len);
                    err++, printf("miscompare iter.%d len.%d\n",i,len);
                }
                else fprintf(stderr,"+"), err = 0;
                free(decrypted);
            }
            free(encrypted);
        }
        free(data);
        errs += err;
    }
    return(errs);
}
#endif
