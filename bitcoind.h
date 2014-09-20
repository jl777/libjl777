//
//  bitcoind.h
//  xcode
//
//  Created by jl777 on 7/30/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_bitcoind_h
#define xcode_bitcoind_h


// lowest level bitcoind functions
int64_t issue_bitcoind_command(char *extract,struct coin_info *cp,char *command,char *field,char *arg)
{
    char *retstr = 0;
    cJSON *obj,*json;
    int64_t val = 0;
    retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,command,arg);
    json = 0;
    if ( retstr != 0 && retstr[0] != 0 )
    {
        json = cJSON_Parse(retstr);
        if ( field != 0 )
        {
            if ( json != 0 )
            {
                if ( extract == 0 )
                    val = get_cJSON_int(json,field);
                else
                {
                    obj = cJSON_GetObjectItem(json,field);
                    copy_cJSON(extract,obj);
                    val = strlen(extract);
                }
            }
        }
        else if ( extract != 0 )
            copy_cJSON(extract,json);
        if ( json != 0 )
            free_json(json);
        free(retstr);
    }
    return(val);
}

int64_t get_blockheight(struct coin_info *cp)
{
    if ( cp->lastheighttime+1000000 < microseconds() )
    {
        cp->height = issue_bitcoind_command(0,cp,"getinfo","blocks","");
        cp->lastheighttime = microseconds();
        if ( cp->CACHE.ignorelist == 0 && cp->height > 0 )
        {
            cp->CACHE.ignoresize = (int32_t)(cp->height + 100000);
            cp->CACHE.ignorelist = malloc(cp->CACHE.ignoresize);
            memset(cp->CACHE.ignorelist,1,cp->CACHE.ignoresize);
        }
    }
    return(cp->height);
}

void backupwallet(struct coin_info *cp)
{
    char fname[512];
    sprintf(fname,"[\"%s/wallet%s.%d\"]",cp->backupdir,cp->name,cp->backupcount++);
    printf("backup to (%s)\n",fname);
    issue_bitcoind_command(0,cp,"backupwallet",fname,fname);
}

int32_t prep_wallet(struct coin_info *cp,char *walletpass,int32_t unlockseconds)
{
    char walletkey[512],*retstr = 0;
    if ( walletpass != 0 && walletpass[0] != 0 )
    {
        // locking first avoids error, hacky but no time for wallet fiddling now
        retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"walletlock",0);
        if ( retstr != 0 )
        {
            printf("lock returns (%s)\n",retstr);
            free(retstr);
        }
        // jl777: add some error handling!
        sprintf(walletkey,"[\"%s\",%d]",walletpass,unlockseconds);
        retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"walletpassphrase",walletkey);
        if ( retstr != 0 )
        {
            printf("unlock returns (%s)\n",retstr);
            free(retstr);
        }
    }
    return(0);
}

int32_t validate_coinaddr(char pubkey[512],struct coin_info *cp,char *coinaddr)
{
    char quotes[512];
    int64_t len;
    if ( coinaddr[0] != '"' )
        sprintf(quotes,"\"%s\"",coinaddr);
    else safecopy(quotes,coinaddr,sizeof(quotes));
    len = issue_bitcoind_command(pubkey,cp,"validateaddress","pubkey",quotes);
    return((int32_t)len);
}

cJSON *create_vins_json_params(char **localcoinaddrs,struct coin_info *cp,struct rawtransaction *rp)
{
    int32_t i;
    char *txid;
    cJSON *json,*array;
    struct coin_value *vp;
    array = cJSON_CreateArray();
    for (i=0; i<rp->numinputs; i++)
    {
        if ( localcoinaddrs != 0 )
            localcoinaddrs[i] = 0;
        vp = rp->inputs[i];
        if ( vp->iscoinbase != 0 )
        {
            printf("unexpected coinbase in spending input\n");
            free_json(array);
            return(0);
        }
        else
        {
            if ( (txid= vp->txid) == 0 && vp->parent != 0 )
                txid = vp->parent->txid;
            if ( txid == 0 || vp->U.script == 0 )
            {
                printf("unexpected missing txid or script\n");
                free_json(array);
                return(0);
            }
            json = cJSON_CreateObject();
            cJSON_AddItemToObject(json,"txid",cJSON_CreateString(txid));
            cJSON_AddItemToObject(json,"vout",cJSON_CreateNumber(vp->parent_vout));
            cJSON_AddItemToObject(json,"scriptPubKey",cJSON_CreateString(vp->U.script));
            //cJSON_AddItemToObject(json,"redeemScript",cJSON_CreateString(vp->redeemScript));
            if ( localcoinaddrs != 0 )
                localcoinaddrs[i] = vp->coinaddr;
            cJSON_AddItemToArray(array,json);
        }
    }
    return(array);
}

