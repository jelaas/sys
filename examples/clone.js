Sys=Sys1;

pid = Sys.clone(Sys.CLONE_NEWPID);

Sys.dprint(1, "pid = " + pid + "\n");
Sys.dprint(1, "getpid = " + Sys.getpid());
