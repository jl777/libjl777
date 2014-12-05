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
        /*if ( cp->CACHE.ignorelist == 0 && height > 0 )
        {
            cp->CACHE.ignoresize = (int32_t)(height + 1000000);
            cp->CACHE.ignorelist = malloc(cp->CACHE.ignoresize);
            memset(cp->CACHE.ignorelist,1,cp->CACHE.ignoresize);
        }*/
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
    struct coin_txidind *vp;
    array = cJSON_CreateArray();
    for (i=0; i<rp->numinputs; i++)
    {
        if ( localcoinaddrs != 0 )
            localcoinaddrs[i] = 0;
        vp = rp->inputs[i];
        txid = vp->txid;
        if ( txid == 0 || vp->script == 0 )
        {
            printf("unexpected missing txid or script\n");
            free_json(array);
            return(0);
        }
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json,"txid",cJSON_CreateString(txid));
        cJSON_AddItemToObject(json,"vout",cJSON_CreateNumber(vp->entry.v));
        cJSON_AddItemToObject(json,"scriptPubKey",cJSON_CreateString(vp->script));
        //cJSON_AddItemToObject(json,"redeemScript",cJSON_CreateString(vp->redeemScript));
        if ( localcoinaddrs != 0 )
            localcoinaddrs[i] = vp->coinaddr;
        cJSON_AddItemToArray(array,json);
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
    if ( IS_LIBTEST > 1 )
    {
        // if ( strlen(addr) < 10 ) while ( 1 ) sleep(60);
        set_address_entry(&B,blocknum,txind,vin,vout,isinternal,spent);
        _add_address_entry(coin,addr,&B,syncflag);
    }
}

void update_address_entry(char *coin,char *addr,uint32_t blocknum,int32_t txind,int32_t vin,int32_t vout,int32_t isinternal,int32_t spent,int32_t syncflag)
{
    struct address_entry B,*vec;
    int32_t n;
    if ( IS_LIBTEST > 1 )
    {
        set_address_entry(&B,blocknum,txind,vin,vout,0,spent);
        if ( (vec= dbupdate_address_entries(&n,coin,addr,&B,1||syncflag)) != 0 )
            free(vec);
    }
}

struct address_entry *get_address_entries(int32_t *nump,char *coin,char *addr)
{
    *nump = 0;
    if ( IS_LIBTEST > 1 )
        return(dbupdate_address_entries(nump,coin,addr,0,0));
    else return(0);
}

char *get_blockhashstr(struct coin_info *cp,uint32_t blockheight)
{
    char numstr[128],*blockhashstr=0;
    sprintf(numstr,"%u",blockheight);
    blockhashstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"getblockhash",numstr);
    if ( blockhashstr == 0 || blockhashstr[0] == 0 )
    {
        printf("couldnt get blockhash for %u\n",blockheight);
        if ( blockhashstr != 0 )
            free(blockhashstr);
        return(0);
    }
    return(blockhashstr);
}

cJSON *get_blockjson(uint32_t *heightp,struct coin_info *cp,char *blockhashstr,uint32_t blocknum)
{
    cJSON *json = 0;
    int32_t flag = 0;
    char buf[1024],*blocktxt = 0;
    if ( blockhashstr == 0 )
        blockhashstr = get_blockhashstr(cp,blocknum), flag = 1;
    if ( blockhashstr != 0 )
    {
        sprintf(buf,"\"%s\"",blockhashstr);
        blocktxt = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"getblock",buf);
        if ( blocktxt != 0 && blocktxt[0] != 0 && (json= cJSON_Parse(blocktxt)) != 0 && heightp != 0 )
            *heightp = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"height"),0xffffffff);
        if ( flag != 0 && blockhashstr != 0 )
            free(blockhashstr);
        if ( blocktxt != 0 )
            free(blocktxt);
    }
    return(json);
}

cJSON *_get_blocktxarray(uint32_t *blockidp,int32_t *numtxp,struct coin_info *cp,cJSON *blockjson)
{
    cJSON *txarray = 0;
    if ( blockjson != 0 )
    {
        *blockidp = (uint32_t)get_API_int(cJSON_GetObjectItem(blockjson,"height"),0);
        txarray = cJSON_GetObjectItem(blockjson,"tx");
        *numtxp = cJSON_GetArraySize(txarray);
    }
    return(txarray);
}

