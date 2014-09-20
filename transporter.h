//
//  transporter.h
//  xcode
//
//  Created by jl777 on 8/6/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_televolve_h
#define xcode_televolve_h

char *set_Pendingjson(int32_t *maxlenp,char **pending_ptrp,cJSON *json)
{
    long len;
    char *pending_jsonstr,*jsonstr = 0;
    pending_jsonstr = *pending_ptrp;
    if ( json != 0 )
        jsonstr = cJSON_Print(json);
    if ( pending_jsonstr == 0 )
        pending_jsonstr = jsonstr;
    else if ( jsonstr != 0 )
    {
        len = strlen(jsonstr);
        if ( (len+1) > *maxlenp )
        {
            *maxlenp = (int32_t)(len + 1);
            pending_jsonstr = realloc(pending_jsonstr,*maxlenp);
        }
        strcpy(pending_jsonstr,jsonstr);
        free(jsonstr);
    }
    *pending_ptrp = pending_jsonstr;
    return(pending_jsonstr);
}

struct transporter_log *find_transporter_log(struct coin_info *cp,char *otherpubaddr,uint32_t totalcrc,int32_t dir)
{
    struct transporter_log *log = 0;
    int32_t i;
    for (i=0; i<cp->numlogs; i++)
    {
        if ( (log= cp->logs[i]) != 0 && log->totalcrc == totalcrc && log->dir == dir && strcmp(log->otherpubaddr,otherpubaddr) == 0 )
            return(log);
        printf("%d: %s.%u dir.%d\n",i,log->otherpubaddr,log->totalcrc,log->dir);
    }
    printf("couldnt find crc.%u NXT.%s dir.%d\n",totalcrc,otherpubaddr,dir);
    return(0);
}

struct transporter_log *add_transporter_log(struct coin_info *cp,struct transporter_log *log)
{
    printf("check ADD log.%d %p dir.%d crc.%u other.%s\n",cp->numlogs,log,log->dir,log->totalcrc,log->otherpubaddr);
    if ( find_transporter_log(cp,log->otherpubaddr,log->totalcrc,log->dir) != 0 )
        return(0);
    printf("ADD log.%d %p dir.%d\n",cp->numlogs,log,log->dir);
    cp->logs = (void **)realloc(cp->logs,sizeof(*cp->logs) * (cp->numlogs+1));
    cp->logs[cp->numlogs] = log;
    cp->numlogs++;
    return(log);
}

double calc_telepod_metric(struct telepod **pods,int32_t n,uint64_t satoshis,uint32_t height)
{
    int32_t i;
    double metric;
    int64_t age,youngest,agesum = 0,sum = 0;
    youngest = -1;
    for (i=0; i<n; i++)
    {
        age = (height - pods[i]->height);
        agesum += age;
        if ( youngest < 0 || age < youngest )
            youngest = age;
        sum += pods[i]->satoshis;
    }
    agesum /= n;
    metric = n * sqrt((double)agesum / (youngest + 1)) + (satoshis - sum + 1);
    printf("metric.%f youngest.%lld agesum %llu n %d, (%lld - %lld)\n",metric,(long long)youngest,(long long)agesum,n,(long long)satoshis,(long long)sum);
    if ( sum < satoshis || youngest < 0 )
        return(-1.);
    return(metric);
}

int32_t set_inhwm_flags(struct telepod **hwmpods,int32_t numhwm,struct telepod **allpods,int32_t n)
{
    int32_t i,inhwm = 0;;
    for (i=0; i<n; i++)
        allpods[i]->inhwm = 0;
    for (i=0; i<numhwm; i++)
        hwmpods[i]->inhwm = 1, inhwm++;
    return(n - inhwm);
}

