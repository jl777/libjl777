//
//  mgwNXT.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_mgwNXT_h
#define crypto777_mgwNXT_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"
#include "uthash.h"
#include "utils777.c"
#include "NXT777.c"
#include "msig.c"
#include "search.c"

#define DEPOSIT_XFER_DURATION 5
#define MIN_DEPOSIT_FACTOR 5

#define GET_COINDEPOSIT_ADDRESS 'g'
#define BIND_DEPOSIT_ADDRESS 'b'
#define DEPOSIT_CONFIRMED 'd'
#define MONEY_SENT 'm'

int32_t is_trusted_issuer(char *issuer);
uint64_t MGWtransfer_asset(cJSON **transferjsonp,int32_t forceflag,uint64_t nxt64bits,char *depositors_pubkey,struct NXT_asset *ap,uint64_t value,char *coinaddr,char *txidstr,struct address_entry *entry,int32_t *buyNXTp,char *srvNXTADDR,char *srvNXTACCTSECRET,int32_t deadline);

#endif
#else
#ifndef crypto777_mgwNXT_c
#define crypto777_mgwNXT_c

#ifndef crypto777_mgwNXT_h
#define DEFINES_ONLY
#include "mgwNXT.c"
#undef DEFINES_ONLY
#endif

int32_t is_trusted_issuer(char *issuer)
{
    int32_t i,n;
    uint64_t issuerbits;
    if ( (n= SUPERNET.numissuers) > 0 )
    {
        issuerbits = conv_rsacctstr(issuer,0);
        for (i=0; i<n; i++)
        {
            if ( issuerbits == SUPERNET.issuers[i] )
                return(1);
        }
    }
    /*char str[MAX_JSON_FIELD];
    array = cJSON_GetObjectItem(MGWconf,"issuers");
    if ( array != 0 && is_cJSON_Array(array) != 0 )
    {
        n = cJSON_GetArraySize(array);
        for (i=0; i<n; i++)
        {
            if ( array == 0 || n == 0 )
                break;
            copy_cJSON(str,cJSON_GetArrayItem(array,i));
            if ( str[0] == 'N' && str[1] == 'X' && str[2] == 'T' )
            {
                nxt64bits = conv_rsacctstr(str,0);
                printf("str.(%s) -> %llu\n",str,(long long)nxt64bits);
                expand_nxt64bits(str,nxt64bits);
            }
            if ( strcmp(str,issuer) == 0 )
                return(1);
        }
    }*/
    return(0);
}

