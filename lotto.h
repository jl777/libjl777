
//
//  lotto.h
//
//  Created by jl777 on 12/20/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef LOTTO_H
#define LOTTO_H

#define MIN_REQUIRED 100
#define EXCLUDED_ACCT "4383817337783094122"

cJSON *gen_lottotickets_json(uint64_t *bestp,uint64_t seed,int32_t numtickets,uint64_t lotto)
{
    cJSON *array = cJSON_CreateArray();
    int32_t i,dist, bestdist = 64;
    bits256 hash,tmp;
    uint64_t ticket = seed;
    char numstr[64];
    memset(&hash,0,sizeof(hash));
    hash.txid = seed;
    *bestp = 0;
    for (i=0; i<numtickets; i++)
    {
        sprintf(numstr,"%llu",(long long)ticket), cJSON_AddItemToArray(array,cJSON_CreateString(numstr));
        if ( (dist= bitweight(lotto ^ ticket)) < bestdist )
        {
            bestdist = dist;
            *bestp = ticket;
        }
        cJSON_AddItemToArray(array,cJSON_CreateNumber(dist));
        tmp = sha256_key(hash);
        ticket = tmp.txid;
        hash = tmp;
    }
    return(array);
}

uint64_t calc_netassets(uint64_t refbits,uint64_t *boughtp,uint64_t *receivedp,uint64_t *soldp,uint64_t *xferp,uint64_t buyer,char *assetidstr,uint32_t firstblock,uint64_t total)
{
    uint64_t sold,xfer,sellerbits,buyerbits,qnt,soldfactor,xferfactor,bought,received,tickets;
    char refNXTaddr[64],cmd[512],*jsonstr;
    int32_t i,n,iter;
    uint32_t blockid;
    cJSON *json,*array,*item;
    expand_nxt64bits(refNXTaddr,buyer);
    *soldp = *xferp = *boughtp = *receivedp = 0;
    if ( strcmp(EXCLUDED_ACCT,refNXTaddr) == 0 )
    {
        printf("hardcoded exclude.(%s)\n",refNXTaddr);
        return(0);
    }
    sold = xfer = bought = received = tickets = 0;
    soldfactor = 1;
    xferfactor = 0;
    for (iter=0; iter<2; iter++)
    {
        sprintf(cmd,"%s=%s&account=%s&asset=%s",_NXTSERVER,(iter & 1) ? "getAssetTransfers" : "getTrades",refNXTaddr,assetidstr);
        if ( (jsonstr= issue_NXTPOST(0,cmd)) != 0 )
        {
            if ( (json= cJSON_Parse(jsonstr)) != 0 )
            {
                if ( (array= cJSON_GetObjectItem(json,(iter & 1) ? "transfers" : "trades")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
                {
                    for (i=0; i<n; i++)
                    {
                        item = cJSON_GetArrayItem(array,i);
                        blockid = (int32_t)get_API_int(cJSON_GetObjectItem(item,"height"),0);
                        sellerbits = get_API_nxt64bits(cJSON_GetObjectItem(item,(iter & 1) ? "sender" : "seller"));
                        buyerbits = get_API_nxt64bits(cJSON_GetObjectItem(item,(iter & 1) ? "recipient" : "buyer"));
                        qnt = get_API_nxt64bits(cJSON_GetObjectItem(item,"quantityQNT"));
                        if ( blockid >= firstblock )
                        {
                             if ( sellerbits == buyer )
                            {
                                sold += (soldfactor * qnt);
                                xfer += (xferfactor * qnt);
                            }
                            else if ( buyerbits == buyer )
                            {
                                if ( sellerbits != refbits )
                                {
                                    bought += (soldfactor * qnt);
                                    received += (xferfactor * qnt);
                                } else tickets += qnt;
                            }
                        }
                        if ( 0 && (calc_nxt64bits("12298820653909015414") == sellerbits || calc_nxt64bits("12298820653909015414") == buyerbits) )
                            printf("%s\nbuyer %llu <- seller %llu: 1st %u, blockid.%u | sold.%lld xfer.%lld bought.%lld received.%lld | tickets.%lld vs total.%lld | net.%lld\n",cJSON_Print(item),(long long)buyer,(long long)sellerbits,firstblock,blockid,(long long)sold,(long long)xfer,(long long)bought,(long long)received,(long long)tickets,(long long)total,(long long)(total - (sold + xfer)));
                    }
                }
                free_json(json);
            }
            free(jsonstr);
        }
        soldfactor = 0;
        xferfactor = 1;
    }
    *soldp = sold;
    *xferp = xfer;
    *boughtp = bought;
    *receivedp = received;
    return(total - (sold + xfer));
}

void process_lotto(uint64_t refbits,double prizefund,char *assetidstr,uint64_t lotto,cJSON **jsonp,int32_t iter,uint64_t buyerbits,int64_t assetoshis,int32_t blockid,uint64_t txid)
{
    int32_t w,i,n,numwinners,num,netcount,nettickets,netplayers,netnum,bestdist = 64;
    cJSON *array,*item = 0;
    char numstr[64],rsaddr[64];
    uint32_t dist,numtickets,numplayers,firstblock = 0;
    uint64_t winners[1000],bestticket,bestplayer,sold,xfer,bought,received,best,buyer = 0,seed = 0;
    int64_t total = 0;
    memset(winners,0,sizeof(winners));
    bestticket = bestplayer = 0;
    numwinners = nettickets = netplayers = numtickets = numplayers = 0;
    if ( (n= cJSON_GetArraySize(*jsonp)) > 0 )
    {
        for (i=0; i<n; i++)
        {
            item = cJSON_GetArrayItem(*jsonp,i);
            buyer = get_API_nxt64bits(cJSON_GetObjectItem(item,"buyer"));
            if ( iter < 0 || buyerbits == buyer )
            {
                seed = get_API_nxt64bits(cJSON_GetObjectItem(item,"seed"));
                total = get_API_int(cJSON_GetObjectItem(item,"total"),0);
                firstblock = (uint32_t)get_API_int(cJSON_GetObjectItem(item,"firstblock"),0);
                if ( iter >= 0 )
                    break;
                if ( total > 0 && total >= MIN_REQUIRED )
                {
                    num = (int32_t)(total / MIN_REQUIRED);
                    netnum = 0;
                    printf("num.%d numtickets.%d total.%lld\n",numtickets,num,(long long)total);
                    if ( total >= MIN_REQUIRED )
                    {
                        if ( (netcount = (int32_t)calc_netassets(refbits,&bought,&received,&sold,&xfer,buyer,assetidstr,firstblock,total)) >= MIN_REQUIRED )
                        {
                            netnum = (netcount / MIN_REQUIRED);
                            cJSON_AddItemToObject(item,"tickets",gen_lottotickets_json(&best,seed,netnum,lotto));
                            sprintf(numstr,"%llu",(long long)best), cJSON_AddItemToObject(item,"best",cJSON_CreateString(numstr));
                            dist = bitweight(best ^ lotto);
                            if ( dist < bestdist )
                            {
                                numwinners = 0;
                                memset(winners,0,sizeof(winners));
                                winners[numwinners++] = buyer;
                                bestdist = dist;
                                bestticket = best;
                                bestplayer = buyer;
                            } else if ( dist == bestdist && (numwinners == 0 || winners[numwinners-1] != buyer) )
                                winners[numwinners++] = buyer;
                            cJSON_AddItemToObject(item,"numtickets",cJSON_CreateNumber(netnum));
                            cJSON_AddItemToObject(item,"dist",cJSON_CreateNumber(dist));
                            netplayers++;
                        }
                        if ( sold != 0 )
                            sprintf(numstr,"%llu",(long long)sold), cJSON_AddItemToObject(item,"sold",cJSON_CreateString(numstr));
                        if ( xfer != 0 )
                            sprintf(numstr,"%llu",(long long)xfer), cJSON_AddItemToObject(item,"xfer",cJSON_CreateString(numstr));
                        if ( bought != 0 )
                            sprintf(numstr,"%llu",(long long)bought), cJSON_AddItemToObject(item,"bought",cJSON_CreateString(numstr));
                        if ( received != 0 )
                            sprintf(numstr,"%llu",(long long)received), cJSON_AddItemToObject(item,"received",cJSON_CreateString(numstr));
                        cJSON_AddItemToObject(item,"netcount",cJSON_CreateNumber(netcount));
                        numplayers++;
                    }
                    numtickets += num;
                    nettickets += netnum;
                }
            }
        }
    } else i = 0;
    if ( iter < 0 )
    {
        item = cJSON_CreateObject();
        cJSON_AddItemToObject(item,"numtickets",cJSON_CreateNumber(numtickets));
        cJSON_AddItemToObject(item,"numplayers",cJSON_CreateNumber(numplayers));
        cJSON_AddItemToObject(item,"netplayers",cJSON_CreateNumber(netplayers));
        cJSON_AddItemToObject(item,"nettickets",cJSON_CreateNumber(nettickets));
        cJSON_AddItemToObject(item,"prizefund",cJSON_CreateNumber(prizefund));
        if ( netplayers > 0 )
        {
            cJSON_AddItemToObject(item,"ave tickets",cJSON_CreateNumber((double)numtickets/netplayers));
            if ( nettickets > 0 )
            {
                sprintf(numstr,"%.2f",prizefund / nettickets), cJSON_AddItemToObject(item,"theoretical value",cJSON_CreateString(numstr));
                cJSON_AddItemToObject(item,"best",cJSON_CreateNumber(bitweight(bestticket ^ lotto)));
                sprintf(numstr,"%llu",(long long)bestticket), cJSON_AddItemToObject(item,"winningticket",cJSON_CreateString(numstr));
                if ( numwinners > 0 )
                {
                    array = cJSON_CreateArray();
                    for (w=0; w<numwinners; w++)
                    {
                        conv_rsacctstr(rsaddr,winners[w]);
                        cJSON_AddItemToArray(array,cJSON_CreateString(rsaddr));
                    }
                    cJSON_AddItemToObject(item,"winners",array);
                    sprintf(numstr,"%.2f",prizefund / numwinners), cJSON_AddItemToObject(item,"prizes",cJSON_CreateString(numstr));
                }
            }
        }
        cJSON_AddItemToObject(*jsonp,"Lotto Results",item);
        return;
    }
    if ( i == n )
    {
        item = cJSON_CreateObject();
        conv_rsacctstr(rsaddr,buyerbits);
        cJSON_AddItemToObject(item,"RS",cJSON_CreateString(rsaddr));
        sprintf(numstr,"%llu",(long long)txid), cJSON_AddItemToObject(item,"seed",cJSON_CreateString(numstr));
        sprintf(numstr,"%llu",(long long)buyerbits), cJSON_AddItemToObject(item,"buyer",cJSON_CreateString(numstr));
        sprintf(numstr,"%lld",(long long)assetoshis), cJSON_AddItemToObject(item,"total",cJSON_CreateString(numstr));
        cJSON_AddItemToObject(item,"firstblock",cJSON_CreateNumber(blockid));
        cJSON_AddItemToArray(*jsonp,item);
    }
    else if ( assetoshis > 0 || blockid >= firstblock )
    {
        if ( iter < 2 )
            seed ^= txid;
        printf("total %lld -> %lld | txid.%llx seed.%llx | ",(long long)total,(long long)(total+assetoshis),(long long)txid,(long long)seed);
        total += assetoshis;
        sprintf(numstr,"%lld",(long long)total), cJSON_ReplaceItemInObject(item,"total",cJSON_CreateString(numstr));
        if ( firstblock == 0 || (blockid != 0 && blockid < firstblock) )
        {
            firstblock = blockid;
            cJSON_ReplaceItemInObject(item,"firstblock",cJSON_CreateNumber(firstblock));
        }
    }
    printf("iter.%d: %24llu block.%d %lld total %lld 1st.%u | numtickets.%d\n",iter,(long long)buyerbits,blockid,(long long)assetoshis,(long long)total,firstblock,numtickets);
}

char *update_lotto_transactions(char *refNXTaddr,char *assetidstr,char *lottoseed,double prizefund)
{
    char cmd[1024],hexstr[128],hexstr2[128],*jsonstr;
    bits256 hash,hash2;
    int32_t createdflag,blockid,iter,i,n = 0;
    cJSON *item,*json,*array,*jsonarg;
    uint64_t refbits,sellerbits,buyerbits,qnt,txid,lotto;
    struct NXT_asset *ap;
    struct coin_info *cp;
    calc_sha256(hexstr,hash.bytes,(uint8_t *)lottoseed,(int32_t)strlen(lottoseed));
    lotto = hash.txid;
    calc_sha256(hexstr2,hash2.bytes,(uint8_t *)hexstr,(int32_t)strlen(hexstr));
    //if ( MGW_initdone == 0 )
    //    return(clonestr("{\"error\":\"not initialized yet\"}"));
    printf("lottoseed.(%s) -> %s %llu | hash2 %s\n",lottoseed,hexstr,(long long)lotto,hexstr2);
    jsonarg = cJSON_CreateArray();
    ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
    if ( init_asset(ap,assetidstr,0) == 0 )
        init_asset(ap,assetidstr,1);
    if ( refNXTaddr == 0 || assetidstr == 0 )//|| ap->mult == 0 )
    {
        printf("illegal refNXTaddr.(%s) (%s).mult %d\n",refNXTaddr,assetidstr,(int)ap->mult);
        return(0);
    }
    cp = get_coin_info(ap->name);
    refbits = conv_acctstr(refNXTaddr);
    for (iter=0; iter<4; iter++)
    {
        sprintf(cmd,"%s=%s&account=%s&asset=%s",_NXTSERVER,(iter & 1) ? "getAssetTransfers" : "getTrades",refNXTaddr,assetidstr);
        if ( (jsonstr= issue_NXTPOST(0,cmd)) != 0 )
        {
            if ( (json= cJSON_Parse(jsonstr)) != 0 )
            {
                if ( (array= cJSON_GetObjectItem(json,(iter & 1) ? "transfers" : "trades")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
                {
                    for (i=0; i<n; i++)
                    {
                        item = cJSON_GetArrayItem(array,i);
                        if ( (iter & 1) != 0 )
                            txid = get_API_nxt64bits(cJSON_GetObjectItem(item,"assetTransfer"));
                        else txid = (get_API_nxt64bits(cJSON_GetObjectItem(item,"bidOrder")) ^ get_API_nxt64bits(cJSON_GetObjectItem(item,"askOrder")));
                        qnt = get_API_nxt64bits(cJSON_GetObjectItem(item,"quantityQNT"));
                        blockid = (int32_t)get_API_int(cJSON_GetObjectItem(item,"height"),0);
                        sellerbits = get_API_nxt64bits(cJSON_GetObjectItem(item,(iter & 1) ? "sender" : "seller"));
                        buyerbits = get_API_nxt64bits(cJSON_GetObjectItem(item,(iter & 1) ? "recipient" : "buyer"));
                        if ( calc_nxt64bits(EXCLUDED_ACCT) == buyerbits )
                            continue;
                        if ( ((iter&2) == 0 && sellerbits == refbits) || ((iter&2) != 0 && buyerbits == refbits) )
                            process_lotto(refbits,prizefund,assetidstr,lotto,&jsonarg,iter,(iter&2) == 0 ? buyerbits : sellerbits,qnt * (1 - (iter&2)),blockid,txid);
                    }
                }
                free_json(json);
            }
            free(jsonstr);
        } else printf("iter.%d error with update_lotto_transactions.(%s)\n",iter,cmd);
    }
    process_lotto(refbits,prizefund,assetidstr,lotto,&jsonarg,-1,0,0,0,0);
    item = cJSON_CreateObject();
    cJSON_AddItemToObject(item,"lottoseed",cJSON_CreateString(lottoseed));
    cJSON_AddItemToObject(item,"hash",cJSON_CreateString(hexstr));
    cJSON_AddItemToObject(item,"hash2",cJSON_CreateString(hexstr2));
    cJSON_AddItemToArray(jsonarg,item);

    return(cJSON_Print(jsonarg));
}

#endif

