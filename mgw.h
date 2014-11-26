//
//  mgw.h
//
//  Created by jl777 2014, refactored MGW
//  Copyright (c) 2014 jl777. MIT License.
//

#ifndef mgw_h
#define mgw_h

#define DEPOSIT_XFER_DURATION 10

#define GET_COINDEPOSIT_ADDRESS 'g'
#define BIND_DEPOSIT_ADDRESS 'b'
#define DEPOSIT_CONFIRMED 'd'
#define MONEY_SENT 'm'


int32_t is_limbo_redeem(char *coinstr,char *txid)
{
    uint64_t nxt64bits;
    int32_t j,skipflag = 0;
    struct coin_info *cp;
    if ( (cp= get_coin_info(coinstr)) != 0 && cp->limboarray != 0 )
    {
        nxt64bits = calc_nxt64bits(txid);
        for (j=0; cp->limboarray[j]!=0; j++)
            if ( nxt64bits == cp->limboarray[j] )
            {
                printf(">>>>>>>>>>> j.%d found limbotx %llu\n",j,(long long)cp->limboarray[j]);
                skipflag = 1;
                break;
            }
    }
    return(skipflag);
}

struct multisig_addr *find_msigaddr(char *msigaddr)
{
    return((struct multisig_addr *)find_storage(MULTISIG_DATA,msigaddr,0));
}

void update_msig_info(struct multisig_addr *msig)
{
    if ( msig->H.size == 0 )
        msig->H.size = sizeof(*msig) + (msig->n * sizeof(msig->pubkeys[0]));
    update_storage(&SuperNET_dbs[MULTISIG_DATA],msig->multisigaddr,&msig->H);
}

// network aware funcs
void publish_withdraw_info(struct coin_info *cp,struct batch_info *wp)
{
    struct batch_info W;
    int32_t gatewayid;
    wp->W.coinid = conv_coinstr(cp->name);
    if ( wp->W.coinid < 0 )
    {
        printf("unknown coin.(%s)\n",cp->name);
        return;
    }
    wp->W.srcgateway = Global_mp->gatewayid;
    for (gatewayid=0; gatewayid<NUM_GATEWAYS; gatewayid++)
    {
        wp->W.destgateway = gatewayid;
        W = *wp;
        fprintf(stderr,"publish_withdraw_info.%d -> %d coinid.%d %.8f crc %08x\n",Global_mp->gatewayid,gatewayid,wp->W.coinid,dstr(wp->W.amount),W.rawtx.batchcrc);
        if ( gatewayid == Global_mp->gatewayid )
            cp->withdrawinfos[gatewayid] = *wp;
        else if ( server_request(&Global_mp->gensocks[gatewayid],Server_names[gatewayid],&W.W.H,MULTIGATEWAY_VARIANT,MULTIGATEWAY_SYNCWITHDRAW) == sizeof(W) )
        {
            portable_mutex_lock(&cp->consensus_mutex);
            cp->withdrawinfos[gatewayid] = W;
            portable_mutex_unlock(&cp->consensus_mutex);
        }
        fprintf(stderr,"got publish_withdraw_info.%d -> %d coinid.%d %.8f crc %08x\n",Global_mp->gatewayid,gatewayid,wp->W.coinid,dstr(wp->W.amount),cp->withdrawinfos[gatewayid].rawtx.batchcrc);
    }
}

int32_t process_directnet_syncwithdraw(struct batch_info *wp,char *clientip)
{
    int32_t gatewayid;
    struct coin_info *cp;
    if ( (cp= get_coin_info(coinid_str(wp->W.coinid))) == 0 )
        printf("cant find coinid.%d\n",wp->W.coinid);
    else
    {
        gatewayid = (wp->W.srcgateway % NUM_GATEWAYS);
        cp->withdrawinfos[gatewayid] = *wp;
        *wp = cp->withdrawinfos[Global_mp->gatewayid];
        printf("GOT <<<<<<<<<<<< publish_withdraw_info.%d coinid.%d %.8f crc %08x\n",gatewayid,wp->W.coinid,dstr(wp->W.amount),cp->withdrawinfos[gatewayid].rawtx.batchcrc);
    }
    return(sizeof(*wp));
}
// end of network funcs

char *create_moneysent_jsontxt(int32_t coinid,struct batch_info *bp)
{
    cJSON *json,*obj,*array,*item;
    char *jsontxt;
    int32_t i;
    struct withdraw_info *wp = &bp->W;
    struct coin_value *vp;
    json = cJSON_CreateObject();
    obj = cJSON_CreateString(wp->NXTaddr); cJSON_AddItemToObject(json,"NXTaddr",obj);
    obj = cJSON_CreateNumber(coinid); cJSON_AddItemToObject(json,"coinid",obj);
    obj = cJSON_CreateNumber(issue_getTime(0)); cJSON_AddItemToObject(json,"timestamp",obj);
    obj = cJSON_CreateString(coinid_str(coinid)); cJSON_AddItemToObject(json,"coin",obj);
    obj = cJSON_CreateString(wp->redeemtxid); cJSON_AddItemToObject(json,"redeemtxid",obj);
    obj = cJSON_CreateNumber(wp->amount); cJSON_AddItemToObject(json,"value",obj);
    obj = cJSON_CreateString(wp->withdrawaddr); cJSON_AddItemToObject(json,"coinaddr",obj);
    obj = cJSON_CreateString(wp->cointxid); cJSON_AddItemToObject(json,"cointxid",obj);
    if ( bp->rawtx.numinputs > 0 )
    {
        array = cJSON_CreateArray();
        for (i=0; i<bp->rawtx.numinputs; i++)
        {
            item = cJSON_CreateObject();
            vp = bp->rawtx.inputs[i];
            if ( vp->parent != 0 )
                cJSON_AddItemToObject(item,"txin",cJSON_CreateString(vp->parent->txid));
            cJSON_AddItemToObject(item,"vout",cJSON_CreateNumber(vp->parent_vout));
            cJSON_AddItemToArray(array,item);
        }
        cJSON_AddItemToObject(json,"vins",array);
    }
    jsontxt = cJSON_Print(json);
    free_json(json);
    return(jsontxt);
}

char *create_batch_jsontxt(struct coin_info *cp,int *firstitemp)
{
    cJSON *json,*obj,*array = 0;
    char *jsontxt,redeemtxid[128];
    int32_t i,ind;
    struct rawtransaction *rp = &cp->BATCH.rawtx;
    json = cJSON_CreateObject();
    obj = cJSON_CreateNumber(cp->coinid); cJSON_AddItemToObject(json,"coinid",obj);
    obj = cJSON_CreateNumber(issue_getTime(0)); cJSON_AddItemToObject(json,"timestamp",obj);
    obj = cJSON_CreateString(cp->name); cJSON_AddItemToObject(json,"coin",obj);
    obj = cJSON_CreateString(cp->BATCH.W.cointxid); cJSON_AddItemToObject(json,"cointxid",obj);
    obj = cJSON_CreateNumber(cp->BATCH.rawtx.batchcrc); cJSON_AddItemToObject(json,"batchcrc",obj);
    if ( rp->numredeems > 0 )
    {
        ind = *firstitemp;
        for (i=0; i<32; i++)    // 32 * 22 = 768 bytes AM total limit 1000 bytes
        {
            ind = *firstitemp + i;
            if ( ind >= rp->numredeems )
                break;
            if ( array == 0 )
                array = cJSON_CreateArray();
            expand_nxt64bits(redeemtxid,rp->redeems[ind]);
            cJSON_AddItemToArray(array,cJSON_CreateString(redeemtxid));
        }
        *firstitemp = ind + 1;
        if ( array != 0 )
            cJSON_AddItemToObject(json,"redeems",array);
    }
    jsontxt = cJSON_Print(json);
    free_json(json);
    return(jsontxt);
}

/*uint64_t get_asset_mult(char *assetidstr)
{
    int32_t createdflag;
    struct NXT_asset *ap;
    ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
    if ( ap->mult == 0 )
        ap->mult = 1;
    return(ap->mult);
}*/

