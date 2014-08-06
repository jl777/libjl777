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

#define TELEPOD_CONTENTS_VOUT 0 // must be 0
#define TELEPOD_CHANGE_VOUT 1   // vout 0 is for the pod contents and last one (1 if no change or 2) is marker

struct telepod
{
    uint32_t filenum,xfered,height,sentheight,crc;
    uint16_t cloneout,pad;
    int8_t inhwm,ischange,saved,dir;
    char clonetxid[MAX_COINTXID_LEN],cloneaddr[MAX_COINADDR_LEN];
    struct telepod *abortion;
    uint64_t modified,satoshis;
    uint16_t podsize,vout;
    char coinstr[8],txid[MAX_COINTXID_LEN],coinaddr[MAX_COINADDR_LEN],pubkey[128];
    char privkey[];
};

void beg_for_changepod(struct coin_info *cp,char *NXTACCTSECRET)
{
    // jl777: need to send request to faucet
    printf("beg for changepod\n");
}

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
    return(_crc32(0,(void *)((long)&pod->modified + sizeof(pod->modified)),pod->podsize - ((long)&pod->modified - (long)pod)));
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

int32_t find_telepod_slot(char *name,int32_t filenum)
{
    FILE *fp = 0;
    char fname[512];
    do
    {
        if ( fp != 0 )
        {
            fseek(fp,0,SEEK_END);
            if ( ftell(fp) == 0 )
            {
                fclose(fp);
                printf("found empty slot.%d for telepod\n",filenum);
                break;
            }
            fclose(fp);
            fp = 0;
            filenum++;
        }
        set_telepod_fname(fname,name,filenum);
        printf("check (%s)\n",fname);
    }
    while ( (fp= fopen(fname,"rb")) != 0 );
    return(filenum);
}

int32_t ensure_telepod_has_backup(struct coin_info *cp,struct telepod *refpod,char *NXTACCTSECRET)
{
    struct telepod *pod,*newpod;
    int32_t createdflag;
    uint64_t hashval;
    if ( refpod == 0 )
        return(-1);
    if ( refpod->saved == 0 )
        cp->savedtelepods = find_telepod_slot(cp->name,cp->savedtelepods);
    if ( update_telepod_file(cp->name,cp->savedtelepods,refpod,NXTACCTSECRET) == 0 )
    {
        if ( (newpod= _load_telepod(cp->name,cp->savedtelepods,NXTACCTSECRET)) == 0 )
            printf("error verifying telepod.%d\n",cp->savedtelepods);
        else
        {
            // WARNING: advanced hashtable replacement, be careful to mess with this
            pod = MTadd_hashtable(&createdflag,&cp->telepods,refpod->pubkey);
            hashval = MTsearch_hashtable(&cp->telepods,refpod->pubkey);
            if ( hashval != HASHSEARCH_ERROR )
            {
                if ( pod == cp->telepods->hashtable[hashval] )
                {
                    cp->savedtelepods++;
                    cp->telepods->hashtable[hashval] = newpod;
                    free(pod);
                    return(0);
                }
                else
                {
                    printf("unexpected newpod %p != %p in [%llu]\n",newpod,cp->telepods->hashtable[hashval],(long long)hashval);
                }
            }
            free(newpod);
        }
    }
    return(-1);
}

struct telepod *create_telepod(int32_t saveflag,char *NXTACCTSECRET,struct coin_info *cp,uint64_t satoshis,char *podaddr,char *pubkey,char *privkey,char *txid,int32_t vout)
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
    if ( saveflag != 0 )
        ensure_telepod_has_backup(cp,pod,NXTACCTSECRET);
    
    disp_telepod("create",pod);
    return(pod);
}

void update_minipod_info(struct coin_info *cp,struct telepod *pod)
{
    struct telepod *refpod;
    if ( pod->satoshis <= cp->min_telepod_satoshis && pod->satoshis >= cp->txfee && pod->clonetxid[0] == 0 && pod->height > (uint32_t)(get_blockheight(cp) - cp->minconfirms) )
    {
        if ( (refpod= cp->changepod) == 0 || (pod->height != 0 && (refpod->height == 0 || pod->height < refpod->height)) )
        {
            cp->changepod = pod;
            if ( refpod != 0 )
                refpod->ischange = 0;
            pod->ischange = 1;
        }
    }
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
            if ( (height - pod->height) > minage && pod->ischange == 0 )
            {
                (*balancep) += pod->satoshis;
                pods[numactive++] = pod;
            }
            else printf("reject telepod.%d height.%d - pod->height.%d minage.%d, ischange.%d\n",i,height,pod->height,minage,pod->ischange);
        }
    }
    free(rawlist);
    //printf("numactive.%d\n",numactive);
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

