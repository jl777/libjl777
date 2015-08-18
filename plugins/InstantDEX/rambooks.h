//
//  rambooks.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_rambooks_h
#define xcode_rambooks_h


struct InstantDEX_quote *find_iQ(uint64_t quoteid)
{
    struct InstantDEX_quote *iQ;
    HASH_FIND(hh,AllQuotes,&quoteid,sizeof(quoteid),iQ);
    return(iQ);
}

struct InstantDEX_quote *create_iQ(struct InstantDEX_quote *iQ)
{
    struct InstantDEX_quote *newiQ;
    if ( (newiQ= find_iQ(iQ->quoteid)) != 0 )
        return(newiQ);
    newiQ = calloc(1,sizeof(*newiQ));
    *newiQ = *iQ;
    HASH_ADD(hh,AllQuotes,quoteid,sizeof(newiQ->quoteid),newiQ);
    {
        struct InstantDEX_quote *checkiQ;
        if ( (checkiQ= find_iQ(iQ->quoteid)) == 0 || iQcmp(iQ,checkiQ) != 0 )//memcmp((uint8_t *)((long)checkiQ + sizeof(checkiQ->hh) + sizeof(checkiQ->quoteid)),(uint8_t *)((long)iQ + sizeof(iQ->hh) + sizeof(iQ->quoteid)),sizeof(*iQ) - sizeof(iQ->hh) - sizeof(iQ->quoteid)) != 0 )
        {
            int32_t i;
            for (i=(sizeof(iQ->hh) - sizeof(iQ->quoteid)); i<sizeof(*iQ) - sizeof(iQ->hh) - sizeof(iQ->quoteid); i++)
                printf("%02x ",((uint8_t *)iQ)[i]);
            printf("iQ\n");
            for (i=(sizeof(checkiQ->hh) + sizeof(checkiQ->quoteid)); i<sizeof(*checkiQ) - sizeof(checkiQ->hh) - sizeof(checkiQ->quoteid); i++)
                printf("%02x ",((uint8_t *)checkiQ)[i]);
            printf("checkiQ\n");
            printf("error finding iQ after adding %llu vs %llu\n",(long long)checkiQ->quoteid,(long long)iQ->quoteid);
        }
    }
    return(newiQ);
}