struct telepod **evolve_podlist(int32_t *hwmnump,struct telepod **hwmpods,struct telepod **allpods,int32_t n,int32_t maxiters,uint64_t satoshis,uint32_t height)
{
    int32_t i,j,k,finished,numtestpods,nohwm,replacei,numhwm = *hwmnump;
    struct telepod **testlist;
    double metric,bestmetric;
    bestmetric = calc_telepod_metric(hwmpods,numhwm,satoshis,height);
    testlist = calloc(n,sizeof(*testlist));
    set_inhwm_flags(hwmpods,numhwm,allpods,n);
    finished = 0;
    if ( numhwm == 0 || n == 0 )
        return(0);
    nohwm = 0;
    for (i=0; i<maxiters; i++)
    {
        if ( ++nohwm > 100 )
            break;
        memcpy(testlist,hwmpods,sizeof(*testlist) * numhwm);
        replacei = (rand() % numhwm);
        for (j=0; j<n; j++)
        {
            printf("i.%d of %d, replacei.%d j.%d inhwm.%d\n",i,maxiters,replacei,j,allpods[j]->inhwm);
            if ( allpods[j]->inhwm == 0 )
            {
                testlist[replacei] = allpods[j];
                numtestpods = numhwm;
                for (k=0; k<2; k++)
                {
                    metric = calc_telepod_metric(testlist,numtestpods,satoshis,height);
                    printf("i.%d of %d, replacei.%d j.%d k.%d metric %f\n",i,maxiters,replacei,j,k,metric);
                    if ( metric >= 0 && (bestmetric < 0 || metric < bestmetric) )
                    {
                        bestmetric = metric;
                        memcpy(hwmpods,testlist,numtestpods * sizeof(*hwmpods));
                        *hwmnump = numhwm = numtestpods;
                        printf("new HWM i.%d j.%d k.%d replacei.%d bestmetric %f, numtestpods.%d\n",i,j,k,replacei,bestmetric,numtestpods);
                        nohwm = 0;
                        if ( metric <= 1.0 )//set_inhwm_flags(hwmpods,numhwm,allpods,n) == 0 )
                        {
                            finished = 1;
                            break;
                        }
                    }
                    if ( k == 0 )
                    {
                        if ( numtestpods > 1 )
                        {
                            testlist[rand() % numtestpods] = testlist[numtestpods - 1];
                            numtestpods--;
                        } else break;
                    }
                }
                if ( finished != 0 )
                    break;
            }
            if ( finished != 0 )
                break;
        }
    }
    free(testlist);
    hwmpods = realloc(hwmpods,sizeof(*hwmpods) * numhwm);
    return(hwmpods);
}

struct telepod **evolve_transporter(int32_t *nump,int32_t maxiters,struct coin_info *cp,int32_t minage,uint64_t satoshis,uint32_t height)
{
    int32_t i,n,bestn = 0;
    uint64_t sum,balance = 0;
    struct telepod **allpods,**hwmpods = 0;
    allpods = gather_telepods(&n,&balance,cp,minage);
    if ( allpods != 0 )
    {
        if ( balance < satoshis )
        {
            free(allpods);
            return(0);
        }
        sum = 0;
        for (i=0; i<n; i++)
        {
            sum += allpods[i]->satoshis;
            if ( sum >= satoshis )
                break;
        }
        //printf("i.%d of n.%d\n",i,n);
        if ( i == n )
        {
            free(allpods);
            return(0);
        }
        else n = (i+1);
    }
    printf("evolve maxiters.%d with n.%d\n",maxiters,n);
    if ( n > 0 )
    {
        bestn = n;
        hwmpods = calloc(n+1,sizeof(*hwmpods));
        memcpy(hwmpods,allpods,n * sizeof(*hwmpods));
        if ( maxiters > 0 )
            hwmpods = evolve_podlist(&bestn,hwmpods,allpods,n,maxiters,satoshis,height);
        for (i=0; i<bestn; i++)
            disp_telepod("hwm",hwmpods[i]);
        free(allpods);
    }
    printf("-> evolved with bestn.%d %p\n",bestn,hwmpods);
    *nump = bestn;
    return(hwmpods);
}

cJSON *gen_podjson(struct telepod *pod)
{
    char numstr[64];
    cJSON *json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"podstate",cJSON_CreateNumber(pod->podstate));
    cJSON_AddItemToObject(json,"filenum",cJSON_CreateNumber(pod->filenum));
    cJSON_AddItemToObject(json,"crc",cJSON_CreateNumber(pod->crc));
    sprintf(numstr,"%.8f",dstr(pod->satoshis));
    cJSON_AddItemToObject(json,"value",cJSON_CreateString(numstr));
    return(json);
}

