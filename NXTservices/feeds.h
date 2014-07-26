//
//  feeds.h
//  xcode
//
//  Created by jl777 on 7/24/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_feeds_h
#define xcode_feeds_h

struct exchange_state
{
    double bidminmax[2],askminmax[2],hbla[2];
    char name[64],url[512],url2[512],base[64],rel[64],lbase[64],lrel[64];
    int32_t updated,polarity,numbids,numasks,numquotes;
    uint32_t type;
    uint64_t obookid,feedid,baseid,relid,basemult,relmult;
    FILE *fp;
    double lastmilli;
    struct orderbook_tx **orders;
};

#define _issue_curl(curl_handle,label,url) bitcoind_RPC(curl_handle,label,url,0,0,0)

int32_t bid_orderbook_tx(struct orderbook_tx *tx,int32_t type,uint64_t nxt64bits,uint64_t obookid,double price,double volume);
int32_t ask_orderbook_tx(struct orderbook_tx *tx,int32_t type,uint64_t nxt64bits,uint64_t obookid,double price,double volume);
uint64_t create_raw_orders(uint64_t assetA,uint64_t assetB);
struct raw_orders *find_raw_orders(uint64_t obookid);
int32_t is_orderbook_bid(int32_t polarity,struct raw_orders *raw,struct orderbook_tx *tx);

uint64_t ensure_orderbook(uint64_t *baseidp,uint64_t *relidp,int32_t *polarityp,char *base,char *rel)
{
    uint64_t baseid,relid,obookid;
    struct raw_orders *raw;
    *baseidp = baseid = get_orderbook_assetid(base);
    *relidp = relid = get_orderbook_assetid(rel);
    if ( baseid == 0 || relid == 0 )
        return(0);
    obookid = create_raw_orders(baseid,relid);
    raw = find_raw_orders(obookid);
    if ( raw->assetA == baseid && raw->assetB == relid )
        *polarityp = 1;
    else if ( raw->assetA == relid && raw->assetB == baseid )
        *polarityp = -1;
    else *polarityp = 0;
    return(obookid);
}

struct exchange_state *init_exchange_state(char *name,char *base,char *rel,uint64_t feedid,uint32_t type,int32_t quotefile)
{
    char fname[512];
    struct exchange_state *ep;
    long fpos,entrysize = (sizeof(uint32_t) + 2*sizeof(float));
    ep = calloc(1,sizeof(*ep));
    memset(ep,0,sizeof(*ep));
    safecopy(ep->name,name,sizeof(ep->name));
    safecopy(ep->base,base,sizeof(ep->base));
    safecopy(ep->lbase,base,sizeof(ep->lbase));
    touppercase(ep->base); tolowercase(ep->lbase);
    safecopy(ep->rel,rel,sizeof(ep->rel));
    safecopy(ep->lrel,rel,sizeof(ep->lrel));
    touppercase(ep->rel); tolowercase(ep->lrel);
    ep->basemult = ep->relmult = 1L;
    ep->feedid = feedid;
    ep->type = type;
    ep->obookid = ensure_orderbook(&ep->baseid,&ep->relid,&ep->polarity,base,rel);
    if ( ep->obookid == 0 )
    {
        printf("couldnt initialize orderbook for %s %s %s\n",name,base,rel);
        free(ep);
        return(0);
    }
    if ( strcmp(name,"NXT") == 0 )
    {
        ep->basemult = get_asset_mult(ep->baseid);
        if ( ep->relid != ORDERBOOK_NXTID || ep->basemult == 0 )
        {
            printf("NXT only supports trading against NXT not %s || basemult.%llu is zero\n",ep->rel,(long long)ep->basemult);
            free(ep);
            return(0);
        }
    }
    if ( quotefile != 0 )
    {
        ensure_directory("exchanges");
        sprintf(fname,"exchanges/%s_%s_%s",name,base,rel);
        ep->fp = fopen(fname,"rb+");
        if ( ep->fp != 0 )
        {
            fseek(ep->fp,0,SEEK_END);
            fpos = ftell(ep->fp);
            if ( (fpos % entrysize) != 0 )
            {
                fpos /= entrysize;
                fpos *= entrysize;
                fseek(ep->fp,fpos,SEEK_SET);
            }
        }
        else ep->fp = fopen(fname,"wb");
        printf("exchange file.(%s) %p\n",fname,ep->fp);
    }
    return(ep);
}

