//
//  relays777.c
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
//#include "gen1block.c"
#undef DEFINES_ONLY
#define NN_WS -4

int32_t relay_idle(struct plugin_info *plugin) { return(0); }

STRUCTNAME RELAYS;
char *PLUGNAME(_methods)[] = { "list", "add", "direct", "join", "busdata", "msigaddr", "allservices" }; // list of supported methods
char *PLUGNAME(_pubmethods)[] = { "list", "add", "direct", "join", "busdata", "msigaddr", "serviceprovider", "allservices", "nonce" }; // list of supported methods
char *PLUGNAME(_authmethods)[] = { "list", "add", "direct", "join", "busdata", "msigaddr", "allservices" }; // list of supported methods

int32_t nn_typelist[] = { NN_REP, NN_REQ, NN_RESPONDENT, NN_SURVEYOR, NN_PUB, NN_SUB, NN_PULL, NN_PUSH, NN_BUS, NN_PAIR };
char *nn_transports[] = { "tcp", "ws", "ipc", "inproc", "tcpmux", "tbd1", "tbd2", "tbd3" };

void expand_epbits(char *endpoint,struct endpoint epbits)
{
    char ipaddr[64];
    if ( epbits.ipbits != 0 )
        expand_ipbits(ipaddr,epbits.ipbits);
    else strcpy(ipaddr,"*");
    sprintf(endpoint,"%s://%s:%d",nn_transports[epbits.transport],ipaddr,epbits.port);
}

struct endpoint calc_epbits(char *transport,uint32_t ipbits,uint16_t port,int32_t type)
{
    int32_t i; struct endpoint epbits;
    memset(&epbits,0,sizeof(epbits));
    for (i=0; i<(int32_t)(sizeof(nn_transports)/sizeof(*nn_transports)); i++)
        if ( strcmp(transport,nn_transports[i]) == 0 )
        {
            epbits.ipbits = ipbits;
            epbits.port = port;
            epbits.transport = i;
            epbits.nn = type;
            break;
        }
    return(epbits);
}

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *argjson)
{
    uint64_t disableflags = 0;
    return(disableflags); // set bits corresponding to array position in _methods[]
}

char *nn_typestr(int32_t type)
{
    switch ( type )
    {
            // Messages that need a response from the set of peers: SURVEY
        case NN_SURVEYOR: return("NN_SURVEYOR"); break;
        case NN_RESPONDENT: return("NN_RESPONDENT"); break;
            // Messages that need a response, but only from one peer: REQ/REP
        case NN_REQ: return("NN_REQ"); break;
        case NN_REP: return("NN_REP"); break;
            // One-way messages to one peer: PUSH/PULL
        case NN_PUSH: return("NN_PUSH"); break;
        case NN_PULL: return("NN_PULL"); break;
            //  One-way messages to all: PUB/SUB
        case NN_PUB: return("NN_PUB"); break;
        case NN_SUB: return("NN_SUB"); break;
        case NN_BUS: return("NN_BUS"); break;
        case NN_PAIR: return("NN_PAIR"); break;
    }
    return("NN_ERROR");
}

int32_t nn_oppotype(int32_t type)
{
    switch ( type )
    {
            // Messages that need a response from the set of peers: SURVEY
        case NN_SURVEYOR: return(NN_RESPONDENT); break;
        case NN_RESPONDENT: return(NN_SURVEYOR); break;
            // Messages that need a response, but only from one peer: REQ/REP
        case NN_REQ: return(NN_REP); break;
        case NN_REP: return(NN_REQ); break;
            // One-way messages to one peer: PUSH/PULL
        case NN_PUSH: return(NN_PULL); break;
        case NN_PULL: return(NN_PUSH); break;
            //  One-way messages to all: PUB/SUB
        case NN_PUB: return(NN_SUB); break;
        case NN_SUB: return(NN_PUB); break;
        case NN_BUS: return(NN_BUS); break;
        case NN_PAIR: return(NN_PAIR); break;
    }
    return(-1);
}

