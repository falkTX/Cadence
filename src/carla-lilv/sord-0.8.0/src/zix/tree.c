/*
  Copyright 2011 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "tree.h"

typedef struct ZixTreeNodeImpl ZixTreeNode;

struct ZixTreeImpl {
	ZixTreeNode*  root;
	ZixComparator cmp;
	void*         cmp_data;
        size_t        size;
	bool          allow_duplicates;
};

struct ZixTreeNodeImpl {
	void*                   data;
	struct ZixTreeNodeImpl* left;
	struct ZixTreeNodeImpl* right;
	struct ZixTreeNodeImpl* parent;
	int_fast8_t             balance;
};

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

ZIX_API
ZixTree*
zix_tree_new(bool allow_duplicates, ZixComparator cmp, void* cmp_data)
{
	ZixTree* t = (ZixTree*)malloc(sizeof(ZixTree));
	t->root             = NULL;
	t->cmp              = cmp;
	t->cmp_data         = cmp_data;
        t->size             = 0;
	t->allow_duplicates = allow_duplicates;
	return t;
}

static void
zix_tree_free_rec(ZixTreeNode* n)
{
	if (n) {
		zix_tree_free_rec(n->left);
		zix_tree_free_rec(n->right);
		free(n);
	}
}

ZIX_API
void
zix_tree_free(ZixTree* t)
{
	zix_tree_free_rec(t->root);

	free(t);
}

size_t
zix_tree_size(ZixTree* t)
{
        return t->size;
}

static void
rotate(ZixTreeNode* p, ZixTreeNode* q)
{
	assert(q->parent == p);
	assert(p->left == q || p->right == q);

	q->parent = p->parent;
	if (q->parent) {
		if (q->parent->left == p) {
			q->parent->left = q;
		} else {
			q->parent->right = q;
		}
	}

	if (p->right == q) {
		// Rotate left
		p->right = q->left;
		q->left  = p;
		if (p->right) {
			p->right->parent = p;
		}
	} else {
		// Rotate right
		assert(p->left == q);
		p->left  = q->right;
		q->right = p;
		if (p->left) {
			p->left->parent = p;
		}
	}

	p->parent = q;
}

/**
 * Rotate left about @a p.
 *
 *    p              q
 *   / \            / \
 *  A   q    =>    p   C
 *     / \        / \
 *    B   C      A   B
 */
static ZixTreeNode*
rotate_left(ZixTreeNode* p, int* height_change)
{
	ZixTreeNode* const q = p->right;
	*height_change = (q->balance == 0) ? 0 : -1;

	assert(p->balance == 2);
	assert(q->balance == 0 || q->balance == 1);

	rotate(p, q);

	--q->balance;
	p->balance = -(q->balance);

	return q;
}

/**
 * Rotate right about @a p.
 *
 *      p          q
 *     / \        / \
 *    q   C  =>  A   p
 *   / \            / \
 *  A   B          B   C
 *
 */
static ZixTreeNode*
rotate_right(ZixTreeNode* p, int* height_change)
{
	ZixTreeNode* const q = p->left;
	*height_change = (q->balance == 0) ? 0 : -1;

	assert(p->balance == -2);
	assert(q->balance == 0 || q->balance == -1);

	rotate(p, q);

	++q->balance;
	p->balance = -(q->balance);

	return q;
}

/**
 * Rotate left about @a p->left then right about @a p.
 *
 *      p             r
 *     / \           / \
 *    q   D  =>    q     p
 *   / \          / \   / \
 *  A   r        A   B C   D
 *     / \
 *    B   C
 *
 */
static ZixTreeNode*
rotate_left_right(ZixTreeNode* p, int* height_change)
{
	ZixTreeNode* const q = p->left;
	ZixTreeNode* const r = q->right;

	assert(p->balance == -2);
	assert(q->balance == 1);
	assert(r->balance == -1 || r->balance == 0 || r->balance == 1);

	rotate(q, r);
	rotate(p, r);

	q->balance -= 1 + MAX(0, r->balance);
	p->balance += 1 - MIN(MIN(0, r->balance) - 1, r->balance + q->balance);
	r->balance = 0;

	*height_change = -1;

	return r;
}

/**
 * Rotate right about @a p->right then right about @a p.
 *
 *    p               r
 *   / \             / \
 *  A   q    =>    p     q
 *     / \        / \   / \
 *    r   D      A   B C   D
 *   / \
 *  B   C
 *
 */
