//
//  jl777.cpp
//  glue code for pNXT
//
//  Created by jimbo laptop on 7/6/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#define SATOSHIDEN 100000000L
#define dstr(x) ((double)(x) / SATOSHIDEN)
#define pNXT_SERVERA_NXTADDR "14841113963360176068"
#define pNXT_SERVERB_NXTADDR "13682911413647488545"
#define CURRENCY_DONATIONS_ADDRESS "1Gx7pfdh8aZRUU9paTc37gcXUWbYxcqbu814DgAdHxdKGAeHLdYHKS13B5SoC9j2Zv9BvkzPik53nS5nyPiiaoDqQpSs6Z1"
#define CURRENCY_ROYALTY_ADDRESS "1JnCpSjCFwTDcDwoU3BJsqUC1kn5EChEpA6Bi5kYfd1qMPCbHddDs8FD2bd2d5BvrG6MKzXLcTQ8JdmnmZ4DaLDYL6FEHv6"

#ifdef INSIDE_CCODE
#ifdef MAINNET
#define PRIVATENXT "8149788036721522865"
#else
#define PRIVATENXT "13533482370298135570"
#endif

int lwsmain(int argc,char **argv);
void *pNXT_get_wallet(char *fname,char *password);
uint64_t pNXT_sync_wallet(void *wallet);
char *pNXT_walletaddr(char *addr,void *wallet);
int32_t pNXT_startmining(void *core,void *wallet);
uint64_t pNXT_rawbalance(void *wallet);
uint64_t pNXT_confbalance(void *wallet);
int32_t pNXT_sendmoney(void *wallet,int32_t numfakes,char *dest,uint64_t amount);
uint64_t pNXT_height(void *core); // declare the wrapper function
void p2p_glue(void *p2psrv);
void rpc_server_glue(void *rpc_server);
void upnp_glue(void *upnp);
int32_t pNXT_submit_tx(void *m_core,void *wallet,char *txbytes);


struct pNXT_info
{
    void *wallet,*core,*p2psrv,*upnp,*rpc_server;
    char walletaddr[512];
};
struct pNXT_info *Global_pNXT;

int64_t get_asset_quantity(int64_t *unconfirmedp,char *NXTaddr,char *assetidstr)
{
    char *assetid_str(int32_t coinid);
    char cmd[4096],assetid[512];
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

uint64_t get_privateNXT_balance(char *NXTaddr)
{
    int64_t qty,unconfirmed;
    qty = get_asset_quantity(&unconfirmed,NXTaddr,PRIVATENXT);
    return(qty * 1000000L); // assumes 2 decimal points
}

char *get_pNXT_addr()
{
    if ( Global_pNXT != 0 && Global_pNXT->walletaddr != 0 )
        return(Global_pNXT->walletaddr);
    return("<No pNXT address>");
}

uint64_t get_pNXT_confbalance()
{
    if ( Global_pNXT != 0 && Global_pNXT->wallet != 0 )
        return(pNXT_confbalance(Global_pNXT->wallet));
    return(0);
}

uint64_t get_pNXT_rawbalance()
{
    if ( Global_pNXT != 0 && Global_pNXT->wallet != 0 )
        return(pNXT_rawbalance(Global_pNXT->wallet));
    return(0);
}

void init_pNXT(void *core,void *p2psrv,void *rpc_server,void *upnp)
{
    //uint64_t amount = 100000000000;
    struct pNXT_info *gp;
    if ( Global_pNXT == 0 )
        Global_pNXT = calloc(1,sizeof(*Global_pNXT));
    gp = Global_pNXT;
    gp->core = core; gp->p2psrv = p2psrv; gp->upnp = upnp; gp->rpc_server = rpc_server;
    if ( gp->wallet == 0 )
    {
        while ( Global_mp->NXTACCTSECRET[0] == 0 )
            sleep(1);
        gp->wallet = pNXT_get_wallet("wallet.bin",Global_mp->NXTACCTSECRET);
    }
    printf("got gp->wallet.%p\n",gp->wallet);
    if ( gp->wallet != 0 )
    {
        strcpy(gp->walletaddr,"no pNXT address");
        pNXT_walletaddr(gp->walletaddr,gp->wallet);
        printf("got walletaddr (%s)\n",gp->walletaddr);
        pNXT_submit_tx(gp->core,gp->wallet,"0001020304050607080000000000000000000000000000ff");
        printf("submit tx done\n");
        //pNXT_startmining(gp->core,gp->wallet);
        //pNXT_sendmoney(gp->wallet,0,"1Bs3GNG1ScLQ2GGoK9CMQCAxvZfiyX1JdT8cwQeHCzseSnGD5bLXGgYQkp9k3rJfhN8mJ2sVLA8zkWRoE4HSs9cJMfqxJFj",amount);
    }
}

char *redeem_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs)
{
    struct NXT_asset *assetp;
    int64_t amount,minwithdraw=0;
    int32_t i,createdflag,deadline = 720;
    uint64_t redeemtxid;
    char coinname[64],NXTaddr[64],numstr[512],comment[1024],buf[4096],*str,*retstr = 0;
    copy_cJSON(NXTaddr,objs[0]);
    copy_cJSON(coinname,objs[1]);
    copy_cJSON(numstr,objs[2]);
    copy_cJSON(comment,objs[3]);
    stripwhite(comment,strlen(comment));
    for (i=0; coinname[i]!=0; i++)
        coinname[i] = toupper(coinname[i]);
    amount = conv_floatstr(numstr);
    printf("sender.%s valid.%d: NXT.%s %s (%s)=%.8f (%s) minwithdraw %.8f\n",sender,valid,NXTaddr,str,numstr,dstr(amount),comment,dstr(minwithdraw));
    if ( strcmp(NXTaddr,sender) == 0 && amount > minwithdraw )
    {
        assetp = get_NXTasset(&createdflag,Global_mp,str);
        if ( assetp != 0 )
            amount /= assetp->mult;
        redeemtxid = issue_transferAsset(&retstr,Global_mp->curl_handle2,Global_mp->NXTACCTSECRET,NXTISSUERACCT,str,(long long)amount,MIN_NQTFEE,deadline,comment);
        if ( retstr == 0 )
            retstr = clonestr("{\"error\":\"no redeemtxid from asset transfer\"}");
        else
        {
            expand_nxt64bits(numstr,redeemtxid);
            sprintf(buf,"{\"status\":\"success\",\"redeemtxid\":\"%s\"}",numstr);
            retstr = clonestr(buf);
        }
    } else retstr = clonestr("{\"error\":\"invalid withdraw request\"}");
    return(retstr);
}

