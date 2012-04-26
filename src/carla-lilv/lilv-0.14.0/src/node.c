/*
  Copyright 2007-2011 David Robillard <http://drobilla.net>

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

#include "lilv_internal.h"

static void
lilv_node_set_numerics_from_string(LilvNode* val, size_t len)
{
	char* endptr;

	switch (val->type) {
	case LILV_VALUE_URI:
	case LILV_VALUE_BLANK:
	case LILV_VALUE_STRING:
		break;
	case LILV_VALUE_INT:
		val->val.int_val = strtol(val->str_val, &endptr, 10);
		break;
	case LILV_VALUE_FLOAT:
		val->val.float_val = serd_strtod(val->str_val, &endptr);
		break;
	case LILV_VALUE_BOOL:
		val->val.bool_val = (!strcmp(val->str_val, "true"));
		break;
	case LILV_VALUE_BLOB:
		val->val.blob_val.buf = serd_base64_decode(
			(const uint8_t*)val->str_val, len, &val->val.blob_val.size);
	}
}

/** Note that if @a type is numeric or boolean, the returned value is corrupt
 * until lilv_node_set_numerics_from_string is called.  It is not
 * automatically called from here to avoid overhead and imprecision when the
 * exact string value is known.
 */
LilvNode*
lilv_node_new(LilvWorld* world, LilvNodeType type, const char* str)
{
	LilvNode* val = (LilvNode*)malloc(sizeof(LilvNode));
	val->world = world;
	val->type  = type;

	switch (type) {
	case LILV_VALUE_URI:
		val->val.uri_val = sord_new_uri(world->world, (const uint8_t*)str);
		val->str_val = (char*)sord_node_get_string(val->val.uri_val);
		break;
	case LILV_VALUE_BLANK:
		val->val.uri_val = sord_new_blank(world->world, (const uint8_t*)str);
		val->str_val = (char*)sord_node_get_string(val->val.uri_val);
	case LILV_VALUE_STRING:
	case LILV_VALUE_INT:
	case LILV_VALUE_FLOAT:
	case LILV_VALUE_BOOL:
	case LILV_VALUE_BLOB:
		val->str_val = lilv_strdup(str);
		break;
	}

	return val;
}

/** Create a new LilvNode from @a node, or return NULL if impossible */
LilvNode*
lilv_node_new_from_node(LilvWorld* world, const SordNode* node)
{
	LilvNode*    result       = NULL;
	SordNode*    datatype_uri = NULL;
	LilvNodeType type         = LILV_VALUE_STRING;
	size_t       len          = 0;

	switch (sord_node_get_type(node)) {
	case SORD_URI:
		result              = (LilvNode*)malloc(sizeof(LilvNode));
		result->world       = (LilvWorld*)world;
		result->type        = LILV_VALUE_URI;
		result->val.uri_val = sord_node_copy(node);
		result->str_val     = (char*)sord_node_get_string(result->val.uri_val);
		break;
	case SORD_BLANK:
		result              = (LilvNode*)malloc(sizeof(LilvNode));
		result->world       = (LilvWorld*)world;
		result->type        = LILV_VALUE_BLANK;
		result->val.uri_val = sord_node_copy(node);
		result->str_val     = (char*)sord_node_get_string(result->val.uri_val);
		break;
	case SORD_LITERAL:
		datatype_uri = sord_node_get_datatype(node);
		if (datatype_uri) {
			if (sord_node_equals(datatype_uri, world->uris.xsd_boolean))
				type = LILV_VALUE_BOOL;
			else if (sord_node_equals(datatype_uri, world->uris.xsd_decimal)
			         || sord_node_equals(datatype_uri, world->uris.xsd_double))
				type = LILV_VALUE_FLOAT;
			else if (sord_node_equals(datatype_uri, world->uris.xsd_integer))
				type = LILV_VALUE_INT;
			else if (sord_node_equals(datatype_uri,
			                          world->uris.xsd_base64Binary))
				type = LILV_VALUE_BLOB;
			else
				LILV_ERRORF("Unknown datatype `%s'\n",
				            sord_node_get_string(datatype_uri));
		}
		result = lilv_node_new(
			world, type, (const char*)sord_node_get_string_counted(node, &len));
		lilv_node_set_numerics_from_string(result, len);
		break;
	default:
		assert(false);
	}

	return result;
}

LILV_API
LilvNode*
lilv_new_uri(LilvWorld* world, const char* uri)
{
	return lilv_node_new(world, LILV_VALUE_URI, uri);
}

LILV_API
LilvNode*
lilv_new_string(LilvWorld* world, const char* str)
{
	return lilv_node_new(world, LILV_VALUE_STRING, str);
}

LILV_API
LilvNode*
lilv_new_int(LilvWorld* world, int val)
{
	char str[32];
	snprintf(str, sizeof(str), "%d", val);
	LilvNode* ret = lilv_node_new(world, LILV_VALUE_INT, str);
	ret->val.int_val = val;
	return ret;
}

LILV_API
LilvNode*
lilv_new_float(LilvWorld* world, float val)
{
	char str[32];
	snprintf(str, sizeof(str), "%f", val);
	LilvNode* ret = lilv_node_new(world, LILV_VALUE_FLOAT, str);
	ret->val.float_val = val;
	return ret;
}

LILV_API
LilvNode*
lilv_new_bool(LilvWorld* world, bool val)
{
	LilvNode* ret = lilv_node_new(world, LILV_VALUE_BOOL,
	                              val ? "true" : "false");
	ret->val.bool_val = val;
	return ret;
}

