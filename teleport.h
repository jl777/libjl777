//
//  teleport.h
//  xcode
//
//  Created by jl777 on 8/1/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_teleport_h
#define xcode_teleport_h

struct telepod
{
    struct storage_header H;
    int32_t vout;
    uint32_t crc,createtime,clonetime,cloneout,podstate,inhwm,podsize;
    double evolve_amount;
    char clonetxid[MAX_COINTXID_LEN],cloneaddr[MAX_COINADDR_LEN];
    uint64_t senderbits,destbits,unspent,modified,satoshis; // everything after modified is used for crc
    char coinstr[8],txid[MAX_COINTXID_LEN],coinaddr[MAX_COINADDR_LEN],script[128];
    char privkey[];
};

#define TELEPOD_CONTENTS_VOUT 0 // must be 0
#define TELEPOD_CHANGE_VOUT 1   // vout 0 is for the pod contents and last one (1 if no change or 2) is marker
#define TELEPOD_ERROR_VOUT 30000

#define TELEPOD_AVAIL 0
#define TELEPOD_INBOUND 1
#define TELEPOD_OUTBOUND 2
#define TELEPOD_DOUBLESPENT 3
#define TELEPOD_CANCELLED 4
#define TELEPOD_CLONED 5
#define TELEPOD_SPENT 6
#define TELEPOD_WITHDRAWN 7

int32_t telepod_normal_spend(struct telepod *pod)
{
    if ( pod->podstate == TELEPOD_CLONED || pod->podstate == TELEPOD_SPENT || pod->podstate == TELEPOD_WITHDRAWN )
        return(1);
    else return(0);
}

uint64_t calc_transporter_fee(struct coin_info *cp,uint64_t satoshis)
{
    if ( strcmp(cp->name,"BTCD") == 0 )
        return(cp->txfee);
    else return(cp->txfee + 0*(satoshis>>10));
}
#include "bitcoinglue.h"

void update_telepod(struct telepod *pod,char *keystr)
{
    update_storage(TELEPOD_DATA,keystr,&pod->H);
}

char *_podstate(int32_t podstate)
{
    switch ( podstate )
    {
        case TELEPOD_AVAIL: return("unspent");
        case TELEPOD_INBOUND: return("inbound");
        case TELEPOD_OUTBOUND: return("outbound");
        case TELEPOD_DOUBLESPENT: return("doublespent");
        case TELEPOD_CANCELLED: return("cancelled");
        case TELEPOD_CLONED: return("cloned");
        case TELEPOD_SPENT: return("spent");
        case TELEPOD_WITHDRAWN: return("withdrawn");
    }
    return("illegal");
}

cJSON *coin_specific_json(struct telepod *pod)
{
    cJSON *tpd;
    tpd = cJSON_CreateObject();
    cJSON_AddItemToObject(tpd,"t",cJSON_CreateNumber(pod->createtime));
    cJSON_AddItemToObject(tpd,"c",cJSON_CreateNumber(pod->crc));
    cJSON_AddItemToObject(tpd,"x",cJSON_CreateString(pod->txid));
    cJSON_AddItemToObject(tpd,"p",cJSON_CreateString(pod->privkey));
    if ( strcmp(pod->coinstr,"BBR") != 0 )
    {
        cJSON_AddItemToObject(tpd,"a",cJSON_CreateString(pod->coinaddr));
        cJSON_AddItemToObject(tpd,"o",cJSON_CreateNumber(pod->vout));
        cJSON_AddItemToObject(tpd,"s",cJSON_CreateString(pod->script));
    }
    return(tpd);
}

cJSON *telepod_json(struct telepod *pod)
{
    cJSON *json,*tpd;
    json = cJSON_CreateObject();
    if ( pod->podstate != TELEPOD_AVAIL )
        cJSON_AddItemToObject(json,"status",cJSON_CreateString(_podstate(pod->podstate)));
    cJSON_AddItemToObject(json,"c",cJSON_CreateString(pod->coinstr));
    tpd = coin_specific_json(pod);
    cJSON_AddItemToObject(json,"tpd",tpd);
    return(json);
}

void disp_telepod(char *msg,struct telepod *pod)
{
    char *str;
    cJSON *podjson;
    podjson = telepod_json(pod);
    if ( podjson != 0 )
    {
        str = cJSON_Print(podjson);
        stripwhite_ns(str,strlen(str));
        printf("%8s %s\n",msg,str);
        free(str);
        free_json(podjson);
    }
    else printf("%8s %s %s %s/vout.%d\n",msg,pod->coinstr,pod->coinaddr,pod->txid,pod->vout);
    //int32_t calc_multisig_N(struct telepod *pod);
    //char hexstr[1024];
    //init_hexbytes_noT(hexstr,_get_privkeyptr(pod,calc_multisig_N(pod)),pod->len_plus1-1);
    //printf("%p %6s %13.8f height.%-6d %6s %s %s/vout_%d priv.(%s)\n",pod,msg,dstr(pod->satoshis),pod->height,pod->coinstr,pod->coinaddr,pod->txid,pod->vout,_get_privkeyptr(pod,calc_multisig_N(pod)));
}

