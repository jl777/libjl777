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

#define BUNDLED
#define PLUGINSTR "jumblr"
#define PLUGNAME(NAME) jumblr ## NAME
#define STRUCTNAME struct PLUGNAME(_info) 
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
#include "../includes/portable777.h"
#include "../coins/coins777.c"
#include "../utils/ramcoder.c"
#include "plugin777.c"
#undef DEFINES_ONLY

STRUCTNAME
{
    uint32_t timestamp,numaddrs;
    uint64_t basebits,addrs[64],shuffleid,quoteid,fee,unspent;
    char signedtx[65536],rawtx[65536],base[16],destaddr[64],changeaddr[64],inputtxid[128],vinaddr[128],vinpubP[128],scriptPubKey[4096]; void *vinkey;
    char sigs[64][512],*vinstr,*voutstr,*changestr,*cointxid;
    int32_t vin,myind,srcacct,done; uint64_t change,amount,sigmask;
    struct cointx_info *T;
} *SHUFFLES[1000];

int32_t jumblr_encrypt(uint64_t destbits,uint8_t *dest,uint8_t *src,int32_t len)
{
    uint8_t *cipher; bits256 destpubkey,onetime_pubkey,onetime_privkey,seed; HUFF H,*hp = &H; char destNXT[64]; int32_t haspubkey,cipherlen;
    expand_nxt64bits(destNXT,destbits);
    destpubkey = issue_getpubkey(&haspubkey,destNXT);
    if ( haspubkey == 0 )
    {
        printf("%s doesnt have pubkey, cant shuffle with him\n",destNXT);
        return(-1);
    }
    crypto_box_keypair(onetime_pubkey.bytes,onetime_privkey.bytes);
    memset(seed.bytes,0,sizeof(seed));
    seed.bytes[0] = 1;
    if ( (cipher= encode_str(&cipherlen,src,len,destpubkey,onetime_privkey,onetime_pubkey)) != 0 )
    {
        memset(dest,0,cipherlen*2);
        _init_HUFF(hp,cipherlen*2,dest);
        ramcoder_encoder(0,1,cipher,cipherlen,hp,&seed);
        cipherlen = hconv_bitlen(hp->bitoffset);
        free(cipher);
    } else cipherlen = 0;
    return(cipherlen);
}

int32_t jumblr_decrypt(uint64_t nxt64bits,uint8_t *dest,int32_t maxlen,uint8_t *src,int32_t len)
{
    bits256 seed; uint8_t buf[32768]; int32_t newlen = -1; HUFF H,*hp = &H;
    if ( nxt64bits == SUPERNET.my64bits )
    {
        memset(seed.bytes,0,sizeof(seed)), seed.bytes[0] = 1;
        _init_HUFF(hp,len,src), hp->endpos = len << 3;
        newlen = ramcoder_decoder(0,1,buf,sizeof(buf),hp,&seed);
        if ( decode_cipher((void *)dest,buf,&newlen,SUPERNET.myprivkey) != 0 )
            printf("jumblr_decrypt Error: decode_cipher error len.%d -> newlen.%d\n",len,newlen);
    } else printf("cant decrypt another accounts packet\n");
    return(newlen);
}

uint64_t jumblr_txfee(struct coin777 *coin,int32_t numvins,int32_t numvouts)
{
    int32_t estimatedsize,incr;
    estimatedsize = (numvins * 130) + (numvouts * 50);
    incr = (estimatedsize / 200);
    return(coin->mgw.txfee * (incr+1));
}

char *jumblr_onetimeaddress(char *pubkey,struct coin777 *coin,char *account)
{
    char coinaddr[128],acctstr[128],*retstr; struct destbuf tmp;
    sprintf(acctstr,"\"%s\"",account);
    pubkey[0] = 0;
    if ( (retstr= bitcoind_passthru(coin->name,coin->serverport,coin->userpass,"getnewaddress",acctstr)) != 0 )
    {
        strcpy(coinaddr,retstr);
        if ( get_pubkey(&tmp,coin->name,coin->serverport,coin->userpass,coinaddr) > 0 )
            strcpy(pubkey,tmp.buf);
    }
    return(retstr);
}

int32_t jumblr_peel(char *peeled[],cJSON *strs,int32_t num)
{
    int32_t i,len,n; void *dest,*src; char *hexstr,*str;
    for (i=0; i<num; i++)
    {
        if ( (str= jstr(jitem(strs,i),0)) != 0 )
        {
            //printf("peel.(%s) -> ",str);
            len = (int32_t)strlen(str) >> 1;
            src = calloc(1,len+16);
            dest = calloc(1,2*len+16);
            decode_hex(src,len,str);
            n = jumblr_decrypt(SUPERNET.my64bits,dest,2*len,src,len);
            hexstr = calloc(1,n*2+1);
            init_hexbytes_noT(hexstr,dest,n);
            free(src), free(dest);
            peeled[i] = hexstr;
            //printf("(%s)\n",hexstr);
        }
        else
        {
            printf("jumblr_peel: cant extract strs[%d]\n",i);
            return(-1);
        }
    }
    return(num);
}

char *jumblr_layer(char *str,uint64_t *addrs,int32_t num)
{
    int32_t i,n,len; uint8_t data[8192],dest[8192];
    len = (int32_t)strlen(str) >> 1;
    if ( len > sizeof(dest)/2 )
    {
        printf("jumblr_layer str.(%s) is too big\n",str);
        return(0);
    }
    decode_hex(data,len,str);
    //printf("layer.(%s) len.%d num.%d %llx\n",str,len,num,*(long long *)data);
    if ( num > 0 )
    {
        for (i=num-1; i>=0; i--)
        {
            n = jumblr_encrypt(addrs[i],dest,data,len);
            memcpy(data,dest,n);
            len = n;
        }
    }
    init_hexbytes_noT((char *)dest,data,len);
    //printf("(%s) newlen.%d\n",(char *)dest,len);
    return(clonestr((char *)dest));
}

