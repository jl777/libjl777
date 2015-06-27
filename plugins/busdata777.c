//
//  relays777.c
//  SuperNET API extension example plugin
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

#define BUNDLED
#define PLUGINSTR "relay"
#define PLUGNAME(NAME) relay ## NAME
#define STRUCTNAME struct PLUGNAME(_info) 
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
#include "system777.c"
#include "NXT777.c"
#include "plugin777.c"
#include "SaM.c"
#undef DEFINES_ONLY

int32_t issue_generateToken(char encoded[NXT_TOKEN_LEN],char *key,char *secret)
{
    char cmd[16384],secretstr[8192],token[MAX_JSON_FIELD+2*NXT_TOKEN_LEN+1],*jsontxt; cJSON *tokenobj,*json;
    encoded[0] = 0;
    if ( strlen(secretstr) >= sizeof(secretstr)/3-1 )
    {
        fprintf(stderr,"secret too long!\n");
        return(-1);
    }
    escape_code(secretstr,secret);
    sprintf(cmd,"requestType=generateToken&website=%s&secretPhrase=%s",key,secretstr);
    if ( (jsontxt= issue_NXTPOST(cmd)) != 0 )
    {
        //printf("(%s) -> (%s)\n",cmd,jsontxt);
        if ( (json= cJSON_Parse(jsontxt)) != 0 )
        {
            //printf("(%s) -> token.(%s)\n",cmd,cJSON_Print(json));
            tokenobj = cJSON_GetObjectItem(json,"token");
            copy_cJSON(token,tokenobj);
            if ( encoded != 0 )
                strcpy(encoded,token);
            free_json(json);
        }
        free(jsontxt);
    }
    return(-1);
}

uint32_t calc_nonce(char *str,int32_t leverage,int32_t maxmillis,uint32_t nonce)
{
    uint64_t hit,threshold; bits384 sig; double endmilli; int32_t len,numrounds = 10;
    len = (int32_t)strlen(str);
    if ( leverage != 0 )
    {
        threshold = calc_SaMthreshold(leverage);
        if ( maxmillis == 0 )
        {
            if ( (hit= calc_SaM(&sig,(void *)str,len,(void *)&nonce,sizeof(nonce),numrounds)) >= threshold )
            {
                printf("nonce failure hit.%llu >= threshold.%llu\n",(long long)hit,(long long)threshold);
                if ( (threshold - hit) > (1L << 32) )
                    return(0xffffffff);
                else return((uint32_t)(threshold - hit));
            }
        }
        else
        {
            endmilli = (milliseconds() + maxmillis);
            while ( milliseconds() < endmilli )
            {
                randombytes((void *)&nonce,sizeof(nonce));
                if ( (hit= calc_SaM(&sig,(void *)str,len,(void *)&nonce,sizeof(nonce),numrounds)) < threshold )
                    return(nonce);
            }
        }
    }
    return(0);
}

uint32_t nonce_func(int32_t *leveragep,char *str,char *broadcaststr,int32_t maxmillis,uint32_t nonce)
{
    int32_t leverage = 2;
    if ( broadcaststr != 0 && broadcaststr[0] != 0 )
    {
        if ( strcmp(broadcaststr,"allnodes") == 0 )
            leverage = 6;
        else if ( strcmp(broadcaststr,"allrelays") == 0 )
            leverage = 4;
    }
    if ( maxmillis == 0 && *leveragep != leverage )
        return(0xffffffff);
    *leveragep = leverage;
    return(calc_nonce(str,leverage,maxmillis,nonce));
}

int32_t construct_tokenized_req(char *tokenized,char *cmdjson,char *NXTACCTSECRET,char *broadcastmode)
{
    char encoded[2*NXT_TOKEN_LEN+1],broadcaststr[512]; uint32_t nonce,nonceerr; int32_t i,leverage;
    if ( broadcastmode == 0 )
        broadcastmode = "";
    _stripwhite(cmdjson,' ');
    for (i=0; i<100; i++)
    {
        if ( (nonce= nonce_func(&leverage,cmdjson,broadcastmode,5000,0)) != 0 )
            break;
        printf("iter.%d nonce.%u failed, try again\n",i,nonce);
    }
    if ( (nonceerr= nonce_func(&leverage,cmdjson,broadcastmode,0,nonce)) != 0 )
    {
        printf("error validating nonce.%u -> %u\n",nonce,nonceerr);
        tokenized[0] = 0;
        return(0);
    }
    sprintf(broadcaststr,",\"broadcast\":\"%s\",\"usedest\":\"yes\",\"nonce\":\"%u\",\"leverage\":\"%u\"",broadcastmode,nonce,leverage);
    //sprintf(broadcaststr,",\"broadcast\":\"%s\",\"usedest\":\"yes\"",broadcastmode);
    //printf("GEN.(%s).(%s) -> (%s) len.%d crc.%u\n",broadcastmode,cmdjson,broadcaststr,(int32_t)strlen(cmdjson),_crc32(0,(void *)cmdjson,(int32_t)strlen(cmdjson)));
    issue_generateToken(encoded,cmdjson,NXTACCTSECRET);
    encoded[NXT_TOKEN_LEN] = 0;
    if ( SUPERNET.iamrelay == 0 )
        sprintf(tokenized,"[%s, {\"token\":\"%s\"%s}]",cmdjson,encoded,broadcaststr);
    else sprintf(tokenized,"[%s, {\"token\":\"%s\",\"forwarder\":\"%s\"%s}]",cmdjson,encoded,SUPERNET.NXTADDR,broadcaststr);
    return((int32_t)strlen(tokenized)+1);
}

