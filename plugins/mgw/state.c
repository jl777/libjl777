//
//  mgwstate.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_mgwstate_h
#define crypto777_mgwstate_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"
#include "system777.c"
#include "NXT777.c"
#include "bits777.c"
#include "search.c"
#include "storage.c"

#define GET_COINDEPOSIT_ADDRESS 'g'
#define BIND_DEPOSIT_ADDRESS 'b'
#define DEPOSIT_CONFIRMED 'd'
#define MONEY_SENT 'm'
#define NXT_ASSETLIST_INCR 16

#endif
#else
#ifndef crypto777_mgwstate_c
#define crypto777_mgwstate_c

#ifndef crypto777_mgwstate_h
#define DEFINES_ONLY
#include "state.c"
#undef DEFINES_ONLY
#endif

int32_t _valid_txamount(struct ramchain_info *ram,uint64_t value,char *coinaddr)
{
    if ( value >= MIN_DEPOSIT_FACTOR * (ram->txfee + ram->NXTfee_equiv) )
    {
        if ( coinaddr == 0 || (strcmp(coinaddr,ram->marker) != 0 && strcmp(coinaddr,ram->marker2) != 0) )
            return(1);
    }
    return(0);
}

int32_t _is_limbo_redeem(struct ramchain_info *ram,uint64_t redeemtxidbits)
{
    int32_t j;
    if ( ram->limboarray != 0 )
    {
        for (j=0; ram->limboarray[j]!=0; j++)
            if ( redeemtxidbits == ram->limboarray[j] )
            {
                printf("redeemtxid.%llu in slot.%d\n",(long long)redeemtxidbits,j);
                return(1);
            }
    } else printf("no limboarray for %s?\n",ram->name);
    return(0);
}

void _complete_assettxid(struct NXT_assettxid *tp)
{
    if ( tp->completed == 0 && tp->redeemstarted != 0 )
    {
        printf("pending redeem %llu %.8f completed | elapsed %d seconds\n",(long long)tp->redeemtxid,dstr(tp->U.assetoshis),(uint32_t)(time(NULL) - tp->redeemstarted));
        tp->redeemstarted = (uint32_t)(time(NULL) - tp->redeemstarted);
    }
    tp->completed = 1;
}

struct MGWstate *ram_select_MGWstate(struct ramchain_info *ram,int32_t selector)
{
    struct MGWstate *sp = 0;
    if ( ram != 0 )
    {
        if ( selector < 0 )
            sp = &ram->S;
        else if ( selector < ram->numgateways )
            sp = &ram->otherS[selector];
    }
    return(sp);
}

void ram_set_MGWpingstr(char *pingstr,struct ramchain_info *ram,int32_t selector)
{
    struct MGWstate *sp = ram_select_MGWstate(ram,selector);
    if ( ram != 0 && ram->S.gatewayid >= 0 )
    {
        ram->S.supply = (ram->S.totaloutputs - ram->S.totalspends);
        ram->otherS[ram->S.gatewayid] = ram->S;
    }
    if ( sp != 0 )
    {
        sprintf(pingstr,"\"coin\":\"%s\",\"gatewayid\":\"%d\",\"balance\":\"%llu\",\"sentNXT\":\"%llu\",\"unspent\":\"%llu\",\"supply\":\"%llu\",\"circulation\":\"%llu\",\"pendingredeems\":\"%llu\",\"pendingdeposits\":\"%llu\",\"internal\":\"%llu\",\"RTNXT\":{\"height\":\"%d\",\"lag\":\"%d\",\"ECblock\":\"%llu\",\"ECheight\":\"%u\"},\"%s\":{\"permblocks\":\"%d\",\"height\":\"%d\",\"lag\":\"%d\"},",ram->name,sp->gatewayid,(long long)(sp->MGWbalance),(long long)(sp->sentNXT),(long long)(sp->MGWunspent),(long long)(sp->supply),(long long)(sp->circulation),(long long)(sp->MGWpendingredeems),(long long)(sp->MGWpendingdeposits),(long long)(sp->orphans),sp->NXT_RTblocknum,sp->NXT_RTblocknum-sp->NXTblocknum,(long long)sp->NXT_ECblock,sp->NXT_ECheight,sp->name,sp->permblocks,sp->RTblocknum,sp->RTblocknum - sp->blocknum);
    }
}

