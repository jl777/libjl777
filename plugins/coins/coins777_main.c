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

void ensure_packedptrs(struct coin777 *coin)
{
    uint32_t newmax;
    coin->RTblocknum = _get_RTheight(&coin->lastgetinfo,coin->name,coin->serverport,coin->userpass,coin->RTblocknum);
    newmax = (uint32_t)(coin->RTblocknum * 1.1);
    if ( coin->maxpackedblocks < newmax )
    {
        coin->packed = realloc(coin->packed,sizeof(*coin->packed) * newmax);
        memset(&coin->packed[coin->maxpackedblocks],0,sizeof(*coin->packed) * (newmax - coin->maxpackedblocks));
        coin->maxpackedblocks = newmax;
    }
}

void coins_verify(struct coin777 *coin,struct packedblock *packed,uint32_t blocknum)
{
    int32_t i;
    ram_clear_rawblock(&coin->DECODE,1);
    coin777_unpackblock(&coin->DECODE,packed,blocknum);
    if ( memcmp(&coin->DECODE,&coin->EMIT,sizeof(coin->DECODE)) != 0 )
    {
        for (i=0; i<sizeof(coin->DECODE); i++)
            if ( ((uint8_t *)&coin->DECODE)[i] != ((uint8_t *)&coin->DECODE)[i] )
                break;
        printf("packblock decode error blocknum.%u\n",coin->readahead);
        coin777_disprawblock(&coin->EMIT);
        printf("----> \n");
        coin777_disprawblock(&coin->DECODE);
        printf("mismatch\n");
        while ( 1 ) sleep(1);
    } else printf("COMPARED! ");
}

int32_t coins_idle(struct plugin_info *plugin)
{
    int32_t i,flag = 0; uint32_t width = 1000;
    struct coin777 *coin; struct ledger_info *ledger;
    for (i=0; i<COINS.num; i++)
    {
        if ( (coin= COINS.LIST[i]) != 0 && coin->packed != 0 )
        {
            coin->RTblocknum = _get_RTheight(&coin->lastgetinfo,coin->name,coin->serverport,coin->userpass,coin->RTblocknum);
            while ( coin->packedblocknum <= coin->RTblocknum && coin->packedblocknum < coin->packedend )
            {
                ram_clear_rawblock(&coin->EMIT,1);
                if ( rawblock_load(&coin->EMIT,coin->name,coin->serverport,coin->userpass,coin->packedblocknum) > 0 )
                {
                    if ( coin->packed[coin->packedblocknum] == 0 )
                    {
                        if ( (coin->packed[coin->packedblocknum]= coin777_packrawblock(coin,&coin->EMIT)) != 0 )
                        {
                            ramchain_setpackedblock(&coin->ramchain,coin->packed[coin->packedblocknum],coin->packedblocknum);
                            //coins_verify(coin,coin->packed[coin->packedblocknum],coin->packedblocknum);
                            coin->packedblocknum += coin->packedincr;
                        }
                        flag = 1;
                    }
                } else break;
                coin->packedblocknum += coin->packedincr;
            }
            if ( 0 && flag == 0 && (ledger= coin->ramchain.activeledger) != 0 )
            {
                //printf("readahead.%d vs blocknum.%u\n",coin->readahead,ledger->blocknum);
                if ( coin->readahead <= ledger->blocknum )
                    coin->readahead = ledger->blocknum;
                while ( coin->readahead <= ledger->blocknum+width )
                {
                    //printf("readahead.%u %p\n",coin->readahead++,coin->packed[coin->readahead]);
                    if ( coin->packed[coin->readahead] == 0 )
                    {
                        ram_clear_rawblock(&coin->EMIT,1);
                        if ( rawblock_load(&coin->EMIT,coin->name,coin->serverport,coin->userpass,coin->readahead) > 0 )
                        {
                            if ( (coin->packed[coin->readahead]= coin777_packrawblock(coin,&coin->EMIT)) != 0 )
                            {
                                ramchain_setpackedblock(&coin->ramchain,coin->packed[coin->readahead],coin->readahead);
                                //coins_verify(coin,coin->packed[coin->readahead],coin->readahead);
                                width++;
                                if ( coin->readahead > width && coin->readahead-width > ledger->blocknum && coin->packed[coin->readahead-width] != 0 )
                                {
                                    printf("purge.%u\n",coin->readahead-width);
                                    free(coin->packed[coin->readahead-width]), coin->packed[coin->readahead-width] = 0;
                                }
                                
                                flag = 1;
                                break;
                            }
                        } else printf("error rawblock_load.%u\n",coin->readahead);
                    } else coin->readahead++;
                }
            }
        }
    }
    return(flag);
}