cJSON *create_transporter_json(uint32_t *totalcrcp,char *coinstr,uint32_t height,int32_t minage,uint64_t satoshis,struct telepod **pods,int32_t n,int32_t M,int32_t N,uint8_t *sharenrs,int32_t logstate)
{
    int32_t i;
    struct coin_info *cp;
    uint32_t totalcrc = 0;
    char numstr[1024];
    cJSON *json,*array;
    *totalcrcp = 0;
    cp = get_coin_info(coinstr);
    if ( pods == 0 || n == 0 || cp == 0 )
        return(0);
    json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"requestType",cJSON_CreateString("transporter"));
    cJSON_AddItemToObject(json,"coin",cJSON_CreateString(coinstr));
    cJSON_AddItemToObject(json,"pubaddr",cJSON_CreateString(cp->pubaddr));
    cJSON_AddItemToObject(json,"height",cJSON_CreateNumber(height));
    cJSON_AddItemToObject(json,"minage",cJSON_CreateNumber(minage));
    cJSON_AddItemToObject(json,"M",cJSON_CreateNumber(M));
    cJSON_AddItemToObject(json,"N",cJSON_CreateNumber(N));
    if ( N > 1 )
    {
        init_hexbytes(numstr,sharenrs,N);
        printf("sharenrs.(%s) -> [%s]\n",sharenrs,numstr);
        if ( numstr[0] != 0 )
            cJSON_AddItemToObject(json,"sharenrs",cJSON_CreateString(numstr));
    }
    sprintf(numstr,"%.8f",dstr(satoshis));
    cJSON_AddItemToObject(json,"value",cJSON_CreateString(numstr));
    array = cJSON_CreateArray();
    for (i=0; i<n; i++)
    {
        sprintf(numstr,"%u",pods[i]->crc);
        cJSON_AddItemToArray(array,cJSON_CreateString(numstr));
        totalcrc = _crc32(totalcrc,&pods[i]->crc,sizeof(pods[i]->crc));
    }
    cJSON_AddItemToObject(json,"telepods",array);
    *totalcrcp = totalcrc;
    cJSON_AddItemToObject(json,"totalcrc",cJSON_CreateNumber(totalcrc));
    if ( logstate != 0 )
        cJSON_AddItemToObject(json,"state",cJSON_CreateNumber(logstate));
    return(json);
}

cJSON *gen_logjson(struct transporter_log *log)
{
    cJSON *array,*item,*logjson = cJSON_CreateObject();
    int32_t i;
    logjson = create_transporter_json(&log->totalcrc,log->cp->name,log->createheight,log->minage,log->satoshis,log->pods,log->numpods,log->M,log->N,log->sharenrs,log->logstate);
    array = cJSON_CreateArray();
    for (i=0; i<log->numpods; i++)
        if ( (item= gen_podjson(log->pods[i])) != 0 )
            cJSON_AddItemToArray(array,item);
    cJSON_AddItemToObject(logjson,"telepods",array);
    return(logjson);
}

cJSON *gen_coin_logjsons(struct coin_info *cp)
{
    int32_t i;
    cJSON *item,*array = cJSON_CreateArray();
    for (i=0; i<cp->numlogs; i++)
        if ( (item= gen_logjson(cp->logs[i])) != 0 )
            cJSON_AddItemToArray(array,item);
    return(array);
}

