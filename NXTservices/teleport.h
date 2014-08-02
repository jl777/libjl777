//
//  teleport.h
//  xcode
//
//  Created by jl777 on 8/1/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//
// todo: encrypt local telepods, monitor blockchain, build acct history

#ifndef xcode_teleport_h
#define xcode_teleport_h

#define TELEPORT_TRANSPORTER_TIMEOUT 10
#define TELEPORT_TELEPOD_TIMEOUT 60

#define TELEPOD_CONTENTS_VOUT 0
#define TELEPOD_CHANGE_VOUT 1   // vout 0 is for the pod contents and last one (1 if no change or 2) is marker

queue_t podQ;
struct telepod
{
    uint32_t filenum,saved,xfered,height,sentheight,crc;
    uint16_t podsize,vout,cloneout,inhwm;
    uint64_t satoshis,modified;
    struct telepod *localclone;
    char coinstr[8],txid[MAX_COINTXID_LEN],clonetxid[MAX_COINTXID_LEN];
    char coinaddr[MAX_COINADDR_LEN],cloneaddr[MAX_COINADDR_LEN],pubkey[128];
    char privkey[];
};

void disp_telepod(char *msg,struct telepod *pod)
{
    printf("%6s %13.8f height.%-6d %6s %s %s/vout_%d\n",msg,dstr(pod->satoshis),pod->height,pod->coinstr,pod->coinaddr,pod->txid,pod->vout);
}

void set_telepod_fname(char *fname,char *coinstr,int32_t podnumber)
{
    sprintf(fname,"backups/telepods/%s.%d",coinstr,podnumber);
}

uint32_t calc_telepodcrc(struct telepod *pod)
{
    return(_crc32(0,(void *)((long)&pod->crc + sizeof(pod->crc)),pod->podsize - ((long)&pod->crc - (long)pod)));
}

int32_t update_telepod_file(char *coinstr,int32_t filenum,struct telepod *pod,char *NXTACCTSECRET)
{
    //jl777: encrypt with NXTACCTSECRET
    FILE *fp;
    int32_t retval = -1;
    char fname[512];
    set_telepod_fname(fname,coinstr,filenum);
    if ( (fp= fopen(fname,"wb")) != 0 )
    {
        pod->saved = 1;
        pod->filenum = filenum;
        if ( fwrite(pod,1,pod->podsize,fp) != pod->podsize )
        {
            pod->saved = 0;
            printf("error saving refpod.(%s)\n",fname);
        }
        else
        {
            printf("(%s) ",fname);
            disp_telepod("update",pod);
            retval = 0;
        }
        fclose(fp);
        fp = 0;
    }
    return(retval);
}

struct telepod *_load_telepod(char *coinstr,int32_t podnumber,char *NXTACCTSECRET)
{
    //jl777: encrypt with NXTACCTSECRET
    FILE *fp;
    long fsize;
    char fname[512];
    struct telepod *pod = 0;
    set_telepod_fname(fname,coinstr,podnumber);
    if ( (fp= fopen(fname,"rb")) != 0 )
    {
        fseek(fp,0,SEEK_END);
        fsize = ftell(fp);
        rewind(fp);
        pod = calloc(1,fsize);
        if ( fread(pod,1,fsize,fp) != fsize )
        {
            printf("error loading refpod.(%s)\n",fname);
            free(pod);
            pod = 0;
        }
        else if ( pod->podsize != fsize )
        {
            printf("telepod corruption: podsize.%d != fsize.%ld\n",pod->podsize,fsize);
            free(pod);
            pod = 0;
        }
        else if ( pod->filenum != 0 && pod->filenum != podnumber )
        {
            printf("warning: pod number.%d -> %d\n",pod->filenum,podnumber);
            pod->filenum = podnumber;
        }
        if ( calc_telepodcrc(pod) != pod->crc )
        {
            printf("telepod corruption: crc mistatch.%08x != %08x\n",calc_telepodcrc(pod),pod->crc);
            free(pod);
            pod = 0;
        }
        fclose(fp);
    }
    return(pod);
}

