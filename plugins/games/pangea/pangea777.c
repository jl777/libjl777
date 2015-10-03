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

#define BUNDLED
#define PLUGINSTR "pangea"
#define PLUGNAME(NAME) pangea ## NAME
#define STRUCTNAME struct PLUGNAME(_info) 
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
//#include "../../utils/huffstream.c"
#include "../../common/hostnet777.c"
#include "../../includes/cJSON.h"
#include "../../agents/plugin777.c"
#undef DEFINES_ONLY

#include "../../includes/InstantDEX_quote.h"
#include "../../utils/curve25519.h"

#define _PANGEA_MAXTHREADS 9
#define PANGEA_MINRAKE_MILLIS 5
#define PANGEA_USERTIMEOUT 60
#define PANGEA_MAX_HOSTRAKE 10

struct pangea_info
{
    uint32_t timestamp,numaddrs; uint64_t basebits,bigblind,ante,addrs[CARDS777_MAXPLAYERS],tableid; char base[16]; int32_t myind;
    struct pangea_thread *tp; struct cards777_privdata *priv; struct cards777_pubdata *dp;
} *TABLES[100];

struct pangea_thread
{
    union hostnet777 hn; uint64_t nxt64bits; int32_t threadid,ishost,M,N,numcards;
} *THREADS[_PANGEA_MAXTHREADS];

int32_t PANGEA_MAXTHREADS = _PANGEA_MAXTHREADS;
//uint64_t Pangea_waiting,Pangea_userinput_betsize; uint32_t Pangea_userinput_starttime; int32_t Pangea_userinput_cardi; char Pangea_userinput[128];

char *clonestr(char *);
uint64_t stringbits(char *str);

uint32_t set_handstr(char *handstr,uint8_t cards[7],int32_t verbose);
int32_t cardstr(char *cardstr,uint8_t card);
int32_t init_hexbytes_noT(char *hexbytes,uint8_t *message,long len);
bits256 issue_getpubkey(int32_t *haspubkeyp,char *acct);
uint64_t set_account_NXTSECRET(void *myprivkey,void *mypubkey,char *NXTacct,char *NXTaddr,char *secret,int32_t max,cJSON *argjson,char *coinstr,char *serverport,char *userpass);


#define PANGEA_COMMANDS "start", "newtable", "status", "turn"

char *PLUGNAME(_methods)[] = { PANGEA_COMMANDS };
char *PLUGNAME(_pubmethods)[] = { PANGEA_COMMANDS };
char *PLUGNAME(_authmethods)[] = { PANGEA_COMMANDS };

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *argjson)
{
    uint64_t disableflags = 0;
    return(disableflags); // set bits corresponding to array position in _methods[]
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    return(retcode);
}

bits256 pangea_destpub(uint64_t destbits)
{
    int32_t i,haspubkey; bits256 destpub; char destNXT[64];
    memset(destpub.bytes,0,sizeof(destpub));
    for (i=0; i<sizeof(TABLES)/sizeof(*TABLES); i++)
        if ( TABLES[i] != 0 && TABLES[i]->tp->nxt64bits == destbits )
        {
            destpub = TABLES[i]->tp->hn.client->H.pubkey;
            break;
        }
    if ( i == sizeof(TABLES)/sizeof(*TABLES) )
    {
        expand_nxt64bits(destNXT,destbits);
        destpub = issue_getpubkey(&haspubkey,destNXT);
    }
    return(destpub);
}

struct pangea_info *pangea_find(uint64_t tableid,int32_t threadid)
{
    int32_t i;
    for (i=0; i<sizeof(TABLES)/sizeof(*TABLES); i++)
        if ( TABLES[i] != 0 && tableid == TABLES[i]->tableid && TABLES[i]->tp->threadid == threadid )
            return(TABLES[i]);
    return(0);
}

void pangea_free(struct pangea_info *sp)
{
    int32_t i;
    for (i=0; i<sizeof(TABLES)/sizeof(*TABLES); i++)
        if ( TABLES[i] == sp )
        {
            TABLES[i] = 0;
            break;
        }
    printf("PANGEA PURGE %llu\n",(long long)sp->tableid);
    free(sp);
}

void pangea_sendcmd(char *hex,union hostnet777 *hn,char *cmdstr,int32_t destplayer,uint8_t *data,int32_t datalen,int32_t cardi,int32_t turni)
{
    int32_t n,hexlen,blindflag = 0; uint64_t destbits; bits256 destpub; cJSON *json; struct cards777_pubdata *dp = hn->client->H.pubdata;
    sprintf(hex,"{\"cmd\":\"%s\",\"millitime\":\"%lld\",\"state\":%u,\"turni\":%d,\"myind\":%d,\"cardi\":%d,\"dest\":%d,\"sender\":\"%llu\",\"timestamp\":\"%lu\",\"n\":%u,\"data\":\"",cmdstr,(long long)hostnet777_convmT(&hn->client->H.mT,0),hn->client->H.state,turni,hn->client->H.slot,cardi,destplayer,(long long)hn->client->H.nxt64bits,time(NULL),datalen);
    if ( data != 0 && datalen != 0 )
    {
        n = (int32_t)strlen(hex);
        init_hexbytes_noT(&hex[n],data,datalen);
    }
    strcat(hex,"\"}");
    if ( (json= cJSON_Parse(hex)) == 0 )
    {
        printf("error creating json\n");
        return;
    }
    free_json(json);
    hexlen = (int32_t)strlen(hex)+1;
    //printf("HEX.[%s] hexlen.%d n.%d\n",hex,hexlen,datalen);
    if ( destplayer < 0 )
    {
        destbits = 0;
        memset(destpub.bytes,0,sizeof(destpub));
    }
    else
    {
        destpub = dp->playerpubs[destplayer];
        destbits = acct777_nxt64bits(destpub);
    }
    hostnet777_msg(destbits,destpub,hn,blindflag,hex,hexlen);
}

int32_t pangea_bet(union hostnet777 *hn,struct cards777_pubdata *dp,int32_t player,int64_t bet)
{
    int32_t retval = 0; uint64_t sum;
    player %= dp->N;
    if ( Debuglevel > 2 )
        printf("PANGEA_BET[%d] <- %.8f\n",player,dstr(bet));
    if ( dp->hand.betstatus[player] == CARDS777_ALLIN )
        return(CARDS777_ALLIN);
    else if ( dp->hand.betstatus[player] == CARDS777_FOLD )
        return(CARDS777_FOLD);
    if ( bet > 0 && bet >= dp->balances[player] )
    {
        bet = dp->balances[player];
        dp->hand.betstatus[player] = retval = CARDS777_ALLIN;
    }
    else
    {
        if ( bet > dp->hand.betsize && bet > dp->hand.lastraise && bet < (dp->hand.lastraise<<1) )
        {
            printf("pangea_bet %.8f not double %.8f, clip to lastraise\n",dstr(bet),dstr(dp->hand.lastraise));
            bet = dp->hand.lastraise;
        }
    }
    sum = dp->hand.bets[player];
    if ( sum+bet < dp->hand.betsize && retval != CARDS777_ALLIN )
    {
        dp->hand.betstatus[player] = retval = CARDS777_FOLD;
        if ( Debuglevel > 2 )
            printf("player.%d betsize %.8f < hand.betsize %.8f FOLD\n",player,dstr(bet),dstr(dp->hand.betsize));
        return(-1);
    }
    else if ( bet >= 2*dp->hand.lastraise )
        dp->hand.lastraise = bet, dp->hand.numactions = 0; // allows all players to check/bet again
    dp->balances[player] -= bet;
    sum += bet;
    if ( sum > dp->hand.betsize )
    {
        dp->hand.betsize = sum, dp->hand.lastbettor = player;
        if ( sum > dp->hand.lastraise && retval == CARDS777_ALLIN )
            dp->hand.lastraise = sum;
    }
    dp->hand.bets[player] += bet;
    return(retval);
}

int32_t pangea_newhand(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen)
{
    int32_t i,j; char *nrs; bits256 *final,*cardpubs; char hex[1024];
    if ( data == 0 || datalen != (dp->numcards + 1) * sizeof(bits256) )
    {
        printf("pangea_newhand invalid datalen.%d vs %ld\n",datalen,(dp->numcards + 1) * sizeof(bits256));
        return(-1);
    }
    printf("NEWHAND\n");
    priv->hole[0] = priv->hole[1] = 0xff;
    memset(priv->holecards,0,sizeof(priv->holecards));
    dp->numhands++;
    dp->button++;
    if ( dp->button >= dp->N )
        dp->button = 0;
    final = dp->hand.final, cardpubs = dp->hand.cardpubs;
    memset(&dp->hand,0,sizeof(dp->hand));
    dp->hand.final = final, dp->hand.cardpubs = cardpubs;
    for (i=0; i<5; i++)
        dp->hand.community[i] = 0xff;
    memcpy(dp->hand.cardpubs,data,(dp->numcards + 1) * sizeof(bits256));
    dp->hand.checkprod = cards777_pubkeys(dp->hand.cardpubs,dp->numcards,dp->hand.cardpubs[dp->numcards]);
    //printf("player.%d (%llx vs %llx) got cardpubs.%llx\n",hn->client->H.slot,(long long)hn->client->H.pubkey.txid,(long long)dp->playerpubs[hn->client->H.slot].txid,(long long)dp->checkprod.txid);
    if ( (nrs= jstr(json,"sharenrs")) != 0 )
        decode_hex(dp->hand.sharenrs,(int32_t)strlen(nrs)>>1,nrs);
    for (i=0; i<dp->N; i++)
        if ( dp->balances[i] <= 0 )
            dp->hand.betstatus[i] = CARDS777_FOLD;
    if ( dp->ante != 0 )
    {
        for (i=0; i<dp->N; i++)
        {
            if ( i != dp->button && i != (dp->button+1) % dp->N )
            {
                if ( dp->balances[i] < dp->ante )
                    dp->hand.betstatus[i] = CARDS777_FOLD;
                else pangea_bet(hn,dp,i,dp->ante);
            }
        }
    }
    for (i=0; i<dp->N; i++)
    {
        j = (dp->button + i) % dp->N;
        if ( dp->balances[j] < (dp->bigblind >> 1) )
            dp->hand.betstatus[j] = CARDS777_FOLD;
        else
        {
            dp->button = j;
            pangea_bet(hn,dp,dp->button,(dp->bigblind>>1));
            break;
        }
    }
    for (i=1; i<dp->N; i++)
    {
        j = (dp->button + i) % dp->N;
        if ( dp->balances[j] < dp->bigblind )
            dp->hand.betstatus[j] = CARDS777_FOLD;
        else
        {
            pangea_bet(hn,dp,j,dp->bigblind);
            break;
        }
    }
    pangea_sendcmd(hex,hn,"gotdeck",-1,dp->hand.checkprod.bytes,sizeof(uint64_t),dp->hand.cardi,dp->hand.userinput_starttime);
    return(0);
}

