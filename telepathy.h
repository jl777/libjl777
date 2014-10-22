//
//  contacts.h
//  libjl777
//
//  Created by jl777 on 10/15/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//


#ifndef contacts_h
#define contacts_h

#define MAX_DROPPED_PACKETS 64

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
    uint64_t nxt64bits,deaddrop,mydrop;
    int32_t numsent,numrecv,lastrecv,lastsent,lastentry;
} *Contacts;

struct telepathy_entry
{
    uint64_t modified,location,contactbits;
    struct kademlia_storage *sp;
    bits256 AESpassword;
    char locationstr[MAX_NXTADDR_LEN];
    int32_t sequenceid;
};

int32_t Num_contacts,Max_contacts;
portable_mutex_t Contacts_mutex;


struct telepathy_entry *find_telepathy_entry(char *locationstr)
{
    uint64_t hashval;
    hashval = MTsearch_hashtable(Global_mp->Telepathy_tablep,locationstr);
    if ( hashval == HASHSEARCH_ERROR )
        return(0);
    else return((*Global_mp->Telepathy_tablep)->hashtable[hashval]);
}

struct telepathy_entry *add_telepathy_entry(char *locationstr,struct contact_info *contact,bits256 AESpassword,int32_t sequenceid)
{
    int32_t createdflag = 0;
    struct telepathy_entry *tel;
    tel = MTadd_hashtable(&createdflag,Global_mp->Telepathy_tablep,locationstr);
    if ( createdflag != 0 )
    {
        tel->location = calc_nxt64bits(locationstr);
        tel->contactbits = contact->nxt64bits;
        tel->AESpassword = AESpassword;
        tel->sequenceid = sequenceid;
        if ( sequenceid > contact->lastentry )
            contact->lastentry = sequenceid;
        if ( (tel->sp= find_storage(PRIVATE_DATA,locationstr)) != 0 )
        {
            if ( sequenceid > contact->lastrecv )
                contact->lastrecv = sequenceid;
            contact->numrecv++;
        }
        printf("add (%s.%d) %llu\n",contact->handle,sequenceid,(long long)tel->location);
    } else printf("add_telepathy_entry warning: already created %s.%s\n",contact->handle,locationstr);
    return(tel);
}

struct contact_info *_find_handle(char *handle)
{
    int32_t i;
    if ( Num_contacts != 0 )
    {
        //printf("find handle.(%s)\n",handle);
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
        //printf("_find_contact_nxt64bits.(%llu)\n",(long long)nxt64bits);
        for (i=0; i<Num_contacts; i++)
            if ( Contacts[i].nxt64bits == nxt64bits )
                return(&Contacts[i]);
    }
    return(0);
}

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

void free_privkeys(char **privkeys,int32_t *cipherids)
{
    int32_t i;
    if ( privkeys != 0 )
    {
        for (i=0; privkeys[i]!=0; i++)
            free(privkeys[i]);
        free(privkeys);
        free(cipherids);
    }
}

int32_t AES_codec(uint8_t *buf,int32_t decryptflag,char *msg,char *AESpasswordstr)
{
    int32_t *cipherids,len;
    char **privkeys;//,*decompressed;
    uint8_t *retdata = 0;
    privkeys = gen_privkeys(&cipherids,"AES",AESpasswordstr,GENESIS_SECRET,"");
    if ( decryptflag == 0 )
    {
        len = (int32_t)strlen(msg) + 1;
        retdata = ciphers_codec(0,privkeys,cipherids,(uint8_t *)msg,&len);
        memcpy(buf,retdata,len);
        if ( 0 )
        {
            uint8_t *ret;
            int32_t retlen = len;
            ret = ciphers_codec(1,privkeys,cipherids,(uint8_t *)retdata,&retlen);
            if ( ret != 0 )
            {
                if ( strlen((char *)ret) != strlen(msg) || strcmp((char *)ret,msg) != 0 )
                    printf("ciper.(%s) error len %ld != %ld || (%s) != (%s)\n",AESpasswordstr,strlen((char *)ret),(strlen(msg)+1),ret,msg);
                else printf("(%s) matches (%s)\n",(char *)ret,msg);
                free(ret);
            }
        }
    }
    else
    {
        len = decryptflag;
        retdata = ciphers_codec(1,privkeys,cipherids,buf,&len);
        //printf("cipher decrypted.(%s)\n",retdata);
        memcpy(msg,retdata,len);
    }
    if ( retdata != 0 )
        free(retdata);
    free_privkeys(privkeys,cipherids);
    return(len);
}