int32_t issue_decodeToken(char *sender,int32_t *validp,char *key,unsigned char encoded[NXT_TOKEN_LEN])
{
    char cmd[4096],token[MAX_JSON_FIELD+2*NXT_TOKEN_LEN+1],*retstr;
    cJSON *nxtobj,*validobj,*json;
    *validp = -1;
    sender[0] = 0;
    memcpy(token,encoded,NXT_TOKEN_LEN);
    token[NXT_TOKEN_LEN] = 0;
    sprintf(cmd,"requestType=decodeToken&website=%s&token=%s",key,token);
    if ( (retstr = issue_NXTPOST(cmd)) != 0 )
    {
        //printf("(%s) -> (%s)\n",cmd,retstr);
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            validobj = cJSON_GetObjectItem(json,"valid");
            if ( validobj != 0 )
                *validp = ((validobj->type&0xff) == cJSON_True) ? 1 : 0;
            nxtobj = cJSON_GetObjectItem(json,"account");
            copy_cJSON(sender,nxtobj);
            free_json(json), free(retstr);
            //printf("decoded valid.%d NXT.%s len.%d\n",*validp,sender,(int32_t)strlen(sender));
            if ( sender[0] != 0 )
                return((int32_t)strlen(sender));
            else return(0);
        }
        free(retstr);
    }
    return(-1);
}

int32_t validate_token(char *forwarder,char *pubkey,char *NXTaddr,char *tokenizedtxt,int32_t strictflag)
{
    cJSON *array=0,*firstitem=0,*tokenobj,*obj; uint32_t nonce; int64_t timeval,diff = 0; int32_t valid,leverage,retcode = -13;
    char buf[MAX_JSON_FIELD],sender[MAX_JSON_FIELD],broadcaststr[MAX_JSON_FIELD],*firstjsontxt = 0; unsigned char encoded[4096];
    array = cJSON_Parse(tokenizedtxt);
    NXTaddr[0] = pubkey[0] = forwarder[0] = 0;
    if ( array == 0 )
    {
        printf("couldnt validate.(%s)\n",tokenizedtxt);
        return(-2);
    }
    if ( is_cJSON_Array(array) != 0 && cJSON_GetArraySize(array) == 2 )
    {
        firstitem = cJSON_GetArrayItem(array,0);
        if ( pubkey != 0 )
        {
            obj = cJSON_GetObjectItem(firstitem,"pubkey");
            copy_cJSON(pubkey,obj);
        }
        obj = cJSON_GetObjectItem(firstitem,"NXT"), copy_cJSON(buf,obj);
        if ( NXTaddr[0] != 0 && strcmp(buf,NXTaddr) != 0 )
            retcode = -3;
        else
        {
            strcpy(NXTaddr,buf);
//printf("decoded.(%s)\n",NXTaddr);
            if ( strictflag != 0 )
            {
                timeval = get_cJSON_int(firstitem,"time");
                diff = timeval - time(NULL);
                if ( diff < 0 )
                    diff = -diff;
                if ( diff > strictflag )
                {
                    printf("time diff %lld too big %lld vs %ld\n",(long long)diff,(long long)timeval,time(NULL));
                    retcode = -5;
                }
            }
            if ( retcode != -5 )
            {
                firstjsontxt = cJSON_Print(firstitem), _stripwhite(firstjsontxt,' ');
//printf("(%s)\n",firstjsontxt);
                tokenobj = cJSON_GetArrayItem(array,1);
                obj = cJSON_GetObjectItem(tokenobj,"token");
                copy_cJSON((char *)encoded,obj);
                copy_cJSON(forwarder,cJSON_GetObjectItem(tokenobj,"forwarder"));
                memset(sender,0,sizeof(sender));
                valid = -1;
                if ( issue_decodeToken(sender,&valid,firstjsontxt,encoded) > 0 )
                {
                    if ( NXTaddr[0] == 0 )
                        strcpy(NXTaddr,sender);
                    if ( strcmp(sender,NXTaddr) == 0 )
                    {
                        nonce = (uint32_t)get_API_int(cJSON_GetObjectItem(tokenobj,"nonce"),0);
                        leverage = (uint32_t)get_API_int(cJSON_GetObjectItem(tokenobj,"leverage"),0);
                        copy_cJSON(broadcaststr,cJSON_GetObjectItem(tokenobj,"broadcast"));
                        retcode = valid;
                        //int32_t len = (int32_t)strlen(firstjsontxt);
                        if ( nonce_func(&leverage,firstjsontxt,broadcaststr,0,nonce) != 0 )
                        {
                            //printf("(%s) -> (%s) leverage.%d len.%d crc.%u\n",broadcaststr,firstjsontxt,leverage,len,_crc32(0,(void *)firstjsontxt,len));
                            retcode = -4;
                        }
                        if ( Debuglevel > 2 )
                            printf("signed by valid NXT.%s valid.%d diff.%lld forwarder.(%s)\n",sender,valid,(long long)diff,forwarder);
                    }
                    else
                    {
                        printf("valid.%d diff sender.(%s) vs NXTaddr.(%s)\n",valid,sender,NXTaddr);
                        //if ( strcmp(NXTaddr,buf) == 0 )
                        //    retcode = valid;
                        retcode = -7;
                    }
                } else printf("decode error\n");
                if ( retcode < 0 )
                    printf("err.%d: signed by invalid sender.(%s) NXT.%s valid.%d or timediff too big diff.%lld, buf.(%s)\n",retcode,sender,NXTaddr,valid,(long long)diff,tokenizedtxt);
                free(firstjsontxt);
            }
        }
    } else printf("decode arraysize.%d\n",cJSON_GetArraySize(array));
    if ( array != 0 )
        free_json(array);
    //printf("validate retcode.%d\n",retcode);
    return(retcode);
}