int32_t pangea_card(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t cardi)
{
    int32_t destplayer,card; bits256 cardpriv; char hex[1024],cardAstr[8],cardBstr[8];
    if ( data == 0 || datalen != sizeof(bits256) )
    {
        printf("pangea_card invalid datalen.%d vs %ld\n",datalen,sizeof(bits256));
        return(-1);
    }
    //printf("pangea_card priv.%llx\n",(long long)hn->client->H.privkey.txid);
    destplayer = juint(json,"dest");
    if ( (card= cards777_checkcard(&cardpriv,cardi,hn->client->H.slot,destplayer,hn->client->H.privkey,dp->hand.cardpubs,dp->numcards,*(bits256 *)data)) >= 0 )
    {
        if ( Debuglevel > 2 )
            printf("player.%d got card.[%d]\n",hn->client->H.slot,card);
        memcpy(&priv->incards[cardi*dp->N + destplayer],cardpriv.bytes,sizeof(bits256));
    } else printf("ERROR player.%d got no card %llx\n",hn->client->H.slot,*(long long *)data);
    priv->holecards[cardi / dp->N] = cardpriv, priv->hole[cardi / dp->N] = cardpriv.bytes[1];
    cardAstr[0] = cardBstr[0] = 0;
    if ( priv->hole[0] != 0xff )
        cardstr(cardAstr,priv->hole[0]);
    if ( priv->hole[1] != 0xff )
        cardstr(cardBstr,priv->hole[1]);
    printf(">>>>>>>>>> dest.%d priv.%p holecards[%d] cardi.%d / dp->N %d (%d %d) -> (%s %s)\n",destplayer,priv,priv->hole[cardi / dp->N],cardi,dp->N,priv->hole[0],priv->hole[1],cardAstr,cardBstr);
    if ( cardi < dp->N*2 )
        pangea_sendcmd(hex,hn,"facedown",-1,(void *)&cardi,sizeof(cardi),cardi,-1);
    else pangea_sendcmd(hex,hn,"faceup",-1,cardpriv.bytes,sizeof(cardpriv),cardi,-1);
    return(0);
}

int32_t pangea_preflop(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen)
{
    char hex[2 * CARDS777_MAXPLAYERS * CARDS777_MAXCARDS * sizeof(bits256)]; int32_t i,card,iter,cardi,destplayer; bits256 cardpriv,decoded;
    if ( data == 0 || datalen != (2 * dp->N) * (dp->N * sizeof(bits256)) )
    {
        printf("pangea_preflop invalid datalen.%d vs %ld\n",datalen,(2 * dp->N) * sizeof(bits256));
        return(-1);
    }
    printf("preflop player.%d\n",hn->client->H.slot);
    memcpy(priv->incards,data,datalen);
    if ( hn->client->H.slot > 1 )
    {
        //for (i=0; i<dp->numcards*dp->N; i++)
        //    printf("%llx ",(long long)priv->outcards[i].txid);
        //printf("player.%d outcards\n",hn->client->H.slot);
        for (cardi=0; cardi<dp->N*2; cardi++)
            for (destplayer=0; destplayer<dp->N; destplayer++)
            {
                if ( 0 && (card= cards777_checkcard(&cardpriv,cardi,hn->client->H.slot,destplayer,hn->client->H.privkey,dp->hand.cardpubs,dp->numcards,priv->incards[cardi*dp->N + destplayer])) >= 0 )
                    printf("ERROR: unexpected decode player.%d got card.[%d]\n",hn->client->H.slot,card);
                decoded = cards777_decode(priv->xoverz,destplayer,priv->incards[cardi*dp->N + destplayer],priv->outcards,dp->numcards,dp->N);
                *(bits256 *)&data[(cardi*dp->N + destplayer) * sizeof(bits256)] = decoded;
            }
        pangea_sendcmd(hex,hn,"preflop",hn->client->H.slot-1,data,datalen,dp->N * 2 * dp->N,-1);
    }
    else
    {
        for (iter=cardi=0; iter<2; iter++)
            for (i=0; i<dp->N; i++,cardi++)
            {
                destplayer = (dp->button + i) % dp->N;
                decoded = cards777_decode(priv->xoverz,destplayer,priv->incards[cardi*dp->N + destplayer],priv->outcards,dp->numcards,dp->N);
                //printf("[%llx -> %llx] ",*(long long *)&data[(cardi*dp->N + destplayer) * sizeof(bits256)],(long long)decoded.txid);
                if ( destplayer == hn->client->H.slot )
                    pangea_card(hn,json,dp,priv,decoded.bytes,sizeof(bits256),cardi);
                else pangea_sendcmd(hex,hn,"card",destplayer,decoded.bytes,sizeof(bits256),cardi,-1);
            }
    }
    return(0);
}

int32_t pangea_encoded(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen)
{
    char *hex;
    if ( data == 0 || datalen != (dp->numcards * dp->N) * sizeof(bits256) )
    {
        printf("pangea_encode invalid datalen.%d vs %ld (%s)\n",datalen,(dp->numcards * dp->N) * sizeof(bits256),jprint(json,0));
        return(-1);
    }
    cards777_encode(priv->outcards,priv->xoverz,priv->allshares,priv->myshares,dp->hand.sharenrs,dp->M,(void *)data,dp->numcards,dp->N);
    //int32_t i; for (i=0; i<dp->numcards*dp->N; i++)
    //    printf("%llx ",(long long)priv->outcards[i].txid);
    printf("player.%d encodes into %p %llx -> %llx\n",hn->client->H.slot,priv->outcards,*(uint64_t *)data,(long long)priv->outcards[0].txid);
    hex = malloc(65536);
    if ( hn->client->H.slot < dp->N-1 )
    {
        printf("send encoded\n");
        pangea_sendcmd(hex,hn,"encoded",hn->client->H.slot+1,priv->outcards[0].bytes,datalen,dp->N*dp->numcards,-1);
    }
    else
    {
        pangea_sendcmd(hex,hn,"final",-1,priv->outcards[0].bytes,datalen,dp->N*dp->numcards,-1);
        pangea_preflop(hn,json,dp,priv,priv->outcards[0].bytes,(2 * dp->N) * (dp->N * sizeof(bits256)));
    }
    free(hex);
    return(0);
}

int32_t pangea_final(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen)
{
    if ( data == 0 || datalen != (dp->numcards * dp->N) * sizeof(bits256) )
    {
        printf("pangea_final invalid datalen.%d vs %ld\n",datalen,(dp->numcards * dp->N) * sizeof(bits256));
        return(-1);
    }
    //if ( Debuglevel > 2 )
        printf("player.%d final into %p\n",hn->client->H.slot,priv->outcards);
    memcpy(dp->hand.final,data,sizeof(bits256) * dp->N * dp->numcards);
    if ( hn->client->H.slot == dp->N-1 )
        memcpy(priv->incards,data,sizeof(bits256) * dp->N * dp->numcards);
    return(0);
}

void pangea_startbets(union hostnet777 *hn,struct cards777_pubdata *dp,int32_t cardi)
{
    uint32_t now; char hex[1024];
    now = (uint32_t)time(NULL);
    dp->hand.undergun = ((dp->button + 2) % dp->N);
    dp->hand.numactions = 0;
    pangea_sendcmd(hex,hn,"turn",-1,(void *)&now,sizeof(now),cardi,dp->hand.undergun);
}

int32_t pangea_decode(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen)
{
    int32_t cardi,destplayer,card; bits256 cardpriv,decoded; char hex[1024];
    if ( data == 0 || datalen != sizeof(bits256) )
    {
        printf("pangea_decode invalid datalen.%d vs %ld\n",datalen,sizeof(bits256));
        return(-1);
    }
    cardi = juint(json,"cardi");
    destplayer = 0;//juint(json,"dest");
    memcpy(&priv->incards[cardi*dp->N + destplayer],data,sizeof(bits256));
    if ( hn->client->H.slot == 0 && (card= cards777_checkcard(&cardpriv,cardi,hn->client->H.slot,destplayer,hn->client->H.privkey,dp->hand.cardpubs,dp->numcards,*(bits256 *)data)) >= 0 && hn->client->H.slot > 0 )
        printf("ERROR: unexpected decode player.%d got card.[%d]\n",hn->client->H.slot,card);
    decoded = cards777_decode(priv->xoverz,destplayer,priv->incards[cardi*dp->N + destplayer],priv->outcards,dp->numcards,dp->N);
    if ( hn->client->H.slot > 0 )
    {
        pangea_sendcmd(hex,hn,"decoded",hn->client->H.slot-1,decoded.bytes,sizeof(decoded),cardi,-1);
        //printf("player.%d decoded cardi.%d %llx -> %llx\n",hn->client->H.slot,cardi,(long long)priv->incards[cardi*dp->N + destplayer].txid,(long long)decoded.txid);
    }
    else
    {
        if ( Debuglevel > 2 )
            printf("decoded community cardi.%d [%d]\n",cardi,card);
        pangea_sendcmd(hex,hn,"faceup",-1,cardpriv.bytes,sizeof(cardpriv),cardi,-1);
        if ( cardi >= 2*dp->N+2 && cardi < 2*dp->N+5 )
            pangea_startbets(hn,dp,++cardi);
    }
    return(0);
}

