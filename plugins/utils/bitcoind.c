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
#include "../ramchain/ramchain.c"
#include "../mgw/msig.c"
#include "utils777.c"
#include "huffstream.c"

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

struct rawblock
{
    uint32_t blocknum,allocsize;
    uint16_t format,numtx,numrawvins,numrawvouts;
    uint64_t minted;
    struct rawtx txspace[MAX_BLOCKTX];
    struct rawvin vinspace[MAX_BLOCKTX];
    struct rawvout voutspace[MAX_BLOCKTX];
};
int32_t _extract_txvals(char *coinaddr,char *script,int32_t nohexout,cJSON *txobj);
void ram_clear_rawblock(struct rawblock *raw,int32_t totalflag);
cJSON *ram_rawblock_json(struct rawblock *raw,int32_t allocsize);

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

void _set_string(char type,char *dest,char *src,long max)
{
    if ( src == 0 || src[0] == 0 )
        sprintf(dest,"ffff");
    else if ( strlen(src) < max-1)
        strcpy(dest,src);
    else sprintf(dest,"nonstandard");
}

uint64_t _get_txvouts(struct rawblock *raw,struct rawtx *tx,struct ramchain_info *ram,cJSON *voutsobj)
{
    cJSON *item;
    uint64_t value,total = 0;
    struct rawvout *v;
    int32_t i,numvouts = 0;
    char coinaddr[8192],script[8192];
    tx->firstvout = raw->numrawvouts;
    if ( voutsobj != 0 && is_cJSON_Array(voutsobj) != 0 && (numvouts= cJSON_GetArraySize(voutsobj)) > 0 && tx->firstvout+numvouts < MAX_BLOCKTX )
    {
        for (i=0; i<numvouts; i++,raw->numrawvouts++)
        {
            item = cJSON_GetArrayItem(voutsobj,i);
            value = conv_cJSON_float(item,"value");
            v = &raw->voutspace[raw->numrawvouts];
            memset(v,0,sizeof(*v));
            v->value = value;
            total += value;
            _extract_txvals(coinaddr,script,1,item); // default to nohexout
            _set_string('a',v->coinaddr,coinaddr,sizeof(v->coinaddr));
            _set_string('s',v->script,script,sizeof(v->script));
        }
    } else printf("error with vouts\n");
    tx->numvouts = numvouts;
    return(total);
}

int32_t _get_txvins(struct rawblock *raw,struct rawtx *tx,struct ramchain_info *ram,cJSON *vinsobj)
{
    cJSON *item;
    struct rawvin *v;
    int32_t i,numvins = 0;
    char txidstr[8192],coinbase[8192];
    tx->firstvin = raw->numrawvins;
    if ( vinsobj != 0 && is_cJSON_Array(vinsobj) != 0 && (numvins= cJSON_GetArraySize(vinsobj)) > 0 && tx->firstvin+numvins < MAX_BLOCKTX )
    {
        for (i=0; i<numvins; i++,raw->numrawvins++)
        {
            item = cJSON_GetArrayItem(vinsobj,i);
            if ( numvins == 1  )
            {
                copy_cJSON(coinbase,cJSON_GetObjectItem(item,"coinbase"));
                if ( strlen(coinbase) > 1 )
                    return(0);
            }
            copy_cJSON(txidstr,cJSON_GetObjectItem(item,"txid"));
            v = &raw->vinspace[raw->numrawvins];
            memset(v,0,sizeof(*v));
            v->vout = (int)get_cJSON_int(item,"vout");
            _set_string('t',v->txidstr,txidstr,sizeof(v->txidstr));
        }
    } else printf("error with vins\n");
    tx->numvins = numvins;
    return(numvins);
}

char *_get_transaction(struct ramchain_info *ram,char *txidstr)
{
    char *rawtransaction=0,txid[4096];
    sprintf(txid,"[\"%s\", 1]",txidstr);
    //printf("get_transaction.(%s)\n",txidstr);
    rawtransaction = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"getrawtransaction",txid);
    return(rawtransaction);
}