void nn_syncbus(cJSON *json)
{
    cJSON *argjson,*second; char forwarder[MAX_JSON_FIELD],*jsonstr; uint64_t forwardbits,nxt64bits;
    //printf("pubsock.%d iamrelay.%d arraysize.%d\n",RELAYS.pubsock,SUPERNET.iamrelay,cJSON_GetArraySize(json));
    if ( RELAYS.bus.sock >= 0 && SUPERNET.iamrelay != 0 && is_cJSON_Array(json) != 0 && cJSON_GetArraySize(json) == 2 )
    {
        argjson = cJSON_GetArrayItem(json,0);
        second = cJSON_GetArrayItem(json,1);
        copy_cJSON(forwarder,cJSON_GetObjectItem(second,"forwarder"));
        ensure_jsonitem(second,"forwarder",SUPERNET.NXTADDR);
        jsonstr = cJSON_Print(json), _stripwhite(jsonstr,' ');
        forwardbits = conv_acctstr(forwarder), nxt64bits = conv_acctstr(SUPERNET.NXTADDR);
        if ( forwardbits == 0 )//|| forwardbits == nxt64bits )
        {
            printf("BUS-SEND.(%s) forwarder.%llu vs %llu\n",jsonstr,(long long)forwardbits,(long long)nxt64bits);
            nn_send(RELAYS.bus.sock,jsonstr,(int32_t)strlen(jsonstr)+1,0);
        }
        free(jsonstr);
    }
}

char *busdata_decrypt(char *sender,uint8_t *msg,int32_t datalen)
{
    uint8_t *buf;
    buf = malloc(datalen);
    decode_hex(buf,datalen,(char *)msg);
    return((char *)buf);
}

cJSON *busdata_decode(char *destNXT,int32_t validated,char *sender,uint8_t *msg,int32_t datalen)
{
    char *jsonstr; cJSON *json = 0;
    if ( validated >= 0 )
    {
        if ( (jsonstr= busdata_decrypt(sender,msg,datalen)) != 0 )
        {
            json = cJSON_Parse((char *)msg);
            copy_cJSON(destNXT,cJSON_GetObjectItem(json,"destNXT"));
            free(jsonstr);
        } else printf("couldnt decrypt.(%s)\n",msg);
    } else printf("neg validated.%d\n",validated);
    return(json);
}

queue_t busdataQ[2];
struct busdata_item { struct queueitem DL; bits256 hash; cJSON *json; char *retstr,*key; uint64_t dest64bits,senderbits; uint32_t queuetime,donetime; };
struct service_provider { UT_hash_handle hh; int32_t sock,numendpoints; char **endpoints; } *Service_providers;

