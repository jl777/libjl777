//
//  ramtokens.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_ramtokens_h
#define crypto777_ramtokens_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "huffstream.c"
#include "huff.c"
#include "init.c"
#include "blocks.c"

#endif
#else
#ifndef crypto777_ramtokens_c
#define crypto777_ramtokens_c

#ifndef crypto777_ramtokens_h
#define DEFINES_ONLY
#include "tokens.c"
#undef DEFINES_ONLY
#endif

uint64_t ram_extract_varint(HUFF *hp)
{
    uint8_t c; uint16_t s; uint32_t i; uint64_t varint = 0;
    hmemcpy(&c,0,hp,1);
    if ( c >= 0xfd )
    {
        if ( c == 0xfd )
            hmemcpy(&s,0,hp,2), varint = s;
        else if ( c == 0xfe )
            hmemcpy(&i,0,hp,4), varint = i;
        else hmemcpy(&varint,0,hp,8);
    } else varint = c;
    //printf("c.%d -> (%llu)\n",c,(long long)varint);
    return(varint);
}

int32_t ram_extract_varstr(uint8_t *data,HUFF *hp)
{
    uint8_t c,*ptr; uint16_t s; int32_t datalen; uint64_t varint = 0;
    hmemcpy(&c,0,hp,1), data[0] = c, ptr = &data[1];
    if ( c >= 0xfd )
    {
        if ( c == 0xfd )
            hmemcpy(&s,0,hp,2), memcpy(ptr,&s,2), datalen = s, ptr += 2;
        else if ( c == 0xfe )
            hmemcpy(&datalen,0,hp,4), memcpy(ptr,&datalen,4), ptr += 4;
        else hmemcpy(&varint,0,hp,8),  memcpy(ptr,&varint,8), datalen = (uint32_t)varint, ptr += 8;
    } else datalen = c;
    hmemcpy(ptr,0,hp,datalen);
    return((int32_t)((long)ptr - (long)data));
}

uint32_t ram_extractstring(char *hashstr,char type,struct ramchain_info *ram,int32_t selector,int32_t offset,HUFF *hp,int32_t format)
{
    uint8_t hashdata[8192],*hashdataptr;
    union ramtypes U;
    uint32_t rawind = 0;
    if ( format == 'V' )
    {
        ram_extract_varstr(hashdata,hp);
        if ( ram_decode_hashdata(hashstr,type,hashdata) == 0 )
        {
            printf("ram_extractstring.V t.(%c) decode_hashdata error\n",type);
            return(0);
        }
        rawind = ram_conv_hashstr(0,0,ram_gethash(ram,type),hashstr);
    }
    else
    {
        if ( format == 'B' )
            hdecode_varbits(&rawind,hp);
        else if ( format == 'H' )
        {
            fprintf(stderr,"not supported yet"), exit(-1);
            //ram_decode_huffcode(&U,ram,selector,offset);
            rawind = U.val32;
        }
        else printf("ram_extractstring illegal format.%d\n",format);
        //printf("type.%c rawind.%d hashdataptr.%p\n",type,rawind,ram_gethashdata(ram,type,rawind));
        if ( (hashdataptr = ram_gethashdata(ram,type,rawind)) == 0 || ram_decode_hashdata(hashstr,type,hashdataptr) == 0 )
            rawind = 0;
        //printf("(%d) ramextract string rawind.%d (%c) -> (%s)\n",rawind,format,type,hashstr);
    }
    return(rawind);
}

uint32_t ram_extractint(struct ramchain_info *ram,int32_t selector,int32_t offset,HUFF *hp,int32_t format)
{
    uint32_t i = 0;
    union ramtypes U;
    if ( format == 'B' )
        hdecode_varbits(&i,hp);
    else if ( format == 'H' )
    {
        fprintf(stderr,"not supported yet\n"); exit(-1);
        //ram_decode_huffcode(&U,ram,selector,offset);
        i = U.val32;
    } else printf("invalid format.%d\n",format);
    return(i);
}

uint16_t ram_extractshort(struct ramchain_info *ram,int32_t selector,int32_t offset,HUFF *hp,int32_t format)
{
    uint16_t s = 0;
    union ramtypes U;
    //fprintf(stderr,"s.%d: ",hp->bitoffset);
    if ( format == 'B' )
        hdecode_smallbits(&s,hp);
    else if ( format == 'H' )
    {
        fprintf(stderr,"not supported yet\n"); exit(-1);
        //ram_decode_huffcode(&U,ram,selector,offset);
        s = U.val16;
    } else printf("invalid format.%d\n",format);
    return(s);
}

uint64_t ram_extractlong(struct ramchain_info *ram,int32_t selector,int32_t offset,HUFF *hp,int32_t format)
{
    uint64_t x = 0;
    union ramtypes U;
    if ( format == 'B' )
        hdecode_valuebits(&x,hp);
    else if ( format == 'H' )
    {
        fprintf(stderr,"not supported yet\n"); exit(-1);
        //ram_decode_huffcode(&U,ram,selector,offset);
        x = U.val64;
    } else printf("invalid format.%d\n",format);
    return(x);
}

struct ramchain_token *ram_set_token_hashdata(struct ramchain_info *ram,char type,char *hashstr,uint32_t rawind)
{
    uint8_t data[4097],*hashdata;
    char strbuf[8192];
    struct ramchain_hashptr *ptr;
    struct ramchain_token *token = 0;
    int32_t datalen;
    if ( type == 'a' || type == 't' || type == 's' )
    {
        if ( hashstr == 0 )
        {
            if ( rawind == 0 || rawind == 0xffffffff )
            {
                printf("ram_set_token_hashdata no hashstr and rawind.%d\n",rawind); while ( 1 ) sleep(1);
                return(0);
            }
            hashstr = strbuf;
            if ( ram_conv_rawind(hashstr,ram,rawind,type) == 0 )
                rawind = 0;
            token->rawind = rawind;
            printf("ram_set_token converted rawind.%d -> (%c).(%s)\n",rawind,type,hashstr);
        }
        else if ( hashstr[0] == 0 )
            token = memalloc(&ram->Tmp,sizeof(*token));
        else if ( (hashdata= ram_encode_hashstr(&datalen,data,type,hashstr)) != 0 )
        {
            token = memalloc(&ram->Tmp,sizeof(*token) + datalen - sizeof(token->U));
            memcpy(token->U.hashdata,hashdata,datalen);
            token->numbits = (datalen << 3);
            if ( (ptr= ram_hashdata_search(1,ram_gethash(ram,type),hashdata,datalen)) != 0 )
                token->rawind = ptr->rawind;
            // printf(">>>>>> rawind.%d -> %d\n",rawind,token->rawind);
        } else printf("encode_hashstr error for (%c).(%s)\n",type,hashstr);
    }
    else //if ( destformat == 'B' )
    {
        token = memalloc(&ram->Tmp,sizeof(*token));
        token->numbits = (sizeof(rawind) << 3);
        if ( hashstr != 0 && hashstr[0] != 0 )
        {
            rawind = ram_conv_hashstr(0,0,ram_gethash(ram,type),hashstr);
            //printf("(%s) -> %d\n",hashstr,rawind);
        }
        token->rawind = rawind;
        //printf("<<<<<<<<<< rawind.%d -> %d\n",rawind,token->rawind);
    }
    return(token);
}

void ram_sprintf_number(char *hashstr,union ramtypes *U,char type)
{
    switch ( type )
    {
        case 8: sprintf(hashstr,"%.8f",dstr(U->val64)); break;
        case -8: sprintf(hashstr,"%.14f",U->dval); break;
        case -4: sprintf(hashstr,"%.10f",U->fval); break;
        case 4: sprintf(hashstr,"%u",U->val32); break;
        case 2: sprintf(hashstr,"%u",U->val16); break;
        case 1: sprintf(hashstr,"%u",U->val8); break;
        default: sprintf(hashstr,"invalid type error.%d",type);
    }
}

