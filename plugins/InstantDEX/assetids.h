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

char *MGWassets[][3] =
{
    { "12659653638116877017", "BTC", "8" },
    { "17554243582654188572", "BTC", "8" }, // assetid, name, decimals
    { "4551058913252105307", "BTC", "8" },
    { "6918149200730574743", "BTCD", "4" },
    { "11060861818140490423", "BTCD", "4" },
    { "13120372057981370228", "BITS", "6" },
    { "16344939950195952527", "DOGE", "4" },
    { "2303962892272487643", "DOGE", "4" },
    { "6775076774325697454", "OPAL", "8" },
    { "7734432159113182240", "VPN", "4" },
    { "9037144112883608562", "VRC", "8" },
    { "1369181773544917037", "BBR", "8" },
    { "17353118525598940144", "DRK", "8" },
    { "2881764795164526882", "LTC", "4" },
    { "7117580438310874759", "BC", "4" },
    { "275548135983837356", "VIA", "4" },
    { "6220108297598959542", "CNMT", "0" },
    { "7474435909229872610", "CNMT", "0" },
};

uint64_t is_MGWcoin(char *name)
{
    int32_t i;
    for (i=0; i<(int32_t)(sizeof(MGWassets)/sizeof(*MGWassets)); i++)
        if ( strcmp(MGWassets[i][1],name) == 0 )
            return(calc_nxt64bits(MGWassets[i][0]));
    return(0);
}

char *is_MGWasset(uint64_t assetid)
{
    int32_t i; char assetidstr[64];
    expand_nxt64bits(assetidstr,assetid);
    for (i=0; i<(int32_t)(sizeof(MGWassets)/sizeof(*MGWassets)); i++)
        if ( strcmp(MGWassets[i][0],assetidstr) == 0 )
            return(MGWassets[i][1]);
    return(0);
}

int32_t is_native_crypto(char *name,uint64_t bits)
{
    int32_t i,n;
    if ( (n= (int32_t)strlen(name)) > 0 || (n= unstringbits(name,bits)) <= 5 )
    {
        for (i=0; i<n; i++)
        {
            if ( (name[i] >= '0' && name[i] <= '9') || (name[i] >= 'A' && name[i] <= 'Z') )// || (name[i] >= '0' && name[i] <= '9') )
                continue;
            printf("(%s) is not native crypto\n",name);
            return(0);
        }
        printf("(%s) is native crypto\n",name);
        return(1);
    }
    return(0);
}

int32_t add_exchange_assetid(uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid,int32_t exchangeid)
{
    int32_t i;
    if ( n > 0 )
    {
        for (i=0; i<n; i+=3)
        {
            if ( baseid == assetids[i] && relid == assetids[i+1] && exchangeid == assetids[i+2] )
            {
                //printf("SKIP: %llu %llu %d %s\n",(long long)baseid,(long long)relid,exchangeid,Exchanges[exchangeid].name);
                return(n);
            }
        }
    }
    if ( Debuglevel > 2 )
        printf("ADD: %llu %llu %d %s\n",(long long)baseid,(long long)relid,exchangeid,exchange_str(exchangeid));
    assetids[n++] = baseid, assetids[n++] = relid, assetids[n++] = exchangeid;
    return(n);
}

int32_t _add_related(uint64_t *assetids,int32_t n,uint64_t refbaseid,uint64_t refrelid,int32_t exchangeid,uint64_t baseid,uint64_t relid)
{
    if ( baseid == refbaseid || baseid == refrelid || relid == refbaseid || relid == refrelid )
        n = add_exchange_assetid(assetids,n,refbaseid,refrelid,exchangeid);
    return(n);
}

int32_t add_related(uint64_t *assetids,int32_t n,uint64_t supported[][2],long num,int32_t exchangeid,uint64_t baseid,uint64_t relid)
{
    int32_t i;
    for (i=0; i<num; i++)
        n = _add_related(assetids,n,supported[i][0],supported[i][1],exchangeid,baseid,relid);
    return(n);
}

int32_t add_exchange_symbolmap(uint64_t *assetids,int32_t n,uint64_t refbits,uint64_t assetid,int32_t exchangeid,char *symbolmap[][8],int32_t numsymbols)
{
    int32_t i,j;
    char assetidstr[64];
    uint64_t namebits;
    expand_nxt64bits(assetidstr,assetid);
    for (i=0; i<numsymbols; i++)
    {
        namebits = stringbits(symbolmap[i][0]);
        for (j=1; j<8; j++)
        {
            if ( symbolmap[i][j] == 0 || symbolmap[i][j][0] == 0 )
                break;
            //printf("{%s %s} ",symbolmap[i][j],assetidstr);
            if ( strcmp(symbolmap[i][j],assetidstr) == 0 )
                n = add_exchange_assetid(assetids,n,namebits,refbits,exchangeid);
        }
    }
    //printf("(assetsearch.%s) numsymbols.%d\n",assetidstr,numsymbols);
    return(n);
}

int32_t add_exchange_assetids(uint64_t *assetids,int32_t n,uint64_t refassetid,uint64_t baseid,uint64_t relid,int32_t exchangeid,char *symbolmap[][8],int32_t numsymbols)
{
    char name[64];
    //printf("(%s) ref.%llu (%llu/%llu) numsymbols.%d\n",Exchanges[exchangeid].name,(long long)refassetid,(long long)baseid,(long long)relid,numsymbols);
    if ( baseid != refassetid )
    {
        if ( symbolmap != 0 && numsymbols > 0 )
            n = add_exchange_symbolmap(assetids,n,refassetid,baseid,exchangeid,symbolmap,numsymbols);
        if ( is_native_crypto(name,baseid) > 0 )
            n = add_exchange_assetid(assetids,n,baseid,refassetid,exchangeid);
    }
    if ( relid != refassetid )
    {
        if ( symbolmap != 0 && numsymbols > 0 )
            n = add_exchange_symbolmap(assetids,n,refassetid,relid,exchangeid,symbolmap,numsymbols);
        if ( is_native_crypto(name,relid) > 0 )
            n = add_exchange_assetid(assetids,n,relid,refassetid,exchangeid);
    }
    return(n);
}