char *pNXT_json_commands(struct NXThandler_info *mp,struct pNXT_info *gp,cJSON *argjson,char *sender,int32_t valid)
{
    //static char *genDepositaddrs[] = { (char *)genDepositaddrs_func, "genDepositaddrs", "V", "NXT", "coins", 0 };
    //static char *dispNXTacct[] = { (char *)dispNXTacct_func, "dispNXTacct", "", "NXT", "coin", "assetid", 0 };
   // static char *dispcoininfo[] = { (char *)dispNXTacct_func, "dispcoininfo", "", "NXT", "coin", "nxtaddr", 0 };
    static char *redeem[] = { (char *)redeem_func, "withdraw", "V", "NXT", "amount", 0 };
    static char **commands[] = { redeem };
    int32_t i,j;
    cJSON *obj,*nxtobj,*objs[16];
    char NXTaddr[64],command[4096],**cmdinfo,*retstr;
    memset(objs,0,sizeof(objs));
    command[0] = 0;
    memset(NXTaddr,0,sizeof(NXTaddr));
    if ( argjson != 0 )
    {
        obj = cJSON_GetObjectItem(argjson,"requestType");
        nxtobj = cJSON_GetObjectItem(argjson,"NXT");
        copy_cJSON(NXTaddr,nxtobj);
        copy_cJSON(command,obj);
        //printf("(%s) command.(%s) NXT.(%s)\n",cJSON_Print(argjson),command,NXTaddr);
    }
    printf("pNXT_json_commands sender.(%s) valid.%d | size.%d\n",sender,valid,(int32_t)(sizeof(commands)/sizeof(*commands)));
    for (i=0; i<(int32_t)(sizeof(commands)/sizeof(*commands)); i++)
    {
        cmdinfo = commands[i];
        printf("needvalid.(%c) sender.(%s) valid.%d %d of %d: cmd.(%s) vs command.(%s)\n",cmdinfo[2][0],sender,valid,i,(int32_t)(sizeof(commands)/sizeof(*commands)),cmdinfo[1],command);
        if ( strcmp(cmdinfo[1],command) == 0 )
        {
            if ( cmdinfo[2][0] != 0 )
            {
                if ( sender[0] == 0 || valid != 1 || strcmp(NXTaddr,sender) != 0 )
                {
                    printf("verification valid.%d missing for %s sender.(%s) vs NXT.(%s)\n",valid,cmdinfo[1],NXTaddr,sender);
                    return(0);
                }
            }
            for (j=3; cmdinfo[j]!=0&&j<3+(int32_t)(sizeof(objs)/sizeof(*objs)); j++)
                objs[j-3] = cJSON_GetObjectItem(argjson,cmdinfo[j]);
            retstr = (*(json_handler)cmdinfo[0])(sender,valid,objs,j-3);
            if ( retstr != 0 )
                printf("json_handler returns.(%s)\n",retstr);
            return(retstr);
        }
    }
    return(0);
}