struct telepod *select_changepod(struct coin_info *cp,int32_t minage)
{
    int32_t i,n;
    uint64_t balance;
    struct telepod **allpods;
    cp->changepod = 0;
    allpods = gather_telepods(&n,&balance,cp,minage);
    for (i=0; i<n; i++)
        allpods[i]->ischange = 0;
    for (i=0; i<n; i++)
        update_minipod_info(cp,allpods[i]);
    return(cp->changepod);
}

int32_t load_telepods(struct coin_info *cp,int32_t maxnofile,char *NXTACCTSECRET)
{
    int nofile = 0;
    struct telepod *pod;
    if ( cp->telepods != 0 )
        return(cp->savedtelepods);
    cp->telepods = hashtable_create("telepods",HASHTABLES_STARTSIZE,sizeof(*pod),((long)&pod->pubkey[0] - (long)pod),sizeof(pod->pubkey),((long)&pod->modified - (long)pod));
    if ( cp->min_telepod_satoshis == 0 )
        cp->min_telepod_satoshis = SATOSHIDEN/10;
    while ( nofile < maxnofile )
    {
        if ( (pod= _load_telepod(cp->name,cp->savedtelepods,NXTACCTSECRET)) != 0 )
        {
            if ( pod->clonetxid[0] == 0 )
            {
                update_minipod_info(cp,pod);
                ensure_telepod_has_backup(cp,pod,NXTACCTSECRET);
                disp_telepod("load",pod);
            }
            else
            {
                disp_telepod("spent",pod);
                cp->savedtelepods++;
            }
        } else nofile++;
    }
    if ( cp->changepod == 0 )
        beg_for_changepod(cp,NXTACCTSECRET);
    printf("after loaded %d telepods\n",cp->savedtelepods);
    return(cp->savedtelepods);
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
            printf("i.%d j.%d k.%d finished.%d\n",i,j,k,finished);
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
                if ( crcs[i] == ((struct telepod *)destnp->outbound[i])->crc )
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

char *teleport(char *NXTaddr,char *NXTACCTSECRET,uint64_t satoshis,char *dest,struct coin_info *cp,int32_t minage)
{
    static unsigned char zerokey[crypto_box_PUBLICKEYBYTES];
    char buf[4096];
    struct telepod **pods = 0;
    struct NXT_acct *np,*destnp;
    cJSON *json;
    double startmilli;
    int32_t i,n,err,pending;
    uint32_t totalcrc,height,startheight;
    printf("%s -> teleport %.8f %s -> %s minage.%d\n",NXTaddr,dstr(satoshis),cp->name,dest,minage);
    if ( minage == 0 )
        minage = cp->minconfirms;
    destnp = search_addresses(dest);
    if ( destnp->outbound != 0 )
    {
        sprintf(buf,"{\"error\":\"teleport: have to wait for current transport to complete\"}");
        return(clonestr(buf));
    }
    //prep_wallet(cp,walletpass,60);
    if ( load_telepods(cp,100,NXTACCTSECRET) == 0 )
    {
        int32_t make_traceable_telepods(struct coin_info *cp,char *NXTACCTSECRET,uint64_t satoshis);
        make_traceable_telepods(cp,NXTACCTSECRET,SATOSHIDEN/5);
    }

    height = (uint32_t)get_blockheight(cp);
    if ( (pods= evolve_telepod_bundle(&n,0,cp,minage,satoshis,height)) == 0 )
        sprintf(buf,"{\"error\":\"teleport: not enough available for %.8f %s to %s\"}",dstr(satoshis),cp->name,dest);
    else
    {
        free(pods), pods = 0;
        np = find_NXTacct(NXTaddr,NXTACCTSECRET);
        if ( memcmp(destnp->pubkey,zerokey,sizeof(zerokey)) == 0 )
        {
            query_pubkey(destnp->H.NXTaddr,NXTACCTSECRET);
            sprintf(buf,"{\"error\":\"no pubkey for %s, request sent\"}",dest);
        }
        else
        {
            printf("start evolving at %f\n",milliseconds());
            pods = evolve_telepod_bundle(&n,100000,cp,minage,satoshis,height);
            printf("finished evolving at %f\n",milliseconds());
            if ( pods == 0 )
                sprintf(buf,"{\"error\":\"unexpected bundle evolve failure for %.8f %s to %s\"}",dstr(satoshis),cp->name,dest);
            else
            {
                json = create_telepod_bundle_json(&totalcrc,cp->name,height,minage,satoshis,pods,n);
                if ( json != 0 )
                {
                    destnp->gotstatus = 0;
                    destnp->outbound = (void **)pods;
                    err = sendandfree_jsonmessage(NXTaddr,NXTACCTSECRET,json,destnp->H.NXTaddr);
                    if ( err < 0 )
                        sprintf(buf,"{\"error\":\"sendandfree_jsonmessage telepod bundle failure for %.8f %s to %s\"}",dstr(satoshis),cp->name,dest);
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
                            sprintf(buf,"{\"error\":\"no response or badcrc %08x vs %08x from %s for: %.8f %s to %s or expect %.8f vs %.8f or num %d vs %d\"}",destnp->gotstatus,totalcrc,destnp->H.NXTaddr,dstr(satoshis),cp->name,dest,dstr(destnp->expect),dstr(satoshis),destnp->numincoming,n);
                        }
                        else
                        {
                            startheight = (uint32_t)get_blockheight(cp);
                            startmilli = milliseconds();
                            pending = 0;
                            while ( milliseconds() < (startmilli + TELEPORT_TELEPOD_TIMEOUT*1000) )
                            {
                                for (i=pending=0; i<n; i++)
                                {
                                    if ( destnp->statuscrcs != 0 && destnp->statuscrcs[i] == pods[i]->crc )
                                    {
                                        //if ( pods[i]->satoshis == cp->min_telepod_satoshis )
                                        //    cp->numunitpods--;
                                        continue;
                                    }
                                    pending++;
                                    if ( transporter(NXTaddr,NXTACCTSECRET,destnp->H.NXTaddr,pods[i],totalcrc,i) != 0 )
                                    {
                                        sprintf(buf,"{\"error\":\"transporter error on %d of %d: %.8f %s to %s\"}",i,n,dstr(satoshis),cp->name,dest);
                                        printf("%s\n",buf);
                                        break;
                                    }
                                    else
                                    {
                                        pods[i]->sentheight = startheight;
                                        pods[i]->dir = 1;
                                        if ( cp->enabled == 0 )
                                        {
                                            strcpy(cp->NXTACCTSECRET,NXTACCTSECRET);
                                            cp->blockheight = startheight, cp->enabled = 1;
                                        }
                                    }
                                }
                                if ( i == n && pending == 0 )
                                    break;
                                sleep(3);
                            }
                            if ( pending == 0 )
                            {
                                sprintf(buf,"{\"result\":\"transporter sent all %d telepods: %.8f %s to %s\"}",n,dstr(satoshis),cp->name,dest);
                            }
                            else
                            {
                                struct telepod *clone_telepod(struct coin_info *cp,char *NXTACCTSECRET,struct telepod *pod);
                                printf("ABORTING Teleport, cloning all pending telepods\n");
                                for (i=0; i<n; i++)
                                    pods[i]->abortion = clone_telepod(cp,NXTACCTSECRET,pods[i]);
                            }
                            for (i=0; i<n; i++)
                                queue_enqueue(&cp->podQ.pingpong[0],pods[i]); // jl777: monitor status on blockchain
                        }
                    }
                }
                else sprintf(buf,"{\"error\":\"unexpected bundle evolve createjson error for %.8f %s to %s\"}",dstr(satoshis),cp->name,dest);
            }
        }
    }
    return(clonestr(buf));
}

