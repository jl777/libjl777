
//
//  api.h
//
//  Created by jl777 on 7/6/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef API_H
#define API_H

#include "libwebsockets.h"

char *block_on_SuperNET(int32_t blockflag,char *JSONstr);

#ifndef INSTALL_DATADIR
#define INSTALL_DATADIR "~"
#endif
#define LOCAL_RESOURCE_PATH INSTALL_DATADIR
char *resource_path = LOCAL_RESOURCE_PATH;

static volatile int force_exit = 0;
static struct libwebsocket_context *context;

struct serveable
{
	const char *urlpath;
	const char *mimetype;
};

struct per_session_data__http
{
	int fd;
};

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

void return_http_str(struct libwebsocket *wsi,char *retstr)
{
    int32_t len;
    unsigned char buffer[1024];
    len = (int32_t)strlen(retstr);
    sprintf((char *)buffer,
            "HTTP/1.0 200 OK\x0d\x0a"
            "Server: NXTprotocol.jl777\x0d\x0a"
            "Content-Type: text/html\x0d\x0a"
            "Access-Control-Allow-Origin: *\x0d\x0a"
            "Content-Length: %u\x0d\x0a\x0d\x0a",
            (unsigned int)len);
    //printf("html hdr.(%s)\n",buffer);
    libwebsocket_write(wsi,buffer,strlen((char *)buffer),LWS_WRITE_HTTP);
    libwebsocket_write(wsi,(unsigned char *)retstr,len,LWS_WRITE_HTTP);
}

// this protocol server (always the first one) just knows how to do HTTP
static int callback_http(struct libwebsocket_context *context,struct libwebsocket *wsi,enum libwebsocket_callback_reasons reason,void *user,void *in,size_t len)
{
	char buf[MAX_JSON_FIELD],*retstr;
    cJSON *json,*array;
    //if ( len != 0 )
    //printf("reason.%d len.%ld\n",reason,len);
	switch ( reason )
    {
        case LWS_CALLBACK_HTTP:
            if ( len < 1 )
            {
                //libwebsockets_return_http_status(context, wsi,HTTP_STATUS_BAD_REQUEST, NULL);
                return -1;
            }
            if ( strchr((const char *)in + 1, '/') != 0 ) // this server has no concept of directories
            {
                //libwebsockets_return_http_status(context, wsi,HTTP_STATUS_FORBIDDEN, NULL);
                return -1;
            }
            // if a legal POST URL, let it continue and accept data
            if ( lws_hdr_total_length(wsi,WSI_TOKEN_POST_URI) != 0 )
                return 0;
            //printf("GOT.(%s)\n",(char *)in);
            convert_percent22((char *)in);
            retstr = block_on_SuperNET(1,(char *)in+1);
            if ( retstr != 0 )
            {
                return_http_str(wsi,retstr);
                /*len = strlen(retstr);
                sprintf((char *)buffer,
                        "HTTP/1.0 200 OK\x0d\x0a"
                        "Server: NXTprotocol.jl777\x0d\x0a"
                        "Content-Type: text/html\x0d\x0a"
                        "Access-Control-Allow-Origin: *\x0d\x0a"
                        "Content-Length: %u\x0d\x0a\x0d\x0a",
                        (unsigned int)len);
                printf("html hdr.(%s)\n",buffer);
                libwebsocket_write(wsi,buffer,strlen((char *)buffer),LWS_WRITE_HTTP);
                libwebsocket_write(wsi,(unsigned char *)retstr,len,LWS_WRITE_HTTP);*/
                free(retstr);
            }
            return(-1);
            break;
        case LWS_CALLBACK_HTTP_BODY:
            //printf("RPC.(%s)\n",(char *)in);
            //{"jsonrpc": "1.0", "id":"curltest", "method": "SuperNET", "params": ["{\"requestType\":\"getpeers\"}"]  }
            if ( (json= cJSON_Parse((char *)in)) != 0 )
            {
                if ( (array= cJSON_GetObjectItem(json,"params")) != 0 && is_cJSON_Array(array) != 0 )
                {
                    copy_cJSON(buf,cJSON_GetArrayItem(array,0));
                    replace_backslashquotes(buf);
                    stripwhite_ns(buf,strlen(buf));
                    retstr = block_on_SuperNET(1,buf);
                    if ( retstr != 0 )
                    {
                        //stripwhite_ns(retstr,strlen(retstr));
                        //strcat(retstr,"\n");
                        //printf("RPC return.(%s)\n",retstr);
                        return_http_str(wsi,retstr);
                        free(retstr);
                        free(json);
                        return(-1);
                    } else printf("(%s) returned null\n",buf);
                }
                else
                {
                    strncpy(buf,in,sizeof(buf)-1);
                    buf[sizeof(buf)-1] = '\0';
                    //if ( len < 20 )
                    //    buf[len] = '\0';
                    lwsl_notice("LWS_CALLBACK_HTTP_BODY: %s\n",buf);
                }
                free_json(json);
            }
            else printf("couldnt parse (%s)\n",(char *)in);
            return_http_str(wsi,"{\"error\":\"couldnt parse JSON\"}");
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
            /*do
            {
                n = (int)read(pss->fd,buffer,sizeof buffer);
                if ( n < 0 ) // problem reading, close conn
                    goto bail;
                if ( n == 0 ) // sent it all, close conn
                    goto flush_bail;
                // because it's HTTP and not websocket, don't need to take care about pre and postamble
                m = libwebsocket_write(wsi,buffer,n,LWS_WRITE_HTTP);
                if ( m < 0 ) // write failed, close conn
                    goto bail;
                if ( m != n ) // partial write, adjust
                    lseek(pss->fd,m - n,SEEK_CUR);
            } while ( lws_send_pipe_choked(wsi) == 0 );
            libwebsocket_callback_on_writable(context,wsi);
            break;
        flush_bail:
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
#if 0
            libwebsockets_get_peer_addresses(context, wsi, (int)(long)in, client_name,sizeof(client_name), client_ip, sizeof(client_ip));
            fprintf(stderr, "Received network connect from %s (%s)\n",client_name, client_ip);
#endif
            // if we returned non-zero from here, we kill the connection
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
	struct lws_context_creation_info info;
	memset(&info, 0, sizeof info);
	info.port = port;
    /*#if !defined(LWS_NO_DAEMONIZE) && !defined(WIN32)
     if ( lws_daemonize("/tmp/.SuperNET-lock") != 0 )
     {
     fprintf(stderr,"Failed to daemonize\n");
     return(-1);
     }
     #endif*/
	signal(SIGINT, sighandler);
	lwsl_notice("libwebsockets test server - (C) Copyright 2010-2013 Andy Green <andy@warmcat.com> -  licensed under LGPL2.1\n");
	info.iface = iface;
	info.protocols = protocols;
 	info.gid = -1;
	info.uid = -1;
	info.options = opts;
	if ( use_ssl == 0 )
    {
		info.ssl_cert_filepath = NULL;
		info.ssl_private_key_filepath = NULL;
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
		info.ssl_cert_filepath = cert_path;
		info.ssl_private_key_filepath = key_path;
	}
	context = libwebsocket_create_context(&info);
	if ( context == NULL )
    {
		lwsl_err("libwebsocket init failed\n");
		return -1;
	}
	n = 0;
	while ( n >= 0 && !force_exit )
    {
		n = libwebsocket_service(context,millis);
	}
	libwebsocket_context_destroy(context);
	lwsl_notice("libwebsockets-test-server exited cleanly\n");
	return 0;
}

char *orderbook_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    int32_t i,polarity,allflag;
    uint64_t obookid;
    cJSON *json,*bids,*asks,*item;
    struct orderbook *op;
    char obook[64],buf[MAX_JSON_FIELD],assetA[64],assetB[64],*retstr = 0;
    obookid = get_API_nxt64bits(objs[0]);
    expand_nxt64bits(obook,obookid);
    polarity = get_API_int(objs[1],1);
    if ( polarity == 0 )
        polarity = 1;
    allflag = get_API_int(objs[2],0);
    if ( obookid != 0 && (op= create_orderbook(obookid,polarity,0,0)) != 0 )
    {
        if ( op->numbids == 0 && op->numasks == 0 )
            retstr = clonestr("{\"error\":\"no bids or asks\"}");
        else
        {
            json = cJSON_CreateObject();
            bids = cJSON_CreateArray();
            for (i=0; i<op->numbids; i++)
            {
                item = create_order_json(&op->bids[i],1,allflag);
                cJSON_AddItemToArray(bids,item);
            }
            asks = cJSON_CreateArray();
            for (i=0; i<op->numasks; i++)
            {
                item = create_order_json(&op->asks[i],1,allflag);
                cJSON_AddItemToArray(asks,item);
            }
            expand_nxt64bits(assetA,op->assetA);
            expand_nxt64bits(assetB,op->assetB);
            cJSON_AddItemToObject(json,"orderbook",cJSON_CreateString(obook));
            cJSON_AddItemToObject(json,"assetA",cJSON_CreateString(assetA));
            cJSON_AddItemToObject(json,"assetB",cJSON_CreateString(assetB));
            cJSON_AddItemToObject(json,"polarity",cJSON_CreateNumber(polarity));
            cJSON_AddItemToObject(json,"bids",bids);
            cJSON_AddItemToObject(json,"asks",asks);
            retstr = cJSON_Print(json);
        }
        free_orderbook(op);
    } else
    {
        sprintf(buf,"{\"error\":\"no such orderbook.(%llu)\"}",(long long)obookid);
        retstr = clonestr(buf);
    }
    return(retstr);
}

