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

// rc = Db.del(txn, dbi, "trolleri", data);
var data = {};
rc = Db.del(txn, dbi, "trolleri", undefined);
Sys.dprint(1, "del rc = "+rc+"\n");

rc = Db.txn_commit(txn);
Sys.dprint(1, "txn_commit rc = "+rc+"\n");
