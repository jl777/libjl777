//
//  InstantDEX.c
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

#define BUNDLED
#define PLUGINSTR "InstantDEX"
#define PLUGNAME(NAME) InstantDEX ## NAME
#define STRUCTNAME struct PLUGNAME(_info) 
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
#include "../plugin777.c"
#include "../utils/NXT777.c"
#undef DEFINES_ONLY

#include "InstantDEX.h"
typedef char *(*json_handler)(int32_t localaccess,int32_t valid,char *sender,cJSON **objs,int32_t numobjs,char *origargstr);

int32_t InstantDEX_idle(struct plugin_info *plugin)
{
    poll_pending_offers(SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET);
    return(0);
}

char *PLUGNAME(_methods)[] = { "makeoffer3", "allorderbooks", "orderbook", "lottostats", "cancelquote", "openorders", "placebid", "placeask", "respondtx", "jumptrades", "tradehistory", "msigaddr", "setmsigaddr", "LSUM" }; // list of supported methods approved for local access
char *PLUGNAME(_pubmethods)[] = { "bid", "ask", "makeoffer3", "msigaddr", "setmsigaddr", "LSUM", "orderbook" }; // list of supported methods approved for public (Internet) access
char *PLUGNAME(_authmethods)[] = { "echo" }; // list of supported methods that require authentication

char *makeoffer3_func(int32_t localaccess,int32_t valid,char *sender,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char *retstr = 0;
    //printf("makeoffer3 localaccess.%d\n",localaccess);
    if ( valid > 0 )
        retstr = call_makeoffer3(localaccess,SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,objs);
    return(retstr);
}

char *respondtx_func(int32_t localaccess,int32_t valid,char *sender,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char cmdstr[MAX_JSON_FIELD],triggerhash[MAX_JSON_FIELD],utx[MAX_JSON_FIELD],offerNXT[MAX_JSON_FIELD],sig[MAX_JSON_FIELD],*retstr = 0;
    uint64_t quoteid,assetid,qty,priceNQT,otherassetid,otherqty;
    int32_t minperc;
    printf("got respond_tx.(%s)\n",origargstr);
    if ( localaccess == 0 )
        return(0);
    copy_cJSON(cmdstr,objs[0]);
    assetid = get_API_nxt64bits(objs[1]);
    qty = get_API_nxt64bits(objs[2]);
    priceNQT = get_API_nxt64bits(objs[3]);
    copy_cJSON(triggerhash,objs[4]);
    quoteid = get_API_nxt64bits(objs[5]);
    copy_cJSON(sig,objs[6]);
    copy_cJSON(utx,objs[7]);
    minperc = (int32_t)get_API_int(objs[8],INSTANTDEX_MINVOL);
    //if ( localaccess != 0 )
        copy_cJSON(offerNXT,objs[9]);
    otherassetid = get_API_nxt64bits(objs[10]);
    otherqty = get_API_nxt64bits(objs[11]);
    if ( strcmp(offerNXT,SUPERNET.NXTADDR) == 0 && valid > 0 && triggerhash[0] != 0 )
        retstr = respondtx(SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,offerNXT,cmdstr,assetid,qty,priceNQT,triggerhash,quoteid,sig,utx,minperc,otherassetid,otherqty);
    //else retstr = clonestr("{\"result\":\"invalid respondtx_func request\"}");
    return(retstr);
}