char *getorderbooks_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    cJSON *json;
    char *retstr = 0;
    json = create_orderbooks_json();
    if ( json != 0 )
    {
        retstr = cJSON_Print(json);
        free_json(json);
    }
    else retstr = clonestr("{\"error\":\"no orderbooks\"}");
    return(retstr);
}

char *placequote_func(struct sockaddr *prevaddr,int32_t dir,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    int32_t polarity;
    uint64_t obookid,nxt64bits,assetA,assetB,txid = 0;
    double price,volume;
    struct orderbook_tx tx;
    char buf[MAX_JSON_FIELD],txidstr[64],*retstr = 0;
    if ( prevaddr != 0 )
        return(0);
    nxt64bits = calc_nxt64bits(sender);
    obookid = get_API_nxt64bits(objs[0]);
    polarity = get_API_int(objs[1],1);
    if ( polarity == 0 )
        polarity = 1;
    volume = get_API_float(objs[2]);
    price = get_API_float(objs[3]);
    assetA = get_API_nxt64bits(objs[4]);
    assetB = get_API_nxt64bits(objs[5]);
    printf("PLACE QUOTE polarity.%d dir.%d\n",polarity,dir);
    if ( sender[0] != 0 && find_raw_orders(obookid) != 0 && valid > 0 )
    {
        if ( price != 0. && volume != 0. && dir != 0 )
        {
            if ( dir*polarity > 0 )
                bid_orderbook_tx(&tx,0,nxt64bits,obookid,price,volume);
            else ask_orderbook_tx(&tx,0,nxt64bits,obookid,price,volume);
            printf("need to finish porting this\n");
            //len = construct_tokenized_req(tx,cmd,NXTACCTSECRET);
            //txid = call_SuperNET_broadcast((uint8_t *)packet,len,PUBADDRS_MSGDURATION);
            if ( txid != 0 )
            {
                expand_nxt64bits(txidstr,txid);
                sprintf(buf,"{\"txid\":\"%s\"}",txidstr);
                retstr = clonestr(buf);
            }
        }
        if ( retstr == 0 )
        {
            sprintf(buf,"{\"error submitting\":\"place%s error obookid.%llx polarity.%d volume %f price %f\"}",dir>0?"bid":"ask",(long long)obookid,polarity,volume,price);
            retstr = clonestr(buf);
        }
    }
    else if ( assetA != 0 && assetB != 0 && assetA != assetB )
    {
        if ( (obookid= create_raw_orders(assetA,assetB)) != 0 )
        {
            sprintf(buf,"{\"obookid\":\"%llu\"}",(long long)obookid);
            retstr = clonestr(buf);
        }
        else
        {
            sprintf(buf,"{\"error\":\"couldnt create orderbook for assets %llu and %llu\"}",(long long)assetA,(long long)assetB);
            retstr = clonestr(buf);
        }
    }
    else
    {
        sprintf(buf,"{\"error\":\"place%s error obookid.%llx polarity.%d volume %f price %f\"}",polarity>0?"bid":"ask",(long long)obookid,polarity,volume,price);
        retstr = clonestr(buf);
    }
    return(retstr);
}

char *placebid_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    return(placequote_func(prevaddr,1,sender,valid,objs,numobjs,origargstr));
}

char *placeask_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    return(placequote_func(prevaddr,-1,sender,valid,objs,numobjs,origargstr));
}

