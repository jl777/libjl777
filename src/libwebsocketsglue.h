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

#include "../NXTservices/NXTservices.c"
uv_loop_t *UV_loop;

char *changeurl_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs)
{
    extern char NXTPROTOCOL_HTMLFILE[512];
    char URL[64],*retstr = 0;
    copy_cJSON(URL,objs[0]);
    if ( URL[0] != 0 )
    {
        URL_changed = 1;
        strcpy(NXTPROTOCOL_HTMLFILE,URL);
        testforms[0] = 0;
        retstr = clonestr(URL);
    }
    return(retstr);
}

//#include "../../NXTservices/InstantDEX/InstantDEX.h"
//#include "../../NXTservices/multigateway/multigateway.h"
//#include "../../NXTservices/NXTorrent.h"
//#include "../../NXTservices/NXTsubatomic.h"
//#include "../../NXTservices/NXTcoinsco.h"
//#include "NXTmixer.h"
#include "../NXTservices/html.h"

#define STUB_SIG 0x99999999
struct stub_info { char privatdata[10000]; };
struct stub_info *Global_stub;

char *stub_jsonhandler(cJSON *argjson)
{
    char *jsonstr;
    if ( argjson != 0 )
    {
        jsonstr = cJSON_Print(argjson);
        printf("stub_jsonhandler.(%s)\n",jsonstr);
        return(jsonstr);
    }
    return(0);
}

void process_stub_AM(struct stub_info *dp,struct NXT_protocol_parms *parms)
{
    cJSON *argjson;
   // char *jsontxt;
    struct json_AM *ap;
    char NXTaddr[64],*sender,*receiver;
    sender = parms->sender; receiver = parms->receiver; ap = parms->AMptr; //txid = parms->txid;
    expand_nxt64bits(NXTaddr,ap->H.nxt64bits);
    if ( strcmp(NXTaddr,sender) != 0 )
    {
        printf("unexpected NXTaddr %s != sender.%s when receiver.%s\n",NXTaddr,sender,receiver);
        return;
    }
    if ( (argjson = parse_json_AM(ap)) != 0 )
    {
        printf("process_stub_AM got jsontxt.(%s)\n",ap->jsonstr);
        free_json(argjson);
    }
}

void process_stub_typematch(struct stub_info *dp,struct NXT_protocol_parms *parms)
{
    char NXTaddr[64],*sender,*receiver,*txid;
    sender = parms->sender; receiver = parms->receiver; txid = parms->txid;
    safecopy(NXTaddr,sender,sizeof(NXTaddr));
    printf("got txid.(%s) type.%d subtype.%d sender.(%s) -> (%s)\n",txid,parms->type,parms->subtype,sender,receiver);
}

void *stub_handler(struct NXThandler_info *mp,struct NXT_protocol_parms *parms,void *handlerdata)
{
    struct stub_info *dp = handlerdata;
    if ( parms->txid == 0 )     // indicates non-transaction event
    {
        if ( parms->mode == NXTPROTOCOL_WEBJSON )
            return(stub_jsonhandler(parms->argjson));
        else if ( parms->mode == NXTPROTOCOL_NEWBLOCK )
            printf("stub new RTblock %d time %ld microseconds %lld\n",mp->RTflag,time(0),(long long)microseconds());
        else if ( parms->mode == NXTPROTOCOL_IDLETIME )
                 printf("stub new idletime %d time %ld microseconds %lld \n",mp->RTflag,time(0),(long long)microseconds());
        else if ( parms->mode == NXTPROTOCOL_INIT )
        {
            printf("stub NXThandler_info init %d\n",mp->RTflag);
            dp = Global_stub = calloc(1,sizeof(*Global_stub));
        }
        return(dp);
    }
    else if ( parms->mode == NXTPROTOCOL_AMTXID )
        process_stub_AM(dp,parms);
    else if ( parms->mode == NXTPROTOCOL_TYPEMATCH )
        process_stub_typematch(dp,parms);
    return(dp);
}

