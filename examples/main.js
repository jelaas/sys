Sys=require('#Sys1');

Sys.dprint(1, "main1\n");

function main()
{
    Sys.dprint(1, "main2\n");
    var mod = require('test');
    Sys.dprint(1, "main3\n");
    var mod = require('testfail');
}