struct withdraw_info *parse_batch_json(struct withdraw_info *W,cJSON *argjson)
{
    uint64_t tmp;
    int32_t i,n,createdflag,timestamp,coinid;
    char buf[512],assetidstr[64],NXTtxidstr[64],cointxid[MAX_COINTXID_LEN],redeemtxid[64];
    cJSON *array,*item;
    struct NXT_assettxid *tp;
    struct withdraw_info *wp = 0;
    struct coin_info *cp;
    if ( argjson != 0 )
    {
        coinid = (int32_t)get_cJSON_int(argjson,"coinid");
        if ( extract_cJSON_str(buf,sizeof(buf),argjson,"coin") <= 0 ) return(0);
        printf("got coindid.%d and %s\n",coinid,buf);
        if ( strcmp(buf,coinid_str(coinid)) != 0 )
            return(0);
        cp = get_coin_info(buf);
        if ( extract_cJSON_str(cointxid,sizeof(cointxid),argjson,"cointxid") <= 0 ) return(0);
        timestamp = (int32_t)get_cJSON_int(argjson,"timestamp");
        printf("timestamp.%d (%s)\n",timestamp,cointxid);
        strcpy(assetidstr,cp->assetid);
        if ( cp != 0 && strcmp(assetidstr,ILLEGAL_COINASSET) != 0 )
        {
            array = cJSON_GetObjectItem(argjson,"redeems");
            if ( array != 0 && is_cJSON_Array(array) != 0 )
            {
                n = cJSON_GetArraySize(array);
                for (i=0; i<n; i++)
                {
                    item = cJSON_GetArrayItem(array,i);
                    copy_cJSON(redeemtxid,item);
                    if ( redeemtxid[0] != 0 && strcmp("1423932192",redeemtxid) != 0 && strcmp("53387808",redeemtxid) != 0 )
                    {
                        wp = MTadd_hashtable(&createdflag,Global_mp->redeemtxids,redeemtxid);
                        wp->submitted = 1;
                        strcpy(wp->cointxid,cointxid);
                        calc_NXTcointxid(NXTtxidstr,cointxid,-1);
                        fprintf(stderr,"%d of %d: calc_NXTcointxid NXTtxidstr.(%s) ^ redeem.(%s)\n",i,n,NXTtxidstr,redeemtxid);
                        tmp = calc_nxt64bits(NXTtxidstr) ^ calc_nxt64bits(redeemtxid);
                        expand_nxt64bits(NXTtxidstr,tmp);
                        //printf("calling update assettxid_list\n");
                        if ( wp->NXTaddr[0] == 0 )
                        {
                            expand_nxt64bits(wp->NXTaddr,get_sender((uint64_t *)&wp->amount,redeemtxid));
                            wp->amount *= get_asset_mult(calc_nxt64bits(cp->assetid));
                            fprintf(stderr,"GETSENDER.(%s) %.8f\n",wp->NXTaddr,dstr(wp->amount));
                        }
                        if ( wp->NXTaddr[0] == 0 )
                            continue;
                        tp = update_assettxid_list(wp->NXTaddr,NXTISSUERACCT,assetidstr,NXTtxidstr,timestamp,argjson);
                        if ( tp != 0 )
                        {
                            tp->quantity = 0;
                            if ( redeemtxid[0] != 0 )
                                tp->redeemtxid = calc_nxt64bits(redeemtxid);
                            tp->U.price = wp->amount;
                            tp->cointxid = clonestr(cointxid);
                            fprintf(stderr,"%s >>>>>>> MONEY_SENT %.8f - %.8f assetoshis for redeem.(%s) NXT.%s coin.%s\n",assetidstr,dstr(wp->amount),dstr(cp->txfee+cp->NXTfee_equiv),redeemtxid,wp->NXTaddr,tp->cointxid);
                        } else fprintf(stderr,"null tp returned\n");
                    } else fprintf(stderr,"no redeem txid in item.%d of %d\n",i,n);
                }
            }
            *W = *wp;
            return(W);
        }
    }
    return(0);
}

struct withdraw_info *parse_moneysent_json(struct withdraw_info *W,cJSON *argjson)
{
    int64_t total;
    int32_t i,j,n,numxps,createdflag,timestamp,vout,coinid = -1;
    char buf[512],assetidstr[512],NXTtxidstr[64],txin[1024];
    cJSON *array,*item;
    struct NXT_assettxid *tp;
    struct coin_txid *cointp;
    struct coin_info *cp;
    struct crosschain_info **xps;
    struct withdraw_info *wp = 0;
    if ( argjson != 0 )
    {
        coinid = (int32_t)get_cJSON_int(argjson,"coinid");
        if ( extract_cJSON_str(buf,sizeof(buf),argjson,"coin") <= 0 ) return(0);
        //printf("got coindid.%d and %s\n",coinid,buf);
        if ( strcmp(buf,coinid_str(coinid)) != 0 )
            return(0);
        if ( extract_cJSON_str(W->redeemtxid,sizeof(W->redeemtxid),argjson,"redeemtxid") <= 0 ) return(0);
        if ( strcmp("1423932192",W->redeemtxid) == 0 || strcmp("53387808",W->redeemtxid) == 0 )
            return(0);
        cp = get_coin_info(buf);
        // printf("got lp.%p\n",lp);
        if ( cp == 0 )
            return(0);
        wp = MTadd_hashtable(&createdflag,Global_mp->redeemtxids,W->redeemtxid);
        if ( extract_cJSON_str(wp->NXTaddr,sizeof(wp->NXTaddr),argjson,"NXTaddr") <= 0 ) return(0);
        if ( extract_cJSON_str(wp->cointxid,sizeof(wp->cointxid),argjson,"cointxid") <= 0 ) return(0);
        if ( extract_cJSON_str(wp->withdrawaddr,sizeof(wp->withdrawaddr),argjson,"coinaddr") <= 0 ) return(0);
        extract_cJSON_str(wp->comment,sizeof(wp->comment),argjson,"comment");
        wp->amount = get_satoshi_obj(argjson,"value");
        timestamp = (int32_t)get_cJSON_int(argjson,"timestamp");
        wp->submitted = 1;
        cointp = MTadd_hashtable(&createdflag,Global_mp->coin_txids,wp->cointxid);
        if ( wp->redeemtxid[0] != 0 )
            cointp->redeemtxid = calc_nxt64bits(wp->redeemtxid);
        array = cJSON_GetObjectItem(argjson,"vins");
        if ( array != 0 && is_cJSON_Array(array) != 0 )
        {
            /*xps = get_confirmed_deposits(&total,&numxps,coinid);
            n = cJSON_GetArraySize(array);
            printf("cointxid.(%s) xps.%p num.%d n.%d total %.8f\n",wp->cointxid,xps,numxps,n,dstr(total));
            for (i=0; i<n; i++)
            {
                item = cJSON_GetArrayItem(array,i);
                if ( extract_cJSON_str(txin,sizeof(txin),item,"txin") > 0 )
                {
                    vout = (int32_t)get_cJSON_int(item,"vout");
                    printf("txin.(%s).v%d\n",txin,vout);
                    if ( xps != 0 )
                    {
                        for (j=0; j<numxps; j++)
                        {
                            if ( xp_vout(xps[j]) == vout && xp_txid(xps[j]) != 0 && strcmp(xp_txid(xps[j]),txin) == 0 )
                                break;
                        }
                        if ( j < numxps )
                        {
                            if ( xps[j]->parent != 0 )
                                xps[j]->parent->pendingspend = xps[j]->parent->parent; // not right, but just need non-zero
                            printf("%d of %d: PENDING.(%s).%d j.%d of %d %p total %.8f\n",i,n,txin,vout,j,numxps,xps[j]->parent != 0?xps[j]->parent->pendingspend:0,dstr(total));
                        }
                    }
                }
            }*/
        }
        calc_NXTcointxid(NXTtxidstr,wp->cointxid,-1);
        printf("got wp.%p NXT.%s %.8f -> %s hashval.%s\n",wp,wp->NXTaddr,dstr(wp->amount),wp->withdrawaddr,NXTtxidstr);
        strcpy(assetidstr,cp->assetid);
        if ( strcmp(assetidstr,ILLEGAL_COINASSET) != 0 )
        {
            tp = update_assettxid_list(wp->NXTaddr,NXTISSUERACCT,assetidstr,NXTtxidstr,timestamp,argjson);
            printf("updated assettxid.%p for %s\n",tp,wp->NXTaddr);
            if ( tp != 0 )
            {
                tp->quantity = 0;
                if ( wp->redeemtxid[0] != 0 )
                    tp->redeemtxid = calc_nxt64bits(wp->redeemtxid);
                tp->U.price = wp->amount;
                tp->cointxid = clonestr(wp->cointxid);
                printf("%s >>>>>>> MONEY_SENT %.8f assetoshis for redeem.(%llu) cointxid.%s\n",assetidstr,dstr(wp->amount),(long long)tp->redeemtxid,tp->cointxid);
            }
        }
    }
    return(wp);
}

void update_money_sent(cJSON *argjson,char *AMtxid,int32_t height)
{
    struct withdraw_info W,*wp;
    if ( argjson != 0 && AMtxid != 0 )
    {
        memset(&W,0,sizeof(W));
        if ( height < NXT_FORKHEIGHT )
            wp = parse_moneysent_json(&W,argjson);
        else wp = parse_batch_json(&W,argjson);
        if ( wp != 0 )
        {
            wp->AMtxidbits = calc_nxt64bits(AMtxid);
            fprintf(stderr,">>>>>>> money sent AM txid.%llu\n",(long long)wp->AMtxidbits);
        } //else printf("error parsing moneysent json.(%s)\n",cJSON_Print(argjson));
    } else fprintf(stderr,"error updating money sent (%p %p)\n",argjson,AMtxid);
}

