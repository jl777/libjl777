//
//  jl777hash.h
//  Created by jl777
//  MIT License
//
#ifndef gateway_jl777hash_h
#define gateway_jl777hash_h

#define HASHTABLES_STARTSIZE 100
#define HASHTABLE_RESIZEFACTOR 3
#define HASHSEARCH_ERROR ((uint64_t)-1)
#define HASHTABLE_FULL .666


union _hashpacket_result { void *result; uint64_t hashval; };

struct hashpacket
{
    struct queueitem DL;
    int32_t *createdflagp,doneflag,funcid;
    char *key;
    struct hashtable **hp_ptr;
    union _hashpacket_result U;
};

extern int32_t Historical_done;
#ifdef portig
struct queueitem *queueitem(char *str)
{
    struct queueitem *item = calloc(1,sizeof(struct queueitem) + strlen(str) + 1);
    strcpy((char *)((long)item + sizeof(struct queueitem)),str);
    return(item);
}

void free_queueitem(void *itemptr)
{
    free((void *)((long)itemptr - sizeof(struct queueitem)));
}

void lock_queue(queue_t *queue)
{
    if ( queue->initflag == 0 )
    {
        portable_mutex_init(&queue->mutex);
        queue->initflag = 1;
    }
	portable_mutex_lock(&queue->mutex);
}

void queue_enqueue(char *name,queue_t *queue,struct queueitem *item)
{
    if ( Debuglevel > 2 )
        fprintf(stderr,"name.(%s) append.%p list.%p (next.%p prev.%p)\n",name,item,queue->list,item->next,item->prev);
    if ( queue->list == 0 && name != 0 && name[0] != 0 )
        safecopy(queue->name,name,sizeof(queue->name));
    if ( item == 0 )
    {
        printf("FATAL type error: queueing empty value\n"), getchar();
        return;
    }
    lock_queue(queue);
    DL_APPEND(queue->list,item);
    portable_mutex_unlock(&queue->mutex);
}

void *queue_dequeue(queue_t *queue,int32_t offsetflag)
{
    struct queueitem *item = 0;
    lock_queue(queue);
    if ( Debuglevel > 2 )
        fprintf(stderr,"queue_dequeue name.(%s) dequeue.%p\n",queue->name,queue->list);
    if ( queue->list != 0 )
    {
        item = queue->list;
        DL_DELETE(queue->list,item);
        if ( Debuglevel > 2 )
            fprintf(stderr,"name.(%s) dequeue.%p list.%p\n",queue->name,item,queue->list);
    }
	portable_mutex_unlock(&queue->mutex);
    if ( item != 0 && offsetflag != 0 )
        return((void *)((long)item + sizeof(struct queueitem)));
    else return(item);
}

int32_t queue_size(queue_t *queue)
{
    int32_t count = 0;
    struct queueitem *tmp;
    lock_queue(queue);
    DL_COUNT(queue->list,tmp,count);
    portable_mutex_unlock(&queue->mutex);
	return count;
}
#endif

int32_t init_pingpong_queue(struct pingpong_queue *ppq,char *name,int32_t (*action)(),queue_t *destq,queue_t *errorq)
{
    ppq->name = name;
    ppq->destqueue = destq;
    ppq->errorqueue = errorq;
    ppq->action = action;
    ppq->offset = 1;
    return(queue_size(&ppq->pingpong[0]) + queue_size(&ppq->pingpong[1]));  // init mutex side effect
}

