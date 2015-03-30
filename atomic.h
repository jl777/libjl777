//
//  atomic.h
//
//
//  Created by jl777 on 7/16/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//
// need to verify expirations (deadline + timestamp)

#ifndef xcode_atomic_h
#define xcode_atomic_h
#define INSTANTDEX_NAME "InstantDEX"
#define INSTANTDEX_NXTAENAME "nxtae"
#define INSTANTDEX_EXCHANGEID 0
#define INSTANTDEX_NXTAEID 1
#define INSTANTDEX_MINVOLPERC 0.75
#define INSTANTDEX_PRICESLIPPAGE 0.001
#define INSTANTDEX_NATIVE 0
#define INSTANTDEX_ASSET 1
#define INSTANTDEX_MSCOIN 2
#define INSTANTDEX_UNKNOWN 3
#define ORDERBOOK_EXPIRATION 300
#define _obookid(baseid,relid) ((baseid) ^ (relid))

#define MAX_EXCHANGES 64

#define MAX_TXPTRS 1024
#define INSTANTDEX_TRIGGERDEADLINE 15
#define JUMPTRADE_SECONDS 10
#define INSTANTDEX_ACCT "4383817337783094122"
#define INSTANTDEX_FEE ((long)(2.5 * SATOSHIDEN))
//#define FEEBITS 10

int32_t time_to_nextblock(int32_t lookahead)
{
    /*
     sha256(lastblockgensig+publickey)[0-7] / basetarget * effective balance
     first 8 bytes of sha256 to make a long
     that gives you seconds to forge block
     or you can just look if base target is below 90% and adjust accordingly
     */
    // http://jnxt.org/forge/forgers.json
    return(lookahead * 600); // clearly need to do some calcs
}

union _NXT_tx_num { int64_t amountNQT; int64_t quantityQNT; };

struct NXT_tx
{
    bits256 refhash,sighash,fullhash;
    uint64_t senderbits,recipientbits,assetidbits,txid,priceNQT,quoteid;
    int64_t feeNQT;
    union _NXT_tx_num U;
    int32_t deadline,type,subtype,verify,number;
    uint32_t timestamp;
    char comment[4096];
};

struct orderpair { struct rambook_info *bids,*asks; void (*ramparse)(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui); float lastmilli; };
struct exchange_info { uint64_t nxt64bits; char name[16]; struct orderpair orderpairs[4096]; uint32_t num,exchangeid,lastblock; float lastmilli; } Exchanges[MAX_EXCHANGES];

struct _tradeleg { uint64_t nxt64bits,assetid,amount; };
struct tradeleg { struct _tradeleg src,dest; uint64_t nxt64bits,txid,qty,NXTprice; uint32_t validated,expiration; };
struct jumptrades
{
    uint64_t fee,otherfee,feetxid,otherfeetxid,baseid,relid,basenxtamount,relnxtamount,nxt64bits,other64bits,jump64bits,qtyA,qtyB,jumpqty,balancetxid;
    char comment[MAX_JSON_FIELD],feeutxbytes[MAX_JSON_FIELD],feesignedtx[MAX_JSON_FIELD],triggerhash[65],feesighash[65],numlegs;
    int64_t balances[3][3];
    uint32_t otherexpiration;
    int32_t state;
    struct tradeleg legs[16];
    float endmilli;
    struct NXT_tx *feetx,*balancetx;
    struct InstantDEX_quote *iQ;
} JTRADES[MAX_TXPTRS];

void free_txptrs(struct NXT_tx *txptrs[],int32_t numptrs)
{
    int32_t i;
    if ( txptrs != 0 && numptrs > 0 )
    {
        for (i=0; i<numptrs; i++)
            if ( txptrs[i] != 0 )
                free(txptrs[i]);
    }
}

int32_t search_txptrs(struct NXT_tx *txptrs[],uint64_t txid)
{
    int32_t i; struct NXT_tx *tx;
    for (i=0; i<MAX_TXPTRS; i++)
    {
        if ( (tx= txptrs[i]) == 0 )
            return(-1);
        if ( tx->txid == txid )
            return(i);
    }
    return(-1);
}

double _calc_price_volume(double *volumep,uint64_t baseamount,uint64_t relamount)
{
    *volumep = (((double)baseamount + 0.0000000099) / SATOSHIDEN);
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
        printf("(%llu/%llu) -> %f %f -> (%llu %llu)\n",(long long)baseamount,(long long)relamount,price,vol,(long long)checkbase,(long long)checkrel);
    return(price);
}

double make_jumpquote(uint64_t *baseamountp,uint64_t *relamountp,uint64_t *frombasep,uint64_t *fromrelp,uint64_t *tobasep,uint64_t *torelp)
{
    double p0,v0,p1,v1,price,vol,checkprice,checkvol,ratio;
    *baseamountp = *relamountp = 0;
    p0 = calc_price_volume(&v0,*tobasep,*torelp);
    p1 = calc_price_volume(&v1,*frombasep,*fromrelp);
    if ( p0 > 0. )
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
    } else return(0);
}

int32_t NXTutxcmp(struct NXT_tx *ref,struct NXT_tx *tx,double myshare)
{
    if ( ref->senderbits == tx->senderbits && ref->recipientbits == tx->recipientbits && ref->type == tx->type && ref->subtype == tx->subtype)
    {
        if ( ref->feeNQT != tx->feeNQT || ref->deadline != tx->deadline )
            return(-1);
        if ( ref->assetidbits != NXT_ASSETID )
        {
            if ( ref->assetidbits == tx->assetidbits && fabs((ref->U.quantityQNT*myshare) - tx->U.quantityQNT) < 0.5 && strcmp(ref->comment,tx->comment) == 0 )
                return(0);
        }
        else
        {
            if ( fabs((ref->U.amountNQT*myshare) - tx->U.amountNQT) < 0.5 )
                return(0);
        }
    }
    return(-1);
}

cJSON *gen_NXT_tx_json(struct NXT_tx *utx,char *reftxid,double myshare,char *NXTACCTSECRET,uint64_t nxt64bits)
{
    cJSON *json = 0;
    char cmd[MAX_JSON_FIELD],destNXTaddr[64],assetidstr[64],*retstr;
    if ( utx->senderbits == nxt64bits )
    {
        expand_nxt64bits(destNXTaddr,utx->recipientbits);
        cmd[0] = 0;
        if ( utx->type == 0 && utx->subtype == 0 )
            sprintf(cmd,"%s=sendMoney&amountNQT=%lld",_NXTSERVER,(long long)(utx->U.amountNQT*myshare));
        else
        {
            expand_nxt64bits(assetidstr,utx->assetidbits);
            if ( utx->type == 2 && utx->subtype == 1 )
                sprintf(cmd,"%s=transferAsset&asset=%s&quantityQNT=%lld",_NXTSERVER,assetidstr,(long long)(utx->U.quantityQNT*myshare));
            else if ( utx->type == 5 && utx->subtype == 3 )
                sprintf(cmd,"%s=transferCurrency&currency=%s&units=%lld",_NXTSERVER,assetidstr,(long long)(utx->U.quantityQNT*myshare));
            else
            {
                printf("unsupported type.%d subtype.%d\n",utx->type,utx->subtype);
                return(0);
            }
        }
        if ( utx->comment[0] != 0 )
            strcat(cmd,"&messageIsText=true&message="),strcat(cmd,utx->comment);
        if ( reftxid != 0 && reftxid[0] != 0 && cmd[0] != 0 )
            strcat(cmd,"&referencedTransactionFullHash="),strcat(cmd,reftxid);
        if ( cmd[0] != 0 )
        {
            sprintf(cmd+strlen(cmd),"&deadline=%u&feeNQT=%lld&secretPhrase=%s&recipient=%s&broadcast=false",utx->deadline,(long long)utx->feeNQT,NXTACCTSECRET,destNXTaddr);
            if ( reftxid != 0 && reftxid[0] != 0 )
                sprintf(cmd+strlen(cmd),"&referencedTransactionFullHash=%s",reftxid);
            printf("generated cmd.(%s) reftxid.(%s)\n",cmd,reftxid);
            retstr = issue_NXTPOST(0,cmd);
            if ( retstr != 0 )
            {
                json = cJSON_Parse(retstr);
                //if ( json != 0 )
                 //   printf("Parsed.(%s)\n",cJSON_Print(json));
                free(retstr);
            }
        }
    } else printf("cant gen_NXT_txjson when sender.%llu is not me.%llu\n",(long long)utx->senderbits,(long long)nxt64bits);
    return(json);
}

uint64_t set_NXTtx(uint64_t nxt64bits,struct NXT_tx *tx,uint64_t assetidbits,int64_t amount,uint64_t other64bits,int32_t feebits)
{
    char assetidstr[64];
    uint64_t fee = 0;
    int32_t createdflag;
    struct NXT_tx U;
    struct NXT_asset *ap;
    memset(&U,0,sizeof(U));
    U.senderbits = nxt64bits;
    U.recipientbits = other64bits;
    U.assetidbits = assetidbits;
    if ( feebits >= 0 )
    {
        fee = (amount >> feebits);
        if ( fee == 0 )
            fee = 1;
    }
    if ( assetidbits != NXT_ASSETID )
    {
        expand_nxt64bits(assetidstr,assetidbits);
        ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
        U.type = ap->type;
        U.subtype = ap->subtype;
        U.U.quantityQNT = amount - fee;
    } else U.U.amountNQT = amount - fee;
    U.feeNQT = MIN_NQTFEE;
    U.deadline = INSTANTDEX_TRIGGERDEADLINE;
    printf("set_NXTtx(%llu -> %llu) %.8f of %llu\n",(long long)U.senderbits,(long long)U.recipientbits,dstr(amount),(long long)assetidbits);
    *tx = U;
    return(fee);
}

/*void truncate_utxbytes(char *utxbytes)
{
    long i,n;
    n = strlen(utxbytes);
    if ( n > 128 )
    {
        for (i=n-128; i<n; i++)
            if ( utxbytes[i] != '0' )
                printf("calc_raw_NXTtx: unexpected non-'0' byte (%c) %d\n",utxbytes[i],utxbytes[i]);
        utxbytes[n-128] = 0;
        //printf("truncate n.%ld to %ld\n",n,n-128);
    } else printf("utxlen.%ld\n",n);
}*/

int32_t calc_raw_NXTtx(char *utxbytes,char *sighash,uint64_t assetidbits,int64_t amount,uint64_t other64bits,char *NXTACCTSECRET,uint64_t nxt64bits)
{
    int32_t retval = -1;
    struct NXT_tx U;
    cJSON *json;
    utxbytes[0] = sighash[0] = 0;
    set_NXTtx(nxt64bits,&U,assetidbits,amount,other64bits,-1);
    json = gen_NXT_tx_json(&U,0,1.,NXTACCTSECRET,nxt64bits);
    if ( json != 0 )
    {
        if ( extract_cJSON_str(utxbytes,MAX_JSON_FIELD,json,"transactionBytes") > 0 && extract_cJSON_str(sighash,1024,json,"signatureHash") > 0 )
        {
            retval = 0;
            printf("generated utx.(%s) sighash.(%s)\n",utxbytes,sighash);
        } else printf("missing tx or sighash.(%s)\n",cJSON_Print(json));
        free_json(json);
    } else printf("calc_raw_NXTtx error doing gen_NXT_tx_json\n");
    return(retval);
}

struct NXT_tx *set_NXT_tx(cJSON *json)
{
    long size;
    int32_t n = 0;
    uint64_t assetidbits,quantity,price;
    cJSON *attachmentobj;
    struct NXT_tx *utx = 0;
    char sender[1024],recipient[1024],deadline[1024],feeNQT[1024],amountNQT[1024],type[1024],subtype[1024],verify[1024],referencedTransaction[1024],quantityQNT[1024],priceNQT[1024],comment[1024],assetidstr[1024],sighash[1024],fullhash[1024],timestamp[1024],transaction[1024];
    if ( json == 0 )
        return(0);
    if ( extract_cJSON_str(sender,sizeof(sender),json,"sender") > 0 ) n++;
    if ( extract_cJSON_str(recipient,sizeof(recipient),json,"recipient") > 0 ) n++;
    if ( extract_cJSON_str(referencedTransaction,sizeof(referencedTransaction),json,"referencedTransactionFullHash") > 0 ) n++;
    if ( extract_cJSON_str(amountNQT,sizeof(amountNQT),json,"amountNQT") > 0 ) n++;
    if ( extract_cJSON_str(feeNQT,sizeof(feeNQT),json,"feeNQT") > 0 ) n++;
    if ( extract_cJSON_str(deadline,sizeof(deadline),json,"deadline") > 0 ) n++;
    if ( extract_cJSON_str(type,sizeof(type),json,"type") > 0 ) n++;
    if ( extract_cJSON_str(subtype,sizeof(subtype),json,"subtype") > 0 ) n++;
    if ( extract_cJSON_str(verify,sizeof(verify),json,"verify") > 0 ) n++;
    if ( extract_cJSON_str(sighash,sizeof(sighash),json,"signatureHash") > 0 ) n++;
    if ( extract_cJSON_str(fullhash,sizeof(fullhash),json,"fullHash") > 0 ) n++;
    if ( extract_cJSON_str(timestamp,sizeof(timestamp),json,"timestamp") > 0 ) n++;
    if ( extract_cJSON_str(transaction,sizeof(transaction),json,"transaction") > 0 ) n++;
    comment[0] = 0;
    assetidbits = NXT_ASSETID;
    quantity = price = 0;
    size = sizeof(*utx);
    if ( strcmp(type,"2") == 0 || strcmp(type,"5") == 0 )//&& strcmp(subtype,"3") == 0) )
    {
        attachmentobj = cJSON_GetObjectItem(json,"attachment");
        if ( attachmentobj != 0 )
        {
            if ( extract_cJSON_str(assetidstr,sizeof(assetidstr),attachmentobj,"asset") > 0 )
                assetidbits = calc_nxt64bits(assetidstr);
            else if ( extract_cJSON_str(assetidstr,sizeof(assetidstr),attachmentobj,"currency") > 0 )
                assetidbits = calc_nxt64bits(assetidstr);
            if ( extract_cJSON_str(comment,sizeof(comment),attachmentobj,"message") > 0 )
                size += strlen(comment);
            if ( extract_cJSON_str(quantityQNT,sizeof(quantityQNT),attachmentobj,"quantityQNT") > 0 )
                quantity = calc_nxt64bits(quantityQNT);
            else if ( extract_cJSON_str(quantityQNT,sizeof(quantityQNT),attachmentobj,"units") > 0 )
                quantity = calc_nxt64bits(quantityQNT);
            if ( extract_cJSON_str(priceNQT,sizeof(priceNQT),attachmentobj,"priceNQT") > 0 )
                price = calc_nxt64bits(priceNQT);
        }
    }
    utx = malloc(size);
    memset(utx,0,size);
    if ( strlen(referencedTransaction) == 64 )
        decode_hex(utx->refhash.bytes,32,referencedTransaction);
    if ( strlen(fullhash) == 64 )
        decode_hex(utx->fullhash.bytes,32,fullhash);
    if ( strlen(sighash) == 64 )
        decode_hex(utx->sighash.bytes,32,sighash);
    utx->txid = calc_nxt64bits(transaction);
    utx->senderbits = calc_nxt64bits(sender);
    utx->recipientbits = calc_nxt64bits(recipient);
    utx->assetidbits = assetidbits;
    utx->feeNQT = calc_nxt64bits(feeNQT);
    if ( quantity != 0 )
        utx->U.quantityQNT = quantity;
    else utx->U.amountNQT = calc_nxt64bits(amountNQT);
    utx->priceNQT = price;
    utx->deadline = atoi(deadline);
    utx->type = atoi(type);
    utx->subtype = atoi(subtype);
    utx->timestamp = atoi(timestamp);
    utx->verify = (strcmp("true",verify) == 0);
    strcpy(utx->comment,comment);
    unstringify(utx->comment);
    return(utx);
}