char *oldget_transaction(struct coin_info *cp,char *txidstr)
{
    char *rawtransaction=0,txid[4096]; //*retstr=0,*str,
    sprintf(txid,"\"%s\"",txidstr);
    rawtransaction = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"gettransaction",txid);
    return(rawtransaction);
}


char *get_rawtransaction(struct coin_info *cp,char *txidstr)
{
    char txid[4096];
    sprintf(txid,"[\"%s\"]",txidstr);
    return(bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"getrawtransaction",txid));
}

char *get_transaction(struct coin_info *cp,char *txidstr)
{
    char *rawtransaction=0,txid[4096];
    sprintf(txid,"[\"%s\", 1]",txidstr);
    rawtransaction = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"getrawtransaction",txid);
    /* if ( rawtransaction != 0 )
     {
     if ( rawtransaction[0] != 0 )
     {
     str = malloc(strlen(rawtransaction)+4);
     sprintf(str,"\"%s\"",rawtransaction);
     retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"decoderawtransaction",str);
     if ( retstr == 0 )
     printf("null retstr from decoderawtransaction (%s)\n",retstr);
     free(str);
     }
     free(rawtransaction);
     } else printf("null rawtransaction\n");*/
    return(rawtransaction);
}

uint64_t get_txvout(char *blockhash,int32_t *numvoutsp,char *coinaddr,char *script,struct coin_info *cp,cJSON *txjson,char *txidstr,int32_t vout)
{
    char *retstr;
    uint64_t value = 0;
    int32_t numvouts,flag = 0;
    cJSON *vouts,*obj;
    if ( numvoutsp != 0 )
        *numvoutsp = 0;
    coinaddr[0] = script[0] = 0;
    if ( txjson == 0 && txidstr != 0 && txidstr[0] != 0 )
    {
        retstr = get_transaction(cp,txidstr);
        if ( retstr != 0 && retstr[0] != 0 )
            txjson = cJSON_Parse(retstr), flag = 1;
        if ( retstr != 0 )
            free(retstr);
    }
    if ( txjson != 0 )
    {
        if ( blockhash != 0 )
            copy_cJSON(blockhash,cJSON_GetObjectItem(txjson,"blockhash"));
        vouts = cJSON_GetObjectItem(txjson,"vout");
        numvouts = cJSON_GetArraySize(vouts);
        if ( numvoutsp != 0 )
            *numvoutsp = numvouts;
        if ( vout < numvouts )
        {
            obj = cJSON_GetArrayItem(vouts,vout);
            if ( (value = conv_cJSON_float(obj,"value")) > 0 )
            {
                extract_txvals(coinaddr,script,cp->nohexout,obj);
                if ( coinaddr[0] == 0 )
                    printf("(%s) obj.%p vouts.%p num.%d vs %d %s\n",coinaddr,obj,vouts,vout,numvouts,cJSON_Print(txjson));
                if ( script[0] == 0 && value > 0 )
                    printf("process_vouts WARNING coinaddr,(%s) %s\n",coinaddr,script);
            }
        } else printf("vout.%d >= numvouts.%d\n",vout,numvouts);
        if ( flag != 0 )
            free_json(txjson);
    } else printf("get_txout: null txjson\n");
    return(value);
}

int32_t calc_isinternal(struct coin_info *cp,char *coinaddr_v0,uint32_t height,int32_t i,int32_t numvouts)
{
    if ( coinaddr_v0 == 0 || (cp->marker != 0 && strcmp(cp->marker,coinaddr_v0) == 0) )
    {
        if ( height < cp->forkheight )
            return((i > 1) ? 1 : 0);
        else return((i == numvouts-1) ? 1 : 0);
    }
    return(0);
}

