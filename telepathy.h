//
//  contacts.h
//  libjl777
//
//  Created by jl777 on 10/15/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//


#ifndef contacts_h
#define contacts_h

struct telepathy_args
{
    uint64_t mytxid,othertxid,refaddr,bestaddr,refaddrs[8],otheraddrs[8];
    bits256 mypubkey,otherpubkey;
    int numrefs;
};

struct contact_info
{
    bits256 pubkey,shared;
    char handle[64];
    uint64_t nxt64bits,deaddrops[8];
    int32_t numsent,numrecv;
} *Contacts;
int32_t Num_contacts,Max_contacts;
portable_mutex_t Contacts_mutex;

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

uint64_t calc_seqaddr(char *buf,uint64_t nxt64bits,int32_t sequenceid)
{
    bits256 secret,pubkey;
    sprintf(buf,"%llu.%d",(long long)nxt64bits,sequenceid);
    return(conv_NXTpassword(secret.bytes,pubkey.bytes,buf));
}

uint64_t calc_privatelocation(char *seqpass,bits256 *passp,int32_t dir,struct contact_info *contact,int32_t sequenceid)
{
    static bits256 zerokey;
    uint64_t nxt64bits,seqaddr;
    struct coin_info *cp = get_coin_info("BTCD");
    if ( cp == 0 || memcmp(&zerokey,&contact->shared,sizeof(zerokey)) == 0 )
        return(0);
    if ( dir > 0 ) // transmission
        nxt64bits = cp->privatebits;
    else nxt64bits = contact->nxt64bits;
    seqaddr = calc_seqaddr(seqpass,nxt64bits,sequenceid);
    calc_sha256cat(passp->bytes,(uint8_t *)seqpass,(int32_t)strlen(seqpass),contact->shared.bytes,(int32_t)sizeof(contact->shared));
    return(seqaddr);
}

uint8_t *AES_codec(int32_t *lenp,int32_t decryptflag,char *msg,uint8_t *data,int32_t *datalenp,char *name,char *password)
{
    int32_t i,*cipherids,len;
    char **privkeys,*decompressed;
    uint8_t *retdata,*combined = 0;
    struct compressed_json *compressed = 0;
    privkeys = gen_privkeys(&cipherids,name,password,GENESIS_SECRET,"");
    if ( decryptflag == 0 )
    {
        len = (int32_t)strlen(msg);
        if ( data != 0 && *datalenp > 0 )
        {
            combined = calloc(1,1 + len + *datalenp);
            memcpy(combined,msg,len + 1);
            memcpy(combined+len,data,*datalenp);
            len += (*datalenp);
            compressed = 0;//encode_json((char *)combined,len);
        } else compressed = 0;//encode_json(msg,len);
        if ( compressed != 0 )
        {
            *lenp = compressed->complen;
            data = (uint8_t *)compressed;
            free(combined);
        }
        else if ( combined != 0 )
        {
            data = combined;
            *lenp = len;
        }
        else
        {
            data = (uint8_t *)clonestr(msg);
            len++;
            *lenp = len;
        }
    }
    else *lenp = *datalenp;
    retdata = ciphers_codec(decryptflag,privkeys,cipherids,data,lenp);
    if ( decryptflag != 0 )
    {
        compressed = (struct compressed_json *)retdata;
        decompressed = 0;//decode_json(compressed,0);
        if ( decompressed != 0 )
        {
            free(retdata);
            retdata = (uint8_t *)decompressed;
            *lenp = compressed->origlen;
            *datalenp = (int32_t)(compressed->origlen - strlen(decompressed) - 1);
        }
    } else free(data);
    if ( privkeys != 0 )
    {
        for (i=0; privkeys[i]!=0; i++)
            free(privkeys[i]);
        free(privkeys);
        free(cipherids);
    }
    return(retdata);
}

int32_t verify_AES_codec(uint8_t *encoded,int32_t origlen,char *name,char *passwordstr,char *msg,char *datastr)
{
    int32_t datalen,len,decodedlen,retval = -1;
    uint8_t *decoded,*dataptr,data[8192];
    decodedlen = 0;
    len = origlen;
    decoded = AES_codec(&decodedlen,1,0,encoded,&len,name,passwordstr);
    if ( decoded != 0 )
    {
        if ( strcmp(msg,(char *)decoded) == 0 )
        {
            printf("decrypted.(%s)\n",(char *)decoded);
            dataptr = conv_datastr(&datalen,data,datastr);
            if ( dataptr != 0 && memcmp(msg+strlen(msg)+1,dataptr,len - strlen(msg) - 1) != 0 )
                printf("AES_codec error on datastr\n");
            else retval = 0;
        } else printf("AES_codec error on msg\n");
        free(decoded);
    } else printf("AES_codec unexpected null decoded\n");
    return(retval);
}

