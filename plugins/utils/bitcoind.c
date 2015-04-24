//
//  bitcoind.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_bitcoind_h
#define crypto777_bitcoind_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "../cJSON.h"
#include "../includes/uthash.h"
//#include "../ramchain/ramchain.c"
#include "../mgw/msig.c"
#include "utils777.c"
//#include "huffstream.c"

#define MAX_BLOCKTX 0xffff
struct rawvin { char txidstr[128]; uint16_t vout; };
struct rawvout { char coinaddr[64],script[256]; uint64_t value; };
struct rawtx { uint16_t firstvin,numvins,firstvout,numvouts; char txidstr[128]; };

#define MAX_COINTX_INPUTS 16
#define MAX_COINTX_OUTPUTS 8
struct cointx_input { struct rawvin tx; char coinaddr[64],sigs[1024]; uint64_t value; uint32_t sequence; char used; };
struct cointx_info
{
    uint32_t crc; // MUST be first
    char coinstr[16];
    uint64_t inputsum,amount,change,redeemtxid;
    uint32_t allocsize,batchsize,batchcrc,gatewayid,isallocated;
    // bitcoin tx order
    uint32_t version,timestamp,numinputs;
    uint32_t numoutputs;
    struct cointx_input inputs[MAX_COINTX_INPUTS];
    struct rawvout outputs[MAX_COINTX_OUTPUTS];
    uint32_t nlocktime;
    // end bitcoin txcalc_nxt64bits
    char signedtx[];
};
int32_t _validate_coinaddr(char pubkey[512],char *coinstr,char *serverport,char *userpass,char *coinaddr);
cJSON *_get_localaddresses(char *coinstr,char *serverport,char *userpass);
int32_t _extract_txvals(char *coinaddr,char *script,int32_t nohexout,cJSON *txobj);
char *_get_transaction(char *coinstr,char *serverport,char *userpass,char *txidstr);
char *_get_blockhashstr(char *coinstr,char *serverport,char *userpass,uint32_t blocknum);
cJSON *_get_blockjson(uint32_t *heightp,char *coinstr,char *serverport,char *userpass,char *blockhashstr,uint32_t blocknum);
char *_insert_OP_RETURN(char *rawtx,int32_t do_opreturn,int32_t replace_vout,uint64_t *redeems,int32_t numredeems,int32_t oldtx);

#endif
#else
#ifndef crypto777_bitcoind_c
#define crypto777_bitcoind_c

#ifndef crypto777_bitcoind_h
#define DEFINES_ONLY
#include __BASE_FILE__
#undef DEFINES_ONLY
#endif

enum opcodetype
{
    // push value
    OP_0 = 0x00,
    OP_FALSE = OP_0,
    OP_PUSHDATA1 = 0x4c,
    OP_PUSHDATA2 = 0x4d,
    OP_PUSHDATA4 = 0x4e,
    OP_1NEGATE = 0x4f,
    OP_RESERVED = 0x50,
    OP_1 = 0x51,
    OP_TRUE=OP_1,
    OP_2 = 0x52,
    OP_3 = 0x53,
    OP_4 = 0x54,
    OP_5 = 0x55,
    OP_6 = 0x56,
    OP_7 = 0x57,
    OP_8 = 0x58,
    OP_9 = 0x59,
    OP_10 = 0x5a,
    OP_11 = 0x5b,
    OP_12 = 0x5c,
    OP_13 = 0x5d,
    OP_14 = 0x5e,
    OP_15 = 0x5f,
    OP_16 = 0x60,
    
    // control
    OP_NOP = 0x61,
    OP_VER = 0x62,
    OP_IF = 0x63,
    OP_NOTIF = 0x64,
    OP_VERIF = 0x65,
    OP_VERNOTIF = 0x66,
    OP_ELSE = 0x67,
    OP_ENDIF = 0x68,
    OP_VERIFY = 0x69,
#define OP_RETURN_OPCODE 0x6a
    OP_RETURN = 0x6a,
    
    // stack ops
    OP_TOALTSTACK = 0x6b,
    OP_FROMALTSTACK = 0x6c,
    OP_2DROP = 0x6d,
    OP_2DUP = 0x6e,
    OP_3DUP = 0x6f,
    OP_2OVER = 0x70,
    OP_2ROT = 0x71,
    OP_2SWAP = 0x72,
    OP_IFDUP = 0x73,
    OP_DEPTH = 0x74,
    OP_DROP = 0x75,
    OP_DUP = 0x76,
    OP_NIP = 0x77,
    OP_OVER = 0x78,
    OP_PICK = 0x79,
    OP_ROLL = 0x7a,
    OP_ROT = 0x7b,
    OP_SWAP = 0x7c,
    OP_TUCK = 0x7d,
    
