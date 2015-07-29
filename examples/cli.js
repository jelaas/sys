Sys=require('#Sys1');
Sys.dprint(1, "Number of arguments: "+Sys.argc + "\n");
for(i=0;i<Sys.argc;i++)
    Sys.dprint(1, i + ": "+ Sys[i] + "\n");