int32_t jumblr_decodevin(struct cointx_input *vin,char *vinstr)
{
    uint8_t vout,scriptlen; char *scriptPubKey,*txid;
    decode_hex(&vout,1,vinstr);
    decode_hex(&scriptlen,1,vinstr+2);
    scriptPubKey = vinstr + 4;
    memcpy(vin->sigs,scriptPubKey,scriptlen << 1);
    vin->sigs[scriptlen << 1] = 0;
    txid = scriptPubKey + (scriptlen << 1);
    safecopy(vin->tx.txidstr,txid,sizeof(vin->tx.txidstr));
    vin->tx.vout = vout;
    vin->sequence = 0xffffffff;
    return(0);
}

char *jumblr_encodevin(uint64_t *changep,char *vintxid,int32_t vout,uint64_t unspent,char *scriptPubKey,struct coin777 *coin,uint64_t amount,uint64_t *addrs,int32_t num)
{
    struct rawvin vin; int32_t scriptlen; char buf[4096],*retstr = 0;
    memset(&vin,0,sizeof(vin));
    *changep = 0;
    if ( unspent >= amount  )
    {
        if ( (scriptlen= (int32_t)strlen(scriptPubKey)) >= (0xfd<<1) || (scriptlen & 1) != 0 )
        {
            printf("script too big to fit.(%s)\n",scriptPubKey);
            return(0);
        }
        *changep = (unspent - amount);
        sprintf(buf,"%02x%02x%s%s",vout,scriptlen >> 1,scriptPubKey,vintxid);
        printf("jumblr_vin.(%s)<- (%s v%d) amount %.8f sum %.8f change %.8f\n",buf,vintxid,vout,dstr(amount),dstr(unspent),dstr(*changep));
        retstr = jumblr_layer(buf,addrs,num);
    }
    return(retstr);
}

char *jumblr_encodevout(char *hexaddress,char *destaddress,struct coin777 *coin,uint64_t amount,uint64_t *addrs,int32_t num)
{
    char buf[512],*retstr = 0; uint64_t x; int32_t j; uint8_t val;
    if ( hexaddress[0] != 0 )
    {
        x = amount;
        for (j=0; j<8; j++,x>>=8)
        {
            val = (x & 0xff);
            init_hexbytes_noT(&buf[j*2],&val,1);
        }
        buf[j*2] = 0;
        strcat(buf,hexaddress);
        printf("(%s %.8f -> %s) ",destaddress,dstr(amount),buf);
        retstr = jumblr_layer(buf,addrs,num);
    }
    return(retstr);
}

int32_t jumblr_decodevout(struct rawvout *vout,char *voutstr)
{
    uint64_t value; int32_t j; uint8_t data[8],rmd160[21];
    decode_hex(data,8,voutstr);
    for (value=j=0; j<8; j++)
    {
        value = (value << 8) | data[7-j];
        //printf("{%02x} ",data[7-j]);
    }
    //printf("decode.(%s %.8f %llx)\n",vouts[i] + 16,dstr(value),(long long)value);
    decode_hex(rmd160,21,voutstr + 16);
    if ( btc_convrmd160(vout->coinaddr,rmd160[0],rmd160+1) == 0 )
    {
        sprintf(vout->script,"76a914%s88ac",voutstr+18); // only pkhash supported for jumblr vouts
        vout->value = value;
        return(0);
        //printf("%d.(%s %s %.8f) ",T->numoutputs,v->coinaddr,v->script,dstr(value));
    } else printf("error converting rmd160.(%s)\n",vout->coinaddr);
    return(-1);
}

int32_t jumblr_next(struct jumblr_info *sp,struct coin777 *coin,uint64_t *addrs,int32_t num,int32_t i,uint64_t baseamount,int32_t sourceacct)
{
    sp->amount = baseamount;
    sp->fee = (sp->amount >> 10);
    if ( coin->jvinkey == 0 || coin->jpubP[0] == 0 || coin->junspent < baseamount || coin->jvintxid[0] == 0 || coin->jvin < 0 || coin->jscriptPubKey[0] == 0 || coin->jvinaddr[0] == 0 || coin->jchangehex[0] == 0 || coin->jchangeaddr[0] == 0 || coin->jdesthex[0] == 0 || coin->jdestaddr[0] == 0 )
    {
        printf(" jumblr vintxid or dest not ready\n");
        return(-1);
    }
    sp->vinstr = jumblr_encodevin(&sp->change,coin->jvintxid,coin->jvin,coin->junspent,coin->jscriptPubKey,coin,sp->amount + sp->fee + 2*coin->mgw.txfee,&addrs[i+1],num-i-1);
    strcpy(sp->inputtxid,coin->jvintxid), coin->jvintxid[0] = 0;
    sp->vin = coin->jvin, coin->jvin = -1;
    sp->unspent = coin->junspent, coin->junspent = 0;
    strcpy(sp->scriptPubKey,coin->jscriptPubKey);
    strcpy(sp->vinaddr,coin->jvinaddr);
    coin->jscriptPubKey[0] = coin->jvinaddr[0] = 0;
    strcpy(sp->vinpubP,coin->jpubP), coin->jpubP[0] = 0;
    sp->vinkey = coin->jvinkey, coin->jvinkey = 0;
    if ( sp->change != 0 )
    {
        sp->changestr = jumblr_encodevout(coin->jchangehex,coin->jchangeaddr,coin,sp->change,&addrs[i+1],num-i-1);
        strcpy(sp->changeaddr,coin->jchangeaddr);
        coin->jchangeaddr[0] = coin->jchangehex[0] = 0;
    }
    sp->voutstr = jumblr_encodevout(coin->jdesthex,coin->jdestaddr,coin,sp->amount,&addrs[i+1],num-i-1);
    strcpy(sp->destaddr,coin->jdestaddr);
    coin->jdestaddr[0] = coin->jdesthex[0] = 0;
    if ( sp->amount == 0 || coin == 0 || sp->vinstr == 0 || sp->voutstr == 0 )
    {
        printf("num.%d amount %.8f (%s) vinstr.(%s) voutstr.(%s)\n",num,dstr(sp->amount),coin->name,sp->vinstr,sp->voutstr);
        return(-1);
    }
    return(0);
}

