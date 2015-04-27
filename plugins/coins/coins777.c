//
//  coins777.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_coins777_h
#define crypto777_coins777_h
#include <stdio.h>
#include "cJSON.h"
#include "storage.c"
#include "db777.c"

#define OP_RETURN_OPCODE 0x6a

char *bitcoind_RPC(char **retstrp,char *debugstr,char *url,char *userpass,char *command,char *params);
char *bitcoind_passthru(char *coinstr,char *serverport,char *userpass,char *method,char *params);
struct coin777 *coin777_create(char *coinstr,char *serverport,char *userpass,cJSON *argjson);
int32_t coin777_close(char *coinstr);
struct coin777 *coin777_find(char *coinstr);
char *extract_userpass(char *userhome,char *coindir,char *confname);

struct coin777 { char name[16],serverport[64],userpass[128],*jsonstr,multisigchar; cJSON *argjson; int32_t use_addmultisig,gatewayid; };

#endif
#else
#ifndef crypto777_coins777_c
#define crypto777_coins777_c

#ifndef crypto777_coins777_h
#define DEFINES_ONLY
#include "utils777.c"
#include "coins777.c"
#undef DEFINES_ONLY
#endif


char *bitcoind_passthru(char *coinstr,char *serverport,char *userpass,char *method,char *params)
{
    return(bitcoind_RPC(0,coinstr,serverport,userpass,method,params));
}

char *parse_conf_line(char *line,char *field)
{
    line += strlen(field);
    for (; *line!='='&&*line!=0; line++)
        break;
    if ( *line == 0 )
        return(0);
    if ( *line == '=' )
        line++;
    stripstr(line,strlen(line));
    if ( Debuglevel > 0 )
        printf("[%s]\n",line);
    return(clonestr(line));
}

char *extract_userpass(char *userhome,char *coindir,char *confname)
{
    FILE *fp;
    char fname[2048],line[1024],userpass[1024],*rpcuser,*rpcpassword,*str;
    userpass[0] = 0;
    sprintf(fname,"%s/%s/%s",userhome,coindir,confname);
    if ( (fp= fopen(os_compatible_path(fname),"r")) != 0 )
    {
        if ( Debuglevel > 0 )
            printf("extract_userpass from (%s)\n",fname);
        rpcuser = rpcpassword = 0;
        while ( fgets(line,sizeof(line),fp) != 0 )
        {
            if ( line[0] == '#' )
                continue;
            //printf("line.(%s) %p %p\n",line,strstr(line,"rpcuser"),strstr(line,"rpcpassword"));
            if ( (str= strstr(line,"rpcuser")) != 0 )
                rpcuser = parse_conf_line(str,"rpcuser");
            else if ( (str= strstr(line,"rpcpassword")) != 0 )
                rpcpassword = parse_conf_line(str,"rpcpassword");
        }
        if ( rpcuser != 0 && rpcpassword != 0 )
            sprintf(userpass,"%s:%s",rpcuser,rpcpassword);
        else userpass[0] = 0;
        if ( Debuglevel > 0 )
            printf("-> (%s):(%s) userpass.(%s)\n",rpcuser,rpcpassword,userpass);
        if ( rpcuser != 0 )
            free(rpcuser);
        if ( rpcpassword != 0 )
            free(rpcpassword);
        fclose(fp);
    }
    else
    {
        printf("extract_userpass cant open.(%s)\n",fname);
        return(0);
    }
    if ( userpass[0] != 0 )
        return(clonestr(userpass));
    return(0);
}

