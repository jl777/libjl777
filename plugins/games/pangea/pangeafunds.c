/******************************************************************************
 * Copyright © 2014-2015 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

char *pangea_typestr(uint8_t type)
{
    static char err[64];
    switch ( type )
    {
        case 0xff: return("fold");
        case CARDS777_START: return("start");
        case CARDS777_ANTE: return("ante");
        case CARDS777_SMALLBLIND: return("smallblind");
        case CARDS777_BIGBLIND: return("bigblind");
        case CARDS777_CHECK: return("check");
        case CARDS777_CALL: return("call");
        case CARDS777_BET: return("bet");
        case CARDS777_RAISE: return("raise");
        case CARDS777_FULLRAISE: return("fullraise");
        case CARDS777_SENTCARDS: return("sentcards");
        case CARDS777_ALLIN: return("allin");
        case CARDS777_FACEUP: return("faceup");
        case CARDS777_WINNINGS: return("won");
        case CARDS777_RAKES: return("rakes");
    }
    sprintf(err,"unknown type.%d",type);
    return(err);
}

char *pangea_dispsummary(int32_t verbose,uint8_t *summary,int32_t summarysize,uint64_t tableid,int32_t handid,int32_t numplayers)
{
    int32_t i,cardi,n = 0,numhands = 0,len = 0; uint8_t type,player = 0; uint64_t bits64 = 0,rake = 0; bits256 card; char hexstr[65],str[16];
    cJSON *item,*json,*all,*cardis[52],*players[CARDS777_MAXPLAYERS],*pitem,*array = cJSON_CreateArray();
    all = cJSON_CreateArray();
    memset(cardis,0,sizeof(cardis));
    for (i=0; i<numplayers; i++)
        players[i] = cJSON_CreateArray();
    while ( len < summarysize )
    {
        len += hostnet777_copybits(1,&summary[len],(void *)&type,sizeof(type));
        if ( type == CARDS777_START )
            len += hostnet777_copybits(1,&summary[len],(void *)&numhands,sizeof(numhands));
        else if ( type == CARDS777_RAKES )
            len += hostnet777_copybits(1,&summary[len],(void *)&rake,sizeof(rake));
        else len += hostnet777_copybits(1,&summary[len],(void *)&player,sizeof(player));
        if ( type == CARDS777_FACEUP )
            len += hostnet777_copybits(1,&summary[len],card.bytes,sizeof(card));
        else len += hostnet777_copybits(1,&summary[len],(void *)&bits64,sizeof(bits64));
        item = cJSON_CreateObject();
        switch ( type )
        {
            case CARDS777_START: jaddnum(item,"hand",numhands); jadd64bits(item,"checkprod",bits64); break;
            case CARDS777_RAKES: jaddnum(item,"hostrake",dstr(rake)); jaddnum(item,"pangearake",dstr(bits64)); break;
            case CARDS777_WINNINGS:
                jaddnum(item,"player",player);
                jaddnum(item,"won",dstr(bits64));
                if ( pitem == 0 )
                    pitem = cJSON_CreateObject();
                jaddnum(pitem,"won",dstr(bits64));
                break;
            case CARDS777_FACEUP:
                cardi = player;
                jaddnum(item,"cardi",cardi);
                cardstr(str,card.bytes[1]);
                jaddnum(item,str,card.bytes[1]);
                init_hexbytes_noT(hexstr,card.bytes,sizeof(card));
                jaddstr(item,"privkey",hexstr);
                cardis[cardi] = item, item = 0;
                break;
            default:
                jaddstr(item,"action",pangea_typestr(type));
                jaddnum(item,"player",player);
                if ( pitem == 0 )
                    pitem = cJSON_CreateObject();
                if ( bits64 != 0 )
                {
                    jaddnum(item,"bet",dstr(bits64));
                    jaddnum(pitem,pangea_typestr(type),dstr(bits64));
                }
                else jaddstr(pitem,"action",pangea_typestr(type));
                break;
        }
        if ( pitem != 0 )
        {
            jaddnum(pitem,"n",n), n++;
            jaddi(players[player],pitem), pitem = 0;
        }
        jaddi(array,item);
    }
    for (i=0; i<numplayers; i++)
        jaddi(all,players[i]);
    if ( verbose == 0 )
    {
        for (i=0; i<52; i++)
            if ( cardis[i] != 0 )
                free_json(cardis[i]);
        free_json(array);
        return(jprint(all,1));
    }
    else
    {
        json = cJSON_CreateObject();
        jadd64bits(json,"tableid",tableid);
        jaddnum(json,"handid",handid);
        //jaddnum(json,"crc",_crc32(0,summary,summarysize));
        jadd(json,"hand",array);
        array = cJSON_CreateArray();
        for (i=0; i<52; i++)
            if ( cardis[i] != 0 )
                jaddi(array,cardis[i]);
        jadd(json,"cards",array);
        //jadd(json,"players",all);
        return(jprint(json,1));
    }
}

void pangea_fold(struct cards777_pubdata *dp,int32_t player)
{
    uint8_t tmp;
    printf("player.%d folded\n",player); //getchar();
    dp->hand.handmask |= (1 << player);
    dp->hand.betstatus[player] = CARDS777_FOLD;
    dp->hand.actions[player] = CARDS777_FOLD;
    tmp = player;
    pangea_summary(dp,CARDS777_FOLD,&tmp,sizeof(tmp),(void *)&dp->hand.bets[player],sizeof(dp->hand.bets[player]));
}

uint64_t pangea_totalbet(struct cards777_pubdata *dp)
{
    int32_t j; uint64_t total;
    for (total=j=0; j<dp->N; j++)
        total += dp->hand.bets[j];
    return(total);
}

int32_t pangea_actives(int32_t *activej,struct cards777_pubdata *dp)
{
    int32_t i,n;
    *activej = -1;
    for (i=n=0; i<dp->N; i++)
    {
        if ( dp->hand.betstatus[i] != CARDS777_FOLD )
        {
            if ( *activej < 0 )
                *activej = i;
            n++;
        }
    }
    return(n);
}

struct pangea_info *pangea_usertables(int32_t *nump,uint64_t my64bits,uint64_t tableid)
{
    int32_t i,j,num = 0; struct pangea_info *sp,*retsp = 0;
    *nump = 0;
    for (i=0; i<sizeof(TABLES)/sizeof(*TABLES); i++)
    {
        if ( (sp= TABLES[i]) != 0 )
        {
            for (j=0; j<sp->numaddrs; j++)
                if ( sp->addrs[j] == my64bits && (tableid == 0 || sp->tableid == tableid) )
                {
                    if ( num++ == 0 )
                    {
                        retsp = sp;
                        break;
                    }
                }
        }
    }
    *nump = num;
    return(retsp);
}

struct pangea_info *pangea_threadtables(int32_t *nump,int32_t threadid,uint64_t tableid)
{
    int32_t i,j,num = 0; struct pangea_info *sp,*retsp = 0;
    *nump = 0;
    for (i=0; i<sizeof(TABLES)/sizeof(*TABLES); i++)
    {
        if ( (sp= TABLES[i]) != 0 )
        {
            for (j=0; j<sp->numaddrs; j++)
                if ( sp->tp != 0 && sp->tp->threadid == threadid && (tableid == 0 || sp->tableid == tableid) )
                {
                    if ( num++ == 0 )
                    {
                        retsp = sp;
                        break;
                    }
                }
        }
    }
    *nump = num;
    return(retsp);
}

int32_t pangea_bet(union hostnet777 *hn,struct cards777_pubdata *dp,int32_t player,int64_t bet,int32_t action)
{
    uint64_t sum; uint8_t tmp;
    player %= dp->N;
    if ( Debuglevel > 2 )
        printf("player.%d PANGEA_BET[%d] <- %.8f\n",hn->client->H.slot,player,dstr(bet));
    if ( dp->hand.betstatus[player] == CARDS777_ALLIN )
        return(CARDS777_ALLIN);
    else if ( dp->hand.betstatus[player] == CARDS777_FOLD )
        return(CARDS777_FOLD);
    if ( bet > 0 && bet >= dp->balances[player] )
    {
        bet = dp->balances[player];
        dp->hand.betstatus[player] = action = CARDS777_ALLIN;
    }
    else
    {
        if ( bet > dp->hand.betsize && bet > dp->hand.lastraise && bet < (dp->hand.lastraise<<1) )
        {
            printf("pangea_bet %.8f not double %.8f, clip to lastraise\n",dstr(bet),dstr(dp->hand.lastraise));
            bet = dp->hand.lastraise;
            action = CARDS777_RAISE;
        }
    }
    sum = dp->hand.bets[player];
    if ( sum+bet < dp->hand.betsize && action != CARDS777_ALLIN )
    {
        pangea_fold(dp,player);
        action = CARDS777_FOLD;
        tmp = player;
        if ( Debuglevel > 2 )
            printf("player.%d betsize %.8f < hand.betsize %.8f FOLD\n",player,dstr(bet),dstr(dp->hand.betsize));
        return(action);
    }
    else if ( bet >= 2*dp->hand.lastraise )
    {
        dp->hand.lastraise = bet;
        dp->hand.numactions = 0;
        if ( action == CARDS777_CHECK )
        {
            action = CARDS777_FULLRAISE; // allows all players to check/bet again
            printf("FULLRAISE by player.%d\n",player);
        }
    }
    sum += bet;
    if ( sum > dp->hand.betsize )
    {
        dp->hand.numactions = 0;
        dp->hand.betsize = sum, dp->hand.lastbettor = player;
        if ( sum > dp->hand.lastraise && action == CARDS777_ALLIN )
            dp->hand.lastraise = sum;
        else if ( action == CARDS777_CHECK )
            action = CARDS777_BET;
    }
    if ( bet > 0 && action == CARDS777_CHECK )
        action = CARDS777_CALL;
    tmp = player;
    pangea_summary(dp,action,&tmp,sizeof(tmp),(void *)&bet,sizeof(bet));
    dp->balances[player] -= bet, dp->hand.bets[player] += bet;
    printf("player.%d: player.%d BET %f -> balances %f bets %f\n",hn->client->H.slot,player,dstr(bet),dstr(dp->balances[player]),dstr(dp->hand.bets[player]));
    return(action);
}

uint64_t pangea_winnings(int32_t player,uint64_t *pangearakep,uint64_t *hostrakep,uint64_t total,int32_t numwinners,int32_t rakemillis)
{
    uint64_t split,pangearake,rake;
    if ( numwinners > 0 )
    {
        split = (total * (1000 - rakemillis)) / (1000 * numwinners);
        pangearake = (total - split*numwinners);
    }
    else
    {
        split = 0;
        pangearake = total;
    }
    if ( rakemillis > PANGEA_MINRAKE_MILLIS )
    {
        rake = (pangearake * (rakemillis - PANGEA_MINRAKE_MILLIS)) / rakemillis;
        pangearake -= rake;
    }
    else rake = 0;
    *hostrakep = rake;
    *pangearakep = pangearake;
    printf("\nP%d: rakemillis.%d total %.8f split %.8f rake %.8f pangearake %.8f\n",player,rakemillis,dstr(total),dstr(split),dstr(rake),dstr(pangearake));
    return(split);
}

int32_t pangea_sidepots(int32_t dispflag,uint64_t sidepots[CARDS777_MAXPLAYERS][CARDS777_MAXPLAYERS],struct cards777_pubdata *dp,int64_t *bets)
{
    int32_t i,j,nonz,n = 0; uint64_t bet,minbet = 0;
    memset(sidepots,0,sizeof(uint64_t)*CARDS777_MAXPLAYERS*CARDS777_MAXPLAYERS);
    for (j=0; j<dp->N; j++)
        sidepots[0][j] = bets[j];
    nonz = 1;
    while ( nonz > 0 )
    {
        for (minbet=j=0; j<dp->N; j++)
        {
            if ( (bet= sidepots[n][j]) != 0 )
            {
                if ( dp->hand.betstatus[j] != CARDS777_FOLD )
                {
                    if ( minbet == 0 || bet < minbet )
                        minbet = bet;
                }
            }
        }
        for (j=nonz=0; j<dp->N; j++)
        {
            if ( sidepots[n][j] > minbet && dp->hand.betstatus[j] != CARDS777_FOLD )
                nonz++;
        }
        if ( nonz > 0 )
        {
            for (j=0; j<dp->N; j++)
            {
                if ( sidepots[n][j] > minbet )
                {
                    sidepots[n+1][j] = (sidepots[n][j] - minbet);
                    sidepots[n][j] = minbet;
                }
            }
        }
        if ( ++n >= dp->N )
            break;
    }
    if ( dispflag != 0 )
    {
        for (i=0; i<n; i++)
        {
            for (j=0; j<dp->N; j++)
                printf("%.8f ",dstr(sidepots[i][j]));
            printf("sidepot.%d of %d\n",i,n);
        }
    }
    return(n);
}

int64_t pangea_splitpot(int64_t *won,uint64_t *pangearakep,uint64_t sidepot[CARDS777_MAXPLAYERS],union hostnet777 *hn,int32_t rakemillis)
{
    int32_t winners[CARDS777_MAXPLAYERS],j,n,numwinners = 0; uint32_t bestrank,rank; uint8_t tmp;
    uint64_t total = 0,bet,split,rake=0,pangearake=0; char handstr[128],besthandstr[128]; struct cards777_pubdata *dp;
    dp = hn->client->H.pubdata;
    bestrank = 0;
    for (j=n=0; j<dp->N; j++)
    {
        if ( (bet= sidepot[j]) != 0 )
        {
            total += bet;
            if ( dp->hand.betstatus[j] != CARDS777_FOLD )
            {
                if ( dp->hand.handranks[j] > bestrank )
                {
                    bestrank = dp->hand.handranks[j];
                    set_handstr(besthandstr,dp->hand.hands[j],0);
                }
            }
        }
    }
    for (j=0; j<dp->N; j++)
    {
        if ( dp->hand.betstatus[j] != CARDS777_FOLD && sidepot[j] > 0 )
        {
            if ( dp->hand.handranks[j] == bestrank )
                winners[numwinners++] = j;
            rank = set_handstr(handstr,dp->hand.hands[j],0);
            if ( handstr[strlen(handstr)-1] == ' ' )
                handstr[strlen(handstr)-1] = 0;
            //if ( hn->server->H.slot == 0 )
            printf("(p%d %14s)",j,handstr[0]!=' '?handstr:handstr+1);
            //printf("(%2d %2d).%d ",dp->hands[j][5],dp->hands[j][6],(int32_t)dp->balances[j]);
        }
    }
    if ( numwinners == 0 )
        printf("pangea_splitpot error: numwinners.0\n");
    else
    {
        split = pangea_winnings(hn->client->H.slot,&pangearake,&rake,total,numwinners,rakemillis);
        (*pangearakep) += pangearake;
        for (j=0; j<numwinners; j++)
        {
            tmp = winners[j];
            pangea_summary(dp,CARDS777_WINNINGS,&tmp,sizeof(tmp),(void *)&split,sizeof(split));
            dp->balances[winners[j]] += split;
            won[winners[j]] += split;
        }
        if ( split*numwinners + rake + pangearake != total )
            printf("pangea_split total error %.8f != split %.8f numwinners %d rake %.8f pangearake %.8f\n",dstr(total),dstr(split),numwinners,dstr(rake),dstr(pangearake));
        //if ( hn->server->H.slot == 0 )
        {
            printf(" total %.8f split %.8f rake %.8f Prake %.8f %s N%d winners ",dstr(total),dstr(split),dstr(rake),dstr(pangearake),besthandstr,dp->numhands);
            for (j=0; j<numwinners; j++)
                printf("%d ",winners[j]);
            printf("\n");
        }
    }
    return(rake);
}

uint64_t pangea_bot(union hostnet777 *hn,struct cards777_pubdata *dp,int32_t turni,int32_t cardi,uint64_t betsize)
{
    int32_t r,action=CARDS777_CHECK,n,activej; char hex[1024]; uint64_t threshold,total,sum,amount = 0;
    sum = dp->hand.bets[hn->client->H.slot];
    action = 0;
    n = pangea_actives(&activej,dp);
    if ( (r = (rand() % 100)) == 0 )
        amount = dp->balances[hn->client->H.slot], action = CARDS777_ALLIN;
    else
    {
        if ( betsize == sum )
        {
            if ( r < 100/n )
            {
                amount = dp->hand.lastraise * 2;
                action = 2;
            }
        }
        else if ( betsize > sum )
        {
            amount = (betsize - sum);
            total = pangea_totalbet(dp);
            threshold = (100 * amount)/(1 + total);
            n++;
            if ( 1 || r/n > threshold )
            {
                action = 1;
                if ( r/n > 3*threshold && amount < dp->hand.lastraise*2 )
                    amount = dp->hand.lastraise * 2, action = 2;
                else if ( r/n > 10*threshold )
                    amount = dp->balances[hn->client->H.slot], action = CARDS777_ALLIN;
            } else action = CARDS777_FOLD, amount = 0;
        }
        else printf("pangea_turn error betsize %.8f vs sum %.8f\n",dstr(betsize),dstr(sum));
        if ( amount > dp->balances[hn->client->H.slot] )
            amount = dp->balances[hn->client->H.slot], action = CARDS777_ALLIN;
    }
    pangea_sendcmd(hex,hn,"action",-1,(void *)&amount,sizeof(amount),cardi,action);
    printf("playerbot.%d got pangea_turn.%d for player.%d action.%d bet %.8f\n",hn->client->H.slot,cardi,turni,action,dstr(amount));
    return(amount);
}

cJSON *pangea_handjson(struct cards777_handinfo *hand,uint8_t *holecards,int32_t isbot)
{
    int32_t i,card; char cardAstr[8],cardBstr[8],pairstr[18],cstr[128]; cJSON *array,*json = cJSON_CreateObject();
    array = cJSON_CreateArray();
    cstr[0] = 0;
    for (i=0; i<5; i++)
    {
        if ( (card= hand->community[i]) != 0xff )
        {
            jaddinum(array,card);
            cardstr(&cstr[strlen(cstr)],card);
            strcat(cstr," ");
        }
    }
    jaddstr(json,"community",cstr);
    jadd(json,"cards",array);
    if ( (card= holecards[0]) != 0xff )
    {
        jaddnum(json,"cardA",card);
        cardstr(cardAstr,holecards[0]);
    } else cardAstr[0] = 0;
    if ( (card= holecards[1]) != 0xff )
    {
        jaddnum(json,"cardB",card);
        cardstr(cardBstr,holecards[1]);
    } else cardBstr[0] = 0;
    sprintf(pairstr,"%s %s",cardAstr,cardBstr);
    jaddstr(json,"holecards",pairstr);
    jaddnum(json,"betsize",dstr(hand->betsize));
    jaddnum(json,"lastraise",dstr(hand->lastraise));
    jaddnum(json,"lastbettor",hand->lastbettor);
    jaddnum(json,"numactions",hand->numactions);
    jaddnum(json,"undergun",hand->undergun);
    jaddnum(json,"isbot",isbot);
    jaddnum(json,"cardi",hand->cardi);
    return(json);
}

char *pangea_statusstr(int32_t status)
{
    if ( status == CARDS777_FOLD )
        return("folded");
    else if ( status == CARDS777_ALLIN )
        return("ALLin");
    else return("active");
}

int32_t pangea_countdown(struct cards777_pubdata *dp,int32_t player)
{
    if ( dp->hand.undergun == player && dp->hand.userinput_starttime != 0 )
        return((int32_t)(dp->hand.userinput_starttime + PANGEA_USERTIMEOUT - time(NULL)));
    else return(-1);
}

cJSON *pangea_tablestatus(struct pangea_info *sp)
{
    uint64_t sidepots[CARDS777_MAXPLAYERS][CARDS777_MAXPLAYERS],totals[CARDS777_MAXPLAYERS],sum; char *handhist;
    int32_t i,n,j,countdown; int64_t total; struct cards777_pubdata *dp; cJSON *bets,*item,*array,*json = cJSON_CreateObject();
    jadd64bits(json,"tableid",sp->tableid);
    jadd64bits(json,"myind",sp->myind);
    dp = sp->dp;
    jaddnum(json,"minbuyin",dp->minbuyin);
    jaddnum(json,"maxbuyin",dp->maxbuyin);
    jaddnum(json,"button",dp->button);
    jaddnum(json,"M",dp->M);
    jaddnum(json,"N",dp->N);
    jaddnum(json,"numcards",dp->numcards);
    jaddnum(json,"numhands",dp->numhands);
    jaddnum(json,"rake",(double)dp->rakemillis/10.);
    jaddnum(json,"hostrake",dstr(dp->hostrake));
    jaddnum(json,"pangearake",dstr(dp->pangearake));
    jaddnum(json,"bigblind",dstr(dp->bigblind));
    jaddnum(json,"ante",dstr(dp->ante));
    array = cJSON_CreateArray();
    for (i=0; i<dp->N; i++)
        jaddi64bits(array,sp->addrs[i]);
    jadd(json,"addrs",array);
    array = cJSON_CreateArray();
    for (i=0; i<dp->N; i++)
        jaddinum(array,dp->hand.turnis[i]);
    jadd(json,"turns",array);
    array = cJSON_CreateArray();
    for (i=0; i<dp->N; i++)
        jaddinum(array,dstr(dp->balances[i]));
    jadd(json,"balances",array);
    array = cJSON_CreateArray();
    for (i=0; i<dp->N; i++)
        jaddinum(array,dstr(dp->hand.snapshot[i]));
    jadd(json,"snapshot",array);
    array = cJSON_CreateArray();
    for (i=0; i<dp->N; i++)
        jaddistr(array,pangea_statusstr(dp->hand.betstatus[i]));
    jadd(json,"status",array);
    bets = cJSON_CreateArray();
    for (total=i=0; i<dp->N; i++)
    {
        total += dp->hand.bets[i];
        jaddinum(bets,dstr(dp->hand.bets[i]));
    }
    jadd(json,"bets",bets);
    jaddnum(json,"totalbets",dstr(total));
    if ( (n= pangea_sidepots(0,sidepots,dp,dp->hand.snapshot)) > 0 && n < dp->N )
    {
        array = cJSON_CreateArray();
        for (i=0; i<n; i++)
        {
            item = cJSON_CreateArray();
            for (sum=j=0; j<dp->N; j++)
                jaddinum(item,dstr(sidepots[i][j])), sum += sidepots[i][j];
            totals[i] = sum;
            jaddi(array,item);
        }
        jadd(json,"pots",array);
        item = cJSON_CreateArray();
        for (sum=i=0; i<n; i++)
            jaddinum(item,dstr(totals[i])), sum += totals[i];
        jadd(json,"potTotals",item);
        jaddnum(json,"sum",dstr(sum));
    }
    if ( sp->priv != 0 )
    {
        jadd64bits(json,"autoshow",sp->priv->autoshow);
        jadd64bits(json,"autofold",sp->priv->autofold);
        jadd(json,"hand",pangea_handjson(&dp->hand,sp->priv->hole,dp->isbot[sp->myind]));
    }
    if ( (handhist= pangea_dispsummary(0,dp->summary,dp->summarysize,sp->tableid,dp->numhands-1,dp->N)) != 0 )
    {
        if ( (item= cJSON_Parse(handhist)) != 0 )
            jadd(json,"actions",item);
    }
    if ( (countdown= pangea_countdown(dp,sp->myind)) >= 0 )
        jaddnum(json,"timeleft",countdown);
    if ( dp->hand.pangearake != 0 )
    {
        item = cJSON_CreateObject();
        jaddnum(item,"hostrake",dstr(dp->hand.hostrake));
        jaddnum(item,"pangearake",dstr(dp->hand.pangearake));
        array = cJSON_CreateArray();
        for (i=0; i<dp->N; i++)
            jaddinum(array,dstr(dp->hand.won[i]));
        jadd(item,"won",array);
        jadd(json,"summary",item);
    }
    return(json);
}

void pangea_playerprint(struct cards777_pubdata *dp,int32_t i,int32_t myind)
{
    int32_t countdown; char str[8];
    if ( (countdown= pangea_countdown(dp,i)) >= 0 )
        sprintf(str,"%2d",countdown);
    else str[0] = 0;
    printf("%d: %6s %12.8f %2s  | %12.8f %s\n",i,pangea_statusstr(dp->hand.betstatus[i]),dstr(dp->hand.bets[i]),str,dstr(dp->balances[i]),i == myind ? "<<<<<<<<<<<": "");
}

void pangea_statusprint(struct cards777_pubdata *dp,struct cards777_privdata *priv,int32_t myind)
{
    int32_t i; char handstr[64]; uint8_t hand[7];
    for (i=0; i<dp->N; i++)
        pangea_playerprint(dp,i,myind);
    handstr[0] = 0;
    if ( dp->hand.community[0] != dp->hand.community[1] )
    {
        for (i=0; i<5; i++)
            if ( (hand[i]= dp->hand.community[i]) == 0xff )
                break;
        if ( i == 5 )
        {
            if ( (hand[5]= priv->hole[0]) != 0xff && (hand[6]= priv->hole[1]) != 0xff )
                set_handstr(handstr,hand,1);
        }
    }
    printf("%s\n",handstr);
}

void pangea_startbets(union hostnet777 *hn,struct cards777_pubdata *dp,int32_t cardi)
{
    uint32_t now,i; char hex[1024];
    if ( dp->hand.betstarted == 0 )
    {
        dp->hand.betstarted = 1;
    } else dp->hand.betstarted++;
    dp->hand.numactions = 0;
    dp->hand.cardi = cardi;
    printf("STARTBETS.%d cardi.%d numactions.%d\n",dp->hand.betstarted,cardi,dp->hand.numactions);
    now = (uint32_t)time(NULL);
    memset(dp->hand.actions,0,sizeof(dp->hand.actions));
    memset(dp->hand.turnis,0xff,sizeof(dp->hand.turnis));
    dp->hand.undergun = ((dp->button + 3) % dp->N);
    if ( cardi > dp->N*2 )
    {
        for (i=0; i<dp->N; i++)
            dp->hand.snapshot[i] = dp->hand.bets[i];
    }
    dp->hand.snapshot[dp->N] = dp->hand.betsize;
    pangea_sendcmd(hex,hn,"turn",-1,(void *)dp->hand.snapshot,sizeof(uint64_t)*(dp->N+1),cardi,dp->hand.undergun);
}

int32_t pangea_turn(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t senderind)
{
    int32_t turni,cardi; char hex[2048]; struct pangea_info *sp = dp->table;
    turni = juint(json,"turni");
    cardi = juint(json,"cardi");
    printf("P%d: got turn.%d from %d | cardi.%d summary[%d] crc.%u\n",hn->server->H.slot,turni,senderind,cardi,dp->summarysize,_crc32(0,dp->summary,dp->summarysize));
    dp->hand.turnis[senderind] = turni;
    if ( senderind == 0 && sp != 0 )
    {
        dp->hand.cardi = cardi;
        dp->hand.betstarted = 1;
        dp->hand.undergun = turni;
        if ( hn->client->H.slot != 0 )
        {
            memcpy(dp->hand.snapshot,data,datalen);
            dp->hand.betsize = dp->hand.snapshot[dp->N];
            //printf("player.%d sends confirmturn.%d\n",hn->client->H.slot,turni);
            pangea_sendcmd(hex,hn,"confirmturn",-1,(void *)&sp->tableid,sizeof(sp->tableid),cardi,turni);
        }
    }
    return(0);
}

int32_t pangea_confirmturn(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t senderind)
{
    uint32_t starttime; int32_t i,turni,cardi; uint64_t betsize=SATOSHIDEN,amount=0; struct pangea_info *sp=0; char hex[1024];
    if ( data == 0 )
    {
        printf("pangea_turn: null data\n");
        return(-1);
    }
    turni = juint(json,"turni");
    cardi = juint(json,"cardi");
    //printf("got confirmturn.%d cardi.%d sender.%d\n",turni,cardi,senderind);
    if ( datalen == sizeof(betsize) )
        memcpy(&betsize,data,sizeof(betsize)), starttime = dp->hand.starttime;
    if ( (sp= dp->table) != 0 )
    {
        if ( senderind == 0 )
        {
            dp->hand.undergun = turni;
            dp->hand.cardi = cardi;
            dp->hand.betsize = betsize;
        }
        dp->hand.turnis[senderind] = turni;
        for (i=0; i<dp->N; i++)
        {
            //printf("[i%d %d] ",i,dp->turnis[i]);
            if ( dp->hand.turnis[i] != turni )
                break;
        }
        //printf("sp.%p vs turni.%d cardi.%d hand.cardi %d\n",sp,turni,cardi,dp->hand.cardi);
        if ( hn->client->H.slot == 0 && i == dp->N && senderind != 0 )
        {
            //printf("player.%d sends confirmturn.%d cardi.%d\n",hn->client->H.slot,dp->hand.undergun,dp->hand.cardi);
            pangea_sendcmd(hex,hn,"confirmturn",-1,(void *)&dp->hand.betsize,sizeof(dp->hand.betsize),dp->hand.cardi,dp->hand.undergun);
        }
        if ( senderind == 0 && (turni= dp->hand.undergun) == hn->client->H.slot )
        {
            if ( dp->hand.betsize != betsize )
                printf("pangea_turn warning hand.betsize %.8f != betsize %.8f\n",dstr(dp->hand.betsize),dstr(betsize));
            if ( dp->isbot[hn->client->H.slot] != 0 )
                pangea_bot(hn,dp,turni,cardi,betsize);
            else if ( dp->hand.betstatus[hn->client->H.slot] == CARDS777_FOLD || dp->hand.betstatus[hn->client->H.slot] == CARDS777_ALLIN )
                pangea_sendcmd(hex,hn,"action",-1,(void *)&amount,sizeof(amount),cardi,0);
            else if ( priv->autofold != 0 )
                pangea_sendcmd(hex,hn,"action",-1,(void *)&amount,sizeof(amount),cardi,0);
            else
            {
                dp->hand.userinput_starttime = (uint32_t)time(NULL);
                dp->hand.cardi = cardi;
                dp->hand.betsize = betsize;
                fprintf(stderr,"Waiting for user input cardi.%d: ",cardi);
            }
            printf("%s\n",jprint(pangea_tablestatus(sp),1));
            pangea_statusprint(dp,priv,hn->client->H.slot);
        }
    }
    return(0);
}

int32_t pangea_lastman(union hostnet777 *hn,struct cards777_pubdata *dp,struct cards777_privdata *priv)
{
    int32_t activej = -1; uint64_t total,split,rake,pangearake; char hex[1024]; uint8_t tmp;
    if ( dp->hand.betstarted != 0 && pangea_actives(&activej,dp) <= 1 )
    {
        total = pangea_totalbet(dp);
        split = pangea_winnings(hn->server->H.slot,&pangearake,&rake,total,1,dp->rakemillis);
        dp->hostrake += rake;
        dp->pangearake += pangearake;
        dp->hand.hostrake = rake;
        dp->hand.pangearake = pangearake;
        dp->hand.won[hn->server->H.slot] = split;
        if ( activej >= 0 )
            dp->balances[activej] += split;
        if ( hn->server->H.slot == activej && priv->autoshow != 0 )
        {
            pangea_sendcmd(hex,hn,"faceup",-1,priv->holecards[0].bytes,sizeof(priv->holecards[0]),priv->cardis[0],priv->cardis[0] != 0xff);
            pangea_sendcmd(hex,hn,"faceup",-1,priv->holecards[1].bytes,sizeof(priv->holecards[1]),priv->cardis[1],priv->cardis[0] != 0xff);
        }
        tmp = activej;
        pangea_summary(dp,CARDS777_WINNINGS,&tmp,sizeof(tmp),(void *)&split,sizeof(split));
        pangea_summary(dp,CARDS777_RAKES,(void *)&rake,sizeof(rake),(void *)&pangearake,sizeof(pangearake));
        printf("player.%d lastman standing, wins %.8f hostrake %.8f pangearake %.8f\n",activej,dstr(split),dstr(rake),dstr(pangearake));
        printf("%s\n",jprint(pangea_tablestatus(dp->table),1));
        /*if ( hn->server->H.slot == 0 )
        {
            pangea_sendsummary(dp);
            pangea_clearhand(dp,&dp->hand,priv);
            //pangea_anotherhand(hn,dp,0);
        }*/
        return(1);
    }
    return(0);
}