void ram_set_MGWdispbuf(char *dispbuf,struct ramchain_info *ram,int32_t selector)
{
    struct MGWstate *sp = ram_select_MGWstate(ram,selector);
    if ( sp != 0 )
    {
        sprintf(dispbuf,"[%.8f %s - %.0f NXT rate %.2f] msigs.%d unspent %.8f circ %.8f/%.8f pend.(W%.8f D%.8f) NXT.%d %s.%d\n",dstr(sp->MGWbalance),ram->name,dstr(sp->sentNXT),sp->MGWbalance<=0?0:dstr(sp->sentNXT)/dstr(sp->MGWbalance),ram->nummsigs,dstr(sp->MGWunspent),dstr(sp->circulation),dstr(sp->supply),dstr(sp->MGWpendingredeems),dstr(sp->MGWpendingdeposits),sp->NXT_RTblocknum,ram->name,sp->RTblocknum);
    }
}

void ram_get_MGWpingstr(struct ramchain_info *ram,char *MGWpingstr,int32_t selector)
{
    MGWpingstr[0] = 0;
    // printf("get MGWpingstr\n");
    if ( Numramchains != 0 )
    {
        if ( ram == 0 )
            ram = Ramchains[rand() % Numramchains];
        if ( ram != 0 )
            ram_set_MGWpingstr(MGWpingstr,ram,selector);
    }
}

void ram_parse_MGWstate(struct MGWstate *sp,cJSON *json,char *coinstr,char *NXTaddr)
{
    cJSON *nxtobj,*coinobj;
    if ( sp == 0 || json == 0 || coinstr == 0 || coinstr[0] == 0 )
        return;
    memset(sp,0,sizeof(*sp));
    sp->nxt64bits = calc_nxt64bits(NXTaddr);
    sp->MGWbalance = get_API_nxt64bits(cJSON_GetObjectItem(json,"balance"));
    sp->sentNXT = get_API_nxt64bits(cJSON_GetObjectItem(json,"sentNXT"));
    sp->MGWunspent = get_API_nxt64bits(cJSON_GetObjectItem(json,"unspent"));
    sp->circulation = get_API_nxt64bits(cJSON_GetObjectItem(json,"circulation"));
    sp->MGWpendingredeems = get_API_nxt64bits(cJSON_GetObjectItem(json,"pendingredeems"));
    sp->MGWpendingdeposits = get_API_nxt64bits(cJSON_GetObjectItem(json,"pendingdeposits"));
    sp->supply = get_API_nxt64bits(cJSON_GetObjectItem(json,"supply"));
    sp->orphans = get_API_nxt64bits(cJSON_GetObjectItem(json,"internal"));
    if ( (nxtobj= cJSON_GetObjectItem(json,"RTNXT")) != 0 )
    {
        sp->NXT_RTblocknum = (uint32_t)get_API_int(cJSON_GetObjectItem(nxtobj,"height"),0);
        sp->NXTblocknum = (sp->NXT_RTblocknum - (uint32_t)get_API_int(cJSON_GetObjectItem(nxtobj,"lag"),0));
        sp->NXT_ECblock = get_API_nxt64bits(cJSON_GetObjectItem(nxtobj,"ECblock"));
        sp->NXT_ECheight = (uint32_t)get_API_int(cJSON_GetObjectItem(nxtobj,"ECheight"),0);
    }
    if ( (coinobj= cJSON_GetObjectItem(json,coinstr)) != 0 )
    {
        sp->permblocks = (uint32_t)get_API_int(cJSON_GetObjectItem(coinobj,"permblocks"),0);
        sp->RTblocknum = (uint32_t)get_API_int(cJSON_GetObjectItem(coinobj,"height"),0);
        sp->blocknum = (sp->NXT_RTblocknum - (uint32_t)get_API_int(cJSON_GetObjectItem(coinobj,"lag"),0));
    }
}

