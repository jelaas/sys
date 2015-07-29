sys
=====

javascript vm with extensions.

Uses http://duktape.org/ for javascript vm.

Keeping as close as possible to the C APIs that are mapped in the javascript modules.
No callbacks unless the C API must have them.

Current extensions are:
* Sys
  System extensions. Mainly direct access to kernel systemcalls.
* LMDB
  The Lightning Memory-Mapped Database Manager.
* HTTP
  Working with HTTP.

