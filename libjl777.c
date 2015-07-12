//
//  jl777.cpp
//  glue code for pNXT
//
//  Created by jl777 on 7/6/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifdef oldway
#include "jl777.h"
struct pingpong_queue Pending_offersQ;

#ifdef _WIN32
#include "pton.h"
#endif

void handler_gotfile(char *sender,char *senderip,struct transfer_args *args,uint8_t *data,int32_t len,uint32_t crc)
{
    void _RTmgw_handler(struct transfer_args *args);
    void bridge_handler(struct transfer_args *args);
    //FILE *fp;
    //char buf[512],*str;
    uint32_t now = (uint32_t)time(NULL);
    if ( args->syncmem != 0 )
    {
        if ( args->handlercrc == crc )
        {
            if ( now < args->handlertime+10 )
                return;
        }
        args->handlercrc = crc;
        args->handlertime = now;
    }
#ifdef later
    if ( strcmp(args->handler,"mgw") == 0 || strcmp(args->handler,"RTmgw") == 0 )
    {
        set_handler_fname(buf,args->handler,args->name);
        if ( (fp= fopen(os_compatible_path(buf),"wb")) != 0 )
        {
            printf("handler_gotfile created.(%s).%d\n",buf,args->totallen);
            fwrite(args->data,1,args->totallen,fp);
            fclose(fp);
        }
        if ( strcmp(args->handler,"RTmgw") == 0 )
            _RTmgw_handler(args);
        else MGW_handler(args);
    }
    else if ( strcmp(args->handler,"bridge") == 0 )
        bridge_handler(args);
    else if ( strcmp(args->handler,"ramchain") == 0 )
    {
        printf("handler_gotfile(%s len.%d) (%s)\n",args->name,args->totallen,args->data);
        if ( (str= ramresponse((char *)args->data,sender,senderip,0)) != 0 )
            free(str);
    }
    else printf("unknown handler.(%s)\n",args->handler);
#endif
    if ( args->syncmem == 0 )
    {
        memset(args->data,0,args->totallen);
        memset(args->crcs,0,args->numfrags * sizeof(*args->gotcrcs));
    }
}

char *get_public_srvacctsecret()
{
    struct coin_info *cp = get_coin_info("BTCD");
    if ( cp != 0 )
        return(cp->srvNXTACCTSECRET);
    else return(GENESIS_SECRET);
}

int32_t expire_nodestats(struct nodestats *stats,uint32_t now)
{
    if ( stats->lastcontact != 0 && (now - stats->lastcontact) > NODESTATS_EXPIRATION )
    {
        char ipaddr[64];
        expand_ipbits(ipaddr,stats->ipbits);
        printf("expire_nodestats %s | %u %u %d\n",ipaddr,now,stats->lastcontact,now - stats->lastcontact);
        stats->gotencrypted = stats->modified = 0;
        //stats->sentmilli = 0;
        stats->expired = 1;
        return(1);
    }
    return(0);
}

int32_t pingall(char *coinstr,char *NXTACCTSECRET)
{
    int32_t i,n = 0;
    char ipaddr[64],cmdstr[1024];
    struct pserver_info *pserver;
    for (i=n=0; i<Num_in_whitelist; i++)
    {
        expand_ipbits(ipaddr,SuperNET_whitelist[i]);
        pserver = get_pserver(0,ipaddr,0,0);
        if ( ismyipaddr(ipaddr) == 0 )
        {
            gen_pingstr(cmdstr,1,coinstr);
            send_to_ipaddr(0,1,pserver->ipaddr,cmdstr,NXTACCTSECRET);
        }
    }
    return(n);
}

void every_minute(int32_t counter)
{
    static int broadcast_count;
    uint32_t now = (uint32_t)time(NULL);
    int32_t i,n,numnodes,len;
    char ipaddr[64],_cmd[MAX_JSON_FIELD];
    uint8_t finalbuf[MAX_JSON_FIELD];
    struct coin_info *cp;
    struct nodestats *stats,**nodes;
    struct pserver_info *pserver;
    if ( Finished_init == 0 )
        return;
    now = (uint32_t)time(NULL);
    cp = get_coin_info("BTCD");
    if ( cp == 0 )
        return;
    //printf("<<<<<<<<<<<<< EVERY_MINUTE\n");
    refresh_buckets(cp->srvNXTACCTSECRET);
    if ( broadcast_count == 0 )
    {
        p2p_publishpacket(0,0);
        update_Kbuckets(get_nodestats(cp->srvpubnxtbits),cp->srvpubnxtbits,cp->myipaddr,0,0,0);
        nodes = (struct nodestats **)copy_all_DBentries(&numnodes,NODESTATS_DATA);
        if ( nodes != 0 )
        {
            now = (uint32_t)time(NULL);
            for (i=0; i<numnodes; i++)
            {
                expand_ipbits(ipaddr,nodes[i]->ipbits);
                printf("(%llu %d %s) ",(long long)nodes[i]->nxt64bits,nodes[i]->lastcontact-now,ipaddr);
                if ( gen_pingstr(_cmd,1,0) > 0 )
                {
                    len = construct_tokenized_req((char *)finalbuf,_cmd,cp->srvNXTACCTSECRET);
                    send_packet(!prevent_queueing("ping"),nodes[i]->ipbits,0,finalbuf,len);
                    pserver = get_pserver(0,ipaddr,0,0);
                    send_kademlia_cmd(0,pserver,"ping",cp->srvNXTACCTSECRET,0,0);
                    p2p_publishpacket(pserver,0);
                }
                free(nodes[i]);
            }
            free(nodes);
        }
        printf("numnodes.%d\n",numnodes);
    }
    if ( (broadcast_count % 10) == 0 )
    {
        for (i=n=0; i<Num_in_whitelist; i++)
        {
            expand_ipbits(ipaddr,SuperNET_whitelist[i]);
            pserver = get_pserver(0,ipaddr,0,0);
            if ( ismyipaddr(ipaddr) == 0 && ((stats= get_nodestats(pserver->nxt64bits)) == 0 || broadcast_count == 0 || (now - stats->lastcontact) > NODESTATS_EXPIRATION) )
                send_kademlia_cmd(0,pserver,"ping",cp->srvNXTACCTSECRET,0,0), n++;
        }
        if ( Debuglevel > 0 )
            printf("PINGED.%d\n",n);
    }
    broadcast_count++;
}

void SuperNET_idler(uv_idle_t *handle)
{
    static int counter;
    static double lastattempt,lastclock;
    double millis;
    void *up;
    struct udp_queuecmd *qp;
    struct write_req_t *wr;//,*firstwr = 0;
    int32_t i,flag;
    char *jsonstr,*retstr;
    struct resultsitem *rp;
    if ( Finished_init == 0 || IS_LIBTEST == 7 )
        return;
    millis = milliseconds();//((double)uv_hrtime() / 1000000);
    for (i=0; i<10000; i++)
        if ( poll_daemons() <= 0 )
            break;
    if ( millis > (lastattempt + APISLEEP) )
    {
        lastattempt = millis;
#ifdef TIMESCRAMBLE
        while ( (wr= queue_dequeue(&sendQ,0)) != 0 )
        {
            if ( wr == firstwr )
            {
                //queue_enqueue("sendQ",&sendQ,wr);
                process_sendQ_item(wr);
                if ( Debuglevel > 2 )
                    printf("SuperNET_idler: reached firstwr.%p\n",firstwr);
                break;
            }
            if ( wr->queuetime == 0 || wr->queuetime > lastattempt )
            {
                process_sendQ_item(wr);
                // free(wr); libuv does this
                break;
            }
            if ( firstwr == 0 )
                firstwr = wr;
            queue_enqueue("sendQ",&sendQ,&wr->DL);
        }
        if ( Debuglevel > 2 && queue_size(&sendQ) != 0 )
            printf("sendQ size.%d\n",queue_size(&sendQ));
#else
#endif
        flag = 1;
        while ( flag != 0 )
        {
            flag = 0;
            if ( (qp= queue_dequeue(&udp_JSON,0)) != 0 )
            {
                //printf("process qp argjson.%p\n",qp->argjson);
                char previpaddr[64];
                expand_ipbits(previpaddr,qp->previpbits);
                jsonstr = SuperNET_json_commands(Global_mp,previpaddr,qp->argjson,qp->tokenized_np->H.U.NXTaddr,qp->valid,qp->decoded);
                //printf("free qp (%s) argjson.%p\n",jsonstr,qp->argjson);
                if ( jsonstr != 0 )
                    free(jsonstr);
                free(qp->decoded);
                free_json(qp->argjson);
                free(qp);
                flag++;
            }
            else if ( (rp= queue_dequeue(&JSON_Q,0)) != 0 )
            {
                jsonstr = rp->argstr;
                if ( Debuglevel > 3 )
                    printf("dequeue JSON_Q.(%s)\n",jsonstr);
                if ( (retstr= call_SuperNET_JSON(jsonstr)) == 0 )
                    retstr = clonestr("{\"result\":null}");
                rp->retstr = retstr;
                if ( rp->txid != 0 )
                    queue_GUIpoll(rp);
                flag++;
            }
        }
        if ( process_storageQ() != 0 )
        {
            //printf("processed storage\n");
        }
    }
#ifndef TIMESCRAMBLE
    while ( (wr= queue_dequeue(&sendQ,0)) != 0 )
    {
        //printf("sendQ size.%d\n",queue_size(&sendQ));
        process_sendQ_item(wr);
    }
#endif
    while ( (up= queue_dequeue(&UDP_Q,0)) != 0 )
        process_udpentry(up);
    if ( millis > (lastclock + 1000) )
    {
        poll_pending_offers(Global_mp->myNXTADDR,Global_mp->srvNXTACCTSECRET);
        //poll_pricedbs();
        every_second(counter);
        retstr = findaddress(0,0,0,0,0,0,0,0,0,0);
        if ( retstr != 0 )
        {
            printf("findaddress completed (%s)\n",retstr);
            free(retstr);
        }
        if ( (counter % 60) == 17 )
        {
            every_minute(counter/60);
            update_Allnodes(0,0);
            poll_telepods("BTCD");
            poll_telepods("BTC");
        }
        counter++;
        lastclock = millis;
    }
    msleep(APISLEEP);
}

