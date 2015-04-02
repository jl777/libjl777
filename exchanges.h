//
//  exchanges.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_exchanges_h
#define xcode_exchanges_h

double get_current_rate(char *base,char *rel)
{
    struct coin_info *cp;
    if ( strcmp(rel,"NXT") == 0 )
    {
        if ( (cp= get_coin_info(base)) != 0 )
        {
            if ( cp->RAM.NXTconvrate != 0. )
                return(cp->RAM.NXTconvrate);
            if ( cp->NXTfee_equiv != 0 && cp->txfee != 0 )
                return(cp->NXTfee_equiv / cp->txfee);
        }
    }
    return(1.);
}

void set_exchange_fname(char *fname,char *exchangestr,char *base,char *rel,uint64_t baseid,uint64_t relid)
{
    char exchange[16],basestr[64],relstr[64];
    uint64_t mult;
    if ( strcmp(exchange,"iDEX") == 0 )
        strcpy(exchange,"InstantDEX");
    else strcpy(exchange,exchangestr);
    ensure_dir(PRICEDIR);
    sprintf(fname,"%s/%s",PRICEDIR,exchange);
    ensure_dir(fname);
    if ( is_cryptocoin(base) != 0 )
        strcpy(basestr,base);
    else set_assetname(&mult,basestr,baseid);
    if ( is_cryptocoin(rel) != 0 )
        strcpy(relstr,rel);
    else set_assetname(&mult,relstr,relid);
    sprintf(fname,"%s/%s/%s_%s",PRICEDIR,exchange,basestr,relstr);
}

struct exchange_info *find_exchange(char *exchangestr,int32_t createflag)
{
    int32_t exchangeid;
    struct exchange_info *exchange = 0;
    for (exchangeid=0; exchangeid<MAX_EXCHANGES; exchangeid++)
    {
        exchange = &Exchanges[exchangeid];
        if ( exchange->name[0] == 0 )
        {
            if ( createflag == 0 )
                return(0);
            strcpy(exchange->name,exchangestr);
            exchange->exchangeid = exchangeid;
            exchange->nxt64bits = stringbits(exchangestr);
            printf("CREATE EXCHANGE.(%s) id.%d %llu\n",exchangestr,exchangeid,(long long)exchange->nxt64bits);
            break;
        }
        if ( strcmp(exchangestr,exchange->name) == 0 )
            break;
    }
    return(exchange);
}

int32_t is_exchange_nxt64bits(uint64_t nxt64bits)
{
    int32_t exchangeid;
    struct exchange_info *exchange = 0;
    for (exchangeid=0; exchangeid<MAX_EXCHANGES; exchangeid++)
    {
        exchange = &Exchanges[exchangeid];
        // printf("(%s).(%llu vs %llu) ",exchange->name,(long long)exchange->nxt64bits,(long long)nxt64bits);
        if ( exchange->name[0] == 0 )
            return(0);
        if ( exchange->nxt64bits == nxt64bits )
            return(1);
    }
    printf("no exchangebits match\n");
    return(0);
}

