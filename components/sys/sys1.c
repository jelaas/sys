/*
 * File: sys1.c
 * Implements: javascript vm (duktape) with system extensions (syscalls mainly)
 *
 * Copyright: Jens Låås, 2014
 * Copyright license: According to GPL, see file COPYING in this directory.
 *
 */

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/wait.h>
#include <poll.h>
#include <signal.h>
#include <sched.h>
#include <syscall.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>

//#include <stdio.h>
#include "duktape.h"
#include "sys1.h"

static int sys1_push_stat(duk_context *ctx, struct stat *stat)
{
	duk_push_object(ctx);
	duk_push_int(ctx, stat->st_dev);
	duk_put_prop_string(ctx, -2, "st_dev");
	duk_push_int(ctx, stat->st_ino);
	duk_put_prop_string(ctx, -2, "st_ino");
	duk_push_int(ctx, stat->st_mode);
	duk_put_prop_string(ctx, -2, "st_mode");
	duk_push_int(ctx, stat->st_nlink);
	duk_put_prop_string(ctx, -2, "st_nlink");
	duk_push_int(ctx, stat->st_uid);
	duk_put_prop_string(ctx, -2, "st_uid");
	duk_push_int(ctx, stat->st_gid);
	duk_put_prop_string(ctx, -2, "st_gid");
	duk_push_int(ctx, stat->st_rdev);
	duk_put_prop_string(ctx, -2, "st_rdev");
	duk_push_int(ctx, stat->st_size);
	duk_put_prop_string(ctx, -2, "st_size");
	duk_push_int(ctx, stat->st_blksize);
	duk_put_prop_string(ctx, -2, "st_blklsize");
	duk_push_int(ctx, stat->st_blocks);
	duk_put_prop_string(ctx, -2, "st_blocks");
	duk_push_int(ctx, stat->st_atime);
	duk_put_prop_string(ctx, -2, "st_atime");
	duk_push_int(ctx, stat->st_mtime);
	duk_put_prop_string(ctx, -2, "st_mtime");
	duk_push_int(ctx, stat->st_ctime);
	duk_put_prop_string(ctx, -2, "st_ctime");
	return 0;
}

static int sys1_accept(duk_context *ctx)
{
	int fd = duk_to_int(ctx, 0);
	ssize_t rc;
	char buf[64];
	union {
		struct sockaddr dst;
		struct sockaddr_in dst4;
		struct sockaddr_in6 dst6;
		struct sockaddr_un dstun;
	} addr;
	socklen_t addrlen = sizeof(addr);
	
	memset(&addr, 0, sizeof(addr));

	rc = accept(fd, &addr.dst, &addrlen);

	duk_push_object(ctx);
	
	duk_push_int(ctx, rc);
	duk_put_prop_string(ctx, -2, "rc");
	duk_push_int(ctx, addr.dst.sa_family);
	duk_put_prop_string(ctx, -2, "family");
	if((addr.dst.sa_family == AF_INET) && inet_ntop(addr.dst.sa_family, &addr.dst4.sin_addr, buf, sizeof(buf))) {
		duk_push_string(ctx, buf);
		duk_put_prop_string(ctx, -2, "addr");
	}

	return 1;
}

