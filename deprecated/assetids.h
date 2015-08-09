//
//  assetids.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_assetids_h
#define xcode_assetids_h

#define INSTANTDEX_NATIVE 0
#define INSTANTDEX_ASSET 1
#define INSTANTDEX_MSCOIN 2
#define INSTANTDEX_UNKNOWN 3

char *MGWassets[][3] =
{
    { "17554243582654188572", "BTC", "8" }, // assetid, name, decimals
    { "4551058913252105307", "BTC", "8" },
    { "12659653638116877017", "BTC", "8" },
    { "11060861818140490423", "BTCD", "4" },
    { "6918149200730574743", "BTCD", "4" },
    { "13120372057981370228", "BITS", "6" },
    { "2303962892272487643", "DOGE", "4" },
    { "16344939950195952527", "DOGE", "4" },
    { "6775076774325697454", "OPAL", "8" },
    { "7734432159113182240", "VPN", "4" },
    { "9037144112883608562", "VRC", "8" },
    { "1369181773544917037", "BBR", "8" },
    { "17353118525598940144", "DRK", "8" },
    { "2881764795164526882", "LTC", "8" },
    { "7117580438310874759", "BC", "4" },
    { "275548135983837356", "VIA", "4" },
    { "6220108297598959542", "CNMT", "0" },
    { "7474435909229872610", "CNMT", "0" },
};

char *identicalassets[][8] = { { "7474435909229872610", "6220108297598959542" } };

char *bterassets[][8] = { { "UNITY", "12071612744977229797" },  { "ATOMIC", "11694807213441909013" },  { "DICE", "18184274154437352348" },  { "MRKT", "134138275353332190" },  { "MGW", "10524562908394749924" } };
char *poloassets[][8] = { { "UNITY", "12071612744977229797" },  { "JLH", "6932037131189568014" },  { "XUSD", "12982485703607823902" },  { "LQD", "4630752101777892988" },  { "NXTI", "14273984620270850703" }, { "CNMT", "7474435909229872610", "6220108297598959542" } };

int32_t unstringbits(char *buf,uint64_t bits)
{
    int32_t i;
    for (i=0; i<8; i++,bits>>=8)
        if ( (buf[i]= (char)(bits & 0xff)) == 0 )
            break;
    buf[i] = 0;
    return(i);
}

int32_t get_equivalent_assetids(uint64_t *equivids,uint64_t bits)
{
    char name[16],str[64];
    int i,j,matchi,n = 0;
    memset(name,0,sizeof(name));
    unstringbits(name,bits);
    expand_nxt64bits(str,bits);
    for (matchi=-1,i=0; i<(int32_t)(sizeof(MGWassets)/sizeof(*MGWassets)); i++)
    {
        if ( strcmp(MGWassets[i][1],name) == 0 )
            equivids[n++] = calc_nxt64bits(MGWassets[i][0]);
        if ( strcmp(MGWassets[i][0],str) == 0 )
            matchi = i;
    }
    if ( matchi >= 0 )
    {
        strcpy(name,MGWassets[matchi][1]);
        for (i=0; i<(int32_t)(sizeof(MGWassets)/sizeof(*MGWassets)); i++)
            if ( strcmp(MGWassets[i][1],name) == 0 )
                equivids[n++] = calc_nxt64bits(MGWassets[i][0]);
    }
    for (matchi=-1,i=0; i<(int32_t)(sizeof(identicalassets)/sizeof(*identicalassets)); i++)
    {
        for (matchi=-1,j=0; j<(int32_t)(sizeof(identicalassets[0])/sizeof(*identicalassets[0]))&&identicalassets[i][j]!=0 ; j++)
        {
            if ( strcmp(identicalassets[i][j],str) == 0 )
            {
                matchi = j;
                break;
            }
        }
        if ( matchi >= 0 )
        {
            for (j=0; j<(int32_t)(sizeof(identicalassets[0])/sizeof(*identicalassets[0]))&&identicalassets[i][j]!=0 ; j++)
            {
                if ( j != matchi )
                    equivids[n++] = calc_nxt64bits(identicalassets[i][j]);
            }
            break;
        }
    }
    /*if ( bits == calc_nxt64bits("6220108297598959542") || bits == calc_nxt64bits("7474435909229872610") ) // coinomat
    {
        equivids[n++] = calc_nxt64bits("6220108297598959542");
        equivids[n++] = calc_nxt64bits("7474435909229872610");
    }*/
    if ( n == 0 )
        equivids[n++] = bits;
    return(n);
}

