//
//  rambooks.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_rambooks_h
#define xcode_rambooks_h


struct rambook_info *find_rambook(uint64_t rambook_hashbits[3])
{
    struct rambook_info *rb;
    HASH_FIND(hh,Rambooks,rambook_hashbits,sizeof(rb->assetids),rb);
    return(rb);
}

uint64_t purge_oldest_order(struct rambook_info *rb,struct InstantDEX_quote *iQ) // allow one pair per orderbook
{
    char NXTaddr[64];
    struct NXT_acct *np;
    int32_t age,oldi,createdflag,duration;
    uint64_t nxt64bits = 0;
    uint32_t now,i,oldest = 0;
    if ( rb->numquotes == 0 )
        return(0);
    oldi = -1;
    now = (uint32_t)time(NULL);
    for (i=0; i<rb->numquotes; i++)
    {
        duration = rb->quotes[i].duration;
        if ( duration <= 0 || duration > ORDERBOOK_EXPIRATION )
            duration = ORDERBOOK_EXPIRATION;
        age = (now - rb->quotes[i].timestamp);
        if ( rb->quotes[i].exchangeid == INSTANTDEX_EXCHANGEID && (age >= ORDERBOOK_EXPIRATION || age >= duration) )
        {
            if ( (iQ == 0 || rb->quotes[i].nxt64bits == iQ->nxt64bits) && (oldest == 0 || rb->quotes[i].timestamp < oldest) )
            {
                oldest = rb->quotes[i].timestamp;
                //fprintf(stderr,"(oldi.%d %u) ",j,oldest);
                nxt64bits = rb->quotes[i].nxt64bits;
                oldi = i;
            }
        }
    }
    if ( oldi >= 0 )
    {
        rb->quotes[oldi] = rb->quotes[--rb->numquotes];
        memset(&rb->quotes[rb->numquotes],0,sizeof(rb->quotes[rb->numquotes]));
        expand_nxt64bits(NXTaddr,nxt64bits);
        np = get_NXTacct(&createdflag,NXTaddr);
        if ( np->openorders > 0 )
            np->openorders--;
        fprintf(stderr,"purge_oldest_order from NXT.%llu (openorders.%d) oldi.%d timestamp %u\n",(long long)nxt64bits,np->openorders,oldi,oldest);
    } //else fprintf(stderr,"no purges: numquotes.%d\n",rb->numquotes);
    return(nxt64bits);
}

struct rambook_info *get_rambook(char *_base,uint64_t baseid,char *_rel,uint64_t relid,uint64_t exchangebits)
{
    char base[16],rel[16];
    uint64_t assetids[3],basemult,relmult;
    uint32_t i,basetype,reltype,exchangeid = (uint32_t)(exchangebits >> 1);
    struct rambook_info *rb;
    if ( exchangeid >= MAX_EXCHANGES )
    {
        printf("illegal exchangeid.%d from exchangebits.%llx\n",exchangeid,(long long)exchangebits);
        return(0);
    }
    if ( exchangeid == INSTANTDEX_NXTAEID || exchangeid == INSTANTDEX_UNCONFID )
    {
        if ( relid != NXT_ASSETID || is_native_crypto(base,baseid) != 0 )
        {
            printf("illegal NXT rambook %llu/%llu\n",(long long)baseid,(long long)relid), getchar();
            return(0);
        }
    }
    if ( _base == 0 )
        basetype = set_assetname(&basemult,base,baseid);
    else basetype = INSTANTDEX_NATIVE, strcpy(base,_base);
    if ( _rel == 0 )
        reltype = set_assetname(&relmult,rel,relid);
    else reltype = INSTANTDEX_NATIVE, strcpy(rel,_rel);
    assetids[0] = baseid, assetids[1] = relid, assetids[2] = exchangebits;//, assetids[3] = (((uint64_t)basetype << 32) | reltype);
    if ( (rb= find_rambook(assetids)) == 0 )
    {
        rb = calloc(1,sizeof(*rb));
        strncpy(rb->exchange,Exchanges[exchangeid].name,sizeof(rb->exchange)-1);
        strcpy(rb->base,base), strcpy(rb->rel,rel);
        touppercase(rb->base), strcpy(rb->lbase,rb->base), tolowercase(rb->lbase);
        touppercase(rb->rel), strcpy(rb->lrel,rb->rel), tolowercase(rb->lrel);
        for (i=0; i<3; i++)
            rb->assetids[i] = assetids[i];
        if ( Debuglevel > 1 )
            printf("CREATE RAMBOOK.(%llu -> %llu).%d %s (%s) (%s)\n",(long long)baseid,(long long)relid,(int)exchangebits,Exchanges[exchangeid].name,rb->base,rb->rel);
        HASH_ADD(hh,Rambooks,assetids,sizeof(rb->assetids),rb);
    }
    return(rb);
}