int32_t verify_AES_codec(uint8_t *encoded,int32_t encodedlen,char *msg,char *AESpasswordstr)
{
    int32_t decodedlen;
    char decoded[4096];
    decodedlen = AES_codec(encoded,encodedlen,decoded,AESpasswordstr);
    if ( decodedlen > 0 )
    {
        if ( strcmp(msg,decoded) == 0 )
        {
            printf("decrypted.(%s) len.%d\n",decoded,decodedlen);
        }
        else { printf("AES_codec error on msg.(%s) != (%s)\n",msg,decoded); decodedlen = -1; }
    } else printf("AES_codec unexpected decode error.%d\n",decodedlen);
    return(decodedlen);
}

uint64_t calc_AESkeys(bits256 *AESpassword,char *AESpasswordstr,uint8_t *shared,uint64_t nxt64bits,int32_t sequenceid)
{
    char buf[128];
    bits256 secret,pubkey,tmp;
    if ( AESpassword == 0 )
        AESpassword = &tmp;
    sprintf(buf,"%llu.%d",(long long)nxt64bits,sequenceid);
    calc_sha256cat(AESpassword->bytes,(uint8_t *)buf,(int32_t)strlen(buf),shared,(int32_t)sizeof(bits256));
    init_hexbytes(AESpasswordstr,AESpassword->bytes,sizeof(bits256));
    return(conv_NXTpassword(secret.bytes,pubkey.bytes,AESpasswordstr));
}

#define calc_sendAESkeys(AESpassword,AESpasswordstr,contact,sequence) calc_privatelocation(AESpassword,AESpasswordstr,1,contact,sequence)
#define calc_recvAESkeys(AESpassword,AESpasswordstr,contact,sequence) calc_privatelocation(AESpassword,AESpasswordstr,-1,contact,sequence)
uint64_t calc_privatelocation(bits256 *AESpassword,char *AESpasswordstr,int32_t dir,struct contact_info *contact,int32_t sequenceid)
{
    static bits256 zerokey;
    uint64_t nxt64bits;
    struct coin_info *cp = get_coin_info("BTCD");
    if ( cp == 0 || memcmp(&zerokey,&contact->shared,sizeof(zerokey)) == 0 || contact->nxt64bits == 0 )
    {
        printf("ERROR: illegal calc_privatelocation dir.%d for %llu.%d no shared secret\n",dir,(long long)contact->nxt64bits,sequenceid);
        return(0);
    }
    if ( dir > 0 ) // transmission
        nxt64bits = cp->privatebits;
    else nxt64bits = contact->nxt64bits;
    return(calc_AESkeys(AESpassword,AESpasswordstr,contact->shared.bytes,nxt64bits,sequenceid));
}

void create_telepathy_entry(struct contact_info *contact,int32_t sequenceid)
{
    uint64_t location;
    bits256 AESpassword;
    char AESpasswordstr[512],locationstr[64];
    if ( contact->lastentry != 0 && sequenceid <= contact->lastentry )
    {
        printf("lastentry.%d vs seqid.%d\n",contact->lastentry,sequenceid);
        return;
    }
    if ( (location= calc_recvAESkeys(&AESpassword,AESpasswordstr,contact,sequenceid)) != 0 )
    {
        expand_nxt64bits(locationstr,location);
        if ( find_telepathy_entry(locationstr) == 0 )
            add_telepathy_entry(locationstr,contact,AESpassword,sequenceid);
    }
}

uint64_t calc_privatedatastr(bits256 *AESpassword,char *AESpasswordstr,char *privatedatastr,struct contact_info *contact,int32_t sequence,char *msg)
{
    uint64_t location,retval = 0;
    int32_t encodedlen;
    uint8_t encoded[4096];
    if ( (location= calc_sendAESkeys(AESpassword,AESpasswordstr,contact,sequence)) != 0 )
    {
        encodedlen = AES_codec(encoded,0,msg,AESpasswordstr);
        if ( encodedlen > 0 )
        {
            init_hexbytes(privatedatastr,encoded,encodedlen);
            if ( verify_AES_codec(encoded,encodedlen,msg,AESpasswordstr) > 0 )
                retval = location;
        }
    }
    return(retval);
}