void NXTservices_idler(uv_idle_t *handle)
{
    static int64_t nexttime;
    //int32_t process_syncmem_queue();
    void call_handlers(struct NXThandler_info *mp,int32_t mode,int32_t height);
    static uint64_t counter;
    usleep(1000);
    if ( (counter++ % 1000) == 0 && microseconds() > nexttime )
    {
        call_handlers(Global_mp,NXTPROTOCOL_IDLETIME,0);
        nexttime = (microseconds() + 1000000);
    }
    //while ( process_syncmem_queue() > 0 )
    //    ;
    //process_UDPsend_queue();
}

void run_UVloop(void *arg)
{
    uv_idle_t idler;
    uv_idle_init(UV_loop,&idler);
    uv_idle_start(&idler,NXTservices_idler);
    //multicast_test();
    uv_run(UV_loop,UV_RUN_DEFAULT);
    printf("end of uv_run\n");
}

void run_NXTservices(void *arg)
{
    struct NXThandler_info *mp = arg;
    //static char *whitelist[] = { NXTISSUERACCT, NXTACCTA, NXTACCTB, NXTACCTC, NXTACCTD, NXTACCTE, "" };
    
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
    /*argv[argc++] = "punch";
    stripwhite(mp->dispname,strlen(mp->dispname));
    argv[argc++] = "-u", argv[argc++] = mp->dispname;
    argv[argc++] = "-n", argv[argc++] = mp->NXTADDR;
    if ( mp->groupname[0] != 0 )
        argv[argc++] = "-g", argv[argc++] = mp->groupname;*/
    init_NXThashtables(mp);
    if ( portable_thread_create(process_hashtablequeues,mp) == 0 )
        printf("ERROR hist process_hashtablequeues\n");
/*#ifndef MAINNET
    //printf("run_UVloop\n");
    //if ( portable_thread_create(run_UVloop,mp) == 0 )
     //   printf("ERROR run_UVloop\n");
    if ( portable_thread_create(init_NXTprivacy,EMERGENCY_PUNCH_SERVER) == 0 )
        printf("ERROR run_NXTsync\n");
    printf("after init_NXTprivacy\n");
while ( 1 )
    sleep(3);
    register_NXT_handler("NXTcoinsco",mp,2,NXTPROTOCOL_ILLEGALTYPE,NXTcoinsco_handler,NXTCOINSCO_SIG,1,0,0);
    return;
#endif*/
   if ( portable_thread_create(getNXTblocks,mp) == 0 )
        printf("ERROR start_Histloop\n");
    if ( 1 )
    {
        printf(">>>>>>>>>>>>>>> %s: %s %s NXT.(%s)\n",mp->dispname,PC_USERNAME,mp->ipaddr,mp->NXTADDR);
#ifdef MAINNET
        if ( get_gatewayid(mp->ipaddr) < 0 )
        {
            if ( 0 && portable_thread_create(run_NXTsync,EMERGENCY_PUNCH_SERVER) == 0 )
                printf("ERROR run_NXTsync\n");
            sleep(3);
            register_NXT_handler("NXTcoinsco",mp,NXTPROTOCOL_ILLEGALTYPE,NXTPROTOCOL_ILLEGALTYPE,NXTcoinsco_handler,NXTCOINSCO_SIG,1,0,0);
            if ( 1 )
            {
                register_NXT_handler("NXTorrent",mp,NXTPROTOCOL_ILLEGALTYPE,NXTPROTOCOL_ILLEGALTYPE,NXTorrent_handler,NXTORRENT_SIG,1,0,0);
                register_NXT_handler("subatomic",mp,NXTPROTOCOL_ILLEGALTYPE,NXTPROTOCOL_ILLEGALTYPE,subatomic_handler,SUBATOMIC_SIG,1,0,0);
                register_NXT_handler("multigateway",mp,2,-1,multigateway_handler,GATEWAY_SIG,1,0,whitelist);
            }
        }
        else
        {
            if ( 0 && portable_thread_create(run_NXTsync,EMERGENCY_PUNCH_SERVER) == 0 )
                printf("ERROR run_NXTsync\n");
            sleep(3);
            register_NXT_handler("multigateway",mp,2,-1,multigateway_handler,GATEWAY_SIG,1,0,whitelist);
        }
#else
        //if ( get_gatewayid(mp->ipaddr) < 0 )
        {
            //register_NXT_handler("NXTcoinsco",mp,NXTPROTOCOL_ILLEGALTYPE,NXTPROTOCOL_ILLEGALTYPE,NXTcoinsco_handler,NXTCOINSCO_SIG,1,0,0);
        }
        //else
        {
            //register_NXT_handler("NXTcoinsco",mp,NXTPROTOCOL_ILLEGALTYPE,NXTPROTOCOL_ILLEGALTYPE,NXTcoinsco_handler,NXTCOINSCO_SIG,1,0,0);
            //register_NXT_handler("multigateway",mp,2,-1,multigateway_handler,GATEWAY_SIG,1,0,whitelist);
        }
#endif
    }
    //else register_NXT_handler("NXTcoinsco",mp,2,NXTPROTOCOL_ILLEGALTYPE,NXTcoinsco_handler,NXTCOINSCO_SIG,1,0,0);

    while ( Finished_loading == 0 )
        sleep(1);
    init_assets(mp);
    printf("start_NXTloops\n");
    portable_mutex_init(&mp->hash_mutex);
    portable_mutex_init(&mp->hashtable_queue[0].mutex);
    portable_mutex_init(&mp->hashtable_queue[1].mutex);
    //portable_mutex_init(&UDPsend_queue.mutex);
    NXTloop(mp);
    printf("start_NXTloops done\n");
    while ( 1 ) sleep(60);
}

