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

//#define BUNDLED
#define PLUGINSTR "pangea"
#define PLUGNAME(NAME) pangea ## NAME
#define STRUCTNAME struct PLUGNAME(_info) 
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
#include "../../utils/huffstream.c"
#include "../../utils/ramcoder.c"
#include "../../includes/cJSON.h"
#include "../../agents/plugin777.c"
#undef DEFINES_ONLY

#include "../../includes/InstantDEX_quote.h"
#include "../../includes/tweetnacl.h"
#include "../../utils/curve25519.h"

#define PANGEA_STATE_INIT -1
#define PANGEA_STATE_READY 0
#define PANGEA_STATE_SENTMOFN 1

queue_t pangeaQ;
struct keypair { bits256 pub,priv; };
struct pangea_card { struct keypair key; bits256 ciphers[9][9],checkpub; };
struct pangea_player { struct keypair comm; bits256 permiciphers[52],ciphers[52],snaps[9][52],xoverz[9][52],permi_snaps[52],permi_xoverz[52]; int32_t cipherdests[52]; uint8_t cards[2]; };

struct pangea_deck
{
    struct pangea_player players[9];
    struct pangea_card cards[52],permi[52];
    bits256 checkprod,permiprod;
    uint64_t deckid; int32_t numplayers; uint8_t sharenrs[255],M,N,faceups[5],*mofn[9];
};

struct pangea_info
{
    uint32_t timestamp,numaddrs;
    uint64_t basebits,bigblind,ante,addrs[9],tableid,quoteid;
    struct pangea_deck deck;
    char base[16],ipaddr[64],endpoint[128]; uint8_t sendbuf[65536*2]; uint32_t statetimestamp;
    int32_t readyflag,button,myind,done,pushsock,pullsock,pubsock,subsock,sendlen,mask,layers,didturn,state,states[9]; uint16_t port;
} *TABLES[100];
STRUCTNAME PANGEA;

char *clonestr(char *);
uint64_t stringbits(char *str);

void calc_shares(unsigned char *shares,unsigned char *secret,int32_t size,int32_t width,int32_t M,int32_t N,unsigned char *sharenrs);
int32_t init_sharenrs(unsigned char sharenrs[255],unsigned char *orig,int32_t m,int32_t n);
void gfshare_ctx_dec_newshares(void *ctx,unsigned char *sharenrs);
void gfshare_ctx_dec_giveshare(void *ctx,unsigned char sharenr,unsigned char *share);
void gfshare_ctx_dec_extract(void *ctx,unsigned char *secretbuf);
void *gfshare_ctx_init_dec(unsigned char *sharenrs,uint32_t sharecount,uint32_t size);
void gfshare_ctx_free(void *ctx);

uint32_t set_handstr(char *handstr,uint8_t cards[7]);
int32_t init_hexbytes_noT(char *hexbytes,uint8_t *message,long len);
int32_t uniq_specialaddrs(int32_t *myindp,uint64_t addrs[],int32_t max,char *base,char *special,int32_t addrtype);
bits256 issue_getpubkey(int32_t *haspubkeyp,char *acct);
uint64_t set_account_NXTSECRET(void *myprivkey,void *mypubkey,char *NXTacct,char *NXTaddr,char *secret,int32_t max,cJSON *argjson,char *coinstr,char *serverport,char *userpass);

int32_t decode_cipher(uint8_t *str,uint8_t *cipher,int32_t *lenp,uint8_t *myprivkey);
uint8_t *encode_str(int32_t *cipherlenp,void *str,int32_t len,bits256 destpubkey,bits256 myprivkey,bits256 mypubkey);

struct InstantDEX_quote *find_iQ(uint64_t quoteid);
bits256 ramcoder_encoder(struct ramcoder *coder,int32_t updateprobs,uint8_t *buf,int32_t len,HUFF *hp,bits256 *seed);
int32_t nn_createsocket(char *endpoint,int32_t bindflag,char *name,int32_t type,uint16_t port,int32_t sendtimeout,int32_t recvtimeout);
int32_t nn_socket_status(int32_t sock,int32_t timeoutmillis);

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

bits256 pangea_shared(bits256 privkey,bits256 otherpub)
{
    bits256 shared,hash;
    shared = curve25519(privkey,otherpub);
    vcalc_sha256(0,hash.bytes,shared.bytes,sizeof(shared));
    //printf("priv.%llx pub.%llx shared.%llx -> hash.%llx\n",privkey.txid,pubkey.txid,shared.txid,hash.txid);
    hash.bytes[0] &= 0xf8, hash.bytes[31] &= 0x7f, hash.bytes[31] |= 64;
    return(hash);
}

bits256 pangea_encrypt(bits320 data,bits256 privkey,bits256 pubkey,int32_t invert)
{
    bits256 hash; bits320 hexp;
    hash = pangea_shared(privkey,pubkey);
    hexp = fexpand(hash);
    if ( invert != 0 )
        hexp = crecip(hexp);
    //for (i=0; i<5; i++)
    //    data.ulongs[i] ^= hash.ulongs[i % 4];
    return(fcontract(fmul(data,hexp)));
}

bits256 keypair(bits256 *pubkeyp)
{
    bits256 rand256(int32_t privkeyflag);
    bits256 basepoint,privkey;
    memset(&basepoint,0,sizeof(basepoint));
    basepoint.bytes[0] = 9;
    privkey = rand256(1);
    *pubkeyp = curve25519(privkey,basepoint);
    //printf("[%llx %llx] ",privkey.txid,(*pubkeyp).txid);
    return(privkey);
}

bits256 pangea_initdeck(struct pangea_deck *deck,int32_t numplayers)
{
    bits256 basepoint,privkey,pubkey,hash,permipub; bits320 bp,prod,hexp; int32_t i,j,numcards = 0; uint64_t mask = 0; struct pangea_card *cards;
    cards = (numplayers != 0) ? deck->cards : deck->permi;
    memset(&basepoint,0,sizeof(basepoint));
    basepoint.bytes[0] = 9;
    bp = fexpand(basepoint);
    prod = crecip(bp);
    prod = fmul(bp,prod);
    permipub = deck->players[deck->numplayers-1].comm.pub;
    printf("unit.%llx\n",(long long)prod.txid);
    while ( mask != (1LL << 52)-1 )
    {
        privkey = keypair(&pubkey);
        if ( (i=privkey.bytes[1]) < 52 && ((1LL << i) & mask) == 0 )
        {
            mask |= (1LL << i);
            if ( numplayers > 0 )
            {
                cards[numcards].key.pub = pubkey;
                cards[numcards].key.priv = privkey;
                for (j=0; j<numplayers; j++)
                    cards[numcards].ciphers[0][j] = pangea_encrypt(fexpand(privkey),privkey,deck->players[j].comm.pub,0);
            }
            else
            {
                cards[numcards].key.pub = pubkey;
                cards[numcards].key.priv = privkey;
                cards[numcards].ciphers[deck->numplayers-1][deck->numplayers-1] = pangea_encrypt(fexpand(privkey),privkey,permipub,0);
            }
            vcalc_sha256(0,hash.bytes,pubkey.bytes,sizeof(pubkey));
            hash.bytes[0] &= 0xf8, hash.bytes[31] &= 0x7f, hash.bytes[31] |= 64;
            hexp = fexpand(hash);
            prod = fmul(prod,hexp);
            numcards++;
        }
    }
    for (i=0; i<52; i++)
        printf("%d ",cards[i].key.priv.bytes[1]);
    printf("init order permiflag.%d\n",numplayers == 0);
    if ( numplayers != 0 )
        deck->checkprod = fcontract(prod);
    else deck->permiprod = fcontract(prod);
    return(fcontract(prod)); // broadcast cardpubkeys + prod, PM to next node 52xnumplayers ciphers
}

