//
//  tradebot.h
//  xcode
//
//  Created by jl777 on 7/27/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_tradebot_h
#define xcode_tradebot_h

#define TRADEBOT_PRICECHANGE 1
#define TRADEBOT_NEWMINUTE 2

struct InstantDEX_state
{
    struct price_data *dp;
    struct exchange_state **eps;
    uint64_t obookid;
    int32_t numexchanges,event;
};

struct tradebot
{
    void *compiled,*privatedata;
    void **signals;
    char *botname,**signalnames;
    uint32_t lastupdatetime;
    int32_t botid,numsignals,metalevel,disabled,numsources,*signalsizes;
    struct tradebot **sources;
};

struct tradebot_language
{
    char name[64];
    int32_t (*compiler_func)(void **compiledptr,char *retjsonstr,cJSON *codejson);
    int32_t (*runtime_func)(uint32_t jdatetime,struct tradebot *bot,struct InstantDEX_state *state);
    struct tradebot **tradebots;
    int32_t numtradebots,maxtradebots;
} **Languages;
int32_t Num_languages,Max_metalevel;

struct basic_tradebot_data
{
    double *inputs[16];
};

int32_t basic_tradebot_runtime(uint32_t jdatetime,struct tradebot *bot,struct InstantDEX_state *state)
{
    struct basic_tradebot_data *bp = bot->privatedata;
    printf("event.%d %s bot.(%s) id.%d %s %s\n",state->event,jdatetime_str(jdatetime),bot->botname,bot->botid,state->dp->base,state->dp->rel);
    return(0);
}

struct tradebot *find_tradebot(char *lang,char *botname,int32_t botid)
{
    int32_t i,l;
    struct tradebot *bot;
    struct tradebot_language *lp;
    for (l=0; l<Num_languages; l++)
    {
        lp = Languages[l];
        if ( strcmp(lang,lp->name) != 0 )
            continue;
        for (i=0; i<lp->numtradebots; i++)
        {
            bot = lp->tradebots[i];
            if ( bot->disabled == 0 && strcmp(botname,bot->botname) == 0 && (botid < 0 || botid == bot->botid) )
                return(bot);
        }
    }
    return(0);
}

void *get_signal_ptr(char *lang,char *botname,int32_t botid,char *signalname)
{
    int32_t i;
    struct tradebot *bot;
    if ( (bot= find_tradebot(lang,botname,botid)) != 0 )
    {
        for (i=0; i<bot->numsignals; i++)
            if ( strcmp(bot->signalnames[i],signalname) == 0 )
                return(bot->signals[i]);
    }
    return(0);
}

int32_t basic_tradebot_compiler(void **compiledptr,char *retbuf,cJSON *codejson)
{
    int32_t i,n,inbotid,smoother,offset,numsignals=0,numinputs=0,metalevel=0,signalsize,outputsize = 0;
    cJSON *signalobj,*item;
    char exchange[512],base[512],rel[512],inbotname[512],inputsignal[512],inlang[512];
    signalobj = cJSON_GetObjectItem(codejson,"signals");
    if ( signalobj != 0 && is_cJSON_Array(signalobj) != 0 )
    {
        numsignals = n = cJSON_GetArraySize(signalobj);
        for (i=0; i<n; i++)
        {
            item = cJSON_GetArrayItem(signalobj,i);
            signalsize = (int32_t)get_cJSON_int(item,"size");
            if ( signalsize <= 0 )
                signalsize = sizeof(double);
            printf("%d of %d: size.%d\n",i,n,signalsize);
            outputsize += signalsize;
        }
    }
    signalobj = cJSON_GetObjectItem(codejson,"inputs");
    if ( signalobj != 0 && is_cJSON_Array(signalobj) != 0 )
    {
        numinputs = n = cJSON_GetArraySize(signalobj);
        for (i=0; i<n; i++)
        {
            item = cJSON_GetArrayItem(signalobj,i);
            extract_cJSON_str(exchange,sizeof(exchange),signalobj,"exchange");
            extract_cJSON_str(base,sizeof(base),signalobj,"base");
            extract_cJSON_str(rel,sizeof(rel),signalobj,"rel");
            if ( extract_cJSON_str(inlang,sizeof(inlang),signalobj,"inlang") <= 0 )
                strcpy(inlang,"btl");
            extract_cJSON_str(inbotname,sizeof(inbotname),signalobj,"inbot");
            extract_cJSON_str(inputsignal,sizeof(inputsignal),signalobj,"insig");
            inbotid = (int32_t)get_cJSON_int(signalobj,"inbotid");
            offset = (int32_t)get_cJSON_int(signalobj,"offset");
            smoother = (int32_t)get_cJSON_int(signalobj,"offset");
            printf("%d of %d: x.(%s) (%s)/(%s) lang.(%s) inbot.(%s) id.%d sig.(%s) offset.%d smooth.%d\n",i,n,exchange,base,rel,inlang,inbotname,inbotid,inputsignal,offset,smoother);
        }
    }
    *compiledptr = calloc(1,outputsize + sizeof(struct basic_tradebot_data));
    sprintf(retbuf,"{\"metalevel\":%d,\"numinputs\":%d,\"numsignals\":%d,\"status\":\"compiled\"}",metalevel,numinputs,numsignals);
    return(0);
}

