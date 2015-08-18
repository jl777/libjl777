//
//  trades.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_trades_h
#define xcode_trades_h

struct tradehistory { uint64_t assetid,purchased,sold; };

struct tradehistory *_update_tradehistory(struct tradehistory *hist,uint64_t assetid,uint64_t purchased,uint64_t sold)
{
    int32_t i = 0;
    if ( hist == 0 )
        hist = calloc(1,sizeof(*hist));
    if ( hist[i].assetid != 0 )
    {
        for (i=0; hist[i].assetid!=0; i++)
            if ( hist[i].assetid == assetid )
                break;
    }
    if ( hist[i].assetid == 0 )
    {
        hist = realloc(hist,(i+2) * sizeof(*hist));
        memset(&hist[i],0,2 * sizeof(hist[i]));
        hist[i].assetid = assetid;
    }
    if ( hist[i].assetid == assetid )
    {
        hist[i].purchased += purchased;
        hist[i].sold += sold;
        printf("hist[%d] %llu +%llu -%llu -> (%llu %llu)\n",i,(long long)hist[i].assetid,(long long)purchased,(long long)sold,(long long)hist[i].purchased,(long long)hist[i].sold);
    } else printf("_update_tradehistory: impossible case!\n");
    return(hist);
}

struct tradehistory *update_tradehistory(struct tradehistory *hist,uint64_t srcasset,uint64_t srcamount,uint64_t destasset,uint64_t destamount)
{
    hist = _update_tradehistory(hist,srcasset,0,srcamount);
    hist = _update_tradehistory(hist,destasset,destamount,0);
    return(hist);
}

cJSON *_tradehistory_json(struct tradehistory *asset)
{
    cJSON *json = cJSON_CreateObject();
    char numstr[64];
    sprintf(numstr,"%llu",(long long)asset->assetid), cJSON_AddItemToObject(json,"assetid",cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",dstr(asset->purchased)), cJSON_AddItemToObject(json,"purchased",cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",dstr(asset->sold)), cJSON_AddItemToObject(json,"sold",cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",dstr(asset->purchased) - dstr(asset->sold)), cJSON_AddItemToObject(json,"net",cJSON_CreateString(numstr));
    return(json);
}

cJSON *tradehistory_json(struct tradehistory *hist,cJSON *array)
{
    int32_t i; char assetname[64],numstr[64]; cJSON *assets,*netpos,*item,*json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"rawtrades",array);
    assets = cJSON_CreateArray();
    netpos = cJSON_CreateArray();
    for (i=0; hist[i].assetid!=0; i++)
    {
        cJSON_AddItemToArray(assets,_tradehistory_json(&hist[i]));
        item = cJSON_CreateObject();
        get_assetname(assetname,hist[i].assetid);
        cJSON_AddItemToObject(item,"asset",cJSON_CreateString(assetname));
        sprintf(numstr,"%.8f",dstr(hist[i].purchased) - dstr(hist[i].sold)), cJSON_AddItemToObject(item,"net",cJSON_CreateString(numstr));
        cJSON_AddItemToArray(netpos,item);
    }
    cJSON_AddItemToObject(json,"assets",assets);
    cJSON_AddItemToObject(json,"netpositions",netpos);
    return(json);
}

