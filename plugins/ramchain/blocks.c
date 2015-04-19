//
//  ramchainblocks.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_ramchainblocks_h
#define crypto777_ramchainblocks_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "bits777.c"
#include "system777.c"
#include "files777.c"
#include "huff.c"
#include "init.c"

#endif
#else
#ifndef crypto777_ramchainblocks_c
#define crypto777_ramchainblocks_c

#ifndef crypto777_ramchainblocks_h
#define DEFINES_ONLY
#include __BASE_FILE__
#undef DEFINES_ONLY
#endif

int32_t ram_get_blockoffset(struct mappedblocks *blocks,uint32_t blocknum)
{
    int32_t offset = -1;
    if ( blocknum >= blocks->firstblock )
    {
        offset = (blocknum - blocks->firstblock);
        if ( offset >= blocks->numblocks )
        {
            printf("(%d - %d) = offset.%d >= numblocks.%d for format.%d\n",blocknum,blocks->firstblock,offset,blocks->numblocks,blocks->format);
            offset = -1;
        }
    }
    return(offset);
}

HUFF **ram_get_hpptr(struct mappedblocks *blocks,uint32_t blocknum)
{
    int32_t offset = ram_get_blockoffset(blocks,blocknum);
    return((offset < 0) ? 0 : &blocks->hps[offset]);
}

struct mappedptr *ram_get_M(struct mappedblocks *blocks,uint32_t blocknum)
{
    int32_t offset = ram_get_blockoffset(blocks,blocknum);
    return((offset < 0) ? 0 : &blocks->M[offset]);
}

HUFF *ram_makehp(HUFF *tmphp,int32_t format,struct ramchain_info *ram,struct rawblock *tmp,int32_t blocknum)
{
    int32_t datalen;
    uint8_t *block;
    HUFF *hp = 0;
    hclear(tmphp,1);
    if ( (datalen= ram_emitblock(tmphp,format,ram,tmp)) > 0 )
    {
        //printf("ram_emitblock datalen.%d (%d) bitoffset.%d\n",datalen,hconv_bitlen(tmphp->endpos),tmphp->bitoffset);
        block = permalloc(ram->name,&ram->Perm,datalen,5);
        memcpy(block,tmphp->buf,datalen);
        hp = hopen(ram->name,&ram->Perm,block,datalen,0);
        hseek(hp,0,SEEK_END);
        //printf("ram_emitblock datalen.%d bitoffset.%d endpos.%d\n",datalen,hp->bitoffset,hp->endpos);
    } else printf("error emitblock.%d\n",blocknum);
    return(hp);
}