int32_t jumblr_vintxid(uint64_t *unspentp,char *vinaddr,char *scriptPubKey,char *vintxid,struct coin777 *coin,uint64_t amount,int32_t srcacct)
{
    uint64_t total; int32_t n,vout = -1; struct rawvin vin; struct subatomic_unspent_tx *utx,*up; char sourceacct[64];
    memset(&vin,0,sizeof(vin));
    *unspentp = 0;
    sourceacct[0] = 0;
    if ( srcacct > 0 )
        sprintf(sourceacct,"jumblr.%d",srcacct);
    else if ( srcacct == 0 )
        strcpy(sourceacct,"jumblrchange");
    else sourceacct[0] = 0;
    printf("call gather_unspents.%s %.8f\n",sourceacct,dstr(amount));
    if ( (utx= gather_unspents(&total,&n,coin,sourceacct)) != 0  )
    {
        if ( (up= subatomic_bestfit(coin,utx,n,amount,1)) != 0 )
        {
            *unspentp = up->amount;
            strcpy(vintxid,up->txid.buf);
            strcpy(vinaddr,up->address.buf);
            strcpy(scriptPubKey,up->scriptPubKey.buf);
            vout = up->vout;
            printf("VINTXID.%s srcaccount.(%s) %s/v%d %s %s %.8f\n",coin->name,sourceacct,vintxid,vout,vinaddr,scriptPubKey,dstr(*unspentp));
        }
        free(utx);
    }
    return(vout);
}

char *jumblr_onetime(char *pubkey,struct coin777 *coin,char *type,uint64_t *addrs,int32_t num)
{
    char *newaddress,*retstr = 0;
    if ( (newaddress= jumblr_onetimeaddress(pubkey,coin,type)) != 0 )
    {
        strcpy(pubkey,newaddress);
        retstr = jumblr_layer(pubkey,addrs,num);
        free(newaddress);
    }
    return(retstr);
}

int32_t jumblr_destaddress(char *hexaddress,char *destaddress,struct coin777 *coin,int32_t destacct)
{
    char pubkey[128],destaccount[64],*str;
    destaccount[0] = 0;
    if ( destacct > 0 )
        sprintf(destaccount,"jumblr.%d",destacct);
    else strcpy(destaccount,"jumblrchange");
    if ( (str= jumblr_onetimeaddress(pubkey,coin,destaccount)) != 0 )
    {
        strcpy(destaddress,str), free(str);
        btc_convaddr(hexaddress,destaddress);
        printf("DESTADDRESS.%s (%s) <- (%s) (%s)\n",coin->name,destaccount,destaddress,hexaddress);
        return(0);
    }
    return(-1);
}

struct jumblr_info *jumblr_find(uint64_t shuffleid)
{
    int32_t i;
    for (i=0; i<sizeof(SHUFFLES)/sizeof(*SHUFFLES); i++)
        if ( SHUFFLES[i] != 0 && shuffleid == SHUFFLES[i]->shuffleid )
            return(SHUFFLES[i]);
    return(0);
}

void jumblr_free(struct jumblr_info *sp)
{
    int32_t i; void *ptr;
    for (i=0; i<sizeof(SHUFFLES)/sizeof(*SHUFFLES); i++)
        if ( SHUFFLES[i] == sp )
        {
            SHUFFLES[i] = 0;
            break;
        }
    printf("JUMBLR PURGE %llu\n",(long long)sp->quoteid);
    if ( (ptr= sp->vinstr) != 0 )
        sp->vinstr = 0, free(ptr);
    if ( (ptr= sp->voutstr) != 0 )
        sp->voutstr = 0, free(ptr);
    if ( (ptr= sp->changestr) != 0 )
        sp->changestr = 0, free(ptr);
    if ( (ptr= sp->cointxid) != 0 )
        sp->cointxid = 0, free(ptr);
    if ( (ptr= sp->T) != 0 )
        sp->T = 0, free(ptr);
    free(sp);
}

uint64_t jumblr_amount(struct coin777 *coin,uint64_t amount) { return(amount < coin->mgw.txfee*10000 ? amount : coin->mgw.txfee * 10000); }

int32_t jumblr_idle(struct plugin_info *plugin)
{
    static uint32_t lastpurge;
    struct coin777 *coin; int32_t i; uint32_t now = (uint32_t)time(NULL);
    if ( SUPERNET.iamrelay == 0 && COINS.num > 0 )
    {
        for (i=0; i<COINS.num; i++)
        {
            if ( (coin= COINS.LIST[i]) != 0 )
            {
                if ( coin->jpubP[0] == 0 && coin->jvinkey == 0 && coin->junspent == 0 && coin->jvintxid[0] == 0 && coin->jvin < 0 && coin->jscriptPubKey[0] == 0 && coin->jvinaddr[0] >= 0 )
                {
                    strcpy(coin->jvintxid,"error getting unspent txid");
                    if ( (coin->jvin= jumblr_vintxid(&coin->junspent,coin->jvinaddr,coin->jscriptPubKey,coin->jvintxid,coin,jumblr_amount(coin,SATOSHIDEN*10000),-1)) >= 0 )
                    {
                        if ( coin->jvinaddr[0] != 0 )
                            coin->jvinkey = jumblr_bpkey(coin->jpubP,coin,coin->jvinaddr);
                        if ( coin->jchangehex[0] == 0 && coin->jchangeaddr[0] == 0 )
                            jumblr_destaddress(coin->jchangehex,coin->jchangeaddr,coin,0);
                        if ( coin->jdesthex[0] == 0 || coin->jdestaddr[0] == 0 )
                            jumblr_destaddress(coin->jdesthex,coin->jdestaddr,coin,1);
                    }
                }
            }
        }
        if ( now > lastpurge+60 )
        {
            lastpurge = now;
            for (i=0; i<sizeof(SHUFFLES)/sizeof(*SHUFFLES); i++)
                if ( SHUFFLES[i] != 0 && SHUFFLES[i]->done != 0 && now > SHUFFLES[i]->timestamp+3600 )
                    jumblr_free(SHUFFLES[i]);
        }
    }
    return(0);
}

