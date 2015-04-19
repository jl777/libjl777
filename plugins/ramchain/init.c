//
//  raminit.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_raminit_h
#define crypto777_raminit_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "utils777.c"
#include "bits777.c"
#include "NXT777.c"
#include "huff.c"
#include "bitcoind.c"
#include "tokens.c"
#include "ramchain.c"
int32_t ram_calcsha256(bits256 *sha,HUFF *bitstreams[],int32_t num);
void ram_purge_badblock(struct ramchain_info *ram,uint32_t blocknum);
void ram_setfname(char *fname,struct ramchain_info *ram,uint32_t blocknum,char *str);
void ram_setformatstr(char *formatstr,int32_t format);
long *ram_load_bitstreams(struct ramchain_info *ram,bits256 *sha,char *fname,HUFF *bitstreams[],int32_t *nump);
int32_t ram_save_bitstreams(bits256 *refsha,char *fname,HUFF *bitstreams[],int32_t num);
int32_t ram_map_bitstreams(int32_t verifyflag,struct ramchain_info *ram,int32_t blocknum,struct mappedptr *M,bits256 *sha,HUFF *blocks[],int32_t num,char *fname,bits256 *refsha);
uint32_t ram_update_RTblock(struct ramchain_info *ram);

#endif
#else
#ifndef crypto777_raminit_c
#define crypto777_raminit_c

#ifndef crypto777_raminit_h
#define DEFINES_ONLY
#include __BASE_FILE__
#undef DEFINES_ONLY
#endif


void ram_setdirA(char *dirA,struct ramchain_info *ram)
{
#ifndef _WIN32
    sprintf(dirA,"%s/ramchains/%s/bitstream",ram->dirpath,ram->name);
#else
	sprintf(dirA,"%s\\ramchains\\%s\\bitstream",ram->dirpath,ram->name);
#endif
}

void ram_setdirB(int32_t mkdirflag,char *dirB,struct ramchain_info *ram,uint32_t blocknum)
{
    static char lastdirB[1024];
    char dirA[1024];
    int32_t i;
    blocknum %= (64 * 64 * 64);
    ram_setdirA(dirA,ram);
    i = blocknum / (64 * 64);
#ifndef _WIN32
    sprintf(dirB,"%s/%05x_%05x",dirA,i*64*64,(i+1)*64*64-1);
#else
    sprintf(dirB,"%s\\%05x_%05x",dirA,i*64*64,(i+1)*64*64-1);
#endif
    if ( mkdirflag != 0 && strcmp(dirB,lastdirB) != 0 )
    {
        ensure_dir(dirB);
        //printf("DIRB: (%s)\n",dirB);
        strcpy(lastdirB,dirB);
    }
}

void ram_setdirC(int mkdirflag,char *dirC,struct ramchain_info *ram,uint32_t blocknum)
{
    static char lastdirC[1024];
    char dirB[1024];
    int32_t i,j;
    blocknum %= (64 * 64 * 64);
    ram_setdirB(mkdirflag,dirB,ram,blocknum);
    i = blocknum / (64 * 64);
    j = (blocknum - (i * 64 * 64)) / 64;
#ifndef _WIN32
    sprintf(dirC,"%s/%05x_%05x",dirB,i*64*64 + j*64,i*64*64 + (j+1)*64 - 1);
#else
 	sprintf(dirC,"%s\\%05x_%05x",dirB,i*64*64 + j*64,i*64*64 + (j+1)*64 - 1);
#endif
    if ( mkdirflag != 0 && strcmp(dirC,lastdirC) != 0 )
    {
        ensure_dir(dirC);
        //printf("DIRC: (%s)\n",dirC);
        strcpy(lastdirC,dirC);
    }
}

void ram_setfname(char *fname,struct ramchain_info *ram,uint32_t blocknum,char *str)
{
    char dirC[1024];
    ram_setdirC(0,dirC,ram,blocknum);
#ifndef _WIN32
    sprintf(fname,"%s/%u.%s",dirC,blocknum,str);
#else
    sprintf(fname,"%s\\%u.%s",dirC,blocknum,str);
#endif
}

void ram_setformatstr(char *formatstr,int32_t format)
{
    if ( format == 'V' || format == 'B' || format == 'H' )
    {
        formatstr[1] = 0;
        formatstr[0] = format;
    }
    else sprintf(formatstr,"B%d",format);
}

int32_t ram_calcsha256(bits256 *sha,HUFF *bitstreams[],int32_t num)
{
    int32_t i;
    bits256 tmp;
    memset(sha,0,sizeof(*sha));
    for (i=0; i<num; i++)
    {
        //printf("i.%d %p\n",i,bitstreams[i]);
        if ( bitstreams[i] != 0 && bitstreams[i]->buf != 0 )
            calc_sha256cat(tmp.bytes,sha->bytes,sizeof(*sha),bitstreams[i]->buf,hconv_bitlen(bitstreams[i]->endpos)), *sha = tmp;
        else
        {
            printf("ram_calcsha256(%d): bitstreams[%d] == 0? %p\n",num,i,bitstreams[i]);
            return(-1);
        }
    }
    return(num);
}

int32_t ram_save_bitstreams(bits256 *refsha,char *fname,HUFF *bitstreams[],int32_t num)
{
    FILE *fp;
    int32_t i,len = -1;
    if ( (fp= fopen(os_compatible_path(fname),"wb")) != 0 )
    {
        if ( ram_calcsha256(refsha,bitstreams,num) < 0 )
        {
            fclose(fp);
            return(-1);
        }
        printf("saving %s num.%d %llx\n",os_compatible_path(fname),num,(long long)refsha->txid);
        if ( fwrite(&num,1,sizeof(num),fp) == sizeof(num) )
        {
            if ( fwrite(refsha,1,sizeof(*refsha),fp) == sizeof(*refsha) )
            {
                for (i=0; i<num; i++)
                {
                    if ( bitstreams[i] != 0 )
                        hflush(fp,bitstreams[i]);
                    else
                    {
                        printf("unexpected null bitstream at %d\n",i);
                        break;
                    }
                }
                if ( i == num )
                    len = (int32_t)ftell(fp);
            }
        }
        fclose(fp);
    }
    return(len);
}

