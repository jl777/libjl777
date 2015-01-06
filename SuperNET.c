//
//  main.c
//  libtest
//
//  Created by jl777 on 8/13/14.
//  Copyright (c) 2014 jl777. MIT License.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <memory.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "uthash.h"

//Miniupnp code for supernet by chanc3r
#include <time.h>
#ifdef _WIN32
#include <winsock2.h>
#define snprintf _snprintf
#else
// for IPPROTO_TCP / IPPROTO_UDP
#include <netinet/in.h>
#endif
#include "miniupnpc/miniwget.h"
#include "miniupnpc/miniupnpc.h"
#include "miniupnpc/upnpcommands.h"
#include "miniupnpc/upnperrors.h"


#include "SuperNET.h"
#include "cJSON.h"
#define NUM_GATEWAYS 3
extern char Server_names[256][MAX_JSON_FIELD],MGWROOT[];
extern char Server_NXTaddrs[256][MAX_JSON_FIELD];
extern int32_t IS_LIBTEST,USESSL,SUPERNET_PORT,ENABLE_GUIPOLL,Debuglevel,UPNP,MULTIPORT,Finished_init;
extern cJSON *MGWconf;
#define issue_curl(curl_handle,cmdstr) bitcoind_RPC(curl_handle,"curl",cmdstr,0,0,0)
char *bitcoind_RPC(void *deprecated,char *debugstr,char *url,char *userpass,char *command,char *params);
void expand_ipbits(char *ipaddr,uint32_t ipbits);
uint64_t conv_acctstr(char *acctstr);
void calc_sha256(char hashstr[(256 >> 3) * 2 + 1],unsigned char hash[256 >> 3],unsigned char *src,int32_t len);
int32_t decode_hex(unsigned char *bytes,int32_t n,char *hex);
int32_t expand_nxt64bits(char *NXTaddr,uint64_t nxt64bits);
char *clonestr(char *);
int32_t init_hexbytes_noT(char *hexbytes,unsigned char *message,long len);
char *_mbstr(double n);
double milliseconds();
struct coin_info *get_coin_info(char *coinstr);
uint32_t get_blockheight(struct coin_info *cp);
long stripwhite_ns(char *buf,long len);
int32_t safecopy(char *dest,char *src,long len);

void set_compressionvars_fname(int32_t readonly,char *fname,char *coinstr,char *typestr,int32_t subgroup)
{
    char *dirname = (1*readonly != 0) ? "/Users/jimbolaptop/ramchains" : "ramchains";
    if ( subgroup < 0 )
        sprintf(fname,"%s/%s/%s.%s",dirname,coinstr,coinstr,typestr);
    else sprintf(fname,"%s/%s/%s/%s.%d",dirname,coinstr,typestr,coinstr,subgroup);
}
uint32_t conv_rawind(uint32_t huffid,uint32_t rawind) { return((rawind << 4) | (huffid&0xf)); }

void update_bitstream(struct bitstream_file *bfp,uint32_t blocknum)
{
    bfp->blocknum = blocknum;
}

#define INCLUDE_CODE
#include "ramchain.h"
#undef INCLUDE_CODE

int32_t calc_frequi(uint32_t *slicep,char *coinstr,uint32_t blocknum)
{
    int32_t slice,incr;
    incr = 1000;
    slice = (incr / HUFF_NUMFREQS);
    if ( slicep != 0 )
        *slicep = slice;
    return((blocknum / slice) % HUFF_NUMFREQS);
}

cJSON *gen_blockjson(struct compressionvars *V,uint32_t blocknum)
{
    cJSON *array,*item,*json = cJSON_CreateObject();
    char *txidstr,*addr,*script;
    struct blockinfo *block,*next;
    struct address_entry *vin;
    struct voutinfo *vout;
    int32_t ind;
    if ( (block= get_blockinfo(V,blocknum)) != 0 && (next= get_blockinfo(V,blocknum+1)) != 0 )
    {
        printf("block.%d: (%d %d) next.(%d %d)\n",blocknum,block->firstvin,block->firstvout,next->firstvout,next->firstvout);
        if ( block->firstvin <= next->firstvin && block->firstvout <= next->firstvout )
        {
            if ( next->firstvin > block->firstvin )
            {
                array = cJSON_CreateArray();
                for (ind= block->firstvin; ind<next->firstvin; ind++)
                {
                    if ( (vin= get_vininfo(V,ind)) != 0 )
                    {
                        item = cJSON_CreateObject();
                        cJSON_AddItemToObject(item,"block",cJSON_CreateNumber(vin->blocknum));
                        cJSON_AddItemToObject(item,"txind",cJSON_CreateNumber(vin->txind));
                        cJSON_AddItemToObject(item,"vout",cJSON_CreateNumber(vin->v));
                        cJSON_AddItemToArray(array,item);
                    }
                }
                cJSON_AddItemToObject(json,"vins",array);
            }
            if ( next->firstvout > block->firstvout )
            {
                array = cJSON_CreateArray();
                for (ind= block->firstvout; ind<next->firstvout; ind++)
                {
                    if ( (vout= get_voutinfo(V,ind)) != 0 )
                    {
                        item = cJSON_CreateObject();
                        if ( (txidstr= conv_txidind(V,vout->tp_ind)) != 0 )
                            cJSON_AddItemToObject(item,"txid",cJSON_CreateString(txidstr));
                        cJSON_AddItemToObject(item,"vout",cJSON_CreateNumber(vout->vout));
                        cJSON_AddItemToObject(item,"value",cJSON_CreateNumber(dstr(vout->value)));
                        if ( (addr= conv_addrind(V,vout->addr_ind)) != 0 )
                            cJSON_AddItemToObject(item,"addr",cJSON_CreateString(addr));
                        if ( (script= conv_scriptind(V,vout->sp_ind)) != 0 )
                            cJSON_AddItemToObject(item,"script",cJSON_CreateString(script));
                        cJSON_AddItemToArray(array,item);
                    }
                }
                cJSON_AddItemToObject(json,"vouts",array);
            }
        } else cJSON_AddItemToObject(json,"error",cJSON_CreateString("block firstvin or firstvout violation"));
    }
    return(json);
}

