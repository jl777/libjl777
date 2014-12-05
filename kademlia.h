//
//  kademlia.h
//  libjl777
//
//  Created by jl777 on 10/3/14.
//  Copyright (c) 2014 jl777. MIT license.
//

#ifndef libjl777_kademlia_h
#define libjl777_kademlia_h

#define KADEMLIA_MINTHRESHOLD 30
#define KADEMLIA_MAXTHRESHOLD 32

#define KADEMLIA_ALPHA 7
#define NODESTATS_EXPIRATION 600
#define KADEMLIA_BUCKET_REFRESHTIME 3600
#define KADEMLIA_NUMBUCKETS ((int)(sizeof(K_buckets)/sizeof(K_buckets[0])))
#define KADEMLIA_NUMK ((int)(sizeof(K_buckets[0])/sizeof(K_buckets[0][0])))

struct storage_queue_entry { uint64_t keyhash,destbits; int32_t selector; };

struct nodestats *K_buckets[64+1][7];
long Kbucket_updated[KADEMLIA_NUMBUCKETS];
uint64_t Allnodes[10000];
int32_t Numallnodes;
#define MAX_ALLNODES ((int32_t)(sizeof(Allnodes)/sizeof(*Allnodes)))

void update_Allnodes()
{
    int32_t i,lag;
    uint32_t now = (uint32_t)time(NULL);
    char NXTaddr[64];
    struct nodestats *stats,*sp;
    for (i=0; i<MAX_ALLNODES; i++)
    {
        if ( Allnodes[i] != 0 )
        {
            stats = get_nodestats(Allnodes[i]);
            expand_nxt64bits(NXTaddr,Allnodes[i]);
            if ( (sp= (struct nodestats *)find_storage(NODESTATS_DATA,NXTaddr,0)) != 0 )
            {
                lag = (now - sp->lastcontact);
                if ( lag > NODESTATS_EXPIRATION )
                    sp->lastcontact = 0;
                if ( memcmp(sp,stats,sizeof(*sp)) != 0 )
                    update_nodestats_data(stats);
                free(sp);
            }
        }
    }
}

struct nodestats *find_nodestats(uint64_t nxt64bits)
{
    int32_t i;
    if ( nxt64bits != 0 && Numallnodes > 0 )
    {
        for (i=0; i<MAX_ALLNODES; i++)
        {
            if ( Allnodes[i] == nxt64bits )
                return(get_nodestats(Allnodes[i]));
        }
    }
    return(0);
}

void add_new_node(uint64_t nxt64bits)
{
    if ( nxt64bits != 0 && find_nodestats(nxt64bits) == 0 )
    {
        if ( Debuglevel > 0 )
            printf("[%d of %d] ADDNODE.%llu\n",Numallnodes,MAX_ALLNODES,(long long)nxt64bits);
        Allnodes[Numallnodes % MAX_ALLNODES] = nxt64bits;
        Numallnodes++;
    }
}

