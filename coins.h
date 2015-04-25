//
//  coins.h
//  gateway
//
//  Created by jl777 on 7/19/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef gateway_coins_h
#define gateway_coins_h
#define DEFINES_ONLY
#include "plugins/includes/cJSON.h"
#include "plugins/utils/files777.c"
#include "plugins/sophia/storage.c"
#undef DEFINES_ONLY

#define NXT_COINID 0
#define BTC_COINID 1
#define LTC_COINID 2
#define CGB_COINID 3
#define DOGE_COINID 4
#define DRK_COINID 5
#define ANC_COINID 6
#define BC_COINID 7
#define BTCD_COINID 8
#define PPC_COINID 9
#define NMC_COINID 10
#define XC_COINID 11
#define VRC_COINID 12
#define ZET_COINID 13
#define QRK_COINID 14
#define RDD_COINID 15
#define XPM_COINID 16
#define FTC_COINID 17
#define CLOAK_COINID 18
#define VIA_COINID 19
#define MEC_COINID 20
#define URO_COINID 21
#define YBC_COINID 22
#define IFC_COINID 23
#define VTC_COINID 24
#define POT_COINID 25
#define KEY_COINID 26
#define FRAC_COINID 27
#define CNL_COINID 28
#define VOOT_COINID 29
#define GML_COINID 30
#define SYNC_COINID 31
#define CRYPT_COINID 32
#define RZR_COINID 33
#define ICB_COINID 34
#define CYC_COINID 35
#define EAC_COINID 36
#define MAX_COINID 37
#define START_COINID 38
#define BBR_COINID 39
#define XMR_COINID 40
#define BTM_COINID 41
#define CHA_COINID 42
#define OPAL_COINID 43
#define BITS_COINID 44
#define VPN_COINID 45

#define BTC_MARKER "17outUgtsnLkguDuXm14tcQ7dMbdD8KZGK"
#define LTC_MARKER "Le9hFCEGKDKp7qYpzfWyEFAq58kSQsjAqX"
#define CGB_MARKER "PTqkPVfNkenMF92ZP8wfMQgQJc9DWZmwpB"
#define DOGE_MARKER "D72Xdw5cVyuX9JLivLxG3V9awpvj7WvsMi"
#define DRK_MARKER "XiiSWYGYozVKg3jyDLfJSF2xbieX15bNU8"
#define ANC_MARKER "D72Xdw5cVyuX9JLivLxG3V9awpvj7WvsMi"
#define BC_MARKER "BPyox1j426KkLy7x2uF3fygkSaM8LKVLY1"
#define BTCD_MARKER "RMMGbxZdav3cRJmWScNVX6BJivn6BNbBE8"
#define PPC_MARKER "PWLZF7Rw5zBGAQz2b84a29AYuk2FDDRd5V"
#define NMC_MARKER "NEkzWvNfccfHH7T1sU2E6FhFdTqBDfn59v"
#define XC_MARKER "XLwwCT4iGevPXqWHXBAe1BNMVLDzNRygFN"
#define VRC_MARKER "VYtKsjtepCpy8TonPNXersfikh2uzGmXeh"
#define ZET_MARKER "ZSyLYZ7X2Mmod4365S7cWcc3gdudjkGeSx"
#define QRK_MARKER "QXHfTKKsWYnSqZaVqYSYrcSyYyvAbeAcoF" // bter
#define RDD_MARKER "RpCQXn6LB4tCekx62DWoSPHbeuRsZdt59e"
#define XPM_MARKER "AJSE7mDNgkkSYnu2XaRoarKojCk687WC9R"
#define FTC_MARKER "6eheBngsQN5iitCiXpMDpSGawmw9TkGck5"
#define CLOAK_MARKER "C3AkZBH3wcTkJ1H7KpZZeMLkkvk73eKYcD"
#define VIA_MARKER "VjRSxRg9gBr4bAWNUgA5871eWa8fJqjFU4"
#define MEC_MARKER "MRpHsm9vV3Mt59e6Ssv4XBQUbXAsJ7GFpV" // bter
#define URO_MARKER "UPrW8bs9G9WohzQgoNCSAyVHJ6YmjytSkW"
#define YBC_MARKER "Ydh2WyYuJa3fkMTLqqfkSkTP5dyk1N7yFV"
#define IFC_MARKER "iB4zLQZKcuwJS7ra9eewbY8ntTKCC2U4Up" // bter
#define VTC_MARKER "VmW1mmJNA3ThX2Ef7yL9jYEVaWuTqaNWne"
#define POT_MARKER "PJjYpxFLCZrU6roseEjeG7q4ket2timHcp"
#define KEY_MARKER "KCqRbcUrCY3PrGZMVgqru2S4Brwtu1Ahha"
#define FRAC_MARKER "Fffoss3zCKP1DteiFaEUZmAYtmw6yvD3ui"
#define CNL_MARKER "119c8uD1rqE6MFBaBetMG8dMFkXA8iWiV"
#define VOOT_MARKER "VPNPpebJfxPcoYzBAxme54FVND9h1rHkYP"
#define GML_MARKER "GapogT2oKxjRRcEbuDGMomM5gyYF3TSw2e"
#define SYNC_MARKER "SRPYSpw2YoKC9Vkdt7sqPjgzfg3LppcBZA"
#define CRYPT_MARKER "ExpiWbZJMSqjaKYXWz2ywVcKQxUsk91fyG"
#define RZR_MARKER "RUZgYrCcrERhYDqShgW1cYX2cE3Maw2bUi"
#define ICB_MARKER "iqhciwDtWyiQhR5SMxYauiXf4sAmFkGXuT"
#define CYC_MARKER "CfNs2kTF6vZ8zsxKc54SYQPksFH87nM9hn"
#define EAC_MARKER "eYyTgezT256CstvUiZ3TobkaY4obhrefsR"
#define MAX_MARKER "mXRm6wDYZFZsPzFwGKSaNuXPczSEfLi67Y"
#define START_MARKER "sXWgrDSE6ugt5uAfJnLWGiN7yYrk97DSNH"
#define BBR_MARKER "1D9hJ1nEjwuhxZMk6fupoTjKLtS2KzkfCQ7kF25k5B6Sc4UJjt9FrvDNYomVd4ZVHv36FskVRJGZa1JZAnZ35GiuAHf7gBy" //"99c9794748071f0a557e8152a99fc47ff1d47b86e056fe85baa50c80b3fd5e5f"
#define XMR_MARKER "47sghzufGhJJDQEbScMCwVBimTuq6L5JiRixD8VeGbpjCTA12noXmi4ZyBZLc99e66NtnKff34fHsGRoyZk3ES1s1V4QVcB" //6e1eb2b0abe3cef6acb62facff7a2f070e40b4898a0d7c0f1f6f09654fc3552b
#define BTM_MARKER "bX4cpvqvfHB87YiDJAHB656TwVZBpD1VR2"
#define CHA_MARKER "1AYNVduuf4X4jLzJ7f9Zpj7Ykz6WYkdS99"
#define OPAL_MARKER "oJwRshRPXxgmntiS44hYQaf9Ahf7o4HT18"
#define BITS_MARKER "BPeZsNiahkKF54YxvSJpeNgSaWbyXo4NPF"
#define VPN_MARKER "Vaw75Sz2YeHbiGygjgGu6LrhP7TJTP5tG8"

int32_t Numcoins;
struct coin_info **Coin_daemons;

struct coin_info *get_coin_info(char *coinstr)
{
    int32_t i;
    for (i=0; i<Numcoins; i++)
        if ( strcmp(coinstr,Coin_daemons[i]->name) == 0 )
            return(Coin_daemons[i]);
    return(0);
}

uint64_t get_accountid(char *buf)
{
    struct coin_info *cp = get_coin_info("BTCD");
    if ( cp != 0 )
        strcpy(buf,cp->srvNXTADDR);
    else strcpy(buf,"nobtcdsrvNXTADDR");
    return(calc_nxt64bits(buf));
}

uint64_t mynxt64bits()
{
    struct coin_info *cp = get_coin_info("BTCD");
    if ( cp != 0 && cp->srvNXTADDR[0] != 0 )
        return(calc_nxt64bits(cp->srvNXTADDR));
    else return(0);
}

