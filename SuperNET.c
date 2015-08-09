//
//  main.c
//  libtest
//
//  Created by jl777 on 8/13/14.
//  Copyright (c) 2014 jl777. MIT License.
//


#include <stdio.h>
#include <curl/curl.h>
#define DEFINES_ONLY
#include "plugins/includes/portable777.h"
#include "plugins/utils/utils777.c"
#include "plugins/utils/files777.c"
#include "plugins/common/system777.c"
#include "plugins/agents/plugins.h"
#undef DEFINES_ONLY


#ifdef INSIDE_MGW
struct kv777 *DB_revNXTtrades,DB_NXTtrades;
#else
struct ramkv777 *DB_revNXTtrades,DB_NXTtrades;
#endif

struct pending_cgi { struct queueitem DL; char apitag[24],*jsonstr; cJSON *json; int32_t sock,retind; };

char *SuperNET_install(char *plugin,char *jsonstr,cJSON *json)
{
    char ipaddr[MAX_JSON_FIELD],path[MAX_JSON_FIELD],*str,*retstr;
    int32_t i,ind,async;
    uint16_t port,websocket;
    if ( find_daemoninfo(&ind,plugin,0,0) != 0 )
        return(clonestr("{\"error\":\"plugin already installed\"}"));
    copy_cJSON(path,cJSON_GetObjectItem(json,"path"));
    copy_cJSON(ipaddr,cJSON_GetObjectItem(json,"ipaddr"));
    port = get_API_int(cJSON_GetObjectItem(json,"port"),0);
    async = get_API_int(cJSON_GetObjectItem(json,"daemonize"),0);
    websocket = get_API_int(cJSON_GetObjectItem(json,"websocket"),0);
    str = stringifyM(jsonstr);
    retstr = language_func(plugin,ipaddr,port,websocket,async,path,str,call_system);
    for (i=0; i<3; i++)
    {
        if ( find_daemoninfo(&ind,plugin,0,0) != 0 )
            break;
        poll_daemons();
        sleep(1);
    }
    free(str);
    return(retstr);
}

int32_t got_newpeer(const char *ip_port) { if ( Debuglevel > 2 ) printf("got_newpeer.(%s)\n",ip_port); return(0); }