// seems a bit wastefull to do all the two iter queueing/dequeuing with threadlock overhead
// however, there is assumed to be plenty of CPU time relative to actual blockchain events
// also this method allows for adding of parallel threads without worry
int32_t process_pingpong_queue(struct pingpong_queue *ppq,void *argptr)
{
    int32_t iter,retval,freeflag = 0;
    void *ptr;
    //printf("%p process_pingpong_queue.%s %d %d\n",ppq,ppq->name,queue_size(&ppq->pingpong[0]),queue_size(&ppq->pingpong[1]));
    for (iter=0; iter<2; iter++)
    {
        while ( (ptr= queue_dequeue(&ppq->pingpong[iter],ppq->offset)) != 0 )
        {
            if ( Debuglevel > 2 )
                printf("%s pingpong[%d].%p action.%p\n",ppq->name,iter,ptr,ppq->action);
            retval = (*ppq->action)(&ptr,argptr);
            if ( retval == 0 )
                queue_enqueue(ppq->name,&ppq->pingpong[iter ^ 1],ptr);
            else if ( ptr != 0 )
            {
                if ( retval < 0 )
                {
                    if ( Debuglevel > 0 )
                        printf("%s iter.%d errorqueue %p vs %p\n",ppq->name,iter,ppq->errorqueue,&ppq->pingpong[0]);
                    if ( ppq->errorqueue == &ppq->pingpong[0] )
                        queue_enqueue(ppq->name,&ppq->pingpong[iter ^ 1],ptr);
                    else if ( ppq->errorqueue != 0 )
                        queue_enqueue(ppq->name,ppq->errorqueue,ptr);
                    else freeflag = 1;
                }
                else if ( ppq->destqueue != 0 )
                {
                    if ( Debuglevel > 0 )
                        printf("%s iter.%d destqueue %p vs %p\n",ppq->name,iter,ppq->destqueue,&ppq->pingpong[0]);
                    if ( ppq->destqueue == &ppq->pingpong[0] )
                        queue_enqueue(ppq->name,&ppq->pingpong[iter ^ 1],ptr);
                    else if ( ppq->destqueue != 0 )
                        queue_enqueue(ppq->name,ppq->destqueue,ptr);
                    else freeflag = 1;
                }
                if ( freeflag != 0 )
                {
                    if ( ppq->offset == 0 )
                        free(ptr);
                    else free_queueitem(ptr);
                }
            }
        }
    }
    return(queue_size(&ppq->pingpong[0]) + queue_size(&ppq->pingpong[0]));
}

uint64_t calc_decimalhash(char *key)
{
    char c;
    uint64_t i,a,b,hashval = 0;
    a = 63689; b = 378551;
    for (i=0; key[i]!=0; i++,a*=b)
    {
        c = key[i];
        if ( c >= '0' && c <= '9' )
            hashval = hashval*a + (c - '0');
        else
        {
            hashval = hashval*a + (c&0xf);
            c >>= 4;
            hashval = hashval*a + (c&0xf);
        }
    }
    return(hashval);
}

struct hashtable *hashtable_create(char *name,int64_t hashsize,long structsize,long keyoffset,long keysize,long modifiedoffset)
{
    struct hashtable *hp;
    hp = calloc(1,sizeof(*hp));
    hp->name = clonestr(name);
    hp->hashsize = hashsize;
    hp->structsize = structsize;
    hp->keyoffset = keyoffset;
    hp->keysize = keysize;
    if ( keysize == 0 && keyoffset != structsize )
    {
        printf("illegal hashtable_create for dynamic sized keys keyoffset.%ld != structsize.%ld\n",keyoffset,structsize);
        exit(-1);
    }
    hp->modifiedoffset = modifiedoffset;
    hp->hashtable = calloc((long)hp->hashsize,sizeof(*hp->hashtable));
    return(hp);
}

int64_t _hashtable_clear_modified(struct hashtable *hp,long offset) // no MT support (yet)
{
    void *ptr;
    uint64_t i,nonz = 0;
    for (i=0; i<hp->hashsize; i++)
    {
        ptr = hp->hashtable[i];
        if ( ptr != 0 && *(int64_t *)((long)ptr + offset) != 0 )
            *(int64_t *)((long)ptr + offset) = 0, nonz++;
    }
    return(nonz);
}
#define hashtable_clear_modified(hp)_hashtable_clear_modified(hp,(hp)->modifiedoffset)