void ram_update_remotesrc(struct ramchain_info *ram,struct MGWstate *sp)
{
    int32_t i,oldi = -1,oldest = -1;
    //printf("update remote %llu\n",(long long)sp->nxt64bits);
    if ( sp->nxt64bits == 0 )
        return;
    for (i=0; i<(int32_t)(sizeof(ram->remotesrcs)/sizeof(*ram->remotesrcs)); i++)
    {
        if ( ram->remotesrcs[i].nxt64bits == 0 || sp->nxt64bits == ram->remotesrcs[i].nxt64bits )
        {
            ram->remotesrcs[i] = *sp;
            //printf("set slot.%d <- permblocks.%u\n",i,sp->permblocks);
            return;
        }
        if ( oldest < 0 || (ram->remotesrcs[i].permblocks != 0 && ram->remotesrcs[i].permblocks < oldest) )
            ram->remotesrcs[i].permblocks = oldest,oldi = i;
    }
    if ( oldi >= 0 && (sp->permblocks != 0 && sp->permblocks > oldest) )
    {
        //printf("overwrite slot.%d <- permblocks.%u\n",oldi,sp->permblocks);
        ram->remotesrcs[oldi] = *sp;
    }
}

void ram_parse_MGWpingstr(struct ramchain_info *ram,char *sender,char *pingstr)
{
    void save_MGW_status(char *NXTaddr,char *jsonstr);
    char name[512],coinstr[MAX_JSON_FIELD],*jsonstr = 0;
    struct MGWstate S;
    int32_t gatewayid;
    cJSON *json,*array;
    if ( (array= cJSON_Parse(pingstr)) != 0 && is_cJSON_Array(array) != 0 )
    {
        json = cJSON_GetArrayItem(array,0);
        if ( ram == 0 )
        {
            copy_cJSON(coinstr,cJSON_GetObjectItem(json,"coin"));
            if ( coinstr[0] != 0 )
                ram = get_ramchain_info(coinstr);
        }
        if ( Debuglevel > 2 || (ram != 0 && ram->remotemode != 0) )
            printf("[%s] parse.(%s)\n",coinstr,pingstr);
        if ( ram != 0 )
        {
            cJSON_DeleteItemFromObject(json,"ipaddr");
            if ( (gatewayid= (int32_t)get_API_int(cJSON_GetObjectItem(json,"gatewayid"),-1)) >= 0 && gatewayid < ram->numgateways )
            {
                if ( strcmp(ram->special_NXTaddrs[gatewayid],sender) == 0 )
                    ram_parse_MGWstate(&ram->otherS[gatewayid],json,ram->name,sender);
                else printf("ram_parse_MGWpingstr: got wrong address.(%s) for gatewayid.%d expected.(%s)\n",sender,gatewayid,ram->special_NXTaddrs[gatewayid]);
            }
            else
            {
                //printf("call parse.(%s)\n",cJSON_Print(json));
                ram_parse_MGWstate(&S,json,ram->name,sender);
                ram_update_remotesrc(ram,&S);
            }
            jsonstr = cJSON_Print(json);
            if ( gatewayid >= 0 && gatewayid < 3 && strcmp(ram->mgwstrs[gatewayid],jsonstr) != 0 )
            {
                safecopy(ram->mgwstrs[gatewayid],jsonstr,sizeof(ram->mgwstrs[gatewayid]));
                //sprintf(name,"%s.%s",ram->name,Server_ipaddrs[gatewayid]);
                sprintf(name,"%s.%d",ram->name,gatewayid);
                //printf("name is (%s) + (%s) -> (%s)\n",ram->name,Server_ipaddrs[gatewayid],name);
                save_MGW_status(name,jsonstr);
            }
        }
        if ( jsonstr != 0 )
            free(jsonstr);
        free_json(array);
    }
    //printf("parsed\n");
}

int32_t MGWstatecmp(struct MGWstate *spA,struct MGWstate *spB)
{
    return(memcmp((void *)((long)spA + sizeof(spA->gatewayid)),(void *)((long)spB + sizeof(spB->gatewayid)),sizeof(*spA) - sizeof(spA->gatewayid)));
}

double _enough_confirms(double redeemed,double estNXT,int32_t numconfs,int32_t minconfirms)
{
    double metric;
    if ( numconfs < minconfirms )
        return(0);
    metric = log(estNXT + sqrt(redeemed));
    return(metric - 1.);
}