int32_t save_transporter_log(struct transporter_log *log)
{
    int32_t loglen,numerrs = 0;
    char fname[512];
    cJSON *json;
    long len;
    struct coin_info *cp = log->cp;
    uint8_t *encrypted,*data;
    if ( cp == 0 )
    {
        printf("illegal cp in save_transporter_log\n");
        return(-1);
    }
    ensure_directory("backups/logs");
    sprintf(fname,"backups/logs/%s.%u",log->otherpubaddr,log->totalcrc);
    if ( (json= gen_coin_logjsons(cp)) != 0 )
        set_Pendingjson(&cp->pending_ptrmaxlen,&cp->pending_ptr,json);
    len = cp->pending_ptr != 0 ? strlen(cp->pending_ptr) : 0;
    loglen = (int32_t)len + 1 + sizeof(*log) + (log->numpods * (sizeof(*log->crcs) + sizeof(*log->filenums)));
    data = calloc(1,loglen);
    if ( cp->pending_ptr != 0 )
        memcpy(data,cp->pending_ptr,len+1);
    memcpy(data+len+1,log,sizeof(*log));
    memcpy(data+len+1+sizeof(*log),log->crcs,sizeof(*log->crcs)*log->numpods);
    memcpy(data+len+1+sizeof(*log)+sizeof(*log->crcs)*log->numpods,log->filenums,sizeof(*log->filenums)*log->numpods);
    if ( (encrypted= save_encrypted(fname,cp,data,&loglen)) != 0 )
        free(encrypted);
    else printf("ERROR saving %s\n",fname), numerrs++;
    if ( json != 0 )
        free_json(json);
    return(-numerrs);
}

struct transporter_log *create_transporter_log(char *NXTaddr,char *refcipher,cJSON *ciphersobj,int32_t dir,struct NXT_acct *othernp,struct coin_info *cp,int32_t minage,uint64_t satoshis,struct telepod **pods,int32_t n,int32_t M,int32_t N,uint8_t *sharenrs,char *otherpubaddr)
{
    struct transporter_log *log = calloc(1,sizeof(*log));
    log->dir = dir;
    log->cp = cp;
    strcpy(log->otherpubaddr,otherpubaddr);
    strcpy(log->mypubaddr,cp->pubaddr);
    //safecopy(log->NXTACCTSECRET,NXTACCTSECRET,sizeof(log->NXTACCTSECRET));
    log->minage = minage;
    log->satoshis = satoshis;
    log->pods = calloc(n,sizeof(*log->pods));
    if ( pods != 0 )
        memcpy(log->pods,pods,n*sizeof(*log->pods));
    //safecopy(log->refcipher,refcipher,sizeof(log->refcipher));
    //log->ciphersobj = ciphersobj;
    log->numpods = n;
    log->M = M;
    log->N = N;
    log->createheight = (uint32_t)get_blockheight(cp);
    memcpy(log->sharenrs,sharenrs,sizeof(log->sharenrs));
    log->crcs = calloc(n,sizeof(*log->crcs));
    log->filenums = calloc(n,sizeof(*log->filenums));
    return(log);
}

struct transporter_log *send_transporter_log(char *NXTaddr,char *NXTACCTSECRET,struct NXT_acct *destnp,struct coin_info *cp,int32_t minage,uint64_t satoshis,struct telepod **pods,int32_t n,int32_t M,int32_t N,uint8_t *sharenrs,char *otherpubaddr)
{
    int32_t i,err;
    cJSON *json;
    struct transporter_log *log;
    log = create_transporter_log(NXTaddr,cp->name,cp->ciphersobj,TRANSPORTER_SEND,destnp,cp,minage,satoshis,pods,n,M,N,sharenrs,otherpubaddr);
    for (i=0; i<n; i++)
    {
        pods[i]->dir = TRANSPORTER_SEND;
        update_telepod_file(cp,pods[i]->filenum,pods[i]);
        log->crcs[i] = pods[i]->crc;
        log->filenums[i] = pods[i]->filenum;
    }
    log->logstate = TRANSPORTER_SEND;
    save_transporter_log(log);
    json = create_transporter_json(&log->totalcrc,cp->name,log->createheight,minage,satoshis,pods,n,M,N,sharenrs,0);
    add_transporter_log(cp,log);
    if ( json != 0 )
    {
        err = sendandfree_jsoncmd(Global_mp->Lfactor,NXTaddr,NXTACCTSECRET,json,destnp->H.U.NXTaddr);
        printf("AFTER send_transporter_log.%u to %s err.%d height.%d %d at %f\n",log->totalcrc,destnp->H.U.NXTaddr,err,log->createheight,(uint32_t)get_blockheight(cp),log->startmilli);
        if ( err == 0 )
        {
            log->startmilli = milliseconds();
            printf("AFTER send_transporter_log to %s err.%d height.%d %d at %f\n",destnp->H.U.NXTaddr,err,log->createheight,(uint32_t)get_blockheight(cp),log->startmilli);
            queue_enqueue(&Transporter_sendQ.pingpong[0],log);
        }
    }
    else
    {
        free(log);
        log = 0;
    }
    return(log);
}

