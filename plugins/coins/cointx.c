//
//  cointx.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_cointx_h
#define crypto777_cointx_h
#include <stdio.h>
#include <stdint.h>
#include <memory.h>
#include "cJSON.h"
#include "utils777.c"
#include "coins777.c"
#include "system777.c"
#include "gen1auth.c"
#include "msig.c"

//#define MAX_BLOCKTX 0xffff
//struct rawvin { char txidstr[128]; uint16_t vout; };
//struct rawvout { char coinaddr[64],script[256]; uint64_t value; };
//struct rawtx { uint16_t firstvin,numvins,firstvout,numvouts; char txidstr[128]; };
char *_insert_OP_RETURN(char *rawtx,int32_t do_opreturn,int32_t replace_vout,uint64_t *redeems,int32_t numredeems,int32_t oldtx);
struct cointx_info *_decode_rawtransaction(char *hexstr,int32_t oldtx);
int32_t _emit_cointx(char *hexstr,long len,struct cointx_info *cointx,int32_t oldtx);
char *_createsignraw_json_params(char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx,char *rawbytes,char **privkeys,int32_t gatewayid,int32_t numgateways);
char *_createrawtxid_json_params(char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx,int32_t gatewayid,int32_t numgateways);
int32_t hcalc_varint(uint8_t *buf,uint64_t x);
long hdecode_varint(uint64_t *valp,uint8_t *ptr,long offset,long mappedsize);

#endif
#else
#ifndef crypto777_cointx_c
#define crypto777_cointx_c

#ifndef crypto777_cointx_h
#define DEFINES_ONLY
#include "cointx.c"
#undef DEFINES_ONLY
#endif

int32_t _map_msigaddr(char *redeemScript,char *coinstr,char *serverport,char *userpass,char *normaladdr,char *msigaddr,int32_t gatewayid,int32_t numgateways); //could map to rawind, but this is rarely called

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

cJSON *_create_privkeys_json_params(char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx,char **privkeys,int32_t numinputs,int32_t gatewayid,int32_t numgateways)
{
    int32_t allocflag,i,ret,nonz = 0;
    cJSON *array;
    char normaladdr[1024],redeemScript[4096];
    //printf("create privkeys %p numinputs.%d\n",privkeys,numinputs);
    if ( privkeys == 0 )
    {
        privkeys = calloc(numinputs,sizeof(*privkeys));
        for (i=0; i<numinputs; i++)
        {
            if ( (ret= _map_msigaddr(redeemScript,coinstr,serverport,userpass,normaladdr,cointx->inputs[i].coinaddr,gatewayid,numgateways)) >= 0 )
            {
                //fprintf(stderr,"(%s) -> (%s).%d ",normaladdr,normaladdr,i);
                if ( (privkeys[i]= dumpprivkey(coinstr,serverport,userpass,normaladdr)) == 0 )
                    printf("error getting privkey to (%s)\n",normaladdr);
            } else fprintf(stderr,"ret.%d for %d (%s)\n",ret,i,normaladdr);
        }
        allocflag = 1;
        //fprintf(stderr,"allocated\n");
    } else allocflag = 0;
    array = cJSON_CreateArray();
    for (i=0; i<numinputs; i++)
    {
        if ( privkeys[i] != 0 )
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

cJSON *_create_vins_json_params(char **localcoinaddrs,char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx,int32_t gatewayid,int32_t numgateways)
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
        if ( (ret= _map_msigaddr(redeemScript,coinstr,serverport,userpass,normaladdr,vin->coinaddr,gatewayid,numgateways)) >= 0 )
            cJSON_AddItemToObject(json,"redeemScript",cJSON_CreateString(redeemScript));
        else printf("ret.%d redeemScript.(%s) (%s) for (%s)\n",ret,redeemScript,normaladdr,vin->coinaddr);
        if ( localcoinaddrs != 0 )
            localcoinaddrs[i] = vin->coinaddr;
        cJSON_AddItemToArray(array,json);
    }
    return(array);
}

char *_createsignraw_json_params(char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx,char *rawbytes,char **privkeys,int32_t gatewayid,int32_t numgateways)
{
    char *paramstr = 0;
    cJSON *array,*rawobj,*vinsobj=0,*keysobj=0;
    rawobj = cJSON_CreateString(rawbytes);
    if ( rawobj != 0 )
    {
        vinsobj = _create_vins_json_params(0,coinstr,serverport,userpass,cointx,gatewayid,numgateways);
        if ( vinsobj != 0 )
        {
            keysobj = _create_privkeys_json_params(coinstr,serverport,userpass,cointx,privkeys,cointx->numinputs,gatewayid,numgateways);
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

char *_createrawtxid_json_params(char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx,int32_t gatewayid,int32_t numgateways)
{
    char *paramstr = 0;
    cJSON *array,*vinsobj,*voutsobj;
    vinsobj = _create_vins_json_params(0,coinstr,serverport,userpass,cointx,gatewayid,numgateways);
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

#endif
#endif