static int sys1_bind(duk_context *ctx)
{
	int rc;
	int port = 0;
	int fd = duk_to_int(ctx, 0);
	union {
		struct sockaddr dst;
		struct sockaddr_in dst4;
		struct sockaddr_in6 dst6;
		struct sockaddr_un dstun;
		struct sockaddr_ll dstll;
	} addr;
	socklen_t addrlen = 0;

	memset(&addr, 0, sizeof(addr));

	duk_get_prop_string(ctx, 1, "port");
	if(duk_check_type(ctx, 2, DUK_TYPE_NUMBER)) {
		port = duk_to_int(ctx, 2);
	}
	duk_pop(ctx);

	duk_get_prop_string(ctx, 1, "ifindex");
	if(duk_check_type(ctx, 2, DUK_TYPE_NUMBER)) {
		addrlen = sizeof(addr.dstll);
		addr.dstll.sll_ifindex = duk_to_int(ctx, 2);
		addr.dstll.sll_family = AF_PACKET;
		addr.dstll.sll_protocol = htons(ETH_P_ALL);

		duk_get_prop_string(ctx, 1, "protocol");
		if(duk_check_type(ctx, 2, DUK_TYPE_NUMBER)) {
			addr.dstll.sll_protocol = htons(duk_to_int(ctx, 2));
		}
		duk_pop(ctx);
	}
	duk_pop(ctx);
	
	duk_get_prop_string(ctx, 1, "in");
	if(duk_check_type(ctx, 2, DUK_TYPE_STRING)) {
		addrlen = sizeof(addr.dst4);
		inet_aton(duk_to_string(ctx, 2), &addr.dst4.sin_addr);
		addr.dst4.sin_port = htons(port);
		addr.dst4.sin_family = AF_INET;
	}
	duk_pop(ctx);
	
	duk_get_prop_string(ctx, 1, "un");
	if(duk_check_type(ctx, 2, DUK_TYPE_STRING)) {
		const char *path;
		addrlen = sizeof(addr.dstun);
		path = duk_to_string(ctx, 2);
		if(strlen(path) > sizeof(addr.dstun.sun_path)) {
			duk_pop(ctx);
			rc=-1;
			goto out;
		}
		memcpy(addr.dstun.sun_path, path, strlen(path));
		addr.dstun.sun_family = AF_LOCAL;
	}
	duk_pop(ctx);

	duk_get_prop_string(ctx, 1, "una");
	if(duk_check_type(ctx, 2, DUK_TYPE_STRING)) {
		const char *path;
		addrlen = sizeof(addr.dstun);
		path = duk_to_string(ctx, 2);
		if(strlen(path) > (sizeof(addr.dstun.sun_path)-1)) {
			duk_pop(ctx);
			rc=-1;
			goto out;
		}
		memcpy(addr.dstun.sun_path+1, path, strlen(path));
		addr.dstun.sun_family = AF_LOCAL;
	}
	duk_pop(ctx);

	duk_get_prop_string(ctx, 1, "in6");
	if(duk_check_type(ctx, 2, DUK_TYPE_STRING)) {
		addrlen = sizeof(addr.dst6);
		inet_pton(AF_INET6, duk_to_string(ctx, 2), &addr.dst6.sin6_addr);
		addr.dst6.sin6_port = htons(port);
		addr.dst6.sin6_family = AF_INET6;
	}
	duk_pop(ctx);

	rc = bind(fd, &addr.dst, addrlen);

out:	
	duk_push_number(ctx, rc);
	return 1;
}

static int sys1_chdir(duk_context *ctx)
{
	const char *path = duk_to_string(ctx, 0);
	int rc;

	rc = chdir(path);

	duk_push_int(ctx, rc);
	return 1;
}

static int sys1_chmod(duk_context *ctx)
{
	const char *path = duk_to_string(ctx, 0);
	mode_t mode = duk_to_int(ctx, 1);
	int rc;

	rc = chmod(path, mode);

	duk_push_int(ctx, rc);
	return 1;
}

static int sys1_clone(duk_context *ctx)
{
	int flags = duk_to_int(ctx, 0);
	pid_t pid;

	pid = syscall(SYS_clone, flags|SIGCHLD, 0);
	duk_push_int(ctx, pid);
	return 1;
}

static int sys1_close(duk_context *ctx)
{
	int fd = duk_to_int(ctx, 0);
	
	int rc  = close(fd);
	
	duk_push_number(ctx, rc);
	return 1;
}

static int sys1_connect(duk_context *ctx)
{
	int rc;
	int port = 0;
	int fd = duk_to_int(ctx, 0);
	union {
		struct sockaddr dst;
		struct sockaddr_in dst4;
		struct sockaddr_in6 dst6;
		struct sockaddr_un dstun;
	} addr;
	socklen_t addrlen = 0;

	memset(&addr, 0, sizeof(addr));

	duk_get_prop_string(ctx, 1, "port");
	if(duk_check_type(ctx, 2, DUK_TYPE_NUMBER)) {
		port = duk_to_int(ctx, 2);
	}
	duk_pop(ctx);

	duk_get_prop_string(ctx, 1, "in");
	if(duk_check_type(ctx, 2, DUK_TYPE_STRING)) {
		addrlen = sizeof(addr.dst4);
		inet_aton(duk_to_string(ctx, 2), &addr.dst4.sin_addr);
		addr.dst4.sin_port = htons(port);
		addr.dst4.sin_family = AF_INET;
	}
	duk_pop(ctx);

	duk_get_prop_string(ctx, 1, "un");
	if(duk_check_type(ctx, 2, DUK_TYPE_STRING)) {
		const char *path;
		addrlen = sizeof(addr.dstun);
		path = duk_to_string(ctx, 2);
		if(strlen(path) > sizeof(addr.dstun.sun_path)) {
			duk_pop(ctx);
			rc=-1;
			goto out;
		}
		memcpy(addr.dstun.sun_path, path, strlen(path));
		addr.dstun.sun_family = AF_LOCAL;
	}
	duk_pop(ctx);

	duk_get_prop_string(ctx, 1, "una");
	if(duk_check_type(ctx, 2, DUK_TYPE_STRING)) {
		const char *path;
		addrlen = sizeof(addr.dstun);
		path = duk_to_string(ctx, 2);
		if(strlen(path) > (sizeof(addr.dstun.sun_path)-1)) {
			duk_pop(ctx);
			rc=-1;
			goto out;
		}
		memcpy(addr.dstun.sun_path+1, path, strlen(path));
		addr.dstun.sun_family = AF_LOCAL;
	}
	duk_pop(ctx);

	duk_get_prop_string(ctx, 1, "in6");
	if(duk_check_type(ctx, 2, DUK_TYPE_STRING)) {
		addrlen = sizeof(addr.dst6);
		inet_pton(AF_INET6, duk_to_string(ctx, 2), &addr.dst6.sin6_addr);
		addr.dst6.sin6_port = htons(port);
		addr.dst6.sin6_family = AF_INET6;
	}
	duk_pop(ctx);

	rc = connect(fd, &addr.dst, addrlen);

out:	
	duk_push_number(ctx, rc);
	return 1;
}