    // splice ops
    OP_CAT = 0x7e,
    OP_SUBSTR = 0x7f,
    OP_LEFT = 0x80,
    OP_RIGHT = 0x81,
    OP_SIZE = 0x82,
    
    // bit logic
    OP_INVERT = 0x83,
    OP_AND = 0x84,
    OP_OR = 0x85,
    OP_XOR = 0x86,
    OP_EQUAL = 0x87,
    OP_EQUALVERIFY = 0x88,
    OP_RESERVED1 = 0x89,
    OP_RESERVED2 = 0x8a,
    
    // numeric
    OP_1ADD = 0x8b,
    OP_1SUB = 0x8c,
    OP_2MUL = 0x8d,
    OP_2DIV = 0x8e,
    OP_NEGATE = 0x8f,
    OP_ABS = 0x90,
    OP_NOT = 0x91,
    OP_0NOTEQUAL = 0x92,
    
    OP_ADD = 0x93,
    OP_SUB = 0x94,
    OP_MUL = 0x95,
    OP_DIV = 0x96,
    OP_MOD = 0x97,
    OP_LSHIFT = 0x98,
    OP_RSHIFT = 0x99,
    
    OP_BOOLAND = 0x9a,
    OP_BOOLOR = 0x9b,
    OP_NUMEQUAL = 0x9c,
    OP_NUMEQUALVERIFY = 0x9d,
    OP_NUMNOTEQUAL = 0x9e,
    OP_LESSTHAN = 0x9f,
    OP_GREATERTHAN = 0xa0,
    OP_LESSTHANOREQUAL = 0xa1,
    OP_GREATERTHANOREQUAL = 0xa2,
    OP_MIN = 0xa3,
    OP_MAX = 0xa4,
    
    OP_WITHIN = 0xa5,
    
    // crypto
    OP_RIPEMD160 = 0xa6,
    OP_SHA1 = 0xa7,
    OP_SHA256 = 0xa8,
    OP_HASH160 = 0xa9,
    OP_HASH256 = 0xaa,
    OP_CODESEPARATOR = 0xab,
    OP_CHECKSIG = 0xac,
    OP_CHECKSIGVERIFY = 0xad,
    OP_CHECKMULTISIG = 0xae,
    OP_CHECKMULTISIGVERIFY = 0xaf,
    
    // expansion
    OP_NOP1 = 0xb0,
    OP_NOP2 = 0xb1,
    OP_NOP3 = 0xb2,
    OP_NOP4 = 0xb3,
    OP_NOP5 = 0xb4,
    OP_NOP6 = 0xb5,
    OP_NOP7 = 0xb6,
    OP_NOP8 = 0xb7,
    OP_NOP9 = 0xb8,
    OP_NOP10 = 0xb9,
    
    // template matching params
    OP_SMALLINTEGER = 0xfa,
    OP_PUBKEYS = 0xfb,
    OP_PUBKEYHASH = 0xfd,
    OP_PUBKEY = 0xfe,
    
    OP_INVALIDOPCODE = 0xff,
};

int32_t _add_opcode(char *hex,int32_t offset,int32_t opcode)
{
    hex[offset + 0] = hexbyte((opcode >> 4) & 0xf);
    hex[offset + 1] = hexbyte(opcode & 0xf);
    return(offset+2);
}