cJSON *tabulate_trade_history(uint64_t mynxt64bits,cJSON *array)
{
    int32_t i,n;
    cJSON *item;
    long balancing;
    struct tradehistory *hist = 0;
    uint64_t src64bits,srcamount,srcasset,dest64bits,destamount,destasset,jump64bits,jumpamount,jumpasset;
    //{"requestType":"processjumptrade","NXT":"5277534112615305538","assetA":"5527630","amountA":"6700000000","other":"1510821971811852351","assetB":"12982485703607823902","amountB":"100000000","feeA":"250000000","balancing":0,"feeAtxid":"1234468909119892020","triggerhash":"34ea5aaeeeb62111a825a94c366b4ae3d12bb73f9a3413a27d1b480f6029a73c"}
    if ( array != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
    {
        for (i=0; i<n; i++)
        {
            item = cJSON_GetArrayItem(array,i);
            src64bits = get_API_nxt64bits(cJSON_GetObjectItem(item,"NXT"));
            srcamount = get_API_nxt64bits(cJSON_GetObjectItem(item,"amountA"));
            srcasset = get_API_nxt64bits(cJSON_GetObjectItem(item,"assetA"));
            dest64bits = get_API_nxt64bits(cJSON_GetObjectItem(item,"other"));
            destamount = get_API_nxt64bits(cJSON_GetObjectItem(item,"amountB"));
            destasset = get_API_nxt64bits(cJSON_GetObjectItem(item,"assetB"));
            jump64bits = get_API_nxt64bits(cJSON_GetObjectItem(item,"jumper"));
            jumpamount = get_API_nxt64bits(cJSON_GetObjectItem(item,"jumpasset"));
            jumpasset = get_API_nxt64bits(cJSON_GetObjectItem(item,"jumpamount"));
            balancing = (long)get_API_int(cJSON_GetObjectItem(item,"balancing"),0);
            if ( src64bits != 0 && srcamount != 0 && srcasset != 0 && dest64bits != 0 && destamount != 0 && destasset != 0 )
            {
                if ( src64bits == mynxt64bits )
                    hist = update_tradehistory(hist,srcasset,srcamount,destasset,destamount);
                else if ( dest64bits == mynxt64bits )
                    hist = update_tradehistory(hist,destasset,destamount,srcasset,srcamount);
                else if ( jump64bits == mynxt64bits )
                    continue;
                else printf("illegal tabulate_trade_entry %llu: (%llu -> %llu) via %llu\n",(long long)mynxt64bits,(long long)src64bits,(long long)dest64bits,(long long)jump64bits);
            } else printf("illegal tabulate_trade_entry %llu: %llu %llu %llu || %llu %llu %llu\n",(long long)mynxt64bits,(long long)src64bits,(long long)srcamount,(long long)srcasset,(long long)dest64bits,(long long)destamount,(long long)destasset);
        }
    }
    if ( hist != 0 )
    {
        array = tradehistory_json(hist,array);
        free(hist);
    }
    return(array);
}

cJSON *get_tradehistory(char *refNXTaddr,uint32_t timestamp)
{
    char cmdstr[1024],NXTaddr[64],receiverstr[MAX_JSON_FIELD],message[MAX_JSON_FIELD],newtriggerhash[MAX_JSON_FIELD],triggerhash[MAX_JSON_FIELD],*jsonstr;
    cJSON *json,*array,*txobj,*msgobj,*attachment,*retjson = 0,*histarray = 0;
    int32_t i,j,n,m,duplicates = 0;
    uint64_t senderbits;
    if ( timestamp == 0 )
        timestamp = 38785003;
    sprintf(cmdstr,"requestType=getBlockchainTransactions&account=%s&timestamp=%u&withMessage=true",refNXTaddr,timestamp);
    if ( (jsonstr= issue_NXTPOST(cmdstr)) != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"transactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    txobj = cJSON_GetArrayItem(array,i);
                    copy_cJSON(receiverstr,cJSON_GetObjectItem(txobj,"recipient"));
                    if ( (senderbits = get_API_nxt64bits(cJSON_GetObjectItem(txobj,"sender"))) != 0 )
                    {
                        expand_nxt64bits(NXTaddr,senderbits);
                        if ( refNXTaddr != 0 && strcmp(NXTaddr,refNXTaddr) == 0 )
                        {
                            if ( (attachment= cJSON_GetObjectItem(txobj,"attachment")) != 0 && (msgobj= cJSON_GetObjectItem(attachment,"message")) != 0 )
                            {
                                copy_cJSON(message,msgobj);
                                //printf("(%s) -> ",message);
                                unstringify(message);
                                if ( (msgobj= cJSON_Parse(message)) != 0 )
                                {
                                    //printf("(%s)\n",message);
                                    if ( histarray == 0 )
                                        histarray = cJSON_CreateArray(), j = m = 0;
                                    else
                                    {
                                        copy_cJSON(newtriggerhash,cJSON_GetObjectItem(msgobj,"triggerhash"));
                                        m = cJSON_GetArraySize(histarray);
                                        for (j=0; j<m; j++)
                                        {
                                            copy_cJSON(triggerhash,cJSON_GetObjectItem(cJSON_GetArrayItem(histarray,j),"triggerhash"));
                                            if ( strcmp(triggerhash,newtriggerhash) == 0 )
                                            {
                                                duplicates++;
                                                break;
                                            }
                                        }
                                    }
                                    if ( j == m )
                                        cJSON_AddItemToArray(histarray,msgobj);
                                } else printf("parse error on.(%s)\n",message);
                            }
                        }
                    }
                }
            }
            free_json(json);
        }
        free(jsonstr);
    }
    if ( histarray != 0 )
        retjson = tabulate_trade_history(calc_nxt64bits(refNXTaddr),histarray);
    printf("duplicates.%d\n",duplicates);
    return(retjson);
}

char *InstantDEX_tradehistory()
{
    cJSON *history,*json;
    json = cJSON_CreateObject();
    history = get_tradehistory(SUPERNET.NXTADDR,0);
    if ( history != 0 )
        cJSON_AddItemToObject(json,"tradehistory",history);
    return(jprint(json,1));
}

int32_t substr128(char *dest,char *src)
{
    char zeroes[129],*match; int32_t i;
    for (i=0; i<128; i++)
        zeroes[i] = '0';
    zeroes[i] = 0;
    strcpy(dest,src);
    if ( (match= strstr(dest,zeroes)) != 0 )
    {
        strcpy(match,"Z");
        for (i=0; match[128+i]!=0; i++)
            match[i+1] = match[128+i];
        match[i+1] = 0;
    }
    //printf("substr128.(%s) -> (%s)\n",src,dest);
    return(0);
}

