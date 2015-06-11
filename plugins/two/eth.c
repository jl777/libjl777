//
//  eth.c
//

//#define BUNDLED
#define PLUGINSTR "eth"
#define PLUGNAME(NAME) eth ## NAME
#define STRUCTNAME struct PLUGNAME(_info)
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)
#include "../utils/bitcoind_RPC.c"
#include <stdio.h>
#define DEFINES_ONLY
#include "../plugin777.c"
#undef DEFINES_ONLY
#define issue_curl(curl_handle,cmdstr,method,params) bitcoind_RPC(curl_handle,"curl",cmdstr,0,method,params)
#define fetch_URL(curl_handle,cmdstr) bitcoind_RPC(curl_handle,"fetch",cmdstr,0,0,0)

int32_t eth_idle(struct plugin_info *plugin) { return(0); }

STRUCTNAME
{
    int32_t pad;
};
char *PLUGNAME(_methods)[] = { "ethcall" }; // list of supported methods approved for local access
char *PLUGNAME(_pubmethods)[] = { "ethcall" }; // list of supported methods approved for public (Internet) access
char *PLUGNAME(_authmethods)[] = { "ethcall" }; // list of supported methods that require authentication

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *argjson)
{
    uint64_t disableflags = 0;
    printf("init %s size.%ld\n",plugin->name,sizeof(struct eth_info));
    // runtime specific state can be created and put into *data
    return(disableflags); // set bits corresponding to array position in _methods[]
}
char *web3call(char *method){
    char *ethresp = issue_curl(0,"http://localhost:8545",method,"[]");
    return ethresp;
}
int32_t PLUGNAME(_process_json)(struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag)
{
    char ethcallstr[MAX_JSON_FIELD],*resultstr,*methodstr;
    retbuf[0] = 0;
    //printf("<<<<<<<<<<<< INSIDE PLUGIN! process %s (%s)\n",plugin->name,jsonstr);
    if ( initflag > 0 )
    {
        // configure settings
        plugin->allowremote = 1;
        strcpy(retbuf,"{\"result\":\"eth init\"}");
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        methodstr = cJSON_str(cJSON_GetObjectItem(json,"method"));
        copy_cJSON(ethcallstr,cJSON_GetObjectItem(json,"ethcallstr"));
        retbuf[0] = 0;
        if ( methodstr == 0 || methodstr[0] == 0 )
        {
            printf("(%s) has not method\n",jsonstr);
            return(0);
        }
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
        else if ( strcmp(methodstr,"ethcall") == 0 )
        {
            sprintf(retbuf,"{\"result\":\"%s\"}",web3call(ethcallstr));
        }
    }
    return((int32_t)strlen(retbuf));
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    return(retcode);
}
#include "../plugin777.c"