char *get_marker(char *coinstr)
{
    struct coin_info *cp;
    if ( (cp= get_coin_info(coinstr)) != 0 && cp->marker[0] != 0 )
        return(cp->marker);
    if ( strcmp(coinstr,"NXT") == 0 )
        return("8712042175100667547"); // NXTprivacy
    else if ( strcmp(coinstr,"BTC") == 0 )
        return(BTC_MARKER);
    else if ( strcmp(coinstr,"BTCD") == 0 )
        return(BTCD_MARKER);
    else if ( strcmp(coinstr,"VRC") == 0 )
        return(VRC_MARKER);
    else if ( strcmp(coinstr,"OPAL") == 0 )
        return(OPAL_MARKER);
    else if ( strcmp(coinstr,"VPN") == 0 )
        return(VPN_MARKER);
    else if ( strcmp(coinstr,"BBR") == 0 )
        return(BBR_MARKER);
    else if ( strcmp(coinstr,"BITS") == 0 )
        return(BITS_MARKER);
    else if ( strcmp(coinstr,"CHA") == 0 )
        return(CHA_MARKER);
    else if ( strcmp(coinstr,"LTC") == 0 )
        return(LTC_MARKER);
    else if ( strcmp(coinstr,"DOGE") == 0 )
        return(DOGE_MARKER);
    else if ( strcmp(coinstr,"DRK") == 0 )
        return(DRK_MARKER);
    else if ( strcmp(coinstr,"PPC") == 0 )
        return(PPC_MARKER);
    else if ( strcmp(coinstr,"NMC") == 0 )
        return(NMC_MARKER);
    /*else if ( strcmp(coinstr,"BTC") == 0 )
     return("177MRHRjAxCZc7Sr5NViqHRivDu1sNwkHZ");
     else if ( strcmp(coinstr,"BTCD") == 0 )
     return("RMMGbxZdav3cRJmWScNVX6BJivn6BNbBE8");
     else if ( strcmp(coinstr,"LTC") == 0 )
     return("LUERp4v5abpTk9jkuQh3KFxc4mFSGTMCiz");
     else if ( strcmp(coinstr,"DRK") == 0 )
     return("XmxSWLPA92QyAXxw2FfYFFex6QgBhadv2Q");*/
    else return(0);
}

void set_legacy_coinid(char *coinstr,int32_t legacyid)
{
    switch ( legacyid )
    {
        case BTC_COINID: strcpy(coinstr,"BTC"); return;
        case LTC_COINID: strcpy(coinstr,"LTC"); return;
        case DOGE_COINID: strcpy(coinstr,"DOGE"); return;
        case BC_COINID: strcpy(coinstr,"BC"); return;
        case VIA_COINID: strcpy(coinstr,"VIA"); return;
        case BTCD_COINID: strcpy(coinstr,"BTCD"); return;
    }
}

/*char *coinid_str(int32_t coinid)
 {
 switch ( coinid )
 {
 case NXT_COINID: return("NXT");
 case BTC_COINID: return("BTC");
 case LTC_COINID: return("LTC");
 case CGB_COINID: return("CGB");
 case DOGE_COINID: return("DOGE");
 case DRK_COINID: return("DRK");
 case ANC_COINID: return("ANC");
 case BC_COINID: return("BC");
 case BTCD_COINID: return("BTCD");
 case PPC_COINID: return("PPC");
 case NMC_COINID: return("NMC");
 case XC_COINID: return("XC");
 case VRC_COINID: return("VRC");
 case ZET_COINID: return("ZET");
 case QRK_COINID: return("QRK");
 case RDD_COINID: return("RDD");
 case XPM_COINID: return("XPM");
 case FTC_COINID: return("FTC");
 case CLOAK_COINID: return("CLOAK");
 case VIA_COINID: return("VIA");
 case MEC_COINID: return("MEC");
 case URO_COINID: return("URO");
 case YBC_COINID: return("YBC");
 case IFC_COINID: return("IFC");
 case VTC_COINID: return("VTC");
 case POT_COINID: return("POT");
 case KEY_COINID: return("KEY");
 case FRAC_COINID: return("FRAC");
 case CNL_COINID: return("CNL");
 case VOOT_COINID: return("VOOT");
 case GML_COINID: return("GML");
 case SYNC_COINID: return("SYNC");
 case CRYPT_COINID: return("CRYPT");
 case RZR_COINID: return("RZR");
 case ICB_COINID: return("ICB");
 case CYC_COINID: return("CYC");
 case EAC_COINID: return("EAC");
 case MAX_COINID: return("MAX");
 case START_COINID: return("START");
 case BBR_COINID: return("BBR");
 case XMR_COINID: return("XMR");
 case BTM_COINID: return("BTM");
 case CHA_COINID: return("CHA");
 case OPAL_COINID: return("OPAL");
 case BITS_COINID: return("BITS");
 case VPN_COINID: return("VPN");
 }
 return(ILLEGAL_COIN);
 }
 
 int32_t conv_coinstr(char *_name)
 {
 int32_t i,coinid;
 char name[256];
 strcpy(name,_name);
 for (i=0; name[i]!=0; i++)
 name[i] = toupper(name[i]);
 for (coinid=0; coinid<64; coinid++)
 if ( strcmp(coinid_str(coinid),name) == 0 )
 return(coinid);
 return(-1);
 }
 
 char *get_backupmarker(char *coinstr)
 {
 int32_t coinid;
 if ( (coinid= conv_coinstr(coinstr)) < 0 )
 return("<no marker>");
 printf("backupmarker.(%s) coinid.%d\n",coinstr,coinid);
 switch ( coinid )
 {
 case NXT_COINID: return("NXT doesnt need a marker");
 case BTC_COINID: return(BTC_MARKER);
 case LTC_COINID: return(LTC_MARKER);
 case CGB_COINID: return(CGB_MARKER);
 case DOGE_COINID: return(DOGE_MARKER);
 case DRK_COINID: return(DRK_MARKER);
 case ANC_COINID: return(ANC_MARKER);
 case BC_COINID: return(BC_MARKER);
 case BTCD_COINID: return(BTCD_MARKER);
 case PPC_COINID: return(PPC_MARKER);
 case NMC_COINID: return(NMC_MARKER);
 case XC_COINID: return(XC_MARKER);
 case VRC_COINID: return(VRC_MARKER);
 case ZET_COINID: return(ZET_MARKER);
 case QRK_COINID: return(QRK_MARKER);
 case RDD_COINID: return(RDD_MARKER);
 case XPM_COINID: return(XPM_MARKER);
 case FTC_COINID: return(FTC_MARKER);
 case CLOAK_COINID: return(CLOAK_MARKER);
 case VIA_COINID: return(VIA_MARKER);
 case MEC_COINID: return(MEC_MARKER);
 case URO_COINID: return(URO_MARKER);
 case YBC_COINID: return(YBC_MARKER);
 case IFC_COINID: return(IFC_MARKER);
 case VTC_COINID: return(VTC_MARKER);
 case POT_COINID: return(POT_MARKER);
 case KEY_COINID: return(KEY_MARKER);
 case FRAC_COINID: return(FRAC_MARKER);
 case CNL_COINID: return(CNL_MARKER);
 case VOOT_COINID: return(VOOT_MARKER);
 case GML_COINID: return(GML_MARKER);
 case SYNC_COINID: return(SYNC_MARKER);
 case CRYPT_COINID: return(CRYPT_MARKER);
 case RZR_COINID: return(RZR_MARKER);
 case ICB_COINID: return(ICB_MARKER);
 case CYC_COINID: return(CYC_MARKER);
 case EAC_COINID: return(EAC_MARKER);
 case MAX_COINID: return(MAX_MARKER);
 case START_COINID: return(START_MARKER);
 case BBR_COINID: return(BBR_MARKER);
 case XMR_COINID: return(XMR_MARKER);
 case BTM_COINID: return(BTM_MARKER);
 case CHA_COINID: return(CHA_MARKER);
 case OPAL_COINID: return(OPAL_MARKER);
 case BITS_COINID: return(BITS_MARKER);
 case VPN_COINID: return(VPN_MARKER);
 }
 return(0);
 }*/

