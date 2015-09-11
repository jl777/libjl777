/******************************************************************************
 * Copyright © 2014-2015 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * Nxt software, including this file, may be copied, modified, propagated,    *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

#define BUNDLED
#define PLUGINSTR "InstantDEX"
#define PLUGNAME(NAME) InstantDEX ## NAME
#define STRUCTNAME struct PLUGNAME(_info) 
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define _issue_curl(curl_handle,label,url) bitcoind_RPC(curl_handle,label,url,0,0,0)

#define INSTANTDEX_MINVOL 75
#define INSTANTDEX_MINVOLPERC ((double)INSTANTDEX_MINVOL / 100.)
#define INSTANTDEX_PRICESLIPPAGE 0.001
#define FINISH_HEIGHT 7

#define INSTANTDEX_TRIGGERDEADLINE 120
#define JUMPTRADE_SECONDS 100
#define INSTANTDEX_ACCT "4383817337783094122"
#define INSTANTDEX_FEE ((long)(2.5 * SATOSHIDEN))

#define DEFINES_ONLY
#include "../includes/portable777.h"
#include "../agents/plugin777.c"
#include "../utils/NXT777.c"
#include "../common/txind777.c"
#undef DEFINES_ONLY

char *Supported_exchanges[] = { INSTANTDEX_NAME, INSTANTDEX_NXTAEUNCONF, INSTANTDEX_NXTAENAME, INSTANTDEX_BASKETNAME, "basketNXT", "basketUSD", "basketBTC", "basketCNY", INSTANTDEX_ACTIVENAME, "wallet", "shuffle", "peggy", // peggy MUST be last of special exchanges
    "bitfinex", "btc38", "bitstamp", "btce", "poloniex", "bittrex", "huobi", "coinbase", "okcoin", "bityes", "lakebtc", "quadriga",
    "kraken", "gatecoin", "quoine", "jubi", "hitbtc"  // no trading for these exchanges yet
}; // "bter" <- orderbook is backwards and all entries are needed, later to support, "exmo" flakey apiservers

#define INSTANTDEX_LOCALAPI "allorderbooks", "orderbook", "lottostats", "LSUM", "makebasket", "disable", "enable", "peggyrates", "tradesequence", "placebid", "placeask", "orderstatus", "openorders", "cancelorder", "tradehistory", "balance", "allexchanges", "coinshuffle"

#define INSTANTDEX_REMOTEAPI "msigaddr", "bid", "ask", "swap" //, "funding", "refund"
char *PLUGNAME(_methods)[] = { INSTANTDEX_REMOTEAPI}; // list of supported methods approved for local access
char *PLUGNAME(_pubmethods)[] = { INSTANTDEX_REMOTEAPI }; // list of supported methods approved for public (Internet) access
char *PLUGNAME(_authmethods)[] = { "echo" }; // list of supported methods that require authentication

typedef char *(*json_handler)(int32_t localaccess,int32_t valid,char *sender,cJSON **objs,int32_t numobjs,char *origargstr);

queue_t InstantDEXQ,TelepathyQ;
struct InstantDEX_info INSTANTDEX;
struct pingpong_queue Pending_offersQ;
cJSON *InstantDEX_lottostats();

//#include "NXT_tx.h"
#include "trades.h"
#include "quotes.h"
#include "subatomic.h"

// {"plugin":"InstantDEX","method":"orderbook","baseid":"8688289798928624137","rel":"USD","exchange":"active","allfields":1}

// {"plugin":"InstantDEX","method":"orderbook","baseid":"17554243582654188572","rel":"12071612744977229797","exchange":"active","allfields":1}
// {"plugin":"InstantDEX","method":"orderbook","baseid":"6918149200730574743","rel":"XMR","exchange":"active","allfields":1}

uint32_t prices777_NXTBLOCK,FIRST_EXTERNAL,MAX_DEPTH = 100;
int32_t InstantDEX_idle(struct plugin_info *plugin) { return(0); }

int32_t supported_exchange(char *exchangestr)
{
    int32_t i;
    for (i=0; i<sizeof(Supported_exchanges)/sizeof(*Supported_exchanges); i++)
        if ( strcmp(exchangestr,Supported_exchanges[i]) == 0 )
            return(i);
    return(-1);
}

void idle()
{
    char *jsonstr,*str; cJSON *json; int32_t n = 0; uint32_t nonce;
    while ( INSTANTDEX.readyflag == 0 )
        sleep(1);
    while ( 1 )
    {
        if ( n == 0 )
            msleep(1000);
        n = 0;
        if ( (jsonstr= queue_dequeue(&InstantDEXQ,1)) != 0 )
        {
            if ( (json= cJSON_Parse(jsonstr)) != 0 )
            {
                //printf("Dequeued InstantDEX.(%s)\n",jsonstr);
                //fprintf(stderr,"dequeued\n");
                if ( (str= busdata_sync(&nonce,jsonstr,"allnodes",0)) != 0 )
                {
                    //fprintf(stderr,"busdata.(%s)\n",str);
                    free(str);
                }
                free_json(json);
                n++;
            } else printf("error parsing (%s) from InstantDEXQ\n",jsonstr);
            free_queueitem(jsonstr);
        }
    }
}

void idle2()
{
    static double lastmilli;
    uint32_t NXTblock;
    while ( INSTANTDEX.readyflag == 0 )
        sleep(1);
    while ( 1 )
    {
        if ( milliseconds() < (lastmilli + 5000) )
            msleep(100);
        NXTblock = _get_NXTheight(0);
        if ( 1 && NXTblock != prices777_NXTBLOCK )
        {
            prices777_NXTBLOCK = NXTblock;
            InstantDEX_update(SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET);
            //fprintf(stderr,"done idle NXT\n");
        }
        lastmilli = milliseconds();
    }
}

int32_t prices777_key(char *key,char *exchange,char *name,char *base,uint64_t baseid,char *rel,uint64_t relid)
{
    int32_t len,keysize = 0;
    memcpy(&key[keysize],&baseid,sizeof(baseid)), keysize += sizeof(baseid);
    memcpy(&key[keysize],&relid,sizeof(relid)), keysize += sizeof(relid);
    strcpy(&key[keysize],exchange), keysize += strlen(exchange) + 1;
    strcpy(&key[keysize],name), keysize += strlen(name) + 1;
    memcpy(&key[keysize],base,strlen(base)+1), keysize += strlen(base) + 1;
    if ( rel != 0 && (len= (int32_t)strlen(rel)) > 0 )
        memcpy(&key[keysize],rel,len+1), keysize += len+1;
    return(keysize);
}

int32_t get_assetname(char *name,uint64_t assetid)
{
    char assetidstr[64],*jsonstr; cJSON *json;
    name[0] = 0;
    if ( is_native_crypto(name,assetid) != 0 )
        return((int32_t)strlen(name));
    expand_nxt64bits(assetidstr,assetid);
    name[0] = 0;
    if ( (jsonstr= _issue_getAsset(assetidstr)) != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            extract_cJSON_str(name,15,json,"name");
            free_json(json);
        }
        free(jsonstr);
    }
    return((int32_t)strlen(assetidstr));
}

uint64_t prices777_equiv(uint64_t assetid)
{
    char *str;
    if ( (str= is_MGWasset(0,assetid)) != 0 )
        return(stringbits(str));
    return(assetid);
}

uint64_t InstantDEX_name(char *key,int32_t *keysizep,char *exchange,char *name,char *base,uint64_t *baseidp,char *rel,uint64_t *relidp)
{
    uint64_t baseid,relid,assetbits = 0; char *s,*str;
    baseid = *baseidp, relid = *relidp;
//printf(">>>>>> name.(%s) (%s/%s) %llu/%llu\n",name,base,rel,(long long)baseid,(long long)relid);
    if ( strcmp(base,"5527630") == 0 || baseid == 5527630 )
        strcpy(base,"NXT");
    if ( strcmp(rel,"5527630") == 0 || relid == 5527630 )
        strcpy(rel,"NXT");
    if ( relid == 0 && rel[0] != 0 )
    {
        if ( is_decimalstr(rel) != 0 )
            relid = calc_nxt64bits(rel);
        else relid = is_MGWcoin(rel);
    }
    else if ( (str= is_MGWasset(0,relid)) != 0 )
        strcpy(rel,str);
    if ( baseid == 0 && base[0] != 0 )
    {
        if ( is_decimalstr(base) != 0 )
            baseid = calc_nxt64bits(base);
        else baseid = is_MGWcoin(base);
    }
    else if ( (str= is_MGWasset(0,baseid)) != 0 )
        strcpy(base,str);
    if ( strcmp("InstantDEX",exchange) == 0 || strcmp("nxtae",exchange) == 0 || strcmp("unconf",exchange) == 0 || (baseid != 0 && relid != 0) )
    {
        if ( strcmp(rel,"NXT") == 0 )
            s = "+", relid = stringbits("NXT"), strcpy(rel,"NXT");
        else if ( strcmp(base,"NXT") == 0 )
            s = "-", baseid = stringbits("NXT"), strcpy(base,"NXT");
        else s = "";
        if ( base[0] == 0 )
        {
            get_assetname(base,baseid);
            //printf("mapped %llu -> (%s)\n",(long long)baseid,base);
        }
        if ( rel[0] == 0 )
        {
            get_assetname(rel,relid);
            //printf("mapped %llu -> (%s)\n",(long long)relid,rel);
        }
        if ( name[0] == 0 )
        {
            if ( relid == NXT_ASSETID )
                sprintf(name,"%llu",(long long)baseid);
            else if ( baseid == NXT_ASSETID )
                sprintf(name,"-%llu",(long long)relid);
            else sprintf(name,"%llu/%llu",(long long)baseid,(long long)relid);
        }
    }
    else
    {
        if ( base[0] != 0 && rel[0] != 0 && baseid == 0 && relid == 0 )
        {
            baseid = peggy_basebits(base), relid = peggy_basebits(rel);
            if ( name[0] == 0 && baseid != 0 && relid != 0 )
            {
                strcpy(name,base); // need to be smarter
                strcat(name,"/");
                strcat(name,rel);
            }
        }
        if ( name[0] == 0 || baseid == 0 || relid == 0 || base[0] == 0 || rel[0] == 0 )
        {
            if ( baseid == 0 && base[0] != 0 )
                baseid = stringbits(base);
            else if ( baseid != 0 && base[0] == 0 )
                sprintf(base,"%llu",(long long)baseid);
            if ( relid == 0 && rel[0] != 0 )
            {
                relid = stringbits(rel);
                printf("set relid.%llu <- (%s)\n",(long long)relid,rel);
            }
            else if ( relid != 0 && rel[0] == 0 )
                sprintf(rel,"%llu",(long long)relid);
            if ( name[0] == 0 )
                strcpy(name,base), strcat(name,"/"), strcat(name,rel);
        }
    }
    *baseidp = baseid, *relidp = relid;
    *keysizep = prices777_key(key,exchange,name,base,baseid,rel,relid);
//printf("<<<<<<< name.(%s) (%s/%s) %llu/%llu\n",name,base,rel,(long long)baseid,(long long)relid);
    return(assetbits);
}

cJSON *InstantDEX_lottostats()
{
    char cmdstr[1024],NXTaddr[64],buf[1024],*jsonstr; struct destbuf receiverstr;
    cJSON *json,*array,*txobj; int32_t i,n,totaltickets = 0; uint64_t amount,senderbits; uint32_t timestamp = 0;
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
                    copy_cJSON(&receiverstr,cJSON_GetObjectItem(txobj,"recipient"));
                    if ( strcmp(receiverstr.buf,INSTANTDEX_ACCT) == 0 )
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

int32_t bidask_parse(struct destbuf *exchangestr,struct destbuf *name,struct destbuf *base,struct destbuf *rel,struct destbuf *gui,struct InstantDEX_quote *iQ,cJSON *json)
{
    uint64_t basemult,relmult,baseamount,relamount; double price,volume; int32_t exchangeid,keysize,flag; char key[1024],buf[64],*methodstr;
    memset(iQ,0,sizeof(*iQ));
    iQ->s.baseid = j64bits(json,"baseid"); iQ->s.relid = j64bits(json,"relid");
    iQ->s.baseamount = j64bits(json,"baseamount"), iQ->s.relamount = j64bits(json,"relamount");
    iQ->s.vol = jdouble(json,"volume"); iQ->s.price = jdouble(json,"price");
    copy_cJSON(exchangestr,jobj(json,"exchange"));
    if ( exchangestr->buf[0] == 0 || find_exchange(&exchangeid,exchangestr->buf) == 0 )
        exchangeid = -1;
    iQ->exchangeid = exchangeid;
    copy_cJSON(base,jobj(json,"base"));
    copy_cJSON(rel,jobj(json,"rel"));
    copy_cJSON(name,jobj(json,"name"));
    methodstr = jstr(json,"method");
    if ( methodstr != 0 && (strcmp(methodstr,"placeask") == 0 || strcmp(methodstr,"ask") == 0) )
        iQ->s.isask = 1;
    if ( strcmp(exchangestr->buf,"wallet") == 0 && (iQ->s.baseid == NXT_ASSETID || strcmp(base->buf,"NXT") == 0) )
    {
        flag = 1;
        if ( strcmp(methodstr,"placeask") == 0 )
            methodstr = "placebid";
        else if ( strcmp(methodstr,"placebid") == 0 )
            methodstr = "placeask";
        else if ( strcmp(methodstr,"ask") == 0 )
            methodstr = "bid";
        else if ( strcmp(methodstr,"bid") == 0 )
            methodstr = "ask";
        else flag = 0;
        if ( flag != 0 )
        {
            iQ->s.baseid = iQ->s.relid, iQ->s.relid = NXT_ASSETID;
            strcpy(base->buf,rel->buf), strcpy(rel->buf,"NXT");
            baseamount = iQ->s.baseamount;
            iQ->s.baseamount = iQ->s.relamount, iQ->s.relamount = baseamount;
            name->buf[0] = 0;
            if ( iQ->s.vol > SMALLVAL && iQ->s.price > SMALLVAL )
            {
                iQ->s.vol *= iQ->s.price;
                iQ->s.price = 1. / iQ->s.price;
            }
            iQ->s.isask ^= 1;
            printf("INVERT\n");
        }
    }
    if ( (iQ->s.timestamp= juint(json,"timestamp")) == 0 )
        iQ->s.timestamp = (uint32_t)time(NULL);
    copy_cJSON(gui,jobj(json,"gui")), strncpy(iQ->gui,gui->buf,sizeof(iQ->gui)-1);
    iQ->s.automatch = juint(json,"automatch");
    iQ->s.minperc = juint(json,"minperc");
    if ( (iQ->s.duration= juint(json,"duration")) == 0 || iQ->s.duration > ORDERBOOK_EXPIRATION )
        iQ->s.duration = ORDERBOOK_EXPIRATION;
    InstantDEX_name(key,&keysize,exchangestr->buf,name->buf,base->buf,&iQ->s.baseid,rel->buf,&iQ->s.relid);
    //printf(">>>>>>>>>>>> BASE.(%s) REL.(%s)\n",base->buf,rel->buf);
    iQ->s.basebits = stringbits(base->buf);
    iQ->s.relbits = stringbits(rel->buf);
    iQ->s.offerNXT = j64bits(json,"offerNXT");
    iQ->s.quoteid = j64bits(json,"quoteid");
    if ( strcmp(exchangestr->buf,"shuffle") == 0 )
    {
        if ( iQ->s.price == 0. )
            iQ->s.price = 1.;
        if ( iQ->s.vol == 0. )
            iQ->s.vol = 1.;
        if ( iQ->s.baseamount == 0 )
            iQ->s.baseamount = iQ->s.price * SATOSHIDEN;
    }
    else
    {
        if ( iQ->s.baseamount == 0 || iQ->s.relamount == 0 )
        {
            if ( iQ->s.price <= SMALLVAL || iQ->s.vol <= SMALLVAL )
                return(-1);
            set_best_amounts(&iQ->s.baseamount,&iQ->s.relamount,iQ->s.price,iQ->s.vol);
        }
    }
    if ( iQ->s.quoteid == 0 )
        iQ->s.quoteid = calc_quoteid(iQ);
    else if ( iQ->s.quoteid != calc_quoteid(iQ) )
    {
        printf("bidask_parse quoteid.%llu != calc.%llu\n",(long long)iQ->s.quoteid,(long long)calc_quoteid(iQ));
        return(-1);
    }
    if ( iQ->s.price > SMALLVAL && iQ->s.vol > SMALLVAL && iQ->s.baseid != 0 && iQ->s.relid != 0 )
    {
        buf[0] = 0, _set_assetname(&basemult,buf,0,iQ->s.baseid);
        //printf("baseid.%llu -> %s mult.%llu\n",(long long)iQ->baseid,buf,(long long)basemult);
        buf[0] = 0, _set_assetname(&relmult,buf,0,iQ->s.relid);
        //printf("relid.%llu -> %s mult.%llu\n",(long long)iQ->relid,buf,(long long)relmult);
        //basemult = get_assetmult(iQ->baseid), relmult = get_assetmult(iQ->relid);
        baseamount = (iQ->s.baseamount + basemult/2) / basemult, baseamount *= basemult;
        relamount = (iQ->s.relamount + relmult/2) / relmult, relamount *= relmult;
        if ( iQ->s.price != 0. && iQ->s.vol != 0 )
        {
            price = prices777_price_volume(&volume,baseamount,relamount);
            if ( fabs(iQ->s.price - price)/price > 0.001 )
            {
                printf("cant create accurate price ref.(%f %f) -> (%f %f)\n",iQ->s.price,iQ->s.vol,price,volume);
                return(-1);
            }
        }
    }
    return(0);
}

char *InstantDEX(char *jsonstr,char *remoteaddr,int32_t localaccess)
{
    char *prices777_allorderbooks();
    char *InstantDEX_openorders();
    char *InstantDEX_tradehistory(int32_t firsti,int32_t endi);
    char *InstantDEX_cancelorder(char *activenxt,char *secret,uint64_t sequenceid,uint64_t quoteid);
    struct destbuf exchangestr,method,gui,name,base,rel; double balance;
    char *retstr = 0,key[512],retbuf[1024],*activenxt,*secret,*coinstr; struct InstantDEX_quote iQ; struct exchange_info *exchange;
    cJSON *json; uint64_t assetbits,sequenceid; uint32_t maxdepth; int32_t invert=0,keysize,allfields; struct prices777 *prices;
    //printf("INSTANTDEX.(%s)\n",jsonstr);
    if ( INSTANTDEX.readyflag == 0 )
        return(0);
    if ( jsonstr != 0 && (json= cJSON_Parse(jsonstr)) != 0 )
    {
        // test: asset/asset, asset/external, external/external, autofill and automatch
        // peggy integration
        bidask_parse(&exchangestr,&name,&base,&rel,&gui,&iQ,json);
        if ( iQ.s.offerNXT == 0 )
            iQ.s.offerNXT = SUPERNET.my64bits;
        //printf("isask.%d base.(%s) rel.(%s)\n",iQ.s.isask,base.buf,rel.buf);
        copy_cJSON(&method,jobj(json,"method"));
        if ( (sequenceid= j64bits(json,"orderid")) == 0 )
            sequenceid = j64bits(json,"sequenceid");
        allfields = juint(json,"allfields");
        if ( (maxdepth= juint(json,"maxdepth")) <= 0 )
            maxdepth = MAX_DEPTH;
        if ( exchangestr.buf[0] == 0 )
        {
            if ( iQ.s.baseid != 0 && iQ.s.relid != 0 )
                strcpy(exchangestr.buf,"nxtae");
            else strcpy(exchangestr.buf,"basket");
        }
        assetbits = InstantDEX_name(key,&keysize,exchangestr.buf,name.buf,base.buf,&iQ.s.baseid,rel.buf,&iQ.s.relid);
        //printf("2nd isask.%d base.(%s) rel.(%s)\n",iQ.s.isask,base.buf,rel.buf);
        exchange = exchange_find(exchangestr.buf);
        secret = jstr(json,"secret"), activenxt = jstr(json,"activenxt");
        if ( secret == 0 )
        {
            secret = SUPERNET.NXTACCTSECRET;
            activenxt = SUPERNET.NXTADDR;
        }
        if ( strcmp(method.buf,"allorderbooks") == 0 )
            retstr = prices777_allorderbooks();
        /*else if ( strcmp(method.buf,"coinshuffle") == 0 )
        {
            if ( strcmp(exchangestr.buf,"shuffle") == 0 )
                retstr = InstantDEX_coinshuffle(base.buf,&iQ,json);
            else retstr = clonestr("{\"error\":\"coinshuffle must use shuffle exchange\"}");
        }*/
        else if ( strcmp(method.buf,"openorders") == 0 )
            retstr = InstantDEX_openorders(SUPERNET.NXTADDR,juint(json,"allorders"));
        else if ( strcmp(method.buf,"allexchanges") == 0 )
            retstr = jprint(exchanges_json(),1);
        else if ( strcmp(method.buf,"cancelorder") == 0 )
            retstr = InstantDEX_cancelorder(jstr(json,"activenxt"),jstr(json,"secret"),sequenceid,iQ.s.quoteid);
        else if ( strcmp(method.buf,"orderstatus") == 0 )
            retstr = InstantDEX_orderstatus(sequenceid,iQ.s.quoteid);
        else if ( strcmp(method.buf,"tradehistory") == 0 )
            retstr = InstantDEX_tradehistory(juint(json,"firsti"),juint(json,"endi"));
        else if ( strcmp(method.buf,"lottostats") == 0 )
            retstr = jprint(InstantDEX_lottostats(),1);
        else if ( strcmp(method.buf,"balance") == 0 )
        {
            if ( exchange != 0 && exchange->trade != 0 )
            {
                if ( (coinstr= jstr(json,"base")) != 0 )
                {
                    if ( exchange->coinbalance != 0 )
                    {
                        if ( exchange->balancejson == 0 )
                        {
                            (*exchange->trade)(&retstr,exchange,0,0,0,0,0);
                            if ( retstr != 0 )
                            {
                                exchange->balancejson = cJSON_Parse(retstr);
                                free(retstr);
                            }
                        }
                        return((*exchange->coinbalance)(exchange,&balance,coinstr));
                    }
                    else retstr = clonestr("{\"error\":\"coinbalance missing\"}");
                }
                else (*exchange->trade)(&retstr,exchange,0,0,0,0,0);
            } else retstr = clonestr("{\"error\":\"cant find exchange\"}");
            printf("%s ptr%.p trade.%p\n",exchangestr.buf,exchange,exchange!=0?exchange->trade:0);
        }
        else if ( strcmp(method.buf,"tradesequence") == 0 )
        {
            //printf("call tradesequence.(%s)\n",jsonstr);
            retstr = InstantDEX_tradesequence(activenxt,secret,json);
        }
        else if ( strcmp(method.buf,"makebasket") == 0 )
        {
            if ( (prices= prices777_makebasket(0,json,1,"basket",0,0)) != 0 )
                retstr = clonestr("{\"result\":\"basket made\"}");
            else retstr = clonestr("{\"error\":\"couldnt make basket\"}");
        }
        else if ( strcmp(method.buf,"peggyrates") == 0 )
        {
            if ( SUPERNET.peggy != 0 )
                retstr = peggyrates(juint(json,"timestamp"));
            else retstr = clonestr("{\"error\":\"peggy disabled\"}");
        }
        else if ( strcmp(method.buf,"LSUM") == 0 )
        {
            sprintf(retbuf,"{\"result\":\"%s\",\"amount\":%d}",(rand() & 1) ? "BUY" : "SELL",(rand() % 100) * 100000);
            retstr = clonestr(retbuf);
        }
        else if ( strcmp(method.buf,"placebid") == 0 || strcmp(method.buf,"placeask") == 0 )
            return(InstantDEX_placebidask(0,sequenceid,exchangestr.buf,name.buf,base.buf,rel.buf,&iQ,jstr(json,"extra"),secret,activenxt,json));
        else if ( strcmp(exchangestr.buf,"active") == 0 && strcmp(method.buf,"orderbook") == 0 )
            retstr = prices777_activebooks(name.buf,base.buf,rel.buf,iQ.s.baseid,iQ.s.relid,maxdepth,allfields,strcmp(exchangestr.buf,"active") == 0 || juint(json,"tradeable"));
        else if ( (prices= prices777_find(&invert,iQ.s.baseid,iQ.s.relid,exchangestr.buf)) == 0 )
        {
            if ( (prices= prices777_poll(exchangestr.buf,name.buf,base.buf,iQ.s.baseid,rel.buf,iQ.s.relid)) != 0 )
            {
                if ( prices777_equiv(prices->baseid) == prices777_equiv(iQ.s.baseid) && prices777_equiv(prices->relid) == prices777_equiv(iQ.s.relid) )
                    invert = 0;
                else if ( prices777_equiv(prices->baseid) == prices777_equiv(iQ.s.relid) && prices777_equiv(prices->relid) == prices777_equiv(iQ.s.baseid) )
                    invert = 1;
                else invert = 0, printf("baserel not matching (%s %s) %llu %llu vs (%s %s) %llu %llu\n",prices->base,prices->rel,(long long)prices->baseid,(long long)prices->relid,base.buf,rel.buf,(long long)iQ.s.baseid,(long long)iQ.s.relid);
            }
        }
        if ( retstr == 0 && prices != 0 )
        {
            if ( strcmp(method.buf,"disable") == 0 )
            {
                if ( prices != 0 )
                {
                    if ( strcmp(prices->exchange,"unconf") == 0 )
                        return(clonestr("{\"error\":\"cannot disable unconf\"}"));
                    prices->disabled = 1;
                    return(clonestr("{\"result\":\"success\"}"));
                }
                else return(clonestr("{\"error\":\"no prices to disable\"}"));
            }
            else if ( strcmp(method.buf,"enable") == 0 )
            {
                if ( prices != 0 )
                {
                    prices->disabled = 0;
                    return(clonestr("{\"result\":\"success\"}"));
                }
                else return(clonestr("{\"error\":\"no prices to enable\"}"));
            }
            else if ( strcmp(method.buf,"orderbook") == 0 )
            {
                if ( maxdepth < MAX_DEPTH )
                    return(prices777_orderbook_jsonstr(invert,SUPERNET.my64bits,prices,&prices->O,maxdepth,allfields));
                else if ( (retstr= prices->orderbook_jsonstrs[invert][allfields]) == 0 )
                {
                    retstr = prices777_orderbook_jsonstr(invert,SUPERNET.my64bits,prices,&prices->O,MAX_DEPTH,allfields);
                    portable_mutex_lock(&prices->mutex);
                    if ( prices->orderbook_jsonstrs[invert][allfields] != 0 )
                        free(prices->orderbook_jsonstrs[invert][allfields]);
                    prices->orderbook_jsonstrs[invert][allfields] = retstr;
                    portable_mutex_unlock(&prices->mutex);
                    if ( retstr == 0 )
                        retstr = clonestr("{}");
                }
                if ( retstr != 0 )
                    retstr = clonestr(retstr);
            }
        }
        //if ( Debuglevel > 2 )
            printf("(%s) %p exchange.(%s) base.(%s) %llu rel.(%s) %llu | name.(%s) %llu\n",retstr!=0?retstr:"",prices,exchangestr.buf,base.buf,(long long)iQ.s.baseid,rel.buf,(long long)iQ.s.relid,name.buf,(long long)assetbits);
    }
    return(retstr);
}

