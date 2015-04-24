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
    //if ( cp->lastheighttime+100000 < microseconds() )
    {
        height = (uint32_t)issue_bitcoind_command(0,cp,"getinfo","blocks","");
        cp->lastheighttime = microseconds();
       // printf("heoight.%d\n",height);
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
    int32_t map_msigaddr(char *redeemScript,struct coin_info *cp,char *normaladdr,char *msigaddr);
    int32_t i;
    char *txid;//normaladdr[1024],redeemScript[4096];
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
        printf("vp.%p txid.(%s).%d %s %.8f\n",vp,vp->txid,vp->entry.v,vp->coinaddr,dstr(vp->value));
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json,"txid",cJSON_CreateString(txid));
        cJSON_AddItemToObject(json,"vout",cJSON_CreateNumber(vp->entry.v));
        cJSON_AddItemToObject(json,"scriptPubKey",cJSON_CreateString(vp->script));
#ifdef later
        if ( (ret= map_msigaddr(redeemScript,cp,normaladdr,vp->coinaddr)) >= 0 )
            cJSON_AddItemToObject(json,"redeemScript",cJSON_CreateString(redeemScript));
        else printf("ret.%d redeemScript.(%s) (%s) for (%s)\n",ret,redeemScript,normaladdr,vp->coinaddr);
#endif
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

int32_t establish_connection(char *ipaddr,char *NXTADDR,char *NXTACCTSECRET,uint32_t timeout,int32_t selector)
{
    uint32_t i,start,totallen = 65536;
    struct pserver_info *pserver;
    uint8_t *buf;
    char *retstr;
    printf("ESTABLISH_CONNECTION.(%s)\n",ipaddr);
    pserver = get_pserver(0,ipaddr,0,0);
    start = (uint32_t)time(NULL);
    timeout += start;
    buf = calloc(1,totallen);
    while ( time(NULL) < timeout )
    {
        for (i=0; i<7; i++)
        {
            switch ( selector )
            {
                case 2:  p2p_publishpacket(pserver,0); break;
                case 0:  send_kademlia_cmd(0,pserver,"ping",NXTACCTSECRET,0,0); break;
                case 1:
                    retstr = start_transfer(0,NXTADDR,NXTADDR,NXTACCTSECRET,pserver->ipaddr,"ramtest",buf,totallen,timeout,"null",1);
                    if ( retstr != 0 )
                        free(retstr);
                    break;
            }
            portable_sleep(1);
            if ( pserver->lastcontact > start )
                break;
            fprintf(stderr,"%u ",pserver->lastcontact);
        }
        fprintf(stderr,"| vs start.%u\n",start);
    }
    for (i=0; i<totallen; i++)
        buf[i] = rand() >> 8;
    //printf("START_TRANSFER\n");
    retstr = start_transfer(0,NXTADDR,NXTADDR,NXTACCTSECRET,pserver->ipaddr,"ramtest",buf,totallen,timeout,"null",1);
    if ( retstr != 0 )
        free(retstr);
    free(buf);
    return(pserver->lastcontact > start);
}

void establish_connections(char *myipaddr,char *NXTADDR,char *NXTACCTSECRET)
{
    char ipaddr[MAX_JSON_FIELD];
    int32_t iter,i,n,m = 0;
    cJSON *array;
    array = cJSON_GetObjectItem(MGWconf,"whitelist");
    if ( array != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
    {
        for (iter=0; iter<1; iter++)
        {
            m = 0;
            while ( m < n )
            {
                for (i=m=0; i<n; i++)
                {
                    copy_cJSON(ipaddr,cJSON_GetArrayItem(array,i));
                    if ( strcmp(ipaddr,myipaddr) != 0 )
                        m += establish_connection(ipaddr,NXTADDR,NXTACCTSECRET,15,iter);
                }
            }
        }
    }
}
#endif
