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

#include "../includes/cJSON.h"
#define DEFINES_ONLY
#include "../common/system777.c"
#include "../utils/utils777.c"
//#undef DEFINES_ONLY

// return data from the server
struct return_string {
    char *ptr;
    size_t len;
};

size_t accumulate(void *ptr, size_t size, size_t nmemb, struct return_string *s);
void init_string(struct return_string *s);


/************************************************************************
 *
 * return the current system time in milliseconds
 *
 ************************************************************************/

#define EXTRACT_BITCOIND_RESULT     // if defined, ensures error is null and returns the "result" field
#ifdef EXTRACT_BITCOIND_RESULT

/************************************************************************
 *
 * perform post processing of the results
 *
 ************************************************************************/

char *post_process_bitcoind_RPC(char *debugstr,char *command,char *rpcstr,char *params)
{
    long i,j,len;
    char *retstr = 0;
    cJSON *json,*result,*error;
    if ( command == 0 || rpcstr == 0 || rpcstr[0] == 0 )
    {
        //fprintf(stderr,"<<<<<<<<<<< bitcoind_RPC: %s post_process_bitcoind_RPC.%s.[%s]\n",debugstr,command,rpcstr);
        return(rpcstr);
    }
    json = cJSON_Parse(rpcstr);
    if ( json == 0 )
    {
        fprintf(stderr,"<<<<<<<<<<< bitcoind_RPC: %s post_process_bitcoind_RPC.%s can't parse.(%s) params.(%s)\n",debugstr,command,rpcstr,params);
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
            fprintf(stderr,"<<<<<<<<<<< bitcoind_RPC: %s post_process_bitcoind_RPC (%s) error.%s\n",debugstr,command,rpcstr);
        free(rpcstr);
    } else retstr = rpcstr;
    free_json(json);
    //fprintf(stderr,"<<<<<<<<<<< bitcoind_RPC: postprocess returns.(%s)\n",retstr);
    return(retstr);
}
#endif

/************************************************************************
 *
 * perform the query
 *
 ************************************************************************/

