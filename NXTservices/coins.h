//
//  coins.h
//  gateway
//
//  Created by jl777 on 7/19/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef gateway_coins_h
#define gateway_coins_h

int32_t Numcoins;
struct coin_info **Daemons;

struct coin_info *get_coin_info(char *coinstr)
{
    int32_t i;
    for (i=0; i<Numcoins; i++)
        if ( strcmp(coinstr,Daemons[i]->name) == 0 )
            return(Daemons[i]);
    return(0);
}

char *get_marker(char *coinstr)
{
    struct coin_info *cp;
    if ( strcmp(coinstr,"NXT") == 0 )
        return("8712042175100667547"); // NXTprivacy
    else if ( strcmp(coinstr,"BTC") == 0 )
        return("177MRHRjAxCZc7Sr5NViqHRivDu1sNwkHZ");
    else if ( strcmp(coinstr,"BTCD") == 0 )
        return("RMMGbxZdav3cRJmWScNVX6BJivn6BNbBE8");
    else if ( strcmp(coinstr,"LTC") == 0 )
        return("LUERp4v5abpTk9jkuQh3KFxc4mFSGTMCiz");
    else if ( strcmp(coinstr,"DRK") == 0 )
        return("XmxSWLPA92QyAXxw2FfYFFex6QgBhadv2Q");
    else if ( (cp= get_coin_info(coinstr)) != 0 )
        return(cp->marker);
    else return(0);
}

char *get_assetid_str(char *coinstr)
{
    struct coin_info *cp;
    if ( (cp= get_coin_info(coinstr)) != 0 && cp->assetid[0] != 0 )
        return(cp->assetid);
    return(0);
}

uint64_t get_assetidbits(char *coinstr)
{
    struct coin_info *cp;
    if ( (cp= get_coin_info(coinstr)) != 0 && cp->assetid[0] != 0 )
        return(calc_nxt64bits(cp->assetid));
    return(0);
}

struct coin_info *conv_assetid(char *assetid)
{
    int32_t i;
    for (i=0; i<Numcoins; i++)
        if ( strcmp(Daemons[i]->assetid,assetid) == 0 )
            return(Daemons[i]);
    return(0);
}

uint64_t get_orderbook_assetid(char *coinstr)
{
    struct coin_info *cp;
    int32_t i;
    uint64_t virtassetid;
    if ( strcmp(coinstr,"NXT") == 0 )
        return(ORDERBOOK_NXTID);
    if ( (cp= get_coin_info(coinstr)) != 0 )
        return(calc_nxt64bits(cp->assetid));
    virtassetid = 0;
    for (i=0; coinstr[i]!=0; i++)
    {
        virtassetid <<= 8;
        virtassetid |= (coinstr[i] & 0xff);
    }
    return(virtassetid);
}

int32_t is_gateway_addr(char *addr)
{
    int32_t i;
    if ( strcmp(addr,NXTISSUERACCT) == 0 )
        return(1);
    for (i=0; i<256; i++)
    {
        if ( Server_NXTaddrs[i][0] == 0 )
            break;
        if ( strcmp(addr,Server_NXTaddrs[i]) == 0 )
            return(1);
    }
    return(0);
}

char *parse_conf_line(char *line,char *field)
{
    line += strlen(field);
    for (; *line!='='&&*line!=0; line++)
        break;
    if ( *line == 0 )
        return(0);
    if ( *line == '=' )
        line++;
    stripstr(line,strlen(line));
    printf("[%s]\n",line);
    return(clonestr(line));
}

char *extract_userpass(struct coin_info *cp,char *serverport,char *userpass,char *fname)
{
    FILE *fp;
    char line[1024],*rpcuser,*rpcpassword,*str;
    userpass[0] = 0;
    if ( (fp= fopen(fname,"r")) != 0 )
    {
        printf("extract_userpass from (%s)\n",fname);
        rpcuser = rpcpassword = 0;
        while ( fgets(line,sizeof(line),fp) > 0 )
        {
            if ( line[0] == '#' )
                continue;
            //printf("line.(%s) %p %p\n",line,strstr(line,"rpcuser"),strstr(line,"rpcpassword"));
            if ( (str= strstr(line,"rpcuser")) != 0 )
                rpcuser = parse_conf_line(str,"rpcuser");
            else if ( (str= strstr(line,"rpcpassword")) != 0 )
                rpcpassword = parse_conf_line(str,"rpcpassword");
        }
        if ( rpcuser != 0 && rpcpassword != 0 )
            sprintf(userpass,"%s:%s",rpcuser,rpcpassword);
        else userpass[0] = 0;
        printf("-> (%s):(%s) userpass.(%s)\n",rpcuser,rpcpassword,userpass);
        if ( rpcuser != 0 )
            free(rpcuser);
        if ( rpcpassword != 0 )
            free(rpcpassword);
    }
    else
    {
        printf("extract_userpass cant open.(%s)\n",fname);
        return(0);
    }
    return(serverport);
}

