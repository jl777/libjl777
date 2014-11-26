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

uint32_t get_blockheight(struct coin_info *cp)
{
    uint32_t height = (uint32_t)cp->RTblockheight;
    if ( cp->lastheighttime+1000000 < microseconds() )
    {
        height = (uint32_t)issue_bitcoind_command(0,cp,"getinfo","blocks","");
        cp->lastheighttime = microseconds();
        if ( cp->CACHE.ignorelist == 0 && height > 0 )
        {
            cp->CACHE.ignoresize = (int32_t)(height + 1000000);
            cp->CACHE.ignorelist = malloc(cp->CACHE.ignoresize);
            memset(cp->CACHE.ignorelist,1,cp->CACHE.ignoresize);
        }
    }
    return(height);
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
        fprintf(stderr,"sendrawtransaction returns.(%s)\n",retstr);
    else fprintf(stderr,"null return from sendrawtransaction\n");
    free(args);
    return(retstr);
}

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

//extern int32_t is_relevant_coinvalue(int32_t spentflag,struct coin_info *cp,char *txid,int32_t vout,uint64_t value,char *asmbuf,uint32_t blocknum);

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
                        if ( 0 )//is_relevant_coinvalue(1,cp,vintp->txid,i,vintp->vouts[vout]->value,"",height) != 0 )
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

int32_t extract_txvals(char *coinaddr,char *script,int32_t nohexout,cJSON *txobj)
{
    int32_t numaddresses;
    cJSON *scriptobj,*addrobj,*hexobj;
    scriptobj = cJSON_GetObjectItem(txobj,"scriptPubKey");
    if ( scriptobj != 0 )
    {
        addrobj = script_has_address(&numaddresses,scriptobj);
        if ( coinaddr != 0 )
            copy_cJSON(coinaddr,addrobj);
        if ( nohexout != 0 )
            hexobj = cJSON_GetObjectItem(scriptobj,"asm");
        else hexobj = cJSON_GetObjectItem(scriptobj,"hex");
        if ( script != 0 )
        {
            copy_cJSON(script,hexobj);
            if ( nohexout != 0 )
                convert_to_bitcoinhex(script);
        }
        return(0);
    }
    return(-1);
}

uint32_t process_vouts(int32_t height,char *debugstr,struct coin_info *cp,struct coin_txid *tp,int32_t isconfirmed,cJSON *vouts)
{
    uint64_t value,unspent = 0;
    char coinaddr[4096],script[4096];
    cJSON *obj;
    int32_t i,oldnumvouts,isinternal,flag = 0;
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
                value = conv_cJSON_float(obj,"value");
                extract_txvals(coinaddr,script,cp->nohexout,obj);
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
                    //if ( is_relevant_coinvalue(0,cp,tp->txid,i,value,script,height) != 0 )
                    //    flag = 1;
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
                        update_coincache(cp->CACHE.cachefp,tp->txid,(uint8_t *)retstr,(int32_t)strlen(retstr)+1);
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
    } else printf("error decoding.(%s)\n",retstr);
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
            update_coincache(cp->CACHE.blocksfp,numstr,(uint8_t *)blocktxt,(int32_t)strlen(blocktxt)+1);
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

// ADDRESS_DATA DB

void set_address_entry(struct address_entry *bp,uint32_t blocknum,int32_t txind,int32_t vin,int32_t vout,int32_t isinternal,int32_t spent)
{
    memset(bp,0,sizeof(*bp));
    bp->blocknum = blocknum;
    bp->txind = txind;
    bp->isinternal = isinternal;
    bp->spent = spent;
    if ( vout >= 0 && vin < 0 )
        bp->v = vout;
    else if ( vin >= 0 && vout < 0 )
        bp->v = vin, bp->vinflag = 1;
}

void add_address_entry(char *coin,char *addr,uint32_t blocknum,int32_t txind,int32_t vin,int32_t vout,int32_t isinternal,int32_t spent,int32_t syncflag)
{
    struct address_entry B;
    set_address_entry(&B,blocknum,txind,vin,vout,isinternal,spent);
    _add_address_entry(coin,addr,&B,1||syncflag);
}

