//
//  telepods.h
//  xcode
//
//  Created by jl777 on 8/6/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_telepods_h
#define xcode_telepods_h

#define _get_privkeyptr(pod,sharei) ((uint8_t *)((long)(pod)->privkey_shares + (sharei) * (pod)->len_plus1))
struct telepod *find_telepod(struct coin_info *cp,char *txid)
{
    uint64_t hashval;
    if ( cp == 0 )
        return(0);
    hashval = MTsearch_hashtable(&cp->telepods,txid);
   // printf("find_telepod.(%s) cp.%p telepods.%p hashtable.%p hashval.%d\n",txid,cp,cp->telepods,cp->telepods->hashtable,(int)hashval);
    if ( hashval == HASHSEARCH_ERROR )
        return(0);
    else return(cp->telepods->hashtable[hashval]);
}

void disp_telepod(char *msg,struct telepod *pod)
{
    int32_t calc_multisig_N(struct telepod *pod);
    //char hexstr[1024];
    //init_hexbytes_noT(hexstr,_get_privkeyptr(pod,calc_multisig_N(pod)),pod->len_plus1-1);
    printf("%p %6s %13.8f height.%-6d %6s %s %s/vout_%d priv.(%s)\n",pod,msg,dstr(pod->satoshis),pod->height,pod->coinstr,pod->coinaddr,pod->txid,pod->vout,_get_privkeyptr(pod,calc_multisig_N(pod)));
}

void beg_for_changepod(struct coin_info *cp)
{
    // jl777: need to send request to faucet
    if ( calc_transporter_fee(cp,0) != 0 )
        printf("beg for changepod for %s\n",cp->name);
}

void set_telepod_fname(int32_t selector,char *fname,char *coinstr,int32_t podnumber)
{
    sprintf(fname,"%s/telepods/%s.%d",selector==0?"backups":"archive",coinstr,podnumber);
}

int32_t calc_multisig_N(struct telepod *pod)
{
    int32_t N;
    if ( pod == 0 )
        return(1);
    N = ((pod->podsize - sizeof(*pod)) / pod->len_plus1) - 1;
    if ( N < 1 )
        N = 1;
    else if ( N > 254 )
        N = 254;
    //printf("N is %d\n",N);
    return(N);
}

uint32_t calc_telepodcrc(struct telepod *pod)
{
    uint32_t N,crc = 0;
    N = calc_multisig_N(pod);
    //crc = _crc32(crc,(void *)((long)&pod->modified + sizeof(pod->modified)),sizeof(*pod) - ((long)&pod->modified - (long)pod));
    crc = _crc32(crc,_get_privkeyptr(pod,N),pod->len_plus1 - 1);
    return(crc);
}

int32_t update_telepod_file(struct coin_info *cp,int32_t filenum,struct telepod *pod)
{
    FILE *fp;
    uint8_t *encrypted,*decrypted;
    int32_t iter,podlen,retval = -1;
    char fname[512];
    set_telepod_fname(1,fname,cp->name,filenum);
    if ( (fp= fopen(fname,"rb")) != 0 )
    {
        fclose(fp);
        iter = 1;
    }
    else iter = 0;
    for (; iter<2; iter++)
    {
        set_telepod_fname(iter^1,fname,cp->name,filenum);
        pod->saved = 1;
        pod->filenum = filenum;
        pod->crc = calc_telepodcrc(pod);
        podlen = pod->podsize;
        if ( (encrypted= save_encrypted(fname,cp,(uint8_t *)pod,&podlen)) != 0 )
        {
            free(encrypted);
            if ( (decrypted= load_encrypted(&podlen,fname,cp)) != 0 )
            {
                if ( podlen != pod->podsize )
                    printf("ERROR: podlen.%d vs %d pod->podsize after encrypt/decrypt\n",podlen,pod->podsize);
                if ( memcmp(pod,decrypted,podlen) != 0 )
                    printf("ERROR: decrypt compare error??\n");
                free(decrypted);
            } else printf("ERROR: couldnt load_encrypted telepod.(%s)\n",fname);
        } else printf("ERROR: couldnt save_encrypted telepod.(%s)\n",fname);
    }
    return(retval);
}