int32_t nn_portoffset(int32_t type)
{
    int32_t i;
    for (i=0; i<(int32_t)(sizeof(nn_typelist)/sizeof(*nn_typelist)); i++)
        if ( nn_typelist[i] == type )
            return(i + 2);
    return(-1);
}

struct endpoint find_epbits(struct _relay_info *list,uint32_t ipbits,uint16_t port,int32_t type)
{
    int32_t i; struct endpoint epbits;
    memset(&epbits,0,sizeof(epbits));
    if ( list != 0 && list->num > 0 )
    {
        if ( type >= 0 )
            type = nn_portoffset(type);
        for (i=0; i<list->num&&i<(int32_t)(sizeof(list->connections)/sizeof(*list->connections)); i++)
            if ( list->connections[i].ipbits == ipbits && (port == 0 || port == list->connections[i].port)  && (type < 0 || type == list->connections[i].nn) )
                return(list->connections[i]);
    }
    return(epbits);
}

int32_t add_relay(struct _relay_info *list,struct endpoint epbits)
{
    //static portable_mutex_t mutex; static int didinit;
    //if ( didinit == 0 ) didinit++, portable_mutex_init(&mutex);
    //portable_mutex_lock(&mutex);
    list->connections[list->num % (sizeof(list->connections)/sizeof(*list->connections))] = epbits, list->num++;
    //portable_mutex_unlock(&mutex);
    if ( list->num > (sizeof(list->connections)/sizeof(*list->connections)) )
        printf("add_relay warning num.%d > %ld\n",list->num,(sizeof(list->connections)/sizeof(*list->connections)));
    return(list->num);
}

/*int32_t update_serverbits(struct _relay_info *list,char *transport,uint32_t ipbits,uint16_t port,int32_t type)
{
    char endpoint[1024]; struct endpoint epbits;
    if ( ipbits == SUPERNET.myipbits )
        return(-1);
    if ( list->sock < 0 )
    {
        printf("illegal list sock.%d\n",list->sock);
        return(-1);
    }
//printf("%p update_serverbits sock.%d type.%d num.%d ipbits.%llx\n",list,list->sock,type,list->num,(long long)ipbits);
    epbits = find_epbits(list,ipbits,port,type);
    if ( epbits.ipbits == 0 )
    {
        epbits = calc_epbits(transport,ipbits,port,type);
        expand_epbits(endpoint,epbits);
        if ( nn_connect(list->sock,endpoint) < 0 )
            printf("error connecting to (%s) %s\n",endpoint,nn_errstr());
        else
        {
            if ( type == NN_PUB )
            {
                printf("subscribed to (%s)\n",endpoint);
                nn_setsockopt(list->sock,NN_SUB,NN_SUB_SUBSCRIBE,"",0);
            }
            //add_relay(list,epbits);
        }
    }
    return(list->num);
}*/

int32_t nn_add_lbservers(uint16_t port,uint16_t globalport,uint16_t relaysport,int32_t priority,int32_t sock,char servers[][MAX_SERVERNAME],int32_t num)
{
    int32_t i; char endpoint[512],pubendpoint[512]; struct endpoint epbits; uint32_t ipbits;
    if ( num > 0 && servers != 0 && nn_setsockopt(sock,NN_SOL_SOCKET,NN_SNDPRIO,&priority,sizeof(priority)) >= 0 )
    {
        for (i=0; i<num; i++)
        {
            if ( (ipbits= (uint32_t)calc_ipbits(servers[i])) == 0 )
            {
                printf("null ipbits.(%s)\n",servers[i]);
                continue;
            }
            //printf("epbits.%llx ipbits.%x %s\n",*(long long *)&epbits,(uint32_t)ipbits,endpoint);
            if ( ismyaddress(servers[i]) == 0 )
            {
                epbits = calc_epbits("tcp",ipbits,port,NN_REP);
                expand_epbits(endpoint,epbits);
                if ( nn_connect(sock,endpoint) >= 0 )
                {
                    printf("+R%s ",endpoint);
                    add_relay(&RELAYS.active,epbits);
                }
                if ( RELAYS.subclient >= 0 )
                {
                    if ( SUPERNET.iamrelay != 0 )
                    {
                        epbits = calc_epbits("tcp",ipbits,relaysport,NN_PUB);
                        expand_epbits(pubendpoint,epbits);
                        if ( nn_connect(RELAYS.subclient,pubendpoint) >= 0 )
                            printf("+P%s ",pubendpoint);
                    }
                    epbits = calc_epbits("tcp",ipbits,globalport,NN_PUB);
                    expand_epbits(pubendpoint,epbits);
                    if ( nn_connect(RELAYS.subclient,pubendpoint) >= 0 )
                        printf("+P%s ",pubendpoint);
                }
            }
        }
        priority++;
    } else printf("error setting priority.%d (%s)\n",priority,nn_errstr());
    return(priority);
}

