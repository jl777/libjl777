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

char *clonestr(char *);
uint64_t stringbits(char *str);

uint32_t set_handstr(char *handstr,uint8_t cards[7],int32_t verbose);
int32_t cardstr(char *cardstr,uint8_t card);
int32_t init_hexbytes_noT(char *hexbytes,uint8_t *message,long len);
bits256 issue_getpubkey(int32_t *haspubkeyp,char *acct);
uint64_t set_account_NXTSECRET(void *myprivkey,void *mypubkey,char *NXTacct,char *NXTaddr,char *secret,int32_t max,cJSON *argjson,char *coinstr,char *serverport,char *userpass);


#define PANGEA_COMMANDS "start", "newtable"

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

/*void pangea_sendping(struct pangea_info *sp)
{
    int32_t cardi,j,permiflag,ind,sendsock,i,mofn = 0; uint64_t mask[3]; char buf[65],*pingstr; bits256 hash;
    cJSON *array,*json = cJSON_CreateObject();
    player = &sp->player;
    jaddstr(json,"cmd","ping");
    jaddnum(json,"myind",player->myind);
    jaddnum(json,"button",sp->deck.button);
    jaddnum(json,"M",sp->deck.M);
    jaddnum(json,"N",sp->deck.M);
    jadd64bits(json,"tableid",(long long)sp->tableid);
    calc_sha256(buf,hash.bytes,(void *)sp->deck.final,sizeof(sp->deck.final));
    jaddstr(json,"final",buf);
    mofn = (1 << player->myind);
    for (i=0; i<sp->deck.numplayers; i++)
        if ( player->mofn[i] != 0 )
            mofn |= (1 << i);
    jaddnum(json,"mofn",mofn);
    jadd64bits(json,"rawprod",(long long)sp->deck.checkprod.txid);
    jadd64bits(json,"permiprod",(long long)sp->deck.permiprod.txid);
    jaddnum(json,"hole",(player->hole[0] != 0xff) | ((player->hole[1] != 0xff) << 1));
    if ( strncmp(player->ep.endpoint,"tcp://127.0.0.1",strlen("tcp://127.0.0.1")) == 0 )
    {
        if ( player->hole[0] != 0xff )
            cardstr(buf,player->hole[0]), jaddstr(json,"holeA",buf);
        if ( player->hole[1] != 0xff )
            cardstr(buf,player->hole[1]), jaddstr(json,"holeB",buf);
    }
    for (permiflag=0; permiflag<2; permiflag++)
    {
        memset(mask,0,sizeof(mask));
        for (cardi=ind=0; cardi<sp->deck.numplayers*2; cardi++)
        {
            for (j=0; j<sp->deck.numplayers; j++,ind++)
                if ( sp->player.decode[permiflag][cardi].toplayer[j].txid != 0 )
                    SETBIT(mask,ind);
        }
        init_hexbytes_noT(buf,(void *)mask,sizeof(mask));
        jaddstr(json,(permiflag == 0) ? "raw" : "permi",buf);
    }
    array = cJSON_CreateArray();
    for (i=0; i<5; i++)
        if ( sp->deck.faceups[i] != 0xff )
        {
            cardstr(buf,sp->deck.faceups[i]);
            jaddistr(array,buf);
        }
    jadd(json,"cards",array);
    array = cJSON_CreateArray();
    for (i=0; i<player->numbets; i++)
        jaddinum(array,dstr(player->bets[i]));
    jadd(json,"bets",array);
    array = cJSON_CreateArray();
    for (i=0; i<player->numpots; i++)
        jaddinum(array,dstr(player->pots[i]));
    jadd(json,"pots",array);
    array = cJSON_CreateArray();
    for (i=0; i<sp->deck.numpots; i++)
        jaddinum(array,dstr(sp->deck.pots[i]));
    jadd(json,"tablepots",array);
    array = cJSON_CreateArray();
    for (i=0; i<sp->deck.numplayers; i++)
        jaddinum(array,player->state.states[i]);
    jadd(json,"states",array);
    array = cJSON_CreateArray();
    for (i=1; i<PANGEA_MAXSTATE; i++)
    {
        sprintf(buf,"%x",player->state.masks[i]);
        jaddistr(array,buf);
    }
    jadd(json,"statemasks",array);
    jaddnum(json,"layers",sp->player.state.layers);
    jaddnum(json,"state",player->state.mystate);
    jaddnum(json,"lastping",player->ep.lastping);
    pingstr = jprint(json,1);
    //pangea_send(player->ep.threadid,sendsock,pingstr,(int32_t)strlen(pingstr)+1);
    free(pingstr);
}

int32_t pangea_cmd(struct pangea_info *sp,char *cmdstr,cJSON *json)
{
    return(0);
}*/