void run_UVloop(void *arg)
{
    uv_idle_t idler;
    uv_idle_init(UV_loop,&idler);
    //uv_async_init(UV_loop,&Tasks_async,async_handler);
    uv_idle_start(&idler,SuperNET_idler);
    uv_run(UV_loop,UV_RUN_DEFAULT);
    printf("end of uv_run\n");
}

#ifndef INSTALL_DATADIR
#define INSTALL_DATADIR "~"
#endif
#define LOCAL_RESOURCE_PATH INSTALL_DATADIR
char *resource_path = LOCAL_RESOURCE_PATH;
static volatile int SSL_done;
static volatile int force_exit = 0;

struct serveable
{
	const char *urlpath;
	const char *mimetype;
};

struct per_session_data__http
{
	int fd;
};

void dump_handshake_info(struct libwebsocket *wsi)
{
	int n;
	static const char *token_names[] = {
		/*[WSI_TOKEN_GET_URI]		=*/ "GET URI",
		/*[WSI_TOKEN_POST_URI]		=*/ "POST URI",
		/*[WSI_TOKEN_HOST]		=*/ "Host",
		/*[WSI_TOKEN_CONNECTION]	=*/ "Connection",
		/*[WSI_TOKEN_KEY1]		=*/ "key 1",
		/*[WSI_TOKEN_KEY2]		=*/ "key 2",
		/*[WSI_TOKEN_PROTOCOL]		=*/ "Protocol",
		/*[WSI_TOKEN_UPGRADE]		=*/ "Upgrade",
		/*[WSI_TOKEN_ORIGIN]		=*/ "Origin",
		/*[WSI_TOKEN_DRAFT]		=*/ "Draft",
		/*[WSI_TOKEN_CHALLENGE]		=*/ "Challenge",
        
		/* new for 04 */
		/*[WSI_TOKEN_KEY]		=*/ "Key",
		/*[WSI_TOKEN_VERSION]		=*/ "Version",
		/*[WSI_TOKEN_SWORIGIN]		=*/ "Sworigin",
        
		/* new for 05 */
		/*[WSI_TOKEN_EXTENSIONS]	=*/ "Extensions",
        
		/* client receives these */
		/*[WSI_TOKEN_ACCEPT]		=*/ "Accept",
		/*[WSI_TOKEN_NONCE]		=*/ "Nonce",
		/*[WSI_TOKEN_HTTP]		=*/ "Http",
        
		"Accept:",
		"If-Modified-Since:",
		"Accept-Encoding:",
		"Accept-Language:",
		"Pragma:",
		"Cache-Control:",
		"Authorization:",
		"Cookie:",
		"Content-Length:",
		"Content-Type:",
		"Date:",
		"Range:",
		"Referer:",
		"Uri-Args:",
        
		/*[WSI_TOKEN_MUXURL]	=*/ "MuxURL",
	};
	char buf[256];
    
	for (n = 0; n < sizeof(token_names) / sizeof(token_names[0]); n++) {
		if (!lws_hdr_total_length(wsi, n))
			continue;
        
		lws_hdr_copy(wsi, buf, sizeof buf, n);
        
		fprintf(stderr, "    %s = %s\n", token_names[n], buf);
	}
}

const char * get_mimetype(const char *file)
{
	int n = (int)strlen(file);
    
	if (n < 5)
		return NULL;
    
	if (!strcmp(&file[n - 4], ".ico"))
		return "image/x-icon";
    
	if (!strcmp(&file[n - 4], ".png"))
		return "image/png";
    
	if (!strcmp(&file[n - 5], ".html"))
		return "text/html";
    
	return NULL;
}

void return_http_str(struct libwebsocket *wsi,uint8_t *retstr,int32_t retlen,char *insertstr,char *mediatype)
{
    int32_t len;
    unsigned char buffer[8192];
    len = retlen;
    if ( retlen == 0 )
        return;
    if ( insertstr != 0 && insertstr[0] != 0 )
        len += (int32_t)strlen(insertstr);
    sprintf((char *)buffer,
            "HTTP/1.0 200 OK\r\n"
            "Server: SuperNET\r\n"
            "Content-Type: %s\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Headers: Authorization, Content-Type\r\n"
            "Access-Control-Allow-Credentials: true\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Content-Length: %u\r\n\r\n",
            mediatype,(unsigned int)len);
    //printf("html hdr.(%s)\n",buffer);
    if ( insertstr != 0 && insertstr[0] != 0 )
        strcat((char *)buffer,insertstr);
    libwebsocket_write(wsi,buffer,strlen((char *)buffer),LWS_WRITE_HTTP);
    libwebsocket_write(wsi,(unsigned char *)retstr,retlen,LWS_WRITE_HTTP);
    if ( Debuglevel > 3 )
        printf("SuperNET >>>>>>>>>>>>>> sends back (%s) retlen.%d\n",mediatype,retlen);
}

uint64_t conv_URL64(char *password,char *pin,char *str)
{
    char numstr[64],**pinp,**passwordp;
    int32_t i,j;
    pinp = passwordp = 0;
    for (i=0; str[i]!=0; i++)
    {
        if ( str[i] == '?' )
        {
            *passwordp = (str + i + 1);
            for (j=0; (*passwordp)[j]!=0; j++)
            {
                if ( (*passwordp)[j] == '&' )
                {
                    *pinp = &(*passwordp)[j+1];
                    (*passwordp)[j] = 0;
                    printf("pin.(%s) ",*pinp);
                    break;
                }
            }
            printf("password.(%s)\n",*passwordp);
            break;
        }
        if ( str[i] >= '0' && str[i] <= '9' )
            numstr[i] = str[i];
        else return(0);
    }
    if ( i >= MAX_NXTTXID_LEN )
        return(0);
    numstr[i] = 0;
    if ( passwordp != 0 )
        strcpy(password,*passwordp);
    if ( pinp != 0 )
        strcpy(pin,*pinp);
    return(calc_nxt64bits(numstr));
}

