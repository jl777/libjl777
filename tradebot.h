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
#define MAX_TRADEBOT_INPUTS (1024*1024)
#define MAX_BOTTYPE_BITS 30
#define MAX_BOTTYPE_VNUMBITS 11
#define MAX_BOTTYPE_ITEMBITS (MAX_BOTTYPE_BITS - 2*MAX_BOTTYPE_VNUMBITS)

struct tradebot_type { uint32_t itembits_sub1:MAX_BOTTYPE_ITEMBITS,vnum:MAX_BOTTYPE_VNUMBITS,vind:MAX_BOTTYPE_VNUMBITS,isfloat:1,hasneg:1; };

struct tradebot
{
    char *botname,**outputnames;
    void *compiled,*codestr;
    void **inputs,**outputs;
    uint32_t lastupdatetime;
    int32_t botid,numoutputs,metalevel,disabled,numinputs,bitpacked;
    struct tradebot_type *outtypes,*intypes;
    uint64_t *inconditioners;
};

struct InstantDEX_state
{
    struct price_data *dp;
    struct exchange_state *ep;
    char exchange[64],base[64],rel[64];
    uint64_t obookid,changedmask;
    int32_t polarity,numexchanges,event;
    uint32_t jdatetime;
    
    int maxbars,numbids,numasks;
    uint64_t *bidnxt,*asknxt;
    double *bids,*asks,*inv_bids,*inv_asks;
    double *bidvols,*askvols,*inv_bidvols,*inv_askvols;
    float *m1,*m2,*m3,*m4,*m5,*m10,*m15,*m30,*h1;
    float *inv_m1,*inv_m2,*inv_m3,*inv_m4,*inv_m5,*inv_m10,*inv_m15,*inv_m30,*inv_h1;
};

struct tradebot_language
{
    char name[64];
    int32_t (*compiler_func)(int32_t *iosizep,void **inputs,struct tradebot_type *intypes,uint64_t *conditioners,int32_t *numinputsp,int32_t *metalevelp,void **compiledptr,char *retbuf,cJSON *codejson);
    int32_t (*runtime_func)(uint32_t jdatetime,struct tradebot *bot,struct InstantDEX_state *state);
    struct tradebot **tradebots;
    int32_t numtradebots,maxtradebots;
} **Languages;
int32_t Num_languages,Max_metalevel;


int32_t calc_conditioned_size(struct tradebot_type intype,uint64_t conditioner)
{
    int32_t insize;
    insize = (((intype.itembits_sub1 + 1) * intype.vnum) >> 3);
    if ( insize == 0 )
        insize = 1;
    if ( conditioner != 0 )
        return(sizeof(double));
    return(insize);
}

int32_t extract_tradebot_item(void *item,int32_t bitoffset,void *ptr,struct tradebot_type intype)
{
    int32_t i,ind,destind,itembits = (intype.itembits_sub1 + 1);
    if ( (itembits & 7) == 0 && bitoffset == 0 )
    {
        itembits >>= 3;
        memcpy(item,(void *)((long)ptr + itembits*intype.vind),itembits);
        return(itembits << 3);
    }
    else
    {
        memset(item,0,(itembits >> 3) + 1);
        ind = itembits * intype.vind;
        destind = bitoffset;
        for (i=0; i<itembits; i++,ind++,destind++)
            if ( GETBIT(ptr,ind) != 0 )
                SETBIT(item,destind);
        return(itembits);
    }
}