char *jumblr_cointx(struct coin777 *coin,struct jumblr_info *sp,char *vins[],int32_t numvins,char *vouts[],int32_t numvouts)
{
    struct cointx_info *T; int32_t i; uint64_t totaloutputs,totalinputs,value,fee,sharedfee; struct rawvout *v; struct cointx_input *vin;
    T = calloc(1,sizeof(*T));
    T->version = 1;
    T->timestamp = (uint32_t)time(NULL);
    totaloutputs = totalinputs = 0;
    for (i=0; i<numvins; i++)
    {
        vin = &T->inputs[T->numinputs];
        if ( jumblr_decodevin(vin,vins[i]) < 0 )
        {
            printf("error decoding vin.(%s)\n",vins[i]);
            break;
        }
        printf("jumblr_getcoinaddr.(%s v%d [%s %s]) ",vin->tx.txidstr,vin->tx.vout,vin->sigs,vin->coinaddr);
        if ( (value= ram_verify_txstillthere(coin->name,coin->serverport,coin->userpass,vin->tx.txidstr,vin->tx.vout)) > 0 )
            totalinputs += value;
        else
        {
            printf("error getting unspent.(%s v%d)\n",vin->tx.txidstr,vin->tx.vout);
            break;
        }
        T->numinputs++;
    }
    //printf("numinputs.%d numvins.%d total %.8f\n",T->numinputs,numvins,dstr(totalinputs));
    if ( T->numinputs == numvins )
    {
        for (T->numoutputs=i=0; i<numvouts; i++)
        {
            v = &T->outputs[T->numoutputs];
            if ( jumblr_decodevout(v,vouts[i]) < 0 )
            {
                printf("error decoding vin.(%s)\n",vins[i]);
                break;
            }
            totaloutputs += v->value;
            T->numoutputs++;
        }
        //printf("numoutputs.%d numvouts.%d total %.8f\n",T->numoutputs,numvouts,dstr(totaloutputs));
        if ( T->numoutputs == numvouts )
        {
            fee = (numvouts * coin->mgw.txfee);
            if ( totalinputs < totaloutputs+fee )
            {
                printf("not enough inputs %.8f for outputs %.8f + fee %.8f diff %.8f\n",dstr(totalinputs),dstr(totaloutputs),dstr(fee),dstr(totaloutputs+fee-totalinputs));
                free(T);
                return(0);
            }
            if ( (sharedfee= (totalinputs - totaloutputs) - fee) > numvouts )
            {
                printf("sharedfee %.8f\n",dstr(sharedfee));
                if ( coin->donationaddress[0] != 0 && sharedfee >= coin->mgw.txfee )
                {
                    T->outputs[T->numoutputs].value = sharedfee;
                    strcpy(T->outputs[T->numoutputs].coinaddr,coin->donationaddress);
                    strcpy(T->outputs[T->numoutputs].script,coin->donationscript);
                    T->numoutputs++;
                }
                else
                {
                    printf("share excess fee %.8f among %d outputs\n",dstr(sharedfee),numvouts);
                    sharedfee /= numvouts;
                    for (i=0; i<numvouts; i++)
                        T->outputs[i].value += sharedfee;
                }
            }
            disp_cointx(T);
            _emit_cointx(sp->rawtx,sizeof(sp->rawtx),T,coin->mgw.oldtx_format);
            free(T);
            return(sp->rawtx);
        }
    }
    return(0);
}

char *jumblr_send(struct coin777 *coin,struct jumblr_info *sp)
{
    char *signedtx,*txid,*jsonstr,*str; cJSON *json; uint32_t locktime; int64_t value; struct destbuf scriptPubKey; int32_t allocsize = 65536 + 4;
    if ( sp->T != 0 )
    {
        if ( bitweight(sp->sigmask) >= sp->numaddrs )
        {
            signedtx = calloc(1,allocsize);
            strcpy(signedtx,"[\"");
            _emit_cointx(signedtx+2,allocsize-3,sp->T,coin->mgw.oldtx_format);
            if ( (txid= subatomic_decodetxid(&value,&scriptPubKey,&locktime,coin,signedtx+2,0)) != 0 )
            {
                if ( (jsonstr= _get_transaction(coin->name,coin->serverport,coin->userpass,txid)) != 0 )
                {
                    if ( (json= cJSON_Parse(signedtx)) != 0 )
                    {
                        if ( (str= jstr(json,"hex")) != 0 )
                        {
                            if ( strcmp(str,signedtx+2) == 0 )
                            {
                                printf("SIGNEDTX already there!\n");
                                sp->done = 1;
                            }
                        }
                        free_json(json);
                    }
                    free(jsonstr);
                }
                free(txid);
            }
            if ( sp->done == 0 )
            {
                strcat(signedtx,"\"]");
                if ( (sp->cointxid= bitcoind_passthru(coin->name,coin->serverport,coin->userpass,"sendrawtransaction",signedtx)) != 0 )
                    printf(">>>>>>>>>>>>> %s.%llu BROADCAST.(%s) (%s)\n",coin->name,(long long)sp->quoteid,signedtx,sp->cointxid);
                else printf("error sending transaction.(%s)\n",signedtx);
                sp->done = 1;
            }
            delete_iQ(sp->quoteid);
            free(signedtx);
        } else printf("sigmask.%d wt.%d vs numaddrs.%d\n",(int32_t)sp->sigmask,(int32_t)bitweight(sp->sigmask),sp->numaddrs);
        return(sp->cointxid);
    }
    return(0);
}