int32_t checkblock(struct blockinfo *current,struct blockinfo *prev,uint32_t blocknum)
{
    int32_t numvins,numvouts;
    //return((abs((int)~(blockcheck>>32)-blocknum)+abs((int)blockcheck-blocknum)));
    if ( prev != 0 )
    {
        if ( (numvins= (current->firstvin - prev->firstvin)) < 0 || (numvouts= (current->firstvout - prev->firstvout)) < 0 )
            return(1);
        printf("block.%d: vins.(%d %d) vouts.(%d %d)\n",blocknum-1,prev->firstvin,numvins,prev->firstvout,numvouts);
    }
    return(0);
}

int32_t scan_ramchain(struct compressionvars *V)
{
    cJSON *json;
    char *jsonstr;
    int i,checkval,frequi,errs = 0;
    //uint64_t blockcheck;
    struct huffitem **items;
    struct blockinfo B,prevB;
    struct bitstream_file *bfp;
    memset(&prevB,0,sizeof(prevB));
    bfp = V->bfps[0];
    if ( bfp->fp == 0 )
        return(-1);
    rewind(bfp->fp);
    frequi = 0;
    for (i=0; i<bfp->blocknum; i++)
    {
        fread(&B,1,sizeof(B),bfp->fp);
        checkval = checkblock(&B,i==0?0:&prevB,i);
        if ( checkval != 0 )
            printf("%i: %d %d | %s\n",i,B.firstvout,B.firstvin,checkval!=0?"ERROR":"OK");
        if ( (V->blocknum= i) > 0 )
        {
            json = gen_blockjson(V,i-1);
            if ( json != 0 )
            {
                jsonstr = cJSON_Print(json);
                stripwhite_ns(jsonstr,strlen(jsonstr));
                printf("%s\n",jsonstr);
                free(jsonstr);
                free_json(json);
            }
        }
        prevB = B;
        errs += (checkval != 0);
    }
    //uint32_t valuebfp,inblockbfp,txinbfp,invoutbfp,addrbfp,txidbfp,scriptbfp,voutsbfp,vinsbfp,bitstream,numbfps;
    printf("scan_ramchain %s: errs.%d blocks.%u values.%u addrs.%u txids.%u scripts.%u vouts.%u vins.%u | VIN block.%u txind.%u v.%u\n",bfp->coinstr,errs,bfp->blocknum,V->bfps[V->valuebfp]->ind,V->bfps[V->addrbfp]->ind,V->bfps[V->txidbfp]->ind,V->bfps[V->scriptbfp]->ind,V->bfps[V->voutsbfp]->ind,V->bfps[V->vinsbfp]->ind,V->inblockbfp->ind,V->txinbfp->ind,V->voutbfp->ind);
    for (i=0; i<16; i++)
    {
        if ( (bfp= V->bfps[i]) != 0 && bfp->ind > 0 && (bfp->itemptrs != 0 || (bfp->mode & BITSTREAM_STATSONLY) != 0) )
        {
            int j;
            items = 0;
            printf("huff_create.%s ind.%d\n",bfp->typestr,bfp->ind);
            if ( (bfp->mode & BITSTREAM_STATSONLY) != 0 )
            {
                items = calloc(bfp->maxitems,sizeof(*items));
                for (j=0; j<bfp->maxitems; j++)
                    items[j] = &bfp->dataptr[j];
                bfp->huff = huff_create(0,V,items,bfp->maxitems,frequi);
                huff_disp(0,bfp->huff,frequi);
                free(items);
            }
            else if ( (items= bfp->itemptrs) != 0 )
            {
                bfp->huff = huff_create(0,V,items+1,bfp->ind,frequi);
                huff_disp(0,bfp->huff,frequi);
             }
        }
    }
    for (i=0; i<16; i++)
    {
        struct huffcode *huff;
        if ( (bfp= V->bfps[i]) != 0 && (huff= bfp->huff) != 0 )
        {
            printf("Total.%-8s numnodes.%d %.0f -> %.0f: ratio %.3f\n",bfp->typestr,huff->numnodes,huff->totalbytes,huff->totalbits,huff->totalbytes*8/(huff->totalbits+1));
        }
    }
    getchar();
    return(errs);
}

void clear_compressionvars(struct compressionvars *V,int32_t clearstats,int32_t frequi)
{
    V->maxitems = 0;
    /* int32_t i;
     struct scriptinfo *sp = 0;
     struct txinfo *tp = 0;
     struct coinaddrinfo *addrp = 0;
     struct valueinfo *valp = 0;
     if ( clearstats != 0 )
     {
     clear_hashtable_field(V->values,(long)((long)&valp->item.freq[frequi] - (long)valp),sizeof(valp->item.freq[frequi]));
     clear_hashtable_field(V->addrs,(long)((long)&addrp->item.freq[frequi] - (long)addrp),sizeof(addrp->item.freq[frequi]));
     clear_hashtable_field(V->txids,(long)((long)&tp->item.freq[frequi] - (long)tp),sizeof(tp->item.freq[frequi]));
     clear_hashtable_field(V->scripts,(long)((long)&sp->item.freq[frequi] - (long)sp),sizeof(sp->item.freq[frequi]));
     for (i=0; i<V->maxblocknum; i++)
     V->blockitems[i].freq[frequi] = 0;
     for (i=0; i<(1<<16); i++)
     V->txinditems[i].freq[frequi] = 0;
     for (i=0; i<(1<<16); i++)
     V->voutitems[i].freq[frequi] = 0;
     }*/
}

double estimate_completion(char *coinstr,double startmilli,int32_t processed,int32_t numleft)
{
    double elapsed,rate;
    if ( processed <= 0 )
        return(0.);
    elapsed = (milliseconds() - startmilli);
    rate = (elapsed / processed);
    if ( rate <= 0. )
        return(0.);
    //printf("numleft %d rate %f\n",numleft,rate);
    return(numleft * rate / 1000.);
}