cJSON *parse_encrypted_data(int32_t updatedb,int32_t *sequenceidp,struct contact_info *contact,char *key,uint8_t *data,int32_t datalen,char *AESpasswordstr)
{
    cJSON *json = 0;
    uint64_t deaddrop;
    int32_t i,decodedlen,hint,retransmit;
    char deaddropstr[MAX_JSON_FIELD],decoded[4096],privatedatastr[8192];
    *sequenceidp = -1;
    decodedlen = AES_codec(data,datalen,decoded,AESpasswordstr);
    if ( decodedlen > 0 )
    {
        if ( (json= cJSON_Parse(decoded)) != 0 )
        {
            if ( updatedb != 0 )
            {
                init_hexbytes(privatedatastr,data,datalen);
                add_storage(PRIVATE_DATA,key,privatedatastr,0,0);
                printf("saved parsed decrypted.(%s)\n",decoded);
            }
            hint = get_API_int(cJSON_GetObjectItem(json,"hint"),-1);
            if ( hint > 0 )
            {
                for (i=0; i<MAX_DROPPED_PACKETS; i++)
                    create_telepathy_entry(contact,hint+i);
            }
            retransmit = get_API_int(cJSON_GetObjectItem(json,"retransmit"),-1);
            if ( retransmit > 0 )
            {
                // republish this sequenceid
            }
            *sequenceidp = get_API_int(cJSON_GetObjectItem(json,"id"),-1);
            copy_cJSON(deaddropstr,cJSON_GetObjectItem(json,"deaddrop"));
            if ( deaddropstr[0] != 0 )
            {
                deaddrop = calc_nxt64bits(deaddropstr);
                if ( contact->deaddrop != deaddrop )
                {
                    printf("DECRYPTED: handle.(%s) deaddrop.%llu <- %llu\n",contact->handle,(long long)contact->deaddrop,(long long)deaddrop);
                    contact->deaddrop = deaddrop;
                }
            }
        } else printf("couldnt parse decrypted.(%s) len.%d %d\n",decoded,decodedlen,datalen);
    } else printf("AES_codec failure.%d\n",decodedlen);
    return(json);
}

char *check_privategenesis(struct contact_info *contact)
{
    cJSON *json;
    uint64_t location;
    int32_t sequenceid = 0;
    char AESpasswordstr[512],key[64];
    struct kademlia_storage *sp;
    if ( (location= calc_recvAESkeys(0,AESpasswordstr,contact,sequenceid)) != 0 )
    {
        expand_nxt64bits(key,location);
        sp = kademlia_getstored(PUBLIC_DATA,location,0);
        if ( sp != 0 && sp->data != 0 ) // no need to query if we already have it
        {
            if ( (json= parse_encrypted_data(0,&sequenceid,contact,key,sp->data,sp->datalen,AESpasswordstr)) != 0 )
                free_json(json);
        }
        else
        {
            printf("need to get %s deaddrop from %s\n",contact->handle,key);
            return(kademlia_find("findvalue",0,key,AESpasswordstr,key,key,0,0));
        }
    }
    return(0);
}

char *private_publish(struct contact_info *contact,int32_t sequenceid,char *msg)
{
    char privatedatastr[8192],AESpasswordstr[512],seqacct[64],key[64],*retstr = 0;
    uint64_t location;
    if ( 0 && contact->deaddrop == 0 )
    {
        if ( (retstr= check_privategenesis(contact)) != 0 )
            free(retstr);
    }
    if ( (location= calc_privatedatastr(0,AESpasswordstr,privatedatastr,contact,sequenceid,msg)) != 0 )
    {
        expand_nxt64bits(seqacct,location);
        if ( location != issue_getAccountId(0,AESpasswordstr) )
            printf("ERROR: private_publish location %llu != %llu from (%s)\n",(long long)location,(long long)issue_getAccountId(0,AESpasswordstr),AESpasswordstr);
        if ( sequenceid == 0 )
        {
            expand_nxt64bits(key,location);
            printf("store.(%s) len.%ld -> %llu %llu\n",privatedatastr,strlen(privatedatastr)/2,(long long)seqacct,(long long)location);
            retstr = kademlia_storedata(0,seqacct,AESpasswordstr,seqacct,key,privatedatastr);
            add_storage(PRIVATE_DATA,key,privatedatastr,0,0);
            add_storage(PUBLIC_DATA,key,privatedatastr,0,0);
        }
        else
        {
            add_storage(PRIVATE_DATA,key,privatedatastr,0,0);
            if ( contact->deaddrop != 0 )
            {
                contact->numsent++;
                contact->lastsent = sequenceid;
                //printf("telepathic send to %s.%d via %llu using %llu (%s)\n",contact->handle,sequenceid,(long long)contact->deaddrop,(long long)location,AESpasswordstr);
                expand_nxt64bits(key,contact->deaddrop);
                retstr = kademlia_find("findnode",0,seqacct,AESpasswordstr,seqacct,key,privatedatastr,0); // find and you shall telepath
            } else retstr = clonestr("{\"error\":\"no deaddrop address\"}");
        }
    }
    return(retstr);
}