int32_t _lb_socket(uint16_t port,uint16_t globalport,uint16_t relaysport,int32_t maxmillis,char servers[][MAX_SERVERNAME],int32_t num,char backups[][MAX_SERVERNAME],int32_t numbacks,char failsafes[][MAX_SERVERNAME],int32_t numfailsafes)
{
    int32_t lbsock,timeout,retrymillis,priority = 1;
    if ( (lbsock= nn_socket(AF_SP,NN_REQ)) >= 0 )
    {
        retrymillis = (maxmillis / 30) + 1;
printf("!!!!!!!!!!!! lbsock.%d !!!!!!!!!!!\n",lbsock);
        if ( nn_setsockopt(lbsock,NN_SOL_SOCKET,NN_RECONNECT_IVL,&retrymillis,sizeof(retrymillis)) < 0 )
            printf("error setting NN_REQ NN_RECONNECT_IVL_MAX socket %s\n",nn_errstr());
        else if ( nn_setsockopt(lbsock,NN_SOL_SOCKET,NN_RECONNECT_IVL_MAX,&maxmillis,sizeof(maxmillis)) < 0 )
            fprintf(stderr,"error setting NN_REQ NN_RECONNECT_IVL_MAX socket %s\n",nn_errstr());
        timeout = SUPERNET.PLUGINTIMEOUT;
        if ( nn_setsockopt(lbsock,NN_SOL_SOCKET,NN_RCVTIMEO,&timeout,sizeof(timeout)) < 0 )
            printf("error setting NN_SOL_SOCKET NN_RCVTIMEO socket %s\n",nn_errstr());
        timeout = 100;
        if ( nn_setsockopt(lbsock,NN_SOL_SOCKET,NN_SNDTIMEO,&timeout,sizeof(timeout)) < 0 )
            printf("error setting NN_SOL_SOCKET NN_SNDTIMEO socket %s\n",nn_errstr());
        if ( num > 0 )
            priority = nn_add_lbservers(port,globalport,relaysport,priority,lbsock,servers,num);
        if ( numbacks > 0 )
            priority = nn_add_lbservers(port,globalport,relaysport,priority,lbsock,backups,numbacks);
        if ( numfailsafes > 0 )
            priority = nn_add_lbservers(port,globalport,relaysport,priority,lbsock,failsafes,numfailsafes);
    } else printf("error getting req socket %s\n",nn_errstr());
    //printf("RELAYS.lb.num %d\n",RELAYS.lb.num);
    return(lbsock);
}

int32_t badass_servers(char servers[][MAX_SERVERNAME],int32_t max,int32_t port)
{
    int32_t n = 0;
    strcpy(servers[n++],"89.248.160.237");
    strcpy(servers[n++],"89.248.160.238");
    strcpy(servers[n++],"89.248.160.239");
    strcpy(servers[n++],"89.248.160.240");
    strcpy(servers[n++],"89.248.160.241");
    strcpy(servers[n++],"89.248.160.242");
    strcpy(servers[n++],"89.248.160.243");
    strcpy(servers[n++],"89.248.160.244");
    //strcpy(servers[n++],"89.248.160.245");
    return(n);
}

