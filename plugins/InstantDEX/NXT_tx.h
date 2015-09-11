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

#ifndef xcode_NXT_tx_h
#define xcode_NXT_tx_h

#define MAX_TXPTRS 1024

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

uint32_t calc_expiration(struct NXT_tx *tx)
{
    if ( tx == 0 || tx->timestamp == 0 )
        return(0);
    return((NXT_GENESISTIME + tx->timestamp) + 60*tx->deadline);
}

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

struct NXT_tx *search_txptrs(struct NXT_tx *txptrs[],uint64_t txid,uint64_t quoteid,uint64_t baseid,uint64_t relid)
{
    int32_t i; struct NXT_tx *tx;
    for (i=0; i<MAX_TXPTRS; i++)
    {
        if ( (tx= txptrs[i]) == 0 )
            return(0);
        if ( quoteid != 0 )
            printf("Q.%llu ",(long long)tx->quoteid);
        if ( (txid != 0 && tx->txid == txid) || (quoteid != 0 && tx->quoteid == quoteid) || (baseid != 0 && tx->assetidbits == baseid) || (relid != 0 && tx->assetidbits == relid) )
            return(tx);
    }
    return(0);
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
    char secret[8192],cmd[MAX_JSON_FIELD],destNXTaddr[64],assetidstr[64],*retstr;
    if ( utx->senderbits == nxt64bits )
    {
        expand_nxt64bits(destNXTaddr,utx->recipientbits);
        cmd[0] = 0;
        if ( utx->type == 0 && utx->subtype == 0 )
            sprintf(cmd,"requestType=sendMoney&amountNQT=%lld",(long long)(utx->U.amountNQT*myshare));
        else
        {
            expand_nxt64bits(assetidstr,utx->assetidbits);
            if ( utx->type == 2 && utx->subtype == 1 )
                sprintf(cmd,"requestType=transferAsset&asset=%s&quantityQNT=%lld",assetidstr,(long long)(utx->U.quantityQNT*myshare));
            else if ( utx->type == 5 && utx->subtype == 3 )
                sprintf(cmd,"requestType=transferCurrency&currency=%s&units=%lld",assetidstr,(long long)(utx->U.quantityQNT*myshare));
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
            escape_code(secret,NXTACCTSECRET);
            sprintf(cmd+strlen(cmd),"&deadline=%u&feeNQT=%lld&secretPhrase=%s&recipient=%s&broadcast=false",utx->deadline,(long long)utx->feeNQT,secret,destNXTaddr);
            if ( reftxid != 0 && reftxid[0] != 0 )
                sprintf(cmd+strlen(cmd),"&referencedTransactionFullHash=%s",reftxid);
            //printf("generated cmd.(%s) reftxid.(%s)\n",cmd,reftxid);
            retstr = issue_NXTPOST(cmd);
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
    char assetidstr[64]; int32_t decimals;
    uint64_t fee = 0;
    struct NXT_tx U;
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
        U.type = get_assettype(&decimals,assetidstr);
        //U.subtype = ap->subtype;
        U.U.quantityQNT = amount - fee;
    } else U.U.amountNQT = amount - fee;
    U.feeNQT = MIN_NQTFEE;
    U.deadline = INSTANTDEX_TRIGGERDEADLINE;
    printf("set_NXTtx(%llu -> %llu) %.8f of %llu\n",(long long)U.senderbits,(long long)U.recipientbits,dstr(amount),(long long)assetidbits);
    *tx = U;
    return(fee);
}

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
    char sender[MAX_JSON_FIELD],recipient[MAX_JSON_FIELD],deadline[MAX_JSON_FIELD],feeNQT[MAX_JSON_FIELD],amountNQT[MAX_JSON_FIELD],type[MAX_JSON_FIELD],subtype[MAX_JSON_FIELD],verify[MAX_JSON_FIELD],referencedTransaction[MAX_JSON_FIELD],quantityQNT[MAX_JSON_FIELD],priceNQT[MAX_JSON_FIELD],comment[MAX_JSON_FIELD],assetidstr[MAX_JSON_FIELD],sighash[MAX_JSON_FIELD],fullhash[MAX_JSON_FIELD],timestamp[MAX_JSON_FIELD],transaction[MAX_JSON_FIELD];
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
    //if ( strcmp(type,"2") == 0 || strcmp(type,"5") == 0 )//&& strcmp(subtype,"3") == 0) )
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
    utx->deadline = myatoi(deadline,1000);
    utx->type = myatoi(type,256);
    utx->subtype = myatoi(subtype,256);
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
            if ( (parsed= issue_parseTransaction(signedtx)) != 0 )
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
    if ( (parsed = issue_parseTransaction(txbytes)) != 0 )
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

uint32_t get_txhashes(char *sighash,char *fullhash,struct NXT_tx *tx)
{
    init_hexbytes_noT(sighash,tx->sighash.bytes,sizeof(tx->sighash));
    init_hexbytes_noT(fullhash,tx->fullhash.bytes,sizeof(tx->fullhash));
    return(calc_expiration(tx));
}

uint64_t send_feetx(uint64_t assetbits,uint64_t fee,char *fullhash,char *comment)
{
    char feeutx[MAX_JSON_FIELD],signedfeetx[MAX_JSON_FIELD];
    struct NXT_tx feeT,*feetx;
    uint64_t feetxid = 0;
    int32_t errcode;
    set_NXTtx(calc_nxt64bits(SUPERNET.NXTADDR),&feeT,assetbits,fee,calc_nxt64bits(INSTANTDEX_ACCT),-1);
    if ( comment != 0 && comment[0] != 0 )
        strcpy(feeT.comment,comment);
    printf("feetx for %llu %.8f fullhash.(%s) secret.(%s)\n",(long long)SUPERNET.NXTADDR,dstr(fee),fullhash,SUPERNET.NXTACCTSECRET);
    if ( (feetx= sign_NXT_tx(feeutx,signedfeetx,SUPERNET.NXTACCTSECRET,calc_nxt64bits(SUPERNET.NXTADDR),&feeT,fullhash,1.)) != 0 )
    {
        printf("broadcast fee for %llu\n",(long long)assetbits);
        feetxid = issue_broadcastTransaction(&errcode,0,signedfeetx,SUPERNET.NXTACCTSECRET);
        free(feetx);
    }
    return(feetxid);
}

#ifdef INSIDE_MGW
int32_t NXT_set_revassettrade(uint32_t ind,uint64_t key[2])
{
    void *obj;
    //printf("NXT_set_revassettrade\n");
    if ( DB_NXTtrades != 0 && DB_NXTtrades->db != 0 && (obj= sp_object(DB_NXTtrades->db)) != 0 )
    {
        if ( sp_set(obj,"key",&ind,sizeof(ind)) == 0 && sp_set(obj,"value",key,sizeof(*key)*2) == 0 )
            return(sp_set(DB_NXTtrades->db,obj));
        else
        {
            sp_destroy(obj);
            printf("error NXT_set_revassettrade ind.%d\n",ind);
        }
    }
    return(-1);
}

int32_t NXT_revassettrade(uint64_t key[2],uint32_t ind)
{
    void *obj,*result,*value; int32_t len = 0;
    //printf("NXT_revassettrade\n");
    memset(key,0,sizeof(*key)*2);
    if ( DB_NXTtrades != 0 && DB_NXTtrades->db != 0 && (obj= sp_object(DB_NXTtrades->db)) != 0 )
    {
        if ( sp_set(obj,"key",&ind,sizeof(ind)) == 0 && (result= sp_get(DB_NXTtrades->db,obj)) != 0 )
        {
            value = sp_get(result,"value",&len);
            if ( len == sizeof(*key)*2 )
                memcpy(key,value,len);
            else printf("NXT_revassettrade mismatched len.%d vs %ld\n",len,sizeof(*key)*2);
            sp_destroy(result);
        }
    }
    return(len);
}

int32_t NXT_add_assettrade(struct assettrade *dest,struct assettrade *tp,uint32_t ind)
{
    void *obj; uint64_t key[2];
    //printf("NXT_add_assettrade\n");
    if ( DB_NXTtrades != 0 && DB_NXTtrades->db != 0 && tp != 0 )
    {
        key[0] = tp->bidorder, key[1] = tp->askorder;
        if ( (obj= sp_object(DB_NXTtrades->db)) != 0 )
        {
            if ( sp_set(obj,"key",key,sizeof(key)) == 0 && sp_set(obj,"value",tp,sizeof(*tp)) == 0 )
                sp_set(DB_NXTtrades->db,obj);
            else
            {
                sp_destroy(obj);
                printf("error NXT_add_assettrade %llu ind.%d\n",(long long)(tp->bidorder ^ tp->askorder),ind);
            }
        }
        NXT_set_revassettrade(ind,key);
    }
    return(0);
}

int32_t NXT_assettrade(struct assettrade *dest,struct assettrade *tp,uint32_t ind)
{
    void *obj,*result,*value; int32_t len = 0; uint64_t key[2];
    //printf("NXT_assettrade\n");
    if ( DB_NXTtrades != 0 && DB_NXTtrades->db != 0 && (obj= sp_object(DB_NXTtrades->db)) != 0 )
    {
        key[0] = tp->bidorder, key[1] = tp->askorder;
        if ( sp_set(obj,"key",key,sizeof(key)) == 0 && (result= sp_get(DB_NXTtrades->db,obj)) != 0 )
        {
            value = sp_get(result,"value",&len);
            if ( len == sizeof(*dest) )
                memcpy(dest,value,len);
            else printf("ERROR unexpected len.%d vs %ld\n",len,sizeof(*dest));
            sp_destroy(result);
        }
    }
    return(len);
}
#else
extern struct ramkv777 *DB_revNXTtrades,*DB_NXTtrades;
int32_t NXT_set_revassettrade(uint32_t ind,uint64_t key[2])
{
printf("NXT_set_revassettrade.%u -> %llu\n",ind,(long long)key[0]);
    if ( DB_revNXTtrades != 0 )
    {
        if ( ramkv777_write(DB_revNXTtrades,&ind,key,sizeof(*key)*2) == 0 )
            printf("error NXT_set_revassettrade ind.%d\n",ind);
        else return(0);
    }
    return(-1);
}

int32_t NXT_revassettrade(uint64_t key[2],uint32_t ind)
{
    void *value; int32_t len = 0;
printf("NXT_revassettrade\n");
    memset(key,0,sizeof(*key)*2);
    if ( DB_revNXTtrades != 0 )
    {
        if ( (value= ramkv777_read(&len,DB_revNXTtrades,&ind)) != 0 && len == sizeof(*key)*2 )
            memcpy(key,value,len);
    }
    return(len);
}

int32_t NXT_add_assettrade(struct assettrade *dest,struct assettrade *tp,uint32_t ind)
{
    uint64_t key[2];
printf("NXT_add_assettrade\n");
    if ( DB_NXTtrades != 0 )
    {
        key[0] = tp->bidorder, key[1] = tp->askorder;
        if ( ramkv777_write(DB_NXTtrades,key,tp,sizeof(*tp)) != 0 )
            NXT_set_revassettrade(ind,key);
        else printf("error writing NXT assettrade\n");
    }
    return(0);
}

int32_t NXT_assettrade(struct assettrade *dest,struct assettrade *tp,uint32_t ind)
{
    void *value; int32_t len = 0; uint64_t key[2];
printf("NXT_assettrade\n");
    if ( DB_NXTtrades != 0 )
    {
        key[0] = tp->bidorder, key[1] = tp->askorder;
        value = ramkv777_read(&len,DB_NXTtrades,key);
        if ( len == sizeof(*dest) )
            memcpy(dest,value,len);
        else printf("ERROR unexpected len.%d vs %ld\n",len,sizeof(*dest));
        return(len);
    }
    return(0);
}
#endif

int32_t NXT_trade(struct assettrade *tp,uint32_t ind)
{
    struct assettrade T; int32_t flag = 1;
    if ( NXT_assettrade(&T,tp,ind) == sizeof(T) )
    {
        if ( memcmp(&T,tp,sizeof(*tp)) != 0 )
            printf("mismatched NXT_trade ind.%d for %llu\n",ind,(long long)(tp->bidorder ^ tp->askorder));
        else flag = 0;
    }
    if ( flag != 0 )
        return(NXT_add_assettrade(&T,tp,ind));
    return(0);
}

uint64_t set_assettrade(int32_t i,int32_t n,struct assettrade *tp,cJSON *json)
{
    uint64_t ap_mult; char type[64],name[4096];
    memset(tp,0,sizeof(*tp));
    /*
     "seller": "13507302008315288445",
     "quantityQNT": "4217933",
     "bidOrder": "7711071669082465415",
     "sellerRS": "NXT-V8VX-MLHS-NK94-DMWWQ",
     "buyer": "1533153801325642313",
     "priceNQT": "490000",
     "askOrder": "18381562686533022497",
     "buyerRS": "NXT-W2LB-ABK2-M37N-3VQKC",
     "decimals": 4,
     "name": "SkyNET",
     "block": "3211297665777998350",
     "asset": "6854596569382794790",
     "askOrderHeight": 443292,
     "bidOrderHeight": 451101,
     "tradeType": "buy",
     "timestamp": 49051994,
     "height": 451101
     */
    if ( (tp->assetid= get_API_nxt64bits(cJSON_GetObjectItem(json,"asset"))) != 0 )
    {
        ap_mult = get_assetmult(tp->assetid);
        if ( ap_mult != 0 )
        {
            tp->price = SATOSHIDEN * get_API_nxt64bits(cJSON_GetObjectItem(json,"priceNQT")) / ap_mult;
            tp->amount = (get_API_nxt64bits(cJSON_GetObjectItem(json,"quantityQNT")) * ap_mult);
        }
        copy_cJSON(type,cJSON_GetObjectItem(json,"tradeType"));
        if ( strcmp(type,"sell") == 0 )
            tp->sellflag = 1;
        tp->seller = get_API_nxt64bits(cJSON_GetObjectItem(json,"seller"));
        tp->buyer = get_API_nxt64bits(cJSON_GetObjectItem(json,"buyer"));
        tp->askorder = get_API_nxt64bits(cJSON_GetObjectItem(json,"askOrder"));
        tp->bidorder = get_API_nxt64bits(cJSON_GetObjectItem(json,"bidOrder"));
        tp->bidheight = juint(json,"bidOrderHeight");
        tp->askheight = juint(json,"askOrderHeight");
        if ( 1 )
        {
            copy_cJSON(name,cJSON_GetObjectItem(json,"name"));
            printf("%6d of %6d: %24llu -> %24llu %16.8f %16.8f %10s | bid.%-24llu %u | ask.%-24llu %u\n",i,n,(long long)tp->seller,(long long)tp->buyer,(tp->sellflag != 0 ? -1 : 1) * dstr(tp->price),dstr(tp->amount),name,(long long)tp->bidorder,tp->bidheight,(long long)tp->askorder,tp->askheight);
        }
    } else printf("no assetid\n");
    return(tp->assetid);
}

int32_t NXT_assettrades(struct assettrade *trades,long max,int32_t firstindex,int32_t lastindex)
{
    char cmd[1024],*jsonstr; cJSON *transfers,*array; struct assettrade T;
    int32_t i,n = 0; uint64_t assetidbits;
    sprintf(cmd,"requestType=getAllTrades");
    if ( firstindex >= 0 && lastindex >= firstindex )
        sprintf(cmd + strlen(cmd),"&firstIndex=%u&lastIndex=%u",firstindex,lastindex);
    if ( (jsonstr= issue_NXTPOST(cmd)) != 0 )
    {
        printf("(%s) -> len.%ld\n",cmd,strlen(jsonstr));
        if ( (transfers= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(transfers,"trades")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    if ( (assetidbits= set_assettrade(i,n,&T,cJSON_GetArrayItem(array,i))) != 0 )
                    {
                        if ( i < max )
                            trades[i] = T;
                        if ( firstindex < 0 && lastindex <= firstindex )
                            NXT_trade(&T,n - i);
                    }
                }
            } else printf("error getting array\n");
            free_json(transfers);
        } else printf("couldnt parse trades\n");
        free(jsonstr);
    }
    //if ( firstindex < 0 || lastindex <= firstindex )
        printf(" -> %d entries\n",n);
    return(n);
}

