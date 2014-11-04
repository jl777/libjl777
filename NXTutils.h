
//  Created by jl777
//  MIT License
//

#ifndef gateway_NXTutils_h
#define gateway_NXTutils_h


char *load_file(char *fname,char **bufp,int64_t  *lenp,int64_t  *allocsizep)
{
    FILE *fp;
    int64_t  filesize,buflen = *allocsizep;
    char *buf = *bufp;
    *lenp = 0;
    if ( (fp= fopen(fname,"rb")) != 0 )
    {
        fseek(fp,0,SEEK_END);
        filesize = ftell(fp);
        if ( filesize == 0 )
        {
            fclose(fp);
            *lenp = 0;
            return(0);
        }
        if ( filesize > buflen-1 )
        {
            *allocsizep = filesize+1;
            *bufp = buf = realloc(buf,(long)*allocsizep);
            //buflen = filesize+1;
        }
        rewind(fp);
        if ( buf == 0 )
            printf("Null buf ???\n");
        else
        {
            if ( fread(buf,1,(long)filesize,fp) != (unsigned long)filesize )
                printf("error reading filesize.%ld\n",(long)filesize);
            buf[filesize] = 0;
        }
        fclose(fp);
        
        *lenp = filesize;
    }
    return(buf);
}

#ifndef WIN32
char *_issue_cmd_to_buffer(char *prog,char *arg1,char *arg2,char *arg3)
{
    char buffer[4096],cmd[512];
    int32_t fd[2];
    unsigned long len;
	static portable_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
 	portable_mutex_lock(&mutex);
    if ( pipe(fd) != 0 )
        printf("_issue_cmd_to_buffer error doing pipe(fd)\n");
    pid_t pid = fork();
    if ( pid == 0 )
    {
        dup2(fd[1],STDOUT_FILENO);
        close(fd[0]);
        sprintf(cmd,"%s %s %s %s",prog,arg1,arg2,arg3);
#ifdef DEBUG_MODE
        if ( strcmp(arg1,"getinfo") != 0 && strcmp(arg1,"getrawtransaction") != 0 &&
            strcmp(arg1,"decoderawtransaction") != 0 && strcmp(arg1,"validateaddress") != 0 &&
            strcmp(arg1,"getaccountaddress") != 0 )
#endif
        fprintf(stderr,"pid.0 ISSUE.(%s)\n",cmd);
        if ( system(cmd) != 0 )
            printf("error issuing system(%s)\n",cmd);
        exit(0);
    }
    else
    {
        fprintf(stderr,"pid != 0 ISSUE.(%s)\n",prog);
        dup2(fd[0],STDIN_FILENO);
        close(fd[1]);
        len = 0;
        buffer[0] = 0;
        while ( fgets(buffer+len,(int32_t)(sizeof(buffer)-len),stdin) != 0 )
        {
            printf("%s\n",buffer+len);
            len += strlen(buffer+len);
        }
        waitpid(pid,NULL,0);
        while ( len > 2 && (buffer[len-1] == '\r' || buffer[len-1] == '\n' || buffer[len-1] == '\t' || buffer[len-1] == ' ') )
            len--;
        buffer[len] = 0;
        while ( fgetc(stdin) != EOF )
            ;
        portable_mutex_unlock(&mutex);
        if ( len > 0 )
            return(clonestr(buffer));
    }
    return(0);
}

char *oldget_ipaddr()
{
    static char *ipaddr;
    char *devs[] = { "eth0", "em1" };
    int32_t iter;
    char *retstr,*tmp;
    if ( ipaddr != 0 )
        return(ipaddr);
    for (iter=0; iter<(int32_t)((sizeof(devs))/(sizeof(*devs))); iter++)
    {
        printf("iter.%d\n",iter);
        retstr = _issue_cmd_to_buffer("ifconfig",devs[iter],"| grep \"inet addr\" | awk '{print $2}'","");
        if ( retstr != 0 && strncmp(retstr,"addr:",strlen("addr:")) == 0 )
        {
            tmp = clonestr(retstr + strlen("addr:"));
            myfree(retstr,"1");
            if ( ipaddr != 0 )
                myfree(ipaddr,"12");
            ipaddr = tmp;
            return(ipaddr);
        } else ipaddr = 0;
    }
    ipaddr = clonestr("127.0.0.1");
    return(ipaddr);
}
#endif

char *get_ipaddr()
{
    static CURL *curl_handle;
    static char *_ipaddr;
    char *ipsrcs[] = { "http://ip-api.com/json", "http://ip.jsontest.com/?showMyIP", "http://www.telize.com/jsonip", "http://www.trackip.net/ip?json"};
    int32_t i,match = 1;
    cJSON *json,*obj;
    char *jsonstr,ipaddr[512],str[512];
  //  return(0);
    if ( _ipaddr != 0 )
        return(_ipaddr);
    if ( curl_handle == 0 )
        curl_handle = curl_easy_init();
    if ( curl_handle == 0 )
        return(0);
    ipaddr[0] = 0;
    for (i=0; i<(int32_t)(sizeof(ipsrcs)/sizeof(*ipsrcs)); i++)
    {
        jsonstr = issue_curl(curl_handle,ipsrcs[i]);
        if ( jsonstr != 0 )
        {
            json = cJSON_Parse(jsonstr);
            if ( json != 0 )
            {
                obj = cJSON_GetObjectItem(json,(i==0)?"query":"ip");
                copy_cJSON(str,obj);
                printf("(%s) ",str);
                if ( strcmp(str,ipaddr) != 0 )
                    strcpy(ipaddr,str);
                else match++;
                free_json(json);
            }
            free(jsonstr);
        }
    }
    if ( match != i )
        printf("-> (%s) ipaddr matches.%d vs queries.%d\n",ipaddr,match,i);
    if ( ipaddr[0] != 0 )
        _ipaddr = clonestr(ipaddr);
    return(_ipaddr);
}

/*const char *choose_poolserver(char *NXTaddr)
{
    //static int32_t lastind = -1;
   // uint64_t hashval;
    return(SERVER_NAMEA);
    while ( 1 )
    {
        if ( lastind == -1 )
        {
            hashval = calc_decimalhash(NXTaddr);
            lastind = (hashval % Numguardians);
        }
        else lastind = ((lastind+1) % Numguardians);
        if ( Guardian_names[lastind][0] != 0 )
            break;
    }
    return(Guardian_names[lastind]);
}*/

union NXTtype extract_NXTfield(CURL *curl_handle,char *origoutput,char *cmd,char *field,int32_t type)
{
    char *jsonstr,*output,NXTaddr[MAX_NXTADDR_LEN];
    cJSON *json,*obj,*errobj;
    union NXTtype retval;
    retval.nxt64bits = 0;
    if ( origoutput == 0 )
        output = NXTaddr;
    else output = origoutput;
    jsonstr = issue_NXTPOST(curl_handle,cmd);
    if ( jsonstr != 0 )
    {
        //if ( field != 0 && strcmp(field,"transactionId") == 0 )
        //    printf("jsonstr.(%s)\n",jsonstr);
        json = cJSON_Parse(jsonstr);
        if ( json == 0 ) printf("Error before: (%s) -> [%s]\n",jsonstr,cJSON_GetErrorPtr());
        else
        {
            errobj = cJSON_GetObjectItem(json,"errorCode");
            if ( errobj != 0 )
            {
                printf("cmd.(%s) -> %s\n",cmd,jsonstr);
            }
            if ( field == 0 )
            {
                if ( origoutput == 0 )
                    retval.json = json;
                else
                {
                    copy_cJSON(origoutput,json);
                    retval.str = origoutput;
                    free_json(json);
                }
            }
            else
            {
                obj = cJSON_GetObjectItem(json,field);
                if ( obj != 0 )
                {
                    copy_cJSON(output,obj);
                    //if ( strcmp(field,"transactionId") == 0 )
                    //    printf("obj.(%s) type.%d\n",output,type);
                    switch ( type )
                    {
                        case sizeof(double):
                            retval.dval = atof(output);
                            break;
                        case sizeof(uint32_t):
                            retval.uval = atoi(output);
                            break;
                        case -(int32_t)sizeof(int32_t):
                            retval.val = atoi(output);
                            break;
                        case -(int32_t)sizeof(int64_t):
                            retval.lval = calc_nxt64bits(output);
                            break;
                        case 64:
                            retval.nxt64bits = calc_nxt64bits(output);
                            //if ( strcmp(field,"transactionId") == 0 )
                            //    printf("transactionId.%s\n",nxt64str(retval.nxt64bits));
                           break;
                        case 0:
                            if ( origoutput != 0 )
                                retval.str = origoutput;
                            else retval.str = 0;
                            break;
                        default: printf("extract_NXTfield: warning unknown type.%d\n",type);
                    }
                }
                free_json(json);
            }
        }
        myfree(jsonstr,"33");
    }
    else printf("ERROR submitting cmd.(%s)\n",cmd);
    return(retval);
}

int32_t issue_getTime(CURL *curl_handle)
{
    char cmd[4096];
    union NXTtype ret;
    sprintf(cmd,"%s=getTime",_NXTSERVER);
    ret = extract_NXTfield(curl_handle,0,cmd,"time",sizeof(int32_t));
    return(ret.val);
}

