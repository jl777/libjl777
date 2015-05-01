//
//  echodemo.c
//  SuperNET API extension example plugin
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

#define BUNDLED
#define PLUGINSTR "coins"
#define PLUGNAME(NAME) coins ## NAME
#define STRUCTNAME struct PLUGNAME(_info) 
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)


#define DEFINES_ONLY
#include "cJSON.h"
#include "../plugin777.c"
#include "files777.c"
#include "coins777.c"
#include "gen1auth.c"
#include "msig.c"
#undef DEFINES_ONLY


char *genmultisig(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *coinstr,char *refacct,int32_t M,int32_t N,uint64_t *srv64bits,int32_t n,char *userpubkey,char *email,uint32_t buyNXT);
char *setmultisig(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,char *origargstr);
char *getmsigpubkey(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,char *coinstr,char *refNXTaddr,char *myacctcoinaddr,char *mypubkey);
char *setmsigpubkey(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,char *coinstr,char *refNXTaddr,char *acctcoinaddr,char *userpubkey);

void coins_idle(struct plugin_info *plugin) {}

STRUCTNAME COINS;
char *PLUGNAME(_methods)[] = { "addcoin", "remove", "genmultisig", "setmsigpubkey", "getmsigpubkey", "setmultisig" };

struct coin777 *coin777_find(char *coinstr)
{
    int32_t i;
    if ( COINS.num > 0 )
    {
        for (i=0; i<COINS.num; i++)
        {
            if ( strcmp(coinstr,COINS.LIST[i]->name) == 0 )
                return(COINS.LIST[i]);
        }
    }
    return(0);
}

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
                if ( coin->serverport != 0 )
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
    if ( (coin= coin777_find(coinstr)) != 0 )
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

struct coin777 *coin777_create(char *coinstr,char *serverport,char *userpass,cJSON *argjson)
{
    struct coin777 *coin = calloc(1,sizeof(*coin));
    safecopy(coin->name,coinstr,sizeof(coin->name));
    if ( serverport != 0 )
        safecopy(coin->serverport,serverport,sizeof(coin->serverport));
    if ( userpass != 0 )
        safecopy(coin->userpass,userpass,sizeof(coin->userpass));
    coin->use_addmultisig = (strcmp("BTC",coinstr) != 0);
    coin->gatewayid = MGW.gatewayid;
    if ( argjson != 0 )
    {
        coin->jsonstr = cJSON_Print(argjson);
        coin->argjson = cJSON_Duplicate(argjson,1);
    }
    COINS.LIST = realloc(COINS.LIST,(COINS.num+1) * sizeof(*coin));
    COINS.LIST[COINS.num] = coin, COINS.num++;
    return(coin);
}

uint64_t set_account_NXTSECRET(char *NXTacct,char *NXTaddr,char *secret,int32_t max,cJSON *argjson,char *coinstr,char *serverport,char *userpass)
{
    uint64_t allocsize,nxt64bits;
    uint8_t mysecret[32],mypublic[32];
    char coinaddr[MAX_JSON_FIELD],*str,*privkey;
    NXTaddr[0] = 0;
    extract_cJSON_str(secret,max,argjson,"secret");
    //printf("set_account_NXTSECRET.(%s)\n",secret);
    if ( secret[0] == 0 )
    {
        extract_cJSON_str(coinaddr,sizeof(coinaddr),argjson,"privateaddr");
        if ( strcmp(coinaddr,"privateaddr") == 0 )
        {
            if ( (str= loadfile(&allocsize,"privateaddr")) != 0 )
            {
                if ( allocsize < 128 )
                    strcpy(coinaddr,str);
                free(str);
            }
        }
        if ( coinaddr[0] == 0 )
            extract_cJSON_str(coinaddr,sizeof(coinaddr),argjson,"pubsrvaddr");
        printf("coinaddr.(%s)\n",coinaddr);
        if ( coinstr == 0 || serverport == 0 || userpass == 0 || (privkey= dumpprivkey(coinstr,serverport,userpass,coinaddr)) == 0 )
            gen_randomacct(33,NXTaddr,secret,"randvals");
        else
        {
            strcpy(secret,privkey);
            free(privkey);
        }
    }
    nxt64bits = conv_NXTpassword(mysecret,mypublic,(uint8_t *)secret,(int32_t)strlen(secret));
    expand_nxt64bits(NXTaddr,nxt64bits);
    conv_rsacctstr(NXTacct,nxt64bits);
    //printf("(%s) (%s) (%s)\n",NXTacct,NXTaddr,secret);
    return(nxt64bits);
}