char *bidask_func(int32_t localaccess,int32_t valid,char *sender,cJSON *json,char *origargstr)
{
    struct destbuf gui,exchangestr,name,base,rel,offerNXT; struct InstantDEX_quote iQ;
    copy_cJSON(&offerNXT,jobj(json,"offerNXT"));
//printf("got (%s)\n",origargstr);
    if ( strcmp(SUPERNET.NXTADDR,offerNXT.buf) != 0 )
    {
        if ( bidask_parse(&exchangestr,&name,&base,&rel,&gui,&iQ,json) == 0 )
            return(InstantDEX_placebidask(sender,j64bits(json,"orderid"),exchangestr.buf,name.buf,base.buf,rel.buf,&iQ,jstr(json,"extra"),jstr(json,"secret"),jstr(json,"activenxt"),json));
        else printf("error with incoming bidask\n");
    } else fprintf(stderr,"got my bidask from network (%s)\n",origargstr);
    return(clonestr("{\"result\":\"got loopback bidask\"}"));
}

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *argjson)
{
    uint64_t disableflags = 0;
    printf("init %s size.%ld\n",plugin->name,sizeof(struct InstantDEX_info));
    // runtime specific state can be created and put into *data
    return(disableflags); // set bits corresponding to array position in _methods[]
}

