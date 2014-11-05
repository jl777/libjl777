//
//  contacts.h
//  libjl777
//
//  Created by jl777 on 10/15/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//


#ifndef contacts_h
#define contacts_h

struct telepathy_entry
{
    uint64_t modified,location,contactbits;
    bits256 AESpassword;
    char locationstr[MAX_NXTADDR_LEN];
    int32_t sequenceid;
};

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
    struct SuperNET_storage *sp;
    tel = MTadd_hashtable(&createdflag,Global_mp->Telepathy_tablep,locationstr);
    if ( createdflag != 0 )
    {
        tel->location = calc_nxt64bits(locationstr);
        tel->contactbits = contact->nxt64bits;
        tel->AESpassword = AESpassword;
        tel->sequenceid = sequenceid;
        if ( sequenceid > contact->lastentry )
            contact->lastentry = sequenceid;
        if ( (sp= (struct SuperNET_storage *)find_storage(PRIVATE_DATA,locationstr,0)) != 0 )
        {
            if ( sequenceid > contact->lastrecv )
                contact->lastrecv = sequenceid;
            contact->numrecv++;
            free(sp);
        }
        printf("add (%s.%d) %llu\n",contact->handle,sequenceid,(long long)tel->location);
    } else printf("add_telepathy_entry warning: already created %s.%s\n",contact->handle,locationstr);
    return(tel);
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
    init_hexbytes_noT(AESpasswordstr,AESpassword->bytes,sizeof(bits256));
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
        if ( sequenceid < 0 )
            exit(-1);
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
            init_hexbytes_noT(privatedatastr,encoded,encodedlen);
            if ( verify_AES_codec(encoded,encodedlen,msg,AESpasswordstr) > 0 )
                retval = location;
        }
    } else fprintf(stderr,"calc_privatedatastr error calculating location for %s.%d\n",contact->handle,sequence);
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
                init_hexbytes_noT(privatedatastr,data,datalen);
                add_storage(PRIVATE_DATA,key,privatedatastr);
                printf("saved parsed decrypted.(%s)\n",decoded);
            }
            hint = get_API_int(cJSON_GetObjectItem(json,"hint"),-1);
            if ( hint > 0 )
            {
                printf("hint.%d\n",hint);
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
    struct SuperNET_storage *sp;
    if ( (location= calc_recvAESkeys(0,AESpasswordstr,contact,sequenceid)) != 0 )
    {
        expand_nxt64bits(key,location);
        sp = kademlia_getstored(PUBLIC_DATA,location,0);
        if ( sp != 0 ) // no need to query if we already have it
        {
            if ( sp->data != 0 && (json= parse_encrypted_data(0,&sequenceid,contact,key,sp->data,sp->H.size-sizeof(*sp),AESpasswordstr)) != 0 )
                free_json(json);
            free(sp);
        }
        else
        {
            printf("need to get %s deaddrop from %s\n",contact->handle,key);
            return(kademlia_find("findvalue",0,key,AESpasswordstr,key,key,0,0));
        }
    }
    return(0);
}