int32_t crackfoo_servers(char servers[][MAX_SERVERNAME],int32_t max,int32_t port)
{
    int32_t n = 0;
    /*strcpy(servers[n++],"192.99.151.160");
     strcpy(servers[n++],"167.114.96.223");
     strcpy(servers[n++],"167.114.113.197");
     strcpy(servers[n++],"5.9.105.170");
     strcpy(servers[n++],"136.243.5.70");
     strcpy(servers[n++],"5.9.155.145");*/
    if ( 0 )
    {
        strcpy(servers[n++],"167.114.96.223");
        strcpy(servers[n++],"167.114.113.25");
        strcpy(servers[n++],"167.114.113.27");
        strcpy(servers[n++],"167.114.113.194");
        strcpy(servers[n++],"167.114.113.197");
        strcpy(servers[n++],"167.114.113.201");
        strcpy(servers[n++],"167.114.113.246");
        strcpy(servers[n++],"167.114.113.249");
        strcpy(servers[n++],"167.114.113.250");
        strcpy(servers[n++],"192.99.151.160");
        strcpy(servers[n++],"167.114.96.222");
    }
    return(n);
}

int32_t nn_lbsocket(int32_t maxmillis,int32_t port,uint16_t globalport,uint16_t relaysport)
{
    char Cservers[32][MAX_SERVERNAME],Bservers[32][MAX_SERVERNAME],failsafes[4][MAX_SERVERNAME];
    int32_t n,m,lbsock,numfailsafes = 0;
    strcpy(failsafes[numfailsafes++],"5.9.56.103");
    strcpy(failsafes[numfailsafes++],"5.9.102.210");
    n = crackfoo_servers(Cservers,sizeof(Cservers)/sizeof(*Cservers),port);
    m = badass_servers(Bservers,sizeof(Bservers)/sizeof(*Bservers),port);
    lbsock = _lb_socket(port,globalport,relaysport,maxmillis,Bservers,0*m,Cservers,0*n,failsafes,numfailsafes);
    return(lbsock);
}

int32_t nn_settimeouts(int32_t sock,int32_t sendtimeout,int32_t recvtimeout)
{
    int32_t retrymillis,maxmillis;
    maxmillis = SUPERNET.PLUGINTIMEOUT, retrymillis = maxmillis/40;
    if ( nn_setsockopt(sock,NN_SOL_SOCKET,NN_RECONNECT_IVL,&retrymillis,sizeof(retrymillis)) < 0 )
        fprintf(stderr,"error setting NN_REQ NN_RECONNECT_IVL_MAX socket %s\n",nn_errstr());
    else if ( nn_setsockopt(sock,NN_SOL_SOCKET,NN_RECONNECT_IVL_MAX,&maxmillis,sizeof(maxmillis)) < 0 )
        fprintf(stderr,"error setting NN_REQ NN_RECONNECT_IVL_MAX socket %s\n",nn_errstr());
    else if ( sendtimeout > 0 && nn_setsockopt(sock,NN_SOL_SOCKET,NN_SNDTIMEO,&sendtimeout,sizeof(sendtimeout)) < 0 )
        fprintf(stderr,"error setting sendtimeout %s\n",nn_errstr());
    else if ( recvtimeout > 0 && nn_setsockopt(sock,NN_SOL_SOCKET,NN_RCVTIMEO,&recvtimeout,sizeof(recvtimeout)) < 0 )
        fprintf(stderr,"error setting sendtimeout %s\n",nn_errstr());
    else return(0);
    return(-1);
}

