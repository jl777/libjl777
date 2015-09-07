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

//
//  subatomic.h
//  Created by jl777, April 17-22 2014
//

// expire matching AM's, I also need to make sure AM offers expire pretty soon, so some forgotten trade doesnt just goes active
// fix overwriting sequenceid!

#ifndef xcode_subatomic_h
#define xcode_subatomic_h

#ifdef fromlastyear
char *Signedtx; // hack for testing atomic_swap

#define HALFDUPLEX
#define BITCOIN_WALLET_UNLOCKSECONDS 300
#define SUBATOMIC_PORTNUM 6777
#define SUBATOMIC_VARIANT (SUBATOMIC_PORTNUM - SERVER_PORT)
#define SUBATOMIC_SIG 0x84319574
#define SUBATOMIC_LOCKTIME (3600 * 2)
#define SUBATOMIC_DONATIONRATE .001
#define SUBATOMIC_DEFAULTINCR 100
#define SUBATOMIC_TOOLONG (300 * 1000000)
#define MAX_SUBATOMIC_OUTPUTS 4
#define MAX_SUBATOMIC_INPUTS 16
#define SUBATOMIC_STARTING_SEQUENCEID 1000
#define SUBATOMIC_CANTSEND_TOLERANCE 3
#define MAX_NXT_TXBYTES 2048

#define SUBATOMIC_TYPE 0
#define SUBATOMIC_FORNXT_TYPE 3
#define NXTFOR_SUBATOMIC_TYPE 2
#define ATOMICSWAP_TYPE 1

#define SUBATOMIC_TRADEFUNC 't'
#define SUBATOMIC_ATOMICSWAP 's'

struct NXT_tx
{
    unsigned char refhash[32];
    uint64_t senderbits,recipientbits,assetidbits;
    int64_t feeNQT;
    union { int64_t amountNQT; int64_t quantityQNT; };
    int32_t deadline,type,subtype,verify;
    char comment[128];
};

struct atomic_swap
{
    char *parsed[2];
    cJSON *jsons[2];
    char NXTaddr[64],otherNXTaddr[64],otheripaddr[32];
    char txbytes[2][MAX_NXT_TXBYTES],signedtxbytes[2][MAX_NXT_TXBYTES];
    char sighash[2][68],fullhash[2][68];
    int32_t numfragis,atomictx_waiting;
    struct NXT_tx *mytx;
};

struct subatomic_halftx
{
    int32_t coinid,destcoinid,minconfirms;
    int64_t destamount,avail,amount,donation,myamount,otheramount;  // amount = (myamount + otheramount + donation + txfee)
    struct subatomic_rawtransaction funding,refund,micropay;
    char funding_scriptPubKey[512],countersignedrefund[1024],completedmicropay[1024];
    char *fundingtxid,*refundtxid,*micropaytxid;
    char NXTaddr[64],coinaddr[64],pubkey[128],ipaddr[32];
    char otherNXTaddr[64],destcoinaddr[64],destpubkey[128],otheripaddr[32];
    struct multisig_addr *xferaddr;
};

struct subatomic_tx_args
{
    char NXTaddr[64],otherNXTaddr[64],coinaddr[2][64],destcoinaddr[2][64],otheripaddr[32],mypubkeys[2][128];
    int64_t amount,destamount;
    double myshare;
    int32_t coinid,destcoinid,numincr,incr,otherincr;
};

struct subatomic_tx
{
    struct subatomic_tx_args ARGS,otherARGS;
    struct subatomic_halftx myhalf,otherhalf;
    struct atomic_swap swap;
    char *claimtxid;
    int64_t lastcontact,myexpectedamount,myreceived,otherexpectedamount,sent_to_other;//,recvflags;
    int32_t status,initflag,connsock,refundlockblock,cantsend,type,longerflag,tag,verified;
    int32_t txs_created,other_refundtx_done,myrefundtx_done,other_fundingtx_confirms;
    int32_t myrefund_fragi,microtx_fragi,funding_fragi,other_refund_fragi;
    int32_t other_refundtx_waiting,myrefundtx_waiting,other_fundingtx_waiting,other_micropaytx_waiting;
    unsigned char recvbufs[4][sizeof(struct subatomic_rawtransaction)];
};

struct subatomic_info
{
    char ipaddr[32],NXTADDR[32],NXTACCTSECRET[512];
    struct subatomic_tx **subatomics;
    CURL *curl_handle,*curl_handle2;
    int32_t numsubatomics,enable_bitcoin_broadcast;
} *Global_subatomic;

struct subatomic_packet
{
    //struct server_request_header H;
    struct subatomic_tx_args ARGS;
    char pubkeys[2][128],retpubkeys[2][128];
    struct subatomic_rawtransaction rawtx;
#ifdef HALFDUPLEX
    // struct subatomic_rawtransaction needsig,havesig,micropay;   // jl777: hack for my broken Mac
#endif
};
int32_t subatomic_gen_pubkeys(struct subatomic_tx *atx,struct subatomic_halftx *htx);

#ifdef test
void calc_OP_HASH160(unsigned char hash160[20],char *msg)
{
    unsigned char sha256[32];
    hash_state md;
    
    sha256_init(&md);
    sha256_process(&md,(unsigned char *)msg,strlen(msg));
    sha256_done(&md,sha256);
    
    rmd160_init(&md);
    rmd160_process(&md,(unsigned char *)sha256,256/8);
    rmd160_done(&md,hash160);
    {
        int i;
        for (i=0; i<20; i++)
            printf("%02x",hash160[i]);
        printf("<- (%s)\n",msg);
    }
}

// bitcoind functions


void subatomic_uint32_splicer(char *txbytes,int32_t offset,uint32_t spliceval)
{
    int32_t i;
    uint32_t x;
    if ( offset < 0 )
    {
        static int foo;
        if ( foo++ < 3 )
            printf("subatomic_uint32_splicer illegal offset.%d\n",offset);
        return;
    }
    for (i=0; i<4; i++)
    {
        x = spliceval & 0xff; spliceval >>= 8;
        txbytes[offset + i*2] = hexbyte((x>>4) & 0xf);
        txbytes[offset + i*2 + 1] = hexbyte(x & 0xf);
    }
}

struct btcinput_data
{
    unsigned char txid[32];
    uint32_t vout;
    int64_t scriptlen;
    uint32_t sequenceid;
    unsigned char script[];
};

struct btcoutput_data
{
    int64_t value,pk_scriptlen;
    unsigned char pk_script[];
};

struct btcinput_data *decode_btcinput(int32_t *offsetp,char *txbytes)
{
    struct btcinput_data *ptr,I;
    int32_t i;
    memset(&I,0,sizeof(I));
    for (i=0; i<32; i++)
        I.txid[31-i] = _decode_hex(txbytes + *offsetp), (*offsetp) += 2;
    I.vout = _decode_hexint(offsetp,txbytes);
    I.scriptlen = _decode_varint(offsetp,txbytes);
    if ( I.scriptlen > 1024 )
    {
        printf("decode_btcinput: very long script starting at offset.%d of (%s)\n",*offsetp,txbytes);
        return(0);
    }
    ptr = calloc(1,sizeof(*ptr) + I.scriptlen);
    *ptr = I;
    for (i=0; i<I.scriptlen; i++)
        ptr->script[i] = _decode_hex(txbytes + *offsetp), (*offsetp) += 2;
    ptr->sequenceid = _decode_hexint(offsetp,txbytes);
    return(ptr);
}

struct btcoutput_data *decode_btcoutput(int32_t *offsetp,char *txbytes)
{
    int32_t i;
    struct btcoutput_data *ptr,btcO;
    memset(&btcO,0,sizeof(btcO));
    btcO.value = _decode_hexlong(offsetp,txbytes);
    btcO.pk_scriptlen = _decode_varint(offsetp,txbytes);
    if ( btcO.pk_scriptlen > 1024 )
    {
        printf("decode_btcoutput: very long script starting at offset.%d of (%s)\n",*offsetp,txbytes);
        return(0);
    }
    ptr = calloc(1,sizeof(*ptr) + btcO.pk_scriptlen);
    *ptr = btcO;
    for (i=0; i<btcO.pk_scriptlen; i++)
        ptr->pk_script[i] = _decode_hex(txbytes + *offsetp), (*offsetp) += 2;
    return(ptr);
}

uint32_t calc_vin0seqstart(char *txbytes)
{
    struct btcinput_data *btcinput,*firstinput = 0;
    struct btcoutput_data *btcoutput;
    char buf[4096];
    int32_t i,vin0seqstart,numoutputs,numinputs,offset = 0;
    //version = _decode_hexint(&offset,txbytes);
    numinputs = _decode_hex(&txbytes[offset]), offset += 2;
    vin0seqstart = 0;
    for (i=0; i<numinputs; i++)
    {
        btcinput = decode_btcinput(&offset,txbytes);
        if ( btcinput != 0 )
        {
            init_hexbytes(buf,btcinput->txid,32);
            // printf("(%s vout%d) ",buf,btcinput->vout);
        }
        if ( i == 0 )
        {
            firstinput = btcinput;
            vin0seqstart = offset - sizeof(int32_t)*2;
        }
        else if ( btcinput != 0 ) free(btcinput);
    }
    //printf("-> ");
    numoutputs = _decode_hex(&txbytes[offset]), offset += 2;
    for (i=0; i<numoutputs; i++)
    {
        btcoutput = decode_btcoutput(&offset,txbytes);
        if ( btcoutput != 0 )
        {
            init_hexbytes(buf,btcoutput->pk_script,btcoutput->pk_scriptlen);
            // printf("(%s %.8f) ",buf,dstr(btcoutput->value));
            free(btcoutput);
        }
    }
    //locktime =
    _decode_hexint(&offset,txbytes);
    // printf("version.%d 1st.seqid %d @ %d numinputs.%d numoutputs.%d locktime.%d\n",version,firstinput!=0?firstinput->sequenceid:0xffffffff,vin0seqstart,numinputs,numoutputs,locktime);
    //0100000001ac8511d408d35e62ccc7925ed2437022e9b7e9e731197a42a58495e4465439d10000000000ffffffff0200dc5c24020000001976a914bf685a09e61215c7e824d0b73bc6d6d3ba9d9d9688ac00c2eb0b000000001976a91414b24a5b6f8c8df0f7c9b519d362618ca211e60988ac00000000
    if ( firstinput != 0 )
        return(vin0seqstart);
    else return(-1);
}


/* https://en.bitcoin.it/wiki/Contracts#Example_7:_Rapidly-adjusted_.28micro.29payments_to_a_pre-determined_party
 1) Create a public key (K1). Request a public key from the server (K2).
 2) Create and sign but do not broadcast a transaction (T1) that sets up a payment of (for example) 10 BTC to an output requiring both the server's public key and one of your own to be used. A good way to do this is use OP_CHECKMULTISIG. The value to be used is chosen as an efficiency tradeoff.
 3) Create a refund transaction (T2) that is connected to the output of T1 which sends all the money back to yourself. It has a time lock set for some time in the future, for instance a few hours. Don't sign it, and provide the unsigned transaction to the server. By convention, the output script is "2 K1 K2 2 CHECKMULTISIG"
 4) The server signs T2 using its public key K2 and returns the signature to the client. Note that it has not seen T1 at this point, just the hash (which is in the unsigned T2).
 5) The client verifies the servers signature is correct and aborts if not.
 6) The client signs T1 and passes the signature to the server, which now broadcasts the transaction (either party can do this if they both have connectivity). This locks in the money.
 
 7) The client then creates a new transaction, T3, which connects to T1 like the refund transaction does and has two outputs. One goes to K1 and the other goes to K2. It starts out with all value allocated to the first output (K1), ie, it does the same thing as the refund transaction but is not time locked. The client signs T3 and provides the transaction and signature to the server.
 8) The server verifies the output to itself is of the expected size and verifies the client's provided signature is correct.
 9) When the client wishes to pay the server, it adjusts its copy of T3 to allocate more value to the server's output and less to its ow. It then re-signs the new T3 and sends the signature to the server. It does not have to send the whole transaction, just the signature and the amount to increment by is sufficient. The server adjusts its copy of T3 to match the new amounts, verifies the signature and continues.
 
 10) This continues until the session ends, or the 1-day period is getting close to expiry. The AP then signs and broadcasts the last transaction it saw, allocating the final amount to itself. The refund transaction is needed to handle the case where the server disappears or halts at any point, leaving the allocated value in limbo. If this happens then once the time lock has expired the client can broadcast the refund transaction and get back all the money.
 */

// subatomic logic functions

void subatomic_callback(struct NXT_acct *np,int32_t fragi,struct subatomic_tx *atx,struct json_AM *ap,cJSON *json,void *binarydata,int32_t binarylen,uint32_t *targetcrcs)
{
    void *ptr;
    uint32_t TCRC,tcrc;
    int32_t *iptr,coinid,i,j,n,starti,completed,incr,ind,funcid = (ap->funcid & 0xffff);
    cJSON *addrobj,*pubkeyobj,*txobj;
    char coinaddr[64],pubkey[128],txbytes[1024];
    if ( funcid == SUBATOMIC_SEND_ATOMICTX )
    {
        // printf("got atomictx\n");
        txobj = cJSON_GetObjectItem(json,"txbytes");
        if ( txobj != 0 )
        {
            copy_cJSON(txbytes,txobj);
            if ( strlen(txbytes) > 32 )
            {
                //printf("atx.%p TXBYTES.(%s)\n",atx,txbytes);
                safecopy(atx->swap.signedtxbytes[1],txbytes,sizeof(atx->swap.signedtxbytes[1]));
                atx->swap.atomictx_waiting = 1;
            }
        }
    }
    else if ( funcid == SUBATOMIC_SEND_PUBKEY )
    {
        if ( json != 0 )
        {
            coinid = (int32_t)get_cJSON_int(json,"coinid");
            if ( coinid != 0 )
            {
                addrobj = cJSON_GetObjectItem(json,coinid_str(coinid));
                copy_cJSON(coinaddr,addrobj);
                if ( strcmp(coinaddr,atx->otherhalf.coinaddr) == 0 )
                {
                    pubkeyobj = cJSON_GetObjectItem(json,"pubkey");
                    copy_cJSON(pubkey,pubkeyobj);
                    if ( strlen(pubkey) >= 64 )
                        strcpy(atx->otherhalf.pubkey,pubkey);
                }
            }
        }
    }
    else
    {
        starti = (int32_t)get_cJSON_int(json,"starti");
        i = (int32_t)get_cJSON_int(json,"i");
        n = (int32_t)get_cJSON_int(json,"n");
        incr = (int32_t)get_cJSON_int(json,"incr");
        TCRC = (uint32_t)get_cJSON_int(json,"TCRC");
        if ( incr == binarylen && starti >= 0 && starti < SYNC_MAXUNREPORTED && i >= 0 && i < SYNC_MAXUNREPORTED && n >= 0 && n < SYNC_MAXUNREPORTED )
        {
            completed = 0;
            for (j=starti; j<starti+n; j++)
            {
                //printf("(%08x vs %08x) ",np->memcrcs[j],targetcrcs[j]);
                if ( np->memcrcs[j] == targetcrcs[j] )
                    completed++;
            }
            ind = -1;
            ptr = 0; iptr = 0;
            switch ( funcid )
            {
                default: printf("illegal funcid.%d\n",funcid); break;
                case SUBATOMIC_REFUNDTX_NEEDSIG: ptr = &atx->otherhalf.refund; ind = 0; iptr = &atx->other_refundtx_waiting; break;
                case SUBATOMIC_REFUNDTX_SIGNED: ptr = &atx->myhalf.refund; ind = 1; iptr = &atx->myrefundtx_waiting; break;
                case SUBATOMIC_FUNDINGTX: ptr = &atx->otherhalf.funding; ind = 2; iptr = &atx->other_fundingtx_waiting; break;
                case SUBATOMIC_SEND_MICROTX: ptr = &atx->otherhalf.micropay; ind = 3; iptr = &atx->other_micropaytx_waiting; break;
            }
            if ( ind >= 0 )
            {
                memcpy(atx->recvbufs[ind]+i*incr,binarydata,binarylen);
                tcrc = _crc32(0,atx->recvbufs[ind],sizeof(struct subatomic_rawtransaction));
                if ( completed == n && TCRC == tcrc )
                {
                    //printf("completed.%d ptr.%p ind.%d i.%d binarydata.%p binarylen.%d crc.%u vs %u\n",completed,ptr,ind,i,binarydata,binarylen,tcrc,TCRC);
                    memcpy(ptr,atx->recvbufs[ind],sizeof(struct subatomic_rawtransaction));
                }
            }
            if ( iptr != 0 )
                *iptr = completed;
        }
    }
}

struct subatomic_tx *subatomic_search_connections(char *NXTaddr,int32_t otherflag,int32_t type)
{
    struct subatomic_info *gp = Global_subatomic;
    int32_t i;
    struct subatomic_tx *atx;
    for (i=0; i<gp->numsubatomics; i++)
    {
        atx = gp->subatomics[i];
        if ( type != atx->type || atx->initflag != 3 || atx->status == SUBATOMIC_COMPLETED || atx->status == SUBATOMIC_ABORTED )
            continue;
        if ( otherflag == 0 && strcmp(NXTaddr,atx->ARGS.NXTaddr) == 0 )  // assumes one IP per NXT addr
            return(atx);
        else if ( otherflag != 0 && strcmp(NXTaddr,atx->ARGS.otherNXTaddr) == 0 )  // assumes one IP per NXT addr
            return(atx);
    }
    return(0);
}

void sharedmem_callback(member_t *pm,int32_t fragi,void *ptr,uint32_t crc,uint32_t *targetcrcs)
{
    cJSON *json;
    void *binarydata;
    struct subatomic_tx *atx;
    int32_t createdflag,binarylen;
    char otherNXTaddr[64],*jsontxt = 0;
    struct NXT_acct *np;
    struct json_AM *ap = ptr;
    expand_nxt64bits(otherNXTaddr,ap->H.nxt64bits);
    if ( strcmp(pm->NXTaddr,otherNXTaddr) != 0 )
        printf("WARNING: mismatched member NXT.%s vs sender.%s\n",pm->NXTaddr,otherNXTaddr);
    binarylen = (ap->funcid>>16)&0xffff;
    if ( ap->H.size > 16 && ap->H.size+binarylen < 1024 && binarylen > 0 )
        binarydata = (void *)((long)ap + ap->H.size);
    else binarydata = 0;
    json = parse_json_AM(ap);
    if ( json != 0 )
        jsontxt = cJSON_Print(json);
    np = get_NXTacct(&createdflag,Global_mp,otherNXTaddr);
    np->recvid++;
    np->memcrcs[fragi] = crc;
    //printf("[R%d S%d] other.[R%d S%d] %x size.%d %p.[%08x].binarylen.%d %s (funcid.%d arg.%d seqid.%d flag.%d) [%s]\n",np->recvid,np->sentid,ap->gatewayid,ap->timestamp,ap->H.sig,ap->H.size,binarydata,binarydata!=0?*(int *)binarydata:0,binarylen,nxt64str(ap->H.nxt64bits),ap->funcid&0xffff,ap->gatewayid,ap->timestamp,ap->jsonflag,jsontxt!=0?jsontxt:"");
    if ( ap->H.sig == SUBATOMIC_SIG )
    {
        if ( (ap->funcid&0xffff) == SUBATOMIC_SEND_ATOMICTX )
            atx = subatomic_search_connections(otherNXTaddr,1,ATOMICSWAP_TYPE);
        else
            atx = subatomic_search_connections(otherNXTaddr,1,0);
        if ( atx != 0 )
            subatomic_callback(np,fragi,atx,ap,json,binarydata,binarylen,targetcrcs);
    }
    if ( jsontxt != 0 )
        free(jsontxt);
    if ( json != 0 )
        free_json(json);
}

int32_t share_pubkey(struct NXT_acct *np,int32_t fragi,int32_t destcoinid,char *destcoinaddr,char *destpubkey)
{
    char jsonstr[512];
    // also check other pubkey and if matches atx->otherhalf.coinaddr set atx->otherhalf.pubkey
    sprintf(jsonstr,"{\"coinid\":%d,\"%s\":\"%s\",\"pubkey\":\"%s\"}",destcoinid,coinid_str(destcoinid),destcoinaddr,destpubkey);
    send_to_NXTaddr(&np->localcrcs[fragi],np->H.NXTaddr,fragi,SUBATOMIC_SIG,SUBATOMIC_SEND_PUBKEY,jsonstr,0,0);
    return(fragi+1);
}