void pangea_sendsummary(union hostnet777 *hn,struct cards777_pubdata *dp,struct cards777_privdata *priv)
{
    char hex[4096];
    pangea_sendcmd(hex,hn,"summary",-1,dp->summary,dp->summarysize,0,0);
}

int32_t pangea_gotsummary(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t senderind)
{
    char *otherhist,*handhist = 0; int32_t matched = 0; struct pangea_info *sp = dp->table;
    if ( Debuglevel > 2 ) // ordering changes crc
        printf("player.%d [%d]: got summary.%d from %d memcmp.%d\n",hn->client->H.slot,dp->summarysize,datalen,senderind,memcmp(data,dp->summary,datalen));
    if ( datalen == dp->summarysize )
    {
        if ( memcmp(dp->summary,data,datalen) == 0 )
            matched = 1;
        else
        {
            if ( (handhist= pangea_dispsummary(1,dp->summary,dp->summarysize,sp->tableid,dp->numhands-1,dp->N)) != 0 )
            {
                if ( (otherhist= pangea_dispsummary(1,data,datalen,sp->tableid,dp->numhands-1,dp->N)) != 0 )
                {
                    if ( strcmp(handhist,otherhist) == 0 )
                        matched = 1;
                    else printf("\n[%s] vs \n[%s]\n",handhist,otherhist);
                    free(otherhist);
                }
                free(handhist);
            }
        }
    }
    if ( matched != 0 )
        dp->summaries |= (1LL << senderind);
    else dp->mismatches |= (1LL << senderind);
    //if ( senderind == 0 && hn->client->H.slot != 0 )
    //    pangea_sendsummary(hn,dp,priv);
    if ( (dp->mismatches | dp->summaries) == (1LL << dp->N)-1 )
    {
        printf("P%d: hand summary matches.%llx errors.%llx\n",hn->client->H.slot,(long long)dp->summaries,(long long)dp->mismatches);
        //if ( handhist == 0 && (handhist= pangea_dispsummary(1,dp->summary,dp->summarysize,sp->tableid,dp->numhands-1,dp->N)) != 0 )
        //    printf("HAND.(%s)\n",handhist), free(handhist);
        if ( hn->server->H.slot == 0 )
            pangea_anotherhand(hn,dp,3);
    }
    return(0);
}