uint64_t gen_NXTtx(struct NXTtx *tx,uint64_t dest64bits,uint64_t assetidbits,uint64_t qty,uint64_t orderid,uint64_t quoteid,int32_t deadline,char *reftx,char *phaselink,uint32_t finishheight)
{
    char secret[8192],cmd[16384],destNXTaddr[64],assetidstr[64],hexstr[64],*retstr; cJSON *json; uint64_t phasecost = 0;
    expand_nxt64bits(destNXTaddr,dest64bits);
    memset(tx,0,sizeof(*tx));
    if ( phaselink!= 0 && phaselink[0] != 0 && finishheight <= _get_NXTheight(0) )
    {
        printf("finish height.%u must be in the future.%u\n",finishheight,_get_NXTheight(0));
        return(0);
    }
    if ( phaselink != 0 )
        phasecost = MIN_NQTFEE;
    cmd[0] = 0;
    if ( assetidbits == NXT_ASSETID )
        sprintf(cmd,"requestType=sendMoney&amountNQT=%lld",(long long)qty);
    else
    {
        expand_nxt64bits(assetidstr,assetidbits);
        //if ( utx->type == 2 && utx->subtype == 1 )
        sprintf(cmd,"requestType=transferAsset&asset=%s&quantityQNT=%lld",assetidstr,(long long)qty);
        /*else if ( utx->type == 5 && utx->subtype == 3 )
         sprintf(cmd,"requestType=transferCurrency&currency=%s&units=%lld",assetidstr,(long long)(utx->U.quantityQNT));
         else
         {
         printf("unsupported type.%d subtype.%d\n",utx->type,utx->subtype);
         return(0);
         }*/
    }
    if ( quoteid != 0 )
    {
        init_hexbytes_noT(hexstr,(void *)&orderid,sizeof(orderid));
        init_hexbytes_noT(hexstr+16,(void *)&quoteid,sizeof(quoteid));
        sprintf(cmd+strlen(cmd),"&messageIsText=true&message=%s",hexstr);
    }
    if ( cmd[0] != 0 )
    {
        escape_code(secret,SUPERNET.NXTACCTSECRET);
        sprintf(cmd+strlen(cmd),"&deadline=%u&feeNQT=%lld&secretPhrase=%s&recipient=%s&broadcast=false",deadline,(long long)MIN_NQTFEE+phasecost,secret,destNXTaddr);
        if ( reftx != 0 && reftx[0] != 0 )
            sprintf(cmd+strlen(cmd),"&referencedTransactionFullHash=%s",reftx);
        if ( phaselink != 0 && phaselink[0] != 0 )
            sprintf(cmd+strlen(cmd),"&phased=true&phasingFinishHeight=%u&phasingVotingModel=4&phasingQuorum=1&phasingLinkedFullHash=%s",finishheight,phaselink);
//printf("generated cmd.(%s)\n",cmd);
        if ( (retstr= issue_NXTPOST(cmd)) != 0 )
        {
//printf("(%s)\n",retstr);
            if ( (json= cJSON_Parse(retstr)) != 0 )
            {
                if ( extract_cJSON_str(tx->txbytes,MAX_JSON_FIELD,json,"transactionBytes") > 0 &&
                    extract_cJSON_str(tx->utxbytes,MAX_JSON_FIELD,json,"unsignedTransactionBytes") > 0 &&
                    extract_cJSON_str(tx->fullhash,MAX_JSON_FIELD,json,"fullHash") > 0 &&
                    extract_cJSON_str(tx->sighash,MAX_JSON_FIELD,json,"signatureHash") > 0 )
                {
                    tx->txid = j64bits(json,"transaction");
                    substr128(tx->utxbytes2,tx->utxbytes);
                }
                free_json(json);
            }
            free(retstr);
        }
    }
    return(tx->txid);
}

uint64_t InstantDEX_swapstr(uint64_t *txidp,char *triggertx,char *txbytes,char *swapstr,uint64_t orderid,struct prices777_order *order,char *triggerhash,char *phaselink,int32_t finishheight)
{
    struct NXTtx fee,sendtx; uint64_t otherqty = 0,otherassetbits = 0,assetidbits = 0,qty = 0;
    if ( finishheight != 0 )
        finishheight += _get_NXTheight(0);
    swapstr[0] = triggertx[0] = txbytes[0] = 0;
    *txidp = 0;
    gen_NXTtx(&fee,calc_nxt64bits(INSTANTDEX_ACCT),NXT_ASSETID,INSTANTDEX_FEE,orderid,order->s.quoteid,INSTANTDEX_TRIGGERDEADLINE,triggerhash,0,0);
    strcpy(triggertx,fee.txbytes);
    if ( order->s.baseamount < 0 )
        assetidbits = order->s.baseid, qty = -order->s.baseamount, otherassetbits = order->s.relid, otherqty = order->s.relamount;
    else if ( order->s.relamount < 0 )
        assetidbits = order->s.baseid, qty = -order->s.relamount, otherassetbits = order->s.baseid, otherqty = order->s.baseamount;
    if ( assetidbits != 0 && qty != 0 )
    {
        if ( triggerhash == 0 || triggerhash[0] == 0 )
            triggerhash = fee.fullhash;
        gen_NXTtx(&sendtx,order->s.offerNXT,assetidbits,qty,orderid,order->s.quoteid,INSTANTDEX_TRIGGERDEADLINE,fee.fullhash,phaselink,finishheight);
        *txidp = sendtx.txid;
        strcpy(txbytes,sendtx.txbytes);
        sprintf(swapstr,",\"F\":\"%u\",\"T\":\"%s\",\"FH\":\"%s\",\"U\":\"%s\",\"S\":\"%s\",\"a\":\"%llu\",\"q\":\"%llu\"}",finishheight,fee.fullhash,sendtx.fullhash,sendtx.utxbytes2,sendtx.sighash,(long long)otherassetbits,(long long)otherqty);
    }
    return(fee.txid);
}