int32_t ram_MGW_ready(struct ramchain_info *ram,uint32_t blocknum,uint32_t NXTheight,uint64_t nxt64bits,uint64_t amount)
{
    int32_t retval = 0;
    if ( ram->S.gatewayid >= 0 && ram->S.gatewayid < 3 && strcmp(ram->srvNXTADDR,ram->special_NXTaddrs[ram->S.gatewayid]) != 0 )
    {
        if ( Debuglevel > 2 )
            printf("mismatched gatewayid.%d\n",ram->S.gatewayid);
        return(0);
    }
    if ( ram->S.gatewayid < 0 || (nxt64bits != 0 && (nxt64bits % NUM_GATEWAYS) != ram->S.gatewayid) || ram->S.MGWbalance < 0 )
        return(0);
    else if ( blocknum != 0 && ram->S.NXT_is_realtime != 0 && (blocknum + ram->depositconfirms) <= ram->S.RTblocknum && ram->S.enable_deposits != 0 )
        retval = 1;
    else if ( NXTheight != 0 && ram->S.is_realtime != 0 && ram->S.enable_withdraws != 0 && _enough_confirms(0.,amount * ram->NXTconvrate,ram->S.NXT_RTblocknum - NXTheight,ram->withdrawconfirms) > 0. )
        retval = 1;
    if ( retval != 0 )
    {
        if ( MGWstatecmp(&ram->otherS[0],&ram->otherS[1]) != 0 || MGWstatecmp(&ram->otherS[0],&ram->otherS[2]) != 0 )
        {
            printf("MGWstatecmp failure %d, %d\n",MGWstatecmp(&ram->otherS[0],&ram->otherS[1]),MGWstatecmp(&ram->otherS[0],&ram->otherS[2]));
            //return(0);
        }
    }
    return(retval);
}

int32_t ram_mark_depositcomplete(struct ramchain_info *ram,struct NXT_assettxid *tp,uint32_t blocknum)
{ // NXT
    struct ramchain_hashptr *addrptr,*txptr;
    struct rampayload *addrpayload,*txpayload;
    int32_t numtxpayloads;
    if ( tp->cointxid != 0 )
    {
        if ( (txpayload= ram_payloads(&txptr,&numtxpayloads,ram,tp->cointxid,'t')) != 0 && tp->coinv < numtxpayloads )
        {
            txpayload = &txpayload[tp->coinv];
            if ( (addrpayload= ram_getpayloadi(&addrptr,ram,'a',txpayload->otherind,txpayload->extra)) != 0 )
            {
                if ( txptr->rawind == addrpayload->otherind && txpayload->value == addrpayload->value )
                {
                    if ( addrpayload->pendingdeposit != 0 )
                    {
                        printf("deposit complete %s.%s/v%d %.8f -> NXT.%llu txid.%llu\n",ram->name,tp->cointxid,tp->coinv,dstr(tp->U.assetoshis),(long long)tp->receiverbits,(long long)tp->redeemtxid);
                        addrpayload->pendingdeposit = 0;
                        _complete_assettxid(tp);
                    }
                    else
                    {
                        if ( tp->completed == 0 )
                            printf("deposit NOT PENDING? complete %s.%s/v%d %.8f -> NXT.%llu txid.%llu\n",ram->name,tp->cointxid,tp->coinv,dstr(tp->U.assetoshis),(long long)tp->receiverbits,(long long)tp->redeemtxid);
                    }
                    return(1);
                } else printf("ram_mark_depositcomplete: mismatched rawind or value (%u vs %d) (%.8f vs %.8f)\n",txptr->rawind,addrpayload->otherind,dstr(txpayload->value),dstr(addrpayload->value));
            } else printf("ram_mark_depositcomplete: couldnt find addrpayload for %s vout.%d\n",tp->cointxid,tp->coinv);
        } else printf("ram_mark_depositcomplete: couldnt find (%s) txpayload.%p or tp->coinv.%d >= %d numtxpayloads blocknum.%d\n",tp->cointxid,txpayload,tp->coinv,numtxpayloads,blocknum);
    } else printf("ram_mark_depositcomplete: unexpected null cointxid\n");
    return(0);
}