bits256 pangea_pubkeys(cJSON *json,struct cards777_pubdata *dp)
{
    int32_t i; bits256 bp,pubkey,hash,check; bits320 prod,hexp; // cJSON *array; char *hexstr;
    memset(check.bytes,0,sizeof(check));
    memset(bp.bytes,0,sizeof(bp)), bp.bytes[0] = 9;
    //if ( (array= jarray(&n,json,str)) != 0 )
    {
        prod = fmul(fexpand(bp),crecip(fexpand(bp)));
        for (i=0; i<52; i++)
        {
            //if ( (hexstr= jstr(jitem(array,i),0)) != 0 )
            {
                //decode_hex(pubkey.bytes,sizeof(pubkey),hexstr);
                pubkey = dp->cardpubs[i];
                vcalc_sha256(0,hash.bytes,pubkey.bytes,sizeof(pubkey));
                dp->cardpubs[i] = pubkey;
                hash.bytes[0] &= 0xf8, hash.bytes[31] &= 0x7f, hash.bytes[31] |= 64;
                hexp = fexpand(hash);
                prod = fmul(prod,hexp);
            } //else printf("no hexstr in cardpubs array[%d]\n",i);
        }
        //if ( (hexstr= jstr(jitem(array,52),0)) != 0 )
        {
            pubkey = dp->cardpubs[dp->numcards];
            check = fcontract(prod);
            //decode_hex(pubkey.bytes,sizeof(pubkey),hexstr);
            if ( memcmp(check.bytes,pubkey.bytes,sizeof(check)) != 0 )
                printf("permicheck.%llx != prod.%llx (%s)\n",(long long)check.txid,(long long)pubkey.txid,jprint(json,0));
            else printf("pubkeys matched\n");
        }
    }
    return(check);
}

int32_t pangea_newhand(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen)
{
    char *nrs;
    if ( datalen != (dp->numcards + 1) * sizeof(bits256) )
    {
        printf("pangea_newhand invalid datalen.%d vs %ld\n",datalen,(dp->numcards + 1) * sizeof(bits256));
        return(-1);
    }
    memcpy(dp->cardpubs,data,(dp->numcards + 1) * sizeof(bits256));
    dp->checkprod = pangea_pubkeys(json,dp);
    printf("player.%d got cardpubs.%llx\n",hn->client->H.slot,(long long)dp->checkprod.txid);
    if ( (nrs= jstr(json,"sharenrs")) != 0 )
        decode_hex(dp->sharenrs,(int32_t)strlen(nrs)>>1,nrs);
    memset(dp->handranks,0,sizeof(dp->handranks));
    memset(priv->hole,0,sizeof(priv->hole));
    memset(priv->holecards,0,sizeof(priv->holecards));
    memset(dp->community,0,sizeof(dp->community));
    dp->handmask = 0;
    dp->numhands++;
    dp->button++;
    if ( dp->button >= dp->N )
        dp->button = 0;
    dp->tablepot = 3, dp->balances[dp->button]--, dp->balances[(dp->button + 1) % dp->N] -= 2;
    return(0);
}

int32_t pangea_encode(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen)
{
    if ( datalen != (dp->numcards * dp->N) * sizeof(bits256) )
    {
        printf("pangea_encode invalid datalen.%d vs %ld\n",datalen,(dp->numcards + 1) * sizeof(bits256));
        return(-1);
    }
    //if ( Debuglevel > 2 )
    printf("player.%d encodes into %p\n",hn->client->H.slot,priv->outcards);
    cards777_encode(priv->outcards,priv->xoverz,priv->allshares,priv->myshares,dp->sharenrs,dp->M,(void *)data,dp->numcards,dp->N);
    return(0);
}

int32_t pangea_final(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen)
{
    if ( datalen != (dp->numcards * dp->N) * sizeof(bits256) )
    {
        printf("pangea_final invalid datalen.%d vs %ld\n",datalen,(dp->numcards * dp->N) * sizeof(bits256));
        return(-1);
    }
    //if ( Debuglevel > 2 )
    printf("player.%d final into %p\n",hn->client->H.slot,priv->outcards);
    memcpy(dp->final,data,sizeof(*dp->final) * dp->N * dp->numcards);
    if ( hn->client->H.slot == dp->N-1 )
        memcpy(priv->incards,data,sizeof(*priv->incards) * dp->N * dp->numcards);
    printf("player.%d got final crc.%04x %llx\n",hn->client->H.slot,_crc32(0,data,datalen),(long long)dp->final[1].txid);
    return(0);
}

