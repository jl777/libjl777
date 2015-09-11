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


#ifdef DEFINES_ONLY
#ifndef crypto777_gen1_h
#define crypto777_gen1_h
#include <stdio.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "../includes/cJSON.h"
#include "../includes/uthash.h"
#include "cointx.c"
#include "../utils/utils777.c"
#include "coins777.c"

int32_t bitcoin_assembler(char *script);
int32_t _extract_txvals(struct destbuf *coinaddr,struct destbuf *script,int32_t nohexout,cJSON *txobj);
uint8_t *ram_encode_hashstr(int32_t *datalenp,uint8_t *data,char type,char *hashstr);
char *ram_decode_hashdata(char *strbuf,char type,uint8_t *hashdata);

char *_get_transaction(char *coinstr,char *serverport,char *userpass,char *txidstr);
uint64_t ram_verify_txstillthere(char *coinstr,char *serverport,char *userpass,char *txidstr,int32_t vout);
//char *_get_blockhashstr(char *coinstr,char *serverport,char *userpass,uint32_t blocknum);
cJSON *_get_blockjson(uint32_t *heightp,char *coinstr,char *serverport,char *userpass,char *blockhashstr,uint32_t blocknum);
uint32_t _get_RTheight(double *lastmillip,char *coinstr,char *serverport,char *userpass,int32_t current_RTblocknum);
cJSON *_rawblock_txarray(uint32_t *blockidp,int32_t *numtxp,cJSON *blockjson);

char *dumpprivkey(char *coinstr,char *serverport,char *userpass,char *coinaddr);
char *get_acct_coinaddr(char *coinaddr,char *coinstr,char *serverport,char *userpass,char *NXTaddr);
int32_t get_pubkey(struct destbuf *pubkey,char *coinstr,char *serverport,char *userpass,char *coinaddr);
cJSON *_get_localaddresses(char *coinstr,char *serverport,char *userpass);

#endif
#else
#ifndef crypto777_gen1_c
#define crypto777_gen1_c

#ifndef crypto777_gen1_h
#define DEFINES_ONLY
#include "gen1.c"
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

int32_t ram_expand_scriptdata(char *scriptstr,uint8_t *scriptdata,int32_t datalen)
{
    char *prefix,*suffix;
    int32_t mode,n = 0;
    scriptstr[0] = 0;
    switch ( (mode= scriptdata[n++]) )
    {
        case 0: case 'z': prefix = "ffff", suffix = ""; break;
        case 'n': prefix = "nonstandard", suffix = ""; break;
        case 's': prefix = "76a914", suffix = "88ac"; break;
        case 'm': prefix = "a9", suffix = "ac"; break;
        case 'r': prefix = "", suffix = "ac"; break;
        case ' ': prefix = "", suffix = ""; break;
        default: printf("unexpected scriptmode.(%d) (%c)\n",mode,mode); prefix = "", suffix = ""; return(-1); break;
    }
    strcpy(scriptstr,prefix);
    init_hexbytes_noT(scriptstr+strlen(scriptstr),scriptdata+n,datalen-n);
    if ( suffix[0] != 0 )
    {
        //printf("mode.(%c) suffix.(%s) [%s]\n",mode,suffix,scriptstr);
        strcat(scriptstr,suffix);
    }
    return(mode);
}

uint64_t ram_check_redeemcointx(int32_t *unspendablep,char *coinstr,char *script,uint32_t blocknum)
{
    uint64_t redeemtxid = 0;
    int32_t i;
    *unspendablep = 0;
    if ( strcmp(script,"76a914000000000000000000000000000000000000000088ac") == 0 )
        *unspendablep = 1;
    if ( strcmp(script+22,"00000000000000000000000088ac") == 0 )
    {
        for (redeemtxid=i=0; i<(int32_t)sizeof(uint64_t); i++)
        {
            redeemtxid <<= 8;
            redeemtxid |= (_decode_hex(&script[6 + 14 - i*2]) & 0xff);
        }
        printf("%s >>>>>>>>>>>>>>> found MGW redeem @blocknum.%u %s -> %llu | unspendable.%d\n",coinstr,blocknum,script,(long long)redeemtxid,*unspendablep);
    }
    else if ( *unspendablep != 0 )
        printf("%s >>>>>>>>>>>>>>> found unspendable %s\n",coinstr,script);
    
    //else printf("(%s).%d\n",script+22,strcmp(script+16,"00000000000000000000000088ac"));
    return(redeemtxid);
}

