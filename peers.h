//
//  peers.h
//  libjl777
//
//  Created by jl777 on 9/26/14.
//  Copyright (c) 2014 jl777. MIT license.
//

#ifndef libjl777_peers_h
#define libjl777_peers_h


/*int32_t process_PeerQ(void **ptrp,void *arg) // added when inbound transporter sequence is started
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
}*/

struct peerinfo *get_random_pserver()
{
    if ( Numpservers != 0 )
        return(Pservers[(rand() >> 8) % Numpservers]);
    return(0);
}

void set_peer_json(char *buf,char *NXTaddr,struct peerinfo *pi)
{
    char pubkey[128],srvipaddr[64],srvnxtaddr[64],coinsjson[1024];
    //struct peerinfo *pi = &pubnp->mypeerinfo;
    expand_ipbits(srvipaddr,pi->srv.ipbits);
    expand_nxt64bits(srvnxtaddr,pi->srvnxtbits);
    init_hexbytes_noT(pubkey,pi->srv.pubkey,sizeof(pi->srv.pubkey));
    _coins_jsonstr(coinsjson,pi->coins);
    sprintf(buf,"{\"requestType\":\"publishaddrs\",\"NXT\":\"%s\",\"Numpservers\":\"%d\",\"xorsum\":\"%u\",\"pubkey\":\"%s\",\"pubNXT\":\"%s\",\"pubBTCD\":\"%s\",\"pubBTC\":\"%s\",\"time\":%ld,\"srvNXTaddr\":\"%s\",\"srvipaddr\":\"%s\",\"srvport\":\"%d\"%s}",NXTaddr,Numpservers,calc_xorsum(Pservers,Numpservers),pubkey,NXTaddr,pi->pubBTCD,pi->pubBTC,time(NULL),srvnxtaddr,srvipaddr,pi->srv.supernet_port,coinsjson);
}