char *get_assetid_str(char *coinstr)
{
    struct coin_info *cp;
    if ( (cp= get_coin_info(coinstr)) != 0 && cp->assetid[0] != 0 )
        return(cp->assetid);
    return(0);
}

uint64_t get_assetidbits(char *coinstr)
{
    struct coin_info *cp;
    if ( (cp= get_coin_info(coinstr)) != 0 && cp->assetid[0] != 0 )
        return(calc_nxt64bits(cp->assetid));
    return(0);
}

struct coin_info *conv_assetid(char *assetid)
{
    int32_t i;
    for (i=0; i<Numcoins; i++)
        if ( strcmp(Coin_daemons[i]->assetid,assetid) == 0 )
            return(Coin_daemons[i]);
    return(0);
}

uint64_t get_orderbook_assetid(char *coinstr)
{
    struct coin_info *cp;
    int32_t i;
    uint64_t virtassetid;
    if ( strcmp(coinstr,"NXT") == 0 )
        return(NXT_ASSETID);
    if ( (cp= get_coin_info(coinstr)) != 0 )
        return(calc_nxt64bits(cp->assetid));
    virtassetid = 0;
    for (i=0; coinstr[i]!=0; i++)
    {
        virtassetid <<= 8;
        virtassetid |= (coinstr[i] & 0xff);
    }
    return(virtassetid);
}

/*int32_t is_gateway_addr(char *addr)
 {
 int32_t i;
 if ( strcmp(addr,NXTISSUERACCT) == 0 )
 return(1);
 for (i=0; i<256; i++)
 {
 if ( Server_NXTaddrs[i][0] == 0 )
 break;
 if ( strcmp(addr,Server_NXTaddrs[i]) == 0 )
 return(1);
 }
 return(0);
 }*/

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

char *extract_userpass(struct coin_info *cp,char *serverport,char *userpass,char *fname)
{
    FILE *fp;
    char line[1024],*rpcuser,*rpcpassword,*str;
    if ( userpass[0] != 0 )
    {
        printf("use: (%s) userpass.(%s)\n",serverport,userpass);
        return(serverport);
    }
    userpass[0] = 0;
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
    }
    else
    {
        printf("extract_userpass cant open.(%s)\n",fname);
        return(0);
    }
    return(serverport);
}

struct coin_info *create_coin_info(int32_t nohexout,int32_t useaddmultisig,int32_t estblocktime,char *name,int32_t minconfirms,uint64_t txfee,int32_t pollseconds,char *asset,char *conf_fname,char *serverport,int32_t blockheight,char *marker,char *marker2,uint64_t NXTfee_equiv,int32_t forkblock,char *userpass)
{
    struct coin_info *cp = calloc(1,sizeof(*cp));
    //char userpass[512];
    if ( forkblock == 0 )
        forkblock = blockheight;
    safecopy(cp->name,name,sizeof(cp->name));
    cp->estblocktime = estblocktime;
    cp->NXTfee_equiv = NXTfee_equiv;
    safecopy(cp->assetid,asset,sizeof(cp->assetid));
    cp->marker = clonestr(marker);
    cp->marker2 = clonestr(marker2);
    cp->blockheight = blockheight;
    cp->min_confirms = minconfirms;
    cp->markeramount = cp->txfee = txfee;
    serverport = extract_userpass(cp,serverport,userpass,conf_fname);
    if ( serverport != 0 )
    {
        cp->serverport = clonestr(serverport);
        cp->userpass = clonestr(userpass);
        if ( Debuglevel > 0 )
            printf("%s userpass.(%s) -> (%s)\n",cp->name,cp->userpass,cp->serverport);
        cp->nohexout = nohexout;
        cp->use_addmultisig = useaddmultisig;
        cp->minconfirms = minconfirms;
        cp->estblocktime = estblocktime;
        cp->txfee = txfee;
        cp->forkheight = forkblock;
        if ( Debuglevel > 0 )
            printf("%s minconfirms.%d txfee %.8f | marker %.8f NXTfee %.8f | firstblock.%ld fork.%d %d seconds\n",cp->name,cp->minconfirms,dstr(cp->txfee),dstr(cp->markeramount),dstr(cp->NXTfee_equiv),(long)cp->blockheight,cp->forkheight,cp->estblocktime);
    }
    else
    {
        free(cp);
        cp = 0;
    }
    return(cp);
}