const char *get_opname(enum opcodetype opcode)
{
    switch (opcode)
    {
            // push value
        case OP_0                      : return "0";
        case OP_PUSHDATA1              : return "OP_PUSHDATA1";
        case OP_PUSHDATA2              : return "OP_PUSHDATA2";
        case OP_PUSHDATA4              : return "OP_PUSHDATA4";
        case OP_1NEGATE                : return "-1";
        case OP_RESERVED               : return "OP_RESERVED";
        case OP_1                      : return "1";
        case OP_2                      : return "2";
        case OP_3                      : return "3";
        case OP_4                      : return "4";
        case OP_5                      : return "5";
        case OP_6                      : return "6";
        case OP_7                      : return "7";
        case OP_8                      : return "8";
        case OP_9                      : return "9";
        case OP_10                     : return "10";
        case OP_11                     : return "11";
        case OP_12                     : return "12";
        case OP_13                     : return "13";
        case OP_14                     : return "14";
        case OP_15                     : return "15";
        case OP_16                     : return "16";
            
            // control
        case OP_NOP                    : return "OP_NOP";
        case OP_VER                    : return "OP_VER";
        case OP_IF                     : return "OP_IF";
        case OP_NOTIF                  : return "OP_NOTIF";
        case OP_VERIF                  : return "OP_VERIF";
        case OP_VERNOTIF               : return "OP_VERNOTIF";
        case OP_ELSE                   : return "OP_ELSE";
        case OP_ENDIF                  : return "OP_ENDIF";
        case OP_VERIFY                 : return "OP_VERIFY";
        case OP_RETURN                 : return "OP_RETURN";
            
            // stack ops
        case OP_TOALTSTACK             : return "OP_TOALTSTACK";
        case OP_FROMALTSTACK           : return "OP_FROMALTSTACK";
        case OP_2DROP                  : return "OP_2DROP";
        case OP_2DUP                   : return "OP_2DUP";
        case OP_3DUP                   : return "OP_3DUP";
        case OP_2OVER                  : return "OP_2OVER";
        case OP_2ROT                   : return "OP_2ROT";
        case OP_2SWAP                  : return "OP_2SWAP";
        case OP_IFDUP                  : return "OP_IFDUP";
        case OP_DEPTH                  : return "OP_DEPTH";
        case OP_DROP                   : return "OP_DROP";
        case OP_DUP                    : return "OP_DUP";
        case OP_NIP                    : return "OP_NIP";
        case OP_OVER                   : return "OP_OVER";
        case OP_PICK                   : return "OP_PICK";
        case OP_ROLL                   : return "OP_ROLL";
        case OP_ROT                    : return "OP_ROT";
        case OP_SWAP                   : return "OP_SWAP";
        case OP_TUCK                   : return "OP_TUCK";
            
            // splice ops
        case OP_CAT                    : return "OP_CAT";
        case OP_SUBSTR                 : return "OP_SUBSTR";
        case OP_LEFT                   : return "OP_LEFT";
        case OP_RIGHT                  : return "OP_RIGHT";
        case OP_SIZE                   : return "OP_SIZE";
            
            // bit logic
        case OP_INVERT                 : return "OP_INVERT";
        case OP_AND                    : return "OP_AND";
        case OP_OR                     : return "OP_OR";
        case OP_XOR                    : return "OP_XOR";
        case OP_EQUAL                  : return "OP_EQUAL";
        case OP_EQUALVERIFY            : return "OP_EQUALVERIFY";
        case OP_RESERVED1              : return "OP_RESERVED1";
        case OP_RESERVED2              : return "OP_RESERVED2";
            
            // numeric
        case OP_1ADD                   : return "OP_1ADD";
        case OP_1SUB                   : return "OP_1SUB";
        case OP_2MUL                   : return "OP_2MUL";
        case OP_2DIV                   : return "OP_2DIV";
        case OP_NEGATE                 : return "OP_NEGATE";
        case OP_ABS                    : return "OP_ABS";
        case OP_NOT                    : return "OP_NOT";
        case OP_0NOTEQUAL              : return "OP_0NOTEQUAL";
        case OP_ADD                    : return "OP_ADD";
        case OP_SUB                    : return "OP_SUB";
        case OP_MUL                    : return "OP_MUL";
        case OP_DIV                    : return "OP_DIV";
        case OP_MOD                    : return "OP_MOD";
        case OP_LSHIFT                 : return "OP_LSHIFT";
        case OP_RSHIFT                 : return "OP_RSHIFT";
        case OP_BOOLAND                : return "OP_BOOLAND";
        case OP_BOOLOR                 : return "OP_BOOLOR";
        case OP_NUMEQUAL               : return "OP_NUMEQUAL";
        case OP_NUMEQUALVERIFY         : return "OP_NUMEQUALVERIFY";
        case OP_NUMNOTEQUAL            : return "OP_NUMNOTEQUAL";
        case OP_LESSTHAN               : return "OP_LESSTHAN";
        case OP_GREATERTHAN            : return "OP_GREATERTHAN";
        case OP_LESSTHANOREQUAL        : return "OP_LESSTHANOREQUAL";
        case OP_GREATERTHANOREQUAL     : return "OP_GREATERTHANOREQUAL";
        case OP_MIN                    : return "OP_MIN";
        case OP_MAX                    : return "OP_MAX";
        case OP_WITHIN                 : return "OP_WITHIN";
            
            // crypto
        case OP_RIPEMD160              : return "OP_RIPEMD160";
        case OP_SHA1                   : return "OP_SHA1";
        case OP_SHA256                 : return "OP_SHA256";
        case OP_HASH160                : return "OP_HASH160";
        case OP_HASH256                : return "OP_HASH256";
        case OP_CODESEPARATOR          : return "OP_CODESEPARATOR";
        case OP_CHECKSIG               : return "OP_CHECKSIG";
        case OP_CHECKSIGVERIFY         : return "OP_CHECKSIGVERIFY";
        case OP_CHECKMULTISIG          : return "OP_CHECKMULTISIG";
        case OP_CHECKMULTISIGVERIFY    : return "OP_CHECKMULTISIGVERIFY";
            
            // expanson
        case OP_NOP1                   : return "OP_NOP1";
        case OP_NOP2                   : return "OP_NOP2";
        case OP_NOP3                   : return "OP_NOP3";
        case OP_NOP4                   : return "OP_NOP4";
        case OP_NOP5                   : return "OP_NOP5";
        case OP_NOP6                   : return "OP_NOP6";
        case OP_NOP7                   : return "OP_NOP7";
        case OP_NOP8                   : return "OP_NOP8";
        case OP_NOP9                   : return "OP_NOP9";
        case OP_NOP10                  : return "OP_NOP10";
            
        case OP_INVALIDOPCODE          : return "OP_INVALIDOPCODE";
            
            // Note:
            //  The template matching params OP_SMALLDATA/etc are defined in opcodetype enum
            //  as kind of implementation hack, they are *NOT* real opcodes.  If found in real
            //  Script, just let the default: case deal with them.
            
        default:
            return "OP_UNKNOWN";
    }
}