cJSON *create_vouts_json_params(struct rawtransaction *rp)
{
    int32_t i;
    cJSON *json,*obj;
    json = cJSON_CreateObject();
    for (i=0; i<rp->numoutputs; i++)
    {
        obj = cJSON_CreateNumber((double)rp->destamounts[i]/SATOSHIDEN);
        cJSON_AddItemToObject(json,rp->destaddrs[i],obj);
    }
    printf("numdests.%d (%s)\n",rp->numoutputs,cJSON_Print(json));
    return(json);
}

char *send_rawtransaction(struct coin_info *cp,char *txbytes)
{
    char *args,*retstr = 0;
    if ( cp == 0 )
        return(0);
    args = malloc(strlen(txbytes)+4);
    strcpy(args+2,txbytes);
    args[0] = '[';
    args[1] = '"';
    strcat(args,"\"]");

    printf("about to send.(%s)\n",args);
    //getchar();
    retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"sendrawtransaction",args);
    if ( retstr != 0 )
    {
        printf("sendrawtransaction returns.(%s)\n",retstr);
    }
    free(args);
    return(retstr);
}

#ifdef after_testing

int64_t calc_rawinputs(struct coin_info *cp,struct rawtransaction *rp,struct coin_value **ups,int32_t num,int64_t amount)
{
    int64_t sum = 0;
    struct coin_value *vp;
    int32_t i;
    rp->inputsum = rp->numinputs = 0;
    for (i=0; i<num&&i<((int)(sizeof(rp->inputs)/sizeof(*rp->inputs))); i++)
    {
        vp = ups[i];
        sum += vp->value;
        rp->inputs[rp->numinputs++] = vp;
        if ( sum >= amount )
        {
            rp->amount = (amount - cp->txfee);
            rp->change = (sum - amount);
            rp->inputsum = sum;
            printf("numinputs %d sum %.8f vs amount %.8f change %.8f -> miners %.8f\n",rp->numinputs,dstr(rp->inputsum),dstr(amount),dstr(rp->change),dstr(sum - rp->change - rp->amount));
            return(rp->inputsum);
        }
    }
    printf("i.%d error numinputs %d sum %.8f\n",i,rp->numinputs,dstr(rp->inputsum));
    return(0);
}

int64_t calc_rawoutputs(struct coin_info *cp,struct rawtransaction *rp,char *withdrawaddr,int64_t amount,char *changeaddr)
{
    int32_t n = 0;
    int64_t change,fee;
    fee = calc_transporter_fee(cp,amount);
    if ( rp->amount == (amount - fee) && rp->amount <= rp->inputsum )
    {
        if ( cp->marker != 0 )
        {
            rp->destaddrs[n] = cp->marker;
            rp->destamounts[n] = cp->markeramount;
            n++;
        }
        rp->destaddrs[n] = withdrawaddr;
        rp->destamounts[n] = rp->amount;
        n++;
        if ( (change= rp->change) > 0 )
        {
            if ( changeaddr != 0 )
            {
                rp->destaddrs[n] = changeaddr;
                rp->destamounts[n] = rp->change;
                n++;
            }
            else if ( cp->marker != 0 )
            {
                rp->destaddrs[n] = cp->marker;
                rp->destamounts[n] = rp->change;
                n++;
            }
            else
            {
                printf("no place to send the change of %.8f\n",dstr(amount));
                rp->numoutputs = 0;
                return(0);
            }
        }
    }
    else printf("not enough inputsum %.8f for withdrawal %.8f %.8f fee %.8f\n",dstr(rp->inputsum),dstr(rp->amount),dstr(amount),dstr(fee));
    rp->numoutputs = n;
    printf("numoutputs.%d\n",n);
    return(rp->amount);
}