uint32_t calc_telepodcrc(struct telepod *pod)
{
    uint32_t offset,crc = 0;
    offset = (uint32_t)((long)&pod->modified + sizeof(pod->modified) - (long)pod);
    crc = _crc32(crc,(void *)((long)pod + offset),(pod->podsize - offset));
    return(crc);
}

struct telepod *create_telepod(uint32_t createtime,char *coinstr,uint64_t satoshis,char *podaddr,char *script,char *privkey,char *txid,int32_t vout)
{
    struct telepod *pod;
    int32_t size;
    size = (int32_t)(sizeof(*pod) + (strlen(privkey) + 1));
    pod = calloc(1,size);
    pod->createtime = createtime;
    pod->podsize = size;
    pod->vout = vout;
    pod->cloneout = -1;
    pod->satoshis = satoshis;
    safecopy(pod->coinstr,coinstr,sizeof(pod->coinstr));
    safecopy(pod->txid,txid,sizeof(pod->txid));
    safecopy(pod->coinaddr,podaddr,sizeof(pod->coinaddr));
    safecopy(pod->script,script,sizeof(pod->script));
    if ( privkey != 0 )
        strcpy((void *)pod->privkey,privkey);
    pod->crc = calc_telepodcrc(pod);
    disp_telepod("create",pod);
    return(pod);
}

char *issue_BBRcmd(struct coin_info *cp,char *method,char *basement,char *acctkeys,uint64_t amount,char *withdrawaddr)
{
    char params[MAX_JSON_FIELD],withdrawstr[MAX_JSON_FIELD];
    if ( strcmp(method,"maketelepod") == 0 )
    {
        if ( amount != 0 )
            sprintf(params,"{\"amount\":%llu}",(long long)amount);
        else return(0);
    }
    else
    {
        withdrawstr[0] = 0;
        if ( strcmp(method,"withdrawtelepod") == 0 )
            sprintf(withdrawstr,",\"addr\":\"%s\"",withdrawaddr);
        sprintf(params,"{\"tpd\":{\"account_keys_hex\":\"%s\",\"basement_tx_id_hex\":\"%s\"}%s}",acctkeys,basement,withdrawstr);
    }
    return(bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,method,params));
}

uint64_t BBR_telepodstatus(uint32_t *createtimep,struct coin_info *cp,struct telepod *pod)
{
    cJSON *json = 0;
    char *retstr;
    uint64_t unspent = 0;
    *createtimep = 0;
    if ( (retstr= issue_BBRcmd(cp,"telepodstatus",pod->txid,pod->privkey,0,0)) != 0 )
    {
        json = cJSON_Parse(retstr);
        unspent = get_API_nxt64bits(cJSON_GetObjectItem(json,"unspent"));
        *createtimep = get_API_int(cJSON_GetObjectItem(json,"createtime"),0);
        free(retstr);
    }
    return(unspent);
}

int32_t get_telepod_info(uint64_t *unspentp,uint32_t *createtimep,char *coinstr,struct telepod *pod)
{
    struct coin_info *cp = get_coin_info(coinstr);
    *unspentp = 0;
    if ( cp != 0 )
    {
        if ( strcmp(pod->coinstr,"BBR") == 0 )
            *unspentp = BBR_telepodstatus(createtimep,cp,pod);
        else *unspentp = check_txid(createtimep,cp,cp->minconfirms,pod->coinaddr,pod->txid,pod->vout,pod->script);
        if ( *createtimep == 0 )
            printf("get_telepod_info error for %s %s.vout.%d\n",pod->coinstr,pod->txid,pod->vout);
    }
    else
    {
        printf("need to implement remote API for get_telepod_info\n");
    }
    return((*createtimep == 0) ? -1 : 0);
}

struct telepod *conv_BBR_json(struct coin_info *cp,struct cJSON *tpd)
{
    struct telepod *pod = 0;
    char basement[MAX_JSON_FIELD],acctkeys[MAX_JSON_FIELD];
    extract_cJSON_str(acctkeys,sizeof(acctkeys),tpd,"account_keys_hex");
    extract_cJSON_str(basement,sizeof(basement),tpd,"basement_tx_id_hex");
    if ( acctkeys[0] != 0 && basement[0] != 0 )
    {
        pod = create_telepod(0,cp->name,0,"","",acctkeys,basement,-1);
        if ( (pod->unspent= BBR_telepodstatus(&pod->createtime,cp,pod)) != 0 )
            pod->satoshis = pod->unspent;
    }
    return(pod);
}