struct rambook_info **get_allrambooks(int32_t *numbooksp)
{
    int32_t i = 0;
    struct rambook_info *rb,*tmp,**obooks;
    *numbooksp = HASH_COUNT(Rambooks);
    obooks = calloc(*numbooksp,sizeof(*rb));
    HASH_ITER(hh,Rambooks,rb,tmp)
    {
        purge_oldest_order(rb,0);
        obooks[i++] = rb;
    }
    if ( i != *numbooksp )
        printf("get_allrambooks HASH_COUNT.%d vs i.%d\n",*numbooksp,i);
    return(obooks);
}

cJSON *rambook_json(struct rambook_info *rb)
{
    cJSON *json = cJSON_CreateObject();
    char numstr[64];
    cJSON_AddItemToObject(json,"base",cJSON_CreateString(rb->base));
    sprintf(numstr,"%llu",(long long)rb->assetids[0]), cJSON_AddItemToObject(json,"baseid",cJSON_CreateString(numstr));
    cJSON_AddItemToObject(json,"rel",cJSON_CreateString(rb->rel));
    sprintf(numstr,"%llu",(long long)rb->assetids[1]), cJSON_AddItemToObject(json,"relid",cJSON_CreateString(numstr));
    cJSON_AddItemToObject(json,"numquotes",cJSON_CreateNumber(rb->numquotes));
    cJSON_AddItemToObject(json,"exchange",cJSON_CreateString(rb->exchange));
    cJSON_AddItemToObject(json,"type",cJSON_CreateString((rb->assetids[2]&1) == 0 ? "bids" : "asks"));
    return(json);
}

cJSON *all_orderbooks()
{
    cJSON *array,*json = 0;
    struct rambook_info **obooks;
    int32_t i,numbooks;
    if ( (obooks= get_allrambooks(&numbooks)) != 0 )
    {
        array = cJSON_CreateArray();
        for (i=0; i<numbooks; i++)
            cJSON_AddItemToArray(array,rambook_json(obooks[i]));
        free(obooks);
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json,"orderbooks",array);
    }
    return(json);
}

struct InstantDEX_quote *findquoteid(uint64_t quoteid,int32_t evenclosed)
{
    struct rambook_info **obooks,*rb;
    struct InstantDEX_quote *iQ;
    int32_t i,j,numbooks;
    if ( (obooks= get_allrambooks(&numbooks)) != 0 )
    {
        for (i=0; i<numbooks; i++)
        {
            rb = obooks[i];
            if ( rb->numquotes == 0 )
                continue;
            for (j=0; j<rb->numquotes; j++)
            {
                iQ = &rb->quotes[j];
                if ( (evenclosed != 0 || iQ->closed == 0) && calc_quoteid(iQ) == quoteid )
                {
                    free(obooks);
                    return(iQ);
                }
            }
        }
        free(obooks);
    }
    return(0);
}

void add_user_order(struct rambook_info *rb,struct InstantDEX_quote *iQ)
{
    static portable_mutex_t mutex;
    static int didinit;
    int32_t i;
    if ( rb->numquotes > 0 )
    {
        for (i=0; i<rb->numquotes; i++)
        {
            if ( memcmp(iQ,&rb->quotes[i],sizeof(rb->quotes[i])) == 0 )
                break;
        }
    } else i = 0;
    if ( i == rb->numquotes )
    {
        if ( didinit == 0 )
        {
            portable_mutex_init(&mutex);
            didinit = 1;
        }
        portable_mutex_lock(&mutex);
        if ( i >= rb->maxquotes )
        {
            rb->maxquotes += 50;
            rb->quotes = realloc(rb->quotes,rb->maxquotes * sizeof(*rb->quotes));
            memset(&rb->quotes[i],0,50 * sizeof(*rb->quotes));
        }
        rb->quotes[rb->numquotes++] = *iQ;
        portable_mutex_unlock(&mutex);
    }
    rb->updated = 1;
   // printf("add_user_order i.%d numquotes.%d\n",i,rb->numquotes);
}