int32_t make_MGWbus(uint16_t port,char *bindaddr,char serverips[MAX_MGWSERVERS][64],int32_t n)
{
    char tcpaddr[64];
    int32_t i,err,sock,timeout = 1;
    if ( (sock= nn_socket(AF_SP,NN_BUS)) < 0 )
    {
        printf("error getting socket.%d %s\n",sock,nn_strerror(nn_errno()));
        return(-1);
    }
    if ( bindaddr != 0 && bindaddr[0] != 0 )
    {
        sprintf(tcpaddr,"tcp://%s:%d",bindaddr,port);
        printf("MGW bind.(%s)\n",tcpaddr);
        if ( (err= nn_bind(sock,tcpaddr)) < 0 )
        {
            printf("error binding socket.%d %s\n",sock,nn_strerror(nn_errno()));
            return(-1);
        }
        if ( (err= nn_connect(sock,tcpaddr)) < 0 )
        {
            printf("error nn_connect (%s <-> %s) socket.%d %s\n",bindaddr,tcpaddr,sock,nn_strerror(nn_errno()));
            return(-1);
        }
        if ( timeout > 0 && nn_setsockopt(sock,NN_SOL_SOCKET,NN_RCVTIMEO,&timeout,sizeof(timeout)) < 0 )
        {
            printf("error nn_setsockopt socket.%d %s\n",sock,nn_strerror(nn_errno()));
            return(-1);
        }
        for (i=0; i<n; i++)
        {
            //if ( strcmp(bindaddr,serverips[i]) != 0 )
            {
                sprintf(tcpaddr,"tcp://%s:%d",serverips[i],port);
                printf("conn.(%s) ",tcpaddr);
                if ( (err= nn_connect(sock,tcpaddr)) < 0 )
                {
                    printf("error nn_connect (%s <-> %s) socket.%d %s\n",bindaddr,tcpaddr,sock,nn_strerror(nn_errno()));
                    return(-1);
                }
            }
        }
    } else nn_shutdown(sock,0), sock = -1;
    return(sock);
}

cJSON *check_conffile(int32_t *allocflagp,cJSON *json)
{
    char buf[MAX_JSON_FIELD],*filestr;
    uint64_t allocsize;
    cJSON *item;
    *allocflagp = 0;
    if ( json == 0 )
        return(0);
    copy_cJSON(buf,cJSON_GetObjectItem(json,"filename"));
    if ( buf[0] != 0 && (filestr= loadfile(&allocsize,buf)) != 0 )
    {
        if ( (item= cJSON_Parse(filestr)) != 0 )
        {
            json = item;
            *allocflagp = 1;
            printf("parsed (%s) for JSON\n",buf);
        }
        free(filestr);
    }
    return(json);
}

