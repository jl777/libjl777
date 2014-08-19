//
//  libwebsocketsglue.h
//  Created by jl777, April 5th, 2014
//  MIT License
//


#ifndef gateway_dispstr_h
#define gateway_dispstr_h

int URL_changed,testimagelen;
char dispstr[65536];
char testforms[1024*1024],PC_USERNAME[512],MY_IPADDR[512];
char NXTPROTOCOL_HTMLFILE[512] = { "/tmp/NXTprotocol.html" };
int Finished_loading,Historical_done;
#define pNXT_SIG 0x99999999

#include "NXTservices.c"
uv_loop_t *UV_loop;

//#include "../../NXTservices/InstantDEX/InstantDEX.h"
//#include "../../NXTservices/multigateway/multigateway.h"
//#include "../../NXTservices/NXTorrent.h"
//#include "../../NXTservices/NXTsubatomic.h"
//#include "../../NXTservices/NXTcoinsco.h"
//#include "NXTmixer.h"
//#include "../NXTservices/NXTprivacy.h"
#include "html.h"

void NXTservices_idler(uv_idle_t *handle)
{
    static int64_t nexttime;
    //void call_handlers(struct NXThandler_info *mp,int32_t mode,int32_t height);
    static uint64_t counter;
    usleep(1000);
    if ( (counter++ % 1000) == 0 && microseconds() > nexttime )
    {
        call_handlers(Global_mp,NXTPROTOCOL_IDLETIME,0);
        nexttime = (microseconds() + 1000000);
    }
}

void run_UVloop(void *arg)
{
    uv_idle_t idler;
    uv_idle_init(UV_loop,&idler);
    //uv_idle_init(UV_loop,&idler);
    uv_idle_start(&idler,NXTprivacy_idler);
    uv_idle_start(&idler,NXTservices_idler);
    uv_run(UV_loop,UV_RUN_DEFAULT);
    printf("end of uv_run\n");
}

void run_NXTservices(void *arg)
{
    void *pNXT_handler(struct NXThandler_info *mp,struct NXT_protocol_parms *parms,void *handlerdata,int32_t height);
    struct NXThandler_info *mp = arg;
    printf("inside run_NXTservices %p\n",mp);
    register_NXT_handler("pNXT",mp,2,NXTPROTOCOL_ILLEGALTYPE,pNXT_handler,pNXT_SIG,1,0,0);
    printf("NXTloop\n");
    NXTloop(mp);
    printf("NXTloop done\n");
    while ( 1 ) sleep(60);
}

void init_NXTservices(char *JSON_or_fname)
{
    struct NXThandler_info *mp = Global_mp;    // seems safest place to have main data structure
    char *ipaddr;
    UV_loop = uv_default_loop();
    portable_mutex_init(&mp->hash_mutex);
    portable_mutex_init(&mp->hashtable_queue[0].mutex);
    portable_mutex_init(&mp->hashtable_queue[1].mutex);
 
    init_NXThashtables(mp);
    init_NXTAPI(mp->curl_handle);
    ipaddr = 0;
    if ( ipaddr != 0 )
    {
        strcpy(MY_IPADDR,get_ipaddr());
        strcpy(mp->ipaddr,MY_IPADDR);
    }
    safecopy(mp->ipaddr,MY_IPADDR,sizeof(mp->ipaddr));
    mp->upollseconds = 333333 * 0;
    mp->pollseconds = POLL_SECONDS;
    crypto_box_keypair(Global_mp->session_pubkey,Global_mp->session_privkey);
    init_hexbytes(Global_mp->pubkeystr,Global_mp->session_pubkey,sizeof(Global_mp->session_pubkey));
    if ( portable_thread_create(process_hashtablequeues,mp) == 0 )
        printf("ERROR hist process_hashtablequeues\n");
    init_MGWconf(JSON_or_fname);
    printf("start getNXTblocks\n");
    if ( portable_thread_create(getNXTblocks,mp) == 0 )
        printf("ERROR start_Histloop\n");
    printf("start init_NXTprivacy\n");
    if ( portable_thread_create(init_NXTprivacy,"") == 0 )
        printf("ERROR init_NXTprivacy\n");
    printf("start gen_testforms\n");
    gen_testforms(0);
    
    printf("run_NXTservices >>>>>>>>>>>>>>> %p %s: %s %s\n",mp,mp->dispname,PC_USERNAME,mp->ipaddr);
    void run_NXTservices(void *arg);
    if ( portable_thread_create(run_NXTservices,mp) == 0 )
        printf("ERROR hist process_hashtablequeues\n");
    void *Coinloop(void *arg);
    printf("start Coinloop\n");
    if ( portable_thread_create(Coinloop,mp) == 0 )
        printf("ERROR Coin_genaddrloop\n");
}


#endif
