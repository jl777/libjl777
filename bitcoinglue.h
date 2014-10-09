//
//  bitcoinglue.h
//  xcode
//
//  Created by jl777 on 8/6/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_bitcoinglue_h
#define xcode_bitcoinglue_h


struct coin_value *conv_telepod(struct telepod *pod)
{
    struct coin_value *vp;
    vp = calloc(1,sizeof(*vp));
    vp->value = pod->satoshis;
    vp->txid = pod->txid;
    vp->parent_vout = pod->vout;
    safecopy(vp->coinaddr,pod->coinaddr,sizeof(vp->coinaddr));
    vp->U.script = clonestr(pod->script);
    return(vp);
}

void free_coin_value(struct coin_value *vp)
{
    if ( vp != 0 )
    {
        if ( vp->U.script != 0 )
            free(vp->U.script);
        free(vp);
    }
}

void purge_rawtransaction(struct rawtransaction *raw)
{
    int32_t i;
    if ( raw->rawtxbytes != 0 )
        free(raw->rawtxbytes);
    if ( raw->signedtx != 0 )
        free(raw->signedtx);
    for (i=0; i<raw->numinputs; i++)
        if ( raw->inputs[i] != 0 )
            free_coin_value(raw->inputs[i]);
}

char *get_telepod_privkey(char **podaddrp,char *pubkey,struct coin_info *cp)
{
    char *podaddr,*privkey,args[40906];
    privkey = 0;
    pubkey[0] = 0;
    if ( (podaddr= *podaddrp) == 0 )
        podaddr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"getnewaddress","[\"telepods\"]");
    (*podaddrp) = podaddr;
    if ( podaddr != 0 )
    {
        sprintf(args,"\"%s\"",podaddr);
        privkey = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"dumpprivkey",args);
        //fprintf(stderr,"got podaddr.(%s) privkey.%p\n",podaddr,privkey);
        if ( privkey != 0 )
        {
            if ( validate_coinaddr(pubkey,cp,podaddr) > 0 )
            {
                //fprintf(stderr,"pubkey.(%s) podaddr.(%s) validated\n",pubkey,podaddr);
                (*podaddrp) = podaddr;
            }
            else
            {
                fprintf(stderr,"pubkey.(%s) podaddr.(%s) NOT validated\n",pubkey,podaddr);
                free(podaddr);
                free(privkey);
                privkey = 0;
            }
        }
    } else fprintf(stderr,"error getnewaddress telepods\n");
    return(privkey);
}

cJSON *create_privkeys_json_params(struct coin_info *cp,char **privkeys,int32_t numinputs)
{
    int32_t i,nonz = 0;
    cJSON *array;
    array = cJSON_CreateArray();
    for (i=0; i<numinputs; i++)
    {
        if ( cp != 0 && privkeys[i] != 0 )
        {
            nonz++;
            //printf("%s ",localcoinaddrs[i]);
            cJSON_AddItemToArray(array,cJSON_CreateString(privkeys[i]));
        }
    }
    if ( nonz == 0 )
        free_json(array), array = 0;
    //else printf("privkeys.%d of %d: %s\n",nonz,numinputs,cJSON_Print(array));
    return(array);
}

char *createsignraw_json_params(struct coin_info *cp,struct rawtransaction *rp,char *rawbytes,char **privkeys)
{
    char *paramstr = 0;
    cJSON *array,*rawobj,*vinsobj,*keysobj;
    rawobj = cJSON_CreateString(rawbytes);
    if ( rawobj != 0 )
    {
        vinsobj = create_vins_json_params(0,cp,rp);
        if ( vinsobj != 0 )
        {
            keysobj = create_privkeys_json_params(cp,privkeys,rp->numinputs);
            if ( keysobj != 0 )
            {
                array = cJSON_CreateArray();
                cJSON_AddItemToArray(array,rawobj);
                cJSON_AddItemToArray(array,vinsobj);
                cJSON_AddItemToArray(array,keysobj);
                paramstr = cJSON_Print(array);
                free_json(array);
            }
            else free_json(vinsobj);
        }
        else free_json(rawobj);
    }
    return(paramstr);
}