// other thread
int32_t process_podQ(void *ptr)
{
    struct telepod *pod = ptr;
    if ( pod->dir > 0 && pod->clonetxid[0] != 0 ) // outbound telepod
        return(1);
    else if ( pod->dir < 0 && pod->height != 0 ) // cloned telepod
        return(1);
    return(0);
}

int32_t is_relevant_coinvalue(struct coin_info *cp,struct coin_value *vp,uint32_t blocknum,struct coin_txid *utxo,int32_t utx_out) // jl777: this confusing vin vs vout?!
{
    char pubkey[512];
    struct telepod *pod;
    if ( validate_coinaddr(pubkey,cp,vp->coinaddr) > 0 )
    {
        if ( (pod= find_telepod(cp,pubkey)) != 0 && pod->vout == vp->parent_vout )
        {
            printf("found relevant telepod dir.%d block.%d txid.(%s) vout.%d %p %d\n",pod->dir,blocknum,vp->txid!=0?vp->txid:"notxid",vp->parent_vout,utxo,utx_out);
            if ( utxo != 0 )
            {
                if ( pod->dir <= 0 )
                    printf("WARNING: unexpected dir.%d unset in vin telepod\n",pod->dir);
                safecopy(pod->clonetxid,utxo->txid,sizeof(pod->clonetxid)), pod->cloneout = utx_out;
                if ( update_telepod_file(cp->name,pod->filenum,pod,cp->NXTACCTSECRET) != 0 )
                    printf("ERROR saving cloned refpod\n");
            }
            else
            {
                if ( pod->dir >= 0 || pod->height == 0 )
                    printf("WARNING: unexpected dir.%d set in vout telepod or height.%d set\n",pod->dir,pod->height);
                pod->height = blocknum;
                if ( update_telepod_file(cp->name,pod->filenum,pod,cp->NXTACCTSECRET) != 0 )
                    printf("ERROR saving cloned refpod\n");
            }
            return(1);
        }
    } printf("warning: couldnt find pubkey for (%s)\n",vp->coinaddr);
    return(0);
}