uint64_t html_mappings(cJSON *array,char *fname)
{
    static int didinit,numhtmlmappings;
    static portable_mutex_t mutex;
    static char *mappings[1000];
    static uint64_t URL64s[sizeof(mappings)/sizeof(*mappings)];
    int32_t i,j,n;
    cJSON *item;
    uint64_t URL64 = 0;
    char filename[1024];
    if ( didinit == 0 )
    {
        portable_mutex_init(&mutex);
        didinit = 1;
    }
    if ( array != 0 )
    {
        if ( is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
        {
            printf("n.%d items in array\n",n);
            portable_mutex_lock(&mutex);
            for (i=0; i<n; i++)
            {
                item = cJSON_GetArrayItem(array,i);
                if ( cJSON_GetArraySize(item) == 2 )
                {
                    copy_cJSON(filename,cJSON_GetArrayItem(item,0));
                    URL64 = get_API_nxt64bits(cJSON_GetArrayItem(item,1));
                    printf("%d (%s %llu)\n",i,filename,(long long)URL64);
                    if ( numhtmlmappings > 0 )
                    {
                        for (j=0; j<numhtmlmappings; j++)
                            if ( strcmp(filename,mappings[j]) == 0 )
                                break;
                    } else j = 0;
                    URL64s[j] = URL64;
                    if ( j == numhtmlmappings && numhtmlmappings < (int32_t)(sizeof(mappings)/sizeof(*mappings)) )
                    {
                        mappings[j] = clonestr(filename);
                        numhtmlmappings++;
                    }
                }
            }
            portable_mutex_unlock(&mutex);
        }
    }
    else
    {
        portable_mutex_lock(&mutex);
        if ( numhtmlmappings > 0 )
        {
            for (j=0; j<numhtmlmappings; j++)
                if ( strcmp(fname,mappings[j]) == 0 )
                {
                    URL64 = URL64s[j];
                    break;
                }
        }
        portable_mutex_unlock(&mutex);
    }
    return(URL64);
}

// this protocol server (always the first one) just knows how to do HTTP
static int callback_http(struct libwebsocket_context *context,struct libwebsocket *wsi,enum libwebsocket_callback_reasons reason,void *user,void *in,size_t len)
{
    char *block_on_SuperNET(int32_t blockflag,char *JSONstr);
    static char password[512],pin[64],*filebuf=0;
    static int64_t filelen=0,allocsize=0;
	char buf[MAX_JSON_FIELD],fname[MAX_JSON_FIELD],mediatype[MAX_JSON_FIELD],client_name[1024],client_ip[1024],*retstr,*str,*filestr;
    cJSON *json,*array,*map;
    struct coin_info *cp;
    uint64_t URL64;
    if ( 0 && len != 0 )
        printf("LWS_CALLBACK_HTTP.%d reason.%d len.%ld\n",LWS_CALLBACK_HTTP,reason,len);
    strcpy(mediatype,"text/html");
    switch ( reason )
    {
        case LWS_CALLBACK_HTTP:
            if ( len < 1 )
            {
                //libwebsockets_return_http_status(context, wsi,HTTP_STATUS_BAD_REQUEST, NULL);
                return -1;
            }
            // if a legal POST URL, let it continue and accept data
            if ( lws_hdr_total_length(wsi,WSI_TOKEN_POST_URI) != 0 )
                return 0;
            str = malloc(len+1);
            memcpy(str,(void *)((long)in + 1),len-1);
            str[len-1] = 0;
            convert_percent22(str);
            if ( Debuglevel > 1 )
                printf("RPC GOT.(%s) from user.(%s)\n",str,user);
            if ( str[0] == 0 || (str[0] != '[' && str[0] != '{') )
            {
                URL64 = 0;
                buf[0] = 0;
                if ( str[0] == 0 )
                    strcpy(fname,"html/index.html");
                else if ( (URL64= html_mappings(0,str)) == 0 )
                {
                    if ( (URL64= conv_URL64(password,pin,str)) == 0 )
                        sprintf(fname,"html/%s",str);
                } else printf("MAPPED.(%s) -> %llu\n",str,(long long)URL64);
                if ( strcmp(str,"supernet.js") == 0 )
                {
                    strcpy(mediatype,"application/javascript");
                    if ( (cp= get_coin_info("BTCD")) != 0 )
                        sprintf(buf,"var authToken = btoa(\"%s\");",cp->userpass);
                }
                printf("FNAME.(%s) URL64.%llu str.(%s)\n",fname,(long long)URL64,str);
                if ( URL64 != 0 )
                {
                    filestr = load_URL64(&map,mediatype,URL64,&filebuf,&filelen,&allocsize,password,pin);
                    if ( map != 0 )
                    {
                        html_mappings(map,0);
                        free_json(map);
                    }
                }
                else filestr = load_file(fname,&filebuf,&filelen,&allocsize);
                if ( filestr != 0 )
                    return_http_str(wsi,(uint8_t *)filestr,(int32_t)filelen,buf,mediatype);
                else return_http_str(wsi,(uint8_t *)str,(int32_t)strlen(str),"ERROR LOADING: ",mediatype);
            }
            else
            {
                if ( (json= cJSON_Parse(str)) != 0 )
                {
                    retstr = block_on_SuperNET(is_BTCD_command(json) == 0,str);
                    if ( retstr != 0 )
                    {
                        return_http_str(wsi,(uint8_t *)retstr,(int32_t)strlen(retstr),0,"text/html");
                        free(retstr);
                    }
                    free_json(json);
                } else printf("couldnt parse.(%s)\n",str);
            }
            free(str);
            return(-1);
            break;
        case LWS_CALLBACK_HTTP_BODY:
            str = malloc(len+1), memcpy(str,in,len), str[len] = 0;
            //printf("(%s)\n",str);
            //if ( wsi != 0 )
            //dump_handshake_info(wsi);
            if ( Debuglevel > 3 && strcmp("{\"requestType\":\"BTCDpoll\"}",str) != 0 )
                fprintf(stderr,">>>>>>>>>>>>>> SuperNET received RPC.(%s) wsi.%p user.%p\n",str,wsi,user);
            //>>>>>>>>>>>>>> SuperNET received RPC.({"requestType":"BTCDjson","json":{\"requestType\":\"getpeers\"}})
            //{"jsonrpc": "1.0", "id":"curltest", "method": "SuperNET", "params": ["{\"requestType\":\"getpeers\"}"]  }
            if ( (json= cJSON_Parse(str)) != 0 )
            {
                if ( (array= cJSON_GetObjectItem(json,"params")) != 0 && is_cJSON_Array(array) != 0 )
                {
                    copy_cJSON(buf,cJSON_GetArrayItem(array,0));
                    unstringify(buf);
                    stripwhite_ns(buf,strlen(buf));
                    retstr = block_on_SuperNET(1,buf);
                }
                else retstr = block_on_SuperNET(is_BTCD_command(json) == 0,str);
                if ( retstr != 0 )
                {
                    return_http_str(wsi,(uint8_t *)retstr,(int32_t)strlen(retstr),0,mediatype);
                    free(retstr);
                }
                free_json(json);
            }
            else return_http_str(wsi,(uint8_t *)str,(int32_t)strlen(str),0,mediatype);
            free(str);
            return(-1);
            break;
        case LWS_CALLBACK_HTTP_BODY_COMPLETION: // the whole sent body arried, close the connection
            //lwsl_notice("LWS_CALLBACK_HTTP_BODY_COMPLETION\n");
            //libwebsockets_return_http_status(context, wsi,HTTP_STATUS_OK, NULL);
            return -1;
        case LWS_CALLBACK_HTTP_FILE_COMPLETION:     // kill the connection after we sent one file
            //		lwsl_info("LWS_CALLBACK_HTTP_FILE_COMPLETION seen\n");
            return -1;
        case LWS_CALLBACK_HTTP_WRITEABLE:           // we can send more of whatever it is we were sending
            /*flush_bail:
             if ( lws_send_pipe_choked(wsi) == 0 )   // true if still partial pending
             {
             libwebsocket_callback_on_writable(context, wsi);
             break;
             }
             bail:
             close(pss->fd);*/
            return -1;
            /*
             * callback for confirming to continue with client IP appear in
             * protocol 0 callback since no websocket protocol has been agreed
             * yet.  You can just ignore this if you won't filter on client IP
             * since the default uhandled callback return is 0 meaning let the
             * connection continue.
             */
        case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
            if ( ENABLE_EXTERNALACCESS == 0 )
            {
                libwebsockets_get_peer_addresses(context, wsi, (int)(long)in, client_name,sizeof(client_name), client_ip, sizeof(client_ip));
                // if we returned non-zero from here, we kill the connection
                if ( strcmp("127.0.0.1",client_ip) != 0  && strncmp("192.168.",client_ip,strlen("192.168.")) != 0 && (is_whitelisted(client_ip) == 0 && (Global_mp->gatewayid >= 0 || Global_mp->iambridge != 0)) )
                {
                    if ( Debuglevel > 0 )
                        fprintf(stderr, "Received network connect from %s (%s)\n",client_name, client_ip);
                    return(-1);
                }
            }
            break;
        case LWS_CALLBACK_GET_THREAD_ID:
            /*
             * if you will call "libwebsocket_callback_on_writable"
             * from a different thread, return the caller thread ID
             * here so lws can use this information to work out if it
             * should signal the poll() loop to exit and restart early
             */
            /* return pthread_getthreadid_np(); */
            break;
        default:
            break;
	}
	return 0;
}

static struct libwebsocket_protocols protocols[] =
{
	// first protocol must always be HTTP handler
    
	{
		"http-only",		// name
		callback_http,		// callback
		sizeof (struct per_session_data__http),	// per_session_data_size
		0,			// max frame size / rx buffer
	},
    /*{
		"websocket",
		callback_websockets,
		sizeof(struct per_session_data),
		8192,
	},*/
    { NULL, NULL, 0, 0 } // terminator
};

void sighandler(int sig)
{
	force_exit = 1;
	//libwebsocket_cancel_service(context);
}

int32_t init_API_port(int32_t use_ssl,uint16_t port,uint32_t millis)
{
	int n,opts = 0;
	const char *iface = NULL;
	static struct lws_context_creation_info infos[2];
    struct lws_context_creation_info *info;
    while ( Finished_init == 0 )
        fprintf(stderr,"."), portable_sleep(1);
    info = &infos[use_ssl];
	memset(info,0,sizeof(*info));
	info->port = port;
	lwsl_notice("libwebsockets test server - (C) Copyright 2010-2013 Andy Green <andy@warmcat.com> -  licensed under LGPL2.1\n");
	info->iface = iface;
	info->protocols = protocols;
 	info->gid = -1;
	info->uid = -1;
	info->options = opts;
	if ( use_ssl == 0 )
    {
		info->ssl_cert_filepath = NULL;
		info->ssl_private_key_filepath = NULL;
	}
    else
    {
		char cert_path[1024];
        char key_path[1024];
		sprintf(cert_path,"SuperNET.pem");
		if (strlen(resource_path) > sizeof(key_path) - 32)
        {
			lwsl_err("resource path too long\n");
			return -1;
		}
		sprintf(key_path,"SuperNET.key.pem");
		info->ssl_cert_filepath = cert_path;
		info->ssl_private_key_filepath = key_path;
	}
	LWScontext = libwebsocket_create_context(info);
	if ( LWScontext == NULL )
    {
		lwsl_err("libwebsocket init failed\n");
		return -1;
	}
	n = 0;
    SSL_done |= (1 << use_ssl);
	while ( n >= 0 && !force_exit )
    {
        //libwebsocket_callback_on_writable_all_protocol(&protocols[1]);
        n = libwebsocket_service(LWScontext,millis);
        msleep(1);
	}
	libwebsocket_context_destroy(LWScontext);
	lwsl_notice("libwebsockets-test-server exited cleanly\n");
	return 0;
}

void run_libwebsockets(void *arg)
{
    int32_t usessl = *(int32_t *)arg;
    init_API_port(USESSL,APIPORT+!usessl*0,APISLEEP);
}

void init_NXThashtables(struct NXThandler_info *mp)
{
    struct NXT_acct *np = 0;
    struct NXT_asset *ap = 0;
    struct NXT_assettxid *tp = 0;
    struct coin_txidind *ctp = 0;
    struct coin_txidmap *map = 0;
    struct withdraw_info *wp = 0;
    struct pserver_info *pp = 0;
    struct telepathy_entry *tel = 0;
    struct transfer_args *args = 0;
    static struct hashtable *Pendings,*NXTasset_txids,*NXTaddrs,*NXTassets,*Pserver,*Telepathy_hash,*Redeems,*Coin_txidinds,*Coin_txidmap;
    if ( NXTasset_txids == 0 )
        NXTasset_txids = hashtable_create("NXTasset_txids",HASHTABLES_STARTSIZE,sizeof(struct NXT_assettxid),((long)&tp->H.U.txid[0] - (long)tp),sizeof(tp->H.U.txid),((long)&tp->H.modified - (long)tp));
    if ( NXTassets == 0 )
        NXTassets = hashtable_create("NXTassets",HASHTABLES_STARTSIZE,sizeof(struct NXT_asset),((long)&ap->H.U.assetid[0] - (long)ap),sizeof(ap->H.U.assetid),((long)&ap->H.modified - (long)ap));
    if ( NXTaddrs == 0 )
        NXTaddrs = hashtable_create("NXTaddrs",HASHTABLES_STARTSIZE,sizeof(struct NXT_acct),((long)&np->H.U.NXTaddr[0] - (long)np),sizeof(np->H.U.NXTaddr),((long)&np->H.modified - (long)np));
    if ( Telepathy_hash == 0 )
        Telepathy_hash = hashtable_create("Telepath_hash",HASHTABLES_STARTSIZE,sizeof(struct telepathy_entry),((long)&tel->locationstr[0] - (long)tel),sizeof(tel->locationstr),((long)&tel->modified - (long)tel));
    if ( Pserver == 0 )
        Pserver = hashtable_create("Pservers",HASHTABLES_STARTSIZE,sizeof(struct pserver_info),((long)&pp->ipaddr[0] - (long)pp),sizeof(pp->ipaddr),((long)&pp->modified - (long)pp));
    if ( Redeems == 0 )
        Redeems = hashtable_create("Redeems",HASHTABLES_STARTSIZE,sizeof(struct withdraw_info),((long)&wp->redeemtxid[0] - (long)wp),sizeof(wp->redeemtxid),((long)&wp->modified - (long)wp));
    if ( Coin_txidinds == 0 )
        Coin_txidinds = hashtable_create("Coin_txidinds",HASHTABLES_STARTSIZE,sizeof(struct coin_txidind),((long)&ctp->indstr[0] - (long)ctp),sizeof(ctp->indstr),((long)&ctp->modified - (long)ctp));
    if ( Coin_txidmap == 0 )
        Coin_txidmap = hashtable_create("Coin_txidmap",HASHTABLES_STARTSIZE,sizeof(struct coin_txidmap),((long)&map->txidmapstr[0] - (long)map),sizeof(map->txidmapstr),((long)&map->modified - (long)map));
    if ( Pendings == 0 )
        Pendings = hashtable_create("Pendings",HASHTABLES_STARTSIZE,sizeof(struct transfer_args),((long)&args->hashstr[0] - (long)args),sizeof(args->hashstr),((long)&args->modified - (long)args));
    if ( mp != 0 )
    {
        mp->pending_xfers = &Pendings;
        mp->Telepathy_tablep = &Telepathy_hash;
        mp->Pservers_tablep = &Pserver;
        mp->redeemtxids = &Redeems;
        mp->NXTaccts_tablep = &NXTaddrs;
        mp->NXTassets_tablep = &NXTassets;
        mp->NXTasset_txids_tablep = &NXTasset_txids;
        mp->coin_txidinds = &Coin_txidinds;
        mp->coin_txidmap = &Coin_txidmap;
        //printf("init_NXThashtables: %p %p %p %p\n",NXTguids,NXTaddrs,NXTassets,NXTasset_txids);
    }
}

void *init_SuperNET_globals()
{
    struct NXT_str *tp = 0;
    Global_mp = calloc(1,sizeof(*Global_mp));
    curl_global_init(CURL_GLOBAL_ALL); //init the curl session
    if ( Global_pNXT == 0 )
    {
        Global_pNXT = calloc(1,sizeof(*Global_pNXT));
        orderbook_txids = hashtable_create("orderbook_txids",HASHTABLES_STARTSIZE,sizeof(struct NXT_str),((long)&tp->U.txid[0] - (long)tp),sizeof(tp->U.txid),((long)&tp->modified - (long)tp));
        Global_pNXT->orderbook_txidsp = &orderbook_txids;
        Global_pNXT->msg_txids = hashtable_create("msg_txids",HASHTABLES_STARTSIZE,sizeof(struct NXT_str),((long)&tp->U.txid[0] - (long)tp),sizeof(tp->U.txid),((long)&tp->modified - (long)tp));
    }
    portable_mutex_init(&Global_mp->hash_mutex);
    portable_mutex_init(&Global_mp->hashtable_queue[0].mutex);
    portable_mutex_init(&Global_mp->hashtable_queue[1].mutex);
    
    init_NXThashtables(Global_mp);
    Global_mp->upollseconds = 333333 * 0;
    Global_mp->pollseconds = POLL_SECONDS;
    if ( portable_thread_create((void *)process_hashtablequeues,Global_mp) == 0 )
        printf("ERROR hist process_hashtablequeues\n");
    return(Global_mp);
}

char *init_NXTservices(char *JSON_or_fname,char *myipaddr)
{
    //static int32_t zero;//,one = 1;
    struct coin_info *cp;
    struct NXThandler_info *mp = Global_mp;    // seems safest place to have main data structure
    if ( Debuglevel > 0 )
       printf("init_NXTservices.(%s)\n",myipaddr);
    UV_loop = uv_default_loop();
    if ( 0 )
    {
        //uint32_t before,after;
        //int32_t numinputs;
        struct cointx_info *_decode_rawtransaction(char *hexstr,int oldtx);
        struct cointx_info *cointx;
        char *rawtx = clonestr("0100000074fc77540156a5b19aaada0496780b1fbce72f7647da5f940883da7ee5d5774f673c6703c401000000fdfd0000483045022100f0b26a43136af6c28d381f461a9fcd30309788456cb81784dbc68ee85ae4151d022036fc96c4edd7b87e762bb139d5d34838a88054c37173236236f72940b2e5309801473044022068ae115a397d9a6f462b78416d58582736fa38ecc4eab2c26759e1e58d6326bc02204e148ab84d49b0f1c8aab1033819ad90c536c604ce24dab419013a5914d39d8a014c695221035827b3c432eb5a528a21657d36a1b61dd85078a6ba5f328bed2d928c173a46c421024ae5e013fda966cf8544025534012156f84a40a5672a894c42e144b0664202502102acdb9c782d499de9b98e8b166fc22bd68895e2293cb49e4a2e071f1254d1a7aa53aeffffffff0340420f00000000001976a9148466f34f39c23547abf922d422e3e5322fdf156588ac20cd8800020000001976a914cd073e0a5d4225f2577113400c3abf9ac1ad2cc488ac60c791e50600000017a914194a1499c343beefe3127e041f480ee4aef058408700000000");
        //before = extract_sequenceid(&numinputs,get_coin_info("BTCD"),rawtx,0);
        // replace_bitcoin_sequenceid(get_coin_info("BTCD"),rawtx,12345678);
        //after = extract_sequenceid(&numinputs,get_coin_info("BTCD"),rawtx,0);
        //printf("newtx.(%s) before.%u after.%u\n",rawtx,before,after); getchar();
        if ( (cointx= _decode_rawtransaction(rawtx,0)) != 0 )
        {
            free(cointx);
        }
        //getchar();
    }
    myipaddr = init_MGWconf(JSON_or_fname,myipaddr);
    //setenv("PYTHONHOME",PYTHONPATH,1);
    //strcat(PYTHONPATH,"/Lib");
    //setenv("PYTHONPATH",PYTHONPATH,1);
    /*if ( 0 )
    {
        struct cointx_info *_decode_rawtransaction(char *hexstr,int32_t oldtx);
        void *ret = _decode_rawtransaction("0100000001a131c270d541c9d2be98b6f7a88c6cbea5d5a395ec82c9954083675226f399ee0300000000ffffffff042f7500000000000017a9140cc0def37d9682c292d18b3f579b7432adf4703187a0f70300000000001976a914e8bf7b6c41702de3451d189db054c985fe6fbbdb88ac01000000000000001976a914f9fab825f93c5f0ddcf90c4c96c371dc3dbca95788ac10eb09000000000017a914309924e8dad854d4cb8e3d6b839a932aea22590c8700000000",1);
        printf(">>>>>>>>>>>>>>>>>>>>> ret = %p\n",ret);
        getchar();
    }*/
 //if ( IS_LIBTEST == 7 )
    //    return(myipaddr);
    if ( IS_LIBTEST != 7 )
    {
        mp->udp = start_libuv_udpserver(4,SUPERNET_PORT,on_udprecv);
        if ( (cp= get_coin_info("BTCD")) != 0 && cp->bridgeport != 0 )
            cp->bridgeudp = start_libuv_udpserver(4,cp->bridgeport,on_bridgerecv);
    }
    if ( myipaddr != 0 )
        strcpy(mp->ipaddr,myipaddr);
    //if ( IS_LIBTEST > 1 && portable_thread_create((void *)Coinloop,0) == 0 && IS_LIBTEST != 7 )
     //   printf("ERROR hist Coinloop SSL\n");
    Finished_loading = 1;
    if ( Debuglevel > 0 )
        printf("run_UVloop\n");
    //portable_sleep(3);
     {
        struct coin_info *cp;
        while ( (cp= get_coin_info("BTCD")) == 0 )
        {
            printf("no BTCD coin info\n");
            portable_sleep(10);
        }
        parse_ipaddr(cp->myipaddr,myipaddr);
        bind_NXT_ipaddr(cp->srvpubnxtbits,myipaddr);
        if ( IS_LIBTEST > 0 )//&& IS_LIBTEST < 7 )
        {
            void *process_ramchains(void *_argcoinstr);
            if ( Global_mp->gatewayid >= 0 )
                init_SuperNET_storage(cp->backupdir);
#ifdef later
            if ( IS_LIBTEST > 0 && IS_LIBTEST < 7 && NORAMCHAINS == 0 && portable_thread_create((void *)process_ramchains,0) == 0 )
                printf("ERROR hist run_libwebsockets\n");
#endif
        }
        if ( portable_thread_create((void *)run_UVloop,Global_mp) == 0 )
            printf("ERROR hist process_hashtablequeues\n");
    }
    return(myipaddr);
}

char *call_SuperNET_JSON(char *JSONstr)
{
    cJSON *json,*array; int32_t valid; uint64_t tag; char NXTaddr[64],numstr[64],_tokbuf[2*MAX_JSON_FIELD],encoded[NXT_TOKEN_LEN+1],*cmdstr,*retstr = 0;
    struct coin_info *cp = get_coin_info("BTCD");
    if ( Finished_init == 0 )
    {
        printf("Finished_init still 0\n");
        return(clonestr("{\"result\":null}"));
    }
//printf("got call_SuperNET_JSON.(%s)\n",JSONstr);
    if ( cp != 0 && (json= cJSON_Parse(JSONstr)) != 0 )
    {
        expand_nxt64bits(NXTaddr,cp->srvpubnxtbits);
        if ( cJSON_GetObjectItem(json,"NXT") == 0 )
            cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(NXTaddr));
        if ( cJSON_GetObjectItem(json,"pubkey") == 0 )
            cJSON_AddItemToObject(json,"pubkey",cJSON_CreateString(Global_mp->pubkeystr));
        if ( cJSON_GetObjectItem(json,"timestamp") == 0 )
            cJSON_AddItemToObject(json,"timestamp",cJSON_CreateNumber(time(NULL)));
        if ( cJSON_GetObjectItem(json,"tag") == 0 )
        {
            randombytes((void *)&tag,sizeof(tag));
            sprintf(numstr,"%llu",(long long)tag), cJSON_AddItemToObject(json,"tag",cJSON_CreateString(numstr));
        }
        cmdstr = cJSON_Print(json);
        if ( cmdstr != 0 )
        {
            stripwhite_ns(cmdstr,strlen(cmdstr));
            issue_generateToken(0,encoded,cmdstr,cp->srvNXTACCTSECRET);
            encoded[NXT_TOKEN_LEN] = 0;
            sprintf(_tokbuf,"[%s,{\"token\":\"%s\"}]",cmdstr,encoded);
            free(cmdstr);
            //printf("made tokbuf.(%s)\n",_tokbuf);
            array = cJSON_Parse(_tokbuf);
            if ( array != 0 )
            {
                cmdstr = verify_tokenized_json(0,NXTaddr,&valid,array);
                //printf("cmdstr.%s valid.%d\n",cmdstr,valid);
                retstr = SuperNET_json_commands(Global_mp,0,array,NXTaddr,valid,_tokbuf);
                //printf("json command return.(%s)\n",retstr);
                if ( cmdstr != 0 )
                    free(cmdstr);
                free_json(array);
            } else printf("couldnt parse tokbuf.(%s)\n",_tokbuf);
        }
        free_json(json);
    } else printf("couldnt parse (%s)\n",JSONstr);
    if ( retstr == 0 )
        retstr = clonestr("{\"result\":null}");
    return(retstr);
}