uint64_t calc_privatedatastr(char *seqpass,char *privatedatastr,struct contact_info *contact,int32_t sequence,char *msg,char *datastr)
{
    bits256 password;
    uint64_t location,retval = 0;
    char passwordstr[512];
    int32_t len,datalen,encodedlen;
    uint8_t *encoded,*dataptr,data[4096];
    if ( (location= calc_privatelocation(seqpass,&password,1,contact,sequence)) != 0 )
    {
        init_hexbytes(passwordstr,password.bytes,sizeof(password));
        dataptr = conv_datastr(&datalen,data,datastr);
        encoded = AES_codec(&encodedlen,0,msg,dataptr,&datalen,contact->handle,passwordstr);
        if ( encoded != 0 )
        {
            init_hexbytes(privatedatastr,encoded,encodedlen);
            len = encodedlen;
            if ( verify_AES_codec(encoded,encodedlen,contact->handle,passwordstr,msg,datastr) == 0 )
                retval = location;
            free(encoded);
        }
    }
    return(retval);
}

double groupdist(uint64_t *addrs,int32_t n,uint64_t addr)
{
    int32_t i,dist = 0;
    for (i=0; i<n; i++)
        dist += bitweight(addrs[i] ^ addr);
    return((double)dist / n);
}

int32_t update_bestdist(uint64_t *bestaddrp,int32_t bestdist,uint64_t testaddr,int32_t dist)
{
    if ( dist < bestdist )
    {
        *bestaddrp = testaddr;
        bestdist = dist;
    }
    return(bestdist);
}

uint64_t calc_quadaddr(uint64_t a,uint64_t b,uint64_t c,uint64_t d)
{
    uint8_t r[8];
    int32_t i,j,dist,dista,distb,distc,distd,bestdist,numdiff = 0;
    uint64_t allset,allclear,mask,testaddr,bestaddr = 0;
    allset = allclear = 0;
    for (i=0; i<64; i++)
    {
        mask = (1L << i);
        if ( (mask & a) == 0 && (mask & b) == 0 && (mask & c) == 0 && (mask & d) == 0 )
            allclear |= mask;
        else if ( (mask & a) != 0 && (mask & b) != 0 && (mask & c) != 0 && (mask & d) != 0 )
            allset |= mask;
        else numdiff++;
    }
    bestdist = 64;
    for (i=0; i<100000; i++)
    {
        for (j=0; j<8; j++)
            r[j] = (rand() >> 8) & 0xff;
        memcpy(&testaddr,r,sizeof(testaddr));
        testaddr |= allset;
        testaddr &= (~allclear);
        dista = bitweight(testaddr ^ a);
        distb = bitweight(testaddr ^ b);
        distc = bitweight(testaddr ^ c);
        distd = bitweight(testaddr ^ d);
        dist = (dista + distb + distc + distd);
        bestdist = update_bestdist(&bestaddr,bestdist,testaddr,dist);
    }
    printf("allclear.%d allset.%d bestaddr.%d\n",bitweight(allclear),bitweight(allset),bestdist);
    return(bestaddr);
}