void prep_exchange_state(struct exchange_state *ep)
{
    memset(ep->bidminmax,0,sizeof(ep->bidminmax));
    memset(ep->askminmax,0,sizeof(ep->askminmax));
}

int32_t generate_quote_entry(struct exchange_state *ep)
{
    struct { uint32_t jdatetime; float highbid,lowask; } Q;
    ep->updated = 0;
    if ( ep->hbla[0] == 0. || ep->bidminmax[1] != ep->hbla[0] )
        ep->hbla[0] = ep->bidminmax[1], ep->updated = 1;
    if ( ep->hbla[1] == 0. || ep->askminmax[0] != ep->hbla[1] )
        ep->hbla[1] = ep->askminmax[0], ep->updated = 1;
    if ( ep->updated != 0 && ep->fp != 0 && ep->hbla[0] != 0. && ep->hbla[1] != 0. )
    {
        Q.jdatetime = conv_unixtime((uint32_t)time(NULL));
        Q.highbid = ep->hbla[0];
        Q.lowask = ep->hbla[1];
        if ( fwrite(&Q,1,sizeof(Q),ep->fp) != sizeof(Q) )
            printf("error writing quote to %s\n",ep->name);
        else printf("%12s %s %5s/%-5s %.8f %.8f\n",ep->name,jdatetime_str(Q.jdatetime),ep->base,ep->rel,Q.highbid,Q.lowask);
        fflush(ep->fp);
    }
    return(ep->updated);
}

struct orderbook_tx **conv_quotes(int32_t *nump,int32_t type,uint64_t nxt64bits,uint64_t obookid,int32_t polarity,double *bids,int32_t numbids,double *asks,int32_t numasks)
{
    int32_t iter,i,ret,m,n = 0;
    double *quotes;
    struct orderbook_tx *tx,**orders = 0;
    if ( (m= (numbids + numasks)) == 0 )
        return(0);
    orders = (struct orderbook_tx **)calloc(m,sizeof(*orders));
    tx = 0;
    for (iter=-1; iter<=1; iter+=2)
    {
        if ( iter < 0 )
        {
            quotes = asks;
            m = numasks;
        }
        else
        {
            quotes = bids;
            m = numbids;
        }
        for (i=0; i<m; i++)
        {
            if ( tx == 0 )
                tx = (struct orderbook_tx *)calloc(1,sizeof(*tx));
            if ( iter*polarity > 0 )
                ret = bid_orderbook_tx(tx,type,nxt64bits,obookid,quotes[i*2],quotes[i*2+1]);
            else ret = ask_orderbook_tx(tx,type,nxt64bits,obookid,quotes[i*2],quotes[i*2+1]);
            if ( ret == 0 )
            {
                orders[n++] = tx;
                tx = 0;
            }
        }
    }
    if ( tx != 0 )
        free(tx);
    *nump = n;
    return(orders);
}

int32_t parse_json_quotes(double minmax[2],double *quotes,cJSON *array,int32_t maxdepth,char *pricefield,char *volfield)
{
    cJSON *item;
    int32_t i,n;
    double price;
    n = cJSON_GetArraySize(array);
    if ( n > maxdepth )
        n = maxdepth;
    for (i=0; i<n; i++)
    {
        item = cJSON_GetArrayItem(array,i);
        if ( pricefield != 0 && volfield != 0 )
        {
            quotes[(i<<1)] = price = get_API_float(cJSON_GetObjectItem(item,pricefield));
            quotes[(i<<1) + 1] = get_API_float(cJSON_GetObjectItem(item,volfield));
        }
        else if ( is_cJSON_Array(item) != 0 && cJSON_GetArraySize(item) == 2 ) // big assumptions about order within nested array!
        {
            quotes[(i<<1)] = price = get_API_float(cJSON_GetArrayItem(item,0));
            quotes[(i<<1) + 1] = get_API_float(cJSON_GetArrayItem(item,1));
        }
        else
        {
            printf("unexpected case in parse_json_quotes\n");
            continue;
        }
        if ( minmax[0] == 0. || price < minmax[0] )
            minmax[0] = price;
        if ( minmax[1] == 0. || price > minmax[1] )
            minmax[1] = price;
    }
    return(n);
}