uint64_t _get_txidinfo(struct rawblock *raw,struct rawtx *tx,struct ramchain_info *ram,int32_t txind,char *txidstr)
{
    char *retstr = 0;
    cJSON *txjson;
    uint64_t total = 0;
    if ( strlen(txidstr) < sizeof(tx->txidstr)-1 )
        strcpy(tx->txidstr,txidstr);
    tx->numvouts = tx->numvins = 0;
    if ( (retstr= _get_transaction(ram,txidstr)) != 0 )
    {
        if ( (txjson= cJSON_Parse(retstr)) != 0 )
        {
            _get_txvins(raw,tx,ram,cJSON_GetObjectItem(txjson,"vin"));
            total = _get_txvouts(raw,tx,ram,cJSON_GetObjectItem(txjson,"vout"));
            free_json(txjson);
        } else printf("update_txid_infos parse error.(%s)\n",retstr);
        free(retstr);
    } else printf("error getting.(%s)\n",txidstr);
    //printf("tx.%d: (%s) numvins.%d numvouts.%d (raw %d %d)\n",txind,tx->txidstr,tx->numvins,tx->numvouts,raw->numrawvins,raw->numrawvouts);
    return(total);
}

char *_get_blockhashstr(struct ramchain_info *ram,uint32_t blocknum)
{
    char numstr[128],*blockhashstr=0;
    sprintf(numstr,"%u",blocknum);
    blockhashstr = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"getblockhash",numstr);
    if ( blockhashstr == 0 || blockhashstr[0] == 0 )
    {
        printf("couldnt get blockhash for %u\n",blocknum);
        if ( blockhashstr != 0 )
            free(blockhashstr);
        return(0);
    }
    return(blockhashstr);
}

cJSON *_get_blockjson(uint32_t *heightp,struct ramchain_info *ram,char *blockhashstr,uint32_t blocknum)
{
    cJSON *json = 0;
    int32_t flag = 0;
    char buf[1024],*blocktxt = 0;
    if ( blockhashstr == 0 )
        blockhashstr = _get_blockhashstr(ram,blocknum), flag = 1;
    if ( blockhashstr != 0 )
    {
        sprintf(buf,"\"%s\"",blockhashstr);
        //printf("get_blockjson.(%d %s)\n",blocknum,blockhashstr);
        blocktxt = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"getblock",buf);
        if ( blocktxt != 0 && blocktxt[0] != 0 && (json= cJSON_Parse(blocktxt)) != 0 && heightp != 0 )
            *heightp = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"height"),0xffffffff);
        if ( flag != 0 && blockhashstr != 0 )
            free(blockhashstr);
        if ( blocktxt != 0 )
            free(blocktxt);
    }
    return(json);
}

cJSON *_get_blocktxarray(uint32_t *blockidp,int32_t *numtxp,struct ramchain_info *ram,cJSON *blockjson)
{
    cJSON *txarray = 0;
    if ( blockjson != 0 )
    {
        *blockidp = (uint32_t)get_API_int(cJSON_GetObjectItem(blockjson,"height"),0);
        txarray = cJSON_GetObjectItem(blockjson,"tx");
        *numtxp = cJSON_GetArraySize(txarray);
    }
    return(txarray);
}

uint32_t _get_blockinfo(struct rawblock *raw,struct ramchain_info *ram,uint32_t blocknum)
{
    char txidstr[8192],mintedstr[8192];
    cJSON *json,*txobj;
    uint32_t blockid;
    int32_t txind,n;
    uint64_t total = 0;
    ram_clear_rawblock(raw,1);
    //raw->blocknum = blocknum;
    //printf("_get_blockinfo.%d\n",blocknum);
    raw->minted = raw->numtx = raw->numrawvins = raw->numrawvouts = 0;
    if ( (json= _get_blockjson(0,ram,0,blocknum)) != 0 )
    {
        raw->blocknum = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"height"),0);
        copy_cJSON(mintedstr,cJSON_GetObjectItem(json,"mint"));
        if ( mintedstr[0] != 0 )
            raw->minted = (uint64_t)(atof(mintedstr) * SATOSHIDEN);
        if ( (txobj= _get_blocktxarray(&blockid,&n,ram,json)) != 0 && blockid == blocknum && n < MAX_BLOCKTX )
        {
            for (txind=0; txind<n; txind++)
            {
                copy_cJSON(txidstr,cJSON_GetArrayItem(txobj,txind));
                //printf("block.%d txind.%d TXID.(%s)\n",blocknum,txind,txidstr);
                total += _get_txidinfo(raw,&raw->txspace[raw->numtx++],ram,txind,txidstr);
            }
        } else printf("error _get_blocktxarray for block.%d got %d, n.%d vs %d\n",blocknum,blockid,n,MAX_BLOCKTX);
        if ( raw->minted == 0 )
            raw->minted = total;
        free_json(json);
    } else printf("get_blockjson error parsing.(%s)\n",txidstr);
    //printf("BLOCK.%d: block.%d numtx.%d minted %.8f rawnumvins.%d rawnumvouts.%d\n",blocknum,raw->blocknum,raw->numtx,dstr(raw->minted),raw->numrawvins,raw->numrawvouts);
    return(raw->numtx);
}