int32_t share_atomictx(struct NXT_acct *np,char *txbytes,int32_t fragi)
{
    char jsonstr[512];
    sprintf(jsonstr,"{\"txbytes\":\"%s\"}",txbytes);
    send_to_NXTaddr(&np->localcrcs[fragi],np->H.NXTaddr,fragi,SUBATOMIC_SIG,SUBATOMIC_SEND_ATOMICTX,jsonstr,0,0);
    return(fragi+1);
}

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
                refutx = sign_NXT_tx(Global_subatomic->curl_handle,signedtx,utx,sp->fullhash[1],1.);
                /*txjson = gen_NXT_tx_json(utx,sp->fullhash[1],1.);
                 signedtx[0] = 0;
                 if ( txjson != 0 )
                 {
                 if ( extract_cJSON_str(signedtx,sizeof(signedtx),txjson,"transactionBytes") > 0 )
                 {
                 if ( (parsed = issue_parseTransaction(signedtx)) != 0 )
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
                 }*/
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

char *AM_subatomic(int32_t func,int32_t rating,char *destaddr,char *senderNXTaddr,char *jsontxt)
{
    char AM[4096],*retstr;
    struct json_AM *ap = (struct json_AM *)AM;
    stripwhite(jsontxt,strlen(jsontxt));
    printf("sender.%s func.(%c) AM_subatomic(%s) -> NXT.(%s) rating.%d\n",senderNXTaddr,func,jsontxt,destaddr,rating);
    set_json_AM(ap,SUBATOMIC_SIG,func,senderNXTaddr,rating,jsontxt,1);
    retstr = submit_AM(Global_subatomic->curl_handle,destaddr,&ap->H,0);
    printf("AM_subatomic.(%s)\n",retstr);
    return(retstr);
}

cJSON *gen_atomicswap_json(int AMflag,struct atomic_swap *sp,char *senderip)
{
    cJSON *json;
    json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"type",cJSON_CreateString("NXTatomicswap"));
    if ( sp->NXTaddr[0] != 0 )
        cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(sp->NXTaddr));
    if ( sp->parsed[0] != 0 )
    {
        if ( AMflag == 0 )
        {
            if ( sp->signedtxbytes[0] != 0 )
                cJSON_AddItemToObject(json,"signedtxbytes",cJSON_CreateString(sp->signedtxbytes[0]));
            if ( sp->fullhash[0] != 0 )
                cJSON_AddItemToObject(json,"myfullhash",cJSON_CreateString(sp->fullhash[0]));
            if ( sp->jsons != 0 )
                cJSON_AddItemToObject(json,"myTX",sp->jsons[0]);
        }
        if ( sp->txbytes[0] != 0 )
            cJSON_AddItemToObject(json,"mytxbytes",cJSON_CreateString(sp->txbytes[0]));
        if ( sp->sighash[0] != 0 )
            cJSON_AddItemToObject(json,"mysighash",cJSON_CreateString(sp->sighash[0]));
    }
    if ( sp->otherNXTaddr[0] != 0 )
        cJSON_AddItemToObject(json,"destNXT",cJSON_CreateString(sp->otherNXTaddr));
    if ( sp->otheripaddr[0] != 0 )
        cJSON_AddItemToObject(json,"otheripaddr",cJSON_CreateString(sp->otheripaddr));
    if ( sp->parsed[0] != 0 )
    {
        if ( sp->txbytes[1] != 0 )
            cJSON_AddItemToObject(json,"othertxbytes",cJSON_CreateString(sp->txbytes[1]));
        if ( sp->sighash[1] != 0 )
            cJSON_AddItemToObject(json,"othersighash",cJSON_CreateString(sp->sighash[1]));
        if ( AMflag == 0 )
        {
            if ( sp->signedtxbytes[1] != 0 )
                cJSON_AddItemToObject(json,"othersignedtxbytes",cJSON_CreateString(sp->signedtxbytes[1]));
            if ( sp->fullhash[1] != 0 )
                cJSON_AddItemToObject(json,"otherfullhash",cJSON_CreateString(sp->fullhash[1]));
            if ( sp->jsons[1] != 0 )
                cJSON_AddItemToObject(json,"otherTX",sp->jsons[1]);
        }
    }
    
    return(json);
}

cJSON *gen_subatomic_json(int AMflag,int type,struct subatomic_tx *_atx,char *senderip)
{
    struct subatomic_tx_args *atx = &_atx->ARGS;
    struct subatomic_info *gp = Global_subatomic;
    char numstr[512];
    cJSON *json;
    json = cJSON_CreateObject();
    if ( type == ATOMICSWAP_TYPE )
        return(gen_atomicswap_json(AMflag,&_atx->swap,senderip));
    cJSON_AddItemToObject(json,"type",cJSON_CreateString(type==0?"subatomic_crypto":"NXTatomicswap"));
    if ( AMflag == 0 )
    {
        cJSON_AddItemToObject(json,"type",cJSON_CreateString(type==0?"subatomic_crypto":"NXTatomicswap"));
        if ( _atx->myexpectedamount != 0 )
            cJSON_AddItemToObject(json,"completed",cJSON_CreateNumber(dstr((double)_atx->myreceived/_atx->myexpectedamount)));
        cJSON_AddItemToObject(json,"received",cJSON_CreateNumber(dstr(_atx->myreceived)));
        cJSON_AddItemToObject(json,"expected",cJSON_CreateNumber(dstr(_atx->myexpectedamount)));
        cJSON_AddItemToObject(json,"sent",cJSON_CreateNumber(dstr(_atx->sent_to_other)));
        cJSON_AddItemToObject(json,"sending",cJSON_CreateNumber(dstr(_atx->otherexpectedamount)));
        if ( senderip == 0 && gp->ipaddr[0] != 0 )
            cJSON_AddItemToObject(json,"ipaddr",cJSON_CreateString(gp->ipaddr));
        if ( atx->coinaddr[1][0] != 0 )
            cJSON_AddItemToObject(json,"destNXTcoinaddr",cJSON_CreateString(atx->coinaddr[1]));
        if ( atx->destcoinaddr[1][0] != 0 )
            cJSON_AddItemToObject(json,"destNXTdestcoinaddr",cJSON_CreateString(atx->destcoinaddr[1]));
        if ( senderip == 0 && atx->otheripaddr[0] != 0 )
            cJSON_AddItemToObject(json,"otherip",cJSON_CreateString(atx->otheripaddr));
    }
    else cJSON_AddItemToObject(json,"senderip",cJSON_CreateString(Global_mp->ipaddr));
    if ( gp->NXTADDR[0] != 0 )
        cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(gp->NXTADDR));
    cJSON_AddItemToObject(json,"coin",cJSON_CreateString(coinid_str(atx->coinid)));
    sprintf(numstr,"%.8f",dstr(atx->amount)); cJSON_AddItemToObject(json,"amount",cJSON_CreateString(numstr));
    if ( atx->coinaddr[0][0] != 0 )
        cJSON_AddItemToObject(json,"coinaddr",cJSON_CreateString(atx->coinaddr[0]));
    cJSON_AddItemToObject(json,"destcoin",cJSON_CreateString(coinid_str(atx->destcoinid)));
    sprintf(numstr,"%.8f",dstr(atx->destamount)); cJSON_AddItemToObject(json,"destamount",cJSON_CreateString(numstr));
    if ( atx->destcoinaddr[0][0] != 0 )
        cJSON_AddItemToObject(json,"destcoinaddr",cJSON_CreateString(atx->destcoinaddr[0]));
    
    if ( atx->otherNXTaddr[0] != 0 )
        cJSON_AddItemToObject(json,"destNXT",cJSON_CreateString(atx->otherNXTaddr));
    
    return(json);
}

char *subatomic_cancel_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs)
{
    char *retstr = 0;
    retstr = clonestr("{\"result\":\"subatomic cancel trade not supported yet\"}");
    return(retstr);
}

char *subatomic_status_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs)
{
    struct subatomic_info *gp = Global_subatomic;
    int32_t i;
    cJSON *item,*array;
    char *retstr = 0;
    printf("do status\n");
    if ( gp->numsubatomics == 0 )
        retstr = clonestr("{\"result\":\"subatomic no trades pending\"}");
    else
    {
        array = cJSON_CreateArray();
        for (i=0; i<gp->numsubatomics; i++)
        {
            item = gen_subatomic_json(0,gp->subatomics[i]->type,gp->subatomics[i],0);
            if ( item != 0 )
                cJSON_AddItemToArray(array,item);
        }
        retstr = cJSON_Print(array);
        free_json(array);
    }
    return(retstr);
}

char *subatomic_trade_func(char *sender,int32_t valid,cJSON **objs,int32_t numobjs)
{
    char NXTaddr[64],amountstr[128],destamountstr[128],otheripaddr[32],coin[64],destcoin[64],coinaddr[MAX_NXT_TXBYTES],destcoinaddr[MAX_NXT_TXBYTES],otherNXTaddr[64],*retstr = 0;
    cJSON *json;
    int32_t flipped,type;
    struct subatomic_tx T;
    char *jsontxt,*AMtxid,buf[4096];
    if ( (type= set_subatomic_argstrs(&flipped,sender,0,objs,amountstr,destamountstr,NXTaddr,coin,destcoin,coinaddr,destcoinaddr,otherNXTaddr,otheripaddr)) >= 0 )
    {
        if ( set_subatomic_trade(type,&T,NXTaddr,coin,amountstr,coinaddr,otherNXTaddr,destcoin,destamountstr,destcoinaddr,otheripaddr,flipped) == 0 )
        {
            json = gen_subatomic_json(1,type,&T,otheripaddr[0]!=0?otheripaddr:0);
            if ( json != 0 )
            {
                jsontxt = cJSON_Print(json);
                AMtxid = AM_subatomic(SUBATOMIC_TRADEFUNC,0,otherNXTaddr,NXTaddr,jsontxt);
                if ( AMtxid != 0 )
                {
                    sprintf(buf,"{\"result\":\"good\",\"AMtxid\":\"%s\",\"trade\":%s}",AMtxid,jsontxt);
                    retstr = clonestr(buf);
                    free(AMtxid);
                }
                else
                {
                    sprintf(buf,"{\"result\":\"error\",\"AMtxid\":\"0\",\"trade\":%s}",jsontxt);
                    retstr = clonestr(buf);
                }
                free(jsontxt);
                free_json(json);
            }
            else retstr = clonestr("{\"result\":\"error generating json\"}");
        }
        else retstr = clonestr("{\"result\":\"error initializing subatomic trade\"}");
    }
    else retstr = clonestr("{\"result\":\"invalid trade args\"}");
    return(retstr);
}

char *subatomic_json_commands(struct subatomic_info *gp,cJSON *argjson,char *sender,int32_t valid)
{
    static char *subatomic_status[] = { (char *)subatomic_status_func, "subatomic_status", "", "NXT", 0 };
    static char *subatomic_cancel[] = { (char *)subatomic_cancel_func, "subatomic_cancel", "", "NXT", 0 };
    static char *subatomic_trade[] = { (char *)subatomic_trade_func, "subatomic_trade", "", "NXT", "coin", "amount", "coinaddr", "destNXT", "destcoin", "destamount", "destcoinaddr", "senderip", "type", "mytxbytes", "othertxbytes", "mysighash", "othersighash", 0 };
    static char **commands[] = { subatomic_status, subatomic_cancel, subatomic_trade };
    int32_t i,j;
    cJSON *obj,*nxtobj,*objs[16];
    char NXTaddr[64],command[4096],**cmdinfo;
    printf("subatomic_json_commands\n");
    memset(objs,0,sizeof(objs));
    command[0] = 0;
    memset(NXTaddr,0,sizeof(NXTaddr));
    if ( argjson != 0 )
    {
        obj = cJSON_GetObjectItem(argjson,"requestType");
        nxtobj = cJSON_GetObjectItem(argjson,"NXT");
        copy_cJSON(NXTaddr,nxtobj);
        copy_cJSON(command,obj);
        //printf("(%s) command.(%s) NXT.(%s)\n",cJSON_Print(argjson),command,NXTaddr);
    }
    //printf("multigateway_json_commands sender.(%s) valid.%d\n",sender,valid);
    for (i=0; i<(int32_t)(sizeof(commands)/sizeof(*commands)); i++)
    {
        cmdinfo = commands[i];
        //printf("needvalid.(%c) sender.(%s) valid.%d %d of %d: cmd.(%s) vs command.(%s)\n",cmdinfo[2][0],sender,valid,i,(int32_t)(sizeof(commands)/sizeof(*commands)),cmdinfo[1],command);
        if ( strcmp(cmdinfo[1],command) == 0 )
        {
            if ( cmdinfo[2][0] != 0 )
            {
                if ( sender[0] == 0 || valid != 1 || strcmp(NXTaddr,sender) != 0 )
                {
                    printf("verification valid.%d missing for %s sender.(%s) vs NXT.(%s)\n",valid,cmdinfo[1],NXTaddr,sender);
                    return(0);
                }
            }
            for (j=3; cmdinfo[j]!=0&&j<3+(int32_t)(sizeof(objs)/sizeof(*objs)); j++)
                objs[j-3] = cJSON_GetObjectItem(argjson,cmdinfo[j]);
            return((*(json_handler)cmdinfo[0])(sender,valid,objs,j-3));
        }
    }
    return(0);
}

char *subatomic_jsonhandler(cJSON *argjson)
{
    struct subatomic_info *gp = Global_subatomic;
    long len;
    int32_t valid;
    cJSON *json,*obj,*parmsobj,*tokenobj,*secondobj;
    char sender[64],*parmstxt,encoded[NXT_TOKEN_LEN],*retstr = 0;
    sender[0] = 0;
    valid = -1;
    printf("subatomic_jsonhandler argjson.%p\n",argjson);
    if ( argjson == 0 )
    {
        json = cJSON_CreateObject();
        obj = cJSON_CreateString(gp->NXTADDR); cJSON_AddItemToObject(json,"NXTaddr",obj);
        obj = cJSON_CreateString(gp->ipaddr); cJSON_AddItemToObject(json,"ipaddr",obj);
        
        obj = gen_NXTaccts_json(0);
        if ( obj != 0 )
            cJSON_AddItemToObject(json,"NXTaccts",obj);
        retstr = cJSON_Print(json);
        free_json(json);
        return(retstr);
    }
    else if ( (argjson->type&0xff) == cJSON_Array && cJSON_GetArraySize(argjson) == 2 )
    {
        parmsobj = cJSON_GetArrayItem(argjson,0);
        secondobj = cJSON_GetArrayItem(argjson,1);
        tokenobj = cJSON_GetObjectItem(secondobj,"token");
        copy_cJSON(encoded,tokenobj);
        parmstxt = cJSON_Print(parmsobj);
        len = strlen(parmstxt);
        stripwhite(parmstxt,len);
        printf("website.(%s) encoded.(%s) len.%ld\n",parmstxt,encoded,strlen(encoded));
        if ( strlen(encoded) == NXT_TOKEN_LEN )
            issue_decodeToken(Global_subatomic->curl_handle2,sender,&valid,parmstxt,encoded);
        free(parmstxt);
        argjson = parmsobj;
    }
    if ( sender[0] == 0 )
        strcpy(sender,gp->NXTADDR);
    retstr = subatomic_json_commands(gp,argjson,sender,valid);
    return(retstr);
}

void process_subatomic_AM(struct subatomic_info *dp,struct NXT_protocol_parms *parms)
{
    cJSON *argjson;
    //char *jsontxt;
    struct json_AM *ap;
    char *sender,*receiver;
    sender = parms->sender; receiver = parms->receiver; ap = parms->AMptr; //txid = parms->txid;
    if ( (argjson = parse_json_AM(ap)) != 0 && (strcmp(dp->NXTADDR,sender) == 0 || strcmp(dp->NXTADDR,receiver) == 0) )
    {
        printf("process_subatomic_AM got jsontxt.(%s)\n",ap->jsonstr);
        update_subatomic_state(dp,ap->funcid,ap->timestamp,argjson,ap->H.nxt64bits,sender,receiver);
        free_json(argjson);
    }
}

void process_subatomic_typematch(struct subatomic_info *dp,struct NXT_protocol_parms *parms)
{
    char NXTaddr[64],*sender,*receiver,*txid;
    sender = parms->sender; receiver = parms->receiver; txid = parms->txid;
    safecopy(NXTaddr,sender,sizeof(NXTaddr));
    printf("got txid.(%s) type.%d subtype.%d sender.(%s) -> (%s)\n",txid,parms->type,parms->subtype,sender,receiver);
}

void *subatomic_handler(struct NXThandler_info *mp,struct NXT_protocol_parms *parms,void *handlerdata,int32_t height)
{
    struct subatomic_info *gp = handlerdata;
    //static int32_t variant;
    cJSON *obj;
    char buf[512];
    if ( parms->txid == 0 )     // indicates non-transaction event
    {
        if ( parms->mode == NXTPROTOCOL_WEBJSON )
            return(subatomic_jsonhandler(parms->argjson));
        else if ( parms->mode == NXTPROTOCOL_NEWBLOCK )
        {
            //printf("subatomic new RTblock %d time %lld microseconds %ld\n",mp->RTflag,time(0),microseconds());
        }
        else if ( parms->mode == NXTPROTOCOL_IDLETIME )
        {
            update_subatomic_transfers(gp->NXTADDR);
            static int testhack;
            if ( testhack == 0 )
            {
#ifndef __APPLE__
                //char *teststr = "{\"requestType\":\"subatomic_trade\",\"NXT\":\"423766016895692955\",\"coin\":\"DOGE\",\"amount\":\"100\",\"coinaddr\":\"DNbAcP82bpd9xdXNA1Vtf1Vo6yqP1rZvcu\",\"senderip\":\"209.126.71.170\",\"destNXT\":\"8989816935121514892\",\"destcoin\":\"LTC\",\"destamount\":\".1\",\"destcoinaddr\":\"LLedxvb1e5aCYmQfn8PHEPFR56AsbGgbUG\"}";
                Signedtx = "00002724c6009a024e5bbad625df3d536fa90b1e6a28c3f5a56e1fcbe34132391c8d3fd7f671cb198c71b555df3ec27c00e1f5050000000000e1f50500000000000000000000000000000000000000000000000000000000000000000000000058f36933200b9766566c7c3a9250f4e7607f75011bda5b3ba40c9f820ae3a60f1471b68e203bf9897b147aced9938cfb91bdf3230a02637264e25bff85ce382a";
                char *teststr = "{\"requestType\":\"subatomic_trade\",\"type\":\"NXTatomicswap\",\"NXT\":\"423766016895692955\",\"mytxbytes\":\"00002724c6009a024e5bbad625df3d536fa90b1e6a28c3f5a56e1fcbe34132391c8d3fd7f671cb198c71b555df3ec27c00e1f5050000000000e1f505000000000000000000000000000000000000000000000000000000000000000000000000\",\"mysighash\":\"422bf4f0a389398589553995f4dcb0b2a0811641ffb7e8222ce3dc3272628065\",\"destNXT\":\"8989816935121514892\",\"othertxbytes\":\"0000be25c6009a0225c5fed2690701cf06f267e7c227b1a3c0dfa9c6fc3cdb593b3af6f16d65302f9b30f378f284e10500e1f5050000000000e1f505000000000000000000000000000000000000000000000000000000000000000000000000\",\"othersighash\":\"8e72bba1bdf481bd6c03e61bceb53dfa642be04cb71f23755b5fa939b24ffa55\"}";
#else
                //char *teststr = "{\"requestType\":\"subatomic_trade\",\"NXT\":\"8989816935121514892\",\"coin\":\"LTC\",\"amount\":\".1\",\"coinaddr\":\"LRQ3ZyrZDhvKFkcbmC6cLutvJfPbxnYUep\",\"senderip\":\"181.112.79.236\",\"destNXT\":\"423766016895692955\",\"destcoin\":\"DOGE\",\"destamount\":\"100\",\"destcoinaddr\":\"DNyEii2mnvVL1BM1kTZkmKDcf8SFyqAX4Z\"}";
                Signedtx = "0000be25c6009a0225c5fed2690701cf06f267e7c227b1a3c0dfa9c6fc3cdb593b3af6f16d65302f9b30f378f284e10500e1f5050000000000e1f50500000000000000000000000000000000000000000000000000000000000000000000000044b7df75da4a7132c3ee64bd00ad1b56f0e3be56dfefd48e7d56679dc2b857010cd7bf789e290e2c8f4c20653ccb273dce3f248d25fb44ae802289a2c95e3bbf";
                char *teststr = "{\"requestType\":\"subatomic_trade\",\"type\":\"NXTatomicswap\",\"NXT\":\"8989816935121514892\",\"mysighash\":\"8e72bba1bdf481bd6c03e61bceb53dfa642be04cb71f23755b5fa939b24ffa55\",\"mytxbytes\":\"0000be25c6009a0225c5fed2690701cf06f267e7c227b1a3c0dfa9c6fc3cdb593b3af6f16d65302f9b30f378f284e10500e1f5050000000000e1f505000000000000000000000000000000000000000000000000000000000000000000000000\",\"destNXT\":\"423766016895692955\",\"othersighash\":\"422bf4f0a389398589553995f4dcb0b2a0811641ffb7e8222ce3dc3272628065\",\"othertxbytes\":\"00002724c6009a024e5bbad625df3d536fa90b1e6a28c3f5a56e1fcbe34132391c8d3fd7f671cb198c71b555df3ec27c00e1f5050000000000e1f505000000000000000000000000000000000000000000000000000000000000000000000000\"}";
#endif
                cJSON *json = 0;
                if ( 0 && teststr != 0 )
                    json = cJSON_Parse(teststr);
                if ( json != 0 )
                {
                    subatomic_jsonhandler(json);
                    printf("HACK %s\n",cJSON_Print(json));
                    free_json(json);
                } else if ( 0 && teststr != 0 ) printf("error parsing.(%s)\n",teststr);
            }
            // printf("subatomic.%d new idletime %d time %ld microseconds %lld \n",testhack,mp->RTflag,time(0),(long long)microseconds());
            testhack++;
        }
        else if ( parms->mode == NXTPROTOCOL_INIT )
        {
            printf("subatomic NXThandler_info init %d\n",mp->RTflag);
            gp = Global_subatomic = calloc(1,sizeof(*Global_subatomic));
            gp->curl_handle = curl_easy_init();
            gp->curl_handle2 = curl_easy_init();
            strcpy(gp->ipaddr,mp->ipaddr);
            strcpy(gp->NXTADDR,mp->NXTADDR);
            strcpy(gp->NXTACCTSECRET,mp->NXTACCTSECRET);
            if ( mp->accountjson != 0 )
            {
                obj = cJSON_GetObjectItem(mp->accountjson,"enable_bitcoin_broadcast");
                copy_cJSON(buf,obj);
                gp->enable_bitcoin_broadcast = myatoi(buf,2);
                printf("enable_bitcoin_broadcast set to %d\n",gp->enable_bitcoin_broadcast);
            }
            //portnum = SUBATOMIC_PORTNUM;
            // if ( portable_thread_create(subatomic_server,&portnum) == 0 )
            //     printf("ERROR _server_loop\n");
            //variant = SUBATOMIC_VARIANT;
            /*register_variant_handler(SUBATOMIC_VARIANT,process_subatomic_packet,SUBATOMIC_SEND_PUBKEY,sizeof(struct subatomic_packet),sizeof(struct subatomic_packet),0);
             register_variant_handler(SUBATOMIC_VARIANT,process_subatomic_packet,SUBATOMIC_REFUNDTX_NEEDSIG,sizeof(struct subatomic_packet),sizeof(struct subatomic_packet),0);
             register_variant_handler(SUBATOMIC_VARIANT,process_subatomic_packet,SUBATOMIC_REFUNDTX_SIGNED,sizeof(struct subatomic_packet),sizeof(struct subatomic_packet),0);
             register_variant_handler(SUBATOMIC_VARIANT,process_subatomic_packet,SUBATOMIC_FUNDINGTX,sizeof(struct subatomic_packet),sizeof(struct subatomic_packet),0);
             register_variant_handler(SUBATOMIC_VARIANT,process_subatomic_packet,SUBATOMIC_SEND_MICROTX,sizeof(struct subatomic_packet),sizeof(struct subatomic_packet),0);
             if ( pthread_create(malloc(sizeof(pthread_t)),NULL,_server_loop,&variant) != 0 )
             printf("ERROR _server_loop\n");*/
        }
        return(gp);
    }
    else if ( parms->mode == NXTPROTOCOL_AMTXID )
        process_subatomic_AM(gp,parms);
    else if ( parms->mode == NXTPROTOCOL_TYPEMATCH )
        process_subatomic_typematch(gp,parms);
    return(gp);
}