extern int32_t process_podQ(void *ptr);
struct coin_info *init_coin_info(cJSON *json,char *coinstr,char *userdir)
{
    void add_new_node(uint64_t nxt64bits);
    char *get_telepod_privkey(char **podaddrp,char *pubkey,struct coin_info *cp);
    int32_t i,j,n,useaddmultisig,nohexout,estblocktime,minconfirms,pollseconds,blockheight,forkblock,*cipherids;
    char numstr[MAX_JSON_FIELD],rpcuserpass[512],asset[256],_marker[512],_marker2[512],confstr[512],conf_filename[512],tradebotfname[512],serverip_port[512],buf[512];
    char *marker,*marker2,*privkey,*coinaddr,**privkeys,multisigchar[32];
    cJSON *ciphersobj,*limbo;
    //struct coinaddr *addrp = 0;
    struct nodestats *stats;
    uint64_t txfee,NXTfee_equiv,min_telepod_satoshis,dust,redeemtxid,*limboarray = 0;
    struct coin_info *cp = 0;
    if ( Debuglevel > 0 )
        printf("init_coin.(%s)\n",cJSON_Print(json));
    if ( json != 0 )
    {
        limbo = cJSON_GetObjectItem(json,"limbo");
        if ( limbo != 0 && is_cJSON_Array(limbo) != 0 )
        {
            n = cJSON_GetArraySize(limbo);
            if ( n > 0 )
            {
                limboarray = calloc(n+1,sizeof(*limboarray));
                for (i=j=0; i<n; i++)
                {
                    copy_cJSON(numstr,cJSON_GetArrayItem(limbo,i));
                    if ( (redeemtxid= calc_nxt64bits(numstr)) != 0 )
                    {
                        if ( Debuglevel > 0 )
                            printf("%llu ",(long long)redeemtxid);
                        limboarray[j++] = redeemtxid;
                    }
                }
                if ( Debuglevel > 0 )
                    printf("LIMBO ARRAY of %d of %d redeemtxids\n",j,n);
            }
        }
        nohexout = get_API_int(cJSON_GetObjectItem(json,"nohexout"),1);
        useaddmultisig = get_API_int(cJSON_GetObjectItem(json,"useaddmultisig"),1);
        blockheight = get_API_int(cJSON_GetObjectItem(json,"blockheight"),0);
        forkblock = get_API_int(cJSON_GetObjectItem(json,"forkblock"),0);
        pollseconds = get_API_int(cJSON_GetObjectItem(json,"pollseconds"),60);
        minconfirms = get_API_int(cJSON_GetObjectItem(json,"minconfirms"),10);
        estblocktime = get_API_int(cJSON_GetObjectItem(json,"estblocktime"),300);
        min_telepod_satoshis = get_API_nxt64bits(cJSON_GetObjectItem(json,"min_telepod_satoshis"));
        dust = get_API_nxt64bits(cJSON_GetObjectItem(json,"dust"));
        txfee = get_API_nxt64bits(cJSON_GetObjectItem(json,"txfee_satoshis"));
        if ( txfee == 0 )
            txfee = (uint64_t)(SATOSHIDEN * get_API_float(cJSON_GetObjectItem(json,"txfee")));
        if ( txfee == 0 )
            txfee = 10000;
        NXTfee_equiv = get_API_nxt64bits(cJSON_GetObjectItem(json,"NXTfee_equiv_satoshis"));
        if ( NXTfee_equiv == 0 )
            NXTfee_equiv = (uint64_t)(SATOSHIDEN * get_API_float(cJSON_GetObjectItem(json,"NXTfee_equiv")));
        extract_cJSON_str(_marker,sizeof(_marker),json,"marker");
        extract_cJSON_str(_marker2,sizeof(_marker2),json,"marker2");
        if ( _marker[0] == 0 )
            marker = get_marker(coinstr);
        else marker = _marker;
        if ( _marker2[0] == 0 )
            marker2 = get_marker(coinstr);
        else marker2 = _marker2;
        if ( //marker != 0 && txfee != 0. && NXTfee_equiv != 0. &&
            extract_cJSON_str(confstr,sizeof(confstr),json,"conf") > 0 &&
            extract_cJSON_str(asset,sizeof(asset),json,"asset") > 0 &&
            extract_cJSON_str(serverip_port,sizeof(serverip_port),json,"rpc") > 0 )
        {
            extract_cJSON_str(rpcuserpass,sizeof(rpcuserpass),json,"rpcuserpass");
            if ( userdir[0] != 0 )
                sprintf(conf_filename,"%s/%s",userdir,confstr);
            else strcpy(conf_filename,confstr);
            cp = create_coin_info(nohexout,useaddmultisig,estblocktime,coinstr,minconfirms,txfee,pollseconds,asset,os_compatible_path(conf_filename),serverip_port,blockheight,marker,marker2,NXTfee_equiv,forkblock,rpcuserpass);
            if ( cp != 0 )
            {
                portable_mutex_init(&cp->consensus_mutex);
                extract_cJSON_str(cp->myipaddr,sizeof(cp->myipaddr),json,"myipaddr");
                extract_cJSON_str(cp->MGWissuer,sizeof(cp->MGWissuer),json,"issuer");
                if ( cp->MGWissuer[0] == 'N' && cp->MGWissuer[1] == 'X' && cp->MGWissuer[2] == 'T' )
                    expand_nxt64bits(cp->MGWissuer,conv_rsacctstr(cp->MGWissuer,0));
                if ( Debuglevel > 0 )
                    printf("MGW issuer.(%s) marker.(%s)\n",cp->MGWissuer,cp->marker!=0?cp->marker:"no marker");
                cp->RAM.limboarray = limboarray;
                if ( strcmp(cp->name,"BTCD") == 0 )
                {
                    extract_cJSON_str(cp->srvNXTACCTSECRET,sizeof(cp->srvNXTACCTSECRET),MGWconf,"secret");
                    Global_mp->Lfactor = (int32_t)get_API_int(cJSON_GetObjectItem(json,"Lfactor"),1);
                    if ( Global_mp->Lfactor > MAX_LFACTOR )
                        Global_mp->Lfactor = MAX_LFACTOR;
                    cp->srvport = get_API_int(cJSON_GetObjectItem(json,"srvport"),SUPERNET_PORT);
                    cp->bridgeport = get_API_int(cJSON_GetObjectItem(json,"bridgeport"),0);
                    extract_cJSON_str(cp->bridgeipaddr,sizeof(cp->bridgeipaddr),json,"bridgeipaddr");
                }
                if ( extract_cJSON_str(tradebotfname,sizeof(tradebotfname),json,"tradebotfname") > 0 )
                    cp->tradebotfname = clonestr(tradebotfname);
                if ( extract_cJSON_str(cp->privacyserver,sizeof(cp->privacyserver),json,"privacyServer") > 0 )
                    if ( Debuglevel > 0 )
                        printf("set default privacyServer to (%s)\n",cp->privacyserver);
                if ( extract_cJSON_str(cp->privateaddr,sizeof(cp->privateaddr),json,"privateaddr") > 0 || extract_cJSON_str(cp->privateaddr,sizeof(cp->privateaddr),json,"pubaddr") > 0 )
                {
                    if ( strcmp(cp->privateaddr,"privateaddr") == 0 )
                    {
                        char *str; int32_t allocsize;
                        if ( (str= loadfile(&allocsize,"privateaddr")) != 0 )
                        {
                            strcpy(cp->privateaddr,str);
                            free(str);
                        }
                    }
                    coinaddr = cp->privateaddr;
                    if ( (privkey= get_telepod_privkey(&coinaddr,cp->coinpubkey,cp)) != 0 )
                    {
                        if ( Debuglevel > 0 )
                            printf("copy key <- (%s)\n",privkey);
                        safecopy(cp->privateNXTACCTSECRET,privkey,sizeof(cp->privateNXTACCTSECRET));
                        cp->privatebits = issue_getAccountId(0,privkey);
                        expand_nxt64bits(cp->privateNXTADDR,cp->privatebits);
                        conv_NXTpassword(Global_mp->myprivkey.bytes,Global_mp->mypubkey.bytes,(uint8_t *)cp->privateNXTACCTSECRET,(int32_t)strlen(cp->privateNXTACCTSECRET));
                        if ( Debuglevel > 2 )
                            printf("SET ACCTSECRET for %s.%s to %s NXT.%llu\n",cp->name,cp->privateaddr,cp->privateNXTACCTSECRET,(long long)cp->privatebits);
                        free(privkey);
                        if ( (stats = get_nodestats(cp->privatebits)) != 0 )
                        {
                            add_new_node(cp->privatebits);
                            memcpy(stats->pubkey,Global_mp->mypubkey.bytes,sizeof(stats->pubkey));
                            //conv_NXTpassword(Global_mp->private_privkey,Global_mp->private_pubkey,cp->privateNXTACCTSECRET);
                        } else { printf("null return from get_nodestats in init_coin_info\n"); exit(1); }
                    }
                    if ( extract_cJSON_str(cp->srvpubaddr,sizeof(cp->srvpubaddr),json,"srvpubaddr") > 0 )
                    {
                        coinaddr = cp->srvpubaddr;
                        if ( cp->srvNXTACCTSECRET[0] == 0 )
                        {
                            if ( strcmp("randvals",cp->srvpubaddr) != 0 && (privkey= get_telepod_privkey(&coinaddr,cp->srvcoinpubkey,cp)) != 0 )
                            {
                                safecopy(cp->srvNXTACCTSECRET,privkey,sizeof(cp->srvNXTACCTSECRET));
                                free(privkey);
                            }
                            else
                            {
                                gen_randomacct(0,33,cp->srvNXTADDR,cp->srvNXTACCTSECRET,"randvals");
                                cp->srvpubnxtbits = calc_nxt64bits(cp->srvNXTADDR);
                            }
                        }
                        if ( strcmp("BTCD",cp->name) == 0 )
                            Global_mp->srvNXTACCTSECRET = cp->srvNXTACCTSECRET;
                        cp->srvpubnxtbits = issue_getAccountId(0,cp->srvNXTACCTSECRET);
                        expand_nxt64bits(cp->srvNXTADDR,cp->srvpubnxtbits);
                        if ( Debuglevel > 2 )
                            printf("SET ACCTSECRET for %s.%s to %s NXT.%llu\n",cp->name,cp->srvpubaddr,cp->srvNXTACCTSECRET,(long long)cp->srvpubnxtbits);
                        conv_NXTpassword(Global_mp->loopback_privkey,Global_mp->loopback_pubkey,cp->srvNXTACCTSECRET,(int32_t)strlen(cp->srvNXTACCTSECRET));
                        init_hexbytes_noT(Global_mp->pubkeystr,Global_mp->loopback_pubkey,sizeof(Global_mp->loopback_pubkey));
                        //printf("SRV pubaddr.(%s) secret.(%s) -> %llu\n",cp->srvpubaddr,cp->srvNXTACCTSECRET,(long long)cp->srvpubnxtbits);
                        if ( (stats= get_nodestats(cp->srvpubnxtbits)) != 0 )
                        {
                            stats->ipbits = calc_ipbits(cp->myipaddr);
                            add_new_node(cp->srvpubnxtbits);
                            memcpy(stats->pubkey,Global_mp->loopback_pubkey,sizeof(stats->pubkey));
                        } else { printf("null return from get_nodestats in init_coin_info B\n"); exit(1); }
                    }
                }
                else if ( IS_LIBTEST > 0 && strcmp(cp->name,"BTCD") == 0 )
                {
                    char args[1024],*addr,*pubaddr,*srvpubaddr;
                    sprintf(args,"[\"funding\"]");
                    addr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"getaccountaddress",args);
                    sprintf(args,"[\"pubaddr\"]");
                    pubaddr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"getaccountaddress",args);
                    sprintf(args,"[\"srvpubaddr\"]");
                    srvpubaddr = bitcoind_RPC(0,cp->name,cp->serverport,cp->userpass,"getaccountaddress",args);
                    fprintf(stderr,"Withdraw directly from an exchange to your funding address %s\nAdd the following to jl777.conf: \"pubaddr\":\"%s\",\"srvpubaddr\":\"%s\"\n",addr,pubaddr,srvpubaddr);
                    exit(1);
                }
                if ( (cp->min_telepod_satoshis= min_telepod_satoshis) == 0 )
                    cp->min_telepod_satoshis = (dust == 0) ? SATOSHIDEN/10000 : dust;
                if ( dust == 0 )
                    dust = 10000;
                cp->dust = dust;
                if ( extract_cJSON_str(multisigchar,sizeof(multisigchar),json,"multisigchar") > 0 )
                    cp->multisigchar = multisigchar[0];
                cp->maxevolveiters = get_API_int(cJSON_GetObjectItem(json,"maxevolveiters"),100);
                cp->M = get_API_int(cJSON_GetObjectItem(json,"telepod_M"),1);
                cp->N = get_API_int(cJSON_GetObjectItem(json,"telepod_N"),1);
                cp->clonesmear = get_API_int(cJSON_GetObjectItem(json,"clonesmear"),3600);
                cp->clonesmear += (((rand() >> 8) % ((3+cp->clonesmear)/2)) - (cp->clonesmear/4));
                if ( cp->clonesmear < 600 )
                    cp->clonesmear = 600;
                if ( extract_cJSON_str(cp->backupdir,sizeof(cp->backupdir),json,"backupdir") <= 0 )
                    strcpy(cp->backupdir,"backups");
                ciphersobj = cJSON_GetObjectItem(json,"ciphers");
                privkeys = 0;
                cipherids = 0;
                if ( ciphersobj == 0 || (privkeys= validate_ciphers(&cipherids,cp,ciphersobj)) == 0 )
                {
                    free_cipherptrs(ciphersobj,privkeys,cipherids);
                    sprintf(buf,"[{\"aes\":\"%s\"}]",cp->privateaddr);
                    cp->ciphersobj = cJSON_Parse(buf);
                } else cp->ciphersobj = ciphersobj;
                free_cipherptrs(0,privkeys,cipherids);
                privkeys = 0;
                cipherids = 0;
                if ( IS_LIBTEST > 0 && strcmp(cp->name,"BTCD") == 0 && (privkeys= validate_ciphers(&cipherids,cp,cp->ciphersobj)) == 0 )
                {
                    fprintf(stderr,"FATAL error: cant validate ciphers sequence for %s\n",cp->name);
                    exit(-1);
                }
                free_cipherptrs(0,privkeys,cipherids);
            }
            else printf("create_coin_info failed for (%s)\n",coinstr);
        }
    }
    return(cp);
}