void update_address_entry(char *coin,char *addr,uint32_t blocknum,int32_t txind,int32_t vin,int32_t vout,int32_t isinternal,int32_t spent,int32_t syncflag)
{
    struct address_entry B,*vec;
    int32_t n;
    set_address_entry(&B,blocknum,txind,vin,vout,0,spent);
    if ( (vec= dbupdate_address_entries(&n,coin,addr,&B,1||syncflag)) != 0 )
        free(vec);
}

struct address_entry *get_address_entries(int32_t *nump,char *coin,char *addr)
{
    *nump = 0;
    return(dbupdate_address_entries(nump,coin,addr,0,0));
}

char *get_transaction(struct coin_info *cp,char *txidstr)
{
    char *rawtransaction=0,txid[4096]; //*retstr=0,*str,
    sprintf(txid,"\"%s\", 1",txidstr);
    rawtransaction = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"getrawtransaction",txid);
    /*if ( rawtransaction != 0 )
    {
        if ( rawtransaction[0] != 0 )
        {
            str = malloc(strlen(rawtransaction)+4);
            sprintf(str,"\"%s\"",rawtransaction);
            retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"decoderawtransaction",str);
            free(str);
            if ( retstr == 0 )
                printf("null retstr from decoderawtransaction (%s)\n",rawtransaction);
        }
        free(rawtransaction);
    } else printf("null rawtransaction from (%s) (%s)\n",txid,txidstr);
    return(retstr);*/
    return(rawtransaction);
}

int32_t calc_isinternal(struct coin_info *cp,char *coinaddr,uint32_t height,int32_t i,int32_t numvouts)
{
    if ( i == 0 && cp->marker != 0 && strcmp(cp->marker,coinaddr) == 0 )
    {
        if ( height < cp->forkheight )
            return((i > 1) ? 1 : 0);
        else return((i == numvouts-1) ? 1 : 0);
    }
    return(0);
}

uint64_t update_vouts(int32_t *isinternalp,char *coinaddr,char *script,struct coin_info *cp,uint32_t blockheight,int32_t txind,cJSON *vouts,int32_t vind,int32_t syncflag)
{
    uint64_t value,unspent = 0;
    cJSON *obj;
    int32_t i,numvouts,flag = 0;
    *isinternalp = 0;
    if ( vouts != 0 )
    {
        numvouts = cJSON_GetArraySize(vouts);
        for (i=0; i<numvouts; i++)
        {
            obj = cJSON_GetArrayItem(vouts,i);
            if ( obj != 0 && (vind < 0 || i == vind) )
            {
                value = conv_cJSON_float(obj,"value");
                extract_txvals(coinaddr,script,cp->nohexout,obj);
                if ( script[0] == 0 && value > 0 )
                    printf("process_vouts WARNING coinaddr,(%s) %s\n",coinaddr,script);
                if ( value == 0 )
                    continue;
                unspent += value;
                (*isinternalp) = flag = calc_isinternal(cp,coinaddr,blockheight,i,numvouts);
                add_address_entry(cp->name,coinaddr,blockheight,txind,-1,i,flag,0,syncflag * (i == (numvouts-1) || vind < 0));
            }
        }
    }
    return(unspent);
}