uint32_t flush_compressionvars(struct compressionvars *V,uint32_t prevblocknum,uint32_t newblocknum,int32_t frequi)
{
    struct blockinfo B;
    long sum,sum2,fpos;
    uint32_t slice,i;
    memset(&B,0,sizeof(B));
    if ( prevblocknum != 0xffffffff )
    {
        B.firstvout = V->firstvout, B.firstvin = V->firstvin;
        append_to_streamfile(V->bfps[0],prevblocknum,&B,1,0);
        V->firstvout = V->bfps[V->voutsbfp]->ind;
        V->firstvin = V->bfps[V->vinsbfp]->ind;
        B.firstvout = V->firstvout, B.firstvin = V->firstvin;
        fpos = ftell(V->bfps[0]->fp);
        append_to_streamfile(V->bfps[0],prevblocknum,&B,1,1);
        fseek(V->bfps[0]->fp,fpos,SEEK_SET);
        sum = sum2 = fpos;
        for (i=1; i<V->numbfps; i++)
        {
            if ( V->bfps[i]->fp != 0 )
            {
                emit_blockcheck(V->bfps[i]->fp,prevblocknum);
                sum += ftell(V->bfps[i]->fp);
            }
        }
        sum2 += ftell(V->bfps[V->voutsbfp]->fp) + ftell(V->bfps[V->vinsbfp]->fp);
        // numhuffinds = emit_compressed_block(V,prevblocknum,frequi);
        if ( V->disp != 0 )
        {
            sprintf(V->disp+strlen(V->disp),"-> max.%-4d %.1f %.1f est %.1f minutes\n%s F.%d NEWBLOCK.%u | ",V->maxitems,(double)sum/(prevblocknum+1),(double)sum2/(prevblocknum+1),estimate_completion(V->coinstr,V->startmilli,V->processed,(int32_t)V->maxblocknum-prevblocknum)/60,V->coinstr,frequi,prevblocknum);
            printf("%s",V->disp);
            V->disp[0] = 0;
        }
        calc_frequi(&slice,V->coinstr,newblocknum);
        clear_compressionvars(V,(newblocknum % slice) == 0,frequi);
    }
    V->blocknum = newblocknum;
    V->processed++;
    return(newblocknum);
}

void set_voutinfo(struct voutinfo *v,uint32_t tp_ind,uint32_t vout,uint32_t addr_ind,uint64_t value,uint32_t sp_ind)
{
    memset(v,0,sizeof(*v));
    v->tp_ind = tp_ind;
    v->vout = vout;
    v->addr_ind = addr_ind;
    v->value = value;
    v->sp_ind = sp_ind;
}

void update_ramchain(struct compressionvars *V,char *coinstr,char *addr,struct address_entry *bp,uint64_t value,char *txidstr,char *script,int32_t numvins,uint64_t inputsum,int32_t numvouts,uint64_t remainder)
{
    char valuestr[128];
    int32_t frequi,datalen,createdflag;
    uint8_t databuf[4096];
    struct voutinfo vout;
    union huffinfo U;
    struct huffitem *addrp,*tp,*sp,*valp;
    //printf("update ramchain.(%s) addr.(%s) block.%d vin.%d %p %p\n",coinstr,addr,bp->blocknum,bp->vinflag,txidstr,script);
    if ( V->numbfps != 0 )
    {
        //printf("update compressionvars vinflag.%d\n",bp->vinflag);
        if ( bp->vinflag == 0 )
        {
            if (txidstr != 0 && script != 0 ) // txidstr != 0 && script != 0 && value != 0 &&
            {
                //printf("txid.(%s) %s\n",txidstr,script);
                frequi = calc_frequi(0,V->coinstr,V->blocknum);
                if ( bp->blocknum != V->blocknum )
                    V->blocknum = flush_compressionvars(V,V->blocknum,bp->blocknum,frequi);
                memset(&U,0,sizeof(U));
                addrp = update_bitstream_file(V,&createdflag,V->bfps[V->addrbfp],bp->blocknum,0,0,addr,&U,-1);
                datalen = (uint32_t)(strlen(txidstr) >> 1);
                decode_hex(databuf,datalen,txidstr);
                memset(&U,0,sizeof(U));
                U.tx.txind = bp->txind;
                U.tx.numvins = numvins;
                U.tx.numvouts = numvouts;
                if ( (tp = update_bitstream_file(V,&createdflag,V->bfps[V->txidbfp],bp->blocknum,databuf,datalen,txidstr,&U,-1)) != 0 && createdflag != 0 )
                {
                    if ( tp->U.tx.txind != bp->txind || tp->U.tx.numvins != numvins || tp->U.tx.numvouts != numvouts )
                    {
                        printf("error creating tx.U: tp->U.tx.txind %d != %d bp->txind || tp->U.tx.numvins %d != %d numvins || tp->U.tx.numvouts %d != %d numvouts\n",tp->U.tx.txind,bp->txind,tp->U.tx.numvins,numvins,tp->U.tx.numvouts,numvouts);
                        exit(-1);
                    }
                }
                if ( strlen(script) < 1024 )
                {
                    memset(&U,0,sizeof(U));
                    U.script.addrind = (addrp->huffind >> 4);
                    U.script.mode = calc_scriptmode(&datalen,databuf,script,1);
                    //printf("mode.%d addrhuff.%d vs %d\n",U.script.mode,U.script.addrind,addrp->huffind);
                    if ( (sp= update_bitstream_file(V,&createdflag,V->bfps[V->scriptbfp],bp->blocknum,databuf,datalen,script,&U,-1)) != 0 && createdflag != 0 )
                    {
                        if ( sp->U.script.mode != U.script.mode || sp->U.script.addrind != (addrp->huffind>>4) )
                        {
                            printf("U.script error: sp->U.script.mode %d != %d U.script.mode || sp->U.script.addrind %d != %d (addrp->huffind>>4)\n",sp->U.script.mode,U.script.mode,sp->U.script.addrind,(addrp->huffind>>4));
                            exit(-1);
                        }
                    }
                    printf("mode.%d addrhuff.%d vs %d after update_bitstream_file\n",U.script.mode,U.script.addrind,sp->U.script.addrind);
                } else sp = 0;
                expand_nxt64bits(valuestr,value);
                memset(&U,0,sizeof(U));
                U.value = value;
                valp = update_bitstream_file(V,&createdflag,V->bfps[V->valuebfp],bp->blocknum,&value,sizeof(value),valuestr,&U,-1);
                //if ( 0 && V->disp != 0 )
                //    sprintf(V->disp+strlen(V->disp),"{A%d T%d.%d S%d %.8f} ",V->addrind,V->txind,bp->v,V->scriptind,dstr(value));
                if ( tp != 0 && addrp != 0 && sp != 0 )
                {
                    set_voutinfo(&vout,tp->huffind>>4,bp->v,addrp->huffind>>4,value,sp->huffind>>4);
                    printf("output tp.%d v.%d A.%d s.%d (%d) %.8f | numvins.%d inputs %.8f numvouts.%d %.8f [%.8f]\n",vout.tp_ind,vout.vout,vout.addr_ind,vout.sp_ind,sp->U.script.addrind,dstr(vout.value),numvins,dstr(inputsum),numvouts,dstr(remainder),dstr(remainder)-dstr(value));
                    memset(&U,0,sizeof(U));
                    update_bitstream_file(V,&createdflag,V->bfps[V->voutsbfp],bp->blocknum,&vout,sizeof(vout),0,&U,-1);
                }
            }
            else
            {
                if ( 0 && V->disp != 0 )
                    sprintf(V->disp+strlen(V->disp),"[%d %d %d] ",bp->blocknum,bp->txind,bp->v);
                update_vinsbfp(V,V->bfps[V->vinsbfp],bp,V->blocknum);
            }
            V->maxitems++;
        }
        else
        {
            // vin txid:vin is dereferenced above
            //sprintf(V->disp+strlen(V->disp),"(%d %d %d) ",bp->blocknum,bp->txind,bp->v);
        }
        //if ( IS_LIBTEST != 7 )
        //    fclose(V->fp);
    }
}