struct ramchain_token *ram_createtoken(struct ramchain_info *ram,char selector,uint16_t offset,char type,char *hashstr,uint32_t rawind,union ramtypes *U,int32_t datalen)
{
    char strbuf[128];
    struct ramchain_token *token;
    switch ( type )
    {
        case 'a': case 't': case 's':
            //printf("%c.%d: (%c) token.(%s) rawind.%d\n",selector,offset,type,hashstr,rawind);
            token = ram_set_token_hashdata(ram,type,hashstr,rawind);
            break;
        case 16: case 8: case -8: case 4: case -4: case 2: case 1:
            token = memalloc(&ram->Tmp,sizeof(*token));
            /*if ( destformat == '*' )
             {
             token->numbits = ram_huffencode(&token->U.val64,ram,token,U,token->numbits);
             token->ishuffcode = 1;
             }
             else*/
        {
            token->U = *U;
            token->numbits = ((type >= 0) ? type : -type) << 3;
        }
            memcpy(&token->rawind,U->hashdata,sizeof(token->rawind));
            if ( hashstr == 0 )
                hashstr = strbuf;
            ram_sprintf_number(hashstr,U,type);
            //printf("%c.%d: (%d) token.%llu rawind.%d\n",selector,offset,type,(long long)token->U.val64,rawind);
            break;
        default: printf("ram_createtoken: illegal tokentype.%d\n",type); return(0); break;
    }
    if ( token != 0 )
    {
        token->selector = selector;
        token->offset = offset;
        token->type = type;
        //fprintf(stderr,"{%c.%c.%d %03u (%s)} ",token->type>16?token->type : '0'+token->type,token->selector,token->offset,token->rawind&0xffff,hashstr);
    }  //else fprintf(stderr,"{%c.%c.%d %03d ERR } ",type>16?type : '0'+type,selector,offset,rawind);
    return(token);
}

void *ram_tokenstr(void *longspacep,int32_t *datalenp,struct ramchain_info *ram,char *hashstr,struct ramchain_token *token)
{
    union ramtypes U;
    char type;
    uint32_t rawind = 0xffffffff;
    hashstr[0] = 0;
    type = token->type;
    if ( type == 'a' || type == 't' || type == 's' )
    {
        if ( token->ishuffcode != 0 )
        {
            fprintf(stderr,"not supported yet\n"); exit(-1);
            //ram_decode_huffcode(&U,ram,token->selector,token->offset);
            rawind = U.val32;
        }
        else rawind = token->rawind;
        if ( rawind != 0xffffffff && rawind != 0 && ram_conv_rawind(hashstr,ram,rawind,type) == 0 )
            rawind = 0;
        if ( ram_decode_hashdata(hashstr,token->type,token->U.hashdata) == 0 )
        {
            *datalenp = 0;
            printf("ram_expand_token decode_hashdata error\n");
            return(0);
        }
        *datalenp = (int32_t)(strlen(hashstr) + 1);
        return(hashstr);
    }
    else
    {
        if ( token->ishuffcode != 0 )
        {
            fprintf(stderr,"not supported yet\n"); exit(-1);
            //*datalenp = ram_decode_huffcode(&U,ram,token->selector,token->offset);
        }
        else
        {
            *datalenp = (token->numbits >> 3);
            U = token->U;
        }
        ram_sprintf_number(hashstr,&U,type);
        memcpy(longspacep,&U,*datalenp);
    }
    return(longspacep);
}

int32_t ram_rawvout_conv(HUFF *permhp,struct ramchain_info *ram,HUFF *hp,uint32_t blocknum,uint16_t txind,uint16_t vout,uint16_t numvouts)
{
    struct ramchain_hashtable *table;
    struct ramchain_hashptr *addrptr,*scriptptr;
    uint32_t scriptind,addrind;
    uint64_t value;
    int32_t numbits = 0;
    numbits += hdecode_varbits(&scriptind,hp);
    numbits += hdecode_varbits(&addrind,hp);
    table = ram_gethash(ram,'s');
    if ( scriptind > 0 && scriptind <= table->ind && (scriptptr= table->ptrs[scriptind]) != 0 )
    {
        hemit_varbits(permhp,scriptptr->permind);
        table = ram_gethash(ram,'a');
        if ( addrind > 0 && addrind <= table->ind && (addrptr= table->ptrs[addrind]) != 0 )
        {
            hemit_varbits(permhp,addrptr->permind);
            numbits += hdecode_valuebits(&value,hp), hemit_valuebits(permhp,value);
            return(numbits);
        } else printf("ram_rawvout_update block.%d txind.%d vout.%d can find addrind.%d ptr.%p\n",blocknum,txind,vout,addrind,addrptr);
    } else printf("ram_rawvout_update block.%d txind.%d vout.%d can find scriptind.%d\n",blocknum,txind,vout,scriptind);
    return(-1);
}

int32_t ram_rawvin_conv(HUFF *permhp,struct ramchain_info *ram,HUFF *hp,uint32_t blocknum,uint16_t txind,uint16_t vin,uint16_t numvins)
{
    struct ramchain_hashptr *txptr;
    struct ramchain_hashtable *table;
    uint32_t txid_rawind;
    int32_t numbits = 0;
    uint16_t vout;
    table = ram_gethash(ram,'t');
    numbits = hdecode_varbits(&txid_rawind,hp);
    if ( txid_rawind > 0 && txid_rawind <= table->ind )
    {
        numbits += hdecode_smallbits(&vout,hp);
        if ( (txptr= table->ptrs[txid_rawind]) != 0 && txptr->payloads != 0 )
        {
            if ( vout < txptr->numpayloads )
            {
                hemit_varbits(permhp,txptr->permind);
                hemit_smallbits(permhp,vout);
                return(numbits);
            }
            else printf("(%d %d %d) vout.%d overflows bp->v.%d\n",blocknum,txind,vin,vout,vin);
        } else printf("rawvin_update: unexpected null table->ptrs[%d] or no payloads.%p\n",txid_rawind,txptr->payloads);
    } else printf("txid_rawind.%u out of range %d\n",txid_rawind,table->ind);
    return(-1);
}

int32_t ram_rawtx_conv(HUFF *permhp,struct ramchain_info *ram,HUFF *hp,uint32_t blocknum,uint16_t txind)
{
    struct ramchain_hashptr *txptr;
    uint32_t txid_rawind = 0;
    int32_t i,retval,numbits = 0;
    uint16_t numvins,numvouts;
    struct ramchain_hashtable *table;
    table = ram_gethash(ram,'t');
    numbits += hdecode_smallbits(&numvins,hp), hemit_smallbits(permhp,numvins);
    numbits += hdecode_smallbits(&numvouts,hp), hemit_smallbits(permhp,numvouts);
    numbits += hdecode_varbits(&txid_rawind,hp);
    if ( txid_rawind > 0 && txid_rawind <= table->ind )
    {
        if ( (txptr= table->ptrs[txid_rawind]) != 0 )
        {
            hemit_varbits(permhp,txptr->permind);
            if ( numvins > 0 ) // alloc and update payloads in iter 0, no payload operations in iter 1
            {
                for (i=0; i<numvins; i++,numbits+=retval)
                    if ( (retval= ram_rawvin_conv(permhp,ram,hp,blocknum,txind,i,numvins)) < 0 )
                        return(-1);
            }
            if ( numvouts > 0 ) // just count number of payloads needed in iter 0, iter 1 allocates and updates
            {
                for (i=0; i<numvouts; i++,numbits+=retval)
                {
                    if ( (retval= ram_rawvout_conv(permhp,ram,hp,blocknum,txind,i,numvouts)) < 0 )
                        return(-2);
                }
            }
            return(numbits);
        }
    } else printf("ram_rawtx_conv: parse error\n");
    return(-3);
}

