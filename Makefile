CC=musl-gcc-x86_32
C_DEFS=-DCOMPSYS -DCOMPHTTP -DCOMPLMDB
CFLAGS=-Wall -Os -std=c99 -D_POSIX_SOURCE -D_GNU_SOURCE -I. $(C_DEFS)
C_SYS=components/sys/sys1.o
sys:	sys.o prg.o $(C_SYS) components/http/http1.o components/lmdb/db1.o duktape.o liblmdb.a
liblmdb.a:	components/lmdb/mdb.o components/lmdb/midl.o
	ar rs $@ components/lmdb/mdb.o components/lmdb/midl.o
README.md:
	cat HEAD.md > README.md
	examples/genexamples.sh >> README.md
clean:
	rm -f *.o components/*/*.o liblmdb.a sys