char *jumblr_validate(struct coin777 *coin,char *rawtx,struct jumblr_info *sp)
{
    struct cointx_info *cointx; uint32_t nonce; int32_t i,j,vin=-1,vout=-1,changeout=-1;
    char buf[8192],coinaddr[64],*str; uint8_t rmd160[20];
    if ( sp == 0 )
    {
        printf("cant find shuffleid.%llu\n",(long long)sp->shuffleid);
        return(clonestr("{\"error\":\"cant find shuffleid\"}"));
    }
    //printf("validate.(%s) vin.%s vout.%s\n",rawtx,sp->inputtxid,sp->destaddr);
    if ( (cointx= _decode_rawtransaction(rawtx,coin->mgw.oldtx_format)) != 0 )
    {
        //printf("validate.(%s) vin.%s vout.%s numoutputs.%d numinputs.%d\n",rawtx,sp->inputtxid,sp->destaddr,cointx->numoutputs,cointx->numinputs);
        sp->T = cointx;
        for (i=0; i<cointx->numoutputs; i++)
        {
            for (j=0; j<cointx->numoutputs; j++)
                if ( i != j && strcmp(cointx->outputs[i].script,cointx->outputs[j].script) == 0 )
                {
                    printf("jumblr_validate: duplicate output error i.%d == j.%d (%s)\n",i,j,cointx->outputs[i].script);
                    free(sp->T), sp->T = 0;
                    sp->done = 1;
                    return(0);
                }
            decode_hex(rmd160,20,&cointx->outputs[i].script[6]);
            btc_convrmd160(coinaddr,coin->addrtype,rmd160);
            //printf("%d of %d: %s -> %s\n",i,cointx->numoutputs,cointx->outputs[i].script,coinaddr);
            if ( vout < 0 && strcmp(coinaddr,sp->destaddr) == 0 )
            {
                printf("matched dest.(%s) %.8f\n",sp->destaddr,dstr(sp->amount));
                if ( cointx->outputs[i].value >= sp->amount )
                    vout = i;
                else printf("warning: amount mismatch %.8f vs %.8f\n",dstr(cointx->outputs[i].value),dstr(sp->amount));
            }
            if ( sp->change != 0 && changeout < 0 && strcmp(coinaddr,sp->changeaddr) == 0 )
            {
                printf("matched change.(%s) %.8f\n",sp->changeaddr,dstr(sp->change));
                if ( cointx->outputs[i].value >= sp->change )
                    changeout = i;
                else printf("warning: change mismatch %.8f vs %.8f\n",dstr(cointx->outputs[i].value),dstr(sp->change));
            }
            if ( (sp->change == 0 || changeout >= 0) && vout >= 0 )
                break;
        }
        printf("sp->change %llu changeout.%d vout.%d\n",(long long)sp->change,changeout,vout);
        if ( (sp->change == 0 || changeout >= 0) && vout >= 0 )
        {
            for (i=0; i<cointx->numinputs; i++)
            {
                for (j=0; j<cointx->numinputs; j++)
                    if ( i != j && strcmp(cointx->inputs[i].tx.txidstr,cointx->inputs[j].tx.txidstr) == 0 && cointx->inputs[i].tx.vout == cointx->inputs[j].tx.vout )
                    {
                        printf("jumblr_validate: duplicate input error i.%d == j.%d (%s/v%d)\n",i,j,cointx->inputs[i].tx.txidstr,cointx->inputs[i].tx.vout);
                        free(sp->T), sp->T = 0;
                        sp->done = 1;
                        return(0);
                    }
                printf("%s ",cointx->inputs[i].tx.txidstr);
                //cointx->inputs[i].value = jumblr_getcoinaddr(cointx->inputs[i].coinaddr,&scriptPubKey,coin,cointx->inputs[i].tx.txidstr,cointx->inputs[i].tx.vout);
                //strcpy(cointx->inputs[i].sigs,scriptPubKey.buf);
                if ( vin < 0 && strcmp(cointx->inputs[i].tx.txidstr,sp->inputtxid) == 0 )
                {
                    printf("i.%d matched input.(%s) vin.%d\n",i,sp->inputtxid,sp->vin);
                    if ( cointx->inputs[i].tx.vout == sp->vin )
                    {
                        vin = i;
                        break;
                    } else printf("warning: vout mismatch %d vs %d\n",cointx->inputs[i].tx.vout,sp->vin);
                }
            }
            if ( vin >= 0 )
            {
                char *jumblr_signvin(char *sigstr,struct coin777 *coin,char *buf,int32_t bufsize,void *bpkey,char *pubP,struct cointx_info *refT,int32_t redeemi,char *rawtx);
                if ( jumblr_signvin(sp->sigs[vin],coin,sp->signedtx,sizeof(sp->signedtx),sp->vinkey,sp->vinpubP,cointx,vin,rawtx) != 0 )
                {
                    sprintf(buf,"{\"shuffleid\":\"%llu\",\"timestamp\":\"%u\",\"plugin\":\"relay\",\"destplugin\":\"jumblr\",\"method\":\"busdata\",\"submethod\":\"signed\",\"sig\":\"%s\",\"vin\":%d}",(long long)sp->shuffleid,sp->timestamp,sp->sigs[vin],vin);
                    if ( (str= busdata_sync(&nonce,buf,"allnodes",0)) != 0 )
                        free(str);
                    printf("signed.(%s)\n",buf);
                    sp->sigmask |= (1LL << vin);
                    for (i=0; i<sp->numaddrs; i++)
                    {
                        if ( sp->sigs[i][0] != 0 )
                            strcpy(cointx->inputs[i].sigs,sp->sigs[i]);
                        //sp->sigs[i][0] = 0;
                    }
                    jumblr_send(coin,sp);
                    return(clonestr(buf));
                }
            }
        }
    }
    printf("invalidtx\n");
    return(clonestr("{\"error\":\"shuffle tx invalid\"}"));
}