double conv_tradebot_type(void *item,struct tradebot_type type)
{
    double val;
    if ( type.isfloat != 0 )
    {
        if ( type.itembits_sub1 == (sizeof(float) - 1) )
            val = *(float *)item;
        else if ( type.itembits_sub1 == (sizeof(double) - 1) )
            val = *(double *)item;
        else return(0.);
        if ( type.hasneg == 0 && val < 0 )
            val = -val;
    }
    else if ( type.hasneg != 0 )
    {
        switch ( (type.itembits_sub1 + 1) )
        {
            case sizeof(int8_t): val = *(int8_t *)item; break;
            case sizeof(int16_t): val = *(int16_t *)item; break;
            case sizeof(int32_t): val = *(int32_t *)item; break;
            case sizeof(int64_t): val = *(int64_t *)item; break;
            default: printf("unexpected itemsize.%d\n",type.itembits_sub1); return(0); break;
        }
    }
    else
    {
        switch ( (type.itembits_sub1 + 1) )
        {
            case sizeof(uint8_t): val = *(uint8_t *)item; break;
            case sizeof(uint16_t): val = *(uint16_t *)item; break;
            case sizeof(uint32_t): val = *(uint32_t *)item; break;
            case sizeof(uint64_t): val = *(uint64_t *)item; break;
            default:
                printf("unexpected conversion of struct item size.%d\n",type.itembits_sub1);
                return(0.);
                break;
        }
    }
    return(val);
}

double condition_item(double oldval,void *item,struct tradebot_type intype,uint64_t conditioner)
{
    double val;
    val = conv_tradebot_type(item,intype);
    return((oldval * .5) + (.5 * val)); // of course need to actually implement conditioners!
}

void prep_picoc(void *ptr)
{
    struct InstantDEX_state *state = ptr;
    void set_jl777vars(uint32_t jdatetime,int event,int changed,char *exchange,char *base,char *rel,double *bids,double *inv_bids,int numbids,double *asks,double *inv_asks,int numasks,float *m1,float *m2,float *m3,float *m4,float *m5,float *m10,float *m15,float *m30,float *h1,float *inv_m1,float *inv_m2,float *inv_m3,float *inv_m4,float *inv_m5,float *inv_m10,float *inv_m15,float *inv_m30,float *inv_h1,int maxbars,double *bidvols,double *inv_bidvols,double *askvols,double *inv_askvols,uint64_t *bidnxt,uint64_t *asknxt);
    set_jl777vars(state->jdatetime,state->event,state->changedmask!=0,state->exchange,state->base,state->rel,state->bids,state->inv_bids,state->numbids,state->asks,state->inv_asks,state->numasks,state->m1,state->m2,state->m3,state->m4,state->m5,state->m10,state->m15,state->m30,state->h1,state->inv_m1,state->inv_m2,state->inv_m3,state->inv_m4,state->inv_m5,state->inv_m10,state->inv_m15,state->inv_m30,state->inv_h1,state->maxbars,state->bidvols,state->inv_bidvols,state->askvols,state->inv_askvols,state->bidnxt,state->asknxt);
   // printf("bids.%p %p, asks.%p %p\n",state->bids,state->inv_bids,state->asks,state->inv_asks);
}