void init_exchanges(cJSON *json)
{
    cJSON *array; int32_t i,n;
    for (FIRST_EXTERNAL=0; FIRST_EXTERNAL<sizeof(Supported_exchanges)/sizeof(*Supported_exchanges); FIRST_EXTERNAL++)
    {
        find_exchange(0,Supported_exchanges[FIRST_EXTERNAL]);
        if ( strcmp(Supported_exchanges[FIRST_EXTERNAL],"peggy") == 0 )
        {
            FIRST_EXTERNAL++;
            break;
        }
    }
    for (i=FIRST_EXTERNAL; i<sizeof(Supported_exchanges)/sizeof(*Supported_exchanges); i++)
        find_exchange(0,Supported_exchanges[i]);
    prices777_initpair(-1,0,0,0,0,0.,0,0,0,0);
    if ( (array= jarray(&n,json,"baskets")) != 0 )
    {
        for (i=0; i<n; i++)
            prices777_makebasket(0,jitem(array,i),1,"basket",0,0);
    }
    void prices777_basketsloop(void *ptr);
    portable_thread_create((void *)prices777_basketsloop,0);
    //prices777_makebasket("{\"name\":\"NXT/BTC\",\"base\":\"NXT\",\"rel\":\"BTC\",\"basket\":[{\"exchange\":\"bittrex\"},{\"exchange\":\"poloniex\"},{\"exchange\":\"btc38\"}]}",0);
}

