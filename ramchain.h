//
//  ramchain
//  SuperNET
//
//  by jl777 on 12/29/14.
//  MIT license
//  huffman coding based on code from: http://rosettacode.org/wiki/Huffman_coding

/*
 Create a leaf node for each symbol and add it to the priority queue.
 While there is more than one node in the queue:
 Remove the node of highest priority (lowest probability) twice to get two nodes.
 Create a new internal node with these two nodes as children and with probability equal to the sum of the two nodes' probabilities.
 Add the new node to the queue.
 The remaining node is the root node and the tree is complete.
 */

#ifndef ramchain_h
#define ramchain_h

#define SETBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] |= (1 << ((bitoffset) & 7)))
#define GETBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] & (1 << ((bitoffset) & 7)))
#define CLEARBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] &= ~(1 << ((bitoffset) & 7)))

#define MAX_HUFFBITS 6
struct huffentry { uint64_t numbits:MAX_HUFFBITS,bits:(64-MAX_HUFFBITS); };

struct huffheap
{
    uint32_t *f;
    uint32_t *h,n,s,cs,pad;
};
typedef struct huffheap heap_t;

/*struct huffnode
{
	struct huffnode *left,*right;
	uint32_t freq,ind;
};*/


/*struct ramchain_info
{
    int32_t blocknum_bits,txind_bits,vout_bits;
};
struct ramchain_entry { uint64_t value; uint32_t outblock:20,txind:15,vout:14,spentblock:20,spentind:15,vin:14; };
*/

void hclose(HUFF *hp)
{
    if ( hp != 0 )
        free(hp);
}

HUFF *hopen(uint8_t *bits,int32_t num)
{
    HUFF *hp = calloc(1,sizeof(*hp));
    hp->ptr = hp->buf = bits;
    if ( (num & 7) != 0 )
        num++;
    hp->allocsize = num;
    return(hp);
}

void _hseek(HUFF *hp)
{
    hp->ptr = &hp->buf[hp->bitoffset >> 3];
    hp->maski = (hp->bitoffset & 7);
}

void hrewind(HUFF *hp)
{
    hp->bitoffset = 0;
    _hseek(hp);
}

void hclear(HUFF *hp)
{
    hp->bitoffset = 0;
    _hseek(hp);
    memset(hp->buf,0,hp->allocsize);
    hp->endpos = 0;
}

int32_t hseek(HUFF *hp,int32_t offset,int32_t mode)
{
    if ( mode == SEEK_END )
        offset += hp->endpos;
    if ( offset >= 0 && (offset>>3) < hp->allocsize )
        hp->bitoffset = offset, _hseek(hp);
    else
    {
        printf("hseek.%d: illegal offset.%d %d >= allocsize.%d\n",mode,offset,offset>>3,hp->allocsize);
        return(-1);
    }
    return(0);
}

int32_t hgetbit(HUFF *hp)
{
    int32_t bit = 0;
    if ( hp->bitoffset < hp->endpos )
    {
        if ( (*hp->ptr & huffmasks[hp->maski++]) != 0 )
            bit = 1;
        hp->bitoffset++;
        if ( hp->maski == 8 )
        {
            hp->maski = 0;
            hp->ptr++;
        }
        return(bit);
    }
    return(-1);
}

int32_t hputbit(HUFF *hp,int32_t bit)
{
    if ( bit != 0 )
        *hp->ptr |= huffmasks[hp->maski];
    else *hp->ptr &= huffoppomasks[hp->maski];
    if ( ++hp->maski >= 8 )
    {
        hp->maski = 0;
        hp->ptr++;
    }
    if ( ++hp->bitoffset > hp->endpos )
        hp->endpos = hp->bitoffset;
    if ( (hp->bitoffset>>3) >= hp->allocsize )
    {
        printf("hwrite: bitoffset.%d >= allocsize.%d\n",hp->bitoffset,hp->allocsize);
        _hseek(hp);
        return(-1);
    }
    return(0);
}