struct telepod *conv_BBR_jsonstr(struct coin_info *cp,char *retstr)
{
    cJSON *tpd,*json = 0;
    struct telepod *pod = 0;
    if ( (json= cJSON_Parse(retstr)) != 0 )
    {
        if ( (tpd= cJSON_GetObjectItem(json,"tpd")) != 0 )
            pod = conv_BBR_json(cp,tpd);
        free_json(json);
    }
    free(retstr);
    return(pod);
}

struct telepod *BBR_clonetelepod(struct coin_info *cp,struct telepod *refpod)
{
    struct telepod *pod = 0;
    char *retstr;
    if ( (retstr= issue_BBRcmd(cp,"clonetelepod",pod->txid,pod->privkey,0,0)) != 0 )
        pod = conv_BBR_jsonstr(cp,retstr);
    return(pod);
}

struct telepod *BBR_maketelepod(struct coin_info *cp,uint64_t satoshis)
{
    struct telepod *pod = 0;
    char *retstr;
    if ( (retstr= issue_BBRcmd(cp,"maketelepod",0,0,satoshis,0)) != 0 )
        pod = conv_BBR_jsonstr(cp,retstr);
    return(pod);
}

char *BBR_withdrawtelepod(struct coin_info *cp,struct telepod *pod,char *withdrawaddr)
{
    cJSON *json;
    char *retstr,buf[MAX_JSON_FIELD],*txidstr = 0;
    if ( (retstr= issue_BBRcmd(cp,"withdrawtelepod",pod->txid,pod->privkey,0,withdrawaddr)) != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            copy_cJSON(buf,cJSON_GetObjectItem(json,"txid"));
            if ( buf[0] != 0 )
                txidstr = clonestr(buf);
            free_json(json);
        }
        free(retstr);
    }
    return(txidstr);
}

