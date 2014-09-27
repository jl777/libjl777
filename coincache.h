//
//  coincache.h
//  xcode
//
//  Created by jimbo laptop on 7/30/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_coincache_h
#define xcode_coincache_h

char *load_cachestr(FILE *fp,int32_t len)
{
    char *str;
    str = malloc(len);
    if ( (int32_t)fread(str,1,len,fp) != len )
    {
        printf("init_rawtx_cache: error reading len.%d at %ld\n",len,ftell(fp));
        free(str);
        return(0);
    }
    return(str);
}

void update_coincache(FILE *fp,char *key,char *data,int32_t height)
{
    long fpos;
    int32_t addrlen,len;
    if ( fp == 0 )
    {
        printf("update_coincache: null fp rejected\n");
        return;
    }
    addrlen = (int32_t)(strlen(key) + 1);
    len = (int32_t)(strlen(data) + 1);
    fpos = ftell(fp);
    if ( fwrite(&addrlen,1,sizeof(addrlen),fp) != sizeof(addrlen) )
        fseek(fp,fpos,SEEK_SET);
    else if ( (int32_t)fwrite(key,1,addrlen,fp) != addrlen )
        fseek(fp,fpos,SEEK_SET);
    if ( fwrite(&len,1,sizeof(len),fp) != sizeof(len) )
        fseek(fp,fpos,SEEK_SET);
    else if ( (int32_t)fwrite(data,1,len,fp) != len )
        fseek(fp,fpos,SEEK_SET);
    else
    {
        fflush(fp);
        //printf("wrote %d bytes for caching %s\n",len,key);
    }
}

void ensure_coincache_blocks(struct coincache_info *cache,int32_t block)
{
    if ( (block+1) > cache->numblocks )
    {
        cache->blocks = realloc(cache->blocks,sizeof(*cache->blocks) * (block+1));
        memset(&cache->blocks[cache->numblocks],0,sizeof(*cache->blocks) * (block + 1 - cache->numblocks));
        cache->numblocks = (int32_t)(block + 1);
    }
}

void create_ignorelist(struct coincache_info *cache,char *name,int32_t lastblock,int32_t padding)
{
    FILE *fp;
    char fname[512];
    // update ignorelist
    if ( padding < 100 )
        padding = 100;
    sprintf(fname,"backups/ignorelist.%s",name);
    if ( (fp= fopen(fname,"wb")) != 0 )
    {
        lastblock -= padding;
        if ( lastblock < 0 )
            lastblock = 1;
        fwrite(&lastblock,1,sizeof(lastblock),fp);
        fwrite(cache->ignorelist,1,cache->ignoresize,fp);
        fclose(fp);
    }
}

void purge_coincache(struct coincache_info *cache,char *name,int32_t blockid)
{
    static long totalpurged;
    cJSON *json,*txobj,*txidobj;
    char *blocktxt;
    int32_t j,i,n,createdflag;
    char txid[512];
    struct coin_txid *tp;
    if ( cache == 0 || blockid < 0 )
        return;
    for (j=cache->purgedblock; j<=blockid; j++)
    {
        blocktxt = cache->blocks[j];
        if ( blocktxt != 0 )
        {
            json = cJSON_Parse(blocktxt);
            if ( json != 0 )
            {
                txobj = cJSON_GetObjectItem(json,"tx");
                n = cJSON_GetArraySize(txobj);
                for (i=0; i<n; i++)
                {
                    txidobj = cJSON_GetArrayItem(txobj,i);
                    copy_cJSON(txid,txidobj);
                    //printf("blocktxt.%ld i.%d of n.%d %s\n",(long)block,i,n,txid);
                    if ( txid[0] != 0 )
                    {
                        tp = MTadd_hashtable(&createdflag,&cache->coin_txids,txid);
                        if ( tp != 0 && tp->decodedrawtx != 0 )
                        {
                            //printf("%s\n",tp->decodedrawtx);
                            totalpurged += strlen(tp->decodedrawtx);
                            free(tp->decodedrawtx);
                            tp->decodedrawtx = 0;
                        }
                    }
                }
                free_json(json);
            }
            cache->blocks[j] = 0;
            totalpurged += strlen(blocktxt);
            free(blocktxt);
        }
        cache->purgedblock = j;
    }
    if ( ((rand()>>8) % 100) == 0 )
        printf("purging.%ld %s blockid.%d\n",totalpurged,name,blockid);
}

