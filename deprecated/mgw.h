//
//  mgw.h
//
//  Created by jl777 2014, refactored MGW
//  Copyright (c) 2014 jl777. MIT License.
//

#ifndef mgw_h
#define mgw_h


int32_t msigcmp(struct multisig_addr *ref,struct multisig_addr *msig);
struct multisig_addr *decode_msigjson(char *NXTaddr,cJSON *obj,char *sender);
char *create_multisig_json(struct multisig_addr *msig,int32_t truncated);

double enough_confirms(double redeemed,double estNXT,int32_t numconfs,int32_t minconfirms)
{
    double metric;
    if ( numconfs < minconfirms )
        return(0);
    metric = log(estNXT + sqrt(redeemed));
    if ( metric < 1 )
        metric = 1.;
    return(((double)numconfs/minconfirms) - metric);
}

int32_t in_specialNXTaddrs(char *specialNXTaddrs[],char *NXTaddr)
{
    int32_t i;
    for (i=0; specialNXTaddrs[i]!=0; i++)
        if ( strcmp(specialNXTaddrs[i],NXTaddr) == 0 )
            return(1);
    return(0);
}

int32_t is_limbo_redeem(struct coin_info *cp,uint64_t redeemtxidbits)
{
    int32_t j;
    if ( cp != 0 && cp->RAM.limboarray != 0 )
    {
        for (j=0; cp->RAM.limboarray[j]!=0; j++)
            if ( redeemtxidbits == cp->RAM.limboarray[j] )
                return(1);
    }
    return(0);
}

void set_txidmap_str(char *mapstr,char *txidstr,char *coinstr,int32_t v) // for txid/vout -> presence check
{
    sprintf(mapstr,"%s=%s.%d",coinstr,txidstr,v);
}

void set_MGW_fname(char *fname,char *dirname,char *NXTaddr)
{
    if ( NXTaddr == 0 )
        sprintf(fname,"%s/MGW/%s/ALL",MGWROOT,dirname);
    else sprintf(fname,"%s/MGW/%s/%s",MGWROOT,dirname,NXTaddr);
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
    set_MGW_statusfname(os_compatible_path(fname),NXTaddr);
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



/*int32_t jsonstrcmp2(void *ref,void *item)
{
    long reflen,len;
    reflen = strlen(ref);
    len = strlen(item);
    if ( reflen == len && strcmp(ref,item) == 0 )
        return(strcmp((char *)((long)ref + len + 1),(char *)((long)item + len + 1)));
    return(-1);
}

void update_MGW_msigfile(char *NXTaddr,struct multisig_addr *refmsig,char *jsonstr)
{
    FILE *fp;
    cJSON *json = 0,*newjson,*item;
    int32_t i,n;
    struct multisig_addr *msig;
    char sender[MAX_JSON_FIELD],fname[1024];
    set_MGW_msigfname(fname,NXTaddr);
    if ( (json= update_MGW_file(&fp,&newjson,fname,jsonstr)) != 0 && newjson != 0 && fp != 0 )
    {
        if ( is_cJSON_Array(json) != 0 && (n= cJSON_GetArraySize(json)) > 0 )
        {
            msig = 0;
            for (i=0; i<n; i++)
            {
                item = cJSON_GetArrayItem(json,i);
                copy_cJSON(sender,cJSON_GetObjectItem(item,"sender"));
                if  ( (msig= decode_msigjson(0,item,sender)) != 0 )
                {
                    if ( msigcmp(refmsig,msig) == 0 )
                        break;
                    free(msig), msig = 0;
                }
            }
            if ( msig != 0 )
                free(msig);
            if ( i == n )
                newjson = append_MGW_file(fname,fp,json,newjson);
        }
        fclose(fp);
        if ( newjson != 0 )
            free_json(newjson);
        free_json(json);
    }
}*/

int32_t update_MGW_msig(struct multisig_addr *msig,char *sender)
{
    char *jsonstr;
    int32_t appendflag = 0;
    if ( msig != 0 )
    {
        jsonstr = create_multisig_json(msig,0);
        if ( jsonstr != 0 )
        {
            //if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
             //   printf("add_MGWaddr(%s) from (%s)\n",jsonstr,sender!=0?sender:"");
            //broadcast_bindAM(msig->NXTaddr,msig,origargstr);
            //update_MGW_msigfile(0,msig,jsonstr);
           // update_MGW_msigfile(msig->NXTaddr,msig,jsonstr);
            update_MGW_jsonfile(set_MGW_msigfname,extract_jsonmsig,jsonmsigcmp,0,jsonstr,0,0);
            appendflag = update_MGW_jsonfile(set_MGW_msigfname,extract_jsonmsig,jsonmsigcmp,msig->NXTaddr,jsonstr,0,0);
            free(jsonstr);
        }
    }
    return(appendflag);
}

void broadcast_bindAM(char *refNXTaddr,struct multisig_addr *msig,char *origargstr)
{
    struct coin_info *cp = get_coin_info("BTCD");
    char *jsontxt,*AMtxid,AM[4096];
    struct json_AM *ap = (struct json_AM *)AM;
    if ( cp != 0 && (jsontxt= create_multisig_json(msig,1)) != 0 )
    {
        printf(">>>>>>>>>>>>>>>>>>>>>>>>>> send bind address AM\n");
        set_json_AM(ap,GATEWAY_SIG,BIND_DEPOSIT_ADDRESS,refNXTaddr,0,origargstr!=0?origargstr:jsontxt,1);
        AMtxid = submit_AM(0,refNXTaddr,&ap->H,0,cp->srvNXTACCTSECRET);
        if ( AMtxid != 0 )
            free(AMtxid);
        free(jsontxt);
    }
}

struct multisig_addr *MSIG_table,**MSIGS;
portable_mutex_t MSIGmutex;
int32_t didMSIGinit,Num_MSIGS;
struct multisig_addr *find_msigaddr(char *msigaddr)
{
    int32_t i;//createdflag;
    struct multisig_addr *msig = 0;
    if ( didMSIGinit == 0 )
    {
        portable_mutex_init(&MSIGmutex);
        didMSIGinit = 1;
    }
    portable_mutex_lock(&MSIGmutex);
    //HASH_FIND(hh,MSIG_table,msigaddr,strlen(msigaddr),msig);
    for (i=0; i<Num_MSIGS; i++)
        if ( strcmp(msigaddr,MSIGS[i]->multisigaddr) == 0 )
        {
            msig = MSIGS[i];
            break;
        }
    portable_mutex_unlock(&MSIGmutex);
    return(msig);
    /*if ( MTsearch_hashtable(&SuperNET_dbs[MULTISIG_DATA].ramtable,msigaddr) == HASHSEARCH_ERROR )
     {
     printf("(%s) not MGW multisig addr\n",msigaddr);
     return(0);
     }
     printf("found (%s)\n",msigaddr);
     return(MTadd_hashtable(&createdflag,&SuperNET_dbs[MULTISIG_DATA].ramtable,msigaddr));*/ // only do this if it is already there
    //return((struct multisig_addr *)find_storage(MULTISIG_DATA,msigaddr,0));
}

struct multisig_addr *ram_add_msigaddr(char *msigaddr,int32_t n,char *NXTaddr,char *NXTpubkey,int32_t buyNXT)
{
    struct multisig_addr *msig;
    if ( (msig= find_msigaddr(msigaddr)) == 0 )
    {
        msig = calloc(1,sizeof(*msig) + n*sizeof(struct pubkey_info));
        strcpy(msig->multisigaddr,msigaddr);
        if ( NXTaddr != 0 )
            strcpy(msig->NXTaddr,NXTaddr);
        if ( NXTpubkey != 0 )
            strcpy(msig->NXTpubkey,NXTpubkey);
        if ( buyNXT >= 0 )
            msig->buyNXT = buyNXT;
        if ( didMSIGinit == 0 )
        {
            //portable_mutex_init(&MSIGmutex);
            didMSIGinit = 1;
        }
        //printf("ram_add_msigaddr MSIG[%s] NXT.%s (%s) buyNXT.%d\n",msigaddr,msig->NXTaddr,msig->NXTpubkey,msig->buyNXT);
        portable_mutex_lock(&MSIGmutex);
        MSIGS = realloc(MSIGS,(1+Num_MSIGS) * sizeof(*MSIGS));
        MSIGS[Num_MSIGS] = msig, Num_MSIGS++;
        //HASH_ADD_KEYPTR(hh,MSIG_table,clonestr(msigaddr),strlen(msigaddr),msig);
        portable_mutex_unlock(&MSIGmutex);
        //printf("done ram_add_msigaddr MSIG[%s] NXT.%s (%s) buyNXT.%d\n",msigaddr,msig->NXTaddr,msig->NXTpubkey,msig->buyNXT);
    }
    return(msig);
}

int32_t update_msig_info(struct multisig_addr *msig,int32_t syncflag,char *sender)
{
    DBT key,data,*datap;
    int32_t i,ret = 0;
    struct multisig_addr *msigram;
    struct SuperNET_db *sdb = &SuperNET_dbs[MULTISIG_DATA];
    update_MGW_msig(msig,sender);
    if ( IS_LIBTEST <= 0 )
        return(-1);
    if ( msig == 0 )
    {
        if ( syncflag != 0 && sdb != 0 && sdb->storage != 0 )
            return(dbsync(sdb,0));
        return(0);
    }
    for (i=0; i<msig->n; i++)
        if ( msig->pubkeys[i].nxt64bits != 0 && msig->pubkeys[i].coinaddr[0] != 0 && msig->pubkeys[i].pubkey[0] != 0 )
            add_NXT_coininfo(msig->pubkeys[i].nxt64bits,calc_nxt64bits(msig->NXTaddr),msig->coinstr,msig->pubkeys[i].coinaddr,msig->pubkeys[i].pubkey);
    if ( msig->H.size == 0 )
        msig->H.size = sizeof(*msig) + (msig->n * sizeof(msig->pubkeys[0]));
    msigram = ram_add_msigaddr(msig->multisigaddr,msig->n,msig->NXTaddr,msig->NXTpubkey,msig->buyNXT);//MTadd_hashtable(&createdflag,&sdb->ramtable,msig->multisigaddr);
    if ( msigram->created != 0 && msig->created != 0 )
    {
        if ( msigram->created < msig->created )
            msig->created = msigram->created;
        else msigram->created = msig->created;
    }
    else if ( msig->created == 0 )
        msig->created = msigram->created;
    if ( msigram->NXTpubkey[0] != 0 && msig->NXTpubkey[0] == 0 )
        safecopy(msig->NXTpubkey,msigram->NXTpubkey,sizeof(msig->NXTpubkey));
    //if ( msigram->sender == 0 && msig->sender != 0 )
    //    createdflag = 1;
    if ( memcmp(msigram,msig,msig->H.size) != 0 ) //createdflag != 0 ||
    {
        clear_pair(&key,&data);
        key.data = msig->multisigaddr;
        key.size = (int32_t)(strlen(msig->multisigaddr) + 1);
        data.size = msig->H.size;
        if ( sdb->overlap_write != 0 )
        {
            data.data = calloc(1,msig->H.size);
            memcpy(data.data,msig,msig->H.size);
            datap = calloc(1,sizeof(*datap));
            *datap = data;
        }
        else
        {
            data.data = msig;
            datap = &data;
        }
        if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
            printf("add (%s) NXTpubkey.(%s) sdb.%p\n",msig->multisigaddr,msig->NXTpubkey,sdb->dbp);
        if ( sdb != 0 && sdb->storage != 0 )
        {
            if (  (ret= dbput(sdb,0,&key,datap,0)) != 0 )
                sdb->storage->err(sdb->storage,ret,"Database put for quote failed.");
            else if ( syncflag != 0 ) ret = dbsync(sdb,0);
        }
        ret = 1;
    }
    return(ret);
}

int32_t update_MGWaddr(cJSON *argjson,char *sender)
{
    int32_t i,retval = 0;
    uint64_t senderbits;
    struct multisig_addr *msig;
    if  ( (msig= decode_msigjson(0,argjson,sender)) != 0 )
    {
        senderbits = calc_nxt64bits(sender);
        for (i=0; i<msig->n; i++)
        {
            if ( msig->pubkeys[i].nxt64bits == senderbits )
            {
                update_msig_info(msig,1,sender);
                update_MGW_msig(msig,sender);
                retval = 1;
                break;
            }
        }
        free(msig);
    }
    return(retval);
}

int32_t add_MGWaddr(char *previpaddr,char *sender,int32_t valid,char *origargstr)
{
    cJSON *origargjson,*argjson;
    if ( valid > 0 && (origargjson= cJSON_Parse(origargstr)) != 0 )
    {
        if ( is_cJSON_Array(origargjson) != 0 )
            argjson = cJSON_GetArrayItem(origargjson,0);
        else argjson = origargjson;
        return(update_MGWaddr(argjson,sender));
    }
    return(0);
}

int32_t _update_redeembits(struct coin_info *cp,uint64_t redeembits,uint64_t AMtxidbits)
{
    struct NXT_asset *ap;
    int32_t createdflag;
    int32_t i,n = 0,num = 0;
    if ( cp == 0 )
        return(0);
    if ( cp->RAM.limboarray != 0 )
        for (n=0; cp->RAM.limboarray[n]!=0; n++)
            if ( cp->RAM.limboarray[n] == redeembits )
                break;
    if ( cp->RAM.limboarray[n] != redeembits )
    {
        cp->RAM.limboarray = realloc(cp->RAM.limboarray,sizeof(*cp->RAM.limboarray) * (n+2));
        cp->RAM.limboarray[n++] = redeembits;
        cp->RAM.limboarray[n] = 0;
        num++;
    }
    if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
        printf("n.%d set AMtxidbits.%llu -> %s.(%llu)\n",n,(long long)AMtxidbits,cp->name,(long long)redeembits);
    if ( cp != 0 )
    {
        ap = get_NXTasset(&createdflag,Global_mp,cp->assetid);
        if ( ap->num > 0 )
        {
            for (i=0; i<ap->num; i++)
                if ( ap->txids[i]->redeemtxid == redeembits )
                {
                    ap->txids[i]->AMtxidbits = AMtxidbits;
                    num++;
                }
        }
    }
    return(num);
}

int32_t update_redeembits(struct coin_info *refcp,cJSON *argjson,uint64_t AMtxidbits)
{
    cJSON *array;
    int32_t i,n,num = 0;
    struct coin_info *cp;
    char coinstr[MAX_JSON_FIELD],redeemtxid[MAX_JSON_FIELD];
    if ( extract_cJSON_str(coinstr,sizeof(coinstr),argjson,"coin") <= 0 )
        return(0);
    if ( refcp != 0 && strcmp(refcp->name,coinstr) != 0 )
        return(0);
    cp = get_coin_info(coinstr);
    array = cJSON_GetObjectItem(argjson,"redeems");
    if ( array != 0 && is_cJSON_Array(array) != 0 )
    {
        n = cJSON_GetArraySize(array);
        for (i=0; i<n; i++)
        {
            copy_cJSON(redeemtxid,cJSON_GetArrayItem(array,i));
            if ( redeemtxid[0] != 0 && is_limbo_redeem(cp,calc_nxt64bits(redeemtxid)) == 0 )
                num += _update_redeembits(cp,calc_nxt64bits(redeemtxid),AMtxidbits);
        }
    }
    if ( extract_cJSON_str(redeemtxid,sizeof(redeemtxid),argjson,"redeemtxid") > 0 && is_limbo_redeem(cp,calc_nxt64bits(redeemtxid)) == 0 )
        num += _update_redeembits(cp,calc_nxt64bits(redeemtxid),AMtxidbits);
    return(num);
}

struct NXT_assettxid *set_assettxid(char **specialNXTaddrs,struct coin_info *cp,struct NXT_asset *ap,uint64_t redeemtxid,uint64_t senderbits,uint64_t receiverbits,uint32_t timestamp,char *commentstr,uint64_t quantity)
{
    struct NXT_assettxid *tp;
    int32_t createdflag;
    cJSON *json,*cointxidobj;
    char sender[MAX_JSON_FIELD],redeemtxidstr[MAX_JSON_FIELD],cointxid[MAX_JSON_FIELD],coinstr[MAX_JSON_FIELD];
    expand_nxt64bits(redeemtxidstr,redeemtxid);
    tp = find_NXT_assettxid(&createdflag,ap,redeemtxidstr);
    tp->assetbits = ap->assetbits;
    tp->redeemtxid = redeemtxid;
    if ( timestamp > tp->timestamp )
        tp->timestamp = timestamp;
    tp->quantity = quantity;
    tp->U.assetoshis = (quantity * ap->mult);
    tp->receiverbits = receiverbits;
    tp->senderbits = senderbits;
    if ( commentstr != 0 && (tp->comment == 0 || strcmp(tp->comment,commentstr) != 0) && (json= cJSON_Parse(commentstr)) != 0 )
    {
        copy_cJSON(coinstr,cJSON_GetObjectItem(json,"coin"));
        if ( coinstr[0] == 0 )
            strcpy(coinstr,cp->name);
        if ( strcmp(coinstr,cp->name) == 0 )
        {
            if ( tp->comment != 0 )
                free(tp->comment);
            tp->comment = commentstr;
            stripwhite_ns(tp->comment,strlen(tp->comment));
            tp->buyNXT = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"buyNXT"),0);
            tp->coinblocknum = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"coinblocknum"),0);
            tp->cointxind = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"cointxind"),0);
            tp->coinv = (uint32_t)get_API_int(cJSON_GetObjectItem(json,"coinv"),0);
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
                    if ( in_specialNXTaddrs(specialNXTaddrs,sender) != 0 && tp->sentNXT != tp->buyNXT )
                    {
                        cp->boughtNXT -= tp->sentNXT;
                        cp->boughtNXT += tp->buyNXT;
                        tp->sentNXT = tp->buyNXT;
                        if ( cp->NXTfee_equiv != 0 && cp->txfee != 0 )
                            tp->estNXT = (((double)cp->NXTfee_equiv / cp->txfee) * tp->U.assetoshis / SATOSHIDEN);
                     }
                }
                if ( Debuglevel > 1 )
                    printf("sender.%llu receiver.%llu got.(%llu) comment.(%s) cointxidstr.(%s) buyNXT.%d\n",(long long)senderbits,(long long)receiverbits,(long long)redeemtxid,tp->comment,cointxid,tp->buyNXT);
            }
            else
            {
                if ( Debuglevel > 1 )
                    printf("%s txid.(%s) got comment.(%s) gotpossibleredeem.(%d.%d.%d) %.8f/%.8f NXTequiv %.8f -> redeemtxid.%llu\n",ap->name,redeemtxidstr,tp->comment!=0?tp->comment:"",tp->coinblocknum,tp->cointxind,tp->coinv,dstr(tp->quantity * ap->mult),dstr(tp->U.assetoshis),tp->estNXT,(long long)tp->redeemtxid);
            }
        } else printf("mismatched coin.%s vs (%s) for transfer.%llu (%s)\n",coinstr,cp->name,(long long)redeemtxid,commentstr);
        free_json(json);
    }
    return(tp);
}