int32_t pangea_decode(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen)
{
    int32_t cardi,destplayer,card; bits256 cardpriv;
    if ( datalen != sizeof(bits256) )
    {
        printf("pangea_decode invalid datalen.%d vs %ld\n",datalen,sizeof(bits256));
        return(-1);
    }
    cardi = juint(json,"cardi");
    destplayer = juint(json,"dest");
    if ( (card= cards777_checkcard(&cardpriv,cardi,hn->client->H.slot,destplayer,priv->privkey,dp->cardpubs,dp->numcards,*(bits256 *)data)) >= 0 )
        printf("ERROR: player.%d got card.[%d]\n",hn->client->H.slot,card);
    memcpy(&priv->incards[cardi*dp->N + destplayer],data,sizeof(bits256));
    printf("decode\n");
    return(0);
}

int32_t pangea_card(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen)
{
    int32_t cardi,destplayer,card; bits256 cardpriv;
    if ( datalen != sizeof(bits256) )
    {
        printf("pangea_card invalid datalen.%d vs %ld\n",datalen,sizeof(bits256));
        return(-1);
    }
    cardi = juint(json,"cardi");
    destplayer = juint(json,"dest");
    if ( (card= cards777_checkcard(&cardpriv,cardi,hn->client->H.slot,destplayer,priv->privkey,dp->cardpubs,dp->numcards,*(bits256 *)data)) >= 0 )
    {
        printf("player.%d got card.[%d]\n",hn->client->H.slot,card);
        memcpy(&priv->incards[cardi*dp->N + destplayer],cardpriv.bytes,sizeof(bits256));
    }
    else printf("ERROR player.%d got no card\n",hn->client->H.slot);
    return(0);
}

int32_t pangea_facedown(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen)
{
    int32_t destplayer;
    if ( datalen != sizeof(bits256) )
    {
        printf("pangea_facedown invalid datalen.%d vs %ld\n",datalen,sizeof(bits256));
        return(-1);
    }
    destplayer = juint(json,"dest");
    printf("player.%d sees that destplayer.%d got card\n",hn->client->H.slot,destplayer);
    return(0);
}

int32_t pangea_faceup(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen)
{
    int32_t cardi; char hexstr[65];
    if ( datalen != sizeof(bits256) )
    {
        printf("pangea_faceup invalid datalen.%d vs %ld\n",datalen,(dp->numcards + 1) * sizeof(bits256));
        return(-1);
    }
    init_hexbytes_noT(hexstr,data,sizeof(bits256));
    cardi = juint(json,"cardi");
    printf("player.%d was REVEALED.[%d] (%s) cardi.%d\n",hn->client->H.slot,data[1],hexstr,cardi);
    dp->community[cardi - 2*dp->N] = data[1];
    return(0);
}

int32_t pangea_turn(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen)
{
    printf("pangea_turn for player.%d\n",hn->client->H.slot);
    return(0);
}

