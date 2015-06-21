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
//#include "gen1block.c"
#undef DEFINES_ONLY
#define NN_WS -4

int32_t relay_idle(struct plugin_info *plugin) { return(0); }

STRUCTNAME RELAYS;
char *PLUGNAME(_methods)[] = { "list", "add", "direct", "join", "busdata", "msigaddr" }; // list of supported methods
char *PLUGNAME(_pubmethods)[] = { "list", "add", "direct", "join", "busdata", "msigaddr" }; // list of supported methods
char *PLUGNAME(_authmethods)[] = { "list", "add", "direct", "join", "busdata", "msigaddr" }; // list of supported methods

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
            return(i+10);
    return(-1);
}

/*void set_endpointaddr(char *transport,char *endpoint,char *domain,uint16_t refport,int32_t type)
{
    sprintf(endpoint,"%s://%s:%d",transport,domain,refport + nn_portoffset(type));
}*/

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

int32_t update_serverbits(struct _relay_info *list,char *transport,uint32_t ipbits,uint16_t port,int32_t type)
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
            if ( list->desttype == NN_PUB )
            {
                printf("subscribed to (%s)\n",endpoint);
                nn_setsockopt(list->sock,NN_SUB,NN_SUB_SUBSCRIBE,"",0);
            }
            add_relay(list,epbits);
        }
    }
    return(list->num);
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
    strcpy(servers[n++],"89.248.160.245");
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
    return(n);
}

int32_t nn_addservers(int32_t priority,int32_t sock,char servers[][MAX_SERVERNAME],int32_t num)
{
    int32_t i; char endpoint[512]; struct endpoint epbits; uint32_t ipbits;
    if ( num > 0 && servers != 0 && nn_setsockopt(sock,NN_SOL_SOCKET,NN_SNDPRIO,&priority,sizeof(priority)) >= 0 )
    {
        for (i=0; i<num; i++)
        {
            if ( (ipbits= (uint32_t)calc_ipbits(servers[i])) == 0 )
            {
                printf("null ipbits.(%s)\n",servers[i]);
                continue;
            }
            epbits = calc_epbits("tcp",ipbits,SUPERNET.port + nn_portoffset(NN_REP),NN_REP);
            expand_epbits(endpoint,epbits);
            //printf("epbits.%llx ipbits.%x %s\n",*(long long *)&epbits,(uint32_t)ipbits,endpoint);
            if ( ismyaddress(servers[i]) == 0 && nn_connect(sock,endpoint) >= 0 )
            {
                printf("+%s ",endpoint);
                add_relay(&RELAYS.lb,epbits);
                //set_endpointaddr("ws",endpoint,servers[i],SUPERNET.port,NN_REP);
                //nn_connect(sock,endpoint);
            }
        }
        priority++;
    } else printf("error setting priority.%d (%s)\n",priority,nn_errstr());
    return(priority);
}

int32_t _lb_socket(int32_t retrymillis,char servers[][MAX_SERVERNAME],int32_t num,char backups[][MAX_SERVERNAME],int32_t numbacks,char failsafes[][MAX_SERVERNAME],int32_t numfailsafes)
{
    int32_t lbsock,timeout,priority = 1;
    if ( (lbsock= nn_socket(AF_SP,NN_REQ)) >= 0 )
    {
        //printf("!!!!!!!!!!!! lbsock.%d !!!!!!!!!!!\n",lbsock);
        if ( nn_setsockopt(lbsock,NN_SOL_SOCKET,NN_RECONNECT_IVL_MAX,&retrymillis,sizeof(retrymillis)) < 0 )
            printf("error setting NN_REQ NN_RECONNECT_IVL_MAX socket %s\n",nn_errstr());
        timeout = 10000;
        if ( nn_setsockopt(lbsock,NN_SOL_SOCKET,NN_RCVTIMEO,&timeout,sizeof(timeout)) < 0 )
            printf("error setting NN_SOL_SOCKET NN_RCVTIMEO socket %s\n",nn_errstr());
        timeout = 100;
        if ( nn_setsockopt(lbsock,NN_SOL_SOCKET,NN_SNDTIMEO,&timeout,sizeof(timeout)) < 0 )
            printf("error setting NN_SOL_SOCKET NN_SNDTIMEO socket %s\n",nn_errstr());
        priority = nn_addservers(priority,lbsock,servers,num);
        priority = nn_addservers(priority,lbsock,backups,numbacks);
        priority = nn_addservers(priority,lbsock,failsafes,numfailsafes);
    } else printf("error getting req socket %s\n",nn_errstr());
    //printf("RELAYS.lb.num %d\n",RELAYS.lb.num);
    return(lbsock);
}

int32_t nn_lbsocket(int32_t retrymillis,int32_t port)
{
    char Cservers[32][MAX_SERVERNAME],Bservers[32][MAX_SERVERNAME],failsafes[4][MAX_SERVERNAME];
    int32_t n,m,lbsock,numfailsafes = 0;
    strcpy(failsafes[numfailsafes++],"76.176.198.6");
    n = crackfoo_servers(Cservers,sizeof(Cservers)/sizeof(*Cservers),port);
    m = badass_servers(Bservers,sizeof(Bservers)/sizeof(*Bservers),port);
    //if ( europeflag != 0 )
    //    lbsock = nn_loadbalanced_socket(retrymillis,Bservers,m,Cservers,n,failsafes,numfailsafes);
    //else lbsock = nn_loadbalanced_socket(retrymillis,Cservers,n,Bservers,m,failsafes,numfailsafes);
    lbsock = _lb_socket(retrymillis,Bservers,m,Cservers,n,failsafes,numfailsafes);
    return(lbsock);
}

int32_t nn_directsocket(struct endpoint epbits)
{
    int32_t sock;
    if ( epbits.directind > 0 && (sock= RELAYS.directlinks[epbits.directind].sock) > 0 )
        return(sock);
    return(-1);
}

