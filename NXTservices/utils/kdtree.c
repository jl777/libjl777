#ifndef KDTREE_C
#define KDTREE_C
/*
This file is part of ``kdtree'', a library for working with kd-trees.
Copyright (C) 2007-2011 John Tsiombikas <nuclear@member.fsf.org>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/
/* single nearest neighbor search written by Tamas Nepusz <tamas@cs.rhul.ac.uk> */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "kdtree.h"

#if defined(WIN32) || defined(__WIN32__)
#include <malloc.h>
#endif

#ifdef USE_LIST_NODE_ALLOCATOR

#ifndef NO_PTHREADS
#include <pthread.h>
#else

#ifndef I_WANT_THREAD_BUGS
#error "You are compiling with the fast list node allocator, with pthreads disabled! This WILL break if used from multiple threads."
#endif	/* I want thread bugs */

#endif	/* pthread support */
#endif	/* use list node allocator */


double calc_dist_sq(const kdtype *refpos,const kdtype *nvals,int32_t dim,int32_t ordered)
{
	int32_t i;
	float diff;
	double dist_sq;
	dist_sq = 0.;
	if ( ordered >= 0 )
	{
		for (i=0; i<dim; i++) 
			dist_sq += SQ(refpos[i] - nvals[i]);
	}
	else
	{
		for (i=0; i<dim; i++)
		{
			if ( refpos[i] != 0.f && nvals[i] != 0.f )
				dist_sq += SQ(refpos[i] - nvals[i]);
			else if ( refpos[i] != 0.f || nvals[i] != 0.f )
			{
				diff = (refpos[i] + nvals[i]) / 2.f;
				dist_sq += (diff < 0.f) ? -diff : diff;
				//printf("zcase(%f %f) ",refpos[i],nvals[i]);
			}
		}
	}
	return(dist_sq);
}

// ---- hyperrectangle helpers ----
struct kdhyperrect *hyperrect_create(int32_t dim,const kdtype *min,const kdtype *max)
{
	size_t minmaxsize = dim * sizeof(kdtype);
	struct kdhyperrect *rect = 0;
	if ( !(rect = malloc(sizeof(struct kdhyperrect) + 2*minmaxsize)) )
		return(0);
	/*if ( !(rect->min = malloc(size)) ) {
	 free(rect);
	 return(0);
	 }
	 if ( !(rect->max = malloc(size)) ) {
	 free(rect->min);
	 free(rect);
	 return(0);
	 }*/
	rect->min = (kdtype *)((long)rect + sizeof(*rect));
	rect->max = (kdtype *)((long)rect + sizeof(*rect) + minmaxsize);
	rect->dim = dim;
	memcpy(rect->min,min,minmaxsize);
	memcpy(rect->max,max,minmaxsize);
	return(rect);
}

struct kdhyperrect *hyperrect_duplicate(const struct kdhyperrect *rect)
{
	size_t minmaxsize = rect->dim * sizeof(kdtype);
	struct kdhyperrect *clone = 0;
	if ( !(clone = malloc(sizeof(struct kdhyperrect) + 2*minmaxsize)) )
		return(0);
	memcpy(clone,rect,sizeof(struct kdhyperrect) + 2*minmaxsize);
	return(clone);
}

void hyperrect_free(struct kdhyperrect *rect)
{
	//free(rect->min);
	//free(rect->max);
	free(rect);
}

void hyperrect_extend(struct kdhyperrect *rect,const kdtype *pos)
{
	int32_t i;
	for (i=0; i<rect->dim; i++) 
	{
		if ( pos[i] < rect->min[i] ) 
			rect->min[i] = pos[i];
		if ( pos[i] > rect->max[i] ) 
			rect->max[i] = pos[i];
	}
}

double hyperrect_dist_sq(struct kdhyperrect *rect,const kdtype *pos)
{
	int32_t i;
	double diff,result = 0;
	for (i=0; i<rect->dim; i++) 
	{
		if ( pos[i] < rect->min[i] )
			diff = (rect->min[i] - pos[i]);
		else if ( pos[i] > rect->max[i] )
			diff = (rect->max[i] - pos[i]);
		else continue;
		result += diff * diff;
	}
	return(result);
}

