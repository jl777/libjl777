/******************************************************************************
 * Copyright © 2014-2015 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * Nxt software, including this file, may be copied, modified, propagated,    *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/


#include <stdio.h>
#include <curl/curl.h>
#define DEFINES_ONLY
#include "plugins/includes/portable777.h"
#include "plugins/utils/utils777.c"
#include "plugins/utils/files777.c"
#include "plugins/KV/kv777.c"
#include "plugins/common/system777.c"
#include "plugins/agents/plugins.h"
#undef DEFINES_ONLY


#ifdef INSIDE_MGW
struct db777 *DB_revNXTtrades,*DB_NXTtrades;
#else
struct kv777 *DB_revNXTtrades,*DB_NXTtrades;
#endif

struct pending_cgi { struct queueitem DL; char apitag[24],*jsonstr; cJSON *json; int32_t sock,retind; };

char *SuperNET_install(char *plugin,char *jsonstr,cJSON *json)
{
    struct destbuf ipaddr,path; char *str,*retstr;
    int32_t i,ind,async;
    uint16_t port,websocket;
    if ( find_daemoninfo(&ind,plugin,0,0) != 0 )
        return(clonestr("{\"error\":\"plugin already installed\"}"));
    copy_cJSON(&path,cJSON_GetObjectItem(json,"path"));
    copy_cJSON(&ipaddr,cJSON_GetObjectItem(json,"ipaddr"));
    port = get_API_int(cJSON_GetObjectItem(json,"port"),0);
    async = get_API_int(cJSON_GetObjectItem(json,"daemonize"),0);
    websocket = get_API_int(cJSON_GetObjectItem(json,"websocket"),0);
    str = stringifyM(jsonstr);
    retstr = language_func(plugin,ipaddr.buf,port,websocket,async,path.buf,str,call_system);
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
    struct destbuf apitag,plugin,method; char *str = 0,*broadcaststr,*destNXT; struct pending_cgi *ptr =_ptr;
    uint32_t nonce; int32_t localaccess,checklen,retlen,timeout;
    copy_cJSON(&apitag,cJSON_GetObjectItem(ptr->json,"apitag"));
    safecopy(ptr->apitag,apitag.buf,sizeof(ptr->apitag));
    copy_cJSON(&plugin,cJSON_GetObjectItem(ptr->json,"agent"));
    if ( plugin.buf[0] == 0 )
        copy_cJSON(&plugin,cJSON_GetObjectItem(ptr->json,"plugin"));
    copy_cJSON(&method,cJSON_GetObjectItem(ptr->json,"method"));
    localaccess = juint(ptr->json,"localaccess");
    if ( ptr->sock < 0 )
        localaccess = 1;
    timeout = get_API_int(cJSON_GetObjectItem(ptr->json,"timeout"),SUPERNET.PLUGINTIMEOUT);
    broadcaststr = cJSON_str(cJSON_GetObjectItem(ptr->json,"broadcast"));
    if ( ptr->sock >= 0 )
        fprintf(stderr,"sock.%d (%s) API RECV.(%s)\n",ptr->sock,broadcaststr!=0?broadcaststr:"",ptr->jsonstr);
    if ( ptr->sock >= 0 && (ptr->retind= nn_connect(ptr->sock,apitag.buf)) < 0 )
        fprintf(stderr,"error connecting to (%s)\n",apitag.buf);
    else
    {
        destNXT = cJSON_str(cJSON_GetObjectItem(ptr->json,"destNXT"));
        if ( strcmp(plugin.buf,"relay") == 0 || (broadcaststr != 0 && strcmp(broadcaststr,"remoteaccess") == 0) || cJSON_str(cJSON_GetObjectItem(ptr->json,"servicename")) != 0 )
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
            str = plugin_method(ptr->sock,0,localaccess,plugin.buf,method.buf,0,0,ptr->jsonstr,(int32_t)strlen(ptr->jsonstr)+1,timeout,0);
        }
        if ( str != 0 )
        {
            int32_t busdata_validate(char *forwarder,char *sender,uint32_t *timestamp,uint8_t *databuf,int32_t *datalenp,void *msg,cJSON *json);
            struct destbuf forwarder,sender,methodstr; uint32_t timestamp = 0; uint8_t databuf[8192]; int32_t valid=-1,datalen; cJSON *retjson,*argjson;
            forwarder.buf[0] = sender.buf[0] = 0;
            if ( (retjson= cJSON_Parse(str)) != 0 )
            {
                if ( is_cJSON_Array(retjson) != 0 && cJSON_GetArraySize(retjson) == 2 )
                {
                    argjson = cJSON_GetArrayItem(retjson,0);
                    copy_cJSON(&methodstr,cJSON_GetObjectItem(argjson,"method"));
                    if ( strcmp(methodstr.buf,"busdata") == 0 )
                    {
                        //fprintf(stderr,"call validate\n");
                        if ( (valid= busdata_validate(forwarder.buf,sender.buf,&timestamp,databuf,&datalen,str,retjson)) > 0 )
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
            if ( str != 0 && (retjson= cJSON_Parse(str)) != 0 )
            {
                if ( juint(retjson,"done") != 0 )
                {
                    cJSON_DeleteItemFromObject(retjson,"daemonid");
                    cJSON_DeleteItemFromObject(retjson,"myid");
                    cJSON_DeleteItemFromObject(retjson,"allowremote");
                    cJSON_DeleteItemFromObject(retjson,"tag");
                    cJSON_DeleteItemFromObject(retjson,"NXT");
                    cJSON_DeleteItemFromObject(retjson,"done");
                    free(str);
                    str = jprint(retjson,0);
                }
                free_json(retjson);
            }
            if ( ptr->sock >= 0 )
            {
                retlen = (int32_t)strlen(str) + 1;
                if ( (checklen= nn_send(ptr->sock,str,retlen,0)) != retlen )
                    fprintf(stderr,"checklen.%d != len.%d for nn_send to (%s)\n",checklen,retlen,apitag.buf);
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
    struct destbuf plugin,method,request; char buf[65536],*bstr,*retstr;
    uint64_t daemonid,instanceid,tag;
    int32_t override=0,broadcastflag = 0;
    cJSON *json;
    //fprintf(stderr,"process_jl777_msg previpaddr.(%s) (%s)\n",previpaddr!=0?previpaddr:"",jsonstr);
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        copy_cJSON(&request,cJSON_GetObjectItem(json,"requestType"));
        copy_cJSON(&plugin,cJSON_GetObjectItem(json,"plugin"));
        if ( jstr(json,"plugin") != 0 && jstr(json,"agent") != 0 )
            override = 1;
        //fprintf(stderr,"SuperNET_JSON override.%d\n",override);
        if ( plugin.buf[0] == 0 )
            copy_cJSON(&plugin,cJSON_GetObjectItem(json,"agent"));
        if ( override == 0 && strcmp(plugin.buf,"InstantDEX") == 0 )
        {
            if ( (retstr= InstantDEX(jsonstr,0,1)) != 0 )
            {
                free_json(json);
                return(retstr);
            }
        }
        if ( strcmp(request.buf,"install") == 0 && plugin.buf[0] != 0 )
        {
            fprintf(stderr,"call install path\n");
            retstr = SuperNET_install(plugin.buf,jsonstr,json);
            free_json(json);
            return(retstr);
        }
        tag = get_API_nxt64bits(cJSON_GetObjectItem(json,"tag"));
        daemonid = get_API_nxt64bits(cJSON_GetObjectItem(json,"daemonid"));
        instanceid = get_API_nxt64bits(cJSON_GetObjectItem(json,"instanceid"));
        copy_cJSON(&method,cJSON_GetObjectItem(json,"method"));
        if ( (bstr= cJSON_str(cJSON_GetObjectItem(json,"broadcast"))) != 0 )
            broadcastflag = 1;
        else broadcastflag = 0;
        if ( method.buf[0] == 0 )
        {
            strcpy(method.buf,request.buf);
            if ( plugin.buf[0] == 0 && set_first_plugin(plugin.buf,method.buf) < 0 )
                return(clonestr("{\"error\":\"no method or plugin specified, search for requestType failed\"}"));
        }
        if ( strlen(jsonstr) < sizeof(buf)-1)
        {
            strcpy(buf,jsonstr);
            //if ( previpaddr == 0 || previpaddr[0] == 0 )
            //    sprintf(buf + strlen(buf)-1,",\"rand\":\"%d\"}",rand());
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
    struct destbuf request,name; char *retstr = 0;;
    uint64_t daemonid,instanceid;
    cJSON *json;
    fprintf(stderr,"call_SuperNET_JSON\n");
    if ( (json= cJSON_Parse(JSONstr)) != 0 )
    {
        copy_cJSON(&request,cJSON_GetObjectItem(json,"requestType"));
        copy_cJSON(&name,cJSON_GetObjectItem(json,"plugin"));
        if ( name.buf[0] == 0 )
            copy_cJSON(&name,cJSON_GetObjectItem(json,"agent"));
        if ( strcmp(request.buf,"register") == 0 )
        {
            daemonid = get_API_nxt64bits(cJSON_GetObjectItem(json,"daemonid"));
            instanceid = get_API_nxt64bits(cJSON_GetObjectItem(json,"instanceid"));
            retstr = register_daemon(name.buf,daemonid,instanceid,cJSON_GetObjectItem(json,"methods"),cJSON_GetObjectItem(json,"pubmethods"),cJSON_GetObjectItem(json,"authmethods"));
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
    strs[n++] = language_func((char *)"relay","",0,0,1,(char *)"relay",jsonargs,call_system);
    while ( RELAYS.readyflag == 0 || find_daemoninfo(&ind,"relay",0,0) == 0 )
        poll_daemons();
    strs[n++] = language_func((char *)"shuffle","",0,0,1,(char *)"shuffle",jsonargs,call_system);
    while ( RELAYS.readyflag == 0 || find_daemoninfo(&ind,"shuffle",0,0) == 0 )
        poll_daemons();
    //strs[n++] = language_func((char *)"dcnet","",0,0,1,(char *)"dcnet",jsonargs,call_system);
    //while ( RELAYS.readyflag == 0 || find_daemoninfo(&ind,"dcnet",0,0) == 0 )
    //    poll_daemons();
#ifdef INSIDE_MGW
    if ( SUPERNET.gatewayid >= 0 )
    {
        strs[n++] = language_func((char *)"MGW","",0,0,1,(char *)"MGW",jsonargs,call_system);
        while ( MGW.readyflag == 0 || find_daemoninfo(&ind,"MGW",0,0) == 0 )
            poll_daemons();
        strs[n++] = language_func((char *)"ramchain","",0,0,1,(char *)"ramchain",jsonargs,call_system);
        while ( RAMCHAINS.readyflag == 0 || find_daemoninfo(&ind,"ramchain",0,0) == 0 )
            poll_daemons();
    }
    if ( SUPERNET.gatewayid >= 0 )
        printf("MGW sock = %d\n",MGW.all.socks.both.bus);
#else
    //if ( SUPERNET.gatewayid < 0 )
    {
        strs[n++] = language_func((char *)"InstantDEX","",0,0,1,(char *)"InstantDEX",jsonargs,call_system);
        while ( INSTANTDEX.readyflag == 0 || find_daemoninfo(&ind,"InstantDEX",0,0) == 0 )
            poll_daemons();
    }
    printf(">>>>>>>>>>>>>>>> START PRICE DAEMON\n");
    strs[n++] = language_func((char *)"prices","",0,0,1,(char *)"prices",jsonargs,call_system);
    while ( PRICES.readyflag == 0 || find_daemoninfo(&ind,"prices",0,0) == 0 )
        poll_daemons();
    printf(">>>>>>>>>>>>>>>> GOT PRICE DAEMON\n");
#endif
    /*strs[n++] = language_func((char *)"teleport","",0,0,1,(char *)"teleport",jsonargs,call_system);
    while ( TELEPORT.readyflag == 0 || find_daemoninfo(&ind,"teleport",0,0) == 0 )
        poll_daemons();
    strs[n++] = language_func((char *)"cashier","",0,0,1,(char *)"cashier",jsonargs,call_system);
    while ( CASHIER.readyflag == 0 || find_daemoninfo(&ind,"cashier",0,0) == 0 )
        poll_daemons();
     strs[n++] = language_func((char *)"rps","",0,0,1,(char *)"rps",jsonargs,call_system);
*/
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

