"use strict";
var Db=LMDB1;
var Sys=Sys1;

var env = {}, rc;
rc = Db.env_create(env);
Sys.dprint(1, "rc = "+rc+"\n");
Sys.dprint(1, "env = "+env.env+"\n");

rc = Db.env_set_maxdbs(env, 8);
Sys.dprint(1, "env_set_maxdbs rc = "+rc+"\n");

rc = Db.env_open(env, "./test", 0, parseInt("0644", 8));
Sys.dprint(1, "rc = "+rc+"\n");

var txn = {};
rc = Db.txn_begin(env, undefined, Db.MDB_RDONLY, txn);
Sys.dprint(1, "txn_begin rc = "+rc+"\n");
Sys.dprint(1, "txn = "+txn.txn+"\n");

var dbi = {};
rc = Db.dbi_open(txn, "testdb", Db.MDB_CREATE, dbi)
Sys.dprint(1, "dbi_open rc = "+rc+"\n");
Sys.dprint(1, "dbi = "+dbi.dbi+"\n");

// rc = Db.get(txn, dbi, "trolleri", data);
var data = {};
rc = Db.get(txn, dbi, "trolleri", data);
Sys.dprint(1, "get rc = "+rc+"\n");
Sys.dprint(1, "data = " + JSON.stringify(data)+"\n");

rc = Db.txn_commit(txn);
Sys.dprint(1, "txn_commit rc = "+rc+"\n");