char *bitcoind_RPC(char **retstrp,char *debugstr,char *url,char *userpass,char *command,char *params)
{
    static int count,count2;
    static double elapsedsum,elapsedsum2;
    char *bracket0,*bracket1,*databuf = 0;
    struct curl_slist *headers = NULL;
    struct return_string s;
    CURLcode res;
    CURL *curl_handle;
    long len;
    int32_t specialcase,numretries;
    double starttime;
    numretries = 0;
    if ( debugstr != 0 && strcmp(debugstr,"BTCD") == 0 && command != 0 && strcmp(command,"SuperNET") ==  0 )
        specialcase = 1;
    else specialcase = 0;
    if ( specialcase != 0 && 0 )
        fprintf(stderr,"<<<<<<<<<<< bitcoind_RPC: debug.(%s) url.(%s) command.(%s) params.(%s)\n",debugstr,url,command,params);
try_again:
    if ( retstrp != 0 )
        *retstrp = 0;
    starttime = milliseconds();
    curl_handle = curl_easy_init();
    init_string(&s);
    headers = curl_slist_append(0,"Expect:");
    
  	curl_easy_setopt(curl_handle,CURLOPT_USERAGENT,"mozilla/4.0");//"Mozilla/4.0 (compatible; )");
    curl_easy_setopt(curl_handle,CURLOPT_HTTPHEADER,	headers);
    curl_easy_setopt(curl_handle,CURLOPT_URL,		url);
    curl_easy_setopt(curl_handle,CURLOPT_WRITEFUNCTION,	accumulate); 		// send all data to this function
    curl_easy_setopt(curl_handle,CURLOPT_WRITEDATA,		&s); 			// we pass our 's' struct to the callback
    curl_easy_setopt(curl_handle,CURLOPT_NOSIGNAL,		1L);   			// supposed to fix "Alarm clock" and long jump crash
	curl_easy_setopt(curl_handle,CURLOPT_NOPROGRESS,	1L);			// no progress callback
    if ( strncmp(url,"https",5) == 0 )
    {
        curl_easy_setopt(curl_handle,CURLOPT_SSL_VERIFYPEER,0);
        curl_easy_setopt(curl_handle,CURLOPT_SSL_VERIFYHOST,0);
    }
    if ( userpass != 0 )
        curl_easy_setopt(curl_handle,CURLOPT_USERPWD,	userpass);
    databuf = 0;
    if ( params != 0 )
    {
        if ( command != 0 && specialcase == 0 )
        {
            len = strlen(params);
            if ( len > 0 && params[0] == '[' && params[len-1] == ']' ) {
                bracket0 = bracket1 = (char *)"";
            }
            else
            {
                bracket0 = (char *)"[";
                bracket1 = (char *)"]";
            }
            
            databuf = (char *)malloc(256 + strlen(command) + strlen(params));
            sprintf(databuf,"{\"id\":\"jl777\",\"method\":\"%s\",\"params\":%s%s%s}",command,bracket0,params,bracket1);
//fprintf(stderr,"url.(%s) userpass.(%s) databuf.(%s)\n",url,userpass,databuf);
            //
        } //else if ( specialcase != 0 ) fprintf(stderr,"databuf.(%s)\n",params);
        curl_easy_setopt(curl_handle,CURLOPT_POST,1L);
        if ( databuf != 0 )
            curl_easy_setopt(curl_handle,CURLOPT_POSTFIELDS,databuf);
        else curl_easy_setopt(curl_handle,CURLOPT_POSTFIELDS,params);
    }
    //laststart = milliseconds();
    res = curl_easy_perform(curl_handle);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl_handle);
    if ( databuf != 0 ) // clean up temporary buffer
    {
        free(databuf);
        databuf = 0;
    }
    if ( res != CURLE_OK )
    {
        numretries++;
        if ( specialcase != 0 )
        {
            fprintf(stderr,"<<<<<<<<<<< bitcoind_RPC.(%s): BTCD.%s timeout params.(%s) s.ptr.(%s) err.%d\n",url,command,params,s.ptr,res);
            free(s.ptr);
            return(0);
        }
        else if ( numretries >= 2 )
        {
            fprintf(stderr,"Maximum number of retries exceeded!\n");
            free(s.ptr);
            return(0);
        }
        fprintf(stderr, "curl_easy_perform() failed: %s %s.(%s %s), retries: %d\n",curl_easy_strerror(res),debugstr,url,command,numretries);
        free(s.ptr);
        portable_sleep((1<<numretries));
        goto try_again;
        
    }
    else
    {
        if ( command != 0 && specialcase == 0 )
        {
            count++;
            elapsedsum += (milliseconds() - starttime);
            if ( (count % 10000) == 0)
                fprintf(stderr,"%d: ave %9.6f | elapsed %.3f millis | bitcoind_RPC.(%s) url.(%s)\n",count,elapsedsum/count,(milliseconds() - starttime),command,url);
            if ( retstrp != 0 )
            {
                *retstrp = s.ptr;
                return(s.ptr);
            }
            return(post_process_bitcoind_RPC(debugstr,command,s.ptr,params));
        }
        else
        {
            if ( 0 && specialcase != 0 )
                fprintf(stderr,"<<<<<<<<<<< bitcoind_RPC: BTCD.(%s) -> (%s)\n",params,s.ptr);
            count2++;
            elapsedsum2 += (milliseconds() - starttime);
            if ( (count2 % 10000) == 0)
                fprintf(stderr,"%d: ave %9.6f | elapsed %.3f millis | NXT calls.(%s) cmd.(%s)\n",count2,elapsedsum2/count2,(double)(milliseconds() - starttime),url,command);
            return(s.ptr);
        }
    }
    fprintf(stderr,"bitcoind_RPC: impossible case\n");
    free(s.ptr);
    return(0);
}



/************************************************************************
 *
 * Initialize the string handler so that it is thread safe
 *
 ************************************************************************/

void init_string(struct return_string *s)
{
    s->len = 0;
    s->ptr = (char *)calloc(1,s->len+1);
    if ( s->ptr == NULL )
    {
        fprintf(stderr,"init_string malloc() failed\n");
        exit(-1);
    }
    s->ptr[0] = '\0';
}