int32_t sign_rawtransaction(char *deststr,unsigned long destsize,struct coin_info *cp,struct rawtransaction *rp,char *rawbytes,char *signparams)
{
    cJSON *json,*hexobj,*compobj;
    int32_t completed = -1;
    char *retstr;
    deststr[0] = 0;
    printf("sign_rawtransaction rawbytes.(%s)\n",rawbytes);
    //signparams = createsignraw_json_params(cp,rp,rawbytes,privkeys);
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

char *calc_rawtransaction(struct coin_info *cp,struct rawtransaction *rp,char *destaddr,uint64_t fee,char *changeaddr,struct coin_value **ups,int32_t num,int64_t amount,int64_t balance)
{
    char *localcoinaddrs[MAX_COIN_INPUTS],privkeys[MAX_COIN_INPUTS][MAX_PRIVKEY_SIZE];
    long len;
    int64_t retA,retB;
    char *rawparams,*signparams,*retstr = 0;
    printf("calc rawtransaction.%s %.8f -> %s balance %.8f\n",cp->name,dstr(amount),destaddr,dstr(balance));
    memset(privkeys,0,sizeof(privkeys));
    memset(localcoinaddrs,0,sizeof(localcoinaddrs));
    if ( amount > fee && balance >= 0 )
    {
        retB = 0;
        if ( (retA= calc_rawinputs(cp,rp,ups,num,amount)) > 0 && (retB= calc_rawoutputs(cp,rp,destaddr,amount,changeaddr)) > 0 && retA >= retB+cp->txfee )
        {
            rawparams = createrawtxid_json_params(privkeys,localcoinaddrs,cp,rp);
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
                    signparams = createsignraw_json_params(cp,rp,rp->rawtxbytes,privkeys);
                    if ( signparams != 0 && sign_rawtransaction(rp->signedtx,len,cp,rp,rp->rawtxbytes,signparams) != 0 )
                    {
                        //jl777: broadcast and save record
                        printf("signed transaction completed\n");
                    }
                    else
                    {
                        retstr = 0;
                        printf("error signing rawtransaction\n");
                    }
                    if ( signparams != 0 )
                        free(signparams);
                } else printf("error creating rawtransaction\n");
                free(rawparams);
            } else printf("error creating rawparams\n");
        } else printf("error calculating rawinputs.%.8f or outputs.%.8f\n",dstr(retA),dstr(retB));
    } //else printf("not enough %s balance %.8f for withdraw %.8f -> %s\n",cp->name,dstr(bp->balance),dstr(amount),destaddr);
    return(retstr);
}
#endif

struct coin_value *update_coin_value(int32_t height,int32_t numoutputs,struct coin_info *cp,int32_t isinternal,int32_t isconfirmed,struct coin_txid *tp,int32_t i,struct coin_value *vp,int64_t value,char *coinaddr,char *script)
{
    if ( vp == 0 )
        vp = calloc(1,sizeof(*vp));
    vp->value = value;
    vp->parent = tp;
    vp->parent_vout = i;
    if ( height < cp->forkheight )
        vp->isinternal = (i > 1) ? isinternal : 0;
    else vp->isinternal = (i == numoutputs-1) ? isinternal : 0;
    safecopy(vp->coinaddr,coinaddr,sizeof(vp->coinaddr));
    if ( script != 0 )
    {
        if ( script[0] != 0 )
        {
            if ( vp->U.script != 0 )
                free(vp->U.script);
            vp->U.script = clonestr(script);
        }
        else printf("update_coin_value: null script? %s.(%d)\n",coinaddr,i);
    }
    //vp->xp = update_crosschain_info(isinternal,coinid,tp,vp,isconfirmed,vp->xp,value,coinaddr);
    tp->hasinternal += vp->isinternal;
    vp->isconfirmed += isconfirmed;
    if ( isinternal != 0 )
        printf("height.%d isinternal.%d %s isconfirmed.%d %.8f -> %s.v%d\n",height,vp->isinternal,cp->name,isconfirmed,dstr(value),coinaddr,vp->parent_vout);
    return(vp);
}

struct coin_txid *find_coin_txid(struct coin_info *cp,char *txid)
{
    uint64_t hashval;
    if ( cp == 0 )
        return(0);
    hashval = MTsearch_hashtable(&cp->CACHE.coin_txids,txid);
    if ( hashval == HASHSEARCH_ERROR )
        return(0);
    else return(cp->CACHE.coin_txids->hashtable[hashval]);
}

extern int32_t is_relevant_coinvalue(int32_t spentflag,struct coin_info *cp,char *txid,int32_t vout,uint64_t value,char *asmbuf,uint32_t blocknum);