void ramparse_cryptsy(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui) // "BTC-BTCD"
{
    static char *marketstr = "[{\"42\":141},{\"AC\":199},{\"AGS\":253},{\"ALF\":57},{\"AMC\":43},{\"ANC\":66},{\"APEX\":257},{\"ARG\":48},{\"AUR\":160},{\"BC\":179},{\"BCX\":142},{\"BEN\":157},{\"BET\":129},{\"BLU\":251},{\"BQC\":10},{\"BTB\":23},{\"BTCD\":256},{\"BTE\":49},{\"BTG\":50},{\"BUK\":102},{\"CACH\":154},{\"CAIx\":221},{\"CAP\":53},{\"CASH\":150},{\"CAT\":136},{\"CGB\":70},{\"CINNI\":197},{\"CLOAK\":227},{\"CLR\":95},{\"CMC\":74},{\"CNC\":8},{\"CNL\":260},{\"COMM\":198},{\"COOL\":266},{\"CRC\":58},{\"CRYPT\":219},{\"CSC\":68},{\"DEM\":131},{\"DGB\":167},{\"DGC\":26},{\"DMD\":72},{\"DOGE\":132},{\"DRK\":155},{\"DVC\":40},{\"EAC\":139},{\"ELC\":12},{\"EMC2\":188},{\"EMD\":69},{\"EXE\":183},{\"EZC\":47},{\"FFC\":138},{\"FLT\":192},{\"FRAC\":259},{\"FRC\":39},{\"FRK\":33},{\"FST\":44},{\"FTC\":5},{\"GDC\":82},{\"GLC\":76},{\"GLD\":30},{\"GLX\":78},{\"GLYPH\":229},{\"GUE\":241},{\"HBN\":80},{\"HUC\":249},{\"HVC\":185},{\"ICB\":267},{\"IFC\":59},{\"IXC\":38},{\"JKC\":25},{\"JUDGE\":269},{\"KDC\":178},{\"KEY\":255},{\"KGC\":65},{\"LGD\":204},{\"LK7\":116},{\"LKY\":34},{\"LTB\":202},{\"LTC\":3},{\"LTCX\":233},{\"LYC\":177},{\"MAX\":152},{\"MEC\":45},{\"MIN\":258},{\"MINT\":156},{\"MN1\":187},{\"MN2\":196},{\"MNC\":7},{\"MRY\":189},{\"MYR\":200},{\"MZC\":164},{\"NAN\":64},{\"NAUT\":207},{\"NAV\":252},{\"NBL\":32},{\"NEC\":90},{\"NET\":134},{\"NMC\":29},{\"NOBL\":264},{\"NRB\":54},{\"NRS\":211},{\"NVC\":13},{\"NXT\":159},{\"NYAN\":184},{\"ORB\":75},{\"OSC\":144},{\"PHS\":86},{\"Points\":120},{\"POT\":173},{\"PPC\":28},{\"PSEUD\":268},{\"PTS\":119},{\"PXC\":31},{\"PYC\":92},{\"QRK\":71},{\"RDD\":169},{\"RPC\":143},{\"RT2\":235},{\"RYC\":9},{\"RZR\":237},{\"SAT2\":232},{\"SBC\":51},{\"SC\":225},{\"SFR\":270},{\"SHLD\":248},{\"SMC\":158},{\"SPA\":180},{\"SPT\":81},{\"SRC\":88},{\"STR\":83},{\"SUPER\":239},{\"SXC\":153},{\"SYNC\":271},{\"TAG\":117},{\"TAK\":166},{\"TEK\":114},{\"TES\":223},{\"TGC\":130},{\"TOR\":250},{\"TRC\":27},{\"UNB\":203},{\"UNO\":133},{\"URO\":247},{\"USDe\":201},{\"UTC\":163},{\"VIA\":261},{\"VOOT\":254},{\"VRC\":209},{\"VTC\":151},{\"WC\":195},{\"WDC\":14},{\"XC\":210},{\"XJO\":115},{\"XLB\":208},{\"XPM\":63},{\"XXX\":265},{\"YAC\":11},{\"YBC\":73},{\"ZCC\":140},{\"ZED\":170},{\"ZET\":85}]";
    
    int32_t i,m,numoldbids,numoldasks;
    cJSON *json,*obj,*marketjson;
    char *jsonstr,market[128];
    struct InstantDEX_quote *prevbids,*prevasks;
    prevbids = clone_quotes(&numoldbids,bids), prevasks = clone_quotes(&numoldasks,asks);
    if ( bids->url[0] == 0 )
    {
        if ( strcmp(bids->rel,"BTC") != 0 )
        {
            //printf("parse_cryptsy.(%s/%s): only BTC markets supported\n",bids->base,bids->rel);
            return;
        }
        marketjson = cJSON_Parse(marketstr);
        if ( marketjson == 0 )
        {
            printf("parse_cryptsy: cant parse.(%s)\n",marketstr);
            return;
        }
        m = cJSON_GetArraySize(marketjson);
        market[0] = 0;
        for (i=0; i<m; i++)
        {
            obj = cJSON_GetArrayItem(marketjson,i);
            printf("(%s) ",get_cJSON_fieldname(obj));
            if ( strcmp(bids->base,get_cJSON_fieldname(obj)) == 0 )
            {
                printf("set market -> %llu\n",(long long)obj->child->valueint);
                sprintf(market,"%llu",(long long)obj->child->valueint);
                break;
            }
        }
        free(marketjson);
        if ( market[0] == 0 )
        {
            printf("parse_cryptsy: no marketid for %s\n",bids->base);
            return;
        }
        sprintf(bids->url,"http://pubapi.cryptsy.com/api.php?method=singleorderdata&marketid=%s",market);
    }
    jsonstr = _issue_curl(0,"cryptsy",bids->url);
    if ( jsonstr != 0 )
    {
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            if ( get_cJSON_int(json,"success") == 1 && (obj= cJSON_GetObjectItem(json,"return")) != 0 )
                ramparse_json_orderbook(bids,asks,maxdepth,obj,bids->base,"buyorders","sellorders","price","quantity",gui);
            free_json(json);
        }
        free(jsonstr);
    }
    emit_orderbook_changes(bids,prevbids,numoldbids), emit_orderbook_changes(asks,prevasks,numoldasks);
}