char *pNXT_jsonhandler(cJSON *argjson,char *argstr)
{
    struct NXThandler_info *mp = Global_mp;
    long len;
    int32_t valid,firsttime = 1;
    cJSON *tokenobj,*secondobj,*json,*parmsobj = 0;
    char sender[64],*parmstxt=0,encoded[NXT_TOKEN_LEN+1],*retstr = 0;
again:
    sender[0] = 0;
    valid = -1;
    // printf("pNXT_jsonhandler argjson.%p\n",argjson);
    if ( argjson != 0 )
    {
        parmstxt = cJSON_Print(argjson);
        len = strlen(parmstxt);
        stripwhite(parmstxt,len);
    }
    if ( argjson == 0 )
    {
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json,"error",cJSON_CreateString("cant parse"));
        cJSON_AddItemToObject(json,"argstr",cJSON_CreateString(argstr));
        retstr = cJSON_Print(json);
        free_json(json);
        return(retstr);
    }
    else if ( (argjson->type&0xff) == cJSON_Array && cJSON_GetArraySize(argjson) == 2 )
    {
        parmsobj = cJSON_GetArrayItem(argjson,0);
        if ( parmstxt != 0 )
            free(parmstxt);
        parmstxt = cJSON_Print(parmsobj);
        len = strlen(parmstxt);
        stripwhite(parmstxt,len);
        
        secondobj = cJSON_GetArrayItem(argjson,1);
        tokenobj = cJSON_GetObjectItem(secondobj,"token");
        copy_cJSON(encoded,tokenobj);
        //printf("website.(%s) encoded.(%s) len.%ld\n",parmstxt,encoded,strlen(encoded));
        if ( strlen(encoded) == NXT_TOKEN_LEN )
            issue_decodeToken(Global_mp->curl_handle2,sender,&valid,parmstxt,encoded);
        argjson = parmsobj;
    }
    retstr = pNXT_json_commands(mp,Global_pNXT,argjson,sender,valid);
    printf("back from pNXT_json_commands\n");
    if ( firsttime != 0 && retstr == 0 && argjson != 0 && argjson != parmsobj )
    {
        char _tokbuf[2048];
        firsttime = 0;
        issue_generateToken(mp->curl_handle2,encoded,parmstxt,mp->NXTACCTSECRET);
        encoded[NXT_TOKEN_LEN] = 0;
        sprintf(_tokbuf,"[%s,{\"token\":\"%s\"}]",parmstxt,encoded);
        argjson = cJSON_Parse(_tokbuf);
        printf("%s arg.%p\n",_tokbuf,argjson);
        goto again;
    }
    if ( parmstxt != 0 )
        free(parmstxt);
    return(retstr);
}

/*char *pNXT_jsonhandler(cJSON *argjson)
{
    char *jsonstr;
    if ( argjson != 0 )
    {
        jsonstr = cJSON_Print(argjson);
        printf("pNXT_jsonhandler.(%s)\n",jsonstr);
        return(jsonstr);
    }
    return(0);
}*/

void process_pNXT_AM(struct pNXT_info *dp,struct NXT_protocol_parms *parms)
{
    cJSON *argjson;
    struct json_AM *ap;
    char NXTaddr[64],*sender,*receiver;
    sender = parms->sender; receiver = parms->receiver; ap = parms->AMptr;
    expand_nxt64bits(NXTaddr,ap->H.nxt64bits);
    if ( strcmp(NXTaddr,sender) != 0 )
    {
        printf("unexpected NXTaddr %s != sender.%s when receiver.%s\n",NXTaddr,sender,receiver);
        return;
    }
    if ( (argjson = parse_json_AM(ap)) != 0 )
    {
        printf("process_pNXT_AM got jsontxt.(%s)\n",ap->jsonstr);
        free_json(argjson);
    }
}