int32_t calc_global_deaddrop(uint64_t *deaddrops,int32_t max)
{
    double dist,bestdist = (1./SMALLVAL);
    int32_t i,j,numrefs,maxrefs;
    uint64_t alladdrs[64],refaddrs[64],pairaddrs[32],bestaddr = 0;
    memset(alladdrs,0,sizeof(alladdrs));
    memset(refaddrs,0,sizeof(refaddrs));
    maxrefs = numrefs = scan_nodes(alladdrs,sizeof(alladdrs)/sizeof(*alladdrs),GENESIS_SECRET);
    memcpy(refaddrs,alladdrs,numrefs * sizeof(*refaddrs));
    while ( numrefs >= 2 )
    {
        for (i=j=0; i<(numrefs>>1)-2; i++,j++)
        {
            pairaddrs[j] = calc_quadaddr(refaddrs[i<<1],refaddrs[(i<<1)+1],refaddrs[(i<<1)+2],refaddrs[(i<<1)+3]);
            printf("(%.1f %.1f).%.1f ",groupdist(alladdrs,maxrefs,refaddrs[i<<1]),groupdist(alladdrs,maxrefs,refaddrs[(i<<1)+1]),groupdist(alladdrs,maxrefs,pairaddrs[j]));
        }
        if ( (i<<1) < numrefs )
        {
            pairaddrs[j++] = refaddrs[i<<1];
            printf("%.1f ",groupdist(alladdrs,maxrefs,refaddrs[i<<1]));
        }
        for (i=0; i<numrefs+j; i++)
        {
            dist = groupdist(alladdrs,maxrefs,(i<numrefs) ? refaddrs[i] : pairaddrs[i-numrefs]);
            if ( dist < bestdist )
            {
                bestdist = dist;
                bestaddr = refaddrs[i];
            }
        }
        printf("numrefs.%d j.%d | best %.1f\n",numrefs,j,bestdist);
        memcpy(refaddrs,pairaddrs,j * sizeof(*refaddrs));
        numrefs = j;
    }
    deaddrops[0] = bestaddr;
    for (i=1; i<max&&i<=maxrefs/2; i++)
        deaddrops[i] = pairaddrs[i-1];
    return(i);
}

char *private_publish(struct contact_info *contact,int32_t sequenceid,char *msg,char *datastr)
{
    int32_t i,n;
    uint64_t location,deaddrops[16];
    char privatedatastr[8192],seqpass[512],seqacct[64],key[64],*retstr = 0;
    if ( (location= calc_privatedatastr(seqpass,privatedatastr,contact,0,msg,datastr)) > 0 )
    {
        expand_nxt64bits(seqacct,location);
        if ( sequenceid == 0 )
        {
            expand_nxt64bits(key,location);
            retstr = kademlia_storedata(0,seqacct,seqpass,seqacct,key,privatedatastr);
        }
        else
        {
            memset(deaddrops,0,sizeof(deaddrops));
            for (i=n=0; i<(int)(sizeof(contact->deaddrops)/sizeof(*contact->deaddrops)); i++)
                if ( contact->deaddrops[i] != 0 )
                    deaddrops[n++] = contact->deaddrops[i];
            if ( n > 0 )
            {
                deaddrops[0] = deaddrops[(rand()>>8) % n];
                n = 1;
            }
            else n = calc_global_deaddrop(deaddrops,(int)(sizeof(deaddrops)/sizeof(*deaddrops)));
            for (i=0; i<n; i++)
            {
                expand_nxt64bits(key,deaddrops[i]);
                retstr = kademlia_find("findnode",0,seqacct,seqpass,seqacct,key,privatedatastr);
            }
        }
    }
    return(retstr);
}

struct contact_info *_find_handle(char *handle)
{
    int32_t i;
    if ( Num_contacts != 0 )
    {
        for (i=0; i<Num_contacts; i++)
            if ( strcmp(Contacts[i].handle,handle) == 0 )
                return(&Contacts[i]);
    }
    return(0);
}

struct contact_info *_find_contact_nxt64bits(uint64_t nxt64bits)
{
    int32_t i;
    if ( Num_contacts != 0 )
    {
        for (i=0; i<Num_contacts; i++)
            if ( Contacts[i].nxt64bits == nxt64bits )
                return(&Contacts[i]);
    }
    return(0);
}

struct contact_info *_find_contact(char *contactstr)
{
    int32_t len;
    uint64_t nxt64bits = 0;
    struct contact_info *contact = 0;
    if ( (contact= _find_handle(contactstr)) == 0 )
    {
        if ( (len= is_decimalstr(contactstr)) > 0 && len < 22 )
            nxt64bits = calc_nxt64bits(contactstr);
        else if ( strncmp("NXT-",contactstr,4) == 0 )
            nxt64bits = conv_rsacctstr(contactstr,0);
        contact = _find_contact_nxt64bits(nxt64bits);
    }
    return(contact);
}

struct contact_info *find_contact(char *handle)
{
    struct contact_info *contact = 0;
    portable_mutex_lock(&Contacts_mutex);
    contact = _find_contact(handle);
    portable_mutex_unlock(&Contacts_mutex);
    return(contact);
}