int32_t pico_tradebot_runtime(uint32_t jdatetime,struct tradebot *bot,struct InstantDEX_state *state)
{
    unsigned char item[((1 << MAX_BOTTYPE_ITEMBITS) >> 3) + 1];
    char *argv[16];
    void *dataptr;
    int32_t i,bitoffset,insize,itembits,argc = 0;
    double val;
    dataptr = bot->compiled;
    bitoffset = 0;
    if ( state->event == TRADEBOT_PRICECHANGE )
    {
        for (i=0; i<bot->numinputs; i++)
        {
            if ( bot->bitpacked != 0 )
                bitoffset += extract_tradebot_item(dataptr,bitoffset,bot->inputs[i],bot->intypes[i]);
            else
            {
                memset(item,0,sizeof(item));
                itembits = extract_tradebot_item(item,0,bot->inputs[i],bot->intypes[i]);
                if ( (bitoffset & 7) != 0 )
                {
                    printf("FATAL compiler error i.%d of %d inputs: bitoffset misaligned.%d\n",i,bot->numinputs,bitoffset);
                    return(-1);
                }
                insize = calc_conditioned_size(bot->intypes[i],bot->inconditioners[i]);
                if ( bot->inconditioners[i] != 0 )
                {
                    memcpy(&val,(void *)((long)dataptr + bitoffset),sizeof(val));
                    val = condition_item(val,item,bot->intypes[i],bot->inconditioners[i]);
                    memcpy((void *)((long)dataptr + bitoffset),&val,sizeof(val));
                }
                else memcpy((void *)((long)dataptr + (bitoffset>>3)),item,insize);
                bitoffset += (insize << 3);
            }
        }
    }
    argc = 0;
    argv[argc++] = (char *)bot->compiled;
    argv[argc++] = (char *)bot->outputs[0];
    argv[argc++] = (char *)(long)jdatetime;
    argv[argc++] = (char *)state;
    argv[argc++] = (char *)bot;
    prep_picoc(state);
    val = picoc(argc,argv,clonestr(bot->codestr));

    //printf("%s returns %.15f [%.15f] event.%d %s id.%d %s %s changed.%llx\n",bot->botname,val,*(double *)bot->outputs[0],state->event,jdatetime_str(jdatetime),bot->botid,state->base,state->rel,(long long)state->changedmask);
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

void *get_signal_ptr(int32_t *metalevelp,struct tradebot_type *intypep,char *lang,char *botname,int32_t botid,char *signalname)
{
    int32_t i;
    struct tradebot *bot;
    memset(intypep,0,sizeof(*intypep));
    if ( (bot= find_tradebot(lang,botname,botid)) != 0 )
    {
        for (i=0; i<bot->numoutputs; i++)
            if ( strcmp(bot->outputnames[i],signalname) == 0 )
            {
                *metalevelp = bot->metalevel + 1;
                *intypep = bot->outtypes[i];
                return(bot->outputs[i]);
            }
    }
    return(0);
}

int32_t is_this_type(char **typestrp,char *name)
{
    char *typestr = (*typestrp);
    long len = strlen(name);
    if ( strncmp(name,typestr,len) == 0 )
    {
        (*typestrp) += len;
        return(1);
    }
    else return(0);
}

struct tradebot_type conv_tradebot_typestr(int32_t *itemsizep,char *typestr)
{
    char *origstr = typestr;
    struct tradebot_type bottype;
    int32_t isfloat,vnum,hasneg,itembits = 0;
    isfloat = 0; // default integer
    vnum = 1;     // default scalar
    hasneg = 0;  // default unsigned
    *itemsizep = 0;
    memset(&bottype,0,sizeof(bottype));
    if ( is_this_type(&typestr,"struct") != 0 )
    {
        itembits = atoi(typestr) * 8;
        if ( itembits <= 0 )
        {
            printf("ILLEGAL tradebot struct type (%s)\n",origstr);
            return(bottype);
        }
    }
    else if ( is_this_type(&typestr,"bit") != 0 )
    {
        if ( typestr[0] != 0 )
        {
            itembits = atoi(typestr);
            if ( itembits <= 0 )
            {
                printf("ILLEGAL tradebot numbits type (%s)\n",origstr);
                return(bottype);
            }
        }
        else itembits = 1;
    }
    else
    {
        if ( typestr[0] == 'u' )
        {
            hasneg = 0;
            typestr++;
        }
        else hasneg = 1;
        if ( is_this_type(&typestr,"double") != 0 )
            isfloat = 1, itembits = sizeof(double) * 8;
        else if ( is_this_type(&typestr,"float") != 0 )
            isfloat = 1, itembits = sizeof(float) * 8;
        else if ( is_this_type(&typestr,"int") != 0 )
            itembits = sizeof(int32_t) * 8;
        else if ( is_this_type(&typestr,"long") != 0 )
            itembits = sizeof(int64_t) * 8;
        else if ( is_this_type(&typestr,"short") != 0 )
            itembits = sizeof(int16_t) * 8;
        else if ( is_this_type(&typestr,"char") != 0 )
            itembits = sizeof(int8_t) * 8;
        else
        {
            printf("ILLEGAL tradebot type (%s)\n",origstr);
            return(bottype);
        }
        if ( typestr[0] != 0 )
        {
            vnum = atoi(typestr);
            if ( vnum <= 0 )
            {
                printf("ILLEGAL tradebot vnum type (%s)\n",origstr);
                return(bottype);
            }
        }
    }
    if ( vnum >= (1 << MAX_BOTTYPE_VNUMBITS) )
    {
        printf("ILLEGAL vnum.%d > MAX_BOTTYPE_VNUMBITS %d (%s)\n",vnum,MAX_BOTTYPE_VNUMBITS,origstr);
        return(bottype);
    }
    if ( itembits > (1 << MAX_BOTTYPE_ITEMBITS) )
    {
        printf("ILLEGAL itembits.%d > MAX_BOTTYPE_ITEMBITS %d (%s)\n",vnum,MAX_BOTTYPE_ITEMBITS,origstr);
        return(bottype);
    }
    bottype.isfloat = isfloat;
    bottype.hasneg = hasneg;
    bottype.vnum = vnum;
    bottype.itembits_sub1 = (itembits - 1);
    *itemsizep = ((itembits * vnum) >> 3);
    if ( ((itembits * vnum) & 7) != 0 )
        (*itemsizep)++;
    return(bottype);
}

int32_t pico_tradebot_compiler(int32_t *iosizep,void **inputs,struct tradebot_type *intypes,uint64_t *conditioners,int32_t *numinputsp,int32_t *metalevelp,void **compiledptr,char *retbuf,cJSON *codejson)
{
    struct tradebot_type intype;
    int32_t i,n,tmp,vind,maxmetalevel,codelen,bitsize,bitpack,inbotid,conditioner,numoutputs=0,numinputs=0,metalevel=0,iosize = 0;
    void *input;
    cJSON *signalobj,*item,*codeobj,*typeobj;
    char exchange[512],base[512],rel[512],typestr[512],inbotname[512],inputsignal[512],inlang[512];
    signalobj = cJSON_GetObjectItem(codejson,"inputs");
    *iosizep = *numinputsp = *metalevelp = maxmetalevel = bitsize = bitpack = 0;
    if ( signalobj != 0 && is_cJSON_Array(signalobj) != 0 )
    {
        bitpack = 1;
        numinputs = 0;
        n = cJSON_GetArraySize(signalobj);
        for (i=0; i<n; i++)
        {
            item = cJSON_GetArrayItem(signalobj,i);
            extract_cJSON_str(exchange,sizeof(exchange),signalobj,"exchange");
            extract_cJSON_str(base,sizeof(base),signalobj,"base");
            extract_cJSON_str(rel,sizeof(rel),signalobj,"rel");
            if ( extract_cJSON_str(inlang,sizeof(inlang),signalobj,"inlang") <= 0 )
                strcpy(inlang,"ptl");
            extract_cJSON_str(inbotname,sizeof(inbotname),signalobj,"inbot");
            extract_cJSON_str(inputsignal,sizeof(inputsignal),signalobj,"insig");
            inbotid = (int32_t)get_cJSON_int(signalobj,"inbotid");
            vind = (int32_t)get_cJSON_int(signalobj,"vind");
            conditioner = (int32_t)get_cJSON_int(signalobj,"cond");
            printf("%d of %d: x.(%s) (%s)/(%s) lang.(%s) inbot.(%s) id.%d sig.(%s) vind.%d conditioner.%d\n",i,n,exchange,base,rel,inlang,inbotname,inbotid,inputsignal,vind,conditioner);
            if ( (input= get_signal_ptr(&metalevel,&intype,inlang,inbotname,inbotid,inputsignal)) != 0 )
            {
                if ( metalevel > maxmetalevel )
                    maxmetalevel = metalevel;
                if ( vind > 0 && vind >= intype.vnum )
                {
                    printf("ERROR vind.%d > %d numvector elements???\n",vind,intype.vnum);
                    return(-1);
                }
                intype.vind = vind;
                inputs[numinputs] = input, intypes[numinputs] = intype, conditioners[numinputs] = conditioner, numinputs++;
                if ( conditioner != 0 || (bitsize != 0 && bitsize != (intype.itembits_sub1+1)) )
                    bitpack = 0;
                else if ( bitsize == 0 )
                    bitsize = (intype.itembits_sub1 + 1);
                iosize += calc_conditioned_size(intype,conditioner);
            } else { printf("ERROR cant find input\n"); return(-1); }
        }
        if ( numinputs != 0 )
        {
            printf("ERROR numinputs.%d != n.%d???\n",numinputs,n);
            return(-1);
        }
        else if ( bitpack != 0 && bitsize != 0 )
        {
            printf("BITPACKING! iosize.%d numinputs.%d bitsize.%d -> ",iosize,numinputs,bitsize);
            iosize = (bitsize * numinputs) >> 3;
            if ( ((bitsize * numinputs) & 7) != 0 )
                iosize++;
            printf("bitpacked iosize.%d\n",iosize);
        }
        *numinputsp = numinputs;
    }
    signalobj = cJSON_GetObjectItem(codejson,"outputs");
    if ( signalobj != 0 && is_cJSON_Array(signalobj) != 0 )
    {
        numoutputs = n = cJSON_GetArraySize(signalobj);
        if ( bitpack != 0 && n > 1 )
        {
            printf("can only have a single struct%d output when bitpacking\n",iosize);
            return(-1);
        }
        for (i=0; i<n; i++)
        {
            item = cJSON_GetArrayItem(signalobj,i);
            typeobj = cJSON_GetObjectItem(item,"type");
            copy_cJSON(typestr,typeobj);
            tmp = (bitsize * numinputs) >> 3;
            if ( ((bitsize * numinputs) & 7) != 0 )
                tmp++;
            if ( bitpack != 0 && (strncmp(typestr,"struct",strlen("struct")) != 0 || tmp != iosize) )
            {
                printf("bitpacking output mismatch expected struct%d, got %s bitsize.%d numinputs.%d\n",iosize,typestr,bitsize,numinputs);
                return(-1);
            }
            conv_tradebot_typestr(&tmp,typestr);
            iosize += tmp;
        }
    }
    codeobj = cJSON_GetObjectItem(codejson,"picoc");
    if ( codeobj->valuestring != 0 )
        codelen = (int32_t)(strlen(codeobj->valuestring) + 1);
    else codelen = 0;
    *compiledptr = calloc(1,iosize + codelen);
    if ( codelen != 0 )
        memcpy((void *)((long)*compiledptr + iosize),codeobj->valuestring,codelen);
    sprintf(retbuf,"{\"metalevel\":%d,\"numinputs\":%d,\"numoutputs\":%d,\"IOsize\":\"%d\",\"codelen\":\"%d\"}",maxmetalevel,numinputs,numoutputs,iosize,codelen);
    *metalevelp = maxmetalevel;
    *iosizep = iosize;
    printf("RETBUF.(%s)\n",retbuf);
    return(bitpack);
}

struct tradebot *add_compiled(struct tradebot_language *lp,int32_t expected_iosize,void **inputs,struct tradebot_type *intypes,uint64_t *conditioners,int32_t numinputs,int32_t metalevel,void *compiled,cJSON *botjson)
{
    int32_t i,n,insize,outputsize,iosize = 0;
    void *ptr;
    char signalname[1024],botname[1024],typestr[512];
    cJSON *signalobj,*item,*typeobj;
    struct tradebot *tradebot;
    if ( compiled == 0 )
        return(0);
    if ( lp->maxtradebots >= lp->numtradebots )
    {
        lp->maxtradebots++;
        lp->tradebots = realloc(lp->tradebots,sizeof(*lp->tradebots) * lp->maxtradebots);
        lp->tradebots[lp->numtradebots] = 0;
    }
    tradebot = calloc(1,sizeof(*tradebot));
    ptr = compiled;
    if ( numinputs > 0 )
    {
        tradebot->inputs = calloc(numinputs,sizeof(*tradebot->inputs));
        tradebot->intypes = calloc(numinputs,sizeof(*tradebot->intypes));
        tradebot->inconditioners = calloc(numinputs,sizeof(*tradebot->inconditioners));
        for (i=0; i<numinputs; i++)
        {
            tradebot->inputs[i] = inputs[i];
            tradebot->intypes[i] = intypes[i];
            tradebot->inconditioners[i] = conditioners[i];
            insize = calc_conditioned_size(intypes[i],conditioners[i]);
            ptr = (void *)((long)ptr + insize);
            iosize += insize;
        }
    }
    if ( botjson != 0 )
    {
        extract_cJSON_str(botname,sizeof(botname),botjson,"botname");
        if ( botname[0] != 0 )
            tradebot->botname = clonestr(botname);
        else tradebot->botname = clonestr("bot with no name");
        tradebot->botid = (int32_t)get_cJSON_int(botjson,"botid");
        signalobj = cJSON_GetObjectItem(botjson,"outputs");
        if ( signalobj != 0 && is_cJSON_Array(signalobj) != 0 )
        {
            tradebot->numoutputs = n = cJSON_GetArraySize(signalobj);
            if ( n > 0 )
            {
                tradebot->outputs = calloc(n,sizeof(*tradebot->outputs));
                tradebot->outtypes = calloc(n,sizeof(*tradebot->outtypes));
                tradebot->outputnames = calloc(n,sizeof(*tradebot->outputnames));
            }
            for (i=0; i<n; i++)
            {
                item = cJSON_GetArrayItem(signalobj,i);
                tradebot->outputs[i] = ptr;
                if ( extract_cJSON_str(signalname,sizeof(signalname),item,"name") > 0 )
                    tradebot->outputnames[i] = clonestr(signalname);
                else tradebot->outputnames[i] = clonestr("unnamed");
                typeobj = cJSON_GetObjectItem(item,"type");
                copy_cJSON(typestr,typeobj);
                tradebot->outtypes[i] = conv_tradebot_typestr(&outputsize,typestr);
                printf("%d of %d: \"%s\".id%d %s compiled.%p ptr.%p (%s).type %s\n",i,n,tradebot->botname,tradebot->botid,tradebot->outputnames[i],compiled,ptr,signalname,typestr);
                ptr = (void *)((long)ptr + outputsize);
                iosize += outputsize;
            }
        }
        //free_json(json);
    }
    if ( iosize != expected_iosize )
    {
        printf("COMPILER ERROR! iosize.%d != expected_iosize.%d\n",iosize,expected_iosize);
        free(compiled);
        return(0);
    }
    tradebot->compiled = compiled;
    tradebot->codestr = ptr;
    lp->tradebots[lp->numtradebots++] = tradebot;
    return(tradebot);
}

int32_t register_tradebot_language(char *name,int32_t (*compiler_func)(int32_t *iosizep,void **inputs,struct tradebot_type *intypes,uint64_t *conditioners,int32_t *numinputsp,int32_t *metalevelp,void **compiledptr,char *retbuf,cJSON *codejson),int32_t (*runtime_func)(uint32_t jdatetime,struct tradebot *bot,struct InstantDEX_state *state))
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
    //uv_lib_t lib;
    //void *compiler,*runtime;
    char dyldfname[1024];//,*langname;
    //register_tradebot_language("ptl",pico_tradebot_compiler,pico_tradebot_runtime);
    if ( tradebot_languages != 0 && is_cJSON_Array(tradebot_languages) != 0 )
    {
        n = cJSON_GetArraySize(tradebot_languages);
        for (i=0; i<n; i++)
        {
            item = cJSON_GetArrayItem(tradebot_languages,i);
            copy_cJSON(dyldfname,item);
#ifdef later
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
                            {
                                //register_tradebot_language(langname,compiler,runtime);
                            }
                            else printf("error (%s) trying to find runtime (%s)\n",uv_dlerror(&lib),dyldfname);
                        } else printf("error (%s) trying to find compiler (%s)\n",uv_dlerror(&lib),dyldfname);
                    } else printf("error (%s) trying to find langname (%s)\n",uv_dlerror(&lib),dyldfname);
                } else printf("error (%s) trying to load tradebot_language in (%s)\n",uv_dlerror(&lib),dyldfname);
            }
