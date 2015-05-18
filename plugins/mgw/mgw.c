//
//  MGW v6
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
#include "gen1auth.c"
#include "gen1pub.c"
#include "gen1.c"
#include "tokens.c"
#include "init.c"
#include "api.c"
#include "search.c"
#include "db777.c"
#include "huff.c"
#include "msig.c"
#include "mgwNXT.c"
#include "mgwcomms.c"
//#undef DEFINES_ONLY

//#define DEPOSIT_XFER_DURATION 5
#define MIN_DEPOSIT_FACTOR 5

char *_calc_withdrawaddr(char *withdrawaddr,struct ramchain_info *ram,struct NXT_assettxid *tp,cJSON *argjson)
{
    cJSON *json;
    int32_t i,c,convert = 0;
    struct ramchain_info *newram;
    char buf[MAX_JSON_FIELD],autoconvert[MAX_JSON_FIELD],issuer[MAX_JSON_FIELD],*retstr;
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
    if ( autoconvert[0] != 0 )
    {
        if ( (newram= get_ramchain_info(autoconvert)) == 0 )
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
            strcpy(tp->convname,newram->name);
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
    }
    printf("PARSED.%s withdrawaddr.(%s) autoconvert.(%s)\n",ram->name,withdrawaddr,autoconvert);
    if ( withdrawaddr[0] == 0 || autoconvert[0] != 0 )
        return(0);
    for (i=0; withdrawaddr[i]!=0; i++)
        if ( (c= withdrawaddr[i]) < ' ' || c == '\\' || c == '"' )
            return(0);
    //printf("return.(%s)\n",withdrawaddr);
    return(withdrawaddr);
}