char *lb_serviceprovider(struct service_provider *sp,uint8_t *data,int32_t datalen)
{
    int32_t i,sendlen,recvlen; char *msg,*jsonstr = 0;
    for (i=0; i<10; i++)
        if ( (nn_socket_status(sp->sock,1) & NN_POLLOUT) != 0 )
            break;
    printf("lb_serviceprovider.(%s)\n",data);
    if ( (sendlen= nn_send(sp->sock,data,datalen,0)) == datalen )
    {
        for (i=0; i<1000; i++)
            if ( (nn_socket_status(sp->sock,1) & NN_POLLIN) != 0 )
                break;
        if ( (recvlen= nn_recv(sp->sock,&msg,NN_MSG,0)) > 0 )
        {
            jsonstr = clonestr((char *)msg);
            nn_freemsg(msg);
        }
    } else printf("sendlen.%d != datalen.%d\n",sendlen,datalen);
    return(jsonstr);
}

char *busdata_addpending(char *destNXT,char *sender,char *key,uint32_t timestamp,cJSON *json,char *forwarder,cJSON *origjson)
{
    cJSON *argjson; struct busdata_item *ptr = calloc(1,sizeof(*ptr));
    struct service_provider *sp; int32_t i,sendtimeout,recvtimeout,retrymillis,maxmillis;
    char submethod[512],endpoint[512],destplugin[512],retbuf[128],servicename[512],*hashstr,*str,*retstr;
    if ( key == 0 || key[0] == 0 )
        key = "0";
    ptr->json = json, ptr->queuetime = (uint32_t)time(NULL), ptr->key = clonestr(key);
    ptr->dest64bits = conv_acctstr(destNXT), ptr->senderbits = conv_acctstr(sender);
    if ( (hashstr= cJSON_str(cJSON_GetObjectItem(json,"H"))) != 0 )
        decode_hex(ptr->hash.bytes,sizeof(ptr->hash),hashstr);
    else memset(ptr->hash.bytes,0,sizeof(ptr->hash));
    copy_cJSON(submethod,cJSON_GetObjectItem(json,"submethod"));
    copy_cJSON(destplugin,cJSON_GetObjectItem(json,"destplugin"));
    copy_cJSON(servicename,cJSON_GetObjectItem(json,"servicename"));
    if ( strcmp(submethod,"serviceprovider") == 0 )
    {
        copy_cJSON(endpoint,cJSON_GetObjectItem(json,"endpoint"));
        HASH_FIND(hh,Service_providers,servicename,strlen(servicename),sp);
        if ( sp != 0 )
        {
            if ( sp->numendpoints > 0 )
            {
                for (i=0; i<sp->numendpoints; i++)
                    if ( strcmp(sp->endpoints[i],endpoint) == 0 )
                        return(clonestr("{\"result\":\"serviceprovider duplicate endpoint\"}"));
            }
        }
        if ( sp == 0 )
        {
            sp = calloc(1,sizeof(*sp));
            HASH_ADD_KEYPTR(hh,Service_providers,servicename,strlen(servicename),sp);
            sp->sock = nn_socket(AF_SP,NN_REQ);
            sendtimeout = 1000, recvtimeout = 10000, maxmillis = 1000, retrymillis = 25;
            if ( sendtimeout > 0 && nn_setsockopt(sp->sock,NN_SOL_SOCKET,NN_SNDTIMEO,&sendtimeout,sizeof(sendtimeout)) < 0 )
                fprintf(stderr,"error setting sendtimeout %s\n",nn_errstr());
            else if ( recvtimeout > 0 && nn_setsockopt(sp->sock,NN_SOL_SOCKET,NN_RCVTIMEO,&recvtimeout,sizeof(recvtimeout)) < 0 )
                fprintf(stderr,"error setting sendtimeout %s\n",nn_errstr());
            else if ( nn_setsockopt(sp->sock,NN_SOL_SOCKET,NN_RECONNECT_IVL,&retrymillis,sizeof(retrymillis)) < 0 )
                fprintf(stderr,"error setting NN_REQ NN_RECONNECT_IVL_MAX socket %s\n",nn_errstr());
            else if ( nn_setsockopt(sp->sock,NN_SOL_SOCKET,NN_RECONNECT_IVL_MAX,&maxmillis,sizeof(maxmillis)) < 0 )
                fprintf(stderr,"error setting NN_REQ NN_RECONNECT_IVL_MAX socket %s\n",nn_errstr());
            fprintf(stderr,"create servicename.(%s) sock.%d <-> (%s)\n",servicename,sp->sock,endpoint);
        }
        sp->endpoints = realloc(sp->endpoints,sizeof(*sp->endpoints) * (sp->numendpoints + 1));
        sp->endpoints[sp->numendpoints++] = clonestr(endpoint);
        nn_connect(sp->sock,endpoint);
        nn_syncbus(origjson);
        sprintf(retbuf,"{\"result\":\"serviceprovider added\",\"endpoint\":\"%s\"}",endpoint);
        return(clonestr(retbuf));
    }
    else
    {
        HASH_FIND(hh,Service_providers,servicename,strlen(servicename),sp);
        printf("service.%s (%s) sp.%p\n",servicename,submethod,sp);
        if ( sp == 0 )
            return(clonestr("{\"result\":\"serviceprovider not found\"}"));
        else
        {
            argjson = cJSON_Duplicate(origjson,1);
            ensure_jsonitem(cJSON_GetArrayItem(argjson,1),"usedest","yes");
            str = cJSON_Print(argjson), _stripwhite(str,' ');
            free_json(argjson);
            if ( (retstr= lb_serviceprovider(sp,(uint8_t *)str,(int32_t)strlen(str)+1)) != 0 )
            {
                free(str);
                return(retstr);
            }
            free(str);
            return(clonestr("{\"result\":\"no response from provider\"}"));
        }
    }
    printf("%s -> %s add pending %llx\n",sender,destNXT,(long long)ptr->hash.txid);
    queue_enqueue("busdata",&busdataQ[0],&ptr->DL);
    return(0);
}

