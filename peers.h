//
//  peers.h
//  libjl777
//
//  Created by jl777 on 9/26/14.
//  Copyright (c) 2014 jl777. MIT license.
//

#ifndef libjl777_peers_h
#define libjl777_peers_h


#define PEER_SENT 1
#define PEER_RECV 2
#define PEER_PENDINGRECV 4
#define PEER_MASK (PEER_SENT|PEER_RECV|PEER_PENDINGRECV)
#define PEER_TIMEOUT 0x40
#define PEER_FINISHED 0x80
#define PEER_EXPIRATION (60 * 1000.)

struct peer_queue_entry
{
    struct peerinfo *peer,*hop;
    float startmilli,elapsed;
    uint8_t stateid,state;
};

queue_t P2P_Q;
struct pingpong_queue PeerQ;
struct peerinfo **Peers,**Pservers;
int32_t Numpeers,Numpservers,Num_in_whitelist;
uint32_t *SuperNET_whitelist;

int32_t on_SuperNET_whitelist(char *ipaddr)
{
    uint32_t i,ipbits;
    if ( Num_in_whitelist > 0 && SuperNET_whitelist != 0 )
    {
        ipbits = calc_ipbits(ipaddr);
        for (i=0; i<Num_in_whitelist; i++)
            if ( SuperNET_whitelist[i] == ipbits )
                return(1);
    }
    return(0);
}

int32_t add_SuperNET_whitelist(char *ipaddr)
{
    if ( on_SuperNET_whitelist(ipaddr) == 0 )
    {
        SuperNET_whitelist = realloc(SuperNET_whitelist,sizeof(*SuperNET_whitelist) * (Num_in_whitelist + 1));
        SuperNET_whitelist[Num_in_whitelist] = calc_ipbits(ipaddr);
        printf(">>>>>>>>>>>>>>>>>> add to SuperNET_whitelist[%d] (%s)\n",Num_in_whitelist,ipaddr);
        Num_in_whitelist++;
        return(1);
    }
    return(0);
}

int32_t process_PeerQ(void **ptrp,void *arg) // added when inbound transporter sequence is started
{
    struct peer_queue_entry *ptr = (*ptrp);
    struct peerinfo *peer,*hop;
    int32_t stateid;
    double timediff;
    peer = ptr->peer;
    hop = ptr->hop;
    timediff = (double)(milliseconds() - ptr->startmilli);
    stateid = ptr->stateid;
    if ( peer != 0 && hop != 0 && timediff >= -3000 && stateid < NUM_PEER_STATES )
    {
        if ( timediff > PEER_EXPIRATION )
        {
            if ( (peer->states[stateid]&PEER_FINISHED) != PEER_FINISHED || (hop->states[stateid]&PEER_FINISHED) != PEER_FINISHED )
            {
                peer->states[stateid] |= PEER_TIMEOUT;
                hop->states[stateid] |= PEER_TIMEOUT;
            }
            printf("process_peerQ: %s %llu state.%d -> %llu state.%d | elapsed.%.3f timediff.%.3f stateid.%d of %d\n",(timediff > PEER_EXPIRATION)?"EXPIRED!":"COMPLETED",(long long)peer->pubnxtbits,peer->states[stateid],(long long)hop->pubnxtbits,hop->states[stateid],ptr->elapsed,timediff,stateid,NUM_PEER_STATES);
            return(1);
        } else return(0);
    }
    else
    {
        printf("process_peerQ: illegal peer.%p or hop.%p | elapsed.%.3f timediff.%.3f stateid.%d of %d\n",peer,hop,hop->elapsed[stateid],timediff,stateid,NUM_PEER_STATES);
        return(-1);
    }
}

void update_peerstate(struct peerinfo *peer,struct peerinfo *hop,int32_t stateid,int32_t state)
{
    struct peer_queue_entry *ptr;
    if ( stateid >= NUM_PEER_STATES )
    {
        printf("illegal stateid.%d of %d: state,%d\n",stateid,NUM_PEER_STATES,state);
        return;
    }
    if ( state == PEER_SENT )
    {
        peer->states[stateid] = state;
        hop->states[stateid] = PEER_PENDINGRECV;
        ptr = calloc(1,sizeof(*ptr));
        ptr->peer = peer;
        ptr->hop = hop;
        ptr->stateid = stateid;
        ptr->state = state;
        ptr->startmilli = peer->startmillis[stateid] = milliseconds();
        hop->elapsed[stateid] = 0;
        queue_enqueue(&PeerQ.pingpong[0],ptr);
    }
    else if ( state == PEER_RECV )
    {
        if ( (peer->states[stateid]&PEER_MASK) == PEER_SENT && (hop->states[stateid]&PEER_MASK) == PEER_PENDINGRECV )
        {
            peer->states[stateid] |= PEER_FINISHED;
            hop->states[stateid] |= PEER_FINISHED;
            hop->elapsed[stateid] = (double)(milliseconds() - peer->startmillis[stateid]);
        }
        else printf("unexpected states[%d] NXT.%llu %d and hopNXT.%llu %d\n",stateid,(long long)peer->pubnxtbits,peer->states[stateid],(long long)hop->pubnxtbits,hop->states[stateid]);
    }
    else printf("update_peerstate unknown state.%d\n",state);
}

struct peerinfo *get_random_pserver()
{
    if ( Numpservers != 0 )
        return(Pservers[(rand() >> 8) % Numpservers]);
    return(0);
}

uint32_t addto_hasips(int32_t recalc_flag,struct pserver_info *pserver,uint32_t ipbits)
{
    int32_t i;
    uint32_t xorsum = 0;
    if ( ipbits == 0 )
        return(0);
    if ( pserver->hasips != 0 && pserver->numips > 0 )
    {
        for (i=0; i<pserver->numips; i++)
            if ( pserver->hasips[i] == ipbits )
                return(0);
    }
    pserver->hasips = realloc(pserver->hasips,sizeof(*pserver->hasips) + (pserver->numips + 1));
    pserver->hasips[pserver->numips] = ipbits;
    pserver->numips++;
    if ( recalc_flag != 0 )
    {
        for (i=0; i<pserver->numips; i++)
            xorsum ^= pserver->hasips[i];
        pserver->xorsum = xorsum;
        pserver->hasnum = pserver->numips;
    }
    return(xorsum);
}