struct bitcoin_opcode { UT_hash_handle hh; uint8_t opcode; } *optable;
int32_t bitcoin_assembler(char *script)
{
    static struct bitcoin_opcode *optable;
    char hexstr[8192],*hex,*str;
    struct bitcoin_opcode *op;
    char *opname,*invalid = "OP_UNKNOWN";
    int32_t i,j;
    long len,k;
    if ( optable == 0 )
    {
        for (i=0; i<0x100; i++)
        {
            opname = (char *)get_opname(i);
            if ( strcmp(invalid,opname) != 0 )
            {
                op = calloc(1,sizeof(*op));
                HASH_ADD_KEYPTR(hh,optable,opname,strlen(opname),op);
                //printf("{%-16s %02x} ",opname,i);
                op->opcode = i;
            }
        }
        //printf("bitcoin opcodes\n");
    }
    if ( script[0] == 0 )
    {
        strcpy(script,"ffff");
        return(-1);
    }
    len = strlen(script);
    hex = (len < sizeof(hexstr)-2) ? hexstr : calloc(1,len+1);
    strcpy(hex,"ffff");
    str = script;
    k = 0;
    script[len + 1] = 0;
    while ( *str != 0 )
    {
        //printf("k.%ld (%s)\n",k,str);
        for (j=0; str[j]!=0&&str[j]!=' '; j++)
            ;
        str[j] = 0;
        len = strlen(str);
        if ( is_hexstr(str) != 0 && (len & 1) == 0 )
        {
            k = _add_opcode(hex,(int32_t)k,(int32_t)len>>1);
            strcpy(hex+k,str);
            k += len;//, printf("%s ",str);
        }
        else
        {
            HASH_FIND(hh,optable,str,len,op);
            if ( op != 0 )
                k = _add_opcode(hex,(int32_t)k,op->opcode);
            //sprintf(hex+k,"%02x",op->opcode), k += 2, printf("{%s}.%02x ",str,op->opcode);
        }
        str += (j+1);
    }
    hex[k] = 0;
    strcpy(script,hex);
    if ( hex != hexstr )
        free(hex);
    //fprintf(stderr,"-> (%s).k%ld\n",script,k);
    return((is_hexstr(script) != 0 && (strlen(script) & 1) == 0) ? 0 : -1);
}

long hdecode_varint(uint64_t *valp,uint8_t *ptr,long offset,long mappedsize)
{
    uint16_t s; uint32_t i; int32_t c;
    if ( ptr == 0 )
        return(-1);
    *valp = 0;
    if ( offset < 0 || offset >= mappedsize )
        return(-1);
    c = ptr[offset++];
    switch ( c )
    {
        case 0xfd: if ( offset+sizeof(s) > mappedsize ) return(-1); memcpy(&s,&ptr[offset],sizeof(s)), *valp = s, offset += sizeof(s); break;
        case 0xfe: if ( offset+sizeof(i) > mappedsize ) return(-1); memcpy(&i,&ptr[offset],sizeof(i)), *valp = i, offset += sizeof(i); break;
        case 0xff: if ( offset+sizeof(*valp) > mappedsize ) return(-1); memcpy(valp,&ptr[offset],sizeof(*valp)), offset += sizeof(*valp); break;
        default: *valp = c; break;
    }
    return(offset);
}

