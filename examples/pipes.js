Sys=Sys1;

fds = Sys.pipe();
Sys.dprint(1, fds.rc + "\n");
Sys.dprint(1, fds[0] + "\n");
Sys.dprint(1, fds[1] + "\n"); 