char *addcontact(struct sockaddr *prevaddr,char *NXTaddr,char *NXTACCTSECRET,char *sender,char *handle,char *acct)
{
    uint64_t nxt64bits;
    bits256 mysecret,mypublic;
    struct coin_info *cp = get_coin_info("BTCD");
    struct contact_info *contact;
    char retstr[1024],pubkeystr[128],sharedstr[128];
    if ( cp == 0 )
    {
        printf("addcontact: no BTCD cp?\n");
        return(0);
    }
    handle[sizeof(contact->handle)-1] = 0;
    portable_mutex_lock(&Contacts_mutex);
    if ( (contact= _find_contact(handle)) == 0 )
    {
        if ( Num_contacts >= Max_contacts )
        {
            Max_contacts = (Num_contacts + 1 );
            Contacts = realloc(Contacts,(sizeof(*Contacts) * Max_contacts));
        }
        contact = &Contacts[Num_contacts++];
        safecopy(contact->handle,handle,sizeof(contact->handle));
    }
    else if ( strcmp(handle,"myhandle") == 0 )
       return(clonestr("{\"error\":\"cant override myhandle\"}"));
    nxt64bits = conv_rsacctstr(acct,0);
    if ( nxt64bits != contact->nxt64bits )
    {
        contact->nxt64bits = nxt64bits;
        contact->pubkey = issue_getpubkey(acct);
        init_hexbytes(pubkeystr,contact->pubkey.bytes,sizeof(contact->pubkey));
        if ( contact->pubkey.txid == 0 )
            sprintf(retstr,"{\"error\":\"(%s) acct.(%s) has no pubkey.(%s)\"}",handle,acct,pubkeystr);
        else
        {
            conv_NXTpassword(mysecret.bytes,mypublic.bytes,cp->privateNXTACCTSECRET);
            contact->shared = curve25519(mysecret,contact->pubkey);
            init_hexbytes(sharedstr,contact->shared.bytes,sizeof(contact->shared));
            printf("shared.(%s)\n",sharedstr);
            sprintf(retstr,"{\"result\":\"(%s) acct.(%s) (%llu) has pubkey.(%s)\"}",handle,acct,(long long)contact->nxt64bits,pubkeystr);
        }
    } else sprintf(retstr,"{\"result\":\"(%s) acct.(%s) (%llu) unchanged\"}",handle,acct,(long long)contact->nxt64bits);
    portable_mutex_unlock(&Contacts_mutex);
    printf("ADD.(%s -> %s)\n",handle,acct);
    return(clonestr(retstr));
}

char *removecontact(struct sockaddr *prevaddr,char *NXTaddr,char *NXTACCTSECRET,char *sender,char *handle)
{
    struct contact_info *contact;
    char retstr[1024];
    handle[sizeof(contact->handle)-1] = 0;
    if ( strcmp("myhandle",handle) == 0 )
        return(0);
    portable_mutex_lock(&Contacts_mutex);
    if ( (contact= _find_contact(handle)) != 0 )
    {
        if ( contact != &Contacts[--Num_contacts] )
        {
            *contact = Contacts[Num_contacts];
            memset(&Contacts[Num_contacts],0,sizeof(Contacts[Num_contacts]));
        }
        if ( Num_contacts == 0 )
        {
            Max_contacts = 0;
            free(Contacts);
            Contacts = 0;
            printf("freed all contacts\n");
        }
        sprintf(retstr,"{\"result\":\"handle.(%s) deleted num.%d max.%d\"}",handle,Num_contacts,Max_contacts);
    } else sprintf(retstr,"{\"error\":\"handle.(%s) doesnt exist\"}",handle);
    portable_mutex_unlock(&Contacts_mutex);
    printf("REMOVE.(%s)\n",handle);
    return(clonestr(retstr));
}

void set_contactstr(char *contactstr,struct contact_info *contact)
{
    char pubkeystr[128],rsacctstr[128];
    rsacctstr[0] = 0;
    conv_rsacctstr(rsacctstr,contact->nxt64bits);
    if ( strcmp(contact->handle,"myhandle") == 0 )
        init_hexbytes(pubkeystr,Global_mp->mypubkey.bytes,sizeof(Global_mp->mypubkey));
    else init_hexbytes(pubkeystr,contact->pubkey.bytes,sizeof(contact->pubkey));
    sprintf(contactstr,"{\"result\":\"handle\":\"%s\",\"acct\":\"%s\",\"NXT\":\"%llu\",\"pubkey\":\"%s\"}",contact->handle,rsacctstr,(long long)contact->nxt64bits,pubkeystr);
}