char *block_on_SuperNET(int32_t blockflag,char *JSONstr)
{
    char **ptrs,*retstr,retbuf[1024];
    uint64_t txid = 0;
    ptrs = calloc(5,sizeof(*ptrs));
    if ( sizeof(struct queueitem) != 2*sizeof(ptrs[0]) )
        fatal("unexpected queueitem size");
    ptrs[2] = clonestr(JSONstr);
    if ( blockflag == 0 )
    {
        txid = calc_txid((uint8_t *)JSONstr,(int32_t)strlen(JSONstr));
        ptrs[4] = (char *)txid;
    }
    if ( Debuglevel > 3 )
        printf("block.%d QUEUE.(%s)\n",blockflag,JSONstr);
    queue_enqueue("JSON_Q",&JSON_Q,(void *)ptrs);
    if ( blockflag != 0 )
    {
        while ( (retstr= ptrs[3]) == 0 )
            usleep(1000);
        if ( ptrs[2] != 0 )
            free(ptrs[2]);
        free(ptrs);
        //printf("block.%d returned.(%s)\n",blockflag,retstr);
        return(retstr);
    }
    else
    {
        sprintf(retbuf,"{\"result\":\"pending SuperNET API call\",\"txid\":\"%llu\"}",(long long)txid);
        //printf("queue.%d returned.(%s)\n",blockflag,retbuf);
        return(clonestr(retbuf));
    }
}