struct orderbook_tx **parse_json_orderbook(struct exchange_state *ep,int32_t maxdepth,cJSON *json,char *resultfield,char *bidfield,char *askfield,char *pricefield,char *volfield)
{
    cJSON *obj = 0,*bidobj,*askobj;
    double *bids,*asks;
    struct orderbook_tx **orders = 0;
    if ( resultfield == 0 )
        obj = json;
    //printf("resultfield.%p obj.%p json.%p\n",resultfield,obj,json);
    if ( resultfield == 0 || (obj= cJSON_GetObjectItem(json,resultfield)) != 0 )
    {
        //printf("inside\n");
        bids = (double *)calloc(maxdepth,sizeof(double) * 2);
        asks = (double *)calloc(maxdepth,sizeof(double) * 2);
        if ( (bidobj= cJSON_GetObjectItem(obj,bidfield)) != 0 && is_cJSON_Array(bidobj) != 0 )
            ep->numbids = parse_json_quotes(ep->bidminmax,bids,bidobj,maxdepth,pricefield,volfield);
        if ( (askobj= cJSON_GetObjectItem(obj,askfield)) != 0 && is_cJSON_Array(askobj) != 0 )
            ep->numasks = parse_json_quotes(ep->askminmax,asks,askobj,maxdepth,pricefield,volfield);
        orders = conv_quotes(&ep->numquotes,ep->type,ep->feedid,ep->obookid,ep->polarity,bids,ep->numbids,asks,ep->numasks);
        free(bids);
        free(asks);
        generate_quote_entry(ep);
        if ( orders != 0 )
        {
            if ( ep->orders != 0 )
                free(ep->orders);
            ep->orders = orders;
        }
    }
    return(ep->orders);
}

struct orderbook_tx **parse_cryptsy(struct exchange_state *ep,int32_t maxdepth) // "BTC-BTCD"
{
    static char *marketstr = "[{\"42\":141},{\"AC\":199},{\"AGS\":253},{\"ALF\":57},{\"AMC\":43},{\"ANC\":66},{\"APEX\":257},{\"ARG\":48},{\"AUR\":160},{\"BC\":179},{\"BCX\":142},{\"BEN\":157},{\"BET\":129},{\"BLU\":251},{\"BQC\":10},{\"BTB\":23},{\"BTCD\":256},{\"BTE\":49},{\"BTG\":50},{\"BUK\":102},{\"CACH\":154},{\"CAIx\":221},{\"CAP\":53},{\"CASH\":150},{\"CAT\":136},{\"CGB\":70},{\"CINNI\":197},{\"CLOAK\":227},{\"CLR\":95},{\"CMC\":74},{\"CNC\":8},{\"CNL\":260},{\"COMM\":198},{\"COOL\":266},{\"CRC\":58},{\"CRYPT\":219},{\"CSC\":68},{\"DEM\":131},{\"DGB\":167},{\"DGC\":26},{\"DMD\":72},{\"DOGE\":132},{\"DRK\":155},{\"DVC\":40},{\"EAC\":139},{\"ELC\":12},{\"EMC2\":188},{\"EMD\":69},{\"EXE\":183},{\"EZC\":47},{\"FFC\":138},{\"FLT\":192},{\"FRAC\":259},{\"FRC\":39},{\"FRK\":33},{\"FST\":44},{\"FTC\":5},{\"GDC\":82},{\"GLC\":76},{\"GLD\":30},{\"GLX\":78},{\"GLYPH\":229},{\"GUE\":241},{\"HBN\":80},{\"HUC\":249},{\"HVC\":185},{\"ICB\":267},{\"IFC\":59},{\"IXC\":38},{\"JKC\":25},{\"JUDGE\":269},{\"KDC\":178},{\"KEY\":255},{\"KGC\":65},{\"LGD\":204},{\"LK7\":116},{\"LKY\":34},{\"LTB\":202},{\"LTC\":3},{\"LTCX\":233},{\"LYC\":177},{\"MAX\":152},{\"MEC\":45},{\"MIN\":258},{\"MINT\":156},{\"MN1\":187},{\"MN2\":196},{\"MNC\":7},{\"MRY\":189},{\"MYR\":200},{\"MZC\":164},{\"NAN\":64},{\"NAUT\":207},{\"NAV\":252},{\"NBL\":32},{\"NEC\":90},{\"NET\":134},{\"NMC\":29},{\"NOBL\":264},{\"NRB\":54},{\"NRS\":211},{\"NVC\":13},{\"NXT\":159},{\"NYAN\":184},{\"ORB\":75},{\"OSC\":144},{\"PHS\":86},{\"Points\":120},{\"POT\":173},{\"PPC\":28},{\"PSEUD\":268},{\"PTS\":119},{\"PXC\":31},{\"PYC\":92},{\"QRK\":71},{\"RDD\":169},{\"RPC\":143},{\"RT2\":235},{\"RYC\":9},{\"RZR\":237},{\"SAT2\":232},{\"SBC\":51},{\"SC\":225},{\"SFR\":270},{\"SHLD\":248},{\"SMC\":158},{\"SPA\":180},{\"SPT\":81},{\"SRC\":88},{\"STR\":83},{\"SUPER\":239},{\"SXC\":153},{\"SYNC\":271},{\"TAG\":117},{\"TAK\":166},{\"TEK\":114},{\"TES\":223},{\"TGC\":130},{\"TOR\":250},{\"TRC\":27},{\"UNB\":203},{\"UNO\":133},{\"URO\":247},{\"USDe\":201},{\"UTC\":163},{\"VIA\":261},{\"VOOT\":254},{\"VRC\":209},{\"VTC\":151},{\"WC\":195},{\"WDC\":14},{\"XC\":210},{\"XJO\":115},{\"XLB\":208},{\"XPM\":63},{\"XXX\":265},{\"YAC\":11},{\"YBC\":73},{\"ZCC\":140},{\"ZED\":170},{\"ZET\":85}]";

