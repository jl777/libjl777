//
//  mgwcomms.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_mgwcomms_h
#define crypto777_mgwcomms_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"
#include "uthash.h"
#include "utils777.c"
#include "mgwgen1.c"

void ram_send_cointx(struct ramchain_info *ram,struct cointx_info *cointx);
char *ram_check_consensus(char *txidstr,struct ramchain_info *ram,struct NXT_assettxid *tp);

#endif
#else
#ifndef crypto777_mgwcomms_c
#define crypto777_mgwcomms_c

#ifndef crypto777_mgwcomms_h
#define DEFINES_ONLY
#include "mgwcomms.c"
#undef DEFINES_ONLY
#endif

void set_handler_fname(char *fname,char *handler,char *name)
{
    if ( strstr("../",name) != 0 || strstr("..\\",name) != 0 || name[0] == '/' || name[0] == '\\' || strcmp(name,"..") == 0  || strcmp(name,"*") == 0 )
    {
        //printf("(%s) invalid_filename.(%s) %p %p %d %d %d %d\n",handler,name,strstr("../",name),strstr("..\\",name),name[0] == '/',name[0] == '\\',strcmp(name,".."),strcmp(name,"*"));
        name = "invalid_filename";
    }
    sprintf(fname,"%s/%s/%s",SUPERNET.DATADIR,handler,name);
}