void ramparse_bittrex(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui) // "BTC-BTCD"
{
    cJSON *json,*obj;
    char *jsonstr,market[128];
    int32_t numoldbids,numoldasks;
    struct InstantDEX_quote *prevbids,*prevasks;
    prevbids = clone_quotes(&numoldbids,bids), prevasks = clone_quotes(&numoldasks,asks);
    if ( bids->url[0] == 0 )
    {
        sprintf(market,"%s-%s",bids->rel,bids->base);
        sprintf(bids->url,"https://bittrex.com/api/v1.1/public/getorderbook?market=%s&type=both&depth=%d",market,maxdepth);
    }
    jsonstr = _issue_curl(0,"trex",bids->url);
    //printf("(%s) -> (%s)\n",ep->url,jsonstr);
    // {"success":true,"message":"","result":{"buy":[{"Quantity":1.69284935,"Rate":0.00122124},{"Quantity":130.39771416,"Rate":0.00122002},{"Quantity":77.31781088,"Rate":0.00122000},{"Quantity":10.00000000,"Rate":0.00120150},{"Quantity":412.23470195,"Rate":0.00119500}],"sell":[{"Quantity":8.58353086,"Rate":0.00123019},{"Quantity":10.93796714,"Rate":0.00127744},{"Quantity":17.96825904,"Rate":0.00128000},{"Quantity":2.80400381,"Rate":0.00129999},{"Quantity":200.00000000,"Rate":0.00130000}]}}
    if ( jsonstr != 0 )
    {
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (obj= cJSON_GetObjectItem(json,"success")) != 0 && is_cJSON_True(obj) != 0 )
                ramparse_json_orderbook(bids,asks,maxdepth,json,"result","buy","sell","Rate","Quantity",gui);
            free_json(json);
        }
        free(jsonstr);
    }
    emit_orderbook_changes(bids,prevbids,numoldbids), emit_orderbook_changes(asks,prevasks,numoldasks);
}