void SuperNET_agentloop(void *ipaddr)
{
    int32_t n = 0;
    while ( 1 )
    {
        if ( poll_daemons() == 0 )
        {
            n++;
            msleep(1000 * (n + 1));
            if ( n > 100 )
                n = 100;
        } else n = 0;
    }
}

void SuperNET_apiloop(void *ipaddr)
{
    struct destbuf plugin; char *jsonstr,*retstr,*msg; int32_t sock,len,retlen,checklen; cJSON *json;
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
                if ( (len= nn_recv(sock,&msg,NN_MSG,0)) > 0 )
                {
                    jsonstr = clonestr(msg);
                    nn_freemsg(msg);
                    retstr = 0;
                    //fprintf(stderr,"apirecv.(%s)\n",jsonstr);
                    if ( INSTANTDEX.readyflag != 0 && (json= cJSON_Parse(jsonstr)) != 0 )
                    {
                        copy_cJSON(&plugin,jobj(json,"agent"));
                        if ( plugin.buf[0] == 0 )
                            copy_cJSON(&plugin,jobj(json,"plugin"));
                        if ( strcmp(plugin.buf,"InstantDEX") == 0 )
                        {
                            retstr = clonestr("retstr");
                            if ( (retstr= InstantDEX(jsonstr,jstr(json,"remoteaddr"),juint(json,"localaccess"))) != 0 )
                            {
                                retlen = (int32_t)strlen(retstr) + 1;
                                if ( (checklen= nn_send(sock,retstr,retlen,0)) != retlen )
                                    fprintf(stderr,"checklen.%d != len.%d for nn_send of (%s)\n",checklen,retlen,retstr);
                            }
                        } else fprintf(stderr,">>>>>>>>> request is not InstantDEX (%s) %s\n",plugin.buf,jsonstr);
                        free_json(json);
                    }
                    if ( retstr == 0 && (retstr= process_nn_message(sock,jsonstr)) != 0 )
                        free(retstr);
                }
                msleep(SUPERNET.recvtimeout);
            }
        }
        nn_shutdown(sock,0);
    }
}