#endif
        }
    }
    return(Num_languages);
}

char *start_tradebot(char *sender,char *NXTACCTSECRET,cJSON *botjson)
{
    int32_t i,metalevel,numinputs,expected_iosize,bitpacked;
    void *compiled;
    char buf[1024],retbuf[32768];
    void **inputs;
    struct tradebot *bot;
    struct tradebot_type *intypes;
    uint64_t *conditioners;
    printf("tradebot got (%s)\n",cJSON_Print(botjson));
    if ( extract_cJSON_str(buf,sizeof(buf),botjson,"lang") > 0 )
    {
        for (i=0; i<Num_languages; i++)
        {
            if ( strcmp(buf,Languages[i]->name) == 0 )
            {
                inputs = calloc(sizeof(*inputs),MAX_TRADEBOT_INPUTS);
                conditioners = calloc(sizeof(*conditioners),MAX_TRADEBOT_INPUTS);
                intypes = calloc(sizeof(*intypes),MAX_TRADEBOT_INPUTS);
                if ( (bitpacked= (*Languages[i]->compiler_func)(&expected_iosize,inputs,intypes,conditioners,&numinputs,&metalevel,&compiled,retbuf,botjson)) >= 0 )
                {
                    if ( (bot= add_compiled(Languages[i],expected_iosize,inputs,intypes,conditioners,numinputs,metalevel,compiled,botjson)) == 0 )
                        strcpy(retbuf,"{\"result\":\"tradebot second pass compiler returns error\"}");
                    else
                    {
                        bot->bitpacked = bitpacked;
                        if ( metalevel > Max_metalevel )
                        {
                            Max_metalevel = metalevel;
                            printf("New MAX_metalevel!! %d\n",metalevel);
                        }
                    }
                }
                else strcpy(retbuf,"{\"result\":\"tradebot compiler returns error\"}");
                free(inputs); free(conditioners); free(intypes);
                break;
            }
        }
    }
    else strcpy(retbuf,"{\"result\":\"unknown tradebot lang(uage) field\"}");
    return(clonestr(retbuf));
}