char *InstantDEX_parser(char *forwarder,char *sender,int32_t valid,char *origargstr,cJSON *origargjson)
{
    static char *allorderbooks[] = { (char *)allorderbooks_func, "allorderbooks", "", 0 };
    static char *orderbook[] = { (char *)orderbook_func, "orderbook", "", "baseid", "relid", "allfields", "oldest", "maxdepth", "base", "rel", "gui", "showall", "exchange", 0 };
    static char *lottostats[] = { (char *)lottostats_func, "lottostats", "", "timestamp", 0 };
    static char *cancelquote[] = { (char *)cancelquote_func, "cancelquote", "", "quoteid", 0 };
    static char *openorders[] = { (char *)openorders_func, "openorders", "", 0 };
    static char *placebid[] = { (char *)placebid_func, "placebid", "", "baseid", "relid", "volume", "price", "timestamp", "baseamount", "relamount", "gui", "automatch", "minperc", "duration", "exchange", "offerNXT", 0 };
    static char *placeask[] = { (char *)placeask_func, "placeask", "", "baseid", "relid", "volume", "price", "timestamp", "baseamount", "relamount", ",gui", "automatch", "minperc", "duration", "exchange", "offerNXT", 0 };
    static char *bid[] = { (char *)bid_func, "bid", "", "baseid", "relid", "volume", "price", "timestamp", "baseamount", "relamount", "gui", "automatch", "minperc", "duration", "exchange", "offerNXT", 0 };
    static char *ask[] = { (char *)ask_func, "ask", "", "baseid", "relid", "volume", "price", "timestamp", "baseamount", "relamount", "gui", "automatch", "minperc", "duration", "exchange", "offerNXT", 0 };
    static char *makeoffer3[] = { (char *)makeoffer3_func, "makeoffer3", "", "baseid", "relid", "quoteid", "perc", "deprecated", "baseiQ", "reliQ", "askoffer", "price", "volume", "exchange", "baseamount", "relamount", "offerNXT", "minperc", "jumpasset", 0 };
    static char *respondtx[] = { (char *)respondtx_func, "respondtx", "", "cmd", "assetid", "quantityQNT", "priceNQT", "triggerhash", "quoteid", "sig", "data", "minperc", "offerNXT", "otherassetid", "otherqty", 0 };
    static char *jumptrades[] = { (char *)jumptrades_func, "jumptrades", "", 0 };
    static char *tradehistory[] = { (char *)tradehistory_func, "tradehistory", "", "timestamp", 0 };
    static char **commands[] = { allorderbooks, lottostats, cancelquote, respondtx, jumptrades, tradehistory, openorders, makeoffer3, placebid, bid, placeask, ask, orderbook };
    int32_t i,j,localaccess = 0;
    cJSON *argjson,*obj,*nxtobj,*secretobj,*objs[64];
    char NXTaddr[MAX_JSON_FIELD],NXTACCTSECRET[MAX_JSON_FIELD],command[MAX_JSON_FIELD],offerNXT[MAX_JSON_FIELD],**cmdinfo,*argstr,*retstr=0;
    memset(objs,0,sizeof(objs));
    command[0] = 0;
    valid = 1;
    memset(NXTaddr,0,sizeof(NXTaddr));
    if ( is_cJSON_Array(origargjson) != 0 )
    {
        argjson = cJSON_GetArrayItem(origargjson,0);
        argstr = cJSON_Print(argjson), _stripwhite(argstr,' ');
    } else argjson = origargjson, argstr = origargstr;
    NXTACCTSECRET[0] = 0;
    if ( argjson != 0 )
    {
        if ( (obj= cJSON_GetObjectItem(argjson,"requestType")) == 0 )
            obj = cJSON_GetObjectItem(argjson,"method");
        nxtobj = cJSON_GetObjectItem(argjson,"NXT");
        secretobj = cJSON_GetObjectItem(argjson,"secret");
        copy_cJSON(NXTaddr,nxtobj);
        copy_cJSON(offerNXT,cJSON_GetObjectItem(argjson,"offerNXT"));
        if ( NXTaddr[0] == 0 && offerNXT[0] == 0 )
        {
            strcpy(NXTaddr,SUPERNET.NXTADDR);
            strcpy(offerNXT,SUPERNET.NXTADDR);
            strcpy(sender,SUPERNET.NXTADDR);
            ensure_jsonitem(argjson,"NXT",NXTaddr);
            ensure_jsonitem(argjson,"offerNXT",offerNXT);
        }
        else if ( offerNXT[0] == 0 && strcmp(NXTaddr,SUPERNET.NXTADDR) == 0 )
        {
            strcpy(offerNXT,SUPERNET.NXTADDR);
            strcpy(sender,SUPERNET.NXTADDR);
            ensure_jsonitem(argjson,"offerNXT",offerNXT);
        }
        else strcpy(sender,offerNXT);
        if ( strcmp(offerNXT,SUPERNET.NXTADDR) == 0 )
            localaccess = 1;
        if ( localaccess != 0 && strcmp(NXTaddr,SUPERNET.NXTADDR) != 0 )
        {
            strcpy(NXTaddr,SUPERNET.NXTADDR);
            ensure_jsonitem(argjson,"NXT",NXTaddr);
            //printf("subsititute NXT.%s\n",NXTaddr);
        }
//printf("localaccess.%d myaddr.(%s) NXT.(%s) offerNXT.(%s)\n",localaccess,SUPERNET.NXTADDR,NXTaddr,offerNXT);
        copy_cJSON(command,obj);
        copy_cJSON(NXTACCTSECRET,secretobj);
        if ( NXTACCTSECRET[0] == 0 )
        {
            if ( localaccess != 0 || strcmp(command,"findnode") != 0 )
            {
                safecopy(NXTACCTSECRET,SUPERNET.NXTACCTSECRET,sizeof(NXTACCTSECRET));
                strcpy(NXTaddr,SUPERNET.NXTADDR);
             }
        }
//printf("(%s) argstr.(%s) command.(%s) NXT.(%s) valid.%d\n",cJSON_Print(argjson),argstr,command,NXTaddr,valid);
        //fprintf(stderr,"SuperNET_json_commands sender.(%s) valid.%d | size.%d | command.(%s) orig.(%s)\n",sender,valid,(int32_t)(sizeof(commands)/sizeof(*commands)),command,origargstr);
        for (i=0; i<(int32_t)(sizeof(commands)/sizeof(*commands)); i++)
        {
            cmdinfo = commands[i];
            if ( strcmp(cmdinfo[1],command) == 0 )
            {
                //printf("needvalid.(%c) valid.%d %d of %d: cmd.(%s) vs command.(%s)\n",cmdinfo[2][0],valid,i,(int32_t)(sizeof(commands)/sizeof(*commands)),cmdinfo[1],command);
                if ( cmdinfo[2][0] != 0 && valid <= 0 )
                    return(0);
                for (j=3; cmdinfo[j]!=0&&j<3+(int32_t)(sizeof(objs)/sizeof(*objs)); j++)
                    objs[j-3] = cJSON_GetObjectItem(argjson,cmdinfo[j]);
                retstr = (*(json_handler)cmdinfo[0])(localaccess,valid,sender,objs,j-3,argstr);
                break;
            }
        }
    } else printf("not JSON to parse?\n");
    if ( argstr != origargstr )
        free(argstr);
    return(retstr);
}

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *argjson)
{
    uint64_t disableflags = 0;
    printf("init %s size.%ld\n",plugin->name,sizeof(struct InstantDEX_info));
    // runtime specific state can be created and put into *data
    return(disableflags); // set bits corresponding to array position in _methods[]
}