char *createrawtxid_json_params(struct coin_info *cp,struct rawtransaction *rp)
{
    char *paramstr = 0;
    cJSON *array,*vinsobj,*voutsobj;
    vinsobj = create_vins_json_params(0,cp,rp);
    if ( vinsobj != 0 )
    {
        voutsobj = create_vouts_json_params(rp);
        if ( voutsobj != 0 )
        {
            array = cJSON_CreateArray();
            cJSON_AddItemToArray(array,vinsobj);
            cJSON_AddItemToArray(array,voutsobj);
            paramstr = cJSON_Print(array);
            free_json(array);   // this frees both vinsobj and voutsobj
        }
        else free_json(vinsobj);
    } else printf("error create_vins_json_params\n");
    printf("createrawtxid_json_params.%s\n",paramstr);
    return(paramstr);
}

struct telepod *parse_unspent_json(struct coin_info *cp,cJSON *json)
{
    struct telepod *create_telepod(int32_t saveflag,char *refcipher,cJSON *ciphersobj,struct coin_info *cp,uint64_t satoshis,char *podaddr,char *script,char *privkey,char *txid,int32_t vout,uint8_t M,uint8_t N,uint8_t sharenrs[255],uint8_t sharei,int32_t height);
    char args[MAX_JSON_FIELD],txid[MAX_JSON_FIELD],script[MAX_JSON_FIELD],podaddr[MAX_JSON_FIELD],*privkey = 0;
    unsigned char sharenrs[255];
    int32_t vout,M,N;
    uint64_t amount;
    struct telepod *pod = 0;
    M = N = 1;
    memset(sharenrs,0,sizeof(sharenrs));
    copy_cJSON(txid,cJSON_GetObjectItem(json,"txid"));
    copy_cJSON(podaddr,cJSON_GetObjectItem(json,"address"));
    copy_cJSON(script,cJSON_GetObjectItem(json,"scriptPubKey"));
    amount = (uint64_t)(SATOSHIDEN * get_API_float(cJSON_GetObjectItem(json,"amount")));
    vout = get_API_int(cJSON_GetObjectItem(json,"vout"),0);
    if ( txid[0] != 0 && podaddr[0] != 0 && script[0] != 0 && amount != 0 && vout >= 0 )
    {
        sprintf(args,"\"%s\"",podaddr);
        privkey = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"dumpprivkey",args);
        //fprintf(stderr,"got podaddr.(%s) privkey.%p\n",podaddr,privkey);
        if ( privkey != 0 )
        {
            pod = create_telepod(0,cp->name,cp->ciphersobj,cp,amount,podaddr,script,privkey,txid,vout,M,N,sharenrs,N,0);
            free(privkey);
        }
    }
    else printf("illegal unspent output: (%s) (%s) (%s) %.8f %d\n",txid,podaddr,script,dstr(amount),vout);
    return(pod);
}

uint64_t listunspent(struct telepod *inputpods[MAX_COIN_INPUTS],struct coin_info *cp,int32_t minconfirms,char *coinaddr,uint64_t mask)
{
    uint64_t sum = 0;
    int32_t i,j,n;
    cJSON *array;
    char *retstr,params[512];
    sprintf(params,"%d, 99999999, [\"%s\"]",minconfirms,coinaddr);
    //printf("LISTUNSPENT.(%s)\n",params);
    retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"listunspent",params);
    if ( retstr != 0 && retstr[0] != 0 )
    {
        //printf("listunspent (%s) -> (%s)\n",params,retstr);
        if ( (array= cJSON_Parse(retstr)) != 0 )
        {
            if ( is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=j=0; i<n; i++)
                    if ( (inputpods[j]= parse_unspent_json(cp,cJSON_GetArrayItem(array,i))) != 0 )
                        sum += inputpods[j++]->satoshis;
            }
            free(array);
        }
        free(retstr);
    }
    return(sum);
}