int32_t truncate_telepod_file(struct telepod *pod,char *NXTACCTSECRET)
{
    long err;
    struct telepod *loadpod;
    if ( (loadpod= _load_telepod(pod->coinstr,pod->filenum,NXTACCTSECRET)) != 0 )
    {
        if ( pod->podsize != loadpod->podsize )
        {
            printf("truncate telepod: loaded pod doesnt match size %d vs %d\n",pod->podsize,loadpod->podsize);
            free(loadpod);
            return(-1);
        }
        else if ( (err= memcmp(pod,loadpod,pod->podsize)) != 0 )
        {
            printf("truncate telepod: memcmp error at %ld\n",err);
            free(loadpod);
            return(-1);
        }
        return(0);
    } else printf("no telepod file %s.%d??\n",pod->coinstr,pod->filenum);
    return(-2);
}

struct telepod *create_telepod(int32_t saveflag,struct coin_info *cp,uint64_t satoshis,char *podaddr,char *pubkey,char *privkey,char *txid,int32_t vout)
{
    struct telepod *pod;
    int32_t size,len;
    len = (int32_t)strlen(privkey);
    size = (int32_t)(sizeof(*pod) + len + 1);
    pod = calloc(1,size);
    pod->height = (uint32_t)get_blockheight(cp);
    pod->podsize = size;
    pod->vout = vout;
    pod->satoshis = satoshis;
    safecopy(pod->coinstr,cp->name,sizeof(pod->coinstr));
    safecopy(pod->txid,txid,sizeof(pod->txid));
    safecopy(pod->coinaddr,podaddr,sizeof(pod->coinaddr));
    safecopy(pod->pubkey,pubkey,sizeof(pod->pubkey));
    safecopy(pod->privkey,privkey,len+1);
    pod->crc = calc_telepodcrc(pod);
    disp_telepod("create",pod);
    return(pod);
}

struct telepod *find_telepod(struct coin_info *cp,char *pubkey)
{
    uint64_t hashval;
    if ( cp == 0 )
        return(0);
    hashval = MTsearch_hashtable(&cp->telepods,pubkey);
    if ( hashval == HASHSEARCH_ERROR )
        return(0);
    else return(cp->telepods->hashtable[hashval]);
}

int32_t ensure_telepod_has_backup(struct coin_info *cp,struct telepod *refpod,char *NXTACCTSECRET)
{
    FILE *fp = 0;
    char fname[512];
    struct telepod *pod;
    int32_t createdflag;
    if ( (pod= find_telepod(cp,refpod->pubkey)) == 0 || pod->vout != refpod->vout )
    {
        pod = MTadd_hashtable(&createdflag,&cp->telepods,pod->pubkey);
        if ( refpod->saved == 0 )
        {
            do
            {
                if ( fp != 0 )
                {
                    fseek(fp,0,SEEK_END);
                    if ( ftell(fp) == 0 )
                    {
                        fclose(fp);
                        printf("found empty slot.%d for telepod\n",cp->savedtelepods);
                        break;
                    }
                    fclose(fp);
                    fp = 0;
                    cp->savedtelepods++;
                }
                set_telepod_fname(fname,cp->name,cp->savedtelepods);
                printf("check (%s)\n",fname);
            }
            while ( (fp= fopen(fname,"rb")) != 0 );
            if ( update_telepod_file(cp->name,cp->savedtelepods,refpod,NXTACCTSECRET) == 0 )
                cp->savedtelepods++;
        }
    }
    if ( fp != 0 )
        fclose(fp);
    if ( refpod->saved == 0 )
        return(-1);
    else return(0);
}

