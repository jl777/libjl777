/******************************************************************************
 * Copyright © 2014-2015 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * Nxt software, including this file, may be copied, modified, propagated,    *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

// add to limbo:  txids 204961939803594792, 7301360590217481477, 14387806392702706073 for a total of 568.1248 BTCD
// 17638509709909095430 ~170
// http://chain.explorebtcd.info/tx/1dc0faf122e64aa46393291f00483f7779b73504573c1776301347545b760ea9

#ifdef INSIDE_MGW
#define DEPOSIT_XFER_DURATION 30
#define MIN_DEPOSIT_FACTOR 5

#define BUNDLED
#define PLUGINSTR "MGW"
#define PLUGNAME(NAME) MGW ## NAME
#define STRUCTNAME struct PLUGNAME(_info)
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
#include "../agents/plugin777.c"
#include "../KV/kv777.c"
#include "../common/system777.c"
#include "../utils/NXT777.c"
#include "../ramchain/ramchain.c"
#include "old/db777.c"
#undef DEFINES_ONLY

int32_t MGW_idle(struct plugin_info *plugin)
{
    return(0);
}
char *fix_msigaddr(struct coin777 *coin,char *NXTaddr,char *method);

STRUCTNAME MGW;
char *PLUGNAME(_methods)[] = { "myacctpubkeys", "myacctpubkey", "askacctpubkey", "msigaddr", "status", "findmsigaddr" };
char *PLUGNAME(_pubmethods)[] = { "myacctpubkeys", "myacctpubkey", "askacctpubkey", "msigaddr", "status", "findmsigaddr" };
char *PLUGNAME(_authmethods)[] = { "myacctpubkeys", "myacctpubkey", "askacctpubkey", "msigaddr", "status" };

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME   *data,cJSON *json)
{
    uint64_t disableflags = 0;
    printf("init %s size.%ld\n",plugin->name,sizeof(struct MGW_info));
    return(disableflags); // set bits corresponding to array position in _methods[]
}

int32_t NXT_revassettxid(struct extra_info *extra,uint64_t assetidbits,uint32_t ind)
{
    void *obj,*result,*value; uint64_t revkey[2]; int32_t len = 0;
    memset(extra,0,sizeof(*extra));
    if ( (obj= sp_object(DB_NXTtxids->db)) != 0 )
    {
        revkey[0] = assetidbits, revkey[1] = ind;
        if ( sp_set(obj,"key",revkey,sizeof(revkey)) == 0 && (result= sp_get(DB_NXTtxids->db,obj)) != 0 )
        {
            value = sp_get(result,"value",&len);
            if ( len == sizeof(*extra) )
                memcpy(extra,value,len);
            else printf("NXT_revassettxid mismatched len.%d vs %ld\n",len,sizeof(*extra));
            sp_destroy(result);
        } //else sp_destroy(obj);
    }
    return(len);
}

int32_t NXT_add_assettxid(uint64_t assetidbits,uint64_t txidbits,void *value,int32_t valuelen,uint32_t ind,struct extra_info *extra)
{
    void *obj;
    if ( value != 0 )
    {
        if ( (obj= sp_object(DB_NXTtxids->db)) != 0 )
        {
            extra->assetidbits = assetidbits, extra->txidbits = txidbits, extra->ind = ind;
            if ( sp_set(obj,"key",&txidbits,sizeof(txidbits)) == 0 && sp_set(obj,"value",value,valuelen) == 0 )
                sp_set(DB_NXTtxids->db,obj);
            else
            {
                sp_destroy(obj);
                printf("error NXT_add_assettxid %llu ind.%d\n",(long long)txidbits,ind);
            }
        }
        NXT_set_revassettxid(assetidbits,ind,extra);
    }
    return(0);
}

char *NXT_txidstr(struct mgw777 *mgw,char *txid,int32_t writeflag,uint32_t ind)
{
    void *obj,*value,*result = 0; int32_t slen,len,flag; uint64_t txidbits,savedbits; struct extra_info extra; char *txidjsonstr = 0; cJSON *json,*txobj;
    printf("NXT_txidstr.(%s) write.%d ind.%d\n",txid,writeflag,ind);
    if ( txid[0] != 0 && (txidjsonstr= _issue_getTransaction(txid)) != 0 )
    {
        flag = writeflag;
        if ( (json= cJSON_Parse(txidjsonstr)) != 0 )
        {
            free(txidjsonstr);
            cJSON_DeleteItemFromObject(json,"requestProcessingTime");
            cJSON_DeleteItemFromObject(json,"confirmations");
            cJSON_DeleteItemFromObject(json,"transactionIndex");
            txidjsonstr = cJSON_Print(json);
            free_json(json);
        } else printf("PARSE ERROR.(%s)\n",txidjsonstr);
        _stripwhite(txidjsonstr,' ');
        slen = (int32_t)strlen(txidjsonstr)+1;
        txidbits = calc_nxt64bits(txid);
        if ( (obj= sp_object(DB_NXTtxids->db)) != 0 )
        {
            if ( sp_set(obj,"key",&txidbits,sizeof(txidbits)) == 0 && (result= sp_get(DB_NXTtxids->db,obj)) != 0 )
            {
                value = sp_get(result,"value",&len);
                if ( value != 0 )
                {
                    if ( len != slen || strcmp(value,txidjsonstr) != 0 )
                        printf("mismatched NXT_txidstr ind.%d for %llu: lens %d vs %d (%s) vs (%s)\n",ind,(long long)txidbits,slen,len,txidjsonstr,value);
                    else flag = 0;
                }
                sp_destroy(result);
            } //else sp_destroy(obj);
        }
        if ( flag != 0 )
        {
            int32_t mgw_markunspent(char *txidstr,int32_t vout,int32_t status);
            NXT_revassettxid(&extra,mgw->assetidbits,ind);
            savedbits = extra.txidbits;
            memset(&extra,0,sizeof(extra));
            if ( (txobj= cJSON_Parse(txidjsonstr)) != 0 )
            {
                int32_t process_assettransfer(uint32_t *heightp,uint64_t *senderbitsp,uint64_t *receiverbitsp,uint64_t *amountp,int32_t *flagp,char *coindata,int32_t confirmed,struct mgw777 *mgw,cJSON *txobj);
                extra.vout = process_assettransfer(&extra.height,&extra.senderbits,&extra.receiverbits,&extra.amount,&extra.flags,extra.coindata,0,mgw,txobj);
                free_json(txobj);
                if ( extra.vout >= 0 )
                {
                    mgw_markunspent(extra.coindata,extra.vout,MGW_DEPOSITDONE);
                    printf("MARK DEPOSITDONE %llu.%d oldval.%llu -> newval flags.%d %llu (%s v%d %.8f)\n",(long long)mgw->assetidbits,ind,(long long)savedbits,extra.flags,(long long)txidbits,extra.coindata,extra.vout,dstr(extra.amount));
                }
            } else extra.vout = -1;
            printf("for %llu.%d oldval.%llu -> newval flags.%d %llu (%s v%d %.8f)\n",(long long)mgw->assetidbits,ind,(long long)savedbits,extra.flags,(long long)txidbits,extra.coindata,extra.vout,dstr(extra.amount));
            NXT_add_assettxid(mgw->assetidbits,txidbits,txidjsonstr,slen,ind,&extra);
        }
    }
    return(txidjsonstr);
}

int32_t NXT_assettransfers(struct mgw777 *mgw,uint64_t *txids,long max,int32_t firstindex,int32_t lastindex)
{
    char cmd[1024],txid[64],*jsonstr,*txidstr; cJSON *transfers,*array;
    int32_t i,n = 0; uint64_t txidbits,revkey[2];
    sprintf(cmd,"requestType=getAssetTransfers&asset=%s",mgw->assetidstr);
    if ( firstindex >= 0 && lastindex >= firstindex )
        sprintf(cmd + strlen(cmd),"&firstIndex=%u&lastIndex=%u",firstindex,lastindex);
    revkey[0] = mgw->assetidbits;
    //printf("issue.(%s) max.%ld\n",cmd,max);
    if ( (jsonstr= issue_NXTPOST(cmd)) != 0 )
    {
        //printf("(%s) -> (%s)\n",cmd,jsonstr);
        if ( (transfers = cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(transfers,"transfers")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    copy_cJSON(txid,cJSON_GetObjectItem(cJSON_GetArrayItem(array,i),"assetTransfer"));
                    if ( txid[0] != 0 && (txidbits= calc_nxt64bits(txid)) != 0 )
                    {
                        if ( i < max )
                            txids[i] = txidbits;
                        if ( firstindex < 0 && lastindex <= firstindex )
                        {
                            if ( (txidstr= NXT_txidstr(mgw,txid,1,n - i)) != 0 )
                                free(txidstr);
                        }
                    }
                }
            } free_json(transfers);
        } free(jsonstr);
    }
    //if ( firstindex < 0 || lastindex <= firstindex )
    //    printf("assetid.(%s) -> %d entries\n",mgw->assetidstr,n);
    return(n);
}

int32_t NXT_mark_withdrawdone(struct mgw777 *mgw,uint64_t redeemtxid)
{
    int32_t i,count; struct extra_info extra;
    if ( NXT_revassettxid(&extra,mgw->assetidbits,0) == sizeof(extra) )
    {
        //printf("got extra ind.%d\n",extra.ind);
        count = extra.ind;
        for (i=1; i<=count; i++)
        {
            NXT_revassettxid(&extra,mgw->assetidbits,i);
            if ( extra.txidbits == redeemtxid != 0 && (extra.flags & MGW_PENDINGREDEEM) != 0 && (extra.flags & MGW_WITHDRAWDONE) == 0 )
            {
                extra.flags |= MGW_WITHDRAWDONE;
                printf("NXT_mark_withdrawdone %s.%llu %.8f\n",mgw->coinstr,(long long)redeemtxid,dstr(extra.amount));
                NXT_set_revassettxid(mgw->assetidbits,i,&extra);
                return(i);
            }
            //fprintf(stderr,"%llu.%d ",(long long)extra.txidbits,extra.flags);
        }
    }
    return(-1);
}

int32_t update_NXT_assettransfers(struct mgw777 *mgw)
{
    int32_t len,verifyflag = 0;
    uint64_t txids[100],mostrecent; int32_t i,count = 0; char txidstr[128],nxt_txid[64],*txidjsonstr; struct extra_info extra;
    mgw->assetidbits = calc_nxt64bits(mgw->assetidstr);
    mgw->withdrawsum = mgw->numwithdraws = 0;
    if ( (len= NXT_revassettxid(&extra,mgw->assetidbits,0)) == sizeof(extra) )
    {
        //printf("got extra ind.%d\n",extra.ind);
        count = extra.ind;
        for (i=1; i<=count; i++)
        {
            NXT_revassettxid(&extra,mgw->assetidbits,i);
            if ( SUPERNET.gatewayid >= 0 && (extra.flags & MGW_PENDINGREDEEM) != 0 && (extra.flags & MGW_WITHDRAWDONE) == 0 )
            {
                int32_t mgw_update_redeem(struct mgw777 *mgw,struct extra_info *extra);
                expand_nxt64bits(nxt_txid,extra.txidbits);
                if ( in_jsonarray(mgw->limbo,nxt_txid) != 0 || mgw_update_redeem(mgw,&extra) != 0 )
                {
                    extra.flags |= MGW_WITHDRAWDONE;
                    NXT_set_revassettxid(mgw->assetidbits,i,&extra);
                }
            }
            //fprintf(stderr,"%llu.%d ",(long long)extra.txidbits,extra.flags);
        }
        //fprintf(stderr,"sequential tx.%d\n",count);
        NXT_revassettxid(&extra,mgw->assetidbits,count);
        mostrecent = extra.txidbits;
        for (i=0; i<sizeof(txids)/sizeof(*txids); i++)
        {
            if ( NXT_assettransfers(mgw,&txids[i],1,i,i) == 1 && txids[i] == mostrecent )
            {
                if ( i != 0 )
                    printf("asset.(%s) count.%d i.%d mostrecent.%llu vs %llu\n",mgw->assetidstr,count,i,(long long)mostrecent,(long long)txids[i]);
                while ( i-- > 0 )
                {
                    expand_nxt64bits(txidstr,txids[i]);
                    if ( (txidjsonstr= NXT_txidstr(mgw,txidstr,1,++count)) != 0 )
                        free(txidjsonstr);
                }
                break;
            }
        }
        if ( i == 100 )
            count = 0;
    } else printf("cant get count len.%d\n",len);
    if ( count == 0 )
        count = NXT_assettransfers(mgw,txids,sizeof(txids)/sizeof(*txids) - 1,-1,-1);
    if ( NXT_revassettxid(&extra,mgw->assetidbits,0) != sizeof(extra) || extra.ind != count )
    {
        memset(&extra,0,sizeof(extra));
        extra.ind = count;
        NXT_set_revassettxid(mgw->assetidbits,0,&extra);
    }
    if ( verifyflag != 0 )
        NXT_assettransfers(mgw,txids,sizeof(txids)/sizeof(*txids) - 1,-1,-1);
    return(count);
}

int32_t NXT_set_revassettxid(uint64_t assetidbits,uint32_t ind,struct extra_info *extra)
{
    uint64_t revkey[2]; void *obj;
    if ( (obj= sp_object(DB_NXTtxids->db)) != 0 )
    {
        revkey[0] = assetidbits, revkey[1] = ind;
        //printf("set ind.%d <- txid.%llu\n",ind,(long long)extra->txidbits);
        if ( sp_set(obj,"key",revkey,sizeof(revkey)) == 0 && sp_set(obj,"value",extra,sizeof(*extra)) == 0 )
            return(sp_set(DB_NXTtxids->db,obj));
        else
        {
            sp_destroy(obj);
            printf("error NXT_add_assettxid rev %llu ind.%d\n",(long long)extra->txidbits,ind);
        }
    }
    return(-1);
}

char *NXT_assettxid(uint64_t assettxid)
{
    void *obj,*result,*value; int32_t len; char *retstr = 0;
    if ( (obj= sp_object(DB_NXTtxids->db)) != 0 )
    {
        if ( sp_set(obj,"key",&assettxid,sizeof(assettxid)) == 0 && (result= sp_get(DB_NXTtxids->db,obj)) != 0 )
        {
            value = sp_get(result,"value",&len);
            retstr = clonestr(value);
            sp_destroy(result);
        }// else sp_destroy(obj);
    }
    return(retstr);
}

int32_t get_NXT_coininfo(uint64_t srvbits,uint64_t nxt64bits,char *coinstr,char *coinaddr,char *pubkey)
{
    uint64_t key[3]; char *keycoinaddr,buf[256]; int32_t flag,len = sizeof(buf);
    key[0] = stringbits(coinstr);
    key[1] = srvbits;
    key[2] = nxt64bits;
    flag = 0;
    coinaddr[0] = pubkey[0] = 0;
    if ( (keycoinaddr= db777_read(buf,&len,0,DB_NXTaccts,key,sizeof(key),0)) != 0 )
    {
        strcpy(coinaddr,keycoinaddr);
        //free(keycoinaddr);
    }
    if ( coinaddr[0] != 0 )
        db777_findstr(pubkey,512,DB_NXTaccts,coinaddr);
//printf("(%llu %llu) get.(%s) -> (%s)\n",(long long)srvbits,(long long)nxt64bits,coinaddr,pubkey);
    return(coinaddr[0] != 0 && pubkey[0] != 0);
}

int32_t add_NXT_coininfo(uint64_t srvbits,uint64_t nxt64bits,char *coinstr,char *newcoinaddr,char *newpubkey)
{
    uint64_t key[3]; char *coinaddr,pubkey[513],buf[1024]; int32_t len = sizeof(buf),flag,updated = 0;
    key[0] = stringbits(coinstr);
    key[1] = srvbits;
    key[2] = nxt64bits;
    flag = 1;
    if ( (coinaddr= db777_read(buf,&len,0,DB_NXTaccts,key,sizeof(key),0)) != 0 )
    {
        if ( newcoinaddr[0] != 0 && strcmp(coinaddr,newcoinaddr) == 0 )
            flag = 0;
    }
    if ( flag != 0 )
    {
        if ( db777_write(0,DB_NXTaccts,key,sizeof(key),newcoinaddr,(int32_t)strlen(newcoinaddr)+1) == 0 )
            updated = 1;
        else printf("error adding (%s)\n",newcoinaddr);
    }
    flag = 1;
    if ( db777_findstr(pubkey,sizeof(pubkey),DB_NXTaccts,newcoinaddr) > 0 )
    {
        if ( newpubkey[0] != 0 && strcmp(pubkey,newpubkey) == 0 )
            flag = 0;
    }
    //printf("(%llu %llu) add.(%s) -> (%s) flag.%d\n",(long long)srvbits,(long long)nxt64bits,newcoinaddr,newpubkey,flag);
    if ( flag != 0 )
    {
        if ( db777_addstr(DB_NXTaccts,newcoinaddr,newpubkey) == 0 )
            updated = 1;//, printf("added (%s)\n",newpubkey);
        else printf("error adding (%s)\n",newpubkey);
    }
    return(updated);
}

struct multisig_addr *find_msigaddr(struct multisig_addr *msig,int32_t *lenp,char *coinstr,char *multisigaddr)
{
    char keystr[1024];
    sprintf(keystr,"%s.%s",coinstr,multisigaddr);
    //printf("search_msig.(%s)\n",keystr);
    return(db777_read(msig,lenp,0,DB_msigs,keystr,(int32_t)strlen(keystr)+1,0));
}

int32_t save_msigaddr(char *coinstr,char *NXTaddr,struct multisig_addr *msig)
{
    char keystr[1024];
    sprintf(keystr,"%s.%s",coinstr,msig->multisigaddr);
    printf("save_msig.(%s)\n",keystr);
    return(db777_write(0,DB_msigs,keystr,(int32_t)strlen(keystr)+1,msig,msig->size));
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
    redeemScript[0] = normaladdr[0] = 0; len = sizeof(buf);
    if ( (msig= find_msigaddr((struct multisig_addr *)buf,&len,coinstr,msigaddr)) == 0 )
    {
        strcpy(normaladdr,msigaddr);
        if ( Debuglevel > 2 )
            printf("cant find_msigaddr.(%s %s)\n",coinstr,msigaddr);
        return(0);
    }
    if ( msig->redeemScript[0] != 0 && gatewayid >= 0 && gatewayid < numgateways )
    {
        strcpy(normaladdr,msig->pubkeys[gatewayid].coinaddr);
        strcpy(redeemScript,msig->redeemScript);
        if ( Debuglevel > 2 )
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
    msig->sig = stringbits("multisig");
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

struct multisig_addr *get_NXT_msigaddr(uint64_t *srv64bits,int32_t m,int32_t n,uint64_t nxt64bits,char *coinstr,char coinaddrs[][256],char pubkeys[][1024],char *userNXTpubkey,int32_t buyNXT)
{
    uint64_t key[16]; char NXTpubkey[128],NXTaddr[64],multisigaddr[128],databuf[8192]; int32_t flag,i,keylen,len; struct coin777 *coin;
    struct multisig_addr *msig = 0;
    //printf("get_NXT_msig %llu (%s)\n",(long long)nxt64bits,coinstr);
    expand_nxt64bits(NXTaddr,nxt64bits);
    set_NXTpubkey(NXTpubkey,NXTaddr);
    if ( NXTpubkey[0] == 0 && userNXTpubkey != 0 && userNXTpubkey[0] != 0 )
        strcpy(NXTpubkey,userNXTpubkey);
    key[0] = stringbits(coinstr);
    for (i=0; i<n; i++)
        key[i+1] = srv64bits[i];
    key[i+1] = nxt64bits;
    keylen = (int32_t)(sizeof(*key) * (i+2));
    len = sizeof(multisigaddr);
    if ( db777_read(multisigaddr,&len,0,DB_msigs,key,keylen,0) != 0 )
    {
        len = sizeof(databuf);
        if ( (msig= find_msigaddr((void *)databuf,&len,coinstr,multisigaddr)) != 0 )
        {
            printf("found msig for NXT.%llu -> (%s)\n",(long long)nxt64bits,msig->multisigaddr);
            return(msig);
        }
    }
    msig = alloc_multisig_addr(coinstr,m,n,NXTaddr,NXTpubkey,0);
    memset(databuf,0,sizeof(databuf)), memcpy(databuf,msig,msig->size), free(msig), msig = (struct multisig_addr *)databuf;
    if ( (coin= coin777_find(coinstr,0)) != 0 )
    {
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
        flag = issue_createmultisig(msig->multisigaddr,msig->redeemScript,coinstr,coin->serverport,coin->userpass,coin->mgw.use_addmultisig,msig);
        if ( flag == 0 )
            return(0);
        save_msigaddr(coinstr,NXTaddr,msig);
        if ( db777_write(0,DB_msigs,key,keylen,msig->multisigaddr,(int32_t)strlen(msig->multisigaddr)+1) != 0 )
            printf("error saving msig.(%s)\n",msig->multisigaddr);
    } else printf("cant find coin.(%s)\n",coinstr);
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
    char jsontxt[8192],pubkeyjsontxt[8192],rsacct[64];
    if ( msig != 0 )
    {
        //printf("create_multisig %s\n",msig->coinstr);
        if ( (coin= coin777_find(msig->coinstr,0)) != 0 )
            gatewayid = SUPERNET.gatewayid;
        rsacct[0] = 0;
        conv_rsacctstr(rsacct,calc_nxt64bits(msig->NXTaddr));
        pubkeyjsontxt[0] = 0;
        for (i=0; i<msig->n; i++)
            len += calc_pubkey_jsontxt(truncated,pubkeyjsontxt+strlen(pubkeyjsontxt),&msig->pubkeys[i],(i<(msig->n - 1)) ? ", " : "");
        sprintf(jsontxt,"{%s\"NXT\":\"%s\",\"serviceNXT\":\"%s\",\"buyNXT\":%u,\"created\":%u,\"M\":%d,\"N\":%d,\"NXTaddr\":\"%s\",\"NXTpubkey\":\"%s\",\"RS\":\"%s\",\"address\":\"%s\",\"redeemScript\":\"%s\",\"coin\":\"%s\",\"gatewayid\":\"%d\",\"pubkey\":[%s]}",truncated==0?"\"requestType\":\"plugin\",\"plugin\":\"coins\",\"method\":\"gotmsigaddr\",":"",SUPERNET.NXTADDR,SUPERNET.SERVICENXT,msig->buyNXT,msig->created,msig->m,msig->n,msig->NXTaddr,msig->NXTpubkey,rsacct,msig->multisigaddr,msig->redeemScript,msig->coinstr,gatewayid,pubkeyjsontxt);
        //if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
        //    printf("(%s) pubkeys len.%ld msigjsonlen.%ld\n",jsontxt,len,strlen(jsontxt));
        //printf("-> (%s)\n",jsontxt);
        return(clonestr(jsontxt));
    }
    else return(0);
}

int32_t ensure_NXT_msigaddr(char *msigjsonstr,char *coinstr,char *NXTaddr,char *userNXTpubkey,int32_t buyNXT)
{
    char coinaddrs[16][256],pubkeys[16][1024],*str; struct coin777 *coin;
    int32_t g,m,mask,retval = 0;
    uint64_t nxt64bits;
    struct multisig_addr *msig;
    msigjsonstr[0] = 0;
    nxt64bits = calc_nxt64bits(NXTaddr);
    for (g=m=mask=0; g<SUPERNET.numgateways; g++)
    {
        if ( g == SUPERNET.gatewayid && (coin= coin777_find(coinstr,0)) != 0 )
        {
            get_acct_coinaddr(coinaddrs[g],coin->name,coin->serverport,coin->userpass,NXTaddr);
            get_pubkey(pubkeys[g],coin->name,coin->serverport,coin->userpass,coinaddrs[g]);
            printf("ensure_NXT_msigaddr %s new address.(%s) -> (%s) (%s)\n",coin->name,NXTaddr,coinaddrs[g],pubkeys[g]);
            add_NXT_coininfo(MGW.srv64bits[g],nxt64bits,coinstr,coinaddrs[g],pubkeys[g]);
        }
        if ( get_NXT_coininfo(MGW.srv64bits[g],nxt64bits,coinstr,coinaddrs[g],pubkeys[g]) != 0 )
            m++, mask |= (1 << g);
    }
    //printf("m.%d ensure.(%s)\n",m,coinstr);
    if ( m == SUPERNET.numgateways && (msig= get_NXT_msigaddr(MGW.srv64bits,MGW.M,SUPERNET.numgateways,nxt64bits,coinstr,coinaddrs,pubkeys,userNXTpubkey,buyNXT)) != 0 )
    {
        if ( (str= create_multisig_jsonstr(msig,0)) != 0 )
        {
            strcpy(msigjsonstr,str);
            _stripwhite(msigjsonstr,' ');
            nn_send(MGW.all.socks.both.bus,(uint8_t *)msigjsonstr,(int32_t)strlen(msigjsonstr)+1,0);
            //nn_publish((uint8_t *)msigjsonstr,(int32_t)strlen(msigjsonstr)+1,1);
            printf("ENSURE.(%s)\n",msigjsonstr);
            retval = 1;
            free(str);
        } else printf("error creating msigaddr\n");
        //free(msig);
    } else printf("m.%d mask.%d vs numgateways.%d\n",m,mask,SUPERNET.numgateways);
    return(retval);
}

int32_t process_acctpubkey(int32_t *havemsigp,cJSON *item,int32_t gatewayid,uint64_t gatewaybits)
{
    uint64_t gbits,nxt64bits; int32_t buyNXT,g,updated = 0;
    char msigjsonstr[MAX_JSON_FIELD],userNXTpubkey[MAX_JSON_FIELD],NXTaddr[MAX_JSON_FIELD],coinaddr[MAX_JSON_FIELD],pubkey[MAX_JSON_FIELD],coinstr[MAX_JSON_FIELD];
    *havemsigp = 0;
    copy_cJSON(coinstr,cJSON_GetObjectItem(item,"coin"));
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
            printf("SKIP: SUPERNET.gatewayid %d g.%d vs gatewayid.%d gbits.%llu vs %llu %s\n",SUPERNET.gatewayid,g,gatewayid,(long long)gbits,(long long)gatewaybits,cJSON_Print(item));
            return(0);
        }
    }
    if ( (nxt64bits= conv_acctstr(NXTaddr)) != 0 )
    {
        //printf("%s.G%d +(%s %s): ",coinstr,g,coinaddr,pubkey);
        updated = add_NXT_coininfo(gatewaybits,nxt64bits,coinstr,coinaddr,pubkey);
        *havemsigp = ensure_NXT_msigaddr(msigjsonstr,coinstr,NXTaddr,userNXTpubkey,buyNXT);
    }
    return(updated);
}

cJSON *mgw_stdjson(char *coinstr,char *NXTaddr,int32_t gatewayid,char *method)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"agent",cJSON_CreateString("MGW"));
    cJSON_AddItemToObject(json,"method",cJSON_CreateString(method));
    cJSON_AddItemToObject(json,"coin",cJSON_CreateString(coinstr));
    cJSON_AddItemToObject(json,"gatewayNXT",cJSON_CreateString(NXTaddr));
    cJSON_AddItemToObject(json,"gatewayid",cJSON_CreateNumber(gatewayid));
    return(json);
}

cJSON *msig_itemjson(char *coinstr,char *account,char *coinaddr,char *pubkey,int32_t allfields)
{
    cJSON *item = cJSON_CreateObject();
    cJSON_AddItemToObject(item,"coin",cJSON_CreateString(coinstr));
    cJSON_AddItemToObject(item,"userNXT",cJSON_CreateString(account));
    if ( coinaddr[0] != 0 )
        cJSON_AddItemToObject(item,"coinaddr",cJSON_CreateString(coinaddr));
    if ( pubkey[0] != 0 )
        cJSON_AddItemToObject(item,"pubkey",cJSON_CreateString(pubkey));
    if ( allfields != 0 && SUPERNET.gatewayid >= 0 )
    {
        cJSON_AddItemToObject(item,"gatewayNXT",cJSON_CreateString(SUPERNET.NXTADDR));
        cJSON_AddItemToObject(item,"gatewayid",cJSON_CreateNumber(SUPERNET.gatewayid));
    }
    //printf("(%s)\n",cJSON_Print(item));
    return(item);
}

char *fix_msigaddr(struct coin777 *coin,char *NXTaddr,char *method)
{
    cJSON *msigjson,*array; struct destbuf coinaddr,pubkey; char *retstr = 0; uint64_t nxt64bits;
    if ( coin != 0 && NXTaddr != 0 && NXTaddr[0] != 0 && SUPERNET.gatewayid >= 0 )
    {
        nxt64bits = conv_acctstr(NXTaddr), expand_nxt64bits(NXTaddr,nxt64bits);
        get_acct_coinaddr(coinaddr,coin->name,coin->serverport,coin->userpass,NXTaddr);
        get_pubkey(&pubkey,coin->name,coin->serverport,coin->userpass,coinaddr);
        printf("%s new address.(%s) -> (%s) (%s)\n",coin->name,NXTaddr,coinaddr,pubkey.buf);
        if ( (msigjson= mgw_stdjson(coin->name,SUPERNET.NXTADDR,SUPERNET.gatewayid,method)) != 0 )
        {
            array = cJSON_CreateArray();
            cJSON_AddItemToArray(array,msig_itemjson(coin->name,NXTaddr,coinaddr,pubkey.buf,1));
            cJSON_AddItemToObject(msigjson,"pubkeys",array);
            retstr = cJSON_Print(msigjson), _stripwhite(retstr,' ');
        } else printf("couldnt generate mgw_stdjson\n");
    }
    return(retstr);
}

int32_t process_acctpubkeys(char *coinstr,int32_t gatewayid,uint64_t gatewaybits,char *retbuf,char *jsonstr,cJSON *json,char *methodstr)
{
    char userNXT[64]; cJSON *array,*item; int32_t i,havemsig,n=0,count = 0,updated = 0;
    if ( SUPERNET.gatewayid >= 0 )
    {
        if ( (array= cJSON_GetObjectItem(json,"pubkeys")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
        {
            //printf("arraysize.%d\n",n);
            for (i=0; i<n; i++)
            {
                item = cJSON_GetArrayItem(array,i);
                copy_cJSON(userNXT,cJSON_GetObjectItem(item,"userNXT"));
                updated += process_acctpubkey(&havemsig,item,gatewayid,gatewaybits);
                count += havemsig;
            }
        }
        sprintf(retbuf,"{\"result\":\"success\",\"coin\":\"%s\",\"updated\":%d,\"total\":%d,\"msigs\":%d}",coinstr,updated,n,count);
        printf("(%s)\n",retbuf);
    }
    return(updated);
}

char *mgw_other_redeem(char *signedtx,uint64_t redeemtxid,uint64_t gatewaybits)
{
    uint64_t key[2]; int32_t len = 65536;
    key[0] = redeemtxid, key[1] = gatewaybits;
    if ( signedtx == 0 )
    {
        signedtx = malloc(len);
        if ( db777_read(signedtx,&len,0,DB_redeems,key,sizeof(key),0) != 0 )
            return(signedtx);
        free(signedtx);
    }
    else
    {
        printf("WRITE.(%s) %llu G.%llu\n",signedtx,(long long)redeemtxid,(long long)gatewaybits);
        db777_write(0,DB_redeems,key,sizeof(key),signedtx,(int32_t)strlen(signedtx)+1);
    }
    return(0);
}

int32_t mgw_other_redeems(char *signedtxs[NUM_GATEWAYS],uint64_t redeemtxid)
{
    int32_t gatewayid,flags = 0;
    for (gatewayid=0; gatewayid<NUM_GATEWAYS; gatewayid++)
        if ( (signedtxs[gatewayid]= mgw_other_redeem(0,redeemtxid,MGW.srv64bits[gatewayid])) != 0 )
            flags |= (1 << gatewayid);
    return(flags);
}

int32_t process_redeem(char *coinstr,int32_t gatewayid,uint64_t gatewaybits,char *retbuf,char *jsonstr,cJSON *json)
{
    uint64_t redeemtxid; char *signedtx,*buf,*cointxid; struct coin777 *coin;
    if ( (redeemtxid= get_API_nxt64bits(cJSON_GetObjectItem(json,"redeemtxid"))) != 0 )
    {
        if ( (cointxid= cJSON_str(cJSON_GetObjectItem(json,"cointxid"))) != 0 )
        {
            if ( (coin= coin777_find(coinstr,0)) != 0 )
            {
                NXT_mark_withdrawdone(&coin->mgw,redeemtxid);
                mgw_other_redeem(jsonstr,redeemtxid,0);
            }
        }
        if ( (signedtx= cJSON_str(cJSON_GetObjectItem(json,"signedtx"))) != 0 )
        {
            if ( (buf= mgw_other_redeem(0,redeemtxid,gatewaybits)) == 0 || strcmp(signedtx+12,buf+12) != 0 )
            {
                mgw_other_redeem(signedtx,redeemtxid,gatewaybits);
                sprintf(retbuf,"{\"result\":\"success\",\"coin\":\"%s\",\"redeemtxid\":\"%llu\",\"gatewayid\":%d,\"gatewayNXT\":\"%llu\",\"signedtx\":\"%s\"}",coinstr,(long long)redeemtxid,gatewayid,(long long)gatewaybits,signedtx!=0?signedtx:"");
                //printf("G%d NEW REDEEM.(%s)\n",gatewayid,retbuf);
            }
            if ( buf != 0 )
                free(buf);
        } else sprintf(retbuf,"{\"error\":\"no signedtx\",\"coin\":\"%s\"}",coinstr), printf("%s\n",retbuf);
    } else sprintf(retbuf,"{\"error\":\"no redeemtxid\",\"coin\":\"%s\"}",coinstr), printf("%s\n",retbuf);
    return(0);
}

int32_t mgw_processbus(char *retbuf,char *jsonstr,cJSON *json)
{
    char coinstr[MAX_JSON_FIELD],NXTaddr[MAX_JSON_FIELD],gatewayNXT[MAX_JSON_FIELD],*methodstr,*str;
    uint64_t gatewaybits; int32_t gatewayid,retval = 0; struct coin777 *coin; cJSON *array;
    printf("MGW PROCESS.(%s)\n",jsonstr);
    if ( (methodstr= cJSON_str(cJSON_GetObjectItem(json,"method"))) != 0 )
    {
        copy_cJSON(coinstr,cJSON_GetObjectItem(json,"coin"));
        gatewayid = get_API_int(cJSON_GetObjectItem(json,"gatewayid"),-1);
        copy_cJSON(gatewayNXT,cJSON_GetObjectItem(json,"gatewayNXT"));
        gatewaybits = calc_nxt64bits(gatewayNXT);
        coin = coin777_find(coinstr,0);
        if ( strncmp(methodstr,"myacctpubkeys",strlen("myacctpubkey")) == 0 )
            retval = process_acctpubkeys(coinstr,gatewayid,gatewaybits,retbuf,jsonstr,json,methodstr);
        else if ( strcmp(methodstr,"askacctpubkey") == 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"pubkeys")) != 0 )
                copy_cJSON(NXTaddr,cJSON_GetObjectItem(cJSON_GetArrayItem(array,0),"userNXT"));
            if ( NXTaddr[0] != 0 && (str= fix_msigaddr(coin,NXTaddr,"myacctpubkey")) != 0 )
            {
                strcpy(retbuf,str);
                nn_send(MGW.all.socks.both.bus,(uint8_t *)str,(int32_t)strlen(str)+1,0);
                printf("MGW SEND.(%s)\n",str);
                free(str);
            }
        }
        else if ( strcmp(methodstr,"redeemtxid") == 0 )
            retval = process_redeem(coinstr,gatewayid,gatewaybits,retbuf,jsonstr,json);
    }
    return(retval);
}

int32_t MGW_publishjson(char *retbuf,cJSON *json)
{
    static uint32_t lastcrc,lasttime;
    char *jsonstr; int32_t sendlen,retval = 0; uint32_t crc;
    jsonstr = cJSON_Print(json);
    _stripwhite(jsonstr,' ');
    crc = _crc32(0,jsonstr,(int32_t)strlen(jsonstr));
    if ( crc != lastcrc || time(NULL) > (lasttime + 10) )
    {
        sendlen = nn_send(MGW.all.socks.both.bus,jsonstr,(int32_t)strlen(jsonstr)+1,0);
        retval = mgw_processbus(retbuf,jsonstr,json);
        printf("MGW publish.(%s) -> (%s) sock.%d sendlen.%d crc.%u\n",jsonstr,retbuf,MGW.all.socks.both.bus,sendlen,crc);
        lastcrc = crc, lasttime = (uint32_t)time(NULL);
    }
    free(jsonstr);
    return(retval);
}

char *get_msig_pubkeys(char *coinstr,char *serverport,char *userpass)
{
    char pubkey[512],NXTaddr[64],account[512],coinaddr[512],*retstr = 0;
    cJSON *json,*item,*array = cJSON_CreateArray();
    uint64_t nxt64bits;
    int32_t i,n;
    if ( (retstr= bitcoind_passthru(coinstr,serverport,userpass,"listreceivedbyaddress","[1, true]")) != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            if ( is_cJSON_Array(json) != 0 && (n= cJSON_GetArraySize(json)) > 0 )
            {
                fprintf(stderr,"listaccounts returns %d addresses\n",n);
                for (i=0; i<n; i++)
                {
                    fprintf(stderr,".");
                    item = cJSON_GetArrayItem(json,i);
                    copy_cJSON(account,cJSON_GetObjectItem(item,"account"));
                    if ( is_decimalstr(account) > 0 )
                    {
                        nxt64bits = calc_nxt64bits(account);
                        expand_nxt64bits(NXTaddr,nxt64bits);
                        if ( strcmp(account,NXTaddr) == 0 )
                        {
                            copy_cJSON(coinaddr,cJSON_GetObjectItem(item,"address"));
                            if ( get_pubkey(pubkey,coinstr,serverport,userpass,coinaddr) != 0 )
                                cJSON_AddItemToArray(array,msig_itemjson(coinstr,account,coinaddr,pubkey,1));
                        }
                        else printf("decimal.%d (%s) -> (%s)? ",is_decimalstr(account),account,NXTaddr);
                    }
                }
            }
            free_json(json);
        } else printf("couldnt parse.(%s)\n",retstr);
        free(retstr);
    } else printf("listreceivedbyaddress doesnt return any accounts\n");
    retstr = cJSON_Print(array);
    _stripwhite(retstr,' ');
    return(retstr);
}

char *devMGW_command(char *jsonstr,cJSON *json)
{
    int32_t i,buyNXT; uint64_t nxt64bits; cJSON *item = 0; struct coin777 *coin;
    char nxtaddr[64],userNXTpubkey[MAX_JSON_FIELD],msigjsonstr[MAX_JSON_FIELD],NXTaddr[MAX_JSON_FIELD],coinstr[1024],*str,*addr;
    if ( SUPERNET.gatewayid >= 0 )
    {
        copy_cJSON(NXTaddr,cJSON_GetObjectItem(json,"userNXT"));
        if ( NXTaddr[0] != 0 )
        {
            if ( (nxt64bits= conv_acctstr(NXTaddr)) != 0 )
                expand_nxt64bits(nxtaddr,nxt64bits);
        } else nxt64bits = 0;
        if ( nxt64bits == 0 )
            return(clonestr("{\"error\":\"illegal NXT addr\"}"));
        //printf("NXTaddr.(%s) %llu\n",nxtaddr,(long long)nxt64bits);
        copy_cJSON(coinstr,cJSON_GetObjectItem(json,"coin"));
        copy_cJSON(userNXTpubkey,cJSON_GetObjectItem(json,"userpubkey"));
        buyNXT = get_API_int(cJSON_GetObjectItem(json,"buyNXT"),0);
        printf("NXTaddr.(%s) %llu %s\n",nxtaddr,(long long)nxt64bits,coinstr);
        if ( nxtaddr[0] != 0 && coinstr != 0 && (coin= coin777_find(coinstr,0)) != 0 )
        {
            for (i=0; i<1; i++)
            {
                if ( ensure_NXT_msigaddr(msigjsonstr,coinstr,nxtaddr,userNXTpubkey,buyNXT) > 0 )
                {
                    item = cJSON_Parse(msigjsonstr);
                    if ( (addr= cJSON_str(cJSON_GetObjectItem(item,"address"))) != 0 )
                        break;
                    free_json(item), item = 0;
                }
                msleep(250);
            }
        }
        if ( item != 0 )
            str = cJSON_Print(item), _stripwhite(str,' '), free_json(item);
        else
        {
            if ( (str= fix_msigaddr(coin,nxtaddr,"myacctpubkey")) == 0 )
            {
                sprintf(msigjsonstr,"{\"error\":\"cant find multisig address\",\"coin\":\"%s\",\"userNXT\":\"%s\"}",coinstr!=0?coinstr:"",nxtaddr);
                _stripwhite(msigjsonstr,' ');
                str = clonestr(msigjsonstr);
            }
        }
        return(str);
    } else return(0);
}

int32_t MGW_publish_acctpubkeys(char *coinstr,char *str)
{
    char retbuf[1024];
    cJSON *json,*array;
    if ( SUPERNET.gatewayid >= 0 && (array= cJSON_Parse(str)) != 0 )
    {
        if ( (json= mgw_stdjson(coinstr,SUPERNET.NXTADDR,SUPERNET.gatewayid,"myacctpubkeys")) != 0 )
        {
            cJSON_AddItemToObject(json,"pubkeys",array);
            MGW_publishjson(retbuf,json);
            free_json(json);
            printf("%s processed.(%s) SUPERNET.gatewayid %d %s\n",coinstr,retbuf,SUPERNET.gatewayid,SUPERNET.NXTADDR);
            return(0);
        }
    }
    return(-1);
}

int32_t pubkeycmp(struct pubkey_info *ref,struct pubkey_info *cmp)
{
    if ( strcmp(ref->pubkey,cmp->pubkey) != 0 )
        return(1);
    if ( strcmp(ref->coinaddr,cmp->coinaddr) != 0 )
        return(2);
    if ( ref->nxt64bits != cmp->nxt64bits )
        return(3);
    return(0);
}

int32_t msigcmp(struct multisig_addr *ref,struct multisig_addr *msig)
{
    int32_t i,x;
    if ( ref == 0 )
        return(-1);
    if ( strcmp(ref->multisigaddr,msig->multisigaddr) != 0 || msig->m != ref->m || msig->n != ref->n )
    {
        if ( Debuglevel > 3 )
            printf("A ref.(%s) vs msig.(%s)\n",ref->multisigaddr,msig->multisigaddr);
        return(1);
    }
    if ( strcmp(ref->NXTaddr,msig->NXTaddr) != 0 )
    {
        if ( Debuglevel > 3 )
            printf("B ref.(%s) vs msig.(%s)\n",ref->NXTaddr,msig->NXTaddr);
        return(2);
    }
    if ( strcmp(ref->redeemScript,msig->redeemScript) != 0 )
    {
        if ( Debuglevel > 3 )
            printf("C ref.(%s) vs msig.(%s)\n",ref->redeemScript,msig->redeemScript);
        return(3);
    }
    for (i=0; i<ref->n; i++)
        if ( (x= pubkeycmp(&ref->pubkeys[i],&msig->pubkeys[i])) != 0 )
        {
            if ( Debuglevel > 3 )
            {
                switch ( x )
                {
                    case 1: printf("P.%d pubkey ref.(%s) vs msig.(%s)\n",x,ref->pubkeys[i].pubkey,msig->pubkeys[i].pubkey); break;
                    case 2: printf("P.%d pubkey ref.(%s) vs msig.(%s)\n",x,ref->pubkeys[i].coinaddr,msig->pubkeys[i].coinaddr); break;
                    case 3: printf("P.%d pubkey ref.(%llu) vs msig.(%llu)\n",x,(long long)ref->pubkeys[i].nxt64bits,(long long)msig->pubkeys[i].nxt64bits); break;
                    default: printf("unexpected retval.%d\n",x);
                }
            }
            return(4+i);
        }
    return(0);
}

#define BTC_COINID 1
#define LTC_COINID 2
#define DOGE_COINID 4
#define BTCD_COINID 8
void set_legacy_coinid(char *coinstr,int32_t legacyid)
{
    switch ( legacyid )
    {
        case BTC_COINID: strcpy(coinstr,"BTC"); return;
        case LTC_COINID: strcpy(coinstr,"LTC"); return;
        case DOGE_COINID: strcpy(coinstr,"DOGE"); return;
        case BTCD_COINID: strcpy(coinstr,"BTCD"); return;
    }
}

struct multisig_addr *decode_msigjson(char *NXTaddr,cJSON *obj,char *sender)
{
    int32_t j,M,n;
    char nxtstr[512],coinstr[64],ipaddr[64],numstr[64],NXTpubkey[128];
    struct multisig_addr *msig = 0;
    cJSON *pobj,*redeemobj,*pubkeysobj,*addrobj,*nxtobj,*nameobj,*idobj;
    if ( obj == 0 )
    {
        printf("decode_msigjson cant decode null obj\n");
        return(0);
    }
    nameobj = cJSON_GetObjectItem(obj,"coin");
    copy_cJSON(coinstr,nameobj);
    if ( coinstr[0] == 0 )
    {
        if ( (idobj = cJSON_GetObjectItem(obj,"coinid")) != 0 )
        {
            copy_cJSON(numstr,idobj);
            if ( numstr[0] != 0 )
                set_legacy_coinid(coinstr,myatoi(numstr,256));
        }
    }
    if ( coinstr[0] != 0 )
    {
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
            n = cJSON_GetArraySize(pubkeysobj);
            M = (int32_t)get_API_int(cJSON_GetObjectItem(obj,"M"),n-1);
            copy_cJSON(NXTpubkey,cJSON_GetObjectItem(obj,"NXTpubkey"));
            if ( NXTpubkey[0] == 0 )
                set_NXTpubkey(NXTpubkey,nxtstr);
            msig = alloc_multisig_addr(coinstr,M,n,nxtstr,NXTpubkey,sender);
            safecopy(msig->coinstr,coinstr,sizeof(msig->coinstr));
            copy_cJSON(msig->redeemScript,redeemobj);
            copy_cJSON(msig->multisigaddr,addrobj);
            msig->buyNXT = (uint32_t)get_API_int(cJSON_GetObjectItem(obj,"buyNXT"),10);
            for (j=0; j<n; j++)
            {
                pobj = cJSON_GetArrayItem(pubkeysobj,j);
                if ( pobj != 0 )
                {
                    copy_cJSON(msig->pubkeys[j].coinaddr,cJSON_GetObjectItem(pobj,"address"));
                    copy_cJSON(msig->pubkeys[j].pubkey,cJSON_GetObjectItem(pobj,"pubkey"));
                    msig->pubkeys[j].nxt64bits = get_API_nxt64bits(cJSON_GetObjectItem(pobj,"srv"));
                    copy_cJSON(ipaddr,cJSON_GetObjectItem(pobj,"ipaddr"));
                    if ( Debuglevel > 2 )
                        fprintf(stderr,"{(%s) (%s) %llu ip.(%s)}.%d ",msig->pubkeys[j].coinaddr,msig->pubkeys[j].pubkey,(long long)msig->pubkeys[j].nxt64bits,ipaddr,j);
                    //if ( ipaddr[0] == 0 && j < 3 )
                    //   strcpy(ipaddr,Server_ipaddrs[j]);
                    //msig->pubkeys[j].ipbits = calc_ipbits(ipaddr);
                } else { free(msig); msig = 0; }
            }
            //printf("NXT.%s -> (%s)\n",nxtstr,msig->multisigaddr);
            if ( Debuglevel > 3 )
                fprintf(stderr,"for msig.%s\n",msig->multisigaddr);
        } else { printf("%p %p %p\n",addrobj,redeemobj,pubkeysobj); free(msig); msig = 0; }
        //printf("return msig.%p\n",msig);
        return(msig);
    } else fprintf(stderr,"decode msig:  error parsing.(%s)\n",cJSON_Print(obj));
    return(0);
}

void *extract_jsonkey(cJSON *item,void *arg,void *arg2)
{
    char *redeemstr = calloc(1,MAX_JSON_FIELD);
    copy_cJSON(redeemstr,cJSON_GetObjectItem(item,arg));
    return(redeemstr);
}

void *extract_jsonints(cJSON *item,void *arg,void *arg2)
{
    char argstr[MAX_JSON_FIELD],*keystr;
    cJSON *obj0=0,*obj1=0;
    if ( arg != 0 )
        obj0 = cJSON_GetObjectItem(item,arg);
    if ( arg2 != 0 )
        obj1 = cJSON_GetObjectItem(item,arg2);
    if ( obj0 != 0 && obj1 != 0 )
    {
        sprintf(argstr,"%llu.%llu",(long long)get_API_int(obj0,0),(long long)get_API_int(obj1,0));
        keystr = calloc(1,strlen(argstr)+1);
        strcpy(keystr,argstr);
        return(keystr);
    } else return(0);
}

void *extract_jsonmsig(cJSON *item,void *arg,void *arg2)
{
    char sender[MAX_JSON_FIELD];
    copy_cJSON(sender,cJSON_GetObjectItem(item,"sender"));
    return(decode_msigjson(0,item,sender));
}

int32_t jsonmsigcmp(void *ref,void *item) { return(msigcmp(ref,item)); }
int32_t jsonstrcmp(void *ref,void *item) { return(strcmp(ref,item)); }

void set_MGW_fname(char *fname,char *dirname,char *NXTaddr)
{
    if ( NXTaddr == 0 )
        sprintf(fname,"%s/%s/ALL",MGW.PATH,dirname);
    else sprintf(fname,"%s/%s/%s",MGW.PATH,dirname,NXTaddr);
}

void set_MGW_msigfname(char *fname,char *NXTaddr) { set_MGW_fname(fname,"msig",NXTaddr); }
void set_MGW_statusfname(char *fname,char *NXTaddr) { set_MGW_fname(fname,"status",NXTaddr); }
void set_MGW_moneysentfname(char *fname,char *NXTaddr) { set_MGW_fname(fname,"sent",NXTaddr); }
void set_MGW_depositfname(char *fname,char *NXTaddr) { set_MGW_fname(fname,"deposit",NXTaddr); }

void save_MGW_file(char *fname,char *jsonstr)
{
    FILE *fp;
    //char cmd[1024];
    if ( (fp= fopen(os_compatible_path(fname),"wb+")) != 0 )
    {
        fwrite(jsonstr,1,strlen(jsonstr),fp);
        fclose(fp);
        //sprintf(cmd,"chmod +r %s",fname);
        //system(cmd);
        //printf("fname.(%s) cmd.(%s)\n",fname,cmd);
    }
}

void save_MGW_status(char *NXTaddr,char *jsonstr)
{
    char fname[1024];
    set_MGW_statusfname(fname,NXTaddr);
    //printf("save_MGW_status.(%s) -> (%s)\n",NXTaddr,fname);
    save_MGW_file(fname,jsonstr);
}

cJSON *update_MGW_file(FILE **fpp,cJSON **newjsonp,char *fname,char *jsonstr)
{
    FILE *fp;
    long fsize;
    cJSON *json,*newjson;
    char cmd[1024],*str;
    *newjsonp = 0;
    *fpp = 0;
    if ( (newjson= cJSON_Parse(jsonstr)) == 0 )
    {
        printf("update_MGW_files: cant parse.(%s)\n",jsonstr);
        return(0);
    }
    if ( (fp= fopen(os_compatible_path(fname),"rb+")) == 0 )
    {
        fp = fopen(os_compatible_path(fname),"wb+");
        if ( fp != 0 )
        {
            if ( (json = cJSON_CreateArray()) != 0 )
            {
                cJSON_AddItemToArray(json,newjson), newjson = 0;
                str = cJSON_Print(json);
                fprintf(fp,"%s",str);
                free(str);
                free_json(json);
            }
            fclose(fp);
#ifndef WIN32
            sprintf(cmd,"chmod +r %s",fname);
            if ( system(os_compatible_path(cmd)) != 0 )
                printf("update_MGW_file chmod error\n");
#endif
        } else printf("couldnt open (%s)\n",fname);
        if ( newjson != 0 )
            free_json(newjson);
        return(0);
    }
    else
    {
        *fpp = fp;
        fseek(fp,0,SEEK_END);
        fsize = ftell(fp);
        rewind(fp);
        str = calloc(1,fsize);
        if ( fread(str,1,fsize,fp) != fsize )
            printf("error reading %ld from %s\n",fsize,fname);
        json = cJSON_Parse(str);
        free(str);
        *newjsonp = newjson;
        return(json);
    }
}

cJSON *append_MGW_file(char *fname,FILE *fp,cJSON *json,cJSON *newjson)
{
    char *str;
    cJSON_AddItemToArray(json,newjson);//, newjson = 0;
    str = cJSON_Print(json);
    rewind(fp);
    fprintf(fp,"%s",str);
    free(str);
    printf("updated (%s)\n",fname);
    return(0);
}

int32_t update_MGW_jsonfile(void (*setfname)(char *fname,char *NXTaddr),void *(*extract_jsondata)(cJSON *item,void *arg,void *arg2),int32_t (*jsoncmp)(void *ref,void *item),char *NXTaddr,char *jsonstr,void *arg,void *arg2)
{
    FILE *fp;
    int32_t i,n,cmpval,appendflag = 0;
    void *refdata,*itemdata;
    cJSON *json,*newjson;
    char fname[1024];
    (*setfname)(fname,NXTaddr);
    if ( (json= update_MGW_file(&fp,&newjson,fname,jsonstr)) != 0 && newjson != 0 && fp != 0 )
    {
        refdata = (*extract_jsondata)(newjson,arg,arg2);
        if ( refdata != 0 && is_cJSON_Array(json) != 0 && (n= cJSON_GetArraySize(json)) > 0 )
        {
            for (i=0; i<n; i++)
            {
                if ( (itemdata = (*extract_jsondata)(cJSON_GetArrayItem(json,i),arg,arg2)) != 0 )
                {
                    cmpval = (*jsoncmp)(refdata,itemdata);
                    if ( itemdata != 0 ) free(itemdata);
                    if ( cmpval == 0 )
                        break;
                }
            }
            if ( i == n )
                newjson = append_MGW_file(fname,fp,json,newjson), appendflag = 1;
        }
        fclose(fp);
        if ( refdata != 0 ) free(refdata);
        if ( newjson != 0 ) free_json(newjson);
        free_json(json);
    }
    return(appendflag);
}

double get_current_rate(char *base,char *rel)
{
    struct coin777 *coin;
    if ( strcmp(rel,"NXT") == 0 )
    {
        if ( (coin= coin777_find(base,0)) != 0 )
        {
            if ( coin->mgw.NXTconvrate != 0. )
                return(coin->mgw.NXTconvrate);
            else if ( coin->mgw.NXTfee_equiv != 0 && coin->mgw.txfee != 0 )
                return(coin->mgw.NXTfee_equiv / coin->mgw.txfee);
        }
    }
    return(1.);
}

uint64_t MGWtransfer_asset(cJSON **transferjsonp,int32_t forceflag,uint64_t nxt64bits,char *depositors_pubkey,struct coin777 *coin,uint64_t value,char *coinaddr,char *txidstr,uint16_t vout,int32_t *buyNXTp,int32_t deadline)
{
    char buf[MAX_JSON_FIELD],nxtassetidstr[64],numstr[64],assetidstr[64],rsacct[64],NXTaddr[64],comment[MAX_JSON_FIELD],*errjsontxt,*str;
    uint64_t depositid = 0,convamount,total = 0;
    int32_t haspubkey,iter,flag,buyNXT = *buyNXTp;
    double rate;
    cJSON *pair,*errjson,*item;
    expand_nxt64bits(NXTaddr,nxt64bits);
    conv_rsacctstr(rsacct,nxt64bits);
    issue_getpubkey(&haspubkey,rsacct);
    if ( haspubkey != 0 )//&& depositors_pubkey[0] == 0 )
    {
        set_NXTpubkey(depositors_pubkey,NXTaddr);
        printf("set.%s pubkey.(%s)\n",NXTaddr,depositors_pubkey);
    }
    expand_nxt64bits(NXTaddr,nxt64bits);
    //sprintf(comment,"{\"coin\":\"%s\",\"coinaddr\":\"%s\",\"cointxid\":\"%s\",\"coinv\":%u,\"amount\":\"%.8f\",\"sender\":\"%s\",\"receiver\":\"%llu\",\"timestamp\":%u,\"quantity\":\"%llu\"}",coin->name,coinaddr,txidstr,vout,dstr(value),SUPERNET.NXTADDR,(long long)nxt64bits,(uint32_t)time(NULL),(long long)(value/coin->mgw.ap_mult));
    sprintf(comment,"{\"coin\":\"%s\",\"coinaddr\":\"%s\",\"cointxid\":\"%s\",\"coinv\":%u,\"amount\":\"%.8f\"}",coin->name,coinaddr,txidstr,vout,dstr(value));//,SUPERNET.NXTADDR,(long long)nxt64bits,(uint32_t)time(NULL),(long long)(value/coin->mgw.ap_mult));
    pair = cJSON_Parse(comment);
    //cJSON_AddItemToObject(pair,"NXT",cJSON_CreateString(NXTaddr));
    printf("forceflag.%d haspubkey.%d >>>>>>>>>>>>>> Need to transfer %.8f %ld assetoshis | %s to %llu for (%s) %s\n",forceflag,haspubkey,dstr(value),(long)(value/coin->mgw.ap_mult),coin->name,(long long)nxt64bits,txidstr,comment);
    total += value;
    convamount = 0;
    if ( haspubkey == 0 && buyNXT > 0 )
    {
        if ( (rate = get_current_rate(coin->name,"NXT")) != 0. )
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
            expand_nxt64bits(assetidstr,coin->mgw.assetidbits);
            depositid = issue_transferAsset(&errjsontxt,0,SUPERNET.NXTACCTSECRET,NXTaddr,(iter == 0) ? assetidstr : nxtassetidstr,(iter == 0) ? (value/coin->mgw.ap_mult) : buyNXT*SATOSHIDEN,MIN_NQTFEE,deadline,str,depositors_pubkey);
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
    return(depositid);
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
    char str[64];
    expand_nxt64bits(str,redeemtxidbits);
    return(in_jsonarray(mgw->limbo,str));
}
    
int32_t mgw_depositstatus(struct coin777 *coin,struct multisig_addr *msig,char *txidstr,int32_t vout)
{
    int32_t i,n,flag = 0; struct extra_info extra;
    NXT_revassettxid(&extra,coin->mgw.assetidbits,0), n = extra.ind;
    for (i=1; i<=n; i++)
    {
        if ( NXT_revassettxid(&extra,coin->mgw.assetidbits,i) == sizeof(extra) )
        {
            //printf("(%d) ",flag);
            if ( (extra.flags & MGW_DEPOSITDONE) != 0 )
            {
                if ( extra.vout == vout && strcmp(txidstr,extra.coindata) == 0 )
                {
                    //  printf("pendingxfer.(%s).v%d vs (%s).v%d\n",extra.coindata,extra.vout,txidstr,vout);
                    flag = MGW_DEPOSITDONE;
                    break;
                }
            }
            else if ( (extra.flags & MGW_IGNORE) != 0 )
                flag = MGW_IGNORE;
        } else printf("error loading assettxid[%d] for %llu\n",i,(long long)coin->mgw.assetidbits);
    }
    //printf("n.%d ",n);
    return(flag);
}

int32_t mgw_encode_OP_RETURN(char *scriptstr,uint64_t redeemtxid)
{
    long _emit_uint32(uint8_t *data,long offset,uint32_t x);
    uint8_t script[256],revbuf[8]; int32_t j; long offset;
    scriptstr[0] = 0;
    script[0] = OP_RETURN_OPCODE;
    script[1] = (sizeof(uint64_t) + 3), script[2] = 'M', script[3] = 'G', script[4] = 'W';
    offset = 5;
    for (j=0; j<sizeof(uint64_t); j++)
        revbuf[j] = ((uint8_t *)&redeemtxid)[j];
    memcpy(&redeemtxid,revbuf,sizeof(redeemtxid));
    offset = _emit_uint32(script,offset,(uint32_t)redeemtxid);
    offset = _emit_uint32(script,offset,(uint32_t)(redeemtxid >> 32));
    init_hexbytes_noT(scriptstr,script,offset);
    return((int32_t)offset);
}

uint64_t mgw_decode_OP_RETURN(uint8_t *script,int32_t scriptlen)
{
    uint8_t *scriptptr,zero12[12]; int32_t j; uint64_t redeemtxid = 0;
    memset(zero12,0,sizeof(zero12));
    if ( scriptlen >= 23 && script[0] == 0x76 && script[1] == 0xa9 && script[2] == 0x14 && memcmp(&script[11],zero12,12) == 0 )
        scriptptr = &script[3];
    else if ( script[0] == OP_RETURN_OPCODE && script[1] == 'M' && script[2] == 'G' && script[3] == 'W' )
        scriptptr = &script[4];
    else return(0);
    for (redeemtxid=j=0; j<(int32_t)sizeof(uint64_t); j++)
        redeemtxid <<= 8, redeemtxid |= (scriptptr[7 - j] & 0xff);
//printf("(REDEEMTXID.%llx %llu) ",(long long)redeemtxid,(long long)redeemtxid);
    return(redeemtxid);
}

uint64_t mgw_is_mgwtx(struct coin777 *coin,uint32_t txidind,uint64_t value)
{
    struct unspent_info U; struct coin777_addrinfo A; struct spend_info S; bits256 txid; struct multisig_addr *msig;
    uint8_t script[4096],*scriptptr; char scriptstr[8192],txidstr[128],buf[8192];
    uint32_t txoffsets[2],nexttxoffsets[2],unspentind,spendind; uint64_t tmp,redeemtxid = 0; int32_t scriptlen,vout,len,missing = 0;
    if ( coin777_RWmmap(0,txoffsets,coin,&coin->ramchain.txoffsets,txidind) == 0 && coin777_RWmmap(0,nexttxoffsets,coin,&coin->ramchain.txoffsets,txidind+1) == 0 )
    {
        if ( coin777_RWmmap(0,&U,coin,&coin->ramchain.unspents,txoffsets[0]) == 0 && coin777_RWmmap(0,&A,coin,&coin->ramchain.addrinfos,U.addrind) == 0 && (U.addrind == coin->mgw.marker_addrind || U.addrind == coin->mgw.marker2_addrind) )
            redeemtxid = 1;
        coin777_RWmmap(0,&txid,coin,&coin->ramchain.txidbits,txidind);
        init_hexbytes_noT(txidstr,txid.bytes,sizeof(txid));
        for (spendind=txoffsets[1]; spendind<nexttxoffsets[1]; spendind++)
        {
            if ( coin777_RWmmap(0,&S,coin,&coin->ramchain.spends,spendind) == 0 )
            {
                if ( coin777_RWmmap(0,&U,coin,&coin->ramchain.unspents,S.unspentind) == 0 && coin777_RWmmap(0,&A,coin,&coin->ramchain.addrinfos,U.addrind) == 0 )
                {
                    if ( (msig= find_msigaddr((struct multisig_addr *)buf,&len,coin->name,A.coinaddr)) == 0 )
                        missing++;
                    //printf("-(%s %.8f m%d).%llu ",A.coinaddr,dstr(U.value),missing,(long long)redeemtxid);
                } else printf("couldnt find spend ind.%u\n",S.unspentind);
            } else printf("error getting spendind.%u\n",spendind);
        }
        if ( missing != 0 )
            return(redeemtxid);
        //printf("MGW tx (%s) numvouts.%d: ",txidstr,nexttxoffsets[0] - txoffsets[0]);
        redeemtxid |= 2;
        for (unspentind=txoffsets[0],vout=0; unspentind<nexttxoffsets[0]; unspentind++,vout++)
        {
            if ( coin777_RWmmap(0,&U,coin,&coin->ramchain.unspents,unspentind) == 0 && coin777_RWmmap(0,&A,coin,&coin->ramchain.addrinfos,U.addrind) == 0 )
            {
                if ( (scriptptr= coin777_scriptptr(&A)) != 0 )
                    init_hexbytes_noT(scriptstr,scriptptr,A.scriptlen);
                else coin777_scriptstr(coin,scriptstr,sizeof(scriptstr),U.rawind_or_blocknum,U.addrind);
                scriptlen = ((int32_t)strlen(scriptstr) >> 1);
                decode_hex(script,scriptlen,scriptstr);
                if ( (tmp= mgw_decode_OP_RETURN(script,scriptlen)) != 0 )
                    redeemtxid  = tmp;
                else if ( U.value <= value && U.value >= value*.99 )
                    redeemtxid |= 4;
                //printf("+[a%d %.8f].%llu ",U.addrind,dstr(U.value),(long long)redeemtxid);
            } else printf("couldnt find unspentind.%u\n",unspentind);
        }
    } else printf("cant find txoffsets[txidind.%u]\n",txidind);
    return(redeemtxid);
}

int32_t mgw_isinternal(struct coin777 *coin,struct multisig_addr *msig,uint32_t addrind,uint32_t unspentind,char *txidstr,int32_t vout,uint64_t value)
{
    char txidstr0[1024]; int32_t vout0; uint32_t txidind0;
    if ( in_jsonarray(coin->mgw.limbo,txidstr) != 0 )
    {
        printf("TXID.(%s) in limbo\n",txidstr);
        return(MGW_ISINTERNAL);
    }
    if ( (vout0= coin777_unspentmap(&txidind0,txidstr0,coin,unspentind - vout)) == 0 )
    {
        if ( mgw_is_mgwtx(coin,txidind0,value) != 0 )
            return(MGW_ISINTERNAL);
    }
    return(0);
}

int32_t validate_coinaddr(char *coinstr,char *serverport,char *userpass,char *coinaddr)
{
    char quotes[512],*retstr; int32_t retval = 0; cJSON *validobj,*json;
    if ( coinaddr[0] != '"' )
        sprintf(quotes,"\"%s\"",coinaddr);
    else safecopy(quotes,coinaddr,sizeof(quotes));
    if ( (retstr= bitcoind_RPC(0,coinstr,serverport,userpass,"validateaddress",quotes)) != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            validobj = cJSON_GetObjectItem(json,"isvalid");
            if ( is_cJSON_True(validobj) != 0 )
                retval = 1;
            free_json(json);
        }
        free(retstr);
    }
    return(retval);
}

int32_t calc_opreturn_script(char *scriptstr,uint64_t redeemtxid,int32_t do_opreturn)
{
    char str40[41]; int32_t i;
    if ( do_opreturn != 0 )
        mgw_encode_OP_RETURN(scriptstr,redeemtxid);
    else
    {
        init_hexbytes_noT(str40,(void *)&redeemtxid,sizeof(redeemtxid));
        for (i=(int32_t)strlen(str40); i<40; i++)
            str40[i] = '0';
        str40[i] = 0;
        sprintf(scriptstr,"76a914%s88ac",str40);
    }
    return((int32_t)strlen(scriptstr));
}

int32_t mgw_update_redeem(struct mgw777 *mgw,struct extra_info *extra)
{
    uint32_t scriptind,txidind,addrind = 0,firstblocknum; int32_t i,vout; uint64_t redeemtxid; char txidstr[256],scriptstr[32768];
    struct coin777_Lentry L; struct addrtx_info ATX; struct coin777 *coin = coin777_find(mgw->coinstr,0);
    if ( coin != 0 && coin->ramchain.readyflag != 0 )
    {
        if ( (extra->flags & MGW_PENDINGREDEEM) != 0 )
        {
            if ( _is_limbo_redeem(&coin->mgw,extra->txidbits) != 0 )
            {
                printf("height.%u is LIMBO REDEEM: (%llu %.8f -> %s)\n",extra->height,(long long)extra->txidbits,dstr(extra->amount),extra->coindata);
                return(MGW_WITHDRAWDONE);
            }
            calc_opreturn_script(scriptstr,extra->txidbits,coin->mgw.do_opreturn);
            scriptind = coin777_scriptind(&firstblocknum,coin,extra->coindata,scriptstr);
            if ( scriptind > 0 && scriptind < 0xffffffff )
            {
                printf("height.%u MATCHED REDEEM: (%llu %.8f -> %s) script.(%s) scriptind.%d\n",extra->height,(long long)extra->txidbits,dstr(extra->amount),extra->coindata,scriptstr,scriptind);
                return(MGW_WITHDRAWDONE);
            }
            printf("height.%u NO REDEEM: (%llu %.8f -> %s) script.(%s) scriptind.%d\n",extra->height,(long long)extra->txidbits,dstr(extra->amount),extra->coindata,scriptstr,scriptind);
            if ( (addrind= coin777_addrind(&firstblocknum,coin,extra->coindata)) != 0 && coin777_RWmmap(0,&L,coin,&coin->ramchain.ledger,addrind) == 0 )
            {
                for (i=0; i<L.numaddrtx; i++)
                {
                    coin777_RWaddrtx(0,coin,addrind,&ATX,&L,i);
                    if ( (vout= coin777_unspentmap(&txidind,txidstr,coin,ATX.unspentind)) >= 0 )
                    {
                        if ( (redeemtxid= mgw_is_mgwtx(coin,txidind,extra->amount)) == extra->txidbits )
                        {
                            printf("height.%u MATCHED REDEEM: (%llu %.8f -> %s) addrind.%u numaddrtx.%d\n",extra->height,(long long)extra->txidbits,dstr(extra->amount),extra->coindata,addrind,L.numaddrtx);
                            return(MGW_WITHDRAWDONE);
                        } //else printf(" %s.v%d for %s\n",txidstr,vout,extra->coindata);
                    } else printf("(%s.v%d != %s)\n",txidstr,vout,extra->coindata);
                }
            } //else printf("skip flag.%d (%s).v%d %.8f\n",extra->flags,extra->coindata,extra->vout,dstr(extra->amount));
            if ( coin->mgw.redeemheight == 0 || extra->height >= coin->mgw.redeemheight )
            {
                if ( validate_coinaddr(mgw->coinstr,coin->serverport,coin->userpass,extra->coindata) == 0 )
                {
                    printf("height.%u ILLEGAL WITHDRAW ADDR: (%llu %.8f -> %s) addrind.%u numaddrtx.%d\n",extra->height,(long long)extra->txidbits,dstr(extra->amount),extra->coindata,addrind,L.numaddrtx);
                    return(MGW_WITHDRAWDONE);
                }
                else
                {
                    printf("[numconfs.%d] height.%u PENDING WITHDRAW: (%llu %.8f -> %s) addrind.%u numaddrtx.%d\n",mgw->RTNXT_height-extra->height,extra->height,(long long)extra->txidbits,dstr(extra->amount),extra->coindata,addrind,L.numaddrtx);
                    if ( coin->mgw.numwithdraws < sizeof(coin->mgw.withdraws)/sizeof(*coin->mgw.withdraws) )
                    {
                        coin->mgw.withdrawsum += extra->amount;
                        memcpy(&coin->mgw.withdraws[coin->mgw.numwithdraws++],extra,sizeof(*extra));
                    }
                }
            }
        } else printf("cant find MGW_PENDINGREDEEM (%s) (%llu %.8f)\n",extra->coindata,(long long)extra->txidbits,dstr(extra->amount));
    }
    return(0);
}

int32_t mgw_unspentkey(uint8_t *key,int32_t maxlen,char *txidstr,uint16_t vout)
{
    int32_t slen;
    slen = (int32_t)strlen(txidstr) >> 1;
    memcpy(key,&vout,sizeof(vout)), decode_hex(&key[sizeof(vout)],slen,txidstr), slen += sizeof(vout);
    return(slen);
}

int32_t mgw_unspentstatus(char *txidstr,uint16_t vout)
{
    uint8_t key[1024]; int32_t status,keylen,len = sizeof(status);
    keylen = mgw_unspentkey(key,sizeof(key),txidstr,vout);
    if ( db777_read(&status,&len,0,DB_MGW,key,keylen,0) != 0 )
        return(status);
    return(0);
}

int32_t mgw_markunspent(char *txidstr,int32_t vout,int32_t status)
{
    uint8_t key[1024]; int32_t keylen;
    if ( status < 0 )
        status = MGW_ERRORSTATUS;
    status |= mgw_unspentstatus(txidstr,vout);
    keylen = mgw_unspentkey(key,sizeof(key),txidstr,vout);
    printf("(%s v%d) <- MGW status.%d\n",txidstr,vout,status);
    return(db777_write(0,DB_MGW,key,keylen,&status,sizeof(status)));
}

int32_t mgw_isrealtime(struct coin777 *coin)
{
    printf("verified.%d lag.%d (coin->ramchain.RTblocknum - coin->ramchain.blocknum) <= coin->minconfirms %d vs %d\n",coin->verified,coin->lag,(coin->ramchain.RTblocknum - coin->ramchain.blocknum),coin->minconfirms);
    if ( (coin->lag >= 0 && coin->lag <= coin->minconfirms) && coin->verified != 0 )
        return(1);
    return(0);
}

uint64_t mgw_unspentsfunc(struct coin777 *coin,void *args,uint32_t addrind,struct addrtx_info *unspents,int32_t num,uint64_t balance)
{
    struct mgw777 *mgw; struct multisig_addr *msig = args;
    int32_t i,Ustatus,status,vout; uint32_t unspentind,txidind; char txidstr[512]; uint64_t nxt64bits,atx_value,sum = 0; struct unspent_info U;
    mgw = &coin->mgw;
    for (i=0; i<num; i++)
    {
        unspentind = unspents[i].unspentind, unspents[i].spendind = 1;
        atx_value = coin777_Uvalue(&U,coin,unspentind), U.rawind_or_blocknum = unspentind;
        if ( (vout= coin777_unspentmap(&txidind,txidstr,coin,unspentind)) >= 0 )
        {
            Ustatus = mgw_unspentstatus(txidstr,vout);
            if ( (Ustatus & (MGW_DEPOSITDONE | MGW_ISINTERNAL | MGW_IGNORE)) == 0 )
            {
                if ( (status= mgw_isinternal(coin,msig,addrind,unspentind,txidstr,vout,0)) != 0 )
                {
                    printf("ISINTERNAL.%u (%s).v%d %.8f -> %s | Ustatus.%d status.%d\n",unspentind,txidstr,vout,dstr(atx_value),msig->multisigaddr,Ustatus,status);
                    mgw_markunspent(txidstr,vout,Ustatus | MGW_ISINTERNAL);
                }
                else
                {
                    if ( (status= mgw_depositstatus(coin,msig,txidstr,vout)) != 0 )
                    {
                        if ( (status & MGW_DEPOSITDONE) != 0 )
                        {
                            printf("DEPOSIT DONE.%u (%s).v%d %.8f -> %s Ustatus.%d status.%d\n",unspentind,txidstr,vout,dstr(atx_value),msig->multisigaddr,Ustatus,status);
                            mgw_markunspent(txidstr,vout,Ustatus | status);
                        }
                        else if ( (status & MGW_IGNORE) != 0 )
                        {
                            printf("MGW_IGNORE.%u (%s).v%d %.8f -> %s Ustatus.%d status.%d\n",unspentind,txidstr,vout,dstr(atx_value),msig->multisigaddr,Ustatus,status);
                            mgw_markunspent(txidstr,vout,Ustatus | status);
                        }
                        else printf("UNKNOWN.%u (%s).v%d %.8f -> %s Ustatus.%d status.%d\n",unspentind,txidstr,vout,dstr(atx_value),msig->multisigaddr,Ustatus,status);
                    }
                    else
                    {
                        // withdraw 11364111978695678059
                        printf("unhandled case.%u (%s).v%d %.8f -> %s | Ustatus.%d status.%d\n",unspentind,txidstr,vout,dstr(atx_value),msig->multisigaddr,Ustatus,status);
                    }
                }
            }
            else
            {
                nxt64bits = calc_nxt64bits(msig->NXTaddr);
                if ( (Ustatus & MGW_ISINTERNAL) != 0 )
                {
                    sum += U.value;
                    mgw->unspents = realloc(mgw->unspents,sizeof(*mgw->unspents) * (mgw->numunspents + 1)), mgw->unspents[mgw->numunspents++] = U;
                    //printf("ISINTERNAL.%u (%s).v%d %.8f -> %s\n",unspentind,txidstr,vout,dstr(atx_value),msig->multisigaddr);
                }
                else if ( (Ustatus & MGW_DEPOSITDONE) == 0 )
                {
                    if ( (Ustatus & MGW_PENDINGXFER) != 0 )
                    {
                        printf("G%d PENDINGXFER.%u (%s).v%d %.8f -> %s\n",(int32_t)(nxt64bits % msig->n),unspentind,txidstr,vout,dstr(atx_value),msig->multisigaddr);
                        if ( (status= mgw_depositstatus(coin,msig,txidstr,vout)) == MGW_DEPOSITDONE )
                            mgw_markunspent(txidstr,vout,Ustatus | MGW_DEPOSITDONE);
                    }
                    else if ( coin->mgw.firstunspentind == 0 || unspentind >= coin->mgw.firstunspentind )
                    {
                        printf("pending deposit.%u (%s).v%d %.8f -> %s nxt.%llu pubkey.%s | Ustatus.%d status.%d\n",unspentind,txidstr,vout,dstr(atx_value),msig->multisigaddr,(long long)nxt64bits,msig->NXTpubkey,Ustatus,status);
                        if ( (nxt64bits % msig->n) == SUPERNET.gatewayid )
                        {
                            if ( mgw_isrealtime(coin) != 0 )
                            {
                                if ( MGWtransfer_asset(0,1,nxt64bits,msig->NXTpubkey,coin,atx_value,msig->multisigaddr,txidstr,vout,&msig->buyNXT,DEPOSIT_XFER_DURATION) != 0 )
                                    mgw_markunspent(txidstr,vout,Ustatus | MGW_PENDINGXFER);
                            }
                        } else mgw_markunspent(txidstr,vout,Ustatus | MGW_PENDINGXFER);
                    }
                    else
                    {
                        sum += U.value;
                        mgw->unspents = realloc(mgw->unspents,sizeof(*mgw->unspents) * (mgw->numunspents + 1)), mgw->unspents[mgw->numunspents++] = U;
                        mgw_markunspent(txidstr,vout,Ustatus | MGW_DEPOSITDONE);
                    }
                }
                else if ( (Ustatus & MGW_DEPOSITDONE) != 0 )
                {
                    sum += U.value;
                    mgw->unspents = realloc(mgw->unspents,sizeof(*mgw->unspents) * (mgw->numunspents + 1)), mgw->unspents[mgw->numunspents++] = U;
                }
            }
        } else printf("error getting unspendind.%u\n",unspentind);
    }
    return(sum);
}

char *mgw_sign_rawbytes(uint32_t *completedp,char *signedbytes,int32_t max,char *coinstr,char *serverport,char *userpass,char *rawbytes)
{
    char *hexstr,*retstr = 0;
    cJSON *json,*compobj;
    *completedp = 0;
    if ( (retstr= bitcoind_passthru(coinstr,serverport,userpass,"signrawtransaction",rawbytes)) != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            if ( (compobj= cJSON_GetObjectItem(json,"complete")) != 0 )
                *completedp = ((compobj->type&0xff) == cJSON_True);
            if ( (hexstr= cJSON_str(cJSON_GetObjectItem(json,"hex"))) != 0 )
            {
                if ( strlen(hexstr) > max )
                    printf("sign_rawbytes: strlen(hexstr) %ld > %d destize (%s)\n",strlen(hexstr),max,retstr), free(retstr), retstr = 0;
                else strcpy(signedbytes,hexstr);
            } else printf("no hex.(%s)\n",retstr);
            free_json(json);
        } else printf("json parse error.(%s)\n",retstr);
    } else printf("error signing rawtx\n");
    return(retstr);
}

char *mgw_sign_localtx_plus2(uint32_t *completedp,char *coinstr,char *serverport,char *userpass,char *signparams,int32_t gatewayid,int32_t numgateways)
{
    char *batchsigned,*retstr; int32_t batchcrc,batchsize;
    batchsize = (uint32_t)strlen(signparams) + 1;
    batchcrc = _crc32(0,signparams+12,batchsize-12); // skip past timediff
    batchsigned = malloc(batchsize*16 + 512);
    batchsigned[0] = '[';
    batchsigned[1] = '"';
    if ( (retstr= mgw_sign_rawbytes(completedp,batchsigned+2,batchsize*16 + 512,coinstr,serverport,userpass,signparams)) != 0 )
    {
        printf("mgw_sign_localtx_plus2.(%s) -> (%s)\n",signparams,retstr);
        free(retstr);
    }
    return(batchsigned+2);
}

char *mgw_OP_RETURN(int32_t opreturn,char *rawtx,int32_t do_opreturn,uint64_t redeemtxid,int32_t oldtx_format)
{
    char scriptstr[1024],*retstr = 0; uint64_t checktxid; long len; struct cointx_info *cointx; struct rawvout *vout;
    if ( (cointx= _decode_rawtransaction(rawtx,oldtx_format)) != 0 )
    {
        vout = &cointx->outputs[opreturn];
        calc_opreturn_script(scriptstr,redeemtxid,do_opreturn);
        if ( do_opreturn != 0 )
        {
            vout->value = 0;
            vout->coinaddr[0] = 0;
        }
        safecopy(vout->script,scriptstr,sizeof(vout->script));
        printf("do_opreturn.%d opreturn vout.%d (%s)\n",do_opreturn,opreturn,vout->script);
        if ( 1 )
        {
            uint8_t script[128];
            decode_hex(script,(int32_t)strlen(scriptstr)>>1,scriptstr);
            if ( (checktxid= mgw_decode_OP_RETURN(script,(int32_t)strlen(scriptstr)>>1)) != redeemtxid )
                printf("redeemtxid.%llx -> opreturn.%llx\n",(long long)redeemtxid,(long long)checktxid);
        }
        len = strlen(rawtx) * 2;
        retstr = calloc(1,len + 1);
        if ( Debuglevel > 1 )
            disp_cointx(cointx);
        printf("vout.%d %p (%s) (%s)\n",opreturn,vout,vout->script,cointx->outputs[opreturn].script);
        if ( _emit_cointx(retstr,len,cointx,oldtx_format) < 0 )
            free(retstr), retstr = 0;
        free(cointx);
    } else printf("error mgw_encode_OP_RETURN\n");
    return(retstr);
}

cJSON *mgw_create_vins(cJSON **keysobjp,char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx,int32_t gatewayid,int32_t numgateways)
{
    int32_t i,ret,nonz; cJSON *json,*array,*keysobj; char normaladdr[1024],redeemScript[4096],*privkey; struct cointx_input *vin;
    array = cJSON_CreateArray();
    keysobj = cJSON_CreateArray();
    for (i=nonz=0; i<cointx->numinputs; i++)
    {
        vin = &cointx->inputs[i];
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json,"txid",cJSON_CreateString(vin->tx.txidstr));
        cJSON_AddItemToObject(json,"vout",cJSON_CreateNumber(vin->tx.vout));
        cJSON_AddItemToObject(json,"scriptPubKey",cJSON_CreateString(vin->sigs));
        if ( (ret= _map_msigaddr(redeemScript,coinstr,serverport,userpass,normaladdr,cointx->inputs[i].coinaddr,gatewayid,numgateways)) >= 0 )
        {
            cJSON_AddItemToObject(json,"redeemScript",cJSON_CreateString(redeemScript));
            if ( (privkey= dumpprivkey(coinstr,serverport,userpass,normaladdr)) == 0 )
                printf("error getting privkey to (%s)\n",normaladdr);
            else
            {
                nonz++;
                cJSON_AddItemToArray(keysobj,cJSON_CreateString(privkey));
                free(privkey);
            }
        }
        else printf("ret.%d redeemScript.(%s) (%s) for (%s)\n",ret,redeemScript,normaladdr,vin->coinaddr);
        //printf("vin.(%s)\n",cJSON_Print(json));
        cJSON_AddItemToArray(array,json);
    }
    *keysobjp = keysobj;
    return(array);
}

cJSON *mgw_create_vouts(struct cointx_info *cointx)
{
    int32_t i;
    cJSON *json;
    json = cJSON_CreateObject();
    for (i=0; i<cointx->numoutputs; i++)
        cJSON_AddItemToObject(json,cointx->outputs[i].coinaddr, cJSON_CreateNumber(dstr(cointx->outputs[i].value)));
    return(json);
}

struct cointx_info *mgw_createrawtransaction(struct mgw777 *mgw,char *coinstr,char *serverport,char *userpass,struct cointx_info *cointx,int32_t opreturn,uint64_t redeemtxid,int32_t gatewayid,int32_t numgateways,int32_t oldtx_format,int32_t do_opreturn)
{
    struct cointx_info *rettx = 0; char *signedtxs[NUM_GATEWAYS],*txbytes,*signedtx,*txbytes2,*paramstr,*cointxid;
    cJSON *array,*voutsobj=0,*vinsobj=0,*keysobj=0; int32_t i,flags,allocsize,len = 65536;
    txbytes = calloc(1,len);
    if ( _emit_cointx(txbytes,len,cointx,oldtx_format) < 0 )
    {
        free(txbytes);
        return(0);
    }
    vinsobj = mgw_create_vins(&keysobj,coinstr,serverport,userpass,cointx,gatewayid,numgateways);
    if ( vinsobj != 0 && (voutsobj= mgw_create_vouts(cointx)) != 0 && keysobj != 0 && txbytes != 0 )
    {
        array = cJSON_CreateArray();
        cJSON_AddItemToArray(array,cJSON_Duplicate(vinsobj,1));
        cJSON_AddItemToArray(array,cJSON_Duplicate(voutsobj,1));
        paramstr = cJSON_Print(array), free_json(array), _stripwhite(paramstr,' ');
        if ( Debuglevel > 1 )
            fprintf(stderr,"len.%ld calc_rawtransaction.%llu txbytes.(%s) params.(%s)\n",strlen(txbytes),(long long)redeemtxid,txbytes,paramstr);
        txbytes = bitcoind_passthru(coinstr,serverport,userpass,"createrawtransaction",paramstr);
        free(paramstr);
        if ( txbytes == 0 )
            return(0);
        printf("got txbytes.(%s) opreturn.%d\n",txbytes,opreturn);
        if ( opreturn >= 0 )
        {
            if ( (txbytes2= mgw_OP_RETURN(opreturn,txbytes,do_opreturn,redeemtxid,oldtx_format)) == 0 )
            {
                fprintf(stderr,"error replacing with OP_RETURN.%s txout.%d (%s)\n",coinstr,opreturn,txbytes);
                free(txbytes);
                return(0);
            }
            free(txbytes);
            txbytes = txbytes2, txbytes2 = 0;
            printf("opreturn txbytes.(%s)\n",txbytes);
        }
        array = cJSON_CreateArray();
        if ( (flags= mgw_other_redeems(signedtxs,redeemtxid)) != 0 )
        {
            if ( (txbytes2= signedtxs[(SUPERNET.gatewayid + 1) % NUM_GATEWAYS]) != 0 )
                free(txbytes), txbytes = txbytes2, signedtxs[(SUPERNET.gatewayid + 1) % NUM_GATEWAYS] = 0;
            if ( (txbytes2= signedtxs[(SUPERNET.gatewayid - 1 + NUM_GATEWAYS) % NUM_GATEWAYS]) != 0 )
                free(txbytes), txbytes = txbytes2, signedtxs[(SUPERNET.gatewayid - 1 + NUM_GATEWAYS) % NUM_GATEWAYS] = 0;
            for (i=0; i<NUM_GATEWAYS; i++)
                if ( signedtxs[i] != 0 )
                    free(signedtxs[i]);
        }
        cJSON_AddItemToArray(array,cJSON_CreateString(txbytes));
        cJSON_AddItemToArray(array,vinsobj);
        cJSON_AddItemToArray(array,keysobj);
        paramstr = cJSON_Print(array), free_json(array);
        if ( (signedtx= mgw_sign_localtx_plus2(&cointx->completed,coinstr,serverport,userpass,paramstr,gatewayid,numgateways)) != 0 )
        {
            allocsize = (int32_t)(sizeof(*rettx) + strlen(signedtx) + 1);
            printf("signedtx returns.(%s) allocsize.%d\n",signedtx,allocsize);
            rettx = calloc(1,allocsize);
            *rettx = *cointx;
            rettx->allocsize = allocsize;
            rettx->isallocated = allocsize;
            strcpy(rettx->signedtx,signedtx);
            signedtx -= 2;
            if ( cointx->completed != 0 )
            {
                strcat(signedtx,"\"]");
                if ( (cointxid= bitcoind_passthru(coinstr,serverport,userpass,"sendrawtransaction",signedtx)) != 0 )
                {
                    strcpy(rettx->cointxid,cointxid);
                    free(cointxid);
                    NXT_mark_withdrawdone(mgw,redeemtxid);
                }
                printf(">>>>>>>>>>>>> BROADCAST.(%s) (%s) cointxid.%s\n",signedtx,paramstr,rettx->cointxid);
            }
            free(signedtx);
        } else fprintf(stderr,"error _sign_localtx.(%s)\n",txbytes);
        free(paramstr);
    }
    else
    {
        fprintf(stderr,"error creating rawtransaction.(%s)\n",txbytes);
        if ( vinsobj != 0 )
            free_json(vinsobj);
        if ( voutsobj != 0 )
            free_json(voutsobj);
    }
    if ( txbytes != 0 )
        free(txbytes);
    return(rettx);
}

struct unspent_info *coin777_bestfit(uint64_t *valuep,struct coin777 *coin,struct unspent_info *unspents,int32_t numunspents,uint64_t value)
{
    int32_t i; uint64_t above,below,gap,atx_value; struct unspent_info *vin,*abovevin,*belowvin;
    abovevin = belowvin = 0;
    *valuep = 0;
    for (above=below=i=0; i<numunspents; i++)
    {
        vin = &unspents[i];
        *valuep = atx_value = vin->value;
        if ( atx_value == value )
            return(vin);
        else if ( atx_value > value )
        {
            gap = (atx_value - value);
            if ( above == 0 || gap < above )
            {
                above = gap;
                abovevin = vin;
            }
        }
        else
        {
            gap = (value - atx_value);
            if ( below == 0 || gap < below )
            {
                below = gap;
                belowvin = vin;
            }
        }
    }
    return((abovevin != 0) ? abovevin : belowvin);
}

int64_t coin777_inputs(uint64_t *changep,uint32_t *nump,struct coin777 *coin,struct cointx_input *inputs,int32_t max,uint64_t amount,uint64_t txfee)
{
    int64_t remainder,sum = 0; int32_t i,numinputs = 0; uint32_t txidind,unspentind; uint64_t value;
    struct unspent_info *vin,U; struct cointx_input I; struct coin777_addrinfo A; struct mgw777 *mgw = &coin->mgw;
    *nump = 0;
    remainder = amount + txfee;
    for (i=0; i<mgw->numunspents&&i<max-1; i++)
    {
        if ( (vin= coin777_bestfit(&value,coin,mgw->unspents,mgw->numunspents,remainder)) != 0 )
        {
            sum += vin->value;
            remainder -= vin->value;
            // struct cointx_input { struct rawvin tx; char coinaddr[64],sigs[1024]; uint64_t value; uint32_t sequence; char used; };
            coin777_RWmmap(0,&A,coin,&coin->ramchain.addrinfos,vin->addrind);
            memset(&I,0,sizeof(I));
            strcpy(I.coinaddr,A.coinaddr);
            unspentind = vin->rawind_or_blocknum;
            coin777_RWmmap(0,&U,coin,&coin->ramchain.unspents,unspentind);
            coin777_scriptstr(coin,I.sigs,sizeof(I.sigs),U.rawind_or_blocknum,U.addrind);
            I.tx.vout = coin777_unspentmap(&txidind,I.tx.txidstr,coin,unspentind);
            if ( Debuglevel > 2 )
                printf("{%s %s %s}.i%d ",I.coinaddr,I.tx.txidstr,I.sigs,numinputs);
            I.value = vin->value;
            inputs[numinputs++] = I;
            memset(vin,0,sizeof(*vin));
            if ( sum >= (amount + txfee) )
            {
                *nump = numinputs;
                *changep = (sum - amount - txfee);
                fprintf(stderr,"numinputs %d sum %.8f vs amount %.8f change %.8f -> miners %.8f\n",numinputs,dstr(sum),dstr(amount),dstr(*changep),dstr(sum - *changep - amount));
                return(sum);
            }
        } else printf("no bestfit found i.%d of %d\n",i,mgw->numunspents);
    }
    fprintf(stderr,"error numinputs %d sum %.8f\n",numinputs,dstr(sum));
    return(0);
}

struct cointx_info *mgw_cointx_withdraw(struct coin777 *coin,char *destaddr,uint64_t value,uint64_t redeemtxid,char *smallest,char *smallestB)
{
    //int64 nPayFee = nTransactionFee * (1 + (int64)nBytes / 1000);
    char *changeaddr; int64_t MGWfee,amount,opreturn_amount; int32_t opreturn_output=-1; struct cointx_info *cointx,TX,*rettx = 0; struct mgw777 *mgw;
    mgw = &coin->mgw;
    cointx = &TX, memset(cointx,0,sizeof(*cointx));
    if ( coin->minoutput == 0 )
        coin->minoutput = 1;
    opreturn_amount = (coin->mgw.do_opreturn != 0) ? 0 : coin->minoutput;
    memset(cointx,0,sizeof(*cointx));
    strcpy(cointx->coinstr,coin->name);
    cointx->redeemtxid = redeemtxid;
    cointx->gatewayid = SUPERNET.gatewayid;
    MGWfee = (value >> 11) + (2 * (mgw->txfee + mgw->NXTfee_equiv)) - opreturn_amount - mgw->txfee;
    if ( value <= MGWfee + opreturn_amount + mgw->txfee )
    {
        printf("%s redeem.%llu withdraw %.8f < MGWfee %.8f + minoutput %.8f + txfee %.8f\n",coin->name,(long long)redeemtxid,dstr(value),dstr(MGWfee),dstr(opreturn_amount),dstr(mgw->txfee));
        return(0);
    }
    strcpy(cointx->outputs[cointx->numoutputs].coinaddr,mgw->marker);
    if ( strcmp(destaddr,mgw->marker) == 0 )
        cointx->outputs[cointx->numoutputs++].value = value - opreturn_amount - mgw->txfee;
    else
    {
        cointx->outputs[cointx->numoutputs++].value = MGWfee;
        strcpy(cointx->outputs[cointx->numoutputs].coinaddr,destaddr);
        cointx->outputs[cointx->numoutputs++].value = value - MGWfee - opreturn_amount - mgw->txfee;
    }
    if ( opreturn_amount != 0 )
    {
        opreturn_output = cointx->numoutputs;
        //printf("opreturn (%s)\n",coin->mgw.opreturnmarker);
        strcpy(cointx->outputs[cointx->numoutputs].coinaddr,mgw->opreturnmarker);
        cointx->outputs[cointx->numoutputs++].value = opreturn_amount;
    }
    cointx->amount = amount = value - mgw->txfee;//(MGWfee + value + opreturn_amount + mgw->txfee);
    printf("total %.8f: value %.8f txfee %.8f MGWfee %.8f\n",dstr(cointx->amount),dstr(value),dstr(mgw->txfee),dstr(MGWfee));
    if ( mgw->balance >= 0 )
    {
        cointx->inputsum = coin777_inputs(&cointx->change,&cointx->numinputs,coin,cointx->inputs,sizeof(cointx->inputs)/sizeof(*cointx->inputs),amount,mgw->txfee);
        if ( cointx->inputsum >= cointx->amount )
        {
            if ( cointx->change != 0 )
            {
                changeaddr = (strcmp(smallest,destaddr) != 0) ? smallest : smallestB;
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
            if ( opreturn_amount == 0 )
            {
                opreturn_output = cointx->numoutputs;
                printf("SET opreturn.%d (%s)\n",opreturn_output,coin->mgw.opreturnmarker);
                strcpy(cointx->outputs[cointx->numoutputs].coinaddr,coin->mgw.opreturnmarker);
                cointx->outputs[cointx->numoutputs++].value = coin->minoutput;
            }
            if ( SUPERNET.gatewayid >= 0 )
            {
                rettx = mgw_createrawtransaction(mgw,coin->name,coin->serverport,coin->userpass,cointx,opreturn_output,redeemtxid,SUPERNET.gatewayid,NUM_GATEWAYS,coin->mgw.oldtx_format,mgw->do_opreturn);
            }
        } else fprintf(stderr,"error calculating rawinputs.%.8f or outputs.%.8f | txfee %.8f\n",dstr(cointx->inputsum),dstr(cointx->amount),dstr(mgw->txfee));
    } else fprintf(stderr,"not enough %s balance %.8f for withdraw %.8f txfee %.8f\n",coin->name,dstr(mgw->balance),dstr(cointx->amount),dstr(mgw->txfee));
    return(rettx);
}

uint64_t mgw_calc_unspent(char *smallestaddr,char *smallestaddrB,struct coin777 *coin)
{
    struct multisig_addr **msigs; int32_t i,n = 0,m=0; uint32_t firstblocknum; uint64_t circulation,smallest,val,unspent = 0; int64_t balance;
    cJSON *json,*retjson,*item,*waiting,*pending; char buf[1024],numstr[64],*jsonstr,*retbuf; struct extra_info *extra; struct mgw777 *mgw = &coin->mgw;
    ramchain_prepare(coin,&coin->ramchain);
    if ( mgw->unspents != 0 )
        free(mgw->unspents);
    mgw->unspents = 0;
    mgw->numunspents = 0;
    coin->ramchain.paused = 0;
    smallestaddr[0] = smallestaddrB[0] = 0;
    if ( coin == 0 )
    {
        printf("mgw_calc_MGWunspent: no coin777\n");
        return(0);
    }
    retjson = cJSON_CreateObject(), waiting = cJSON_CreateArray(), pending = cJSON_CreateArray();
    if ( mgw->marker_addrind == 0 && mgw->marker != 0 )
        mgw->marker_addrind = coin777_addrind(&firstblocknum,coin,mgw->marker);
    if ( mgw->marker2_addrind == 0 && mgw->marker2 != 0 )
        mgw->marker2_addrind = coin777_addrind(&firstblocknum,coin,mgw->marker2);
    if ( (msigs= (struct multisig_addr **)db777_copy_all(&n,DB_msigs,"value",0)) != 0 )
    {
        for (smallest=i=m=0; i<n; i++)
        {
            if ( msigs[i]->sig != stringbits("multisig") )
            {
                free(msigs[i]);
                continue;
            }
            if ( strcmp(msigs[i]->coinstr,coin->name) == 0 && (val= coin777_unspents(mgw_unspentsfunc,coin,msigs[i]->multisigaddr,msigs[i])) != 0 )
            {
                m++;
                unspent += val;
                if ( smallest == 0 || val < smallest )
                {
                    smallest = val;
                    strcpy(smallestaddrB,smallestaddr);
                    strcpy(smallestaddr,msigs[i]->multisigaddr);
                }
                else if ( smallestaddrB[0] == 0 && strcmp(smallestaddr,msigs[i]->multisigaddr) != 0 )
                    strcpy(smallestaddrB,msigs[i]->multisigaddr);
            }
            free(msigs[i]);
        }
        free(msigs);
        if ( Debuglevel > 2 )
            printf("smallest (%s %.8f)\n",smallestaddr,dstr(smallest));
    }
    mgw->circulation = circulation = calc_circulation(0,mgw,0);
    mgw->unspent = unspent;
    balance = (unspent - circulation - mgw->withdrawsum);
    printf("%s circulation %.8f vs unspents %.8f numwithdraws.%d withdrawsum %.8f [balance %.8f] nummsigs.%d\n",coin->name,dstr(circulation),dstr(unspent),mgw->numwithdraws,dstr(mgw->withdrawsum),dstr(balance),m);
    cJSON_AddItemToObject(retjson,"coin",cJSON_CreateString(coin->name));
    cJSON_AddItemToObject(retjson,"circulation",cJSON_CreateNumber(dstr(circulation)));
    cJSON_AddItemToObject(retjson,"unspent",cJSON_CreateNumber(dstr(unspent)));
    cJSON_AddItemToObject(retjson,"numwithdraws",cJSON_CreateNumber(mgw->numwithdraws));
    cJSON_AddItemToObject(retjson,"withdrawsum",cJSON_CreateNumber(dstr(mgw->withdrawsum)));
    cJSON_AddItemToObject(retjson,"balance",cJSON_CreateNumber(dstr(balance)));
    sprintf(buf,"G%d:[+%.8f %s - %.0f NXT rate %.2f] unspent %.8f circ %.8f/%.8f pend.(R%.8f D%.8f) NXT.%d %s.%d<BR>",SUPERNET.gatewayid,dstr(balance),coin->name,dstr(mgw->S.sentNXT),balance<=0?0:dstr(mgw->S.sentNXT)/dstr(balance),dstr(unspent),dstr(circulation),dstr(coin->ramchain.addrsum),dstr(mgw->withdrawsum),dstr(0),_get_NXTheight(0),coin->name,coin->ramchain.RTblocknum);
    if ( balance >= -SATOSHIDEN && mgw->numwithdraws > 0 && mgw_isrealtime(coin) != 0 )
    {
        struct cointx_info *cointx;
        for (i=0; i<mgw->numwithdraws; i++)
        {
            extra = &mgw->withdraws[i];
            item = cJSON_CreateObject();
            sprintf(numstr,"%llu",(long long)extra->txidbits), cJSON_AddItemToObject(item,"redeemtxid",cJSON_CreateString(numstr));
            cJSON_AddItemToObject(item,"withdrawaddr",cJSON_CreateString(extra->coindata));
            cJSON_AddItemToObject(item,"amount",cJSON_CreateNumber(dstr(extra->amount)));
            if ( (mgw->RTNXT_height - extra->height) < SUPERNET.NXTconfirms )
            {
                printf("numconfs.%d of %d for redeemtxid.%llu -> (%s) %.8f\n",(mgw->RTNXT_height - extra->height),SUPERNET.NXTconfirms,(long long)extra->txidbits,extra->coindata,dstr(extra->amount));
                cJSON_AddItemToObject(item,"numconfs",cJSON_CreateNumber(mgw->RTNXT_height - extra->height));
                cJSON_AddItemToObject(item,"needed",cJSON_CreateNumber(SUPERNET.NXTconfirms));
                cJSON_AddItemToArray(waiting,item);
            }
            else
            {
                cointx = mgw_cointx_withdraw(coin,extra->coindata,extra->amount,extra->txidbits,smallestaddr,smallestaddrB);
                if ( cointx != 0 )
                {
                    printf("%p height.%u PENDING WITHDRAW: (%llu %.8f -> %s) inputsum %.8f numinputs.%d change %.8f miners %.8f\n",cointx,extra->height,(long long)extra->txidbits,dstr(extra->amount),extra->coindata,dstr(cointx->inputsum),cointx->numinputs,dstr(cointx->change),dstr(cointx->inputsum)-dstr(cointx->change));
                    cJSON_AddItemToObject(item,"inputsum",cJSON_CreateNumber(dstr(cointx->inputsum)));
                    cJSON_AddItemToObject(item,"numinputs",cJSON_CreateNumber(cointx->numinputs));
                    cJSON_AddItemToObject(item,"change",cJSON_CreateNumber(dstr(cointx->change)));
                    cJSON_AddItemToObject(item,"miners",cJSON_CreateNumber(dstr(cointx->inputsum) - dstr(cointx->change)));
                    cJSON_AddItemToArray(pending,item);
                    if ( (json= mgw_stdjson(coin->name,SUPERNET.NXTADDR,SUPERNET.gatewayid,"redeemtxid")) != 0 )
                    {
                        sprintf(numstr,"%llu",(long long)extra->txidbits), cJSON_AddItemToObject(json,"redeemtxid",cJSON_CreateString(numstr));
                        cJSON_AddItemToObject(json,"gatewayid",cJSON_CreateNumber(SUPERNET.gatewayid));
                        cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(SUPERNET.NXTADDR));
                        cJSON_AddItemToObject(json,"signedtx",cJSON_CreateString(cointx->signedtx));
                        if ( cointx->cointxid[0] != 0 )
                            cJSON_AddItemToObject(json,"cointxid",cJSON_CreateString(cointx->cointxid));
                        jsonstr = cJSON_Print(json), _stripwhite(jsonstr,' ');
                        retbuf = malloc(65536), MGW_publishjson(retbuf,json);
                        free(retbuf), free(jsonstr), free_json(json);
                    }
                    free(cointx);
                }
            }
        }
    } else if ( mgw->numwithdraws == 0 )
        coin->mgw.lastupdate = (milliseconds() + 60000);
    cJSON_AddItemToObject(retjson,"waiting",waiting);
    cJSON_AddItemToObject(retjson,"pending",pending);
    cJSON_AddItemToObject(retjson,"summary",cJSON_CreateString(buf));
    if ( mgw->retjson != 0 )
        free_json(mgw->retjson);
    mgw->retjson = retjson;
    return(unspent);
}

int32_t make_MGWbus(uint16_t port,char *bindaddr,char serverips[MAX_MGWSERVERS][64],int32_t n)
{
    char tcpaddr[64];
    int32_t i,err,sock,timeout = 10;
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
        if ( 0 && (err= nn_connect(sock,tcpaddr)) < 0 )
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
            if ( serverips[i] != 0 && serverips[i][0] != 0 && strcmp(bindaddr,serverips[i]) != 0 )
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

int32_t PLUGNAME(_process_json)(char *forwarder,char *sender,int32_t valid,struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag,char *tokenstr)
{
    char NXTaddr[64],nxtaddr[64],ipaddr[64],*resultstr,*coinstr,*methodstr,*coinaddr,*retstr = 0; int32_t i,j,n; cJSON *array; uint64_t nxt64bits;
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
        MGW.M = get_API_int(cJSON_GetObjectItem(json,"M"),2);
        if ( (array= cJSON_GetObjectItem(json,"MGWservers")) != 0 && (n= cJSON_GetArraySize(array)) > 0 && (n & 1) == 0 )
        {
            for (i=j=0; i<n/2&&i<MAX_MGWSERVERS; i++)
            {
                copy_cJSON(ipaddr,cJSON_GetArrayItem(array,i<<1));
                copy_cJSON(nxtaddr,cJSON_GetArrayItem(array,(i<<1)+1));
                if ( nxtaddr[0] != 0 )
                    nxt64bits = conv_acctstr(nxtaddr);
                else nxt64bits = 0;
                if ( strcmp(ipaddr,MGW.bridgeipaddr) != 0 )
                {
                    MGW.srv64bits[j] = nxt64bits;
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
        }
        MGW.readyflag = 1;
        plugin->allowremote = 1;
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf) + (retbuf[0]!=0));
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        methodstr = cJSON_str(cJSON_GetObjectItem(json,"method"));
        coinstr = cJSON_str(cJSON_GetObjectItem(json,"coin"));
        coinaddr = cJSON_str(cJSON_GetObjectItem(json,"coinaddr"));
        if ( methodstr == 0 || methodstr[0] == 0 || SUPERNET.gatewayid < 0 )
        {
            printf("(%s) has not method or not a gateway node %d\n",jsonstr,SUPERNET.gatewayid);
            return(0);
        }
        printf("MGW.(%s) for (%s)\n",methodstr,coinstr!=0?coinstr:"");
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
        else if ( strcmp(methodstr,"findmsigaddr") == 0 )
        {
            struct multisig_addr *msig; char buf[4096]; int32_t len = sizeof(buf);
            if ( coinstr != 0 && coinaddr != 0 && (msig= find_msigaddr((struct multisig_addr *)buf,&len,coinstr,coinaddr)) != 0 )
                sprintf(retbuf,"{\"result\":\"success\",\"NXT\":\"%s\"}",msig->NXTaddr);
            else strcpy(retbuf,"{\"result\":\"cant find addr\"}");
        }
        else if ( strcmp(methodstr,"msigaddr") == 0 )
        {
            if ( SUPERNET.gatewayid >= 0 )
            {
                char *str;
                if ( (str= fix_msigaddr(coin777_find(coinstr,0),cJSON_str(cJSON_GetObjectItem(json,"userNXT")),"askacctpubkey")) != 0 )
                {
                    strcpy(retbuf,str);
                    nn_send(MGW.all.socks.both.bus,(uint8_t *)str,(int32_t)strlen(str)+1,0);
                    printf("MGW SEND.(%s)\n",str);
                    free(str);
                    msleep(250);
                }
                if ( (retstr= devMGW_command(jsonstr,json)) != 0 )
                    printf("msigaddr.(%s)\n",retstr);
                else sprintf(retbuf,"{\"result\":\"start msigaddr\"}");
            }
        }
        else if ( coinstr != 0 && strcmp(methodstr,"status") == 0 )
        {
            struct coin777 *coin;
            if ( (coin= coin777_find(coinstr,0)) != 0 && coin->mgw.retjson != 0 )
                retstr = cJSON_Print(coin->mgw.retjson), _stripwhite(retstr,' ');
        }
        else if ( strcmp(methodstr,"myacctpubkeys") == 0 || strcmp(methodstr,"myacctpubkey") == 0 || strcmp(methodstr,"askacctpubkey") == 0 )
            mgw_processbus(retbuf,jsonstr,json);
    }
    return(plugin_copyretstr(retbuf,maxlen,retstr));
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    return(retcode);
}
#include "../agents/plugin777.c"
#endif
#include <stdint.h>
extern int32_t Debuglevel;