struct coin_info *create_coin_info(int32_t nohexout,int32_t useaddmultisig,int32_t estblocktime,char *name,int32_t minconfirms,double txfee,int32_t pollseconds,char *asset,char *conf_fname,char *serverport,int32_t blockheight,char *marker,double NXTfee_equiv,int32_t forkblock)
{
    struct coin_info *cp = calloc(1,sizeof(*cp));
    char userpass[512];
    if ( forkblock == 0 )
        forkblock = blockheight;
    safecopy(cp->name,name,sizeof(cp->name));
    cp->estblocktime = estblocktime;
    cp->NXTfee_equiv = (NXTfee_equiv  * SATOSHIDEN);
    safecopy(cp->assetid,asset,sizeof(cp->assetid));
    cp->marker = clonestr(marker);
    cp->blockheight = blockheight;
    cp->min_confirms = minconfirms;
    cp->markeramount = (uint64_t)(txfee * SATOSHIDEN);
    cp->txfee = (uint64_t)(txfee  * SATOSHIDEN);
    serverport = extract_userpass(cp,serverport,userpass,conf_fname);
    if ( serverport != 0 )
    {
        cp->serverport = clonestr(serverport);
        cp->userpass = clonestr(userpass);
        printf("%s userpass.(%s) -> (%s)\n",cp->name,cp->userpass,cp->serverport);
        cp->nohexout = nohexout;
        cp->use_addmultisig = useaddmultisig;
        cp->minconfirms = minconfirms;
        cp->estblocktime = estblocktime;
        cp->txfee = (uint64_t)(txfee  * SATOSHIDEN);
        cp->forkheight = forkblock;
        printf("%s minconfirms.%d txfee %.8f | marker %.8f NXTfee %.8f | firstblock.%ld fork.%d %d seconds\n",cp->name,cp->minconfirms,dstr(cp->txfee),dstr(cp->markeramount),dstr(cp->NXTfee_equiv),(long)cp->blockheight,cp->forkheight,cp->estblocktime);
    }
    else
    {
        free(cp);
        cp = 0;
    }
    return(cp);
}

struct coin_info *init_coin_info(cJSON *json,char *coinstr)
{
    int32_t useaddmultisig,nohexout,estblocktime,minconfirms,pollseconds,blockheight,forkblock;
    char asset[256],_marker[512],conf_filename[512],tradebotfname[512],serverip_port[512],*marker,*name;
    double txfee,NXTfee_equiv;
    struct coin_info *cp = 0;
    if ( json != 0 )
    {
        nohexout = get_API_int(cJSON_GetObjectItem(json,"nohexout"),0);
        useaddmultisig = get_API_int(cJSON_GetObjectItem(json,"useaddmultisig"),0);
        blockheight = get_API_int(cJSON_GetObjectItem(json,"blockheight"),0);
        forkblock = get_API_int(cJSON_GetObjectItem(json,"forkblock"),0);
        pollseconds = get_API_int(cJSON_GetObjectItem(json,"pollseconds"),60);
        minconfirms = get_API_int(cJSON_GetObjectItem(json,"minconfirms"),10);
        estblocktime = get_API_int(cJSON_GetObjectItem(json,"estblocktime"),300);
        txfee = get_API_float(cJSON_GetObjectItem(json,"txfee"));
        NXTfee_equiv = get_API_float(cJSON_GetObjectItem(json,"NXTfee_equiv"));
        if ( (marker = get_marker(coinstr)) == 0 )
        {
            extract_cJSON_str(_marker,sizeof(_marker),json,"marker");
            marker = clonestr(_marker);
        }
        if ( marker != 0 && txfee != 0. && NXTfee_equiv != 0. &&
            extract_cJSON_str(conf_filename,sizeof(conf_filename),json,"conf") > 0 &&
            extract_cJSON_str(asset,sizeof(asset),json,"asset") > 0 &&
            extract_cJSON_str(serverip_port,sizeof(serverip_port),json,"rpc") > 0 )
        {
            cp = create_coin_info(nohexout,useaddmultisig,estblocktime,name,minconfirms,txfee,pollseconds,asset,conf_filename,serverip_port,blockheight,marker,NXTfee_equiv,forkblock);
            if ( extract_cJSON_str(tradebotfname,sizeof(tradebotfname),json,"tradebotfname") > 0 )
                cp->tradebotfname = clonestr(tradebotfname);
        }
    }
    return(cp);
}