uint64_t set_account_NXTSECRET(char *NXTacct,char *NXTaddr,char *secret,int32_t max,cJSON *argjson,char *coinstr,char *serverport,char *userpass)
{
    uint64_t allocsize,nxt64bits;
    char coinaddr[MAX_JSON_FIELD],*str,*privkey;
    NXTaddr[0] = 0;
    extract_cJSON_str(secret,max,argjson,"secret");
    if ( Debuglevel > 2 )
        printf("set_account_NXTSECRET.(%s)\n",secret);
    if ( secret[0] == 0 )
    {
        extract_cJSON_str(coinaddr,sizeof(coinaddr),argjson,"privateaddr");
        if ( strcmp(coinaddr,"privateaddr") == 0 )
        {
            if ( (str= loadfile(&allocsize,"privateaddr")) != 0 )
            {
                if ( allocsize < 128 )
                    strcpy(coinaddr,str);
                free(str);
            }
        }
        if ( coinaddr[0] == 0 )
            extract_cJSON_str(coinaddr,sizeof(coinaddr),argjson,"pubsrvaddr");
        printf("coinaddr.(%s)\n",coinaddr);
        if ( coinstr == 0 || serverport == 0 || userpass == 0 || (privkey= dumpprivkey(coinstr,serverport,userpass,coinaddr)) == 0 )
            gen_randomacct(33,NXTaddr,secret,"randvals");
        else
        {
            strcpy(secret,privkey);
            free(privkey);
        }
    }
    else if ( strcmp(secret,"randvals") == 0 )
        gen_randomacct(33,NXTaddr,secret,"randvals");
    nxt64bits = conv_NXTpassword(SUPERNET.myprivkey,SUPERNET.mypubkey,(uint8_t *)secret,(int32_t)strlen(secret));
    expand_nxt64bits(NXTaddr,nxt64bits);
    if ( 1 )
        conv_rsacctstr(NXTacct,nxt64bits);
    printf("(%s) (%s) (%s)\n",NXTacct,NXTaddr,Debuglevel > 2 ? secret : "<secret>");
    return(nxt64bits);
}

