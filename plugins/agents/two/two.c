//
//  two.c
//

//#define BUNDLED
#define PLUGINSTR "two"
#define PLUGNAME(NAME) two ## NAME
#define STRUCTNAME struct PLUGNAME(_info)
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)
#include "../utils/bitcoind_RPC.c"
#include <stdio.h>
#define DEFINES_ONLY
#include "../plugin777.c"
#undef DEFINES_ONLY
#define issue_curl(cmdstr) bitcoind_RPC(0,"curl",cmdstr,0,0,0)
#define issue_NXTPOST(cmdstr) bitcoind_RPC(0,"curl","http://localhost:7876",0,0,cmdstr)
#define issue_CRYPTIPOST(cmdstr) bitcoind_RPC(0,"curl","http://localhost:6040",0,0,cmdstr)
#define issue_RIPPLEPOST(cmdstr) bitcoind_RPC(0,"curl","http://s1.ripple.com:51234",0,0,cmdstr)

int32_t two_idle(struct plugin_info *plugin) { return(0); }

STRUCTNAME
{
    int32_t pad;
};
char *PLUGNAME(_methods)[] = { "nxtcall", "qoracall", "nxtpost", "crypticall", "cryptipost", "ripplepost", "xcpcall" }; // list of supported methods approved for local access
char *PLUGNAME(_pubmethods)[] = { "nxtcall", "qoracall", "nxtpost", "crypticall", "cryptipost", "ripplepost", "xcpcall" }; // list of supported methods approved for public (Internet) access
char *PLUGNAME(_authmethods)[] = { "nxtcall", "qoracall", "nxtpost", "crypticall", "cryptipost", "ripplepost", "xcpcall"  }; // list of supported methods that require authentication

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *argjson)
{
    uint64_t disableflags = 0;
    printf("init %s size.%ld\n",plugin->name,sizeof(struct two_info));
    // runtime specific state can be created and put into *data
    return(disableflags); // set bits corresponding to array position in _methods[]
}
char *qoracall(char *method){
    char *qurl1 = "http://localhost:9085/";
    char *qurl2 = method;
    char *qurl = (char *) malloc(1 + strlen(qurl1)+ strlen(qurl2) );
    strcpy(qurl, qurl1);
    strcat(qurl, qurl2);
    char *qoraresp = issue_curl(qurl);
    return qoraresp;
}
char *nxtcall(char *method){
    char *qurl1 = "http://localhost:7876/";
    char *qurl2 = method;
    char *qurl = (char *) malloc(1 + strlen(qurl1)+ strlen(qurl2) );
    strcpy(qurl, qurl1);
    strcat(qurl, qurl2);
    char *nxtresp = issue_curl(qurl);
    return nxtresp;
}

char *xcpcall(char *method){
    char *qurl1 = "http://localhost:4000/";
    char *qurl2 = method;
    char *qurl = (char *) malloc(1 + strlen(qurl1)+ strlen(qurl2) );
    strcpy(qurl, qurl1);
    strcat(qurl, qurl2);
    char *xcpresp = issue_curl(qurl);
    return xcpresp;
}

char *crypticall(char *method){
    char *qurl1 = "http://localhost:6040/api/";
    char *qurl2 = method;
    char *qurl = (char *) malloc(1 + strlen(qurl1)+ strlen(qurl2) );
    strcpy(qurl, qurl1);
    strcat(qurl, qurl2);
    char *cryptiresp = issue_curl(qurl);
    return cryptiresp;
}

int32_t PLUGNAME(_process_json)(struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag)
{
    char callstr[MAX_JSON_FIELD],*resultstr,*methodstr;
    retbuf[0] = 0;
    //printf("<<<<<<<<<<<< INSIDE PLUGIN! process %s (%s)\n",plugin->name,jsonstr);
    if ( initflag > 0 )
    {
        // configure settings
        plugin->allowremote = 1;
        strcpy(retbuf,"{\"result\":\"two init\"}");
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        methodstr = cJSON_str(cJSON_GetObjectItem(json,"method"));
        copy_cJSON(callstr,cJSON_GetObjectItem(json,"callstr"));
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
        else if ( strcmp(methodstr,"nxtcall") == 0 )
        {
            sprintf(retbuf,"{\"result\":\"%s\"}",nxtcall(callstr));
        }
        else if ( strcmp(methodstr,"nxtpost") == 0 )
        {
            sprintf(retbuf,"{\"result\":\"%s\"}",issue_NXTPOST(callstr));
        }
        else if ( strcmp(methodstr,"crypticall") == 0 )
        {
            sprintf(retbuf,"{\"result\":\"%s\"}",crypticall(callstr));
        }
        else if ( strcmp(methodstr,"cryptipost") == 0 )
        {
            sprintf(retbuf,"{\"result\":\"%s\"}",issue_CRYPTIPOST(callstr));
        }
        else if ( strcmp(methodstr,"qoracall") == 0 )
        {
            sprintf(retbuf,"{\"result\":\"%s\"}",qoracall(callstr));
        }
        else if ( strcmp(methodstr,"ripplepost") == 0 )
        {
            sprintf(retbuf,"{\"result\":\"%s\"}",issue_RIPPLEPOST(callstr));
        }
        else if ( strcmp(methodstr,"xcpcall") == 0 )
        {
            sprintf(retbuf,"{\"result\":\"%s\"}",xcpcall(callstr));
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