cJSON *_get_localaddresses(struct ramchain_info *ram)
{
    char *retstr;
    cJSON *json = 0;
    retstr = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"listaddressgroupings","");
    if ( retstr != 0 )
    {
        json = cJSON_Parse(retstr);
        free(retstr);
    }
    return(json);
}

int32_t _validate_coinaddr(char pubkey[512],struct ramchain_info *ram,char *coinaddr)
{
    char quotes[512],*retstr;
    int64_t len = 0;
    cJSON *json;
    if ( coinaddr[0] != '"' )
        sprintf(quotes,"\"%s\"",coinaddr);
    else safecopy(quotes,coinaddr,sizeof(quotes));
    if ( (retstr= bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"validateaddress",quotes)) != 0 )
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

int32_t _verify_coinaddress(char *account,int32_t *ismultisigp,int32_t *isminep,struct ramchain_info *ram,char *coinaddr)
{
    char arg[1024],str[MAX_JSON_FIELD],addr[MAX_JSON_FIELD],*retstr;
    cJSON *json,*array;
    struct ramchain_hashptr *addrptr;
    int32_t i,n,verified = 0;
    sprintf(arg,"\"%s\"",coinaddr);
    *ismultisigp = *isminep = 0;
    retstr = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"validateaddress",arg);
    if ( retstr != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            //if ( is_cJSON_True(cJSON_GetObjectItem(json,"ismine")) != 0 )
            //    *isminep = 1;
            copy_cJSON(str,cJSON_GetObjectItem(json,"script"));
            copy_cJSON(account,cJSON_GetObjectItem(json,"account"));
            if ( strcmp(str,"multisig") == 0 )
                *ismultisigp = 1;
            if ( (array= cJSON_GetObjectItem(json,"addresses")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    copy_cJSON(addr,cJSON_GetArrayItem(array,i));
                    if ( addr[0] != 0 && (addrptr= ram_hashsearch(ram->name,0,0,&ram->addrhash,coinaddr,'a')) != 0 && addrptr->mine != 0 )
                    {
                        *isminep = 1;
                        break;
                    }
                }
            }
            verified = 1;
            free_json(json);
        }
        free(retstr);
    }
    return(verified);
}

int32_t _map_msigaddr(char *redeemScript,struct ramchain_info *ram,char *normaladdr,char *msigaddr) //could map to rawind, but this is rarely called
{
    int32_t i,n,ismine;
    cJSON *json,*array,*json2;
    struct multisig_addr *msig;
    char addr[1024],args[1024],*retstr,*retstr2;
    redeemScript[0] = normaladdr[0] = 0;
    if ( (msig= find_msigaddr(msigaddr)) == 0 )
    {
        strcpy(normaladdr,msigaddr);
        printf("cant find_msigaddr.(%s)\n",msigaddr);
        return(0);
    }
    if ( msig->redeemScript[0] != 0 && ram->S.gatewayid >= 0 && ram->S.gatewayid < NUM_GATEWAYS )
    {
        strcpy(normaladdr,msig->pubkeys[ram->S.gatewayid].coinaddr);
        strcpy(redeemScript,msig->redeemScript);
        printf("_map_msigaddr.(%s) -> return (%s) redeem.(%s)\n",msigaddr,normaladdr,redeemScript);
        return(1);
    }
    sprintf(args,"\"%s\"",msig->multisigaddr);
    retstr = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"validateaddress",args);
    if ( retstr != 0 )
    {
        printf("got retstr.(%s)\n",retstr);
        if ( (json = cJSON_Parse(retstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"addresses")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    ismine = 0;
                    copy_cJSON(addr,cJSON_GetArrayItem(array,i));
                    if ( addr[0] != 0 )
                    {
                        sprintf(args,"\"%s\"",addr);
                        retstr2 = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"validateaddress",args);
                        if ( retstr2 != 0 )
                        {
                            if ( (json2 = cJSON_Parse(retstr2)) != 0 )
                                ismine = is_cJSON_True(cJSON_GetObjectItem(json2,"ismine"));
                            free(retstr2);
                        }
                    }
                    if ( ismine != 0 )
                    {
                        //printf("(%s) ismine.%d\n",addr,ismine);
                        strcpy(normaladdr,addr);
                        copy_cJSON(redeemScript,cJSON_GetObjectItem(json,"hex"));
                        break;
                    }
                }
            } free_json(json);
        } free(retstr);
    }
    if ( normaladdr[0] != 0 )
        return(1);
    strcpy(normaladdr,msigaddr);
    return(-1);
}