struct InstantDEX_quote *findquoteid(uint64_t quoteid,int32_t evenclosed)
{
    struct InstantDEX_quote *iQ;
    if ( (iQ= find_iQ(quoteid)) != 0 )
    {
        if ( evenclosed != 0 || iQ->closed == 0 )
        {
            if ( calc_quoteid(iQ) == quoteid )
                return(iQ);
            else printf("calc_quoteid %llu vs %llu\n",(long long)calc_quoteid(iQ),(long long)quoteid);
        } //else printf("quoteid.%llu closed.%d\n",(long long)quoteid,iQ->closed);
    } else printf("couldnt find %llu\n",(long long)quoteid);
    return(0);
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

void add_user_order(struct rambook_info *rb,struct InstantDEX_quote *iQ)
{
    static portable_mutex_t mutex;
    static int didinit;
    int32_t i,incr = 50;
    if ( rb->numquotes > 0 )
    {
        for (i=0; i<rb->numquotes; i++)
        {
            if ( rb->quotes[i] != 0 && memcmp(iQ,rb->quotes[i],sizeof(*rb->quotes[i])) == 0 )
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
            rb->maxquotes += incr;
            rb->quotes = realloc(rb->quotes,rb->maxquotes * sizeof(*rb->quotes));
            memset(&rb->quotes[i],0,incr * sizeof(*rb->quotes));
        }
        rb->quotes[rb->numquotes] = create_iQ(iQ), rb->numquotes++;
        portable_mutex_unlock(&mutex);
    }
    rb->updated = 1;
    // printf("add_user_order i.%d numquotes.%d\n",i,rb->numquotes);
}

struct rambook_info *find_rambook(uint64_t rambook_hashbits[3])
{
    struct rambook_info *rb;
    HASH_FIND(hh,Rambooks,rambook_hashbits,sizeof(rb->assetids),rb);
    return(rb);
}

uint64_t purge_oldest_order(struct rambook_info *rb,struct InstantDEX_quote *iQ) // allow one pair per orderbook
{
    char NXTaddr[64]; struct NXT_acct *np; int32_t age,oldi,createdflag,duration; uint64_t nxt64bits = 0; uint32_t now,i,oldest = 0;
    struct InstantDEX_quote *rbiQ;
    if ( rb->numquotes == 0 )
        return(0);
    oldi = -1;
    now = (uint32_t)time(NULL);
    for (i=0; i<rb->numquotes; i++)
    {
        if ( (rbiQ= rb->quotes[i]) != 0 )
        {
            duration = rbiQ->duration;
            if ( duration <= 0 || duration > ORDERBOOK_EXPIRATION )
                duration = ORDERBOOK_EXPIRATION;
            age = (now - rbiQ->timestamp);
            if ( rbiQ->exchangeid == INSTANTDEX_EXCHANGEID && (age >= ORDERBOOK_EXPIRATION || age >= duration) )
            {
                if ( (iQ == 0 || rbiQ->nxt64bits == iQ->nxt64bits) && (oldest == 0 || rbiQ->timestamp < oldest) )
                {
                    oldest = rbiQ->timestamp;
                    //fprintf(stderr,"(oldi.%d %u) ",j,oldest);
                    nxt64bits = rbiQ->nxt64bits;
                    oldi = i;
                }
            }
        }
    }
    if ( oldi >= 0 )
    {
        cancel_InstantDEX_quote(rb->quotes[oldi]);
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

struct rambook_info *get_rambook(char *_base,uint64_t baseid,char *_rel,uint64_t relid,uint64_t exchangebits,char *gui)
{
    struct rambook_info *rb; char base[16],rel[16]; uint64_t assetids[3],basemult,relmult; uint32_t i,basetype,reltype,exchangeid = (uint32_t)(exchangebits >> 1);
    if ( exchangeid >= MAX_EXCHANGES )
    {
        printf("illegal exchangeid.%d from exchangebits.%llx\n",exchangeid,(long long)exchangebits);
        return(0);
    }
    if ( exchangeid == INSTANTDEX_NXTAEID || exchangeid == INSTANTDEX_UNCONFID )
    {
        if ( relid != NXT_ASSETID || is_native_crypto(base,baseid) != 0 )
        {
            printf("illegal NXT rambook %llu/%llu\n",(long long)baseid,(long long)relid);//, getchar();
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
        strncpy(rb->exchange,exchange_str(exchangeid),sizeof(rb->exchange)-1);
        strcpy(rb->base,base), strcpy(rb->rel,rel);
        if ( gui != 0 )
            strcpy(rb->gui,gui);
        touppercase(rb->base), strcpy(rb->lbase,rb->base), tolowercase(rb->lbase);
        touppercase(rb->rel), strcpy(rb->lrel,rb->rel), tolowercase(rb->lrel);
        for (i=0; i<3; i++)
            rb->assetids[i] = assetids[i];
        if ( Debuglevel > 1 )
            printf("CREATE RAMBOOK.(%llu -> %llu).%d name.(%s) (%s) (%s) gui.(%s)\n",(long long)baseid,(long long)relid,(int)exchangebits,exchange_str(exchangeid),rb->base,rb->rel,gui!=0?"":gui);
        HASH_ADD(hh,Rambooks,assetids,sizeof(rb->assetids),rb);
    }
    if ( rb->gui[0] == 0 && gui != 0 )
        strcpy(rb->gui,gui);
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
    //void emit_iQ(struct rambook_info *rb,struct InstantDEX_quote *iQ);
    struct exchange_info *exchange; int32_t exchangeid; struct rambook_info *rb = 0;
    memset(iQ,0,sizeof(*iQ));
    if ( (exchange= find_exchange(&exchangeid,exchangestr)) != 0 )
    {
        //printf("add.(%llu %llu) %llu/%llu\n",(long long)baseid,(long long)relid,(long long)baseamount,(long long)relamount);
        if ( (rb= get_rambook(0,baseid,0,relid,(exchangeid<<1) | (dir<0),gui)) != 0 )
        {
            //printf("add.(%llu %llu) %llu/%llu price %.8f vol %.8f\n",(long long)baseid,(long long)relid,(long long)baseamount,(long long)relamount,price,volume);
            if ( _add_rambook_quote(iQ,rb,nxt64bits,timestamp,dir,price,volume,baseid,baseamount,relid,relamount,gui,quoteid,duration) != 0 )
            {
                save_InstantDEX_quote(rb,iQ);
                //emit_iQ(rb,iQ);
            } else return(0);
        }
    } else printf("add_rambook_quote cant find.(%s)\n",exchangestr);
    return(rb);
}

char *allorderbooks_func(int32_t localaccess,int32_t valid,char *sender,cJSON **objs,int32_t numobjs,char *origargstr)
{
    cJSON *exchanges_json();
    cJSON *json; char *jsonstr;
   // printf("all orderbooks\n");
    if ( (json= all_orderbooks()) != 0 )
    {
        cJSON_AddItemToObject(json,"exchanges",exchanges_json());
        cJSON_AddItemToObject(json,"NXTAPIURL",cJSON_CreateString(SUPERNET.NXTAPIURL));
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
                if ( (iQ= rb->quotes[j]) != 0 )
                {
                    expand_nxt64bits(nxtaddr,iQ->nxt64bits);
                    if ( strcmp(NXTaddr,nxtaddr) == 0 && iQ->closed == 0 )
                    {
                        baseamount = iQ->baseamount, relamount = iQ->relamount;
                        if ( (item= gen_InstantDEX_json(0,&baseamount,&relamount,0,iQ->isask,iQ,rb->assetids[0],rb->assetids[1],0)) != 0 )
                        {
                            ptr = (uint64_t)iQ;
                            sprintf(numstr,"%llu",(long long)ptr), cJSON_AddItemToObject(item,"iQ",cJSON_CreateString(numstr));
                            cJSON_AddItemToArray(array,item);
                            n++;
                        }
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

char *openorders_func(int32_t localaccess,int32_t valid,char *sender,cJSON **objs,int32_t numobjs,char *origargstr)
{
    cJSON *json = 0;
    char *jsonstr;
    if ( (json= openorders_json(SUPERNET.NXTADDR)) != 0 )
    {
        jsonstr = cJSON_Print(json);
        free_json(json);
        return(jsonstr);
    }
    return(clonestr("{\"error\":\"no openorders\"}"));
}

char *cancelquote_func(int32_t localaccess,int32_t valid,char *sender,cJSON **objs,int32_t numobjs,char *origargstr)
{
    struct InstantDEX_quote *iQ;
    uint64_t quoteid; char *retstr;
    if ( localaccess == 0 )
        return(0);
    quoteid = get_API_nxt64bits(objs[0]);
    if ( (retstr= cancel_orderid(SUPERNET.NXTADDR,quoteid)) != 0 )
    {
        if ( (iQ= findquoteid(quoteid,0)) != 0 && iQ->nxt64bits == calc_nxt64bits(SUPERNET.NXTADDR) )
            cancel_InstantDEX_quote(iQ);
        return(retstr);
    }
    if ( cancelquote(SUPERNET.NXTADDR,quoteid) > 0 )
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
                    if ( (iQ= rb->quotes[k]) && calc_quoteid(iQ) == quoteid )
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
    //struct rambook_info **get_allrambooks(int32_t *numbooksp);
    uint64_t quoteid,assetid,amount,qty,priceNQT;
    char cmd[1024],txidstr[MAX_JSON_FIELD],account[MAX_JSON_FIELD],comment[MAX_JSON_FIELD],*jsonstr;
    cJSON *json,*array,*txobj,*attachment,*msgobj,*commentobj;
    int32_t i,n,numbooks,type,subtype,m = 0;
    struct rambook_info **obooks;
    txptrs[0] = 0;
    if ( (obooks= get_allrambooks(&numbooks)) == 0 )
        return(0);
    sprintf(cmd,"requestType=getUnconfirmedTransactions");
    if ( (jsonstr= issue_NXTPOST(cmd)) != 0 )
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

char *InstantDEX_openorders()
{
    return(clonestr("{\"error\":\"API is not yet\"}"));
}

char *InstantDEX_tradehistory()
{
    return(clonestr("{\"error\":\"API is not yet\"}"));
}

char *InstantDEX_cancelorder(uint64_t orderid)
{
    return(clonestr("{\"error\":\"API is not yet\"}"));
}

cJSON *InstantDEX_lottostats()
{
    char cmdstr[1024],NXTaddr[64],buf[1024],receiverstr[MAX_JSON_FIELD],*jsonstr;
    cJSON *json,*array,*txobj;
    int32_t i,n,totaltickets = 0;
    uint64_t amount,senderbits;
    uint32_t timestamp = 0;
    if ( timestamp == 0 )
        timestamp = 38785003;
    sprintf(cmdstr,"requestType=getAccountTransactions&account=%s&timestamp=%u&type=0&subtype=0",INSTANTDEX_ACCT,timestamp);
    //printf("cmd.(%s)\n",cmdstr);
    if ( (jsonstr= issue_NXTPOST(cmdstr)) != 0 )
    {
        // printf("jsonstr.(%s)\n",jsonstr);
        // mm string.({"requestProcessingTime":33,"transactions":[{"fullHash":"2a2aab3b84dadf092cf4cedcd58a8b5a436968e836338e361c45651bce0ef97e","confirmations":203,"signatureHash":"52a4a43d9055fe4861b3d13fbd03a42fecb8c9ad4ac06a54da7806a8acd9c5d1","transaction":"711527527619439146","amountNQT":"1100000000","transactionIndex":2,"ecBlockHeight":360943,"block":"6797727125503999830","recipientRS":"NXT-74VC-NKPE-RYCA-5LMPT","type":0,"feeNQT":"100000000","recipient":"4383817337783094122","version":1,"sender":"423766016895692955","timestamp":38929220,"ecBlockId":"10121077683890606382","height":360949,"subtype":0,"senderPublicKey":"4e5bbad625df3d536fa90b1e6a28c3f5a56e1fcbe34132391c8d3fd7f671cb19","deadline":1440,"blockTimestamp":38929430,"senderRS":"NXT-8E6V-YBWH-5VMR-26ESD","signature":"4318f36d9cf68ef0a8f58303beb0ed836b670914065a868053da5fe8b096bc0c268e682c0274e1614fc26f81be4564ca517d922deccf169eafa249a88de58036"}]})
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"transactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    txobj = cJSON_GetArrayItem(array,i);
                    copy_cJSON(receiverstr,cJSON_GetObjectItem(txobj,"recipient"));
                    if ( strcmp(receiverstr,INSTANTDEX_ACCT) == 0 )
                    {
                        if ( (senderbits = get_API_nxt64bits(cJSON_GetObjectItem(txobj,"sender"))) != 0 )
                        {
                            expand_nxt64bits(NXTaddr,senderbits);
                            amount = get_API_nxt64bits(cJSON_GetObjectItem(txobj,"amountNQT"));
                            if ( amount == INSTANTDEX_FEE )
                                totaltickets++;
                            else if ( amount >= 2*INSTANTDEX_FEE )
                                totaltickets += 2;
                        }
                    }
                }
            }
            free_json(json);
        }
        free(jsonstr);
    }
    sprintf(buf,"{\"result\":\"lottostats\",\"totaltickets\":\"%d\"}",totaltickets);
    return(cJSON_Parse(buf));
}

#endif