char *sendmsg_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    static int counter;
    char previp[64],nexthopNXTaddr[64],destNXTaddr[64],msg[MAX_JSON_FIELD],*retstr = 0;
    int32_t L,port,len;
    copy_cJSON(destNXTaddr,objs[0]);
    copy_cJSON(msg,objs[1]);
    if ( prevaddr != 0 )
    {
        printf("GOT MESSAGE.(%s) from %s\n",msg,sender);
        return(0);
    }
    L = (int32_t)get_API_int(objs[2],Global_mp->Lfactor);
    nexthopNXTaddr[0] = 0;
    //printf("sendmsg_func sender.(%s) valid.%d dest.(%s) (%s)\n",sender,valid,destNXTaddr,origargstr);
    if ( sender[0] != 0 && valid > 0 && destNXTaddr[0] != 0 )
    {
        if ( prevaddr != 0 )
        {
            port = extract_nameport(previp,sizeof(previp),(struct sockaddr_in *)prevaddr);
            fprintf(stderr,"%d >>>>>>>>>>>>> received message.(%s) NXT.%s from hop.%s/%d\n",counter,msg,sender,previp,port);
            counter++;
            //retstr = clonestr("{\"result\":\"received message\"}");
        }
        else
        {
            len = (int32_t)strlen(origargstr)+1;
            stripwhite_ns(origargstr,len);
            len = (int32_t)strlen(origargstr)+1;
            retstr = sendmessage(nexthopNXTaddr,L,sender,origargstr,len,destNXTaddr,0,0);
        }
    }
    //if ( retstr == 0 )
    //    retstr = clonestr("{\"error\":\"invalid sendmessage request\"}");
    return(retstr);
}

char *sendbinary_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    static int counter;
    char previp[64],nexthopNXTaddr[64],destNXTaddr[64],cmdstr[MAX_JSON_FIELD],datastr[MAX_JSON_FIELD],*retstr = 0;
    int32_t L,port;
    if ( prevaddr != 0 )
        return(0);
    copy_cJSON(destNXTaddr,objs[0]);
    copy_cJSON(datastr,objs[1]);
    L = (int32_t)get_API_int(objs[2],Global_mp->Lfactor);
    nexthopNXTaddr[0] = 0;
    //printf("sendmsg_func sender.(%s) valid.%d dest.(%s) (%s)\n",sender,valid,destNXTaddr,origargstr);
    if ( sender[0] != 0 && valid > 0 && destNXTaddr[0] != 0 )
    {
        if ( prevaddr != 0 )
        {
            port = extract_nameport(previp,sizeof(previp),(struct sockaddr_in *)prevaddr);
            fprintf(stderr,"%d >>>>>>>>>>>>> received binary message.(%s) NXT.%s from hop.%s/%d\n",counter,datastr,sender,previp,port);
            counter++;
            //retstr = clonestr("{\"result\":\"received message\"}");
        }
        else
        {
            sprintf(cmdstr,"{\"requestType\":\"sendbinary\",\"NXT\":\"%s\",\"data\":\"%s\",\"dest\":\"%s\"}",NXTaddr,datastr,destNXTaddr);
            retstr = send_tokenized_cmd(nexthopNXTaddr,L,NXTaddr,NXTACCTSECRET,cmdstr,destNXTaddr);
        }
    }
    //if ( retstr == 0 )
    //    retstr = clonestr("{\"error\":\"invalid sendmessage request\"}");
    return(retstr);
}

char *checkmsg_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char senderNXTaddr[MAX_JSON_FIELD],*retstr = 0;
    if ( prevaddr != 0 )
        return(0);
    copy_cJSON(senderNXTaddr,objs[0]);
    if ( sender[0] != 0 && valid > 0 )
        retstr = checkmessages(sender,NXTACCTSECRET,senderNXTaddr);
    else retstr = clonestr("{\"result\":\"invalid checkmessages request\"}");
    return(retstr);
}

char *makeoffer_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    uint64_t assetA,assetB;
    double qtyA,qtyB;
    int32_t type;
    char otherNXTaddr[MAX_JSON_FIELD],*retstr = 0;
    if ( prevaddr != 0 )
        return(0);
    copy_cJSON(otherNXTaddr,objs[0]);
    assetA = get_API_nxt64bits(objs[1]);
    qtyA = get_API_float(objs[2]);
    assetB = get_API_nxt64bits(objs[3]);
    qtyB = get_API_float(objs[4]);
    type = get_API_int(objs[5],0);
    
    if ( sender[0] != 0 && valid > 0 && otherNXTaddr[0] != 0 )//&& assetA != 0 && qtyA != 0. && assetB != 0. && qtyB != 0. )
        retstr = makeoffer(sender,NXTACCTSECRET,otherNXTaddr,assetA,qtyA,assetB,qtyB,type);
    else retstr = clonestr("{\"result\":\"invalid makeoffer_func request\"}");
    return(retstr);
}

char *processutx_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char utx[MAX_JSON_FIELD],full[MAX_JSON_FIELD],sig[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(utx,objs[0]);
    copy_cJSON(sig,objs[1]);
    copy_cJSON(full,objs[2]);
    if ( sender[0] != 0 && valid > 0 )
        retstr = processutx(sender,utx,sig,full);
    else retstr = clonestr("{\"result\":\"invalid makeoffer_func request\"}");
    return(retstr);
}

char *respondtx_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char signedtx[MAX_JSON_FIELD],*retstr = 0;
    if ( prevaddr != 0 )
        return(0);
    copy_cJSON(signedtx,objs[0]);
    if ( sender[0] != 0 && valid > 0 && signedtx[0] != 0 )
        retstr = respondtx(sender,signedtx);
    else retstr = clonestr("{\"result\":\"invalid makeoffer_func request\"}");
    return(retstr);
}

char *tradebot_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    static char *buf;
    static int64_t filelen,allocsize;
    long len;
    cJSON *botjson;
    char code[MAX_JSON_FIELD],retbuf[4096],*str,*retstr = 0;
    if ( prevaddr != 0 )
        return(0);
    copy_cJSON(code,objs[0]);
    printf("tradebotfunc.(%s) sender.(%s) valid.%d code.(%s)\n",origargstr,sender,valid,code);
    if ( sender[0] != 0 && valid > 0 && code[0] != 0 )
    {
        len = strlen(code);
        if ( code[0] == '(' && code[len-1] == ')' )
        {
            code[len-1] = 0;
            str = load_file(code+1,&buf,&filelen,&allocsize);
            if ( str == 0 )
            {
                sprintf(retbuf,"{\"error\":\"cant open (%s)\"}",code+1);
                return(clonestr(retbuf));
            }
        }
        else
        {
            str = code;
            //printf("str is (%s)\n",str);
        }
        if ( str != 0 )
        {
            //printf("before.(%s) ",str);
            replace_singlequotes(str);
            //printf("after.(%s)\n",str);
            if ( (botjson= cJSON_Parse(str)) != 0 )
            {
                retstr = start_tradebot(sender,NXTACCTSECRET,botjson);
                free_json(botjson);
            }
            else
            {
                str[sizeof(retbuf)-128] = 0;
                sprintf(retbuf,"{\"error\":\"couldnt parse (%s)\"}",str);
                printf("%s\n",retbuf);
            }
            if ( str != code )
                free(str);
        }
    }
    else retstr = clonestr("{\"result\":\"invalid tradebot request\"}");
    return(retstr);
}