char *SuperNET_JSON(char *JSONstr)
{
    char *retstr = 0;
    struct coin_info *cp = get_coin_info("BTCD");
    cJSON *json;
    if ( Finished_init == 0 )
        return(0);
    if ( Debuglevel > 1 )
        printf("got JSON.(%s)\n",JSONstr);
    if ( cp != 0 && (json= cJSON_Parse(JSONstr)) != 0 )
    {
        if ( 1 && is_BTCD_command(json) != 0 ) // deadlocks as the SuperNET API came from locked BTCD RPC
        {
            //if ( Debuglevel > 1 )
            //    printf("is_BTCD_command\n");
            return(block_on_SuperNET(0,JSONstr));
        }
        else retstr = block_on_SuperNET(1,JSONstr);
        free_json(json);
    } else printf("couldnt parse (%s)\n",JSONstr);
    if ( retstr == 0 )
        retstr = clonestr("{\"result\":null}");
    return(retstr);
}

uint64_t call_SuperNET_broadcast(struct pserver_info *pserver,char *msg,int32_t len,int32_t duration)
{
    int32_t SuperNET_broadcast(char *msg,int32_t duration);
    int32_t SuperNET_narrowcast(char *destip,unsigned char *msg,int32_t len);
    char ip_port[64],*ptr;
    uint64_t txid = 0;
    int32_t port;
    if ( pserver != 0 && strcmp(pserver->ipaddr,"0.0.0.0") == 0 )
        return(0);
    if ( 0 && SUPERNET_PORT != _SUPERNET_PORT )
        return(0);
    if ( Debuglevel > 2 )
        printf("call_SuperNET_broadcast.%p %p len.%d\n",pserver,msg,len);
    txid = calc_txid((uint8_t *)msg,(int32_t)strlen(msg));
    if ( pserver != 0 )
    {
        port = (pserver->p2pport == 0) ? BTCD_PORT : pserver->p2pport;
        //fprintf(stderr,"port.%d\n",port);
        sprintf(ip_port,"%s:%d",pserver->ipaddr,port);
        txid ^= calc_ipbits(pserver->ipaddr);
        if ( Debuglevel > 1 )
        {
            char debugstr[4096];
            init_hexbytes_noT(debugstr,(uint8_t *)msg,len);
            debugstr[32] = 0;
            fprintf(stderr,"%s NARROWCAST.(%s) txid.%llu (%s) %llu\n",pserver->ipaddr,debugstr,(long long)txid,ip_port,(long long)pserver->nxt64bits);
        }
        ptr = calloc(1,sizeof(struct queueitem) + 64 + sizeof(len) + len + 1);
        memcpy(&ptr[sizeof(struct queueitem)],&len,sizeof(len));
        memcpy(&ptr[sizeof(struct queueitem) + sizeof(len)],ip_port,strlen(ip_port));
        memcpy(&ptr[sizeof(struct queueitem) + sizeof(len) + 64],msg,len);
        queue_enqueue("NarrowQ",&NarrowQ,(void *)ptr);
        return(txid);
    }
    else
    {
        char *cmdstr,NXTaddr[64];
        cJSON *array;
        int32_t valid;
        array = cJSON_Parse(msg);
        if ( array != 0 )
        {
            cmdstr = verify_tokenized_json(0,NXTaddr,&valid,array);
            if ( cmdstr != 0 )
                free(cmdstr);
            free_json(array);
            if ( Debuglevel > 2 )
            {
                char debugstr[4096];
                init_hexbytes_noT(debugstr,(uint8_t *)msg,len);
                debugstr[32] = 0;
                printf("BROADCAST parms.(%s) valid.%d duration.%d txid.%llu len.%d\n",debugstr,valid,duration,(long long)txid,len);
            }
            ptr = calloc(1,sizeof(struct queueitem) + sizeof(len) + sizeof(duration) + len + 1);
            memcpy(&ptr[sizeof(struct queueitem)],&len,sizeof(len));
            memcpy(&ptr[sizeof(struct queueitem) + sizeof(len)],&duration,sizeof(duration));
            memcpy(&ptr[sizeof(struct queueitem) + sizeof(len) + sizeof(duration)],msg,len);
            ptr[sizeof(len) + sizeof(duration) + len] = 0;
            queue_enqueue("BroadcastQ",&BroadcastQ,(void *)ptr);
            update_Allnodes(msg,len);
            return(txid);
        } else printf("cant broadcast non-JSON.(%s)\n",msg);
    }
    return(txid);
}