int32_t hwrite(uint64_t codebits,int32_t numbits,HUFF *hp)
{
    int32_t i;
    for (i=0; i<numbits; i++,codebits>>=1)
    {
        if ( hputbit(hp,codebits & 1) < 0 )
            return(-1);
    }
    return(numbits);
}

int32_t hflush(FILE *fp,HUFF *hp)
{
    long emit_varint(FILE *fp,uint64_t x);
    uint32_t len;
    if ( emit_varint(fp,hp->endpos) < 0 )
        return(-1);
    len = hp->endpos >> 3;
    if ( (hp->endpos & 7) != 0 )
        len++;
    if ( fwrite(hp->buf,1,len,fp) != len )
        return(-1);
    return(0);
}

void huff_free(struct huffcode *huff)
{
    if ( huff->tree != 0 )
        free(huff->tree);
    free(huff);
}

void huff_iteminit(struct huffitem *hip,void *ptr,long size,long wt,int32_t ishexstr)
{
    if ( size > sizeof(*hip) )
    {
        printf("FATAL: huff_iteminit overflow size.%ld vs %ld\n",size,sizeof(*hip));
        exit(-1);
        return;
    }
    memcpy(hip->U.bits.bytes,ptr,size);
    hip->size = (uint8_t)size;
    if ( wt > 0xff )
        wt = 0xff;
    hip->wt = (uint8_t)((wt != 0) ? wt : 8);
    hip->ishex = ishexstr;
    hip->codebits = 0;
    hip->numbits = 0;
}

const void *huff_getitem(struct huffcode *huff,int32_t *sizep,uint32_t ind)
{
    static unsigned char defaultbytes[256];
    const struct huffitem *hip;
    int32_t i;
    if ( huff != 0 && (hip= huff->items[ind]) != 0 )
    {
        *sizep = hip->size;
        return(hip->U.bits.bytes);
    }
    if ( defaultbytes[0xff] != 0xff )
        for (i=0; i<256; i++)
            defaultbytes[i] = i;
    *sizep = 1;
    return(&defaultbytes[ind & 0xff]);
}

int32_t huff_output(struct huffcode *huff,uint8_t *output,int32_t num,int32_t maxlen,int32_t ind)
{
    int32_t size;
    const void *ptr;
    ptr = huff_getitem(huff,&size,ind);
    if ( num+size <= maxlen )
    {
        //printf("%d.(%d %c) ",size,ind,*(char *)ptr);
        memcpy(output + num,ptr,size);
        return(num+size);
    }
    printf("huffoutput error: num.%d size.%d > maxlen.%d\n",num,size,maxlen);
    return(-1);
}

uint64_t _reversebits(uint64_t x,int32_t n)
{
    uint64_t rev = 0;
    int32_t i = 0;
    while ( n > 0 )
    {
        if ( GETBIT((void *)&x,n-1) != 0 )
            SETBIT(&rev,i);
        i++;
        n--;
    }
    return(rev);
}

void huff_clearstats(struct huffcode *huff)
{
    huff->totalbits = huff->totalbytes = 0;
}

uint64_t huff_convstr(char *str)
{
    uint64_t mask,codebits = 0;
    long n = strlen(str);
    mask = (1 << (n-1));
    while ( n > 0 )
    {
        if ( str[n-1] != '0' )
            codebits |= mask;
        //printf("(%c %llx m%x) ",str[n-1],(long long)codebits,(int)mask);
        mask >>= 1;
        n--;
    }
    //printf("(%s -> %llx)\n",str,(long long)codebits);
    return(codebits);
}

char *huff_str(uint64_t codebits,int32_t n)
{
    static char str[128];
    uint64_t mask = 1;
    int32_t i;
    for (i=0; i<n; i++,mask<<=1)
        str[i] = ((codebits & mask) != 0) + '0';
    str[i] = 0;
    return(str);
}