cJSON *_create_privkeys_json_params(struct ramchain_info *ram,struct cointx_info *cointx,char **privkeys,int32_t numinputs)
{
    int32_t allocflag,i,ret,nonz = 0;
    cJSON *array;
    char args[1024],normaladdr[1024],redeemScript[4096];
    //printf("create privkeys %p numinputs.%d\n",privkeys,numinputs);
    if ( privkeys == 0 )
    {
        privkeys = calloc(numinputs,sizeof(*privkeys));
        for (i=0; i<numinputs; i++)
        {
            if ( (ret= _map_msigaddr(redeemScript,ram,normaladdr,cointx->inputs[i].coinaddr)) >= 0 )
            {
                sprintf(args,"[\"%s\"]",normaladdr);
                //fprintf(stderr,"(%s) -> (%s).%d ",normaladdr,normaladdr,i);
                privkeys[i] = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"dumpprivkey",args);
            } else fprintf(stderr,"ret.%d for %d (%s)\n",ret,i,normaladdr);
        }
        allocflag = 1;
        //fprintf(stderr,"allocated\n");
    } else allocflag = 0;
    array = cJSON_CreateArray();
    for (i=0; i<numinputs; i++)
    {
        if ( ram != 0 && privkeys[i] != 0 )
        {
            nonz++;
            //printf("(%s %s) ",privkeys[i],cointx->inputs[i].coinaddr);
            cJSON_AddItemToArray(array,cJSON_CreateString(privkeys[i]));
        }
    }
    if ( nonz == 0 )
        free_json(array), array = 0;
    // else printf("privkeys.%d of %d: %s\n",nonz,numinputs,cJSON_Print(array));
    if ( allocflag != 0 )
    {
        for (i=0; i<numinputs; i++)
            if ( privkeys[i] != 0 )
                free(privkeys[i]);
        free(privkeys);
    }
    return(array);
}

cJSON *_create_vins_json_params(char **localcoinaddrs,struct ramchain_info *ram,struct cointx_info *cointx)
{
    int32_t i,ret;
    char normaladdr[1024],redeemScript[4096];
    cJSON *json,*array;
    struct cointx_input *vin;
    array = cJSON_CreateArray();
    for (i=0; i<cointx->numinputs; i++)
    {
        vin = &cointx->inputs[i];
        if ( localcoinaddrs != 0 )
            localcoinaddrs[i] = 0;
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json,"txid",cJSON_CreateString(vin->tx.txidstr));
        cJSON_AddItemToObject(json,"vout",cJSON_CreateNumber(vin->tx.vout));
        cJSON_AddItemToObject(json,"scriptPubKey",cJSON_CreateString(vin->sigs));
        if ( (ret= _map_msigaddr(redeemScript,ram,normaladdr,vin->coinaddr)) >= 0 )
            cJSON_AddItemToObject(json,"redeemScript",cJSON_CreateString(redeemScript));
        else printf("ret.%d redeemScript.(%s) (%s) for (%s)\n",ret,redeemScript,normaladdr,vin->coinaddr);
        if ( localcoinaddrs != 0 )
            localcoinaddrs[i] = vin->coinaddr;
        cJSON_AddItemToArray(array,json);
    }
    return(array);
}

char *_createsignraw_json_params(struct ramchain_info *ram,struct cointx_info *cointx,char *rawbytes,char **privkeys)
{
    char *paramstr = 0;
    cJSON *array,*rawobj,*vinsobj=0,*keysobj=0;
    rawobj = cJSON_CreateString(rawbytes);
    if ( rawobj != 0 )
    {
        vinsobj = _create_vins_json_params(0,ram,cointx);
        if ( vinsobj != 0 )
        {
            keysobj = _create_privkeys_json_params(ram,cointx,privkeys,cointx->numinputs);
            if ( keysobj != 0 )
            {
                array = cJSON_CreateArray();
                cJSON_AddItemToArray(array,rawobj);
                cJSON_AddItemToArray(array,vinsobj);
                cJSON_AddItemToArray(array,keysobj);
                paramstr = cJSON_Print(array);
                free_json(array);
            }
            else free_json(vinsobj);
        }
        else free_json(rawobj);
        //printf("vinsobj.%p keysobj.%p rawobj.%p\n",vinsobj,keysobj,rawobj);
    }
    return(paramstr);
}