void peer_link_ipaddr(struct pserver_info *pserver)
{
    int32_t i;
    struct peerinfo *peer;
    char srvNXTaddr[64];
    if ( pserver != 0 && Numpservers > 0 )
    {
        for (i=0; i<Numpservers; i++)
        {
            if ( (peer= Pservers[i]) != 0 )
            {
                if ( pserver->ipbits == peer->srvipbits )
                {
                    expand_nxt64bits(srvNXTaddr,peer->srvnxtbits);
                    printf("LINK.%p new ipaddr.(%s/%d %d) matches NXT.%s %d %d\n",pserver,pserver->ipaddr,pserver->p2pport,pserver->supernet_port,srvNXTaddr,peer->p2pport,peer->srvport);
                    if ( pserver->p2pport != 0 )
                        peer->p2pport = pserver->p2pport;
                    else if ( peer->p2pport != 0 )
                        pserver->p2pport = peer->p2pport;

                    if ( pserver->supernet_port != 0 )
                        peer->srvport = pserver->supernet_port;
                    else if ( peer->srvport != 0 )
                        pserver->supernet_port = peer->srvport;
               }
            }
        }
    }
}

int32_t is_privacyServer(struct peerinfo *peer)
{
    if ( peer->srvnxtbits == peer->pubnxtbits && peer->srvipbits != 0 && peer->srvport != 0 )
        return(1);
    return(0);
}

char *_coins_jsonstr(char *coinsjson,uint64_t coins[4])
{
    int32_t i,n = 0;
    char *str;
    if ( coins == 0 )
        return(0);
    strcpy(coinsjson,",\"coins\":[");
    for (i=0; i<4*64; i++)
        if ( (coins[i>>6] & (1L << (i&63))) != 0 )
        {
            str = coinid_str(i);
            if ( strcmp(str,ILLEGAL_COIN) != 0 )
            {
                if ( n++ != 0 )
                    strcat(coinsjson,", ");
                sprintf(coinsjson+strlen(coinsjson),"\"%s\"",str);
            }
        }
    if ( n == 0 )
        coinsjson[0] = coinsjson[1] = 0;
    else strcat(coinsjson,"]");
    return(coinsjson);
}

void set_peer_json(char *buf,char *NXTaddr,struct NXT_acct *pubnp)
{
    char pubkey[128],srvipaddr[64],srvnxtaddr[64],coinsjson[1024];
    struct peerinfo *pi = &pubnp->mypeerinfo;
    expand_ipbits(srvipaddr,pi->srvipbits);
    expand_nxt64bits(srvnxtaddr,pi->srvnxtbits);
    init_hexbytes(pubkey,pi->pubkey,sizeof(pi->pubkey));
    _coins_jsonstr(coinsjson,pi->coins);
    sprintf(buf,"{\"requestType\":\"publishaddrs\",\"NXT\":\"%s\",\"Numpservers\":\"%d\",\"xorsum\":\"%u\",\"pubkey\":\"%s\",\"pubNXT\":\"%s\",\"pubBTCD\":\"%s\",\"pubBTC\":\"%s\",\"time\":%ld,\"srvNXTaddr\":\"%s\",\"srvipaddr\":\"%s\",\"srvport\":\"%d\"%s}",NXTaddr,Numpservers,calc_xorsum(Pservers,Numpservers),pubkey,pubnp->H.U.NXTaddr,pi->pubBTCD,pi->pubBTC,time(NULL),srvnxtaddr,srvipaddr,pi->srvport,coinsjson);
}

cJSON *gen_pserver_json(struct pserver_info *pserver)
{
    cJSON *array,*json = cJSON_CreateObject();
    int32_t i;
    char ipaddr[64];
    uint32_t *ipaddrs;
    double millis = milliseconds();
    if ( pserver != 0 )
    {
        if ( (ipaddrs= pserver->hasips) != 0 && pserver->numips > 0 )
        {
            array = cJSON_CreateArray();
            for (i=0; i<pserver->numips; i++)
            {
                expand_ipbits(ipaddr,ipaddrs[i]);
                cJSON_AddItemToArray(array,cJSON_CreateString(ipaddr));
            }
            cJSON_AddItemToObject(json,"hasips",array);
        }
        cJSON_AddItemToObject(json,"hasnum",cJSON_CreateNumber(pserver->hasnum));
        cJSON_AddItemToObject(json,"xorsum",cJSON_CreateNumber(pserver->xorsum));
        if ( pserver->p2pport != 0 && pserver->p2pport != BTCD_PORT )
            cJSON_AddItemToObject(json,"p2p",cJSON_CreateNumber(pserver->p2pport));
        if ( pserver->supernet_port != 0 && pserver->supernet_port != SUPERNET_PORT )
            cJSON_AddItemToObject(json,"port",cJSON_CreateNumber(pserver->supernet_port));
        if ( pserver->numsent != 0 )
            cJSON_AddItemToObject(json,"sent",cJSON_CreateNumber(pserver->numsent));
        if ( pserver->sentmilli != 0 )
            cJSON_AddItemToObject(json,"lastsent",cJSON_CreateNumber((millis - pserver->sentmilli)/60000.));
        if ( pserver->numrecv != 0 )
            cJSON_AddItemToObject(json,"recv",cJSON_CreateNumber(pserver->numrecv));
        if ( pserver->recvmilli != 0 )
            cJSON_AddItemToObject(json,"lastrecv",cJSON_CreateNumber((millis - pserver->recvmilli)/60000.));
    }
    return(json);
}