char *SuperNET_gotpacket(char *msg,int32_t duration,char *ip_port)
{
    static int flood,duplicates;
    cJSON *json;
    uint16_t p2pport;
    struct pserver_info *pserver;
    uint64_t txid;
    struct sockaddr prevaddr;
    int32_t len,createdflag,valid;
    unsigned char packet[2*MAX_JSON_FIELD];
    char ipaddr[64],txidstr[64],retjsonstr[2*MAX_JSON_FIELD],verifiedNXTaddr[64],*cmdstr,*retstr;
    if ( Debuglevel > 1 )
        printf("gotpacket.(%s) duration.%d from (%s) | (%d vs %d)\n",msg,duration,ip_port,SUPERNET_PORT,_SUPERNET_PORT);
    if ( 0 && SUPERNET_PORT != _SUPERNET_PORT )
        return(clonestr("{\"error\":private SuperNET}"));
    strcpy(retjsonstr,"{\"result\":null}");
    if ( Finished_loading == 0 )
    {
        if ( is_hexstr(msg) == 0 )
        {
            //printf("QUEUE.(%s)\n",msg);
            return(block_on_SuperNET(0,msg));
        }
        return(clonestr(retjsonstr));
    }
    p2pport = parse_ipaddr(ipaddr,ip_port);
    uv_ip4_addr(ipaddr,0,(struct sockaddr_in *)&prevaddr);
    pserver = get_pserver(0,ipaddr,0,p2pport);
    len = (int32_t)strlen(msg);
    if ( is_hexstr(msg) != 0 )
    {
        len >>= 1;
        len = decode_hex(packet,len,msg);
        txid = calc_txid(packet,len);//hash,sizeof(hash));
        sprintf(txidstr,"%llu",(long long)txid);
        MTadd_hashtable(&createdflag,&Global_pNXT->msg_txids,txidstr);
        if ( createdflag == 0 )
        {
            duplicates++;
            return(clonestr("{\"error\":\"duplicate msg\"}"));
        }
        if ( (len<<1) == 30 ) // hack against flood
            flood++;
        if ( Debuglevel > 0 )
            printf("gotpacket %d | Finished_loading.%d | flood.%d duplicates.%d\n",duration,Finished_loading,flood,duplicates);
        if ( is_encrypted_packet(packet,len) != 0 )
            process_packet(0,retjsonstr,packet,len,0,&prevaddr,ipaddr,0);
        /*else if ( (obookid= is_orderbook_tx(packet,len)) != 0 )
        {
            if ( update_orderbook_tx(1,obookid,(struct orderbook_tx *)packet,txid) == 0 )
            {
                ((struct orderbook_tx *)packet)->txid = txid;
                sprintf(retjsonstr,"{\"result\":\"SuperNET_gotpacket got obbokid.%llu packet txid.%llu\"}",(long long)obookid,(long long)txid);
            } else sprintf(retjsonstr,"{\"result\":\"SuperNET_gotpacket error updating obookid.%llu\"}",(long long)obookid);
        }*/ else sprintf(retjsonstr,"{\"error\":\"SuperNET_gotpacket cant find obookid\"}");
    }
    else
    {
        txid = calc_txid((uint8_t *)msg,len);//hash,sizeof(hash));
        sprintf(txidstr,"%llu",(long long)txid);
        MTadd_hashtable(&createdflag,&Global_pNXT->msg_txids,txidstr);
        if ( Debuglevel > 1 )
            printf("C SuperNET_gotpacket from %s:%d size.%d ascii txid.%llu | flood.%d created.%d\n",ipaddr,p2pport,len,(long long)txid,flood,createdflag);
        if ( createdflag == 0 )
        {
            duplicates++;
            return(clonestr("{\"error\":\"duplicate msg\"}"));
        }
        if ( len == 30 ) // hack against flood
            flood++;
        if ( (json= cJSON_Parse((char *)msg)) != 0 )
        {
            cJSON *argjson;
            char pubkeystr[MAX_JSON_FIELD];
            cmdstr = verify_tokenized_json(0,verifiedNXTaddr,&valid,json);
            pubkeystr[0] = 0;
            if ( (argjson= cJSON_Parse(cmdstr)) != 0 )
            {
                copy_cJSON(pubkeystr,cJSON_GetObjectItem(argjson,"pubkey"));
                if ( pubkeystr[0] != 0 )
                    add_new_node(calc_nxt64bits(verifiedNXTaddr));
                free_json(argjson);
            } else printf("update parse error (%s) (%s:%d) %s\n",verifiedNXTaddr,ipaddr,p2pport,pubkeystr);
            kademlia_update_info(verifiedNXTaddr,ipaddr,p2pport,pubkeystr,(int32_t)time(NULL),1);
            retstr = SuperNET_json_commands(Global_mp,ipaddr,json,verifiedNXTaddr,valid,(char *)msg);
            if ( cmdstr != 0 )
                free(cmdstr);
            free_json(json);
            if ( retstr == 0 )
                retstr = clonestr("{\"result\":null}");
            return(retstr);
        } else printf("cJSON_Parse error.(%s)\n",msg);
    }
    return(clonestr(retjsonstr));
}

int SuperNET_start(char *JSON_or_fname,char *myipaddr)
{
    FILE *fp = 0;
    struct coin_info *cp;
    int32_t i,creatededflag;
    //myipaddr = clonestr("[2607:5300:100:200::b1d]:14631");
    //myipaddr = clonestr("[2001:16d8:dd24:0:86c9:681e:f931:256]");
    if ( myipaddr[0] == '[' )
        myipaddr = clonestr(conv_ipv6(myipaddr));
    if ( myipaddr != 0 )
        myipaddr = clonestr(myipaddr);
    expand_nxt64bits(NXT_ASSETIDSTR,NXT_ASSETID);
    printf("SuperNET_start(%s) %p ipaddr.(%s)\n",JSON_or_fname,myipaddr,myipaddr);
    if ( JSON_or_fname != 0 && JSON_or_fname[0] != '{' )
    {
        fp = fopen(os_compatible_path(JSON_or_fname),"rb");
        if ( fp == 0 )
            return(-1);
        fclose(fp);
    }
    Global_mp = init_SuperNET_globals();
    if ( Debuglevel > 0 )
        printf("call init_NXTservices (%s)\n",myipaddr);
    myipaddr = init_NXTservices(JSON_or_fname,myipaddr);
    //if ( IS_LIBTEST < 7 )
    {
#ifdef later
        uint64_t pendingtxid;
        if ( Global_mp->gatewayid >= 0 )
            ready_to_xferassets(&pendingtxid);
#endif
        //if ( Debuglevel > 0 )
        //    printf("back from init_NXTservices (%s) NXTheight.%d\n",myipaddr,get_NXTheight());
        p2p_publishpacket(0,0);
        if ( (cp= get_coin_info("BTCD")) == 0 || cp->srvNXTACCTSECRET[0] == 0 || cp->srvNXTADDR[0] == 0 )
        {
            fprintf(stderr,"need to have BTCD active and also srvpubaddr\n");
            exit(-1);
        }
        if ( Global_mp->gatewayid >= 0 )
            issue_startForging(0,cp->srvNXTACCTSECRET);
        Global_mp->myNXTADDR = cp->srvNXTADDR;
        Global_mp->nxt64bits = calc_nxt64bits(Global_mp->myNXTADDR);
        SaM_PrepareIndices();
        Historical_done = 1;
        Finished_init = 1;
        //if ( IS_LIBTEST > 1 && Global_mp->gatewayid >= 0 )
        //    register_variant_handler(MULTIGATEWAY_VARIANT,process_directnet_syncwithdraw,MULTIGATEWAY_SYNCWITHDRAW,sizeof(struct batch_info),sizeof(struct batch_info),MGW_whitelist);
        printf("finished addcontact SUPERNET_PORT.%d USESSL.%d\n",SUPERNET_PORT,USESSL);
        for (i=0; i<Numcoins; i++)
        {
            cp = Coin_daemons[i];
            if ( is_active_coin(cp->name) > 0 && cp->assetid[0] != 0 )
            {
                printf("initializing NXT %s asset.%s\n",cp->name,cp->assetid);
                cp->RAM.ap = get_NXTasset(&creatededflag,Global_mp,cp->assetid);
            }
        }
    }
    if ( IS_LIBTEST != 7 )
    {
        static int32_t zero;
        if ( portable_thread_create((void *)run_libwebsockets,&zero) == 0 )
            printf("ERROR hist run_libwebsockets\n");
        portable_sleep(3);
    }
    //char path[1024];
    ensure_directory(SOPHIA_DIR);
    //SuperNET_JSON("{\"requestType\":\"syscall\",\"plugin\":\"sophia\"}");
    init_InstantDEX(calc_nxt64bits(Global_mp->myNXTADDR),1);
    int32_t tmp = 1;
    if ( strncmp(Global_mp->ipaddr,"209",3) != 0 && (Global_mp->gatewayid >= 0 || Global_mp->iambridge != 0) )
        tmp = 0;
    return((tmp << 17) | ((SUPERNET_PORT & 0xffff) << 1) | (USESSL&1));
}
#else