int32_t _get_txvins(struct rawblock *raw,struct rawtx *tx,struct coin_info *cp,cJSON *vinsobj)
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
            if ( strlen(txidstr) < sizeof(v->txidstr)-1 )
                strcpy(v->txidstr,txidstr);
            //printf("numraw.%d vin.%d (%s).v%d\n",raw->numrawvins,i,v->txidstr,v->vout);
        }
    } else printf("error with vins\n");
    tx->numvins = numvins;
    return(numvins);
}

int32_t _get_txvouts(struct rawblock *raw,struct rawtx *tx,struct coin_info *cp,cJSON *voutsobj)
{
    int32_t extract_txvals(char *coinaddr,char *script,int32_t nohexout,cJSON *txobj);
    cJSON *item;
    uint64_t value;
    struct rawvout *v;
    int32_t i,numvouts = 0;
    char coinaddr[8192],script[8192];
    tx->firstvout = raw->numrawvouts;
    if ( voutsobj != 0 && is_cJSON_Array(voutsobj) != 0 && (numvouts= cJSON_GetArraySize(voutsobj)) > 0 && tx->firstvout+numvouts < MAX_BLOCKTX )
    {
        for (i=0; i<numvouts; i++,raw->numrawvouts++)
        {
            item = cJSON_GetArrayItem(voutsobj,i);
            if ( (value = conv_cJSON_float(item,"value")) > 0 )
            {
                v = &raw->voutspace[raw->numrawvouts];
                memset(v,0,sizeof(*v));
                v->value = value;
                extract_txvals(coinaddr,script,1,item); // default to nohexout
                if ( strlen(coinaddr) < sizeof(v->coinaddr)-1 )
                    strcpy(v->coinaddr,coinaddr);//,sizeof(raw->voutspace[numrawvouts].coinaddr));
                if ( strlen(script) < sizeof(v->script)-1 )
                    strcpy(v->script,script);
                //printf("rawnum.%d vout.%d (%s) script.(%s) %.8f\n",raw->numrawvouts,i,v->coinaddr,v->script,dstr(v->value));
            }
        }
    } else printf("error with vouts\n");
    tx->numvouts = numvouts;
    return(numvouts);
}

void _get_txidinfo(struct rawblock *raw,struct rawtx *tx,struct coin_info *cp,int32_t txind,char *txidstr)
{
    char *get_transaction(struct coin_info *cp,char *txidstr);
    char *retstr = 0;
    cJSON *txjson;
    tx->numvouts = tx->numvins = 0;
    if ( (retstr= get_transaction(cp,txidstr)) != 0 )
    {
        if ( (txjson= cJSON_Parse(retstr)) != 0 )
        {
            _get_txvins(raw,tx,cp,cJSON_GetObjectItem(txjson,"vin"));
            _get_txvouts(raw,tx,cp,cJSON_GetObjectItem(txjson,"vout"));
            free_json(txjson);
        } else printf("update_txid_infos parse error.(%s)\n",retstr);
        free(retstr);
    } else printf("error getting.(%s)\n",txidstr);
    //printf("tx.%d: numvins.%d numvouts.%d (raw %d %d)\n",txind,tx->numvins,tx->numvouts,raw->numrawvins,raw->numrawvouts);
}

uint32_t _get_blockinfo(struct rawblock *raw,struct coin_info *cp,uint32_t blockheight)
{
    cJSON *get_blockjson(uint32_t *heightp,struct coin_info *cp,char *blockhashstr,uint32_t blocknum);
    cJSON *_get_blocktxarray(uint32_t *blockidp,int32_t *numtxp,struct coin_info *cp,cJSON *blockjson);
    char txidstr[8192],mintedstr[8192];
    cJSON *json,*txobj;
    uint32_t blockid;
    int32_t txind,n;
    raw->blocknum = blockheight;
    raw->minted = raw->numtx = raw->numrawvins = raw->numrawvouts = 0;
    if ( (json= get_blockjson(0,cp,0,blockheight)) != 0 )
    {
        copy_cJSON(mintedstr,cJSON_GetObjectItem(json,"mint"));
        if ( mintedstr[0] != 0 )
            raw->minted = (uint64_t)(atof(mintedstr) * SATOSHIDEN);
        if ( (txobj= _get_blocktxarray(&blockid,&n,cp,json)) != 0 && blockid == blockheight && n < MAX_BLOCKTX )
        {
            for (txind=0; txind<n; txind++)
            {
                copy_cJSON(txidstr,cJSON_GetArrayItem(txobj,txind));
                _get_txidinfo(raw,&raw->txspace[raw->numtx++],cp,txind,txidstr);
            }
        } else printf("error _get_blocktxarray for block.%d got %d, n.%d vs %d\n",blockheight,blockid,n,MAX_BLOCKTX);
        free_json(json);
    } else printf("get_blockjson error parsing.(%s)\n",txidstr);
    //printf("block.%d numtx.%d rawnumvins.%d rawnumvouts.%d\n",raw->blocknum,raw->numtx,raw->numrawvins,raw->numrawvouts);
    return(raw->numtx);
}

char *SuperNET_url()
{
    static char urls[2][64];
    sprintf(urls[0],"http://127.0.0.1:%d",SUPERNET_PORT+1);
    sprintf(urls[1],"https://127.0.0.1:%d",SUPERNET_PORT);
    return(urls[USESSL]);
}