    int32_t i,m;
    cJSON *json,*obj,*marketjson;
    char *jsonstr,market[128];
    if ( ep->url[0] == 0 )
    {
        if ( strcmp(ep->rel,"BTC") != 0 )
        {
            printf("parse_cryptsy: only BTC markets supported\n");
            return(0);
        }
        marketjson = cJSON_Parse(marketstr);
        if ( marketjson == 0 )
        {
            printf("parse_cryptsy: cant parse.(%s)\n",marketstr);
            return(0);
        }
        m = cJSON_GetArraySize(marketjson);
        market[0] = 0;
        for (i=0; i<m; i++)
        {
            obj = cJSON_GetArrayItem(marketjson,i);
            printf("(%s) ",get_cJSON_fieldname(obj));
            if ( strcmp(ep->base,get_cJSON_fieldname(obj)) == 0 )
            {
                printf("set market -> %llu\n",(long long)obj->child->valueint);
                sprintf(market,"%llu",(long long)obj->child->valueint);
                break;
            }
        }
        free(marketjson);
        if ( market[0] == 0 )
        {
            printf("parse_cryptsy: no marketid\n");
            return(0);
        }
        sprintf(ep->url,"http://pubapi.cryptsy.com/api.php?method=singleorderdata&marketid=%s",market);
    }
    prep_exchange_state(ep);
    jsonstr = _issue_curl(0,ep->name,ep->url);
    if ( jsonstr != 0 )
    {
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            if ( get_cJSON_int(json,"success") == 1 && (obj= cJSON_GetObjectItem(json,"return")) != 0 )
            {
                parse_json_orderbook(ep,maxdepth,obj,ep->base,"buyorders","sellorders","price","quantity");
            }
            free_json(json);
        }
        free(jsonstr);
    }
    return(ep->orders);
}

struct orderbook_tx **parse_bittrex(struct exchange_state *ep,int32_t maxdepth) // "BTC-BTCD"
{
    cJSON *json,*obj;
    char *jsonstr,market[128];
    prep_exchange_state(ep);
    if ( ep->url[0] == 0 )
    {
        sprintf(market,"%s-%s",ep->rel,ep->base);
        sprintf(ep->url,"https://bittrex.com/api/v1.1/public/getorderbook?market=%s&type=both&depth=%d",market,maxdepth);
    }
    jsonstr = _issue_curl(0,ep->name,ep->url);
    //printf("(%s) -> (%s)\n",ep->url,jsonstr);
   // {"success":true,"message":"","result":{"buy":[{"Quantity":1.69284935,"Rate":0.00122124},{"Quantity":130.39771416,"Rate":0.00122002},{"Quantity":77.31781088,"Rate":0.00122000},{"Quantity":10.00000000,"Rate":0.00120150},{"Quantity":412.23470195,"Rate":0.00119500}],"sell":[{"Quantity":8.58353086,"Rate":0.00123019},{"Quantity":10.93796714,"Rate":0.00127744},{"Quantity":17.96825904,"Rate":0.00128000},{"Quantity":2.80400381,"Rate":0.00129999},{"Quantity":200.00000000,"Rate":0.00130000}]}}
    if ( jsonstr != 0 )
    {
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (obj= cJSON_GetObjectItem(json,"success")) != 0 && is_cJSON_True(obj) != 0 )
                parse_json_orderbook(ep,maxdepth,json,"result","buy","sell","Rate","Quantity");
            free_json(json);
        }
        free(jsonstr);
    }
    return(ep->orders);
}

