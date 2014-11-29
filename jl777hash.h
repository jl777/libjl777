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
struct hashtable
{
    char *name;
    void **hashtable;
    uint64_t hashsize,numsearches,numiterations,numitems;
    unsigned long keyoffset,keysize,modifiedoffset,structsize;
};

union _hashpacket_result { void *result; uint64_t hashval; };

struct hashpacket
{
    int32_t *createdflagp,doneflag,funcid;
    char *key;
    struct hashtable **hp_ptr;
    union _hashpacket_result U;
};

extern int32_t Historical_done;


void queue_enqueue(queue_t *queue,void *value)
{
    if ( queue->initflag == 0 )
    {
        portable_mutex_init(&queue->mutex);
        queue->initflag = 1;
    }
	portable_mutex_lock(&(queue->mutex));
	while ( queue->size == queue->capacity )
    {
        queue->capacity++;
        queue->buffer = realloc(queue->buffer,sizeof(*queue->buffer) * queue->capacity);
        queue->buffer[queue->size] = 0;
        if ( queue->in == 0 )
            queue->in = queue->size;
        //printf("increase Q size.%d capacity.%d\n",queue->size,queue->capacity);
		//pthread_cond_wait(&(queue->cond_full), &(queue->mutex));
    }
    //printf("enqueue %lx -> [%d] size.%d capacity.%d\n",(long)value,queue->in,queue->size,queue->capacity);
	queue->buffer[queue->in++] = value;
	++queue->size;
    queue->in %= queue->capacity;
    /*if ( queue->capacity >= 3 )
    printf("added to Q, size.%d in.%d | %p %p %p\n",queue->size,queue->in,queue->buffer[0],queue->buffer[1],queue->buffer[2]);
    else printf("added to Q, size.%d in %d capacity %d\n",queue->size,queue->in,queue->capacity);
	*/
    portable_mutex_unlock(&(queue->mutex));
	//pthread_cond_broadcast(&(queue->cond_empty));
}

void *queue_dequeue(queue_t *queue)
{
    void *value = 0;
    if ( queue->initflag == 0 )
    {
        portable_mutex_init(&queue->mutex);
        queue->initflag = 1;
    }
    portable_mutex_lock(&(queue->mutex));
	//while ( queue->size == 0 )
	//	pthread_cond_wait(&(queue->cond_empty), &(queue->mutex));
    if ( queue->size > 0 )
    {
        value = queue->buffer[queue->out];
        queue->buffer[queue->out] = 0;
        //printf("dequeue %lx from %d, size.%d capacity.%d\n",(long)value,queue->out,queue->size,queue->capacity);
        --queue->size;
        ++queue->out;
        queue->out %= queue->capacity;
        //printf("new size.%d out.%d\n",queue->size,queue->out);
    }
	portable_mutex_unlock(&(queue->mutex));
	//pthread_cond_broadcast(&(queue->cond_full));
	return value;
}

int32_t queue_size(queue_t *queue)
{
    if ( queue->buffer == 0 )
        portable_mutex_init(&queue->mutex);
	portable_mutex_lock(&(queue->mutex));
	int32_t size = queue->size;
	portable_mutex_unlock(&(queue->mutex));
	return size;
}

int32_t init_pingpong_queue(struct pingpong_queue *ppq,char *name,int32_t (*action)(),queue_t *destq,queue_t *errorq)
{
    ppq->name = name;
    ppq->destqueue = destq;
    ppq->errorqueue = errorq;
    ppq->action = action;
    return(queue_size(&ppq->pingpong[0]) + queue_size(&ppq->pingpong[1]));  // init mutex side effect
}