char *broadcast_moneysentAM(struct coin_info *cp,int32_t height)
{
    cJSON *argjson;
    int32_t i,firstitem = 0;
    char AM[4096],*jsontxt,*AMtxid = 0;
    struct json_AM *ap = (struct json_AM *)AM;
    if ( cp == 0 || Global_mp->gatewayid < 0 )
        return(0);
    //jsontxt = create_moneysent_jsontxt(coinid,wp);
    i = 0;
    while ( firstitem < cp->BATCH.rawtx.numredeems )
    {
        jsontxt = create_batch_jsontxt(cp,&firstitem);
        if ( jsontxt != 0 )
        {
            set_json_AM(ap,GATEWAY_SIG,MONEY_SENT,NXTISSUERACCT,Global_mp->timestamp,jsontxt,1);
            printf("%d BATCH_AM.(%s)\n",i,jsontxt);
            i++;
            AMtxid = submit_AM(0,NXTISSUERACCT,&ap->H,0,cp->srvNXTACCTSECRET);
            if ( AMtxid == 0 )
            {
                printf("Error submitting moneysent for (%s)\n",jsontxt);
                while ( 1 )
                    printf("broadcast_moneysentAM: %s failed. FATAL need to manually mark transaction PAID %s JSON.(%s)\n",coinid_str(cp->coinid),cp->BATCH.W.cointxid,jsontxt), sleep(60);
            }
            else
            {
                argjson = cJSON_Parse(jsontxt);
                if ( argjson != 0 )
                    update_money_sent(argjson,AMtxid,height);
                else printf("parse error (%s)\n",jsontxt);
            }
            free(jsontxt);
        }
        else
        {
            printf("moneysent error creating JSON?\n");
            while ( 1 )
                printf("broadcast_moneysentAM: %s failed. FATAL need to manually mark transaction PAID %s JSON.(%s)\n",coinid_str(cp->coinid),cp->BATCH.W.cointxid,jsontxt), sleep(60);
        }
    }
    return(AMtxid);
}

int32_t sign_and_sendmoney(struct coin_info *cp,int32_t height)
{
    char *submit_withdraw(struct coin_info *cp,struct batch_info *wp,struct batch_info *otherwp);
    char *retstr;
    fprintf(stderr,"achieved consensus and sign! %s\n",cp->BATCH.rawtx.batchsigned);
    if ( (retstr= submit_withdraw(cp,&cp->BATCH,&cp->withdrawinfos[(Global_mp->gatewayid + 1) % NUM_GATEWAYS])) != 0 )
    {
        safecopy(cp->BATCH.W.cointxid,retstr,sizeof(cp->BATCH.W.cointxid));
        broadcast_moneysentAM(cp,height);
        free(retstr);
        backupwallet(cp);
        return(1);
    }
    else printf("sign_and_sendmoney: error sending rawtransaction %s\n",cp->BATCH.rawtx.batchsigned);
    return(MGW_PENDING_WITHDRAW);
}

char *calc_withdraw_addr(char *destaddr,char *NXTaddr,struct coin_info *cp,struct NXT_assettxid *tp,struct NXT_asset *ap)
{
    char pubkey[1024],withdrawaddr[1024];
    int64_t amount,minwithdraw;
    cJSON *obj,*argjson = 0;
   // struct coin_acct *acct;
    destaddr[0] = withdrawaddr[0] = 0;
    if ( tp->comment != 0 && tp->comment[0] != 0 && (argjson= cJSON_Parse(tp->comment)) != 0 )
    {
        obj = cJSON_GetObjectItem(argjson,"withdrawaddr");
        copy_cJSON(withdrawaddr,obj);
    }
    amount = tp->quantity * ap->mult;
    minwithdraw = cp->txfee * 5;//get_min_withdraw(coinid);
    if ( amount <= minwithdraw )
    {
        printf("minimum withdrawal must be more than %.8f %s\n",dstr(minwithdraw),cp->name);
        if ( argjson != 0 )
            free_json(argjson);
        return(0);
    }
    else if ( tp->redeemtxid == 0 )
    {
        printf("no redeem txid %s %s\n",cp->name,cJSON_Print(argjson));
        if ( argjson != 0 )
            free_json(argjson);
        return(0);
    }
    if ( withdrawaddr[0] == 0 )
    {
        printf("no withdraw address for %.8f | ",dstr(amount));
        if ( argjson != 0 )
            free_json(argjson);
        return(0);
    }
    //printf("withdraw addr.(%s) lp.%p\n",withdrawaddr,lp);
    if ( cp != 0 && validate_coinaddr(pubkey,cp,withdrawaddr) < 0 )
    {
        printf("invalid address.(%s) for NXT.%s %.8f validate.%d\n",withdrawaddr,NXTaddr,dstr(amount),validate_coinaddr(pubkey,cp,withdrawaddr));
        if ( argjson != 0 )
            free_json(argjson);
        return(0);
    }
    if ( argjson != 0 )
        free_json(argjson);
    strcpy(destaddr,withdrawaddr);
    return((destaddr[0] != 0) ? destaddr : 0);
}

struct multisig_addr *alloc_multisig_addr(char *coinstr,int32_t m,int32_t n,char *NXTaddr)
{
    struct multisig_addr *msig;
    msig = calloc(1,sizeof(*msig) + n*sizeof(struct pubkey_info));
    msig->n = n;
    msig->created = (uint32_t)time(NULL);
    safecopy(msig->coinstr,coinstr,sizeof(msig->coinstr));
    safecopy(msig->NXTaddr,NXTaddr,sizeof(msig->NXTaddr));
    msig->m = m;
    return(msig);
}

long calc_pubkey_jsontxt(char *jsontxt,struct pubkey_info *ptr,char *postfix)
{
    sprintf(jsontxt,"{\"address\":\"%s\",\"pubkey\":\"%s\",\"srv\":\"%llu\"}%s",ptr->coinaddr,ptr->pubkey,(long long)ptr->nxt64bits,postfix);//\"ipaddr\":\"%s\"ptr->server,
    return(strlen(jsontxt));
}

char *create_multisig_json(struct multisig_addr *msig)
{
    long i,len = 0;
    char jsontxt[65536],pubkeyjsontxt[65536];
    for (i=0; i<msig->n; i++)
        len += calc_pubkey_jsontxt(pubkeyjsontxt,&msig->pubkeys[i],(i<(msig->n - 1)) ? "," : "");
    sprintf(jsontxt,"{\"created\":%u,\"NXTaddr\":\"%s\",\"address\":\"%s\",\"redeemScript\":\"%s\",\"coin\":\"%s\",\"coinid\":\"%d\",\"pubkey\":[%s]}",msig->created,msig->NXTaddr,msig->multisigaddr,msig->redeemScript,msig->coinstr,conv_coinstr(msig->coinstr),pubkeyjsontxt);
    printf("(%s) pubkeys len.%ld msigjsonlen.%ld\n",jsontxt,len,strlen(jsontxt));
    return(clonestr(jsontxt));
}

char *createmultisig_json_params(struct multisig_addr *msig,char *acctparm)
{
    int32_t i;
    char *paramstr = 0;
    cJSON *array,*mobj,*keys,*key;
    keys = cJSON_CreateArray();
    for (i=0; i<msig->n; i++)
    {
        key = cJSON_CreateString(msig->pubkeys[i].pubkey);
        cJSON_AddItemToArray(keys,key);
    }
    mobj = cJSON_CreateNumber(msig->m);
    array = cJSON_CreateArray();
    if ( array != 0 )
    {
        cJSON_AddItemToArray(array,mobj);
        cJSON_AddItemToArray(array,keys);
        if ( acctparm != 0 )
            cJSON_AddItemToArray(array,cJSON_CreateString(acctparm));
        paramstr = cJSON_Print(array);
        free_json(array);
    }
    //printf("createmultisig_json_params.%s\n",paramstr);
    return(paramstr);
}

int32_t replace_msig_json(int32_t replaceflag,char *refNXTaddr,char *acctcoinaddr,char *pubkey,char *coinstr,char *jsonstr)
{
    cJSON *array,*item,*json;
    int32_t flag,n,i = -1;
    char coin[1024],refaddr[1024],*str;
    flag = 0;
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        array = cJSON_GetObjectItem(json,"coins");
        if ( array != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) >= 0 )
        {
            for (i=0; i<n; i++)
            {
                item = cJSON_GetArrayItem(array,i);
                copy_cJSON(coin,cJSON_GetObjectItem(item,"coin"));
                copy_cJSON(refaddr,cJSON_GetObjectItem(item,"refNXT"));
                if ( strcmp(coin,coinstr) == 0 && strcmp(refaddr,refNXTaddr) == 0 )
                {
                    if ( replaceflag != 0 )
                    {
                        ensure_jsonitem(item,"pubkey",pubkey);
                        ensure_jsonitem(item,"addr",acctcoinaddr);
                        flag = 1;
                    }
                    else
                    {
                        copy_cJSON(pubkey,cJSON_GetObjectItem(item,"pubkey"));
                        copy_cJSON(acctcoinaddr,cJSON_GetObjectItem(item,"addr"));
                    }
                    break;
                }
            }
            if ( i == n )
                i = -1;
        }
        if ( replaceflag != 0 )
        {
            if ( flag == 0 )
            {
                item = cJSON_CreateObject();
                ensure_jsonitem(item,"coin",coinstr);
                ensure_jsonitem(item,"refNXT",refNXTaddr);
                ensure_jsonitem(item,"addr",acctcoinaddr);
                ensure_jsonitem(item,"pubkey",pubkey);
                if ( array != 0 )
                    cJSON_AddItemToArray(array,item);
                else
                {
                    array = cJSON_CreateArray();
                    cJSON_AddItemToArray(array,item);
                    cJSON_AddItemToObject(json,"coins",array);
                }
            }
            str = cJSON_Print(json);
            stripwhite_ns(str,strlen(str));
            strcpy(jsonstr,str);
            free(str);
        }
        free_json(json);
        return(i);
    }
    if ( replaceflag != 0 && flag == 0 )
    {
        sprintf(jsonstr,"{\"coins\":[{\"coin\":\"%s\",\"refNXT\":\"%s\",\"addr\":\"%s\",\"pubkey\":\"%s\"}]}",coinstr,refNXTaddr,acctcoinaddr,pubkey);
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
            free_json(json);
        else printf("PARSEERROR.(%s)\n",jsonstr);
        return(0);
    }
    return(-1);
}

