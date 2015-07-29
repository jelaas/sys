"use strict";
var Db=require('#LMDB1');
var Sys=require('#Sys1');

var env = {}, rc;
rc = Db.env_create(env);
Sys.dprint(1, "rc = "+rc+"\n");
Sys.dprint(1, "env = "+env.env+"\n");

rc = Db.env_set_maxdbs(env, 8);
Sys.dprint(1, "env_set_maxdbs rc = "+rc+"\n");

rc = Db.env_open(env, "./test", 0, parseInt("0644", 8));
Sys.dprint(1, "rc = "+rc+"\n");

var txn = {};
rc = Db.txn_begin(env, undefined, 0, txn);
Sys.dprint(1, "txn_begin rc = "+rc+"\n");
Sys.dprint(1, "txn = "+txn.txn+"\n");

var dbi = {};
rc = Db.dbi_open(txn, "testdb", Db.MDB_CREATE, dbi)
Sys.dprint(1, "dbi_open rc = "+rc+"\n");
Sys.dprint(1, "dbi = "+dbi.dbi+"\n");

var cursor = {};
rc = Db.cursor_open(txn, dbi, cursor);
Sys.dprint(1, "cursor_open rc = "+rc+"\n");
Sys.dprint(1, "cursor = "+cursor.cursor+"\n");

var data = {};
var key = { key: "" };
rc = Db.cursor_get(cursor, key, data, LMDB1.MDB_FIRST);
Sys.dprint(1, "cursor_get rc = "+rc+"\n");
Sys.dprint(1, "key = " + JSON.stringify(key)+"\n");
Sys.dprint(1, "data = " + JSON.stringify(data)+"\n");

while(!rc) {
    rc = Db.cursor_get(cursor, key, data, LMDB1.MDB_NEXT);
    Sys.dprint(1, "cursor_get rc = "+rc+"\n");
    if(!rc) {
	Sys.dprint(1, "key = " + JSON.stringify(key)+"\n");
	Sys.dprint(1, "data = " + JSON.stringify(data)+"\n");
    }
}

rc = Db.cursor_put(cursor, "hoppsan", "hejsan", LMDB1.MDB_NOOVERWRITE);
Sys.dprint(1, "cursor_put rc = "+rc+"\n");

rc = Db.txn_commit(txn);
Sys.dprint(1, "txn_commit rc = "+rc+"\n");