struct jumblr_info *jumblr_create(int32_t *createdflagp,char *base,uint32_t timestamp,uint64_t *addrs,int32_t numaddrs)
{
    struct jumblr_info *sp = 0; bits256 hash; int32_t i,firstslot = -1;
    if ( createdflagp != 0 )
        *createdflagp = 0;
    for (i=0; i<numaddrs; i++)
        if ( addrs[i] == SUPERNET.my64bits )
            break;
    if ( i == numaddrs )
    {
        printf("this node not in addrs\n");
        return(0);
    }
    if ( numaddrs > 0 && (sp= calloc(1,sizeof(*sp))) != 0 )
    {
        sp->myind = i;
        strcpy(sp->base,base);
        if ( (sp->timestamp= timestamp) == 0 )
            sp->timestamp = (uint32_t)time(NULL);
        sp->numaddrs = numaddrs;
        sp->basebits = stringbits(base);
        memcpy(sp->addrs,addrs,numaddrs * sizeof(sp->addrs[0]));
        calc_sha256(0,hash.bytes,(uint8_t *)sp,numaddrs * sizeof(sp->addrs[0]) + 2*sizeof(uint64_t));
        sp->shuffleid = hash.txid;
        for (i=0; i<sizeof(SHUFFLES)/sizeof(*SHUFFLES); i++)
        {
            if ( SHUFFLES[i] != 0 )
            {
                if ( sp->shuffleid == SHUFFLES[i]->shuffleid )
                {
                    printf("shuffleid %llu already exists!\n",(long long)sp->shuffleid);
                    free(sp);
                    return(SHUFFLES[i]);
                }
            }
            else if ( firstslot < 0 )
                firstslot = i;
        }
        SHUFFLES[firstslot] = sp;
        if ( createdflagp != 0 )
            *createdflagp = 1;
    }
    return(sp);
}

int32_t uniq_specialaddrs(int32_t *myindp,uint64_t addrs[],int32_t max,char *base,char *special,int32_t addrtype)
{
    char destNXT[64],rsaddr[64]; cJSON *array; int32_t k,i,j,n,r,num=0,haspubkey,myind = -1; uint64_t tmp,x,quoteid = 0;
    if ( (array= InstantDEX_specialorders(&quoteid,SUPERNET.my64bits,base,special,0,addrtype)) != 0 )
    {
        if ( (n= cJSON_GetArraySize(array)) > 0 )
        {
            r = (rand() % n);
            for (j=0; j<n; j++)
            {
                i = (j + r) % n;
                x = j64bits(jitem(array,i),0);
                if ( num > 0 )
                {
                    for (k=0; k<num; k++)
                        if ( x == addrs[k] )
                            break;
                } else k = 0;
                if ( k == num )
                {
                    if ( x == SUPERNET.my64bits )
                        myind = num;
                    expand_nxt64bits(destNXT,x);
                    issue_getpubkey(&haspubkey,destNXT);
                    if ( haspubkey == 0 )
                    {
                        RS_encode(rsaddr,addrs[num]);
                        tmp = RS_decode(rsaddr);
                        printf("skipping %s without pubkey RS.%s %llu\n",destNXT,rsaddr,(long long)tmp);
                    } else addrs[num++] = x;
                    if ( num == max )
                        break;
                }
                printf("n.%d r.%d i.%d j.%d k.%d num.%d %llu\n",n,r,i,j,k,num,(long long)addrs[num-1]);
            }
        }
        free_json(array);
    }
    *myindp = 0;
    return(num);
}