int32_t nn_createsocket(char *endpoint,int32_t bindflag,char *name,int32_t type,uint16_t port,int32_t sendtimeout,int32_t recvtimeout)
{
    int32_t sock;
    if ( (sock= nn_socket(AF_SP,type)) < 0 )
        fprintf(stderr,"error getting socket %s\n",nn_errstr());
    if ( bindflag != 0 )
    {
        if ( endpoint[0] == 0 )
            expand_epbits(endpoint,calc_epbits(SUPERNET.transport,0,port,type));
        if ( nn_bind(sock,endpoint) < 0 )
            fprintf(stderr,"error binding to relaypoint sock.%d type.%d to (%s) (%s) %s\n",sock,type,name,endpoint,nn_errstr());
        else fprintf(stderr,"BIND.(%s) <- %s\n",endpoint,name);
    }
    else if ( bindflag == 0 && endpoint[0] != 0 )
    {
        if ( nn_connect(sock,endpoint) < 0 )
            fprintf(stderr,"error connecting to relaypoint sock.%d type.%d to (%s) (%s) %s\n",sock,type,name,endpoint,nn_errstr());
        else fprintf(stderr,"%s -> CONNECT.(%s)\n",name,endpoint);
    }
    if ( nn_settimeouts(sock,sendtimeout,recvtimeout) < 0 )
    {
        fprintf(stderr,"nn_createsocket.(%s) %d\n",name,sock);
        return(-1);
    }
    return(sock);
}

int32_t nn_socket_status(int32_t sock,int32_t timeoutmillis)
{
    struct nn_pollfd pfd;
    int32_t rc;
    pfd.fd = sock;
    pfd.events = NN_POLLIN | NN_POLLOUT;
    if ( (rc= nn_poll(&pfd,1,timeoutmillis)) == 0 )
        return(pfd.revents);
    else return(-1);
}

void add_standard_fields(char *request)
{
    cJSON *json;
    if ( (json= cJSON_Parse(request)) != 0 )
    {
        if ( get_API_nxt64bits(cJSON_GetObjectItem(json,"NXT")) == 0 )
        {
            sprintf(request + strlen(request) - 1,",\"NXT\":\"%s\",\"tag\":\"%llu\"}",SUPERNET.NXTADDR,(((long long)rand() << 32) | (uint32_t)rand()));
            if ( SUPERNET.iamrelay != 0 && (SUPERNET.hostname[0] != 0 || SUPERNET.myipaddr[0] != 0) )
                sprintf(request + strlen(request) - 1,",\"iamrelay\":\"%s\"}",SUPERNET.hostname[0]!=0?SUPERNET.hostname:SUPERNET.myipaddr);
        }
        free_json(json);
    }
}

char *nn_loadbalanced(uint8_t *data,int32_t len)
{
    char *msg,*jsonstr = 0;
    int32_t sendlen,i,lbsock,recvlen = 0;
    if ( (lbsock= RELAYS.lbclient) < 0 )
        return(clonestr("{\"error\":\"invalid load balanced socket\"}"));
    for (i=0; i<10; i++)
        if ( (nn_socket_status(lbsock,1) & NN_POLLOUT) != 0 )
            break;
//printf("sock.%d NN_LBSEND.(%s)\n",lbsock,data);
    if ( (sendlen= nn_send(lbsock,data,len,0)) == len )
    {
        for (i=0; i<10; i++)
            if ( (nn_socket_status(lbsock,1) & NN_POLLIN) != 0 )
                break;
        if ( (recvlen= nn_recv(lbsock,&msg,NN_MSG,0)) > 0 )
        {
            if ( Debuglevel > 2 )
                printf("LBRECV.(%s)\n",msg);
            jsonstr = clonestr((char *)msg);
            nn_freemsg(msg);
        }
        else
        {
            printf("got recvlen.%d %s\n",recvlen,nn_errstr());
            jsonstr = clonestr("{\"error\":\"lb recv error, probably timeout\"}");
        }
    } else printf("got sendlen.%d instead of %d %s\n",sendlen,len,nn_errstr()), jsonstr = clonestr("{\"error\":\"lb send error\"}");
    return(jsonstr);
}