int32_t busdata_match(struct busdata_item *ptr,uint64_t dest64bits,uint64_t senderbits,char *key,uint32_t timestamp,cJSON *json)
{
    if ( ptr->dest64bits == senderbits && ptr->senderbits == dest64bits && strcmp(key,ptr->key) == 0 )
        return(1);
    else return(0);
}

int32_t busdata_isduplicate(char *destNXT,char *sender,char *key,uint32_t timestamp,cJSON *json)
{
    char *hashstr; bits256 hash; struct queueitem *ptr; struct busdata_item *busdata; int32_t i,iter;
    if ( (hashstr= cJSON_str(cJSON_GetObjectItem(json,"H"))) != 0 )
        decode_hex(hash.bytes,sizeof(hash),hashstr);
    else memset(hash.bytes,0,sizeof(hash));
    for (iter=0; iter<2; iter++)
    {
        i = 0;
        DL_FOREACH(busdataQ[iter].list,ptr)
        {
            busdata = (struct busdata_item *)ptr;
            //printf("%d.(%llx vs %llx).i%d ",iter,(long long)busdata->hash.txid,(long long)hash.txid,i);
            if ( busdata->hash.txid == hash.txid )
                return(1 * 0);
            i++;
        }
    }
    return(0);
}

char *busdata_matchquery(char *response,char *destNXT,char *sender,char *key,uint32_t timestamp,cJSON *json)
{
    uint64_t dest64bits,senderbits; struct busdata_item *ptr; char *retstr = 0; int32_t iter; uint32_t now = (uint32_t)time(NULL);
    dest64bits = conv_acctstr(destNXT), senderbits = conv_acctstr(sender);
    for (iter=0; iter<2; iter++)
    {
        if ( (ptr= queue_dequeue(&busdataQ[iter],0)) != 0 )
        {
            if ( busdata_match(ptr,dest64bits,senderbits,key,timestamp,json) != 0 )
            {
                if ( (retstr= ptr->retstr) != 0 )
                    retstr = clonestr("{\"result\":\"busdata request done\"}");
                if ( ptr->json != 0 )
                    free_json(ptr->json);
                if ( ptr->key != 0 )
                    free(ptr->key);
                return(retstr);
            }
            else if ( (now - ptr->queuetime) > 600 )
            {
                printf("expired busdataQ.%u at %u\n",ptr->queuetime,now);
                if ( ptr->retstr != 0 )
                    free(ptr->retstr);
                if ( ptr->json != 0 )
                    free_json(ptr->json);
                if ( ptr->key != 0 )
                    free(ptr->key);
                free(ptr);
            }
            else queue_enqueue("re-busdata",&busdataQ[iter ^ 1],&ptr->DL);
        }
    }
    return(retstr);
}

char *busdata(char *forwarder,char *sender,int32_t valid,char *key,uint32_t timestamp,uint8_t *msg,int32_t datalen,cJSON *origjson)
{
    cJSON *json; char destNXT[64],response[1024],*retstr = 0;
    //printf("busdata\n");
    if ( SUPERNET.iamrelay != 0 && valid > 0 )
    {
        if ( (json= busdata_decode(destNXT,valid,sender,msg,datalen)) != 0 )
        {
            copy_cJSON(response,cJSON_GetObjectItem(json,"response"));
            if ( response[0] == 0 )
            {
                //if ( busdata_isduplicate(destNXT,sender,key,timestamp,json) != 0 )
                //    return(clonestr("{\"error\":\"busdata duplicate request\"}"));
                if ( (retstr= busdata_addpending(destNXT,sender,key,timestamp,json,forwarder,origjson)) == 0 )
                {
                    nn_syncbus(origjson);
                }
            }
            else if ( (retstr= busdata_matchquery(response,destNXT,sender,key,timestamp,json)) != 0 )
            {
                printf("busdata_query returned.(%s)\n",retstr);
                return(retstr);
            }
            else return(clonestr("{\"error\":\"busdata response without matching query\"}"));
            free_json(json);
        } else printf("couldnt decode.(%s)\n",msg);
    }
    return(retstr);
}