void huff_disp(struct huffcode *huff)
{
    char *strbit;
    int32_t i,n;
    uint64_t codebits;
    strbit = calloc(1,huff->maxbits);
    for (i=0; i<huff->numinds; i++)
    {
        n = huff->items[i]->numbits;
        codebits = _reversebits(huff->items[i]->codebits,n);
        if ( n != 0 )
        {
            //inttobits(codebits,n,strbit);
            printf("%05d (%c): (%8s).%d %s\n",i,i,huff_str(codebits,n),n,strbit);
        }
    }
    free(strbit);
}

heap_t *_heap_create(uint32_t s,uint32_t *f)
{
    heap_t *h;
    h = malloc(sizeof(heap_t));
    h->h = malloc(sizeof(*h->h) * s);
   // printf("_heap_create heap.%p h.%p s.%d\n",h,h->h,s);
    h->s = h->cs = s;
    h->n = 0;
    h->f = f;
    return(h);
}

void _heap_destroy(heap_t *heap)
{
    free(heap->h);
    free(heap);
}

#define swap_(I,J) do { int t_; t_ = a[(I)];	\
a[(I)] = a[(J)]; a[(J)] = t_; } while(0)
void _heap_sort(heap_t *heap)
{
    uint32_t i=1,j=2; // gnome sort
    uint32_t *a = heap->h;
    while ( i < heap->n ) // smaller values are kept at the end
    {
        if ( heap->f[a[i-1]] >= heap->f[a[i]] )
            i = j, j++;
        else
        {
            swap_(i-1, i);
            i--;
            i = (i == 0) ? j++ : i;
        }
    }
}
#undef swap_

void _heap_add(heap_t *heap,uint32_t ind)
{
    //printf("add to heap ind.%d n.%d s.%d\n",ind,heap->n,heap->s);
    if ( (heap->n + 1) > heap->s )
    {
        heap->h = realloc(heap->h,heap->s + heap->cs);
        heap->s += heap->cs;
    }
    heap->h[heap->n++] = ind;
    _heap_sort(heap);
}

int32_t _heap_remove(heap_t *heap)
{
    if ( heap->n > 0 )
        return(heap->h[--heap->n]);
    return(-1);
}