char *jumblr_start(char *base,int32_t srcacct)
{
    char destNXT[64],changestr[2048],*addrstr; cJSON *array; struct InstantDEX_quote *iQ = 0;
    int32_t createdflag,i,num=0,myind = -1; uint32_t now,timestamp = 0;
    uint64_t addrs[64],quoteid = 0; struct jumblr_info *sp; struct coin777 *coin;
    if ( base == 0 || base[0] == 0 )
        return(clonestr("{\"error\":\"no base defined\"}"));
    srcacct = -1;
    coin = coin777_find(base,1);
    now = (uint32_t)time(NULL);
    if ( timestamp != 0 && now > timestamp+777 )
        return(clonestr("{\"error\":\"shuffle expired\"}"));
    if ( (num= uniq_specialaddrs(&myind,addrs,sizeof(addrs)/sizeof(*addrs),base,"jumblr",coin->addrtype)) < 3 )
    {
        printf("need at least 3 to shuffle\n");
        return(clonestr("{\"error\":\"not enough shufflers\"}"));
    }
    printf("jumblr_start(%s) myind.%d num.%d\n",base,myind,num);
    if ( (i= myind) > 0 )
    {
        addrs[i] = addrs[0];
        addrs[0] = SUPERNET.my64bits;
        i = 0;
    }
    if ( (sp= jumblr_create(&createdflag,base,timestamp,addrs,num)) == 0 )
    {
        printf("cant create shuffle.(%s) numaddrs.%d\n",base,num);
        return(clonestr("{\"error\":\"cant create shuffleid\"}"));
    }
    sp->srcacct = srcacct;
    if ( createdflag != 0 && sp->myind == 0 && addrs[sp->myind] == SUPERNET.my64bits )
    {
        printf("inside\n");
        if ( quoteid == 0 )
        {
            if ( (array= InstantDEX_specialorders(&quoteid,SUPERNET.my64bits,base,"jumblr",0,coin->addrtype)) != 0 )
                free_json(array);
        }
        printf("quoteid.%llu\n",(long long)quoteid);
        if ( (iQ= find_iQ(quoteid)) != 0 )
        {
            iQ->s.pending = 1;
            if ( jumblr_next(sp,coin,addrs,num,i,jumblr_amount(coin,iQ->s.baseamount),srcacct) < 0 )
                return(clonestr("{\"error\":\"this node not shuffling due to calc error\"}"));
            array = addrs_jsonarray(addrs,num);
            addrstr = jprint(array,1);
            changestr[0] = 0;
            if ( sp->changestr != 0 )
                sprintf(changestr,", \"%s\"",sp->changestr);
            sprintf(sp->rawtx,"{\"shuffleid\":\"%llu\",\"timestamp\":%u,\"base\":\"%s\",\"vins\":[\"%s\"],\"vouts\":[\"%s\"%s],\"addrs\":%s}",(long long)sp->shuffleid,sp->timestamp,sp->base,sp->vinstr,sp->voutstr,changestr,addrstr);
            free(addrstr);
            expand_nxt64bits(destNXT,addrs[i + 1]);
            printf("destNXT.(%s) addrs[%d] %llu\n",destNXT,i+1,(long long)addrs[i+1]);
            telepathic_PM(destNXT,sp->rawtx);
            sp->rawtx[0] = 0;
            sp->quoteid = iQ->s.quoteid;
            return(clonestr("{\"success\":\"shuffle created\"}"));
        }
    }
    return(clonestr("{\"success\":\"shuffle already there\"}"));
}

int32_t jumblr_strs(char *ptrs[],uint8_t num)
{
    int32_t i; uint8_t r; char *tmp;
    for (i=0; i<num; i++)
    {
        randombytes(&r,sizeof(r));
        r %= num;
        tmp = ptrs[i];
        ptrs[i] = ptrs[r];
        ptrs[r] = tmp;
        //printf("%d<->%d ",i,r);
    }
    //printf("shuffle\n");
    return(i);
}

cJSON *jumblr_strarray(char *ptrs[],int32_t num)
{
    cJSON *array = cJSON_CreateArray(); int32_t i;
    for (i=0; i<num; i++)
        jaddistr(array,ptrs[i]);
    return(array);
}

int32_t jumblr_incoming(char *jsonstr)
{
    struct coin777 *coin = 0; cJSON *newjson,*json,*vins,*vouts,*array; int32_t i,j,num,createdflag,numvins,numvouts,myind = -1;
    char *newvins[1024],*newvouts[1024],destNXT[64],buf[8192],*base,*str,*txbytes,*msg; uint32_t nonce;
    uint64_t addrs[64],shuffleid,quoteid; struct jumblr_info *sp; struct InstantDEX_quote *iQ;
    if ( (json= cJSON_Parse(jsonstr)) != 0 && (base= jstr(json,"base")) != 0 && (shuffleid= j64bits(json,"shuffleid")) != 0 )
    {
        coin = coin777_find(base,1);
        if ( (sp= jumblr_find(shuffleid)) == 0 )
        {
            if ( (array= jarray(&num,json,"addrs")) != 0 )
            {
                for (i=0; i<num; i++)
                    if ( (addrs[i]= j64bits(jitem(array,i),0)) == SUPERNET.my64bits )
                        myind = i;
            }
            sp = jumblr_create(&createdflag,base,juint(json,"timestamp"),addrs,num);
            if ( sp == 0 || (sp != 0 && sp->shuffleid != shuffleid) )
            {
                printf("shuffleid mismatch %llu vs %llu\n",(long long)sp->shuffleid,(long long)shuffleid);
                free_json(json);
                return(-1);
            }
        }
        if ( sp != 0 && strcmp(sp->base,base) == 0 && coin != 0 && (vins= jarray(&numvins,json,"vins")) != 0 && (vouts= jarray(&numvouts,json,"vouts")) != 0 )
        {
            if ( myind >= 0 && numvins < sizeof(newvins)/sizeof(*newvins)-2 && numvouts < sizeof(newvouts)/sizeof(*newvouts)-2 )
            {
                //printf("incoming numvins.%d numvouts.%d\n",numvins,numvouts);
                if ( (array= InstantDEX_specialorders(&quoteid,SUPERNET.my64bits,base,"jumblr",0,coin->addrtype)) != 0 )
                    free_json(array);
                if ( (iQ= find_iQ(quoteid)) != 0 )
                {
                    iQ->s.pending = 1;
                    sp->quoteid = quoteid;
                    if ( jumblr_next(sp,coin,addrs,num,myind,jumblr_amount(coin,iQ->s.baseamount),sp->srcacct) < 0 )
                        return(-1);
                    jumblr_peel(newvins,vins,numvins), newvins[numvins++] = clonestr(sp->vinstr);
                    jumblr_peel(newvouts,vouts,numvouts), newvouts[numvouts++] = clonestr(sp->voutstr);
                    if ( sp->change != 0 )
                        newvouts[numvouts++] = clonestr(sp->changestr);
                    //printf("after adding incoming numvins.%d numvouts.%d\n",numvins,numvouts);
                    for (j=0; j<3; j++)
                        jumblr_strs(newvins,numvins);
                    for (j=0; j<3; j++)
                        jumblr_strs(newvouts,numvouts);
                    //printf("myind.%d numaddrs.%d numvins.%d numvouts.%d\n",myind,sp->numaddrs,numvins,numvouts);
                    if ( myind == sp->numaddrs-1 )
                    {
                        if ( (txbytes= jumblr_cointx(coin,sp,newvins,numvins,newvouts,numvouts)) != 0 )
                        {
                            sprintf(buf,"{\"shuffleid\":\"%llu\",\"base\":\"%s\",\"timestamp\":\"%u\",\"plugin\":\"relay\",\"destplugin\":\"jumblr\",\"method\":\"busdata\",\"submethod\":\"validate\",\"rawtx\":\"%s\"}",(long long)sp->shuffleid,sp->base,sp->timestamp,txbytes);
                            if ( (str= busdata_sync(&nonce,buf,"allnodes",0)) != 0 )
                                free(str);
                            printf("RAWTX.(%s)\n",txbytes);
                            //msleep(250 + (rand() % 2000));
                            //if ( (str= jumblr_validate(coin,txbytes,sp)) != 0 )
                            //    free(str);
                        } else printf("jumblr_cointx null return\n");
                    }
                    else
                    {
                        newjson = cJSON_CreateObject();
                        vins = jumblr_strarray(newvins,numvins), vouts = jumblr_strarray(newvouts,numvouts);
                        jadd(newjson,"vins",vins), jadd(newjson,"vouts",vouts);
                        jadd64bits(newjson,"shuffleid",sp->shuffleid);
                        jaddnum(newjson,"timestamp",sp->timestamp);
                        jaddstr(newjson,"base",sp->base);
                        jadd(newjson,"addrs",addrs_jsonarray(addrs,num));
                        msg = jprint(newjson,1);
                        expand_nxt64bits(destNXT,sp->addrs[myind+1]);
                        if ( Debuglevel > 2 )
                            printf("telepathic.(%s) -> destNXT.(%s)\n",msg,destNXT);
                        telepathic_PM(destNXT,msg);
                        free(msg);
                    }
                    for (i=0; i<numvins; i++)
                        free(newvins[i]);
                    for (i=0; i<numvouts; i++)
                        free(newvouts[i]);
                }
            }
        } else printf("jumblr_incoming: missing sp.%p or coin.%p\n",sp,coin);
        free_json(json);
    }
    return(0);
}