struct NXT_tx *sign_NXT_tx(char utxbytes[MAX_JSON_FIELD],char signedtx[MAX_JSON_FIELD],char *NXTACCTSECRET,uint64_t nxt64bits,struct NXT_tx *utx,char *reftxid,double myshare)
{
    cJSON *refjson,*txjson;
    char *parsed,*str,errstr[MAX_JSON_FIELD],_utxbytes[MAX_JSON_FIELD];
    struct NXT_tx *refutx = 0;
    printf("sign_NXT_tx.%llu  reftxid.(%s)\n",(long long)nxt64bits,reftxid);
    txjson = gen_NXT_tx_json(utx,reftxid,myshare,NXTACCTSECRET,nxt64bits);
    if ( utxbytes == 0 )
        utxbytes = _utxbytes;
    signedtx[0] = 0;
    if ( txjson != 0 )
    {
        if ( extract_cJSON_str(errstr,sizeof(errstr),txjson,"errorCode") > 0 )
        {
            str = cJSON_Print(txjson);
            strcpy(signedtx,str);
            strcpy(utxbytes,errstr);
            free(str);
        }
        else if ( extract_cJSON_str(utxbytes,MAX_JSON_FIELD,txjson,"unsignedTransactionBytes") > 0 && extract_cJSON_str(signedtx,MAX_JSON_FIELD,txjson,"transactionBytes") > 0 )
        {
            printf("signedbytes.(%s)\n",signedtx);
            if ( (parsed= issue_parseTransaction(0,signedtx)) != 0 )
            {
                refjson = cJSON_Parse(parsed);
                if ( refjson != 0 )
                {
                    refutx = set_NXT_tx(refjson);
                    free_json(refjson);
                }
                free(parsed);
            }
        }
        free_json(txjson);
    }
    return(refutx);
}

int32_t equiv_NXT_tx(struct NXT_tx *tx,char *comment)
{
    cJSON *json;
    uint64_t assetA,assetB,qtyA,qtyB,asset,qty;
    if ( (json= cJSON_Parse(comment)) != 0 )
    {
        assetA = get_satoshi_obj(json,"assetA");
        qtyA = get_satoshi_obj(json,"qtyA");
        assetB = get_satoshi_obj(json,"assetB");
        qtyB = get_satoshi_obj(json,"qtyB");
        free_json(json);
        if ( assetA != 0 && qtyA != 0 )
        {
            asset = assetA;
            qty = qtyA;
        }
        else if ( assetB != 0 && qtyB != 0 )
        {
            asset = assetB;
            qty = qtyB;
        } else return(-2);
        printf("tx->assetbits %llu vs asset.%llu\n",(long long)tx->assetidbits,(long long)asset);
        if ( tx->assetidbits != asset )
            return(-3);
        if ( tx->U.quantityQNT != qty ) // tx->quantityQNT is union as long as same assetid, then these can be compared directly
            return(-4);
        printf("tx->U.quantityQNT %llu vs qty.%llu\n",(long long)tx->U.quantityQNT,(long long)qty);
        return(0);
    }
    printf("error parsing.(%s)\n",comment);
    return(-1);
}

struct NXT_tx *conv_txbytes(char *txbytes)
{
    struct NXT_tx *tx = 0;
    char *parsed;
    cJSON *json;
    if ( (parsed = issue_parseTransaction(0,txbytes)) != 0 )
    {
        if ( (json= cJSON_Parse(parsed)) != 0 )
        {
            tx = set_NXT_tx(json);
            free_json(json);
        }
        free(parsed);
    }
    return(tx);
}

char *respondtx(char *sender,char *signedtx,uint64_t feeB,char *feetxidstr,uint64_t quoteid)
{
    // RESPONSETX.({"fullHash":"210f0b0f6e817897929dc4a0a83666246287925c742a6a8a1613626fa5662d16","signatureHash":"3f8e42ba625f78c9f741501a83d86db4ed0dba2af5ab60315a5f7b01d0f8b737","transaction":"10914616006631165729","amountNQT":"0","verify":true,"attachment":{"asset":"7631394205089352260","quantityQNT":"1000","comment":"{\"assetA\":\"7631394205089352260\",\"qtyA\":\"1000\"}"},"recipientRS":"NXT-CWEE-VXCV-697E-9YKJT","type":2,"feeNQT":"100000000","recipient":"8989816935121514892","sender":"8989816935121514892","timestamp":20877092,"height":2147483647,"subtype":1,"senderPublicKey":"25c5fed2690701cf06f267e7c227b1a3c0dfa9c6fc3cdb593b3af6f16d65302f","deadline":720,"senderRS":"NXT-CWEE-VXCV-697E-9YKJT","signature":"005b7022a385932cabb7d3dd2b2d51d585f9b8f3ece8837746209a08d623cc0fb3f3c76107ec3ce01d7bb7093befd0421756bea4b1caa075766733f9a76d4193"})
    char otherNXTaddr[64],NXTaddr[64],buf[MAX_JSON_FIELD],*pendingtxbytes;
    uint64_t othertxid,mytxid;
    int32_t createdflag,errcode;
    struct NXT_acct *othernp;
    struct NXT_tx *pendingtx,*recvtx;
    sprintf(buf,"{\"error\":\"some error with respondtx got (%s) from NXT.%s\"}",signedtx,sender);
    printf("RESPONDTX.(%s) from (%s)\n",signedtx,sender);
    recvtx = conv_txbytes(signedtx);
    if ( recvtx != 0 )
    {
        expand_nxt64bits(otherNXTaddr,recvtx->senderbits);
        othernp = get_NXTacct(&createdflag,Global_mp,otherNXTaddr);
        expand_nxt64bits(NXTaddr,recvtx->recipientbits);
        //np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
        if ( (pendingtxbytes= othernp->signedtx) != 0 && strcmp(NXTaddr,Global_mp->myNXTADDR) == 0 )
        {
            pendingtx = conv_txbytes(pendingtxbytes);
            if ( pendingtx != 0 && pendingtx->senderbits == recvtx->recipientbits && pendingtx->recipientbits == recvtx->senderbits )
            {
                if ( recvtx->verify != 0 && memcmp(pendingtx->fullhash.bytes,recvtx->refhash.bytes,sizeof(pendingtx->fullhash)) == 0 )
                {
                    if ( equiv_NXT_tx(recvtx,pendingtx->comment) == 0 && equiv_NXT_tx(pendingtx,recvtx->comment) == 0 )
                    {
                        sprintf(buf,"{\"error\":\"error broadcasting tx\"}");
                        othertxid = issue_broadcastTransaction(&errcode,0,signedtx,Global_mp->srvNXTACCTSECRET);
                        if ( othertxid != 0 && errcode == 0 )
                        {
                            mytxid = issue_broadcastTransaction(&errcode,0,pendingtxbytes,Global_mp->srvNXTACCTSECRET);
                            if ( mytxid != 0 && errcode == 0 )
                            {
                                sprintf(buf,"{\"result\":\"tradecompleted\",\"txid\":\"%llu\",\"signedtx\":\"%s\",\"othertxid\":\"%llu\",\"quoteid\":\"%llu\"}",(long long)mytxid,pendingtxbytes,(long long)othertxid,(long long)quoteid);
                                printf("TRADECOMPLETE.(%s)\n",buf);
                                free(othernp->signedtx);
                                othernp->signedtx = 0;
                            } else printf("TRADE ERROR.%d mytxid.%llu\n",errcode,(long long)mytxid);
                        } else printf("TRADE ERROR.%d othertxid.%llu\n",errcode,(long long)othertxid);
                    } else sprintf(buf,"{\"error\":\"pendingtx for NXT.%s (%s) doesnt match received tx (%s)\"}",otherNXTaddr,pendingtxbytes,signedtx);
                } else sprintf(buf,"{\"error\":\"refhash != fullhash from NXT.%s or unsigned.%d\"}",otherNXTaddr,recvtx->verify);
                free(pendingtx);
            } else sprintf(buf,"{\"error\":\"mismatched sender/recipient NXT.%s <-> NXT.%s\"}",otherNXTaddr,NXTaddr);
        } else sprintf(buf,"{\"error\":\"no pending tx with (%s) or cant access account NXT.%s\"}",otherNXTaddr,NXTaddr);
        free(recvtx);
    }
    printf("RETVAL.(%s)\n",buf);
    return(clonestr(buf));
}

uint64_t send_feetx(uint64_t assetbits,uint64_t fee,char *fullhash,char *comment)
{
    char feeutx[MAX_JSON_FIELD],signedfeetx[MAX_JSON_FIELD];
    struct NXT_tx feeT,*feetx;
    uint64_t feetxid = 0;
    int32_t errcode;
    set_NXTtx(calc_nxt64bits(Global_mp->myNXTADDR),&feeT,assetbits,fee,calc_nxt64bits(INSTANTDEX_ACCT),-1);
    if ( comment != 0 && comment[0] != 0 )
        strcpy(feeT.comment,comment);
    printf("feetx for %llu %.8f fullhash.(%s) secret.(%s)\n",(long long)Global_mp->myNXTADDR,dstr(fee),fullhash,Global_mp->srvNXTACCTSECRET);
    if ( (feetx= sign_NXT_tx(feeutx,signedfeetx,Global_mp->srvNXTACCTSECRET,calc_nxt64bits(Global_mp->myNXTADDR),&feeT,fullhash,1.)) != 0 )
    {
        printf("broadcast fee for %llu\n",(long long)assetbits);
        feetxid = issue_broadcastTransaction(&errcode,0,signedfeetx,Global_mp->srvNXTACCTSECRET);
        free(feetx);
    }
    return(feetxid);
}