int32_t _sign_rawtransaction(char *deststr,unsigned long destsize,struct ramchain_info *ram,struct cointx_info *cointx,char *rawbytes,char **privkeys)
{
    cJSON *json,*hexobj,*compobj;
    int32_t completed = -1;
    char *retstr,*signparams;
    deststr[0] = 0;
    //printf("sign_rawtransaction rawbytes.(%s) %p\n",rawbytes,privkeys);
    if ( (signparams= _createsignraw_json_params(ram,cointx,rawbytes,privkeys)) != 0 )
    {
        _stripwhite(signparams,0);
        printf("got signparams.(%s)\n",signparams);
        retstr = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"signrawtransaction",signparams);
        if ( retstr != 0 )
        {
            printf("got retstr.(%s)\n",retstr);
            json = cJSON_Parse(retstr);
            if ( json != 0 )
            {
                hexobj = cJSON_GetObjectItem(json,"hex");
                compobj = cJSON_GetObjectItem(json,"complete");
                if ( compobj != 0 )
                    completed = ((compobj->type&0xff) == cJSON_True);
                copy_cJSON(deststr,hexobj);
                if ( strlen(deststr) > destsize )
                    printf("sign_rawtransaction: strlen(deststr) %ld > %ld destize\n",strlen(deststr),destsize);
                //printf("got signedtransaction.(%s) ret.(%s) completed.%d\n",deststr,retstr,completed);
                free_json(json);
            } else printf("json parse error.(%s)\n",retstr);
            free(retstr);
        } else printf("error signing rawtx\n");
        free(signparams);
    } else printf("error generating signparams\n");
    return(completed);
}

struct cointx_input *_find_bestfit(struct ramchain_info *ram,uint64_t value)
{
    uint64_t above,below,gap;
    int32_t i;
    struct cointx_input *vin,*abovevin,*belowvin;
    abovevin = belowvin = 0;
    for (above=below=i=0; i<ram->MGWnumunspents; i++)
    {
        vin = &ram->MGWunspents[i];
        if ( vin->used != 0 )
            continue;
        if ( vin->value == value )
            return(vin);
        else if ( vin->value > value )
        {
            gap = (vin->value - value);
            if ( above == 0 || gap < above )
            {
                above = gap;
                abovevin = vin;
            }
        }
        else
        {
            gap = (value - vin->value);
            if ( below == 0 || gap < below )
            {
                below = gap;
                belowvin = vin;
            }
        }
    }
    return((abovevin != 0) ? abovevin : belowvin);
}

int64_t _calc_cointx_inputs(struct ramchain_info *ram,struct cointx_info *cointx,int64_t amount)
{
    int64_t remainder,sum = 0;
    int32_t i;
    struct cointx_input *vin;
    cointx->inputsum = cointx->numinputs = 0;
    remainder = amount + ram->txfee;
    for (i=0; i<ram->MGWnumunspents&&i<((int)(sizeof(cointx->inputs)/sizeof(*cointx->inputs)))-1; i++)
    {
        if ( (vin= _find_bestfit(ram,remainder)) != 0 )
        {
            sum += vin->value;
            remainder -= vin->value;
            vin->used = 1;
            cointx->inputs[cointx->numinputs++] = *vin;
            if ( sum >= (amount + ram->txfee) )
            {
                cointx->amount = amount;
                cointx->change = (sum - amount - ram->txfee);
                cointx->inputsum = sum;
                fprintf(stderr,"numinputs %d sum %.8f vs amount %.8f change %.8f -> miners %.8f\n",cointx->numinputs,dstr(cointx->inputsum),dstr(amount),dstr(cointx->change),dstr(sum - cointx->change - cointx->amount));
                return(cointx->inputsum);
            }
        } else printf("no bestfit found i.%d of %d\n",i,ram->MGWnumunspents);
    }
    fprintf(stderr,"error numinputs %d sum %.8f\n",cointx->numinputs,dstr(cointx->inputsum));
    return(0);
}

char *_sign_localtx(struct ramchain_info *ram,struct cointx_info *cointx,char *rawbytes)
{
    char *batchsigned;
    cointx->batchsize = (uint32_t)strlen(rawbytes) + 1;
    cointx->batchcrc = _crc32(0,rawbytes+12,cointx->batchsize-12); // skip past timediff
    batchsigned = malloc(cointx->batchsize + cointx->numinputs*512 + 512);
    _sign_rawtransaction(batchsigned,cointx->batchsize + cointx->numinputs*512 + 512,ram,cointx,rawbytes,0);
    return(batchsigned);
}