HUFF *ram_conv_permind(HUFF *permhp,struct ramchain_info *ram,HUFF *hp,uint32_t checkblocknum)
{
    uint64_t minted; uint16_t numtx; uint32_t blocknum; int32_t txind,numbits,retval,format;
    hrewind(hp);
    hclear(permhp,0);
    format = hp->buf[0], hp->ptr++, hp->bitoffset = 8;
    permhp->buf[0] = format, permhp->ptr++, permhp->endpos = permhp->bitoffset = 8;
    if ( format != 'B' )
    {
        printf("only format B supported for now\n");
        return(0);
    }
    numbits = hdecode_varbits(&blocknum,hp);
    if ( blocknum != checkblocknum )
    {
        printf("ram_conv_permind: hp->blocknum.%d vs checkblocknum.%d\n",blocknum,checkblocknum);
        //return(0);
        blocknum = checkblocknum;
    }
    hemit_varbits(permhp,blocknum);
    numbits += hdecode_smallbits(&numtx,hp), hemit_smallbits(permhp,numtx);
    numbits += hdecode_valuebits(&minted,hp), hemit_valuebits(permhp,minted);
    if ( numtx > 0 )
    {
        for (txind=0; txind<numtx; txind++,numbits+=retval)
            if ( (retval= ram_rawtx_conv(permhp,ram,hp,blocknum,txind)) < 0 )
            {
                printf("ram_conv_permind: blocknum.%d txind.%d parse error\n",blocknum,txind);
                return(0);
            }
    }
    //printf("hp.%d (end.%d bit.%d) -> permhp.%d (end.%d bit.%d)\n",datalen,hp->endpos,hp->bitoffset,hconv_bitlen(permhp->bitoffset),permhp->endpos,permhp->bitoffset);
    if ( ram->permfp != 0 )
        hsync(ram->permfp,permhp,(void *)ram->R3);
    return(permhp);
}

void ram_write_permentry(struct ramchain_hashtable *table,struct ramchain_hashptr *ptr)
{
    uint8_t databuf[8192];
    int32_t datalen,noexit,varlen = 0;
    uint64_t varint;
    long fpos,len;
    if ( table->permfp != 0 )
    {
        varlen += hdecode_varint(&varint,ptr->hh.key,0,9);
        datalen = ((int32_t)varint + varlen);
        fpos = ftell(table->permfp);
        if ( table->endpermpos < (fpos + datalen) )
        {
            fseek(table->permfp,0,SEEK_END);
            table->endpermpos = ftell(table->permfp);
            fseek(table->permfp,fpos,SEEK_SET);
        }
        if ( (fpos + datalen) <= table->endpermpos )
        {
            noexit = 0;
            if ( datalen < sizeof(databuf) )
            {
                if ( (len= fread(databuf,1,datalen,table->permfp)) == datalen )
                {
                    if ( memcmp(databuf,ptr->hh.key,datalen) != 0 )
                    {
                        printf("ram_write_permentry: memcmp error in permhash datalen.%d\n",datalen);
                        fseek(table->permfp,fpos,SEEK_SET);
                        noexit = 1;
                    }
                    else return;
                } else printf("ram_write_permentry: len.%ld != datalen.%d\n",len,datalen);
            } else printf("datalen.%d too big for databuf[%ld]\n",datalen,sizeof(databuf));
            if ( noexit == 0 )
                exit(-1); // unrecoverable
        }
        if ( fwrite(ptr->hh.key,1,datalen,table->permfp) != datalen )
        {
            printf("ram_write_permentry: error saving type.%d ind.%d datalen.%d\n",table->type,ptr->permind,datalen);
            exit(-1);
        }
        fflush(table->permfp);
        table->endpermpos = ftell(table->permfp);
    }
}

int32_t ram_rawvout_update(int32_t iter,uint32_t *script_rawindp,uint32_t *addr_rawindp,struct rampayload *txpayload,struct ramchain_info *ram,HUFF *hp,uint32_t blocknum,uint16_t txind,uint16_t vout,uint16_t numvouts,uint32_t txid_rawind,int32_t isinternal)
{
    struct rampayload payload;
    struct ramchain_hashtable *table;
    struct ramchain_hashptr *addrptr,*scriptptr;
    //struct rawvout_huffs *pair;
    uint32_t scriptind,addrind;
    struct address_entry B;
    char coinaddr[1024],txidstr[512],scriptstr[512]; //*str,
    uint64_t value;
    int32_t unspendable,numbits = 0;
    *addr_rawindp = 0;
    numbits += hdecode_varbits(&scriptind,hp);
    numbits += hdecode_varbits(&addrind,hp);
    numbits += hdecode_valuebits(&value,hp);
    if ( toupper(iter) == 'H' )
    {
        /*if ( vout == 0 )
         pair = &ram->H.vout0, str = "vout0";
         else if ( vout == (numvouts - 1) )
         pair = &ram->H.voutn, str = "voutn";
         else if ( vout == 1 )
         pair = &ram->H.vout1, str = "vout1";
         else if ( vout == 2 )
         pair = &ram->H.vout2, str = "vout2";
         else pair = &ram->H.vouti, str = "vouti";
         huffpair_update(iter,ram,str,"script",&pair->script,scriptind,4);
         huffpair_update(iter,ram,str,"addr",&pair->addr,addrind,4);
         huffpair_update(iter,ram,str,"value",&pair->value,value,8);*/
    }
    table = ram_gethash(ram,'s');
    if ( scriptind > 0 && scriptind <= table->ind && (scriptptr= table->ptrs[scriptind]) != 0 )
    {
        ram_script(scriptstr,ram,scriptind);
        scriptptr->unspent = ram_check_redeemcointx(&unspendable,ram->name,scriptstr,blocknum);
        if ( iter != 1 )
        {
            if ( scriptptr->unspent != 0 )  // this is MGW redeemtxid
            {
                ram_txid(txidstr,ram,txid_rawind);
                printf("coin redeemtxid.(%s) with script.(%s)\n",txidstr,scriptstr);
                memset(&B,0,sizeof(B));
                B.blocknum = blocknum, B.txind = txind, B.v = vout;
                _ram_update_redeembits(ram,scriptptr->unspent,0,txidstr,&B);
            }
            if ( scriptptr->permind == 0 )
            {
                scriptptr->permind = ++ram->next_script_permind;
                ram_write_permentry(table,scriptptr);
                if ( scriptptr->permind != scriptptr->rawind )
                    ram->permind_changes++;
            }
        }
        table = ram_gethash(ram,'a');
        if ( addrind > 0 && addrind <= table->ind && (addrptr= table->ptrs[addrind]) != 0 )
        {
            if ( iter != 1 && addrptr->permind == 0 )
            {
                addrptr->permind = ++ram->next_addr_permind;
                ram_write_permentry(table,addrptr);
                if ( addrptr->permind != addrptr->rawind )
                    ram->permind_changes++;
            }
            *addr_rawindp = addrind;
            *script_rawindp = scriptind;
            if ( txpayload == 0 )
                addrptr->maxpayloads++;
            else
            {
                if ( addrptr->payloads == 0 )
                {
                    addrptr->maxpayloads += 2;
                    //printf("ptr.%p alloc max.%d for (%d %d %d)\n",ptr,ptr->maxpayloads,blocknum,txind,vout);
                    addrptr->payloads = calloc(addrptr->maxpayloads,sizeof(struct rampayload));
                }
                if ( addrptr->numpayloads >= addrptr->maxpayloads )
                {
                    //printf("realloc max.%d for (%d %d %d) with num.%d\n",ptr->maxpayloads,blocknum,txind,vout,ptr->numpayloads);
                    addrptr->maxpayloads = (addrptr->numpayloads + 16);
                    addrptr->payloads = realloc(addrptr->payloads,addrptr->maxpayloads * sizeof(struct rampayload));
                }
                if ( iter <= 2 )
                {
                    memset(&payload,0,sizeof(payload));
                    payload.B.blocknum = blocknum, payload.B.txind = txind, payload.B.v = vout;
                    if ( value > 1 )
                        payload.B.isinternal = isinternal;
                    payload.otherind = txid_rawind, payload.extra = scriptind, payload.value = value;
                    //if ( ram_script_nonstandard(ram,scriptind) != 0 )
                    //    addrptr->nonstandard = 1;
                    //if ( ram_script_multisig(ram,scriptind) != 0 )
                    //    addrptr->multisig = 1;
                    if ( ram_addr(coinaddr,ram,addrind) != 0 && coinaddr[0] == ram->multisigchar )
                        addrptr->multisig = 1;
                    if ( unspendable == 0 )
                        ram_addunspent(ram,coinaddr,txpayload,addrptr,&payload,addrind,addrptr->numpayloads);
                    addrptr->payloads[addrptr->numpayloads++] = payload;
                }
            }
            return(numbits);
        } else printf("ram_rawvout_update block.%d txind.%d vout.%d can find addrind.%d ptr.%p\n",blocknum,txind,vout,addrind,addrptr);
    } else printf("ram_rawvout_update block.%d txind.%d vout.%d can find scriptind.%d\n",blocknum,txind,vout,scriptind);
    return(-1);
}

