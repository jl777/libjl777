//
//  teleport.h
//  xcode
//
//  Created by jl777 on 8/1/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//
// todo: encrypt local telepods, monitor blockchain, build acct history
/*
 aes
 blowfish
 xtea
 rc5
 rc6
 saferp
 twofish
 safer_k64
 safer_sk64
 safer_k128
 safer_sk128
 rc2
 des
 des3
 cast5
 noekeon
 skipjack
 khazad
 anubis
 */

// make all packets the same size (option)

#ifndef xcode_teleport_h
#define xcode_teleport_h

#define TRANSPORTER_SEND 1
#define TRANSPORTER_RECV 2
#define TELEPORT_TRANSPORTER_TIMEOUT (10. * 1000.)
#define TELEPORT_TELEPODS_TIMEOUT (60. * 1000.)
#define TELEPORT_MAX_CLONETIME (3600. * 1000.)

#define TELEPOD_CONTENTS_VOUT 0 // must be 0
#define TELEPOD_CHANGE_VOUT 1   // vout 0 is for the pod contents and last one (1 if no change or 2) is marker
#define TELEPOD_ERROR_VOUT 30000

struct transporter_log
{
    struct coin_info *cp;
    cJSON *ciphersobj;
    struct telepod **pods;
    double startmilli,recvmilli,completemilli;
    uint64_t satoshis;
    uint32_t dir,numpods,totalcrc,createheight,firstheight,*crcs,*filenums;
    int32_t status,minage,M,N;
    char refcipher[16],mypubaddr[128],otherpubaddr[128];
    uint8_t sharenrs[255];
};

struct telepod
{
    uint32_t filenum,xfered,height,sentheight,crc,totalcrc,cloneblock,confirmheight;
    int16_t cloneout;
    uint16_t len_plus1,tbd2,tbd3,tbd4,tbd5;
    int8_t inhwm,ischange,saved,dir,gotack,spentflag,tbd6,tbd7;
    struct transporter_log *log;
    struct telepod *abortion,*clone;
    double sentmilli,recvmilli,completemilli;
    char clonetxid[MAX_COINTXID_LEN],cloneaddr[MAX_COINADDR_LEN];
    uint64_t destnxt64bits,unspent,modified,satoshis; // everything after modified is used for crc
    uint16_t podsize,vout,spad;
    uint8_t pad,pad2,extraspace[255],sharei;
    char coinstr[8],txid[MAX_COINTXID_LEN],coinaddr[MAX_COINADDR_LEN],script[128];
    unsigned char privkey_shares[];
};
struct pingpong_queue Transporter_sendQ,Transporter_recvQ,CloneQ;//Teleport_sendQ,Teleport_recvQ;

uint64_t calc_transporter_fee(struct coin_info *cp,uint64_t satoshis)
{
    if ( strcmp(cp->name,"BTCD") == 0 )
        return(cp->txfee * 0);
    else return(cp->txfee + (satoshis>>10));
}

#include "bitcoinglue.h"
#include "telepods.h"
#include "transporter.h"

int32_t is_relevant_coinvalue(int32_t spentflag,struct coin_info *cp,char *txid,int32_t vout,uint64_t value,char *script,uint32_t blocknum)
{
    struct telepod *pod;
    //if ( validate_coinaddr(pubkey,cp,vp->coinaddr) > 0 )
    {
        if ( (pod= find_telepod(cp,txid)) != 0 && pod->vout == vout )
        {
            printf("spentflag.%d found relevant telepod dir.%d block.%d txid.(%s) vout.%d\n",spentflag,pod->dir,blocknum,txid,vout);
            if ( spentflag != 0 )
            {
                if ( pod->dir != TRANSPORTER_SEND || value != pod->satoshis )
                    printf("WARNING: unexpected dir.%d unset in vin telepod || value mismatch %.8f vs %.8f\n",pod->dir,dstr(value),dstr(pod->satoshis));
                //else
                {
                    pod->spentflag = 1;
                    safecopy(pod->clonetxid,txid,sizeof(pod->clonetxid)), pod->cloneout = vout;
                    if ( update_telepod_file(cp->name,pod->filenum,pod,cp->name,cp->ciphersobj) != 0 )
                        printf("ERROR saving cloned refpod\n");
                }
            }
            else
            {
                if ( pod->dir != TRANSPORTER_RECV || pod->height == 0 || value != pod->satoshis )
                {
                    printf("WARNING: unexpected dir.%d set in vout telepod or height.%d set || value mismatch %.8f vs %.8f\n",pod->dir,pod->height,dstr(value),dstr(pod->satoshis));
                    pod->dir = TRANSPORTER_RECV;
                }
                //else
                {
                    pod->confirmheight = blocknum;
                    safecopy(pod->script,script,sizeof(pod->script));
                    printf("set script.(%s)\n",script);
                    if ( update_telepod_file(cp->name,pod->filenum,pod,cp->name,cp->ciphersobj) != 0 )
                        printf("ERROR saving cloned refpod\n");
                }
            }
            return(1);
        } //else printf("not caring about %s\n",txid);
    }// else printf("warning: couldnt find pubkey for (%s)\n",vp->coinaddr);
    return(0);
}

