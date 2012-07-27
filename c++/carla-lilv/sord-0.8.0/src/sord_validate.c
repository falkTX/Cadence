/*
  Copyright 2012 David Robillard <http://drobilla.net>

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

#define _BSD_SOURCE  // for realpath

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#    include <windows.h>
#endif

#include "serd/serd.h"
#include "sord/sord.h"
#include "sord_config.h"

#ifdef HAVE_PCRE
#    include <pcre.h>
#endif

#define USTR(s) ((const uint8_t*)s)

#define NS_foaf (const uint8_t*)"http://xmlns.com/foaf/0.1/"
#define NS_owl  (const uint8_t*)"http://www.w3.org/2002/07/owl#"
#define NS_rdf  (const uint8_t*)"http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_rdfs (const uint8_t*)"http://www.w3.org/2000/01/rdf-schema#"
#define NS_xsd  (const uint8_t*)"http://www.w3.org/2001/XMLSchema#"

typedef struct {
	SordNode* foaf_Document;
	SordNode* owl_AnnotationProperty;
	SordNode* owl_Class;
	SordNode* owl_DatatypeProperty;
	SordNode* owl_FunctionalProperty;
	SordNode* owl_InverseFunctionalProperty;
	SordNode* owl_ObjectProperty;
	SordNode* owl_OntologyProperty;
	SordNode* owl_Thing;
	SordNode* owl_equivalentClass;
	SordNode* rdf_Property;
	SordNode* rdf_type;
	SordNode* rdfs_Class;
	SordNode* rdfs_Literal;
	SordNode* rdfs_Resource;
	SordNode* rdfs_domain;
	SordNode* rdfs_range;
	SordNode* rdfs_subClassOf;
	SordNode* xsd_pattern;
	SordNode* xsd_string;
} URIs;

int  n_errors        = 0;
bool one_line_errors = false;

int
print_version()
{
	printf("sord_validate " SORD_VERSION
	       " <http://drobilla.net/software/sord>\n");
	printf("Copyright 2012 David Robillard <http://drobilla.net>.\n"
	       "License: <http://www.opensource.org/licenses/isc>\n"
	       "This is free software; you are free to change and redistribute it."
	       "\nThere is NO WARRANTY, to the extent permitted by law.\n");
	return 0;
}

int
print_usage(const char* name, bool error)
{
	FILE* const os = error ? stderr : stdout;
	fprintf(os, "Usage: %s [OPTION]... INPUT...\n", name);
	fprintf(os, "Validate RDF data\n\n");
	fprintf(os, "  -l  Print errors on a single line.\n");
	fprintf(os,
	        "Validate RDF data.  This is a simple validator which checks\n"
	        "that all used properties are actually defined.  It does not do\n"
	        "any fancy file retrieval, the files passed on the command line\n"
	        "are the only data that is read.  In other words, you must pass\n"
	        "the definition of all vocabularies used on the command line.\n");
	return error ? 1 : 0;
}

uint8_t*
absolute_path(const uint8_t* path)
{
#ifdef _WIN32
	char* out = (char*)malloc(MAX_PATH);
	GetFullPathName((const char*)path, MAX_PATH, out, NULL);
	return (uint8_t*)out;
#else
	return (uint8_t*)realpath((const char*)path, NULL);
#endif
}

void
error(const char* msg, const SordQuad quad)
{
	const char* sep = one_line_errors ? "\t" : "\n       ";
	++n_errors;
	fprintf(stderr, "error: %s:%s%s%s%s%s%s\n",
	        msg,
	        sep, (const char*)sord_node_get_string(quad[SORD_SUBJECT]),
	        sep, (const char*)sord_node_get_string(quad[SORD_PREDICATE]),
	        sep, (const char*)sord_node_get_string(quad[SORD_OBJECT]));
}

bool
is_subclass_of(SordModel*      model,
               const URIs*     uris,
               const SordNode* klass,
               const SordNode* super)
{
	if (!klass) {
		return false;
	} else if (sord_node_equals(klass, super) ||
	           sord_ask(model, klass, uris->owl_equivalentClass, super, NULL)) {
		return true;
	}

	SordIter* i = sord_search(model, klass, uris->rdfs_subClassOf, NULL, NULL);
	for (; !sord_iter_end(i); sord_iter_next(i)) {
		const SordNode* o = sord_iter_get_node(i, SORD_OBJECT);
		if (sord_node_equals(klass, o)) {
			continue;  // Class is explicitly subClassOf itself
		}
		if (is_subclass_of(model, uris, o, super)) {
			sord_iter_free(i);
			return true;
		}
	}
	sord_iter_free(i);

	return false;
}

bool
regexp_match(const char* pat, const char* str)
{
#ifdef HAVE_PCRE
	const char* error;
	int         erroffset;
	pcre*       re = pcre_compile(pat, PCRE_ANCHORED, &error, &erroffset, NULL);
	if (!re) {
		fprintf(stderr, "Error in regexp \"%s\" at offset %d (%s)\n",
		        pat, erroffset, error);
		return false;
	}

	int st = pcre_exec(re, NULL, str, strlen(str), 0, 0, NULL, 0);
	if (st < 0) {
		fprintf(stderr, "Error %d executing regexp \"%s\"\n", st, pat);
		return false;
	}
#endif  // HAVE_PCRE
	return true;
}

bool
literal_is_valid(SordModel*      model,
                 const URIs*     uris,
                 const SordNode* literal,
                 const SordNode* type)
{
	if (!type) {
		return true;
	}

	SordIter*       p       = sord_search(model, type, uris->xsd_pattern, 0, 0);
	const SordNode* pattern = sord_iter_get_node(p, SORD_OBJECT);
	if (!pattern) {
		fprintf(stderr, "warning: No pattern for datatype <%s>\n",
		        sord_node_get_string(type));
		return true;
	}
	if (regexp_match((const char*)sord_node_get_string(pattern),
	                 (const char*)sord_node_get_string(literal))) {
		return true;
	}
	fprintf(stderr, "Literal \"%s\" does not match <%s> pattern \"%s\"\n",
	        sord_node_get_string(literal),
	        sord_node_get_string(type),
	        sord_node_get_string(pattern));
	return false;
}

bool
check_type(SordModel*      model,
           URIs*           uris,
           const SordNode* node,
           const SordNode* type)
{
	if (sord_node_equals(type, uris->rdfs_Resource) ||
	    sord_node_equals(type, uris->owl_Thing)) {
		return true;
	}

	if (sord_node_get_type(node) == SORD_LITERAL) {
		if (sord_node_equals(type, uris->rdfs_Literal) ||
		    sord_node_equals(type, uris->xsd_string)) {
			return true;
		} else {
			const SordNode* datatype = sord_node_get_datatype(node);
			return is_subclass_of(model, uris, datatype, type) ||
				literal_is_valid(model, uris, node, type);
		}
	} else if (sord_node_get_type(node) == SORD_URI) {
		if (sord_node_equals(type, uris->foaf_Document)) {
			return true;  // Questionable...
		} else {
			SordIter* t = sord_search(model, node, uris->rdf_type, NULL, NULL);
			for (; !sord_iter_end(t); sord_iter_next(t)) {
				if (is_subclass_of(model, uris,
				                   sord_iter_get_node(t, SORD_OBJECT),
				                   type)) {
					sord_iter_free(t);
					return true;
				}
			}
			sord_iter_free(t);
			return false;
		}
	} else {
		return true;  // Blanks often lack explicit types, ignore
	}

	return false;
}

int
main(int argc, char** argv)
{
	if (argc < 2) {
		return print_usage(argv[0], true);
	}

	int a = 1;
	for (; a < argc && argv[a][0] == '-'; ++a) {
		if (argv[a][1] == 'l') {
			one_line_errors = true;
		} else {
			fprintf(stderr, "%s: Unknown option `%s'\n", argv[0], argv[a]);
			return print_usage(argv[0], true);
		}
	}

	SordWorld*  world  = sord_world_new();
	SordModel*  model  = sord_new(world, SORD_SPO|SORD_OPS, false);
	SerdEnv*    env    = serd_env_new(&SERD_NODE_NULL);
	SerdReader* reader = sord_new_reader(model, env, SERD_TURTLE, NULL);

	for (; a < argc; ++a) {
		const uint8_t* input   = (const uint8_t*)argv[a];
		uint8_t*       in_path = absolute_path(serd_uri_to_path(input));

		if (!in_path) {
			fprintf(stderr, "Skipping file %s\n", input);
			continue;
		}

		SerdURI  base_uri;
		SerdNode base_uri_node = serd_node_new_file_uri(
			in_path, NULL, &base_uri, false);

		serd_env_set_base_uri(env, &base_uri_node);
		const SerdStatus st = serd_reader_read_file(reader, in_path);
		if (st) {
			fprintf(stderr, "error reading %s: %s\n",
			        in_path, serd_strerror(st));
		}

		serd_node_free(&base_uri_node);
	}

#define URI(prefix, suffix) \
	uris.prefix##_##suffix = sord_new_uri(world, NS_##prefix #suffix)

	URIs uris;
	URI(foaf, Document);
	URI(owl, AnnotationProperty);
	URI(owl, Class);
	URI(owl, DatatypeProperty);
	URI(owl, FunctionalProperty);
	URI(owl, InverseFunctionalProperty);
	URI(owl, ObjectProperty);
	URI(owl, OntologyProperty);
	URI(owl, Thing);
	URI(owl, equivalentClass);
	URI(rdf, Property);
	URI(rdf, type);
	URI(rdfs, Class);
	URI(rdfs, Literal);
	URI(rdfs, Resource);
	URI(rdfs, domain);
	URI(rdfs, range);
	URI(rdfs, subClassOf);
	URI(xsd, pattern);
	URI(xsd, string);

#ifndef HAVE_PCRE
	fprintf(stderr, "warning: Built without PCRE, datatypes not checked.\n");
#endif

	SordIter* i = sord_begin(model);
	for (; !sord_iter_end(i); sord_iter_next(i)) {
		SordQuad quad;
		sord_iter_get(i, quad);

		const SordNode* subj = quad[SORD_SUBJECT];
		const SordNode* pred = quad[SORD_PREDICATE];
		const SordNode* obj  = quad[SORD_OBJECT];

		bool is_Property = sord_ask(
			model, pred, uris.rdf_type, uris.rdf_Property, 0);
		bool is_OntologyProperty = sord_ask(
			model, pred, uris.rdf_type, uris.owl_OntologyProperty, 0);
		bool is_ObjectProperty = sord_ask(
			model, pred, uris.rdf_type, uris.owl_ObjectProperty, 0);
		bool is_FunctionalProperty = sord_ask(
			model, pred, uris.rdf_type, uris.owl_FunctionalProperty, 0);
		bool is_InverseFunctionalProperty = sord_ask(
			model, pred, uris.rdf_type, uris.owl_InverseFunctionalProperty, 0);
		bool is_DatatypeProperty = sord_ask(
			model, pred, uris.rdf_type, uris.owl_DatatypeProperty, 0);
		bool is_AnnotationProperty = sord_ask(
			model, pred, uris.rdf_type, uris.owl_AnnotationProperty, 0);

		if (!is_Property && !is_OntologyProperty && !is_ObjectProperty &&
		    !is_FunctionalProperty && !is_InverseFunctionalProperty &&
		    !is_DatatypeProperty && !is_AnnotationProperty) {
			error("Use of undefined property", quad);
		}

		if (is_DatatypeProperty &&
		    sord_node_get_type(obj) != SORD_LITERAL) {
			error("Datatype property with non-literal value", quad);
		}

		if (is_ObjectProperty &&
		    sord_node_get_type(obj) == SORD_LITERAL) {
			error("Object property with literal value", quad);
		}

		if (is_FunctionalProperty &&
		    sord_count(model, subj, pred, NULL, NULL) > 1) {
			error("Functional property with several objects", quad);
		}

		if (is_InverseFunctionalProperty &&
		    sord_count(model, NULL, pred, obj, NULL) > 1) {
			error("Inverse functional property with several subjects", quad);
		}

		if (sord_node_equals(pred, uris.rdf_type) &&
		    !sord_ask(model, obj, uris.rdf_type, uris.rdfs_Class, NULL) &&
		    !sord_ask(model, obj, uris.rdf_type, uris.owl_Class, NULL)) {
			error("Type is not a rdfs:Class or owl:Class", quad);
		}

		if (sord_node_get_type(obj) == SORD_LITERAL &&
		    !literal_is_valid(model, &uris, obj, sord_node_get_datatype(obj))) {
			error("Literal does not match datatype", quad);
		}

		SordIter* r = sord_search(model, pred, uris.rdfs_range, NULL, NULL);
		if (r) {
			const SordNode* range = sord_iter_get_node(r, SORD_OBJECT);
			if (!check_type(model, &uris, obj, range)) {
				error("Object not in property range", quad);
				fprintf(stderr, "note: Range is <%s>\n",
				        sord_node_get_string(range));
			}
		}

		SordIter* d = sord_search(model, pred, uris.rdfs_domain, NULL, NULL);
		if (d) {
			const SordNode* domain = sord_iter_get_node(d, SORD_OBJECT);
			if (!check_type(model, &uris, subj, domain)) {
				error("Subject not in property domain", quad);
				fprintf(stderr, "note: Domain is <%s>\n",
				        sord_node_get_string(domain));
			}
		}
	}
	sord_iter_free(i);

	printf("Found %d errors among %d files\n", n_errors, argc - 1);
	return 0;
}