cJSON *gen_peerinfo_json(struct peerinfo *peer)
{
    char srvipaddr[64],srvnxtaddr[64],numstr[64],pubNXT[64],hexstr[512],coinsjsonstr[1024];
    cJSON *coins,*json = cJSON_CreateObject();
    struct pserver_info *pserver;
    expand_ipbits(srvipaddr,peer->srvipbits);
    expand_nxt64bits(srvnxtaddr,peer->srvnxtbits);
    expand_nxt64bits(pubNXT,peer->pubnxtbits);
    if ( is_privacyServer(peer) != 0 )
    {
        cJSON_AddItemToObject(json,"is_privacyServer",cJSON_CreateNumber(1));
        cJSON_AddItemToObject(json,"pubNXT",cJSON_CreateString(srvnxtaddr));
        cJSON_AddItemToObject(json,"srvipaddr",cJSON_CreateString(srvipaddr));
        sprintf(numstr,"%d",peer->srvport);
        if ( peer->srvport != 0 && peer->srvport != SUPERNET_PORT )
            cJSON_AddItemToObject(json,"srvport",cJSON_CreateString(numstr));
        if ( peer->numsent != 0 )
            cJSON_AddItemToObject(json,"sent",cJSON_CreateNumber(peer->numsent));
        if ( peer->numrecv != 0 )
            cJSON_AddItemToObject(json,"recv",cJSON_CreateNumber(peer->numrecv));
        if ( (pserver= get_pserver(0,srvipaddr,0,0)) != 0 )
        {
            printf("%s pserver.%p\n",srvipaddr,pserver);
            cJSON_AddItemToObject(json,"pserver",gen_pserver_json(pserver));
        }
    }
    else cJSON_AddItemToObject(json,"pubNXT",cJSON_CreateString(pubNXT));
    init_hexbytes(hexstr,peer->pubkey,sizeof(peer->pubkey));
    cJSON_AddItemToObject(json,"pubkey",cJSON_CreateString(hexstr));
    if ( _coins_jsonstr(coinsjsonstr,peer->coins) != 0 )
    {
        coins = cJSON_Parse(coinsjsonstr+9);
        if ( coins != 0 )
            cJSON_AddItemToObject(json,"coins",coins);
        else printf("error parsing.(%s)\n",coinsjsonstr);
    }
    return(json);
}

cJSON *gen_Pservers_json(int32_t firstPserver)
{
    int32_t i,j;
    struct coin_info *cp = get_coin_info("BTCD");
    char srvipaddr[64];
    struct peerinfo *pserver;
    cJSON *json,*array;
    json = cJSON_CreateObject();
    array = cJSON_CreateArray();
    if ( Pservers != 0 && Numpservers >= firstPserver )
    {
        for (j=0,i=firstPserver; i<Numpservers&&j<32; i++,j++)
        {
            pserver = Pservers[i];
            expand_ipbits(srvipaddr,pserver->srvipbits);
            cJSON_AddItemToArray(array,cJSON_CreateString(srvipaddr));
        }
        if ( cp != 0 && strcmp(cp->privacyserver,"127.0.0.1") == 0 && cp->srvNXTADDR[0] != 0 )
        {
            cJSON_AddItemToObject(json,"requestType",cJSON_CreateString("publishPservers"));
            cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(cp->srvNXTADDR));
        }
        cJSON_AddItemToObject(json,"Pservers",array);
        cJSON_AddItemToObject(json,"firstPserver",cJSON_CreateNumber(firstPserver));
        cJSON_AddItemToObject(json,"Numpservers",cJSON_CreateNumber(Numpservers));
        cJSON_AddItemToObject(json,"xorsum",cJSON_CreateNumber(calc_xorsum(Pservers,Numpservers)));
    }
    return(json);
}

void set_pservers_json(char *buf,int32_t firstPserver)
{
    cJSON *json;
    char *jsonstr;
    if ( (json = gen_Pservers_json(firstPserver)) != 0 )
    {
        if ( (jsonstr = cJSON_Print(json)) != 0 )
        {
            stripwhite_ns(jsonstr,strlen(jsonstr));
            strcpy(buf,jsonstr);
            free(jsonstr);
        }
        free_json(json);
    }
}

cJSON *gen_peers_json(int32_t only_privacyServers)
{
    int32_t i,n;
    struct peerinfo **peers;
    cJSON *json,*array;
    //printf("inside gen_peer_json.%d\n",only_privacyServers);
    json = cJSON_CreateObject();
    array = cJSON_CreateArray();
    if ( only_privacyServers != 0 )
    {
        peers = Pservers;
        n = Numpservers;
    }
    else
    {
        peers = Peers;
        n = Numpeers;
    }
    if ( peers != 0 && n > 0 )
    {
        for (i=0; i<n; i++)
        {
            if ( peers[i] != 0 )
                cJSON_AddItemToArray(array,gen_peerinfo_json(peers[i]));
        }
        cJSON_AddItemToObject(json,"peers",array);
        //cJSON_AddItemToObject(json,"only_privacyServers",cJSON_CreateNumber(only_privacyServers));
        cJSON_AddItemToObject(json,"num",cJSON_CreateNumber(n));
        cJSON_AddItemToObject(json,"Numpservers",cJSON_CreateNumber(Numpservers));
    }
    return(json);
}

struct peerinfo *find_privacyserver(struct peerinfo *refpeer)
{
    struct peerinfo *peer = 0;
    int32_t i;
    for (i=0; i<Numpservers; i++)
    {
        if ( (peer= Pservers[i]) != 0 )
        {
            if ( refpeer->srvnxtbits == peer->srvnxtbits && refpeer->srvipbits == peer->srvipbits && refpeer->srvport == peer->srvport )
                return(peer);
        }
    }
    return(0);
}

struct peerinfo *find_peerinfo(uint64_t pubnxtbits,char *pubBTCD,char *pubBTC)
{
    struct peerinfo *peer = 0;
    int32_t i;
    for (i=0; i<Numpeers; i++)
    {
        if ( (peer= Peers[i]) != 0 )
        {
            if ( pubnxtbits != 0 && peer->pubnxtbits == pubnxtbits )
                return(peer);
            if ( pubBTCD != 0 && pubBTCD[0] != 0 && strcmp(peer->pubBTCD,pubBTCD) == 0 )
                return(peer);
            if ( pubBTC != 0 && pubBTC[0] != 0 && strcmp(peer->pubBTC,pubBTC) == 0 )
                return(peer);
        }
    }
    return(0);
}