void init_InstantDEX(uint64_t nxt64bits,int32_t testflag,cJSON *json)
{
    int32_t a,b;
    init_pingpong_queue(&Pending_offersQ,"pending_offers",0,0,0);
    Pending_offersQ.offset = 0;
    init_exchanges(json);
    find_exchange(&a,INSTANTDEX_NXTAENAME), find_exchange(&b,INSTANTDEX_NAME);
    if ( a != INSTANTDEX_NXTAEID || b != INSTANTDEX_EXCHANGEID )
        printf("invalid exchangeid %d, %d\n",a,b);
    printf("NXT-> %llu BTC -> %llu\n",(long long)stringbits("NXT"),(long long)stringbits("BTC"));
}

int32_t PLUGNAME(_process_json)(char *forwarder,char *sender,int32_t valid,struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag,char *tokenstr)
{
    char *resultstr,*methodstr,*retstr = 0;
    retbuf[0] = 0;
    if ( Debuglevel > 2 )
        fprintf(stderr,"<<<<<<<<<<<< INSIDE PLUGIN! process %s (%s) initflag.%d\n",plugin->name,jsonstr,initflag);
    if ( initflag > 0 )
    {
        // configure settings
        plugin->allowremote = 1;
        portable_mutex_init(&plugin->mutex);
        init_InstantDEX(calc_nxt64bits(SUPERNET.NXTADDR),0,json);
        INSTANTDEX.history = txinds777_init(SUPERNET.DBPATH,"InstantDEX");
        INSTANTDEX.numhist = (int32_t)INSTANTDEX.history->curitem;
        InstantDEX_inithistory(0,INSTANTDEX.numhist);
        //update_NXT_assettrades();
        INSTANTDEX.readyflag = 1;
        strcpy(retbuf,"{\"result\":\"InstantDEX init\"}");
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        if ( (methodstr= cJSON_str(cJSON_GetObjectItem(json,"method"))) == 0 )
            methodstr = cJSON_str(cJSON_GetObjectItem(json,"requestType"));
        retbuf[0] = 0;
        if ( methodstr == 0 || methodstr[0] == 0 )
        {
            printf("(%s) has not method\n",jsonstr);
            return(0);
        }
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
#ifdef INSIDE_MGW
        else if ( strcmp(methodstr,"msigaddr") == 0 )
        {
            char *devMGW_command(char *jsonstr,cJSON *json);
            if ( SUPERNET.gatewayid >= 0 )
            {
                if ( (retstr= devMGW_command(jsonstr,json)) != 0 )
                {
                    //should_forward(sender,retstr);
                }
            } //else retstr = nn_loadbalanced((uint8_t *)jsonstr,(int32_t)strlen(jsonstr)+1);
        }
#endif
        else if ( strcmp(methodstr,"LSUM") == 0 )
        {
            sprintf(retbuf,"{\"result\":\"%s\",\"amount\":%d}",(rand() & 1) ? "BUY" : "SELL",(rand() % 100) * 100000);
            retstr = clonestr(retbuf);
        }
        else if ( SUPERNET.iamrelay <= 1 )
        {
            if ( strcmp(methodstr,"bid") == 0 || strcmp(methodstr,"ask") == 0 )
                retstr = bidask_func(0,1,sender,json,jsonstr);
            else if ( strcmp(methodstr,"swap") == 0 )
                retstr = swap_func(0,1,sender,json,jsonstr);
            //else if ( strcmp(methodstr,"shuffle") == 0 )
            //    retstr = shuffle_func(0,1,sender,json,jsonstr);
        } else retstr = clonestr("{\"result\":\"relays only relay\"}");
    }
    return(plugin_copyretstr(retbuf,maxlen,retstr));
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    return(retcode);
}
#include "../agents/plugin777.c"