uint64_t update_vins(int32_t *isinternalp,char *coinaddr,char *script,struct coin_info *cp,uint32_t blockheight,int32_t txind,cJSON *vins,int32_t vind,int32_t syncflag)
{
    uint32_t get_blocktxind(int32_t *txindp,struct coin_info *cp,uint32_t blockheight,char *blockhashstr,char *txidstr);
    cJSON *obj,*txidobj,*coinbaseobj,*json,*vouts,*vobj;
    int32_t i,vout,numvins,numvouts,oldtxind,flag = 0;
    char txid[4096],coinbase[4096],blockhash[512],*transaction;
    uint32_t oldblockheight;
    uint64_t value;
    if ( vins != 0 )
    {
        numvins = cJSON_GetArraySize(vins);
        for (i=0; i<numvins; i++)
        {
            if ( vind >= 0 && vind != i )
                continue;
            obj = cJSON_GetArrayItem(vins,i);
            if ( numvins == 1  )
            {
                coinbaseobj = cJSON_GetObjectItem(obj,"coinbase");
                copy_cJSON(coinbase,coinbaseobj);
                if ( strlen(coinbase) > 1 )
                {
                    //printf("txid.%s is coinbase.%s\n",tp->txid,coinbase);
                    return(flag);
                }
            }
            txidobj = cJSON_GetObjectItem(obj,"txid");
            if ( txidobj != 0 && cJSON_GetObjectItem(obj,"vout") != 0 )
            {
                vout = (int)get_cJSON_int(obj,"vout");
                copy_cJSON(txid,txidobj);
                if ( txid[0] != 0 && (transaction= get_transaction(cp,txid)) != 0 )
                {
                    if ( transaction != 0 && transaction[0] != 0 )
                    {
                        json = cJSON_Parse(transaction);
                        if ( json != 0 )
                        {
                            copy_cJSON(blockhash,cJSON_GetObjectItem(obj,"blockhash"));
                            vouts = 0;
                            if ( blockhash[0] == 0 && (vouts= cJSON_GetObjectItem(json,"vout")) != 0 )
                            {
                                numvouts = cJSON_GetArraySize(vouts);
                                if ( vout < numvouts )
                                {
                                    if ( (vobj= cJSON_GetArrayItem(vouts,vout)) != 0 )
                                    {
                                        value = conv_cJSON_float(obj,"value");
                                        extract_txvals(coinaddr,script,cp->nohexout,obj);
                                        if ( script[0] == 0 && value > 0 )
                                            printf("process_vouts WARNING coinaddr,(%s) %s\n",coinaddr,script);
                                        if ( value == 0 )
                                            continue;
                                        if ( (oldblockheight= get_blocktxind(&oldtxind,cp,0,blockhash,txid)) > 0 )
                                        {
                                            update_address_entry(cp->name,coinaddr,oldblockheight,oldtxind,-1,vout,-1,1,0);
                                            add_address_entry(cp->name,coinaddr,blockheight,txind,i,-1,-1,1,syncflag * (i == (numvins-1) ||                                      vind < 0));
                                        }
                                    }
                                } else printf("illegal vout.%d when numvouts.%d for txid.(%s)\n",vout,numvouts,txid);
                            } else printf("no blockhash.(%s) or no vouts.%p for txid.(%s)\n",blockhash,vouts,txid);
                        }
                        free(transaction);
                    }
                }
            }
        }
    }
    return(flag);
}

int32_t update_txid_infos(struct coin_info *cp,uint32_t blockheight,int32_t txind,char *txidstr,int32_t syncflag)
{
    char *retstr=0,coinaddr[4096],script[4096];
    cJSON *json;
    int32_t isinternal,flag = 0;
    retstr = get_transaction(cp,txidstr);
    if ( retstr != 0 && retstr[0] != 0 )
    {
        json = cJSON_Parse(retstr);
        if ( json != 0 )
        {
            if ( update_vouts(&isinternal,coinaddr,script,cp,blockheight,txind,cJSON_GetObjectItem(json,"vout"),-1,syncflag) != 0 )
                flag++;
            if ( update_vins(&isinternal,coinaddr,script,cp,blockheight,txind,cJSON_GetObjectItem(json,"vin"),-1,syncflag) != 0 )
                flag++;
            free_json(json);
        }
    } else printf("error decoding.(%s)\n",retstr);
    if ( retstr != 0 )
        free(retstr);
    return(flag);
}