int32_t pangea_action(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t senderind)
{
    uint32_t now; int32_t action,cardi,i,j,destplayer = 0; bits256 audit[CARDS777_MAXPLAYERS]; char hex[1024]; uint8_t tmp; uint64_t amount = 0;
    action = juint(json,"turni");
    cardi = juint(json,"cardi");
    memcpy(&amount,data,sizeof(amount));
    if ( cardi < 2*dp->N )
        printf("pangea_action: illegal cardi.%d: (%s)\n",cardi,jprint(json,0));
    if ( senderind != dp->hand.undergun )
    {
        printf("out of turn action.%d by player.%d cardi.%d amount %.8f\n",action,senderind,cardi,dstr(amount));
        return(-1);
    }
    tmp = senderind;
    pangea_bet(hn,dp,senderind,amount,CARDS777_CHECK);
    dp->hand.actions[senderind] = action;
    dp->hand.undergun = (dp->hand.undergun + 1) % dp->N;
    dp->hand.numactions++;
    printf("player.%d: got action.%d cardi.%d senderind.%d -> undergun.%d numactions.%d\n",hn->client->H.slot,action,cardi,senderind,dp->hand.undergun,dp->hand.numactions);
    if ( pangea_lastman(hn,dp,priv) > 0 )
    {
        pangea_sendsummary(hn,dp,priv);
        return(0);
    }
    if ( hn->client->H.slot == 0 )
    {
        now = (uint32_t)time(NULL);
        for (i=0; i<dp->N; i++)
        {
            j = (dp->hand.undergun + i) % dp->N;
            if ( dp->hand.betstatus[j] == CARDS777_FOLD || dp->hand.betstatus[j] == CARDS777_ALLIN )
            {
                dp->hand.actions[j] = dp->hand.betstatus[j];
                dp->hand.undergun = (dp->hand.undergun + 1) % dp->N;
                dp->hand.numactions++;
            } else break;
        }
        printf("senderind.%d i.%d j.%d\n",senderind,i,j);
        if ( dp->hand.numactions < dp->N )
        {
            printf("undergun.%d cardi.%d\n",dp->hand.undergun,dp->hand.cardi);
            //if ( senderind != 0 )
                pangea_sendcmd(hex,hn,"turn",-1,(void *)dp->hand.snapshot,sizeof(uint64_t)*(dp->N+1),dp->hand.cardi,dp->hand.undergun);
        }
        else
        {
            for (i=0; i<5; i++)
            {
                if ( dp->hand.community[i] == 0xff )
                    break;
                printf("%02x ",dp->hand.community[i]);
            }
            printf("COMMUNITY\n");
            if ( i == 0 )
            {
                if ( dp->hand.cardi != dp->N * 2 )
                    printf("cardi mismatch %d != %d\n",dp->hand.cardi,dp->N * 2);
                cardi = dp->hand.cardi;
                printf("decode flop\n");
                for (i=0; i<3; i++,cardi++)
                {
                    memset(audit,0,sizeof(audit));
                    audit[0] = dp->hand.final[cardi*dp->N + destplayer];
                    pangea_sendcmd(hex,hn,"decoded",-1,audit[0].bytes,sizeof(bits256)*dp->N,cardi,dp->N-1);
                }
            }
            else if ( i == 3 )
            {
                if ( dp->hand.cardi != dp->N * 2+3 )
                    printf("cardi mismatch %d != %d\n",dp->hand.cardi,dp->N * 2 + 3);
                cardi = dp->hand.cardi;
                printf("decode turn\n");
                memset(audit,0,sizeof(audit));
                audit[0] = dp->hand.final[cardi*dp->N + destplayer];
                pangea_sendcmd(hex,hn,"decoded",-1,audit[0].bytes,sizeof(bits256)*dp->N,cardi,dp->N-1);
                //pangea_sendcmd(hex,hn,"decoded",-1,dp->hand.final[cardi*dp->N + destplayer].bytes,sizeof(dp->hand.final[cardi*dp->N + destplayer]),cardi,dp->N-1);
            }
            else if ( i == 4 )
            {
                printf("decode river\n");
                if ( dp->hand.cardi != dp->N * 2+4 )
                    printf("cardi mismatch %d != %d\n",dp->hand.cardi,dp->N * 2+4);
                cardi = dp->hand.cardi;
                memset(audit,0,sizeof(audit));
                audit[0] = dp->hand.final[cardi*dp->N + destplayer];
                pangea_sendcmd(hex,hn,"decoded",-1,audit[0].bytes,sizeof(bits256)*dp->N,cardi,dp->N-1);
                //pangea_sendcmd(hex,hn,"decoded",-1,dp->hand.final[cardi*dp->N + destplayer].bytes,sizeof(dp->hand.final[cardi*dp->N + destplayer]),cardi,dp->N-1);
            }
            else
            {
                cardi = dp->N * 2 + 5;
                if ( dp->hand.cardi != dp->N * 2+5 )
                    printf("cardi mismatch %d != %d\n",dp->hand.cardi,dp->N * 2+5);
                for (i=0; i<dp->N; i++)
                {
                    j = (dp->hand.lastbettor + i) % dp->N;
                    if ( dp->hand.betstatus[j] != CARDS777_FOLD )
                        break;
                }
                dp->hand.undergun = j;
                printf("sent showdown request for undergun.%d\n",j);
                pangea_sendcmd(hex,hn,"showdown",-1,(void *)&dp->hand.betsize,sizeof(dp->hand.betsize),cardi,dp->hand.undergun);
            }
        }
    }
    if ( Debuglevel > 1 || hn->client->H.slot == 0 )
    {
        printf("player.%d got pangea_action.%d for player.%d action.%d amount %.8f | numactions.%d\n%s\n",hn->client->H.slot,cardi,senderind,action,dstr(amount),dp->hand.numactions,jprint(pangea_tablestatus(dp->table),1));
    }
    return(0);
}

