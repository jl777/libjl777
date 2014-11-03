//
//  deaddrop.h
//  telepathy
//
//  Created by jl777 on 10/13/14.
//  Copyright (c) 2014 jl777. MIT License.
//

#ifndef libtest_deaddrop_h
#define libtest_deaddrop_h

struct telepathy_args
{
    uint64_t mytxid,othertxid,refaddr,bestaddr,refaddrs[8],otheraddrs[8];
    bits256 mypubkey,otherpubkey;
    int numrefs;
};

double calc_nradius(uint64_t *addrs,int32_t n,uint64_t testaddr,double refdist)
{
    int32_t i;
    double dist,sum = 0.;
    if ( n == 0 )
        return(0.);
    for (i=0; i<n; i++)
    {
        dist = (bitweight(addrs[i] ^ testaddr) - refdist);
        sum += (dist * dist);
    }
    if ( sum < 0. )
        printf("huh? sum %f n.%d -> %f\n",sum,n,sqrt(sum/n));
    return(sqrt(sum/n));
}

int32_t Task_mindmeld(void *_args,int32_t argsize)
{
    static bits256 zerokey;
    struct telepathy_args *args = _args;
    int32_t i,j,iter,dist;
    double sum,metric,bestmetric;
    cJSON *json;
    uint64_t calcaddr;
    struct coin_info *cp = get_coin_info("BTCD");
    char key[64],datastr[1024],sender[64],otherkeystr[512],*retstr;
    if ( cp == 0 )
        return(-1);
    if ( memcmp(&args->otherpubkey,&zerokey,sizeof(zerokey)) == 0 )
    {
        expand_nxt64bits(key,args->othertxid);
        gen_randacct(sender);
        retstr = kademlia_find("findvalue",0,cp->srvNXTADDR,cp->srvNXTACCTSECRET,sender,key,0,0);
        if ( retstr != 0 )
        {
            if ( (json= cJSON_Parse(retstr)) != 0 )
            {
                copy_cJSON(datastr,cJSON_GetObjectItem(json,"data"));
                if ( strlen(datastr) == sizeof(zerokey)*2 )
                {
                    printf("set otherpubkey to (%s)\n",datastr);
                    decode_hex(args->otherpubkey.bytes,sizeof(args->otherpubkey),datastr);
                } else printf("Task_mindmeld: unexpected len.%ld\n",strlen(datastr));
                free_json(json);
            }
            free(retstr);
        }
    }
    sum = 0.;
    for (i=0; i<args->numrefs; i++)
    {
        for (j=0; j<args->numrefs; j++)
        {
            if ( i == j )
                dist = bitweight(args->refaddr ^ args->refaddrs[j]);
            else
            {
                dist = bitweight(args->refaddrs[i] ^ args->refaddrs[j]);
                sum += dist;
            }
            printf("%2d ",dist);
        }
        printf("\n");
    }
    printf("dist from privateaddr above -> ");
    sum /= (args->numrefs * args->numrefs - args->numrefs);
    if ( args->bestaddr == 0 )
        randombytes((uint8_t *)&args->bestaddr,sizeof(args->bestaddr));
    bestmetric = calc_nradius(args->refaddrs,args->numrefs,args->bestaddr,(int)sum);
    printf("bestmetric %.3f avedist %.1f\n",bestmetric,sum);
    for (iter=0; iter<1000000; iter++)
    {
        //ind = (iter % 65);
        //if ( ind == 64 )
        if( (iter & 1) != 0 )
            randombytes((unsigned char *)&calcaddr,sizeof(calcaddr));
        else calcaddr = (args->bestaddr ^ (1L << ((rand()>>8)&63)));
        metric = calc_nradius(args->refaddrs,args->numrefs,calcaddr,(int)sum);
        if ( metric < bestmetric )
        {
            bestmetric = metric;
            args->bestaddr = calcaddr;
        }
    }
    for (i=0; i<args->numrefs; i++)
    {
        for (j=0; j<args->numrefs; j++)
        {
            if ( i == j )
                printf("%2d ",bitweight(args->bestaddr ^ args->refaddrs[j]));
            else printf("%2d ",bitweight(args->refaddrs[i] ^ args->refaddrs[j]));
        }
        printf("\n");
    }
    printf("bestaddr.%llu bestmetric %.3f\n",(long long)args->bestaddr,bestmetric);
    init_hexbytes(otherkeystr,args->otherpubkey.bytes,sizeof(args->otherpubkey));
    printf("Other pubkey.(%s)\n",otherkeystr);
    for (i=0; i<args->numrefs; i++)
        printf("%llu ",(long long)args->refaddrs[i]);
    printf("mytxid.%llu othertxid.%llu | myaddr.%llu\n",(long long)args->mytxid,(long long)args->othertxid,(long long)args->refaddr);
    return(0);
}