int32_t hcalc_varint(uint8_t *buf,uint64_t x)
{
    uint16_t s; uint32_t i; int32_t len = 0;
    if ( x < 0xfd )
        buf[len++] = (uint8_t)(x & 0xff);
    else
    {
        if ( x <= 0xffff )
        {
            buf[len++] = 0xfd;
            s = (uint16_t)x;
            memcpy(&buf[len],&s,sizeof(s));
            len += 2;
        }
        else if ( x <= 0xffffffffL )
        {
            buf[len++] = 0xfe;
            i = (uint32_t)x;
            memcpy(&buf[len],&i,sizeof(i));
            len += 4;
        }
        else
        {
            buf[len++] = 0xff;
            memcpy(&buf[len],&x,sizeof(x));
            len += 8;
        }
    }
    return(len);
}

cJSON *_script_has_address(int32_t *nump,cJSON *scriptobj)
{
    int32_t i,n;
    cJSON *addresses,*addrobj;
    if ( scriptobj == 0 )
        return(0);
    addresses = cJSON_GetObjectItem(scriptobj,"addresses");
    *nump = 0;
    if ( addresses != 0 )
    {
        *nump = n = cJSON_GetArraySize(addresses);
        for (i=0; i<n; i++)
        {
            addrobj = cJSON_GetArrayItem(addresses,i);
            return(addrobj);
        }
    }
    return(0);
}

int32_t _extract_txvals(char *coinaddr,char *script,int32_t nohexout,cJSON *txobj)
{
    int32_t numaddresses;
    cJSON *scriptobj,*addrobj,*hexobj;
    scriptobj = cJSON_GetObjectItem(txobj,"scriptPubKey");
    if ( scriptobj != 0 )
    {
        addrobj = _script_has_address(&numaddresses,scriptobj);
        if ( coinaddr != 0 )
            copy_cJSON(coinaddr,addrobj);
        if ( nohexout != 0 )
            hexobj = cJSON_GetObjectItem(scriptobj,"asm");
        else
        {
            hexobj = cJSON_GetObjectItem(scriptobj,"hex");
            if ( hexobj == 0 )
                hexobj = cJSON_GetObjectItem(scriptobj,"asm"), nohexout = 1;
        }
        if ( script != 0 )
        {
            copy_cJSON(script,hexobj);
            if ( nohexout != 0 )
            {
                //fprintf(stderr,"{%s} ",script);
                bitcoin_assembler(script);
            }
        }
        return(0);
    }
    return(-1);
}

long _emit_uint32(uint8_t *data,long offset,uint32_t x)
{
    uint32_t i;
    for (i=0; i<4; i++,x>>=8)
        data[offset + i] = (x & 0xff);
    offset += 4;
    return(offset);
}

long _decode_uint32(uint32_t *uintp,uint8_t *data,long offset,long len)
{
    uint32_t i,x = 0;
    for (i=0; i<4; i++)
        x <<= 8, x |= data[offset + 3 - i];
    offset += 4;
    *uintp = x;
    if ( offset > len )
        return(-1);
    return(offset);
}

long _decode_vin(struct cointx_input *vin,uint8_t *data,long offset,long len)
{
    uint8_t revdata[32];
    uint32_t i,vout;
    uint64_t siglen;
    memset(vin,0,sizeof(*vin));
    for (i=0; i<32; i++)
        revdata[i] = data[offset + 31 - i];
    init_hexbytes_noT(vin->tx.txidstr,revdata,32), offset += 32;
    if ( (offset= _decode_uint32(&vout,data,offset,len)) < 0 )
        return(-1);
    vin->tx.vout = vout;
    if ( (offset= hdecode_varint(&siglen,data,offset,len)) < 0 || siglen > sizeof(vin->sigs) )
        return(-1);
    init_hexbytes_noT(vin->sigs,&data[offset],(int32_t)siglen), offset += siglen;
    if ( (offset= _decode_uint32(&vin->sequence,data,offset,len)) < 0 )
        return(-1);
    //printf("txid.(%s) vout.%d siglen.%d (%s) seq.%d\n",vin->tx.txidstr,vout,(int)siglen,vin->sigs,vin->sequence);
    return(offset);
}

long _emit_cointx_input(uint8_t *data,long offset,struct cointx_input *vin)
{
    uint8_t revdata[32];
    long i,siglen;
    decode_hex(revdata,32,vin->tx.txidstr);
    for (i=0; i<32; i++)
        data[offset + 31 - i] = revdata[i];
    offset += 32;
    
    offset = _emit_uint32(data,offset,vin->tx.vout);
    siglen = strlen(vin->sigs) >> 1;
    offset += hcalc_varint(&data[offset],siglen);
    //printf("decodesiglen.%ld\n",siglen);
    if ( siglen != 0 )
        decode_hex(&data[offset],(int32_t)siglen,vin->sigs), offset += siglen;
    offset = _emit_uint32(data,offset,vin->sequence);
    return(offset);
}

