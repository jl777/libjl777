//
//  jl777.cpp
//  glue code for pNXT
//
//  Created by jimbo laptop on 7/6/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#define SATOSHIDEN 100000000L

#ifdef INSIDE_CCODE

void *pNXT_get_wallet(char *fname,char *password);
uint64_t pNXT_sync_wallet(void *wallet);
const char *pNXT_get_walletaddr(void *wallet);
int32_t pNXT_startmining(void *core,void *wallet);
uint64_t pNXT_rawbalance(void *wallet);
uint64_t pNXT_confbalance(void *wallet);
int32_t pNXT_sendmoney(void *wallet,int32_t numfakes,char *dest,uint64_t amount);
uint64_t pNXT_height(void *core); // declare the wrapper function
void p2p_glue(void *p2psrv);
void rpc_server_glue(void *rpc_server);
void upnp_glue(void *upnp);

void _init_lws(void *arg)
{
    void *core,*p2psrv,*rpc_server,*upnp,**ptrs = (void **)arg;
    sleep(3);
    core = ptrs[0]; p2psrv = ptrs[1]; rpc_server = ptrs[2]; upnp = ptrs[3];
    p2p_glue(p2psrv);
    rpc_server_glue(rpc_server);
    upnp_glue(upnp);
    printf("call lwsmain pNXT.(%p) height.%lld | %p %p %p\n",ptrs[0],(long long)pNXT_height(core),p2psrv,rpc_server,upnp);
    while ( 1 )
    {
        sleep(60);
        printf("Height: %lld\n",(long long)pNXT_height(core));
    }
}

void _init_lws2(void *arg)
{
    char *argv[2];
    argv[0] = "from_init_lws";
    argv[1] = 0;
    printf("call lwsmain\n");
    lwsmain(1,argv);
}

void init_lws(void *core,void *p2p,void *cprotocol,void *upnp)
{
    static void *ptrs[4];
    ptrs[0] = core; ptrs[1] = p2p; ptrs[2] = cprotocol; ptrs[3] = upnp;
    printf("init_lws(%p %p %p %p)\n",core,p2p,cprotocol,upnp);
    if ( portable_thread_create(_init_lws2,ptrs) == 0 )
        printf("ERROR launching _init_lws2\n");
    printf("done init_lws2()\n");
    if ( portable_thread_create(_init_lws,ptrs) == 0 )
        printf("ERROR launching _init_lws\n");
    printf("done init_lws()\n");
}
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

extern "C" const char *pNXT_get_walletaddr(currency::simple_wallet *wallet)
{
    std::string addr;
    currency::account_public_address acct;
    addr = wallet->get_address(acct);
    return(addr.c_str());
}

extern "C" int32_t pNXT_startmining(currency::core *m,currency::simple_wallet *wallet)
{
    int numthreads = 1;
    std::string addr;
    currency::account_public_address acct;
    addr = wallet->get_address(acct);
    if ( m->get_miner().start(acct,numthreads) == 0 )
    {
        printf("Failed, mining not started for (%s)\n",addr.c_str());
        return(-1);
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
    sprintf(buf,"%d",numfakes);
    args.push_back(buf);
    args.push_back(dest);
    sprintf(buf,"%.8f",(double)amount/SATOSHIDEN);
    args.push_back(buf);
    return(wallet->transfer(args));
}

extern "C" uint64_t pNXT_height(currency::core *m)
{
    /*static int didinit;
    int numthreads = 1;
    std::string addr;
    currency::account_public_address apa;
    if ( didinit == 0 )
    {
        if ( wallet.open_wallet("wallet.test","password") == 0 )
            wallet.new_wallet("wallet.test","password");
        addr = wallet.get_address(apa);
        wallet.load_blocks();
        wallet.show_balance();
        if ( m->get_miner().start(apa,numthreads) == 0 )
        {
            printf("Failed, mining not started for (%s)\n",addr.c_str());
        }
        printf("core.%p: start mining (%s) balance %.8f %.8f\n",m,addr.c_str(),(double)wallet.get_confbalance()/100000000L,(double)wallet.get_rawbalance()/100000000L);
        didinit = 1;
    }
    else
    {
        wallet.idle_time();
    }*/
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