uint64_t issue_getAccountId(CURL *curl_handle,char *password)
{
    char cmd[4096];
    union NXTtype ret;
    bits256 mysecret,mypublic;
    return(conv_NXTpassword(mysecret.bytes,mypublic.bytes,password));
    sprintf(cmd,"%s=getAccountId&secretPhrase=%s",_NXTSERVER,password);
    ret = extract_NXTfield(curl_handle,0,cmd,"accountId",64);
    if ( ret.nxt64bits == 0 )
        ret = extract_NXTfield(curl_handle,0,cmd,"account",64);
    return(ret.nxt64bits);
}

int32_t issue_startForging(CURL *curl_handle,char *secret)
{
    char cmd[4096];
    union NXTtype ret;
    sprintf(cmd,"%s=startForging&secretPhrase=%s",_NXTSERVER,secret);
    ret = extract_NXTfield(curl_handle,0,cmd,"deadline",sizeof(int32_t));
    return(ret.val);
}

uint64_t issue_broadcastTransaction(int32_t *errcodep,CURL *curl_handle,char *txbytes,char *NXTACCTSECRET)
{
    cJSON *json,*errjson;
    uint64_t txid = 0;
    char cmd[4096],*retstr;
    sprintf(cmd,"%s=broadcastTransaction&secretPhrase=%s&transactionBytes=%s",_NXTSERVER,NXTACCTSECRET,txbytes);
    retstr = issue_NXTPOST(curl_handle,cmd);
    *errcodep = -1;
    if ( retstr != 0 )
    {
        printf("broadcast got.(%s)\n",retstr);
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            errjson = cJSON_GetObjectItem(json,"errorCode");
            if ( errjson != 0 )
            {
                printf("ERROR submitting assetxfer.(%s)\n",retstr);
                *errcodep = (int32_t)get_cJSON_int(json,"errorCode");
            }
            else
            {
                if ( (txid = get_satoshi_obj(json,"transaction")) != 0 )
                    *errcodep = 0;
            }
        }
        free(retstr);
    }
    return(txid);
}

char *issue_signTransaction(CURL *curl_handle,char *txbytes,char *NXTACCTSECRET)
{
    char cmd[4096];
    sprintf(cmd,"%s=signTransaction&secretPhrase=%s&unsignedTransactionBytes=%s",_NXTSERVER,NXTACCTSECRET,txbytes);
    return(issue_NXTPOST(curl_handle,cmd));
}

uint64_t issue_transferAsset(char **retstrp,CURL *curl_handle,char *secret,char *recipient,char *asset,int64_t quantity,int64_t feeNQT,int32_t deadline,char *comment)
{
    char cmd[4096],numstr[128],*str,*jsontxt;
    uint64_t txid = 0;
    cJSON *json,*errjson,*txidobj;
    if ( retstrp != 0 )
        *retstrp = 0;
    sprintf(cmd,"%s=transferAsset&secretPhrase=%s&recipient=%s&asset=%s&quantityQNT=%lld&feeNQT=%lld&deadline=%d",_NXTSERVER,secret,recipient,asset,(long long)quantity,(long long)feeNQT,deadline);
    if ( comment != 0 )
    {
        if ( Global_mp->NXTheight >= DGSBLOCK )
            strcat(cmd,"&message=");
        else strcat(cmd,"&comment=");
        strcat(cmd,comment);
    }
    jsontxt = issue_NXTPOST(curl_handle,cmd);
    if ( jsontxt != 0 )
    {
        printf(" transferAsset.(%s) -> %s\n",cmd,jsontxt);
        //if ( field != 0 && strcmp(field,"transactionId") == 0 )
        //    printf("jsonstr.(%s)\n",jsonstr);
        json = cJSON_Parse(jsontxt);
        if ( json != 0 )
        {
            errjson = cJSON_GetObjectItem(json,"errorCode");
            if ( errjson != 0 )
            {
                printf("ERROR submitting assetxfer.(%s)\n",jsontxt);
#ifdef BTC_COINID
                int32_t _get_gatewayid();
                if ( _get_gatewayid() >= 0 )
                {
                    sleep(60);
                    fprintf(stderr,"ERROR submitting assetxfer.(%s)\n",jsontxt);
                    exit(-1);
                }
#endif
            }
            txidobj = cJSON_GetObjectItem(json,"transaction");
            copy_cJSON(numstr,txidobj);
            txid = calc_nxt64bits(numstr);
            if ( txid == 0 )
            {
                str = cJSON_Print(json);
                printf("ERROR WITH ASSET TRANSFER.(%s) -> \n%s\n",cmd,str);
                free(str);
            }
            free_json(json);
        } else printf("error issuing asset.(%s) -> %s\n",cmd,jsontxt);
        if ( retstrp == 0 )
            free(jsontxt);
        else *retstrp = jsontxt;
    }
    return(txid);
}

cJSON *issue_getAccountInfo(CURL *curl_handle,int64_t *amountp,char *name,char *username,char *NXTaddr,char *groupname)
{
    union NXTtype retval;
    cJSON *obj,*json = 0;
    char buf[2048];
    *amountp = 0;
    sprintf(buf,"%s=getAccount&account=%s",_NXTSERVER,NXTaddr);
    retval = extract_NXTfield(curl_handle,0,buf,0,0);
    if ( retval.json != 0 )
    {
        //printf("%s\n",cJSON_Print(retval.json));
        obj = cJSON_GetObjectItem(retval.json,"balanceNQT");
        copy_cJSON(buf,obj);
        *amountp = calc_nxt64bits(buf);
        obj = cJSON_GetObjectItem(retval.json,"name");
        copy_cJSON(name,obj);
        obj = cJSON_GetObjectItem(retval.json,"description");
        copy_cJSON(buf,obj);
        unstringify(buf);
        json = cJSON_Parse(buf);
        if ( json != 0 )
        {
            //printf("%s\n",cJSON_Print(json));
            obj = cJSON_GetObjectItem(json,"username");
            copy_cJSON(username,obj);
            obj = cJSON_GetObjectItem(json,"group");
            copy_cJSON(groupname,obj);
            //printf("name.%s username.%s groupname.%s\n%s\n",name,username,cJSON_Print(json),groupname);
        }
        free_json(retval.json);
    }
    return(json);
}


struct NXT_asset *get_NXTasset(int32_t *createdp,struct NXThandler_info *mp,char *assetidstr)
{
    struct NXT_asset *ap;
    ap = MTadd_hashtable(createdp,mp->NXTassets_tablep,assetidstr);
    if ( *createdp != 0 )
    {
        ap->assetbits = ap->H.nxt64bits = calc_nxt64bits(assetidstr);
    }
    return(ap);
}

struct NXT_acct *get_NXTacct(int32_t *createdp,struct NXThandler_info *mp,char *NXTaddr)
{
    struct NXT_acct *np;
    //printf("NXTaccts hash %p\n",mp->NXTaccts_tablep);
    //printf("get_NXTacct.(%s)\n",NXTaddr);
    np = MTadd_hashtable(createdp,mp->NXTaccts_tablep,NXTaddr);
    if ( *createdp != 0 )
    {
        //queue_enqueue(&np->incoming,clonestr("testmessage"));
        np->H.nxt64bits = calc_nxt64bits(NXTaddr);
        //portable_set_illegaludp(&np->Usock);//np->Usock = -1;
    }
    return(np);
}

/*struct NXT_acct *search_addresses(char *addr)
{
    char NXTaddr[64],*BTCDaddr,*BTCaddr;
    int32_t createdflag;
    struct NXT_acct *np;
    struct other_addr *op;
    if ( addr == 0 || addr[0] == 0 )
        return(0);
    NXTaddr[0] = 0;
    if ( strlen(addr) < MAX_NXTADDR_LEN && search_hashtable(*Global_mp->NXTaccts_tablep,addr) != HASHSEARCH_ERROR )
        return(get_NXTacct(&createdflag,Global_mp,addr));
    if ( search_hashtable(*Global_mp->otheraddrs_tablep,addr) != HASHSEARCH_ERROR )
    {
        op = MTadd_hashtable(&createdflag,Global_mp->otheraddrs_tablep,addr);
        expand_nxt64bits(NXTaddr,op->nxt64bits);
        np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
        if ( np != 0 )
        {
            BTCDaddr = np->mypeerinfo.pubBTCD;
            BTCaddr = np->mypeerinfo.pubBTC;
        } else BTCDaddr = BTCaddr = "";
        if ( (BTCDaddr[0] != 0 && strcmp(BTCDaddr,addr) == 0) || (BTCaddr[0] != 0 && strcmp(BTCaddr,addr) == 0) )
            return(np);
        printf("UNEXPECTED ERROR searching (%s), got NXT.%s but doesnt match (%s) (%s)\n",addr,np->H.U.NXTaddr,BTCDaddr,BTCaddr);
        return(0);
    }
    return(0);
}*/

uint64_t calc_txid(unsigned char *buf,int32_t len)
{
    uint64_t txid,hash[4];
    calc_sha256(0,(unsigned char *)&hash[0],buf,len);
    if ( sizeof(hash) >= sizeof(txid) )
        memcpy(&txid,hash,sizeof(txid));
    else memcpy(&txid,hash,sizeof(hash));
    //printf("calc_txid.(%llu)\n",(long long)txid);
    //return(hash[0] ^ hash[1] ^ hash[2] ^ hash[3]);
    return(txid);
}