long _emit_cointx_output(uint8_t *data,long offset,struct rawvout *vout)
{
    long scriptlen;
    offset = _emit_uint32(data,offset,(uint32_t)vout->value);
    offset = _emit_uint32(data,offset,(uint32_t)(vout->value >> 32));
    scriptlen = strlen(vout->script) >> 1;
    offset += hcalc_varint(&data[offset],scriptlen);
    // printf("scriptlen.%ld\n",scriptlen);
    if ( scriptlen != 0 )
        decode_hex(&data[offset],(int32_t)scriptlen,vout->script), offset += scriptlen;
    return(offset);
}

long _decode_vout(struct rawvout *vout,uint8_t *data,long offset,long len)
{
    uint32_t low,high;
    uint64_t scriptlen;
    memset(vout,0,sizeof(*vout));
    if ( (offset= _decode_uint32(&low,data,offset,len)) < 0 )
        return(-1);
    if ( (offset= _decode_uint32(&high,data,offset,len)) < 0 )
        return(-1);
    vout->value = ((uint64_t)high << 32) | low;
    if ( (offset= hdecode_varint(&scriptlen,data,offset,len)) < 0 || scriptlen > sizeof(vout->script) )
        return(-1);
    init_hexbytes_noT(vout->script,&data[offset],(int32_t)scriptlen), offset += scriptlen;
    return(offset);
}

void disp_cointx_output(struct rawvout *vout)
{
    printf("(%s %s %.8f) ",vout->coinaddr,vout->script,dstr(vout->value));
}

void disp_cointx_input(struct cointx_input *vin)
{
    printf("{ %s/v%d %s }.s%d ",vin->tx.txidstr,vin->tx.vout,vin->sigs,vin->sequence);
}

void disp_cointx(struct cointx_info *cointx)
{
    int32_t i;
    printf("version.%u timestamp.%u nlocktime.%u numinputs.%u numoutputs.%u\n",cointx->version,cointx->timestamp,cointx->nlocktime,cointx->numinputs,cointx->numoutputs);
    for (i=0; i<cointx->numinputs; i++)
        disp_cointx_input(&cointx->inputs[i]);
    printf("-> ");
    for (i=0; i<cointx->numoutputs; i++)
        disp_cointx_output(&cointx->outputs[i]);
    printf("\n");
}

int32_t _emit_cointx(char *hexstr,long len,struct cointx_info *cointx,int32_t oldtx)
{
    uint8_t *data;
    long offset = 0;
    int32_t i;
    data = calloc(1,len);
    offset = _emit_uint32(data,offset,cointx->version);
    if ( oldtx == 0 )
        offset = _emit_uint32(data,offset,cointx->timestamp);
    offset += hcalc_varint(&data[offset],cointx->numinputs);
    for (i=0; i<cointx->numinputs; i++)
        offset = _emit_cointx_input(data,offset,&cointx->inputs[i]);
    offset += hcalc_varint(&data[offset],cointx->numoutputs);
    for (i=0; i<cointx->numoutputs; i++)
        offset = _emit_cointx_output(data,offset,&cointx->outputs[i]);
    offset = _emit_uint32(data,offset,cointx->nlocktime);
    init_hexbytes_noT(hexstr,data,(int32_t)offset);
    free(data);
    return((int32_t)offset);
}

int32_t _validate_decoderawtransaction(char *hexstr,struct cointx_info *cointx,int32_t oldtx)
{
    char *checkstr;
    long len;
    int32_t retval;
    len = strlen(hexstr) * 2;
    //disp_cointx(cointx);
    checkstr = calloc(1,len + 1);
    _emit_cointx(checkstr,len,cointx,oldtx);
    if ( (retval= strcmp(checkstr,hexstr)) != 0 )
    {
        disp_cointx(cointx);
        printf("_validate_decoderawtransaction: error: \n(%s) != \n(%s)\n",hexstr,checkstr);
        //getchar();
    }
    //else printf("_validate_decoderawtransaction.(%s) validates\n",hexstr);
    free(checkstr);
    return(retval);
}