int32_t ram_calc_scriptmode(int32_t *datalenp,uint8_t scriptdata[4096],char *script,int32_t trimflag)
{
    int32_t n=0,len,mode = 0;
    len = (int32_t)strlen(script);
    *datalenp = 0;
    if ( len >= 8191 )
    {
        printf("calc_scriptmode overflow len.%d\n",len);
        return(-1);
    }
    if ( strcmp(script,"ffff") == 0 )
    {
        mode = 'z';
        if ( trimflag != 0 )
            script[0] = 0;
    }
    else if ( strcmp(script,"nonstandard") == 0 )
    {
        if ( trimflag != 0 )
            script[0] = 0;
        mode = 'n';
    }
    else if ( strncmp(script,"76a914",6) == 0 && strcmp(script+len-4,"88ac") == 0 )
    {
        if ( trimflag != 0 )
        {
            script[len-4] = 0;
            script += 6;
        }
        mode = 's';
    }
    else if ( strcmp(script+len-2,"ac") == 0 )
    {
        if ( strncmp(script,"a9",2) == 0 )
        {
            if ( trimflag != 0 )
            {
                script[len-2] = 0;
                script += 2;
            }
            mode = 'm';
        }
        else
        {
            if ( trimflag != 0 )
                script[len-2] = 0;
            mode = 'r';
        }
    } else mode = ' ';
    if ( trimflag != 0 )
    {
        scriptdata[n++] = mode;
        if ( (len= (int32_t)(strlen(script) >> 1)) > 0 )
            decode_hex(scriptdata+n,len,script);
        (*datalenp) = (len + n);
        //printf("set pubkey.(%s).%ld <- (%s)\n",pubkeystr,strlen(pubkeystr),script);
    }
    return(mode);
}

uint8_t *ram_encode_hashstr(int32_t *datalenp,uint8_t *data,char type,char *hashstr)
{
    uint8_t varbuf[9];
    char buf[8192];
    int32_t varlen,datalen=0,scriptmode = 0;
    *datalenp = 0;
    if ( type == 's' )
    {
        if ( hashstr[0] == 0 )
            return(0);
        strcpy(buf,hashstr);
        if ( (scriptmode = ram_calc_scriptmode(&datalen,&data[9],buf,1)) < 0 )
        {
            printf("encode_hashstr: scriptmode.%d for (%s)\n",scriptmode,hashstr);
            exit(-1);
        }
    }
    else if ( type == 't' )
    {
        datalen = (int32_t)(strlen(hashstr) >> 1);
        if ( datalen > 4096 )
        {
            printf("encode_hashstr: type.%d (%c) datalen.%d > sizeof(data) %d\n",type,type,(int)datalen,4096);
            getchar();//exit(-1);
        }
        decode_hex(&data[9],datalen,hashstr);
    }
    else if ( type == 'a' )
    {
        datalen = (int32_t)strlen(hashstr) + 1;
        memcpy(&data[9],hashstr,datalen);
    }
    else
    {
        printf("encode_hashstr: unsupported type.%d (%c)\n",type,type);
        getchar();//exit(-1);
    }
    if ( datalen > 0 )
    {
        varlen = hcalc_varint(varbuf,datalen);
        memcpy(&data[9-varlen],varbuf,varlen);
        //HASH_FIND(hh,hash->table,&ptr[-varlen],datalen+varlen,hp);
        *datalenp = (datalen + varlen);
        return(&data[9-varlen]);
    }
    return(0);
}

