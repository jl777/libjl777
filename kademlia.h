//
//  kademlia.h
//  libjl777
//
//  Created by jl777 on 10/3/14.
//  Copyright (c) 2014 jl777. MIT license.
//

#ifndef libjl777_kademlia_h
#define libjl777_kademlia_h

#define KADEMLIA_ALPHA 7
#define NODESTATS_EXPIRATION 600
#define KADEMLIA_BUCKET_REFRESHTIME 3600
#define KADEMLIA_NUMBUCKETS ((int)(sizeof(K_buckets)/sizeof(K_buckets[0])))
#define KADEMLIA_NUMK ((int)(sizeof(K_buckets[0])/sizeof(K_buckets[0][0])))
struct nodestats *K_buckets[64][6];
long Kbucket_updated[KADEMLIA_NUMBUCKETS];

struct kademlia_store
{
    uint64_t keyhash;
    char *value;
    uint32_t laststored;
};
struct kademlia_store K_store[1000];

struct kademlia_store *kademlia_getstored(uint64_t keyhash,int32_t storeflag)
{
    int32_t i,oldi = -1;
    uint32_t oldest = 0;
    for (i=0; i<(int)(sizeof(K_store)/sizeof(*K_store)); i++)
    {
        if ( K_store[i].keyhash == 0 || K_store[i].keyhash == keyhash )
        {
            if ( storeflag == 0 && K_store[i].keyhash == 0 )
                return(0);
            K_store[i].keyhash = keyhash;
            return(&K_store[i]);
        }
        if ( oldest == 0 || K_store[i].laststored < oldest )
        {
            oldest = K_store[i].laststored;
            oldi = i;
        }
    }
    if ( storeflag != 0 )
    {
        K_store[oldi].keyhash = keyhash;
        if ( K_store[oldi].value != 0 )
            free(K_store[oldi].value), K_store[oldi].value = 0;
        return(&K_store[oldi]);
    }
    return(0);
}

int32_t calc_np_dist(struct NXT_acct *np,struct NXT_acct *destnp)
{
    uint64_t a,b;
    a = calc_nxt64bits(np->H.U.NXTaddr);
    b = calc_nxt64bits(destnp->H.U.NXTaddr);
    return(bitweight(a ^ b));
}