char *teleport_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    double amount;
    char contactstr[MAX_JSON_FIELD],minage[MAX_JSON_FIELD],coinstr[MAX_JSON_FIELD],withdrawaddr[MAX_JSON_FIELD],*retstr = 0;
    if ( prevaddr != 0 )
        return(0);
    if ( Historical_done == 0 )
        return(clonestr("{\"error\":\"historical processing is not done yet\"}"));
    amount = get_API_float(objs[0]);
    copy_cJSON(contactstr,objs[1]);
    copy_cJSON(coinstr,objs[2]);
    copy_cJSON(minage,objs[3]);
    copy_cJSON(withdrawaddr,objs[4]);
    printf("amount.(%.8f) minage.(%s) %d\n",amount,minage,atoi(minage));
    if ( sender[0] != 0 && amount > 0 && valid > 0 && contactstr[0] != 0 )
        retstr = teleport(contactstr,coinstr,(uint64_t)(SATOSHIDEN * amount),atoi(minage),withdrawaddr);
    else retstr = clonestr("{\"error\":\"invalid teleport request\"}");
    return(retstr);
}

char *telepodacct_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    double amount;
    char contactstr[MAX_JSON_FIELD],coinstr[MAX_JSON_FIELD],withdrawaddr[MAX_JSON_FIELD],comment[MAX_JSON_FIELD],cmd[MAX_JSON_FIELD],*retstr = 0;
    if ( prevaddr != 0 )
        return(0);
    if ( Historical_done == 0 )
        return(clonestr("{\"error\":\"historical processing is not done yet\"}"));
    amount = get_API_float(objs[0]);
    copy_cJSON(contactstr,objs[1]);
    copy_cJSON(coinstr,objs[2]);
    copy_cJSON(comment,objs[3]);
    copy_cJSON(cmd,objs[4]);
    copy_cJSON(withdrawaddr,objs[5]);
    if ( sender[0] != 0 && valid > 0 && contactstr[0] != 0 )
        retstr = telepodacct(contactstr,coinstr,(uint64_t)(SATOSHIDEN * amount),withdrawaddr,comment,cmd);
    else retstr = clonestr("{\"error\":\"invalid teleport request\"}");
    return(retstr);
}

char *maketelepods_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    uint64_t value;
    char coinstr[MAX_JSON_FIELD],*retstr = 0;
    if ( prevaddr != 0 )
        return(0);
    value = (SATOSHIDEN * get_API_float(objs[0]));
    copy_cJSON(coinstr,objs[1]);
    printf("maketelepods.%s %.8f\n",coinstr,dstr(value));
    if ( coinstr[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = maketelepods(NXTACCTSECRET,sender,coinstr,value);
    else retstr = clonestr("{\"error\":\"invalid maketelepods_func arguments\"}");
    return(retstr);
}

char *getpeers_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    cJSON *json;
    int32_t scanflag;
    char *jsonstr = 0;
    if ( prevaddr != 0 )
        return(0);
    scanflag = get_API_int(objs[0],0);
    json = gen_peers_json(prevaddr,NXTaddr,NXTACCTSECRET,sender,scanflag);
    if ( json != 0 )
    {
        jsonstr = cJSON_Print(json);
        free_json(json);
    }
    return(jsonstr);
}

char *savefile_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    FILE *fp;
    int32_t L,M,N;
    char pin[MAX_JSON_FIELD],fname[MAX_JSON_FIELD],usbname[MAX_JSON_FIELD],password[MAX_JSON_FIELD],*retstr = 0;
    if ( prevaddr != 0 )
        return(clonestr("{\"error\":\"savefile is only for local access\"}"));
    copy_cJSON(fname,objs[0]);
    L = get_API_int(objs[1],0);
    M = get_API_int(objs[2],1);
    N = get_API_int(objs[3],1);
    if ( N < 0 )
        N = 0;
    else if ( N > 254 )
        N = 254;
    if ( M >= N )
        M = N;
    else if ( M < 1 )
        M = 1;
    copy_cJSON(usbname,objs[4]);
    copy_cJSON(password,objs[5]);
    copy_cJSON(pin,objs[5]);
    fp = fopen(fname,"rb");
    if ( fp == 0 )
        printf("cant find file (%s)\n",fname);
    if ( fp != 0 && sender[0] != 0 && valid > 0 )
        retstr = mofn_savefile(prevaddr,NXTaddr,NXTACCTSECRET,sender,pin,fp,L,M,N,usbname,password,fname);
    else retstr = clonestr("{\"error\":\"invalid savefile_func arguments\"}");
    if ( fp != 0 )
        fclose(fp);
    return(retstr);
}

char *restorefile_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    FILE *fp;
    char txidstr[MAX_JSON_FIELD];
    cJSON *array,*item;
    uint64_t *txids = 0;
    int32_t L,M,N,i,n;
    char pin[MAX_JSON_FIELD],fname[MAX_JSON_FIELD],sharenrs[MAX_JSON_FIELD],destfname[MAX_JSON_FIELD],usbname[MAX_JSON_FIELD],password[MAX_JSON_FIELD],*retstr = 0;
    if ( prevaddr != 0 )
        return(clonestr("{\"error\":\"restorefile is only for local access\"}"));
    copy_cJSON(fname,objs[0]);
    L = get_API_int(objs[1],0);
    M = get_API_int(objs[2],1);
    N = get_API_int(objs[3],1);
    if ( N < 0 )
        N = 0;
    else if ( N > 254 )
        N = 254;
    if ( M >= N )
        M = N;
    else if ( M < 1 )
        M = 1;
    copy_cJSON(usbname,objs[4]);
    copy_cJSON(password,objs[5]);
    copy_cJSON(destfname,objs[6]);
    copy_cJSON(sharenrs,objs[7]);
    array = objs[8];
    if ( is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
    {
        txids = calloc(n+1,sizeof(*txids));
        for (i=0; i<n; i++)
        {
            item = cJSON_GetArrayItem(array,i);
            copy_cJSON(txidstr,item);
            if ( txidstr[0] != 0 )
                txids[i] = calc_nxt64bits(txidstr);
        }
    }
    copy_cJSON(pin,objs[9]);
    if ( destfname[0] == 0 )
        strcpy(destfname,fname), strcat(destfname,".restore");
    if ( 0 )
    {
        fp = fopen(destfname,"rb");
        if ( fp != 0 )
        {
            fclose(fp);
            return(clonestr("{\"error\":\"destfilename is already exists\"}"));
        }
    }
    fp = fopen(destfname,"wb");
    if ( fp != 0 && sender[0] != 0 && valid > 0 && destfname[0] != 0  )
        retstr = mofn_restorefile(prevaddr,NXTaddr,NXTACCTSECRET,sender,pin,fp,L,M,N,usbname,password,fname,sharenrs,txids);
    else retstr = clonestr("{\"error\":\"invalid savefile_func arguments\"}");
    if ( fp != 0 )
        fclose(fp);
    if ( txids != 0 )
        free(txids);
    {
        char cmdstr[512];
        printf("\n*****************\ncompare (%s) vs (%s)\n",fname,destfname);
        sprintf(cmdstr,"cmp %s %s",fname,destfname);
        system(cmdstr);
        printf("done\n\n");
    }
    return(retstr);
}