void init_NXTservices(int _argc,char **_argv)
{
    struct NXThandler_info *mp = calloc(1,sizeof(*mp));    // seems safest place to have main data structure
    Global_mp = mp;
    cJSON *json;
    char *ipaddr,*jsonstr,descr[4096];
    UV_loop = uv_default_loop();
    curl_global_init(CURL_GLOBAL_ALL); //init the curl session
    mp->curl_handle = curl_easy_init();
    mp->curl_handle2 = curl_easy_init();
    mp->curl_handle3 = curl_easy_init();
    init_NXTAPI(mp->curl_handle);
    ipaddr = get_ipaddr();
    if ( ipaddr != 0 )
    {
        strcpy(MY_IPADDR,get_ipaddr());
        strcpy(mp->ipaddr,MY_IPADDR);
    }
    if ( _argc >= 4 && strcmp(_argv[1],"DIV") == 0 )
    {
        strcpy(EX_DIVIDEND_ASSETID,_argv[2]);
        if ( EX_DIVIDEND_ASSETID[0] == 0 )
            strcpy(EX_DIVIDEND_ASSETID,"all");
        else
        {
            jsonstr = issue_getAsset(mp->curl_handle,EX_DIVIDEND_ASSETID);
            if ( (json= cJSON_Parse(jsonstr)) != 0 )
            {
                if ( extract_cJSON_str(descr,sizeof(descr),json,"description") > 0 )
                {
                    EX_DIVIDEND_DESCRIPTION = clonestr(descr);
                    EX_DIVIDEND_DECIMALS = (int32_t)get_cJSON_int(json,"decimals");
                } else
                    free_json(json);
            }
        }
        EX_DIVIDEND_HEIGHT = atoi(_argv[3]);
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
        printf("run_NXTservices\n");
        if ( portable_thread_create(run_NXTservices,mp) == 0 )
            printf("ERROR hist process_hashtablequeues\n");
        while ( Finished_loading == 0 )
            sleep(1);
        printf("run_UVloop\n");
        if ( portable_thread_create(run_UVloop,mp) == 0 )
            printf("ERROR hist process_hashtablequeues\n");
    }
 }


#endif