#include <stdio.h>
//
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

#define BUNDLED
#define PLUGINSTR "SuperNET"
#define PLUGNAME(NAME) SuperNET ## NAME
#define STRUCTNAME struct PLUGNAME(_info)
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
#include "plugins/plugin777.c"
#include "plugins/sophia/kv777.c"
#include "plugins/utils/NXT777.c"
#include "plugins/utils/system777.c"
#include "plugins/utils/files777.c"
#undef DEFINES_ONLY

int32_t SuperNET_idle(struct plugin_info *plugin)
{
    return(0);
}

STRUCTNAME SUPERNET;
int32_t Debuglevel;

char *PLUGNAME(_methods)[] = { "install", "plugin", "agent" }; // list of supported methods
char *PLUGNAME(_pubmethods)[] = { "ping", "pong" }; // list of supported methods
char *PLUGNAME(_authmethods)[] = { "setpass" }; // list of supported methods

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

// redirect port on external upnp enabled router to port on *this* host
int upnpredirect(const char* eport, const char* iport, const char* proto, const char* description)
{
    
    //  Discovery parameters
    struct UPNPDev * devlist = 0;
    struct UPNPUrls urls;
    struct IGDdatas data;
    int i;
    char lanaddr[64];	// my ip address on the LAN
    const char* leaseDuration="0";
    
    //  Redirect & test parameters
    char intClient[40];
    char intPort[6];
    char externalIPAddress[40];
    char duration[16];
    int error=0;
    
    //  Find UPNP devices on the network
    if ((devlist=upnpDiscover(2000, 0, 0,0, 0, &error))) {
        struct UPNPDev * device = 0;
        printf("UPNP INIALISED: List of UPNP devices found on the network.\n");
        for(device = devlist; device; device = device->pNext) {
            printf("UPNP INFO: dev [%s] \n\t st [%s]\n",
                   device->descURL, device->st);
        }
    } else {
        printf("UPNP ERROR: no device found - MANUAL PORTMAP REQUIRED\n");
        return 0;
    }
    
    //  Output whether we found a good one or not.
    if((error = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr)))) {
        switch(error) {
            case 1:
                printf("UPNP OK: Found valid IGD : %s\n", urls.controlURL);
                break;
            case 2:
                printf("UPNP WARN: Found a (not connected?) IGD : %s\n", urls.controlURL);
                break;
            case 3:
                printf("UPNP WARN: UPnP device found. Is it an IGD ? : %s\n", urls.controlURL);
                break;
            default:
                printf("UPNP WARN: Found device (igd ?) : %s\n", urls.controlURL);
        }
        printf("UPNP OK: Local LAN ip address : %s\n", lanaddr);
    } else {
        printf("UPNP ERROR: no device found - MANUAL PORTMAP REQUIRED\n");
        return 0;
    }
    
    //  Get the external IP address (just because we can really...)
    if(UPNP_GetExternalIPAddress(urls.controlURL,
                                 data.first.servicetype,
                                 externalIPAddress)!=UPNPCOMMAND_SUCCESS)
        printf("UPNP WARN: GetExternalIPAddress failed.\n");
    else
        printf("UPNP OK: ExternalIPAddress = %s\n", externalIPAddress);
    
    //  Check for existing supernet mapping - from this host and another host
    //  In theory I can adapt this so multiple nodes can exist on same lan and choose a different portmap
    //  for each one :)
    //  At the moment just delete a conflicting portmap and override with the one requested.
    i=0;
    error=0;
    do {
        char index[6];
        char extPort[6];
        char desc[80];
        char enabled[6];
        char rHost[64];
        char protocol[4];
        
        snprintf(index, 6, "%d", i++);
        
        if(!(error=UPNP_GetGenericPortMappingEntry(urls.controlURL,
                                                   data.first.servicetype,
                                                   index,
                                                   extPort, intClient, intPort,
                                                   protocol, desc, enabled,
                                                   rHost, duration))) {
            // printf("%2d %s %5s->%s:%-5s '%s' '%s' %s\n",i, protocol, extPort, intClient, intPort,desc, rHost, duration);
            
            // check for an existing supernet mapping on this host
            if(!strcmp(lanaddr, intClient)) { // same host
                if(!strcmp(protocol,proto)) { //same protocol
                    if(!strcmp(intPort,iport)) { // same port
                        printf("UPNP WARN: existing mapping found (%s:%s)\n",lanaddr,iport);
                        if(!strcmp(extPort,eport)) {
                            printf("UPNP OK: exact mapping already in place (%s:%s->%s)\n", lanaddr, iport, eport);
                            FreeUPNPUrls(&urls);
                            freeUPNPDevlist(devlist);
                            return 1;
                            
                        } else { // delete old mapping
                            printf("UPNP WARN: deleting existing mapping (%s:%s->%s)\n",lanaddr, iport, extPort);
                            if(UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, extPort, proto, rHost))
                                printf("UPNP WARN: error deleting old mapping (%s:%s->%s) continuing\n", lanaddr, iport, extPort);
                            else printf("UPNP OK: old mapping deleted (%s:%s->%s)\n",lanaddr, iport, extPort);
                        }
                    }
                }
            } else { // ipaddr different - check to see if requested port is already mapped
                if(!strcmp(protocol,proto)) {
                    if(!strcmp(extPort,eport)) {
                        printf("UPNP WARN: EXT port conflict mapped to another ip (%s-> %s vs %s)\n", extPort, lanaddr, intClient);
                        if(UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, extPort, proto, rHost))
                            printf("UPNP WARN: error deleting conflict mapping (%s:%s) continuing\n", intClient, extPort);
                        else printf("UPNP OK: conflict mapping deleted (%s:%s)\n",intClient, extPort);
                    }
                }
            }
        } else
            printf("UPNP OK: GetGenericPortMappingEntry() End-of-List (%d entries) \n", i);
    } while(error==0);
    
    //  Set the requested port mapping
    if((i=UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                              eport, iport, lanaddr, description,
                              proto, 0, leaseDuration))!=UPNPCOMMAND_SUCCESS) {
        printf("UPNP ERROR: AddPortMapping(%s, %s, %s) failed with code %d (%s)\n",
               eport, iport, lanaddr, i, strupnperror(i));
        
        FreeUPNPUrls(&urls);
        freeUPNPDevlist(devlist);
        return 0; //error - adding the port map primary failure
    }
    
    if((i=UPNP_GetSpecificPortMappingEntry(urls.controlURL,
                                           data.first.servicetype,
                                           eport, proto, NULL/*remoteHost*/,
                                           intClient, intPort, NULL/*desc*/,
                                           NULL/*enabled*/, duration))!=UPNPCOMMAND_SUCCESS) {
        printf("UPNP ERROR: GetSpecificPortMappingEntry(%s, %s, %s) failed with code %d (%s)\n", eport, iport, lanaddr,
               i, strupnperror(i));
        FreeUPNPUrls(&urls);
        freeUPNPDevlist(devlist);
        return 0; //error - port map wasnt returned by query so likely failed.
    }
    else printf("UPNP OK: EXT (%s:%s) %s redirected to INT (%s:%s) (duration=%s)\n",externalIPAddress, eport, proto, intClient, intPort, duration);
    FreeUPNPUrls(&urls);
    freeUPNPDevlist(devlist);
    return 1; //ok - we are mapped:)
}

