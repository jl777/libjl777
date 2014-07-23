//
//  libwebsocketsglue.h
//  Created by jl777, April 5th, 2014
//  MIT License
//


#ifndef gateway_dispstr_h
#define gateway_dispstr_h

int URL_changed;
char dispstr[65536];
char testforms[1024*1024],PC_USERNAME[512],MY_IPADDR[512];
char NXTPROTOCOL_HTMLFILE[512] = { "/tmp/NXTprotocol.html" };
int Finished_loading,Historical_done;
#define pNXT_SIG 0x99999999

#include "../NXTservices/NXTservices.c"
uv_loop_t *UV_loop;

//#include "../../NXTservices/InstantDEX/InstantDEX.h"
//#include "../../NXTservices/multigateway/multigateway.h"
//#include "../../NXTservices/NXTorrent.h"
//#include "../../NXTservices/NXTsubatomic.h"
//#include "../../NXTservices/NXTcoinsco.h"
//#include "NXTmixer.h"
//#include "../NXTservices/NXTprivacy.h"
#include "../NXTservices/html.h"

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

void init_NXTservices(int _argc,char **_argv)
{
    struct NXThandler_info *mp = calloc(1,sizeof(*mp));    // seems safest place to have main data structure
    Global_mp = mp;
    char *ipaddr;
    UV_loop = uv_default_loop();
    portable_mutex_init(&mp->hash_mutex);
    portable_mutex_init(&mp->hashtable_queue[0].mutex);
    portable_mutex_init(&mp->hashtable_queue[1].mutex);
    curl_global_init(CURL_GLOBAL_ALL); //init the curl session
    mp->curl_handle = curl_easy_init();
    mp->curl_handle2 = curl_easy_init();
    mp->curl_handle3 = curl_easy_init();
  
    init_NXThashtables(mp);
    init_NXTAPI(mp->curl_handle);
    ipaddr = 0;//get_ipaddr();
    if ( ipaddr != 0 )
    {
        strcpy(MY_IPADDR,get_ipaddr());
        strcpy(mp->ipaddr,MY_IPADDR);
    }
    if ( _argc >= 2 && (strcmp(_argv[1],"server") == 0 || strcmp(mp->ipaddr,EMERGENCY_PUNCH_SERVER) == 0) )
    {
#ifndef WIN32
        //punch_server_main(_argc-1,_argv+1);
        printf("hole punch server done\n");
        exit(0);
#endif
    }
    else
    {
        safecopy(mp->ipaddr,MY_IPADDR,sizeof(mp->ipaddr));
        mp->upollseconds = 333333 * 0;
        mp->pollseconds = POLL_SECONDS;
        //safecopy(mp->NXTAPISERVER,NXTSERVER,sizeof(mp->NXTAPISERVER));
        crypto_box_keypair(Global_mp->session_pubkey,Global_mp->session_privkey);
        init_hexbytes(Global_mp->pubkeystr,Global_mp->session_pubkey,sizeof(Global_mp->session_pubkey));
        //mp->accountjson = issue_getAccountInfo(mp->curl_handle,&Global_mp->acctbalance,mp->dispname,PC_USERNAME,mp->NXTADDR,mp->groupname);
#ifdef __linux__
        //char NXTADDR[64],NXTACCTSECRET[256];
        //gen_randomacct(mp->curl_handle,33,NXTADDR,NXTACCTSECRET,"randvals");
        //printf("(%s) (%s) (%s) (%s) [%s]\n",mp->dispname,PC_USERNAME,NXTADDR,mp->groupname,NXTACCTSECRET);
#endif
        //mp->myind = -1;
        //mp->nxt64bits = calc_nxt64bits(mp->NXTADDR);
        if ( portable_thread_create(process_hashtablequeues,mp) == 0 )
            printf("ERROR hist process_hashtablequeues\n");
        if ( portable_thread_create(getNXTblocks,mp) == 0 )
            printf("ERROR start_Histloop\n");
        if ( portable_thread_create(init_NXTprivacy,_argv[1]) == 0 )
            printf("ERROR init_NXTprivacy\n");
        gen_testforms(_argc>1 ? _argv[1] : 0);

        printf("run_NXTservices >>>>>>>>>>>>>>> %p %s: %s %s\n",mp,mp->dispname,PC_USERNAME,mp->ipaddr);
        void run_NXTservices(void *arg);
        if ( portable_thread_create(run_NXTservices,mp) == 0 )
            printf("ERROR hist process_hashtablequeues\n");
    }
 }


#endif