struct kdnode *insert_rec(struct kdnode **nptr,const kdtype *pos,void *data,int32_t dir,int32_t dim)
{
	int32_t new_dir;
	struct kdnode *node;
	if ( *nptr == 0 )
	{
		if ( !(node = (struct kdnode *)alloc_aligned_buffer(sizeof(*node) + dim * sizeof *node->pos)) )
			return(0);
		/*if ( !(node->pos = malloc(dim * sizeof *node->pos)) )
		{
			free(node);
			return(0);
		}*/
		memcpy(node->pos,pos,dim * sizeof(*node->pos));
		node->data = data;
		node->dir = dir;
		node->left = node->right = 0;
		//printf("created %p\n",node);
		*nptr = node;
		return(node);
	}
	node = *nptr;
	new_dir = (node->dir + 1) % dim;
	if ( pos[node->dir] < node->pos[node->dir] )
		return(insert_rec(&(*nptr)->left, pos, data, new_dir, dim));
	return(insert_rec(&(*nptr)->right, pos, data, new_dir, dim));
}

struct kdnode *kd_insert(struct kdtree *tree,const kdtype *pos,void *data)
{
	struct kdnode *node;
	if ( (node= insert_rec(&tree->root, pos, data, 0, tree->dim)) == 0 )
		return(0);
	if ( tree->rect == 0 )
		tree->rect = hyperrect_create(tree->dim, pos, pos);
	else hyperrect_extend(tree->rect, pos);
	return(node);
}

struct kdnode *kd_insert3(struct kdtree *tree, kdtype x, kdtype y, kdtype z, void *data)
{
	kdtype buf[3];
	buf[0] = x;
	buf[1] = y;
	buf[2] = z;
	return(kd_insert(tree, buf, data));
}

void clear_rec(struct kdnode *node, void (*destr)(void*))
{
	if ( node == 0 ) 
		return;
	clear_rec(node->left, destr);
	clear_rec(node->right, destr);
	if ( destr != 0 )
		destr(node->data);
	//free(node->pos);
	free(node);
}

void kd_data_destructor(struct kdtree *tree, void (*destr)(void*))
{
	tree->destr = destr;
}

struct kdtree *kd_create(int32_t dimensions)
{
	struct kdtree *tree;
	if ( dimensions >= (1<<14) )
		printf("kd_create dir bitfield too small\n");
	printf("size of kdnode %ld\n",sizeof(struct kdnode));
	if ( !(tree = malloc(sizeof *tree)) )
		return(0);
	tree->dim = dimensions;
	tree->root = 0;
	tree->destr = 0;
	tree->rect = 0;
	return(tree);
}

void kd_clear(struct kdtree *tree)
{
	clear_rec(tree->root, tree->destr);
	tree->root = 0;
	if ( tree->rect != 0 ) 
	{
		hyperrect_free(tree->rect);
		tree->rect = 0;
	}
}

void kd_free(struct kdtree *tree)
{
	if ( tree != 0 ) 
	{
		kd_clear(tree);
		free(tree);
	}
}

// ---- static helpers ---- 

#ifdef USE_LIST_NODE_ALLOCATOR
// special list node allocators. 
static struct res_node *free_nodes;
#define NO_PTHREADS
#ifndef NO_PTHREADS
static pthread_mutex_t alloc_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

struct res_node *alloc_resnode(void)
{
	struct res_node *node;
#ifndef NO_PTHREADS
	pthread_mutex_lock(&alloc_mutex);
#endif
	if ( free_nodes == 0 )
		node = malloc(sizeof *node);
	else
	{
		node = free_nodes;
		free_nodes = free_nodes->next;
		node->next = 0;
	}
#ifndef NO_PTHREADS
	pthread_mutex_unlock(&alloc_mutex);
#endif
	return(node);
}

static void free_resnode(struct res_node *node)
{
#ifndef NO_PTHREADS
	pthread_mutex_lock(&alloc_mutex);
#endif
	node->next = free_nodes;
	free_nodes = node;
#ifndef NO_PTHREADS
	pthread_mutex_unlock(&alloc_mutex);
#endif
}
#endif	/* list node allocator or not */

// inserts the item. if dist_sq is >= 0, then do an ordered insert 
// TODO make the ordering code use heapsort 
static int32_t rlist_insert(struct res_node *list, struct kdnode *item, double dist_sq)
{
	struct res_node *rnode;
	if ( (rnode = alloc_resnode()) == 0 ) 
		return(-1);
	rnode->item = item;
	rnode->dist_sq = dist_sq;
	if ( dist_sq >= 0.0 )
		while ( list->next && list->next->dist_sq < dist_sq )
			list = list->next;
	rnode->next = list->next;
	list->next = rnode;
	return(0);
}

void clear_results(struct kdres *rset)
{
	struct res_node *tmp,*node = rset->rlist->next;
	while ( node != 0 )
	{
		tmp = node;
		node = node->next;
		free_resnode(tmp);
	}
	rset->rlist->next = 0;
}

