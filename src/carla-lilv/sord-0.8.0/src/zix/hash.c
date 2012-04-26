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
#include <stdlib.h>
#include <string.h>

#include "hash.h"

/**
   Primes, each slightly less than twice its predecessor, and as far away
   from powers of two as possible.
*/
static const unsigned sizes[] = {
	53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317,
	196613, 393241, 786433, 1572869, 3145739, 6291469, 12582917, 25165843,
	50331653, 100663319, 201326611, 402653189, 805306457, 1610612741, 0
};

typedef struct _Entry {
	const void*    key;   ///< Hash key
	void*          data;  ///< Value
	struct _Entry* next;  ///< Next entry in bucket
	unsigned       hash;  ///< Non-modulo hash value (for cheap rehash)
} Entry;

struct ZixHashImpl {
	ZixHashFunc     hash_func;
	ZixEqualFunc    key_equal_func;
	Entry**         buckets;
	const unsigned* n_buckets;
	unsigned        count;
};

ZixHash*
zix_hash_new(ZixHashFunc  hash_func,
             ZixEqualFunc key_equal_func)

{
	ZixHash* hash = (ZixHash*)malloc(sizeof(ZixHash));
	hash->hash_func      = hash_func;
	hash->key_equal_func = key_equal_func;
	hash->count          = 0;
	hash->n_buckets      = &sizes[0];
	hash->buckets        = (Entry**)malloc(*hash->n_buckets * sizeof(Entry*));
	memset(hash->buckets, 0, *hash->n_buckets * sizeof(Entry*));

	return hash;
}

void
zix_hash_free(ZixHash* hash)
{
	for (unsigned b = 0; b < *hash->n_buckets; ++b) {
		Entry* bucket = hash->buckets[b];
		for (Entry* e = bucket; e;) {
			Entry* next = e->next;
			free(e);
			e = next;
		}
	}

	free(hash->buckets);
	free(hash);
}

unsigned
zix_string_hash(const void* key)
{
	// Trusty old DJB hash
	const char* str = (const char*)key;
	unsigned    h   = 5381;
	for (const char* s = str; *s != '\0'; ++s) {
		h = (h << 5) + h + *s;  // h = h * 33 + c
	}
	return h;
}

bool
zix_string_equal(const void* a, const void* b)
{
	return !strcmp((const char*)a, (const char*)b);
}

static void
insert_entry(Entry** bucket,
             Entry*  entry)
{
	entry->next = *bucket;
	*bucket     = entry;
}

static ZixStatus
rehash(ZixHash* hash, unsigned new_n_buckets)
{
	Entry** new_buckets = (Entry**)malloc(new_n_buckets * sizeof(Entry*));
	if (!new_buckets) {
		return ZIX_STATUS_NO_MEM;
	}

	memset(new_buckets, 0, new_n_buckets * sizeof(Entry*));

	for (unsigned b = 0; b < *hash->n_buckets; ++b) {
		for (Entry* e = hash->buckets[b]; e;) {
			Entry* const   next = e->next;
			const unsigned h    = e->hash % new_n_buckets;
			insert_entry(&new_buckets[h], e);
			e = next;
		}
	}

	free(hash->buckets);
	hash->buckets = new_buckets;

	return ZIX_STATUS_SUCCESS;
}

static Entry*
find_entry(const ZixHash* hash,
           const void*    key,
           unsigned   h)
{
	for (Entry* e = hash->buckets[h]; e; e = e->next) {
		if (hash->key_equal_func(e->key, key)) {
			return e;
		}
	}

	return NULL;
}

void*
zix_hash_find(const ZixHash* hash, const void* key)
{
	const unsigned h     = hash->hash_func(key) % *hash->n_buckets;
	Entry* const   entry = find_entry(hash, key, h);
	return entry ? entry->data : 0;
}

ZixStatus
zix_hash_insert(ZixHash* hash, const void* key, void* data)
{
	unsigned h_nomod = hash->hash_func(key);
	unsigned h       = h_nomod % *hash->n_buckets;

	Entry* elem = find_entry(hash, key, h);
	if (elem) {
		assert(elem->hash == h_nomod);
		return ZIX_STATUS_EXISTS;
	}

	elem = (Entry*)malloc(sizeof(Entry));
	if (!elem) {
		return ZIX_STATUS_NO_MEM;
	}
	elem->key  = key;
	elem->data = data;
	elem->next = NULL;
	elem->hash = h_nomod;
	const unsigned next_n_buckets = *(hash->n_buckets + 1);
	if (next_n_buckets != 0 && (hash->count + 1) >= next_n_buckets) {
		if (!rehash(hash, next_n_buckets)) {
			h = h_nomod % *(++hash->n_buckets);
		}
	}

	insert_entry(&hash->buckets[h], elem);
	++hash->count;
	return ZIX_STATUS_SUCCESS;
}

ZixStatus
zix_hash_remove(ZixHash* hash, const void* key)
{
	unsigned h = hash->hash_func(key) % *hash->n_buckets;

	Entry** next_ptr = &hash->buckets[h];
	for (Entry* e = hash->buckets[h]; e; e = e->next) {
		if (hash->key_equal_func(e->key, key)) {
			*next_ptr = e->next;
			free(e);
			return ZIX_STATUS_SUCCESS;
		}
		next_ptr = &e->next;
	}

	if (hash->n_buckets != sizes) {
		const unsigned prev_n_buckets = *(hash->n_buckets - 1);
		if (hash->count - 1 <= prev_n_buckets) {
			if (!rehash(hash, prev_n_buckets)) {
				--hash->n_buckets;
			}
		}
	}

	--hash->count;
	return ZIX_STATUS_NOT_FOUND;
}

ZIX_API
void
zix_hash_foreach(const ZixHash* hash,
                 void (*f)(const void* key, void* value, void* user_data),
                 void* user_data)
{
	for (unsigned b = 0; b < *hash->n_buckets; ++b) {
		Entry* bucket = hash->buckets[b];
		for (Entry* e = bucket; e; e = e->next) {
			f(e->key, e->data, user_data);
		}
	}
}

