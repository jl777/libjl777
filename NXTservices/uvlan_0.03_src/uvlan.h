#ifndef UVLAN_H_
#define UVLAN_H_

#ifdef WIN32
 #define socklen_t int
#else
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <netdb.h>
#endif
#include <pcap.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <tomcrypt.h>

#define UVLAN_MAX 32

enum {
   UVLAN_MODE_SEND_RECV=0,
   UVLAN_MODE_MAKE_SEED_MAC_FAILED,
   UVLAN_MODE_RECV_SEED,
   UVLAN_MODE_MAKE_KEY,
   UVLAN_MODE_MAKE_SEED_RECV_SEED
};

enum {
   UVLAN_TYPE_PKT=0,
   UVLAN_TYPE_SEED,
   UVLAN_TYPE_HB
};

#define UVLAN_TREE_NOT_FOUND -1
#define UVLAN_PORT_LOCAL     6667 /* switch port # indicating local traffic */
#define LISTEN_SIZE          2048

enum {
   UVLAN_READ_OK=0,
   UVLAN_FAIL_MAC,    /* fatal, causes re-seeding */
   UVLAN_FAIL_CTR,     /* non-fatal, just drops the packet */
   UVLAN_FAIL_SIZE,
   UVLAN_FAIL_SEND 
};


/* --- STRUCTURES --- */
typedef struct {
   volatile int  cur_mode;          /* current mode */
   volatile unsigned long ticker;   /* ticker used by various "things" */
   
   unsigned char loc_seed[MAXBLOCKSIZE], /* our random seed */
                 rem_seed[MAXBLOCKSIZE], /* their random seed */
                 loc_ctr[MAXBLOCKSIZE],  /* our counter */
                 rem_ctr[MAXBLOCKSIZE];  /* their counter */
                 
   /* the nagle buffers */                 
   unsigned char buf[LISTEN_SIZE+2];
   int           buf_len;
                    
   symmetric_key       out_key, in_key;          
   struct sockaddr_in  dest_sin;
   
   symmetric_key       master_key;
   unsigned char       master_ukey[MAXBLOCKSIZE];
   int                 master_ukey_len;   
   
   pthread_mutex_t     lock;
   char                *nick;
} uvlan_port;

typedef struct {
   unsigned char addr[6];
   int           port;
   uvlan_port   *portptr;
   ulong64       bytes_in[2], bytes_out[2];
   unsigned char ipaddr[4];
   time_t	 last;
} uvlan_host_entry;

typedef struct TREE {
   uvlan_host_entry ent;
   struct TREE      *left, *right;
} uvlan_tree;

/* --- GLOBALS --- */
extern volatile int bound_sock, debug_level, nagle_delay;
extern prng_state uvlan_prng;

/* --- TREE functions --- */
void tree_init(void);
uvlan_tree* tree_add(uvlan_host_entry *ent);
uvlan_tree* tree_find(const unsigned char *addr);
void tree_draw(char *target_string);

/* --- CRYPTO functions --- */
void crypto_init(void);
void crypto_init_port(uvlan_port *port, const unsigned char *masterkey, int keylen);
int crypto_make_send_seed(uvlan_port *port);
int crypto_read_seed(uvlan_port *port, const unsigned char *buf, int buflen);
void crypto_make_key(uvlan_port *port);
int crypto_send(uvlan_port *port, const unsigned char *out, int outlen);
int crypto_write_buf(uvlan_port *port, const unsigned char *out, int outlen);
int  crypto_read(uvlan_port *port, const unsigned char *in,  int inlen, unsigned char *out, int *outlen);

/* HTML/JS consts */
const char *jshtml_part1, *jshtml_part2, *jshtml_part3;

/* XML parsing */
#define xmlp struct xmlp_t
struct xmlp_t {
  char *tag, *data;
  xmlp *next, *down;
  /* non-zero if desired tag could not be found */
  char notfound;
  /* get N-th element from this depth of the XML structure with tag */
  xmlp*(*gn)(struct xmlp_t *, const char *tag, int N);
  /* idential to xmlp->gn with index=0 */
  xmlp*(*g)(struct xmlp_t *, const char *tag);
};

enum {
  XMLP_OK = 0,
  XMLP_NO_OPEN_TAG,
  XMLP_NO_CLOSE_TAG,
  XMLP_RESERVED
};

const char* xmlp_error_to_string(int code);
void xmlp_free(xmlp *x);
void xmlp_init(xmlp *x);
void xmlp_print(xmlp *x, FILE *fout);
int xmlp_read(xmlp *x, const char *xmldata);
xmlp* xmlp_get_elem(xmlp *x, const char *tag, int idx);
const char* xmlp_get_data(xmlp *x, const char *tag, int idx);
const char* xmlp_error_to_string(int code);

#endif