int32_t ram_rawvin_update(int32_t iter,struct ramchain_info *ram,HUFF *hp,uint32_t blocknum,uint16_t txind,uint16_t vin,uint16_t numvins,uint32_t spendtxid_rawind)
{
    static struct address_entry zeroB;
    struct address_entry B,*bp;
    struct ramchain_hashptr *txptr;
    struct ramchain_hashtable *table;
    //struct rawvin_huffs *pair;
    //char *str;
    uint32_t txid_rawind;
    int32_t numbits = 0;
    uint16_t vout;
    table = ram_gethash(ram,'t');
    numbits = hdecode_varbits(&txid_rawind,hp);
    if ( txid_rawind > 0 && txid_rawind <= table->ind )
    {
        numbits += hdecode_smallbits(&vout,hp);
        if ( iter == 0 )
            return(numbits);
        else if ( toupper(iter) == 'H' )
        {
            /*if ( vin == 0 )
             pair = &ram->H.vin0, str = "vin0";
             else if ( vin == 1 )
             pair = &ram->H.vin1, str = "vin1";
             else pair = &ram->H.vini, str = "vini";
             huffpair_update(iter,ram,str,"txid",&pair->txid,txid_rawind,4);
             huffpair_update(iter,ram,str,"vout",&pair->vout,vout,2);*/
        }
        if ( (txptr= table->ptrs[txid_rawind]) != 0 && txptr->payloads != 0 )
        {
            if ( iter != 1 && txptr->permind == 0 )
                printf("raw_rawvin_update: unexpected null permind for txid_rawind.%d in blocknum.%d txind.%d\n",txid_rawind,blocknum,txind);
            memset(&B,0,sizeof(B)), B.blocknum = blocknum, B.txind = txind, B.v = vin, B.spent = 1;
            if ( vout < txptr->numpayloads )
            {
                if ( iter <= 2 )
                {
                    bp = &txptr->payloads[vout].spentB;
                    if ( memcmp(bp,&zeroB,sizeof(zeroB)) == 0 )
                        ram_markspent(ram,&txptr->payloads[vout],&B,txid_rawind);
                    else if ( memcmp(bp,&B,sizeof(B)) == 0 )
                        printf("duplicate spentB (%d %d %d)\n",B.blocknum,B.txind,B.v);
                    else
                    {
                        printf("interloper.%u perm.%u at (blocknum.%d txind.%d vin.%d)! (%d %d %d).%d vs (%d %d %d).%d >>>>>>> delete? <<<<<<<<\n",txid_rawind,txptr->permind,blocknum,txind,vin,bp->blocknum,bp->txind,bp->v,bp->spent,B.blocknum,B.txind,B.v,B.spent);
                        //if ( getchar() == 'y' )
                        ram_purge_badblock(ram,bp->blocknum);
                        ram_purge_badblock(ram,blocknum);
                        exit(-1);
                    }
                }
                return(numbits);
            } else printf("(%d %d %d) vout.%d overflows bp->v.%d\n",blocknum,txind,vin,vout,B.v);
        } else printf("rawvin_update: unexpected null table->ptrs[%d] or no payloads.%p\n",txid_rawind,txptr->payloads);
    } else printf("txid_rawind.%u out of range %d\n",txid_rawind,table->ind);
    return(-1);
}

int32_t ram_rawtx_update(int32_t iter,struct ramchain_info *ram,HUFF *hp,uint32_t blocknum,uint16_t txind)
{
    struct rampayload payload;
    struct ramchain_hashptr *txptr;
    //char *str;
    struct address_entry B;
    uint32_t addr_rawind,script_rawind,txid_rawind = 0;
    int32_t i,internalvout,retval,isinternal,numbits = 0;
    uint16_t numvins,numvouts;
    //struct rawtx_huffs *pair;
    struct ramchain_hashtable *table;
    table = ram_gethash(ram,'t');
    numbits += hdecode_smallbits(&numvins,hp);
    numbits += hdecode_smallbits(&numvouts,hp);
    numbits += hdecode_varbits(&txid_rawind,hp);
    if ( toupper(iter) == 'H' )
    {
        /*if ( txind == 0 )
         pair = &ram->H.tx0, str = "tx0";
         else if ( txind == 1 )
         pair = &ram->H.tx1, str = "tx1";
         else pair = &ram->H.txi, str = "txi";
         huffpair_update(iter,ram,str,"numvins",&pair->numvins,numvins,2);
         huffpair_update(iter,ram,str,"numvouts",&pair->numvouts,numvouts,2);
         huffpair_update(iter,ram,str,"txid",&pair->txid,txid_rawind,4);*/
    }
    if ( txid_rawind > 0 && txid_rawind <= table->ind )
    {
        if ( (txptr= table->ptrs[txid_rawind]) != 0 )
        {
            if ( iter != 1 && txptr->permind == 0 )
            {
                txptr->permind = ++ram->next_txid_permind;
                ram_write_permentry(table,txptr);
                if ( txptr->permind != txptr->rawind )
                    ram->permind_changes++;
            }
            if ( iter == 0 || iter == 2 )
            {
                memset(&payload,0,sizeof(payload));
                if ( txptr->payloads != 0 )
                {
                    payload.B = txptr->payloads[0].B;
                    printf("%p txid_rawind.%d txid already there: (block.%d txind.%d)[%d] vs B.(%d %d %d)\n",txptr,txid_rawind,blocknum,txind,numvouts,payload.B.blocknum,payload.B.txind,payload.B.v);
                }
                else
                {
                    payload.B.blocknum = blocknum, payload.B.txind = txind;
                    txptr->numpayloads = numvouts;
                    //printf("%p txid_rawind.%d maxpayloads.%d numpayloads.%d (%d %d %d)\n",txptr,txid_rawind,txptr->maxpayloads,txptr->numpayloads,blocknum,txind,numvouts);
                    txptr->payloads = (SUPERNET.MAP_HUFF != 0) ? (struct rampayload *)permalloc(ram->name,&ram->Perm,txptr->numpayloads * sizeof(*txptr->payloads),7) : calloc(1,txptr->numpayloads * sizeof(*txptr->payloads));
                    for (payload.B.v=0; payload.B.v<numvouts; payload.B.v++)
                        txptr->payloads[payload.B.v] = payload;
                }
            }
            if ( numvins > 0 ) // alloc and update payloads in iter 0, no payload operations in iter 1
            {
                for (i=0; i<numvins; i++,numbits+=retval)
                    if ( (retval= ram_rawvin_update(iter,ram,hp,blocknum,txind,i,numvins,txid_rawind)) < 0 )
                        return(-1);
            }
            if ( numvouts > 0 ) // just count number of payloads needed in iter 0, iter 1 allocates and updates
            {
                internalvout = (numvouts - 1);
                memset(&B,0,sizeof(B));
                B.blocknum = blocknum, B.txind = txind;
                for (i=isinternal=0; i<numvouts; i++,numbits+=retval,B.v++)
                {
                    if ( (retval= ram_rawvout_update(iter,&script_rawind,&addr_rawind,iter==0?0:&txptr->payloads[i],ram,hp,blocknum,txind,i,numvouts,txid_rawind,isinternal*(i == 0 || i == internalvout))) < 0 )
                        return(-2);
                    if ( i == 0 && (addr_rawind == ram->marker_rawind || addr_rawind == ram->marker2_rawind) )
                        isinternal = 1;
                    /*else if ( isinternal != 0 && (numredeems= ram_is_MGW_OP_RETURN(redeemtxids,ram,script_rawind)) != 0 )
                     {
                     ram_txid(txidstr,ram,txid_rawind);
                     printf("found OP_RETURN.(%s)\n",txidstr);
                     internalvout = (i + 1);
                     for (j=0; j<numredeems; j++)
                     _ram_update_redeembits(ram,redeemtxids[j],0,txidstr,&B);
                     }*/
                }
            }
            return(numbits);
        }
    } else printf("ram_rawtx_update: parse error\n");
    return(-3);
}