int32_t process_transporterQ(void **ptrp,void *arg) // added when outbound transporter sequence is started
{
    struct transporter_log *log = (*ptrp);
    struct NXT_acct *destnp,*np;
    unsigned char *buffer;
    int32_t i,sharei,err=0,verified = 0;
    struct telepod *pod;
    if ( log->cp->initdone < 2 )
        return(0);
    np = search_addresses(log->cp->pubaddr);
    destnp = search_addresses(log->otherpubaddr);
    printf("log recv %f complete %f\n",log->recvmilli,log->completemilli);
    if ( log->recvmilli != 0. && log->completemilli == 0. )
    {
        for (i=0; i<log->numpods; i++)
        {
            //printf("check %d'th pod\n",i);
            if ( (pod= log->pods[i]) != 0 )
            {
                printf("pod sentmilli %f complete %f clonetxid.(%c) cloneout.%d\n",pod->sentmilli,pod->completemilli,pod->clonetxid[0],pod->cloneout);
                if ( pod->clonetxid[0] != 0 && pod->cloneout >= 0 )
                    verified++;
                else if ( pod->sentmilli == 0. )
                {
                    if ( log->N > 1 )
                    {
                        void calc_shares(unsigned char *shares,unsigned char *secret,int32_t size,int32_t width,int32_t M,int32_t N,unsigned char *sharenrs);
                        buffer = calloc(log->N+1,pod->len_plus1);
                        calc_shares(buffer,_get_privkeyptr(pod,calc_multisig_N(pod)),pod->len_plus1 - 1,pod->len_plus1,log->M,log->N,log->sharenrs);
                        printf("back from calc_shares pod.%p (%s) %u %u %u\n",pod,_get_privkeyptr(pod,calc_multisig_N(pod)),_crc32(0,buffer,pod->len_plus1-1),_crc32(0,buffer+pod->len_plus1,pod->len_plus1-1),_crc32(0,buffer+2*pod->len_plus1,pod->len_plus1-1));
                        void gfshare_extract(unsigned char *secretbuf,uint8_t *sharenrs,int32_t N,uint8_t *buffer,int32_t len,int32_t size);
                        gfshare_extract(buffer+log->N*pod->len_plus1,log->sharenrs,log->N,buffer,pod->len_plus1-1,pod->len_plus1);
                        {
                            char hexstr[4096];
                            init_hexbytes(hexstr,buffer+log->N*pod->len_plus1,pod->len_plus1);
                            printf("DECODED.(%s)\n",hexstr);
                        }
                        for (sharei=err=0; sharei<log->N; sharei++)
                        {
                            printf("transport bunny.%d of %d\n",sharei,log->N);
                            if ( teleport_telepod(log->cp->pubaddr,np->H.NXTaddr,log->cp->NXTACCTSECRET,destnp->H.NXTaddr,pod,log->totalcrc,sharei,i,log->M,log->N,buffer+sharei*pod->len_plus1) < 0 )
                            {
                                err++;
                                break;
                            }
                        }
                        free(buffer);
                    }
                    else
                    {
                        sharei = log->N;
                        printf("transport bunny\n");
                        if ( teleport_telepod(log->cp->pubaddr,np->H.NXTaddr,log->cp->NXTACCTSECRET,destnp->H.NXTaddr,pod,log->totalcrc,sharei,i,1,1,_get_privkeyptr(pod,calc_multisig_N(pod))) < 0 )
                            err++;
                    }
                    printf("sharei.%d N.%d err.%d\n",sharei,log->N,err);
                    if ( sharei == log->N )
                        pod->sentmilli = milliseconds();
                    break;
                }
            } else printf("unexpected null pod at %d of %d\n",i,log->numpods);
        }
        //printf("verified.%d of %d, started %f\n",verified,log->numpods,log->startmilli);
        if ( verified == log->numpods )
        {
            log->completemilli = milliseconds();
            return(1);
        }
        else if ( (milliseconds() - log->startmilli) > TELEPORT_MAX_CLONETIME )
        {
            log->completemilli = milliseconds();
            printf("exceed maximum cloning time!\n");
            return(-1);
        }
    }
    else
    {
        if ( log->recvmilli == 0. && (milliseconds() - log->startmilli) > TELEPORT_TRANSPORTER_TIMEOUT )
            return(-1);
        else if ( log->recvmilli != 0. && (milliseconds() - log->recvmilli) > TELEPORT_TELEPODS_TIMEOUT )
            return(-1);
    }
    return(0);
}

