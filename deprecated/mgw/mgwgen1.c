//
//  mgwgen1.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_mgwgen1_h
#define crypto777_mgwgen1_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "../includes/cJSON.h"
#include "../includes/uthash.h"
#include "../utils/utils777.c"
#include "../coins/msig.c"

char *_sign_and_sendmoney(char *cointxid,struct ramchain_info *ram,struct cointx_info *cointx,char *othersignedtx,uint64_t *redeems,uint64_t *amounts,int32_t numredeems);
void ram_update_MGWunspents(struct ramchain_info *ram,char *addr,int32_t vout,uint32_t txid_rawind,uint32_t script_rawind,uint64_t value);

#endif
#else
#ifndef crypto777_mgwgen1_c
#define crypto777_mgwgen1_c

#ifndef crypto777_mgwgen1_h
#define DEFINES_ONLY
#include "mgwgen1.c"
#undef DEFINES_ONLY
#endif

double get_current_rate(char *base,char *rel)
{
    struct ramchain_info *ram;
    if ( strcmp(rel,"NXT") == 0 )
    {
        if ( (ram= get_ramchain_info(base)) != 0 )
        {
            if ( ram->NXTconvrate != 0. )
                return(ram->NXTconvrate);
            if ( ram->NXTfee_equiv != 0 && ram->txfee != 0 )
                return(ram->NXTfee_equiv / ram->txfee);
        }
    }
    return(1.);
}

char *_submit_withdraw(struct ramchain_info *ram,struct cointx_info *cointx,char *othersignedtx)
{
    FILE *fp;
    char fname[512],*cointxid,*signed2transaction;
    if ( ram->S.gatewayid < 0 )
        return(0);
    if ( cosigntransaction(&cointxid,&signed2transaction,ram->name,ram->serverport,ram->userpass,cointx,othersignedtx,ram->S.gatewayid,NUM_GATEWAYS) > 0 )
    {
        if ( signed2transaction != 0 && signed2transaction[0] != 0 )
        {
            if ( cointxid != 0 && cointxid[0] != 0 )
            {
                sprintf(fname,"%s/%s.%s",ram->backups,cointxid,ram->name);
                if ( (fp= fopen(os_compatible_path(fname),"w")) != 0 )
                {
                    fprintf(fp,"%s\n",signed2transaction);
                    fclose(fp);
                    printf("wrote.(%s) to file.(%s)\n",signed2transaction,fname);
                }
                else printf("unexpected %s cointxid.%s already there before submit??\n",ram->name,cointxid);
                printf("rawtxid len.%ld submitted.%s\n",strlen(signed2transaction),cointxid);
                free(signed2transaction);
                return(clonestr(cointxid));
            } else printf("error null cointxid\n");
        } else printf("error submit raw.%s\n",signed2transaction);
    }
    return(0);
}