uint64_t is_cryptocoin(char *name)
{
    int32_t i;
    for (i=0; i<(int32_t)(sizeof(MGWassets)/sizeof(*MGWassets)); i++)
        if ( strcmp(MGWassets[i][1],name) == 0 )
            return(calc_nxt64bits(MGWassets[i][0]));
    return(0);
}

int32_t is_native_crypto(char *name,uint64_t bits)
{
    int32_t i,n;
    if ( (n= unstringbits(name,bits)) <= 5 )
    {
        for (i=0; i<n; i++)
        {
            if ( (name[i] >= '0' && name[i] <= '9') || (name[i] >= 'A' && name[i] <= 'Z') )// || (name[i] >= '0' && name[i] <= '9') )
                continue;
            //printf("(%s) is not native crypto\n",name);
            return(0);
        }
        //printf("(%s) is native crypto\n",name);
        return(1);
    }
    return(0);
}

uint64_t _calc_decimals_mult(int32_t decimals)
{
    int32_t i;
    uint64_t mult = 1;
    for (i=7-decimals; i>=0; i--)
        mult *= 10;
    //printf("_calc_decimals_mult(%d) -> %llu\n",decimals,(long long)mult);
    return(mult);
}

int32_t _set_assetname(uint64_t *multp,char *buf,char *jsonstr)
{
    int32_t decimals = -1;
    cJSON *json;
    *multp = 0;
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        if ( get_cJSON_int(json,"errorCode") == 0 )
        {
            decimals = (int32_t)get_cJSON_int(json,"decimals");
            if ( decimals >= 0 && decimals <= 8 )
                *multp = _calc_decimals_mult(decimals);
            if ( extract_cJSON_str(buf,16,json,"name") <= 0 )
                decimals = -1;
        }
        free_json(json);
    }
    return(decimals);
}

uint32_t set_assetname(uint64_t *multp,char *name,uint64_t assetbits)
{
    char assetstr[64],buf[MAX_JSON_FIELD],*jsonstr,*jsonstr2;
    uint32_t i,retval = INSTANTDEX_UNKNOWN;
    *multp = 1;
    if ( assetbits == NXT_ASSETID )
    {
        *multp = 1;
        strcpy(name,"NXT");
        return(INSTANTDEX_NATIVE); // native crypto type
    }
    else if ( is_native_crypto(name,assetbits) > 0 )
        return(INSTANTDEX_NATIVE); // native crypto type
    expand_nxt64bits(assetstr,assetbits);
    strcpy(name,"unknown");
    for (i=0; i<(int32_t)(sizeof(MGWassets)/sizeof(*MGWassets)); i++)
    {
        if ( strcmp(MGWassets[i][0],assetstr) == 0 )
        {
            strcpy(name,MGWassets[i][1]);
            *multp = _calc_decimals_mult(atoi(MGWassets[i][2]));
            //printf("SETASSETNAME.(%s) <- %s mult.%llu\n",name,assetstr,(long long)*multp);
            return(INSTANTDEX_NATIVE); // native crypto type
        }
    }
    if ( (jsonstr= issue_getAsset(0,assetstr)) != 0 )
    {
        //  printf("set assetname for (%s)\n",jsonstr);
        if ( _set_assetname(multp,buf,jsonstr) < 0 )
        {
            if ( (jsonstr2= issue_getAsset(1,assetstr)) != 0 )
            {
                if ( _set_assetname(multp,buf,jsonstr) >= 0 )
                {
                    retval = INSTANTDEX_MSCOIN;
                    strncpy(name,buf,15);
                    name[15] = 0;
                }
                free(jsonstr2);
            }
        }
        else
        {
            retval = INSTANTDEX_ASSET;
            strncpy(name,buf,15);
            name[15] = 0;
        }
        free(jsonstr);
    }
    return(retval);
}

uint64_t min_asset_amount(uint64_t assetid)
{
    struct NXT_asset *ap;
    int32_t createdflag;
    char assetidstr[64];
    if ( assetid == NXT_ASSETID )
        return(1);
    expand_nxt64bits(assetidstr,assetid);
    ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
    return(ap->mult);
}

int32_t get_assetdecimals(uint64_t assetid)
{
    struct NXT_asset *ap;
    int32_t createdflag;
    char assetidstr[64];
    if ( assetid == NXT_ASSETID )
        return(8);
    expand_nxt64bits(assetidstr,assetid);
    ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
    return(ap->decimals);
}