int32_t process_recvQ(void **ptrp,void *arg) // added when inbound transporter sequence is started
{
    struct transporter_log *log = (*ptrp);
    if ( log->cp->initdone < 2 )
        return(0);
    if ( log->completemilli != 0 )
        return(1);
    else
    {
        if ( (milliseconds() - log->startmilli) > (TELEPORT_TRANSPORTER_TIMEOUT+TELEPORT_TELEPODS_TIMEOUT) )
            return(-1);
    }
    return(0);
}

int32_t process_cloneQ(void **ptrp,void *arg) // added to this queue when process_telepodQ is done
{
    struct telepod *clone,*pod = (*ptrp);
    struct coin_info *cp;
    cp = get_coin_info(pod->coinstr);
    printf("process cloneQ %p (%s) %p %d\n",pod,pod->coinstr,cp,TELEPOD_ERROR_VOUT);
    if ( cp == 0 )
        return(-1);
    if ( cp->initdone < 2 )
        return(0);
    if ( pod->cloneout == TELEPOD_CONTENTS_VOUT )
    {
        if ( (clone= pod->clone) != 0 )
        {
            if ( clone->confirmheight != 0 )
            {
                printf("Clone confirmed!\n");
                return(1);
            }
        }
    }
    else if ( pod->cloneblock < get_blockheight(cp) )
    {
        pod->clone = clone_telepod(cp,cp->name,cp->ciphersobj,pod);
        if ( pod->cloneout != TELEPOD_CONTENTS_VOUT || pod->clonetxid[0] == 0 )
            return(-1);
    }
    else printf("clone at %d, now %d, wait %d\n",pod->cloneblock,(int)get_blockheight(cp),pod->cloneblock - (int)get_blockheight(cp));
    return(0);
}

void teleport_idler()
{
    //printf("teleport_idler\n");
    process_pingpong_queue(&Transporter_sendQ,0);
    process_pingpong_queue(&Transporter_recvQ,0);
    process_pingpong_queue(&CloneQ,0);
}

void init_Teleport()
{
    printf("init_Teleport()\n");
    init_pingpong_queue(&Transporter_sendQ,"sendQ",process_transporterQ,0,0);
    init_pingpong_queue(&Transporter_recvQ,"recvQ",process_recvQ,0,0);

    init_pingpong_queue(&CloneQ,"cloneQ",process_cloneQ,0,0);
    if ( portable_thread_create(teleport_idler,Global_mp) == 0 )
        printf("ERROR teleport_idler\n");
}

void complete_telepod_reception(struct coin_info *cp,struct telepod *pod,int32_t height)
{
    pod->unspent = get_unspent_value(pod->script,cp,pod);
    pod->completemilli = milliseconds();
    pod->cloneblock = height + (rand() % cp->clonesmear) + 1;
    if ( pod->unspent != pod->satoshis )
    {
        printf("unspent is %.8f instead of %.8f | Naughty sender detected!\n",dstr(pod->unspent),dstr(pod->satoshis));
    } else printf("pod.%p %s unspent %.8f clone in %d blocks\n",pod,pod->coinstr,dstr(pod->unspent),pod->cloneblock - height);
    //ensure_telepod_has_backup(cp,pod,cp->name,cp->ciphersobj);
    if ( update_telepod_file(cp->name,pod->filenum,pod,cp->name,cp->ciphersobj) != 0 )
        printf("ERROR saving cloned refpod\n");
}