void save_InstantDEX_quote(struct rambook_info *rb,struct InstantDEX_quote *iQ)
{
    char NXTaddr[64];
    struct NXT_acct *np;
    int32_t createdflag,maxallowed;
    maxallowed = calc_users_maxopentrades(iQ->nxt64bits);
    expand_nxt64bits(NXTaddr,iQ->nxt64bits);
    np = get_NXTacct(&createdflag,NXTaddr);
    //if ( np->openorders >= maxallowed )
    //    purge_oldest_order(rb,iQ);
    purge_oldest_order(rb,0);
    add_user_order(rb,iQ);
    np->openorders++;
}

struct rambook_info *_add_rambook_quote(struct InstantDEX_quote *iQ,struct rambook_info *rb,uint64_t nxt64bits,uint32_t timestamp,int32_t dir,double refprice,double refvolume,uint64_t baseid,uint64_t baseamount,uint64_t relid,uint64_t relamount,char *gui,uint64_t quoteid,int32_t duration)
{
    double price,volume;
    uint64_t basemult,relmult;
    if ( timestamp == 0 )
        timestamp = (uint32_t)time(NULL);
    if ( baseamount == 0 || relamount == 0 )
        set_best_amounts(&baseamount,&relamount,refprice,refvolume);
    basemult = get_assetmult(baseid), relmult = get_assetmult(relid);
    baseamount = (baseamount + basemult/2) / basemult, baseamount *= basemult;
    relamount = (relamount + relmult/2) / relmult, relamount *= relmult;
    if ( refprice != 0. && refvolume != 0 )
    {
        price = calc_price_volume(&volume,baseamount,relamount);
        if ( fabs(refprice - price)/price > 0.001 )
        {
            printf("cant create accurate price ref.(%f %f) -> (%f %f)\n",refprice,refvolume,price,volume);
            return(0);
        }
    }
    create_InstantDEX_quote(iQ,timestamp,dir < 0,quoteid,0,0,baseid,baseamount,relid,relamount,rb->exchange,nxt64bits,gui,0,0,duration);
    if ( timestamp > rb->lastaccess )
        rb->lastaccess = timestamp;
    if ( iQ->exchangeid != INSTANTDEX_EXCHANGEID )
        iQ->minperc = 1;
    if ( rb->assetids[0] != baseid || rb->assetids[1] != relid || iQ->baseid != baseid || iQ->relid != relid )
        printf("_add_rambook_quote mismatch: %llu %llu baseid.%llu | relid.%llu %llu %llu\n",(long long)rb->assetids[0],(long long)iQ->baseid,(long long)baseid,(long long)relid,(long long)iQ->relid,(long long)rb->assetids[1]);
    return(rb);
}

struct rambook_info *add_rambook_quote(char *exchangestr,struct InstantDEX_quote *iQ,uint64_t nxt64bits,uint32_t timestamp,int32_t dir,uint64_t baseid,uint64_t relid,double price,double volume,uint64_t baseamount,uint64_t relamount,char *gui,uint64_t quoteid,int32_t duration)
{
    void emit_iQ(struct rambook_info *rb,struct InstantDEX_quote *iQ);
    struct exchange_info *exchange;
    struct rambook_info *rb = 0;
    memset(iQ,0,sizeof(*iQ));
    if ( (exchange= find_exchange(exchangestr,0,0)) != 0 )
    {
        //printf("add.(%llu %llu) %llu/%llu\n",(long long)baseid,(long long)relid,(long long)baseamount,(long long)relamount);
        if ( (rb= get_rambook(0,baseid,0,relid,(exchange->exchangeid<<1) | (dir<0))) != 0 )
        {
            //printf("add.(%llu %llu) %llu/%llu\n",(long long)baseid,(long long)relid,(long long)baseamount,(long long)relamount);
            if ( _add_rambook_quote(iQ,rb,nxt64bits,timestamp,dir,price,volume,baseid,baseamount,relid,relamount,gui,quoteid,duration) != 0 )
            {
                save_InstantDEX_quote(rb,iQ);
                emit_iQ(rb,iQ);
            } else return(0);
        }
    } else printf("add_rambook_quote cant find.(%s)\n",exchangestr);
    return(rb);
}