int32_t pubkey_to_256bits(uint8_t *bytes,char *pubkey)
{
    //Bitcoin private keys are 32 bytes, but are often stored in their full OpenSSL-serialized form of 279 bytes. They are serialized as 51 base58 characters, or 64 hex characters.
    //Bitcoin public keys (traditionally) are 65 bytes (the first of which is 0x04). They are typically encoded as 130 hex characters.
    //Bitcoin compressed public keys (as of 0.6.0) are 33 bytes (the first of which is 0x02 or 0x03). They are typically encoded as 66 hex characters.
    //Bitcoin addresses are RIPEMD160(SHA256(pubkey)), 20 bytes. They are typically encoded as 34 base58 characters.
    char zpadded[65]; int32_t i,j,n;
    if ( (n= (int32_t)strlen(pubkey)) > 66 )
    {
        printf("pubkey_to_256bits pubkey.(%s) len.%ld > 66\n",pubkey,strlen(pubkey));
        return(-1);
    }
    if ( pubkey[0] != '0' || (pubkey[1] != '2' && pubkey[1] != '3') )
    {
        printf("pubkey_to_256bits pubkey.(%s) len.%ld unexpected first byte\n",pubkey,strlen(pubkey));
        return(-1);
    }
    for (i=0; i<64; i++)
        zpadded[i] = '0';
    zpadded[64] = 0;
    for (i=66-n,j=2; i<64; i++,j++)
        zpadded[i] = pubkey[j];
    if ( pubkey[j] != 0 )
    {
        printf("pubkey_to_256bits unexpected nonzero at j.%d\n",j);
        return(-1);
    }
    printf("pubkey.(%s) -> zpadded.(%s)\n",pubkey,zpadded);
    decode_hex(bytes,32,zpadded);
    //for (i=0; i<32; i++)
    //    printf("%02x",bytes[i]);
    //printf("\n");
    return(0);
}

#ifdef bitcoin_doesnt_seem_to_support_nonstandard_tx
/* https://bitcointalk.org/index.php?topic=193281.msg3315031#msg3315031
 
 jl777:
 This approach is supposed to be the "holy grail", but only one miner (elgius) even accepts the nonstandard
 transactions for bitcoin network. Probably none of the miners for any of the altcoins do, so basically it
 is a thought experiment that cant possible actually work. I realized this after coding some parts of it up...
 
 description:
 A picks a random number x
 
 A creates TX1: "Pay w BTC to <B's public key> if (x for H(x) known and signed by B) or (signed by A & B)"
 
 A creates TX2: "Pay w BTC from TX1 to <A's public key>, locked 48 hours in the future, signed by A"
 
 A sends TX2 to B
 
 B signs TX2 and returns to A
 
 1) A submits TX1 to the network
 
 B creates TX3: "Pay v alt-coins to <A-public-key> if (x for H(x) known and signed by A) or (signed by A & B)"
 
 B creates TX4: "Pay v alt-coins from TX3 to <B's public key>, locked 24 hours in the future, signed by B"
 
 B sends TX4 to A
 
 A signs TX4 and sends back to B
 
 2) B submits TX3 to the network
 
 3) A spends TX3 giving x
 
 4) B spends TX1 using x
 
 This is atomic (with timeout).  If the process is halted, it can be reversed no matter when it is stopped.
 
 Before 1: Nothing public has been broadcast, so nothing happens
 Between 1 & 2: A can use refund transaction after 48 hours to get his money back
 Between 2 & 3: B can get refund after 24 hours.  A has 24 more hours to get his refund
 After 3: Transaction is completed by 2
 - A must spend his new coin within 24 hours or B can claim the refund and keep his coins
 - B must spend his new coin within 48 hours or A can claim the refund and keep his coins
 
 For safety, both should complete the process with lots of time until the deadlines.
 locktime for A significantly longer than for B
 
 
 ************** NXT variant
 A creates any ref TX to generate fullhash, sighash, utxbytes to give to B, signature is used as x
 Pay w BTC to <B's public key> if (x for H(x) known and signed by B)
 
 B creates and broadcasts "pay NXT to A" referencing fullhash of refTX and sends unsigned txbytes to A, expires in 24 hours
 
 A broadcasts ref TX and gets paid the NXT
 B obtains signature from refTX and spends TX1
 
 
 **************
 OP_IF 0x63
 // Refund for A
 2 <pubkeyA> <pubkeyB> 2 OP_CHECKMULTISIGVERIFY 0xaf
 OP_ELSE 0x67
 // Ordinary claim for B
 OP_HASH160 0xa9 <H(x)> OP_EQUAL 0x87  <pubkeyB/pubkeyA> OP_CHECKSIGVERIFY 0xad
 OP_ENDIF 0x68
 
 0x63 2 <pubkeyA> <pubkeyB> 2 0xaf 0x67 0xa9 <H(x)> 0x87 <pubkeyB> 0xad
 */

#define SCRIPT_OP_IF 0x63
#define SCRIPT_OP_ELSE 0x67
#define SCRIPT_OP_ENDIF 0x68
#define SCRIPT_OP_EQUAL 0x87
#define SCRIPT_OP_HASH160 0xa9
#define SCRIPT_OP_CHECKSIGVERIFY 0xad
#define SCRIPT_OP_CHECKMULTISIGVERIFY 0xaf

int32_t atomictx_addbytes(unsigned char *hex,int32_t n,unsigned char *bytes,int32_t len)
{
    int32_t i;
    for (i=0; i<len; i++)
        hex[n++] = bytes[i];
    return(n);
}

char *create_atomictx_scriptPubkey(int a_or_b,char *pubkeyA,char *pubkeyB,unsigned char *hash160)
{
    char *retstr;
    unsigned char pubkeyAbytes[32],pubkeyBbytes[32],hex[4096];
    int32_t i,n = 0;
    //if ( pubkey_to_256bits(pubkeyAbytes,pubkeyA) < 0 || pubkey_to_256bits(pubkeyBbytes,pubkeyB) < 0 )
    //    return(0);
    decode_hex(pubkeyAbytes,32,pubkeyA);
    decode_hex(pubkeyBbytes,32,pubkeyB);
    hex[n++] = SCRIPT_OP_IF;
    hex[n++] = 2;
    hex[n++] = 32, memcpy(&hex[n],pubkeyAbytes,32), n += 32;
    hex[n++] = 32, memcpy(&hex[n],pubkeyBbytes,32), n += 32;
    hex[n++] = 2;
    hex[n++] = SCRIPT_OP_CHECKMULTISIGVERIFY;
    hex[n++] = SCRIPT_OP_ELSE;
    hex[n++] = SCRIPT_OP_HASH160;
    hex[n++] = 20; memcpy(&hex[n],hash160,20); n += 20;
    hex[n++] = SCRIPT_OP_EQUAL;
    if ( a_or_b == 'a' )
        hex[n++] = 32, memcpy(&hex[n],pubkeyAbytes,32), n += 32;
    else if ( a_or_b == 'b' )
        hex[n++] = 32, memcpy(&hex[n],pubkeyBbytes,32), n += 32;
    else { printf("illegal a_or_b %d\n",a_or_b); return(0); }
    hex[n++] = SCRIPT_OP_CHECKSIGVERIFY;
    hex[n++] = SCRIPT_OP_ENDIF;
    retstr = malloc(n*2+1);
    printf("pubkeyA.(%s) pubkeyB.(%s) ->\n",pubkeyA,pubkeyB);
    for (i=0; i<n; i++)
    {
        retstr[i*2] = hexbyte((hex[i]>>4) & 0xf);
        retstr[i*2 + 1] = hexbyte(hex[i] & 0xf);
        printf("%02x",hex[i]);
    }
    retstr[n*2] = 0;
    printf("-> (%s)\n",retstr);
    return(retstr);
}

void _init_atomictx_half(struct atomictx_half *atomic,int coinid,char *txid,int vout,char *scriptpubkey,uint64_t amount,char *srcaddr,char *srcpubkey,char *destaddr,char *destpubkey)
{
    atomic->amount = amount; atomic->vout = vout; atomic->coinid = coinid;
    atomic->srctxid = txid; atomic->srcscriptpubkey = scriptpubkey;
    atomic->srcaddr = srcaddr; atomic->srcpubkey = srcpubkey;
    atomic->destaddr = destaddr; atomic->destpubkey = destpubkey;
}

void gen_atomictx_initiator(struct atomictx_half *atomic)
{
    int32_t i;
    cJSON *obj,*json;
    char msg[65],txidstr[4096],*jsonstr;
    unsigned char hash160[20],x[32];
    randombytes(x,sizeof(x)/sizeof(*x));
    for (i=0; i<(int32_t)(sizeof(x)/sizeof(*x)); i++)
    {
        msg[i*2] = hexbyte((x[i]>>4) & 0xf);
        msg[i*2 + 1] = hexbyte(x[i] & 0xf);
        printf("%02x ",x[i]);
    }
    msg[64] = 0;
    printf("-> (%s) x[]\n",msg);
    calc_OP_HASH160(hash160,msg);
    atomic->redeemScript = create_atomictx_scriptPubkey('b',atomic->srcpubkey,atomic->destpubkey,hash160);
    if ( atomic->redeemScript != 0 )
    {
        json = cJSON_CreateObject();
        obj = cJSON_CreateString(atomic->srctxid); cJSON_AddItemToObject(json,"txid",obj);
        obj = cJSON_CreateNumber(atomic->vout); cJSON_AddItemToObject(json,"vout",obj);
        obj = cJSON_CreateString(atomic->srcscriptpubkey); cJSON_AddItemToObject(json,"scriptPubKey",obj);
        obj = cJSON_CreateString(atomic->redeemScript); cJSON_AddItemToObject(json,"redeemScript",obj);
        jsonstr = cJSON_Print(json);
        free_json(json);
        sprintf(txidstr,"'[%s]'  '{\"%s\":%lld}'",jsonstr,atomic->srcaddr,(long long)atomic->amount);
        stripwhite(txidstr,strlen(txidstr));
        printf("refundtx.(%s) change last 4 bytes or rawtransaction to future\n",txidstr);
        atomic->refundtx = clonestr(txidstr);
        sprintf(txidstr,"'[%s]'  '{\"%s\":%lld}'",jsonstr,atomic->destaddr,(long long)atomic->amount);
        stripwhite(txidstr,strlen(txidstr));
        printf("claimtx.(%s)\n",txidstr);
        free(jsonstr);
    }
}

void test_atomictx()
{
    struct atomictx_half initiator;
    _init_atomictx_half(&initiator,DOGE_COINID,"48ae45b3741c125ba63c8b98f2adb4dcfd050583734ca57a12d5bc874f8334bd",1,"76a914d43f07a88b9cf437314d5c281f201dd0bb5486e588ac",2*SATOSHIDEN,"DQVMJKcn3yW34nWpqheiNiRAhaY9qFeuER","0279886d0de4bfba245774d0c0a5c062578244b03eec567fe71a02c410b45ca86a","DDNhdeLp4PrwKKN1PeFnUv2pnzvhPLMgCd","03628f9fa04ed2ed83e07f8d3b73a654d1df86e670542008b96b9339a27f1905cb");
    gen_atomictx_initiator(&initiator);
}
#endif

#endif
#endif

#define SUBATOMIC_ABORTED -1
#define SUBATOMIC_HAVEREFUND 1
#define SUBATOMIC_WAITFOR_CONFIRMS 2
#define SUBATOMIC_COMPLETED 3

#define MAX_SUBATOMIC_OUTPUTS 4
#define MAX_SUBATOMIC_INPUTS 16
#define SUBATOMIC_STARTING_SEQUENCEID 1000
#define SUBATOMIC_LOCKTIME (3600 * 2)
#define SUBATOMIC_DONATIONRATE .001
#define SUBATOMIC_DEFAULTINCR 100
#define SUBATOMIC_TYPE 0

struct subatomic_unspent_tx
{
    int64_t amount;    // MUST be first!
    uint32_t vout,confirmations;
    struct destbuf txid,address,scriptPubKey,redeemScript;
};

struct subatomic_rawtransaction
{
    char destaddrs[MAX_SUBATOMIC_OUTPUTS][64];
    int64_t amount,change,inputsum,destamounts[MAX_SUBATOMIC_OUTPUTS];
    int32_t numoutputs,numinputs,completed,broadcast,confirmed;
    char rawtransaction[1024],signedtransaction[1024],txid[128];
    struct subatomic_unspent_tx inputs[MAX_SUBATOMIC_INPUTS];   // must be last, could even make it variable sized
};

struct subatomic_halftx
{
    int32_t minconfirms;
    int64_t destamount,avail,amount,myamount,otheramount;  // amount = (myamount + otheramount + donation + txfee)
    struct subatomic_rawtransaction funding,refund,micropay;
    struct destbuf funding_scriptPubKey,multisigaddr,redeemScript;
    char fundingtxid[128],refundtxid[128],micropaytxid[128],countersignedrefund[1024],completedmicropay[1024];
    char NXTaddr[64],coinaddr[64],pubkey[128],ipaddr[32],coinstr[16],destcoinstr[16];
    char otherNXTaddr[64],destcoinaddr[64],destpubkey[128],otheripaddr[32];
};

struct subatomic_tx_args
{
    char coinstr[16],destcoinstr[16],NXTaddr[64],otherNXTaddr[64],coinaddr[2][64],destcoinaddr[2][64],otheripaddr[32],mypubkeys[2][128];
    int64_t amount,destamount;
    double myshare;
    int32_t numincr,incr,otherincr;
};

struct subatomic_tx
{
    struct subatomic_tx_args ARGS,otherARGS;
    struct subatomic_halftx myhalf,otherhalf;
    char *claimtxid;
    int64_t lastcontact,myexpectedamount,myreceived,otherexpectedamount,sent_to_other;
    int32_t status,initflag,connsock,refundlockblock,cantsend,type,longerflag,tag,verified;
    int32_t txs_created,other_refundtx_done,myrefundtx_done,other_fundingtx_confirms;
    int32_t other_refundtx_waiting,myrefundtx_waiting,other_fundingtx_waiting,other_micropaytx_waiting;
    unsigned char recvbufs[4][sizeof(struct subatomic_rawtransaction)];
};


#define SCRIPT_OP_IF 0x63
#define SCRIPT_OP_ELSE 0x67
#define SCRIPT_OP_DUP 0x76
#define SCRIPT_OP_ENDIF 0x68
#define SCRIPT_OP_TRUE 0x51
#define SCRIPT_OP_2 0x52
#define SCRIPT_OP_3 0x53
#define SCRIPT_OP_EQUALVERIFY 0x88
#define SCRIPT_OP_HASH160 0xa9
#define SCRIPT_OP_EQUAL 0x87
#define SCRIPT_OP_CHECKSIG 0xac
#define SCRIPT_OP_CHECKMULTISIG 0xae
#define SCRIPT_OP_CHECKMULTISIGVERIFY 0xaf

char *create_atomictx_scripts(char *scriptPubKey,char *pubkeyA,char *pubkeyB,char *hash160str)
{
    // if ( refund ) OP_HASH160 <2of2 multisig hash> OP_EQUAL   // standard multisig
    // else OP_DUP OP_HASH160 <hash160> OP_EQUALVERIFY OP_CHECKSIG // standard spend
    char *retstr; uint8_t pubkeyAbytes[33],pubkeyBbytes[33],hash160[20],tmpbuf[24],hex[4096]; int32_t i,n = 0;
    decode_hex(pubkeyAbytes,33,pubkeyA);
    decode_hex(pubkeyBbytes,33,pubkeyB);
    decode_hex(hash160,20,hash160str);
    hex[n++] = SCRIPT_OP_IF;
    hex[n++] = SCRIPT_OP_2;
    hex[n++] = 33, memcpy(&hex[n],pubkeyAbytes,33), n += 33;
    hex[n++] = 33, memcpy(&hex[n],pubkeyBbytes,33), n += 33;
    hex[n++] = SCRIPT_OP_2;
    hex[n++] = SCRIPT_OP_CHECKMULTISIG;
    hex[n++] = SCRIPT_OP_ELSE;
    hex[n++] = SCRIPT_OP_DUP;
    hex[n++] = SCRIPT_OP_HASH160;
    hex[n++] = 20; memcpy(&hex[n],hash160,20); n += 20;
    hex[n++] = SCRIPT_OP_EQUALVERIFY;
    hex[n++] = SCRIPT_OP_CHECKSIG;
    hex[n++] = SCRIPT_OP_ENDIF;
    retstr = calloc(1,n*2+16);
    //printf("pubkeyA.(%s) pubkeyB.(%s) hash160.(%s) ->\n",pubkeyA,pubkeyB,hash160str);
    //strcpy(retstr,"01");
    //sprintf(retstr+2,"%02x",n);
    for (i=0; i<n; i++)
    {
        retstr[4*0 + i*2] = hexbyte((hex[i]>>4) & 0xf);
        retstr[4*0 + i*2 + 1] = hexbyte(hex[i] & 0xf);
        //printf("%02x",hex[i]);
    }
    retstr[4*0 + n*2] = 0;
    calc_OP_HASH160(scriptPubKey,tmpbuf+2,retstr+4*0);
    tmpbuf[0] = SCRIPT_OP_HASH160;
    tmpbuf[1] = 20;
    tmpbuf[22] = SCRIPT_OP_EQUAL;
    init_hexbytes_noT(scriptPubKey,tmpbuf,23);
    //base58_conv(addr,5,tmpbuf+2);
    //addr[0] = 0;
    //printf("-> redeemScript.(%s) scriptPubKey.(%s) p2sh.(%s)\n",retstr,scriptPubKey,addr);
    return(retstr);
}