uint32_t get_blocktxind(int32_t *txindp,struct coin_info *cp,uint32_t blockheight,char *blockhashstr,char *txidstr)
{
    char txid[1024],buf[1024],*blocktxt;
    cJSON *json,*txobj;
    int32_t txind,n;
    uint32_t blockid = 0;
    *txindp = -1;
    sprintf(buf,"\"%s\"",blockhashstr);
    blocktxt = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"getblock",buf);
    if ( blocktxt != 0 && blocktxt[0] != 0 )
    {
        json = cJSON_Parse(blocktxt);
        if ( json != 0 )
        {
            blockid = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"height"),0);
            if ( blockheight == 0 )
                blockheight = blockid;
            txobj = cJSON_GetObjectItem(json,"tx");
            n = cJSON_GetArraySize(txobj);
            for (txind=0; txind<n; txind++)
            {
                copy_cJSON(txid,cJSON_GetArrayItem(txobj,txind));
                printf("%-5s blocktxt.%ld i.%d of n.%d %s\n",cp->name,(long)blockheight,txind,n,txid);
                if ( txidstr != 0 )
                {
                    if ( txid[0] != 0 && strcmp(txid,txidstr) == 0 )
                    {
                        *txindp = txind;
                        break;
                    }
                }
                else update_txid_infos(cp,blockheight,txind,txid,txind == n-1);
            }
            free_json(json);
        } else printf("getblock error parsing.(%s)\n",txid);
    }
    if ( blocktxt != 0 )
        free(blocktxt);
    return(blockid);
}

int32_t update_address_infos(struct coin_info *cp,uint32_t blockheight)
{
    char numstr[128],*blockhashstr=0;
    int32_t txind,flag = 0;
    uint32_t height;
    sprintf(numstr,"%u",blockheight);
    blockhashstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"getblockhash",numstr);
    if ( blockhashstr == 0 || blockhashstr[0] == 0 )
    {
        printf("couldnt get blockhash for %u\n",blockheight);
        return(flag);
    }
    if ( (height= get_blocktxind(&txind,cp,blockheight,blockhashstr,0)) != blockheight )
        printf("mismatched blockheight %u != %u (%s)\n",blockheight,height,blockhashstr);
    else flag++;
    free(blockhashstr);
    return(flag);
}

uint64_t get_txindstr(char *txidstr,char *coinaddr,char *script,struct coin_info *cp,uint32_t blockheight,int32_t txind,int32_t vout)
{
    char numstr[128],buf[1024],*blocktxt,*retstr,*blockhashstr=0;
    uint64_t value = 0;
    cJSON *obj,*json,*txobj,*vouts,*txjson;
    int32_t blockid,n,numvouts,flag = 0;
    sprintf(numstr,"%u",blockheight);
    blockhashstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"getblockhash",numstr);
    if ( blockhashstr == 0 || blockhashstr[0] == 0 )
    {
        printf("couldnt get blockhash for %u\n",blockheight);
        return(flag);
    }
    sprintf(buf,"\"%s\"",blockhashstr);
    blocktxt = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"getblock",buf);
    if ( blocktxt != 0 && blocktxt[0] != 0 )
    {
        json = cJSON_Parse(blocktxt);
        if ( json != 0 )
        {
            blockid = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"height"),0);
            if ( blockheight == 0 )
                blockheight = blockid;
            txobj = cJSON_GetObjectItem(json,"tx");
            n = cJSON_GetArraySize(txobj);
            if ( txind < n )
            {
                copy_cJSON(txidstr,cJSON_GetArrayItem(txobj,txind));
                printf("%-5s blocktxt.%ld i.%d of n.%d %s\n",cp->name,(long)blockheight,txind,n,txidstr);
                if ( txidstr[0] != 0 )
                {
                    retstr = get_transaction(cp,txidstr);
                    if ( retstr != 0 && retstr[0] != 0 )
                    {
                        txjson = cJSON_Parse(retstr);
                        if ( txjson != 0 )
                        {
                            vouts = cJSON_GetObjectItem(txjson,"vout");
                            numvouts = cJSON_GetArraySize(vouts);
                            if ( vout < numvouts )
                            {
                                obj = cJSON_GetArrayItem(vouts,vout);
                                value = conv_cJSON_float(obj,"value");
                                extract_txvals(coinaddr,script,cp->nohexout,obj);
                                if ( script[0] == 0 && value > 0 )
                                    printf("process_vouts WARNING coinaddr,(%s) %s\n",coinaddr,script);
                            } else printf("vout.%d >= numvouts.%d for block.%d txind.%d\n",vout,numvouts,blockheight,txind);
                            free_json(txjson);
                        }
                    }
                    if ( retstr != 0 )
                        free(retstr);
                }
            } else printf("txind.%d >= numtxinds.%d for block.%d\n",txind,n,blockheight);
            free_json(json);
        }
    }
    if ( blocktxt != 0 )
        free(blocktxt);
    free(blockhashstr);
    return(value);
}