cJSON *relay_json(struct _relay_info *list)
{
    cJSON *json,*array;
    char endpoint[512];
    int32_t i;
    if ( list == 0 || list->num == 0 )
        return(0);
    array = cJSON_CreateArray();
    for (i=0; i<list->num&&i<(int32_t)(sizeof(list->connections)/sizeof(*list->connections)); i++)
    {
        expand_epbits(endpoint,list->connections[i]);
        cJSON_AddItemToArray(array,cJSON_CreateString(endpoint));
    }
    json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"endpoints",array);
    cJSON_AddItemToObject(json,"type",cJSON_CreateString(nn_typestr(list->mytype)));
    cJSON_AddItemToObject(json,"dest",cJSON_CreateString(nn_typestr(list->desttype)));
    cJSON_AddItemToObject(json,"total",cJSON_CreateNumber(list->num));
    return(json);
}

char *relays_jsonstr(char *jsonstr,cJSON *argjson)
{
    cJSON *json; char *retstr;
    if ( SUPERNET.iamrelay != 0 && SUPERNET.myipaddr[0] != 0 )
    {
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json,"relay",cJSON_CreateString(SUPERNET.myipaddr));
        if ( RELAYS.active.num > 0 )
            cJSON_AddItemToObject(json,"relays",relay_json(&RELAYS.active));
        retstr = cJSON_Print(json);
        free_json(json);
        return(retstr);
    }
    else return(clonestr("{\"error\":\"get relay list from relay\"}"));
}

void serverloop(void *_args)
{
    int32_t make_MGWbus(uint16_t port,char *bindaddr,char serverips[MAX_MGWSERVERS][64],int32_t n);
    int32_t mgw_processbus(char *retbuf,char *jsonstr,cJSON *json);
    int32_t poll_daemons();
    int32_t len,n; char retbuf[8192],*jsonstr; cJSON *json;
    busdata_init(10,1,0);
    if ( SUPERNET.gatewayid >= 0 )
        MGW.all.socks.both.bus = make_MGWbus(MGW.port,SUPERNET.myipaddr,MGW.serverips,SUPERNET.numgateways+1*0);
    sleep(10);
    while ( OS_getppid() == SUPERNET.ppid )
    {
        n = poll_daemons();
        if ( SUPERNET.gatewayid >= 0 && (len= nn_recv(MGW.all.socks.both.bus,&jsonstr,NN_MSG,0)) > 0 )
        {
            if ( (json= cJSON_Parse(jsonstr)) != 0 )
            {
                mgw_processbus(retbuf,jsonstr,json);
                free_json(json);
            }
            //printf("MGW bus recv.%d json.%p\n",len,json);
            nn_freemsg(jsonstr);
        }
        n += busdata_poll();
        if ( n == 0 && SUPERNET.APISLEEP > 0 )
            msleep(SUPERNET.APISLEEP);
    }
}

void calc_nonces(char *destpoint)
{
    char buf[8192],endpoint[8192],*str; int32_t n = 0; double endmilli = milliseconds() + 60000;
    //printf("calc_nonces.(%s)\n",destpoint);
    expand_epbits(endpoint,calc_epbits("tcp",(uint32_t)calc_ipbits(SUPERNET.myipaddr),SUPERNET.port+PUBRELAYS_OFFSET,NN_PUB));
    while ( milliseconds() < endmilli )
    {
        sprintf(buf,"{\"plugin\":\"relay\",\"counter\":\"%d\",\"destplugin\":\"relay\",\"method\":\"nonce\",\"broadcast\":\"8\",\"myendpoint\":\"%s\",\"destpoint\":\"%s\",\"NXT\":\"%s\"}",n,endpoint,destpoint,SUPERNET.NXTADDR);
        n++;
        if ( (str= busdata_sync(buf,"8")) != 0 )
        {
            fprintf(stderr,"send.(%s)\n",buf);
            free(str);
        }
    }
    SUPERNET.noncing = 0;
    printf("finished noncing for (%s)\n",destpoint);
    free(destpoint);
}