// seems a bit wastefull to do all the two iter queueing/dequeuing with threadlock overhead
// however, there is assumed to be plenty of CPU time relative to actual blockchain events
// also this method allows for adding of parallel threads without worry
int32_t process_pingpong_queue(struct pingpong_queue *ppq,void *argptr)
{
    int32_t iter,retval;
    void *ptr;
    //printf("%p process_pingpong_queue.%s %d %d\n",ppq,ppq->name,queue_size(&ppq->pingpong[0]),queue_size(&ppq->pingpong[1]));
    for (iter=0; iter<2; iter++)
    {
        while ( (ptr= queue_dequeue(&ppq->pingpong[iter])) != 0 )
        {
            //printf("%s pingpong[%d].%p action.%p\n",ppq->name,iter,ptr,ppq->action);
            retval = (*ppq->action)(&ptr,argptr);
            if ( retval == 0 )
                queue_enqueue(&ppq->pingpong[iter ^ 1],ptr);
            else if ( retval < 0 )
            {
                printf("%s iter.%d errorqueue %p vs %p\n",ppq->name,iter,ppq->errorqueue,&ppq->pingpong[0]);
                if ( ppq->errorqueue == &ppq->pingpong[0] )
                    queue_enqueue(&ppq->pingpong[iter ^ 1],ptr);
                else if ( ppq->errorqueue != 0 )
                    queue_enqueue(ppq->errorqueue,ptr);
                else free(ptr);
            }
            else if ( ppq->destqueue != 0 )
            {
                printf("%s iter.%d destqueue %p vs %p\n",ppq->name,iter,ppq->destqueue,&ppq->pingpong[0]);
                if ( ppq->destqueue == &ppq->pingpong[0] )
                    queue_enqueue(&ppq->pingpong[iter ^ 1],ptr);
                else if ( ppq->destqueue != 0 )
                    queue_enqueue(ppq->destqueue,ptr);
                else free(ptr);
            }
            else free(ptr);
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
    //while ( Global_mp->hashprocessing != 0 )
    //    usleep(100);
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

uint64_t search_hashtable(struct hashtable *hp,char *key)
{
    void *ptr,**hashtable = hp->hashtable;
    uint64_t i,hashval,ind;
    if ( hp == 0 || key == 0 || key[0] == 0 )
        return(HASHSEARCH_ERROR);
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
        if ( strcmp((void *)((long)ptr + hp->keyoffset),key) == 0 )
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
            newind = search_hashtable(newhp,(char *)((long)ptr + newhp->keyoffset));
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
    struct hashtable *hp = *hp_ptr;
    *createdflagp = 0;
    if ( key == 0 || *key == 0 || hp == 0 || strlen(key) >= hp->keysize )
    {
        printf("%p key.(%s) len.%ld is too big for %s %ld, FATAL\n",key,key,strlen(key),hp->name,hp->keysize);
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
        ptr = calloc(1,hp->structsize);
        hp->hashtable[ind] = ptr;
        strcpy((void *)((long)ptr + hp->keyoffset),key);
        //*(int64_t *)((long)ptr + hp->modifiedoffset) = 1;
        hp->numitems++;
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
    if ( key == 0 || key[0] == 0 || hp_ptr == 0 || *hp_ptr == 0  || strlen(key) >= (*hp_ptr)->keysize )
    {
        printf("MTadd_hashtable. illegal key?? (%s)\n",key);
#ifdef __APPLE__
        //while ( 1 )
       //     sleep(1);
#endif
    }
    ptr = calloc(1,sizeof(*ptr));
    ptr->createdflagp = createdflagp;
    ptr->hp_ptr = hp_ptr;
    ptr->key = key;
    ptr->funcid = 'A';
    //while ( Global_mp->hashprocessing != 0 )
    //    usleep(1);
    Global_mp->hashprocessing++;
    queue_enqueue(&Global_mp->hashtable_queue[1],ptr);
    usleep(1);
    while ( ptr->doneflag == 0 )
        usleep(1);
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
    //while ( Global_mp->hashprocessing != 0 )
    //    usleep(1);
    Global_mp->hashprocessing++;
    queue_enqueue(&Global_mp->hashtable_queue[0],ptr);
    usleep(1);
    while ( ptr->doneflag == 0 )
        usleep(1);
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
            usleep(1000);
        for (iter=n=0; iter<2; iter++)
        {
            while ( (ptr= queue_dequeue(&Global_mp->hashtable_queue[iter])) != 0 )
            {
                //printf("numitems.%ld process.%p hp %p\n",(long)(*ptr->hp_ptr)->hashsize,ptr,ptr->hp_ptr);
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
#endif