int32_t update_NXT_assettrades()
{
    struct assettrade *trades;
    int32_t max,len,verifyflag = 0;
    uint64_t key[2]; int32_t i,count = 0;
    return(0);
    max = 20000, trades = calloc(max,sizeof(*trades));
    if ( (len= NXT_revassettrade(key,0)) == sizeof(key) )
    {
        for (i=1; i<=count; i++)
        {
            NXT_revassettrade(key,i);
        }
        fprintf(stderr,"sequential tx.%d\n",count);
        NXT_revassettrade(key,count);
        printf("mostrecent.%llu count.%d\n",(long long)(key[0] ^ key[1]),count);
        for (i=0; i<max; i++)
        {
            if ( NXT_assettrades(&trades[i],1,i,i) == 1 && trades[i].bidorder == key[0] && trades[i].askorder == key[1] )
            {
                if ( i != 0 )
                    printf("count.%d i.%d mostrecent.%llu vs %llu\n",count,i,(long long)(key[0] ^ key[1]),(long long)(trades[i].bidorder ^ trades[i].askorder));
                while ( i-- > 0 )
                    NXT_trade(&trades[i],++count);
                break;
            }
        }
        if ( i == max )
            count = 0;
    } else printf("cant get count len.%d\n",len);
    if ( count == 0 )
        count = NXT_assettrades(trades,max - 1,-1,-1);
    if ( NXT_revassettrade(key,0) != sizeof(key) || key[0] != count )
        NXT_set_revassettrade(0,key);
    if ( verifyflag != 0 )
        NXT_assettrades(trades,max - 1,-1,-1);
    free(trades);
    return(count);
}