cJSON *SuperAPI(char *cmd,char *field0,char *arg0,char *field1,char *arg1)
{
    cJSON *json = 0;
    char params[1024],*retstr;
    if ( field0 != 0 && field0[0] != 0 )
    {
        if ( field1 != 0 && field1[0] != 0 )
            sprintf(params,"{\"requestType\":\"%s\",\"%s\":\"%s\",\"%s\":\"%s\"}",cmd,field0,arg0,field1,arg1);
        else sprintf(params,"{\"requestType\":\"%s\",\"%s\":\"%s\"}",cmd,field0,arg0);
    }
    else sprintf(params,"{\"requestType\":\"%s\"}",cmd);
    retstr = bitcoind_RPC(0,(char *)"BTCD",SuperNET_url(),(char *)"",(char *)"SuperNET",params);
    if ( retstr != 0 )
    {
        json = cJSON_Parse(retstr);
        free(retstr);
    }
    return(json);
}

char *GUIpoll(char *txidstr,char *senderipaddr,uint16_t *portp)
{
    void unstringify(char *);
    char params[4096],buf[1024],buf2[1024],ipaddr[64],args[8192],*retstr;
    int32_t port;
    cJSON *json,*argjson;
    txidstr[0] = 0;
    sprintf(params,"{\"requestType\":\"GUIpoll\"}");
    retstr = bitcoind_RPC(0,(char *)"BTCD",SuperNET_url(),(char *)"",(char *)"SuperNET",params);
    //fprintf(stderr,"<<<<<<<<<<< SuperNET poll_for_broadcasts: issued bitcoind_RPC params.(%s) -> retstr.(%s)\n",params,retstr);
    if ( retstr != 0 )
    {
        //sprintf(retbuf+sizeof(ptrs),"{\"result\":%s,\"from\":\"%s\",\"port\":%d,\"args\":%s}",str,ipaddr,port,args);
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            copy_cJSON(buf,cJSON_GetObjectItem(json,"result"));
            if ( buf[0] != 0 )
            {
                unstringify(buf);
                copy_cJSON(txidstr,cJSON_GetObjectItem(json,"txid"));
                if ( txidstr[0] != 0 )
                {
                    if ( Debuglevel > 0 )
                        fprintf(stderr,"<<<<<<<<<<< GUIpoll: (%s) for [%s]\n",buf,txidstr);
                }
                else
                {
                    copy_cJSON(ipaddr,cJSON_GetObjectItem(json,"from"));
                    copy_cJSON(args,cJSON_GetObjectItem(json,"args"));
                    port = (int32_t)get_API_int(cJSON_GetObjectItem(json,"port"),0);
                    if ( args[0] != 0 )
                    {
                        unstringify(args);
                        if ( Debuglevel > 2 )
                            printf("(%s) from (%s:%d) -> (%s) Qtxid.(%s)\n",args,ipaddr,port,buf,txidstr);
                        free(retstr);
                        retstr = clonestr(args);
                        if ( (argjson= cJSON_Parse(retstr)) != 0 )
                        {
                            copy_cJSON(buf2,cJSON_GetObjectItem(argjson,"result"));
                            if ( strcmp(buf2,"nothing pending") == 0 )
                                free(retstr), retstr = 0;
                            //else printf("RESULT.(%s)\n",buf2);
                            free_json(argjson);
                        }
                    }
                }
            }
            free_json(json);
        } else fprintf(stderr,"<<<<<<<<<<< GUI poll_for_broadcasts: PARSE_ERROR.(%s)\n",retstr);
        // free(retstr);
    } //else fprintf(stderr,"<<<<<<<<<<< BTCD poll_for_broadcasts: bitcoind_RPC returns null\n");
    return(retstr);
}