int server_address(char *server,int port,struct sockaddr_in *addr)
{
    struct hostent *hp = gethostbyname(server);
    if ( hp == NULL )
    {
        perror("gethostbyname");
        return -1;
    }
    addr->sin_port = htons(port);
    addr->sin_family = AF_INET;
    memcpy(&addr->sin_addr.s_addr, hp->h_addr, hp->h_length);
    return(0);
}

uint16_t extract_nameport(char *name,long namesize,struct sockaddr_in *addr)
{
    int32_t r;
    uint16_t port;
    if ( (r= uv_ip4_name(addr,name,namesize)) != 0 )
    {
        fprintf(stderr,"uv_ip4_name error %d (%s)\n",r,uv_err_name(r));
        return(0);
    }
    port = ntohs(addr->sin_port);
    //printf("sender.(%s) port.%d\n",name,port);
    return(port);
}

struct NXT_acct *find_NXTacct(char *NXTaddr,char *NXTACCTSECRET)
{
    int32_t createdflag;
    uint64_t nxt64bits;
    if ( NXTaddr[0] == 0 )
    {
        nxt64bits = issue_getAccountId(0,NXTACCTSECRET);
        expand_nxt64bits(NXTaddr,nxt64bits);
    }
    return(get_NXTacct(&createdflag,Global_mp,NXTaddr));
}

#ifdef BTC_COINID
int64_t get_coin_quantity(CURL *curl_handle,int64_t *unconfirmedp,int32_t coinid,char *NXTaddr)
{
    char *assetid_str(int32_t coinid);
    char cmd[4096],assetid[512],*assetstr;
    union NXTtype retval;
    int32_t i,n,iter,createdflag;
    cJSON *array,*item,*obj;
    int64_t quantity,qty,mult = 1;
    struct NXT_asset *assetp;
    quantity = *unconfirmedp = 0;
    sprintf(cmd,"%s=getAccount&account=%s",_NXTSERVER,NXTaddr);
    retval = extract_NXTfield(curl_handle,0,cmd,0,0);
    if ( retval.json != 0 )
    {
        assetstr = assetid_str(coinid);
        assetp = get_NXTasset(&createdflag,Global_mp,assetid_str(coinid));
        mult = assetp->mult;
        for (iter=0; iter<2; iter++)
        {
            qty = 0;
            array = cJSON_GetObjectItem(retval.json,iter==0?"assetBalances":"unconfirmedAssetBalances");
            if ( is_cJSON_Array(array) != 0 )
            {
                n = cJSON_GetArraySize(array);
                for (i=0; i<n; i++)
                {
                    item = cJSON_GetArrayItem(array,i);
                    obj = cJSON_GetObjectItem(item,"asset");
                    copy_cJSON(assetid,obj);
                    //printf("i.%d of %d: %s(%s)\n",i,n,assetid,cJSON_Print(item));
                    if ( strcmp(assetid,assetstr) == 0 )
                    {
                        qty = mult * get_cJSON_int(item,iter==0?"balanceQNT":"unconfirmedBalanceQNT");
                        break;
                    }
                }
            }
            if ( iter == 0 )
                quantity = qty;
            else *unconfirmedp = qty;
        }
    }
    return(quantity);
}
#endif

char *issue_getTransaction(CURL *curl_handle,char *txidstr)
{
    char cmd[4096];//,*jsonstr;
    //sprintf(cmd,"%s=getTransaction&transaction=%s",_NXTSERVER,txidstr);
    //jsonstr = issue_NXTPOST(curl_handle,cmd);
    sprintf(cmd,"%s=getTransaction&transaction=%s",NXTSERVER,txidstr);
    return(issue_curl(curl_handle,cmd));
}

uint64_t gen_randacct(char *randaddr)
{
    char secret[33];
    uint64_t randacct;
    bits256 priv,pub;
    randombytes((uint8_t *)secret,sizeof(secret));
    secret[sizeof(secret)-1] = 0;
    randacct = conv_NXTpassword(priv.bytes,pub.bytes,secret);
    expand_nxt64bits(randaddr,randacct);
    return(randacct);
}

uint64_t conv_rsacctstr(char *rsacctstr,uint64_t nxt64bits)
{
    cJSON *json;
    char field[32],cmd[4096],retstr[4096],*jsonstr = 0;
    strcpy(field,"account");
    retstr[0] = 0;
    if ( nxt64bits != 0 )
    {
        sprintf(cmd,"%s=rsConvert&account=%llu",NXTSERVER,(long long)nxt64bits);
        strcat(field,"RS");
        jsonstr = issue_curl(0,cmd);
    }
    else if ( rsacctstr[0] != 0 )
    {
        sprintf(cmd,"%s=rsConvert&account=%s",NXTSERVER,rsacctstr);
        jsonstr = issue_curl(0,cmd);
    }
    else printf("conv_rsacctstr: illegal parms %s %llu\n",rsacctstr,(long long)nxt64bits);
    if ( jsonstr != 0 )
    {
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            copy_cJSON(retstr,cJSON_GetObjectItem(json,field));
            free_json(json);
        }
        free(jsonstr);
        if ( nxt64bits != 0 )
            strcpy(rsacctstr,retstr);
        else nxt64bits = calc_nxt64bits(retstr);
    }
    return(nxt64bits);
}

bits256 issue_getpubkey(char *acct)
{
    cJSON *json;
    bits256 pubkey;
    char cmd[4096],pubkeystr[MAX_JSON_FIELD],*jsonstr;
    //sprintf(cmd,"%s=getTransaction&transaction=%s",_NXTSERVER,txidstr);
    //jsonstr = issue_NXTPOST(curl_handle,cmd);
    sprintf(cmd,"%s=getAccountPublicKey&account=%s",NXTSERVER,acct);
    jsonstr = issue_curl(0,cmd);
    pubkeystr[0] = 0;
    memset(&pubkey,0,sizeof(pubkey));
    if ( jsonstr != 0 )
    {
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            copy_cJSON(pubkeystr,cJSON_GetObjectItem(json,"publicKey"));
            free_json(json);
            if ( strlen(pubkeystr) == sizeof(pubkey)*2 )
                decode_hex(pubkey.bytes,sizeof(pubkey),pubkeystr);
        }
        free(jsonstr);
    }
    return(pubkey);
}

uint64_t cJSON_convassetid(cJSON *obj)
{
    char numstr[1024];
    uint64_t assetid = 0;
    copy_cJSON(numstr,obj);
    assetid = calc_nxt64bits(numstr);
    return(assetid);
}

uint64_t *issue_getAssetIds(int32_t *nump)
{
    cJSON *json,*array;
    int32_t i,n = 0;
    uint64_t *assetids = 0;
    char cmd[4096],*retstr;
    sprintf(cmd,"%s=getAssetIds",_NXTSERVER);
    retstr = issue_NXTPOST(0,cmd);
    if ( retstr != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"assetIds")) != 0 && is_cJSON_Array(array) != 0 )
            {
                n = cJSON_GetArraySize(array);
                assetids = malloc(sizeof(*assetids) * (n+1));
                for (i=0; i<n; i++)
                    assetids[i] = cJSON_convassetid(cJSON_GetArrayItem(array,i));
                assetids[i] = 0;
            }
            free_json(json);
        }
        free(retstr);
    }
    *nump = n;
    return(assetids);
}

char *issue_getAsset(CURL *curl_handle,char *assetidstr)
{
    char cmd[4096];
    sprintf(cmd,"%s=getAsset&asset=%s",_NXTSERVER,assetidstr);
    //printf("cmd.(%s)\n",cmd);
    return(issue_NXTPOST(curl_handle,cmd));
    //printf("calculated.(%s)\n",ret.str);
}

uint64_t get_asset_mult(uint64_t assetidbits)
{
    cJSON *json;
    int32_t i,decimals,errcode;
    uint64_t mult = 0;
    char assetidstr[64],*jsonstr;
    if ( assetidbits == 0 || assetidbits == ORDERBOOK_NXTID )
        return(1);
    expand_nxt64bits(assetidstr,assetidbits);
    jsonstr = issue_getAsset(0,assetidstr);
    if ( jsonstr != 0 )
    {
        //printf("Assetjson.(%s) for asset.(%s)\n",jsonstr,assetidstr);
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            errcode = (int32_t)get_cJSON_int(json,"errorCode");
            if ( errcode == 0 )
            {
                decimals = (int32_t)get_cJSON_int(json,"decimals");
                mult = 1;
                for (i=7-decimals; i>=0; i--)
                    mult *= 10;
            }
            free_json(json);
        }
        free(jsonstr);
    }
    //printf("assetoshis.%llu\n",(long long)assetoshis);
    return(mult);
}

uint64_t calc_assetoshis(uint64_t assetidbits,double amount)
{
    cJSON *json;
    int32_t i,decimals,errcode;
    uint64_t mult,assetoshis = 0;
    char assetidstr[64],*jsonstr;
    if ( assetidbits == 0 || assetidbits == ORDERBOOK_NXTID )
        return(amount * SATOSHIDEN);
    expand_nxt64bits(assetidstr,assetidbits);
    jsonstr = issue_getAsset(0,assetidstr);
    if ( jsonstr != 0 )
    {
        //printf("Assetjson.(%s) for asset.(%s)\n",jsonstr,assetidstr);
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            errcode = (int32_t)get_cJSON_int(json,"errorCode");
            if ( errcode == 0 )
            {
                decimals = (int32_t)get_cJSON_int(json,"decimals");
                mult = 1;
                for (i=7-decimals; i>=0; i--)
                    mult *= 10;
                assetoshis = (amount * SATOSHIDEN) / mult;
            }
            free_json(json);
        }
        free(jsonstr);
    }
    //printf("assetoshis.%llu\n",(long long)assetoshis);
    return(assetoshis);
}