int32_t pangea_shuffle(struct pangea_deck *deck,int32_t playerj,int32_t permiflag)
{
    bits256 perm[52][9]; int32_t i,j,pos,permi[52],desti[52]; uint8_t x; uint64_t mask; struct pangea_card *cards;
    cards = (permiflag != 0) ? deck->permi : deck->cards;
    memset(desti,0,sizeof(desti));
    for (i=0; i<52; i++)
        desti[i] = i;
    for (i=0; i<52; i++)
    {
        randombytes(&x,1);
        pos = (x % ((51-i) + 1));
        //printf("%d ",pos);
        permi[i] = desti[pos];
        desti[pos] = desti[51 - i];
        desti[51 - i] = -1;
    }
    //printf("pos\n");
    for (mask=i=0; i<52; i++)
    {
        printf("%d ",permi[i]);
        mask |= (1LL << permi[i]);
        if ( permiflag == 0 )
        {
            for (j=0; j<deck->numplayers; j++)
                perm[i][j] = cards[permi[i]].ciphers[playerj-1][j];
        }
        else perm[i][deck->numplayers-1] = cards[permi[i]].ciphers[playerj+1][deck->numplayers-1];
    }
    printf("permiflag.%d mask.%llx err.%llx\n",permiflag,(long long)mask,(long long)(mask ^ ((1LL<<52)-1)));
    for (i=0; i<52; i++)
    {
        if ( permiflag == 0 )
        {
            for (j=0; j<deck->numplayers; j++)
                cards[i].ciphers[playerj][j] = perm[i][j];
        }
        else cards[i].ciphers[playerj][deck->numplayers-1] = perm[i][deck->numplayers-1];
    }
    return(0);
}

int32_t pangea_send(int32_t sock,void *ptr,int32_t len)
{
    int32_t j,sendlen = 0;
    if ( sock >= 0 )
    {
        for (j=0; j<10; j++)
            if ( (nn_socket_status(sock,10) & NN_POLLOUT) != 0 )
                break;
        if ( (sendlen= nn_send(sock,ptr,len,0)) == len )
        {
        }
    }
    return(sendlen);
}

int32_t pangea_PM(struct pangea_info *sp,uint8_t *mypriv,uint8_t *mypub,char *destNXT,uint8_t *msg,int32_t len)
{
    int32_t haspubkey,cipherlen,datalen; bits256 destpub,seed; uint8_t *data,*cipher; HUFF H,*hp = &H;
    if ( destNXT != 0 )
    {
        destpub = issue_getpubkey(&haspubkey,destNXT);
        seed = pangea_shared(*(bits256 *)mypriv,destpub);
    }
    memset(seed.bytes,0,sizeof(seed)), seed.bytes[0] = 1;
    data = calloc(1,len*2);
    _init_HUFF(hp,len*2,data);
    ramcoder_encoder(0,1,data,len*2,hp,&seed);
    datalen = hconv_bitlen(hp->bitoffset);
    printf("len.%d -> datalen.%d\n",len,datalen);
    if ( destNXT != 0 )
    {
        if ( (cipher= encode_str(&cipherlen,data,datalen,destpub,*(bits256 *)mypriv,*(bits256 *)mypub)) != 0 )
        {
            pangea_send(sp->myind == 0 ? sp->pubsock : sp->pushsock,cipher,cipherlen);
            free(cipher);
        }
    }
    else pangea_send(sp->myind == 0 ? sp->pubsock : sp->pushsock,data,datalen), cipherlen = datalen;
    free(data);
    return(cipherlen);
}

int32_t pangea_sendnext(uint8_t *mypriv,uint8_t *mypub,struct pangea_info *sp,int32_t incr)
{
    char destNXT[64];
    expand_nxt64bits(destNXT,sp->addrs[sp->myind + incr]);
    printf("pangea_sendnext: destNXT.(%s) [%d] -> addrs[%d] %llu\n",destNXT,sp->myind,sp->myind+incr,(long long)sp->addrs[sp->myind+incr]);
    pangea_PM(sp,mypriv,mypub,destNXT,sp->sendbuf,sp->sendlen);
    return(0);
}

int32_t pangea_decrypt(uint8_t *mypriv,uint64_t my64bits,uint8_t *dest,int32_t maxlen,uint8_t *src,int32_t len)
{
    bits256 seed,senderpub; uint8_t *buf; int32_t newlen = -1; HUFF H,*hp = &H;
    buf = calloc(1,maxlen);
    if ( decode_cipher((void *)buf,src,&len,mypriv) != 0 )
        printf("pangea_decrypt Error: decode_cipher error len.%d -> newlen.%d\n",len,newlen);
    else
    {
        memcpy(senderpub.bytes,src,sizeof(senderpub));
        seed = pangea_shared(*(bits256 *)mypriv,senderpub);
        memset(seed.bytes,0,sizeof(seed)), seed.bytes[0] = 1;
        _init_HUFF(hp,len,buf), hp->endpos = len << 3;
        newlen = ramcoder_decoder(0,1,dest,maxlen,hp,&seed);
    }
    free(buf);
    return(newlen);
}

void pangea_jsoncmd(char *destNXT,cJSON *json,char *cmd,struct plugin_info *plugin,struct pangea_info *sp)
{
    char *jsonstr;
    jaddstr(json,"cmd",cmd);
    jaddstr(json,"base",sp->base);
    jadd64bits(json,"my64bits",plugin->nxt64bits);
    jadd64bits(json,"tableid",sp->tableid);
    jaddnum(json,"timestamp",sp->timestamp);
    jaddnum(json,"M",sp->deck.M);
    jaddnum(json,"N",sp->deck.N);
    jaddnum(json,"myind",sp->myind);
    jadd64bits(json,"bigblind",sp->bigblind);
    jadd64bits(json,"ante",sp->ante);
    jsonstr = jprint(json,1);
    pangea_PM(sp,plugin->mypriv,plugin->mypub,destNXT,(void *)jsonstr,(int32_t)strlen(jsonstr)+1);
    printf("add sig! pangea_jsoncmd myind.%d %s %d -> %s\n",sp->myind,cmd,(int32_t)strlen(jsonstr)+1,destNXT!=0?destNXT:"broadcast");
    free(jsonstr);
}

void pangea_layer(struct plugin_info *plugin,struct pangea_info *sp,struct pangea_deck *deck,int32_t playerj,int32_t permiflag)
{
    bits256 rand256(int32_t privkeyflag);
    char hexstr[65],destNXT[64],*str; int32_t i,k; bits256 basepoint; bits320 bp,x,z,xoverz,zoverx,newval; cJSON *array,*json;
    struct pangea_player *player; struct pangea_card *cards;
    cards = (permiflag != 0) ? deck->permi : deck->cards;
    memset(&basepoint,0,sizeof(basepoint));
    basepoint.bytes[0] = 9;
    bp = fexpand(basepoint);
    player = &deck->players[playerj];
    array = cJSON_CreateArray();
    for (i=0; i<52; i++)
    {
        for (k=permiflag*(deck->numplayers-1); k<deck->numplayers; k++)
        {
            cmult(&x,&z,rand256(1),bp);
            xoverz = fmul(x,crecip(z));
            zoverx = crecip(xoverz);
            newval = fmul(zoverx,fexpand(cards[i].ciphers[playerj][k]));
            if ( permiflag == 0 )
            {
                player->snaps[k][i] = fcontract(newval);
                player->xoverz[k][i] = fcontract(xoverz);
            }
            else
            {
                //printf("%d.(%llx -> %llx).%d ",playerj,(long long)cards[i].ciphers[playerj][k].txid,(long long)newval.txid,k);
                player->permi_snaps[i] = fcontract(newval);
                player->permi_xoverz[i] = fcontract(xoverz);
            }
            cards[i].ciphers[playerj][k] = fcontract(newval);
            init_hexbytes_noT(hexstr,cards[i].ciphers[playerj][k].bytes,sizeof(bits256));
            jaddistr(array,hexstr);
        }
        //printf("card.%d from.%d\n",i,playerj);
    }
    json = cJSON_CreateObject();
    jadd(json,"ciphers",array);
    if ( sp->myind != 0 && sp->myind != sp->deck.numplayers-1 )
        expand_nxt64bits(destNXT,sp->addrs[playerj + ((permiflag != 0) ? -1 : 1)]), str = destNXT;
    else str = 0;
    pangea_jsoncmd(str,json,"encode",plugin,sp);
}