cJSON *_create_vouts_json_params(struct cointx_info *cointx)
{
    int32_t i;
    cJSON *json,*obj;
    json = cJSON_CreateObject();
    for (i=0; i<cointx->numoutputs; i++)
    {
        obj = cJSON_CreateNumber(dstr(cointx->outputs[i].value));
        if ( strcmp(cointx->outputs[i].coinaddr,"OP_RETURN") != 0 )
            cJSON_AddItemToObject(json,cointx->outputs[i].coinaddr,obj);
        else
        {
            // int32_t ram_make_OP_RETURN(char *scriptstr,uint64_t *redeems,int32_t numredeems)
            cJSON_AddItemToObject(json,cointx->outputs[0].coinaddr,obj);
        }
    }
    printf("numdests.%d (%s)\n",cointx->numoutputs,cJSON_Print(json));
    return(json);
}

char *_createrawtxid_json_params(struct ramchain_info *ram,struct cointx_info *cointx)
{
    char *paramstr = 0;
    cJSON *array,*vinsobj,*voutsobj;
    vinsobj = _create_vins_json_params(0,ram,cointx);
    if ( vinsobj != 0 )
    {
        voutsobj = _create_vouts_json_params(cointx);
        if ( voutsobj != 0 )
        {
            array = cJSON_CreateArray();
            cJSON_AddItemToArray(array,vinsobj);
            cJSON_AddItemToArray(array,voutsobj);
            paramstr = cJSON_Print(array);
            free_json(array);   // this frees both vinsobj and voutsobj
        }
        else free_json(vinsobj);
    } else printf("_error create_vins_json_params\n");
    //printf("_createrawtxid_json_params.%s\n",paramstr);
    return(paramstr);
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

char *_insert_OP_RETURN(char *rawtx,int32_t replace_vout,uint64_t *redeems,int32_t numredeems,int32_t oldtx)
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
        //safecopy(vout->script,scriptstr,sizeof(vout->script));
        init_hexbytes_noT(str40,(void *)&redeems[0],sizeof(redeems[0]));
        for (i=strlen(str40); i<40; i++)
            str40[i] = '0';
        str40[i] = 0;
        sprintf(scriptstr,"76a914%s88ac",str40);
        strcpy(vout->script,scriptstr);
        len = strlen(rawtx) * 2;
        retstr = calloc(1,len + 1);
        disp_cointx(cointx);
        if ( _emit_cointx(retstr,len,cointx,oldtx) < 0 )
            free(retstr), retstr = 0;
        free(cointx);
    }
    return(retstr);
}

int32_t ram_is_MGW_OP_RETURN(uint64_t *redeemtxids,struct ramchain_info *ram,uint32_t script_rawind)
{
    static uint8_t zero12[12];
    void *ram_gethashdata(struct ramchain_info *ram,char type,uint32_t rawind);
    int32_t j,numredeems = 0;
    uint8_t *hashdata;
    uint64_t redeemtxid;
    if ( (hashdata= ram_gethashdata(ram,'s',script_rawind)) != 0 )
    {
        /*if ( (len= hashdata[0]) < 256 && hashdata[1] == OP_RETURN_OPCODE && hashdata[2] == 'M' && hashdata[3] == 'G' && hashdata[4] == 'W' )
         {
         numredeems = hashdata[5];
         if ( (numredeems*sizeof(uint64_t) + 5) == len )
         {
         hashdata = &hashdata[6];
         for (i=0; i<numredeems; i++)
         {
         for (redeemtxid=j=0; j<(int32_t)sizeof(uint64_t); j++)
         redeemtxid <<= 8, redeemtxid |= (*hashdata++ & 0xff);
         redeemtxids[i] = redeemtxid;
         }
         } else printf("ram_is_MGW_OP_RETURN: numredeems.%d + 5 != %d len\n",numredeems,len);
         }*/
        //sprintf(scriptstr,"76a914%s88ac",str40);
        if ( hashdata[1] == 0x76 && hashdata[2] == 0xa9 && hashdata[3] == 0x14 && memcmp(&hashdata[12],zero12,12) == 0 )
        {
            hashdata = &hashdata[4];
            for (redeemtxid=j=0; j<(int32_t)sizeof(uint64_t); j++)
                redeemtxid <<= 8, redeemtxid |= (*hashdata++ & 0xff);
            redeemtxids[0] = redeemtxid;
            printf("FOUND HACKRETURN.(%llu)\n",(long long)redeemtxid);
            numredeems = 1;
        }
    }
    return(numredeems);
}