void *issue_cgicall(void *_ptr)
{
    char apitag[1024],plugin[1024],method[1024],*str = 0,*broadcaststr,*destNXT; uint32_t nonce; int32_t checklen,retlen,timeout; struct pending_cgi *ptr =_ptr;
    copy_cJSON(apitag,cJSON_GetObjectItem(ptr->json,"apitag"));
    safecopy(ptr->apitag,apitag,sizeof(ptr->apitag));
    copy_cJSON(plugin,cJSON_GetObjectItem(ptr->json,"agent"));
    if ( plugin[0] == 0 )
        copy_cJSON(plugin,cJSON_GetObjectItem(ptr->json,"plugin"));
    copy_cJSON(method,cJSON_GetObjectItem(ptr->json,"method"));
    timeout = get_API_int(cJSON_GetObjectItem(ptr->json,"timeout"),SUPERNET.PLUGINTIMEOUT);
    broadcaststr = cJSON_str(cJSON_GetObjectItem(ptr->json,"broadcast"));
    fprintf(stderr,"sock.%d (%s) API RECV.(%s)\n",ptr->sock,broadcaststr!=0?broadcaststr:"",ptr->jsonstr);
    if ( ptr->sock >= 0 && (ptr->retind= nn_connect(ptr->sock,apitag)) < 0 )
        fprintf(stderr,"error connecting to (%s)\n",apitag);
    else
    {
        destNXT = cJSON_str(cJSON_GetObjectItem(ptr->json,"destNXT"));
        if ( strcmp(plugin,"relay") == 0 || (broadcaststr != 0 && strcmp(broadcaststr,"publicaccess") == 0) || cJSON_str(cJSON_GetObjectItem(ptr->json,"servicename")) != 0 )
        {
            if ( Debuglevel > 1 )
                printf("call busdata_sync.(%s)\n",ptr->jsonstr);
            //printf("destNXT.(%s)\n",destNXT!=0?destNXT:"");
            str = busdata_sync(&nonce,ptr->jsonstr,broadcaststr,destNXT);
            //printf("got.(%s)\n",str);
        }
        else
        {
            if ( Debuglevel > 2 )
                fprintf(stderr,"call plugin_method.(%s)\n",ptr->jsonstr);
            str = plugin_method(ptr->sock,0,1,plugin,method,0,0,ptr->jsonstr,(int32_t)strlen(ptr->jsonstr)+1,timeout,0);
        }
        if ( str != 0 )
        {
            int32_t busdata_validate(char *forwarder,char *sender,uint32_t *timestamp,uint8_t *databuf,int32_t *datalenp,void *msg,cJSON *json);
            char forwarder[512],sender[512],methodstr[512]; uint32_t timestamp = 0; uint8_t databuf[8192]; int32_t valid=-1,datalen; cJSON *retjson,*argjson;
            forwarder[0] = sender[0] = 0;
            if ( (retjson= cJSON_Parse(str)) != 0 )
            {
                if ( is_cJSON_Array(retjson) != 0 && cJSON_GetArraySize(retjson) == 2 )
                {
                    argjson = cJSON_GetArrayItem(retjson,0);
                    copy_cJSON(methodstr,cJSON_GetObjectItem(argjson,"method"));
                    if ( strcmp(methodstr,"busdata") == 0 )
                    {
                        //fprintf(stderr,"call validate\n");
                        if ( (valid= busdata_validate(forwarder,sender,&timestamp,databuf,&datalen,str,retjson)) > 0 )
                        {
                            if ( datalen > 0 )
                            {
                                free(str);
                                str = malloc(datalen);
                                memcpy(str,databuf,datalen);
                            }
                        }
                    }
                }
                free_json(retjson);
            }
            //fprintf(stderr,"sock.%d mainstr.(%s) valid.%d sender.(%s) forwarder.(%s) time.%u\n",ptr->sock,str,valid,sender,forwarder,timestamp);
            if ( ptr->sock >= 0 )
            {
                retlen = (int32_t)strlen(str) + 1;
                if ( (checklen= nn_send(ptr->sock,str,retlen,0)) != retlen )
                    fprintf(stderr,"checklen.%d != len.%d for nn_send to (%s)\n",checklen,retlen,apitag);
                free(str), str = 0;
            }
        } else printf("null str returned\n");
    }
    if ( ptr->sock >= 0 )
    {
        //nn_freemsg(ptr->jsonstr);
        nn_shutdown(ptr->sock,ptr->retind);
        if ( str != 0 )
            free(str), str = 0;
    }
    free_json(ptr->json);
    free(ptr);
    return(str);
}

char *process_nn_message(int32_t sock,char *jsonstr)
{
    cJSON *json; struct pending_cgi *ptr; char *retstr = 0;
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        ptr = calloc(1,sizeof(*ptr));
        ptr->sock = sock;
        ptr->json = json;
        ptr->jsonstr = jsonstr;
        if ( sock >= 0 )
            portable_thread_create((void *)issue_cgicall,ptr);
        else retstr = issue_cgicall(ptr);
    } //else if ( sock >= 0 ) free(jsonstr);
    return(retstr);
}