int32_t raw_blockcmp(struct rawblock *ref,struct rawblock *raw)
{
    int32_t i;
    if ( ref->numtx != raw->numtx )
    {
        printf("ref numtx.%d vs %d\n",ref->numtx,raw->numtx);
        return(-1);
    }
    if ( ref->numrawvins != raw->numrawvins )
    {
        printf("numrawvouts.%d vs %d\n",ref->numrawvins,raw->numrawvins);
        return(-2);
    }
    if ( ref->numrawvouts != raw->numrawvouts )
    {
        printf("numrawvouts.%d vs %d\n",ref->numrawvouts,raw->numrawvouts);
        return(-3);
    }
    if ( ref->numtx != 0 && memcmp(ref->txspace,raw->txspace,ref->numtx*sizeof(*raw->txspace)) != 0 )
    {
        struct rawtx *reftx,*rawtx;
        int32_t flag = 0;
        for (i=0; i<ref->numtx; i++)
        {
            reftx = &ref->txspace[i];
            rawtx = &raw->txspace[i];
            printf("1st.%d %d, %d %d (%s) vs 1st %d %d, %d %d (%s)\n",reftx->firstvin,reftx->firstvout,reftx->numvins,reftx->numvouts,reftx->txidstr,rawtx->firstvin,rawtx->firstvout,rawtx->numvins,rawtx->numvouts,rawtx->txidstr);
            flag = 0;
            if ( reftx->firstvin != rawtx->firstvin )
                flag |= 1;
            if ( reftx->firstvout != rawtx->firstvout )
                flag |= 2;
            if ( reftx->numvins != rawtx->numvins )
                flag |= 4;
            if ( reftx->numvouts != rawtx->numvouts )
                flag |= 8;
            if ( strcmp(reftx->txidstr,rawtx->txidstr) != 0 )
                flag |= 16;
            if ( flag != 0 )
                break;
        }
        if ( i != ref->numtx )
        {
            printf("flag.%d numtx.%d\n",flag,ref->numtx);
            //while ( 1 )
            //    portable_sleep(1);
            return(-4);
        }
    }
    if ( ref->numrawvins != 0 && memcmp(ref->vinspace,raw->vinspace,ref->numrawvins*sizeof(*raw->vinspace)) != 0 )
    {
        return(-5);
    }
    if ( ref->numrawvouts != 0 && memcmp(ref->voutspace,raw->voutspace,ref->numrawvouts*sizeof(*raw->voutspace)) != 0 )
    {
        struct rawvout *reftx,*rawtx;
        int32_t err = 0;
        for (i=0; i<ref->numrawvouts; i++)
        {
            reftx = &ref->voutspace[i];
            rawtx = &raw->voutspace[i];
            if ( strcmp(reftx->coinaddr,rawtx->coinaddr) != 0 || strcmp(reftx->script,rawtx->script) || reftx->value != rawtx->value )
                printf("%d of %d: (%s) (%s) %.8f vs (%s) (%s) %.8f\n",i,ref->numrawvouts,reftx->coinaddr,reftx->script,dstr(reftx->value),rawtx->coinaddr,rawtx->script,dstr(rawtx->value));
            if ( reftx->value != rawtx->value || strcmp(reftx->coinaddr,rawtx->coinaddr) != 0 || strcmp(reftx->script,rawtx->script) != 0 )
                err++;
        }
        if ( err != 0 )
        {
            printf("rawblockcmp error for vouts\n");
            getchar();
            return(-6);
        }
    }
    return(0);
}

void ram_clear_rawblock(struct rawblock *raw,int32_t totalflag)
{
    int32_t i;
    if ( totalflag != 0 )
        memset(raw,0,sizeof(*raw));
    else
    {
        raw->numtx = raw->numrawvins = raw->numrawvouts = 0;
        for (i=0; i<MAX_BLOCKTX; i++)
        {
            raw->txspace[i].txidstr[0] = 0;
            raw->vinspace[i].txidstr[0] = 0;
            raw->voutspace[i].script[0] = 0;
            raw->voutspace[i].coinaddr[0] = 0;
        }
    }
}

HUFF *ram_genblock(HUFF *tmphp,struct rawblock *tmp,struct ramchain_info *ram,int32_t blocknum,int32_t format,HUFF **prevhpp)
{
    HUFF *hp = 0;
    int32_t regenflag = 0;
    if ( format == 0 )
        format = 'V';
    if ( 0 && format == 'B' && prevhpp != 0 && (hp= *prevhpp) != 0 && strcmp(ram->name,"BTC") == 0 )
    {
        if ( ram_expand_bitstream(0,tmp,ram,hp) <= 0 )
        {
            char fname[1024],formatstr[16];
            ram_setformatstr(formatstr,'V');
            ram_setfname(fname,ram,blocknum,formatstr);
            //delete_file(fname,0);
            ram_setformatstr(formatstr,'B');
            ram_setfname(fname,ram,blocknum,formatstr);
            //delete_file(fname,0);
            regenflag = 1;
            printf("ram_genblock fatal error generating %s blocknum.%d\n",ram->name,blocknum);
            //exit(-1);
        }
        hp = 0;
    }
    if ( hp == 0 )
    {
        if ( _get_blockinfo(tmp,ram,blocknum) > 0 )
        {
            if ( tmp->blocknum != blocknum )
            {
                printf("WARNING: genblock.%c for %d: got blocknum.%d numtx.%d minted %.8f\n",format,blocknum,tmp->blocknum,tmp->numtx,dstr(tmp->minted));
                return(0);
            }
        } else printf("error _get_blockinfo.(%u)\n",blocknum);
    }
    hp = ram_makehp(tmphp,format,ram,tmp,blocknum);
    return(hp);
}

HUFF *ram_getblock(struct ramchain_info *ram,uint32_t blocknum)
{
    HUFF **hpp;
    if ( (hpp= ram_get_hpptr(&ram->blocks,blocknum)) != 0 )
        return(*hpp);
    return(0);
}

