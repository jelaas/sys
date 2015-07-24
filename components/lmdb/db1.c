#include <errno.h>

#include "duktape.h"
#include "db1.h"
#include "lmdb.h"

#define MAXENVS 16
#define MAXTXNS 32
#define MAXCURSORS 32
static struct {
	MDB_env *env[MAXENVS];	
	MDB_txn *txn[MAXTXNS];	
	MDB_cursor *cursor[MAXCURSORS];
} _db1;

//int mdb_cursor_get(MDB_cursor *cursor, MDB_val *key, MDB_val *data, MDB_cursor_op op)
static int db1_cursor_get(duk_context *ctx)
{
	int rc, cursor, op;
	struct MDB_val key;
	struct MDB_val data;
	
	duk_get_prop_string(ctx, 0, "cursor");
        cursor = duk_to_int(ctx, 4);
        duk_pop(ctx);

	duk_get_prop_string(ctx, 1, "key");
	key.mv_data = (void*) duk_to_string(ctx, 4);
	key.mv_size = strlen(key.mv_data)+1;
        duk_pop(ctx);
	
	data.mv_data = (void*) 0;
	data.mv_size = 0;
	
        op = duk_to_int(ctx, 3);
	if(op == MDB_SET_RANGE)
		key.mv_size--;
	
	rc = mdb_cursor_get(_db1.cursor[cursor], &key, &data, op);

	if(rc) goto done;

	if(op != MDB_SET) {
		if(key.mv_size && (*(char*)(key.mv_data+key.mv_size-1) == 0)) {
			duk_push_string(ctx, key.mv_data);
			duk_put_prop_string(ctx, 1, "key");
			duk_push_int(ctx, key.mv_size);
			duk_put_prop_string(ctx, 1, "mv_size");
		}		
	}

	/* If there was data and it is null-terminated */
	if(data.mv_size && (*(char*)(data.mv_data+data.mv_size-1) == 0)) {
		duk_push_string(ctx, data.mv_data);
		duk_put_prop_string(ctx, 2, "mv_data");
		duk_push_int(ctx, data.mv_size);
		duk_put_prop_string(ctx, 2, "mv_size");
	}
	
done:
	duk_push_int(ctx, rc);
	return 1;
}

//int mdb_cursor_open(MDB_txn *txn, MDB_dbi dbi, MDB_cursor **cursor)
static int db1_cursor_open(duk_context *ctx)
{
	int i, rc;
	int txn, dbi;
	int idx = 0;

	for(i=0;i<MAXCURSORS;i++) {
		if(!_db1.cursor[i]) break;
	}
	idx=i;

	duk_get_prop_string(ctx, 0, "txn");
        txn = duk_to_int(ctx, 3);
        duk_pop(ctx);

	duk_get_prop_string(ctx, 1, "dbi");
        dbi = duk_to_int(ctx, 3);
        duk_pop(ctx);
	
	rc = mdb_cursor_open(_db1.txn[txn], dbi, &_db1.cursor[idx]);

	duk_push_int(ctx, idx);
	duk_put_prop_string(ctx, 2, "cursor");

	duk_push_int(ctx, rc);
	return 1;
}

// int mdb_cursor_put(MDB_cursor *cursor, MDB_val *key, MDB_val *data, unsigned int flags)
static int db1_cursor_put(duk_context *ctx)
{
	int rc, cursor;
	unsigned int flags;
	struct MDB_val key;
	struct MDB_val data;
	
	duk_get_prop_string(ctx, 0, "cursor");
        cursor = duk_to_int(ctx, 4);
        duk_pop(ctx);

	key.mv_data = (void*) duk_to_string(ctx, 1);
	key.mv_size = strlen(key.mv_data)+1;

	data.mv_data = (void*) duk_to_string(ctx, 2);
	data.mv_size = strlen(data.mv_data)+1;

        flags = duk_to_uint(ctx, 3);
	if(!_db1.cursor[cursor]) {
		rc = ENOENT;
		goto done;
	}
	rc = mdb_cursor_put(_db1.cursor[cursor], &key, &data, flags);

done:
	duk_push_int(ctx, rc);
	return 1;	
}

