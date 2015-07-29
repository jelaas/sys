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