uint64_t prices777_swapbuf(uint64_t *txidp,char *triggertx,char *txbytes,char *swapbuf,struct prices777 *prices,struct prices777_order *order,uint64_t orderid)
{
    char swapstr[4096]; uint64_t txid = 0;
    *txidp = 0;
    sprintf(swapbuf,"{\"orderid\":\"%llu\",\"quoteid\":\"%llu\",\"offerNXT\":\"%s\",\"plugin\":\"relay\",\"destplugin\":\"InstantDEX\",\"method\":\"busdata\",\"submethod\":\"%s\",\"exchange\":\"%s\",\"base\":\"%s\",\"rel\":\"%s\",\"baseid\":\"%llu\",\"relid\":\"%llu\",\"baseqty\":\"%lld\",\"relqty\":\"%lld\"}",(long long)orderid,(long long)order->s.quoteid,SUPERNET.NXTADDR,order->wt > 0. ? "buy" : (order->wt < 0. ? "sell" : "swap"),prices->exchange,prices->base,prices->rel,(long long)order->s.baseid,(long long)order->s.relid,(long long)order->s.baseamount,(long long)order->s.relamount);
    if ( order->s.price > SMALLVAL )
        sprintf(swapbuf + strlen(swapbuf) - 1,",\"price\":%.8f,\"volume\":%.8f}",order->s.price,order->s.vol);
    txid = InstantDEX_swapstr(txidp,triggertx,txbytes,swapstr,orderid,order,0,0,5);
    strcpy(swapbuf+strlen(swapbuf)-1,swapstr);
    //printf("swapbuf.(%s)\n",swapbuf);
    return(txid);
}

char *prices777_trade(struct prices777 *prices,int32_t dir,double price,double volume,struct InstantDEX_quote *iQ,struct prices777_order *order,uint64_t orderid)
{
    struct InstantDEX_quote _iQ;
    char *retstr; struct exchange_info *exchange; struct pending_trade *pend; uint32_t nonce; char *str,swapbuf[8192],triggertx[4096],txbytes[4096];
    pend = calloc(1,sizeof(*pend));
    pend->prices = prices, pend->dir = dir, pend->price = price, pend->volume = volume, pend->orderid = orderid;
    if ( iQ == 0 && order == 0 )
    {
        printf("prices777_trade: need to have either iQ or order\n");
        return(clonestr("{\"error\":\"need to have either iQ or order\"}\n"));
    }
    else if ( iQ == 0 && (iQ= find_iQ(order->s.quoteid)) == 0 )
    {
        iQ = &_iQ;
        memset(&_iQ,0,sizeof(_iQ));
        iQ->s = order->s;
        iQ->exchangeid = prices->exchangeid;
        iQ->s.timestamp = (uint32_t)time(NULL);
        iQ = create_iQ(iQ);
        //printf("prices777_trade: need to have iQ \n");
        //return(clonestr("{\"error\":\"need to have iQ\"}\n"));
    } else iQ = create_iQ(iQ);
    iQ->s.pending = 1;
    pend->quoteid = iQ->s.quoteid;
    if ( order != 0 )
        pend->order = *order;
    pend->timestamp = (uint32_t)time(NULL);
    queue_enqueue("PendingQ",&Pending_offersQ.pingpong[0],&pend->DL);
    if ( strcmp(prices->exchange,INSTANTDEX_NAME) == 0 )
    {
        if ( order == 0 )
        {
            printf("must call prices777_trade with swapbuf or order to do InstantDEX swap trade\n");
            return(clonestr("{\"error\":\"need to specify swapbuf\"}\n"));
        }
        pend->triggertxid = prices777_swapbuf(&pend->txid,triggertx,txbytes,swapbuf,prices,order,orderid);
        if ( triggertx[0] != 0 )
            pend->triggertx = clonestr(triggertx);
        if ( txbytes != 0 )
            pend->txbytes = clonestr(txbytes);
        pend->tradesjson = cJSON_Parse(swapbuf);
        pend->type = 'T';
        iQ->s.swap = 1;
        if ( (str= busdata_sync(&nonce,clonestr(swapbuf),"allnodes",0)) != 0 )
            free(str);
        return(clonestr(swapbuf));
    }
    else if ( strcmp(prices->exchange,"nxtae") == 0 )
    {
        pend->type = 'N';
        retstr = fill_nxtae(&pend->txid,SUPERNET.my64bits,dir,price,volume,prices->baseid,prices->relid);
    }
    else if ( (exchange= find_exchange(0,prices->exchange)) != 0 )
    {
        if ( exchange->trade != 0 )
        {
            printf(" issue dir.%d %s/%s price %f vol %f -> %s\n",dir,prices->base,prices->rel,price,volume,prices->exchange);
            (*exchange->trade)(&retstr,exchange,prices->base,prices->rel,dir,price,volume);
            return(retstr);
        } else return(clonestr("{\"error\":\"no trade function for exchange\"}\n"));
    }
    return(clonestr("{\"error\":\"exchange not active, check SuperNET.conf exchanges array\"}\n"));
}