int32_t load_ignorelist(struct coincache_info *cache,char *name)
{
    FILE *fp;
    long endpos;
    int32_t lastblock = 0;
    char fname[512];
    sprintf(fname,"backups/ignorelist.%s",name);
    if ( (fp= fopen(fname,"rb")) != 0 )
    {
        fseek(fp,0,SEEK_END);
        endpos = ftell(fp) - sizeof(lastblock);
        cache->ignoresize = (int32_t)endpos;
        rewind(fp);
        if ( cache->ignorelist != 0 )
            free(cache->ignorelist);
        cache->ignorelist = malloc(endpos);
        fread(&lastblock,1,sizeof(lastblock),fp);
        fread(cache->ignorelist,1,cache->ignoresize,fp);
        fclose(fp);
        if ( 1 )
        {
            int i,numzeros = 0;
            for (i=0; i<cache->ignoresize; i++)
                if ( cache->ignorelist[i] == 0 )
                    numzeros++;
            printf("loaded ignorelist.%s lastblock.%d size.%d zero.%d\n",name,lastblock,cache->ignoresize,numzeros);
        }
    }
    return(lastblock);
}

int32_t init_rawtx_cache(FILE **fpp,struct coincache_info *cache,char *name,char *suffix)
{
    char buf[4096];
    long endpos,fpos;
    int32_t block,blockscache,createdflag,addrlen,len=0,count = 0;
    struct coin_txid *tp;
    printf("init_rawtx_cache %s\n",name);
    cache->lastignore = load_ignorelist(cache,name);
    blockscache = (strcmp(suffix,"blocks") == 0);
    if ( (*fpp) == 0 )
    {
        sprintf(buf,"backups/%s.%s",name,suffix);
        (*fpp) = fopen(buf,"rb+");
        if ( (*fpp) == 0 )
        {
            (*fpp) = fopen(buf,"wb");
            return(0);
        }
    }
    fseek((*fpp),0,SEEK_END);
    endpos = ftell((*fpp));
    rewind((*fpp));
    while ( (fpos= ftell((*fpp))) < endpos )
    {
        if ( fread(&addrlen,1,sizeof(addrlen),(*fpp)) == sizeof(addrlen) && addrlen < (int32_t)sizeof(buf) )
        {
            if ( (int32_t)fread(buf,1,addrlen,(*fpp)) == addrlen && fread(&len,1,sizeof(len),(*fpp)) == sizeof(len) )
            {
                if ( blockscache != 0 )
                {
                    if ( (block= atoi(buf)) >= 0 && block < (1<<30) )
                    {
                        ensure_coincache_blocks(cache,block);
                        if ( cache->blocks[block] != 0 )
                        {
                            free(cache->blocks[block]);
                            cache->blocks[block] = 0;
                        }
                        if ( cache != 0 && block < cache->lastignore && cache->ignorelist[block] != 0 )
                        {
                            fseek(*fpp,ftell(*fpp)+len,SEEK_SET);
                        }
                        else
                        {
                            if ( (cache->blocks[block]= load_cachestr((*fpp),len)) != 0 )
                                count++;
                        }
                    }
                }
                else
                {
                    tp = MTadd_hashtable(&createdflag,&cache->coin_txids,buf);
                    if ( tp->decodedrawtx != 0 )
                        free(tp->decodedrawtx);
                    if ( (tp->decodedrawtx= load_cachestr((*fpp),len)) != 0 )
                        count++;
                }
            }
            else
            {
                printf("init_rawtx_cache: error reading addrlen.%d or len.%d\n",addrlen,len);
                fseek(*fpp,fpos,SEEK_SET);
                break;
            }
        }
        else
        {
            printf("init_rawtx_cache: unexpected file read error at %ld, len %d\n",ftell((*fpp)),len);
            fseek(*fpp,fpos,SEEK_SET);
            break;
        }
    }
    printf("loaded %d entries for %s.%s fpos.%ld vs endpos.%ld\n",count,name,suffix,fpos,endpos);
    if ( fpos < endpos )
    {
        fflush(*fpp);
#ifndef WIN32
        if (ftruncate(fileno(*fpp), ftello(*fpp)) == -1)
            fprintf(stderr,"ftruncate(*fpp) failed: %s\n",name);
        else printf("%s truncated to %ld\n",name,fpos);
#endif
    }
    return(count);
}

#endif
