//
//  bitcoind_RPC.c
//  Created by jl777, Mar 27, 2014
//  MIT License
//
// based on example from http://curl.haxx.se/libcurl/c/getinmemory.html and util.c from cpuminer.c

#ifndef JL777_BITCOIND_RPC
#define JL777_BITCOIND_RPC

#include <curl/curl.h>
#include <curl/easy.h>

#include "cJSON.h"


double milliseconds(void)
{
    static struct timeval timeval,first_timeval;
    gettimeofday(&timeval,0);
    if ( first_timeval.tv_sec == 0 )
    {
        first_timeval = timeval;
        return(0);
    }
    return((timeval.tv_sec - first_timeval.tv_sec) * 1000. + (timeval.tv_usec - first_timeval.tv_usec)/1000.);
}

#define EXTRACT_BITCOIND_RESULT     // if defined, ensures error is null and returns the "result" field
#ifdef EXTRACT_BITCOIND_RESULT

char *post_process_bitcoind_RPC(char *debugstr,char *command,char *rpcstr)
{
    long i,j,len;
    char *retstr = 0;
    cJSON *json,*result,*error;
    if ( command == 0 || rpcstr == 0 || rpcstr[0] == 0 )
        return(rpcstr);
    //printf("%s post_process_bitcoind_RPC.%s.[%s]\n",debugstr,command,rpcstr);
    json = cJSON_Parse(rpcstr);
    if ( json == 0 )
    {
        printf("%s post_process_bitcoind_RPC.%s can't parse.(%s)\n",debugstr,command,rpcstr);
        free(rpcstr);
        return(0);
    }
    result = cJSON_GetObjectItem(json,"result");
    error = cJSON_GetObjectItem(json,"error");
    if ( error != 0 && result != 0 )
    {
        if ( (error->type&0xff) == cJSON_NULL && (result->type&0xff) != cJSON_NULL )
        {
            retstr = cJSON_Print(result);
            len = strlen(retstr);
            if ( retstr[0] == '"' && retstr[len-1] == '"' )
            {
                for (i=1,j=0; i<len-1; i++,j++)
                    retstr[j] = retstr[i];
                retstr[j] = 0;
            }
        }
        else if ( (error->type&0xff) != cJSON_NULL || (result->type&0xff) != cJSON_NULL )
            printf("%s post_process_bitcoind_RPC (%s) error.%s\n",debugstr,command,rpcstr);
    }
    free(rpcstr);
    free_json(json);
    return(retstr);
}
#endif

struct upload_buffer { const unsigned char *buf; size_t len; };
struct MemoryStruct { char *memory; size_t size; };

static size_t WriteMemoryCallback(void *ptr,size_t size,size_t nmemb,void *data)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)data;
    //printf("WriteMemoryCallback\n");
    mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory) {
        memcpy(&(mem->memory[mem->size]), ptr, realsize);
        mem->size += realsize;
        mem->memory[mem->size] = 0;
    }
    return realsize;
}

double milliseconds();