uint64_t update_vins(int32_t *isinternalp,char *coinaddr,char *script,struct coin_info *cp,uint32_t blockheight,int32_t txind,cJSON *vins,int32_t vind,int32_t syncflag)
{
    uint32_t get_blocktxind(int32_t *txindp,struct coin_info *cp,uint32_t blockheight,char *blockhashstr,char *txidstr);
    cJSON *obj,*txidobj,*coinbaseobj;
    int32_t i,vout,numvins,numvouts,oldtxind,flag = 0;
    char txidstr[1024],coinbase[1024],blockhash[1024];
    uint32_t oldblockheight;
    if ( vins != 0 && is_cJSON_Array(vins) != 0 && (numvins= cJSON_GetArraySize(vins)) > 0 )
    {
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
                    if ( txind > 0 )
                        printf("txind.%d is coinbase.%s\n",txind,coinbase);
                    return(flag);
                }
            }
            txidobj = cJSON_GetObjectItem(obj,"txid");
            if ( txidobj != 0 && cJSON_GetObjectItem(obj,"vout") != 0 )
            {
                vout = (int)get_cJSON_int(obj,"vout");
                copy_cJSON(txidstr,txidobj);
                if ( txidstr[0] != 0 && get_txvout(blockhash,&numvouts,coinaddr,script,cp,0,txidstr,vout) != 0 && blockhash[0] != 0 )
                {
                    //printf("process input.(%s)\n",coinaddr);
                    if ( (oldblockheight= get_blocktxind(&oldtxind,cp,0,blockhash,txidstr)) > 0 )
                    {
                        flag++;
                        add_address_entry(cp->name,coinaddr,oldblockheight,oldtxind,-1,vout,-1,1,0);
                        add_address_entry(cp->name,coinaddr,blockheight,txind,i,-1,-1,1,syncflag * (i == (numvins-1)));
                    } else printf("error getting oldblockheight (%s %s)\n",blockhash,txidstr);
                } else printf("unexpected error vout.%d %s\n",vout,txidstr);
            } else printf("illegal txid.(%s)\n",txidstr);
        }
    }
    return(flag);
}

void update_txid_infos(struct coin_info *cp,uint32_t blockheight,int32_t txind,char *txidstr,int32_t syncflag)
{
    char coinaddr[1024],script[4096],coinaddr_v0[1024],*retstr = 0;
    int32_t v,tmp,numvouts,isinternal = 0;
    cJSON *txjson;
    if ( (retstr= get_transaction(cp,txidstr)) != 0 )
    {
        if ( (txjson= cJSON_Parse(retstr)) != 0 )
        {
            v = 0;
            if ( get_txvout(0,&numvouts,coinaddr_v0,script,cp,txjson,0,v) > 0 )
                add_address_entry(cp->name,coinaddr_v0,blockheight,txind,-1,v,isinternal,0,syncflag * (v == (numvouts-1)));
            for (v=1; v<numvouts; v++)
            {
                if ( v < numvouts && get_txvout(0,&tmp,coinaddr,script,cp,txjson,0,v) > 0 )
                {
                    isinternal = calc_isinternal(cp,coinaddr_v0,blockheight,v,numvouts);
                    add_address_entry(cp->name,coinaddr,blockheight,txind,-1,v,isinternal,0,syncflag * (v == (numvouts-1)));
                }
            }
            update_vins(&isinternal,coinaddr,script,cp,blockheight,txind,cJSON_GetObjectItem(txjson,"vin"),-1,syncflag);
            free_json(txjson);
        } else printf("update_txid_infos parse error.(%s)\n",retstr);
        free(retstr);
    } else printf("error getting.(%s)\n",txidstr);
}

uint32_t get_blocktxind(int32_t *txindp,struct coin_info *cp,uint32_t blockheight,char *blockhashstr,char *reftxidstr)
{
    char txidstr[1024];
    cJSON *json,*txobj;
    int32_t txind,n;
    uint32_t blockid = 0;
    *txindp = -1;
    if ( (json= get_blockjson(0,cp,blockhashstr,blockheight)) != 0 )
    {
        if ( (txobj= _get_blocktxarray(&blockid,&n,cp,json)) != 0 )
        {
            if ( blockheight == 0 )
                blockheight = blockid;
            for (txind=0; txind<n; txind++)
            {
                copy_cJSON(txidstr,cJSON_GetArrayItem(txobj,txind));
                if ( Debuglevel > 2 )
                    printf("%-5s blocktxt.%ld i.%d of n.%d %s\n",cp->name,(long)blockheight,txind,n,txidstr);
                if ( reftxidstr != 0 )
                {
                    if ( txidstr[0] != 0 && strcmp(txidstr,reftxidstr) == 0 )
                    {
                        *txindp = txind;
                        break;
                    }
                }  else update_txid_infos(cp,blockheight,txind,txidstr,txind == n-1);
            }
        }
        free_json(json);
    } else printf("get_blockjson error parsing.(%s)\n",txidstr);
    return(blockid);
}