cJSON *get_transaction_json(struct coin777 *coin,char *txid)
{
    char txidstr[512],*transaction = 0; cJSON *json = 0;
    if ( coin == 0 )
        return(0);
    sprintf(txidstr,"\"%s\"",txid);
    if ( (transaction= bitcoind_passthru(coin->name,coin->serverport,coin->userpass,"gettransaction",txidstr)) != 0 && transaction[0] != 0 )
    {
        //printf("got transaction.(%s)\n",transaction);
        json = cJSON_Parse(transaction);
    }
    if ( transaction != 0 )
        free(transaction);
    return(json);
}

int32_t subatomic_tx_confirmed(struct coin777 *coin,char *txid)
{
    cJSON *json; int32_t numconfirmed = -1;
    if ( (json= get_transaction_json(coin,txid)) != 0 )
    {
        numconfirmed = (int32_t)get_cJSON_int(json,"confirmations");
        free_json(json);
    }
    return(numconfirmed);
}

cJSON *get_decoderaw_json(struct coin777 *coin,char *rawtransaction)
{
    char *str,*retstr; cJSON *json = 0;
    str = malloc(strlen(rawtransaction)+4);
    //printf("got rawtransaction.(%s)\n",rawtransaction);
    sprintf(str,"\"%s\"",rawtransaction);
    if ( (retstr= bitcoind_passthru(coin->name,coin->serverport,coin->userpass,"decoderawtransaction",str)) != 0 && retstr[0] != 0 )
    {
        //printf("got decodetransaction.(%s)\n",retstr);
        json = cJSON_Parse(retstr);
    } else printf("error decoding.(%s)\n",str);
    if ( retstr != 0 )
        free(retstr);
    free(str);
    return(json);
}

cJSON *get_rawtransaction_json(struct coin777 *coin,char *txid)
{
    char txidstr[512],*rawtransaction=0; cJSON *json = 0;
    if ( coin == 0 )
        return(0);
    sprintf(txidstr,"\"%s\"",txid);
    if ( (rawtransaction= bitcoind_passthru(coin->name,coin->serverport,coin->userpass,"getrawtransaction",txidstr)) != 0 && rawtransaction[0] != 0 )
        json = get_decoderaw_json(coin,rawtransaction);
    else printf("error with getrawtransaction %s %s\n",coin->name,txid);
    if ( rawtransaction != 0 )
        free(rawtransaction);
    return(json);
}

int32_t generate_multisigaddr(struct destbuf *multisigaddr,struct destbuf *redeemScript,struct coin777 *coin,char *serverport,char *userpass,int32_t addmultisig,char *params)
{
    char addr[1024],*retstr; cJSON *json,*redeemobj,*msigobj; int32_t flag = 0;
    if ( addmultisig != 0 )
    {
        if ( (retstr= bitcoind_passthru(coin->name,serverport,userpass,"addmultisigaddress",params)) != 0 )
        {
            strcpy(multisigaddr->buf,retstr);
            free(retstr);
            sprintf(addr,"\"%s\"",multisigaddr->buf);
            if ( (retstr= bitcoind_passthru(coin->name,serverport,userpass,"validateaddress",addr)) != 0 )
            {
                json = cJSON_Parse(retstr);
                if ( json == 0 ) printf("Error before: [%s]\n",cJSON_GetErrorPtr());
                else
                {
                    if ( (redeemobj= cJSON_GetObjectItem(json,"hex")) != 0 )
                    {
                        copy_cJSON(redeemScript,redeemobj);
                        flag = 1;
                    } else printf("missing redeemScript in (%s)\n",retstr);
                    free_json(json);
                }
                free(retstr);
            }
        } else printf("error creating multisig address\n");
    }
    else
    {
        if ( (retstr= bitcoind_passthru(coin->name,serverport,userpass,"createmultisig",params)) != 0 )
        {
            json = cJSON_Parse(retstr);
            if ( json == 0 ) printf("Error before: [%s]\n",cJSON_GetErrorPtr());
            else
            {
                if ( (msigobj= cJSON_GetObjectItem(json,"address")) != 0 )
                {
                    if ( (redeemobj= cJSON_GetObjectItem(json,"redeemScript")) != 0 )
                    {
                        copy_cJSON(multisigaddr,msigobj);
                        copy_cJSON(redeemScript,redeemobj);
                        flag = 1;
                    } else printf("missing redeemScript in (%s)\n",retstr);
                } else printf("multisig missing address in (%s) params.(%s)\n",retstr,params);
                free_json(json);
            }
            free(retstr);
        } else printf("error issuing createmultisig.(%s)\n",params);
    }
    return(flag);
}

char *createmultisig_json_params(struct pubkey_info *pubkeys,int32_t m,int32_t n,char *acctparm)
{
    int32_t i; char *paramstr = 0; cJSON *array,*mobj,*keys,*key;
    keys = cJSON_CreateArray();
    for (i=0; i<n; i++)
    {
        key = cJSON_CreateString(pubkeys[i].pubkey);
        cJSON_AddItemToArray(keys,key);
    }
    mobj = cJSON_CreateNumber(m);
    array = cJSON_CreateArray();
    if ( array != 0 )
    {
        cJSON_AddItemToArray(array,mobj);
        cJSON_AddItemToArray(array,keys);
        if ( acctparm != 0 )
            cJSON_AddItemToArray(array,cJSON_CreateString(acctparm));
        paramstr = cJSON_Print(array);
        _stripwhite(paramstr,' ');
        free_json(array);
    }
    printf("createmultisig_json_params.(%s)\n",paramstr);
    return(paramstr);
}

int32_t subatomic_createmultisig(struct destbuf *multisigaddr,struct destbuf *redeemScript,struct coin777 *coin,char *serverport,char *userpass,int32_t use_addmultisig,int32_t m,int32_t n,char *account,struct pubkey_info *pubkeys)
{
    int32_t flag = 0; char *params;
    if ( (params = createmultisig_json_params(pubkeys,m,n,(use_addmultisig != 0) ? account : 0)) != 0 )
    {
        flag = generate_multisigaddr(multisigaddr,redeemScript,coin,serverport,userpass,use_addmultisig,params);
        free(params);
    } else printf("error generating msig params\n");
    return(flag);
}

int32_t subatomic_gen_multisig(struct subatomic_tx *atx,struct subatomic_halftx *htx)
{
    char coinaddrs[3][64]; struct destbuf pubkeys[3]; struct coin777 *coin; struct pubkey_info pubkeydata[16];
    if ( (coin= coin777_find(htx->coinstr,0)) == 0 )
        return(-1);
    if ( htx->coinaddr[0] != 0 && htx->pubkey[0] != 0 && atx->otherhalf.coinaddr[0] != 0 && atx->otherhalf.pubkey[0] != 0 )
    {
        safecopy(coinaddrs[0],htx->coinaddr,sizeof(coinaddrs[0]));
        safecopy(pubkeys[0].buf,htx->pubkey,sizeof(pubkeys[0]));
        safecopy(coinaddrs[1],atx->otherhalf.coinaddr,sizeof(coinaddrs[1]));
        safecopy(pubkeys[1].buf,atx->otherhalf.pubkey,sizeof(pubkeys[1]));
        strcpy(pubkeydata[0].pubkey,pubkeys[0].buf);
        strcpy(pubkeydata[1].pubkey,pubkeys[1].buf);
        subatomic_createmultisig(&htx->multisigaddr,&htx->redeemScript,coin,coin->serverport,coin->userpass,coin->mgw.use_addmultisig,2,2,0,pubkeydata);
        //#ifdef DEBUG_MODE
        if ( htx->multisigaddr.buf[0] != 0 )
            get_pubkey(&pubkeys[2],coin->name,coin->serverport,coin->userpass,htx->multisigaddr.buf);
        //#endif
        return(htx->multisigaddr.buf[0] != 0);
    } else printf("cant gen multisig %d %d %d %d\n",htx->coinaddr[0],htx->pubkey[0],atx->otherhalf.coinaddr[0],atx->otherhalf.pubkey[0]);
    return(-1);
}

char *subatomic_broadcasttx(struct subatomic_halftx *htx,char *bytes,int32_t myincr,int32_t lockedblock)
{
    FILE *fp; char fname[512],*retstr = 0; cJSON *txjson; struct coin777 *coin;
    if ( (coin= coin777_find(htx->coinstr,0)) == 0 )
        return(0);
    txjson = get_decoderaw_json(coin,bytes);
    if ( txjson != 0 )
    {
        retstr = cJSON_Print(txjson);
        printf("broadcasting is disabled for now: (%s) ->\n(%s)\n",bytes,retstr);
        sprintf(fname,"backups/%s_%lld_%s_%s_%lld.%03d_%d",coin->name,(long long)htx->amount,htx->otherNXTaddr,htx->destcoinstr,(long long)htx->destamount,myincr,lockedblock);
        if ( (fp=fopen(fname,"w")) != 0 )
        {
            fprintf(fp,"%s\n%s\n",bytes,retstr);
            fclose(fp);
        }
        free(retstr);
    }
    retstr = 0;
    if ( 0 )
    {
        if ( (retstr= bitcoind_passthru(coin->name,coin->serverport,coin->userpass,"sendrawtransaction",bytes)) != 0 )
        {
            printf("sendrawtransaction returns.(%s)\n",retstr);
        }
    }
    return(retstr);
}

struct subatomic_unspent_tx *gather_unspents(uint64_t *totalp,int32_t *nump,struct coin777 *coin,char *coinaddr)
{
    int32_t i,j,num; struct subatomic_unspent_tx *ups = 0; char params[128],*retstr; cJSON *json,*item;
    /*{
     "txid" : "1ccd2a9d0f8d690ed13b6768fc6c041972362f5531922b6b152ed2c98d3fe113",
     "vout" : 1,
     "address" : "DK3nxu6GshBcQNDMqc66ARcwqDZ1B5TJe5",
     "scriptPubKey" : "76a9149891029995222077889b36c77e2b85690878df9088ac",
     "amount" : 2.00000000,
     "confirmations" : 72505
     },*/
    *totalp = 0;
    sprintf(params,"%d, 99999999",coin->minconfirms);
    if ( (retstr= bitcoind_passthru(coin->name,coin->serverport,coin->userpass,"listunspent",params)) != 0 )
    {
        //printf("unspents (%s)\n",retstr);
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            if ( is_cJSON_Array(json) != 0 && (num= cJSON_GetArraySize(json)) > 0 )
            {
                ups = calloc(num,sizeof(struct subatomic_unspent_tx));
                for (i=j=0; i<num; i++)
                {
                    item = cJSON_GetArrayItem(json,i);
                    copy_cJSON(&ups[j].address,cJSON_GetObjectItem(item,"address"));
                    if ( coinaddr == 0 || strcmp(coinaddr,ups[j].address.buf) == 0 )
                    {
                        copy_cJSON(&ups[j].txid,cJSON_GetObjectItem(item,"txid"));
                        copy_cJSON(&ups[j].scriptPubKey,cJSON_GetObjectItem(item,"scriptPubKey"));
                        ups[j].vout = (int32_t)get_cJSON_int(item,"vout");
                        ups[j].amount = conv_cJSON_float(item,"amount");
                        ups[j].confirmations = (int32_t)get_cJSON_int(item,"confirmations");
                        *totalp += ups[j].amount;
                        j++;
                    }
                }
                *nump = j;
                if ( j > 1 )
                {
                    int _decreasing_signedint64(const void *a,const void *b);
                    qsort(ups,j,sizeof(*ups),_decreasing_signedint64);
                    if ( coin->changeaddr[0] == 0 )
                        strcpy(coin->changeaddr,ups[0].address.buf);
                    //for (i=0; i<j; i++)
                    //printf("%s/v%-3d %13.6f %s confs.%-6d | total %.6f\n",ups[i].txid.buf,ups[i].vout,dstr(ups[i].amount),ups[i].address.buf,ups[i].confirmations,dstr(*totalp));
                }
            }
            free_json(json);
        }
        free(retstr);
    }
    return(ups);
}

struct subatomic_unspent_tx *subatomic_bestfit(uint64_t *valuep,struct coin777 *coin,struct subatomic_unspent_tx *unspents,int32_t numunspents,uint64_t value)
{
    int32_t i; uint64_t above,below,gap,atx_value; struct subatomic_unspent_tx *vin,*abovevin,*belowvin;
    abovevin = belowvin = 0;
    *valuep = 0;
    for (above=below=i=0; i<numunspents; i++)
    {
        vin = &unspents[i];
        *valuep = atx_value = vin->amount;
        if ( atx_value == value )
            return(vin);
        else if ( atx_value > value )
        {
            gap = (atx_value - value);
            if ( above == 0 || gap < above )
            {
                above = gap;
                abovevin = vin;
            }
        }
        else
        {
            gap = (value - atx_value);
            if ( below == 0 || gap < below )
            {
                below = gap;
                belowvin = vin;
            }
        }
    }
    return((abovevin != 0) ? abovevin : belowvin);
}

int64_t subatomic_calc_rawinputs(struct coin777 *coin,struct subatomic_rawtransaction *rp,uint64_t amount,struct subatomic_unspent_tx *ups,int32_t num,uint64_t donation)
{
    uint64_t value,sum = 0; struct subatomic_unspent_tx *up; int32_t i;
    rp->inputsum = rp->numinputs = 0;
    printf("unspent num %d, amount %.8f vs donation %.8f txfee %.8f\n",num,dstr(amount),dstr(donation),dstr(coin->mgw.txfee));
    if ( coin == 0 || (donation + coin->mgw.txfee) > amount || num == 0 )
        return(0);
    amount += coin->mgw.txfee + donation;
    for (i=0; i<num&&i<((int32_t)(sizeof(rp->inputs)/sizeof(*rp->inputs))); i++)
    {
        if ( (up= subatomic_bestfit(&value,coin,ups,num,amount)) != 0 )
        {
            sum += up->amount;
            printf("%.8f ",dstr(value));
            rp->inputs[rp->numinputs++] = *up;
            if ( sum >= amount )
            {
                rp->amount = (amount - coin->mgw.txfee - donation);
                rp->change = (sum - amount);
                rp->inputsum = sum;
                printf("numinputs %d sum %.8f vs amount %.8f change %.8f -> txfee %.8f\n",rp->numinputs,dstr(rp->inputsum),dstr(amount),dstr(rp->change),dstr(sum - rp->change - rp->amount));
                return(rp->inputsum);
            }
        }
        printf("error getting bestfit unspent\n");
        break;
    }
    printf("i.%d error numinputs %d sum %.8f\n",i,rp->numinputs,dstr(rp->inputsum));
    return(0);
}

int32_t subatomic_calc_rawoutputs(struct subatomic_halftx *htx,struct coin777 *coin,struct subatomic_rawtransaction *rp,double myshare,char *myaddr,char *otheraddr,char *changeaddr,uint64_t donation)
{
    int32_t n = 0;
    printf("rp->amount %.8f vs %.8f, txfee %.8f, change %.8f vs %.8f (%.8f - %.8f) donation %.8f\n",dstr(rp->amount),dstr(htx->avail),dstr(coin->mgw.txfee),dstr(rp->change),dstr(rp->inputsum - htx->avail),dstr(rp->inputsum),dstr(htx->avail),dstr(donation));
    if ( rp->amount == htx->avail && rp->change == (rp->inputsum - htx->avail - coin->mgw.txfee - donation) )
    {
        if ( changeaddr == 0 )
            changeaddr = coin->donationaddress;
        if ( coin->donationaddress[0] == 0 )
            donation = 0;
        htx->myamount = rp->amount * myshare;
        if ( htx->myamount > rp->amount )
            htx->myamount = rp->amount;
        htx->otheramount = (rp->amount - htx->myamount);
        if ( htx->myamount > 0 && myaddr != 0 )
        {
            if ( otheraddr != 0 && strcmp(myaddr,otheraddr) == 0 )
            {
                htx->myamount += htx->otheramount;
                htx->otheramount = 0;
            }
            if ( changeaddr != 0 && strcmp(myaddr,changeaddr) == 0 )
            {
                htx->myamount += rp->change;
                rp->change = 0;
            }
            safecopy(rp->destaddrs[n],myaddr,sizeof(rp->destaddrs[n]));
            rp->destamounts[n] = htx->myamount;
            //printf("(%s) <- %.8f mine\n",myaddr,dstr(htx->myamount));
            n++;
        }
        if ( otheraddr == 0 && htx->otheramount != 0 )
        {
            printf("no otheraddr, boost donation by %.8f\n",dstr(htx->otheramount));
            donation += htx->otheramount;
            htx->otheramount = 0;
        }
        if ( htx->otheramount > 0 )
        {
            safecopy(rp->destaddrs[n],otheraddr,sizeof(rp->destaddrs[n]));
            rp->destamounts[n] = htx->otheramount;
            //printf("(%s) <- %.8f other\n",otheraddr,dstr(htx->otheramount));
            n++;
        }
        if ( changeaddr == 0 && (rp->change != 0 || strcmp(changeaddr,coin->donationaddress) == 0) )
        {
            printf("no changeaddr, boost donation %.8f\n",dstr(rp->change));
            donation += rp->change;
            rp->change = 0;
        }
        if ( rp->change > 0 )
        {
            safecopy(rp->destaddrs[n],changeaddr,sizeof(rp->destaddrs[n]));
            rp->destamounts[n] = rp->change;
            //printf("(%s) <- %.8f change\n",changeaddr,dstr(rp->change));
            n++;
        }
        if ( donation > 0 && coin->donationaddress[0] != 0 )
        {
            safecopy(rp->destaddrs[n],coin->donationaddress,sizeof(rp->destaddrs[n]));
            rp->destamounts[n] = donation;
            //printf("(%s) <- %.8f donation\n",coin->donationaddress,dstr(donation));
            n++;
        }
    }
    else printf("error myshare %.6f %.8f -> %s, other %.8f -> %s, change %.8f -> %s, donation %.8f -> %s \n",myshare,dstr(htx->myamount),myaddr,dstr(htx->otheramount),otheraddr,dstr(rp->change),changeaddr,dstr(donation),coin->donationaddress);
    rp->numoutputs = n;
    //printf("numoutputs.%d\n",n);
    return(n);
}

int32_t script_has_coinaddr(cJSON *scriptobj,char *coinaddr)
{
    int32_t i,n; struct destbuf buf; cJSON *addresses,*addrobj;
    if ( scriptobj == 0 )
        return(0);
    addresses = cJSON_GetObjectItem(scriptobj,"addresses");
    if ( addresses != 0 )
    {
        n = cJSON_GetArraySize(addresses);
        for (i=0; i<n; i++)
        {
            addrobj = cJSON_GetArrayItem(addresses,i);
            copy_cJSON(&buf,addrobj);
            if ( strcmp(buf.buf,coinaddr) == 0 )
                return(1);
        }
    }
    return(0);
}

char *subatomic_decodetxid(int64_t *valuep,struct destbuf *scriptPubKey,int32_t *locktimep,struct coin777 *coin,char *rawtransaction,char *mycoinaddr)
{
    char *txidstr,checkasmstr[1024],*asmstr,*txid = 0; uint64_t value = 0; int32_t i,n,nval,reqSigs; cJSON *json,*scriptobj,*array,*item,*hexobj;
    *locktimep = -1;
    if ( (json= get_decoderaw_json(coin,rawtransaction)) != 0 )
    {
        *locktimep = (int32_t)get_cJSON_int(json,"locktime");
        if ( (txidstr= jstr(json,"txid")) == 0 )
        {
            printf("subatomic_decodetxid no txid\n");
            return(0);
        }
        txid = clonestr(txidstr);
        array = cJSON_GetObjectItem(json,"vout");
        if ( mycoinaddr != 0 && is_cJSON_Array(array) != 0 )
        {
            n = cJSON_GetArraySize(array);
            for (i=0; i<n; i++)
            {
                item = cJSON_GetArrayItem(array,i);
                hexobj = 0;
                scriptobj = cJSON_GetObjectItem(item,"scriptPubKey");
                if ( mycoinaddr != 0 && scriptobj != 0 && script_has_coinaddr(scriptobj,mycoinaddr) != 0 )
                {
                    nval = (int32_t)get_cJSON_int(item,"n");
                    if ( nval == i )
                    {
                        reqSigs = (int32_t)get_cJSON_int(item,"reqSigs");
                        value = conv_cJSON_float(item,"value");
                        hexobj = cJSON_GetObjectItem(scriptobj,"hex");
                        if ( scriptPubKey != 0 && hexobj != 0 )
                            copy_cJSON(scriptPubKey,hexobj);
                        if ( reqSigs == 1 && hexobj != 0 )
                        {
                            sprintf(checkasmstr,"OP_DUP OP_HASH160 %s OP_EQUALVERIFY OP_CHECKSIG","need to figure out how ot gen magic number");
                            if ( (asmstr= jstr(scriptobj,"asm")) != 0 && strcmp(asmstr,checkasmstr) != 0 )
                                printf("warning: (%s) != check.(%s)\n",asmstr,checkasmstr);
                        }
                    }
                }
            }
        }
    }
    if ( valuep != 0 )
        *valuep = value;
    return(txid);
}