int32_t is_whitelisted(char *ipaddr)
{
    int32_t i,n;
    cJSON *array;
    char str[MAX_JSON_FIELD];
    array = cJSON_GetObjectItem(MGWconf,"whitelist");
    if ( array != 0 && is_cJSON_Array(array) != 0 )
    {
        n = cJSON_GetArraySize(array);
        for (i=0; i<n; i++)
        {
            if ( array == 0 || n == 0 )
                break;
            copy_cJSON(str,cJSON_GetArrayItem(array,i));
            if ( strcmp(str,ipaddr) == 0 )
                return(1);
        }
    }
    return(0);
}

int32_t is_active_coin(char *coinstr)
{
    int32_t i,n;
    cJSON *array;
    char str[MAX_JSON_FIELD];
    array = cJSON_GetObjectItem(MGWconf,"active");
    if ( array != 0 && is_cJSON_Array(array) != 0 )
    {
        n = cJSON_GetArraySize(array);
        for (i=0; i<n; i++)
        {
            if ( array == 0 || n == 0 )
                break;
            copy_cJSON(str,cJSON_GetArrayItem(array,i));
            if ( strcmp(str,coinstr) == 0 )
                return(i+1);
        }
    }
    return(0);
}

int32_t is_enabled_command(char *command)
{
    int32_t i,n;
    cJSON *array;
    char str[MAX_JSON_FIELD];
    array = cJSON_GetObjectItem(MGWconf,"commands");
    if ( array != 0 && is_cJSON_Array(array) != 0 )
    {
        n = cJSON_GetArraySize(array);
        for (i=0; i<n; i++)
        {
            if ( array == 0 || n == 0 )
                break;
            copy_cJSON(str,cJSON_GetArrayItem(array,i));
            if ( strcmp(str,command) == 0 )
                return(i+1);
            //fprintf(stderr,"%s ",str);
        }
    }
    //fprintf(stderr,"%s not in commands[]\n",command);
    return(0);
}

void init_Specialaddrs()
{
    cJSON *array,*item;
    int32_t i,n;
    char NXTADDR[MAX_JSON_FIELD];
    array = cJSON_GetObjectItem(MGWconf,"special_NXTaddrs");
    if ( array != 0 && is_cJSON_Array(array) != 0 ) // first three must be the gateway's addresses
    {
        n = cJSON_GetArraySize(array);
        for (i=0; i<n; i++)
        {
            if ( array == 0 || n == 0 )
                break;
            item = cJSON_GetArrayItem(array,i);
            copy_cJSON(NXTADDR,item);
            if ( NXTADDR[0] == 0 )
            {
                fprintf(stderr,"Illegal special NXTaddr.%d\n",i);
                exit(1);
            }
            if ( Debuglevel > 0 )
                printf("%s ",NXTADDR);
            strcpy(Server_NXTaddrs[i],NXTADDR);
            if ( i < 3 )
            {
                void bind_NXT_ipaddr(uint64_t nxt64bits,char *ip_port);
                bind_NXT_ipaddr(calc_nxt64bits(NXTADDR),Server_ipaddrs[i]);
            }
            MGW_blacklist[i] = MGW_whitelist[i] = clonestr(NXTADDR);
        }
        if ( Debuglevel > 0 )
            printf("special_addrs.%d\n",n);
    }
    else
    {
        n = 0;
        MGW_whitelist[n++] = "423766016895692955";
        MGW_whitelist[n++] = "12240549928875772593";
        MGW_whitelist[n++] = "8279528579993996036";
        MGW_whitelist[n++] = "13434315136155299987";
        MGW_whitelist[n++] = "10694781281555936856";
        MGW_whitelist[n++] = "7581814105672729429";
        MGW_whitelist[n++] = "15467703240466266079";
    }
    MGW_blacklist[n] = MGW_whitelist[n] = NXTISSUERACCT, n++;
    MGW_blacklist[n] = MGW_whitelist[n] = GENESISACCT, n++;
    MGW_whitelist[n] = "";
    MGW_blacklist[n++] = "4551058913252105307";    // from accidental transfer
    MGW_blacklist[n++] = "";
}

int32_t is_trusted_issuer(char *issuer)
{
    int32_t i,n;
    cJSON *array;
    uint64_t nxt64bits;
    char str[MAX_JSON_FIELD];
    array = cJSON_GetObjectItem(MGWconf,"issuers");
    if ( array != 0 && is_cJSON_Array(array) != 0 )
    {
        n = cJSON_GetArraySize(array);
        for (i=0; i<n; i++)
        {
            if ( array == 0 || n == 0 )
                break;
            copy_cJSON(str,cJSON_GetArrayItem(array,i));
            if ( str[0] == 'N' && str[1] == 'X' && str[2] == 'T' )
            {
                nxt64bits = conv_rsacctstr(str,0);
                printf("str.(%s) -> %llu\n",str,(long long)nxt64bits);
                expand_nxt64bits(str,nxt64bits);
            }
            if ( strcmp(str,issuer) == 0 )
                return(1);
        }
    }
    return(0);
}

