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
#include "bitcoind.c"
#include "ramchain.c"
#include "tokens.c"
#include "init.c"
#include "api.c"
#include "search.c"
#include "huff.c"
#include "msig.c"
#undef DEFINES_ONLY

//#define DEPOSIT_XFER_DURATION 5
#define MIN_DEPOSIT_FACTOR 5

void set_handler_fname(char *fname,char *handler,char *name)
{
    if ( strstr("../",name) != 0 || strstr("..\\",name) != 0 || name[0] == '/' || name[0] == '\\' || strcmp(name,"..") == 0  || strcmp(name,"*") == 0 )
    {
        //printf("(%s) invalid_filename.(%s) %p %p %d %d %d %d\n",handler,name,strstr("../",name),strstr("..\\",name),name[0] == '/',name[0] == '\\',strcmp(name,".."),strcmp(name,"*"));
        name = "invalid_filename";
    }
    sprintf(fname,"%s/%s/%s",DATADIR,handler,name);
}

int32_t load_handler_fname(void *dest,int32_t len,char *handler,char *name)
{
    FILE *fp;
    int32_t retval = -1;
    char fname[1024];
    set_handler_fname(fname,handler,name);
    if ( (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
    {
        fseek(fp,0,SEEK_END);
        if ( ftell(fp) == len )
        {
            rewind(fp);
            if ( fread(dest,1,len,fp) == len )
                retval = len;
        }
        fclose(fp);
    }
    return(retval);
}

void _set_RTmgwname(char *RTmgwname,char *name,char *coinstr,int32_t gatewayid,uint64_t redeemtxid)
{
    sprintf(name,"%s.%llu.g%d",coinstr,(long long)redeemtxid,gatewayid);
    set_handler_fname(RTmgwname,"RTmgw",name);
}


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

int32_t is_trusted_issuer(char *issuer)
{
    int32_t i,n;
    cJSON *array;
    uint64_t nxt64bits;
    char str[MAX_JSON_FIELD];
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
    }
    return(0);
}

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
                _complete_assettxid(ram,tp);
                retstr = 0;
            }
        }
        if ( retstr != 0 )
        {
            minwithdraw = ram->txfee * MIN_DEPOSIT_FACTOR;
            if ( amount <= minwithdraw )
            {
                printf("%llu: minimum withdrawal must be more than %.8f %s\n",(long long)tp->redeemtxid,dstr(minwithdraw),ram->name);
                _complete_assettxid(ram,tp);
                retstr = 0;
            }
            else if ( withdrawaddr[0] == 0 )
            {
                printf("%llu: no withdraw address for %.8f | ",(long long)tp->redeemtxid,dstr(amount));
                _complete_assettxid(ram,tp);
                retstr = 0;
            }
            else if ( ram != 0 && get_pubkey(pubkey,ram->name,ram->serverport,ram->userpass,withdrawaddr) < 0 )
            {
                printf("%llu: invalid address.(%s) for NXT.%s %.8f validate.%d\n",(long long)tp->redeemtxid,withdrawaddr,NXTaddr,dstr(amount),get_pubkey(pubkey,ram->name,ram->serverport,ram->userpass,withdrawaddr));
                _complete_assettxid(ram,tp);
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

struct NXT_assettxid *_process_realtime_MGW(int32_t *sendip,struct ramchain_info **ramp,struct cointx_info *cointx,char *sender,char *recvname)
{
    uint32_t crc;
    int32_t gatewayid;
    char redeemtxidstr[64];
    struct ramchain_info *ram;
    *ramp = 0;
    *sendip = -1;
    if ( (ram= get_ramchain_info(cointx->coinstr)) == 0 )
        printf("cant find coin.(%s)\n",cointx->coinstr);
    else
    {
        *ramp = ram;
        if ( strncmp(recvname,ram->name,strlen(ram->name)) != 0 ) // + archive/RTmgw/
        {
            printf("_process_realtime_MGW: coin mismatch recvname.(%s) vs (%s).%ld\n",recvname,ram->name,strlen(ram->name));
            return(0);
        }
        crc = _crc32(0,(uint8_t *)((long)cointx+sizeof(cointx->crc)),(int32_t)(cointx->allocsize-sizeof(cointx->crc)));
        if ( crc != cointx->crc )
        {
            printf("_process_realtime_MGW: crc mismatch %x vs %x\n",crc,cointx->crc);
            return(0);
        }
        expand_nxt64bits(redeemtxidstr,cointx->redeemtxid);
        if ( strncmp(recvname+strlen(ram->name)+1,redeemtxidstr,strlen(redeemtxidstr)) != 0 )
        {
            printf("_process_realtime_MGW: redeemtxid mismatch (%s) vs (%s)\n",recvname+strlen(ram->name)+1,redeemtxidstr);
            return(0);
        }
        gatewayid = cointx->gatewayid;
        if ( gatewayid < 0 || gatewayid >= ram->numgateways )
        {
            printf("_process_realtime_MGW: illegal gatewayid.%d\n",gatewayid);
            return(0);
        }
        if ( strcmp(ram->special_NXTaddrs[gatewayid],sender) != 0 )
        {
            printf("_process_realtime_MGW: gatewayid mismatch %d.(%s) vs %s\n",gatewayid,ram->special_NXTaddrs[gatewayid],sender);
            return(0);
        }
        //ram_add_pendingsend(0,ram,0,cointx);
        printf("GOT <<<<<<<<<<<< _process_realtime_MGW.%d coin.(%s) %.8f crc %08x redeemtxid.%llu\n",gatewayid,cointx->coinstr,dstr(cointx->amount),cointx->batchcrc,(long long)cointx->redeemtxid);
    }
    return(0);
}

int32_t cointxcmp(struct cointx_info *txA,struct cointx_info *txB)
{
    if ( txA != 0 && txB != 0 )
    {
        if ( txA->batchcrc == txB->batchcrc )
            return(0);
    }
    return(-1);
}

char *_submit_withdraw(struct ramchain_info *ram,struct cointx_info *cointx,char *othersignedtx)
{
    FILE *fp;
    char fname[512],*cointxid,*signed2transaction;
    if ( ram->S.gatewayid < 0 )
        return(0);
    if ( cosigntransaction(&cointxid,&signed2transaction,ram->name,ram->serverport,ram->userpass,cointx,othersignedtx) > 0 )
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
    int32_t _ram_update_redeembits(struct ramchain_info *ram,uint64_t redeembits,uint64_t AMtxidbits,char *cointxid,struct address_entry *bp);
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

char *ram_check_consensus(char *txidstr,struct ramchain_info *ram,struct NXT_assettxid *tp)
{
    uint64_t retval,allocsize;
    char RTmgwname[1024],name[512],cmd[1024],hopNXTaddr[64],*cointxid,*retstr = 0;
    int32_t i,gatewayid;
    struct cointx_info *cointxs[16],*othercointx;
    memset(cointxs,0,sizeof(cointxs));
    for (gatewayid=0; gatewayid<ram->numgateways; gatewayid++)
    {
        _set_RTmgwname(RTmgwname,name,ram->name,gatewayid,tp->redeemtxid);
        if ( (cointxs[gatewayid]= loadfile(&allocsize,RTmgwname)) == 0 )
        {
            char *send_tokenized_cmd(int32_t queueflag,char *hopNXTaddr,int32_t L,char *verifiedNXTaddr,char *NXTACCTSECRET,char *cmdstr,char *destNXTaddr);
            hopNXTaddr[0] = 0;
            sprintf(cmd,"{\"requestType\":\"getfile\",\"NXT\":\"%s\",\"timestamp\":\"%ld\",\"name\":\"%s\",\"handler\":\"RTmgw\"}",ram->srvNXTADDR,(long)time(NULL),name);
            if ( (retstr= send_tokenized_cmd(0,hopNXTaddr,0,ram->srvNXTADDR,ram->srvNXTACCTSECRET,cmd,ram->special_NXTaddrs[gatewayid])) != 0 )
                free(retstr), retstr = 0;
            printf("cant find.(%s) for %llu %.8f | sent.(%s) to %s\n",RTmgwname,(long long)tp->redeemtxid,dstr(tp->U.assetoshis),cmd,ram->special_NXTaddrs[gatewayid]);
            break;
        }
        for (i=0; i<gatewayid; i++)
            if ( cointxcmp(cointxs[i],cointxs[gatewayid]) != 0 )
            {
                printf("MGW%d %x != %x MGW%d for redeem.%llu %.8f\n",i,cointxs[i]->batchcrc,cointxs[gatewayid]->batchcrc,gatewayid,(long long)tp->redeemtxid,dstr(tp->U.assetoshis));
                break;
            }
    }
    if ( gatewayid != ram->numgateways )
    {
        for (i=0; i<=gatewayid; i++)
            free(cointxs[i]);
        return(0);
    }
    printf("got consensus for %llu %.8f\n",(long long)tp->redeemtxid,dstr(tp->U.assetoshis));
    if ( ram_MGW_ready(ram,0,tp->height,tp->senderbits,tp->U.assetoshis) > 0 )
    {
        if ( (retval= ram_verify_NXTtxstillthere(ram->ap->mult,tp->redeemtxid)) != tp->U.assetoshis )
        {
            fprintf(stderr,"ram_check_consensus tx gone due to a fork. NXT.%llu txid.%llu %.8f vs retval %.8f\n",(long long)tp->senderbits,(long long)tp->redeemtxid,dstr(tp->U.assetoshis),dstr(retval));
            exit(1); // seems the best thing to do
        }
        othercointx = cointxs[(ram->S.gatewayid ^ 1) % ram->numgateways];
        //printf("[%d] othercointx = %p\n",(ram->S.gatewayid ^ 1) % ram->numgateways,othercointx);
        if ( (cointxid= _sign_and_sendmoney(txidstr,ram,cointxs[ram->S.gatewayid],othercointx->signedtx,&tp->redeemtxid,&tp->U.assetoshis,1)) != 0 )
        {
            _complete_assettxid(ram,tp);
            //ram_add_pendingsend(&sendi,ram,tp,0);
            printf("completed redeem.%llu for %.8f cointxidstr.%s\n",(long long)tp->redeemtxid,dstr(tp->U.assetoshis),txidstr);
            retstr = txidstr;
        }
        else printf("ram_check_consensus error _sign_and_sendmoney for NXT.%llu redeem.%llu %.8f (%s)\n",(long long)tp->senderbits,(long long)tp->redeemtxid,dstr(tp->U.assetoshis),othercointx->signedtx);
    }
    for (gatewayid=0; gatewayid<ram->numgateways; gatewayid++)
        free(cointxs[gatewayid]);
    return(retstr);
}

void _RTmgw_handler(struct transfer_args *args)
{
    struct NXT_assettxid *tp;
    struct ramchain_info *ram;
    int32_t sendi;
    //char txidstr[512];
    printf("_RTmgw_handler(%s %d bytes)\n",args->name,args->totallen);
    if ( (tp= _process_realtime_MGW(&sendi,&ram,(struct cointx_info *)args->data,args->sender,args->name)) != 0 )
    {
        if ( sendi >= ram->numpendingsends || sendi < 0 || ram->pendingsends[sendi] != tp )
        {
            printf("FATAL: _RTmgw_handler sendi %d >= %d ram->numpendingsends || sendi %d < 0 || %p ram->pendingsends[sendi] != %ptp\n",sendi,ram->numpendingsends,sendi,ram->pendingsends[sendi],tp);
            exit(1);
        }
        //ram_check_consensus(txidstr,ram,tp);
    }
    //getchar();
}

void ram_send_cointx(struct ramchain_info *ram,struct cointx_info *cointx)
{
    char *start_transfer(char *previpaddr,char *sender,char *verifiedNXTaddr,char *NXTACCTSECRET,char *dest,char *name,uint8_t *data,int32_t totallen,int32_t timeout,char *handler,int32_t syncmem);
    char RTmgwname[512],name[512],*retstr;
    int32_t gatewayid;
    FILE *fp;
    _set_RTmgwname(RTmgwname,name,cointx->coinstr,cointx->gatewayid,cointx->redeemtxid);
    cointx->crc = _crc32(0,(uint8_t *)((long)cointx+sizeof(cointx->crc)),(int32_t)(cointx->allocsize - sizeof(cointx->crc)));
    if ( (fp= fopen(os_compatible_path(RTmgwname),"wb")) != 0 )
    {
        printf("save to (%s).%d crc.%x | batchcrc %x\n",RTmgwname,cointx->allocsize,cointx->crc,cointx->batchcrc);
        fwrite(cointx,1,cointx->allocsize,fp);
        fclose(fp);
    }
    for (gatewayid=0; gatewayid<NUM_GATEWAYS; gatewayid++)
    {
        if ( gatewayid != cointx->gatewayid )
        {
            retstr = start_transfer(0,ram->srvNXTADDR,ram->srvNXTADDR,ram->srvNXTACCTSECRET,Server_ipaddrs[gatewayid],name,(uint8_t *)cointx,cointx->allocsize,300,"RTmgw",1);
            if ( retstr != 0 )
                free(retstr);
        }
        fprintf(stderr,"got publish_withdraw_info.%d -> %d coin.(%s) %.8f crc %08x\n",ram->S.gatewayid,gatewayid,cointx->coinstr,dstr(cointx->amount),cointx->batchcrc);
    }
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

struct cointx_info *_calc_cointx_withdraw(struct ramchain_info *ram,char *destaddr,uint64_t value,uint64_t redeemtxid)
{
    //int64 nPayFee = nTransactionFee * (1 + (int64)nBytes / 1000);
    char *rawparams,*changeaddr;
    int64_t MGWfee,sum,amount;
    int32_t opreturn_output,numoutputs = 0;
    struct cointx_info *cointx,TX,*rettx = 0;
    cointx = &TX;
    memset(cointx,0,sizeof(*cointx));
    strcpy(cointx->coinstr,ram->name);
    cointx->redeemtxid = redeemtxid;
    cointx->gatewayid = ram->S.gatewayid;
    MGWfee = 0*(value >> 10) + (2 * (ram->txfee + ram->NXTfee_equiv)) - ram->minoutput - ram->txfee;
    if ( value <= MGWfee + ram->minoutput + ram->txfee )
    {
        printf("%s redeem.%llu withdraw %.8f < MGWfee %.8f + minoutput %.8f + txfee %.8f\n",ram->name,(long long)redeemtxid,dstr(value),dstr(MGWfee),dstr(ram->minoutput),dstr(ram->txfee));
        return(0);
    }
    strcpy(cointx->outputs[numoutputs].coinaddr,ram->marker2);
    if ( strcmp(destaddr,ram->marker2) == 0 )
        cointx->outputs[numoutputs++].value = value - ram->minoutput - ram->txfee;
    else
    {
        cointx->outputs[numoutputs++].value = MGWfee;
        strcpy(cointx->outputs[numoutputs].coinaddr,destaddr);
        cointx->outputs[numoutputs++].value = value - MGWfee - ram->minoutput - ram->txfee;
    }
    opreturn_output = numoutputs;
    strcpy(cointx->outputs[numoutputs].coinaddr,ram->opreturnmarker);
    cointx->outputs[numoutputs++].value = ram->minoutput;
    cointx->numoutputs = numoutputs;
    cointx->amount = amount = (MGWfee + value + ram->minoutput + ram->txfee);
    fprintf(stderr,"calc_withdraw.%s %llu amount %.8f -> balance %.8f\n",ram->name,(long long)redeemtxid,dstr(cointx->amount),dstr(ram->S.MGWbalance));
    if ( ram->S.MGWbalance >= 0 )
    {
        if ( (sum= _calc_cointx_inputs(ram,cointx,cointx->amount)) >= (cointx->amount + ram->txfee) )
        {
            if ( cointx->change != 0 )
            {
                changeaddr = (strcmp(ram->MGWsmallest,destaddr) != 0) ? ram->MGWsmallest : ram->MGWsmallestB;
                if ( changeaddr[0] == 0 )
                {
                    printf("Need to create more deposit addresses, need to have at least 2 available\n");
                    exit(1);
                }
                if ( strcmp(cointx->outputs[0].coinaddr,changeaddr) != 0 )
                {
                    strcpy(cointx->outputs[cointx->numoutputs].coinaddr,changeaddr);
                    cointx->outputs[cointx->numoutputs].value = cointx->change;
                    cointx->numoutputs++;
                } else cointx->outputs[0].value += cointx->change;
            }
            if ( (rawparams= _createrawtxid_json_params(ram->name,ram->serverport,ram->userpass,cointx)) != 0 )
            {
                //fprintf(stderr,"len.%ld rawparams.(%s)\n",strlen(rawparams),rawparams);
                _stripwhite(rawparams,0);
                if (  ram->S.gatewayid >= 0 )
                    rettx = createrawtransaction(ram->name,ram->serverport,ram->userpass,rawparams,cointx,opreturn_output,redeemtxid);
                free(rawparams);
            } else fprintf(stderr,"error creating rawparams\n");
        } else fprintf(stderr,"error calculating rawinputs.%.8f or outputs.%.8f | txfee %.8f\n",dstr(sum),dstr(cointx->amount),dstr(ram->txfee));
    } else fprintf(stderr,"not enough %s balance %.8f for withdraw %.8f txfee %.8f\n",ram->name,dstr(ram->S.MGWbalance),dstr(cointx->amount),dstr(ram->txfee));
    return(rettx);
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
                _complete_assettxid(ram,tp);
            }
            else
            {
                if ( _is_limbo_redeem(ram,tp->redeemtxid) != 0 )
                {
                    printf("autocomplete: limbo %llu cointxid.%p\n",(long long)tp->redeemtxid,tp->cointxid);
                    _complete_assettxid(ram,tp);
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
                                        _complete_assettxid(ram,tp);
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
                    _complete_assettxid(ram,tp);
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

char *_ram_loadfp(FILE *fp)
{
    long len;
    char *jsonstr;
    fseek(fp,0,SEEK_END);
    len = ftell(fp);
    jsonstr = calloc(1,len);
    rewind(fp);
    fread(jsonstr,1,len,fp);
    return(jsonstr);
}

uint64_t add_pendingxfer(int32_t removeflag,uint64_t txid)
{
    static int numpending;
    static uint64_t *pendingxfers;
    int32_t nonz,i = 0;
    uint64_t pendingtxid = 0;
    nonz = 0;
    if ( numpending > 0 )
    {
        for (i=0; i<numpending; i++)
        {
            if ( removeflag == 0 )
            {
                if ( pendingxfers[i] == 0 )
                {
                    pendingxfers[i] = txid;
                    break;
                } else nonz++;
            }
            else if ( pendingxfers[i] == txid )
            {
                printf("PENDING.(%llu) removed\n",(long long)txid);
                pendingxfers[i] = 0;
                return(0);
            }
        }
    }
    if ( i == numpending && txid != 0 && removeflag == 0 )
    {
        pendingxfers = realloc(pendingxfers,sizeof(*pendingxfers) * (numpending+1));
        pendingxfers[numpending++] = txid;
        printf("(%d) PENDING.(%llu) added\n",numpending,(long long)txid);
    }
    if ( numpending > 0 )
    {
        for (i=0; i<numpending; i++)
        {
            if ( pendingtxid == 0 && pendingxfers[i] != 0 )
            {
                pendingtxid = pendingxfers[i];
                break;
            }
        }
    }
    return(pendingtxid);
}

uint64_t MGWtransfer_asset(cJSON **transferjsonp,int32_t forceflag,uint64_t nxt64bits,char *depositors_pubkey,struct NXT_asset *ap,uint64_t value,char *coinaddr,char *txidstr,struct address_entry *entry,uint32_t *buyNXTp,char *srvNXTADDR,char *srvNXTACCTSECRET,int32_t deadline)
{
    double get_current_rate(char *base,char *rel);
    char buf[MAX_JSON_FIELD],numstr[64],assetidstr[64],rsacct[64],NXTaddr[64],comment[MAX_JSON_FIELD],*errjsontxt,*str;
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
        for (iter=(value==0); iter<2; iter++)
        {
            errjsontxt = 0;
            str = cJSON_Print(pair);
            _stripwhite(str,' ');
            expand_nxt64bits(assetidstr,ap->assetbits);
            depositid = issue_transferAsset(&errjsontxt,0,srvNXTACCTSECRET,NXTaddr,(iter == 0) ? assetidstr : NXT_ASSETIDSTR,(iter == 0) ? (value/ap->mult) : buyNXT*SATOSHIDEN,MIN_NQTFEE,deadline,str,depositors_pubkey);
            free(str);
            if ( depositid != 0 && errjsontxt == 0 )
            {
                printf("%s worked.%llu\n",(iter == 0) ? "deposit" : "convert",(long long)depositid);
                if ( iter == 1 )
                    *buyNXTp = buyNXT = 0;
                flag++;
                add_pendingxfer(0,depositid);
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

uint32_t _update_ramMGW(uint32_t *firsttimep,struct ramchain_info *ram,uint32_t mostrecent)
{
    FILE *fp;
    cJSON *redemptions=0,*transfers,*array,*json;
    char fname[512],cmd[1024],txid[512],*jsonstr,*txidjsonstr;
    struct NXT_asset *ap;
    uint32_t i,j,n,height,iter,timestamp,oldest;
    while ( (ap= ram->ap) == 0 )
        portable_sleep(1);
    if ( ram->MGWbits == 0 )
    {
        printf("no MGWbits for %s\n",ram->name);
        return(0);
    }
    i = _get_NXTheight(&oldest);
    ram->S.NXT_ECblock = _get_NXT_ECblock(&ram->S.NXT_ECheight);
    //printf("NXTheight.%d ECblock.%d mostrecent.%d\n",i,ram->S.NXT_ECheight,mostrecent);
    if ( firsttimep != 0 )
        *firsttimep = oldest;
    if ( i != ram->S.NXT_RTblocknum )
    {
        ram->S.NXT_RTblocknum = i;
        printf("NEW NXTblocknum.%d when mostrecent.%d\n",i,mostrecent);
    }
    if ( mostrecent > 0 )
    {
        //printf("mostrecent %d <= %d (ram->S.NXT_RTblocknum %d - %d ram->min_NXTconfirms)\n", mostrecent,(ram->S.NXT_RTblocknum - ram->min_NXTconfirms),ram->S.NXT_RTblocknum,ram->min_NXTconfirms);
        while ( mostrecent <= (ram->S.NXT_RTblocknum - ram->min_NXTconfirms) )
        {
            sprintf(cmd,"requestType=getBlock&height=%u&includeTransactions=true",mostrecent);
            //printf("send cmd.(%s)\n",cmd);
            if ( (jsonstr= _issue_NXTPOST(cmd)) != 0 )
            {
                // printf("getBlock.%d (%s)\n",mostrecent,jsonstr);
                if ( (json= cJSON_Parse(jsonstr)) != 0 )
                {
                    timestamp = (uint32_t)get_cJSON_int(json,"timestamp");
                    if ( timestamp != 0 && timestamp > ram->NXTtimestamp )
                    {
                        if ( ram->firsttime == 0 )
                            ram->firsttime = timestamp;
                        if ( ram->S.enable_deposits == 0 && timestamp > (ram->firsttime + (ram->DEPOSIT_XFER_DURATION+1)*60) )
                        {
                            ram->S.enable_deposits = 1;
                            printf("1st.%d ram->NXTtimestamp %d -> %d: enable_deposits.%d | %d > %d\n",ram->firsttime,ram->NXTtimestamp,timestamp,ram->S.enable_deposits,(timestamp - ram->firsttime),(ram->DEPOSIT_XFER_DURATION+1)*60);
                        }
                        ram->NXTtimestamp = timestamp;
                    }
                    if ( (array= cJSON_GetObjectItem(json,"transactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
                    {
                        for (i=0; i<n; i++)
                            _process_NXTtransaction(1,ram,cJSON_GetArrayItem(array,i));
                    }
                    free_json(json);
                } else printf("error parsing.(%s)\n",jsonstr);
                free(jsonstr);
            } else printf("error sending.(%s)\n",cmd);
            mostrecent++;
        }
        if ( ram->min_NXTconfirms == 0 )
        {
            sprintf(cmd,"requestType=getUnconfirmedTransactions");
            if ( (jsonstr= _issue_NXTPOST(cmd)) != 0 )
            {
                //printf("getUnconfirmedTransactions.%d (%s)\n",mostrecent,jsonstr);
                if ( (json= cJSON_Parse(jsonstr)) != 0 )
                {
                    if ( (array= cJSON_GetObjectItem(json,"unconfirmedTransactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
                    {
                        for (i=0; i<n; i++)
                            _process_NXTtransaction(0,ram,cJSON_GetArrayItem(array,i));
                    }
                    free_json(json);
                }
                free(jsonstr);
            }
        }
    }
    else
    {
        for (j=0; j<ram->numspecials; j++)
        {
            fp = 0;
            sprintf(fname,"%s/ramchains/NXT.%s",MGWROOT,ram->special_NXTaddrs[j]);
            printf("(%s) init NXT special.%d of %d (%s) [%s]\n",ram->name,j,ram->numspecials,ram->special_NXTaddrs[j],fname);
            timestamp = 0;
            for (iter=1; iter<2; iter++)
            {
                if ( iter == 0 )
                {
                    if ( (fp= fopen(os_compatible_path(fname),"rb")) == 0 )
                        continue;
                    fread(&timestamp,1,sizeof(timestamp),fp);
                    jsonstr = _ram_loadfp(fp);
                    fclose(fp);
                }
                else
                {
                    sprintf(cmd,"requestType=getAccountTransactions&account=%s&timestamp=%u",ram->special_NXTaddrs[j],timestamp);
                    jsonstr = _issue_NXTPOST(cmd);
                    if ( fp == 0 && (fp= fopen(os_compatible_path(fname),"wb")) != 0 )
                    {
                        fwrite(&oldest,1,sizeof(oldest),fp);
                        fwrite(jsonstr,1,strlen(jsonstr)+1,fp);
                        fclose(fp);
                    }
                }
                if ( jsonstr != 0 )
                {
                    //printf("special.%d (%s) (%s)\n",j,ram->special_NXTaddrs[j],jsonstr);
                    if ( (redemptions= cJSON_Parse(jsonstr)) != 0 )
                    {
                        if ( (array= cJSON_GetObjectItem(redemptions,"transactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
                        {
                            for (i=0; i<n; i++)
                            {
                                //fprintf(stderr,".");
                                if ( (height= _process_NXTtransaction(1,ram,cJSON_GetArrayItem(array,i))) != 0 && height > mostrecent )
                                    mostrecent = height;
                            }
                        }
                        free_json(redemptions);
                    }
                    free(jsonstr);
                }
            }
        }
        sprintf(fname,"%s/ramchains/NXTasset.%llu",MGWROOT,(long long)ap->assetbits);
        fp = 0;
        if ( 1 || (fp= fopen(os_compatible_path(fname),"rb")) == 0 )
        {
            sprintf(cmd,"requestType=getAssetTransfers&asset=%llu",(long long)ap->assetbits);
            jsonstr = _issue_NXTPOST(cmd);
        } else jsonstr = _ram_loadfp(fp), fclose(fp);
        if ( jsonstr != 0 )
        {
            if ( fp == 0 && (fp= fopen(os_compatible_path(fname),"wb")) != 0 )
            {
                fwrite(jsonstr,1,strlen(jsonstr)+1,fp);
                fclose(fp);
            }
            if ( (transfers = cJSON_Parse(jsonstr)) != 0 )
            {
                if ( (array= cJSON_GetObjectItem(transfers,"transfers")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
                {
                    for (i=0; i<n; i++)
                    {
                        //fprintf(stderr,"t");
                        if ( (i % 10) == 9 )
                            fprintf(stderr,"%.1f%% ",100. * (double)i / n);
                        copy_cJSON(txid,cJSON_GetObjectItem(cJSON_GetArrayItem(array,i),"assetTransfer"));
                        if ( txid[0] != 0 && (txidjsonstr= _issue_getTransaction(txid)) != 0 )
                        {
                            if ( (json= cJSON_Parse(txidjsonstr)) != 0 )
                            {
                                if ( (height= _process_NXTtransaction(1,ram,json)) != 0 && height > mostrecent )
                                    mostrecent = height;
                                free_json(json);
                            }
                            free(txidjsonstr);
                        }
                    }
                }
                free_json(transfers);
            }
            free(jsonstr);
        }
    }
    ram->S.circulation = _calc_circulation(ram->min_NXTconfirms,ram->ap,ram);
    if ( ram->S.is_realtime != 0 )
        ram->S.orphans = _find_pending_transfers(&ram->S.MGWpendingredeems,ram);
    //printf("return mostrecent.%d\n",mostrecent);
    return(mostrecent);
}

uint32_t ram_update_disp(struct ramchain_info *ram)
{
    //int32_t pingall(char *coinstr,char *srvNXTACCTSECRET);
    if ( ram_update_RTblock(ram) > ram->lastdisp )
    {
        ram->blocks.blocknum = ram->blocks.contiguous = ram_setcontiguous(&ram->blocks);
        ram_disp_status(ram);
        ram->lastdisp = ram_update_RTblock(ram);
        //pingall(ram->name,ram->srvNXTACCTSECRET);
        return(ram->lastdisp);
    }
    return(0);
}

uint32_t ram_process_blocks(struct ramchain_info *ram,struct mappedblocks *blocks,struct mappedblocks *prev,double timebudget)
{
    HUFF **hpptr,*hp = 0;
    char formatstr[16];
    double estimated,startmilli = milliseconds();
    int32_t newflag,processed = 0;
    if ( ram->remotemode != 0 )
    {
        if ( blocks->format == 'B' )
        {
        }
        return(processed);
    }
    ram_setformatstr(formatstr,blocks->format);
    //printf("%s shift.%d %-5s.%d %.1f min left | [%d < %d]? %f %f timebudget %f\n",formatstr,blocks->shift,ram->name,blocks->blocknum,estimated,(blocks->blocknum >> blocks->shift),(prev->blocknum >> blocks->shift),milliseconds(),(startmilli + timebudget),timebudget);
    while ( (blocks->blocknum >> blocks->shift) < (prev->blocknum >> blocks->shift) && milliseconds() < (startmilli + timebudget) )
    {
        //printf("inside (%d) block.%d\n",blocks->format,blocks->blocknum);
        newflag = (ram->blocks.hps[blocks->blocknum] == 0);
        ram_create_block(1,ram,blocks,prev,blocks->blocknum), processed++;
        if ( (hpptr= ram_get_hpptr(blocks,blocks->blocknum)) != 0 && (hp= *hpptr) != 0 )
        {
            if ( blocks->format == 'B' && newflag != 0 )
            {
                if ( ram_rawblock_update(2,ram,hp,blocks->blocknum) < 0 )
                {
                    printf("FATAL: error updating block.%d %c\n",blocks->blocknum,blocks->format);
                    while ( 1 ) portable_sleep(1);
                }
                if ( ram->permfp != 0 )
                {
                    ram_conv_permind(ram->tmphp2,ram,hp,blocks->blocknum);
                    fflush(ram->permfp);
                }
            }
            //else printf("hpptr.%p hp.%p newflag.%d\n",hpptr,hp,newflag);
        } //else printf("ram_process_blocks: hpptr.%p hp.%p\n",hpptr,hp);
        blocks->processed += (1 << blocks->shift);
        blocks->blocknum += (1 << blocks->shift);
        estimated = estimate_completion(startmilli,blocks->processed,(int32_t)ram->S.RTblocknum-blocks->blocknum) / 60000.;
        //break;
    }
    //printf("(%d >> %d) < (%d >> %d)\n",blocks->blocknum,blocks->shift,prev->blocknum,blocks->shift);
    return(processed);
}

void *ram_process_blocks_loop(void *_blocks)
{
    struct mappedblocks *blocks = _blocks;
    printf("start _process_mappedblocks.%s format.%d\n",blocks->ram->name,blocks->format);
    while ( 1 )
    {
        ram_update_RTblock(blocks->ram);
        if ( ram_process_blocks(blocks->ram,blocks,blocks->prevblocks,1000.) == 0 )
            portable_sleep(sqrt(1 << blocks->shift));
    }
}

void *ram_process_ramchain(void *_ram)
{
    struct ramchain_info *ram = _ram;
    int32_t pass;
    ram->startmilli = milliseconds();
    for (pass=1; pass<=4; pass++)
    {
        if ( portable_thread_create((void *)ram_process_blocks_loop,ram->mappedblocks[pass]) == 0 )
            printf("ERROR _process_ramchain.%s\n",ram->name);
    }
    while ( 1 )
    {
        ram_update_disp(ram);
        portable_sleep(1);
    }
    return(0);
}

void activate_ramchain(struct ramchain_info *ram,char *name)
{
    Ramchains[Numramchains++] = ram;
    if ( Debuglevel > 0 )
        printf("ram.%p Add ramchain.(%s) (%s) Num.%d\n",ram,ram->name,name,Numramchains);
}

void *process_ramchains(void *_argcoinstr)
{
    extern int32_t MULTITHREADS;
    //void ensure_SuperNET_dirs(char *backupdir);
    char *argcoinstr = (_argcoinstr != 0) ? ((char **)_argcoinstr)[0] : 0;
    int32_t iter,gatewayid,modval,numinterleaves;
    double startmilli;
    struct ramchain_info *ram;
    int32_t i,pass,processed = 0;
    //ensure_SuperNET_dirs("ramchains");
    startmilli = milliseconds();
    if ( _argcoinstr != 0 && ((long *)_argcoinstr)[1] != 0 && ((long *)_argcoinstr)[2] != 0 )
    {
        modval = (int32_t)((long *)_argcoinstr)[1];
        numinterleaves = (int32_t)((long *)_argcoinstr)[2];
        printf("modval.%d numinterleaves.%d\n",modval,numinterleaves);
    } else modval = 0, numinterleaves = 1;
    for (iter=0; iter<3; iter++)
    {
        for (i=0; i<Numramchains; i++)
        {
            if ( argcoinstr == 0 || strcmp(argcoinstr,Ramchains[i]->name) == 0 )
            {
                if ( iter > 1 )
                {
                    Ramchains[i]->S.NXTblocknum = _update_ramMGW(0,Ramchains[i],0);
                    if ( Ramchains[i]->S.NXTblocknum > 1000 )
                        Ramchains[i]->S.NXTblocknum -= 1000;
                    else Ramchains[i]->S.NXTblocknum = 0;
                    printf("i.%d of %d: NXTblock.%d (%s) 1sttime %d\n",i,Numramchains,Ramchains[i]->S.NXTblocknum,Ramchains[i]->name,Ramchains[i]->firsttime);
                }
                else if ( iter == 1 )
                {
                    ram_init_ramchain(Ramchains[i]);
                    Ramchains[i]->startmilli = milliseconds();
                }
                else if ( i != 0 )
                    Ramchains[i]->firsttime = Ramchains[0]->firsttime;
                else _get_NXTheight(&Ramchains[i]->firsttime);
            }
            printf("took %.1f seconds to init %s for %d coins\n",(milliseconds() - startmilli)/1000.,iter==0?"NXTheight":(iter==1)?"ramchains":"MGW",Numramchains);
        }
    }
    MGW_initdone = 1;
    while ( processed >= 0 )
    {
        processed = 0;
        for (i=0; i<Numramchains; i++)
        {
            ram = Ramchains[i];
            if ( argcoinstr == 0 || strcmp(argcoinstr,ram->name) == 0 )
            {
                if ( MULTITHREADS != 0 )
                {
                    printf("%d of %d: (%s) argcoinstr.%s\n",i,Numramchains,ram->name,argcoinstr!=0?argcoinstr:"ALL");
                    printf("call process_ramchain.(%s)\n",ram->name);
                    if ( portable_thread_create((void *)ram_process_ramchain,ram) == 0 )
                        printf("ERROR _process_ramchain.%s\n",ram->name);
                    processed--;
                }
                else //if ( (ram->S.NXTblocknum+ram->min_NXTconfirms) < _get_NXTheight() || (ram->mappedblocks[1]->blocknum+ram->min_confirms) < _get_RTheight(&ram->lastgetinfo,ram->name,ram->serverport,ram->userpass,ram->S.RTblocknum) )
                {
                    //if ( strcmp(ram->name,"BTC") != 0 )//ram->S.is_realtime != 0 )
                    {
                        ram->S.NXTblocknum = _update_ramMGW(0,ram,ram->S.NXTblocknum);
                        if ( (ram->S.MGWpendingredeems + ram->S.MGWpendingdeposits) != 0 )
                            printf("\n");
                        ram->S.NXT_is_realtime = (ram->S.NXTblocknum >= (ram->S.NXT_RTblocknum - ram->min_NXTconfirms));
                    } //else ram->S.NXT_is_realtime = 0;
                    ram_update_RTblock(ram);
                    for (pass=1; pass<=4; pass++)
                    {
                        processed += ram_process_blocks(ram,ram->mappedblocks[pass],ram->mappedblocks[pass-1],60000.*pass*pass);
                        //if ( (ram->mappedblocks[pass]->blocknum >> ram->mappedblocks[pass]->shift) < (ram->mappedblocks[pass-1]->blocknum >> ram->mappedblocks[pass]->shift) )
                        //    break;
                    }
                    if ( ram_update_disp(ram) != 0 || 1 )
                    {
                        static char dispbuf[1000],lastdisp[1000];
                        ram->S.MGWunspent = ram_calc_MGWunspent(&ram->S.MGWpendingdeposits,ram);
                        ram->S.MGWbalance = ram->S.MGWunspent - ram->S.circulation - ram->S.MGWpendingredeems - ram->S.MGWpendingdeposits;
                        ram_set_MGWdispbuf(dispbuf,ram,-1);
                        if ( strcmp(dispbuf,lastdisp) != 0 )
                        {
                            strcpy(lastdisp,dispbuf);
                            ram_set_MGWpingstr(ram->MGWpingstr,ram,-1);
                            for (gatewayid=0; gatewayid<ram->numgateways; gatewayid++)
                            {
                                ram_set_MGWdispbuf(dispbuf,ram,gatewayid);
                                printf("G%d:%s",gatewayid,dispbuf);
                            }
                            if ( ram->pendingticks != 0 )
                            {
                                /*int32_t j;
                                 struct NXT_assettxid *tp;
                                 for (j=0; j<ram->numpendingsends; j++)
                                 {
                                 if ( (tp= ram->pendingsends[j]) != 0 )
                                 printf("(%llu %x %x %x) ",(long long)tp->redeemtxid,_extract_batchcrc(tp,0),_extract_batchcrc(tp,1),_extract_batchcrc(tp,2));
                                 }*/
                                printf("pendingticks.%d",ram->pendingticks);
                            }
                            putchar('\n');
                        }
                    }
                }
            }
        }
        for (i=0; i<Numramchains; i++)
        {
            ram = Ramchains[i];
            if ( argcoinstr == 0 || strcmp(argcoinstr,ram->name) == 0 )
                ram_update_disp(ram);
        }
        /*if ( processed == 0 )
        {
            void poll_nanomsg();
            poll_nanomsg();
            portable_sleep(1);
        }*/
        MGW_initdone++;
    }
    printf("process_ramchains: finished launching\n");
    while ( 1 )
        portable_sleep(60);
}

int32_t set_bridge_dispbuf(char *dispbuf,char *coinstr)
{
    int32_t gatewayid;
    struct ramchain_info *ram;
    dispbuf[0] = 0;
    if ( (ram= get_ramchain_info(coinstr)) != 0 )
    {
        for (gatewayid=0; gatewayid<NUM_GATEWAYS; gatewayid++)
        {
            ram_set_MGWdispbuf(dispbuf,ram,gatewayid);
            sprintf(dispbuf+strlen(dispbuf),"G%d:%s",gatewayid,dispbuf);
        }
        printf("set_bridge_dispbuf.(%s)\n",dispbuf);
        return((int32_t)strlen(dispbuf));
    }
    return(0);
}

void update_coinacct_addresses(uint64_t nxt64bits,cJSON *json,char *txid)
{
    printf("update_coinacct_addresses\n");
   /* struct coin_info *cp,*refcp = get_coin_info("BTCD");
    int32_t i,M,N=3;
    struct multisig_addr *msig;
    char coinaddr[512],NXTaddr[64],NXTpubkey[128],*retstr;
    cJSON *coinjson;
    struct contact_info *contacts[4],_contacts[4];
    expand_nxt64bits(NXTaddr,nxt64bits);
    memset(contacts,0,sizeof(contacts));
    M = (N - 1);
    if ( Global_mp->gatewayid < 0 || refcp == 0 )
        return;
    for (i=0; i<3; i++)
    {
        contacts[i] = &_contacts[i];
        memset(contacts[i],0,sizeof(*contacts[i]));
        contacts[i]->nxt64bits = calc_nxt64bits(Server_NXTaddrs[i]);
    }
    if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
        printf("update_coinacct_addresses.(%s)\n",NXTaddr);
    for (i=0; i<Numcoins; i++)
    {
        cp = Coin_daemons[i];
        if ( (cp= Coin_daemons[i]) != 0 && is_active_coin(cp->name) >= 0 )
        {
            coinjson = cJSON_GetObjectItem(json,cp->name);
            if ( coinjson == 0 )
                continue;
            copy_cJSON(coinaddr,coinjson);
            if ( (msig= find_NXT_msig(0,NXTaddr,cp->name,contacts,N)) == 0 )
            {
                set_NXTpubkey(NXTpubkey,NXTaddr);
                retstr = genmultisig(refcp->srvNXTADDR,refcp->srvNXTACCTSECRET,0,cp->name,NXTaddr,M,N,contacts,N,NXTpubkey,0,0);
                if ( retstr != 0 )
                {
                    if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
                        printf("UPDATE_COINACCT_ADDRESSES (%s) -> (%s)\n",cp->name,retstr);
                    free(retstr);
                }
            }
            else free(msig);
        }
    }*/
}


