Sys=require('#Sys1');

pid = Sys.fork();

if(pid == 0) {
    Sys._exit(10);
}

r = Sys.waitpid(pid, 0);
Sys.dprint(1, JSON.stringify(r) + "\n");