cJSON *subatomic_vins_json_params(struct coin777 *coin,struct subatomic_rawtransaction *rp)
{
    int32_t i; cJSON *json,*array; struct subatomic_unspent_tx *up;
    array = cJSON_CreateArray();
    for (i=0; i<rp->numinputs; i++)
    {
        up = &rp->inputs[i];
        json = cJSON_CreateObject();
        jaddstr(json,"txid",up->txid.buf);
        jaddnum(json,"vout",up->vout);
        if ( up->scriptPubKey.buf[0] != 0 )
            jaddstr(json,"scriptPubKey",up->scriptPubKey.buf);
        if ( up->redeemScript.buf[0] != 0 )
            jaddstr(json,"redeemScript",up->redeemScript.buf);
        cJSON_AddItemToArray(array,json);
    }
    return(array);
}

cJSON *subatomic_privkeys_json_params(struct coin777 *coin,char **coinaddrs,int32_t n)
{
    int32_t i; char *privkey; cJSON *array = cJSON_CreateArray();
    //sprintf(walletkey,"[\"%s\",%d]",Global_subatomic->NXTADDR,BITCOIN_WALLET_UNLOCKSECONDS);
    // locking first avoids error, hacky but no time for wallet fiddling now
    //bitcoind_RPC(0,coin->name,coin->serverport,coin->userpass,"walletlock",0);
    //bitcoind_RPC(0,coin->name,coin->serverport,coin->userpass,"walletpassphrase",walletkey);
    for (i=0; i<n; i++)
    {
        if ( coinaddrs[i][0] != 0 )
        {
            printf("privkeys.(%s)\n",coinaddrs[i]);
            if ( (privkey= dumpprivkey(coin->name,coin->serverport,coin->userpass,coinaddrs[i])) != 0 )
            {
                jaddistr(array,privkey);
                free(privkey);
            }
        }
    }
    return(array);
}

char *subatomic_signraw_json_params(char *skipaddr,char *coinaddr,struct coin777 *coin,struct subatomic_rawtransaction *rp,char *rawbytes)
{
    int32_t i,j,flag; char *coinaddrs[MAX_SUBATOMIC_INPUTS+1],*paramstr = 0; cJSON *array,*rawobj,*vinsobj,*keysobj;
    if ( (rawobj= cJSON_CreateString(rawbytes)) != 0 )
    {
        if ( (vinsobj= subatomic_vins_json_params(coin,rp)) != 0 )
        {
            // printf("add %d inputs skipaddr.%s coinaddr.%s\n",rp->numinputs,skipaddr,coinaddr);
            for (i=flag=j=0; i<rp->numinputs; i++)
            {
                //printf("i.%d j.%d flag.%d %s\n",i,j,flag,rp->inputs[i].address);
                if ( skipaddr != 0 && strcmp(rp->inputs[i].address.buf,skipaddr) != 0 )
                {
                    coinaddrs[j] = rp->inputs[i].address.buf;
                    if ( coinaddr != 0 && strcmp(coinaddrs[j],coinaddr) == 0 )
                        flag++;
                    j++;
                }
            }
            //printf("i.%d j.%d flag.%d\n",i,j,flag);
            if ( coinaddr != 0 && flag == 0 )
                coinaddrs[j++] = coinaddr;
            coinaddrs[j] = 0;
            keysobj = subatomic_privkeys_json_params(coin,coinaddrs,j);
            if ( keysobj != 0 )
            {
                array = cJSON_CreateArray();
                cJSON_AddItemToArray(array,rawobj);
                cJSON_AddItemToArray(array,vinsobj);
                cJSON_AddItemToArray(array,keysobj);
                paramstr = cJSON_Print(array);
                free_json(array);
            }
            else free_json(vinsobj);
        }
        else free_json(rawobj);
    }
    return(paramstr);
}

char *subatomic_signtx(char *skipaddr,int32_t *lockedblockp,int64_t *valuep,char *coinaddr,char *deststr,unsigned long destsize,struct coin777 *coin,struct subatomic_rawtransaction *rp,char *rawbytes)
{
    cJSON *json,*compobj; char *retstr,*signparams,*txid = 0; int32_t locktime = 0;
    rp->txid[0] = deststr[0] = 0;
    rp->completed = -1;
    //printf("cp.%d vs %d: subatomic_signtx rawbytes.(%s)\n",cp->coinid,coinid,rawbytes);
    if ( coin != 0 && (signparams= subatomic_signraw_json_params(skipaddr,coinaddr,coin,rp,rawbytes)) != 0 )
    {
        _stripwhite(signparams,' ');
        // printf("got signparams.(%s)\n",signparams);
        if ( (retstr= bitcoind_RPC(0,coin->name,coin->serverport,coin->userpass,"signrawtransaction",signparams)) != 0 )
        {
            //printf("got retstr.(%s)\n",retstr);
            if ( (json= cJSON_Parse(retstr)) != 0 )
            {
                if ( (deststr= jstr(json,"hex")) != 0 )
                {
                    compobj = cJSON_GetObjectItem(json,"complete");
                    if ( compobj != 0 )
                        rp->completed = ((compobj->type&0xff) == cJSON_True);
                    if ( strlen(deststr) > destsize )
                        printf("sign_rawtransaction: strlen(deststr) %ld > %ld destize\n",strlen(deststr),destsize);
                    else
                    {
                        txid = subatomic_decodetxid(valuep,0,&locktime,coin,deststr,coinaddr);
                        if ( txid != 0 )
                        {
                            safecopy(rp->txid,txid,sizeof(rp->txid));
                            free(txid);
                            txid = rp->txid;
                        }
                        // printf("got signedtransaction -> txid.(%s) %.8f\n",rp->txid,dstr(valuep!=0?*valuep:0));
                    }
                } else printf("cant get hex from.(%s)\n",retstr);
                free_json(json);
            } else printf("json parse error.(%s)\n",retstr);
            free(retstr);
        } else printf("error signing rawtx\n");
        free(signparams);
    } else printf("error generating signparams\n");
    if ( lockedblockp != 0 )
        *lockedblockp = locktime;
    return(txid);
}

cJSON *subatomic_vouts_json_params(struct subatomic_rawtransaction *rp)
{
    int32_t i; cJSON *json,*obj;
    json = cJSON_CreateObject();
    for (i=0; i<rp->numoutputs; i++)
    {
        obj = cJSON_CreateNumber((double)rp->destamounts[i]/SATOSHIDEN);
        cJSON_AddItemToObject(json,rp->destaddrs[i],obj);
    }
    // printf("numdests.%d (%s)\n",rp->numoutputs,cJSON_Print(json));
    return(json);
}

char *subatomic_rawtxid_json(struct coin777 *coin,struct subatomic_rawtransaction *rp)
{
    char *paramstr = 0; cJSON *array,*vinsobj,*voutsobj;
    if ( (vinsobj= subatomic_vins_json_params(coin,rp)) != 0 )
    {
        if ( (voutsobj= subatomic_vouts_json_params(rp)) != 0 )
        {
            array = cJSON_CreateArray();
            cJSON_AddItemToArray(array,vinsobj);
            cJSON_AddItemToArray(array,voutsobj);
            paramstr = cJSON_Print(array);
            free_json(array);   // this frees both vinsobj and voutsobj
        }
        else free_json(vinsobj);
    }
    // printf("subatomic_rawtxid_json.%s\n",paramstr);
    return(paramstr);
}

// if signcoindaddr is non-zero then signtx and return txid, otherwise just return rawtransaction bytes (NOT allocated)
char *subatomic_gen_rawtransaction(char *skipaddr,struct coin777 *coin,struct subatomic_rawtransaction *rp,char *signcoinaddr,uint32_t locktime,uint32_t vin0sequenceid)
{
    char *rawparams,*retstr=0; int64_t value; long len; struct cointx_info *cointx;
    if ( (rawparams = subatomic_rawtxid_json(coin,rp)) != 0 )
    {
        _stripwhite(rawparams,' ');
        //printf("create.(%s)\n",rawparams);
        if ( (retstr= bitcoind_RPC(0,coin->name,coin->serverport,coin->userpass,"createrawtransaction",rawparams)) != 0 )
        {
            if ( retstr[0] != 0 )
            {
                // printf("calc_rawtransaction retstr.(%s)\n",retstr);
                safecopy(rp->rawtransaction,retstr,sizeof(rp->rawtransaction));
                free(retstr);
                len = strlen(rp->rawtransaction);
                if ( len < 8 )
                {
                    printf("funny rawtransactionlen %ld??\n",len);
                    free(rawparams);
                    return(0);
                }
                if ( locktime != 0 )
                {
                    if ( (cointx= _decode_rawtransaction(rp->rawtransaction,coin->mgw.oldtx_format)) != 0 )
                    {
                        //printf("%s\n->\n",rp->rawtransaction);
                        cointx->nlocktime = locktime;
                        cointx->inputs[0].sequence = vin0sequenceid;
                        _emit_cointx(rp->rawtransaction,sizeof(rp->rawtransaction),cointx,coin->mgw.oldtx_format);
                        _validate_decoderawtransaction(rp->rawtransaction,cointx,coin->mgw.oldtx_format);
                        //printf("spliced tx.(%s)\n",rp->rawtransaction);
                        free(cointx);
                    }
                    printf("locktime.%d sequenceid.%d signcoinaddr.(%s)\n",locktime,vin0sequenceid,signcoinaddr!=0?signcoinaddr:"");
                }
                if ( signcoinaddr != 0 )
                    retstr = subatomic_signtx(skipaddr,0,&value,signcoinaddr,rp->signedtransaction,sizeof(rp->signedtransaction),coin,rp,rp->rawtransaction);
                else retstr = rp->rawtransaction;
            } else { free(retstr); retstr = 0; };
        } else printf("error creating rawtransaction from.(%s)\n",rawparams);
        free(rawparams);
    } else printf("error creating rawparams\n");
    return(retstr);
}

uint64_t subatomic_donation(struct coin777 *coin,uint64_t amount)
{
    uint64_t donation = 0;
    if ( coin->donationaddress[0] != 0 )
    {
        donation = amount >> 11;
        if ( donation < coin->mgw.txfee )
            donation = coin->mgw.txfee;
    }
    return(donation);
}

char *subatomic_create_fundingtx(struct subatomic_halftx *htx,int64_t amount,int32_t lockgap,char *changeaddr)
{
    //2) Create and sign but do not broadcast a transaction (T1) that sets up a payment of amount to funding acct
    struct subatomic_unspent_tx *ups; struct coin777 *coin; char *txid,*retstr = 0; uint64_t total,donation; int32_t num,check_locktime,locktime;
    if ( (coin= coin777_find(htx->coinstr,0)) == 0 )
    {
        printf("subatomic_create_fundingtx: cant find (%s)\n",htx->coinstr);
        return(0);
    }
    if ( (locktime= lockgap) != 0 )
        locktime = (uint32_t)(time(NULL) + lockgap);
    donation = subatomic_donation(coin,amount);
    printf("CREATE FUNDING TX.(%s) for %.8f -> %s locktime.%u donation %.8f\n",coin->name,dstr(amount),htx->coinaddr,locktime,dstr(donation));
    memset(&htx->funding,0,sizeof(htx->funding));
    if ( (ups= gather_unspents(&total,&num,coin,0)) && num != 0 )
    {
        if ( subatomic_calc_rawinputs(coin,&htx->funding,amount,ups,num,donation) >= amount )
        {
            htx->avail = amount;
            if ( subatomic_calc_rawoutputs(htx,coin,&htx->funding,1.,htx->multisigaddr.buf,0,changeaddr,donation) > 0 )
            {
                if ( (retstr= subatomic_gen_rawtransaction(htx->multisigaddr.buf,coin,&htx->funding,htx->coinaddr,locktime,locktime==0?0xffffffff:(uint32_t)time(NULL))) != 0 )
                {
                    txid = subatomic_decodetxid(0,&htx->funding_scriptPubKey,&check_locktime,coin,htx->funding.rawtransaction,htx->multisigaddr.buf);
                    //printf("txid.%s fundingtx %.8f -> %.8f %s completed.%d locktimes %d vs %d\n",txid,dstr(amount),dstr(htx->funding.amount),retstr,htx->funding.completed,check_locktime,locktime);
                    //printf("(%s)\nfunding.(%s)\n",htx->funding.rawtransaction,htx->funding.signedtransaction);
                }
            }
        }
    }
    if ( ups != 0 )
        free(ups);
    return(retstr);
}

void subatomic_set_unspent_tx0(struct subatomic_unspent_tx *up,struct subatomic_halftx *htx)
{
    memset(up,0,sizeof(*up));
    up->vout = 0;
    up->amount = htx->avail;
    safecopy(up->txid.buf,htx->fundingtxid,sizeof(up->txid));
    safecopy(up->address.buf,htx->multisigaddr.buf,sizeof(up->address));
    safecopy(up->scriptPubKey.buf,htx->funding_scriptPubKey.buf,sizeof(up->scriptPubKey));
    safecopy(up->redeemScript.buf,htx->redeemScript.buf,sizeof(up->redeemScript));
}

char *subatomic_create_paytx(struct subatomic_rawtransaction *rp,char *signcoinaddr,struct subatomic_halftx *htx,char *othercoinaddr,int32_t locktime,double myshare,int32_t seqid)
{
    struct subatomic_unspent_tx U; int32_t check_locktime; int64_t value,donation; char *txid = 0; struct coin777 *coin = coin777_find(htx->coinstr,0);
    if ( coin == 0 )
        return(0);
    //struct subatomic_unspent_tx *gather_unspents(int32_t *nump,struct coin777 *coin,char *coinaddr)
    printf("create paytx %s\n",coin->name);
    subatomic_set_unspent_tx0(&U,htx);
    donation = subatomic_donation(coin,htx->avail);
    rp->numinputs = 0;
    rp->inputs[rp->numinputs++] = U;
    rp->amount = (htx->avail - coin->mgw.txfee - donation);
    rp->change = 0;
    rp->inputsum = htx->avail;
    // jl777: make sure sequence number is not -1!!
    if ( subatomic_calc_rawoutputs(htx,coin,rp,myshare,htx->coinaddr,othercoinaddr,0,donation) > 0 )
    {
        subatomic_gen_rawtransaction(htx->multisigaddr.buf,coin,rp,signcoinaddr,locktime,seqid);
        txid = subatomic_decodetxid(&value,0,&check_locktime,coin,rp->rawtransaction,htx->coinaddr);
        if ( check_locktime != locktime )
        {
            printf("check_locktime.%d vs locktime.%d\n",check_locktime,locktime);
            return(0);
        }
        printf("created paytx %.8f to %s value %.8f, locktime.%d\n",dstr(value),htx->coinaddr,dstr(value),locktime);
    }
    return(txid);
}

int32_t subatomic_gen_pubkeys(struct subatomic_tx *atx,struct subatomic_halftx *htx)
{
    char coinaddrs[3][128],pubkeys[3][256],*coinstr; int32_t i,flag=0; struct destbuf pubkey; char *coinaddr,*pubkeystr; struct coin777 *coin;
    if ( htx->multisigaddr.buf[0] == 0 )
    {
        memset(coinaddrs,0,sizeof(coinaddrs));
        memset(pubkeys,0,sizeof(pubkeys));
        for (i=0; i<2; i++)
        {
            if ( i == 0 )
            {
                pubkeystr = atx->myhalf.pubkey;
                coinaddr = atx->myhalf.coinaddr;
                coinstr = atx->ARGS.coinstr;
            }
            else
            {
                pubkeystr = atx->myhalf.destpubkey;
                coinaddr = atx->myhalf.destcoinaddr;
                coinstr = atx->ARGS.destcoinstr;
            }
            if ( pubkeystr[0] == 0 && (coin= coin777_find(coinstr,0)) != 0 )
            {
                get_pubkey(&pubkey,coin->name,coin->serverport,coin->userpass,coinaddr);
                pubkeystr = pubkey.buf;
            }
            if ( pubkeystr[0] != 0 )
            {
                flag++;
                safecopy(atx->ARGS.mypubkeys[i],pubkey.buf,sizeof(atx->ARGS.mypubkeys[i]));
                printf("i.%d gen pubkey %s (%s) for (%s)\n",i,coinstr,pubkey.buf,coinaddr);
            }
            else
            {
                printf("i.%d cant generate %s pubkey for addr.%s\n",i,coinstr,coinaddr);
                //return(-1);
            }
        }
    }
    return(flag);
}

// Alice sends a future transaction to Bob's address
// Bob sends a phased tx to Alice that triggers off of his pubkey
// When Bob spends the tx, the pubkey triggers his phased tx to bob.
#define SUBATOMIC_SEND_PUBKEY 'P'
#define SUBATOMIC_REFUNDTX_NEEDSIG 'R'
#define SUBATOMIC_REFUNDTX_SIGNED 'S'
#define SUBATOMIC_FUNDINGTX 'F'
#define SUBATOMIC_SEND_MICROTX 'T'
#define SUBATOMIC_SEND_ATOMICTX 'A'

int32_t share_tx(struct subatomic_rawtransaction *rp,int32_t funcid)
{
    /*uint32_t TCRC;
    int32_t incr,size;
    char i,n,jsonstr[512];
    incr = (int32_t)(SYNC_FRAGSIZE - sizeof(struct json_AM) - 60);
    size = (sizeof(*rp) - sizeof(rp->inputs) + sizeof(rp->inputs[0])*rp->numinputs);
    n = size / incr;
    if ( (size / incr) != 0 )
        n++;
    TCRC = _crc32(0,rp,sizeof(struct subatomic_rawtransaction));
    for (i=0; i<n; i++)
    {
        sprintf(jsonstr,"{\"TCRC\":%u,\"starti\":%d,\"i\":%d,\"n\":%d,\"incr\":%d}",TCRC,startfragi,i,n,incr);
        send_to_NXTaddr(&np->localcrcs[startfragi+i],np->H.NXTaddr,startfragi+i,SUBATOMIC_SIG,funcid,jsonstr,(void *)((long)rp+i*incr),incr);
    }*/
    return(0);
}

int32_t share_refundtx(struct subatomic_rawtransaction *rp)
{
    return(share_tx(rp,SUBATOMIC_REFUNDTX_NEEDSIG));
}

int32_t share_other_refundtx(struct subatomic_rawtransaction *rp)
{
    return(share_tx(rp,SUBATOMIC_REFUNDTX_SIGNED));
}

int32_t share_fundingtx(struct subatomic_rawtransaction *rp)
{
    return(share_tx(rp,SUBATOMIC_FUNDINGTX));
}

int32_t share_micropaytx(struct subatomic_rawtransaction *rp)
{
    return(share_tx(rp,SUBATOMIC_SEND_MICROTX));
}

int32_t subatomic_ensure_txs(struct subatomic_halftx *otherhalf,struct subatomic_halftx *htx,int32_t locktime)
{
    char *fundingtxid,*refundtxid,*micropaytxid; struct coin777 *coin; int32_t blocknum = 0;
    if ( (coin= coin777_find(htx->coinstr,0)) == 0 || htx->multisigaddr.buf[0] == 0 )
    {
        printf("cant get valid daemon for %s or no xferaddr.%s\n",coin->name,htx->multisigaddr.buf);
        return(-1);
    }
    if ( locktime != 0 )
    {
        coin->ramchain.RTblocknum = _get_RTheight(&coin->ramchain.lastgetinfo,coin->name,coin->serverport,coin->userpass,coin->ramchain.RTblocknum);
        if ( (blocknum= coin->ramchain.RTblocknum) == 0 )
        {
            printf("cant get valid blocknum for %s\n",coin->name);
            return(-1);
        }
        blocknum += (locktime/coin->estblocktime) + 1;
    }
    if ( htx->fundingtxid == 0 )
    {
        //printf("create funding TX\n");
        if ( (fundingtxid= subatomic_create_fundingtx(htx,htx->amount,SUBATOMIC_LOCKTIME,htx->coinaddr)) == 0 )
            return(-1);
        safecopy(htx->fundingtxid,fundingtxid,sizeof(htx->fundingtxid));
        free(fundingtxid);
        htx->avail = htx->myamount;
    }
    if ( htx->refundtxid == 0 )
    {
        // printf("create refund TX\n");
        if ( (refundtxid= subatomic_create_paytx(&htx->refund,0,htx,otherhalf->coinaddr,blocknum,1.,SUBATOMIC_STARTING_SEQUENCEID-1)) == 0 )
            return(-1);
        safecopy(htx->refundtxid,refundtxid,sizeof(htx->refundtxid));
        free(refundtxid);
        //printf("created refundtx.(%s)\n",htx->refundtxid);
    }
    if ( htx->micropaytxid == 0 )
    {
        //printf("create micropay TX\n");
        if ( (micropaytxid= subatomic_create_paytx(&htx->micropay,htx->coinaddr,htx,otherhalf->coinaddr,0,1.,SUBATOMIC_STARTING_SEQUENCEID)) == 0 )
            return(-1);
        safecopy(htx->micropaytxid,micropaytxid,sizeof(htx->micropaytxid));
        free(micropaytxid);
    }
    return(blocknum);
}