int32_t busdata_validate(char *forwarder,char *sender,uint32_t *timestamp,uint8_t *databuf,int32_t *datalenp,void *msg,cJSON *json)
{
    char pubkey[256],hexstr[65],sha[65],datastr[8192]; int32_t valid; cJSON *argjson; bits256 hash;
    *timestamp = *datalenp = 0;
    //printf("busdata_validate.(%s)\n",msg);
    if ( is_cJSON_Array(json) != 0 && cJSON_GetArraySize(json) == 2 )
    {
        argjson = cJSON_GetArrayItem(json,0);
        *timestamp = (uint32_t)get_API_int(cJSON_GetObjectItem(argjson,"time"),0);
        sender[0] = 0;
        if ( (valid= validate_token(forwarder,pubkey,sender,msg,(*timestamp != 0) * MAXTIMEDIFF)) <= 0 )
            return(valid);
        copy_cJSON(sha,cJSON_GetObjectItem(argjson,"H"));
        copy_cJSON(datastr,cJSON_GetObjectItem(argjson,"data"));
        decode_hex(databuf,(int32_t)(strlen(datastr)+1)>>1,datastr);
        *datalenp = (uint32_t)get_API_int(cJSON_GetObjectItem(argjson,"n"),0);
        calc_sha256(hexstr,hash.bytes,databuf,*datalenp);
        //printf("valid.%d sender.(%s) (%s) datalen.%d %llx [%llx]\n",valid,sender,databuf,*datalenp,(long long)hash.txid,(long long)databuf);
        if ( strcmp(hexstr,sha) == 0 )
            return(1);
    }
    return(-1);
}

char *busdata_deref(char *forwarder,char *sender,int32_t valid,char *databuf,cJSON *json)
{
    char plugin[MAX_JSON_FIELD],method[MAX_JSON_FIELD],*broadcaststr,*str,*retstr = 0;
    cJSON *dupjson,*second,*argjson; uint64_t forwardbits;
    if ( SUPERNET.iamrelay != 0 && (broadcaststr= cJSON_str(cJSON_GetObjectItem(cJSON_GetArrayItem(json,1),"broadcast"))) != 0 )
    {
        dupjson = cJSON_Duplicate(json,1);
        second = cJSON_GetArrayItem(dupjson,1);
        ensure_jsonitem(second,"forwarder",SUPERNET.NXTADDR);
        if ( (forwardbits= conv_acctstr(forwarder)) == 0 && cJSON_GetObjectItem(second,"stop") == 0 )
        {
            ensure_jsonitem(second,"stop","end");
            str = cJSON_Print(dupjson), _stripwhite(str,' ');
            printf("broadcast.(%s) forwarder.%llu vs %s\n",str,(long long)forwardbits,SUPERNET.NXTADDR);
            if ( strcmp(broadcaststr,"allrelays") == 0 )
                nn_send(RELAYS.bus.sock,str,(int32_t)strlen(str)+1,0);
            else if ( strcmp(broadcaststr,"allnodes") == 0 )
                nn_send(RELAYS.pubsock,str,(int32_t)strlen(str)+1,0);
            free(str);
        } else printf("forwardbits.%llu stop.%p\n",(long long)forwardbits,cJSON_GetObjectItem(second,"stop"));
        free_json(dupjson);
    }
    if ( (argjson= cJSON_Parse(databuf)) != 0 )
    {
        copy_cJSON(method,cJSON_GetObjectItem(argjson,"submethod"));
        copy_cJSON(plugin,cJSON_GetObjectItem(argjson,"destplugin"));
        cJSON_ReplaceItemInObject(argjson,"method",cJSON_CreateString(method));
        cJSON_ReplaceItemInObject(argjson,"plugin",cJSON_CreateString(plugin));
        cJSON_DeleteItemFromObject(argjson,"submethod");
        cJSON_DeleteItemFromObject(argjson,"destplugin");
        str = cJSON_Print(argjson), _stripwhite(str,' ');
        printf("call (%s %s) (%s)\n",plugin,method,str);
        retstr = plugin_method(0,0,plugin,method,0,0,str,(int32_t)strlen(str)+1,SUPERNET.PLUGINTIMEOUT/2);
        free_json(argjson);
        free(str);
    }
    return(retstr);
}