char *findaddress_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char txidstr[MAX_JSON_FIELD],*retstr = 0;
    cJSON *array,*item;
    struct coin_info *cp = get_coin_info("BTCD");
    int32_t targetdist,numthreads,duration,i,n = 0;
    uint64_t refaddr,*txids = 0;
    if ( prevaddr != 0 )
        return(clonestr("{\"error\":\"can only findaddress locally\"}"));
    refaddr = get_API_nxt64bits(objs[0]);
    array = objs[1];
    targetdist = (int32_t)get_API_int(objs[2],10);
    duration = (int32_t)get_API_int(objs[3],60);
    numthreads = (int32_t)get_API_int(objs[4],8);
    if ( is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
    {
        txids = calloc(n+1,sizeof(*txids));
        for (i=0; i<n; i++)
        {
            item = cJSON_GetArrayItem(array,i);
            copy_cJSON(txidstr,item);
            if ( txidstr[0] != 0 )
                txids[i] = calc_nxt64bits(txidstr);
        }
    }
    else if ( (n= cp->numnxtaccts) > 0 )
    {
        txids = calloc(n+1,sizeof(*txids));
        memcpy(txids,cp->nxtaccts,n*sizeof(*txids));
    }
    else
    {
        n = 512;
        txids = calloc(n+1,sizeof(*txids));
        for (i=0; i<n; i++)
            randombytes((unsigned char *)&txids[i],sizeof(txids[i]));
    }
    if ( txids != 0 && sender[0] != 0 && valid > 0 )
        retstr = findaddress(prevaddr,NXTaddr,NXTACCTSECRET,sender,refaddr,txids,n,targetdist,duration,numthreads);
    else retstr = clonestr("{\"error\":\"invalid findaddress_func arguments\"}");
    //if ( txids != 0 ) freed on completion
    //    free(txids);
    return(retstr);
}

char *sendfile_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    FILE *fp;
    int32_t L;
    char fname[MAX_JSON_FIELD],dest[MAX_JSON_FIELD],*retstr = 0;
    if ( prevaddr != 0 )
        return(0);
    copy_cJSON(fname,objs[0]);
    copy_cJSON(dest,objs[1]);
    L = get_API_int(objs[2],0);
    fp = fopen(fname,"rb");
    if ( fp != 0 && sender[0] != 0 && valid > 0 )
        retstr = onion_sendfile(L,prevaddr,NXTaddr,NXTACCTSECRET,sender,dest,fp);
    else retstr = clonestr("{\"error\":\"invalid sendfile_func arguments\"}");
    if ( fp != 0 )
        fclose(fp);
    return(retstr);
}

char *ping_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    int32_t port;
    char pubkey[MAX_JSON_FIELD],destip[MAX_JSON_FIELD],ipaddr[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(pubkey,objs[0]);
    copy_cJSON(ipaddr,objs[1]);
    port = get_API_int(objs[2],0);
    copy_cJSON(destip,objs[3]);
    //printf("ping got sender.(%s) valid.%d pubkey.(%s) ipaddr.(%s) port.%d destip.(%s)\n",sender,valid,pubkey,ipaddr,port,destip);
    if ( sender[0] != 0 && valid > 0 )
        retstr = kademlia_ping(prevaddr,NXTaddr,NXTACCTSECRET,sender,ipaddr,port,destip,origargstr);
    else retstr = clonestr("{\"error\":\"invalid ping_func arguments\"}");
    return(retstr);
}

char *pong_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char pubkey[MAX_JSON_FIELD],ipaddr[MAX_JSON_FIELD],*retstr = 0;
    uint16_t port;
    if ( prevaddr == 0 )
        return(0);
    copy_cJSON(pubkey,objs[0]);
    copy_cJSON(ipaddr,objs[1]);
    port = get_API_int(objs[2],0);
    //printf("pong got pubkey.(%s) ipaddr.(%s) port.%d \n",pubkey,ipaddr,port);
    if ( sender[0] != 0 && valid > 0 )
        retstr = kademlia_pong(prevaddr,NXTaddr,NXTACCTSECRET,sender,ipaddr,port);
    else retstr = clonestr("{\"error\":\"invalid pong_func arguments\"}");
    return(retstr);
}

