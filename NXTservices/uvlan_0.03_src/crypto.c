#include "uvlan.h"

static pthread_mutex_t prng_lock = PTHREAD_MUTEX_INITIALIZER;
static int             aes_idx;

static void crypto_err(const char *func, int err)
{
   fprintf(stderr, "%s: %s\n", func, error_to_string(err));
   exit(EXIT_FAILURE);
}

void crypto_init(void)
{
   int err;
   
   register_cipher(&aes_desc);
   register_hash(&sha256_desc);
   register_prng(&yarrow_desc);

   aes_idx = find_cipher("aes");
    
   pthread_mutex_lock(&prng_lock);
     if ((err = rng_make_prng(256, find_prng("yarrow"), &uvlan_prng, NULL)) != CRYPT_OK) {
        crypto_err("crypto_init", err);
     }
   pthread_mutex_unlock(&prng_lock);
   
}

void crypto_init_port(uvlan_port *port, const unsigned char *masterkey, int keylen)
{
   int err;
   
   pthread_mutex_lock(&port->lock);

   keylen                = MIN((int)sizeof(port->master_ukey), keylen);
   memcpy(port->master_ukey, masterkey, keylen);
   port->master_ukey_len = keylen;
   
   if ((err = aes_setup(masterkey, keylen, 0, &port->master_key)) != CRYPT_OK) {
      crypto_err("crypto_init_port", err);
   }
   
   pthread_mutex_lock(&prng_lock);
      // randomize the seeds 
      if (yarrow_read(port->loc_seed, 16, &uvlan_prng) != 16) {
         crypto_err("crypto_init_port", -1);
      }
      if (yarrow_read(port->rem_seed, 16, &uvlan_prng) != 16) {
         crypto_err("crypto_init_port", -1);
      }
   pthread_mutex_unlock(&prng_lock);

   aes_setup(port->loc_seed, 16, 0, &port->out_key);
   aes_setup(port->rem_seed, 16, 0, &port->in_key);

   pthread_mutex_unlock(&port->lock);
}

int crypto_make_send_seed(uvlan_port *port)
{
   unsigned char IV[12], buf[128];
   int           err, y;
   unsigned      long taglen;
   
   pthread_mutex_lock(&prng_lock);
      /* make a random IV and seed */
      if (yarrow_read(IV, 12, &uvlan_prng) != 12) {
         crypto_err("crypto_make_send_seed", -1);
      }
      if (yarrow_read(port->loc_seed, 16, &uvlan_prng) != 16) {
         crypto_err("crypto_make_send_seed", -1);
      }
   pthread_mutex_unlock(&prng_lock);
      
      /* encrypt it and form the output */
      buf[0] = UVLAN_TYPE_SEED;
      memcpy(buf+1, IV, 12);

      taglen = sizeof(buf) - 1 - 12 - 16; 
      if ((err = ccm_memory(aes_idx,
                            NULL, port->master_ukey_len,
                            &port->master_key,
                            IV, 12,
                            NULL, 0,
                            port->loc_seed, 16,
                            buf+1+12+16,
                            buf+1+12, &taglen,
                            CCM_ENCRYPT)) != CRYPT_OK) {
         crypto_err("crypto_make_send_seed", err);
      }
      
      /* send it */
      y = sendto(bound_sock, buf, 1 + 16 + 12 + 16, 0, (struct sockaddr *)&port->dest_sin, sizeof(port->dest_sin));
      if (y != (1+16+12+16)) {
         perror("sendto");
         return UVLAN_FAIL_SEND;
      }
   return UVLAN_READ_OK;
}

int crypto_read_seed(uvlan_port *port, const unsigned char *buf, int buflen)
{
   unsigned char tag[16];
   unsigned long taglen;
   int           err;     
   
   if (buflen != 1+12+16+16) {
      return UVLAN_FAIL_SIZE;
   }
   
      /* decrypt it */
      taglen = sizeof(tag); 
      if ((err = ccm_memory(aes_idx,
                            NULL, port->master_ukey_len,
                            &port->master_key,
                            buf+1, 12,
                            NULL, 0,
                            port->rem_seed, 16,
                            (unsigned char *)buf+1+12+16,
                            tag, &taglen,
                            CCM_DECRYPT)) != CRYPT_OK) {
         crypto_err("crypto_read_seed", err);
      }
      
      /* check tag */
      if (memcmp(tag, buf+1+12, 16)) {
         return UVLAN_FAIL_MAC;
      }
   return UVLAN_READ_OK;
}      
   