uint64_t calc_assetoshis(uint64_t assetidbits,uint64_t amount)
{
    struct NXT_asset *ap;
    int32_t createdflag;
    uint64_t assetoshis = 0;
    char assetidstr[64];
    printf("calc_assetoshis %.8f %llu\n",dstr(amount),(long long)assetidbits);
    if ( assetidbits == 0 || assetidbits == NXT_ASSETID )
        return(amount);
    expand_nxt64bits(assetidstr,assetidbits);
    ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
    if ( ap->mult != 0 )
        assetoshis = amount / ap->mult;
    else
    {
        printf("FATAL asset.%s has no mult\n",assetidstr);
        exit(-1);
    }
    printf("amount %llu -> assetoshis.%llu\n",(long long)amount,(long long)assetoshis);
    return(assetoshis);
}

uint64_t issue_getBalance(CURL *curl_handle,char *NXTaddr)
{
    char cmd[4096];
    union NXTtype ret;
    sprintf(cmd,"%s=getBalance&account=%s",_NXTSERVER,NXTaddr);
    ret = extract_NXTfield(curl_handle,0,cmd,"balanceNQT",64);
    return(ret.nxt64bits);
}

int64_t get_asset_quantity(int64_t *unconfirmedp,char *NXTaddr,char *assetidstr)
{
    char cmd[2*MAX_JSON_FIELD],assetid[MAX_JSON_FIELD],*jsonstr;
    union NXTtype retval;
    int32_t i,n,iter;
    cJSON *array,*item,*obj,*json;
    uint64_t assetidbits = calc_nxt64bits(assetidstr);
    int64_t quantity,qty = 0;
    quantity = *unconfirmedp = 0;
    if ( assetidbits == NXT_ASSETID )
    {
        sprintf(cmd,"requestType=getBalance&account=%s",NXTaddr);
        if ( (jsonstr= issue_NXTPOST(0,cmd)) != 0 )
        {
            if ( (json= cJSON_Parse(jsonstr)) != 0 )
            {
                qty = get_API_nxt64bits(cJSON_GetObjectItem(json,"balanceNQT"));
                *unconfirmedp = get_API_nxt64bits(cJSON_GetObjectItem(json,"unconfirmedBalanceNQT"));
                printf("(%s)\n",jsonstr);
                free_json(json);
            }
            free(jsonstr);
        }
        return(qty);
    }
    sprintf(cmd,"%s=getAccount&account=%s",_NXTSERVER,NXTaddr);
    retval = extract_NXTfield(0,0,cmd,0,0);
    if ( retval.json != 0 )
    {
        for (iter=0; iter<2; iter++)
        {
            qty = 0;
            array = cJSON_GetObjectItem(retval.json,iter==0?"assetBalances":"unconfirmedAssetBalances");
            if ( is_cJSON_Array(array) != 0 )
            {
                n = cJSON_GetArraySize(array);
                for (i=0; i<n; i++)
                {
                    item = cJSON_GetArrayItem(array,i);
                    obj = cJSON_GetObjectItem(item,"asset");
                    copy_cJSON(assetid,obj);
                    //printf("i.%d of %d: %s(%s)\n",i,n,assetid,cJSON_Print(item));
                    if ( strcmp(assetid,assetidstr) == 0 )
                    {
                        qty = get_cJSON_int(item,iter==0?"balanceQNT":"unconfirmedBalanceQNT");
                        break;
                    }
                }
            }
            if ( iter == 0 )
                quantity = qty;
            else *unconfirmedp = qty;
        }
    }
    return(quantity);
}