int32_t pangea_showdown(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen)
{
    char *handstr,tmp[128]; int32_t rank,j,bestj,bestrank,senderslot;
    senderslot = juint(json,"myslot");
    if ( (handstr= jstr(json,"hand")) != 0 )
    {
        rank = set_handstr(tmp,data,0);
        if ( strcmp(handstr,tmp) != 0 || rank != juint(json,"rank") )
            printf("checkhand.(%s) != (%s) || rank.%u != %u\n",tmp,handstr,rank,juint(json,"rank"));
        else
        {
            //printf("sender.%d (%s) (%d %d)\n",senderslot,handstr,buf[5],buf[6]);
            dp->handranks[senderslot] = rank;
            memcpy(dp->hands[senderslot],data,7);
            dp->handmask |= (1 << senderslot);
            if ( dp->handmask == (1 << dp->N)-1 )
            {
                bestj = 0;
                bestrank = dp->handranks[0];
                for (j=1; j<dp->N; j++)
                    if ( dp->handranks[j] > bestrank )
                    {
                        bestrank = dp->handranks[j];
                        bestj = j;
                    }
                rank = set_handstr(tmp,dp->hands[bestj],0);
                if ( rank == bestrank )
                {
                    for (j=0; j<dp->N; j++)
                    {
                        rank = set_handstr(tmp,dp->hands[j],0);
                        if ( tmp[strlen(tmp)-1] == ' ' )
                            tmp[strlen(tmp)-1] = 0;
                        printf("%14s|",tmp[0]!=' '?tmp:tmp+1);
                        //printf("(%2d %2d).%d ",dp->hands[j][5],dp->hands[j][6],(int32_t)dp->balances[j]);
                    }
                    rank = set_handstr(tmp,dp->hands[bestj],0);
                    dp->balances[bestj] += dp->tablepot, dp->tablepot = 0;
                    printf("->P%d $%-5lld %s N%d p%d $%d\n",bestj,(long long)dp->balances[bestj],tmp,dp->numhands,hn->client->H.slot,(int32_t)dp->balances[hn->client->H.slot]);
                } else printf("bestrank.%u mismatch %u\n",bestrank,rank);
            }
            //printf("player.%d got rank %u (%s) from %d\n",hn->client->H.slot,rank,handstr,senderslot);
        }
    }
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
            }
            if ( (cmdstr= jstr(json,"cmd")) != 0 )
            {
                if ( strcmp(cmdstr,"newhand") == 0 )
                    pangea_newhand(hn,json,dp,priv,buf,len);
                else if ( strcmp(cmdstr,"encode") == 0 )
                    pangea_encode(hn,json,dp,priv,buf,len);
                else if ( strcmp(cmdstr,"final") == 0 )
                    pangea_final(hn,json,dp,priv,buf,len);
                else if ( strcmp(cmdstr,"decode") == 0 )
                    pangea_decode(hn,json,dp,priv,buf,len);
                else if ( strcmp(cmdstr,"card") == 0 )
                    pangea_card(hn,json,dp,priv,buf,len);
                else if ( strcmp(cmdstr,"facedown") == 0 )
                    pangea_facedown(hn,json,dp,priv,buf,len);
                else if ( strcmp(cmdstr,"faceup") == 0 )
                    pangea_faceup(hn,json,dp,priv,buf,len);
                else if ( strcmp(cmdstr,"turn") == 0 )
                    pangea_turn(hn,json,dp,priv,buf,len);
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

int32_t pangea_newdeck(union hostnet777 *src)
{
    uint8_t data[(CARDS777_MAXCARDS + 1) * sizeof(bits256)]; struct cards777_pubdata *dp; bits256 destpub; char nrs[512],hex[32768];
    struct cards777_privdata *priv; int32_t n,hexlen,len,state = 0;
    dp = src->client->H.pubdata;
    priv = src->client->H.privdata;
    memset(dp->sharenrs,0,sizeof(dp->sharenrs));
    init_sharenrs(dp->sharenrs,0,dp->N,dp->N);
    dp->checkprod = dp->cardpubs[dp->numcards] = cards777_initdeck(priv->outcards,dp->cardpubs,dp->numcards,dp->N,dp->playerpubs);
    init_hexbytes_noT(nrs,dp->sharenrs,dp->N);
    len = (dp->numcards + 1) * sizeof(bits256);
    sprintf(hex,"{\"cmd\":\"%s\",\"state\":%u,\"sender\":\"%llu\",\"timestamp\":\"%lu\",\"sharenrs\":\"%s\",\"n\":%u,\"data\":\"","newhand",state,(long long)src->client->H.nxt64bits,time(NULL),nrs,len);
    n = (int32_t)strlen(hex);
    memcpy(data,dp->cardpubs,len);
    init_hexbytes_noT(&hex[n],data,len);
    strcat(hex,"\"}");
    hexlen = (int32_t)strlen(hex)+1;
    memset(destpub.bytes,0,sizeof(destpub));
    hostnet777_msg(0,destpub,src,0,hex,hexlen);
    //printf("NEWDECK.(%s)\n",hex);
    return(state);
}

int32_t pangea_statemachine(union hostnet777 *hn,int32_t numclients,int32_t state)
{
    int32_t blindflag,len,n,i,j,k,hexlen,cardi,destplayer,revealed; uint32_t rank; cJSON *json; union hostnet777 src; uint64_t srcbits,destbits;
    char *cmdstr,hex[32768+128],handstr[128]; uint8_t data[16384]; struct cards777_privdata *priv;
    struct cards777_pubdata *dp; bits256 destpub,card;
    state %= (numclients + numclients * (numclients*2 + 5 + 1));
    revealed = -1;
    rank = handstr[0] = 0;
    blindflag = 0;
    cardi = destplayer = -1;
    //printf("numclients.%d dp.%p priv.%p\n",numclients,hn->client->H.pubdata,hn->client->H.privdata);
    if ( (i= state) < numclients )
    {
        if ( hn->server->H.slot != i )
            return(0);
        //printf("i.%d numclients.%d slot.%d\n",state,numclients,hn->server->H.slot);
        dp = hn->client->H.pubdata;
        priv = hn->client->H.privdata;
        if ( i < numclients-1 )
        {
            if ( state == 0 )
                pangea_newdeck(hn);
            j = i+1, cmdstr = "encode";
        }
        else j = -1, cmdstr = "final";
        len = sizeof(bits256) * dp->N * dp->numcards;
        printf("player.%d len.%d N.%d numcards.%d from outcards.%p\n",i,len,dp->N,dp->numcards,priv->outcards);
        memcpy(data,priv->outcards,len);
    }
    else
    {
        cardi = (state / numclients) - 1;
        dp = hn->client->H.pubdata;
        destplayer = ((cardi + dp->button) % numclients);
        if ( cardi < numclients*2 + 5 )
        {
            i = (numclients - 1) - (state % numclients);
            if ( i > 1 )
                j = i - 1, cmdstr = "decode";
            else if ( i == 1 )
                j = destplayer, cmdstr = "card";
            else //if ( i == 0 )
            {
                j = -1;
                i = destplayer;
                if ( cardi < numclients*2 )
                    cmdstr = "facedown";
                else cmdstr = "faceup";
            }
        }
        else
        {
            j = -1;
            i = (state % numclients);
            cmdstr = "showdown";
        }
        if ( hn->client->H.slot != i )
            return(0);
        dp = hn->client->H.pubdata;
        priv = hn->client->H.privdata;
        if ( dp == 0 || priv == 0 )
        {
            printf("null dp.%p or priv.%p\n",dp,priv);
            getchar();
        }
        if ( strcmp(cmdstr,"showdown") == 0 )
        {
            len = 7;
            for (k=0; k<5; k++)
                data[k] = dp->community[k];
            data[k++] = priv->hole[0];
            data[k++] = priv->hole[1];
            for (k=0; k<7; k++)
            {
                printf("%2d ",data[k]);
                if ( data[k] >= 52 )
                    data[k] = 0;
            }
            printf("call set_handstr\n");
            rank = set_handstr(handstr,data,0);
        }
        else
        {
            card = priv->incards[cardi*numclients + destplayer];
            if ( j >= 0 )
                card = cards777_decode(priv->xoverz,destplayer,card,priv->outcards,dp->numcards,numclients);
            else
            {
                if ( strcmp(cmdstr,"facedown") == 0 )
                {
                    priv->hole[cardi / numclients] = card.bytes[1];
                    priv->holecards[cardi / numclients] = card;
                    memset(card.bytes,0,sizeof(card));
                }
                else
                {
                    revealed = card.bytes[1];
                    //printf("cmd.%s player.%d %llx (cardi.%d destplayer.%d) card.[%d]\n",cmdstr,i,(long long)card.txid,cardi,destplayer,card.bytes[1]);
                }
            }
            len = sizeof(bits256);
            memcpy(data,card.bytes,len);
        }
    }
    //printf("state.%d i.%d cardi.%d destplayer.%d j.%d\n",state,i,cardi,destplayer,j);
    if ( i == 0 )
        src.server = hn->server;
    else src.client = hn->client;
    dp = hn->client->H.pubdata;
    priv = hn->client->H.privdata;
    if ( j < 0 )
    {
        destbits = 0;
        memset(destpub.bytes,0,sizeof(destpub));
    }
    else
    {
        destpub = dp->playerpubs[j];
        destbits = acct777_nxt64bits(destpub);
    }
    srcbits = src.client->H.nxt64bits;
    sprintf(hex,"{\"cmd\":\"%s\",\"state\":%u,\"myslot\":%d,\"hand\":\"%s\",\"rank\":%u,\"cardi\":%d,\"dest\":%d,\"sender\":\"%llu\",\"timestamp\":\"%lu\",\"n\":%u,\"data\":\"",cmdstr,state,i,handstr,rank,cardi,destplayer,(long long)srcbits,time(NULL),len);
    n = (int32_t)strlen(hex);
    init_hexbytes_noT(&hex[n],data,len);
    strcat(hex,"\"}");
    if ( (json= cJSON_Parse(hex)) == 0 )
    {
        printf("error creating json\n");
        return(-1);
    }
    free_json(json);
    hexlen = (int32_t)strlen(hex)+1;
    printf("HEX.[%s] hexlen.%d n.%d\n",hex,hexlen,len);
    hostnet777_msg(destbits,destpub,&src,blindflag,hex,hexlen);
    return(0);
}

int32_t pangea_idle(struct plugin_info *plugin)
{
    int32_t i,n,m; uint64_t senderbits; uint32_t timestamp; struct pangea_thread *tp; union hostnet777 *hn;
    while ( 1 )
    {
        for (i=n=m=0; i<PANGEA_MAXTHREADS; i++)
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
                }
            }
        }
        if ( n == 0 )
            break;
        if ( m == 0 )
            msleep(3);
    }
    return(0);
}

