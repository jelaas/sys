Sys=Sys1;
fd = Sys.socket(Sys.AF_INET, Sys.SOCK_STREAM, Sys.IPPROTO_TCP);
Sys.connect(fd, { in: '127.0.0.1', port: 80 });
Sys.dprint(fd, "Hello");
Sys.close(fd);

