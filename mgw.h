//
//  mgw.h
//
//  Created by jl777 2014, refactored MGW
//  Copyright (c) 2014 jl777. MIT License.
//

#ifndef mgw_h
#define mgw_h

#define DEPOSIT_XFER_DURATION 5
#define MIN_DEPOSIT_FACTOR 5

#define GET_COINDEPOSIT_ADDRESS 'g'
#define BIND_DEPOSIT_ADDRESS 'b'
#define DEPOSIT_CONFIRMED 'd'
#define MONEY_SENT 'm'

int32_t in_specialNXTaddrs(char *specialNXTaddrs[],char *NXTaddr)
{
    int32_t i;
    for (i=0; specialNXTaddrs[i]!=0; i++)
        if ( strcmp(specialNXTaddrs[i],NXTaddr) == 0 )
            return(1);
    return(0);
}

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

struct multisig_addr *find_msigaddr(char *msigaddr)
{
    int32_t createdflag;
    return(MTadd_hashtable(&createdflag,&SuperNET_dbs[MULTISIG_DATA].ramtable,msigaddr));
    //return((struct multisig_addr *)find_storage(MULTISIG_DATA,msigaddr,0));
}

int32_t update_msig_info(struct multisig_addr *msig,int32_t syncflag)
{
    DBT key,data,*datap;
    int32_t ret,createdflag;
    struct multisig_addr *msigram;
    struct SuperNET_db *sdb = &SuperNET_dbs[MULTISIG_DATA];
    if ( msig == 0 && syncflag != 0 )
        return(dbsync(sdb,0));
    if ( msig->H.size == 0 )
        msig->H.size = sizeof(*msig) + (msig->n * sizeof(msig->pubkeys[0]));
    msigram = MTadd_hashtable(&createdflag,&sdb->ramtable,msig->multisigaddr);
    if ( msigram->created != 0 && msig->created != 0 )
    {
        if ( msigram->created < msig->created )
            msig->created = msigram->created;
        else msigram->created = msig->created;
    }
    else if ( msig->created == 0 )
        msig->created = msigram->created;
    //if ( msigram->sender == 0 && msig->sender != 0 )
    //    createdflag = 1;
    if ( createdflag != 0 )//|| memcmp(msigram,msig,msig->H.size) != 0 )
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
        printf("add (%s)\n",msig->multisigaddr);
        if ( (ret= dbput(sdb,0,&key,datap,0)) != 0 )
            sdb->storage->err(sdb->storage,ret,"Database put for quote failed.");
        else if ( syncflag != 0 ) ret = dbsync(sdb,0);
    } return(1);
    return(ret);
}

struct multisig_addr *alloc_multisig_addr(char *coinstr,int32_t m,int32_t n,char *NXTaddr,char *sender)
{
    struct multisig_addr *msig;
    int32_t size = (int32_t)(sizeof(*msig) + n*sizeof(struct pubkey_info));
    msig = calloc(1,size);
    msig->H.size = size;
    msig->n = n;
    msig->created = (uint32_t)time(NULL);
    msig->sender = calc_nxt64bits(sender);
    safecopy(msig->coinstr,coinstr,sizeof(msig->coinstr));
    safecopy(msig->NXTaddr,NXTaddr,sizeof(msig->NXTaddr));
    msig->m = m;
    return(msig);
}

long calc_pubkey_jsontxt(int32_t truncated,char *jsontxt,struct pubkey_info *ptr,char *postfix)
{
    if ( truncated != 0 )
        sprintf(jsontxt,"{\"pubkey\":\"%s\",\"srv\":\"%llu\"}%s",ptr->pubkey,(long long)ptr->nxt64bits,postfix);
    else sprintf(jsontxt,"{\"address\":\"%s\",\"pubkey\":\"%s\",\"srv\":\"%llu\"}%s",ptr->coinaddr,ptr->pubkey,(long long)ptr->nxt64bits,postfix);
    return(strlen(jsontxt));
}

char *create_multisig_json(struct multisig_addr *msig,int32_t truncated)
{
    long i,len = 0;
    char jsontxt[65536],pubkeyjsontxt[65536];
    pubkeyjsontxt[0] = 0;
    for (i=0; i<msig->n; i++)
        len += calc_pubkey_jsontxt(truncated,pubkeyjsontxt+strlen(pubkeyjsontxt),&msig->pubkeys[i],(i<(msig->n - 1)) ? ", " : "");
    sprintf(jsontxt,"{%s\"sender\":\"%llu\",\"created\":%u,\"M\":%d,\"N\":%d,\"NXTaddr\":\"%s\",\"address\":\"%s\",\"redeemScript\":\"%s\",\"coin\":\"%s\",\"coinid\":\"%d\",\"pubkey\":[%s]}",truncated==0?"\"requestType\":\"MGWaddr\",":"",(long long)msig->sender,msig->created,msig->m,msig->n,msig->NXTaddr,msig->multisigaddr,msig->redeemScript,msig->coinstr,conv_coinstr(msig->coinstr),pubkeyjsontxt);
    printf("(%s) pubkeys len.%ld msigjsonlen.%ld\n",jsontxt,len,strlen(jsontxt));
    return(clonestr(jsontxt));
}

struct multisig_addr *decode_msigjson(char *NXTaddr,cJSON *obj,char *sender)
{
    int32_t j,M,n,coinid;
    char nxtstr[512],coinstr[64],ipaddr[64];
    struct multisig_addr *msig = 0;
    cJSON *pobj,*redeemobj,*pubkeysobj,*addrobj,*nxtobj,*nameobj;
    coinid = (int)get_cJSON_int(obj,"coinid");
    nameobj = cJSON_GetObjectItem(obj,"coin");
    copy_cJSON(coinstr,nameobj);
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
            msig = alloc_multisig_addr(coinstr,M,n,nxtstr,sender);
            safecopy(msig->coinstr,coinstr,sizeof(msig->coinstr));
            copy_cJSON(msig->redeemScript,redeemobj);
            copy_cJSON(msig->multisigaddr,addrobj);
            for (j=0; j<n; j++)
            {
                pobj = cJSON_GetArrayItem(pubkeysobj,j);
                if ( pobj != 0 )
                {
                    copy_cJSON(msig->pubkeys[j].coinaddr,cJSON_GetObjectItem(pobj,"address"));
                    copy_cJSON(msig->pubkeys[j].pubkey,cJSON_GetObjectItem(pobj,"pubkey"));
                    msig->pubkeys[j].nxt64bits = get_API_nxt64bits(cJSON_GetObjectItem(pobj,"srv"));
                    copy_cJSON(ipaddr,cJSON_GetObjectItem(pobj,"ipaddr"));
                    //printf("ip%d.(%s) ",j,ipaddr);
                    if ( ipaddr[0] == 0 && j < 3 )
                        strcpy(ipaddr,Server_names[j]);
                    msig->pubkeys[j].ipbits = calc_ipbits(ipaddr);
                } else { free(msig); msig = 0; }
            }
        } else { printf("%p %p %p\n",addrobj,redeemobj,pubkeysobj); free(msig); msig = 0; }
        return(msig);
    }
    //printf("decode msig:  error parsing.(%s)\n",cJSON_Print(obj));
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
    return(flag);
}