int32_t pangea_sendmofn(struct plugin_info *plugin,struct pangea_info *sp,struct pangea_deck *deck,int32_t playerj)
{
    static int err;
    bits256 xoverz[10][52]; int32_t i,j,m,size; uint8_t *allshares,*recomb,testnrs[255]; void *G; cJSON *json; char *hexstr,destNXT[64];
    for (j=0; j<deck->numplayers; j++)
        for (i=0; i<52; i++)
            xoverz[j][i] = deck->players[playerj].xoverz[j][i];
    for (i=0; i<52; i++)
        xoverz[deck->numplayers][i] = deck->players[playerj].permi_xoverz[i];
    size = (deck->numplayers + 1) * sizeof(bits256) * 52;
    allshares = calloc(deck->N,size);
    recomb = calloc(1,size);
    calc_shares(allshares,(void *)xoverz,size,size,deck->M,deck->N,deck->sharenrs); // PM &allshares[playerj * size] to playerJ
    for (j=0; j<deck->N; j++)
    {
        if ( plugin != 0 && j != playerj )
        {
            hexstr = calloc(1,size*2+1);
            init_hexbytes_noT(hexstr,&allshares[j * size],size);
            json = cJSON_CreateObject();
            jaddstr(json,"data",hexstr);
            expand_nxt64bits(destNXT,sp->addrs[j]);
            pangea_jsoncmd(destNXT,json,"mofn",plugin,sp);
            free(hexstr);
        }
    }
    for (j=0; j<0; j++)
    {
        memset(testnrs,0,sizeof(testnrs));
        memset(recomb,0,size);
        m = deck->M + (rand() % (deck->N - deck->M));
        if ( init_sharenrs(testnrs,deck->sharenrs,m,deck->N) < 0 )
        {
            printf("iter.%d error init_sharenrs(m.%d of n.%d)\n",j,m,deck->N);
            return(-1);
        }
        G = gfshare_ctx_init_dec(testnrs,deck->N,size);
        for (i=0; i<deck->N; i++)
            if ( testnrs[i] == deck->sharenrs[i] )
                gfshare_ctx_dec_giveshare(G,i,&allshares[i*size]);
        gfshare_ctx_dec_newshares(G,testnrs);
        gfshare_ctx_dec_extract(G,recomb);
        if ( memcmp(xoverz,recomb,size) != 0 )
            fprintf(stderr,"(ERROR %d M.%d)\n",err,m), err++;
        else fprintf(stderr,"reconstructed with M.%d\n",m);
        gfshare_ctx_free(G);
    }
    free(allshares);
    free(recomb);
    return(-err);
}

struct pangea_info *pangea_find(uint64_t tableid)
{
    int32_t i;
    for (i=0; i<sizeof(TABLES)/sizeof(*TABLES); i++)
        if ( TABLES[i] != 0 && tableid == TABLES[i]->tableid )
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
    for (i=0; i<sp->deck.numplayers; i++)
        if ( sp->deck.mofn[i] != 0 )
            free(sp->deck.mofn[i]);
    free(sp);
}

int32_t pangea_ready(struct plugin_info *plugin,struct pangea_info *sp,cJSON *json)
{
    uint64_t other64bits; int32_t otherind;
    otherind = juint(json,"myind");
    other64bits = j64bits(json,"my64bits");
    if ( otherind >= 0 && otherind < sp->deck.numplayers )
        sp->mask |= (1 << otherind);
    printf("pangea_ready tableid.%llu from %llu otherind.%d mask.%d\n",(long long)sp->tableid,(long long)other64bits,otherind,sp->mask);
    if ( sp->mask == (1 << sp->deck.numplayers)-1 )
    {
        printf("myind.%d got ready from ind.%d\n",sp->myind,juint(json,"myind"));
        if ( sp->myind == 0 )
            pangea_sendnext(plugin->mypriv,plugin->mypub,sp,1);
        else if ( sp->myind == sp->deck.numplayers-1 )
            pangea_sendnext(plugin->mypriv,plugin->mypub,sp,-1);
        return(1);
    } else return(0);
}

int32_t pangea_loadciphers(bits256 *ciphers,cJSON *array,int32_t n)
{
    int32_t i,nonz = 0; char *hexstr; bits256 pubkey;
    for (i=0; i<n; i++)
    {
        if ( (hexstr= jstr(jitem(array,i),0)) != 0 )
        {
            decode_hex(pubkey.bytes,sizeof(pubkey),hexstr);
            ciphers[nonz++] = pubkey;
        }
    }
    return(nonz);
}

int32_t pangea_checkstate(struct pangea_info *sp,int32_t state)
{
    int32_t i;
    for (i=0; i<sp->deck.numplayers; i++)
        if ( sp->states[i] != state )
            break;
    if ( i == sp->deck.numplayers )
    {
        sp->state = state;
        sp->statetimestamp = (uint32_t)time(NULL);
        sp->didturn = 0;
        return(state);
    }
    return(-1);
}

bits256 pangea_deal(struct pangea_deck *deck,int32_t myind,int32_t cardi,int32_t playerj,int32_t permiflag,bits256 cipher)
{
    int32_t i; struct pangea_player *player; struct pangea_card *cards;
    cards = (permiflag == 0) ? deck->cards : deck->permi;
    player = &deck->players[myind];
    //printf("%llx ",(long long)cipher.txid);
    for (i=0; i<52; i++)
    {
        if ( permiflag == 0 )
        {
            if ( memcmp(player->snaps[playerj][i].bytes,cipher.bytes,32) == 0 )
            {
                cipher = fcontract(fmul(fexpand(player->xoverz[playerj][i]),fexpand(cipher)));
                //printf("matched %d -> %llx/%llx\n",i,(long long)cipher.txid,(long long)cards[cardi].ciphers[j-1][playerj].txid);
                break;
            }
        }
        else
        {
            if ( memcmp(player->permi_snaps[i].bytes,cipher.bytes,32) == 0 )
            {
                cipher = fcontract(fmul(fexpand(player->permi_xoverz[i]),fexpand(cipher)));
                //printf("permi matched %d -> %llx/%llx\n",i,(long long)cipher.txid,(long long)cards[cardi].ciphers[j-1][playerj].txid);
                break;
            }
        }
    }
    if ( i == 52 )
        printf("decryption error: myind.%d cardi.%d playerj.%d permiflag.%d no match\n",myind,cardi,playerj,permiflag), memset(cipher.bytes,0,sizeof(cipher));
    return(cipher);
}

bits256 pangea_cardpriv(struct plugin_info *plugin,struct pangea_info *sp,struct pangea_deck *deck,int32_t myind,bits256 cipher,int32_t permiflag)
{
    bits256 myprivkey,bp,cardpriv,checkpub; struct pangea_card *cards; int32_t i;
    cards = (permiflag == 0) ? deck->cards : deck->permi;
    myprivkey = deck->players[myind].comm.priv;
    memset(bp.bytes,0,sizeof(bp));
    bp.bytes[0] = 9;
    for (i=0; i<52; i++)
    {
        cardpriv = pangea_encrypt(fexpand(cipher),myprivkey,cards[i].key.pub,1);
        checkpub = curve25519(cardpriv,bp);
        if ( memcmp(checkpub.bytes,cards[i].key.pub.bytes,sizeof(bits256)) == 0 )
        {
            //printf("%d ",cardpriv.bytes[1]);
            printf("decrypted card.%d %llx\n",cardpriv.bytes[1],(long long)cardpriv.txid);
            return(cardpriv);
        }
    }
    printf("error: did not match any card\n");
    memset(cardpriv.bytes,0,sizeof(cardpriv));
    return(cardpriv);
}

bits256 pangea_dealcard(struct plugin_info *plugin,struct pangea_info *sp,struct pangea_deck *deck,int32_t cardi,int32_t playerj,int32_t showflag,int32_t permiflag)
{
    bits256 cardpriv,cipher; int32_t j,firstj,lastj,incr; struct pangea_card *cards;
    if ( permiflag == 0 )
    {
        cards = deck->cards;
        firstj = deck->numplayers-1;
        lastj = 0;
        incr = -1;
    }
    else
    {
        cards = deck->permi;
        firstj = 0;
        lastj = deck->numplayers-1;
        incr = 1;
    }
    for (j=firstj; j!=lastj; j+=incr) // broadcast except when j == 1 and showflag == 0
    {
        // verify we should be decrypting this card!!
        cipher = pangea_deal(deck,j,cardi,playerj,permiflag,cards[cardi].ciphers[j][playerj]);
        if ( cipher.txid == 0 )
            break;
    }
    if ( cipher.txid != 0 )
        cardpriv = pangea_cardpriv(plugin,sp,deck,playerj,cipher,permiflag);
    else memset(cardpriv.bytes,0,sizeof(cardpriv));
    return(cardpriv);
}