// Receive
uint64_t calc_transporter_fee(struct coin_info *cp,uint64_t satoshis)
{
    if ( strcmp(cp->name,"BTCD") == 0 )
        return(cp->txfee);
    else return(cp->txfee + (satoshis>>10));
}

struct coin_value *conv_telepod(struct telepod *pod)
{
    char script[1024];
    struct coin_value *vp;
    vp = calloc(1,sizeof(*vp));
    vp->value = pod->satoshis;
    vp->txid = pod->txid;
    vp->parent_vout = pod->vout;
    safecopy(vp->coinaddr,pod->coinaddr,sizeof(vp->coinaddr));
    calc_script(script,pod->pubkey);
    vp->script = clonestr(script);
    return(vp);
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

void purge_rawtransaction(struct rawtransaction *raw)
{
    int32_t i;
    if ( raw->rawtxbytes != 0 )
        free(raw->rawtxbytes);
    if ( raw->signedtx != 0 )
        free(raw->signedtx);
    for (i=0; i<raw->numinputs; i++)
        if ( raw->inputs[i] != 0 )
            free_coin_value(raw->inputs[i]);
}

char *get_telepod_privkey(char **podaddrp,char *pubkey,struct coin_info *cp)
{
    char *podaddr,*privkey,args[1024];
    (*podaddrp) = privkey = 0;
    pubkey[0] = 0;
    podaddr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"getnewaddress","[\"telepods\"]");
    if ( podaddr != 0 )
    {
        sprintf(args,"\"%s\"",podaddr);
        privkey = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"dumpprivkey",args);
        printf("got podaddr.(%s) privkey.%p\n",podaddr,privkey);
        if ( privkey != 0 )
        {
            if ( validate_coinaddr(pubkey,cp,podaddr) > 0 )
                (*podaddrp) = podaddr;
            else
            {
                free(podaddr);
                free(privkey);
                privkey = 0;
            }
        } else free(podaddr);
    } else printf("error getnewaddress telepods\n");
    return(privkey);
}

struct telepod *make_traceable_telepod(struct coin_info *cp,char *NXTACCTSECRET,uint64_t satoshis)
{
    char args[1024],pubkey[1024],*podaddr,*txid,*privkey;
    struct telepod *pod = 0;
    printf("make_traceable_telepod %.8f\n",dstr(satoshis));
    if ( (privkey= get_telepod_privkey(&podaddr,pubkey,cp)) != 0 )
    {
        sprintf(args,"[\"transporter\",\"%s\",%.8f]",podaddr,dstr(satoshis));
        printf("args.(%s)\n",args);
        if ( (txid= bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"sendfrom",args)) == 0 )
            printf("error funding %.8f telepod.(%s) from transporter\n",dstr(satoshis),podaddr);
        else pod = create_telepod(1,NXTACCTSECRET,cp,satoshis,podaddr,pubkey,privkey,txid,TELEPOD_CONTENTS_VOUT);
        free(privkey);
        free(podaddr);
    }
    return(pod);
}

int32_t make_traceable_telepods(struct coin_info *cp,char *NXTACCTSECRET,uint64_t satoshis)
{
    int32_t i,n,standard_denominations[] = { 10000, 5000, 1000, 500, 100, 50, 20, 10, 5, 1 };
    uint64_t incr,amount;
    struct telepod *pod;
    n = 0;
    while ( satoshis > 0 )
    {
        amount = satoshis;
        for (i=0; i<(int)(sizeof(standard_denominations)/sizeof(*standard_denominations)); i++)
        {
            incr = (cp->min_telepod_satoshis * standard_denominations[i]);
            if ( satoshis > incr )
            {
                amount = incr;
                break;
            }
        }
        pod = make_traceable_telepod(cp,NXTACCTSECRET,amount);
        if ( pod == 0 )
        {
            printf("error making traceable telepod of %.8f\n",dstr(amount));
            break;
        }
        satoshis -= amount;
        n++;
    }
    return(n);
}