struct multisig_addr *gen_multisig_addr(char *sender,int32_t M,int32_t N,struct coin_info *cp,char *refNXTaddr,struct contact_info **contacts)
{
    int32_t i ,flag = 0;
    char acctcoinaddr[1024],pubkey[1024];
    struct contact_info *contact;
    struct multisig_addr *msig;
    if ( cp == 0 )
        return(0);
    msig = alloc_multisig_addr(cp->name,M,N,refNXTaddr,sender);
    for (i=0; i<N; i++)
    {
        flag = 0;
        if ( (contact= contacts[i]) != 0 && contact->nxt64bits != 0 )
        {
            acctcoinaddr[0] = pubkey[0] = 0;
            if ( get_NXT_coininfo(acctcoinaddr,pubkey,contact->nxt64bits,cp->name) != 0 && acctcoinaddr[0] != 0 && pubkey[0] != 0 )
            {
                strcpy(msig->pubkeys[i].coinaddr,acctcoinaddr);
                strcpy(msig->pubkeys[i].pubkey,pubkey);
                msig->pubkeys[i].nxt64bits = contact->nxt64bits;
            }
        }
    }
    flag = issue_createmultisig(cp,msig);
    if ( flag == 0 )
    {
        free(msig);
        return(0);
    }
    return(msig);
}

void broadcast_bindAM(char *refNXTaddr,struct multisig_addr *msig)
{
    struct coin_info *cp = get_coin_info("BTCD");
    char *jsontxt,*AMtxid,AM[4096];
    struct json_AM *ap = (struct json_AM *)AM;
    if ( cp != 0 && (jsontxt= create_multisig_json(msig,1)) != 0 )
    {
        printf(">>>>>>>>>>>>>>>>>>>>>>>>>> send bind address AM\n");
        set_json_AM(ap,GATEWAY_SIG,BIND_DEPOSIT_ADDRESS,refNXTaddr,0,jsontxt,1);
        AMtxid = submit_AM(0,cp->srvNXTADDR,&ap->H,0,cp->srvNXTACCTSECRET);
        if ( AMtxid != 0 )
            free(AMtxid);
        free(jsontxt);
    }
}

void add_MGWaddr(char *previpaddr,char *sender,char *origargstr)
{
    cJSON *origargjson,*argjson;
    struct multisig_addr *msig;
    char *retstr;
    if ( (origargjson= cJSON_Parse(origargstr)) != 0 )
    {
        if ( is_cJSON_Array(origargjson) != 0 )
            argjson = cJSON_GetArrayItem(origargjson,0);
        else argjson = origargjson;
        if  ( (msig= decode_msigjson(0,argjson,sender)) != 0 )
        {
            retstr = create_multisig_json(msig,1);
            printf("add_MGWaddr(%s)\n",retstr);
            broadcast_bindAM(msig->NXTaddr,msig);
            free(msig);
        }
    }
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
        printf("A ref.(%s) vs msig.(%s)\n",ref->multisigaddr,msig->multisigaddr);
        return(1);
    }
    if ( strcmp(ref->NXTaddr,msig->NXTaddr) != 0 )
    {
        printf("B ref.(%s) vs msig.(%s)\n",ref->NXTaddr,msig->NXTaddr);
        return(2);
    }
    if ( strcmp(ref->redeemScript,msig->redeemScript) != 0 )
    {
        printf("C ref.(%s) vs msig.(%s)\n",ref->redeemScript,msig->redeemScript);
        return(3);
    }
    for (i=0; i<ref->n; i++)
        if ( (x= pubkeycmp(&ref->pubkeys[i],&msig->pubkeys[i])) != 0 )
        {
            switch ( x )
            {
                case 1: printf("P.%d pubkey ref.(%s) vs msig.(%s)\n",x,ref->pubkeys[i].pubkey,msig->pubkeys[i].pubkey); break;
                case 2: printf("P.%d pubkey ref.(%s) vs msig.(%s)\n",x,ref->pubkeys[i].coinaddr,msig->pubkeys[i].coinaddr); break;
                case 3: printf("P.%d pubkey ref.(%llu) vs msig.(%llu)\n",x,(long long)ref->pubkeys[i].nxt64bits,(long long)msig->pubkeys[i].nxt64bits); break;
                default: printf("unexpected retval.%d\n",x);
            }
            return(4+i);
        }
    return(0);
}