char *allorderbooks_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    cJSON *json;
    char *jsonstr;
    if ( (json= all_orderbooks()) != 0 )
    {
        jsonstr = cJSON_Print(json);
        free_json(json);
        return(jsonstr);
    }
    return(clonestr("{\"error\":\"no orderbooks\"}"));
}

cJSON *openorders_json(char *NXTaddr)
{
    cJSON *array,*item,*json = 0;
    struct rambook_info **obooks,*rb;
    struct InstantDEX_quote *iQ;
    int32_t i,j,numbooks,n = 0;
    uint64_t baseamount,relamount,ptr;
    char nxtaddr[64],numstr[64];
   // update_iQ_flags(0,0,0,0);
    if ( (obooks= get_allrambooks(&numbooks)) != 0 )
    {
        array = cJSON_CreateArray();
        for (i=0; i<numbooks; i++)
        {
            rb = obooks[i];
            if ( rb->numquotes == 0 )
                continue;
            for (j=0; j<rb->numquotes; j++)
            {
                iQ = &rb->quotes[j];
                expand_nxt64bits(nxtaddr,iQ->nxt64bits);
                if ( strcmp(NXTaddr,nxtaddr) == 0 && iQ->closed == 0 )
                {
                    baseamount = iQ->baseamount, relamount = iQ->relamount;
                    if ( (item= gen_InstantDEX_json(&baseamount,&relamount,0,iQ->isask,iQ,rb->assetids[0],rb->assetids[1],0)) != 0 )
                    {
                        ptr = (uint64_t)iQ;
                        sprintf(numstr,"%llu",(long long)ptr), cJSON_AddItemToObject(item,"iQ",cJSON_CreateString(numstr));
                        cJSON_AddItemToArray(array,item);
                        n++;
                    }
                }
            }
        }
        free(obooks);
        if ( n > 0 )
        {
            json = cJSON_CreateObject();
            cJSON_AddItemToObject(json,"openorders",array);
            return(json);
        }
    }
    return(json);
}

char *openorders_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    cJSON *json = 0;
    char *jsonstr;
    if ( (json= openorders_json(NXTaddr)) != 0 )
    {
        jsonstr = cJSON_Print(json);
        free_json(json);
        return(jsonstr);
    }
    return(clonestr("{\"error\":\"no openorders\"}"));
}

int32_t cancelquote(char *NXTaddr,uint64_t quoteid)
{
    struct InstantDEX_quote *iQ;
    char nxtaddr[64];
    if ( (iQ= findquoteid(quoteid,0)) != 0 && iQ->nxt64bits == calc_nxt64bits(NXTaddr) && iQ->baseiQ == 0 && iQ->reliQ == 0 && iQ->exchangeid == INSTANTDEX_EXCHANGEID )
    {
        expand_nxt64bits(nxtaddr,iQ->nxt64bits);
        if ( strcmp(NXTaddr,nxtaddr) == 0 && calc_quoteid(iQ) == quoteid )
        {
            cancel_InstantDEX_quote(iQ);
            return(1);
        }
    }
    return(0);
}

char *cancelquote_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    uint64_t quoteid;
    printf("inside cancelquote\n");
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    quoteid = get_API_nxt64bits(objs[0]);
    printf("cancelquote %llu\n",(long long)quoteid);
    if ( cancelquote(NXTaddr,quoteid) > 0 )
        return(clonestr("{\"result\":\"quote cancelled\"}"));
    else return(clonestr("{\"result\":\"you can only cancel your InstantDEX orders\"}"));
}

int32_t match_unconfirmed(struct rambook_info **obooks,int32_t numbooks,char *account,uint64_t quoteid)
{
    int32_t j,k;
    struct rambook_info *rb;
    struct InstantDEX_quote *iQ;
    if ( strcmp(account,INSTANTDEX_ACCT) == 0 && quoteid != 0 )
    {
        for (j=0; j<numbooks; j++)
        {
            rb = obooks[j];
            if ( strcmp(INSTANTDEX_NAME,rb->exchange) == 0 )
            {
                for (k=0; k<rb->numquotes; k++)
                {
                    iQ = &rb->quotes[k];
                    if ( calc_quoteid(iQ) == quoteid )
                    {
                        if ( iQ->matched == 0 )
                        {
                            iQ->matched = 1;
                            printf("MARK MATCHED TRADE FROM UNCONFIRMED\n");
                            return(1);
                        } else return(0);
                    }
                }
            }
        }
    }
    return(-1);
}