/************************************************************************
 *
 * Use the "writer" to accumulate text until done
 *
 ************************************************************************/

size_t accumulate(void *ptr,size_t size,size_t nmemb,struct return_string *s)
{
    size_t new_len = s->len + size*nmemb;
    s->ptr = (char *)realloc(s->ptr,new_len+1);
    if ( s->ptr == NULL )
    {
        fprintf(stderr, "accumulate realloc() failed\n");
        exit(-1);
    }
    memcpy(s->ptr+s->len,ptr,size*nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;
    return(size * nmemb);
}

struct MemoryStruct { char *memory; size_t size; };

static size_t WriteMemoryCallback(void *ptr,size_t size,size_t nmemb,void *data)
{
    size_t realsize = (size * nmemb);
    struct MemoryStruct *mem = (struct MemoryStruct *)data;
    mem->memory = (ptr != 0) ? realloc(mem->memory,mem->size + realsize + 1) : malloc(mem->size + realsize + 1);
    if ( mem->memory != 0 )
    {
        memcpy(&(mem->memory[mem->size]),ptr,realsize);
        mem->size += realsize;
        mem->memory[mem->size] = 0;
    }
    return(realsize);
}

void *curl_post(CURL **cHandlep,char *url,char *userpass,char *postfields,char *hdr0,char *hdr1,char *hdr2)
{
    struct MemoryStruct chunk; CURL *cHandle; long code; struct curl_slist *headers = 0;
    if ( (cHandle= *cHandlep) == NULL )
		*cHandlep = cHandle = curl_easy_init();
    else curl_easy_reset(cHandle);
//#ifdef DEBUG
	curl_easy_setopt(cHandle,CURLOPT_VERBOSE, 1);
//#endif
	curl_easy_setopt(cHandle,CURLOPT_USERAGENT,"mozilla/4.0");//"Mozilla/4.0 (compatible; )");
	curl_easy_setopt(cHandle,CURLOPT_SSL_VERIFYPEER,0);
	curl_easy_setopt(cHandle,CURLOPT_URL,url);
    //curl_easy_setopt(cHandle,CURLOPT_POST,1);
  	curl_easy_setopt(cHandle,CURLOPT_CONNECTTIMEOUT,10);
    if ( userpass != 0 )
        curl_easy_setopt(cHandle,CURLOPT_USERPWD,userpass);
	if ( postfields != NULL )
		curl_easy_setopt(cHandle,CURLOPT_POSTFIELDS,postfields);
    if ( hdr0 != NULL )
    {
        headers = curl_slist_append(headers,hdr0);
        if ( hdr1 != 0 )
            headers = curl_slist_append(headers,hdr1);
        if ( hdr2 != 0 )
            headers = curl_slist_append(headers,hdr2);
    } //headers = curl_slist_append(0,"Expect:");
    if ( headers != 0 )
        curl_easy_setopt(cHandle,CURLOPT_HTTPHEADER,headers);
    //res = curl_easy_perform(cHandle);
    memset(&chunk,0,sizeof(chunk));
    curl_easy_setopt(cHandle,CURLOPT_WRITEFUNCTION,WriteMemoryCallback);
    curl_easy_setopt(cHandle,CURLOPT_WRITEDATA,(void *)&chunk);
    curl_easy_perform(cHandle);
    curl_easy_getinfo(cHandle,CURLINFO_RESPONSE_CODE,&code);
    if ( code != 200 )
        printf("error: (%s) server responded with code %ld\n",url,code);
    if ( headers != 0 )
        curl_slist_free_all(headers);
    return(chunk.memory);
}

void foo(char *serverport,char *userpass)
{
    static CURL *cHandle;
    char *retstr;
    //sprintf(databuf,"{\"id\":\"jl777\",\"method\":\"%s\",\"params\":%s%s%s}",command,bracket0,params,bracket1);
    retstr = curl_post(&cHandle,serverport,userpass,"{\"method\":\"listreceivedbyaddress\",\"params\":[1 true]}",0,0,0);
    printf("retstr.%p (%s)\n",retstr,retstr!=0?retstr:"");
}
#endif