cJSON *create_privkeys_json_params(struct coin_info *cp,char **privkeys,int32_t numinputs)
{
    int32_t i,nonz = 0;
    cJSON *array;
    array = cJSON_CreateArray();
    for (i=0; i<numinputs; i++)
    {
        if ( cp != 0 && privkeys[i] != 0 )
        {
            nonz++;
            //printf("%s ",localcoinaddrs[i]);
            cJSON_AddItemToArray(array,cJSON_CreateString(privkeys[i]));
        }
    }
    if ( nonz == 0 )
        free_json(array), array = 0;
    //else printf("privkeys.%d of %d: %s\n",nonz,numinputs,cJSON_Print(array));
    return(array);
}

char *createsignraw_json_params(struct coin_info *cp,struct rawtransaction *rp,char *rawbytes,char **privkeys)
{
    char *paramstr = 0;
    cJSON *array,*rawobj,*vinsobj,*keysobj;
    rawobj = cJSON_CreateString(rawbytes);
    if ( rawobj != 0 )
    {
        vinsobj = create_vins_json_params(0,cp,rp);
        if ( vinsobj != 0 )
        {
            keysobj = create_privkeys_json_params(cp,privkeys,rp->numinputs);
            if ( keysobj != 0 )
            {
                array = cJSON_CreateArray();
                cJSON_AddItemToArray(array,rawobj);
                cJSON_AddItemToArray(array,vinsobj);
                cJSON_AddItemToArray(array,keysobj);
                paramstr = cJSON_Print(array);
                free_json(array);
            }
            else free_json(vinsobj);
        }
        else free_json(rawobj);
    }
    return(paramstr);
}

char *createrawtxid_json_params(struct coin_info *cp,struct rawtransaction *rp)
{
    char *paramstr = 0;
    cJSON *array,*vinsobj,*voutsobj;
    vinsobj = create_vins_json_params(0,cp,rp);
    if ( vinsobj != 0 )
    {
        voutsobj = create_vouts_json_params(rp);
        if ( voutsobj != 0 )
        {
            array = cJSON_CreateArray();
            cJSON_AddItemToArray(array,vinsobj);
            cJSON_AddItemToArray(array,voutsobj);
            paramstr = cJSON_Print(array);
            free_json(array);   // this frees both vinsobj and voutsobj
        }
        else free_json(vinsobj);
    } else printf("error create_vins_json_params\n");
    printf("createrawtxid_json_params.%s\n",paramstr);
    return(paramstr);
}

uint64_t calc_telepod_inputs(char **coinaddrs,char **privkeys,struct coin_info *cp,struct rawtransaction *rp,struct telepod *srcpod,struct telepod *changepod,uint64_t amount,uint64_t fee,uint64_t change)
{
    rp->inputsum = srcpod->satoshis;
    if ( changepod != 0 )
        rp->inputsum += changepod->satoshis;
    if ( rp->inputsum != (amount + fee + change) )
    {
        printf("calc_telepod_inputs: unspent inputs total %.8f != %.8f\n",dstr(rp->inputsum),dstr(amount + fee + change));
        return(0);
    }
    rp->amount = amount;
    rp->change = change;
    rp->numinputs = 0;
    coinaddrs[rp->numinputs] = srcpod->coinaddr;
    privkeys[rp->numinputs] = srcpod->privkey;
    rp->inputs[rp->numinputs++] = conv_telepod(srcpod);
    if ( changepod != 0 )
    {
        coinaddrs[rp->numinputs] = changepod->coinaddr;
        privkeys[rp->numinputs] = changepod->privkey;
        rp->inputs[rp->numinputs++] = conv_telepod(changepod);
    }
    printf("numinputs %d sum %.8f vs amount %.8f change %.8f -> miners %.8f\n",rp->numinputs,dstr(rp->inputsum),dstr(amount),dstr(rp->change),dstr(rp->inputsum - rp->change - rp->amount));
    return(rp->inputsum);
}