/*
struct huffnode *huff_leafnode(struct huffcode *huff,uint32_t ind)
{
	struct huffnode *leaf = 0;
    uint32_t freq;
	if ( (freq= huff->items[ind].freq) != 0 )
    {
        leaf = huff->pool + huff->n_nodes++;
        leaf->ind = ind, leaf->freq = freq;
        if ( ind > huff->maxind )
            huff->maxind = ind;
    }
	return(leaf);
}

void huff_insert(struct huffcode *huff,struct huffnode *node)
{
	int j, i;
    if ( node != 0 )
    {
        i = huff->qend++;
        while ( (j= (i >> 1)) )
        {
            if ( huff->q[j]->freq <= node->freq )
                break;
            huff->q[i] = huff->q[j], i = j;
        }
        huff->q[i] = node;
    }
}

struct huffnode *huff_newnode(struct huffcode *huff,uint32_t freq,uint32_t ind,struct huffnode *a,struct huffnode *b)
{
	struct huffnode *n = 0;
	if ( freq == 0 )
    {
        if ( a == 0 || b == 0 )
        {
            printf("huff_newnode null a.%p or b.%p\n",a,b);
            while ( 1 ) sleep(1);
        }
        n = huff->pool + huff->n_nodes++;
		n->left = a, n->right = b;
		n->freq = a->freq + b->freq;
	}
    else
    {
        printf("huff_newnode called with freq.%d ind.%d\n",freq,ind);
        while ( 1 )
            sleep(1);
    }
	return(n);
}

struct huffnode *huff_remove(struct huffcode *huff)
{
	int l,i = 1;
	struct huffnode *n = huff->q[i];
   // printf("remove: set n <- q[i %d].%d\n",i,huff->q[i]->ind);
	if ( huff->qend < 2 )
    {
        printf("huff->qend.%d null return\n",huff->qend); while ( 1 ) sleep(1);
        return(0);
    }
	huff->qend--;
	while ( (l= (i << 1)) < huff->qend )
    {
		if ( (l + 1) < huff->qend && huff->q[l + 1]->freq < huff->q[l]->freq )
            l++;
		huff->q[i] = huff->q[l], i = l;
	}
	huff->q[i] = huff->q[huff->qend];
	return(n);
}
 
void huff_buildcode(struct huffcode *huff,struct huffnode *n,char *s,int32_t len)
{
	static char buf[1024],*out = buf;
    uint64_t codebits;
    uint32_t ind;
    if ( n == 0 )
    {
        printf("huff_buildcode null n\n");
        while ( 1 ) sleep(1);
    }
	if ( (ind= n->ind) != 0 ) // leaf node
    {
        if ( ind <= huff->numinds )
        {
            s[len] = 0;
            strcpy(out,s);
            codebits = huff_convstr(out);
            huff->numnodes++;
            huff->items[ind].codebits = codebits;
            huff->items[ind].numbits = len;
            huff->totalbits += len * huff->items[ind].freq;
            huff->totalbytes += huff->items[ind].size * huff->items[ind].freq;
            if ( len > huff->maxbits )
                huff->maxbits = len;
            if ( ind > huff->maxind )
                huff->maxind = ind;
            out += len + 1;
            fprintf(stderr,"%6d: %8s (%12s).%-2d | nodes.%d bytes.%d -> bits.%d %.3f\n",ind,out,huff_str(codebits,len),len,huff->numnodes,huff->totalbytes,huff->totalbits,((double)huff->totalbytes*8)/huff->totalbits);
            return;
        } else printf("FATAL: ind.%d overflow vs numinds.%d\n",ind,huff->numinds);
    }
    else // combo node
    {
        s[len] = '0'; huff_buildcode(huff,n->left,s,len + 1);
        s[len] = '1'; huff_buildcode(huff,n->right,s,len + 1);
    }
}

void inttobits(uint64_t c,int32_t n,char *s)
{
    s[n] = 0;
    while ( n > 0 )
    {
        s[n-1] = (c%2) + '0';
        c >>= 1;
        n--;
    }
}
*/

int32_t decodetest(uint32_t *indp,int32_t *tree,int32_t depth,int32_t val,int32_t numinds)
{
    int32_t i,ind;
    *indp = -1;
    for (i=0; i<depth; )
    {
        if ( (ind= tree[((1 << i++) & val) != 0]) < 0 )
        {
            tree -= ind;
            continue;
        }
        *indp = ind;
        if ( ind >= numinds )
            printf("decode error: val.%x -> ind.%d >= numinds %d\n",val,ind,numinds);
        return(i);
    }
    printf("decodetest error: val.%d\n",val);
    return(-1);
}