void update_teleport_summary(cJSON *array,struct coin_info *cp,int32_t i,int32_t n,struct NXT_acct *sendernp,uint64_t satoshis,uint32_t crc)
{
    cJSON *item;
    char numstr[32];
    item = cJSON_CreateObject();
    sprintf(numstr,"%.8f",dstr(satoshis));
    cJSON_AddItemToObject(item,"satoshis",cJSON_CreateString(numstr));
    cJSON_AddItemToObject(item,"crc",cJSON_CreateNumber(crc));
    cJSON_AddItemToArray(array,item);
}

char *calc_teleport_summary(struct coin_info *cp,struct NXT_acct *sendernp,struct transporter_log *log)
{
    int32_t i;
    char *retstr,numstr[32];
    cJSON *json,*array;
    json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"coin",cJSON_CreateString(cp->name));
    sprintf(numstr,"%.8f",dstr(log->satoshis));
    cJSON_AddItemToObject(json,"value",cJSON_CreateString(numstr));
    array = cJSON_CreateArray();
    for (i=0; i<log->numpods; i++)
        update_teleport_summary(array,cp,i,log->numpods,sendernp,log->pods[i]->satoshis,log->crcs[i]);
    cJSON_AddItemToObject(json,"telepods",array);
    retstr = cJSON_Print(json);
    stripwhite_ns(retstr,strlen(retstr));
    return(retstr);
}

void complete_transporter_reception(struct coin_info *cp,struct transporter_log *log,char *NXTACCTSECRET) // start cloning
{
    char verifiedNXTaddr[64],*retstr;
    int32_t i;
    struct NXT_acct *destnp,*np;
    np = find_NXTacct(verifiedNXTaddr,NXTACCTSECRET);
    //expand_nxt64bits(destNXTaddr,log->otherpubaddr);
    //sendernp = get_NXTacct(&createdflag,Global_mp,destNXTaddr);
    destnp = search_addresses(log->otherpubaddr);
    retstr = calc_teleport_summary(cp,destnp,log);
    if ( retstr != 0 )
    {
        send_tokenized_cmd(verifiedNXTaddr,NXTACCTSECRET,retstr,destnp->H.NXTaddr);
        free(retstr);
    }
    for (i=0; i<log->numpods; i++)
    {
        printf("(%p %s) ",log->pods[i],log->pods[i]->coinstr);
        disp_telepod("clone",log->pods[i]);
        queue_enqueue(&CloneQ.pingpong[0],log->pods[i]);
    }
}

int32_t count_numshares(uint8_t haveshares[255],struct telepod *pod,uint8_t *sharenrs)
{
    static char zeroes[4096];
    char *privkey;
    int32_t i,size,N,numshares = 0;
    memset(haveshares,0,255);
    if ( pod->len_plus1 > sizeof(zeroes) )
    {
        printf("unexpected big privkey! len %d vs %ld\n",pod->len_plus1,sizeof(zeroes));
        size = sizeof(zeroes);
    } else size = pod->len_plus1 - 1;
    N = calc_multisig_N(pod);
    privkey = (char *)pod->privkey_shares;
    for (i=0; i<N; i++,privkey+=pod->len_plus1)
        if ( memcmp(privkey,zeroes,size) != 0 )
            numshares++, haveshares[i] = sharenrs[i];
    //printf("counted %d shares\n",numshares);
    return(numshares);
}