struct cointx_info *_calc_cointx_withdraw(struct ramchain_info *ram,char *destaddr,uint64_t value,uint64_t redeemtxid)
{
    //int64 nPayFee = nTransactionFee * (1 + (int64)nBytes / 1000);
    char *rawparams,*signedtx,*changeaddr,*with_op_return=0,*retstr = 0;
    int64_t MGWfee,sum,amount;
    int32_t allocsize,opreturn_output,numoutputs = 0;
    struct cointx_info *cointx,TX,*rettx = 0;
    cointx = &TX;
    memset(cointx,0,sizeof(*cointx));
    strcpy(cointx->coinstr,ram->name);
    cointx->redeemtxid = redeemtxid;
    cointx->gatewayid = ram->S.gatewayid;
    MGWfee = 0*(value >> 10) + (2 * (ram->txfee + ram->NXTfee_equiv)) - ram->minoutput - ram->txfee;
    if ( value <= MGWfee + ram->minoutput + ram->txfee )
    {
        printf("%s redeem.%llu withdraw %.8f < MGWfee %.8f + minoutput %.8f + txfee %.8f\n",ram->name,(long long)redeemtxid,dstr(value),dstr(MGWfee),dstr(ram->minoutput),dstr(ram->txfee));
        return(0);
    }
    strcpy(cointx->outputs[numoutputs].coinaddr,ram->marker2);
    if ( strcmp(destaddr,ram->marker2) == 0 )
        cointx->outputs[numoutputs++].value = value - ram->minoutput - ram->txfee;
    else
    {
        cointx->outputs[numoutputs++].value = MGWfee;
        strcpy(cointx->outputs[numoutputs].coinaddr,destaddr);
        cointx->outputs[numoutputs++].value = value - MGWfee - ram->minoutput - ram->txfee;
    }
    opreturn_output = numoutputs;
    strcpy(cointx->outputs[numoutputs].coinaddr,ram->opreturnmarker);
    cointx->outputs[numoutputs++].value = ram->minoutput;
    cointx->numoutputs = numoutputs;
    cointx->amount = amount = (MGWfee + value + ram->minoutput + ram->txfee);
    fprintf(stderr,"calc_withdraw.%s %llu amount %.8f -> balance %.8f\n",ram->name,(long long)redeemtxid,dstr(cointx->amount),dstr(ram->S.MGWbalance));
    if ( ram->S.MGWbalance >= 0 )
    {
        if ( (sum= _calc_cointx_inputs(ram,cointx,cointx->amount)) >= (cointx->amount + ram->txfee) )
        {
            if ( cointx->change != 0 )
            {
                changeaddr = (strcmp(ram->MGWsmallest,destaddr) != 0) ? ram->MGWsmallest : ram->MGWsmallestB;
                if ( changeaddr[0] == 0 )
                {
                    printf("Need to create more deposit addresses, need to have at least 2 available\n");
                    exit(1);
                }
                if ( strcmp(cointx->outputs[0].coinaddr,changeaddr) != 0 )
                {
                    strcpy(cointx->outputs[cointx->numoutputs].coinaddr,changeaddr);
                    cointx->outputs[cointx->numoutputs].value = cointx->change;
                    cointx->numoutputs++;
                } else cointx->outputs[0].value += cointx->change;
            }
            rawparams = _createrawtxid_json_params(ram,cointx);
            if ( rawparams != 0 )
            {
                //fprintf(stderr,"len.%ld rawparams.(%s)\n",strlen(rawparams),rawparams);
                _stripwhite(rawparams,0);
                if (  ram->S.gatewayid >= 0 )
                {
                    retstr = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"createrawtransaction",rawparams);
                    if (retstr != 0 && retstr[0] != 0 )
                    {
                        fprintf(stderr,"len.%ld calc_rawtransaction retstr.(%s)\n",strlen(retstr),retstr);
                        if ( (with_op_return= _insert_OP_RETURN(retstr,opreturn_output,&redeemtxid,1,ram->oldtx)) != 0 )
                        {
                            if ( (signedtx= _sign_localtx(ram,cointx,with_op_return)) != 0 )
                            {
                                allocsize = (int32_t)(sizeof(*rettx) + strlen(signedtx) + 1);
                                // printf("signedtx returns.(%s) allocsize.%d\n",signedtx,allocsize);
                                rettx = calloc(1,allocsize);
                                *rettx = *cointx;
                                rettx->allocsize = allocsize;
                                rettx->isallocated = allocsize;
                                strcpy(rettx->signedtx,signedtx);
                                free(signedtx);
                                cointx = 0;
                            } else printf("error _sign_localtx.(%s)\n",with_op_return);
                            free(with_op_return);
                        } else printf("error replacing with OP_RETURN\n");
                    } else fprintf(stderr,"error creating rawtransaction\n");
                }
                free(rawparams);
                if ( retstr != 0 )
                    free(retstr);
            } else fprintf(stderr,"error creating rawparams\n");
        } else fprintf(stderr,"error calculating rawinputs.%.8f or outputs.%.8f | txfee %.8f\n",dstr(sum),dstr(cointx->amount),dstr(ram->txfee));
    } else fprintf(stderr,"not enough %s balance %.8f for withdraw %.8f txfee %.8f\n",ram->name,dstr(ram->S.MGWbalance),dstr(cointx->amount),dstr(ram->txfee));
    return(rettx);
}