bits256 pangea_pubkeys(struct plugin_info *plugin,struct pangea_info *sp,cJSON *json,char *str,struct pangea_card *cards)
{
    int32_t i,n; cJSON *array; char *hexstr; bits256 bp,pubkey,hash,check; bits320 prod,hexp;
    memset(check.bytes,0,sizeof(check));
    memset(bp.bytes,0,sizeof(bp)), bp.bytes[0] = 9;
    if ( (array= jarray(&n,json,str)) != 0 )
    {
        prod = fmul(fexpand(bp),crecip(fexpand(bp)));
        for (i=0; i<52; i++)
        {
            if ( (hexstr= jstr(jitem(array,i),0)) != 0 )
            {
                decode_hex(pubkey.bytes,sizeof(pubkey),hexstr);
                vcalc_sha256(0,hash.bytes,pubkey.bytes,sizeof(pubkey));
                cards[i].key.pub = pubkey;
                hash.bytes[0] &= 0xf8, hash.bytes[31] &= 0x7f, hash.bytes[31] |= 64;
                hexp = fexpand(hash);
                prod = fmul(prod,hexp);
            } else printf("no hexstr in cardpubs array[%d]\n",i);
        }
        if ( (hexstr= jstr(jitem(array,52),0)) != 0 )
        {
            check = fcontract(prod);
            decode_hex(pubkey.bytes,sizeof(pubkey),hexstr);
            if ( memcmp(check.bytes,pubkey.bytes,sizeof(check)) != 0 )
                printf("permicheck.%llx != prod.%llx\n",(long long)check.txid,(long long)pubkey.txid);
            else printf("permiprod matched\n");
        }
    }
    return(check);
}

int32_t pangea_encode(struct plugin_info *plugin,struct pangea_info *sp,cJSON *json,int32_t permiflag)
{
    bits256 ciphers[52*10]; int32_t otherind,n=0,i,k,nonz; cJSON *array; struct pangea_card *cards;
    cards = (permiflag == 0) ? sp->deck.cards : sp->deck.permi;
    sp->layers |= (1 << permiflag);
    if ( (otherind= juint(json,"myind")) >= 0 && otherind < sp->deck.numplayers && (array= jarray(&n,json,"ciphers")) != 0 )
    {
        if ( pangea_loadciphers(ciphers,array,n) == n && n == sp->deck.numplayers*((permiflag == 0) ? 52 : 1) )
        {
            for (i=nonz=0; i<52; i++)
            {
                k = (permiflag == 0) ? 0 : sp->deck.numplayers - 1;
                for (; k<sp->deck.numplayers; k++)
                    cards[i].ciphers[otherind][k] = ciphers[nonz++];
            }
        }
        else
        {
            printf("mismatched loadciphers for n.%d\n",n);
            return(0);
        }
    }
    printf("shuffle myind.%d got %d ciphers from otherind.%d\n",sp->myind,n,otherind);
    pangea_shuffle(&sp->deck,sp->myind,permiflag);
    pangea_layer(plugin,sp,&sp->deck,sp->myind,permiflag);
    if ( sp->layers == 3 )
    {
        printf("myind.%d: sent shamir's shares\n",sp->myind);
        pangea_sendmofn(plugin,sp,&sp->deck,sp->myind);
        sp->layers = 7;
    }
    return(0);
}

int32_t pangea_mofn(struct plugin_info *plugin,struct pangea_info *sp,cJSON *json)
{
    char *hexstr; int32_t otherind,n;
    if ( (hexstr= jstr(json,"data")) != 0 && (otherind= juint(json,"myind")) >= 0 && otherind < sp->deck.numplayers )
    {
        if ( sp->deck.mofn[otherind] != 0 )
            free(sp->deck.mofn[otherind]);
        n = (int32_t)strlen(hexstr);
        sp->deck.mofn[otherind] = malloc(n);
        printf("received otherind.%d mofn[%d]\n",otherind,n);
        decode_hex(sp->deck.mofn[otherind],n,hexstr);
        sp->states[otherind] = PANGEA_STATE_SENTMOFN;
        pangea_checkstate(sp,sp->states[otherind]);
        return(1);
    } else printf("mofn: no hexstr\n");
    return(0);
}

cJSON *pangea_item(cJSON *array,char *hexstr)
{
    if ( array == 0 )
        array = cJSON_CreateArray();
    jaddistr(array,hexstr);
    return(array);
}

cJSON *pangea_decode(bits256 *decodeciphers,struct plugin_info *plugin,struct pangea_info *sp,int32_t cardi,int32_t playerj,cJSON *array,bits256 cipher,int32_t permiflag,int32_t faceup)
{
    char hexstr[65],destNXT[64],*str; cJSON *item;
    if ( permiflag == 0 )
        sp->deck.players[sp->myind].ciphers[cardi] = cipher;
    else sp->deck.players[sp->myind].permiciphers[cardi] = cipher;
    sp->deck.players[sp->myind].cipherdests[cardi] = playerj;
    decodeciphers[cardi] = pangea_deal(&sp->deck,sp->myind,cardi,playerj,permiflag,cipher);
    init_hexbytes_noT(hexstr,decodeciphers[cardi].bytes,sizeof(bits256));
    if ( sp->myind != 1 && sp->myind != sp->deck.numplayers-1 )
        return(pangea_item(array,hexstr));
    else
    {
        item = cJSON_CreateObject();
        jaddstr(item,"card",hexstr);
        jaddnum(item,"cardi",cardi);
        jaddnum(item,"permiflag",permiflag);
        expand_nxt64bits(destNXT,sp->addrs[playerj]);
        if ( sp->myind == 1 )
        {
            if ( permiflag == 0 )
                str = "rawcard";
            else return(pangea_item(array,hexstr));
        } else str = (faceup != 0) ? "faceup" : "facedown";
        pangea_jsoncmd(destNXT,item,str,plugin,sp);
    }
    return(array);
}

int32_t pangea_preflop(struct plugin_info *plugin,struct pangea_info *sp,cJSON *origjson,int32_t permiflag)
{
    int32_t otherind,cardi,playerj,n,dir; char destNXT[64]; cJSON *array,*json; bits256 ciphers[9*2],decodeciphers[9*2];
    if ( (otherind= juint(origjson,"myind")) >= 0 && otherind < sp->deck.numplayers && (array= jarray(&n,origjson,"ciphers")) != 0 )
    {
        if ( pangea_loadciphers(ciphers,array,n) == n && n == sp->deck.numplayers*2 )
        {
            array = 0;
            dir = (permiflag == 0) ? 1 : -1;
            for (cardi=0,playerj=sp->button+1; cardi<sp->deck.numplayers*2; cardi++,playerj++)
            {
                playerj %= sp->deck.numplayers;
                array = pangea_decode(decodeciphers,plugin,sp,cardi,playerj,array,ciphers[cardi],permiflag,0);
            }
            json = cJSON_CreateObject();
            expand_nxt64bits(destNXT,sp->addrs[sp->myind + dir]);
            if ( sp->myind != 1 )
            {
                jadd(json,"ciphers",array);
                jaddnum(json,"permiflag",permiflag);
                pangea_jsoncmd(destNXT,json,"preflop",plugin,sp);
            }
        }
    }
    return(0);
}

int32_t pangea_faceups(struct plugin_info *plugin,struct pangea_info *sp,char *cmdstr,cJSON *origjson,int32_t permiflag)
{
    int32_t otherind,playerj,cardi,starti,n,dir; char destNXT[64]; cJSON *array,*json; bits256 ciphers[52],decodeciphers[52];
    if ( (otherind= juint(origjson,"myind")) >= 0 && otherind < sp->deck.numplayers && (array= jarray(&n,origjson,"ciphers")) != 0 )
    {
        if ( pangea_loadciphers(ciphers,array,n) == n && ((strcmp(cmdstr,"flop") == 0 && n == 3) || n == 1) )
        {
            array = 0;
            playerj = 0;
            dir = (permiflag == 0) ? 1 : -1;
            starti = sp->deck.numplayers*2;
            if ( strcmp(cmdstr,"turn") == 0 )
                starti += 3;
            else if ( strcmp(cmdstr,"river") == 0 )
                starti += 4;
            for (cardi=starti; cardi<starti+n; cardi++)
                array = pangea_decode(decodeciphers,plugin,sp,cardi,playerj,array,ciphers[cardi],permiflag,1);
            if ( sp->myind != sp->deck.numplayers-1 )
            {
                json = cJSON_CreateObject();
                expand_nxt64bits(destNXT,sp->addrs[sp->myind + dir]);
                jadd(json,"ciphers",array);
                if ( sp->myind == 0 )
                    jaddnum(json,"permiflag",1);
                pangea_jsoncmd(destNXT,json,cmdstr,plugin,sp);
            }
        }
    }
    return(0);
}