void copy_peerinfo(struct peerinfo *dest,struct peerinfo *src)
{
    *dest = *src;
}

struct peerinfo *add_peerinfo(struct peerinfo *refpeer)
{
    void say_hello(struct NXT_acct *np,int32_t pservers_flag);
    char NXTaddr[64],ipaddr[16];
    int32_t createdflag,isPserver;
    struct coin_info *cp = get_coin_info("BTCD");
    struct NXT_acct *np = 0;
    struct peerinfo *peer = 0;
    if ( (peer= find_peerinfo(refpeer->pubnxtbits,refpeer->pubBTCD,refpeer->pubBTC)) != 0 )
        return(peer);
    Peers = realloc(Peers,sizeof(*Peers) * (Numpeers + 1));
    if ( refpeer->pubnxtbits != 0 )
    {
        expand_nxt64bits(NXTaddr,refpeer->pubnxtbits);
        np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
        peer = &np->mypeerinfo;
    }
    else
    {
        printf("FATAL: add_peerinfo without nxtbits (%s %llu %s)\n",refpeer->pubBTCD,(long long)refpeer->pubnxtbits,refpeer->pubBTC);
        peer = calloc(1,sizeof(*peer));
    }
    copy_peerinfo(peer,refpeer);
    Peers[Numpeers] = peer, Numpeers++;
    if ( (isPserver= is_privacyServer(peer)) != 0 )
    {
        if ( find_privacyserver(peer) == 0 )
        {
            Pservers = realloc(Pservers,sizeof(*Pservers) * (Numpservers + 1));
            Pservers[Numpservers] = peer, Numpservers++;
            expand_ipbits(ipaddr,peer->srvipbits);
            printf("ADDED privacyServer.%d\n",Numpservers);
            if ( cp != 0 && cp->myipaddr[0] != 0 )
                addto_hasips(1,get_pserver(0,cp->myipaddr,0,0),peer->srvipbits);
        }
    }
    printf("isPserver.%d add_peerinfo Numpeers.%d added %llu srv.%llu\n",isPserver,Numpeers,(long long)refpeer->pubnxtbits,(long long)refpeer->srvnxtbits);
    return(peer);
}

struct peerinfo *update_peerinfo(int32_t *createdflagp,struct peerinfo *refpeer)
{
    static uint8_t zeropubkey[crypto_box_PUBLICKEYBYTES];
    struct peerinfo *peer = 0;
    int32_t savedNumpeers = Numpeers;
    if ( memcmp(refpeer->pubkey,zeropubkey,sizeof(refpeer->pubkey)) == 0 )
    {
        printf("ERROR: update_peerinfo null pubkey not allowed: Numpeers.%d\n",Numpeers);
        return(0);
    }
    if ( refpeer->pubnxtbits == 0 && refpeer->pubBTCD[0] == 0 && refpeer->pubBTC[0] == 0 )
    {
        printf("ERROR: update_peerinfo all zero pubaddrs not supported: Numpeers.%d\n",Numpeers);
        return(0);
    }
    if ( (peer= add_peerinfo(refpeer)) == 0 )
    {
        printf("ERROR: update_peerinfo null ptr from add_peerinfo: Numpeers.%d\n",Numpeers);
        return(0);
    }
    printf("update_peerinfo Numpeers.%d added %llu srv.%llu\n",Numpeers,(long long)refpeer->pubnxtbits,(long long)refpeer->srvnxtbits);
    *createdflagp = (Numpeers != savedNumpeers);
    return(peer);
}

int32_t set_pubpeerinfo(char *srvNXTaddr,char *srvipaddr,int32_t srvport,struct peerinfo *peer,char *pubBTCD,char *pubkey,uint64_t pubnxtbits,char *pubBTC)
{
    memset(peer,0,sizeof(*peer));
    if ( srvNXTaddr != 0 )
        peer->srvnxtbits = calc_nxt64bits(srvNXTaddr);
    if ( srvport == 0 )
        srvport = SUPERNET_PORT;
    if ( srvipaddr != 0 )
    {
        peer->srvipbits = calc_ipbits(srvipaddr);
        peer->srvport = srvport;
    }
    peer->pubnxtbits = pubnxtbits;
    safecopy(peer->pubBTCD,pubBTCD,sizeof(peer->pubBTCD));
    safecopy(peer->pubBTC,pubBTC,sizeof(peer->pubBTC));
    if ( decode_hex(peer->pubkey,(int32_t)strlen(pubkey)/2,pubkey) != sizeof(peer->pubkey) )
    {
        printf("WARNING: (%s %llu %s) illegal pubkey.(%s)\n",pubBTCD,(long long)pubnxtbits,pubBTC,pubkey);
        return(-1);
    }
    return(0);
}

void query_pubkey(char *destNXTaddr,char *NXTACCTSECRET)
{
    struct NXT_acct *np;
    char NXTaddr[64];
    NXTaddr[0] = 0;
    np = find_NXTacct(destNXTaddr,NXTACCTSECRET);
    memcpy(np->mypeerinfo.pubkey,Global_mp->session_pubkey,sizeof(np->mypeerinfo.pubkey));
    printf("(%s) [%s] need to implement query (and propagation) mechanism for pubkeys\n",destNXTaddr,np->H.U.NXTaddr);
}