bits256 ram_snapshot(struct ramsnapshot *snap,struct ramchain_info *ram,HUFF **hps,int32_t num)
{
    bits256 hash;
    memset(snap,0,sizeof(*snap));
    snap->addrind = ram->next_addr_permind;
    if ( ram->addrhash.permfp != 0 )
        snap->addroffset = ftell(ram->addrhash.permfp);
    snap->txidind = ram->next_txid_permind;
    if ( ram->txidhash.permfp != 0 )
        snap->txidoffset = ftell(ram->txidhash.permfp);
    snap->scriptind = ram->next_script_permind;
    if ( ram->scripthash.permfp != 0 )
        snap->scriptoffset = ftell(ram->scripthash.permfp);
    if ( ram->permfp != 0 )
        snap->permoffset = ftell(ram->permfp);
    ram_calcsha256(&hash,hps,num);
    return(hash);
}

int32_t ram_rawblock_update(int32_t iter,struct ramchain_info *ram,HUFF *hp,uint32_t checkblocknum)
{
    uint16_t numtx;
    uint64_t minted;
    uint32_t blocknum;
    int32_t txind,numbits,retval,format,datalen = 0;
    hrewind(hp);
    format = hp->buf[datalen++], hp->ptr++, hp->bitoffset = 8;
    if ( format != 'B' )
    {
        printf("only format B supported for now: (%c) %d not\n",format,format);
        return(-1);
    }
    numbits = hdecode_varbits(&blocknum,hp);
    if ( blocknum != checkblocknum )
    {
        printf("ram_rawblock_update: blocknum.%d vs checkblocknum.%d\n",blocknum,checkblocknum);
        return(-1);
    }
    if ( iter != 1 )
    {
        if ( blocknum != ram->S.permblocks ) //PERMUTE_RAWINDS != 0 &&
        {
            printf("ram_rawblock_update: blocknum.%d vs ram->S.permblocks.%d\n",blocknum,ram->S.permblocks);
            return(-1);
        }
        if ( (blocknum % 64) == 63 )
        {
            ram->snapshots[blocknum >> 6].hash = ram_snapshot(&ram->snapshots[blocknum >> 6],ram,&ram->blocks.hps[(blocknum >> 6) << 6],1<<6);
            if ( (blocknum % 4096) == 4095 )
                ram_calcsha256(&ram->permhash4096[blocknum >> 12],&ram->blocks.hps[(blocknum >> 12) << 12],1 << 12);
        }
        ram->S.permblocks++;
    }
    numbits += hdecode_smallbits(&numtx,hp);
    numbits += hdecode_valuebits(&minted,hp);
    if ( toupper(iter) == 'H' )
    {
        //huffpair_update(iter,ram,"","numtx",&ram->H.numtx,numtx,2);
        //huffpair_update(iter,ram,"minted",&ram->H.minted,minted,8);
    }
    if ( numtx > 0 )
    {
        for (txind=0; txind<numtx; txind++,numbits+=retval)
            if ( (retval= ram_rawtx_update(iter,ram,hp,blocknum,txind)) < 0 )
                return(-1);
    }
    datalen += hconv_bitlen(numbits);
    return(datalen);
}

void *ram_getrawdest(struct rawblock *raw,struct ramchain_token *token)
{
    int32_t txind;
    struct rawtx *tx;
    switch ( token->selector )
    {
        case 'B':
            if ( token->type == 4 )
                return(&raw->blocknum);
            else if ( token->type == 2 )
                return(&raw->numtx);
            else if ( token->type == 8 )
                return(&raw->minted);
            break;
        case 'T':
            txind = (token->offset >> 1);
            tx = &raw->txspace[txind];
            if ( token->type == 't' )
                return(tx->txidstr);
            else if ( (token->offset & 1) != 0 )
                return(&tx->numvouts);
            else if ( token->type == 2 )
                return(&tx->numvins);
            break;
        case 'I':
            if ( token->type == 't' )
                return(raw->vinspace[token->offset].txidstr);
            else return(&raw->vinspace[token->offset].vout);
            break;
        case 'O':
            if ( token->type == 's' )
                return(raw->voutspace[token->offset].script);
            else if ( token->type == 'a' )
                return(raw->voutspace[token->offset].coinaddr);
            else if ( token->type == 8 )
                return(&raw->voutspace[token->offset].value);
            break;
        default: printf("illegal token selector.%d\n",token->selector); return(0); break;
    }
    return(0);
}

int32_t ram_emit_token(int32_t compressflag,HUFF *outbits,struct ramchain_info *ram,struct ramchain_token *token)
{
    uint8_t *hashdata;
    int32_t i,bitlen,type;
    hashdata = token->U.hashdata;
    type = token->type;
    //printf("emit token.(%d) datalen.%d\n",token->type,outbits->bitoffset);
    if ( compressflag != 0 && type != 16 )
    {
        if ( type == 'a' || type == 't' || type == 's' )
        {
            if ( (token->numbits & 7) != 0 )
            {
                printf("misaligned token numbits.%d\n",token->numbits);
                return(-1);
            }
            return(hemit_varbits(outbits,token->rawind));
        }
        else
        {
            if ( type == 2 )
                return(hemit_smallbits(outbits,token->U.val16));
            else if ( type == 4 || type == -4 )
                return(hemit_varbits(outbits,token->U.val32));
            else if ( type == 8 || type == -8 )
                return(hemit_valuebits(outbits,token->U.val64));
            else return(hemit_smallbits(outbits,token->U.val8));
        }
    }
    bitlen = token->numbits;
    if ( (token->numbits & 7) == 0 && (outbits->bitoffset & 7) == 0 )
    {
        memcpy(outbits->ptr,hashdata,bitlen>>3);
        outbits->bitoffset += bitlen;
        hupdate_internals(outbits);
    }
    else
    {
        for (i=0; i<bitlen; i++)
            hputbit(outbits,GETBIT(hashdata,i));
    }
    return(bitlen);
}

void ram_patch_rawblock(struct rawblock *raw)
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

#define num_rawblock_tokens(raw) 3
#define num_rawtx_tokens(raw) 3
#define num_rawvin_tokens(raw) 2
#define num_rawvout_tokens(raw) 3