void SuperNET_initconf(cJSON *json)
{
    struct destbuf myipaddr,tmp; uint8_t mysecret[32],mypublic[32]; FILE *fp;
    MAX_DEPTH = get_API_int(cJSON_GetObjectItem(json,"MAX_DEPTH"),MAX_DEPTH);
    if ( MAX_DEPTH > _MAX_DEPTH )
        MAX_DEPTH = _MAX_DEPTH;
    SUPERNET.disableNXT = get_API_int(cJSON_GetObjectItem(json,"disableNXT"),0);
    SUPERNET.ismainnet = get_API_int(cJSON_GetObjectItem(json,"MAINNET"),1);
    SUPERNET.usessl = get_API_int(cJSON_GetObjectItem(json,"USESSL"),0);
    SUPERNET.NXTconfirms = get_API_int(cJSON_GetObjectItem(json,"NXTconfirms"),10);
    copy_cJSON(&tmp,cJSON_GetObjectItem(json,"NXTAPIURL")), safecopy(SUPERNET.NXTAPIURL,tmp.buf,sizeof(SUPERNET.NXTAPIURL));
    if ( SUPERNET.NXTAPIURL[0] == 0 )
    {
        if ( SUPERNET.usessl == 0 )
            strcpy(SUPERNET.NXTAPIURL,"http://127.0.0.1:");
        else strcpy(SUPERNET.NXTAPIURL,"https://127.0.0.1:");
        if ( SUPERNET.ismainnet != 0 )
            strcat(SUPERNET.NXTAPIURL,"7876/nxt");
        else strcat(SUPERNET.NXTAPIURL,"6876/nxt");
    }
    copy_cJSON(&tmp,cJSON_GetObjectItem(json,"userdir")), safecopy(SUPERNET.userhome,tmp.buf,sizeof(SUPERNET.userhome));
    if ( SUPERNET.userhome[0] == 0 )
        strcpy(SUPERNET.userhome,"/root");
    strcpy(SUPERNET.NXTSERVER,SUPERNET.NXTAPIURL);
    strcat(SUPERNET.NXTSERVER,"?requestType");
    copy_cJSON(&tmp,cJSON_GetObjectItem(json,"myNXTacct")), safecopy(SUPERNET.myNXTacct,tmp.buf,sizeof(SUPERNET.myNXTacct));
    if ( SUPERNET.disableNXT == 0 )
        set_account_NXTSECRET(SUPERNET.NXTACCT,SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,sizeof(SUPERNET.NXTACCTSECRET)-1,json,0,0,0);
    else strcpy(SUPERNET.NXTADDR,SUPERNET.myNXTacct);
    SUPERNET.my64bits = conv_acctstr(SUPERNET.NXTADDR);
    copy_cJSON(&myipaddr,cJSON_GetObjectItem(json,"myipaddr"));
    if ( myipaddr.buf[0] != 0 || SUPERNET.myipaddr[0] == 0 )
        safecopy(SUPERNET.myipaddr,myipaddr.buf,sizeof(SUPERNET.myipaddr));
    if ( SUPERNET.myipaddr[0] != 0 )
        SUPERNET.myipbits = (uint32_t)calc_ipbits(SUPERNET.myipaddr);
    //KV777.mmapflag = get_API_int(cJSON_GetObjectItem(json,"mmapflag"),0);
    //if ( strncmp(SUPERNET.myipaddr,"89.248",5) == 0 )
    //    SUPERNET.iamrelay = get_API_int(cJSON_GetObjectItem(json,"iamrelay"),1*0);
    //else
    SUPERNET.iamrelay = get_API_int(cJSON_GetObjectItem(json,"iamrelay"),0);
    copy_cJSON(&tmp,cJSON_GetObjectItem(json,"hostname")), safecopy(SUPERNET.hostname,tmp.buf,sizeof(SUPERNET.hostname));
    SUPERNET.port = get_API_int(cJSON_GetObjectItem(json,"SUPERNET_PORT"),SUPERNET_PORT);
    SUPERNET.serviceport = get_API_int(cJSON_GetObjectItem(json,"serviceport"),SUPERNET_PORT - 2);
    copy_cJSON(&tmp,cJSON_GetObjectItem(json,"transport")), safecopy(SUPERNET.transport,tmp.buf,sizeof(SUPERNET.transport));
    if ( SUPERNET.transport[0] == 0 )
        strcpy(SUPERNET.transport,SUPERNET.UPNP == 0 ? "tcp" : "ws");
    sprintf(SUPERNET.lbendpoint,"%s://%s:%u",SUPERNET.transport,SUPERNET.myipaddr,SUPERNET.port + LB_OFFSET);
    sprintf(SUPERNET.relayendpoint,"%s://%s:%u",SUPERNET.transport,SUPERNET.myipaddr,SUPERNET.port + PUBRELAYS_OFFSET);
    sprintf(SUPERNET.globalendpoint,"%s://%s:%u",SUPERNET.transport,SUPERNET.myipaddr,SUPERNET.port + PUBGLOBALS_OFFSET);
    copy_cJSON(&tmp,cJSON_GetObjectItem(json,"SERVICESECRET")), safecopy(SUPERNET.SERVICESECRET,tmp.buf,sizeof(SUPERNET.SERVICESECRET));
    expand_nxt64bits(SUPERNET.SERVICENXT,conv_NXTpassword(mysecret,mypublic,(uint8_t *)SUPERNET.SERVICESECRET,(int32_t)strlen(SUPERNET.SERVICESECRET)));
    printf("SERVICENXT.%s\n",SUPERNET.SERVICENXT);
    SUPERNET.automatch = get_API_int(cJSON_GetObjectItem(json,"automatch"),3);
#ifndef __linux__
    SUPERNET.UPNP = 1;
#endif
    SUPERNET.telepathicdelay = get_API_int(cJSON_GetObjectItem(json,"telepathicdelay"),1000);
    SUPERNET.peggy = get_API_int(cJSON_GetObjectItem(json,"peggy"),0);
    SUPERNET.idlegap = get_API_int(cJSON_GetObjectItem(json,"idlegap"),60);
    SUPERNET.recvtimeout = get_API_int(cJSON_GetObjectItem(json,"recvtimeout"),10);
    SUPERNET.exchangeidle = get_API_int(cJSON_GetObjectItem(json,"exchangeidle"),3);
    SUPERNET.gatewayid = get_API_int(cJSON_GetObjectItem(json,"gatewayid"),-1);
    SUPERNET.numgateways = get_API_int(cJSON_GetObjectItem(json,"numgateways"),3);
    SUPERNET.UPNP = get_API_int(cJSON_GetObjectItem(json,"UPNP"),SUPERNET.UPNP);
    SUPERNET.APISLEEP = get_API_int(cJSON_GetObjectItem(json,"APISLEEP"),DEFAULT_APISLEEP);
    SUPERNET.PLUGINTIMEOUT = get_API_int(cJSON_GetObjectItem(json,"PLUGINTIMEOUT"),10000);
    if ( SUPERNET.APISLEEP <= 1 )
        SUPERNET.APISLEEP = 1;
    copy_cJSON(&tmp,cJSON_GetObjectItem(json,"DATADIR")), safecopy(SUPERNET.DATADIR,tmp.buf,sizeof(SUPERNET.DATADIR));
    if ( SUPERNET.DATADIR[0] == 0 )
        strcpy(SUPERNET.DATADIR,"archive");
    Debuglevel = get_API_int(cJSON_GetObjectItem(json,"debug"),Debuglevel);
    if ( (fp= fopen("libs/websocketd","rb")) != 0 )
    {
        fclose(fp);
        strcpy(SUPERNET.WEBSOCKETD,"libs/websocketd");
    }
    else strcpy(SUPERNET.WEBSOCKETD,"websocketd");
    copy_cJSON(&tmp,cJSON_GetObjectItem(json,"backups")), safecopy(SUPERNET.BACKUPS,tmp.buf,sizeof(SUPERNET.BACKUPS));
    if ( SUPERNET.BACKUPS[0] == 0 )
        strcpy(SUPERNET.BACKUPS,"/tmp");
    copy_cJSON(&tmp,cJSON_GetObjectItem(json,"DBPATH")), safecopy(SUPERNET.DBPATH,tmp.buf,sizeof(SUPERNET.DBPATH));
    if ( SUPERNET.DBPATH[0] == 0 )
        strcpy(SUPERNET.DBPATH,"./DB");
    os_compatible_path(SUPERNET.DBPATH), ensure_directory(SUPERNET.DBPATH);
#ifdef INSIDE_MGW
    char buf[512];
    copy_cJSON(RAMCHAINS.pullnode,cJSON_GetObjectItem(json,"pullnode"));
    copy_cJSON(SOPHIA.PATH,cJSON_GetObjectItem(json,"SOPHIA"));
    copy_cJSON(SOPHIA.RAMDISK,cJSON_GetObjectItem(json,"RAMDISK"));
    if ( SOPHIA.PATH[0] == 0 )
        strcpy(SOPHIA.PATH,"./DB");
    os_compatible_path(SOPHIA.PATH), ensure_directory(SOPHIA.PATH);
    MGW.port = get_API_int(cJSON_GetObjectItem(json,"MGWport"),7643);
    copy_cJSON(MGW.PATH,cJSON_GetObjectItem(json,"MGWPATH"));
    if ( MGW.PATH[0] == 0 )
        strcpy(MGW.PATH,"/var/www/html/MGW");
    ensure_directory(MGW.PATH);
    sprintf(buf,"%s/msig",MGW.PATH), ensure_directory(buf);
    sprintf(buf,"%s/status",MGW.PATH), ensure_directory(buf);
    sprintf(buf,"%s/sent",MGW.PATH), ensure_directory(buf);
    sprintf(buf,"%s/deposit",MGW.PATH), ensure_directory(buf);
    printf("MGWport.%u >>>>>>>>>>>>>>>>>>> INIT ********************** (%s) (%s) (%s) SUPERNET.port %d UPNP.%d NXT.%s ip.(%s) iamrelay.%d pullnode.(%s)\n",MGW.port,SOPHIA.PATH,MGW.PATH,SUPERNET.NXTSERVER,SUPERNET.port,SUPERNET.UPNP,SUPERNET.NXTADDR,SUPERNET.myipaddr,SUPERNET.iamrelay,RAMCHAINS.pullnode);
    //if ( SUPERNET.gatewayid >= 0 )
    {
        if ( DB_NXTaccts == 0 )
            DB_NXTaccts = db777_create(0,0,"NXTaccts",0,0);
        //if ( DB_nodestats == 0 )
        //    DB_nodestats = db777_create(0,0,"nodestats",0,0);
        //if ( DB_busdata == 0 )
        //    DB_busdata = db777_create(0,0,"busdata",0,0);
        if ( DB_NXTtxids == 0 )
            DB_NXTtxids = db777_create(0,0,"NXT_txids",0,0);
        if ( DB_redeems == 0 )
            DB_redeems = db777_create(0,0,"redeems",0,0);
        if ( DB_MGW == 0 )
            DB_MGW = db777_create(0,0,"MGW",0,0);
        if ( DB_msigs == 0 )
            DB_msigs = db777_create(0,0,"msigs",0,0);
        if ( DB_NXTtrades == 0 )
            DB_NXTtrades = db777_create(0,0,"NXT_trades",0,0);
        //if ( DB_services == 0 )
        //    DB_services = db777_create(0,0,"services",0,0);
    }
#else
    {
        //struct kv777 *kv777_init(char *path,char *name,struct kv777_flags *flags); // kv777_init IS NOT THREADSAFE!
        if ( DB_NXTtrades == 0 )
            DB_NXTtrades = kv777_init(SUPERNET.DBPATH,"NXT_trades",0);
        if ( DB_revNXTtrades == 0 )
            DB_revNXTtrades = kv777_init(SUPERNET.DBPATH,"revNXT_trades",0);
        SUPERNET.NXTaccts = kv777_init(SUPERNET.DBPATH,"NXTaccts",0);
        SUPERNET.NXTtxids = kv777_init(SUPERNET.DBPATH,"NXT_txids",0);
    }
#endif
    if ( SUPERNET.iamrelay != 0 )
    {
        SUPERNET.PM = kv777_init(SUPERNET.DBPATH,"PM",0);
        SUPERNET.alias = kv777_init(SUPERNET.DBPATH,"alias",0);
        SUPERNET.protocols = kv777_init(SUPERNET.DBPATH,"protocols",0);
        SUPERNET.rawPM = kv777_init(SUPERNET.DBPATH,"rawPM",0);
        SUPERNET.services = kv777_init(SUPERNET.DBPATH,"services",0);
        SUPERNET.invoices = kv777_init(SUPERNET.DBPATH,"invoices",0);
    }
}