struct multisig_addr *gen_multisig_addr(int32_t M,int32_t N,struct coin_info *cp,char *refNXTaddr,struct contact_info **contacts)
{
    int32_t i,j,flag = 0;
    char addr[256],acctcoinaddr[128],pubkey[1024];
    struct contact_info *contact;
    cJSON *json,*msigobj,*redeemobj;
    struct multisig_addr *msig;
    char *params,*retstr = 0;
    if ( cp == 0 )
        return(0);
    msig = alloc_multisig_addr(cp->name,M,N,refNXTaddr);
    for (i=0; i<N; i++)
    {
        flag = 0;
        if ( (contact= contacts[i]) != 0 && (contact->jsonstr[0] == '[' || contact->jsonstr[0] == '{') )
        {
            acctcoinaddr[0] = pubkey[0] = 0;
            if ( (j= replace_msig_json(0,refNXTaddr,acctcoinaddr,pubkey,cp->name,contact->jsonstr)) >= 0 )
            {
                strcpy(msig->pubkeys[j].coinaddr,acctcoinaddr);
                strcpy(msig->pubkeys[j].pubkey,pubkey);
                msig->pubkeys[j].nxt64bits = contact->nxt64bits;
            }
            free_json(json);
            if ( flag == 0 )
            {
                free(msig);
                return(0);
            }
        }
    }
    params = createmultisig_json_params(msig,0);
    flag = 0;
    if ( params != 0 )
    {
        printf("multisig params.(%s)\n",params);
        if ( cp->use_addmultisig != 0 )
        {
            retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"addmultisigaddress",params);
            if ( retstr != 0 )
            {
                strcpy(msig->multisigaddr,retstr);
                free(retstr);
                sprintf(addr,"\"%s\"",msig->multisigaddr);
                retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"validateaddress",addr);
                if ( retstr != 0 )
                {
                    json = cJSON_Parse(retstr);
                    if ( json == 0 ) printf("Error before: [%s]\n",cJSON_GetErrorPtr());
                    else
                    {
                        if ( (redeemobj= cJSON_GetObjectItem(json,"hex")) != 0 )
                        {
                            copy_cJSON(msig->redeemScript,redeemobj);
                            flag = 1;
                        } else printf("missing redeemScript in (%s)\n",retstr);
                        free_json(json);
                    }
                    free(retstr);
                }
            } else printf("error creating multisig address\n");
        }
        else
        {
            retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"createmultisig",params);
            if ( retstr != 0 )
            {
                json = cJSON_Parse(retstr);
                if ( json == 0 ) printf("Error before: [%s]\n",cJSON_GetErrorPtr());
                else
                {
                    if ( (msigobj= cJSON_GetObjectItem(json,"address")) != 0 )
                    {
                        if ( (redeemobj= cJSON_GetObjectItem(json,"redeemScript")) != 0 )
                        {
                            copy_cJSON(msig->multisigaddr,msigobj);
                            copy_cJSON(msig->redeemScript,redeemobj);
                            flag = 1;
                        } else printf("missing redeemScript in (%s)\n",retstr);
                    } else printf("multisig missing address in (%s) params.(%s)\n",retstr,params);
                    printf("addmultisig.(%s)\n",retstr);
                    free_json(json);
                }
                free(retstr);
            } else printf("error issuing createmultisig.(%s)\n",params);
        }
        free(params);
    } else printf("error generating msig params\n");
    if ( flag == 0 )
    {
        free(msig);
        return(0);
    }
    return(msig);
}

char *get_bitcoind_pubkey(char *pubkey,struct coin_info *cp,char *coinaddr)
{
    char addr[256],*retstr;
    cJSON *json,*pubobj;
    pubkey[0] = 0;
    if ( cp == 0 )
    {
        printf("get_bitcoind_pubkey null cp?\n");
        return(0);
    }
    sprintf(addr,"\"%s\"",coinaddr);
    retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"validateaddress",addr);
    if ( retstr != 0 )
    {
        printf("got retstr.(%s)\n",retstr);
        json = cJSON_Parse(retstr);
        if ( json != 0 )
        {
            pubobj = cJSON_GetObjectItem(json,"pubkey");
            copy_cJSON(pubkey,pubobj);
            printf("got.%s get_coinaddr_pubkey (%s)\n",cp->name,pubkey);
            free_json(json);
        } else printf("get_coinaddr_pubkey.%s: parse error.(%s)\n",cp->name,retstr);
        free(retstr);
        return(pubkey);
    } else printf("%s error issuing validateaddress\n",cp->name);
    return(0);
}

char *get_acct_coinaddr(char *coinaddr,struct coin_info *cp,char *NXTaddr)
{
    char addr[128];
    char *retstr;
    coinaddr[0] = 0;
    sprintf(addr,"\"%s\"",NXTaddr);
    retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"getaccountaddress",addr);
    if ( retstr != 0 )
    {
        strcpy(coinaddr,retstr);
        free(retstr);
        return(coinaddr);
    }
    return(0);
}

char *genmultisig(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *coinstr,char *refacct,int32_t M,int32_t N,struct contact_info **contacts,int32_t n)
{
    struct coin_info *cp = get_coin_info(coinstr);
    struct multisig_addr *msig,*dbmsig;
    struct contact_info *contact,*refcontact = 0;
    char refNXTaddr[64],hopNXTaddr[64],destNXTaddr[64],pubkey[1024],acctcoinaddr[128],buf[1024],*retstr = 0;
    int32_t i,valid = 0;
    refNXTaddr[0] = 0;
    if ( (refcontact= find_contact(refacct)) != 0 )
    {
        if ( refcontact->nxt64bits != 0 )
            expand_nxt64bits(refNXTaddr,refcontact->nxt64bits);
    }
    if ( refNXTaddr[0] == 0 )
        return(clonestr("\"error\":\"genmultisig couldnt find refcontact\"}"));
    for (i=0; i<n; i++)
    {
        acctcoinaddr[0] = pubkey[0] = 0;
        if ( (contact= contacts[i]) != 0 && contact->nxt64bits != 0 )
        {
            if ( ismynxtbits(contact->nxt64bits) != 0 )
            {
                expand_nxt64bits(destNXTaddr,contact->nxt64bits);
                if ( cp != 0 && get_acct_coinaddr(acctcoinaddr,cp,destNXTaddr) != 0 && get_bitcoind_pubkey(pubkey,cp,acctcoinaddr) != 0 )
                {
                    valid += replace_msig_json(1,refNXTaddr,acctcoinaddr,pubkey,cp->name,contact->jsonstr);
                    update_contact_info(contact);
                }
                else printf("error getting msigaddr for cp.%p ref.(%s) addr.(%s) pubkey.(%s)\n",cp,destNXTaddr,acctcoinaddr,pubkey);
            }
            else
            {
                hopNXTaddr[0] = 0;
                sprintf(buf,"{\"requestType\":\"getmsigpubkey\",\"coin\":\"%s\",\"refNXTaddr\":\"%s\"}",coinstr,refNXTaddr);
                retstr = send_tokenized_cmd(hopNXTaddr,0,NXTaddr,NXTACCTSECRET,buf,destNXTaddr);
            }
        }
    }
    if ( valid == N )
    {
        if ( (msig= gen_multisig_addr(M,N,cp,NXTaddr,contacts)) != 0 )
        {
            if ( (dbmsig= find_msigaddr(msig->multisigaddr)) == 0 )
                update_msig_info(msig);
            else free(dbmsig);
            retstr = create_multisig_json(msig);
            free(msig);
        }
    }
    else
    {
        sprintf(buf,"{\"error\":\"missing msig info\",\"refacct\":\"%s\",\"coin\":\"%s\",\"M\":%d,\"N\":%d,\"valid\":%d}",refacct,coinstr,M,N,valid);
        retstr = clonestr(buf);
    }
    return(retstr);
}