double conv_assetoshis(uint64_t assetidbits,uint64_t assetoshis)
{
    cJSON *json;
    int32_t i,decimals,errcode;
    uint64_t mult;
    double amount = 0;
    char assetidstr[64],*jsonstr;
    if ( assetidbits == 0 || assetidbits == ORDERBOOK_NXTID )
        return((double)assetoshis / SATOSHIDEN);
    expand_nxt64bits(assetidstr,assetidbits);
    jsonstr = issue_getAsset(0,assetidstr);
    if ( jsonstr != 0 )
    {
        //printf("Assetjson.(%s) for asset.(%s)\n",jsonstr,assetidstr);
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            errcode = (int32_t)get_cJSON_int(json,"errorCode");
            if ( errcode == 0 )
            {
                decimals = (int32_t)get_cJSON_int(json,"decimals");
                mult = 1;
                for (i=7-decimals; i>=0; i--)
                    mult *= 10;
                amount = ((double)(assetoshis * mult) / SATOSHIDEN);
            }
            free_json(json);
        }
        free(jsonstr);
    }
    //printf("assetoshis.%llu\n",(long long)assetoshis);
    return(amount);
}

char *issue_calculateFullHash(CURL *curl_handle,char *unsignedtxbytes,char *sighash)
{
    char cmd[4096];
    sprintf(cmd,"%s=calculateFullHash&unsignedTransactionBytes=%s&signatureHash=%s",_NXTSERVER,unsignedtxbytes,sighash);
    return(issue_NXTPOST(curl_handle,cmd));
}

char *issue_parseTransaction(CURL *curl_handle,char *txbytes)
{
    char cmd[4096],*retstr = 0;
    sprintf(cmd,"%s=parseTransaction&transactionBytes=%s",_NXTSERVER,txbytes);
    retstr = issue_NXTPOST(curl_handle,cmd);
    //printf("issue_parseTransaction.%s %s\n",txbytes,retstr);
    if ( retstr != 0 )
    {
        //retstr = parse_NXTresults(0,"sender","",results_processor,jsonstr,strlen(jsonstr));
        //free(jsonstr);
    } else printf("error getting txbytes.%s\n",txbytes);
    return(retstr);
}

int32_t get_NXTtxid_confirmations(CURL *curl_handle,char *txid)
{
    char cmd[4096],*jsonstr;
    //union NXTtype ret;
    cJSON *json;
#ifdef DEBUG_MODE
    if ( strcmp(txid,"123456") == 0 )
        return(1000);
#endif
    sprintf(cmd,"%s=getTransaction&transaction=%s",NXTSERVER,txid);
    jsonstr = issue_curl(curl_handle,cmd);
    json = cJSON_Parse(jsonstr);
    free(jsonstr);
    return((int32_t)get_cJSON_int(json,"confirmations"));
   // ret = extract_NXTfield(0,cmd,"confirmations",sizeof(int32_t));
    //return(ret.val);
}

uint64_t issue_getBalance(CURL *curl_handle,char *NXTaddr)
{
    char cmd[4096];
    union NXTtype ret;
    sprintf(cmd,"%s=getBalance&account=%s",_NXTSERVER,NXTaddr);
    ret = extract_NXTfield(curl_handle,0,cmd,"balanceNQT",64);
    return(ret.nxt64bits);
}

int32_t issue_decodeToken(CURL *curl_handle,char *sender,int32_t *validp,char *key,unsigned char encoded[NXT_TOKEN_LEN])
{
    char cmd[4096],token[512+2*NXT_TOKEN_LEN+1];
    cJSON *nxtobj,*validobj;
    union NXTtype retval;
    *validp = -1;
    sender[0] = 0;
    memcpy(token,encoded,NXT_TOKEN_LEN);
    token[NXT_TOKEN_LEN] = 0;
    sprintf(cmd,"%s=decodeToken&website=%s&token=%s",_NXTSERVER,key,token);
    //printf("cmd.(%s)\n",cmd);
    retval = extract_NXTfield(curl_handle,0,cmd,0,0);
    if ( retval.json != 0 )
    {
        validobj = cJSON_GetObjectItem(retval.json,"valid");
        if ( validobj != 0 )
            *validp = ((validobj->type&0xff) == cJSON_True) ? 1 : 0;
        nxtobj = cJSON_GetObjectItem(retval.json,"account");
        copy_cJSON(sender,nxtobj);
        free_json(retval.json);
        //printf("decoded valid.%d NXT.%s len.%d\n",*validp,sender,(int32_t)strlen(sender));
        if ( sender[0] != 0 )
            return((int32_t)strlen(sender));
        else return(0);
    }
    return(-1);
}

int32_t issue_generateToken(CURL *curl_handle,char encoded[NXT_TOKEN_LEN],char *key,char *secret)
{
    char cmd[4096],token[512+2*NXT_TOKEN_LEN+1];
    cJSON *tokenobj;
    union NXTtype retval;
    encoded[0] = 0;
    sprintf(cmd,"%s=generateToken&website=%s&secretPhrase=%s",_NXTSERVER,key,secret);
    // printf("cmd.(%s)\n",cmd);
    retval = extract_NXTfield(curl_handle,0,cmd,0,0);
    if ( retval.json != 0 )
    {
        //printf("token.(%s)\n",cJSON_Print(retval.json));
        tokenobj = cJSON_GetObjectItem(retval.json,"token");
        memset(token,0,sizeof(token));
        copy_cJSON(token,tokenobj);
        free_json(retval.json);
        if ( token[0] != 0 )
        {
            memcpy(encoded,token,NXT_TOKEN_LEN);
            return(0);
        }
    }
    return(-1);
}

char *submit_AM(CURL *curl_handle,char *recipient,struct NXT_AMhdr *ap,char *reftxid,char *NXTACCTSECRET)
{
    //union NXTtype retval;
    int32_t len,deadline = 720;
    cJSON *json,*txjson,*errjson;
    char hexbytes[4096],cmd[5120],txid[MAX_NXTADDR_LEN],*jsonstr,*retstr = 0;
    len = ap->size;//(int32_t)sizeof(*ap);
    if ( len > 1000 || len < 1 )
    {
        printf("issue_sendMessage illegal len %d\n",len);
        return(0);
    }
    // jl777: here is where the entire message could be signed;
   // printf("in submit_AM\n");
    memset(hexbytes,0,sizeof(hexbytes));
    init_hexbytes_truncate(hexbytes,(void *)ap,len);
    sprintf(cmd,"%s=sendMessage&secretPhrase=%s&recipient=%s&message=%s&deadline=%u%s&feeNQT=%lld",_NXTSERVER,NXTACCTSECRET,recipient,hexbytes,deadline,reftxid!=0?reftxid:"",(long long)MIN_NQTFEE);
    //printf("submit_AM.(%s)\n",cmd);
    jsonstr = issue_NXTPOST(curl_handle,cmd);
    printf("back from issue_NXTPOST.(%s) %p\n",jsonstr,curl_handle);
    if ( jsonstr != 0 )
    {
        json = cJSON_Parse(jsonstr);
        if ( json == 0 ) printf("Error parsing: (%s)\n",jsonstr);
        else
        {
            errjson = cJSON_GetObjectItem(json,"errorCode");
            if ( errjson != 0 )
            {
                fprintf(stderr,"ERROR submitting AM.(%s)\n",jsonstr);
                sleep(60);
                exit(-1);
            }
            txjson = cJSON_GetObjectItem(json,"transaction");
            copy_cJSON(txid,txjson);
            if ( txid[0] != 0 )
            {
                retstr = clonestr(txid);
                printf("AMtxid.%s\n",txid);
            }
            free_json(json);
        }
        free(jsonstr);
        if ( (retstr == 0 || retstr[0] == 0) && jsonstr != 0 )
            printf("submitAM.(%s) -> (%s)\n",cmd,jsonstr);
    }
    return(retstr);
}

