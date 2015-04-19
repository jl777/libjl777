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
#include "system777.c"
#include "NXT777.c"
#include "bits777.c"
#include "search.c"
#include "storage.c"
#include "ramchain.c"

#define GET_COINDEPOSIT_ADDRESS 'g'
#define BIND_DEPOSIT_ADDRESS 'b'
#define DEPOSIT_CONFIRMED 'd'
#define MONEY_SENT 'm'
#define NXT_ASSETLIST_INCR 100
extern char NXT_ASSETIDSTR[];

#endif
#else
#ifndef crypto777_mgwstate_c
#define crypto777_mgwstate_c

#ifndef crypto777_mgwstate_h
#define DEFINES_ONLY
#include __BASE_FILE__
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

void _complete_assettxid(struct ramchain_info *ram,struct NXT_assettxid *tp)
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
    extern int32_t NORAMCHAINS;
    void save_MGW_status(char *NXTaddr,char *jsonstr);
    char name[512],coinstr[MAX_JSON_FIELD],*jsonstr = 0;
    struct MGWstate S;
    int32_t gatewayid;
    cJSON *json,*array;
    if ( Finished_init == 0 )
        return;
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
        } else if ( Debuglevel > 1 && NORAMCHAINS == 0 && coinstr[0] != 0 ) printf("dont have ramchain_info for (%s) (%s)\n",coinstr,pingstr);
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
                        _complete_assettxid(ram,tp);
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
        if ( addrptr->multisig != 0 && (msig= find_msigaddr(coinaddr)) != 0 && _in_specialNXTaddrs(ram->special_NXTaddrs,ram->numspecials,msig->NXTaddr) == 0 )
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