int32_t pangea_myrank(struct cards777_pubdata *dp,int32_t senderind)
{
    int32_t i; uint32_t myrank = dp->hand.handranks[senderind];
    for (i=0; i<dp->N; i++)
        if ( i != senderind && dp->hand.handranks[i] > myrank )
            return(-1);
    return(myrank != 0);
}

int32_t pangea_showdown(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t senderind)
{
    char hex[1024]; int32_t i,turni,cardi; uint64_t amount = 0;
    turni = juint(json,"turni");
    cardi = juint(json,"cardi");
    if ( Debuglevel > 2 )
        printf("P%d: showdown from sender.%d\n",hn->client->H.slot,senderind);
    if ( dp->hand.betstatus[hn->client->H.slot] != CARDS777_FOLD && ((priv->autoshow != 0 && dp->hand.actions[hn->client->H.slot] != CARDS777_SENTCARDS) || (turni == hn->client->H.slot && dp->hand.lastbettor == hn->client->H.slot)) )
    {
        if ( priv->autoshow == 0 && pangea_myrank(dp,hn->client->H.slot) < 0 )
            pangea_sendcmd(hex,hn,"action",-1,(void *)&amount,sizeof(amount),cardi,CARDS777_FOLD);
        else
        {
            pangea_sendcmd(hex,hn,"faceup",-1,priv->holecards[0].bytes,sizeof(priv->holecards[0]),priv->cardis[0],priv->cardis[0] != 0xff);
            pangea_sendcmd(hex,hn,"faceup",-1,priv->holecards[1].bytes,sizeof(priv->holecards[1]),priv->cardis[1],priv->cardis[0] != 0xff);
            dp->hand.actions[hn->client->H.slot] = CARDS777_SENTCARDS;
        }
    }
    if ( pangea_lastman(hn,dp,priv) > 0 )
    {
        pangea_sendsummary(hn,dp,priv);
        return(0);
    }
    if ( hn->client->H.slot == 0 )
    {
        for (i=0; i<dp->N; i++)
        {
            dp->hand.undergun = (dp->hand.undergun + 1) % dp->N;
            if ( dp->hand.undergun == dp->hand.lastbettor )
            {
                printf("all players queried with showdown handmask.%x finished.%u\n",dp->hand.handmask,dp->hand.finished);
                return(0);
            }
            if ( dp->hand.betstatus[dp->hand.undergun] != CARDS777_FOLD )
                break;
        }
        printf("senderind.%d host sends showdown for undergun.%d\n",senderind,dp->hand.undergun);
        pangea_sendcmd(hex,hn,"showdown",-1,(void *)&dp->hand.betsize,sizeof(dp->hand.betsize),cardi,dp->hand.undergun);
    }
    return(0);
}