// int mdb_cursor_del(MDB_cursor *cursor, unsigned int flags)
static int db1_cursor_del(duk_context *ctx)
{
	int rc, cursor;
	unsigned int flags;
	
	duk_get_prop_string(ctx, 0, "cursor");
        cursor = duk_to_int(ctx, 2);
        duk_pop(ctx);

        flags = duk_to_uint(ctx, 1);
	if(!_db1.cursor[cursor]) {
		rc = ENOENT;
		goto done;
	}
	rc = mdb_cursor_del(_db1.cursor[cursor], flags);

done:
	duk_push_int(ctx, rc);
	return 1;	
}

// int mdb_cursor_close(MDB_cursor *cursor)
static int db1_cursor_close(duk_context *ctx)
{
	int rc = 0, cursor;
	
	duk_get_prop_string(ctx, 0, "cursor");
        cursor = duk_to_int(ctx, 1);
        duk_pop(ctx);

	if(!_db1.cursor[cursor]) {
		rc = ENOENT;
		goto done;
	}
	mdb_cursor_close(_db1.cursor[cursor]);

done:
	duk_push_int(ctx, rc);
	return 1;	
}

//int mdb_dbi_open( MDB_txn *txn, const char *name, unsigned int flags, MDB_dbi *dbi) 
static int db1_dbi_open(duk_context *ctx)
{
	MDB_dbi dbi = -1;
	int rc, txn;
	const char *name = duk_to_string(ctx, 1);
	unsigned int flags = duk_to_uint(ctx, 2);

	duk_get_prop_string(ctx, 0, "txn");
	txn = duk_to_int(ctx, 4);
	duk_pop(ctx);
	
	rc = mdb_dbi_open(_db1.txn[txn], name, flags, &dbi);

	duk_push_int(ctx, dbi);
	duk_put_prop_string(ctx, 3, "dbi");

	duk_push_int(ctx, rc);
	return 1;
}

//int mdb_del( MDB_txn *txn, MDB_dbi dbi, MDB_val *key, MDB_val *data) 
static int db1_del(duk_context *ctx)
{
	int rc, txn, dbi;
	struct MDB_val key;
	struct MDB_val data;
	int passdata = 0;
	
	key.mv_data = (void*) duk_to_string(ctx, 2);
	key.mv_size = strlen(key.mv_data)+1;

	if(!duk_check_type(ctx, 1, DUK_TYPE_UNDEFINED)) {
		data.mv_data = (void*) duk_to_string(ctx, 3);
		data.mv_size = strlen(data.mv_data)+1;
		passdata = 1;
	}

	duk_get_prop_string(ctx, 0, "txn");
        txn = duk_to_int(ctx, 4);
        duk_pop(ctx);

	duk_get_prop_string(ctx, 1, "dbi");
        dbi = duk_to_int(ctx, 4);
        duk_pop(ctx);
	
	rc = mdb_del(_db1.txn[txn], dbi, &key, passdata?&data:(void*)0);

	duk_push_int(ctx, rc);
        return 1;	
}

// int mdb_drop ( MDB_txn *txn, MDB_dbi dbi, int del)
static int db1_drop(duk_context *ctx)
{
	int rc, txn, dbi;
	int del;
	
	duk_get_prop_string(ctx, 0, "txn");
        txn = duk_to_int(ctx, 3);
        duk_pop(ctx);

	duk_get_prop_string(ctx, 1, "dbi");
        dbi = duk_to_int(ctx, 3);
        duk_pop(ctx);
	
        del = duk_to_int(ctx, 2);
	
	rc = mdb_drop(_db1.txn[txn], dbi, del);

	duk_push_int(ctx, rc);
        return 1;	
}

// int mdb_env_create ( MDB_env **  env) 
static int db1_env_create(duk_context *ctx)
{
	int i, rc;
	int idx = 0;

	for(i=0;i<MAXENVS;i++) {
		if(!_db1.env[i]) break;
	}
	idx=i;
	
	rc = mdb_env_create(&_db1.env[idx]);

	duk_push_int(ctx, idx);
	duk_put_prop_string(ctx, 0, "env");

	duk_push_int(ctx, rc);
	return 1;
}