char *ram_decode_hashdata(char *strbuf,char type,uint8_t *hashdata)
{
    uint64_t varint;
    int32_t datalen,scriptmode;
    strbuf[0] = 0;
    if ( hashdata == 0 )
        return(0);
    hashdata += hdecode_varint(&varint,hashdata,0,9);
    datalen = (int32_t)varint;
    if ( type == 's' )
    {
        if ( (scriptmode= ram_expand_scriptdata(strbuf,hashdata,(uint32_t)datalen)) < 0 )
        {
            printf("decode_hashdata: scriptmode.%d for (%s)\n",scriptmode,strbuf);
            return(0);
        }
        //printf("EXPANDSCRIPT.(%c) -> [%s]\n",scriptmode,strbuf);
    }
    else if ( type == 't' )
    {
        /*if ( datalen > MAX_RAWTX_SPACE )
        {
            init_hexbytes_noT(strbuf,hashdata,64);
            printf("decode_hashdata: type.%d (%c) datalen.%d > sizeof(data) %d | (%s)\n",type,type,(int)datalen,MAX_RAWTX_SPACE,strbuf);
            exit(-1);
        }*/
        init_hexbytes_noT(strbuf,hashdata,datalen);
    }
    else if ( type == 'a' )
        memcpy(strbuf,hashdata,datalen);
    else
    {
        printf("decode_hashdata: unsupported type.%d (%c)\n",type,type);
        return(0);
        getchar();//exit(-1);
    }
    return(strbuf);
}

cJSON *_script_has_address(int32_t *nump,cJSON *scriptobj)
{
    int32_t i,n;
    cJSON *addresses,*addrobj;
    *nump = 0;
    if ( scriptobj == 0 )
    {
        printf("no scriptobj\n");
        return(0);
    }
    addresses = cJSON_GetObjectItem(scriptobj,"addresses");
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

/*
 {
    "PrivateKey" : "308201130201010420cca2db1e12cd4b17b09edd638ae1349f58c7217c0f7d22e58c9c10c2b029653fa081a53081a2020101302c06072a8648ce3d0101022100fffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2f300604010004010704410479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8022100fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141020101a14403420004d8e8b5d8979c841668ce0f006eae6e502c3d8522e828ec0d574ef1b28b5b80f75236fdb332f2a2f1ea364850283328ef56387517df1e616bb9bdfb764988c190",
    "PublicKey" : "04d8e8b5d8979c841668ce0f006eae6e502c3d8522e828ec0d574ef1b28b5b80f75236fdb332f2a2f1ea364850283328ef56387517df1e616bb9bdfb764988c190"
}
￼
{
    "isvalid" : true,
    "address" : "RTvcjHngNQKXaK5NjPcH7Wssin7hvTkZ5m",
    "ismine" : false,
    "iscompressed" : false
}
 a15cedfe4b6b00291f99e89762e71b76c3f45bd211f36a132620a5735a19ef9d -> 
 {
 "value" : 1.00000000,
 "n" : 1,
 "scriptPubKey" : {
 "asm" : "OP_DUP OP_HASH160 cc863b881f35bbeca374b25d018d4e17dc4aa19b OP_EQUALVERIFY OP_CHECKSIG",
 "reqSigs" : 1,
 "type" : "pubkeyhash",
 "addresses" : [
 "RTvcjHngNQKXaK5NjPcH7Wssin7hvTkZ5m"
 ]
 }
 ripemd160(sha256(04d8e8b5d8979c841668ce0f006eae6e502c3d8522e828ec0d574ef1b28b5b80f75236fdb332f2a2f1ea364850283328ef56387517df1e616bb9bdfb764988c190)) -> sha256.196ee5c15db800db4cfebbe8be6d9ef666f839ef8dcdd36a81a358aae312720e -> 9f8b558621baa463e410860702742c260d3a89ee
 
 cc863b881f35bbeca374b25d018d4e17dc4aa19b


036b0ac4361f0058710840f9db9f85733e8014211d9fcc7930dd833aa098ed4d33",
30440220387a6dcdc95a44487c4a978a8a91a7834ee2294a8a10dda11750e21e8891776b02207a17dcf7ee082a1a9db385900d6abc76735397776ee3f74f1d4fa8177d9b2f0c01
*/

int32_t _extract_txvals(struct destbuf *coinaddr,struct destbuf *script,int32_t nohexout,cJSON *txobj)
{
    int32_t numaddresses;
    struct destbuf typestr;
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
                bitcoin_assembler(script->buf);
                //fprintf(stderr,"-> {%s}\n",script);
            }
        }
        if ( coinaddr->buf[0] == 0 )
        {
            copy_cJSON(&typestr,cJSON_GetObjectItem(scriptobj,"type"));
            if ( strcmp(typestr.buf,"pubkey") != 0 && strcmp(typestr.buf,"nonstandard") != 0 && strcmp(typestr.buf,"nulldata") != 0 )
                printf("missing addr? (%s)\n",cJSON_Print(txobj));//, getchar();
        }
        return(0);
    }
    return(-1);
}