void change_podstate(struct coin_info *cp,struct telepod *pod,int32_t podstate)
{
    printf("change podstate %02x -> %02x for %s.%d %.8f\n",pod->podstate,pod->podstate|podstate,cp->name,pod->filenum,dstr(pod->satoshis));
    pod->podstate |= podstate;
    if ( update_telepod_file(cp,pod->filenum,pod) != 0 )
        printf("change_podstate ERROR: saving cloned refpod\n");
}

struct telepod *_load_telepod(int32_t selector,char *coinstr,int32_t podnumber,char *refcipher,cJSON *ciphersobj)
{
    int32_t podlen;
    char fname[512];
    struct coin_info *cp;
    struct telepod *pod = 0;
    cp = get_coin_info(coinstr);
    if ( cp == 0 )
    {
        printf("cant get coininfo.(%s)\n",coinstr);
        return(0);
    }
    set_telepod_fname(selector,fname,coinstr,podnumber);
    if ( (pod= (struct telepod *)load_encrypted(&podlen,fname,cp)) != 0 )
    {
        printf("%s pod.%p\n",fname,pod);
        if ( pod->podsize != podlen )
        {
            printf("telepod corruption: (%s) podsize.%d != fsize.%d\n",fname,pod->podsize,podlen);
            free(pod);
            pod = 0;
        }
        else if ( pod->filenum != 0 && pod->filenum != podnumber )
        {
            printf("warning: pod number.%d -> %d\n",pod->filenum,podnumber);
            pod->filenum = podnumber;
        }
        if ( pod != 0 )
        {
            if ( calc_telepodcrc(pod) != pod->crc )
            {
                printf("telepod corruption: (%s) crc mistatch.%08x != %08x\n",fname,calc_telepodcrc(pod),pod->crc);
                free(pod);
                pod = 0;
            }
            pod->log = 0;
            //pod->dir = 0;
        }
    } //else printf("cant load (%s)\n",fname);
    return(pod);
}

int32_t truncate_telepod_file(struct telepod *pod,char *refcipher,cJSON *ciphersobj)
{
    long err;
    struct telepod *loadpod;
    if ( (loadpod= _load_telepod(0,pod->coinstr,pod->filenum,refcipher,ciphersobj)) != 0 )
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

int32_t find_telepod_slot(char *name,int32_t filenum,char *refcipher)
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
        set_telepod_fname(0,fname,name,filenum);
        printf("check (%s)\n",fname);
    }
    while ( (fp= fopen(fname,"rb")) != 0 );
    return(filenum);
}

struct telepod *ensure_telepod_has_backup(struct coin_info *cp,struct telepod *refpod)
{
    struct telepod *pod,*newpod;
    int32_t createdflag;
    uint64_t hashval;
    if ( refpod == 0 )
        return(0);
    if ( refpod->saved == 0 )
        cp->savedtelepods = find_telepod_slot(cp->name,cp->savedtelepods,cp->name);
    if ( update_telepod_file(cp,cp->savedtelepods,refpod) == 0 )
    {
        if ( (newpod= _load_telepod(0,cp->name,cp->savedtelepods,cp->name,cp->ciphersobj)) == 0 )
            printf("error verifying telepod.%d\n",cp->savedtelepods);
        else
        {
            // WARNING: advanced hashtable replacement, be careful to mess with this
            pod = MTadd_hashtable(&createdflag,&cp->telepods,refpod->txid);
            hashval = MTsearch_hashtable(&cp->telepods,refpod->txid);
            if ( hashval != HASHSEARCH_ERROR )
            {
                if ( pod == cp->telepods->hashtable[hashval] )
                {
                    cp->savedtelepods++;
                    cp->telepods->hashtable[hashval] = newpod;
                    free(pod);
                    return(newpod);
                }
                else
                {
                    printf("ensure_telepod_has_backup: unexpected newpod %p != %p in [%llu]\n",newpod,cp->telepods->hashtable[hashval],(long long)hashval);
                }
            } else printf("ensure_telepod_has_backup: unexpected unfound pod??\n");
            free(newpod);
        }
    }
    return(0);
}