void process_pNXT_typematch(struct pNXT_info *dp,struct NXT_protocol_parms *parms)
{
    char NXTaddr[64],*sender,*receiver,*txid;
    sender = parms->sender; receiver = parms->receiver; txid = parms->txid;
    safecopy(NXTaddr,sender,sizeof(NXTaddr));
    printf("got txid.(%s) type.%d subtype.%d sender.(%s) -> (%s)\n",txid,parms->type,parms->subtype,sender,receiver);
}

void *pNXT_handler(struct NXThandler_info *mp,struct NXT_protocol_parms *parms,void *handlerdata,int32_t height)
{
    struct pNXT_info *gp = handlerdata;
    if ( parms->txid == 0 )     // indicates non-transaction event
    {
        if ( parms->mode == NXTPROTOCOL_WEBJSON )
            return(pNXT_jsonhandler(parms->argjson,parms->argstr));
        else if ( parms->mode == NXTPROTOCOL_NEWBLOCK )
        {
            printf("pNXT Height: %lld | %s raw %.8f confirmed %.8f |",(long long)pNXT_height(gp->core),gp->walletaddr!=0?gp->walletaddr:"no wallet address",dstr(pNXT_rawbalance(gp->wallet)),dstr(pNXT_confbalance(gp->wallet)));
            pNXT_sendmoney(gp->wallet,0,gp->walletaddr,12345678);

            printf("pNXT new RTblock %d time %ld microseconds %lld\n",mp->RTflag,time(0),(long long)microseconds());
        }
        else if ( parms->mode == NXTPROTOCOL_IDLETIME )
        {
            //printf("pNXT new idletime %d time %ld microseconds %lld \n",mp->RTflag,time(0),(long long)microseconds());
        }
        else if ( parms->mode == NXTPROTOCOL_INIT )
        {
            printf("pNXT NXThandler_info init %d\n",mp->RTflag);
            if ( Global_pNXT == 0 )
                Global_pNXT = calloc(1,sizeof(*Global_pNXT));
            gp = Global_pNXT;
        }
        return(gp);
    }
    else if ( parms->mode == NXTPROTOCOL_AMTXID )
        process_pNXT_AM(gp,parms);
    else if ( parms->mode == NXTPROTOCOL_TYPEMATCH )
        process_pNXT_typematch(gp,parms);
    return(gp);
}

#ifdef FROM_pNXT
void _init_lws(void *arg)
{
    void *core,*p2psrv,*rpc_server,*upnp,**ptrs = (void **)arg;
    sleep(3);
    core = ptrs[0]; p2psrv = ptrs[1]; rpc_server = ptrs[2]; upnp = ptrs[3];
    p2p_glue(p2psrv);
    rpc_server_glue(rpc_server);
    upnp_glue(upnp);
    init_pNXT(core,p2psrv,rpc_server,upnp);
    printf("finished call lwsmain pNXT.(%p) height.%lld | %p %p %p\n",ptrs[0],(long long)pNXT_height(core),p2psrv,rpc_server,upnp);
}

void _init_lws2(void *arg)
{
    char *argv[2];
    argv[0] = "from_init_lws";
    argv[1] = 0;
    printf("call lwsmain\n");
    lwsmain(1,argv);
}

void init_lws(void *core,void *p2p,void *rpc_server,void *upnp)
{
    static void *ptrs[4];
    ptrs[0] = core; ptrs[1] = p2p; ptrs[2] = rpc_server; ptrs[3] = upnp;
    printf("init_lws(%p %p %p %p)\n",core,p2p,rpc_server,upnp);
    if ( portable_thread_create(_init_lws2,ptrs) == 0 )
        printf("ERROR launching _init_lws2\n");
    printf("done init_lws2()\n");
    if ( portable_thread_create(_init_lws,ptrs) == 0 )
        printf("ERROR launching _init_lws\n");
    printf("done init_lws()\n");
}
#else
void *pNXT_get_wallet(char *fname,char *password){return(0);}
uint64_t pNXT_sync_wallet(void *wallet){return(0);}
char *pNXT_walletaddr(char *addr,void *wallet){return(0);}
int32_t pNXT_startmining(void *core,void *wallet){return(0);}
uint64_t pNXT_rawbalance(void *wallet){return(0);}
uint64_t pNXT_confbalance(void *wallet){return(0);}
int32_t pNXT_sendmoney(void *wallet,int32_t numfakes,char *dest,uint64_t amount){return(0);}
uint64_t pNXT_height(void *core){return(0);}
int32_t pNXT_submit_tx(void *m_core,void *wallet,char *txbytes){return(0);}

#endif

#else