struct huffcode *huff_create(const struct huffitem **items,int32_t numinds,int32_t frequi)
{
    int32_t depth,numbits,pred,ix,r1,r2,i,n,extf,*preds;
    uint32_t *efreqs,*revtree;
    struct huffcode *huff;
    struct huffitem *hip;
    uint64_t bc;
    heap_t *heap;
    extf = numinds;
    revtree = calloc(2 * numinds,sizeof(*revtree));
    efreqs = calloc(2 * numinds,sizeof(*efreqs));
    preds = calloc(2 * numinds,sizeof(*preds));
    for (i=0; i<numinds; i++)
        efreqs[i] = items[i]->freq[frequi] * items[i]->wt;
    memset(&efreqs[numinds],0,sizeof(*efreqs) * numinds);
    if ( (heap= _heap_create(numinds*2,efreqs)) == NULL )
    {
        free(efreqs);
        free(preds);
        return(NULL);
    }
    huff = calloc(numinds,sizeof(*huff));// + (sizeof(*huff->pool)));
    huff->items = items;
    huff->numinds = numinds;
   //printf("heap.%p h.%p s.%d\n",heap,heap->h,heap->s);
    for (i=0; i<numinds; i++)
        if ( efreqs[i] > 0 )
            _heap_add(heap,i);
    depth = 0;
    while ( heap->n > 1 )
    {
        r1 = _heap_remove(heap);
        r2 = _heap_remove(heap);
        efreqs[extf] = (efreqs[r1] + efreqs[r2]);
        revtree[(depth << 1) + 1] = r1;
        revtree[(depth << 1) + 0] = r2;
        depth++;
        _heap_add(heap,extf);
        preds[r1] = extf;
        preds[r2] = -extf;
        //printf("r1.%d (%d) <- %d | r2.%d (%d) <- %d\n",r1,efreqs[r1],extf,r2,efreqs[r2],-extf);
        extf++;
    }
    huff->depth = depth;
    r1 = _heap_remove(heap);
    n = 0;
    huff->tree = calloc(sizeof(*huff->tree),2*(extf-1));
    for (i=depth-1; i>=0; i--)
    {
        //printf("%d.[%d:%d%s %d:%d%s] ",numinds+i,-(numinds+i-revtree[i][0])*2,revtree[i][0],revtree[i][0] < numinds ? ".*" : "  ",-(numinds+i-revtree[i][1])*2,revtree[i][1],revtree[i][1] < numinds ? ".*" : "  ");
        huff->tree[n<<1] = (revtree[i*2+0] < numinds) ? revtree[i*2+0] : -(numinds+i-revtree[i*2+0])*2;
        huff->tree[(n<<1) + 1] = (revtree[i*2+1] < numinds) ? revtree[i*2+1] : -(numinds+i-revtree[i*2+1])*2;
        n++;
    }
    free(revtree);
    //printf("tree depth.%d n.%d\n",depth,n);
    /*for (i=0; i<8; i++)
    {
        n = decodetest(&ind,tree,depth,i,numinds);
        printf("i.%d %s -> %d bits ind.%d\n",i,huff_str(i,n),n,ind);
    }*/
    preds[r1] = r1;
    _heap_destroy(heap);
    for (i=0; i<numinds; i++)
    {
        bc = numbits = 0;
        if ( efreqs[i] != 0 )
        {
            ix = i;
            pred = preds[ix];
            while ( pred != ix && -pred != ix )
            {
                if ( pred >= 0 )
                {
                    bc |= (1L << numbits);
                    ix = pred;
                }
                else ix = -pred;
                pred = preds[ix];
                numbits++;
            }
            if ( numbits > huff->maxbits )
                huff->maxbits = numbits;
            huff->maxind = i;
            huff->numnodes++;
            if ( numbits > huff->maxbits )
                huff->maxbits = numbits;
            if ( i > huff->maxind )
                huff->maxind = i;
            if ( (hip = (struct huffitem *)huff->items[i]) != 0 )
            {
                hip->codebits = _reversebits(bc,numbits);
                hip->numbits = numbits;
                huff->totalbits += numbits * hip->freq[frequi];
                huff->totalbytes += hip->wt * hip->size * hip->freq[frequi];
            } else printf("FATAL: null item ptr.%d\n",i);
        }
    }
    free(preds);
    free(efreqs);
    if ( huff->maxbits >= (1<<MAX_HUFFBITS) )
    {
        printf("maxbits.%d wont fit in (1 << MAX_HUFFBITS.%d)\n",huff->maxbits,MAX_HUFFBITS);
        huff_free(huff);
        return(0);
    }
    //huff_disp(huff);
    return(huff);
}