STRUCTNAME COINS;
char *PLUGNAME(_methods)[] = { "acctpubkeys",  "packblocks", "sendrawtransaction" };
char *PLUGNAME(_pubmethods)[] = { "acctpubkeys" };
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
    char *coindirs[][3] = { {"BTC","Bitcoin","bitcoin"}, {"BTCD","BitcoinDark"}, {"LTC","Litecoin","litecoin"}, {"VRC","VeriCoin","vericoin"} };
#else
    char *coindirs[][3] = { {"BTC",".bitcoin"}, {"BTCD",".BitcoinDark"}, {"LTC",".litecoin"}, {"VRC",".vericoin"}, {"VPN",".vpncoin"} };
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
    sprintf(fname,"%s/%s/%s",userhome,coindir,confname);
}

uint16_t extract_userpass(char *serverport,char *userpass,char *coinstr,char *userhome,char *coindir,char *confname)
{
    FILE *fp; uint16_t port = 0;
    char fname[2048],line[1024],*rpcuser,*rpcpassword,*rpcport,*str;
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
            port = atoi(rpcport);
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

struct coin777 *coin777_create(char *coinstr,cJSON *argjson)
{
    struct coin777 *coin = calloc(1,sizeof(*coin));
    char *serverport,*path=0,*conf=0;
    safecopy(coin->name,coinstr,sizeof(coin->name));
    if ( argjson != 0 )
    {
        coin->jsonstr = cJSON_Print(argjson);
        coin->argjson = cJSON_Duplicate(argjson,1);
        if ( (serverport= cJSON_str(cJSON_GetObjectItem(argjson,"rpc"))) != 0 )
            safecopy(coin->serverport,serverport,sizeof(coin->serverport));
        coin->use_addmultisig = get_API_int(cJSON_GetObjectItem(argjson,"useaddmultisig"),(strcmp("BTC",coinstr) != 0));
        coin->minconfirms = get_API_int(cJSON_GetObjectItem(argjson,"minconfirms"),(strcmp("BTC",coinstr) == 0) ? 3 : 10);
        path = cJSON_str(cJSON_GetObjectItem(argjson,"path"));
        conf = cJSON_str(cJSON_GetObjectItem(argjson,"conf"));
    }
    else coin->minconfirms = (strcmp("BTC",coinstr) == 0) ? 3 : 10;
    extract_userpass(coin->serverport,coin->userpass,coinstr,SUPERNET.userhome,path,conf);
    COINS.LIST = realloc(COINS.LIST,(COINS.num+1) * sizeof(*coin));
    COINS.LIST[COINS.num] = coin, COINS.num++;
    //ensure_packedptrs(coin);
    return(coin);
}

struct coin777 *coin777_find(char *coinstr,int32_t autocreate)
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
    if ( autocreate != 0 )
        return(coin777_create(coinstr,0));
    return(0);
}

