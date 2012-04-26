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

#ifndef SORD_SORD_INTERNAL_H
#define SORD_SORD_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

#include "sord/sord.h"

/** Node */
struct SordNodeImpl {
	const char* lang;         ///< Literal language (interned string)
	SordNode*   datatype;     ///< Literal data type (ID of a URI node, or 0)
	size_t      refs;         ///< Reference count (# of containing quads)
	size_t      refs_as_obj;  ///< References as a quad object
	SerdNode    node;         ///< Serd node
};

const char*
sord_intern_lang(SordWorld* world, const char* lang);

#endif /* SORD_SORD_INTERNAL_H */
