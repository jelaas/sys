/*
 * File: prg.c
 * Implements: parsing javascript program aggregate
 *
 * Copyright: Jens Låås, 2015
 * Copyright license: According to GPL, see file COPYING in this directory.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "duktape.h"
#include "prg.h"

static struct prg *prg_call;
static struct native_mod *prg_native_modules;

int prg_register(struct prg *prg, struct native_mod *native_modules)
{
	prg_call = prg;
	prg_native_modules = native_modules;
	return 0;
}

struct mod *prg_storage_byname(const char *name)
{
	struct mod *mod;
	for(mod = prg_call->modules; mod; mod=mod->next) {
		if(!strcmp(mod->fullname, name))
			return mod;
	}
	return (void*)0;
}

struct mod *prg_storage_byid(int id)
{
	struct mod *mod;
	for(mod = prg_call->modules; mod; mod=mod->next) {
		if(mod->id == id)
			return mod;
	}
	return (void*)0;
}

static struct mod *mod_new()
{
	struct mod *m;
	m = malloc(sizeof(struct mod));
	if(m) memset(m, 0, sizeof(struct mod));
	return m;
}

int prg_wrapped_compile_execute(duk_context *ctx) {
	int ret;
	struct prg *prg = prg_call;
      
	duk_compile(ctx, 0);
	close(prg->fd);
	munmap(prg->buf, prg->size);
	
	duk_push_global_object(ctx);  /* 'this' binding */
	duk_call_method(ctx, 0);
	prg->status = duk_to_int(ctx, -1);
	duk_pop(ctx); // pop return value
	
	// Check if global property 'main' exists
	duk_push_global_object(ctx);
	ret = duk_get_prop_string(ctx, -1, "main");
	duk_remove(ctx, -2); // remove global object
	
	// If main exists we call it
	if(ret && duk_get_type(ctx, -1) != DUK_TYPE_UNDEFINED) {
		int i;
		
		duk_push_global_object(ctx);  /* 'this' binding */
		for(i=1;i<prg->argc;i++) {
			duk_push_string(ctx, prg->argv[i]);
		}
		duk_call_method(ctx, prg->argc-1);
		prg->status = duk_to_int(ctx, -1);
	}
	duk_pop(ctx);

	return 0; // no values returned (0)
}

static int x(duk_context *ctx)
{
	duk_push_string(ctx, "hello from x");
	return 1;
}

static const duk_function_list_entry x_funcs[] = {
	{ "x", x, 0 /* fd */ },
	{ NULL, NULL, 0 }
};


/* Duktape.modSearch = function (id, require, exports, module) */
static int modSearch(duk_context *ctx)
{
	struct mod *mod;
	struct prg *prg;
	int i;
	
	prg = prg_call;

	const char *id = duk_to_string(ctx, 0);

	/*
	 * To support the native module case, the module search function can also return undefined
	 * (or any non-string value), in which case Duktape will assume that the module was found
	 * but has no Ecmascript source to execute. Symbols written to exports in the module search
	 * function are the only symbols provided by the module.
	 */
	for(i=0;;i++) {
		if(!prg_native_modules[i].name) break;
		if(!strcmp(id, prg_native_modules[i].name)) {
			prg_native_modules[i].fn(ctx, 2, prg);
			duk_push_undefined(ctx);
			return 1;
		}
	}

	if(!strcmp(id, "x")) {
		duk_put_function_list(ctx, 2, x_funcs);
		duk_push_undefined(ctx);
		return 1;
	}

	/*
	 * If a module is found, the module search function can return a string providing the source
	 * code for the module. Duktape will then take care of compiling and executing the module code
	 * so that module symbols get registered into the exports object.
	 */
	for(mod=prg->modules;mod;mod=mod->next) {
		if(!strcmp(mod->name, id)) {
			duk_push_lstring(ctx, mod->buf, mod->size);
			return 1;
		}
	}
	duk_error(ctx, DUK_ERR_ERROR, "failed to find module '%s'", id);
	return DUK_RET_ERROR;
}


int prg_push_modsearch(duk_context *ctx)
{
	duk_get_prop_string(ctx, -1, "Duktape");
	duk_push_c_function(ctx, modSearch, 4);
	duk_put_prop_string(ctx, -2, "modSearch");
	duk_pop(ctx); // pop Duktape
	return 0;
}