int32_t pangea_cardistate(struct pangea_info *sp,int32_t cardi)
{
    int32_t state;
    state = -1;
    if ( cardi == sp->deck.numplayers*2+2 )
        state = 3;
    else if ( cardi == sp->deck.numplayers*2+3 )
        state = 4;
    else if ( cardi == sp->deck.numplayers*2+4 )
        state = 5;
    return(state);
}

int32_t pangea_rawcard(struct plugin_info *plugin,struct pangea_info *sp,cJSON *json)
{
    int32_t otherind,card,cardi,state;
    otherind = juint(json,"myind");
    card = juint(json,"card");
    cardi = juint(json,"cardi");
    if ( (state= pangea_cardistate(sp,cardi)) > sp->states[otherind] )
    {
        sp->states[otherind] = state;
        pangea_checkstate(sp,sp->states[otherind]);
    }
    // start permi
    printf("myind.%d got showcard.%d from ind.%d | turn.%d %u\n",sp->myind,card,otherind,sp->state,sp->statetimestamp);
    return(0);
}

int32_t pangea_faceup(struct plugin_info *plugin,struct pangea_info *sp,cJSON *json)
{
    char buf[65],*hexstr; int32_t cardi; bits256 cipher,cardpriv;
    if ( (hexstr= jstr(json,"card")) != 0 && (cardi= juint(json,"cardi")) >= 2*sp->deck.numplayers && cardi < 2*sp->deck.numplayers+5 )
    {
        decode_hex(cipher.bytes,sizeof(cipher),hexstr);
        cardpriv = pangea_cardpriv(plugin,sp,&sp->deck,sp->myind,cipher,juint(json,"permiflag"));
        if ( cardpriv.txid != 0 )
        {
            sp->deck.faceups[cardi - 2*sp->deck.numplayers] = cardpriv.bytes[1];
            json = cJSON_CreateObject();
            init_hexbytes_noT(buf,cardpriv.bytes,sizeof(cardpriv));
            jaddnum(json,"card",cardpriv.bytes[1]);
            jaddnum(json,"cardi",cardi);
            jaddnum(json,"state",pangea_cardistate(sp,cardi));
            jaddstr(json,"cardpriv",buf);
            pangea_jsoncmd(0,json,"showcard",plugin,sp);
        }
    }
    return(0);
}

int32_t pangea_facedown(struct plugin_info *plugin,struct pangea_info *sp,cJSON *json)
{
    char *hexstr; int32_t cardi,state; bits256 cipher,cardpriv;
    if ( (hexstr= jstr(json,"card")) != 0 && (cardi= juint(json,"cardi")) >= 0 && cardi < 2*sp->deck.numplayers )
    {
        decode_hex(cipher.bytes,sizeof(cipher),hexstr);
        cardpriv = pangea_cardpriv(plugin,sp,&sp->deck,sp->myind,cipher,juint(json,"permiflag"));
        if ( cardpriv.txid != 0 )
        {
            state = cardi / sp->deck.numplayers;
            sp->deck.players[sp->myind].cards[state] = cardpriv.bytes[1];
            json = cJSON_CreateObject();
            jaddnum(json,"state",state+1);
            pangea_jsoncmd(0,json,"state",plugin,sp);
        }
    }
    return(0);
}

int32_t pangea_showcard(struct plugin_info *plugin,struct pangea_info *sp,cJSON *json)
{
    char *hexstr; int32_t cardi; bits256 cardpriv;
    if ( (hexstr= jstr(json,"cardpriv")) != 0 && (cardi= juint(json,"cardi")) >= 2*sp->deck.numplayers && cardi < 2*sp->deck.numplayers+5 )
    {
        decode_hex(cardpriv.bytes,sizeof(cardpriv),hexstr);
        if ( cardpriv.txid != 0 )
        {
            // update state
        }
    }
    return(0);
}

int32_t pangea_cmd(struct plugin_info *plugin,struct pangea_info *sp,char *cmdstr,cJSON *json)
{
    int32_t otherind;
    if ( strcmp(cmdstr,"ready") == 0 )
        return(pangea_ready(plugin,sp,json));
    else if ( strcmp(cmdstr,"state") == 0 )
    {
        otherind = juint(json,"myind");
        sp->states[otherind] = juint(json,"state");
        printf("myind.%d got turn.%d from ind.%d | turn.%d %u\n",sp->myind,sp->states[otherind],otherind,sp->state,sp->statetimestamp);
        return(pangea_checkstate(sp,sp->states[otherind]));
    }
    else if ( strcmp(cmdstr,"encode") == 0 )
        return(pangea_encode(plugin,sp,json,0));
    else if ( strcmp(cmdstr,"permipubs") == 0 )
    {
        sp->deck.permiprod = pangea_pubkeys(plugin,sp,json,"permipubs",sp->deck.permi);
        return(sp->deck.permiprod.txid != 0);
    }
    else if ( strcmp(cmdstr,"mofn") == 0 )
        return(pangea_mofn(plugin,sp,json));
    else if ( strcmp(cmdstr,"preflop") == 0 )
        return(pangea_preflop(plugin,sp,json,juint(json,"permiflag")));
    else if ( strcmp(cmdstr,"flop") == 0 && strcmp(cmdstr,"turn") == 0 && strcmp(cmdstr,"river") == 0 )
        return(pangea_faceups(plugin,sp,cmdstr,json,juint(json,"permiflag")));
    else if ( strcmp(cmdstr,"rawcard") == 0 )
        return(pangea_rawcard(plugin,sp,json));
    else if ( strcmp(cmdstr,"faceup") == 0 )
        return(pangea_faceup(plugin,sp,json));
    else if ( strcmp(cmdstr,"facedown") == 0 )
        return(pangea_facedown(plugin,sp,json));
    else if ( strcmp(cmdstr,"showcard") == 0 )
        return(pangea_showcard(plugin,sp,json));
    printf("unsupported cmdstr.(%s)\n",cmdstr);
    return(0);
}

int32_t pangea_recv(struct plugin_info *plugin,struct pangea_info *sp,void *data,int32_t datalen)
{
    cJSON *json = 0; char *cmdstr; uint64_t tableid; int32_t newlen = -1;
    if ( strncmp("{\"cmd\":",data,strlen("{\"cmd\":")) == 0 )
        json = cJSON_Parse(data);
    else if ( (newlen= pangea_decrypt(plugin->mypriv,plugin->nxt64bits,plugin->recvbuf,sizeof(plugin->recvbuf),data,datalen)) >= 0 )
    {
        data = plugin->recvbuf;
        if ( strncmp("{\"cmd\":",data,strlen("{\"cmd\":")) == 0 )
            json = cJSON_Parse(data);
    }
    if ( json != 0 )
    {
        if ( (tableid= j64bits(json,"tableid")) == sp->tableid && (cmdstr= jstr(json,"cmd")) != 0 )
            newlen = pangea_cmd(plugin,sp,cmdstr,json);
        free_json(json);
    }
    if ( newlen < 0 )
        queue_enqueue("pangeaQ",&pangeaQ,queueitem(data));
    return(newlen);
}