struct cointx_info *_decode_rawtransaction(char *hexstr,int32_t oldtx)
{
    uint8_t data[8192];
    long len,offset = 0;
    uint64_t numinputs,numoutputs;
    struct cointx_info *cointx;
    uint32_t vin,vout;
    if ( (len= strlen(hexstr)) >= sizeof(data)*2-1 || is_hexstr(hexstr) == 0 || (len & 1) != 0 )
    {
        printf("_decode_rawtransaction: hexstr too long %ld vs %ld || is_hexstr.%d || oddlen.%ld\n",strlen(hexstr),sizeof(data)*2-1,is_hexstr(hexstr),(len & 1));
        return(0);
    }
    //_decode_rawtransaction("0100000001a131c270d541c9d2be98b6f7a88c6cbea5d5a395ec82c9954083675226f399ee0300000000ffffffff042f7500000000000017a9140cc0def37d9682c292d18b3f579b7432adf4703187a0f70300000000001976a914e8bf7b6c41702de3451d189db054c985fe6fbbdb88ac01000000000000001976a914f9fab825f93c5f0ddcf90c4c96c371dc3dbca95788ac10eb09000000000017a914309924e8dad854d4cb8e3d6b839a932aea22590c8700000000"); getchar();
    len >>= 1;
    //printf("(%s).%ld\n",hexstr,len);
    decode_hex(data,(int32_t)len,hexstr);
    //for (int i=0; i<len; i++)
    //    printf("%02x ",data[i]);
    //printf("converted\n");
    cointx = calloc(1,sizeof(*cointx));
    offset = _decode_uint32(&cointx->version,data,offset,len);
    if ( oldtx == 0 )
        offset = _decode_uint32(&cointx->timestamp,data,offset,len);
    offset = hdecode_varint(&numinputs,data,offset,len);
    if ( numinputs > MAX_COINTX_INPUTS )
    {
        printf("_decode_rawtransaction: numinputs %lld > %d MAX_COINTX_INPUTS version.%d timestamp.%u %ld offset.%ld [%x]\n",(long long)numinputs,MAX_COINTX_INPUTS,cointx->version,cointx->timestamp,time(NULL),offset,*(int *)&data[offset]);
        return(0);
    }
    for (vin=0; vin<numinputs; vin++)
        if ( (offset= _decode_vin(&cointx->inputs[vin],data,offset,len)) < 0 )
            break;
    if ( vin != numinputs )
    {
        printf("_decode_rawtransaction: _decode_vin.%d err\n",vin);
        return(0);
    }
    offset = hdecode_varint(&numoutputs,data,offset,len);
    if ( numoutputs > MAX_COINTX_OUTPUTS )
    {
        printf("_decode_rawtransaction: numoutputs %lld > %d MAX_COINTX_INPUTS\n",(long long)numoutputs,MAX_COINTX_OUTPUTS);
        return(0);
    }
    for (vout=0; vout<numoutputs; vout++)
        if ( (offset= _decode_vout(&cointx->outputs[vout],data,offset,len)) < 0 )
            break;
    if ( vout != numoutputs )
    {
        printf("_decode_rawtransaction: _decode_vout.%d err\n",vout);
        return(0);
    }
    offset = _decode_uint32(&cointx->nlocktime,data,offset,len);
    cointx->numinputs = (uint32_t)numinputs;
    cointx->numoutputs = (uint32_t)numoutputs;
    if ( offset != len )
        printf("_decode_rawtransaction warning: offset.%ld vs len.%ld for (%s)\n",offset,len,hexstr);
    _validate_decoderawtransaction(hexstr,cointx,oldtx);
    return(cointx);
}

int32_t _make_OP_RETURN(char *scriptstr,uint64_t *redeems,int32_t numredeems)
{
    long _emit_uint32(uint8_t *data,long offset,uint32_t x);
    uint8_t hashdata[256],revbuf[8];
    uint64_t redeemtxid;
    int32_t i,j,size;
    long offset;
    scriptstr[0] = 0;
    if ( numredeems >= (sizeof(hashdata)/sizeof(uint64_t))-1 )
    {
        printf("ram_make_OP_RETURN numredeems.%d is crazy\n",numredeems);
        return(-1);
    }
    hashdata[1] = OP_RETURN_OPCODE;
    hashdata[2] = 'M', hashdata[3] = 'G', hashdata[4] = 'W';
    hashdata[5] = numredeems;
    offset = 6;
    for (i=0; i<numredeems; i++)
    {
        redeemtxid = redeems[i];
        for (j=0; j<8; j++)
            revbuf[j] = ((uint8_t *)&redeemtxid)[7-j];
        memcpy(&redeemtxid,revbuf,sizeof(redeemtxid));
        offset = _emit_uint32(hashdata,offset,(uint32_t)redeemtxid);
        offset = _emit_uint32(hashdata,offset,(uint32_t)(redeemtxid >> 32));
    }
    hashdata[0] = size = (int32_t)(5 + sizeof(uint64_t)*numredeems);
    init_hexbytes_noT(scriptstr,hashdata+1,hashdata[0]);
    if ( size > 0xfc )
    {
        printf("ram_make_OP_RETURN numredeems.%d -> size.%d too big\n",numredeems,size);
        return(-1);
    }
    return(size);
}