char *process_commandline_json(cJSON *json)
{
    char *inject_pushtx(char *coinstr,cJSON *json);
    bits256 issue_getpubkey(int32_t *haspubkeyp,char *acct);
    char *issue_MGWstatus(int32_t mask,char *coinstr,char *userNXTaddr,char *userpubkey,char *email,int32_t rescan,int32_t actionflag);
    struct multisig_addr *decode_msigjson(char *NXTaddr,cJSON *obj,char *sender);
    int32_t send_email(char *email,char *destNXTaddr,char *pubkeystr,char *msg);
    void issue_genmultisig(char *coinstr,char *userNXTaddr,char *userpubkey,char *email,int32_t buyNXT);
    char txidstr[1024],senderipaddr[1024],cmd[2048],coin[2048],userpubkey[2048],NXTacct[2048],userNXTaddr[2048],email[2048],convertNXT[2048],retbuf[1024],buf2[1024],coinstr[1024],cmdstr[512],*retstr = 0,*waitfor = 0,errstr[2048],*str;
    bits256 pubkeybits;
    unsigned char hash[256>>3],mypublic[256>>3];
    uint16_t port;
    uint64_t nxt64bits,checkbits,deposit_pending = 0;
    int32_t i,n,haspubkey,iter,gatewayid,actionflag = 0,rescan = 1;
    uint32_t buyNXT = 0;
    cJSON *array,*argjson,*retjson,*retjsons[3];
    copy_cJSON(cmdstr,cJSON_GetObjectItem(json,"webcmd"));
    if ( strcmp(cmdstr,"SuperNET") == 0 )
    {
        str = cJSON_Print(json);
        //printf("GOT webcmd.(%s)\n",str);
        retstr = bitcoind_RPC(0,"webcmd",SuperNET_url(),(char *)"",(char *)"SuperNET",str);
        free(str);
        return(retstr);
    }
    copy_cJSON(coin,cJSON_GetObjectItem(json,"coin"));
    copy_cJSON(cmd,cJSON_GetObjectItem(json,"requestType"));
    if ( strcmp(cmd,"pushtx") == 0 )
        return(inject_pushtx(coin,json));
    copy_cJSON(email,cJSON_GetObjectItem(json,"email"));
    copy_cJSON(NXTacct,cJSON_GetObjectItem(json,"NXT"));
    copy_cJSON(userpubkey,cJSON_GetObjectItem(json,"pubkey"));
    if ( userpubkey[0] == 0 )
    {
        pubkeybits = issue_getpubkey(&haspubkey,NXTacct);
        if ( haspubkey != 0 )
            init_hexbytes_noT(userpubkey,pubkeybits.bytes,sizeof(pubkeybits.bytes));
    }
    copy_cJSON(convertNXT,cJSON_GetObjectItem(json,"convertNXT"));
    if ( convertNXT[0] != 0 )
        buyNXT = (uint32_t)atol(convertNXT);
    nxt64bits = conv_acctstr(NXTacct);
    expand_nxt64bits(userNXTaddr,nxt64bits);
    decode_hex(mypublic,sizeof(mypublic),userpubkey);
    calc_sha256(0,hash,mypublic,32);
    memcpy(&checkbits,hash,sizeof(checkbits));
    if ( checkbits != nxt64bits )
    {
        sprintf(retbuf,"{\"error\":\"invalid pubkey\",\"pubkey\":\"%s\",\"NXT\":\"%s\",\"checkNXT\":\"%llu\"}",userpubkey,userNXTaddr,(long long)checkbits);
        return(clonestr(retbuf));
    }
    memset(retjsons,0,sizeof(retjsons));
    cmdstr[0] = 0;
    //printf("got cmd.(%s)\n",cmd);
    if ( strcmp(cmd,"newbie") == 0 )
    {
        waitfor = "MGWaddr";
        strcpy(cmdstr,cmd);
        array = cJSON_GetObjectItem(MGWconf,"active");
        if ( array != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
        {
            for (i=0; i<100; i++) // flush queue
                GUIpoll(txidstr,senderipaddr,&port);
            for (iter=0; iter<3; iter++) // give chance for servers to consensus
            {
                for (i=0; i<n; i++)
                {
                    copy_cJSON(coinstr,cJSON_GetArrayItem(array,i));
                    if ( coinstr[0] != 0 )
                    {
                        issue_genmultisig(coinstr,userNXTaddr,userpubkey,email,buyNXT);
                        sleep(1);
                    }
                }
                sleep(1);
            }
        }
    }
    else if ( strcmp(cmd,"status") == 0 )
    {
        waitfor = "MGWresponse";
        strcpy(cmdstr,cmd);
        //printf("cmdstr.(%s) waitfor.(%s)\n",cmdstr,waitfor);
        retstr = issue_MGWstatus((1<<NUM_GATEWAYS)-1,coin,userNXTaddr,userpubkey,0,rescan,actionflag);
        if ( retstr != 0 )
            free(retstr), retstr = 0;
    }
    else return(clonestr("{\"error\":\"only newbie command is supported now\"}"));
    if ( waitfor != 0 )
    {
        for (i=0; i<3000; i++)
        {
            if ( (retstr= GUIpoll(txidstr,senderipaddr,&port)) != 0 )
            {
                if ( retstr[0] == '[' || retstr[0] == '{' )
                {
                    if ( (retjson= cJSON_Parse(retstr)) != 0 )
                    {
                        if ( is_cJSON_Array(retjson) != 0 && (n= cJSON_GetArraySize(retjson)) == 2 )
                        {
                            argjson = cJSON_GetArrayItem(retjson,0);
                            copy_cJSON(buf2,cJSON_GetObjectItem(argjson,"requestType"));
                            gatewayid = (int32_t)get_API_int(cJSON_GetObjectItem(argjson,"gatewayid"),-1);
                            if ( gatewayid >= 0 && gatewayid < 3 && retjsons[gatewayid] == 0 )
                            {
                                copy_cJSON(errstr,cJSON_GetObjectItem(argjson,"error"));
                                if ( strlen(errstr) > 0 || strcmp(buf2,waitfor) == 0 )
                                {
                                    retjsons[gatewayid] = retjson, retjson = 0;
                                    if ( retjsons[0] != 0 && retjsons[1] != 0 && retjsons[2] != 0 )
                                        break;
                                }
                            }
                        }
                        if ( retjson != 0 )
                            free_json(retjson);
                    }
                }
                //fprintf(stderr,"(%p) %s\n",retjson,retstr);
                free(retstr),retstr = 0;
            } else usleep(3000);
        }
    }
    for (i=0; i<3; i++)
        if ( retjsons[i] == 0 )
            break;
    if ( i < 3 && cmdstr[0] != 0 )
    {
        for (i=0; i<3; i++)
        {
            if ( retjsons[i] == 0 && (retstr= issue_curl(0,cmdstr)) != 0 )
            {
                /*printf("(%s) -> (%s)\n",cmdstr,retstr);
                 if ( (msigjson= cJSON_Parse(retstr)) != 0 )
                 {
                 if ( is_cJSON_Array(msigjson) != 0 && (n= cJSON_GetArraySize(msigjson)) > 0 )
                 {
                 for (j=0; j<n; j++)
                 {
                 item = cJSON_GetArrayItem(msigjson,j);
                 copy_cJSON(coinstr,cJSON_GetObjectItem(item,"coin"));
                 if ( strcmp(coinstr,xxx) == 0 )
                 {
                 msig = decode_msigjson(0,item,Server_NXTaddrs[i]);
                 break;
                 }
                 }
                 }
                 else msig = decode_msigjson(0,msigjson,Server_NXTaddrs[i]);
                 if ( msig != 0 )
                 {
                 free(msig);
                 free_json(msigjson);
                 if ( email[0] != 0 )
                 send_email(email,userNXTaddr,0,retstr);
                 //printf("[%s]\n",retstr);
                 return(retstr);
                 }
                 } else printf("error parsing.(%s)\n",retstr);
                 free_json(msigjson);
                 free(retstr);*/
                if ( retstr[0] == '{' || retstr[0] == '[' )
                {
                    //if ( email[0] != 0 )
                    //    send_email(email,userNXTaddr,0,retstr);
                    //return(retstr);
                    retjsons[i] = cJSON_Parse(retstr);
                }
                free(retstr);
            } //else printf("cant find (%s)\n",cmdstr);
        }
    }
    json = cJSON_CreateArray();
    for (i=0; i<3; i++)
    {
        char *load_filestr(char *userNXTaddr,int32_t gatewayid);
        char *filestr;
        if ( retjsons[i] == 0 && userNXTaddr[0] != 0 && (filestr= load_filestr(userNXTaddr,i)) != 0 )
        {
            retjsons[i] = cJSON_Parse(filestr);
            printf(">>>>>>>>>>>>>>> load_filestr!!! %s.%d json.%p\n",userNXTaddr,i,json);
        }
        if ( retjsons[i] != 0 )
            cJSON_AddItemToArray(json,retjsons[i]);
    }
    if ( deposit_pending != 0 )
    {
        actionflag = 1;
        rescan = 0;
        retstr = issue_MGWstatus(1<<NUM_GATEWAYS,coin,0,0,0,rescan,actionflag);
        if ( retstr != 0 )
            free(retstr), retstr = 0;
    }
    retstr = cJSON_Print(json);
    free_json(json);
    if ( email[0] != 0 )
        send_email(email,userNXTaddr,0,retstr);
    for (i=0; i<1000; i++)
    {
        if ( (str= GUIpoll(txidstr,senderipaddr,&port)) != 0 )
            free(str);
        else break;
    }
    return(retstr);
}

char *load_filestr(char *userNXTaddr,int32_t gatewayid)
{
    long fpos;
    FILE *fp;
    char fname[1024],*buf=0,*retstr = 0;
    sprintf(fname,"%s/gateway%d/%s",MGWROOT,gatewayid,userNXTaddr);
    if ( (fp= fopen(fname,"rb")) != 0 )
    {
        fseek(fp,0,SEEK_END);
        fpos = ftell(fp);
        if ( fpos > 0 )
        {
            rewind(fp);
            buf = calloc(1,fpos);
            if ( fread(buf,1,fpos,fp) == fpos )
                retstr = buf, buf = 0;
        }
        fclose(fp);
        if ( buf != 0 )
            free(buf);
    }
    return(retstr);
}

void bridge_handler(struct transfer_args *args)
{
    FILE *fp;
    int32_t gatewayid = -1;
    char fname[1024],cmd[1024],*name = args->name;
    if ( strncmp(name,"MGW",3) == 0 && name[3] >= '0' && name[3] <= '2' )
    {
        gatewayid = (name[3] - '0');
        name += 5;
        sprintf(fname,"%s/gateway%d/%s",MGWROOT,gatewayid,name);
        if ( (fp= fopen(fname,"wb+")) != 0 )
        {
            fwrite(args->data,1,args->totallen,fp);
            fclose(fp);
            sprintf(cmd,"chmod +r %s",fname);
            system(cmd);
        }
    }
    printf("bridge_handler.gateway%d/(%s).%d\n",gatewayid,name,args->totallen);
}

void *GUIpoll_loop(void *arg)
{
    uint16_t port;
    char txidstr[1024],senderipaddr[1024],*retstr;
    while ( 1 )
    {
        sleep(1);
        if ( (retstr= GUIpoll(txidstr,senderipaddr,&port)) != 0 )
            free(retstr);
    }
    return(0);
}

// redirect port on external upnp enabled router to port on *this* host
int upnpredirect(const char* eport, const char* iport, const char* proto, const char* description) {
    
    //  Discovery parameters
    struct UPNPDev * devlist = 0;
    struct UPNPUrls urls;
    struct IGDdatas data;
    int i;
    char lanaddr[64];	// my ip address on the LAN
    const char* leaseDuration="0";
    
    //  Redirect & test parameters
    char intClient[40];
    char intPort[6];
    char externalIPAddress[40];
    char duration[16];
    int error=0;
    
    //  Find UPNP devices on the network
    if ((devlist=upnpDiscover(2000, 0, 0,0, 0, &error))) {
        struct UPNPDev * device = 0;
        printf("UPNP INIALISED: List of UPNP devices found on the network.\n");
        for(device = devlist; device; device = device->pNext) {
            printf("UPNP INFO: dev [%s] \n\t st [%s]\n",
                   device->descURL, device->st);
        }
    } else {
        printf("UPNP ERROR: no device found - MANUAL PORTMAP REQUIRED\n");
        return 0;
    }
    
    //  Output whether we found a good one or not.
    if((error = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr)))) {
        switch(error) {
            case 1:
                printf("UPNP OK: Found valid IGD : %s\n", urls.controlURL);
                break;
            case 2:
                printf("UPNP WARN: Found a (not connected?) IGD : %s\n", urls.controlURL);
                break;
            case 3:
                printf("UPNP WARN: UPnP device found. Is it an IGD ? : %s\n", urls.controlURL);
                break;
            default:
                printf("UPNP WARN: Found device (igd ?) : %s\n", urls.controlURL);
        }
        printf("UPNP OK: Local LAN ip address : %s\n", lanaddr);
    } else {
        printf("UPNP ERROR: no device found - MANUAL PORTMAP REQUIRED\n");
        return 0;
    }
    
    //  Get the external IP address (just because we can really...)
    if(UPNP_GetExternalIPAddress(urls.controlURL,
                                 data.first.servicetype,
                                 externalIPAddress)!=UPNPCOMMAND_SUCCESS)
        printf("UPNP WARN: GetExternalIPAddress failed.\n");
    else
        printf("UPNP OK: ExternalIPAddress = %s\n", externalIPAddress);
    
    //  Check for existing supernet mapping - from this host and another host
    //  In theory I can adapt this so multiple nodes can exist on same lan and choose a different portmap
    //  for each one :)
    //  At the moment just delete a conflicting portmap and override with the one requested.
    i=0;
    error=0;
    do {
        char index[6];
        char extPort[6];
        char desc[80];
        char enabled[6];
        char rHost[64];
        char protocol[4];
        
        snprintf(index, 6, "%d", i++);
        
        if(!(error=UPNP_GetGenericPortMappingEntry(urls.controlURL,
                                                   data.first.servicetype,
                                                   index,
                                                   extPort, intClient, intPort,
                                                   protocol, desc, enabled,
                                                   rHost, duration))) {
            // printf("%2d %s %5s->%s:%-5s '%s' '%s' %s\n",i, protocol, extPort, intClient, intPort,desc, rHost, duration);
            
            // check for an existing supernet mapping on this host
            if(!strcmp(lanaddr, intClient)) { // same host
                if(!strcmp(protocol,proto)) { //same protocol
                    if(!strcmp(intPort,iport)) { // same port
                        printf("UPNP WARN: existing mapping found (%s:%s)\n",lanaddr,iport);
                        if(!strcmp(extPort,eport)) {
                            printf("UPNP OK: exact mapping already in place (%s:%s->%s)\n", lanaddr, iport, eport);
                            FreeUPNPUrls(&urls);
                            freeUPNPDevlist(devlist);
                            return 1;
                            
                        } else { // delete old mapping
                            printf("UPNP WARN: deleting existing mapping (%s:%s->%s)\n",lanaddr, iport, extPort);
                            if(UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, extPort, proto, rHost))
                                printf("UPNP WARN: error deleting old mapping (%s:%s->%s) continuing\n", lanaddr, iport, extPort);
                            else printf("UPNP OK: old mapping deleted (%s:%s->%s)\n",lanaddr, iport, extPort);
                        }
                    }
                }
            } else { // ipaddr different - check to see if requested port is already mapped
                if(!strcmp(protocol,proto)) {
                    if(!strcmp(extPort,eport)) {
                        printf("UPNP WARN: EXT port conflict mapped to another ip (%s-> %s vs %s)\n", extPort, lanaddr, intClient);
                        if(UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, extPort, proto, rHost))
                            printf("UPNP WARN: error deleting conflict mapping (%s:%s) continuing\n", intClient, extPort);
                        else printf("UPNP OK: conflict mapping deleted (%s:%s)\n",intClient, extPort);
                    }
                }
            }
        } else
            printf("UPNP OK: GetGenericPortMappingEntry() End-of-List (%d entries) \n", i);
    } while(error==0);
    
    //  Set the requested port mapping
    if((i=UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                              eport, iport, lanaddr, description,
                              proto, 0, leaseDuration))!=UPNPCOMMAND_SUCCESS) {
        printf("UPNP ERROR: AddPortMapping(%s, %s, %s) failed with code %d (%s)\n",
               eport, iport, lanaddr, i, strupnperror(i));
        
        FreeUPNPUrls(&urls);
        freeUPNPDevlist(devlist);
        return 0; //error - adding the port map primary failure
    }
    
    if((i=UPNP_GetSpecificPortMappingEntry(urls.controlURL,
                                           data.first.servicetype,
                                           eport, proto, NULL/*remoteHost*/,
                                           intClient, intPort, NULL/*desc*/,
                                           NULL/*enabled*/, duration))!=UPNPCOMMAND_SUCCESS) {
        printf("UPNP ERROR: GetSpecificPortMappingEntry(%s, %s, %s) failed with code %d (%s)\n", eport, iport, lanaddr,
               i, strupnperror(i));
        FreeUPNPUrls(&urls);
        freeUPNPDevlist(devlist);
        return 0; //error - port map wasnt returned by query so likely failed.
    }
    else printf("UPNP OK: EXT (%s:%s) %s redirected to INT (%s:%s) (duration=%s)\n",externalIPAddress, eport, proto, intClient, intPort, duration);
    FreeUPNPUrls(&urls);
    freeUPNPDevlist(devlist);
    return 1; //ok - we are mapped:)
}