HUFF *ram_verify_Vblock(struct ramchain_info *ram,uint32_t blocknum,HUFF *hp)
{
    int32_t datalen,err;
    HUFF **hpptr;
    if ( hp == 0 && ((hpptr= ram_get_hpptr(&ram->Vblocks,blocknum)) == 0 || (hp = *hpptr) == 0) )
    {
        printf("verify_Vblock: no hp found for hpptr.%p %d\n",hpptr,blocknum);
        return(0);
    }
    if ( (datalen= ram_expand_bitstream(0,ram->Vblocks.R,ram,hp)) > 0 )
    {
        _get_blockinfo(ram->Vblocks.R2,ram,blocknum);
        if ( (err= raw_blockcmp(ram->Vblocks.R2,ram->Vblocks.R)) == 0 )
        {
            //printf("OK ");
            return(hp);
        } else printf("verify_Vblock.%d err.%d\n",blocknum,err);
    } else printf("ram_expand_bitstream returned.%d\n",datalen);
    return(0);
}

HUFF *ram_verify_Bblock(struct ramchain_info *ram,uint32_t blocknum,HUFF *Bhp)
{
    int32_t format,i,n;
    HUFF **hpp,*hp = 0,*retval = 0;
    cJSON *json;
    char *jsonstrs[2],fname[1024],strs[2][2];
    memset(strs,0,sizeof(strs));
    n = 0;
    memset(jsonstrs,0,sizeof(jsonstrs));
    for (format='V'; format>='B'; format-=('V'-'B'),n++)
    {
        strs[n][0] = format;
        ram_setfname(fname,ram,blocknum,strs[n]);
        hpp = ram_get_hpptr((format == 'V') ? &ram->Vblocks : &ram->Bblocks,blocknum);
        if ( format == 'B' && hpp != 0 && *hpp == 0 )
            hp = Bhp;
        else if ( hpp != 0 )
            hp = *hpp;
        if ( hp != 0 )
        {
            //fprintf(stderr,"\n%c: ",format);
            json = 0;
            ram_expand_bitstream(&json,(format == 'V') ? ram->Bblocks.R : ram->Bblocks.R2,ram,hp);
            if ( json != 0 )
            {
                jsonstrs[n] = cJSON_Print(json);
                free_json(json), json = 0;
            }
            if ( format == 'B' )
                retval = hp;
        } else hp = 0;
    }
    if ( jsonstrs[0] == 0 || jsonstrs[1] == 0 || strcmp(jsonstrs[0],jsonstrs[1]) != 0 )
    {
        if ( 1 && jsonstrs[0] != 0 && jsonstrs[1] != 0 )
        {
            if ( raw_blockcmp(ram->Bblocks.R,ram->Bblocks.R2) != 0 )
            {
                printf("(%s).V vs (%s).B)\n",jsonstrs[0],jsonstrs[1]);
                retval = 0;
            }
        }
    }
    for (i=0; i<2; i++)
        if ( jsonstrs[i] != 0 )
            free(jsonstrs[i]);
    return(retval);
}