int32_t process_vins(struct coin_info *cp,uint32_t height,struct coin_txid *tp,int32_t isconfirmed,cJSON *vins)
{
    cJSON *obj,*txidobj,*coinbaseobj;
    int32_t i,vout,oldnumvins,flag = 0;
    char txid[4096],coinbase[4096];
    struct coin_value *vp;
    struct coin_txid *vintp;
    if ( vins != 0 )
    {
        oldnumvins = tp->numvins;
        tp->numvins = cJSON_GetArraySize(vins);
        tp->vins = realloc(tp->vins,tp->numvins * sizeof(*tp->vins));
        if ( oldnumvins < tp->numvins )
            memset(&tp->vins[oldnumvins],0,(tp->numvins - oldnumvins) * sizeof(*tp->vins));
        for (i=0; i<tp->numvins; i++)
        {
            obj = cJSON_GetArrayItem(vins,i);
            if ( tp->numvins == 1  )
            {
                coinbaseobj = cJSON_GetObjectItem(obj,"coinbase");
                copy_cJSON(coinbase,coinbaseobj);
                if ( strlen(coinbase) > 1 )
                {
                    //printf("got coinbase.(%s)\n",coinbase);
                    vp = calloc(1,sizeof(*vp));
                    vp->parent_vout = 0;
                    vp->parent = tp;
                    vp->iscoinbase = 1;
                    vp->U.coinbase = clonestr(coinbase);
                    tp->vins[0] = vp;
                    //printf("txid.%s is coinbase.%s\n",tp->txid,coinbase);
                    return(flag);
                }
            }
            txidobj = cJSON_GetObjectItem(obj,"txid");
            if ( txidobj != 0 && cJSON_GetObjectItem(obj,"vout") != 0 )
            {
                vout = (int)get_cJSON_int(obj,"vout");
                copy_cJSON(txid,txidobj);
                vintp = find_coin_txid(cp,txid);
                if ( vintp != 0 && vintp->vouts != 0 )
                {
                    //printf("%s vin.%d %.8f\n",txid,i,dstr(vintp->vouts[vout]->value));
                    if ( vout >= 0 && vout < vintp->numvouts )
                    {
                        //printf("got txid.%s in i.%d: vintp.%p vouts.%p (%s)\n",txid,i,vintp,vintp!=0?vintp->vouts:0,vintp->vouts[vout]->coinaddr);
                        vintp->vouts[vout]->spent = tp;
                        vintp->vouts[vout]->spent_vin = i;
                        flag = 1;
                        if ( is_relevant_coinvalue(1,cp,vintp->txid,i,vintp->vouts[vout]->value,"",height) != 0 )
                        {
                            if ( tp->hasinternal == 0 )
                            {
                                printf("unexpected non-internal txid.%s vin.%d internal address %s %s.%d!\n",tp->txid,i,vintp->vouts[vout]->coinaddr,txid,vout);
                                tp->hasinternal = 1;
                            }
                        }
                        vp = vintp->vouts[vout];
                        if ( vp == 0 || vp->parent_vout != vout || vp->parent != vintp )
                        {
                            if ( vp != 0 )
                                printf("error finding vin txid.%s vout.%d | %p %d %p %p\n",txid,vout,vp,vp->parent_vout,vp->parent,vintp);
                        }
                        else
                        {
                            tp->vins[i] = vp;
                            if ( isconfirmed != 0 )
                            {
                                vp->spent = tp, vp->spent_vin = i;
                                if ( vp->pendingspend != 0 && (tp != vp->pendingspend || i != vp->pending_spendvin) )
                                    printf("WARNING: confirm spend %p.%d, pending %p.%d\n",tp,i,vp->pendingspend,vp->pending_spendvin);
                            }
                            else vp->pendingspend = tp, vp->pending_spendvin = i;
                        }
                    }
                    //else printf("vout.%d error when txid.%s numvouts.%d\n",vout,txid,vintp->numvouts);
                }
            } else printf("tp->numvins.%d missing txid.vout for %s\n",tp->numvins,tp->txid);
        }
    }
    return(flag);
}

cJSON *script_has_address(int32_t *nump,cJSON *scriptobj)
{
    int32_t i,n;
    cJSON *addresses,*addrobj;
    if ( scriptobj == 0 )
        return(0);
    addresses = cJSON_GetObjectItem(scriptobj,"addresses");
    *nump = 0;
    if ( addresses != 0 )
    {
        *nump = n = cJSON_GetArraySize(addresses);
        for (i=0; i<n; i++)
        {
            addrobj = cJSON_GetArrayItem(addresses,i);
            return(addrobj);
        }
    }
    return(0);
}