char *private_publish(uint64_t *locationp,struct contact_info *contact,int32_t sequenceid,char *msg)
{
    char privatedatastr[8192],AESpasswordstr[512],seqacct[64],key[64],*retstr = 0;
    uint64_t location;
    if ( 0 && contact->deaddrop == 0 )
    {
        if ( (retstr= check_privategenesis(contact)) != 0 )
            free(retstr);
    }
    if ( locationp != 0 )
        *locationp = 0;
    printf("private_publish(%s) -> %s.%d\n",msg,contact->handle,sequenceid);
    if ( (location= calc_privatedatastr(0,AESpasswordstr,privatedatastr,contact,sequenceid,msg)) != 0 )
    {
        printf("location.%llu\n",(long long)location);
        if ( locationp != 0 )
            *locationp = location;
        expand_nxt64bits(seqacct,location);
        if ( location != issue_getAccountId(0,AESpasswordstr) )
            printf("ERROR: private_publish location %llu != %llu from (%s)\n",(long long)location,(long long)issue_getAccountId(0,AESpasswordstr),AESpasswordstr);
        expand_nxt64bits(key,location);
        if ( sequenceid == 0 )
        {
            printf("store.(%s) len.%ld -> %llu %llu\n",privatedatastr,strlen(privatedatastr)/2,(long long)seqacct,(long long)location);
            retstr = kademlia_storedata(0,seqacct,AESpasswordstr,seqacct,key,privatedatastr);
            if ( IS_LIBTEST != 0 )
            {
                add_storage(PRIVATE_DATA,key,privatedatastr);
                add_storage(PUBLIC_DATA,key,privatedatastr);
            }
        }
        else
        {
            printf("telepathic.(%s) len.%ld -> %llu %llu\n",privatedatastr,strlen(privatedatastr)/2,(long long)seqacct,(long long)location);
            if ( IS_LIBTEST != 0 )
                add_storage(PRIVATE_DATA,key,privatedatastr);
            if ( contact->deaddrop != 0 )
            {
                contact->numsent++;
                contact->lastsent = sequenceid;
                printf("telepathic send to %s.%d via %llu using %llu (%s)\n",contact->handle,sequenceid,(long long)contact->deaddrop,(long long)location,AESpasswordstr);
                expand_nxt64bits(key,contact->deaddrop);
                retstr = kademlia_find("findnode",0,seqacct,AESpasswordstr,seqacct,key,privatedatastr,0); // find and you shall telepath
            } else retstr = clonestr("{\"error\":\"no deaddrop address\"}");
        }
        update_contact_info(contact);
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
    char AESpasswordstr[512],typestr[1024],locationstr[64],*jsonstr;
    cJSON *json,*attachjson;
    expand_nxt64bits(locationstr,senderbits); // overloading sender with locationbits!
    if ( (tel= find_telepathy_entry(locationstr)) != 0 )
    {
        contact = find_contact_nxt64bits(tel->contactbits);
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
                    copy_cJSON(typestr,cJSON_GetObjectItem(json,"type"));
                    if ( strcmp(typestr,"teleport") == 0 && (attachjson=cJSON_GetObjectItem(json,"attach")) != 0 )
                    {
                        void telepathic_teleport(struct contact_info *contact,cJSON *attachjson);
                        telepathic_teleport(contact,attachjson);
                    }
                    free(jsonstr);
                    update_contact_info(contact);
                } else printf("sequenceid mismatch %d != %d\n",sequenceid,tel->sequenceid);
                free_json(json);
            }
            free(contact);
        } else printf("dont have contact info for %llu\n",(long long)tel->contactbits);
        printf("(%s.%d) pass.(%s) | ",contact->handle,tel->sequenceid,AESpasswordstr);
    }
    if ( cp != 0 )
    {
        char datastr[4096];
        init_hexbytes_noT(datastr,data,datalen);
        printf("process_telepathic: key.(%s) got.(%s) len.%d from %llu dist %2d vs mydist srv %d priv %d | %s\n",key,datastr,datalen,(long long)senderbits,bitweight(keybits ^ senderbits),bitweight(keybits ^ cp->srvpubnxtbits),bitweight(keybits ^ cp->privatebits),senderip);
    }
}

cJSON *telepathic_transmit(char retbuf[MAX_JSON_FIELD],struct contact_info *contact,int32_t sequenceid,char *type,cJSON *attachmentjson)
{
    char numstr[64],*retstr,*jsonstr,*str,*str2;
    uint64_t location;
    cJSON *json = cJSON_CreateObject();
    if ( sequenceid < 0 )
        sequenceid = contact->lastsent+1;
    sprintf(numstr,"%llu",(long long)contact->mydrop);
    cJSON_AddItemToObject(json,"deaddrop",cJSON_CreateString(numstr));
    cJSON_AddItemToObject(json,"id",cJSON_CreateNumber(sequenceid));
    cJSON_AddItemToObject(json,"time",cJSON_CreateNumber(time(NULL)));
    if ( type != 0 && type[0] != 0 )
    {
        cJSON_AddItemToObject(json,"type",cJSON_CreateString(type));
        if ( attachmentjson != 0 )
        {
            str = cJSON_Print(attachmentjson);
            if ( str != 0 )
            {
                stripwhite_ns(str,strlen(str));
                str2 = stringifyM(str);
                cJSON_AddItemToObject(json,"attach",cJSON_CreateString(str));
                free(str);
                free(str2);
            }
        }
    }
    if ( (jsonstr= cJSON_Print(json)) != 0 )
    {
        stripwhite_ns(jsonstr,strlen(jsonstr));
        retstr = private_publish(&location,contact,sequenceid,jsonstr);
        if ( retstr != 0 )
        {
            strcpy(retbuf,retstr);
            printf("telepathy.(%s) -> (%s).%d @ %llu\n",jsonstr,contact->handle,sequenceid,(long long)location);
            free(retstr);
        } else strcpy(retbuf,"{\"error\":\"no result from private_publish\"}");
        free(jsonstr);
    } else strcpy(retbuf,"{\"error\":\"no result from cJSON_Print\"}");
    attachmentjson = cJSON_DetachItemFromObject(json,"attach");
    free_json(json);
    return(attachmentjson);
}