char *find_good_changeaddr(struct multisig_addr **msigs,int32_t nummsigs,struct coin_info *cp,char *destaddrs[],int32_t numdestaddrs)
{
    int32_t i,j;
    if ( cp == 0 || destaddrs == 0 )
        return(0);
    for (i=0; i<nummsigs; i++)
    {
        if ( msigs[i] != 0 )
        {
            for (j=0; j<numdestaddrs; j++)
                if ( destaddrs[j] != 0 && strcmp(msigs[i]->coinstr,cp->name) == 0 && strcmp(destaddrs[j],msigs[i]->multisigaddr) == 0 )
                    break;
            if ( j == numdestaddrs )
                return(msigs[i]->multisigaddr);
        }
    }
    return(0);
}

int64_t calc_batchinputs(struct multisig_addr **msigs,int32_t nummsigs,struct coin_info *cp,struct rawtransaction *rp,int64_t amount)
{
    int64_t sum = 0;
    struct coin_value *vp;
    int32_t i;
    struct unspent_info *up = &cp->unspent;
    if ( rp == 0 || up == 0 )
    {
        fprintf(stderr,"unexpected null ptr %p %p\n",up,rp);
        return(0);
    }
    rp->inputsum = rp->numinputs = 0;
    for (i=0; i<up->num&&i<((int)(sizeof(rp->inputs)/sizeof(*rp->inputs))); i++)
    {
        vp = up->vps[i];
        if ( vp == 0 )//|| (rp->xps[rp->numinputs] = vp->xp) == 0 )
            continue;
        sum += vp->value;
        //fprintf(stderr,"input.%d value %.8f\n",rp->numinputs,dstr(vp->value));
        rp->inputs[rp->numinputs++] = vp;
        if ( sum >= (amount + cp->txfee) )
        {
            rp->amount = amount;
            rp->change = (sum - amount - cp->txfee);
            rp->inputsum = sum;
            fprintf(stderr,"numinputs %d sum %.8f vs amount %.8f change %.8f -> miners %.8f\n",rp->numinputs,dstr(rp->inputsum),dstr(amount),dstr(rp->change),dstr(sum - rp->change - rp->amount));
            return(rp->inputsum);
        }
    }
    fprintf(stderr,"error numinputs %d sum %.8f\n",rp->numinputs,dstr(rp->inputsum));
    return(0);
}

int32_t init_batchoutputs(struct coin_info *cp,struct rawtransaction *rp,uint64_t MGWfee)
{
    char *marker = get_backupmarker(cp->name);
    if ( rp->destaddrs[0] == 0 || strcmp(rp->destaddrs[0],marker) != 0 )
        rp->destaddrs[0] = clonestr(marker);
    rp->destamounts[0] = MGWfee;
    return(1);
}

