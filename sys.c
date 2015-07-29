/*
 * File: sys.c
 * Implements: javascript vm (duktape) with system extensions
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

static struct native_mod native_modules[] = {
#ifdef COMPSYS
	{ "#Sys1", sys1_load },
#endif
#ifdef COMPLMDB
	{ "#LMDB1", db1_load },
#endif
#ifdef COMPHTTP
	{ "#Http1", http1_load },
#endif
	{ "#Prg1", prg1_load },
	{ (void*)0, (void*)0 }
};

int main(int argc, char **argv)
{
	int rc;
	
	prg.argc = argc;
	prg.argv = argv;
	prg.name = argv[1];

	duk_context *ctx = duk_create_heap_default();

	duk_push_global_object(ctx);
	prg_push_modsearch(ctx);
	duk_pop(ctx);

	prg_parse_appfile(&prg);
	
	// push file
	duk_push_lstring(ctx, prg.main->buf, prg.main->size);
	duk_push_string(ctx, argv[1]);

	// execute file (compile + call)
	prg_register(&prg, native_modules);
	rc = duk_safe_call(ctx, prg_wrapped_compile_execute, 2 /*nargs*/, 1 /*nret*/);
	if (rc != DUK_EXEC_SUCCESS) {
		print_error(ctx, stderr);
		exit(2);
	}
	duk_pop(ctx);  /* pop eval result */
	
	duk_destroy_heap(ctx);
	
	return prg.status;
}