#define OP_HASH160_OPCODE 0xa9
#define OP_EQUAL_OPCODE 0x87
#define OP_DUP_OPCODE 0x76
#define OP_EQUALVERIFY_OPCODE 0x88
#define OP_CHECKSIG_OPCODE 0xac

int32_t add_opcode(char *hex,int32_t offset,int32_t opcode)
{
    hex[offset + 0] = hexbyte((opcode >> 4) & 0xf);
    hex[offset + 1] = hexbyte(opcode & 0xf);
    return(offset+2);
}

void calc_script(char *script,char *pubkey)
{
    int32_t offset,len;
    offset = 0;
    len = (int32_t)strlen(pubkey);
    offset = add_opcode(script,offset,OP_DUP_OPCODE);
    offset = add_opcode(script,offset,OP_HASH160_OPCODE);
    offset = add_opcode(script,offset,len/2);
    memcpy(script+offset,pubkey,len), offset += len;
    offset = add_opcode(script,offset,OP_EQUALVERIFY_OPCODE);
    offset = add_opcode(script,offset,OP_CHECKSIG_OPCODE);
    script[offset] = 0;
}

int32_t convert_to_bitcoinhex(char *scriptasm)
{
    //"asm" : "OP_HASH160 db7f9942da71fd7a28f4a4b2e8c51347240b9e2d OP_EQUAL",
    char *hex,pubkey[512];
    int32_t middlelen,len,OP_HASH160_len,OP_EQUAL_len;
    len = (int32_t)strlen(scriptasm);
    // worlds most silly assembler!
    OP_HASH160_len = strlen("OP_DUP OP_HASH160");
    OP_EQUAL_len = strlen("OP_EQUALVERIFY OP_CHECKSIG");
    if ( strncmp(scriptasm,"OP_DUP OP_HASH160",OP_HASH160_len) == 0 && strncmp(scriptasm+len-OP_EQUAL_len,"OP_EQUALVERIFY OP_CHECKSIG",OP_EQUAL_len) == 0 )
    {
        middlelen = len - OP_HASH160_len - OP_EQUAL_len - 2;
        memcpy(pubkey,scriptasm+OP_HASH160_len+1,middlelen);
        pubkey[middlelen] = 0;
        
        hex = calloc(1,len+1);
        calc_script(hex,pubkey);
        //printf("(%s) -> script.(%s) (%s)\n",scriptasm,pubkey,hex);
        strcpy(scriptasm,hex);
        free(hex);
        return((int32_t)(2+middlelen+2));
    }
    // printf("cant assembly anything but OP_HASH160 + <key> + OP_EQUAL (%s)\n",scriptasm);
    return(-1);
}

uint32_t process_vouts(int32_t height,char *debugstr,struct coin_info *cp,struct coin_txid *tp,int32_t isconfirmed,cJSON *vouts)
{
    uint64_t value,unspent = 0;
    char coinaddr[4096],script[4096];
    cJSON *obj,*scriptobj,*addrobj,*hexobj;
    int32_t i,numaddresses,oldnumvouts,isinternal,flag = 0;
    struct coin_value *vp;
    if ( vouts != 0 )
    {
        oldnumvouts = tp->numvouts;
        tp->numvouts = cJSON_GetArraySize(vouts);
        tp->vouts = realloc(tp->vouts,tp->numvouts*sizeof(*tp->vouts));
        if ( oldnumvouts < tp->numvouts )
            memset(&tp->vouts[oldnumvouts],0,(tp->numvouts-oldnumvouts) * sizeof(*tp->vouts));
        isinternal = 0;
        for (i=0; i<tp->numvouts; i++)
        {
            obj = cJSON_GetArrayItem(vouts,i);
            if ( obj != 0 )
            {
                addrobj = hexobj = 0;
                scriptobj = cJSON_GetObjectItem(obj,"scriptPubKey");
                value = conv_cJSON_float(obj,"value");
                if ( scriptobj != 0 )
                {
                    addrobj = script_has_address(&numaddresses,scriptobj);
                    if ( cp->nohexout != 0 )
                        hexobj = cJSON_GetObjectItem(scriptobj,"asm");
                    else
                        hexobj = cJSON_GetObjectItem(scriptobj,"hex");
                }
                copy_cJSON(script,hexobj);
                if ( cp->nohexout != 0 )
                    convert_to_bitcoinhex(script);
                copy_cJSON(coinaddr,addrobj);
                if ( script[0] == 0 && value > 0 )
                    printf("process_vouts WARNING.(%s) coinaddr,(%s) %s\n",debugstr,coinaddr,script);
                if ( value == 0 )
                    continue;
                if ( i == 0 && cp->marker != 0 && strcmp(cp->marker,coinaddr) == 0 )
                {
                    isinternal = 1;
                    flag = 1;
                }
                vp = update_coin_value(height,tp->numvouts,cp,isinternal,isconfirmed,tp,i,tp->vouts[i],value,coinaddr,script);
                if ( vp != 0 )
                {
                    tp->vouts[i] = vp;
                    if ( vp->spent == 0 && vp->pendingspend == 0 )
                        unspent += value;
                    //printf("%s vout.%d %.8f\n",tp->txid,i,dstr(value));
                    if ( is_relevant_coinvalue(0,cp,tp->txid,i,value,script,height) != 0 )
                        flag = 1;
                }
                // else if ( value > 0 )   // OP_RETURN has no value and no address
                //     printf("unexpected error nval.%d for i.%d of %d | scriptobj %p (%s) addrobj.%p debugstr.(%s)\n",nval,i,tp->numvouts,scriptobj,tp->txid,addrobj,debugstr);
            }
        }
    }
    return(flag);
}