char *swap_func(int32_t localaccess,int32_t valid,char *sender,cJSON *origjson,char *origargstr)
{
    char offerNXT[MAX_JSON_FIELD],UTX[MAX_JSON_FIELD],calchash[256],*triggerhash,*utx,*sighash,*jsonstr,*parsed,*fullhash,*cmpstr;
    cJSON *json,*txobj; uint64_t otherbits,otherqty,quoteid,orderid,recvasset; int64_t recvqty; uint32_t i,j,finishheight; struct InstantDEX_quote *iQ,_iQ;
    copy_cJSON(offerNXT,jobj(origjson,"offerNXT"));
    //printf("got (%s)\n",origargstr);
    if ( 1 ) //strcmp(SUPERNET.NXTADDR,offerNXT) != 0 )
    {
        orderid = j64bits(origjson,"orderid");
        quoteid = j64bits(origjson,"quoteid");
        if ( (iQ= find_iQ(quoteid)) == 0 )
        {
            printf("cant find quoteid.%llu\n",(long long)quoteid);
            iQ = &_iQ, memset(iQ,0,sizeof(*iQ));
            //return(clonestr("{\"error\":\"cant find quoteid\"}"));
        }
        if ( iQ->s.responded != 0 )
        {
            fprintf(stderr,"already responded quoteid.%llu\n",(long long)iQ->s.quoteid);
            return(0);
        }
        //sprintf(swapstr,",\"F\":\"%u\",\"T\":\"%s\",\"FH\":\"%s\",\"U\":\"%s\",\"S\":\"%s\",\"a\":\"%llu\",\"q\":\"%llu\"}",finishheight,fee.fullhash,sendtx.fullhash,sendtx.utxbytes,sendtx.sighash,(long long)otherassetbits,(long long)otherqty);
        otherbits = j64bits(origjson,"a");
        otherqty = j64bits(origjson,"q");
        triggerhash = jstr(origjson,"T");
        fullhash = jstr(origjson,"FH");
        utx = jstr(origjson,"U");
        for (i=0; utx[i]!=0; i++)
            if ( utx[i] == 'Z' )
            {
                memcpy(UTX,utx,i);
                for (j=0; j<128; j++)
                    UTX[i+j] = '0';
                UTX[i+j] = 0;
                strcat(UTX,utx+i+1);
                break;
            }
        sighash = jstr(origjson,"S");
        finishheight = juint(origjson,"F");
        //printf("utx.(%s) -> UTX.(%s) sighash.(%s)\n",utx,UTX,sighash);
        if ( (jsonstr= issue_calculateFullHash(UTX,sighash)) != 0 )
        {
            //printf("calculated.(%s)\n",jsonstr);
            if ( (json= cJSON_Parse(jsonstr)) != 0 )
            {
                copy_cJSON(calchash,jobj(json,"fullHash"));
                if ( strcmp(calchash,fullhash) == 0 )
                {
                    if ( (parsed= issue_parseTransaction(UTX)) != 0 )
                    {
                        _stripwhite(parsed,' ');
                        //printf("iQ (%llu/%llu) otherbits.%llu qty %llu PARSED OFFER.(%s) triggerhash.(%s) (%s) offer sender.%s\n",(long long)iQ->s.baseid,(long long)iQ->s.relid,(long long)otherbits,(long long)otherqty,parsed,fullhash,calchash,sender);
                        if ( (txobj= cJSON_Parse(parsed)) != 0 )
                        {
                            if ( (cmpstr= jstr(txobj,"referencedTransactionFullHash")) != 0 && strcmp(cmpstr,triggerhash) == 0 )
                            {
                                // https://nxtforum.org/nrs-releases/nrs-v1-5-15/msg191715/#msg191715
                                struct NXTtx fee,responsetx; int32_t errcode,errcode2; cJSON *retjson; char *txstr=0,*txstr2=0; struct pending_trade *pend;
                                if ( iQ->s.isask == 0 )
                                    recvasset = iQ->s.baseid, recvqty = -iQ->s.baseamount;
                                else recvasset = iQ->s.relid, recvqty = -iQ->s.relamount;
                                if ( InstantDEX_verify(SUPERNET.my64bits,otherbits,otherqty,txobj,recvasset,recvqty) == 0 )
                                {
                                    gen_NXTtx(&fee,calc_nxt64bits(INSTANTDEX_ACCT),NXT_ASSETID,INSTANTDEX_FEE,orderid,quoteid,INSTANTDEX_TRIGGERDEADLINE,fullhash,0,0);
                                    gen_NXTtx(&responsetx,calc_nxt64bits(offerNXT),otherbits,otherqty,orderid,quoteid,INSTANTDEX_TRIGGERDEADLINE,triggerhash,fullhash,finishheight);
                                    if ( (fee.txid= issue_broadcastTransaction(&errcode,&txstr,fee.txbytes,SUPERNET.NXTACCTSECRET)) != 0 )
                                    {
                                        if ( (responsetx.txid= issue_broadcastTransaction(&errcode2,&txstr2,responsetx.txbytes,SUPERNET.NXTACCTSECRET)) != 0 )
                                        {
                                            pend = calloc(1,sizeof(*pend));
                                            pend->orderid = orderid, pend->quoteid = quoteid;
                                            iQ->s.responded = 1;
                                            iQ->s.pending = 1;
                                            pend->triggertx = clonestr(triggerhash);
                                            pend->txbytes = clonestr(fullhash);
                                            pend->type = 'R';
                                            pend->quoteid = iQ->s.quoteid;
                                            retjson = cJSON_CreateObject();
                                            jadd(retjson,"fee",cJSON_Parse(txstr));
                                            jadd(retjson,"responsetx",cJSON_Parse(txstr2));
                                            pend->tradesjson = retjson;
                                            pend->timestamp = (uint32_t)time(NULL);
                                            queue_enqueue("PendingQ",&Pending_offersQ.pingpong[0],&pend->DL);
                                            printf("BROADCAST fee.txid %llu and %llu\n",(long long)fee.txid,(long long)responsetx.txid);
                                        } else printf("error.%d broadcasting responsetx.(%s) %s\n",errcode2,responsetx.txbytes,txstr2);
                                    } else printf("error.%d broadcasting feetx.(%s) %s\n",errcode,fee.txbytes,txstr);
                                } else printf("(%s) didnt validate against quoteid.%llu\n",parsed,(long long)quoteid);
                                if ( txstr != 0 )
                                    free(txstr);
                                if ( txstr2 != 0 )
                                    free(txstr2);
                            }
                            free_json(txobj);
                        }
                    }
                } else printf("mismatch (%s) != (%s)\n",calchash,fullhash);
                free_json(json);
            }
            free(jsonstr);
        } else printf("calchash.(%s)\n",jsonstr);
    } else fprintf(stderr,"got my swap from network (%s)\n",origargstr);
    return(0);
}