char *nn_busdata_processor(uint8_t *msg,int32_t len)
{
    cJSON *json,*argjson; uint32_t timestamp; int32_t datalen,valid; uint8_t databuf[8192];
    char usedest[128],key[MAX_JSON_FIELD],src[MAX_JSON_FIELD],forwarder[MAX_JSON_FIELD],sender[MAX_JSON_FIELD],*retstr = 0;
    //printf("nn_busdata_processor\n");
    if ( (json= cJSON_Parse((char *)msg)) != 0 )
    {
        if ( (valid= busdata_validate(forwarder,sender,&timestamp,databuf,&datalen,msg,json)) > 0 )
        {
            argjson = cJSON_GetArrayItem(json,0);
            copy_cJSON(src,cJSON_GetObjectItem(argjson,"NXT"));
            copy_cJSON(key,cJSON_GetObjectItem(argjson,"key"));
            copy_cJSON(usedest,cJSON_GetObjectItem(cJSON_GetArrayItem(json,1),"usedest"));
            if ( usedest[0] != 0 )
                retstr = busdata_deref(forwarder,sender,valid,(char *)databuf,json);
            else retstr = busdata(forwarder,sender,valid,key,timestamp,databuf,datalen,json);
            //printf("valid.%d forwarder.(%s) NXT.%-24s key.(%s) datalen.%d\n",valid,forwarder,src,key,datalen);
        } else retstr = clonestr("{\"error\":\"busdata doesnt validate\"}");
        free_json(json);
    } else retstr = clonestr("{\"error\":\"couldnt parse busdata\"}");
   // printf("BUSDATA.(%s) (%s)\n",msg,retstr);
    return(retstr);
}

char *create_busdata(int32_t *datalenp,char *jsonstr,char *broadcastmode)
{
    char key[MAX_JSON_FIELD],method[MAX_JSON_FIELD],plugin[MAX_JSON_FIELD],endpoint[128],hexstr[65],numstr[65],*str,*str2,*tokbuf = 0,*tmp;
    bits256 hash; uint64_t nxt64bits,tag; uint32_t timestamp; cJSON *datajson,*json; int32_t tlen,datalen = 0;
    *datalenp = 0;
    //printf("create_busdata\n");
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        if ( is_cJSON_Array(json) != 0 && cJSON_GetArraySize(json) == 2 )
        {
            *datalenp = (int32_t)strlen(jsonstr) + 1;
            free_json(json);
            return(jsonstr);
        }
        if ( broadcastmode != 0 && broadcastmode[0] != 0 )
        {
            copy_cJSON(method,cJSON_GetObjectItem(json,"method"));
            copy_cJSON(plugin,cJSON_GetObjectItem(json,"plugin"));
            cJSON_ReplaceItemInObject(json,"method",cJSON_CreateString("busdata"));
            cJSON_ReplaceItemInObject(json,"plugin",cJSON_CreateString("relay"));
            cJSON_AddItemToObject(json,"submethod",cJSON_CreateString(method));
            if ( strcmp(plugin,"relay") != 0 )
                cJSON_AddItemToObject(json,"destplugin",cJSON_CreateString(plugin));
        }
        else
        {
            sprintf(endpoint,"%s://%s:%u",SUPERNET.transport,SUPERNET.myipaddr,SUPERNET.port - 2);
            cJSON_AddItemToObject(json,"endpoint",cJSON_CreateString(endpoint));
        }
        randombytes((uint8_t *)&tag,sizeof(tag));
        sprintf(numstr,"%llu",(long long)tag), cJSON_AddItemToObject(json,"tag",cJSON_CreateString(numstr));
        timestamp = (uint32_t)time(NULL);
        copy_cJSON(key,cJSON_GetObjectItem(json,"key"));
        nxt64bits = conv_acctstr(SUPERNET.NXTADDR);
        datajson = cJSON_CreateObject();
        cJSON_AddItemToObject(datajson,"method",cJSON_CreateString("busdata"));
        cJSON_AddItemToObject(datajson,"key",cJSON_CreateString(key));
        cJSON_AddItemToObject(datajson,"time",cJSON_CreateNumber(timestamp));
        sprintf(numstr,"%llu",(long long)nxt64bits), cJSON_AddItemToObject(datajson,"NXT",cJSON_CreateString(numstr));
        str = cJSON_Print(json), _stripwhite(str,' ');
        datalen = (int32_t)(strlen(str) + 1);
        tmp = malloc((datalen << 1) + 1);
        init_hexbytes_noT(tmp,(uint8_t *)str,datalen);
        cJSON_AddItemToObject(datajson,"data",cJSON_CreateString(tmp));
        calc_sha256(hexstr,hash.bytes,(uint8_t *)str,datalen);
        cJSON_AddItemToObject(datajson,"n",cJSON_CreateNumber(datalen));
        cJSON_AddItemToObject(datajson,"H",cJSON_CreateString(hexstr));
        str2 = cJSON_Print(datajson), _stripwhite(str2,' ');
        tokbuf = calloc(1,strlen(str2) + 1024);
        tlen = construct_tokenized_req(tokbuf,str2,SUPERNET.NXTACCTSECRET,broadcastmode);
//printf("created busdata.(%s) -> (%s) tlen.%d\n",str,tokbuf,tlen);
        free(tmp), free(str), free(str2), str = str2 = 0;
        *datalenp = tlen;
        if ( SUPERNET.iamrelay != 0 && (str= nn_busdata_processor((uint8_t *)tokbuf,tlen)) != 0 )
            free(str);
        free_json(json);
    } else printf("couldnt parse busdata json.(%s)\n",jsonstr);
    return(tokbuf);
}