struct rawoutput_entry { char destaddr[MAX_COINADDR_LEN]; double amount; };
void sort_rawoutputs(struct rawtransaction *rp)
{
    struct rawoutput_entry sortbuf[MAX_MULTISIG_OUTPUTS+MAX_MULTISIG_INPUTS];
    int32_t i;
    //fprintf(stderr,"sort_rawoutputs.%d\n",rp->numoutputs);
    if ( rp->numoutputs > 2 )
    {
        memset(sortbuf,0,sizeof(sortbuf));
        for (i=1; i<rp->numoutputs; i++)
        {
            sortbuf[i-1].amount = rp->destamounts[i];
            strcpy(sortbuf[i-1].destaddr,rp->destaddrs[i]);
            //fprintf(stderr,"%d of %d: %s %.8f\n",i-1,rp->numoutputs,sortbuf[i-1].destaddr,dstr(sortbuf[i-1].amount));
        }
        revsortstrs(&sortbuf[0].destaddr[0],rp->numoutputs-1,sizeof(sortbuf[0]));
        //fprintf(stderr,"SORTED\n");
        for (i=0; i<rp->numoutputs-1; i++)
        {
            rp->destamounts[i+1] = sortbuf[i].amount;
            strcpy(rp->destaddrs[i+1],sortbuf[i].destaddr);
            //fprintf(stderr,"%d of %d: %s %.8f\n",i,rp->numoutputs-1,sortbuf[i].destaddr,dstr(sortbuf[i].amount));
        }
    }
}

struct rawinput_entry { char str[MAX_COINTXID_LEN]; struct coin_value *input; struct crosschain_info *xp; };
void sort_rawinputs(struct rawtransaction *rp)
{
    struct rawinput_entry sortbuf[MAX_MULTISIG_INPUTS];
    int32_t i,n = 0;
    //fprintf(stderr,"rawinput_entry.%d\n",rp->numinputs);
    if ( rp->numinputs > 1 )
    {
        memset(sortbuf,0,sizeof(sortbuf));
        for (i=0; i<rp->numinputs; i++)
        {
            if ( rp->inputs[i] != 0 )//&& rp->xps[i] != 0 )
            {
                sprintf(sortbuf[n].str,"%s.%d",rp->inputs[i]->coinaddr,rp->inputs[i]->parent_vout);
                sortbuf[n].input = rp->inputs[i];
                //sortbuf[n].xp = rp->xps[i];
                //fprintf(stderr,"i.%d of %d: %s %p %p\n",i,rp->numinputs,sortbuf[n].str,sortbuf[n].input,sortbuf[n].xp);
                n++;
            }
        }
        if ( n > 0 )
        {
            revsortstrs(&sortbuf[0].str[0],n,sizeof(sortbuf[0]));
            for (i=0; i<n; i++)
            {
                rp->inputs[i] = sortbuf[i].input;
                //rp->xps[i] = sortbuf[i].xp;
                //fprintf(stderr,"i.%d of %d: %s %p %p\n",i,n,sortbuf[i].str,rp->inputs[i],rp->xps[i]);
            }
            rp->numinputs = n;
        }
    }
}

void finalize_destamounts(struct multisig_addr **msigs,int32_t nummsigs,struct coin_info *cp,struct rawtransaction *rp,uint64_t change,uint64_t dust)
{
    struct unspent_info *up = &cp->unspent;
    int32_t i;
    char *changeaddr;
    fprintf(stderr,"finalize_destamounts %p %.f %.f\n",rp,dstr(change),dstr(dust));
    if ( change == 0 ) // need to always have a change addr
    {
        change = rp->destamounts[0] >> 1;
        if ( change > dust )
            change = dust;
        rp->destamounts[0] -= change;
    }
    fprintf(stderr,"sort_rawoutputs.%d\n",rp->numoutputs);
    sort_rawoutputs(rp);
    fprintf(stderr,"sort_rawinputs.%d\n",rp->numinputs);
    sort_rawinputs(rp);
    for (i=0; i<rp->numredeems; i++)
        printf("\"%llu\",",(long long)rp->redeems[i]);
    printf("numredeems.%d\n",rp->numredeems);
    for (i=0; i<rp->numredeems; i++)
        fprintf(stderr,"\"%llu\",",(long long)rp->redeems[i]);
    fprintf(stderr,"FINISHED numredeems.%d\n",rp->numredeems);
    if ( up != 0 && up->minvp != 0 && up->minvp->coinaddr != 0 && up->minvp->coinaddr[0] != 0 )
    {
        for (i=0; i<rp->numoutputs; i++)
            if ( strcmp(up->minvp->coinaddr,rp->destaddrs[i]) == 0 )
                break;
        if ( i != rp->numoutputs )
            changeaddr = find_good_changeaddr(msigs,nummsigs,cp,rp->destaddrs,rp->numoutputs);
        else changeaddr = up->minvp->coinaddr;
        if ( changeaddr != 0 )
        {
            rp->destamounts[rp->numoutputs] = change;
            rp->destaddrs[rp->numoutputs] = clonestr(changeaddr);
            rp->numoutputs++;
        }
        else printf("ERROR: cant get valid change address for coin.%s\n",cp->name);
    }
    else
    {
        //if ( search_multisig_addrs(cp,rp->destaddrs[rp->numoutputs-1]) != 0 )
        //    printf("WARNING: no min acct, change %.8f WILL categorize last output as isinternal even though it was withdraw to deposit addr!\n",dstr(change));
        rp->destamounts[0] += change;
    }
}