char *_parse_withdraw_instructions(char *destaddr,char *NXTaddr,struct ramchain_info *ram,struct NXT_assettxid *tp,struct NXT_asset *ap)
{
    char pubkey[1024],withdrawaddr[1024],*retstr = destaddr;
    int64_t amount,minwithdraw;
    cJSON *argjson = 0;
    destaddr[0] = withdrawaddr[0] = 0;
    if ( tp->redeemtxid == 0 )
    {
        printf("no redeem txid %s %s\n",ram->name,cJSON_Print(argjson));
        retstr = 0;
    }
    else
    {
        amount = tp->quantity * ap->mult;
        if ( tp->comment != 0 && (argjson= cJSON_Parse(tp->comment)) != 0 ) //(tp->comment[0] == '{' || tp->comment[0] == '[') &&
        {
            if ( _calc_withdrawaddr(withdrawaddr,ram,tp,argjson) == 0 )
            {
                printf("(%llu) no withdraw.(%s) or autoconvert.(%s)\n",(long long)tp->redeemtxid,withdrawaddr,tp->comment);
                _complete_assettxid(tp);
                retstr = 0;
            }
        }
        if ( retstr != 0 )
        {
            minwithdraw = ram->txfee * MIN_DEPOSIT_FACTOR;
            if ( amount <= minwithdraw )
            {
                printf("%llu: minimum withdrawal must be more than %.8f %s\n",(long long)tp->redeemtxid,dstr(minwithdraw),ram->name);
                _complete_assettxid(tp);
                retstr = 0;
            }
            else if ( withdrawaddr[0] == 0 )
            {
                printf("%llu: no withdraw address for %.8f | ",(long long)tp->redeemtxid,dstr(amount));
                _complete_assettxid(tp);
                retstr = 0;
            }
            else if ( ram != 0 && get_pubkey(pubkey,ram->name,ram->serverport,ram->userpass,withdrawaddr) < 0 )
            {
                printf("%llu: invalid address.(%s) for NXT.%s %.8f validate.%d\n",(long long)tp->redeemtxid,withdrawaddr,NXTaddr,dstr(amount),get_pubkey(pubkey,ram->name,ram->serverport,ram->userpass,withdrawaddr));
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

uint64_t _find_pending_transfers(uint64_t *pendingredeemsp,struct ramchain_info *ram)
{
    int32_t j,disable_newsends,specialsender,specialreceiver,numpending = 0;
    char sender[64],receiver[64],txidstr[512],withdrawaddr[512],*destaddr;
    struct NXT_assettxid *tp;
    struct NXT_asset *ap;
    struct cointx_info *cointx;
    uint64_t orphans = 0;
    *pendingredeemsp = 0;
    disable_newsends = ((ram->numpendingsends > 0) || (ram->S.gatewayid < 0));
    if ( (ap= ram->ap) == 0 )
    {
        printf("no NXT_asset for %s\n",ram->name);
        return(0);
    }
    for (j=0; j<ap->num; j++)
    {
        tp = ap->txids[j];
        //if ( strcmp(ram->name,"BITS") == 0 )
        //printf("%d of %d: check %s.%llu completed.%d\n",j,ap->num,ram->name,(long long)tp->redeemtxid,tp->completed);
        if ( tp->completed == 0 )
        {
            expand_nxt64bits(sender,tp->senderbits);
            specialsender = _in_specialNXTaddrs(ram->special_NXTaddrs,ram->numspecials,sender);
            expand_nxt64bits(receiver,tp->receiverbits);
            specialreceiver = _in_specialNXTaddrs(ram->special_NXTaddrs,ram->numspecials,receiver);
            if ( (specialsender ^ specialreceiver) == 0 || tp->cointxid != 0 )
            {
                printf("autocomplete: %llu cointxid.%p\n",(long long)tp->redeemtxid,tp->cointxid);
                _complete_assettxid(tp);
            }
            else
            {
                if ( _is_limbo_redeem(ram,tp->redeemtxid) != 0 )
                {
                    printf("autocomplete: limbo %llu cointxid.%p\n",(long long)tp->redeemtxid,tp->cointxid);
                    _complete_assettxid(tp);
                }
                //printf("receiver.%llu vs MGW.%llu\n",(long long)tp->receiverbits,(long long)ram->MGWbits);
                if ( tp->receiverbits == ram->MGWbits ) // redeem start
                {
                    destaddr = "coinaddr";
                    if ( _valid_txamount(ram,tp->U.assetoshis,0) > 0 && (tp->convwithdrawaddr != 0 || (destaddr= _parse_withdraw_instructions(withdrawaddr,sender,ram,tp,ap)) != 0) )
                    {
                        if ( tp->convwithdrawaddr == 0 )
                            tp->convwithdrawaddr = clonestr(destaddr);
                        if ( tp->redeemstarted == 0 )
                        {
                            printf("find_pending_transfers: redeem.%llu started %s %.8f for NXT.%s to %s\n",(long long)tp->redeemtxid,ram->name,dstr(tp->U.assetoshis),sender,destaddr!=0?destaddr:"no withdraw address");
                            tp->redeemstarted = (uint32_t)time(NULL);
                        }
                        else
                        {
                            int32_t i,numpayloads;
                            struct ramchain_hashptr *addrptr;
                            struct rampayload *payloads;
                            if ( (payloads= ram_addrpayloads(&addrptr,&numpayloads,ram,destaddr)) != 0 && addrptr != 0 && numpayloads > 0 )
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
                            printf("%s NXT.%llu withdraw.(%llu %.8f).rt%d_%d_%d_%d.g%d -> %s elapsed %.1f minutes | pending.%d\n",ram->name,(long long)tp->senderbits,(long long)tp->redeemtxid,dstr(tp->U.assetoshis),ram->S.enable_withdraws,ram->S.is_realtime,(tp->height + ram->withdrawconfirms) <= ram->S.NXT_RTblocknum,ram->S.MGWbalance >= 0,(int32_t)(tp->senderbits % NUM_GATEWAYS),tp->convwithdrawaddr,(double)(time(NULL) - tp->redeemstarted)/60,ram->numpendingsends);
                            numpending++;
                            if ( disable_newsends == 0 )
                            {
                                if ( (cointx= _calc_cointx_withdraw(ram,tp->convwithdrawaddr,tp->U.assetoshis,tp->redeemtxid)) != 0 )
                                {
                                    if ( ram_MGW_ready(ram,0,tp->height,0,tp->U.assetoshis) > 0 )
                                    {
                                        ram_send_cointx(ram,cointx);
                                        ram->numpendingsends++;
                                        //ram_add_pendingsend(0,ram,tp,cointx);
                                        // disable_newsends = 1;
                                    } else printf("not ready to withdraw yet\n");
                                }
                                else if ( ram->S.enable_withdraws != 0 && ram->S.is_realtime != 0 && ram->S.NXT_is_realtime != 0 )
                                {
                                    //tp->completed = 1; // ignore malformed requests for now
                                }
                            }
                            if ( ram->S.gatewayid >= 0 && ram_check_consensus(txidstr,ram,tp) != 0 )
                                printf("completed redeem.%llu with cointxid.%s\n",(long long)tp->redeemtxid,txidstr);
                            //printf("(%llu %.8f).%d ",(long long)tp->redeemtxid,dstr(tp->U.assetoshis),(int32_t)(time(NULL) - tp->redeemstarted));
                        } else printf("%llu %.8f: completed.%d withdraw.%p destaddr.%p\n",(long long)tp->redeemtxid,dstr(tp->U.assetoshis),tp->completed,tp->convwithdrawaddr,destaddr);
                    }
                    else if ( tp->completed == 0 && _valid_txamount(ram,tp->U.assetoshis,0) > 0 )
                        printf("incomplete but skipped.%llu: %.8f destaddr.%s\n",(long long)tp->redeemtxid,dstr(tp->U.assetoshis),destaddr);
                    else printf("%s.%llu %.8f is too small, thank you for your donation to MGW\n",ram->name,(long long)tp->redeemtxid,dstr(tp->U.assetoshis)), tp->completed = 1;
                }
                else if ( tp->completed == 0 && specialsender != 0 ) // deposit complete w/o cointxid (shouldnt happen normally)
                {
                    orphans += tp->U.assetoshis;
                    _complete_assettxid(tp);
                    printf("find_pending_transfers: internal transfer.%llu limbo.%d complete %s %.8f to NXT.%s\n",(long long)tp->redeemtxid,_is_limbo_redeem(ram,tp->redeemtxid),ram->name,dstr(tp->U.assetoshis),receiver);
                } else tp->completed = 1; // this is some independent tx
            }
        }
    }
    if ( numpending == 0 && ram->numpendingsends != 0 )
    {
        printf("All pending withdraws done!\n");
        ram->numpendingsends = 0;
    }
    return(orphans);
}

