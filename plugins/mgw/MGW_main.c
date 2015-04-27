//
//  echodemo.c
//  SuperNET API extension example plugin
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

#define BUNDLED
#define PLUGINSTR "MGW"
#define PLUGNAME(NAME) MGW ## NAME
#define STRUCTNAME struct PLUGNAME(_info)
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
#include "plugin777.c"
#include "storage.c"
#include "system777.c"
#undef DEFINES_ONLY

STRUCTNAME
{
    // this will be at the end of the plugins structure and will be called with all zeros to _init
};
char *PLUGNAME(_methods)[] = { "stats", "echo2" }; // list of supported methods

uint64_t PLUGNAME(_init)(struct plugin_info *plugin,STRUCTNAME *data)
{
    uint64_t disableflags = 0;
    
    printf("init %s size.%ld\n",plugin->name,sizeof(struct MGW_info));
    return(disableflags); // set bits corresponding to array position in _methods[]
}

int32_t PLUGNAME(_process_json)(struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag)
{
    char onetimestr[64],resultstr[MAX_JSON_FIELD],*str;
    retbuf[0] = 0;
    //printf("<<<<<<<<<<<< INSIDE PLUGIN! process %s (%s)\n",plugin->name,jsonstr);
    if ( initflag > 0 )
    {
        strcpy(retbuf,"{\"result\":\"return JSON init\"}");
    }
    else
    {
        if ( plugin_result(retbuf,json) > 0 )
            return((int32_t)strlen(retbuf));
        copy_cJSON(resultstr,cJSON_GetObjectItem(json,"result"));
        if ( strcmp(resultstr,"registered") == 0 )
            plugin->registered = 1;
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
#include "../plugin777.c"