cJSON *_process_MGW_message(struct ramchain_info *ram,uint32_t height,int32_t funcid,cJSON *argjson,uint64_t itemid,uint64_t units,char *sender,char *receiver,char *txid)
{
    struct multisig_addr *decode_msigjson(char *NXTaddr,cJSON *obj,char *sender);
    void update_coinacct_addresses(uint64_t nxt64bits,cJSON *json,char *txid);
    uint64_t nxt64bits = calc_nxt64bits(sender);
    struct multisig_addr *msig;
    int32_t len;
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
                    if ( strcmp(msig->coinstr,ram->name) == 0 && find_msigaddr(&len,msig->coinstr,msig->NXTaddr,msig->multisigaddr) == 0 )
                    {
                        //if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone > 1 )
                        //    fprintf(stderr,"BINDFUNC: %s func.(%c) %s -> %s txid.(%s)\n",msig->coinstr,funcid,sender,receiver,txid);
                        if ( update_msig_info(msig,1,sender) > 0 )
                        {
                            update_MGW_msig(msig,sender);
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
                        _complete_assettxid(tp);
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

uint64_t MGWtransfer_asset(cJSON **transferjsonp,int32_t forceflag,uint64_t nxt64bits,char *depositors_pubkey,struct NXT_asset *ap,uint64_t value,char *coinaddr,char *txidstr,struct address_entry *entry,int32_t *buyNXTp,char *srvNXTADDR,char *srvNXTACCTSECRET,int32_t deadline)
{
    double get_current_rate(char *base,char *rel);
    char buf[MAX_JSON_FIELD],nxtassetidstr[64],numstr[64],assetidstr[64],rsacct[64],NXTaddr[64],comment[MAX_JSON_FIELD],*errjsontxt,*str;
    uint64_t depositid,convamount,total = 0;
    int32_t haspubkey,iter,flag,buyNXT = *buyNXTp;
    double rate;
    cJSON *pair,*errjson,*item;
    conv_rsacctstr(rsacct,nxt64bits);
    issue_getpubkey(&haspubkey,rsacct);
    //printf("UNPAID cointxid.(%s) <-> (%u %d %d)\n",txidstr,entry->blocknum,entry->txind,entry->v);
    if ( ap->mult == 0 )
    {
        fprintf(stderr,"FATAL: ap->mult is 0 for %s\n",ap->name);
        exit(-1);
    }
    expand_nxt64bits(NXTaddr,nxt64bits);
    sprintf(comment,"{\"coin\":\"%s\",\"coinaddr\":\"%s\",\"cointxid\":\"%s\",\"coinblocknum\":%u,\"cointxind\":%u,\"coinv\":%u,\"amount\":\"%.8f\",\"sender\":\"%s\",\"receiver\":\"%llu\",\"timestamp\":%u,\"quantity\":\"%llu\"}",ap->name,coinaddr,txidstr,entry->blocknum,entry->txind,entry->v,dstr(value),srvNXTADDR,(long long)nxt64bits,(uint32_t)time(NULL),(long long)(value/ap->mult));
    pair = cJSON_Parse(comment);
    cJSON_AddItemToObject(pair,"NXT",cJSON_CreateString(NXTaddr));
    printf("forceflag.%d haspubkey.%d >>>>>>>>>>>>>> Need to transfer %.8f %ld assetoshis | %s to %llu for (%s) %s\n",forceflag,haspubkey,dstr(value),(long)(value/ap->mult),ap->name,(long long)nxt64bits,txidstr,comment);
    total += value;
    convamount = 0;
    if ( haspubkey == 0 && buyNXT > 0 )
    {
        if ( (rate = get_current_rate(ap->name,"NXT")) != 0. )
        {
            if ( buyNXT > MAX_BUYNXT )
                buyNXT = MAX_BUYNXT;
            convamount = ((double)(buyNXT+2) * SATOSHIDEN) / rate; // 2 NXT extra to cover the 2 NXT txfees
            if ( convamount >= value )
            {
                convamount = value / 2;
                buyNXT = ((convamount * rate) / SATOSHIDEN);
            }
            cJSON_AddItemToObject(pair,"rate",cJSON_CreateNumber(rate));
            cJSON_AddItemToObject(pair,"conv",cJSON_CreateNumber(dstr(convamount)));
            cJSON_AddItemToObject(pair,"buyNXT",cJSON_CreateNumber(buyNXT));
            value -= convamount;
        }
    } else buyNXT = 0;
    if ( forceflag > 0 && (value > 0 || convamount > 0) )
    {
        flag = 0;
        expand_nxt64bits(nxtassetidstr,NXT_ASSETID);
        for (iter=(value==0); iter<2; iter++)
        {
            errjsontxt = 0;
            str = cJSON_Print(pair);
            _stripwhite(str,' ');
            expand_nxt64bits(assetidstr,ap->assetbits);
            depositid = issue_transferAsset(&errjsontxt,0,srvNXTACCTSECRET,NXTaddr,(iter == 0) ? assetidstr : nxtassetidstr,(iter == 0) ? (value/ap->mult) : buyNXT*SATOSHIDEN,MIN_NQTFEE,deadline,str,depositors_pubkey);
            free(str);
            if ( depositid != 0 && errjsontxt == 0 )
            {
                printf("%s worked.%llu\n",(iter == 0) ? "deposit" : "convert",(long long)depositid);
                if ( iter == 1 )
                    *buyNXTp = buyNXT = 0;
                flag++;
                //add_pendingxfer(0,depositid);
                if ( transferjsonp != 0 )
                {
                    if ( *transferjsonp == 0 )
                        *transferjsonp = cJSON_CreateArray();
                    sprintf(numstr,"%llu",(long long)depositid);
                    cJSON_AddItemToObject(pair,(iter == 0) ? "depositid" : "convertid",cJSON_CreateString(numstr));
                }
            }
            else if ( errjsontxt != 0 )
            {
                printf("%s failed.(%s)\n",(iter == 0) ? "deposit" : "convert",errjsontxt);
                if ( 1 && (errjson= cJSON_Parse(errjsontxt)) != 0 )
                {
                    if ( (item= cJSON_GetObjectItem(errjson,"error")) != 0 )
                    {
                        copy_cJSON(buf,item);
                        cJSON_AddItemToObject(pair,(iter == 0) ? "depositerror" : "converterror",cJSON_CreateString(buf));
                    }
                    free_json(errjson);
                }
                else cJSON_AddItemToObject(pair,(iter == 0) ? "depositerror" : "converterror",cJSON_CreateString(errjsontxt));
                free(errjsontxt);
            }
            if ( buyNXT == 0 )
                break;
        }
        if ( flag != 0 )
        {
            void *extract_jsonints(cJSON *item,void *arg,void *arg2);
            int32_t jsonstrcmp(void *ref,void *item);
            str = cJSON_Print(pair);
            _stripwhite(str,' ');
            fprintf(stderr,"updatedeposit.ALL (%s)\n",str);
            update_MGW_jsonfile(set_MGW_depositfname,extract_jsonints,jsonstrcmp,0,str,"coinv","cointxind");
            fprintf(stderr,"updatedeposit.%s (%s)\n",NXTaddr,str);
            update_MGW_jsonfile(set_MGW_depositfname,extract_jsonints,jsonstrcmp,NXTaddr,str,"coinv","cointxind");
            free(str);
        }
    }
    if ( transferjsonp != 0 )
        cJSON_AddItemToArray(*transferjsonp,pair);
    else free_json(pair);
    return(total);
}

#endif
#endif