struct NXT_assettxid *find_NXT_assettxid(int32_t *createdflagp,struct NXT_asset *ap,char *txid)
{
    int32_t createdflag;
    struct NXT_assettxid *tp;
    if ( createdflagp == 0 )
        createdflagp = &createdflag;
    tp = MTadd_hashtable(createdflagp,&NXT_assettxids,txid);
    if ( *createdflagp != 0 )
    {
        //tp->assetbits = ap->assetbits;
        // tp->redeemtxid = calc_nxt64bits(txid);
        // tp->timestamp = timestamp;
        //printf("%d) %s txid.%s\n",ap->num,ap->name,txid);
        if ( ap != 0 )
        {
            if ( ap->num >= ap->max )
            {
                ap->max = ap->num + NXT_ASSETLIST_INCR;
                ap->txids = realloc(ap->txids,sizeof(*ap->txids) * ap->max);
            }
            ap->txids[ap->num++] = tp;
        }
    }
    return(tp);
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
    if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone > 1 )
        printf("_ram_update_redeembits.apnum.%d set AMtxidbits.%llu -> %s redeem (%llu) cointxid.%p tp.%p\n",ap->num,(long long)AMtxidbits,ram->name,(long long)redeembits,cointxid,tp);
    if ( tp->redeemtxid == redeembits )
    {
        if ( AMtxidbits != 0 )
            tp->AMtxidbits = AMtxidbits;
        _complete_assettxid(ram,tp);
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

cJSON *_process_MGW_message(struct ramchain_info *ram,uint32_t height,int32_t funcid,cJSON *argjson,uint64_t itemid,uint64_t units,char *sender,char *receiver,char *txid)
{
    struct multisig_addr *decode_msigjson(char *NXTaddr,cJSON *obj,char *sender);
    void update_coinacct_addresses(uint64_t nxt64bits,cJSON *json,char *txid);
    uint64_t nxt64bits = calc_nxt64bits(sender);
    struct multisig_addr *msig;
    // we can ignore height as the sender is validated or it is non-money get_coindeposit_address request
    if ( calc_nxt64bits(receiver) != ram->MGWbits && calc_nxt64bits(sender) != ram->MGWbits )
        return(0);
    if ( argjson != 0 )
    {
        //if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
        //    fprintf(stderr,"func.(%c) %s -> %s txid.(%s) JSON.(%s)\n",ap->funcid,sender,receiver,txid,ap->U.jsonstr);
        switch ( funcid )
        {
            case GET_COINDEPOSIT_ADDRESS:
                // start address gen
                //fprintf(stderr,"GENADDRESS: func.(%c) %s -> %s txid.(%s) JSON.(%s)\n",ap->funcid,sender,receiver,txid,ap->U.jsonstr);
                //fprintf(stderr,"g");
                update_coinacct_addresses(nxt64bits,argjson,txid);
                break;
            case BIND_DEPOSIT_ADDRESS:
                //fprintf(stderr,"b");
                if ( _in_specialNXTaddrs(ram->special_NXTaddrs,ram->numspecials,sender) != 0 && (msig= decode_msigjson(0,argjson,sender)) != 0 ) // strcmp(sender,receiver) == 0) &&
                {
                    if ( strcmp(msig->coinstr,ram->name) == 0 && find_msigaddr(msig->multisigaddr) == 0 )
                    {
                        //if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone > 1 )
                        //    fprintf(stderr,"BINDFUNC: %s func.(%c) %s -> %s txid.(%s)\n",msig->coinstr,funcid,sender,receiver,txid);
                        if ( update_msig_info(msig,1,sender) > 0 )
                        {
                            //fprintf(stderr,"%s func.(%c) %s -> %s txid.(%s)\n",msig->coinstr,funcid,sender,receiver,txid);
                        }
                    }
                    free(msig);
                } //else printf("WARNING: sender.%s == NXTaddr.%s\n",sender,NXTaddr);
                break;
            case DEPOSIT_CONFIRMED:
                // need to mark cointxid with AMtxid to prevent confirmation process generating AM each time
                /*if ( is_gateway_addr(sender) != 0 && (coinid= decode_depositconfirmed_json(argjson,txid)) >= 0 )
                 {
                 printf("deposit confirmed for coinid.%d %s\n",coinid,coinid_str(coinid));
                 }*/
                break;
            case MONEY_SENT:
                //fprintf(stderr,"m");
                //printf("MONEY_SENT.(%s)\n",cJSON_Print(argjson));
                if ( _in_specialNXTaddrs(ram->special_NXTaddrs,ram->numspecials,sender) != 0 )
                    ram_update_redeembits(ram,argjson,calc_nxt64bits(txid));
                break;
            default: printf("funcid.(%c) not handled\n",funcid);
        }
    }
    return(argjson);
}

cJSON *_parse_json_AM(struct json_AM *ap)
{
    char *jsontxt;
    if ( ap->jsonflag != 0 )
    {
        jsontxt = (ap->jsonflag == 1) ? ap->U.jsonstr : 0;//decode_json(&ap->U.jsn,ap->jsonflag);
        if ( jsontxt != 0 )
        {
            if ( jsontxt[0] == '"' && jsontxt[strlen(jsontxt)-1] == '"' )
                unstringify(jsontxt);
            return(cJSON_Parse(jsontxt));
        }
    }
    return(0);
}

void _process_AM_message(struct ramchain_info *ram,uint32_t height,struct json_AM *AM,char *sender,char *receiver,char *txid)
{
    cJSON *argjson;
    if ( AM == 0 )
        return;
    if ( (argjson= _parse_json_AM(AM)) != 0 )
    {
        if ( 0 && ((MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone > 1) )
            fprintf(stderr,"func.(%c) %s -> %s txid.(%s) JSON.(%s)\n",AM->funcid,sender,receiver,txid,AM->U.jsonstr);
        //else fprintf(stderr,"%c",AM->funcid);
        if ( AM->funcid > 0 )
            argjson = _process_MGW_message(ram,height,AM->funcid,argjson,0,0,sender,receiver,txid);
        if ( argjson != 0 )
            free_json(argjson);
    }
}

struct NXT_assettxid *_set_assettxid(struct ramchain_info *ram,uint32_t height,char *redeemtxidstr,uint64_t senderbits,uint64_t receiverbits,uint32_t timestamp,char *commentstr,uint64_t quantity)
{
    uint64_t redeemtxid;
    struct NXT_assettxid *tp;
    int32_t createdflag;
    struct NXT_asset *ap;
    cJSON *json,*cointxidobj,*obj;
    char sender[MAX_JSON_FIELD],cointxid[MAX_JSON_FIELD],coinstr[MAX_JSON_FIELD];
    if ( (ap= ram->ap) == 0 )
    {
        printf("no NXT_asset for %s\n",ram->name);
        return(0);
    }
    redeemtxid = calc_nxt64bits(redeemtxidstr);
    tp = find_NXT_assettxid(&createdflag,ap,redeemtxidstr);
    tp->assetbits = ap->assetbits;
    if ( (tp->height= height) != 0 )
        tp->numconfs = (ram->S.NXT_RTblocknum - height);
    tp->redeemtxid = redeemtxid;
    if ( timestamp > tp->timestamp )
        tp->timestamp = timestamp;
    tp->quantity = quantity;
    tp->U.assetoshis = (quantity * ap->mult);
    tp->receiverbits = receiverbits;
    tp->senderbits = senderbits;
    //printf("_set_assettxid(%s)\n",commentstr);
    if ( commentstr != 0 && (json= cJSON_Parse(commentstr)) != 0 ) //(tp->comment == 0 || strcmp(tp->comment,commentstr) != 0) &&
    {
        copy_cJSON(coinstr,cJSON_GetObjectItem(json,"coin"));
        if ( coinstr[0] == 0 )
            strcpy(coinstr,ram->name);
        //printf("%s txid.(%s) (%s)\n",ram->name,redeemtxidstr,commentstr!=0?commentstr:"NULL");
        if ( strcmp(coinstr,ram->name) == 0 )
        {
            if ( tp->comment != 0 )
                free(tp->comment);
            tp->comment = clonestr(commentstr);
            _stripwhite(tp->comment,' ');
            tp->buyNXT = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"buyNXT"),0);
            tp->coinblocknum = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"coinblocknum"),0);
            tp->cointxind = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"cointxind"),0);
            if ( (obj= cJSON_GetObjectItem(json,"coinv")) == 0 )
                obj = cJSON_GetObjectItem(json,"vout");
            tp->coinv = (uint32_t)get_API_int(obj,0);
            if ( ram->NXTfee_equiv != 0 && ram->txfee != 0 )
                tp->estNXT = (((double)ram->NXTfee_equiv / ram->txfee) * tp->U.assetoshis / SATOSHIDEN);
            if ( (cointxidobj= cJSON_GetObjectItem(json,"cointxid")) != 0 )
            {
                copy_cJSON(cointxid,cointxidobj);
                if ( cointxid[0] != 0 )
                {
                    if ( tp->cointxid != 0 && strcmp(tp->cointxid,cointxid) != 0 )
                    {
                        printf("cointxid conflict for redeemtxid.%llu: (%s) != (%s)\n",(long long)redeemtxid,tp->cointxid,cointxid);
                        free(tp->cointxid);
                    }
                    tp->cointxid = clonestr(cointxid);
                    expand_nxt64bits(sender,senderbits);
                    if ( _in_specialNXTaddrs(ram->special_NXTaddrs,ram->numspecials,sender) != 0 && tp->sentNXT != tp->buyNXT )
                    {
                        //ram->boughtNXT -= tp->sentNXT;
                        //ram->boughtNXT += tp->buyNXT;
                        tp->sentNXT = tp->buyNXT;
                    }
                }
                if ( tp->completed == 0 )
                {
                    if ( ram_mark_depositcomplete(ram,tp,tp->coinblocknum) != 0 )
                        _complete_assettxid(ram,tp);
                }
                if ( Debuglevel > 2 )
                    printf("sender.%llu receiver.%llu got.(%llu) comment.(%s) (B%d t%d) cointxidstr.(%s)/v%d buyNXT.%d completed.%d\n",(long long)senderbits,(long long)receiverbits,(long long)redeemtxid,tp->comment,tp->coinblocknum,tp->cointxind,cointxid,tp->coinv,tp->buyNXT,tp->completed);
            }
            else
            {
                if ( Debuglevel > 2 )
                    printf("%s txid.(%s) got comment.(%s) gotpossibleredeem.(%d.%d.%d) %.8f/%.8f NXTequiv %.8f -> redeemtxid.%llu\n",ap->name,redeemtxidstr,tp->comment!=0?tp->comment:"",tp->coinblocknum,tp->cointxind,tp->coinv,dstr(tp->quantity * ap->mult),dstr(tp->U.assetoshis),tp->estNXT,(long long)tp->redeemtxid);
            }
        } else printf("mismatched coin.%s vs (%s) for transfer.%llu (%s)\n",coinstr,ram->name,(long long)redeemtxid,commentstr);
        free_json(json);
    } //else printf("error with (%s) tp->comment %p\n",commentstr,tp->comment);
    return(tp);
}