#define INSIDE_DAEMON
#include "simplewallet/password_container.cpp"
#include "simplewallet/simplewallet.cpp"
extern "C" void init_lws(currency::core *,void *,void *,void *);

extern "C" currency::simple_wallet *pNXT_get_wallet(char *fname,char *password)
{
    currency::simple_wallet *wallet = new(currency::simple_wallet);
    if ( wallet->open_wallet(fname,password) == 0 )
        if ( wallet->new_wallet(fname,password) == 0 )
            free(wallet), wallet = 0;
    if ( wallet != 0 )
        wallet->load_blocks();
    return(wallet);
}

extern "C" void pNXT_sync_wallet(currency::simple_wallet *wallet)
{
    wallet->sync_wallet();
}

extern "C" char *pNXT_walletaddr(char *walletaddr,currency::simple_wallet *wallet)
{
    std::string addr;
    currency::account_public_address acct;
    addr = wallet->get_address(acct);
    printf("inside got wallet addr.(%s)\n",addr.c_str());
    strcpy(walletaddr,addr.c_str());
    return(walletaddr);
}

extern "C" int32_t pNXT_startmining(currency::core *core,currency::simple_wallet *wallet)
{
    int numthreads = 1;
    std::string addr;
    currency::account_public_address acct;
    wallet->sync_wallet();
    addr = wallet->get_address(acct);
    if ( core->get_miner().start(acct,numthreads) == 0 )
    {
        printf("Failed, mining not started for (%s)\n",addr.c_str());
        return(-1);
    }
    else
    {
        wallet->show_balance();
   }
    return(0);
}

extern "C" uint64_t pNXT_rawbalance(currency::simple_wallet *wallet)
{
    return(wallet->get_rawbalance());
}

extern "C" uint64_t pNXT_confbalance(currency::simple_wallet *wallet)
{
    return(wallet->get_confbalance());
}

extern "C" bool pNXT_sendmoney(currency::simple_wallet *wallet,int32_t numfakes,char *dest,uint64_t amount)
{
    std::vector<std::string> args;
    char buf[512];
    args.reserve(3);
    wallet->sync_wallet();
    printf("sending %.8f pNXT to (%s)  ",dstr(amount),dest);
    wallet->show_balance();
    sprintf(buf,"%d",numfakes);
    args.push_back(buf);
    args.push_back(dest);
    sprintf(buf,"%.8f",(double)amount/SATOSHIDEN);
    args.push_back(buf);
    return(wallet->transfer(args));
}

extern "C" uint64_t pNXT_height(currency::core *m)
{
    return(m->get_current_blockchain_height());
}

extern "C" void p2p_glue(nodetool::node_server<currency::t_currency_protocol_handler<currency::core> >* p2psrv)
{
    printf("p2p_glue.%p port.%d\n",p2psrv,p2psrv->get_this_peer_port());
}

extern "C" void rpc_server_glue(currency::core_rpc_server *rpc_server)
{
    printf("rpc_server_glue.%p\n",rpc_server);
}

extern "C" void upnp_glue(tools::miniupnp_helper *upnp)
{
    printf("upnp_glue.%p: lan_addr.(%s)\n",upnp,upnp->pub_lanaddr);
}

extern "C" int32_t pNXT_submit_tx(currency::core *m_core,currency::simple_wallet *wallet,char *txbytes)
{
    int i;
    blobdata txb,b;
    transaction tx = AUTO_VAL_INIT(tx);
    NOTIFY_NEW_TRANSACTIONS::request req;
    currency_connection_context fake_context = AUTO_VAL_INIT(fake_context);
    tx_verification_context tvc = AUTO_VAL_INIT(tvc);
    tx.version = 0;
    //txb = tx_to_blob(tx);
    txb.erase();
    for (i=0; i<4; i++)
        txb.push_back(0);
    for (i=0; txbytes[i]!=0; t++)
        txb.push_back(i);

    if ( !m_core->handle_incoming_tx(txb,tvc,false) )
    {
        LOG_PRINT_L0("[on_send_raw_tx]: Failed to process tx");
        return -2;
    }
    if ( tvc.m_verifivation_failed )
    {
        LOG_PRINT_L0("[on_send_raw_tx]: tx verification failed");
        return -3;
    }
    if( !tvc.m_should_be_relayed )
    {
        LOG_PRINT_L0("[on_send_raw_tx]: tx accepted, but not relayed");
        return -4;
    }
    req.txs.push_back(txb);
    m_core->get_protocol()->relay_transactions(req,fake_context);
    return(0);
}


#endif
