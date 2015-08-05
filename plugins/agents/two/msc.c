//
//  msc.c
//

//#define BUNDLED
#define PLUGINSTR "msc"
#define PLUGNAME(NAME) msc ## NAME
#define STRUCTNAME struct PLUGNAME(_info)
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)
#include "../utils/bitcoind_RPC.c"
#include <stdio.h>
#define DEFINES_ONLY
#include "../plugin777.c"
#undef DEFINES_ONLY

int32_t msc_idle(struct plugin_info *plugin) { return(0); }

STRUCTNAME
{
    int32_t pad;
};
char *PLUGNAME(_methods)[] = { "msccall" }; // list of supported methods approved for local access
char *PLUGNAME(_pubmethods)[] = { "msccall" }; // list of supported methods approved for public (Internet) access
char *PLUGNAME(_authmethods)[] = { "msccall"  }; // list of supported methods that require authentication

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *argjson)
{
    uint64_t disableflags = 0;
    printf("init %s size.%ld\n",plugin->name,sizeof(struct msc_info));
    // runtime specific state can be created and put into *data
    return(disableflags); // set bits corresponding to array position in _methods[]
}

char *msccall(char *method,char *params){
    char *retstr = bitcoind_RPC(0,0,"http://localhost:8332","mscserver:publicpass",method,params);
    return retstr;
}

int32_t PLUGNAME(_process_json)(struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag)
{
    char callstr[MAX_JSON_FIELD],params[MAX_JSON_FIELD],*resultstr,*methodstr;
    retbuf[0] = 0;
    //printf("<<<<<<<<<<<< INSIDE PLUGIN! process %s (%s)\n",plugin->name,jsonstr);
    if ( initflag > 0 )
    {
        // configure settings
        plugin->allowremote = 1;
        strcpy(retbuf,"{\"result\":\"msc init\"}");
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        methodstr = cJSON_str(cJSON_GetObjectItem(json,"method"));
        copy_cJSON(callstr,cJSON_GetObjectItem(json,"callstr"));
        copy_cJSON(params,cJSON_GetObjectItem(json,"params"));
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
        else if ( strcmp(methodstr,"msccall") == 0 )
        {
            sprintf(retbuf,"{\"result\":\"%s\"}",msccall(callstr,params));
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