char *addcontact_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char handle[MAX_JSON_FIELD],acct[MAX_JSON_FIELD],*retstr = 0;
    if ( prevaddr != 0 )
        return(0);
    copy_cJSON(handle,objs[0]);
    copy_cJSON(acct,objs[1]);
    printf("handle.(%s) acct.(%s) valid.%d\n",handle,acct,valid);
    if ( handle[0] != 0 && acct[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = addcontact(handle,acct);
    else retstr = clonestr("{\"error\":\"invalid addcontact_func arguments\"}");
    return(retstr);
}

char *removecontact_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char handle[MAX_JSON_FIELD],*retstr = 0;
    if ( prevaddr != 0 )
        return(0);
    copy_cJSON(handle,objs[0]);
    if ( handle[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = removecontact(prevaddr,NXTaddr,NXTACCTSECRET,sender,handle);
    else retstr = clonestr("{\"error\":\"invalid removecontact_func arguments\"}");
    return(retstr);
}

char *dispcontact_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char handle[MAX_JSON_FIELD],*retstr = 0;
    if ( prevaddr != 0 )
        return(0);
    copy_cJSON(handle,objs[0]);
    if ( handle[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = dispcontact(prevaddr,NXTaddr,NXTACCTSECRET,sender,handle);
    else retstr = clonestr("{\"error\":\"invalid dispcontact arguments\"}");
    return(retstr);
}

void set_kademlia_args(char *key,cJSON *keyobj,cJSON *nameobj)
{
    uint64_t hash;
    long len;
    char name[MAX_JSON_FIELD];
    key[0] = 0;
    copy_cJSON(name,nameobj);
    if ( name[0] != 0 )
    {
        len = strlen(name);
        if ( len < 64 )
        {
            hash = calc_txid((unsigned char *)name,(int32_t)strlen(name));
            expand_nxt64bits(key,hash);
        }
    }
    else copy_cJSON(key,keyobj);
}

char *findnode_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char pubkey[MAX_JSON_FIELD],key[MAX_JSON_FIELD],value[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(pubkey,objs[0]);
    set_kademlia_args(key,objs[1],objs[2]);
    copy_cJSON(value,objs[3]);
    if ( Debuglevel > 1 )
        printf("findnode.%p (%s) (%s) (%s) (%s)\n",prevaddr,sender,pubkey,key,value);
    if ( key[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = kademlia_find("findnode",prevaddr,NXTaddr,NXTACCTSECRET,sender,key,value,origargstr);
    else retstr = clonestr("{\"error\":\"invalid findnode_func arguments\"}");
    return(retstr);
}

char *findvalue_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char pubkey[MAX_JSON_FIELD],key[MAX_JSON_FIELD],value[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(pubkey,objs[0]);
    set_kademlia_args(key,objs[1],objs[2]);
    copy_cJSON(value,objs[3]);
    if ( Debuglevel > 1 )
        printf("findvalue.%p (%s) (%s) (%s)\n",prevaddr,sender,pubkey,key);
    if ( key[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = kademlia_find("findvalue",prevaddr,NXTaddr,NXTACCTSECRET,sender,key,value,origargstr);
    else retstr = clonestr("{\"error\":\"invalid findvalue_func arguments\"}");
    if ( Debuglevel > 1 )
        printf("back from findvalue\n");
    return(retstr);
}

char *havenode_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char pubkey[MAX_JSON_FIELD],key[MAX_JSON_FIELD],value[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(pubkey,objs[0]);
    set_kademlia_args(key,objs[1],objs[2]);
    copy_cJSON(value,objs[3]);
    if ( Debuglevel > 1 )
        printf("got HAVENODE.(%s) for key.(%s) from %s\n",value,key,sender);
    if ( key[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = kademlia_havenode(0,prevaddr,NXTaddr,NXTACCTSECRET,sender,key,value);
    else retstr = clonestr("{\"error\":\"invalid havenode_func arguments\"}");
    return(retstr);
}

char *havenodeB_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char pubkey[MAX_JSON_FIELD],key[MAX_JSON_FIELD],value[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(pubkey,objs[0]);
    set_kademlia_args(key,objs[1],objs[2]);
    copy_cJSON(value,objs[3]);
    if ( Debuglevel > 1 )
        printf("got HAVENODEB.(%s) for key.(%s) from %s\n",value,key,sender);
    if ( key[0] != 0 && sender[0] != 0 && valid > 0 )
        retstr = kademlia_havenode(1,prevaddr,NXTaddr,NXTACCTSECRET,sender,key,value);
    else retstr = clonestr("{\"error\":\"invalid havenodeB_func arguments\"}");
    return(retstr);
}

char *store_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char pubkey[MAX_JSON_FIELD],key[MAX_JSON_FIELD],datastr[MAX_JSON_FIELD],*retstr = 0;
    copy_cJSON(pubkey,objs[0]);
    set_kademlia_args(key,objs[1],objs[2]);
    copy_cJSON(datastr,objs[3]);
    if ( key[0] != 0 && sender[0] != 0 && valid > 0 && datastr[0] != 0 )
    {
        retstr = kademlia_storedata(prevaddr,NXTaddr,NXTACCTSECRET,sender,key,datastr);
    }
    else retstr = clonestr("{\"error\":\"invalid store_func arguments\"}");
    return(retstr);
}

char *getdb_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char dirstr[MAX_JSON_FIELD],contact[MAX_JSON_FIELD],key[MAX_JSON_FIELD],*retstr = 0;
    int32_t sequenceid,dir;
    copy_cJSON(contact,objs[0]);
    sequenceid = get_API_int(objs[1],0);
    copy_cJSON(key,objs[2]);
    copy_cJSON(dirstr,objs[3]);
    if ( (contact[0] != 0 || key[0] != 0) && sender[0] != 0 && valid > 0 )
    {
        if ( strcmp(dirstr,"send") == 0 )
            dir = 1;
        else dir = -1;
        retstr = getdb(prevaddr,NXTaddr,NXTACCTSECRET,sender,dir,contact,sequenceid,key);
    }
    else retstr = clonestr("{\"error\":\"invalid getdb_func arguments\"}");
    return(retstr);
}

char *cosign_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    static unsigned char zerokey[32];
    char retbuf[MAX_JSON_FIELD],plaintext[MAX_JSON_FIELD],seedstr[MAX_JSON_FIELD],otheracctstr[MAX_JSON_FIELD],hexstr[65],ret0str[65];
    bits256 ret,ret0,seed,priv,pub,sha;
    struct nodestats *stats;
    // WARNING: if this is being remotely invoked, make sure you trust the requestor as the rawkey is being sent
    
    copy_cJSON(otheracctstr,objs[0]);
    copy_cJSON(seedstr,objs[1]);
    copy_cJSON(plaintext,objs[2]);
    if ( seedstr[0] != 0 )
        decode_hex(seed.bytes,sizeof(seed),seedstr);
    if ( plaintext[0] != 0 )
    {
        calc_sha256(0,sha.bytes,(unsigned char *)plaintext,(int32_t)strlen(plaintext));
        if ( seedstr[0] != 0 && memcmp(seed.bytes,sha.bytes,sizeof(seed)) != 0 )
            printf("cosign_func: error comparing seed %llx with sha256 %llx?n",(long long)seed.ulongs[0],(long long)sha.ulongs[0]);
        seed = sha;
    }
    if ( seedstr[0] == 0 )
        init_hexbytes(seedstr,seed.bytes,sizeof(seed));
    stats = get_nodestats(calc_nxt64bits(otheracctstr));
    if ( strlen(seedstr) == 64 && sender[0] != 0 && valid > 0 && stats != 0 && memcmp(stats->pubkey,zerokey,sizeof(stats->pubkey)) != 0 )
    {
        memcpy(priv.bytes,Global_mp->loopback_privkey,sizeof(priv));
        memcpy(pub.bytes,stats->pubkey,sizeof(pub));
        ret0 = curve25519(priv,pub);
        init_hexbytes(ret0str,ret0.bytes,sizeof(ret0));
        ret = xor_keys(seed,ret0);
        init_hexbytes(hexstr,ret.bytes,sizeof(ret));
        sprintf(retbuf,"{\"requestType\":\"cosigned\",\"seed\":\"%s\",\"result\":\"%s\",\"privacct\":\"%s\",\"pubacct\":\"%s\",\"ret0\":\"%s\"}",seedstr,ret0str,NXTaddr,otheracctstr,hexstr);
        return(clonestr(retbuf));
    }
    return(clonestr("{\"error\":\"invalid cosign_func arguments\"}"));
}

char *cosigned_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char retbuf[MAX_JSON_FIELD],resultstr[MAX_JSON_FIELD],seedstr[MAX_JSON_FIELD],hexstr[65];
    bits256 ret,seed,priv,val;
    uint64_t privacct,pubacct;
    // 0 ABc sha256_key(xor_keys(seed,curve25519(A,curve25519(B,c))))
    // 2 AbC sha256_key(xor_keys(seed,curve25519(A,curve25519(C,b))))
    // 4 ABc sha256_key(xor_keys(seed,curve25519(B,curve25519(A,c))))
    // 6 aBC sha256_key(xor_keys(seed,curve25519(B,curve25519(C,a))))
    // 8 AbC sha256_key(xor_keys(seed,curve25519(C,curve25519(A,b))))
    // 10 aBC sha256_key(xor_keys(seed,curve25519(C,curve25519(B,a))))
    copy_cJSON(seedstr,objs[0]);
    copy_cJSON(resultstr,objs[1]);
    privacct = get_API_nxt64bits(objs[2]);
    pubacct = get_API_nxt64bits(objs[3]);
    if ( strlen(seedstr) == 64 && sender[0] != 0 && valid > 0 )
    {
        decode_hex(seed.bytes,sizeof(seed),seedstr);
        decode_hex(val.bytes,sizeof(val),resultstr);
        memcpy(priv.bytes,Global_mp->loopback_privkey,sizeof(priv));
        ret = sha256_key(xor_keys(seed,curve25519(priv,val)));
        init_hexbytes(hexstr,ret.bytes,sizeof(ret));
        sprintf(retbuf,"{\"seed\":\"%s\",\"result\":\"%s\",\"acct\":\"%s\",\"privacct\":\"%llu\",\"pubacct\":\"%llu\",\"input\":\"%s\"}",seedstr,hexstr,NXTaddr,(long long)privacct,(long long)pubacct,resultstr);
        return(clonestr(retbuf));
    }
    return(clonestr("{\"error\":\"invalid cosigned_func arguments\"}"));
}

char *gotpacket_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char *SuperNET_gotpacket(char *msg,int32_t duration,char *ip_port);
    char msg[MAX_JSON_FIELD],ip_port[MAX_JSON_FIELD];
    int32_t duration;
    copy_cJSON(msg,objs[0]);
    duration = (int32_t)get_API_int(objs[1],600);
    copy_cJSON(ip_port,objs[2]);
    return(SuperNET_gotpacket(msg,duration,ip_port));
}

char *gotnewpeer_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    int32_t got_newpeer(char *ip_port);
    char ip_port[MAX_JSON_FIELD];
    copy_cJSON(ip_port,objs[0]);
    if ( ip_port[0] != 0 )
        queue_enqueue(&P2P_Q,clonestr(ip_port));
    return(0);
}

char *gotjson_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char *SuperNET_json_commands(struct NXThandler_info *mp,struct sockaddr *prevaddr,cJSON *origargjson,char *sender,int32_t valid,char *origargstr);
    char jsonstr[MAX_JSON_FIELD],*retstr = 0;
    cJSON *array;
    copy_cJSON(jsonstr,objs[0]);
    if ( jsonstr[0] != 0 )
    {
        //printf("got jsonstr.(%s)\n",jsonstr);
        replace_backslashquotes(jsonstr);
        array = cJSON_Parse(jsonstr);
        if ( array != 0 )
        {
            retstr = SuperNET_json_commands(Global_mp,prevaddr,array,sender,valid,origargstr);
            free_json(array);
        }
    }
    return(retstr);
}

char *SuperNET_json_commands(struct NXThandler_info *mp,struct sockaddr *prevaddr,cJSON *origargjson,char *sender,int32_t valid,char *origargstr)
{
    // glue
    static char *gotjson[] = { (char *)gotjson_func, "BTCDjson", "", "json", 0 };
    static char *gotpacket[] = { (char *)gotpacket_func, "gotpacket", "", "msg", "dur", "ip", 0 };
    static char *gotnewpeer[] = { (char *)gotnewpeer_func, "gotnewpeer", "ip_port", 0 };
  
    // multisig
    static char *cosign[] = { (char *)cosign_func, "cosign", "V", "otheracct", "seed", "text", 0 };
    static char *cosigned[] = { (char *)cosigned_func, "cosigned", "V", "seed", "result", "privacct", "pubacct", 0 };

    // Kademlia DHT
    static char *ping[] = { (char *)ping_func, "ping", "V", "pubkey", "ipaddr", "port", "destip", 0 };
    static char *pong[] = { (char *)pong_func, "pong", "V", "pubkey", "ipaddr", "port", 0 };
    static char *store[] = { (char *)store_func, "store", "V", "pubkey", "key", "name", "data", 0 };
    static char *findvalue[] = { (char *)findvalue_func, "findvalue", "V", "pubkey", "key", "name", "data", 0 };
    static char *findnode[] = { (char *)findnode_func, "findnode", "V", "pubkey", "key", "name", "data", 0 };
    static char *havenode[] = { (char *)havenode_func, "havenode", "V", "pubkey", "key", "name", "data", 0 };
    static char *havenodeB[] = { (char *)havenodeB_func, "havenodeB", "V", "pubkey", "key", "name", "data", 0 };
    static char *findaddress[] = { (char *)findaddress_func, "findaddress", "V", "refaddr", "list", "dist", "duration", "numthreads", 0 };

    // MofNfs
    static char *savefile[] = { (char *)savefile_func, "savefile", "V", "filename", "L", "M", "N", "backup", "password", "pin", 0 };
    static char *restorefile[] = { (char *)restorefile_func, "restorefile", "V", "filename", "L", "M", "N", "backup", "password", "destfile", "sharenrs", "txids", "pin", 0 };
    static char *sendfile[] = { (char *)sendfile_func, "sendfile", "V", "filename", "dest", "L", 0 };
    
    // Telepathy
    static char *getpeers[] = { (char *)getpeers_func, "getpeers", "V",  "scan", 0 };
    static char *addcontact[] = { (char *)addcontact_func, "addcontact", "V",  "handle", "acct", 0 };
    static char *removecontact[] = { (char *)removecontact_func, "removecontact", "V",  "contact", 0 };
    static char *dispcontact[] = { (char *)dispcontact_func, "dispcontact", "V",  "contact", 0 };
    static char *telepathy[] = { (char *)telepathy_func, "telepathy", "V",  "contact", "id", "type", "attach", 0 };
    static char *getdb[] = { (char *)getdb_func, "getdb", "V",  "contact", "id", "key", "dir", 0 };
    static char *sendmsg[] = { (char *)sendmsg_func, "sendmessage", "V", "dest", "msg", "L", 0 };
    static char *sendbinary[] = { (char *)sendbinary_func, "sendbinary", "V", "dest", "data", "L", 0 };
    static char *checkmsg[] = { (char *)checkmsg_func, "checkmessages", "V", "sender", 0 };

    // Teleport
    static char *maketelepods[] = { (char *)maketelepods_func, "maketelepods", "V", "amount", "coin", 0 };
    static char *teleport[] = { (char *)teleport_func, "teleport", "V", "amount", "contact", "coin", "minage", "withdraw", 0 };
    static char *telepodacct[] = { (char *)telepodacct_func, "telepodacct", "V", "amount", "contact", "coin", "comment", "cmd", "withdraw", 0 };
   //static char *telepod[] = { (char *)telepod_func, "telepod", "V", "crc", "i", "h", "c", "v", "a", "t", "o", "p", "k", "L", "s", "M", "N", "D", 0 };
    //static char *transporter[] = { (char *)transporter_func, "transporter", "V", "coin", "height", "minage", "value", "totalcrc", "telepods", "M", "N", "sharenrs", "pubaddr", 0 };
    //static char *transporterstatus[] = { (char *)transporterstatus_func, "transporter_status", "V", "status", "coin", "totalcrc", "value", "num", "minage", "height", "crcs", "sharei", "M", "N", "sharenrs", "ind", "pubaddr", 0 };
    
   // InstantDEX
    static char *respondtx[] = { (char *)respondtx_func, "respondtx", "V", "signedtx", 0 };
    static char *processutx[] = { (char *)processutx_func, "processutx", "V", "utx", "sig", "full", 0 };
    static char *orderbook[] = { (char *)orderbook_func, "orderbook", "V", "obookid", "polarity", "allfields", 0 };
    static char *getorderbooks[] = { (char *)getorderbooks_func, "getorderbooks", "V", 0 };
    static char *placebid[] = { (char *)placebid_func, "placebid", "V", "obookid", "polarity", "volume", "price", "assetA", "assetB", 0 };
    static char *placeask[] = { (char *)placeask_func, "placeask", "V", "obookid", "polarity", "volume", "price", "assetA", "assetB", 0 };
    static char *makeoffer[] = { (char *)makeoffer_func, "makeoffer", "V", "other", "assetA", "qtyA", "assetB", "qtyB", "type", 0 };
    
    // Tradebot
    static char *tradebot[] = { (char *)tradebot_func, "tradebot", "V", "code", 0 };

     static char **commands[] = { gotjson, gotpacket, gotnewpeer, getdb, cosign, cosigned, telepathy, addcontact, dispcontact, removecontact, findaddress, ping, pong, store, findnode, havenode, havenodeB, findvalue, sendfile, getpeers, maketelepods, tradebot, respondtx, processutx, checkmsg, placebid, placeask, makeoffer, sendmsg, sendbinary, orderbook, getorderbooks, teleport, telepodacct, savefile, restorefile  };
    int32_t i,j;
    struct coin_info *cp;
    cJSON *argjson,*obj,*nxtobj,*secretobj,*objs[64];
    char NXTaddr[MAX_JSON_FIELD],NXTACCTSECRET[MAX_JSON_FIELD],command[MAX_JSON_FIELD],**cmdinfo,*retstr=0;
    memset(objs,0,sizeof(objs));
    command[0] = 0;
    memset(NXTaddr,0,sizeof(NXTaddr));
    if ( is_cJSON_Array(origargjson) != 0 )
        argjson = cJSON_GetArrayItem(origargjson,0);
    else argjson = origargjson;
    NXTACCTSECRET[0] = 0;
    if ( argjson != 0 )
    {
        obj = cJSON_GetObjectItem(argjson,"requestType");
        nxtobj = cJSON_GetObjectItem(argjson,"NXT");
        secretobj = cJSON_GetObjectItem(argjson,"secret");
        copy_cJSON(NXTaddr,nxtobj);
        copy_cJSON(command,obj);
        copy_cJSON(NXTACCTSECRET,secretobj);
        if ( NXTACCTSECRET[0] == 0 && (cp= get_coin_info("BTCD")) != 0 )
        {
            if ( prevaddr == 0 || strcmp(command,"findnode") != 0 )
            {
                if ( 1 || strcmp("127.0.0.1",cp->privacyserver) == 0 )
                {
                    safecopy(NXTACCTSECRET,cp->srvNXTACCTSECRET,sizeof(NXTACCTSECRET));
                    expand_nxt64bits(NXTaddr,cp->srvpubnxtbits);
                    //printf("use localserver NXT.%s to send command\n",NXTaddr);
                }
                else
                {
                    safecopy(NXTACCTSECRET,cp->privateNXTACCTSECRET,sizeof(NXTACCTSECRET));
                    expand_nxt64bits(NXTaddr,cp->privatebits);
                    //printf("use NXT.%s to send command\n",NXTaddr);
                }
            }
        }
        //printf("(%s) command.(%s) NXT.(%s)\n",cJSON_Print(argjson),command,NXTaddr);
        //printf("SuperNET_json_commands sender.(%s) valid.%d | size.%d | command.(%s) orig.(%s)\n",sender,valid,(int32_t)(sizeof(commands)/sizeof(*commands)),command,origargstr);
        for (i=0; i<(int32_t)(sizeof(commands)/sizeof(*commands)); i++)
        {
            cmdinfo = commands[i];
            //printf("needvalid.(%c) sender.(%s) valid.%d %d of %d: cmd.(%s) vs command.(%s)\n",cmdinfo[2][0],sender,valid,i,(int32_t)(sizeof(commands)/sizeof(*commands)),cmdinfo[1],command);
            if ( strcmp(cmdinfo[1],command) == 0 )
            {
                if ( cmdinfo[2][0] != 0 && valid <= 0 )
                    return(0);
                for (j=3; cmdinfo[j]!=0&&j<3+(int32_t)(sizeof(objs)/sizeof(*objs)); j++)
                    objs[j-3] = cJSON_GetObjectItem(argjson,cmdinfo[j]);
                retstr = (*(json_handler)cmdinfo[0])(NXTaddr,NXTACCTSECRET,prevaddr,sender,valid,objs,j-3,origargstr);
                break;
            }
        }
    } else printf("not JSON to parse?\n");
    return(retstr);
}


#endif