int SuperNET_start(char *fname,char *myip)
{
    void crypto_update();
    int32_t init_SUPERNET_pullsock(int32_t sendtimeout,int32_t recvtimeout);
    char ipaddr[256],*jsonstr = 0; cJSON *json; uint64_t i,allocsize;
    portable_OS_init();
    printf("%p myip.(%s)\n",myip,myip);
    parse_ipaddr(ipaddr,myip);
    Debuglevel = 2;
    if ( (jsonstr= loadfile(&allocsize,fname)) == 0 )
    {
        printf("ERROR >>>>>>>>>>> SuperNET.conf file doesnt exist\n");
        exit(-666);
    }
    else
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
            SuperNET_initconf(json), free_json(json);
        else
        {
            printf("ERROR >>>>>>>>>>> SuperNET.conf file couldnt be parsed\n");
            exit(-666);
        }
#ifdef __APPLE__
        //void test_subatomic();
        //test_subatomic();
#endif
    }
    strcpy(SUPERNET.myipaddr,ipaddr);
    init_SUPERNET_pullsock(10,SUPERNET.recvtimeout);
    busdata_init(10,1,0);
    printf("SuperNET_start myip.(%s) -> ipaddr.(%s) SUPERNET.port %d\n",myip!=0?myip:"",ipaddr,SUPERNET.port);
    language_func("SuperNET","",0,0,1,"SuperNET",jsonstr,call_system);
    if ( jsonstr != 0 )
        free(jsonstr);
    portable_thread_create((void *)SuperNET_loop,myip);
    printf("busdata_init done\n");
    for (i=0; i<100; i++)
    {
        if ( INSTANTDEX.readyflag != 0 )
            break;
        msleep(10000);
    }
    portable_thread_create((void *)SuperNET_agentloop,myip);
    portable_thread_create((void *)SuperNET_apiloop,myip);
    portable_thread_create((void *)crypto_update,myip);
    void idle(); void idle2();
    portable_thread_create((void *)idle,myip);
    portable_thread_create((void *)idle2,myip);
    return(0);
}