int32_t set_json_AM(struct json_AM *ap,int32_t sig,int32_t funcid,char *nxtaddr,int32_t timestamp,char *jsonstr,int32_t compressjson)
{
    int32_t jsonflag;
    long len = 0;
    char *teststr;
    struct compressed_json *jsn = 0;
    compressjson = 0; // some cases dont decompress on the other side, even though decode tests fine :(
    if ( jsonstr == 0 )
        jsonflag = 0;
    else jsonflag = 1 + (compressjson != 0);
    if ( jsonflag == 2 )
    {
        if ( (jsn= encode_json(jsonstr,(int32_t)strlen(jsonstr)+1)) != 0 )
            len = sizeof(*ap) + jsn->complen;
        else
        {
            printf("set_json_AM: error encoding.(%s)\n",jsonstr);
            return(-1);
        }
    }
    if ( jsonflag != 0 )
    {
        if ( len == 0 )
        {
            len = sizeof(*ap) + strlen(jsonstr) + 1;
            jsonflag = 1;
        }
    } else len = sizeof(*ap);
    memset(ap,0,len);
    ap->H.sig = sig;
    if ( nxtaddr != 0 )
    {
        ap->H.nxt64bits = calc_nxt64bits(nxtaddr);
        //printf("set_json_AM: NXT.%s -> %s %llx\n",nxtaddr,nxt64str(ap->H.nxt64bits),ap->H.nxt64bits);
    }
    ap->funcid = funcid;
    //ap->gatewayid = gp->gatewayid;
    ap->timestamp = timestamp;
    ap->jsonflag = jsonflag;
    if ( jsonflag == 1 )
        strcpy(ap->U.jsonstr,jsonstr);//,999 - ((long)ap->jsonstr - (long)ap));
    else if ( jsonflag > 1 )
    {
        memcpy(&ap->U.jsn,jsn,sizeof(*jsn)+jsn->complen);
        free(jsn);
        teststr = decode_json(&ap->U.jsn,jsonflag-2);
        if ( teststr != 0 )
        {
            stripstr(teststr,(int64_t)ap->U.jsn.origlen);
            if ( strcmp(teststr,jsonstr) != 0 )
                printf("JSONcodec error (%s) != (%s)\n",teststr,jsonstr);
            else printf("decoded.(%s) %d %d %d starting\n",teststr,ap->U.jsn.complen,ap->U.jsn.origlen,ap->U.jsn.sublen);
            free(teststr);
        }
    }
    //printf("AM len.%ld vs %ld (%s)\n",len,strlen(ap->jsonstr),ap->jsonstr);
    ap->H.size = (int32_t)len;
    return(0);
}

void set_next_NXTblock(CURL *curl_handle,int32_t *timestamp,char *nextblock,char *blockidstr)
{
    union NXTtype retval;
    char cmd[4096],*jsonstr;
    int32_t errcode;
    cJSON *nextjson;
    nextblock[0] = 0;
    if ( timestamp != 0 )
        *timestamp = 0;
    sprintf(cmd,"%s=getBlock&block=%s",NXTSERVER,blockidstr);
    jsonstr = issue_curl(curl_handle,cmd);
    if ( jsonstr != 0 )
    {
        retval.json = cJSON_Parse(jsonstr);
        free(jsonstr);
    }
    else retval.json = 0;
    // printf("cmd.(%s)\n",cmd);
    //retval = extract_NXTfield(0,cmd,0,0);
    if ( retval.json != 0 )
    {
        //
        errcode = (int32_t)get_cJSON_int(retval.json,"errorCode");
        if ( errcode == 0 )
        {
            if ( timestamp != 0 )
                *timestamp = (int32_t)get_cJSON_int(retval.json,"timestamp");
            nextjson = cJSON_GetObjectItem(retval.json,"nextBlock");
            if ( nextjson != 0 )
                copy_cJSON(nextblock,nextjson);
        } else printf("%s\n",cJSON_Print(retval.json));
        free_json(retval.json);
    }
}

void set_prev_NXTblock(CURL *curl_handle,int32_t *height,int32_t *timestamp,char *prevblock,char *blockidstr)
{
    union NXTtype retval;
    char cmd[4096],*jsonstr;
    int32_t errcode;
    cJSON *prevjson;
    prevblock[0] = 0;
    if ( timestamp != 0 )
        *timestamp = 0;
    sprintf(cmd,"%s=getBlock&block=%s",NXTSERVER,blockidstr);
    jsonstr = issue_curl(curl_handle,cmd);
    if ( jsonstr != 0 )
    {
        retval.json = cJSON_Parse(jsonstr);
        free(jsonstr);
    }
    else retval.json = 0;
    // printf("cmd.(%s)\n",cmd);
    //retval = extract_NXTfield(0,cmd,0,0);
    if ( retval.json != 0 )
    {
        //printf("%s\n",cJSON_Print(retval.json));
        errcode = (int32_t)get_cJSON_int(retval.json,"errorCode");
        if ( errcode == 0 )
        {
            if ( timestamp != 0 )
                *timestamp = (int32_t)get_cJSON_int(retval.json,"timestamp");
            if ( height != 0 )
                *height = (int32_t)get_cJSON_int(retval.json,"height");
            prevjson = cJSON_GetObjectItem(retval.json,"previousBlock");
            if ( prevjson != 0 )
                copy_cJSON(prevblock,prevjson);
        } else printf("%s\n",cJSON_Print(retval.json));
        free_json(retval.json);
    }
}

int32_t set_current_NXTblock(int32_t *isrescanp,CURL *curl_handle,char *blockidstr)
{
    int32_t numblocks = 0;//,numunlocked = 0;
    union NXTtype retval;
    cJSON *blockjson,*scanjson;//,*unlocked;//*nextjson,
    char cmd[256],scanstr[256];//,unlockedstr[1024];
    sprintf(cmd,"%s=getState",_NXTSERVER);
    *isrescanp = 0;
    blockidstr[0] = 0;
    retval = extract_NXTfield(curl_handle,0,cmd,0,0);
    if ( retval.json != 0 )
    {
        numblocks = (int32_t)get_cJSON_int(retval.json,"numberOfBlocks");
        //*isrescanp = (int32_t)get_cJSON_int(retval.json,"isScanning");
        scanjson = cJSON_GetObjectItem(retval.json,"isScanning");
        if ( scanjson != 0 )
        {
            copy_cJSON(scanstr,scanjson);
            if ( strcmp(scanstr,"true") == 0 )
                *isrescanp = 1;
        }
        blockjson = cJSON_GetObjectItem(retval.json,"lastBlock");
        copy_cJSON(blockidstr,blockjson);
        free_json(retval.json);
    }
    return(numblocks);
}

int32_t gen_randomacct(CURL *curl_handle,uint32_t randchars,char *NXTaddr,char *NXTsecret,char *randfilename)
{
    uint32_t i,j,x,iter,bitwidth = 6;
    FILE *fp;
    char fname[512];
    unsigned char bits[33];
    NXTaddr[0] = 0;
    randchars /= 8;
    if ( randchars > (int32_t)sizeof(bits) )
        randchars = (int32_t)sizeof(bits);
    if ( randchars < 3 )
        randchars = 3;
    for (iter=0; iter<=8; iter++)
    {
        sprintf(fname,"%s.%d",randfilename,iter);
        fp = fopen(fname,"rb");
        if ( fp == 0 )
        {
            randombytes(bits,sizeof(bits));
            for (i = 0; i < sizeof(bits); i++)
                printf("%02x ", bits[i]);
            printf("write\n");
            //sprintf(buf,"dd if=/dev/random count=%d bs=1 > %s",randchars*8,fname);
            //printf("cmd.(%s)\n",buf);
            //if ( system(buf) != 0 )
            //    printf("error issuing system(%s)\n",buf);
            fp = fopen(fname,"wb");
            if ( fp != 0 )
            {
                fwrite(bits,1,sizeof(bits),fp);
                fclose(fp);
            }
            sleep(3);
            fp = fopen(fname,"rb");
        }
        if ( fp != 0 )
        {
            if ( fread(bits,1,sizeof(bits),fp) == 0 )
                printf("gen_random_acct: error reading bits\n");
            for (i=0; i+bitwidth<(sizeof(bits)*8) && i/bitwidth<randchars; i+=bitwidth)
            {
                for (j=x=0; j<6; j++)
                {
                    if ( GETBIT(bits,i*bitwidth+j) != 0 )
                        x |= (1 << j);
                }
                //printf("i.%d j.%d x.%d %c\n",i,j,x,1+' '+x);
                NXTsecret[randchars*iter + i/bitwidth] = safechar64(x);
            }
            NXTsecret[randchars*iter + i/bitwidth] = 0;
            fclose(fp);
        }
    }
    expand_nxt64bits(NXTaddr,issue_getAccountId(curl_handle,NXTsecret));
    printf("NXT.%s NXTsecret.(%s)\n",NXTaddr,NXTsecret);
    return(0);
}

int32_t init_NXTAPI(CURL *curl_handle)
{
    cJSON *json,*obj;
    int32_t timestamp;
    char cmd[4096],dest[1024],*jsonstr;
    init_jsoncodec(0,0);
    return(0);
    sprintf(cmd,"%s=getTime",_NXTSERVER);
    while ( 1 )
    {
        while ( (jsonstr= issue_NXTPOST(curl_handle,cmd)) == 0 )
        {
            printf("error communicating to NXT network\n");
            sleep(3);
        }
        timestamp = 0;
        json = cJSON_Parse(jsonstr);
        if ( json == 0 ) printf("Error before: (%s) -> [%s]\n",jsonstr,cJSON_GetErrorPtr());
        else
        {
            obj = cJSON_GetObjectItem(json,"time");
            if ( obj != 0 )
            {
                copy_cJSON(dest,obj);
                timestamp = atoi(dest);
            }
        }
        if ( json != 0 )
            free_json(json);
        if ( timestamp > 0 )
        {
            printf("init_NXTAPI timestamp.%d\n",timestamp);
            free(jsonstr);
            return(timestamp);
        }
        printf("no time found (%s)\n",jsonstr);
        free(jsonstr);
        sleep(3);
    }
}