char *telepod_received(char *sender,char *NXTACCTSECRET,char *coinstr,uint32_t crc,int32_t ind,uint32_t height,uint64_t satoshis,char *coinaddr,char *txid,int32_t vout,char *script,char *privkey,uint32_t totalcrc,int32_t sharei,int32_t M,int32_t N,char *otherpubaddr)
{
    char retbuf[4096],verifiedNXTaddr[64];//,*retstr = 0;
    struct NXT_acct *sendernp,*np;
    struct coin_info *cp;
    struct telepod *pod;
    struct transporter_log *log;
    uint8_t haveshares[255];
    int32_t i,completed,createdflag,errflag = -1;
    verifiedNXTaddr[0] = 0;
    sprintf(retbuf,"telepod_received from %s totalcrc.%u sharei.%d crc.%u ind.%d height.%d %.8f %s %s/vout%d (%s) M.%d N.%d",otherpubaddr,totalcrc,sharei,crc,ind,height,dstr(satoshis),coinaddr,txid,vout,script,M,N);
    np = find_NXTacct(verifiedNXTaddr,NXTACCTSECRET);
    sendernp = get_NXTacct(&createdflag,Global_mp,sender);
    cp = get_coin_info(coinstr);
    if ( cp == 0 || crc == 0 || ind < 0 || height == 0 || satoshis == 0 || coinaddr[0] == 0 || txid[0] == 0 || vout < 0 || script[0] == 0 || privkey[0] == 0 )
        strcat(retbuf," <<<<< ERROR");
    else if ( (log= find_transporter_log(cp,otherpubaddr,totalcrc,TRANSPORTER_RECV)) != 0 )
    {
       // printf("telepod received log.%p\n",log);
        if ( crc != 0 && ind < log->numpods )
        {
            //printf("crc.%u for ind.%d of %d\n",crc,ind,log->numpods);
            if ( (pod= log->pods[ind]) == 0 )
            {
                //init_sharenrs(sharenrs,0,N,N);
                pod = create_telepod(0,cp->name,cp->ciphersobj,cp,satoshis,coinaddr,script,privkey,txid,vout,M,N,log->sharenrs,sharei);
                if ( (pod->destnxt64bits= np->H.nxt64bits) == 0 ) // the destination is us! This is flag for cloning
                {
                    np->H.nxt64bits = calc_nxt64bits(verifiedNXTaddr);
                    pod->destnxt64bits = np->H.nxt64bits;
                    printf("unexpected unset nxt64bits %llu for NXT.(%s)\n",(long long)np->H.nxt64bits,verifiedNXTaddr);
                }
                if ( pod->recvmilli == 0. )
                    pod->recvmilli = milliseconds();
                log->pods[ind] = pod;
                printf("<<<<<< set pods[%d] <- %p\n",ind,pod);
            }
            if ( log->N > 1 )
            {
                //char hexstr[4096];
                set_privkey_share(pod,privkey,sharei);
                //init_hexbytes(hexstr,(void *)privkey,pod->len_plus1-1);
                //printf("sharei.%d = (%s)\n",sharei,hexstr);
            }
            if ( log->M == M && log->N == N && pod->satoshis == satoshis && strcmp(pod->coinaddr,coinaddr) == 0 && strcmp(pod->script,script) == 0 && strcmp(pod->txid,txid) == 0 && pod->vout == vout && strcmp(pod->coinstr,cp->name) == 0 )
            {
                if ( pod->completemilli == 0. )
                {
                    if ( N > 1 && count_numshares(haveshares,pod,log->sharenrs) == M )
                    {
                        void gfshare_extract(unsigned char *secretbuf,uint8_t *sharenrs,int32_t N,uint8_t *buffer,int32_t len,int32_t size);
                        printf("haveshares %02x %02x %02x\n",haveshares[0],haveshares[1],haveshares[2]);
                        gfshare_extract(_get_privkeyptr(pod,log->N),haveshares,log->N,_get_privkeyptr(pod,0),pod->len_plus1-1,pod->len_plus1);
                        {
                            char hexstr[4096];
                            init_hexbytes(hexstr,_get_privkeyptr(pod,log->N),pod->len_plus1-1);
                            printf("DECODED.(%s)\n",hexstr);
                            //getchar();
                        }
                    }
                    if ( calc_telepodcrc(pod) == crc )
                    {
                        printf("CRC matched ind.%d %u\n",ind,crc);
                        errflag = 0;
                        pod->crc = crc;
                        log->crcs[ind] = crc;
                        complete_telepod_reception(cp,pod,height);
                        if ( log->completemilli == 0. )
                        {
                            completed = 0;
                            for (i=0; i<log->numpods; i++)
                                if ( log->crcs[ind] != 0 && pod->unspent == pod->satoshis )
                                    completed++;
                            if ( completed == log->numpods )
                            {
                                log->completemilli = milliseconds();
                                printf("completed receipt of %d telepods from NXT.%s for %s %.8f | elapsed %.2f seconds\n",log->numpods,sender,cp->name,dstr(log->satoshis),(log->completemilli - log->startmilli)/1000.);
                                complete_transporter_reception(cp,log,NXTACCTSECRET);
                            }
                        }
                    } //else printf("crc mismatch after decode %u vs %u\n",calc_telepodcrc(pod),crc);
                    //send_transporter_status(verifiedNXTaddr,NXTACCTSECRET,sendernp,cp,height,totalcrc,satoshis,errflag,log->numpods,log->crcs,log->minage,ind,sharei,M,N,log->sharenrs);
                }
            }
            else printf("ERROR >>>>>>>>>>>>>>>> telepod received, but failed test %d %d %d %d %d %d %d %d\n",log->M == M ,log->N == N ,pod->satoshis == satoshis ,strcmp(pod->coinaddr,coinaddr) == 0 ,strcmp(pod->script,script) == 0 ,strcmp(pod->txid,txid) == 0 ,pod->vout == vout ,strcmp(pod->coinstr,cp->name) == 0);
        } else printf("ERROR: unexpected transporter_log received! crc.%u %d\n",crc,ind);
    } else printf("ERROR: unexpected transporter_log received! %u from NXT.%s\n",totalcrc,sender);
    return(clonestr(retbuf));
}