int32_t PLUGNAME(_process_json)(char *forwarder,char *sender,int32_t valid,struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag,char *tokenstr)
{
    char *resultstr,*methodstr,*retstr = 0;
    retbuf[0] = 0;
    if ( Debuglevel > 2 )
        fprintf(stderr,"<<<<<<<<<<<< INSIDE PLUGIN! process %s (%s)\n",plugin->name,jsonstr);
    if ( initflag > 0 )
    {
        // configure settings
        plugin->allowremote = 1;
        portable_mutex_init(&plugin->mutex);
        init_InstantDEX(calc_nxt64bits(SUPERNET.NXTADDR),0);
        update_NXT_assettrades();
        INSTANTDEX.readyflag = 1;
        strcpy(retbuf,"{\"result\":\"InstantDEX init\"}");
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        if ( (methodstr= cJSON_str(cJSON_GetObjectItem(json,"method"))) == 0 )
            methodstr = cJSON_str(cJSON_GetObjectItem(json,"requestType"));
        retbuf[0] = 0;
        if ( methodstr == 0 || methodstr[0] == 0 )
        {
            printf("(%s) has not method\n",jsonstr);
            return(0);
        }
        portable_mutex_lock(&plugin->mutex);
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
        else if ( strcmp(methodstr,"msigaddr") == 0 )
        {
            char *devMGW_command(char *jsonstr,cJSON *json);
            if ( SUPERNET.gatewayid >= 0 )
            {
                if ( (retstr= devMGW_command(jsonstr,json)) != 0 )
                {
                    //should_forward(sender,retstr);
                }
            } //else retstr = nn_loadbalanced((uint8_t *)jsonstr,(int32_t)strlen(jsonstr)+1);
        }
        else if ( strcmp(methodstr,"LSUM") == 0 )
        {
            sprintf(retbuf,"{\"result\":\"%s\",\"amount\":%d}",(rand() & 1) ? "BUY" : "SELL",(rand() % 100) * 100000);
            retstr = clonestr(retbuf);
        }
        else if ( SUPERNET.iamrelay == 0 )
        {
            retstr = InstantDEX_parser(forwarder,sender,valid,jsonstr,json);
//printf("InstantDEX_parser return.(%s)\n",retstr);
        } else retstr = clonestr("{\"result\":\"relays only relay\"}");
        if ( retstr != 0 )
        {
            if ( strlen(retstr) >= maxlen-1 )
                retstr[maxlen-1] = 0;
            strcpy(retbuf,retstr);
            free(retstr);
        }
        //else sprintf(retbuf,"{\"error\":\"method %s not found\"}",methodstr);
        portable_mutex_unlock(&plugin->mutex);
    }
    return((int32_t)strlen(retbuf) + retbuf[0]!=0);
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    return(retcode);
}
#include "../plugin777.c"
