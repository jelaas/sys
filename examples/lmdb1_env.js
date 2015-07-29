"use strict";
var Db=LMDB1;
var Sys=Sys1;

var env = {}, rc;
rc = Db.env_create(env);
Sys.dprint(1, "env_create rc = "+rc+"\n");
Sys.dprint(1, "env = "+env.env+"\n");
rc = Db.env_set_maxdbs(env, 8);
Sys.dprint(1, "env_set_maxdbs rc = "+rc+"\n");

// Set maximum database size to 64 GB
rc = Db.env_set_mapsize(env, 64 * 1024 );
Sys.dprint(1, "env_set_mapsize rc = "+rc+"\n");

rc = Db.env_open(env, "./test", 0, parseInt("0644", 8));
Sys.dprint(1, "env_open rc = "+rc+"\n");
if(rc) { Sys.dprint(1, "error: "+Db.strerror(rc)+"\n"); }