void update_routing_probs(struct peerinfo *peer,struct sockaddr *addr)
{
    //struct pserver_info *pserver;
    struct Uaddr *uaddr;
    int32_t i,port;//,createdflag;
    char ipaddr[64],srvip[64];
    port = extract_nameport(ipaddr,sizeof(ipaddr),(struct sockaddr_in *)addr);
    expand_ipbits(srvip,peer->srvipbits);
    //pserver = get_pserver(&createdflag,ipaddr,port,0);
    peer->numrecv++;
    if ( is_privacyServer(peer) != 0 && strcmp(ipaddr,srvip) == 0 && port == peer->srvport )
    {
        printf("direct comm from privacy server %s/%d vs (%s/%d) NXT.%llu | recv.%d send.%d\n",ipaddr,port,srvip,peer->srvport,(long long)peer->srvnxtbits,peer->numrecv,peer->numsent);
    }
    else
    {
        i = 0;
        uaddr = 0;
        if ( peer->numUaddrs > 0 )
        {
            for (; i<peer->numUaddrs; i++)
            {
                uaddr = &peer->Uaddrs[i];
                if ( memcmp(&uaddr->addr,addr,sizeof(uaddr->addr)) == 0 )
                    break;
            }
        }
        if ( i == peer->numUaddrs )
        {
            peer->Uaddrs = realloc(peer->Uaddrs,sizeof(*peer->Uaddrs) * (peer->numUaddrs+1));
            uaddr = &peer->Uaddrs[peer->numUaddrs++];
            memset(uaddr,0,sizeof(*uaddr));
            uaddr->addr = *addr;
            printf("add.%d Uaddr.(%s/%d) to %llu\n",peer->numUaddrs,ipaddr,port,(long long)peer->pubnxtbits);
        }
        if ( uaddr != 0 )
        {
            uaddr->numrecv++;
            uaddr->lastcontact = (uint32_t)time(NULL);
        } else printf("update_routing_probs: unexpected null uaddr for %s/%d!\n",ipaddr,port);
    }
}

static int _cmp_Uaddr(const void *a,const void *b)
{
#define val_a ((struct Uaddr *)a)->metric
#define val_b ((struct Uaddr *)b)->metric
	if ( val_b > val_a )
		return(1);
	else if ( val_b < val_a )
		return(-1);
	return(0);
#undef val_a
#undef val_b
}

double calc_Uaddr_metric(struct Uaddr *uaddr,uint32_t now)
{
    double elapsed,metric = 0.;
    if ( uaddr->lastcontact != 0 && uaddr->numrecv > 0 )
    {
        if ( uaddr->lastcontact <= now )
        {
            elapsed = ((double)(now - uaddr->lastcontact) + 1) / 60.;
            metric = (double)(uaddr->numsent + 1) / uaddr->numrecv;
            if ( metric > 1. )
                metric = 1.;
            metric /= sqrt(elapsed);
        }
    }
    return(metric);
}

int32_t sort_topaddrs(struct Uaddr **Uaddrs,int32_t max,struct peerinfo *peer)
{
    int32_t i,n;
    void **ptrs;
    uint32_t now = (uint32_t)time(NULL);
    if ( peer->numUaddrs == 0 || peer->Uaddrs == 0 )
        return(0);
    n = (peer->numUaddrs > max) ? max : peer->numUaddrs;
    if ( peer->numUaddrs > 1 )
    {
        ptrs = malloc(sizeof(*Uaddrs) * peer->numUaddrs);
        for (i=0; i<peer->numUaddrs; i++)
        {
            peer->Uaddrs[i].metric = calc_Uaddr_metric(ptrs[i],now);
            ptrs[i] = &peer->Uaddrs[i];
        }
        qsort(ptrs,peer->numUaddrs,sizeof(*Uaddrs),_cmp_Uaddr);
        for (i=0; i<n; i++)
        {
            if ( ((struct Uaddr *)ptrs[i])->metric <= 0. )
            {
                n = i;
                break;
            }
            Uaddrs[i] = ptrs[i];
        }
        free(ptrs);
    }
    else Uaddrs[0] = &peer->Uaddrs[0];
    return(n);
}

int32_t crcize(unsigned char *final,unsigned char *encoded,int32_t len)
{
    uint32_t i,n;//,crc = 0;
    uint32_t crc = _crc32(0,encoded,len);
    memcpy(final,&crc,sizeof(crc));
    memcpy(final + sizeof(crc),encoded,len);
    return(len + sizeof(crc));

    if ( len + sizeof(crc) < MAX_UDPLEN )
    {
        for (i=(uint32_t)(len + sizeof(crc)); i<MAX_UDPLEN; i++)
            final[i] = (rand() >> 8);
        n = MAX_UDPLEN - sizeof(crc);
    } else n = len;
    memcpy(final + sizeof(crc),encoded,len);
    crc = _crc32(0,final + sizeof(crc),n);
    memcpy(final,&crc,sizeof(crc));
    return(n + sizeof(crc));
}

int32_t is_encrypted_packet(unsigned char *tx,int32_t len)
{
    uint32_t crc,packet_crc;
    len -= sizeof(crc);
    if ( len <= 0 )
        return(0);
    memcpy(&packet_crc,tx,sizeof(packet_crc));
    tx += sizeof(crc);
    crc = _crc32(0,tx,len);
    //printf("got crc of %08x vx packet_crc %08x\n",crc,packet_crc);
    return(packet_crc == crc);
}

