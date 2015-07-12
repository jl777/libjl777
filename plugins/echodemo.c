//
//  echodemo.c
//  SuperNET API extension example agent
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

//#define BUNDLED
#define PLUGINSTR "echodemo"
#define PLUGNAME(NAME) echodemo ## NAME
#define STRUCTNAME struct PLUGNAME(_info) 
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
#include "plugin777.c"
#undef DEFINES_ONLY

STRUCTNAME
{
    int32_t pad;
    // this will be at the end of the plugins structure and will be called with all zeros to _init
};

int32_t echodemo_idle(struct plugin_info *plugin) { return(0); }

char *PLUGNAME(_methods)[] = { "echo", "RS" }; // list of supported methods approved for local access
char *PLUGNAME(_pubmethods)[] = { "echo", "RS" }; // list of supported methods approved for public (Internet) access
char *PLUGNAME(_authmethods)[] = { "echo", "RS" }; // list of supported methods that require authentication

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *argjson)
{
    uint64_t disableflags = 0;
    printf("init %s size.%ld\n",plugin->name,sizeof(struct echodemo_info));
    plugin->allowremote = 1;
    // runtime specific state can be created and put into *data
    return(disableflags); // set bits corresponding to array position in _methods[]
}

int32_t PLUGNAME(_process_json)(char *forwarder,char *sender,int32_t valid,struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag,char *tokenstr)
{
    char echostr[MAX_JSON_FIELD],*resultstr,*methodstr,*addr;
    retbuf[0] = 0;
    plugin->allowremote = 1;
    //printf("<<<<<<<<<<<< INSIDE PLUGIN! process %s (%s)\n",plugin->name,jsonstr);
    if ( initflag > 0 )
    {
        // configure settings
        strcpy(retbuf,"{\"result\":\"echodemo init\"}");
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        methodstr = cJSON_str(cJSON_GetObjectItem(json,"method"));
        copy_cJSON(echostr,cJSON_GetObjectItem(json,"echostr"));
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
        else if ( strcmp(methodstr,"echo") == 0 )
        {
            sprintf(retbuf,"{\"result\":\"%s\"}",echostr);
        }
        else if ( strcmp(methodstr,"RS") == 0 )
        {
            int32_t is_decimalstr(char *str);
            uint64_t RS_decode(char *rs);
            int32_t RS_encode(char *,uint64_t id);
            char rsaddr[64]; uint64_t nxt64bits = 0;
            if ( (addr= cJSON_str(cJSON_GetObjectItem(json,"addr"))) != 0 )
            {
                rsaddr[0] = 0;
                if ( strlen(addr) > 4 )
                {
                    if ( strncmp(addr,"NXT-",4) == 0 )
                    {
                        nxt64bits = RS_decode(addr);
                        sprintf(retbuf,"{\"result\":\"success\",\"accountRS\":\"%s\",\"account\":\"%llu\"}",addr,(long long)nxt64bits);
                    }
                    else if ( is_decimalstr(addr) != 0 )
                    {
                        nxt64bits = calc_nxt64bits(addr), RS_encode(rsaddr,nxt64bits);
                        sprintf(retbuf,"{\"result\":\"success\",\"account\":\"%llu\",\"accountRS\":\"%s\"}",(long long)nxt64bits,rsaddr);
                    }
                }
                else sprintf(retbuf,"{\"error\":\"illegal addr field\",\"addr\":\"%s\"}",addr);
            }
            else sprintf(retbuf,"{\"error\":\"no addr field\"}");
        }
    }
    return((int32_t)strlen(retbuf) + retbuf[0] != 0);
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    return(retcode);
}
#include "plugin777.c"
