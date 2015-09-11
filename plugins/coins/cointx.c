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
#ifndef crypto777_cointx_h
#define crypto777_cointx_h
#include <stdio.h>
#include <stdint.h>
#include <memory.h>
#include "../includes/cJSON.h"
#include "../utils/utils777.c"
#include "coins777.c"
#include "../common/system777.c"
//#include "gen1auth.c"
//#include "../coins/msig.c"

struct cointx_info *_decode_rawtransaction(char *hexstr,int32_t oldtx);
int32_t _emit_cointx(char *hexstr,long len,struct cointx_info *cointx,int32_t oldtx);
int32_t _validate_decoderawtransaction(char *hexstr,struct cointx_info *cointx,int32_t oldtx);
int32_t emit_cointx(bits256 *hash2,uint8_t *data,long max,struct cointx_info *cointx,int32_t oldtx,uint32_t hashtype);

void disp_cointx(struct cointx_info *cointx);

#endif
#else
#ifndef crypto777_cointx_c
#define crypto777_cointx_c

#ifndef crypto777_cointx_h
#define DEFINES_ONLY
#include "cointx.c"
#undef DEFINES_ONLY
#endif


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
    printf("{ %s/v%d (%s) }.s%d ",vin->tx.txidstr,vin->tx.vout,vin->sigs,vin->sequence);
}

void disp_cointx(struct cointx_info *cointx)
{
    int32_t i;
    printf("disp_cointx version.%u timestamp.%u nlocktime.%u numinputs.%u numoutputs.%u\n",cointx->version,cointx->timestamp,cointx->nlocktime,cointx->numinputs,cointx->numoutputs);
    for (i=0; i<cointx->numinputs; i++)
        disp_cointx_input(&cointx->inputs[i]);
    printf("-> ");
    for (i=0; i<cointx->numoutputs; i++)
        disp_cointx_output(&cointx->outputs[i]);
    printf("\n");
}

int32_t emit_cointxdata(uint8_t *data,long maxlen,struct cointx_info *cointx,int32_t oldtx)
{
    long i,offset = 0;
    offset = _emit_uint32(data,offset,cointx->version);
    if ( oldtx == 0 )
        offset = _emit_uint32(data,offset,cointx->timestamp);
    offset += hcalc_varint(&data[offset],cointx->numinputs);
    for (i=0; i<cointx->numinputs; i++)
    {
        if ( (offset= _emit_cointx_input(data,offset,&cointx->inputs[i])) > maxlen )
            return(-1);
    }
    offset += hcalc_varint(&data[offset],cointx->numoutputs);
    for (i=0; i<cointx->numoutputs; i++)
    {
        if ( (offset= _emit_cointx_output(data,offset,&cointx->outputs[i])) > maxlen )
            return(-1);
    }
    offset = _emit_uint32(data,offset,cointx->nlocktime);
    return((int32_t)offset);
}

int32_t _emit_cointx(char *hexstr,long len,struct cointx_info *cointx,int32_t oldtx)
{
    uint8_t *data; int32_t offset;
    data = calloc(1,len);
    offset = emit_cointxdata(data,len,cointx,oldtx);
    if ( offset < len/2-1 )
        init_hexbytes_noT(hexstr,data,(int32_t)offset);
    else hexstr[0] = 0, offset = -1;
    free(data);
    return(offset);
}

int32_t emit_cointx(bits256 *hash2,uint8_t *data,long max,struct cointx_info *cointx,int32_t oldtx,uint32_t hashtype)
{
    int32_t offset; bits256 hash;
    if ( (offset= emit_cointxdata(data,max,cointx,oldtx)) > max || offset < 0 )
        return(-1);
    if ( hashtype != 0 )
    {
        _emit_uint32(data,offset,hashtype);
        calc_sha256(0,hash.bytes,data,offset + sizeof(uint32_t));
        calc_sha256(0,hash2->bytes,hash.bytes,sizeof(*hash2));
    }
    return(offset);
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
        long hlen;
        if ( strlen(checkstr)+2 == (hlen= strlen(hexstr)) && hexstr[hlen-1] == hexstr[hlen-2] && hexstr[hlen-1] == '0' )
        {
            printf("hexstr has 2 extra '0', truncate\n");
            hexstr[hlen-2] = 0;
        }
        else
        {
            printf("_validate_decoderawtransaction: error: \n(%s) != \n(%s)\n",hexstr,checkstr);
            //strcpy(hexstr,checkstr);
            disp_cointx(cointx);
        }
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
        printf("_decode_rawtransaction: hexstr.(%s) too long %ld vs %ld || is_hexstr.%d || oddlen.%ld\n",hexstr,strlen(hexstr),sizeof(data)*2-1,is_hexstr(hexstr),(len & 1));
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

#endif
#endif