void process_telepathic(char *key,uint8_t *data,int32_t datalen,uint64_t senderbits,char *senderip)
{
    struct coin_info *cp = get_coin_info("BTCD");
    uint64_t keybits = calc_nxt64bits(key);
    struct contact_info *contact;
    struct telepathy_entry *tel;
    int32_t sequenceid,i,n;
    char AESpasswordstr[512],locationstr[64],*jsonstr;
    cJSON *json;
    expand_nxt64bits(locationstr,senderbits); // overloading sender with locationbits!
    if ( (tel= find_telepathy_entry(locationstr)) != 0 )
    {
        portable_mutex_lock(&Contacts_mutex);
        contact = _find_contact_nxt64bits(tel->contactbits);
        portable_mutex_unlock(&Contacts_mutex);
        if ( contact != 0 )
        {
            init_hexbytes_noT(AESpasswordstr,tel->AESpassword.bytes,sizeof(tel->AESpassword));
            //printf("try AESpassword.(%s)\n",AESpasswordstr);
            if ( (json= parse_encrypted_data(1,&sequenceid,contact,key,data,datalen,AESpasswordstr)) != 0 )
            {
                if ( sequenceid == tel->sequenceid )
                {
                    jsonstr = cJSON_Print(json);
                    stripwhite_ns(jsonstr,strlen(jsonstr));
                    printf("DECRYPTED expected (%s.%d) (%s) lastrecv.%d lastentry.%d\n",contact->handle,tel->sequenceid,jsonstr,contact->lastrecv,contact->lastentry);
                    contact->lastrecv = tel->sequenceid;
                    contact->numrecv++;
                    if ( contact->lastentry < (tel->sequenceid + MAX_DROPPED_PACKETS) )
                    {
                        n = (contact->lastentry + MAX_DROPPED_PACKETS);
                        for (i=contact->lastentry; i<n; i++)
                            create_telepathy_entry(contact,i);
                    }
                    free(jsonstr);
                } else printf("sequenceid mismatch %d != %d\n",sequenceid,tel->sequenceid);
                free_json(json);
            }
        } else printf("dont have contact info for %llu\n",(long long)tel->contactbits);
        printf("(%s.%d) pass.(%s) | ",contact->handle,tel->sequenceid,AESpasswordstr);
    }
    {
        char datastr[4096];
        init_hexbytes(datastr,data,datalen);
        printf("process_telepathic: key.(%s) got.(%s) len.%d from %llu dist %2d vs mydist srv %d priv %d | %s\n",key,datastr,datalen,(long long)senderbits,bitweight(keybits ^ senderbits),bitweight(keybits ^ cp->srvpubnxtbits),bitweight(keybits ^ cp->privatebits),senderip);
    }
}

void publish_deaddrop(struct contact_info *contact)
{
    struct coin_info *cp = get_coin_info("BTCD");
    char deaddropjsonstr[512],sharedstr[512],*retstr;
    sprintf(deaddropjsonstr,"{\"deaddrop\":\"%llu\",\"id\":%d}",(long long)contact->mydrop,0);
    retstr = private_publish(contact,0,deaddropjsonstr);
    init_hexbytes(sharedstr,contact->shared.bytes,sizeof(contact->shared));
    printf("shared.(%s) ret.(%s) %llu %llx vs %llx dist.%d\n",sharedstr,retstr,(long long)contact->mydrop,(long long)contact->mydrop,(long long)cp->srvpubnxtbits,bitweight(contact->mydrop ^ cp->srvpubnxtbits));
    if ( retstr != 0 )
        free(retstr);
}