int32_t nn_directind()
{
    int32_t i;
    for (i=0; i<(int32_t)(sizeof(RELAYS.directlinks)/sizeof(*RELAYS.directlinks)); i++)
        if ( RELAYS.directlinks[i].sock == 0 )
        {
            RELAYS.directlinks[i].sock = -1;
            return(i);
        }
    for (i=0; i<(int32_t)(sizeof(RELAYS.directlinks)/sizeof(*RELAYS.directlinks)); i++)
        if ( RELAYS.directlinks[i].sock == -1 )
            return(i);
    return(0);
}

struct endpoint nn_directepbits(char *retbuf,char *transport,char *ipaddr,uint16_t port)
{
    struct endpoint epbits;
    if ( transport == 0 || transport[0] == 0 )
        transport = "tcp";
    if ( ipaddr == 0 || ipaddr[0] == 0 )
        sprintf(retbuf,"{\"error\":\"no ipaddr specified\"}");
    else if ( port == 0 )
        sprintf(retbuf,"{\"error\":\"no port specified\"}");
    else return(find_epbits(&RELAYS.pair,(uint32_t)calc_ipbits(ipaddr),port,NN_PAIR));
    memset(&epbits,0,sizeof(epbits));
    return(epbits);
}

void nn_startdirect(struct endpoint epbits,int32_t sock,char *handler)
{
    struct direct_connection *dc;
    dc = &RELAYS.directlinks[epbits.directind];
    if ( handler != 0 && handler[0] != 0 )
        safecopy(dc->handler,handler,sizeof(dc->handler));
    dc->sock = sock;
    dc->epbits = epbits;
}