int32_t huff_decode(struct huffcode *huff,uint8_t *output,int32_t maxlen,HUFF *hp)
{
    int32_t i,c = 0,*tree,ind,numinds,depth,num = 0;
    output[0] = 0;
    numinds = huff->numinds;
    depth = huff->depth;
	while ( 1 )
    {
        tree = huff->tree;
        for (i=0; i<depth; i++)
        {
            if ( (c= hgetbit(hp)) < 0 )
                break;
            //printf("%c",c+'0');
            if ( (ind= tree[c]) < 0 )
            {
                tree -= ind;
                continue;
            }
            if ( ind >= numinds )
            {
                printf("decode error: val.%x -> ind.%d >= numinds %d\n",c,ind,numinds);
                return(-1);
            }
            else
            {
                if ( (num= huff_output(huff,output,num,maxlen,ind)) < 0 )
                    return(-1);
                //output[num++] = ind;
            }
            break;
        }
        if ( c < 0 )
            break;
	}
    //printf("(%s) huffdecode num.%d\n",output,num);
    return(num);
}

int32_t emit_bitstream(uint8_t *bits,int32_t bitpos,uint32_t raw,struct huffentry *code,int32_t maxind)
{
    int32_t i,n = 0;
    uint64_t codebits;
    if ( raw < maxind )
    {
        n = code[raw].numbits;
        codebits = code[raw].bits;
        for (i=0; i<n; i++,codebits>>=1,bitpos++)
        {
            //printf("%c",(char)((codebits & 1) + '0'));
            if ( (codebits & 1) != 0 )
                SETBIT(bits,bitpos);
        }
    } else printf("raw.%u >= maxind.%d\n",raw,maxind);
    return(n);
}