char *processutx(char *sender,char *utx,char *sig,char *full,uint64_t feeAtxid,uint64_t quoteid)
{
    struct InstantDEX_quote *order_match(uint64_t nxt64bits,uint64_t baseid,uint64_t baseqty,uint64_t othernxtbits,uint64_t relid,uint64_t relqty,uint64_t relfee,uint64_t relfeetxid,char *fullhash);
   //PARSED OFFER.({"sender":"8989816935121514892","timestamp":20810867,"height":2147483647,"amountNQT":"0","verify":false,"subtype":1,"attachment":{"asset":"7631394205089352260","quantityQNT":"1000","comment":"{\"assetB\":\"1639299849328439538\",\"qtyB\":\"1000000\"}"},"recipientRS":"NXT-CWEE-VXCV-697E-9YKJT","feeNQT":"100000000","senderPublicKey":"25c5fed2690701cf06f267e7c227b1a3c0dfa9c6fc3cdb593b3af6f16d65302f","type":2,"deadline":720,"senderRS":"NXT-CWEE-VXCV-697E-9YKJT","recipient":"8989816935121514892"})
    struct NXT_tx T,*tx;
    struct InstantDEX_quote *iQ;
    cJSON *json,*obj,*offerjson,*commentobj;
    struct NXT_tx *offertx;
    double vol,price,amountB;
    uint64_t qtyB,assetB,fee,feeA,feetxid = 0;
    char *jsonstr,*parsed,hopNXTaddr[64],buf[MAX_JSON_FIELD],calchash[MAX_JSON_FIELD],NXTaddr[64],responseutx[MAX_JSON_FIELD],signedtx[MAX_JSON_FIELD],otherNXTaddr[64];
    printf("PROCESSUTX.(%s) sig.%s full.%s from (%s)\n",utx,sig,full,sender);
    jsonstr = issue_calculateFullHash(0,utx,sig);
    if ( jsonstr != 0 )
    {
        json = cJSON_Parse(jsonstr);
        if ( json != 0 )
        {
            obj = cJSON_GetObjectItem(json,"fullHash");
            copy_cJSON(calchash,obj);
            if ( strcmp(calchash,full) == 0 )
            {
                if ( (parsed = issue_parseTransaction(0,utx)) != 0 )
                {
                    stripwhite_ns(parsed,strlen(parsed));
                    printf("PARSED OFFER.(%s) full.(%s) (%s) offer sender.%s\n",parsed,full,calchash,sender);
                    if ( (offerjson= cJSON_Parse(parsed)) != 0 )
                    {
                        offertx = set_NXT_tx(offerjson);
                        expand_nxt64bits(otherNXTaddr,offertx->senderbits);
                        vol = conv_assetoshis(offertx->assetidbits,offertx->U.quantityQNT);
                        //printf("other.(%s) vol %f\n",otherNXTaddr,vol);
                        if ( vol != 0. && offertx->comment[0] != 0 )
                        {
                            commentobj = cJSON_Parse(offertx->comment);
                            if ( commentobj != 0 )
                            {
                                assetB = get_satoshi_obj(commentobj,"assetB");
                                qtyB = get_satoshi_obj(commentobj,"qtyB");
                                feeA = get_satoshi_obj(commentobj,"feeA");
                                //feeAtxid = get_satoshi_obj(commentobj,"feeAtxid");
                                amountB = conv_assetoshis(assetB,qtyB);
                                price = (amountB / vol);
                                printf("assetB.%llu qtyB.%llu feeA.%llu feeAtxid.%llu | %.8f amountB %.8f qty %llu\n",(long long)assetB,(long long)qtyB,(long long)feeA,(long long)feeAtxid,price,amountB,(long long)offertx->U.quantityQNT);
                                free_json(commentobj);
                                if ( (iQ= order_match(offertx->recipientbits,assetB,qtyB,offertx->senderbits,offertx->assetidbits,offertx->U.quantityQNT,feeA,feeAtxid,full)) != 0 )
                                {
                                    fee = set_NXTtx(offertx->recipientbits,&T,assetB,qtyB,offertx->senderbits,-1);//FEEBITS);
                                    fee = INSTANTDEX_FEE;
                                    //feetxid = send_feetx(assetB,fee,full);
                                    sprintf(T.comment,"{\"obookid\":\"%llu\",\"assetA\":\"%llu\",\"qtyA\":\"%llu\",\"feeB\":\"%llu\",\"feetxid\":\"%llu\"}",(long long)(offertx->assetidbits ^ assetB),(long long)offertx->assetidbits,(long long)offertx->U.quantityQNT,(long long)fee,(long long)feetxid);
                                    feetxid = send_feetx(NXT_ASSETID,fee,full,T.comment);
                                    sprintf(T.comment,"{\"obookid\":\"%llu\",\"assetA\":\"%llu\",\"qtyA\":\"%llu\",\"feeB\":\"%llu\",\"feetxid\":\"%llu\"}",(long long)(offertx->assetidbits ^ assetB),(long long)offertx->assetidbits,(long long)offertx->U.quantityQNT,(long long)fee,(long long)feetxid);
                                    printf("processutx.(%s) -> (%llu) price %.8f vol %.8f\n",T.comment,(long long)offertx->recipientbits,price,vol);
                                    expand_nxt64bits(NXTaddr,offertx->recipientbits);
                                    if ( strcmp(Global_mp->myNXTADDR,NXTaddr) == 0 )
                                    {
                                        tx = sign_NXT_tx(responseutx,signedtx,Global_mp->srvNXTACCTSECRET,offertx->recipientbits,&T,full,1.);
                                        if ( tx != 0 )
                                        {
                                            sprintf(buf,"{\"requestType\":\"respondtx\",\"NXT\":\"%s\",\"signedtx\":\"%s\",\"timestamp\":%ld,\"feeB\":\"%llu\",\"feetxid\":\"%llu\",\"quoteid\":\"%llu\"}",NXTaddr,signedtx,time(NULL),(long long)fee,(long long)feetxid,(long long)quoteid);
                                            hopNXTaddr[0] = 0;
                                            send_tokenized_cmd(!prevent_queueing("respondtx"),hopNXTaddr,0,NXTaddr,Global_mp->srvNXTACCTSECRET,buf,otherNXTaddr);
                                            free(tx);
                                            iQ->sent = 1;
                                            printf("send (%s) to %s\n",buf,otherNXTaddr);
                                            sprintf(buf,"{\"results\":\"utx from NXT.%llu accepted with fullhash.(%s) %.8f of %llu for %.8f of %llu -> price %.8f\"}",(long long)offertx->senderbits,full,vol,(long long)offertx->assetidbits,amountB,(long long)assetB,price);
                                        }
                                        else sprintf(buf,"{\"error\":\"from %s error signing responsutx.(%s)\"}",otherNXTaddr,NXTaddr);
                                    } else sprintf(buf,"{\"error\":\"cant send response to %s, no access to acct %s\"}",otherNXTaddr,NXTaddr);
                                } else sprintf(buf,"{\"error\":\"nothing matches offer from %s\"}",otherNXTaddr);
                            }
                            else sprintf(buf,"{\"error\":\"%s error parsing comment comment.(%s)\"}",otherNXTaddr,offertx->comment);
                        }
                        else sprintf(buf,"{\"error\":\"%s missing comment.(%s) or zero vol %.8f\"}",otherNXTaddr,parsed,vol);
                        free_json(offerjson);
                    }
                    else sprintf(buf,"{\"error\":\"%s cant json parse offer.(%s)\"}",otherNXTaddr,parsed);
                    free(parsed);
                }
                else sprintf(buf,"{\"error\":\"%s cant parse processutx.(%s)\"}",otherNXTaddr,utx);
            }
            free_json(json);
        }
        else sprintf(buf,"{\"error\":\"cant parse calcfullhash results.(%s) from %s\"}",jsonstr,otherNXTaddr);
        free(jsonstr);
    }
    else sprintf(buf,"{\"error\":\"processutx cant issue calcfullhash\"}");
    printf("PROCESS RETURNS.(%s)\n",buf);
    return(clonestr(buf));
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

char *submit_atomic_txfrag(char *apicmd,char *cmdstr,char *NXTaddr,char *NXTACCTSECRET,char *destNXTaddr)
{
    long n;
    char _tokbuf[4096],hopNXTaddr[64];
    n = construct_tokenized_req(_tokbuf,cmdstr,NXTACCTSECRET);
    hopNXTaddr[0] = 0;
    printf("send.(%s) to NXT.%s\n",_tokbuf,destNXTaddr);
    return(sendmessage(!prevent_queueing(apicmd),hopNXTaddr,0,NXTaddr,_tokbuf,(int32_t)n,destNXTaddr,0,0));
}

char *makeoffer(char *verifiedNXTaddr,char *NXTACCTSECRET,char *otherNXTaddr,uint64_t assetA,uint64_t qtyA,uint64_t assetB,uint64_t qtyB,int32_t type,uint64_t quoteid)
{
    char hopNXTaddr[64],buf[MAX_JSON_FIELD],signedtx[MAX_JSON_FIELD],utxbytes[MAX_JSON_FIELD],sighash[65],fullhash[65];
    struct NXT_tx T,*tx;
    struct NXT_acct *othernp;
    long i,n;
    int32_t createdflag;
    uint64_t nxt64bits,other64bits,assetoshisA,assetoshisB,fee,feetxid = 0;
    hopNXTaddr[0] = 0;
    find_NXTacct(verifiedNXTaddr,NXTACCTSECRET);
    nxt64bits = calc_nxt64bits(verifiedNXTaddr);
    other64bits = calc_nxt64bits(otherNXTaddr);
    assetoshisA = calc_assetoshis(assetA,qtyA);
    assetoshisB = calc_assetoshis(assetB,qtyB);
    othernp = get_NXTacct(&createdflag,Global_mp,otherNXTaddr);
    if ( othernp->signedtx != 0 )
    {
        printf("cancelling preexisting trade with NXT.%s (%s)\n",otherNXTaddr,othernp->signedtx);
        if ( othernp->signedtx != 0 )
            free(othernp->signedtx);
        othernp->signedtx = 0;
    }
    printf("assetoshis A %llu B %llu\n",(long long)assetoshisA,(long long)assetoshisB);
    if ( assetoshisA == 0 || assetoshisB == 0 || assetA == assetB )
    {
        sprintf(buf,"{\"error\":\"%s\",\"descr\":\"NXT.%llu makeoffer to NXT.%s %.8f asset.%llu for %.8f asset.%llu, type.%d\"","illegal parameter",(long long)nxt64bits,otherNXTaddr,dstr(assetoshisA),(long long)assetA,dstr(assetoshisB),(long long)assetB,type);
        return(clonestr(buf));
    }
    fee = set_NXTtx(nxt64bits,&T,assetA,assetoshisA,other64bits,-1);//FEEBITS);
    fee = INSTANTDEX_FEE;
    sprintf(T.comment,"{\"obookid\":\"%llu\",\"assetB\":\"%llu\",\"qtyB\":\"%llu\",\"feeA\":\"%llu\"}",(long long)(assetA ^ assetB),(long long)assetB,(long long)assetoshisB,(long long)fee);
    tx = sign_NXT_tx(utxbytes,signedtx,NXTACCTSECRET,nxt64bits,&T,0,1.);
    if ( tx != 0 )
    {
        init_hexbytes_noT(sighash,tx->sighash.bytes,sizeof(tx->sighash));
        init_hexbytes_noT(fullhash,tx->fullhash.bytes,sizeof(tx->fullhash));
        //feetxid = send_feetx(assetA,fee,fullhash);
        sprintf(buf,"{\"requestType\":\"processutx\",\"NXT\":\"%s\",\"utx\":\"%s\",\"sig\":\"%s\",\"full\":\"%s\",\"timestamp\":%ld,\"feeA\":\"%llu\",\"feeAtxid\":\"%llu\",\"quoteid\":\"%llu\"}",verifiedNXTaddr,utxbytes,sighash,fullhash,time(NULL),(long long)INSTANTDEX_FEE,(long long)feetxid,(long long)quoteid);
        feetxid = send_feetx(NXT_ASSETID,INSTANTDEX_FEE,fullhash,buf);
        sprintf(buf,"{\"requestType\":\"processutx\",\"NXT\":\"%s\",\"utx\":\"%s\",\"sig\":\"%s\",\"full\":\"%s\",\"timestamp\":%ld,\"feeA\":\"%llu\",\"feeAtxid\":\"%llu\",\"quoteid\":\"%llu\"}",verifiedNXTaddr,utxbytes,sighash,fullhash,time(NULL),(long long)INSTANTDEX_FEE,(long long)feetxid,(long long)quoteid);
        free(tx);
        if ( 0 )
        {
            n = strlen(utxbytes);
            for (i=n; i<n+128; i++)
                utxbytes[i] = '0';
            utxbytes[i] = 0;
            char *tmp = issue_parseTransaction(0,utxbytes);
            printf("(%s)\n",tmp);
            free(tmp);
        }
        return(submit_atomic_txfrag("processutx",buf,verifiedNXTaddr,NXTACCTSECRET,otherNXTaddr));
        //n = construct_tokenized_req(_tokbuf,buf,NXTACCTSECRET);
       // othernp->signedtx = clonestr(signedtx);
       // hopNXTaddr[0] = 0;
       // printf("send.(%s) to NXT.%s\n",_tokbuf,otherNXTaddr);
       // return(sendmessage(!prevent_queueing("processutx"),hopNXTaddr,0,verifiedNXTaddr,_tokbuf,(int32_t)n,otherNXTaddr,0,0));
    }
    else sprintf(buf,"{\"error\":\"%s\",\"descr\":\"%s\",\"comment\":\"NXT.%llu makeoffer to NXT.%s %.8f asset.%llu for %.8f asset.%llu, type.%d\"",utxbytes,signedtx,(long long)nxt64bits,otherNXTaddr,dstr(assetoshisA),(long long)assetA,dstr(assetoshisB),(long long)assetB,type);
    return(clonestr(buf));
}

cJSON *_tradeleg_json(struct _tradeleg *halfleg)
{
    uint32_t set_assetname(uint64_t *multp,char *name,uint64_t assetbits);
    cJSON *json = cJSON_CreateObject();
    char numstr[64],name[64];
    uint64_t mult;
    sprintf(numstr,"%llu",(long long)halfleg->nxt64bits), cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)halfleg->assetid), cJSON_AddItemToObject(json,"assetid",cJSON_CreateString(numstr));
    set_assetname(&mult,name,halfleg->assetid), cJSON_AddItemToObject(json,"name",cJSON_CreateString(name));
    sprintf(numstr,"%.8f",dstr(halfleg->amount)), cJSON_AddItemToObject(json,"amount",cJSON_CreateString(numstr));
    return(json);
}

cJSON *tradeleg_json(struct tradeleg *leg)
{
    cJSON *json = cJSON_CreateObject();
    char numstr[64];
    cJSON_AddItemToObject(json,"src",_tradeleg_json(&leg->src));
    cJSON_AddItemToObject(json,"dest",_tradeleg_json(&leg->dest));
    sprintf(numstr,"%llu",(long long)leg->nxt64bits), cJSON_AddItemToObject(json,"sender",cJSON_CreateString(numstr));
    if ( leg->txid != 0 )
        sprintf(numstr,"%llu",(long long)leg->txid), cJSON_AddItemToObject(json,"txid",cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)leg->qty), cJSON_AddItemToObject(json,"qty",cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",dstr(leg->NXTprice)), cJSON_AddItemToObject(json,"price",cJSON_CreateString(numstr));
    if ( leg->expiration != 0 )
        cJSON_AddItemToObject(json,"expiration",cJSON_CreateNumber(leg->expiration - time(NULL)));
    return(json);
}

cJSON *jumptrade_json(struct jumptrades *jtrades)
{
    cJSON *array = cJSON_CreateArray(),*json;
    int32_t i;
    if ( (json= cJSON_Parse(jtrades->comment)) != 0 )
    {
        for (i=0; i<jtrades->numlegs; i++)
            cJSON_AddItemToArray(array,tradeleg_json(&jtrades->legs[i]));
        cJSON_AddItemToObject(json,"legs",array);
        cJSON_AddItemToObject(json,"numlegs",cJSON_CreateNumber(jtrades->numlegs));
        cJSON_AddItemToObject(json,"state",cJSON_CreateNumber(jtrades->state));
        cJSON_AddItemToObject(json,"expiration",cJSON_CreateNumber((milliseconds() - jtrades->endmilli) / 1000.));
    } else json = cJSON_CreateObject();
    return(json);
}

cJSON *jumptrades_json()
{
    cJSON *array = 0,*json = cJSON_CreateObject();
    struct jumptrades *jtrades = 0;
    int32_t i;
    for (i=0; i<(int32_t)(sizeof(JTRADES)/sizeof(*JTRADES)); i++)
    {
        jtrades = &JTRADES[i];
        if ( jtrades->state != 0 )
        {
            if ( array == 0 )
                array = cJSON_CreateArray();
            cJSON_AddItemToArray(array,jumptrade_json(jtrades));
        }
    }
    if ( array != 0 )
        cJSON_AddItemToObject(json,"jumptrades",array);
    return(json);
}