#define ram_createstring(ram,selector,offset,type,str,rawind) ram_createtoken(ram,selector,offset,type,str,rawind,0,0)
#define ram_createbyte(ram,selector,offset,rawind) ram_createtoken(ram,selector,offset,1,0,rawind,&U,1)
#define ram_createshort(ram,selector,offset,rawind) ram_createtoken(ram,selector,offset,2,0,rawind,&U,2)
#define ram_createint(ram,selector,offset,rawind) ram_createtoken(ram,selector,offset,4,0,rawind,&U,4)
#define ram_createfloat(ram,selector,offset,rawind) ram_createtoken(ram,selector,offset,-4,0,rawind,&U,4)
#define ram_createdouble(ram,selector,offset,rawind) ram_createtoken(ram,selector,offset,-8,0,rawind,&U,8)
#define ram_createlong(ram,selector,offset,rawind) ram_createtoken(ram,selector,offset,8,0,rawind,&U,8)

void raw_emitstr(HUFF *hp,char type,char *hashstr)
{
    uint8_t data[8192],*hashdata;
    int32_t i,numbits,datalen = 0;
    if ( (hashdata= ram_encode_hashstr(&datalen,data,type,hashstr)) != 0 )
    {
        numbits = (datalen << 3);
        for (i=0; i<numbits; i++)
            hputbit(hp,GETBIT(hashdata,i));
    }
}

void ram_rawvin_emit(HUFF *hp,struct rawvin *vi)
{
    raw_emitstr(hp,'t',vi->txidstr);
    hemit_bits(hp,vi->vout,sizeof(vi->vout));
}

void ram_rawvout_emit(HUFF *hp,struct rawvout *vo)
{
    raw_emitstr(hp,'s',vo->script);
    raw_emitstr(hp,'a',vo->coinaddr);
    hemit_longbits(hp,vo->value);
}

int32_t ram_rawtx_emit(HUFF *hp,struct rawblock *raw,struct rawtx *tx)
{
    int32_t i,numvins,numvouts;
    hemit_bits(hp,tx->numvins,sizeof(tx->numvins));
    hemit_bits(hp,tx->numvouts,sizeof(tx->numvouts));
    raw_emitstr(hp,'t',tx->txidstr);
    if ( (numvins= tx->numvins) > 0 )
        for (i=0; i<numvins; i++)
            ram_rawvin_emit(hp,&raw->vinspace[tx->firstvin + i]);
    if ( (numvouts= tx->numvouts) > 0 )
        for (i=0; i<numvouts; i++)
            ram_rawvout_emit(hp,&raw->voutspace[tx->firstvout + i]);
    return(0);
}

int32_t ram_rawblock_emit(HUFF *hp,struct ramchain_info *ram,struct rawblock *raw)
{
    int32_t txind,n;
    ram_patch_rawblock(raw);
    hrewind(hp);
    hemit_bits(hp,'V',8);
    hemit_bits(hp,raw->blocknum,sizeof(raw->blocknum));
    hemit_bits(hp,raw->numtx,sizeof(raw->numtx));
    hemit_longbits(hp,raw->minted);
    if ( (n= raw->numtx) > 0 )
    {
        for (txind=0; txind<n; txind++)
            ram_rawtx_emit(hp,raw,&raw->txspace[txind]);
    }
    return(hconv_bitlen(hp->bitoffset));
}

int32_t ram_rawvin_scan(struct ramchain_info *ram,struct ramchain_token **tokens,int32_t numtokens,struct rawblock *raw,struct rawtx *tx,int32_t vin)
{
    struct rawvin *vi;
    union ramtypes U;
    int32_t numrawvins;
    uint32_t txid_rawind,rawind = 0;
    numrawvins = (tx->firstvin + vin);
    vi = &raw->vinspace[numrawvins];
    if ( tokens != 0 )
    {
        if ( (txid_rawind= ram_txidind(ram,vi->txidstr)) != 0 )
            tokens[numtokens++] = ram_createstring(ram,'I',numrawvins,'t',vi->txidstr,txid_rawind);
        else tokens[numtokens++] = 0;
        memset(&U,0,sizeof(U)), U.val16 = vi->vout, tokens[numtokens++] = ram_createshort(ram,'I',numrawvins,rawind);
    }
    else numtokens += num_rawvin_tokens(raw);
    return(numtokens);
}

int32_t ram_rawvout_scan(struct ramchain_info *ram,struct ramchain_token **tokens,int32_t numtokens,struct rawblock *raw,struct rawtx *tx,int32_t vout)
{
    struct rawvout *vo;
    union ramtypes U;
    int numrawvouts;
    uint32_t addrind,scriptind,rawind = 0;
    numrawvouts = (tx->firstvout + vout);
    vo = &raw->voutspace[numrawvouts];
    if ( tokens != 0 )
    {
        if ( (scriptind= ram_scriptind(ram,vo->script)) != 0 )
            tokens[numtokens++] = ram_createstring(ram,'O',numrawvouts,'s',vo->script,scriptind);
        else tokens[numtokens++] = 0;
        if ( (addrind= ram_addrind(ram,vo->coinaddr)) != 0 )
            tokens[numtokens++] = ram_createstring(ram,'O',numrawvouts,'a',vo->coinaddr,addrind);
        else tokens[numtokens++] = 0;
        U.val64 = vo->value, tokens[numtokens++] = ram_createlong(ram,'O',numrawvouts,rawind);
    } else numtokens += num_rawvout_tokens(raw);
    return(numtokens);
}

int32_t ram_rawtx_scan(struct ramchain_info *ram,struct ramchain_token **tokens,int32_t numtokens,struct rawblock *raw,int32_t txind)
{
    struct rawtx *tx;
    union ramtypes U;
    uint32_t txid_rawind,rawind = 0;
    int32_t i,numvins,numvouts;
    tx = &raw->txspace[txind];
    if ( tokens != 0 )
    {
        memset(&U,0,sizeof(U)), U.val16 = tx->numvins, tokens[numtokens++] = ram_createshort(ram,'T',(txind<<1) | 0,rawind);
        U.val16 = tx->numvouts, tokens[numtokens++] = ram_createshort(ram,'T',(txind<<1) | 1,rawind);
        if ( (txid_rawind= ram_txidind(ram,tx->txidstr)) != 0 )
            tokens[numtokens++] = ram_createstring(ram,'T',(txind<<1),'t',tx->txidstr,txid_rawind);
        else tokens[numtokens++] = 0;
    } else numtokens += num_rawtx_tokens(raw);
    if ( (numvins= tx->numvins) > 0 )
    {
        for (i=0; i<numvins; i++)
            numtokens = ram_rawvin_scan(ram,tokens,numtokens,raw,tx,i);
    }
    if ( (numvouts= tx->numvouts) > 0 )
    {
        for (i=0; i<numvouts; i++)
            numtokens = ram_rawvout_scan(ram,tokens,numtokens,raw,tx,i);
    }
    return(numtokens);
}

struct ramchain_token **ram_tokenize_rawblock(int32_t *numtokensp,struct ramchain_info *ram,struct rawblock *raw)
{ // parse structure full of gaps
    union ramtypes U;
    uint32_t rawind = 0;
    int32_t i,n,maxtokens,numtokens = 0;
    struct ramchain_token **tokens = 0;
    maxtokens = num_rawblock_tokens(raw) + (raw->numtx * num_rawtx_tokens(raw)) + (raw->numrawvins * num_rawvin_tokens(raw)) + (raw->numrawvouts * num_rawvout_tokens(raw));
    clear_alloc_space(&ram->Tmp);
    tokens = memalloc(&ram->Tmp,maxtokens*sizeof(*tokens));
    ram_patch_rawblock(raw);
    if ( tokens != 0 )
    {
        memset(&U,0,sizeof(U)), U.val32 = raw->blocknum, tokens[numtokens++] = ram_createint(ram,'B',0,rawind);
        memset(&U,0,sizeof(U)), U.val16 = raw->numtx, tokens[numtokens++] = ram_createshort(ram,'B',0,rawind);
        U.val64 = raw->minted, tokens[numtokens++] = ram_createlong(ram,'B',0,rawind);
    } else numtokens += num_rawblock_tokens(raw);
    if ( (n= raw->numtx) > 0 )
    {
        for (i=0; i<n; i++)
            numtokens = ram_rawtx_scan(ram,tokens,numtokens,raw,i);
    }
    if ( numtokens > maxtokens )
        printf("numtokens.%d > maxtokens.%d\n",numtokens,maxtokens);
    *numtokensp = numtokens;
    return(tokens);
}