int32_t PLUGNAME(_process_json)(char *forwarder,char *sender,int32_t valid,struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *origjsonstr,cJSON *origjson,int32_t initflag)
{
    char buf[8192],endpoint[128],tagstr[512],*resultstr,*retstr = 0,*methodstr,*jsonstr; cJSON *retjson,*json;
    retbuf[0] = 0;
    if ( is_cJSON_Array(origjson) != 0 && cJSON_GetArraySize(origjson) == 2 )
        json = cJSON_GetArrayItem(origjson,0), jsonstr = cJSON_Print(json), _stripwhite(jsonstr,' ');
    else json = origjson, jsonstr = origjsonstr;
    //printf("<<<<<<<<<<<< INSIDE relays PLUGIN! process %s (%s)\n",plugin->name,jsonstr);
    if ( initflag > 0 )
    {
        // configure settings
        RELAYS.readyflag = 1;
        plugin->allowremote = 1;
        strcpy(retbuf,"{\"result\":\"initflag > 0\"}");
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        methodstr = cJSON_str(cJSON_GetObjectItem(json,"method"));
        if ( methodstr == 0 || methodstr[0] == 0 )
        {
            printf("(%s) has not method\n",jsonstr);
            return(0);
        }
        //fprintf(stderr,"RELAYS methodstr.(%s) (%s)\n",methodstr,jsonstr);
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
        else if ( strcmp(methodstr,"msigaddr") == 0 )
        {
            char *devMGW_command(char *jsonstr,cJSON *json);
            if ( SUPERNET.gatewayid >= 0 )
                retstr = devMGW_command(jsonstr,json);
        }
        else
        {
            strcpy(retbuf,"{\"result\":\"relay command under construction\"}");
            if ( strcmp(methodstr,"list") == 0 )
                retstr = relays_jsonstr(jsonstr,json);
            else if ( strcmp(methodstr,"busdata") == 0 )
                retstr = busdata_sync(jsonstr,cJSON_str(cJSON_GetObjectItem(json,"broadcast")));
            else if ( strcmp(methodstr,"allservices") == 0 )
            {
                if ( (retjson= serviceprovider_json()) != 0 )
                {
                    retstr = cJSON_Print(retjson), _stripwhite(retstr,' ');
                    free_json(retjson);
                    //printf("got.(%s)\n",retstr);
                } else printf("null serviceprovider_json()\n");
            }
            else if ( strcmp(methodstr,"join") == 0 || strcmp(methodstr,"nonce") == 0  )
            {
                if ( SUPERNET.iamrelay != 0 && SUPERNET.noncing == 0 )
                {
                    copy_cJSON(tagstr,cJSON_GetObjectItem(json,"tag"));
                    copy_cJSON(endpoint,cJSON_GetObjectItem(json,"endpoint"));
                    //nn_connect(RELAYS.subclient,endpoint);
                    if ( strcmp(methodstr,"join") == 0 )
                    {
                        SUPERNET.noncing = 1;
                        portable_thread_create((void *)calc_nonces,clonestr(endpoint));
                        sprintf(retbuf,"{\"result\":\"noncing\",\"endpoint\":\"%s\"}",endpoint);
                    } else sprintf(retbuf,"{\"result\":\"nonce stats\",\"endpoint\":\"%s\"}",endpoint);
                    fprintf(stderr,"join or nonce.(%s)\n",retbuf);
                }
                else
                {
                    char endpoint[512],sender[64];
                    copy_cJSON(endpoint,cJSON_GetObjectItem(json,"myendpoint"));
                    copy_cJSON(sender,cJSON_GetObjectItem(json,"NXT"));
                    sprintf(buf,"{\"result\":\"gotnonce\",\"endpoint\":\"%s\",\"NXT\":\"%s\"}",endpoint,sender);
                    fprintf(stderr,"received.(%s) from (%s)\n",origjsonstr,sender);
                }
            }
        }
        if ( retstr != 0 )
        {
            strcpy(retbuf,retstr);
            free(retstr);
        }
    }
    return((int32_t)strlen(retbuf) + retbuf[0] != 0);
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    return(retcode);
}
#include "plugin777.c"