char *got_transporter_status(char *NXTACCTSECRET,char *sender,char *coinstr,int32_t status,uint32_t totalcrc,uint64_t value,int32_t num,uint32_t *crcs,int32_t ind,int32_t minage,uint32_t height,int32_t sharei,int32_t M,int32_t N,uint8_t *sharenrs,char *otherpubaddr)
{
    char verifiedNXTaddr[64],retbuf[4096];
    struct coin_info *cp;
    struct transporter_log *log;
    struct NXT_acct *destnp,*np;
    int32_t createdflag,completed,mismatches,i;
    printf("got transporter status.%d %s %.8f totalcrc.%u num.%d ind.%d crcs.%p minage.%d, M.%d N.%d other.%s\n",status,coinstr,dstr(value),totalcrc,num,ind,crcs,minage,M,N,otherpubaddr);
    sprintf(retbuf,"{\"status\":%d,\"num\":%d,\"totalcrc\":%u,\"value\":%.8f}",status,num,totalcrc,dstr(value));
    verifiedNXTaddr[0] = 0;
    np = find_NXTacct(verifiedNXTaddr,NXTACCTSECRET);
    destnp = get_NXTacct(&createdflag,Global_mp,sender);
    cp = get_coin_info(coinstr);
    if ( (log= find_transporter_log(cp,otherpubaddr,totalcrc,TRANSPORTER_SEND)) != 0 )
    {
        if ( crcs == 0 )
        {
            log->status = status;
            log->recvmilli = milliseconds();
            log->logstate |= TRANSPORTER_GOTACK;
            save_transporter_log(log);

            printf("RECV LOG from receiver %02x %02x %02x\n",sharenrs[0],sharenrs[1],sharenrs[2]);
            sprintf(retbuf,"{\"result\":\"%s\",\"status\":%d,\"num\":%d,\"totalcrc\":%u,\"value\":%.8f}",status==0?"ready to start teleporting":"error reported from other side",status,num,totalcrc,dstr(value));
        }
        else
        {
            if ( ind >= 0 && ind < log->numpods && log->pods[ind] != 0 )
            {
                log->crcs[ind] = crcs[ind];
                completed = mismatches = 0;
                for (i=0; i<log->numpods; i++)
                {
                    if ( log->pods[i] != 0 && log->pods[i]->log != log )
                        log->pods[i]->log = log;
                    if ( crcs[i] == log->pods[i]->crc )
                    {
                        if ( log->pods[i]->gotack == 0 )
                        {
                            log->pods[i]->gotack = 1;
                            change_podstate(cp,log->pods[i],PODSTATE_GOTACK);
                        }
                        completed++;
                    }
                    else mismatches++;
                }
                if ( completed == log->numpods && log->completemilli == 0. )
                {
                    log->completemilli = milliseconds();
                    log->logstate |= TRANSPORTER_TRANSFERRED;
                    save_transporter_log(log);
                    printf(">>>>>>>>>>>> completed send and ack of all telepods to NXT.%s for %s %.8f | elapsed %.2f seconds\n",sender,cp->name,dstr(value),(log->completemilli - log->recvmilli)/1000.);
                }
                sprintf(retbuf,"{\"result\":\"got crcs\",\"completed\":%d,\"mismatch\":%d,\"num\":%d,\"totalcrc\":%u}",completed,mismatches,num,totalcrc);
            }
            else printf("illegal ind.%d or null pod.%p\n",ind,log->pods[ind]);
        }
    }
    return(clonestr(retbuf));
}