struct ramchain_token *ram_extract_and_tokenize(union ramtypes *Uptr,struct ramchain_info *ram,int32_t selector,int32_t offset,HUFF *hp,int32_t srcformat,int32_t size)
{
    int32_t i;
    union ramtypes U;
    memset(&U,0,sizeof(U));
    if ( srcformat == 'V' )
    {
        size <<= 3;
        if ( ((hp->bitoffset+size) >> 3) > hp->allocsize )
        {
            printf("hp->bitoffset.%d + size.%d >= allocsize.%d bytes.(%d)\n",hp->bitoffset,size,hp->allocsize<<3,hp->allocsize);
            return(0);
        }
        for (i=0; i<size; i++)
            if ( hgetbit(hp) != 0 )
                SETBIT(U.hashdata,i);
        size >>= 3;
        //fprintf(stderr,"[%llx].%d ",(long long)U.val64,hp->bitoffset);
    }
    else
    {
        if ( size == 2 )
            U.val16 = ram_extractshort(ram,selector,offset,hp,srcformat);
        else if ( size == 4 )
            U.val32 = ram_extractint(ram,selector,offset,hp,srcformat);
        else if ( size == 8 )
            U.val64 = ram_extractlong(ram,selector,offset,hp,srcformat);
        else printf("extract_and_tokenize illegalsize %d\n",size);
        //fprintf(stderr,"(%llx).%d ",(long long)U.val64,hp->bitoffset);
    }
    *Uptr = U;
    if ( size == 2 )
        return(ram_createshort(ram,selector,offset,0));
    else if ( size == 4 )
        return(ram_createint(ram,selector,offset,0));
    else if ( size == 8 )
        return(ram_createlong(ram,selector,offset,0));
    printf("bad place you are\n");
    exit(-1);
    return(0);
}

int32_t ram_rawvin_huffscan(struct ramchain_info *ram,struct ramchain_token **tokens,int32_t numtokens,HUFF *hp,int32_t format,int32_t numrawvins)
{
    char txidstr[4096];
    union ramtypes U;
    uint32_t txid_rawind,orignumtokens = numtokens;
    if ( tokens != 0 )
    {
        if ( (txid_rawind= ram_extractstring(txidstr,'t',ram,'I',numrawvins,hp,format)) != 0 )
            tokens[numtokens++] = ram_createstring(ram,'I',numrawvins,'t',txidstr,txid_rawind);
        else return(orignumtokens);
        if ( (tokens[numtokens++]= ram_extract_and_tokenize(&U,ram,'I',numrawvins,hp,format,sizeof(uint16_t))) == 0 )
        {
            printf("numrawvins.%d: txid_rawind.%d (%s) vout.%d\n",numrawvins,txid_rawind,txidstr,U.val16);
            while ( 1 )
                portable_sleep(1);
            return(orignumtokens);
        }
    }
    else numtokens += num_rawvin_tokens(raw);
    return(numtokens);
}

int32_t ram_rawvout_huffscan(struct ramchain_info *ram,struct ramchain_token **tokens,int32_t numtokens,HUFF *hp,int32_t format,int32_t numrawvouts)
{
    char scriptstr[4096],coinaddr[4096];
    uint32_t scriptind,addrind,orignumtokens = numtokens;
    union ramtypes U;
    if ( tokens != 0 )
    {
        if ( (scriptind= ram_extractstring(scriptstr,'s',ram,'O',numrawvouts,hp,format)) != 0 )
            tokens[numtokens++] = ram_createstring(ram,'O',numrawvouts,'s',scriptstr,scriptind);
        else return(orignumtokens);
        if ( (addrind= ram_extractstring(coinaddr,'a',ram,'O',numrawvouts,hp,format)) != 0 )
            tokens[numtokens++] = ram_createstring(ram,'O',numrawvouts,'a',coinaddr,addrind);
        else return(orignumtokens);
        if ( (tokens[numtokens++] = ram_extract_and_tokenize(&U,ram,'O',numrawvouts,hp,format,sizeof(uint64_t))) == 0 )
        {
            printf("numrawvouts.%d: scriptind.%d (%s) addrind.%d (%s) value.(%.8f)\n",numrawvouts,scriptind,scriptstr,addrind,coinaddr,dstr(U.val64));
            while ( 1 )
                portable_sleep(1);
            return(orignumtokens);
        }
    } else numtokens += num_rawvout_tokens(raw);
    return(numtokens);
}

int32_t ram_rawtx_huffscan(struct ramchain_info *ram,struct ramchain_token **tokens,int32_t numtokens,HUFF *hp,int32_t format,int32_t txind,int32_t *firstvinp,int32_t *firstvoutp)
{
    union ramtypes U;
    char txidstr[4096];
    uint32_t txid_rawind,lastnumtokens,orignumtokens;
    int32_t i,numvins = 0,numvouts = 0;
    orignumtokens = numtokens;
    if ( tokens != 0 )
    {
        tokens[numtokens++] = ram_extract_and_tokenize(&U,ram,'T',(txind<<1) | 0,hp,format,sizeof(uint16_t)), numvins = U.val16;
        tokens[numtokens++] = ram_extract_and_tokenize(&U,ram,'T',(txind<<1) | 1,hp,format,sizeof(uint16_t)), numvouts = U.val16;
        if ( tokens[numtokens-1] == 0 || tokens[numtokens-2] == 0 )
        {
            printf("txind.%d (%d %d) numvins.%d numvouts.%d (%s) txid_rawind.%d (%p %p)\n",txind,*firstvinp,*firstvoutp,numvins,numvouts,txidstr,txid_rawind,tokens[numtokens-1],tokens[numtokens-2]);
            while ( 1 )
                portable_sleep(1);
            return(orignumtokens);
        }
        if ( (txid_rawind= ram_extractstring(txidstr,'t',ram,'T',(txind<<1),hp,format)) != 0 )
            tokens[numtokens++] = ram_createstring(ram,'T',(txind<<1),'t',txidstr,txid_rawind);
        else
        {
            printf("error ram_extractstring(%s,'t',txid.%d,%c) hp bitoffset.%d of %d\n",txidstr,txind,format,hp->bitoffset,hp->endpos);
            return(orignumtokens);
        }
    } else numtokens += num_rawtx_tokens(raw);
    if ( numvins > 0 )
    {
        lastnumtokens = numtokens;
        for (i=0; i<numvins; i++,(*firstvinp)++,lastnumtokens=numtokens)
            if ( (numtokens= ram_rawvin_huffscan(ram,tokens,numtokens,hp,format,*firstvinp)) == lastnumtokens )
                return(orignumtokens);
    }
    if ( numvouts > 0 )
    {
        lastnumtokens = numtokens;
        for (i=0; i<numvouts; i++,(*firstvoutp)++,lastnumtokens=numtokens)
            if ( (numtokens= ram_rawvout_huffscan(ram,tokens,numtokens,hp,format,*firstvoutp)) == lastnumtokens )
                return(orignumtokens);
    }
    //printf("1st vout.%d vin.%d\n",*firstvoutp,*firstvinp);
    return(numtokens);
}

struct ramchain_token **ram_purgetokens(int32_t *numtokensp,struct ramchain_token **tokens,int32_t numtokens)
{
    if ( numtokensp != 0 )
        *numtokensp = -1;
    return(0);
}

