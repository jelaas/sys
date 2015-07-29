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