int32_t pangea_idle(struct plugin_info *plugin)
{
    static uint32_t lastpurge;
    struct pangea_info *sp; int32_t i,len,playerj,cardi,sendlen,flag,n=0; uint64_t tableid; cJSON *json,*array; bits256 ciphers[9*2];
    char *ptr,*msg,*jsonstr,*str,hexstr[65],destNXT[64]; uint32_t now = (uint32_t)time(NULL);
    for (i=0; i<sizeof(TABLES)/sizeof(*TABLES); i++)
    {
        if ( (sp= TABLES[i]) != 0 )
        {
            if ( sp->pullsock >= 0 && (len= nn_recv(sp->pullsock,&msg,NN_MSG,0)) > 0 )
            {
                printf("pullsock recv.%d\n",len);
                ptr = malloc(len);
                memcpy(ptr,msg,len);
                nn_freemsg(msg);
                sendlen = pangea_send(sp->pubsock,ptr,len);
                pangea_recv(plugin,sp,ptr,len);
                free(ptr);
                n++;
            }
            if ( sp->subsock >= 0 && (len= nn_recv(sp->subsock,&msg,NN_MSG,0)) > 0 )
            {
                printf("subsock recv.%d\n",len);
                ptr = malloc(len);
                memcpy(ptr,msg,len);
                nn_freemsg(msg);
                pangea_recv(plugin,sp,ptr,len);
                free(ptr);
                n++;
            }
            if ( n == 0 )
            {
                flag = 0;
                if ( (ptr= queue_dequeue(&pangeaQ,1)) != 0 )
                {
                    if ( (json= cJSON_Parse(ptr)) != 0 )
                    {
                        if ( (tableid= j64bits(json,"tableid")) != 0 )
                            pangea_recv(plugin,sp,ptr,(int32_t)strlen(ptr)+1), flag++;
                        free_json(json);
                    }
                    if ( flag == 0 )
                        queue_enqueue("pangeaQ",&pangeaQ,queueitem(ptr));
                }
            }
            /*if ( sp->state == 0 )
            {
                if ( sp->didturn == 0 && sp->myind == sp->deck.numplayers-1 )
                {
                    array = cJSON_CreateArray();
                    for (cardi=0,playerj=1; cardi<sp->deck.numplayers*2; cardi++,playerj++)
                    {
                        playerj %= sp->deck.numplayers;
                        ciphers[cardi] = pangea_deal(&sp->deck,sp->myind,cardi,playerj,0,sp->deck.cards[cardi].ciphers[sp->myind][playerj]);
                        sp->deck.players[sp->myind].ciphers[cardi] = ciphers[cardi];
                        sp->deck.players[sp->myind].cipherdests[cardi] = playerj;
                        init_hexbytes_noT(hexstr,ciphers[cardi].bytes,sizeof(bits256));
                        jaddistr(array,hexstr);
                    }
                    json = pangea_jsoncmd("preflop",plugin,sp);
                    jadd(json,"ciphers",array);
                    jsonstr = jprint(json,1);
                    expand_nxt64bits(destNXT,sp->addrs[sp->myind - 1]);
                    pangea_PM(sp,plugin->mypriv,plugin->mypub,destNXT,(void *)jsonstr,(int32_t)strlen(jsonstr)+1);
                    printf("myind.%d send preflop permi.%d %d to j.%d destNXT.%p\n",sp->myind,0,(int32_t)strlen(jsonstr)+1,playerj,destNXT);
                    free(jsonstr);
                    sp->didturn = 1;
                }
            }
            else if ( sp->state == 2 )
            {
                if ( sp->didturn == 0 && sp->myind == sp->deck.numplayers-1 )
                {
                    array = cJSON_CreateArray();
                    playerj = 0;
                    for (cardi=sp->deck.numplayers*2; cardi<sp->deck.numplayers*2+3; cardi++)
                    {
                        ciphers[cardi] = pangea_deal(&sp->deck,sp->myind,cardi,playerj,0,sp->deck.cards[cardi].ciphers[sp->myind][playerj]);
                        sp->deck.players[sp->myind].ciphers[cardi] = ciphers[cardi];
                        sp->deck.players[sp->myind].cipherdests[cardi] = playerj;
                        init_hexbytes_noT(hexstr,ciphers[cardi].bytes,sizeof(bits256));
                        jaddistr(array,hexstr);
                    }
                    json = pangea_jsoncmd("flop",plugin,sp);
                    jadd(json,"ciphers",array);
                    jsonstr = jprint(json,1);
                    pangea_PM(sp,plugin->mypriv,plugin->mypub,0,(void *)jsonstr,(int32_t)strlen(jsonstr)+1);
                    printf("myind.%d send flop permi.%d %d\n",sp->myind,0,(int32_t)strlen(jsonstr)+1);
                    free(jsonstr);
                    sp->didturn = 1;
                }
            }
            else if ( sp->state == 3 || sp->state == 4 )
            {
                if ( sp->didturn == 0 && sp->myind == sp->deck.numplayers-1 )
                {
                    if ( sp->state == 3 )
                        str = "turn", cardi = sp->deck.numplayers*2 + 3;
                    else str = "river", cardi = sp->deck.numplayers*2 + 4;
                    array = cJSON_CreateArray();
                    playerj = 0;
                    ciphers[cardi] = pangea_deal(&sp->deck,sp->myind,cardi,playerj,0,sp->deck.cards[cardi].ciphers[sp->myind][playerj]);
                    sp->deck.players[sp->myind].ciphers[cardi] = ciphers[cardi];
                    sp->deck.players[sp->myind].cipherdests[cardi] = playerj;
                    init_hexbytes_noT(hexstr,ciphers[cardi].bytes,sizeof(bits256));
                    jaddistr(array,hexstr);
                    json = pangea_jsoncmd(str,plugin,sp);
                    jadd(json,"ciphers",array);
                    jsonstr = jprint(json,1);
                    pangea_PM(sp,plugin->mypriv,plugin->mypub,0,(void *)jsonstr,(int32_t)strlen(jsonstr)+1);
                    printf("myind.%d send %s permi.%d %d\n",sp->myind,str,0,(int32_t)strlen(jsonstr)+1);
                    free(jsonstr);
                    sp->didturn = 1;
                }
            }
            else if ( sp->state == 5 )
            {
                // evaluate hands
            }*/
        }
    }
    if ( now > lastpurge+60 )
    {
        lastpurge = now;
        for (i=0; i<sizeof(TABLES)/sizeof(*TABLES); i++)
            if ( TABLES[i] != 0 && TABLES[i]->done != 0 && now > TABLES[i]->timestamp+3600 )
                pangea_free(TABLES[i]);
    }
    return(n);
}

