//
//  portable777.h
//  crypto777
//
//  Created by James on 7/7/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifndef crypto777_portable777_h
#define crypto777_portable777_h

#include <stdint.h>
#include "mutex.h"
#include "nn.h"
#include "pubsub.h"
#include "pipeline.h"
#include "survey.h"
#include "reqrep.h"
#include "bus.h"
#include "pair.h"
#include "sha256.h"
#include "cJSON.h"
#include "uthash.h"

#define OP_RETURN_OPCODE 0x6a


int32_t portable_truncate(char *fname,long filesize);
void *map_file(char *fname,uint64_t *filesizep,int32_t enablewrite);
int32_t os_supports_mappedfiles();
char *os_compatible_path(char *str);
char *OS_rmstr();
int32_t OS_launch_process(char *args[]);
int32_t OS_getppid();
int32_t OS_getpid();
int32_t OS_waitpid(int32_t childpid,int32_t *statusp,int32_t flags);
int32_t OS_conv_unixtime(int32_t *secondsp,time_t timestamp);
uint32_t OS_conv_datenum(int32_t datenum,int32_t hour,int32_t minute,int32_t second);

char *nn_typestr(int32_t type);

// only OS portable functions in this file
#define portable_mutex_t struct nn_mutex
#define portable_mutex_init nn_mutex_init
#define portable_mutex_lock nn_mutex_lock
#define portable_mutex_unlock nn_mutex_unlock

struct queueitem { struct queueitem *next,*prev; };
typedef struct queue
{
	struct queueitem *list;
	portable_mutex_t mutex;
    char name[31],initflag;
} queue_t;

struct pingpong_queue
{
    char *name;
    queue_t pingpong[2],*destqueue,*errorqueue;
    int32_t (*action)();
    int offset;
};

#define ACCTS777_MAXRAMKVS 8
#define BTCDADDRSIZE 36
union _bits128 { uint8_t bytes[16]; uint16_t ushorts[8]; uint32_t uints[4]; uint64_t ulongs[2]; uint64_t txid; };
typedef union _bits128 bits128;
union _bits256 { uint8_t bytes[32]; uint16_t ushorts[16]; uint32_t uints[8]; uint64_t ulongs[4]; uint64_t txid; };
typedef union _bits256 bits256;
union _bits384 { bits256 sig; uint8_t bytes[48]; uint16_t ushorts[24]; uint32_t uints[12]; uint64_t ulongs[6]; uint64_t txid; };
typedef union _bits384 bits384;

struct ramkv777_item { UT_hash_handle hh; uint16_t valuesize,tbd; uint32_t rawind; uint8_t keyvalue[]; };
struct ramkv777
{
    char name[63],threadsafe;
    portable_mutex_t mutex;
    struct ramkv777_item *table;
    struct sha256_state state; bits256 sha256;
    int32_t numkeys,keysize,dispflag; uint8_t kvind;
};

#define ramkv777_itemsize(kv,valuesize) (sizeof(struct ramkv777_item) + (kv)->keysize + valuesize)
#define ramkv777_itemkey(item) (item)->keyvalue
#define ramkv777_itemvalue(kv,item) (&(item)->keyvalue[(kv)->keysize])

void ramkv777_lock(struct ramkv777 *kv);
void ramkv777_unlock(struct ramkv777 *kv);
int32_t ramkv777_delete(struct ramkv777 *kv,void *key);
void *ramkv777_write(struct ramkv777 *kv,void *key,void *value,int32_t valuesize);
void *ramkv777_read(int32_t *valuesizep,struct ramkv777 *kv,void *key);
void *ramkv777_iterate(struct ramkv777 *kv,void *args,void *(*iterator)(struct ramkv777 *kv,void *args,void *key,void *value,int32_t valuesize));
struct ramkv777 *ramkv777_init(int32_t kvind,char *name,int32_t keysize,int32_t threadsafe);
void ramkv777_free(struct ramkv777 *kv);
int32_t ramkv777_clone(struct ramkv777 *clone,struct ramkv777 *kv);
struct ramkv777_item *ramkv777_itemptr(struct ramkv777 *kv,void *value);

void lock_queue(queue_t *queue);
void queue_enqueue(char *name,queue_t *queue,struct queueitem *item);
void *queue_dequeue(queue_t *queue,int32_t offsetflag);
int32_t queue_size(queue_t *queue);
struct queueitem *queueitem(char *str);
struct queueitem *queuedata(void *data,int32_t datalen);
void free_queueitem(void *itemptr);
void *queue_delete(queue_t *queue,struct queueitem *copy,int32_t copysize);
void *queue_free(queue_t *queue);
void *queue_clone(queue_t *clone,queue_t *queue,int32_t size);

void calc_sha256(char hashstr[(256 >> 3) * 2 + 1],uint8_t hash[256 >> 3],uint8_t *src,int32_t len);
void calc_sha256cat(uint8_t hash[256 >> 3],uint8_t *src,int32_t len,uint8_t *src2,int32_t len2);
void update_sha256(uint8_t hash[256 >> 3],struct sha256_state *state,uint8_t *src,int32_t len);

int32_t init_hexbytes_noT(char *hexbytes,uint8_t *message,long len);
int32_t decode_hex(unsigned char *bytes,int32_t n,char *hex);
int init_base32(char *tokenstr,uint8_t *token,int32_t len);
int decode_base32(uint8_t *token,uint8_t *tokenstr,int32_t len);
uint64_t stringbits(char *str);

long _stripwhite(char *buf,int accept);
char *clonestr(char *str);
int32_t is_decimalstr(char *str);
int32_t safecopy(char *dest,char *src,long len);

int32_t hcalc_varint(uint8_t *buf,uint64_t x);
long hdecode_varint(uint64_t *valp,uint8_t *ptr,long offset,long mappedsize);

double milliseconds();

#endif