uint32_t _get_RTheight(struct ramchain_info *ram)
{
    char *retstr;
    cJSON *json;
    uint32_t height = 0;
    if ( milliseconds() > ram->lastgetinfo+10000 )
    {
        //printf("RTheight.(%s) (%s)\n",ram->name,ram->serverport);
        retstr = bitcoind_RPC(0,ram->name,ram->serverport,ram->userpass,"getinfo","");
        if ( retstr != 0 )
        {
            if ( (json= cJSON_Parse(retstr)) != 0 )
            {
                height = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"blocks"),0);
                free_json(json);
                ram->lastgetinfo = milliseconds();
            }
            free(retstr);
        }
    } else height = ram->S.RTblocknum;
    return(height);
}

cJSON *ram_rawvin_json(struct rawblock *raw,struct rawtx *tx,int32_t vin)
{
    struct rawvin *vi = &raw->vinspace[tx->firstvin + vin];
    cJSON *json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"txid",cJSON_CreateString(vi->txidstr));
    cJSON_AddItemToObject(json,"vout",cJSON_CreateNumber(vi->vout));
    return(json);
}

cJSON *ram_rawvout_json(struct rawblock *raw,struct rawtx *tx,int32_t vout)
{
    struct rawvout *vo = &raw->voutspace[tx->firstvout + vout];
    cJSON *item,*array,*json = cJSON_CreateObject();
    /*"scriptPubKey" : {
     "asm" : "OP_DUP OP_HASH160 b2098d38dfd1bee61b12c9abc40e988d273d90ae OP_EQUALVERIFY OP_CHECKSIG",
     "reqSigs" : 1,
     "type" : "pubkeyhash",
     "addresses" : [
     "RRWZoKdmHGDbS5vfj7KwBScK3uSTpt9pHL"
     ]
     }*/
    cJSON_AddItemToObject(json,"value",cJSON_CreateNumber(dstr(vo->value)));
    cJSON_AddItemToObject(json,"n",cJSON_CreateNumber(vout));
    item = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"hex",cJSON_CreateString(vo->script));
    array = cJSON_CreateArray();
    cJSON_AddItemToArray(array,cJSON_CreateString(vo->coinaddr));
    cJSON_AddItemToObject(item,"addresses",array);
    cJSON_AddItemToObject(json,"scriptPubKey",item);
    return(json);
}

cJSON *ram_rawtx_json(struct rawblock *raw,int32_t txind)
{
    struct rawtx *tx = &raw->txspace[txind];
    cJSON *array,*json = cJSON_CreateObject();
    int32_t i,numvins,numvouts;
    cJSON_AddItemToObject(json,"txid",cJSON_CreateString(tx->txidstr));
    if ( (numvins= tx->numvins) > 0 )
    {
        array = cJSON_CreateArray();
        for (i=0; i<numvins; i++)
            cJSON_AddItemToArray(array,ram_rawvin_json(raw,tx,i));
        cJSON_AddItemToObject(json,"vin",array);
    }
    if ( (numvouts= tx->numvouts) > 0 )
    {
        array = cJSON_CreateArray();
        for (i=0; i<numvouts; i++)
            cJSON_AddItemToArray(array,ram_rawvout_json(raw,tx,i));
        cJSON_AddItemToObject(json,"vout",array);
    }
    return(json);
}

cJSON *ram_rawblock_json(struct rawblock *raw,int32_t allocsize)
{
    int32_t i,n;
    cJSON *array,*json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"height",cJSON_CreateNumber(raw->blocknum));
    cJSON_AddItemToObject(json,"numtx",cJSON_CreateNumber(raw->numtx));
    cJSON_AddItemToObject(json,"mint",cJSON_CreateNumber(dstr(raw->minted)));
    if ( allocsize != 0 )
        cJSON_AddItemToObject(json,"rawsize",cJSON_CreateNumber(allocsize));
    if ( (n= raw->numtx) > 0 )
    {
        array = cJSON_CreateArray();
        for (i=0; i<n; i++)
            cJSON_AddItemToArray(array,ram_rawtx_json(raw,i));
        cJSON_AddItemToObject(json,"tx",array);
    }
    return(json);
}


#endif
#endif