// int mdb_env_open( MDB_env *env, const char *path, unsigned int flags, mdb_mode_t mode)
static int db1_env_open(duk_context *ctx)
{
	int rc;
	int env;
	const char *path = duk_to_string(ctx, 1);
	unsigned int flags = duk_to_uint(ctx, 2);
	mdb_mode_t mode = duk_to_uint(ctx, 3);

	duk_get_prop_string(ctx, 0, "env");
	env = duk_to_int(ctx, 4);
	duk_pop(ctx);
	
	rc = mdb_env_open(_db1.env[env], path, flags, mode);
	
	duk_push_int(ctx, rc);
        return 1;
}


//int mdb_env_set_mapsize(MDB_env *env, size_t size)
// The default is 10485760 bytes, which is often too small!
// NOTE: differs from lmdb API: size here is given in megabytes
static int db1_env_set_mapsize(duk_context *ctx)
{
	int rc, env;
	size_t size;

	size = duk_to_int(ctx, 1);
	
	duk_get_prop_string(ctx, 0, "env");
	env = duk_to_int(ctx, 2);
        duk_pop(ctx);

	rc = mdb_env_set_mapsize(_db1.env[env], size * (1024*1024));
	
	duk_push_int(ctx, rc);
        return 1;
}

//int mdb_env_set_maxdbs(MDB_env *env, MDB_dbi dbs) 
static int db1_env_set_maxdbs(duk_context *ctx)
{
	MDB_dbi dbs = duk_to_uint(ctx, 1);
	int rc, env;

	duk_get_prop_string(ctx, 0, "env");
	env = duk_to_int(ctx, 2);
        duk_pop(ctx);

	rc = mdb_env_set_maxdbs(_db1.env[env], dbs);

	duk_push_int(ctx, rc);
        return 1;
}

// int mdb_get(MDB_txn *txn, MDB_dbi dbi, MDB_val *key, MDB_val *data)
static int db1_get(duk_context *ctx)
{
	int rc, txn, dbi;
	struct MDB_val key;
	struct MDB_val data;
	
	key.mv_data = (void*) duk_to_string(ctx, 2);
	key.mv_size = strlen(key.mv_data)+1;

	duk_get_prop_string(ctx, 0, "txn");
        txn = duk_to_int(ctx, 4);
        duk_pop(ctx);

	duk_get_prop_string(ctx, 1, "dbi");
        dbi = duk_to_int(ctx, 4);
        duk_pop(ctx);
	
	rc = mdb_get(_db1.txn[txn], dbi, &key, &data);

	if(rc) goto done;

	/* If there was data and it is null-terminated */
	if(data.mv_size && (*(char*)(data.mv_data+data.mv_size-1) == 0)) {
		duk_push_string(ctx, data.mv_data);
		duk_put_prop_string(ctx, 3, "mv_data");
		duk_push_int(ctx, data.mv_size);
		duk_put_prop_string(ctx, 3, "mv_size");
	}
	
done:
	duk_push_int(ctx, rc);
        return 1;	
}

// int mdb_put(MDB_txn *txn, MDB_dbi dbi, MDB_val *key, MDB_val *data, unsigned int flags)
static int db1_put(duk_context *ctx)
{
	int rc, txn, dbi;
	unsigned int flags = duk_to_uint(ctx, 4);
	struct MDB_val key;
	struct MDB_val data;
	
	key.mv_data = (void*) duk_to_string(ctx, 2);
	key.mv_size = strlen(key.mv_data)+1;

	data.mv_data = (void*) duk_to_string(ctx, 3);
	data.mv_size = strlen(data.mv_data)+1;

	duk_get_prop_string(ctx, 0, "txn");
        txn = duk_to_int(ctx, 5);
        duk_pop(ctx);

	duk_get_prop_string(ctx, 1, "dbi");
        dbi = duk_to_int(ctx, 5);
        duk_pop(ctx);
	
	rc = mdb_put(_db1.txn[txn], dbi, &key, &data, flags);
	
	duk_push_int(ctx, rc);
        return 1;
}

// void mdb_txn_abort ( MDB_txn *  txn) 
static int db1_txn_abort(duk_context *ctx)
{
	int rc=0, txn;

	duk_get_prop_string(ctx, 0, "txn");
        txn = duk_to_int(ctx, 1);
        duk_pop(ctx);
	
	mdb_txn_abort(_db1.txn[txn]);
	_db1.txn[txn] = (void*)0;

	duk_push_int(ctx, rc);
	return 1;
}