int32_t add_compiled(struct tradebot_language *lp,void *compiled,cJSON *botjson)
{
    int32_t i,n;
    void *ptr;
    long signalsize;
    char signalname[1024],botname[1024];
    cJSON *signalobj,*item;
    struct tradebot *tradebot;
    if ( compiled == 0 )
        return(-1);
    if ( lp->maxtradebots >= lp->numtradebots )
    {
        lp->maxtradebots++;
        lp->tradebots = realloc(lp->tradebots,sizeof(*lp->tradebots) * lp->maxtradebots);
    }
    tradebot = calloc(1,sizeof(*tradebot));
    if ( botjson != 0 )
    {
        extract_cJSON_str(botname,sizeof(botname),botjson,"botname");
        if ( botname[0] != 0 )
            tradebot->botname = clonestr(botname);
        else tradebot->botname = clonestr("bot with no name");
        tradebot->botid = (int32_t)get_cJSON_int(botjson,"botid");
        signalobj = cJSON_GetObjectItem(botjson,"signals");
        if ( signalobj != 0 && is_cJSON_Array(signalobj) != 0 )
        {
            tradebot->numsignals = n = cJSON_GetArraySize(signalobj);
            if ( n > 0 )
            {
                tradebot->signals = calloc(n,sizeof(*tradebot->signals));
                tradebot->signalsizes = calloc(n,sizeof(*tradebot->signalsizes));
                tradebot->signalnames = calloc(n,sizeof(*tradebot->signalnames));
            }
            ptr = compiled;
            for (i=0; i<n; i++)
            {
                item = cJSON_GetArrayItem(signalobj,i);
                tradebot->signals[i] = ptr;
                if ( extract_cJSON_str(signalname,sizeof(signalname),item,"name") > 0 )
                    tradebot->signalnames[i] = clonestr(signalname);
                else tradebot->signalnames[i] = clonestr("unnamed");
                signalsize = (long)get_cJSON_int(item,"size");
                if ( signalsize <= 0 )
                    signalsize = sizeof(double);
                tradebot->signalsizes[i] = (int32_t)signalsize;
                printf("%d of %d: \"%s\".id%d %s compiled.%p ptr.%p (%s).size%ld\n",i,n,tradebot->botname,tradebot->botid,tradebot->signalnames[i],compiled,ptr,signalname,signalsize);
                ptr = (void *)((long)ptr + signalsize);
            }
            tradebot->privatedata = ptr;
        }
        //free_json(json);
    }
    tradebot->compiled = compiled;
    lp->tradebots[lp->numtradebots++] = tradebot;
    return(lp->numtradebots);
}

int32_t register_tradebot_language(char *name,int32_t (*compiler_func)(void **compiledptr,char *retjsonstr,cJSON *codejson),int32_t (*runtime_func)(uint32_t jdatetime,struct tradebot *bot,struct InstantDEX_state *state))
{
    int32_t i;
    struct tradebot_language *lp;
    if ( Num_languages > 0 )
    {
        for (i=0; i<Num_languages; i++)
        {
            if ( strcmp(name,Languages[i]->name) == 0 )
            {
                printf("cant register (%s) already existing tradebot language\n",name);
                return(-1);
            }
        }
    }
    Languages = realloc(Languages,sizeof(*Languages) * (Num_languages + 1));
    lp = calloc(1,sizeof(struct tradebot_language));
    safecopy(lp->name,name,sizeof(lp->name));
    lp->compiler_func = compiler_func;
    lp->runtime_func = runtime_func;
    Languages[Num_languages++] = lp;
    return(Num_languages);
}

