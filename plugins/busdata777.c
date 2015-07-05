//
//  relays777.c
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//
// sync relays
// and then also to make sure adding relays on the fly syncs up to the current set of serviceproviders
// encryption
// ipv6
// btc38

// "servicesecret" in SuperNET.conf
// register: ./BitcoinDarkd SuperNET '{"plugin":"relay","method":"busdata","destplugin":"relay","submethod":"serviceprovider","servicename":"echo","endpoint":""}'
// ./BitcoinDarkd SuperNET '{"method":"busdata","plugin":"relay","servicename":"echo","serviceNXT":"4273301882745002507","destplugin":"echodemo","submethod":"echo","echostr":"remote echo"}'

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

int32_t issue_generateToken(char encoded[NXT_TOKEN_LEN],char *key,char *origsecret)
{
    char cmd[16384],secret[8192],token[MAX_JSON_FIELD+2*NXT_TOKEN_LEN+1],*jsontxt; cJSON *tokenobj,*json;
    encoded[0] = 0;
    escape_code(secret,origsecret);
    sprintf(cmd,"requestType=generateToken&website=%s&secretPhrase=%s",key,secret);
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
        return(0);
    }
    return(-1);
}

uint32_t calc_nonce(char *str,int32_t leverage,int32_t maxmillis,uint32_t nonce)
{
    uint64_t hit,threshold; bits384 sig; double endmilli; int32_t len;
    len = (int32_t)strlen(str);
    if ( leverage != 0 )
    {
        threshold = calc_SaMthreshold(leverage);
        if ( maxmillis == 0 )
        {
            if ( (hit= calc_SaM(&sig,(void *)str,len,(void *)&nonce,sizeof(nonce))) >= threshold )
            {
                printf("nonce failure hit.%llu >= threshold.%llu\n",(long long)hit,(long long)threshold);
                if ( (threshold - hit) > ((uint64_t)1L << 32) )
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
                if ( (hit= calc_SaM(&sig,(void *)str,len,(void *)&nonce,sizeof(nonce))) < threshold )
                    return(nonce);
            }
        }
    }
    return(0);
}

int32_t nonce_leverage(char *broadcaststr)
{
    int32_t leverage = 4;
    if ( broadcaststr != 0 && broadcaststr[0] != 0 )
    {
        if ( strcmp(broadcaststr,"allnodes") == 0 )
            leverage = 7;
        else if ( strcmp(broadcaststr,"join") == 0 )
            leverage = 9;
        else if ( strcmp(broadcaststr,"servicerequest") == 0 )
            leverage = 6;
        else if ( strcmp(broadcaststr,"allrelays") == 0 )
            leverage = 5;
        else if ( atoi(broadcaststr) != 0 )
            leverage = atoi(broadcaststr);
    }
    return(leverage);
}

char *get_broadcastmode(cJSON *json,char *broadcastmode)
{
    char servicename[MAX_JSON_FIELD],*bstr;
    copy_cJSON(servicename,cJSON_GetObjectItem(json,"servicename"));
    if ( servicename[0] != 0 )
        broadcastmode = "servicerequest";
    else if ( (bstr= cJSON_str(cJSON_GetObjectItem(json,"broadcast"))) != 0 )
        return(bstr);
    //printf("(%s) get_broadcastmode.(%s) servicename.[%s]\n",cJSON_Print(json),broadcastmode!=0?broadcastmode:"",servicename);
    return(broadcastmode);
}

uint32_t nonce_func(int32_t *leveragep,char *str,char *broadcaststr,int32_t maxmillis,uint32_t nonce)
{
    int32_t leverage = nonce_leverage(broadcaststr);
    if ( maxmillis == 0 && *leveragep != leverage )
        return(0xffffffff);
    *leveragep = leverage;
    return(calc_nonce(str,leverage,maxmillis,nonce));
}