int32_t find_nearest(struct kdnode *node,const kdtype *pos,double range,struct res_node *list,int32_t ordered,int32_t dim)
{
	double dist_sq, dx;
	int32_t ret,added_res = 0;
	if ( node == 0 ) 
		return(0);
	dist_sq = calc_dist_sq(node->pos,pos,dim,ordered);
	if ( dist_sq <= SQ(range) )
	{
		if ( rlist_insert(list, node, ordered ? dist_sq : -1.0) == -1 )
			return(-1);
		added_res = 1;
	}
	dx = pos[node->dir] - node->pos[node->dir];
	ret = find_nearest(dx <= 0.0 ? node->left : node->right, pos, range, list, ordered, dim);
	if ( ret >= 0 && fabs(dx) < range )
	{
		added_res += ret;
		ret = find_nearest(dx <= 0.0 ? node->right : node->left, pos, range, list, ordered, dim);
	}
	if ( ret == -1 )
		return(-1);
	added_res += ret;
	return(added_res);
}

#if 0
static int32_t find_nearest_n(struct kdnode *node, const kdtype *pos, double range, int32_t num, struct rheap *heap, int32_t dim)
{
	double dist_sq, dx;
	int32_t i, ret, added_res = 0;
	if ( !node ) return(0);
	// if the photon is close enough, add it to the result heap 
	dist_sq = calc_dist_sq(node->pos,pos,dim,ordered);
	if ( dist_sq <= range_sq )
	{
		if ( heap->size >= num ) // get furthest element
		{
			struct res_node *maxelem = rheap_get_max(heap);
			if ( maxelem->dist_sq > dist_sq ) // and check if the new one is closer than that
			{
				rheap_remove_max(heap);
				if ( rheap_insert(heap, node, dist_sq) == -1 )
					return(-1);
				added_res = 1;
				range_sq = dist_sq;
			}
		} 
		else 
		{
			if ( rheap_insert(heap, node, dist_sq) == -1 )
				return(-1);
			added_res = 1;
		}
	}
	// find signed distance from the splitting plane 
	dx = pos[node->dir] - node->pos[node->dir];
	ret = find_nearest_n(dx <= 0.0 ? node->left : node->right, pos, range, num, heap, dim);
	if (ret >= 0 && fabs(dx) < range )
	{
		added_res += ret;
		ret = find_nearest_n(dx <= 0.0 ? node->right : node->left, pos, range, num, heap, dim);
	}
}
#endif

void kd_nearest_i(struct kdnode *node,const kdtype *pos,struct kdnode **result,double *result_dist_sq,struct kdhyperrect *rect,int32_t ordered)
{
	int32_t dir = node->dir;
	double dummy,dist_sq;
	struct kdnode *nearer_subtree,*farther_subtree;
	kdtype *nearer_hyperrect_coord, *farther_hyperrect_coord;

	// Decide whether to go left or right in the tree 
	dummy = pos[dir] - node->pos[dir];
	if ( dummy <= 0 ) {
		nearer_subtree = node->left;
		farther_subtree = node->right;
		nearer_hyperrect_coord = rect->max + dir;
		farther_hyperrect_coord = rect->min + dir;
	} else {
		nearer_subtree = node->right;
		farther_subtree = node->left;
		nearer_hyperrect_coord = rect->min + dir;
		farther_hyperrect_coord = rect->max + dir;
	}
	if ( nearer_subtree != 0 )
	{
		dummy = *nearer_hyperrect_coord;			// Slice the hyperrect to get the hyperrect of the nearer subtree 
		*nearer_hyperrect_coord = node->pos[dir];
		kd_nearest_i(nearer_subtree, pos, result, result_dist_sq, rect,ordered); // Recurse down into nearer subtree 
		*nearer_hyperrect_coord = dummy;			// Undo the slice 
	}
	dist_sq = calc_dist_sq(node->pos,pos,rect->dim,ordered);
	if ( dist_sq < *result_dist_sq ) 
	{
		*result = node;
		*result_dist_sq = dist_sq;
	}
	if ( farther_subtree != 0 ) {
		dummy = *farther_hyperrect_coord;		// Get the hyperrect of the farther subtree
		*farther_hyperrect_coord = node->pos[dir];
		// Check if we have to recurse down by calculating the closest point of the hyperrect
		// and see if it's closer than our minimum distance in result_dist_sq.
		if ( hyperrect_dist_sq(rect,pos) < *result_dist_sq )
			
			kd_nearest_i(farther_subtree, pos, result, result_dist_sq, rect,ordered); // Recurse down into farther subtree 
		*farther_hyperrect_coord = dummy;	// Undo the slice on the hyperrect
	}
}

