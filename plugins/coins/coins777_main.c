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
#define PLUGINSTR "coins"
#define PLUGNAME(NAME) coins ## NAME
#define STRUCTNAME struct PLUGNAME(_info) 
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)


#define DEFINES_ONLY
#include "../includes/cJSON.h"
#include "../agents/plugin777.c"
#include "../utils/files777.c"
#include "../utils/NXT777.c"
#include "coins777.c"
//#include "gen1auth.c"
//#include "msig.c"
#undef DEFINES_ONLY

int32_t coins_idle(struct plugin_info *plugin)
{
    int32_t i,flag = 0;
    struct coin777 *coin;
    if ( COINS.num > 0 )
    {
        for (i=0; i<COINS.num; i++)
        {
            if ( (coin= COINS.LIST[i]) != 0 )
            {
#ifdef INSIDE_MGW
                if ( SUPERNET.gatewayid >= 0 )
                {
                    if ( coin->mgw.assetidstr[0] != 0 && milliseconds() > coin->mgw.lastupdate+60000 )
                    {
                        uint64_t mgw_calc_unspent(char *smallestaddr,char *smallestaddrB,struct coin777 *coin);
                        char smallestaddr[128],smallestaddrB[128];
                        update_NXT_assettransfers(&coin->mgw);
                        if ( coin->ramchain.readyflag != 0 )
                            mgw_calc_unspent(smallestaddr,smallestaddrB,coin);
                        coin->mgw.lastupdate = milliseconds();
                    }
                    /*if ( coin->mgw.marker[0] != 0 && coin->ramchain.startmilli == 0 && coin->ramchain.readyflag == 0 )
                    {
                        uint32_t ramchain_prepare(struct coin777 *coin,struct ramchain *ramchain);
                        ramchain_prepare(coin,&coin->ramchain);
                        coin->ramchain.readyflag = 1;
                        coin->ramchain.paused = 0;
                    }*/
                }
#endif
            }
        }
    }
    return(flag);
}

STRUCTNAME COINS;
char *PLUGNAME(_methods)[] = { "acctpubkeys",  "packblocks", "sendrawtransaction", "setmultisig", "gotmsigaddr" };
char *PLUGNAME(_pubmethods)[] = { "acctpubkeys", "setmultisig", "gotmsigaddr" };
char *PLUGNAME(_authmethods)[] = { "acctpubkeys" };

cJSON *coins777_json()
{
    cJSON *item,*array = cJSON_CreateArray();
    struct coin777 *coin;
    int32_t i;
    if ( COINS.num > 0 )
    {
        for (i=0; i<COINS.num; i++)
        {
            if ( (coin= COINS.LIST[i]) != 0 )
            {
                item = cJSON_CreateObject();
                cJSON_AddItemToObject(item,"name",cJSON_CreateString(coin->name));
                if ( coin->serverport[0] != 0 )
                    cJSON_AddItemToObject(item,"rpc",cJSON_CreateString(coin->serverport));
                cJSON_AddItemToArray(array,item);
            }
        }
    }
    return(array);
}

int32_t coin777_close(char *coinstr)
{
    struct coin777 *coin;
    if ( (coin= coin777_find(coinstr,0)) != 0 )
    {
        if ( coin->jsonstr != 0 )
            free(coin->jsonstr);
        if ( coin->argjson != 0 )
            free_json(coin->argjson);
        coin = COINS.LIST[--COINS.num], COINS.LIST[COINS.num] = 0;
        free(coin);
        return(0);
    }
    return(-1);
}

