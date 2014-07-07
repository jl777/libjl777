//
//  jl777.cpp
//  glue code for pNXT
//
//  Created by jimbo laptop on 7/6/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#define SATOSHIDEN 100000000L

#ifdef INSIDE_CCODE
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


struct pNXT_info
{
    void *wallet,*core,*p2psrv,*upnp,*rpc_server;
    char walletaddr[512];
};
struct pNXT_info *Global_pNXT;

char *get_pNXT_addr()
{
    if ( Global_pNXT != 0 && Global_pNXT->walletaddr != 0 )
        return(Global_pNXT->walletaddr);
    return("No pNXT address");
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
    //uint64_t amount = 12345678;
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
        pNXT_startmining(gp->core,gp->wallet);
        //pNXT_sendmoney(gp->wallet,0,gp->walletaddr,amount);
    }
}

char *pNXT_jsonhandler(cJSON *argjson)
{
    char *jsonstr;
    if ( argjson != 0 )
    {
        jsonstr = cJSON_Print(argjson);
        printf("pNXT_jsonhandler.(%s)\n",jsonstr);
        return(jsonstr);
    }
    return(0);
}

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
            return(pNXT_jsonhandler(parms->argjson));
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
        printf("core.%p: start mining (%s) ",core,pNXT_walletaddr(core,wallet));
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
    printf("sending %.8f pNXT to (%s) from %s ",dstr(amount),dest,pNXT_walletaddr(core,wallet));
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
#endif