struct nodestats *get_random_node()
{
    uint32_t now = (uint32_t)time(NULL);
    struct nodestats *stats;
    int32_t n;
    if ( Numallnodes > 0 )
    {
        n = (Numallnodes < MAX_ALLNODES) ? Numallnodes : MAX_ALLNODES;
        if ( (stats= get_nodestats(Allnodes[(rand()>>8) % n])) != 0 && stats->ipbits != 0 && stats->nxt64bits != 0 && (now - stats->lastcontact) < 600 )
            return(stats);
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

int32_t verify_addr(uint16_t *prevportp,char *previpaddr,char *refipaddr,int32_t refport)
{
    //char ipaddr[64];
    //*prevportp = extract_nameport(ipaddr,sizeof(ipaddr),(struct sockaddr_in *)addr);
    if ( strcmp(previpaddr,refipaddr) != 0 )//|| refport != port )
    {
        printf("verify_addr error: (%s) vs (%s)\n",refipaddr,previpaddr);
        strcpy(refipaddr,previpaddr);
        return(-1);
    }
    return(0);
}

int32_t ismynxtbits(uint64_t destbits)
{
    struct coin_info *cp = get_coin_info("BTCD");
    if ( cp != 0 )
    {
        if ( destbits == cp->privatebits || destbits == cp->srvpubnxtbits )
            return(1);
        else return(0);
    } else return(-1);
}

int32_t ismyipaddr(char *ipaddr)
{
    struct coin_info *cp = get_coin_info("BTCD");
    if ( cp != 0 )
    {
        if ( notlocalip(ipaddr) == 0 || strcmp(cp->myipaddr,ipaddr) == 0 )
            return(1);
        else return(0);
    } else return(-1);
}

int32_t ismynode(struct sockaddr *addr)
{
    char ipaddr[64];
    int32_t port;
    if ( addr == 0 )
        return(1);
    port = extract_nameport(ipaddr,sizeof(ipaddr),(struct sockaddr_in *)addr);
    return(ismyipaddr(ipaddr));
}

int32_t sort_all_buckets(uint64_t *sortbuf,uint64_t hash)
{
    //uint32_t now = (uint32_t)time(NULL);
    struct nodestats *stats;
    struct pserver_info *pserver;
    int32_t i,j,n;
    char ipaddr[64];
    for (i=n=0; i<KADEMLIA_NUMBUCKETS; i++)
    {
        for (j=0; j<KADEMLIA_NUMK; j++)
        {
            if ( (stats= K_buckets[i][j]) == 0 )
                break;
            if ( stats->ipbits != 0 && stats->nxt64bits != 0 )
            {
                expand_ipbits(ipaddr,stats->ipbits);
                pserver = get_pserver(0,ipaddr,0,0);
                //if ( (now - stats->lastcontact) < 600 )//pserver->decrypterrs == 0 &&
                {
                    sortbuf[n<<1] = bitweight(stats->nxt64bits ^ hash);// + ((stats->gotencrypted == 0) ? 64 : 0);
                    sortbuf[(n<<1) + 1] = stats->nxt64bits;
                    n++;
                }
            }
        }
    }
    if ( n == 0 )
        return(0);
    else sort64s(sortbuf,n,sizeof(*sortbuf)*2);
    return(n);
}

int32_t calc_bestdist(uint64_t keyhash)
{
    int32_t i,j,n,dist,bestdist = 10000;
    struct nodestats *stats;
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

uint64_t _send_kademlia_cmd(int32_t encrypted,struct pserver_info *pserver,char *cmdstr,char *NXTACCTSECRET,unsigned char *data,int32_t datalen)
{
    int32_t len;// = (int32_t)strlen(cmdstr);
    char _tokbuf[4096];
    uint64_t txid;
    if ( strcmp("0.0.0.0",pserver->ipaddr) == 0 )
    {
        printf("_send_kademlia_cmd illegal ip addr %s (%s)\n",pserver->ipaddr,cmdstr);
        return(0);
    }
    len = construct_tokenized_req(_tokbuf,cmdstr,NXTACCTSECRET);
    if ( Debuglevel > 1 )
        printf(">>>>>>>> directsend.[%s] encrypted.%d -> (%s)\n",_tokbuf,encrypted,pserver->ipaddr);
    txid = directsend_packet(encrypted,pserver,_tokbuf,len,data,datalen);
    return(txid);
}

uint8_t *replace_datafield(char *cmdstr,uint8_t *databuf,int32_t *datalenp,char *datastr)
{
    int32_t len = 0;
    uint8_t *data = 0;
    if ( datastr != 0 && datastr[0] != 0 )
    {
        len = (int32_t)strlen(datastr);
        if ( len < 2 || (len & 1) != 0 || is_hexstr(datastr) == 0 )
        {
            if ( datastr[0] == '[' )
                sprintf(cmdstr+strlen(cmdstr),",\"data\":%s",datastr);
            else sprintf(cmdstr+strlen(cmdstr),",\"data\":\"%s\"",datastr);
            len = 0;
        }
        else
        {
            len >>= 1;
            sprintf(cmdstr+strlen(cmdstr),",\"data\":%d",len);
            data = databuf;
            decode_hex(data,len,datastr);
        }
    }
    *datalenp = len;
    return(data);
}

int32_t gen_pingstr(char *cmdstr,int32_t completeflag)
{
    struct coin_info *cp = get_coin_info("BTCD");
    if ( cp != 0 )
    {
        sprintf(cmdstr,"{\"requestType\":\"ping\",\"NXT\":\"%s\",\"time\":%ld,\"pubkey\":\"%s\",\"ipaddr\":\"%s\",\"ver\":\"%s\"",cp->srvNXTADDR,(long)time(NULL),Global_mp->pubkeystr,cp->myipaddr,HARDCODED_VERSION);
        if ( completeflag != 0 )
            strcat(cmdstr,"}");
        return((int32_t)strlen(cmdstr));
    } else return(0);
}

void send_to_ipaddr(int32_t tokenizeflag,char *ipaddr,char *jsonstr,char *NXTACCTSECRET)
{
    char _tokbuf[MAX_JSON_FIELD];
    struct pserver_info *pserver;
    struct sockaddr destaddr;
    struct nodestats *stats;
    int32_t port;
    pserver = get_pserver(0,ipaddr,0,0);
    stats = get_nodestats(pserver->nxt64bits);
    if ( stats != 0 )
        port = stats->supernet_port;
    else port = 0;
    if ( port == 0 )
        port = SUPERNET_PORT;
    uv_ip4_addr(ipaddr,port,(struct sockaddr_in *)&destaddr);
    if ( tokenizeflag != 0 )
        construct_tokenized_req(_tokbuf,jsonstr,NXTACCTSECRET);
    else safecopy(_tokbuf,jsonstr,sizeof(_tokbuf));
    fprintf(stderr,"send_to_ipaddr.(%s)\n",_tokbuf);
    portable_udpwrite(0,(struct sockaddr *)&destaddr,Global_mp->udp,_tokbuf,strlen(_tokbuf)+1,ALLOCWR_ALLOCFREE);
}

int32_t filter_duplicate_kadcmd(char *cmdstr,char *key,char *datastr,uint64_t nxt64bits)
{
    static int lasti;
    static uint64_t txids[8192]; // filter out infinite loops
    bits256 hash;
    uint64_t txid,keybits;
    int32_t i,dist;
    keybits = calc_nxt64bits(key);
    dist = bitweight(keybits ^ nxt64bits);
    //fprintf(stderr,"filter duplicate.(%s) lasti.%d\n",cmdstr,lasti);
    //if ( dist <= KADEMLIA_MAXTHRESHOLD )
    {
        if ( datastr != 0 && datastr[0] != 0 )
            calc_sha256cat(hash.bytes,(uint8_t *)cmdstr,(int32_t)strlen(cmdstr),(uint8_t *)datastr,(int32_t)strlen(datastr));
        else calc_sha256(0,hash.bytes,(uint8_t *)cmdstr,(int32_t)strlen(cmdstr));
        txid = (hash.txid ^ nxt64bits ^ keybits);
        for (i=0; i<(int)(sizeof(txids)/sizeof(*txids)); i++)
        {
            if ( txids[i] == 0 )
                break;
            else if ( txids[i] == txid )
            {
                if ( Debuglevel > 1 )
                    printf("send_kademlia_cmd.(%s): SKIP duplicate txid.%llu to %llu in slot.%d lasti.%d\n",cmdstr,(long long)txid,(long long)nxt64bits,i,lasti);
                return(1);
            }
        }
        if ( i == (int)(sizeof(txids)/sizeof(*txids)) )
        {
            i = lasti++;
            if ( lasti >= (int)(sizeof(txids)/sizeof(*txids)) )
                lasti = 0;
        }
        txids[i] = txid;
    }
    //fprintf(stderr,"done filter duplicate.(%s) lasti.%d\n",cmdstr,lasti);
    return(0);
}

uint64_t send_kademlia_cmd(uint64_t nxt64bits,struct pserver_info *pserver,char *kadcmd,char *NXTACCTSECRET,char *key,char *datastr)
{
    int32_t encrypted,createdflag,datalen = 0;
    struct nodestats *stats;
    struct NXT_acct *np;
    unsigned char databuf[32768],*data = 0;
    struct coin_info *cp = get_coin_info("BTCD");
    char pubkeystr[1024],ipaddr[64],cmdstr[2048],verifiedNXTaddr[64],destNXTaddr[64];
    if ( NXTACCTSECRET[0] == 0 || cp == 0 )
    {
        fprintf(stderr,"send_kademlia_cmd.%s srvpubaddr or cp.%p dest.%llu\n",kadcmd,cp,(long long)nxt64bits);
        strcpy(NXTACCTSECRET,cp->srvNXTACCTSECRET);
    }
    init_hexbytes_noT(pubkeystr,Global_mp->loopback_pubkey,sizeof(Global_mp->loopback_pubkey));
    verifiedNXTaddr[0] = 0;
    find_NXTacct(verifiedNXTaddr,NXTACCTSECRET);
    //fprintf(stderr,"send_kademlia_cmd (%s) [%s] pserver.%p\n",verifiedNXTaddr,NXTACCTSECRET,pserver);
    if ( pserver == 0 )
    {
        expand_nxt64bits(destNXTaddr,nxt64bits);
        np = get_NXTacct(&createdflag,Global_mp,destNXTaddr);
        expand_ipbits(ipaddr,np->stats.ipbits);
        pserver = get_pserver(0,ipaddr,0,0);
        if ( pserver->nxt64bits == 0 )
            pserver->nxt64bits = nxt64bits;
    } else nxt64bits = pserver->nxt64bits;
    strcpy(ipaddr,pserver->ipaddr);
    if ( strcmp(kadcmd,"ping") != 0 && nxt64bits == 0 )
    {
        printf("send_kademlia_cmd.(%s) No destination\n",kadcmd);
        return(0);
    }
    if ( 0 && (pserver->nxt64bits == cp->privatebits || pserver->nxt64bits == cp->srvpubnxtbits) )
    {
        printf("no point to send yourself (%s) dest.%llu pub.%llu srvpub.%llu\n",kadcmd,(long long)pserver->nxt64bits,(long long)cp->privatebits,(long long)cp->srvpubnxtbits);
        return(0);
    }
    encrypted = 2;
    stats = get_nodestats(nxt64bits);
    if ( strcmp(kadcmd,"ping") == 0 )
    {
        encrypted = 0;
        if ( stats != 0 )
        {
            stats->pingmilli = milliseconds();
            stats->numpings++;
        }
        gen_pingstr(cmdstr,1);
        send_to_ipaddr(1,pserver->ipaddr,cmdstr,NXTACCTSECRET);
        return(0);
    }
    else
    {
        if ( strcmp(kadcmd,"pong") == 0 )
        {
            encrypted = 1;
            sprintf(cmdstr,"{\"requestType\":\"%s\",\"NXT\":\"%s\",\"time\":%ld,\"yourip\":\"%s\",\"yourport\":%d,\"ipaddr\":\"%s\",\"pubkey\":\"%s\",\"ver\":\"%s\"",kadcmd,verifiedNXTaddr,(long)time(NULL),pserver->ipaddr,pserver->port,cp->myipaddr,pubkeystr,HARDCODED_VERSION);
            //send_to_ipaddr(1,pserver->ipaddr,cmdstr,NXTACCTSECRET);
            //return(0);
            //len = construct_tokenized_req(_tokbuf,cmdstr,NXTACCTSECRET);
            //portable_udpwrite(0,(struct sockaddr *)&destaddr,Global_mp->udp,_tokbuf,strlen(_tokbuf),ALLOCWR_ALLOCFREE);
        }
        else
        {
            if ( strncmp(kadcmd,"have",4) == 0 )
                encrypted = 1;
            else if ( (strcmp(kadcmd,"store") == 0 || strcmp(kadcmd,"findnode") == 0) && datastr != 0 && datastr[0] != 0 )
                encrypted = 4;
            sprintf(cmdstr,"{\"requestType\":\"%s\",\"NXT\":\"%s\",\"time\":%ld",kadcmd,verifiedNXTaddr,(long)time(NULL));
        }
    }
    if ( key != 0 && key[0] != 0 )
        sprintf(cmdstr+strlen(cmdstr),",\"key\":\"%s\"",key);
    data = replace_datafield(cmdstr,databuf,&datalen,datastr);
    strcat(cmdstr,"}");
    if ( key == 0 || key[0] == 0 || filter_duplicate_kadcmd(cmdstr,key,datastr,nxt64bits) == 0 )
        return(_send_kademlia_cmd(encrypted,pserver,cmdstr,NXTACCTSECRET,data,datalen));
    return(0);
}

void kademlia_update_info(char *destNXTaddr,char *ipaddr,int32_t port,char *pubkeystr,uint32_t lastcontact,int32_t p2pflag)
{
    uint64_t nxt64bits;
    uint32_t changed,ipbits = 0;
    struct pserver_info *pserver;
    struct nodestats *stats = 0;
    if ( destNXTaddr != 0 && destNXTaddr[0] != 0 )
        nxt64bits = calc_nxt64bits(destNXTaddr);
    else nxt64bits = 0;
    changed = 0;
    if ( port == BTCD_PORT && p2pflag == 0 )
    {
        printf("warning: kademlia_update_info port is %d?\n",port);
        port = 0;
    }
    if ( ipaddr != 0 && ipaddr[0] != 0 )
    {
        ipbits = calc_ipbits(ipaddr);
        pserver = get_pserver(0,ipaddr,p2pflag==0?port:0,p2pflag!=0?port:0);
        if ( nxt64bits != 0 && (pserver->nxt64bits == 0 || pserver->nxt64bits != nxt64bits) )
        {
            printf("kademlia_update_info: pserver nxt64bits %llu -> %llu\n",(long long)pserver->nxt64bits,(long long)nxt64bits);
            pserver->nxt64bits = nxt64bits;
            changed++;
        }
    }
    if ( nxt64bits != 0 )
    {
        stats = get_nodestats(nxt64bits);
        if ( stats->nxt64bits == 0 || stats->nxt64bits != nxt64bits )
        {
            printf("kademlia_update_info: nxt64bits %llu -> %llu\n",(long long)stats->nxt64bits,(long long)nxt64bits);
            stats->nxt64bits = nxt64bits;
            changed++;
        }
        if ( ipbits != 0 && (stats->ipbits == 0 || stats->ipbits != ipbits) )
        {
            printf("kademlia_update_info: stats ipbits %u -> %u\n",stats->ipbits,ipbits);
            stats->ipbits = ipbits;
            changed++;
        }
        if ( port != 0 )
        {
            if ( p2pflag != 0 )
            {
                if ( stats->p2pport == 0 || stats->p2pport != port )
                {
                    printf("kademlia_update_info: p2pport %u -> %u\n",stats->p2pport,port);
                    stats->p2pport = port;
                    changed++;
                }
            }
            else
            {
                if ( stats->supernet_port == 0 || stats->supernet_port != port )
                {
                    printf("kademlia_update_info: supernet_port %u -> %u\n",stats->supernet_port,port);
                    stats->supernet_port = port;
                    changed++;
                }
            }
        }
        if ( pubkeystr != 0 && pubkeystr[0] != 0 && update_pubkey(stats->pubkey,pubkeystr) != 0 && lastcontact != 0 )
            stats->lastcontact = lastcontact, changed++;
        if ( changed != 0 )
            update_nodestats_data(stats);
    }
}

void change_nodeinfo(char *ipaddr,uint16_t port,uint64_t nxt64bits)
{
    struct nodestats *stats;
    struct pserver_info *pserver;
    stats = get_nodestats(nxt64bits);
    stats->ipbits = calc_ipbits(ipaddr);
    if ( port != 0 )//(stats->supernet_port == 0 || stats->supernet_port == SUPERNET_PORT) )
    {
        printf("OVERRIDE supernet_port.%d -> %d\n",stats->supernet_port,port);
        stats->supernet_port = port;
    }
    pserver = get_pserver(0,ipaddr,port,0);
    pserver->nxt64bits = nxt64bits;
    add_new_node(nxt64bits);
}

void set_myipaddr(struct coin_info *cp,char *ipaddr,uint16_t port)
{
    strcpy(cp->myipaddr,ipaddr);
    change_nodeinfo(ipaddr,port,cp->srvpubnxtbits);
}

char *kademlia_ping(char *previpaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,char *ipaddr,int32_t port,char *destip,char *origargstr)
{
    uint64_t txid = 0;
    char retstr[1024];
    uint16_t prevport;
    struct coin_info *cp = get_coin_info("BTCD");
    retstr[0] = 0;
    if ( is_remote_access(previpaddr) == 0 ) // user invoked
    {
        if ( destip != 0 && destip[0] != 0 )
        {
            if ( ismyipaddr(destip) == 0 )
                txid = send_kademlia_cmd(0,get_pserver(0,destip,0,0),"ping",NXTACCTSECRET,0,0);
            sprintf(retstr,"{\"result\":\"kademlia_ping to %s\",\"txid\":\"%llu\"}",destip,(long long)txid);
        }
        else sprintf(retstr,"{\"error\":\"kademlia_ping no destip\"}");
    }
    else // sender ping'ed us
    {
        if ( notlocalip(cp->myipaddr) == 0 && destip[0] != 0 && calc_ipbits(destip) != 0 )
        {
            fprintf(stderr,"AUTO SETTING MYIP <- (%s)\n",destip);
            set_myipaddr(cp,ipaddr,port);
        }
        prevport = 0;
        if ( verify_addr(&prevport,previpaddr,ipaddr,port) < 0 ) // auto-corrects ipaddr
        {
            change_nodeinfo(ipaddr,prevport,calc_nxt64bits(sender));
            //sprintf(retstr,"{\"error\":\"kademlia_ping from %s doesnt verify (%s) -> new IP (%s:%d)\"}",sender,origargstr,ipaddr,prevport);
        }
        txid = send_kademlia_cmd(0,get_pserver(0,ipaddr,prevport,0),"pong",NXTACCTSECRET,0,0);
        sprintf(retstr,"{\"result\":\"kademlia_pong to (%s/%d)\",\"txid\":\"%llu\"}",ipaddr,prevport,(long long)txid);
    }
    if ( is_remote_access(previpaddr) != 0 && retstr[0] != 0 )
        fprintf(stderr,"PING.(%s)\n",retstr);
    return(clonestr(retstr));
}

char *kademlia_pong(char *previpaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,char *ipaddr,uint16_t port,char *yourip,int32_t yourport,char *tag)
{
    char retstr[1024];
    struct nodestats *stats;
    struct coin_info *cp = get_coin_info("BTCD");
    stats = get_nodestats(calc_nxt64bits(sender));
    if ( cp != 0 && notlocalip(cp->myipaddr) == 0 && yourip[0] != 0 && calc_ipbits(yourip) != 0 )
    {
        printf("AUTOUPDATE IP <= (%s)\n",yourip);
        set_myipaddr(cp,yourip,yourport);
    }
    if ( stats != 0 )
    {
        stats->pongmilli = milliseconds();
        stats->pingpongsum += (stats->pongmilli - stats->pingmilli);
        stats->numpongs++;
        sprintf(retstr,"{\"result\":\"kademlia_pong\",\"tag\":\"%s\",\"NXT\":\"%s\",\"ipaddr\":\"%s\",\"port\":%d,\"lag\":\"%.3f\",\"numpings\":%d,\"numpongs\":%d,\"ave\":\"%.3f\"}",tag,sender,ipaddr,port,stats->pongmilli-stats->pingmilli,stats->numpings,stats->numpongs,(2*stats->pingpongsum)/(stats->numpings+stats->numpongs+1));
    }
    else sprintf(retstr,"{\"result\":\"kademlia_pong\",\"tag\":\"%s\",\"NXT\":\"%s\",\"ipaddr\":\"%s\",\"port\":%d\"}",tag,sender,ipaddr,port);
    //if ( Debuglevel > 0 )
        printf("PONG.(%s)\n",retstr);
    return(clonestr(retstr));
}

struct SuperNET_storage *kademlia_getstored(int32_t selector,uint64_t keyhash,char *datastr)
{
    char key[64];
    struct SuperNET_storage *sp;
    expand_nxt64bits(key,keyhash);
    sp = (struct SuperNET_storage *)find_storage(selector,key,0);
    if ( datastr == 0 )
        return(sp);
    if ( sp != 0 )
        free(sp);
    add_storage(selector,key,datastr);
    return((struct SuperNET_storage *)find_storage(selector,key,0));
}

uint64_t *find_closer_Kstored(int32_t selector,uint64_t refbits,uint64_t newbits)
{
    struct SuperNET_storage *sp,**sps;
    int32_t i,numentries,dist,max,m,refdist,n = 0;
    uint64_t *keys = 0;
    max = (int32_t)max_in_db(selector);
    max += 100;
    m = 0;
    sps = (struct SuperNET_storage **)copy_all_DBentries(&numentries,selector);
    if ( sps == 0 )
        return(0);
    keys = (uint64_t *)calloc(sizeof(*keys),max);
    for (i=0; i<numentries; i++)
    {
        sp = sps[i];
        m++;
        refdist = bitweight(refbits ^ sp->H.keyhash);
        dist = bitweight(newbits ^ sp->H.keyhash);
        if ( dist < refdist )
        {
            keys[n++] = sp->H.keyhash;
            if ( n >= max )
            {
                max += 100;
                keys = (uint64_t *)realloc(keys,sizeof(*keys)*max);
            }
        }
        free(sps[i]);
    }
    free(sps);
    if ( m > max_in_db(selector) )
        set_max_in_db(selector,m);
    return(keys);
}

int32_t kademlia_pushstore(int32_t selector,uint64_t refbits,uint64_t newbits)
{
    int32_t n = 0;
    uint64_t *keys,key;
    struct storage_queue_entry *ptr;
    //fprintf(stderr,"pushstore\n");
    if ( (keys= find_closer_Kstored(selector,refbits,newbits)) != 0 )
    {
        while ( (key= keys[n++]) != 0 )
        {
            ptr = calloc(1,sizeof(*ptr));
            ptr->destbits = newbits;
            ptr->selector = selector;
            ptr->keyhash = key;
            //printf("%p queue.%d to %llu\n",ptr,n,(long long)newbits);
            queue_enqueue(&storageQ,ptr);
        }
        //printf("free sps.%p\n",sps);
        free(keys);
        if ( Debuglevel > 1 )
            printf("Queue n.%d pushstore to %llu\n",n,(long long)newbits);
    }
    return(n);
}

uint64_t process_storageQ()
{
    struct storage_queue_entry *ptr;
    char key[64],datastr[8193];
    uint64_t txid = 0;
    struct SuperNET_storage *sp;
    struct coin_info *cp = get_coin_info("BTCD");
    //fprintf(stderr,"process_storageQ\n");
    if ( (ptr= queue_dequeue(&storageQ)) != 0 )
    {
        //fprintf(stderr,"dequeue StorageQ %p key.(%llu) dest.(%llu) selector.%d\n",ptr,(long long)ptr->keyhash,(long long)ptr->destbits,ptr->selector);
        expand_nxt64bits(key,ptr->keyhash);
        if ( (sp= (struct SuperNET_storage *)find_storage(ptr->selector,key,0)) != 0 )
        {
            init_hexbytes_noT(datastr,sp->data,sp->H.size-sizeof(*sp));
            //fprintf(stderr,"dequeued storageQ %p: (%s) len.%ld\n",ptr,datastr,sp->H.size-sizeof(*sp));
            txid = send_kademlia_cmd(ptr->destbits,0,"store",cp->srvNXTACCTSECRET,key,datastr);
            if ( Debuglevel > 2 )
                fprintf(stderr,"txid.%llu send queued push storage key.(%s) to %llu\n",(long long)txid,key,(long long)ptr->destbits);
            free(sp);
        }
        free(ptr);
    }
    return(txid);
}

void do_localstore(uint64_t *txidp,char *keystr,char *datastr,char *NXTACCTSECRET)
{
    void check_for_InstantDEX(char *decoded,char *keystr);
    uint64_t keybits;
    int32_t len,createdflag;
    char decoded[MAX_JSON_FIELD];
    struct NXT_acct *keynp;
    struct SuperNET_storage *sp;
    keybits = calc_nxt64bits(keystr);
    fprintf(stderr,"do_localstore(%s) <- (%s)\n",keystr,datastr);
    keynp = get_NXTacct(&createdflag,Global_mp,keystr);
    *txidp = 0;
    if ( keynp->bestbits != 0 )
    {
        if ( ismynxtbits(keynp->bestdist) == 0 )
        {
            printf("store at bestbits\n");
            *txidp = send_kademlia_cmd(keynp->bestdist,0,"store",NXTACCTSECRET,keystr,datastr);
        }
        printf("Bestdist.%d bestbits.%llu\n",keynp->bestdist,(long long)keynp->bestbits);
        keynp->bestbits = 0;
        keynp->bestdist = 0;
    }
    len = (int32_t)strlen(datastr)/2;
    decode_hex((uint8_t *)decoded,len,datastr);
    if ( (decoded[len-1] == 0 || decoded[len-1] == '}' || decoded[len-1] == ']') && (decoded[0] == '{' || decoded[0] == '[') )
        check_for_InstantDEX(decoded,keystr);
    if ( (sp= kademlia_getstored(PUBLIC_DATA,keybits,datastr)) != 0 )
        free(sp);
}

char *kademlia_storedata(char *previpaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,char *key,char *datastr)
{
    char retstr[32768];
    uint64_t sortbuf[2 * KADEMLIA_NUMBUCKETS * KADEMLIA_NUMK];
    uint64_t keybits,destbits,txid = 0;
    int32_t i,n,z,dist,mydist;
    struct coin_info *cp = get_coin_info("BTCD");
    if ( datastr != 0 && strlen(datastr) > 2048 )
    {
        printf("store: datastr too big.%ld\n",strlen(datastr));
        return(0);
    }
    if ( cp == 0 || key == 0 || key[0] == 0 || datastr == 0 || datastr[0] == 0 )
    {
        printf("kademlia_storedata null args cp.%p key.%p datastr.%p\n",cp,key,datastr);
        return(0);
    }
    keybits = calc_nxt64bits(key);
    mydist = bitweight(keybits ^ cp->srvpubnxtbits);
    memset(sortbuf,0,sizeof(sortbuf));
    n = sort_all_buckets(sortbuf,keybits);
    retstr[0] = 0;
    if ( n != 0 )
    {
        for (i=z=0; i<n&&z<KADEMLIA_NUMK; i++)
        {
            destbits = sortbuf[(i<<1) + 1];
            dist = bitweight(destbits ^ keybits);
            if ( ismynxtbits(destbits) == 0 && dist < mydist )
            {
                printf("store i.%d of %d, (%llu) dist.%d vs mydist.%d\n",i,n,(long long)destbits,dist,mydist);
                txid = send_kademlia_cmd(destbits,0,"store",NXTACCTSECRET,key,datastr);
                z++;
            }
        }
        sprintf(retstr,"{\"result\":\"kademlia_store\",\"key\":\"%s\",\"data\":\"%s\",\"len\":%ld,\"txid\":\"%llu\"}",key,datastr,strlen(datastr)/2,(long long)txid);
    } else sprintf(retstr,"{\"result\":\"localstore only\"}");
    do_localstore(&txid,key,datastr,NXTACCTSECRET);
    //if ( Debuglevel > 0 )
        printf("STORE.(%s)\n",retstr);
    return(clonestr(retstr));
}

char *kademlia_havenode(int32_t valueflag,char *previpaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,char *key,char *value)
{
    char retstr[1024],ipaddr[MAX_JSON_FIELD],destNXTaddr[MAX_JSON_FIELD],portstr[MAX_JSON_FIELD],lastcontactstr[MAX_JSON_FIELD];
    int32_t i,n,createdflag,dist,mydist;
    uint32_t lastcontact,port;
    uint64_t keyhash,txid = 0;
    cJSON *array,*item;
    struct coin_info *cp = get_coin_info("BTCD");
    struct pserver_info *pserver = 0;
    struct NXT_acct *keynp;
    if ( value != 0 && strlen(value) > 2048 )
    {
        printf("havenode datastr too big.%ld\n",strlen(value));
        return(0);
    }
   keyhash = calc_nxt64bits(key);
    mydist = bitweight(cp->srvpubnxtbits ^ keyhash);
    if ( key != 0 && key[0] != 0 && value != 0 && value[0] != 0 && (array= cJSON_Parse(value)) != 0 )
    {
        if ( is_remote_access(previpaddr) != 0 )
            pserver = get_pserver(0,previpaddr,0,0);
        else if ( cp != 0 ) pserver = get_pserver(0,cp->myipaddr,0,0);
        keynp = get_NXTacct(&createdflag,Global_mp,key);
        //printf("parsed value array.%p\n",array);
        if ( is_cJSON_Array(array) != 0 )
        {
            n = cJSON_GetArraySize(array);
            //printf("is arraysize.%d\n",n);
            for (i=0; i<n; i++)
            {
                item = cJSON_GetArrayItem(array,i);
                //printf("item.%p isarray.%d num.%d\n",item,is_cJSON_Array(item),cJSON_GetArraySize(item));
                if ( is_cJSON_Array(item) != 0 && cJSON_GetArraySize(item) == 4 )
                {
                    copy_cJSON(destNXTaddr,cJSON_GetArrayItem(item,0));
                    //if ( destNXTaddr[0] != 0 )
                    //    addto_hasnxt(pserver,calc_nxt64bits(destNXTaddr));
                    //copy_cJSON(pubkeystr,cJSON_GetArrayItem(item,1));
                    copy_cJSON(ipaddr,cJSON_GetArrayItem(item,1));
                    //if ( ipaddr[0] != 0 && notlocalip(ipaddr) != 0 )
                    //    addto_hasips(1,pserver,calc_ipbits(ipaddr));
                    copy_cJSON(portstr,cJSON_GetArrayItem(item,2));
                    copy_cJSON(lastcontactstr,cJSON_GetArrayItem(item,3));
                    port = (uint32_t)atol(portstr);
                    lastcontact = (uint32_t)atol(lastcontactstr);
                    //printf("[%s ip.%s %s port.%d lastcontact.%d]\n",destNXTaddr,ipaddr,pubkeystr,port,lastcontact);
                    if ( destNXTaddr[0] != 0 && ipaddr[0] != 0 )
                    {
                        kademlia_update_info(destNXTaddr,ipaddr,port,0,lastcontact,0);
                        dist = bitweight(keynp->H.nxt64bits ^ calc_nxt64bits(destNXTaddr));
                        if ( dist < calc_bestdist(keyhash) )
                        {
                            if ( Debuglevel > 1 )
                                printf("%s new bestdist %d vs %d\n",destNXTaddr,dist,keynp->bestdist);
                            keynp->bestdist = dist;
                            keynp->bestbits = calc_nxt64bits(destNXTaddr);
                        }
                        if ( keynp->bestbits != 0 && ismynxtbits(keynp->bestbits) == 0 && dist < mydist )
                            txid = send_kademlia_cmd(keynp->bestbits,0,valueflag!=0?"findvalue":"findnode",NXTACCTSECRET,key,0);
                    }
                }
            }
        } else printf("kademlia.(havenode) not array\n");
        free_json(array);
    }
    sprintf(retstr,"{\"result\":\"kademlia_havenode from NXT.%s key.(%s) value.(%s)\"}",sender,key,value);
    //if ( Debuglevel > 0 )
        printf("HAVENODE.%d %s\n",valueflag,retstr);
    return(clonestr(retstr));
}

cJSON *gen_find_nodejson(char *NXTaddr)
{
    char ipaddr[64],numstr[64];
    struct nodestats *stats;
    cJSON *item = 0;
    if ( (stats = get_nodestats(calc_nxt64bits(NXTaddr))) != 0 )
    {
        item = cJSON_CreateArray();
        cJSON_AddItemToArray(item,cJSON_CreateString(NXTaddr));
        /*if ( memcmp(stats->pubkey,zerokey,sizeof(stats->pubkey)) != 0 )
        {
            init_hexbytes_noT(pubkeystr,stats->pubkey,sizeof(stats->pubkey));
            cJSON_AddItemToArray(item,cJSON_CreateString(pubkeystr));
        }*/
        if ( stats->ipbits != 0 )
        {
            expand_ipbits(ipaddr,stats->ipbits);
            cJSON_AddItemToArray(item,cJSON_CreateString(ipaddr));
            sprintf(numstr,"%d",stats->supernet_port==0?SUPERNET_PORT:stats->supernet_port);
            cJSON_AddItemToArray(item,cJSON_CreateString(numstr));
            sprintf(numstr,"%u",stats->lastcontact);
            cJSON_AddItemToArray(item,cJSON_CreateString(numstr));
        }
    }
    return(item);
}

void passthrough_packet(char *destNXTaddr,char *origargstr,char *datastr)
{
    int32_t onionize(char *hopNXTaddr,unsigned char *maxbuf,unsigned char *encoded,char *destNXTaddr,unsigned char **payloadp,int32_t len);
    int32_t len,datalen;
    char hopNXTaddr[64];
    uint8_t maxbuf[MAX_UDPLEN],encoded[MAX_UDPLEN],encodedF[MAX_UDPLEN],data[MAX_UDPLEN*2],*outbuf;
    datalen = (int32_t)(strlen(datastr) / 2);
    decode_hex(data,datalen,datastr);
    outbuf = encoded;
    len = (int32_t)strlen(origargstr)+1;
    memcpy(encoded,origargstr,len);
    memcpy(encoded+len,data,datalen);
    len += datalen;
    hopNXTaddr[0] = 0;
    memset(encodedF,0,sizeof(encodedF));
    len = onionize(hopNXTaddr,maxbuf,encodedF,destNXTaddr,&outbuf,len);
    len = onionize(hopNXTaddr,maxbuf,0,destNXTaddr,&outbuf,len);
    route_packet(1,0,hopNXTaddr,outbuf,len);
}

void gen_havenode_str(char *jsonstr,uint64_t *sortbuf,int32_t n)
{
    int32_t i;
    char *value,destNXTaddr[64];
    cJSON *array,*item;
    array = cJSON_CreateArray();
    for (i=0; i<n&&i<KADEMLIA_NUMK; i++)
    {
        expand_nxt64bits(destNXTaddr,sortbuf[(i<<1) + 1]);
        if ( (item= gen_find_nodejson(destNXTaddr)) != 0 )
            cJSON_AddItemToArray(array,item);
    }
    value = cJSON_Print(array);
    free_json(array);
    stripwhite_ns(value,strlen(value));
    strcpy(jsonstr,value);
    free(value);
}

int32_t get_public_datastr(char *retstr,char *databuf,uint64_t keyhash)
{
    struct SuperNET_storage *sp;
    databuf[0] = 0;
    sp = kademlia_getstored(PUBLIC_DATA,keyhash,0);
    if ( sp != 0 )
    {
        init_hexbytes_noT(databuf,sp->data,sp->H.size-sizeof(*sp));
        //if ( ismynxtbits(senderbits) == 0 && is_remote_access(previpaddr) != 0 )
        //    txid = send_kademlia_cmd(senderbits,0,"store",NXTACCTSECRET,key,databuf);
        if ( retstr != 0 )
            sprintf(retstr,"{\"data\":\"%s\"}",databuf);
        free(sp);
        return(0);
    }
    return(-1);
}

int32_t process_special_packets(char *retstr,int32_t isvalue,char *key,char *datastr,uint64_t senderbits,char *previpaddr,char *NXTACCTSECRET)
{
    void process_telepathic(char *key,char *datastr,uint64_t senderbits,char *senderip);
    int32_t filtered_orderbook(char *retdatastr,char *jsonstr);
    char decoded[MAX_JSON_FIELD],retdatastr[MAX_JSON_FIELD];
    int32_t len,remoteflag = 0;
    if ( isvalue == 0 )
    {
        if ( is_remote_access(previpaddr) != 0 )
            process_telepathic(key,datastr,senderbits,previpaddr);
        remoteflag = 1;
    }
    else
    {
        len = (int32_t)strlen(datastr)/2;
        decode_hex((uint8_t *)decoded,len,datastr);
        if ( (decoded[len-1] == 0 || decoded[len-1] == '}' || decoded[len-1] == ']') && (decoded[0] == '{' || decoded[0] == '[') )
        {
            if ( filtered_orderbook(retdatastr,decoded) > 0 )
            {
                if ( ismynxtbits(senderbits) == 0 && is_remote_access(previpaddr) != 0 )
                    send_kademlia_cmd(senderbits,0,"store",NXTACCTSECRET,key,retdatastr);
                sprintf(retstr,"{\"data\":\"%s\",\"instantDEX\":\"%s\"}",retdatastr,datastr);
                printf("FOUND_InstantDEX.(%s)\n",retstr);
                //return(clonestr(retstr));
            }
        }
    }
    return(remoteflag);
}

char *kademlia_find(char *cmd,char *previpaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,char *key,char *datastr,char *origargstr)
{
    char retstr[32768],databuf[32768],_previpaddr[64],destNXTaddr[64];
    uint64_t keyhash,senderbits,destbits,txid = 0;
    uint64_t sortbuf[2 * KADEMLIA_NUMBUCKETS * KADEMLIA_NUMK];
    int32_t z,i,n,iter,isvalue,createdflag,mydist,dist,remoteflag = 0;
    struct coin_info *cp = get_coin_info("BTCD");
    struct NXT_acct *keynp,*np;
    if ( datastr != 0 && strlen(datastr) > 2048 )
    {
        printf("%s: datastr too big.%ld\n",cmd,strlen(datastr));
        return(0);
    }
    if ( previpaddr == 0 )
    {
        _previpaddr[0] = 0;
        previpaddr = _previpaddr;
    }
    retstr[0] = 0;
    if ( Debuglevel > 0 )
        printf("myNXT.(%s) kademlia_find.(%s) (%s) data.(%s) is_remote_access.%d (%s)\n",verifiedNXTaddr,cmd,key,datastr!=0?datastr:"",is_remote_access(previpaddr),previpaddr==0?"":previpaddr);
    if ( key != 0 && key[0] != 0 )
    {
        senderbits = calc_nxt64bits(sender);
        keyhash = calc_nxt64bits(key);
        mydist = bitweight(cp->srvpubnxtbits ^ keyhash);
        isvalue = (strcmp(cmd,"findvalue") == 0);
        if ( datastr != 0 && datastr[0] != 0 ) 
            remoteflag = process_special_packets(retstr,isvalue,key,datastr,senderbits,previpaddr,NXTACCTSECRET);
        memset(sortbuf,0,sizeof(sortbuf));
        n = sort_all_buckets(sortbuf,keyhash);
        if ( n != 0 )
        {
            printf("search n.%d sorted mydist.%d remoteflag.%d remoteaccess.%d\n",n,mydist,remoteflag,is_remote_access(previpaddr));
            if ( is_remote_access(previpaddr) == 0 || remoteflag != 0 ) // propagate to closer nodes or final node does local broadcast
            {
                keynp = get_NXTacct(&createdflag,Global_mp,key);
                keynp->bestdist = 10000;
                keynp->bestbits = 0;
                for (iter=z=0; iter<2; iter++)
                {
                    for (i=0; i<n; i++)
                    {
                        destbits = sortbuf[(i<<1) + 1];
                        dist = bitweight(destbits ^ keyhash);
                        if ( ismynxtbits(destbits) == 0 && (dist < mydist || (iter == 1 && dist < KADEMLIA_MINTHRESHOLD)) )
                        {
                            expand_nxt64bits(destNXTaddr,destbits);
                            np = get_NXTacct(&createdflag,Global_mp,destNXTaddr);
                            if ( Debuglevel > 1 )
                                printf("call %llu (%s) dist.%d mydist.%d ipbits.%x vs %x\n",(long long)destbits,cmd,bitweight(destbits ^ keyhash),mydist,np->stats.ipbits,calc_ipbits(previpaddr));
                            if ( np->stats.ipbits != 0 && np->stats.ipbits != calc_ipbits(previpaddr) )
                            {
                                if ( remoteflag != 0 && origargstr != 0 )
                                    passthrough_packet(destNXTaddr,origargstr,datastr);
                                else txid = send_kademlia_cmd(destbits,0,cmd,NXTACCTSECRET,key,datastr);
                                if ( z++ > KADEMLIA_NUMK )
                                    break;
                            }
                        }
                    }
                    if ( z > 0 || remoteflag == 0 )
                        break;
                }
            }
        } else printf("no candidate destaddrs\n");
        get_public_datastr(retstr,databuf,keyhash);
        if ( is_remote_access(previpaddr) != 0 && ismynxtbits(senderbits) == 0 && remoteflag == 0 ) // need to respond to sender
        {
            if ( databuf[0] != 0 )
                txid = send_kademlia_cmd(senderbits,0,"store",NXTACCTSECRET,key,databuf);
            else if ( n > 0 )
            {
                gen_havenode_str(databuf,sortbuf,n);
                txid = send_kademlia_cmd(senderbits,0,isvalue==0?"havenode":"havenodeB",NXTACCTSECRET,key,databuf);
            }
            fprintf(stderr,"send back.(%s) to %llu\n",databuf,(long long)senderbits);
        }
    }
    if ( retstr[0] == 0 )
        sprintf(retstr,"{\"result\":\"kademlia_%s from.(%s) previp.(%s) key.(%s) datalen.%ld txid.%llu\"}",cmd,sender,previpaddr,key,datastr!=0?strlen(datastr):0,(long long)txid);
    //if ( Debuglevel > 0 )
        printf("FIND.(%s)\n",retstr);
    return(clonestr(retstr));
}

void update_Kbucket(int32_t bucketid,struct nodestats *buckets[],int32_t n,struct nodestats *stats)
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
            printf("bucketid.%d: Got pong back from eviction candidate in slot.%d\n",bucketid,j);
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
                add_new_node(stats->nxt64bits);
                printf("APPEND.%d: bucket[%d] <- %llu %s then call pushstore\n",bucketid,j,(long long)stats->nxt64bits,ipaddr);
                if ( cp != 0 && ismynxtbits(stats->nxt64bits) == 0 )
                    kademlia_pushstore(PUBLIC_DATA,mynxt64bits(),stats->nxt64bits);
            }
            else if ( j > 0 )
            {
                if ( matchflag != j-1 )
                {
                    for (k=matchflag; k<j-1; k++)
                        buckets[k] = buckets[k+1];
                    buckets[k] = stats;
                    for (k=0; k<j; k++)
                        printf("%llu ",(long long)buckets[k]->nxt64bits);
                    printf("ROTATE.%d: bucket[%d] <- %llu %s | bucketid.%d\n",j-1,matchflag,(long long)stats->nxt64bits,ipaddr,bucketid);
                }
            } else printf("update_Kbucket.%d: impossible case matchflag.%d j.%d\n",bucketid,matchflag,j);
            return;
        }
        else if ( buckets[j] == stats )
            matchflag = j;
    }
    if ( matchflag < 0 ) // stats is new and the bucket is full
    {
        add_new_node(stats->nxt64bits);
        eviction = buckets[0];
        for (k=0; k<n-1; k++)
            buckets[k] = buckets[k+1];
        buckets[n-1] = stats;
        printf("bucket[%d] <- %llu, check for eviction of %llu %s | bucketid.%d\n",n-1,(long long)stats->nxt64bits,(long long)eviction->nxt64bits,ipaddr,bucketid);
        stats->eviction = eviction;
        if ( cp != 0 && ismynxtbits(eviction->nxt64bits) == 0 )
        {
            kademlia_pushstore(PUBLIC_DATA,mynxt64bits(),stats->nxt64bits);
            txid = send_kademlia_cmd(eviction->nxt64bits,0,"ping",cp->srvNXTACCTSECRET,0,0);
            printf("check for eviction with destip.%u txid.%llu | bucketid.%d\n",eviction->ipbits,(long long)txid,bucketid);
        }
    }
}

