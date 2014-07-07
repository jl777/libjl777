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

#include "../NXTservices/NXTservices.c"
uv_loop_t *UV_loop;

//#include "../../NXTservices/InstantDEX/InstantDEX.h"
//#include "../../NXTservices/multigateway/multigateway.h"
//#include "../../NXTservices/NXTorrent.h"
//#include "../../NXTservices/NXTsubatomic.h"
//#include "../../NXTservices/NXTcoinsco.h"
//#include "NXTmixer.h"
#include "../NXTservices/html.h"

#define pNXT_SIG 0x99999999
struct pNXT_info { char privatdata[10000]; };
struct pNXT_info *Global_pNXT;

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
    struct pNXT_info *dp = handlerdata;
    if ( parms->txid == 0 )     // indicates non-transaction event
    {
        if ( parms->mode == NXTPROTOCOL_WEBJSON )
            return(pNXT_jsonhandler(parms->argjson));
        else if ( parms->mode == NXTPROTOCOL_NEWBLOCK )
            printf("pNXT new RTblock %d time %ld microseconds %lld\n",mp->RTflag,time(0),(long long)microseconds());
        else if ( parms->mode == NXTPROTOCOL_IDLETIME )
        {
            //printf("pNXT new idletime %d time %ld microseconds %lld \n",mp->RTflag,time(0),(long long)microseconds());
        }
        else if ( parms->mode == NXTPROTOCOL_INIT )
        {
            printf("pNXT NXThandler_info init %d\n",mp->RTflag);
            dp = Global_pNXT = calloc(1,sizeof(*Global_pNXT));
        }
        return(dp);
    }
    else if ( parms->mode == NXTPROTOCOL_AMTXID )
        process_pNXT_AM(dp,parms);
    else if ( parms->mode == NXTPROTOCOL_TYPEMATCH )
        process_pNXT_typematch(dp,parms);
    return(dp);
}

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
    uv_idle_start(&idler,NXTservices_idler);
    uv_run(UV_loop,UV_RUN_DEFAULT);
    printf("end of uv_run\n");
}

void run_NXTservices(void *arg)
{
    struct NXThandler_info *mp = arg;
    register_NXT_handler("pNXT",mp,2,NXTPROTOCOL_ILLEGALTYPE,pNXT_handler,pNXT_SIG,1,0,0);
    NXTloop(mp);
    printf("start_NXTloops done\n");
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
    ipaddr = get_ipaddr();
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
        safecopy(mp->NXTAPISERVER,NXTSERVER,sizeof(mp->NXTAPISERVER));
        gen_randomacct(mp->curl_handle,33,mp->NXTADDR,mp->NXTACCTSECRET,"randvals");
        crypto_box_keypair(Global_mp->session_pubkey,Global_mp->session_privkey);
        init_hexbytes(Global_mp->pubkeystr,Global_mp->session_pubkey,sizeof(Global_mp->session_pubkey));
        mp->accountjson = issue_getAccountInfo(mp->curl_handle,&Global_mp->acctbalance,mp->dispname,PC_USERNAME,mp->NXTADDR,mp->groupname);
        printf("(%s) (%s) (%s) (%s) [%s]\n",mp->dispname,PC_USERNAME,mp->NXTADDR,mp->groupname,mp->NXTACCTSECRET);
        mp->myind = -1;
        mp->nxt64bits = calc_nxt64bits(mp->NXTADDR);
        if ( portable_thread_create(process_hashtablequeues,mp) == 0 )
            printf("ERROR hist process_hashtablequeues\n");
        if ( portable_thread_create(getNXTblocks,mp) == 0 )
            printf("ERROR start_Histloop\n");
        gen_testforms();

        printf("run_NXTservices >>>>>>>>>>>>>>> %s: %s %s NXT.(%s)\n",mp->dispname,PC_USERNAME,mp->ipaddr,mp->NXTADDR);
        if ( portable_thread_create(run_NXTservices,mp) == 0 )
            printf("ERROR hist process_hashtablequeues\n");
    }
 }


#endif