void free_pending(struct pending_trade *pend)
{
    struct InstantDEX_quote *iQ;
    if ( (iQ= find_iQ(pend->quoteid)) != 0 )
        iQ->s.closed = 1;
    else printf("free_pending: cant find pending tx for %llu\n",(long long)pend->quoteid);
    if ( pend->triggertx != 0 )
        free(pend->triggertx);
    if ( pend->txbytes != 0 )
        free(pend->txbytes);
    if ( pend->tradesjson != 0 )
        free_json(pend->tradesjson);
    free(pend);
}

/*void pending_completed(uint64_t orderid,uint64_t quoteid)
{
    int32_t iter; struct pending_trade *pend;
    for (iter=0; iter<2; iter++)
    {
        while ( (pend= queue_dequeue(&Pending_offersQ.pingpong[iter],0)) != 0 )
        {
            if ( pend->orderid == orderid && pend->quoteid == quoteid )
            {
                printf("COMPLETED %llu/%llu %d %f %f\n",(long long)pend->orderid,(long long)pend->quoteid,pend->dir,pend->price,pend->volume);
                free_pending(pend);
            } else queue_enqueue("requeue",&Pending_offersQ.pingpong[iter ^ 1],&pend->DL);
        }
    }
}*/

int32_t complete_swap(struct InstantDEX_quote *iQ,uint64_t orderid,uint64_t quoteid)
{
    int32_t errcode,errcode2; char *txstr,*txstr2; int32_t iter; struct pending_trade *pend;
    for (iter=0; iter<2; iter++)
    {
        while ( (pend= queue_dequeue(&Pending_offersQ.pingpong[iter],0)) != 0 )
        {
            if ( issue_broadcastTransaction(&errcode2,&txstr2,pend->txbytes,SUPERNET.NXTACCTSECRET) == pend->txid && errcode2 == 0 )
            {
                if ( issue_broadcastTransaction(&errcode,&txstr,pend->triggertx,SUPERNET.NXTACCTSECRET) == pend->triggertxid && errcode == 0 )
                {
                    iQ->s.matched = 1;
                    iQ->s.closed = 1;
                    printf("COMPLETED %llu/%llu %d %f %f\n",(long long)pend->orderid,(long long)pend->quoteid,pend->dir,pend->price,pend->volume);
                    free_pending(pend);
                    return(1);
                }
            }
            queue_enqueue("requeue",&Pending_offersQ.pingpong[iter ^ 1],&pend->DL);
        }
    }
    return(-1);
}

int32_t match_unconfirmed(char *sender,char *hexstr,cJSON *txobj)
{
    uint64_t orderid,quoteid,recvasset,sendasset; int64_t recvqty,sendqty; struct InstantDEX_quote *iQ;
    decode_hex((void *)&orderid,sizeof(orderid),hexstr);
    decode_hex((void *)&quoteid,sizeof(quoteid),hexstr+16);
    if ( (iQ= find_iQ(quoteid)) != 0 && iQ->s.closed == 0 && iQ->s.pending != 0 && (iQ->s.responded == 0 || iQ->s.feepaid == 0) )
    {
        printf("match unconfirmed %llu/%llu feepaid.%d responded.%d sender.(%s) me.(%s)\n",(long long)orderid,(long long)quoteid,iQ->s.feepaid,iQ->s.responded,sender,SUPERNET.NXTADDR);
        if ( iQ->s.swap != 0 )
        {
            if ( verify_NXTtx(txobj,NXT_ASSETID,INSTANTDEX_FEE,calc_nxt64bits(INSTANTDEX_ACCT)) == 0 )
            {
                iQ->s.feepaid = 1;
                printf("FEE DETECTED\n");
                complete_swap(iQ,orderid,quoteid);
            }
            else
            {
                if ( iQ->s.isask != 0 )
                {
                    sendasset = iQ->s.relid, sendqty = -iQ->s.relamount;
                    recvasset = iQ->s.baseid, recvqty = iQ->s.baseamount;
                }
                else
                {
                    sendasset = iQ->s.baseid, sendqty = -iQ->s.baseamount;
                    recvasset = iQ->s.relid, recvqty = iQ->s.relamount;
                }
                printf("iQ: %llu/%llu %lld/%lld | recv %llu %lld offerNXT.%llu\n",(long long)iQ->s.baseid,(long long)iQ->s.relid,(long long)iQ->s.baseamount,(long long)iQ->s.relamount,(long long)recvasset,(long long)recvqty,(long long)iQ->s.offerNXT);
                if ( InstantDEX_verify(SUPERNET.my64bits,sendasset,sendqty,txobj,recvasset,recvqty) == 0 )
                {
                    iQ->s.responded = 1;
                    printf("RESPONSE DETECTED\n");
                    complete_swap(iQ,orderid,quoteid);
                } else printf("mismatched response\n");
            }
        }
    }
    return(-1);
}