char *_insert_OP_RETURN(char *rawtx,int32_t do_opreturn,int32_t replace_vout,uint64_t *redeems,int32_t numredeems,int32_t oldtx)
{
    char scriptstr[1024],str40[41],*retstr = 0;
    long len,i;
    struct rawvout *vout;
    struct cointx_info *cointx;
    if ( _make_OP_RETURN(scriptstr,redeems,numredeems) > 0 && (cointx= _decode_rawtransaction(rawtx,oldtx)) != 0 )
    {
        //if ( replace_vout == cointx->numoutputs-1 )
        //    cointx->outputs[cointx->numoutputs] = cointx->outputs[cointx->numoutputs-1];
        //cointx->numoutputs++;
        vout = &cointx->outputs[replace_vout];
        ///vout->value = 1;
        //cointx->outputs[0].value -= vout->value;
        //vout->coinaddr[0] = 0;
        if ( do_opreturn != 0 )
            safecopy(vout->script,scriptstr,sizeof(vout->script));
        else
        {
            init_hexbytes_noT(str40,(void *)&redeems[0],sizeof(redeems[0]));
            for (i=strlen(str40); i<40; i++)
                str40[i] = '0';
            str40[i] = 0;
            sprintf(scriptstr,"76a914%s88ac",str40);
            strcpy(vout->script,scriptstr);
        }
        len = strlen(rawtx) * 2;
        retstr = calloc(1,len + 1);
        disp_cointx(cointx);
        if ( _emit_cointx(retstr,len,cointx,oldtx) < 0 )
            free(retstr), retstr = 0;
        free(cointx);
    }
    return(retstr);
}

cJSON *_get_localaddresses(char *coinstr,char *serverport,char *userpass)
{
    char *retstr;
    cJSON *json = 0;
    retstr = bitcoind_RPC(0,coinstr,serverport,userpass,"listaddressgroupings","");
    if ( retstr != 0 )
    {
        json = cJSON_Parse(retstr);
        free(retstr);
    }
    return(json);
}

int32_t _validate_coinaddr(char pubkey[512],char *coinstr,char *serverport,char *userpass,char *coinaddr)
{
    char quotes[512],*retstr;
    int64_t len = 0;
    cJSON *json;
    if ( coinaddr[0] != '"' )
        sprintf(quotes,"\"%s\"",coinaddr);
    else safecopy(quotes,coinaddr,sizeof(quotes));
    if ( (retstr= bitcoind_RPC(0,coinstr,serverport,userpass,"validateaddress",quotes)) != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            copy_cJSON(pubkey,cJSON_GetObjectItem(json,"pubkey"));
            len = (int32_t)strlen(pubkey);
            free_json(json);
        }
        free(retstr);
    }
    return((int32_t)len);
}

char *_get_transaction(char *coinstr,char *serverport,char *userpass,char *txidstr)
{
    char *rawtransaction=0,txid[4096];
    sprintf(txid,"[\"%s\", 1]",txidstr);
    //printf("get_transaction.(%s)\n",txidstr);
    rawtransaction = bitcoind_RPC(0,coinstr,serverport,userpass,"getrawtransaction",txid);
    return(rawtransaction);
}

char *_get_blockhashstr(char *coinstr,char *serverport,char *userpass,uint32_t blocknum)
{
    char numstr[128],*blockhashstr=0;
    sprintf(numstr,"%u",blocknum);
    blockhashstr = bitcoind_RPC(0,coinstr,serverport,userpass,"getblockhash",numstr);
    if ( blockhashstr == 0 || blockhashstr[0] == 0 )
    {
        printf("couldnt get blockhash for %u\n",blocknum);
        if ( blockhashstr != 0 )
            free(blockhashstr);
        return(0);
    }
    return(blockhashstr);
}

cJSON *_get_blockjson(uint32_t *heightp,char *coinstr,char *serverport,char *userpass,char *blockhashstr,uint32_t blocknum)
{
    cJSON *json = 0;
    int32_t flag = 0;
    char buf[1024],*blocktxt = 0;
    if ( blockhashstr == 0 )
        blockhashstr = _get_blockhashstr(coinstr,serverport,userpass,blocknum), flag = 1;
    if ( blockhashstr != 0 )
    {
        sprintf(buf,"\"%s\"",blockhashstr);
        //printf("get_blockjson.(%d %s)\n",blocknum,blockhashstr);
        blocktxt = bitcoind_RPC(0,coinstr,serverport,userpass,"getblock",buf);
        if ( blocktxt != 0 && blocktxt[0] != 0 && (json= cJSON_Parse(blocktxt)) != 0 && heightp != 0 )
            *heightp = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"height"),0xffffffff);
        if ( flag != 0 && blockhashstr != 0 )
            free(blockhashstr);
        if ( blocktxt != 0 )
            free(blocktxt);
    }
    return(json);
}

#endif
#endif