void purge_jumptrades(struct jumptrades *jtrades)
{
    printf("purge_jumptrades\n");
    if ( jtrades->feetx != 0 )
        free(jtrades->feetx);
    if ( jtrades->balancetx != 0 )
        free(jtrades->balancetx);
    memset(jtrades,0,sizeof(*jtrades));
    //free(jtrades);
}

struct jumptrades *recycle_jumptrade(int32_t *lastip,struct jumptrades *jtrades,int32_t ind)
{
    purge_jumptrades(jtrades);
    jtrades->state = 1;
    *lastip = ind;
    return(jtrades);
}

struct jumptrades *get_jumptrade()
{
    static int32_t lasti;
    int32_t i,ind,firstused = -1;
    struct jumptrades *jtrades = 0;
    for (i=0; i<(int32_t)(sizeof(JTRADES)/sizeof(*JTRADES)); i++)
    {
        ind = (i + lasti) % ((int32_t)(sizeof(JTRADES)/sizeof(*JTRADES)));
        jtrades = &JTRADES[ind];
        if ( jtrades->state == 0 )
            return(recycle_jumptrade(&lasti,jtrades,ind));
        else if ( jtrades->state < 0 && firstused < 0 )
            firstused = ind;
    }
    if ( firstused >= 0 )
        return(recycle_jumptrade(&lasti,&JTRADES[firstused],firstused));
    return(jtrades);
}

void get_txhashes(char *sighash,char *fullhash,struct NXT_tx *tx)
{
    init_hexbytes_noT(sighash,tx->sighash.bytes,sizeof(tx->sighash));
    init_hexbytes_noT(fullhash,tx->fullhash.bytes,sizeof(tx->fullhash));
}

uint64_t calc_nxtmedianprice(uint64_t assetid)
{
    uint64_t highbid,lowask;
    highbid = get_nxthighbid(0,assetid);
    lowask = get_nxtlowask(0,assetid);
    if ( highbid != 0 && lowask != 0 )
        return((highbid + lowask) >> 1);
    return(get_nxtlastprice(assetid));
}

uint64_t calc_nxtprice(struct jumptrades *jtrades,uint64_t assetid,uint64_t amount)
{
    uint64_t *ptr = 0,nxtprice,nxtamount = 0;
    printf("calc_nxtprice\n");
    if ( jtrades->baseid == assetid )
    {
        if ( (nxtamount= jtrades->basenxtamount) != 0 )
            return(nxtamount);
        ptr = &jtrades->basenxtamount;
    }
    else if ( jtrades->relid == assetid )
    {
        if ( (nxtamount= jtrades->relnxtamount) != 0 )
            return(nxtamount);
        ptr = &jtrades->relnxtamount;
    }
    else
    {
        printf("calc_nxtprice: illegal assetid.%llu\n",(long long)assetid);
        return(0);
    }
    nxtprice = calc_nxtmedianprice(assetid);
    nxtamount = (nxtprice * amount); // already adjusted for decimals
    printf("nxtprice %.8f nxtamount %.8f -> (*%p)\n",dstr(nxtprice),dstr(nxtamount),ptr);
    *ptr = nxtamount;
    return(nxtprice);
}

uint64_t calc_NXTprice(uint64_t *qtyp,struct _tradeleg *src,uint64_t srcqty,struct _tradeleg *dest,uint64_t destqty)
{
    printf("calc_NXTprice dest.%.8f src.%8f\n",dstr(destqty),dstr(srcqty));
    if ( src->assetid == NXT_ASSETID )
    {
        *qtyp = destqty;
        return(src->amount / destqty);
    }
    else if ( dest->assetid == NXT_ASSETID )
    {
        *qtyp = srcqty;
        return(dest->amount / srcqty);
    }
    return(0);
}

struct tradeleg *set_tradeleg(struct tradeleg *leg,struct _tradeleg *src,struct _tradeleg *dest) { leg->src = *src, leg->dest = *dest; return(leg); }

int32_t set_tradepair(int32_t numlegs,uint64_t *NXTpricep,struct jumptrades *jtrades,struct _tradeleg *src,uint64_t srcqty,struct _tradeleg *dest,uint64_t destqty)
{
    struct tradeleg *leg;
    uint64_t qty;
    *NXTpricep = calc_NXTprice(&qty,src,srcqty,dest,destqty);
    if ( qty == 0 )
    {
        printf("zero quantity, probably due to lack of decimals\n");
        return(0);
    }
    leg = set_tradeleg(&jtrades->legs[numlegs++],src,dest), leg->nxt64bits = src->nxt64bits, leg->qty = qty, leg->NXTprice = *NXTpricep;
    printf("set_tradeleg src.(%llu legqty %.8f price %.8f)\n",(long long)src->nxt64bits,dstr(leg->qty),dstr(leg->NXTprice));
    leg = set_tradeleg(&jtrades->legs[numlegs++],dest,src), leg->nxt64bits = dest->nxt64bits, leg->qty = qty, leg->NXTprice = *NXTpricep;
    printf("set_tradeleg dest.(%llu legqty %.8f price %.8f)\n",(long long)dest->nxt64bits,dstr(leg->qty),dstr(leg->NXTprice));
    return(numlegs);
}

int32_t outofband_price(uint64_t assetid,uint64_t NXTprice)
{
    uint64_t highbid,lowask;
    if ( assetid == NXT_ASSETID )
        return(0);
    highbid = get_nxthighbid(0,assetid);
    lowask = get_nxtlowask(0,assetid);
    printf("%llu highbid %.8f lowask %.8f | NXTprice %.8f\n",(long long)assetid,dstr(highbid),dstr(lowask),dstr(NXTprice));
    if ( lowask > highbid && highbid != 0 && NXTprice > highbid && NXTprice < lowask )
        return(0);
    return(-1);
}

int32_t jumptrade_ind(struct jumptrades *jtrades,uint64_t nxt64bits)
{
    if ( nxt64bits == jtrades->nxt64bits )
        return(0);
    else if ( nxt64bits == jtrades->other64bits )
        return(1);
    else if ( nxt64bits == jtrades->jump64bits )
        return(2);
    else printf("jumptrade_ind illegal %llu\n",(long long)nxt64bits);
    return(-1);
}

int32_t set_tradequad(int32_t numlegs,struct jumptrades *jtrades,struct _tradeleg *src,uint64_t srcqty,struct _tradeleg *dest,uint64_t destqty)
{
    int32_t srci,desti;
    struct tradeleg *leg;
    struct _tradeleg srcae,destae;
    uint64_t destNXTprice,srcNXTprice;
    srcae.nxt64bits = 0, srcae.assetid = NXT_ASSETID, srcae.amount = 0;
    destae = srcae;
    srci = jumptrade_ind(jtrades,src->nxt64bits);
    desti = jumptrade_ind(jtrades,dest->nxt64bits);
    srcNXTprice = calc_nxtprice(jtrades,src->assetid,src->amount);
    destNXTprice = calc_nxtprice(jtrades,dest->assetid,dest->amount);
    jtrades->balances[srci][desti] += (srcqty * srcNXTprice);
    jtrades->balances[desti][srci] -= (srcqty * srcNXTprice);
    jtrades->balances[srci][desti] -= (destqty * destNXTprice);
    jtrades->balances[desti][srci] += (destqty * destNXTprice);
    printf("set_tradequad %.8f src.(%llu amount %.8f qty %llu) dest.(%llu amount %.8f qty %llu) %.8f\n",dstr(srcNXTprice),(long long)src->assetid,dstr(src->amount),(long long)srcqty,(long long)dest->assetid,dstr(dest->amount),(long long)destqty,dstr(destNXTprice));
    sleep(5);
    if ( srcqty == 0 || destqty == 0 || outofband_price(src->assetid,srcNXTprice) != 0 || outofband_price(dest->assetid,destNXTprice) != 0 )
    {
        printf("Outofband error\n");
        return(0);
    }
    leg = set_tradeleg(&jtrades->legs[numlegs++],src,&srcae), leg->qty = srcqty, leg->NXTprice = srcNXTprice, leg->nxt64bits = src->nxt64bits;
    leg = set_tradeleg(&jtrades->legs[numlegs++],&srcae,dest), leg->qty = srcqty, leg->NXTprice = srcNXTprice, leg->nxt64bits = dest->nxt64bits;
    leg = set_tradeleg(&jtrades->legs[numlegs++],dest,&destae), leg->qty = destqty, leg->NXTprice = destNXTprice, leg->nxt64bits = dest->nxt64bits;
    leg = set_tradeleg(&jtrades->legs[numlegs++],&destae,src), leg->qty = destqty, leg->NXTprice = destNXTprice, leg->nxt64bits = src->nxt64bits;
    return(numlegs);
}

int32_t set_jtrade(int32_t numlegs,struct jumptrades *jtrades,struct _tradeleg *src,uint64_t srcqty,struct _tradeleg *dest,uint64_t destqty)
{
    printf(">>>>>>>>>>>>>>>>>>> set_jtrade\n");
    uint64_t NXTprice;
    if ( src->assetid == NXT_ASSETID )
    {
        numlegs = set_tradepair(numlegs,&NXTprice,jtrades,src,srcqty,dest,destqty);
        if ( outofband_price(dest->assetid,NXTprice) != 0 )
        {
            printf("outofband rejects trade\n");
            purge_jumptrades(jtrades);
            return(0);
        }
        return(numlegs);
    }
    else if ( dest->assetid == NXT_ASSETID )
    {
        numlegs = set_tradepair(numlegs,&NXTprice,jtrades,src,srcqty,dest,destqty);
        if ( outofband_price(src->assetid,NXTprice) != 0 )
        {
            printf("outofband rejects trade\n");
            purge_jumptrades(jtrades);
            return(0);
        }
        return(numlegs);
    } else return(set_tradequad(numlegs,jtrades,src,srcqty,dest,destqty));
}

uint64_t submit_triggered_bidask(char **retjsonstrp,char *bidask,uint64_t nxt64bits,char *NXTACCTSECRET,uint64_t assetid,uint64_t qty,uint64_t NXTprice,char *triggerhash,char *comment)
{
    int32_t deadline = 1 + time_to_nextblock(2)/60;
    uint64_t txid = 0;
    char cmd[4096],errstr[MAX_JSON_FIELD],*jsonstr;
    cJSON *json;
    if ( retjsonstrp != 0 )
        *retjsonstrp = 0;
    sprintf(cmd,"%s=%s&asset=%llu&secretPhrase=%s&feeNQT=%llu&quantityQNT=%llu&priceNQT=%llu&deadline=%d",_NXTSERVER,bidask,(long long)assetid,NXTACCTSECRET,(long long)MIN_NQTFEE,(long long)qty,(long long)NXTprice,deadline);
    if ( triggerhash != 0 && triggerhash[0] != 0 )
        sprintf(cmd+strlen(cmd),"&referencedTransactionFullHash=%s",triggerhash);
    if ( comment != 0 && comment[0] != 0 )
        sprintf(cmd+strlen(cmd),"&message=%s",comment);
    if ( (jsonstr= issue_NXTPOST(0,cmd)) != 0 )
    {
        stripwhite_ns(jsonstr,(int32_t)strlen(jsonstr));
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            copy_cJSON(errstr,cJSON_GetObjectItem(json,"error"));
            if ( errstr[0] == 0 )
                copy_cJSON(errstr,cJSON_GetObjectItem(json,"errorDescription"));
            if ( errstr[0] != 0 )
            {
                printf("submit_triggered_bidask.(%s) -> (%s)\n",cmd,jsonstr);
                if ( retjsonstrp != 0 )
                    *retjsonstrp = clonestr(errstr);
            }
            else txid = get_API_nxt64bits(cJSON_GetObjectItem(json,"transaction"));
        }
        free(jsonstr);
    }
    return(txid);
}

void set_jtrade_tx(struct jumptrades *jtrade,struct tradeleg *leg,char *NXTACCTSECRET,uint64_t nxt64bits,char *triggerhash)
{
    printf("legqty.%llu set_jtrade_tx.%llu (%s) (%llu %llu %.8f) (%llu %llu %.8f)\n",(long long)leg->qty,(long long)nxt64bits,triggerhash,(long long)leg->src.nxt64bits,(long long)leg->src.assetid,dstr(leg->src.amount),(long long)leg->dest.nxt64bits,(long long)leg->dest.assetid,dstr(leg->dest.amount));
    if ( leg->src.assetid == NXT_ASSETID || leg->dest.assetid == NXT_ASSETID )
    {
        if ( leg->src.nxt64bits == nxt64bits )
        {
            if ( leg->src.assetid == NXT_ASSETID )
                leg->txid = submit_triggered_bidask(0,"placeBidOrder",nxt64bits,NXTACCTSECRET,leg->dest.assetid,leg->qty,leg->NXTprice,triggerhash,jtrade->comment);
            else leg->txid = submit_triggered_bidask(0,"placeAskOrder",nxt64bits,NXTACCTSECRET,leg->src.assetid,leg->qty,leg->NXTprice,triggerhash,jtrade->comment);
            printf("txid normal %llu\n",(long long)leg->txid);
        }
        else if ( leg->src.nxt64bits == 0 )
        {
            if ( leg->dest.nxt64bits == nxt64bits )
                leg->txid = submit_triggered_bidask(0,"placeBidOrder",nxt64bits,NXTACCTSECRET,leg->dest.assetid,leg->qty,leg->NXTprice,triggerhash,jtrade->comment);
            else if ( leg->src.nxt64bits == nxt64bits )
                leg->txid = submit_triggered_bidask(0,"placeAskOrder",nxt64bits,NXTACCTSECRET,leg->src.assetid,leg->qty,leg->NXTprice,triggerhash,jtrade->comment);
            printf("txid nxtae %llu\n",(long long)leg->txid);
        } else printf("set_jtrade_tx: illegal src.%llu or dest.%llu jtrade_tx by %llu\n",(long long)leg->src.nxt64bits,(long long)leg->dest.nxt64bits,(long long)nxt64bits);
    } else printf("set_jtrade_tx: unsupported jtrade_tx\n");
}