int32_t update_address_infos(struct coin_info *cp,uint32_t blockheight)
{
    char *blockhashstr=0;
    int32_t txind,flag = 0;
    uint32_t height;
    if ( (blockhashstr = get_blockhashstr(cp,blockheight)) != 0 )
    {
        if ( (height= get_blocktxind(&txind,cp,blockheight,blockhashstr,0)) != blockheight )
            printf("mismatched blockheight %u != %u (%s)\n",blockheight,height,blockhashstr);
        else flag++;
    }
    free(blockhashstr);
    return(flag);
}

uint64_t get_txoutstr(int32_t *numvoutsp,char *txidstr,char *coinaddr,char *script,struct coin_info *cp,uint32_t blockheight,int32_t txind,int32_t vout)
{
    uint64_t value = 0;
    cJSON *json,*txobj;
    int32_t n;
    uint32_t blockid = 0;
    if ( (json= get_blockjson(0,cp,0,blockheight)) != 0 )
    {
        if ( (txobj= _get_blocktxarray(&blockid,&n,cp,json)) != 0 && txind < n )
        {
            copy_cJSON(txidstr,cJSON_GetArrayItem(txobj,txind));
            if ( Debuglevel > 2 )
                printf("%-5s blocktxt.%ld i.%d of n.%d %s\n",cp->name,(long)blockheight,txind,n,txidstr);
            value = get_txvout(0,numvoutsp,coinaddr,script,cp,0,txidstr,vout);
        } else printf("txind.%d >= numtxinds.%d for block.%d\n",txind,n,blockheight);
        free_json(json);
    }
    return(value);
}

uint32_t get_txidind(int32_t *txindp,struct coin_info *cp,char *reftxidstr,int32_t vout)
{
    char blockhash[1024],coinaddr[1024],txidstr[1024],script[4096];
    uint32_t blockid,blocknum = 0xffffffff;
    int32_t i,n,numvouts;
    cJSON *json,*txarray;
    *txindp = -1;
    if ( txidstr[0] != 0 && get_txvout(blockhash,&numvouts,coinaddr,script,cp,0,reftxidstr,vout) != 0 && blockhash[0] != 0 )
    {
        if ( (json= get_blockjson(&blocknum,cp,blockhash,blocknum)) != 0 )
        {
            if ( (txarray= _get_blocktxarray(&blockid,&n,cp,json)) != 0 )
            {
                for (i=0; i<n; i++)
                {
                    copy_cJSON(txidstr,cJSON_GetArrayItem(txarray,i));
                    if ( strcmp(txidstr,reftxidstr) == 0 )
                    {
                        *txindp = i;
                        break;
                    }
                }
            }
            free_json(json);
        }
    }
    return(blocknum);
}

