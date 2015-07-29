Sys=require('#Sys1');

if( (pid=Sys.fork()) == 0) Sys.execve( "/bin/uname", [ "/bin/uname", "-a" ], [ ] );
Sys.waitpid(pid, 0);
Sys.execve( "/bin/env", [ "/bin/env" ], [ "HELLO=World", "PATH=/bin" ] );