char *dispcontact(struct sockaddr *prevaddr,char *NXTaddr,char *NXTACCTSECRET,char *sender,char *handle)
{
    int32_t i;
    struct contact_info *contact;
    char retbuf[1024],*retstr = 0;
    handle[sizeof(contact->handle)-1] = 0;
    retbuf[0] = 0;
    portable_mutex_lock(&Contacts_mutex);
    if ( strcmp(handle,"*") == 0 )
    {
        retstr = clonestr("[");
        for (i=0; i<Num_contacts; i++)
        {
            set_contactstr(retbuf,&Contacts[i]);
            retstr = realloc(retstr,strlen(retstr)+strlen(retbuf)+2);
            strcat(retstr,retbuf);
            if ( i < Num_contacts-1 )
                strcat(retstr,",");
        }
        strcat(retstr,"]");
    }
    else
    {
        if ( (contact= _find_contact(handle)) != 0 )
            set_contactstr(retbuf,contact);
        else sprintf(retbuf,"{\"error\":\"handle.(%s) doesnt exist\"}",handle);
        retstr = clonestr(retbuf);
    }
    portable_mutex_unlock(&Contacts_mutex);
    printf("Contact.(%s)\n",retstr);
    return(retstr);
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
        retstr = kademlia_find("findvalue",0,cp->srvNXTADDR,cp->srvNXTACCTSECRET,sender,key,0);
        if ( retstr != 0 )
        {
            if ( (json= cJSON_Parse(retstr)) != 0 )
            {
                copy_cJSON(datastr,cJSON_GetObjectItem(json,"data"));
                if ( strlen(datastr) == sizeof(zerokey)*2 )
                {
                    printf("set otherpubkey to (%s)\n",datastr);
                    decode_hex(args->otherpubkey.bytes,sizeof(args->otherpubkey),datastr);
                }
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

char *telepathy_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    //struct coin_info *cp = get_coin_info("BTCD");
    char desthandle[MAX_JSON_FIELD],msg[MAX_JSON_FIELD],datastr[MAX_JSON_FIELD],*retstr = 0;
    //struct telepathy_args args;
    struct contact_info *contact;
    if ( prevaddr != 0 )//|| cp == 0 )
        return(0);
    copy_cJSON(desthandle,objs[0]);
    contact = find_contact(desthandle);
    copy_cJSON(msg,objs[1]);
    copy_cJSON(datastr,objs[2]);
    if ( contact != 0 && desthandle[0] != 0 && msg[0] != 0 && sender[0] != 0 && valid > 0 )
    {
        retstr = private_publish(contact,0,msg,datastr);
         /*if ( retstr != 0 )
            free(retstr);
        memset(&args,0,sizeof(args));
        args.mytxid = myhash.txid;
        args.othertxid = otherhash.txid;
        args.refaddr = cp->privatebits;
        args.numrefs = scan_nodes(args.refaddrs,sizeof(args.refaddrs)/sizeof(*args.refaddrs),NXTACCTSECRET);
        start_task(Task_mindmeld,"telepathy",1000000,(void *)&args,sizeof(args));
        retstr = clonestr(retbuf);*/
    }
    else retstr = clonestr("{\"error\":\"invalid telepathy_func arguments\"}");
    return(retstr);
}

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

char *findaddress(struct sockaddr *prevaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,uint64_t addr,uint64_t *list,int32_t n,int32_t targetdist,int32_t duration,int32_t numthreads)
{
    static double lastbest,endmilli,best,metric;
    static uint64_t calcaddr,bestaddr = 0;
    static char refNXTaddr[64],retbuf[2048],bestpassword[512],bestNXTaddr[64];
    static struct loopargs **args;
    bits256 secret,pubkey;
    int32_t i;
    if ( endmilli == 0. )
    {
        if ( numthreads <= 0 )
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
            args[i]->targetdist = targetdist-(i%5);
            args[i]->best = lastbest;
            args[i]->list = list;
            args[i]->numinlist = n;
            if ( portable_thread_create((void *)findaddress_loop,args[i]) == 0 )
                printf("ERROR hist findaddress_loop\n");
        }
        endmilli = milliseconds() + (duration * 1000.);
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