int64_t calc_telepod_outputs(struct coin_info *cp,struct rawtransaction *rp,char *cloneaddr,uint64_t amount,uint64_t change,char *changeaddr)
{
    int32_t n = 0;
    if ( rp->amount == (amount - cp->txfee) && rp->amount <= rp->inputsum )
    {
        rp->destaddrs[TELEPOD_CONTENTS_VOUT] = cloneaddr;
        rp->destamounts[TELEPOD_CONTENTS_VOUT] = rp->amount;
        n++;
        if ( change > 0 )
        {
            if ( changeaddr != 0 )
            {
                rp->destaddrs[TELEPOD_CHANGE_VOUT] = changeaddr;
                rp->destamounts[TELEPOD_CHANGE_VOUT] = rp->change;
                n++;
            }
            else
            {
                printf("no place to send the change of %.8f\n",dstr(amount));
                rp->numoutputs = 0;
                return(0);
            }
        }
        if ( cp->marker != 0 && (rp->inputsum - rp->amount) > cp->txfee )
        {
            rp->destaddrs[n] = cp->marker;
            rp->destamounts[n] = (rp->inputsum - rp->amount) - cp->txfee;
            n++;
        }
    }
    else printf("not enough inputsum %.8f for withdrawal %.8f %.8f\n",dstr(rp->inputsum),dstr(rp->amount),dstr(amount));
    rp->numoutputs = n;
    printf("numoutputs.%d\n",n);
    return(rp->amount);
}

int32_t sign_rawtransaction(char *deststr,unsigned long destsize,struct coin_info *cp,struct rawtransaction *rp,char *rawbytes,char **privkeys)
{
    cJSON *json,*hexobj,*compobj;
    int32_t completed = -1;
    char *retstr,*signparams;
    deststr[0] = 0;
    printf("sign_rawtransaction rawbytes.(%s)\n",rawbytes);
    signparams = createsignraw_json_params(cp,rp,rawbytes,privkeys);
    if ( signparams != 0 )
    {
        stripwhite(signparams,strlen(signparams));
        printf("got signparams.(%s)\n",signparams);
        retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"signrawtransaction",signparams);
        if ( retstr != 0 )
        {
            printf("got retstr.(%s)\n",retstr);
            json = cJSON_Parse(retstr);
            if ( json != 0 )
            {
                hexobj = cJSON_GetObjectItem(json,"hex");
                compobj = cJSON_GetObjectItem(json,"complete");
                if ( compobj != 0 )
                    completed = ((compobj->type&0xff) == cJSON_True);
                copy_cJSON(deststr,hexobj);
                if ( strlen(deststr) > destsize )
                    printf("sign_rawtransaction: strlen(deststr) %ld > %ld destize\n",strlen(deststr),destsize);
                //printf("got signedtransaction.(%s) ret.(%s)\n",deststr,retstr);
                free_json(json);
            } else printf("json parse error.(%s)\n",retstr);
            free(retstr);
        } else printf("error signing rawtx\n");
        //free(signparams);
    } else printf("error generating signparams\n");
    return(completed);
}

char *calc_telepod_transaction(struct coin_info *cp,struct rawtransaction *rp,struct telepod *srcpod,char *destaddr,uint64_t fee,struct telepod *changepod,uint64_t change,char *changeaddr)
{
    long len;
    int64_t retA,retB;
    uint64_t amount = srcpod->satoshis;
    char *rawparams,*localcoinaddrs[3],*privkeys[3],*retstr = 0;
    if ( calc_telepod_inputs(localcoinaddrs,privkeys,cp,rp,srcpod,changepod,amount,fee,change) == (srcpod->satoshis + fee + change) )
    {
        if ( (retB= calc_telepod_outputs(cp,rp,destaddr,amount,change,changeaddr)) == (srcpod->satoshis + fee) )
        {
            rawparams = createrawtxid_json_params(cp,rp);
            if ( rawparams != 0 )
            {
                printf("rawparams.(%s)\n",rawparams);
                stripwhite(rawparams,strlen(rawparams));
                retstr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"createrawtransaction",rawparams);
                if ( retstr != 0 && retstr[0] != 0 )
                {
                    printf("calc_rawtransaction retstr.(%s)\n",retstr);
                    if ( rp->rawtxbytes != 0 )
                        free(rp->rawtxbytes);
                    rp->rawtxbytes = retstr;
                    len = strlen(retstr) + 4096;
                    if ( rp->signedtx != 0 )
                        free(rp->signedtx);
                    rp->signedtx = calloc(1,len);
                    if ( sign_rawtransaction(rp->signedtx,len,cp,rp,rp->rawtxbytes,privkeys) != 0 )
                    {
                        //jl777: broadcast and save record
                    }
                    else
                    {
                        retstr = 0;
                        printf("error signing rawtransaction\n");
                    }
                } else printf("error creating rawtransaction\n");
                free(rawparams);
            } else printf("error creating rawparams\n");
        } else printf("error calculating rawinputs.%.8f or outputs.%.8f\n",dstr(retA),dstr(retB));
    } //else printf("not enough %s balance %.8f for withdraw %.8f -> %s\n",cp->name,dstr(bp->balance),dstr(amount),destaddr);
    return(retstr);
}