struct multisig_addr *decode_msigjson(char *NXTaddr,cJSON *obj)
{
    int32_t j,n,coinid;
    struct coin_info *cp;
    char nxtstr[512],namestr[64];
    struct multisig_addr *msig = 0;
    cJSON *pobj,*redeemobj,*pubkeysobj,*addrobj,*tmp,*nxtobj,*nameobj;
    coinid = (int)get_cJSON_int(obj,"coinid");
    nameobj = cJSON_GetObjectItem(obj,"coin");
    copy_cJSON(namestr,nameobj);
    if ( namestr[0] != 0 && strcmp(namestr,"BTCD") == 0 )
    {
        cp = get_coin_info(namestr);
        if ( cp == 0 || conv_coinstr(namestr) != coinid )
        {
            printf("name miscompare %s.%d != %s.%d\n",cp!=0?cp->name:"nullcp",conv_coinstr(namestr),namestr,coinid);
            return(0);
        }
        addrobj = cJSON_GetObjectItem(obj,"address");
        redeemobj = cJSON_GetObjectItem(obj,"redeemScript");
        pubkeysobj = cJSON_GetObjectItem(obj,"pubkey");
        nxtobj = cJSON_GetObjectItem(obj,"NXTaddr");
        if ( nxtobj != 0 )
        {
            copy_cJSON(nxtstr,nxtobj);
            if ( NXTaddr != 0 && strcmp(nxtstr,NXTaddr) != 0 )
                printf("WARNING: mismatched NXTaddr.%s vs %s\n",nxtstr,NXTaddr);
        }
        //printf("msig.%p %p %p %p\n",msig,addrobj,redeemobj,pubkeysobj);
        if ( nxtstr[0] != 0 && addrobj != 0 && redeemobj != 0 && pubkeysobj != 0 )
        {
            msig = alloc_multisig_addr(cp->name,NUM_GATEWAYS-1,NUM_GATEWAYS,nxtstr);
            n = cJSON_GetArraySize(pubkeysobj);
            copy_cJSON(msig->redeemScript,redeemobj);
            copy_cJSON(msig->multisigaddr,addrobj);
            for (j=0; j<n; j++)
            {
                pobj = cJSON_GetArrayItem(pubkeysobj,j);
                if ( pobj != 0 )
                {
                    tmp = cJSON_GetObjectItem(pobj,"address"); copy_cJSON(msig->pubkeys[j].coinaddr,tmp);
                    tmp = cJSON_GetObjectItem(pobj,"pubkey"); copy_cJSON(msig->pubkeys[j].pubkey,tmp);
                    msig->pubkeys[j].nxt64bits = get_API_nxt64bits(cJSON_GetObjectItem(pobj,"srv"));
                } else { free(msig); msig = 0; }
            }
        } else { printf("%p %p %p\n",addrobj,redeemobj,pubkeysobj); free(msig); msig = 0; }
        return(msig);
    }
    //printf("decode msig:  error parsing.(%s)\n",cJSON_Print(obj));
    return(0);
}