char *genmultisig(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *coinstr,char *refacct,int32_t M,int32_t N,struct contact_info **contacts,int32_t n)
{
    struct coin_info *cp = get_coin_info(coinstr);
    struct multisig_addr *msig;//,*dbmsig;
    struct contact_info *contact,*refcontact = 0;
    char refNXTaddr[64],hopNXTaddr[64],destNXTaddr[64],mypubkey[1024],myacctcoinaddr[1024],pubkey[1024],acctcoinaddr[1024],buf[1024],*retstr = 0;
    int32_t i,iter,flag,valid = 0;
    printf("GENMULTISIG from (%s)\n",previpaddr);
    refNXTaddr[0] = 0;
    if ( (refcontact= find_contact(refacct)) != 0 )
    {
        if ( refcontact->nxt64bits != 0 )
            expand_nxt64bits(refNXTaddr,refcontact->nxt64bits);
    }
    printf("GENMULTISIG.(%s) n.%d\n",refNXTaddr,n);
    if ( refNXTaddr[0] == 0 )
        return(clonestr("\"error\":\"genmultisig couldnt find refcontact\"}"));
    flag = 0;
    for (iter=0; iter<2; iter++)
    for (i=0; i<n; i++)
    {
        if ( (contact= contacts[i]) != 0 && contact->nxt64bits != 0 )
        {
            if ( iter == 0 && ismynxtbits(contact->nxt64bits) != 0 )
            {
                myacctcoinaddr[0] = mypubkey[0] = 0;
                printf("Is me.%llu\n",(long long)contact->nxt64bits);
                if ( cp != 0 && get_acct_coinaddr(myacctcoinaddr,cp,refNXTaddr) != 0 && get_bitcoind_pubkey(mypubkey,cp,myacctcoinaddr) != 0 && myacctcoinaddr[0] != 0 && mypubkey[0] != 0 )
                {
                    flag++;
                    add_NXT_coininfo(contact->nxt64bits,cp->name,myacctcoinaddr,mypubkey);
                    valid++;
                }
                else printf("error getting msigaddr for cp.%p ref.(%s) addr.(%s) pubkey.(%s)\n",cp,refNXTaddr,acctcoinaddr,pubkey);
            }
            else if ( iter == 1 && ismynxtbits(contact->nxt64bits) == 0 )
            {
                acctcoinaddr[0] = pubkey[0] = 0;
                if ( get_NXT_coininfo(acctcoinaddr,pubkey,contact->nxt64bits,cp->name) == 0 || acctcoinaddr[0] == 0 || pubkey[0] == 0 )
                {
                    expand_nxt64bits(destNXTaddr,contact->nxt64bits);
                    hopNXTaddr[0] = 0;
                    if ( myacctcoinaddr[0] != 0 && mypubkey[0] != 0 )
                        sprintf(buf,"{\"requestType\":\"getmsigpubkey\",\"NXT\":\"%s\",\"myaddr\":\"%s\",\"mypubkey\":\"%s\",\"coin\":\"%s\",\"refNXTaddr\":\"%s\"}",NXTaddr,myacctcoinaddr,mypubkey,coinstr,refNXTaddr);
                    else sprintf(buf,"{\"requestType\":\"getmsigpubkey\",\"NXT\":\"%s\",\"coin\":\"%s\",\"refNXTaddr\":\"%s\"}",NXTaddr,coinstr,refNXTaddr);
                    printf("SENDREQ.(%s)\n",buf);
                    retstr = send_tokenized_cmd(hopNXTaddr,0,NXTaddr,NXTACCTSECRET,buf,destNXTaddr);
                } else valid++;
                printf("check with get_NXT_coininfo i.%d valid.%d\n",i,valid);
            }
        }
    }
    if ( valid == N )
    {
        if ( (msig= gen_multisig_addr(NXTaddr,M,N,cp,refNXTaddr,contacts)) != 0 )
        {
            retstr = create_multisig_json(msig,0);
            update_msig_info(msig,1);
            /*if ( (dbmsig= find_msigaddr(msig->multisigaddr)) == 0 )
            {
                update_msig_info(msig,1);
                free(msig);
            }
            else
            {
                if ( msigcmp(dbmsig,msig) == 0 )
                    free(msig), msig = 0;
            }*/
            printf("retstr.(%s)\n",retstr);
            if ( retstr != 0 && previpaddr != 0 && previpaddr[0] != 0 )
                send_to_ipaddr(1,previpaddr,retstr,NXTACCTSECRET);
            /*if ( msig != 0 )
            {
                if ( 0 && flag != 0 )
                    broadcast_bindAM(refNXTaddr,msig);
                free(msig);
            }*/
        }
    }
    if ( valid != N || retstr == 0 )
    {
        sprintf(buf,"{\"error\":\"missing msig info\",\"refacct\":\"%s\",\"coin\":\"%s\",\"M\":%d,\"N\":%d,\"valid\":%d}",refacct,coinstr,M,N,valid);
        retstr = clonestr(buf);
    }
    return(retstr);
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
//end of network funcs

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

void _update_redeembits(char *coinstr,uint64_t redeembits,uint64_t AMtxidbits)
{
    struct coin_info *cp;
    struct NXT_asset *ap;
    int32_t createdflag;
    int32_t i;
    if ( (cp= get_coin_info(coinstr)) != 0 )
    {
        ap = get_NXTasset(&createdflag,Global_mp,cp->assetid);
        if ( ap->num > 0 )
        {
            for (i=0; i<ap->num; i++)
                if ( ap->txids[i]->redeemtxid == redeembits )
                    ap->txids[i]->AMtxidbits = AMtxidbits;
        }
    }

}

void update_redeembits(cJSON *argjson,uint64_t AMtxidbits)
{
    cJSON *array;
    int32_t i,n;
    char coinstr[1024],redeemtxid[1024];
    if ( extract_cJSON_str(coinstr,sizeof(coinstr),argjson,"coin") <= 0 )
        return;
    array = cJSON_GetObjectItem(argjson,"redeems");
    if ( array != 0 && is_cJSON_Array(array) != 0 )
    {
        n = cJSON_GetArraySize(array);
        for (i=0; i<n; i++)
        {
            copy_cJSON(redeemtxid,cJSON_GetArrayItem(array,i));
            if ( redeemtxid[0] != 0 && is_limbo_redeem(coinstr,redeemtxid) == 0 )
                _update_redeembits(coinstr,calc_nxt64bits(redeemtxid),AMtxidbits);
        }
    }
    if ( extract_cJSON_str(redeemtxid,sizeof(redeemtxid),argjson,"redeemtxid") > 0 && is_limbo_redeem(coinstr,redeemtxid) == 0 )
        _update_redeembits(coinstr,calc_nxt64bits(redeemtxid),AMtxidbits);
}

void process_MGW_message(char *specialNXTaddrs[],struct json_AM *ap,char *sender,char *receiver,char *txid,int32_t syncflag,char *coinstr)
{
    char NXTaddr[64];
    cJSON *argjson;
    struct multisig_addr *msig;
    expand_nxt64bits(NXTaddr,ap->H.nxt64bits);
    if ( (argjson = parse_json_AM(ap)) != 0 )
    {
        printf("func.(%c) %s -> %s txid.(%s) JSON.(%s)\n",ap->funcid,sender,receiver,txid,ap->U.jsonstr);
        switch ( ap->funcid )
        {
            case GET_COINDEPOSIT_ADDRESS:
                // start address gen
                printf("GENADDRESS: func.(%c) %s -> %s txid.(%s) JSON.(%s)\n",ap->funcid,sender,receiver,txid,ap->U.jsonstr);
                //update_coinacct_addresses(ap->H.nxt64bits,argjson,txid,-1);
                break;
            case BIND_DEPOSIT_ADDRESS:
                if ( (msig= decode_msigjson(0,argjson,sender)) != 0 )
                {
                    //printf("%s func.(%c) %s -> %s txid.(%s) JSON.(%s)\n",msig->coinstr,ap->funcid,sender,receiver,txid,ap->U.jsonstr);
                    if ( strcmp(msig->coinstr,coinstr) == 0 )
                    {
                        if ( update_msig_info(msig,syncflag) == 0 )
                            printf("%s func.(%c) %s -> %s txid.(%s) JSON.(%s)\n",msig->coinstr,ap->funcid,sender,receiver,txid,ap->U.jsonstr);
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
                    update_redeembits(argjson,calc_nxt64bits(txid));
                break;
            default: printf("funcid.(%c) not handled\n",ap->funcid);
        }
        if ( argjson != 0 )
            free_json(argjson);
    } else printf("can't JSON parse (%s)\n",ap->U.jsonstr);
}

uint64_t process_NXTtransaction(char *specialNXTaddrs[],char *sender,char *receiver,cJSON *item,char *refNXTaddr,char *assetid,int32_t syncflag,struct coin_info *cp)
{
    int32_t conv_coinstr(char *);
    char AMstr[4096],txid[4096],comment[4096],*assetidstr,*commentstr = 0;
    cJSON *senderobj,*attachment,*message,*assetjson,*commentobj,*cointxidobj;
    char cointxid[128];
    unsigned char buf[4096];
    struct NXT_AMhdr *hdr;
    struct NXT_asset *ap = 0;
    struct NXT_assettxid *tp;
    uint64_t retbits = 0;
    int32_t height,timestamp=0,coinid;
    int64_t type,subtype,n,assetoshis = 0;
    assetid[0] = 0;
    if ( item != 0 )
    {
        hdr = 0; assetidstr = 0;
        sender[0] = receiver[0] = 0;
        copy_cJSON(txid,cJSON_GetObjectItem(item,"transaction"));
        type = get_cJSON_int(item,"type");
        subtype = get_cJSON_int(item,"subtype");
        //if ( strcmp(txid,"9366367254950472318") == 0 )
        //    fprintf(stderr,"AMAMAMAM start type.%d subtype.%d txid.(%s)\n",(int)type,(int)subtype,txid);
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
                    hdr = (struct NXT_AMhdr *)buf;
                    process_MGW_message(specialNXTaddrs,(void *)hdr,sender,receiver,txid,syncflag,cp->name);
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
                                            printf("got.(%s) comment.(%s) cointxidstr.(%s)\n",txid,tp->comment,cointxid);
                                            if ( cointxid[0] != 0 )
                                                tp->cointxid = clonestr(cointxid);
                                        } else cointxid[0] = 0;
                                        free_json(commentobj);
                                    }
                                    if ( coinid >= 0 && is_limbo_redeem(ap->name,txid) == 0 )
                                    {
                                        if ( strcmp(receiver,refNXTaddr) == 0 )
                                        {
                                            if ( Debuglevel > 1 )
                                                printf("%s txid.(%s) got comment.(%s) gotpossibleredeem.(%s) coinid.%d %.8f\n",ap->name,txid,tp->comment,cointxid,coinid,dstr(tp->quantity * ap->mult));
                                            tp->redeemtxid = calc_nxt64bits(txid);
                                        }
                                    }
                                }
                                break;
                            case 2:
                            case 3: // bids and asks, no indication they are filled at this point, so nothing to do
                                break;
                        }
                    }
                    tp->U.assetoshis = assetoshis;
                    add_pendingxfer(1,tp->txidbits);
                    retbits = tp->txidbits;
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
    uint32_t timestamp;
    struct NXT_acct *np;
    cJSON *item,*json,*array;
    if ( refNXTaddr == 0 )
    {
        printf("illegal refNXT.(%s)\n",refNXTaddr);
        return(0);
    }
    sprintf(cmd,"%s=getAccountTransactions&account=%s&type=%d",_NXTSERVER,refNXTaddr,txtype);
    coinid = conv_coinstr(cp->name);
    np = get_NXTacct(&createdflag,Global_mp,refNXTaddr);
    if ( coinid > 0 && np->timestamps[coinid] != 0 && coinid < 64 )
        sprintf(cmd + strlen(cmd),"&timestamp=%d",cp->timestamps[coinid]);
    if ( Debuglevel > 2 )
        printf("update_NXT_transactions.(%s) for (%s) cmd.(%s)\n",refNXTaddr,cp->name,cmd);
    if ( (jsonstr= issue_NXTPOST(0,cmd)) != 0 )
    {
        //printf("(%s)\n",jsonstr);
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"transactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    if ( Debuglevel > 2 )
                        fprintf(stderr,"%d/%d ",i,n);
                    item = cJSON_GetArrayItem(array,i);
                    process_NXTtransaction(specialNXTaddrs,sender,receiver,item,refNXTaddr,assetid,0,cp);
                    timestamp = (int32_t)get_cJSON_int(item,"blockTimestamp") - 3600;
                    if ( timestamp > np->timestamps[coinid] )
                    {
                        printf("new timestamp.%d %d -> %d\n",coinid,np->timestamps[coinid],timestamp);
                        np->timestamps[coinid] = timestamp;
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

uint64_t process_msigdeposits(cJSON **transferjsonp,int32_t forceflag,struct coin_info *cp,struct address_entry *entry,uint64_t nxt64bits,struct NXT_asset *ap,char *msigaddr)
{
    char txidstr[1024],coinaddr[1024],script[4096],coinaddr_v0[1024],script_v0[4096],comment[4096],NXTaddr[64],numstr[64];
    struct NXT_assettxid *tp;
    uint64_t depositid,value,total = 0;
    int32_t j,numvouts;
    cJSON *pair;
    for (j=0; j<ap->num; j++)
    {
        tp = ap->txids[j];
        //printf("%d of %d: process.(%s) isinternal.%d %llu (%llu -> %llu)\n",j,ap->num,msigaddr,entry->isinternal,(long long)nxt64bits,(long long)tp->senderbits,(long long)tp->receiverbits);
        if ( tp->receiverbits == nxt64bits && tp->coinblocknum == entry->blocknum && tp->cointxind == entry->txind && tp->coinv == entry->v )
            break;
    }
    if ( j == ap->num )
    {
        value = get_txoutstr(&numvouts,txidstr,coinaddr,script,cp,entry->blocknum,entry->txind,entry->v);
        if ( strcmp("31dcbc5b7cfd7fc8f2c1cedf65f38ec166b657cc9eb15e7d1292986eada35ea9",txidstr) == 0 ) // due to uncommented tx
            return(0);
        if ( entry->v == numvouts-1 )
        {
            get_txoutstr(0,txidstr,coinaddr_v0,script_v0,cp,entry->blocknum,entry->txind,0);
            if ( strcmp(coinaddr_v0,cp->marker) == 0 )
                return(0);
        }
        if ( strcmp(msigaddr,coinaddr) == 0 && txidstr[0] != 0 && value >= (cp->NXTfee_equiv * MIN_DEPOSIT_FACTOR) )
        {
            for (j=0; j<ap->num; j++)
            {
                tp = ap->txids[j];
                if ( tp->receiverbits == nxt64bits && tp->cointxid != 0 && strcmp(tp->cointxid,txidstr) == 0 )
                {
                    if ( Debuglevel > 0 )
                        printf("%llu set cointxid.(%s) <-> (%u %d %d)\n",(long long)nxt64bits,txidstr,entry->blocknum,entry->txind,entry->v);
                    tp->cointxind = entry->txind;
                    tp->coinv = entry->v;
                    tp->coinblocknum = entry->blocknum;
                    break;
                }
            }
            if ( j == ap->num )
            {
                //printf("UNPAID cointxid.(%s) <-> (%u %d %d)\n",txidstr,entry->blocknum,entry->txind,entry->v);
                sprintf(comment,"{\"coinaddr\":\"%s\",\"cointxid\":\"%s\",\"coinblocknum\":%u,\"cointxind\":%u,\"coinv\":%u}",coinaddr,txidstr,entry->blocknum,entry->txind,entry->v);
                printf(">>>>>>>>>>>>>> Need to transfer %.8f %ld assetoshis | %s to %llu for (%s) %s\n",dstr(value),(long)(value/ap->mult),cp->name,(long long)nxt64bits,txidstr,comment);
                total += value;
                if ( forceflag > 0 )
                {
                    expand_nxt64bits(NXTaddr,nxt64bits);
                    depositid = issue_transferAsset(0,0,cp->srvNXTACCTSECRET,NXTaddr,cp->assetid,value/ap->mult,MIN_NQTFEE,DEPOSIT_XFER_DURATION,comment);
                    add_pendingxfer(0,depositid);
                    if ( transferjsonp != 0 )
                    {
                        if ( *transferjsonp == 0 )
                            *transferjsonp = cJSON_CreateArray();
                        pair = cJSON_Parse(comment);
                        sprintf(numstr,"\"%llu\"",(long long)depositid);
                        cJSON_AddItemToObject(pair,"depositid",cJSON_CreateString(numstr));
                        cJSON_AddItemToArray(*transferjsonp,pair);
                    }
                }
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
    sprintf(buf,"%s_%s_%d",txidstr,cp->name,v);
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
        if ( Debuglevel > 1 )
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

uint64_t process_msigaddr(int32_t *numunspentp,uint64_t *unspentp,cJSON **transferjsonp,int32_t forceflag,struct NXT_asset *ap,char *NXTaddr,struct coin_info *cp,char *msigaddr)
{
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
    if ( (entries= get_address_entries(&n,cp->name,msigaddr)) != 0 )
    {
        if ( Debuglevel > 2 )
            printf(">>>>>>>>>>>>>>>> %d address entries for (%s)\n",n,msigaddr);
        for (i=0; i<n; i++)
        {
            entry = &entries[i];
            if ( entry->vinflag == 0 )
                pendingdeposits += process_msigdeposits(transferjsonp,forceflag,cp,entry,nxt64bits,ap,msigaddr);
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
                /*if ( (unspent= check_txout(&createtime,cp,cp->minconfirms,0,cointp->txid,cointp->entry.v,0)) == 0 )
                    cointp->entry.spent = 1;
                else if ( unspent != cointp->value )
                    printf("ERROR: %.8f != %.8f | %s %s.%d\n",dstr(unspent),dstr(cointp->value),cp->name,cointp->txid,cointp->entry.v);
                else*/
                {
                    cointp->unspent = unspent;
                    (*numunspentp)++;
                    (*unspentp) += unspent;
                    printf("%s | %16.8f unspenttotal %.8f\n",cointp->txid,dstr(cointp->unspent),dstr((*unspentp)));
                    update_unspent_funds(cp,cointp,0);
                }
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

int32_t init_specialNXTaddrs(char *specialNXTaddrs[],char *ipaddrs[],char *specialNXT,char *NXT0,char *NXT1,char *NXT2,char *ip0,char *ip1,char *ip2,char *exclude0,char *exclude1)
{
    int32_t i,numgateways = 3;
    specialNXTaddrs[0] = NXT0, specialNXTaddrs[1] = NXT1, specialNXTaddrs[2] = NXT2;
    ipaddrs[0] = ip0, ipaddrs[1] = ip1, ipaddrs[2] = ip2;
    for (i=0; i<numgateways; i++)
    {
        if ( specialNXTaddrs[i] == 0 )
            specialNXTaddrs[i] = "";
        if ( ipaddrs[i] == 0 )
            strcpy(ipaddrs[i],Server_names[i]);
    }
    specialNXTaddrs[numgateways++] = GENESISACCT;
    specialNXTaddrs[numgateways++] = specialNXT;
    if ( exclude0 != 0 && exclude0[0] != 0 )
        specialNXTaddrs[numgateways++] = exclude0;
    if ( exclude1 != 0 && exclude1[0] != 0 )
        specialNXTaddrs[numgateways++] = exclude1;
    specialNXTaddrs[numgateways+1] = 0;
    return(numgateways);
}

uint64_t update_NXTblockchain_info(struct coin_info *cp,char *specialNXTaddrs[],int32_t numgateways,char *refNXTaddr)
{
    struct coin_info *btcdcp;
    uint64_t pendingtxid;
    int32_t i;
    ready_to_xferassets(&pendingtxid);
    if ( (btcdcp= get_coin_info("BTCD")) != 0 )
    {
        update_NXT_transactions(specialNXTaddrs,1,btcdcp->srvNXTADDR,cp);
        update_NXT_transactions(specialNXTaddrs,1,btcdcp->privateNXTADDR,cp);
    }
    update_NXT_transactions(specialNXTaddrs,1,refNXTaddr,cp);
    update_NXT_transactions(specialNXTaddrs,2,refNXTaddr,cp);
        for (i=0; i<numgateways; i++)
        update_NXT_transactions(specialNXTaddrs,1,specialNXTaddrs[i],cp); // first numgateways of specialNXTaddrs[] are gateways
    update_msig_info(0,1); // sync MULTISIG_DATA
    return(pendingtxid);
}

char *wait_for_pendingtxid(struct coin_info *cp,char *specialNXTaddrs[],char *refNXTaddr,uint64_t pendingtxid)
{
    char txidstr[64],sender[64],receiver[64],assetstr[64],retbuf[1024],*retstr;
    cJSON *json;
    uint64_t val;
    expand_nxt64bits(txidstr,pendingtxid);
    sprintf(retbuf,"{\"result\",\"pendingtxid\",\"waitingfor\":\"%llu\"}",(long long)pendingtxid);
    if ( (retstr= issue_getTransaction(0,txidstr)) != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            if ( (val= process_NXTtransaction(specialNXTaddrs,sender,receiver,json,refNXTaddr,assetstr,1,cp)) != 0 )
                sprintf(retbuf,"{\"result\",\"pendingtxid\",\"processed\":\"%llu\"}",(long long)val);
            free_json(json);
        }
        free(retstr);
    }
    return(clonestr(retbuf));
}

char *process_deposits(uint64_t *unspentp,struct multisig_addr **msigs,int32_t nummsigs,struct coin_info *cp,char *ipaddrs[],char *specialNXTaddrs[],int32_t numgateways,char *refNXTaddr,struct NXT_asset *ap,int32_t transferassets,uint64_t circulation)
{
    uint64_t pendingtxid,unspent = 0,total = 0;
    int32_t i,m,max,readyflag,tmp,nonz,numunspent;
    struct multisig_addr *msig;
    struct address_entry *entries;
    struct unspent_info *up;
    cJSON *transferjson = 0;
    char *transferstr=0,*retstr = 0;
    *unspentp = 0;
    if ( msigs != 0 )
    {
        readyflag = ready_to_xferassets(&pendingtxid);
        printf("readyflag.%d\n",readyflag);
        for (i=max=0; i<nummsigs; i++)
        {
            if ( (msig= (struct multisig_addr *)msigs[i]) != 0 && (entries= get_address_entries(&m,cp->name,msig->multisigaddr)) != 0 )
            {
                max += m;
                free(entries);
            }
        }
        if ( Debuglevel > 1 )
            printf("got n.%d msigs readyflag.%d | max.%d pendingtxid.%llu\n",nummsigs,readyflag,max,(long long)pendingtxid);
        unspent = nonz = numunspent = 0;
        up = &cp->unspent;
        update_unspent_funds(cp,0,max);
        for (i=0; i<nummsigs; i++)
        {
            if ( (msig= (struct multisig_addr *)msigs[i]) != 0 )
            {
                if ( max > 0 && valid_msig(msig,cp->name,refNXTaddr,specialNXTaddrs,ipaddrs,2,3) != 0 )
                {
                    if ( Debuglevel > 2 )
                        printf("MULTISIG: %s: %d of %d %s %s\n",cp->name,i,nummsigs,msig->coinstr,msig->multisigaddr);
                    update_NXT_transactions(specialNXTaddrs,2,msig->NXTaddr,cp);
                    if ( readyflag > 0 && pendingtxid == 0 )
                    {
                        tmp = numunspent;
                        total += process_msigaddr(&numunspent,&unspent,&transferjson,transferassets,ap,msig->NXTaddr,cp,msig->multisigaddr);
                        if ( numunspent > tmp )
                            nonz++;
                    }
                }
            }
        }
        if ( up->num > 1 )
            sort_vps(up->vps,up->num);
        printf("max %.8f min %.8f median %.8f |unspent %.8f numunspent.%d in nonz.%d accts\n",dstr(up->maxavail),dstr(up->minavail),dstr((up->maxavail+up->minavail)/2),dstr(up->unspent),numunspent,nonz);
        if ( transferjson != 0 )
        {
            transferstr = cJSON_Print(transferjson);
            free_json(transferjson);
            stripwhite_ns(transferstr,strlen(transferstr));
            retstr = malloc(strlen(transferstr) + 4096);
        } else retstr = malloc(4096);
        sprintf(retstr,"{\"circulation\":\"%.8f\",\"unspent\":\"%.8f\",\"pendingdeposits\":\"%.8f\",\"transfers\":%s}",dstr(circulation),dstr(unspent),dstr(total),transferstr!=0?transferstr:"[]");
        if ( transferstr != 0 )
            free(transferstr);
    }
    *unspentp = unspent;
    return(retstr);
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
        //printf("circ.(%s) <- (%s)\n",retstr,cmd);
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"accountAssets")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    item = cJSON_GetArrayItem(array,i);
                    copy_cJSON(acct,cJSON_GetObjectItem(item,"account"));
                    //printf("%s ",acct);
                    if ( acct[0] != 0 && in_specialNXTaddrs(specialNXTaddrs,acct) == 0 )
                    {
                        if ( (quantity= get_API_nxt64bits(cJSON_GetObjectItem(item,"quantityQNT"))) != 0 )
                            circulation += quantity;
                        //printf("%llu, ",(long long)quantity);
                    }
                }
            }
        }
        free(retstr);
    }
    return(circulation * ap->mult);
}

char *calc_withdraw_addr(char *destaddr,char *NXTaddr,struct coin_info *cp,struct NXT_assettxid *tp,struct NXT_asset *ap)
{
    char pubkey[1024],withdrawaddr[1024],*retstr = destaddr;
    int64_t amount,minwithdraw;
    cJSON *obj,*argjson = 0;
    destaddr[0] = withdrawaddr[0] = 0;
    if ( tp->comment != 0 && tp->comment[0] != 0 && (argjson= cJSON_Parse(tp->comment)) != 0 )
    {
        obj = cJSON_GetObjectItem(argjson,"withdrawaddr");
        copy_cJSON(withdrawaddr,obj);
    }
    amount = tp->quantity * ap->mult;
    minwithdraw = cp->txfee * MIN_DEPOSIT_FACTOR;
    if ( amount <= minwithdraw )
    {
        printf("minimum withdrawal must be more than %.8f %s\n",dstr(minwithdraw),cp->name);
        retstr = 0;
    }
    else if ( tp->redeemtxid == 0 )
    {
        printf("no redeem txid %s %s\n",cp->name,cJSON_Print(argjson));
        retstr = 0;
    }
    if ( withdrawaddr[0] == 0 )
    {
        printf("no withdraw address for %.8f | ",dstr(amount));
        retstr = 0;
    }
    //printf("withdraw addr.(%s) lp.%p\n",withdrawaddr,lp);
    if ( cp != 0 && validate_coinaddr(pubkey,cp,withdrawaddr) < 0 )
    {
        printf("invalid address.(%s) for NXT.%s %.8f validate.%d\n",withdrawaddr,NXTaddr,dstr(amount),validate_coinaddr(pubkey,cp,withdrawaddr));
        retstr = 0;
    }
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
    if ( destaddr == 0 || destaddr[0] == 0 )
    {
        printf("add_redeem with null destaddr.%p\n",destaddr);
        return(numredeems);
    }
    amount = tp->quantity * ap->mult - (cp->txfee + cp->NXTfee_equiv);
    (*pending_withdrawp) += amount;
    if ( numredeems > 0 )
    {
        for (j=0; j<numredeems; j++)
            if ( redeems[j] == tp->txidbits )
                break;
    } else j = 0;
    if ( j == numredeems )
    {
        destaddrs[numredeems] = clonestr(destaddr);
        destamounts[numredeems] = amount;
        redeems[numredeems] = tp->txidbits;
        numredeems++;
        printf("withdraw_addr.%d R.(%llu %.8f %s)\n",j,(long long)tp->txidbits,dstr(destamounts[j]),destaddrs[j]);
    }
    else printf("ERROR: duplicate redeembits.%llu numredeems.%d j.%d\n",(long long)tp->txidbits,numredeems,j);
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

int32_t process_destaddr(char *destaddrs[MAX_MULTISIG_OUTPUTS],uint64_t destamounts[MAX_MULTISIG_OUTPUTS],uint64_t redeems[MAX_MULTISIG_OUTPUTS],uint64_t *pending_withdrawp,struct coin_info *cp,uint64_t nxt64bits,struct NXT_asset *ap,char *destaddr,struct NXT_assettxid *tp,int32_t numredeems)
{
    struct address_entry *entries,*entry;
    struct coin_txidind *cointp;
    struct unspent_info *up;
    int32_t j,n,createdflag;
    //char *rawtx;
    if ( (entries= get_address_entries(&n,cp->name,destaddr)) != 0 )
    {
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
        {
            /*for (j=0; j<n; j++)
            {
                entry = &entries[j];
                if ( entry->vinflag == 0 )
                {
                    cointp = get_cointp(cp,entry);
                    if ( cointp->seq0 == 0 )
                    {
                        cointp->redeemtxid = -1;
                        printf("check txid.(%s)\n",cointp->txid);
                        if ( (rawtx= get_rawtransaction(cp,cointp->txid)) != 0 )
                        {
                            cointp->seq0 = extract_sequenceid(&cointp->numinputs,cp,rawtx,0);
                            cointp->seq1 = extract_sequenceid(&cointp->numinputs,cp,rawtx,1);
                            cointp->redeemtxid = ((uint64_t)cointp->seq1 << 32) | cointp->seq0;
                            printf("%x %x -> redeem.%llu\n",cointp->seq0,cointp->seq1,(long long)cointp->redeemtxid);
                            free(rawtx);
                        }
                    }
                    if ( cointp->redeemtxid == tp->redeemtxid )
                        break;
                }
            }
            if ( j == n )*/
            {
                up = &cp->unspent;
                printf("numredeems.%d (%p %p) PENDING REDEEM %s %s %llu %llu %.8f %.8f | %llu\n",numredeems,up->maxvp,up->minvp,cp->name,destaddr,(long long)nxt64bits,(long long)tp->redeemtxid,dstr(tp->quantity),dstr(tp->U.assetoshis),(long long)tp->AMtxidbits);
                numredeems = add_redeem(destaddrs,destamounts,redeems,pending_withdrawp,cp,ap,destaddr,tp,numredeems);
                printf("%p numredeems.%d (%s) %.8f %llu\n",&destaddrs[numredeems-1],numredeems,destaddrs[numredeems-1],dstr(destamounts[numredeems-1]),(long long)redeems[numredeems-1]);
            }
        }
        free(entries);
    }
    return(numredeems);
}

char *create_batch_jsontxt(struct coin_info *cp,int *firstitemp)
{
    struct rawtransaction *rp = &cp->BATCH.rawtx;
    cJSON *json,*obj,*array = 0;
    char *jsontxt,redeemtxid[128];
    int32_t i,ind;
    json = cJSON_CreateObject();
    obj = cJSON_CreateNumber(cp->coinid); cJSON_AddItemToObject(json,"coinid",obj);
    obj = cJSON_CreateNumber(issue_getTime(0)); cJSON_AddItemToObject(json,"timestamp",obj);
    obj = cJSON_CreateString(coinid_str(cp->coinid)); cJSON_AddItemToObject(json,"coin",obj);
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

/*struct withdraw_info *parse_batch_json(struct withdraw_info *W,cJSON *argjson)
{
    uint64_t tmp;
    struct coin_info *cp;
    int32_t i,n,createdflag,timestamp,coinid;
    char buf[512],assetidstr[64],NXTtxidstr[64],cointxid[MAX_COINTXID_LEN],redeemtxid[64];
    cJSON *array,*item;
    struct coin_txidind *cointp;
    struct withdraw_info *wp = 0;
    if ( argjson != 0 )
    {
        coinid = (int32_t)get_cJSON_int(argjson,"coinid");
        if ( extract_cJSON_str(buf,sizeof(buf),argjson,"coin") <= 0 ) return(0);
        printf("got coindid.%d and %s\n",coinid,buf);
        if ( strcmp(buf,coinid_str(coinid)) != 0 )
            return(0);
        if ( (cp= get_coin_info(buf)) == 0 )
            return(0);
        if ( extract_cJSON_str(cointxid,sizeof(cointxid),argjson,"cointxid") <= 0 ) return(0);
        timestamp = (int32_t)get_cJSON_int(argjson,"timestamp");
        printf("timestamp.%d (%s)\n",timestamp,cointxid);
        strcpy(assetidstr,cp->assetid);
        if ( strcmp(assetidstr,ILLEGAL_COINASSET) != 0 )
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
                        add_pendingxfer(0,calc_nxt64bits(redeemtxid));
                        cointp = conv_txidstr(cp,cointxid,0);

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
                            wp->amount *= get_asset_mult(calc_nxt64bits(assetidstr));
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

void update_money_sent(cJSON *argjson,char *AMtxid,int32_t height)
{
    struct withdraw_info W,*wp;
    if ( argjson != 0 && AMtxid != 0 )
    {
        memset(&W,0,sizeof(W));
        //if ( height < NXT_FORKHEIGHT )
        //    wp = parse_moneysent_json(&W,argjson);
        //else
            wp = parse_batch_json(&W,argjson);
        if ( wp != 0 )
        {
            wp->AMtxidbits = calc_nxt64bits(AMtxid);
            fprintf(stderr,">>>>>>> money sent AM txid.%llu\n",(long long)wp->AMtxidbits);
        } //else printf("error parsing moneysent json.(%s)\n",cJSON_Print(argjson));
    } else fprintf(stderr,"error updating money sent (%p %p)\n",argjson,AMtxid);
}*/

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
                    update_redeembits(argjson,calc_nxt64bits(AMtxid)); //update_money_sent(argjson,AMtxid,height);
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
    char *retstr;
    fprintf(stderr,"achieved consensus and sign! %s\n",cp->BATCH.rawtx.batchsigned);
    if ( (retstr= submit_withdraw(cp,&cp->BATCH,&cp->withdrawinfos[(Global_mp->gatewayid + 1) % NUM_GATEWAYS])) != 0 )
    {
        safecopy(cp->BATCH.W.cointxid,retstr,sizeof(cp->BATCH.W.cointxid));
        broadcast_moneysentAM(cp,height);
        free(retstr);
        //backupwallet(cp,cp->coinid);
        return(1);
    }
    else printf("sign_and_sendmoney: error sending rawtransaction %s\n",cp->BATCH.rawtx.batchsigned);
    return(MGW_PENDING_WITHDRAW);
}

char *process_withdraws(struct multisig_addr **msigs,int32_t nummsigs,uint64_t unspent,struct coin_info *cp,struct NXT_asset *ap,char *specialNXT,int32_t sendmoney,uint64_t circulation)
{
    struct NXT_assettxid *tp;
    struct rawtransaction *rp;
    struct batch_info *otherwp;
    int32_t i,j,numredeems,gatewayid;
    uint64_t destamounts[MAX_MULTISIG_OUTPUTS],redeems[MAX_MULTISIG_OUTPUTS],nxt64bits,sum,pending_withdraw = 0;
    char withdrawaddr[64],sender[64],redeemtxid[64],*destaddrs[MAX_MULTISIG_OUTPUTS],*destaddr="",*batchsigned,*str,*retstr = 0;
    if ( ap->num <= 0 )
        return(0);
    rp = &cp->BATCH.rawtx;
    clear_BATCH(rp);
    rp->numoutputs = init_batchoutputs(cp,rp,cp->txfee);
    numredeems = 0;
    memset(redeems,0,sizeof(redeems));
    memset(destaddrs,0,sizeof(destaddrs));
    memset(destamounts,0,sizeof(destamounts));
    nxt64bits = calc_nxt64bits(specialNXT);
    for (i=0; i<ap->num; i++)
    {
        tp = ap->txids[i];
        if ( tp->redeemtxid != 0 && tp->receiverbits == nxt64bits && tp->assetbits == ap->assetbits )
        {
            str = (tp->AMtxidbits != 0) ? ": REDEEMED" : " <- redeem";
            expand_nxt64bits(sender,tp->senderbits);
            if ( (destaddr= calc_withdraw_addr(withdrawaddr,sender,cp,tp,ap)) != 0 && destaddr[0] != 0 && tp->AMtxidbits == 0 )
            {
                stripwhite(destaddr,strlen(destaddr));
                numredeems = process_destaddr(destaddrs,destamounts,redeems,&pending_withdraw,cp,nxt64bits,ap,destaddr,tp,numredeems);
            }
            if ( Debuglevel > 2 )
                printf("%s %s %llu %s %llu %.8f %.8f | %llu\n",cp->name,destaddr,(long long)nxt64bits,str,(long long)tp->redeemtxid,dstr(tp->quantity),dstr(tp->U.assetoshis),(long long)tp->AMtxidbits);
        }
    }
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
            free(batchsigned);
        }
    }
    else
    {
        if ( numredeems > 0 )
            printf("%.8f is not enough to pay for MGWfees.%s %.8f for %d redeems\n",dstr(pending_withdraw),cp->name,dstr(cp->NXTfee_equiv),numredeems);
        pending_withdraw = 0;
    }
    return(retstr);
}

char *MGWdeposits(char *specialNXT,int32_t rescan,int32_t actionflag,char *coin,char *assetstr,char *NXT0,char *NXT1,char *NXT2,char *ip0,char *ip1,char *ip2,char *exclude0,char *exclude1)
{
    static int32_t firsttimestamp;
    char retbuf[4096],*specialNXTaddrs[257],*ipaddrs[3],*retstr,*retstr2;
    struct coin_info *cp;
    uint64_t pendingtxid,circulation,unspent = 0;
    int32_t i,numgateways,createdflag,nummsigs;
    struct NXT_asset *ap;
    struct multisig_addr **msigs;
    ap = get_NXTasset(&createdflag,Global_mp,assetstr);
    cp = conv_assetid(assetstr);
    if ( cp == 0 || ap == 0 )
    {
        sprintf(retbuf,"{\"error\":\"dont have coin_info for (%s) ap.%p\"}",assetstr,ap);
        return(clonestr(retbuf));
    }
    if ( firsttimestamp == 0 )
        get_NXTblock(&firsttimestamp);
    numgateways = init_specialNXTaddrs(specialNXTaddrs,ipaddrs,specialNXT,NXT0,NXT1,NXT2,ip0,ip1,ip2,exclude0,exclude1);
    if ( (pendingtxid= update_NXTblockchain_info(cp,specialNXTaddrs,numgateways,specialNXT)) != 0 )
        return(wait_for_pendingtxid(cp,specialNXTaddrs,specialNXT,pendingtxid));
    circulation = calc_circulation(0,ap,specialNXTaddrs);
    retstr = retstr2 = 0;
    printf("circulation %.8f\n",dstr(circulation));
    if ( (msigs= (struct multisig_addr **)copy_all_DBentries(&nummsigs,MULTISIG_DATA)) != 0 )
    {
        printf("nummsigs.%d\n",nummsigs);
        if ( actionflag >= 0 )
            retstr = process_deposits(&unspent,msigs,nummsigs,cp,ipaddrs,specialNXTaddrs,numgateways,specialNXT,ap,actionflag > 0,circulation);
        printf("actionflag.%d retstr.%p\n",actionflag,retstr);
        if ( actionflag <= 0 )
        {
            if ( actionflag < 0 )
                retstr = process_deposits(&unspent,msigs,nummsigs,cp,ipaddrs,specialNXTaddrs,numgateways,specialNXT,ap,actionflag > 0,circulation);
            retstr2 = process_withdraws(msigs,nummsigs,unspent,cp,ap,specialNXT,actionflag < 0,circulation);
        }
        for (i=0; i<nummsigs; i++)
            free(msigs[i]);
        free(msigs);
    }
    if ( retstr == 0 )
        retstr = clonestr("{}");
    else if ( retstr2 == 0 )
        retstr2 = clonestr("{}");
    sprintf(retbuf,"[%s, %s]\n",retstr,retstr2);
    free(retstr);
    free(retstr2);
    retstr = clonestr(retbuf);
    printf("MGWDEPOSITS.(%s)\n",retstr);
    return(retstr);
}
#endif