struct telepod *clone_telepod(struct coin_info *cp,char *NXTACCTSECRET,struct telepod *refpod)
{
    char *change_podaddr,*change_privkey,*podaddr,*txid,*privkey,pubkey[1024],change_pubkey[1024];
    struct rawtransaction RAW;
    struct telepod *pod,*changepod;
    uint64_t satoshis,fee,change,availchange;
    changepod = cp->changepod;
    change_privkey = change_podaddr = 0;
    availchange = fee = change_pubkey[0] = 0;
    if ( (privkey= get_telepod_privkey(&podaddr,pubkey,cp)) != 0 )
    {
        if ( changepod != 0 )
        {
            availchange = changepod->satoshis;
            change_privkey = get_telepod_privkey(&change_podaddr,change_pubkey,cp);
            fee = calc_transporter_fee(cp,satoshis);
        }
        if ( fee <= availchange )
        {
            change = (availchange - fee);
            memset(&RAW,0,sizeof(RAW));
            if ( (txid= calc_telepod_transaction(cp,&RAW,refpod,podaddr,fee,changepod,change,change_podaddr)) == 0 )
                printf("error cloning %.8f telepod.(%s) to %s\n",dstr(satoshis),refpod->coinaddr,podaddr);
            else
            {
                pod = create_telepod(1,NXTACCTSECRET,cp,satoshis,podaddr,pubkey,privkey,txid,TELEPOD_CONTENTS_VOUT);
                safecopy(refpod->clonetxid,txid,sizeof(refpod->clonetxid)), refpod->cloneout = TELEPOD_CONTENTS_VOUT;
                pod->dir = -1;
                pod->height = 0;
                if ( update_telepod_file(cp->name,refpod->filenum,refpod,NXTACCTSECRET) != 0 )
                    printf("ERROR saving cloned refpod\n");
                queue_enqueue(&cp->podQ.pingpong[0],pod);
                if ( change != 0 )
                {
                    changepod = create_telepod(1,NXTACCTSECRET,cp,change,change_podaddr,change_pubkey,change_privkey,txid,TELEPOD_CHANGE_VOUT);
                    safecopy(changepod->clonetxid,txid,sizeof(changepod->clonetxid)), changepod->cloneout = TELEPOD_CHANGE_VOUT;
                    changepod->dir = -1;
                    changepod->height = 0;
                    if ( update_telepod_file(cp->name,changepod->filenum,changepod,NXTACCTSECRET) != 0 )
                        printf("ERROR saving changepod after used\n");
                    queue_enqueue(&cp->podQ.pingpong[0],changepod);
                    if ( (cp->changepod= select_changepod(cp,cp->minconfirms)) == 0 )
                        cp->changepod = changepod;
                }
            }
            if ( cp->enabled == 0 )
            {
                strcpy(cp->NXTACCTSECRET,NXTACCTSECRET);
                cp->blockheight = (uint32_t)get_blockheight(cp), cp->enabled = 1;
            }
            purge_rawtransaction(&RAW);
            if ( change_privkey != 0 )
                free(change_privkey);
            if ( change_podaddr != 0 )
                free(change_podaddr);
        }
        free(privkey);
        free(podaddr);
    }
    return(pod);
}

void update_transporter_summary(cJSON *array,struct coin_info *cp,int32_t i,int32_t n,struct NXT_acct *sendernp,struct telepod *clone,struct telepod *pod)
{
    cJSON *item;
    char numstr[32];
    item = cJSON_CreateObject();
    sprintf(numstr,"%.8f",dstr(clone->satoshis));
    cJSON_AddItemToObject(item,"value",cJSON_CreateString(numstr));
    cJSON_AddItemToObject(item,"clonecrc",cJSON_CreateNumber(clone->crc));
    cJSON_AddItemToObject(item,"origcrc",cJSON_CreateNumber(pod->crc));
    cJSON_AddItemToArray(array,item);
}

char *calc_transporter_summary(struct coin_info *cp,struct NXT_acct *sendernp,struct telepod **clones,struct telepod **incoming,int32_t num)
{
    int32_t i;
    char *retstr,numstr[32];
    cJSON *json,*array;
    json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"coin",cJSON_CreateString(cp->name));
    sprintf(numstr,"%.8f",dstr(sendernp->expect));
    cJSON_AddItemToObject(json,"value",cJSON_CreateString(numstr));
    array = cJSON_CreateArray();
    for (i=0; i<num; i++)
        update_transporter_summary(array,cp,i,num,sendernp,clones[i],incoming[i]);
    cJSON_AddItemToObject(json,"telepods",array);
    retstr = cJSON_Print(json);
    stripwhite_ns(retstr,strlen(retstr));
    return(retstr);
}