int32_t init_tradebots(cJSON *tradebot_languages)
{
    cJSON *item;
    int32_t i,n;
    uv_lib_t lib;
    void *compiler,*runtime;
    char dyldfname[1024],*langname;
    register_tradebot_language("btl",basic_tradebot_compiler,basic_tradebot_runtime);
    if ( tradebot_languages != 0 && is_cJSON_Array(tradebot_languages) != 0 )
    {
        n = cJSON_GetArraySize(tradebot_languages);
        for (i=0; i<n; i++)
        {
            item = cJSON_GetArrayItem(tradebot_languages,i);
            copy_cJSON(dyldfname,item);
            if ( dyldfname[0] != 0 )
            {
                printf("try to load tradebot_language in (%s)\n",dyldfname);
                if ( uv_dlopen(dyldfname,&lib) == 0 )
                {
                    if ( uv_dlsym(&lib,"langname",(void **)&langname) == 0 )
                    {
                        printf("found langname.(%s)!\n",langname);
                        if ( uv_dlsym(&lib,"compiler",&compiler) == 0 )
                        {
                            if ( uv_dlsym(&lib,"runtime",&runtime) == 0 )
                                register_tradebot_language(langname,compiler,runtime);
                            else printf("error (%s) trying to find runtime (%s)\n",uv_dlerror(&lib),dyldfname);
                        } else printf("error (%s) trying to find compiler (%s)\n",uv_dlerror(&lib),dyldfname);
                    } else printf("error (%s) trying to find langname (%s)\n",uv_dlerror(&lib),dyldfname);
                } else printf("error (%s) trying to load tradebot_language in (%s)\n",uv_dlerror(&lib),dyldfname);
            }
        }
    }
    return(Num_languages);
}

char *start_tradebot(char *sender,char *NXTACCTSECRET,cJSON *botjson)
{
    int32_t i;
    void *compiled;
    char buf[1024],retbuf[32768];
    printf("tradbot got (%s)\n",cJSON_Print(botjson));
    if ( extract_cJSON_str(buf,sizeof(buf),botjson,"lang") > 0 )
    {
        for (i=0; i<Num_languages; i++)
        {
            if ( strcmp(buf,Languages[i]->name) == 0 )
            {
                if ( (*Languages[i]->compiler_func)(&compiled,retbuf,botjson) == 0 )
                {
                    add_compiled(Languages[i],compiled,botjson);
                    return(clonestr(retbuf));
                }
                else return(clonestr("{\"result\":\"tradebot compiler returns error\"}"));
            }
        }
    }
    return(clonestr("{\"result\":\"unknown tradebot lang(uage) field\"}"));
}

void tradebot_event_processor(uint32_t jdatetime,struct price_data *dp,struct exchange_state **eps,int32_t numexchanges,uint64_t obookid,int32_t newminuteflag)
{
    static int errors;
    struct InstantDEX_state S;
    int32_t metalevel,i,iter,l;
    struct tradebot *bot;
    struct tradebot_language *lp;
    memset(&S,0,sizeof(S));
    S.dp = dp;
    S.eps = eps;
    S.numexchanges = numexchanges;
    S.obookid = obookid;
    if ( newminuteflag != 0 )
        S.event = TRADEBOT_NEWMINUTE;
    else S.event = TRADEBOT_PRICECHANGE;
    for (iter=0; iter<2; iter++)
    for (metalevel=0; metalevel<=Max_metalevel; metalevel++)
    {
        for (l=0; l<Num_languages; l++)
        {
            lp = Languages[l];
            for (i=0; i<lp->numtradebots; i++)
            {
                bot = lp->tradebots[i];
                if ( iter == 1 )
                {
                    if ( bot->disabled == 0 && bot->lastupdatetime != jdatetime )
                    {
                        printf("errors.%d: bot.(%s) botid.%d %s != %s\n",errors,bot->botname,bot->botid,jdatetime_str(bot->lastupdatetime),jdatetime_str2(jdatetime));
                        errors++;
                    }
                }
                else
                {
                    if ( bot->disabled == 0 && bot->metalevel == metalevel )
                    {
                        if ( (*lp->runtime_func)(jdatetime,bot,&S) < 0 )
                        {
                            bot->disabled = 1;
                            if ( bot->compiled != bot->privatedata )
                                memset(bot->compiled,0,(long)bot->privatedata - (long)bot->compiled);
                            printf("bot.(%s) botid.%d disabled: numsignals.%d metalevel.%d\n",bot->botname,bot->botid,bot->numsignals,bot->metalevel);
                        }
                        bot->lastupdatetime = jdatetime;
                    }
                }
            }
        }
    }
}

#endif