#ifdef STANDALONE

int32_t SuperNET_broadcast(char *msg,int32_t duration) { printf(">>>>>>>>> BROADCAST.(%s)\n",msg); return(0); }
int32_t SuperNET_narrowcast(char *destip,unsigned char *msg,int32_t len) { printf(">>>>>>>>>>> NARROWCAST.(%s) -> (%s)\n",msg,destip);  return(0); }

#ifdef zkp
#define DEFINES_ONLY
#include "NXT777.c"
#undef DEFINES_ONLY

void zktest()
{
    /*
# Given the public key of B (remote_pub), shows that the shared secret
# between A and B was generated by A.
# Returns zero-knowledge proof of shared Diffie-Hellman secret between A & B.
    def prove_shared_secret(self, remote_pub):
    G = self.G; prover_pub = self.public; phi = self. P - 1;
    secret = self.get_shared_secret(remote_pub)
    
# Random key in the group Z_q
    randKey = DiffieHellman() # random secret
    commit1 = randKey.public
    commit2 = randKey.get_shared_secret(remote_pub)
    */
    void fdifference_backwards(uint64_t *out, const uint64_t *in); // output = in - output
    void fmul(uint64_t *output,const uint64_t *in,const uint64_t *in2);
    void fcontract(uint8_t *output, const uint64_t *input);
    void fexpand(uint64_t *output, const uint8_t *in);
    bits256 curve25519(bits256,bits256);
    static uint8_t _basepoint[32] = {9};
    bits320 randsecret,challenge,product,response,selfsecret,secret;
    bits256 remote_pub,basepoint,remote_secret,randkey,commit1,commit2,_secret,tmp,buf[8]; int32_t n = 0;
    tmp = GENESIS_PRIVKEY;
    _secret = curve25519(tmp,remote_pub);
    fexpand(secret.ulongs,_secret.bytes);
    randombytes(randkey.bytes,sizeof(randkey)), randkey.bytes[0] &= 248, randkey.bytes[31] &= 127, randkey.bytes[31] |= 64;
    randombytes(remote_secret.bytes,sizeof(remote_secret)), remote_secret.bytes[0] &= 248, remote_secret.bytes[31] &= 127, remote_secret.bytes[31] |= 64;
    memcpy(basepoint.bytes,_basepoint,sizeof(basepoint));
    remote_pub = curve25519(remote_secret,basepoint);
    fexpand(randsecret.ulongs,randkey.bytes);
    curve25519_donna(commit1.bytes,randkey.bytes,_basepoint);
    commit2 = curve25519(randkey,remote_pub);
    /*
# shift and hash
    concat = str(G) + str(prover_pub) + str(remote_pub) + str(secret) + str(commit1) + str(commit2)
    h = hashlib.md5()
    h.update(concat.encode("utf-8"))
    challenge = int(h.hexdigest(), 16)
    product = (self.secret * challenge) % phi
    response = (randKey.secret - product) % phi
    
    return (secret, challenge, response)*/
    buf[n++] = GENESIS_PRIVKEY, buf[n++] = GENESIS_PUBKEY;
    buf[n++] = remote_pub, buf[n++] = _secret, buf[n++] = commit1, buf[n++] = commit2;
    memset(challenge.bytes,0,sizeof(challenge));
    calc_sha256(0,tmp.bytes,buf[0].bytes,n*sizeof(buf[0]));
    fexpand(challenge.ulongs,tmp.bytes);
    tmp = GENESIS_PRIVKEY;
    fexpand(selfsecret.ulongs,tmp.bytes);
    fmul(product.ulongs,selfsecret.ulongs,challenge.ulongs);
    response = product;
    fdifference_backwards(product.ulongs,randsecret.ulongs);
    /*
# Verifies proof generated above. Verifier c is showing that
# shared secret between A and B was generated by A.
# returns 0 if if verification fails; returns shared secret otherwise
    def verify_shared_secret(self, prover_pub, remote_pub, secret, challenge,
                             response):
    P = self.P; G = self.G ; public = self.public
    
# g^r * (a's public key)^challenge
    commit1 = (pow(G, response, P) * pow(public, challenge, P)) % P
    
# (b's public key)^response * (secret)^challenge
    commit2 = (pow(remote_pub, response, P) * pow(secret, challenge, P)) % P
    */
    bits256 _commit1b,_commit2b,_tmp2,_challenge,_response; bits320 Tmp,Tmp2,commit2b;
    fcontract(_challenge.bytes,challenge.ulongs);
    fcontract(_response.bytes,response.ulongs);
    tmp = curve25519(_secret,_challenge);
    _tmp2 = curve25519(remote_pub,_response);
    fexpand(Tmp.ulongs,tmp.bytes);
    fexpand(Tmp2.ulongs,_tmp2.bytes);
    fmul(commit2b.ulongs,Tmp.ulongs,Tmp2.ulongs);
    fcontract(_commit2b.bytes,commit2b.ulongs);
    printf("commits %llx %llx vs %llx %llx\n",commit1.txid,commit2.txid,_commit1b.txid,_commit2b.txid);
    /*
# Shift and hash
    hasher = hashlib.md5()
    concat = str(G) + str(prover_pub) + str(remote_pub) + str(secret) + str(commit1) + str(commit2)
    hasher.update(concat.encode("utf-8"))
    check = int(hasher.hexdigest(), 16)
    
    if challenge == check:
        return secret
        else:
            return 0
            
            def main():
            a = DiffieHellman()
            b = DiffieHellman()
            results = a.prove_shared_secret(b.public)
            assert a.verify_shared_secret(a.public, b.public, results[0],  \
                                          results[1], results[2])
*/
}
#endif