char *busdata_sync(char *jsonstr,char *broadcastmode)
{
    int32_t datalen,sendlen = 0; char *data,*retstr;
    //printf("busdata_sync.(%s) (%s)\n",jsonstr,broadcastmode==0?"":broadcastmode);
    if ( (data= create_busdata(&datalen,jsonstr,broadcastmode)) != 0 )
    {
        if ( SUPERNET.iamrelay != 0 )
        {
            if ( RELAYS.bus.sock >= 0 )
            {
                if( (sendlen= nn_send(RELAYS.bus.sock,data,datalen,0)) != datalen )
                {
                    if ( Debuglevel > 1 )
                        printf("sendlen.%d vs datalen.%d (%s) %s\n",sendlen,datalen,(char *)data,nn_errstr());
                    free(data);
                    return(clonestr("{\"error\":\"couldnt send to bus\"}"));
                } else printf("PUB.(%s)\n",data);
            }
            if ( data != jsonstr )
                free(data);
            return(clonestr("{\"result\":\"sent to bus\"}"));
        }
        else
        {
            retstr = nn_loadbalanced((uint8_t *)data,datalen);
            //if ( retstr != 0 )
            //    printf("busdata nn_loadbalanced retstr.(%s)\n",retstr);
            if ( data != jsonstr )
                free(data);
            return(retstr);
        }
    }
    return(clonestr("{\"error\":\"error creating busdata\"}"));
}

void busdata_init(int32_t sendtimeout,int32_t recvtimeout)
{
    char endpoint[512]; int32_t iter,sock,type,portoffset,retrymillis,maxmillis;
    type = NN_REP, portoffset = -2;
    for (iter=0; iter<1+SUPERNET.iamrelay; iter++)
    {
        if ( (sock= nn_socket(AF_SP,type)) >= 0 ) // NN_BUS seems to have 4x redundant packets
        {
            if ( iter == 0 )
                RELAYS.servicesock = sock;
            else RELAYS.bus.sock = sock;
            expand_epbits(endpoint,calc_epbits(SUPERNET.transport,(uint32_t)calc_ipbits(SUPERNET.myipaddr),SUPERNET.port + portoffset,type));
            nn_bind(sock,endpoint);
            printf("SERVICE BIND.(%s)\n",endpoint);
            maxmillis = 1000, retrymillis = 25;
            if ( sendtimeout > 0 && nn_setsockopt(sock,NN_SOL_SOCKET,NN_SNDTIMEO,&sendtimeout,sizeof(sendtimeout)) < 0 )
                fprintf(stderr,"error setting sendtimeout %s\n",nn_errstr());
            else if ( recvtimeout > 0 && nn_setsockopt(sock,NN_SOL_SOCKET,NN_RCVTIMEO,&recvtimeout,sizeof(recvtimeout)) < 0 )
                fprintf(stderr,"error setting sendtimeout %s\n",nn_errstr());
            else if ( nn_setsockopt(sock,NN_SOL_SOCKET,NN_RECONNECT_IVL,&retrymillis,sizeof(retrymillis)) < 0 )
                fprintf(stderr,"error setting NN_REQ NN_RECONNECT_IVL_MAX socket %s\n",nn_errstr());
            else if ( nn_setsockopt(sock,NN_SOL_SOCKET,NN_RECONNECT_IVL_MAX,&maxmillis,sizeof(maxmillis)) < 0 )
                fprintf(stderr,"error setting NN_REQ NN_RECONNECT_IVL_MAX socket %s\n",nn_errstr());
        }
        type = NN_PUB;
        portoffset = nn_portoffset(NN_BUS);
    }
}

void busdata_poll()
{
    char *str,*jsonstr; cJSON *json; int32_t len,sock;
    sock = RELAYS.servicesock;
    if ( sock >= 0 && (len= nn_recv(sock,&jsonstr,NN_MSG,0)) > 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (str= nn_busdata_processor((uint8_t *)jsonstr,len)) != 0 )
            {
                nn_send(sock,str,(int32_t)strlen(str)+1,0);
                free(str);
            }
            free_json(json);
        }
        printf("SERVICESOCK recv.%d (%s)\n",len,jsonstr);
        nn_freemsg(jsonstr);
    }
}