void set_privkey_share(struct telepod *pod,char *privkey,int32_t sharei)
{
    int32_t N;
    char *privkey_share;
    N = calc_multisig_N(pod);
    if ( sharei < 0 || sharei >= N || sharei == 0xff )
        sharei = N;
    privkey_share = (char *)_get_privkeyptr(pod,sharei);
    //printf("set sharei.%d <- (%s)\n",sharei,privkey);
    memcpy(privkey_share,privkey,pod->len_plus1);
    privkey_share[pod->len_plus1-1] = 0;
}

struct telepod *create_telepod(int32_t saveflag,char *refcipher,cJSON *ciphersobj,struct coin_info *cp,uint64_t satoshis,char *podaddr,char *script,char *privkey,char *txid,int32_t vout,uint8_t M,uint8_t N,uint8_t sharenrs[255],uint8_t sharei,int32_t height)
{
    struct telepod *pod;
    int32_t size,len;
    len = (int32_t)strlen(privkey);
    size = (int32_t)(sizeof(*pod) + (N+1)*(len + 1));
    pod = calloc(1,size);
    pod->len_plus1 = (len + 1);
    if ( height == 0 )
        pod->height = (uint32_t)get_blockheight(cp);
    else pod->height = height;
    pod->podsize = size;
    pod->vout = vout;
    pod->cloneout = -1;
    pod->satoshis = satoshis;
    safecopy(pod->coinstr,cp->name,sizeof(pod->coinstr));
    safecopy(pod->txid,txid,sizeof(pod->txid));
    safecopy(pod->coinaddr,podaddr,sizeof(pod->coinaddr));
    safecopy(pod->script,script,sizeof(pod->script));
    set_privkey_share(pod,privkey,sharei);
    pod->crc = calc_telepodcrc(pod);
    if ( saveflag != 0 )
        ensure_telepod_has_backup(cp,pod);
    disp_telepod("create",pod);
    if ( N != calc_multisig_N(pod) )
        fatal("create_telepod: N != calc_multisig_N(pod)");
    return(pod);
}

void update_minipod_info(struct coin_info *cp,struct telepod *pod)
{
    struct telepod *refpod;
    if ( calc_transporter_fee(cp,0) == 0 )
    {
        pod->ischange = 0;
        if ( pod->cloneout < 0 )
        {
            pod->dir = 0;
            pod->sentmilli = 0;
            pod->completemilli = 0;
            pod->recvmilli = 0;
            pod->clonetxid[0] = 0;
        }
        return;
    }
    if ( pod->satoshis <= cp->min_telepod_satoshis && pod->satoshis >= cp->txfee && pod->clonetxid[0] == 0 && pod->height < (uint32_t)(get_blockheight(cp) - cp->minconfirms) )
    {
        if ( (refpod= cp->changepod) == 0 || (pod->height != 0 && (refpod->height == 0 || pod->height < refpod->height)) )
        {
            cp->changepod = pod;
            if ( refpod != 0 )
                refpod->ischange = 0;
            printf("SET CHANGE!\n");
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
            if ( pod->spentflag == 0 && (height - pod->height) > minage && pod->ischange == 0 && (pod->dir == 0 || pod->dir == TRANSPORTER_RECV) && pod->script[0] != 0 )
            {
                (*balancep) += pod->satoshis;
                pods[numactive++] = pod;
            }
            else printf("reject telepod.%d height.%d - pod->height.%d minage.%d, ischange.%d dir.%d script.(%s)\n",i,height,pod->height,minage,pod->ischange,pod->dir,pod->script);
        }
    }
    free(rawlist);
    printf("numactive.%d\n",numactive);
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
    printf("selected_changepod %p\n",cp->changepod);
    return(cp->changepod);
}

