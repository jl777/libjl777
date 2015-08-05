//
//  MGW v7
//  SuperNET
//
//  by jl777 on 12/29/14.
//  MIT license


#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <ctype.h>
#include <fcntl.h>

#define DEFINES_ONLY
#include "cJSON.h"
#include "system777.c"
#include "utils777.c"
#include "NXT777.c"
#include "files777.c"
#include "db777.c"
//#include "mgwNXT.c"

#define DEPOSIT_XFER_DURATION 5
#define MIN_DEPOSIT_FACTOR 5

struct MGWstate
{
    char name[64];
    uint64_t nxt64bits;
    int64_t MGWbalance,supply;
    uint64_t totalspends,numspends,totaloutputs,numoutputs;
    uint64_t boughtNXT,circulation,sentNXT,MGWpendingredeems,orphans,MGWunspent,MGWpendingdeposits,NXT_ECblock;
    int32_t gatewayid;
    uint32_t blocknum,RTblocknum,NXT_RTblocknum,NXTblocknum,is_realtime,NXT_is_realtime,enable_deposits,enable_withdraws,NXT_ECheight,permblocks;
};

#define NUM_GATEWAYS 3
struct mgw777
{
    uint64_t MGWbits,NXTfee_equiv,txfee,*limboarray; char *coinstr,*serverport,*userpass,*marker,*marker2;
    int32_t numgateways,nummsigs,depositconfirms,withdrawconfirms,remotemode,numpendingsends,min_NXTconfirms,numspecials;
    uint32_t firsttime,NXTtimestamp;
    double NXTconvrate;
    struct MGWstate S,otherS[16],remotesrcs[16];
    struct NXT_asset *ap;
    char multisigchar,*name,*srvNXTADDR,**special_NXTaddrs,*MGWredemption,*backups,MGWsmallest[256],MGWsmallestB[256],MGWpingstr[1024],mgwstrs[3][8192];
};
#undef DEFINES_ONLY

int32_t _in_specialNXTaddrs(struct mgw777 *mgw,char *NXTaddr)
{
    // char **specialNXTaddrs,int32_t n,
    int32_t i;
    for (i=0; i<mgw->numspecials; i++)
        if ( strcmp(mgw->special_NXTaddrs[i],NXTaddr) == 0 )
            return(1);
    return(0);
}

int32_t _valid_txamount(struct mgw777 *mgw,uint64_t value,char *coinaddr)
{
    if ( value >= MIN_DEPOSIT_FACTOR * (mgw->txfee + mgw->NXTfee_equiv) )
    {
        if ( coinaddr == 0 || (strcmp(coinaddr,mgw->marker) != 0 && strcmp(coinaddr,mgw->marker2) != 0) )
            return(1);
    }
    return(0);
}