uint64_t route_packet(uv_udp_t *udp,char *hopNXTaddr,unsigned char *outbuf,int32_t len)
{
    unsigned char finalbuf[4096],hash[256>>3];
    char destip[64];
    struct sockaddr_in addr;
    struct Uaddr *Uaddrs[8];
    uint64_t txid = 0;
    int32_t createdflag,port,i,n;
    struct NXT_acct *np;
    memset(finalbuf,0,sizeof(finalbuf));
    np = get_NXTacct(&createdflag,Global_mp,hopNXTaddr);
    expand_ipbits(destip,np->mypeerinfo.srvipbits);
    len = crcize(finalbuf,outbuf,len);
    if ( len > sizeof(finalbuf) )
    {
        printf("sendmessage: len.%d > sizeof(finalbuf) %ld\n",len,sizeof(finalbuf));
        exit(-1);
    }
    if ( is_privacyServer(&np->mypeerinfo) != 0 )
    {
        printf("DIRECT udpsend {%s} to %s/%d finalbuf.%d\n",hopNXTaddr,destip,np->mypeerinfo.srvport,len);
        np->mypeerinfo.numsent++;
        if ( len <= MAX_UDPLEN )
        {
            uv_ip4_addr(destip,np->mypeerinfo.srvport,&addr);
            portable_udpwrite((struct sockaddr *)&addr,udp,finalbuf,len,ALLOCWR_ALLOCFREE);
        }
        else
        {
            if ( np->mypeerinfo.p2pport != 0 )
                sprintf(destip+strlen(destip),":%u",np->mypeerinfo.p2pport);
            call_SuperNET_broadcast(destip,(char *)finalbuf,len,0);
        }
    }
    else
    {
        memset(Uaddrs,0,sizeof(Uaddrs));
        n = sort_topaddrs(Uaddrs,(int32_t)(sizeof(Uaddrs)/sizeof(*Uaddrs)),&np->mypeerinfo);
        printf("n.%d hopNXTaddr.(%s) narrowcast %s:%d %d via p2p\n",len,hopNXTaddr,destip,np->mypeerinfo.srvport,len);
        if ( n > 0 )
        {
            for (i=0; i<n; i++)
            {
                np->mypeerinfo.numsent++;
                Uaddrs[i]->numsent++;
                if ( len <= MAX_UDPLEN )
                    portable_udpwrite((struct sockaddr *)&Uaddrs[i]->addr,udp,finalbuf,len,ALLOCWR_ALLOCFREE);
                else
                {
                    port = extract_nameport(destip,sizeof(destip),(struct sockaddr_in *)&Uaddrs[i]->addr);
                    if ( port != 0 )
                        sprintf(destip+strlen(destip),":%u",port);
                    call_SuperNET_broadcast(destip,(char *)finalbuf,len,0);
                }
            }
        }
    }
    calc_sha256(0,hash,finalbuf,len);
    txid = calc_txid(hash,sizeof(hash));
    return(txid);
}

char *getpubkey(char *NXTaddr,char *NXTACCTSECRET,char *pubaddr,char *destcoin)
{
    char buf[4096];
    struct NXT_acct *pubnp;
    printf("in getpubkey(%s)\n",pubaddr);
    pubnp = search_addresses(pubaddr);
    if ( pubnp != 0 )
    {
        set_peer_json(buf,NXTaddr,pubnp);
        return(clonestr(buf));
    } else return(clonestr("{\"error\":\"cant find pubaddr\"}"));
}

char *sendpeerinfo(int32_t pservers_flag,char *hopNXTaddr,char *NXTaddr,char *NXTACCTSECRET,char *destaddr,char *destcoin)
{
    char buf[4096];
    struct NXT_acct *pubnp,*destnp;
    printf("in sendpeerinfo(%s)\n",destaddr);
    pubnp = search_addresses(NXTaddr);
    destnp = search_addresses(destaddr);
    if ( pubnp != 0 && destnp != 0 )
    {
        if ( pservers_flag <= 0 )
            set_peer_json(buf,NXTaddr,pubnp);
        else set_pservers_json(buf,pservers_flag-1);
        if ( hopNXTaddr != 0 )
        {
            printf("SENDPEERINFO.%d >>>>>>>>>> (%s)\n",pservers_flag,buf);
            hopNXTaddr[0] = 0;
            return(send_tokenized_cmd(hopNXTaddr,0,NXTaddr,NXTACCTSECRET,buf,destnp->H.U.NXTaddr));
        } else return(0);
    } else return(clonestr("{\"error\":\"sendpeerinfo cant find pubaddr\"}"));
}

void say_hello(struct NXT_acct *np,int32_t pservers_flag)
{
    struct coin_info *cp = get_coin_info("BTCD");
    char srvNXTaddr[64],hopNXTaddr[64],*retstr;
    struct NXT_acct *hopnp;
    int32_t createflag;
    if ( np == 0 )
        return;
    //printf("in say_hello.cp %p\n",cp);
    expand_nxt64bits(srvNXTaddr,cp->srvpubnxtbits);
    if ( (retstr= sendpeerinfo(pservers_flag,hopNXTaddr,srvNXTaddr,cp->srvNXTACCTSECRET,np->H.U.NXTaddr,0)) != 0 )
    {
        printf("say_hello.(%s)\n",retstr);
        if ( hopNXTaddr[0] != 0 )
        {
            hopnp = get_NXTacct(&createflag,Global_mp,hopNXTaddr);
            //update_peerstate(&np->mypeerinfo,&hopnp->mypeerinfo,PEER_HELLOSTATE,PEER_SENT);
        }
        else printf("say_hello no hopNXTaddr?\n");
        free(retstr);
    } else printf("say_hello error sendpeerinfo?\n");
}

void ack_hello(struct NXT_acct *np,struct sockaddr *prevaddr)
{
    struct coin_info *cp = get_coin_info("BTCD");
    char srvNXTaddr[64];
    expand_nxt64bits(srvNXTaddr,cp->srvpubnxtbits);
    printf("ack_hello to %s\n",np->H.U.NXTaddr);
}

void ask_pservers(struct NXT_acct *np)
{
    struct coin_info *cp = get_coin_info("BTCD");
    char srvNXTaddr[64],hopNXTaddr[64],*retstr,cmd[512];
    printf("in ask_pservers cp %p\n",cp);
    if ( cp != 0 )
    {
        expand_nxt64bits(srvNXTaddr,cp->srvpubnxtbits);
        hopNXTaddr[0] = 0;
        sprintf(cmd,"{\"requestType\":\"getPservers\",\"NXT\":\"%s\"}",srvNXTaddr);
        if ( (retstr= send_tokenized_cmd(hopNXTaddr,0,srvNXTaddr,cp->srvNXTACCTSECRET,cmd,np->H.U.NXTaddr)) != 0 )
            free(retstr);
    }
}

uint64_t broadcast_publishpacket(char *ip_port)
{
    char cmd[MAX_JSON_FIELD*4],packet[MAX_JSON_FIELD*4];
    int32_t len,createdflag;
    struct NXT_acct *np;
    struct coin_info *cp = get_coin_info("BTCD");
    if ( cp != 0 )
    {
        np = get_NXTacct(&createdflag,Global_mp,cp->srvNXTADDR);
        set_peer_json(cmd,np->H.U.NXTaddr,np);
        len = construct_tokenized_req(packet,cmd,cp->srvNXTACCTSECRET);
        return(call_SuperNET_broadcast(ip_port,packet,len+1,PUBADDRS_MSGDURATION));
    }
    printf("ERROR: broadcast_publishpacket null cp\n");
    return(0);
}