char *unspent_json_params(char *txid,int32_t vout)
{
    char *unspentstr;
    cJSON *array,*nobj,*txidobj;
    array = cJSON_CreateArray();
    nobj = cJSON_CreateNumber(vout);
    txidobj = cJSON_CreateString(txid);
    cJSON_AddItemToArray(array,txidobj);
    cJSON_AddItemToArray(array,nobj);
    unspentstr = cJSON_Print(array);
    free_json(array);
    return(unspentstr);
}

uint64_t calc_coin_unspent(char *script,struct coin_info *cp,struct coin_txid *tp,int32_t vout)
{
    int32_t i,flag;
    char *retstr,*unspentstr;
    cJSON *json;
    struct coin_value *vp;
    int64_t unspent = 0;
    for (i=0; i<tp->numvouts; i++)
    {
        if ( vout >= 0 && i != vout )
            continue;
        vp = tp->vouts[i];
        if ( vp != 0 )
        {
            unspentstr = unspent_json_params(tp->txid,i);
            retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"gettxout",unspentstr);
            flag = 0;
            if ( retstr != 0 && retstr[0] != 0 )
            {
                json = cJSON_Parse(retstr);
                if ( json != 0 )
                {
                    unspent += conv_cJSON_float(json,"value");
                    flag = 1;
                    free_json(json);
                }
                free(retstr);
            }
            if ( (vp->spent == 0 && flag == 0) || (vp->spent != 0 && flag != 0) )
            {
                printf("unspent tracking mismatch?? vp->spent %p vs flag.%d for txid.%s vout.%d (%s)\n",vp->spent,flag,tp->txid,i,unspentstr);
            }
            free(unspentstr);
        }
    }
    return(unspent);
}

void update_coin_values(struct coin_info *cp,struct coin_txid *tp,int64_t blockheight)
{
    char *rawtransaction=0,*retstr=0,*str=0,txid[4096];
    cJSON *json;
    int32_t isconfirmed;
    tp->numvouts = tp->numvins = 0;
    tp->height = (int32_t)blockheight;
    isconfirmed = (blockheight <= (cp->RTblockheight - cp->min_confirms));
    sprintf(txid,"\"%s\"",tp->txid);
    if ( (retstr= tp->decodedrawtx) == 0 )
    {
        rawtransaction = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"getrawtransaction",txid);
        if ( rawtransaction != 0 )
        {
            if ( rawtransaction[0] != 0 )
            {
                str = malloc(strlen(rawtransaction)+4);
                //printf("got rawtransaction.(%s)\n",rawtransaction);
                sprintf(str,"\"%s\"",rawtransaction);
                retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"decoderawtransaction",str);
                if ( retstr != 0 && retstr[0] != 0 )
                {
                    tp->decodedrawtx = retstr;
                    if ( cp->CACHE.cachefp != 0 )
                        update_coincache(cp->CACHE.cachefp,tp->txid,retstr,(int32_t)blockheight);
                } else printf("null retstr from decoderawtransaction (%s)\n",str);
            }
            if ( rawtransaction != 0 )
                free(rawtransaction);
        } else printf("null rawtransaction\n");
    }
    if ( retstr != 0 && retstr[0] != 0 )
    {
        json = cJSON_Parse(retstr);
        if ( json != 0 )
        {
            if ( process_vouts((int32_t)blockheight,retstr,cp,tp,isconfirmed,cJSON_GetObjectItem(json,"vout")) != 0 )
                cp->CACHE.ignorelist[blockheight] = 0;
            if ( process_vins(cp,(uint32_t)blockheight,tp,isconfirmed,cJSON_GetObjectItem(json,"vin")) != 0 )
                cp->CACHE.ignorelist[blockheight] = 0;
            free_json(json);
        }
    } else printf("error decoding.(%s)\n",str);
    if ( str != 0 )
        free(str);
    //if ( retstr != 0 ) its in cache dont free!
    //    free(retstr);
}