void init_MGWconf(char *NXTADDR,char *NXTACCTSECRET,struct NXThandler_info *mp)
{
    int32_t init_tradebots(cJSON *languagesobj);
    static int32_t exchangeflag;
    uint64_t nxt64bits;
    //FILE *fp;
    cJSON *array,*item,*languagesobj = 0;
    char coinstr[512],NXTaddr[64],*buf=0,*jsonstr,*origblock;
    int32_t i,n,ismainnet,timezone=0;
    int64_t len=0,allocsize=0;
    exchangeflag = !strcmp(NXTACCTSECRET,"exchanges");
    printf("init_MGWconf exchangeflag.%d\n",exchangeflag);
    //init_filtered_bufs(); crashed ubunty
    ensure_directory("backups");
    ensure_directory("backups/telepods");
    /*if ( (fp= fopen("backups/tmp","rb")) == 0 )
    {
        if ( system("mkdir backups") != 0 )
            printf("error making backup dir\n");
        fp = fopen("backups/tmp","wb");
        fclose(fp);
    }
    else fclose(fp);*/
    if ( 0 )
    {
        char *argv[1] = { "test" };
        double picoc(int argc,char **argv,char *codestr);
        picoc(1,argv,clonestr("double main(){ double val = 1.234567890123456; printf(\"hello world val %.20f\\n\",val); return(val);}"));
        getchar();
    }
    printf("load MGW.conf\n");
    jsonstr = load_file("jl777.conf",&buf,&len,&allocsize);
    if ( jsonstr != 0 )
    {
        printf("loaded.(%s)\n",jsonstr);
        MGWconf = cJSON_Parse(jsonstr);
        if ( MGWconf != 0 )
        {
            printf("parsed\n");
            timezone = get_API_int(cJSON_GetObjectItem(MGWconf,"timezone"),0);
            init_jdatetime(NXT_GENESISTIME,timezone * 3600);
            languagesobj = cJSON_GetObjectItem(MGWconf,"tradebot_languages");
            MIN_NQTFEE = get_API_int(cJSON_GetObjectItem(MGWconf,"MIN_NQTFEE"),(int32_t)MIN_NQTFEE);
            MIN_NXTCONFIRMS = get_API_int(cJSON_GetObjectItem(MGWconf,"MIN_NXTCONFIRMS"),MIN_NXTCONFIRMS);
            GATEWAY_SIG = get_API_int(cJSON_GetObjectItem(MGWconf,"GATEWAY_SIG"),0);
            extract_cJSON_str(ORIGBLOCK,sizeof(ORIGBLOCK),MGWconf,"ORIGBLOCK");
            extract_cJSON_str(NXTAPIURL,sizeof(NXTAPIURL),MGWconf,"NXTAPIURL");
            extract_cJSON_str(NXTISSUERACCT,sizeof(NXTISSUERACCT),MGWconf,"NXTISSUERACCT");
            ismainnet = get_API_int(cJSON_GetObjectItem(MGWconf,"MAINNET"),0);
            if ( ismainnet != 0 )
            {
                NXT_FORKHEIGHT = 173271;
                if ( NXTAPIURL[0] == 0 )
                    strcpy(NXTAPIURL,"http://127.0.0.1:7876/nxt");
                if ( NXTISSUERACCT[0] == 0 )
                    strcpy(NXTISSUERACCT,"7117166754336896747");
                origblock = "14398161661982498695";    //"91889681853055765";//"16787696303645624065";
            }
            else
            {
                if ( NXTAPIURL[0] == 0 )
                    strcpy(NXTAPIURL,"http://127.0.0.1:6876/nxt");
                if ( NXTISSUERACCT[0] == 0 )
                    strcpy(NXTISSUERACCT,"18232225178877143084");
                origblock = "16787696303645624065";   //"91889681853055765";//"16787696303645624065";
            }
            if ( ORIGBLOCK[0] == 0 )
                strcpy(ORIGBLOCK,origblock);
            strcpy(NXTSERVER,NXTAPIURL);
            strcat(NXTSERVER,"?requestType");
            extract_cJSON_str(Server_names[0],sizeof(Server_names[0]),MGWconf,"MGW0_ipaddr");
            extract_cJSON_str(Server_names[1],sizeof(Server_names[1]),MGWconf,"MGW1_ipaddr");
            extract_cJSON_str(Server_names[2],sizeof(Server_names[2]),MGWconf,"MGW2_ipaddr");
            extract_cJSON_str(NXTACCTSECRET,sizeof(NXTACCTSECRET),MGWconf,"secret");
            if ( NXTACCTSECRET[0] == 0 )
                gen_randomacct(0,33,NXTADDR,NXTACCTSECRET,"randvals");
            nxt64bits = issue_getAccountId(0,NXTACCTSECRET);
            expand_nxt64bits(NXTADDR,nxt64bits);
            for (i=0; i<3; i++)
                printf("%s | ",Server_names[i]);
            printf("issuer.%s %08x NXTAPIURL.%s, minNXTconfirms.%d port.%s orig.%s\n",NXTISSUERACCT,GATEWAY_SIG,NXTAPIURL,MIN_NXTCONFIRMS,SERVER_PORTSTR,ORIGBLOCK);
            array = cJSON_GetObjectItem(MGWconf,"coins");
            if ( array != 0 && is_cJSON_Array(array) != 0 )
            {
                n = cJSON_GetArraySize(array);
                for (i=0; i<n; i++)
                {
                    if ( array == 0 || n == 0 )
                        break;
                    item = cJSON_GetArrayItem(array,i);
                    copy_cJSON(coinstr,cJSON_GetObjectItem(item,"name"));
                    if ( coinstr[0] != 0 )
                    {
                        MGWcoins = realloc(MGWcoins,sizeof(*MGWcoins) * (Numcoins+1));
                        MGWcoins[Numcoins] = item;
                        Daemons[Numcoins] = init_coin_info(item,coinstr);
                        printf("i.%d coinid.%d %s asset.%s\n",i,Numcoins,coinstr,Daemons[Numcoins]->assetid);
                        Numcoins++;
                    }
                }
            }
            array = cJSON_GetObjectItem(MGWconf,"special_NXTaddrs");
            if ( array != 0 && is_cJSON_Array(array) != 0 ) // first three must be the gateway's addresses
            {
                n = cJSON_GetArraySize(array);
                for (i=0; i<n; i++)
                {
                    if ( array == 0 || n == 0 )
                        break;
                    item = cJSON_GetArrayItem(array,i);
                    copy_cJSON(NXTaddr,item);
                    if ( NXTaddr[0] == 0 )
                    {
                        printf("Illegal special NXTaddr.%d\n",i);
                        exit(1);
                    }
                    printf("%s ",NXTaddr);
                    strcpy(Server_NXTaddrs[i],NXTaddr);
                    MGW_blacklist[i] = MGW_whitelist[i] = clonestr(NXTaddr);
                }
                printf("special_addrs.%d\n",n);
                MGW_blacklist[n] = MGW_whitelist[n] = NXTISSUERACCT, n++;
                MGW_whitelist[n] = "";
                MGW_blacklist[n++] = "4551058913252105307";    // from accidental transfer
                MGW_blacklist[n++] = "";
            }
            void start_polling_exchanges(int32_t exchangeflag);
            int32_t init_exchanges(cJSON *confobj,int32_t exchangeflag);
            if ( init_exchanges(MGWconf,exchangeflag) > 0 )
                start_polling_exchanges(exchangeflag);
        }
    }
    init_tradebots(languagesobj);
    if ( ORIGBLOCK[0] == 0 )
    {
        printf("need a non-zero origblock.(%s)\n",ORIGBLOCK);
        exit(1);
    }
}
#endif
