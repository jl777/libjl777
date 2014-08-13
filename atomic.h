//
//  atomic.h
//  xcode
//
//  Created by jl777 on 7/16/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//
// is_relevant_coinvalue?
// secure and redundant telepod storage, but need to enforce deletions on xfer!
// "telepod" JSON API -> auto trigger copy
// jl777.conf options: privacyserver

#ifndef xcode_atomic_h
#define xcode_atomic_h

#define INSTANTDEX_ACCT "4383817337783094122"

struct NXT_tx
{
    unsigned char refhash[32],sighash[32],fullhash[32];
    uint64_t senderbits,recipientbits,assetidbits;
    int64_t feeNQT;
    union { int64_t amountNQT; int64_t quantityQNT; };
    int32_t deadline,type,subtype,verify,number;
    char comment[128];
};

int32_t NXTutxcmp(struct NXT_tx *ref,struct NXT_tx *tx,double myshare)
{
    if ( ref->senderbits == tx->senderbits && ref->recipientbits == tx->recipientbits && ref->type == tx->type && ref->subtype == tx->subtype)
    {
        if ( ref->feeNQT != tx->feeNQT || ref->deadline != tx->deadline )
            return(-1);
        if ( ref->assetidbits != ORDERBOOK_NXTID )
        {
            if ( ref->assetidbits == tx->assetidbits && fabs((ref->quantityQNT*myshare) - tx->quantityQNT) < 0.5 && strcmp(ref->comment,tx->comment) == 0 )
                return(0);
        }
        else
        {
            if ( fabs((ref->amountNQT*myshare) - tx->amountNQT) < 0.5 )
                return(0);
        }
    }
    return(-1);
}

cJSON *gen_NXT_tx_json(struct NXT_tx *utx,char *reftxid,double myshare,char *NXTACCTSECRET,uint64_t nxt64bits)
{
    cJSON *json = 0;
    char cmd[1024],destNXTaddr[64],assetidstr[64],*retstr;
    if ( utx->senderbits == nxt64bits )
    {
        expand_nxt64bits(destNXTaddr,utx->recipientbits);
        cmd[0] = 0;
        if ( utx->type == 0 && utx->subtype == 0 )
            sprintf(cmd,"%s=sendMoney&amountNQT=%lld",_NXTSERVER,(long long)(utx->amountNQT*myshare));
        else if ( utx->type == 2 && utx->subtype == 1 )
        {
            expand_nxt64bits(assetidstr,utx->assetidbits);
            sprintf(cmd,"%s=transferAsset&asset=%s&quantityQNT=%lld",_NXTSERVER,assetidstr,(long long)(utx->quantityQNT*myshare));
            if ( utx->comment[0] != 0 )
                strcat(cmd,"&comment="),strcat(cmd,utx->comment);
        }
        else printf("unsupported type.%d subtype.%d\n",utx->type,utx->subtype);
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
                if ( json != 0 )
                    printf("Parsed.(%s)\n",cJSON_Print(json));
                free(retstr);
            }
        }
    } else printf("cant gen_NXT_txjson when sender is not me\n");
    return(json);
}

void set_NXTtx(uint64_t nxt64bits,struct NXT_tx *tx,uint64_t assetidbits,int64_t amount,uint64_t other64bits)
{
    struct NXT_tx U;
    memset(&U,0,sizeof(U));
    U.senderbits = nxt64bits;
    U.recipientbits = other64bits;
    U.assetidbits = assetidbits;
    if ( assetidbits != ORDERBOOK_NXTID )
    {
        U.type = 2;
        U.subtype = 1;
        U.quantityQNT = amount;
    } else U.amountNQT = amount;
    U.feeNQT = MIN_NQTFEE;
    U.deadline = DEFAULT_NXT_DEADLINE;
    *tx = U;
}

void truncate_utxbytes(char *utxbytes)
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
}