void process_MGW_message(struct json_AM *ap,char *sender,char *receiver,char *txid)
{
    char NXTaddr[64];
    cJSON *argjson;
    struct multisig_addr *msig;
    expand_nxt64bits(NXTaddr,ap->H.nxt64bits);
    if ( (argjson = parse_json_AM(ap)) != 0 )
    {
        //printf("func.(%c) %s -> %s txid.(%s) JSON.(%s)\n",ap->funcid,sender,receiver,txid,ap->U.jsonstr);
        switch ( ap->funcid )
        {
            case GET_COINDEPOSIT_ADDRESS:
                // start address gen
                //update_coinacct_addresses(ap->H.nxt64bits,argjson,txid,-1);
                break;
            case BIND_DEPOSIT_ADDRESS:
                if ( is_gateway_addr(sender) != 0 && (msig= decode_msigjson(0,argjson)) != 0 )
                {
                    printf("%p func.(%c) %s -> %s txid.(%s) JSON.(%s)\n",msig,ap->funcid,sender,receiver,txid,ap->U.jsonstr);
                    update_msig_info(msig);
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
                //if ( is_gateway_addr(sender) != 0 )
                //    update_money_sent(argjson,txid,height);
                break;
            default: printf("funcid.(%c) not handled\n",ap->funcid);
        }
        if ( argjson != 0 )
            free_json(argjson);
    } else printf("can't JSON parse (%s)\n",ap->U.jsonstr);
}

int32_t add_pendingxfer(int32_t removeflag,uint64_t txid)
{
    static int numpending;
    static uint64_t *pendingxfers;
    int32_t nonz,i = 0;
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
    if ( i == numpending && txid != 0 )
    {
        pendingxfers = realloc(pendingxfers,sizeof(*pendingxfers) * (numpending+1));
        pendingxfers[numpending++] = txid;
    }
    if ( txid == 0 )
        return(nonz);
    return(numpending);
}

int32_t ensure_wp(struct coin_info *cp,uint64_t amount,char *NXTaddr,char *redeemtxid)
{
    int32_t createdflag;
    struct withdraw_info *wp;
    if ( cp != 0 )
    {
        if ( is_limbo_redeem(cp->name,redeemtxid) != 0 )
            return(-1);
        wp = MTadd_hashtable(&createdflag,Global_mp->redeemtxids,redeemtxid);
        if ( createdflag != 0 )
        {
            wp->amount = amount;
            wp->coinid = conv_coinstr(cp->name);//coinid;
            strcpy(wp->NXTaddr,NXTaddr);
            printf("%s ensure redeem.%s %.8f -> NXT.%s\n",cp->name,redeemtxid,dstr(amount),NXTaddr);
        }
    } else if ( cp != 0 ) printf("Unexpected missing replicated_coininfo for coin.%s\n",cp->name);
    return(0);
}

int64_t process_NXTtransaction(char *sender,char *receiver,cJSON *item,char *refNXTaddr,char *assetid)
{
    int32_t conv_coinstr(char *);
    char AMstr[4096],txid[1024],comment[1024],*assetidstr,*commentstr = 0;
    cJSON *senderobj,*attachment,*message,*assetjson,*commentobj,*cointxidobj;
    char cointxid[128];
    unsigned char buf[4096];
    struct NXT_AMhdr *hdr;
    struct NXT_asset *ap = 0;
    struct NXT_assettxid *tp;
    int32_t height,timestamp,coinid,dir = 0;
    int64_t type,subtype,n,assetoshis = 0;
    assetid[0] = 0;
    if ( item != 0 )
    {
        hdr = 0; assetidstr = 0;
        sender[0] = receiver[0] = 0;
        copy_cJSON(txid,cJSON_GetObjectItem(item,"transaction"));
        type = get_cJSON_int(item,"type");
        subtype = get_cJSON_int(item,"subtype");
        timestamp = (int32_t)get_cJSON_int(item,"blockTimestamp");
        height = (int32_t)get_cJSON_int(item,"height");
        senderobj = cJSON_GetObjectItem(item,"sender");
        if ( senderobj == 0 )
            senderobj = cJSON_GetObjectItem(item,"accountId");
        else if ( senderobj == 0 )
            senderobj = cJSON_GetObjectItem(item,"account");
        copy_cJSON(sender,senderobj);
        copy_cJSON(receiver,cJSON_GetObjectItem(item,"recipient"));
        attachment = cJSON_GetObjectItem(item,"attachment");
        if ( attachment != 0 )
        {
            message = cJSON_GetObjectItem(attachment,"message");
            assetjson = cJSON_GetObjectItem(attachment,"asset");
            if ( message != 0 && type == 1 )
            {
                copy_cJSON(AMstr,message);
                //printf("AM message.(%s).%ld\n",AMstr,strlen(AMstr));
                n = strlen(AMstr);
                if ( is_hexstr(AMstr) != 0 )
                {
                    if ( (n&1) != 0 || n > 2000 )
                        printf("warning: odd message len?? %ld\n",(long)n);
                    decode_hex((void *)buf,(int32_t)(n>>1),AMstr);
                    hdr = (struct NXT_AMhdr *)buf;
                    process_MGW_message((void *)hdr,sender,receiver,txid);
                }
            }
            else if ( assetjson != 0 && type == 2 && subtype <= 1 )
            {
                commentobj = cJSON_GetObjectItem(attachment,"comment");
                if ( commentobj == 0 )
                    commentobj = message;
                copy_cJSON(comment,commentobj);
                if ( comment[0] != 0 )
                    commentstr = clonestr(replace_backslashquotes(comment));
                tp = add_NXT_assettxid(&ap,assetid,assetjson,txid,timestamp);
                if ( tp != 0 )
                {
                    if ( tp->comment != 0 )
                        free(tp->comment);
                    tp->comment = commentstr;
                    tp->timestamp = timestamp;
                    if ( type == 2 )
                    {
                        tp->quantity = get_cJSON_int(attachment,"quantityQNT");
                        assetoshis = tp->quantity;
                        switch ( subtype )
                        {
                            case 0:
                                break;
                            case 1:
                                tp->senderbits = calc_nxt64bits(sender);
                                tp->receiverbits = calc_nxt64bits(receiver);
                                if ( ap != 0 )
                                {
                                    coinid = conv_coinstr(ap->name);
                                    commentobj = 0;
                                    if ( ap->mult != 0 )
                                        assetoshis *= ap->mult;
                                    //printf("case1 sender.(%s) receiver.(%s) comment.%p cmp.%d\n",sender,receiver,tp->comment,strcmp(receiver,refNXTaddr)==0);
                                    if ( tp->comment != 0 && (commentobj= cJSON_Parse(tp->comment)) != 0 )
                                    {
                                        cointxidobj = cJSON_GetObjectItem(commentobj,"cointxid");
                                        if ( cointxidobj != 0 )
                                        {
                                            copy_cJSON(cointxid,cointxidobj);
                                            printf("got comment.(%s) cointxidstr.(%s)\n",tp->comment,cointxid);
                                            if ( cointxid[0] != 0 )
                                                tp->cointxid = clonestr(cointxid);
                                        } else cointxid[0] = 0;
                                        free_json(commentobj);
                                    }
                                    if ( coinid >= 0 && is_limbo_redeem(ap->name,txid) == 0 )
                                    {
                                        if ( strcmp(receiver,refNXTaddr) == 0 )
                                        {
                                            printf("%s got comment.(%s) gotredeem.(%s) coinid.%d %.8f\n",ap->name,tp->comment,cointxid,coinid,dstr(tp->quantity * ap->mult));
                                            tp->redeemtxid = calc_nxt64bits(txid);
                                            printf("protocol redeem.(%s)\n",txid);
                                            dir = -1;
                                            if ( tp->comment != 0 )
                                                tp->completed = MGW_PENDING_WITHDRAW;
                                            ensure_wp(get_coin_info(ap->name),tp->quantity * ap->mult,sender,txid);
                                        }
                                        else if ( strcmp(sender,refNXTaddr) == 0 )
                                            dir = 1;
                                    }
                                }
                                break;
                            case 2:
                            case 3: // bids and asks, no indication they are filled at this point, so nothing to do
                                break;
                        }
                    }
                    add_pendingxfer(1,tp->txidbits);
                }
            }
        }
    }
    else printf("unexpected error iterating timestamp.(%d) txid.(%s)\n",timestamp,txid);
    return(assetoshis * dir);
}

/*uint64_t disp_MGW_state(struct NXT_acct *np,struct coin_info *cp,uint64_t ap_mult,int32_t maxtimestamp)
{
    char qtystr[512];
    cJSON *json,*assetjson,*item,*array;
    char *jsonstr;
    uint64_t balance = 0;
    jsonstr = emit_acct_json(np->H.U.NXTaddr,cp->assetid,maxtimestamp,2);
    balance = np->quantity * ap_mult;
    if ( jsonstr != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (assetjson= cJSON_GetObjectItem(json,"assets")) != 0 && is_cJSON_Array(assetjson) != 0 )
            {
                item = cJSON_GetArrayItem(assetjson,0);
                if ( (array= cJSON_GetObjectItem(item,"transactions")) != 0 && cJSON_GetArraySize(array) > 0 )
                {
                    // need to update with batch redeems
                    // printf("txlog.2a %s %.8f (%s)\n",coinid_str(coinid),dstr(balance),jsonstr);
                }
                else
                {
                    item = cJSON_GetObjectItem(item,"qty");
                    copy_cJSON(qtystr,item);
                    //printf("txlog.2: NXT.%s %s (%s)\n",np->H.NXTaddr,coinid_str(coinid),qtystr);
                }
            }
            else printf("txlog.2b %s (%s) %.8f\n",cp->name,jsonstr,dstr(balance));
            free_json(json);
        }
        else printf("txlog.2c %s (%s) %.8f\n",cp->name,jsonstr,dstr(balance));
        free(jsonstr);
    }
    return(balance);
}*/

int32_t add_destaddress(struct rawtransaction *rp,char *destaddr,uint64_t amount)
{
    int32_t i;
    stripwhite(destaddr,strlen(destaddr));
    for (i=0; i<rp->numoutputs; i++)
    {
        //printf("compare.%d of %d: %s vs %s\n",i,rp->numoutputs,rp->destaddrs[i],destaddr);
        if ( strcmp(rp->destaddrs[i],destaddr) == 0 )
            break;
    }
    printf("add[%d] of %d: %.8f -> %s\n",i,rp->numoutputs,dstr(amount),destaddr);
    if ( i == rp->numoutputs )
    {
        if ( rp->numoutputs >= (int)(sizeof(rp->destaddrs)/sizeof(*rp->destaddrs)) )
        {
            printf("overflow %d vs %ld\n",rp->numoutputs,(sizeof(rp->destaddrs)/sizeof(*rp->destaddrs)));
            return(-1);
        }
        printf("create new output.%d %s ",rp->numoutputs,destaddr);
        rp->destaddrs[rp->numoutputs++] = clonestr(destaddr);
    }
    rp->destamounts[i] += amount;
    return(i);
}

int32_t is_the_same(struct NXT_assettxid *tp,struct NXT_assettxid *othertp)
{
    if ( tp != othertp && tp->completed == othertp->completed )
    {
        if ( tp->completed == MGW_PENDING_DEPOSIT && tp->cointxid != 0 && othertp->cointxid != 0 && strcmp(tp->cointxid,othertp->cointxid) == 0 )
            return(1);
        else if ( tp->completed == MGW_PENDING_WITHDRAW && tp->redeemtxid != 0 && othertp->redeemtxid != 0 && tp->redeemtxid == othertp->redeemtxid )
            return(1);
    }
    return(0);
}

struct NXT_assettxid **prune_already_completed(int32_t *nump,struct NXT_acct *np,struct NXT_assettxid **alltxids,int32_t num)
{
    int32_t i,j,k;
    struct NXT_assettxid **txids,*tp;
    txids = calloc(num,sizeof(*txids));
    for (i=j=0; i<num; i++)
    {
        if ( (tp = alltxids[i]) != 0 )
        {
            if ( tp->completed <= 0 )
            {
                if ( j != 0 )
                {
                    for (k=0; k<j; k++)
                        if ( is_the_same(tp,txids[k]) != 0 )
                            break;
                    if ( k == j )
                        txids[j++] = tp;
                }
                else txids[j++] = tp;
            }
        }
    }
    //printf("active.%d\n",j);
    *nump = j;
    return(txids);
}

int32_t is_matched_assettxid(struct NXT_assettxid *tp,struct NXT_assettxid *othertp)
{
    // printf("%p vs %p, %s vs %s\n",tp,othertp,tp->cointxid,othertp->cointxid);
    if ( tp != othertp )
    {
        if ( tp->cointxid != 0 && othertp->cointxid != 0 && strcmp(tp->cointxid,othertp->cointxid) == 0 )
            return(1);
        else if ( tp->redeemtxid != 0 && othertp->redeemtxid != 0 && tp->redeemtxid == othertp->redeemtxid )
            return(1);
    }
    return(0);
}

int32_t has_other_half(struct NXT_assettxid *tp,struct NXT_acct *np,struct NXT_assettxid **alltxids,int32_t num)
{
    int i;
    struct NXT_assettxid *othertp;
    for (i=0; i<num; i++)
    {
        if ( (othertp= alltxids[i]) != tp )
        {
            if ( is_matched_assettxid(tp,othertp) != 0 )
            {
                othertp->completed = 1;
                return(1);
            }
        }
    }
    //printf("cointxid.%s didnt match any of %d\n",tp->cointxid,num);
    return(0);
}

int64_t init_NXT_transactions(char *refNXTaddr,char *refassetid)
{
    char sender[1024],receiver[1024],assetid[1024],cmd[1024],*jsonstr;
    int64_t val,netsatoshis = 0;
    int32_t i,n;
    cJSON *item,*json,*array;
    if ( refNXTaddr[0] == 0 || refassetid[0] == 0 )
    {
        printf("illegal refNXT.(%s) or asset.(%s)\n",refNXTaddr,refassetid);
        return(0);
    }
    sprintf(cmd,"%s=getAccountTransactions&account=%s",_NXTSERVER,refNXTaddr);
    printf("init_NXT_transactions.(%s) for (%s) cmd.(%s)\n",refNXTaddr,refassetid,cmd);
    if ( (jsonstr= issue_NXTPOST(0,cmd)) != 0 )
    {
        //printf("(%s)\n",jsonstr);
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"transactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    fprintf(stderr,"%d/%d ",i,n);
                    item = cJSON_GetArrayItem(array,i);
                    val =  process_NXTtransaction(sender,receiver,item,refNXTaddr,assetid);
                    if ( refassetid != 0 && strcmp(assetid,refassetid) == 0 )
                        netsatoshis += val;
                }
            }
            free_json(json);
        }
        free(jsonstr);
    } else printf("error with init_NXT_transactions.(%s)\n",cmd);
    return(netsatoshis);
}

int32_t ready_to_xferassets()
{
    // if fresh reboot, need to wait the xfer max duration + 1 block before running this
    static uint32_t firsttime,firstNXTblock;
    if ( firsttime == 0 )
        firsttime = (uint32_t)time(NULL);
    if ( time(NULL) < (firsttime + DEPOSIT_XFER_DURATION*60) )
        return(0);
    if ( firstNXTblock == 0 )
        firstNXTblock = get_NXTblock();
    if ( firstNXTblock == 0 || get_NXTblock() < (firstNXTblock + 3) )
        return(0);
    if ( add_pendingxfer(0,0) != 0 )
        return(0);
    return(1);
}