int32_t construct_tokenized_req(char *tokenized,char *cmdjson,char *NXTACCTSECRET,char *broadcastmode)
{
    char encoded[2*NXT_TOKEN_LEN+1],broadcaststr[512]; uint32_t nonce,nonceerr; int32_t i,leverage,n = 100;
    if ( broadcastmode == 0 )
        broadcastmode = "";
    _stripwhite(cmdjson,' ');
    for (i=0; i<n; i++)
    {
        if ( (nonce= nonce_func(&leverage,cmdjson,broadcastmode,5000,0)) != 0 )
            break;
        printf("iter.%d of %d couldnt find nonce, try again\n",i,n);
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
    char buf[MAX_JSON_FIELD],serviceNXT[MAX_JSON_FIELD],sender[MAX_JSON_FIELD],broadcaststr[MAX_JSON_FIELD],*broadcastmode,*firstjsontxt = 0;
    unsigned char encoded[4096];
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
        obj = cJSON_GetObjectItem(firstitem,"serviceNXT"), copy_cJSON(serviceNXT,obj);
        if ( NXTaddr[0] != 0 && strcmp(buf,NXTaddr) != 0 )
            retcode = -3;
        else
        {
            strcpy(NXTaddr,buf);
//printf("decoded.(%s)\n",NXTaddr);
            if ( strictflag != 0 )
            {
                timeval = get_cJSON_int(firstitem,"time");
                diff = (timeval - time(NULL));
                if ( diff < -60 )
                    retcode = -6;
                else if ( diff > strictflag )
                {
                    printf("time diff %lld too big %lld vs %ld\n",(long long)diff,(long long)timeval,time(NULL));
                    retcode = -5;
                }
            }
            if ( retcode != -5 && retcode != -6 )
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
                    nonce = (uint32_t)get_API_int(cJSON_GetObjectItem(tokenobj,"nonce"),0);
                    leverage = (uint32_t)get_API_int(cJSON_GetObjectItem(tokenobj,"leverage"),0);
                    copy_cJSON(broadcaststr,cJSON_GetObjectItem(tokenobj,"broadcast"));
                    broadcastmode = get_broadcastmode(firstitem,broadcaststr);
                    retcode = valid;
                    if ( nonce_func(&leverage,firstjsontxt,broadcastmode,0,nonce) != 0 )
                    {
                        //printf("(%s) -> (%s) leverage.%d len.%d crc.%u\n",broadcaststr,firstjsontxt,leverage,len,_crc32(0,(void *)firstjsontxt,len));
                        retcode = -4;
                    }
                    if ( Debuglevel > 2 )
                        printf("signed by valid NXT.%s valid.%d diff.%lld forwarder.(%s)\n",sender,valid,(long long)diff,forwarder);
                    if ( strcmp(sender,NXTaddr) != 0 && strcmp(sender,serviceNXT) != 0 )
                    {
                        printf("valid.%d diff sender.(%s) vs NXTaddr.(%s) serviceNXT.(%s)\n",valid,sender,NXTaddr,serviceNXT);
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
    if ( retcode < 0 )
        printf("ret.%d signed by valid NXT.%s valid.%d diff.%lld forwarder.(%s)\n",retcode,sender,valid,(long long)diff,forwarder);
    return(retcode);
}

void nn_syncbus(cJSON *json)
{
    cJSON *argjson,*second; char forwarder[MAX_JSON_FIELD],*jsonstr; uint64_t forwardbits,nxt64bits;
    //printf("pubsock.%d iamrelay.%d arraysize.%d\n",RELAYS.pubsock,SUPERNET.iamrelay,cJSON_GetArraySize(json));
    if ( RELAYS.pubrelays >= 0 && SUPERNET.iamrelay != 0 && is_cJSON_Array(json) != 0 && cJSON_GetArraySize(json) == 2 )
    {
        argjson = cJSON_GetArrayItem(json,0);
        second = cJSON_GetArrayItem(json,1);
        copy_cJSON(forwarder,cJSON_GetObjectItem(second,"forwarder"));
        ensure_jsonitem(second,"forwarder",SUPERNET.NXTADDR);
        jsonstr = cJSON_Print(json), _stripwhite(jsonstr,' ');
        forwardbits = conv_acctstr(forwarder), nxt64bits = conv_acctstr(SUPERNET.NXTADDR);
        if ( forwardbits == 0 )//|| forwardbits == nxt64bits )
        {
            if ( Debuglevel > 2 )
                printf("BUS-SEND.(%s) forwarder.%llu vs %llu\n",jsonstr,(long long)forwardbits,(long long)nxt64bits);
            nn_send(RELAYS.pubrelays,jsonstr,(int32_t)strlen(jsonstr)+1,0);
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
struct service_provider { UT_hash_handle hh; int32_t sock; } *Service_providers;
struct serviceprovider { uint64_t servicebits; char name[32],endpoint[64]; };

void free_busdata_item(struct busdata_item *ptr)
{
    if ( ptr->json != 0 )
        free_json(ptr->json);
    if ( ptr->retstr != 0 )
        free(ptr->retstr);
    if ( ptr->key != 0 )
        free(ptr->key);
    free(ptr);
}

char *lb_serviceprovider(struct service_provider *sp,uint8_t *data,int32_t datalen)
{
    int32_t i,sendlen,recvlen; char *msg,*jsonstr = 0;
    for (i=0; i<10; i++)
        if ( (nn_socket_status(sp->sock,1) & NN_POLLOUT) != 0 )
            break;
    if ( Debuglevel > 2 )
        printf("lb_serviceprovider.(%s)\n",data);
    if ( (sendlen= nn_send(sp->sock,data,datalen,0)) == datalen )
    {
        for (i=0; i<10; i++)
            if ( (nn_socket_status(sp->sock,1) & NN_POLLIN) != 0 )
                break;
        if ( (recvlen= nn_recv(sp->sock,&msg,NN_MSG,0)) > 0 )
        {
            printf("servicerecv.(%s)\n",msg);
            jsonstr = clonestr((char *)msg);
            nn_freemsg(msg);
        } else printf("lb_serviceprovider timeout\n");
    } else printf("sendlen.%d != datalen.%d\n",sendlen,datalen);
    return(jsonstr);
}

cJSON *serviceprovider_json()
{
    struct serviceprovider **sps; int32_t i,num; cJSON *json,*item,*array; char numstr[64];
    json = cJSON_CreateObject();
    array = cJSON_CreateArray();
    if ( (sps= (struct serviceprovider **)db777_copy_all(&num,DB_services,"key",0)) != 0 )
    {
        for (i=0; i<num; i++)
        {
            if ( sps[i] != 0 )
            {
                item = cJSON_CreateObject();
                cJSON_AddItemToObject(item,sps[i]->name,cJSON_CreateString(sps[i]->endpoint));
                sprintf(numstr,"%llu",(long long)sps[i]->servicebits), cJSON_AddItemToObject(item,"serviceNXT",cJSON_CreateString(numstr));
                free(sps[i]);
                cJSON_AddItemToArray(array,item);
            }
        }
        free(sps);
    }
    cJSON_AddItemToObject(json,"services",array);
    return(json);
}

uint32_t find_serviceprovider(struct serviceprovider *S)
{
    void *obj,*result,*value; int32_t len; uint32_t timestamp = 0;
    if ( (obj= sp_object(DB_services->db)) != 0 )
    {
        if ( sp_set(obj,"key",S,sizeof(*S)) == 0 && (result= sp_get(DB_services->db,obj)) != 0 )
        {
            value = sp_get(result,"value",&len);
            memcpy(&timestamp,value,len);
            sp_destroy(result);
        }
    }
    return(timestamp);
}

int32_t remove_service_provider(char *serviceNXT,char *servicename,char *endpoint)
{
    void *obj; int32_t retval; struct serviceprovider S;
    memset(&S,0,sizeof(S));
    S.servicebits = conv_acctstr(serviceNXT);
    strncpy(S.name,servicename,sizeof(S.name)-1);
    S.endpoint[sizeof(S.endpoint)-1] = 0;
    strncpy(S.endpoint,endpoint,sizeof(S.endpoint)-1);
    if ( find_serviceprovider(&S) != 0 && (obj= sp_object(DB_services->db)) != 0 )
    {
        if ( sp_set(obj,"key",&S,sizeof(S)) == 0 )
        {
            printf("DELETE SERVICEPROVIDER.(%s) (%s) serviceNXT.(%s) (%s)\n",servicename,endpoint,serviceNXT,cJSON_Print(serviceprovider_json()));
            retval = sp_delete(DB_services->db,obj);
            printf("after delete retval.%d (%s)\n",retval,cJSON_Print(serviceprovider_json()));
        }
    }
    return(-1);
}

int32_t add_serviceprovider(struct serviceprovider *S,uint32_t timestamp)
{
    void *obj;
    if ( (obj= sp_object(DB_services->db)) != 0 )
    {
        if ( sp_set(obj,"key",S,sizeof(*S)) == 0 && sp_set(obj,"value",&timestamp,sizeof(timestamp)) == 0 )
            return(sp_set(DB_services->db,obj));
        else
        {
            sp_destroy(obj);
            printf("error add_serviceprovider %s\n",db777_errstr(DB_services->ctl));
        }
    }
    return(-1);
}

int32_t add_service_provider(char *serviceNXT,char *servicename,char *endpoint)
{
    struct serviceprovider S;
    memset(&S,0,sizeof(S));
    S.servicebits = conv_acctstr(serviceNXT);
    strncpy(S.name,servicename,sizeof(S.name)-1);
    S.endpoint[sizeof(S.endpoint)-1] = 0;
    strncpy(S.endpoint,endpoint,sizeof(S.endpoint)-1);
    if ( find_serviceprovider(&S) == 0 )
        add_serviceprovider(&S,(uint32_t)time(NULL));
    return(0);
}

struct service_provider *find_servicesock(char *servicename,char *endpoint)
{
    struct service_provider *sp,*checksp; struct serviceprovider **sps; int32_t i,num,sendtimeout,recvtimeout,retrymillis,maxmillis;
    HASH_FIND(hh,Service_providers,servicename,strlen(servicename),sp);
    if ( sp == 0 )
    {
        printf("Couldnt find service.(%s)\n",servicename);
        sp = calloc(1,sizeof(*sp));
        HASH_ADD_KEYPTR(hh,Service_providers,servicename,strlen(servicename),sp);
        sp->hh.key = clonestr(servicename);
        HASH_FIND(hh,Service_providers,servicename,strlen(servicename),checksp);
        if ( checksp != sp )
        {
            printf("checksp.%p != %p\n",checksp,sp);
        }
        sp->sock = nn_socket(AF_SP,NN_REQ);
        sendtimeout = 1000, recvtimeout = 10000, maxmillis = 3000, retrymillis = 100;
        if ( sendtimeout > 0 && nn_setsockopt(sp->sock,NN_SOL_SOCKET,NN_SNDTIMEO,&sendtimeout,sizeof(sendtimeout)) < 0 )
            fprintf(stderr,"error setting sendtimeout %s\n",nn_errstr());
        else if ( recvtimeout > 0 && nn_setsockopt(sp->sock,NN_SOL_SOCKET,NN_RCVTIMEO,&recvtimeout,sizeof(recvtimeout)) < 0 )
            fprintf(stderr,"error setting sendtimeout %s\n",nn_errstr());
        else if ( nn_setsockopt(sp->sock,NN_SOL_SOCKET,NN_RECONNECT_IVL,&retrymillis,sizeof(retrymillis)) < 0 )
            fprintf(stderr,"error setting NN_REQ NN_RECONNECT_IVL_MAX socket %s\n",nn_errstr());
        else if ( nn_setsockopt(sp->sock,NN_SOL_SOCKET,NN_RECONNECT_IVL_MAX,&maxmillis,sizeof(maxmillis)) < 0 )
            fprintf(stderr,"error setting NN_REQ NN_RECONNECT_IVL_MAX socket %s\n",nn_errstr());
        // scan DB and nn_connect
        if ( (sps= (struct serviceprovider **)db777_copy_all(&num,DB_services,"key",0)) != 0 )
        {
            for (i=0; i<num; i++)
            {
                if ( sps[i] != 0 )
                {
                    if ( strcmp(servicename,sps[i]->name) == 0 )
                    {
                        nn_connect(sp->sock,sps[i]->endpoint), printf("SERVICEPROVIDER CONNECT ");
                        if ( endpoint != 0 && strcmp(sps[i]->endpoint,endpoint) == 0 )
                            endpoint = 0;
                    }
                    printf("%24llu %16s %s\n",(long long)sps[i]->servicebits,sps[i]->name,sps[i]->endpoint);
                    free(sps[i]);
                }
            }
            free(sps);
        }
    } // else printf("sp.%p found servicename.(%s) sock.%d\n",sp,servicename,sp->sock);
    if ( endpoint != 0 )
    {
        fprintf(stderr,"create servicename.(%s) sock.%d <-> (%s)\n",servicename,sp->sock,endpoint);
        nn_connect(sp->sock,endpoint);
    }
    return(sp);
}

char *busdata_addpending(char *destNXT,char *sender,char *key,uint32_t timestamp,cJSON *json,char *forwarder,cJSON *origjson)
{
    cJSON *argjson; struct busdata_item *ptr; bits256 hash; struct service_provider *sp; int32_t valid;
    char submethod[512],servicecmd[512],endpoint[512],destplugin[512],retbuf[128],serviceNXT[128],servicename[512],servicetoken[512],*hashstr,*str,*retstr;
    if ( key == 0 || key[0] == 0 )
        key = "0";
     if ( (hashstr= cJSON_str(cJSON_GetObjectItem(json,"H"))) != 0 )
        decode_hex(hash.bytes,sizeof(hash),hashstr);
    else memset(hash.bytes,0,sizeof(hash));
    copy_cJSON(submethod,cJSON_GetObjectItem(json,"submethod"));
    copy_cJSON(destplugin,cJSON_GetObjectItem(json,"destplugin"));
    copy_cJSON(servicename,cJSON_GetObjectItem(json,"servicename"));
    copy_cJSON(servicecmd,cJSON_GetObjectItem(json,"servicecmd"));
    //printf("addpending.(%s %s).%s\n",destplugin,servicename,submethod);
    if ( strcmp(submethod,"serviceprovider") == 0 )
    {
        copy_cJSON(endpoint,cJSON_GetObjectItem(json,"endpoint"));
        copy_cJSON(servicetoken,cJSON_GetObjectItem(json,"servicetoken"));
        if ( issue_decodeToken(serviceNXT,&valid,endpoint,(void *)servicetoken) > 0 )
            printf("valid.(%s) from serviceNXT.%s\n",endpoint,serviceNXT);
        if ( strcmp(servicecmd,"remove") == 0 )
        {
            remove_service_provider(serviceNXT,servicename,endpoint);
            sprintf(retbuf,"{\"result\":\"serviceprovider endpoint removed\",\"endpoint\":\"%s\",\"serviceNXT\":\"%s\"}",endpoint,serviceNXT);
        }
        else
        {
            if ( add_service_provider(serviceNXT,servicename,endpoint) == 0 )
                find_servicesock(servicename,endpoint);
            else find_servicesock(servicename,0);
            find_servicesock(servicename,0);
            sprintf(retbuf,"{\"result\":\"serviceprovider added\",\"endpoint\":\"%s\",\"serviceNXT\":\"%s\"}",endpoint,serviceNXT);
        }
        nn_syncbus(origjson);
        return(clonestr(retbuf));
    }
    else
    {
        copy_cJSON(serviceNXT,cJSON_GetObjectItem(json,"serviceNXT"));
        printf("service.%s (%s) serviceNXT.%s\n",servicename,submethod,serviceNXT);
        if ( (sp= find_servicesock(servicename,0)) == 0 )
            return(clonestr("{\"result\":\"serviceprovider not found\"}"));
        else
        {
            //HASH_FIND(hh,Service_providers,servicename,strlen(servicename),sp);
            argjson = cJSON_Duplicate(origjson,1);
            ensure_jsonitem(cJSON_GetArrayItem(argjson,1),"usedest","yes");
            str = cJSON_Print(argjson), _stripwhite(str,' ');
            free_json(argjson);
            if ( (retstr= lb_serviceprovider(sp,(uint8_t *)str,(int32_t)strlen(str)+1)) != 0 )
            {
                free(str);
                if ( Debuglevel > 2 )
                    printf("LBS.(%s)\n",retstr);
                return(retstr);
            }
            free(str);
            return(clonestr("{\"result\":\"no response from provider\"}"));
        }
    }
    ptr = calloc(1,sizeof(*ptr));
    ptr->json = cJSON_Duplicate(json,1), ptr->queuetime = (uint32_t)time(NULL), ptr->key = clonestr(key);
    ptr->dest64bits = conv_acctstr(destNXT), ptr->senderbits = conv_acctstr(sender);
    if ( (hashstr= cJSON_str(cJSON_GetObjectItem(json,"H"))) != 0 )
        decode_hex(ptr->hash.bytes,sizeof(ptr->hash),hashstr);
    else memset(ptr->hash.bytes,0,sizeof(ptr->hash));
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
    if ( Debuglevel > 2 )
        printf("busdata.(%s) valid.%d -> (%s)\n",msg,valid,retstr!=0?retstr:"");
    return(retstr);
}

int32_t busdata_validate(char *forwarder,char *sender,uint32_t *timestamp,uint8_t *databuf,int32_t *datalenp,void *msg,cJSON *json)
{
    char pubkey[256],hexstr[65],sha[65],datastr[8192]; int32_t valid; cJSON *argjson; bits256 hash;
    *timestamp = *datalenp = 0;
    forwarder[0] = sender[0] = 0;
    //printf("busdata_validate.(%s)\n",msg);
    if ( is_cJSON_Array(json) != 0 && cJSON_GetArraySize(json) == 2 )
    {
        argjson = cJSON_GetArrayItem(json,0);
        *timestamp = (uint32_t)get_API_int(cJSON_GetObjectItem(argjson,"time"),0);
        if ( (valid= validate_token(forwarder,pubkey,sender,msg,(*timestamp != 0) * MAXTIMEDIFF)) <= 0 )
        {
            fprintf(stderr,"error valid.%d sender.(%s) forwarder.(%s)\n",valid,sender,forwarder);
            return(valid);
        }
        copy_cJSON(datastr,cJSON_GetObjectItem(argjson,"data"));
        if ( strcmp(sender,SUPERNET.NXTADDR) != 0 || datastr[0] != 0 )
        {
            copy_cJSON(sha,cJSON_GetObjectItem(argjson,"H"));
            if ( datastr[0] != 0 )
                decode_hex(databuf,(int32_t)(strlen(datastr)+1)>>1,datastr);
            else databuf[0] = 0;
            *datalenp = (uint32_t)get_API_int(cJSON_GetObjectItem(argjson,"n"),0);
            calc_sha256(hexstr,hash.bytes,databuf,*datalenp);
            if ( strcmp(hexstr,sha) == 0 )
                return(1);
            else printf("hash mismatch %s vs %s\n",hexstr,sha);
        }
        else
        {
            strcpy((char *)databuf,msg);
            *datalenp = (int32_t)strlen((char *)databuf) + 1;
            return(1);
        }
    } else printf("busdata_validate not array (%s)\n",msg);
    return(-1);
}

char *busdata_deref(char *forwarder,char *sender,int32_t valid,char *databuf,cJSON *json)
{
    char plugin[MAX_JSON_FIELD],method[MAX_JSON_FIELD],buf[MAX_JSON_FIELD],servicename[MAX_JSON_FIELD],*broadcaststr,*str,*retstr = 0;
    cJSON *dupjson,*second,*argjson,*origjson; uint64_t forwardbits;
    if ( SUPERNET.iamrelay != 0 && (broadcaststr= cJSON_str(cJSON_GetObjectItem(cJSON_GetArrayItem(json,1),"broadcast"))) != 0 )
    {
        dupjson = cJSON_Duplicate(json,1);
        second = cJSON_GetArrayItem(dupjson,1);
        ensure_jsonitem(second,"forwarder",SUPERNET.NXTADDR);
        if ( (forwardbits= conv_acctstr(forwarder)) == 0 && cJSON_GetObjectItem(second,"stop") == 0 )
        {
            ensure_jsonitem(second,"stop","end");
            str = cJSON_Print(dupjson), _stripwhite(str,' ');
            if ( strcmp(broadcaststr,"allrelays") == 0 || strcmp(broadcaststr,"join") == 0 )
            {
                printf("[%s] broadcast.(%s) forwarder.%llu vs %s\n",broadcaststr,str,(long long)forwardbits,SUPERNET.NXTADDR);
                nn_send(RELAYS.pubrelays,str,(int32_t)strlen(str)+1,0);
            }
            else if ( strcmp(broadcaststr,"allnodes") == 0 )
            {
                printf("[%s] broadcast.(%s) forwarder.%llu vs %s\n",broadcaststr,str,(long long)forwardbits,SUPERNET.NXTADDR);
                nn_send(RELAYS.pubglobal,str,(int32_t)strlen(str)+1,0);
            }
            free(str);
        } // else printf("forwardbits.%llu stop.%p\n",(long long)forwardbits,cJSON_GetObjectItem(second,"stop"));
        free_json(dupjson);
    }
    if ( (origjson= cJSON_Parse(databuf)) != 0 )
    {
        if ( is_cJSON_Array(origjson) != 0 && cJSON_GetArraySize(origjson) == 2 )
        {
            argjson = cJSON_GetArrayItem(origjson,0);
            copy_cJSON(buf,cJSON_GetObjectItem(argjson,"NXT"));
            if ( strcmp(buf,SUPERNET.NXTADDR) != 0 )
            {
                printf("tokenized json not local.(%s)\n",databuf);
                free_json(origjson);
                return(clonestr("{\"error\":\"tokenized json not local\"}"));
            }
        }
        else argjson = origjson;
        copy_cJSON(plugin,cJSON_GetObjectItem(argjson,"destplugin"));
        copy_cJSON(method,cJSON_GetObjectItem(argjson,"submethod"));
        copy_cJSON(buf,cJSON_GetObjectItem(argjson,"method"));
        copy_cJSON(servicename,cJSON_GetObjectItem(argjson,"servicename"));
        if ( Debuglevel > 2 )
            printf("relay.%d buf.(%s) method.(%s) servicename.(%s)\n",SUPERNET.iamrelay,buf,method,servicename);
        if ( SUPERNET.iamrelay != 0 && ((strcmp(buf,"busdata") == 0 && strcmp(method,"serviceprovider") == 0) || servicename[0] != 0) ) //
        {
// printf("bypass deref\n");
            free_json(origjson);
            return(0);
        }
        cJSON_ReplaceItemInObject(argjson,"method",cJSON_CreateString(method));
        cJSON_ReplaceItemInObject(argjson,"plugin",cJSON_CreateString(plugin));
        cJSON_DeleteItemFromObject(argjson,"submethod");
        cJSON_DeleteItemFromObject(argjson,"destplugin");
        str = cJSON_Print(argjson), _stripwhite(str,' ');
        if ( Debuglevel > 2 )
            printf("call (%s %s) (%s)\n",plugin,method,str);
        retstr = plugin_method(-1,0,0,plugin,method,0,0,str,(int32_t)strlen(str)+1,SUPERNET.PLUGINTIMEOUT/2);
        free_json(origjson);
        free(str);
    }
    return(retstr);
}

char *nn_busdata_processor(uint8_t *msg,int32_t len)
{
    cJSON *json,*argjson; uint32_t timestamp; int32_t datalen,valid; uint8_t databuf[8192];
    char usedest[128],key[MAX_JSON_FIELD],src[MAX_JSON_FIELD],forwarder[MAX_JSON_FIELD],sender[MAX_JSON_FIELD],*retstr = 0;
    if ( Debuglevel > 2 )
        fprintf(stderr,"nn_busdata_processor.(%s)\n",msg);
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
            if ( retstr == 0 )
                retstr = busdata(forwarder,sender,valid,key,timestamp,databuf,datalen,json);
//printf("valid.%d forwarder.(%s) NXT.%-24s key.(%s) datalen.%d\n",valid,forwarder,src,key,datalen);
        } else retstr = clonestr("{\"error\":\"busdata doesnt validate\"}");
        free_json(json);
    } else retstr = clonestr("{\"error\":\"couldnt parse busdata\"}");
    if ( Debuglevel > 2 )
        fprintf(stderr,"BUSDATA.(%s) -> %p.(%s)\n",msg,retstr,retstr);
    return(retstr);
}

char *create_busdata(int32_t *datalenp,char *jsonstr,char *broadcastmode)
{
    char key[MAX_JSON_FIELD],method[MAX_JSON_FIELD],plugin[MAX_JSON_FIELD],servicetoken[NXT_TOKEN_LEN+1],endpoint[128],hexstr[65],numstr[65];
    char *str,*str2,*tokbuf = 0,*tmp,*secret;
    bits256 hash; uint64_t nxt64bits,tag; uint16_t port; uint32_t timestamp; cJSON *datajson,*json; int32_t tlen,diff,datalen = 0;
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
        broadcastmode = get_broadcastmode(json,broadcastmode);
        if ( broadcastmode != 0 && strcmp(broadcastmode,"join") == 0 )
            diff = 60, port = SUPERNET.port + LB_OFFSET;
        else diff = 0, port = SUPERNET.serviceport;
        copy_cJSON(method,cJSON_GetObjectItem(json,"method"));
        copy_cJSON(plugin,cJSON_GetObjectItem(json,"plugin"));
        secret = SUPERNET.NXTACCTSECRET;
        if ( cJSON_GetObjectItem(json,"endpoint") != 0 )
        {
            sprintf(endpoint,"%s://%s:%u",SUPERNET.transport,SUPERNET.myipaddr,port);
            cJSON_ReplaceItemInObject(json,"endpoint",cJSON_CreateString(endpoint));
            if ( SUPERNET.SERVICESECRET[0] != 0 && issue_generateToken(servicetoken,endpoint,SUPERNET.SERVICESECRET) == 0 )
            {
                cJSON_AddItemToObject(json,"servicetoken",cJSON_CreateString(servicetoken));
                secret = SUPERNET.SERVICESECRET;
            }
        }
        if ( broadcastmode != 0 && broadcastmode[0] != 0 )
        {
            cJSON_ReplaceItemInObject(json,"method",cJSON_CreateString("busdata"));
            cJSON_ReplaceItemInObject(json,"plugin",cJSON_CreateString("relay"));
            cJSON_AddItemToObject(json,"submethod",cJSON_CreateString(method));
            //if ( strcmp(plugin,"relay") != 0 )
                cJSON_AddItemToObject(json,"destplugin",cJSON_CreateString(plugin));
        }
        randombytes((uint8_t *)&tag,sizeof(tag));
        sprintf(numstr,"%llu",(long long)tag), cJSON_AddItemToObject(json,"tag",cJSON_CreateString(numstr));
        timestamp = (uint32_t)time(NULL);
        copy_cJSON(key,cJSON_GetObjectItem(json,"key"));
        nxt64bits = conv_acctstr(SUPERNET.NXTADDR);
        datajson = cJSON_CreateObject();
        cJSON_AddItemToObject(datajson,"plugin",cJSON_CreateString("relay"));
        cJSON_AddItemToObject(datajson,"method",cJSON_CreateString("busdata"));
        if ( SUPERNET.SERVICESECRET[0] != 0 )
            cJSON_AddItemToObject(datajson,"serviceNXT",cJSON_CreateString(SUPERNET.SERVICENXT));
        cJSON_AddItemToObject(datajson,"key",cJSON_CreateString(key));
        cJSON_AddItemToObject(datajson,"time",cJSON_CreateNumber(timestamp + diff));
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
        tlen = construct_tokenized_req(tokbuf,str2,secret,broadcastmode);
//printf("created busdata.(%s) -> (%s) tlen.%d\n",str,tokbuf,tlen);
        free(tmp), free(str), free(str2), str = str2 = 0;
        *datalenp = tlen;
        free_json(json);
    } else printf("couldnt parse busdata json.(%s)\n",jsonstr);
    return(tokbuf);
}

char *busdata_sync(char *jsonstr,char *broadcastmode)
{
    int32_t sentflag,datalen,sendlen = 0; char plugin[512],destplugin[512],*data,*retstr; cJSON *json;
    json = cJSON_Parse(jsonstr);
    copy_cJSON(plugin,cJSON_GetObjectItem(json,"plugin"));
    copy_cJSON(destplugin,cJSON_GetObjectItem(json,"destplugin"));
    if ( strcmp(plugin,"relay") == 0 && strcmp(destplugin,"relay") == 0 && broadcastmode == 0 )
        broadcastmode = "4";
    sentflag = 0;
    //printf("relay.%d busdata_sync.(%s) (%s)\n",SUPERNET.iamrelay,jsonstr,broadcastmode==0?"":broadcastmode);
    if ( (data= create_busdata(&datalen,jsonstr,broadcastmode)) != 0 )
    {
        if ( SUPERNET.iamrelay != 0 )
        {
            if ( json != 0 )
            {
                if ( strcmp(broadcastmode,"publicaccess") == 0 )
                {
                    retstr = nn_busdata_processor((uint8_t *)data,datalen);
                    if ( data != jsonstr )
                        free(data);
                    free_json(json);
                    //printf("relay returns publicaccess.(%s)\n",retstr);
                    return(retstr);
                } else free_json(json);
                if ( RELAYS.pubglobal >= 0 && (strcmp(broadcastmode,"allnodes") == 0 || strcmp(broadcastmode,"8") == 0) )
                {
                    if( (sendlen= nn_send(RELAYS.pubglobal,data,datalen,0)) != datalen )
                    {
                        if ( Debuglevel > 1 )
                            printf("globl sendlen.%d vs datalen.%d (%s) %s\n",sendlen,datalen,(char *)data,nn_errstr());
                        free(data);
                        return(clonestr("{\"error\":\"couldnt send to allnodes\"}"));
                    }
                    sentflag = 1;
                }
            }
            if ( sentflag == 0 && RELAYS.pubrelays >= 0 )
            {
                if( (sendlen= nn_send(RELAYS.pubrelays,data,datalen,0)) != datalen )
                {
                    if ( Debuglevel > 1 )
                        printf("sendlen.%d vs datalen.%d (%s) %s\n",sendlen,datalen,(char *)data,nn_errstr());
                    free(data);
                    return(clonestr("{\"error\":\"couldnt send to allrelays\"}"));
                } // else printf("PUB.(%s) sendlen.%d datalen.%d\n",data,sendlen,datalen);
            }
            if ( data != jsonstr )
                free(data);
            return(clonestr("{\"result\":\"sent to bus\"}"));
        }
        else
        {
            if ( json != 0 )
            {
                if ( broadcastmode == 0 && cJSON_str(cJSON_GetObjectItem(json,"servicename")) == 0 )
                {
                    //printf("call busdata proc.(%s)\n",data);
                    retstr = nn_busdata_processor((uint8_t *)data,datalen);
                }
                else
                {
                    //printf("LBsend.(%s)\n",data);
                    retstr = nn_loadbalanced((uint8_t *)data,datalen);
                }
                if ( 0 && retstr != 0 )
                    printf("busdata nn_loadbalanced retstr.(%s) %p\n",retstr,retstr);
                if ( data != jsonstr )
                    free(data);
                free_json(json);
                return(retstr);
            } else printf("Cant parse busdata_sync.(%s)\n",jsonstr);
        }
    }
    return(clonestr("{\"error\":\"error creating busdata\"}"));
}

int32_t complete_relay(struct relayargs *args,char *retstr)
{
    int32_t len,sendlen;
    _stripwhite(retstr,' ');
    len = (int32_t)strlen(retstr)+1;
    if ( args->type != NN_BUS && args->type != NN_SUB && (sendlen= nn_send(args->sock,retstr,len,0)) != len )
    {
        printf("complete_relay.%s warning: send.%d vs %d for (%s) sock.%d %s\n",args->name,sendlen,len,retstr,args->sock,nn_errstr());
        return(-1);
    }
    //printf("SUCCESS complete_relay.(%s) -> sock.%d %s\n",retstr,args->sock,args->name);
    return(0);
}

int32_t busdata_poll()
{
    char tokenized[65536],*msg,*retstr; cJSON *json,*retjson; int32_t len,noneed,sock,i,n = 0;
    if ( RELAYS.numservers > 0 )
    {
        for (i=0; i<RELAYS.numservers; i++)
        {
            sock = RELAYS.pfd[i].fd;
            //printf("n.%d i.%d check socket.%d:%d revents.%d\n",n,i,RELAYS.pfd[i].fd,RELAYS.pfd[i].fd,RELAYS.pfd[i].revents);
            //if ( (RELAYS.pfd[i].revents & NN_POLLIN) != 0 && (len= nn_recv(sock,&msg,NN_MSG,0)) > 0 )
            if ( (len= nn_recv(sock,&msg,NN_MSG,0)) > 0 )
            {
                if ( Debuglevel > 2 )
                    printf("RECV.%d (%s)\n",sock,msg);
                n++;
                if ( (json= cJSON_Parse(msg)) != 0 )
                {
                    if ( (retstr= nn_busdata_processor((uint8_t *)msg,len)) != 0 )
                    {
                        noneed = 0;
                        if ( (retjson= cJSON_Parse(retstr)) != 0 )
                        {
                            if ( is_cJSON_Array(retjson) != 0 && cJSON_GetArraySize(retjson) == 2 )
                            {
                                noneed = 1;
                                //fprintf(stderr,"return.(%s)\n",retstr);
                                nn_send(sock,retstr,(int32_t)strlen(retstr)+1,0);
                            }
                            free_json(retjson);
                        }
                        if ( noneed == 0 )
                        {
                            len = construct_tokenized_req(tokenized,retstr,(sock == RELAYS.servicesock) ? SUPERNET.SERVICESECRET : SUPERNET.NXTACCTSECRET,0);
                            //fprintf(stderr,"tokenized return.(%s)\n",tokenized);
                            nn_send(sock,tokenized,len,0);
                        }
                        free(retstr);
                    } else nn_send(sock,"{\"error\":\"null return\"}",(int32_t)strlen("{\"error\":\"null return\"}")+1,0);
                    free_json(json);
                }
                nn_freemsg(msg);
            }
        }
    }
    return(n);
}

void busdata_init(int32_t sendtimeout,int32_t recvtimeout,int32_t firstiter)
{
    char endpoint[512]; int32_t i;
    RELAYS.servicesock = RELAYS.pubglobal = RELAYS.pubrelays = RELAYS.lbserver = -1;
    endpoint[0] = 0;
    if ( (RELAYS.subclient= nn_createsocket(endpoint,0,"NN_SUB",NN_SUB,0,sendtimeout,recvtimeout)) >= 0 )
    {
        RELAYS.pfd[RELAYS.numservers++].fd = RELAYS.subclient, printf("numservers.%d\n",RELAYS.numservers);
        nn_setsockopt(RELAYS.subclient,NN_SUB,NN_SUB_SUBSCRIBE,"",0);
    } else printf("error creating subclient\n");
    RELAYS.lbclient = nn_lbsocket(SUPERNET.PLUGINTIMEOUT,SUPERNET_PORT + LB_OFFSET,SUPERNET.port + PUBGLOBALS_OFFSET,SUPERNET.port + PUBRELAYS_OFFSET);
    sprintf(endpoint,"%s://%s:%u",SUPERNET.transport,SUPERNET.myipaddr,SUPERNET.serviceport);
    if ( (RELAYS.servicesock= nn_createsocket(endpoint,1,"NN_REP",NN_REP,SUPERNET.serviceport,sendtimeout,recvtimeout)) >= 0 )
        RELAYS.pfd[RELAYS.numservers++].fd = RELAYS.servicesock, printf("numservers.%d\n",RELAYS.numservers);
    else printf("error createing servicesock\n");
    if ( SUPERNET.iamrelay != 0 )
    {
        sprintf(endpoint,"%s://%s:%u",SUPERNET.transport,SUPERNET.myipaddr,SUPERNET.port + LB_OFFSET);
        if ( (RELAYS.lbserver= nn_createsocket(endpoint,1,"NN_REP",NN_REP,SUPERNET.port + LB_OFFSET,sendtimeout,recvtimeout)) >= 0 )
            RELAYS.pfd[RELAYS.numservers++].fd = RELAYS.lbserver, printf("numservers.%d\n",RELAYS.numservers);
        else printf("error creating lbserver\n");
        sprintf(endpoint,"%s://%s:%u",SUPERNET.transport,SUPERNET.myipaddr,SUPERNET.port + PUBGLOBALS_OFFSET);
        RELAYS.pubglobal = nn_createsocket(endpoint,1,"NN_PUB",NN_PUB,SUPERNET.port + PUBGLOBALS_OFFSET,sendtimeout,recvtimeout);
        sprintf(endpoint,"%s://%s:%u",SUPERNET.transport,SUPERNET.myipaddr,SUPERNET.port + PUBRELAYS_OFFSET);
        RELAYS.pubrelays = nn_createsocket(endpoint,1,"NN_PUB",NN_PUB,SUPERNET.port + PUBRELAYS_OFFSET,sendtimeout,recvtimeout);
    }
    for (i=0; i<RELAYS.numservers; i++)
        RELAYS.pfd[i].events = NN_POLLIN | NN_POLLOUT;
    printf("SUPERNET.iamrelay %d, numservers.%d\n",SUPERNET.iamrelay,RELAYS.numservers);
}

int32_t init_SUPERNET_pullsock(int32_t sendtimeout,int32_t recvtimeout)
{
    char bindaddr[64],*transportstr; int32_t iter;
    if ( (SUPERNET.pullsock= nn_socket(AF_SP,NN_PULL)) < 0 )
    {
        printf("error creating pullsock %s\n",nn_strerror(nn_errno()));
        return(-1);
    }
    else if ( nn_settimeouts(SUPERNET.pullsock,sendtimeout,recvtimeout) < 0 )
    {
        printf("error settime pullsock timeouts %s\n",nn_strerror(nn_errno()));
        return(-1);
    }
    printf("SUPERNET.pullsock.%d\n",SUPERNET.pullsock);
/*#ifdef _WIN32
    sprintf(bindaddr,"tcp://127.0.0.1:7774");
    if ( nn_bind(SUPERNET.pullsock,bindaddr) < 0 )
    {
        printf("error binding pullsock to (%s) %s\n",bindaddr,nn_strerror(nn_errno()));
        return(-1);
    }
#else*/
    for (iter=0; iter<2; iter++)
    {
        transportstr = (iter == 0) ? "ipc" : "inproc";
        sprintf(bindaddr,"%s://SuperNET",transportstr);
        if ( nn_bind(SUPERNET.pullsock,bindaddr) < 0 )
        {
            printf("error binding pullsock to (%s) %s\n",bindaddr,nn_strerror(nn_errno()));
            return(-1);
        }
    }
//#endif
    return(0);
}