void init_SuperNET_whitelist()
{
    cJSON *array,*item;
    int32_t i,n;
    char ipaddr[MAX_JSON_FIELD];
    array = cJSON_GetObjectItem(MGWconf,"whitelist");
    if ( array != 0 && is_cJSON_Array(array) != 0 )
    {
        int32_t add_SuperNET_whitelist(char *ipaddr);
        n = cJSON_GetArraySize(array);
        for (i=0; i<n; i++)
        {
            if ( array == 0 || n == 0 )
                break;
            item = cJSON_GetArrayItem(array,i);
            copy_cJSON(ipaddr,item);
            add_SuperNET_whitelist(ipaddr);
        }
    }
}

#ifdef soon
void init_ram_MGWconfs(struct ramchain_info *ram,cJSON *confjson,char *MGWredemption,struct NXT_asset *ap)
{
    cJSON *array,*item;
    int32_t i,n,hasredemption = 0;
    char NXTADDR[MAX_JSON_FIELD];
    if ( (ram->ap= ap) == 0 )
    {
        printf("no asset for %s\n",ram->name);
        return;
    }
    ram->MGWbits = calc_nxt64bits(MGWredemption);
    if ( Debuglevel > 0 )
        printf("init_ram_MGWconfs.(%s) -> %llu\n",MGWredemption,(long long)ram->MGWbits);
    array = cJSON_GetObjectItem(confjson,"special_NXTaddrs");
    if ( array != 0 && is_cJSON_Array(array) != 0 ) // first three must be the gateway's addresses
    {
        n = cJSON_GetArraySize(array);
        ram->special_NXTaddrs = calloc(n+2,sizeof(*ram->special_NXTaddrs));
        for (i=0; i<n; i++)
        {
            if ( array == 0 || n == 0 )
                break;
            item = cJSON_GetArrayItem(array,i);
            copy_cJSON(NXTADDR,item);
            if ( NXTADDR[0] == 0 )
            {
                fprintf(stderr,"Illegal special NXTaddr.%d\n",i);
                exit(1);
            }
            ram->special_NXTaddrs[i] = clonestr(NXTADDR);
            if ( strcmp(NXTADDR,MGWredemption) == 0 )
                hasredemption = 1;
        }
        if ( Debuglevel > 0 )
            printf("special_addrs.%d\n",n);
        if ( hasredemption == 0 )
            ram->special_NXTaddrs[i++] = clonestr(MGWredemption);
        ram->special_NXTaddrs[i++] = clonestr(GENESISACCT);
        n = i;
    }
    else
    {
        for (n=0; MGW_whitelist[n][0]!=0; n++)
            ;
        ram->special_NXTaddrs = calloc(n,sizeof(*ram->special_NXTaddrs));
        for (i=0; i<n; i++)
            ram->special_NXTaddrs[i] = clonestr(MGW_whitelist[i]);
    }
    ram->numspecials = n;
    if ( Debuglevel > 0 )
    {
        for (i=0; i<n; i++)
            printf("(%s) ",ram->special_NXTaddrs[i]);
        printf("numspecials.%d\n",ram->numspecials);
    }
    if ( ram->limboarray == 0 )
        ram->limboarray = calloc(2,sizeof(*ram->limboarray));
    if ( Debuglevel > 0 )
    {
        for (i=0; ram->limboarray[i]!=0&&ram->limboarray[i]!=0; i++)
            printf("%llu ",(long long)ram->limboarray[i]);
        printf("limboarray.%d\n",i);
    }
}

struct ramchain_info *get_ramchain_info(char *coinstr)
{
    struct coin_info *cp = get_coin_info(coinstr);
    if ( NORAMCHAINS == 0 && cp != 0 )
        return(&cp->RAM);
    else return(0);
}

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

void init_confcontacts()
{
    cJSON *array,*item;
    char handle[MAX_JSON_FIELD],acct[MAX_JSON_FIELD];
    int32_t i,n;
    array = (IS_LIBTEST > 0) ? cJSON_GetObjectItem(MGWconf,"contacts") : 0;
    if ( array != 0 && is_cJSON_Array(array) != 0 ) // first three must be the gateway's addresses
    {
        n = cJSON_GetArraySize(array);
        printf("scanning %d contacts\n",n);
        for (i=0; i<n; i++)
        {
            if ( array == 0 || n == 0 )
                break;
            item = cJSON_GetArrayItem(array,i);
            if ( is_cJSON_Array(item) != 0 && cJSON_GetArraySize(item) == 2 ) // must be ["handle","<acct>"]
            {
                copy_cJSON(handle,cJSON_GetArrayItem(item,0));
                copy_cJSON(acct,cJSON_GetArrayItem(item,1));
                if ( handle[0] != 0 && acct[0] != 0 )
                {
                    printf("addcontact (%s) <-> (%s)\n",handle,acct);
                    //retstr = addcontact(handle,acct);
                    //if ( retstr != 0 )
                    //   free(retstr);
                }
            }
        }
        printf("contacts.%d\n",n);
    }
}

void init_legacyMGW(char *myipaddr)
{
    int32_t i,ismainnet;
    ismainnet = get_API_int(cJSON_GetObjectItem(MGWconf,"MAINNET"),1);
    extract_cJSON_str(NXTAPIURL,sizeof(NXTAPIURL),MGWconf,"NXTAPIURL");
    if ( NXTAPIURL[0] == 0 )
    {
        if ( USESSL == 0 )
            strcpy(NXTAPIURL,"http://127.0.0.1:");
        else strcpy(NXTAPIURL,"https://127.0.0.1:");
        if ( ismainnet != 0 )
            strcat(NXTAPIURL,"7876/nxt");
        else strcat(NXTAPIURL,"6876/nxt");
    }
    strcpy(NXTSERVER,NXTAPIURL);
    strcat(NXTSERVER,"?requestType");
    if ( ismainnet != 0 )
    {
        NXT_FORKHEIGHT = 173271;
        if ( NXTISSUERACCT[0] == 0 )
            strcpy(NXTISSUERACCT,"7117166754336896747");
        //origblock = "14398161661982498695";    //"91889681853055765";//"16787696303645624065";
    }
    else
    {
        DGSBLOCK = 0;
        if ( NXTISSUERACCT[0] == 0 )
            strcpy(NXTISSUERACCT,"18232225178877143084");
        //origblock = "16787696303645624065";   //"91889681853055765";//"16787696303645624065";
    }
    //if ( ORIGBLOCK[0] == 0 )
    //   strcpy(ORIGBLOCK,origblock);
    extract_cJSON_str(Server_ipaddrs[0],sizeof(Server_ipaddrs[0]),MGWconf,"MGW0_ipaddr");
    if ( Server_ipaddrs[0][0] == 0 )
        strcpy(Server_ipaddrs[0],MGW0_IPADDR);
    extract_cJSON_str(Server_ipaddrs[1],sizeof(Server_ipaddrs[1]),MGWconf,"MGW1_ipaddr");
    if ( Server_ipaddrs[1][0] == 0 )
        strcpy(Server_ipaddrs[1],MGW1_IPADDR);
    extract_cJSON_str(Server_ipaddrs[2],sizeof(Server_ipaddrs[2]),MGWconf,"MGW2_ipaddr");
    if ( Server_ipaddrs[2][0] == 0 )
        strcpy(Server_ipaddrs[2],MGW2_IPADDR);
    extract_cJSON_str(Server_ipaddrs[3],sizeof(Server_ipaddrs[3]),MGWconf,"BRIDGE_ipaddr");
    if ( Server_ipaddrs[3][0] == 0 )
        strcpy(Server_ipaddrs[3],"76.176.198.6");
    
    // extract_cJSON_str(NXTACCTSECRET,sizeof(NXTACCTSECRET),MGWconf,"secret");
    Global_mp->gatewayid = -1;
    for (i=0; i<3; i++)
    {
        Global_mp->gensocks[i] = -1;
        if ( strcmp(myipaddr,Server_ipaddrs[i]) == 0 )
            Global_mp->gatewayid = i;
        if ( Debuglevel > 0 )
            printf("%s | ",Server_ipaddrs[i]);
    }
    if ( Global_mp->gatewayid < 0 )
        Global_mp->gatewayid = get_API_int(cJSON_GetObjectItem(MGWconf,"gatewayid"),Global_mp->gatewayid);
    if ( Global_mp->gatewayid >= 0 )
        strcpy(myipaddr,Server_ipaddrs[Global_mp->gatewayid]), NORAMCHAINS = 0;
}