void **hashtable_gather_modified(int64_t *changedp,struct hashtable *hp,int32_t forceflag) // partial MT support
{
    uint64_t i,m,n = 0;
    void *ptr,**list = 0;
    if ( hp == 0 )
        return(0);
    if ( hp->modifiedoffset < 0 )
        forceflag = 1;
    Global_mp->hashprocessing++;
    for (i=0; i<hp->hashsize; i++)
    {
        ptr = hp->hashtable[i];
        if ( ptr != 0 && (forceflag != 0 || *(int64_t *)((long)ptr + hp->modifiedoffset) != 0) )
            n++;
    }
    if ( n != 0 )
    {
        list = calloc((long)n+1,sizeof(*list));
        for (i=m=0; i<hp->hashsize; i++)
        {
            ptr = hp->hashtable[i];
            if ( ptr != 0 && (forceflag != 0 || *(int64_t *)((long)ptr + hp->modifiedoffset) != 0) )
                list[m++] = ptr;
        }
        if ( m != n )
            printf("gather_modified: unexpected m.%ld != n.%ld\n",(long)m,(long)n);
    }
    Global_mp->hashprocessing--;
    *changedp = n;
    return(list);
}

uint64_t search_hashtable(struct hashtable *hp,void *key)
{
    void *ptr,**hashtable = hp->hashtable;
    uint64_t i,hashval,ind;
    int32_t keysize;
    if ( hp == 0 || key == 0 || ((char *)key)[0] == 0 || hp->hashsize == 0 )
        return(HASHSEARCH_ERROR);
    keysize = (int32_t)hp->keysize;
    hashval = calc_decimalhash(key);
    //printf("hashval = %ld\n",(unsigned long)hashval);
    ind = (hashval % hp->hashsize);
    hp->numsearches++;
    if ( (hp->numsearches % 100000) == 0 )
        printf("search_hashtable  ave %.3f numsearches.%ld numiterations.%ld\n",(double)hp->numiterations/hp->numsearches,(long)hp->numsearches,(long)hp->numiterations);
    for (i=0; i<hp->hashsize; i++,ind++)
    {
        hp->numiterations++;
        if ( ind >= hp->hashsize )
            ind = 0;
        ptr = hashtable[ind];
        if ( ptr == 0 )
            return(ind);
        if ( 1 || keysize == 0 ) // dead space maybe causes problems for now?
        {
            if ( strcmp((void *)((long)ptr + hp->keyoffset),key) == 0 )
                return(ind);
        }
        else if ( memcmp((void *)((long)ptr + hp->keyoffset),key,keysize) == 0 )
            return(ind);
    }
    return(HASHSEARCH_ERROR);
}

struct hashtable *resize_hashtable(struct hashtable *hp,int64_t newsize)
{
    void *ptr;
    uint64_t ind,newind;
    struct hashtable *newhp = calloc(1,sizeof(*newhp));
    *newhp = *hp;
    //printf("about to resize %s %ld, hp.%p\n",hp->name,(long)hp->numitems,hp);
    newhp->hashsize = newsize;
    newhp->numitems = 0;
    newhp->hashtable = calloc((long)newhp->hashsize,sizeof(*newhp->hashtable));
    for (ind=0; ind<hp->hashsize; ind++)
    {
        ptr = hp->hashtable[ind];
        if ( ptr != 0 )
        {
            newind = search_hashtable(newhp,(char *)((long)ptr + hp->keyoffset));
            if ( newind != HASHSEARCH_ERROR && newhp->hashtable[newind] == 0 )
            {
                newhp->hashtable[newind] = ptr;
                newhp->numitems++;
                //printf("%ld old.%ld -> new.%ld\n",(long)newhp->numitems,(long)ind,(long)newind);
            } else printf("duplicate entry?? at ind.%ld newind.%ld\n",(long)ind,(long)newind);
        }
    }
    if ( hp->numitems != newhp->numitems )
        printf("RESIZE ERROR??? %ld != %ld\n",(long)hp->numitems,(long)newhp->numitems);
    else
    {
        for (ind=0; ind<hp->hashsize; ind++)
        {
            ptr = hp->hashtable[ind];
            if ( ptr != 0 )
            {
                newind = search_hashtable(newhp,(char *)((long)ptr + newhp->keyoffset));
                if ( newind == HASHSEARCH_ERROR || newhp->hashtable[newind] == 0 )
                    printf("ERROR: cant find %s after resizing\n",(char *)((long)ptr + hp->keyoffset));
            }
        }
    }
    void *tmp = hp->hashtable;
    hp->hashtable = newhp->hashtable;
    free(tmp);
    free(newhp);
    hp->hashsize = newsize;
    return(hp);