struct orderbook_tx **parse_poloniex(struct exchange_state *ep,int32_t maxdepth)
{
    cJSON *json;
    char *jsonstr,market[128];
    prep_exchange_state(ep);
    if ( ep->url[0] == 0 )
    {
        sprintf(market,"%s_%s",ep->rel,ep->base);
        sprintf(ep->url,"https://poloniex.com/public?command=returnOrderBook&currencyPair=%s&depth=%d",market,maxdepth);
    }
    jsonstr = _issue_curl(0,ep->name,ep->url);
    //{"asks":[[7.975e-5,200],[7.98e-5,108.46834002],[7.998e-5,1.2644054],[8.0e-5,1799],[8.376e-5,1.7528442],[8.498e-5,30.25055195],[8.499e-5,570.01953171],[8.5e-5,1399.91458777],[8.519e-5,123.82790941],[8.6e-5,1000],[8.696e-5,1.3914002],[8.7e-5,1000],[8.723e-5,112.26190114],[8.9e-5,181.28118327],[8.996e-5,1.237759],[9.0e-5,270.096],[9.049e-5,2993.99999999],[9.05e-5,3383.48549983],[9.068e-5,2.52582092],[9.1e-5,30],[9.2e-5,40],[9.296e-5,1.177861],[9.3e-5,81.59999998],[9.431e-5,2],[9.58e-5,1.074289],[9.583e-5,3],[9.644e-5,858.48948115],[9.652e-5,3441.55358115],[9.66e-5,15699.9569377],[9.693e-5,23.5665998],[9.879e-5,1.0843656],[9.881e-5,2],[9.9e-5,700],[9.999e-5,123.752],[0.0001,34.04293],[0.00010397,1.742916],[0.00010399,11.24446],[0.00010499,1497.79999999],[0.00010799,1.2782902],[0.000108,1818.80661458],[0.00011,1395.27245417],[0.00011407,0.89460453],[0.00011409,0.89683778],[0.0001141,0.906],[0.00011545,458.09939081],[0.00011599,5],[0.00011798,1.0751625],[0.00011799,5],[0.00011999,5.86],[0.00012,279.64865088]],"bids":[[7.415e-5,4495],[7.393e-5,1.8650999],[7.392e-5,974.53828463],[7.382e-5,896.34272554],[7.381e-5,3000],[7.327e-5,1276.26600246],[7.326e-5,77.32705432],[7.32e-5,190.98472093],[7.001e-5,2.2642602],[7.0e-5,1112.19485714],[6.991e-5,2000],[6.99e-5,5000],[6.951e-5,500],[6.914e-5,91.63013181],[6.9e-5,500],[6.855e-5,500],[6.85e-5,238.86947265],[6.212e-5,5.2800413],[6.211e-5,4254.38737723],[6.0e-5,1697.3335],[5.802e-5,3.1241932],[5.801e-5,4309.60179279],[5.165e-5,20],[5.101e-5,6.2903434],[5.1e-5,100],[5.0e-5,5000],[4.5e-5,15],[3.804e-5,16.67907],[3.803e-5,30],[3.002e-5,1400],[3.001e-5,15.320937],[3.0e-5,10000],[2.003e-5,32.345771],[2.002e-5,50],[2.0e-5,25000],[1.013e-5,79.250137],[1.012e-5,200],[1.01e-5,200000],[2.0e-7,5000],[1.9e-7,5000],[1.4e-7,1644.2107],[1.2e-7,1621.8622],[1.1e-7,10000],[1.0e-7,100000],[6.0e-8,4253.7528],[4.0e-8,3690.3146],[3.0e-8,100000],[1.0e-8,100000]],"isFrozen":"0"}
    if ( jsonstr != 0 )
    {
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            parse_json_orderbook(ep,maxdepth,json,0,"bids","asks",0,0);
            free_json(json);
        }
        free(jsonstr);
    }
    return(ep->orders);
}