int32_t trigger_items(cJSON *item)
{
    char feeutxbytes[MAX_JSON_FIELD],feesignedtx[MAX_JSON_FIELD],triggerhash[65],feesighash[65];
    struct NXT_tx T,*feetx; uint32_t triggerheight,expiration;
    set_NXTtx(SUPERNET.my64bits,&T,NXT_ASSETID,INSTANTDEX_FEE,calc_nxt64bits(INSTANTDEX_ACCT),-1);
    triggerheight = _get_NXTheight(0) + FINISH_HEIGHT;
    if ( (feetx= sign_NXT_tx(feeutxbytes,feesignedtx,SUPERNET.NXTACCTSECRET,SUPERNET.my64bits,&T,0,1.)) != 0 )
    {
        expiration = get_txhashes(feesighash,triggerhash,feetx);
        jadd64bits(item,"feetxid",feetx->txid);
        jaddstr(item,"triggerhash",triggerhash);
        jaddnum(item,"triggerheight",triggerheight);
        //sprintf(offer->comment + strlen(offer->comment) - 1,",\"feetxid\":\"%llu\",\"triggerhash\":\"%s\",\"triggerheight\":\"%u\"}",(long long)feetx->txid,triggerhash,offer->triggerheight);
        //if ( strlen(triggerhash) == 64 && (retstr= submit_trades(offer,NXTACCTSECRET)) != 0 )
        //    return(retstr);
        return(0);
    }
    return(-1);
}

#endif