int32_t update_pserver_xorsum(struct NXT_acct *othernp,int32_t hasnum,uint32_t xorsum)
{
    int32_t retflag = 0;
    struct coin_info *cp;
    struct pserver_info *mypserver = 0;
    return(0);
    if ( mypserver == 0 )
    {
        cp = get_coin_info("BTCD");
        if ( cp != 0 && cp->myipaddr[0] != 0 )
            mypserver = get_pserver(0,cp->myipaddr,0,0);
    }
    if ( mypserver != 0 )
        
    {
        if ( othernp->pserver_pending == 0 && (xorsum == 0 || hasnum > mypserver->hasnum || (hasnum == mypserver->hasnum && xorsum != mypserver->xorsum)) )
        {
            othernp->pserver_pending = 1;
            ask_pservers(othernp), retflag |= 1;
        }
        if (  (mypserver->hasnum > hasnum || (mypserver->hasnum == hasnum && mypserver->xorsum != xorsum)) )
            say_hello(othernp,1), retflag |= 2;

        printf("update_pserver_xorsum retflag.%d\n",retflag);
        return(1);
    }
    return(retflag);
}

char *publishaddrs(struct sockaddr *prevaddr,uint64_t coins[4],char *NXTACCTSECRET,char *pubNXT,char *pubkeystr,char *BTCDaddr,char *BTCaddr,char *srvNXTaddr,char *srvipaddr,int32_t srvport,int32_t hasnum,uint32_t xorsum)
{
    int32_t createdflag,updatedflag = 0;
    struct NXT_acct *np;
    struct coin_info *cp;
    struct other_addr *op;
    struct pserver_info *pserver;
    struct peerinfo *refpeer,peer;
    char verifiedNXTaddr[64],mysrvNXTaddr[64];
    unsigned char pubkey[crypto_box_PUBLICKEYBYTES];
    uint64_t pubnxtbits;
    cp = get_coin_info("BTCD");
    np = get_NXTacct(&createdflag,Global_mp,pubNXT);
    pubnxtbits = calc_nxt64bits(np->H.U.NXTaddr);
    if ( (refpeer= find_peerinfo(pubnxtbits,BTCDaddr,BTCaddr)) != 0 )
    {
        safecopy(refpeer->pubBTCD,BTCDaddr,sizeof(refpeer->pubBTCD));
        safecopy(refpeer->pubBTC,BTCaddr,sizeof(refpeer->pubBTC));
        if ( pubkeystr != 0 && pubkeystr[0] != 0 )
        {
            memset(pubkey,0,sizeof(pubkey));
            decode_hex(pubkey,(int32_t)sizeof(pubkey),pubkeystr);
            if ( memcmp(refpeer->pubkey,pubkey,sizeof(refpeer->pubkey)) != 0 )
            {
                memcpy(refpeer->pubkey,pubkey,sizeof(refpeer->pubkey));
                updatedflag = 1;
            }
        }
        if ( srvport != 0 )
            refpeer->srvport = srvport;
        if ( srvipaddr != 0 && strcmp(srvipaddr,"127.0.0.1") != 0 )
            refpeer->srvipbits = calc_ipbits(srvipaddr);
        if ( srvNXTaddr != 0 && srvNXTaddr[0] != 0 )
            refpeer->srvnxtbits = calc_nxt64bits(srvNXTaddr);
        printf("prev.%p found %s and updated.%d %s | coins.%p\n",prevaddr,pubNXT,updatedflag,np->H.U.NXTaddr,coins);
    }
    else
    {
        set_pubpeerinfo(srvNXTaddr,srvipaddr,srvport,&peer,BTCDaddr,pubkeystr,pubnxtbits,BTCaddr);
        refpeer = update_peerinfo(&createdflag,&peer);
        printf("prev.%p created path for (%s) | coins.%p\n",prevaddr,pubNXT,coins);
    }
    if ( refpeer != 0 )
    {
        if ( coins != 0 )
            memcpy(refpeer->coins,coins,sizeof(refpeer->coins));
        else if ( cp != 0 && cp->pubnxtbits == refpeer->pubnxtbits )
            memcpy(refpeer->coins,Global_mp->coins,sizeof(refpeer->coins));
        printf("set coins.%llx\n",(long long)coins[0]);
    }
    if ( srvipaddr != 0 && strcmp(srvipaddr,"127.0.0.1") != 0 && (pserver= get_pserver(0,srvipaddr,0,0)) != 0 )
    {
        pserver->hasnum = hasnum;
        pserver->xorsum = xorsum;
    }
    copy_peerinfo(&np->mypeerinfo,refpeer);
    //printf("in secret.(%s) publishaddrs.(%s) np.%p %llu\n",NXTACCTSECRET,pubNXT,np,(long long)np->H.nxt64bits);
    if ( BTCDaddr[0] != 0 )
    {
        //safecopy(np->BTCDaddr,BTCDaddr,sizeof(np->BTCDaddr));
        op = MTadd_hashtable(&createdflag,Global_mp->otheraddrs_tablep,BTCDaddr),op->nxt64bits = np->H.nxt64bits;
        //printf("op.%p for %s\n",op,BTCDaddr);
    }
    if ( BTCaddr != 0 && BTCaddr[0] != 0 )
    {
        //safecopy(np->BTCaddr,BTCaddr,sizeof(np->BTCaddr));
        op = MTadd_hashtable(&createdflag,Global_mp->otheraddrs_tablep,BTCaddr),op->nxt64bits = np->H.nxt64bits;
    }
    if ( prevaddr != 0 )
    {
        if ( updatedflag != 0 )
            say_hello(np,0);
        return(0);
    }
    verifiedNXTaddr[0] = 0;
    np = find_NXTacct(verifiedNXTaddr,NXTACCTSECRET);
    expand_nxt64bits(mysrvNXTaddr,np->mypeerinfo.srvnxtbits);
    if ( strcmp(srvNXTaddr,pubNXT) == 0 )
    {
        strcpy(verifiedNXTaddr,srvNXTaddr);
        if ( cp != 0 )
            strcpy(NXTACCTSECRET,cp->srvNXTACCTSECRET);
        //broadcast_publishpacket(0);
    }
    return(getpubkey(verifiedNXTaddr,NXTACCTSECRET,pubNXT,0));
}