struct telepod *clone_telepod(struct coin_info *cp,struct telepod *refpod,uint64_t refsatoshis,char *withdrawaddr)
{
    char *change_podaddr=0,*change_privkey,*podaddr=0,*txid,*privkey = 0,*retstr,pubkey[1024],change_pubkey[1024];
    struct rawtransaction RAW;
    struct telepod *pod = 0,*changepod=0,*inputpods[MAX_COIN_INPUTS],*refpods[2];
    uint64_t fee,change,availchange = 0;
    int32_t i,didalloc = 0;
    memset(inputpods,0,sizeof(inputpods));
    change_privkey = change_podaddr = 0;
    availchange = fee = change_pubkey[0] = 0;
    podaddr = withdrawaddr;
    if ( refpod != 0 )
    {
        if ( strcmp(cp->name,"BBR") == 0 )
        {
            if ( withdrawaddr == 0 || withdrawaddr[0] == 0 )
                return(BBR_clonetelepod(cp,refpod));
            else
            {
                retstr = BBR_withdrawtelepod(cp,refpod,withdrawaddr);
                if ( retstr != 0 )
                {
                    pod = calloc(1,sizeof(*pod));
                    safecopy(pod->txid,retstr,sizeof(pod->txid));
                    safecopy(pod->coinaddr,withdrawaddr,sizeof(pod->coinaddr));
                    free(retstr);
                }
                return(pod);
            }
        }
        refpods[0] = refpod;
        refpods[1] = 0;
        if ( refsatoshis != 0 )
        {
            printf("clone_telepod: unexpected nonzero %.8f refsatoshis\n",dstr(refsatoshis));
            return(0);
        }
        refsatoshis = refpod->satoshis;
        fee = calc_transporter_fee(cp,refsatoshis);
        if ( get_changepod(&changepod,cp) < fee || changepod == 0 )
        {
            printf("not enough changpod for %.8f\n",dstr(fee));
            if ( changepod != 0 )
                free(changepod);
            return(0);
        }
        didalloc = 1;
    }
    else fee = cp->txfee;
    if ( changepod != 0 )
    {
        availchange = changepod->satoshis;
        change_podaddr = changepod->coinaddr;
        change_privkey = changepod->privkey;
    }
    else
    {
        change_podaddr = get_transporter_unspent(inputpods,&availchange,cp);
        if ( change_podaddr == 0 || availchange < refsatoshis+fee )
        {
            printf("clone_telepod: cant get transporter addr || avail %.8f < %.8f + %.8f\n",dstr(availchange),dstr(refsatoshis),dstr(fee));
            return(0);
        }
        printf("fee (%.8f) changeaddr.(%s) availchange %.8f, refsatoshis %.8f\n",dstr(fee),change_podaddr,dstr(availchange),dstr(refsatoshis));
        availchange -= refsatoshis;
        printf("availchange %.8f refsatoshis %.8f\n",dstr(availchange),dstr(refsatoshis));
        //getchar();
    }
    if ( withdrawaddr[0] != 0 || (privkey= get_telepod_privkey(&podaddr,pubkey,cp)) != 0 )
    {
        if ( fee <= availchange )
        {
            change = (availchange - fee);
            memset(&RAW,0,sizeof(RAW));
            if ( (txid= calc_telepod_transaction(cp,&RAW,refpod!=0?refpods:inputpods,refsatoshis,podaddr,fee,changepod,change,change_podaddr)) == 0 )
            {
                if ( refpod != 0 )
                {
                    refpod->cloneout = TELEPOD_ERROR_VOUT;
                    update_telepod(refpod,refpod->txid);
                }
                printf("error cloning %.8f telepod.(%s) to %s\n",dstr(refsatoshis),refpod!=0?refpod->coinaddr:"transporter",podaddr);
            }
            else
            {
                if ( refpod != 0 )
                {
                    safecopy(refpod->cloneaddr,podaddr,sizeof(refpod->cloneaddr));
                    safecopy(refpod->clonetxid,txid,sizeof(refpod->clonetxid));
                    refpod->cloneout = TELEPOD_CONTENTS_VOUT;
                    printf("set refpod.%p (%s).vout%d\n",refpod,refpod->clonetxid,refpod->cloneout);
                    update_telepod(refpod,refpod->txid);
                }
                pod = create_telepod((uint32_t)time(NULL),cp->name,refsatoshis,podaddr,"",privkey,txid,TELEPOD_CONTENTS_VOUT);
                update_telepod(pod,pod->txid);
            }
            //if ( cp->enabled == 0 )
            //    cp->blockheight = (uint32_t)get_blockheight(cp), cp->enabled = 1;
            purge_rawtransaction(&RAW);
        } else printf("clone_telepod fee %llu change %llu\n",(long long)fee,(long long)availchange);
        if ( didalloc == 0 )
        {
            if ( change_privkey != 0 )
                free(change_privkey);
            if ( change_podaddr != 0 )
                free(change_podaddr);
        }
        else if ( changepod != 0 )
            free(changepod);
        if ( privkey != 0 )
            free(privkey);
        if ( podaddr != 0 && podaddr != withdrawaddr )
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
        if ( strcmp(cp->name,"BBR") == 0 )
            pod = BBR_maketelepod(cp,amount);
        else pod = clone_telepod(cp,0,amount,0);
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

double get_InstantDEX_rate(char *base,char *rel)
{
    if ( strcmp(base,rel) != 0 )
        return(0.);
    else return(1.);
}

double calc_convamount(char *base,char *rel,uint64_t satoshis)
{
    double rate;
    if ( (rate= get_InstantDEX_rate(base,rel)) != 0. )
        return(rate * ((double)satoshis / SATOSHIDEN));
    return(0.);
}

#define ADD_TELEPOD \
    { pod = calloc(1,data.size + key.size);\
    memcpy(pod,data.data,data.size);\
    memcpy(pod+data.size,key.data,key.size);\
    pods[n++] = pod;\
    if ( n >= max )\
    {\
        max += 100;\
        pods = (struct telepod **)realloc(pods,sizeof(*pods)*(max+1));\
    } }

struct telepod **available_telepods(int32_t *nump,double *availp,double *maturingp,double *inboundp,double *outboundp,double *doublespentp,double *cancelledp,char *coinstr,int32_t minage)
{
    uint32_t now = (uint32_t)time(NULL);
    DB *dbp = get_selected_database(TELEPOD_DATA);
    struct telepod *pod,**pods = 0;
    int32_t ret,max,m,n = 0;
    double evolve_amount;
    DBT key,data;
    DBC *cursorp = 0;
    *nump = 0;
    *availp = *maturingp = *inboundp = *outboundp = *doublespentp = *cancelledp = 0.;
    if ( dbp == 0 )
        return(0);
    max = num_in_db(TELEPOD_DATA);
    max += 100;
    m = 0;
    dbp->cursor(dbp,NULL,&cursorp,0);
    if ( cursorp != 0 )
    {
        clear_pair(&key,&data);
        pods = (struct telepod **)calloc(sizeof(*pods),max+1);
        while ( (ret= cursorp->get(cursorp,&key,&data,DB_NEXT)) == 0 )
        {
            m++;
            pod = data.data;
            if ( minage < 0 )
                ADD_TELEPOD
            evolve_amount = calc_convamount(pod->coinstr,coinstr,pod->satoshis);
            if ( evolve_amount == 0. )
            {
                clear_pair(&key,&data);
                continue;
            }
            if ( pod->podstate == TELEPOD_AVAIL )
            {
                if ( pod->createtime > (now - minage) )
                {
                    (*availp) += evolve_amount;
                    if ( minage >= 0 )
                        ADD_TELEPOD
                } else (*maturingp) += evolve_amount;
            }
            else if ( pod->podstate == TELEPOD_OUTBOUND ) // telepod is waiting to be cloned by destination
                (*outboundp) += evolve_amount;
            else if ( pod->podstate == TELEPOD_INBOUND ) // we are going to clone this within clonesmear timewindow
                (*inboundp) += evolve_amount;
            else if ( telepod_normal_spend(pod) == 0 ) // inbound -> cloned, outbound->spent
            {
                if ( pod->podstate == TELEPOD_DOUBLESPENT ) // sender of inbound telepod cashed in telepod before us
                    (*doublespentp) += evolve_amount;
                else if ( pod->podstate == TELEPOD_CANCELLED ) // we cashed the telepod before destination cloned
                    (*cancelledp) += evolve_amount;
            }
            clear_pair(&key,&data);
        }
        cursorp->close(cursorp);
    }
    //printf("find_closer_Kstored returns n.%d %p\n",n,sps);
    if ( m > num_in_db(TELEPOD_DATA) )
        set_num_in_db(TELEPOD_DATA,m);
    if ( pods != 0 )
        pods[n] = 0;
    *nump = n;
    return(pods);
}

struct telepod *process_telepathic_teleport(struct coin_info *cp,struct contact_info *contact,cJSON *tpd)
{
    char coinaddr[MAX_JSON_FIELD],podaddr[MAX_JSON_FIELD],script[MAX_JSON_FIELD],privkey[MAX_JSON_FIELD],txid[MAX_JSON_FIELD];
    struct telepod *pod = 0;
    uint32_t createtime,vout = -1;
    createtime = get_API_int(cJSON_GetObjectItem(tpd,"t"),0);
    extract_cJSON_str(txid,sizeof(txid),tpd,"x");
    extract_cJSON_str(privkey,sizeof(privkey),tpd,"p");
    if ( strcmp(cp->name,"BBR") != 0 )
    {
        vout = get_API_int(cJSON_GetObjectItem(tpd,"v"),-1);
        extract_cJSON_str(coinaddr,sizeof(coinaddr),tpd,"a");
        extract_cJSON_str(script,sizeof(script),tpd,"s");
    } else coinaddr[0] = script[0] = 0;
    pod = create_telepod(createtime,cp->name,0,podaddr,script,privkey,txid,vout);
    if ( get_telepod_info(&pod->unspent,&pod->createtime,cp->name,pod) < 0 )
    {
        printf("Invalid pod (%s) received from %s\n",pod->txid,contact->handle);
        free(pod);
        pod = 0;
    }
    else
    {
        pod->satoshis = pod->unspent;
        pod->senderbits = contact->nxt64bits;
        if ( pod->clonetime == 0 )
            pod->clonetime = (uint32_t)(((rand()>>8) % (1+cp->clonesmear)) + time(NULL));
        printf("pod (%s) received from %s for %.8f %s clonetime in %.1f minutes\n",pod->txid,contact->handle,dstr(pod->unspent),pod->coinstr,((double)pod->clonetime-time(NULL))/60.);
    }
    return(pod);
}

void telepathic_teleport(struct contact_info *contact,cJSON *attachjson)
{
    char coinstr[512];
    cJSON *tpd;
    struct coin_info *cp;
    struct telepod *pod = 0;
    if ( extract_cJSON_str(coinstr,sizeof(coinstr),attachjson,"c") > 0 && (tpd= cJSON_GetObjectItem(attachjson,"tpd")) != 0 )
    {
        if ( (cp= get_coin_info(coinstr)) != 0 )
        {
            pod = process_telepathic_teleport(cp,contact,tpd);
            if ( pod != 0 )
                update_telepod(pod,pod->txid);
        }
    }
}

int32_t poll_telepods(char *relstr)
{
    uint64_t unspent;
    uint32_t createtime,now;
    struct coin_info *cp;
    int32_t flag,i,err,n,m = 0;
    struct telepod **pods,*pod,*clonepod;
    double avail,inbound,outbound,maturing,doublespent,cancelled;
    pods = available_telepods(&n,&avail,&maturing,&inbound,&outbound,&doublespent,&cancelled,relstr,-1);
    if ( pods != 0 )
    {
        now = (uint32_t)time(NULL);
        for (i=0; i<n; i++)
        {
            if ( (pod= pods[i]) != 0 )
            {
                flag = err = 0;
                unspent = 0;
                createtime = 0;
                if ( pod->podstate == TELEPOD_AVAIL || pod->podstate == TELEPOD_INBOUND || pod->podstate == TELEPOD_OUTBOUND )
                    err = get_telepod_info(&unspent,&createtime,pod->coinstr,pod);
                if ( err == 0 )
                {
                    switch ( pod->podstate )
                    {
                        case TELEPOD_AVAIL:
                        case TELEPOD_INBOUND:
                            if ( unspent != pod->satoshis )
                            {
                                if ( unspent == 0 )
                                    flag = TELEPOD_DOUBLESPENT;
                                printf("Doublespend? txid.%s vout.%d satoshis %.8f vs %.8f\n",pod->txid,pod->vout,dstr(unspent),dstr(pod->satoshis));
                            }
                            else if ( pod->podstate == TELEPOD_INBOUND && pod->clonetime > now )
                            {
                                cp = get_coin_info(pod->coinstr);
                                if ( cp != 0 && (clonepod= clone_telepod(cp,pod,pod->satoshis,0)) != 0 )
                                {
                                    flag = TELEPOD_CLONED;
                                    free(clonepod);
                                }
                                else printf("error cloning %s %s.%d\n",pod->coinstr,pod->txid,pod->vout);
                            }
                            break;
                            break;
                        case TELEPOD_OUTBOUND:
                            if ( unspent == 0 )
                            {
                                flag = TELEPOD_SPENT;
                                printf("Cloned txid.%s vout.%d satoshis %.8f vs %.8f\n",pod->txid,pod->vout,dstr(unspent),dstr(pod->satoshis));
                            }
                            break;
                            // nothing left to do for these
                        case TELEPOD_DOUBLESPENT:
                        case TELEPOD_CANCELLED:
                        case TELEPOD_CLONED:
                        case TELEPOD_SPENT:
                            break;
                    }
                    if ( flag != 0 )
                    {
                        pod->podstate = flag;
                        update_telepod(pod,(char *)((long)pod + pod->H.datalen));
                    }
                }
                free(pod);
            }
        }
        free(pods);
    }
    return(m);
}

double calc_telepod_metric(struct telepod **pods,int32_t n,double target,uint32_t now)
{
    int32_t i;
    double metric;
    int64_t age,youngest,agesum = 0;
    double sum = 0.;
    youngest = -1;
    for (i=0; i<n; i++)
    {
        age = (now - pods[i]->createtime);
        agesum += age;
        if ( youngest < 0 || age < youngest )
            youngest = age;
        sum += pods[i]->evolve_amount;
    }
    agesum /= n;
    metric = n * sqrt((double)agesum / (youngest + 1)) + (target - sum + 1);
    printf("metric.%f youngest.%lld agesum %llu n %d, (%.8f - %.8f)\n",metric,(long long)youngest,(long long)agesum,n,target,sum);
    if ( sum < target || youngest < 0 )
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

struct telepod **evolve_podlist(int32_t *hwmnump,struct telepod **hwmpods,struct telepod **allpods,int32_t n,int32_t maxiters,double target)
{
    int32_t i,j,k,finished,numtestpods,nohwm,replacei,numhwm = *hwmnump;
    struct telepod **testlist;
    double metric,bestmetric;
    uint32_t now = (uint32_t)time(NULL);
    bestmetric = calc_telepod_metric(hwmpods,numhwm,target,now);
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
        replacei = ((rand()>>8) % numhwm);
        for (j=0; j<n; j++)
        {
            printf("i.%d of %d, replacei.%d j.%d inhwm.%d\n",i,maxiters,replacei,j,allpods[j]->inhwm);
            if ( allpods[j]->inhwm == 0 )
            {
                testlist[replacei] = allpods[j];
                numtestpods = numhwm;
                for (k=0; k<2; k++)
                {
                    metric = calc_telepod_metric(testlist,numtestpods,target,now);
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
                            testlist[(rand()>>8) % numtestpods] = testlist[numtestpods - 1];
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

struct telepod **evolve_telepods(int32_t *nump,int32_t maxiters,struct telepod **allpods,uint64_t satoshis)
{
    int32_t i,n,bestn = 0;
    double target,sum = 0.;
    struct telepod **hwmpods = 0;
    target = (double)satoshis / SATOSHIDEN;
    n = *nump;
    if ( allpods != 0 )
    {
        sum = 0;
        for (i=0; i<n; i++)
        {
            sum += allpods[i]->evolve_amount;
            if ( sum >= target )
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
            hwmpods = evolve_podlist(&bestn,hwmpods,allpods,n,maxiters,target);
        for (i=0; i<bestn; i++)
            disp_telepod("hwm",hwmpods[i]);
        free(allpods);
    }
    printf("-> evolved with bestn.%d %p\n",bestn,hwmpods);
    *nump = bestn;
    return(hwmpods);
}

char *teleport(char *contactstr,char *coinstr,uint64_t satoshis,int32_t minage,char *withdrawaddr)
{
    char buf[4096];
    struct telepod *pod,**pods = 0;
    struct contact_info *contact = 0;
    struct coin_info *cp = get_coin_info(coinstr);
    int32_t i,n;
    cJSON *attachmentjson;
    double avail,inbound,outbound,maturing,doublespent,cancelled;
    pods = available_telepods(&n,&avail,&maturing,&inbound,&outbound,&doublespent,&cancelled,coinstr,minage);
    sprintf(buf,"{\"result\":\"teleport %.8f %s minage.%d -> (%s)\",\"avail\":%.8f,\"inbound\":%.8f,\"outbound\":%.8f,\"maturing\":%.8f,\"doublespent\":%.8f,\"cancelled\":%.8f}",dstr(satoshis),coinstr,minage,contactstr,avail,inbound,outbound,maturing,doublespent,cancelled);
    if ( strcmp(contactstr,"balance") == 0 )
        return(clonestr(buf));
    contact = find_contact(contactstr);
    if ( (withdrawaddr[0] == 0 && contact == 0) || avail < satoshis || (satoshis % cp->min_telepod_satoshis) != 0 )
    {
        sprintf(buf,"{\"error\":\"cant find contact.(%s) or lack of telepods %.8f %s for %.8f\",\"modval\":\"%llu\"}",contactstr,avail,coinstr,dstr(satoshis),(long long)(satoshis % cp->min_telepod_satoshis));
        return(clonestr(buf));
    }
    printf("start evolving at %f\n",milliseconds());
    pods = evolve_telepods(&n,cp->maxevolveiters,pods,satoshis);
    printf("finished evolving at %f\n",milliseconds());
    if ( pods == 0 )
        sprintf(buf,"{\"error\":\"transporter evolve failure for %.8f %s to %s\"}",dstr(satoshis),coinstr,contactstr);
    else
    {
        for (i=0; i<n; i++)
        {
            if ( (pod= pods[i]) != 0 )
            {
                if ( (attachmentjson= telepod_json(pod)) != 0 )
                {
                    if ( withdrawaddr[0] != 0 )
                    {
                        pod->podstate = TELEPOD_WITHDRAWN;
                        update_telepod(pod,(char *)((long)pod + pod->H.datalen));
                    }
                    else
                    {
                        attachmentjson = telepathic_transmit(buf,contact,-1,"teleport",attachmentjson);
                        pod->podstate = TELEPOD_OUTBOUND;
                        update_telepod(pod,(char *)((long)pod + pod->H.datalen));
                        if ( attachmentjson != 0 )
                            free_json(attachmentjson);
                    }
                }
                free(pod);
            }
        }
        free(pods);
        if ( withdrawaddr[0] == 0 )
            sprintf(buf,"{\"results\":\"withdrew %.8f %s to %s\",\"num\":%d}",dstr(satoshis),coinstr,withdrawaddr,n);
        else sprintf(buf,"{\"results\":\"teleported %.8f %s to %s\",\"num\":%d}",dstr(satoshis),coinstr,contactstr,n);
    }
    return(clonestr(buf));
}

double filter_telepod(struct telepod *pod,struct contact_info *contact,char *coinstr,char *withdrawaddr)
{
    double net = 0.;
    if ( contact != 0 && pod->destbits != contact->nxt64bits && pod->senderbits != contact->nxt64bits )
        return(0.);
    if ( coinstr != 0 && strcmp(coinstr,pod->coinstr) != 0 )
        return(0.);
    if ( withdrawaddr != 0 && strcmp(withdrawaddr,pod->coinaddr) != 0 )
        return(0.);
    net = calc_convamount(pod->coinstr,(coinstr!=0&&coinstr[0]!=0)?coinstr:"BTC",pod->satoshis);
    return(net);
}

struct telepod *create_debitcredit(char *txidstr,char *cmd,char *contactstr,char *coinstr,uint64_t satoshis,char *comment)
{
    unsigned char tmp[32];
    struct telepod *pod = 0;
    randombytes(tmp,sizeof(tmp));
    init_hexbytes(txidstr,tmp,sizeof(tmp));
    pod = create_telepod((uint32_t)time(NULL),coinstr,satoshis,contactstr,cmd,comment,txidstr,-1);
    return(pod);
}

cJSON *telepod_dispjson(struct telepod *pod,double netamount)
{
    cJSON *dispjson = cJSON_CreateObject();
    int32_t dir = 0;
    char numstr[64];
    if ( strcmp(pod->script,"debit") == 0 )
        dir = -1;
    else if ( strcmp(pod->script,"credit") == 0 )
        dir = 1;
    if ( dir != 0 )
    {
        cJSON_AddItemToObject(dispjson,"type",cJSON_CreateString(pod->script));
        cJSON_AddItemToObject(dispjson,"contact",cJSON_CreateString(pod->coinaddr));
        cJSON_AddItemToObject(dispjson,"comment",cJSON_CreateString(pod->privkey));
    }
    else
    {
        cJSON_AddItemToObject(dispjson,"type",cJSON_CreateString(_podstate(pod->podstate)));
        if ( pod->senderbits != 0 )
        {
            sprintf(numstr,"%llu",(long long)pod->senderbits);
            cJSON_AddItemToObject(dispjson,"sender",cJSON_CreateString(numstr));
        }
        if ( pod->destbits != 0 )
        {
            sprintf(numstr,"%llu",(long long)pod->destbits);
            cJSON_AddItemToObject(dispjson,"dest",cJSON_CreateString(numstr));
        }
    }
    cJSON_AddItemToObject(dispjson,"txid",cJSON_CreateString(pod->txid));
    cJSON_AddItemToObject(dispjson,"coin",cJSON_CreateString(pod->coinstr));
    cJSON_AddItemToObject(dispjson,"amount",cJSON_CreateNumber(dir * (double)pod->satoshis/SATOSHIDEN));
    return(dispjson);
}

char *telepodacct(char *contactstr,char *coinstr,uint64_t amount,char *withdrawaddr,char *comment,char *cmd)
{
    char retbuf[MAX_JSON_FIELD],txidstr[MAX_JSON_FIELD],NXTaddr[64],*retstr;
    int32_t i,n,dir,numpos,numneg;
    struct contact_info *contact = 0;
    cJSON *json,*array,*item;
    struct telepod **pods,*pod;
    double avail,inbound,outbound,maturing,doublespent,cancelled,credits,debits,net;
    if ( strcmp(cmd,"debit") == 0 )
        dir = -1;
    else if ( strcmp(cmd,"credit") == 0 )
        dir = 1;
    else dir = 0;
    if ( dir != 0 )
    {
        if ( contactstr != 0 && contactstr[0] != 0 && amount != 0 && coinstr[0] != 0 && withdrawaddr[0] == 0 )
        {
            pod = create_debitcredit(txidstr,cmd,contactstr,coinstr,amount,comment);
            update_telepod(pod,txidstr);
            item = telepod_dispjson(pod,((double)dir * amount) / SATOSHIDEN);
            free(pod);
            retstr = cJSON_Print(item);
            stripwhite_ns(retstr,strlen(retstr));
            return(retstr);
        } else return(clonestr("{\"error\":\"illegal debit/credit parameter\"}"));
    }
    pods = available_telepods(&n,&avail,&maturing,&inbound,&outbound,&doublespent,&cancelled,coinstr,-1);
    sprintf(retbuf,"{\"result\":\"telepodacct %.8f %s \",\"avail\":%.8f,\"inbound\":%.8f,\"outbound\":%.8f,\"maturing\":%.8f,\"doublespent\":%.8f,\"cancelled\":%.8f}",dstr(amount),coinstr,avail,inbound,outbound,maturing,doublespent,cancelled);
    if ( pods != 0 )
    {
        json = cJSON_CreateObject();
        array = cJSON_CreateArray();
        contact = find_contact(contactstr);
        if ( contact != 0 )
        {
            cJSON_AddItemToObject(json,"contact",cJSON_CreateString(contact->handle));
            expand_nxt64bits(NXTaddr,contact->nxt64bits);
            cJSON_AddItemToObject(json,"acct",cJSON_CreateString(NXTaddr));
        }
        if ( withdrawaddr[0] != 0 )
            cJSON_AddItemToObject(json,"withdrawaddr",cJSON_CreateString(withdrawaddr));
        if ( coinstr[0] != 0 )
            cJSON_AddItemToObject(json,"coin",cJSON_CreateString(coinstr));
        credits = debits = 0.;
        numpos = numneg = 0;
        for (i=0; i<n; i++)
        {
            if ( (pod= pods[i]) != 0 )
            {
                if ( (net= filter_telepod(pod,contact,coinstr,withdrawaddr)) != 0. )
                {
                    if ( net > 0. )
                    {
                        numpos++;
                        credits += net;
                    }
                    else if ( net < 0. )
                    {
                        numneg++;
                        debits -= net;
                    }
                    item = telepod_dispjson(pod,net);
                    cJSON_AddItemToArray(array,item);
                }
                free(pod);
            }
        }
        cJSON_AddItemToObject(json,"telepods",array);
        cJSON_AddItemToObject(json,"numdebits",cJSON_CreateNumber(numneg));
        cJSON_AddItemToObject(json,"debits",cJSON_CreateNumber(debits));
        cJSON_AddItemToObject(json,"numcredits",cJSON_CreateNumber(numpos));
        cJSON_AddItemToObject(json,"credits",cJSON_CreateNumber(credits));
        retstr = cJSON_Print(json);
        stripwhite_ns(retstr,strlen(retstr));
        return(retstr);
    }
    return(clonestr(retbuf));
}

#endif