/*
void activate_ramchain(struct ramchain_info *ram,char *name);
uint32_t get_blockheight(struct coin_info *cp);
void init_ramchain_info(struct ramchain_info *ram,struct coin_info *cp,int32_t DEPOSIT_XFER_DURATION,int32_t oldtx)
{
    //struct NXT_asset *ap = 0;
    struct coin_info *refcp = get_coin_info("BTCD");
    int32_t createdflag;
    strcpy(ram->name,cp->name);
    strcpy(ram->S.name,ram->name);
    if ( refcp->myipaddr[0] != 0 )
        strcpy(ram->myipaddr,refcp->myipaddr);
    strcpy(ram->srvNXTACCTSECRET,refcp->srvNXTACCTSECRET);
    strcpy(ram->srvNXTADDR,refcp->srvNXTADDR);
    ram->oldtx = oldtx;
    ram->S.nxt64bits = calc_nxt64bits(refcp->srvNXTADDR);
    if ( cp->marker == 0 )
        cp->marker = clonestr(get_marker(cp->name));
    if ( cp->marker != 0 )
        ram->marker = clonestr(cp->marker);
    if ( cp->marker2 == 0 )
        cp->marker2 = clonestr(get_marker(cp->name));
    if ( cp->marker2 != 0 )
        ram->marker2 = clonestr(cp->marker2);
    if ( cp->privateaddr[0] != 0 )
        ram->opreturnmarker = clonestr(cp->privateaddr);
    ram->dust = cp->dust;
    if ( cp->backupdir[0] != 0 )
        ram->backups = clonestr(cp->backupdir);
    if ( cp->userpass != 0 )
        ram->userpass = clonestr(cp->userpass);
    if ( cp->serverport != 0 )
        ram->serverport = clonestr(cp->serverport);
    ram->lastheighttime = (uint32_t)cp->lastheighttime;
    ram->S.RTblocknum = (uint32_t)cp->RTblockheight;
    ram->minoutput = get_API_int(cJSON_GetObjectItem(cp->json,"minoutput"),1);
    ram->min_confirms = cp->min_confirms;
    ram->depositconfirms = get_API_int(cJSON_GetObjectItem(cp->json,"depositconfirms"),ram->min_confirms);
    ram->min_NXTconfirms = MIN_NXTCONFIRMS;
    ram->withdrawconfirms = get_API_int(cJSON_GetObjectItem(cp->json,"withdrawconfirms"),ram->min_NXTconfirms);
    ram->remotemode = get_API_int(cJSON_GetObjectItem(cp->json,"remote"),0);
    ram->multisigchar = cp->multisigchar;
    ram->estblocktime = cp->estblocktime;
    ram->firstiter = 1;
    ram->numgateways = NUM_GATEWAYS;
    if ( ram->numgateways != (sizeof(ram->otherS)/sizeof(*ram->otherS)) )
    {
        printf("expected numgateways.%ld instead of %u\n",(sizeof(ram->otherS)/sizeof(*ram->otherS)),ram->numgateways);
        exit(1);
    }
    ram->S.gatewayid = Global_mp->gatewayid;
    ram->NXTfee_equiv = cp->NXTfee_equiv;
    ram->txfee = cp->txfee;
    ram->NXTconvrate = get_API_float(cJSON_GetObjectItem(cp->json,"NXTconv"));//();
    ram->DEPOSIT_XFER_DURATION = get_API_int(cJSON_GetObjectItem(cp->json,"DEPOSIT_XFER_DURATION"),DEPOSIT_XFER_DURATION);
    if ( Global_mp->iambridge != 0 || (IS_LIBTEST > 0 && is_active_coin(cp->name) > 0) )
    {
        if ( Debuglevel > 0 )
            printf("gatewayid.%d MGWissuer.(%s) init_ramchain_info(%s) (%s) active.%d (%s %s) multisigchar.(%c) confirms.(deposit %d withdraw %d) rate %.8f\n",ram->S.gatewayid,cp->MGWissuer,ram->name,cp->name,is_active_coin(cp->name),ram->serverport,ram->userpass,ram->multisigchar,ram->depositconfirms,ram->withdrawconfirms,ram->NXTconvrate);
        init_ram_MGWconfs(ram,cp->json,(cp->MGWissuer[0] != 0) ? cp->MGWissuer : NXTISSUERACCT,get_NXTasset(&createdflag,Global_mp,cp->assetid));
#ifdef later
        activate_ramchain(ram,cp->name);
#endif
    } //else printf("skip activate ramchains\n");
}
#endif

void init_coinsarray(char *userdir,char *myipaddr)
{
    uint32_t get_blockheight(struct coin_info *cp);
    char *pubNXT,*BTCDaddr,*BTCaddr;
    cJSON *array,*item;
    char coinstr[MAX_JSON_FIELD];
    int32_t i,n;
    struct coin_info *cp;
    array = cJSON_GetObjectItem(MGWconf,"coins");
    if ( array != 0 && is_cJSON_Array(array) != 0 )
    {
        pubNXT = BTCDaddr = BTCaddr = "";
        n = cJSON_GetArraySize(array);
        for (i=0; i<n; i++)
        {
            if ( array == 0 || n == 0 )
                break;
            item = cJSON_GetArrayItem(array,i);
            copy_cJSON(coinstr,cJSON_GetObjectItem(item,"name"));
            if ( coinstr[0] != 0 && (cp= init_coin_info(item,coinstr,userdir)) != 0 )
            {
                if ( Debuglevel > 0 )
                    printf("coinstr.(%s) myip.(%s)\n",coinstr,myipaddr);
                Coin_daemons = realloc(Coin_daemons,sizeof(*Coin_daemons) * (Numcoins+1));
                MGWcoins = realloc(MGWcoins,sizeof(*MGWcoins) * (Numcoins+1));
                MGWcoins[Numcoins] = item;
                Coin_daemons[Numcoins] = cp;
                cp->RTblockheight = get_blockheight(cp);
                if ( Debuglevel > 0 )
                    printf("i.%d coinid.%d %s asset.%s RTheight.%u\n",i,Numcoins,coinstr,Coin_daemons[Numcoins]->assetid,(uint32_t)cp->RTblockheight);
                Numcoins++;
                cp->json = item;
                parse_ipaddr(cp->myipaddr,myipaddr);
                strcpy(cp->name,coinstr);
                if ( strcmp(coinstr,"BTCD") == 0 )
                {
                    if ( DATADIR[0] == 0 )
                        strcpy(DATADIR,"archive");
                    if ( MGWROOT[0] == 0 )
                    {
                        if ( Global_mp->gatewayid >= 0 || Global_mp->iambridge != 0 || Global_mp->isMM != 0 )
                            strcpy(MGWROOT,"/var/www");
                        else strcpy(MGWROOT,".");
                        os_compatible_path(MGWROOT);
                    }
                    //addcontact(Global_mp->myhandle,cp->privateNXTADDR);
                    //addcontact("mypublic",cp->srvNXTADDR);
                }
#ifdef soon
                if ( NORAMCHAINS == 0 )
                    init_ramchain_info(&cp->RAM,cp,get_API_int(cJSON_GetObjectItem(MGWconf,"DEPOSIT_XFER_DURATION"),10),get_API_int(cJSON_GetObjectItem(cp->json,"OLDTX"),strcmp(cp->name,"BTC")));
#endif
            }
        }
    } else printf("no coins array.%p ?\n",array);
}
*/

#endif
#endif