struct peerinfo *find_privacyserver(struct peerinfo *refpeer)
{
    struct peerinfo *peer = 0;
    int32_t i;
    for (i=0; i<Numpservers; i++)
    {
        if ( (peer= Pservers[i]) != 0 )
        {
            if ( refpeer->srvnxtbits == peer->srvnxtbits && refpeer->srv.ipbits == peer->srv.ipbits )//&& refpeer->srv.supernet_port == peer->srv.supernet_port )
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
    if ( dest != src )
        *dest = *src;
}

struct peerinfo *add_peerinfo(struct peerinfo *refpeer)
{
    static portable_mutex_t mutex;
    char NXTaddr[64],ipaddr[16];
    int32_t createdflag,isPserver;
    //struct coin_info *cp = get_coin_info("BTCD");
    struct NXT_acct *np = 0;
    struct peerinfo *peer = 0;
    struct pserver_info *pserver;
    if ( refpeer == 0 )
        return(0);
    if ( (peer= find_peerinfo(refpeer->pubnxtbits,refpeer->pubBTCD,refpeer->pubBTC)) != 0 )
        return(peer);
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
    if ( Numpeers == 0 )
        portable_mutex_init(&mutex);
    portable_mutex_lock(&mutex);
    copy_peerinfo(peer,refpeer);
    Peers = realloc(Peers,sizeof(*Peers) * (Numpeers + 1));
    Peers[Numpeers] = peer, Numpeers++;
    if ( (isPserver= is_privacyServer(peer)) != 0 )
    {
        if ( find_privacyserver(peer) == 0 )
        {
            Pservers = realloc(Pservers,sizeof(*Pservers) * (Numpservers + 1));
            Pservers[Numpservers] = peer, Numpservers++;
            expand_ipbits(ipaddr,peer->srv.ipbits);
            pserver = get_pserver(0,ipaddr,0,0);
            pserver->nxt64bits = peer->srvnxtbits;
            printf("ADDED privacyServer.%d: %s\n",Numpservers,ipaddr);
            //if ( cp != 0 && cp->myipaddr[0] != 0 )
            //    addto_hasips(1,get_pserver(0,cp->myipaddr,0,0),peer->srv.ipbits);
        }
    }
    portable_mutex_unlock(&mutex);
    printf("isPserver.%d add_peerinfo Numpeers.%d added %llu srv.%llu\n",isPserver,Numpeers,(long long)refpeer->pubnxtbits,(long long)refpeer->srvnxtbits);
    return(peer);
}

struct peerinfo *update_peerinfo(int32_t *createdflagp,struct peerinfo *refpeer)
{
    static uint8_t zeropubkey[crypto_box_PUBLICKEYBYTES];
    struct peerinfo *peer = 0;
    int32_t savedNumpeers = Numpeers;
    if ( memcmp(refpeer->srv.pubkey,zeropubkey,sizeof(refpeer->srv.pubkey)) == 0 )
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
    //printf("update_peerinfo Numpeers.%d added %llu srv.%llu\n",Numpeers,(long long)refpeer->pubnxtbits,(long long)refpeer->srvnxtbits);
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
        peer->srv.ipbits = calc_ipbits(srvipaddr);
        peer->srv.supernet_port = srvport;
    }
    peer->pubnxtbits = pubnxtbits;
    safecopy(peer->pubBTCD,pubBTCD,sizeof(peer->pubBTCD));
    safecopy(peer->pubBTC,pubBTC,sizeof(peer->pubBTC));
    if ( pubkey != 0 && pubkey[0] != 0 && decode_hex(peer->srv.pubkey,(int32_t)strlen(pubkey)/2,pubkey) != sizeof(peer->srv.pubkey) )
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
    memcpy(np->mypeerinfo.srv.pubkey,Global_mp->session_pubkey,sizeof(np->mypeerinfo.srv.pubkey));
    printf("(%s) [%s] need to implement query (and propagation) mechanism for pubkeys\n",destNXTaddr,np->H.U.NXTaddr);
}

char *getpubkey(char *NXTaddr,char *NXTACCTSECRET,char *pubaddr,char *destcoin)
{
    char buf[4096];
    struct NXT_acct *pubnp;
    printf("in getpubkey(%s)\n",pubaddr);
    pubnp = search_addresses(pubaddr);
    if ( pubnp != 0 )
    {
        set_peer_json(buf,NXTaddr,&pubnp->mypeerinfo);
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
            set_peer_json(buf,NXTaddr,&pubnp->mypeerinfo);
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

char *publishaddrs(struct sockaddr *prevaddr,uint64_t coins[4],char *NXTACCTSECRET,char *pubNXT,char *pubkeystr,char *BTCDaddr,char *BTCaddr,char *srvNXTaddr,char *srvipaddr,int32_t srvport,int32_t hasnum,uint32_t xorsum)
{
    int32_t createdflag,updatedflag = 0;
    struct NXT_acct *np;
    struct coin_info *cp;
    struct other_addr *op;
    struct pserver_info *pserver;
    struct peerinfo *refpeer,peer;
    char verifiedNXTaddr[64],mysrvNXTaddr[64];
    uint64_t pubnxtbits;
    cp = get_coin_info("BTCD");
    np = get_NXTacct(&createdflag,Global_mp,pubNXT);
    pubnxtbits = calc_nxt64bits(np->H.U.NXTaddr);
    if ( (refpeer= find_peerinfo(pubnxtbits,BTCDaddr,BTCaddr)) != 0 )
    {
        safecopy(refpeer->pubBTCD,BTCDaddr,sizeof(refpeer->pubBTCD));
        safecopy(refpeer->pubBTC,BTCaddr,sizeof(refpeer->pubBTC));
        updatedflag = update_pubkey(refpeer->srv.pubkey,pubkeystr);
        if ( srvport != 0 && srvport != BTCD_PORT )
            refpeer->srv.supernet_port = srvport;
        if ( srvipaddr != 0 && strcmp(srvipaddr,"127.0.0.1") != 0 )
            refpeer->srv.ipbits = calc_ipbits(srvipaddr);
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
        refpeer->srv.nxt64bits = pubnxtbits;
        if ( coins != 0 )
            memcpy(refpeer->coins,coins,sizeof(refpeer->coins));
        else if ( cp != 0 && cp->pubnxtbits == refpeer->pubnxtbits )
            memcpy(refpeer->coins,Global_mp->coins,sizeof(refpeer->coins));
        printf("set coins.%llx srv.nxt %llu\n",(long long)coins[0],(long long)refpeer->srv.nxt64bits);
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
        if ( cp != 0 && strcmp(NXTACCTSECRET,cp->srvNXTACCTSECRET) != 0 )
        {
            printf("%p.(%s) <- %p.(%s)\n",NXTACCTSECRET,NXTACCTSECRET,cp->srvNXTACCTSECRET,cp->srvNXTACCTSECRET);
            strcpy(NXTACCTSECRET,cp->srvNXTACCTSECRET);
        }
    }
    return(getpubkey(verifiedNXTaddr,NXTACCTSECRET,pubNXT,0));
}

char *publishPservers(struct sockaddr *prevaddr,char *NXTACCTSECRET,char *sender,int32_t firsti,int32_t hasnum,uint32_t *pservers,int32_t n,uint32_t xorsum)
{
    int32_t port,createdflag,i;
    char ipaddr[64],refipaddr[64];
    struct NXT_acct *np;
    struct pserver_info *pserver = 0;
    if ( pservers == 0 || n <= 0 )
        return(0);
    np = get_NXTacct(&createdflag,Global_mp,sender);
    if ( firsti == 0 )
        expand_ipbits(refipaddr,pservers[0]);
    else if ( np->mypeerinfo.srv.ipbits != 0 )
        expand_ipbits(refipaddr,np->mypeerinfo.srv.ipbits);
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
            expand_ipbits(srvipaddr,pserver->ipbits);
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
#endif