uint32_t ram_create_block(int32_t verifyflag,struct ramchain_info *ram,struct mappedblocks *blocks,struct mappedblocks *prevblocks,uint32_t blocknum)
{
    char fname[1024],formatstr[16];
    FILE *fp;
    bits256 sha,refsha;
    HUFF *hp,**hpptr,**hps,**prevhps;
    int32_t i,n,numblocks,datalen = 0;
    ram_setformatstr(formatstr,blocks->format);
    prevhps = ram_get_hpptr(prevblocks,blocknum);
    ram_setfname(fname,ram,blocknum,formatstr);
    //printf("check create.(%s)\n",fname);
    if ( blocks->format == 'V' && (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
    {
        fclose(fp);
        // if ( verifyflag == 0 )
        return(0);
    }
    if ( 0 && blocks->format == 'V' )
    {
        if ( _get_blockinfo(blocks->R,ram,blocknum) > 0 )
        {
            if ( (fp= fopen("test","wb")) != 0 )
            {
                hp = blocks->tmphp;
                if ( ram_rawblock_emit(hp,ram,blocks->R) <= 0 )
                    printf("error ram_rawblock_emit.%d\n",blocknum);
                hflush(fp,hp);
                fclose(fp);
                for (i=0; i<(hp->bitoffset>>3); i++)
                    printf("%02x ",hp->buf[i]);
                printf("ram_rawblock_emit\n");
            }
            if ( (hp= ram_genblock(blocks->tmphp,blocks->R2,ram,blocknum,blocks->format,0)) != 0 )
            {
                if ( (fp= fopen(os_compatible_path(fname),"wb")) != 0 )
                {
                    hflush(fp,hp);
                    fclose(fp);
                    compare_files("test",fname);
                    for (i=0; i<(hp->bitoffset>>3); i++)
                        printf("%02x ",hp->buf[i]);
                    printf("ram_genblock\n");
                }
                hclose(hp);
            }
        } else printf("error _get_blockinfo.%d\n",blocknum);
        return(0);
    }
    else if ( blocks->format == 'V' || blocks->format == 'B' )
    {
        //printf("create %s %d\n",formatstr,blocknum);
        if ( (hpptr= ram_get_hpptr(blocks,blocknum)) != 0 )
        {
            if ( (hp= ram_genblock(blocks->tmphp,blocks->R,ram,blocknum,blocks->format,prevhps)) != 0 )
            {
                //printf("block.%d created.%c block.%d numtx.%d minted %.8f\n",blocknum,blocks->format,blocks->R->blocknum,blocks->R->numtx,dstr(blocks->R->minted));
                if ( (fp= fopen(os_compatible_path(fname),"wb")) != 0 )
                {
                    hflush(fp,hp);
                    fclose(fp);
                }
                if ( ram_verify(ram,hp,blocks->format) == blocknum )
                {
                    if ( verifyflag != 0 && ((blocks->format == 'B') ? ram_verify_Bblock(ram,blocknum,hp) : ram_verify_Vblock(ram,blocknum,hp)) == 0 )
                    {
                        printf("error creating %cblock.%d\n",blocks->format,blocknum), datalen = 0;
                        delete_file(fname,0), hclose(hp);
                    }
                    else
                    {
                        datalen = (1 + hp->allocsize);
                        //if ( blocks->format == 'V' )
                        fprintf(stderr," %s CREATED.%c block.%d datalen.%d | RT.%u lag.%d\n",ram->name,blocks->format,blocknum,datalen+1,ram->S.RTblocknum,ram->S.RTblocknum-blocknum);
                        //else fprintf(stderr,"%s.B.%d ",ram->name,blocknum);
                        if ( 0 && *hpptr != 0 )
                        {
                            hclose(*hpptr);
                            *hpptr = 0;
                            printf("OVERWRITE.(%s) size.%ld bitoffset.%d allocsize.%d\n",fname,ftell(fp),hp->bitoffset,hp->allocsize);
                        }
                        *hpptr = hp;
                        if ( blocks->format != 'V' && ram->blocks.hps[blocknum] == 0 )
                            ram->blocks.hps[blocknum] = hp;
                    }
                } //else delete_file(fname,0), hclose(hp);
            } else printf("genblock error %s (%c) blocknum.%u\n",ram->name,blocks->format,blocknum);
        } else printf("%s.%u couldnt get hpp\n",formatstr,blocknum);
    }
    else if ( blocks->format == 64 || blocks->format == 4096 )
    {
        n = blocks->format;
        for (i=0; i<n; i++)
            if ( prevhps[i] == 0 )
                break;
        if ( i == n )
        {
            ram_setfname(fname,ram,blocknum,formatstr);
            hps = ram_get_hpptr(blocks,blocknum);
            if ( ram_save_bitstreams(&refsha,fname,prevhps,n) > 0 )
                numblocks = ram_map_bitstreams(verifyflag,ram,blocknum,&blocks->M[(blocknum-blocks->firstblock) >> blocks->shift],&sha,hps,n,fname,&refsha);
        } else printf("%s prev.%d missing blockptr at %d (%d of %d)\n",ram->name,prevblocks->format,blocknum+i,i,n);
    }
    else
    {
        printf("illegal format to blocknum.%d create.%d\n",blocknum,blocks->format);
        return(0);
    }
    if ( datalen != 0 )
    {
        blocks->sum += datalen;
        blocks->count += (1 << blocks->shift);
    }
    return(datalen);
}

#endif
#endif