char *bitcoind_passthru(char *coinstr,char *serverport,char *userpass,char *method,char *params)
{
    return(bitcoind_RPC(0,coinstr,serverport,userpass,method,params));
}
uint32_t _get_RTheight(double *lastmillip,char *coinstr,char *serverport,char *userpass,int32_t current_RTblocknum)
{
    char *retstr;
    cJSON *json;
    uint32_t height = 0;
    if ( milliseconds() > (*lastmillip + 1000) )
    {
        //printf("RTheight.(%s) (%s)\n",ram->name,ram->serverport);
        retstr = bitcoind_passthru(coinstr,serverport,userpass,"getinfo","");
        if ( retstr != 0 )
        {
            if ( (json= cJSON_Parse(retstr)) != 0 )
            {
                height = juint(json,"blocks");
                //printf("get_RTheight %u\n",height);
                free_json(json);
                *lastmillip = milliseconds();
            }
            free(retstr);
        }
    } else height = current_RTblocknum;
    return(height);
}

char *_get_transaction(char *coinstr,char *serverport,char *userpass,char *txidstr)
{
    char *rawtransaction=0,txid[4096];
    sprintf(txid,"[\"%s\", 1]",txidstr);
    //printf("get_transaction.(%s)\n",txidstr);
    rawtransaction = bitcoind_passthru(coinstr,serverport,userpass,"getrawtransaction",txid);
    return(rawtransaction);
}

uint64_t wait_for_txid(char *script,struct coin777 *coin,char *txidstr,int32_t vout,uint64_t recvamount,int32_t minconfirms,int32_t maxseconds)
{
    uint64_t value; char *rawtx,*jsonstr,*str; struct cointx_info *cointx; struct destbuf buf; cJSON *json;
    uint32_t unconf=0,i,n,starttime = (uint32_t)time(NULL);
    script[0] = 0;
    while ( 1 )
    {
        if ( unconf == 0 && (jsonstr= bitcoind_passthru(coin->name,coin->serverport,coin->userpass,"getrawmempool","")) != 0 )
        {
            //printf("rawmempool.(%s)\n",jsonstr);
            if ( (json= cJSON_Parse(jsonstr)) != 0 && is_cJSON_Array(json) != 0 && (n= cJSON_GetArraySize(json)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    copy_cJSON(&buf,jitem(json,i));
                    if ( strcmp(buf.buf,txidstr) == 0 )
                    {
                        unconf = 1;
                        printf("FOUND %s in unconfirmed\n",txidstr);
                        break;
                    }
                }
                free_json(json);
            }
            free(jsonstr);
        }
        printf("unconf.%d get.(%s)\n",unconf,txidstr);
        value = 0;
        if ( (rawtx= _get_transaction(coin->name,coin->serverport,coin->userpass,txidstr)) != 0 )
        {
            if ( (json= cJSON_Parse(rawtx)) != 0 )
            {
                if ( (str= jstr(json,"hex")) != 0 )
                {
                    if ( (cointx= _decode_rawtransaction(str,coin->mgw.oldtx_format)) != 0 )
                    {
                        strcpy(script,cointx->inputs[0].sigs);
                        if ( (value= cointx->outputs[vout].value) != recvamount )
                        {
                            printf("TXID amount mismatch (%s v%d) %.8f vs expected %.8f\n",txidstr,vout,dstr(cointx->outputs[vout].value),dstr(recvamount));
                        }
                        free(cointx);
                    }
                }
                free_json(json);
            }
            free(rawtx);
        }
        if ( value != 0 )
            break;
        fprintf(stderr,".");
        if ( maxseconds != 0 && starttime+maxseconds < time(NULL) )
            break;
        sleep(20);
    }
    return(value);
}