int main(int argc,const char *argv[])
{
    char _ipaddr[64],*jsonstr = 0,*ipaddr = "127.0.0.1:7777";
    int32_t i;
    cJSON *json = 0;
    uint64_t ipbits,allocsize;
#ifdef __APPLE__
    char dest[512*2 + 1];
    hmac_sha512_str(dest,"exchange->apisecret",(int32_t)strlen("exchange->apisecret"),"helloworld"); printf("hmac_sha512.(%s)\n",dest);
    hmac_sha384_str(dest,"exchange->apisecret",(int32_t)strlen("exchange->apisecret"),"helloworld"); printf("hmac_sha384.(%s)\n",dest);
    hmac_sha256_str(dest,"exchange->apisecret",(int32_t)strlen("exchange->apisecret"),"helloworld"); printf("hmac_sha256.(%s)\n",dest);
    hmac_sha224_str(dest,"exchange->apisecret",(int32_t)strlen("exchange->apisecret"),"helloworld"); printf("hmac_sha224.(%s)\n",dest);
    hmac_rmd160_str(dest,"exchange->apisecret",(int32_t)strlen("exchange->apisecret"),"helloworld"); printf("hmac_rmd160.(%s)\n",dest);
    hmac_rmd128_str(dest,"exchange->apisecret",(int32_t)strlen("exchange->apisecret"),"helloworld"); printf("hmac_rmd128.(%s)\n",dest);
    hmac_rmd256_str(dest,"exchange->apisecret",(int32_t)strlen("exchange->apisecret"),"helloworld"); printf("hmac_rmd256.(%s)\n",dest);
    hmac_rmd320_str(dest,"exchange->apisecret",(int32_t)strlen("exchange->apisecret"),"helloworld"); printf("hmac_rmd320.(%s)\n",dest);
    hmac_sha1_str(dest,"exchange->apisecret",(int32_t)strlen("exchange->apisecret"),"helloworld"); printf("hmac_sha1.(%s)\n",dest);
    hmac_md2_str(dest,"exchange->apisecret",(int32_t)strlen("exchange->apisecret"),"helloworld"); printf("hmac_md2.(%s)\n",dest);
    hmac_md4_str(dest,"exchange->apisecret",(int32_t)strlen("exchange->apisecret"),"helloworld"); printf("hmac_md4.(%s)\n",dest);
    hmac_md5_str(dest,"exchange->apisecret",(int32_t)strlen("exchange->apisecret"),"helloworld"); printf("hmac_md5.(%s)\n",dest);
    hmac_tiger_str(dest,"exchange->apisecret",(int32_t)strlen("exchange->apisecret"),"helloworld"); printf("hmac_tiger.(%s)\n",dest);
    hmac_whirlpool_str(dest,"exchange->apisecret",(int32_t)strlen("exchange->apisecret"),"helloworld"); printf("hmac_whirlpool.(%s)\n",dest);
    //getchar();
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
        //getchar();
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
   // sleep(60);
    uint32_t nonce;
    char *str,*teststr = "{\"offerNXT\":\"423766016895692955\",\"plugin\":\"relay\",\"destplugin\":\"InstantDEX\",\"method\":\"busdata\",\"submethod\":\"swap\",\"exchange\":\"InstantDEX\",\"base\":\"LTC\",\"rel\":\"NXT\",\"baseid\":\"2881764795164526882\",\"relid\":\"5527630\",\"baseqty\":\"-10000\",\"relqty\":\"10720000000\",\"price\":107.20000000,\"volume\":1.00000000,\"triggerhash\":\"c237c5827bf727957304faa7ef97f1a7bcc3037a0eb77190e6eee5c15d26a68c\",\"fullhash\":\"038b259a3582df5522eddb09f9c287696b8af3580bcf0ce6c200f4bef8703208\",\"utx\":\"021132a53f033c004e5bbad625df3d536fa90b1e6a28c3f5a56e1fcbe34132391c8d3fd7f671cb199b30f378f284e105000000000000000000e1f50500000000c237c5827bf727957304faa7ef97f1a7bcc3037a0eb77190e6eee5c15d26a68c0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000879e070057b572fe4442d0d3012209021ed015fe271027000000000000\",\"sighash\":\"5d63be5663821581f382228a14dec02be73becfac1e6b1f1b70afc8df6d433a4\",\"otherbits\":\"5527630\",\"otherqty\":\"10720000000\"}";
    if ( 0 && (str= busdata_sync(&nonce,clonestr(teststr),"allnodes",0)) != 0 )
    {
        printf("retstr.(%s)\n",str);
        getchar();
    }
    while ( 1 )
    {
        char line[32768];
        line[0] = 0;
        if ( getline777(line,sizeof(line)-1) > 0 )
        {
            //printf("getline777.(%s)\n",line);
            process_userinput(line);
        }
    }
    return(0);
}
#endif