struct orderbook_tx **parse_bter(struct exchange_state *ep,int32_t maxdepth)
{
    cJSON *json,*obj;
    char resultstr[512],*jsonstr;
    prep_exchange_state(ep);
    if ( ep->url[0] == 0 )
        sprintf(ep->url,"http://data.bter.com/api/1/depth/%s_%s",ep->lbase,ep->lrel);
    jsonstr = _issue_curl(0,ep->name,ep->url);
    //{"result":"true","asks":[["0.00008035",100],["0.00008030",2030],["0.00008024",100],["0.00008018",643.41783554],["0.00008012",100]
    if ( jsonstr != 0 )
    {
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (obj= cJSON_GetObjectItem(json,"result")) != 0 )
            {
                copy_cJSON(resultstr,obj);
                if ( strcmp(resultstr,"true") == 0 )
                {
                    parse_json_orderbook(ep,maxdepth,json,0,"bids","asks",0,0);
                }
            }
            free_json(json);
        }
        free(jsonstr);
    }
    return(ep->orders);
}

struct orderbook_tx **parse_mintpal(struct exchange_state *ep,int32_t maxdepth)
{
    cJSON *json,*bidobj,*askobj;
    char *buystr,*sellstr;
    prep_exchange_state(ep);
    if ( ep->url[0] == 0 )
        sprintf(ep->url,"https://api.mintpal.com/v1/market/orders/%s/%s/BUY",ep->base,ep->rel);
    if ( ep->url2[0] == 0 )
        sprintf(ep->url2,"https://api.mintpal.com/v1/market/orders/%s/%s/SELL",ep->base,ep->rel);
    buystr = _issue_curl(0,ep->name,ep->url);
    sellstr = _issue_curl(0,ep->name,ep->url2);
//{"count":3,"type":"BUY","orders":[{"price":"0.00000003","amount":"137066327.96066495","total":"4.11198982"},{"price":"0.00000002","amount":"293181381.39291047","total":"5.86362760"},{"price":"0.00000001","amount":"493836943.18472463","total":"4.93836939"}]}
    if ( buystr != 0 && sellstr != 0 )
    {
        bidobj = askobj = 0;
        if ( (json = cJSON_Parse(buystr)) != 0 )
        {
            bidobj = cJSON_DetachItemFromObject(json,"orders");
            free_json(json);
        }
        if ( (json = cJSON_Parse(sellstr)) != 0 )
        {
            askobj = cJSON_DetachItemFromObject(json,"orders");
            free_json(json);
        }
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json,"bids",bidobj);
        cJSON_AddItemToObject(json,"asks",askobj);
        parse_json_orderbook(ep,maxdepth,json,0,"bids","asks","price","amount");
        free_json(json);
    }
    if ( buystr != 0 )
        free(buystr);
    if ( sellstr != 0 )
        free(sellstr);
    return(ep->orders);
}

cJSON *conv_NXT_quotejson(cJSON *json,char *fieldname,uint64_t ap_mult)
{
    //"priceNQT": "12900",
    //"asset": "4551058913252105307",
    //"order": "8128728940342496249",
    //"quantityQNT": "20000000",
    int32_t i,n;
    double price,vol,factor;
    cJSON *srcobj,*srcitem,*inner,*array = 0;
    factor = (double)ap_mult / SATOSHIDEN;
    srcobj = cJSON_GetObjectItem(json,fieldname);
    if ( srcobj != 0 )
    {
        if ( (n= cJSON_GetArraySize(srcobj)) > 0 )
        {
            array = cJSON_CreateArray();
            for (i=0; i<n; i++)
            {
                srcitem = cJSON_GetArrayItem(srcobj,i);
                price = (double)get_satoshi_obj(srcitem,"priceNQT") / ap_mult;
                vol = (double)get_satoshi_obj(srcitem,"quantityQNT") * factor;
                inner = cJSON_CreateArray();
                cJSON_AddItemToArray(inner,cJSON_CreateNumber(price));
                cJSON_AddItemToArray(inner,cJSON_CreateNumber(vol));
                cJSON_AddItemToArray(array,inner);
            }
        }
        free_json(json);
        json = array;
    }
    else free_json(json), json = 0;
    return(json);
}