void load_telepods(struct coin_info *cp,int32_t maxnofile)
{
    int savedtelepods,maxfilenum,earliest = 0,nofile = 0;
    struct telepod *pod = 0;
    if ( cp->telepods != 0 )
    {
        if ( cp->changepod == 0 )
            cp->changepod = select_changepod(cp,cp->minconfirms);
    }
    cp->telepods = hashtable_create("telepods",HASHTABLES_STARTSIZE,sizeof(*pod),((long)&pod->txid[0] - (long)pod),sizeof(pod->txid),((long)&pod->modified - (long)pod));
    if ( cp->min_telepod_satoshis == 0 )
        cp->min_telepod_satoshis = SATOSHIDEN/100;
    maxfilenum = savedtelepods = cp->savedtelepods;
    if ( savedtelepods > 0 )
        savedtelepods--;
    //printf("load_telepods.%s saved.%d max.%d\n",cp->name,savedtelepods,maxfilenum);
    while ( nofile < maxnofile )
    {
        if ( (pod= _load_telepod(0,cp->name,savedtelepods,cp->name,cp->ciphersobj)) != 0 )
        {
            maxfilenum = savedtelepods + 1;
            if ( pod->clonetxid[0] == 0 )
            {
                update_minipod_info(cp,pod);
                if ( pod->height != 0 && (earliest == 0 || pod->height < earliest) )
                    earliest = pod->height;
                if ( pod->sentheight != 0 && (earliest == 0 || pod->sentheight < earliest) )
                    earliest = pod->sentheight;
                ensure_telepod_has_backup(cp,pod);
                disp_telepod("load",pod);
            }
            else disp_telepod("spent",pod);
        }
        else nofile++;
        savedtelepods++;
        //printf("%s nofile.%d savedtelepods.%d\n",cp->name,nofile,savedtelepods);
    }
    if ( maxfilenum > cp->savedtelepods )
        cp->savedtelepods = maxfilenum;
    if ( cp->changepod == 0 )
        beg_for_changepod(cp);
    if ( earliest != 0 && (cp->blockheight == 0 || earliest < cp->blockheight) )
    {
        cp->blockheight = earliest;
        cp->enabled = 1;
    }
    printf("after loaded %d telepods.%s earliest.%d enabled.%d | blockheight.%ld\n",cp->savedtelepods,cp->name,earliest,cp->enabled,(long)cp->blockheight);
}