void init_tradebots_conf(cJSON *MGWconf)
{
   /* int32_t writeflag = 1;
    void start_polling_exchanges(int32_t exchangeflag);
    int32_t init_exchanges(cJSON *confobj,int32_t exchangeflag);
    if ( init_exchanges(MGWconf,writeflag) > 0 )
        start_polling_exchanges(writeflag);
    int32_t init_tradebots(cJSON *languagesobj);
     if ( didinit == 0 )
     {
     languagesobj = cJSON_GetObjectItem(MGWconf,"tradebot_languages");
     init_tradebots(languagesobj);
      }*/
}

void init_SuperNET_settings(char *userdir)
{
    uint64_t nxt64bits;
    extract_cJSON_str(userdir,MAX_JSON_FIELD,MGWconf,"userdir");
    Global_mp->isMM = get_API_int(cJSON_GetObjectItem(MGWconf,"MMatrix"),0);
    if ( (Global_mp->iambridge = get_API_int(cJSON_GetObjectItem(MGWconf,"isbridge"),0)) != 0 && Debuglevel > 0 )
        printf("I AM A BRIDGE\n");
    if ( extract_cJSON_str(Global_mp->myhandle,sizeof(Global_mp->myhandle),MGWconf,"myhandle") <= 0 )
        strcpy(Global_mp->myhandle,"myhandle");
    if ( extract_cJSON_str(PRICEDIR,sizeof(PRICEDIR),MGWconf,"PRICEDIR") <= 0 )
        strcpy(PRICEDIR,"./prices");
    os_compatible_path(PRICEDIR);
    init_jdatetime(NXT_GENESISTIME,get_API_int(cJSON_GetObjectItem(MGWconf,"timezone"),0) * 3600);
    MIN_NQTFEE = get_API_int(cJSON_GetObjectItem(MGWconf,"MIN_NQTFEE"),(int32_t)MIN_NQTFEE);
    PERMUTE_RAWINDS = get_API_int(cJSON_GetObjectItem(MGWconf,"PERMUTE"),0);
    MIN_NXTCONFIRMS = get_API_int(cJSON_GetObjectItem(MGWconf,"MIN_NXTCONFIRMS"),MIN_NXTCONFIRMS);
    GATEWAY_SIG = get_API_int(cJSON_GetObjectItem(MGWconf,"GATEWAY_SIG"),0);
    FIRST_NXTBLOCK = get_API_int(cJSON_GetObjectItem(MGWconf,"FIRST_NXTBLOCK"),0);
    extract_cJSON_str(ORIGBLOCK,sizeof(ORIGBLOCK),MGWconf,"ORIGBLOCK");
    if ( ORIGBLOCK[0] != 0 || FIRST_NXTBLOCK != 0 )
        FIRST_NXTTIMESTAMP = get_NXTtimestamp(ORIGBLOCK,FIRST_NXTBLOCK);
    extract_cJSON_str(NXTISSUERACCT,sizeof(NXTISSUERACCT),MGWconf,"NXTISSUERACCT");
    extract_cJSON_str(WEBSOCKETD,sizeof(WEBSOCKETD),MGWconf,"WEBSOCKETD");
    if ( WEBSOCKETD[0] == 0 )
        strcpy(WEBSOCKETD,"/usr/bin/websocketd");
    if ( NXTISSUERACCT[0] == 'N' && NXTISSUERACCT[1] == 'X' && NXTISSUERACCT[2] == 'T' )
    {
        nxt64bits = conv_rsacctstr(NXTISSUERACCT,0);
        if ( Debuglevel > 0 )
            printf("NXTISSUERACCT.(%s) -> %llu\n",NXTISSUERACCT,(long long)nxt64bits);
        expand_nxt64bits(NXTISSUERACCT,nxt64bits);
    }
    extract_cJSON_str(DATADIR,sizeof(NXTISSUERACCT),MGWconf,"DATADIR");
    extract_cJSON_str(MGWROOT,sizeof(MGWROOT),MGWconf,"MGWROOT");
    extract_cJSON_str(SOPHIA_DIR,sizeof(SOPHIA_DIR),MGWconf,"SOPHIA_DIR");
    if ( SOPHIA_DIR[0] == 0 )
        strcpy(SOPHIA_DIR,"./DB");
    os_compatible_path(SOPHIA_DIR);
    if ( IS_LIBTEST >= 0 )
        IS_LIBTEST = get_API_int(cJSON_GetObjectItem(MGWconf,"LIBTEST"),1);
    MULTITHREADS = get_API_int(cJSON_GetObjectItem(MGWconf,"MULTITHREADS"),0);
    SOFTWALL = get_API_int(cJSON_GetObjectItem(MGWconf,"SOFTWALL"),0);
    MAP_HUFF = get_API_int(cJSON_GetObjectItem(MGWconf,"MAP_HUFF"),1);
    FASTMODE = get_API_int(cJSON_GetObjectItem(MGWconf,"FASTMODE"),1);
    SERVER_PORT = get_API_int(cJSON_GetObjectItem(MGWconf,"SERVER_PORT"),3000);
    SUPERNET_PORT = get_API_int(cJSON_GetObjectItem(MGWconf,"SUPERNET_PORT"),_SUPERNET_PORT);
    APIPORT = get_API_int(cJSON_GetObjectItem(MGWconf,"APIPORT"),SUPERNET_PORT);
    DBSLEEP = get_API_int(cJSON_GetObjectItem(MGWconf,"DBSLEEP"),100);
    MAX_BUYNXT = get_API_int(cJSON_GetObjectItem(MGWconf,"MAX_BUYNXT"),10);
    THROTTLE = get_API_int(cJSON_GetObjectItem(MGWconf,"THROTTLE"),0);
    QUOTE_SLEEP = get_API_int(cJSON_GetObjectItem(MGWconf,"QUOTE_SLEEP"),10);
    EXCHANGE_SLEEP = get_API_int(cJSON_GetObjectItem(MGWconf,"EXCHANGE_SLEEP"),5);
    APISLEEP = get_API_int(cJSON_GetObjectItem(MGWconf,"APISLEEP"),10);
    NORAMCHAINS = get_API_int(cJSON_GetObjectItem(MGWconf,"NORAMCHAINS"),1);
    ENABLE_EXTERNALACCESS = get_API_int(cJSON_GetObjectItem(MGWconf,"EXTERNALACCESS"),0);
    
    /*#ifndef HUFF_GENMODE
     DBSLEEP *= 10;
     APISLEEP *= 10;
     #endif*/
    USESSL = get_API_int(cJSON_GetObjectItem(MGWconf,"USESSL"),0);
    UPNP = get_API_int(cJSON_GetObjectItem(MGWconf,"UPNP"),1);
    LOG2_MAX_XFERPACKETS = get_API_int(cJSON_GetObjectItem(MGWconf,"LOG2_MAXPACKETS"),3);
    MULTIPORT = get_API_int(cJSON_GetObjectItem(MGWconf,"MULTIPORT"),0);
    ENABLE_GUIPOLL = get_API_int(cJSON_GetObjectItem(MGWconf,"GUIPOLL"),1);
    if ( Debuglevel >= 0 )
        Debuglevel = get_API_int(cJSON_GetObjectItem(MGWconf,"debug"),Debuglevel);
}

#ifdef __APPLE__
#include "nn.h"
#include "bus.h"
#else
#include "includes/nn.h"
#include "includes/bus.h"
#endif