static int sys1_dprint(duk_context *ctx)
{
	int fd = duk_to_int(ctx, 0);
	const char *buf = duk_to_string(ctx, 1);
	int len;
	ssize_t rc;

	len = strlen(buf);

	rc = write(fd, buf, len);

	duk_push_int(ctx, rc);
	return 1;
}

static int sys1_dup2(duk_context *ctx)
{
	int rc;

	int oldfd = duk_to_int(ctx, 0);
	int newfd = duk_to_int(ctx, 1);

	rc = dup2(oldfd, newfd);
	
	duk_push_number(ctx, rc);
	return 1;	
}

static int sys1_errno(duk_context *ctx)
{
	duk_push_int(ctx, errno);
	return 1;
}

static int sys1_execve(duk_context *ctx)
{
	const char *filename = duk_to_string(ctx, 0);
	int rc;
	char **argv;
	char **envp;
	int argc = 0, envc = 0, i;
	
	// count arguments
	// FIXME: Use duk_get_length() instead ?
	while(1) {
		rc = duk_get_prop_index(ctx, 1, argc);
		duk_pop(ctx);
		if(rc == 0) break;
		argc++;
	}

	argv = malloc(sizeof(char*)*(argc+1));
	
	for(i=0;i<argc;i++) {
		duk_get_prop_index(ctx, 1, i);
		argv[i] = (char*) duk_to_string(ctx, 3);
		duk_pop(ctx);
	}
	argv[i] = (void*)0;

	// count arguments
	// FIXME: Use duk_get_length() instead ?
	while(1) {
		rc = duk_get_prop_index(ctx, 2, envc);
		duk_pop(ctx);
		if(rc == 0) break;
		envc++;
	}

	envp = malloc(sizeof(char*)*(envc+1));
	
	for(i=0;i<envc;i++) {
		duk_get_prop_index(ctx, 2, i);
		envp[i] = (char*) duk_to_string(ctx, 3);
		duk_pop(ctx);
	}
	envp[i] = (void*)0;
	
	rc = execve(filename, argv, envp);
	
	duk_push_int(ctx, rc);
	return 1;
}

static int sys1_exit(duk_context *ctx)
{
	int code = duk_to_int(ctx, 0);

	exit(code);

	return 1;
}

static int sys1__exit(duk_context *ctx)
{
	int code = duk_to_int(ctx, 0);

	_exit(code);

	return 1;
}

static int sys1_fchmod(duk_context *ctx)
{
	int fd = duk_to_int(ctx, 0);
	mode_t mode = duk_to_int(ctx, 1);
	int rc;

	rc = fchmod(fd, mode);

	duk_push_int(ctx, rc);
	return 1;
}

static int sys1_fcntl(duk_context *ctx)
{
	int rc;
	int fd = duk_to_int(ctx, 0);
	int cmd = duk_to_int(ctx, 1);
	int arg = 0;
	
	if(cmd == F_DUPFD ||
	   cmd == F_DUPFD_CLOEXEC ||
	   cmd == F_SETFD ||
	   cmd == F_SETFL) {
		arg = duk_to_int(ctx, 2);
	}

	rc = fcntl(fd, cmd, arg);
	
	duk_push_int(ctx, rc);
	return 1;
}

static int sys1_fork(duk_context *ctx)
{
	duk_push_number(ctx, fork());
	return 1;
}

static int sys1_fstat(duk_context *ctx)
{
	int fd = duk_to_int(ctx, 0);
	struct stat stat;
	int rc;
	
	rc = fstat(fd, &stat);
	sys1_push_stat(ctx, &stat);
	duk_push_int(ctx, rc);
	duk_put_prop_string(ctx, -2, "rc");
	return 1;
}

static int sys1_getenv(duk_context *ctx)
{
	const char *name = duk_to_string(ctx, 0);
	char *val;

	val = getenv(name);
	if(val)
		duk_push_string(ctx, val);
	else 
		duk_push_undefined(ctx);
	return 1;
}