int32_t calc_raw_NXTtx(char *utxbytes,char *sighash,uint64_t assetidbits,int64_t amount,uint64_t other64bits,char *NXTACCTSECRET,uint64_t nxt64bits)
{
    int32_t retval = -1;
    struct NXT_tx U;
    cJSON *json;
    utxbytes[0] = sighash[0] = 0;
    set_NXTtx(nxt64bits,&U,assetidbits,amount,other64bits);
    json = gen_NXT_tx_json(&U,0,1.,NXTACCTSECRET,nxt64bits);
    if ( json != 0 )
    {
        if ( extract_cJSON_str(utxbytes,1024,json,"transactionBytes") > 0 && extract_cJSON_str(sighash,1024,json,"signatureHash") > 0 )
        {
            //truncate_utxbytes(utxbytes);
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
    uint64_t assetidbits,quantity;
    cJSON *attachmentobj;
    struct NXT_tx *utx = 0;
    char sender[1024],recipient[1024],deadline[1024],feeNQT[1024],amountNQT[1024],type[1024],subtype[1024],verify[1024],referencedTransaction[1024],quantityQNT[1024],comment[1024],assetidstr[1024],sighash[1024],fullhash[1024];
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
    comment[0] = 0;
    assetidbits = ORDERBOOK_NXTID;
    quantity = 0;
    size = sizeof(*utx);
    if ( strcmp(type,"2") == 0 && strcmp(subtype,"1") == 0 )
    {
        attachmentobj = cJSON_GetObjectItem(json,"attachment");
        if ( attachmentobj != 0 )
        {
            if ( extract_cJSON_str(assetidstr,sizeof(assetidstr),attachmentobj,"asset") > 0 )
                assetidbits = calc_nxt64bits(assetidstr);
            if ( extract_cJSON_str(comment,sizeof(comment),attachmentobj,"comment") > 0 )
                size += strlen(comment);
            if ( extract_cJSON_str(quantityQNT,sizeof(quantityQNT),attachmentobj,"quantityQNT") > 0 )
                quantity = calc_nxt64bits(quantityQNT);
        }
    }
    utx = malloc(size);
    memset(utx,0,size);
    if ( strlen(referencedTransaction) == 64 )
        decode_hex(utx->refhash,32,referencedTransaction);
    if ( strlen(fullhash) == 64 )
        decode_hex(utx->fullhash,32,fullhash);
    if ( strlen(sighash) == 64 )
        decode_hex(utx->sighash,32,sighash);
    utx->senderbits = calc_nxt64bits(sender);
    utx->recipientbits = calc_nxt64bits(recipient);
    utx->assetidbits = assetidbits;
    utx->feeNQT = calc_nxt64bits(feeNQT);
    if ( quantity != 0 )
        utx->quantityQNT = quantity;
    else utx->amountNQT = calc_nxt64bits(amountNQT);
    utx->deadline = atoi(deadline);
    utx->type = atoi(type);
    utx->subtype = atoi(subtype);
    utx->verify = (strcmp("true",verify) == 0);
    strcpy(utx->comment,comment);
    replace_backslashquotes(utx->comment);
    return(utx);
}

struct NXT_tx *sign_NXT_tx(char utxbytes[1024],char signedtx[1024],char *NXTACCTSECRET,uint64_t nxt64bits,struct NXT_tx *utx,char *reftxid,double myshare)
{
    cJSON *refjson,*txjson;
    char *parsed,*str,errstr[32];
    struct NXT_tx *refutx = 0;
    printf("sign_NXT_tx reftxid.(%s)\n",reftxid);
    txjson = gen_NXT_tx_json(utx,reftxid,myshare,NXTACCTSECRET,nxt64bits);
    utxbytes[0] = signedtx[0] = 0;
    if ( txjson != 0 )
    {
        if ( extract_cJSON_str(errstr,sizeof(errstr),txjson,"errorCode") > 0 )
        {
            str = cJSON_Print(txjson);
            strcpy(signedtx,str);
            strcpy(utxbytes,errstr);
        }
        else if ( extract_cJSON_str(utxbytes,1024,txjson,"unsignedTransactionBytes") > 0 &&
                 extract_cJSON_str(signedtx,1024,txjson,"transactionBytes") > 0 )
        {
            //truncate_utxbytes(utxbytes);
            if ( (parsed = issue_parseTransaction(0,signedtx)) != 0 )
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
        if ( tx->assetidbits != asset )
            return(-3);
        if ( tx->quantityQNT != qty ) // tx->quantityQNT is union as long as same assetid, then these can be compared directly
            return(-4);
        return(0);
    } else return(-1);
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

/*
 int32_t update_atomic(struct NXT_acct *np,struct subatomic_tx *atx)
 {
 cJSON *txjson,*json;
 int32_t j,status = 0;
 char signedtx[1024],fullhash[128],*parsed,*retstr,*padded;
 struct atomic_swap *sp;
 struct NXT_tx *utx,*refutx = 0;
 sp = &atx->swap;
 if ( atx->longerflag != 1 )
 {
 //printf("atomixtx waiting.%d atx.%p\n",sp->atomictx_waiting,atx);
 if ( sp->atomictx_waiting != 0 )
 {
 printf("GOT.(%s)\n",sp->signedtxbytes[1]);
 if ( (parsed = issue_parseTransaction(Global_subatomic->curl_handle,sp->signedtxbytes[1])) != 0 )
 {
 json = cJSON_Parse(parsed);
 if ( json != 0 )
 {
 refutx = set_NXT_tx(sp->jsons[1]);
 utx = set_NXT_tx(json);
 //printf("refutx.%p utx.%p verified.%d\n",refutx,utx,utx->verify);
 if ( utx != 0 && refutx != 0 && utx->verify != 0 )
 {
 if ( NXTutxcmp(refutx,utx,1.) == 0 )
 {
 padded = malloc(strlen(sp->txbytes[0]) + 129);
 strcpy(padded,sp->txbytes[0]);
 for (j=0; j<128; j++)
 strcat(padded,"0");
 retstr = issue_signTransaction(Global_subatomic->curl_handle,padded);
 free(padded);
 printf("got signed tx that matches agreement submit.(%s) (%s)\n",padded,retstr);
 if ( retstr != 0 )
 {
 txjson = cJSON_Parse(retstr);
 if ( txjson != 0 )
 {
 extract_cJSON_str(sp->signedtxbytes[0],sizeof(sp->signedtxbytes[0]),txjson,"transactionBytes");
 if ( extract_cJSON_str(fullhash,sizeof(fullhash),txjson,"fullHash") > 0 )
 {
 if ( strcmp(fullhash,sp->fullhash[0]) == 0 )
 {
 printf("broadcast (%s) and (%s)\n",sp->signedtxbytes[0],sp->signedtxbytes[1]);
 status = SUBATOMIC_COMPLETED;
 }
 else printf("ERROR: can't reproduct fullhash of trigger tx %s != %s\n",fullhash,sp->fullhash[0]);
 }
 free_json(txjson);
 }
 free(retstr);
 }
 } else printf("tx compare error\n");
 }
 if ( utx != 0 ) free(utx);
 if ( refutx != 0 ) free(refutx);
 free_json(json);
 } else printf("error JSON parsing.(%s)\n",parsed);
 free(parsed);
 } else printf("error parsing (%s)\n",sp->signedtxbytes[1]);
 sp->atomictx_waiting = 0;
 }
 }
 else if ( atx->longerflag == 1 )
 {
 if ( sp->numfragis == 0 )
 {
 utx = set_NXT_tx(sp->jsons[0]);
 if ( utx != 0 )
 {
 refutx = sign_NXT_tx(signedtx,NXTACCTSECRECT,nxt64bits,utx,sp->fullhash[1],1.);
 if ( refutx != 0 )
 {
 if ( NXTutxcmp(refutx,utx,1.) == 0 )
 {
 printf("signed and referenced tx verified\n");
 safecopy(sp->signedtxbytes[0],signedtx,sizeof(sp->signedtxbytes[0]));
 sp->numfragis = share_atomictx(np,sp->signedtxbytes[0],1);
 status = SUBATOMIC_COMPLETED;
 }
 free(refutx);
 }
 free(utx);
 }
 }
 else
 {
 // wont get here now, eventually add checks for blockchain completion or direct xfer from other side
 share_atomictx(np,sp->signedtxbytes[0],1);
 }
 }
 return(status);
 }
 
 void foo()
 {
 char signedtx[1024];
 double myshare = .01;
 struct NXT_tx *utx,*refutx = 0;
 utx = set_NXT_tx(sp->jsons[0]);
 if ( utx != 0 )
 {
 refutx = sign_NXT_tx(signedtx,NXTACCTSECRECT,nxt64bits,utx,sp->fullhash[1],myshare);
 if ( refutx != 0 )
 {
 if ( NXTutxcmp(refutx,utx,myshare) == 0 )
 {
 printf("signed and referenced tx verified\n");
 safecopy(sp->signedtxbytes[0],signedtx,sizeof(sp->signedtxbytes[0]));
 //sp->numfragis = share_atomictx(np,sp->signedtxbytes[0],1);
 //status = SUBATOMIC_COMPLETED;
 sp->numfragis = 2;
 sp->mytx = utx;
 return(1);
 }
 free(refutx);
 }
 //free(utx);
 }
 }
 */
#endif
