//
//  contacts.h
//  libjl777
//
//  Created by jl777 on 10/15/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//


#ifndef contacts_h
#define contacts_h

struct contact_info
{
    char handle[64];
    uint64_t nxt64bits;
    bits256 pubkey,shared;
} *Contacts;
int32_t Num_contacts,Max_contacts;
portable_mutex_t Contacts_mutex;

struct contact_info *_find_contact(char *handle)
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

struct contact_info *find_contact(char *handle)
{
    struct contact_info *contact;
    portable_mutex_lock(&Contacts_mutex);
    contact = _find_contact(handle);
    portable_mutex_unlock(&Contacts_mutex);
    return(contact);
}

char *addcontact(struct sockaddr *prevaddr,char *NXTaddr,char *NXTACCTSECRET,char *sender,char *handle,char *acct)
{
    bits256 mysecret,mypublic;
    struct coin_info *cp = get_coin_info("BTCD");
    struct contact_info *contact;
    char retstr[1024],pubkeystr[128];
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
    contact->nxt64bits = conv_rsacctstr(acct,0);
    contact->pubkey = issue_getpubkey(acct);
    init_hexbytes(pubkeystr,contact->pubkey.bytes,sizeof(contact->pubkey));
    if ( contact->pubkey.txid == 0 )
        sprintf(retstr,"{\"error\":\"(%s) acct.(%s) has no pubkey.(%s)\"}",handle,acct,pubkeystr);
    else
    {
        conv_NXTpassword(mysecret.bytes,mypublic.bytes,cp->privateNXTACCTSECRET);
        contact->shared = curve25519(mysecret,contact->pubkey);
        sprintf(retstr,"{\"result\":\"(%s) acct.(%s) (%llu) has pubkey.(%s)\"}",handle,acct,(long long)contact->nxt64bits,pubkeystr);
    }
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
    init_hexbytes(pubkeystr,contact->pubkey.bytes,sizeof(contact->pubkey));
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

#endif