int offer_checkitem(struct pending_trade *pend,cJSON *item)
{
    uint64_t quoteid; struct InstantDEX_quote *iQ;
    if ( (quoteid= j64bits(item,"quoteid")) != 0 && (iQ= find_iQ(quoteid)) != 0 && iQ->s.closed != 0 )
        return(0);
    return(-1);
}

char *offer_statemachine(struct pending_trade *pend)
{
    int32_t i,n,pending = 0; cJSON *array,*item;
    if ( pend->type == 'R' )
    {
        // wait for alice tx
    }
    else if ( pend->type == 'T' )
    {
        // already handled in unconf
    }
    else if ( pend->type == 'S' )
    {
        if ( pend->tradesjson != 0 && (array= jarray(&n,pend->tradesjson,"trades")) != 0 )
        {
            for (i=0; i<n; i++)
            {
                item = jitem(array,i);
                if ( offer_checkitem(pend,item) < 0 )
                    pending++;
            }
            if ( pending == 0 )
                return(clonestr("{\"status\":\"success\",\"trade sequence completed\"}"));
        }
    }
    else
    {
        // exchange specific check status on trade
    }
    return(0);
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

cJSON *InstantDEX_tradejson(struct prices777_order *order,int32_t dotrade,uint64_t orderid)
{
    char swapbuf[8192],buf[8192],triggertx[4096],txbytes[4096],*retstr,*exchange; uint64_t txid,qty,avail,priceNQT; struct prices777 *prices; cJSON *json = 0;
    if ( (prices= order->source) != 0 )
    {
        exchange = prices->exchange;
        swapbuf[0] = 0;
        if ( strcmp(exchange,INSTANTDEX_NAME) == 0 )
            prices777_swapbuf(&txid,triggertx,txbytes,swapbuf,prices,order,orderid);
        if ( dotrade == 0 )
        {
            if ( strcmp(exchange,INSTANTDEX_NAME) != 0 )
            {
                sprintf(buf,"{\"orderid\":\"%llu\",\"trade\":\"%s\",\"exchange\":\"%s\",\"base\":\"%s\",\"rel\":\"%s\",\"baseid\":\"%llu\",\"relid\":\"%llu\",\"price\":%.8f,\"volume\":%.8f}",(long long)orderid,order->wt > 0. ? "buy" : "sell",exchange,prices->base,prices->rel,(long long)prices->baseid,(long long)prices->relid,order->s.price,order->s.vol);
                if ( strcmp(exchange,"nxtae") == 0 )
                {
                    qty = calc_asset_qty(&avail,&priceNQT,SUPERNET.NXTADDR,0,prices->baseid,order->s.price,order->s.vol);
                    sprintf(buf+strlen(buf)-1,",\"priceNQT\":\"%llu\",\"quantityQNT\":\"%llu\",\"avail\":\"%llu\"}",(long long)priceNQT,(long long)qty,(long long)avail);
                    if ( qty == 0 )
                        sprintf(buf+strlen(buf)-1,",\"error\":\"insufficient balance\"}");
                }
                return(cJSON_Parse(buf));
            }
            else return(cJSON_Parse(swapbuf));
        }
        retstr = prices777_trade(prices,order->wt,order->s.price,order->s.vol,0,order,orderid);
        if ( retstr != 0 )
        {
            json = cJSON_Parse(retstr);
            free(retstr);
        }
    }
    return(json);
}

char *InstantDEX_dotrades(cJSON *json,struct prices777_order *trades,int32_t numtrades,int32_t dotrade)
{
    char exchangestr[MAX_JSON_FIELD],gui[MAX_JSON_FIELD],name[MAX_JSON_FIELD],base[MAX_JSON_FIELD],rel[MAX_JSON_FIELD]; struct InstantDEX_quote iQ;
    cJSON *retjson,*retarray,*item; int32_t i; struct pending_trade *pend;
    bidask_parse(exchangestr,name,base,rel,gui,&iQ,json);
    retjson = cJSON_CreateObject(), retarray = cJSON_CreateArray();
    for (i=0; i<numtrades; i++)
    {
        item = InstantDEX_tradejson(&trades[i],dotrade,iQ.s.quoteid);
        //printf("GOT%d.(%s)\n",i,jprint(item,0));
        jaddi(retarray,item);
    }
    jadd(retjson,"traderesults",retarray);
    if ( dotrade != 0 )
    {
        pend = calloc(1,sizeof(*pend));
        pend->dir = iQ.s.isask == 0 ? 1 : -1, pend->price = iQ.s.price, pend->volume = iQ.s.vol, pend->orderid = iQ.s.quoteid;
        pend->tradesjson = json;
        pend->type = 'S';
        pend->timestamp = (uint32_t)time(NULL);
        queue_enqueue("PendingQ",&Pending_offersQ.pingpong[0],&pend->DL);
    }
    return(jprint(retjson,1));
}

char *InstantDEX_tradesequence(cJSON *json)
{
    //"trades":[[{"basket":"bid","rootwt":-1,"groupwt":1,"wt":-1,"price":40000,"volume":0.00015000,"group":0,"trade":"buy","exchange":"nxtae","asset":"17554243582654188572","base":"BTC","rel":"NXT","orderid":"3545444239044461477","orderprice":40000,"ordervolume":0.00015000}], [{"basket":"bid","rootwt":-1,"groupwt":1,"wt":1,"price":0.00376903,"volume":1297.41480000,"group":10,"trade":"sell","exchange":"coinbase","name":"BTC/USD","base":"BTC","rel":"USD","orderid":"1","orderprice":265.32000000,"ordervolume":4.89000000}]]}
    cJSON *array,*item; int32_t i,n,dir; char *tradestr,*exchangestr,base[512],rel[512],name[512]; struct prices777_order trades[256],*order;
    uint64_t orderid,assetid,currency,baseid,relid,quoteid; int64_t sendbase,recvbase,sendrel,recvrel;
    double orderprice,ordervolume; struct prices777 *prices; uint32_t timestamp;
    memset(trades,0,sizeof(trades));
    if ( (array= jarray(&n,json,"trades")) != 0 )
    {
        if ( n > sizeof(trades)/sizeof(*trades) )
            return(clonestr("{\"error\":\"exceeded max trades possible in a tradesequence\"}"));
        timestamp = (uint32_t)time(NULL);
        for (i=0; i<n; i++)
        {
            order = &trades[i];
            item = jitem(array,i);
            tradestr = jstr(item,"trade"), exchangestr = jstr(item,"exchange");
            copy_cJSON(base,jobj(item,"base")), copy_cJSON(rel,jobj(item,"rel")), copy_cJSON(name,jobj(item,"name"));
            orderid = j64bits(item,"orderid"), quoteid = j64bits(item,"quoteid");
            if ( orderid == 0 )
                orderid = quoteid;
            if ( quoteid == 0 )
                quoteid = orderid;
            order->id = orderid, order->s.quoteid = quoteid;
            assetid = j64bits(item,"asset"), currency = j64bits(item,"currency");
            baseid = j64bits(item,"baseid"), relid = j64bits(item,"relid");
            sendbase = j64bits(item,"sendbase"), recvbase = j64bits(item,"recvbase");
            sendrel = j64bits(item,"sendrel"), recvrel = j64bits(item,"recvrel");
            order->s.baseamount = (recvbase - sendbase);
            order->s.relamount = (recvrel - sendrel);
            orderprice = jdouble(item,"orderprice"), ordervolume = jdouble(item,"ordervolume");
            order->s.timestamp = juint(item,"timestamp");
            order->s.duration = juint(item,"duration");
            order->s.minperc = juint(item,"minperc");
            order->s.baseid = baseid;
            order->s.relid = relid;
            //printf("ITEM.(%s)\n",jprint(item,0));
            if ( tradestr != 0 )
            {
                if ( strcmp(tradestr,"buy") == 0 )
                    dir = 1;
                else if ( strcmp(tradestr,"sell") == 0 )
                    dir = -1;
                else if ( strcmp(tradestr,"swap") == 0 )
                    dir = 0;
                else return(clonestr("{\"error\":\"invalid trade direction\"}"));
                if ( (prices= prices777_initpair(1,0,exchangestr,base,rel,0.,name,baseid,relid,0)) != 0 )
                {
                    order->source = prices;
                    order->s.offerNXT = j64bits(item,"offerNXT");
                    order->wt = dir, order->s.price = orderprice, order->s.vol = ordervolume;
                    printf("item[%d] dir.%d baseid.%llu relid.%llu sendbase.%llu recvbase.%llu sendrel.%llu recvrel.%llu | baseqty.%lld relqty.%lld\n",i,dir,(long long)order->s.baseid,(long long)order->s.relid,(long long)sendbase,(long long)recvbase,(long long)sendrel,(long long)recvrel,(long long)order->s.baseamount,(long long)order->s.relamount);
                } else return(clonestr("{\"error\":\"invalid exchange or contract pair\"}"));
            } else return(clonestr("{\"error\":\"no trade specified\"}"));
        }
        return(InstantDEX_dotrades(json,trades,n,juint(json,"dotrade")));
    }
    printf("error parsing.(%s)\n",jprint(json,0));
    return(clonestr("{\"error\":\"couldnt process trades\"}"));
}

#endif