int32_t PLUGNAME(_process_json)(struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag)
{
    char *resultstr,sender[MAX_JSON_FIELD],*methodstr,zerobuf[1],*coinstr,*str = 0;
    cJSON *array,*item;
    int32_t i,n,j = 0;
    struct coin777 *coin;
    retbuf[0] = 0;
    if ( initflag > 0 )
    {
        if ( json != 0 )
        {
            COINS.argjson = cJSON_Duplicate(json,1);
            COINS.slicei = get_API_int(cJSON_GetObjectItem(json,"slice"),0);
            copy_cJSON(SUPERNET.userhome,cJSON_GetObjectItem(json,"userdir"));
            if ( SUPERNET.userhome[0] == 0 )
                strcpy(SUPERNET.userhome,"/root");
            if ( (array= cJSON_GetObjectItem(json,"coins")) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=j=0; i<n; i++)
                {
                    item = cJSON_GetArrayItem(array,i);
                    coinstr = cJSON_str(cJSON_GetObjectItem(item,"name"));
                    if ( coinstr != 0 && coinstr[0] != 0 && (coin= coin777_find(coinstr,0)) == 0 )
                        coin777_create(coinstr,item);
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
        printf("COINS.(%s) for (%s) (%s)\n",methodstr,coinstr!=0?coinstr:"",jsonstr);
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
        else
        {
            zerobuf[0] = 0;
            str = 0;
            printf("INSIDE COINS.(%s) methods.%ld\n",jsonstr,sizeof(coins_methods)/sizeof(*coins_methods));
            copy_cJSON(sender,cJSON_GetObjectItem(json,"NXT"));
            if ( coinstr == 0 )
                coinstr = zerobuf;
            else coin = coin777_find(coinstr,1);
            if ( strcmp(methodstr,"acctpubkeys") == 0 )
            {
                if ( SUPERNET.gatewayid >= 0 )
                {
                    if ( coinstr[0] == 0 )
                        strcpy(retbuf,"{\"result\":\"need to specify coin\"}");
                    else if ( (coin= coin777_find(coinstr,1)) != 0 )
                    {
                        if ( (str= get_msig_pubkeys(coin->name,coin->serverport,coin->userpass)) != 0 )
                        {
                            MGW_publish_acctpubkeys(coin->name,str);
                            strcpy(retbuf,"{\"result\":\"published and processed acctpubkeys\"}");
                            free(str), str= 0;
                        } else sprintf(retbuf,"{\"error\":\"no get_msig_pubkeys result\",\"method\":\"%s\"}",methodstr);
                    } else sprintf(retbuf,"{\"error\":\"no coin777\",\"method\":\"%s\"}",methodstr);
                } else sprintf(retbuf,"{\"error\":\"gateway only method\",\"method\":\"%s\"}",methodstr);
            }
            else if ( strcmp(methodstr,"packblocks") == 0 )
            {
                coin->packedblocknum = coin->packedstart = get_API_int(cJSON_GetObjectItem(json,"start"),0) + COINS.slicei;
                coin->packedend = get_API_int(cJSON_GetObjectItem(json,"end"),1000000000);
                coin->packedincr = get_API_int(cJSON_GetObjectItem(json,"incr"),1);
                ensure_packedptrs(coin);
                sprintf(retbuf,"{\"result\":\"packblocks\",\"start\":\"%u\",\"end\":\"%u\",\"incr\":\"%u\",\"RTblocknum\":\"%u\"}",coin->packedstart,coin->packedend,coin->packedincr,coin->RTblocknum);
            }
            else sprintf(retbuf,"{\"error\":\"unsupported method\",\"method\":\"%s\"}",methodstr);
            if ( str != 0 )
            {
                strcpy(retbuf,str);
                free(str);
            }
        }
    }
    printf("<<<<<<<<<<<< INSIDE PLUGIN.(%s) initflag.%d process %s slice.%d\n",SUPERNET.myNXTaddr,initflag,plugin->name,COINS.slicei);
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
    COINS.readyflag = 1;
    // runtime specific state can be created and put into *data
    return(disableflags); // set bits corresponding to array position in _methods[]
}

#include "../plugin777.c"