void init_telepathy_contact(struct contact_info *contact)
{
    struct coin_info *cp = get_coin_info("BTCD");
    int32_t i;
    char retbuf[MAX_JSON_FIELD],*retstr;
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
    telepathic_transmit(retbuf,contact,0,0,0);
    if ( (retstr= check_privategenesis(contact)) != 0 )
        free(retstr);
}

char *getdb(char *previpaddr,char *NXTaddr,char *NXTACCTSECRET,char *sender,int32_t dir,char *contactstr,int32_t sequenceid,char *keystr,char *destip)
{
    char retbuf[4096],hexstr[4096],AESpasswordstr[512],locationstr[64],*jsonstr;
    cJSON *json;
    int32_t seqid;
    uint64_t location;
    bits256 AESpassword;
    struct SuperNET_storage *sp = 0;
    struct contact_info *contact;
    retbuf[0] = 0;
    if ( contactstr[0] == 0 )
    {
        if ( keystr[0] != 0 )
        {
            if ( (sp= (struct SuperNET_storage *)find_storage(PUBLIC_DATA,keystr,0)) != 0 )
            {
                if ( (sp->H.size-sizeof(*sp)) < sizeof(hexstr)/2 )
                {
                    init_hexbytes_noT(hexstr,sp->data,sp->H.size-sizeof(*sp));
                    sprintf(retbuf,"{\"requestType\":\"dbret\",\"NXT\":\"%s\",\"key\":\"%s\",\"data\":\"%s\"}",NXTaddr,keystr,hexstr);
                } else strcpy(retbuf,"{\"requestType\":\"dbret\",\"error\":\"cant find key\"}");
                free(sp);
            } else strcpy(retbuf,"{\"requestType\":\"dbret\",\"error\":\"cant find key\"}");
            if ( is_remote_access(previpaddr) != 0 )
                send_to_ipaddr(previpaddr,retbuf,NXTACCTSECRET);
            else
            {
                sprintf(retbuf,"{\"requestType\":\"getdb\",\"NXT\":\"%s\",\"key\":\"%s\"}",NXTaddr,keystr);
                send_to_ipaddr(destip,retbuf,NXTACCTSECRET);
            }
        } else strcpy(retbuf,"{\"requestType\":\"dbret\",\"error\":\"no contact and no key\"}");
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
                if ( (sp= (struct SuperNET_storage *)find_storage(PRIVATE_DATA,locationstr,0)) != 0 )
                {
                    if ( (json= parse_encrypted_data(0,&seqid,contact,locationstr,sp->data,sp->H.size-sizeof(*sp),AESpasswordstr)) != 0 )
                    {
                        jsonstr = cJSON_Print(json);
                        stripwhite_ns(jsonstr,strlen(jsonstr));
                        free_json(json);
                        return(jsonstr);
                    } else strcpy(retbuf,"{\"requestType\":\"dbret\",\"error\":\"couldnt decrypt data\"}");
                    free(sp);
                } else strcpy(retbuf,"{\"requestType\":\"dbret\",\"error\":\"cant find key\"}");
            } else strcpy(retbuf,"{\"requestType\":\"dbret\",\"error\":\"cant get location\"}");
            free(contact);
        } else strcpy(retbuf,"{\"requestType\":\"dbret\",\"error\":\"cant find contact\"}");
    }
    printf("GETDB.(%s)\n",retbuf);
    return(clonestr(retbuf));
}