char *pangea_input(uint64_t my64bits,uint64_t tableid,cJSON *json)
{
    char *actionstr; uint64_t sum,amount=0; int32_t action,num,threadid; struct pangea_info *sp; struct cards777_pubdata *dp; char hex[4096];
    threadid = juint(json,"threadid");
    if ( (sp= pangea_threadtables(&num,threadid,tableid)) == 0 )
        return(clonestr("{\"error\":\"you are not playing on any tables\"}"));
    if ( 0 && num != 1 )
        return(clonestr("{\"error\":\"more than one active table\"}"));
    else if ( (dp= sp->dp) == 0 )
        return(clonestr("{\"error\":\"no pubdata ptr for table\"}"));
    else if ( dp->hand.undergun != sp->myind || dp->hand.betsize == 0 )
    {
        printf("undergun.%d threadid.%d myind.%d\n",dp->hand.undergun,sp->tp->threadid,sp->myind);
        return(clonestr("{\"error\":\"not your turn\"}"));
    }
    else if ( (actionstr= jstr(json,"action")) == 0 )
        return(clonestr("{\"error\":\"on action specified\"}"));
    else
    {
        if ( strcmp(actionstr,"check") == 0 || strcmp(actionstr,"call") == 0 || strcmp(actionstr,"bet") == 0 || strcmp(actionstr,"raise") == 0 || strcmp(actionstr,"allin") == 0 || strcmp(actionstr,"fold") == 0 )
        {
            sum = dp->hand.bets[sp->myind];
            if ( strcmp(actionstr,"allin") == 0 )
                amount = dp->balances[sp->myind], action = CARDS777_ALLIN;
            else if ( strcmp(actionstr,"bet") == 0 )
                amount = j64bits(json,"amount"), action = 1;
            else
            {
                if ( dp->hand.betsize == sum )
                {
                    if ( strcmp(actionstr,"check") == 0 || strcmp(actionstr,"call") == 0 )
                        action = 0;
                    else if ( strcmp(actionstr,"raise") == 0 )
                    {
                        action = 1;
                        if ( (amount= dp->hand.lastraise) < j64bits(json,"amount") )
                            amount = j64bits(json,"amount");
                    }
                    else printf("unsupported userinput command.(%s)\n",actionstr);
                }
                else
                {
                    if ( strcmp(actionstr,"check") == 0 || strcmp(actionstr,"call") == 0 )
                        action = 1, amount = (dp->hand.betsize - sum);
                    else if ( strcmp(actionstr,"raise") == 0 )
                    {
                        action = 2;
                        amount = (dp->hand.betsize - sum);
                        if ( amount < dp->hand.lastraise )
                            amount = dp->hand.lastraise;
                        if ( j64bits(json,"amount") > amount )
                            amount = j64bits(json,"amount");
                    }
                    else if ( strcmp(actionstr,"fold") == 0 )
                        action = 0;
                    else printf("unsupported userinput command.(%s)\n",actionstr);
                }
            }
            if ( amount > dp->balances[sp->myind] )
                amount = dp->balances[sp->myind], action = CARDS777_ALLIN;
            pangea_sendcmd(hex,&sp->tp->hn,"action",-1,(void *)&amount,sizeof(amount),dp->hand.cardi,action);
            printf("ACTION.(%s)\n",hex);
            return(clonestr("{\"result\":\"action submitted\"}"));
        }
        else return(clonestr("{\"error\":\"illegal action specified, must be: check, call, bet, raise, fold or allin\"}"));
    }
}