struct orderbook_tx **parse_NXT(struct exchange_state *ep,int32_t maxdepth)
{
    cJSON *json,*bidobj,*askobj;
    char *buystr,*sellstr;
    if ( ep->relid != ORDERBOOK_NXTID )
    {
        printf("NXT only supports trading against NXT not %s\n",ep->rel);
        return(0);
    }
    prep_exchange_state(ep);
    if ( ep->url[0] == 0 )
        sprintf(ep->url,"%s=getBidOrders&asset=%llu&limit=%d",NXTSERVER,(long long)ep->baseid,maxdepth);
    if ( ep->url2[0] == 0 )
        sprintf(ep->url2,"%s=getAskOrders&asset=%llu&limit=%d",NXTSERVER,(long long)ep->baseid,maxdepth);
    buystr = _issue_curl(0,ep->name,ep->url);
    sellstr = _issue_curl(0,ep->name,ep->url2);
    //{"count":3,"type":"BUY","orders":[{"price":"0.00000003","amount":"137066327.96066495","total":"4.11198982"},{"price":"0.00000002","amount":"293181381.39291047","total":"5.86362760"},{"price":"0.00000001","amount":"493836943.18472463","total":"4.93836939"}]}
    if ( buystr != 0 && sellstr != 0 )
    {
        bidobj = askobj = 0;
        if ( (json = cJSON_Parse(buystr)) != 0 )
            bidobj = conv_NXT_quotejson(json,"bidOrders",ep->basemult);
        if ( (json = cJSON_Parse(sellstr)) != 0 )
            askobj = conv_NXT_quotejson(json,"askOrders",ep->basemult);
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json,"bids",bidobj);
        cJSON_AddItemToObject(json,"asks",askobj);
        parse_json_orderbook(ep,maxdepth,json,0,"bids","asks",0,0);
        free_json(json);
    }
    if ( buystr != 0 )
        free(buystr);
    if ( sellstr != 0 )
        free(sellstr);
    return(ep->orders);
}

#ifdef old
int32_t get_bitstamp_obook(int32_t *numbidsp,uint64_t *bids,int32_t *numasksp,uint64_t *asks,int32_t max)
{
    memset(bids,0,sizeof(*bids) * max);
    memset(asks,0,sizeof(*asks) * max);
    return(parse_bidasks(numbidsp,bids,numasksp,asks,max,_issue_curl(Global_iDEX->curl_handle,"bitstamp","https://www.bitstamp.net/api/order_book/")));
}

int32_t get_btce_obook(char *econtract,int32_t *numbidsp,uint64_t *bids,int32_t *numasksp,uint64_t *asks,int32_t max)
{
    char url[128];
    memset(bids,0,sizeof(*bids) * max);
    memset(asks,0,sizeof(*asks) * max);
    sprintf(url,"https://btc-e.com/api/2/%s/depth",econtract);
    return(parse_bidasks(numbidsp,bids,numasksp,asks,max,_issue_curl(Global_iDEX->curl_handle,"btce",url)));
}


int32_t get_okcoin_obook(char *econtract,int32_t *numbidsp,uint64_t *bids,int32_t *numasksp,uint64_t *asks,int32_t max)
{
    char url[128];
    memset(bids,0,sizeof(*bids) * max);
    memset(asks,0,sizeof(*asks) * max);
    sprintf(url,"https://www.okcoin.com/api/depth.do?symbol=%s",econtract);
    return(parse_bidasks(numbidsp,bids,numasksp,asks,max,_issue_curl(Global_iDEX->curl_handle,"okcoin",url)));
}

void disp_quotepairs(char *base,char *rel,char *label,uint64_t *bids,int32_t numbids,uint64_t *asks,int32_t numasks)
{
    int32_t i;
    char left[256],right[256];
    for (i=0; i<numbids&&i<numasks; i++)
    {
        left[0] = right[0] = 0;
        if ( i < numbids )
            sprintf(left,"%s%10.5f %s%9.2f | bid %s%9.3f",base,dstr(bids[(i<<1)+1]),rel,dstr(bids[(i<<1)]*((double)bids[(i<<1)+1]/SATOSHIDEN)),rel,dstr(bids[i<<1]));
        if ( i < numasks )
            sprintf(right," %s%9.3f ask | %s%9.2f %s%10.5f",rel,dstr(asks[(i<<1)]),rel,dstr(asks[(i<<1)]*((double)asks[(i<<1)+1]/SATOSHIDEN)),base,dstr(asks[(i<<1)+1]));
        printf("(%-8s.%-2d) %32s%-32s\n",label,i,left,right);
    }
}

