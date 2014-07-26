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
#ifndef _KDTREE_H_
#define _KDTREE_H_

extern void *alloc_aligned_buffer(uint64_t allocsize);
typedef float kdtype;

struct kdtree;
struct kdres;

struct kdhyperrect {
	int32_t dim;
	kdtype *min, *max;              /* minimum/maximum coords */
};

struct kdnode 
{
	struct kdnode *left, *right;	/* negative/positive side */
	void *data;
	float answer;
    uint32_t contractnum:6,weekind64:12,dir:14;
	kdtype pos[];
};

struct res_node {
	struct kdnode *item;
	double dist_sq;
	struct res_node *next;
};

struct kdtree {
	int32_t dim;
	struct kdnode *root;
	struct kdhyperrect *rect;
	void (*destr)(void*);
};

struct kdres {
	struct kdtree *tree;
	struct res_node *rlist, *riter;
	int32_t size;
};

#define SQ(x)			((x) * (x))


//static void clear_rec(struct kdnode *node, void (*destr)(void*));
//static int32_t insert_rec(struct kdnode **node, const kdtype *pos, void *data, int32_t dir, int32_t dim);
//static int32_t rlist_insert(struct res_node *list, struct kdnode *item, double dist_sq);
//static void clear_results(struct kdres *set);

#ifdef USE_LIST_NODE_ALLOCATOR
//static struct res_node *alloc_resnode(void);
//static void free_resnode(struct res_node*);
#else
#define alloc_resnode()		malloc(sizeof(struct res_node))
#define free_resnode(n)		free(n)
#endif


#ifdef __cplusplus
extern "C" {
#endif
	
/* create a kd-tree for "k"-dimensional data */
struct kdtree *kd_create(int32_t k);
double calc_dist_sq(const kdtype *refpos,const kdtype *nvals,int32_t dim,int32_t ordered);

/* free the struct kdtree */
void kd_free(struct kdtree *tree);

/* remove all the elements from the tree */
//void kd_clear(struct kdtree *tree);

/* if called with non-null 2nd argument, the function provided
 * will be called on data pointers (see kd_insert) when nodes
 * are to be removed from the tree.
 */
void kd_data_destructor(struct kdtree *tree, void (*destr)(void*));

/* insert a node, specifying its position, and optional data */
struct kdnode *kd_insert(struct kdtree *tree, const kdtype *pos, void *data);
struct kdnode *kd_insert3(struct kdtree *tree, kdtype x, kdtype y, kdtype z, void *data);

/* Find the nearest node from a given point.
 *
 * This function returns a pointer to a result set with at most one element.
 */
struct kdres *kd_nearest(struct kdtree *tree, const kdtype *pos,int32_t ordered);
struct kdres *kd_nearest3(struct kdtree *tree, kdtype x, kdtype y, kdtype z,int32_t ordered);

/* Find the N nearest nodes from a given point.
 *
 * This function returns a pointer to a result set, with at most N elements,
 * which can be manipulated with the kd_res_* functions.
 * The returned pointer can be null as an indication of an error. Otherwise
 * a valid result set is always returned which may contain 0 or more elements.
 * The result set must be deallocated with kd_res_free after use.
 */
/*
struct kdres *kd_nearest_n(struct kdtree *tree, const double *pos, int32_t num);
struct kdres *kd_nearest_nf(struct kdtree *tree, const float *pos, int32_t num);
struct kdres *kd_nearest_n3(struct kdtree *tree, double x, double y, double z);
struct kdres *kd_nearest_n3f(struct kdtree *tree, float x, float y, float z);
*/

/* Find any nearest nodes from a given point within a range.
 *
 * This function returns a pointer to a result set, which can be manipulated
 * by the kd_res_* functions.
 * The returned pointer can be null as an indication of an error. Otherwise
 * a valid result set is always returned which may contain 0 or more elements.
 * The result set must be deallocated with kd_res_free after use.
 */
struct kdres *kd_nearest_range(struct kdtree *tree,const kdtype *pos,double range,int32_t ordered);
struct kdres *kd_nearest_range3(struct kdtree *tree,kdtype x,kdtype y,kdtype z,double range,int32_t ordered);

/* frees a result set returned by kd_nearest_range() */
void kd_res_free(struct kdres *set);

/* returns the size of the result set (in elements) */
int32_t kd_res_size(struct kdres *set);

/* rewinds the result set iterator */
void kd_res_rewind(struct kdres *set);

/* returns non-zero if the set iterator reached the end after the last element */
int32_t kd_res_end(struct kdres *set);

/* advances the result set iterator, returns non-zero on success, zero if
 * there are no more elements in the result set.
 */
int32_t kd_res_next(struct kdres *set);

/* returns the data pointer (can be null) of the current result set item
 * and optionally sets its position to the pointers(s) if not null.
 */
void *kd_res_item(struct kdres *set, kdtype *pos);
void *kd_res_item3(struct kdres *set, kdtype *x, kdtype *y, kdtype *z);

/* equivalent to kd_res_item(set, 0) */
void *kd_res_item_data(struct kdres *set);


#ifdef __cplusplus
}
#endif

#endif	/* _KDTREE_H_ */