int32_t pangea_facedown(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t cardi)
{
    int32_t destplayer,i,n = 0;
    if ( data == 0 || datalen != sizeof(int32_t) )
    {
        printf("pangea_facedown invalid datalen.%d vs %ld\n",datalen,sizeof(bits256));
        return(-1);
    }
    destplayer = juint(json,"myind");
    dp->hand.havemasks[destplayer] |= (1LL << cardi);
    for (i=0; i<dp->N; i++)
    {
        if ( Debuglevel > 2 )
            printf("%llx ",(long long)dp->hand.havemasks[i]);
        if ( bitweight(dp->hand.havemasks[i]) == 2 )
            n++;
    }
    if ( Debuglevel > 2 )
        printf(" | player.%d sees that destplayer.%d got cardi.%d | %llx | n.%d\n",hn->client->H.slot,juint(json,"myind"),cardi,(long long)dp->hand.havemasks[destplayer],n);
    if ( hn->client->H.slot == 0 && n == dp->N )
        pangea_startbets(hn,dp,dp->N*2);
    return(0);
}

int32_t pangea_faceup(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen)
{
    int32_t cardi; char hexstr[65];
    if ( data == 0 || datalen != sizeof(bits256) )
    {
        printf("pangea_faceup invalid datalen.%d vs %ld\n",datalen,(dp->numcards + 1) * sizeof(bits256));
        return(-1);
    }
    init_hexbytes_noT(hexstr,data,sizeof(bits256));
    cardi = juint(json,"cardi");
    //if ( Debuglevel > 2 || hn->client->H.slot == 0 )
        printf("player.%d COMMUNITY.[%d] (%s) cardi.%d\n",hn->client->H.slot,data[1],hexstr,cardi);
    dp->hand.community[cardi - 2*dp->N] = data[1];
    return(0);
}

int32_t pangea_newdeck(union hostnet777 *src)
{
    uint8_t data[(CARDS777_MAXCARDS + 1) * sizeof(bits256)]; struct cards777_pubdata *dp; char nrs[512]; bits256 destpub;//,privs[9];
    struct cards777_privdata *priv; int32_t n,hexlen,len,state = 0;
    dp = src->client->H.pubdata;
    priv = src->client->H.privdata;
    memset(dp->hand.sharenrs,0,sizeof(dp->hand.sharenrs));
    init_sharenrs(dp->hand.sharenrs,0,dp->N,dp->N);
    //for (i=0; i<dp->N; i++)
    //    privs[i] = THREADS[i]->hn.server->H.privkey;
    dp->hand.checkprod = dp->hand.cardpubs[dp->numcards] = cards777_initdeck(priv->outcards,dp->hand.cardpubs,dp->numcards,dp->N,dp->playerpubs,0);
    init_hexbytes_noT(nrs,dp->hand.sharenrs,dp->N);
    len = (dp->numcards + 1) * sizeof(bits256);
    sprintf(dp->newhand,"{\"cmd\":\"%s\",\"millitime\":\"%lld\",\"state\":%u,\"sender\":\"%llu\",\"timestamp\":\"%lu\",\"sharenrs\":\"%s\",\"n\":%u,\"data\":\"","newhand",(long long)hostnet777_convmT(&src->server->H.mT,0),state,(long long)src->client->H.nxt64bits,time(NULL),nrs,len);
    n = (int32_t)strlen(dp->newhand);
    memcpy(data,dp->hand.cardpubs,len);
    init_hexbytes_noT(&dp->newhand[n],data,len);
    strcat(dp->newhand,"\"}");
    hexlen = (int32_t)strlen(dp->newhand)+1;
    memset(destpub.bytes,0,sizeof(destpub));
    hostnet777_msg(0,destpub,src,0,dp->newhand,hexlen);
    dp->startdecktime = (uint32_t)time(NULL);
    printf("NEWDECK encode.%llx numhands.%d\n",(long long)priv->outcards[0].txid,dp->numhands);
    return(state);
}

void pangea_serverstate(union hostnet777 *hn,struct cards777_pubdata *dp,struct cards777_privdata *priv)
{
    int32_t i,hexlen; bits256 destpub;
    if ( dp->newhand[0] != 0 )
    {
        for (i=0; i<dp->N; i++)
        {
            if ( dp->othercardpubs[i] != dp->hand.checkprod.txid )
                break;
        }
        if ( i == dp->N )
        {
            printf("SERVERSTATE issues encoded\n");
            pangea_sendcmd(dp->newhand,hn,"encoded",1,priv->outcards[0].bytes,sizeof(bits256)*dp->N*dp->numcards,dp->N*dp->numcards,-1);
            dp->newhand[0] = 0;
        }
        else if ( time(NULL) > dp->startdecktime+10 )
        {
            hexlen = (int32_t)strlen(dp->newhand)+1;
            memset(destpub.bytes,0,sizeof(destpub));
            hostnet777_msg(0,destpub,hn,0,dp->newhand,hexlen);
            dp->startdecktime = (uint32_t)time(NULL);
            printf("RESEND NEWDECK encode.%llx numhands.%d\n",(long long)priv->outcards[0].txid,dp->numhands);
        }
    }
}

int32_t pangea_gotdeck(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen)
{
    int32_t senderind;
    senderind = juint(json,"myind");
    dp->othercardpubs[senderind] = *(uint64_t *)data;
    printf("player.%d got pangea_gotdeck from senderind.%d\n",hn->client->H.slot,senderind);
    if ( hn->client->H.slot == 0 )
        pangea_serverstate(hn,dp,priv);
    return(0);
}

int32_t pangea_ready(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen)
{
    int32_t senderind;
    senderind = juint(json,"myind");
    dp->readymask |= (1 << senderind);
    printf("player.%d got ready from senderind.%d readymask.%x\n",hn->client->H.slot,senderind,dp->readymask);
    if ( hn->client->H.slot == 0 && (dp->readymask == ((1 << dp->N) - 1)) )
    {
        printf("send newdeck\n");
        pangea_newdeck(hn);
    }
    return(0);
}