LILV_API
LilvNode*
lilv_node_duplicate(const LilvNode* val)
{
	if (val == NULL)
		return NULL;

	LilvNode* result = (LilvNode*)malloc(sizeof(LilvNode));
	result->world = val->world;
	result->type = val->type;

	switch (val->type) {
	case LILV_VALUE_URI:
	case LILV_VALUE_BLANK:
		result->val.uri_val = sord_node_copy(val->val.uri_val);
		result->str_val = (char*)sord_node_get_string(result->val.uri_val);
		break;
	default:
		result->str_val = lilv_strdup(val->str_val);
		result->val = val->val;
	}

	return result;
}

LILV_API
void
lilv_node_free(LilvNode* val)
{
	if (val) {
		switch (val->type) {
		case LILV_VALUE_URI:
		case LILV_VALUE_BLANK:
			sord_node_free(val->world->world, val->val.uri_val);
			break;
		default:
			free(val->str_val);
		}
		free(val);
	}
}

LILV_API
bool
lilv_node_equals(const LilvNode* value, const LilvNode* other)
{
	if (value == NULL && other == NULL)
		return true;
	else if (value == NULL || other == NULL)
		return false;
	else if (value->type != other->type)
		return false;

	switch (value->type) {
	case LILV_VALUE_URI:
		return sord_node_equals(value->val.uri_val, other->val.uri_val);
	case LILV_VALUE_BLANK:
	case LILV_VALUE_STRING:
		return !strcmp(value->str_val, other->str_val);
	case LILV_VALUE_INT:
		return (value->val.int_val == other->val.int_val);
	case LILV_VALUE_FLOAT:
		return (value->val.float_val == other->val.float_val);
	case LILV_VALUE_BOOL:
		return (value->val.bool_val == other->val.bool_val);
	case LILV_VALUE_BLOB:
		return (value->val.blob_val.size == other->val.blob_val.size)
			&& !memcmp(value->val.blob_val.buf, other->val.blob_val.buf,
			           value->val.blob_val.size);
	}

	return false; /* shouldn't get here */
}

LILV_API
char*
lilv_node_get_turtle_token(const LilvNode* value)
{
	size_t   len    = 0;
	char*    result = NULL;
	SerdNode node;

	switch (value->type) {
	case LILV_VALUE_URI:
		len = strlen(value->str_val) + 3;
		result = (char*)calloc(len, 1);
		snprintf(result, len, "<%s>", value->str_val);
		break;
	case LILV_VALUE_BLANK:
		len = strlen(value->str_val) + 3;
		result = (char*)calloc(len, 1);
		snprintf(result, len, "_:%s", value->str_val);
		break;
	case LILV_VALUE_STRING:
	case LILV_VALUE_BOOL:
	case LILV_VALUE_BLOB:
		result = lilv_strdup(value->str_val);
		break;
	case LILV_VALUE_INT:
		node   = serd_node_new_integer(value->val.int_val);
		result = (char*)node.buf;
		break;
	case LILV_VALUE_FLOAT:
		node = serd_node_new_decimal(value->val.float_val, 8);
		result = (char*)node.buf;
		break;
	}

	return result;
}

LILV_API
bool
lilv_node_is_uri(const LilvNode* value)
{
	return (value && value->type == LILV_VALUE_URI);
}

LILV_API
const char*
lilv_node_as_uri(const LilvNode* value)
{
	assert(lilv_node_is_uri(value));
	return value->str_val;
}

const SordNode*
lilv_node_as_node(const LilvNode* value)
{
	assert(lilv_node_is_uri(value));
	return value->val.uri_val;
}

LILV_API
bool
lilv_node_is_blank(const LilvNode* value)
{
	return (value && value->type == LILV_VALUE_BLANK);
}

LILV_API
const char*
lilv_node_as_blank(const LilvNode* value)
{
	assert(lilv_node_is_blank(value));
	return value->str_val;
}

LILV_API
bool
lilv_node_is_literal(const LilvNode* value)
{
	if (!value)
		return false;

	switch (value->type) {
	case LILV_VALUE_STRING:
	case LILV_VALUE_INT:
	case LILV_VALUE_FLOAT:
		return true;
	default:
		return false;
	}
}

LILV_API
bool
lilv_node_is_string(const LilvNode* value)
{
	return (value && value->type == LILV_VALUE_STRING);
}

LILV_API
const char*
lilv_node_as_string(const LilvNode* value)
{
	return value ? value->str_val : NULL;
}

LILV_API
bool
lilv_node_is_int(const LilvNode* value)
{
	return (value && value->type == LILV_VALUE_INT);
}

LILV_API
int
lilv_node_as_int(const LilvNode* value)
{
	assert(value);
	assert(lilv_node_is_int(value));
	return value->val.int_val;
}

LILV_API
bool
lilv_node_is_float(const LilvNode* value)
{
	return (value && value->type == LILV_VALUE_FLOAT);
}

LILV_API
float
lilv_node_as_float(const LilvNode* value)
{
	assert(lilv_node_is_float(value) || lilv_node_is_int(value));
	if (lilv_node_is_float(value)) {
		return value->val.float_val;
	} else {  // lilv_node_is_int(value)
		return (float)value->val.int_val;
	}
}

LILV_API
bool
lilv_node_is_bool(const LilvNode* value)
{
	return (value && value->type == LILV_VALUE_BOOL);
}

LILV_API
bool
lilv_node_as_bool(const LilvNode* value)
{
	assert(value);
	assert(lilv_node_is_bool(value));
	return value->val.bool_val;
}