struct kdres *kd_nearest(struct kdtree *kd,const float *pos,int32_t ordered)
{
	double dist_sq;
	struct kdnode *result;
	struct kdhyperrect *rect;
	struct kdres *rset;
	if ( !kd ) return(0);
	if ( !kd->rect ) return(0);
	// Allocate result set
	if ( !(rset = malloc(sizeof *rset)) )
		return(0);
	if ( !(rset->rlist = alloc_resnode()) ) {
		free(rset);
		return(0);
	}
	rset->rlist->next = 0;
	rset->tree = kd;
	// Duplicate the bounding hyperrectangle, we will work on the copy 
	if ( !(rect = hyperrect_duplicate(kd->rect)) ) 
	{
		kd_res_free(rset);
		return(0);
	}
	//Our first guesstimate is the root node 
	result = kd->root;
	dist_sq = calc_dist_sq(result->pos,pos,kd->dim,ordered);
	// Search for the nearest neighbour recursively 
	kd_nearest_i(kd->root,pos,&result,&dist_sq,rect,ordered);
	hyperrect_free(rect);
	// Store the result 
	if ( result && rlist_insert(rset->rlist,result,-1.0) != -1 )
	{
		rset->size = 1;
		kd_res_rewind(rset);
		return(rset);
	} 
	else 
	{
		kd_res_free(rset);
		return(0);
	}
}

struct kdres *kd_nearest3(struct kdtree *tree,kdtype x,kdtype y,kdtype z,int32_t ordered)
{
	kdtype pos[3];
	pos[0] = x;
	pos[1] = y;
	pos[2] = z;
	return kd_nearest(tree,pos,ordered);
}

// ---- nearest N search ---- 
/*
static kdres *kd_nearest_n(struct kdtree *kd, const kdtype *pos, int32_t num)
{
	int32_t ret;
	struct kdres *rset;

	if(!(rset = malloc(sizeof *rset))) {
		return(0);
	}
	if(!(rset->rlist = alloc_resnode())) {
		free(rset);
		return(0);
	}
	rset->rlist->next = 0;
	rset->tree = kd;

	if((ret = find_nearest_n(kd->root, pos, range, num, rset->rlist, kd->dim)) == -1) {
		kd_res_free(rset);
		return(0);
	}
	rset->size = ret;
	kd_res_rewind(rset);
	return rset;
}*/

struct kdres *kd_nearest_range(struct kdtree *kd,const kdtype *pos,double range,int32_t orderedflag)
{
	int32_t ret;
	struct kdres *rset;

	if ( !(rset = malloc(sizeof *rset)) )
		return(0);
	if ( !(rset->rlist = alloc_resnode()) )
	{
		free(rset);
		return(0);
	}
	rset->rlist->next = 0;
	rset->tree = kd;
	if ( (ret = find_nearest(kd->root,pos,range,rset->rlist,orderedflag,kd->dim)) == -1 )
	{
		kd_res_free(rset);
		return(0);
	}
	rset->size = ret;
	kd_res_rewind(rset);
	return(rset);
}

struct kdres *kd_nearest_range3(struct kdtree *tree,kdtype x,kdtype y,kdtype z,double range,int32_t orderedflag)
{
	kdtype buf[3];
	buf[0] = x;
	buf[1] = y;
	buf[2] = z;
	return(kd_nearest_range(tree,buf,range,orderedflag));
}

void kd_res_free(struct kdres *rset)
{
	clear_results(rset);
	free_resnode(rset->rlist);
	free(rset);
}

int32_t kd_res_size(struct kdres *set)
{
	return (set->size);
}

void kd_res_rewind(struct kdres *rset)
{
	rset->riter = rset->rlist->next;
}

int32_t kd_res_end(struct kdres *rset)
{
	return rset->riter == 0;
}

int32_t kd_res_next(struct kdres *rset)
{
	rset->riter = rset->riter->next;
	return rset->riter != 0;
}

void *kd_res_item(struct kdres *rset,kdtype *pos)
{
	if ( rset->riter != 0 )
	{
		if ( pos != 0 )
			memcpy(pos,rset->riter->item->pos,rset->tree->dim * sizeof *pos);
		return(rset->riter->item->data);
	}
	else return(0);
}

void *kd_res_item3(struct kdres *rset,kdtype *x,kdtype *y,kdtype *z)
{
	if ( rset->riter )
	{
		if ( *x != 0 ) *x = rset->riter->item->pos[0];
		if ( *y != 0 ) *y = rset->riter->item->pos[1];
		if ( *z != 0 ) *z = rset->riter->item->pos[2];
	}
	return(0);
}

void *kd_res_item_data(struct kdres *set)
{
	return(kd_res_item(set,0));
}
#endif