void ramparse_poloniex(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    cJSON *json;
    int32_t numoldbids,numoldasks;
    char *jsonstr,market[128];
    struct InstantDEX_quote *prevbids,*prevasks;
    prevbids = clone_quotes(&numoldbids,bids), prevasks = clone_quotes(&numoldasks,asks);
    if ( bids->url[0] == 0 )
    {
        sprintf(market,"%s_%s",bids->rel,bids->base);
        sprintf(bids->url,"https://poloniex.com/public?command=returnOrderBook&currencyPair=%s&depth=%d",market,maxdepth);
    }
    jsonstr = _issue_curl(0,"polo",bids->url);
    //{"asks":[[7.975e-5,200],[7.98e-5,108.46834002],[7.998e-5,1.2644054],[8.0e-5,1799],[8.376e-5,1.7528442],[8.498e-5,30.25055195],[8.499e-5,570.01953171],[8.5e-5,1399.91458777],[8.519e-5,123.82790941],[8.6e-5,1000],[8.696e-5,1.3914002],[8.7e-5,1000],[8.723e-5,112.26190114],[8.9e-5,181.28118327],[8.996e-5,1.237759],[9.0e-5,270.096],[9.049e-5,2993.99999999],[9.05e-5,3383.48549983],[9.068e-5,2.52582092],[9.1e-5,30],[9.2e-5,40],[9.296e-5,1.177861],[9.3e-5,81.59999998],[9.431e-5,2],[9.58e-5,1.074289],[9.583e-5,3],[9.644e-5,858.48948115],[9.652e-5,3441.55358115],[9.66e-5,15699.9569377],[9.693e-5,23.5665998],[9.879e-5,1.0843656],[9.881e-5,2],[9.9e-5,700],[9.999e-5,123.752],[0.0001,34.04293],[0.00010397,1.742916],[0.00010399,11.24446],[0.00010499,1497.79999999],[0.00010799,1.2782902],[0.000108,1818.80661458],[0.00011,1395.27245417],[0.00011407,0.89460453],[0.00011409,0.89683778],[0.0001141,0.906],[0.00011545,458.09939081],[0.00011599,5],[0.00011798,1.0751625],[0.00011799,5],[0.00011999,5.86],[0.00012,279.64865088]],"bids":[[7.415e-5,4495],[7.393e-5,1.8650999],[7.392e-5,974.53828463],[7.382e-5,896.34272554],[7.381e-5,3000],[7.327e-5,1276.26600246],[7.326e-5,77.32705432],[7.32e-5,190.98472093],[7.001e-5,2.2642602],[7.0e-5,1112.19485714],[6.991e-5,2000],[6.99e-5,5000],[6.951e-5,500],[6.914e-5,91.63013181],[6.9e-5,500],[6.855e-5,500],[6.85e-5,238.86947265],[6.212e-5,5.2800413],[6.211e-5,4254.38737723],[6.0e-5,1697.3335],[5.802e-5,3.1241932],[5.801e-5,4309.60179279],[5.165e-5,20],[5.101e-5,6.2903434],[5.1e-5,100],[5.0e-5,5000],[4.5e-5,15],[3.804e-5,16.67907],[3.803e-5,30],[3.002e-5,1400],[3.001e-5,15.320937],[3.0e-5,10000],[2.003e-5,32.345771],[2.002e-5,50],[2.0e-5,25000],[1.013e-5,79.250137],[1.012e-5,200],[1.01e-5,200000],[2.0e-7,5000],[1.9e-7,5000],[1.4e-7,1644.2107],[1.2e-7,1621.8622],[1.1e-7,10000],[1.0e-7,100000],[6.0e-8,4253.7528],[4.0e-8,3690.3146],[3.0e-8,100000],[1.0e-8,100000]],"isFrozen":"0"}
    if ( jsonstr != 0 )
    {
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            ramparse_json_orderbook(bids,asks,maxdepth,json,0,"bids","asks",0,0,gui);
            free_json(json);
        }
        free(jsonstr);
    }
    emit_orderbook_changes(bids,prevbids,numoldbids), emit_orderbook_changes(asks,prevasks,numoldasks);
}

void ramparse_bitfinex(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    cJSON *json;
    char *jsonstr;
    int32_t numoldbids,numoldasks;
    struct InstantDEX_quote *prevbids,*prevasks;
    prevbids = clone_quotes(&numoldbids,bids), prevasks = clone_quotes(&numoldasks,asks);
    if ( bids->url[0] == 0 )
        sprintf(bids->url,"https://api.bitfinex.com/v1/book/%s%s",bids->base,bids->rel);
    jsonstr = _issue_curl(0,"bitfinex",bids->url);
    //{"bids":[{"price":"239.78","amount":"12.0","timestamp":"1424748729.0"},{"p
    if ( jsonstr != 0 )
    {
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            ramparse_json_orderbook(bids,asks,maxdepth,json,0,"bids","asks","price","amount",gui);
            free_json(json);
        }
        free(jsonstr);
    }
    emit_orderbook_changes(bids,prevbids,numoldbids), emit_orderbook_changes(asks,prevasks,numoldasks);
}

cJSON *inner_json(double price,double vol,uint32_t timestamp,uint64_t quoteid,uint64_t nxt64bits)
{
    cJSON *inner = cJSON_CreateArray();
    char numstr[64];
    cJSON_AddItemToArray(inner,cJSON_CreateNumber(price));
    cJSON_AddItemToArray(inner,cJSON_CreateNumber(vol));
    cJSON_AddItemToArray(inner,cJSON_CreateNumber(timestamp));
    sprintf(numstr,"%llu",(long long)quoteid), cJSON_AddItemToArray(inner,cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)nxt64bits), cJSON_AddItemToArray(inner,cJSON_CreateString(numstr));
    return(inner);
}