int main(int argc,const char *argv[])
{
    FILE *fp;
    cJSON *json = 0;
    int32_t retval;
    char ipaddr[64],*oldport,*newport,portstr[64],*retstr;
#ifdef __APPLE__
#else
    if ( 1 && argc > 1 && strcmp(argv[1],"genfiles") == 0 )
#endif
    {
        void process_coinblocks(char *argcoinstr);
        char *coinstr;
        retval = SuperNET_start("SuperNET.conf","127.0.0.1");
        printf("process coinblocks\n");
        if ( argc > 2 )
            coinstr = (char *)argv[2];
        else coinstr = "BTCD";
        //scan_ramchain(V);
        process_coinblocks(coinstr);
        printf("finished genfiles.%s\n",coinstr);
        getchar();
    }
#ifdef fortesting
    if ( 0 )
    {
        void huff_iteminit(struct huffitem *hip,void *ptr,int32_t size,int32_t isptr,int32_t ishex);
        char *p,buff[1024];//,*str = "this is an example for huffman encoding";
        int i,c,n,numinds = 256;
        int probs[256];
        //struct huffcode *huff;
        struct huffitem *items = calloc(numinds,sizeof(*items));
        int testhuffcode(char *str,struct huffitem *freqs,int32_t numinds);
        for (i=0; i<numinds; i++)
            huff_iteminit(&items[i],&i,1,0,0);
        while ( 1 )
        {
            for (i=0; i<256; i++)
                probs[i] = ((rand()>>8) % 1000);
            for (i=n=0; i<128; i++)
            {
                c = (rand() >> 8) & 0xff;
                while ( c > 0 && ((rand()>>8) % 1000) > probs[c] )
                {
                    buff[n++] = (c % 64) + ' ';
                    if ( n >= sizeof(buff)-1 )
                        break;
                }
            }
            buff[n] = 0;
            for (i=0; i<numinds; i++)
                items[i].freq = 0;
            p = buff;
            while ( *p != '\0' )
                items[*p++].freq++;
            testhuffcode(0,items,numinds);
            fprintf(stderr,"*");
        }
        //getchar();
    }
#endif
    IS_LIBTEST = 1;
    if ( argc > 1 && argv[1] != 0 )
    {
        char *init_MGWconf(char *JSON_or_fname,char *myipaddr);
        //printf("ARGV1.(%s)\n",argv[1]);
        if ( (argv[1][0] == '{' || argv[1][0] == '[') )
        {
            if ( (json= cJSON_Parse(argv[1])) != 0 )
            {
                Debuglevel = IS_LIBTEST = -1;
                init_MGWconf("SuperNET.conf",0);
                if ( (retstr= process_commandline_json(json)) != 0 )
                {
                    printf("%s\n",retstr);
                    free(retstr);
                }
                free_json(json);
                return(0);
            }
        }
        else strcpy(ipaddr,argv[1]);
    }
    else strcpy(ipaddr,"127.0.0.1");
    retval = SuperNET_start("SuperNET.conf",ipaddr);
    sprintf(portstr,"%d",SUPERNET_PORT);
    oldport = newport = portstr;
    if ( UPNP != 0 && upnpredirect(oldport,newport,"UDP","SuperNET_https") == 0 )
        printf("TEST ERROR: failed redirect (%s) to (%s)\n",oldport,newport);
    //sprintf(portstr,"%d",SUPERNET_PORT+1);
    //oldport = newport = portstr;
    //if ( upnpredirect(oldport,newport,"UDP","SuperNET_http") == 0 )
    //    printf("TEST ERROR: failed redirect (%s) to (%s)\n",oldport,newport);
    printf("saving retval.%x (%d usessl.%d) UPNP.%d MULTIPORT.%d\n",retval,retval>>1,retval&1,UPNP,MULTIPORT);
    if ( (fp= fopen("horrible.hack","wb+")) != 0 )
    {
        fwrite(&retval,1,sizeof(retval),fp);
        fclose(fp);
    }
    if ( Debuglevel > 0 )
        system("git log | head -n 1");
    if ( retval >= 0 && ENABLE_GUIPOLL != 0 )
    {
        GUIpoll_loop(ipaddr);
        //if ( portable_thread_create((void *)GUIpoll_loop,ipaddr) == 0 )
        //    printf("ERROR hist process_hashtablequeues\n");
    }
    while ( 1 )
        sleep(60);
    return(0);
}


// stubs
int32_t SuperNET_broadcast(char *msg,int32_t duration) { return(0); }
int32_t SuperNET_narrowcast(char *destip,unsigned char *msg,int32_t len) { return(0); }

#ifdef chanc3r
#endif