char *process_jl777_msg(char *previpaddr,char *jsonstr,int32_t duration)
{
    char *process_user_json(char *plugin,char *method,char *cmdstr,int32_t broadcastflag,int32_t timeout);
    char buf[65536],plugin[MAX_JSON_FIELD],method[MAX_JSON_FIELD],request[MAX_JSON_FIELD],*bstr,*retstr;
    uint64_t daemonid,instanceid,tag;
    int32_t broadcastflag = 0;
    cJSON *json;
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        copy_cJSON(request,cJSON_GetObjectItem(json,"requestType"));
        copy_cJSON(plugin,cJSON_GetObjectItem(json,"plugin"));
        if ( plugin[0] == 0 )
            copy_cJSON(plugin,cJSON_GetObjectItem(json,"agent"));
        if ( strcmp(request,"install") == 0 && plugin[0] != 0 )
        {
            retstr = SuperNET_install(plugin,jsonstr,json);
            free_json(json);
            return(retstr);
        }
        tag = get_API_nxt64bits(cJSON_GetObjectItem(json,"tag"));
        daemonid = get_API_nxt64bits(cJSON_GetObjectItem(json,"daemonid"));
        instanceid = get_API_nxt64bits(cJSON_GetObjectItem(json,"instanceid"));
        copy_cJSON(method,cJSON_GetObjectItem(json,"method"));
        if ( (bstr= cJSON_str(cJSON_GetObjectItem(json,"broadcast"))) != 0 )
            broadcastflag = 1;
        else broadcastflag = 0;
        if ( method[0] == 0 )
        {
            strcpy(method,request);
            if ( plugin[0] == 0 && set_first_plugin(plugin,method) < 0 )
                return(clonestr("{\"error\":\"no method or plugin specified, search for requestType failed\"}"));
        }
        if ( strlen(jsonstr) < sizeof(buf)-1)
        {
            strcpy(buf,jsonstr);
            return(process_nn_message(-1,buf));
        }
    }
    return(clonestr("{\"error\":\"couldnt parse JSON\"}"));
}

char *SuperNET_JSON(char *jsonstr) // BTCD's entry point
{
    return(process_jl777_msg(0,jsonstr,60));
}

char *call_SuperNET_JSON(char *JSONstr) // sub-plugin's entry point
{
    char request[MAX_JSON_FIELD],name[MAX_JSON_FIELD],*retstr = 0;;
    uint64_t daemonid,instanceid;
    cJSON *json;
    if ( (json= cJSON_Parse(JSONstr)) != 0 )
    {
        copy_cJSON(request,cJSON_GetObjectItem(json,"requestType"));
        copy_cJSON(name,cJSON_GetObjectItem(json,"plugin"));
        if ( name[0] == 0 )
            copy_cJSON(name,cJSON_GetObjectItem(json,"agent"));
        if ( strcmp(request,"register") == 0 )
        {
            daemonid = get_API_nxt64bits(cJSON_GetObjectItem(json,"daemonid"));
            instanceid = get_API_nxt64bits(cJSON_GetObjectItem(json,"instanceid"));
            retstr = register_daemon(name,daemonid,instanceid,cJSON_GetObjectItem(json,"methods"),cJSON_GetObjectItem(json,"pubmethods"),cJSON_GetObjectItem(json,"authmethods"));
        } else retstr = process_jl777_msg(0,JSONstr,60);
        free_json(json);
    }
    if ( retstr == 0 )
        retstr = clonestr("{\"result\":\"call_SuperNET_JSON no response\"}");
    return(retstr);
}

char *SuperNET_url()
{
    static char url[64];
    //sprintf(urls[0],"http://127.0.0.1:%d",SUPERNET_PORT+1*0);
    sprintf(url,"https://127.0.0.1:%d",SUPERNET.port);
    return(url);
}

