
//
//  lotto.h
//
//  Created by jl777 on 12/20/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef LOTTO_H
#define LOTTO_H

#define MIN_REQUIRED 100

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
        tmp = sha256_key(hash);
        ticket = tmp.txid;
        hash = tmp;
    }
    return(array);
}

void process_lotto(uint64_t lotto,cJSON **jsonp,int32_t iter,uint64_t buyerbits,int64_t assetoshis,int32_t blockid,uint64_t txid)
{
    int32_t i,n,num,bestdist = 64;
    cJSON *item = 0;
    char numstr[64],rsaddr[64];
    uint32_t dist,numtickets,numplayers,firstblock = 0;
    uint64_t bestticket,bestplayer,best,buyer = 0,seed = 0;
    int64_t total = 0;
    bestticket = bestplayer = 0;
    numtickets = numplayers = 0;
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
                firstblock = (uint32_t)get_API_int(cJSON_GetObjectItem(item,"first"),0);
                if ( iter >= 0 )
                    break;
                if ( total > 0 && total >= MIN_REQUIRED )
                {
                    num = (int32_t)(total / MIN_REQUIRED);
                    printf("num.%d numtickets.%d total.%lld\n",numtickets,num,(long long)total);
                    if ( total >= MIN_REQUIRED )
                    {
                        cJSON_AddItemToObject(item,"tickets",gen_lottotickets_json(&best,seed,num,lotto));
                        sprintf(numstr,"%llu",(long long)best), cJSON_AddItemToObject(item,"best",cJSON_CreateString(numstr));
                        dist = bitweight(best ^ lotto);
                        if ( dist < bestdist )
                        {
                            bestdist = dist;
                            bestticket = best;
                            bestplayer = buyer;
                        }
                        cJSON_AddItemToObject(item,"dist",cJSON_CreateNumber(dist));
                    }
                    numtickets += num;
                    numplayers++;
                }
            }
        }
    } else i = 0;
    if ( iter < 0 )
    {
        printf("Total tickets %d players.%d ave %.1f | best.%d NXT.%llu ticket.%llu\n",numtickets,numplayers,(double)numtickets/numplayers,bitweight(bestticket ^ lotto),(long long)bestplayer,(long long)bestticket);
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

char *update_lotto_transactions(char *refNXTaddr,char *assetidstr,char *lottoseed)
{
    char cmd[1024],hexstr[128],*jsonstr;
    bits256 hash;
    int32_t createdflag,blockid,iter,i,n = 0;
    cJSON *item,*json,*array,*jsonarg;
    uint64_t refbits,sellerbits,buyerbits,qnt,txid,lotto;
    struct NXT_asset *ap;
    struct coin_info *cp;
    calc_sha256(hexstr,hash.bytes,(uint8_t *)lottoseed,(int32_t)strlen(lottoseed));
    lotto = hash.txid;
    printf("lottoseed.(%s) -> %s %llu\n",lottoseed,hexstr,(long long)lotto);
    jsonarg = cJSON_CreateArray();
    ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
    init_asset(ap,assetidstr);
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
                        if ( ((iter&2) == 0 && sellerbits == refbits) || ((iter&2) != 0 && buyerbits == refbits) )
                            process_lotto(lotto,&jsonarg,iter,(iter&2) == 0 ? buyerbits : sellerbits,qnt * (1 - (iter&2)),blockid,txid);
                    }
                }
                free_json(json);
            }
            free(jsonstr);
        } else printf("iter.%d error with update_lotto_transactions.(%s)\n",iter,cmd);
    }
    process_lotto(lotto,&jsonarg,-1,0,0,0,0);
    return(cJSON_Print(jsonarg));
}

#endif

