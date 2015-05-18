//
//  echodemo.c
//  SuperNET API extension example plugin
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

#define BUNDLED
#define PLUGINSTR "echo"
#define PLUGNAME(NAME) echo ## NAME
#define STRUCTNAME struct PLUGNAME(_info) 
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
#include "plugin777.c"
#undef DEFINES_ONLY

int32_t echo_idle(struct plugin_info *plugin) { return(0); }

STRUCTNAME
{
    int32_t pad;
    // this will be at the end of the plugins structure and will be called with all zeros to _init
};
char *PLUGNAME(_methods)[] = { "echo" }; // list of supported methods approved for local access
char *PLUGNAME(_pubmethods)[] = { "echo" }; // list of supported methods approved for public (Internet) access
char *PLUGNAME(_authmethods)[] = { "echo" }; // list of supported methods that require authentication

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *argjson)
{
    uint64_t disableflags = 0;
    printf("init %s size.%ld\n",plugin->name,sizeof(struct echo_info));
    // runtime specific state can be created and put into *data
    return(disableflags); // set bits corresponding to array position in _methods[]
}

int32_t PLUGNAME(_process_json)(struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag)
{
    char onetimestr[64],*resultstr,*str,*methodstr;
    retbuf[0] = 0;
    //printf("<<<<<<<<<<<< INSIDE PLUGIN! process %s (%s)\n",plugin->name,jsonstr);
    if ( initflag > 0 )
    {
        // configure settings
        strcpy(retbuf,"{\"result\":\"initflag > 0\"}");
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        methodstr = cJSON_str(cJSON_GetObjectItem(json,"method"));
        if ( methodstr == 0 || methodstr[0] == 0 )
        {
            printf("(%s) has not method\n",jsonstr);
            return(0);
        }
        printf("ECHO.(%s)\n",methodstr);
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
        else
        {
            str = stringifyM(jsonstr);
            if ( initflag < 0 )
                sprintf(onetimestr,",\"onetime\":%d",initflag);
            else onetimestr[0] = 0;
            sprintf(retbuf,"{\"tag\":\"%llu\",\"args\":%s,\"milliseconds\":%f%s}\n",(long long)tag,str,milliseconds(),onetimestr);
            free(str);
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
#include "plugin777.c"