int32_t PLUGNAME(_process_json)(struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag)
{
    char *resultstr,sender[MAX_JSON_FIELD],*methodstr,zerobuf[1],buf0[MAX_JSON_FIELD],buf1[MAX_JSON_FIELD],buf2[MAX_JSON_FIELD],msigchar[64],nxtaddr[64],ipaddr[64],*coinstr,*serverport,*userpass,*arraystr,*str,*email,*previpaddr = 0;
    cJSON *array,*item;
    int32_t i,n,buyNXT,allocflag,j = 0;
    struct coin777 *coin;
    retbuf[0] = 0;
    if ( initflag > 0 )
    {
        if ( DB_msigs == 0 )
            DB_msigs = db777_create(0,0,"msigs",0);
        if ( DB_NXTaccts == 0 )
            DB_NXTaccts = db777_create(0,0,"NXTacct",0);
        //DB_NXTassettx = db777_create(0,0,"NXTassettxid",0);
        //DB_nodestats = db777_create(0,0,"nodestats",0);
        if ( json != 0 )
        {
            json = check_conffile(&allocflag,json);
            copy_cJSON(SUPERNET.myNXTacct,cJSON_GetObjectItem(json,"myNXTacct"));
            copy_cJSON(MGW.PATH,cJSON_GetObjectItem(json,"MGWROOT"));
            if ( MGW.PATH[0] == 0 )
            {
                strcpy(MGW.PATH,"MGW");
                ensure_directory(MGW.PATH);
            }
            set_account_NXTSECRET(SUPERNET.NXTACCT,SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,sizeof(SUPERNET.NXTACCTSECRET)-1,json,0,0,0);
            SUPERNET.my64bits = conv_acctstr(SUPERNET.NXTADDR);
            /*if ( 1 && SUPERNET.NXTADDR[0] != 0 && SUPERNET.myNXTaddr[0] != 0 && strcmp(SUPERNET.myNXTaddr,SUPERNET.NXTADDR) != 0 )
            {
                sprintf(retbuf,"{\"error\":\"mismatched NXT accounts\",\"fromsecret\":\"%s\",\"myNXT\":\"%s\"}",SUPERNET.NXTADDR,SUPERNET.myNXTaddr);
                printf("ERROR ERROR\n");
                if ( allocflag != 0 )
                   free_json(json);
                return((int32_t)strlen(retbuf));
            }*/
            COINS.argjson = cJSON_Duplicate(json,1);
            copy_cJSON(MGW.bridgeipaddr,cJSON_GetObjectItem(json,"bridgeipaddr"));
            copy_cJSON(MGW.bridgeacct,cJSON_GetObjectItem(json,"bridgeacct"));
            copy_cJSON(SUPERNET.myipaddr,cJSON_GetObjectItem(json,"myipaddr"));
            SUPERNET.port = get_API_int(cJSON_GetObjectItem(json,"SUPERNET_PORT"),0);
            copy_cJSON(SUPERNET.userhome,cJSON_GetObjectItem(json,"userdir"));
            if ( SUPERNET.userhome[0] == 0 )
                strcpy(SUPERNET.userhome,"/root");
            MGW.N = get_API_int(cJSON_GetObjectItem(json,"N"),0);
            MGW.M = get_API_int(cJSON_GetObjectItem(json,"M"),0);
            MGW.gatewayid = get_API_int(cJSON_GetObjectItem(json,"gatewayid"),-1);
            if ( (array= cJSON_GetObjectItem(json,"servers")) != 0 && (n= cJSON_GetArraySize(array)) > 0 && (n & 1) == 0 )
            {
                for (i=j=0; i<n/2&&i<MAX_MGWSERVERS; i++)
                {
                    copy_cJSON(ipaddr,cJSON_GetArrayItem(array,i<<1));
                    copy_cJSON(nxtaddr,cJSON_GetArrayItem(array,(i<<1)+1));
                    if ( strcmp(ipaddr,MGW.bridgeipaddr) != 0 )
                    {
                        MGW.srv64bits[j] = conv_rsacctstr(nxtaddr,0);
                        strcpy(MGW.serverips[j],ipaddr);
                        printf("%d.(%s).%llu ",j,ipaddr,(long long)MGW.srv64bits[j]);
                        j++;
                    }
                }
                printf("ipaddrs: %s %s %s\n",MGW.serverips[0],MGW.serverips[1],MGW.serverips[2]);
                if ( MGW.gatewayid >= 0 && MGW.N )
                {
                    strcpy(SUPERNET.myipaddr,MGW.serverips[MGW.gatewayid]);
                }
                //printf("j.%d M.%d N.%d n.%d (%s).%s gateway.%d\n",j,COINS.M,COINS.N,n,COINS.myipaddr,COINS.myNXTaddr,COINS.gatewayid);
                if ( j != MGW.N )
                    sprintf(retbuf+1,"{\"warning\":\"mismatched servers\",\"details\":\"n.%d j.%d vs M.%d N.%d\",",n,j,MGW.M,MGW.N);
                else
                {
                    strcpy(MGW.serverips[MGW.N],MGW.bridgeipaddr);
                    MGW.srv64bits[MGW.N] = calc_nxt64bits(MGW.bridgeacct);
                    MGW.all.socks.both.bus = make_MGWbus(SUPERNET.port,SUPERNET.myipaddr,MGW.serverips,MGW.N+1);
                    MGW.numgateways = MGW.N;
                    MGW.gatewayid = MGW.gatewayid;
                }
            }
            sprintf(retbuf+strlen(retbuf),"\"M\":%d,\"N\":%d,\"bridge\":\"%s\",\"myipaddr\":\"%s\",\"port\":%d}",MGW.M,MGW.N,MGW.bridgeipaddr,SUPERNET.myipaddr,SUPERNET.port);
            if ( allocflag != 0 )
                free_json(json);
            //int32_t init_public_msigs();
            //init_public_msigs();
        } else strcpy(retbuf,"{\"result\":\"no JSON for init\"}");
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
        printf("COINS.(%s) for (%s) (%s)\n",methodstr,coinstr!=0?coinstr:"",jsonstr);
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
        else
        {
            if ( json == 0 )
                json = COINS.argjson;
            if ( strcmp(methodstr,"addcoin") == 0 )
            {
                if ( (array= cJSON_GetObjectItem(json,"coins")) == 0 )
                    array = cJSON_GetObjectItem(COINS.argjson,"coins");
                allocflag = 0;
                // array = check_conffile(&allocflag,array);
                if ( array != 0 && (n= cJSON_GetArraySize(array)) > 0 )
                {
                    // { "name":"BTCD","rpc":"127.0.0.1:14631","dir":".BitcoinDark","conf":"BitcoinDark.conf","multisigchar":"b" }
                    for (i=j=0; i<n; i++)
                    {
                        item = cJSON_GetArrayItem(array,i);
                        coinstr = cJSON_str(cJSON_GetObjectItem(item,"name"));
                        serverport = cJSON_str(cJSON_GetObjectItem(item,"rpc"));
                        if ( extract_cJSON_str(buf0,sizeof(buf0),item,"path") > 0 && extract_cJSON_str(buf1,sizeof(buf1),item,"conf") > 0 )
                            userpass = extract_userpass(SUPERNET.userhome,buf0,buf1);
                        else userpass = 0;
                        printf("name.(%s)\n",coinstr);
                        if ( coinstr != 0 && coinstr[0] != 0 )
                        {
                            if ( (coin= coin777_find(coinstr)) == 0 )
                                coin = coin777_create(coinstr,serverport,userpass,item);
                            if ( coin != 0 )
                            {
                                j++;
                                if ( serverport != 0 && strcmp(serverport,coin->serverport) != 0 )
                                    strcpy(coin->serverport,serverport);
                                if ( userpass != 0 && strcmp(userpass,coin->userpass) != 0 )
                                    strcpy(coin->userpass,userpass);
                                if ( extract_cJSON_str(msigchar,sizeof(msigchar),item,"multisigchar") > 0 )
                                    coin->multisigchar = msigchar[0];
                                coin->use_addmultisig = get_API_int(cJSON_GetObjectItem(item,"useaddmultisig"),strcmp("BTC",coinstr)!=0);
                                if ( strcmp(coinstr,"BTCD") == 0 )//&& COINS.NXTACCTSECRET[0] == 0 )
                                    set_account_NXTSECRET(SUPERNET.NXTACCT,SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,sizeof(SUPERNET.NXTACCTSECRET)-1,item,coinstr,serverport,userpass);
                                }
                        }
                        if ( userpass != 0 )
                            free(userpass);
                     }
                }
                if ( allocflag != 0 )
                    free_json(array);
                array = coins777_json();
                arraystr = cJSON_Print(array);
                sprintf(retbuf,"{\"result\":\"addcoin\",\"added\":%d,\"coins\":%s,\"NXT\":\"%s\"}",j,arraystr,SUPERNET.NXTADDR);
                free(arraystr), free_json(array);
            }
            else if ( strcmp(methodstr,"remove") == 0 )
            {
                if ( coinstr != 0 && coinstr[0] != 0 )
                {
                    if ( strcmp(coinstr,"*") == 0 || strcmp(coinstr,"allcoins") == 0 )
                        shutdown_coins();
                    else coin777_close(coinstr);
                    array = coins777_json();
                    arraystr = cJSON_Print(array);
                    sprintf(retbuf,"{\"result\":\"remove\",\"coin\":\"%s\",\"coins\":%s}",coinstr,arraystr);
                    free(arraystr), free_json(array);
                }
            }
            else
            {
                zerobuf[0] = 0;
                str = 0;
                printf("INSIDE COINS.(%s)\n",jsonstr);
                copy_cJSON(sender,cJSON_GetObjectItem(json,"NXT"));
                if ( coinstr == 0 )
                    coinstr = zerobuf;
                if ( strcmp(methodstr,"genmultisig") == 0 )
                {
                    if ( coinstr[0] == 0 )
                        strcpy(retbuf,"{\"error\":\"no coin specified\"}");
                    else
                    {
                        copy_cJSON(buf0,cJSON_GetObjectItem(json,"refcontact"));
                        copy_cJSON(buf1,cJSON_GetObjectItem(json,"userpubkey"));
                        email = cJSON_str(cJSON_GetObjectItem(json,"email"));
                        buyNXT = get_API_int(cJSON_GetObjectItem(json,"buyNXT"),0);
                        str = genmultisig(SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,previpaddr,coinstr,buf0,MGW.M,MGW.N,MGW.srv64bits,MGW.N,buf1,email,buyNXT);
                    }
                }
                else if ( strcmp(methodstr,"setmultisig") == 0 )
                    str = setmultisig(SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,previpaddr,sender,jsonstr);
                else if ( strcmp(methodstr,"getmsigpubkey") == 0 && coinstr[0] != 0 )
                {
                    copy_cJSON(buf0,cJSON_GetObjectItem(json,"refNXTaddr"));
                    copy_cJSON(buf1,cJSON_GetObjectItem(json,"myacctcoinaddr"));
                    copy_cJSON(buf2,cJSON_GetObjectItem(json,"mypubkey"));
                    str = getmsigpubkey(SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,previpaddr,sender,coinstr,buf0,buf1,buf2);
                }
                else if ( strcmp(methodstr,"setmsigpubkey") == 0 && coinstr[0] != 0 )
                {
                    copy_cJSON(buf0,cJSON_GetObjectItem(json,"refNXTaddr"));
                    copy_cJSON(buf1,cJSON_GetObjectItem(json,"addr"));
                    copy_cJSON(buf2,cJSON_GetObjectItem(json,"userpubkey"));
                    str = setmsigpubkey(SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,previpaddr,sender,coinstr,buf0,buf1,buf2);
                } else sprintf(retbuf,"{\"error\":\"unsupported method\",\"method\":\"%s\"}",methodstr);
                if ( str != 0 )
                {
                    strcpy(retbuf,str);
                    free(str);
                }
            }
        }
    }
    printf("<<<<<<<<<<<< INSIDE PLUGIN.(%s) initflag.%d process %s\n",SUPERNET.myNXTaddr,initflag,plugin->name);
    return((int32_t)strlen(retbuf));
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
    // runtime specific state can be created and put into *data
    return(disableflags); // set bits corresponding to array position in _methods[]
}

#include "../plugin777.c"