long *ram_load_bitstreams(struct ramchain_info *ram,bits256 *sha,char *fname,HUFF *bitstreams[],int32_t *nump)
{
    FILE *fp;
    long *offsets = 0;
    bits256 tmp,stored;
    int32_t i,x = -1,len = 0;
    if ( (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
    {
        memset(sha,0,sizeof(*sha));
        //fprintf(stderr,"loading %s\n",fname);
        if ( fread(&x,1,sizeof(x),fp) == sizeof(x) && ((*nump) == 0 || x == (*nump)) )
        {
            if ( (*nump) == 0 )
            {
                (*nump) = x;
                //printf("set num to %d\n",x);
            }
            offsets = calloc((*nump),sizeof(*offsets));
            if ( fread(&stored,1,sizeof(stored),fp) == sizeof(stored) )
            {
                //fprintf(stderr,"reading %s num.%d stored.%llx\n",fname,*nump,(long long)stored.txid);
                for (i=0; i<(*nump); i++)
                {
                    if ( (bitstreams[i]= hload(ram,&offsets[i],fp,0)) != 0 && bitstreams[i]->buf != 0 )
                        calc_sha256cat(tmp.bytes,sha->bytes,sizeof(*sha),bitstreams[i]->buf,bitstreams[i]->allocsize), *sha = tmp;
                    else printf("unexpected null bitstream at %d %p offset.%ld\n",i,bitstreams[i],offsets[i]);
                }
                if ( memcmp(sha,&stored,sizeof(stored)) != 0 )
                    printf("sha error %s %llx vs stored.%llx\n",fname,(long long)sha->txid,(long long)stored.txid);
            } else printf("error loading sha\n");
        } else printf("num mismatch %d != num.%d\n",x,(*nump));
        len = (int32_t)ftell(fp);
        //fprintf(stderr," len.%d ",len);
        fclose(fp);
    }
    //fprintf(stderr," return offsets.%p \n",offsets);
    return(offsets);
}

int32_t ram_map_bitstreams(int32_t verifyflag,struct ramchain_info *ram,int32_t blocknum,struct mappedptr *M,bits256 *sha,HUFF *blocks[],int32_t num,char *fname,bits256 *refsha)
{
    HUFF *hp;
    long *offsets;
    uint32_t checkblock;
    int32_t i,n,retval = 0,verified=0,rwflag = 0;
    retval = n = 0;
    if ( (offsets= ram_load_bitstreams(ram,sha,fname,blocks,&num)) != 0 )
    {
        // fprintf(stderr,"offset.%p num.%d refsha.%p sha.%p M.%p\n",offsets,num,refsha,sha,M);
        if ( refsha != 0 && memcmp(sha->bytes,refsha,sizeof(*sha)) != 0 )
        {
            fprintf(stderr,"refsha cmp error for %s %llx vs %llx\n",fname,(long long)sha->txid,(long long)refsha->txid);
            hpurge(blocks,num);
            free(offsets);
            return(0);
        }
        //fprintf(stderr,"about clear M\n");
        if ( M->fileptr != 0 )
            close_mappedptr(M);
        memset(M,0,sizeof(*M));
        //fprintf(stderr,"about to init_mappedptr\n");
        if ( init_mappedptr(0,M,0,rwflag,fname) != 0 )
        {
            //fprintf(stderr,"opened (%s) filesize.%lld\n",fname,(long long)M->allocsize);
            for (i=0; i<num; i++)
            {
                if ( i > 0 && (i % 4096) == 0 )
                    fprintf(stderr,"%.1f%% ",100.*(double)i/num);
                if ( (hp= blocks[i]) != 0 )
                {
                    hp->buf = (void *)((long)M->fileptr + offsets[i]);
                    //if ( (blocks[i]= hopen(ram->name,&ram->Perm,(void *)((long)M->fileptr + offsets[i]),hp->allocsize,0)) != 0 )
                    {
                        if ( verifyflag == 0 || (checkblock= ram_verify(ram,hp,'B')) == blocknum+i )
                            verified++;
                        else
                        {
                            printf("checkblock.%d vs %d (blocknum.%d + i.%d)\n",checkblock,blocknum+i,blocknum,i);
                            break;
                        }
                    }
                } else printf("ram_map_bitstreams: ram_map_bitstreams unexpected null hp at slot.%d\n",i);
            }
            if ( i == num )
            {
                retval = (int32_t)M->allocsize;
                //printf("loaded.%d from %d\n",num,blocknum);
                for (i=0; i<num; i++)
                    if ( (hp= blocks[i]) != 0 && ram->blocks.hps[blocknum+i] == 0 )
                        ram->blocks.hps[blocknum+i] = hp, n++;
            }
            else
            {
                printf("%s: only %d of %d blocks verified\n",fname,verified,num);
                for (i=0; i<num; i++)
                    if ( (hp= blocks[i]) != 0 )
                        hclose(hp), blocks[i] = 0;
                close_mappedptr(M);
                memset(M,0,sizeof(*M));
                delete_file(fname,0);
            }
        } else printf("Error mapping.(%s)\n",fname);
        free(offsets);
    }
    //printf("mapped.%d from %d\n",n,blocknum);
    //clear_alloc_space(&ram->Tmp);
    return(n);
}

uint32_t ram_setcontiguous(struct mappedblocks *blocks)
{
    uint32_t i,n = blocks->firstblock;
    for (i=0; i<blocks->numblocks; i++)
    {
        if ( blocks->hps[i] != 0 )
            n++;
        else break;
    }
    return(n);
}

uint32_t ram_load_blocks(struct ramchain_info *ram,struct mappedblocks *blocks,uint32_t firstblock,int32_t numblocks)
{
    HUFF **hps;
    bits256 sha;
    struct mappedptr *M;
    char fname[1024],formatstr[16];
    uint32_t blocknum,i,flag,incr,n = 0;
    incr = (1 << blocks->shift);
    M = &blocks->M[0];
    ram_setformatstr(formatstr,blocks->format);
    flag = blocks->format == 'B';
    for (i=0; i<numblocks; i+=incr,M++)
    {
        blocknum = (firstblock + i);
        ram_setfname(fname,ram,blocknum,formatstr);
        //printf("loading (%s)\n",fname);
        if ( (hps= ram_get_hpptr(blocks,blocknum)) != 0 )
        {
            if ( blocks->format == 64 || blocks->format == 4096 )
            {
                if ( ram_map_bitstreams(flag,ram,blocknum,M,&sha,hps,incr,fname,0) <= 0 )
                {
                    //break;
                }
            }
            else
            {
                if ( (*hps= hload(ram,0,0,fname)) != 0 )
                {
                    if ( flag == 0 || ram_verify(ram,*hps,blocks->format) == blocknum )
                    {
                        //if ( flag != 0 )
                        //    fprintf(stderr,"=");
                        n++;
                        if ( blocks->format == 'B' && ram->blocks.hps[blocknum] == 0 )
                            ram->blocks.hps[blocknum] = *hps;
                    }
                    else hclose(*hps), *hps = 0;
                    if ( (n % 100) == 0 )
                        fprintf(stderr," total.%d loaded.(%s)\n",n,fname);
                } //else break;
            }
        }
    }
    blocks->contiguous = ram_setcontiguous(blocks);
    return(blocks->contiguous);
}

char *ram_blockstr(struct rawblock *tmp,struct ramchain_info *ram,struct rawblock *raw)
{
    cJSON *json = 0;
    char *retstr = 0;
    if ( (json= ram_rawblock_json(raw,0)) != 0 )
    {
        retstr = cJSON_Print(json);
        free_json(json);
    }
    return(retstr);
}

void ram_purge_badblock(struct ramchain_info *ram,uint32_t blocknum)
{
    char fname[1024];
    //ram_setfname(fname,ram,blocknum,"V");
    //delete_file(fname,0);
    ram_setfname(fname,ram,blocknum,"B");
    delete_file(fname,0);
    //ram_setfname(fname,ram,blocknum,"B64");
    //delete_file(fname,0);
    //ram_setfname(fname,ram,blocknum,"B4096");
    // delete_file(fname,0);
}

int32_t ram_getsources(uint64_t *sources,struct ramchain_info *ram,uint32_t blocknum,int32_t numblocks)
{
    int32_t i,n = 0;
    struct MGWstate S;
    for (i=0; i<(int)(sizeof(ram->remotesrcs)/sizeof(*ram->remotesrcs)); i++)
    {
        S = ram->remotesrcs[i];
        if ( S.nxt64bits == 0 )
            break;
        if ( S.permblocks >= (blocknum + numblocks) )
            sources[n++] = S.nxt64bits;
    }
    return(n);
}


// >>>>>>>>>>>>>>  start initialization and runloops
struct mappedblocks *ram_init_blocks(int32_t noload,HUFF **copyhps,struct ramchain_info *ram,uint32_t firstblock,struct mappedblocks *blocks,struct mappedblocks *prevblocks,int32_t format,int32_t shift)
{
    void *ptr;
    int32_t numblocks,tmpsize = TMPALLOC_SPACE_INCR;
    blocks->R = (MAP_HUFF != 0) ? permalloc(ram->name,&ram->Perm,sizeof(*blocks->R),8) : calloc(1,sizeof(*blocks->R));
    blocks->R2 = (MAP_HUFF != 0) ? permalloc(ram->name,&ram->Perm,sizeof(*blocks->R2),8) : calloc(1,sizeof(*blocks->R2));
    blocks->R3 = (MAP_HUFF != 0) ? permalloc(ram->name,&ram->Perm,sizeof(*blocks->R3),8) : calloc(1,sizeof(*blocks->R3));
    blocks->ram = ram;
    blocks->prevblocks = prevblocks;
    blocks->format = format;
    ptr = (MAP_HUFF != 0) ? permalloc(ram->name,&ram->Perm,tmpsize,8) : calloc(1,tmpsize), blocks->tmphp = hopen(ram->name,&ram->Perm,ptr,tmpsize,0);
    if ( (blocks->shift = shift) != 0 )
        firstblock &= ~((1 << shift) - 1);
    blocks->firstblock = firstblock;
    numblocks = (ram->maxblock+1) - firstblock;
    printf("initblocks.%d 1st.%d num.%d n.%d\n",format,firstblock,numblocks,numblocks>>shift);
    if ( numblocks < 0 )
    {
        printf("illegal numblocks %d with firstblock.%d vs maxblock.%d\n",blocks->numblocks,firstblock,ram->maxblock);
        exit(-1);
    }
    blocks->numblocks = numblocks;
    if ( blocks->hps == 0 )
        blocks->hps = (MAP_HUFF != 0) ? permalloc(ram->name,&ram->Perm,blocks->numblocks*sizeof(*blocks->hps),8) : calloc(1,blocks->numblocks*sizeof(*blocks->hps));
    if ( format != 0 )
    {
        blocks->M = (MAP_HUFF != 0) ? permalloc(ram->name,&ram->Perm,((blocks->numblocks >> shift) + 1)*sizeof(*blocks->M),8) : calloc(1,((blocks->numblocks >> shift) + 1)*sizeof(*blocks->M));
        if ( noload == 0 )
            blocks->blocknum = ram_load_blocks(ram,blocks,firstblock,blocks->numblocks);
    }
    else
    {
        blocks->blocknum = blocks->contiguous = ram_setcontiguous(blocks);
    }
    {
        char formatstr[16];
        ram_setformatstr(formatstr,blocks->format);
        printf("%s.%s contiguous blocks.%d | numblocks.%d\n",ram->name,formatstr,blocks->contiguous,blocks->numblocks);
    }
    return(blocks);
}

uint32_t ram_update_RTblock(struct ramchain_info *ram)
{
    ram->S.RTblocknum = _get_RTheight(ram);
    if ( ram->firstblock == 0 )
        ram->firstblock = ram->S.RTblocknum;
    else if ( (ram->S.RTblocknum - ram->firstblock) >= WITHRAW_ENABLE_BLOCKS )
        ram->S.enable_withdraws = 1;
    ram->S.blocknum = ram->blocks.blocknum = (ram->S.RTblocknum - ram->min_confirms);
    if ( ram->Bblocks.blocknum >= ram->S.RTblocknum-ram->min_confirms )
        ram->S.is_realtime = 1;
    return(ram->S.RTblocknum);
}

uint32_t ram_find_firstgap(struct ramchain_info *ram,int32_t format)
{
    char fname[1024],formatstr[15];
    uint32_t blocknum;
    FILE *fp;
    ram_setformatstr(formatstr,format);
    for (blocknum=0; blocknum<ram->S.RTblocknum; blocknum++)
    {
        ram_setfname(fname,ram,blocknum,formatstr);
        if ( (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
        {
            fclose(fp);
            continue;
        }
        break;
    }
    return(blocknum);
}

void ram_syncblocks(struct ramchain_info *ram,uint32_t blocknum,int32_t numblocks,uint64_t *sources,int32_t n,int32_t addshaflag);

int32_t ram_syncblock(struct ramchain_info *ram,struct syncstate *sync,uint32_t blocknum,int32_t log2bits)
{
    int32_t numblocks,n;
    numblocks = (1 << log2bits);
    while ( (n= ram_getsources(sync->requested,ram,blocknum,numblocks)) == 0 )
    {
        fprintf(stderr,"waiting for peers for block%d.%u of %u | peers.%d\n",numblocks,blocknum,ram->S.RTblocknum,n);
        sleep(3);
    }
    //for (i=0; i<n; i++)
    ///    printf("%llu ",(long long)sync->requested[i]);
    //printf("sources for %d.%d\n",blocknum,numblocks);
    ram_syncblocks(ram,blocknum,numblocks,sync->requested,n,0);
    sync->pending = n;
    sync->blocknum = blocknum;
    sync->format = numblocks;
    ram_update_RTblock(ram);
    return((ram->S.RTblocknum >> log2bits) << log2bits);
}

void ram_selfheal(struct ramchain_info *ram,uint32_t blocknum,int32_t numblocks)
{
    int32_t i;
    for (i=0; i<numblocks; i++)
        printf("magically heal block.%d\n",blocknum + i);
}

uint32_t ram_syncblock64(struct syncstate **subsyncp,struct ramchain_info *ram,struct syncstate *sync,uint32_t blocknum)
{
    uint32_t i,j,last64,done = 0;
    struct syncstate *subsync;
    last64 = (ram->S.RTblocknum >> 6) << 6;
    //fprintf(stderr,"syncblock64 from %d: last64 %d\n",blocknum,last64);
    if ( sync->substate == 0 )
        sync->substate = calloc(64,sizeof(*sync));
    for (i=0; blocknum<=last64&&i<64; blocknum+=64,i++)
    {
        subsync = &sync->substate[i];
        if ( subsync->minoritybits != 0 )
        {
            if ( subsync->substate == 0 )
                subsync->substate = calloc(64,sizeof(*subsync->substate));
            for (j=0; j<64; j++)
                ram_syncblock(ram,&sync->substate[j],blocknum+j,0);
        }
        else if ( subsync->majoritybits == 0 || bitweight(subsync->majoritybits) < 3 )
            last64 = ram_syncblock(ram,subsync,blocknum,6);
        else done++;
    }
    if ( subsyncp != 0 )
        (*subsyncp) = &sync->substate[i];
    //fprintf(stderr,"syncblock64 from %d: %d done of %d\n",blocknum,done,i);
    return(last64);
}

void ram_init_remotemode(struct ramchain_info *ram)
{
    struct syncstate *sync,*subsync,*blocksync;
    uint64_t requested[16];
    int32_t contiguous,activeblock;
    uint32_t blocknum,i,n,last64,last4096,done = 0;
    last4096 = (ram->S.RTblocknum >> 12) << 12;
    activeblock = contiguous = -1;
    while ( done < (last4096 >> 12) )
    {
        for (i=blocknum=0; blocknum<last4096; blocknum+=4096,i++)
        {
            sync = &ram->verified[i];
            if ( sync->minoritybits != 0 )
                ram_syncblock64(0,ram,sync,blocknum);
            else if ( sync->majoritybits == 0 || bitweight(sync->majoritybits) < 3 )
                ram_syncblock(ram,sync,blocknum,12);
            else done++;
        }
        printf("block.%u last4096.%d done.%d of %d\n",blocknum,last4096,done,i);
        last64 = ((ram->S.RTblocknum >> 6) << 6);
        sync = &ram->verified[i];
        ram_syncblock64(&subsync,ram,sync,blocknum);
        if ( subsync->substate == 0 )
            subsync->substate = calloc(64,sizeof(*subsync->substate));
        for (i=0; blocknum<ram->S.RTblocknum&&i<64; blocknum++,i++)
        {
            blocksync = &sync->substate[i];
            if ( blocksync->majoritybits == 0 || bitweight(blocksync->majoritybits) < 3 )
                ram_syncblock(ram,blocksync,blocknum,0);
        }
        portable_sleep(10);
    }
    for (i=0; i<4096; i++)
    {break;
        for (blocknum=i; blocknum<ram->S.RTblocknum; blocknum+=(ram->S.RTblocknum>>12))
        {
            if ( (n= ram_getsources(requested,ram,blocknum,1)) == 0 )
            {
                fprintf(stderr,"unexpected nopeers block.%u of %u | peers.%d\n",blocknum,ram->S.RTblocknum,n);
                continue;
            }
            ram_syncblocks(ram,blocknum,1,&requested[rand() % n],1,0);
        }
        msleep(10);
    }
}

FILE *ram_permopen(char *fname,char *coinstr)
{
    FILE *fp;
    if ( 0 && strcmp(coinstr,"BTC") == 0 )
        return(0);
    if ( (fp= fopen(os_compatible_path(fname),"rb+")) == 0 )
        fp= fopen(os_compatible_path(fname),"wb+");
    if ( fp == 0 )
    {
        printf("ram_permopen couldnt create (%s)\n",fname);
        exit(-1);
    }
    return(fp);
}

int32_t ram_init_hashtable(int32_t deletefile,uint32_t *blocknump,struct ramchain_info *ram,char type)
{
    long offset,len,fileptr;
    uint64_t datalen;
    char fname[1024],destfname[1024],str[64];
    uint8_t *hashdata;
    int32_t varsize,num,rwflag = 0;
    struct ramchain_hashtable *hash;
    struct ramchain_hashptr *ptr;
    hash = ram_gethash(ram,type);
    if ( deletefile != 0 )
        memset(hash,0,sizeof(*hash));
    strcpy(hash->coinstr,ram->name);
    hash->type = type;
    num = 0;
    //if ( ram->remotemode == 0 )
    {
        ram_sethashname(fname,hash,0);
        strcat(fname,".perm");
        hash->permfp = ram_permopen(fname,ram->name);
    }
    ram_sethashname(fname,hash,0);
    printf("inithashtable.(%s.%d) -> [%s]\n",ram->name,type,fname);
    if ( deletefile == 0 && (hash->newfp= fopen(os_compatible_path(fname),"rb+")) != 0 )
    {
        if ( init_mappedptr(0,&hash->M,0,rwflag,fname) == 0 )
            return(1);
        if ( hash->M.allocsize == 0 )
            return(1);
        fileptr = (long)hash->M.fileptr;
        offset = 0;
        while ( (varsize= (int32_t)hload_varint(&datalen,hash->newfp)) > 0 && (offset + datalen) <= hash->M.allocsize )
        {
            hashdata = (uint8_t *)(fileptr + offset);
            if ( num < 10 )
            {
                char hexbytes[8192];
                struct ramchain_hashptr *checkptr;
                init_hexbytes_noT(hexbytes,hashdata,varsize+datalen);
                HASH_FIND(hh,hash->table,hashdata,varsize + datalen,checkptr);
                fprintf(stderr,"%s offset %ld: varsize.%d datalen.%d created.(%s) ind.%d | checkptr.%p\n",ram->name,offset,(int)varsize,(int)datalen,(type != 'a') ? hexbytes :(char *)((long)hashdata+varsize),hash->ind+1,checkptr);
            }
            HASH_FIND(hh,hash->table,hashdata,varsize + datalen,ptr);
            if ( ptr != 0 )
            {
                printf("corrupted hashtable %s: offset.%ld\n",fname,offset);
                exit(-1);
            }
            ptr = (MAP_HUFF != 0 ) ? permalloc(ram->name,&ram->Perm,sizeof(*ptr),6) : calloc(1,sizeof(*ptr));
            ram_addhash(hash,ptr,hashdata,(int32_t)(varsize+datalen));
            offset += (varsize + datalen);
            fseek(hash->newfp,offset,SEEK_SET);
            num++;
        }
        printf("%s: loaded %d strings, ind.%d, offset.%ld allocsize.%llu %s\n",fname,num,hash->ind,offset,(long long)hash->M.allocsize,((sizeof(uint64_t)+offset) != hash->M.allocsize && offset != hash->M.allocsize) ? "ERROR":"OK");
        if ( offset != hash->M.allocsize && offset != (hash->M.allocsize-sizeof(uint64_t)) )
        {
            //*blocknump = ram_load_blockcheck(hash->newfp);
            if ( (offset+sizeof(uint64_t)) != hash->M.allocsize )
            {
                printf("offset.%ld + 8 %ld != %ld allocsize\n",offset,(offset+sizeof(uint64_t)),(long)hash->M.allocsize);
                exit(-1);
            }
        }
        if ( (hash->ind + 1) > ram->maxind )
            ram->maxind = (hash->ind + 1);
        ram_sethashtype(str,hash->type);
        sprintf(destfname,"%s/ramchains/%s.%s",MGWROOT,ram->name,str);
        if ( (len= copy_file(fname,destfname)) > 0 )
            printf("copied (%s) -> (%s) %s\n",fname,destfname,_mbstr(len));
        return(0);
    } else hash->newfp = fopen(os_compatible_path(fname),"wb");
    return(1);
}

void ram_init_directories(struct ramchain_info *ram)
{
    char dirA[1024],dirB[1024],dirC[1024];
    int32_t i,j,blocknum = 0;
    ram_setdirA(dirA,ram);
    ensure_dir(dirA);
    for (i=0; blocknum+64<=ram->maxblock; i++)
    {
        ram_setdirB(1,dirB,ram,i * 64 * 64);
        for (j=0; j<64&&blocknum+64<=ram->maxblock; j++,blocknum+=64)
            ram_setdirC(1,dirC,ram,blocknum);
    }
}

void ram_regen(struct ramchain_info *ram)
{
    uint32_t blocknums[3],pass,firstblock;
    printf("REGEN\n");
    ram->mappedblocks[4] = ram_init_blocks(1,ram->blocks.hps,ram,0,&ram->blocks4096,&ram->blocks64,4096,12);
    ram->mappedblocks[3] = ram_init_blocks(1,ram->blocks.hps,ram,0,&ram->blocks64,&ram->Bblocks,64,6);
    ram->mappedblocks[2] = ram_init_blocks(1,ram->blocks.hps,ram,0,&ram->Bblocks,&ram->Vblocks,'B',0);
    ram->mappedblocks[1] = ram_init_blocks(1,ram->blocks.hps,ram,0,&ram->Vblocks,&ram->blocks,'V',0);
    ram->mappedblocks[0] = ram_init_blocks(0,ram->blocks.hps,ram,0,&ram->blocks,0,0,0);
    ram_update_RTblock(ram);
    for (pass=1; pass<=4; pass++)
    {
        printf("pass.%d\n",pass);
        if ( 1 && pass == 2 )
        {
            if ( ram->permfp != 0 )
                fclose(ram->permfp);
            ram->permfp = ram_permopen(ram->permfname,ram->name);
            ram_init_hashtable(1,&blocknums[0],ram,'a');
            ram_init_hashtable(1,&blocknums[1],ram,'s');
            ram_init_hashtable(1,&blocknums[2],ram,'t');
        }
        else if ( pass == 1 )
        {
            firstblock = ram_find_firstgap(ram,ram->mappedblocks[pass]->format);
            printf("firstblock.%d\n",firstblock);
            if ( firstblock < 10 )
                ram->mappedblocks[pass]->blocknum = 0;
            else ram->mappedblocks[pass]->blocknum = (firstblock - 1);
            printf("firstblock.%u -> %u\n",firstblock,ram->mappedblocks[pass]->blocknum);
        }
        ram_process_blocks(ram,ram->mappedblocks[pass],ram->mappedblocks[pass-1],100000000.);
    }
    printf("FINISHED REGEN\n");
    exit(1);
}

void ram_init_tmpspace(struct ramchain_info *ram,long size)
{
    ram->Tmp.ptr = (MAP_HUFF != 0) ? permalloc(ram->name,&ram->Perm,size,8) : calloc(1,size);
    // mem->ptr = malloc(size);
    ram->Tmp.size = size;
    clear_alloc_space(&ram->Tmp);
}

void ram_allocs(struct ramchain_info *ram)
{
    int32_t tmpsize = TMPALLOC_SPACE_INCR;
    void *ptr;
    permalloc(ram->name,&ram->Perm,PERMALLOC_SPACE_INCR,0);
    ram->blocks.M = permalloc(ram->name,&ram->Perm,sizeof(*ram->blocks.M),8);
    ram->snapshots = permalloc(ram->name,&ram->Perm,sizeof(*ram->snapshots) * (ram->maxblock / 64),8);
    ram->permhash4096 = permalloc(ram->name,&ram->Perm,sizeof(*ram->permhash4096) * (ram->maxblock / 4096),8);
    ram->verified = permalloc(ram->name,&ram->Perm,sizeof(*ram->verified) * (ram->maxblock / 4096),8);
    ram->blocks.hps = permalloc(ram->name,&ram->Perm,ram->maxblock*sizeof(*ram->blocks.hps),8);
    ram_init_tmpspace(ram,tmpsize);
    ptr = (MAP_HUFF != 0 ) ? permalloc(ram->name,&ram->Perm,tmpsize,8) : calloc(1,tmpsize), ram->tmphp = hopen(ram->name,&ram->Perm,ptr,tmpsize,0);
    ptr = (MAP_HUFF != 0 ) ? permalloc(ram->name,&ram->Perm,tmpsize,8) : calloc(1,tmpsize), ram->tmphp2 = hopen(ram->name,&ram->Perm,ptr,tmpsize,0);
    ptr = (MAP_HUFF != 0 ) ? permalloc(ram->name,&ram->Perm,tmpsize,8) : calloc(1,tmpsize), ram->tmphp3 = hopen(ram->name,&ram->Perm,ptr,tmpsize,0);
    ram->R = (MAP_HUFF != 0) ? permalloc(ram->name,&ram->Perm,sizeof(*ram->R),8) : calloc(1,sizeof(*ram->R));
    ram->R2 = (MAP_HUFF != 0) ? permalloc(ram->name,&ram->Perm,sizeof(*ram->R2),8) : calloc(1,sizeof(*ram->R2));
    ram->R3 = (MAP_HUFF != 0) ? permalloc(ram->name,&ram->Perm,sizeof(*ram->R3),8) : calloc(1,sizeof(*ram->R3));
}

uint32_t ram_loadblocks(struct ramchain_info *ram,double startmilli)
{
    uint32_t numblocks;
    numblocks = ram_setcontiguous(&ram->blocks);
    ram->mappedblocks[4] = ram_init_blocks(0,ram->blocks.hps,ram,(numblocks>>12)<<12,&ram->blocks4096,&ram->blocks64,4096,12);
    printf("set ramchain blocknum.%s %d (1st %d num %d) vs RT.%d %.1f seconds to init_ramchain.%s B4096\n",ram->name,ram->blocks4096.blocknum,ram->blocks4096.firstblock,ram->blocks4096.numblocks,ram->blocks.blocknum,(milliseconds() - startmilli)/1000.,ram->name);
    ram->mappedblocks[3] = ram_init_blocks(0,ram->blocks.hps,ram,ram->blocks4096.contiguous,&ram->blocks64,&ram->Bblocks,64,6);
    printf("set ramchain blocknum.%s %d vs (1st %d num %d) RT.%d %.1f seconds to init_ramchain.%s B64\n",ram->name,ram->blocks64.blocknum,ram->blocks64.firstblock,ram->blocks64.numblocks,ram->blocks.blocknum,(milliseconds() - startmilli)/1000.,ram->name);
    ram->mappedblocks[2] = ram_init_blocks(0,ram->blocks.hps,ram,ram->blocks64.contiguous,&ram->Bblocks,&ram->Vblocks,'B',0);
    printf("set ramchain blocknum.%s %d vs (1st %d num %d) RT.%d %.1f seconds to init_ramchain.%s B\n",ram->name,ram->Bblocks.blocknum,ram->Bblocks.firstblock,ram->Bblocks.numblocks,ram->blocks.blocknum,(milliseconds() - startmilli)/1000.,ram->name);
    ram->mappedblocks[1] = ram_init_blocks(0,ram->blocks.hps,ram,ram->Bblocks.contiguous,&ram->Vblocks,&ram->blocks,'V',0);
    printf("set ramchain blocknum.%s %d vs (1st %d num %d) RT.%d %.1f seconds to init_ramchain.%s V\n",ram->name,ram->Vblocks.blocknum,ram->Vblocks.firstblock,ram->Vblocks.numblocks,ram->blocks.blocknum,(milliseconds() - startmilli)/1000.,ram->name);
    //ram_process_blocks(ram,ram->mappedblocks[2],ram->mappedblocks[1],1000.*3600*24);
    ram->mappedblocks[0] = ram_init_blocks(0,ram->blocks.hps,ram,0,&ram->blocks,0,0,0);
    return(numblocks);
}

void ram_Hfiles(struct ramchain_info *ram)
{
    /*uint32_t blocknum,iter,i;
    HUFF *hp;
    for (blocknum=0; blocknum<ram->blocks.contiguous; blocknum+=64)
    {break;
        ram->minval = ram->maxval = ram->minval2 = ram->maxval2 = ram->minval4 = ram->maxval4 = ram->minval8 = ram->maxval8 = 0;
        memset(&ram->H,0,sizeof(ram->H));
        for (iter=0; iter<2; iter++)
            for (i=0; i<64; i++)
            {
                if ( (hp= ram->blocks.hps[blocknum+i]) != 0 )
                    ram_rawblock_update(iter==0?'H':'h',ram,hp,blocknum+i);
            }
        huffpair_gencodes(ram,&ram->H,0);
        free(ram->H.numtx.items);
         free(ram->H.tx0.numvins.items), free(ram->H.tx0.numvouts.items), free(ram->H.tx0.txid.items);
         free(ram->H.tx1.numvins.items), free(ram->H.tx1.numvouts.items), free(ram->H.tx1.txid.items);
         free(ram->H.txi.numvins.items), free(ram->H.txi.numvouts.items), free(ram->H.txi.txid.items);
         free(ram->H.vin0.txid.items), free(ram->H.vin0.vout.items);
         free(ram->H.vin1.txid.items), free(ram->H.vin1.vout.items);
         free(ram->H.vini.txid.items), free(ram->H.vini.vout.items);
         free(ram->H.vout0.addr.items), free(ram->H.vout0.script.items);
         free(ram->H.vout1.addr.items), free(ram->H.vout1.script.items);
         free(ram->H.vouti.addr.items), free(ram->H.vout2.script.items);
         free(ram->H.vout2.addr.items), free(ram->H.vouti.script.items);
         free(ram->H.voutn.addr.items), free(ram->H.voutn.script.items);
    }
    //fprintf(stderr,"totalbytes.%lld %s -> %s totalbits.%lld R%.3f\n",(long long)ram->totalbits,_mbstr((double)ram->totalbytes),_mbstr2((double)ram->totalbits/8),(long long)ram->totalbytes,(double)ram->totalbytes*8/ram->totalbits);*/
}

void ram_convertall(struct ramchain_info *ram)
{
    char fname[1024];
    bits256 refsha,sha;
    uint32_t blocknum,checkblocknum;
    HUFF *hp,*permhp;
    void *buf;
    for (blocknum=0; blocknum<ram->blocks.contiguous; blocknum++)
    {
        if ( (hp= ram->blocks.hps[blocknum]) != 0 )
        {
            buf = (MAP_HUFF != 0) ? permalloc(ram->name,&ram->Perm,hp->allocsize*2,9) : calloc(1,hp->allocsize*2);
            permhp = hopen(ram->name,&ram->Perm,buf,hp->allocsize*2,0);
            ram->blocks.hps[blocknum] = ram_conv_permind(permhp,ram,hp,blocknum);
            permhp->allocsize = hconv_bitlen(permhp->endpos);
            if ( 0 && (checkblocknum= ram_verify(ram,permhp,'B')) != checkblocknum )
                printf("ram_verify(%d) -> %d?\n",checkblocknum,checkblocknum);
        }
        else { printf("unexpected gap at %d\n",blocknum); exit(-1); }
    }
    sprintf(fname,"%s/ramchains/%s.perm",MGWROOT,ram->name);
    ram_save_bitstreams(&refsha,fname,ram->blocks.hps,ram->blocks.contiguous);
    ram_map_bitstreams(1,ram,0,ram->blocks.M,&sha,ram->blocks.hps,ram->blocks.contiguous,fname,&refsha);
    printf("converted to permind, please copy over files with .perm files and restart\n");
    exit(1);
}

int32_t ram_scanblocks(struct ramchain_info *ram)
{
    uint32_t blocknum,errs=0,good=0,iter;
    HUFF *hp;
    for (iter=0; iter<2; iter++)
    {
        for (errs=good=blocknum=0; blocknum<ram->blocks.contiguous; blocknum++)
        {
            if ( (blocknum % 1000) == 0 )
                fprintf(stderr,".");
            if ( (hp= ram->blocks.hps[blocknum]) != 0 )
            {
                if ( ram_rawblock_update(iter,ram,hp,blocknum) > 0 )
                {
                    if ( iter == 0 && ram->permfp != 0 )
                        ram_conv_permind(ram->tmphp2,ram,hp,blocknum);
                    good++;
                }
                else
                {
                    printf("iter.%d error on block.%d purge it\n",iter,blocknum);
                    while ( 1 ) portable_sleep(1);
                    //ram_purge_badblock(ram,blocknum);
                    errs++;
                    exit(-1);
                }
            } else errs++;
        }
        printf(">>>>>>>>>>>>> permind_changes.%d <<<<<<<<<<<<\n",ram->permind_changes);
        if ( 0 && ram->addrhash.permfp != 0 && ram->txidhash.permfp != 0 && ram->scripthash.permfp != 0 && iter == 0 && ram->permind_changes != 0 )
            ram_convertall(ram);
    }
    if ( ram->permfp != 0 )
        fflush(ram->permfp);
    fprintf(stderr,"contiguous.%d good.%d errs.%d\n",ram->blocks.contiguous,good,errs);
    ram_Hfiles(ram);
    return(errs);
}

void ram_init_ramchain(struct ramchain_info *ram)
{
    int32_t datalen,nofile,numblocks,errs;
    uint32_t blocknums[3],permind;
    bits256 refsha,sha;
    double startmilli;
    char fname[1024];
    startmilli = milliseconds();
    strcpy(ram->dirpath,MGWROOT);
    ram->S.RTblocknum = _get_RTheight(ram);
    ram->blocks.blocknum = (ram->S.RTblocknum - ram->min_confirms);
    ram->blocks.numblocks = ram->maxblock = (ram->S.RTblocknum + 10000);
    ram_allocs(ram);
    printf("[%s] ramchain.%s RT.%d %.1f seconds to init_ramchain_directories: next.(%d %d %d %d)\n",ram->dirpath,ram->name,ram->S.RTblocknum,(milliseconds() - startmilli)/1000.,ram->S.permblocks,ram->next_txid_permind,ram->next_script_permind,ram->next_addr_permind);
    memset(blocknums,0,sizeof(blocknums));
    sprintf(ram->permfname,"%s/ramchains/%s.perm",MGWROOT,ram->name);
    nofile = (ram->permfp = ram_permopen(ram->permfname,ram->name)) == 0;
    nofile += ram_init_hashtable(0,&blocknums[0],ram,'a');
    nofile += ram_init_hashtable(0,&blocknums[1],ram,'s');
    nofile += ram_init_hashtable(0,&blocknums[2],ram,'t');
    ram_update_RTblock(ram);
    if ( ram->marker != 0 && ram->marker[0] != 0 && (ram->marker_rawind= ram_addrind_RO(&permind,ram,ram->marker)) == 0 )
        printf("WARNING: MARKER.(%s) set but no rawind. need to have it appear in blockchain first\n",ram->marker);
    if ( ram->marker2 != 0 && ram->marker2[0] != 0 && (ram->marker2_rawind= ram_addrind_RO(&permind,ram,ram->marker2)) == 0 )
        printf("WARNING: MARKER2.(%s) set but no rawind. need to have it appear in blockchain first\n",ram->marker2);
    printf("%.1f seconds to init_ramchain.%s hashtables marker.(%s || %s) %u %u\n",(milliseconds() - startmilli)/1000.,ram->name,ram->marker,ram->marker2,ram->marker_rawind,ram->marker2_rawind);
    if ( ram->remotemode != 0 )
        ram_init_remotemode(ram);
    else
    {
        ram_init_directories(ram);
        if ( nofile >= 3 )
            ram_regen(ram);
    }
    sprintf(fname,"%s/ramchains/%s.blocks",MGWROOT,ram->name);
    ram_map_bitstreams(0,ram,0,ram->blocks.M,&sha,ram->blocks.hps,0,fname,0);
    numblocks = ram_loadblocks(ram,startmilli);
    errs = ram_scanblocks(ram);
    if ( numblocks == 0 && errs == 0 && ram->blocks.contiguous > 4096 )
    {
        printf("saving new %s.blocks\n",ram->name);
        datalen = -1;
        if ( ram_save_bitstreams(&refsha,fname,ram->blocks.hps,ram->blocks.contiguous) > 0 )
            datalen = ram_map_bitstreams(1,ram,0,ram->blocks.M,&sha,ram->blocks.hps,ram->blocks.contiguous,fname,&refsha);
        printf("Created.(%s) datalen.%d | please restart\n",fname,datalen);
        exit(1);
    } else printf("no need to save numblocks.%d errs.%d contiguous.%d\n",numblocks,errs,ram->blocks.contiguous);
    ram_disp_status(ram);
}

void init_ram_MGWconfs(struct ramchain_info *ram,cJSON *confjson,char *MGWredemption,struct NXT_asset *ap)
{
    cJSON *array,*item;
    int32_t i,n,hasredemption = 0;
    char NXTADDR[MAX_JSON_FIELD];
    if ( (ram->ap= ap) == 0 )
    {
        printf("no asset for %s\n",ram->name);
        return;
    }
    ram->MGWbits = calc_nxt64bits(MGWredemption);
    if ( Debuglevel > 0 )
        printf("init_ram_MGWconfs.(%s) -> %llu\n",MGWredemption,(long long)ram->MGWbits);
    array = cJSON_GetObjectItem(confjson,"special_NXTaddrs");
    if ( array != 0 && is_cJSON_Array(array) != 0 ) // first three must be the gateway's addresses
    {
        n = cJSON_GetArraySize(array);
        ram->special_NXTaddrs = calloc(n+2,sizeof(*ram->special_NXTaddrs));
        for (i=0; i<n; i++)
        {
            if ( array == 0 || n == 0 )
                break;
            item = cJSON_GetArrayItem(array,i);
            copy_cJSON(NXTADDR,item);
            if ( NXTADDR[0] == 0 )
            {
                fprintf(stderr,"Illegal special NXTaddr.%d\n",i);
                exit(1);
            }
            ram->special_NXTaddrs[i] = clonestr(NXTADDR);
            if ( strcmp(NXTADDR,MGWredemption) == 0 )
                hasredemption = 1;
        }
        if ( Debuglevel > 0 )
            printf("special_addrs.%d\n",n);
        if ( hasredemption == 0 )
            ram->special_NXTaddrs[i++] = clonestr(MGWredemption);
        ram->special_NXTaddrs[i++] = clonestr(GENESISACCT);
        n = i;
    }
    else
    {
        for (n=0; MGW_whitelist[n][0]!=0; n++)
            ;
        ram->special_NXTaddrs = calloc(n,sizeof(*ram->special_NXTaddrs));
        for (i=0; i<n; i++)
            ram->special_NXTaddrs[i] = clonestr(MGW_whitelist[i]);
    }
    ram->numspecials = n;
    if ( Debuglevel > 0 )
    {
        for (i=0; i<n; i++)
            printf("(%s) ",ram->special_NXTaddrs[i]);
        printf("numspecials.%d\n",ram->numspecials);
    }
    if ( ram->limboarray == 0 )
        ram->limboarray = calloc(2,sizeof(*ram->limboarray));
    if ( Debuglevel > 0 )
    {
        for (i=0; ram->limboarray[i]!=0&&ram->limboarray[i]!=0; i++)
            printf("%llu ",(long long)ram->limboarray[i]);
        printf("limboarray.%d\n",i);
    }
}

struct ramchain_info *get_ramchain_info(char *coinstr)
{
    //struct coin_info *cp = get_coin_info(coinstr);
    //if ( NORAMCHAINS == 0 && cp != 0 )
    //    return(&cp->RAM);
    //else
    printf("need to implement get_ramchain_info\n");
        return(0);
}

void init_ramchain_info(struct ramchain_info *ram,void *cp,int32_t DEPOSIT_XFER_DURATION,int32_t oldtx)
{
    //struct NXT_asset *ap = 0;
   /* struct coin_info *refcp = get_coin_info("BTCD");
    int32_t createdflag;
    strcpy(ram->name,cp->name);
    strcpy(ram->S.name,ram->name);
    if ( refcp->myipaddr[0] != 0 )
        strcpy(ram->myipaddr,refcp->myipaddr);
    strcpy(ram->srvNXTACCTSECRET,refcp->srvNXTACCTSECRET);
    strcpy(ram->srvNXTADDR,refcp->srvNXTADDR);
    ram->oldtx = oldtx;
    ram->S.nxt64bits = calc_nxt64bits(refcp->srvNXTADDR);
    if ( cp->marker == 0 )
        cp->marker = clonestr(get_marker(cp->name));
    if ( cp->marker != 0 )
        ram->marker = clonestr(cp->marker);
    if ( cp->marker2 == 0 )
        cp->marker2 = clonestr(get_marker(cp->name));
    if ( cp->marker2 != 0 )
        ram->marker2 = clonestr(cp->marker2);
    if ( cp->privateaddr[0] != 0 )
        ram->opreturnmarker = clonestr(cp->privateaddr);
    ram->dust = cp->dust;
    if ( cp->backupdir[0] != 0 )
        ram->backups = clonestr(cp->backupdir);
    if ( cp->userpass != 0 )
        ram->userpass = clonestr(cp->userpass);
    if ( cp->serverport != 0 )
        ram->serverport = clonestr(cp->serverport);
    ram->lastheighttime = (uint32_t)cp->lastheighttime;
    ram->S.RTblocknum = (uint32_t)cp->RTblockheight;
    ram->minoutput = get_API_int(cJSON_GetObjectItem(cp->json,"minoutput"),1);
    ram->min_confirms = cp->min_confirms;
    ram->depositconfirms = get_API_int(cJSON_GetObjectItem(cp->json,"depositconfirms"),ram->min_confirms);
    ram->min_NXTconfirms = MIN_NXTCONFIRMS;
    ram->withdrawconfirms = get_API_int(cJSON_GetObjectItem(cp->json,"withdrawconfirms"),ram->min_NXTconfirms);
    ram->remotemode = get_API_int(cJSON_GetObjectItem(cp->json,"remote"),0);
    ram->multisigchar = cp->multisigchar;
    ram->estblocktime = cp->estblocktime;
    ram->firstiter = 1;
    ram->numgateways = NUM_GATEWAYS;
    if ( ram->numgateways != (sizeof(ram->otherS)/sizeof(*ram->otherS)) )
    {
        printf("expected numgateways.%ld instead of %u\n",(sizeof(ram->otherS)/sizeof(*ram->otherS)),ram->numgateways);
        exit(1);
    }
    ram->S.gatewayid = Global_mp->gatewayid;
    ram->NXTfee_equiv = cp->NXTfee_equiv;
    ram->txfee = cp->txfee;
    ram->NXTconvrate = get_API_float(cJSON_GetObjectItem(cp->json,"NXTconv"));//();
    ram->DEPOSIT_XFER_DURATION = get_API_int(cJSON_GetObjectItem(cp->json,"DEPOSIT_XFER_DURATION"),DEPOSIT_XFER_DURATION);
    if ( Global_mp->iambridge != 0 || (IS_LIBTEST > 0 && is_active_coin(cp->name) > 0) )
    {
        if ( Debuglevel > 0 )
            printf("gatewayid.%d MGWissuer.(%s) init_ramchain_info(%s) (%s) active.%d (%s %s) multisigchar.(%c) confirms.(deposit %d withdraw %d) rate %.8f\n",ram->S.gatewayid,cp->MGWissuer,ram->name,cp->name,is_active_coin(cp->name),ram->serverport,ram->userpass,ram->multisigchar,ram->depositconfirms,ram->withdrawconfirms,ram->NXTconvrate);
        init_ram_MGWconfs(ram,cp->json,(cp->MGWissuer[0] != 0) ? cp->MGWissuer : NXTISSUERACCT,get_NXTasset(&createdflag,Global_mp,cp->assetid));
        activate_ramchain(ram,cp->name);
    } //else printf("skip activate ramchains\n");*/
}


#endif
#endif
