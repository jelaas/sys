sys
=====

javascript vm with extensions.

Uses http://duktape.org/ for javascript vm.

Current extensions are:
* Sys
  System extensions. Mainly direct access to kernel systemcalls.
* LMDB
  The Lightning Memory-Mapped Database Manager.
* HTTP
  Working with HTTP.

Examples
========

cgi.js
=========

```
"use strict";

var Sys=require('#Sys1');
var CGI=require('#Http1').CGI;

CGI.init();
CGI.header("Content-Type: text/html");
CGI.header("x-js: true");
CGI.header("Content-type: text/plain; charset=utf-8");
print("content");
CGI.status(404, "Blah");
```

cli.js
=========

```
Sys=require('#Sys1');
Sys.dprint(1, "Number of arguments: "+Sys.argc + "\n");
for(i=0;i<Sys.argc;i++)
    Sys.dprint(1, i + ": "+ Sys[i] + "\n");
```

clone.js
=========

```
Sys=require('#Sys1');

pid = Sys.clone(Sys.CLONE_NEWPID);

Sys.dprint(1, "pid = " + pid + "\n");
Sys.dprint(1, "getpid = " + Sys.getpid());
```

daemon.js
=========

```
var Sys=require('#Sys1');

pid = Sys.fork();
if(pid > 0) {
    Sys._exit(0);
}

Sys.setsid();
Sys.chdir("/");
fd = Sys.open("/dev/null",Sys.O_RDWR, 0);
Sys.dup2(fd, 0);
Sys.dup2(fd, 1);
Sys.dup2(fd, 2);
if(fd > 2) Sys.close(fd);

while(1) {
    Sys.sleep(10);
}
```

exec.js
=========

```
Sys=require('#Sys1');

if( (pid=Sys.fork()) == 0) Sys.execve( "/bin/uname", [ "/bin/uname", "-a" ], [ ] );
Sys.waitpid(pid, 0);
Sys.execve( "/bin/env", [ "/bin/env" ], [ "HELLO=World", "PATH=/bin" ] );
```

file.js
=========

```
Sys=require('#Sys1');

fd = Sys.open('/etc/passwd', Sys.O_RDONLY, 0);
len = Sys.lseek(fd, 0, Sys.SEEK_END);
Sys.lseek(fd, 0, Sys.SEEK_SET);
Sys.dprint(1, "Len: "+len+"\n");
res = Sys.read(fd, len);
Sys.write(1, res.buffer, res.rc);
Sys.close(fd);

stat=Sys.lstat('/etc/group');
Sys.dprint(1, JSON.stringify(stat)+"\n");
```

lmdb1_cursor.js
=========

```
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
```

lmdb1_dbi.js
=========

```
"use strict";
var Db=require('#LMDB1');
var Sys=requre('#Sys1');

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
```

lmdb1_del.js
=========

```
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
```

lmdb1_env.js
=========

```
"use strict";
var Db=require('#LMDB1');
var Sys=require('#Sys1');

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
```

lmdb1_get.js
=========

```
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
```

lmdb1_put.js
=========

```
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

// rc = Db.put(txn, dbi, "trolleri", "trollera", Db.MDB_NOOVERWRITE);
rc = Db.put(txn, dbi, "trolleri", "trollera", 0);
Sys.dprint(1, "put rc = "+rc+"\n");

rc = Db.txn_commit(txn);
Sys.dprint(1, "txn_commit rc = "+rc+"\n");
```

lmdb1_txn.js
=========

```
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

var sub = {};
rc = Db.txn_begin(env, txn, 0, sub);
Sys.dprint(1, "txn_begin rc = "+rc+"\n");
Sys.dprint(1, "txn = "+sub.txn+"\n");

rc = Db.txn_commit(sub);
Sys.dprint(1, "txn_commit rc = "+rc+"\n");
rc = Db.txn_commit(txn);
Sys.dprint(1, "txn_commit rc = "+rc+"\n");
```

main.js
=========

```
Sys=require('#Sys1');

Sys.dprint(1, "main1\n");

function main()
{
    Sys.dprint(1, "main2\n");
    var mod = require('test');
    Sys.dprint(1, "main3\n");
    var mod = require('testfail');
}
```

pipes.js
=========

```
Sys=require('#Sys1');

fds = Sys.pipe();
Sys.dprint(1, fds.rc + "\n");
Sys.dprint(1, fds[0] + "\n");
Sys.dprint(1, fds[1] + "\n"); 
```

poll.js
=========

```
Sys=require('#Sys1');
var fds = [ { fd: 0, events: Sys.POLLIN, a: 11 } ];

while(1) {
    ret = Sys.poll( fds, 1, 5000);
    fds[0].a++;
    Sys.dprint(1, JSON.stringify(ret)+"\n");
    if(ret) {
	for(i=0;i<ret.fds.length;i++) {
	    if(ret.fds[i].revents) {
		res = Sys.read(ret.fds[i].fd, 1024);
		if(res.rc > 0)
		    Sys.dprint(1, "Read "+res.buffer);
	    }
	}
    }
}
```

tcpclient.js
=========

```
Sys=require('#Sys1');
fd = Sys.socket(Sys.AF_INET, Sys.SOCK_STREAM, Sys.IPPROTO_TCP);
Sys.connect(fd, { in: '127.0.0.1', port: 80 });
Sys.dprint(fd, "Hello");
Sys.close(fd);

```

tcpserver.js
=========

```
Sys=require('#Sys1');

fd = Sys.socket(Sys.AF_INET, Sys.SOCK_STREAM, Sys.IPPROTO_TCP);
Sys.setsockopt(fd, Sys.SOL_SOCKET, Sys.SO_REUSEADDR, 1);
Sys.bind(fd, { in: '127.0.0.1', port: 7780 });
Sys.listen(fd, 10);

while(1) {
    res = Sys.accept(fd);
    if(res.rc >= 0) {
	Sys.dprint(res.rc, res.family + " " + res.addr);
	Sys.close(res.rc);
    }
}
```

test.js
=========

```
Sys=require('#Sys1');

Sys.dprint(1, "hello\n");

function a(b)
{
    Sys.dprint(1, "a("+b+")\n");
    Sys.dprint(1, "apa="+this.apa+"\n");
}

c={};
c.apa='fisk';
c.f=a;
c.f('hej');
```

time.js
=========

```
Sys=require('#Sys1');

Sys.dprint(0, JSON.stringify(Sys.localtime(Sys.time())));

```

wait.js
=========

```
Sys=require('#Sys1');

pid = Sys.fork();

if(pid == 0) {
    Sys._exit(10);
}

r = Sys.waitpid(pid, 0);
Sys.dprint(1, JSON.stringify(r) + "\n");
```