int32_t add_NXT_assetids(uint64_t *assetids,int32_t n,uint64_t assetid)
{
    int32_t i,m;
    uint64_t equivids[256];
    if ( (m= get_equivalent_assetids(equivids,assetid)) > 0 )
    {
        for (i=0; i<m; i++)
            n = add_exchange_assetid(assetids,n,assetid,NXT_ASSETID,INSTANTDEX_NXTAEID);
    }
    else n = add_exchange_assetid(assetids,n,assetid,NXT_ASSETID,INSTANTDEX_NXTAEID);
    return(n);
}

uint32_t set_assetname(uint64_t *multp,char *name,uint64_t assetbits)
{
    char assetstr[64]; struct assethash *ap; int32_t decimal,ap_type; uint32_t i,retval = INSTANTDEX_UNKNOWN;
    if ( multp != 0 )
        *multp = 1;
    if ( assetbits == NXT_ASSETID )
    {
        if ( multp != 0 )
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
            if ( multp != 0 )
                *multp = calc_decimals_mult(myatoi(MGWassets[i][2],9));
            //printf("SETASSETNAME.(%s) <- %s mult.%llu\n",name,assetstr,(long long)*multp);
            return(INSTANTDEX_NATIVE); // native crypto type
        }
    }
    if ( (ap_type= get_assettype(&decimal,assetstr)) != 0 )
    {
        if ( (ap= find_asset(assetbits)) != 0 )
        {
            *multp = ap->mult;
            strcpy(name,ap->name);
            if ( ap_type == 2 )
                retval = INSTANTDEX_ASSET;
            else if ( ap_type == 5 )
                retval = INSTANTDEX_MSCOIN;
        }
    } else *multp = 1, strcpy(name,"NXT"), retval = INSTANTDEX_NATIVE;
   /*if ( (jsonstr= _issue_getAsset(assetstr)) != 0 )
    {
        //  printf("set assetname for (%s)\n",jsonstr);
        if ( _set_assetname(multp,buf,jsonstr) < 0 )
        {
            if ( (jsonstr2= _issue_getCurrency(assetstr)) != 0 )
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
    }*/
    return(retval);
}

uint64_t calc_assetoshis(uint64_t assetidbits,uint64_t amount)
{
    uint64_t ap_mult,assetoshis = 0;
    char assetidstr[64];
    printf("calc_assetoshis %.8f %llu\n",dstr(amount),(long long)assetidbits);
    if ( assetidbits == 0 || assetidbits == NXT_ASSETID )
        return(amount);
    expand_nxt64bits(assetidstr,assetidbits);
    if ( (ap_mult= assetmult(assetidstr)) != 0 )
        assetoshis = amount / ap_mult;
    else
    {
        printf("FATAL asset.%s has no mult\n",assetidstr);
        exit(-1);
    }
    printf("amount %llu -> assetoshis.%llu\n",(long long)amount,(long long)assetoshis);
    return(assetoshis);
}

uint64_t calc_asset_qty(uint64_t *availp,uint64_t *priceNQTp,char *NXTaddr,int32_t checkflag,uint64_t assetid,double price,double vol)
{
    char assetidstr[64];
    uint64_t ap_mult,priceNQT,quantityQNT = 0;
    int64_t unconfirmed,balance;
    *priceNQTp = *availp = 0;
    if ( assetid != NXT_ASSETID )
    {
        expand_nxt64bits(assetidstr,assetid);
        if ( (ap_mult= get_assetmult(assetid)) != 0 )
        {
            //price = (double)get_satoshi_obj(srcitem,"priceNQT") / ap_mult;
            //vol = (double)get_satoshi_obj(srcitem,"quantityQNT") * ((double)ap_mult / SATOSHIDEN);
            priceNQT = (price * ap_mult + (ap_mult/2)/SATOSHIDEN);
            quantityQNT = (vol * SATOSHIDEN) / ap_mult;
            balance = get_asset_quantity(&unconfirmed,NXTaddr,assetidstr);
            printf("%s balance %.8f unconfirmed %.8f vs price %llu qty %llu for asset.%s | price_vol.(%f * %f) * (%lld / %llu)\n",NXTaddr,dstr(balance),dstr(unconfirmed),(long long)priceNQT,(long long)quantityQNT,assetidstr,price,vol,(long long)SATOSHIDEN,(long long)ap_mult);
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
        printf("calc_price_volume error: (%llu/%llu) -> %f %f -> (%llu %llu)\n",(long long)baseamount,(long long)relamount,price,vol,(long long)checkbase,(long long)checkrel);//, getchar();
    return(price);
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
    char assetidstr[64],NXTaddr[64],cmd[1024],*jsonstr;
    int64_t ap_mult,unconfirmed,balance = 0;
    cJSON *json;
    expand_nxt64bits(NXTaddr,nxt64bits);
    if ( assetid == NXT_ASSETID )
    {
        sprintf(cmd,"requestType=getAccount&account=%s",NXTaddr);
        if ( (jsonstr= issue_NXTPOST(cmd)) != 0 )
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
        if ( (ap_mult= assetmult(assetidstr)) != 0 )
        {
            expand_nxt64bits(NXTaddr,nxt64bits);
            balance = ap_mult * get_asset_quantity(&unconfirmed,NXTaddr,assetidstr);
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