int32_t verify_addr(struct sockaddr *addr,char *refipaddr,int32_t refport)
{
    int32_t port;
    char ipaddr[64];
    port = extract_nameport(ipaddr,sizeof(ipaddr),(struct sockaddr_in *)addr);
    if ( strcmp(ipaddr,refipaddr) != 0 )//|| refport != port )
        return(-1);
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

uint64_t *sort_all_buckets(int32_t *nump,uint64_t hash)
{
    uint64_t *sortbuf = 0;
    struct nodestats *stats;
    int32_t i,j,n;
    sortbuf = calloc(2 * sizeof(*sortbuf),KADEMLIA_NUMBUCKETS * KADEMLIA_NUMK);
    for (i=n=0; i<KADEMLIA_NUMBUCKETS; i++)
    {
        for (j=0; j<KADEMLIA_NUMK; j++)
        {
            if ( (stats= K_buckets[i][j]) == 0 )
                break;
            sortbuf[n<<1] = bitweight(stats->nxt64bits ^ hash);// + ((stats->gotencrypted == 0) ? 64 : 0);
            sortbuf[(n<<1) + 1] = stats->nxt64bits;
        }
    }
    *nump = n;
    if ( n == 0 )
    {
        free(sortbuf);
        sortbuf = 0;
    }
    else
    {
        sortbuf = realloc(sortbuf,n * sizeof(uint64_t) * 2);
        sort64s(sortbuf,n,sizeof(*sortbuf)*2);
    }
    return(sortbuf);
}

int32_t calc_bestdist(uint64_t keyhash)
{
    int32_t i,j,n,dist,bestdist = 10000;
    struct nodestats *stats;
    if ( kademlia_getstored(keyhash,0) != 0 )
        return(0);
    for (i=n=0; i<KADEMLIA_NUMBUCKETS; i++)
    {
        for (j=0; j<KADEMLIA_NUMK; j++)
        {
            if ( (stats= K_buckets[i][j]) == 0 )
                break;
            dist = bitweight(stats->nxt64bits ^ keyhash);
            if ( dist < bestdist )
                bestdist = dist;
        }
    }
    return(bestdist);
}

void kademlia_update_info(struct peerinfo *peer,char *ipaddr,int32_t port,char *pubkeystr,uint32_t lastcontact)
{
    uint32_t ipbits = calc_ipbits(ipaddr);
    if ( peer->srv.ipbits == 0 )
    {
        peer->srv.ipbits = ipbits;
        peer->srv.supernet_port = port;
        if ( pubkeystr != 0 && pubkeystr[0] != 0 && update_pubkey(peer->srv.pubkey,pubkeystr) != 0 )
            peer->srv.lastcontact = lastcontact;
    }
    else if ( peer->srv.ipbits == ipbits )
    {
        if ( peer->srv.supernet_port == 0 )
            peer->srv.supernet_port = port;
        if ( pubkeystr != 0 && pubkeystr[0] != 0 && update_pubkey(peer->srv.pubkey,pubkeystr) != 0 )
            peer->srv.lastcontact = lastcontact;
    }
    else printf("kademlia_update_info: NXT.%llu ipbits %u != %u\n",(long long)peer->srv.nxt64bits,peer->srv.ipbits,ipbits);
}

uint64_t _send_kademlia_cmd(struct pserver_info *pserver,char *cmdstr,char *NXTACCTSECRET)
{
    int32_t len = (int32_t)strlen(cmdstr);
    char _tokbuf[4096];
    uint64_t txid;
    len = construct_tokenized_req(_tokbuf,cmdstr,NXTACCTSECRET);
    printf(">>>>>>>> directsend.[%s]\n",_tokbuf);
    txid = directsend_packet(pserver,_tokbuf,len);
    return(txid);
}

uint64_t send_kademlia_cmd(uint64_t nxt64bits,struct pserver_info *pserver,char *kadcmd,char *NXTACCTSECRET,char *key,char *value)
{
    int32_t createdflag;
    struct NXT_acct *np;
    struct coin_info *cp = get_coin_info("BTCD");
    char pubkeystr[1024],ipaddr[64],cmdstr[2048],verifiedNXTaddr[64],destNXTaddr[64];
    if ( NXTACCTSECRET[0] == 0 || cp == 0 )
        return(0);
    init_hexbytes(pubkeystr,Global_mp->loopback_pubkey,sizeof(Global_mp->loopback_pubkey));
    verifiedNXTaddr[0] = 0;
    find_NXTacct(verifiedNXTaddr,NXTACCTSECRET);
    if ( pserver == 0 )
    {
        expand_nxt64bits(destNXTaddr,nxt64bits);
        np = get_NXTacct(&createdflag,Global_mp,destNXTaddr);
        expand_ipbits(ipaddr,np->mypeerinfo.srv.ipbits);
        pserver = get_pserver(0,ipaddr,0,0);
        if ( pserver->nxt64bits == 0 )
            pserver->nxt64bits = nxt64bits;
    } else nxt64bits = pserver->nxt64bits;
    sprintf(cmdstr,"{\"requestType\":\"%s\",\"NXT\":\"%s\",\"time\":%ld,\"pubkey\":\"%s\",\"ipaddr\":\"%s\",\"port\":%d",kadcmd,verifiedNXTaddr,(long)time(NULL),pubkeystr,cp->myipaddr,SUPERNET_PORT);
    if ( key != 0 && key[0] != 0 )
        sprintf(cmdstr+strlen(cmdstr),",\"key\":\"%s\"",key);
    if ( value != 0 && value[0] != 0 )
        sprintf(cmdstr+strlen(cmdstr),",\"value\":\"%s\"",value);
    strcat(cmdstr,"}");
    return(_send_kademlia_cmd(pserver,cmdstr,NXTACCTSECRET));
}

char *kademlia_ping(struct sockaddr *prevaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,char *pubkey,char *ipaddr,int32_t port,char *destip)
{
    uint64_t txid = 0;
    char retstr[1024];
    struct nodestats *stats;
    struct pserver_info *pserver;
    if ( prevaddr == 0 ) // user invoked
    {
        if ( destip != 0 && destip[0] != 0 )
        {
            printf("inside ping.(%s)\n",destip);
            pserver = get_pserver(0,destip,0,0);
            stats = get_nodestats(pserver->nxt64bits);
            if ( stats != 0 )
            {
                stats->pingmilli = milliseconds();
                stats->numpings++;
            }
            txid = send_kademlia_cmd(0,get_pserver(0,destip,0,0),"ping",NXTACCTSECRET,0,0);
            sprintf(retstr,"{\"result\":\"kademlia_ping to %s\",\"txid\",\"%llu\"}",destip,(long long)txid);
        }
        else sprintf(retstr,"{\"error\":\"kademlia_ping no destip\"}");
    }
    else // sender ping'ed us
    {
        if ( verify_addr(prevaddr,ipaddr,port) < 0 )
            sprintf(retstr,"{\"error\":\"kademlia_pong from %s doesnt verify (%s/%d)\"}",sender,ipaddr,port);
        else
        {
            txid = send_kademlia_cmd(0,get_pserver(0,ipaddr,0,0),"pong",NXTACCTSECRET,0,0);
            sprintf(retstr,"{\"result\":\"kademlia_pong to (%s/%d)\",\"txid\",\"%llu\"}",ipaddr,port,(long long)txid);
        }
    }
    return(clonestr(retstr));
}

char *kademlia_pong(struct sockaddr *prevaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,char *pubkey,char *ipaddr,uint16_t port)
{
    char retstr[1024];
    struct nodestats *stats;
    stats = get_nodestats(calc_nxt64bits(sender));
    // all the work is already done in update_Kbucket
    if ( stats != 0 )
    {
        stats->pongmilli = milliseconds();
        stats->pingpongsum += (stats->pongmilli - stats->pingmilli);
        stats->numpongs++;
        sprintf(retstr,"{\"result\":\"kademlia_pong from NXT.%s (%s/%d) %.3f millis | numpings.%d numpongs.%d ave %.3f\"}",sender,ipaddr,port,stats->pongmilli-stats->pingmilli,stats->numpings,stats->numpongs,(2*stats->pingpongsum)/(stats->numpings+stats->numpongs+1));
    }
    else sprintf(retstr,"{\"result\":\"kademlia_pong from NXT.%s (%s/%d)\"}",sender,ipaddr,port);
    printf("PONG.(%s)\n",retstr);
    return(clonestr(retstr));
}

char *kademlia_store(struct sockaddr *prevaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,char *pubkey,char *key,char *value)
{
    char retstr[1024];
    uint64_t *sortbuf,keybits,txid = 0;
    int32_t i,n,createdflag;
    struct NXT_acct *keynp;
    struct kademlia_store *sp;
    if ( key == 0 || key[0] == 0 || value == 0 || value[0] == 0 )
        return(0);
    keybits = calc_nxt64bits(key);
    sortbuf = sort_all_buckets(&n,keybits);
    if ( sortbuf != 0 )
    {
        if ( prevaddr == 0 )
        {
            for (i=0; i<n&&i<KADEMLIA_NUMK; i++)
                txid = send_kademlia_cmd(sortbuf[(i<<1) + 1],0,"store",NXTACCTSECRET,key,value);
        }
        else
        {
            sp = kademlia_getstored(keybits,1);
            if ( sp->value != 0 )
                free(sp->value);
            sp->value = clonestr(value);
            sp->laststored = (uint32_t)time(NULL);
            keynp = get_NXTacct(&createdflag,Global_mp,key);
            if ( keynp->bestbits != 0 )
            {
                txid = send_kademlia_cmd(keynp->bestdist,0,"store",NXTACCTSECRET,key,value);
                printf("Bestdist.%d bestbits.%llu\n",keynp->bestdist,(long long)keynp->bestbits);
                keynp->bestbits = 0;
                keynp->bestdist = 0;
            }
        }
        sprintf(retstr,"{\"result\":\"kademlia_store key.(%s) value.(%s) -> txid.%llu\"}",key,value,(long long)txid);
        free(sortbuf);
    }
    else sprintf(retstr,"{\"error\":\"kademlia_store key.(%s) no peers\"}",key);
    return(clonestr(retstr));
}

char *kademlia_havenode(int32_t valueflag,struct sockaddr *prevaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,char *pubkey,char *key,char *value)
{
    char retstr[1024],ipaddr[MAX_JSON_FIELD],destNXTaddr[MAX_JSON_FIELD],pubkeystr[MAX_JSON_FIELD],portstr[MAX_JSON_FIELD],lastcontactstr[MAX_JSON_FIELD];
    int32_t i,n,createdflag,dist;
    uint32_t lastcontact,port;
    uint64_t keyhash,txid = 0;
    cJSON *array,*item;
    struct coin_info *cp = get_coin_info("BTCD");
    struct pserver_info *pserver = 0;
    struct NXT_acct *destnp,*keynp;
    keyhash = calc_nxt64bits(key);
    if ( key != 0 && key[0] != 0 && value != 0 && value[0] != 0 && (array= cJSON_Parse(value)) != 0 )
    {
        if ( prevaddr != 0 )
        {
            extract_nameport(ipaddr,sizeof(ipaddr),(struct sockaddr_in *)prevaddr);
            pserver = get_pserver(0,ipaddr,0,0);
        } else if ( cp != 0 ) pserver = get_pserver(0,cp->myipaddr,0,0);
        keynp = get_NXTacct(&createdflag,Global_mp,key);
        if ( is_cJSON_Array(array) != 0 )
        {
            n = cJSON_GetArraySize(array);
            for (i=0; i<n; i++)
            {
                item = cJSON_GetArrayItem(array,i);
                if ( is_cJSON_Array(item) != 0 && cJSON_GetArraySize(item) == 5 )
                {
                    copy_cJSON(destNXTaddr,cJSON_GetArrayItem(item,0));
                    copy_cJSON(ipaddr,cJSON_GetArrayItem(item,1));
                    if ( ipaddr[0] != 0 )
                        addto_hasips(1,pserver,calc_ipbits(ipaddr));
                    copy_cJSON(portstr,cJSON_GetArrayItem(item,2));
                    copy_cJSON(pubkeystr,cJSON_GetArrayItem(item,3));
                    copy_cJSON(lastcontactstr,cJSON_GetArrayItem(item,4));
                    port = (uint32_t)atol(portstr);
                    lastcontact = (uint32_t)atol(lastcontactstr);
                    if ( destNXTaddr[0] != 0 && ipaddr[0] != 0 )
                    {
                        destnp = get_NXTacct(&createdflag,Global_mp,destNXTaddr);
                        kademlia_update_info(&destnp->mypeerinfo,ipaddr,port,pubkeystr,lastcontact);
                        dist = calc_np_dist(keynp,destnp);
                        if ( dist < keynp->bestdist )
                        {
                            keynp->bestdist = dist;
                            keynp->bestbits = calc_nxt64bits(destnp->H.U.NXTaddr);
                        }
                        if ( dist < calc_bestdist(keyhash) )
                            txid = send_kademlia_cmd(calc_nxt64bits(destnp->H.U.NXTaddr),0,valueflag!=0?"findvalue":"findnode",NXTACCTSECRET,key,0);
                    }
                }
            }
        }
        free_json(array);
    }
    sprintf(retstr,"{\"result\":\"kademlia_havenode from NXT.%s (%s:%s)\"}",sender,key,value);
    return(clonestr(retstr));
}

char *kademlia_find(char *cmd,struct sockaddr *prevaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,char *pubkey,char *key)
{
    static unsigned char zerokey[crypto_box_PUBLICKEYBYTES];
    char retstr[1024],pubkeystr[256],numstr[64],ipaddr[64],destNXTaddr[64],*value;
    uint64_t keyhash,*sortbuf,txid = 0;
    int32_t i,n,createdflag;
    struct NXT_acct *keynp;
    struct NXT_acct *destnp;
    cJSON *array,*item;
    struct kademlia_store *sp;
    struct nodestats *stats;
    if ( key != 0 && key[0] != 0 )
    {
        keyhash = calc_nxt64bits(key);
        if ( strcmp(cmd,"findvalue") == 0 )
        {
            sp = kademlia_getstored(keyhash,0);
            if ( sp != 0 && sp->value != 0 )
            {
                if ( prevaddr != 0 )
                    txid = send_kademlia_cmd(calc_nxt64bits(sender),0,"store",NXTACCTSECRET,key,sp->value);
                sprintf(retstr,"{\"result\":\"%s\"}",sp->value);
                return(clonestr(retstr));
            }
        }
        sortbuf = sort_all_buckets(&n,keyhash);
        if ( sortbuf != 0 )
        {
            if ( prevaddr == 0 ) // user invoked
            {
                keynp = get_NXTacct(&createdflag,Global_mp,key);
                keynp->bestdist = 10000;
                keynp->bestbits = 0;
                for (i=0; i<n&&i<KADEMLIA_ALPHA; i++)
                    txid = send_kademlia_cmd(sortbuf[(i<<1) + 1],0,cmd,NXTACCTSECRET,key,0);
            }
            else // need to respond to sender
            {
                array = cJSON_CreateArray();
                for (i=0; i<n&&i<KADEMLIA_NUMK; i++)
                {
                    expand_nxt64bits(destNXTaddr,sortbuf[(i<<1) + 1]);
                    destnp = get_NXTacct(&createdflag,Global_mp,destNXTaddr);
                    stats = &destnp->mypeerinfo.srv;
                    item = cJSON_CreateArray();
                    cJSON_AddItemToArray(item,cJSON_CreateString(destNXTaddr));
                    if ( memcmp(stats->pubkey,zerokey,sizeof(stats->pubkey)) != 0 )
                    {
                        init_hexbytes(pubkeystr,stats->pubkey,sizeof(stats->pubkey));
                        cJSON_AddItemToArray(item,cJSON_CreateString(pubkeystr));
                    }
                    if ( stats->ipbits != 0 )
                    {
                        expand_ipbits(ipaddr,stats->ipbits);
                        cJSON_AddItemToArray(item,cJSON_CreateString(ipaddr));
                        sprintf(numstr,"%d",stats->supernet_port==0?SUPERNET_PORT:stats->supernet_port);
                        cJSON_AddItemToArray(item,cJSON_CreateString(numstr));
                        if ( stats->lastcontact != 0 )
                        {
                            sprintf(numstr,"%u",stats->lastcontact);
                            cJSON_AddItemToArray(item,cJSON_CreateString(numstr));
                        }
                    }
                    cJSON_AddItemToArray(array,item);
                }
                value = cJSON_Print(array);
                free_json(array);
                stripwhite(value,strlen(value));
                txid = send_kademlia_cmd(calc_nxt64bits(sender),0,strcmp(cmd,"findnode")==0?"havenode":"havenodeB",NXTACCTSECRET,key,value);
                free(value);
            }
        }
        free(sortbuf);
    }
    sprintf(retstr,"{\"result\":\"kademlia_findnode txid.%llu\"}",(long long)txid);
    return(clonestr(retstr));
}

int32_t kademlia_pushstore(uint64_t refbits,uint64_t newbits)
{
    uint64_t txid,keyhash;
    char key[64];
    struct coin_info *cp = get_coin_info("BTCD");
    int32_t dist,refdist,i,n = 0;
    if ( cp == 0 )
        return(0);
    for (i=0; i<(int)(sizeof(K_store)/sizeof(*K_store)); i++)
    {
        if ( (keyhash= K_store[i].keyhash) == 0 )
            break;
        if ( K_store[i].value != 0 && K_store[i].value[0] != 0 )
        {
            refdist = bitweight(refbits ^ keyhash);
            dist = bitweight(newbits ^ keyhash);
            if ( dist < refdist )
            {
                expand_nxt64bits(key,keyhash);
                txid = send_kademlia_cmd(newbits,0,"store",cp->srvNXTACCTSECRET,key,K_store[i].value);
            }
        }
    }
    return(n);
}

void update_Kbucket(struct nodestats *buckets[],int32_t n,struct nodestats *stats)
{
    int32_t j,k,matchflag = -1;
    uint64_t txid;
    char ipaddr[64];
    struct coin_info *cp = get_coin_info("BTCD");
    struct nodestats *eviction;
    if ( stats->ipbits == 0 || stats->nxt64bits == 0 )
        return;
    expand_ipbits(ipaddr,stats->ipbits);
    for (j=n-1; j>=0; j--)
    {
        if ( buckets[j] != 0 && buckets[j]->eviction == stats )
        {
            printf("Got pong back from eviction candidate in slot.%d\n",j);
            buckets[j] = buckets[j]->eviction;
            break;
        }
    }
    for (j=0; j<n; j++)
    {
        if ( buckets[j] == 0 )
        {
            if ( matchflag < 0 )
            {
                buckets[j] = stats;
                printf("APPEND: bucket[%d] <- %llu %s then call pushstore\n",j,(long long)stats->nxt64bits,ipaddr);
                kademlia_pushstore(mynxt64bits(),stats->nxt64bits);
            }
            else if ( j > 0 )
            {
                if ( matchflag != j-1 )
                {
                    for (k=matchflag; k<j-1; k++)
                        buckets[k] = buckets[k+1];
                    buckets[k] = stats;
                    for (k=0; k<j; j++)
                        printf("%llu ",(long long)buckets[k]->nxt64bits);
                    printf("ROTATE.%d: bucket[%d] <- %llu %s\n",j-1,matchflag,(long long)stats->nxt64bits,ipaddr);
                }
            } else printf("update_Kbucket: impossible case matchflag.%d j.%d\n",matchflag,j);
            return;
        }
        else if ( buckets[j] == stats )
            matchflag = j;
    }
    if ( matchflag < 0 ) // stats is new and the bucket is full
    {
        eviction = buckets[0];
        for (k=0; k<n-1; k++)
            buckets[k] = buckets[k+1];
        buckets[n-1] = stats;
        printf("bucket[%d] <- %llu, check for eviction of %llu %s\n",n-1,(long long)stats->nxt64bits,(long long)eviction->nxt64bits,ipaddr);
        stats->eviction = eviction;
        kademlia_pushstore(mynxt64bits(),stats->nxt64bits);
        if ( cp != 0 )
        {
            txid = send_kademlia_cmd(eviction->nxt64bits,0,"ping",cp->srvNXTACCTSECRET,0,0);
            printf("check for eviction with destip.%u txid.%llu\n",eviction->ipbits,(long long)txid);
        }
    }
}

void update_Kbuckets(struct nodestats *stats,uint64_t nxt64bits,char *ipaddr,int32_t port,int32_t p2pflag)
{
    struct coin_info *cp = get_coin_info("BTCD");
    uint64_t xorbits;
    int32_t dist;
    uint32_t ipbits;
    struct pserver_info *pserver;
    if ( nxt64bits != 0 )
    {
        if ( stats->nxt64bits == 0 || stats->nxt64bits != nxt64bits )
        {
            printf("nxt64bits %llu -> %llu\n",(long long)stats->nxt64bits,(long long)nxt64bits);
            stats->nxt64bits = nxt64bits;
        }
        pserver = get_pserver(0,ipaddr,p2pflag==0?port:0,p2pflag!=0?port:0);
        if ( pserver->nxt64bits == 0 || pserver->nxt64bits != nxt64bits )
        {
            printf("pserver nxt64bits %llu -> %llu\n",(long long)pserver->nxt64bits,(long long)nxt64bits);
            pserver->nxt64bits = nxt64bits;
        }
    }
    if ( ipaddr != 0 && ipaddr[0] != 0 )
    {
        ipbits = calc_ipbits(ipaddr);
        if ( stats->ipbits == 0 || stats->ipbits != ipbits )
        {
            printf("stats ipbits %u -> %u\n",stats->ipbits,ipbits);
            stats->ipbits = ipbits;
        }
        if ( port != 0 )
        {
            if ( p2pflag != 0 )
            {
                if ( stats->p2pport == 0 || stats->p2pport != port )
                {
                    printf("p2pport %u -> %u\n",stats->p2pport,port);
                    stats->p2pport = port;
                }
            }
            else
            {
                if ( stats->supernet_port == 0 || stats->supernet_port != port )
                {
                    printf("supernet_port %u -> %u\n",stats->supernet_port,port);
                    stats->supernet_port = port;
                }
            }
        }
    }
    if ( cp != 0 )
    {
        pserver = get_pserver(0,cp->myipaddr,0,0);
        addto_hasips(1,pserver,stats->ipbits);
        xorbits = (cp->srvpubnxtbits != 0) ? cp->srvpubnxtbits : cp->pubnxtbits;
        if ( stats->nxt64bits != xorbits && stats->nxt64bits != 0 )
        {
            xorbits ^= stats->nxt64bits;
            dist = bitweight(xorbits) - 1;
            Kbucket_updated[dist] = time(NULL);
            printf("Kbucket.%d updated: ",dist);
            update_Kbucket(K_buckets[dist],KADEMLIA_NUMK,stats);
        }
    }
}

uint64_t refresh_bucket(struct nodestats *buckets[],long lastcontact,char *NXTACCTSECRET)
{
    uint64_t txid = 0;
    int32_t i,r;
    if ( buckets[0] != 0 && lastcontact >= KADEMLIA_BUCKET_REFRESHTIME )
    {
        for (i=0; i<KADEMLIA_NUMK; i++)
            if ( buckets[i] == 0 )
                break;
        r = ((rand()>>8) % i);
        txid = send_kademlia_cmd(buckets[r]->nxt64bits,0,"findnode",NXTACCTSECRET,0,0);
    }
    return(txid);
}

void refresh_buckets(char *NXTACCTSECRET)
{
    static int didinit;
    long now = time(NULL);
    int32_t i,firstbucket,n = 0;
    firstbucket = -1;
    for (i=0; i<KADEMLIA_NUMBUCKETS; i++)
    {
        if ( firstbucket < 0 && K_buckets[i][0] != 0 )
            firstbucket = (i + 1);
        if ( refresh_bucket(K_buckets[i],now - Kbucket_updated[i],NXTACCTSECRET) != 0 )
            n++;
    }
    if ( n != 0 && didinit == 0 && firstbucket < KADEMLIA_NUMBUCKETS-1 )
    {
        printf("initial bucket refresh\n");
        for (i=firstbucket+1; i<KADEMLIA_NUMBUCKETS; i++)
            refresh_bucket(K_buckets[i],KADEMLIA_BUCKET_REFRESHTIME,NXTACCTSECRET);
        didinit = 1;
    }
}

int32_t expire_nodestats(struct nodestats *stats,uint32_t now)
{
    if ( stats->lastcontact != 0 && (now - stats->lastcontact) > NODESTATS_EXPIRATION )
    {
        char ipaddr[64];
        expand_ipbits(ipaddr,stats->ipbits);
        printf("expire_nodestats %s | %u %u %d\n",ipaddr,now,stats->lastcontact,now - stats->lastcontact);
        stats->gotencrypted = stats->modified = 0;
        stats->sentmilli = 0;
        stats->expired = 1;
        return(1);
    }
    return(0);
}

void every_minute(int32_t counter)
{
    static int broadcast_count;
    uint32_t now;
    int32_t i,createdflag,len,iter,connected,actionflag;
    char ipaddr[64],cmd[4096],packet[4096];//ip_port[64],NXTaddr[64],
    struct NXT_acct *mynp = 0;
    struct coin_info *cp;
    //struct peerinfo *peer;
    struct nodestats *stats;
    struct pserver_info *pserver,*mypserver = 0;
    if ( Finished_init == 0 )
        return;
    now = (uint32_t)time(NULL);
    cp = get_coin_info("BTCD");
   // if ( cp == 0 )
        return;
    //printf("<<<<<<<<<<<<< EVERY_MINUTE\n");
    refresh_buckets(cp->srvNXTACCTSECRET);
    if ( cp->myipaddr[0] != 0 )
        mypserver = get_pserver(0,cp->myipaddr,0,0);
    mynp = get_NXTacct(&createdflag,Global_mp,cp->srvNXTADDR);
    set_peer_json(cmd,cp->srvNXTADDR,&mynp->mypeerinfo);
    len = construct_tokenized_req(packet,cmd,cp->srvNXTACCTSECRET);
    if ( Num_in_whitelist > 0 && SuperNET_whitelist != 0 )
    {
        for (iter=0; iter<2; iter++)
        {
            connected = 0;
            for (i=0; i<Num_in_whitelist; i++)
            {
                expand_ipbits(ipaddr,SuperNET_whitelist[i]);
                pserver = get_pserver(0,ipaddr,0,0);
                if ( pserver != mypserver )
                {
                    actionflag = 1;
                    if ( (stats= get_nodestats(pserver->nxt64bits)) != 0 && expire_nodestats(stats,now) == 0 )
                    {
                        if ( stats->gotencrypted == 0 )
                        {
                            if ( stats->modified != 0 )
                            {
                                if ( iter == 1 )
                                    p2p_publishpacket(pserver,0);
                                connected++;
                            }
                            else if ( iter == 1 ) directsend_packet(pserver,packet,len);
                        } else connected++;
                    } else if ( iter == 1 ) p2p_publishpacket(pserver,0);
                }
            }
            if ( iter == 0 && connected <= (Num_in_whitelist/8) && broadcast_count < 10 )
            {
                printf("only connected to %d of %d, lets broadcast\n",connected,Num_in_whitelist);
                p2p_publishpacket(0,0);
                if ( broadcast_count++ == 0 )
                {
                    for (i=0; i<Num_in_whitelist; i++)
                    {
                        expand_ipbits(ipaddr,SuperNET_whitelist[i]);
                        //send_kademlia_cmd(0,get_pserver(0,ipaddr,0,0),"ping",cp->srvNXTACCTSECRET,0,0);
                    }
                    printf("PINGED.%d\n",Num_in_whitelist);
                }
                break;
            }
        }
    }
}

#endif