void update_Kbuckets(struct nodestats *stats,uint64_t nxt64bits,char *ipaddr,int32_t port,int32_t p2pflag,int32_t lastcontact)
{
    static unsigned char zerokey[crypto_box_PUBLICKEYBYTES];
    struct coin_info *cp = get_coin_info("BTCD");
    uint64_t xorbits;
    int32_t bucketid;
    char pubkeystr[512],NXTaddr[64],tmpipaddr[64],*ptr;
    struct pserver_info *pserver;
    if ( stats == 0 )
    {
        printf("update_Kbuckets null stats\n");
        return;
    }
    if ( memcmp(stats->pubkey,zerokey,sizeof(stats->pubkey)) == 0 )
        ptr = 0;
    else
    {
        init_hexbytes_noT(pubkeystr,stats->pubkey,sizeof(stats->pubkey));
        ptr = pubkeystr;
    }
    expand_nxt64bits(NXTaddr,nxt64bits);
    kademlia_update_info(NXTaddr,ipaddr,port,ptr,lastcontact,p2pflag);
    if ( cp != 0 )
    {
        pserver = get_pserver(0,cp->myipaddr,0,0);
        //fprintf(stderr,"call addto_hasips\n");
        expand_ipbits(tmpipaddr,stats->ipbits);
        //if ( notlocalip(tmpipaddr) != 0 )
        //    addto_hasips(1,pserver,stats->ipbits);
        xorbits = cp->srvpubnxtbits;
        if ( stats->nxt64bits != 0 )
        {
            xorbits ^= stats->nxt64bits;
            bucketid = bitweight(xorbits);
            Kbucket_updated[bucketid] = time(NULL);
            //fprintf(stderr,"call update_Kbucket\n");
            update_Kbucket(bucketid,K_buckets[bucketid],KADEMLIA_NUMK,stats);
        }
    }
}

uint64_t refresh_bucket(struct nodestats *buckets[],long lastcontact,char *NXTACCTSECRET)
{
    uint64_t txid = 0;
    int32_t i,r;
    char key[64];
    if ( buckets[0] != 0 && lastcontact >= KADEMLIA_BUCKET_REFRESHTIME )
    {
        for (i=0; i<KADEMLIA_NUMK; i++)
            if ( buckets[i] == 0 )
                break;
        if ( i > 0 )
        {
            r = ((rand()>>8) % i);
            if ( ismynxtbits(buckets[r]->nxt64bits) == 0 )
            {
                expand_nxt64bits(key,buckets[r]->nxt64bits);
                txid = send_kademlia_cmd(buckets[r]->nxt64bits,0,"findnode",NXTACCTSECRET,key,0);
            }
        }
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

#endif