static int sys1_getpid(duk_context *ctx)
{
	pid_t rc;

	rc = getpid();
	duk_push_int(ctx, rc);
	return 1;
}

static int sys1_getppid(duk_context *ctx)
{
	pid_t rc;

	rc = getppid();
	duk_push_int(ctx, rc);
	return 1;
}

static int sys1_gettimeofday(duk_context *ctx)
{
	pid_t rc;
	struct timeval tv;

	rc = gettimeofday(&tv, (void*)0);

	duk_push_uint(ctx, tv.tv_sec);
	duk_put_prop_string(ctx, 0, "tv_sec");
	duk_push_uint(ctx, tv.tv_usec);
	duk_put_prop_string(ctx, 0, "tv_usec");
	
	duk_push_int(ctx, rc);
	return 1;
}

static int sys1_gmtime(duk_context *ctx)
{
	struct tm tm, *tmp;
	time_t timep = duk_to_uint(ctx, 0);

	tmp = gmtime_r(&timep, &tm);
	if(!tmp) {
		duk_push_undefined(ctx);
		return 1;
	}

	duk_push_object(ctx);
	duk_push_uint(ctx, tm.tm_sec);
	duk_put_prop_string(ctx, 1, "tm_sec");
	duk_push_uint(ctx, tm.tm_min);
	duk_put_prop_string(ctx, 1, "tm_min");
	duk_push_uint(ctx, tm.tm_hour);
	duk_put_prop_string(ctx, 1, "tm_hour");
	duk_push_uint(ctx, tm.tm_mday);
	duk_put_prop_string(ctx, 1, "tm_mday");
	duk_push_uint(ctx, tm.tm_mon);
	duk_put_prop_string(ctx, 1, "tm_mon");
	duk_push_uint(ctx, tm.tm_year);
	duk_put_prop_string(ctx, 1, "tm_year");
	duk_push_uint(ctx, tm.tm_wday);
	duk_put_prop_string(ctx, 1, "tm_wday");
	duk_push_uint(ctx, tm.tm_yday);
	duk_put_prop_string(ctx, 1, "tm_yday");
	duk_push_uint(ctx, tm.tm_isdst);
	duk_put_prop_string(ctx, 1, "tm_isdst");
	
	return 1;
}

static int sys1_kill(duk_context *ctx)
{
	int pid = duk_to_int(ctx, 0);
	int sig = duk_to_int(ctx, 1);
	int rc;
	
	rc = kill(pid, sig);
	duk_push_int(ctx, rc);
	return 1;
}

static int sys1_listen(duk_context *ctx)
{
	int rc;

	int fd = duk_to_int(ctx, 0);
	int backlog = duk_to_int(ctx, 1);

	rc = listen(fd, backlog);
	
	duk_push_number(ctx, rc);
	return 1;	
}

static int sys1_localtime(duk_context *ctx)
{
	struct tm tm, *tmp;
	time_t timep = duk_to_uint(ctx, 0);

	tmp = localtime_r(&timep, &tm);
	if(!tmp) {
		duk_push_undefined(ctx);
		return 1;
	}

	duk_push_object(ctx);
	duk_push_uint(ctx, tm.tm_sec);
	duk_put_prop_string(ctx, 1, "tm_sec");
	duk_push_uint(ctx, tm.tm_min);
	duk_put_prop_string(ctx, 1, "tm_min");
	duk_push_uint(ctx, tm.tm_hour);
	duk_put_prop_string(ctx, 1, "tm_hour");
	duk_push_uint(ctx, tm.tm_mday);
	duk_put_prop_string(ctx, 1, "tm_mday");
	duk_push_uint(ctx, tm.tm_mon);
	duk_put_prop_string(ctx, 1, "tm_mon");
	duk_push_uint(ctx, tm.tm_year);
	duk_put_prop_string(ctx, 1, "tm_year");
	duk_push_uint(ctx, tm.tm_wday);
	duk_put_prop_string(ctx, 1, "tm_wday");
	duk_push_uint(ctx, tm.tm_yday);
	duk_put_prop_string(ctx, 1, "tm_yday");
	duk_push_uint(ctx, tm.tm_isdst);
	duk_put_prop_string(ctx, 1, "tm_isdst");
	
	return 1;
}

static int sys1_lseek(duk_context *ctx)
{
	int fd = duk_to_int(ctx, 0);
	off_t offset = duk_to_int(ctx, 1);
	int whence = duk_to_int(ctx, 2);
	off_t rc;

	rc = lseek(fd, offset, whence);

	duk_push_int(ctx, rc);
	return 1;
}