struct telepod *clone_telepod(struct coin_info *cp,struct telepod *refpod,uint64_t refsatoshis)
{
    char *change_podaddr=0,*change_privkey,*podaddr=0,*txid,*privkey,pubkey[1024],change_pubkey[1024];
    struct rawtransaction RAW;
    uint8_t sharenrs[255],M,N;
    struct telepod *pod = 0,*changepod,*inputpods[MAX_COIN_INPUTS],*refpods[2];
    struct transporter_log *log = 0;
    uint64_t fee,change,availchange = 0;
    int32_t i;
    memset(inputpods,0,sizeof(inputpods));
    changepod = cp->changepod;
    change_privkey = change_podaddr = 0;
    availchange = fee = change_pubkey[0] = 0;
    if ( refpod != 0 )
    {
        refpods[0] = refpod;
        refpods[1] = 0;
        log = refpod->log;
        if ( refsatoshis != 0 )
        {
            printf("clone_telepod: unexpected nonzero %.8f refsatoshis\n",dstr(refsatoshis));
            return(0);
        }
        refsatoshis = refpod->satoshis;
        printf("clone telepod log.%p\n",log);
        if ( log == 0 )
            return(0);
        M = log->M;
        N = log->N;
    } else M = N = 1;
    fee = calc_transporter_fee(cp,refsatoshis);
    if ( changepod != 0 )
    {
        availchange = changepod->satoshis;
        change_privkey = get_telepod_privkey(&change_podaddr,change_pubkey,cp);
    }
    else
    {
        change_podaddr = get_transporter_unspent(inputpods,&availchange,cp);
        if ( change_podaddr == 0 || availchange < refsatoshis+fee )
        {
            printf("clone_telepod: cant get transporter addr || avail %.8f < %.8f + %.8f\n",dstr(availchange),dstr(refsatoshis),dstr(fee));
            return(0);
        }
        printf("changeaddr.(%s) availchange %.8f, refsatoshis %.8f\n",change_podaddr,dstr(availchange),dstr(refsatoshis));
        availchange -= refsatoshis;
        printf("availchange %.8f refsatoshis %.8f\n",dstr(availchange),dstr(refsatoshis));
    }
    if ( (privkey= get_telepod_privkey(&podaddr,pubkey,cp)) != 0 )
    {
        if ( fee <= availchange )
        {
            change = (availchange - fee);
            memset(&RAW,0,sizeof(RAW));
            if ( (txid= calc_telepod_transaction(cp,&RAW,refpod!=0?refpods:inputpods,refsatoshis,podaddr,fee,changepod,change,change_podaddr)) == 0 )
            {
                if ( refpod != 0 )
                    refpod->cloneout = TELEPOD_ERROR_VOUT;
                printf("error cloning %.8f telepod.(%s) to %s\n",dstr(refsatoshis),refpod!=0?refpod->coinaddr:"transporter",podaddr);
            }
            else
            {
                if ( refpod != 0 )
                {
                    safecopy(refpod->clonetxid,txid,sizeof(refpod->clonetxid)), refpod->cloneout = TELEPOD_CONTENTS_VOUT;
                    printf("set refpod.%p (%s).vout%d\n",refpod,refpod->clonetxid,refpod->cloneout);
                    if ( update_telepod_file(cp,refpod->filenum,refpod) != 0 )
                        printf("ERROR saving cloned refpod\n");
                }
                init_sharenrs(sharenrs,0,N,N);
                pod = create_telepod(1,cp->name,cp->ciphersobj,cp,refsatoshis,podaddr,"",privkey,txid,TELEPOD_CONTENTS_VOUT,M,N,sharenrs,N,0);
                pod->height = (uint32_t)get_blockheight(cp);
                printf("SET CLONE HEIGHT <- %d\n",pod->height);
                if ( change != 0 && changepod != 0 )
                {
                    printf("clone_telepod: UNEXPECTED case of having change %s %.8f %.8f!\n",cp->name,dstr(refsatoshis),dstr(change));
                    safecopy(changepod->clonetxid,txid,sizeof(changepod->clonetxid)), changepod->cloneout = TELEPOD_CHANGE_VOUT;
                    if ( update_telepod_file(cp,changepod->filenum,changepod) != 0 )
                        printf("ERROR saving changepod after used\n");
                    init_sharenrs(sharenrs,0,N,N);
                    changepod = create_telepod(1,cp->name,cp->ciphersobj,cp,change,change_podaddr,"",change_privkey,txid,TELEPOD_CHANGE_VOUT,M,N,sharenrs,N,0);
                    changepod->dir = TRANSPORTER_RECV;
                    changepod->height = 0;
                    //queue_enqueue(&cp->podQ.pingpong[0],changepod);
                    if ( (cp->changepod= select_changepod(cp,cp->minconfirms)) == 0 )
                        cp->changepod = changepod;
                }
            }
            if ( cp->enabled == 0 )
                cp->blockheight = (uint32_t)get_blockheight(cp), cp->enabled = 1;
            purge_rawtransaction(&RAW);
        } else printf("clone_telepod fee %llu change %llu\n",(long long)fee,(long long)availchange);
        if ( change_privkey != 0 )
            free(change_privkey);
        if ( change_podaddr != 0 )
            free(change_podaddr);
        free(privkey);
        free(podaddr);
    }
    for (i=0; i<MAX_COIN_INPUTS; i++)
        if ( inputpods[i] != 0 )
            free(inputpods[i]);
    return(pod);
}