struct NXT_acct *add_NXT_acct(char *NXTaddr,struct NXThandler_info *mp,cJSON *obj)
{
    struct NXT_acct *ptr;
    int32_t createdflag;
    if ( obj != 0 )
    {
        copy_cJSON(NXTaddr,obj);
        ptr = get_NXTacct(&createdflag,mp,NXTaddr);
        if ( createdflag != 0 )
            return(ptr);
    }
    return(0);
}

struct NXT_assettxid *add_NXT_assettxid(struct NXT_asset **app,char *assetidstr,struct NXThandler_info *mp,cJSON *obj,char *txid,int32_t timestamp)
{
    int32_t createdflag;
    struct NXT_asset *ap;
    struct NXT_assettxid *tp;
    if ( obj != 0 )
        copy_cJSON(assetidstr,obj);
    
    // printf("add_NXT_assettxid A\n");
    *app = ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
    //printf("add_NXT_assettxid B\n");
    tp = MTadd_hashtable(&createdflag,mp->NXTasset_txids_tablep,txid);
    if ( createdflag != 0 )
    {
        tp->assetbits = ap->assetbits;
        tp->txidbits = calc_nxt64bits(txid);
        tp->timestamp = timestamp;
        printf("%d) %s t%d %s txid.%s\n",ap->num,ap->name,timestamp,assetidstr,txid);
        //if ( strcmp(ap->name,"MGW") == 0 && ap->num == 125 )
        //    while ( 1 ) sleep(1);
        if ( ap->num >= ap->max )
        {
            ap->max = ap->num + NXT_ASSETLIST_INCR;
            ap->txids = realloc(ap->txids,sizeof(*ap->txids) * ap->max);
        }
        ap->txids[ap->num++] = tp;
        return(tp);
    }
    return(0);
}

int32_t addto_account_txlist(struct NXT_acct *acct,int32_t ind,struct NXT_assettxid *tp)
{
    if ( acct->txlists[ind] == 0 || acct->txlists[ind]->num >= acct->txlists[ind]->max )
    {
        if ( acct->txlists[ind] == 0 )
            acct->txlists[ind] = calloc(1,sizeof(*acct->txlists[ind]));
        acct->txlists[ind]->max = acct->txlists[ind]->num + NXT_ASSETLIST_INCR;
        acct->txlists[ind]->txids = realloc(acct->txlists[ind]->txids,sizeof(*acct->txlists[ind]->txids) * acct->txlists[ind]->max);
    }
    acct->txlists[ind]->txids[acct->txlists[ind]->num++] = tp;
    return(acct->txlists[ind]->num);
}

int32_t get_asset_in_acct(struct NXT_acct *acct,struct NXT_asset *ap,int32_t createflag)
{
    int32_t i;
    uint64_t assetbits = ap->assetbits;
    for (i=0; i<acct->numassets; i++)
    {
        if ( acct->assets[i]->assetbits == assetbits )  // need to dereference as hashtables get resized
            return(i);
    }
    if ( createflag != 0 )
    {
        if ( acct->numassets >= acct->maxassets )
        {
            acct->maxassets = acct->numassets + NXT_ASSETLIST_INCR;
            acct->txlists = realloc(acct->txlists,sizeof(*acct->txlists) * acct->maxassets);
            acct->assets = realloc(acct->assets,sizeof(*acct->assets) * acct->maxassets);
            acct->quantities = realloc(acct->quantities,sizeof(*acct->quantities) * acct->maxassets);
        }
        acct->assets[acct->numassets] = ap;
        acct->txlists[acct->numassets] = 0;
        acct->quantities[acct->numassets] = 0;
        return(acct->numassets++);
    }
    else return(-1);
}

struct NXT_assettxid_list *get_asset_txlist(struct NXThandler_info *mp,struct NXT_acct *np,char *assetidstr)
{
    int32_t ind,createdflag;
    struct NXT_asset *ap;
    if ( assetidstr == 0 || assetidstr[0] == 0 || strcmp(assetidstr,ILLEGAL_COINASSET) == 0 )
        return(0);
    //printf("get_asset_txlist\n");
    ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
    ind = get_asset_in_acct(np,ap,0);
    if ( ind >= 0 )
        return(np->txlists[ind]);
    else return(0);
}

struct NXT_assettxid *update_assettxid_list(char *sender,char *receiver,char *assetidstr,char *txid,int32_t timestamp,cJSON *argjson)
{
    int32_t ind,createdflag,iter;
    struct NXT_acct *np;
    struct NXT_asset *ap;
    struct NXT_assettxid *tp;
    tp = add_NXT_assettxid(&ap,assetidstr,Global_mp,0,txid,timestamp);
    if ( tp != 0 )
    {
        tp->receiverbits = calc_nxt64bits(receiver);
        tp->senderbits = calc_nxt64bits(sender);
        if ( argjson != 0 )
        {
            tp->comment = cJSON_Print(argjson);
            stripwhite_ns(tp->comment,strlen(tp->comment));
        }
        //tp->quantity = 0;
        //if ( wp->cointxid[0] != 0 )
        //    tp->cointxid = clonestr(wp->cointxid);
        //tp->price = wp->amount;
        tp->timestamp = timestamp;
        for (iter=0; iter<2; iter++)
        {
            np = get_NXTacct(&createdflag,Global_mp,iter==0?sender:receiver);
            ind = get_asset_in_acct(np,ap,1);
            if ( ind >= 0 )
                addto_account_txlist(np,ind,tp);
            else printf("ERROR get_asset_in_acct ind.%d\n",ind);
        }
    } else printf("duplicate asset txid.%s\n",txid);
    return(tp);
}

void calc_NXTcointxid(char *NXTcointxid,char *cointxid,int32_t vout)
{
    uint64_t hashval;
    hashval = calc_decimalhash(cointxid);
    if ( vout < 0 )
        hashval = ~hashval;
    else hashval ^= (1L << vout);
    expand_nxt64bits(NXTcointxid,hashval);
}

#ifdef BTC_COINID
struct NXT_assettxid *search_cointxid(int32_t coinid,char *NXTaddr,char *cointxid,int32_t vout)
{
    char *assetid_str();
    char NXTcointxid[64];
    int32_t createdflag,ind,i,n;
    struct NXT_acct *np;
    struct NXT_asset *ap;
    struct NXT_assettxid **txids,*tp;
    if ( cointxid[0] == 0 || NXTaddr[0] == 0 )
    {
        printf("search_cointxid: cointxid.(%s) vout.%d NXT.%s\n",cointxid,vout,NXTaddr);
        return(0);
    }
    calc_NXTcointxid(NXTcointxid,cointxid,vout);
    np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
    ap = get_NXTasset(&createdflag,Global_mp,assetid_str(coinid));
    ind = get_asset_in_acct(np,ap,0);
    if ( ind >= 0 )
    {
        txids = np->txlists[ind]->txids;
        n = np->txlists[ind]->num;
        for (i=0; i<n; i++)
        {
            tp = txids[i];
            if ( strcmp(tp->H.txid,NXTcointxid) == 0 )
                return(tp);
        }
    }
    return(0);
}
#endif

int32_t construct_tokenized_req(char *tokenized,char *cmdjson,char *NXTACCTSECRET)
{
    char encoded[NXT_TOKEN_LEN+1];
    stripwhite_ns(cmdjson,strlen(cmdjson));
    issue_generateToken(0,encoded,cmdjson,NXTACCTSECRET);
    encoded[NXT_TOKEN_LEN] = 0;
    sprintf(tokenized,"[%s,{\"token\":\"%s\"}]",cmdjson,encoded);
    return((int32_t)strlen(tokenized)+1);
    // printf("(%s) -> (%s) _tokbuf.[%s]\n",NXTaddr,otherNXTaddr,_tokbuf);
}

int32_t parse_ipaddr(char *ipaddr,char *ip_port)
{
    int32_t j,port = 0;
    if ( ip_port != 0 && ip_port[0] != 0 )
    {
        strcpy(ipaddr,ip_port);
        for (j=0; ipaddr[j]!=0&&j<60; j++)
            if ( ipaddr[j] == ':' )
            {
                port = atoi(ipaddr+j+1);
                break;
            }
        ipaddr[j] = 0;
    } else strcpy(ipaddr,"127.0.0.1");
    return(port);
}

int32_t notlocalip(char *ipaddr)
{
    if ( ipaddr == 0 || ipaddr[0] == 0 || strcmp("127.0.0.1",ipaddr) == 0 || strncmp("192.168",ipaddr,7) == 0 )
        return(0);
    else return(1);
}

int32_t is_remote_access(char *previpaddr)
{
    if ( notlocalip(previpaddr) != 0 )
        return(1);
    else return(0);
}

uint32_t calc_ipbits(char *ip_port)
{
    char ipaddr[64];
    parse_ipaddr(ipaddr,ip_port);
    return(inet_addr(ipaddr));
  /*  int32_t a,b,c,d;
    sscanf(ipaddr,"%d.%d.%d.%d",&a,&b,&c,&d);
    if ( a < 0 || a > 0xff || b < 0 || b > 0xff || c < 0 || c > 0xff || d < 0 || d > 0xff )
    {
        printf("malformed ipaddr?.(%s) -> %d %d %d %d\n",ipaddr,a,b,c,d);
    }
#ifdef WIN32
    return((a<<24) | (b<<16) | (c<<8) | d);
#else
    return((d<<24) | (c<<16) | (b<<8) | a);
#endif*/
}