uint64_t calc_telepod_inputs(char **privkeys,struct coin_info *cp,struct rawtransaction *rp,struct telepod **srcpods,struct telepod *changepod,uint64_t amount,uint64_t fee,uint64_t change)
{
    int32_t calc_multisig_N(struct telepod *pod);
    int32_t i;
    rp->inputsum = 0;
    rp->amount = 0;
    rp->change = change;
    rp->numinputs = 0;
    for (i=0; i<MAX_COIN_INPUTS; i++)
    {
        if ( srcpods[i] == 0 )
            break;
        //coinaddrs[rp->numinputs] = srcpod->coinaddr
        privkeys[rp->numinputs] = (char *)&srcpods[i]->privkey_shares[srcpods[i]->len_plus1 * calc_multisig_N(srcpods[i])];
        rp->inputs[rp->numinputs++] = conv_telepod(srcpods[i]);
        rp->inputsum += srcpods[i]->satoshis;
        if ( rp->inputsum == (amount + fee + change) )
            break;
    }
    if ( changepod != 0 )
    {
        rp->inputsum += changepod->satoshis;
        //coinaddrs[rp->numinputs] = changepod->coinaddr;
        privkeys[rp->numinputs] = (char *)&changepod->privkey_shares[changepod->len_plus1 * calc_multisig_N(changepod)];
        rp->inputs[rp->numinputs++] = conv_telepod(changepod);
    }
    rp->amount = rp->inputsum - rp->change;
    printf("numinputs %d sum %.8f vs amount %.8f change %.8f -> miners %.8f\n",rp->numinputs,dstr(rp->inputsum),dstr(rp->amount),dstr(rp->change),dstr(rp->inputsum - rp->change - rp->amount));
    if ( rp->inputsum != (rp->amount + fee + rp->change) )
    {
        printf("calc_telepod_inputs: unspent inputs total %.8f != %.8f\n",dstr(rp->inputsum),dstr(rp->amount + fee + rp->change));
        return(0);
    }
    return(rp->inputsum);
}

int64_t calc_telepod_outputs(struct coin_info *cp,struct rawtransaction *rp,char *cloneaddr,uint64_t amount,uint64_t change,char *changeaddr)
{
    int32_t n = 0;
    int64_t fee;
    fee = calc_transporter_fee(cp,amount);
    if ( rp->inputsum == (amount + change + fee) && amount <= rp->inputsum )
    {
        rp->destaddrs[TELEPOD_CONTENTS_VOUT] = cloneaddr;
        rp->destamounts[TELEPOD_CONTENTS_VOUT] = rp->amount;
        n++;
        if ( change > 0 )
        {
            if ( changeaddr != 0 )
            {
                rp->destaddrs[TELEPOD_CHANGE_VOUT] = changeaddr;
                rp->destamounts[TELEPOD_CHANGE_VOUT] = rp->change;
                n++;
            }
            else
            {
                printf("no place to send the change of %.8f\n",dstr(amount));
                rp->numoutputs = 0;
                return(0);
            }
        }
        if ( cp->marker != 0 && (rp->inputsum - rp->amount - rp->change) > cp->txfee )
        {
            printf("calc_telepod_outputs unexpected marker payment\n");
            rp->destaddrs[n] = cp->marker;
            rp->destamounts[n] = (rp->inputsum - rp->amount - rp->change) - cp->txfee;
            n++;
        }
    }
    else printf("not enough inputsum %.8f for withdrawal %.8f %.8f fee %.8f\n",dstr(rp->inputsum),dstr(rp->amount),dstr(amount),dstr(fee));
    rp->numoutputs = n;
    printf("numoutputs.%d\n",n);
    return(rp->amount);
}