char *telepod_received(char *sender,char *NXTACCTSECRET,char *coinstr,uint32_t crc,int32_t ind,uint32_t height,uint64_t satoshis,char *coinaddr,char *txid,int32_t vout,char *pubkey,char *privkey)
{
    char retbuf[4096],verifiedNXTaddr[64],*retstr = 0;
    struct NXT_acct *sendernp,*np;
    struct coin_info *cp;
    struct telepod *pod;
    int32_t i,createdflag,errflag = -1;
    np = find_NXTacct(verifiedNXTaddr,NXTACCTSECRET);
    sendernp = get_NXTacct(&createdflag,Global_mp,sender);
    sprintf(retbuf,"telepod_received from NXT.%s crc.%08x ind.%d height.%d %.8f %s %s/%d (%s) (%s)",sender,crc,ind,height,dstr(satoshis),coinaddr,txid,vout,pubkey,privkey);
    cp = get_coin_info(coinstr);
    if ( cp == 0 || crc == 0 || ind < 0 || height == 0 || satoshis == 0 || coinaddr[0] == 0 || txid[0] == 0 || vout < 0 || pubkey[0] == 0 || privkey[0] == 0 )
        strcat(retbuf," <<<<< ERROR");
    else
    {
        errflag = 0;
        pod = create_telepod(0,NXTACCTSECRET,cp,satoshis,coinaddr,pubkey,privkey,txid,vout);
        sendernp->incomingcrcs[ind] = crc;
        ((struct telepod **)sendernp->incoming)[ind] = pod;

        // jl777: need to verify its a valid telepod with matching unspent
        // if all recevied, start cloning
        if ( _crc32(0,sendernp->incomingcrcs,sendernp->numincoming*sizeof(uint32_t)) == sendernp->totalcrc )
        {
            // jl777: eventually pick random schedule
            for (i=0; i<sendernp->numincoming; i++)
            {
                if ( (pod= ((struct telepod **)sendernp->incoming)[i]) != 0 )
                {
                    sendernp->clones[i] = clone_telepod(cp,NXTACCTSECRET,pod);
                    update_minipod_info(cp,sendernp->clones[i]);
                    if ( ensure_telepod_has_backup(cp,sendernp->clones[i],NXTACCTSECRET) < 0 )
                    {
                        printf("error cloning pod[%d] from NXT.%s\n",i,sender);
                        errflag++;
                    }
                }
            }
            retstr = calc_transporter_summary(cp,sendernp,(struct telepod **)sendernp->clones,(struct telepod **)sendernp->incoming,sendernp->numincoming);
        } else errflag = -1;
    }
    send_transporter_status(verifiedNXTaddr,NXTACCTSECRET,sendernp,cp,sendernp->totalcrc,satoshis,errflag,sendernp->numincoming,sendernp->incomingcrcs);
    if ( retstr != 0 )
    {
        strcpy(retbuf,sendmessage(verifiedNXTaddr,NXTACCTSECRET,retstr,(int32_t)strlen(retstr)+1,sender,retstr));
        free(retstr);
    }
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
    if ( cp == 0 || totalcrc == 0 || minage <= 0 || height == 0 || value == 0 || n <= 0 || cp->changepod == 0 )
        strcat(retbuf," <<<<< ERROR"), errflag = -1;
    else if ( sendernp->incomingcrcs == 0 )
    {
        if ( _crc32(0,crcs,n * sizeof(*crcs)) == totalcrc )
        {
            sendernp->totalcrc = totalcrc;
            sendernp->numincoming = n;
            sendernp->clones = calloc(n,sizeof(*sendernp->clones));
            sendernp->incomingcrcs = calloc(n,sizeof(*crcs));
            memcpy(sendernp->incomingcrcs,crcs,n*sizeof(*crcs));
            errflag = 0;
        } else sprintf(retbuf + strlen(retbuf)," totalcrc %08x vs %08x ERROR",totalcrc,_crc32(0,crcs,n * sizeof(*crcs)));
    }
    else strcat(retbuf," <<<<< ERROR already have incomingcrcs");
    send_transporter_status(verifiedNXTaddr,NXTACCTSECRET,sendernp,cp,totalcrc,value,errflag,n,0);
    return(clonestr(retbuf));
}
#endif