void ram_addunspent(struct ramchain_info *ram,char *coinaddr,struct rampayload *txpayload,struct ramchain_hashptr *addrptr,struct rampayload *addrpayload,uint32_t addr_rawind,uint32_t ind)
{ // bitcoin
    // ram_addunspent() is called from the rawtx_update that creates the txpayloads, each coinaddr gets a list of addrpayloads initialized
    // it needs to update the corresponding txpayload so exact state can be quickly calculated
    //struct NXT_assettxid *tp;
    int32_t len;
    struct multisig_addr *msig;
    txpayload->B = addrpayload->B;
    txpayload->value = addrpayload->value;
    //printf("txpayload.%p addrpayload.%p set addr_rawind.%d -> %p txid_rawind.%u\n",txpayload,addrpayload,addr_rawind,txpayload,addrpayload->otherind);
    txpayload->otherind = addr_rawind;  // allows for direct lookup of coin addr payload vector
    txpayload->extra = ind;             // allows for direct lookup of the payload within the vector
    if ( addrpayload->value != 0 )
    {
        //printf("UNSPENT %.8f\n",dstr(addrpayload->value));
        ram->S.totaloutputs += addrpayload->value;
        ram->S.numoutputs++;
        addrptr->unspent += addrpayload->value;
        addrptr->numunspent++;
        if ( addrptr->multisig != 0 && (msig= find_msigaddr(&len,ram->name,0,coinaddr)) != 0 && _in_specialNXTaddrs(ram->special_NXTaddrs,ram->numspecials,msig->NXTaddr) == 0 )
        {
            if ( addrpayload->B.isinternal == 0 ) // all non-internal unspents could be MGW deposit or withdraw
            {
                {
                    char txidstr[256];
                    ram_txid(txidstr,ram,addrpayload->otherind);
                    printf("ram_addunspent.%s: pending deposit %s %.8f -> %s rawind %d.i%d for NXT.%s\n",txidstr,ram->name,dstr(addrpayload->value),coinaddr,addr_rawind,ind,msig->NXTaddr);
                }
                addrpayload->pendingdeposit = _valid_txamount(ram,addrpayload->value,coinaddr);
            } else if ( 0 && addrptr->multisig != 0 )
                printf("find_msigaddr: couldnt find.(%s)\n",coinaddr);
            /*else if ( (tp= _is_pending_withdraw(ram,coinaddr)) != 0 )
             {
             if ( tp->completed == 0 && _is_pending_withdrawamount(ram,tp,addrpayload->value) != 0 ) // one to many problem
             {
             printf("ram_addunspent: pending withdraw %s %.8f -> %s completed for NXT.%llu\n",ram->name,dstr(tp->U.assetoshis),coinaddr,(long long)tp->senderbits);
             tp->completed = 1;
             }
             }*/
        }
    }
}

int32_t _ram_update_redeembits(struct ramchain_info *ram,uint64_t redeembits,uint64_t AMtxidbits,char *cointxid,struct address_entry *bp)
{
    struct NXT_asset *ap = ram->ap;
    struct NXT_assettxid *tp;
    int32_t createdflag,num = 0;
    char txid[64];
    if ( ram == 0 )
        return(0);
    expand_nxt64bits(txid,redeembits);
    tp = find_NXT_assettxid(&createdflag,ap,txid);
    tp->assetbits = ap->assetbits;
    tp->redeemtxid = redeembits;
    if ( tp->redeemtxid == redeembits )
    {
        if ( AMtxidbits != 0 )
            tp->AMtxidbits = AMtxidbits;
        _complete_assettxid(tp);
        if ( bp != 0 && bp->blocknum != 0 )
        {
            tp->coinblocknum = bp->blocknum;
            tp->cointxind = bp->txind;
            tp->coinv = bp->v;
        }
        if ( cointxid != 0 )
        {
            if ( tp->cointxid != 0 )
            {
                if ( strcmp(tp->cointxid,cointxid) != 0 )
                {
                    printf("_ram_update_redeembits: unexpected cointxid.(%s) already there for redeem.%llu (%s)\n",tp->cointxid,(long long)redeembits,cointxid);
                    free(tp->cointxid);
                    tp->cointxid = clonestr(cointxid);
                }
            }
            else tp->cointxid = clonestr(cointxid);
        }
        num++;
    }
    if ( AMtxidbits == 0 && num == 0 )
        printf("_ram_update_redeembits: unexpected no pending redeems when AMtxidbits.0\n");
    return(num);
}