void SuperNET_loop(void *ipaddr)
{
    char *strs[16],jsonargs[512]; int32_t i,ind,n = 0;
    while ( SUPERNET.readyflag == 0 || find_daemoninfo(&ind,"SuperNET",0,0) == 0 )
    {
        if ( poll_daemons() > 0 )
            break;
        msleep(10);
    }
    sleep(1);
    sprintf(jsonargs,"{\"filename\":\"SuperNET.conf\"}");
    strs[n++] = language_func((char *)"kv777","",0,0,1,(char *)"kv777",jsonargs,call_system);
    while ( find_daemoninfo(&ind,"kv777",0,0) == 0 )
         poll_daemons();
    strs[n++] = language_func((char *)"coins","",0,0,1,(char *)"coins",jsonargs,call_system);
    while ( COINS.readyflag == 0 || find_daemoninfo(&ind,"coins",0,0) == 0 )
        poll_daemons();
#ifdef INSIDE_MGW
    //if ( SUPERNET.gatewayid >= 0 )
    {
        strs[n++] = language_func((char *)"MGW","",0,0,1,(char *)"MGW",jsonargs,call_system);
        while ( MGW.readyflag == 0 || find_daemoninfo(&ind,"MGW",0,0) == 0 )
            poll_daemons();
    }
    //if ( SUPERNET.gatewayid >= 0 )
    {
        strs[n++] = language_func((char *)"ramchain","",0,0,1,(char *)"ramchain",jsonargs,call_system);
        while ( RAMCHAINS.readyflag == 0 || find_daemoninfo(&ind,"ramchain",0,0) == 0 )
            poll_daemons();
    }
    if ( SUPERNET.gatewayid >= 0 )
        printf("MGW sock = %d\n",MGW.all.socks.both.bus);
#endif
    
    strs[n++] = language_func((char *)"relay","",0,0,1,(char *)"relay",jsonargs,call_system);
    while ( RELAYS.readyflag == 0 || find_daemoninfo(&ind,"relay",0,0) == 0 )
        poll_daemons();
     strs[n++] = language_func((char *)"prices","",0,0,1,(char *)"prices",jsonargs,call_system);
    while ( PRICES.readyflag == 0 || find_daemoninfo(&ind,"prices",0,0) == 0 )
        poll_daemons();
    /*strs[n++] = language_func((char *)"teleport","",0,0,1,(char *)"teleport",jsonargs,call_system);
    while ( TELEPORT.readyflag == 0 || find_daemoninfo(&ind,"teleport",0,0) == 0 )
        poll_daemons();
    strs[n++] = language_func((char *)"cashier","",0,0,1,(char *)"cashier",jsonargs,call_system);
    while ( CASHIER.readyflag == 0 || find_daemoninfo(&ind,"cashier",0,0) == 0 )
        poll_daemons();*/
    if ( SUPERNET.gatewayid < 0 )
    {
        strs[n++] = language_func((char *)"InstantDEX","",0,0,1,(char *)"InstantDEX",jsonargs,call_system);
        while ( INSTANTDEX.readyflag == 0 || find_daemoninfo(&ind,"InstantDEX",0,0) == 0 )
            poll_daemons();
    }
    strs[n++] = language_func((char *)"rps","",0,0,1,(char *)"rps",jsonargs,call_system);
    for (i=0; i<n; i++)
    {
        printf("%s ",strs[i]);
        free(strs[i]);
    }
    printf("num builtin plugin agents.%d\n",n);
    sleep(3);
    void serverloop(void *_args);
    serverloop(0);
}

void SuperNET_apiloop(void *ipaddr)
{
    char buf[65536],*jsonstr,*str; int32_t sock,len;
    if ( (sock= nn_socket(AF_SP,NN_PAIR)) >= 0 )
    {
        if ( nn_bind(sock,SUPERNET_APIENDPOINT) < 0 )
            fprintf(stderr,"error binding to relaypoint sock.%d type.%d (%s) %s\n",sock,NN_BUS,SUPERNET_APIENDPOINT,nn_errstr());
        else
        {
            if ( nn_settimeouts(sock,10,1) < 0 )
                fprintf(stderr,"error setting sendtimeout %s\n",nn_errstr());
            fprintf(stderr,"BIND.(%s) sock.%d\n",SUPERNET_APIENDPOINT,sock);
            while ( 1 )
            {
                if ( (len= nn_recv(sock,&jsonstr,NN_MSG,0)) > 0 )
                {
                    fprintf(stderr,"apirecv.(%s)\n",jsonstr);
                    if ( strlen(jsonstr) < sizeof(buf) )
                    {
                        strcpy(buf,jsonstr);
                        nn_freemsg(jsonstr);
                        if ( (str= process_nn_message(sock,buf)) != 0 )
                            free(str);
                    }
                }
            }
        }
        nn_shutdown(sock,0);
    }
}

void crypto_update0();
void crypto_update1();

