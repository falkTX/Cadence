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

#ifndef ZIX_HASH_H
#define ZIX_HASH_H

#include "zix/common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ZixHashImpl ZixHash;

/**
   Function for computing the hash of an element.
*/
typedef unsigned (*ZixHashFunc)(const void* key);

ZIX_API
ZixHash*
zix_hash_new(ZixHashFunc  hash_func,
             ZixEqualFunc key_equal_func);

ZIX_API
void
zix_hash_free(ZixHash* hash);

ZIX_API
unsigned
zix_string_hash(const void* key);

ZIX_API
bool
zix_string_equal(const void* a, const void* b);

ZIX_API
ZixStatus
zix_hash_insert(ZixHash*    hash,
                const void* key,
                void*       data);

ZIX_API
ZixStatus
zix_hash_remove(ZixHash* hash, const void* key);

ZIX_API
void*
zix_hash_find(const ZixHash* hash,
              const void*    key);

ZIX_API
void
zix_hash_foreach(const ZixHash* hash,
                 void (*f)(const void* key, void* value, void* user_data),
                 void* user_data);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* ZIX_HASH_H */