int32_t process_msigdeposits(struct coin_info *cp,struct address_entry *entry,uint64_t nxt64bits,struct NXT_asset *ap,char *msigaddr)
{
    char txidstr[1024],coinaddr[1024],script[4096],comment[4096];
    struct NXT_assettxid *tp;
    uint64_t xfertxid,value;
    int32_t j,nonz = 0;
    for (j=0; j<ap->num; j++)
    {
        tp = ap->txids[j];
        if ( tp->receiverbits == nxt64bits && tp->coinblocknum == entry->blocknum && tp->cointxind == entry->txind && tp->coinv == entry->v )
            break;
    }
    if ( j == ap->num )
    {
        value = get_txindstr(txidstr,coinaddr,script,cp,entry->blocknum,entry->txind,entry->v);
        if ( strcmp(msigaddr,coinaddr) == 0 && txidstr[0] != 0 )
        {
            for (j=0; j<ap->num; j++)
            {
                tp = ap->txids[j];
                if ( tp->receiverbits == nxt64bits && tp->cointxid != 0 && strcmp(tp->cointxid,txidstr) == 0 )
                {
                    printf("set cointxid.(%s) <-> (%u %d %d)\n",txidstr,entry->blocknum,entry->txind,entry->v);
                    tp->cointxind = entry->txind;
                    tp->coinv = entry->v;
                    tp->coinblocknum = entry->blocknum;
                    break;
                }
            }
            if ( j == ap->num )
            {
                if ( ready_to_xferassets() > 0 )
                {
                    sprintf(comment,"{\"coinaddr\":\"%s\",\"cointxid\":\"%s\",\"coinblocknum\":%u,\"cointxind\":%u,\"coinv\":%u}",coinaddr,txidstr,entry->blocknum,entry->txind,entry->v);
                    printf("Need to transfer %.8f %ld assetoshis | %s to %llu for (%s) %s\n",dstr(value),(long)(value/ap->mult),cp->name,(long long)nxt64bits,txidstr,comment);
                    xfertxid = 0;//issue_transferAsset(0,0,cp->srvNXTACCTSECRET,NXTaddr,refassetid,value/ap->mult,MIN_NQTFEE,DEPOSIT_XFER_DURATION,comment);
                    add_pendingxfer(0,xfertxid);
                    nonz++;
                    // get xfer txid, add to list and wait for it to get to blockchain before iterating again
                }
            }
        }
    }
    return(nonz);
}

int32_t process_msigaddr(struct NXT_asset *ap,char *refassetid,char *NXTaddr,struct coin_info *cp,char *msigaddr)
{
    struct address_entry *entries,*entry;
    int32_t i,n,nonz = 0;
    uint64_t nxt64bits;
    if ( ap->mult == 0 )
    {
        printf("ap->mult is ZERO for %s?\n",refassetid);
        return(-1);
    }
    nxt64bits = calc_nxt64bits(NXTaddr);
    if ( (entries= get_address_entries(&n,cp->name,msigaddr)) != 0 )
    {
        init_NXT_transactions(NXTaddr,refassetid);
        for (i=0; i<n; i++)
        {
            entry = &entries[i];
            if ( entry->vinflag == 0 && entry->isinternal == 0 && ap->num > 0 )
                nonz += process_msigdeposits(cp,entry,nxt64bits,ap,msigaddr);
        }
        free(entries);
    }
    return(nonz);
}

uint64_t update_assetacct_actions(uint64_t *pending_withdrawp,struct coin_info *cp,struct NXT_acct *np,struct NXT_asset *ap,int32_t maxtimestamp)
{
    char destaddrs[MAX_MULTISIG_OUTPUTS+MAX_MULTISIG_INPUTS][MAX_COINADDR_LEN],redeemtxid[MAX_COINTXID_LEN];
    uint64_t amounts[MAX_MULTISIG_OUTPUTS+MAX_MULTISIG_INPUTS],redeems[MAX_MULTISIG_OUTPUTS+MAX_MULTISIG_INPUTS];
    uint64_t amount,pending_withdraw,sum,balance = 0;
    int32_t ind,i,j,n,numredeems = 0;
    struct NXT_assettxid **txids;
    struct rawtransaction *rp;
    ind = get_asset_in_acct(np,ap,0);
    np->quantity = pending_withdraw = 0;
    if ( ind >= 0 )
    {
        memset(amounts,0,sizeof(amounts));
        memset(redeems,0,sizeof(redeems));
        memset(destaddrs,0,sizeof(destaddrs));
        balance = 0;//disp_MGW_state(np,cp,ap->mult,maxtimestamp);
        txids = prune_already_completed(&n,np,np->txlists[ind]->txids,np->txlists[ind]->num);
        //printf("gateway.%d update_assetacct_actions: txids[%d]\n",Global_mp->gatewayid,n);
        if ( txids != 0 )
        {
            for (i=0; i<n; i++)
            {
                if ( txids[i] != 0 )
                {
                    if ( txids[i]->timestamp > maxtimestamp )
                        printf("TIMESTAMP ERROR: NXT.%s txid[%d] t%d > max.%d\n",np->H.U.NXTaddr,i,txids[i]->timestamp,maxtimestamp);
                    if ( has_other_half(txids[i],np,np->txlists[ind]->txids,np->txlists[ind]->num) != 0 )
                        txids[i]->completed = 1;
                }
            }
            if ( cp != 0 && Global_mp->gatewayid >= 0 )
            {
                rp = &cp->BATCH.rawtx;
                for (i=0; i<n; i++)
                {
                    if ( txids[i]->completed < 0 )
                    {
                        //printf("NXT.%s %.8f i.%d of %d: completed.%d\n",np->H.NXTaddr,dstr(np->quantity * ap->mult),i,n,txids[i]->completed);
                        if ( txids[i]->completed == MGW_PENDING_DEPOSIT )
                        {
                            /*if ( coinid != BTC_COINID && (np->H.nxt64bits % NUM_GATEWAYS) == Global_mp->gatewayid )
                            {
                                assetoshis = txids[i]->assetoshis;
                                if ( (assetoshis * ap->mult) >= (uint64_t)(5 * lp->NXTfee_equiv) )
                                {
                                    asset_txid = issue_transferAsset(0,Global_mp->curl_handle,Global_mp->NXTACCTSECRET,np->H.NXTaddr,ap->H.assetid,assetoshis,MIN_NQTFEE,20,txids[i]->comment);
                                    txids[i]->completed = 1;
                                    printf("i.%d of %d: completed.%d | deposit for %.8f -> %.8f %llu\n",i,n,txids[i]->completed,dstr(txids[i]->assetoshis) * ap->mult,dstr(assetoshis) * ap->mult,(long long)asset_txid);
                                    printf("HAVE Transferred %.8f assets.%p\n",dstr(assetoshis),txids[i]);
                                } else printf("txid.%llu %llu is less than min deposit of %llu | %llu * mult %llu\n",(long long)txids[i]->txidbits,(long long)(assetoshis * ap->mult),(long long)(5*lp->NXTfee_equiv),(long long)assetoshis,(long long)ap->mult);
                            }*/
                        }
                        else if ( txids[i]->completed == MGW_PENDING_WITHDRAW )
                        {
                            if ( txids[i]->txidbits != 0 && (txids[i]->quantity*ap->mult) >= (uint64_t)(cp->txfee + cp->NXTfee_equiv) )
                            {
                                char redeemstr[64];
                                expand_nxt64bits(redeemstr,txids[i]->redeemtxid);
                                if ( numredeems < MAX_MULTISIG_OUTPUTS && ensure_wp(cp,txids[i]->quantity * ap->mult,np->H.U.NXTaddr,redeemstr) == 0 )
                                {
                                    amount = txids[i]->quantity * ap->mult - (cp->txfee + cp->NXTfee_equiv);
                                    pending_withdraw += amount;
                                    for (j=0; j<numredeems; j++)
                                        if ( redeems[j] == txids[i]->txidbits )
                                            break;
                                    if ( j == numredeems )
                                    {
                                        //printf("calc withdraw_addr numredeems.%d %s\n",numredeems,np->H.NXTaddr);
                                        if ( calc_withdraw_addr(destaddrs[numredeems],np->H.U.NXTaddr,cp,txids[i],ap) != 0 )
                                        {
                                            amounts[numredeems] = amount;
                                            redeems[numredeems++] = txids[i]->txidbits;
                                            printf("R.(%llu %.8f %s) ",(long long)txids[i]->txidbits,dstr(amounts[j]),destaddrs[j]);
                                        } else printf("WARNING: no valid withdraw address for NXT.%s %llu\n",np->H.U.NXTaddr,(long long)txids[i]->redeemtxid);
                                    }
                                    else printf("ERROR: duplicate redeembits.%llu i.%d numredeems.%d j.%d\n",(long long)txids[i]->txidbits,i,numredeems,j);
                                }
                            }
                            else printf("ERROR: missing redeembits.%llu i.%d numredeems.%d or too small %.8f vs min %.8f\n",(long long)txids[i]->txidbits,i,numredeems,dstr(txids[i]->quantity * ap->mult),dstr(cp->txfee + cp->NXTfee_equiv));
                        }
                        else printf("unexpected negative completed state.%d\n",txids[i]->completed);
                    }
                }
                if ( (int64_t)pending_withdraw >= ((5 * cp->NXTfee_equiv) - (numredeems * (cp->txfee + cp->NXTfee_equiv))) )
                {
                    for (sum=j=0; j<numredeems&&rp->numoutputs<(int)(sizeof(rp->destaddrs)/sizeof(*rp->destaddrs))-1; j++)
                    {
                        fprintf(stderr,"[%llu %.8f] ",(long long)redeems[j],dstr(amounts[j]));
                        if ( add_destaddress(rp,destaddrs[j],amounts[j]) < 0 )
                        {
                            printf("error adding %s %.8f | numredeems.%d numoutputs.%d\n",destaddrs[j],dstr(amounts[j]),rp->numredeems,rp->numoutputs);
                            break;
                        }
                        sum += amounts[j];
                        expand_nxt64bits(redeemtxid,redeems[j]);
                        rp->redeems[rp->numredeems++] = redeems[j];
                        if ( rp->numredeems >= (int)(sizeof(rp->redeems)/sizeof(*rp->redeems)) )
                            break;
                    }
                    printf("pending_withdraw %.8f -> sum %.8f numredeems.%d numoutputs.%d\n",dstr(pending_withdraw),dstr(sum),rp->numredeems,rp->numoutputs);
                    pending_withdraw = sum;
                }
                else
                {
                    if ( numredeems > 0 )
                        printf("%.8f is not enough to pay for MGWfees.%s %.8f for %d redeems\n",dstr(pending_withdraw),cp->name,dstr(cp->NXTfee_equiv),numredeems);
                    pending_withdraw = 0;
                }
            }
            free(txids);
        }
    } else printf("couldnt get valid ind for asset\n");
    *pending_withdrawp = pending_withdraw;
    return(balance);
}

