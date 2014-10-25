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
    cJSON_AddItemToObject(json,"N",cJSON_CreateNumber(N));
    if ( N > 1 )
    {
        cJSON_AddItemToObject(json,"s",cJSON_CreateNumber(sharei));
        cJSON_AddItemToObject(json,"M",cJSON_CreateNumber(M));
    }
    cJSON_AddItemToObject(json,"D",cJSON_CreateString(mypubaddr));
    return(sendandfree_jsoncmd(Global_mp->Lfactor,NXTaddr,NXTACCTSECRET,json,destNXTaddr));
}

#endif
