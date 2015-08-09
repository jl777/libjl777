//
//  pluginapi.h
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

char *checkmessages(char *NXTaddr,char *NXTACCTSECRET,char *name,uint64_t daemonid,uint64_t instanceid)
{
    char *msg,*retstr = 0;
    cJSON *array = 0,*json = 0;
    struct daemon_info *dp;
    int32_t i,ind;
    if ( (dp= find_daemoninfo(&ind,name,daemonid,instanceid)) != 0 )
    {
        for (i=0; i<10; i++)
        {
            if ( (msg= queue_dequeue(&dp->messages,1)) != 0 )
            {
                if ( json == 0 )
                    json = cJSON_CreateObject(), array = cJSON_CreateArray();
                cJSON_AddItemToArray(array,cJSON_CreateString(msg));
                free_queueitem(msg);
            }
        }
        if ( json == 0 )
            return(clonestr("{\"result\":\"no messages\",\"messages\":[]}"));
        else
        {
            cJSON_AddItemToObject(json,"result",cJSON_CreateString("daemon messages"));
            cJSON_AddItemToObject(json,"messages",array);
            retstr = cJSON_Print(json);
            free_json(json);
            return(retstr);
        }
    }
    return(clonestr("{\"error\":\"cant find daemonid\"}"));
}

char *checkmsg_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char plugin[MAX_JSON_FIELD],*retstr = 0;
    uint64_t daemonid,instanceid;
    copy_cJSON(plugin,objs[0]);
    daemonid = get_API_nxt64bits(objs[1]);
    instanceid = get_API_nxt64bits(objs[2]);
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    if ( sender[0] != 0 && valid > 0 )
        retstr = checkmessages(sender,NXTACCTSECRET,plugin,daemonid,instanceid);
    else retstr = clonestr("{\"result\":\"invalid checkmessages request\"}");
    return(retstr);
}

char *register_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char *register_daemon(char *plugin,uint64_t daemonid,uint64_t instanceid,cJSON *methodsjson);
    char plugin[MAX_JSON_FIELD],retbuf[1024],*methodstr;
    uint64_t daemonid,instanceid;
    struct daemon_info *dp;
    int32_t ind;
    if ( is_remote_access(previpaddr) != 0 )
        return(clonestr("{\"error\":\"cant remote register plugins\"}"));
    copy_cJSON(plugin,objs[0]);
    daemonid = get_API_nxt64bits(objs[1]);
    instanceid = get_API_nxt64bits(objs[2]);
    return(register_daemon(plugin,daemonid,instanceid,objs[3]));
 }

char *plugin_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char plugin[MAX_JSON_FIELD],method[MAX_JSON_FIELD],tagstr[MAX_JSON_FIELD],*retstr = 0;
    struct daemon_info *dp;
    uint64_t daemonid,instanceid,tag;
    int32_t ind,i,async,n = 1;
    copy_cJSON(plugin,objs[0]);
    daemonid = get_API_nxt64bits(objs[1]);
    instanceid = get_API_nxt64bits(objs[2]);
    copy_cJSON(method,objs[3]);
    copy_cJSON(tagstr,objs[4]);
    n = get_API_int(objs[5],1);
    tag = calc_nxt64bits(tagstr);
    timeout = get_API_int(objs[6],1000);
    return(plugin_method(0,previpaddr,plugin,method,daemonid,instanceid,tag,origargstr,0,timeout));
}

char *passthru_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char hopNXTaddr[64],tagstr[MAX_JSON_FIELD],coinstr[MAX_JSON_FIELD],plugin[MAX_JSON_FIELD],method[MAX_JSON_FIELD],params[MAX_JSON_FIELD],*str2,*cmdstr,*retstr = 0;
    struct coin_info *cp = 0;
    uint64_t daemonid,instanceid,tag;
    copy_cJSON(coinstr,objs[0]);
    copy_cJSON(method,objs[1]);
    if ( is_remote_access(previpaddr) != 0 )
    {
        if ( in_jsonarray(cJSON_GetObjectItem(MGWconf,"remote"),method) == 0 && in_jsonarray(cJSON_GetObjectItem(cp->json,"remote"),method) == 0 )
            return(0);
    }
    copy_cJSON(params,objs[2]);
    unstringify(params), stripwhite_ns(params,strlen(params));
    copy_cJSON(tagstr,objs[3]);
    if ( is_decimalstr(tagstr) != 0 )
        tag = calc_nxt64bits(tagstr);
    else tag = rand();
    copy_cJSON(plugin,objs[4]);
    daemonid = get_API_nxt64bits(objs[5]);
    instanceid = get_API_nxt64bits(objs[6]);
    printf("daemonid.%llu tag.(%s) passthru.(%s) %p method=%s [%s] plugin.(%s)\n",(long long)daemonid,tagstr,coinstr,cp,method,params,plugin);
    if ( sender[0] != 0 && valid > 0 )
    {
        if ( daemonid != 0 )
        {
            unstringify(params);
            send_to_daemon(-1,&retstr,plugin,daemonid,instanceid,params,(int32_t)strlen(params),is_remote_access(previpaddr)==0,0);
            return(wait_for_daemon(&retstr,tag,10,10000));
        }
        else if ( (cp= get_coin_info(coinstr)) != 0 && method[0] != 0 )
            retstr = bitcoind_passthru(cp->name,cp->serverport,cp->userpass,method,params);
    }
    else retstr = clonestr("{\"error\":\"invalid passthru_func arguments\"}");
    if ( is_remote_access(previpaddr) != 0 )
    {
        cmdstr = malloc(strlen(retstr)+512);
        str2 = stringifyM(retstr);
        sprintf(cmdstr,"{\"requestType\":\"remote\",\"coin\":\"%s\",\"method\":\"%s\",\"tag\":\"%s\",\"result\":\"%s\"}",coinstr,method,tagstr,str2);
        free(str2);
        hopNXTaddr[0] = 0;
        retstr = send_tokenized_cmd(!prevent_queueing("passthru"),hopNXTaddr,0,NXTaddr,NXTACCTSECRET,cmdstr,sender);
        free(cmdstr);
    }
    return(retstr);
}

char *remote_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    if ( is_remote_access(previpaddr) == 0 )
        return(clonestr("{\"error\":\"cant remote locally\"}"));
    return(clonestr(origargstr));
}

char *syscall_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char jsonargs[MAX_JSON_FIELD],syscall[MAX_JSON_FIELD],plugin[MAX_JSON_FIELD],ipaddr[MAX_JSON_FIELD],*jsonstr,*str;
    cJSON *json;
    int32_t launchflag,websocket;
    uint16_t port;
    copy_cJSON(syscall,objs[0]);
    websocket = get_API_int(objs[2],0);
    copy_cJSON(jsonargs,objs[3]);
    copy_cJSON(plugin,objs[4]);
    if ( is_bundled_plugin(plugin) != 0 )
        launchflag = 1;
    else launchflag = get_API_int(objs[1],0);
    copy_cJSON(ipaddr,objs[5]);
    port = get_API_int(objs[6],0);
    if ( (json= cJSON_Parse(jsonargs)) != 0 )
    {
        jsonstr = cJSON_Print(json);
        free_json(json);
        str = stringifyM(jsonstr);
        strcpy(jsonargs,str);
        free(str);
        free(jsonstr);
        stripwhite_ns(jsonargs,strlen(jsonargs));
    }
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    return(language_func(plugin,ipaddr,port,websocket,launchflag,syscall,jsonargs,call_system));
}