void send_transporter_status(char *verifiedNXTaddr,char *NXTACCTSECRET,struct NXT_acct *destnp,struct coin_info *cp,uint32_t height,uint32_t totalcrc,uint64_t value,int32_t errflag,int32_t num,uint32_t *crcs,int32_t minage,int32_t ind,int32_t sharei,int32_t M,int32_t N,uint8_t *sharenrs)
{
    int32_t i;
    char msg[1024],buf[1024],*retstr;
    sprintf(msg,"{\"requestType\":\"transporter_status\",\"status\":%d,\"coin\":\"%s\",\"totalcrc\":%u,\"height\":%u,\"minage\":%d,\"value\":%.8f,\"num\":%d,\"ind\":%d,\"sharei\":%d,\"M\":%d,\"N\":%d,\"pubaddr\":\"%s\"",errflag,cp!=0?cp->name:"ERROR",totalcrc,height,minage,dstr(value),num,ind,sharei,M,N,cp->pubaddr);
    if ( N > 1 )
    {
        init_hexbytes(buf,sharenrs,N);
        //printf("send sharenrs.(%s) -> [%s]\n",sharenrs,buf);
        sprintf(msg+strlen(msg),",\"sharenrs\":\"%s\"",buf);
    }
    if ( crcs != 0 )
    {
        strcpy(msg+strlen(msg),",\"crcs\":[");
        for (i=0; i<num; i++)
        {
            sprintf(msg+strlen(msg),"%u",crcs[i]);
            if ( i < num-1 )
                strcat(msg,",");
        }
        strcat(msg,"]}");
    }
    else strcat(msg,"}");
    retstr = send_tokenized_cmd(Global_mp->Lfactor,verifiedNXTaddr,NXTACCTSECRET,msg,destnp->H.U.NXTaddr);
    if ( retstr != 0 )
    {
        printf("send_transporter_ACK.(%s)\n",retstr);
        free(retstr);
    }
}

char *transporter_received(char *sender,char *NXTACCTSECRET,char *coinstr,uint32_t totalcrc,uint32_t height,uint64_t value,int32_t minage,uint32_t *crcs,int32_t num,int32_t M,int32_t N,uint8_t *sharenrs,char *otherpubaddr)
{
    struct transporter_log *log;
    char retbuf[4096],verifiedNXTaddr[64];
    struct coin_info *cp;
    int32_t createdflag,errflag = -1;
    struct NXT_acct *sendernp,*np;
    verifiedNXTaddr[0] = 0;
    np = find_NXTacct(verifiedNXTaddr,NXTACCTSECRET);
    sprintf(retbuf,"transporter_received from NXT.%s totalcrc.%u n.%d height.%d %.8f minage.%d M.%d N.%d",sender,totalcrc,num,height,dstr(value),minage,M,N);
    cp = get_coin_info(coinstr);
    sendernp = get_NXTacct(&createdflag,Global_mp,sender);
    if ( cp == 0 || totalcrc == 0 || minage <= 0 || height == 0 || value == 0 || num <= 0 )
        sprintf(retbuf+strlen(retbuf)," <<<<< ERROR"), errflag = -1;
    else
    {
        log = create_transporter_log(verifiedNXTaddr,cp->name,cp->ciphersobj,TRANSPORTER_RECV,sendernp,cp,minage,value,0,num,M,N,sharenrs,otherpubaddr);
        if ( _crc32(0,crcs,num * sizeof(*crcs)) == totalcrc )
        {
            log->totalcrc = totalcrc;
            memcpy(log->crcs,crcs,num*sizeof(*crcs));
            errflag = 0;
            log->logstate = TRANSPORTER_RECV;
            log->startmilli = milliseconds();
            save_transporter_log(log);
            add_transporter_log(cp,log);
            queue_enqueue(&Transporter_recvQ.pingpong[0],log);
            printf(">>>>>>>>>> No errors, allocated transporter_log.%u\n",log->totalcrc);
        }
        else
        {
            sprintf(retbuf + strlen(retbuf)," totalcrc %u vs %u ERROR",totalcrc,_crc32(0,crcs,num * sizeof(*crcs)));
            printf("ERROR CRC (%s)\n",retbuf);
        }
    }
    send_transporter_status(verifiedNXTaddr,NXTACCTSECRET,sendernp,cp,height,totalcrc,value,errflag,num,0,minage,0,0xff,M,N,sharenrs);
    return(clonestr(retbuf));
}


#endif