int32_t nn_createsocket(char *endpoint,int32_t bindflag,char *name,int32_t type,uint16_t port,int32_t sendtimeout,int32_t recvtimeout)
{
    int32_t sock; struct endpoint epbits;
    if ( (sock= nn_socket(AF_SP,type)) < 0 )
        fprintf(stderr,"error getting socket %s\n",nn_errstr());
    if ( bindflag != 0 )
    {
        epbits = calc_epbits(SUPERNET.transport,0,port + nn_portoffset(type),type);
        expand_epbits(endpoint,epbits);
        //set_endpointaddr(SUPERNET.transport,endpoint,"*",SUPERNET.port,type);
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
    if ( sendtimeout > 0 && nn_setsockopt(sock,NN_SOL_SOCKET,NN_SNDTIMEO,&sendtimeout,sizeof(sendtimeout)) < 0 )
        fprintf(stderr,"error setting sendtimeout %s\n",nn_errstr());
    else if ( recvtimeout > 0 && nn_setsockopt(sock,NN_SOL_SOCKET,NN_RCVTIMEO,&recvtimeout,sizeof(recvtimeout)) < 0 )
        fprintf(stderr,"error setting sendtimeout %s\n",nn_errstr());
    else
    {
        fprintf(stderr,"nn_createsocket.(%s) %d\n",name,sock);
        return(sock);
    }
    return(-1);
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

char *nn_directconnect(char *transport,char *ipaddr,uint16_t port,char *handler)
{
    char endpoint[512],retbuf[1024];
    int32_t sock; uint16_t myport;
    struct direct_connection *dc;
    uint32_t ipbits; struct endpoint epbits;
    if ( is_ipaddr(ipaddr) == 0 )
        return(clonestr("{\"error\":\"illegal ipaddress\"}"));
    else if ( port == 0 )
        return(clonestr("{\"error\":\"myport field not set\"}"));
    if ( transport == 0 || transport[0] == 0 )
        transport = "tcp";
    ipbits = (uint32_t)calc_ipbits(ipaddr);
    epbits = find_epbits(&RELAYS.pair,ipbits,port,NN_PAIR);
    myport = SUPERNET.port + nn_portoffset(NN_PAIR) + epbits.directind - 1;
    if ( epbits.ipbits != 0 && epbits.directind != 0 )
    {
        dc = &RELAYS.directlinks[epbits.directind];
        if ( handler == 0 || strcmp(dc->handler,handler) == 0 )
        {
            expand_epbits(endpoint,epbits);
            sprintf(retbuf,"{\"result\":\"success\",\"handler\":\"%s\",\"mytransport\":\"%s\",\"direct\":\"%s\",\"myport\":%u,\"connected\":\"%s\",\"status\":\"already connected\"}",handler,SUPERNET.transport,SUPERNET.myipaddr,myport,endpoint);
            return(clonestr(retbuf));
        }
    }
    if ( is_ipaddr(SUPERNET.myipaddr) == 0 && SUPERNET.iamrelay != 0 )
        return(clonestr("{\"error\":\"dont know myipaddr\"}"));
    if ( (epbits.directind= nn_directind()) == 0 )
        return(clonestr("{\"error\":\"out of directsockets\"}"));
    myport = SUPERNET.port + nn_portoffset(NN_PAIR) + epbits.directind - 1;
    if ( (sock= nn_createsocket(endpoint,1,"direct",NN_PAIR,myport,10,1000)) >= 0 )
    {
        nn_startdirect(epbits,sock,handler);
        expand_epbits(endpoint,epbits);
        printf("try direct connect to (%s) using sock.%d\n",endpoint,sock);
        if ( nn_connect(sock,endpoint) < 0 )
            printf("error connecting to (%s) %s\n",endpoint,nn_errstr());
        else
        {
            add_relay(&RELAYS.pair,epbits);
            sprintf(retbuf,"{\"result\":\"success\",\"mytransport\":\"%s\",\"direct\":\"%s\",\"myport\":%u,\"connected\":\"%s\"}",SUPERNET.transport,SUPERNET.myipaddr,SUPERNET.port + nn_portoffset(NN_PAIR) + epbits.directind - 1,endpoint);
            return(clonestr(retbuf));
        }
        sprintf(retbuf,"{\"error\":\"connect failed\",\"dest\",\"%s\",\"%s\"}",endpoint,nn_errstr());
        return(clonestr(retbuf));
    } else return(clonestr("{\"error\":\"couldnt create direct socket\"}"));
}

int32_t add_relay_connections(char *domain,int32_t skiplb)
{
    uint32_t ipbits; int32_t n;
    //printf("add_relay_connections.(%s)\n",domain);
    if ( domain == 0 || is_ipaddr(domain) == 0 || ismyaddress(domain) != 0 )
        return(-1);
    ipbits = (uint32_t)calc_ipbits(domain);
    n = (RELAYS.lb.num + RELAYS.peer.num + RELAYS.sub.num);
    update_serverbits(&RELAYS.peer,"tcp",ipbits,SUPERNET.port + nn_portoffset(NN_SURVEYOR),NN_SURVEYOR);
    update_serverbits(&RELAYS.sub,"tcp",ipbits,SUPERNET.port + nn_portoffset(NN_PUB),NN_PUB);
    if ( SUPERNET.iamrelay != 0 )
    {
        if ( skiplb == 2 )
        {
            printf("BUS ");
            update_serverbits(&RELAYS.bus,"tcp",ipbits,SUPERNET.port + nn_portoffset(NN_BUS),NN_BUS);
        }
    }
    if ( skiplb == 0 )
        update_serverbits(&RELAYS.lb,"tcp",ipbits,SUPERNET.port + nn_portoffset(NN_REP),NN_REP);
    return((RELAYS.lb.num + RELAYS.peer.num + RELAYS.sub.num) > n);
}

void run_device(void *_args)
{
    struct nn_pollfd *pfds = _args;
    printf(">>>>>>>>>>>>>> RUNDEVICE.(%d <-> %d)\n",pfds[0].fd,pfds[1].fd);
    nn_device(pfds[0].fd,pfds[1].fd);
}

int32_t start_devices(int32_t type)
{
    int32_t i,j,n,err,numtypes,sock,portoffset,devicetypes[] = { NN_RESPONDENT }; //NN_REP, NN_PUB, NN_PULL };
    static struct nn_pollfd pfds[4][2];
    char bindaddr[128];
    numtypes = (int32_t)(sizeof(devicetypes)/sizeof(*devicetypes));
    memset(pfds,0xff,sizeof(pfds));
    devicetypes[0] = type;
    for (i=n=0; i<1; i++)
    {
        for (j=err=0; j<2; j++,n++)
        {
            type = (j == 0) ? devicetypes[i] : nn_oppotype(devicetypes[i]);
            portoffset = nn_portoffset(type);
            expand_epbits(bindaddr,calc_epbits(SUPERNET.transport,0,SUPERNET.port + nn_portoffset(type),type));
            if ( (sock= nn_socket(AF_SP_RAW,type)) < 0 )
                break;
            if ( (err= nn_bind(sock,bindaddr)) < 0 )
                break;
            printf("(%d) type.%d bindaddr.(%s) sock.%d\n",devicetypes[i],type,bindaddr,sock);
            pfds[i][j].fd = sock;
            pfds[i][j].events = NN_POLLIN | NN_POLLOUT;
        }
        portable_thread_create((void *)run_device,&pfds[i][0]);
        if ( j != 2 )
        {
            printf("error.%d launching relays at i.%d of %d j.%d %s\n",err,i,numtypes,j,nn_errstr());
            break;
        }
    }
    return(pfds[0][0].fd);
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
    return(0);
    //else printf("SUCCESS complete_relay.(%s) -> sock.%d %s\n",retstr,args->sock,args->name);
}

char *nn_publish(uint8_t *publishstr,int32_t len,int32_t nostr)
{
    int32_t sendlen = -1;
    char retbuf[1024];
    if ( publishstr != 0 )
    {
        if ( (sendlen= nn_send(RELAYS.pubsock,publishstr,len,0)) != len )
            printf("nn_publish warning: send.%d vs %d for (%s) sock.%d %s\n",sendlen,len,publishstr,RELAYS.pubsock,nn_errstr());
        else printf("nn_publish.(%s) %d bytes\n",len<1400?(char *)publishstr:"<big string>",len);
        sprintf(retbuf,"{\"result\":\"published\",\"len\":%d,\"sendlen\":%d,\"crc\":%u}",len,sendlen,_crc32(0,publishstr,len));
        if ( nostr != 0 )
            return(0);
    }
    else if ( nostr == 0 )
        strcpy(retbuf,"{\"error\":\"null publishstr\"}");
    return(clonestr(retbuf));
}

int32_t nn_directsend(struct endpoint epbits,uint8_t *msg,int32_t len)
{
    int32_t sock,i;
    sock = nn_directsocket(epbits);
    for (i=0; i<10; i++)
        if ( (nn_socket_status(sock,1) & NN_POLLOUT) != 0 )
            break;
    return(nn_send(sock,(char *)msg,len,0));
}

char *nn_direct(char *ipaddr,uint8_t *request,int32_t len)
{
    int32_t ipbits,sendlen; struct endpoint epbits;
    if ( is_ipaddr(ipaddr) != 0 )
    {
        ipbits = (uint32_t)calc_ipbits(ipaddr);
        epbits = find_epbits(&RELAYS.pair,ipbits,0,NN_PAIR);
        if ( epbits.ipbits != 0 )
        {
            len = (int32_t)request + 1;
            if ( (sendlen= nn_directsend(epbits,request,len)) != len )
            {
                printf("sendlen.%d vs len.%d %s\n",sendlen,len,nn_errstr());
                return(clonestr("{\"error\":\"error sending direct packet\"}"));
            }
            else return(clonestr("{\"result\":\"success\",\"details\":\"direct packet send\"}"));
        } else return(clonestr("{\"error\":\"cant find direct socket\"}"));
    } else return(clonestr("{\"error\":\"illegal ipaddr for direct connect\"}"));
}

char *nn_allpeers(uint8_t *_request,int32_t len,int32_t timeoutmillis,char *localresult)
{
    cJSON *item,*json,*array = 0;
    int32_t i,errs,numsuccess,sendlen,peersock,n = 0;
    double startmilli;
    char error[MAX_JSON_FIELD],result[MAX_JSON_FIELD],*request,*msg,*retstr;
    if ( timeoutmillis == 0 )
        timeoutmillis = 2000;
    if ( (peersock= RELAYS.querypeers) < 0 )
        return(clonestr("{\"error\":\"invalid peers socket\"}"));
    if ( nn_setsockopt(peersock,NN_SURVEYOR,NN_SURVEYOR_DEADLINE,&timeoutmillis,sizeof(timeoutmillis)) < 0 )
    {
        printf("error nn_setsockopt %d %s\n",peersock,nn_errstr());
        return(clonestr("{\"error\":\"setting NN_SURVEYOR_DEADLINE\"}"));
    }
    request = malloc(len + 512);
    memcpy(request,_request,len);
    add_standard_fields(request);
    if ( Debuglevel > 2 )
        printf("request_allpeers.(%s)\n",request);
    len = (int32_t)strlen(request) + 1;
    startmilli = milliseconds();
    errs = 0;
    if ( localresult != 0 && (item= cJSON_Parse(localresult)) != 0 )
    {
        if ( array == 0 )
            array = cJSON_CreateArray();
        cJSON_AddItemToObject(item,"locallag",cJSON_CreateNumber(milliseconds()-startmilli));
        cJSON_AddItemToArray(array,item);
        n++;
    }
    for (i=0; i<10; i++)
        if ( (nn_socket_status(peersock,1) & NN_POLLOUT) != 0 )
            break;
    numsuccess = n = 0;
    if ( (sendlen= nn_send(peersock,request,len,0)) == len )
    {
        while ( (len= nn_recv(peersock,&msg,NN_MSG,0)) > 0 )
        {
            n++;
            if ( (item= cJSON_Parse(msg)) != 0 )
            {
                copy_cJSON(error,cJSON_GetObjectItem(item,"error"));
                copy_cJSON(result,cJSON_GetObjectItem(item,"result"));
                if ( error[0] == 0 )//|| strcmp(error,"timeout") != 0 )
                {
                    if ( array == 0 )
                        array = cJSON_CreateArray();
                    cJSON_AddItemToObject(item,"lag",cJSON_CreateNumber(milliseconds()-startmilli));
                    cJSON_AddItemToArray(array,item);
                }
                else if ( strcmp(result,"success") == 0 )
                    numsuccess++;
                else errs++;
            } else errs++;
            nn_freemsg(msg);
        }
    } else printf("nn_allpeers: sendlen.%d vs len.%d\n",sendlen,len);
    if ( n == 0 )
    {
        free_json(array);
        free(request);
        return(clonestr("{\"error\":\"no responses\"}"));
    }
    json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"responses",array);
    cJSON_AddItemToObject(json,"success",cJSON_CreateNumber(numsuccess));
    cJSON_AddItemToObject(json,"err responses",cJSON_CreateNumber(errs));
    cJSON_AddItemToObject(json,"total",cJSON_CreateNumber(n));
    retstr = cJSON_Print(json);
    _stripwhite(retstr,' ');
    printf("globalrequest(%s) returned (%s) from n.%d respondents\n",request,retstr,n);
    free_json(json);
    free(request);
    return(retstr);
}

char *find_directconnect(cJSON *retjson)
{
    uint16_t port; char result[MAX_JSON_FIELD],otheripaddr[MAX_JSON_FIELD],othertransport[MAX_JSON_FIELD],*connectstr,*jsonstr = 0;
    copy_cJSON(result,cJSON_GetObjectItem(retjson,"result"));
    copy_cJSON(othertransport,cJSON_GetObjectItem(retjson,"mytransport"));
    copy_cJSON(otheripaddr,cJSON_GetObjectItem(retjson,"direct"));
    port = get_API_int(cJSON_GetObjectItem(retjson,"myport"),0);
    if ( strcmp(result,"success") == 0 )
    {
        if ( (connectstr= nn_directconnect(othertransport,otheripaddr,port,cJSON_str(cJSON_GetObjectItem(retjson,"myhandler")))) != 0 )
        {
            cJSON_AddItemToObject(retjson,"myconnect",cJSON_CreateString(connectstr));
            free(connectstr);
        } else cJSON_AddItemToObject(retjson,"myconnect",cJSON_CreateString("error"));
        jsonstr = cJSON_Print(retjson);
        _stripwhite(jsonstr,' ');
    }// else printf("result.(%s) != success\n",result);
    return(jsonstr);
}

char *nn_loadbalanced(uint8_t *data,int32_t len)
{
    cJSON *json,*retjson,*array,*item;
    char method[MAX_JSON_FIELD],*msg,*connectstr,*jsonstr = 0;
    int32_t sendlen,i,n,lbsock,recvlen = 0;
    if ( (lbsock= RELAYS.lb.sock) < 0 )
        return(clonestr("{\"error\":\"invalid load balanced socket\"}"));
    for (i=0; i<10; i++)
        if ( (nn_socket_status(lbsock,1) & NN_POLLOUT) != 0 )
            break;
    printf("NN_LBSEND.(%s)\n",data);
    if ( (sendlen= nn_send(lbsock,data,len,0)) == len )
    {
        for (i=0; i<1000; i++)
            if ( (nn_socket_status(lbsock,1) & NN_POLLIN) != 0 )
                break;
        if ( (recvlen= nn_recv(lbsock,&msg,NN_MSG,0)) > 0 )
        {
            jsonstr = clonestr((char *)msg);
            if ( (json= cJSON_Parse((char *)data)) != 0 )
            {
                copy_cJSON(method,cJSON_GetObjectItem(json,"method"));
                if ( strcmp(method,"direct") == 0 )
                {
                    connectstr = 0;
                    if ( (retjson= cJSON_Parse(jsonstr)) != 0 )
                    {
                        if ( (array= cJSON_GetObjectItem(retjson,"responses")) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
                        {
                            for (i=0; i<n; i++)
                            {
                                item = cJSON_GetArrayItem(array,i);
                                if ( (connectstr= find_directconnect(item)) != 0 )
                                      break;
                            }
                        }
                        else connectstr = find_directconnect(retjson);
                        if ( connectstr != 0 )
                        {
                            free(jsonstr);
                            jsonstr = connectstr;
                        }
                        free_json(retjson);
                    } else printf("cant parse retjson.(%s)\n",jsonstr);
                } //else printf("method.(%s) is not direct\n",method);
                free_json(json);
            } else printf("couldnt parse request (%s)\n",data);
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

uint8_t *replace_forwarder(char *pluginbuf,uint8_t *data,int32_t *datalenp)
{
    cJSON *json,*argjson,*second; char *plugin,*jsonstr; int32_t len,datalen,diff; uint8_t *ptr = data;
    pluginbuf[0] = 0;
    if ( (json= cJSON_Parse((char *)data)) != 0 )
    {
        if ( is_cJSON_Array(json) != 0 && cJSON_GetArraySize(json) == 2 )
        {
            argjson = cJSON_GetArrayItem(json,0);
            second = cJSON_GetArrayItem(json,1);
            ensure_jsonitem(second,"forwarder",SUPERNET.NXTADDR);
            datalen = (int32_t)strlen((char *)data) + 1;
            jsonstr = cJSON_Print(json), _stripwhite(jsonstr,' ');
            if ( (len= (int32_t)strlen(jsonstr)+1) == datalen )
                memcpy(data,jsonstr,len);
            else
            {
                diff = *datalenp - datalen;
                ptr = malloc(diff + len);
                memcpy(ptr,jsonstr,len);
                //printf("ptr.(%s) len.%d diff.%d datalen.%d\n",ptr,len,diff,datalen);
                if ( diff > 0 )
                    memcpy(&ptr[len],&data[datalen],diff);
                *datalenp = (diff + len);
            }
            free(jsonstr);
        }
        else argjson = json;
        if ( (plugin= cJSON_str(cJSON_GetObjectItem(argjson,"destplugin"))) != 0 || (plugin= cJSON_str(cJSON_GetObjectItem(argjson,"destagent"))) != 0 || (plugin= cJSON_str(cJSON_GetObjectItem(argjson,"plugin"))) != 0  || (plugin= cJSON_str(cJSON_GetObjectItem(argjson,"agent"))) != 0 )
        {
            strcpy(pluginbuf,plugin);
        }
        free_json(json);
    } else printf("cant parse.(%s)\n",data);
    return(ptr);
}

char *nn_lb_processor(struct relayargs *args,uint8_t *msg,int32_t len)
{
    char *nn_allpeers_processor(struct relayargs *args,uint8_t *msg,int32_t len);
    char *nn_pubsub_processor(struct relayargs *args,uint8_t *msg,int32_t len);
    char plugin[MAX_JSON_FIELD],*retstr = 0; uint8_t *buf;
    //printf("LB PROCESSOR.(%s)\n",msg);
    if ( (buf= replace_forwarder(plugin,msg,&len)) != 0 )
    {
        //printf("NEWLB.(%s)\n",buf);
        if ( strcmp(plugin,"subscriptions") == 0 )
            retstr = nn_pubsub_processor(args,msg,len);
        else if ( strcmp(plugin,"peers") == 0 )
            retstr = nn_allpeers_processor(args,msg,len);
        else retstr = plugin_method(0,-1,plugin,(char *)args,0,0,(char *)msg,len,1000);
    } else { retstr = clonestr("{\"error\":\"couldnt parse LB request\"}"); printf("%s\n",retstr); }
    if ( buf != msg )
        free(buf);
    return(retstr);
}

char *nn_pubsub_processor(struct relayargs *args,uint8_t *msg,int32_t len)
{
    char plugin[MAX_JSON_FIELD],method[MAX_JSON_FIELD],*retstr = 0; uint8_t *buf; cJSON *json;
    if ( (json= cJSON_Parse((char *)msg)) != 0 )
    {
        copy_cJSON(method,cJSON_GetObjectItem(json,"method"));
        free_json(json);
    } else method[0] = 0;
    if ( SUPERNET.gatewayid >= 0 && strcmp(method,"gotmsigaddr") == 0 )
        return(0);
    if ( (buf= replace_forwarder(plugin,msg,&len)) != 0 )
        retstr = plugin_method(0,-1,plugin,(char *)args,0,0,(char *)msg,len,1000);
    else retstr = clonestr((char *)msg);
    if ( buf != msg )
        free(buf);
    return(retstr);
}

char *nn_allpeers_processor(struct relayargs *args,uint8_t *msg,int32_t len)
{
    char plugin[MAX_JSON_FIELD],*retstr = 0; uint8_t *buf;
    if ( (buf= replace_forwarder(plugin,msg,&len)) != 0 )
        retstr = plugin_method(0,-1,plugin==0?"peers":plugin,(char *)args,0,0,(char *)msg,len,500);
    else retstr = clonestr("{\"error\":\"couldnt parse request\"}");
    if ( buf != msg )
        free(buf);
    return(retstr);
}

void nn_direct_processor(int32_t directind,uint8_t *msg,int32_t len)
{
    char *retstr; int32_t sock,retlen,sendlen;
    if ( SUPERNET.iamrelay != 0 )
    {
        if ( (retstr= nn_lb_processor(&RELAYS.args[0],msg,len)) != 0 )
        {
            sock = nn_directsocket(RELAYS.pair.connections[directind]);
            if ( sock >= 0 )
            {
                retlen = (int32_t)strlen(retstr) + 1;
                if ( (sendlen= nn_send(sock,retstr,retlen,0)) != retlen )
                    printf("sendlen.%d vs len.%d for direct response\n",sendlen,retlen);
            }
            else printf("RELAY DIRECTRETURN.(%s) from (%s) illegal sock.%d\n",retstr,(char *)msg,sock);
            free(retstr);
        }
    }
    else
    {
        if ( (retstr = plugin_method(0,0,"subscriptions","directmsg",0,0,(char *)msg,len,1000)) != 0 )
        {
            printf("DIRECTRETURN.(%s) from (%s)\n",retstr,(char *)msg);
            free(retstr);
        }
    }
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
    json = cJSON_CreateObject();
    if ( SUPERNET.iamrelay != 0 && SUPERNET.myipaddr[0] != 0 )
    {
        cJSON_AddItemToObject(json,"relay",cJSON_CreateString(SUPERNET.myipaddr));
        if ( RELAYS.bus.num > 0 )
            cJSON_AddItemToObject(json,"bus",relay_json(&RELAYS.bus));
    }
    if ( RELAYS.peer.num > 0 )
        cJSON_AddItemToObject(json,"peers",relay_json(&RELAYS.peer));
    if ( RELAYS.sub.num > 0 )
        cJSON_AddItemToObject(json,"subscriptions",relay_json(&RELAYS.sub));
    if ( RELAYS.lb.num > 0 )
        cJSON_AddItemToObject(json,"loadbalanced",relay_json(&RELAYS.lb));
    if ( RELAYS.pair.num > 0 )
        cJSON_AddItemToObject(json,"direct",relay_json(&RELAYS.pair));
    retstr = cJSON_Print(json);
    free_json(json);
    return(retstr);
}

void responseloop(void *_args)
{
    struct relayargs *args = _args;
    int32_t len; char *str,*msg,*retstr,*broadcaststr,*retstr2,*methodstr; cJSON *json,*argjson;
    if ( args->sock >= 0 )
    {
        fprintf(stderr,"respondloop.%s %d type.%d <- (%s).%d\n",args->name,args->sock,args->type,args->endpoint,nn_oppotype(args->type));
        while ( 1 )
        {
            if ( (len= nn_recv(args->sock,&msg,NN_MSG,0)) > 0 )
            {
                retstr = 0;
                if ( (json= cJSON_Parse((char *)msg)) != 0 )
                {
                    if ( is_cJSON_Array(json) != 0 && cJSON_GetArraySize(json) == 2 )
                        argjson = cJSON_GetArrayItem(json,0);
                    else argjson = json;
                    if ( (methodstr= cJSON_str(cJSON_GetObjectItem(argjson,"method"))) != 0 && strcmp(methodstr,"busdata") == 0 )
                    {
                        //printf("CALL BUSDATA PROCESSOR.(%s)\n",msg);
                        retstr = nn_busdata_processor(args,(uint8_t *)msg,len);
                    }
                    else
                    {
                        //if ( Debuglevel > 1 )
                        printf("%s RECV.%s (%s).%ld\n",methodstr,args->name,strlen(msg)<1400?msg:"<big message>",strlen(msg));
                        broadcaststr = cJSON_str(cJSON_GetObjectItem(argjson,"broadcast"));
                        if ( broadcaststr != 0 && strcmp(broadcaststr,"allpeers") == 0 )
                        {
                            cJSON_DeleteItemFromObject(argjson,"broadcast");
                            str = cJSON_Print(json), _stripwhite(str,' ');
                            len = (int32_t)strlen(str)+1;
                            retstr = (*args->commandprocessor)(args,(uint8_t *)str,len);
                            retstr2 = nn_allpeers((uint8_t *)str,len,1000,retstr), free(retstr), free(str);
                            retstr = retstr2;
                        }
                        else retstr = (*args->commandprocessor)(args,(uint8_t *)msg,(int32_t)strlen((char *)msg)+1);
                    }
                    free_json(json);
                } else printf("parse error.(%s)\n",(char *)msg);
                if ( retstr != 0 )
                {
                    complete_relay(args,retstr);
                    free(retstr);
                }
                nn_freemsg(msg);
            }// else fprintf(stderr,".");
        }
    } else printf("error getting socket type.%d %s\n",args->type,nn_errstr());
}

int32_t launch_responseloop(struct relayargs *args,char *name,int32_t type,int32_t bindflag,char *(*commandprocessor)(struct relayargs *,uint8_t *msg,int32_t len))
{
    if ( type != NN_RESPONDENT && type != NN_REP && type != NN_SUB && type != NN_BUS && type != NN_PULL )
    {
        printf("responder loop doesnt deal with type.%d %s\n",type,name);
        return(-1);
    }
    strcpy(args->name,name), args->type = type, args->commandprocessor = commandprocessor, args->bindflag = bindflag;
    if ( args->sendtimeout == 0 )
        args->sendtimeout = 10;
    if ( args->recvtimeout == 0 )
        args->recvtimeout = 10000;
    if ( (args->sock= nn_createsocket(args->endpoint,bindflag,name,type,SUPERNET.port,args->sendtimeout,args->recvtimeout)) >= 0 )
        portable_thread_create((void *)responseloop,args);
    else printf("error getting nn_createsocket.%d %s %s\n",type,name,nn_errstr());
    return(args->sock);
}

int32_t poll_direct(int32_t timeoutmillis)
{
    struct _relay_info *list = &RELAYS.pair;
    struct direct_connection *dc;
    int32_t inds[(sizeof(list->connections)/sizeof(*list->connections))];
    struct nn_pollfd pfds[(sizeof(list->connections)/sizeof(*list->connections))];
    int32_t len,tmp,sock,max,received=0,rc,i,n = 0;
    char *msg,*retstr;
    max = (int32_t)(sizeof(list->connections)/sizeof(*list->connections));
    memset(pfds,0xff,sizeof(*pfds) * max);
    memset(inds,0xff,sizeof(*inds) * max);
    for (i=0; i<list->num&&i<max; i++)
    {
        sock = nn_directsocket(list->connections[i]);
        if ( (pfds[n].fd= sock) >= 0 )
        {
            inds[n] = n;
            pfds[n].events = NN_POLLIN | NN_POLLOUT;
            n++;
        }
    }
    if ( n > 0 )
    {
        if ( (rc= nn_poll(pfds,n,timeoutmillis)) > 0 )
        {
            for (i=0; i<n; i++)
            {
                // printf("n.%d i.%d check socket.%d:%d revents.%d\n",n,i,pfd[i].fd,socks->all[i],pfd[i].revents);
                if ( (pfds[i].revents & NN_POLLIN) != 0 && (len= nn_recv(pfds[i].fd,&msg,NN_MSG,0)) > 0 )
                {
                    dc = &RELAYS.directlinks[list->connections[i].directind];
                    if ( dc->handler[0] == 0 || find_daemoninfo(&tmp,dc->handler,0,0) == 0 )
                        nn_direct_processor(inds[i],(uint8_t *)msg,len);
                    else
                    {
                        if ( (retstr = plugin_method(0,0,dc->handler,"directmsg",0,0,(char *)msg,len,1000)) != 0 )
                            free(retstr);
                    }
                    nn_freemsg(msg);
                    received++;
                    if ( Debuglevel > 1 )
                        printf("%d %.6f DIRECT RECEIVED.%d i.%d/%d (%s) -> origind.%d\n",received,milliseconds(),n,i,max,(char *)msg,inds[i]);
                }
            }
        }
        else if ( rc < 0 )
            printf("%s Error.%d polling %d of max.%d daemons [0] == %d\n",nn_strerror(nn_errno()),rc,n,max,pfds[0].fd);
    }
    //if ( n != 0 || received != 0 )
    //   printf("n.%d: received.%d\n",n,received);
    return(received);
}

void serverloop(void *_args)
{
    struct relayargs *peerargs,*lbargs,*arg;
    char endpoint[128],request[1024],ipaddr[64],*retstr;
    int32_t i,sendtimeout,recvtimeout,lbsock,bussock,pubsock,pushsock,peersock,n = 0;
  //start_devices(NN_RESPONDENT);
    sendtimeout = 10, recvtimeout = 10000;
    RELAYS.lb.mytype = NN_REQ, RELAYS.lb.desttype = nn_oppotype(RELAYS.lb.mytype);
    RELAYS.pair.mytype = NN_PAIR, RELAYS.pair.desttype = nn_oppotype(RELAYS.pair.mytype);
    RELAYS.bus.mytype = NN_BUS, RELAYS.bus.desttype = nn_oppotype(RELAYS.bus.mytype);
    RELAYS.peer.mytype = NN_SURVEYOR, RELAYS.peer.desttype = nn_oppotype(RELAYS.peer.mytype);
    RELAYS.sub.mytype = NN_SUB, RELAYS.sub.desttype = nn_oppotype(RELAYS.sub.mytype);
    lbargs = &RELAYS.args[n++];
    RELAYS.pullsock = RELAYS.pushsock = pushsock = -1;
    if ( 0 && RAMCHAINS.pullnode[0] != 0 )
    {
        //printf("my push endpoint.(%s)\n",endpoint), getchar();
        if ( strcmp(RAMCHAINS.pullnode,SUPERNET.myipaddr) == 0 )
        {
            expand_epbits(endpoint,calc_epbits("tcp",0,SUPERNET.port + nn_portoffset(NN_PULL),NN_PULL));
            //nn_connect(pushsock,endpoint);
            RELAYS.pullsock = nn_createsocket(endpoint,1,"NN_PULL",NN_PULL,SUPERNET.port,sendtimeout,recvtimeout);
        }
        else
        {
            expand_epbits(endpoint,calc_epbits("tcp",(uint32_t)calc_ipbits(RAMCHAINS.pullnode),SUPERNET.port + nn_portoffset(NN_PULL),NN_PULL));
            RELAYS.pushsock = pushsock = nn_createsocket(endpoint,0,"NN_PUSH",NN_PUSH,SUPERNET.port,sendtimeout,recvtimeout);
        }
    }
    RELAYS.querypeers = peersock = nn_createsocket(endpoint,1,"NN_SURVEYOR",NN_SURVEYOR,SUPERNET.port,sendtimeout,recvtimeout);
    peerargs = &RELAYS.args[n++], RELAYS.peer.sock = launch_responseloop(peerargs,"NN_RESPONDENT",NN_RESPONDENT,0,nn_allpeers_processor);
    pubsock = nn_createsocket(endpoint,1,"NN_PUB",NN_PUB,SUPERNET.port,sendtimeout,-1);
    RELAYS.sub.sock = launch_responseloop(&RELAYS.args[n++],"NN_SUB",NN_SUB,0,nn_pubsub_processor);
    RELAYS.lb.sock = lbargs->sock = lbsock = nn_lbsocket(10000,SUPERNET.port); // NN_REQ
    bussock = -1;
    busdata_init(sendtimeout,recvtimeout);
    if ( SUPERNET.iamrelay != 0 )
    {
        launch_responseloop(lbargs,"NN_REP",NN_REP,1,nn_lb_processor);
        bussock = launch_responseloop(&RELAYS.args[n++],"NN_BUS",NN_BUS,1,nn_busdata_processor);
    } else lbargs->commandprocessor = nn_lb_processor;
    RELAYS.bus.sock = bussock, RELAYS.pubsock = pubsock;
    for (i=0; i<n; i++)
    {
        arg = &RELAYS.args[i];
        arg->lbsock = lbsock;
        arg->bussock = bussock;
        arg->pubsock = pubsock;
        arg->peersock = peersock;
        arg->pushsock = pushsock;
    }
    for (i=0; i<RELAYS.lb.num; i++)
    {
        expand_ipbits(ipaddr,RELAYS.lb.connections[i].ipbits);
        add_relay_connections(ipaddr,2);
    }
    if ( SUPERNET.iamrelay != 0 )
    {
        if ( SUPERNET.hostname[0] != 0 || SUPERNET.myipaddr[0] != 0 )
        {
            sprintf(request,"{\"plugin\":\"relay\",\"method\":\"add\",\"hostnames\":[\"%s\"]}",SUPERNET.hostname[0]!=0?SUPERNET.hostname:SUPERNET.myipaddr);
            if ( (retstr= nn_loadbalanced((uint8_t *)request,(int32_t)strlen(request)+1)) != 0 )
            {
                printf("LB_RESPONSE.(%s)\n",retstr);
                free(retstr);
            }
        }
    } //else conv_busdata(&i,"{\"key\":\"foo\",\"data\":\"deadbeef\"}");
    if ( SUPERNET.gatewayid >= 0 )
    {
        int32_t make_MGWbus(uint16_t port,char *bindaddr,char serverips[MAX_MGWSERVERS][64],int32_t n);
        MGW.all.socks.both.bus = make_MGWbus(MGW.port,SUPERNET.myipaddr,MGW.serverips,SUPERNET.numgateways+1*0);
    }
    while ( 1 )
    {
        int32_t len; char retbuf[8192],*jsonstr; cJSON *json;
        int32_t poll_daemons();
        if ( poll_daemons() == 0 && poll_direct(1) == 0 && SUPERNET.APISLEEP > 0 )
            msleep(SUPERNET.APISLEEP);
        if ( SUPERNET.gatewayid >= 0 && (len= nn_recv(MGW.all.socks.both.bus,&jsonstr,NN_MSG,0)) > 0 )
        {
            int32_t mgw_processbus(char *retbuf,char *jsonstr,cJSON *json);
            if ( (json= cJSON_Parse(jsonstr)) != 0 )
            {
                mgw_processbus(retbuf,jsonstr,json);
                free_json(json);
            }
            //printf("MGW bus recv.%d json.%p\n",len,json);
            nn_freemsg(jsonstr);
        }
        busdata_poll();
    }
}

int32_t PLUGNAME(_process_json)(struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag)
{
    char *resultstr,*retstr = 0,*methodstr,*hostname,*myipaddr;
    int32_t i,n,count; uint32_t ipbits;
    cJSON *array;
    retbuf[0] = 0;
    printf("<<<<<<<<<<<< INSIDE PLUGIN! process %s (%s)\n",plugin->name,jsonstr);
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
        //printf("RELAYS.(%s)\n",jsonstr);
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
            if ( strcmp(methodstr,"add") == 0 && (array= cJSON_GetObjectItem(json,"hostnames")) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=count=0; i<n; i++)
                    if ( add_relay_connections(cJSON_str(cJSON_GetArrayItem(array,i)),1) > 0 )
                        count++;
                sprintf(retbuf,"{\"result\":\"relay added\",\"count\":%d}",count);
            }
            else if ( strcmp(methodstr,"list") == 0 )
                retstr = relays_jsonstr(jsonstr,json);
            else if ( strcmp(methodstr,"busdata") == 0 )
                retstr = busdata_sync(jsonstr);
            else if ( (myipaddr= cJSON_str(cJSON_GetObjectItem(json,"myipaddr"))) != 0 && is_ipaddr(myipaddr) != 0 )
            {
                if ( strcmp(methodstr,"direct") == 0 )
                {
                    retstr = nn_directconnect(cJSON_str(cJSON_GetObjectItem(json,"mytransport")),myipaddr,get_API_int(cJSON_GetObjectItem(json,"myport"),0),cJSON_str(cJSON_GetObjectItem(json,"myhandler")));
                }
                else if ( strcmp(methodstr,"join") == 0  )
                {
                    ipbits = (uint32_t)calc_ipbits(myipaddr);
                    if ( add_relay_connections(myipaddr,1) > 0 )
                    {
                        if ( SUPERNET.iamrelay != 0 )
                        {
                            if ( ipbits != 0 )
                            {
                                update_serverbits(&RELAYS.peer,"tcp",ipbits,SUPERNET.port + nn_portoffset(NN_SURVEYOR),NN_SURVEYOR);
                                update_serverbits(&RELAYS.sub,"tcp",ipbits,SUPERNET.port + nn_portoffset(NN_PUB),NN_PUB);
                                nn_send(RELAYS.bus.sock,jsonstr,(int32_t)strlen(jsonstr)+1,0);
                            }
                        }
                        sprintf(retbuf,"{\"result\":\"added ipaddr\"}");
                    } else sprintf(retbuf,"{\"result\":\"didnt add ipaddr, probably already there\"}");
                    if ( (hostname= cJSON_str(cJSON_GetObjectItem(json,"iamrelay"))) != 0 )
                        update_serverbits(&RELAYS.bus,"tcp",ipbits,SUPERNET.port + nn_portoffset(NN_BUS),NN_BUS);
                }
            }
        }
        if ( retstr != 0 )
        {
            strcpy(retbuf,retstr);
            free(retstr);
        }
    }
    return((int32_t)strlen(retbuf));
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    return(retcode);
}
#include "plugin777.c"