void shutdown_coins()
{
    while ( COINS.num > 0 )
        if ( coin777_close(COINS.LIST[0]->name) < 0 )
            break;
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

char *default_coindir(char *confname,char *coinstr)
{
    int32_t i;
#ifdef __APPLE__
    char *coindirs[][3] = { {"BTC","Bitcoin","bitcoin"}, {"BTCD","BitcoinDark"}, {"LTC","Litecoin","litecoin"}, {"VRC","Vericoin","vericoin"}, {"OPAL","OpalCoin","opalcoin"}, {"BITS","Bitstar","bitstar"}, {"DOGE","Dogecoin","dogecoin"}, {"DASH","Dash","dash"}, {"BC","Blackcoin","blackcoin"}, {"FIBRE","Fibre","fibre"}, {"VPN","Vpncoin","vpncoin"} };
#else
    char *coindirs[][3] = { {"BTC",".bitcoin"}, {"BTCD",".BitcoinDark"}, {"LTC",".litecoin"}, {"VRC",".vericoin"}, {"OPAL",".opalcoin"}, {"BITS",".Bitstar"}, {"DOGE",".dogecoin"}, {"DASH",".dash"}, {"BC",".blackcoin"}, {"FIBRE",".Fibre"}, {"VPN",".vpncoin"} };
#endif
    for (i=0; i<(int32_t)(sizeof(coindirs)/sizeof(*coindirs)); i++)
        if ( strcmp(coindirs[i][0],coinstr) == 0 )
        {
            if ( coindirs[i][2] != 0 )
                strcpy(confname,coindirs[i][2]);
            else strcpy(confname,coindirs[i][1] + (coindirs[i][1][0] == '.'));
            return(coindirs[i][1]);
        }
    return(coinstr);
}

void set_coinconfname(char *fname,char *coinstr,char *userhome,char *coindir,char *confname)
{
    char buf[64];
    if ( coindir == 0 || coindir[0] == 0 )
        coindir = default_coindir(buf,coinstr);
    if ( confname == 0 || confname[0] == 0 )
    {
        confname = buf;
        sprintf(confname,"%s.conf",buf);
    }
    printf("userhome.(%s) coindir.(%s) confname.(%s)\n",userhome,coindir,confname);
    sprintf(fname,"%s/%s/%s",userhome,coindir,confname);
}

uint16_t extract_userpass(char *serverport,char *userpass,char *coinstr,char *userhome,char *coindir,char *confname)
{
    FILE *fp; uint16_t port = 0;
    char fname[2048],line[1024],*rpcuser,*rpcpassword,*rpcport,*str;
    if ( strcmp(coinstr,"NXT") == 0 )
        return(0);
    serverport[0] = userpass[0] = 0;
    set_coinconfname(fname,coinstr,userhome,coindir,confname);
    printf("set_coinconfname.(%s)\n",fname);
    if ( (fp= fopen(os_compatible_path(fname),"r")) != 0 )
    {
        if ( Debuglevel > 1 )
            printf("extract_userpass from (%s)\n",fname);
        rpcuser = rpcpassword = rpcport = 0;
        while ( fgets(line,sizeof(line),fp) != 0 )
        {
            if ( line[0] == '#' )
                continue;
            //printf("line.(%s) %p %p\n",line,strstr(line,"rpcuser"),strstr(line,"rpcpassword"));
            if ( (str= strstr(line,"rpcuser")) != 0 )
                rpcuser = parse_conf_line(str,"rpcuser");
            else if ( (str= strstr(line,"rpcpassword")) != 0 )
                rpcpassword = parse_conf_line(str,"rpcpassword");
            else if ( (str= strstr(line,"rpcport")) != 0 )
                rpcport = parse_conf_line(str,"rpcport");
        }
        if ( rpcuser != 0 && rpcpassword != 0 )
        {
            if ( userpass[0] == 0 )
                sprintf(userpass,"%s:%s",rpcuser,rpcpassword);
        }
        if ( rpcport != 0 )
        {
            port = myatoi(rpcport,65536);
            if ( serverport[0] == 0 )
                sprintf(serverport,"127.0.0.1:%s",rpcport);
            free(rpcport);
        }
        if ( Debuglevel > 1 )
            printf("-> (%s):(%s) userpass.(%s) serverport.(%s)\n",rpcuser,rpcpassword,userpass,serverport);
        if ( rpcuser != 0 )
            free(rpcuser);
        if ( rpcpassword != 0 )
            free(rpcpassword);
        fclose(fp);
    } else printf("extract_userpass cant open.(%s)\n",fname);
    return(port);
}

void set_atomickeys(struct coin777 *coin)
{
    char *addr; struct destbuf pubkey;
    if ( (addr= get_acct_coinaddr(coin->atomicrecv,coin->name,coin->serverport,coin->userpass,"atomicrecv")) != 0 )
    {
        get_pubkey(&pubkey,coin->name,coin->serverport,coin->userpass,coin->atomicrecv);
        strcpy(coin->atomicrecvpubkey,pubkey.buf);
    }
    if ( (addr= get_acct_coinaddr(coin->atomicsend,coin->name,coin->serverport,coin->userpass,"atomicsend")) != 0 )
    {
        get_pubkey(&pubkey,coin->name,coin->serverport,coin->userpass,coin->atomicsend);
        strcpy(coin->atomicsendpubkey,pubkey.buf);
    }
}

struct coin777 *coin777_create(char *coinstr,cJSON *argjson)
{
    char *serverport,*path=0,*conf=0; struct destbuf tmp;
    struct coin777 *coin = calloc(1,sizeof(*coin));
    if ( coinstr == 0 || coinstr[0] == 0 )
    {
        printf("null coinstr?\n");
        //getchar();
        return(0);
    }
    safecopy(coin->name,coinstr,sizeof(coin->name));
    if ( argjson == 0 || strcmp(coinstr,"NXT") == 0 )
    {
        coin->usep2sh = 1;
        coin->minconfirms = (strcmp("BTC",coinstr) == 0) ? 3 : 10;
        coin->estblocktime = (strcmp("BTC",coinstr) == 0) ? 600 : 120;
        coin->mgw.use_addmultisig = (strcmp("BTC",coinstr) != 0);
    }
    else
    {
        coin->minoutput = get_API_nxt64bits(cJSON_GetObjectItem(argjson,"minoutput"));
        coin->minconfirms = get_API_int(cJSON_GetObjectItem(argjson,"minconfirms"),(strcmp("BTC",coinstr) == 0) ? 3 : 10);
        coin->estblocktime = get_API_int(cJSON_GetObjectItem(argjson,"estblocktime"),(strcmp("BTC",coinstr) == 0) ? 600 : 120);
        coin->jsonstr = cJSON_Print(argjson);
        coin->argjson = cJSON_Duplicate(argjson,1);
        if ( (serverport= cJSON_str(cJSON_GetObjectItem(argjson,"rpc"))) != 0 )
            safecopy(coin->serverport,serverport,sizeof(coin->serverport));
        path = cJSON_str(cJSON_GetObjectItem(argjson,"path"));
        conf = cJSON_str(cJSON_GetObjectItem(argjson,"conf"));

        copy_cJSON(&tmp,cJSON_GetObjectItem(argjson,"assetid")), safecopy(coin->mgw.assetidstr,tmp.buf,sizeof(coin->mgw.assetidstr));
        if ( (coin->mgw.assetidbits= calc_nxt64bits(coin->mgw.assetidstr)) == 0 )
            coin->mgw.assetidbits = is_MGWcoin(coinstr);
        copy_cJSON(&tmp,cJSON_GetObjectItem(argjson,"issuer")), safecopy(coin->mgw.issuer,tmp.buf,sizeof(coin->mgw.issuer));;
        coin->mgw.issuerbits = conv_acctstr(coin->mgw.issuer);
        printf(">>>>>>>>>>>> a issuer.%s %llu assetid.%llu minoutput.%llu\n",coin->mgw.issuer,(long long)coin->mgw.issuerbits,(long long)coin->mgw.assetidbits,(long long)coin->minoutput);
        //uint32_t set_assetname(uint64_t *multp,char *name,uint64_t assetbits);
        if ( coin->mgw.assetidbits != 0 )
            _set_assetname(&coin->mgw.ap_mult,coin->mgw.assetname,0,coin->mgw.assetidbits);
        printf("assetname.(%s) mult.%llu\n",coin->mgw.assetname,coin->mgw.ap_mult);
        strcpy(coin->mgw.coinstr,coinstr);
        if ( (coin->mgw.special= cJSON_GetObjectItem(argjson,"special")) == 0 )
            coin->mgw.special = cJSON_GetObjectItem(COINS.argjson,"special");
        if ( coin->mgw.special != 0 )
        {
            coin->mgw.special = NXT_convjson(coin->mgw.special);
            printf("CONVERTED.(%s)\n",cJSON_Print(coin->mgw.special));
        }
        coin->mgw.limbo = cJSON_GetObjectItem(argjson,"limbo");
        coin->mgw.dust = get_API_nxt64bits(cJSON_GetObjectItem(argjson,"dust"));
        coin->mgw.txfee = get_API_nxt64bits(cJSON_GetObjectItem(argjson,"txfee_satoshis"));
        if ( coin->mgw.txfee == 0 )
            coin->mgw.txfee = (uint64_t)(SATOSHIDEN * get_API_float(cJSON_GetObjectItem(argjson,"txfee")));
        coin->mgw.NXTfee_equiv = get_API_nxt64bits(cJSON_GetObjectItem(argjson,"NXTfee_equiv_satoshis"));
        if ( coin->mgw.NXTfee_equiv == 0 )
            coin->mgw.NXTfee_equiv = (uint64_t)(SATOSHIDEN * get_API_float(cJSON_GetObjectItem(argjson,"NXTfee_equiv")));
        copy_cJSON(&tmp,cJSON_GetObjectItem(argjson,"opreturnmarker")), safecopy(coin->mgw.opreturnmarker,tmp.buf,sizeof(coin->mgw.opreturnmarker));
        printf("OPRETURN.(%s)\n",coin->mgw.opreturnmarker);
        copy_cJSON(&tmp,cJSON_GetObjectItem(argjson,"marker2")), safecopy(coin->mgw.marker2,tmp.buf,sizeof(coin->mgw.marker2));
        coin->mgw.redeemheight = get_API_int(cJSON_GetObjectItem(argjson,"redeemheight"),430000);
        coin->mgw.use_addmultisig = get_API_int(cJSON_GetObjectItem(argjson,"useaddmultisig"),(strcmp("BTC",coinstr) != 0));
        coin->mgw.do_opreturn = get_API_int(cJSON_GetObjectItem(argjson,"do_opreturn"),(strcmp("BTC",coinstr) == 0));
        coin->mgw.oldtx_format = get_API_int(cJSON_GetObjectItem(argjson,"oldtx_format"),(strcmp("BTC",coinstr) == 0));
        coin->mgw.firstunspentind = get_API_int(cJSON_GetObjectItem(argjson,"firstunspent"),(strcmp("BTCD",coinstr) == 0) ? 2500000 : 0);
        if ( (coin->mgw.NXTconvrate = get_API_float(cJSON_GetObjectItem(argjson,"NXTconvrate"))) == 0 )
        {
            if ( coin->mgw.NXTfee_equiv != 0 && coin->mgw.txfee != 0 )
                coin->mgw.NXTconvrate = ((double)coin->mgw.NXTfee_equiv / coin->mgw.txfee);
        }
        copy_cJSON(&tmp,cJSON_GetObjectItem(argjson,"marker")), safecopy(coin->mgw.marker,tmp.buf,sizeof(coin->mgw.marker));
        printf("OPRETURN.(%s)\n",coin->mgw.opreturnmarker);
        coin->addrtype = get_API_int(jobj(argjson,"addrtype"),0);
        coin->p2shtype = get_API_int(jobj(argjson,"p2shtype"),0);
        coin->usep2sh = get_API_int(jobj(argjson,"usep2sh"),1);
    }
    if ( coin->mgw.txfee == 0 )
        coin->mgw.txfee = 10000;
    if ( strcmp(coin->name,"BTC") == 0 )
    {
        coin->addrtype = 0, coin->p2shtype = 5;
        if ( coin->donationaddress[0] == 0 )
            strcpy(coin->donationaddress,"177MRHRjAxCZc7Sr5NViqHRivDu1sNwkHZ");
    }
    else if ( strcmp(coin->name,"LTC") == 0 )
        coin->addrtype = 48, coin->p2shtype = 5, coin->minconfirms = 1, coin->mgw.txfee = 100000, coin->usep2sh = 0;
    else if ( strcmp(coin->name,"BTCD") == 0 )
        coin->addrtype = 60, coin->p2shtype = 85;
    else if ( strcmp(coin->name,"DOGE") == 0 )
        coin->addrtype = 30, coin->p2shtype = 35;
    else if ( strcmp(coin->name,"VRC") == 0 )
        coin->addrtype = 70, coin->p2shtype = 85;
    else if ( strcmp(coin->name,"OPAL") == 0 )
        coin->addrtype = 115, coin->p2shtype = 28;
    else if ( strcmp(coin->name,"BITS") == 0 )
        coin->addrtype = 25, coin->p2shtype = 8;
    printf("coin777_create %s: (%s) %llu mult.%llu NXTconvrate %.8f minconfirms.%d issuer.(%s) %llu opreturn.%d oldformat.%d\n",coin->mgw.coinstr,coin->mgw.assetidstr,(long long)coin->mgw.assetidbits,(long long)coin->mgw.ap_mult,coin->mgw.NXTconvrate,coin->minconfirms,coin->mgw.issuer,(long long)coin->mgw.issuerbits,coin->mgw.do_opreturn,coin->mgw.oldtx_format);
    if ( strcmp(coin->name,"NXT") != 0 )
    {
        extract_userpass(coin->serverport,coin->userpass,coinstr,SUPERNET.userhome,path,conf);
        set_atomickeys(coin);
        printf("COIN.%s serverport.(%s) userpass.(%s) %s/%s %s/%s\n",coin->name,coin->serverport,coin->userpass,coin->atomicrecv,coin->atomicrecvpubkey,coin->atomicsend,coin->atomicsendpubkey);
    }
    COINS.LIST = realloc(COINS.LIST,(COINS.num+1) * sizeof(*coin));
    COINS.LIST[COINS.num] = coin, COINS.num++;
    //ensure_packedptrs(coin);
    return(coin);
}

struct coin777 *coin777_find(char *coinstr,int32_t autocreate)
{
    int32_t i,j,n; cJSON *item,*array; char *str;
    if ( coinstr == 0 )
        return(0);
    if ( COINS.num > 0 )
    {
        for (i=0; i<COINS.num; i++)
        {
            if ( strcmp(coinstr,COINS.LIST[i]->name) == 0 )
                return(COINS.LIST[i]);
        }
    }
    if ( autocreate != 0 )
    {
        if ( COINS.argjson != 0 && (array= cJSON_GetObjectItem(COINS.argjson,"coins")) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
        {
            for (i=j=0; i<n; i++)
            {
                item = cJSON_GetArrayItem(array,i);
                str = cJSON_str(cJSON_GetObjectItem(item,"name"));
                if ( str != 0 && strcmp(str,coinstr) == 0 )
                {
                    printf("CALL coin777_create.(%s) (%s)\n",coinstr,cJSON_Print(item));
                    return(coin777_create(coinstr,item));
                }
            }
        }
        return(coin777_create(coinstr,COINS.argjson));
    }
    return(0);
}

int32_t PLUGNAME(_process_json)(char *forwarder,char *sender,int32_t valid,struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag,char *tokenstr)
{
    char *resultstr,*methodstr,zerobuf[1],*coinstr,*str = 0; cJSON *array,*item; int32_t i,n,j = 0; struct coin777 *coin; struct destbuf tmp;
    retbuf[0] = 0;
    if ( initflag > 0 )
    {
        if ( json != 0 )
        {
            COINS.argjson = cJSON_Duplicate(json,1);
            COINS.slicei = get_API_int(cJSON_GetObjectItem(json,"slice"),0);
            if ( (array= cJSON_GetObjectItem(json,"coins")) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=j=0; i<n; i++)
                {
                    item = cJSON_GetArrayItem(array,i);
                    coinstr = cJSON_str(cJSON_GetObjectItem(item,"name"));
                    if ( coinstr != 0 && coinstr[0] != 0 && (coin= coin777_find(coinstr,0)) == 0 )
                    {
                        printf("CALL coin777_create.(%s) (%s)\n",coinstr,cJSON_Print(item));
                        coin777_create(coinstr,item);
                    }
                }
            }
        } else strcpy(retbuf,"{\"result\":\"no JSON for init\"}");
        COINS.readyflag = 1;
        plugin->allowremote = 1;
        //plugin->sleepmillis = 1;
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        methodstr = cJSON_str(cJSON_GetObjectItem(json,"method"));
        coinstr = cJSON_str(cJSON_GetObjectItem(json,"coin"));
        if ( methodstr == 0 || methodstr[0] == 0 )
        {
            printf("(%s) has not method\n",jsonstr);
            return(0);
        }
        //printf("COINS.(%s) for (%s) (%s)\n",methodstr,coinstr!=0?coinstr:"",jsonstr);
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
        else
        {
            zerobuf[0] = 0;
            str = 0;
            //printf("INSIDE COINS.(%s) methods.%ld\n",jsonstr,sizeof(coins_methods)/sizeof(*coins_methods));
            copy_cJSON(&tmp,cJSON_GetObjectItem(json,"NXT")), safecopy(sender,tmp.buf,32);
            if ( coinstr == 0 )
                coinstr = zerobuf;
            else coin = coin777_find(coinstr,1);
#ifdef INSIDE_MGW
            if ( strcmp(methodstr,"acctpubkeys") == 0 )
            {
                if ( SUPERNET.gatewayid >= 0 )
                {
                    if ( coinstr[0] == 0 )
                        strcpy(retbuf,"{\"result\":\"need to specify coin\"}");
                    else if ( (coin= coin777_find(coinstr,1)) != 0 )
                    {
                        int32_t MGW_publish_acctpubkeys(char *coinstr,char *str);
                        char *get_msig_pubkeys(char *coinstr,char *serverport,char *userpass);
                        if ( (str= get_msig_pubkeys(coin->name,coin->serverport,coin->userpass)) != 0 )
                        {
                            MGW_publish_acctpubkeys(coin->name,str);
                            strcpy(retbuf,"{\"result\":\"published and processed acctpubkeys\"}");
                            free(str), str= 0;
                        } else sprintf(retbuf,"{\"error\":\"no get_msig_pubkeys result\",\"method\":\"%s\"}",methodstr);
                    } else sprintf(retbuf,"{\"error\":\"no coin777\",\"method\":\"%s\"}",methodstr);
                } else sprintf(retbuf,"{\"error\":\"gateway only method\",\"method\":\"%s\"}",methodstr);
            }
            else if ( strcmp(methodstr,"gotmsigaddr") == 0 )
            {
                if ( SUPERNET.gatewayid < 0 )
                    printf("GOTMSIG.(%s)\n",jsonstr);
            }
            else
#endif
                sprintf(retbuf,"{\"error\":\"unsupported method\",\"method\":\"%s\"}",methodstr);
        }
    }
    //printf("<<<<<<<<<<<< INSIDE PLUGIN.(%s) initflag.%d process %s slice.%d\n",SUPERNET.myNXTaddr,initflag,plugin->name,COINS.slicei);
    return(plugin_copyretstr(retbuf,maxlen,str));
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    shutdown_coins();
    return(retcode);
}

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *argjson)
{
    uint64_t disableflags = 0;
    printf("register init %s size.%ld\n",plugin->name,sizeof(struct coins_info));
    COINS.readyflag = 1;
    // runtime specific state can be created and put into *data
    return(disableflags); // set bits corresponding to array position in _methods[]
}

#include "../agents/plugin777.c"