void load_orderbooks()
{
    int numbids,numasks;
    uint64_t bids[10],asks[sizeof(bids)/sizeof(*bids)];
    //if ( get_bitstamp_obook(&numbids,bids,&numasks,asks,(sizeof(bids)/sizeof(*bids))/2) > 0 )
    //    disp_quotepairs("B","$","bitstamp",bids,numbids,asks,numasks);
    if ( get_btce_obook("btc_usd",&numbids,bids,&numasks,asks,(sizeof(bids)/sizeof(*bids))/2) > 0 )
        disp_quotepairs("B","$","btc-c",bids,numbids,asks,numasks);
    if ( get_okcoin_obook("btc_cny",&numbids,bids,&numasks,asks,(sizeof(bids)/sizeof(*bids))/2) > 0 )
        disp_quotepairs("B","Y","okcoin",bids,numbids,asks,numasks);
}
#endif

char *exchange_names[] = { "NXT", "bter", "bittrex", "cryptsy", "poloniex", "mintpal" };
typedef struct orderbook_tx **(*exchange_func)(struct exchange_state *ep,int32_t maxdepth);
exchange_func exchange_funcs[sizeof(exchange_names)/sizeof(*exchange_names)] =
{
    parse_NXT, parse_bter, parse_bittrex, parse_cryptsy, parse_poloniex, parse_mintpal
};

struct exchange_state **Activefiles;
double Lastmillis[sizeof(exchange_names)/sizeof(*exchange_names)];

int32_t extract_baserel(char *base,char *rel,cJSON *inner)
{
    if ( cJSON_GetArraySize(inner) == 2 )
    {
        copy_cJSON(base,cJSON_GetArrayItem(inner,0));
        copy_cJSON(rel,cJSON_GetArrayItem(inner,1));
        if ( base[0] != 0 && rel[0] != 0 )
            return(0);
    }
    return(-1);
}

void init_exchanges(cJSON *confobj,int32_t exchangeflag)
{
    struct orderbook_tx **orders;
    struct exchange_state *ep;
    int32_t i,j,n,numactive = 0;
    cJSON *array,*item;
    char base[64],rel[64];
    for (i=0; i<(int)(sizeof(exchange_names)/sizeof(*exchange_names)); i++)
    {
        if ( confobj == 0 )
            break;
        array = cJSON_GetObjectItem(confobj,exchange_names[i]);
        printf("array for %s %p\n",exchange_names[i],array);
        if ( array == 0 )
            continue;
        if ( (n= cJSON_GetArraySize(array)) > 0 )
        {
            for (j=0; j<n; j++)
            {
                item = cJSON_GetArrayItem(array,j);
                if ( extract_baserel(base,rel,item) == 0 )
                {
                    Activefiles = realloc(Activefiles,sizeof(*Activefiles) * (numactive+1));
                    Activefiles[numactive] = init_exchange_state(exchange_names[i],base,rel,0xfeed0000L + i,ORDERBOOK_FEED,1);
                    if ( Activefiles[numactive] != 0 )
                        numactive++;
                }
            }
        }
    }
    if ( exchangeflag != 0 )
    {
        int32_t maxdepth = 8;
        printf("exchange mode: numactive.%d maxdepth.%d\n",numactive,maxdepth);
        while ( numactive > 0 )
        {
            for (i=0; i<numactive; i++)
            {
                ep = Activefiles[i];
                j = (int32_t)(ep->feedid - 0xfeed0000L);
                if ( Lastmillis[j] == 0. || milliseconds() > Lastmillis[j] + 1000 )
                {
                    //printf("%.3f Last %.3f: check %s\n",milliseconds(),Lastmillis[j],ep->name);
                    if ( ep->lastmilli == 0. || milliseconds() > ep->lastmilli + 10000 )
                    {
                        //printf("%.3f lastmilli %.3f: %s: %s %s\n",milliseconds(),ep->lastmilli,ep->name,ep->base,ep->rel);
                        orders = (*exchange_funcs[j])(ep,maxdepth);
                        ep->lastmilli = Lastmillis[j] = milliseconds();
                    }
                }
            }
            sleep(1);
        }
        exit(0);
    }
}

#endif