uint64_t calc_asset_qty(uint64_t *availp,uint64_t *priceNQTp,char *NXTaddr,int32_t checkflag,uint64_t assetid,double price,double vol)
{
    char assetidstr[64];
    struct NXT_asset *ap;
    int32_t createdflag;
    uint64_t priceNQT,quantityQNT = 0;
    int64_t unconfirmed,balance;
    *priceNQTp = *availp = 0;
    if ( assetid != NXT_ASSETID )
    {
        expand_nxt64bits(assetidstr,assetid);
        ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
        if ( ap->mult != 0 )
        {
            //price = (double)get_satoshi_obj(srcitem,"priceNQT") / ap_mult;
            //vol = (double)get_satoshi_obj(srcitem,"quantityQNT") * ((double)ap_mult / SATOSHIDEN);
            priceNQT = (price * ap->mult + ap->mult/2);
            quantityQNT = (vol * SATOSHIDEN) / ap->mult;
            balance = get_asset_quantity(&unconfirmed,NXTaddr,assetidstr);
            printf("%s balance %.8f unconfirmed %.8f vs price %llu qty %llu for asset.%s | price_vol.(%f * %f) * (%ld / %llu)\n",NXTaddr,dstr(balance),dstr(unconfirmed),(long long)priceNQT,(long long)quantityQNT,assetidstr,price,vol,SATOSHIDEN,(long long)ap->mult);
            //getchar();
            if ( checkflag != 0 && (balance < quantityQNT || unconfirmed < quantityQNT) )
            {
                printf("balance %.8f < qty %.8f || unconfirmed %.8f < qty %llu\n",dstr(balance),dstr(quantityQNT),dstr(unconfirmed),(long long)quantityQNT);
                return(0);
            }
            *priceNQTp = priceNQT;
            *availp = unconfirmed;
        } else printf("%llu null apmult\n",(long long)assetid);
    }
    else
    {
        *priceNQTp = price * SATOSHIDEN;
        quantityQNT = vol;
    }
    return(quantityQNT);
}

double _calc_price_volume(double *volumep,uint64_t baseamount,uint64_t relamount)
{
    *volumep = (((double)baseamount + 0.000000009999999) / SATOSHIDEN);
    if ( baseamount > 0. )
        return((double)relamount / (double)baseamount);
    else return(0.);
}

void set_best_amounts(uint64_t *baseamountp,uint64_t *relamountp,double price,double volume)
{
    double checkprice,checkvol,distA,distB,metric,bestmetric = (1. / SMALLVAL);
    uint64_t baseamount,relamount,bestbaseamount = 0,bestrelamount = 0;
    int32_t i,j;
    baseamount = volume * SATOSHIDEN;
    relamount = ((price * volume) * SATOSHIDEN);
    //*baseamountp = baseamount, *relamountp = relamount;
    //return;
    for (i=-1; i<=1; i++)
        for (j=-1; j<=1; j++)
        {
            checkprice = _calc_price_volume(&checkvol,baseamount+i,relamount+j);
            distA = (checkprice - price);
            distA *= distA;
            distB = (checkvol - volume);
            distB *= distB;
            metric = sqrt(distA + distB);
            if ( metric < bestmetric )
            {
                bestmetric = metric;
                bestbaseamount = baseamount + i;
                bestrelamount = relamount + j;
                //printf("i.%d j.%d metric. %f\n",i,j,metric);
            }
        }
    *baseamountp = bestbaseamount;
    *relamountp = bestrelamount;
}

double calc_price_volume(double *volumep,uint64_t baseamount,uint64_t relamount)
{
    double price,vol;
    uint64_t checkbase,checkrel;
    *volumep = vol = (((double)baseamount + 0.00000000999999999) / SATOSHIDEN);
    if ( baseamount > 0. )
        price = ((double)relamount / (double)baseamount);
    else return(0.);
    set_best_amounts(&checkbase,&checkrel,price,vol);
    if ( checkbase != baseamount || checkrel != relamount )
        printf("calc_price_volume error: (%llu/%llu) -> %f %f -> (%llu %llu)\n",(long long)baseamount,(long long)relamount,price,vol,(long long)checkbase,(long long)checkrel);
    return(price);
}

uint64_t get_assetmult(uint64_t assetid)
{
    struct NXT_asset *ap;
    char assetidstr[64];
    int32_t createdflag;
    expand_nxt64bits(assetidstr,assetid);
    ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
    //printf("ap->mult %llu\n",(long long)ap->mult);getchar();
    return(ap->mult);
}

double get_minvolume(uint64_t assetid)
{
    return(dstr(get_assetmult(assetid)));
}

uint64_t calc_baseamount(uint64_t *relamountp,uint64_t assetid,uint64_t qty,uint64_t priceNQT)
{
    *relamountp = (qty * priceNQT);
    return(qty * get_assetmult(assetid));
}

double check_ratios(uint64_t baseamount,uint64_t relamount,uint64_t baseamount2,uint64_t relamount2)
{
    double price,price2,vol,vol2;
    if ( baseamount != baseamount2 || relamount != relamount2 )
    {
        price = calc_price_volume(&vol,baseamount,relamount);
        price2 = calc_price_volume(&vol2,baseamount2,relamount2);
        if ( vol2 > vol )
        {
            printf("check_ratios: increased volumes? %f -> %f\n",vol,vol2);
            return(-1);
        }
        if ( price != 0. && price2 != 0. )
            return(price / price2);
        else return(0.);
    } return(1.);
}