void clear_BATCH(struct rawtransaction *rp)
{
    int32_t i;
    fprintf(stderr,"clear_BATCH\n");
    for (i=0; i<rp->numoutputs; i++)
        if ( rp->destaddrs[i] != 0 )
            free(rp->destaddrs[i]);
    memset(rp,0,sizeof(*rp));
}

char *sign_localtx(struct coin_info *cp,struct rawtransaction *rp,char *rawbytes)
{
    int32_t sign_rawtransaction(char *deststr,unsigned long destsize,struct coin_info *cp,struct rawtransaction *rp,char *rawbytes,char **privkeys);
    char *batchsigned;
    fprintf(stderr,"sign_localtx\n");
    rp->batchsize = strlen(rawbytes);
    rp->batchcrc = _crc32(0,rawbytes+10,rp->batchsize-10); // skip past timediff
    batchsigned = malloc(rp->batchsize + rp->numinputs*512 + 512);
    sign_rawtransaction(batchsigned,rp->batchsize + rp->numinputs*512 + 512,cp,rp,rawbytes,0);
    if ( sizeof(rp->batchsigned) < strlen(rp->batchsigned) )
        printf("FATAL: sizeof(rp->signedtransaction) %ld < %ld strlen(rp->batchsigned)\n",sizeof(rp->batchsigned),strlen(rp->batchsigned));
    strncpy(rp->batchsigned,batchsigned,sizeof(rp->batchsigned)-1);
    return(batchsigned);
}

uint64_t scale_batch_outputs(struct coin_info *cp,struct rawtransaction *rp)
{
    uint64_t MGWfee,amount;
    int32_t i,nummarkers;
    MGWfee = (rp->numredeems * (cp->txfee + cp->NXTfee_equiv)) - cp->txfee;
    nummarkers = init_batchoutputs(cp,rp,MGWfee);
    amount = 0;
    for (i=nummarkers; i<rp->numoutputs; i++)
        amount += rp->destamounts[i];
    if ( amount <= MGWfee )
        return(0);
    return(MGWfee + amount);
}