int32_t _is_limbo_redeem(struct mgw777 *mgw,uint64_t redeemtxidbits)
{
    int32_t j;
    if ( mgw->limboarray != 0 )
    {
        for (j=0; mgw->limboarray[j]!=0; j++)
            if ( redeemtxidbits == mgw->limboarray[j] )
            {
                printf("redeemtxid.%llu in slot.%d\n",(long long)redeemtxidbits,j);
                return(1);
            }
    } else printf("no limboarray for %s?\n",mgw->coinstr);
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

struct MGWstate *ram_select_MGWstate(struct mgw777 *mgw,int32_t selector)
{
    struct MGWstate *sp = 0;
    if ( mgw != 0 )
    {
        if ( selector < 0 )
            sp = &mgw->S;
        else if ( selector < mgw->numgateways )
            sp = &mgw->otherS[selector];
    }
    return(sp);
}

void ram_set_MGWpingstr(char *pingstr,struct mgw777 *mgw,int32_t selector)
{
    struct MGWstate *sp = ram_select_MGWstate(mgw,selector);
    if ( mgw != 0 && mgw->S.gatewayid >= 0 )
    {
        mgw->S.supply = (mgw->S.totaloutputs - mgw->S.totalspends);
        mgw->otherS[mgw->S.gatewayid] = mgw->S;
    }
    if ( sp != 0 )
    {
        sprintf(pingstr,"\"coin\":\"%s\",\"gatewayid\":\"%d\",\"balance\":\"%llu\",\"sentNXT\":\"%llu\",\"unspent\":\"%llu\",\"supply\":\"%llu\",\"circulation\":\"%llu\",\"pendingredeems\":\"%llu\",\"pendingdeposits\":\"%llu\",\"internal\":\"%llu\",\"RTNXT\":{\"height\":\"%d\",\"lag\":\"%d\",\"ECblock\":\"%llu\",\"ECheight\":\"%u\"},\"%s\":{\"permblocks\":\"%d\",\"height\":\"%d\",\"lag\":\"%d\"},",mgw->coinstr,sp->gatewayid,(long long)(sp->MGWbalance),(long long)(sp->sentNXT),(long long)(sp->MGWunspent),(long long)(sp->supply),(long long)(sp->circulation),(long long)(sp->MGWpendingredeems),(long long)(sp->MGWpendingdeposits),(long long)(sp->orphans),sp->NXT_RTblocknum,sp->NXT_RTblocknum-sp->NXTblocknum,(long long)sp->NXT_ECblock,sp->NXT_ECheight,sp->name,sp->permblocks,sp->RTblocknum,sp->RTblocknum - sp->blocknum);
    }
}

void ram_set_MGWdispbuf(char *dispbuf,struct mgw777 *mgw,int32_t selector)
{
    struct MGWstate *sp = ram_select_MGWstate(mgw,selector);
    if ( sp != 0 )
    {
        sprintf(dispbuf,"[%.8f %s - %.0f NXT rate %.2f] msigs.%d unspent %.8f circ %.8f/%.8f pend.(W%.8f D%.8f) NXT.%d %s.%d\n",dstr(sp->MGWbalance),mgw->coinstr,dstr(sp->sentNXT),sp->MGWbalance<=0?0:dstr(sp->sentNXT)/dstr(sp->MGWbalance),mgw->nummsigs,dstr(sp->MGWunspent),dstr(sp->circulation),dstr(sp->supply),dstr(sp->MGWpendingredeems),dstr(sp->MGWpendingdeposits),sp->NXT_RTblocknum,mgw->coinstr,sp->RTblocknum);
    }
}

void ram_get_MGWpingstr(struct mgw777 *mgw,char *MGWpingstr,int32_t selector)
{
    MGWpingstr[0] = 0;
    printf("get MGWpingstr\n"); getchar();
    /*if ( Numramchains != 0 )
    {
        if ( ram == 0 )
            ram = Ramchains[rand() % Numramchains];
        if ( ram != 0 )
            ram_set_MGWpingstr(MGWpingstr,ram,selector);
    }*/
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

void ram_update_remotesrc(struct mgw777 *mgw,struct MGWstate *sp)
{
    int32_t i,oldi = -1,oldest = -1;
    //printf("update remote %llu\n",(long long)sp->nxt64bits);
    if ( sp->nxt64bits == 0 )
        return;
    for (i=0; i<(int32_t)(sizeof(mgw->remotesrcs)/sizeof(*mgw->remotesrcs)); i++)
    {
        if ( mgw->remotesrcs[i].nxt64bits == 0 || sp->nxt64bits == mgw->remotesrcs[i].nxt64bits )
        {
            mgw->remotesrcs[i] = *sp;
            //printf("set slot.%d <- permblocks.%u\n",i,sp->permblocks);
            return;
        }
        if ( oldest < 0 || (mgw->remotesrcs[i].permblocks != 0 && mgw->remotesrcs[i].permblocks < oldest) )
            mgw->remotesrcs[i].permblocks = oldest,oldi = i;
    }
    if ( oldi >= 0 && (sp->permblocks != 0 && sp->permblocks > oldest) )
    {
        //printf("overwrite slot.%d <- permblocks.%u\n",oldi,sp->permblocks);
        mgw->remotesrcs[oldi] = *sp;
    }
}

void ram_parse_MGWpingstr(struct mgw777 *mgw,char *sender,char *pingstr)
{
    void save_MGW_status(char *NXTaddr,char *jsonstr);
    char name[512],coinstr[MAX_JSON_FIELD],*jsonstr = 0;
    struct MGWstate S;
    int32_t gatewayid;
    cJSON *json,*array;
    if ( (array= cJSON_Parse(pingstr)) != 0 && is_cJSON_Array(array) != 0 )
    {
        json = cJSON_GetArrayItem(array,0);
        if ( mgw == 0 )
        {
            copy_cJSON(coinstr,cJSON_GetObjectItem(json,"coin"));
            if ( coinstr[0] != 0 )
                mgw = get_MGW_info(coinstr);
        }
        if ( Debuglevel > 2 || (mgw != 0 && mgw->remotemode != 0) )
            printf("[%s] parse.(%s)\n",coinstr,pingstr);
        if ( mgw != 0 )
        {
            cJSON_DeleteItemFromObject(json,"ipaddr");
            if ( (gatewayid= (int32_t)get_API_int(cJSON_GetObjectItem(json,"gatewayid"),-1)) >= 0 && gatewayid < mgw->numgateways )
            {
                if ( strcmp(mgw->special_NXTaddrs[gatewayid],sender) == 0 )
                    ram_parse_MGWstate(&mgw->otherS[gatewayid],json,mgw->coinstr,sender);
                else printf("ram_parse_MGWpingstr: got wrong address.(%s) for gatewayid.%d expected.(%s)\n",sender,gatewayid,mgw->special_NXTaddrs[gatewayid]);
            }
            else
            {
                //printf("call parse.(%s)\n",cJSON_Print(json));
                ram_parse_MGWstate(&S,json,mgw->coinstr,sender);
                ram_update_remotesrc(mgw,&S);
            }
            jsonstr = cJSON_Print(json);
            if ( gatewayid >= 0 && gatewayid < 3 && strcmp(mgw->mgwstrs[gatewayid],jsonstr) != 0 )
            {
                safecopy(mgw->mgwstrs[gatewayid],jsonstr,sizeof(mgw->mgwstrs[gatewayid]));
                //sprintf(name,"%s.%s",mgw->coinstr,Server_ipaddrs[gatewayid]);
                sprintf(name,"%s.%d",mgw->coinstr,gatewayid);
                //printf("name is (%s) + (%s) -> (%s)\n",mgw->coinstr,Server_ipaddrs[gatewayid],name);
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

int32_t ram_MGW_ready(struct mgw777 *mgw,uint32_t blocknum,uint32_t NXTheight,uint64_t nxt64bits,uint64_t amount)
{
    int32_t retval = 0;
    if ( mgw->S.gatewayid >= 0 && mgw->S.gatewayid < 3 && strcmp(mgw->srvNXTADDR,mgw->special_NXTaddrs[mgw->S.gatewayid]) != 0 )
    {
        if ( Debuglevel > 2 )
            printf("mismatched gatewayid.%d\n",mgw->S.gatewayid);
        return(0);
    }
    if ( mgw->S.gatewayid < 0 || (nxt64bits != 0 && (nxt64bits % NUM_GATEWAYS) != mgw->S.gatewayid) || mgw->S.MGWbalance < 0 )
        return(0);
    else if ( blocknum != 0 && mgw->S.NXT_is_realtime != 0 && (blocknum + mgw->depositconfirms) <= mgw->S.RTblocknum && mgw->S.enable_deposits != 0 )
        retval = 1;
    else if ( NXTheight != 0 && mgw->S.is_realtime != 0 && mgw->S.enable_withdraws != 0 && _enough_confirms(0.,amount * mgw->NXTconvrate,mgw->S.NXT_RTblocknum - NXTheight,mgw->withdrawconfirms) > 0. )
        retval = 1;
    if ( retval != 0 )
    {
        if ( MGWstatecmp(&mgw->otherS[0],&mgw->otherS[1]) != 0 || MGWstatecmp(&mgw->otherS[0],&mgw->otherS[2]) != 0 )
        {
            printf("MGWstatecmp failure %d, %d\n",MGWstatecmp(&mgw->otherS[0],&mgw->otherS[1]),MGWstatecmp(&mgw->otherS[0],&mgw->otherS[2]));
            //return(0);
        }
    }
    return(retval);
}

/*
int32_t ram_mark_depositcomplete(struct mgw777 *mgw,struct NXT_assettxid *tp,uint32_t blocknum)
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
                        printf("deposit complete %s.%s/v%d %.8f -> NXT.%llu txid.%llu\n",mgw->coinstr,tp->cointxid,tp->coinv,dstr(tp->U.assetoshis),(long long)tp->receiverbits,(long long)tp->redeemtxid);
                        addrpayload->pendingdeposit = 0;
                        _complete_assettxid(tp);
                    }
                    else
                    {
                        if ( tp->completed == 0 )
                            printf("deposit NOT PENDING? complete %s.%s/v%d %.8f -> NXT.%llu txid.%llu\n",mgw->coinstr,tp->cointxid,tp->coinv,dstr(tp->U.assetoshis),(long long)tp->receiverbits,(long long)tp->redeemtxid);
                    }
                    return(1);
                } else printf("ram_mark_depositcomplete: mismatched rawind or value (%u vs %d) (%.8f vs %.8f)\n",txptr->rawind,addrpayload->otherind,dstr(txpayload->value),dstr(addrpayload->value));
            } else printf("ram_mark_depositcomplete: couldnt find addrpayload for %s vout.%d\n",tp->cointxid,tp->coinv);
        } else printf("ram_mark_depositcomplete: couldnt find (%s) txpayload.%p or tp->coinv.%d >= %d numtxpayloads blocknum.%d\n",tp->cointxid,txpayload,tp->coinv,numtxpayloads,blocknum);
    } else printf("ram_mark_depositcomplete: unexpected null cointxid\n");
    return(0);
}

void ram_addunspent(struct mgw777 *mgw,char *coinaddr,struct rampayload *txpayload,struct ramchain_hashptr *addrptr,struct rampayload *addrpayload,uint32_t addr_rawind,uint32_t ind)
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
        mgw->S.totaloutputs += addrpayload->value;
        mgw->S.numoutputs++;
        addrptr->unspent += addrpayload->value;
        addrptr->numunspent++;
        if ( addrptr->multisig != 0 && (msig= find_msigaddr(&len,mgw->coinstr,0,coinaddr)) != 0 && _in_specialNXTaddrs(msig->NXTaddr) == 0 )
        {
            if ( addrpayload->B.isinternal == 0 ) // all non-internal unspents could be MGW deposit or withdraw
            {
                {
                    char txidstr[256];
                    ram_txid(txidstr,ram,addrpayload->otherind);
                    printf("ram_addunspent.%s: pending deposit %s %.8f -> %s rawind %d.i%d for NXT.%s\n",txidstr,mgw->coinstr,dstr(addrpayload->value),coinaddr,addr_rawind,ind,msig->NXTaddr);
                }
                addrpayload->pendingdeposit = _valid_txamount(ram,addrpayload->value,coinaddr);
            } else if ( 0 && addrptr->multisig != 0 )
                printf("find_msigaddr: couldnt find.(%s)\n",coinaddr);
            //else if ( (tp= _is_pending_withdraw(ram,coinaddr)) != 0 )
            // {
            // if ( tp->completed == 0 && _is_pending_withdrawamount(ram,tp,addrpayload->value) != 0 ) // one to many problem
            // {
            // printf("ram_addunspent: pending withdraw %s %.8f -> %s completed for NXT.%llu\n",mgw->coinstr,dstr(tp->U.assetoshis),coinaddr,(long long)tp->senderbits);
            // tp->completed = 1;
            // }
            // }
        }
    }
}

int32_t _ram_update_redeembits(struct mgw777 *mgw,uint64_t redeembits,uint64_t AMtxidbits,char *cointxid,struct address_entry *bp)
{
    struct NXT_asset *ap = mgw->ap;
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


int32_t ram_update_redeembits(struct mgw777 *mgw,cJSON *argjson,uint64_t AMtxidbits)
{
    cJSON *array;
    uint64_t redeembits;
    int32_t i,n,num = 0;
    struct address_entry B;
    char coinstr[MAX_JSON_FIELD],redeemtxid[MAX_JSON_FIELD],cointxid[MAX_JSON_FIELD];
    if ( extract_cJSON_str(coinstr,sizeof(coinstr),argjson,"coin") <= 0 )
        return(0);
    if ( ram != 0 && strcmp(mgw->coinstr,coinstr) != 0 )
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

void ram_setdispstr(char *buf,struct mgw777 *mgw,double startmilli)
{
    double estimatedV,estimatedB,estsizeV,estsizeB;
    estimatedV = estimate_completion(startmilli,mgw->Vblocks.processed,(int32_t)mgw->S.RTblocknum-mgw->Vblocks.blocknum)/60000;
    estimatedB = estimate_completion(startmilli,mgw->Bblocks.processed,(int32_t)mgw->S.RTblocknum-mgw->Bblocks.blocknum)/60000;
    estsizeV = (mgw->Vblocks.sum / (1 + mgw->Vblocks.count)) * mgw->S.RTblocknum;
    estsizeB = (mgw->Bblocks.sum / (1 + mgw->Bblocks.count)) * mgw->S.RTblocknum;
    sprintf(buf,"%-5s: RT.%d nonz.%d V.%d B.%d B64.%d B4096.%d | %s %s R%.2f | minutes: V%.1f B%.1f | outputs.%llu %.8f spends.%llu %.8f -> balance: %llu %.8f ave %.8f",mgw->coinstr,mgw->S.RTblocknum,mgw->nonzblocks,mgw->Vblocks.blocknum,mgw->Bblocks.blocknum,mgw->blocks64.blocknum,mgw->blocks4096.blocknum,_mbstr(estsizeV),_mbstr2(estsizeB),estsizeV/(estsizeB+1),estimatedV,estimatedB,(long long)mgw->S.numoutputs,dstr(mgw->S.totaloutputs),(long long)mgw->S.numspends,dstr(mgw->S.totalspends),(long long)(mgw->S.numoutputs - mgw->S.numspends),dstr(mgw->S.totaloutputs - mgw->S.totalspends),dstr(mgw->S.totaloutputs - mgw->S.totalspends)/(mgw->S.numoutputs - mgw->S.numspends));
}

void ram_disp_status(struct mgw777 *mgw)
{
    char buf[1024];
    int32_t i,n = 0;
    for (i=0; i<mgw->S.RTblocknum; i++)
    {
        if ( mgw->blocks.hps[i] != 0 )
            n++;
    }
    mgw->nonzblocks = n;
    ram_setdispstr(buf,ram,mgw->startmilli);
    fprintf(stderr,"%s\n",buf);
}
*/

char *_calc_withdrawaddr(char *withdrawaddr,struct mgw777 *mgw,struct NXT_assettxid *tp,cJSON *argjson)
{
    //cJSON *json;
    int32_t i,c;//,convert = 0;
    //struct ramchain *newram;
    char buf[MAX_JSON_FIELD],autoconvert[MAX_JSON_FIELD];//,issuer[MAX_JSON_FIELD],*retstr;
    copy_cJSON(withdrawaddr,cJSON_GetObjectItem(argjson,"withdrawaddr"));
    //if ( withdrawaddr[0] != 0 )
    //    return(withdrawaddr);
    //else return(0);
    if ( tp->convname[0] != 0 )
    {
        withdrawaddr[0] = 0;
        return(0);
    }
    copy_cJSON(autoconvert,cJSON_GetObjectItem(argjson,"autoconvert"));
    copy_cJSON(buf,cJSON_GetObjectItem(argjson,"teleport")); // "send" or <emailaddr>
    safecopy(tp->teleport,buf,sizeof(tp->teleport));
    tp->convassetid = tp->assetbits;
    /*if ( autoconvert[0] != 0 )
    {
        if ( (newram= get_ramchain(autoconvert)) == 0 )
        {
            if ( (retstr= _issue_getAsset(autoconvert)) != 0 )
            {
                if ( (json= cJSON_Parse(retstr)) != 0 )
                {
                    copy_cJSON(issuer,cJSON_GetObjectItem(json,"account"));
                    if ( is_trusted_issuer(issuer) > 0 )
                    {
                        copy_cJSON(tp->convname,cJSON_GetObjectItem(json,"name"));
                        convert = 1;
                    }
                    free_json(json);
                }
            }
        }
        else
        {
            strcpy(tp->convname,newmgw->coinstr);
            convert = 1;
        }
        if ( convert != 0 )
        {
            tp->minconvrate = get_API_float(cJSON_GetObjectItem(argjson,"rate"));
            tp->convexpiration = (int32_t)get_API_int(cJSON_GetObjectItem(argjson,"expiration"),0);
            if ( withdrawaddr[0] != 0 ) // no address means to create user credit
            {
                _stripwhite(withdrawaddr,0);
                tp->convwithdrawaddr = clonestr(withdrawaddr);
            }
        }
        else withdrawaddr[0] = autoconvert[0] = 0;
    }*/
    printf("PARSED.%s withdrawaddr.(%s) autoconvert.(%s)\n",mgw->coinstr,withdrawaddr,autoconvert);
    if ( withdrawaddr[0] == 0 || autoconvert[0] != 0 )
        return(0);
    for (i=0; withdrawaddr[i]!=0; i++)
        if ( (c= withdrawaddr[i]) < ' ' || c == '\\' || c == '"' )
            return(0);
    //printf("return.(%s)\n",withdrawaddr);
    return(withdrawaddr);
}

char *_parse_withdraw_instructions(char *destaddr,char *NXTaddr,struct mgw777 *mgw,struct NXT_assettxid *tp,struct NXT_asset *ap)
{
    char pubkey[1024],withdrawaddr[1024],*retstr = destaddr;
    int64_t amount,minwithdraw;
    cJSON *argjson = 0;
    destaddr[0] = withdrawaddr[0] = 0;
    if ( tp->redeemtxid == 0 )
    {
        printf("no redeem txid %s %s\n",mgw->coinstr,cJSON_Print(argjson));
        retstr = 0;
    }
    else
    {
        amount = tp->quantity * ap->mult;
        if ( tp->comment != 0 && (argjson= cJSON_Parse(tp->comment)) != 0 ) //(tp->comment[0] == '{' || tp->comment[0] == '[') &&
        {
            if ( _calc_withdrawaddr(withdrawaddr,mgw,tp,argjson) == 0 )
            {
                printf("(%llu) no withdraw.(%s) or autoconvert.(%s)\n",(long long)tp->redeemtxid,withdrawaddr,tp->comment);
                _complete_assettxid(tp);
                retstr = 0;
            }
        }
        if ( retstr != 0 )
        {
            minwithdraw = mgw->txfee * MIN_DEPOSIT_FACTOR;
            if ( amount <= minwithdraw )
            {
                printf("%llu: minimum withdrawal must be more than %.8f %s\n",(long long)tp->redeemtxid,dstr(minwithdraw),mgw->coinstr);
                _complete_assettxid(tp);
                retstr = 0;
            }
            else if ( withdrawaddr[0] == 0 )
            {
                printf("%llu: no withdraw address for %.8f | ",(long long)tp->redeemtxid,dstr(amount));
                _complete_assettxid(tp);
                retstr = 0;
            }
            else if ( mgw != 0 && get_pubkey(pubkey,mgw->coinstr,mgw->serverport,mgw->userpass,withdrawaddr) < 0 )
            {
                printf("%llu: invalid address.(%s) for NXT.%s %.8f validate.%d\n",(long long)tp->redeemtxid,withdrawaddr,NXTaddr,dstr(amount),get_pubkey(pubkey,mgw->coinstr,mgw->serverport,mgw->userpass,withdrawaddr));
                _complete_assettxid(tp);
                retstr = 0;
            }
        }
    }
    //printf("withdraw addr.(%s) for (%s)\n",withdrawaddr,NXTaddr);
    if ( retstr != 0 )
        strcpy(retstr,withdrawaddr);
    if ( argjson != 0 )
        free_json(argjson);
    return(retstr);
}

uint64_t _find_pending_transfers(uint64_t *pendingredeemsp,struct mgw777 *mgw)
{
    int32_t j,disable_newsends,specialsender,specialreceiver,numpending = 0;
    char sender[64],receiver[64],txidstr[512],withdrawaddr[512],*destaddr;
    struct NXT_assettxid *tp;
    struct NXT_asset *ap;
    struct cointx_info *cointx;
    uint64_t orphans = 0;
    *pendingredeemsp = 0;
    disable_newsends = ((mgw->numpendingsends > 0) || (mgw->S.gatewayid < 0));
    if ( (ap= mgw->ap) == 0 )
    {
        printf("no NXT_asset for %s\n",mgw->coinstr);
        return(0);
    }
    for (j=0; j<ap->num; j++)
    {
        tp = ap->txids[j];
        //if ( strcmp(mgw->coinstr,"BITS") == 0 )
        //printf("%d of %d: check %s.%llu completed.%d\n",j,ap->num,mgw->coinstr,(long long)tp->redeemtxid,tp->completed);
        if ( tp->completed == 0 )
        {
            expand_nxt64bits(sender,tp->senderbits);
            specialsender = _in_specialNXTaddrs(mgw,sender);
            expand_nxt64bits(receiver,tp->receiverbits);
            specialreceiver = _in_specialNXTaddrs(mgw,receiver);
            if ( (specialsender ^ specialreceiver) == 0 || tp->cointxid != 0 )
            {
                printf("autocomplete: %llu cointxid.%p\n",(long long)tp->redeemtxid,tp->cointxid);
                _complete_assettxid(tp);
            }
            else
            {
                if ( _is_limbo_redeem(mgw,tp->redeemtxid) != 0 )
                {
                    printf("autocomplete: limbo %llu cointxid.%p\n",(long long)tp->redeemtxid,tp->cointxid);
                    _complete_assettxid(tp);
                }
                //printf("receiver.%llu vs MGW.%llu\n",(long long)tp->receiverbits,(long long)mgw->MGWbits);
                if ( tp->receiverbits == mgw->MGWbits ) // redeem start
                {
                    destaddr = "coinaddr";
                    if ( _valid_txamount(mgw,tp->U.assetoshis,0) > 0 && (tp->convwithdrawaddr != 0 || (destaddr= _parse_withdraw_instructions(withdrawaddr,sender,mgw,tp,ap)) != 0) )
                    {
                        if ( tp->convwithdrawaddr == 0 )
                            tp->convwithdrawaddr = clonestr(destaddr);
                        if ( tp->redeemstarted == 0 )
                        {
                            printf("find_pending_transfers: redeem.%llu started %s %.8f for NXT.%s to %s\n",(long long)tp->redeemtxid,mgw->coinstr,dstr(tp->U.assetoshis),sender,destaddr!=0?destaddr:"no withdraw address");
                            tp->redeemstarted = (uint32_t)time(NULL);
                        }
                        else
                        {
                            int32_t i,numpayloads;
                            struct ramchain_hashptr *addrptr;
                            struct rampayload *payloads;
                            if ( (payloads= ram_addrpayloads(&addrptr,&numpayloads,mgw,destaddr)) != 0 && addrptr != 0 && numpayloads > 0 )
                            {
                                for (i=0; i<numpayloads; i++)
                                    if ( (dstr(tp->U.assetoshis) - dstr(payloads[i].value)) == .0101 ) // historical BTCD parameter
                                    {
                                        printf("(autocomplete.%llu payload.i%d >>>>>>>> %.8f <<<<<<<<<) ",(long long)tp->redeemtxid,i,dstr(payloads[i].value));
                                        _complete_assettxid(tp);
                                    }
                            }
                        }
                        if ( tp->completed == 0 && tp->convwithdrawaddr != 0 )
                        {
                            (*pendingredeemsp) += tp->U.assetoshis;
                            printf("%s NXT.%llu withdraw.(%llu %.8f).rt%d_%d_%d_%d.g%d -> %s elapsed %.1f minutes | pending.%d\n",mgw->coinstr,(long long)tp->senderbits,(long long)tp->redeemtxid,dstr(tp->U.assetoshis),mgw->S.enable_withdraws,mgw->S.is_realtime,(tp->height + mgw->withdrawconfirms) <= mgw->S.NXT_RTblocknum,mgw->S.MGWbalance >= 0,(int32_t)(tp->senderbits % NUM_GATEWAYS),tp->convwithdrawaddr,(double)(time(NULL) - tp->redeemstarted)/60,mgw->numpendingsends);
                            numpending++;
                            if ( disable_newsends == 0 )
                            {
                                if ( (cointx= _calc_cointx_withdraw(mgw,tp->convwithdrawaddr,tp->U.assetoshis,tp->redeemtxid)) != 0 )
                                {
                                    if ( ram_MGW_ready(mgw,0,tp->height,0,tp->U.assetoshis) > 0 )
                                    {
                                        ram_send_cointx(mgw,cointx);
                                        mgw->numpendingsends++;
                                        //ram_add_pendingsend(0,ram,tp,cointx);
                                        // disable_newsends = 1;
                                    } else printf("not ready to withdraw yet\n");
                                }
                                else if ( mgw->S.enable_withdraws != 0 && mgw->S.is_realtime != 0 && mgw->S.NXT_is_realtime != 0 )
                                {
                                    //tp->completed = 1; // ignore malformed requests for now
                                }
                            }
                            if ( mgw->S.gatewayid >= 0 && ram_check_consensus(txidstr,mgw,tp) != 0 )
                                printf("completed redeem.%llu with cointxid.%s\n",(long long)tp->redeemtxid,txidstr);
                            //printf("(%llu %.8f).%d ",(long long)tp->redeemtxid,dstr(tp->U.assetoshis),(int32_t)(time(NULL) - tp->redeemstarted));
                        } else printf("%llu %.8f: completed.%d withdraw.%p destaddr.%p\n",(long long)tp->redeemtxid,dstr(tp->U.assetoshis),tp->completed,tp->convwithdrawaddr,destaddr);
                    }
                    else if ( tp->completed == 0 && _valid_txamount(mgw,tp->U.assetoshis,0) > 0 )
                        printf("incomplete but skipped.%llu: %.8f destaddr.%s\n",(long long)tp->redeemtxid,dstr(tp->U.assetoshis),destaddr);
                    else printf("%s.%llu %.8f is too small, thank you for your donation to MGW\n",mgw->coinstr,(long long)tp->redeemtxid,dstr(tp->U.assetoshis)), tp->completed = 1;
                }
                else if ( tp->completed == 0 && specialsender != 0 ) // deposit complete w/o cointxid (shouldnt happen normally)
                {
                    orphans += tp->U.assetoshis;
                    _complete_assettxid(tp);
                    printf("find_pending_transfers: internal transfer.%llu limbo.%d complete %s %.8f to NXT.%s\n",(long long)tp->redeemtxid,_is_limbo_redeem(mgw,tp->redeemtxid),mgw->coinstr,dstr(tp->U.assetoshis),receiver);
                } else tp->completed = 1; // this is some independent tx
            }
        }
    }
    if ( numpending == 0 && mgw->numpendingsends != 0 )
    {
        printf("All pending withdraws done!\n");
        mgw->numpendingsends = 0;
    }
    return(orphans);
}