/*if ( retstr != 0 )
 free(retstr);
 memset(&args,0,sizeof(args));
 args.mytxid = myhash.txid;
 args.othertxid = otherhash.txid;
 args.refaddr = cp->privatebits;
 args.numrefs = scan_nodes(args.refaddrs,sizeof(args.refaddrs)/sizeof(*args.refaddrs),NXTACCTSECRET);
 start_task(Task_mindmeld,"telepathy",1000000,(void *)&args,sizeof(args));
 retstr = clonestr(retbuf);*/


double calc_address_metric(int32_t dispflag,uint64_t refaddr,uint64_t *list,int32_t n,uint64_t calcaddr,double targetdist)
{
    int32_t i,numabove,numbelow,exact,flag = 0;
    double metric,dist,diff,sum,balance;
    metric = bitweight(refaddr ^ calcaddr);
    if ( metric > targetdist )
        return(10000000.);
    exact = 0;
    diff = sum = balance = 0.;
    if ( list != 0 && n != 0 )
    {
        numabove = numbelow = 0;
        for (i=0; i<n; i++)
        {
            if ( list[i] != refaddr )
            {
                dist = bitweight(list[i] ^ calcaddr);
                if ( dist > metric )
                    numabove++;
                else if ( dist < metric )
                    numbelow++;
                else exact++;
                if ( dispflag > 1 )
                    printf("(%llx %.0f) ",(long long)list[i],dist);
                else if ( dispflag != 0 )
                    printf("%.0f ",dist);
                sum += (dist * dist);
                dist -= metric;
                diff += (dist * dist);
            } else flag = 1;
        }
        if ( n == 1 )
            flag = 0;
        balance = fabs(numabove - numbelow);
        balance *= balance * 10;
        sum = sqrt(sum / (n - flag));
        diff = sqrt(diff / (n - flag));
        if ( dispflag != 0 )
            printf("n.%d flag.%d sum %.3f | diff %.3f | exact.%d above.%d below.%d balance %.0f ",n,flag,sum,diff,exact,numabove,numbelow,balance);
    }
    if ( dispflag != 0 )
        printf("dist %.3f -> %.3f %llx %llu\n",metric,(diff + balance)/(exact*exact+1),(long long)refaddr,(long long)refaddr);
    return((cbrt(metric) + diff + balance)/(exact*exact+1));
}

struct loopargs
{
    char refacct[256],bestpassword[4096];
    double best;
    uint64_t bestaddr,*list;
    int32_t abortflag,threadid,numinlist,targetdist,numthreads,duration;
};

void *findaddress_loop(void *ptr)
{
    struct loopargs *args = ptr;
    uint64_t addr,calcaddr;
    int32_t i,n=0;
    double startmilli,metric;
    unsigned char hash[256 >> 3],mypublic[256>>3],pass[49];
    addr = calc_nxt64bits(args->refacct);
    n = 0;
    startmilli = milliseconds();
    while ( args->abortflag == 0 )
    {
        if ( 0 )
        {
            //memset(pass,0,sizeof(pass));
            //randombytes(pass,(sizeof(pass)/sizeof(*pass))-1);
            for (i=0; i<(int)(sizeof(pass)/sizeof(*pass))-1; i++)
            {
                //if ( pass[i] == 0 )
                pass[i] = safechar64((rand() >> 8) % 63);
            }
            pass[i] = 0;
            memset(hash,0,sizeof(hash));
            memset(mypublic,0,sizeof(mypublic));
            calcaddr = conv_NXTpassword(hash,mypublic,(char *)pass);
        }
        else randombytes((unsigned char *)&calcaddr,sizeof(calcaddr));
        if ( bitweight(addr ^ calcaddr) <= args->targetdist )
        {
            metric = calc_address_metric(0,addr,args->list,args->numinlist,calcaddr,args->targetdist);
            if ( metric < args->best )
            {
                metric = calc_address_metric(1,addr,args->list,args->numinlist,calcaddr,args->targetdist);
                args->best = metric;
                args->bestaddr = calcaddr;
                strcpy(args->bestpassword,(char *)pass);
                printf("thread.%d n.%d: best.%.4f -> %llu | %llu calcaddr | ave micros %.3f\n",args->threadid,n,args->best,(long long)args->bestaddr,(long long)calcaddr,1000*(milliseconds()-startmilli)/n);
            }
        }
        n++;
    }
    args->abortflag = -1;
    return(0);
}