char *bitcoind_RPC(CURL *curl_handle,char *debugstr,char *url,char *userpass,char *command,char *params)
{
    static uint32_t count,count2,numcurls,numcurlretries,numrecovered,numfails;
    static double elapsedsum,elapsedsum2,laststart;
    char *bracket0,*bracket1,*databuf = 0;
    struct curl_slist *headers = NULL;
    CURLcode res;
    long len;
    int32_t i,pause;
    double starttime;
    struct MemoryStruct chunk;
    if ( 0 && curl_handle == 0 )
    {
        printf("bitcoind_RPC: null curl_handle??\n");
        return(0);
    }
#ifndef __APPLE__
#ifdef WIN32
    pause = 3;
#else
    pause = 3;
#endif
#else
    pause = 2;
#endif
    if ( 0 && laststart+pause > milliseconds() ) // horrible hack for bitcoind "Couldn't connect to server"
        usleep(pause*1000);
    
    starttime = milliseconds();
    numcurls++;
    for (i=0; i<10; i++)
    {
        memset(&chunk,0,sizeof(chunk));
        chunk.memory = (char *)calloc(1,1);     // will be grown as needed by the realloc above
        chunk.size = 0;                 // no data at this point
//#ifndef __APPLE__
        curl_handle = curl_easy_init();
//#else
//        curl_easy_reset(curl_handle);
//#endif
        curl_easy_setopt(curl_handle,CURLOPT_WRITEFUNCTION,WriteMemoryCallback); // send all data to this function
        curl_easy_setopt(curl_handle,CURLOPT_WRITEDATA,(void *)&chunk); // we pass our 'chunk' struct to the callback
        //curl_easy_setopt(curl_handle,CURLOPT_USERAGENT,"libcurl-agent/1.0"); // some servers don't like requests that are made without a user-agent field, so we provide one
        curl_easy_setopt(curl_handle,CURLOPT_CONNECTTIMEOUT,10); // chanc3r: limit any *real* timeouts to ten seconds
        curl_easy_setopt(curl_handle,CURLOPT_NOSIGNAL,1);        // supposed to fix "Alarm clock" and long jump crash
     
        //curl_easy_setopt(curl_handle,CURLOPT_FRESH_CONNECT,1);  // chanc3r: force a new connection for each request
        curl_easy_setopt(curl_handle,CURLOPT_URL,url);
        
        headers = curl_slist_append(0,"Expect:");
        curl_easy_setopt(curl_handle,CURLOPT_HTTPHEADER,headers);
        if ( userpass != 0 )
            curl_easy_setopt(curl_handle,CURLOPT_USERPWD,userpass);
        databuf = 0;
        if ( params != 0 )
        {
            if ( command != 0 )
            {
                len = strlen(params);
                if ( len > 0 && params[0] == '[' && params[len-1] == ']' )
                    bracket0 = bracket1 = (char *)"";
                else bracket0 = (char *)"[", bracket1 = (char *)"]";
                databuf = (char *)malloc(4096 + strlen(command) + strlen(params));
                sprintf(databuf,"{\"id\":\"jl777\",\"method\":\"%s\",\"params\":%s%s%s}",command,bracket0,params,bracket1);
            }
            curl_easy_setopt(curl_handle,CURLOPT_POST,1);
            if ( databuf != 0 )
                curl_easy_setopt(curl_handle,CURLOPT_POSTFIELDS,databuf);
            else curl_easy_setopt(curl_handle,CURLOPT_POSTFIELDS,params);
        }
        laststart = milliseconds();
        res = curl_easy_perform(curl_handle);
//#ifndef __APPLE__
        curl_easy_cleanup(curl_handle);
//#endif
        if ( databuf != 0 )
            free(databuf), databuf = 0;
        curl_slist_free_all(headers);
       // printf("chunk.mem.(%s)\n",chunk.memory);
        if ( res != CURLE_OK )
            fprintf(stderr, "curl_easy_perform() failed: %s %s.(%s %s %s)\n",curl_easy_strerror(res),debugstr,url,command,params);
        else //if ( chunk.memory[0] != 0 )
        {
            if ( i > 0 )
            {
                numrecovered++;
                printf("retries.%d total retries.%d, numcurls.%d numrecovered.%d numfails.%d\n",i,numcurlretries,numcurls,numrecovered,numfails);
            }
            if ( command != 0 )
            {
                count++;
                elapsedsum += (milliseconds() - starttime);
                if ( (count % 1000) == 999)
                    fprintf(stderr,"%d: ave %9.6f | elapsed %.3f millis | bitcoind_RPC.(%s)\n",count,elapsedsum/count,(milliseconds() - starttime),command);
                return(post_process_bitcoind_RPC(debugstr,command,chunk.memory));
            }
            else
            {
                count2++;
                elapsedsum2 += (milliseconds() - starttime);
                if ( (count2 % 1000) == 999)
                    fprintf(stderr,"%d: ave %9.6f | elapsed %.3f millis | NXT calls.(%s)\n",count2,elapsedsum2/count2,(double)(milliseconds() - starttime),url);
                return(chunk.memory);
            }
        }
        free(chunk.memory);
        numcurlretries++;
        sleep(1);
    }
    numfails++;
    printf("NOT GOOD. ");
    printf("retries.%d total retries.%d, numcurls.%d numrecovered.%d numfails.%d\n",i,numcurlretries,numcurls,numrecovered,numfails);
    return(0);
}


#endif