int32_t sign_rawtransaction(char *deststr,unsigned long destsize,struct coin_info *cp,struct rawtransaction *rp,char *rawbytes,char **privkeys)
{
    cJSON *json,*hexobj,*compobj;
    int32_t completed = -1;
    char *retstr,*signparams;
    deststr[0] = 0;
    printf("sign_rawtransaction rawbytes.(%s)\n",rawbytes);
    signparams = createsignraw_json_params(cp,rp,rawbytes,privkeys);
    if ( signparams != 0 )
    {
        stripwhite(signparams,strlen(signparams));
        printf("got signparams.(%s)\n",signparams);
        retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"signrawtransaction",signparams);
        if ( retstr != 0 )
        {
            printf("got retstr.(%s)\n",retstr);
            json = cJSON_Parse(retstr);
            if ( json != 0 )
            {
                hexobj = cJSON_GetObjectItem(json,"hex");
                compobj = cJSON_GetObjectItem(json,"complete");
                if ( compobj != 0 )
                    completed = ((compobj->type&0xff) == cJSON_True);
                copy_cJSON(deststr,hexobj);
                if ( strlen(deststr) > destsize )
                    printf("sign_rawtransaction: strlen(deststr) %ld > %ld destize\n",strlen(deststr),destsize);
                //printf("got signedtransaction.(%s) ret.(%s)\n",deststr,retstr);
                free_json(json);
            } else printf("json parse error.(%s)\n",retstr);
            free(retstr);
        } else printf("error signing rawtx\n");
        //free(signparams);
    } else printf("error generating signparams\n");
    return(completed);
}

char *get_transporter_unspent(struct telepod *inputpods[MAX_COIN_INPUTS],uint64_t *availchangep,struct coin_info *cp)
{
    char pubkey[512],coinaddr[MAX_JSON_FIELD],*retstr,*addr = 0;
    cJSON *array;
    int32_t i,n,j;
    *availchangep = 0;
    if ( cp->transporteraddr[0] == 0 )
    {
        retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"getaddressesbyaccount","[\"transporter\"]");
        if ( retstr != 0 && retstr[0] != 0 )
        {
            if ( (array= cJSON_Parse(retstr)) != 0 )
            {
                if ( is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) != 0 )
                {
                    for (i=0; i<n; i++)
                    {
                        coinaddr[0] = 0;
                        *availchangep = 0;
                        copy_cJSON(coinaddr,cJSON_GetArrayItem(array,i));
                        if ( coinaddr[0] != 0 )
                        {
                            if ( validate_coinaddr(pubkey,cp,coinaddr) > 0 )
                            {
                                memset(inputpods,0,sizeof(*inputpods) * MAX_COIN_INPUTS);
                                *availchangep = listunspent(inputpods,cp,1,coinaddr,-1);
                                printf("(%s %.8f) ",coinaddr,dstr(*availchangep));
                                if ( *availchangep > 0 )
                                {
                                    strcpy(cp->transporteraddr,coinaddr);
                                    addr = clonestr(cp->transporteraddr);
                                    printf("set TRANSPORTER.%d ADDRESS.(%s) %.8f\n",i,coinaddr,dstr(*availchangep));
                                    break;
                                }
                                else
                                {
                                    for (j=0; j<MAX_COIN_INPUTS; j++)
                                        if ( inputpods[j] != 0 )
                                            free(inputpods[j]);
                                }
                            }
                            else printf("error with transporter addr.(%s)\n",retstr);
                        }
                    }
                }
                free_json(array);
            }
            free(retstr);
        } else printf("No unspent outputs for transporter account\n");
    }
    return(addr);
}

