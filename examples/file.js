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