int32_t load_handler_fname(void *dest,int32_t len,char *handler,char *name)
{
    FILE *fp;
    int32_t retval = -1;
    char fname[1024];
    set_handler_fname(fname,handler,name);
    if ( (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
    {
        fseek(fp,0,SEEK_END);
        if ( ftell(fp) == len )
        {
            rewind(fp);
            if ( fread(dest,1,len,fp) == len )
                retval = len;
        }
        fclose(fp);
    }
    return(retval);
}

void _set_RTmgwname(char *RTmgwname,char *name,char *coinstr,int32_t gatewayid,uint64_t redeemtxid)
{
    sprintf(name,"%s.%llu.g%d",coinstr,(long long)redeemtxid,gatewayid);
    set_handler_fname(RTmgwname,"RTmgw",name);
}

int32_t cointxcmp(struct cointx_info *txA,struct cointx_info *txB)
{
    if ( txA != 0 && txB != 0 )
    {
        if ( txA->batchcrc == txB->batchcrc )
            return(0);
    }
    return(-1);
}

char *ram_check_consensus(char *txidstr,struct ramchain_info *ram,struct NXT_assettxid *tp)
{
    uint64_t retval,allocsize;
    char RTmgwname[1024],name[512],cmd[1024],hopNXTaddr[64],*cointxid,*retstr = 0;
    int32_t i,gatewayid,len;
    struct cointx_info *cointxs[16],*othercointx;
    memset(cointxs,0,sizeof(cointxs));
    for (gatewayid=0; gatewayid<ram->numgateways; gatewayid++)
    {
        _set_RTmgwname(RTmgwname,name,ram->name,gatewayid,tp->redeemtxid);
        if ( (cointxs[gatewayid]= loadfile(&allocsize,RTmgwname)) == 0 )
        {
            char *send_tokenized_cmd(int32_t queueflag,char *hopNXTaddr,int32_t L,char *verifiedNXTaddr,char *NXTACCTSECRET,char *cmdstr,char *destNXTaddr);
            hopNXTaddr[0] = 0;
            sprintf(cmd,"{\"requestType\":\"getfile\",\"NXT\":\"%s\",\"timestamp\":\"%ld\",\"name\":\"%s\",\"handler\":\"RTmgw\"}",ram->srvNXTADDR,(long)time(NULL),name);
            if ( (len= nn_send(SUPERNET.all.socks.both.bus,(uint8_t *)cmd,(int32_t)strlen(cmd)+1,0)) <= 0 )
                printf("error sending.(%s)\n",cmd);
            else printf("sent (%s).%d\n",cmd,len);
            //if ( (retstr= send_tokenized_cmd(0,hopNXTaddr,0,ram->srvNXTADDR,ram->srvNXTACCTSECRET,cmd,ram->special_NXTaddrs[gatewayid])) != 0 )
            //    free(retstr), retstr = 0;
            printf("cant find.(%s) for %llu %.8f | sent.(%s) to %s\n",RTmgwname,(long long)tp->redeemtxid,dstr(tp->U.assetoshis),cmd,ram->special_NXTaddrs[gatewayid]);
            break;
        }
        for (i=0; i<gatewayid; i++)
            if ( cointxcmp(cointxs[i],cointxs[gatewayid]) != 0 )
            {
                printf("MGW%d %x != %x MGW%d for redeem.%llu %.8f\n",i,cointxs[i]->batchcrc,cointxs[gatewayid]->batchcrc,gatewayid,(long long)tp->redeemtxid,dstr(tp->U.assetoshis));
                break;
            }
    }
    if ( gatewayid != ram->numgateways )
    {
        for (i=0; i<=gatewayid; i++)
            free(cointxs[i]);
        return(0);
    }
    printf("got consensus for %llu %.8f\n",(long long)tp->redeemtxid,dstr(tp->U.assetoshis));
    if ( ram_MGW_ready(ram,0,tp->height,tp->senderbits,tp->U.assetoshis) > 0 )
    {
        if ( (retval= ram_verify_NXTtxstillthere(ram->ap->mult,tp->redeemtxid)) != tp->U.assetoshis )
        {
            fprintf(stderr,"ram_check_consensus tx gone due to a fork. NXT.%llu txid.%llu %.8f vs retval %.8f\n",(long long)tp->senderbits,(long long)tp->redeemtxid,dstr(tp->U.assetoshis),dstr(retval));
            exit(1); // seems the best thing to do
        }
        othercointx = cointxs[(ram->S.gatewayid ^ 1) % ram->numgateways];
        //printf("[%d] othercointx = %p\n",(ram->S.gatewayid ^ 1) % ram->numgateways,othercointx);
        if ( (cointxid= _sign_and_sendmoney(txidstr,ram,cointxs[ram->S.gatewayid],othercointx->signedtx,&tp->redeemtxid,&tp->U.assetoshis,1)) != 0 )
        {
            _complete_assettxid(tp);
            //ram_add_pendingsend(&sendi,ram,tp,0);
            printf("completed redeem.%llu for %.8f cointxidstr.%s\n",(long long)tp->redeemtxid,dstr(tp->U.assetoshis),txidstr);
            retstr = txidstr;
        }
        else printf("ram_check_consensus error _sign_and_sendmoney for NXT.%llu redeem.%llu %.8f (%s)\n",(long long)tp->senderbits,(long long)tp->redeemtxid,dstr(tp->U.assetoshis),othercointx->signedtx);
    }
    for (gatewayid=0; gatewayid<ram->numgateways; gatewayid++)
        free(cointxs[gatewayid]);
    return(retstr);
}

struct NXT_assettxid *_process_realtime_MGW(int32_t *sendip,struct ramchain_info **ramp,struct cointx_info *cointx,char *sender,char *recvname)
{
    uint32_t crc;
    int32_t gatewayid;
    char redeemtxidstr[64];
    struct ramchain_info *ram;
    *ramp = 0;
    *sendip = -1;
    if ( (ram= get_ramchain_info(cointx->coinstr)) == 0 )
        printf("cant find coin.(%s)\n",cointx->coinstr);
    else
    {
        *ramp = ram;
        if ( strncmp(recvname,ram->name,strlen(ram->name)) != 0 ) // + archive/RTmgw/
        {
            printf("_process_realtime_MGW: coin mismatch recvname.(%s) vs (%s).%ld\n",recvname,ram->name,strlen(ram->name));
            return(0);
        }
        crc = _crc32(0,(uint8_t *)((long)cointx+sizeof(cointx->crc)),(int32_t)(cointx->allocsize-sizeof(cointx->crc)));
        if ( crc != cointx->crc )
        {
            printf("_process_realtime_MGW: crc mismatch %x vs %x\n",crc,cointx->crc);
            return(0);
        }
        expand_nxt64bits(redeemtxidstr,cointx->redeemtxid);
        if ( strncmp(recvname+strlen(ram->name)+1,redeemtxidstr,strlen(redeemtxidstr)) != 0 )
        {
            printf("_process_realtime_MGW: redeemtxid mismatch (%s) vs (%s)\n",recvname+strlen(ram->name)+1,redeemtxidstr);
            return(0);
        }
        gatewayid = cointx->gatewayid;
        if ( gatewayid < 0 || gatewayid >= ram->numgateways )
        {
            printf("_process_realtime_MGW: illegal gatewayid.%d\n",gatewayid);
            return(0);
        }
        if ( strcmp(ram->special_NXTaddrs[gatewayid],sender) != 0 )
        {
            printf("_process_realtime_MGW: gatewayid mismatch %d.(%s) vs %s\n",gatewayid,ram->special_NXTaddrs[gatewayid],sender);
            return(0);
        }
        //ram_add_pendingsend(0,ram,0,cointx);
        printf("GOT <<<<<<<<<<<< _process_realtime_MGW.%d coin.(%s) %.8f crc %08x redeemtxid.%llu\n",gatewayid,cointx->coinstr,dstr(cointx->amount),cointx->batchcrc,(long long)cointx->redeemtxid);
    }
    return(0);
}

void _RTmgw_handler(struct transfer_args *args)
{
    struct NXT_assettxid *tp;
    struct ramchain_info *ram;
    int32_t sendi;
    //char txidstr[512];
    printf("_RTmgw_handler(%s %d bytes)\n",args->name,args->totallen);
    if ( (tp= _process_realtime_MGW(&sendi,&ram,(struct cointx_info *)args->data,args->sender,args->name)) != 0 )
    {
        if ( sendi >= ram->numpendingsends || sendi < 0 || ram->pendingsends[sendi] != tp )
        {
            printf("FATAL: _RTmgw_handler sendi %d >= %d ram->numpendingsends || sendi %d < 0 || %p ram->pendingsends[sendi] != %ptp\n",sendi,ram->numpendingsends,sendi,ram->pendingsends[sendi],tp);
            exit(1);
        }
        //ram_check_consensus(txidstr,ram,tp);
    }
    //getchar();
}

void ram_send_cointx(struct ramchain_info *ram,struct cointx_info *cointx)
{
    //char *start_transfer(char *previpaddr,char *sender,char *verifiedNXTaddr,char *NXTACCTSECRET,char *dest,char *name,uint8_t *data,int32_t totallen,int32_t timeout,char *handler,int32_t syncmem);
    char RTmgwname[512],name[512];//,*retstr;
    int32_t len;
    FILE *fp;
    _set_RTmgwname(RTmgwname,name,cointx->coinstr,cointx->gatewayid,cointx->redeemtxid);
    cointx->crc = _crc32(0,(uint8_t *)((long)cointx+sizeof(cointx->crc)),(int32_t)(cointx->allocsize - sizeof(cointx->crc)));
    if ( (fp= fopen(os_compatible_path(RTmgwname),"wb")) != 0 )
    {
        printf("save to (%s).%d crc.%x | batchcrc %x\n",RTmgwname,cointx->allocsize,cointx->crc,cointx->batchcrc);
        fwrite(cointx,1,cointx->allocsize,fp);
        fclose(fp);
    }
    //for (gatewayid=0; gatewayid<NUM_GATEWAYS; gatewayid++)
    {
        /*if ( gatewayid != cointx->gatewayid )
        {
            retstr = start_transfer(0,ram->srvNXTADDR,ram->srvNXTADDR,ram->srvNXTACCTSECRET,SUPERNET.Server_ipaddrs[gatewayid],name,(uint8_t *)cointx,cointx->allocsize,300,"RTmgw",1);
            if ( retstr != 0 )
                free(retstr);
        }*/
        if ( (len= nn_send(SUPERNET.all.socks.both.bus,(uint8_t *)cointx,cointx->allocsize,0)) <= 0 )
            printf("error sending cointx.(%p)\n",cointx);
        else printf("sent cointx.(%p).%d\n",cointx,len);
        
        fprintf(stderr,"got publish_withdraw_info.%d -> MGWbus coin.(%s) %.8f crc %08x\n",ram->S.gatewayid,cointx->coinstr,dstr(cointx->amount),cointx->batchcrc);
    }
}

#endif
#endif