/*

int32_t huff_encode(struct huffcode *huff,HUFF *hp,char *s)
{
    uint64_t codebits;
    int32_t i,n,count = 0;
	while ( *s )
    {
        codebits = huff->items[(int)*s].codebits;
        n = huff->items[(int)*s].numbits;
        for (i=0; i<n; i++,codebits>>=1)
            hputbit(hp,codebits & 1);
        count += n;
        s++;
	}
    return(count);
}

struct huffcode *testhuffcode(char *str,struct huffitem *items,int32_t numinds)
{
    uint8_t bits2[8192],output[8192];
    int32_t i,c,len,num,dlen;
    struct huffcode *huff;
    HUFF *hp;
    double startmilli = milliseconds();
    huff = huff_create(items,numinds);
    //huff_disp(huff);
    if ( str != 0 )
    {
        printf("elapsed time %.3f millis (%s)\n",milliseconds() - startmilli,str);
        len = (int32_t)strlen(str);
        memset(bits2,0,sizeof(bits2));
        hp = hopen(bits2,sizeof(bits2));
        for (i=num=0; i<len; i++)
        {
            c = str[i];
            num += hwrite(huff->items[c].codebits,huff->items[c].numbits,hp);
        }
        hrewind(hp);
        dlen = huff_decode(huff,output,(int32_t)sizeof(output),hp);
        output[num] = 0;
        fprintf(stderr,"(%d:%d -> %d %.2f).%d ",dlen,len*8,num,(double)(len*8)/num,memcmp((char *)output,str,len));
    }
    return(huff);
}


 //memset(bits,0,sizeof(bits));
 //for (i=num=0; i<len&&str[i]!=0; i++)
 //    num += emit_bitstream(bits,num,(uint8_t)str[i],huff->codes,numinds);
 //printf("numinds.%d maxbits.%d num for str.%d vs %d [%.2f]\n",numinds,huff->maxbits,num,len*8,((double)len * 8) / num);
 //for (i=0; i<num; i++)
 //    printf("%c",(GETBIT(bits,i) != 0) + '0');
 //printf("\n");

 
 struct huffcode *huff_init(struct huffitem *items,int32_t numinds)
{
    struct huffcode *huff;
    uint32_t ind,nonz;
   	char c[1024];
    huff = calloc(numinds,sizeof(*huff) + (sizeof(*huff->pool)));
    huff->items = items;
    huff->numinds = numinds;
    huff->qend = 1;
    huff->qqq = calloc(1,sizeof(*huff->qqq) * numinds);
    huff->q = huff->qqq - 1;

	for (ind=0; ind<numinds; ind++)
        huff_insert(huff,huff_leafnode(huff,ind));
    printf("inserted qend.%d maxin.%d\n",huff->qend,huff->maxind);
    for (ind=nonz=0; ind<numinds; ind++)
        if ( huff->q[ind] != 0 )
            printf("(%p %p %d).%d ",huff->q[ind]->left,huff->q[ind]->right,huff->q[ind]->freq,huff->q[ind]->ind), nonz++;
    printf("nonz.%d\n",nonz);
    while ( huff->qend > 2 )
		huff_insert(huff,huff_newnode(huff,0,0,huff_remove(huff),huff_remove(huff)));
    printf("coalesced\n");
    for (ind=nonz=0; ind<numinds; ind++)
        if ( huff->q[ind] != 0 )
            printf("(%p %p %d).%d ",huff->q[ind]->left,huff->q[ind]->right,huff->q[ind]->freq,huff->q[ind]->ind), nonz++;
    printf("nonz.%d\n",nonz);
    huff_buildcode(huff,huff->q[1],c,0);
    huff->numinds++;
    if ( huff->maxbits >= (1<<MAX_HUFFBITS) )
    {
        printf("maxbits.%d wont fit in (1 << MAX_HUFFBITS.%d)\n",huff->maxbits,MAX_HUFFBITS);
        huff_free(huff);
        return(0);
    }
    return(huff);
}

int32_t huffdecode(struct huffcode *huff,uint8_t *output,int32_t maxlen,HUFF *hp,struct huffnode *t)
{
    int32_t c,num = 0;
	struct huffnode *n = t;
	while ( (c= hgetbit(hp)) >= 0 )
    {
		if ( c == 0 )
            n = n->left;
		else n = n->right;
		if ( n->ind != 0 )
        {
            if ( (num = huff_output(huff,output,num,maxlen,n->ind)) < 0 )
                return(-1);
            n = t;
        }
	}
    printf("(%s) huffdecode num.%d\n",output,num);
	if ( t != n )
        printf("garbage input\n");
    return(num);
}

int testhuffcode(char *str,struct huffitem *items,int32_t numinds)
{
    uint8_t bits2[8192],output[8192];
    struct huffcode *huff;
    int num;
    HUFF *hp;
    double endmilli,startmilli = milliseconds();
	if ( (huff= teststuff(str,items,numinds)) == 0 )
        printf("error huff codec\n");
    //return(0);

    //huff = huff_init(items,numinds);
    endmilli = milliseconds();
	//for (i=0; i<numinds; i++)
	//	if ( huff->code[i].numbits != 0 )
    //        printf("'%c': %llx.%d\n",i,(long long)huff->code[i].bits,huff->code[i].numbits);
    printf("%.3f millis to encode\n",endmilli-startmilli);
    hp = hopen(bits2,sizeof(bits2));
    num = huff_encode(huff,hp,str);
    hrewind(hp);
	printf("encoded: %s\n",str);
    for (i=0; i<num; i++)
        printf("%c",(GETBIT(bits2,i) != 0) + '0');
    printf(" bits2 num.%d\n",num);
    hrewind(hp);
    for (i=0; i<num; i++)
        printf("%c",(hgetbit(hp) + '0'));
    printf(" hgetbit num.%d\n",num);

	printf("decoded: ");
    startmilli = milliseconds();
    hrewind(hp);
    num = huff_decode(huff,output,sizeof(output),hp);
    endmilli = milliseconds();
    output[num] = 0;
    huff_free(huff);
    hclose(hp);
    printf("%s\nnum.%d %.3f millis\n",output,num,endmilli - startmilli);
	return(0);
}
*/


#endif