void copy_tradebot_ptrs(struct InstantDEX_state *state,struct tradebot_ptrs *ptrs,char *exchange,char *base,char *rel,int polarity)
{
    strcpy(state->exchange,exchange);
    state->jdatetime = ptrs->jdatetime;
    if ( polarity > 0 )
    {
        strcpy(state->base,base);
        strcpy(state->rel,rel);
        state->numbids = ptrs->numbids, state->numasks = ptrs->numasks;
        state->bids = ptrs->bids, state->inv_bids = ptrs->inv_bids, state->asks = ptrs->asks, state->inv_asks = ptrs->inv_asks;
        state->bidvols = ptrs->bidvols, state->inv_bidvols = ptrs->inv_bidvols, state->askvols = ptrs->askvols, state->inv_askvols = ptrs->inv_askvols;
        state->bidnxt = ptrs->bidnxt, state->asknxt = ptrs->asknxt;
        state->m1 = ptrs->m1, state->m2 = ptrs->m2, state->m3 = ptrs->m3, state->m4 = ptrs->m4, state->m5 = ptrs->m5, state->m10 = ptrs->m10, state->m15 = ptrs->m15, state->m30 = ptrs->m30, state->h1 = ptrs->h1;
        state->inv_m1 = ptrs->inv_m1, state->inv_m2 = ptrs->inv_m2, state->inv_m3 = ptrs->inv_m3, state->inv_m4 = ptrs->inv_m4, state->inv_m5 = ptrs->inv_m5, state->inv_m10 = ptrs->inv_m10, state->inv_m15 = ptrs->inv_m15, state->inv_m30 = ptrs->inv_m30, state->inv_h1 = ptrs->inv_h1;
        //printf("bids %f %f, asks %f %f copy tradebot normal\n",state->bids[0],state->inv_bids[0],state->asks[0],state->inv_asks[0]);
    }
    else
    {
        strcpy(state->base,rel);
        strcpy(state->rel,base);
        state->numbids = ptrs->numasks, state->numasks = ptrs->numbids;
        state->bidnxt = ptrs->asknxt, state->asknxt = ptrs->bidnxt;
        state->bids = ptrs->inv_bids, state->inv_bids = ptrs->bids, state->asks = ptrs->inv_asks, state->inv_asks = ptrs->asks;
        state->bidvols = ptrs->inv_bidvols, state->inv_bidvols = ptrs->bidvols, state->askvols = ptrs->inv_askvols, state->inv_askvols = ptrs->askvols;
        state->inv_m1 = ptrs->m1, state->inv_m2 = ptrs->m2, state->inv_m3 = ptrs->m3, state->inv_m4 = ptrs->m4, state->inv_m5 = ptrs->m5, state->inv_m10 = ptrs->m10, state->inv_m15 = ptrs->m15, state->inv_m30 = ptrs->m30, state->inv_h1 = ptrs->h1;
        state->m1 = ptrs->inv_m1, state->m2 = ptrs->inv_m2, state->m3 = ptrs->inv_m3, state->m4 = ptrs->inv_m4, state->m5 = ptrs->inv_m5, state->m10 = ptrs->inv_m10, state->m15 = ptrs->inv_m15, state->m30 = ptrs->inv_m30, state->h1 = ptrs->inv_h1;
        //printf("bids %f %f, asks %f %f copy tradebot invert\n",state->bids[0],state->inv_bids[0],state->asks[0],state->inv_asks[0]);
    }
    state->maxbars = ptrs->maxbars;
}

