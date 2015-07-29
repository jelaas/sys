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