uint64_t ram_verify_txstillthere(char *coinstr,char *serverport,char *userpass,char *txidstr,int32_t vout)
{
    char *retstr = 0;
    cJSON *txjson,*voutsobj;
    int32_t numvouts;
    uint64_t value = 0;
    if ( (retstr= _get_transaction(coinstr,serverport,userpass,txidstr)) != 0 )
    {
        if ( (txjson= cJSON_Parse(retstr)) != 0 )
        {
            if ( (voutsobj= cJSON_GetObjectItem(txjson,"vout")) != 0 && is_cJSON_Array(voutsobj) != 0 && (numvouts= cJSON_GetArraySize(voutsobj)) > 0  && vout < numvouts )
                value = conv_cJSON_float(cJSON_GetArrayItem(voutsobj,vout),"value");
            free_json(txjson);
        } else printf("update_txid_infos parse error.(%s)\n",retstr);
        free(retstr);
    } else printf("error getting.(%s)\n",txidstr);
    return(value);
}

char *_get_blockhashstr(char *coinstr,char *serverport,char *userpass,uint32_t blocknum)
{
    char numstr[128],*blockhashstr=0;
    sprintf(numstr,"%u",blocknum);
    blockhashstr = bitcoind_passthru(coinstr,serverport,userpass,"getblockhash",numstr);
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
        blocktxt = bitcoind_passthru(coinstr,serverport,userpass,"getblock",buf);
        if ( blocktxt != 0 && blocktxt[0] != 0 && (json= cJSON_Parse(blocktxt)) != 0 && heightp != 0 )
            if ( (*heightp= juint(json,"height")) == 0 )
                *heightp = 0xffffffff;
        if ( flag != 0 && blockhashstr != 0 )
            free(blockhashstr);
        if ( blocktxt != 0 )
            free(blocktxt);
    }
    return(json);
}

void ram_clear_rawblock(struct rawblock *raw,int32_t totalflag)
{
    //struct rawvin { char txidstr[128]; uint16_t vout; };
    //struct rawvout { char coinaddr[128],script[1024]; uint64_t value; };
    //struct rawtx { uint16_t firstvin,numvins,firstvout,numvouts; char txidstr[128]; };
    int32_t i; long len; struct rawtx *tx;
    if ( totalflag != 0 )
    {
        uint8_t *ptr = (uint8_t *)raw;
        len = sizeof(*raw);
        while ( len > 0 )
        {
            memset(ptr,0,len < 1024*1024 ? len : 1024*1024);
            len -= 1024 * 1024;
            ptr += 1024 * 1024;
        }
    }
    else
    {
        raw->blocknum = raw->minted = raw->numtx = raw->numrawvins = raw->numrawvouts = 0;
        tx = raw->txspace;
        for (i=0; i<MAX_BLOCKTX; i++,tx++)
        {
            tx->txidstr[0] = tx->numvins = tx->numvouts = tx->firstvout = tx->firstvin = 0;
            raw->vinspace[i].txidstr[0] = 0, raw->vinspace[i].vout = 0xffff;
            raw->voutspace[i].coinaddr[0] = raw->voutspace[i].script[0] = 0, raw->voutspace[i].value = 0;
        }
    }
}

void _set_string(char type,char *dest,char *src,long max)
{
    static uint32_t count;
    if ( src == 0 || src[0] == 0 )
        sprintf(dest,"ffff");
    else if ( strlen(src) < max-1 )
        strcpy(dest,src);
    else
    {
        count++;
        printf("count.%d >>>>>>>>>>> len.%ld > max.%ld (%s)\n",count,strlen(src),max,src);
        sprintf(dest,"nonstandard");
    }
}

uint64_t rawblock_txvouts(struct rawblock *raw,struct rawtx *tx,char *coinstr,char *serverport,char *userpass,cJSON *voutsobj)
{
    cJSON *item;
    uint64_t value,total = 0;
    struct rawvout *v;
    int32_t i,numvouts = 0;
    struct destbuf coinaddr,script;
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
            _extract_txvals(&coinaddr,&script,1,item); // default to nohexout
            _set_string('a',v->coinaddr,coinaddr.buf,sizeof(v->coinaddr));
            _set_string('s',v->script,script.buf,sizeof(v->script));
        }
    } else printf("error with vouts\n");
    tx->numvouts = numvouts;
    return(total);
}