// char * mdb_strerror (int err)
static int db1_strerror(duk_context *ctx)
{
	char *str;
	int err;

        err = duk_to_int(ctx, 0);
	
	str = mdb_strerror(err);
	
	duk_push_string(ctx, str);
	return 1;
}

//int mdb_txn_begin ( MDB_env *env, MDB_txn *parent, unsigned int flags, MDB_txn **txn) 
static int db1_txn_begin(duk_context *ctx)
{
	int env;
	int parent = -1;
	unsigned int flags = duk_to_uint(ctx, 2);
	int i, idx = 0, rc = ENOMEM;

	duk_get_prop_string(ctx, 0, "env");
	env = duk_to_int(ctx, 4);
        duk_pop(ctx);
	
	if(!duk_check_type(ctx, 1, DUK_TYPE_UNDEFINED)) {
		duk_get_prop_string(ctx, 1, "txn");
		parent = duk_to_int(ctx, 4);
		duk_pop(ctx);
	}
	
	for(i=0;i<MAXTXNS;i++) {
		if(!_db1.txn[i]) break;
	}
	if(i >= MAXTXNS) goto done;
	
	idx=i;
	
	rc = mdb_txn_begin(_db1.env[env], parent >= 0?_db1.txn[parent]:(void*)0, flags, &_db1.txn[idx]);

	duk_push_int(ctx, idx);
	duk_put_prop_string(ctx, 3, "txn");
	
done:
	duk_push_int(ctx, rc);
	return 1;
}

// int mdb_txn_commit ( MDB_txn *  txn) 
static int db1_txn_commit(duk_context *ctx)
{
	int rc, txn;

	duk_get_prop_string(ctx, 0, "txn");
        txn = duk_to_int(ctx, 1);
        duk_pop(ctx);
	
	rc = mdb_txn_commit(_db1.txn[txn]);
	if(!rc) {
		_db1.txn[txn] = (void*)0;
	}
	duk_push_int(ctx, rc);
	return 1;
}

// int mdb_txn_renew ( MDB_txn *  txn) 
static int db1_txn_renew(duk_context *ctx)
{
	int rc, txn;

	duk_get_prop_string(ctx, 0, "txn");
        txn = duk_to_int(ctx, 1);
        duk_pop(ctx);
	
	rc = mdb_txn_renew(_db1.txn[txn]);
	
	duk_push_int(ctx, rc);
	return 1;
}

// void mdb_txn_reset ( MDB_txn *  txn) 
static int db1_txn_reset(duk_context *ctx)
{
	int rc=0, txn;

	duk_get_prop_string(ctx, 0, "txn");
        txn = duk_to_int(ctx, 1);
        duk_pop(ctx);
	
	mdb_txn_reset(_db1.txn[txn]);
	
	duk_push_int(ctx, rc);
	return 1;
}


static const duk_function_list_entry db1_funcs[] = {
	{ "cursor_del", db1_cursor_del, 2 },
	{ "cursor_get", db1_cursor_get, 4 },
	{ "cursor_open", db1_cursor_open, 3 },
	{ "cursor_put", db1_cursor_put, 4 },
	{ "cursor_close", db1_cursor_close, 1 },
	{ "dbi_open", db1_dbi_open, 4 },
	{ "del", db1_del, 4 },
	{ "drop", db1_drop, 3 },
	{ "env_create", db1_env_create, 1 },
	{ "env_open", db1_env_open, 4 },
	{ "env_set_mapsize", db1_env_set_mapsize, 2 },
	{ "env_set_maxdbs", db1_env_set_maxdbs, 2 },
	{ "get", db1_get, 4 },
	{ "put", db1_put, 5 },
	{ "strerror", db1_strerror, 1 },
	{ "txn_abort", db1_txn_abort, 1 },
	{ "txn_begin", db1_txn_begin, 4 },
	{ "txn_commit", db1_txn_commit, 1 },
	{ "txn_renew", db1_txn_renew, 1 },
	{ "txn_reset", db1_txn_reset, 1 },
	{ NULL, NULL, 0 }
};

