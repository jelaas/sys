"use strict";

var Sys=require('#Sys1');
var CGI=require('#Http1').CGI;

CGI.init();
CGI.header("Content-Type: text/html");
CGI.header("x-js: true");
CGI.header("Content-type: text/plain; charset=utf-8");
print("content");
CGI.status(404, "Blah");