static int sys1_lstat(duk_context *ctx)
{
	const char *path = duk_to_string(ctx, 0);
	struct stat stat;
	int rc;
	
	rc = lstat(path, &stat);
	sys1_push_stat(ctx, &stat);
	duk_push_int(ctx, rc);
	duk_put_prop_string(ctx, -2, "rc");
	return 1;
}

static int sys1_mkdir(duk_context *ctx)
{
	const char *path = duk_to_string(ctx, 0);
	mode_t mode = duk_to_int(ctx, 1);
	int rc;

	rc = mkdir(path, mode);

	duk_push_int(ctx, rc);
	return 1;
}

static int sys1_open(duk_context *ctx)
{
	int fd;
	
	const char *path = duk_to_string(ctx, 0);
	uint32_t flags = duk_to_uint32(ctx, 1);
	int mode = duk_to_int(ctx, 2);

	fd = open(path, flags, mode);
	
	duk_push_number(ctx, fd);
	return 1;
}

static int sys1_pipe(duk_context *ctx)
{
	int rc;
	int fd[2];
	
	rc = pipe(fd);

	duk_push_object(ctx);
	duk_push_int(ctx, rc);
	duk_put_prop_string(ctx, -2, "rc");
	duk_push_int(ctx, fd[0]);
	duk_put_prop_index(ctx, -2, 0);
	duk_push_int(ctx, fd[1]);
	duk_put_prop_index(ctx, -2, 1);
	return 1;
}

static int sys1_poll(duk_context *ctx)
{
	int nfds = duk_to_int(ctx, 1);
	int timeout = duk_to_int(ctx, 2);
	int i, rc;
	struct pollfd *fds;

	fds = malloc(sizeof(struct pollfd)*nfds);
	memset(fds, 0, sizeof(struct pollfd)*nfds);
	
	for(i=0;i<nfds;i++) {
		duk_get_prop_index(ctx, 0, i);
		duk_get_prop_string(ctx, 3, "fd");	
		fds[i].fd = duk_to_int(ctx, 4);
		duk_get_prop_string(ctx, 3, "events");	
		fds[i].events = duk_to_int(ctx, 5);
		duk_pop_n(ctx, 3);
	}

	rc = poll(fds, nfds, timeout);
	duk_push_object(ctx);
	duk_push_int(ctx, rc);
	duk_put_prop_string(ctx, -2, "rc");
	duk_dup(ctx, 0); // dup reference to arg0 array
	for(i=0;i<nfds;i++) {
		duk_get_prop_index(ctx, -1, i); // fetch object from array at i
		duk_push_int(ctx, fds[i].revents);
		duk_put_prop_string(ctx, -2, "revents"); // put revents into object
		duk_pop(ctx); // remove object from stack
	}
	duk_put_prop_string(ctx, -2, "fds");

	return 1;
}

static int sys1_read(duk_context *ctx)
{
	int fd = duk_to_int(ctx, 0);
	int len = duk_to_int(ctx, 1);
	ssize_t rc;
	char *buf;
	
	duk_push_object(ctx);
	
	buf = duk_push_fixed_buffer(ctx, len);
	duk_put_prop_string(ctx, -2, "buffer");

	rc = read(fd, buf, len);

	duk_push_int(ctx, rc);
	duk_put_prop_string(ctx, -2, "rc");
	return 1;
}

static int sys1_rename(duk_context *ctx)
{
	const char *oldpath = duk_to_string(ctx, 0);
	const char *newpath = duk_to_string(ctx, 1);
	int rc;

	rc = rename(oldpath, newpath);

	duk_push_int(ctx, rc);
	return 1;
}

static int sys1_setsid(duk_context *ctx)
{
	duk_push_number(ctx, setsid());
	return 1;
}

static int sys1_setsockopt(duk_context *ctx)
{
	int fd = duk_to_int(ctx, 0);
	int level = duk_to_int(ctx, 1);
	int optname = duk_to_int(ctx, 2);
	int optval;
	int rc;

	if(optname == SO_BINDTODEVICE) {
		const char *optstr;
		optstr = duk_to_string(ctx, 3);
		rc = setsockopt(fd, level, optname, optstr, strlen(optstr));
		goto done;
	}

	optval = duk_to_int(ctx, 3);
	rc = setsockopt(fd, level, optname, (void*)&optval, sizeof(optval));
done:
	duk_push_int(ctx, rc);
	return 1;
}

static int sys1_sleep(duk_context *ctx)
{
	int rc;

	int seconds = duk_to_int(ctx, 0);

	rc = sleep(seconds);
	
	duk_push_number(ctx, rc);
	return 1;
}

static int sys1_socket(duk_context *ctx)
{
	int fd;

	int domain = duk_to_int(ctx, 0);
	int type = duk_to_int(ctx, 1);
	int protocol = duk_to_int(ctx, 2);

	fd = socket(domain, type, protocol);
	
	duk_push_number(ctx, fd);
	return 1;	
}