int32_t make_traceable_telepods(struct coin_info *cp,char *refcipher,cJSON *ciphersobj,uint64_t satoshis)
{
    int32_t i,n,standard_denominations[] = { 10000, 5000, 1000, 500, 100, 50, 20, 10, 5, 1 };
    uint64_t incr,amount;
    struct telepod *pod;
    n = 0;
    while ( satoshis >= standard_denominations[(sizeof(standard_denominations)/sizeof(*standard_denominations))-1] )
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
        printf("satoshis %.8f, i.%d min %.8f\n",dstr(satoshis),i,dstr(cp->min_telepod_satoshis));
        pod = clone_telepod(cp,0,amount);//make_traceable_telepod(cp,refcipher,ciphersobj,amount);
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

char *maketelepods(char *NXTACCTSECRET,char *sender,char *coinstr,int64_t value)
{
    struct coin_info *cp;
    if ( (cp= get_coin_info(coinstr)) != 0 )
    {
        if ( make_traceable_telepods(cp,cp->name,cp->ciphersobj,value) <= 0 )
            return(clonestr("{\"error\":\"maketelepod telepods couldnt created\"}"));
        else return(clonestr("{\"result\":\"maketelepod created telepods\"}"));
    } else return(clonestr("{\"error\":\"maketelepod cant get coininfo\"}"));
}

int32_t teleport_telepod(char *mypubaddr,char *NXTaddr,char *NXTACCTSECRET,char *destNXTaddr,struct telepod *pod,uint32_t totalcrc,uint32_t sharei,int32_t ind,int32_t M,int32_t N,uint8_t *buffer)
{
    cJSON *json;
    char numstr[32],hexstr[4096];
    json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"requestType",cJSON_CreateString("telepod"));
    cJSON_AddItemToObject(json,"crc",cJSON_CreateNumber(pod->crc));
    cJSON_AddItemToObject(json,"i",cJSON_CreateNumber(ind));
    cJSON_AddItemToObject(json,"h",cJSON_CreateNumber(pod->height));
    cJSON_AddItemToObject(json,"c",cJSON_CreateString(pod->coinstr));
    sprintf(numstr,"%.8f",(double)pod->satoshis/SATOSHIDEN);
    cJSON_AddItemToObject(json,"v",cJSON_CreateString(numstr));
    cJSON_AddItemToObject(json,"a",cJSON_CreateString(pod->coinaddr));
    cJSON_AddItemToObject(json,"t",cJSON_CreateString(pod->txid));
    cJSON_AddItemToObject(json,"o",cJSON_CreateNumber(pod->vout));
    cJSON_AddItemToObject(json,"p",cJSON_CreateString(pod->script));
    if ( sharei == 0xff || sharei >= N )
        sharei = N;
    init_hexbytes_noT(hexstr,buffer,pod->len_plus1);
    cJSON_AddItemToObject(json,"k",cJSON_CreateString(hexstr));
    cJSON_AddItemToObject(json,"L",cJSON_CreateNumber(totalcrc));
    cJSON_AddItemToObject(json,"s",cJSON_CreateNumber(sharei));
    cJSON_AddItemToObject(json,"M",cJSON_CreateNumber(M));
    cJSON_AddItemToObject(json,"N",cJSON_CreateNumber(N));
    cJSON_AddItemToObject(json,"D",cJSON_CreateString(mypubaddr));
    return(sendandfree_jsoncmd(Global_mp->Lfactor,NXTaddr,NXTACCTSECRET,json,destNXTaddr));
}

#endif