int32_t load_telepods(struct coin_info *cp,int32_t maxnofile,char *NXTACCTSECRET)
{
    int nofile = 0;
    struct telepod *pod;
    if ( cp->telepods != 0 )
        return(cp->savedtelepods);
    cp->telepods = hashtable_create("telepods",HASHTABLES_STARTSIZE,sizeof(*pod),((long)&pod->pubkey[0] - (long)pod),sizeof(pod->pubkey),((long)&pod->modified - (long)pod));
    while ( nofile < maxnofile )
    {
        if ( (pod= _load_telepod(cp->name,cp->savedtelepods,NXTACCTSECRET)) != 0 )
        {
            if ( pod->xfered == 0 )
            {
                ensure_telepod_has_backup(cp,pod,NXTACCTSECRET);
                disp_telepod("load",pod);
            } else disp_telepod("spent",pod);
            cp->savedtelepods++;
        } else nofile++;
    }
    return(cp->savedtelepods);
}

struct telepod **gather_telepods(int32_t *nump,uint64_t *balancep,struct coin_info *cp,int32_t minage)
{
    int32_t i,numactive = 0;
    int64_t changed = 0;
    uint32_t height;
    void **rawlist;
    struct telepod *pod,**pods = 0;
    *nump = 0;
    *balancep = 0;
    rawlist = hashtable_gather_modified(&changed,cp->telepods,1); // no MT support (yet)
    if ( rawlist != 0 && changed != 0 )
    {
        height = (uint32_t)get_blockheight(cp);
        pods = calloc(changed,sizeof(*pods));
        for (i=0; i<changed; i++)
        {
            pod = rawlist[i];
            if ( (height - pod->height) > minage )
            {
                (*balancep) += pod->satoshis;
                pods[numactive++] = pod;
            }
        }
    }
    free(rawlist);
    if ( pods != 0 && numactive > 0 )
    {
        pods = realloc(pods,sizeof(*pods) * numactive);
        *nump = numactive;
        return(pods);
    }
    else
    {
        free(pods);
        return(0);
    }
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
    if ( sum < satoshis )
        return(-1.);
    agesum /= n;
    metric = n * sqrt(agesum / (youngest + 1)) + (satoshis - sum);
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
    int32_t i,j,k,finished,numtestpods,replacei,numhwm = *hwmnump;
    struct telepod **testlist;
    double metric,bestmetric;
    bestmetric = calc_telepod_metric(hwmpods,numhwm,satoshis,height);
    testlist = calloc(n,sizeof(*testlist));
    set_inhwm_flags(hwmpods,numhwm,allpods,n);
    finished = 0;
    for (i=0; i<maxiters; i++)
    {
        memcpy(testlist,hwmpods,sizeof(*testlist) * numhwm);
        replacei = (rand() % numhwm);
        for (j=0; j<n; j++)
        {
            if ( allpods[j]->inhwm == 0 )
            {
                testlist[replacei] = allpods[j];
                numtestpods = numhwm;
                for (k=0; k<2; k++)
                {
                    metric = calc_telepod_metric(testlist,numtestpods,satoshis,height);
                    if ( metric >= 0 && (bestmetric < 0 || metric < bestmetric) )
                    {
                        bestmetric = metric;
                        memcpy(hwmpods,testlist,numtestpods * sizeof(*hwmpods));
                        *hwmnump = numhwm = numtestpods;
                        printf("new HWM i.%d j.%d k.%d replacei.%d bestmetric %f, numtestpods.%d\n",i,j,k,replacei,bestmetric,numtestpods);
                        if ( set_inhwm_flags(hwmpods,numhwm,allpods,n) == 0 )
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

struct telepod **evolve_telepod_bundle(int32_t *nump,int32_t maxiters,struct coin_info *cp,int32_t minage,uint64_t satoshis,uint32_t height)
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
        if ( i == n )
        {
            free(allpods);
            return(0);
        }
        else n++;
    }
    bestn = n;
    hwmpods = calloc(n+1,sizeof(*hwmpods));
    memcpy(hwmpods,allpods,n * sizeof(*hwmpods));
    if ( maxiters > 0 )
        hwmpods = evolve_podlist(&bestn,hwmpods,allpods,n,maxiters,satoshis,height);
    free(allpods);
    *nump = bestn;
    return(hwmpods);
}

int32_t sendandfree_jsonmessage(char *sender,char *NXTACCTSECRET,cJSON *json,char *destNXTaddr)
{
    int32_t err = -1;
    cJSON *retjson;
    char *msg,*retstr,errstr[512];
    msg = cJSON_Print(json);
    stripwhite_ns(msg,strlen(msg));
    retstr = sendmessage(sender,NXTACCTSECRET,msg,(int32_t)strlen(msg)+1,destNXTaddr,msg);
    if ( retstr != 0 )
    {
        retjson = cJSON_Parse(retstr);
        if ( retjson != 0 )
        {
            if ( extract_cJSON_str(errstr,sizeof(errstr),retjson,"error") > 0 )
            {
                printf("error.(%s) sending (%s)\n",errstr,msg);
                err = -2;
            } else err = 0;
            free_json(retjson);
        }
        free(retstr);
    }
    free_json(json);
    free(msg);
    return(err);
}

int32_t transporter(char *NXTaddr,char *NXTACCTSECRET,char *destNXTaddr,struct telepod *pod,uint32_t totalcrc,int32_t ind)
{
    cJSON *json;
    char numstr[32];
    json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"requestType",cJSON_CreateString("telepod"));
    cJSON_AddItemToObject(json,"crc",cJSON_CreateNumber(totalcrc));
    cJSON_AddItemToObject(json,"i",cJSON_CreateNumber(ind));
    cJSON_AddItemToObject(json,"h",cJSON_CreateNumber(pod->height));
    cJSON_AddItemToObject(json,"c",cJSON_CreateString(pod->coinstr));
    sprintf(numstr,"%.8f",(double)pod->satoshis/SATOSHIDEN);
    cJSON_AddItemToObject(json,"v",cJSON_CreateString(numstr));
    cJSON_AddItemToObject(json,"a",cJSON_CreateString(pod->coinaddr));
    cJSON_AddItemToObject(json,"t",cJSON_CreateString(pod->txid));
    cJSON_AddItemToObject(json,"o",cJSON_CreateNumber(pod->vout));
    cJSON_AddItemToObject(json,"p",cJSON_CreateString(pod->pubkey));
    cJSON_AddItemToObject(json,"k",cJSON_CreateString(pod->privkey));
    return(sendandfree_jsonmessage(NXTaddr,NXTACCTSECRET,json,destNXTaddr));
}

cJSON *create_telepod_bundle_json(uint32_t *totalcrcp,char *coinstr,uint32_t height,int32_t minage,uint64_t satoshis,struct telepod **pods,int32_t n)
{
    int32_t i;
    uint32_t totalcrc = 0;
    char numstr[64];
    cJSON *json,*array;
    *totalcrcp = 0;
    if ( pods == 0 || n == 0 )
        return(0);
    json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"requestType",cJSON_CreateString("transporter"));
    cJSON_AddItemToObject(json,"coin",cJSON_CreateString(coinstr));
    cJSON_AddItemToObject(json,"height",cJSON_CreateNumber(height));
    cJSON_AddItemToObject(json,"minage",cJSON_CreateNumber(minage));
    sprintf(numstr,"%.8f",dstr(satoshis));
    cJSON_AddItemToObject(json,"value",cJSON_CreateString(numstr));
    array = cJSON_CreateArray();
    for (i=0; i<n; i++)
    {
        sprintf(numstr,"%x",pods[i]->crc);
        cJSON_AddItemToArray(array,cJSON_CreateString(numstr));
        totalcrc = _crc32(totalcrc,&pods[i]->crc,sizeof(pods[i]->crc));
    }
    cJSON_AddItemToObject(json,"telepods",array);
    *totalcrcp = totalcrc;
    cJSON_AddItemToObject(json,"totalcrc",cJSON_CreateNumber(totalcrc));
    return(json);
}

char *got_transporter_status(char *NXTACCTSECRET,char *sender,char *coinstr,uint32_t totalcrc,uint64_t value,int32_t num,uint32_t *crcs,int32_t n)
{
    char retbuf[1024],verifiedNXTaddr[64];
    struct coin_info *cp;
    struct NXT_acct *destnp,*np;
    int32_t i,mismatches,completed,createdflag;
    np = find_NXTacct(verifiedNXTaddr,NXTACCTSECRET);
    destnp = get_NXTacct(&createdflag,Global_mp,sender);
    cp = get_coin_info(coinstr);
    if ( totalcrc == 0 )
        totalcrc = (uint32_t)-1;
    if ( crcs == 0 || n == 0 )
    {
        destnp->gotstatus = totalcrc;
        if ( destnp->statuscrcs != 0 )
            free(destnp->statuscrcs);
        destnp->statuscrcs = calloc(num,sizeof(*destnp->statuscrcs));
        destnp->expect = value;
        destnp->numincoming = num;
        sprintf(retbuf,"{\"result\":\"ready to start teleporting\",\"num\":%d,\"totalcrc\":%u,\"value\":%.8f}",num,totalcrc,dstr(value));
    }
    else
    {
        if ( cp == 0 )
            fatal("got transporter status: illegal coin");
        mismatches = completed = 0;
        for (i=0; i<n; i++)
        {
            if ( crcs[i] != 0 )
            {
                destnp->statuscrcs[i] = crcs[i];
                if ( crcs[i] == ((struct telepod *)destnp->intransit[i])->crc )
                    completed++;
                else mismatches++;
            }
        }
        sprintf(retbuf,"{\"result\":\"got crcs\",\"completed\":%d,\"mismatch\":%d,\"num\":%d,\"totalcrc\":%u}",completed,mismatches,num,totalcrc);
    }
    return(clonestr(retbuf));
}

void send_transporter_status(char *verifiedNXTaddr,char *NXTACCTSECRET,struct NXT_acct *destnp,struct coin_info *cp,uint32_t crc,uint64_t value,int32_t errflag,int32_t num,uint32_t *crcs)
{
    int32_t i;
    long len;
    char msg[1024],*retstr;
    sprintf(msg,"{\"requestType\":\"transporter_status\",\"status\":%d,\"coin\":\"%s\",\"crc\":%u,\"value\",%.8f,\"num\":%d",errflag,cp!=0?cp->name:"ERROR",crc,dstr(value),num);
    len = strlen(msg);
    if ( crcs != 0 )
    {
        strcpy(msg+len,"\"crcs\":[");
        for (i=0; i<num; i++)
        {
            sprintf(msg+len,"%u ",crcs[i]);
            len += strlen(msg+len);
        }
        strcpy(msg+len,"]}");
    }
    else strcpy(msg+len,"}");
    retstr = sendmessage(verifiedNXTaddr,NXTACCTSECRET,msg,(int32_t)strlen(msg)+1,destnp->H.NXTaddr,msg);
    if ( retstr != 0 )
    {
        printf("send_transporter_ACK.(%s)\n",retstr);
        free(retstr);
    }
}

char *teleport(char *NXTaddr,char *NXTACCTSECRET,double amount,char *dest,struct coin_info *cp,char *walletpass,int32_t minage)
{
    static unsigned char zerokey[crypto_box_PUBLICKEYBYTES];
    char buf[4096],*retstr;
    struct telepod **pods = 0;
    uint64_t satoshis;
    struct NXT_acct *np,*destnp;
    cJSON *json;
    double startmilli;
    int32_t i,n,err,pending;
    uint32_t totalcrc,height;
    destnp = search_addresses(dest);
    if ( destnp->intransit != 0 )
    {
        sprintf(buf,"{\"error\":\"teleport: have to wait for current transport to complete\"}");
        return(clonestr(buf));
    }
    load_telepods(cp,100,NXTACCTSECRET);
    height = (uint32_t)get_blockheight(cp);
    satoshis = (amount * SATOSHIDEN);
    if ( (pods= evolve_telepod_bundle(&n,0,cp,minage,satoshis,height)) == 0 )
        sprintf(buf,"{\"error\":\"teleport: not enough available for %.8f %s to %s\"}",amount,cp->name,dest);
    else
    {
        free(pods), pods = 0;
        //prep_wallet(cp,walletpass,60);
        np = find_NXTacct(NXTaddr,NXTACCTSECRET);
        if ( memcmp(destnp->pubkey,zerokey,sizeof(zerokey)) == 0 )
        {
            query_pubkey(destnp->H.NXTaddr);
            sprintf(buf,"{\"error\":\"no pubkey for %s, request sent\"}",dest);
        }
        else
        {
            printf("start evolving at %f\n",milliseconds());
            pods = evolve_telepod_bundle(&n,100000,cp,minage,satoshis,height);
            printf("finished evolving at %f\n",milliseconds());
            if ( pods == 0 )
                sprintf(buf,"{\"error\":\"unexpected bundle evolve failure for %.8f %s to %s\"}",amount,cp->name,dest);
            else
            {
                json = create_telepod_bundle_json(&totalcrc,cp->name,height,minage,satoshis,pods,n);
                if ( json != 0 )
                {
                    destnp->gotstatus = 0;
                    destnp->intransit = (void **)pods;
                    err = sendandfree_jsonmessage(NXTaddr,NXTACCTSECRET,json,destnp->H.NXTaddr);
                    if ( err < 0 )
                        sprintf(buf,"{\"error\":\"sendandfree_jsonmessage telepod bundle failure for %.8f %s to %s\"}",amount,cp->name,dest);
                    else
                    {
                        for (i=0; i<TELEPORT_TRANSPORTER_TIMEOUT; i++)
                        {
                            if ( destnp->gotstatus != 0 )
                                break;
                            sleep(1);
                        }
                        if ( i == 0 || destnp->gotstatus != totalcrc || destnp->expect != satoshis || destnp->numincoming != n )
                        {
                            sprintf(buf,"{\"error\":\"no response or badcrc %08x vs %08x from %s for: %.8f %s to %s or expect %.8f vs %.8f or num %d vs %d\"}",destnp->gotstatus,totalcrc,destnp->H.NXTaddr,amount,cp->name,dest,dstr(destnp->expect),dstr(satoshis),destnp->numincoming,n);
                        }
                        else
                        {
                            startmilli = milliseconds();
                            pending = 0;
                            while ( milliseconds() < (startmilli + TELEPORT_TELEPOD_TIMEOUT*1000) )
                            {
                                for (i=pending=0; i<n; i++)
                                {
                                    if ( destnp->statuscrcs != 0 && destnp->statuscrcs[i] == pods[i]->crc )
                                        continue;
                                    pending++;
                                    if ( transporter(NXTaddr,NXTACCTSECRET,destnp->H.NXTaddr,pods[i],totalcrc,i) != 0 )
                                    {
                                        sprintf(buf,"{\"error\":\"transporter error on %d of %d: %.8f %s to %s\"}",i,n,amount,cp->name,dest);
                                        printf("%s\n",buf);
                                        break;
                                    }
                                }
                            }
                            if ( pending == 0 )
                            {
                                sprintf(buf,"{\"result\":\"transporter sent all %d telepods: %.8f %s to %s\"}",n,amount,cp->name,dest);
                            }
                            else
                            {
                                struct telepod *clone_telepod(struct coin_info *cp,struct telepod *pod);
                                printf("ABORTING Teleport, cloning all pending telepods\n");
                                for (i=0; i<n; i++)
                                    pods[i]->localclone = clone_telepod(cp,pods[i]);
                            }
                            queue_enqueue(&podQ,pods);
                        }
                    }
                }
                else sprintf(buf,"{\"error\":\"unexpected bundle evolve createjson error for %.8f %s to %s\"}",amount,cp->name,dest);
            }
        }
        return(retstr);
    }
    return(clonestr(buf));
}

// Receive

void conv_telepod(struct coin_value *vp,struct telepod *pod)
{
    char script[512];
    memset(vp,0,sizeof(*vp));
    vp->value = pod->satoshis;
    vp->txid = pod->txid;
    vp->parent_vout = pod->vout;
    safecopy(vp->coinaddr,pod->coinaddr,sizeof(vp->coinaddr));
    calc_script(script,pod->pubkey);
    vp->script = clonestr(script);
}

void free_coin_value(struct coin_value *vp)
{
    if ( vp != 0 )
    {
        if ( vp->script != 0 )
            free(vp->script);
        free(vp);
    }
}

uint64_t calc_transporter_fee(struct coin_info *cp,uint64_t satoshis)
{
    if ( strcmp(cp->name,"BTCD") == 0 )
        return(cp->txfee);
    else return(cp->txfee + (satoshis>>10));
}

int32_t is_relevant_coinvalue(struct coin_info *cp,struct coin_value *vp) // jl777: this confusing vin vs vout?!
{
    char pubkey[512];
    struct telepod *pod;
    if ( validate_coinaddr(pubkey,cp,vp->coinaddr) > 0 )
    {
        if ( (pod= find_telepod(cp,pubkey)) != 0 && pod->vout == vp->parent_vout )
            return(1);
    } printf("warning: couldnt find pubkey for (%s)\n",vp->coinaddr);
    return(0);
}

int32_t get_privkeys(char privkeys[MAX_COIN_INPUTS][MAX_PRIVKEY_SIZE],struct coin_info *cp,char **localcoinaddrs,int32_t num)
{
    char args[1024],pubkey[512],*privkey;
    int32_t i,n = 0;
    struct telepod *pod;
    for (i=0; i<num; i++)
    {
        if ( cp != 0 && localcoinaddrs[i] != 0 )
        {
            sprintf(args,"\"%s\"",localcoinaddrs[i]);
            privkey = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"dumpprivkey",args);
            if ( privkey != 0 )
            {
                n++;
                strcpy(privkeys[i],privkey);
                free(privkey);
            }
            else
            {
                if ( validate_coinaddr(pubkey,cp,localcoinaddrs[i]) > 0 )
                {
                    if ( (pod= find_telepod(cp,pubkey)) != 0 )
                        strcpy(privkeys[i],pod->privkey);
                    else
                    {
                        printf("cant dumpprivkey for (%s) pubkey.(%s) nor is it a telepod we have\n",localcoinaddrs[i],pubkey);
                        return(-1);
                    }
                }
                else
                {
                    printf("cant get pubkey for (%s) \n",localcoinaddrs[i]);
                    return(-2);
                }
            }
        }
    }
    return(n);
}

struct telepod *clone_telepod(struct coin_info *cp,struct telepod *pod)
{
    /*char *podaddr,*txid,*privkey,*changeaddr,args[1024],script[1024];
    struct coin_value **ups;
    int32_t i,n;
    uint64_t balance,satoshis,fee;
    struct rawtransaction RAW;
    podaddr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"getnewaddress","telepods");
    if ( podaddr != 0 )
    {
        sprintf(args,"\"%s\"",podaddr);
        privkey = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"dumpprivkey",args);
        if ( privkey != 0 )
        {
            if ( pods == 0 )
            {
                sprintf(args,"[{\"transporter\"},{\"%s\"},{%.8f}]",podaddr,amount);
                if ( (txid= bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"sendfrom",args)) == 0 )
                    printf("error funding telepod from transporter\n");
                else pod = create_telepod(0,cp,satoshis,podaddr,script,privkey,txid,TELEPOD_CONTENTS_VOUT);
            }
            else
            {
                fee = calc_transporter_fee(cp,satoshis);
                ups = 0;//select_telepods(&n,&balance,pods,num,satoshis+fee);
                if ( ups != 0 )
                {
                    memset(&RAW,0,sizeof(RAW));
                    if ( balance > satoshis+fee )
                        changeaddr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"getnewaddress","telepods");
                    else changeaddr = 0;
                    if ( (txid= calc_rawtransaction(cp,&RAW,podaddr,fee,changeaddr,ups,n,satoshis+fee,balance)) == 0 )
                        printf("error funding telepod from telepods %p.num %d\n",pods,num);
                    else pod = create_telepod(0,cp,satoshis,podaddr,script,privkey,txid,TELEPOD_CONTENTS_VOUT);
                    purge_rawtransaction(&RAW);
                    for (i=0; i<n; i++)
                        free_coin_value(ups[i]);
                    free(ups);
                    if ( changeaddr != 0 )
                    {
                        // need to set script and privkey
                        create_telepod(1,cp,balance - (satoshis+fee),changeaddr,script,privkey,txid,TELEPOD_CHANGE_VOUT);
                    }
                }
            }
            free(privkey);
        }
        free(podaddr);
    }
     return(pod);*/ return(0);
}

char *telepod_received(char *sender,char *NXTACCTSECRET,char *coinstr,uint32_t crc,int32_t ind,uint32_t height,uint64_t satoshis,char *coinaddr,char *txid,int32_t vout,char *pubkey,char *privkey)
{
    char retbuf[4096],verifiedNXTaddr[64];
    struct NXT_acct *sendernp,*np;
    struct coin_info *cp;
    int32_t createdflag,errflag = -1;
    np = find_NXTacct(verifiedNXTaddr,NXTACCTSECRET);
    sendernp = get_NXTacct(&createdflag,Global_mp,sender);
    sprintf(retbuf,"telepod_received from NXT.%s crc.%08x ind.%d height.%d %.8f %s %s/%d (%s) (%s)",sender,crc,ind,height,dstr(satoshis),coinaddr,txid,vout,pubkey,privkey);
    cp = get_coin_info(coinstr);
    if ( cp == 0 || crc == 0 || ind < 0 || height == 0 || satoshis == 0 || coinaddr[0] == 0 || txid[0] == 0 || vout < 0 || pubkey[0] == 0 || privkey[0] == 0 )
        strcat(retbuf," <<<<< ERROR");
    else
    {
        errflag = 0;
        sendernp->incomingteleport[ind] = crc;
        // jl777: need to verify its a valid telepod
    }
    send_transporter_status(verifiedNXTaddr,NXTACCTSECRET,sendernp,cp,sendernp->totalcrc,satoshis,errflag,sendernp->numincoming,sendernp->incomingteleport);
    return(clonestr(retbuf));
}

char *transporter_received(char *sender,char *NXTACCTSECRET,char *coinstr,uint32_t totalcrc,uint32_t height,uint64_t value,int32_t minage,uint32_t *crcs,int32_t n)
{
    char retbuf[4096],verifiedNXTaddr[64];
    struct coin_info *cp;
    int32_t createdflag,errflag = -1;
    struct NXT_acct *sendernp,*np;
    np = find_NXTacct(verifiedNXTaddr,NXTACCTSECRET);
    sprintf(retbuf,"transporter_received from NXT.%s totalcrc.%08x n.%d height.%d %.8f",sender,totalcrc,n,height,dstr(value));
    cp = get_coin_info(coinstr);
    sendernp = get_NXTacct(&createdflag,Global_mp,sender);
    if ( cp == 0 || totalcrc == 0 || minage <= 0 || height == 0 || value == 0 || n <= 0)
        strcat(retbuf," <<<<< ERROR");
    else if ( sendernp->incomingteleport == 0 )
    {
        if ( _crc32(0,crcs,n * sizeof(*crcs)) == totalcrc )
        {
            sendernp->totalcrc = totalcrc;
            sendernp->numincoming = n;
            sendernp->incomingteleport = calloc(n,sizeof(*crcs));
            memcpy(sendernp->incomingteleport,crcs,n*sizeof(*crcs));
            errflag = 0;
        } else sprintf(retbuf + strlen(retbuf)," totalcrc %08x vs %08x ERROR",totalcrc,_crc32(0,crcs,n * sizeof(*crcs)));
    }
    else strcat(retbuf," <<<<< ERROR already have incomingteleport");
    send_transporter_status(verifiedNXTaddr,NXTACCTSECRET,sendernp,cp,totalcrc,value,errflag,n,0);
    return(clonestr(retbuf));
}
#endif