int32_t pserver_canhop(struct pserver_info *pserver,char *hopNXTaddr)
{
    int32_t createdflag,i = -1;
    uint32_t *hasips;
    struct NXT_acct *np;
    struct peerinfo *peer;
    np = get_NXTacct(&createdflag,Global_mp,hopNXTaddr);
    peer = &np->mypeerinfo;
    if ( is_privacyServer(peer) != 0 && pserver != 0 && pserver->numips > 0 && (hasips= pserver->hasips) != 0 )
    {
        for (i=0; i<pserver->numips; i++)
            if ( hasips[i] == peer->srvipbits )
            {
                char ipaddr[16];
                expand_ipbits(ipaddr,hasips[i]);
                printf(">>>>>>>>>>> HASIP.%s in slot %d of %d\n",ipaddr,i,pserver->numips);
                return(i);
            }
    }
    return(i);
}

char *publishPservers(struct sockaddr *prevaddr,char *NXTACCTSECRET,char *sender,int32_t firsti,int32_t hasnum,uint32_t *pservers,int32_t n,uint32_t xorsum)
{
    int32_t i,port,createdflag;
    char ipaddr[64],refipaddr[64];
    struct NXT_acct *np;
    struct pserver_info *pserver = 0;
    if ( pservers == 0 || n <= 0 )
        return(0);
    np = get_NXTacct(&createdflag,Global_mp,sender);
    if ( firsti == 0 )
        expand_ipbits(refipaddr,pservers[0]);
    else if ( np->mypeerinfo.srvipbits != 0 )
        expand_ipbits(refipaddr,np->mypeerinfo.srvipbits);
    else return(0);
    pserver = get_pserver(0,refipaddr,0,0);
    pserver->hasnum = hasnum;
    pserver->xorsum = xorsum;
    for (i=0; i<n; i++)
        addto_hasips(0,pserver,pservers[i]);
    port = extract_nameport(ipaddr,sizeof(ipaddr),(struct sockaddr_in *)prevaddr);
    printf(">>>>>>>>>>>> publishPservers from sender.(%s) prevaddr.%s/%d first.%d hasnum.%d n.%d\n",sender,ipaddr,port,firsti,hasnum,n);
    return(0);
}

void every_minute()
{
    int32_t i,createdflag;
    char ipaddr[64],ip_port[64],NXTaddr[64];
    struct NXT_acct *np;
    struct coin_info *cp;
    struct peerinfo *peer;
    struct pserver_info *pserver,*mypserver = 0;
    cp = get_coin_info("BTCD");
    //printf("<<<<<<<<<<<<< EVERY_MINUTE\n");
    if ( cp != 0 && cp->myipaddr[0] != 0 )
        mypserver = get_pserver(0,cp->myipaddr,0,0);
    if ( Num_in_whitelist > 0 && SuperNET_whitelist != 0 )
    {
        for (i=0; i<Num_in_whitelist; i++)
        {
            expand_ipbits(ipaddr,SuperNET_whitelist[i]);
            pserver = get_pserver(0,ipaddr,0,0);
            peer_link_ipaddr(pserver);

            //printf("(%s) numrecv.%d numsent.%d\n",ipaddr,pserver->numrecv,pserver->numsent);
            if ( pserver->numrecv == 0 )//&& pserver->numsent < 3 )
            {
                sprintf(ip_port,"%s:%d",ipaddr,pserver->p2pport!=0?pserver->p2pport:BTCD_PORT);
                printf(">>>>>>>>>>>>> A every_minute(%s) sent.%d recv.%d\n",ip_port,pserver->numsent,pserver->numrecv);
                broadcast_publishpacket(ip_port);
                pserver->numsent++;
            }
        }
    }
    if ( mypserver != 0 && Numpservers > 0 && Pservers != 0 )
    {
        for (i=0; i<Numpservers; i++)
        {
            if ( (peer= Pservers[i]) != 0 && peer->srvnxtbits != 0 && peer->srvipbits != 0 )
            {
                expand_ipbits(ipaddr,peer->srvipbits);
                pserver = get_pserver(0,ipaddr,0,0);
                if ( peer->numrecv == 0 || pserver->hasnum < mypserver->hasnum || (pserver->hasnum == mypserver->hasnum && pserver->xorsum != mypserver->xorsum) )
                {
                    expand_ipbits(ipaddr,peer->srvipbits);
                    sprintf(ip_port,"%s:%d",ipaddr,pserver->p2pport!=0?pserver->p2pport:BTCD_PORT);
                    printf(">>>>>>>>>>>>>>> B every_minute(%s) sent.%d recv.%d\n",ip_port,pserver->numsent,pserver->numrecv);
                    broadcast_publishpacket(ip_port);
                    expand_nxt64bits(NXTaddr,peer->srvnxtbits);
                    np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
                    say_hello(np,0);
                    say_hello(np,1);
                }
                /*expand_nxt64bits(NXTaddr,peer->srvnxtbits);
                np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
                if ( pserver->hasnum > mypserver->hasnum || (pserver->hasnum == mypserver->hasnum && pserver->xorsum != mypserver->xorsum) )
                {
                    expand_ipbits(ipaddr,pserver->ipbits);
                    printf(">>>>>>>>>>>>> ASK CUZ %s has more than us %d.%u vs mine %d.%u\n",ipaddr,pserver->hasnum,pserver->xorsum,mypserver->hasnum,mypserver->xorsum);
                    ask_pservers(np); // when other node has more pservers than we do
                }*/
            }
        }
    }
}

#endif