int32_t reject_trade(uint64_t mynxt64bits,struct _tradeleg *src,struct _tradeleg *dest)
{
    if ( mynxt64bits == src->nxt64bits )
    {
        printf("check numconfirms!! verify asset.%llu amount.%llu -> NXT.%llu asset.%llu amount %llu is in orderbook\n",(long long)src->assetid,(long long)src->amount,(long long)dest->nxt64bits,(long long)dest->assetid,(long long)dest->amount);
        return(0);
    }
    else printf("mismatched nxtid %llu != %llu\n",(long long)mynxt64bits,(long long)src->nxt64bits);
    return(-1);
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
            priceNQT = (price * ap->mult);
            quantityQNT = (vol * SATOSHIDEN) / ap->mult;
            balance = get_asset_quantity(&unconfirmed,NXTaddr,assetidstr);
            printf("%s balance %.8f unconfirmed %.8f vs price %llu qty %llu for asset.%s | (%f * %f) * (%ld / %llu)\n",NXTaddr,dstr(balance),dstr(unconfirmed),(long long)priceNQT,(long long)quantityQNT,assetidstr,vol,price,SATOSHIDEN,(long long)ap->mult);
            if ( checkflag != 0 && (balance < quantityQNT || unconfirmed < quantityQNT) )
                return(0);
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

struct jumptrades *init_jtrades(uint64_t feeAtxid,char *triggerhash,uint64_t mynxt64bits,char *NXTACCTSECRET,uint64_t nxt64bits,uint64_t assetA,uint64_t amountA,uint64_t jump64bits,uint64_t jumpasset,uint64_t jumpamount,uint64_t other64bits,uint64_t assetB,uint64_t amountB,int64_t balancing,uint64_t balancetxid,char *gui,uint64_t quoteid)
{
    int32_t i,createdflag;
    struct NXT_tx T;
    struct NXT_asset *ap;
    int64_t unconfirmed;
    uint64_t balance;
    char jumpstr[MAX_JSON_FIELD],comment[MAX_JSON_FIELD],assetidstr[64],NXTaddr[64],balancestr[512];
    struct _tradeleg src,dest,jump,myjump;
    struct jumptrades *jtrades = get_jumptrade();
    if ( jtrades == 0 )
        return(0);
    src.nxt64bits = nxt64bits, src.assetid = assetA, src.amount = amountA;
    myjump.nxt64bits = nxt64bits, jump.assetid = jumpasset, jump.amount = jumpamount;
    jump.nxt64bits = jump64bits, jump.assetid = jumpasset, jump.amount = jumpamount;
    dest.nxt64bits = other64bits, dest.assetid = assetB, dest.amount = amountB;
    memset(jtrades,0,sizeof(*jtrades));
    printf("gui.(%s) init_jtrades(%llu %.8f -> %llu %.8f)\n",gui,(long long)assetA,dstr(amountA),(long long)assetB,dstr(amountB));
    jtrades->qtyA = amountA, jtrades->qtyB = amountB, jtrades->jumpqty = jumpamount;
    jtrades->baseid = assetA, jtrades->relid = assetB;
    if ( assetA != NXT_ASSETID )
    {
        expand_nxt64bits(assetidstr,assetA);
        ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
        if ( ap->mult != 0 )
        {
            jtrades->qtyA /= ap->mult;
            expand_nxt64bits(NXTaddr,nxt64bits);
            balance = ap->mult * get_asset_quantity(&unconfirmed,NXTaddr,assetidstr);
            if ( balance < amountA )
            {
                printf("balance %.8f < amountA %.8f for asset.%s\n",dstr(balance),dstr(amountA),assetidstr);
                return(0);
            }
        }
        else printf("%llu null apmult\n",(long long)assetA);
    }
    if ( assetB != NXT_ASSETID )
    {
        expand_nxt64bits(assetidstr,assetB);
        ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
        if ( ap->mult != 0 )
        {
            jtrades->qtyB /= ap->mult;
            expand_nxt64bits(NXTaddr,other64bits);
            balance = ap->mult * get_asset_quantity(&unconfirmed,NXTaddr,assetidstr);
            if ( balance < amountB )
            {
                printf("balance %.8f < amountB %.8f for asset.%s\n",dstr(balance),dstr(amountB),assetidstr);
                return(0);
            }
        }
        else printf("%llu null apmult\n",(long long)assetB);
    }
    if ( jumpasset != 0 && jumpasset != NXT_ASSETID )
    {
        expand_nxt64bits(assetidstr,jumpasset);
        ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
        if ( ap->mult != 0 )
        {
            jtrades->jumpqty /= ap->mult;
            expand_nxt64bits(NXTaddr,jump64bits);
            balance = ap->mult * get_asset_quantity(&unconfirmed,NXTaddr,assetidstr);
            if ( balance < jumpamount )
            {
                printf("balance %.8f < jumpamount %.8f for asset.%s\n",dstr(balance),dstr(jumpamount),assetidstr);
                return(0);
            }
        }
        else printf("%llu null apmult\n",(long long)jumpasset);
    }
    jtrades->nxt64bits = nxt64bits, jtrades->other64bits = other64bits, jtrades->jump64bits = jump64bits;
    jtrades->fee = jtrades->otherfee = INSTANTDEX_FEE;
    printf("(%llu %llu %llu) assetA.%llu assetB.%llu jumpasset.%llu\n",(long long)nxt64bits,(long long)other64bits,(long long)jump64bits,(long long)assetA,(long long)assetB,(long long)jumpasset);
    if ( jumpasset != 0 )
    {
        jtrades->fee <<= 1;
        if ( mynxt64bits == jump64bits && reject_trade(mynxt64bits,&jump,&src) != 0 )
        {
            printf("jumper rejects jumptrade\n");
            purge_jumptrades(jtrades);
            return(0);
        }
        else if ( mynxt64bits == other64bits && reject_trade(mynxt64bits,&dest,&myjump) != 0 )
        {
            printf("other64bits rejects jumptrade\n");
            purge_jumptrades(jtrades);
            return(0);
        }
        jtrades->numlegs = set_jtrade(jtrades->numlegs,jtrades,&src,jtrades->qtyA,&jump,jtrades->jumpqty);
        jtrades->numlegs = set_jtrade(jtrades->numlegs,jtrades,&myjump,jtrades->jumpqty,&dest,jtrades->qtyB);
        sprintf(jumpstr,"\"jumper\":\"%llu\",\"jumpasset\":\"%llu\",\"jumpamount\":\"%llu\",",(long long)jump64bits,(long long)jumpasset,(long long)jumpamount);
    }
    else
    {
        if ( mynxt64bits == other64bits && reject_trade(mynxt64bits,&dest,&src) != 0 )
        {
            printf("other64bits rejects trade\n");
            purge_jumptrades(jtrades);
            return(0);
        }
        printf("call set_jtrade: src.%llu %.8f -> dest.%llu %.8f\n",(long long)src.assetid,dstr(jtrades->qtyA),(long long)dest.assetid,dstr(jtrades->qtyB));
        jtrades->numlegs = set_jtrade(jtrades->numlegs,jtrades,&src,jtrades->qtyA,&dest,jtrades->qtyB);
        jumpstr[0] = 0;
    }
    if ( jtrades->numlegs == 0 )
    {
        printf("no legs rejects trade\n");
        purge_jumptrades(jtrades);
        return(0);
    }
    jtrades->balances[0][0] = (jtrades->balances[0][1] + jtrades->balances[0][2]);
    jtrades->balances[1][1] = (jtrades->balances[1][0] + jtrades->balances[1][2]);
    jtrades->balances[2][2] = (jtrades->balances[2][0] + jtrades->balances[2][1]);
    if ( (jtrades->balances[0][0]+jtrades->balances[1][1]+jtrades->balances[2][2]) != 0 || jtrades->balances[2][2] != 0 || jtrades->balances[0][0] != -jtrades->balances[1][1] )
    {
        printf("balances[][] error %lld %lld %lld = %lld\n",(long long)jtrades->balances[0][0],(long long)jtrades->balances[1][1],(long long)jtrades->balances[2][2],(long long)(jtrades->balances[0][0]+jtrades->balances[1][1]+jtrades->balances[2][2]));
        return(0);
    }
    if ( balancing != 0 && jtrades->balances[0][0] != balancing )
    {
        printf("balancing mismatch: %lld vs %lld\n",(long long)jtrades->balances[0][0],(long long)balancing);
        return(0);
    }
    sprintf(comment,"{\"requestType\":\"processjumptrade\",\"NXT\":\"%llu\",\"assetA\":\"%llu\",\"amountA\":\"%llu\",%s\"other\":\"%llu\",\"assetB\":\"%llu\",\"amountB\":\"%llu\",\"feeA\":\"%llu\",\"balancing\":%lld,\"gui\":\"%s\",\"quoteid\":\"%llu\"}",(long long)nxt64bits,(long long)assetA,(long long)amountA,jumpstr,(long long)other64bits,(long long)assetB,(long long)amountB,(long long)jtrades->fee,(long long)jtrades->balances[0][0],gui,(long long)quoteid);
    printf("comment.(%s)\n",comment);
    if ( triggerhash == 0 || triggerhash[0] == 0 )
    {
        if ( mynxt64bits == nxt64bits )
        {
            printf("create fee tx %llu vs %llu\n",(long long)calc_nxt64bits(Global_mp->myNXTADDR),(long long)nxt64bits);
            set_NXTtx(nxt64bits,&T,NXT_ASSETID,jtrades->fee,calc_nxt64bits(INSTANTDEX_ACCT),-1);
            strcpy(T.comment,comment);
            jtrades->feetx = sign_NXT_tx(jtrades->feeutxbytes,jtrades->feesignedtx,NXTACCTSECRET,nxt64bits,&T,0,1.);
            get_txhashes(jtrades->feesighash,jtrades->triggerhash,jtrades->feetx);
            triggerhash = jtrades->triggerhash;
            jtrades->feetxid = jtrades->feetx->txid;
        } else printf("%llu is not me %llu\n",(long long)mynxt64bits,(long long)nxt64bits);
    }
    else
    {
        printf("triggerhash.(%s)\n",triggerhash);
        jtrades->feetxid = feeAtxid;
        strcpy(jtrades->triggerhash,triggerhash);
        if ( other64bits == mynxt64bits )
            jtrades->otherfeetxid = send_feetx(NXT_ASSETID,jtrades->otherfee,triggerhash,comment);
    }
    if ( jtrades->balances[0][0] != 0 )
    {
        char balancetxbytes[4096]; int32_t errcode = -1;
        if ( jtrades->balances[0][0] > 0 && mynxt64bits == nxt64bits )
            set_NXTtx(mynxt64bits,&T,NXT_ASSETID,jtrades->balances[0][0],jtrades->other64bits,-1), errcode = 0;
        else if ( jtrades->balances[0][0] < 0 && mynxt64bits == jtrades->other64bits )
            set_NXTtx(mynxt64bits,&T,NXT_ASSETID,-jtrades->balances[0][0],nxt64bits,-1), errcode = 0;
        if ( errcode == 0 )
        {
            strcpy(T.comment,comment);
            jtrades->balancetx = sign_NXT_tx(0,balancetxbytes,NXTACCTSECRET,nxt64bits,&T,jtrades->triggerhash,1.);
            jtrades->balancetxid = issue_broadcastTransaction(&errcode,0,balancetxbytes,Global_mp->srvNXTACCTSECRET);
        }
    }
    strcpy(jtrades->comment,comment);
    if ( jtrades->balancetxid != 0 )
        sprintf(balancestr,",\"balancetxid\":\"%llu\"",(long long)jtrades->balancetxid);
    else balancestr[0] = 0;
    sprintf(jtrades->comment + strlen(jtrades->comment) - 1,",\"feeAtxid\":\"%llu\",\"triggerhash\":\"%s\"%s}",(long long)jtrades->feetxid,jtrades->triggerhash,jtrades->balancetxid == 0 ? "" : balancestr);
    if ( strlen(jtrades->triggerhash) == 64 )
    {
        for (i=0; i<jtrades->numlegs; i++)
            set_jtrade_tx(jtrades,&jtrades->legs[i],NXTACCTSECRET,mynxt64bits,jtrades->triggerhash);
        jtrades->state = 2;
        return(jtrades);
    } else printf("invalid triggerhash\n");
    printf("init_jtrades.(%s) invalid triggerhash\n",jtrades->triggerhash);
    purge_jumptrades(jtrades);
    return(0);
}

int32_t tweak_orders(struct NXT_tx *txptrs[],int32_t numtx,uint64_t sellid,uint64_t avail,uint64_t *sellqtyp,uint64_t *sellpriceNQTp,uint64_t buyid,uint64_t *buyqtyp,uint64_t *buypriceNQTp)
{
    uint64_t lowask,highbid,highbidvol,lowaskvol;
    int32_t retval = 0;
    lowask = get_nxthighbid(&lowaskvol,buyid);
    highbid = get_nxtlowask(&highbidvol,sellid);
    if ( lowask > *buypriceNQTp )
    {
        if ( lowask > *buypriceNQTp*1.001 )
        {
            printf("lowask above buyprice\n");
            retval |= 1;
        } else *buypriceNQTp = lowask;
    }
    if ( highbid < *sellpriceNQTp )
    {
        if ( highbid < *sellpriceNQTp/1.001 )
        {
            printf("highbid below sellprice\n");
            retval |= 2;
        } else *sellpriceNQTp = highbid;
    }
    if ( *sellqtyp < highbidvol )
    {
        if ( avail > highbidvol )
            *sellqtyp = highbidvol, retval |= 4;
        else *sellqtyp = avail, retval |= 8;
    }
    printf("retval.%d buyqty %llu: %llu vs %llu lavol %.8f | avail %llu sellqty %llu: %llu vs %llu hbvol %.8f\n",retval,(long long)*buyqtyp,(long long)*buypriceNQTp,(long long)lowask,dstr(lowaskvol),(long long)avail,(long long)*sellqtyp,(long long)*sellpriceNQTp,(long long)highbid,dstr(highbidvol));
    return(retval);
}

char *makeoffer2(char *NXTaddr,char *NXTACCTSECRET,uint64_t assetA,uint64_t amountA,char *jumpNXTaddr,uint64_t jumpasset,uint64_t jumpamount,char *otherNXTaddr,uint64_t assetB,uint64_t amountB,char *gui,uint64_t quoteid)
{
    char buf[4096],*str;
    struct jumptrades *jtrades;
    uint64_t nxt64bits,other64bits,jump64bits = 0;
    nxt64bits = calc_nxt64bits(NXTaddr);
    other64bits = calc_nxt64bits(otherNXTaddr);
    printf("makeoffer2\n");
    if ( jumpasset != 0 )
    {
        if ( jumpasset == assetA || jumpasset == assetB || strcmp(NXTaddr,jumpNXTaddr) == 0 || strcmp(jumpNXTaddr,otherNXTaddr) == 0 )
        {
            printf("jump collision %llu %llu %llu\n",(long long)assetA,(long long)jumpasset,(long long)assetB);
            return(0);
        }
        jump64bits = calc_nxt64bits(jumpNXTaddr);
    }
    if ( amountA == 0 || amountB == 0 || assetA == assetB || (jumpasset != 0 && jumpNXTaddr[0] == 0) || strcmp(NXTaddr,otherNXTaddr) == 0 )
    {
        printf("{\"error\":\"%s\",\"descr\":\"NXT.%llu makeoffer to NXT.%s %.8f asset.%llu for %.8f asset.%llu jump.%llu (%s)\"}\n","illegal parameter",(long long)nxt64bits,otherNXTaddr,dstr(amountA),(long long)assetA,dstr(amountB),(long long)assetB,(long long)jumpasset,jumpNXTaddr);
        return(0);
    }
    if ( (jtrades= init_jtrades(0,"",calc_nxt64bits(NXTaddr),NXTACCTSECRET,nxt64bits,assetA,amountA,jump64bits,jumpasset,jumpamount,other64bits,assetB,amountB,0,0,gui,quoteid)) != 0 )
    {
        strcpy(buf,jtrades->comment);
        if ( (str= submit_atomic_txfrag("processjumptrade",jtrades->comment,NXTaddr,NXTACCTSECRET,otherNXTaddr)) != 0 )
            free(str);
        if ( jumpNXTaddr != 0 && jumpNXTaddr[0] != 0 && (str= submit_atomic_txfrag("processjumptrade",jtrades->comment,NXTaddr,NXTACCTSECRET,jumpNXTaddr)) != 0 )
            free(str);
        jtrades->endmilli = milliseconds() + 2. * JUMPTRADE_SECONDS * 1000;
        printf("MAKEOFFER2.(%s)\n",buf);
        return(clonestr(buf));
    } //else strcpy(buf,"{\"error\":\"couldnt initialize jtrades, probably too many pending\"}");
    return(0);
}

int32_t validated_jumptrades(struct jumptrades *jtrades)
{
    char cmd[MAX_JSON_FIELD],txidstr[MAX_JSON_FIELD],reftx[MAX_JSON_FIELD],*jsonstr;
    int32_t i,j,type,subtype,iter,n,balanced,numvalid = 0;
    uint32_t timestamp,deadline,expiration;
    uint64_t amount,txid,price,qty,senderbits,assetbits,accts[4];
    cJSON *json,*array,*txobj,*attachobj;
    struct tradeleg *leg;
    accts[0] = calc_nxt64bits(INSTANTDEX_ACCT);
    accts[1] = jtrades->nxt64bits;
    accts[2] = jtrades->other64bits;
    accts[3] = jtrades->jump64bits;
    //getUnconfirmedTransactions ({"unconfirmedTransactions":[{"fullHash":"be0b5973872cd77117e21de0c52096ad0619d84ef66816429ac0a48eb2db387a","referencedTransactionFullHash":"e4981cbf0756bb94019039a93f102d439482b6323e047bb87e2f87a80869b37e","signatureHash":"ace6dc5e2fbcb406c2cca55277291b785dd157b9de8ad16bdc17fb5172ecd476","transaction":"8203074206546070462","amountNQT":"250000000","ecBlockHeight":366610,"recipientRS":"NXT-74VC-NKPE-RYCA-5LMPT","type":0,"feeNQT":"100000000","recipient":"4383817337783094122","version":1,"sender":"12240549928875772593","timestamp":39545749,"ecBlockId":"6211277282542147167","height":2147483647,"subtype":0,"senderPublicKey":"ec7f665fccae39025531b1cb3c48e584916dba00a7034edc60f9e4111f86145d","deadline":10,"senderRS":"NXT-7LPK-BUH3-6SCV-CDTRM","signature":"02ade454abf8ea6157bb23fba59cb06479864fa9f1ceffc0a7957e9a44ada2035d94371d8b0db029d75e32a69c33a8a064885858fe3d4ed4d697ac9f30f261fd"}],"requestProcessingTime":1})
    balanced = (jtrades->balances[0][0] == 0);
    for (iter=0; iter<4&&accts[iter]!=0; iter++)
    {
        sprintf(cmd,"requestType=getUnconfirmedTransactions&account=%llu",(long long)accts[iter]);
        if ( (jsonstr= issue_NXTPOST(0,cmd)) != 0 )
        {
            //printf("iter.%d %llu getUnconfirmedTransactions (%s)\n",iter,(long long)accts[iter],jsonstr);
            if ( (json= cJSON_Parse(jsonstr)) != 0 )
            {
                if ( (array= cJSON_GetObjectItem(json,"unconfirmedTransactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
                {
                    for (i=0; i<n; i++)
                    {
                        txobj = cJSON_GetArrayItem(array,i);
                        copy_cJSON(reftx,cJSON_GetObjectItem(txobj,"referencedTransactionFullHash"));
                        copy_cJSON(txidstr,cJSON_GetObjectItem(txobj,"transaction"));
                        txid = calc_nxt64bits(txidstr);
                        if ( strcmp(reftx,jtrades->triggerhash) == 0 )
                        {
                            senderbits = get_API_nxt64bits(cJSON_GetObjectItem(txobj,"sender"));
                            timestamp = (uint32_t)get_API_int(cJSON_GetObjectItem(txobj,"timestamp"),0);
                            deadline = (uint32_t)get_API_int(cJSON_GetObjectItem(txobj,"deadline"),0);
                            expiration = (timestamp + deadline*60);
                            type = (int32_t)get_API_int(cJSON_GetObjectItem(txobj,"type"),-1);
                            subtype = (int32_t)get_API_int(cJSON_GetObjectItem(txobj,"subtype"),-1);
                            amount = get_API_nxt64bits(cJSON_GetObjectItem(txobj,"amountNQT"));
                            if ( jtrades->balancetxid != 0 && txid == jtrades->balancetxid && type == 0 && subtype == 0 )
                            {
                                if ( jtrades->balances[0][0] > 0 && senderbits == jtrades->nxt64bits && accts[iter] == jtrades->other64bits && amount == jtrades->balances[0][0] )
                                    balanced++;
                                else if ( jtrades->balances[0][0] < 0 && senderbits == jtrades->other64bits && accts[iter] == jtrades->nxt64bits && amount == -jtrades->balances[0][0] )
                                    balanced++;
                                printf("balancing.%lld sender.%llu acct.%llu amount %.8f balanced.%d\n",(long long)jtrades->balances[0][0],(long long)senderbits,(long long)accts[iter],dstr(amount),balanced);
                            }
                            else if ( iter == 0 && senderbits == jtrades->other64bits )
                            {
                                printf("found unconfirmed feetxid from %llu for %.8f vs fee %.8f\n",(long long)senderbits,dstr(amount),dstr(jtrades->otherfee));
                                if ( type == 0 && subtype == 0 && amount >= jtrades->otherfee )
                                {
                                    jtrades->otherfeetxid = txid;
                                    jtrades->otherexpiration = expiration;
                                    numvalid++;
                                }
                            }
                            else if ( type == 2 && subtype >= 2 && (attachobj= cJSON_GetObjectItem(txobj,"attachment")) != 0 ) // AE bid or ask
                            {
                                /* "attachment": {
                                "version.AskOrderPlacement": 1,
                                "quantityQNT": "50000",
                                "priceNQT": "2347900",
                                "asset": "12071612744977229797"
                                 },*/
                                assetbits = get_API_nxt64bits(cJSON_GetObjectItem(attachobj,"asset"));
                                qty = get_API_nxt64bits(cJSON_GetObjectItem(attachobj,"quantityQNT"));
                                price = get_API_nxt64bits(cJSON_GetObjectItem(attachobj,"priceNQT"));
                                printf("numlegs.%d sender.%llu iter.%llu subtype.%d: asset.%llu qty.%llu %.8f\n",jtrades->numlegs,(long long)senderbits,(long long)accts[iter],subtype,(long long)assetbits,(long long)qty,dstr(price));
                                for (j=0; j<jtrades->numlegs; j++)
                                {
                                    leg = &jtrades->legs[j];
                                    printf("j.%d legnxt.%llu sender.%llu\n",j,(long long)leg->nxt64bits,(long long)senderbits);
                                    if ( leg->nxt64bits == senderbits )
                                    {
                                        printf("leg qty.%llu %.8f vs %llu %.8f\n",(long long)leg->qty,dstr(leg->NXTprice),(long long)qty,dstr(price));
                                        if ( leg->qty == qty && leg->NXTprice == price )
                                        {
                                            printf("subtype.%d: asset.%llu vs src.%llu dest.%llu\n",subtype,(long long)assetbits,(long long)leg->src.assetid,(long long)leg->dest.assetid);
                                            if ( (subtype == 2 && leg->src.assetid == assetbits) || (subtype == 3 && leg->dest.assetid == assetbits) )
                                            {
                                                printf("numvalid.%d txid.%llu assetbits.%llu qty %llu price %.8f\n",numvalid,(long long)txid,(long long)assetbits,(long long)qty,dstr(price));
                                                leg->txid = txid;
                                                leg->expiration = expiration;
                                                numvalid++;
                                                break;
                                            }
                                            else printf("subtype.%d src.%llu dest.%llu: numvalid.%d txid.%llu assetbits.%llu qty %llu price %.8f\n",subtype,(long long)leg->src.assetid,(long long)leg->dest.assetid,numvalid,(long long)txid,(long long)assetbits,(long long)qty,dstr(price));
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                free_json(json);
            }
            free(jsonstr);
        }
    }
    return(numvalid * balanced);
}

void oldpoll_jumptrades(char *NXTACCTSECRET)
{
    float millis = milliseconds();
    uint64_t feetxid;
    int32_t i,errcode;
    struct jumptrades *jtrades;
    for (i=0; i<(int32_t)(sizeof(JTRADES)/sizeof(*JTRADES)); i++)
    {
        jtrades = &JTRADES[i];
        if ( jtrades->state >= 2 )
        {
            if ( millis < jtrades->endmilli )
            {
                if ( validated_jumptrades(jtrades) == (jtrades->numlegs + 1) )
                {
                    feetxid = issue_broadcastTransaction(&errcode,0,jtrades->feesignedtx,NXTACCTSECRET);
                    printf("Jump trades triggered! feetxid.%llu\n",(long long)feetxid);
                    jtrades->state = -2;
                }
            }
            else
            {
                if ( jtrades->iQ != 0 )
                {
                    printf(">>>>>>>>>>>>>>> reset iQ->matched\n");
                    jtrades->iQ->sent = jtrades->iQ->matched = 0;
                }
                jtrades->state = -1;
            }
        }
    }
}

char *processjumptrade_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    struct InstantDEX_quote *search_pendingtrades(uint64_t my64bits,uint64_t baseid,uint64_t baseamount,uint64_t relid,uint64_t relamount);
    char triggerhash[MAX_JSON_FIELD],buf[MAX_JSON_FIELD],gui[MAX_JSON_FIELD];
    struct jumptrades *jtrades;
    struct InstantDEX_quote *iQ = 0;
    int64_t balancing;
    uint64_t quoteid,assetA,amountA,other64bits,assetB,amountB,feeA,feeAtxid,jump64bits,jumpasset,jumpamount,senderbits,mybits,balancetxid;
    if ( is_remote_access(previpaddr) == 0 )
        return(0);
    assetA = get_API_nxt64bits(objs[0]);
    amountA = get_API_nxt64bits(objs[1]);
    other64bits = get_API_nxt64bits(objs[2]);
    assetB = get_API_nxt64bits(objs[3]);
    amountB = get_API_nxt64bits(objs[4]);
    feeA = get_API_nxt64bits(objs[5]);
    feeAtxid = get_API_nxt64bits(objs[6]);
    copy_cJSON(triggerhash,objs[7]);
    jump64bits = get_API_nxt64bits(objs[8]);
    jumpasset = get_API_nxt64bits(objs[9]);
    jumpamount = get_API_nxt64bits(objs[10]);
    balancing = get_API_nxt64bits(objs[11]);
    balancetxid = get_API_nxt64bits(objs[12]);
    copy_cJSON(gui,objs[13]), gui[4] = 0;
    quoteid = get_API_nxt64bits(objs[14]);
    senderbits = calc_nxt64bits(sender);
    mybits = calc_nxt64bits(NXTaddr);
    printf("processjumptrade (%llu %.8f -> %llu %.8f)\n",(long long)assetA,dstr(amountA),(long long)assetB,dstr(amountB));
    if ( mybits == other64bits )
        iQ = search_pendingtrades(other64bits,assetB,amountB,assetA,amountA);
    else if ( mybits == jump64bits )
    {
        if ( (iQ= search_pendingtrades(jump64bits,jumpasset,jumpamount,assetA,amountA)) == 0 )
            iQ = search_pendingtrades(jump64bits,jumpasset,jumpamount,assetB,amountB);
    }
    if ( iQ != 0 )
    {
        if ( (jtrades= init_jtrades(feeAtxid,triggerhash,calc_nxt64bits(NXTaddr),NXTACCTSECRET,senderbits,assetA,amountA,jump64bits,jumpasset,jumpamount,other64bits,assetB,amountB,balancing,balancetxid,gui,quoteid)) != 0 )
        {
            iQ->matched = 1;
            jtrades->iQ = iQ;
            jtrades->endmilli = milliseconds() + JUMPTRADE_SECONDS * 1000;
            strcpy(buf,jtrades->comment);
        } else strcpy(buf,"{\"error\":\"couldnt initialize jtrades, probably too many pending\"}");
    } else strcpy(buf,"{\"error\":\"couldnt find matching trade\"}");
    printf("PROCESSJUMPTRADE.(%s)\n",buf);
    return(clonestr(buf));
}

char *jumptrades_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char *retstr;
    cJSON *json;
    json = jumptrades_json();
    retstr = cJSON_Print(json);
    free_json(json);
    return(retstr);
}

char *oldmakeoffer3(char *NXTaddr,char *NXTACCTSECRET,int32_t flip,uint64_t srcqty,uint64_t baseid,uint64_t relid,cJSON *baseobj,cJSON *relobj,uint64_t quoteid,int32_t askoffer)
{
    int32_t update_iQ_flags(struct NXT_tx *txptrs[],int32_t maxtx,uint64_t baseid,uint64_t relid);
    char comment[MAX_JSON_FIELD],assetidstr[64],feeutxbytes[2048],feesignedtx[2048],triggerhash[65],feesighash[65],buf[MAX_JSON_FIELD],*jsonstr = 0;
    uint64_t nxt64bits,qtyA,qtyB,priceNQTA,priceNQTB,feetxid,availA,availB,asktxid,bidtxid,tmp,srcarg,srcamount=0,fee = INSTANTDEX_FEE;
    uint64_t frombase,fromrel,tobase,torel;
    struct NXT_tx T,*txptrs[MAX_TXPTRS],*feetx;
    int32_t i,errcode,numtx,matched,createdflag;
    struct NXT_asset *ap;
    double endmilli,priceA,priceB,volA=0.,volB=0.,ratio = 1.;
    memset(txptrs,0,sizeof(txptrs));
    nxt64bits = calc_nxt64bits(NXTaddr);
    frombase = get_API_nxt64bits(cJSON_GetObjectItem(baseobj,"baseamount")), fromrel = get_API_nxt64bits(cJSON_GetObjectItem(baseobj,"relamount"));
    tobase = get_API_nxt64bits(cJSON_GetObjectItem(relobj,"baseamount")), torel = get_API_nxt64bits(cJSON_GetObjectItem(relobj,"relamount"));
    if ( flip != 0 )
        askoffer = 1;
    if ( askoffer == 0 )
        srcarg = frombase, expand_nxt64bits(assetidstr,baseid);
    else srcarg = tobase, expand_nxt64bits(assetidstr,relid);
    ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
    printf("askoffer.%d flip.%d srcarg %llu  srcmult.%llu\n",askoffer,flip,(long long)srcarg,(long long)ap->mult);
    if ( ap->mult != 0 && srcarg != 0 )
    {
        tmp = (srcarg / ap->mult);
        if ( srcqty != 0 && srcqty < tmp )
            srcamount = ap->mult * srcqty, printf("srcamount path0: %.8f tmp %.8f\n",dstr(srcamount),dstr(tmp));
        else srcamount = (tmp * ap->mult), printf("srcamount path1: %.8f tmp %.8f\n",dstr(srcamount),dstr(tmp));
    } else return(clonestr("{\"error\":\"asset without mult or trading below decimals resolution\"}"));
    if ( srcamount != 0 )
    {
        if ( srcamount < srcarg )
        {
            ratio = ((double)srcamount / srcarg);
            priceA = calc_price_volume(&volA,askoffer == 0 ? srcamount : ratio * frombase,ratio * fromrel);
            priceB = calc_price_volume(&volB,askoffer != 0 ? srcamount : ratio * tobase,ratio * torel);
        }
        else
        {
            priceA = calc_price_volume(&volA,frombase,fromrel);
            priceB = calc_price_volume(&volB,tobase,torel);
        }
        qtyA = calc_asset_qty(&availA,&priceNQTA,NXTaddr,askoffer==0,baseid,priceA,volA);
        qtyB = calc_asset_qty(&availB,&priceNQTB,NXTaddr,askoffer!=0,relid,priceB,volB);
        printf("ratio %f, srcamount %.8f srcarg %.8f srcqty %.8f -> (%f %f) (%f %f)\n",ratio,dstr(srcamount),dstr(srcarg),dstr(srcqty),priceA,volA,priceB,volB);
    } else priceB = priceA = qtyA = qtyB = priceNQTA = priceNQTB = availA = availB = volA = volB = 0;
    printf("qtyA %llu priceA %f volA %f, qtyB %lld priceB %f volB %f\n",(long long)qtyA,priceA,volA,(long long)qtyB,priceB,volB);
    if ( srcamount == 0 || qtyA == 0 || qtyB == 0 || priceNQTA == 0 || priceNQTB == 0 )
    {
        sprintf(buf,"{\"error\":\"%s\",\"descr\":\"NXT.%llu makeoffer3 srcamount.%.8f qtyA %.8f assetA.%llu price %.8f for qtyB %.8f assetB.%llu price.%.8f\"}\n","illegal parameter",(long long)nxt64bits,dstr(srcamount),dstr(qtyA),(long long)baseid,dstr(priceNQTA),dstr(qtyB),(long long)relid,dstr(priceNQTB));
        return(clonestr(buf));
    }
    feetxid = bidtxid = asktxid = 0;
    sprintf(comment,"{\"requestType\":\"makeoffer3\",\"NXT\":\"%llu\",\"ratio\":\"%.8f\",\"srcqty\":\"%llu\",\"baseid\":\"%llu\",\"relid\":\"%llu\",\"frombase\":\"%llu\",\"fromrel\":\"%llu\",\"tobase\":\"%llu\",\"torel\":%llu,\"qtyA\":\"%llu\",\"priceNQTA\":\"%llu\",\"qtyB\":\"%llu\",\"priceNQTB\":\"%llu\",\"fee\":\"%llu\",\"quoteid\":\"%llu\"}",(long long)nxt64bits,ratio,(long long)srcqty,(long long)baseid,(long long)relid,(long long)frombase,(long long)fromrel,(long long)tobase,(long long)torel,(long long)qtyA,(long long)priceNQTA,(long long)qtyB,(long long)priceNQTB,(long long)fee,(long long)quoteid);
    printf("(%s) create fee tx %llu vs %llu\n",comment,(long long)calc_nxt64bits(Global_mp->myNXTADDR),(long long)nxt64bits);
    set_NXTtx(nxt64bits,&T,NXT_ASSETID,fee,calc_nxt64bits(INSTANTDEX_ACCT),-1);
    strcpy(T.comment,comment);
    if ( (feetx= sign_NXT_tx(feeutxbytes,feesignedtx,NXTACCTSECRET,nxt64bits,&T,0,1.)) != 0 )
    {
        get_txhashes(feesighash,triggerhash,feetx);
        sprintf(comment + strlen(comment) - 1,",\"feetxid\":\"%llu\",\"triggerhash\":\"%s\"}",(long long)feetx->txid,triggerhash);
        strcpy(buf,comment);
        if ( strlen(triggerhash) == 64 )
        {
            if ( (asktxid= submit_triggered_bidask(&jsonstr,(askoffer == 0) ? "placeAskOrder" : "placeBidOrder",nxt64bits,NXTACCTSECRET,baseid,qtyA,priceNQTA,triggerhash,comment)) == 0 )
                sprintf(buf+strlen(buf)-1,",\"error\":[%s]}",jsonstr!=0?jsonstr:"first order failed");
            else if ( (bidtxid= submit_triggered_bidask(&jsonstr,(askoffer == 0) ? "placeBidOrder" : "placeAskOrder",nxt64bits,NXTACCTSECRET,relid,qtyB,priceNQTB,triggerhash,comment)) == 0 )
                sprintf(buf+strlen(buf)-1,",\"asktxid\":\"%llu\",\"placeorder\":\"%llu\",\"error\":[%s]}",(long long)asktxid,(long long)relid,jsonstr!=0?jsonstr:"second order failed");
            else
            {
                endmilli = milliseconds() + 2. * JUMPTRADE_SECONDS * 1000;
                matched = 0;
                while ( milliseconds() < endmilli )
                {
                    if ( (numtx= update_iQ_flags(txptrs,sizeof(txptrs)/sizeof(*txptrs),baseid,relid)) > 0 )
                    {
                        for (i=matched=0; i<numtx; i++)
                        {
                            if ( txptrs[i]->txid == asktxid )
                                matched++;
                            else if ( txptrs[i]->txid == bidtxid )
                                matched++;
                            free(txptrs[i]), txptrs[i] = 0;
                        }
                        if ( matched == 2 )
                        {
                            if ( (feetxid= issue_broadcastTransaction(&errcode,0,feesignedtx,NXTACCTSECRET)) != feetx->txid )
                            {
                                printf("Jump trades triggered! feetxid.%llu but unexpected should have been %llu\n",(long long)feetxid,(long long)feetx->txid);
                                sprintf(buf+strlen(buf)-1,",\"asktxid\":\"%llu\",\"bidtxid\":\"%llu\",\"actualfeetxid\":\"%llu\",\"error\":[%s]}",(long long)asktxid,(long long)bidtxid,(long long)feetxid,"unexpected feetxid");
                            } else sprintf(buf+strlen(buf)-1,",\"asktxid\":\"%llu\",\"bidtxid\":\"%llu\"}",(long long)asktxid,(long long)bidtxid);
                            break;
                        }
                    }
                    sleep(1);
                }
                if ( matched < 2 )
                    printf("only found %d matched, aborting\n",matched);
            }
        } else printf("invalid triggerhash.(%s).%ld\n",triggerhash,strlen(triggerhash));
        free(feetx);
        if ( jsonstr != 0 )
            free(jsonstr);
        return(clonestr(buf));
    }
    return(clonestr("{\"error\":\"couldnt submit fee tx\"}"));
}

struct pendinghalf
{
    char exchange[16];
    uint64_t offerNXT,assetid,baseamount,relamount,quoteid,avail,priceNQT,qty;
    double price,vol;
    int32_t exchangeid;
};

struct pending_trade
{
    char comment[MAX_JSON_FIELD],feeutxbytes[MAX_JSON_FIELD],feesignedtx[MAX_JSON_FIELD],triggerhash[65],feesighash[65];
    uint64_t txids[16],actual_feetxid,fee,nxt64bits,baseid,relid,srcqty,quoteid,srcarg,srcamount;
    double ratio,expiration,price,volume;
    int32_t numrequired,errcode,askoffer,flip;
    struct NXT_tx *feetx;
    struct pendinghalf base,rel;
};

char *pending_trade_error(struct pending_trade *pt)
{
    char *jsonstr;
    jsonstr = clonestr(pt->comment);
    if ( pt->feetx != 0 )
        free(pt->feetx);
    free(pt);
    return(jsonstr);
}

int32_t scale_qty(struct pending_trade *pt,char *NXTaddr,struct pendinghalf *base,struct pendinghalf *rel)
{
    char assetidstr[64]; struct NXT_asset *ap; int32_t createdflag; double ratio; uint64_t tmp,mult,assetid;
    if ( pt->askoffer == 0 )
        pt->srcarg = base->baseamount, assetid = base->assetid;
    else pt->srcarg = rel->baseamount, assetid = rel->assetid;
    if ( assetid == NXT_ASSETID )
        mult = 1;
    else
    {
        expand_nxt64bits(assetidstr,rel->assetid);
        ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
        mult = ap->mult;
    }
    printf("askoffer.%d srcarg %llu  srcmult.%llu\n",pt->askoffer,(long long)pt->srcarg,(long long)mult);
    if ( mult != 0 && pt->srcarg != 0 )
    {
        tmp = (pt->srcarg / mult);
        if ( pt->srcqty != 0 && pt->srcqty < tmp )
            pt->srcamount = mult * pt->srcqty, printf("srcamount path0: %.8f tmp %.8f\n",dstr(pt->srcamount),dstr(tmp));
        else pt->srcamount = (tmp * mult), printf("srcamount path1: %.8f tmp %.8f\n",dstr(pt->srcamount),dstr(tmp));
    } else return(0.);
    if ( pt->srcamount != 0 )
    {
        if ( pt->srcamount < pt->srcarg )
        {
            pt->ratio = ratio = ((double)pt->srcamount / pt->srcarg);
            if ( pt->askoffer == 0 )
                base->baseamount = pt->srcamount, base->relamount *= ratio, rel->baseamount *= ratio, rel->relamount *= ratio;
            else rel->baseamount = pt->srcamount, rel->relamount *= ratio, base->baseamount *= ratio, base->relamount *= ratio;
        }
        base->price = calc_price_volume(&base->vol,base->baseamount,base->relamount);
        printf("base: price %f vol %f amount %llu %llu\n",base->price,base->vol,(long long)base->baseamount,(long long)base->relamount);
        base->qty = calc_asset_qty(&base->avail,&base->priceNQT,NXTaddr,pt->askoffer==0,base->assetid,base->price,base->vol);
        rel->price = calc_price_volume(&rel->vol,rel->baseamount,rel->relamount);
        rel->qty = calc_asset_qty(&rel->avail,&rel->priceNQT,NXTaddr,pt->askoffer!=0,rel->assetid,rel->price,rel->vol);
        printf("qtyA %llu priceA %f volA %f, qtyB %lld priceB %f volB %f\n",(long long)base->qty,base->price,base->vol,(long long)rel->qty,rel->price,rel->vol);
        printf("ratio %f, srcamount %.8f srcarg %.8f srcqty %.8f -> (%f %f) (%f %f)\n",ratio,dstr(pt->srcamount),dstr(pt->srcarg),dstr(pt->srcqty),base->price,base->vol,rel->price,rel->vol);
        if ( pt->srcamount == 0 || base->qty == 0 || rel->qty == 0 || base->priceNQT == 0 || rel->priceNQT == 0 )
        {
            sprintf(pt->comment,"{\"error\":\"%s\",\"descr\":\"NXT.%llu makeoffer3 srcamount.%.8f qtyA %.8f assetA.%llu price %.8f for qtyB %.8f assetB.%llu price.%.8f\"}\n","illegal parameter",(long long)calc_nxt64bits(NXTaddr),dstr(pt->srcamount),dstr(base->qty),(long long)base->assetid,dstr(base->priceNQT),dstr(rel->qty),(long long)rel->assetid,dstr(rel->priceNQT));
            return(-1);
        }
        return(0);
    } else return(-1);
}

int32_t set_pendinghalf(struct pendinghalf *half,uint64_t assetid,uint64_t baseamount,uint64_t relamount,uint64_t quoteid,uint64_t offerNXT,char *exchangestr)
{
    struct exchange_info *find_exchange(char *exchangestr,int32_t createflag);
    struct exchange_info *exchange;
    half->assetid = assetid, half->baseamount = baseamount, half->relamount = relamount, half->quoteid = quoteid, half->offerNXT = offerNXT;
    strcpy(half->exchange,exchangestr);
    if ( (exchange= find_exchange(half->exchange,0)) != 0 )
    {
        half->exchangeid = exchange->exchangeid;
        if ( half->offerNXT != 0 && half->baseamount != 0 && half->relamount != 0 )
            return(half->exchangeid == INSTANTDEX_EXCHANGEID);
    }
    return(-1);
}

// phasing! https://nxtforum.org/index.php?topic=6490.msg171048#msg171048
int32_t extract_pendinghalf(struct pendinghalf *half,cJSON *obj,uint64_t assetid)
{
    char exchange[MAX_JSON_FIELD];
    uint64_t baseamount,relamount,quoteid,offerNXT;
    if ( obj != 0 )
    {
        baseamount = get_API_nxt64bits(cJSON_GetObjectItem(obj,"baseamount"));
        relamount = get_API_nxt64bits(cJSON_GetObjectItem(obj,"relamount"));
        quoteid = get_API_nxt64bits(cJSON_GetObjectItem(obj,"quoteid"));
        offerNXT = get_API_nxt64bits(cJSON_GetObjectItem(obj,"offerNXT"));
        copy_cJSON(exchange,cJSON_GetObjectItem(obj,"exchange"));
        return(set_pendinghalf(half,assetid,baseamount,relamount,quoteid,offerNXT,exchange));
    }
    return(-1);
}

int32_t need_to_requesthalf(struct pendinghalf *half,int32_t dir,struct pending_trade *pt)
{
    if ( half->exchangeid == INSTANTDEX_NXTAEID || strcmp("unconf",half->exchange) == 0 || (half->exchangeid == INSTANTDEX_EXCHANGEID && half->offerNXT != pt->nxt64bits) )
        return(0);
    return(1);
}

int32_t process_Pending_tradesQ(struct pending_trade **ptp,void **ptrs)
{
    char *NXTACCTSECRET; struct NXT_tx **txptrs; int32_t i,matched; uint64_t txid,feetxid; struct pending_trade *pt = *ptp;
    NXTACCTSECRET = ptrs[0], txptrs = ptrs[1];
    //printf("process feetxid %llu\n",(long long)pt->feetx->txid);
    if ( milliseconds() > pt->expiration )
    {
        pt->errcode = -1;
        printf("(%f > %f) expired pending trade.(%s)\n",milliseconds(),pt->expiration,pt->comment);
        return(-1);
    }
    for (i=matched=0; i<pt->numrequired; i++)
    {
        if ( (txid= pt->txids[i]) == 0 )
            break;
        if ( search_txptrs(txptrs,txid) >= 0 )
            matched++;
    }
    if ( matched == pt->numrequired )
    {
        if ( (pt->actual_feetxid= issue_broadcastTransaction(&pt->errcode,0,pt->feesignedtx,NXTACCTSECRET)) != pt->feetx->txid )
        {
            printf("Jump trades triggered! feetxid.%llu but unexpected should have been %llu\n",(long long)feetxid,(long long)pt->feetx->txid);
            return(-1);
        }
        else
        {
            printf("Jump trades triggered! feetxid.%llu\n",(long long)feetxid);
            return(1);
        }
    } else printf("matched.%d vs numrequired.%d\n",matched,pt->numrequired);
    return(0);
}

void update_openorder(struct InstantDEX_quote *iQ,struct NXT_tx *tx)
{
    printf("update_openorder iQ.%llu with tx.%llu\n",(long long)iQ->quoteid,(long long)tx->txid);
}

void poll_jumptrades(char *NXTaddr,char *NXTACCTSECRET)
{
    int32_t update_iQ_flags(struct NXT_tx *txptrs[],int32_t maxtx,uint64_t baseid,uint64_t relid);
    cJSON *openorders_json(char *NXTaddr);
    uint64_t calc_quoteid(struct InstantDEX_quote *iQ);
    struct InstantDEX_quote *iQ;
    cJSON *json,*array,*item; struct NXT_tx *txptrs[MAX_TXPTRS]; void *ptrs[2];
    int32_t i,j,n,numtx; uint64_t quoteid,baseid,relid;
    ptrs[0] = NXTACCTSECRET, ptrs[1] = txptrs;
    if ( (numtx= update_iQ_flags(txptrs,(sizeof(txptrs)/sizeof(*txptrs))-1,0,0)) > 0 )
    {
        if ( (json= openorders_json(NXTaddr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"openorders")) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    item = cJSON_GetArrayItem(array,i);
                    if ( (quoteid= get_API_nxt64bits(cJSON_GetObjectItem(item,"quoteid"))) != 0 )
                    {
                        baseid = get_API_nxt64bits(cJSON_GetObjectItem(item,"baseid"));
                        relid = get_API_nxt64bits(cJSON_GetObjectItem(item,"relid"));
                        iQ = (struct InstantDEX_quote *)get_API_nxt64bits(cJSON_GetObjectItem(item,"iQ"));
                        printf("iQ.%p quoteid.%llu vs %llu\n",iQ,(long long)calc_quoteid(iQ),(long long)quoteid);
                        // need to update with new NXT blocks and InstantDEX orderbooks
                        for (j=0; j<numtx; j++)
                            if ( txptrs[j]->quoteid == quoteid ||  txptrs[j]->assetidbits == baseid ||  txptrs[j]->assetidbits == relid )
                                update_openorder(iQ,txptrs[j]);
                    }
                }
            }
            free_json(json);
        }
        process_pingpong_queue(&Pending_tradesQ,ptrs);
        free_txptrs(txptrs,numtx);
    }
}

int32_t submit_trade(uint64_t *txids,struct pendinghalf *half,int32_t dir,struct pending_trade *pt,char *NXTACCTSECRET)
{
    char *jsonstr;
    int32_t numrequired = 0;
    if ( need_to_requesthalf(half,dir,pt) != 0 )
    {
        // send message to other party if not initiator, via direct message and unconf
        printf("request NXT.%llu: asset.%llu qty.%llu price.%llu total %.8f NXT dir.%d\n",(long long)half->offerNXT,(long long)half->assetid,(long long)half->qty,(long long)half->priceNQT,dstr(half->qty * half->priceNQT),dir);
    }
    else if ( half->assetid != NXT_ASSETID && half->qty != 0 && half->priceNQT != 0 )
    {
        if ( (txids[numrequired++] = submit_triggered_bidask(&jsonstr,(dir > 0) ? "placeBidOrder" : "placeAskOrder",pt->nxt64bits,NXTACCTSECRET,half->assetid,half->qty,half->priceNQT,pt->triggerhash,pt->comment)) == 0 )
        {
            if ( jsonstr != 0 )
                sprintf(pt->comment+strlen(pt->comment)-1,",\"error\":[%s]}",jsonstr!=0?jsonstr:"submit_trade failed");
            free(jsonstr);
            return(-1);
        }
    }
    return(numrequired);
}

double calc_asset_QNT(struct pendinghalf *half,uint64_t nxt64bits,int32_t checkflag,uint64_t srcqty)
{
    char NXTaddr[64],assetidstr[64]; struct NXT_asset *ap;
    double ratio = 1.;
    int32_t createdflag;
    int64_t unconfirmed,balance;
    expand_nxt64bits(NXTaddr,nxt64bits);
    expand_nxt64bits(assetidstr,half->assetid);
    ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
    if ( ap->mult != 0 )
    {
        half->priceNQT = (half->relamount * ap->mult) / half->baseamount;
        if ( (half->qty= half->baseamount / ap->mult) == 0 )
            return(0);
        if ( srcqty < half->qty )
        {
            ratio = (double)srcqty / half->qty;
            half->baseamount *= ratio, half->relamount *= ratio;
            half->price = calc_price_volume(&half->vol,half->baseamount,half->relamount);
            half->priceNQT = (half->relamount * ap->mult) / half->baseamount;
            if ( (half->qty= half->baseamount / ap->mult) == 0 )
                return(0);
        }
        else if ( half->price == 0. )
            half->price = calc_price_volume(&half->vol,half->baseamount,half->relamount);
        balance = get_asset_quantity(&unconfirmed,NXTaddr,assetidstr);
        printf("%s balance %.8f unconfirmed %.8f vs price %llu qty %llu for asset.%s | (%f * %f) * (%ld / %llu)\n",NXTaddr,dstr(balance),dstr(unconfirmed),(long long)half->priceNQT,(long long)half->qty,assetidstr,half->vol,half->price,SATOSHIDEN,(long long)ap->mult);
        if ( checkflag != 0 && (balance < half->qty || unconfirmed < half->qty) )
            return(0);
    } else printf("%llu null apmult\n",(long long)half->assetid);
    return(ratio);
}

char *makeoffer3(char *NXTaddr,char *NXTACCTSECRET,double price,double volume,int32_t flip,uint64_t srcqty,uint64_t baseid,uint64_t relid,cJSON *baseobj,cJSON *relobj,uint64_t quoteid,int32_t askoffer,char *exchange,uint64_t baseamount,uint64_t relamount,uint64_t offerNXT)
{
    struct pending_trade *pt = calloc(1,sizeof(*pt));
    struct pendinghalf *first,*second,*base,*rel; struct NXT_tx T; char buf[MAX_JSON_FIELD]; int32_t retval;
    pt->nxt64bits = calc_nxt64bits(NXTaddr);
    if ( flip != 0 )
        askoffer = 1;
    pt->askoffer = askoffer, pt->flip = flip;
    pt->baseid = baseid, pt->relid = relid, pt->quoteid = quoteid, pt->srcqty = srcqty, pt->price = price, pt->volume = volume;
    if ( baseobj != 0 && relobj != 0 )
    {
        if ( extract_pendinghalf(&pt->base,baseobj,baseid) > 0 )
            pt->fee += INSTANTDEX_FEE;
        if ( extract_pendinghalf(&pt->rel,relobj,relid) > 0 )
            pt->fee += INSTANTDEX_FEE;
        if ( pt->fee < INSTANTDEX_FEE )
            pt->fee = INSTANTDEX_FEE;
        if ( scale_qty(pt,NXTaddr,&pt->base,&pt->rel) < 0 )
            return(pending_trade_error(pt));
    }
    else
    {
        pt->fee = INSTANTDEX_FEE;
        if ( baseid != NXT_ASSETID )
            set_pendinghalf(&pt->base,baseid,baseamount,relamount,quoteid,offerNXT,exchange), pt->ratio = calc_asset_QNT(&pt->base,pt->nxt64bits,1,pt->srcqty);
        else set_pendinghalf(&pt->rel,relid,relamount,baseamount,quoteid,offerNXT,exchange), pt->ratio = calc_asset_QNT(&pt->rel,pt->nxt64bits,1,pt->srcqty);
    }
    base = &pt->base, rel = &pt->rel;
    sprintf(pt->comment,"{\"requestType\":\"makeoffer3\",\"NXT\":\"%llu\",\"ratio\":\"%.8f\",\"srcqty\":\"%llu\",\"baseid\":\"%llu\",\"relid\":\"%llu\",\"frombase\":\"%llu\",\"fromrel\":\"%llu\",\"tobase\":\"%llu\",\"torel\":%llu,\"qtyA\":\"%llu\",\"priceNQTA\":\"%llu\",\"qtyB\":\"%llu\",\"priceNQTB\":\"%llu\",\"fee\":\"%llu\",\"quoteid\":\"%llu\"}",(long long)calc_nxt64bits(NXTaddr),pt->ratio,(long long)pt->srcqty,(long long)pt->baseid,(long long)pt->relid,(long long)base->baseamount,(long long)base->relamount,(long long)rel->baseamount,(long long)rel->relamount,(long long)base->qty,(long long)base->priceNQT,(long long)rel->qty,(long long)rel->priceNQT,(long long)pt->fee,(long long)pt->quoteid);
    set_NXTtx(pt->nxt64bits,&T,NXT_ASSETID,pt->fee,calc_nxt64bits(INSTANTDEX_ACCT),-1);
    strcpy(T.comment,pt->comment);
    if ( (pt->feetx= sign_NXT_tx(pt->feeutxbytes,pt->feesignedtx,NXTACCTSECRET,pt->nxt64bits,&T,0,1.)) != 0 )
    {
        get_txhashes(pt->feesighash,pt->triggerhash,pt->feetx);
        sprintf(pt->comment + strlen(pt->comment) - 1,",\"feetxid\":\"%llu\",\"triggerhash\":\"%s\"}",(long long)pt->feetx->txid,pt->triggerhash);
        strcpy(buf,pt->comment);
        if ( strlen(pt->triggerhash) == 64 )
        {
            if ( pt->askoffer == 0 )
                first = &pt->base, second = &pt->rel;
            else first = &pt->rel, second = &pt->base;
            if ( (retval= submit_trade(&pt->txids[pt->numrequired],first,-1,pt,NXTACCTSECRET)) < 0 )
                return(pending_trade_error(pt));
            pt->numrequired = retval;
            if ( (retval= submit_trade(&pt->txids[pt->numrequired],second,1,pt,NXTACCTSECRET)) < 0 )
                return(pending_trade_error(pt));
            pt->numrequired += retval;
            pt->expiration = milliseconds() + 2. * JUMPTRADE_SECONDS * 1000;
            //printf("queue pt.%p expiration %f feetxid.%llu\n",pt,pt->expiration,pt->feetx->txid);
            queue_enqueue("pending_trade",&Pending_tradesQ.pingpong[0],pt);
        } else printf("invalid triggerhash.(%s).%ld\n",pt->triggerhash,strlen(pt->triggerhash));
        return(clonestr(pt->comment));
    }
    return(clonestr("{\"error\":\"couldnt submit fee tx\"}"));
}

#endif