static int sys1_stat(duk_context *ctx)
{
	const char *path = duk_to_string(ctx, 0);
	struct stat buf;
	int rc;
	
	rc = stat(path, &buf);
	sys1_push_stat(ctx, &buf);
	duk_push_int(ctx, rc);
	duk_put_prop_string(ctx, -2, "rc");
	return 1;
}

static int sys1_strerror(duk_context *ctx)
{
	int num = duk_to_int(ctx, 0);
	duk_push_string(ctx, strerror(num));
	return 1;
}

static int sys1_time(duk_context *ctx)
{
	time_t t;

	t = time(0);
	duk_push_uint(ctx, t);
	return 1;
}

static int sys1_umask(duk_context *ctx)
{
	mode_t mask = duk_to_int(ctx, 0);
	mode_t rc;

	rc = umask(mask);
	duk_push_int(ctx, rc);
	return 1;
}

static int sys1_unlink(duk_context *ctx)
{
	const char *pathname = duk_to_string(ctx, 0);
	int rc;

	rc = unlink(pathname);
	duk_push_int(ctx, rc);
	return 1;
}

static int sys1_unshare(duk_context *ctx)
{
	int flags = duk_to_int(ctx, 0);
	int rc;

	rc = unshare(flags);
	duk_push_int(ctx, rc);
	return 1;
}

static int sys1_waitpid(duk_context *ctx)
{
	int rc;
	pid_t pid = duk_to_int(ctx, 0);
	int status = 0;
	int options = duk_to_int(ctx, 1);

	rc = waitpid(pid, &status, options);

	duk_push_object(ctx);
	duk_push_int(ctx, rc);
	duk_put_prop_string(ctx, -2, "rc");
	duk_push_int(ctx, status);
	duk_put_prop_string(ctx, -2, "status");
	duk_push_int(ctx, WIFEXITED(status));
	duk_put_prop_string(ctx, -2, "exited");
	duk_push_int(ctx, WEXITSTATUS(status));
	duk_put_prop_string(ctx, -2, "exitstatus");
	duk_push_int(ctx, WIFSIGNALED(status));
	duk_put_prop_string(ctx, -2, "signaled");
	duk_push_int(ctx, WTERMSIG(status));
	duk_put_prop_string(ctx, -2, "signal");
	return 1;
}

static int sys1_write(duk_context *ctx)
{
	int fd = duk_to_int(ctx, 0);
	size_t bufsize;
	void *buf = duk_to_buffer(ctx, 1, &bufsize);
	int len = duk_to_int(ctx, 2);
	ssize_t rc;
	
	rc = write(fd, buf, len);

	duk_push_int(ctx, rc);
	return 1;
}

static const duk_function_list_entry sys1_funcs[] = {
	{ "accept", sys1_accept, 1 /* fd */ },
	{ "bind", sys1_bind, 2 /* fd, obj { in, in6, un, una, port } */ },
	{ "chdir", sys1_chdir, 1 },
	{ "chmod", sys1_chmod, 2 },
	{ "clone", sys1_clone, 1 },
	{ "close", sys1_close, 1 /* fd */ },
	{ "connect", sys1_connect, 2 /* fd, obj { in, in6, un, una, port } */ },
	{ "dprint", sys1_dprint, 2 /* fd, string */ },
//      { "dprintf", sys1_dprintf, DUK_VARARGS },
	{ "dup2", sys1_dup2, 2 },
	{ "errno", sys1_errno, 0 },
	{ "execve", sys1_execve, 3 },
	{ "exit", sys1_exit, 1 },
	{ "_exit", sys1__exit, 1 },
	{ "fchmod", sys1_fchmod, 2 },
	{ "fcntl", sys1_fcntl, 3 },
	{ "fork", sys1_fork, 1 },
	{ "fstat", sys1_fstat, 1 },
	{ "getenv", sys1_getenv, 1 },
	{ "getpid", sys1_getpid, 0 },
	{ "getppid", sys1_getppid, 0 },
	{ "gettimeofday", sys1_gettimeofday, 1 },
	{ "gmtime", sys1_gmtime, 1 },
	{ "kill", sys1_kill, 2 },
	{ "listen", sys1_listen, 2 /* fd, backlog */ },
	{ "localtime", sys1_localtime, 1 },
	{ "lseek", sys1_lseek, 3 },
	{ "lstat", sys1_lstat, 1 },
	{ "mkdir", sys1_mkdir, 2 },
//	{ "mmap", sys1_mmap, 6 },
//	{ "mount", sys1_mount, 5 },
	{ "open", sys1_open, 3 /* filename, flags, mode */ },
	{ "pipe", sys1_pipe, 0 },
	{ "poll", sys1_poll, 3 },
	{ "read", sys1_read, 2 /* fd, count */ },
	{ "rename", sys1_rename, 2 },
        { "setsid", sys1_setsid, 0 },
	{ "setsockopt", sys1_setsockopt, 4 },
	{ "sleep", sys1_sleep, 1 },
	{ "socket", sys1_socket, 3 /* domain, type, protocol */ },
	{ "stat", sys1_stat, 1 },
	{ "strerror", sys1_strerror, 1 },
//	{ "tcgetattr", sys1_umount , 3 },
//	{ "tcsetattr", sys1_umount , 3 },
	{ "time", sys1_time, 0 },
	{ "umask", sys1_umask , 1 },
//	{ "umount", sys1_umount , 1 },
//	{ "umount2", sys1_umount2 , 2 },
	{ "unlink", sys1_unlink , 1 },
	{ "unshare", sys1_unshare, 1 },
	{ "waitpid", sys1_waitpid, 2 },
	{ "write", sys1_write, 3 /* fd, buffer, count */ },
	{ NULL, NULL, 0 }
	};