struct pangea_info *pangea_create(struct pangea_thread *tp,int32_t *createdflagp,char *base,uint32_t timestamp,uint64_t *addrs,int32_t numaddrs,uint64_t bigblind,uint64_t ante)
{
    struct pangea_info *sp = 0; bits256 hash; int32_t i,j,numcards,firstslot = -1; struct cards777_privdata *priv; struct cards777_pubdata *dp;
    if ( createdflagp != 0 )
        *createdflagp = 0;
    for (i=0; i<numaddrs; i++)
        if ( addrs[i] == tp->nxt64bits )
            break;
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
            dp->balances[j] = 100;
        sp->priv = priv = cards777_allocpriv(numcards,numaddrs);
        strcpy(sp->base,base);
        if ( (sp->timestamp= timestamp) == 0 )
            sp->timestamp = (uint32_t)time(NULL);
        sp->numaddrs = numaddrs;
        sp->basebits = stringbits(base);
        sp->bigblind = bigblind, sp->ante = ante;
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
        jaddistr(array,hexstr);
    }
    return(array);
}

cJSON *pangea_cardpubs(struct cards777_pubdata *dp)
{
    int32_t i; char hexstr[65]; cJSON *array = cJSON_CreateArray();
    for (i=0; i<dp->numcards; i++)
    {
        init_hexbytes_noT(hexstr,dp->cardpubs[i].bytes,sizeof(bits256));
        jaddistr(array,hexstr);
    }
    init_hexbytes_noT(hexstr,dp->checkprod.bytes,sizeof(bits256));
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

char *pangea_newtable(int32_t threadid,cJSON *json)
{
    int32_t createdflag,num,i,myind= -1; uint64_t tableid,addrs[CARDS777_MAXPLAYERS]; struct pangea_info *sp; cJSON *array;
    struct pangea_thread *tp; char *base,*hexstr; uint32_t timestamp; struct cards777_pubdata *dp;
    if ( (tp= THREADS[threadid]) == 0 )
        return(clonestr("{\"error\":\"uinitialized threadid\"}"));
    //printf("T%d NEWTABLE.(%s)\n",threadid,jprint(json,0));
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
            if ( addrs[i] == tp->nxt64bits )
                myind = i;
        }
        if ( myind < 0 )
            return(clonestr("{\"error\":\"this table is not for me\"}"));
        if ( (sp= pangea_create(tp,&createdflag,base,timestamp,addrs,num,j64bits(json,"bigblind"),j64bits(json,"ante"))) == 0 )
        {
            printf("cant create table.(%s) numaddrs.%d\n",base,num);
            return(clonestr("{\"error\":\"cant create table\"}"));
        }
        dp = sp->dp; sp->myind = myind;
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
            THREADS[0]->hn.server->clients[threadid].pubdata = dp;
            THREADS[0]->hn.server->clients[threadid].privdata = sp->priv;
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
            printf("playerpubs.(%s)\n",hexstr);
        }
        if ( myind >= 0 && createdflag != 0 && addrs[myind] == tp->nxt64bits )
        {
            memcpy(sp->addrs,addrs,sizeof(*addrs) * dp->N);
            /*if ( (array= jarray(&n,json,"sharenrs")) == 0 || n != dp->N || num != dp->N )
            {
                printf("invalid sharenrs n.%d vs N.%d num.%d threadid.%d myind.%d (%s)\n",n,dp->N,num,tp->threadid,myind,jprint(json,0));
                return(clonestr("{\"error\":\"invalid sharenrs or mismatched numplayers\"}"));
            }
            printf("sharenrs n.%d vs N.%d num.%d threadid.%d myind.%d (%s)\n",n,dp->N,num,tp->threadid,myind,jprint(json,0));
            for (i=0; i<n; i++)
            {
                dp->sharenrs[i] = juint(jitem(array,i),0);
                printf("%d ",dp->sharenrs[i]);
            }
            printf("sharenrs\n");
            dp->checkprod = pangea_pubkeys(json,"rawpubs",dp);
            if ( myind == dp->N-1 )
            {
                ciphers = jprint(pangea_ciphersjson(dp,sp->priv),1);
                sprintf((char *)sp->player.ep.sendbuf,"{\"cmd\":\"encode\",\"myind\":%d,\"my64bits\":\"%llu\",\"tableid\":\"%llu\",\"timestamp\":%u,\"M\":%d,\"N\":%d,\"base\":\"%s\",\"bigblind\":\"%llu\",\"ante\":\"%llu\",\"ciphers\":%s}",myind,(long long)tp->nxt64bits,(long long)sp->tableid,sp->timestamp,dp->M,dp->N,sp->base,(long long)sp->bigblind,(long long)sp->ante,ciphers);
                free(ciphers);
            }*/
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

int32_t pangea_start(int32_t threadid,char *retbuf,char *base,uint32_t timestamp,uint64_t bigblind,uint64_t ante,int32_t maxplayers,cJSON *json)
{
    char *addrstr,*ciphers,*playerpubs; uint8_t p2shtype; struct pangea_thread *tp; struct cards777_pubdata *dp;
    int32_t createdflag,addrtype,i,j,n,myind=-1,r,num=0; uint64_t addrs[512]; struct pangea_info *sp; cJSON *bids;
    if ( (tp= THREADS[threadid]) == 0 )
    {
        strcpy(retbuf,"{\"error\":\"uinitialized threadid\"}");
        printf("%s\n",retbuf);
        return(-1);
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
            if ( (addrs[num]= j64bits(jitem(bids,i),"offerNXT")) != 0 )
            {
                printf("(i.%d %llu) ",i,(long long)addrs[num]);
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
        }
    }
    printf("(%llu) pangea_start(%s) threadid.%d myind.%d num.%d maxplayers.%d\n",(long long)tp->nxt64bits,base,tp->threadid,myind,num,maxplayers);
    if ( (i= myind) > 0 )
    {
        addrs[i] = addrs[0];
        addrs[0] = tp->nxt64bits;
        i = 0;
    }
    while ( num > maxplayers )
    {
        r = (rand() % (num-1));
        printf("swap out %d of %d\n",r+1,num);
        addrs[r + 1] = addrs[--num];
    }
    printf("pangea numplayers.%d\n",num);
    if ( (sp= pangea_create(tp,&createdflag,base,timestamp,addrs,num,bigblind,ante)) == 0 )
    {
        printf("cant create table.(%s) numaddrs.%d\n",base,num);
        return(-1);
    }
    dp = sp->dp;
    sp->myind = myind;
    if ( createdflag != 0 && myind == 0 && addrs[myind] == tp->nxt64bits )
    {
        tp->numcards = dp->numcards, tp->N = dp->N, tp->M = dp->M;
        tp->hn.server->clients[myind].pubdata = dp;
        tp->hn.server->clients[myind].privdata = sp->priv;
        tp->hn.server->H.pubdata = dp;
        tp->hn.server->H.privdata = sp->priv;
        init_sharenrs(dp->sharenrs,0,dp->N,dp->N);
        for (j=0; j<dp->N; j++)
            dp->playerpubs[j] = THREADS[j]->hn.client->H.pubkey;
        dp->checkprod = cards777_initdeck(sp->priv->outcards,dp->cardpubs,dp->numcards,dp->N,dp->playerpubs);
        addrstr = jprint(addrs_jsonarray(addrs,num),1);
        ciphers = jprint(pangea_ciphersjson(dp,sp->priv),1);
        //cardpubs = jprint(pangea_cardpubs(dp),1);
        //sharenrs = jprint(pangea_sharenrs(dp->sharenrs,num),1);
        playerpubs = jprint(pangea_playerpubs(dp->playerpubs,num),1);
        sprintf(retbuf,"{\"cmd\":\"newtable\",\"broadcast\":\"allnodes\",\"myind\":%d,\"pangea_endpoint\":\"%s\",\"plugin\":\"relay\",\"destplugin\":\"pangea\",\"method\":\"busdata\",\"submethod\":\"newtable\",\"pluginrequest\":\"SuperNET\",\"my64bits\":\"%llu\",\"tableid\":\"%llu\",\"timestamp\":%u,\"M\":%d,\"N\":%d,\"base\":\"%s\",\"bigblind\":\"%llu\",\"ante\":\"%llu\",\"playerpubs\":%s,\"addrs\":%s}",sp->myind,tp->hn.server->ep.endpoint,(long long)tp->nxt64bits,(long long)sp->tableid,sp->timestamp,dp->M,dp->N,sp->base,(long long)bigblind,(long long)ante,playerpubs,addrstr);
        free(addrstr), free(ciphers), free(playerpubs);//free(cardpubs), free(sharenrs);
    }
    return(0);
}

void pangea_test(struct plugin_info *plugin,int32_t numthreads)
{
    char retbuf[65536]; bits256 privkey,pubkey; int32_t i,slot,threadid; struct pangea_thread *tp; struct hostnet777_client **clients;
    struct hostnet777_server *srv; cJSON *item,*bids,*testjson = cJSON_CreateObject();
    sleep(3);
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
            srv->H.privkey = privkey, srv->H.pubkey = pubkey;
        }
        else
        {
            if ( (slot= hostnet777_register(srv,pubkey,-1)) >= 0 )
            {
                if ( (clients[threadid]= hostnet777_client(privkey,pubkey,srv->ep.endpoint,slot)) == 0 )
                    printf("error creating clients[%d]\n",threadid);
                else
                {
                    tp->hn.client = clients[threadid];
                    tp->hn.client->H.privkey = privkey, tp->hn.client->H.pubkey = pubkey;
                    printf("slot.%d client.%p -> %llu pubkey.%llx\n",slot,clients[threadid],(long long)clients[threadid]->H.nxt64bits,(long long)clients[threadid]->H.pubkey.txid);
                    //if ( portable_thread_create((void *)hostnet777_idler,hn) == 0 )
                    //    printf("error launching clients[%d] thread\n",threadid);
                }
            }
        }
    }
    bids = cJSON_CreateArray();
    for (i=0; i<numthreads; i++)
    {
        item = cJSON_CreateObject();
        jadd64bits(item,"offerNXT",THREADS[i]->nxt64bits);
        jaddi(bids,item);
    }
    jadd(testjson,"bids",bids);
    jadd64bits(testjson,"offerNXT",THREADS[0]->nxt64bits);
    printf("TEST.(%s)\n",jprint(testjson,0));
    pangea_start(0,retbuf,"BTCD",0,SATOSHIDEN,0,i,testjson);
    free_json(testjson);
    testjson = cJSON_Parse(retbuf);
    //printf("BROADCAST.(%s)\n",retbuf);
    for (threadid=1; threadid<PANGEA_MAXTHREADS; threadid++)
        pangea_newtable(threadid,testjson);
    uint32_t timestamp; uint64_t senderbits;
    tp = THREADS[0];
    pangea_statemachine(&tp->hn,tp->N,0);
    pangea_poll(&senderbits,&timestamp,&tp->hn);
    sleep(13);
    tp = THREADS[1];
    pangea_poll(&senderbits,&timestamp,&tp->hn);
    pangea_statemachine(&tp->hn,tp->N,1);
}