void update_asset_actions(struct multisig_addr **msigs,int32_t nummsigs)
{
    char *signedbytes,*cointxid;
    struct NXT_asset *ap;
    struct coin_txid *cointp;
    struct NXT_acct **accts = 0;
    struct coin_info *cp;
    struct batch_info *otherwp;
    //struct replicated_coininfo *lp;
    uint64_t pending_withdraw,pending,unspent,sum,balance;
    int32_t gatewayid,coinid,createdflag,i,j,n,maxtimestamp = 0;
    // printf("update_asset_actions height.%d %d\n",height,mp->timestamps[height]);
    for (i=0; i<Numcoins; i++)
    {
        cp = Daemons[i];
        if ( accts != 0 )
            free(accts), accts = 0;
        //      transfer_assets(coinid);
        if ( (accts= get_assetaccts(&n,cp->assetid,maxtimestamp)) == 0 )
            continue;
        printf("maxtime.%d %s height.%d of %d ",maxtimestamp,cp->name,(int)cp->blockheight,(int)cp->RTblockheight);
        ap = get_NXTasset(&createdflag,Global_mp,cp->assetid);
        //printf("%d: got %d accts.%p ap.%p\n",coinid,n,accts,ap);
        unspent = 0;//update_unspent_funds(msigs,nummsigs,cp);
        if ( cp != 0 )
        {
            cointxid = cp->withdrawinfos[0].W.cointxid;
            if ( cp->BATCH.rawtx.batchcrc == cp->withdrawinfos[0].rawtx.batchcrc && cointxid[0] != 0 )
            {
                printf("crc.%08x/%08x check for %s.%s\n",cp->BATCH.rawtx.batchcrc,cp->withdrawinfos[0].rawtx.batchcrc,coinid_str(coinid),cointxid);
                cointp = MTadd_hashtable(&createdflag,Global_mp->coin_txids,cointxid);
                if ( cointp->numvouts == cp->BATCH.rawtx.numoutputs && cointp->numvins == cp->BATCH.rawtx.numinputs )
                {
                    printf("Found BATCH %s on blockchain.%s\n",cointxid,coinid_str(coinid));
                    //mark_as_sent(cp->BATCH.W.redeems,cp->BATCH.W.numredeems);
                    cp->BATCH.rawtx.batchcrc = 0;
                    memset(cp->BATCH.W.cointxid,0,sizeof(cp->BATCH.W.cointxid));
                    publish_withdraw_info(cp,&cp->BATCH);
                }
                else if ( cointp->numvouts != 0 )
                {
                    printf("FATAL MISMATCHED BLOCKCHAIN.%s vouts %d != %d || vins %d != %d\n",cp->name,cointp->numvouts,cp->BATCH.rawtx.numoutputs,cointp->numvins,cp->BATCH.rawtx.numinputs);
                }
                if ( cp->BATCH.rawtx.batchcrc != 0 )
                    continue;
            }
            clear_BATCH(&cp->BATCH.rawtx);
            cp->BATCH.rawtx.numoutputs = init_batchoutputs(cp,&cp->BATCH.rawtx,cp->txfee);
            pending = sum = 0;
            for (i=j=0; i<n; i++)
            {
                //printf("%s i.%d of %d %p\n",accts[i]->H.NXTaddr,i,n,accts[i]);
                if ( skip_address(accts[i]->H.U.NXTaddr) == 0 )
                {
                    balance = update_assetacct_actions(&pending_withdraw,cp,accts[i],ap,maxtimestamp);
                    //printf("back i.%d of %d %p\n",i,n,accts[i]);
                    sum += balance;
                    pending += pending_withdraw;
                    if ( pending_withdraw != 0 )
                    {
                        if ( j > 0 && (j % 3) == 0 )
                            printf("\n");
                        j++;
                        printf("(%-22s %11.8f of %11.8f) ",accts[i]->H.U.NXTaddr,dstr(pending_withdraw),dstr(accts[i]->quantity));
                    }
                }
            }
            fprintf(stderr,"\n-> %s unspent %.8f (%.8f) vs pending_withdraws %.8f numredeems.%d numoutputs.%d\n",coinid_str(coinid),dstr(unspent),dstr(sum),dstr(pending),cp->BATCH.rawtx.numredeems,cp->BATCH.rawtx.numoutputs);
            if ( pending > 0 )
            {
                signedbytes = calc_batchwithdraw(msigs,nummsigs,cp,&cp->BATCH.rawtx,pending,unspent,accts,n,ap);
                if ( signedbytes != 0 )
                {
                    fprintf(stderr,"got signedbytes.(%s)\ncrc %08x\n",signedbytes,cp->BATCH.rawtx.batchcrc);
                    publish_withdraw_info(cp,&cp->BATCH);
                    for (gatewayid=0; gatewayid<NUM_GATEWAYS; gatewayid++)
                    {
                        otherwp = &cp->withdrawinfos[gatewayid];
                        if ( cp->BATCH.rawtx.batchcrc != otherwp->rawtx.batchcrc )
                        {
                            fprintf(stderr,"%08x miscompares with gatewayid.%d which has crc %08x\n",cp->BATCH.rawtx.batchcrc,gatewayid,otherwp->rawtx.batchcrc);
                            break;
                        }
                    }
                    if ( gatewayid == NUM_GATEWAYS )
                    {
                        fprintf(stderr,"all gateways match\n");
                        if ( Global_mp->gatewayid == 0 )
                        {
                            if ( sign_and_sendmoney(cp,(uint32_t)cp->RTblockheight) >= 0 )
                            {
                                fprintf(stderr,"done and publish\n");
                                publish_withdraw_info(cp,&cp->BATCH);
                            }
                            else
                            {
                                fprintf(stderr,"error signing?\n");
                                cp->BATCH.rawtx.batchcrc = 0;
                                cp->withdrawinfos[0].rawtx.batchcrc = 0;
                                cp->withdrawinfos[0].W.cointxid[0] = 0;
                            }
                        }
                    }
                    free(signedbytes);
                }
            }
        }
    }
    if ( accts != 0 )
        free(accts);
}

int32_t iterate_MGW(char *mgwNXTaddr,char *refassetid)
{
    struct storage_header **msigs;
    struct multisig_addr *msig;
    int32_t createdflag,i,n,nonz = 0;
    struct NXT_assettxid *tp;
    struct coin_info *cp;
    struct NXT_asset *ap;
    cp = conv_assetid(refassetid);
    if ( cp == 0 )
    {
        printf("dont have cp for (%s)\n",refassetid);
        return(-1);
    }
    ap = get_NXTasset(&createdflag,Global_mp,refassetid);
    init_NXT_transactions(mgwNXTaddr,refassetid); // side effect updates MULTISIG_DATA
    if ( (msigs= copy_all_DBentries(&n,MULTISIG_DATA)) != 0 )
    {
        for (i=0; i<n; i++)
        {
            if ( (msig= (struct multisig_addr *)msigs[i]) != 0 )
            {
                if ( strcmp(msig->coinstr,cp->name) == 0 )
                    nonz += (process_msigaddr(ap,refassetid,msig->NXTaddr,cp,msig->multisigaddr) > 0);
                free(msig);
            }
        }
        free(msigs);
    }
    for (i=0; i<ap->num; i++)
    {
        tp = ap->txids[i];
        if ( tp->redeemtxid != 0 )
        {
            
        }
//     if ( tp->receiverbits == nxt64bits && tp->coinblocknum == entry->blocknum && tp->cointxind == entry->txind && tp->coinv == entry->v )
 //           break;
    }
    return(nonz);
}

#endif