char *findaddress(char *previpaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,uint64_t addr,uint64_t *list,int32_t n,int32_t targetdist,int32_t duration,int32_t numthreads)
{
    static double lastbest,endmilli,best,metric;
    static uint64_t calcaddr,bestaddr = 0;
    static char refNXTaddr[64],retbuf[2048],bestpassword[512],bestNXTaddr[64];
    static struct loopargs **args;
    bits256 secret,pubkey;
    int32_t i;
    if ( endmilli == 0. )
    {
        if ( numthreads <= 0 || n < 1 )
            return(0);
        expand_nxt64bits(refNXTaddr,addr);
        if ( numthreads > 28 )
            numthreads = 28;
        best = lastbest = 1000000.;
        bestpassword[0] = bestNXTaddr[0] = 0;
        args = calloc(numthreads,sizeof(*args));
        for (i=0; i<numthreads; i++)
        {
            args[i] = calloc(1,sizeof(*args[i]));
            strcpy(args[i]->refacct,refNXTaddr);
            args[i]->threadid = i;
            args[i]->numthreads = numthreads;
            args[i]->targetdist = targetdist;
            args[i]->best = lastbest;
            args[i]->list = list;
            args[i]->numinlist = n;
            if ( portable_thread_create((void *)findaddress_loop,args[i]) == 0 )
                printf("ERROR hist findaddress_loop\n");
            printf("%d ",i);
        }
        endmilli = milliseconds() + (duration * 1000.);
        printf("%d threads started numinlist.%d\n",numthreads,n);
    }
    else
    {
        addr = calc_nxt64bits(args[0]->refacct);
        list = args[0]->list;
        n = args[0]->numinlist;
        targetdist = args[0]->targetdist;
        numthreads = args[0]->numthreads;
        duration = args[0]->duration;
    }
    //if ( milliseconds() < endmilli )
    {
        best = lastbest;
        calcaddr = 0;
        for (i=0; i<numthreads; i++)
        {
            if ( args[i]->best < best )
            {
                if ( args[i]->bestpassword[0] != 0 )
                    calcaddr = conv_NXTpassword(secret.bytes,pubkey.bytes,args[i]->bestpassword);
                else calcaddr = args[i]->bestaddr;
                //printf("(%llx %f) ",(long long)calcaddr,args[i]->best);
                metric = calc_address_metric(1,addr,list,n,calcaddr,targetdist);
                //printf("-> %f, ",metric);
                if ( metric < best )
                {
                    best = metric;
                    bestaddr = calcaddr;
                    if ( calcaddr != args[i]->bestaddr )
                        printf("error calcaddr.%llx vs %llx\n",(long long)calcaddr,(long long)args[i]->bestaddr);
                    expand_nxt64bits(bestNXTaddr,calcaddr);
                    strcpy(bestpassword,args[i]->bestpassword);
                }
            }
        }
        //printf("best %f lastbest %f %llu\n",best,lastbest,(long long)addr);
        if ( best < lastbest )
        {
            printf(">>>>>>>>>>>>>>> new best (%s) %016llx %llu dist.%d metric %.2f vs %016llx %llu\n",bestpassword,(long long)calcaddr,(long long)calcaddr,bitweight(addr ^ bestaddr),best,(long long)addr,(long long)addr);
            lastbest = best;
        }
        //printf("milli %f vs endmilli %f\n",milliseconds(),endmilli);
    }
    if ( milliseconds() >= endmilli )
    {
        for (i=0; i<numthreads; i++)
            args[i]->abortflag = 1;
        for (i=0; i<numthreads; i++)
        {
            while ( args[i]->abortflag != -1 )
                sleep(1);
            free(args[i]);
        }
        free(args);
        args = 0;
        metric = calc_address_metric(2,addr,list,n,bestaddr,targetdist);
        free(list);
        endmilli = 0;
        sprintf(retbuf,"{\"result\":\"metric %.3f\",\"privateaddr\":\"%s\",\"password\":%s\",\"dist\":%d,\"targetdist\":%d}",best,bestNXTaddr,bestpassword,bitweight(addr ^ bestaddr),targetdist);
        printf("FINDADDRESS.(%s)\n",retbuf);
        return(clonestr(retbuf));
    }
    return(0);
}

#endif