void _set_NXT_sender(char *sender,cJSON *txobj)
{
    cJSON *senderobj;
    senderobj = cJSON_GetObjectItem(txobj,"sender");
    if ( senderobj == 0 )
        senderobj = cJSON_GetObjectItem(txobj,"accountId");
    else if ( senderobj == 0 )
        senderobj = cJSON_GetObjectItem(txobj,"account");
    copy_cJSON(sender,senderobj);
}

void ram_gotpayment(struct ramchain_info *ram,char *comment,cJSON *commentobj)
{
    printf("ram_gotpayment.(%s) from %s\n",comment,ram->name);
}

uint32_t _process_NXTtransaction(int32_t confirmed,struct ramchain_info *ram,cJSON *txobj)
{
    char AMstr[MAX_JSON_FIELD],sender[MAX_JSON_FIELD],receiver[MAX_JSON_FIELD],assetidstr[MAX_JSON_FIELD],txid[MAX_JSON_FIELD],comment[MAX_JSON_FIELD],*commentstr = 0;
    cJSON *attachment,*message,*assetjson,*commentobj;
    unsigned char buf[4096];
    struct NXT_AMhdr *hdr;
    struct NXT_asset *ap = ram->ap;
    struct NXT_assettxid *tp;
    uint64_t units;
    uint32_t buyNXT,height = 0;
    int32_t funcid,numconfs,timestamp=0;
    int64_t type,subtype,n,satoshis,assetoshis = 0;
    if ( txobj != 0 )
    {
        hdr = 0;
        sender[0] = receiver[0] = 0;
        if ( confirmed != 0 )
        {
            height = (uint32_t)get_cJSON_int(txobj,"height");
            if ( (numconfs= (int32_t)get_API_int(cJSON_GetObjectItem(txobj,"confirmations"),0)) == 0 )
                numconfs = (ram->S.NXT_RTblocknum - height);
        } else numconfs = 0;
        copy_cJSON(txid,cJSON_GetObjectItem(txobj,"transaction"));
        //if ( strcmp(txid,"1110183143900371107") == 0 )
        //    printf("TX.(%s) %s\n",txid,cJSON_Print(txobj));
        type = get_cJSON_int(txobj,"type");
        subtype = get_cJSON_int(txobj,"subtype");
        timestamp = (int32_t)get_cJSON_int(txobj,"blockTimestamp");
        _set_NXT_sender(sender,txobj);
        copy_cJSON(receiver,cJSON_GetObjectItem(txobj,"recipient"));
        attachment = cJSON_GetObjectItem(txobj,"attachment");
        if ( attachment != 0 )
        {
            message = cJSON_GetObjectItem(attachment,"message");
            assetjson = cJSON_GetObjectItem(attachment,"asset");
            memset(comment,0,sizeof(comment));
            if ( message != 0 && type == 1 )
            {
                copy_cJSON(AMstr,message);
                n = strlen(AMstr);
                if ( is_hexstr(AMstr) != 0 )
                {
                    if ( (n&1) != 0 || n > 2000 )
                        printf("warning: odd message len?? %ld\n",(long)n);
                    decode_hex((void *)buf,(int32_t)(n>>1),AMstr);
                    buf[(n>>1)] = 0;
                    hdr = (struct NXT_AMhdr *)buf;
                    _process_AM_message(ram,height,(void *)hdr,sender,receiver,txid);
                }
            }
            else if ( assetjson != 0 && type == 2 && subtype == 1 )
            {
                commentobj = cJSON_GetObjectItem(attachment,"comment");
                if ( commentobj == 0 )
                    commentobj = message;
                copy_cJSON(comment,commentobj);
                if ( comment[0] != 0 )
                    commentstr = clonestr(unstringify(comment));
                copy_cJSON(assetidstr,cJSON_GetObjectItem(attachment,"asset"));
                //if ( strcmp(txid,"998606823456096714") == 0 )
                //printf("Inside comment.(%s): %s cmp.%d\n",assetidstr,comment,ap->assetbits == calc_nxt64bits(assetidstr));
                if ( assetidstr[0] != 0 && ap->assetbits == calc_nxt64bits(assetidstr) )
                {
                    assetoshis = get_cJSON_int(attachment,"quantityQNT");
                    //fprintf(stderr,"a");
                    tp = _set_assettxid(ram,height,txid,calc_nxt64bits(sender),calc_nxt64bits(receiver),timestamp,commentstr,assetoshis);
                    //if ( _in_specialNXTaddrs(ram->special_NXTaddrs,ram->numspecials,sender) != 0  )
                    //    _add_pendingxfer(ram,height,1,tp->redeemtxid);
                }
            }
            else
            {
                copy_cJSON(comment,message);
                unstringify(comment);
                commentobj = comment[0] != 0 ? cJSON_Parse(comment) : 0;
                if ( type == 5 && subtype == 3 )
                {
                    copy_cJSON(assetidstr,cJSON_GetObjectItem(attachment,"currency"));
                    units = get_API_int(cJSON_GetObjectItem(attachment,"units"),0);
                    if ( commentobj != 0 )
                    {
                        funcid = (int32_t)get_API_int(cJSON_GetObjectItem(commentobj,"funcid"),-1);
                        if ( funcid >= 0 && (commentobj= _process_MGW_message(ram,height,funcid,commentobj,calc_nxt64bits(assetidstr),units,sender,receiver,txid)) != 0 )
                            free_json(commentobj);
                    }
                }
                else if ( type == 0 && subtype == 0 && commentobj != 0 )
                {
                    //if ( strcmp(txid,"998606823456096714") == 0 )
                    //    printf("Inside message: %s\n",comment);
                    if ( _in_specialNXTaddrs(ram->special_NXTaddrs,ram->numspecials,sender) != 0 )
                    {
                        buyNXT = get_API_int(cJSON_GetObjectItem(commentobj,"buyNXT"),0);
                        satoshis = get_API_nxt64bits(cJSON_GetObjectItem(txobj,"amountNQT"));
                        if ( buyNXT*SATOSHIDEN == satoshis )
                        {
                            ram->S.sentNXT += buyNXT * SATOSHIDEN;
                            printf("%s sent %d NXT, total sent %.0f\n",sender,buyNXT,dstr(ram->S.sentNXT));
                        }
                        else if ( buyNXT != 0 )
                            printf("unexpected QNT %.8f vs %d\n",dstr(satoshis),buyNXT);
                    }
                    if ( strcmp(sender,ram->srvNXTADDR) == 0 )
                        ram_gotpayment(ram,comment,commentobj);
                }
            }
        }
    }
    else printf("unexpected error iterating timestamp.(%d) txid.(%s)\n",timestamp,txid);
    //fprintf(stderr,"finish type.%d subtype.%d txid.(%s)\n",(int)type,(int)subtype,txid);
    return(height);
}