int32_t verify_txs_created(struct subatomic_tx *atx)
{
    struct subatomic_halftx *htx = &atx->myhalf;
    if ( atx->other_refundtx_done == 0 )
        printf("multisig addrs %d %d %d %d | refundtx.%d multisigaddr.%s\n",atx->myhalf.coinaddr[0],atx->myhalf.pubkey[0],atx->otherhalf.coinaddr[0],atx->otherhalf.pubkey[0],atx->other_refundtx_done,atx->myhalf.multisigaddr.buf);
    //if ( atx->other_refundtx_done == 0 )
    //    atx->myrefund_fragi = share_pubkey(np,1,htx->destcoinid,htx->destcoinaddr,htx->destpubkey);
    if ( atx->myhalf.multisigaddr.buf[0] == 0 )
    {
        if ( atx->otherhalf.pubkey[0] != 0 )
        {
            printf(">>>> multisig addrs %d %d %d %d\n",htx->coinaddr[0],htx->pubkey[0],atx->otherhalf.coinaddr[0],atx->otherhalf.pubkey[0]);
            subatomic_gen_multisig(atx,htx);
            if ( atx->myhalf.multisigaddr.buf[0] != 0 )
                printf("generated multisig.(%s)\n",atx->myhalf.multisigaddr.buf);
        }
    }
    if ( atx->myhalf.multisigaddr.buf[0] == 0 )
        return(0);
    if ( atx->txs_created == 0 && subatomic_ensure_txs(&atx->otherhalf,htx,(atx->longerflag * SUBATOMIC_LOCKTIME)) < 0 )
    {
        printf("warning: cant create required transactions, probably lack of funds\n");
        return(-1);
    }
    //if ( atx->other_refund_fragi == 0 )
    //    atx->other_refund_fragi = share_refundtx(np,&htx->refund,atx->myrefund_fragi);
    return(1);
}

int32_t update_other_refundtxdone(struct subatomic_tx *atx)
{
    int32_t lockedblock; int64_t value; struct coin777 *coin; struct subatomic_rawtransaction *rp; struct subatomic_halftx *htx = &atx->myhalf;
    if ( (coin= coin777_find(htx->coinstr,0)) != 0 )
    {
        rp = &atx->otherhalf.refund;
        if ( atx->other_refundtx_done == 0 )
        {
            if ( atx->other_refundtx_waiting != 0 )
            {
                if ( subatomic_signtx(0,&lockedblock,&value,htx->destcoinaddr,rp->signedtransaction,sizeof(rp->signedtransaction),coin777_find(htx->destcoinstr,0),rp,rp->rawtransaction) == 0 )
                {
                    printf("warning: error signing other's NXT.%s refund\n",htx->otherNXTaddr);
                    return(0);
                }
                //atx->funding_fragi = share_other_refundtx(np,rp,atx->other_refund_fragi);
                atx->other_refundtx_done = 1;
                printf("other refundtx done\n");
            }
        }
    }
    return(atx->other_refundtx_done);
}

int32_t subatomic_validate_refund(struct subatomic_tx *atx,struct subatomic_halftx *htx)
{
    struct coin777 *coin = coin777_find(htx->coinstr,0); int64_t value; int32_t lockedblock;
    if ( coin == 0 )
        return(-1);
    printf("validate refund\n");
    if ( subatomic_signtx(atx->myhalf.multisigaddr.buf,&lockedblock,&value,htx->coinaddr,htx->countersignedrefund,sizeof(htx->countersignedrefund),coin,&htx->refund,htx->refund.signedtransaction) == 0 )
    {
        printf("error signing refund\n");
        return(-1);
    }
    printf("refund signing completed.%d\n",htx->refund.completed);
    if ( htx->refund.completed <= 0 )
        return(-1);
    printf(">>>>>>>>>>>>>>>>>>>>> refund at %d is locked! txid.%s completed %d %.8f -> %s\n",lockedblock,htx->refund.txid,htx->refund.completed,dstr(value),htx->coinaddr);
    atx->status = SUBATOMIC_HAVEREFUND;
    return(0);
}

int32_t update_my_refundtxdone(struct subatomic_tx *atx)
{
    struct subatomic_halftx *htx = &atx->myhalf;
    if ( atx->myrefundtx_done == 0 )
    {
        if ( atx->myrefundtx_waiting != 0 )
        {
            if ( subatomic_validate_refund(atx,htx) < 0 )
            {
                printf("warning: other side NXT.%s returned invalid signed refund\n",htx->otherNXTaddr);
                return(0);
            }
            subatomic_broadcasttx(&atx->myhalf,atx->myhalf.countersignedrefund,0,atx->refundlockblock);
            atx->myrefundtx_done = 1;
            printf("myrefund done\n");
        }
    }
    return(atx->myrefundtx_done);
}

int32_t update_fundingtx(struct subatomic_tx *atx)
{
    //struct subatomic_halftx *htx = &atx->myhalf;
    /*if ( atx->type == SUBATOMIC_TYPE || atx->type == SUBATOMIC_FORNXT_TYPE )
    {
        if ( atx->microtx_fragi == 0 || atx->other_fundingtx_confirms == 0 )
            atx->microtx_fragi = share_fundingtx(np,&htx->funding,atx->funding_fragi);
    }
    if ( atx->type == NXTFOR_SUBATOMIC_TYPE || atx->type == SUBATOMIC_TYPE )*/
    {
        if ( atx->other_fundingtx_confirms == 0 )
        {
            if ( atx->other_fundingtx_waiting != 0 )
            {
                subatomic_broadcasttx(&atx->otherhalf,atx->otherhalf.funding.signedtransaction,0,0);
                atx->other_fundingtx_confirms = atx->otherhalf.minconfirms+1;
                printf("broadcast other funding\n");
            }
        }
    }
    //else atx->other_fundingtx_confirms = get_numconfirms(&atx->otherhalf);  // jl777: critical to wait for both funding tx to get confirmed
    return(atx->other_fundingtx_confirms);
}

double subatomic_calc_incr(struct subatomic_halftx *htx,int64_t value,int64_t den,int32_t numincr)
{
    //printf("value %.8f/%.8f numincr.%d -> %.6f\n",dstr(value),dstr(den),numincr,(((double)value/den) * numincr));
    return((((double)value/den) * numincr));
}

int32_t subatomic_validate_micropay(struct subatomic_tx *atx,char *skipaddr,char *destbytes,int32_t max,int64_t *valuep,struct subatomic_rawtransaction *rp,struct subatomic_halftx *htx,struct coin777 *srccoin,int64_t srcamount,int32_t numincr,char *refcoinaddr)
{
    int64_t value; int32_t lockedblock; struct coin777 *coin = coin777_find(htx->coinstr,0);
    if ( valuep != 0 )
        *valuep = 0;
    if ( coin == 0 )
        return(-1);
    if ( subatomic_signtx(skipaddr,&lockedblock,&value,refcoinaddr,destbytes,max,coin,rp,rp->signedtransaction) == 0 )
        return(-1);
    if ( valuep != 0 )
        *valuep = value;
    if ( rp->completed <= 0 )
        return(-1);
    //printf("micropay is updated txid.%s completed %d %.8f -> %s, lockedblock.%d\n",rp->txid,rp->completed,dstr(value),htx->coinaddr,lockedblock);
    return(lockedblock);
}

int32_t process_microtx(struct subatomic_tx *atx,struct subatomic_rawtransaction *rp,int32_t incr,int32_t otherincr)
{
    int64_t value;
    if ( subatomic_validate_micropay(atx,0,atx->otherhalf.completedmicropay,(int32_t)sizeof(atx->otherhalf.completedmicropay),&value,rp,&atx->otherhalf,coin777_find(atx->ARGS.destcoinstr,0),atx->ARGS.destamount,atx->ARGS.numincr,atx->myhalf.destcoinaddr) < 0 )
    {
        printf("Error validating micropay from NXT.%s %s %s\n",atx->ARGS.otherNXTaddr,atx->myhalf.destcoinstr,atx->myhalf.destcoinaddr);
        //subatomic_sendabort(atx);
        //atx->status = SUBATOMIC_ABORTED;
        //if ( incr > 1 )
        //    atx->claimtxid = subatomic_broadcasttx(&atx->otherhalf,atx->otherhalf.completedmicropay,0,0);
        return(-1);
    }
    else
    {
        atx->myreceived = value;
        otherincr = subatomic_calc_incr(&atx->otherhalf,value,atx->myexpectedamount,atx->ARGS.numincr);
    }
    if ( otherincr == atx->ARGS.numincr || value == atx->myhalf.destamount )
    {
        printf("TX complete!\n");
        atx->claimtxid = subatomic_broadcasttx(&atx->otherhalf,atx->otherhalf.completedmicropay,0,0);
        atx->status = SUBATOMIC_COMPLETED;
    }
    printf("[%5.2f%%] Received %12.8f of %12.8f | Sent %12.8f of %12.8f\n",100.*(double)atx->myreceived/atx->myexpectedamount,dstr(atx->myreceived),dstr(atx->myexpectedamount),dstr(atx->sent_to_other),dstr(atx->otherexpectedamount));
    
    //printf("incr.%d of %d, otherincr.%d %.8f %.8f \n",incr,atx->ARGS.numincr,otherincr,dstr(value),dstr(atx->myexpectedamount));
    return(otherincr);
}

int64_t subatomic_calc_micropay(struct subatomic_tx *atx,struct subatomic_halftx *htx,char *othercoinaddr,double myshare,int32_t seqid)
{
    struct subatomic_unspent_tx U; uint64_t donation; struct coin777 *coin = coin777_find(htx->coinstr,0); struct subatomic_rawtransaction *rp = &htx->micropay;
    if ( coin == 0 )
        return(0);
    subatomic_set_unspent_tx0(&U,htx);
    donation = subatomic_donation(coin,htx->avail);
    rp->numinputs = 0;
    rp->inputs[rp->numinputs++] = U;
    rp->amount = (htx->avail - coin->mgw.txfee - donation);
    rp->change = 0;
    rp->inputsum = htx->avail;
    //printf("subatomic_sendincr myshare %f seqid.%d\n",myshare,seqid);
    if ( subatomic_calc_rawoutputs(htx,coin,rp,myshare,htx->coinaddr,othercoinaddr,coin->donationaddress,donation) > 0 )
    {
        subatomic_gen_rawtransaction(htx->multisigaddr.buf,coin,rp,htx->coinaddr,0,seqid);
        return(htx->otheramount);
    }
    return(-1);
}

int32_t calc_micropay(struct subatomic_tx *atx)
{
    if ( (atx->sent_to_other= subatomic_calc_micropay(atx,&atx->myhalf,atx->otherhalf.coinaddr,atx->ARGS.myshare,SUBATOMIC_STARTING_SEQUENCEID+atx->ARGS.incr)) > 0 )
    {
        // printf("micropay.(%s)\n",htx->micropay.signedtransaction);
        //printf("send micropay share incr.%d otherincr.%d totalfragis.%d\n",atx->ARGS.incr,atx->ARGS.otherincr,totalfragis);
        return(0);
    }
    return(-1);
}

int32_t send_micropay(struct subatomic_tx *atx)
{
    share_micropaytx(&atx->myhalf.micropay);
    return(0);
}

int32_t update_otherincr(struct subatomic_tx *atx)
{
    return(process_microtx(atx,&atx->otherhalf.micropay,atx->ARGS.incr,atx->ARGS.otherincr));
}

void init_subatomic_halftx(struct subatomic_halftx *htx,struct subatomic_tx *atx)
{
    safecopy(htx->NXTaddr,atx->ARGS.NXTaddr,sizeof(htx->NXTaddr));
    safecopy(htx->otherNXTaddr,atx->ARGS.otherNXTaddr,sizeof(htx->otherNXTaddr));
    safecopy(htx->ipaddr,SUPERNET.myipaddr,sizeof(htx->ipaddr));
    if ( atx->ARGS.otheripaddr[0] != 0 )
        safecopy(htx->otheripaddr,atx->ARGS.otheripaddr,sizeof(htx->otheripaddr));
}

int32_t init_subatomic_tx(struct subatomic_tx *atx,int32_t flipped)
{
    struct coin777 *coin,*destcoin;
    if ( (coin= coin777_find(atx->ARGS.coinstr,1)) == 0 || (destcoin= coin777_find(atx->ARGS.destcoinstr,1)) == 0 )
    {
        printf("coin.(%s) or (%s) not found\n",atx->ARGS.coinstr,atx->ARGS.destcoinstr);
        return(-1);
    }
    if ( atx->ARGS.coinaddr[flipped][0] != 0 && atx->ARGS.destcoinaddr[flipped][0] != 0 ) // atx->ARGS.otherNXTaddr[0] != 0 &&
    {
        if ( atx->ARGS.amount != 0 && atx->ARGS.destamount != 0 && strcmp(atx->ARGS.coinstr,atx->ARGS.destcoinstr) != 0 )
        {
            if ( atx->longerflag == 0 )
            {
                atx->myhalf.minconfirms = coin->minconfirms;
                atx->otherhalf.minconfirms = destcoin->minconfirms;
                atx->ARGS.numincr = SUBATOMIC_DEFAULTINCR;
                atx->longerflag = 1;
                if ( (calc_nxt64bits(atx->ARGS.NXTaddr) % 666) > (calc_nxt64bits(atx->ARGS.otherNXTaddr) % 666) )
                    atx->longerflag = 2;
            }
            init_subatomic_halftx(&atx->myhalf,atx);
            init_subatomic_halftx(&atx->otherhalf,atx);
            atx->connsock = -1;
            if ( flipped == 0 )
            {
                strcpy(atx->myhalf.coinstr,atx->ARGS.coinstr); strcpy(atx->myhalf.destcoinstr,atx->ARGS.destcoinstr);
                atx->myhalf.amount = atx->ARGS.amount; atx->myhalf.destamount = atx->ARGS.destamount;
                safecopy(atx->myhalf.coinaddr,atx->ARGS.coinaddr[0],sizeof(atx->myhalf.coinaddr));
                safecopy(atx->myhalf.destcoinaddr,atx->ARGS.destcoinaddr[0],sizeof(atx->myhalf.destcoinaddr));
                //atx->myhalf.donation = atx->myhalf.amount * SUBATOMIC_DONATIONRATE;
                //if ( atx->myhalf.donation < coin->mgw.txfee )
                //    atx->myhalf.donation = coin->mgw.txfee;
                atx->otherexpectedamount = atx->myhalf.amount;// - 2*coin->mgw.txfee - 2*atx->myhalf.donation;
                subatomic_gen_pubkeys(atx,&atx->myhalf);
            }
            else
            {
                strcpy(atx->otherhalf.coinstr,atx->ARGS.destcoinstr); strcpy(atx->otherhalf.destcoinstr,atx->ARGS.coinstr);
                atx->otherhalf.amount = atx->ARGS.destamount; atx->otherhalf.destamount = atx->ARGS.amount;
                safecopy(atx->otherhalf.coinaddr,atx->ARGS.destcoinaddr[1],sizeof(atx->otherhalf.coinaddr));
                safecopy(atx->otherhalf.destcoinaddr,atx->ARGS.coinaddr[1],sizeof(atx->otherhalf.destcoinaddr));
                //atx->otherhalf.donation = atx->otherhalf.amount * SUBATOMIC_DONATIONRATE;
                //if ( atx->otherhalf.donation < destcoin->mgw.txfee )
                //    atx->otherhalf.donation = destcoin->mgw.txfee;
                atx->myexpectedamount = atx->otherhalf.amount;// - 2*destcoin->mgw.txfee - 2*atx->otherhalf.donation;
            }
            printf("%p.(%s %s %.8f -> %.8f %s <-> %s %s %.8f <- %.8f %s) myhalf.(%s %s) %.8f <-> %.8f other.(%s %s) IP.(%s)\n",atx,atx->ARGS.NXTaddr,atx->myhalf.coinstr,dstr(atx->myhalf.amount),dstr(atx->myhalf.destamount),atx->myhalf.destcoinstr,atx->ARGS.otherNXTaddr,atx->otherhalf.coinstr,dstr(atx->otherhalf.amount),dstr(atx->otherhalf.destamount),atx->otherhalf.destcoinstr,atx->myhalf.coinaddr,atx->myhalf.destcoinaddr,dstr(atx->myexpectedamount),dstr(atx->otherexpectedamount),atx->otherhalf.coinaddr,atx->otherhalf.destcoinaddr,atx->ARGS.otheripaddr);
            return(1 << flipped);
        }
    }
    return(0);
}

int32_t subatomic_txcmp(struct subatomic_tx *_ref,struct subatomic_tx *_atx,int32_t flipped)
{
    struct subatomic_tx_args *ref,*atx;
    ref = &_ref->ARGS; atx = &_atx->ARGS;
    printf("%p.(%s <-> %s) vs %p.(%s <-> %s)\n",ref,ref->NXTaddr,ref->otherNXTaddr,atx,atx->NXTaddr,atx->otherNXTaddr);
    if ( flipped != 0 )
    {
        if ( strcmp(ref->NXTaddr,atx->otherNXTaddr) != 0 )
        {
            printf("%s != %s\n",ref->NXTaddr,atx->otherNXTaddr);
            return(-1);
        }
        if ( strcmp(ref->otherNXTaddr,atx->NXTaddr) != 0 )
            return(-2);
    }
    else
    {
        if ( strcmp(ref->NXTaddr,atx->NXTaddr) != 0 )
        {
            printf("%s != %s\n",ref->NXTaddr,atx->NXTaddr);
            return(-1);
        }
        if ( strcmp(ref->otherNXTaddr,atx->otherNXTaddr) != 0 )
            return(-2);
    }
    if ( flipped == 0 )
    {
        if ( strcmp(ref->coinstr,atx->coinstr) != 0 )
        {
            printf("%s != %s\n",ref->coinstr,atx->coinstr);
            return(-3);
        }
        if ( strcmp(ref->destcoinstr,atx->destcoinstr) != 0 )
            return(-4);
        if ( ref->destamount != atx->destamount )
            return(-5);
        if ( ref->amount != atx->amount )
            return(-6);
    }
    else
    {
        if ( strcmp(ref->coinstr,atx->destcoinstr) != 0 )
        {
            printf("%s != %s\n",ref->coinstr,atx->destcoinstr);
            return(-13);
        }
        if ( strcmp(ref->destcoinstr,atx->coinstr) != 0 )
            return(-14);
        if ( ref->destamount != atx->amount )
            return(-15);
        if ( ref->amount != atx->destamount )
            return(-16);
    }
    return(0);
}

int32_t set_subatomic_trade(struct subatomic_tx *_atx,char *NXTaddr,char *coin,char *amountstr,char *coinaddr,char *otherNXTaddr,char *destcoin,char *destamountstr,char *destcoinaddr,char *otheripaddr,int32_t flipped)
{
    struct subatomic_tx_args *atx = &_atx->ARGS;
    memset(atx,0,sizeof(*atx));
    strcpy(atx->coinstr,coin);
    strcpy(atx->destcoinstr,destcoin);
    atx->amount = conv_floatstr(amountstr);
    atx->destamount = conv_floatstr(destamountstr);
    safecopy(atx->coinaddr[flipped],coinaddr,sizeof(atx->coinaddr[flipped]));
    safecopy(atx->coinaddr[flipped^1],destcoinaddr,sizeof(atx->coinaddr[flipped^1]));
    safecopy(atx->NXTaddr,NXTaddr,sizeof(atx->NXTaddr));
    safecopy(atx->otherNXTaddr,otherNXTaddr,sizeof(atx->otherNXTaddr));
    safecopy(atx->destcoinaddr[flipped],destcoinaddr,sizeof(atx->destcoinaddr[flipped]));
    safecopy(atx->destcoinaddr[flipped^1],coinaddr,sizeof(atx->destcoinaddr[flipped^1]));
    if ( flipped != 0 )
        safecopy(atx->otheripaddr,otheripaddr,sizeof(atx->otheripaddr));
    printf("flipped.%d ipaddr.%s %s %s\n",flipped,atx->otheripaddr,coin,destcoin);
    if ( atx->amount != 0 && atx->destamount != 0 )//&& atx->coinid >= 0 && atx->destcoinid >= 0 )
        return(0);
    printf("error setting subatomic trade %lld %lld %s %s\n",(long long)atx->amount,(long long)atx->destamount,atx->coinstr,atx->destcoinstr);
    return(-1);
}