int SuperNET_start(char *fname,char *myip)
{
    int32_t init_SUPERNET_pullsock(int32_t sendtimeout,int32_t recvtimeout);
    int32_t parse_ipaddr(char *ipaddr,char *ip_port);
    char ipaddr[256],*jsonstr = 0;
    uint64_t allocsize;
    printf("myip.(%s)\n",myip);
    portable_OS_init();
    init_SUPERNET_pullsock(10,1);
    Debuglevel = 2;
    if ( (jsonstr= loadfile(&allocsize,fname)) == 0 )
        jsonstr = clonestr("{}");
    parse_ipaddr(ipaddr,myip);
    strcpy(SUPERNET.myipaddr,ipaddr);
    printf("SuperNET_start myip.(%s) -> ipaddr.(%s)\n",myip!=0?myip:"",ipaddr);
    language_func("SuperNET","",0,0,1,"SuperNET",jsonstr,call_system);
    if ( jsonstr != 0 )
        free(jsonstr);
    portable_thread_create((void *)SuperNET_loop,myip);
    portable_thread_create((void *)SuperNET_apiloop,myip);
    portable_thread_create((void *)crypto_update0,myip);
    portable_thread_create((void *)crypto_update1,myip);
    return(0);
}

#ifdef STANDALONE

int32_t SuperNET_broadcast(char *msg,int32_t duration) { printf(">>>>>>>>> BROADCAST.(%s)\n",msg); return(0); }
int32_t SuperNET_narrowcast(char *destip,unsigned char *msg,int32_t len) { printf(">>>>>>>>>>> NARROWCAST.(%s) -> (%s)\n",msg,destip);  return(0); }

int main(int argc,const char *argv[])
{
    char _ipaddr[64],*jsonstr = 0,*ipaddr = "127.0.0.1:7777";
    int32_t i;
    cJSON *json = 0;
    uint64_t ipbits,allocsize;
#ifdef __APPLE__
    //void peggy_test();
    //portable_OS_init();
    //peggy_test();
    //void txnet777_test(char *protocol,char *path,char *agent);
    //int pegs[64];
    //int32_t peggy_test(int32_t *pegs,int32_t numprices,int32_t maxdays,double apr,int32_t spreadmillis);
    //peggy_test(pegs,64,90,2.5,2000);
    //txnet777_test("rps","RPS","rps");
    //void peggy_test(); peggy_test();
    //void SaM_PrepareIndices();
    //int32_t SaM_test();
    //SaM_PrepareIndices();
    //SaM_test();
    //printf("finished SaM_test\n");
    //void kv777_test(int32_t n);
    //kv777_test(10000);
    //getchar();
    if ( 0 )
    {
        bits128 calc_md5(char digeststr[33],void *buf,int32_t len);
        char digeststr[33],*str = "abc";
        calc_md5(digeststr,str,(int32_t)strlen(str));
        printf("(%s) -> (%s)\n",str,digeststr);
        getchar();
    }
    while ( 0 )
    {
        uint32_t nonce,failed; int32_t leverage;
        nonce = busdata_nonce(&leverage,"test string","allrelays",3000,0);
        failed = busdata_nonce(&leverage,"test string","allrelays",0,nonce);
        printf("nonce.%u failed.%u\n",nonce,failed);
    }
#endif
    if ( (jsonstr= loadfile(&allocsize,"SuperNET.conf")) == 0 )
        jsonstr = clonestr("{}");
    else if ( (json= cJSON_Parse(jsonstr)) == 0 )
    {
        printf("error parsing SuperNET.conf.(%s)\n",jsonstr);
        free(jsonstr);
        exit(-1);
    }
    else free_json(json);
    i = 1;
    if ( argc > 1 )
    {
        ipbits = calc_ipbits((char *)argv[1]);
        expand_ipbits(_ipaddr,ipbits);
        if ( strcmp(_ipaddr,argv[1]) == 0 )
            ipaddr = (char *)argv[1], i++;
    }
    ipbits = calc_ipbits(ipaddr);
    expand_ipbits(_ipaddr,ipbits);
    printf(">>>>>>>>> call SuperNET_start.(%s)\n",ipaddr);
    SuperNET_start("SuperNET.conf",ipaddr);
    printf("<<<<<<<<< back SuperNET_start\n");
    if ( i < argc )
    {
        for (; i<argc; i++)
            if ( is_bundled_plugin((char *)argv[i]) != 0 )
                language_func((char *)argv[i],"",0,0,1,(char *)argv[i],jsonstr,call_system);
    }
    sleep(60);

    while ( 1 )
    {
        char line[1024];
        if ( getline777(line,sizeof(line)-1) > 0 )
            process_userinput(line);
    }
    return(0);
}
#endif