int32_t PLUGNAME(_process_json)(char *forwarder,char *sender,int32_t valid,struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag,char *tokenstr)
{
    char *resultstr,*methodstr,*base,*retstr = 0; int32_t maxplayers; cJSON *argjson; struct pangea_thread *tp;
    retbuf[0] = 0;
    printf("<<<<<<<<<<<< INSIDE PANGEA! process %s (%s)\n",plugin->name,jsonstr);
    if ( initflag > 0 )
    {
        uint64_t conv_NXTpassword(unsigned char *mysecret,unsigned char *mypublic,uint8_t *pass,int32_t passlen);
        //PANGEA.readyflag = 1;
        plugin->sleepmillis = 100;
        plugin->allowremote = 1;
        argjson = cJSON_Parse(jsonstr);
        plugin->nxt64bits = set_account_NXTSECRET(plugin->mypriv,plugin->mypub,plugin->NXTACCT,plugin->NXTADDR,plugin->NXTACCTSECRET,sizeof(plugin->NXTACCTSECRET),argjson,0,0,0);
        free_json(argjson);
        printf("my64bits %llu ipaddr.%s mypriv.%02x mypub.%02x\n",(long long)plugin->nxt64bits,plugin->ipaddr,plugin->mypriv[0],plugin->mypub[0]);
        if ( 1 )
            pangea_test(plugin,2);
        else if ( juint(json,"pangeaport") != 0 )
        {
            PANGEA_MAXTHREADS = 1;
            THREADS[0] = tp = calloc(1,sizeof(*THREADS[0]));
            tp->nxt64bits = plugin->nxt64bits;
            memcpy(tp->hn.client->H.privkey.bytes,plugin->mypriv,sizeof(bits256));
            memcpy(tp->hn.client->H.pubkey.bytes,plugin->mypub,sizeof(bits256));
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
            if ( (base= jstr(json,"base")) != 0 )
            {
                if ( (maxplayers= juint(json,"maxplayers")) < 2 )
                    maxplayers = 2;
                else if ( maxplayers > CARDS777_MAXPLAYERS )
                    maxplayers = CARDS777_MAXPLAYERS;
                if ( jstr(json,"resubmit") == 0 )
                    sprintf(retbuf,"{\"resubmit\":[{\"method\":\"start\"}, {\"bigblind\":\"%llu\"}, {\"ante\":\"%llu\"}, {\"maxplayers\":%d}],\"pluginrequest\":\"SuperNET\",\"plugin\":\"InstantDEX\",\"method\":\"orderbook\",\"base\":\"BTCD\",\"exchange\":\"pangea\",\"allfields\":1}",(long long)j64bits(json,"bigblind"),(long long)j64bits(json,"ante"),maxplayers);
                else pangea_start(juint(json,"threadid"),retbuf,base,0,j64bits(json,"bigblind"),j64bits(json,"ante"),maxplayers,json);
            } else strcpy(retbuf,"{\"error\":\"no base specified\"}");
        }
        else if ( strcmp(methodstr,"newtable") == 0 )
            retstr = pangea_newtable(juint(json,"threadid"),json);
    }
    return(plugin_copyretstr(retbuf,maxlen,retstr));
}

#include "../agents/plugin777.c"