int32_t rawblock_txvins(struct rawblock *raw,struct rawtx *tx,char *coinstr,char *serverport,char *userpass,cJSON *vinsobj)
{
    cJSON *item;
    struct rawvin *v;
    int32_t i,numvins = 0;
    struct destbuf txidstr,coinbase;
    tx->firstvin = raw->numrawvins;
    if ( vinsobj != 0 && is_cJSON_Array(vinsobj) != 0 && (numvins= cJSON_GetArraySize(vinsobj)) > 0 && tx->firstvin+numvins < MAX_BLOCKTX )
    {
        for (i=0; i<numvins; i++,raw->numrawvins++)
        {
            item = cJSON_GetArrayItem(vinsobj,i);
            if ( numvins == 1  )
            {
                copy_cJSON(&coinbase,cJSON_GetObjectItem(item,"coinbase"));
                if ( strlen(coinbase.buf) > 1 )
                    return(0);
            }
            copy_cJSON(&txidstr,cJSON_GetObjectItem(item,"txid"));
            v = &raw->vinspace[raw->numrawvins];
            memset(v,0,sizeof(*v));
            v->vout = (int)get_cJSON_int(item,"vout");
            _set_string('t',v->txidstr,txidstr.buf,sizeof(v->txidstr));
        }
    } else printf("error with vins\n");
    tx->numvins = numvins;
    return(numvins);
}

uint64_t rawblock_txidinfo(struct rawblock *raw,struct rawtx *tx,char *coinstr,char *serverport,char *userpass,int32_t txind,char *txidstr,uint32_t blocknum)
{
    char *retstr = 0;
    cJSON *txjson;
    uint64_t total = 0;
    if ( strlen(txidstr) < sizeof(tx->txidstr)-1 )
        strcpy(tx->txidstr,txidstr);
    tx->numvouts = tx->numvins = 0;
    if ( (retstr= _get_transaction(coinstr,serverport,userpass,txidstr)) != 0 )
    {
        if ( (txjson= cJSON_Parse(retstr)) != 0 )
        {
            rawblock_txvins(raw,tx,coinstr,serverport,userpass,cJSON_GetObjectItem(txjson,"vin"));
            total = rawblock_txvouts(raw,tx,coinstr,serverport,userpass,cJSON_GetObjectItem(txjson,"vout"));
            free_json(txjson);
        } else printf("update_txid_infos parse error.(%s)\n",retstr);
        free(retstr);
    }
    else
    {
        if ( blocknum == 0 )
        {
            
        }
        else printf("error getting.(%s)\n",txidstr);
    }
    //printf("tx.%d: (%s) numvins.%d numvouts.%d (raw %d %d)\n",txind,tx->txidstr,tx->numvins,tx->numvouts,raw->numrawvins,raw->numrawvouts);
    return(total);
}

cJSON *_rawblock_txarray(uint32_t *blockidp,int32_t *numtxp,cJSON *blockjson)
{
    cJSON *txarray = 0;
    if ( blockjson != 0 )
    {
        *blockidp = juint(blockjson,"height");
        txarray = cJSON_GetObjectItem(blockjson,"tx");
        *numtxp = cJSON_GetArraySize(txarray);
    }
    return(txarray);
}

void rawblock_patch(struct rawblock *raw)
{
    int32_t txind,numtx,firstvin,firstvout;
    struct rawtx *tx;
    firstvin = firstvout = 0;
    if ( (numtx= raw->numtx) != 0 )
    {
        for (txind=0; txind<numtx; txind++)
        {
            tx = &raw->txspace[txind];
            tx->firstvin = firstvin, firstvin += tx->numvins;
            tx->firstvout = firstvout, firstvout += tx->numvouts;
        }
    }
    raw->numrawvouts = firstvout;
    raw->numrawvins = firstvin;
}

