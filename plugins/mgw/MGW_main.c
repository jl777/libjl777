//
//  echodemo.c
//  SuperNET API extension example plugin
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//
// add to limbo:  txids 204961939803594792, 7301360590217481477, 14387806392702706073 for a total of 568.1248 BTCD

#define BUNDLED
#define PLUGINSTR "MGW"
#define PLUGNAME(NAME) MGW ## NAME
#define STRUCTNAME struct PLUGNAME(_info)
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
#include "../plugin777.c"
#include "storage.c"
#include "system777.c"
#include "NXT777.c"
#undef DEFINES_ONLY

int32_t MGW_idle(struct plugin_info *plugin) { return(0); }

STRUCTNAME MGW;
char *PLUGNAME(_methods)[] = { "myacctpubkeys" };
char *PLUGNAME(_pubmethods)[] = { "myacctpubkeys" };
char *PLUGNAME(_authmethods)[] = { "myacctpubkeys" };

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME   *data,cJSON *json)
{
    uint64_t disableflags = 0;
    printf("init %s size.%ld\n",plugin->name,sizeof(struct MGW_info));
    return(disableflags); // set bits corresponding to array position in _methods[]
}

int32_t get_NXT_coininfo(uint64_t srvbits,uint64_t nxt64bits,char *coinstr,char *coinaddr,char *pubkey)
{
    uint64_t key[3]; char *keycoinaddr,buf[256]; int32_t flag,len = sizeof(buf);
    key[0] = stringbits(coinstr);
    key[1] = srvbits;
    key[2] = nxt64bits;
    flag = 0;
    coinaddr[0] = pubkey[0] = 0;
    //printf("add.(%s) -> (%s)\n",newcoinaddr,newpubkey);
    if ( (keycoinaddr= db777_get(buf,&len,0,DB_NXTaccts,key,sizeof(key))) != 0 )
    {
        strcpy(coinaddr,keycoinaddr);
        //free(keycoinaddr);
    }
    db777_findstr(pubkey,512,DB_NXTaccts,coinaddr);
    return(coinaddr[0] != 0 && pubkey[0] != 0);
}

int32_t add_NXT_coininfo(uint64_t srvbits,uint64_t nxt64bits,char *coinstr,char *newcoinaddr,char *newpubkey)
{
    uint64_t key[3]; char *coinaddr,pubkey[513],buf[1024]; int32_t len = sizeof(buf),flag,updated = 0;
    key[0] = stringbits(coinstr);
    key[1] = srvbits;
    key[2] = nxt64bits;
    flag = 1;
//printf("add.(%s) -> (%s)\n",newcoinaddr,newpubkey);
    if ( (coinaddr= db777_get(buf,&len,0,DB_NXTaccts,key,sizeof(key))) != 0 )
    {
        if ( strcmp(coinaddr,newcoinaddr) == 0 )
            flag = 0;
    }
    if ( flag != 0 )
    {
        if ( db777_add(1,0,DB_NXTaccts,key,sizeof(key),newcoinaddr,(int32_t)strlen(newcoinaddr)+1) == 0 )
            updated = 1;
        else printf("error adding (%s)\n",newcoinaddr);
    }
    flag = 1;
    if ( db777_findstr(pubkey,sizeof(pubkey),DB_NXTaccts,newcoinaddr) > 0 )
    {
        if ( strcmp(pubkey,newpubkey) == 0 )
            flag = 0;
    }
    //printf("oldpubkey.(%s) new.(%s)\n",pubkey,newpubkey);
    if ( flag != 0 )
    {
        if ( db777_addstr(DB_NXTaccts,newcoinaddr,newpubkey) == 0 )
            updated = 1;//, printf("added (%s)\n",newpubkey);
        else printf("error adding (%s)\n",newpubkey);
    }
    return(updated);
}

void multisig_keystr(char *keystr,char *coinstr,char *NXTaddr,char *msigaddr)
{
    if ( msigaddr == 0 || msigaddr[0] == 0 )
        sprintf(keystr,"%s.%s",coinstr,NXTaddr);
    else sprintf(keystr,"%s.%s",coinstr,msigaddr);
}

struct multisig_addr *find_msigaddr(struct multisig_addr *msig,int32_t *lenp,char *coinstr,char *NXTaddr,char *msigaddr)
{
    char keystr[1024];
    multisig_keystr(keystr,coinstr,NXTaddr,msigaddr);
    printf("search_msig.(%s)\n",keystr);
    return(db777_get(msig,lenp,0,DB_msigs,keystr,(int32_t)strlen(keystr)+1));
}