char *calc_telepod_transaction(struct coin_info *cp,struct rawtransaction *rp,struct telepod *srcpods[],uint64_t srcsatoshis,char *destaddr,uint64_t fee,struct telepod *changepod,uint64_t change,char *changeaddr)
{
    long len;
    int64_t retA=0,retB=0;
    uint64_t amount = (srcsatoshis + change);
    char *rawparams,*privkeys[MAX_COIN_INPUTS],*retstr = 0;
    memset(privkeys,0,sizeof(privkeys));
    printf("calc_telepod_transaction amount %.8f = (%.8f + %.8f)\n",dstr(amount),dstr(srcsatoshis),dstr(change));
    if ( (retA= calc_telepod_inputs(privkeys,cp,rp,srcpods,changepod,amount,fee,change)) == (srcsatoshis + change + fee) )
    {
        if ( (retB= calc_telepod_outputs(cp,rp,destaddr,srcsatoshis,change,changeaddr)) == srcsatoshis )
        {
            rawparams = createrawtxid_json_params(cp,rp);
            if ( rawparams != 0 )
            {
                printf("rawparams.(%s)\n",rawparams);
                stripwhite(rawparams,strlen(rawparams));
                retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"createrawtransaction",rawparams);
                if ( retstr != 0 && retstr[0] != 0 )
                {
                    printf("calc_rawtransaction retstr.(%s)\n",retstr);
                    if ( rp->rawtxbytes != 0 )
                        free(rp->rawtxbytes);
                    rp->rawtxbytes = retstr;
                    len = strlen(retstr) + 4096;
                    if ( rp->signedtx != 0 )
                        free(rp->signedtx);
                    rp->signedtx = calloc(1,len);
                    if ( sign_rawtransaction(rp->signedtx,len,cp,rp,rp->rawtxbytes,privkeys) != 0 )
                    {
                        //jl777: broadcast and save record
                        retstr = send_rawtransaction(cp,rp->signedtx);
                        printf("SEND_TRANSACTION!\n");
                    }
                    else
                    {
                        retstr = 0;
                        printf("error signing rawtransaction\n");
                    }
                } else printf("error creating rawtransaction\n");
                free(rawparams);
            } else printf("error creating rawparams\n");
        } else printf("error calculating rawinputs.%.8f or outputs.%.8f\n",dstr(retA),dstr(retB));
    } else printf("not enough %s clone %.8f\n",cp->name,dstr(srcsatoshis));
    return(retstr);
}

uint64_t get_txid_vout(int32_t *nump,struct coin_info *cp,char *txid,uint32_t vout)
{
    char *rawtransaction=0,*retstr=0,*str=0,buf[4096];
    cJSON *json,*voutobj,*item;
    uint64_t value = 0;
    *nump = 0;
    sprintf(buf,"\"%s\"",txid);
    rawtransaction = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"getrawtransaction",buf);
    if ( rawtransaction != 0 )
    {
        if ( rawtransaction[0] != 0 )
        {
            str = malloc(strlen(rawtransaction)+4);
            sprintf(str,"\"%s\"",rawtransaction);
            retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"decoderawtransaction",str);
            fprintf(stderr,"got rawtransaction.(%s) -> (%s)\n",rawtransaction,retstr);
        }
        if ( rawtransaction != 0 )
            free(rawtransaction);
    } else fprintf(stderr,"null rawtransaction\n");
    if ( retstr != 0 && retstr[0] != 0 )
    {
        json = cJSON_Parse(retstr);
        if ( json != 0 )
        {
            fprintf(stderr,"parsed\n");
            if ( (voutobj= cJSON_GetObjectItem(json,"vout")) != 0 && is_cJSON_Array(voutobj) != 0 && vout < (*nump= cJSON_GetArraySize(voutobj)) )
            {
                item = cJSON_GetArrayItem(voutobj,vout);
                if ( item != 0 )
                    value = conv_cJSON_float(item,"value");
            }
            free_json(json);
        }
    } else fprintf(stderr,"error decoding.(%s)\n",str);
    if ( str != 0 )
        free(str);
    if ( retstr != 0 )
        free(retstr);
    return(value);
}

uint64_t get_unspent_value(char *script,struct coin_info *cp,struct telepod *pod)
{
    uint64_t unspentB,unspent = 0;
    struct coin_txid *tp;
    int32_t createdflag;
    //script[0] = 0;
    while ( cp->initdone < 2 )
    {
        printf(" get_unspent_value %s initdone.%d\n",cp->name,cp->initdone);
        sleep(10);
    }
    tp = MTadd_hashtable(&createdflag,&cp->CACHE.coin_txids,pod->txid);
    printf("tp.numvouts.%d tp->vouts[] %p\n",tp->numvouts,tp->vouts!=0?tp->vouts[pod->vout]:0);
    //unspent = calc_coin_unspent(script,cp,tp,pod->vout);
    if ( pod->vout < tp->numvouts && tp->vouts[pod->vout] != 0 )
    {
        unspentB = tp->vouts[pod->vout]->value;
        if ( unspent != 0 && unspent != unspentB )
            printf("unspent %.8f != unspentB %.8f\n",dstr(unspent),dstr(unspentB));
        else unspent = unspentB;
    }
    printf("%s) txid unspent %.8f\n",pod->txid,dstr(unspent));
    return(unspent);
}
#endif