void tradebot_event_processor(uint32_t jdatetime,struct price_data *dp,struct exchange_state *ep,uint64_t obookid,int32_t newminuteflag,uint64_t changedmask)
{
    static int errors;
    struct InstantDEX_state S;
    int32_t metalevel,i,iter,l;
    struct tradebot *bot;
    struct tradebot_language *lp;
    memset(&S,0,sizeof(S));
    S.dp = dp;
    S.ep = ep;
    S.numexchanges = 1;
    S.obookid = obookid;
    if ( dp != 0 )
    {
        S.polarity = dp->polarity;
        copy_tradebot_ptrs(&S,&dp->PTRS,"ALL",dp->base,dp->rel,dp->polarity);
    }
    else if ( ep != 0 )
    {
        //printf("copy ptrs from ep.%p (%p %p %p %p)\n",ep,ep->P.PTRS.bids,ep->P.PTRS.inv_bids,ep->P.PTRS.asks,ep->P.PTRS.inv_asks);
        copy_tradebot_ptrs(&S,&ep->P.PTRS,ep->name,ep->base,ep->rel,ep->polarity);
    }
    S.changedmask = changedmask;
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
                            if ( bot->compiled != bot->codestr )
                                memset(bot->compiled,0,(long)bot->codestr - (long)bot->compiled);
                            printf("bot.(%s) botid.%d disabled: numoutputs.%d metalevel.%d\n",bot->botname,bot->botid,bot->numoutputs,bot->metalevel);
                        }
                        bot->lastupdatetime = jdatetime;
                    }
                }
            }
        }
    }
}

#endif