#define SHUFFLE_METHODS "validate", "signed", "start"
char *PLUGNAME(_methods)[] = { SHUFFLE_METHODS };
char *PLUGNAME(_pubmethods)[] = { SHUFFLE_METHODS };
char *PLUGNAME(_authmethods)[] = { SHUFFLE_METHODS };

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *argjson)
{
    uint64_t disableflags = 0;
    printf("init %s size.%ld\n",plugin->name,sizeof(struct jumblr_info));
    plugin->allowremote = 1;
    return(disableflags); // set bits corresponding to array position in _methods[]
}

int32_t PLUGNAME(_process_json)(char *forwarder,char *sender,int32_t valid,struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag,char *tokenstr)
{
    char *resultstr,*methodstr,*rawtx,*sig,*retstr = 0; int32_t vin; uint64_t shuffleid; struct coin777 *coin; struct jumblr_info *sp;
    retbuf[0] = 0;
    plugin->allowremote = 1;
    if ( initflag > 0 )
    {
        coin777_find("BTC",1);
        coin777_find("BTCD",1);
        //coin777_find("LTC",1);
        plugin->sleepmillis = 10;
        strcpy(retbuf,"{\"result\":\"shuffle init\"}");
    }
    else if ( SUPERNET.iamrelay == 0 )
    {
        fprintf(stderr,"<<<<<<<<<<<< INSIDE PLUGIN! process %s (%s)\n",plugin->name,jsonstr);
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        methodstr = cJSON_str(cJSON_GetObjectItem(json,"method"));
        retbuf[0] = 0;
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
            return((int32_t)strlen(retbuf));
        }
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        if ( methodstr == 0 || methodstr[0] == 0 )
        {
            printf("(%s) has not method\n",jsonstr);
            return(0);
        }
        else if ( strcmp(methodstr,"start") == 0 )
        {
            retstr = jumblr_start(jstr(json,"base"),juint(json,"source"));
        }
        else if ( strcmp(methodstr,"validate") == 0 )
        {
            if ( (rawtx= jstr(json,"rawtx")) != 0 && (shuffleid= j64bits(json,"shuffleid")) != 0 && (sp= jumblr_find(shuffleid)) != 0 )
            {
                if ( (coin= coin777_find(sp->base,0)) != 0 )
                    retstr = jumblr_validate(coin,rawtx,sp);
            }
            if ( retstr == 0 )
                retstr = clonestr("{\"error\":\"shuffle validate invalid args\"}");
        }
        else if ( strcmp(methodstr,"signed") == 0 )
        {
            if ( (sig= jstr(json,"sig")) != 0 && (shuffleid= j64bits(json,"shuffleid")) != 0 && (sp= jumblr_find(shuffleid)) != 0 )
            {
                if ( (coin= coin777_find(sp->base,0)) != 0 && (vin= juint(json,"vin")) >= 0 && vin < 64 && strlen(sig) < sizeof(sp->sigs[0]) )
                {
                    sp->sigmask |= (1LL << vin);
                    strcpy(sp->sigs[vin],sig);
                    printf("SIGMASK.%d sp->T %p\n",(int32_t)sp->sigmask,sp->T);
                    if ( sp->T != 0 )
                    {
                        strcpy(sp->T->inputs[vin].sigs,sig);
                        jumblr_send(coin,sp);
                    }
                    retstr = clonestr("{\"success\":\"shuffle accepted sig\"}");
                } else retstr = clonestr("{\"error\":\"shuffle rejected sig\"}");
            }
            if ( retstr == 0 )
                retstr = clonestr("{\"error\":\"shuffle signed invalid args\"}");
        }
    } else retstr = clonestr("{\"result\":\"relays dont shuffle\"}");
    return(plugin_copyretstr(retbuf,maxlen,retstr));
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    return(retcode);
}
#include "plugin777.c"