int32_t get_txinstr(char *txidstr,struct coin_info *cp,uint32_t blockheight,int32_t txind,int32_t vin)
{
    char input_txid[1024],*retstr;
    cJSON *obj,*json,*txobj,*vins,*txjson;
    int32_t n,numvins,origvout = -1;
    uint32_t blockid = 0;
    if ( (json= get_blockjson(0,cp,0,blockheight)) != 0 )
    {
        if ( (txobj= _get_blocktxarray(&blockid,&n,cp,json)) != 0 && txind < n )
        {
            copy_cJSON(input_txid,cJSON_GetArrayItem(txobj,txind));
            if ( Debuglevel > 2 )
                printf("%-5s blocktxt.%ld i.%d of n.%d %s\n",cp->name,(long)blockheight,txind,n,input_txid);
            if ( input_txid[0] != 0 && (retstr= get_transaction(cp,input_txid)) != 0 )
            {
                if ( (txjson= cJSON_Parse(retstr)) != 0 )
                {
                    vins = cJSON_GetObjectItem(txjson,"vin");
                    numvins = cJSON_GetArraySize(vins);
                    if ( vin < numvins )
                    {
                        obj = cJSON_GetArrayItem(vins,vin);
                        copy_cJSON(txidstr,cJSON_GetObjectItem(obj,"txid"));
                        origvout = (int32_t)get_API_int(cJSON_GetObjectItem(obj,"vout"),-1);
                    }
                    free_json(txjson);
                }
                free(retstr);
            } else printf("txind.%d >= numtxinds.%d for block.%d\n",txind,n,blockheight);
        }
        free_json(json);
    }
    return(origvout);
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
    struct coin_txidind *vp;
    int32_t i;
    struct unspent_info *up = &cp->unspent;
    if ( rp == 0 || up == 0 )
    {
        fprintf(stderr,"unexpected null ptr %p %p\n",up,rp);
        return(0);
    }
    rp->inputsum = rp->numinputs = 0;
    for (i=0; i<up->num&&i<((int)(sizeof(rp->inputs)/sizeof(*rp->inputs)))-1; i++)
    {
        vp = up->vps[i];
        if ( vp == 0 )
            continue;
        sum += vp->value;
        fprintf(stderr,"%p %s input.%d value %.8f\n",vp,vp->coinaddr,rp->numinputs,dstr(vp->value));
        rp->inputs[rp->numinputs++] = (sum < (amount + cp->txfee)) ? vp : up->vps[up->num-1];
        if ( sum >= (amount + cp->txfee) && rp->numinputs > 1 )
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

struct rawinput_entry { char str[MAX_COINTXID_LEN]; struct coin_txidind *input; void *xp; };
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
                sprintf(sortbuf[n].str,"%s.%d",rp->inputs[i]->coinaddr,rp->inputs[i]->entry.v);
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

char *calc_batchwithdraw(struct multisig_addr **msigs,int32_t nummsigs,struct coin_info *cp,struct rawtransaction *rp,int64_t estimated,int64_t balance,struct NXT_asset *ap)
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

uint32_t extract_sequenceid(int32_t *numinputsp,struct coin_info *cp,char *rawtx,int32_t vind)
{
    uint32_t sequenceid = 0xffffffff;
    int32_t numinputs;
    cJSON *json,*vin,*item;
    char *retstr,*str;
    *numinputsp = 0;
    str = malloc(strlen(rawtx)+4);
    //printf("got rawtransaction.(%s)\n",rawtransaction);
    sprintf(str,"\"%s\"",rawtx);
    retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"decoderawtransaction",str);
    if ( retstr != 0 && retstr[0] != 0 )
    {
        //printf("decoded.(%s)\n",retstr);
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            if ( (vin= cJSON_GetObjectItem(json,"vin")) != 0 && is_cJSON_Array(vin) != 0 && (numinputs= cJSON_GetArraySize(vin)) > vind )
            {
                *numinputsp = numinputs;
                if ( (item= cJSON_GetArrayItem(vin,vind)) != 0 )
                    sequenceid = (uint32_t)get_API_int(cJSON_GetObjectItem(item,"sequence"),0);
            }
            free_json(json);
        }
    }
    if ( retstr != 0 )
        free(retstr);
    free(str);
    return(sequenceid);
}

int32_t replace_bitcoin_sequenceid(struct coin_info *cp,char *rawtx,uint32_t newbytes)
{
    char numstr[9];
    int32_t i,n,numinputs;
    uint32_t val;
    n = (int32_t)(strlen(rawtx) - 8);
    for (i=0; i<n; i++)
        if ( memcmp(&rawtx[i],"ffffffff",8) == 0 )
            break;
    if ( i < n )
    {
        init_hexbytes_noT(numstr,(void *)&newbytes,4);
        memcpy(&rawtx[i],numstr,8);
        if ( cp != 0 )
        {
            if ( (val= extract_sequenceid(&numinputs,cp,rawtx,0)) != newbytes )
            {
                printf("val.%u != newbytes.%u\n",val,newbytes);
                return(-1);
            }
        }
        return(i);
    }
    return(-1);
}

void *Coinloop(void *ptr)
{
    int32_t i,processed;
    struct coin_info *cp;
    int64_t height;
    init_Contacts();
    if ( (cp= get_coin_info("BTCD")) != 0 )
    {
        printf("add myhandle\n");
        addcontact(Global_mp->myhandle,cp->privateNXTADDR);
        printf("add mypublic\n");
        addcontact("mypublic",cp->srvNXTADDR);
    }
    printf("Coinloop numcoins.%d\n",Numcoins);
    scan_address_entries();
    for (i=0; i<Numcoins; i++)
    {
        if ( (cp= Daemons[i]) != 0 )
        {
            printf("coin.%d (%s) firstblock.%d\n",i,cp->name,(int32_t)cp->blockheight);
            //load_telepods(cp,maxnofile);
        }
    }
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
                //if ( Debuglevel > 2 )
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
            if ( Debuglevel > 2 )
                printf("Coinloop: no work, sleep\n");
            sleep(10);
        }
    }
    return(0);
}

#endif