    //printf("free hp.%p\n",hp);
    free(hp->hashtable);
    free(hp);
    //printf("finished %s resized to %ld\n",newhp->name,(long)newhp->hashsize);
    return(newhp);
}

void *add_hashtable(int32_t *createdflagp,struct hashtable **hp_ptr,char *key)
{
    void *ptr;
    uint64_t ind;
    int32_t allocsize;
    struct hashtable *hp = *hp_ptr;
    if ( createdflagp != 0 )
        *createdflagp = 0;
    if ( key == 0 || *key == 0 || hp == 0 || (hp->keysize > 0 && strlen(key) >= hp->keysize) )
    {
        printf("%p key.(%s) len.%ld is too big for %s %ld, FATAL\n",key,key,strlen(key),hp!=0?hp->name:"",hp!=0?hp->keysize:0);
        key = "123456";
    }
    //printf("hp %p %p hashsize.%ld add_hashtable(%s)\n",hp_ptr,hp,(long)hp->hashsize,key);
    if ( hp->hashtable == 0 )
        hp->hashtable = calloc(sizeof(*hp->hashtable),(long)hp->hashsize);
    else if ( hp->numitems > hp->hashsize*HASHTABLE_FULL )
    {
        *hp_ptr = resize_hashtable(hp,hp->hashsize * HASHTABLE_RESIZEFACTOR);
        hp = *hp_ptr;
    }
    ind = search_hashtable(hp,key);
    if ( ind == HASHSEARCH_ERROR ) // table full
        return(0);
    ptr = hp->hashtable[ind];
    //printf("ptr %p, ind.%ld\n",ptr,(long)ind);
    if ( ptr == 0 )
    {
        if ( hp->keysize == 0 )
            allocsize = (int32_t)(hp->structsize + strlen(key) + 1);
        else allocsize = (int32_t)hp->structsize;
        ptr = calloc(1,allocsize);
        hp->hashtable[ind] = ptr;
        strcpy((void *)((long)ptr + hp->keyoffset),key);
        //if ( hp->modifiedoffset >= 0 ) *(int64_t *)((long)ptr + hp->modifiedoffset) = 1;
        hp->numitems++;
        if ( createdflagp != 0 )
            *createdflagp = 1;
        ind = search_hashtable(hp,key);
        if ( ind == (uint64_t)-1 || hp->hashtable[ind] == 0 )
            printf("FATAL ERROR adding (%s) to hashtable.%s\n",key,hp->name);
    }
    return(ptr);
}

void *MTadd_hashtable(int32_t *createdflagp,struct hashtable **hp_ptr,char *key)
{
    void *result;
    extern struct NXThandler_info *Global_mp;
    struct hashpacket *ptr;
    if ( key == 0 || key[0] == 0 || hp_ptr == 0 || *hp_ptr == 0  || ((*hp_ptr)->keysize > 0 && strlen(key) >= (*hp_ptr)->keysize) )
    {
        printf("MTadd_hashtable.(%s) illegal key?? (%s)\n",*hp_ptr!=0?(*hp_ptr)->name:"",key);
#ifdef __APPLE__
        while ( 1 )
            portable_sleep(1);
#endif
    }
    ptr = calloc(1,sizeof(*ptr));
    ptr->createdflagp = createdflagp;
    ptr->hp_ptr = hp_ptr;
    ptr->key = key;
    ptr->funcid = 'A';
    Global_mp->hashprocessing++;
    queue_enqueue("hashtableQ1",&Global_mp->hashtable_queue[1],&ptr->DL);
    msleep(10*APISLEEP);
    while ( ptr->doneflag == 0 )
        msleep(10*APISLEEP);
    result = ptr->U.result;
    free(ptr);
    Global_mp->hashprocessing--;
    return(result);
}