char *_sign_and_sendmoney(char *cointxid,struct ramchain_info *ram,struct cointx_info *cointx,char *othersignedtx,uint64_t *redeems,uint64_t *amounts,int32_t numredeems)
{
    uint64_t get_sender(uint64_t *amountp,char *txidstr);
    void *extract_jsonkey(cJSON *item,void *arg,void *arg2);
    void set_MGW_moneysentfname(char *fname,char *NXTaddr);
    int32_t jsonstrcmp(void *ref,void *item);
    char txidstr[64],NXTaddr[64],jsonstr[4096],*retstr = 0;
    int32_t i;
    uint64_t amount,senderbits,redeemtxid;
    fprintf(stderr,"achieved consensus and sign! (%s)\n",othersignedtx);
    if ( (retstr= _submit_withdraw(ram,cointx,othersignedtx)) != 0 )
    {
        if ( is_hexstr(retstr) != 0 )
        {
            strcpy(cointxid,retstr);
            //*AMtxidp = _broadcast_moneysentAM(ram,height);
            for (i=0; i<numredeems; i++)
            {
                if ( (redeemtxid = redeems[i]) != 0 && amounts[i] != 0 )
                {
                    printf("signed and sent.%d: %llu %.8f\n",i,(long long)redeemtxid,dstr(amounts[i]));
                    _ram_update_redeembits(ram,redeemtxid,0,cointxid,0);
                    expand_nxt64bits(txidstr,redeemtxid);
                    senderbits = get_sender(&amount,txidstr);
                    expand_nxt64bits(NXTaddr,senderbits);
                    sprintf(jsonstr,"{\"NXT\":\"%s\",\"redeemtxid\":\"%llu\",\"amount\":\"%.8f\",\"coin\":\"%s\",\"cointxid\":\"%s\",\"vout\":\"%d\"}",NXTaddr,(long long)redeemtxid,dstr(amounts[i]),ram->name,txidstr,i);
                    update_MGW_jsonfile(set_MGW_moneysentfname,extract_jsonkey,jsonstrcmp,0,jsonstr,"redeemtxid",0);
                    update_MGW_jsonfile(set_MGW_moneysentfname,extract_jsonkey,jsonstrcmp,NXTaddr,jsonstr,"redeemtxid",0);
                }
            }
            //backupwallet(cp,ram->coinid);
        }
        else
        {
            for (i=0; i<numredeems; i++)
                printf("(%llu %.8f) ",(long long)redeems[i],dstr(amounts[i]));
            printf("_sign_and_sendmoney: unexpected return.(%s)\n",retstr);
            exit(1);
        }
        return(retstr);
    }
    else printf("sign_and_sendmoney: error sending rawtransaction %s\n",othersignedtx);
    return(0);
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


void ram_update_MGWunspents(struct ramchain_info *ram,char *addr,int32_t vout,uint32_t txid_rawind,uint32_t script_rawind,uint64_t value)
{
    struct cointx_input *vin;
    if ( ram->MGWnumunspents >= ram->MGWmaxunspents )
    {
        ram->MGWmaxunspents += 512;
        ram->MGWunspents = realloc(ram->MGWunspents,sizeof(*ram->MGWunspents) * ram->MGWmaxunspents);
        memset(&ram->MGWunspents[ram->MGWnumunspents],0,(ram->MGWmaxunspents - ram->MGWnumunspents) * sizeof(*ram->MGWunspents));
    }
    vin = &ram->MGWunspents[ram->MGWnumunspents++];
    if ( ram_script(vin->sigs,ram,script_rawind) != 0 && ram_txid(vin->tx.txidstr,ram,txid_rawind) != 0 )
    {
        strcpy(vin->coinaddr,addr);
        vin->value = value;
        vin->tx.vout = vout;
    } else printf("ram_update_MGWunspents: decode error for (%s) vout.%d txid.%d script.%d (%.8f)\n",addr,vout,txid_rawind,script_rawind,dstr(value));
}

int32_t ram_markspent(struct ramchain_info *ram,struct rampayload *txpayload,struct address_entry *spendbp,uint32_t txid_rawind)
{ // bitcoin
    struct rampayload *ram_getpayloadi(struct ramchain_hashptr **addrptrp,struct ramchain_info *ram,char type,uint32_t rawind,uint32_t i);
    struct ramchain_hashptr *addrptr;
    struct rampayload *addrpayload;
    txpayload->spentB = *spendbp; // now all fields are set for the payloads from the tx
    // now we need to update the addrpayload
    if ( (addrpayload= ram_getpayloadi(&addrptr,ram,'a',txpayload->otherind,txpayload->extra)) != 0 )
    {
        if ( txid_rawind == addrpayload->otherind && txpayload->value == addrpayload->value )
        {
            if ( txpayload->value != 0 )
            {
                ram->S.totalspends += txpayload->value;
                ram->S.numspends++;
                addrpayload->spentB = *spendbp;
                addrpayload->B.spent = 1;
                addrptr->unspent -= addrpayload->value;
                addrptr->numunspent--;
                //printf("SPENT %.8f\n",dstr(addrpayload->value));
                //printf("MATCH: spendtxid_rawind.%u == %u addrpayload->otherind for (%d %d %d)\n",txid_rawind,addrpayload->otherind,spendbp->blocknum,spendbp->txind,spendbp->v);
            }
        } else printf("FATAL: txpayload.%p[%d] addrpayload.%p spendtxid_rawind.%u != %u addrpayload->otherind for (%d %d %d)\n",txpayload,txpayload->extra,addrpayload,txid_rawind,addrpayload->otherind,spendbp->blocknum,spendbp->txind,spendbp->v);
    } else printf("FATAL: ram_markspent cant find addpayload (%d %d %d) addrind.%d i.%d | txpayload.%p txid_rawind.%u\n",spendbp->blocknum,spendbp->txind,spendbp->v,txpayload->otherind,txpayload->extra,txpayload,txid_rawind);
    if ( addrpayload->value != txpayload->value )
        printf("FATAL: addrpayload %.8f vs txpayload %.8f\n",dstr(addrpayload->value),dstr(txpayload->value));
    return(-1);
}

#endif
#endif