int32_t PLUGNAME(_process_json)(char *forwarder,char *sender,int32_t valid,struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag,char *tokenstr)
{
    char *SuperNET_install(char *plugin,char *jsonstr,cJSON *json);
    char *retstr,*resultstr,*methodstr,*destplugin,buf[1024],myipaddr[512];
    uint8_t mysecret[32],mypublic[32];
    FILE *fp;
    int32_t len;
    retbuf[0] = 0;
    //printf("<<<<<<<<<<<< INSIDE PLUGIN.(%s)! (%s) initflag.%d process %s\n",plugin->name,jsonstr,initflag,plugin->name);
    if ( initflag > 0 )
    {
        SUPERNET.ppid = OS_getppid();
        SUPERNET.argjson = cJSON_Duplicate(json,1);
        SUPERNET.disableNXT = get_API_int(cJSON_GetObjectItem(json,"disableNXT"),0);
        SUPERNET.ismainnet = get_API_int(cJSON_GetObjectItem(json,"MAINNET"),1);
        SUPERNET.usessl = get_API_int(cJSON_GetObjectItem(json,"USESSL"),0);
        SUPERNET.NXTconfirms = get_API_int(cJSON_GetObjectItem(json,"NXTconfirms"),10);
        copy_cJSON(SUPERNET.NXTAPIURL,cJSON_GetObjectItem(json,"NXTAPIURL"));
        if ( SUPERNET.NXTAPIURL[0] == 0 )
        {
            if ( SUPERNET.usessl == 0 )
                strcpy(SUPERNET.NXTAPIURL,"http://127.0.0.1:");
            else strcpy(SUPERNET.NXTAPIURL,"https://127.0.0.1:");
            if ( SUPERNET.ismainnet != 0 )
                strcat(SUPERNET.NXTAPIURL,"7876/nxt");
            else strcat(SUPERNET.NXTAPIURL,"6876/nxt");
        }
        copy_cJSON(SUPERNET.userhome,cJSON_GetObjectItem(json,"userdir"));
        if ( SUPERNET.userhome[0] == 0 )
            strcpy(SUPERNET.userhome,"/root");
        copy_cJSON(RAMCHAINS.pullnode,cJSON_GetObjectItem(json,"pullnode"));
        strcpy(SUPERNET.NXTSERVER,SUPERNET.NXTAPIURL);
        strcat(SUPERNET.NXTSERVER,"?requestType");
        copy_cJSON(SUPERNET.myNXTacct,cJSON_GetObjectItem(json,"myNXTacct"));
        if ( SUPERNET.disableNXT == 0 )
            set_account_NXTSECRET(SUPERNET.NXTACCT,SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,sizeof(SUPERNET.NXTACCTSECRET)-1,json,0,0,0);
        else strcpy(SUPERNET.NXTADDR,SUPERNET.myNXTacct);
        SUPERNET.my64bits = conv_acctstr(SUPERNET.NXTADDR);
        copy_cJSON(myipaddr,cJSON_GetObjectItem(json,"myipaddr"));
        if ( myipaddr[0] != 0 || SUPERNET.myipaddr[0] == 0 )
            strcpy(SUPERNET.myipaddr,myipaddr);
        if ( SUPERNET.myipaddr[0] != 0 )
            SUPERNET.myipbits = (uint32_t)calc_ipbits(SUPERNET.myipaddr);
        SUPERNET.mmapflag = get_API_int(cJSON_GetObjectItem(json,"mmapflag"),0);
        //if ( strncmp(SUPERNET.myipaddr,"89.248",5) == 0 )
        //    SUPERNET.iamrelay = get_API_int(cJSON_GetObjectItem(json,"iamrelay"),1*0);
        //else
            SUPERNET.iamrelay = get_API_int(cJSON_GetObjectItem(json,"iamrelay"),0);
        copy_cJSON(SUPERNET.hostname,cJSON_GetObjectItem(json,"hostname"));
        SUPERNET.port = get_API_int(cJSON_GetObjectItem(json,"SUPERNET_PORT"),SUPERNET_PORT);
        SUPERNET.serviceport = get_API_int(cJSON_GetObjectItem(json,"serviceport"),SUPERNET_PORT - 2);
        copy_cJSON(SUPERNET.transport,cJSON_GetObjectItem(json,"transport"));
        if ( SUPERNET.transport[0] == 0 )
            strcpy(SUPERNET.transport,SUPERNET.UPNP == 0 ? "tcp" : "ws");
        sprintf(SUPERNET.lbendpoint,"%s://%s:%u",SUPERNET.transport,SUPERNET.myipaddr,SUPERNET.port + LB_OFFSET);
        sprintf(SUPERNET.relayendpoint,"%s://%s:%u",SUPERNET.transport,SUPERNET.myipaddr,SUPERNET.port + PUBRELAYS_OFFSET);
        sprintf(SUPERNET.globalendpoint,"%s://%s:%u",SUPERNET.transport,SUPERNET.myipaddr,SUPERNET.port + PUBGLOBALS_OFFSET);
        copy_cJSON(SUPERNET.SERVICESECRET,cJSON_GetObjectItem(json,"SERVICESECRET"));
        expand_nxt64bits(SUPERNET.SERVICENXT,conv_NXTpassword(mysecret,mypublic,(uint8_t *)SUPERNET.SERVICESECRET,(int32_t)strlen(SUPERNET.SERVICESECRET)));
        printf("SERVICENXT.%s\n",SUPERNET.SERVICENXT);
        SUPERNET.automatch = get_API_int(cJSON_GetObjectItem(json,"automatch"),3);
#ifndef __linux__
        SUPERNET.UPNP = 1;
#endif
        SUPERNET.gatewayid = get_API_int(cJSON_GetObjectItem(json,"gatewayid"),-1);
        SUPERNET.numgateways = get_API_int(cJSON_GetObjectItem(json,"numgateways"),3);
        SUPERNET.UPNP = get_API_int(cJSON_GetObjectItem(json,"UPNP"),SUPERNET.UPNP);
        SUPERNET.APISLEEP = get_API_int(cJSON_GetObjectItem(json,"APISLEEP"),DEFAULT_APISLEEP);
        SUPERNET.PLUGINTIMEOUT = get_API_int(cJSON_GetObjectItem(json,"PLUGINTIMEOUT"),10000);
        if ( SUPERNET.APISLEEP <= 1 )
            SUPERNET.APISLEEP = 1;
        copy_cJSON(SUPERNET.DATADIR,cJSON_GetObjectItem(json,"DATADIR"));
        if ( SUPERNET.DATADIR[0] == 0 )
            strcpy(SUPERNET.DATADIR,"archive");
        Debuglevel = get_API_int(cJSON_GetObjectItem(json,"debug"),Debuglevel);
        if ( (fp= fopen("libs/websocketd","rb")) != 0 )
        {
            fclose(fp);
            strcpy(SUPERNET.WEBSOCKETD,"libs/websocketd");
        }
        else strcpy(SUPERNET.WEBSOCKETD,"websocketd");
        copy_cJSON(SUPERNET.BACKUPS,cJSON_GetObjectItem(json,"backups"));
        if ( SUPERNET.BACKUPS[0] == 0 )
            strcpy(SUPERNET.BACKUPS,"/tmp");
        copy_cJSON(KV777.PATH,cJSON_GetObjectItem(json,"KV777"));
        copy_cJSON(SOPHIA.PATH,cJSON_GetObjectItem(json,"SOPHIA"));
        copy_cJSON(SOPHIA.RAMDISK,cJSON_GetObjectItem(json,"RAMDISK"));
        if ( SOPHIA.PATH[0] == 0 )
            strcpy(SOPHIA.PATH,"./DB");
        if ( KV777.PATH[0] == 0 )
            strcpy(KV777.PATH,"./DB");
        os_compatible_path(SOPHIA.PATH), ensure_directory(SOPHIA.PATH);
        os_compatible_path(KV777.PATH), ensure_directory(KV777.PATH);
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
        if ( SUPERNET.gatewayid >= 0 )
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
        SUPERNET.PM = kv777_init("PM",0);
        SUPERNET.alias = kv777_init("alias",0);
        SUPERNET.channels = kv777_init("channels",0);
        SUPERNET.NXTaccts = kv777_init("NXTaccts",0);
        SUPERNET.rawPM = kv777_init("rawPM",0);
        SUPERNET.services = kv777_init("services",0);
        SUPERNET.readyflag = 1;
        if ( SUPERNET.UPNP != 0 )
        {
            char portstr[64];
            sprintf(portstr,"%d",SUPERNET.serviceport), upnpredirect(portstr,portstr,"TCP","SuperNET");
            sprintf(portstr,"%d",SUPERNET.port + LB_OFFSET), upnpredirect(portstr,portstr,"TCP","SuperNET");
            sprintf(portstr,"%d",SUPERNET.port + PUBGLOBALS_OFFSET), upnpredirect(portstr,portstr,"TCP","SuperNET");
            sprintf(portstr,"%d",SUPERNET.port + PUBRELAYS_OFFSET), upnpredirect(portstr,portstr,"TCP","SuperNET");
        }
        
        //void kv777_test(); kv777_test();
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        methodstr = cJSON_str(cJSON_GetObjectItem(json,"method"));
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        if ( (destplugin= cJSON_str(cJSON_GetObjectItem(json,"name"))) == 0 )
            destplugin = cJSON_str(cJSON_GetObjectItem(json,"path"));
        printf("SUPERNET\n");
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            retstr = "return registered";
        }
        else if ( methodstr != 0 && strcmp(methodstr,"install") == 0 && destplugin != 0 && destplugin[0] != 0 )
            retstr = SuperNET_install(destplugin,jsonstr,json);
        else retstr = "return JSON result";
        strcpy(retbuf,retstr);
        len = (int32_t)strlen(retbuf);
        while ( --len > 0 )
            if ( retbuf[len] == '}' )
                break;
        sprintf(retbuf + len,",\"debug\":%d,\"USESSL\":%d,\"MAINNET\":%d,\"DATADIR\":\"%s\",\"NXTAPI\":\"%s\",\"WEBSOCKETD\":\"%s\",\"SUPERNET_PORT\":%d,\"APISLEEP\":%d,\"domain\":\"%s\"}",Debuglevel,SUPERNET.usessl,SUPERNET.ismainnet,SUPERNET.DATADIR,SUPERNET.NXTAPIURL,SUPERNET.WEBSOCKETD,SUPERNET.port,SUPERNET.APISLEEP,SUPERNET.hostname);
    }
    return((int32_t)strlen(retbuf));
}

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *Globals,cJSON *json)
{
    uint64_t disableflags = 0;
   //ensure_directory(SUPERNET.SOPHIA_DIR);
    //Globals->Gatewayid = -1, Globals->Numgateways = 3;
    //expand_nxt64bits(Globals->NXT_ASSETIDSTR,NXT_ASSETID);
    //init_InstantDEX(calc_nxt64bits(Global_mp->myNXTADDR),1);
    //SaM_PrepareIndices();
    printf("SuperNET init %s size.%ld\n",plugin->name,sizeof(struct SuperNET_info));
    return(disableflags); // set bits corresponding to array position in _methods[]
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    return(retcode);
}
#include "plugins/plugin777.c"




#endif