// this function needs to invert everything to the point of view of this acct
int32_t decode_subatomic_json(struct subatomic_tx *atx,cJSON *json,char *sender,char *receiver)
{
    struct destbuf amountstr,destamountstr,NXTaddr,coin,destcoin,coinaddr,destcoinaddr,otherNXTaddr,otheripaddr;
    int32_t flipped = 0;
    copy_cJSON(&NXTaddr,jobj(json,"NXT"));
    copy_cJSON(&coin,jobj(json,"coin"));
    copy_cJSON(&amountstr,jobj(json,"amount"));
    copy_cJSON(&coinaddr,jobj(json,"coinaddr"));
    copy_cJSON(&otherNXTaddr,jobj(json,"destNXT"));
    copy_cJSON(&destcoin,jobj(json,"destcoin"));
    copy_cJSON(&destamountstr,jobj(json,"destamount"));
    copy_cJSON(&destcoinaddr,jobj(json,"destcoinaddr"));
    copy_cJSON(&otheripaddr,jobj(json,"senderip"));
    if ( strcmp(otherNXTaddr.buf,SUPERNET.NXTADDR) == 0 )
        flipped = 1;
    if ( set_subatomic_trade(atx,NXTaddr.buf,coin.buf,amountstr.buf,coinaddr.buf,otherNXTaddr.buf,destcoin.buf,destamountstr.buf,destcoinaddr.buf,otheripaddr.buf,flipped) == 0 )
        return(flipped);
    return(-1);
}

static struct subatomic_tx **Subatomics; static int32_t Numsubatomics;
struct subatomic_tx *update_subatomic_state(cJSON *argjson,uint64_t nxt64bits,char *sender,char *receiver)  // the only path into the subatomics[], eg. via AM
{
    int32_t i,flipped,cmpval; struct subatomic_tx *atx = 0,T;
    memset(&T,0,sizeof(T));
    //printf("parse subatomic\n");
    if ( (flipped= decode_subatomic_json(&T,argjson,sender,receiver)) >= 0 )
    {
        //printf("NXT.%s (%s %s %s %s) <-> %s (%s %s %s %s)\n",T.NXTaddr,coinid_str(T.coinid),T.coinaddr[0],coinid_str(T.destcoinid),T.destcoinaddr[0],T.otherNXTaddr,coinid_str(T.coinid),T.coinaddr[1],coinid_str(T.destcoinid),T.destcoinaddr[1]);
        atx = 0;
        for (i=0; i<Numsubatomics; i++)
        {
            atx = Subatomics[i];
            if ( (cmpval= subatomic_txcmp(atx,&T,flipped)) == 0 )
            {
                printf("%d: cmpval.%d vs %p\n",i,cmpval,atx);
                if ( T.type == SUBATOMIC_TYPE )
                {
                    strcpy(atx->ARGS.coinaddr[flipped],T.ARGS.coinaddr[flipped]);
                    strcpy(atx->ARGS.destcoinaddr[flipped],T.ARGS.destcoinaddr[flipped]);
                    if ( flipped != 0 )
                        strcpy(atx->ARGS.otheripaddr,T.ARGS.otheripaddr);
                }
                /*else
                 {
                 if ( flipped != 0 )
                 {
                 strcpy(atx->ARGS.otheripaddr,T.swap.otheripaddr);
                 strcpy(atx->ARGS.NXTaddr,T.swap.otherNXTaddr);
                 strcpy(atx->ARGS.otherNXTaddr,T.swap.NXTaddr);
                 }
                 else
                 {
                 strcpy(atx->ARGS.NXTaddr,T.swap.NXTaddr);
                 strcpy(atx->ARGS.otherNXTaddr,T.swap.otherNXTaddr);
                 }
                 }*/
                break;
            }
        }
        if ( i == Numsubatomics )
        {
            atx = malloc(sizeof(T));
            *atx = T;
            if ( T.type == SUBATOMIC_TYPE )
            {
                strcpy(atx->ARGS.coinaddr[flipped],T.ARGS.coinaddr[flipped]);
                strcpy(atx->ARGS.destcoinaddr[flipped],T.ARGS.destcoinaddr[flipped]);
                if ( flipped != 0 )
                {
                    strcpy(atx->ARGS.otheripaddr,T.ARGS.otheripaddr);
                    strcpy(atx->ARGS.NXTaddr,T.ARGS.otherNXTaddr);
                    strcpy(atx->ARGS.otherNXTaddr,T.ARGS.NXTaddr);
                    strcpy(atx->ARGS.coinstr,T.ARGS.destcoinstr);
                    atx->ARGS.amount = T.ARGS.destamount;
                    strcpy(atx->ARGS.destcoinstr,T.ARGS.coinstr);
                    atx->ARGS.destamount = T.ARGS.amount;
                }
                else
                {
                    strcpy(atx->ARGS.coinstr,T.ARGS.coinstr);
                    atx->ARGS.amount = T.ARGS.amount;
                    strcpy(atx->ARGS.destcoinstr,T.ARGS.destcoinstr);
                    atx->ARGS.destamount = T.ARGS.destamount;
                }
            }
            /*else
             {
             if ( flipped != 0 )
             strcpy(atx->ARGS.otheripaddr,T.swap.otheripaddr);
             strcpy(atx->ARGS.NXTaddr,T.swap.NXTaddr);
             strcpy(atx->ARGS.otherNXTaddr,T.swap.otherNXTaddr);
             }*/
            printf("alloc type.%d new %p atx.%d (%s <-> %s)\n",T.type,atx,Numsubatomics,atx->ARGS.NXTaddr,atx->ARGS.otherNXTaddr);
            Subatomics = realloc(Subatomics,(Numsubatomics + 1) * sizeof(*Subatomics));
            Subatomics[Numsubatomics] = atx, atx->tag = Numsubatomics;
            Numsubatomics++;
        }
        atx->initflag |= init_subatomic_tx(atx,flipped);
        printf("got trade! flipped.%d | initflag.%d\n",flipped,atx->initflag);
        if ( atx->initflag == 3 )
        {
            printf("PENDING SUBATOMIC TRADE from %s %s %.8f <-> %s %s %.8f\n",atx->ARGS.NXTaddr,atx->ARGS.coinstr,dstr(atx->ARGS.amount),atx->ARGS.otherNXTaddr,atx->ARGS.destcoinstr,dstr(atx->ARGS.destamount));
        }
    }
    return(atx);
}

void update_subatomic_transfers(char *NXTaddr)
{
    static double nexttime;
    struct subatomic_tx *atx; int32_t i,j,txcreated,retval;
    //printf("update subatomics\n");
    if ( milliseconds() < nexttime )
        return;
    for (i=0; i<Numsubatomics; i++)
    {
        atx = Subatomics[i];
        if ( atx->initflag == 3 && atx->status != SUBATOMIC_COMPLETED && atx->status != SUBATOMIC_ABORTED )
        {
            /*np = get_NXTacct(&createdflag,Global_mp,atx->ARGS.otherNXTaddr);
            if ( (np->recvid == 0 || np->sentid == 0) && verify_peer_link(SUBATOMIC_SIG,atx->ARGS.otherNXTaddr) != 0 )
            {
                nexttime = (microseconds() + 1000000*300);
                continue;
            }
            if ( atx->type == ATOMICSWAP_TYPE )
            {
                atx->status = update_atomic(np,atx);
                continue;
            }*/
            txcreated = verify_txs_created(atx);
            if ( txcreated < 0 )
            {
                atx->status = SUBATOMIC_ABORTED;
                continue;
            }
            if ( (atx->txs_created= txcreated) == 0 )
                continue;
            update_other_refundtxdone(atx);
            update_my_refundtxdone(atx);
            if ( atx->myrefundtx_done <= 0 || atx->other_refundtx_done <= 0 )
                continue;
            update_fundingtx(atx);
            if ( atx->other_fundingtx_confirms-1 < atx->otherhalf.minconfirms )
                continue;
            //printf("micropay loop: share incr.%d otherincr.%d totalfragis.%d\n",atx->ARGS.incr,atx->ARGS.otherincr,totalfragis);
            if ( atx->other_micropaytx_waiting != 0 )
            {
                retval = update_otherincr(atx);//process_microtx(atx,&atx->otherhalf.micropay,atx->ARGS.incr,atx->ARGS.otherincr);
                if ( retval >= 0 )
                    atx->ARGS.otherincr = retval;
                atx->other_micropaytx_waiting = 0;
            }
            if ( atx->ARGS.incr < atx->ARGS.numincr && atx->ARGS.incr <= atx->ARGS.otherincr+1 )
            {
                atx->ARGS.incr++;
                if ( atx->ARGS.incr < atx->ARGS.otherincr )
                    atx->ARGS.incr = atx->ARGS.otherincr;
                atx->ARGS.myshare = ((double)atx->ARGS.numincr - atx->ARGS.incr) / atx->ARGS.numincr;
                calc_micropay(atx);
            }
            if ( atx->ARGS.incr == 100 )
            {
                for (j=0; j<3; j++)
                {
                    send_micropay(atx);
                    sleep(3);
                }
            }
            else send_micropay(atx);
        }
    }
    //printf("done update subatomics\n");
}

/*
 Alice send a pubkey to Bob, who makes a 2of3 multisig with 2 of his pubkeys and sends this address to alice

she then creates an unsigned funding tx for the full amount and sends it to Bob

Bob and Alice sign a postdated refund transaction that returns all the funds to Alice, say 2 hours in the future

When Alice gets this tx in pocket along with Bob's phased secret tx, she signs and broadcasts the funding tx (edited)
Now if 2 hours goes by the money just comes back to Alice
Bob can only spend the funds by revealing the second of his pubkeys and if he does, Alice can approve the phased secret tx
race condition: Bob cashes the payment right before the refund, not giving Alice anytime to approve the phased tx, so the phased tx must expire after the refund

if alice never broadcasts the signed funding tx, everything expires
if there is a 2+ hour btc block, then the refund tx is triggered and Bob is not getting the funds, but if he puts a higher txfee than alice's refund tx, his tx should get priority, so as long as he submits the spend immediately, we are down to the edge case of a new block where alice broadcast her refundtx, but bob's cashing of it didnt make it until the next block, which ended up being a 2+hr block and Alice sniffed the unconfirmed tx to grab the pubkey and approved the phased tx
the tradeoff is the 2hour time period, since Bob is at risk, he can set the expiration time required, but the finish height is an Alice tradeoff, so I guess she sets that
with well behaved traders, all this could happen in realtime
not sure if there is a malleability attack alice can do with the funding tx... i think she can spend the inputs of the funding tx, so bob need to wait for it to confirm
*/
char *subatomic_txid(char *txbytes,struct coin777 *coin,char *destaddr,uint64_t amount,int32_t future)
{
    struct subatomic_halftx *H; struct subatomic_unspent_tx *utx; int32_t num; uint64_t total; char *txid = 0;
    if ( coin != 0 && destaddr != 0 && destaddr[0] != 0 )
    {
        H = calloc(1,sizeof(*H));
        strcpy(H->coinstr,coin->name);
        utx = gather_unspents(&total,&num,coin,0);
        strcpy(H->multisigaddr.buf,destaddr);
        if ( (txid= subatomic_create_fundingtx(H,amount,future,coin->changeaddr)) != 0 ) //*SUBATOMIC_LOCKTIME);
            strcpy(txbytes,H->funding.rawtransaction);
        free(H);
    }
    return(txid);
}

void oldtest_subatomic()
{
    char *teststr = "{\"requestType\":\"subatomic\",\"NXT\":\"423766016895692955\",\"coin\":\"BTCD\",\"amount\":\"100\",\"coinaddr\":\"RGLbLB5YHM6vngmd8XKvAFCUK8zDfWoSSr\",\"senderip\":\"209.126.71.170\",\"destNXT\":\"8989816935121514892\",\"destcoin\":\"LTC\",\"destamount\":\".1\",\"destcoinaddr\":\"LLedxvb1e5aCYmQfn8PHEPFR56AsbGgbUG\"}";
    struct subatomic_tx *atx; char *redeem,scriptPubKey[1024];
    if ( (atx= update_subatomic_state(cJSON_Parse(teststr),SUPERNET.my64bits,SUPERNET.myipaddr,SUPERNET.myipaddr)) != 0 )
    {
        struct subatomic_unspent_tx *utx; int32_t num; uint64_t total; char *txid,txbytes[8192],hexstr[1024]; uint8_t tmpbuf[128];
        utx = gather_unspents(&total,&num,coin777_find("BTCD",0),0);
        strcpy(atx->myhalf.multisigaddr.buf,"RDRWMSrDdoUcfZRBWUz7KZQSxPS9bZRerM");//bGMMi9syucbu5qdLUbuprHCBMrEowzDDM4");
        txid = subatomic_create_fundingtx(&atx->myhalf,SATOSHIDEN,0*SUBATOMIC_LOCKTIME,"RDRWMSrDdoUcfZRBWUz7KZQSxPS9bZRerM");
        subatomic_txid(txbytes,coin777_find("BTCD",1),"RDRWMSrDdoUcfZRBWUz7KZQSxPS9bZRerM",SATOSHIDEN,0);
        printf("funding.(%s)\n",txbytes);
        // A "RGgbJiV2Hs8eWJbZmZ3hZLS8sSzFA7tYNu", B "REsAF17mNLqPgeeUN8oRkvPmSfSkKzRwPt", rmd160 of "RDRWMSrDdoUcfZRBWUz7KZQSxPS9bZRerM" -> "0235e49408b9714427d3378156dc28abece43ad3f6354aba3d2137a0cbca8cab50" -> "2d7315f3abb363f7bbe8ce5583896e76ac41762b"
        //redeem = create_atomictx_scripts(scriptPubKey,"024aa82667ac0dd37c50c860a4cf6f2ed04e9434b8bf4465cb8da61cc3c0e45f7f","027013ac01bd51a0147623a14e622fc3639551e325da845692563bf95643f23cf3","2d7315f3abb363f7bbe8ce5583896e76ac41762b");
        // A 14XrJHhqMvDszMMwKR2d6u5PPvDeRieQoG -> 03b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd05
        // B 17Y5rr67RpTtuCd929dS6muyKZmjqJZQpr -> 037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc0
        // S 1NBKFDmmHSpSzFnP9bX5bDWnm9zqD6nUv4 -> 03c8a7e64c763e908549aa0f93974ab346a559b486eba32bf14338542b4282b916 -> e84e0808b8cd32c38fa2093a490efabc6be53d7a
        
        // createmultisig 2 '["03b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd05", "037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc0"]'
        // addmultisigaddress 2 '["03b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd05", "037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc0"]'
        calc_OP_HASH160(hexstr,tmpbuf,"03c8a7e64c763e908549aa0f93974ab346a559b486eba32bf14338542b4282b916");
        printf("rmd160.(%s)\n",hexstr);
        redeem = create_atomictx_scripts(scriptPubKey,"03b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd05","037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc0",hexstr);
        // -> redeemScript.(63522103b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd0521037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc052ae6776a914e84e0808b8cd32c38fa2093a490efabc6be53d7a88ac68) scriptPubKey.(a9143d0f47bf8892b8f2847c35cffa576fd74d3436f587) p2sh.(053d0f47bf8892b8f2847c35cffa576fd74d3436f50f7335bf) [3d0f47bf8892b8f2847c35cffa576fd74d3436f5]
        // -> 37FsSwJ59z5UuRjNFjtfNYWVFquBEsUGHC
        // 3d0f47bf8892b8f2847c35cffa576fd74d3436f5
        // txid: b1b915dcbfb9becc368ed5c56c145de26929232ae56b74af76cfe9c0f915896c/v0 has 0.00025
        
        // createrawtransaction '[{"txid":"b1b915dcbfb9becc368ed5c56c145de26929232ae56b74af76cfe9c0f915896c","vout":0,"scriptPubKey":"a9143d0f47bf8892b8f2847c35cffa576fd74d3436f587","redeemScript":"63522103b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd0521037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc052ae6776a914e84e0808b8cd32c38fa2093a490efabc6be53d7a88ac68"}]' '{"37FsSwJ59z5UuRjNFjtfNYWVFquBEsUGHC":0.00015}'
        // signrawtransaction 01000000016c8915f9c0e9cf76af746be52a232969e25d146cc5d58e36ccbeb9bfdc15b9b10000000000ffffffff01983a00000000000017a9143d0f47bf8892b8f2847c35cffa576fd74d3436f58700000000 '[{"txid":"b1b915dcbfb9becc368ed5c56c145de26929232ae56b74af76cfe9c0f915896c","vout":0,"scriptPubKey":"a9143d0f47bf8892b8f2847c35cffa576fd74d3436f587","redeemScript":"63522103b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd0521037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc052ae6776a914e84e0808b8cd32c38fa2093a490efabc6be53d7a88ac68"}]' '["L5o9SpeVSyvyaAwMKBFz69zicpynRuD1cJcmk4hg2XFLFG8YzMDf", "KwzgGzkgWPAbLcnFvXQ2J4JQYAE7Bt3f13WLggvk3CaDoPVVELxx", "KziNoJpt9tLXCn7wHBQATKbi9Kg3vuAD5pUwfFVExBFeZbeuyqHu"]'
        // signrawtransaction 01000000016c8915f9c0e9cf76af746be52a232969e25d146cc5d58e36ccbeb9bfdc15b9b10000000000ffffffff01983a00000000000017a9143d0f47bf8892b8f2847c35cffa576fd74d3436f58700000000 '[{"txid":"b1b915dcbfb9becc368ed5c56c145de26929232ae56b74af76cfe9c0f915896c","vout":0,"scriptPubKey":"a9143d0f47bf8892b8f2847c35cffa576fd74d3436f587","redeemScript":"63522103b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd0521037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc052ae6776a914e84e0808b8cd32c38fa2093a490efabc6be53d7a88ac68"}]' '["L5o9SpeVSyvyaAwMKBFz69zicpynRuD1cJcmk4hg2XFLFG8YzMDf", "KwzgGzkgWPAbLcnFvXQ2J4JQYAE7Bt3f13WLggvk3CaDoPVVELxx"]'

        // --> 01000000016c8915f9c0e9cf76af746be52a232969e25d146cc5d58e36ccbeb9bfdc15b9b100000000654c6363522103b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd0521037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc052ae6776a914e84e0808b8cd32c38fa2093a490efabc6be53d7a88ac68ffffffff01983a00000000000017a9143d0f47bf8892b8f2847c35cffa576fd74d3436f58700000000
        
      //93694eb171f41e988617e047136e6c8688ee90e6
        
        // signed 473044022029fd8c662036f2e7aeb73ea3415162310fdd67e8ffa47ce9d7d84e0e6f9833e002203794cbf352e62c2ddcbce333a456f686d7c1c7f950ab52457b3cd16a7c00216d012103478201a961466c245c00a2cf2dfd87ded6dd3945a8da00c2b99c06d492b0b111
        
        // createrawtransaction '[{"txid":"3b6a96d80c579040442e594da8dde81bac966e3448748c2f32017f7fddfc1a9c","vout":0,"scriptPubKey":"a914eb2d8cbcf2aef2bafa76f57523b974f76dbc060887","redeemScript":"522103b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd0521037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc052ae"}]' '{"36h3SVq4dR6M3WJiSNsB6t5ZCz4QNauBAL":0.0009}'
        // signrawtransaction 01000000019c1afcdd7f7f01322f8c7448346e96ac1be8dda84d592e444090570cd8966a3b0000000000ffffffff01905f01000000000017a91436d9e510cef4bfb5757265a8d840f037dc6df0968700000000 '[{"txid":"3b6a96d80c579040442e594da8dde81bac966e3448748c2f32017f7fddfc1a9c","vout":0,"scriptPubKey":"a91436d9e510cef4bfb5757265a8d840f037dc6df09687","redeemScript":"522103b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd0521037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc052ae"}]' '["L5o9SpeVSyvyaAwMKBFz69zicpynRuD1cJcmk4hg2XFLFG8YzMDf", "KwzgGzkgWPAbLcnFvXQ2J4JQYAE7Bt3f13WLggvk3CaDoPVVELxx"]'
        
        //struct rawvin { char txidstr[128]; uint16_t vout; };
        //struct rawvout { char coinaddr[128],script[2048]; uint64_t value; };
        // struct cointx_input { struct rawvin tx; char coinaddr[64],sigs[1024]; uint64_t value; uint32_t sequence; char used; };
        /* struct cointx_info
        {
            uint32_t crc; // MUST be first
            char coinstr[16],cointxid[128];
            uint64_t inputsum,amount,change,redeemtxid;
            uint32_t allocsize,batchsize,batchcrc,gatewayid,isallocated,completed;
            // bitcoin tx order
            uint32_t version,timestamp,numinputs;
            uint32_t numoutputs;
            struct cointx_input inputs[MAX_COINTX_INPUTS];
            struct rawvout outputs[MAX_COINTX_OUTPUTS];
            uint32_t nlocktime;
            // end bitcoin txcalc_nxt64bits
            char signedtx[];
        };*/
        // createrawtransaction '[{"txid":"b1b915dcbfb9becc368ed5c56c145de26929232ae56b74af76cfe9c0f915896c","vout":0,"scriptPubKey":"a9143d0f47bf8892b8f2847c35cffa576fd74d3436f587","redeemScript":"63522103b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd0521037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc052ae6776a914e84e0808b8cd32c38fa2093a490efabc6be53d7a88ac68"}]' '{"37FsSwJ59z5UuRjNFjtfNYWVFquBEsUGHC":0.00015}'
        // signrawtransaction 01000000016c8915f9c0e9cf76af746be52a232969e25d146cc5d58e36ccbeb9bfdc15b9b10000000000ffffffff01983a00000000000017a9143d0f47bf8892b8f2847c35cffa576fd74d3436f58700000000 '[{"txid":"b1b915dcbfb9becc368ed5c56c145de26929232ae56b74af76cfe9c0f915896c","vout":0,"scriptPubKey":"a9143d0f47bf8892b8f2847c35cffa576fd74d3436f587","redeemScript":"63522103b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd0521037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc052ae6776a914e84e0808b8cd32c38fa2093a490efabc6be53d7a88ac68"}]' '["L5o9SpeVSyvyaAwMKBFz69zicpynRuD1cJcmk4hg2XFLFG8YzMDf", "KwzgGzkgWPAbLcnFvXQ2J4JQYAE7Bt3f13WLggvk3CaDoPVVELxx"]'
        // signrawtransaction 01000000019c1afcdd7f7f01322f8c7448346e96ac1be8dda84d592e444090570cd8966a3b0000000000ffffffff01905f01000000000017a91436d9e510cef4bfb5757265a8d840f037dc6df0968700000000 '[{"txid":"b1b915dcbfb9becc368ed5c56c145de26929232ae56b74af76cfe9c0f915896c","vout":0,"scriptPubKey":"a9143d0f47bf8892b8f2847c35cffa576fd74d3436f587","redeemScript":"002103b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd0521037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc05163522103b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd0521037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc052ae6776a914e84e0808b8cd32c38fa2093a490efabc6be53d7a88ac68"}]' '["L5o9SpeVSyvyaAwMKBFz69zicpynRuD1cJcmk4hg2XFLFG8YzMDf", "KwzgGzkgWPAbLcnFvXQ2J4JQYAE7Bt3f13WLggvk3CaDoPVVELxx"]'
         //00
        //2103b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd05
        //21037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc0
        //51
        //63522103b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd0521037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc052ae6776a914e84e0808b8cd32c38fa2093a490efabc6be53d7a88ac68
     
        //00
        //21037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc0
        //00
        //63522103b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd0521037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc052ae6776a914e84e0808b8cd32c38fa2093a490efabc6be53d7a88ac68

        //strcpy(T.inputs[0].sigs,"002103b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd0521037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc05163522103b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd0521037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc052ae6776a914e84e0808b8cd32c38fa2093a490efabc6be53d7a88ac68");
        //strcpy(T.inputs[0].sigs,"63522103b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd0521037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc052ae6776a914e84e0808b8cd32c38fa2093a490efabc6be53d7a88ac68");
     }
    getchar();
}