void init_telepathy_contact(struct contact_info *contact)
{
    struct coin_info *cp = get_coin_info("BTCD");
    int32_t i;
    char *retstr;
    uint64_t randbits;
    for (i=0; i<=MAX_DROPPED_PACKETS; i++)
        create_telepathy_entry(contact,i);
    if ( contact->mydrop == 0 )
    {
        randbits = cp->srvpubnxtbits;
        while ( bitweight(randbits ^ cp->srvpubnxtbits) < KADEMLIA_MAXTHRESHOLD)
            randbits ^= (1L << ((rand()>>8) & 63));
        contact->mydrop = randbits;
    }
    publish_deaddrop(contact);
    if ( (retstr= check_privategenesis(contact)) != 0 )
        free(retstr);
}

uint64_t conv_acctstr(char *acctstr)
{
    uint64_t nxt64bits = 0;
    int32_t len;
    if ( (len= is_decimalstr(acctstr)) > 0 && len < 22 )
        nxt64bits = calc_nxt64bits(acctstr);
    else if ( strncmp("NXT-",acctstr,4) == 0 )
        nxt64bits = conv_rsacctstr(acctstr,0);
    return(nxt64bits);
}

struct contact_info *_find_contact(char *contactstr)
{
    uint64_t nxt64bits = 0;
    struct contact_info *contact = 0;
    //printf("_find_contact.(%s)\n",contactstr);
    if ( (contact= _find_handle(contactstr)) == 0 )
    {
        if ( (nxt64bits= conv_acctstr(contactstr)) != 0 )
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

char *getdb(struct sockaddr *prevaddr,char *NXTaddr,char *NXTACCTSECRET,char *sender,int32_t dir,char *contactstr,int32_t sequenceid,char *keystr)
{
    char retbuf[4096],hexstr[4096],AESpasswordstr[512],locationstr[64],*jsonstr;
    cJSON *json;
    int32_t seqid;
    uint64_t location;
    bits256 AESpassword;
    struct kademlia_storage *sp = 0;
    struct contact_info *contact;
    retbuf[0] = 0;
    if ( contactstr[0] == 0 )
    {
        if ( keystr[0] != 0 )
        {
            if ( (sp= find_storage(PUBLIC_DATA,keystr)) != 0 )
            {
                if ( sp->datalen < sizeof(hexstr)/2 )
                {
                    init_hexbytes(hexstr,sp->data,sp->datalen);
                    sprintf(retbuf,"{\"data\":\"%s\"}",hexstr);
                } else strcpy(retbuf,"{\"error\":\"cant find key\"}");
            } else strcpy(retbuf,"{\"error\":\"cant find key\"}");
        } else strcpy(retbuf,"{\"error\":\"no contact and no key\"}");
    }
    else
    {
        if ( (contact= find_contact(contactstr)) != 0 )
        {
            if ( dir > 0 )
                location = calc_sendAESkeys(&AESpassword,AESpasswordstr,contact,sequenceid);
            else location = calc_recvAESkeys(&AESpassword,AESpasswordstr,contact,sequenceid);
            if ( location != 0 )
            {
                expand_nxt64bits(locationstr,location);
                if ( (sp= find_storage(PRIVATE_DATA,locationstr)) != 0 )
                {
                    if ( (json= parse_encrypted_data(0,&seqid,contact,locationstr,sp->data,sp->datalen,AESpasswordstr)) != 0 )
                    {
                        jsonstr = cJSON_Print(json);
                        stripwhite_ns(jsonstr,strlen(jsonstr));
                        free_json(json);
                        return(jsonstr);
                    } else strcpy(retbuf,"{\"error\":\"couldnt decrypt data\"}");
                } else strcpy(retbuf,"{\"error\":\"cant find key\"}");
            } else strcpy(retbuf,"{\"error\":\"cant get location\"}");
        } else strcpy(retbuf,"{\"error\":\"cant find contact\"}");
    }
    return(clonestr(retbuf));
}

char *addcontact(char *handle,char *acct)
{
    static bits256 zerokey;
    uint64_t nxt64bits;
    bits256 mysecret,mypublic;
    struct coin_info *cp = get_coin_info("BTCD");
    struct contact_info *contact;
    char retstr[1024],pubkeystr[128],*ret;
    if ( cp == 0 )
    {
        printf("addcontact: no BTCD cp?\n");
        return(0);
    }
    handle[sizeof(contact->handle)-1] = 0;
    
    nxt64bits = conv_acctstr(acct);
    portable_mutex_lock(&Contacts_mutex);
    {
        contact = _find_contact_nxt64bits(nxt64bits);
        if ( contact != 0 && strcmp(contact->handle,handle) != 0 )
        {
            sprintf(retstr,"{\"error\":\"(%s) already has %llu\"}",contact->handle,(long long)nxt64bits);
            portable_mutex_unlock(&Contacts_mutex);
            if ( Debuglevel > 1 )
                printf("addcontact: (%s)\n",retstr);
            return(clonestr(retstr));
        }
        if ( (contact= _find_contact(handle)) == 0 )
        {
            if ( Num_contacts >= Max_contacts )
            {
                Max_contacts = (Num_contacts + 1);
                Contacts = realloc(Contacts,(sizeof(*Contacts) * Max_contacts));
            }
            if ( Debuglevel > 1 )
                printf("Num_contacts.%d Max.%d\n",Num_contacts,Max_contacts);
            contact = &Contacts[Num_contacts++];
            memset(contact,0,sizeof(*contact));
            safecopy(contact->handle,handle,sizeof(contact->handle));
        }
        else if ( strcmp(handle,"myhandle") == 0 )
            return(clonestr("{\"error\":\"cant override myhandle\"}"));
    }
    portable_mutex_unlock(&Contacts_mutex);
    
    if ( Debuglevel > 0 )
        printf("%p ADDCONTACT.(%s) lastcontact.%d acct.(%s) -> %llu\n",contact,contact->handle,contact->lastentry,acct,(long long)nxt64bits);
    if ( nxt64bits != contact->nxt64bits || memcmp(&zerokey,&contact->pubkey,sizeof(zerokey)) == 0 )
    {
        contact->nxt64bits = nxt64bits;
        contact->pubkey = issue_getpubkey(acct);
        init_hexbytes(pubkeystr,contact->pubkey.bytes,sizeof(contact->pubkey));
        if ( contact->pubkey.txid == 0 )
            sprintf(retstr,"{\"error\":\"(%s) acct.(%s) has no pubkey.(%s)\"}",handle,acct,pubkeystr);
        else
        {
            //if ( strcmp(contact->handle,"myhandle") != 0 )
            {
                conv_NXTpassword(mysecret.bytes,mypublic.bytes,cp->privateNXTACCTSECRET);
                contact->shared = curve25519(mysecret,contact->pubkey);
                fprintf(stderr,"init_telepathy_contact\n");
                init_telepathy_contact(contact);
                sprintf(retstr,"{\"result\":\"(%s) acct.(%s) (%llu) has pubkey.(%s)\"}",handle,acct,(long long)contact->nxt64bits,pubkeystr);
            }
        }
    }
    else
    {
        fprintf(stderr,"publish deaddrop\n");
        publish_deaddrop(contact);
        if ( (ret= check_privategenesis(contact)) != 0 )
            free(ret);
        sprintf(retstr,"{\"result\":\"(%s) acct.(%s) (%llu) unchanged\"}",handle,acct,(long long)contact->nxt64bits);
    }
    fprintf(stderr,"ADDCONTACT.(%s)\n",retstr);
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
    printf("REMOVECONTACT.(%s)\n",retstr);
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
    sprintf(contactstr,"{\"handle\":\"%s\",\"acct\":\"%s\",\"NXT\":\"%llu\",\"pubkey\":\"%s\"}",contact->handle,rsacctstr,(long long)contact->nxt64bits,pubkeystr);
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

char *telepathy_func(char *NXTaddr,char *NXTACCTSECRET,struct sockaddr *prevaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    //struct coin_info *cp = get_coin_info("BTCD");
    char desthandle[MAX_JSON_FIELD],msg[MAX_JSON_FIELD],jsonstr[MAX_JSON_FIELD],*retstr = 0;
    //struct telepathy_args args;
    int32_t sequenceid;
    struct contact_info *contact;
    if ( prevaddr != 0 )//|| cp == 0 )
        return(0);
    copy_cJSON(desthandle,objs[0]);
    contact = find_contact(desthandle);
    copy_cJSON(msg,objs[1]);
    sequenceid = get_API_int(objs[2],-1);
    if ( contact != 0 && desthandle[0] != 0 && msg[0] != 0 && sender[0] != 0 && valid > 0 )
    {
        if ( sequenceid < 0 )
            sequenceid = contact->lastsent+1;
        printf("telepathy.(%s -> %s).%d\n",msg,contact->handle,sequenceid);
        sprintf(jsonstr,"{\"deaddrop\":\"%llu\",\"id\":%d,\"msg\":\"%s\"}",(long long)contact->mydrop,sequenceid,msg);
        retstr = private_publish(contact,sequenceid,jsonstr);
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