int32_t add_bitcoind_uniquetxids(struct coin_info *cp,int64_t blockheight)
{
    char txid[4096],numstr[128],*blockhashstr=0,*blocktxt;
    cJSON *json,*txidobj,*txobj;
    int32_t i,n,createdflag;
    struct coin_txid *tp;
    int64_t block;
    block = (long)blockheight;
    sprintf(numstr,"%ld",(long)block);
    ensure_coincache_blocks(&cp->CACHE,(int32_t)block);
    if ( (blocktxt= cp->CACHE.blocks[block]) == 0 )
    {
        blockhashstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"getblockhash",numstr);
        if ( blockhashstr == 0 || blockhashstr[0] == 0 )
        {
            printf("couldnt get blockhash for %ld at %ld\n",(long)block,(long)blockheight);
            return(-1);
        }
        //if ( 1 || RTmode != 0 || (block % 100) == 0 )
            fprintf(stderr,"%s block.%ld blockhash.%s | lag.%lld\n",cp->name,(long)block,blockhashstr,(long long)(get_blockheight(cp)-block));
        sprintf(txid,"\"%s\"",blockhashstr);
        if ( cp->CACHE.blocks[block] != 0 )
            free(cp->CACHE.blocks[block]);
        cp->CACHE.blocks[block] = blocktxt = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"getblock",txid);
        if ( blocktxt != 0 && blocktxt[0] != 0 )
            update_coincache(cp->CACHE.blocksfp,numstr,blocktxt,(int32_t)block);
        else printf("error getting blocktxt from (%s)\n",blockhashstr);
        free(blockhashstr);
    }
    if ( blocktxt != 0 && blocktxt[0] != 0 )
    {
        json = cJSON_Parse(blocktxt);
        if ( json != 0 )
        {
            txobj = cJSON_GetObjectItem(json,"tx");
            n = cJSON_GetArraySize(txobj);
            for (i=0; i<n; i++)
            {
                txidobj = cJSON_GetArrayItem(txobj,i);
                copy_cJSON(txid,txidobj);
                //printf("blocktxt.%ld i.%d of n.%d %s\n",(long)block,i,n,txid);
                if ( txid[0] != 0 )
                {
                    tp = MTadd_hashtable(&createdflag,&cp->CACHE.coin_txids,txid);
                    update_coin_values(cp,tp,block);
                }
            }
            free_json(json);
        } else printf("getblock error parsing.(%s)\n",txid);
    }
    return(0);
}

void update_coin_timestamps_fifo(struct coin_info *cp,int32_t timestamp)
{
    int32_t i,n;
    n = (int32_t)(sizeof(cp->timestamps)/sizeof(*cp->timestamps));
    for (i=n-1; i>0; i--)
        cp->timestamps[i] = cp->timestamps[i-1];
    cp->timestamps[0] = timestamp;
}