void convram_NXT_quotejson(struct NXT_tx *txptrs[],int32_t numptrs,uint64_t assetid,int32_t flip,cJSON *json,char *fieldname,uint64_t ap_mult,int32_t maxdepth,char *gui)
{
    //"priceNQT": "12900",
    //"asset": "4551058913252105307",
    //"order": "8128728940342496249",
    //"quantityQNT": "20000000",
    struct NXT_tx *tx;
    uint32_t timestamp;
    int32_t i,n,dir;
    uint64_t baseamount,relamount;
    struct InstantDEX_quote iQ;
    cJSON *srcobj,*srcitem;
    if ( ap_mult == 0 )
        return;
    if ( flip == 0 )
        dir = 1;
    else dir = -1;
    for (i=0; i<numptrs; i++)
    {
        tx = txptrs[i];
        if ( tx->assetidbits == assetid && tx->type == 2 && tx->subtype == (3 - flip) && tx->refhash.txid == 0 )
        {
            printf("i.%d: assetid.%llu type.%d subtype.%d time.%u\n",i,(long long)tx->assetidbits,tx->type,tx->subtype,tx->timestamp);
            baseamount = (tx->U.quantityQNT * ap_mult);
            relamount = (tx->U.quantityQNT * tx->priceNQT);
            add_rambook_quote(INSTANTDEX_NXTAENAME,&iQ,tx->senderbits,tx->timestamp,dir,assetid,NXT_ASSETID,0.,0.,baseamount,relamount,gui,0);
        }
    }
    srcobj = cJSON_GetObjectItem(json,fieldname);
    if ( srcobj != 0 )
    {
        if ( (n= cJSON_GetArraySize(srcobj)) > 0 )
        {
            for (i=0; i<n&&i<maxdepth; i++)
            {
                srcitem = cJSON_GetArrayItem(srcobj,i);
                baseamount = (get_satoshi_obj(srcitem,"quantityQNT") * ap_mult);
                relamount = (get_satoshi_obj(srcitem,"quantityQNT") * get_satoshi_obj(srcitem,"priceNQT"));
                timestamp = get_blockutime((uint32_t)get_API_int(cJSON_GetObjectItem(srcitem,"height"),0));
                add_rambook_quote(INSTANTDEX_NXTAENAME,&iQ,get_API_nxt64bits(cJSON_GetObjectItem(srcitem,"account")),timestamp,dir,assetid,NXT_ASSETID,0.,0.,baseamount,relamount,gui,get_API_nxt64bits(cJSON_GetObjectItem(srcitem,"order")));
            }
        }
    }
}