char *calc_batchwithdraw(struct multisig_addr **msigs,int32_t nummsigs,struct coin_info *cp,struct rawtransaction *rp,int64_t estimated,int64_t balance,struct NXT_acct **accts,int32_t numaccts,struct NXT_asset *ap)
{
    char *createrawtxid_json_params(struct coin_info *cp,struct rawtransaction *rp);
    int64_t retA;
    char *rawparams,*retstr = 0,*batchsigned = 0;
    fprintf(stderr,"calc_batchwithdraw.%s numoutputs.%d estimated %.8f -> balance %.8f\n",cp->name,rp->numoutputs,dstr(estimated),dstr(balance));
    if ( cp == 0 )
        return(0);
    rp->amount = scale_batch_outputs(cp,rp);
    if ( rp->amount == 0 )
        return(0);
    fprintf(stderr,"calc_batchwithdraw.%s amount %.8f -> balance %.8f\n",cp->name,dstr(rp->amount),dstr(balance));
    if ( rp->amount+cp->txfee <= balance )
    {
        if ( (retA= calc_batchinputs(msigs,nummsigs,cp,rp,rp->amount)) >= (rp->amount + cp->txfee) )
        {
            finalize_destamounts(msigs,nummsigs,cp,rp,rp->change,cp->dust);
            rawparams = createrawtxid_json_params(cp,rp);
            if ( rawparams != 0 )
            {
                fprintf(stderr,"len.%ld rawparams.(%s)\n",strlen(rawparams),rawparams);
                stripwhite(rawparams,strlen(rawparams));
                retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"createrawtransaction",rawparams);
                if ( retstr != 0 && retstr[0] != 0 )
                {
                    fprintf(stderr,"len.%ld calc_rawtransaction retstr.(%s)\n",strlen(retstr),retstr);
                    batchsigned = sign_localtx(cp,rp,retstr);
                } else fprintf(stderr,"error creating rawtransaction\n");
                free(rawparams);
            } else fprintf(stderr,"error creating rawparams\n");
        } else fprintf(stderr,"error calculating rawinputs.%.8f or outputs.%.8f | txfee %.8f\n",dstr(retA),dstr(rp->amount),dstr(cp->txfee));
    } else fprintf(stderr,"not enough %s balance %.8f for withdraw %.8f txfee %.8f\n",cp->name,dstr(balance),dstr(rp->amount),dstr(cp->txfee));
    return(batchsigned);
}

int32_t is_special_addr(char *NXTaddr)
{
    extern int32_t is_gateway_addr(char *);
    if ( strcmp(NXTaddr,NXTISSUERACCT) == 0 || is_gateway_addr(NXTaddr) != 0 )
        return(1);
    else return(0);
}

int32_t skip_address(char *NXTaddr)
{
    if ( strcmp(NXTaddr,"4551058913252105307") == 0 ) // mistakenly sent to BTC asset id
        return(1);
    if ( strcmp(NXTaddr,NXTISSUERACCT) == 0 )
        return(1);
    if ( strcmp(NXTaddr,GENESISACCT) == 0 || is_special_addr(NXTaddr) != 0 )
        return(1);
    return(0);
}

void *Coinloop(void *ptr)
{
    int32_t iterate_MGW(char *mgwNXTaddr,char *refassetid);
    int32_t i,processed;
    struct coin_info *cp;
    int64_t height;
    printf("Coinloop numcoins.%d\n",Numcoins);
    scan_address_entries();
    //iterate_MGW("7117166754336896747","11060861818140490423");

    for (i=0; i<Numcoins; i++)
    {
        if ( (cp= Daemons[i]) != 0 )
        {
            printf("coin.%d (%s) firstblock.%d\n",i,cp->name,(int32_t)cp->blockheight);
            //load_telepods(cp,maxnofile);
        }
    }
    /*while ( Historical_done == 0 ) // must process all historical msig addresses and asset transfers
    {
        sleep(1);
        continue;
    }
    printf("Start coinloop\n");
    //void init_Teleport();
    //init_Teleport();
    printf("teleport initialized\n");*/
    while ( 1 )
    {
        processed = 0;
        for (i=0; i<Numcoins; i++)
        {
            cp = Daemons[i];
            height = get_blockheight(cp);
            cp->RTblockheight = (int32_t)height;
            if ( cp->blockheight < (height - cp->min_confirms) )
            {
                printf("%s: historical block.%ld when height.%ld\n",cp->name,(long)cp->blockheight,(long)height);
                if ( update_address_infos(cp,(uint32_t)cp->blockheight) != 0 )
                {
                    processed++;
                    cp->blockheight++;
                }
            }
        }
        if ( processed == 0 )
        {
            printf("Coinloop: no work, sleep\n");
            sleep(10);
        }
    }
    return(0);
}

#endif