int32_t nano_socket(char *ipaddr,int32_t port)
{
    int32_t sock,err,to = 1;
    char tcpaddr[64];
    sprintf(tcpaddr,"tcp://%s:%d",ipaddr,port);
    if ( (sock= nn_socket(AF_SP,NN_BUS)) < 0 )
    {
        printf("error %d nn_socket err.%s\n",sock,nn_strerror(nn_errno()));
        return(0);
    }
    printf("got sock.%d\n",sock);
    if ( (err= nn_bind(sock,tcpaddr)) < 0 )
    {
        printf("error %d nn_bind.%d (%s) | %s\n",err,sock,tcpaddr,nn_strerror(nn_errno()));
        return(0);
    }
    assert (nn_setsockopt(sock,NN_SOL_SOCKET,NN_RCVTIMEO,&to,sizeof (to)) >= 0);
    printf("bound\n");
    return(sock);
}

void broadcastfile(char *NXTaddr,char *NXTACCTSECRET,char *fname)
{
    FILE *fp;
    char *buf;
    int32_t len,n;
    if ( Global_mp->bussock >= 0 && (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
    {
        fseek(fp,0,SEEK_END);
        len = (int32_t)ftell(fp);
        rewind(fp);
        if ( len > 0 )
        {
            buf = malloc(len + strlen(fname) + 1);
            strcpy(buf,fname);
            if ( (n= (int32_t)fread(buf + strlen(fname) + 1,1,len,fp)) == len )
            {
                nn_send(Global_mp->bussock,buf,len + strlen(fname) + 1,0);
                printf("send (%s).%d to bus\n",fname,len);
            } else printf("read error (%s) got %d vs %d\n",fname,n,len);
            free(buf);
        }
        fclose(fp);
    }
}

void poll_nanomsg()
{
    FILE *fp;
    char fname[1024];
    int32_t recv,filelen,i,n,len,sameflag = 0;
    char *buf,*filebuf;
    if ( Global_mp->bussock < 0 )
        return;
    if ( (recv= nn_recv(Global_mp->bussock,&buf,NN_MSG,0)) >= 0 )
    {
        sprintf(fname,"%s",buf);
        len = (int32_t)strlen(buf) + 1;
        if ( len < recv )
        {
            filebuf = &buf[len];
            filelen = (recv - len);
            if ( (fp= fopen(os_compatible_path(fname),"rb")) != 0 )
            {
                fseek(fp,0,SEEK_END);
                if ( ftell(fp) == filelen )
                {
                    rewind(fp);
                    for (i=0; i<filelen; i++)
                        if ( (fgetc(fp) & 0xff) != (filebuf[i] & 0xff) )
                            break;
                    if ( i == filelen )
                        sameflag = 1;
                }
                fclose(fp);
            }
            if ( sameflag == 0 && (fp= fopen(os_compatible_path(fname),"wb")) != 0 )
            {
                if ( (n= (int32_t)fwrite(filebuf,1,filelen,fp)) != filelen )
                    printf("error writing (%s) only %d written vs %d\n",fname,n,filelen);
                fclose(fp);
            }
        }
        printf ("RECEIVED (%s).%d FROM BUS -> (%s) sameflag.%d\n",buf,recv,fname,sameflag);
        nn_freemsg(buf);
    }
}
/*
int sz_n = strlen(argv[1]) + 1; // '\0' too
printf ("%s: SENDING '%s' ONTO BUS\n", argv[1], argv[1]);
int send = nn_send (sock, argv[1], sz_n, 0);
assert (send == sz_n);
while (1)
{
    // RECV
    char *buf = NULL;
    int recv = nn_recv (sock, &buf, NN_MSG, 0);
    if (recv >= 0)
    {
        printf ("%s: RECEIVED '%s' FROM BUS\n", argv[1], buf);
        nn_freemsg (buf);
    }
}
*/
int32_t nano_connect(int32_t sock,char *ipaddr,int32_t port)
{
    int32_t err;
    char tcpaddr[64];
    sprintf(tcpaddr,"tcp://%s:%d",ipaddr,port);
    if ( (err= nn_connect(sock,tcpaddr)) < 0 )
        printf("error %d nn_connect.%d (%s) | %s\n",err,sock,tcpaddr,nn_strerror(nn_errno()));
    else printf("connected to (%s) err.%d\n",tcpaddr,err);
    return(err);
}

int32_t init_nanobus(char *myipaddr)
{
    cJSON *array,*item;
    int32_t i,n,err = 0;
    char ipaddr[MAX_JSON_FIELD];
    if ( Global_mp->gatewayid >= 0 || Global_mp->iambridge != 0 )
    {
        printf("call nano_socket\n");
        if ( (Global_mp->bussock= nano_socket(myipaddr,SUPERNET_PORT-1)) < 0 )
        {
            printf("err %d nano_socket\n",Global_mp->bussock);
            return(err);
        }
        array = cJSON_GetObjectItem(MGWconf,"whitelist");
        if ( array != 0 && is_cJSON_Array(array) != 0 )
        {
            n = cJSON_GetArraySize(array);
            for (i=0; i<n; i++)
            {
                if ( array == 0 || n == 0 )
                    break;
                item = cJSON_GetArrayItem(array,i);
                copy_cJSON(ipaddr,item);
                printf("call connect(%d) -> (%s)\n",Global_mp->bussock,ipaddr);
                if ( (err= nano_connect(Global_mp->bussock,ipaddr,SUPERNET_PORT)) < 0 )
                {
                    printf("err %d nano_connect i.%d of %d\n",err,i,n);
                    return(err);
                }
            }
        }
    } else Global_mp->bussock = -1;
    return(err);
}

char *init_MGWconf(char *JSON_or_fname,char *myipaddr)
{
    void init_rambases();
    void *init_SuperNET_globals();
    static int didinit,exchangeflag;
    static char ipbuf[64],*buf=0;
    static int64_t len=0,allocsize=0;
    int32_t init_SuperNET_storage(char *backupdir);
    char ipaddr[64],userdir[MAX_JSON_FIELD],*jsonstr;
    void close_SuperNET_dbs();
    if ( Global_mp == 0 )
        Global_mp = init_SuperNET_globals();
    exchangeflag = 0;
    if ( Debuglevel > 0 )
        printf("init_MGWconf exchangeflag.%d myip.(%s)\n",exchangeflag,myipaddr);
    if ( JSON_or_fname[0] == '{' )
        jsonstr = clonestr(JSON_or_fname);
    else jsonstr = load_file(JSON_or_fname,&buf,&len,&allocsize);
    if ( jsonstr != 0 )
    {
        if ( Debuglevel > 0 )
            printf("loaded.(%s)\n",jsonstr);
        if ( MGWconf != 0 )
            free_json(MGWconf);
        MGWconf = cJSON_Parse(jsonstr);
        if ( MGWconf != 0 )
        {
            if ( myipaddr == 0 )
            {
                if ( didinit == 0 && extract_cJSON_str(ipbuf,sizeof(ipbuf),MGWconf,"myipaddr") <= 0 )
                    strcpy(ipbuf,"127.0.0.1");
            } else parse_ipaddr(ipbuf,myipaddr);
            myipaddr = ipbuf;
            init_SuperNET_settings(userdir);
            if ( Debuglevel > 0 )
                printf("(%s) USESSL.%d IS_LIBTEST.%d APIPORT.%d APISLEEP.%d millis\n",ipaddr,USESSL,IS_LIBTEST,APIPORT,APISLEEP);
            init_legacyMGW(myipaddr);
            if ( Debuglevel > 0 )
                printf("issuer.%s %08x NXTAPIURL.%s, minNXTconfirms.%d port.%s orig.%s gatewayid.%d 1st.%d\n",NXTISSUERACCT,GATEWAY_SIG,NXTAPIURL,MIN_NXTCONFIRMS,SERVER_PORTSTR,ORIGBLOCK,Global_mp->gatewayid,FIRST_NXTBLOCK);
            init_rambases();
            init_Specialaddrs();
            init_SuperNET_whitelist();
            init_coinsarray(userdir,myipaddr);
            //init_filtered_bufs(); crashed ubunty
            init_confcontacts();
        }
        else printf("PARSE ERROR\n");
        free(jsonstr);
    }
    //init_tradebots_conf(MGWconf);
    init_nanobus(myipaddr);
    didinit = 1;
    if ( Debuglevel > 1 )
        printf("gatewayid.%d MGWROOT.(%s)\n",Global_mp->gatewayid,MGWROOT);
    return(myipaddr);
}
#endif