int32_t rawblock_load(struct rawblock *raw,char *coinstr,char *serverport,char *userpass,uint32_t blocknum)
{
    struct destbuf txidstr,mintedstr,tmp;
    cJSON *json,*txobj;
    uint32_t blockid;
    int32_t txind,n;
    uint64_t total = 0;
    ram_clear_rawblock(raw,0);
    //raw->blocknum = blocknum;
    //printf("_get_blockinfo.%d\n",blocknum);
    raw->minted = raw->numtx = raw->numrawvins = raw->numrawvouts = 0;
    if ( (json= _get_blockjson(0,coinstr,serverport,userpass,0,blocknum)) != 0 )
    {
        raw->blocknum = juint(json,"height");
        copy_cJSON(&tmp,cJSON_GetObjectItem(json,"hash")), safecopy(raw->blockhash,tmp.buf,sizeof(raw->blockhash));
        //fprintf(stderr,"%u: blockhash.[%s] ",blocknum,raw->blockhash);
        copy_cJSON(&tmp,cJSON_GetObjectItem(json,"merkleroot")), safecopy(raw->merkleroot,tmp.buf,sizeof(raw->merkleroot));
        //fprintf(stderr,"raw->merkleroot.[%s]\n",raw->merkleroot);
        raw->timestamp = (uint32_t)get_cJSON_int(cJSON_GetObjectItem(json,"time"),0);
        copy_cJSON(&mintedstr,cJSON_GetObjectItem(json,"mint"));
        if ( mintedstr.buf[0] == 0 )
            copy_cJSON(&mintedstr,cJSON_GetObjectItem(json,"newmint"));
        if ( mintedstr.buf[0] != 0 )
            raw->minted = (uint64_t)(atof(mintedstr.buf) * SATOSHIDEN);
        if ( (txobj= _rawblock_txarray(&blockid,&n,json)) != 0 && blockid == blocknum && n < MAX_BLOCKTX )
        {
            for (txind=0; txind<n; txind++)
            {
                copy_cJSON(&txidstr,cJSON_GetArrayItem(txobj,txind));
                //printf("block.%d txind.%d TXID.(%s)\n",blocknum,txind,txidstr);
                total += rawblock_txidinfo(raw,&raw->txspace[raw->numtx++],coinstr,serverport,userpass,txind,txidstr.buf,blocknum);
            }
        } else printf("error _get_blocktxarray for block.%d got %d, n.%d vs %d\n",blocknum,blockid,n,MAX_BLOCKTX);
        if ( raw->minted == 0 )
            raw->minted = total;
        free_json(json);
    } else printf("get_blockjson error parsing.(%s)\n",txidstr.buf);
    //printf("BLOCK.%d: block.%d numtx.%d minted %.8f rawnumvins.%d rawnumvouts.%d\n",blocknum,raw->blocknum,raw->numtx,dstr(raw->minted),raw->numrawvins,raw->numrawvouts);
    rawblock_patch(raw);
    return(raw->numtx);
}

char *dumpprivkey(char *coinstr,char *serverport,char *userpass,char *coinaddr)
{
    char args[1024];
    sprintf(args,"[\"%s\"]",coinaddr);
    return(bitcoind_passthru(coinstr,serverport,userpass,"dumpprivkey",args));
}

char *get_acct_coinaddr(char *coinaddr,char *coinstr,char *serverport,char *userpass,char *NXTaddr)
{
    char addr[128],*retstr;
    coinaddr[0] = 0;
    if ( strcmp(coinstr,"NXT") == 0 )
        return(0);
    sprintf(addr,"\"%s\"",NXTaddr);
    retstr = bitcoind_passthru(coinstr,serverport,userpass,"getaccountaddress",addr);
    //printf("get_acct_coinaddr.(%s) -> (%s)\n",NXTaddr,retstr);
    if ( retstr != 0 )
    {
        strcpy(coinaddr,retstr);
        free(retstr);
        return(coinaddr);
    }
    return(0);
}

int32_t get_pubkey(struct destbuf *pubkey,char *coinstr,char *serverport,char *userpass,char *coinaddr)
{
    char quotes[512],*retstr; int64_t len = 0; cJSON *json;
    if ( coinaddr[0] != '"' )
        sprintf(quotes,"\"%s\"",coinaddr);
    else safecopy(quotes,coinaddr,sizeof(quotes));
    if ( (retstr= bitcoind_passthru(coinstr,serverport,userpass,"validateaddress",quotes)) != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            copy_cJSON(pubkey,cJSON_GetObjectItem(json,"pubkey"));
            len = (int32_t)strlen(pubkey->buf);
            free_json(json);
        }
        //printf("get_pubkey.(%s) -> (%s)\n",retstr,pubkey);
        free(retstr);
    }
    return((int32_t)len);
}

cJSON *_get_localaddresses(char *coinstr,char *serverport,char *userpass)
{
    char *retstr;
    cJSON *json = 0;
    retstr = bitcoind_passthru(coinstr,serverport,userpass,"listaddressgroupings","");
    if ( retstr != 0 )
    {
        json = cJSON_Parse(retstr);
        free(retstr);
    }
    return(json);
}

#endif
#endif