static const duk_number_list_entry sys1_consts[] = {
	{ "CLONE_FILES", (double) CLONE_FILES },
	{ "CLONE_FS", (double) CLONE_FS },
	{ "CLONE_NEWIPC", (double) CLONE_NEWIPC },
	{ "CLONE_NEWNET", (double) CLONE_NEWNET },
	{ "CLONE_NEWNS", (double) CLONE_NEWNS },
	{ "CLONE_NEWUTS", (double) CLONE_NEWUTS },
	{ "CLONE_SYSVSEM", (double) CLONE_SYSVSEM },
	{ "CLONE_NEWPID", (double) CLONE_NEWPID },
	{ "O_RDONLY", (double) O_RDONLY },
	{ "O_WRONLY,", (double) O_WRONLY, },
	{ "O_RDWR", (double) O_RDWR },
	{ "O_APPEND", (double) O_APPEND },
	{ "O_ASYNC", (double) O_ASYNC },
	{ "O_CLOEXEC", (double) O_CLOEXEC },
	{ "O_CREAT", (double) O_CREAT },
	{ "O_DIRECT", (double) O_DIRECT },
	{ "O_EXCL", (double) O_EXCL },
	{ "O_LARGEFILE", (double) O_LARGEFILE },
	{ "O_NOATIME", (double) O_NOATIME },
	{ "O_NOCTTY", (double) O_NOCTTY },
	{ "O_NOFOLLOW", (double) O_NOFOLLOW },
	{ "O_NONBLOCK", (double) O_NONBLOCK },
	{ "O_SYNC", (double) O_SYNC },
	{ "O_TRUNC", (double) O_TRUNC },

	{ "F_DUPFD", (double) F_DUPFD },
	{ "F_DUPFD_CLOEXEC", (double) F_DUPFD_CLOEXEC },
	{ "F_GETFD", (double) F_GETFD },
	{ "F_SETFD", (double) F_SETFD },
	{ "F_GETFL", (double) F_GETFL },
	{ "F_SETFL", (double) F_SETFL },
	{ "FD_CLOEXEC", (double) FD_CLOEXEC },

	{ "SOCK_STREAM", (double) SOCK_STREAM },
	{ "SOCK_DGRAM", (double) SOCK_DGRAM },
	{ "SOCK_SEQPACKET", (double) SOCK_SEQPACKET },
	{ "SOCK_RAW", (double) SOCK_RAW },
	{ "SOCK_RDM", (double) SOCK_RDM },
	{ "SOCK_NONBLOCK", (double) SOCK_NONBLOCK },
	{ "SOCK_CLOEXEC", (double) SOCK_CLOEXEC },
	{ "SO_ACCEPTCONN", (double) SO_ACCEPTCONN },
	{ "SO_BINDTODEVICE", (double) SO_BINDTODEVICE },
	{ "SO_BROADCAST", (double) SO_BROADCAST },
	{ "SO_DEBUG", (double) SO_DEBUG },
	{ "SO_DOMAIN", (double) SO_DOMAIN },
	{ "SO_ERROR", (double) SO_ERROR },
	{ "SO_DONTROUTE", (double) SO_DONTROUTE },
	{ "SO_KEEPALIVE", (double) SO_KEEPALIVE },
	{ "SO_LINGER", (double) SO_LINGER },
	{ "SO_MARK", (double) SO_MARK },
	{ "SO_OOBINLINE", (double) SO_OOBINLINE },
	{ "SO_PASSCRED", (double) SO_PASSCRED },
	{ "SO_PEERCRED", (double) SO_PEERCRED },
	{ "SO_PRIORITY", (double) SO_PRIORITY },
	{ "SO_PROTOCOL", (double) SO_PROTOCOL },
	{ "SO_RCVBUF", (double) SO_RCVBUF },
	{ "SO_RCVBUFFORCE", (double) SO_RCVBUFFORCE },
	{ "SO_RCVLOWAT", (double) SO_RCVLOWAT },
	{ "SO_SNDLOWAT", (double) SO_SNDLOWAT },
	{ "SO_RCVTIMEO", (double) SO_RCVTIMEO },
	{ "SO_SNDTIMEO", (double) SO_SNDTIMEO },
	{ "SO_REUSEADDR", (double) SO_REUSEADDR },
	{ "SO_SNDBUF", (double) SO_SNDBUF },
	{ "SO_SNDBUFFORCE", (double) SO_SNDBUFFORCE },
	{ "SO_TIMESTAMP", (double) SO_TIMESTAMP },
	{ "SO_TYPE", (double) SO_TYPE },
	{ "SOL_SOCKET", (double) SOL_SOCKET },
	{ "SOL_IP", (double) SOL_IP },
	{ "SOL_IPV6", (double) SOL_IPV6 },
	{ "SOL_RAW", (double) SOL_RAW },
	{ "SOL_PACKET", (double) SOL_PACKET },
	{ "AF_UNIX", (double) AF_UNIX },
	{ "AF_INET", (double) AF_INET },
	{ "AF_INET6", (double) AF_INET6 },
	{ "IPPROTO_UDP", (double) IPPROTO_UDP },
	{ "IPPROTO_TCP", (double) IPPROTO_TCP },
	{ "IPPROTO_IP", (double) IPPROTO_IP },
	{ "INADDR_ANY", (double) INADDR_ANY },
	{ "INADDR_LOOPBACK", (double) INADDR_LOOPBACK },

	{ "WNOHANG", (double) WNOHANG },
	{ "WUNTRACED", (double) WUNTRACED },
	{ "WCONTINUED", (double) WCONTINUED },

	{ "POLLIN", (double) POLLIN },
	{ "POLLOUT", (double) POLLOUT },
	{ "POLLHUP", (double) POLLHUP },
	{ "POLLERR", (double) POLLERR },
	{ "POLLPRI", (double) POLLPRI },
	{ "POLLNVAL", (double) POLLNVAL },
	{ "SEEK_SET", (double) SEEK_SET },
	{ "SEEK_CUR", (double) SEEK_CUR },
	{ "SEEK_END", (double) SEEK_END },
//	{ "SEEK_DATA", (double) SEEK_DATA },
//	{ "SEEK_HOLE", (double) SEEK_HOLE },
	
	{ "SIGHUP", (double) SIGHUP },
	{ "SIGINT", (double) SIGINT },
	{ "SIGQUIT", (double) SIGQUIT },
	{ "SIGILL", (double) SIGILL },
	{ "SIGTRAP", (double) SIGTRAP },
	{ "SIGABRT", (double) SIGABRT },
	{ "SIGIOT", (double) SIGIOT },
	{ "SIGBUS", (double) SIGBUS },
	{ "SIGFPE", (double) SIGFPE },
	{ "SIGKILL", (double) SIGKILL },
	{ "SIGUSR1", (double) SIGUSR1 },
	{ "SIGSEGV", (double) SIGSEGV },
	{ "SIGUSR2", (double) SIGUSR2 },
	{ "SIGPIPE", (double) SIGPIPE },
	{ "SIGALRM", (double) SIGALRM },
	{ "SIGTERM", (double) SIGTERM },
	{ "SIGSTKFLT", (double) SIGSTKFLT },
	{ "SIGCHLD", (double) SIGCHLD },
	{ "SIGCONT", (double) SIGCONT },
	{ "SIGSTOP", (double) SIGSTOP },
	{ "SIGTSTP", (double) SIGTSTP },
	{ "SIGTTIN", (double) SIGTTIN },
	{ "SIGTTOU", (double) SIGTTOU },
	{ "SIGURG", (double) SIGURG },
	{ "SIGXCPU", (double) SIGXCPU },
	{ "SIGXFSZ", (double) SIGXFSZ },
	{ "SIGVTALRM", (double) SIGVTALRM },
	{ "SIGPROF", (double) SIGPROF },
	{ "SIGWINCH", (double) SIGWINCH },
	{ "SIGIO", (double) SIGIO },
	{ "SIGPOLL", (double) SIGPOLL },
	{ "SIGPWR", (double) SIGPWR },
	{ "SIGSYS", (double) SIGSYS },
	
//	{ "", (double) },

	{ NULL, 0.0 }
};

int sys1(duk_context *ctx)
{
	duk_put_function_list(ctx, -1, sys1_funcs);
	duk_put_number_list(ctx, -1, sys1_consts);
	return 0;
}