struct pangea_info *pangea_create(uint64_t my64bits,int32_t *createdflagp,char *base,uint32_t timestamp,uint64_t *addrs,int32_t numaddrs,uint64_t bigblind,uint64_t ante)
{
    struct pangea_info *sp = 0; bits256 hash; int32_t i,firstslot = -1;
    if ( createdflagp != 0 )
        *createdflagp = 0;
    for (i=0; i<numaddrs; i++)
        if ( addrs[i] == my64bits )
            break;
    if ( i == numaddrs )
    {
        printf("this node not in addrs\n");
        return(0);
    }
    if ( numaddrs > 0 && (sp= calloc(1,sizeof(*sp))) != 0 )
    {
        sp->myind = i;
        for (i=0; i<numaddrs; i++)
            sp->states[i] = PANGEA_STATE_INIT;
        sp->state = PANGEA_STATE_INIT;
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
                if ( sp->tableid == TABLES[i]->tableid )
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

cJSON *pangea_ciphersjson(struct pangea_deck *deck,int32_t permiflag,int32_t myplayerj)
{
    int32_t i,j; struct pangea_card *cards; char hexstr[65]; cJSON *array = cJSON_CreateArray();
    cards = (permiflag != 0) ? deck->permi : deck->cards;
    for (i=0; i<52; i++)
        for (j=0; j<deck->numplayers; j++)
        {
            init_hexbytes_noT(hexstr,cards[i].ciphers[myplayerj][j].bytes,sizeof(bits256));
            jaddistr(array,hexstr);
        }
    return(array);
}

cJSON *pangea_cardpubs(struct pangea_deck *deck,int32_t permiflag)
{
    int32_t i; char hexstr[65]; struct pangea_card *cards; cJSON *array = cJSON_CreateArray();
    cards = (permiflag == 0) ? deck->cards : deck->permi;
    for (i=0; i<52; i++)
    {
        init_hexbytes_noT(hexstr,cards[i].key.pub.bytes,sizeof(bits256));
        jaddistr(array,hexstr);
    }
    init_hexbytes_noT(hexstr,(permiflag == 0) ? deck->checkprod.bytes : deck->permiprod.bytes,sizeof(bits256));
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

int32_t pangea_start(char *retbuf,char *ipaddr,uint16_t port,uint64_t my64bits,char *base,uint32_t timestamp,uint64_t bigblind,uint64_t ante,int32_t maxplayers)
{
    char *addrstr,*ciphers,*cardpubs,*sharenrs; uint8_t p2shtype; cJSON *array; struct InstantDEX_quote *iQ = 0;
    int32_t createdflag,addrtype,i,j,r,num=0,myind = -1; uint64_t addrs[512],quoteid = 0; struct pangea_info *sp;
    if ( base == 0 || base[0] == 0 || maxplayers < 3 || maxplayers > 9 || ipaddr == 0 || ipaddr[0] == 0 || port == 0 )
        return(-1);
    addrtype = coin777_addrtype(&p2shtype,base);
    if ( (num= uniq_specialaddrs(&myind,addrs,sizeof(addrs)/sizeof(*addrs),base,"pangea",addrtype)) < 2 )
    {
        printf("need at least 2 players\n");
        return(-1);
    }
    printf("pangea_start(%s) myind.%d num.%d\n",base,myind,num);
    if ( (i= myind) > 0 )
    {
        addrs[i] = addrs[0];
        addrs[0] = my64bits;
        i = 0;
    }
    while ( num > maxplayers )
    {
        r = (rand() % (num-1));
        printf("swap out %d of %d\n",r+1,num);
        addrs[r + 1] = addrs[--num];
    }
    printf("pangea numplayers.%d\n",num);
    if ( (sp= pangea_create(my64bits,&createdflag,base,timestamp,addrs,num,bigblind,ante)) == 0 )
    {
        printf("cant create shuffle.(%s) numaddrs.%d\n",base,num);
        return(-1);
    }
    if ( createdflag != 0 && sp->myind == 0 && addrs[sp->myind] == my64bits )
    {
        printf("inside\n");
        if ( quoteid == 0 )
        {
            if ( (array= InstantDEX_specialorders(&quoteid,my64bits,base,"pangea",bigblind,addrtype)) != 0 )
                free_json(array);
        }
        printf("quoteid.%llu\n",(long long)quoteid);
        if ( (iQ= find_iQ(quoteid)) != 0 )
        {
            iQ->s.pending = 1;
            memset(&sp->deck,0,sizeof(sp->deck));
            sp->pullsock = sp->pubsock = sp->subsock = -1;
            for (j=0; j<num; j++)
            {
                sp->deck.players[j].comm.priv = keypair(&sp->deck.players[j].comm.pub);
                printf("player.%d: priv.%llx pub.%llx\n",j,(long long)sp->deck.players[j].comm.priv.txid,(long long)sp->deck.players[j].comm.pub.txid);
            }
            sp->deck.numplayers = num;
            sp->deck.N = num;
            sp->deck.M = (num >> 1) + 1;
            init_sharenrs(sp->deck.sharenrs,0,sp->deck.N,sp->deck.N); // this needs to be done to start a hand
            sp->deck.checkprod = pangea_initdeck(&sp->deck,num);
            addrstr = jprint(addrs_jsonarray(addrs,num),1);
            ciphers = jprint(pangea_ciphersjson(&sp->deck,0,0),1);
            cardpubs = jprint(pangea_cardpubs(&sp->deck,0),1);
            sharenrs = jprint(pangea_sharenrs(sp->deck.sharenrs,num),1);
            sprintf(retbuf,"{\"myind\":%d,\"ipaddr\":\"%s\",\"port\":%u,\"plugin\":\"relay\",\"destplugin\":\"pangea\",\"method\":\"busdata\",\"submethod\":\"newtable\",\"pluginrequest\":\"SuperNET\",\"my64bits\":\"%llu\",\"tableid\":\"%llu\",\"timestamp\":%u,\"M\":%d,\"N\":%d,\"base\":\"%s\",\"bigblind\":\"%llu\",\"ante\":\"%llu\",\"cardpubs\":%s,\"addrs\":%s,\"sharenrs\":%s}",sp->myind,ipaddr,port,(long long)my64bits,(long long)sp->tableid,sp->timestamp,sp->deck.M,sp->deck.N,sp->base,(long long)bigblind,(long long)ante,cardpubs,addrstr,sharenrs);
            sprintf((char *)sp->sendbuf,"{\"cmd\":\"encode\",\"myind\":%d,\"my64bits\":\"%llu\",\"tableid\":\"%llu\",\"timestamp\":%u,\"M\":%d,\"N\":%d,\"base\":\"%s\",\"bigblind\":\"%llu\",\"ante\":\"%llu\",\"ciphers\":%s}",sp->myind,(long long)my64bits,(long long)sp->tableid,sp->timestamp,sp->deck.M,sp->deck.N,sp->base,(long long)sp->bigblind,(long long)sp->ante,ciphers);
            sp->sendlen = (int32_t)strlen((char *)sp->sendbuf) + 1;
            free(addrstr), free(ciphers), free(cardpubs), free(sharenrs);
            sp->quoteid = iQ->s.quoteid;
            strcpy(sp->ipaddr,ipaddr), sp->port = port;
            sprintf(sp->endpoint,"tcp://%s:%u",sp->ipaddr,sp->port+1);
            sp->pullsock = nn_createsocket(sp->endpoint,1,"NN_PULL",NN_PULL,sp->port,10,10);
            sprintf(sp->endpoint,"tcp://%s:%u",sp->ipaddr,sp->port);
            sp->pubsock = nn_createsocket(sp->endpoint,1,"NN_PUB",NN_PUB,sp->port,10,10);
            return(0);
        }
    }
    return(0);
}

char *pangea_newtable(struct plugin_info *plugin,cJSON *json)
{
    int32_t createdflag,num,i,n,addrtype,port,myind = -1; uint64_t tableid,quoteid=0,addrs[9]; uint8_t p2shtype; bits256 bp;
    char *base,*ipaddr,*ciphers,*permipubs,cmd[512]; uint32_t timestamp; struct pangea_info *sp; cJSON *array; struct InstantDEX_quote *iQ;
    if ( (tableid= j64bits(json,"tableid")) != 0 && (base= jstr(json,"base")) != 0 && (timestamp= juint(json,"timestamp")) != 0 )
    {
        if ( (array= jarray(&num,json,"addrs")) == 0 || num < 3 || num > 9 )
        {
            printf("no address or illegal num.%d\n",num);
            return(clonestr("{\"error\":\"no addrs or illegal numplayers\"}"));
        }
        for (i=0; i<num; i++)
        {
            addrs[i] = j64bits(jitem(array,i),0);
            if ( addrs[i] == plugin->nxt64bits )
                myind = i;
        }
        free_json(array);
        if ( (sp= pangea_create(plugin->nxt64bits,&createdflag,base,timestamp,addrs,num,j64bits(json,"bigblind"),j64bits(json,"ante"))) == 0 )
        {
            printf("cant create table.(%s) numaddrs.%d\n",base,num);
            return(clonestr("{\"error\":\"cant create table\"}"));
        }
        sp->myind = myind;
        if ( createdflag != 0 && addrs[sp->myind] == plugin->nxt64bits )
        {
            printf("inside\n");
            if ( quoteid == 0 )
            {
                addrtype = coin777_addrtype(&p2shtype,base);
                if ( (array= InstantDEX_specialorders(&quoteid,plugin->nxt64bits,base,"pangea",sp->bigblind,addrtype)) != 0 )
                    free_json(array);
            }
            printf("quoteid.%llu\n",(long long)quoteid);
            if ( (iQ= find_iQ(quoteid)) != 0 )
            {
                memset(&sp->deck,0,sizeof(sp->deck));
                memset(bp.bytes,0,sizeof(bp)), bp.bytes[0] = 9;
                sp->deck.numplayers = num;
                sp->deck.N = juint(json,"N");
                sp->deck.M = juint(json,"M");
                iQ->s.pending = 1;
                sp->deck.checkprod = pangea_pubkeys(plugin,sp,json,"cardpubs",sp->deck.cards);
                if ( (array= jarray(&n,json,"sharenrs")) == 0 || n != sp->deck.N )
                {
                    printf("invalid sharenrs n.%d\n",n);
                    return(clonestr("{\"error\":\"invalid sharenrs\"}"));
                }
                for (i=0; i<n; i++)
                    sp->deck.sharenrs[i] = juint(jitem(array,i),0);
                sp->mask = ((1 << sp->myind) | 1);
                if ( sp->myind == sp->deck.numplayers-1 )
                {
                    sp->deck.permiprod = pangea_initdeck(&sp->deck,0);
                    ciphers = jprint(pangea_ciphersjson(&sp->deck,1,sp->myind),1);
                    sprintf((char *)sp->sendbuf,"{\"cmd\":\"encode\",\"myind\":%d,\"my64bits\":\"%llu\",\"tableid\":\"%llu\",\"timestamp\":%u,\"M\":%d,\"N\":%d,\"base\":\"%s\",\"bigblind\":\"%llu\",\"ante\":\"%llu\",\"ciphers\":%s,\"permiflag\":1}",sp->myind,(long long)plugin->nxt64bits,(long long)sp->tableid,sp->timestamp,sp->deck.M,sp->deck.N,sp->base,(long long)sp->bigblind,(long long)sp->ante,ciphers);
                    free(ciphers);
                    permipubs = jprint(pangea_cardpubs(&sp->deck,1),1);
                    sprintf((char *)cmd,"{\"cmd\":\"permipubs\",\"myind\":%d,\"my64bits\":\"%llu\",\"tableid\":\"%llu\",\"timestamp\":%u,\"M\":%d,\"N\":%d,\"base\":\"%s\",\"bigblind\":\"%llu\",\"ante\":\"%llu\",\"permipubs\":%s}",sp->myind,(long long)plugin->nxt64bits,(long long)sp->tableid,sp->timestamp,sp->deck.M,sp->deck.N,sp->base,(long long)sp->bigblind,(long long)sp->ante,permipubs);
                    pangea_send(sp->pushsock,cmd,(int32_t)strlen(cmd)+1);
                    free(permipubs);
                }
                if ( (ipaddr= jstr(json,"ipaddr")) != 0 && (port= juint(json,"port")) != 0 )
                {
                    strcpy(sp->ipaddr,ipaddr), sp->port = port;
                    sprintf(sp->endpoint,"tcp://%s:%u",sp->ipaddr,sp->port+1);
                    sp->pushsock = nn_createsocket(sp->endpoint,1,"NN_PUSH",NN_PUSH,sp->port,10,10);
                    sprintf(sp->endpoint,"tcp://%s:%u",sp->ipaddr,sp->port);
                    sp->subsock = nn_createsocket(sp->endpoint,1,"NN_SUB",NN_PUB,sp->port,10,10);
                    nn_setsockopt(sp->subsock,NN_SUB,NN_SUB_SUBSCRIBE,"",0);
                }
                if ( sp->pushsock >= 0 )
                {
                    sprintf(cmd,"{\"cmd\":\"ready\",\"myind\":%d,\"my64bits\":\"%llu\",\"tableid\":\"%llu\",\"timestamp\":%u,\"M\":%d,\"N\":%d,\"base\":\"%s\",\"bigblind\":\"%llu\",\"ante\":\"%llu\",\"mask\":%d}",sp->myind,(long long)plugin->nxt64bits,(long long)sp->tableid,sp->timestamp,sp->deck.M,sp->deck.N,sp->base,(long long)sp->bigblind,(long long)sp->ante,sp->mask);
                    pangea_send(sp->pushsock,cmd,(int32_t)strlen(cmd)+1);
                }
                sp->state = PANGEA_STATE_READY;
            }
        }
    }
    return(clonestr("{\"error\":\"no tableid\"}"));
    //sprintf(retbuf,"{\"plugin\":\"relay\",\"destplugin\":\"pangea\",\"method\":\"busdata\",\"submethod\":\"newtable\",\"pluginrequest\":\"SuperNET\",\"my64bits\":\"%llu\",\"tableid\":\"%llu\",\"timestamp\":%u,\"M\":%d,\"N\":%d,\"base\":\"%s\",\"bigblind\":\"%llu\",\"ante\":\"%llu\",\"cardpubs\":%s,\"addrs\":%s,\"sharenrs\":%s}",(long long)my64bits,(long long)sp->tableid,sp->timestamp,sp->deck.M,sp->deck.N,sp->base,(long long)bigblind,(long long)ante,cardpubs,addrstr,sharenrs);
}

int pangea_main()
{
    struct pangea_deck deck; bits256 cardpriv,permipriv,players[9],checkprod,permiprod;
    int32_t i,j,h,numplayers,starttime,order[52],permi[52],final[52]; uint64_t mask,maskp;
    starttime = (int32_t)time(NULL);
    numplayers = sizeof(players)/sizeof(*players);
    printf("sizeof deck %ld start.%u\n",sizeof(deck),starttime);
    for (h=0; h<10; h++)
    {
        mask = maskp = 0;
        memset(&deck,0,sizeof(deck));
        for (j=0; j<numplayers; j++)
        {
            deck.players[j].comm.priv = keypair(&deck.players[j].comm.pub);
            printf("player.%d: priv.%llx pub.%llx\n",j,(long long)deck.players[j].comm.priv.txid,(long long)deck.players[j].comm.pub.txid);
        }
        deck.numplayers = numplayers;
        deck.N = numplayers;
        deck.M = numplayers/2;
        init_sharenrs(deck.sharenrs,0,deck.N,deck.N); // this needs to be done to start a hand
        
        //if ( playerj == 0 )
            checkprod = pangea_initdeck(&deck,numplayers);
        for (j=1; j<deck.numplayers; j++) // upon recv msg
        {
            pangea_shuffle(&deck,j,0);
            pangea_layer(0,0,&deck,j,0);
        }
        //if ( playerj == numplayers-1 )
            permiprod = pangea_initdeck(&deck,0);
        for (j=deck.numplayers-2; j>=0; j--) // upon recv msg
        {
            pangea_shuffle(&deck,j,1);
            pangea_layer(0,0,&deck,j,1);
        }
        for (j=0; j<numplayers; j++) // when node gets both directions
            pangea_sendmofn(0,0,&deck,j);
        for (i=0; i<52; i++) // 2 hole cards per player, after player makes decision, flop, etc
        {
            randombytes((void *)&j,4);
            if ( j < 0 )
                j = -j;
            j %= numplayers;
            //printf("deal card.%d to player.%d -> ",i,j);
            cardpriv = pangea_dealcard(0,0,&deck,i,j,1,0);
            if ( cardpriv.txid != 0 && cardpriv.bytes[1] < 52 )
            {
                mask |= (1LL << cardpriv.bytes[1]);
                order[i] = cardpriv.bytes[1];
                printf("%2d ",order[i]);
                permipriv = pangea_dealcard(0,0,&deck,cardpriv.bytes[1],numplayers-1,1,1);
                if ( permipriv.txid != 0 && permipriv.bytes[1] < 52 )
                {
                    permi[cardpriv.bytes[1]] = permipriv.bytes[1];
                    final[i] = permi[cardpriv.bytes[1]];
                    //printf("rawcard.%d -> %d\n",cardpriv.bytes[1],permipriv.bytes[1]);
                    maskp |= (1LL << permipriv.bytes[1]);
                }
            }
        }
        printf("raworder\n");
        for (i=0; i<52; i++)
            printf("%2d ",permi[i]);
        printf("permi\n");
        for (i=0; i<52; i++)
            printf("%2d ",final[i]);
        printf("final\n");
        printf("mask.%llx vs permi.%llx elapsed %d seconds for %d hands\n",(long long)mask,(long long)maskp,(int32_t)time(NULL)-starttime,h);
    }
    return(0);
}

int32_t PLUGNAME(_process_json)(char *forwarder,char *sender,int32_t valid,struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag,char *tokenstr)
{
    char *resultstr,*methodstr,*base,*retstr = 0; int32_t maxplayers;
    retbuf[0] = 0;
    printf("<<<<<<<<<<<< INSIDE PLUGIN! process %s (%s)\n",plugin->name,jsonstr);
    if ( initflag > 0 )
    {
        PANGEA.readyflag = 1;
        plugin->sleepmillis = 25;
        plugin->allowremote = 1;
        plugin->nxt64bits = set_account_NXTSECRET(plugin->mypriv,plugin->mypub,plugin->NXTACCT,plugin->NXTADDR,plugin->NXTACCTSECRET,sizeof(plugin->NXTACCTSECRET),json,0,0,0);
        printf("my64bits %llu ipaddr.%s\n",(long long)plugin->nxt64bits,plugin->ipaddr);
        //pangea_main();
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
                if ( (maxplayers= juint(json,"maxplayers")) < 3 )
                    maxplayers = 3;
                else if ( maxplayers > 9 )
                    maxplayers = 9;
                pangea_start(retbuf,plugin->ipaddr,plugin->pangeaport,plugin->nxt64bits,base,0,j64bits(json,"bigblind"),j64bits(json,"ante"),maxplayers);
            } else strcpy(retbuf,"{\"error\":\"no base specified\"}");
        }
        else if ( strcmp(methodstr,"newtable") == 0 )
            retstr = pangea_newtable(plugin,json);
    }
    return(plugin_copyretstr(retbuf,maxlen,retstr));
}

#include "../agents/plugin777.c"