static const duk_number_list_entry db1_consts[] = {
	{ "MDB_PANIC", MDB_PANIC },
	{ "MDB_MAP_RESIZED", MDB_MAP_RESIZED },
	{ "MDB_READERS_FULL", MDB_READERS_FULL },
	{ "MDB_VERSION_MISMATCH", MDB_VERSION_MISMATCH },
	{ "MDB_INVALID", MDB_INVALID },
	{ "MDB_NOTFOUND", MDB_NOTFOUND },
	{ "MDB_DBS_FULL", MDB_DBS_FULL },
	{ "MDB_KEYEXIST", MDB_KEYEXIST },
	{ "MDB_MAP_FULL", MDB_MAP_FULL },
	{ "MDB_TXN_FULL", MDB_TXN_FULL },
	{ "ENOENT", ENOENT },
	{ "EACCES", EACCES },
	{ "EAGAIN", EAGAIN },
	{ "ENOMEM", ENOMEM },
	{ "ENOSPC", ENOSPC },
	{ "EIO", EIO },
//	{ "",  },
	{ "MDB_NODUPDATA", MDB_NODUPDATA },
	{ "MDB_RESERVE", MDB_RESERVE },
	{ "MDB_APPEND", MDB_APPEND },
	{ "MDB_APPENDDUP", MDB_APPENDDUP },
	{ "MDB_NOOVERWRITE", MDB_NOOVERWRITE },
	{ "MDB_REVERSEKEY", MDB_REVERSEKEY },
	{ "MDB_DUPSORT", MDB_DUPSORT },
	{ "MDB_DUPFIXED", MDB_DUPFIXED },
	{ "MDB_INTEGERDUP", MDB_INTEGERDUP },
	{ "MDB_REVERSEDUP", MDB_REVERSEDUP },
	{ "MDB_CREATE", MDB_CREATE },
	{ "MDB_FIXEDMAP", MDB_FIXEDMAP },
	{ "MDB_NOSUBDIR", MDB_NOSUBDIR },
	{ "MDB_WRITEMAP", MDB_WRITEMAP },
	{ "MDB_NOMETASYNC", MDB_NOMETASYNC },
	{ "MDB_NOSYNC", MDB_NOSYNC },
	{ "MDB_MAPASYNC", MDB_MAPASYNC },
	{ "MDB_NOTLS", MDB_NOTLS },
	{ "MDB_NOLOCK", MDB_NOLOCK },
	{ "MDB_NORDAHEAD", MDB_NORDAHEAD },
	{ "MDB_NOMEMINIT", MDB_NOMEMINIT },
	{ "MDB_RESERVE", MDB_RESERVE },
	{ "MDB_RDONLY", MDB_RDONLY },
	{ "MDB_FIRST", MDB_FIRST },
	{ "MDB_FIRST_DUP", MDB_FIRST_DUP },
	{ "MDB_GET_BOTH", MDB_GET_BOTH },
	{ "MDB_GET_BOTH_RANGE", MDB_GET_BOTH_RANGE },
	{ "MDB_GET_CURRENT", MDB_GET_CURRENT },
	{ "MDB_GET_MULTIPLE", MDB_GET_MULTIPLE },
	{ "MDB_LAST", MDB_LAST },
	{ "MDB_LAST_DUP", MDB_LAST_DUP },
	{ "MDB_NEXT", MDB_NEXT },
	{ "MDB_NEXT_DUP", MDB_NEXT_DUP },
	{ "MDB_NEXT_MULTIPLE", MDB_NEXT_MULTIPLE },
	{ "MDB_NEXT_NODUP", MDB_NEXT_NODUP },
	{ "MDB_PREV", MDB_PREV },
	{ "MDB_PREV_DUP", MDB_PREV_DUP },
	{ "MDB_PREV_NODUP", MDB_PREV_NODUP },
	{ "MDB_SET", MDB_SET },
	{ "MDB_SET_KEY", MDB_SET_KEY },
	{ "MDB_SET_RANGE", MDB_SET_RANGE },
	{ "MDB_CURRENT", MDB_CURRENT },
	{ NULL, 0.0 }
};

int db1(duk_context *ctx)
{
	duk_put_function_list(ctx, -1, db1_funcs);
	duk_put_number_list(ctx, -1, db1_consts);
	return 0;
}