uint64_t MTsearch_hashtable(struct hashtable **hp_ptr,char *key)
{
    uint64_t hashval;
    struct hashpacket *ptr;
    ptr = calloc(1,sizeof(*ptr));
    ptr->hp_ptr = hp_ptr;
    ptr->key = key;
    ptr->funcid = 'S';
    Global_mp->hashprocessing++;
    queue_enqueue("hashtableQ0",&Global_mp->hashtable_queue[0],&ptr->DL);
    msleep(10*APISLEEP);
    while ( ptr->doneflag == 0 )
        msleep(10*APISLEEP);
    hashval = ptr->U.hashval;
    free(ptr);
    Global_mp->hashprocessing--;
    return(hashval);
}

void *process_hashtablequeues(void *_p) // serialize hashtable functions
{
    int32_t iter,n = 0;
    struct hashpacket *ptr;
    while ( 1 )//Historical_done == 0 )
    {
        if ( Global_mp->hashprocessing == 0 && n == 0 )
            msleep(APISLEEP);
        for (iter=n=0; iter<2; iter++)
        {
            while ( (ptr= queue_dequeue(&Global_mp->hashtable_queue[iter],0)) != 0 )
            {
               // printf("numitems.%ld process.%p hp %p\n",(long)(*ptr->hp_ptr)->hashsize,ptr,ptr->hp_ptr);
                //printf(">>>>> Processs %p\n",ptr);
                if ( ptr->funcid == 'A' )
                    ptr->U.result = add_hashtable(ptr->createdflagp,ptr->hp_ptr,ptr->key);
                else if ( ptr->funcid == 'S' )
                    ptr->U.hashval = search_hashtable(*ptr->hp_ptr,ptr->key);
                else printf("UNEXPECTED MThashtable funcid.(%c) %d\n",ptr->funcid,ptr->funcid);
                ptr->doneflag = 1;
                n++;
                //printf("<<<<<< Finished Processs %p\n",ptr);
                //printf("finished numitems.%ld process.%p hp %p\n",(long)(*ptr->hp_ptr)->hashsize,ptr,ptr->hp_ptr);
            }
        }
    }
    fprintf(stderr,"finished processing hashtable MT queues\n");
    exit(0);
    return(0);
}


int32_t clear_hashtable_field(struct hashtable *hp,long offset,long fieldsize)
{
    long i,j;
    uint8_t *ptr;
    int32_t flag,nonz = 0;
    if ( hp == 0 )
        return(0);
    for (i=0; i<hp->hashsize; i++)
    {
        if ( (ptr= hp->hashtable[i]) != 0 )
        {
            for (j=flag=0; j<fieldsize; j++)
            {
                if ( ptr[offset + j] != 0 )
                    ptr[offset + j] = 0, flag++;
            }
            nonz += (flag != 0);
        }
    }
    return(nonz);
}

int32_t gather_hashtable_field(void **items,int32_t numitems,int32_t maxitems,struct hashtable *hp,long offset,long fieldsize)
{
    long i,j;
    uint8_t *ptr;
    //printf("gather num.%d max.%d hp.%p offset.%ld size.%ld\n",numitems,maxitems,hp,offset,fieldsize);
    if ( items == 0 || hp == 0 )
        return(numitems);
    for (i=0; i<hp->hashsize; i++)
    {
        if ( numitems >= maxitems )
            return(maxitems);
        if ( (ptr= hp->hashtable[i]) != 0 )
        {
            for (j=0; j<fieldsize; j++)
            {
                if ( ptr[offset + j] != 0 )
                {
                    items[numitems++] = ptr;
                    break;
                }
            }
        }
    }
    return(numitems);
}

#endif