double make_jumpquote(uint64_t baseid,uint64_t relid,uint64_t *baseamountp,uint64_t *relamountp,uint64_t *frombasep,uint64_t *fromrelp,uint64_t *tobasep,uint64_t *torelp)
{
    double p0,v0,p1,v1,price,vol,checkprice,checkvol,ratio;
    *baseamountp = *relamountp = 0;
    p0 = calc_price_volume(&v0,*tobasep,*torelp);
    p1 = calc_price_volume(&v1,*frombasep,*fromrelp);
    if ( p0 > 0. && p1 > 0. && v0 >= get_minvolume(baseid) && v1 >= get_minvolume(relid) )
    {
        price = (p1 / p0);
        vol = ((v0 * p0) / p1);
        ratio = (v1 / (vol * price));
        set_best_amounts(baseamountp,relamountp,price,vol);
        if ( p0*v0 > p1*v1 )
        {
            ratio = (p1 * v1) / (p0 * v0);
            *tobasep *= ratio, *torelp *= ratio;
        }
        else
        {
            ratio = (p0 * v0) / (p1 * v1);
            *frombasep *= ratio, *fromrelp *= ratio;
        }
        //*baseamountp *= ratio, *relamountp *= ratio;
        if ( Debuglevel > 2 )
            printf("[%llu/%llu] price %f (p1 %f / p0 %f), vol %f = (v0 %f * p0 %f) / p1 %f ratio %f\n",(long long)*baseamountp,(long long)*relamountp,price,p1,p0,vol,v0,p0,p1,ratio);
        checkprice = calc_price_volume(&checkvol,*baseamountp,*relamountp);
        if ( Debuglevel > 2 )
            printf("from.(%llu/%llu) to.(%llu %llu) -> (%llu/%llu) ratio %f price %f vol %f (%f %f)\n",(long long)*frombasep,(long long)*fromrelp,(long long)*tobasep,(long long)*torelp,(long long)*baseamountp,(long long)*relamountp,ratio,price,vol,checkprice,checkvol);
        //*baseamountp = vol * SATOSHIDEN;
        // *relamountp = ((price * vol) * SATOSHIDEN);
        //printf("make_jumpquote: v0 %f * %f p0 = %f, %f = v1 %f * p1 %f -> %f %f %llu/%llu = %f\n",v0,p0,v0*p0,v1*p1,v1,p1,p,v,(long long)*baseamountp,(long long)*relamountp,1./p);
        //set_best_amounts(baseamountp,relamountp,price,vol);
        //printf("make_jumpquote2: v0 * p0 = %f, %f = v1 * p1 -> %f %f %llu/%llu = %f\n",v0*p0,v1*p1,p,v,(long long)*baseamountp,(long long)*relamountp,1./p);
        return(price);
    } else return(0.);
}

int32_t is_unfunded_order(uint64_t nxt64bits,uint64_t assetid,uint64_t amount)
{
    int32_t createdflag;
    char assetidstr[64],NXTaddr[64],cmd[1024],*jsonstr;
    struct NXT_asset *ap;
    int64_t unconfirmed,balance = 0;
    cJSON *json;
    expand_nxt64bits(NXTaddr,nxt64bits);
    if ( assetid == NXT_ASSETID )
    {
        sprintf(cmd,"requestType=getAccount&account=%s",NXTaddr);
        if ( (jsonstr= issue_NXTPOST(0,cmd)) != 0 )
        {
            if ( (json= cJSON_Parse(jsonstr)) != 0 )
            {
                balance = get_API_nxt64bits(cJSON_GetObjectItem(json,"balanceNQT"));
                free_json(json);
            }
            free(jsonstr);
        }
        strcpy(assetidstr,"NXT");
    }
    else
    {
        expand_nxt64bits(assetidstr,assetid);
        ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
        if ( ap->mult != 0 )
        {
            expand_nxt64bits(NXTaddr,nxt64bits);
            balance = ap->mult * get_asset_quantity(&unconfirmed,NXTaddr,assetidstr);
        }
    }
    if ( balance < amount )
    {
        printf("balance %.8f < amount %.8f for asset.%s\n",dstr(balance),dstr(amount),assetidstr);
        return(1);
    }
    return(0);
}

#endif