int32_t pangea_ping(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen)
{
    int32_t senderind;
    senderind = juint(json,"myind");
    dp->othercardpubs[senderind] = *(uint64_t *)data;
    if ( senderind == 0 )
    {
        dp->hand.undergun = juint(json,"turni");
        dp->hand.cardi = juint(json,"cardi");
    }
    //printf("GOTPING.(%s) %llx\n",jprint(json,0),(long long)dp->othercardpubs[senderind]);
    return(0);
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
    cardstr(cardAstr,holecards[0]);
    cardstr(cardBstr,holecards[1]);
    if ( (card= holecards[0]) != 0xff )
        jaddnum(json,"cardA",card);
    if ( (card= holecards[1]) != 0xff )
        jaddnum(json,"cardB",card);
    sprintf(pairstr,"%s %s",cardAstr,cardBstr);
    jaddstr(json,"holecards",pairstr);
    jaddnum(json,"betsize",dstr(hand->betsize));
    jaddnum(json,"lastraise",dstr(hand->lastraise));
    jaddnum(json,"lastbettor",hand->lastbettor);
    jaddnum(json,"numactions",hand->numactions);
    jaddnum(json,"undergun",hand->undergun);
    jaddnum(json,"isbot",isbot);
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
    int32_t i,countdown; int64_t total; struct cards777_pubdata *dp; cJSON *bets,*array,*json = cJSON_CreateObject();
    jadd64bits(json,"tableid",sp->tableid);
    jadd64bits(json,"myind",sp->myind);
    dp = sp->dp;
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
        jaddinum(array,dstr(dp->balances[i]));
    jadd(json,"balances",array);
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
    jadd(json,"hand",pangea_handjson(&dp->hand,sp->priv->hole,dp->isbot[sp->myind]));
    if ( (countdown= pangea_countdown(dp,sp->myind)) >= 0 )
        jaddnum(json,"timeleft",countdown);
    return(json);
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

uint64_t pangea_bot(union hostnet777 *hn,struct cards777_pubdata *dp,int32_t turni,int32_t cardi,uint64_t betsize)
{
    int32_t r,action=0,n,activej; char hex[1024]; uint64_t threshold,total,sum,amount = 0;
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
            threshold = (100 * amount)/total;
            if ( r/n > threshold )
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

/*void pangea_userpoll(union hostnet777 *hn)
{
    int32_t cardi,action = -1; uint64_t amount=0,sum,betsize; char hex[1024]; struct cards777_pubdata *dp = hn->client->H.pubdata;
    cardi = Pangea_userinput_cardi;
    betsize = Pangea_userinput_betsize;
    if ( time(NULL) < Pangea_userinput_starttime+PANGEA_USERTIMEOUT && Pangea_userinput[0] == 0 )
    {
        if ( Pangea_userinput[0] != 0 )
        {
            sum = dp->hand.bets[hn->client->H.slot];
            if ( strcmp(Pangea_userinput,"allin") == 0 )
                amount = dp->balances[hn->client->H.slot], action = CARDS777_ALLIN;
            else
            {
                if ( betsize == sum )
                {
                    if ( strcmp(Pangea_userinput,"check") == 0 || strcmp(Pangea_userinput,"call") == 0 )
                        action = 0;
                    else if ( strcmp(Pangea_userinput,"bet") == 0 || strcmp(Pangea_userinput,"raise") == 0 )
                        action = 1, amount = dp->hand.lastraise;
                    else printf("unsupported userinput command.(%s)\n",Pangea_userinput);
                }
                else
                {
                    if ( strcmp(Pangea_userinput,"check") == 0 || strcmp(Pangea_userinput,"call") == 0 )
                        action = 1, amount = (betsize - sum);
                    else if ( strcmp(Pangea_userinput,"bet") == 0 || strcmp(Pangea_userinput,"raise") == 0 )
                    {
                        action = 2;
                        amount = (betsize - sum);
                        if ( amount < 2*dp->hand.lastraise )
                            amount = 2*dp->hand.lastraise;
                    }
                    else if ( strcmp(Pangea_userinput,"fold") == 0 )
                        action = 0;
                    else printf("unsupported userinput command.(%s)\n",Pangea_userinput);
                }
            }
            fprintf(stderr,"%ld ",Pangea_userinput_starttime+PANGEA_USERTIMEOUT-time(NULL));
        }
        if ( amount > dp->balances[hn->client->H.slot] )
            amount = dp->balances[hn->client->H.slot], action = CARDS777_ALLIN;
    } else action = 0;
    if ( action >= 0 )
    {
        pangea_sendcmd(hex,hn,"action",-1,(void *)&amount,sizeof(amount),cardi,action);
        memset(Pangea_userinput,0,sizeof(Pangea_userinput));
        Pangea_waiting = 0;
        Pangea_userinput_starttime = 0;
        Pangea_userinput_cardi = 0;
        Pangea_userinput_betsize = 0;
    }
}*/

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

int32_t pangea_turn(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen)
{
    uint32_t starttime; int32_t turni,cardi; uint64_t betsize=SATOSHIDEN; struct pangea_info *sp;
    if ( data == 0 )
    {
        printf("pangea_turn: null data\n");
        return(-1);
    }
    turni = juint(json,"turni");
    cardi = juint(json,"cardi");
    if ( datalen == sizeof(uint32_t) )
        memcpy(&starttime,data,sizeof(starttime)), dp->hand.starttime = starttime, betsize = dp->hand.betsize;
    else memcpy(&betsize,data,sizeof(betsize)), starttime = dp->hand.starttime;
    //if ( senderind == 0 && turni >= 0 && turni < dp->N )
    //    dp->hand.undergun = turni;
    if ( turni == hn->client->H.slot )
    {
        sp = dp->table;
        printf("%s\n",jprint(pangea_tablestatus(sp),1));
        pangea_statusprint(dp,priv,hn->client->H.slot);
        if ( dp->hand.betsize != betsize )
            printf("pangea_turn warning hand.betsize %.8f != betsize %.8f\n",dstr(dp->hand.betsize),dstr(betsize));
        if ( dp->isbot[hn->client->H.slot] != 0 )
            pangea_bot(hn,dp,turni,cardi,betsize);
        else
        {
            dp->hand.userinput_starttime = (uint32_t)time(NULL);
            dp->hand.cardi = cardi;
            dp->hand.betsize = betsize;
            fprintf(stderr,"Waiting for user input cardi.%d: ",cardi);
        }
    }
    return(0);
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
                        retsp = sp;
                 }
        }
    }
    *nump = num;
    return(retsp);
}