static ZixTreeNode*
rotate_right_left(ZixTreeNode* p, int* height_change)
{
	ZixTreeNode* const q = p->right;
	ZixTreeNode* const r = q->left;

	assert(p->balance == 2);
	assert(q->balance == -1);
	assert(r->balance == -1 || r->balance == 0 || r->balance == 1);

	rotate(q, r);
	rotate(p, r);

	q->balance += 1 - MIN(0, r->balance);
	p->balance -= 1 + MAX(MAX(0, r->balance) + 1, r->balance + q->balance);
	r->balance = 0;

	*height_change = -1;

	return r;
}

static ZixTreeNode*
zix_tree_rebalance(ZixTree* t, ZixTreeNode* node, int* height_change)
{
	*height_change = 0;
	const bool is_root = !node->parent;
	assert((is_root && t->root == node) || (!is_root && t->root != node));
	ZixTreeNode* replacement = node;
	if (node->balance == -2) {
		assert(node->left);
		if (node->left->balance == 1) {
			replacement = rotate_left_right(node, height_change);
		} else {
			replacement = rotate_right(node, height_change);
		}
	} else if (node->balance == 2) {
		assert(node->right);
		if (node->right->balance == -1) {
			replacement = rotate_right_left(node, height_change);
		} else {
			replacement = rotate_left(node, height_change);
		}
	}
	if (is_root) {
		assert(!replacement->parent);
		t->root = replacement;
	}

	return replacement;
}

ZIX_API
ZixStatus
zix_tree_insert(ZixTree* t, void* e, ZixTreeIter** ti)
{
	int          cmp = 0;
	ZixTreeNode* n   = t->root;
	ZixTreeNode* p   = NULL;

	// Find the parent p of e
	while (n) {
		p   = n;
		cmp = t->cmp(e, n->data, t->cmp_data);
		if (cmp < 0) {
			n = n->left;
		} else if (cmp > 0) {
			n = n->right;
		} else if (t->allow_duplicates) {
			n = n->right;
		} else {
			if (ti) {
				*ti = n;
			}
			return ZIX_STATUS_EXISTS;
		}
	}

	// Allocate a new node n
	if (!(n = (ZixTreeNode*)malloc(sizeof(ZixTreeNode)))) {
		return ZIX_STATUS_NO_MEM;
	}
	memset(n, '\0', sizeof(ZixTreeNode));
	n->data    = e;
	n->balance = 0;
	if (ti) {
		*ti = n;
	}

	bool p_height_increased = false;

	// Make p the parent of n
	n->parent = p;
	if (!p) {
		t->root = n;
	} else {
		if (cmp < 0) {
			assert(!p->left);
			assert(p->balance == 0 || p->balance == 1);
			p->left = n;
			--p->balance;
			p_height_increased = !p->right;
		} else {
			assert(!p->right);
			assert(p->balance == 0 || p->balance == -1);
			p->right = n;
			++p->balance;
			p_height_increased = !p->left;
		}
	}

	// Rebalance if necessary (at most 1 rotation)
	assert(!p || p->balance == -1 || p->balance == 0 || p->balance == 1);
	if (p && p_height_increased) {
		int height_change = 0;
		for (ZixTreeNode* i = p; i && i->parent; i = i->parent) {
			if (i == i->parent->left) {
				if (--i->parent->balance == -2) {
					zix_tree_rebalance(t, i->parent, &height_change);
					break;
				}
			} else {
				assert(i == i->parent->right);
				if (++i->parent->balance == 2) {
					zix_tree_rebalance(t, i->parent, &height_change);
					break;
				}
			}

			if (i->parent->balance == 0) {
				break;
			}
		}
	}

        ++t->size;

	return ZIX_STATUS_SUCCESS;
}

