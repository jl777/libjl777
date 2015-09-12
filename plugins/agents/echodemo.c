
/**********************************************************************************
 * The MIT License (MIT)                                                          *
 *                                                                                *
 * Copyright © 2014-2015 The SuperNET Developers.                                 *
 *                                                                                *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy  *
 *  of this software and associated documentation files (the "Software"), to deal *
 *  in the Software without restriction, including without limitation the rights  *
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell     *
 *  copies of the Software, and to permit persons to whom the Software is         *
 *  furnished to do so, subject to the following conditions:                      *
 *                                                                                *
 *  The above copyright notice and this permission notice shall be included in    *
 *  all copies or substantial portions of the Software.                           *
 *                                                                                *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR    *
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,      *
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE   *
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER        *
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, *
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN     *
 *  THE SOFTWARE.                                                                 *
 *                                                                                *
 * Removal or modification of this copyright notice is prohibited.                *
 *                                                                                *
 **********************************************************************************/

#define PLUGINSTR "echodemo"
#define PLUGNAME(NAME) echodemo ## NAME
#define STRUCTNAME struct PLUGNAME(_info) 
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
#include "plugin777.c"
#include "../includes/cJSON.h"
#undef DEFINES_ONLY

STRUCTNAME
{
    int32_t pad;
    // this will be at the end of the plugins structure and will be called with all zeros to _init
};

int32_t echodemo_idle(struct plugin_info *plugin) { return(0); }

char *PLUGNAME(_methods)[] = { "echo", "passthru", "RS" }; // list of supported methods approved for local access
char *PLUGNAME(_pubmethods)[] = { "echo", "passthru", "RS" }; // list of supported methods approved for public (Internet) access
char *PLUGNAME(_authmethods)[] = { "echo", "passthru", "RS" }; // list of supported methods that require authentication

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
    char *resultstr,*methodstr,*addr,*retstr = 0; struct destbuf echostr;
    retbuf[0] = 0;
    plugin->allowremote = 1;
    //fprintf(stderr,"<<<<<<<<<<<< INSIDE PLUGIN! process %s (%s)\n",plugin->name,jsonstr);
    if ( initflag > 0 )
    {
        // configure settings
        strcpy(retbuf,"{\"result\":\"echodemo init\"}");
    }
    else
    {
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        methodstr = cJSON_str(cJSON_GetObjectItem(json,"method"));
        copy_cJSON(&echostr,cJSON_GetObjectItem(json,"echostr"));
        retbuf[0] = 0;
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
            return((int32_t)strlen(retbuf));
        }
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        if ( methodstr == 0 || methodstr[0] == 0 )
        {
            printf("(%s) has not method\n",jsonstr);
            return(0);
        }
        else if ( strcmp(methodstr,"echo") == 0 )
        {
            sprintf(retbuf,"{\"result\":\"%s\"}",echostr.buf);
        }
        else if ( strcmp(methodstr,"passthru") == 0 )
        {
            if ( jstr(json,"destplugin") != 0 )
            {
                cJSON_DeleteItemFromObject(json,"plugin");
                jaddstr(json,"plugin",jstr(json,"destplugin"));
                cJSON_DeleteItemFromObject(json,"destplugin");
            }
            if ( jstr(json,"destmethod") != 0 )
            {
                cJSON_DeleteItemFromObject(json,"method");
                jaddstr(json,"method",jstr(json,"destmethod"));
                cJSON_DeleteItemFromObject(json,"destmethod");
            }
            jaddstr(json,"pluginrequest","SuperNET");
            retstr = jprint(json,0);
            //printf("passhru.(%s)\n",retstr);
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
