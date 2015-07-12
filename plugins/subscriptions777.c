//
//  subscriptions777.c
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

#define BUNDLED
#define PLUGINSTR "subscriptions"
#define PLUGNAME(NAME) subscriptions ## NAME
#define STRUCTNAME struct PLUGNAME(_info) 
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
#include "plugin777.c"
#undef DEFINES_ONLY

int32_t subscriptions_idle(struct plugin_info *plugin) { return(0); }

STRUCTNAME SUBSCRIPTIONS;
char *PLUGNAME(_methods)[] = { "publish", "category", "subscribe", "unsubscribe", "list" };
char *PLUGNAME(_pubmethods)[] = { "publish", "category", "subscribe", "unsubscribe", "list" };
char *PLUGNAME(_authmethods)[] = { "publish", }; 

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *argjson)
{
    uint64_t disableflags = 0;
    //printf("init %s size.%ld\n",plugin->name,sizeof(struct subscriptions_info));
    // runtime specific state can be created and put into *data
    return(disableflags); // set bits corresponding to array position in _methods[]
}

int32_t add_publication(char *subscription)
{
    SUBSCRIPTIONS.publications = realloc(SUBSCRIPTIONS.publications,sizeof(*SUBSCRIPTIONS.publications) * (SUBSCRIPTIONS.numpubs + 1));
    SUBSCRIPTIONS.publications[SUBSCRIPTIONS.numpubs] = clonestr(subscription);
    return(++SUBSCRIPTIONS.numpubs);
}

char *publish_jsonstr(char *category)
{
    cJSON *json,*array = cJSON_CreateArray();
    int32_t i; char endpoint[MAX_SERVERNAME],*retstr;
    if ( strcmp(category,"*") == 0 )
        category = 0;
    for (i=0; i<SUBSCRIPTIONS.numpubs; i++)
    {
        if ( category == 0 || strncmp(category,SUBSCRIPTIONS.publications[i],strlen(category)) == 0 )
            cJSON_AddItemToArray(array,cJSON_CreateString(SUBSCRIPTIONS.publications[i]));
    }
    json = cJSON_CreateObject();
    expand_epbits(endpoint,calc_epbits("tcp",(uint32_t)calc_ipbits(SUPERNET.myipaddr),SUPERNET.port + nn_portoffset(NN_PUB),NN_PUB));
    cJSON_AddItemToObject(json,"endpoint",cJSON_CreateString(endpoint));
    retstr = cJSON_Print(json);
    free_json(json);
    _stripwhite(retstr,' ');
    return(retstr);
}

int32_t PLUGNAME(_process_json)(char *forwarder,char *sender,int32_t valid,struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag,char *tokenstr)
{
    char *resultstr,*methodstr,*retstr = 0;
    retbuf[0] = 0;
    //printf("<<<<<<<<<<<< INSIDE PLUGIN! process %s (%s)\n",plugin->name,jsonstr);
    if ( initflag > 0 )
    {
        // configure settings
        SUBSCRIPTIONS.readyflag = 1;
        plugin->allowremote = 1;
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
        printf("SUBSCRIPTIONS.(%s)\n",methodstr);
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
        else if ( strcmp(methodstr,"list") == 0 )
            retstr = publish_jsonstr(cJSON_str(cJSON_GetObjectItem(json,"category")));
        else if ( strcmp(methodstr,"publish") == 0 )
            retstr = publish_jsonstr(cJSON_str(cJSON_GetObjectItem(json,"content")));
        if ( retstr != 0 )
        {
            strcpy(retbuf,retstr);
            free(retstr);
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