void expand_ipbits(char *ipaddr,uint32_t ipbits)
{
    struct sockaddr_in addr;
    memcpy(&addr.sin_addr,&ipbits,sizeof(ipbits));
    strcpy(ipaddr,inet_ntoa(addr.sin_addr));
    //sprintf(ipaddr,"%d.%d.%d.%d",(ipbits>>24)&0xff,(ipbits>>16)&0xff,(ipbits>>8)&0xff,(ipbits&0xff));
}

int32_t is_illegal_ipaddr(char *ipaddr)
{
    char tmp[64];
    expand_ipbits(tmp,calc_ipbits(ipaddr));
    return(strcmp(tmp,ipaddr));
}

char *ipbits_str(uint32_t ipbits)
{
    static char ipaddr[32];
    expand_ipbits(ipaddr,ipbits);
    return(ipaddr);
}

char *ipbits_str2(uint32_t ipbits)
{
    static char ipaddr[32];
    expand_ipbits(ipaddr,ipbits);
    return(ipaddr);
}

struct sockaddr_in conv_ipbits(uint32_t ipbits,int32_t port)
{
    char ipaddr[32];
    struct hostent *host;
    struct sockaddr_in server_addr;
    expand_ipbits(ipaddr,ipbits);
    host = (struct hostent *)gethostbyname(ipaddr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    memset(&(server_addr.sin_zero),0,8);
    return(server_addr);
}

/*uint32_t _calc_xorsum(uint32_t *ipbits,int32_t n)
{
    uint32_t i,xorsum = 0;
    if ( ipbits != 0 && n > 0 )
    {
        for (i=0; i<n; i++)
            if ( ipbits[i] != 0 )
                xorsum ^= ipbits[i];
    }
    return(xorsum);
}

uint32_t calc_xorsum(struct peerinfo **peers,int32_t n)
{
    uint32_t i,xorsum = 0;
    if ( peers != 0 && n > 0 )
    {
        for (i=0; i<n; i++)
            if ( peers[i] != 0 )
                xorsum ^= peers[i]->srv.ipbits;
    }
    return(xorsum);
}*/

struct nodestats *get_nodestats(uint64_t nxt64bits)
{
    struct nodestats *stats = 0;
    int32_t createdflag;
    struct NXT_acct *np;
    char NXTaddr[64];
    if ( nxt64bits != 0 )
    {
        expand_nxt64bits(NXTaddr,nxt64bits);
        np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
        stats = &np->stats;
        if ( stats->nxt64bits == 0 )
            stats->nxt64bits = nxt64bits;
    }
    return(stats);
}

struct pserver_info *get_pserver(int32_t *createdp,char *ipaddr,uint16_t supernet_port,uint16_t p2pport)
{
    int32_t createdflag = 0;
    struct nodestats *stats;
    struct pserver_info *pserver;
    if ( createdp == 0 )
        createdp = &createdflag;
    pserver = MTadd_hashtable(createdp,Global_mp->Pservers_tablep,ipaddr);
    if ( supernet_port != 0 )
        pserver->port = supernet_port;
    if ( (stats= get_nodestats(pserver->nxt64bits)) != 0 )
    {
        if ( *createdp != 0 || (supernet_port != 0 && supernet_port != BTCD_PORT && supernet_port != stats->supernet_port) )
            stats->supernet_port = supernet_port;
        if ( *createdp != 0 || (p2pport != 0 && p2pport != SUPERNET_PORT && p2pport != stats->p2pport) )
            stats->p2pport = p2pport;
    }
    return(pserver);
}


//#ifndef WIN32

//#endif


int64_t microseconds(void)
{
    static struct timeval timeval;
    gettimeofday(&timeval,0);
    return(timeval.tv_sec*1000000 + timeval.tv_usec);
}

int32_t bitweight(uint64_t x)
{
    int32_t wt,i;
	wt = 0;
	for (i=0; i<64; i++)
	{
		if ( (x & 1) != 0 )
			wt++;
		x >>= 1;
		if ( x == 0 )
			break;
	}
	return(wt);
}

void add_nxt64bits_json(cJSON *json,char *field,uint64_t nxt64bits)
{
    cJSON *obj;
    char numstr[64];
    if ( nxt64bits != 0 )
    {
        expand_nxt64bits(numstr,nxt64bits);
        obj = cJSON_CreateString(numstr);
        cJSON_AddItemToObject(json,field,obj);
    }
}

void launch_app_in_new_terminal(char *appname,int argc,char **argv)
{
#ifdef __APPLE__
    FILE *fp;
    int i;
    char cmd[2048];
    if ( (fp= fopen("/tmp/launchit","w")) != 0 )
    {
        fprintf(fp,"osascript <<END\n");
        fprintf(fp,"tell application \"Terminal\"\n");
        fprintf(fp,"do script \"cd \\\"`pwd`\\\";$1;exit\"\n");
        fprintf(fp,"end tell\n");
        fprintf(fp,"END\n");
        fclose(fp);
        system("chmod +x /tmp/launchit");
        sprintf(cmd,"/tmp/launchit \"%s ",appname);
        for (i=0; argv[i]!=0; i++)
            sprintf(cmd+strlen(cmd),"%s ",argv[i]);
        strcat(cmd,"\"");
        printf("cmd.(%s)\n",cmd);
        system(cmd);
    }
#else
    //void *punch_client_glue(void *argv);
    //if ( portable_thread_create(punch_client_glue,argv) == 0 )
    //    printf("ERROR punch_client_glue\n");
#endif
}

int search_uint32_ts(int32_t *ints,int32_t val)
{
    int i;
    for (i=0; ints[i]>=0; i++)
        if ( val == ints[i] )
            return(0);
    return(-1);
}

#ifdef WIN32
#ifdef __MINGW32__
#elif __MINGW64__
#else
void usleep(int utimeout)
{
    utimeout /= 1000;
    if ( utimeout == 0 )
        utimeout = 1;
    Sleep(utimeout);
}

void sleep(int seconds)
{
    Sleep(seconds * 1000);
}
#endif
#endif

int is_printable(const char *s)
{
    if ( s == 0 || *s == 0 )
        return(0);
    for (; *s; ++s)
        if (!isprint(*s))
            return 0;
    return 1;
}

uint32_t calc_file_crc(uint64_t *filesizep,char *fname)
{
    void *ptr;
    uint32_t totalcrc = 0;
#ifdef WIN32
    FILE *fp;
    *filesizep = 0;
    if ( (fp= fopen(fname,"rb")) != 0 )
    {
        fseek(fp,0,SEEK_END);
        *filesizep = ftell(fp);
        rewind(fp);
        ptr = malloc((long)*filesizep);
        fread(ptr,1,(long)*filesizep,fp);
        fclose(fp);
        totalcrc = _crc32(0,ptr,(long)*filesizep);
    }
#else
    uint32_t enablewrite = 0;
    ptr = map_file(fname,filesizep,enablewrite);
    if ( ptr != 0 )
    {
        totalcrc = _crc32(0,ptr,(long)*filesizep);
        release_map_file(ptr,(long)*filesizep);
    }
#endif
    return(totalcrc);
}

cJSON *parse_json_AM(struct json_AM *ap)
{
    char *jsontxt;
    if ( ap->jsonflag != 0 )
    {
        jsontxt = (ap->jsonflag == 1) ? ap->U.jsonstr : decode_json(&ap->U.jsn,ap->jsonflag);
        if ( jsontxt != 0 )
        {
            if ( jsontxt[0] == '"' && jsontxt[strlen(jsontxt)-1] == '"' )
                unstringify(jsontxt);
            return(cJSON_Parse(jsontxt));
        }
    }
    return(0);
}

int32_t get_API_int(cJSON *obj,int32_t val)
{
    char buf[1024];
    if ( obj != 0 )
    {
        copy_cJSON(buf,obj);
        val = atoi(buf);
    }
    return(val);
}

uint32_t get_API_uint(cJSON *obj,uint32_t val)
{
    char buf[1024];
    if ( obj != 0 )
    {
        copy_cJSON(buf,obj);
        val = atoi(buf);
    }
    return(val);
}

uint64_t get_API_nxt64bits(cJSON *obj)
{
    uint64_t nxt64bits = 0;
    char buf[1024];
    if ( obj != 0 )
    {
        copy_cJSON(buf,obj);
        nxt64bits = calc_nxt64bits(buf);
    }
    return(nxt64bits);
}

double get_API_float(cJSON *obj)
{
    double val = 0.;
    char buf[1024];
    if ( obj != 0 )
    {
        copy_cJSON(buf,obj);
        val = atof(buf);
    }
    return(val);
}

int32_t is_valid_NXTtxid(char *txid)
{
    long i,len;
    if ( txid == 0 )
        return(0);
    len = strlen(txid);
    if ( len < 6 )
        return(0);
    for (i=0; i<len; i++)
        if ( txid[i] < '0' || txid[i] > '9' )
            break;
    if ( i != len )
        return(0);
    return(1);
}

void ensure_directory(char *dirname) // jl777: does this work in windows?
{
    FILE *fp;
    char fname[512],cmd[512];
    sprintf(fname,"%s/tmp",dirname);
    if ( (fp= fopen(fname,"rb")) == 0 )
    {
        sprintf(cmd,"mkdir %s",dirname);
        if ( system(cmd) != 0 )
            printf("error making subdirectory (%s) %s (%s)\n",cmd,dirname,fname);
        fp = fopen(fname,"wb");
        if ( fp != 0 )
            fclose(fp);
    }
    else fclose(fp);
}

int32_t gen_tokenjson(CURL *curl_handle,char *jsonstr,char *NXTaddr,long nonce,char *NXTACCTSECRET,char *ipaddr,uint32_t port)
{
    int32_t createdflag;
    struct NXT_acct *np;
    char argstr[1024],pubkey[1024],token[1024];
    np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
    init_hexbytes_noT(pubkey,np->stats.pubkey,sizeof(np->stats.pubkey));
    sprintf(argstr,"{\"NXT\":\"%s\",\"pubkey\":\"%s\",\"time\":%ld,\"yourip\":\"%s\",\"uport\":%d}",NXTaddr,pubkey,nonce,ipaddr,port);
    //printf("got argstr.(%s)\n",argstr);
    issue_generateToken(curl_handle,token,argstr,NXTACCTSECRET);
    token[NXT_TOKEN_LEN] = 0;
    sprintf(jsonstr,"[%s,{\"token\":\"%s\"}]",argstr,token);
    //printf("tokenized.(%s)\n",jsonstr);
    return((int32_t)strlen(jsonstr));
}

int32_t validate_token(CURL *curl_handle,char *pubkey,char *NXTaddr,char *tokenizedtxt,int32_t strictflag)
{
    cJSON *array=0,*firstitem=0,*tokenobj,*obj;
    int64_t timeval,diff = 0;
    int32_t valid,retcode = -13;
    char buf[4096],sender[64],*firstjsontxt = 0;
    unsigned char encoded[4096];
    array = cJSON_Parse(tokenizedtxt);
    if ( array == 0 )
    {
        printf("couldnt validate.(%s)\n",tokenizedtxt);
        return(-2);
    }
    if ( is_cJSON_Array(array) != 0 && cJSON_GetArraySize(array) == 2 )
    {
        firstitem = cJSON_GetArrayItem(array,0);
        if ( pubkey != 0 )
        {
            obj = cJSON_GetObjectItem(firstitem,"pubkey");
            copy_cJSON(pubkey,obj);
        }
        obj = cJSON_GetObjectItem(firstitem,"NXT"), copy_cJSON(buf,obj);
        if ( NXTaddr[0] != 0 && strcmp(buf,NXTaddr) != 0 )
            retcode = -3;
        else
        {
            strcpy(NXTaddr,buf);
            if ( strictflag != 0 )
            {
                timeval = get_cJSON_int(firstitem,"time");
                diff = timeval - time(NULL);
                if ( diff < 0 )
                    diff = -diff;
                if ( diff > strictflag )
                {
                    printf("time diff %lld too big %lld vs %ld\n",(long long)diff,(long long)timeval,time(NULL));
                    retcode = -5;
                }
            }
            if ( retcode != -5 )
            {
                firstjsontxt = cJSON_Print(firstitem), stripwhite_ns(firstjsontxt,strlen(firstjsontxt));
                tokenobj = cJSON_GetArrayItem(array,1);
                obj = cJSON_GetObjectItem(tokenobj,"token");
                copy_cJSON((char *)encoded,obj);
                memset(sender,0,sizeof(sender));
                valid = -1;
                if ( issue_decodeToken(curl_handle,sender,&valid,firstjsontxt,encoded) > 0 )
                {
                    if ( NXTaddr[0] == 0 )
                        strcpy(NXTaddr,sender);
                    if ( strcmp(sender,NXTaddr) == 0 )
                    {
                        printf("signed by valid NXT.%s valid.%d diff.%lld\n",sender,valid,(long long)diff);
                        retcode = valid;
                    }
                    else
                    {
                        printf("diff sender.(%s) vs NXTaddr.(%s)\n",sender,NXTaddr);
                        if ( strcmp(NXTaddr,buf) == 0 )
                            retcode = valid;
                    }
                }
                if ( retcode < 0 )
                    printf("err: signed by invalid sender.(%s) NXT.%s valid.%d or timediff too big diff.%lld, buf.(%s)\n",sender,NXTaddr,valid,(long long)diff,buf);
                free(firstjsontxt);
            }
        }
    }
    if ( array != 0 )
        free_json(array);
    return(retcode);
}

int32_t update_pubkey(unsigned char refpubkey[crypto_box_PUBLICKEYBYTES],char *pubkeystr)
{
    unsigned char pubkey[crypto_box_PUBLICKEYBYTES];
    int32_t updatedflag = 0;
    if ( refpubkey != 0 && pubkeystr != 0 && pubkeystr[0] != 0 )
    {
        memset(pubkey,0,sizeof(pubkey));
        decode_hex(pubkey,(int32_t)sizeof(pubkey),pubkeystr);
        if ( memcmp(refpubkey,pubkey,sizeof(pubkey)) != 0 )
        {
            memcpy(refpubkey,pubkey,sizeof(pubkey));
            updatedflag = 1;
        }
    }
    return(updatedflag);
}

char *verify_tokenized_json(unsigned char *pubkey,char *sender,int32_t *validp,cJSON *json)
{
    long len;
    unsigned char encoded[MAX_JSON_FIELD+1];
    char NXTaddr[MAX_JSON_FIELD],pubkeystr[MAX_JSON_FIELD],*parmstxt = 0;
    cJSON *secondobj,*tokenobj,*parmsobj,*pubkeyobj;
    *validp = -2;
    sender[0] = 0;
    if ( (json->type&0xff) == cJSON_Array && cJSON_GetArraySize(json) == 2 )
    {
        parmsobj = cJSON_GetArrayItem(json,0);
        copy_cJSON(NXTaddr,cJSON_GetObjectItem(parmsobj,"NXT"));
        secondobj = cJSON_GetArrayItem(json,1);
        tokenobj = cJSON_GetObjectItem(secondobj,"token");
        copy_cJSON((char *)encoded,tokenobj);
        parmstxt = cJSON_Print(parmsobj);
        len = strlen(parmstxt);
        stripwhite_ns(parmstxt,len);
        
        if ( strlen((char *)encoded) == NXT_TOKEN_LEN )
            issue_decodeToken(0,sender,validp,parmstxt,encoded);
        if ( *validp <= 0 )
            printf("sender.(%s) vs (%s) valid.%d website.(%s) encoded.(%s) len.%ld\n",sender,NXTaddr,*validp,parmstxt,encoded,strlen((char *)encoded));
        else
        {
            if ( sender[0] != 0 && strcmp(sender,NXTaddr) != 0 )
                *validp = -1;
            if ( pubkey != 0 && *validp > 0 && (pubkeyobj=cJSON_GetObjectItem(parmsobj,"pubkey")) != 0 )
            {
                copy_cJSON(pubkeystr,pubkeyobj);
                if ( pubkeystr[0] != 0 )
                    update_pubkey(pubkey,pubkeystr);
            }
        }
        return(parmstxt);
    }
    else
    {
        char *str = cJSON_Print(json);
        printf("verify_tokenized_json not array of 2 (%s)\n",str);
        free(str);
        //while ( 1 )
        //    sleep(1);
    }
    return(0);
}

int64_t get_asset_quantity(int64_t *unconfirmedp,char *NXTaddr,char *assetidstr)
{
    char cmd[2*MAX_JSON_FIELD],assetid[MAX_JSON_FIELD];
    union NXTtype retval;
    int32_t i,n,iter;
    cJSON *array,*item,*obj;
    int64_t quantity,qty;
    quantity = *unconfirmedp = 0;
    sprintf(cmd,"%s=getAccount&account=%s",_NXTSERVER,NXTaddr);
    retval = extract_NXTfield(0,0,cmd,0,0);
    if ( retval.json != 0 )
    {
        for (iter=0; iter<2; iter++)
        {
            qty = 0;
            array = cJSON_GetObjectItem(retval.json,iter==0?"assetBalances":"unconfirmedAssetBalances");
            if ( is_cJSON_Array(array) != 0 )
            {
                n = cJSON_GetArraySize(array);
                for (i=0; i<n; i++)
                {
                    item = cJSON_GetArrayItem(array,i);
                    obj = cJSON_GetObjectItem(item,"asset");
                    copy_cJSON(assetid,obj);
                    //printf("i.%d of %d: %s(%s)\n",i,n,assetid,cJSON_Print(item));
                    if ( strcmp(assetid,assetidstr) == 0 )
                    {
                        qty = get_cJSON_int(item,iter==0?"balanceQNT":"unconfirmedBalanceQNT");
                        break;
                    }
                }
            }
            if ( iter == 0 )
                quantity = qty;
            else *unconfirmedp = qty;
        }
    }
    return(quantity);
}

void copy_file(char *src,char *dest) // OS portable
{
    int c;
    FILE *srcfp,*destfp;
    if ( (srcfp= fopen(src,"rb")) != 0 )
    {
        if ( (destfp= fopen(dest,"wb")) != 0 )
        {
            while ( (c= fgetc(srcfp)) != EOF )
                fputc(c,destfp);
            fclose(destfp);
        }
        fclose(srcfp);
    }
}
#endif