char *teleport(char *NXTaddr,char *NXTACCTSECRET,uint64_t satoshis,char *otherpubaddr,struct coin_info *cp,int32_t minage,int32_t M,int32_t N)
{
    static unsigned char zerokey[crypto_box_PUBLICKEYBYTES];
    char buf[4096];
    uint8_t sharenrs[255];
    struct telepod **pods = 0;
    struct NXT_acct *np,*destnp;
    struct transporter_log *log;
    int32_t n,err = -1;
    uint32_t height;
    if ( M <= 0 )
        M = cp->M;
    if ( N <= 0 )
        N = cp->N;
    if ( M <= 0 )
        M = 1;
    if ( N <= 0 )
        N = 1;
    memset(sharenrs,0,sizeof(sharenrs));
    if ( N > 1 )
        init_sharenrs(sharenrs,0,N,N);
    
    printf("%s -> teleport %.8f %s -> %s minage.%d | M.%d N.%d dest.(%s)\n",NXTaddr,dstr(satoshis),cp->name,otherpubaddr,minage,M,N,otherpubaddr);
    if ( minage == 0 )
        minage = cp->minconfirms;
    destnp = search_addresses(otherpubaddr);
    height = (uint32_t)get_blockheight(cp);
    if ( (pods= evolve_transporter(&n,0,cp,minage,satoshis,height)) == 0 )
        sprintf(buf,"{\"error\":\"teleport: not enough  for %.8f %s to %s\"}",dstr(satoshis),cp->name,otherpubaddr);
    else
    {
        free(pods), pods = 0;
        np = find_NXTacct(NXTaddr,NXTACCTSECRET);
        if ( memcmp(destnp->pubkey,zerokey,sizeof(zerokey)) == 0 )
        {
            query_pubkey(destnp->H.NXTaddr,NXTACCTSECRET);
            sprintf(buf,"{\"error\":\"no pubkey for %s, request sent\"}",otherpubaddr);
        }
        if ( memcmp(destnp->pubkey,zerokey,sizeof(zerokey)) != 0 )
        {
            printf("start evolving at %f\n",milliseconds());
            pods = evolve_transporter(&n,cp->maxevolveiters,cp,minage,satoshis,height);
            printf("finished evolving at %f\n",milliseconds());
            if ( pods == 0 )
                sprintf(buf,"{\"error\":\"unexpected transporter evolve failure for %.8f %s to %s\"}",dstr(satoshis),cp->name,otherpubaddr);
            else
            {
                log = send_transporter_log(NXTaddr,cp->NXTACCTSECRET,cp->name,cp->ciphersobj,destnp,cp,minage,satoshis,pods,n,M,N,sharenrs,otherpubaddr);
                if ( log == 0 )
                {
                    sprintf(buf,"{\"error\":\"unexpected error sending transporter log %.8f %s to %s\"}",dstr(satoshis),cp->name,otherpubaddr);
                    free(pods);
                    pods = 0;
                }
                else
                {
                    err = 0;
                    printf("teleport AFTER CREATE BUNDLE to %s err.%d\n",destnp->H.NXTaddr,err);
                    process_pingpong_queue(&Transporter_sendQ,log);
                }
            }
        }
    }
    return(clonestr(buf));
}

#endif