ZIX_API
ZixStatus
zix_tree_remove(ZixTree* t, ZixTreeIter* ti)
{
	ZixTreeNode* const n          = ti;
	ZixTreeNode**      pp         = NULL;  // parent pointer
	ZixTreeNode*       to_balance = n->parent;  // lowest node to balance
	int8_t             d_balance  = 0;  // delta(balance) for n->parent

	if ((n == t->root) && !n->left && !n->right) {
		t->root = NULL;
		free(n);
                --t->size;
                assert(t->size == 0);
		return ZIX_STATUS_SUCCESS;
	}

	// Set pp to the parent pointer to n, if applicable
	if (n->parent) {
		assert(n->parent->left == n || n->parent->right == n);
		if (n->parent->left == n) {  // n is left child
			pp        = &n->parent->left;
			d_balance = 1;
		} else {  // n is right child
			assert(n->parent->right == n);
			pp        = &n->parent->right;
			d_balance = -1;
		}
	}

	assert(!pp || *pp == n);

	int height_change = 0;
	if (!n->left && !n->right) {
		// n is a leaf, just remove it
		if (pp) {
			*pp           = NULL;
			to_balance    = n->parent;
			height_change = (!n->parent->left && !n->parent->right) ? -1 : 0;
		}
	} else if (!n->left) {
		// Replace n with right (only) child
		if (pp) {
			*pp        = n->right;
			to_balance = n->parent;
		} else {
			t->root = n->right;
		}
		n->right->parent = n->parent;
		height_change    = -1;
	} else if (!n->right) {
		// Replace n with left (only) child
		if (pp) {
			*pp        = n->left;
			to_balance = n->parent;
		} else {
			t->root = n->left;
		}
		n->left->parent = n->parent;
		height_change   = -1;
	} else {
		// Replace n with in-order successor (leftmost child of right subtree)
		ZixTreeNode* replace = n->right;
		while (replace->left) {
			assert(replace->left->parent == replace);
			replace = replace->left;
		}

		// Remove replace from parent (replace_p)
		if (replace->parent->left == replace) {
			height_change = replace->parent->right ? 0 : -1;
			d_balance     = 1;
			to_balance    = replace->parent;
			replace->parent->left = replace->right;
		} else {
			assert(replace->parent == n);
			height_change = replace->parent->left ? 0 : -1;
			d_balance     = -1;
			to_balance    = replace->parent;
			replace->parent->right = replace->right;
		}

		if (to_balance == n) {
			to_balance = replace;
		}

		if (replace->right) {
			replace->right->parent = replace->parent;
		}

		replace->balance = n->balance;

		// Swap node to delete with replace
		if (pp) {
			*pp = replace;
		} else {
			assert(t->root == n);
			t->root = replace;
		}
		replace->parent = n->parent;
		replace->left   = n->left;
		n->left->parent = replace;
		replace->right  = n->right;
		if (n->right) {
			n->right->parent = replace;
		}

		assert(!replace->parent
		       || replace->parent->left == replace
		       || replace->parent->right == replace);
	}

	// Rebalance starting at to_balance upwards.
	for (ZixTreeNode* i = to_balance; i; i = i->parent) {
		i->balance += d_balance;
		if (d_balance == 0 || i->balance == -1 || i->balance == 1) {
			break;
		}

		assert(i != n);
		i = zix_tree_rebalance(t, i, &height_change);
		if (i->balance == 0) {
			height_change = -1;
		}

		if (i->parent) {
			if (i == i->parent->left) {
				d_balance = height_change * -1;
			} else {
				assert(i == i->parent->right);
				d_balance = height_change;
			}
		}
	}

	free(n);

        --t->size;

	return ZIX_STATUS_SUCCESS;
}

ZIX_API
ZixStatus
zix_tree_find(const ZixTree* t, const void* e, ZixTreeIter** ti)
{
	ZixTreeNode* n = t->root;
	while (n) {
		const int cmp = t->cmp(e, n->data, t->cmp_data);
		if (cmp == 0) {
			break;
		} else if (cmp < 0) {
			n = n->left;
		} else {
			n = n->right;
		}
	}

	*ti = n;
	return (n) ? ZIX_STATUS_SUCCESS : ZIX_STATUS_NOT_FOUND;
}

ZIX_API
void*
zix_tree_get(ZixTreeIter* ti)
{
	return ti->data;
}

ZIX_API
ZixTreeIter*
zix_tree_begin(ZixTree* t)
{
	if (!t->root) {
		return NULL;
	}

	ZixTreeNode* n = t->root;
	while (n->left) {
		n = n->left;
	}
	return n;
}

ZIX_API
ZixTreeIter*
zix_tree_end(ZixTree* t)
{
	return NULL;
}

ZIX_API
bool
zix_tree_iter_is_end(ZixTreeIter* i)
{
	return !i;
}

ZIX_API
ZixTreeIter*
zix_tree_iter_next(ZixTreeIter* i)
{
	if (!i) {
		return NULL;
	}

	if (i->right) {
		i = i->right;
		while (i->left) {
			i = i->left;
		}
	} else {
		while (i->parent && i->parent->right == i) {  // i is a right child
			i = i->parent;
		}

		i = i->parent;
	}

	return i;
}

ZIX_API
ZixTreeIter*
zix_tree_iter_prev(ZixTreeIter* i)
{
	if (!i) {
		return NULL;
	}

	if (i->left) {
		i = i->left;
		while (i->right) {
			i = i->right;
		}
	} else {
		while (i->parent && i->parent->left == i) {  // i is a left child
			i = i->parent;
		}

		i = i->parent;
	}

	return i;
}