#include <stdbool.h>

struct bp_key { void *k; };
typedef struct cstring {
	char	*str;		// string data, incl. NUL
	size_t	len;		// length of string, not including NUL
	size_t	alloc;		// total allocated buffer length
} cstring;

extern bool bp_key_init(struct bp_key *key);
extern void bp_key_free(struct bp_key *key);
extern bool bp_key_generate(struct bp_key *key);
extern bool bp_privkey_set(struct bp_key *key, const void *privkey, size_t pk_len);
extern bool bp_pubkey_set(struct bp_key *key, const void *pubkey, size_t pk_len);
extern bool bp_key_secret_set(struct bp_key *key, const void *privkey_, size_t pk_len);
extern bool bp_privkey_get(const struct bp_key *key, void **privkey, size_t *pk_len);
extern bool bp_pubkey_get(const struct bp_key *key, void **pubkey, size_t *pk_len);
extern bool bp_key_secret_get(void *p, size_t len, const struct bp_key *key);
extern bool bp_sign(const struct bp_key *key, const void *data, size_t data_len,void **sig_, size_t *sig_len_);
extern bool bp_verify(const struct bp_key *key, const void *data, size_t data_len,const void *sig, size_t sig_len);

void cstr_free(cstring *s, bool free_buf);
cstring *base58_encode_check(unsigned char addrtype,bool have_addrtype,const void *data,size_t data_len);
cstring *base58_decode_check(unsigned char *addrtype, const char *s_in);

struct btcaddr
{
	struct bp_key key;
    uint8_t *pubkey; uint16_t p2sh;
    char addr[36],coin[8];
    uint8_t privkey[280];
};

struct btcaddr *btcaddr_new(char *coin,char *p2sh_script)
{
    uint8_t script[8192],md160[20],addrtype; char pubkeystr[512],privkeystr[512],hashstr[41];
    void *privkey=0,*pubkey=0; int32_t n; size_t len,slen; cstring *btc_addr; struct btcaddr *btc;
    btc = calloc(1,sizeof(*btc));
    strncpy(btc->coin,coin,sizeof(btc->coin)-1);
    if ( p2sh_script != 0 )
    {
        if ( strcmp("BTC",coin) == 0 )
            addrtype = 5;
        else if ( strcmp("BTCD",coin) == 0 )
            addrtype = 85;
        else addrtype = 5;
        calc_OP_HASH160(0,md160,p2sh_script);
        btc->p2sh = n = (int32_t)strlen(p2sh_script) >> 1;
        decode_hex(script,n,p2sh_script);
        if ( (btc_addr= base58_encode_check(addrtype,true,md160,sizeof(md160))) != 0 )
        {
            if ( n > sizeof(btc->privkey)-23 )
            {
                printf("script.(%s) len.%d is too big\n",p2sh_script,n);
                free(btc);
                return(0);
            }
            strcpy(btc->addr,btc_addr->str);
            memcpy(btc->privkey,script,n);
            btc->pubkey = &btc->privkey[sizeof(btc->privkey) - 23];
            btc->pubkey[0] = SCRIPT_OP_HASH160;
            btc->pubkey[2] = 20;
            memcpy(&btc->pubkey[2],md160,20);
            btc->pubkey[22] = SCRIPT_OP_EQUAL;
            init_hexbytes_noT(privkeystr,script,n);
            printf("type.%u btcaddr.%ld addr.(%s) %ld p2sh.(%s) %d\n",addrtype,sizeof(struct btcaddr),btc->addr,strlen(btc->addr),privkeystr,n);
            cstr_free(btc_addr,true);
        } else free(btc), btc = 0;
        return(btc);
    }
    else if ( bp_key_init(&btc->key) != 0 && bp_key_generate(&btc->key) != 0 && bp_pubkey_get(&btc->key,&pubkey,&len) != 0 && bp_privkey_get(&btc->key,&privkey,&slen) != 0 )
    {
        if ( len == 33 && slen == 214 && memcmp((void *)((long)privkey + slen - 33),pubkey,33) == 0 )
        {
            init_hexbytes_noT(pubkeystr,pubkey,len);
            init_hexbytes_noT(privkeystr,privkey,slen);
            if ( strcmp("BTC",coin) == 0 )
                addrtype = 0;
            else if ( strcmp("BTCD",coin) == 0 )
                addrtype = 60;
            else addrtype = 0;
            calc_OP_HASH160(hashstr,md160,pubkeystr);
            if ( (btc_addr= base58_encode_check(addrtype,true,md160,sizeof(md160))) != 0 )
            {
                strcpy(btc->addr,btc_addr->str);
                memcpy(btc->privkey,privkey,slen);
                btc->pubkey = &btc->privkey[slen - len];
                printf("type.%u btcaddr.%ld rmd160.(%s) addr.(%s) %ld pubkey.(%s) %ld privkey.(%s) %ld\n",addrtype,sizeof(struct btcaddr),hashstr,btc->addr,strlen(btc->addr),pubkeystr,len,privkeystr,slen);
                cstr_free(btc_addr,true);
            }
            else free(btc), btc = 0;
        } else free(btc), btc = 0;
    }
    return(btc);
}

void testmain()
{
    btcaddr_new("BTC",0);
    btcaddr_new("BTC","63522103b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd0521037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc052ae6776a914e84e0808b8cd32c38fa2093a490efabc6be53d7a88ac68");
    btcaddr_new("BTCD",0);
    btcaddr_new("BTCD","635221024aa82667ac0dd37c50c860a4cf6f2ed04e9434b8bf4465cb8da61cc3c0e45f7f21027013ac01bd51a0147623a14e622fc3639551e325da845692563bf95643f23cf352ae6776a9142d7315f3abb363f7bbe8ce5583896e76ac41762b88ac68");
}

int32_t btc_getpubkey(char pubkeystr[67],uint8_t pubkeybuf[33],struct bp_key *key)
{
    void *pubkey = 0; size_t len = 0;
    bp_pubkey_get(key,&pubkey,&len);
    if ( pubkey != 0 )
    {
        if ( len < 34 )
        {
            init_hexbytes_noT(pubkeystr,pubkey,(int32_t)len);
            memcpy(pubkeybuf,pubkey,len);
        }
        else printf("btc_getpubkey error len.%ld\n",len), len = -1;
        //printf("btc_getpubkey len.%ld (%s).%p\n",len,pubkeystr,pubkeystr);
    } else len = -1;
    return((int32_t)len);
}

int32_t btc_coinaddr(char *coinaddr,struct bp_key *key,uint8_t addrtype,char *pubkeystr)
{
    uint8_t md160[20]; char hashstr[41]; cstring *btc_addr;
    calc_OP_HASH160(hashstr,md160,pubkeystr);
    if ( (btc_addr= base58_encode_check(addrtype,true,md160,sizeof(md160))) != 0 )
    {
        strcpy(coinaddr,btc_addr->str);
        cstr_free(btc_addr,true);
    }
    return(-1);
}

int32_t btc_convwip(uint8_t *privkey,char *wipstr)
{
    uint8_t addrtype; cstring *cstr; int32_t len = -1;
    if ( (cstr= base58_decode_check(&addrtype,(const char *)wipstr)) != 0 )
    {
        init_hexbytes_noT((void *)privkey,(void *)cstr->str,cstr->len);
        if ( cstr->str[cstr->len-1] == 0x01 )
            cstr->len--;
        memcpy(privkey,cstr->str,cstr->len);
        len = (int32_t)cstr->len;
        //printf("addrtype.%02x wipstr.(%s) len.%d\n",addrtype,privkey,len);
        cstr_free(cstr,true);
    }
    return(len);
}

void test_subatomic()
{
    struct cointx_info T,refT; bits256 hash2; char hexstr[8192],scriptPubKey[128],*p2sh_script,pubA[67],pubB[67],pubP[67];
    uint8_t data[8192],tmpbuf[512],privA[256],privB[256],privP[256],sigbuf[512],pubkeyA[33],pubkeyB[33],pubkeyP[33];
    struct bp_key keyA,keyB,keyP; int32_t i,lenA,lenB,lenP,msigredeem;
  	void *sig = NULL; size_t siglen = 0; struct btcaddr *btc; struct coin777 *coin = coin777_find("BTC",1);
    // A 14XrJHhqMvDszMMwKR2d6u5PPvDeRieQoG -> 03b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd05
    // B 17Y5rr67RpTtuCd929dS6muyKZmjqJZQpr -> 037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc0
    // S 1NBKFDmmHSpSzFnP9bX5bDWnm9zqD6nUv4 -> 03c8a7e64c763e908549aa0f93974ab346a559b486eba32bf14338542b4282b916 -> e84e0808b8cd32c38fa2093a490efabc6be53d7a
    // ["L5o9SpeVSyvyaAwMKBFz69zicpynRuD1cJcmk4hg2XFLFG8YzMDf", "KwzgGzkgWPAbLcnFvXQ2J4JQYAE7Bt3f13WLggvk3CaDoPVVELxx", "KziNoJpt9tLXCn7wHBQATKbi9Kg3vuAD5pUwfFVExBFeZbeuyqHu"]
    lenA = btc_convwip(privA,"L5o9SpeVSyvyaAwMKBFz69zicpynRuD1cJcmk4hg2XFLFG8YzMDf");
    lenB = btc_convwip(privB,"KwzgGzkgWPAbLcnFvXQ2J4JQYAE7Bt3f13WLggvk3CaDoPVVELxx");
    lenP = btc_convwip(privP,"KziNoJpt9tLXCn7wHBQATKbi9Kg3vuAD5pUwfFVExBFeZbeuyqHu");
    //04c431daf9d7de4836a1f00808247d6b28253add0b188af3eb12f85917fd7d06f2ca91163564308b6841b4255c1ae02216ab91d8148647ebc1eb9d205f16d393fc
    //02c431daf9d7de4836a1f00808247d6b28253add0b188af3eb12f85917fd7d06f2
    bp_key_init(&keyA), bp_key_init(&keyB), bp_key_init(&keyP);
    if ( bp_key_secret_set(&keyA,privA,lenA) == 0 )
        printf("error setting privkeyA\n");
    if ( bp_key_secret_set(&keyB,privB,lenB) == 0 )
        printf("error setting privkeyB\n");
    if ( bp_key_secret_set(&keyP,privP,lenP) == 0 )
        printf("error setting privkeyP\n");
    if ( btc_getpubkey(pubA,pubkeyA,&keyA) == 33 && btc_getpubkey(pubB,pubkeyB,&keyB) == 33 && btc_getpubkey(pubP,pubkeyP,&keyP) == 33 )
    {
        if ( strcmp(pubA,"03b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd05") != 0 )
            printf("%p pubA.(%s) vs (%s)\n",pubA,pubA,"03b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd05");
        if ( strcmp(pubB,"037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc0") != 0 )
            printf("%p pubB.(%s) vs (%s)\n",pubB,pubB,"037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc0");
        if ( strcmp(pubP,"03c8a7e64c763e908549aa0f93974ab346a559b486eba32bf14338542b4282b916") != 0 )
            printf("%p pubP.(%s) vs (%s)\n",pubP,pubP,"03c8a7e64c763e908549aa0f93974ab346a559b486eba32bf14338542b4282b916");
    }
    //printf("(%d %d %d) pubA.(%s) pubB.(%s) pubP.(%s)\n",lenA,lenB,lenP,pubA,pubB,pubP);
    calc_OP_HASH160(hexstr,tmpbuf,pubP);
    p2sh_script = create_atomictx_scripts(scriptPubKey,pubA,pubB,hexstr);
    btc = btcaddr_new(coin->name,p2sh_script);
    //printf("rmd160.(%s) addr.(%s) scriptPubKey.(%s)\n",hexstr,btc->addr,scriptPubKey);
    if ( strcmp(coin->name,"BTC") == 0 )
        coin->mgw.oldtx_format = 1;
    memset(&refT,0,sizeof(refT));
    refT.version = 1;
    refT.timestamp = (uint32_t)time(NULL);
    strcpy(refT.inputs[0].tx.txidstr,"9bdae874a4e5c80365f0ab1808d93af76aa2c78d74a009f4d071c8d4dd3e44d5");
    refT.inputs[0].tx.vout = 0;
    //refT.inputs[0].value = (uint64_t)(SATOSHIDEN * 0.0004);
    refT.inputs[0].sequence = (uint32_t)-1;
    refT.numinputs = 1;
    
    strcpy(refT.outputs[0].coinaddr,btc->addr);
    strcpy(refT.outputs[0].script,scriptPubKey);
    refT.outputs[0].value = 10000;
    refT.numoutputs = 1;
    
    // strcpy(T.inputs[0].sigs,"002103b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd0521037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc05163522103b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd0521037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc052ae6776a914e84e0808b8cd32c38fa2093a490efabc6be53d7a88ac68");
    for (msigredeem=0; msigredeem<2; msigredeem++)
    {
        T = refT;
        for (i=0; i<T.numinputs; i++)
            strcpy(T.inputs[i].sigs,"00");
        strcpy(T.inputs[0].sigs,p2sh_script);
        T.nlocktime = (msigredeem == 0) ? 0 : (uint32_t)373286;
        if ( msigredeem == 0 )
        {
            strcpy(T.outputs[0].coinaddr,"1NBKFDmmHSpSzFnP9bX5bDWnm9zqD6nUv4");
            sprintf(T.outputs[0].script,"76a914%s88ac","e84e0808b8cd32c38fa2093a490efabc6be53d7a");
            T.inputs[0].sequence = (uint32_t)-1;
        }
        else
        {
            strcpy(T.outputs[0].coinaddr,"14XrJHhqMvDszMMwKR2d6u5PPvDeRieQoG");
            calc_OP_HASH160(hexstr,tmpbuf,"03b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd05");
            sprintf(T.outputs[0].script,"76a914%s88ac",hexstr);
            T.inputs[0].sequence = (uint32_t)time(NULL);
        }
        disp_cointx(&T);
        emit_cointx(&hash2,data,sizeof(data),&T,coin->mgw.oldtx_format,SIGHASH_ALL);
        if ( msigredeem != 0 )
        {
            strcpy(T.inputs[0].sigs,"00");
            if ( bp_sign(&keyA,hash2.bytes,sizeof(hash2),&sig,&siglen) != 0 )
            {
                memcpy(sigbuf,sig,siglen);
                sigbuf[siglen++] = SIGHASH_ALL;
                init_hexbytes_noT(hexstr,sigbuf,(int32_t)siglen);
                sprintf(&T.inputs[0].sigs[strlen(T.inputs[0].sigs)],"%02lx%s",siglen,hexstr);//%02lx%s",siglen,hexstr,strlen(pubA)/2,pubA);
                //printf("after A.(%s) siglen.%02lx\n",T.inputs[0].sigs,siglen);
            }
            if ( bp_sign(&keyB,hash2.bytes,sizeof(hash2),&sig,&siglen) != 0 )
            {
                memcpy(sigbuf,sig,siglen);
                sigbuf[siglen++] = SIGHASH_ALL;
                init_hexbytes_noT(hexstr,sigbuf,(int32_t)siglen);
                sprintf(&T.inputs[0].sigs[strlen(T.inputs[0].sigs)],"%02lx%s",siglen,hexstr);//%02lx%s",siglen,hexstr,strlen(pubB)/2,pubB);
                //printf("after B.(%s) siglen.%02lx\n",T.inputs[0].sigs,siglen);
            }
            strcat(T.inputs[0].sigs,"51");
        }
        else
        {
            if ( bp_sign(&keyP,hash2.bytes,sizeof(hash2),&sig,&siglen) != 0 )
            {
                memcpy(sigbuf,sig,siglen);
                sigbuf[siglen++] = SIGHASH_ALL;
                init_hexbytes_noT(hexstr,sigbuf,(int32_t)siglen);
                sprintf(T.inputs[0].sigs,"%02lx%s%02lx%s00",siglen,hexstr,strlen(pubP)/2,pubP);
                printf("after P.(%s) siglen.%02lx\n",T.inputs[0].sigs,siglen);
            }
        }
        sprintf(&T.inputs[0].sigs[strlen(T.inputs[0].sigs)],"4c%02lx%s",strlen(p2sh_script)/2,p2sh_script);
        printf("scriptSig.(%s)\n",T.inputs[0].sigs);
        _emit_cointx(hexstr,sizeof(hexstr),&T,coin->mgw.oldtx_format);
        printf("T.msigredeem %d -> (%s)\n",msigredeem,hexstr);
    }
    // spend 01000000016c8915f9c0e9cf76af746be52a232969e25d146cc5d58e36ccbeb9bfdc15b9b100000000d2493046022100f1de426daa15d0e9efc9f89afc52afca5f0d8478333b00e563a0d72b4dfd710f022100ecb147fbaf98153f2769cd50dbe90e882d6b235ffbfb4e13d67cc3ed3cac72ed012103c8a7e64c763e908549aa0f93974ab346a559b486eba32bf14338542b4282b916004c6363522103b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd0521037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc052ae6776a914e84e0808b8cd32c38fa2093a490efabc6be53d7a88ac68ffffffff01c40900000000000017a9143d0f47bf8892b8f2847c35cffa576fd74d3436f58700000000
    // refund 01000000016c8915f9c0e9cf76af746be52a232969e25d146cc5d58e36ccbeb9bfdc15b9b100000000f900493046022100ac06c041bec6e946e9d96acebf0e81b13f6ddac7b731c2aea5848dfc70c37e3c022100d8a3d5a2866c52dfdf1eb382541e6dc21c21d22f10e6d51fcf1c591691c4bc3801473044022064c5412a74afef10e02ca3260faa50bacc7c59dd9f6eb40a79f300e34c10046a0220194ab536be7caa3723b46e09568a9d1be2a8db3bf3d1e3085de7ebeb5917803b01514c6363522103b4143b6e7a064a78bc66dab0ca72251fea3d6fd8db1f4c48b91996d7af62dd0521037387677a21b8ebb472ff2d00892098dcd990a9a21b09b8e22aefece8b1cbbdc052ae6776a914e84e0808b8cd32c38fa2093a490efabc6be53d7a88ac68ffffffff01c40900000000000017a9143d0f47bf8892b8f2847c35cffa576fd74d3436f587fe0dec55 -> aac8f093b131d9ceac458850577142f2f99516db74bc08af7255da78a03bfd35
    getchar();
}

#endif