int32_t save_msigaddr(char *coinstr,char *NXTaddr,struct multisig_addr *msig,int32_t len)
{
    char keystr[1024];
    multisig_keystr(keystr,coinstr,NXTaddr,msig->multisigaddr);
    printf("save_msig.(%s)\n",keystr);
    return(db777_add(0,0,DB_msigs,keystr,(int32_t)strlen(keystr)+1,msig,len));
}

int32_t get_redeemscript(char *redeemScript,char *normaladdr,char *coinstr,char *serverport,char *userpass,char *multisigaddr)
{
    cJSON *json,*array,*json2;
    char args[1024],addr[1024],*retstr,*retstr2;
    int32_t i,n,ismine = 0;
    redeemScript[0] = normaladdr[0] = 0;
    sprintf(args,"\"%s\"",multisigaddr);
    if ( (retstr= bitcoind_passthru(coinstr,serverport,userpass,"validateaddress",args)) != 0 )
    {
        printf("get_redeemscript retstr.(%s)\n",retstr);
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"addresses")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    ismine = 0;
                    copy_cJSON(addr,cJSON_GetArrayItem(array,i));
                    if ( addr[0] != 0 )
                    {
                        sprintf(args,"\"%s\"",addr);
                        retstr2 = bitcoind_passthru(coinstr,serverport,userpass,"validateaddress",args);
                        if ( retstr2 != 0 )
                        {
                            if ( (json2= cJSON_Parse(retstr2)) != 0 )
                                ismine = is_cJSON_True(cJSON_GetObjectItem(json2,"ismine")), free_json(json2);
                            free(retstr2);
                        }
                    }
                    if ( ismine != 0 )
                    {
                        //printf("(%s) ismine.%d\n",addr,ismine);
                        strcpy(normaladdr,addr);
                        copy_cJSON(redeemScript,cJSON_GetObjectItem(json,"hex"));
                        break;
                    }
                }
            } free_json(json);
        } free(retstr);
    }
    return(ismine);
}

int32_t _map_msigaddr(char *redeemScript,char *coinstr,char *serverport,char *userpass,char *normaladdr,char *msigaddr,int32_t gatewayid,int32_t numgateways) //could map to rawind, but this is rarely called
{
    int32_t ismine,len; char buf[8192]; struct multisig_addr *msig;
    redeemScript[0] = normaladdr[0] = 0;
    if ( (msig= find_msigaddr((struct multisig_addr *)buf,&len,coinstr,0,msigaddr)) == 0 )
    {
        strcpy(normaladdr,msigaddr);
        printf("cant find_msigaddr.(%s)\n",msigaddr);
        return(0);
    }
    if ( msig->redeemScript[0] != 0 && gatewayid >= 0 && gatewayid < numgateways )
    {
        strcpy(normaladdr,msig->pubkeys[gatewayid].coinaddr);
        strcpy(redeemScript,msig->redeemScript);
        printf("_map_msigaddr.(%s) -> return (%s) redeem.(%s)\n",msigaddr,normaladdr,redeemScript);
        return(1);
    }
    ismine = get_redeemscript(redeemScript,normaladdr,coinstr,serverport,userpass,msig->multisigaddr);
    if ( normaladdr[0] != 0 )
        return(1);
    strcpy(normaladdr,msigaddr);
    return(-1);
}