void crypto_make_key(uvlan_port *port)
{
   unsigned char key[16], tmp[16+16+12+12];
   unsigned long tmplen;
   int           x, err, side;
   
      /* replay prevention, if the keys are equal someone is reflecting!!! */
      if (!memcmp(port->rem_seed, port->loc_seed, 16)) {
         pthread_mutex_lock(&prng_lock);
         // randomize the seeds 
         if (yarrow_read(port->loc_seed, 16, &uvlan_prng) != 16) {
            crypto_err("crypto_init_port", -1);
         }
         if (yarrow_read(port->rem_seed, 16, &uvlan_prng) != 16) {
            crypto_err("crypto_init_port", -1);
         }
         pthread_mutex_unlock(&prng_lock);
         return;
      }
   
      /* mix keys */
      for (x = 0; x < 16; x++) {
         key[x] = port->rem_seed[x] ^ port->loc_seed[x];
      }
      
      /* now PKCS #5 stretch it to 16+16+12+12 bytes */
      tmplen = sizeof(tmp);
      if ((err = pkcs_5_alg2(key, 16, 
                             "uvlanrocks", 10,
                             8, find_hash("sha256"),
                             tmp, &tmplen)) != CRYPT_OK) {
         crypto_err("crypto_make_key", err);
      }
      
      /* clear scheduled keys */
      aes_done(&port->in_key);
      aes_done(&port->out_key);
      
      /* now depending on the side figure out the keys */
      side = memcmp(port->rem_seed, port->loc_seed, 16) <= 0 ? 1 : 0;
      if (side == 0) {
         if ((err = aes_setup(tmp, 16, 0, &port->out_key)) != CRYPT_OK) {
            crypto_err("crypto_make_key", err);
         }
         if ((err = aes_setup(tmp+16, 16, 0, &port->in_key)) != CRYPT_OK) {
            crypto_err("crypto_make_key", err);
         }
         memcpy(port->loc_ctr, tmp+32, 12);
         memcpy(port->rem_ctr, tmp+32+12, 12);
      } else {
         if ((err = aes_setup(tmp, 16, 0, &port->in_key)) != CRYPT_OK) {
            crypto_err("crypto_make_key", err);
         }
         if ((err = aes_setup(tmp+16, 16, 0, &port->out_key)) != CRYPT_OK) {
            crypto_err("crypto_make_key", err);
         }
         memcpy(port->rem_ctr, tmp+32, 12);
         memcpy(port->loc_ctr, tmp+32+12, 12);
      }
   memset(tmp, 0, sizeof(tmp));
   memset(key, 0, sizeof(key));
   
}      

int crypto_write_buf(uvlan_port *port, const unsigned char *out, int outlen)
{
   unsigned char buf[3072];
   unsigned long tmplen;
   int           y, x, err;
   
   if (outlen > (int)sizeof(buf)-256) {
      return UVLAN_FAIL_SIZE;
   }
   
      /* increment counter */
      for (x = 11; x >= 0; x--) {
         if (++(port->loc_ctr[x])) {
            break;
         }
      }
      
      /* form packet */
      buf[0] = UVLAN_TYPE_PKT;
      memcpy(buf+1, port->loc_ctr, 12);
      
      /* encrypt it */
      tmplen = 16;
      if ((err = ccm_memory(aes_idx,
                            NULL, 16, // HACK: they're not used... but this ain't clean
                            &port->out_key,
                            buf+1, 12,
                            NULL, 0,
                            (unsigned char*)out, outlen, // "out" is a bit unfortunate for a name, it's the data to send
                            buf+16+16,
                            buf+16, &tmplen,
                            CCM_ENCRYPT)) != CRYPT_OK) {
         crypto_err("crypto_send", err);
      }
      
      /* send it */
      y = sendto(bound_sock, buf, 16 + 16 + outlen, 0, (struct sockaddr *)&port->dest_sin, sizeof(port->dest_sin));
      if (y != (16 + 16 + outlen)) {
         return UVLAN_FAIL_SEND;
      }
                          
   return UVLAN_READ_OK;
}      

/* append data to the buffer */
int crypto_send(uvlan_port *port, const unsigned char *out, int outlen)
{
   int err;
   
   if (nagle_delay) {
      /* flush if it won't fit */
      if ((outlen + 2 + port->buf_len) > (int)sizeof(port->buf)) {
         if ((err = crypto_write_buf(port, port->buf, port->buf_len)) != UVLAN_READ_OK) {
            return err;
         }
         port->buf_len = 0;
      }
   
      if (outlen + 2 > (int)sizeof(port->buf)) {
         /* this shouldn't happen */
         crypto_err("buffer overflow in crypto_send", -1);
      }
   
      port->buf[port->buf_len]     = (outlen >> 8) & 255;
      port->buf[port->buf_len + 1] = outlen & 255;
      memcpy(port->buf + port->buf_len + 2, out, outlen);
      port->buf_len += 2 + outlen;
      return UVLAN_READ_OK;
   } else {
      port->buf[0] = (outlen >> 8) & 255;
      port->buf[1] = outlen & 255;
      memcpy(port->buf + 2, out, outlen);
      return crypto_write_buf(port, port->buf, outlen + 2);
   }
}
 
int  crypto_read(uvlan_port *port, const unsigned char *in,  int inlen, unsigned char *out, int *outlen)
{
   unsigned char      tag[16];
   unsigned long      tmplen;
   int                err;
   
   if (debug_level) fprintf(stderr, "Parsing %d bytes\n", inlen);
   
   if (inlen < (16+16)) {
      return UVLAN_FAIL_SIZE;
   }
   if ((inlen - 16 - 16) > *outlen) {
      return UVLAN_FAIL_SIZE;
   }
   
      /* decrypt and check it */
      tmplen = 16;
      if ((err = ccm_memory(aes_idx,
                            NULL, 16, // HACK: they're not used... but this ain't clean
                            &port->in_key,
                            in+1, 12,
                            NULL, 0,
                            out, inlen - 16 - 16,
                            (unsigned char*)in+16+16,
                            tag, &tmplen,
                            CCM_DECRYPT)) != CRYPT_OK) {
         crypto_err("crypto_read", err);
      }
      
      if (memcmp(tag, in+16, 16)) {
         if (debug_level) fprintf(stderr, "MAC checksum failed!\n");
         return UVLAN_FAIL_MAC;
      }
      
      /* check counter */
      if (memcmp(port->rem_ctr, in+1, 12) != -1) {
         if (debug_level) fprintf(stderr, "Packet CTR check failed! (soft error)\n");
         return UVLAN_FAIL_CTR;
      }

      /* copy counter and set output size */
      memcpy(port->rem_ctr, in+1, 12);
      *outlen = inlen - 16 - 16;
     
   if (debug_level) fprintf(stderr, "Packet parsed to %d bytes\n", *outlen);
   return UVLAN_READ_OK;
}