int32_t ram_update_redeembits(struct ramchain_info *ram,cJSON *argjson,uint64_t AMtxidbits)
{
    cJSON *array;
    uint64_t redeembits;
    int32_t i,n,num = 0;
    struct address_entry B;
    char coinstr[MAX_JSON_FIELD],redeemtxid[MAX_JSON_FIELD],cointxid[MAX_JSON_FIELD];
    if ( extract_cJSON_str(coinstr,sizeof(coinstr),argjson,"coin") <= 0 )
        return(0);
    if ( ram != 0 && strcmp(ram->name,coinstr) != 0 )
        return(0);
    extract_cJSON_str(cointxid,sizeof(cointxid),argjson,"cointxid");
    memset(&B,0,sizeof(B));
    B.blocknum = (uint32_t)get_cJSON_int(argjson,"coinblocknum");
    B.txind = (uint32_t)get_cJSON_int(argjson,"cointxind");
    B.v = (uint32_t)get_cJSON_int(argjson,"coinv");
    if ( B.blocknum != 0 )
        printf("(%d %d %d) -> %s\n",B.blocknum,B.txind,B.v,cointxid);
    array = cJSON_GetObjectItem(argjson,"redeems");
    if ( array != 0 && is_cJSON_Array(array) != 0 )
    {
        n = cJSON_GetArraySize(array);
        for (i=0; i<n; i++)
        {
            copy_cJSON(redeemtxid,cJSON_GetArrayItem(array,i));
            redeembits = calc_nxt64bits(redeemtxid);
            if ( redeemtxid[0] != 0 )
                num += _ram_update_redeembits(ram,redeembits,AMtxidbits,cointxid,&B);
        }
    }
    else
    {
        if ( extract_cJSON_str(redeemtxid,sizeof(redeemtxid),argjson,"redeemtxid") > 0 )
        {
            printf("no redeems: (%s)\n",cJSON_Print(argjson));
            num += _ram_update_redeembits(ram,calc_nxt64bits(redeemtxid),AMtxidbits,cointxid,&B);
        }
    }
    if ( 0 && num == 0 )
        printf("unexpected num.0 for ram_update_redeembits.(%s)\n",cJSON_Print(argjson));
    return(num);
}

void ram_setdispstr(char *buf,struct ramchain_info *ram,double startmilli)
{
    double estimatedV,estimatedB,estsizeV,estsizeB;
    estimatedV = estimate_completion(startmilli,ram->Vblocks.processed,(int32_t)ram->S.RTblocknum-ram->Vblocks.blocknum)/60000;
    estimatedB = estimate_completion(startmilli,ram->Bblocks.processed,(int32_t)ram->S.RTblocknum-ram->Bblocks.blocknum)/60000;
    estsizeV = (ram->Vblocks.sum / (1 + ram->Vblocks.count)) * ram->S.RTblocknum;
    estsizeB = (ram->Bblocks.sum / (1 + ram->Bblocks.count)) * ram->S.RTblocknum;
    sprintf(buf,"%-5s: RT.%d nonz.%d V.%d B.%d B64.%d B4096.%d | %s %s R%.2f | minutes: V%.1f B%.1f | outputs.%llu %.8f spends.%llu %.8f -> balance: %llu %.8f ave %.8f",ram->name,ram->S.RTblocknum,ram->nonzblocks,ram->Vblocks.blocknum,ram->Bblocks.blocknum,ram->blocks64.blocknum,ram->blocks4096.blocknum,_mbstr(estsizeV),_mbstr2(estsizeB),estsizeV/(estsizeB+1),estimatedV,estimatedB,(long long)ram->S.numoutputs,dstr(ram->S.totaloutputs),(long long)ram->S.numspends,dstr(ram->S.totalspends),(long long)(ram->S.numoutputs - ram->S.numspends),dstr(ram->S.totaloutputs - ram->S.totalspends),dstr(ram->S.totaloutputs - ram->S.totalspends)/(ram->S.numoutputs - ram->S.numspends));
}

void ram_disp_status(struct ramchain_info *ram)
{
    char buf[1024];
    int32_t i,n = 0;
    for (i=0; i<ram->S.RTblocknum; i++)
    {
        if ( ram->blocks.hps[i] != 0 )
            n++;
    }
    ram->nonzblocks = n;
    ram_setdispstr(buf,ram,ram->startmilli);
    fprintf(stderr,"%s\n",buf);
}

#endif
#endif