char *pangea_input(uint64_t my64bits,uint64_t tableid,cJSON *json)
{
    char *actionstr; uint64_t sum,amount=0; int32_t action,num; struct pangea_info *sp; struct cards777_pubdata *dp; char hex[4096];
    if ( (sp= pangea_usertables(&num,my64bits,tableid)) == 0 )
        return(clonestr("{\"error\":\"you are not playing on any tables\"}"));
    if ( num != 1 )
        return(clonestr("{\"error\":\"more than one active table\"}"));
    else if ( (dp= sp->dp) == 0 )
        return(clonestr("{\"error\":\"no pubdata ptr for table\"}"));
    else if ( dp->hand.undergun != sp->myind )
        return(clonestr("{\"error\":\"not your turn\"}"));
    else if ( (actionstr= jstr(json,"action")) == 0 )
        return(clonestr("{\"error\":\"on action specified\"}"));
    else
    {
        if ( strcmp(actionstr,"check") == 0 || strcmp(actionstr,"call") == 0 || strcmp(actionstr,"bet") == 0 || strcmp(actionstr,"raise") == 0 || strcmp(actionstr,"allin") == 0 || strcmp(actionstr,"fold") == 0 )
        {
            sum = dp->hand.bets[sp->myind];
            if ( strcmp(actionstr,"allin") == 0 )
                amount = dp->balances[sp->myind], action = CARDS777_ALLIN;
            else
            {
                if ( dp->hand.betsize == sum )
                {
                    if ( strcmp(actionstr,"check") == 0 || strcmp(actionstr,"call") == 0 )
                        action = 0;
                    else if ( strcmp(actionstr,"bet") == 0 || strcmp(actionstr,"raise") == 0 )
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
                    else if ( strcmp(actionstr,"bet") == 0 || strcmp(actionstr,"raise") == 0 )
                    {
                        action = 2;
                        amount = (dp->hand.betsize - sum);
                        if ( amount < 2*dp->hand.lastraise )
                            amount = 2*dp->hand.lastraise;
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

uint64_t pangea_winnings(uint64_t *pangearakep,uint64_t *hostrakep,uint64_t total,int32_t numwinners,int32_t rakemillis)
{
    uint64_t split,pangearake,rake;
    split = (total * (1000 - rakemillis)) / (1000 * numwinners);
    pangearake = (total - split*numwinners);
    if ( rakemillis > PANGEA_MINRAKE_MILLIS )
    {
        rake = (pangearake * (rakemillis - PANGEA_MINRAKE_MILLIS)) / rakemillis;
        pangearake -= rake;
    }
    else rake = 0;
    *hostrakep = rake;
    *pangearakep = pangearake;
    return(split);
}

int32_t pangea_sidepots(uint64_t sidepots[CARDS777_MAXPLAYERS][CARDS777_MAXPLAYERS],union hostnet777 *hn,struct cards777_pubdata *dp)
{
    int32_t i,j,nonz,n = 0; uint64_t bet,minbet = 0;
    printf("calc sidepots\n");
    for (j=0; j<dp->N; j++)
        sidepots[0][j] = dp->hand.bets[j];
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
        n++;
    }
    if ( hn->server->H.slot == 0 )//&& n > 1 )
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

int64_t pangea_splitpot(uint64_t sidepot[CARDS777_MAXPLAYERS],union hostnet777 *hn,int32_t rakemillis)
{
    int32_t winners[CARDS777_MAXPLAYERS],j,n,numwinners = 0; uint32_t bestrank,rank;
    uint64_t total = 0,bet,split,rake=0,pangearake=0;
    char handstr[128],besthandstr[128]; struct cards777_pubdata *dp;
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
        split = pangea_winnings(&pangearake,&rake,total,numwinners,rakemillis);
        dp->pangearake += pangearake;
        for (j=0; j<numwinners; j++)
            dp->balances[winners[j]] += split;
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

int32_t pangea_again(union hostnet777 *hn,struct cards777_pubdata *dp,int32_t sleepflag)
{
    int32_t i,n,activej = -1; uint64_t total = 0;
    for (i=n=0; i<dp->N; i++)
    {
        total += dp->balances[i];
        printf("(p%d %.8f) ",i,dstr(dp->balances[i]));
        if ( dp->balances[i] != 0 )
        {
            if ( activej < 0 )
                activej = i;
            n++;
        }
    }
    printf("balances %.8f [%.8f]\n",dstr(total),dstr(total + dp->hostrake + dp->pangearake));
    if ( n == 1 )
    {
        printf("Only player.%d left with %.8f | get sigs and cashout after numhands.%d\n",activej,dstr(dp->balances[activej]),dp->numhands);
        return(1);
    }
    else
    {
        if ( sleepflag != 0 )
            sleep(sleepflag);
        pangea_newdeck(hn);
        if ( sleepflag != 0 )
            sleep(sleepflag);
    }
    return(n);
}

int32_t pangea_action(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen)
{
    uint32_t now; int32_t action,cardi,i,senderind,activej; char hex[1024]; bits256 zero; uint64_t split,rake,pangearake,total,amount = 0;
    action = juint(json,"turni");
    cardi = juint(json,"cardi");
    senderind = juint(json,"myind");
    memset(zero.bytes,0,sizeof(zero));
    memcpy(&amount,data,sizeof(amount));
    pangea_bet(hn,dp,senderind,amount);
    if ( pangea_actives(&activej,dp) == 1 )
    {
        total = pangea_totalbet(dp);
        split = pangea_winnings(&pangearake,&rake,total,1,dp->rakemillis);
        dp->hostrake += rake;
        dp->pangearake += pangearake;
        dp->balances[activej] += split;
        if ( hn->server->H.slot == 0 )
        {
            printf("player.%d lastman standing, wins %.8f hostrake %.8f pangearake %.8f\n",activej,dstr(split),dstr(rake),dstr(pangearake));
            pangea_again(hn,dp,0);
        }
        return(0);
    }
    if ( hn->client->H.slot == 0 )
    {
        now = (uint32_t)time(NULL);
        while ( ++dp->hand.numactions < dp->N )
        {
            dp->hand.undergun = (dp->hand.undergun + 1) % dp->N;
            if ( dp->hand.betstatus[dp->hand.undergun] != CARDS777_FOLD && dp->hand.betstatus[dp->hand.undergun] != CARDS777_ALLIN )
                break;
        }
        if ( dp->hand.numactions < dp->N )
            pangea_sendcmd(hex,hn,"turn",-1,(void *)&dp->hand.betsize,sizeof(dp->hand.betsize),cardi,dp->hand.undergun);
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
                cardi = dp->N * 2;
                for (i=0; i<3; i++,cardi++)
                    pangea_sendcmd(hex,hn,"decoded",dp->N-1,dp->hand.final[cardi*dp->N].bytes,sizeof(dp->hand.final[cardi*dp->N]),cardi,dp->hand.undergun);
            }
            else if ( i == 3 )
            {
                cardi = dp->N * 2 + 3;
                pangea_sendcmd(hex,hn,"decoded",dp->N-1,dp->hand.final[cardi*dp->N].bytes,sizeof(dp->hand.final[cardi*dp->N]),cardi,dp->hand.undergun);
            }
            else if ( i == 4 )
            {
                cardi = dp->N * 2 + 4;
                pangea_sendcmd(hex,hn,"decoded",dp->N-1,dp->hand.final[cardi*dp->N].bytes,sizeof(dp->hand.final[cardi*dp->N]),cardi,dp->hand.undergun);
            }
            else
            {
                cardi = dp->N * 2 + 5;
                pangea_sendcmd(hex,hn,"showdown",-1,priv->holecards[0].bytes,sizeof(priv->holecards),cardi,dp->hand.undergun);
            }
        }
    }
    if ( Debuglevel > 1 || hn->client->H.slot == 0 )
    {
        printf("player.%d got pangea_action.%d for player.%d action.%d amount %.8f | numactions.%d\n%s\n",hn->client->H.slot,cardi,senderind,action,dstr(amount),dp->hand.numactions,jprint(pangea_tablestatus(dp->table),1));
    }
    return(0);
}

int32_t pangea_showdown(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen)
{
    char handstr[128],hex[1024]; int32_t rank,j,n,senderslot; bits256 hole[2],hand; uint64_t sidepots[CARDS777_MAXPLAYERS][CARDS777_MAXPLAYERS];
    senderslot = juint(json,"myind");
    hole[0] = *(bits256 *)data, hole[1] = *(bits256 *)&data[sizeof(bits256)];
    printf("showdown: sender.%d [%d] [%d] dp.%p\n",senderslot,hole[0].bytes[1],hole[1].bytes[1],dp);
    for (j=0; j<5; j++)
        hand.bytes[j] = dp->hand.community[j];
    hand.bytes[j++] = hole[0].bytes[1];
    hand.bytes[j++] = hole[1].bytes[1];
    rank = set_handstr(handstr,hand.bytes,0);
    printf("sender.%d (%s) (%d %d) rank.%x\n",senderslot,handstr,hole[0].bytes[1],hole[1].bytes[1],rank);
    dp->hand.handranks[senderslot] = rank;
    memcpy(dp->hand.hands[senderslot],hand.bytes,7);
    dp->hand.handmask |= (1 << senderslot);
    if ( dp->hand.handmask == (1 << dp->N)-1 )
    {
        memset(sidepots,0,sizeof(sidepots));
        n = pangea_sidepots(sidepots,hn,dp);
        for (j=0; j<n; j++)
            dp->hostrake += pangea_splitpot(sidepots[j],hn,dp->rakemillis);
        if ( hn->server->H.slot == 0 )
            pangea_again(hn,dp,0);
    }
    //printf("player.%d got rank %u (%s) from %d\n",hn->client->H.slot,rank,handstr,senderslot);
    if ( hn->client->H.slot != 0 && senderslot == 0 )
        pangea_sendcmd(hex,hn,"showdown",-1,priv->holecards[0].bytes,sizeof(priv->holecards),juint(json,"cardi"),dp->hand.undergun);
    return(0);
}

int32_t pangea_poll(uint64_t *senderbitsp,uint32_t *timestampp,union hostnet777 *hn)
{
    char *jsonstr,*hexstr,*cmdstr; cJSON *json; struct cards777_privdata *priv; struct cards777_pubdata *dp; int32_t len; uint8_t *buf = 0;
    *senderbitsp = 0;
    if ( hn == 0 || hn->client == 0 )
    {
        printf("null hn.%p %p\n",hn,hn!=0?hn->client:0);
        return(-1);
    }
    dp = hn->client->H.pubdata;
    priv = hn->client->H.privdata;
    if ( (jsonstr= queue_dequeue(&hn->client->H.Q,1)) != 0 )
    {
        //printf("GOT.(%s)\n",jsonstr);
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            *senderbitsp = j64bits(json,"sender");
            *timestampp = juint(json,"timestamp");
            hn->client->H.state = juint(json,"state");
            len = juint(json,"n");
            if ( (hexstr= jstr(json,"data")) != 0 && strlen(hexstr) == (len<<1) )
            {
                if ( len > sizeof(bits256)*CARDS777_MAXPLAYERS*CARDS777_MAXCARDS )
                    buf = calloc(1,len);
                else buf = calloc(1,sizeof(bits256)*CARDS777_MAXPLAYERS*CARDS777_MAXCARDS);
                decode_hex(buf,len,hexstr);
            } else printf("len.%d vs hexlen.%ld (%s)\n",len,strlen(hexstr)>>1,hexstr);
            if ( (cmdstr= jstr(json,"cmd")) != 0 )
            {
                if ( strcmp(cmdstr,"newhand") == 0 )
                    pangea_newhand(hn,json,dp,priv,buf,len);
                else if ( strcmp(cmdstr,"ping") == 0 )
                    pangea_ping(hn,json,dp,priv,buf,len);
                else if ( strcmp(cmdstr,"gotdeck") == 0 )
                    pangea_gotdeck(hn,json,dp,priv,buf,len);
                else if ( strcmp(cmdstr,"ready") == 0 )
                    pangea_ready(hn,json,dp,priv,buf,len);
                else if ( strcmp(cmdstr,"encoded") == 0 )
                    pangea_encoded(hn,json,dp,priv,buf,len);
                else if ( strcmp(cmdstr,"final") == 0 )
                    pangea_final(hn,json,dp,priv,buf,len);
                else if ( strcmp(cmdstr,"preflop") == 0 )
                    pangea_preflop(hn,json,dp,priv,buf,len);
                else if ( strcmp(cmdstr,"decoded") == 0 )
                    pangea_decode(hn,json,dp,priv,buf,len);
                else if ( strcmp(cmdstr,"card") == 0 )
                    pangea_card(hn,json,dp,priv,buf,len,juint(json,"cardi"));
                else if ( strcmp(cmdstr,"facedown") == 0 )
                    pangea_facedown(hn,json,dp,priv,buf,len,juint(json,"cardi"));
                else if ( strcmp(cmdstr,"faceup") == 0 )
                    pangea_faceup(hn,json,dp,priv,buf,len);
                else if ( strcmp(cmdstr,"turn") == 0 )
                    pangea_turn(hn,json,dp,priv,buf,len);
                else if ( strcmp(cmdstr,"action") == 0 )
                    pangea_action(hn,json,dp,priv,buf,len);
                else if ( strcmp(cmdstr,"showdown") == 0 )
                    pangea_showdown(hn,json,dp,priv,buf,len);
            }
            free_json(json);
        }
        free_queueitem(jsonstr);
    }
    if ( buf != 0 )
        free(buf);
    return(hn->client->H.state);
}

char *pangea_status(uint64_t my64bits,uint64_t tableid,cJSON *json)
{
    int32_t i,j,threadid = 0; struct pangea_info *sp; cJSON *item,*array=0,*retjson = 0;
    if ( tableid != 0 )
    {
        if ( (sp= pangea_find(tableid,threadid)) != 0 )
        {
            if ( (item= pangea_tablestatus(sp)) != 0 )
            {
                retjson = cJSON_CreateObject();
                jaddstr(retjson,"result","success");
                jadd(retjson,"table",item);
                return(jprint(retjson,1));
            }
        }
    }
    else
    {
        for (i=0; i<sizeof(TABLES)/sizeof(*TABLES); i++)
        {
            if ( (sp= TABLES[i]) != 0 )
            {
                for (j=0; j<sp->numaddrs; j++)
                    if ( sp->addrs[j] == my64bits )
                    {
                        if ( (item= pangea_tablestatus(sp)) != 0 )
                        {
                            if ( array == 0 )
                                array = cJSON_CreateArray();
                            jaddi(array,item);
                        }
                        break;
                    }
            }
        }
    }
    retjson = cJSON_CreateObject();
    if ( array == 0 )
        jaddstr(retjson,"error","no table status");
    else
    {
        jaddstr(retjson,"result","success");
        jadd(retjson,"tables",array);
    }
    return(jprint(retjson,1));
}

int32_t pangea_idle(struct plugin_info *plugin)
{
    int32_t i,n,m,pinggap = 10; uint64_t senderbits; uint32_t timestamp; struct pangea_thread *tp;
    union hostnet777 *hn; struct cards777_pubdata *dp; char hex[1024];
    while ( 1 )
    {
        //printf("pangea idle\n");
        for (i=n=m=0; i<_PANGEA_MAXTHREADS; i++)
        {
            if ( (tp= THREADS[i]) != 0 )
            {
                hn = &tp->hn;
                if ( hn->client->H.done == 0 )
                {
                    n++;
                    if ( hostnet777_idle(hn) != 0 )
                        m++;
                    pangea_poll(&senderbits,&timestamp,hn);
                    if ( hn->client->H.slot == 0 )
                        pinggap = 10;
                    if ( time(NULL) > hn->client->H.lastping + pinggap )
                    {
                        dp = hn->client->H.pubdata;
                        pangea_sendcmd(hex,hn,"ping",-1,dp->hand.checkprod.bytes,sizeof(uint64_t),dp->hand.cardi,dp->hand.undergun);
                        hn->client->H.lastping = (uint32_t)time(NULL);
                        //pangea_statusprint(dp,hn->client->H.privdata,hn->client->H.slot);
                    }
                    if ( hn->client->H.slot == 0 )
                        pangea_serverstate(hn,dp,hn->server->H.privdata);
                }
            }
        }
        if ( n == 0 )
            break;
        if ( m == 0 )
            msleep(3);
    }
    //for (i=0; i<_PANGEA_MAXTHREADS; i++)
    //    if ( THREADS[i] != 0 && Pangea_waiting != 0 )
    //        pangea_userpoll(&THREADS[i]->hn);
    return(0);
}

struct pangea_info *pangea_create(struct pangea_thread *tp,int32_t *createdflagp,char *base,uint32_t timestamp,uint64_t *addrs,int32_t numaddrs,uint64_t bigblind,uint64_t ante,uint64_t *balances,uint64_t *isbot)
{
    struct pangea_info *sp = 0; bits256 hash; int32_t i,j,numcards,firstslot = -1; struct cards777_privdata *priv; struct cards777_pubdata *dp;
    if ( createdflagp != 0 )
        *createdflagp = 0;
    for (i=0; i<numaddrs; i++)
        printf("%llu ",(long long)addrs[i]);
    printf("numaddrs.%d\n",numaddrs);
    for (i=0; i<numaddrs; i++)
    {
        if ( addrs[i] == tp->nxt64bits )
            break;
    }
    if ( i == numaddrs )
    {
        printf("this node not in addrs\n");
        return(0);
    }
    if ( numaddrs > 0 && (sp= calloc(1,sizeof(*sp))) != 0 )
    {
        sp->tp = tp;
        numcards = CARDS777_MAXCARDS;
        tp->numcards = numcards, tp->N = numaddrs;
        sp->dp = dp = cards777_allocpub((numaddrs >> 1) + 1,numcards,numaddrs);
        for (j=0; j<numaddrs; j++)
        {
            if ( balances != 0 )
                dp->balances[j] = balances[j];
            else dp->balances[j] = 100;
            if ( isbot != 0 )
                dp->isbot[j] = isbot[j];
        }
        sp->priv = priv = cards777_allocpriv(numcards,numaddrs);
        strcpy(sp->base,base);
        if ( (sp->timestamp= timestamp) == 0 )
            sp->timestamp = (uint32_t)time(NULL);
        sp->numaddrs = numaddrs;
        sp->basebits = stringbits(base);
        sp->bigblind = dp->bigblind = bigblind, sp->ante = dp->ante = ante;
        memcpy(sp->addrs,addrs,numaddrs * sizeof(sp->addrs[0]));
        vcalc_sha256(0,hash.bytes,(uint8_t *)sp,numaddrs * sizeof(sp->addrs[0]) + 4*sizeof(uint64_t));
        sp->tableid = hash.txid;
        for (i=0; i<sizeof(TABLES)/sizeof(*TABLES); i++)
        {
            if ( TABLES[i] != 0 )
            {
                if ( sp->tableid == TABLES[i]->tableid && tp->threadid == TABLES[i]->tp->threadid )
                {
                    printf("tableid %llu already exists!\n",(long long)sp->tableid);
                    free(sp);
                    return(TABLES[i]);
                }
            }
            else if ( firstslot < 0 )
                firstslot = i;
        }
        TABLES[firstslot] = sp;
        if ( createdflagp != 0 )
            *createdflagp = 1;
    }
    return(sp);
}

cJSON *pangea_ciphersjson(struct cards777_pubdata *dp,struct cards777_privdata *priv)
{
    int32_t i,j,nonz = 0; char hexstr[65]; cJSON *array = cJSON_CreateArray();
    for (i=0; i<dp->numcards; i++)
        for (j=0; j<dp->N; j++,nonz++)
        {
            init_hexbytes_noT(hexstr,priv->outcards[nonz].bytes,sizeof(bits256));
            jaddistr(array,hexstr);
        }
    return(array);
}

cJSON *pangea_playerpubs(bits256 *playerpubs,int32_t num)
{
    int32_t i; char hexstr[65]; cJSON *array = cJSON_CreateArray();
    for (i=0; i<num; i++)
    {
        init_hexbytes_noT(hexstr,playerpubs[i].bytes,sizeof(bits256));
        //printf("(%llx-> %s) ",(long long)playerpubs[i].txid,hexstr);
        jaddistr(array,hexstr);
    }
    //printf("playerpubs\n");
    return(array);
}

cJSON *pangea_cardpubs(struct cards777_pubdata *dp)
{
    int32_t i; char hexstr[65]; cJSON *array = cJSON_CreateArray();
    for (i=0; i<dp->numcards; i++)
    {
        init_hexbytes_noT(hexstr,dp->hand.cardpubs[i].bytes,sizeof(bits256));
        jaddistr(array,hexstr);
    }
    init_hexbytes_noT(hexstr,dp->hand.checkprod.bytes,sizeof(bits256));
    jaddistr(array,hexstr);
    return(array);
}

cJSON *pangea_sharenrs(uint8_t *sharenrs,int32_t n)
{
    int32_t i; cJSON *array = cJSON_CreateArray();
    for (i=0; i<n; i++)
        jaddinum(array,sharenrs[i]);
    return(array);
}

char *pangea_newtable(int32_t threadid,cJSON *json,uint64_t my64bits,bits256 privkey,bits256 pubkey,char *transport,char *ipaddr,uint16_t port)
{
    int32_t createdflag,num,i,myind= -1; uint64_t tableid,addrs[CARDS777_MAXPLAYERS],balances[CARDS777_MAXPLAYERS],isbot[CARDS777_MAXPLAYERS];
    struct pangea_info *sp; cJSON *array; struct pangea_thread *tp; char *base,*hexstr,*endpoint,hex[1024]; uint32_t timestamp;
    struct cards777_pubdata *dp; struct hostnet777_server *srv;
    printf("T%d NEWTABLE.(%s)\n",threadid,jprint(json,0));
    if ( (tableid= j64bits(json,"tableid")) != 0 && (base= jstr(json,"base")) != 0 && (timestamp= juint(json,"timestamp")) != 0 )
    {
        if ( (array= jarray(&num,json,"addrs")) == 0 || num < 2 || num > CARDS777_MAXPLAYERS )
        {
            printf("no address or illegal num.%d\n",num);
            return(clonestr("{\"error\":\"no addrs or illegal numplayers\"}"));
        }
        for (i=0; i<num; i++)
        {
            addrs[i] = j64bits(jitem(array,i),0);
            if ( addrs[i] == my64bits )
            {
                threadid = myind = i;
                if ( (tp= THREADS[threadid]) == 0 )
                {
                    THREADS[threadid] = tp = calloc(1,sizeof(*THREADS[threadid]));
                    if ( i == 0 )
                    {
                        if ( (srv= hostnet777_server(privkey,pubkey,transport,ipaddr,port,num)) == 0 )
                            printf("cant create hostnet777 server\n");
                        else
                        {
                            tp->hn.server = srv;
                            memcpy(srv->H.privkey.bytes,privkey.bytes,sizeof(bits256));
                            memcpy(srv->H.pubkey.bytes,pubkey.bytes,sizeof(bits256));
                        }
                    }
                    else
                    {
                        PANGEA_MAXTHREADS = 1;
                        if ( (endpoint= jstr(json,"pangea_endpoint")) != 0 )
                        {
                            if ( (tp->hn.client= hostnet777_client(privkey,pubkey,endpoint,i)) == 0 )
                            {
                                memcpy(tp->hn.client->H.privkey.bytes,privkey.bytes,sizeof(bits256));
                                memcpy(tp->hn.client->H.pubkey.bytes,pubkey.bytes,sizeof(bits256));
                            }
                        }
                    }
                    tp->nxt64bits = my64bits;
                }
            }
        }
        if ( myind < 0 )
            return(clonestr("{\"error\":\"this table is not for me\"}"));
        if ( (array= jarray(&num,json,"balances")) == 0 )
        {
            printf("no balances or illegal num.%d\n",num);
            return(clonestr("{\"error\":\"no balances or illegal numplayers\"}"));
        }
        for (i=0; i<num; i++)
            balances[i] = j64bits(jitem(array,i),0);
        if ( (array= jarray(&num,json,"isbot")) != 0 )
        {
            for (i=0; i<num; i++)
                isbot[i] = j64bits(jitem(array,i),0);
        }
        else memset(isbot,0,sizeof(isbot));
        printf("call pangea_create\n");
        if ( (sp= pangea_create(tp,&createdflag,base,timestamp,addrs,num,j64bits(json,"bigblind"),j64bits(json,"ante"),balances,isbot)) == 0 )
        {
            printf("cant create table.(%s) numaddrs.%d\n",base,num);
            return(clonestr("{\"error\":\"cant create table\"}"));
        }
        printf("back from pangea_create\n");
        dp = sp->dp; sp->myind = myind;
        dp->rakemillis = juint(json,"rakemillis");
        if ( dp->rakemillis > PANGEA_MAX_HOSTRAKE )
            dp->rakemillis = PANGEA_MAX_HOSTRAKE;
        dp->table = sp;
        tp->numcards = dp->numcards, tp->N = dp->N, tp->M = dp->M;
        if ( threadid == 0 )
        {
            tp->hn.server->clients[0].pubdata = dp;
            tp->hn.server->clients[0].privdata = sp->priv;
            tp->hn.server->H.pubdata = dp;
            tp->hn.server->H.privdata = sp->priv;
        }
        else
        {
            tp->hn.client->my.pubdata = dp;
            tp->hn.client->my.privdata = sp->priv;
            tp->hn.client->H.pubdata = dp;
            tp->hn.client->H.privdata = sp->priv;
            if ( THREADS[0] != 0 )
            {
                THREADS[0]->hn.server->clients[threadid].pubdata = dp;
                THREADS[0]->hn.server->clients[threadid].privdata = sp->priv;
            }
        }
        if ( (array= jarray(&num,json,"playerpubs")) == 0 || num < 2 || num > CARDS777_MAXPLAYERS )
        {
            printf("no address or illegal num.%d\n",num);
            return(clonestr("{\"error\":\"no addrs or illegal numplayers\"}"));
        }
        for (i=0; i<num; i++)
        {
            hexstr = jstr(jitem(array,i),0);
            decode_hex(dp->playerpubs[i].bytes,sizeof(bits256),hexstr);
            printf("set playerpubs.(%s) %llx\n",hexstr,(long long)dp->playerpubs[i].txid);
        }
        if ( myind >= 0 && createdflag != 0 && addrs[myind] == tp->nxt64bits )
        {
            memcpy(sp->addrs,addrs,sizeof(*addrs) * dp->N);
            dp->readymask |= (1 << sp->myind);
            pangea_sendcmd(hex,&tp->hn,"ready",-1,0,0,0,0);
            return(clonestr("{\"result\":\"newtable created\"}"));
        }
        else if ( createdflag == 0 )
        {
            if ( sp->addrs[0] == tp->nxt64bits )
                return(clonestr("{\"result\":\"this is my table\"}"));
            else return(clonestr("{\"result\":\"table already exists\"}"));
        }
    }
    return(clonestr("{\"error\":\"no tableid\"}"));
}

struct pangea_thread *pangea_threadinit(struct plugin_info *plugin,int32_t maxplayers)
{
    struct pangea_thread *tp; struct hostnet777_server *srv;
    PANGEA_MAXTHREADS = 1;
    THREADS[0] = tp = calloc(1,sizeof(*THREADS[0]));
    tp->nxt64bits = plugin->nxt64bits;
    if ( (srv= hostnet777_server(*(bits256 *)plugin->mypriv,*(bits256 *)plugin->mypub,plugin->transport,plugin->ipaddr,plugin->pangeaport,9)) == 0 )
        printf("cant create hostnet777 server\n");
    else
    {
        tp->hn.server = srv;
        memcpy(srv->H.privkey.bytes,plugin->mypriv,sizeof(bits256));
        memcpy(srv->H.pubkey.bytes,plugin->mypub,sizeof(bits256));
    }
    return(tp);
}

int32_t pangea_start(struct plugin_info *plugin,char *retbuf,char *base,uint32_t timestamp,uint64_t bigblind,uint64_t ante,int32_t rakemillis,int32_t maxplayers,cJSON *json)
{
    char *addrstr,*ciphers,*playerpubs,*balancestr,*isbotstr,destNXT[64]; struct pangea_thread *tp; struct cards777_pubdata *dp;
    int32_t createdflag,addrtype,haspubkey,i,j,slot,n,myind=-1,r,num=0,threadid=0; uint64_t addrs[512],balances[512],isbot[512],tmp;
    uint8_t p2shtype; struct pangea_info *sp; cJSON *bids,*walletitem,*item;
    memset(addrs,0,sizeof(addrs));
    memset(balances,0,sizeof(balances));
    if ( rakemillis < 0 || rakemillis > PANGEA_MAX_HOSTRAKE )
    {
        printf("illegal rakemillis.%d\n",rakemillis);
        strcpy(retbuf,"{\"error\":\"illegal rakemillis\"}");
        return(-1);
    }
    if ( bigblind == 0 )
        bigblind = SATOSHIDEN;
    if ( (tp= THREADS[threadid]) == 0 )
    {
        pangea_threadinit(plugin,maxplayers);
        if ( (tp=THREADS[0]) == 0 )
        {
            strcpy(retbuf,"{\"error\":\"uinitialized threadid\"}");
            printf("%s\n",retbuf);
            return(-1);
        }
    }
    printf("mynxt64bits.%llu base.(%s) maxplayers.%d\n",(long long)tp->nxt64bits,base,maxplayers);
    if ( base == 0 || base[0] == 0 || maxplayers < 2 || maxplayers > CARDS777_MAXPLAYERS )
    {
        sprintf(retbuf,"{\"error\":\"bad params\"}");
        printf("%s\n",retbuf);
        return(-1);
    }
    addrtype = coin777_addrtype(&p2shtype,base);
    if ( (bids= jarray(&n,json,"bids")) != 0 )
    {
        printf("numbids.%d\n",n);
        for (i=num=0; i<n; i++)
        {
            item = jitem(bids,i);
            if ( (addrs[num]= j64bits(item,"offerNXT")) != 0 && (walletitem= jobj(item,"wallet")) != 0 )
            {
                if ( j64bits(walletitem,"bigblind") == bigblind && j64bits(walletitem,"ante") == ante && juint(walletitem,"rakemillis") == rakemillis )
                {
                    balances[num] = j64bits(walletitem,"balance");
                    isbot[num] = juint(walletitem,"isbot");
                    printf("(i.%d %llu %.8f) ",i,(long long)addrs[num],dstr(balances[num]));
                    for (j=0; j<num; j++)
                        if ( addrs[j] == addrs[num] )
                            break;
                    if ( j == num )
                    {
                        if ( addrs[num] == tp->nxt64bits )
                            myind = num;
                        printf("%llu ",(long long)addrs[num]);
                        num++;
                    }
                }
                else printf("%d: %llu mismatched walletitem bigblind %.8f ante %.8f rake %.1f%%\n",i,(long long)addrs[num],dstr(j64bits(walletitem,"bigblind")),dstr(j64bits(walletitem,"ante")),(double)juint(walletitem,"rakemillis")/10.);
            }
        }
    }
    printf("(%llu) pangea_start(%s) threadid.%d myind.%d num.%d maxplayers.%d\n",(long long)tp->nxt64bits,base,tp->threadid,myind,num,maxplayers);
    if ( (i= myind) > 0 )
    {
        addrs[i] = addrs[0];
        addrs[0] = tp->nxt64bits;
        tmp = balances[i];
        balances[i] = balances[0];
        balances[0] = tmp;
        tmp = isbot[i];
        isbot[i] = isbot[0];
        isbot[0] = tmp;
        i = 0;
    }
    while ( num > maxplayers )
    {
        r = (rand() % (num-1));
        printf("swap out %d of %d\n",r+1,num);
        num--;
        isbot[r + 1] = isbot[num];
        balances[r + 1] = balances[num];
        addrs[r + 1] = addrs[num];
    }
    printf("pangea numplayers.%d\n",num);
    if ( (sp= pangea_create(tp,&createdflag,base,timestamp,addrs,num,bigblind,ante,balances,isbot)) == 0 )
    {
        printf("cant create table.(%s) numaddrs.%d\n",base,num);
        strcpy(retbuf,"{\"error\":\"cant create table\"}");
        return(-1);
    }
    printf("back from pangea_create\n");
    dp = sp->dp, dp->table = sp;
    sp->myind = myind;
    if ( createdflag != 0 && myind == 0 && addrs[myind] == tp->nxt64bits )
    {
        tp->numcards = dp->numcards, tp->N = dp->N, tp->M = dp->M;
        dp->rakemillis = juint(json,"rakemillis");
        if ( dp->rakemillis > PANGEA_MAX_HOSTRAKE )
            dp->rakemillis = PANGEA_MAX_HOSTRAKE;
        dp->rakemillis += PANGEA_MINRAKE_MILLIS;
        tp->hn.server->clients[myind].pubdata = dp;
        tp->hn.server->clients[myind].privdata = sp->priv;
        tp->hn.server->H.pubdata = dp;
        tp->hn.server->H.privdata = sp->priv;
        init_sharenrs(dp->hand.sharenrs,0,dp->N,dp->N);
        for (j=0; j<dp->N; j++)
        {
            if ( THREADS[j] != 0 )
                dp->playerpubs[j] = THREADS[j]->hn.client->H.pubkey;
            else
            {
                expand_nxt64bits(destNXT,addrs[j]);
                dp->playerpubs[j] = issue_getpubkey(&haspubkey,destNXT);
                if ( (slot= hostnet777_register(THREADS[0]->hn.server,dp->playerpubs[j],-1)) != j )
                    printf("unexpected register slot.%d for j.%d\n",slot,j);
            }
            //printf("thread[%d] pub.%llx priv.%llx\n",j,(long long)dp->playerpubs[j].txid,(long long)THREADS[j]->hn.client->H.privkey.txid);
        }
        isbotstr = jprint(addrs_jsonarray(isbot,num),1);
        balancestr = jprint(addrs_jsonarray(balances,num),1);
        addrstr = jprint(addrs_jsonarray(addrs,num),1);
        ciphers = jprint(pangea_ciphersjson(dp,sp->priv),1);
        playerpubs = jprint(pangea_playerpubs(dp->playerpubs,num),1);
        dp->readymask |= (1 << sp->myind);
        sprintf(retbuf,"{\"cmd\":\"newtable\",\"broadcast\":\"allnodes\",\"myind\":%d,\"pangea_endpoint\":\"%s\",\"plugin\":\"relay\",\"destplugin\":\"pangea\",\"method\":\"busdata\",\"submethod\":\"newtable\",\"my64bits\":\"%llu\",\"tableid\":\"%llu\",\"timestamp\":%u,\"M\":%d,\"N\":%d,\"base\":\"%s\",\"bigblind\":\"%llu\",\"rakemillis\":\"%u\",\"ante\":\"%llu\",\"playerpubs\":%s,\"addrs\":%s,\"balances\":%s,\"isbot\":%s,\"millitime\":\"%lld\"}",sp->myind,tp->hn.server->ep.endpoint,(long long)tp->nxt64bits,(long long)sp->tableid,sp->timestamp,dp->M,dp->N,sp->base,(long long)bigblind,dp->rakemillis,(long long)ante,playerpubs,addrstr,balancestr,isbotstr,(long long)hostnet777_convmT(&tp->hn.server->H.mT,0)); //\"pluginrequest\":\"SuperNET\",
#ifdef BUNDLED
        {
            char *busdata_sync(uint32_t *noncep,char *jsonstr,char *broadcastmode,char *destNXTaddr);
            char *str; uint32_t nonce;
            if ( (str= busdata_sync(&nonce,retbuf,"allnodes",0)) != 0 )
                free(str);
        }
#endif
        printf("START.(%s)\n",retbuf);
        free(addrstr), free(ciphers), free(playerpubs), free(balancestr), free(isbotstr);
    }
    return(0);
}

void pangea_test(struct plugin_info *plugin)//,int32_t numthreads,int64_t bigblind,int64_t ante,int32_t rakemillis)
{
    char retbuf[65536]; bits256 privkey,pubkey; int32_t i,slot,threadid; struct pangea_thread *tp; struct hostnet777_client **clients;
    struct hostnet777_server *srv; cJSON *item,*bids,*walletitem,*testjson = cJSON_CreateObject();
    sleep(3);
    int32_t numthreads; int64_t bigblind,ante; int32_t rakemillis;
    numthreads = 9; bigblind = SATOSHIDEN; ante = SATOSHIDEN/10; rakemillis = PANGEA_MAX_HOSTRAKE;
    plugin->sleepmillis = 1;
    PANGEA_MAXTHREADS = numthreads;
    if ( plugin->transport[0] == 0 )
        strcpy(plugin->transport,"tcp");
    if ( plugin->ipaddr[0] == 0 )
        strcpy(plugin->ipaddr,"127.0.0.1");
    if ( plugin->pangeaport == 0 )
        plugin->pangeaport = 8888;
    //if ( portable_thread_create((void *)hostnet777_idler,hn) == 0 )
    //    printf("error launching server thread\n");
    clients = calloc(numthreads,sizeof(*clients));
    for (threadid=0; threadid<PANGEA_MAXTHREADS; threadid++)
    {
        tp = THREADS[threadid] = calloc(1,sizeof(*THREADS[threadid]));
        tp->threadid = threadid;
        tp->nxt64bits = conv_NXTpassword(privkey.bytes,pubkey.bytes,(void *)&threadid,sizeof(threadid));
        if ( threadid == 0 )
        {
            if ( (srv= hostnet777_server(privkey,pubkey,0,0,0,numthreads)) == 0 )
            {
                printf("cant create hostnet777 server\n");
                return;
            }
            tp->hn.server = srv;
            clients[0] = (void *)srv;
            slot = threadid;
           // srv->H.privkey = privkey, srv->H.pubkey = pubkey;
        }
        else
        {
            if ( (slot= hostnet777_register(srv,pubkey,-1)) >= 0 && slot == threadid )
            {
                if ( (clients[threadid]= hostnet777_client(privkey,pubkey,srv->ep.endpoint,slot)) == 0 )
                    printf("error creating clients[%d]\n",threadid);
                else
                {
                    tp->hn.client = clients[threadid];
                    //tp->hn.client->H.privkey = privkey, tp->hn.client->H.pubkey = pubkey;
                    //if ( portable_thread_create((void *)hostnet777_idler,hn) == 0 )
                    //    printf("error launching clients[%d] thread\n",threadid);
                }
            } else printf("error slot.%d != threadid.%d\n",slot,threadid);
        }
        printf("%llu: slot.%d client.%p -> %llu pubkey.%llx/%llx privkey.%llx/%llx\n",(long long)tp->nxt64bits,slot,clients[threadid],(long long)clients[threadid]->H.nxt64bits,(long long)clients[threadid]->H.pubkey.txid,(long long)pubkey.txid,(long long)clients[threadid]->H.privkey.txid,(long long)privkey.txid);
    }
    bids = cJSON_CreateArray();
    for (i=0; i<numthreads; i++)
    {
        item = cJSON_CreateObject();
        walletitem = cJSON_CreateObject();
        //if ( i != 0 )
            jaddnum(walletitem,"isbot",1);
        jadd64bits(walletitem,"bigblind",bigblind);
        jadd64bits(walletitem,"ante",ante);
        jaddnum(walletitem,"rakemillis",rakemillis);
        jadd64bits(walletitem,"balance",bigblind * 100);
        jadd64bits(item,"offerNXT",THREADS[i]->nxt64bits);
        jadd(item,"wallet",walletitem);
        jaddi(bids,item);
    }
    jadd(testjson,"bids",bids);
    jadd64bits(testjson,"offerNXT",THREADS[0]->nxt64bits);
    jadd64bits(testjson,"bigblind",bigblind);
    jadd64bits(testjson,"ante",ante);
    jaddnum(testjson,"rakemillis",rakemillis);
    printf("TEST.(%s)\n",jprint(testjson,0));
    pangea_start(plugin,retbuf,"BTCD",0,bigblind,ante,rakemillis,i,testjson);
    free_json(testjson);
    testjson = cJSON_Parse(retbuf);
    //printf("BROADCAST.(%s)\n",retbuf);
    for (threadid=1; threadid<PANGEA_MAXTHREADS; threadid++)
        pangea_newtable(threadid,testjson,THREADS[threadid]->nxt64bits,THREADS[threadid]->hn.client->H.privkey,THREADS[threadid]->hn.client->H.pubkey,0,0,0);
    tp = THREADS[0];
   // pangea_newdeck(&tp->hn);
}

int32_t PLUGNAME(_process_json)(char *forwarder,char *sender,int32_t valid,struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag,char *tokenstr)
{
    char *resultstr,*methodstr,*base,*retstr = 0; int32_t maxplayers; cJSON *argjson;
    retbuf[0] = 0;
    fprintf(stderr,"<<<<<<<<<<<< INSIDE PANGEA! process %s (%s) forwarder.(%s) sender.(%s)\n",plugin->name,jsonstr,forwarder,sender);
    if ( initflag > 0 )
    {
        uint64_t conv_NXTpassword(unsigned char *mysecret,unsigned char *mypublic,uint8_t *pass,int32_t passlen);
        //PANGEA.readyflag = 1;
        plugin->sleepmillis = 1;
        plugin->allowremote = 1;
        argjson = cJSON_Parse(jsonstr);
        plugin->nxt64bits = set_account_NXTSECRET(plugin->mypriv,plugin->mypub,plugin->NXTACCT,plugin->NXTADDR,plugin->NXTACCTSECRET,sizeof(plugin->NXTACCTSECRET),argjson,0,0,0);
        free_json(argjson);
        printf("my64bits %llu ipaddr.%s mypriv.%02x mypub.%02x\n",(long long)plugin->nxt64bits,plugin->ipaddr,plugin->mypriv[0],plugin->mypub[0]);
#ifdef __APPLE__
        if ( 0 )
            portable_thread_create((void *)pangea_test,plugin);//,9,SATOSHIDEN,SATOSHIDEN/10,10);
#endif
        printf("initialized PANGEA\n");
        if ( 0 )
        {
            int32_t i; char str[8];
            for (i=0; i<52; i++)
            {
                cardstr(str,i);
                printf("(%d %s) ",i,str);
            }
            printf("cards\n");
        }
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        methodstr = jstr(json,"method");
        resultstr = jstr(json,"result");
        if ( methodstr == 0 || methodstr[0] == 0 )
        {
            printf("(%s) has not method\n",jsonstr);
            return(0);
        }
        else if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
        else if ( strcmp(methodstr,"start") == 0 )
        {
            strcpy(retbuf,"{\"result\":\"start issued\"}");
            if ( (base= jstr(json,"base")) != 0 )
            {
                if ( (maxplayers= juint(json,"maxplayers")) < 2 )
                    maxplayers = 2;
                else if ( maxplayers > CARDS777_MAXPLAYERS )
                    maxplayers = CARDS777_MAXPLAYERS;
                if ( jstr(json,"resubmit") == 0 )
                    sprintf(retbuf,"{\"resubmit\":[{\"method\":\"start\"}, {\"bigblind\":\"%llu\"}, {\"ante\":\"%llu\"}, {\"rakemillis\":\"%u\"}, {\"maxplayers\":%d}],\"pluginrequest\":\"SuperNET\",\"plugin\":\"InstantDEX\",\"method\":\"orderbook\",\"base\":\"BTCD\",\"exchange\":\"pangea\",\"allfields\":1}",(long long)j64bits(json,"bigblind"),(long long)j64bits(json,"ante"),juint(json,"rakemillis"),maxplayers);
                else pangea_start(plugin,retbuf,base,0,j64bits(json,"bigblind"),j64bits(json,"ante"),juint(json,"rakemillis"),maxplayers,json);
            } else strcpy(retbuf,"{\"error\":\"no base specified\"}");
        }
        else if ( strcmp(methodstr,"newtable") == 0 )
            retstr = pangea_newtable(juint(json,"threadid"),json,plugin->nxt64bits,*(bits256 *)plugin->mypriv,*(bits256 *)plugin->mypub,plugin->transport,plugin->ipaddr,plugin->pangeaport);
        else if ( strcmp(methodstr,"status") == 0 )
            retstr = pangea_status(plugin->nxt64bits,j64bits(json,"tableid"),json);
        //else if ( strcmp(methodstr,"turn") == 0 )
        //    retstr = pangea_input(plugin->nxt64bits,j64bits(json,"tableid"),json);
    }
    return(plugin_copyretstr(retbuf,maxlen,retstr));
}

#include "../agents/plugin777.c"