int32_t update_iQ_flags(struct NXT_tx *txptrs[],int32_t maxtx,uint64_t refassetid)
{
    struct rambook_info **get_allrambooks(int32_t *numbooksp);
    uint64_t quoteid,assetid,amount,qty,priceNQT;
    char cmd[1024],txidstr[MAX_JSON_FIELD],account[MAX_JSON_FIELD],comment[MAX_JSON_FIELD],*jsonstr;
    cJSON *json,*array,*txobj,*attachment,*msgobj,*commentobj;
    int32_t i,n,numbooks,type,subtype,m = 0;
    struct rambook_info **obooks;
    txptrs[0] = 0;
    if ( (obooks= get_allrambooks(&numbooks)) == 0 )
        return(0);
    sprintf(cmd,"requestType=getUnconfirmedTransactions");
    if ( (jsonstr= issue_NXTPOST(0,cmd)) != 0 )
    {
        //printf("getUnconfirmedTransactions.(%llu %llu) (%s)\n",(long long)baseid,(long long)relid,jsonstr);
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"unconfirmedTransactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    txobj = cJSON_GetArrayItem(array,i);
                    copy_cJSON(txidstr,cJSON_GetObjectItem(txobj,"transaction"));
                    copy_cJSON(account,cJSON_GetObjectItem(txobj,"account"));
                    if ( account[0] == 0 )
                        copy_cJSON(account,cJSON_GetObjectItem(txobj,"sender"));
                    qty = amount = assetid = quoteid = 0;
                    amount = get_API_nxt64bits(cJSON_GetObjectItem(txobj,"amountNQT"));
                    type = (int32_t)get_API_int(cJSON_GetObjectItem(txobj,"type"),-1);
                    subtype = (int32_t)get_API_int(cJSON_GetObjectItem(txobj,"subtype"),-1);
                    if ( (attachment= cJSON_GetObjectItem(txobj,"attachment")) != 0 )
                    {
                        assetid = get_API_nxt64bits(cJSON_GetObjectItem(attachment,"asset"));
                        comment[0] = 0;
                        if ( (msgobj= cJSON_GetObjectItem(attachment,"message")) != 0 )
                        {
                            qty = get_API_nxt64bits(cJSON_GetObjectItem(attachment,"quantityQNT"));
                            priceNQT = get_API_nxt64bits(cJSON_GetObjectItem(attachment,"priceNQT"));
                            copy_cJSON(comment,msgobj);
                            if ( comment[0] != 0 )
                            {
                                unstringify(comment);
                                if ( (commentobj= cJSON_Parse(comment)) != 0 )
                                {
                                    quoteid = get_API_nxt64bits(cJSON_GetObjectItem(commentobj,"quoteid"));
                                    if ( Debuglevel > 2 )
                                        printf("acct.(%s) pending quoteid.%llu asset.%llu qty.%llu %.8f amount %.8f %d:%d tx.%s\n",account,(long long)quoteid,(long long)assetid,(long long)qty,dstr(priceNQT),dstr(amount),type,subtype,txidstr);
                                    if ( quoteid != 0 )
                                        match_unconfirmed(obooks,numbooks,account,quoteid);
                                    free_json(commentobj);
                                }
                            }
                        }
                        if ( txptrs != 0 && m < maxtx && (refassetid == 0 || refassetid == assetid) )
                        {
                            txptrs[m] = set_NXT_tx(txobj);
                            txptrs[m]->timestamp = calc_expiration(txptrs[m]);
                            txptrs[m]->quoteid = quoteid;
                            strcpy(txptrs[m]->comment,comment);
                            m++;
                            //printf("m.%d: assetid.%llu type.%d subtype.%d price.%llu qty.%llu time.%u vs %ld deadline.%d\n",m,(long long)txptrs[m-1]->assetidbits,txptrs[m-1]->type,txptrs[m-1]->subtype,(long long)txptrs[m-1]->priceNQT,(long long)txptrs[m-1]->U.quantityQNT,txptrs[m-1]->timestamp,time(NULL),txptrs[m-1]->deadline);
                        }
                    }
                }
            } free_json(json);
        } free(jsonstr);
    } free(obooks);
    txptrs[m] = 0;
    return(m);
}

#endif
