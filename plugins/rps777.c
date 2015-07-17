//
//  rps777.c
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

#define BUNDLED
#define PLUGINSTR "rps"
#define PLUGNAME(NAME) rps ## NAME
#define STRUCTNAME struct PLUGNAME(_info) 
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
#include "plugin777.c"
#include "kv777.c"
#undef DEFINES_ONLY

#define DEFAULT_RPSPORT 8193
struct rps_info
{
    int32_t readyflag;
};
STRUCTNAME RPS;

int32_t rps_idle(struct plugin_info *plugin)
{
    return(0);
}

char *bet(char *retbuf,long maxlen,struct protocol_info *prot,cJSON *json,char *jsonstr,char *tokenstr,char *forwarder,char *sender,int32_t valid)
{
    return(0);
}

char *status(char *retbuf,long maxlen,struct protocol_info *prot,cJSON *json,char *jsonstr,char *tokenstr,char *forwarder,char *sender,int32_t valid)
{
    return(0);
}

char *sendcoin(char *retbuf,long maxlen,struct protocol_info *prot,cJSON *json,char *jsonstr,char *tokenstr,char *forwarder,char *sender,int32_t valid)
{
    return(0);
}

char *balance(char *retbuf,long maxlen,struct protocol_info *prot,cJSON *json,char *jsonstr,char *tokenstr,char *forwarder,char *sender,int32_t valid)
{
    return(0);
}

char *deposit(char *retbuf,long maxlen,struct protocol_info *prot,cJSON *json,char *jsonstr,char *tokenstr,char *forwarder,char *sender,int32_t valid)
{
    return(0);
}

char *withdraw(char *retbuf,long maxlen,struct protocol_info *prot,cJSON *json,char *jsonstr,char *tokenstr,char *forwarder,char *sender,int32_t valid)
{
    return(0);
}

typedef char *(*rpsfunc)(char *retbuf,long maxlen,struct protocol_info *prot,cJSON *json,char *jsonstr,char *tokenstr,char *forwarder,char *sender,int32_t valid);
#define RPS_COMMANDS "ping", "endpoint", "sendcoin", "balance", "deposit", "withdraw", "bet", "status"
rpsfunc PLUGNAME(_functions)[] = { protocol_ping, protocol_endpoint, sendcoin, balance, deposit, withdraw, bet, status };

char *PLUGNAME(_methods)[] = { RPS_COMMANDS };
char *PLUGNAME(_pubmethods)[] = { "status", "ping" };
char *PLUGNAME(_authmethods)[] = { "status", "ping" };

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *argjson)
{
    uint64_t disableflags = 0;
    return(disableflags); // set bits corresponding to array position in _methods[]
}

int32_t PLUGNAME(_process_json)(char *forwarder,char *sender,int32_t valid,struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag,char *tokenstr)
{
    char *resultstr,*methodstr,*retstr = 0; int32_t i; double pingmillis = 60000;
    retbuf[0] = 0;
    printf("<<<<<<<<<<<< INSIDE PLUGIN! process %s (%s)\n",plugin->name,jsonstr);
    if ( initflag > 0 )
    {
        RPS.readyflag = 1;
        plugin->allowremote = 1;
        agent_initprotocol(plugin,json,plugin->name,"RPS","rps",pingmillis,0);
        sprintf(retbuf,"{\"result\":\"initialized RPS\",\"pluginNXT\":\"%s\",\"serviceNXT\":\"%s\"}",plugin->NXTADDR,plugin->SERVICENXT);
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        methodstr = jstr(json,"method");
        resultstr = jstr(json,"result");
        if ( methodstr == 0 || methodstr[0] == 0 )
        {
            printf("(%s) has not method\n",jsonstr);
            return(0);
        }
        else if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
        for (i=0; i<sizeof(PLUGNAME(_methods))/sizeof(*PLUGNAME(_methods)); i++)
            if ( strcmp(PLUGNAME(_methods)[i],methodstr) == 0 )
                retstr = (*PLUGNAME(_functions)[i])(retbuf,maxlen,&plugin->protocol,json,jsonstr,tokenstr,forwarder,sender,valid);
    }
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