struct ramchain_token **ram_tokenize_bitstream(uint32_t *blocknump,int32_t *numtokensp,struct ramchain_info *ram,HUFF *hp,int32_t format)
{
    // 'V' packed structure using varints and varstrs
    // 'B' bitstream using varbits and rawind substitution for strings
    // 'H' bitstream using huffman codes
    struct ramchain_token **tokens = 0;
    int32_t i,numtx = 0,firstvin,firstvout,maxtokens,numtokens = 0;
    maxtokens = MAX_BLOCKTX * 2;
    union ramtypes U;
    uint64_t minted = 0;
    uint32_t blocknum,lastnumtokens;
    *blocknump = 0;
    clear_alloc_space(&ram->Tmp);
    tokens = memalloc(&ram->Tmp,maxtokens * sizeof(*tokens));
    tokens[numtokens++] = ram_extract_and_tokenize(&U,ram,'B',0,hp,format,sizeof(uint32_t)), *blocknump = blocknum = U.val32;
    tokens[numtokens++] = ram_extract_and_tokenize(&U,ram,'B',0,hp,format,sizeof(uint16_t)), numtx = U.val16;
    tokens[numtokens++] = ram_extract_and_tokenize(&U,ram,'B',0,hp,format,sizeof(uint64_t)), minted = U.val64;
    if ( tokens[numtokens-1] == 0 || tokens[numtokens-2] == 0 || tokens[numtokens-3] == 0 )
    {
        printf("blocknum.%d numt.%d minted %.8f (%p %p %p)\n",*blocknump,numtx,dstr(minted),tokens[numtokens-1],tokens[numtokens-2],tokens[numtokens-3]);
        //while ( 1 )
        //    portable_sleep(1);
        return(ram_purgetokens(numtokensp,tokens,numtokens));
    }
    if ( numtx > 0 )
    {
        lastnumtokens = numtokens;
        for (i=firstvin=firstvout=0; i<numtx; i++)
            if ( (numtokens= ram_rawtx_huffscan(ram,tokens,numtokens,hp,format,i,&firstvin,&firstvout)) == lastnumtokens )
            {
                printf("block.%d parse error at token %d of %d | firstvin.%d firstvout.%d\n",blocknum,i,numtx,firstvin,firstvout);
                return(ram_purgetokens(numtokensp,tokens,numtokens));
            }
    }
    if ( numtokens > maxtokens )
        printf("numtokens.%d > maxtokens.%d\n",numtokens,maxtokens);
    *numtokensp = numtokens;
    return(tokens);
}

int32_t ram_verify(struct ramchain_info *ram,HUFF *hp,int32_t format)
{
    uint32_t blocknum = 0;
    int32_t checkformat,numtokens = 0;
    struct ramchain_token **tokens;
    hrewind(hp);
    checkformat = hp->buf[0], hp->ptr++, hp->bitoffset = 8;
    if ( checkformat != format )
        return(0);
    tokens = ram_tokenize_bitstream(&blocknum,&numtokens,ram,hp,format);
    if ( tokens != 0 )
    {
        ram_purgetokens(0,tokens,numtokens);
        return(blocknum);
    }
    return(-1);
}

int32_t ram_expand_token(struct rawblock *raw,struct ramchain_info *ram,struct ramchain_token *token)
{
    void *hashdata,*destptr;
    char hashstr[8192];
    int32_t datalen;
    uint64_t longspace;
    if ( (hashdata= ram_tokenstr(&longspace,&datalen,ram,hashstr,token)) != 0 )
    {
        if ( raw != 0 && (destptr= ram_getrawdest(raw,token)) != 0 )
        {
            //printf("[%ld] copy %d bytes to (%c.%c.%d)\n",(long)destptr-(long)raw,datalen,token->type>16?token->type:token->type+'0',token->selector,token->offset);
            memcpy(destptr,hashdata,datalen);
            return(datalen);
        }
        else printf("ram_expand_token: error finding destptr in rawblock\n");
    } else printf("ram_expand_token: error decoding token\n");
    return(-1);
}

int32_t ram_emit_and_free(int32_t compressflag,HUFF *hp,struct ramchain_info *ram,struct ramchain_token **tokens,int32_t numtokens)
{
    int32_t i;
    for (i=0; i<numtokens; i++)
        if ( tokens[i] != 0 )
            ram_emit_token(compressflag,hp,ram,tokens[i]);
    ram_purgetokens(0,tokens,numtokens);
    if ( 0 )
    {
        for (i=0; i<(hp->bitoffset>>3)+1; i++)
            printf("%02x ",hp->buf[i]);
        printf("emit_and_free: endpos.%d\n",hp->endpos);
    }
    return(hconv_bitlen(hp->endpos));
}

int32_t ram_expand_and_free(cJSON **jsonp,struct rawblock *raw,struct ramchain_info *ram,struct ramchain_token **tokens,int32_t numtokens,int32_t allocsize)
{
    int32_t i;
    ram_clear_rawblock(raw,0);
    for (i=0; i<numtokens; i++)
        if ( tokens[i] != 0 )
            ram_expand_token(raw,ram,tokens[i]);
    ram_purgetokens(0,tokens,numtokens);
    ram_patch_rawblock(raw);
    if ( jsonp != 0 )
        (*jsonp) = ram_rawblock_json(raw,allocsize);
    return(numtokens);
}

int32_t ram_compress(HUFF *hp,struct ramchain_info *ram,uint8_t *data,int32_t datalen)
{
    struct ramchain_token **tokens;
    int32_t numtokens;
    uint32_t format,blocknum;
    HUFF *srcbits;
    if ( hp != 0 )
    {
        hrewind(hp);
        hemit_bits(hp,'H',8);
        srcbits = hopen(ram->name,&ram->Perm,data,datalen,0);
        if ( hdecode_bits(&format,srcbits,8) != 8 || format != 'B' )
            printf("error hdecode_bits in ram_expand_rawinds format.%d != (%c) %d",format,'B','B');
        else if ( (tokens= ram_tokenize_bitstream(&blocknum,&numtokens,ram,srcbits,format)) != 0 )
        {
            hclose(srcbits);
            return(ram_emit_and_free(2,hp,ram,tokens,numtokens));
        } else printf("error tokenizing blockhex\n");
        hclose(srcbits);
    }
    return(-1);
}

int32_t ram_emitblock(HUFF *hp,int32_t destformat,struct ramchain_info *ram,struct rawblock *raw)
{
    struct ramchain_token **tokens;
    int32_t numtokens;
    if ( hp != 0 )
    {
        hrewind(hp);
        hemit_bits(hp,destformat,8);
        if ( (tokens= ram_tokenize_rawblock(&numtokens,ram,raw)) != 0 )
            return(ram_emit_and_free(destformat!='V',hp,ram,tokens,numtokens));
    }
    return(-1);
}

int32_t ram_expand_bitstream(cJSON **jsonp,struct rawblock *raw,struct ramchain_info *ram,HUFF *hp)
{
    struct ramchain_token **tokens;
    int32_t numtokens;
    uint32_t format,blocknum;
    ram_clear_rawblock(raw,0);
    if ( hp != 0 )
    {
        hrewind(hp);
        format = hp->buf[0], hp->ptr++, hp->bitoffset = 8;
        if ( 0 )
        {
            int i;
            for (i=0; i<=hp->endpos>>3; i++)
                printf("%02x ",hp->buf[i]);
            printf("(%c).%d\n",format,i);
        }
        if ( format != 'B' && format != 'V' && format != 'H' )
            printf("error hdecode_bits in ram_expand_rawinds format.%d != (%c/%c/%c) %d/%d/%d\n",format,'V','B','H','V','B','H');
        else if ( (tokens= ram_tokenize_bitstream(&blocknum,&numtokens,ram,hp,format)) != 0 )
            return(ram_expand_and_free(jsonp,raw,ram,tokens,numtokens,hp->allocsize));
        else printf("error expanding bitstream\n");
    }
    return(-1);
}

#endif
#endif