int32_t update_iQ_flags(struct NXT_tx *txptrs[],int32_t maxtx,uint64_t refassetid)
{
    uint64_t quoteid,assetid,amount,qty;
    char cmd[1024],txidstr[MAX_JSON_FIELD],account[MAX_JSON_FIELD],comment[MAX_JSON_FIELD],*jsonstr;
    cJSON *json,*array,*txobj,*attachment,*msgobj,*commentobj;
    int32_t i,n,numbooks,type,subtype,m = 0;
    struct rambook_info **obooks;
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
                    type = subtype = -1;
                    if ( (attachment= cJSON_GetObjectItem(txobj,"attachment")) != 0 )
                    {
                        assetid = get_API_nxt64bits(cJSON_GetObjectItem(attachment,"asset"));
                        amount = get_API_nxt64bits(cJSON_GetObjectItem(attachment,"amountNQT"));
                        type = (int32_t)get_API_int(cJSON_GetObjectItem(attachment,"type"),-1);
                        subtype = (int32_t)get_API_int(cJSON_GetObjectItem(attachment,"subtype"),-1);
                        comment[0] = 0;
                        if ( (msgobj= cJSON_GetObjectItem(attachment,"message")) != 0 )
                        {
                            copy_cJSON(comment,msgobj);
                            if ( comment[0] != 0 )
                            {
                                unstringify(comment);
                                if ( (commentobj= cJSON_Parse(comment)) != 0 )
                                {
                                    qty = get_API_nxt64bits(cJSON_GetObjectItem(commentobj,"quantityQNT"));
                                    quoteid = get_API_nxt64bits(cJSON_GetObjectItem(commentobj,"quoteid"));
                                    if ( Debuglevel > 2 )
                                        printf("acct.(%s) pending quoteid.%llu asset.%llu qty.%llu\n",account,(long long)quoteid,(long long)assetid,(long long)qty);
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
    return(m);
}

void ramparse_NXT(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    struct NXT_tx *txptrs[MAX_TXPTRS];
    cJSON *json,*bidobj,*askobj;
    char *buystr,*sellstr;
    struct NXT_asset *ap;
    int32_t createdflag,numoldbids,numoldasks,flip,numptrs;
    char assetidstr[64];
    uint64_t basemult,relmult,assetid;
    if ( NXT_ASSETID != stringbits("NXT") )
        printf("NXT_ASSETID.%llu != %llu stringbits\n",(long long)NXT_ASSETID,(long long)stringbits("NXT"));
    struct InstantDEX_quote *prevbids,*prevasks;
    if ( (bids->assetids[1] != NXT_ASSETID && bids->assetids[0] != NXT_ASSETID) || time(NULL) == bids->lastaccess || time(NULL) == asks->lastaccess )
    {
        //printf("NXT only supports trading against NXT not %llu %llu\n",(long long)bids->assetids[0],(long long)bids->assetids[1]);
        return;
    }
    memset(txptrs,0,sizeof(txptrs));
    prevbids = clone_quotes(&numoldbids,bids), prevasks = clone_quotes(&numoldasks,asks);
    basemult = relmult = 1;
    if ( bids->assetids[0] != NXT_ASSETID )
    {
        expand_nxt64bits(assetidstr,bids->assetids[0]);
        ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
        basemult = ap->mult;
        assetid = bids->assetids[0];
        flip = 0;
        //printf("(%llu %llu) (%llu %llu) flip.%d basemult.%llu\n",(long long)bids->assetids[0],(long long)bids->assetids[1],(long long)asks->assetids[0],(long long)asks->assetids[1],flip,(long long)basemult);
    }
    else
    {
        expand_nxt64bits(assetidstr,bids->assetids[1]);
        ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
        relmult = ap->mult;
        assetid = bids->assetids[1];
        flip = 1;
        //printf("(%llu %llu) (%llu %llu) flip.%d relmult.%llu\n",(long long)bids->assetids[0],(long long)bids->assetids[1],(long long)asks->assetids[0],(long long)asks->assetids[1],flip,(long long)relmult);
    }
    if ( bids->url[0] == 0 )
        sprintf(bids->url,"%s=getBidOrders&asset=%llu&limit=%d",NXTSERVER,(long long)bids->assetids[0],maxdepth);
    if ( asks->url[0] == 0 )
        sprintf(asks->url,"%s=getAskOrders&asset=%llu&limit=%d",NXTSERVER,(long long)asks->assetids[1],maxdepth);
    buystr = _issue_curl(0,"ramparse",bids->url);
    sellstr = _issue_curl(0,"ramparse",asks->url);
    //{"count":3,"type":"BUY","orders":[{"price":"0.00000003","amount":"137066327.96066495","total":"4.11198982"},{"price":"0.00000002","amount":"293181381.39291047","total":"5.86362760"},{"price":"0.00000001","amount":"493836943.18472463","total":"4.93836939"}]}
    if ( buystr != 0 && sellstr != 0 )
    {
        bidobj = askobj = 0;
        numptrs = update_iQ_flags(txptrs,sizeof(txptrs)/sizeof(*txptrs),assetid);
        if ( (json = cJSON_Parse(buystr)) != 0 )
            convram_NXT_quotejson(txptrs,numptrs,assetid,0,json,"bidOrders",ap->mult,maxdepth,gui), free_json(json);
        if ( (json = cJSON_Parse(sellstr)) != 0 )
            convram_NXT_quotejson(txptrs,numptrs,assetid,1,json,"askOrders",ap->mult,maxdepth,gui), free_json(json);
        free_txptrs(txptrs,numptrs);
        asks->lastaccess = bids->lastaccess = (uint32_t)time(NULL);
    }
    if ( buystr != 0 )
        free(buystr);
    if ( sellstr != 0 )
        free(sellstr);
    emit_orderbook_changes(bids,prevbids,numoldbids), emit_orderbook_changes(asks,prevasks,numoldasks);
}

void add_exchange_pair(char *base,uint64_t baseid,char *rel,uint64_t relid,char *exchangestr,void (*ramparse)(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui))
{
    struct rambook_info *bids,*asks;
    int32_t exchangeid,i,n;
    struct orderpair *pair;
    struct exchange_info *exchange = 0;
    bids = get_rambook(base,baseid,rel,relid,exchangestr);
    asks = get_rambook(rel,relid,base,baseid,exchangestr);
    if ( (exchange= find_exchange(exchangestr,1)) == 0 )
        printf("cant add anymore exchanges.(%s)\n",exchangestr);
    else
    {
        exchangeid = exchange->exchangeid;
        if ( (n= exchange->num) == 0 )
        {
            if ( Debuglevel > 2 )
                printf("Start monitoring (%s).%d\n",exchange->name,exchangeid);
            exchange->exchangeid = exchangeid;
            //if ( 0 && portable_thread_create((void *)poll_exchange,&exchange->exchangeid) == 0 )
            //    printf("ERROR poll_exchange\n");
            i = n;
        }
        else
        {
            for (i=0; i<n; i++)
            {
                pair = &exchange->orderpairs[i];
                if ( strcmp(base,pair->bids->base) == 0 && strcmp(rel,pair->bids->rel) == 0 )
                    break;
            }
        }
        if ( i == n )
        {
            pair = &exchange->orderpairs[n];
            pair->bids = bids, pair->asks = asks, pair->ramparse = ramparse;
            if ( exchange->num++ >= (int32_t)(sizeof(exchange->orderpairs)/sizeof(*exchange->orderpairs)) )
            {
                printf("exchange.(%s) orderpairs hit max\n",exchangestr);
                exchange->num = ((int32_t)(sizeof(exchange->orderpairs)/sizeof(*exchange->orderpairs)) - 1);
            }
        }
    }
}

int32_t init_rambooks(char *base,char *rel,uint64_t baseid,uint64_t relid)
{
    int32_t i,createdflag,mask = 0,n = 0;
    char assetidstr[64],_base[64],_rel[64];
    uint64_t basemult,relmult;
    //printf("init_rambooks.(%s %s)\n",base,rel);
    if ( base != 0 && rel != 0 && base[0] != 0 && rel[0] != 0 )
    {
        baseid = stringbits(base);
        relid = stringbits(rel);
        expand_nxt64bits(assetidstr,baseid);
        get_NXTasset(&createdflag,Global_mp,assetidstr);
        expand_nxt64bits(assetidstr,relid);
        get_NXTasset(&createdflag,Global_mp,assetidstr);
        if ( strcmp(base,"BTC") == 0 || strcmp(rel,"BTC") == 0 )
            mask |= 1;
        if ( strcmp(base,"LTC") == 0 || strcmp(rel,"LTC") == 0 )
            mask |= 2;
        if ( strcmp(base,"DRK") == 0 || strcmp(rel,"DRK") == 0 )
            mask |= 4;
        if ( strcmp(base,"USD") == 0 || strcmp(rel,"USD") == 0 )
            mask |= 8;
        if ( (mask & 1) != 0 )
        {
            add_exchange_pair(base,baseid,rel,relid,"cryptsy",ramparse_cryptsy);
            add_exchange_pair(base,baseid,rel,relid,"bittrex",ramparse_bittrex);
            add_exchange_pair(base,baseid,rel,relid,"poloniex",ramparse_poloniex);
            if ( (mask & (~1)) != 0 )
                add_exchange_pair(base,baseid,rel,relid,"bitfinex",ramparse_bitfinex);
        }
        if ( strcmp(base,"NXT") == 0 || strcmp(rel,"NXT") == 0 )
        {
            if ( strcmp(base,"NXT") != 0 )
            {
                for (i=0; i<(int32_t)(sizeof(assetmap)/sizeof(*assetmap)); i++)
                    if ( strcmp(assetmap[i][1],base) == 0 )
                        add_exchange_pair(base,calc_nxt64bits(assetmap[i][0]),rel,relid,INSTANTDEX_NXTAENAME,ramparse_NXT);
            }
            else
            {
                for (i=0; i<(int32_t)(sizeof(assetmap)/sizeof(*assetmap)); i++)
                    if ( strcmp(assetmap[i][1],rel) == 0 )
                        add_exchange_pair(base,baseid,rel,calc_nxt64bits(assetmap[i][0]),INSTANTDEX_NXTAENAME,ramparse_NXT);
            }
        }
    }
    else
    {
        set_assetname(&basemult,_base,baseid);
        set_assetname(&relmult,_rel,relid);
        add_exchange_pair(_base,baseid,_rel,relid,INSTANTDEX_NXTAENAME,ramparse_NXT);
    }
    return(n);
}

/*void *poll_exchange(void *_exchangeidp)
{
    int32_t maxdepth = 13;
    uint32_t i,n,blocknum,sleeptime,exchangeid = *(int32_t *)_exchangeidp;
    struct rambook_info *bids,*asks;
    struct orderpair *pair;
    struct exchange_info *exchange;
    if ( exchangeid >= MAX_EXCHANGES )
    {
        printf("exchangeid.%d >= MAX_EXCHANGES\n",exchangeid);
        exit(-1);
    }
    exchange = &Exchanges[exchangeid];
    sleeptime = EXCHANGE_SLEEP;
    printf("poll_exchange.(%s).%d\n",exchange->name,exchangeid);
    while ( 1 )
    {
        n = 0;
        if ( exchange->lastmilli == 0. || milliseconds() > (exchange->lastmilli + 1000.*EXCHANGE_SLEEP) )
        {
            //printf("%.3f Last %.3f: check %s\n",milliseconds(),Lastmillis[j],ep->name);
            for (i=0; i<exchange->num; i++)
            {
                pair = &exchange->orderpairs[i];
                bids = pair->bids, asks = pair->asks;
                if ( pair->lastmilli == 0. || milliseconds() > (pair->lastmilli + 1000.*QUOTE_SLEEP) )
                {
                    //printf("%.3f lastmilli %.3f: %s: %s %s\n",milliseconds(),pair->lastmilli,exchange->name,bids->base,bids->rel);
                    (*pair->ramparse)(bids,asks,maxdepth,"feed");
                    //fprintf(stderr,"%s:%s_%s.(%d %d)\n",exchange->name,bids->base,bids->rel,bids->numquotes,asks->numquotes);
                    pair->lastmilli = exchange->lastmilli = milliseconds();
                    if ( (bids->updated + asks->updated) != 0 )
                    {
                        n++;
                        if ( bids->updated != 0 )
                            bids->lastmilli = pair->lastmilli;
                        if ( asks->updated != 0 )
                            asks->lastmilli = pair->lastmilli;
                        //if ( _search_list(ep->obookid,obooks,n) < 0 )
                        //    obooks[n] = ep->obookid;
                        //tradebot_event_processor(actual_gmt_jdatetime(),0,&ep,1,ep->obookid,0,1L << j);
                        //changedmasks[(starti + i) % Numactivefiles] |= (1 << j);
                        //n++;
                    }
                }
            }
        }
        if ( strcmp(exchange->name,INSTANTDEX_NXTAENAME) == 0 )
        {
            while ( (blocknum= _get_NXTheight(0)) == exchange->lastblock )
                sleep(1);
            exchange->lastblock = blocknum;
        }
        if ( n == 0 )
        {
            sleep(sleeptime++);
            if ( sleeptime > EXCHANGE_SLEEP*60 )
                sleeptime = EXCHANGE_SLEEP*60;
        } else sleeptime = EXCHANGE_SLEEP;
        
    }
    return(0);
}*/

uint64_t submit_to_exchange(int32_t exchangeid,char **jsonstrp,uint64_t assetid,uint64_t qty,uint64_t priceNQT,int32_t dir,uint64_t nxt64bits,char *NXTACCTSECRET,char *triggerhash,char *comment)
{
    uint64_t txid = 0;
    *jsonstrp = 0;
    if ( exchangeid == INSTANTDEX_NXTAEID || exchangeid == INSTANTDEX_UNCONFID )
    {
        if ( assetid != NXT_ASSETID && qty != 0 && priceNQT != 0 )
        {
            printf("submit to exchange dir.%d\n",dir);
            txid = submit_triggered_bidask(jsonstrp,(dir > 0) ? "placeBidOrder" : "placeAskOrder",nxt64bits,NXTACCTSECRET,assetid,qty,priceNQT,triggerhash,comment);
            if ( *jsonstrp != 0 )
                txid = 0;
        }
    } else printf("submit_to_exchange.(%d) not supported only.%d\n",exchangeid,INSTANTDEX_NXTAEID);
    return(txid);
}

#endif
