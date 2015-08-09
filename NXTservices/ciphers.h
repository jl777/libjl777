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

char *Cipherstrs[] =
{
    "aes","blowfish","xtea","rc5","rc6","saferp","twofish","safer_k64","safer_sk64","safer_k128",
    "safer_sk128","rc2","des3","cast5","noekeon","skipjack","khazad","anubis"
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
            Ciphers[cipherid] = cipher;
            //LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
            return(x);
        }
    }
    //LTC_MUTEX_UNLOCK(&ltc_cipher_mutex);
    return(-1);
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

int32_t register_ciphers()
{
    int32_t i,n = 0;
    myregister_cipher(n++,&aes_desc);
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
    for (i=0; i<n; i++)
        if ( Ciphers[i] == 0 )
        {
            printf("FATAL error Cipher[%d] not found??\n",i);
            exit(-2);
        }
    myregister_hash(&sha256_desc);
    myregister_hash(&rmd160_desc);
    return(n);
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
    privkeys = calloc(n,sizeof(*privkeys));
    (*cipheridsp) = calloc(n,sizeof(*cipheridsp));
    printf("set cipheridsp %p\n",(*cipheridsp));
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
                        printf("(%d (%s) %s).i%d ",cipherid,coinaddr,privkey,i);
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

/*
 int main(int argc, char *argv[])
 {
 unsigned char plaintext[512],ciphertext[512];
 unsigned char tmpkey[512], key[MAXBLOCKSIZE], IV[MAXBLOCKSIZE];
 unsigned char inbuf[512]; // i/o block size
 unsigned long outlen, y, ivsize, x, decrypt;
 symmetric_CTR ctr;
 int cipher_idx, hash_idx, ks;
 char *infile, *outfile, *cipher;
 prng_state prng;
 FILE *fdin, *fdout;
 
 // register algs, so they can be printed
 register_algs();
 
 if (argc < 4) {
 return usage(argv[0]);
 }
 
 if (!strcmp(argv[1], "-d")) {
 decrypt = 1;
 cipher  = argv[2];
 infile  = argv[3];
 outfile = argv[4];
 } else {
 decrypt = 0;
 cipher  = argv[1];
 infile  = argv[2];
 outfile = argv[3];
 }
 
 // file handles setup
 fdin = fopen(infile,"rb");
 if (fdin == NULL) {
 perror("Can't open input for reading");
 exit(-1);
 }
 
 fdout = fopen(outfile,"wb");
 if (fdout == NULL) {
 perror("Can't open output for writing");
 exit(-1);
 }
 
 cipher_idx = find_cipher(cipher);
 if (cipher_idx == -1) {
 printf("Invalid cipher entered on command line.\n");
 exit(-1);
 }
 
 hash_idx = find_hash("sha256");
 if (hash_idx == -1) {
 printf("LTC_SHA256 not found...?\n");
 exit(-1);
 }
 
 ivsize = cipher_descriptor[cipher_idx].block_length;
 ks = hash_descriptor[hash_idx].hashsize;
 if (cipher_descriptor[cipher_idx].keysize(&ks) != CRYPT_OK) {
 printf("Invalid keysize???\n");
 exit(-1);
 }
 
 printf("\nEnter key: ");
 fgets((char *)tmpkey,sizeof(tmpkey), stdin);
 outlen = sizeof(key);
 if ((errno = hash_memory(hash_idx,tmpkey,strlen((char *)tmpkey),key,&outlen)) != CRYPT_OK) {
 printf("Error hashing key: %s\n", error_to_string(errno));
 exit(-1);
 }
 
 if (decrypt) {
 // Need to read in IV
 if (fread(IV,1,ivsize,fdin) != ivsize) {
 printf("Error reading IV from input.\n");
 exit(-1);
 }
 
 if ((errno = ctr_start(cipher_idx,IV,key,ks,0,CTR_COUNTER_LITTLE_ENDIAN,&ctr)) != CRYPT_OK) {
 printf("ctr_start error: %s\n",error_to_string(errno));
 exit(-1);
 }
 
 // IV done
 do {
 y = fread(inbuf,1,sizeof(inbuf),fdin);
 
 if ((errno = ctr_decrypt(inbuf,plaintext,y,&ctr)) != CRYPT_OK) {
 printf("ctr_decrypt error: %s\n", error_to_string(errno));
 exit(-1);
 }
 
 if (fwrite(plaintext,1,y,fdout) != y) {
 printf("Error writing to file.\n");
 exit(-1);
 }
 } while (y == sizeof(inbuf));
 fclose(fdin);
 fclose(fdout);
 
 } else {  //encrypt
 // Setup yarrow for random bytes for IV
 
 if ((errno = rng_make_prng(128, find_prng("yarrow"), &prng, NULL)) != CRYPT_OK) {
 printf("Error setting up PRNG, %s\n", error_to_string(errno));
 }
 
 // You can use rng_get_bytes on platforms that support it
 // x = rng_get_bytes(IV,ivsize,NULL);
 x = yarrow_read(IV,ivsize,&prng);
 if (x != ivsize) {
 printf("Error reading PRNG for IV required.\n");
 exit(-1);
 }
 
 if (fwrite(IV,1,ivsize,fdout) != ivsize) {
 printf("Error writing IV to output.\n");
 exit(-1);
 }
 
 if ((errno = ctr_start(cipher_idx,IV,key,ks,0,CTR_COUNTER_LITTLE_ENDIAN,&ctr)) != CRYPT_OK) {
 printf("ctr_start error: %s\n",error_to_string(errno));
 exit(-1);
 }
 
 do {
 y = fread(inbuf,1,sizeof(inbuf),fdin);
 
 if ((errno = ctr_encrypt(inbuf,ciphertext,y,&ctr)) != CRYPT_OK) {
 printf("ctr_encrypt error: %s\n", error_to_string(errno));
 exit(-1);
 }
 
 if (fwrite(ciphertext,1,y,fdout) != y) {
 printf("Error writing to output.\n");
 exit(-1);
 }
 } while (y == sizeof(inbuf));
 fclose(fdout);
 fclose(fdin);
 }
 return 0;
 }*/
#endif