char *addcontact(char *handle,char *acct)
{
    static bits256 zerokey;
    struct contact_info C;
    uint64_t nxt64bits;
    bits256 mysecret,mypublic;
    struct coin_info *cp = get_coin_info("BTCD");
    struct contact_info *contact;
    char retstr[1024],pubkeystr[128],RSaddr[64],*ret;
    if ( cp == 0 )
    {
        printf("addcontact: no BTCD cp?\n");
        return(0);
    }
    nxt64bits = conv_acctstr(acct);
    contact = find_contact_nxt64bits(nxt64bits);
    if ( contact != 0 && strcmp(contact->handle,handle) != 0 )
    {
        sprintf(retstr,"{\"error\":\"(%s) already has %llu\"}",contact->handle,(long long)nxt64bits);
        if ( Debuglevel > 1 )
            printf("addcontact: existing (%s)\n",retstr);
        return(clonestr(retstr));
    }
    if ( Debuglevel > 1 )
        printf("addcontact: new (%s)\n",retstr);
    if ( (contact= find_contact(handle)) == 0 )
    {
        memset(&C,0,sizeof(C));
        safecopy(C.handle,handle,sizeof(C.handle));
        update_contact_info(&C);
        if ( (contact= find_handle(handle)) == 0 )
            return(clonestr("{\"error\":\"cant find just created handle?\"}"));
        fprintf(stderr,"created new contact seqid.%d\n",contact->lastentry);
    }
    else if ( strcmp(handle,"myhandle") == 0 )
        return(clonestr("{\"error\":\"cant override myhandle\"}"));
    if ( Debuglevel > 0 )
        printf("%p ADDCONTACT.(%s) lastcontact.%d acct.(%s) -> %llu\n",contact,contact->handle,contact->lastentry,acct,(long long)nxt64bits);
    if ( nxt64bits != contact->nxt64bits || memcmp(&zerokey,&contact->pubkey,sizeof(zerokey)) == 0 )
    {
        contact->nxt64bits = nxt64bits;
        contact->pubkey = issue_getpubkey(acct);
        init_hexbytes_noT(pubkeystr,contact->pubkey.bytes,sizeof(contact->pubkey));
        if ( contact->pubkey.txid == 0 )
        {
            conv_rsacctstr(RSaddr,nxt64bits);
            sprintf(retstr,"{\"error\":\"(%s) acct.(%s) has no pubkey.(%s)\",\"RS\":\"%s\"}",handle,acct,pubkeystr,RSaddr);
        }
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
        update_contact_info(contact);
    }
    else
    {
        fprintf(stderr,"publish deaddrop\n");
        telepathic_transmit(retstr,contact,0,0,0);
        if ( (ret= check_privategenesis(contact)) != 0 )
            free(ret);
        sprintf(retstr,"{\"result\":\"(%s) acct.(%s) (%llu) unchanged\"}",handle,acct,(long long)contact->nxt64bits);
    }
    fprintf(stderr,"ADDCONTACT.(%s)\n",retstr);
    free(contact);
    return(clonestr(retstr));
}

char *telepathy_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char retbuf[MAX_JSON_FIELD],contactstr[MAX_JSON_FIELD],typestr[MAX_JSON_FIELD],attachmentstr[MAX_JSON_FIELD],*retstr = 0;
    int32_t sequenceid;
    cJSON *attachmentjson;
    struct contact_info *contact;
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    copy_cJSON(contactstr,objs[0]);
    contact = find_contact(contactstr);
    sequenceid = get_API_int(objs[1],-1);
    copy_cJSON(typestr,objs[2]);
    copy_cJSON(attachmentstr,objs[3]);
    if ( contact != 0 && contactstr[0] != 0 && sender[0] != 0 && valid > 0 )
    {
        attachmentjson = cJSON_Parse(attachmentstr);
        attachmentjson = telepathic_transmit(retbuf,contact,sequenceid,typestr,attachmentjson);
        if ( attachmentjson != 0 )
            free_json(attachmentjson);
        retstr = clonestr(retbuf);
    }
    else retstr = clonestr("{\"error\":\"invalid telepathy_func arguments\"}");
    return(retstr);
}

#endif