struct telepod *make_traceable_telepod(struct coin_info *cp,char *refcipher,cJSON *ciphersobj,uint64_t satoshis);
void *Coinloop(void *ptr)
{
    void load_telepods(struct coin_info *cp,int32_t maxnofile);
    int32_t i,processed,flag,maxnofile = 100, counter = 0;
    struct coin_info *cp;
    struct coin_txid *tp;
    int64_t height;
    printf("Coinloop numcoins.%d\n",Numcoins);
    for (i=0; i<Numcoins; i++)
    {
        if ( (cp= Daemons[i]) != 0 )
        {
            //printf("coin.%d (%s) %d\n",i,cp->name,strcmp(cp->name,"BTCD"));
            load_telepods(cp,maxnofile);
            /*if ( strcmp(cp->name,"BTCD") == 0 )
            {
                char *str;
                char *select_privacyServer(char *ipaddr,char *portstr);
                printf(">>>>>> %s %s %s\n",cp->name,cp->NXTACCTSECRET,cp->privacyserver);
                if ( cp->NXTACCTSECRET[0] != 0 && cp->privacyserver[0] != 0 )
                    if ( (str= select_privacyServer(cp->privacyserver,0)) != 0 )
                    {
                        printf("select privacyserver.(%s) -> (%s)\n",cp->privacyserver,str);
                        free(str);
                   }

              //make_traceable_telepod(cp,"BTCD",cp->ciphersobj,SATOSHIDEN/10);
            }*/
            //printf("coin.%d (%s) %d\n",i,cp->name,strcmp(cp->name,"BTCD"));
        }
    }
    while ( Historical_done == 0 ) // must process all historical msig addresses and asset transfers
    {
        sleep(1);
        continue;
    }
    printf("Start coinloop\n");
    void init_Teleport();
    init_Teleport();
    while ( 1 )
    {
        processed = 0;
        // printf("top of coinloop\n");
        for (i=0; i<Numcoins; i++)
        {
            cp = Daemons[i];
            //printf("i.%d initdone.%d enabled.%d\n",i,cp->initdone,cp->enabled);
            if ( cp == 0 )
                cp->initdone = 2;
            else
            {
                if ( cp->enabled == 0 )
                    continue;
                if ( cp->initdone == 0 )
                {
                    tp = 0;
                    cp->CACHE.coin_txids = hashtable_create("coin_txids",HASHTABLES_STARTSIZE,sizeof(*tp),((long)&tp->txid[0] - (long)tp),sizeof(tp->txid),((long)&tp->modified - (long)tp));
                    init_rawtx_cache(&cp->CACHE.cachefp,&cp->CACHE,cp->name,"cache");
                    init_rawtx_cache(&cp->CACHE.blocksfp,&cp->CACHE,cp->name,"blocks");
                    backupwallet(cp);
                    cp->initdone = 1;
                }
            }
            if ( cp->initdone == 1 && cp != 0 )
            {
                height = get_blockheight(cp);
                cp->RTblockheight = (int32_t)height;
                if ( cp->blockheight < (height - cp->min_confirms) )
                {
                    printf("historical block.%ld when height.%ld\n",(long)cp->blockheight,(long)height);
                    if ( (cp->CACHE.ignorelist != 0 && cp->blockheight < cp->CACHE.lastignore && cp->CACHE.ignorelist[cp->blockheight] != 0) || add_bitcoind_uniquetxids(cp,cp->blockheight) == 0 )
                    {
                        counter++;
                        processed++;
                        cp->blockheight++;
                    }
                    continue;
                }
                cp->initdone = 2;
                printf("ignorelist.%p lastignore.%d Start RTcoinloop.%s cp->height %ld vs latest %ld\n",cp->CACHE.ignorelist,cp->CACHE.lastignore,cp->name,(long)cp->blockheight,(long)get_blockheight(cp));
                create_ignorelist(&cp->CACHE,cp->name,(int32_t)cp->blockheight,cp->min_confirms*10);
            }
            if ( cp != 0 )
            {
                // right after a new msig is created, deposit can be missed...
                height = get_blockheight(cp);
                cp->RTblockheight = (int32_t)height;
                if ( cp->blockheight < (height - cp->min_confirms) )
                {
                    update_coin_timestamps_fifo(cp,issue_getTime(0));
                    flag = !add_bitcoind_uniquetxids(cp,cp->blockheight); // update universe of possible txids
                    cp->blockheight += flag;
                    if ( flag == 0 )
                        printf("error processing %s %d of %d\n",cp->name,(int)cp->blockheight,(int)cp->RTblockheight);
                    else
                    {
                        purge_coincache(&cp->CACHE,cp->name,(int32_t)cp->blockheight - 100);
                        /*if ( process_pingpong_queue(&cp->podQ,cp) == 0 )
                        {
                            printf("Nothing left to process in %s podQ\n",cp->name);
                            cp->enabled = 0;
                        }*/
                    }
                }
            }
        }
        if ( processed == 0 )
        {
            //printf("Coinloop: no work, sleep\n");
            sleep(10);
        }
    }
    return(0);
    //return(counter);
}

#endif
