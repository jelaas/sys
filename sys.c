/*
 * File: cgijs.c
 * Implements: javascript vm (duktape) with HTTP CGI extensions and lmdb support
 *
 * Copyright: Jens Låås, 2014 - 2015
 * Copyright license: According to GPL, see file COPYING in this directory.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <stdio.h>
#include "duktape.h"
#ifdef COMPSYS
#include "components/sys/sys1.h"
#endif
#ifdef COMPLMDB
#include "components/lmdb/db1.h"
#endif
#ifdef COMPHTTP
#include "components/http/http1.h"
#endif
#include "prg.h"

struct prg prg;

static int get_stack_raw(duk_context *ctx) {
	if (!duk_is_object(ctx, -1)) {
		return 1;
	}
	if (!duk_has_prop_string(ctx, -1, "stack")) {
		return 1;
	}

	/* XXX: should check here that object is an Error instance too,
	 * i.e. 'stack' is special.
	 */

	duk_get_prop_string(ctx, -1, "stack");  /* caller coerces */
	duk_remove(ctx, -2);
	return 1;
}

/* Print error to stderr and pop error. */
static void print_error(duk_context *ctx, FILE *f) {
	/* Print error objects with a stack trace specially.
	 * Note that getting the stack trace may throw an error
	 * so this also needs to be safe call wrapped.
	 */
	(void) duk_safe_call(ctx, get_stack_raw, 1 /*nargs*/, 1 /*nrets*/);
	fprintf(f, "%s\n", duk_safe_to_string(ctx, -1));
	fflush(f);
	duk_pop(ctx);
}

int main(int argc, char **argv)
{
	int i, rc;
	
	prg.argc = argc;
	prg.argv = argv;
	prg.name = argv[1];

	duk_context *ctx = duk_create_heap_default();

	duk_push_global_object(ctx);

#ifdef COMPSYS
	duk_push_object(ctx);  /* -> [ ... global obj ] */
	sys1(ctx);
	for(i=1;i<argc;i++) {
		duk_push_string(ctx, argv[i]);
		duk_put_prop_index(ctx, -2, i-1);
	}
	duk_push_number(ctx, argc-1);
	duk_put_prop_string(ctx, -2, "argc");
	duk_put_prop_string(ctx, -2, "Sys1");  /* -> [ ... global ] */
#endif
	
#ifdef COMPLMDB
	duk_push_object(ctx);  /* -> [ ... global obj ] */
	db1(ctx);
	duk_put_prop_string(ctx, -2, "LMDB1");  /* -> [ ... global ] */
#endif

#ifdef COMPHTTP
	duk_push_object(ctx);  /* -> [ ... global obj ] */
	http1(ctx);
	duk_put_prop_string(ctx, -2, "Http1");  /* -> [ ... global ] */
#endif

	duk_push_object(ctx);  /* -> [ ... global obj ] */
	prg1(ctx);
	duk_put_prop_string(ctx, -2, "Prg1");  /* -> [ ... global ] */

	prg_push_modsearch(ctx);
	
	duk_pop(ctx);

	prg_parse_appfile(&prg);
	
	// push file
	duk_push_lstring(ctx, prg.main->buf, prg.main->size);
	duk_push_string(ctx, argv[1]);

	// execute file (compile + call)
	prg_register(&prg);
	rc = duk_safe_call(ctx, prg_wrapped_compile_execute, 2 /*nargs*/, 1 /*nret*/);
	if (rc != DUK_EXEC_SUCCESS) {
		print_error(ctx, stderr);
		exit(2);
	}
	duk_pop(ctx);  /* pop eval result */
	
	duk_destroy_heap(ctx);
	
	return prg.status;
}