int32_t generate_multisigaddr(char *multisigaddr,char *redeemScript,char *coinstr,char *serverport,char *userpass,int32_t addmultisig,char *params)
{
    char addr[1024],*retstr;
    cJSON *json,*redeemobj,*msigobj;
    int32_t flag = 0;
    if ( addmultisig != 0 )
    {
        if ( (retstr= bitcoind_passthru(coinstr,serverport,userpass,"addmultisigaddress",params)) != 0 )
        {
            strcpy(multisigaddr,retstr);
            free(retstr);
            sprintf(addr,"\"%s\"",multisigaddr);
            if ( (retstr= bitcoind_passthru(coinstr,serverport,userpass,"validateaddress",addr)) != 0 )
            {
                json = cJSON_Parse(retstr);
                if ( json == 0 ) printf("Error before: [%s]\n",cJSON_GetErrorPtr());
                else
                {
                    if ( (redeemobj= cJSON_GetObjectItem(json,"hex")) != 0 )
                    {
                        copy_cJSON(redeemScript,redeemobj);
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
        if ( (retstr= bitcoind_passthru(coinstr,serverport,userpass,"createmultisig",params)) != 0 )
        {
            json = cJSON_Parse(retstr);
            if ( json == 0 ) printf("Error before: [%s]\n",cJSON_GetErrorPtr());
            else
            {
                if ( (msigobj= cJSON_GetObjectItem(json,"address")) != 0 )
                {
                    if ( (redeemobj= cJSON_GetObjectItem(json,"redeemScript")) != 0 )
                    {
                        copy_cJSON(multisigaddr,msigobj);
                        copy_cJSON(redeemScript,redeemobj);
                        flag = 1;
                    } else printf("missing redeemScript in (%s)\n",retstr);
                } else printf("multisig missing address in (%s) params.(%s)\n",retstr,params);
                free_json(json);
            }
            free(retstr);
        } else printf("error issuing createmultisig.(%s)\n",params);
    }
    return(flag);
}

char *createmultisig_json_params(struct pubkey_info *pubkeys,int32_t m,int32_t n,char *acctparm)
{
    int32_t i;
    char *paramstr = 0;
    cJSON *array,*mobj,*keys,*key;
    keys = cJSON_CreateArray();
    for (i=0; i<n; i++)
    {
        key = cJSON_CreateString(pubkeys[i].pubkey);
        cJSON_AddItemToArray(keys,key);
    }
    mobj = cJSON_CreateNumber(m);
    array = cJSON_CreateArray();
    if ( array != 0 )
    {
        cJSON_AddItemToArray(array,mobj);
        cJSON_AddItemToArray(array,keys);
        if ( acctparm != 0 )
            cJSON_AddItemToArray(array,cJSON_CreateString(acctparm));
        paramstr = cJSON_Print(array);
        _stripwhite(paramstr,' ');
        free_json(array);
    }
    //printf("createmultisig_json_params.(%s)\n",paramstr);
    return(paramstr);
}

int32_t issue_createmultisig(char *multisigaddr,char *redeemScript,char *coinstr,char *serverport,char *userpass,int32_t use_addmultisig,struct multisig_addr *msig)
{
    int32_t flag = 0;
    char *params;
    params = createmultisig_json_params(msig->pubkeys,msig->m,msig->n,(use_addmultisig != 0) ? msig->NXTaddr : 0);
    flag = 0;
    if ( params != 0 )
    {
        flag = generate_multisigaddr(msig->multisigaddr,msig->redeemScript,coinstr,serverport,userpass,use_addmultisig,params);
        free(params);
    } else printf("error generating msig params\n");
    return(flag);
}

struct multisig_addr *alloc_multisig_addr(char *coinstr,int32_t m,int32_t n,char *NXTaddr,char *userpubkey,char *sender)
{
    struct multisig_addr *msig;
    int32_t size = (int32_t)(sizeof(*msig) + n*sizeof(struct pubkey_info));
    msig = calloc(1,size);
    msig->size = size;
    msig->n = n;
    msig->created = (uint32_t)time(NULL);
    if ( sender != 0 && sender[0] != 0 )
        msig->sender = calc_nxt64bits(sender);
    safecopy(msig->coinstr,coinstr,sizeof(msig->coinstr));
    safecopy(msig->NXTaddr,NXTaddr,sizeof(msig->NXTaddr));
    if ( userpubkey != 0 && userpubkey[0] != 0 )
        safecopy(msig->NXTpubkey,userpubkey,sizeof(msig->NXTpubkey));
    msig->m = m;
    return(msig);
}

struct multisig_addr *get_NXT_msigaddr(struct multisig_addr *msig,uint64_t *srv64bits,int32_t m,int32_t n,uint64_t nxt64bits,char *coinstr,char coinaddrs[][256],char pubkeys[][1024],char *userNXTpubkey,int32_t buyNXT)
{
    uint64_t key[16]; char NXTpubkey[128],NXTaddr[64]; int32_t flag,i,keylen,len; struct coin777 *coin;
    key[0] = stringbits(coinstr);
    for (i=0; i<n; i++)
        key[i+1] = srv64bits[i];
    key[i+1] = nxt64bits;
    keylen = (int32_t)(sizeof(*key) * (i+2));
    len = sizeof(*msig) + 16*sizeof(struct pubkey_info);
    if ( (msig= db777_get(msig,&len,0,DB_msigs,key,keylen)) != 0 )
        return(msig);
    if ( (coin= coin777_find(coinstr,0)) != 0 )
    {
        expand_nxt64bits(NXTaddr,nxt64bits);
        set_NXTpubkey(NXTpubkey,NXTaddr);
        if ( NXTpubkey[0] == 0 && userNXTpubkey[0] != 0 )
            strcpy(NXTpubkey,userNXTpubkey);
        msig = alloc_multisig_addr(coinstr,m,n,NXTaddr,NXTpubkey,0);
        if ( buyNXT > 100 )
            buyNXT = 100;
        msig->buyNXT = buyNXT;
        for (i=0; i<msig->n; i++)
        {
            //printf("i.%d n.%d msig->n.%d NXT.(%s) (%s) (%s)\n",i,n,msig->n,msig->NXTaddr,coinaddrs[i],pubkeys[i]);
            strcpy(msig->pubkeys[i].coinaddr,coinaddrs[i]);
            strcpy(msig->pubkeys[i].pubkey,pubkeys[i]);
            msig->pubkeys[i].nxt64bits = srv64bits[i];
        }
        flag = issue_createmultisig(msig->multisigaddr,msig->redeemScript,coinstr,coin->serverport,coin->userpass,coin->use_addmultisig,msig);
        if ( flag == 0 )
        {
            free(msig);
            return(0);
        }
        save_msigaddr(coinstr,NXTaddr,msig,msig->size);
        if ( db777_add(1,0,DB_msigs,key,keylen,msig,msig->size) != 0 )
            printf("error saving msig.(%s)\n",msig->multisigaddr);
    }
    return(msig);
}

long calc_pubkey_jsontxt(int32_t truncated,char *jsontxt,struct pubkey_info *ptr,char *postfix)
{
    if ( truncated != 0 )
        sprintf(jsontxt,"{\"address\":\"%s\"}%s",ptr->coinaddr,postfix);
    else sprintf(jsontxt,"{\"address\":\"%s\",\"pubkey\":\"%s\",\"srv\":\"%llu\"}%s",ptr->coinaddr,ptr->pubkey,(long long)ptr->nxt64bits,postfix);
    return(strlen(jsontxt));
}

char *create_multisig_jsonstr(struct multisig_addr *msig,int32_t truncated)
{
    long i,len = 0;
    struct coin777 *coin;
    int32_t gatewayid = -1;
    char jsontxt[65536],pubkeyjsontxt[65536],rsacct[64];
    if ( msig != 0 )
    {
        if ( (coin= coin777_find(msig->coinstr,0)) != 0 )
            gatewayid = SUPERNET.gatewayid;
        rsacct[0] = 0;
        conv_rsacctstr(rsacct,calc_nxt64bits(msig->NXTaddr));
        pubkeyjsontxt[0] = 0;
        for (i=0; i<msig->n; i++)
            len += calc_pubkey_jsontxt(truncated,pubkeyjsontxt+strlen(pubkeyjsontxt),&msig->pubkeys[i],(i<(msig->n - 1)) ? ", " : "");
        sprintf(jsontxt,"{%s\"sender\":\"%llu\",\"buyNXT\":%u,\"created\":%u,\"M\":%d,\"N\":%d,\"NXTaddr\":\"%s\",\"NXTpubkey\":\"%s\",\"RS\":\"%s\",\"address\":\"%s\",\"redeemScript\":\"%s\",\"coin\":\"%s\",\"gatewayid\":\"%d\",\"pubkey\":[%s]}",truncated==0?"\"requestType\":\"plugin\",\"plugin\":\"coins\",\"method\":\"setmultisig\",":"",(long long)msig->sender,msig->buyNXT,msig->created,msig->m,msig->n,msig->NXTaddr,msig->NXTpubkey,rsacct,msig->multisigaddr,msig->redeemScript,msig->coinstr,gatewayid,pubkeyjsontxt);
        //if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
        //    printf("(%s) pubkeys len.%ld msigjsonlen.%ld\n",jsontxt,len,strlen(jsontxt));
        return(clonestr(jsontxt));
    }
    else return(0);
}

int32_t ensure_NXT_msigaddr(char *msigjsonstr,char *coinstr,char *NXTaddr,char *userNXTpubkey,int32_t buyNXT)
{
    char buf[8192],coinaddrs[16][256],pubkeys[16][1024],*str;
    int32_t g,m,retval = 0;
    uint64_t nxt64bits;
    struct multisig_addr *msig;
    msigjsonstr[0] = 0;
    nxt64bits = calc_nxt64bits(NXTaddr);
    for (g=m=0; g<SUPERNET.numgateways; g++)
        m += get_NXT_coininfo(MGW.srv64bits[g],nxt64bits,coinstr,coinaddrs[g],pubkeys[g]);
    if ( m == SUPERNET.numgateways && (msig= get_NXT_msigaddr((struct multisig_addr *)buf,MGW.srv64bits,MGW.M,SUPERNET.numgateways,nxt64bits,coinstr,coinaddrs,pubkeys,userNXTpubkey,buyNXT)) != 0 )
    {
        if ( (str= create_multisig_jsonstr(msig,0)) != 0 )
        {
            strcpy(msigjsonstr,str);
            _stripwhite(msigjsonstr,' ');
            printf("ENSURE.(%s)\n",msigjsonstr);
            retval = 1;
            free(str);
        }
        //free(msig);
    }
    return(retval);
}

cJSON *acctpubkey_json(char *coinstr)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"destplugin",cJSON_CreateString("MGW"));
    cJSON_AddItemToObject(json,"method",cJSON_CreateString("myacctpubkeys"));
    cJSON_AddItemToObject(json,"coin",cJSON_CreateString(coinstr));
    cJSON_AddItemToObject(json,"gatewayNXT",cJSON_CreateString(SUPERNET.NXTADDR));
    cJSON_AddItemToObject(json,"gatewayid",cJSON_CreateNumber(SUPERNET.gatewayid));
    return(json);
}

void fix_msigaddr(struct coin777 *coin,char *NXTaddr)
{
    int32_t MGW_publishjson(char *retbuf,cJSON *json);
    cJSON *msig_itemjson(char *account,char *coinaddr,char *pubkey,int32_t allfields);
    cJSON *msigjson,*array; char retbuf[1024],coinaddr[MAX_JSON_FIELD],pubkey[MAX_JSON_FIELD];
    if ( SUPERNET.gatewayid >= 0 )
    {
        get_acct_coinaddr(coinaddr,coin->name,coin->serverport,coin->userpass,NXTaddr);
        get_pubkey(pubkey,coin->name,coin->serverport,coin->userpass,coinaddr);
        printf("new address.(%s) -> (%s) (%s)\n",NXTaddr,coinaddr,pubkey);
        if ( (msigjson= acctpubkey_json(coin->name)) != 0 )
        {
            array = cJSON_CreateArray();
            cJSON_AddItemToArray(array,msig_itemjson(NXTaddr,coinaddr,pubkey,1));
            cJSON_AddItemToObject(msigjson,"pubkeys",array);
            MGW_publishjson(retbuf,msigjson);
            free_json(msigjson);
        }
    }
}

int32_t process_acctpubkey(cJSON *item,int32_t gatewayid,uint64_t gatewaybits)
{
    uint64_t gbits,nxt64bits; int32_t buyNXT,g,count,updated;
    char msigjsonstr[MAX_JSON_FIELD],userNXTpubkey[MAX_JSON_FIELD],NXTaddr[MAX_JSON_FIELD],coinaddr[MAX_JSON_FIELD],pubkey[MAX_JSON_FIELD],coinstr[MAX_JSON_FIELD];
    copy_cJSON(NXTaddr,cJSON_GetObjectItem(item,"userNXT"));
    copy_cJSON(coinaddr,cJSON_GetObjectItem(item,"coinaddr"));
    copy_cJSON(pubkey,cJSON_GetObjectItem(item,"pubkey"));
    copy_cJSON(userNXTpubkey,cJSON_GetObjectItem(item,"userpubkey"));
    buyNXT = get_API_int(cJSON_GetObjectItem(item,"buyNXT"),0);
    g = get_API_int(cJSON_GetObjectItem(item,"gatewayid"),-1);
    gbits = get_API_nxt64bits(cJSON_GetObjectItem(item,"gatewayNXT"));
    if ( g >= 0 )
    {
        if ( g != gatewayid || (gbits != 0 && gbits != gatewaybits) )
        {
            printf("SKIP: g.%d vs gatewayid.%d gbits.%llu vs %llu\n",g,gatewayid,(long long)gbits,(long long)gatewaybits);
            return(0);
        }
    }
    nxt64bits = calc_nxt64bits(NXTaddr);
    updated = add_NXT_coininfo(gatewaybits,nxt64bits,coinstr,coinaddr,pubkey);
    count = ensure_NXT_msigaddr(msigjsonstr,coinstr,NXTaddr,userNXTpubkey,buyNXT);
    return(updated);
}

int32_t process_acctpubkeys(char *retbuf,char *jsonstr,cJSON *json)
{
    cJSON *array; uint64_t gatewaybits; int32_t i,n=0,gatewayid,count = 0,updated = 0;
    char coinstr[MAX_JSON_FIELD],gatewayNXT[MAX_JSON_FIELD];
    struct coin777 *coin;
    if ( SUPERNET.gatewayid >= 0 )
    {
        copy_cJSON(coinstr,cJSON_GetObjectItem(json,"coin"));
        gatewayid = get_API_int(cJSON_GetObjectItem(json,"gatewayid"),-1);
        copy_cJSON(gatewayNXT,cJSON_GetObjectItem(json,"gatewayNXT"));
        gatewaybits = calc_nxt64bits(gatewayNXT);
        coin = coin777_find(coinstr,0);
        if ( (array= cJSON_GetObjectItem(json,"pubkeys")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
        {
            //printf("arraysize.%d\n",n);
            for (i=0; i<n; i++)
                updated += process_acctpubkey(cJSON_GetArrayItem(array,i),gatewayid,gatewaybits);
        }
        sprintf(retbuf,"{\"result\":\"success\",\"coin\":\"%s\",\"updated\":%d,\"total\":%d,\"msigs\":%d}",coinstr,updated,n,count);
        printf("(%s)\n",retbuf);
    }
    return(updated);
}

int32_t MGW_publishjson(char *retbuf,cJSON *json)
{
    char *retstr; int32_t retval;
    retstr = cJSON_Print(json);
    _stripwhite(retstr,' ');
    nn_publish(retstr,1);
    retval = process_acctpubkeys(retbuf,retstr,json);
    free(retstr);
    return(retval);
}

char *devMGW_command(char *jsonstr,cJSON *json)
{
    int32_t i,buyNXT; char userNXTpubkey[MAX_JSON_FIELD],msigjsonstr[MAX_JSON_FIELD],NXTaddr[MAX_JSON_FIELD],*coinstr; struct coin777 *coin;
    if ( SUPERNET.gatewayid >= 0 )
    {
        copy_cJSON(NXTaddr,cJSON_GetObjectItem(json,"userNXT"));
        coinstr = cJSON_str(cJSON_GetObjectItem(json,"coin"));
        copy_cJSON(userNXTpubkey,cJSON_GetObjectItem(json,"userpubkey"));
        buyNXT = get_API_int(cJSON_GetObjectItem(json,"buyNXT"),0);
        if ( NXTaddr[0] != 0 && coinstr != 0 && (coin= coin777_find(coinstr,0)) != 0 )
        {
            for (i=0; i<3; i++)
            {
                if ( ensure_NXT_msigaddr(msigjsonstr,coinstr,NXTaddr,userNXTpubkey,buyNXT) == 0 )
                    fix_msigaddr(coin,NXTaddr), msleep(250);
                else return(clonestr(msigjsonstr));
            }
        }
        sprintf(msigjsonstr,"{\"error\":\"cant find multisig address\",\"coin\":\"%s\",\"userNXT\":\"%s\"}",coinstr!=0?coinstr:"",NXTaddr);
        return(clonestr(msigjsonstr));
    } else return(0);
}

int32_t MGW_publish_acctpubkeys(char *coinstr,char *str)
{
    char retbuf[1024];
    cJSON *json,*array;
    if ( SUPERNET.gatewayid >= 0 && (array= cJSON_Parse(str)) != 0 )
    {
        if ( (json= acctpubkey_json(coinstr)) != 0 )
        {
            cJSON_AddItemToObject(json,"pubkeys",array);
            MGW_publishjson(retbuf,json);
            free_json(json);
            printf("processed.(%s)\n",retbuf);
            return(0);
        }
    }
    return(-1);
}
/*
uint64_t calc_addr_unspent(struct ramchain_info *ram,struct multisig_addr *msig,char *addr,struct rampayload *addrpayload)
{
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
                    _complete_assettxid(tp);
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
                    if ( ram_verify_txstillthere(ram->name,ram->serverport,ram->userpass,txidstr,addrpayload->B.v) != addrpayload->value )
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
    int32_t i,len,numpayloads,n = 0;
    struct rampayload *payloads;
    if ( calc_numunspentp != 0 )
        *calc_numunspentp = 0;
    pending = 0;
    if ( (payloads= ram_addrpayloads(addrptrp,&numpayloads,ram,addr)) != 0 && numpayloads > 0 )
    {
        msig = find_msigaddr(&len,ram->name,0,addr);
        for (i=0; i<numpayloads; i++)
        {
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

uint64_t ram_calc_MGWunspent(uint64_t *pendingp,struct MGW_info *mgw)
{
    struct multisig_addr **msigs;
    int32_t i,n,m;
    uint64_t pending,smallest,val,unspent = 0;
    pending = 0;
    mgw->numunspents = 0;
    if ( mgw->unspents != 0 )
        memset(mgw->unspents,0,sizeof(*mgw->unspents) * mgw->maxunspents);
    if ( (msigs= (struct multisig_addr **)db777_copy_all(&n,DB_msigs,0)) != 0 )
    {
        ram->nummsigs = n;
        mgw->smallest[0] = mgw->smallestB[0] = 0;
        for (smallest=i=m=0; i<n; i++)
        {
            if ( (val= mgw_calc_unspent(&pending,0,0,ram,msigs[i]->multisigaddr,1)) != 0 )
            {
                unspent += val;
                if ( smallest == 0 || val < smallest )
                {
                    smallest = val;
                    strcpy(mgw->smallestB,mgw->smallest);
                    strcpy(mgw->smallest,msigs[i]->multisigaddr);
                }
                else if ( mgw->smallestB[0] == 0 && strcmp(mgw->smallest,msigs[i]->multisigaddr) != 0 )
                    strcpy(mgw->smallestB,msigs[i]->multisigaddr);
            }
            free(msigs[i]);
        }
        free(msigs);
        if ( Debuglevel > 2 )
            printf("MGWnumunspents.%d smallest (%s %.8f)\n",mgw->numunspents,mgw->smallest,dstr(smallest));
    }
    *pendingp = pending;
    return(unspent);
}
*/

int32_t make_MGWbus(uint16_t port,char *bindaddr,char serverips[MAX_MGWSERVERS][64],int32_t n)
{
    char tcpaddr[64];
    int32_t i,err,sock,timeout = 1;
    if ( (sock= nn_socket(AF_SP,NN_BUS)) < 0 )
    {
        printf("error getting socket.%d %s\n",sock,nn_strerror(nn_errno()));
        return(-1);
    }
    if ( bindaddr != 0 && bindaddr[0] != 0 )
    {
        sprintf(tcpaddr,"tcp://%s:%d",bindaddr,port);
        printf("MGW bind.(%s)\n",tcpaddr);
        if ( (err= nn_bind(sock,tcpaddr)) < 0 )
        {
            printf("error binding socket.%d %s\n",sock,nn_strerror(nn_errno()));
            return(-1);
        }
        if ( (err= nn_connect(sock,tcpaddr)) < 0 )
        {
            printf("error nn_connect (%s <-> %s) socket.%d %s\n",bindaddr,tcpaddr,sock,nn_strerror(nn_errno()));
            return(-1);
        }
        if ( timeout > 0 && nn_setsockopt(sock,NN_SOL_SOCKET,NN_RCVTIMEO,&timeout,sizeof(timeout)) < 0 )
        {
            printf("error nn_setsockopt socket.%d %s\n",sock,nn_strerror(nn_errno()));
            return(-1);
        }
        for (i=0; i<n; i++)
        {
            //if ( strcmp(bindaddr,serverips[i]) != 0 )
            {
                sprintf(tcpaddr,"tcp://%s:%d",serverips[i],port);
                printf("conn.(%s) ",tcpaddr);
                if ( (err= nn_connect(sock,tcpaddr)) < 0 )
                {
                    printf("error nn_connect (%s <-> %s) socket.%d %s\n",bindaddr,tcpaddr,sock,nn_strerror(nn_errno()));
                    return(-1);
                }
            }
        }
    } else nn_shutdown(sock,0), sock = -1;
    return(sock);
}

int32_t PLUGNAME(_process_json)(struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag)
{
    char NXTaddr[64],nxtaddr[64],ipaddr[64],*resultstr,*coinstr,*methodstr,*retstr = 0; int32_t i,j,n; cJSON *array; uint64_t nxt64bits;
    retbuf[0] = 0;
    printf("<<<<<<<<<<<< INSIDE PLUGIN! process %s\n",plugin->name);
    if ( initflag > 0 )
    {
        strcpy(retbuf,"{\"result\":\"return JSON init\"}");
        MGW.issuers[MGW.numissuers++] = calc_nxt64bits("423766016895692955");//conv_rsacctstr("NXT-JXRD-GKMR-WD9Y-83CK7",0);
        MGW.issuers[MGW.numissuers++] = calc_nxt64bits("12240549928875772593");//conv_rsacctstr("NXT-3TKA-UH62-478B-DQU6K",0);
        MGW.issuers[MGW.numissuers++] = calc_nxt64bits("8279528579993996036");//conv_rsacctstr("NXT-5294-T9F6-WAWK-9V7WM",0);
        if ( (array= cJSON_GetObjectItem(json,"issuers")) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
        {
            for (i=0; i<n; i++)
            {
                copy_cJSON(NXTaddr,cJSON_GetArrayItem(array,i));
                nxt64bits = calc_nxt64bits(NXTaddr);//conv_rsacctstr(NXTaddr,0);
                for (j=0; j<MGW.numissuers; j++)
                    if ( nxt64bits == MGW.issuers[j] )
                        break;
                if ( j == MGW.numissuers )
                    MGW.issuers[MGW.numissuers++] = nxt64bits;
            }
        }
        MGW.port = get_API_int(cJSON_GetObjectItem(json,"MGWport"),7643);
        MGW.M = get_API_int(cJSON_GetObjectItem(json,"M"),2);
        if ( (array= cJSON_GetObjectItem(json,"MGWservers")) != 0 && (n= cJSON_GetArraySize(array)) > 0 && (n & 1) == 0 )
        {
            for (i=j=0; i<n/2&&i<MAX_MGWSERVERS; i++)
            {
                copy_cJSON(ipaddr,cJSON_GetArrayItem(array,i<<1));
                copy_cJSON(nxtaddr,cJSON_GetArrayItem(array,(i<<1)+1));
                if ( strcmp(ipaddr,MGW.bridgeipaddr) != 0 )
                {
                    MGW.srv64bits[j] = calc_nxt64bits(nxtaddr);//conv_rsacctstr(nxtaddr,0);
                    strcpy(MGW.serverips[j],ipaddr);
                    printf("%d.(%s).%llu ",j,ipaddr,(long long)MGW.srv64bits[j]);
                    j++;
                }
            }
            printf("MGWipaddrs: %s %s %s\n",MGW.serverips[0],MGW.serverips[1],MGW.serverips[2]);
            if ( SUPERNET.gatewayid >= 0 && SUPERNET.numgateways )
            {
                strcpy(SUPERNET.myipaddr,MGW.serverips[SUPERNET.gatewayid]);
            }
            //printf("j.%d M.%d N.%d n.%d (%s).%s gateway.%d\n",j,COINS.M,COINS.N,n,COINS.myipaddr,COINS.myNXTaddr,COINS.gatewayid);
            if ( j != SUPERNET.numgateways )
                sprintf(retbuf+1,"{\"warning\":\"mismatched servers\",\"details\":\"n.%d j.%d vs M.%d N.%d\",",n,j,MGW.M,SUPERNET.numgateways);
            else if ( SUPERNET.gatewayid >= 0 )
            {
                strcpy(MGW.serverips[SUPERNET.numgateways],MGW.bridgeipaddr);
                MGW.srv64bits[SUPERNET.numgateways] = calc_nxt64bits(MGW.bridgeacct);
                MGW.all.socks.both.bus = make_MGWbus(MGW.port,SUPERNET.myipaddr,MGW.serverips,SUPERNET.numgateways+1);
            }
        }
        MGW.readyflag = 1;
        plugin->allowremote = 1;
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        methodstr = cJSON_str(cJSON_GetObjectItem(json,"method"));
        coinstr = cJSON_str(cJSON_GetObjectItem(json,"coin"));
        if ( methodstr == 0 || methodstr[0] == 0 )
        {
            printf("(%s) has not method\n",jsonstr);
            return(0);
        }
        printf("MGW.(%s) for (%s)\n",methodstr,coinstr!=0?coinstr:"");
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
        else if ( strcmp(methodstr,"myacctpubkeys") == 0 )
            process_acctpubkeys(retbuf,jsonstr,json);
        if ( retstr != 0 )
        {
            strcpy(retbuf,retstr);
            free(retstr);
        }
    }
    return((int32_t)strlen(retbuf));
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    return(retcode);
}
#include "../plugin777.c"