int32_t init_deposit(char **specialNXTaddrs,struct coin_info *cp)
{
    FILE *fp;
    long len,n;
    uint32_t timestamp;
    int32_t i,createdflag,num = 0;
    cJSON *json,*item;
    struct NXT_asset *ap;
    uint64_t quantity;
    char fname[512],depositidstr[MAX_JSON_FIELD],sender[MAX_JSON_FIELD],receiver[MAX_JSON_FIELD],*buf;
    ap = get_NXTasset(&createdflag,Global_mp,cp->assetid);
    set_MGW_moneysentfname(fname,0);
    if ( (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
    {
        fseek(fp,0,SEEK_END);
        len = ftell(fp);
        rewind(fp);
        buf = calloc(1,len);
        if ( (n= fread(buf,1,len,fp)) == len )
        {
            if ( (json= cJSON_Parse(buf)) != 0 )
            {
                if ( is_cJSON_Array(json) != 0 && (n= cJSON_GetArraySize(json)) > 0 )
                {
                    for (i=0; i<n; i++)
                    {
                        item = cJSON_GetArrayItem(json,i);
                        copy_cJSON(depositidstr,cJSON_GetObjectItem(item,"depositid"));
                        copy_cJSON(sender,cJSON_GetObjectItem(item,"senderbits"));
                        copy_cJSON(receiver,cJSON_GetObjectItem(item,"receiverbits"));
                        timestamp = (uint32_t)get_API_int(cJSON_GetObjectItem(item,"timestamp"),0);
                        quantity = get_API_nxt64bits(cJSON_GetObjectItem(item,"quantity"));
                        set_assettxid(specialNXTaddrs,cp,ap,calc_nxt64bits(depositidstr),calc_nxt64bits(sender),calc_nxt64bits(receiver),timestamp,cJSON_Print(item),quantity);
                        num++;
                    }
                } else printf("(%s) (%s) is not array or n.%ld is too small\n",fname,buf,n);
                free_json(json);
            } else printf("error parsing (%s) (%s)\n",fname,buf);
        } else printf("error reading in (%s) len %ld != size %ld\n",fname,n,len);
        fclose(fp);
        free(buf);
    }
    printf("loaded %d deposits locally\n",num);
    return(num);
}

int32_t init_moneysent(char **specialNXTaddrs,struct coin_info *cp)
{
    FILE *fp;
    long len,n;
    int32_t i,num = 0;
    cJSON *json,*item;
    uint64_t AMtxidbits;
    char fname[512],AMtxidstr[MAX_JSON_FIELD],*buf;
    set_MGW_moneysentfname(fname,0);
    if ( (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
    {
        fseek(fp,0,SEEK_END);
        len = ftell(fp);
        rewind(fp);
        buf = calloc(1,len);
        if ( (n= fread(buf,1,len,fp)) == len )
        {
            if ( (json= cJSON_Parse(buf)) != 0 )
            {
                if ( is_cJSON_Array(json) != 0 && (n= cJSON_GetArraySize(json)) > 0 )
                {
                    for (i=0; i<n; i++)
                    {
                        item = cJSON_GetArrayItem(json,i);
                        copy_cJSON(AMtxidstr,cJSON_GetObjectItem(item,"AMtxid"));
                        if ( AMtxidstr[0] != 0 )
                            AMtxidbits = calc_nxt64bits(AMtxidstr);
                        else AMtxidbits = 1;
                        num += update_redeembits(cp,item,AMtxidbits);
                    }
                } else printf("(%s) (%s) is not array or n.%ld is too small\n",fname,buf,n);
                free_json(json);
            } else printf("error parsing (%s) (%s)\n",fname,buf);
        } else printf("error reading in (%s) len %ld != size %ld\n",fname,n,len);
        fclose(fp);
        free(buf);
    }
    printf("loaded %d moneysent locally\n",num);
    return(num);
}

int32_t init_multisig(char **specialNXTaddrs,struct coin_info *cp)
{
    FILE *fp;
    long len,n;
    int32_t i,num = 0;
    cJSON *json;
    char fname[512],*buf;
    set_MGW_msigfname(fname,0);
    if ( (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
    {
        fseek(fp,0,SEEK_END);
        len = ftell(fp);
        rewind(fp);
        buf = calloc(1,len);
        if ( (n= fread(buf,1,len,fp)) == len )
        {
            if ( (json= cJSON_Parse(buf)) != 0 )
            {
                if ( is_cJSON_Array(json) != 0 && (n= cJSON_GetArraySize(json)) > 0 )
                {
                    for (i=0; i<n; i++)
                        num += update_MGWaddr(cJSON_GetArrayItem(json,i),Global_mp->myNXTADDR);
                    
                } else printf("(%s) (%s) is not array or n.%ld is too small\n",fname,buf,n);
                free_json(json);
            } else printf("error parsing (%s) (%s)\n",fname,buf);
        } else printf("error reading in (%s) len %ld != size %ld\n",fname,n,len);
        fclose(fp);
        free(buf);
    }
    printf("loaded %d multisig addrs locally\n",num);
    return(num);
}

char *create_batch_jsontxt(struct coin_info *cp,int *firstitemp)
{
    struct rawtransaction *rp = &cp->BATCH.rawtx;
    cJSON *json,*obj,*array = 0;
    char *jsontxt,redeemtxid[128];
    int32_t i,ind;
    json = cJSON_CreateObject();
    //obj = cJSON_CreateNumber(cp->coinid); cJSON_AddItemToObject(json,"coinid",obj);
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

uint64_t broadcast_moneysentAM(struct coin_info *cp,int32_t height)
{
    cJSON *argjson;
    uint64_t AMtxidbits = 0;
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
                for (i=0; i<cp->BATCH.rawtx.numredeems; i++)
                    printf("%llu ",(long long)cp->BATCH.rawtx.redeems[i]);
                printf("broadcast_moneysentAM: %s failed. FATAL need to manually mark transaction PAID %s JSON.(%s)\n",cp->name,cp->BATCH.W.cointxid,jsontxt), sleep(60);
                fprintf(stderr,"broadcast_moneysentAM: %s failed. FATAL need to manually mark transaction PAID %s JSON.(%s)\n",cp->name,cp->BATCH.W.cointxid,jsontxt), sleep(60);
                exit(-1);
            }
            else
            {
                AMtxidbits = calc_nxt64bits(AMtxid);
                free(AMtxid);
                if ( AMtxidbits != 0 )
                    add_pendingxfer(0,AMtxidbits);
                argjson = cJSON_Parse(jsontxt);
                if ( argjson != 0 )
                    update_redeembits(cp,argjson,AMtxidbits); //update_money_sent(argjson,AMtxid,height);
                else
                {
                    for (i=0; i<cp->BATCH.rawtx.numredeems; i++)
                        printf("%llu ",(long long)cp->BATCH.rawtx.redeems[i]);
                    printf("broadcast_moneysentAM: %s failed. AMtxid.%llu FATAL need to manually mark transaction PAID %s JSON.(%s)\n",cp->name,(long long)AMtxid,cp->BATCH.W.cointxid,jsontxt);
                    fprintf(stderr,"broadcast_moneysentAM: %s failed. AMtxid.%llu FATAL need to manually mark transaction PAID %s JSON.(%s)\n",cp->name,(long long)AMtxid,cp->BATCH.W.cointxid,jsontxt);
                    exit(-1);
                }
            }
            free(jsontxt);
        }
        else
        {
            for (i=0; i<cp->BATCH.rawtx.numredeems; i++)
                printf("%llu ",(long long)cp->BATCH.rawtx.redeems[i]);
            printf("broadcast_moneysentAM: %s failed. FATAL need to manually mark transaction PAID %s\n",cp->name,cp->BATCH.W.cointxid);
            fprintf(stderr,"broadcast_moneysentAM: %s failed. FATAL need to manually mark transaction PAID %s\n",cp->name,cp->BATCH.W.cointxid);
            exit(-1);
        }
    }
    return(AMtxidbits);
}


// ADDRESS_DATA DB
int32_t set_address_entry(struct address_entry *bp,uint32_t blocknum,int32_t txind,int32_t vin,int32_t vout,int32_t isinternal,int32_t spent)
{
    memset(bp,0,sizeof(*bp));
    bp->blocknum = blocknum;
    bp->txind = txind;
    bp->isinternal = isinternal;
    bp->spent = spent;
    if ( vout >= 0 && vin < 0 )
    {
        bp->v = vout;
        if ( bp->v != vout )
            return(-1);
    }
    else if ( vin >= 0 && vout < 0 )
    {
        bp->v = vin, bp->vinflag = 1;
        if ( bp->v != vin )
            return(-1);
    }
    if ( bp->blocknum != blocknum || bp->txind != txind )
        return(-1);
    return(0);
}

int32_t add_address_entry(int32_t numvins,uint64_t inputsum,int32_t numvouts,uint64_t remainder,char *coin,char *addr,uint32_t blocknum,int32_t txind,int32_t vin,int32_t vout,int32_t isinternal,int32_t spent,int32_t syncflag,uint64_t value,char *txidstr,char *script)
{
    struct address_entry B;
    struct coin_info *cp;
    struct multisig_addr *msig;
    if ( IS_LIBTEST > 1 )
    {
        if ( set_address_entry(&B,blocknum,txind,vin,vout,isinternal,spent) == 0 )
        {
            _add_address_entry(numvins,inputsum,numvouts,remainder,coin,addr,&B,syncflag,value,txidstr,script);
            if ( Global_mp->gatewayid == (NUM_GATEWAYS-1) && isinternal == 0 && vin < 0 && MGW_initdone != 0 && (cp= get_coin_info(coin)) != 0 )
            {
                if ( (msig= find_msigaddr(addr)) != 0 && msig->NXTaddr[0] != 0 )
                {
                    printf("queue DepositQ for NXT.(%s) %s %.8f\n",msig->NXTaddr,addr,dstr(value));
                    queue_enqueue("DepositQ",&DepositQ,queueitem(addr));
                }
            }
            return(0);
        } else printf("Error creating address entry %s.%s (%u %u %d:%d) %.8f\n",coin,addr,blocknum,txind,vin,vout,dstr(value));
    }
    return(-1);
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

uint64_t get_txvout(char *blockhash,int32_t *numvoutsp,char *coinaddr,char *script,struct coin_info *cp,cJSON *txjson,char *txidstr,int32_t vout)
{
    char *retstr;
    uint64_t value = 0;
    int32_t numvouts,flag = 0;
    cJSON *vouts,*obj;
    if ( numvoutsp != 0 )
        *numvoutsp = 0;
    coinaddr[0] = 0;
    if ( script != 0 )
        script[0] = 0;
    if ( txjson == 0 && txidstr != 0 && txidstr[0] != 0 )
    {
        retstr = _get_transaction(&cp->RAM,txidstr);
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
                _extract_txvals(coinaddr,script,cp->nohexout,obj);
                if ( coinaddr[0] == 0 )
                    printf("(%s) obj.%p vouts.%p num.%d vs %d %s\n",coinaddr,obj,vouts,vout,numvouts,cJSON_Print(txjson));
                if ( script != 0 && script[0] == 0 && value > 0 )
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

uint64_t update_vins(int32_t *numvinsp,int32_t *isinternalp,char *coinaddr,char *script,struct coin_info *cp,uint32_t blockheight,int32_t txind,cJSON *vins,int32_t vind,int32_t syncflag)
{
    uint32_t get_blocktxind(int32_t *txindp,struct coin_info *cp,uint32_t blockheight,char *blockhashstr,char *txidstr);
    cJSON *obj,*txidobj,*coinbaseobj;
    uint64_t value,sum = 0;
    int32_t i,vout,numvins,numvouts,oldtxind,flag = 0;
    char txidstr[1024],coinbase[1024],blockhash[1024];
    uint32_t oldblockheight;
    *numvinsp = 0;
    if ( vins != 0 && is_cJSON_Array(vins) != 0 && (numvins= cJSON_GetArraySize(vins)) > 0 )
    {
        *numvinsp = numvins;
        for (i=0; i<numvins; i++)
        {
            if ( vind >= 0 && vind != i )
                continue;
            obj = cJSON_GetArrayItem(vins,i);
            if ( numvins == 1  )
            {
                coinbaseobj = cJSON_GetObjectItem(obj,"coinbase");
                copy_cJSON(coinbase,coinbaseobj);
                if ( 1 && strlen(coinbase) > 1 )
                {
                    if ( txind > 0 )
                        printf("txind.%d is coinbase.%s\n",txind,coinbase);
                    return(0);
                }
                //printf("process input.(%s) coinbase.(%s) sum.(%.8f)\n",coinaddr,coinbase,dstr(sum));
            } else coinbase[0] = 0;
            txidobj = cJSON_GetObjectItem(obj,"txid");
            if ( txidobj != 0 && cJSON_GetObjectItem(obj,"vout") != 0 )
            {
                vout = (int)get_cJSON_int(obj,"vout");
                copy_cJSON(txidstr,txidobj);
                if ( txidstr[0] != 0 && (value= get_txvout(blockhash,&numvouts,coinaddr,script,cp,0,txidstr,vout)) != 0 && blockhash[0] != 0 )
                {
                    sum += value;
                    if ( (oldblockheight= get_blocktxind(&oldtxind,cp,0,blockhash,txidstr)) > 0 )
                    {
                        flag++;
                        add_address_entry(0,0,0,0,cp->name,coinaddr,blockheight,txind,i,-1,-1,1,0,value,0,0);
                        add_address_entry(0,0,0,0,cp->name,coinaddr,oldblockheight,oldtxind,-1,vout,-1,1,syncflag * (i == (numvins-1)),value,0,0);
                    } else printf("error getting oldblockheight (%s %s)\n",blockhash,txidstr);
                } else printf("unexpected error vout.%d %s\n",vout,txidstr);
            } else printf("illegal txid.(%s)\n",txidstr);
        }
    }
    return(sum);
}

void update_txid_infos(struct coin_info *cp,uint32_t blockheight,int32_t txind,char *txidstr,int32_t syncflag,uint64_t minted)
{
    char coinaddr[1024],script[4096],coinaddr_v0[1024],*retstr = 0;
    int32_t v,tmp,numvouts,numvins,isinternal = 0;
    uint64_t value,remainder,inputsum = 0;
    cJSON *txjson;
    if ( (retstr= _get_transaction(&cp->RAM,txidstr)) != 0 )
    {
        if ( (txjson= cJSON_Parse(retstr)) != 0 )
        {
            inputsum = update_vins(&numvins,&isinternal,coinaddr,script,cp,blockheight,txind,cJSON_GetObjectItem(txjson,"vin"),-1,syncflag);
            if ( inputsum == 0 && txind == 0 )
                inputsum = minted;
            v = 0;
            remainder = inputsum;
            if ( (value= get_txvout(0,&numvouts,coinaddr_v0,script,cp,txjson,0,v)) > 0 )
                add_address_entry(numvins,inputsum,numvouts,remainder,cp->name,coinaddr_v0,blockheight,txind,-1,v,isinternal,0,syncflag * (v == (numvouts-1)),value,txidstr,script), remainder -= value;
            for (v=1; v<numvouts; v++)
            {
                if ( v < numvouts && (value= get_txvout(0,&tmp,coinaddr,script,cp,txjson,0,v)) > 0 )
                {
                    isinternal = calc_isinternal(cp,coinaddr_v0,blockheight,v,numvouts);
                    add_address_entry(numvins,inputsum,numvouts,remainder,cp->name,coinaddr,blockheight,txind,-1,v,isinternal,0,syncflag * (v == (numvouts-1)),value,txidstr,script), remainder -= value;
                }
            }
            free_json(txjson);
        } else printf("update_txid_infos parse error.(%s)\n",retstr);
        free(retstr);
    } else printf("error getting.(%s)\n",txidstr);
}

uint32_t get_blocktxind(int32_t *txindp,struct coin_info *cp,uint32_t blockheight,char *blockhashstr,char *reftxidstr)
{
    char txidstr[MAX_JSON_FIELD],mintedstr[MAX_JSON_FIELD];
    cJSON *json,*txobj;
    int32_t txind,n;
    uint64_t minted = 0;
    uint32_t blockid = 0;
    *txindp = -1;
    if ( (json= _get_blockjson(0,&cp->RAM,blockhashstr,blockheight)) != 0 )
    {
        copy_cJSON(mintedstr,cJSON_GetObjectItem(json,"mint"));
        if ( mintedstr[0] != 0 )
            minted = (uint64_t)(atof(mintedstr) * SATOSHIDEN);
        if ( (txobj= _get_blocktxarray(&blockid,&n,&cp->RAM,json)) != 0 )
        {
            if ( blockheight == 0 )
                blockheight = blockid;
            for (txind=0; txind<n; txind++)
            {
                copy_cJSON(txidstr,cJSON_GetArrayItem(txobj,txind));
                if ( blockheight == 0 || Debuglevel > 3 )
                    printf("%-5s blocktxt.%ld i.%d of n.%d %s\n",cp->name,(long)blockheight,txind,n,txidstr);
                if ( reftxidstr != 0 )
                {
                    if ( txidstr[0] != 0 && strcmp(txidstr,reftxidstr) == 0 )
                    {
                        *txindp = txind;
                        break;
                    }
                }  else update_txid_infos(cp,blockheight,txind,txidstr,txind == n-1,minted);
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
    if ( (blockhashstr = _get_blockhashstr(&cp->RAM,blockheight)) != 0 )
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
    if ( (json= _get_blockjson(0,&cp->RAM,0,blockheight)) != 0 )
    {
        if ( (txobj= _get_blocktxarray(&blockid,&n,&cp->RAM,json)) != 0 && txind < n )
        {
            copy_cJSON(txidstr,cJSON_GetArrayItem(txobj,txind));
            value = get_txvout(0,numvoutsp,coinaddr,script,cp,0,txidstr,vout);
            if ( Debuglevel > 3 || strcmp("bbGihDgrR8kNrDspfSvb2wrPgeha5tcYgn",coinaddr) == 0 )
                printf("%-5s (%s) blocktxt.%ld txind.%d of n.%d %s vout.%d\n",cp->name,coinaddr,(long)blockheight,txind,n,txidstr,vout);
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
        if ( (json= _get_blockjson(&blocknum,&cp->RAM,blockhash,blocknum)) != 0 )
        {
            if ( (txarray= _get_blocktxarray(&blockid,&n,&cp->RAM,json)) != 0 )
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
    if ( (json= _get_blockjson(0,&cp->RAM,0,blockheight)) != 0 )
    {
        if ( (txobj= _get_blocktxarray(&blockid,&n,&cp->RAM,json)) != 0 && txind < n )
        {
            copy_cJSON(input_txid,cJSON_GetArrayItem(txobj,txind));
            if ( Debuglevel > 3 )
                printf("%-5s blocktxt.%ld i.%d of n.%d %s\n",cp->name,(long)blockheight,txind,n,input_txid);
            if ( input_txid[0] != 0 && (retstr= _get_transaction(&cp->RAM,input_txid)) != 0 )
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
        fprintf(stderr,"%p (%s).%d %s input.%d value %.8f | sum %.8f amount %.8f\n",vp,vp->txid,vp->entry.v,vp->coinaddr,rp->numinputs,dstr(vp->value),dstr(sum),dstr(amount));
        rp->inputs[rp->numinputs++] = vp;
        if ( sum >= (amount + cp->txfee) )
        {
            if ( 0 && (vp= up->vps[up->num - 1]) != 0 )
            {
                sum += vp->value;
                rp->inputs[rp->numinputs++] = vp;
                fprintf(stderr,"CABOOSE %p (%s).%d %s input.%d value %.8f | sum %.8f amount %.8f\n",vp,vp->txid,vp->entry.v,vp->coinaddr,rp->numinputs,dstr(vp->value),dstr(sum),dstr(amount));
            }
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
    char *marker = get_marker(cp->name);
    if ( rp->destaddrs[0] == 0 || strcmp(rp->destaddrs[0],marker) != 0 )
        rp->destaddrs[0] = clonestr(marker);
    rp->destamounts[0] = MGWfee;
    return(1);
}

struct rawoutput_entry { char destaddr[MAX_COINADDR_LEN]; uint64_t redeemtxid; double amount; };
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
            sortbuf[i-1].redeemtxid = rp->redeems[i];
            strcpy(sortbuf[i-1].destaddr,rp->destaddrs[i]);
            //fprintf(stderr,"%d of %d: %s %.8f\n",i-1,rp->numoutputs,sortbuf[i-1].destaddr,dstr(sortbuf[i-1].amount));
        }
        revsortstrs(&sortbuf[0].destaddr[0],rp->numoutputs-1,sizeof(sortbuf[0]));
        //fprintf(stderr,"SORTED\n");
        for (i=0; i<rp->numoutputs-1; i++)
        {
            rp->destamounts[i+1] = sortbuf[i].amount;
            rp->redeems[i+1] = sortbuf[i].redeemtxid;
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
    if ( up != 0 && up->minvp != 0 && up->minvp->coinaddr[0] != 0 )
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
    rp->batchcrc = _crc32(0,rawbytes+12,rp->batchsize-12); // skip past timediff
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
                if ( retstr != 0 )
                    free(retstr);
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
        if ( (MGW_initdone == 0 && Debuglevel > 3) || (MGW_initdone != 0 && Debuglevel > 2) )
            printf("got retstr.(%s)\n",retstr);
        json = cJSON_Parse(retstr);
        if ( json != 0 )
        {
            pubobj = cJSON_GetObjectItem(json,"pubkey");
            copy_cJSON(pubkey,pubobj);
            if ( Debuglevel > 2 )
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

char *sign_and_sendmoney(uint64_t *AMtxidp,struct coin_info *cp,int32_t height)
{
    char txidstr[64],NXTaddr[64],jsonstr[4096],*retstr = 0;
    int32_t i;
    uint64_t amount,senderbits,redeemtxid;
    *AMtxidp = 0;
    fprintf(stderr,"achieved consensus and sign! %s\n",cp->BATCH.rawtx.batchsigned);
    if ( (retstr= submit_withdraw(cp,&cp->BATCH,&cp->withdrawinfos[(Global_mp->gatewayid + 1) % NUM_GATEWAYS])) != 0 )
    {
        safecopy(cp->BATCH.W.cointxid,retstr,sizeof(cp->BATCH.W.cointxid));
        *AMtxidp = broadcast_moneysentAM(cp,height);
        for (i=0; i<cp->BATCH.rawtx.numredeems; i++)
        {
            redeemtxid = cp->BATCH.rawtx.redeems[i];
            expand_nxt64bits(txidstr,redeemtxid);
            senderbits = get_sender(&amount,txidstr);
            expand_nxt64bits(NXTaddr,senderbits);
            sprintf(jsonstr,"{\"NXT\":\"%s\",\"redeemtxid\":\"%llu\",\"AMtxid\":\"%llu\",\"coin\":\"%s\",\"cointxid\":\"%s\"}",NXTaddr,(long long)redeemtxid,(long long)*AMtxidp,cp->name,txidstr);
            update_MGW_jsonfile(set_MGW_moneysentfname,extract_jsonkey,jsonstrcmp,0,jsonstr,"redeemtxid",0);
            update_MGW_jsonfile(set_MGW_moneysentfname,extract_jsonkey,jsonstrcmp,NXTaddr,jsonstr,"redeemtxid",0);
        }
        //backupwallet(cp,cp->coinid);
        return(retstr);
    }
    else printf("sign_and_sendmoney: error sending rawtransaction %s\n",cp->BATCH.rawtx.batchsigned);
    return(0);
}

static int32_t _cmp_vps(const void *a,const void *b)
{
#define vp_a (*(struct coin_txidind **)a)
#define vp_b (*(struct coin_txidind **)b)
	if ( vp_b->value > vp_a->value )
		return(1);
	else if ( vp_b->value < vp_a->value )
		return(-1);
	return(0);
#undef vp_a
#undef vp_b
}

void sort_vps(struct coin_txidind **vps,int32_t num)
{
	qsort(vps,num,sizeof(*vps),_cmp_vps);
}

void update_unspent_funds(struct coin_info *cp,struct coin_txidind *cointp,int32_t n)
{
    struct unspent_info *up = &cp->unspent;
    uint64_t value;
    if ( n == 0 )
        n = 1000;
    if ( n > up->maxvps )
    {
        up->vps = realloc(up->vps,n * sizeof(*up->vps));
        up->maxvps = n;
    }
    if ( cointp == 0 )
    {
        up->num = 0;
        up->maxvp = up->minvp = 0;
        memset(up->vps,0,up->maxvps * sizeof(*up->vps));
        up->maxunspent = up->unspent = up->maxavail = up->minavail = 0;
    }
    else
    {
        value = cointp->value;
        up->maxunspent += value;
        if ( cointp->entry.spent == 0 )
        {
            up->vps[up->num++] = cointp;
            up->unspent += value;
            if ( value > up->maxavail )
            {
                up->maxvp = cointp;
                up->maxavail = value;
            }
            if ( up->minavail == 0 || value < up->minavail )
            {
                up->minavail = value;
                up->minvp = cointp;
            }
        }
    }
}

int32_t map_msigaddr(char *redeemScript,struct coin_info *cp,char *normaladdr,char *msigaddr)
{
    struct coin_info *refcp = get_coin_info("BTCD");
    int32_t i,n,ismine;
    cJSON *json,*array,*json2;
    struct multisig_addr *msig;
    char addr[1024],args[1024],*retstr,*retstr2;
    redeemScript[0] = normaladdr[0] = 0;
    if ( cp == 0 || refcp == 0 || (msig= find_msigaddr(msigaddr)) == 0 )
    {
        strcpy(normaladdr,msigaddr);
        return(0);
    }
   /* {
        "isvalid" : true,
        "address" : "bUNry9zFx9EQnukpUNDgHRsw6zy3eUs8yR",
        "ismine" : true,
        "isscript" : true,
        "script" : "multisig",
        "hex" : "522103a07d28c8d4eaa7e90dc34133fec204f9cf7740d5fd21acc00f9b0552e6bd721e21036d2b86cb74aaeaa94bb82549c4b6dd9666355241d37c371b1e0a17d060dad1c82103ceac7876e4655cf4e39021cf34b7228e1d961a2bcc1f8e36047b40149f3730ff53ae",
        "addresses" : [
                       "RGjegNGJDniYFeY584Adfgr8pX2uQegfoj",
                       "RQWB6GWe67EHCYurSiffYbyZPi7RGcrZa2",
                       "RWVebRCCVMz3YWrZEA9Lc3VWKH9kog5wYg"
                       ],
        "sigsrequired" : 2,
        "account" : ""
    }
*/
    sprintf(args,"\"%s\"",msig->multisigaddr);
    retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"validateaddress",args);
    if ( retstr != 0 )
    {
        //printf("got retstr.(%s)\n",retstr);
        if ( (json = cJSON_Parse(retstr)) != 0 )
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
                        retstr2 = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"validateaddress",args);
                        if ( retstr2 != 0 )
                        {
                            if ( (json2 = cJSON_Parse(retstr2)) != 0 )
                                ismine = is_cJSON_True(cJSON_GetObjectItem(json2,"ismine"));
                            free(retstr2);
                        }
                    }
                    if ( ismine != 0 )
                    {
                        printf("(%s) ismine.%d\n",addr,ismine);
                        strcpy(normaladdr,addr);
                        copy_cJSON(redeemScript,cJSON_GetObjectItem(json,"hex"));
                        break;
                    }
                }
            } free_json(json);
        } free(retstr);
    }
    if ( normaladdr[0] != 0 )
        return(1);
    strcpy(normaladdr,msigaddr);
    return(-1);
}

struct multisig_addr *alloc_multisig_addr(char *coinstr,int32_t m,int32_t n,char *NXTaddr,char *userpubkey,char *sender)
{
    struct multisig_addr *msig;
    int32_t size = (int32_t)(sizeof(*msig) + n*sizeof(struct pubkey_info));
    msig = calloc(1,size);
    msig->H.size = size;
    msig->n = n;
    msig->created = (uint32_t)time(NULL);
    if ( sender != 0 && sender[0] != 0 )
        msig->sender = calc_nxt64bits(sender);
    safecopy(msig->coinstr,coinstr,sizeof(msig->coinstr));
    safecopy(msig->NXTaddr,NXTaddr,sizeof(msig->NXTaddr));
    safecopy(msig->NXTpubkey,userpubkey,sizeof(msig->NXTpubkey));
    msig->m = m;
    return(msig);
}

long calc_pubkey_jsontxt(int32_t truncated,char *jsontxt,struct pubkey_info *ptr,char *postfix)
{
    if ( truncated != 0 )
        sprintf(jsontxt,"{\"address\":\"%s\"}%s",ptr->coinaddr,postfix);
    else sprintf(jsontxt,"{\"address\":\"%s\",\"pubkey\":\"%s\",\"srv\":\"%llu\"}%s",ptr->coinaddr,ptr->pubkey,(long long)ptr->nxt64bits,postfix);
    return(strlen(jsontxt));
}

char *create_multisig_json(struct multisig_addr *msig,int32_t truncated)
{
    long i,len = 0;
    char jsontxt[65536],pubkeyjsontxt[65536],rsacct[64];
    if ( msig != 0 )
    {
        rsacct[0] = 0;
        conv_rsacctstr(rsacct,calc_nxt64bits(msig->NXTaddr));
        pubkeyjsontxt[0] = 0;
        for (i=0; i<msig->n; i++)
            len += calc_pubkey_jsontxt(truncated,pubkeyjsontxt+strlen(pubkeyjsontxt),&msig->pubkeys[i],(i<(msig->n - 1)) ? ", " : "");
        sprintf(jsontxt,"{%s\"sender\":\"%llu\",\"buyNXT\":%u,\"created\":%u,\"M\":%d,\"N\":%d,\"NXTaddr\":\"%s\",\"NXTpubkey\":\"%s\",\"RS\":\"%s\",\"address\":\"%s\",\"redeemScript\":\"%s\",\"coin\":\"%s\",\"gatewayid\":\"%d\",\"pubkey\":[%s]}",truncated==0?"\"requestType\":\"MGWaddr\",":"",(long long)msig->sender,msig->buyNXT,msig->created,msig->m,msig->n,msig->NXTaddr,msig->NXTpubkey,rsacct,msig->multisigaddr,msig->redeemScript,msig->coinstr,Global_mp->gatewayid,pubkeyjsontxt);
        //if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
        //    printf("(%s) pubkeys len.%ld msigjsonlen.%ld\n",jsontxt,len,strlen(jsontxt));
        return(clonestr(jsontxt));
    }
    else return(0);
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
                set_legacy_coinid(coinstr,atoi(numstr));
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
                    if ( ipaddr[0] == 0 && j < 3 )
                        strcpy(ipaddr,Server_ipaddrs[j]);
                    msig->pubkeys[j].ipbits = calc_ipbits(ipaddr);
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

int32_t issue_createmultisig(struct coin_info *cp,struct multisig_addr *msig)
{
    int32_t flag = 0;
    char addr[256];
    cJSON *json,*msigobj,*redeemobj;
    char *params,*retstr = 0;
    params = createmultisig_json_params(msig,(cp->use_addmultisig != 0) ? msig->NXTaddr : 0);
    flag = 0;
    if ( params != 0 )
    {
        if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
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
                    if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
                        printf("addmultisig.(%s)\n",retstr);
                    free_json(json);
                }
                free(retstr);
            } else printf("error issuing createmultisig.(%s)\n",params);
        }
        free(params);
    } else printf("error generating msig params\n");
    return(flag);
}

struct multisig_addr *finalize_msig(struct multisig_addr *msig,uint64_t *srvbits,uint64_t refbits)
{
    int32_t i,n;
    char acctcoinaddr[1024],pubkey[1024];
    for (i=n=0; i<msig->n; i++)
    {
        if ( srvbits[i] != 0 && refbits != 0 )
        {
            acctcoinaddr[0] = pubkey[0] = 0;
            if ( get_NXT_coininfo(srvbits[i],acctcoinaddr,pubkey,refbits,msig->coinstr) != 0 && acctcoinaddr[0] != 0 && pubkey[0] != 0 )
            {
                strcpy(msig->pubkeys[i].coinaddr,acctcoinaddr);
                strcpy(msig->pubkeys[i].pubkey,pubkey);
                msig->pubkeys[i].nxt64bits = srvbits[i];
                n++;
            }
        }
    }
    if ( n != msig->n )
        free(msig), msig = 0;
    return(msig);
}

struct multisig_addr *gen_multisig_addr(char *sender,int32_t M,int32_t N,struct coin_info *cp,char *refNXTaddr,char *userpubkey,struct contact_info **contacts)
{
    uint64_t refbits,srvbits[16];
    int32_t i,flag = 0;
    struct multisig_addr *msig;
    if ( cp == 0 )
        return(0);
    refbits = calc_nxt64bits(refNXTaddr);
    msig = alloc_multisig_addr(cp->name,M,N,refNXTaddr,userpubkey,sender);
    for (i=0; i<N; i++)
        srvbits[i] = contacts[i]->nxt64bits;
    if ( (msig= finalize_msig(msig,srvbits,refbits)) != 0 )
        flag = issue_createmultisig(cp,msig);
    if ( flag == 0 )
    {
        free(msig);
        return(0);
    }
    return(msig);
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

struct multisig_addr *http_search_msig(char *external_NXTaddr,char *external_ipaddr,char *NXTaddr)
{
    int32_t i,n;
    cJSON *array;
    struct multisig_addr *msig = 0;
    if ( (array= http_search(external_ipaddr,"MGW/msig",NXTaddr)) != 0 )
    {
        if ( is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
        {
            for (i=0; i<n; i++)
                if ( (msig= decode_msigjson(0,cJSON_GetArrayItem(array,i),external_NXTaddr)) != 0 && (msig= find_msigaddr(msig->multisigaddr)) != 0 )
                    break;
        }
        free_json(array);
    }
    return(msig);
}

struct multisig_addr *find_NXT_msig(int32_t fixflag,char *NXTaddr,char *coinstr,struct contact_info **contacts,int32_t n)
{
    struct multisig_addr **msigs,*retmsig = 0;
    int32_t i,j,nummsigs;
    uint64_t srvbits[16],nxt64bits;
    for (i=0; i<n; i++)
        srvbits[i] = contacts[i]->nxt64bits;
    if ( (msigs= (struct multisig_addr **)copy_all_DBentries(&nummsigs,MULTISIG_DATA)) != 0 )
    {
        nxt64bits = (NXTaddr != 0) ? calc_nxt64bits(NXTaddr) : 0;
        for (i=0; i<nummsigs; i++)
        {
            if ( fixflag != 0 && msigs[i]->valid != msigs[i]->n )
            {
                if ( finalize_msig(msigs[i],srvbits,nxt64bits) == 0 )
                    continue;
                printf("FIXED %llu -> %s\n",(long long)nxt64bits,msigs[i]->multisigaddr);
                update_msig_info(msigs[i],1,0);
                update_MGW_msig(msigs[i],0);
            }
            if ( nxt64bits != 0 && strcmp(coinstr,msigs[i]->coinstr) == 0 && strcmp(NXTaddr,msigs[i]->NXTaddr) == 0 )
            {
                for (j=0; j<n; j++)
                    if ( srvbits[j] != msigs[i]->pubkeys[j].nxt64bits )
                        break;
                if ( j == n )
                {
                    if ( retmsig != 0 )
                        free(retmsig);
                    retmsig = msigs[i];
                }
            }
            if ( msigs[i] != retmsig )
                free(msigs[i]);
        }
        free(msigs);
    }
    return(retmsig);
}

char *genmultisig(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *coinstr,char *refacct,int32_t M,int32_t N,struct contact_info **oldcontacts,int32_t n,char *userpubkey,char *email,uint32_t buyNXT)
{
    struct coin_info *cp = get_coin_info(coinstr);
    struct multisig_addr *msig;//,*dbmsig;
    struct nodestats *stats;
    struct contact_info *contact,*contacts[16],_contacts[16];
    char refNXTaddr[64],hopNXTaddr[64],destNXTaddr[64],mypubkey[1024],myacctcoinaddr[1024],pubkey[1024],acctcoinaddr[1024],buf[1024],*retstr = 0;
    uint64_t refbits = 0;
    int32_t i,iter,flag,valid = 0;
    if ( cp == 0 )
        return(clonestr("\"error\":\"genmultisig unsupported coin\"}"));
    memset(contacts,0,sizeof(contacts));
    memset(_contacts,0,sizeof(_contacts));
    if ( oldcontacts == 0 && N == 3 )
    {
        for (i=0; i<3; i++) // MGW bypass
        {
            contacts[i] = &_contacts[i];
            contacts[i]->nxt64bits = calc_nxt64bits(Server_NXTaddrs[i]);
        }
        n = i;
    }
    else if ( N == n )
    {
        for (i=0; i<n; i++)
            contacts[i] = oldcontacts[i];
    }
    refbits = conv_acctstr(refacct);
    expand_nxt64bits(refNXTaddr,refbits);
    if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
        printf("GENMULTISIG.%d from (%s) for %s refacct.(%s) %llu %s email.(%s) buyNXT.%u\n",N,previpaddr,cp->name,refacct,(long long)refbits,refNXTaddr,email,buyNXT);
    if ( refNXTaddr[0] == 0 )
        return(clonestr("\"error\":\"genmultisig couldnt find refcontact\"}"));
    flag = 0;
    stats = get_nodestats(refbits);
    myacctcoinaddr[0] = mypubkey[0] = 0;
    for (iter=0; iter<2; iter++)
    for (i=0; i<n; i++)
    {
        //fprintf(stderr,"iter.%d i.%d\n",iter,i);
        if ( (contact= contacts[i]) != 0 && contact->nxt64bits != 0 )
        {
            if ( iter == 0 && ismynxtbits(contact->nxt64bits) != 0 )//|| (stats->ipbits != 0 && calc_ipbits(cp->myipaddr) == stats->ipbits)) )
            {
                myacctcoinaddr[0] = mypubkey[0] = 0;
                //if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
                //   printf("Is me.%llu\n",(long long)contact->nxt64bits);
                if ( cp != 0 && get_acct_coinaddr(myacctcoinaddr,cp,refNXTaddr) != 0 && get_bitcoind_pubkey(mypubkey,cp,myacctcoinaddr) != 0 && myacctcoinaddr[0] != 0 && mypubkey[0] != 0 )
                {
                    flag++;
                    add_NXT_coininfo(contact->nxt64bits,refbits,cp->name,myacctcoinaddr,mypubkey);
                    valid++;
                }
                else printf("error getting msigaddr for cp.%p ref.(%s) addr.(%s) pubkey.(%s)\n",cp,refNXTaddr,myacctcoinaddr,mypubkey);
            }
            else if ( iter == 1 && ismynxtbits(contact->nxt64bits) == 0 )//&& http_search_msig(Server_NXTaddrs[i],Server_ipaddrs[i],refacct) == 0 )
            {
                acctcoinaddr[0] = pubkey[0] = 0;
                if ( get_NXT_coininfo(contact->nxt64bits,acctcoinaddr,pubkey,refbits,cp->name) == 0 || acctcoinaddr[0] == 0 || pubkey[0] == 0 )
                {
                    if ( myacctcoinaddr[0] != 0 && mypubkey[0] != 0 )
                        sprintf(buf,"{\"requestType\":\"getmsigpubkey\",\"NXT\":\"%s\",\"myaddr\":\"%s\",\"mypubkey\":\"%s\",\"coin\":\"%s\",\"refNXTaddr\":\"%s\",\"userpubkey\":\"%s\"}",NXTaddr,myacctcoinaddr,mypubkey,coinstr,refNXTaddr,userpubkey);
                    else sprintf(buf,"{\"requestType\":\"getmsigpubkey\",\"NXT\":\"%s\",\"coin\":\"%s\",\"refNXTaddr\":\"%s\",\"userpubkey\":\"%s\"}",NXTaddr,coinstr,refNXTaddr,userpubkey);
                    if ( Debuglevel > 2 )
                        printf("SENDREQ.(%s)\n",buf);
                    hopNXTaddr[0] = 0;
                    expand_nxt64bits(destNXTaddr,contact->nxt64bits);
                    retstr = send_tokenized_cmd(!prevent_queueing("getmsigpubkey"),hopNXTaddr,0,NXTaddr,NXTACCTSECRET,buf,destNXTaddr);
                }
                else
                {
                    //printf("already have %llu:%llu (%s %s)\n",(long long)contact->nxt64bits,(long long)refbits,acctcoinaddr,pubkey);
                    valid++;
                }
                //if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
                //    printf("check %llu with get_NXT_coininfo i.%d valid.%d\n",(long long)contact->nxt64bits,i,valid);
            } //else printf("iter.%d reject %llu\n",iter,(long long)contact->nxt64bits);
        }
    }
    //fprintf(stderr,"call gen_multisig_addr\n");
    if ( (msig= find_NXT_msig(0,NXTaddr,cp->name,contacts,N)) == 0 )
    {
        if ( (msig= gen_multisig_addr(NXTaddr,M,N,cp,refNXTaddr,userpubkey,contacts)) != 0 )
        {
            msig->valid = valid;
            safecopy(msig->email,email,sizeof(msig->email));
            msig->buyNXT = buyNXT;
            update_msig_info(msig,1,NXTaddr);
            update_MGW_msig(msig,NXTaddr);
        }
        //fprintf(stderr,"return valid.%d\n",valid);
    } else valid = N;
    if ( valid == N && msig != 0 )
    {
        retstr = create_multisig_json(msig,0);
        if ( retstr != 0 )
        {
            if ( retstr != 0 && previpaddr != 0 && previpaddr[0] != 0 )
            {
                //if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone > 1 )
                //    printf("retstr.(%s) previp.(%s)\n",retstr,previpaddr);
                send_to_ipaddr(0,1,previpaddr,retstr,NXTACCTSECRET);
            }
            if ( msig != 0 )
            {
                if ( 0 && update_MGW_msig(msig,NXTaddr) > 0 && Global_mp->gatewayid == 2 )
                    broadcast_bindAM(refNXTaddr,msig,0);
            }
        }
    }
    if ( msig != 0 )
        free(msig);
    if ( valid != N || retstr == 0 )
    {
        sprintf(buf,"{\"error\":\"missing msig info\",\"refacct\":\"%s\",\"coin\":\"%s\",\"M\":%d,\"N\":%d,\"valid\":%d}",refacct,coinstr,M,N,valid);
        retstr = clonestr(buf);
        //printf("%s\n",buf);
    }
    return(retstr);
}

void update_coinacct_addresses(uint64_t nxt64bits,cJSON *json,char *txid)
{
    struct coin_info *cp,*refcp = get_coin_info("BTCD");
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
    /*if ( (n= get_MGW_contacts(contacts,N)) != N )
    {
        printf("get_MGW_contacts(%d) only returned %d\n",N,n);
        for (i=0; i<n; i++)
            if ( contacts[i] != 0 )
                free(contacts[i]);
        return;
    }*/
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
    }
}

int32_t process_directnet_syncwithdraw(struct batch_info *wp)
{
    int32_t gatewayid;
    struct coin_info *cp;
    if ( (cp= get_coin_info(wp->W.coinstr)) == 0 )
        printf("cant find coin.(%s)\n",wp->W.coinstr);
    else
    {
        gatewayid = (wp->W.srcgateway % NUM_GATEWAYS);
        cp->withdrawinfos[gatewayid] = *wp;
        *wp = cp->withdrawinfos[Global_mp->gatewayid];
        printf("GOT <<<<<<<<<<<< publish_withdraw_info.%d coin.(%s) %.8f crc %08x balance %.8f unspent %.8f pendingwithdraws %.8f\n",gatewayid,wp->W.coinstr,dstr(wp->W.amount),cp->withdrawinfos[gatewayid].rawtx.batchcrc,dstr(cp->withdrawinfos[gatewayid].C.balance),dstr(cp->withdrawinfos[gatewayid].C.unspent),dstr(cp->withdrawinfos[gatewayid].C.pendingdeposits));
    }
    return(sizeof(*wp));
}

void MGW_handler(struct transfer_args *args)
{
    printf("MGW_handler(%s %d bytes) vs %ld\n",args->name,args->totallen,sizeof(struct batch_info));
    if ( args->totallen == sizeof(struct batch_info) )
        process_directnet_syncwithdraw((struct batch_info *)args->data);
    //getchar();
}

void set_batchname(char *batchname,char *coinstr,int32_t gatewayid)
{
    sprintf(batchname,"%s.MGW%d",coinstr,gatewayid);
}

void publish_withdraw_info(struct coin_info *cp,struct batch_info *wp)
{
    struct coin_info *refcp = get_coin_info("BTCD");
    char batchname[512],fname[512],*retstr;
    struct batch_info W;
    int32_t gatewayid;
    FILE *fp;
    safecopy(wp->W.coinstr,cp->name,sizeof(wp->W.coinstr));
    set_batchname(batchname,cp->name,Global_mp->gatewayid);
    set_handler_fname(fname,"mgw",batchname);
    if ( (fp= fopen(os_compatible_path(fname),"wb+")) != 0 )
    {
        fwrite(wp,1,sizeof(*wp),fp);
        fclose(fp);
        printf("created (%s)\n",fname);
    }
    if ( refcp == 0 )
    {
        printf("unknown coin.(%s) refcp.%p\n",cp->name,refcp);
        return;
    }
    wp->W.srcgateway = Global_mp->gatewayid;
    for (gatewayid=0; gatewayid<NUM_GATEWAYS; gatewayid++)
    {
        wp->W.destgateway = gatewayid;
        W = *wp;
        fprintf(stderr,"publish_withdraw_info.%d -> %d coin.(%s) %.8f crc %08x\n",Global_mp->gatewayid,gatewayid,wp->W.coinstr,dstr(wp->W.amount),W.rawtx.batchcrc);
        if ( gatewayid == Global_mp->gatewayid )
        {
            process_directnet_syncwithdraw(&W);
            cp->withdrawinfos[gatewayid] = W;
        }
        else
        {
            printf("send balance %.8f, unspent %.8f pendingdeposits %.8f\n",dstr(W.C.balance),dstr(W.C.unspent),dstr(W.C.pendingdeposits));
            retstr = start_transfer(0,refcp->srvNXTADDR,refcp->srvNXTADDR,refcp->srvNXTACCTSECRET,Server_ipaddrs[gatewayid],batchname,(uint8_t *)&W,(int32_t)sizeof(W),300,"mgw",1);
            if ( retstr != 0 )
                free(retstr);
        }
        fprintf(stderr,"got publish_withdraw_info.%d -> %d coin.(%s) %.8f crc %08x\n",Global_mp->gatewayid,gatewayid,wp->W.coinstr,dstr(wp->W.amount),cp->withdrawinfos[gatewayid].rawtx.batchcrc);
    }
}

uint64_t get_deposittxid(struct NXT_asset *ap,char *txidstr,int32_t vout)
{
    int32_t i;
    if ( ap->num > 0 )
    {
        for (i=0; i<ap->num; i++)
            if ( ap->txids[i]->cointxid != 0 && strcmp(ap->txids[i]->cointxid,txidstr) == 0 && ap->txids[i]->coinv == vout )
                return(ap->txids[i]->redeemtxid);
    }
    return(0);
}

uint64_t get_sentAM_cointxid(char *txidstr,struct coin_info *cp,cJSON *autojson,char *withdrawaddr,uint64_t redeemtxid,uint64_t AMtxidbits)
{
    char AMtxidstr[64],coinstr[512],redeemstr[MAX_JSON_FIELD],comment[MAX_JSON_FIELD*4],*retstr;
    char checkcointxid[512],checkcoinaddr[512];
    cJSON *json,*commentjson,*array;
    int32_t i,n,matched = 0;
    uint64_t value = 0;
    uint32_t blocknum,txind,vout;
    txidstr[0] = 0;
    blocknum = txind = vout = 0;
    expand_nxt64bits(AMtxidstr,AMtxidbits);
    if ( (retstr= issue_getTransaction(0,AMtxidstr)) != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            copy_cJSON(comment,cJSON_GetObjectItem(json,"comment"));
            if ( comment[0] != 0 && (commentjson= cJSON_Parse(comment)) != 0 ) // (comment[0] == '{' || comment[0] == '[') &&
            {
                if ( extract_cJSON_str(redeemstr,sizeof(redeemstr),commentjson,"redeemtxid") > 0 && calc_nxt64bits(redeemstr) != redeemtxid )
                {
                    array = cJSON_GetObjectItem(commentjson,"redeems");
                    if ( array != 0 && is_cJSON_Array(array) != 0 )
                    {
                        n = cJSON_GetArraySize(array);
                        for (i=0; i<n; i++)
                        {
                            copy_cJSON(redeemstr,cJSON_GetArrayItem(array,i));
                            if ( redeemstr[0] != 0 && calc_nxt64bits(redeemstr) == redeemtxid )
                            {
                                matched++;
                                break;
                            }
                        }
                    }
                }
                if ( autojson == 0 )
                {
                    copy_cJSON(txidstr,cJSON_GetObjectItem(commentjson,"cointxid"));
                    if ( extract_cJSON_str(coinstr,sizeof(coinstr),commentjson,"coin") > 0 && strcmp(coinstr,cp->name) == 0 )
                    {
                        //sprintf(comment,"{\"coinaddr\":\"%s\",\"cointxid\":\"%s\",\"coinblocknum\":%u,\"cointxind\":%u,\"coinv\":%u,\"amount\":\"%.8f\"}",coinaddr,txidstr,entry->blocknum,entry->txind,entry->v,dstr(value));
                        extract_cJSON_str(checkcoinaddr,sizeof(checkcoinaddr),commentjson,"coinaddr");
                        extract_cJSON_str(checkcointxid,sizeof(checkcointxid),commentjson,"cointxid");
                        if ( strcmp(checkcoinaddr,withdrawaddr) == 0 )
                            matched++;
                        if ( strcmp(checkcointxid,txidstr) == 0 )
                            matched++;
                        blocknum = (uint32_t)get_API_int(cJSON_GetObjectItem(commentjson,"coinblocknum"),0);
                        txind = (uint32_t)get_API_int(cJSON_GetObjectItem(commentjson,"cointxind"),0);
                        vout = (uint32_t)get_API_int(cJSON_GetObjectItem(commentjson,"coinv"),0);
                    } else matched++;
                }
                free_json(commentjson);
            }
            free_json(json);
        }
        free(retstr);
    }
    if ( autojson != 0 )
    {
        if ( matched == 0 )
            value = 0;
    }
    else if ( matched == 3 && txidstr[0] != 0 )
    {
        int32_t numvouts;
        value = get_txoutstr(&numvouts,checkcointxid,checkcoinaddr,0,cp,blocknum,txind,vout);
        if ( strcmp(checkcoinaddr,withdrawaddr) == 0 )
            matched++;
        if ( strcmp(checkcointxid,txidstr) == 0 )
            matched++;
        if ( matched != 5 )
            value = 0;
    } else value = 0;
    return(value);
}

void process_MGW_message(char *specialNXTaddrs[],struct json_AM *ap,char *sender,char *receiver,char *txid,int32_t syncflag,char *coinstr)
{
    char NXTaddr[64];
    cJSON *argjson;
    struct multisig_addr *msig;
    expand_nxt64bits(NXTaddr,ap->H.nxt64bits);
    if ( (argjson = parse_json_AM(ap)) != 0 )
    {
       // if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
       //     fprintf(stderr,"func.(%c) %s -> %s txid.(%s) JSON.(%s)\n",ap->funcid,sender,receiver,txid,ap->U.jsonstr);
        switch ( ap->funcid )
        {
            case GET_COINDEPOSIT_ADDRESS:
                // start address gen
                //fprintf(stderr,"GENADDRESS: func.(%c) %s -> %s txid.(%s) JSON.(%s)\n",ap->funcid,sender,receiver,txid,ap->U.jsonstr);
                update_coinacct_addresses(ap->H.nxt64bits,argjson,txid);
                break;
            case BIND_DEPOSIT_ADDRESS:
                if ( (in_specialNXTaddrs(specialNXTaddrs,sender) != 0 || strcmp(sender,receiver) == 0) && (msig= decode_msigjson(0,argjson,sender)) != 0 )
                {
                    if ( strcmp(msig->coinstr,coinstr) == 0 )
                    {
                        if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
                            fprintf(stderr,"BINDFUNC: %s func.(%c) %s -> %s txid.(%s) JSON.(%s)\n",msig->coinstr,ap->funcid,sender,receiver,txid,ap->U.jsonstr);
                        if ( update_msig_info(msig,syncflag,sender) > 0 )
                        {
                            update_MGW_msig(msig,sender);
                            //fprintf(stderr,"%s func.(%c) %s -> %s txid.(%s) JSON.(%s)\n",msig->coinstr,ap->funcid,sender,receiver,txid,ap->U.jsonstr);
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
                //if ( is_gateway_addr(sender) != 0 )
                //    update_money_sent(argjson,txid,height);
                  if ( in_specialNXTaddrs(specialNXTaddrs,sender) != 0 )
                    update_redeembits(0,argjson,calc_nxt64bits(txid));
                break;
            default: printf("funcid.(%c) not handled\n",ap->funcid);
        }
        if ( argjson != 0 )
            free_json(argjson);
    } else if ( Debuglevel > 2 ) printf("can't JSON parse (%s)\n",ap->U.jsonstr);
}

uint64_t process_NXTtransaction(char *specialNXTaddrs[],char *sender,char *receiver,cJSON *item,char *refNXTaddr,char *assetidstr,int32_t syncflag,struct coin_info *refcp)
{
    //int32_t conv_coinstr(char *);
    char AMstr[4096],txid[4096],comment[4096],*commentstr = 0;
    cJSON *senderobj,*attachment,*message,*assetjson,*commentobj;
    char cointxid[128];
    unsigned char buf[4096];
    struct NXT_AMhdr *hdr;
    struct NXT_asset *ap = 0;
    struct NXT_assettxid *tp;
    struct coin_info *cp = 0;
    uint64_t retbits = 0;
    int32_t createdflag,numconfs,height,buyNXT,timestamp=0;
    int64_t type,subtype,n,assetoshis = 0;
    assetidstr[0] = 0;
    cointxid[0] = 0;
    buyNXT = 0;
    if ( item != 0 )
    {
        hdr = 0;
        sender[0] = receiver[0] = 0;
        numconfs = (int32_t)get_API_int(cJSON_GetObjectItem(item,"confirmations"),0);
        copy_cJSON(txid,cJSON_GetObjectItem(item,"transaction"));
        type = get_cJSON_int(item,"type");
        subtype = get_cJSON_int(item,"subtype");
        /*if ( strcmp(txid,"11393134458431817279") == 0 )
        {
            fprintf(stderr,"[%s] start type.%d subtype.%d txid.(%s)\n",cJSON_Print(item),(int)type,(int)subtype,txid);
            getchar();
        }*/
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
                //printf("txid.%s AM message.(%s).%ld\n",txid,AMstr,strlen(AMstr));
                n = strlen(AMstr);
                if ( is_hexstr(AMstr) != 0 )
                {
                    if ( (n&1) != 0 || n > 2000 )
                        printf("warning: odd message len?? %ld\n",(long)n);
                    memset(buf,0,sizeof(buf));
                    decode_hex((void *)buf,(int32_t)(n>>1),AMstr);
                    buf[n>>1] = 0;
                    hdr = (struct NXT_AMhdr *)buf;
                    process_MGW_message(specialNXTaddrs,(void *)hdr,sender,receiver,txid,syncflag,refcp->name);
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
                if ( (cp= conv_assetid(assetidstr)) != 0 )
                {
                    ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
                    assetoshis = get_cJSON_int(attachment,"quantityQNT");
                    tp = set_assettxid(specialNXTaddrs,cp,ap,calc_nxt64bits(txid),calc_nxt64bits(sender),calc_nxt64bits(receiver),timestamp,commentstr,assetoshis);
                    tp->numconfs = numconfs;
                    add_pendingxfer(1,tp->redeemtxid);
                    retbits = tp->redeemtxid;
                }
            }
        }
    }
    else printf("unexpected error iterating timestamp.(%d) txid.(%s)\n",timestamp,txid);
    //fprintf(stderr,"finish type.%d subtype.%d txid.(%s)\n",(int)type,(int)subtype,txid);
    return(retbits);
}

int32_t update_NXT_transactions(char *specialNXTaddrs[],int32_t txtype,char *refNXTaddr,struct coin_info *cp)
{
    char sender[1024],receiver[1024],assetid[1024],cmd[1024],*jsonstr;
    int32_t createdflag,coinid,i,n = 0;
    int32_t timestamp,numconfs;
    struct NXT_acct *np;
    cJSON *item,*json,*array;
    if ( refNXTaddr == 0 || specialNXTaddrs == 0 || specialNXTaddrs[0] == 0 )
    {
        printf("illegal refNXT.(%s)\n",refNXTaddr);
        return(0);
    }
    sprintf(cmd,"%s=getAccountTransactions&account=%s",_NXTSERVER,refNXTaddr);
    if ( txtype >= 0 )
        sprintf(cmd+strlen(cmd),"&type=%d",txtype);
    coinid = is_active_coin(cp->name);
    np = get_NXTacct(&createdflag,refNXTaddr);
    //if ( coinid >= 0 && coinid < 64 && np->timestamps[coinid] != 0 )
    //    sprintf(cmd + strlen(cmd),"&timestamp=%d",np->timestamps[coinid]);
    if ( Debuglevel > 2 )
        printf("minconfirms.%d update_NXT_transactions.(%s) for (%s) cmd.(%s) type.%d\n",MIN_NXTCONFIRMS,refNXTaddr,cp->name,cmd,txtype);
    if ( (jsonstr= issue_NXTPOST(0,cmd)) != 0 )
    {
        //if ( strcmp(refNXTaddr,"7117166754336896747") == 0 )
        //    printf("(%s)\n",jsonstr);//, getchar();
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"transactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    if ( Debuglevel > 2 )
                        fprintf(stderr,"%d/%d ",i,n);
                    item = cJSON_GetArrayItem(array,i);
                    copy_cJSON(sender,cJSON_GetObjectItem(item,"sender"));
                    numconfs = (int32_t)get_API_int(cJSON_GetObjectItem(item,"confirmations"),0);
                    if ( in_specialNXTaddrs(specialNXTaddrs,sender) > 0 || numconfs >= MIN_NXTCONFIRMS )
                    {
                        process_NXTtransaction(specialNXTaddrs,sender,receiver,item,refNXTaddr,assetid,0,cp);
                        timestamp = (int32_t)get_cJSON_int(item,"blockTimestamp");
                        //if ( coinid >= 0 && coinid < 64 && timestamp > 0 && (timestamp - 3600) > np->timestamps[coinid] )
                        {
                           // printf("new.%s timestamp.%d %d -> %d\n",cp->name,coinid,np->timestamps[coinid],timestamp-3600);
                           // np->timestamps[coinid] = (timestamp - 3600); // assumes no hour long block
                        } //else if ( timestamp < 0 ) genesis tx dont have any timestamps!
                          //  printf("missing blockTimestamp.(%s)\n",jsonstr), getchar();
                    }
                }
            }
            free_json(json);
        }
        free(jsonstr);
    } else printf("error with update_NXT_transactions.(%s)\n",cmd);
    return(n);
}

int32_t ready_to_xferassets(uint64_t *txidp)
{
    // if fresh reboot, need to wait the xfer max duration + 1 block before running this
    static int32_t firsttime,firstNXTblock;
    *txidp = 0;
    printf("(%d %d) lag.%ld %d\n",firsttime,firstNXTblock,time(NULL)-firsttime,get_NXTblock(0)-firstNXTblock);
    if ( firsttime == 0 )
        firsttime = (uint32_t)time(NULL);
    if ( firstNXTblock <= 0 )
        firstNXTblock = get_NXTblock(0);
    if ( time(NULL) < (firsttime + DEPOSIT_XFER_DURATION*60) )
        return(0);
    if ( firstNXTblock <= 0 || get_NXTblock(0) < (firstNXTblock + DEPOSIT_XFER_DURATION) )
        return(0);
    if ( (*txidp= add_pendingxfer(0,0)) != 0 )
    {
        printf("waiting for pendingxfer\n");
        return(0);
    }
    return(1);
}

uint64_t conv_address_entry(char *coinaddr,char *txidstr,char *script,struct coin_info *cp,struct address_entry *entry)
{
    char txidstr_v0[1024],coinaddr_v0[1024],script_v0[4096],_script[4096];
    int32_t numvouts;
    uint64_t value = 0;
    coinaddr[0] = txidstr[0] = 0;
    if ( script != 0 )
        script[0] = 0;
    if ( entry->vinflag == 0 )
    {
        if ( script == 0 )
            script = _script;
        value = get_txoutstr(&numvouts,txidstr,coinaddr,script,cp,entry->blocknum,entry->txind,entry->v);
        if ( strcmp("31dcbc5b7cfd7fc8f2c1cedf65f38ec166b657cc9eb15e7d1292986eada35ea9",txidstr) == 0 ) // due to uncommented tx
            return(0);
        if ( entry->v == numvouts-1 ) // the last output when there is a marker is internal change, need to get numvouts first
        {
            get_txoutstr(0,txidstr_v0,coinaddr_v0,script_v0,cp,entry->blocknum,entry->txind,0);
            if ( strcmp(coinaddr_v0,cp->marker) == 0 )
                return(0);
        }
    }
    return(value);
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
            stripwhite_ns(str,strlen(str));
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
            str = cJSON_Print(pair);
            stripwhite_ns(str,strlen(str));
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

uint64_t process_msigdeposits(cJSON **transferjsonp,int32_t forceflag,struct coin_info *cp,struct address_entry *entry,uint64_t nxt64bits,struct NXT_asset *ap,char *msigaddr,char *depositors_pubkey,uint32_t *buyNXTp)
{
    char txidstr[1024],coinaddr[1024],script[4096];
    struct NXT_assettxid *tp;
    uint64_t value,total = 0;
    int32_t j;
    for (j=0; j<ap->num; j++)
    {
        tp = ap->txids[j];
        //printf("%d of %d: process.(%s) isinternal.%d %llu (%llu -> %llu)\n",j,ap->num,msigaddr,entry->isinternal,(long long)nxt64bits,(long long)tp->senderbits,(long long)tp->receiverbits);
        if ( tp->receiverbits == nxt64bits && tp->coinblocknum == entry->blocknum && tp->cointxind == entry->txind && tp->coinv == entry->v )
            break;
    }
    if ( j == ap->num )
    {
        if ( (value= conv_address_entry(coinaddr,txidstr,script,cp,entry)) == 0 )
        {
            if ( Debuglevel > 2 )
                printf("skip %s\n",txidstr);
            return(0);
        }
        if ( strcmp(msigaddr,coinaddr) == 0 && txidstr[0] != 0 && value >= (cp->NXTfee_equiv * MIN_DEPOSIT_FACTOR) )
        {
            for (j=0; j<ap->num; j++)
            {
                tp = ap->txids[j];
                if ( tp->receiverbits == nxt64bits && tp->cointxid != 0 && strcmp(tp->cointxid,txidstr) == 0 )
                {
                    if ( Debuglevel > 1 )
                        printf("%llu set cointxid.(%s) <-> (%u %d %d)\n",(long long)nxt64bits,txidstr,entry->blocknum,entry->txind,entry->v);
                    tp->cointxind = entry->txind;
                    tp->coinv = entry->v;
                    tp->coinblocknum = entry->blocknum;
                    break;
                }
            }
            if ( j == ap->num )
            {
                total = MGWtransfer_asset(transferjsonp,forceflag,nxt64bits,depositors_pubkey,ap,value,coinaddr,txidstr,entry,buyNXTp,cp->srvNXTADDR,cp->srvNXTACCTSECRET,DEPOSIT_XFER_DURATION);
            }
        }
    }
    return(total);
}

struct coin_txidmap *get_txid(struct coin_info *cp,char *txidstr,uint32_t blocknum,int32_t txind,int32_t v)
{
    char buf[1024],coinaddr[1024],script[4096],checktxidstr[1024];
    int32_t createdflag;
    struct coin_txidmap *tp;
    set_txidmap_str(buf,txidstr,cp->name,v);
    tp = MTadd_hashtable(&createdflag,Global_mp->coin_txidmap,buf);
    if ( createdflag != 0 )
    {
        if ( blocknum == 0xffffffff || txind < 0 )
        {
            blocknum = get_txidind(&txind,cp,txidstr,v);
            if ( txind >= 0 && blocknum < 0xffffffff )
            {
                get_txoutstr(0,checktxidstr,coinaddr,script,cp,blocknum,txind,v);
                if ( strcmp(checktxidstr,txidstr) != 0 )
                    printf("checktxid.(%s) != (%s)???\n",checktxidstr,txidstr);
                else printf("txid.(%s) (%d %d %d) verified\n",txidstr,blocknum,txind,v);
            }
        }
        tp->blocknum = blocknum;
        tp->txind = txind;
        tp->v = v;
    }
    return(tp);
}

struct coin_txidind *_get_cointp(int32_t *createdflagp,char *coinstr,uint32_t blocknum,uint16_t txind,uint16_t v)
{
    char indstr[32];
    uint64_t ind;
    ind = ((uint64_t)blocknum << 32) | ((uint64_t)txind << 16) | v;
    strcpy(indstr,coinstr);
    expand_nxt64bits(indstr+strlen(indstr),ind);
    return(MTadd_hashtable(createdflagp,Global_mp->coin_txidinds,indstr));
}

struct coin_txidind *conv_txidstr(struct coin_info *cp,char *txidstr,int32_t v)
{
    int32_t txind,createdflag;
    uint32_t blocknum;
    blocknum = get_txidind(&txind,cp,txidstr,v);
    return(_get_cointp(&createdflag,cp->name,blocknum,txind,v));
}

struct coin_txidind *get_cointp(struct coin_info *cp,struct address_entry *entry)
{
    char script[MAX_JSON_FIELD],origtxidstr[256];
    struct coin_txidind *cointp;
    struct coin_txidmap *tp;
    int32_t createdflag,spentflag;
    uint32_t blocknum;
    uint16_t txind,v;
    if ( entry->vinflag != 0 )
    {
        v = get_txinstr(origtxidstr,cp,entry->blocknum,entry->txind,entry->v);
        tp = get_txid(cp,origtxidstr,0xffffffff,-1,v);
        blocknum = tp->blocknum;
        txind = tp->txind;
        if ( v != tp->v )
            fprintf(stderr,"error (%d != %d)\n",v,tp->v);
        if ( Debuglevel > 2 )
            printf("get_cointpspent.(%016llx) (%d %d %d) -> (%s).%d (%d %d %d)\n",*(long long *)entry,entry->blocknum,entry->txind,entry->v,origtxidstr,v,blocknum,txind,v);
        spentflag = 1;
    }
    else
    {
        blocknum = entry->blocknum;
        txind = entry->txind;
        v = entry->v;
        spentflag = 0;
    }
    cointp = _get_cointp(&createdflag,cp->name,blocknum,txind,v);
    if ( createdflag != 0 || cointp->value == 0 )
    {
        cointp->entry = *entry;
        cointp->value = get_txoutstr(&cointp->numvouts,cointp->txid,cointp->coinaddr,script,cp,blocknum,txind,v);
        if ( cointp->script != 0 )
            free(cointp->script);
        cointp->script = clonestr(script);
        if ( entry->vinflag == 0 )
            get_txid(cp,cointp->txid,blocknum,txind,v);
    }
    if ( spentflag != 0 )
        cointp->entry.spent = 1;
    if ( cointp->entry.spent != 0 && cointp->script != 0 )
        free(cointp->script), cointp->script = 0;
    return(cointp);
}

uint64_t process_msigaddr(int32_t *numunspentp,uint64_t *unspentp,cJSON **transferjsonp,int32_t forceflag,struct NXT_asset *ap,char *NXTaddr,struct coin_info *cp,char *msigaddr,char *depositors_pubkey,uint32_t *buyNXTp)
{
    void set_NXTpubkey(char *,char *);
    struct address_entry *entries,*entry;
    int32_t i,n;
    //uint32_t createtime = 0;
    struct coin_txidind *cointp;
    uint64_t nxt64bits,unspent,pendingdeposits = 0;
    if ( ap->mult == 0 )
    {
        printf("ap->mult is ZERO for %s?\n",ap->name);
        return(0);
    }
    nxt64bits = calc_nxt64bits(NXTaddr);
    if ( depositors_pubkey != 0 && depositors_pubkey[0] == 0 )
        set_NXTpubkey(depositors_pubkey,NXTaddr);
    if ( (entries= get_address_entries(&n,cp->name,msigaddr)) != 0 )
    {
        if ( Debuglevel > 2 )
            printf(">>>>>>>>>>>>>>>> %d address entries for (%s)\n",n,msigaddr);
        for (i=0; i<n; i++)
        {
            entry = &entries[i];
            if ( entry->vinflag == 0 )
                pendingdeposits += process_msigdeposits(transferjsonp,forceflag,cp,entry,nxt64bits,ap,msigaddr,depositors_pubkey,buyNXTp);
            if ( Debuglevel > 2 )
                printf("process_msigaddr.(%s) %d of %d: vin.%d internal.%d spent.%d (%d %d %d)\n",msigaddr,i,n,entry->vinflag,entry->isinternal,entry->spent,entry->blocknum,entry->txind,entry->v);
            get_cointp(cp,entry);
        }
        for (i=0; i<n; i++)
        {
            entry = &entries[i];
            cointp = get_cointp(cp,entry);
            if ( cointp != 0 && cointp->entry.spent == 0 )
            {
                unspent = cointp->value;
                cointp->unspent = unspent;
                (*numunspentp)++;
                (*unspentp) += unspent;
                if ( Debuglevel > 2 )
                    printf("%s | %16.8f unspenttotal %.8f\n",cointp->txid,dstr(cointp->unspent),dstr((*unspentp)));
                update_unspent_funds(cp,cointp,0);
            }
        }
        free(entries);
    } else printf("no entries for (%s)\n",msigaddr);
    return(pendingdeposits);
}

int32_t valid_msig(struct multisig_addr *msig,char *coinstr,char *specialNXT,char *gateways[],char *ipaddrs[],int32_t M,int32_t N)
{
    int32_t i,match = 0;
    char NXTaddr[64],gatewayNXTaddr[64],ipaddr[64];
    //printf("%s %s M.%d N.%d %llu vs %s (%s %s %s)\n",msig->coinstr,coinstr,msig->m,msig->n,(long long)msig->sender,specialNXT,gateways[0],gateways[1],gateways[2]);
    if ( strcmp(msig->coinstr,coinstr) == 0 && msig->m == M && msig->n == N )
    {
        expand_nxt64bits(NXTaddr,msig->sender);
        if ( strcmp(NXTaddr,specialNXT) == 0 )
            match++;
        else
        {
            for (i=0; i<N; i++)
                if ( strcmp(NXTaddr,gateways[i]) == 0 )
                    match++;
        }
        //printf("match.%d check for sender.(%s) vs special %s %s %s %s\n",match,NXTaddr,specialNXT,gateways[0],gateways[1],gateways[2]);
return(match);
        if ( match > 0 )
        {
            for (i=0; i<N; i++)
            {
                expand_nxt64bits(gatewayNXTaddr,msig->pubkeys[i].nxt64bits);
                if ( strcmp(gateways[i],gatewayNXTaddr) != 0 )
                {
                    printf("(%s != %s) ",gateways[i],gatewayNXTaddr);
                    break;
                }
            }
            printf("i.%d\n",i);
            if ( i == N )
                return(1);
            for (i=0; i<N; i++)
            {
                expand_ipbits(ipaddr,msig->pubkeys[i].ipbits);
                printf("(%s) ",ipaddr);
                if ( strcmp(ipaddrs[i],ipaddr) != 0 )
                    break;
            }
            printf("j.%d\n",i);
            if ( i == N )
                return(1);
        }
    }
    return(0);
}

char *get_default_MGWstr(char *str,int32_t ind)
{
    if ( str == 0 || str[0] == 0 )
        str = MGW_whitelist[ind];
    return(str);
}

void init_specialNXTaddrs(char *specialNXTaddrs[],char *ipaddrs[],char *specialNXT,char *NXT0,char *NXT1,char *NXT2,char *ip0,char *ip1,char *ip2,char *exclude0,char *exclude1,char *exclude2)
{
    int32_t i,n = 0;
    NXT0 = get_default_MGWstr(NXT0,0);
    NXT1 = get_default_MGWstr(NXT1,1);
    NXT2 = get_default_MGWstr(NXT2,2);
    exclude0 = get_default_MGWstr(exclude0,3);
    exclude1 = get_default_MGWstr(exclude1,4);
    exclude2 = get_default_MGWstr(exclude2,5);
    
    specialNXTaddrs[n++] = clonestr(NXT0), specialNXTaddrs[n++] = clonestr(NXT1), specialNXTaddrs[n++] = clonestr(NXT2);
    ipaddrs[0] = ip0, ipaddrs[1] = ip1, ipaddrs[2] = ip2;
    for (i=0; i<n; i++)
    {
        if ( specialNXTaddrs[i] == 0 )
            specialNXTaddrs[i] = "";
        if ( ipaddrs[i] == 0 )
            strcpy(ipaddrs[i],Server_ipaddrs[i]);
    }
    if ( exclude0 != 0 && exclude0[0] != 0 )
        specialNXTaddrs[n++] = clonestr(exclude0);
    if ( exclude1 != 0 && exclude1[0] != 0 )
        specialNXTaddrs[n++] = clonestr(exclude1);
    if ( exclude2 != 0 && exclude2[0] != 0 )
        specialNXTaddrs[n++] = clonestr(exclude2);
    specialNXTaddrs[n++] = clonestr(GENESISACCT);
    specialNXTaddrs[n++] = clonestr(specialNXT);
    specialNXTaddrs[n] = 0;
    for (i=0; i<n; i++)
        fprintf(stderr,"%p ",specialNXTaddrs[i]);
    fprintf(stderr,"numspecialNXT.%d\n",n);
}

uint64_t update_NXTblockchain_info(struct coin_info *cp,char *specialNXTaddrs[],char *refNXTaddr)
{
    struct coin_info *btcdcp;
    uint64_t pendingtxid;
    int32_t i;
    ready_to_xferassets(&pendingtxid);
    if ( (btcdcp= get_coin_info("BTCD")) != 0 )
    {
        update_NXT_transactions(specialNXTaddrs,-1,btcdcp->srvNXTADDR,cp);
        update_NXT_transactions(specialNXTaddrs,-1,btcdcp->privateNXTADDR,cp);
    }
    update_NXT_transactions(specialNXTaddrs,-1,refNXTaddr,cp);
    for (i=0; i<specialNXTaddrs[i][0]!=0; i++)
        update_NXT_transactions(specialNXTaddrs,-1,specialNXTaddrs[i],cp); // first numgateways of specialNXTaddrs[] are gateways
    update_msig_info(0,1,0); // sync MULTISIG_DATA
    return(pendingtxid);
}

char *wait_for_pendingtxid(struct coin_info *cp,char *specialNXTaddrs[],char *refNXTaddr,uint64_t pendingtxid)
{
    char txidstr[64],sender[64],receiver[64],assetstr[64],retbuf[1024],*retstr;
    cJSON *json;
    uint64_t val;
    expand_nxt64bits(txidstr,pendingtxid);
    sprintf(retbuf,"{\"result\":\"pendingtxid\",\"waitingfor\":\"%llu\"}",(long long)pendingtxid);
    if ( (retstr= issue_getTransaction(0,txidstr)) != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            if ( (val= process_NXTtransaction(specialNXTaddrs,sender,receiver,json,refNXTaddr,assetstr,1,cp)) != 0 )
                sprintf(retbuf,"{\"result\":\"pendingtxid\",\"processed\":\"%llu\"}",(long long)val);
            free_json(json);
        }
        free(retstr);
    }
    return(clonestr(retbuf));
}

uint64_t process_deposits(cJSON **jsonp,uint64_t *unspentp,struct multisig_addr **msigs,int32_t nummsigs,struct coin_info *cp,char *ipaddrs[],char *specialNXTaddrs[],char *refNXTaddr,struct NXT_asset *ap,int32_t transferassets,uint64_t circulation,char *depositor,char *depositors_pubkey)
{
    uint64_t pendingtxid,unspent = 0,total = 0;
    int32_t i,m,max,readyflag,tmp,tmp2,nonz,numunspent;
    struct multisig_addr *msig;
    struct address_entry *entries;
    struct unspent_info *up;
    char numstr[128];
    cJSON *array;
    *unspentp = 0;
    array = cJSON_CreateArray();
    if ( msigs != 0 )
    {
        readyflag = ready_to_xferassets(&pendingtxid);
        printf("readyflag.%d depositor.(%s) (%s)\n",readyflag,depositor,depositors_pubkey);
        for (i=max=0; i<nummsigs; i++)
        {
            if ( (msig= (struct multisig_addr *)msigs[i]) != 0 && (entries= get_address_entries(&m,cp->name,msig->multisigaddr)) != 0 )
            {
                max += m;
                free(entries);
            }
        }
        if ( Debuglevel > 2 )
            printf("got n.%d msigs readyflag.%d | max.%d pendingtxid.%llu depositor.(%s)\n",nummsigs,readyflag,max,(long long)pendingtxid,depositor);
        unspent = nonz = numunspent = 0;
        up = &cp->unspent;
        update_unspent_funds(cp,0,max);
        for (i=0; i<nummsigs; i++)
        {
            if ( (msig= (struct multisig_addr *)msigs[i]) != 0 )
            {
                if ( max > 0 && valid_msig(msig,cp->name,refNXTaddr,specialNXTaddrs,ipaddrs,2,3) != 0 )//&& (depositor == 0 || strcmp(depositor,refNXTaddr) == 0) )
                {
                    if ( Debuglevel > 2 )
                        printf("MULTISIG: %s: %d of %d %s %s NXTpubkey.(%s)\n",cp->name,i,nummsigs,msig->coinstr,msig->multisigaddr,msig->NXTpubkey);
                    update_NXT_transactions(specialNXTaddrs,2,msig->NXTaddr,cp);
                    if ( transferassets == 0 || (readyflag > 0 && pendingtxid == 0) )
                    {
                        char tmp3[64];
                        tmp = numunspent;
                        tmp2 = msig->buyNXT;
                        total += process_msigaddr(&numunspent,&unspent,&array,transferassets,ap,msig->NXTaddr,cp,msig->multisigaddr,msig->NXTpubkey,&msig->buyNXT);
                        if ( numunspent > tmp )
                            nonz++;
                        if ( msig->buyNXT == 0 && tmp2 != 0 )
                        {
                            expand_nxt64bits(tmp3,msig->sender);
                            update_msig_info(msig,1,tmp3);
                            //update_MGW_msig(msig,NXTaddr);
                        }
                    }
                }
            }
        }
        if ( up->num > 1 )
        {
            //fprintf(stderr,"call sort_vps with %p num.%d\n",up,up->num);
            sort_vps(up->vps,up->num);
            for (i=0; i<10&&i<up->num; i++)
                fprintf(stderr,"%d (%s) (%s).%d %.8f\n",i,up->vps[i]->txid,up->vps[i]->coinaddr,up->vps[i]->entry.v,dstr(up->vps[i]->value));
        }
        fprintf(stderr,"max %.8f min %.8f median %.8f |unspent %.8f numunspent.%d in nonz.%d accts\n",dstr(up->maxavail),dstr(up->minavail),dstr((up->maxavail+up->minavail)/2),dstr(up->unspent),numunspent,nonz);
    }
    sprintf(numstr,"%.8f",dstr(circulation)), cJSON_AddItemToObject(*jsonp,"circulation",cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",dstr(unspent)), cJSON_AddItemToObject(*jsonp,"unspent",cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",dstr(total)), cJSON_AddItemToObject(*jsonp,"pendingdeposits",cJSON_CreateString(numstr));
    if ( cJSON_GetArraySize(array) > 0 )
        cJSON_AddItemToObject(*jsonp,"alldeposits",array);
    else free_json(array);
    *unspentp = unspent;
    return(total);
}

uint64_t get_accountassets(int32_t height,struct NXT_asset *ap,char *NXTacct)
{
    cJSON *json;
    uint64_t quantity = 0;
    char cmd[4096],*retstr = 0;
    sprintf(cmd,"%s=getAccountAssets&asset=%llu&account=%s",_NXTSERVER,(long long)ap->assetbits,NXTacct);
    if ( height > 0 )
        sprintf(cmd+strlen(cmd),"&height=%d",height);
    if ( (retstr= issue_NXTPOST(0,cmd)) != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            if ( (quantity= get_API_nxt64bits(cJSON_GetObjectItem(json,"quantityQNT"))) != 0 && ap->mult != 0 )
                quantity *= ap->mult;
            free_json(json);
        }
        free(retstr);
    }
    return(quantity);
}

uint64_t calc_circulation(int32_t height,struct NXT_asset *ap,char *specialNXTaddrs[])
{
    uint64_t quantity,circulation = 0;
    char cmd[4096],acct[MAX_JSON_FIELD],*retstr = 0;
    cJSON *json,*array,*item;
    int32_t i,n;
    sprintf(cmd,"%s=getAssetAccounts&asset=%llu",_NXTSERVER,(long long)ap->assetbits);
    if ( height > 0 )
        sprintf(cmd+strlen(cmd),"&height=%d",height);
    if ( (retstr= issue_NXTPOST(0,cmd)) != 0 )
    {
        //fprintf(stderr,"circ.(%s) <- (%s)\n",retstr,cmd);
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"accountAssets")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                //fprintf(stderr,"n.%d\n",n);
                for (i=0; i<n; i++)
                {
                    //fprintf(stderr,"i.%d of n.%d\n",i,n);
                    item = cJSON_GetArrayItem(array,i);
                    copy_cJSON(acct,cJSON_GetObjectItem(item,"account"));
                    //printf("%s ",acct);
                    if ( acct[0] != 0 && in_specialNXTaddrs(specialNXTaddrs,acct) == 0 )
                    {
                        if ( (quantity= get_API_nxt64bits(cJSON_GetObjectItem(item,"quantityQNT"))) != 0 )
                        {
                            circulation += quantity;
                            if ( quantity*ap->mult > 1000*SATOSHIDEN )
                                printf("BIGACCT.%d %s %.8f\n",i,acct,dstr(quantity*ap->mult));
                        //printf("%llu, ",(long long)quantity);
                        }
                    }
                }
            }
            free_json(json);
        }
        free(retstr);
    }
    return(circulation * ap->mult);
}

cJSON *gen_autoconvert_json(struct NXT_assettxid *tp)
{
    cJSON *json;
    int32_t delta;
    char typestr[64],numstr[64];
    if ( tp->convname[0] == 0 )
        return(0);
    json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"name",cJSON_CreateString(tp->convname));
    if ( tp->convwithdrawaddr != 0 )
        cJSON_AddItemToObject(json,"coinaddr",cJSON_CreateString(tp->convwithdrawaddr));
    if ( tp->teleport[0] != 0 )
    {
        if ( strcmp(tp->teleport,"send") == 0 )
            strcpy(typestr,"telepod");
        else strcpy(typestr,"emailpod");
    } else strcpy(typestr,"convert");
    cJSON_AddItemToObject(json,"type",cJSON_CreateString(typestr));
    if ( tp->minconvrate != 0 )
        sprintf(numstr,"%.8f",tp->minconvrate), cJSON_AddItemToObject(json,"convrate",cJSON_CreateString(numstr));
    if ( tp->convexpiration != 0 )
    {
        delta = (int32_t)(tp->convexpiration - time(NULL));
        cJSON_AddItemToObject(json,"expires",cJSON_CreateNumber(delta));
    }
    return(json);
}

char *calc_withdrawaddr(char *withdrawaddr,struct coin_info *cp,struct NXT_assettxid *tp,cJSON *argjson)
{
    cJSON *json;
    int32_t convert = 0;
    struct coin_info *newcp;
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
        if ( (newcp= get_coin_info(autoconvert)) == 0 )
        {
            if ( (retstr= issue_getAsset(0,autoconvert)) != 0 )
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
            strcpy(tp->convname,newcp->name);
            convert = 1;
        }
        if ( convert != 0 )
        {
            tp->minconvrate = get_API_float(cJSON_GetObjectItem(argjson,"rate"));
            tp->convexpiration = (int32_t)get_API_int(cJSON_GetObjectItem(argjson,"expiration"),0);
            if ( withdrawaddr[0] != 0 ) // no address means to create user credit
                tp->convwithdrawaddr = clonestr(withdrawaddr);
        }
        else withdrawaddr[0] = autoconvert[0] = 0;
    }
    //printf("withdrawaddr.(%s) autoconvert.(%s)\n",withdrawaddr,autoconvert);
    if ( withdrawaddr[0] == 0 || autoconvert[0] != 0 )
        return(0);
    //printf("return.(%s)\n",withdrawaddr);
    return(withdrawaddr);
}

char *parse_withdraw_instructions(char *destaddr,char *NXTaddr,struct coin_info *cp,struct NXT_assettxid *tp,struct NXT_asset *ap)
{
    char pubkey[1024],withdrawaddr[1024],*retstr = destaddr;
    int64_t amount,minwithdraw;
    cJSON *argjson = 0;
    destaddr[0] = withdrawaddr[0] = 0;
    if ( tp->redeemtxid == 0 )
    {
        printf("no redeem txid %s %s\n",cp->name,cJSON_Print(argjson));
        retstr = 0;
    }
    else
    {
        amount = tp->quantity * ap->mult;
        if ( tp->comment != 0 && (argjson= cJSON_Parse(tp->comment)) != 0 ) //(tp->comment[0] == '{' || tp->comment[0] == '[') &&
        {
            if ( calc_withdrawaddr(withdrawaddr,cp,tp,argjson) == 0 )
            {
                printf("no withdraw.(%s) or autoconvert.(%s)\n",withdrawaddr,tp->comment);
                retstr = 0;
            }
        }
        if ( retstr != 0 )
        {
            minwithdraw = cp->txfee * MIN_DEPOSIT_FACTOR;
            if ( amount <= minwithdraw )
            {
                printf("minimum withdrawal must be more than %.8f %s\n",dstr(minwithdraw),cp->name);
                retstr = 0;
            }
            else if ( withdrawaddr[0] == 0 )
            {
                printf("no withdraw address for %.8f | ",dstr(amount));
                retstr = 0;
            }
            else if ( cp != 0 && validate_coinaddr(pubkey,cp,withdrawaddr) < 0 )
            {
                printf("invalid address.(%s) for NXT.%s %.8f validate.%d\n",withdrawaddr,NXTaddr,dstr(amount),validate_coinaddr(pubkey,cp,withdrawaddr));
                retstr = 0;
            }
        }
    }
    printf("withdraw addr.(%s) for (%s)\n",withdrawaddr,NXTaddr);
    if ( retstr != 0 )
        strcpy(retstr,withdrawaddr);
    if ( argjson != 0 )
        free_json(argjson);
    return(retstr);
}

int32_t add_redeem(char *destaddrs[],uint64_t *destamounts,uint64_t *redeems,uint64_t *pending_withdrawp,struct coin_info *cp,struct NXT_asset *ap,char *destaddr,struct NXT_assettxid *tp,int32_t numredeems)
{
    int32_t j;
    uint64_t amount;
    if ( destaddr == 0 || destaddr[0] == 0 || numredeems >= MAX_MULTISIG_OUTPUTS-1 )
    {
        printf("add_redeem with null destaddr.%p numredeems.%d\n",destaddr,numredeems);
        return(numredeems);
    }
    amount = tp->quantity * ap->mult - (cp->txfee + cp->NXTfee_equiv);
    (*pending_withdrawp) += amount;
    if ( numredeems > 0 )
    {
        for (j=0; j<numredeems; j++)
            if ( redeems[j] == tp->redeemtxid )
                break;
    } else j = 0;
    if ( j == numredeems )
    {
        destaddrs[numredeems] = clonestr(destaddr);
        destamounts[numredeems] = amount;
        redeems[numredeems] = tp->redeemtxid;
        numredeems++;
        printf("withdraw_addr.%d R.(%llu %.8f %s)\n",j,(long long)tp->redeemtxid,dstr(destamounts[j]),destaddrs[j]);
    }
    else printf("ERROR: duplicate redeembits.%llu numredeems.%d j.%d\n",(long long)tp->redeemtxid,numredeems,j);
    return(numredeems);
}

int32_t add_destaddress(struct rawtransaction *rp,char *destaddr,uint64_t amount)
{
    int32_t i;
    if ( rp->numoutputs > 0 )
    {
        for (i=0; i<rp->numoutputs; i++)
        {
            printf("compare.%d of %d: %s vs %s\n",i,rp->numoutputs,rp->destaddrs[i],destaddr);
            if ( strcmp(rp->destaddrs[i],destaddr) == 0 )
                break;
        }
    } else i = 0;
    printf("add[%d] of %d: %.8f -> %s\n",i,rp->numoutputs,dstr(amount),destaddr);
    if ( i == rp->numoutputs )
    {
        if ( rp->numoutputs >= (int)(sizeof(rp->destaddrs)/sizeof(*rp->destaddrs)) )
        {
            printf("overflow %d vs %ld\n",rp->numoutputs,(sizeof(rp->destaddrs)/sizeof(*rp->destaddrs)));
            return(-1);
        }
        printf("create new output.%d (%s)\n",rp->numoutputs,destaddr);
        rp->destaddrs[rp->numoutputs++] = destaddr;
    }
    rp->destamounts[i] += amount;
    return(i);
}

int32_t process_redeem(int32_t *alreadysentp,cJSON **arrayp,char *destaddrs[MAX_MULTISIG_OUTPUTS],uint64_t destamounts[MAX_MULTISIG_OUTPUTS],uint64_t redeems[MAX_MULTISIG_OUTPUTS],uint64_t *pending_withdrawp,struct coin_info *cp,uint64_t nxt64bits,struct NXT_asset *ap,char *destaddr,struct NXT_assettxid *tp,int32_t numredeems,char *sender)
{
    struct unspent_info *up;
    struct NXT_acct *np;
    char numstr[128],rsacct[64];
    int32_t createdflag;
    cJSON *item;
    //double pending;
    if ( is_limbo_redeem(cp,tp->redeemtxid) == 0 )
    {
        np = get_NXTacct(&createdflag,sender);
        *alreadysentp = 0;
        up = &cp->unspent;
        tp->numconfs = get_NXTconfirms(tp->redeemtxid);
        printf("numredeems.%d (%p %p) PENDING REDEEM numconfs.%d %s %s %llu %llu %.8f %.8f | %llu\n",numredeems,up->maxvp,up->minvp,tp->numconfs,cp->name,destaddr,(long long)nxt64bits,(long long)tp->redeemtxid,dstr(tp->quantity),dstr(tp->U.assetoshis),(long long)tp->AMtxidbits);
        item = cJSON_CreateObject();
        conv_rsacctstr(rsacct,np->H.nxt64bits);
        cJSON_AddItemToObject(item,"NXT",cJSON_CreateString(rsacct));
        sprintf(numstr,"%llu",(long long)tp->redeemtxid), cJSON_AddItemToObject(item,"redeemtxid",cJSON_CreateString(numstr));
        cJSON_AddItemToObject(item,"destaddr",cJSON_CreateString(destaddr));
        sprintf(numstr,"%.8f",dstr(tp->quantity * ap->mult)), cJSON_AddItemToObject(item,"amount",cJSON_CreateString(numstr));
        cJSON_AddItemToObject(item,"confirms",cJSON_CreateNumber(tp->numconfs));
        cJSON_AddItemToArray(*arrayp,item);
        /*if ( (pending= enough_confirms(np->redeemed,tp->estNXT,tp->numconfs,1)) > 0 )
        {
            numredeems = add_redeem(destaddrs,destamounts,redeems,pending_withdrawp,cp,ap,destaddr,tp,numredeems);
            np->redeemed += tp->estNXT;
            printf("NXT.(%s) redeemed %.8f %p numredeems.%d (%s) %.8f %llu\n",sender,np->redeemed,&destaddrs[numredeems-1],numredeems,destaddrs[numredeems-1],dstr(destamounts[numredeems-1]),(long long)redeems[numredeems-1]);
        }
        else
        {
            sprintf(numstr,"%.2f",dstr(np->redeemed)), cJSON_AddItemToObject(item,"already",cJSON_CreateString(numstr));
            sprintf(numstr,"%.2f",dstr(tp->estNXT)), cJSON_AddItemToObject(item,"estNXT",cJSON_CreateString(numstr));
            sprintf(numstr,"%.3f",pending), cJSON_AddItemToObject(item,"wait",cJSON_CreateString(numstr));
        }*/
    }
    return(numredeems);
}

int32_t process_destaddr(int32_t *alreadysentp,cJSON **arrayp,char *destaddrs[MAX_MULTISIG_OUTPUTS],uint64_t destamounts[MAX_MULTISIG_OUTPUTS],uint64_t redeems[MAX_MULTISIG_OUTPUTS],uint64_t *pending_withdrawp,struct coin_info *cp,uint64_t nxt64bits,struct NXT_asset *ap,char *destaddr,struct NXT_assettxid *tp,int32_t numredeems,char *sender)
{
    struct address_entry *entries,*entry;
    struct coin_txidind *cointp;
    int32_t j,n,createdflag,processflag = 0;
    *alreadysentp = 0;
    fprintf(stderr,"[");
    if ( (entries= get_address_entries(&n,cp->name,destaddr)) != 0 )
    {
        fprintf(stderr,"].%d ",n);
        *alreadysentp = 1;
        for (j=0; j<n; j++)
        {
            entry = &entries[j];
            if ( entry->vinflag == 0 )
            {
                cointp = _get_cointp(&createdflag,cp->name,entry->blocknum,entry->txind,entry->v);
                if ( cointp->redeemtxid == tp->redeemtxid )
                    break;
            }
        }
        if ( j == n )
            processflag = 1;
        free(entries);
    }
    if ( processflag != 0 )
        numredeems = process_redeem(alreadysentp,arrayp,destaddrs,destamounts,redeems,pending_withdrawp,cp,nxt64bits,ap,destaddr,tp,numredeems,sender);
    return(numredeems);
}

uint64_t process_consensus(cJSON **jsonp,struct coin_info *cp,int32_t sendmoney)
{
    struct batch_info *otherwp;
    int32_t i,readyflag,gatewayid,matches = 0;
    char numstr[64],*cointxid;
    struct rawtransaction *rp;
    cJSON *array,*item;
    uint64_t pendingtxid,AMtxid = 0;
    if ( Global_mp->gatewayid < 0 )
        return(0);
    readyflag = ready_to_xferassets(&pendingtxid);
    printf("%s: readyflag.%d gateway.%d\n",cp->name,readyflag,Global_mp->gatewayid);
    for (gatewayid=0; gatewayid<NUM_GATEWAYS; gatewayid++)
    {
        otherwp = &cp->withdrawinfos[gatewayid];
        if ( cp->BATCH.rawtx.batchcrc != otherwp->rawtx.batchcrc )
        {
            fprintf(stderr,"%08x miscompares with gatewayid.%d which has crc %08x\n",cp->BATCH.rawtx.batchcrc,gatewayid,otherwp->rawtx.batchcrc);
        }
        else if ( cp->BATCH.rawtx.batchcrc != 0 )
            matches++;
    }
    rp = &cp->withdrawinfos[Global_mp->gatewayid].rawtx;
    array = cJSON_CreateArray();
    printf("json.%p numredeems.%d\n",*jsonp,rp->numredeems);
    if ( rp->numredeems > 0 )
    {
        for (i=0; i<rp->numredeems; i++)
        {
            fprintf(stderr,"redeem entry i.%d (%llu %p %.8f)\n",i,(long long)rp->redeems[i],rp->destaddrs[i],dstr(rp->destamounts[i]));
            item = cJSON_CreateObject();
            sprintf(numstr,"%llu",(long long)rp->redeems[i]), cJSON_AddItemToObject(item,"redeemtxid",cJSON_CreateString(numstr));
            if ( rp->destaddrs[i+1] != 0 )
                cJSON_AddItemToObject(item,"destaddr",cJSON_CreateString(rp->destaddrs[i+1]));
            sprintf(numstr,"%.8f",dstr(rp->destamounts[i+1])), cJSON_AddItemToObject(item,"amount",cJSON_CreateString(numstr));
            cJSON_AddItemToArray(array,item);
        }
    }
    if ( cJSON_GetArraySize(array) > 0 )
        cJSON_AddItemToObject(*jsonp,"allredeems",array);
    else free_json(array);
    if ( sendmoney != 0 && matches == NUM_GATEWAYS )
    {
        fprintf(stderr,"all gateways match\n");
        if ( readyflag != 0 )//Global_mp->gatewayid == 0 )
        {
            if ( (cointxid= sign_and_sendmoney(&AMtxid,cp,(uint32_t)cp->RTblockheight)) != 0 )
            {
                cJSON_AddItemToObject(*jsonp,"batchsigned",cJSON_CreateString(rp->batchsigned));
                cJSON_AddItemToObject(*jsonp,"cointxid",cJSON_CreateString(cointxid));
                sprintf(numstr,"%llu",(long long)AMtxid), cJSON_AddItemToObject(*jsonp,"AMtxid",cJSON_CreateString(numstr));
                fprintf(stderr,"done and publish coin.(%s) AM.%llu\n",cointxid,(long long)AMtxid);
                //publish_withdraw_info(cp,&cp->BATCH);
                free(cointxid);
            }
            else
            {
                fprintf(stderr,"error signing?\n");
                cp->BATCH.rawtx.batchcrc = 0;
                cp->withdrawinfos[Global_mp->gatewayid].rawtx.batchcrc = 0;
                cp->withdrawinfos[Global_mp->gatewayid].W.cointxid[0] = 0;
            }
        }
    }
    printf("last array AMtxid.%llu\n",(long long)AMtxid);
    array = cJSON_CreateArray();
    for (gatewayid=0; gatewayid<NUM_GATEWAYS; gatewayid++)
        cJSON_AddItemToArray(array,cJSON_CreateNumber(cp->withdrawinfos[gatewayid].rawtx.batchcrc));
    cJSON_AddItemToObject(*jsonp,"crcs",array);
    cJSON_AddItemToObject(*jsonp,"allwithdraws",cJSON_CreateNumber(rp->numredeems));
    sprintf(numstr,"%.8f",dstr(rp->amount)), cJSON_AddItemToObject(*jsonp,"pending_withdraw",cJSON_CreateString(numstr));
    return(AMtxid);
}

void process_withdraws(cJSON **jsonp,struct multisig_addr **msigs,int32_t nummsigs,uint64_t unspent,struct coin_info *cp,struct NXT_asset *ap,char *specialNXT,int32_t sendmoney,uint64_t circulation,uint64_t pendingdeposits,char *redeemer)
{
    struct NXT_assettxid *tp;
    struct rawtransaction *rp;
    cJSON *array;
    int64_t balance;
    int32_t i,j,numredeems,alreadyspent,published = 0;
    uint64_t destamounts[MAX_MULTISIG_OUTPUTS],redeems[MAX_MULTISIG_OUTPUTS],nxt64bits,sum,pending_withdraw = 0;
    char withdrawaddr[64],sender[64],redeemtxid[64],numstr[64],*destaddrs[MAX_MULTISIG_OUTPUTS],*destaddr="",*batchsigned,*str;
    if ( ap->num <= 0 )
        return;
    rp = &cp->BATCH.rawtx;
    clear_BATCH(rp);
    rp->numoutputs = init_batchoutputs(cp,rp,cp->txfee);
    numredeems = 0;
    memset(redeems,0,sizeof(redeems));
    memset(destaddrs,0,sizeof(destaddrs));
    memset(destamounts,0,sizeof(destamounts));
    nxt64bits = calc_nxt64bits(specialNXT);
    array = cJSON_CreateArray();
    for (i=sum=0; i<ap->num; i++)
    {
        tp = ap->txids[i];
        if ( Debuglevel > 2 )
            printf("%d of %d: (%s) redeem.%llu (%llu vs %llu) (%llu vs %llu)\n",i,ap->num,tp->comment,(long long)tp->redeemtxid,(long long)tp->receiverbits,(long long)nxt64bits,(long long)tp->assetbits,(long long)ap->assetbits);
        str = (tp->AMtxidbits != 0) ? ": REDEEMED" : " <- redeem";
        if ( tp->redeemtxid != 0 && tp->receiverbits == nxt64bits && tp->assetbits == ap->assetbits && tp->U.assetoshis >= MIN_DEPOSIT_FACTOR*(cp->txfee + cp->NXTfee_equiv) )
        {
            expand_nxt64bits(sender,tp->senderbits);
            if ( tp->AMtxidbits == 0 && (destaddr= parse_withdraw_instructions(withdrawaddr,sender,cp,tp,ap)) != 0 && destaddr[0] != 0 )
            {
                stripwhite_ns(destaddr,strlen(destaddr));
                printf("i.%d of %d: process_destaddr.(%s) %.8f\n",i,ap->num,destaddr,dstr(tp->U.assetoshis));
                numredeems = process_destaddr(&alreadyspent,&array,destaddrs,destamounts,redeems,&pending_withdraw,cp,nxt64bits,ap,destaddr,tp,numredeems,sender);
                tp->AMtxidbits = alreadyspent;
                if ( numredeems >= MAX_MULTISIG_OUTPUTS-1 )
                    break;
            }
        }
        if ( Debuglevel > 2 )
            printf("%s (%s, %s) %llu %s %llu %.8f %.8f | %llu\n",cp->name,destaddr,withdrawaddr,(long long)nxt64bits,str,(long long)tp->redeemtxid,dstr(tp->quantity),dstr(tp->U.assetoshis),(long long)tp->AMtxidbits);
    }
    cJSON_AddItemToObject(*jsonp,"redeems",array);
    balance = unspent - pending_withdraw - circulation - pendingdeposits;
    sprintf(numstr,"%.8f",dstr(balance)), cJSON_AddItemToObject(*jsonp,"revenues",cJSON_CreateString(numstr));
    if ( cp->boughtNXT > 0 )
    {
        cJSON_AddItemToObject(*jsonp,"boughtNXT",cJSON_CreateNumber(cp->boughtNXT));
        if ( balance > 0. )
            sprintf(numstr,"%.8f",dstr((double)cp->boughtNXT / balance)), cJSON_AddItemToObject(*jsonp,"costbasis",cJSON_CreateString(numstr));
    }
    cp->BATCH.C.balance = balance;
    cp->BATCH.C.circulation = circulation;
    cp->BATCH.C.unspent = unspent;
    cp->BATCH.C.pendingdeposits = pendingdeposits;
    cp->BATCH.C.pendingwithdraws = pending_withdraw;
    cp->BATCH.C.boughtNXT = cp->boughtNXT;
    array = cJSON_CreateArray();
    if ( (int64_t)pending_withdraw >= ((5 * cp->NXTfee_equiv) - (numredeems * (cp->txfee + cp->NXTfee_equiv))) )
    {
        for (sum=j=0; j<numredeems&&rp->numoutputs<(int)(sizeof(rp->destaddrs)/sizeof(*rp->destaddrs))-1; j++)
        {
            fprintf(stderr,"rp->numoutputs.%d/%d j.%d of %d: (%s) [%llu %.8f]\n",rp->numoutputs,(int)(sizeof(rp->destaddrs)/sizeof(*rp->destaddrs))-1,j,numredeems,destaddrs[j],(long long)redeems[j],dstr(destamounts[j]));
            if ( add_destaddress(rp,destaddrs[j],destamounts[j]) < 0 )
            {
                printf("error adding %s %.8f | numredeems.%d numoutputs.%d\n",destaddrs[j],dstr(destamounts[j]),rp->numredeems,rp->numoutputs);
                break;
            }
            sum += destamounts[j];
            expand_nxt64bits(redeemtxid,redeems[j]);
            rp->redeems[rp->numredeems++] = redeems[j];
            if ( rp->numredeems >= (int)(sizeof(rp->redeems)/sizeof(*rp->redeems)) )
            {
                printf("max numredeems\n");
                break;
            }
        }
        printf("pending_withdraw %.8f -> sum %.8f numredeems.%d numoutputs.%d\n",dstr(pending_withdraw),dstr(sum),rp->numredeems,rp->numoutputs);
        pending_withdraw = sum;
        batchsigned = calc_batchwithdraw(msigs,nummsigs,cp,rp,pending_withdraw,unspent,ap);
        if ( batchsigned != 0 )
        {
            printf("BATCHSIGNED.(%s)\n",batchsigned);
            //if ( sendmoney == 0 )
                publish_withdraw_info(cp,&cp->BATCH), published++;
            process_consensus(jsonp,cp,sendmoney);
            free(batchsigned);
        }
    }
    else
    {
        if ( numredeems > 0 )
            printf("%.8f is not enough to pay for MGWfees.%s %.8f for %d redeems\n",dstr(pending_withdraw),cp->name,dstr(cp->NXTfee_equiv),numredeems);
        pending_withdraw = 0;
    }
    //if ( pendingdeposits != 0 && published == 0 )
        publish_withdraw_info(cp,&cp->BATCH);
}

int32_t cmp_batch_depositinfo(struct consensus_info *refbatch,struct consensus_info *batch)
{
    printf("cmp_batch_depositinfo (%.8f %.8f %.8f %.8f %.8f).%d vs (%.8f %.8f %.8f %.8f %.8f).%d\n",dstr(refbatch->balance),dstr(refbatch->circulation),dstr(refbatch->unspent),dstr(refbatch->pendingdeposits),dstr(refbatch->pendingwithdraws),refbatch->boughtNXT,dstr(batch->balance),dstr(batch->circulation),dstr(batch->unspent),dstr(batch->pendingdeposits),dstr(batch->pendingwithdraws),batch->boughtNXT);
    if ( refbatch->pendingdeposits == 0 && refbatch->pendingwithdraws == 0 )
    {
        printf("no deposits or withdraws\n");
        return(-1);
    }
    if ( fabs(dstr(refbatch->balance) - dstr(batch->balance)) > 1 || fabs(dstr(refbatch->circulation) - dstr(batch->circulation)) > 1 || fabs(dstr(refbatch->unspent) - dstr(batch->unspent)) > 1 || fabs(dstr(refbatch->pendingdeposits) - dstr(batch->pendingdeposits)) > 1 || fabs(dstr(refbatch->pendingwithdraws) - dstr(batch->pendingwithdraws)) > 1 )// || refbatch->boughtNXT != batch->boughtNXT )
    {
        printf("disagreement >1 %.8f %.8f %.8f %.8f %.8f\n",fabs(dstr(refbatch->balance) - dstr(batch->balance)),fabs(dstr(refbatch->circulation) - dstr(batch->circulation)),fabs(dstr(refbatch->unspent) - dstr(batch->unspent)),fabs(dstr(refbatch->pendingdeposits) - dstr(batch->pendingdeposits)),fabs(dstr(refbatch->pendingwithdraws) - dstr(batch->pendingwithdraws)));
        return(-1);
    }
    if ( dstr(refbatch->balance) < -1 )
    {
        printf("too low balance\n");
        return(-1);
    }
    return(0);
}

cJSON *process_MGW(int32_t actionflag,struct coin_info *cp,struct NXT_asset *ap,char *ipaddrs[3],char **specialNXTaddrs,char *issuer,double startmilli,char *NXTaddr,char *depositors_pubkey)
{
    cJSON *json = cJSON_CreateObject();
    char numstr[128];
    uint64_t pendingdeposits,circulation,unspent = 0;
    char *retstr;
    int32_t i,nummsigs;
    struct multisig_addr **msigs;
    pendingdeposits = 0;
    circulation = calc_circulation(0,ap,specialNXTaddrs);
    fprintf(stderr,"circulation %.8f\n",dstr(circulation));
    if ( (msigs= (struct multisig_addr **)copy_all_DBentries(&nummsigs,MULTISIG_DATA)) != 0 )
    {
        //printf("nummsigs.%d\n",nummsigs);
        if ( actionflag >= 0 )
        {
            pendingdeposits = process_deposits(&json,&unspent,msigs,nummsigs,cp,ipaddrs,specialNXTaddrs,issuer,ap,actionflag > 0,circulation,NXTaddr,depositors_pubkey);
            retstr = cJSON_Print(json);
            //fprintf(stderr,"actionflag.%d retstr.(%s)\n",actionflag,retstr);
            free(retstr), retstr = 0;
        }
        if ( actionflag <= 0 )
        {
            if ( actionflag < 0 )
                pendingdeposits = process_deposits(&json,&unspent,msigs,nummsigs,cp,ipaddrs,specialNXTaddrs,issuer,ap,actionflag > 0,circulation,NXTaddr,depositors_pubkey);
            process_withdraws(&json,msigs,nummsigs,unspent,cp,ap,issuer,actionflag < 0,circulation,pendingdeposits,NXTaddr);
        }
        sprintf(numstr,"%.3f",(milliseconds()-startmilli)/1000.), cJSON_AddItemToObject(json,"seconds",cJSON_CreateString(numstr));
        for (i=0; i<nummsigs; i++)
            free(msigs[i]);
        free(msigs);
    }
    return(json);
}

void MGW_useracct_str(cJSON **jsonp,int32_t actionflag,struct coin_info *cp,struct NXT_asset *ap,uint64_t nxt64bits,char *issuerNXT,char **specialNXTaddrs)
{
    char coinaddr[1024],txidstr[1024],depositaddr[128],withdrawaddr[512],rsacct[64],depositstr[64],numstr[128],redeemstr[128],NXTaddr[64];
    struct multisig_addr **msigs,*msig;
    struct address_entry *entries;
    struct NXT_assettxid *tp;
    cJSON *autojson,*array,*item;
    int32_t i,j,n,nummsigs,lastbuyNXT,buyNXT = 0;
    uint64_t pendingtxid,value,deposittxid,withdrew,issuerbits,balance,assetoshis,pending_withdraws,pending_deposits;
    expand_nxt64bits(NXTaddr,nxt64bits);
    if ( *jsonp == 0 )
        *jsonp = cJSON_CreateObject();
    depositaddr[0] = 0;
    lastbuyNXT = 0;
    array = cJSON_CreateArray();
    issuerbits = calc_nxt64bits(issuerNXT);
    pending_withdraws = pending_deposits = 0;
    if ( (msigs= (struct multisig_addr **)copy_all_DBentries(&nummsigs,MULTISIG_DATA)) != 0 )
    {
        for (i=0; i<nummsigs; i++)
        {
            if ( (msig= msigs[i]) != 0 )
            {
                if ( strcmp(msig->NXTaddr,NXTaddr) == 0 )
                {
                    buyNXT += msig->buyNXT;
                    if ( msig->multisigaddr[0] != 0 )
                    {
                        lastbuyNXT = msig->buyNXT;
                        strcpy(depositaddr,msig->multisigaddr);
                    }
                    if ( (entries= get_address_entries(&n,cp->name,msig->multisigaddr)) != 0 )
                    {
                        printf("got %d entries for (%s)\n",n,msig->multisigaddr);
                        for (j=0; j<n; j++)
                        {
                            item = cJSON_CreateObject();
                            if ( (value= conv_address_entry(coinaddr,txidstr,0,cp,&entries[j])) != 0 )
                            {
                                cJSON_AddItemToObject(item,"vout",cJSON_CreateNumber(entries[j].v));
                                cJSON_AddItemToObject(item,"height",cJSON_CreateNumber(entries[j].blocknum));
                                cJSON_AddItemToObject(item,"txid",cJSON_CreateString(txidstr));
                                cJSON_AddItemToObject(item,"addr",cJSON_CreateString(coinaddr));
                                sprintf(numstr,"%.8f",dstr(value)), cJSON_AddItemToObject(item,"value",cJSON_CreateString(numstr));
                                if ( (deposittxid= get_deposittxid(ap,txidstr,entries[j].v)) != 0 )
                                {
                                    expand_nxt64bits(depositstr,deposittxid);
                                    cJSON_AddItemToObject(item,"depositid",cJSON_CreateString(depositstr));
                                    cJSON_AddItemToObject(item,"status",cJSON_CreateString("complete"));
                                }
                                else
                                {
                                    if ( value >= (cp->txfee + cp->NXTfee_equiv) * MIN_DEPOSIT_FACTOR )
                                    {
                                        pending_deposits += value;
                                        cJSON_AddItemToObject(item,"status",cJSON_CreateString("pending"));
                                    }
                                    else cJSON_AddItemToObject(item,"status",cJSON_CreateString("too small"));
                                }
                                cJSON_AddItemToArray(array,item);
                            }
                        }
                    }
                }
                free(msig);
            }
        }
        free(msigs);
        cJSON_AddItemToObject(*jsonp,"userdeposits",array);
    }
    if ( ap->num > 0 )
    {
        array = cJSON_CreateArray();
        for (i=0; i<ap->num; i++)
        {
            tp = ap->txids[i];
            if ( tp->redeemtxid != 0 && tp->senderbits == nxt64bits && tp->receiverbits == issuerbits && tp->assetbits == ap->assetbits )
            {
                item = 0;
                withdrawaddr[0] = 0;
                if ( tp->comment != 0 )//&& (tp->comment[0] == '{' || tp->comment[0] == '[') )
                {
                    if ( (item= cJSON_Parse(tp->comment)) != 0 )
                        copy_cJSON(withdrawaddr,cJSON_GetObjectItem(item,"withdrawaddr"));
                }
                if ( item == 0 )
                    item = cJSON_CreateObject();
                expand_nxt64bits(redeemstr,tp->redeemtxid), cJSON_AddItemToObject(item,"redeemtxid",cJSON_CreateString(redeemstr));
                value = tp->U.assetoshis;
                sprintf(numstr,"%.8f",dstr(value)), cJSON_AddItemToObject(item,"value",cJSON_CreateString(numstr));
                cJSON_AddItemToObject(item,"timestamp",cJSON_CreateNumber(tp->timestamp));
                if ( value < MIN_DEPOSIT_FACTOR*(cp->txfee + cp->NXTfee_equiv) )
                    cJSON_AddItemToObject(item,"status",cJSON_CreateString("too small"));
                else
                {
                    autojson = gen_autoconvert_json(tp);
                    if ( autojson != 0 )
                    {
                        cJSON_AddItemToObject(item,"autoconv",autojson);
                        if ( actionflag > 0 && (tp->convexpiration == 0 || time(NULL) < tp->convexpiration) )
                            if ( (pendingtxid= autoconvert(tp)) != 0 )
                                add_pendingxfer(0,pendingtxid);
                    }
                    if ( tp->AMtxidbits == 0 && is_limbo_redeem(cp,tp->AMtxidbits) == 0 )
                        cJSON_AddItemToObject(item,"status",cJSON_CreateString("queued"));
                    else if ( tp->AMtxidbits == 1 ) cJSON_AddItemToObject(item,"status",cJSON_CreateString("finished"));
                    else if ( is_limbo_redeem(cp,tp->AMtxidbits) != 0 ) cJSON_AddItemToObject(item,"status",cJSON_CreateString("completed"));
                    else if ( tp->AMtxidbits > 1 )
                    {
                        expand_nxt64bits(redeemstr,tp->AMtxidbits), cJSON_AddItemToObject(item,"sentAM",cJSON_CreateString(redeemstr));
                        if ( (withdrew= get_sentAM_cointxid(txidstr,cp,autojson,withdrawaddr,tp->redeemtxid,tp->AMtxidbits)) <= 0 )
                            cJSON_AddItemToObject(item,"status",cJSON_CreateString("error"));
                        else
                        {
                            cJSON_AddItemToObject(item,"cointxid",cJSON_CreateString(txidstr));
                            sprintf(numstr,"%.8f",dstr(withdrew)), cJSON_AddItemToObject(item,"withdrew",cJSON_CreateString(numstr));
                            cJSON_AddItemToObject(item,"status",cJSON_CreateString("completed"));
                        }
                    }
                    else cJSON_AddItemToObject(item,"status",cJSON_CreateString("unexpected"));
                }
                cJSON_AddItemToArray(array,item);
            }
        }
        cJSON_AddItemToObject(*jsonp,"userwithdraws",array);
    }
    assetoshis = get_accountassets(0,ap,NXTaddr);
    balance = assetoshis + pending_deposits + pending_withdraws;
    sprintf(numstr,"%.8f",dstr(assetoshis)), cJSON_AddItemToObject(*jsonp,"AEbalance",cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",dstr(pending_deposits)), cJSON_AddItemToObject(*jsonp,"pending_userdeposits",cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",dstr(pending_withdraws)), cJSON_AddItemToObject(*jsonp,"pending_userwithdraws",cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",dstr(balance)), cJSON_AddItemToObject(*jsonp,"userbalance",cJSON_CreateString(numstr));
    cJSON_AddItemToObject(*jsonp,"userNXT",cJSON_CreateString(NXTaddr));
    conv_rsacctstr(rsacct,nxt64bits);
    cJSON_AddItemToObject(*jsonp,"userRS",cJSON_CreateString(rsacct));
    if ( depositaddr[0] != 0 )
        cJSON_AddItemToObject(*jsonp,"depositaddr",cJSON_CreateString(depositaddr));
    if ( lastbuyNXT != 0 )
    {
        if ( buyNXT != lastbuyNXT )
            cJSON_AddItemToObject(*jsonp,"totalbuyNXT",cJSON_CreateNumber(buyNXT));
        else cJSON_AddItemToObject(*jsonp,"buyNXT",cJSON_CreateNumber(lastbuyNXT));
    }
}

char *check_MGW_cache(struct coin_info *cp,char *userNXTaddr)
{
    char *retstr = 0;
    FILE *fp;
    long fsize;
    cJSON *json;
    char fname[512],*buf;
    uint32_t coinheight,height,timestamp,cacheheight,cachetimestamp;
return(0);
    coinheight = get_blockheight(cp);
    if ( cp->uptodate >= (coinheight - cp->minconfirms) )
    {
        height = get_NXTheight();
        timestamp = (uint32_t)time(NULL);
        set_MGW_statusfname(fname,userNXTaddr);
        if ( (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
        {
            fseek(fp,0,SEEK_END);
            fsize = ftell(fp);
            rewind(fp);
            buf = calloc(1,fsize);
            if ( fread(buf,1,fsize,fp) != fsize )
                printf("fread error in check_MGW_cache\n");
            fclose(fp);
            if ( (json= cJSON_Parse(buf)) != 0 )
            {
                cacheheight = (int32_t)get_cJSON_int(json,"NXTheight");
                cachetimestamp = (int32_t)get_cJSON_int(json,"timestamp");
                printf("uptodate.%u vs %u | cache.(%d t%d) vs (%d t%d)\n",cp->uptodate,(coinheight - cp->minconfirms),cacheheight,cachetimestamp,height,timestamp);
                if ( cacheheight > 0 && cacheheight >= height ) // > can happen on blockchain rescans
                    retstr = buf, buf = 0;
                free_json(json);
            }
            if ( buf != 0 )
                free(buf);
        }
    }
    return(retstr);
}

cJSON *auto_process_MGW(char **specialNXTaddrs,struct coin_info *cp,cJSON *origjson,char *NXTaddr,char *depositors_pubkey)
{
    static portable_mutex_t mutex;
    static int32_t didinit;
    struct batch_info *bp;
    cJSON *json = 0;
    struct NXT_asset *ap;
    int32_t i,createdflag;
    char *ipaddrs[3];
    if ( didinit == 0 )
    {
        portable_mutex_init(&mutex);
        didinit = 1;
    }
    for (i=0; i<=NUM_GATEWAYS; i++)
    {
        bp = (i < NUM_GATEWAYS) ? &cp->withdrawinfos[i] : &cp->BATCH;
        printf("gateway.%d: crc.%u %x balance %.8f pendingdeposits %.8f unspent %.8f\n",i,bp->rawtx.batchcrc,bp->rawtx.batchcrc,dstr(bp->C.balance),dstr(bp->C.pendingdeposits),dstr(bp->C.unspent));
    }
    ap = get_NXTasset(&createdflag,Global_mp,cp->assetid);
    for (i=0; i<3; i++)
        ipaddrs[i] = Server_ipaddrs[i];
    portable_mutex_lock(&mutex);
    if ( cmp_batch_depositinfo(&cp->withdrawinfos[2].C,&cp->withdrawinfos[1].C) == 0 && cmp_batch_depositinfo(&cp->withdrawinfos[0].C,&cp->withdrawinfos[2].C) == 0 )
    {
        if ( cp->withdrawinfos[0].C.pendingwithdraws > 0 && cp->withdrawinfos[0].rawtx.batchcrc != 0 && cp->withdrawinfos[0].rawtx.batchcrc == cp->withdrawinfos[1].rawtx.batchcrc && cp->withdrawinfos[0].rawtx.batchcrc == cp->withdrawinfos[2].rawtx.batchcrc )
        {
            printf(">>>>>>>>>>>>>> STARTING AUTO WITHDRAW %u %.8f <<<<<<<<<<<<<<<<<<<\n",cp->withdrawinfos[2].rawtx.batchcrc,dstr(cp->withdrawinfos[2].C.pendingwithdraws));
            json = process_MGW(-1,cp,ap,ipaddrs,specialNXTaddrs,cp->MGWissuer,milliseconds(),NXTaddr,depositors_pubkey);
        }
        else if ( cp->withdrawinfos[2].C.pendingdeposits > 0 )
        {
            printf(">>>>>>>>>>>>>> STARTING AUTO DEPOSIT %.8f <<<<<<<<<<<<<<<<<<<\n",dstr(cp->withdrawinfos[2].C.pendingdeposits));
            json = process_MGW(1,cp,ap,ipaddrs,specialNXTaddrs,cp->MGWissuer,milliseconds(),NXTaddr,depositors_pubkey);
        }
    }
    portable_mutex_unlock(&mutex);
    if ( origjson == 0 )
        return(json);
    else if ( json == 0 )
        return(origjson);
    cJSON_AddItemToObject(origjson,"auto",json);
    return(origjson);
}

cJSON *verbose_msigstats(char **specialNXTaddrs,struct coin_info *cp,struct NXT_asset *ap)
{
    cJSON *json = cJSON_CreateArray();
    
    return(json);
}

cJSON *verbose_unspentstats(char **specialNXTaddrs,struct coin_info *cp,struct NXT_asset *ap)
{
    cJSON *json = cJSON_CreateArray();
    
    return(json);
}

char *verbose_MGWstats(char **specialNXTaddrs,struct coin_info *cp,struct NXT_asset *ap)
{
    cJSON *json = cJSON_CreateObject();
    char *retstr;
    cJSON_AddItemToObject(json,"msig",verbose_msigstats(specialNXTaddrs,cp,ap));
    cJSON_AddItemToObject(json,"unspents",verbose_unspentstats(specialNXTaddrs,cp,ap));
    retstr = cJSON_Print(json);
    free_json(json);
    return(retstr);
}

char *MGW(char *issuerNXT,int32_t rescan,int32_t actionflag,char *coin,char *assetstr,char *NXT0,char *NXT1,char *NXT2,char *ip0,char *ip1,char *ip2,char *exclude0,char *exclude1,char *exclude2,char *refNXTaddr,char *depositors_pubkey)
{
    static int32_t firsttimestamp;
    static char **specialNXTaddrs;
    char retbuf[4096],NXTaddr[64],rsacct[64],*ipaddrs[3],*retstr = 0;
    struct coin_info *cp = 0;
    uint64_t pendingtxid,nxt64bits = 0;
    double startmilli = milliseconds();
    int32_t i,createdflag;
    struct NXT_asset *ap = 0;
    cJSON *json = 0;
    retbuf[0] = 0;
    if ( MGW_initdone == 0 )
        sprintf(retbuf,"{\"error\":\"MGW not initialized yet\"");
    else
    {
        if ( refNXTaddr != 0 && refNXTaddr[0] == 0 )
        {
            if ( rescan != 0 )
                sprintf(retbuf,"{\"error\":\"need NXT address to rescan\"");
            refNXTaddr = 0;
            nxt64bits = 0;
        }
        else
        {
            nxt64bits = conv_rsacctstr(refNXTaddr,0);
            expand_nxt64bits(NXTaddr,nxt64bits);
        }
        if ( depositors_pubkey != 0 && depositors_pubkey[0] == 0 )
            depositors_pubkey = 0;
        if ( (cp= get_coin_info(coin)) == 0 )
        {
            ap = get_NXTasset(&createdflag,Global_mp,assetstr);
            cp = conv_assetid(assetstr);
            if ( cp == 0 || ap == 0 )
                sprintf(retbuf,"{\"error\":\"dont have coin_info for asset (%s) ap.%p\"",assetstr,ap);
        }
        else
        {
            if ( assetstr != 0 && strcmp(cp->assetid,assetstr) != 0 )
                sprintf(retbuf,"{\"error\":\"mismatched assetid\"");
            ap = get_NXTasset(&createdflag,Global_mp,cp->assetid);
        }
        if ( firsttimestamp == 0 )
            get_NXTblock(&firsttimestamp);
    }
    if ( retbuf[0] != 0 )
    {
        if ( refNXTaddr != 0 && nxt64bits != 0 )
        {
            conv_rsacctstr(rsacct,nxt64bits);
            sprintf(retbuf+strlen(retbuf),",\"userNXT\":\"%llu\",\"RS\":\"%s\"",(long long)nxt64bits,rsacct);
        }
        sprintf(retbuf+strlen(retbuf),",\"gatewayid\":%d",Global_mp->gatewayid);
        if ( (cp= get_coin_info("BTCD")) != 0 )
            sprintf(retbuf+strlen(retbuf),",\"requestType\":\"MGWresponse\",\"NXT\":\"%s\"}",cp->srvNXTADDR);
        printf("MGWERROR.(%s)\n",retbuf);
        return(clonestr(retbuf));
    }
    if ( issuerNXT == 0 || issuerNXT[0] == 0 )
        issuerNXT = cp->MGWissuer;
    if ( NXT0 != 0 && NXT0[0] != 0 )
    {
        specialNXTaddrs = calloc(16,sizeof(*specialNXTaddrs));
        init_specialNXTaddrs(specialNXTaddrs,ipaddrs,issuerNXT,NXT0,NXT1,NXT2,ip0,ip1,ip2,exclude0,exclude1,exclude2);
    } else specialNXTaddrs = MGW_whitelist;
    if ( actionflag == 100 )
        return(verbose_MGWstats(specialNXTaddrs,cp,ap));
    pendingtxid = 0;
    if ( nxt64bits != 0 && rescan != 0 )
    {
        if ( (retstr= check_MGW_cache(cp,NXTaddr)) == 0 )
        {
            pendingtxid = update_NXTblockchain_info(cp,specialNXTaddrs,issuerNXT); // user commands
            if ( actionflag == 0 || pendingtxid == 0 )
            {
                json = process_MGW(0,cp,ap,ipaddrs,specialNXTaddrs,issuerNXT,startmilli,NXTaddr,depositors_pubkey);
                MGW_useracct_str(&json,actionflag,cp,ap,nxt64bits,issuerNXT,specialNXTaddrs);
            } else retstr = clonestr("\"error\":\"action has to wait for pendingtxid to be seen\"}");
        } else publish_withdraw_info(cp,&cp->BATCH);
    }
    else
    {
        if ( (pendingtxid= update_NXTblockchain_info(cp,specialNXTaddrs,issuerNXT)) == 0 )
            json = process_MGW(actionflag,cp,ap,ipaddrs,specialNXTaddrs,issuerNXT,startmilli,NXTaddr,depositors_pubkey);
        else retstr = wait_for_pendingtxid(cp,specialNXTaddrs,issuerNXT,pendingtxid);
    }
    if ( pendingtxid == 0 && actionflag == 0 && Global_mp->gatewayid == NUM_GATEWAYS-1 )
        json = auto_process_MGW(specialNXTaddrs,cp,json,NXTaddr,depositors_pubkey);
    if ( json != 0 )
    {
        cJSON *array;
        if ( NXTaddr[0] == 0 )
            strcpy(NXTaddr,cp->name);
        if ( (cp= get_coin_info("BTCD")) != 0 )
            cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(cp->srvNXTADDR));
        cJSON_AddItemToObject(json,"requestType",cJSON_CreateString("MGWresponse"));
        cJSON_AddItemToObject(json,"gatewayid",cJSON_CreateNumber(Global_mp->gatewayid));
        cJSON_AddItemToObject(json,"timestamp",cJSON_CreateNumber(time(NULL)));
        cJSON_AddItemToObject(json,"NXTheight",cJSON_CreateNumber(get_NXTheight()));
        array = cJSON_CreateArray();
        for (i=0; i<NUM_GATEWAYS; i++)
            cJSON_AddItemToArray(array,cJSON_CreateNumber(dstr(cp->withdrawinfos[i].C.balance)));
        for (i=0; i<NUM_GATEWAYS; i++)
            cJSON_AddItemToArray(array,cJSON_CreateNumber(dstr(cp->withdrawinfos[i].C.unspent)));
        for (i=0; i<NUM_GATEWAYS; i++)
            cJSON_AddItemToArray(array,cJSON_CreateNumber(dstr(cp->withdrawinfos[i].C.pendingdeposits)));
        for (i=0; i<NUM_GATEWAYS; i++)
            cJSON_AddItemToArray(array,cJSON_CreateNumber(dstr(cp->withdrawinfos[i].C.pendingwithdraws)));
        for (i=0; i<NUM_GATEWAYS; i++)
            cJSON_AddItemToArray(array,cJSON_CreateNumber(cp->withdrawinfos[i].rawtx.batchcrc));
        
        cJSON_AddItemToObject(json,"depinfo",array);
        retstr = cJSON_Print(json);
        save_MGW_status(NXTaddr,retstr);
        //stripwhite_ns(retstr,strlen(retstr));
        free_json(json);
    }
    if ( specialNXTaddrs != MGW_whitelist )
    {
        for (i=0; specialNXTaddrs[i]!=0; i++)
            free(specialNXTaddrs[i]);
        free(specialNXTaddrs);
    }
    if ( retstr == 0 )
        retstr = clonestr("{}");
    //printf("MGW.(%s)\n",retstr);
    return(retstr);
}

char *invoke_MGW(char **specialNXTaddrs,struct coin_info *cp,struct multisig_addr *msig,int32_t actionflag)
{
    char *ipaddrs[3],*retstr = 0;
    int32_t j,createdflag;
    struct NXT_asset *ap;
    cJSON *json = 0;
    update_NXTblockchain_info(cp,specialNXTaddrs,cp->MGWissuer);
    ap = get_NXTasset(&createdflag,Global_mp,cp->assetid);
    for (j=0; j<3; j++)
        ipaddrs[j] = Server_ipaddrs[j];
    if ( actionflag == 0 || msig == 0 )
    {
        json = process_MGW(actionflag,cp,ap,ipaddrs,specialNXTaddrs,cp->MGWissuer,milliseconds(),0,0);
        init_multisig(specialNXTaddrs,cp);
        init_deposit(specialNXTaddrs,cp);
        init_moneysent(specialNXTaddrs,cp);
    }
    else
    {
        printf("invoke_MGW.(%s) (%s) buyNXT.%d\n",msig->NXTaddr,msig->NXTpubkey,msig->buyNXT);
        json = process_MGW(actionflag,cp,ap,ipaddrs,specialNXTaddrs,cp->MGWissuer,milliseconds(),msig->NXTaddr,msig->NXTpubkey);
    }
    if ( json != 0 )
    {
        retstr = cJSON_Print(json);
        free_json(json);
        if ( msig != 0 )
            fprintf(stderr,"invoke_MGW for (%s) ->\n%s\n",msig->NXTaddr,retstr);
    }
    return(retstr);
}

//int32_t save_rawblock(int32_t dispflag,FILE *fp,struct rawblock *raw,uint32_t blocknum);
//uint32_t _get_blockinfo(struct rawblock *raw,struct coin_info *cp,uint32_t blockheight);
//double estimate_completion(char *coinstr,double startmilli,int32_t processed,int32_t numleft);
//int32_t init_compressionvars(int32_t readonly,struct compressionvars *V,char *coinstr,uint32_t maxblocknum);

void *Coinloop(void *ptr)
{
    int32_t i,processed;
    struct coin_info *cp;
    struct multisig_addr *msig;
    int64_t height;
    char *retstr,*msigaddr;
    double startmilli;
    while ( Finished_init == 0 || IS_LIBTEST == 7 )
        portable_sleep(1);
    printf("Coinloop numcoins.%d\n",Numcoins);
    init_Contacts();
    printf("Coinloop numcoins.%d\n",Numcoins);
    scan_address_entries();
    if ( (cp= get_coin_info("BTCD")) != 0 )
    {
        //printf("COINLOOP\n");
        //getchar();
        //if ( 0 && IS_LIBTEST > 1 && Global_mp->gatewayid >= 0 )
        //   establish_connections(cp->myipaddr,cp->srvNXTADDR,cp->srvNXTACCTSECRET);
        printf("add myhandle\n");
        addcontact(Global_mp->myhandle,cp->privateNXTADDR);
        printf("add mypublic\n");
        addcontact("mypublic",cp->srvNXTADDR);
    }
    startmilli = milliseconds();
    for (i=0; i<Numcoins; i++)
    {
        if ( (cp= Coin_daemons[i]) != 0 && is_active_coin(cp->name) >= 0 )
        {
            printf("coin.%d (%s) firstblock.%d\n",i,cp->name,(int32_t)cp->blockheight);
            if ( (retstr= invoke_MGW(MGW_whitelist,cp,0,0)) != 0 )
                free(retstr);
            //load_telepods(cp,maxnofile);
        }
    }
    MGW_initdone = 1;
    printf("MGW Initialization took %.3f seconds\n",(milliseconds() - startmilli) / 1000.);
    while ( 1 )
    {
        processed = 0;
        for (i=0; i<Numcoins; i++)
        {
            cp = Coin_daemons[i];
            if ( (cp= Coin_daemons[i]) != 0 && is_active_coin(cp->name) >= 0 )
            {
                height = get_blockheight(cp);
                cp->RTblockheight = (int32_t)height;
                if ( cp->blockheight < (height - cp->min_confirms) )
                {
                    if ( Debuglevel > 1 )
                        printf("%s: historical block.%ld when height.%ld\n",cp->name,(long)cp->blockheight,(long)height);
                    if ( update_address_infos(cp,(uint32_t)cp->blockheight) != 0 )
                    {
                        processed++;
                        cp->blockheight++;
                        if ( cp->blockheight == (height - cp->min_confirms) )
                            cp->uptodate = (uint32_t)cp->blockheight;
                    }
                }
            }
        }
        if ( processed == 0 )
        {
            if ( Debuglevel > 2 )
                printf("Coinloop: no work, sleep\n");
            if ( (msigaddr= queue_dequeue(&DepositQ,1)) != 0 )
            {
                if ( (msig= find_msigaddr(msigaddr)) != 0 )
                {
                    fprintf(stderr,"(%s) -> %s has deposits pending\n",msigaddr,msig->NXTaddr);
                    if ( (retstr= invoke_MGW(MGW_whitelist,cp,msig,1)) != 0 )
                        free(retstr);
                }
                free_queueitem(msigaddr);
            }
            else portable_sleep(10);
        }
    }
    return(0);
}

/*void *_process_coinblocks(void *_ram)
{
    struct coin_info *ram = _ram;
    uint32_t height,blockheight,processed = 0;
    blockheight = 1;
    printf("process coinblocks for (%s) from %d\n",ram->name,blockheight);
    if ( ram != 0 )
    {
        height = get_RTheight(ram);
        while ( blockheight < (height - ram->min_confirms) )
        {
            //if ( dispflag != 0 )
            //    printf("%s: historical block.%ld when height.%ld\n",cp->name,(long)blockheight,(long)height);
            if ( update_address_infos(cp,blockheight) != 0 )
            {
                processed++;
                blockheight++;
            } else break;
        }
    }
    return(0);
}*/
#endif
    