int prg_parse_appfile(struct prg *prg)
{
	char *p, *endp, *start, *m, *mainstart;
	off_t offset, pa_offset;
	struct mod *mod;
	int id = 1000;
	
	// read file prg->name
	prg->fd = open(prg->name, O_RDONLY);
	if(prg->fd == -1) {
		exit(1);
	}
	prg->size = lseek(prg->fd, 0, SEEK_END);
	prg->buf = mmap((void*)0, prg->size, PROT_READ, MAP_PRIVATE, prg->fd, 0);
	
	/* parse file header
	 */
	p = prg->buf;
	endp = prg->buf + prg->size;
	if(*p == '#') {
		while((p < endp) && (*p != '\n')) p++;
		if(p >= endp) {
			fprintf(stderr, "EOF\n");
			exit(1);
		}
		p++;
	}
	mainstart = p;
	mod = mod_new();
	mod->id = id++;
	for(start=p;p < endp;p++) {
		if(*p == '\n') {
			/* is this a module specification? */
			for(m = start; *m == ' '; m++);
			if((*m >= '0') && (*m <= '9')) {
				mod->size = strtoul(m, &m, 10);
				if(!m) break;
				if(*m != ' ') break;
				m++;
				mod->name = strndup(m, p-m);
				mod->fullname = strdup(mod->name);
				if(!strcmp(mod->name, "total"))
					break;
				mod->next = prg->modules;
				prg->modules = mod;
				mod = mod_new();
				mod->id = id++;
			} else
				break;
			start = p+1;
		}
	}
	offset = prg->size;
	for(mod = prg->modules; mod; mod=mod->next) {
		offset -= mod->size;
		pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);
		mod->buf = mmap((void*)0, mod->size + offset - pa_offset,
				PROT_READ, MAP_PRIVATE, prg->fd, pa_offset);
		if(mod->buf == MAP_FAILED) {
			fprintf(stderr, "mmap failed\n");
			exit(1);
		}
		mod->buf += (offset - pa_offset);
	}
	for(mod = prg->modules; mod; mod=mod->next) {
		char *p;
		if((p=strrchr(mod->name, '.'))) {
			if(!strcmp(p, ".js"))
				*p=0;
		}
	}
	for(mod = prg->modules; mod; mod=mod->next) {
		if(!strcmp(mod->name, "main")) {
			prg->main = mod;
		} else {
			char *p;
			if((p=strrchr(mod->name, '/'))) {
				if(!strcmp(p+1, "main")) {
					prg->main = mod;
				}
			}
		}
	}
	if(!prg->modules) {
		prg->main = mod_new();
		prg->main->id = id++;
		prg->main->buf = mainstart;
		prg->main->size = prg->size - (mainstart - prg->buf);
		prg->main->name = "main";
		prg->main->fullname = "main";
	}
	if(!prg->main) {
		fprintf(stderr, "no main module\n");
		exit(1);
	}
	return 0;
}

static int prg1_storage(duk_context *ctx)
{
	const char *name = duk_to_string(ctx, 0);
	struct mod *mod;

	mod = prg_storage_byname(name);

	if(mod) {
		duk_push_object(ctx);
		duk_push_int(ctx, mod->id);
		duk_put_prop_string(ctx, -2, "id");
		duk_push_int(ctx, mod->size);
		duk_put_prop_string(ctx, -2, "size");
		duk_push_string(ctx, mod->name);
		duk_put_prop_string(ctx, -2, "name");
		duk_push_string(ctx, mod->fullname);
		duk_put_prop_string(ctx, -2, "fullname");
	} else {
		duk_push_undefined(ctx);
	}
	
	return 1;
}

static int prg1_storage_write(duk_context *ctx)
{
	struct mod *mod;
	int rc = -1;
	int id, fd;
	size_t offset, len;

	fd = duk_to_int(ctx, 0);
	id = duk_to_int(ctx, 1);
	offset = duk_to_int(ctx, 2);
	len = duk_to_int(ctx, 3);
	
	mod = prg_storage_byid(id);
	
	if(mod) {
		rc = write(fd, mod->buf + offset, len);
	}
	
	duk_push_int(ctx, rc);
	return 1;
}

static int prg1_storage_buffer(duk_context *ctx)
{
	struct mod *mod;
	int id;
	size_t offset, len;
	char *buf;

	id = duk_to_int(ctx, 0);
	offset = duk_to_int(ctx, 1);
	len = duk_to_int(ctx, 2);
	
	mod = prg_storage_byid(id);
	
	if(mod) {
		buf = duk_push_fixed_buffer(ctx, len);
		memcpy(buf, mod->buf + offset, len);
	} else {
		duk_push_undefined(ctx);
	}
	return 1;
}


static const duk_function_list_entry prg1_funcs[] = {
	{ "storage", prg1_storage, 1 /* name */ },
	{ "storage_write", prg1_storage_write, 4 /* fd, id, offset, len */ },
	{ "storage_buffer", prg1_storage_buffer, 4 /* id, offset, len */ },
	{ NULL, NULL, 0 }
};

static const duk_number_list_entry prg1_consts[] = {
	{ NULL, 0.0 }
};

int prg1_load(duk_context *ctx, int n, struct prg *prg)
{
	duk_put_function_list(ctx, n, prg1_funcs);
	duk_put_number_list(ctx, n, prg1_consts);
	return 0;
}