void ram_setdispstr(char *buf,struct ramchain_info *ram,double startmilli)
{
    double estimatedV,estimatedB,estsizeV,estsizeB;
    estimatedV = estimate_completion(ram->name,startmilli,ram->Vblocks.processed,(int32_t)ram->S.RTblocknum-ram->Vblocks.blocknum)/60000;
    estimatedB = estimate_completion(ram->name,startmilli,ram->Bblocks.processed,(int32_t)ram->S.RTblocknum-ram->Bblocks.blocknum)/60000;
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

uint64_t ram_verify_txstillthere(struct ramchain_info *ram,char *txidstr,struct address_entry *bp)
{
    char *retstr = 0;
    cJSON *txjson,*voutsobj;
    int32_t numvouts;
    uint64_t value = 0;
    if ( (retstr= _get_transaction(ram,txidstr)) != 0 )
    {
        if ( (txjson= cJSON_Parse(retstr)) != 0 )
        {
            if ( (voutsobj= cJSON_GetObjectItem(txjson,"vout")) != 0 && is_cJSON_Array(voutsobj) != 0 && (numvouts= cJSON_GetArraySize(voutsobj)) > 0  && bp->v < numvouts )
                value = conv_cJSON_float(cJSON_GetArrayItem(voutsobj,bp->v),"value");
            free_json(txjson);
        } else printf("update_txid_infos parse error.(%s)\n",retstr);
        free(retstr);
    } else printf("error getting.(%s)\n",txidstr);
    return(value);
}

uint64_t calc_addr_unspent(struct ramchain_info *ram,struct multisig_addr *msig,char *addr,struct rampayload *addrpayload)
{
    uint64_t MGWtransfer_asset(cJSON **transferjsonp,int32_t forceflag,uint64_t nxt64bits,char *depositors_pubkey,struct NXT_asset *ap,uint64_t value,char *coinaddr,char *txidstr,struct address_entry *entry,uint32_t *buyNXTp,char *srvNXTADDR,char *srvNXTACCTSECRET,int32_t deadline);
    uint64_t nxt64bits,pending = 0;
    char txidstr[4096];
    struct NXT_asset *ap = ram->ap;
    struct NXT_assettxid *tp;
    int32_t j;
    if ( ap != 0 )
    {
        ram_txid(txidstr,ram,addrpayload->otherind);
        for (j=0; j<ap->num; j++)
        {
            tp = ap->txids[j];
            if ( tp->cointxid != 0 && strcmp(tp->cointxid,txidstr) == 0 )
            {
                if ( ram_mark_depositcomplete(ram,tp,tp->coinblocknum) != 0 )
                    _complete_assettxid(ram,tp);
                break;
            }
        }
        //if ( strcmp("9908a63216f866650f81949684e93d62d543bdb06a23b6e56344e1c419a70d4f",txidstr) == 0 )
        //    printf("calc_addr_unspent.(%s) j.%d of apnum.%d valid.%d msig.%p\n",txidstr,j,ap->num,_valid_txamount(ram,addrpayload->value),msig);
        if ( (addrpayload->pendingdeposit != 0 || j == ap->num) && _valid_txamount(ram,addrpayload->value,msig->multisigaddr) > 0 && msig != 0 )
        {
            //printf("addr_unspent.(%s)\n",msig->NXTaddr);
            if ( (nxt64bits= calc_nxt64bits(msig->NXTaddr)) != 0 )
            {
                printf("deposit.(%s/%d %d,%d %s %.8f)rt%d_%d_%d_%d.g%d -> NXT.%s %d\n",txidstr,addrpayload->B.v,addrpayload->B.blocknum,addrpayload->B.txind,addr,dstr(addrpayload->value),ram->S.NXT_is_realtime,ram->S.enable_deposits,(addrpayload->B.blocknum + ram->depositconfirms) <= ram->S.RTblocknum,ram->S.MGWbalance >= 0,(int32_t)(nxt64bits % NUM_GATEWAYS),msig->NXTaddr,ram->S.NXT_is_realtime != 0 && (addrpayload->B.blocknum + ram->depositconfirms) <= ram->S.RTblocknum && ram->S.enable_deposits != 0);
                pending += addrpayload->value;
                if ( ram_MGW_ready(ram,addrpayload->B.blocknum,0,nxt64bits,0) > 0 )
                {
                    if ( ram_verify_txstillthere(ram,txidstr,&addrpayload->B) != addrpayload->value )
                    {
                        printf("ram_calc_unspent: tx gone due to a fork. (%d %d %d) txid.%s %.8f\n",addrpayload->B.blocknum,addrpayload->B.txind,addrpayload->B.v,txidstr,dstr(addrpayload->value));
                        exit(1); // seems the best thing to do
                    }
                    if ( MGWtransfer_asset(0,1,nxt64bits,msig->NXTpubkey,ram->ap,addrpayload->value,msig->multisigaddr,txidstr,&addrpayload->B,&msig->buyNXT,ram->srvNXTADDR,ram->srvNXTACCTSECRET,ram->DEPOSIT_XFER_DURATION) == addrpayload->value )
                        addrpayload->pendingdeposit = 0;
                }
            }
        }
    }
    return(pending);
}

uint64_t ram_calc_unspent(uint64_t *pendingp,int32_t *calc_numunspentp,struct ramchain_hashptr **addrptrp,struct ramchain_info *ram,char *addr,int32_t MGWflag)
{
    //char redeemScript[8192],normaladdr[8192];
    uint64_t pending,unspent = 0;
    struct multisig_addr *msig;
    int32_t i,numpayloads,n = 0;
    struct rampayload *payloads;
    if ( calc_numunspentp != 0 )
        *calc_numunspentp = 0;
    pending = 0;
    if ( (payloads= ram_addrpayloads(addrptrp,&numpayloads,ram,addr)) != 0 && numpayloads > 0 )
    {
        msig = find_msigaddr(addr);
        for (i=0; i<numpayloads; i++)
        {
            /*{
             char txidstr[512];
             ram_txid(txidstr,ram,payloads[i].otherind);
             if ( strcmp("9908a63216f866650f81949684e93d62d543bdb06a23b6e56344e1c419a70d4f",txidstr) == 0 )
             printf("txid.(%s).%d pendingdeposit.%d %.8f\n",txidstr,payloads[i].B.v,payloads[i].pendingdeposit,dstr(payloads[i].value));
             }*/
            if ( payloads[i].B.spent == 0 )
            {
                unspent += payloads[i].value, n++;
                if ( MGWflag != 0 && payloads[i].pendingsend == 0 )
                {
                    if ( strcmp(addr,"bYjsXxENs4u7raX3EPyrgqPy46MUrq4k8h") != 0 && strcmp(addr,"bKzDfRnGGTDwqNZWGCXa42DRdumAGskqo4") != 0 ) // addresses made with different third dev MGW server for BTCD only
                        ram_update_MGWunspents(ram,addr,payloads[i].B.v,payloads[i].otherind,payloads[i].extra,payloads[i].value);
                }
            }
            if ( payloads[i].pendingdeposit != 0 )
                pending += calc_addr_unspent(ram,msig,addr,&payloads[i]);
        }
    }
    if ( calc_numunspentp != 0 )
        *calc_numunspentp = n;
    if ( pendingp != 0 )
        (*pendingp) += pending;
    return(unspent);
}

uint64_t ram_calc_MGWunspent(uint64_t *pendingp,struct ramchain_info *ram)
{
    struct multisig_addr **msigs;
    int32_t i,n,m;
    uint64_t pending,smallest,val,unspent = 0;
    pending = 0;
    ram->MGWnumunspents = 0;
    if ( ram->MGWunspents != 0 )
        memset(ram->MGWunspents,0,sizeof(*ram->MGWunspents) * ram->MGWmaxunspents);
    if ( (msigs= (struct multisig_addr **)copy_all_DBentries(&n)) != 0 )
    {
        ram->nummsigs = n;
        ram->MGWsmallest[0] = ram->MGWsmallestB[0] = 0;
        for (smallest=i=m=0; i<n; i++)
        {
            if ( (val= ram_calc_unspent(&pending,0,0,ram,msigs[i]->multisigaddr,1)) != 0 )
            {
                unspent += val;
                if ( smallest == 0 || val < smallest )
                {
                    smallest = val;
                    strcpy(ram->MGWsmallestB,ram->MGWsmallest);
                    strcpy(ram->MGWsmallest,msigs[i]->multisigaddr);
                }
                else if ( ram->MGWsmallestB[0] == 0 && strcmp(ram->MGWsmallest,msigs[i]->multisigaddr) != 0 )
                    strcpy(ram->MGWsmallestB,msigs[i]->multisigaddr);
            }
            free(msigs[i]);
        }
        free(msigs);
        if ( Debuglevel > 2 )
            printf("MGWnumunspents.%d smallest (%s %.8f)\n",ram->MGWnumunspents,ram->MGWsmallest,dstr(smallest));
    }
    *pendingp = pending;
    return(unspent);
}

#endif
#endif
