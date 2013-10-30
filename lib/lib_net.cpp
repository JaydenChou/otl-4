#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <sys/un.h>
#include <fcntl.h>
#include "lib_net.h"
#include "lib_log.h"

const int INET_ADDRLEN = 16;
int lib_socket(int family, int type, int protocol)
{
	int val = socket(family, type, protocol);
	if (val == -1) {
		lib_writelog(LIB_LOG_WARNING, "socket(%d, %d, %d) call failed. error[%d] info is %s.", family, type, protocol, errno, strerror(errno));
	}

	return val;
}

int lib_connect(int sockfd, const struct sockaddr *sa, socklen_t addrlen)
{
	int val = connect(sockfd, sa, addrlen);
	if (val == -1) {
		lib_writelog(LIB_LOG_WARNING, "connect(%d, <%d %d %u>, %d) call failed. error[%d] info is %s.", sockfd, ((struct sockaddr_in *) sa)->sin_family, ((struct sockaddr_in *)sa)->sin_port, ((struct sockaddr_in *) sa)->sin_addr.s_addr, addrlen, errno, strerror(errno));
	}

	return val;
}

int lib_bind(int sockfd, const struct sockaddr *sa, socklen_t addrlen)
{
	int val = bind(sockfd, sa, addrlen);
	if (val == -1) {
		lib_writelog(LIB_LOG_WARNING, "bind(%d, <%d %d %u>, %d) call failed. error[%d] info is %s.", sockfd, ((struct sockaddr_in *) sa)->sin_family, ((struct sockaddr_in *)sa)->sin_port, ((struct sockaddr_in *) sa)->sin_addr.s_addr, addrlen, errno, strerror(errno));
	}

	return val;
}

int lib_listen(int sockfd, int backlog)
{
	int val = listen(sockfd, backlog);
	if (val == -1) {
		lib_writelog(LIB_LOG_WARNING, "connect(%d, %d) call failed. error[%d] info is %s.", sockfd, backlog, errno, strerror(errno));
	}

	return val;
}

int lib_read_ipaccess_grant(lib_conf_data_t *conf)
{
	memset(lib_ipaccess_grant, 0, sizeof(lib_ipaccess_grant));
	if (NULL != conf) {
		lib_getconfstr(conf, "ip-access-grant", lib_ipaccess_grant, sizeof(lib_ipaccess_grant));
	}

	if (lib_ipaccess_grant[0] == 0) {
		strncpy(lib_ipaccess_grant, "127.0.0.1:192.168.1.*", sizeof(lib_ipaccess_grant));
	}
	return 0;
}

static int is_onepart_fit(const char *ip, const char *part)
{
	unsigned int part_low;
	unsigned int part_high;
	unsigned int from;
	char *end;
	char *tmp;
	if (ip == NULL || part == NULL) {
		return 2;
	}
	if (strncmp(part, "*", 1) == 0) {
		return 1;
	}

	from = strtoul(ip, &end, 10);

	if (((end != NULL) && (*end != '.') && (*end != '\0')) || (end == ip)) {
		return 2;
	}

	part_low = strtoul(part, &end, 10);
	if (((end != NULL) && (*end != '.') && (*end != '\0') && (*end != '-')) || (end == part)) {
		return 2;
	}

	if ((end != NULL) && (*end == '-')) {
		tmp = end+1;
		part_high = strtoul(tmp, &end, 10);
		if (((end != NULL) && (*end != '.') && (*end != '\0')) || (end == tmp)) {
			return 2;
		}
	} else {
		part_high = part_low;
	}

	if (from >= part_low && from <= part_high) {
		return 1;
	} else {
		return 0;
	}


}

	static bool is_in_subnet(const char *ip, const char *subnet)
{
	int i;
	const char *bip = ip;
	const char *bsubnet = subnet;
	int ret = 0;
	for (i = 0; i < 4; ++i) {
		if (bip == NULL || bsubnet == NULL) {
			return false;
		}

		ret = is_onepart_fit(bip, bsubnet);
		if (ret == 0) {
			return false;
		} else if (ret == 2) {
			return false;
		}

		if (i < 3) {
			bip = strchr(bip, '.');
			bsubnet = strchr(bsubnet, '.');

			if (bip != NULL) ++bip;
			if (bsubnet != NULL) ++bsubnet;
		}
	}

	return true;
}

static bool is_ip_allowed(const char *ip, char *pattern)
{
	char subnet[256];
	const char *bsnet = pattern;
	const char *end;
	if (bsnet == NULL || bsnet[0] == 0) {
		return true;
	}

	while (bsnet != NULL || bsnet[0] != 0) {
	
		end = strchr(bsnet, ':');
		if (end != NULL) {
			if (end - bsnet > (int)sizeof(subnet)-1) {
				bsnet = end+1;
				continue;
			}
			//build a sub net
			memcpy(subnet, bsnet, (size_t)(end - bsnet));
			subnet[end-bsnet] = '\0';

			if (is_in_subnet(ip, subnet)) {
				return true;
			}
			bsnet = end+1;
		} else {
			if (is_in_subnet(ip, bsnet)) {
				return true;
			}
			bsnet = NULL;
		}

	}
	return false;
}

int lib_accept(int sockfd, struct sockaddr *sa, socklen_t *addrlen)
{
	int connfd = 0;
again:
	connfd = accept(sockfd, sa, addrlen);
	if (connfd < 0) {
#ifdef EPROTO
		if (errno == EPROTO || errno == ECONNABORTED) {
#else 
		if (errno == ECONNABORTED) {
#endif 
		goto again;
		}
	} else {
		lib_writelog(LIB_LOG_WARNING, "accept(%d) call failed. error[%d]. info is %s.", sockfd, errno, strerror(errno));
		return -1;
	}

	if (lib_ipaccess_grant == NULL || lib_ipaccess_grant[0] == 0) {
		return connfd;
	}

	//if ipaccess-grant is exist 
	struct sockaddr_in peeraddr;
	struct sockaddr_in localaddr;
	socklen_t adrlen = sizeof(sockaddr_in);
	char ip_net[INET_ADDRLEN];
	memset(ip_net, 0, sizeof(ip_net));

	int peer = 0;
	peer = lib_getpeername(connfd, (struct sockaddr *)&peeraddr, &adrlen);
	if (peer < 0) {
		close(connfd);
		return -1;
	}

	adrlen = sizeof(sockaddr_in);
	lib_getsockname(connfd, (struct sockaddr *)&localaddr, &adrlen);

	struct in_addr in_val;
	in_val.s_addr = peeraddr.sin_addr.s_addr;
	inet_ntop(AF_INET, &in_val, ip_net, INET_ADDRLEN);

	if (ip_net[0] != 0) {
		if (!is_ip_allowed(ip_net, lib_ipaccess_grant)) {
			goto close_connect;
		}
	} else {
		goto close_connect;
	}

	return connfd;
close_connect:
	close(connfd);
	lib_writelog(LIB_LOG_WARNING, "When server listened on port %d a client from ip %s try to connect." , ntohs(localaddr.sin_port), ip_net);
	return -1;
}

int lib_getsockname(int sockfd, struct sockaddr *localaddr, socklen_t *addrlen)
{
	int val = getsockname(sockfd, localaddr, addrlen);
	if (val == -1) {
		lib_writelog(LIB_LOG_WARNING, "getsockname(%d) call failed. error[%d]. info is %s.", sockfd, errno, strerror(errno));
	}

	return val;
}

int lib_getpeername(int sockfd, struct sockaddr *peeraddr, socklen_t *addrlen)
{
	int val = getpeername(sockfd, peeraddr, addrlen);
	if (val == -1) {
		lib_writelog(LIB_LOG_WARNING, "getpeername(%d) call failed. error[%d]. info is %s.", sockfd, errno, strerror(errno));
	}

	return val;
}

int lib_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
	int val = setsockopt(sockfd, level, optname, optval, optlen);
	if (val == -1) {
		lib_writelog(LIB_LOG_WARNING, "setsockopt(%d, %d, %d) call failed. error is %d. info is %s.", sockfd, level, optname, errno, strerror(errno));
	}

	return val;
}

int lib_getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)
{
	int val = getsockopt(sockfd, level, optname, optval, optlen);
	if (val == -1) {
		lib_writelog(LIB_LOG_WARNING, "getsockopt(%d, %d, %d) call failed. error is %d. info is %s.", sockfd, level, optname, errno, strerror(errno));
	}

	return val;
}

int lib_select(int maxfd, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
	int val;
again:
	val = select(maxfd, readfds, writefds, exceptfds, timeout);
	if (val < 0) {
		if (errno == EINTR) {
			if (timeout != NULL) {
//				lib_writelog(LIB_LOG_WARNING,"select() call error. error is EINTR. %d:%d", timeout->tv_sec, timeout->tv_usec);
			}
			goto again;
		}
		lib_writelog(LIB_LOG_WARNING, "select() call error. error[%d] info is %s.", errno, strerror(errno));
	}

	if (val == 0) {
		errno = ETIMEDOUT;
	}

	return val;
}

//#define BIG_ENDIAN 1
//#define LITTLE_ENDIAN 0
extern int lib_gethostbyteorder()
{
	union {
		short s;
		char c[sizeof(short)];
	} un;
	un.s = 0x0102;
	if (sizeof(short) == 2) {
		if (un.c[0] == 1 && un.c[1] == 2) {
			return BIG_ENDIAN;
		} else if (un.c[0] == 2 && un.c[1] == 1) {
			return LITTLE_ENDIAN;
		} else {
			return -1;
		}
	} else {
		return -1;
	}
}

int lib_opsockopt_rcvbuff(int sockfd, void *optval, char mode)
{
	int val;
	int oplen = sizeof(int);
	if (sockfd < 0 || (mode != LIB_GET || mode != LIB_SET)) {
		return -1;
	}

	switch(mode) {
		case LIB_GET:
			val = lib_getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, optval, (socklen_t*) &oplen);
			break;
		case LIB_SET:
			val = lib_setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, optval, (socklen_t) oplen);
			break;
	}

	return val;
}

int lib_opsockopt_sndbuff(int sockfd, void *optval, char mode)
{
	int val;
	int oplen = sizeof(int);
	if (sockfd < 0 || (mode != LIB_GET && mode != LIB_SET)) {
		return -1;
	}

	switch(mode) {
		case LIB_GET:
			val = lib_getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, optval, (socklen_t *)&oplen);
			break;
		case LIB_SET:
			val = lib_setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, optval, (socklen_t)oplen);
			break;
	}

	return val;
}

int lib_opsockopt_ruseaddr(int sockfd, int *optval, char mode)
{
	int val;
	int oplen = sizeof(int);
	if (sockfd < 0 || (mode != LIB_GET && mode != LIB_SET)) {
		return -1;
	}

	switch(mode) {
		case LIB_GET:
			val = lib_getsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *) optval, (socklen_t *) &oplen);
		case LIB_SET:
			val = lib_setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *) optval, (socklen_t ) &oplen);
			break;
	}

	return val;
}

#if SO_REUSEPORT
int lib_opsockopt_ruseport(int sockfd, int *optval, char mode)
{
	int val;
	int oplen = sizeof(int);
	if (sockfd < 0 || (mode != LIB_GET && mode != LIB_SET)) {
		return -1;
	}

	switch(mode) {
		case LIB_GET:
			val = lib_getsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (void *) optval, (socklen_t *) &oplen);
		case LIB_SET:
			val = lib_setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (void *) optval, (socklen_t ) &oplen);
			break;
	}

	return val;
}
#endif


int lib_opsockopt_keepalive(int sockfd, int *op, char mode)
{
	int val;
	int oplen = sizeof(int);
	if (sockfd < 0 || (mode != LIB_GET && mode != LIB_SET)) {
		return -1;
	}

	switch(mode) {
		case LIB_GET:
			val = lib_getsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *) op, (socklen_t *)&oplen);
			break;
		case LIB_SET:
			val = lib_setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *) op, (socklen_t) oplen);
			break;
	}

	return val;
}


int lib_opsockopt_rcvtimeo(int sockfd, struct timeval *tv, char mode)
{
	int val;
	int oplen = sizeof(int);
	if (sockfd < 0 || (mode != LIB_GET && mode != LIB_SET)) {
		return -1;
	}

	switch(mode) {
		case LIB_GET:
			val = lib_getsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, tv, (socklen_t *)&oplen);
			break;
		case LIB_SET:
			val = lib_setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, tv, (socklen_t) oplen);
			break;
	}

	return val;
}

int lib_opsockopt_sndtimeo(int sockfd, struct timeval *tv, char mode)
{
	int val;
	int oplen = sizeof(int);
	if (sockfd < 0 || (mode != LIB_GET && mode != LIB_SET)) {
		return -1;
	}

	switch(mode) {
		case LIB_GET:
			val = lib_getsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, tv, (socklen_t *)&oplen);
			break;
		case LIB_SET:
			val = lib_setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, tv, (socklen_t) oplen);
			break;
	}

	return val;
}

int lib_fillsa4(struct sockaddr_in *sin, char *ip, int port)
{
	bzero(sin, sizeof(struct sockaddr_in));
	sin->sin_family = AF_INET;
	if (ip == NULL) {
		sin->sin_addr.s_addr = htonl(INADDR_ANY);
	} else {
		if ((sin->sin_addr.s_addr == inet_addr(ip)) == INADDR_NONE) {
			return -1;
		}
	}

	sin->sin_port = htons((unsigned short)port);
	return 0;
}

int lib_sclose(int sockfd)
{
	return close(sockfd);
}

int lib_tcplisten(int port, int queue)
{
	int listenfd;
	const int on = 1;
	struct sockaddr_in sin;

	if ((listenfd = lib_socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		return -1;
	}

	lib_setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	bzero(&sin, sizeof(sin));
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_family = AF_INET;
	sin.sin_port = htons((unsigned short)port);
	if (lib_bind(listenfd, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
		lib_sclose(listenfd);
		return -1;
	}

	(queue <= 0) ? queue = 5 : queue;
	
	if (lib_listen(listenfd, queue) < 0) {
		lib_sclose(listenfd);
		return -1;
	}

	return listenfd;
}

static int lib_xconnecto_tv(int sockfd, const struct sockaddr *saptr, socklen_t socklen, struct timeval *tv, int isclose)
{
	int flags;
	int n, error;
	socklen_t len;
	fd_set rset, wset;

	if (sockfd <= 0 || saptr == NULL) {
		if (sockfd > 0) {
			lib_sclose(sockfd);
		}
		return -1;
	}

	error = 0;
	flags = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

	n = connect(sockfd, saptr, socklen);

	if (n < 0) {
		if (errno != EINPROGRESS) {
			lib_writelog(LIB_LOG_WARNING, "connect(%d, <%d, %d, %d>, %d) call failed. error is %d. info is %s.", sockfd, 
					((struct sockaddr_in *)saptr)->sin_family,
				    ((struct sockaddr_in *)saptr)->sin_addr.s_addr,
				    ((struct sockaddr_in *)saptr)->sin_port, socklen, errno, strerror(errno));
			if (isclose) {
				lib_sclose(sockfd);
			}
			return -1;
		}
	}

	if (n == 0) {
		goto done;
	}

	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);

	wset = rset;

	n = lib_select(sockfd+1, &rset, &wset, NULL, tv);

	if (n == 0) {
		lib_sclose(sockfd);
		errno == ETIMEDOUT;
		return -1;
	}

	if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) {
		len = sizeof(error);
		if (lib_getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
			if (isclose) {
				lib_sclose(sockfd);
			}
			return -1;
		}
	} else {
		if (isclose) {
			lib_sclose(sockfd);
		}
		return -1;
	}
done:
	fcntl(sockfd, F_SETFL, flags);
	if (error) {
		lib_sclose(sockfd);
		errno = error;
		return -1;
	}

	return 0;
}

int lib_connecto_sclose_ms(int sockfd, const struct sockaddr *saptr, socklen_t socklen, int msecs)
{
	if (msecs <= 0) {
		return lib_xconnecto_tv(sockfd, saptr, socklen, NULL, 1);
	} else {
		struct timeval tv;
		tv.tv_sec = msecs/1000;
		tv.tv_usec = (msecs%1000)*1000;
		return lib_xconnecto_tv(sockfd, saptr, socklen, &tv, 1);
	}

	return 0;
}


int lib_connecto_ms(int sockfd, const struct sockaddr *saptr, socklen_t socklen, int msecs)
{
	if (msecs <= 0) {
		return lib_xconnecto_tv(sockfd, saptr, socklen, NULL, 0);
	} else {
		struct timeval tv;
		tv.tv_sec = msecs/1000;
		tv.tv_usec = (msecs%1000)*1000;
		return lib_xconnecto_tv(sockfd, saptr, socklen, &tv, 0);
	}

	return 0;
}

int lib_tcpconnecto_ms(char *host, int port, int msecs)
{
	return -1;
}

int lib_tcpdomainlisten(char *path, int queue)
{
	int sockfd;
	struct sockaddr_un servaddr;
	
	if (NULL == path || path[0] == 0) {
		return -1;
	}

	unlink(path);

	sockfd = lib_socket(AF_LOCAL, SOCK_STREAM, 0);
	if (sockfd == -1) {
		return -1;
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strncpy(servaddr.sun_path, path, 100);

	if (lib_bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		lib_sclose(sockfd);
		return -1;
	}

	queue <= 0 ? queue = 5 : queue;
	if (lib_listen(sockfd,queue) < 0) {
		lib_sclose(sockfd);
		return -1;
	}

	return sockfd;
}

int lib_tcpdomianconnect(char *path)
{
	int sockfd;
	struct sockaddr_un clientaddr;
	if ((sockfd = lib_socket(AF_LOCAL, SOCK_STREAM, 0)) == -1) {
		return -1;
	}
	bzero(&clientaddr, sizeof(clientaddr));
	clientaddr.sun_family = AF_LOCAL;
	strncpy(clientaddr.sun_path, path, 100);
	if (lib_connect(sockfd, (struct sockaddr *) &clientaddr, sizeof(clientaddr)) < 0) {
		lib_sclose(sockfd);
		return -1;
	}

	return sockfd;
}

int lib_recv(int sockfd, void *buf, size_t len, int flags)
{
	int val = recv(sockfd, buf, len, flags);
	if (val == -1) {
		lib_writelog(LIB_LOG_WARNING, "recv(%d, %u, %d) call failed. error[%d] info is %s.", sockfd, len, flags, errno, strerror(errno));
		return -1;
	}

	return val;
}

int lib_send(int sockfd, void *buf, size_t len, int flags)
{
	int val = send(sockfd, buf, len, flags);
	if (val == -1) {
		lib_writelog(LIB_LOG_WARNING, "send(%d, %u, %d) call failed. error[%d] info is %s.", sockfd, len, flags, errno, strerror(errno));
		return -1;
	}

	return val;
}

static void cnvt_ms_timeval(const int msec, struct timeval *tv)
{
	if (tv == NULL) {
		return;
	}
	tv->tv_sec = msec/1000;
	tv->tv_usec = (msec % 1000) * 1000;
}

extern int lib_sreadable_ms(int fd, int mseconds)
{
	fd_set fs;
	struct timeval tv;

	FD_ZERO(&fs);
	FD_SET(fd, &fs);
	if (mseconds >= 0) {
		cnvt_ms_timeval(mseconds, &tv);
		return (lib_select(fd+1, &fs, NULL, NULL, &tv));
	} else {
		return (lib_select(fd+1, &fs, NULL, NULL, NULL));
	}
}

int lib_sreadable_tv(int fd, struct timeval *tv)
{
	fd_set fs;

	FD_ZERO(&fs);
	FD_SET(fd, &fs);

	return (lib_select(fd+1, &fs, NULL, NULL, tv));
}

int lib_swriteable_ms(int fd, struct timeval *tv)
{
	fd_set fs;
	FD_ZERO(&fs);
	FD_SET(fd, &fs);

	return (lib_select(fd+1, NULL, &fs, NULL, tv));
}

int lib_reado_ms(int sockfd, void *ptrl, size_t nbytes, int msecs)
{
	int err;
	err = lib_sreadable_ms(sockfd, msecs);
	
	if (err < 0) {
		return -3;
	} else if (err == 0) {
		return -2;
	} else {
		return read(sockfd, ptrl, nbytes);
	}
}

int lib_reado_tv(int sockfd, void *ptrl, size_t nbytes, struct timeval *tv)
{
	int err;
	err = lib_sreadable_tv(sockfd, tv);
	if (err < 0) {
		return -3;
	} else if (err == 0) {
		return -2;
	} else {
		return read(sockfd, ptrl, nbytes);
	}
}

int lib_sreado_ms_ex(int sockfd, void *ptrl, size_t nbytes, int msecs)
{
	int nread;
	int nleft = nbytes;

	char *ptr = (char *) ptrl;
	struct timeval tv;
	struct timeval *tp;
	if (msecs < 0) {
		tp = NULL;
	} else {
		cnvt_ms_timeval(msecs, &tv);
		tp = &tv;
	}

	while (nleft > 0) {
again:
		nread = lib_reado_tv(sockfd, ptr, nleft, tp);
		if (nread < 0) {
			if (nread == -1 && errno == EINTR) {
				goto again;
			} else if (nread == -2) {
				errno = ETIMEDOUT;
				return -1;
			} else if (nread == -3) {
				errno = EIO;
				return -1;
			} else {
				return -1;
			}
		} else if (nread == 0) {
			break;
		} else {
			ptr += nread;
			nleft -= nread;
		}
	}

	return (int)(nbytes-nleft);
}

static int lib_swriteable_tv(int fd, struct timeval *tv)
{
	fd_set fs;
	FD_ZERO(&fs);
	FD_SET(fd, &fs);
	
	return (lib_select(fd+1, NULL, &fs, NULL, tv));
}

static int lib_writeo_tv(int fd, void *ptr, size_t nbytes, struct timeval *tv)
{
	int err;
	err = lib_swriteable_tv(fd, tv);
	if (err < 0) {
		return -1;
	} else if (err == 0) {
		return -2;
	}

	return write(fd, ptr, nbytes);
}

int lib_getsockflag(int sockfd)
{
	int flag = -1;
	while ((flag = fcntl(sockfd, F_GETFL, 0)) == -1) {
		if (errno == EINTR) {
			continue;
		}

		lib_writelog(LIB_LOG_WARNING, "fcntl(%d, F_GETFL) call failed. error is %d info is %s.", sockfd, errno, strerror(errno));
		return -1;
	}

	return flag;
}

int lib_setsocktoblock(int sockfd)
{
	int val = 0;
	int flags, flagt;
	flags = lib_getsockflag(sockfd);
	if (flags == -1) {
		return -1;
	}

	flagt = flags;
	if (!(flagt & O_NONBLOCK)) {
		return 0;
	}

	flags &= ~O_NONBLOCK;
	while ((val = fcntl(sockfd, F_SETFL, flags)) == -1) {
		if (errno == EINTR) {
			continue;
		}
		lib_writelog(LIB_LOG_WARNING, "fcntl(%d, F_SETFL, %d) call failed. error[%d] info is %s.", sockfd, flags, errno, strerror(errno));
		return -1;
	}

	return val;
}

int lib_setsocktononblock(int sockfd)
{
	int val = 0;
	int flags, flagt;
	flags = lib_getsockflag(sockfd);
	if (flags == -1) {
		return -1;
	}

	flagt = flags;

	if (flagt & O_NONBLOCK) {
		return 0;
	}

	flags |= O_NONBLOCK;
	while ((val = fcntl(sockfd, F_SETFL, flags)) == -1) {
		if (errno == EINTR) {
			continue;
		}

		lib_writelog(LIB_LOG_WARNING, "fcntl(%d, F_SETFL, %d) call failed. error[%d] info is %s.", sockfd, flags, errno, strerror(errno));
		return -1;
	}

	return val;
}
int lib_swriteo_ms_ex(int sockfd, void *ptrl, size_t nbytes, int msecs)
{
	int sockflag;
	int n;
	int nleft = nbytes;
	char *ptr = (char *) ptrl;
	struct timeval tv;
	struct timeval *tp;

	if (msecs < 0) {
		tp = NULL;
	} else {
		cnvt_ms_timeval(msecs, &tv);
		tp = &tv;
	}

	sockflag = lib_getsockflag(sockfd);
	if (sockflag < 0) {
		return -1;
	}

	if (!(sockflag & O_NONBLOCK)) {
		if (lib_setsocktononblock(sockfd) < 0) {
			return -1;
		}
	}

	while (nleft > 0) {
		n = lib_writeo_tv(sockfd, ptr, nleft, tp);
		if ((n == -1) && (errno == EINTR)) {
			continue;
		}

		if (n <= 0) {
			if (n == -2) {
				errno = ETIMEDOUT;
			}

			if (!(sockflag & O_NONBLOCK)) {
				lib_setsocktoblock(sockfd);
			}

			return -1;
		}

		ptr += n;
		nleft -= n;
	}

	if (!(sockflag & O_NONBLOCK)) {
		lib_setsocktoblock(sockfd);
	}

	return nbytes-nleft;
}


int lib_shutdown(int fd, int howto)
{
	int val = 0;
	if ((val = shutdown(fd, howto)) == -1) {
		lib_writelog(LIB_LOG_WARNING, "shutdown(%d, %d) call failed. error[%d] info is %s", fd, howto, errno, strerror(errno));
	}

	return val;
}

int lib_gethostbyname_r(const char *hostname, struct hostent *result_buf, char *buf, int buflen, struct hostent **result, int *h_err) 
{
	int val = 0;
	*h_err = 0;
	*result = NULL;
	if ((val = gethostbyname_r(hostname, result_buf, buf, (size_t)buflen, result, h_err)) != 0) {
		lib_writelog(LIB_LOG_WARNING, "gethostbyname_r(%s) call failed. error[%d] info is %s", hostname, errno, strerror(errno));
		return -1;
	}

	if (*h_err != 0 || *result == NULL) {
		lib_writelog(LIB_LOG_WARNING, "gethostbyname_r(%s) call failed. return value is %d. error[%d] info is %s", hostname, val, *h_err, hstrerror(*h_err));
		return -1;
	}

	return 0; //return val ?

}

int lib_getservbyname_r(const char *name, const char *proto, struct servent *result_buf, char *buf, size_t buflen, struct servent **result)
{
	int val = getservbyname_r(name, proto, result_buf, buf, buflen, result);
	if (val != 0) {
		errno = val;
		lib_writelog(LIB_LOG_WARNING, "getservbyname_r(%s, %s) call failed. error[%d] info is %s.", name, proto, errno, strerror(errno));
	}

	return val;
}

int lib_getservbyport_r(int port, const char *proto, struct servent *result_buf, char *buf, size_t buflen, struct servent **result)
{
	int val = getservbyport_r(port, proto, result_buf, buf, buflen, result);
	if (val != 0) {
		errno = val;
		lib_writelog(LIB_LOG_WARNING, "getservbyport(%d, %s) call failed. error[%d] info is %s.", port, proto, errno, strerror(errno));
	}

	return val;
}
/*
int lib_gethostbyname_r(const char *name, struct hostent *result_buf, char *buf, int buflen, struct hostent **result, int *h_err)
{
	int val = 0;
	*h_err = 0;
	*result = NULL;
	if ((val = gethostbyname_r(name, result_buf, buf, buflen, result)) != 0) {
		lib_writelog(LIB_LOG_WARNING, "gethostbyname_r(%s) call failed. return value is %d. error[%d] info is %s.", name, val, *h_err, strerror(*h_err));
		return -1;
	}

	if ((*h_err != 0) || *result == NULL) {
		lib_writelog(LIB_LOG_WARNING, "gethostbyname_r(%s) call failed return value is %d. error[%d] info is %s.", name, val, *h_err, strerror(*h_err));
		return -1;
	}

	return 0;
}
*/

int lib_gethostipbyname_r(const char *name, struct sockaddr_in *sin, int mode, char *asc_str)
{
	struct hostent result_buf;
	struct hostent *result;
	int err;
	char buf[8192];
	
	if (name == NULL || strlen(name) == 0) {
		return -1;
	}

	if (lib_gethostbyname_r(name, &result_buf, buf, sizeof(buf), &result, &err) == -1) {
		h_errno = err;
		return -2;
	}

	if (mode == ASCII) {
		switch (result_buf.h_addrtype) {
#ifdef AF_INET6
			case AF_INET6:
				inet_ntop(result_buf.h_addrtype, result_buf.h_addr, asc_str, INET6_ADDRSTRLEN);
				break;
#endif
			case AF_INET:
				inet_ntop(result_buf.h_addrtype, result_buf.h_addr, asc_str, INET_ADDRSTRLEN);
				break;
			default:
				break;
		}
	} else {
		//bzero(sin, sizeof(sin));//maybe some wrong
		bzero(sin, sizeof(struct sockaddr_in));//myself
		memcpy(&sin->sin_addr.s_addr, result_buf.h_addr, sizeof(sin->sin_addr.s_addr));
	}
	return 0;
}

int lib_gethostallipbyname_r(const char *name, struct sockaddr_in *sin, int mode, char *asc_str, sip_group *sip)
{
	struct hostent result_buf;
	struct hostent *result;
	int err;
	char buf[8192];
	
	if (name == NULL || strlen(name) == 0) {
		return -1;
	}

	if (lib_gethostbyname_r(name, &result_buf, buf, sizeof(buf), &result, &err) == -1) {
		h_errno = err;
		return -2;
	}

	if (mode == ASCII) {
		switch (result_buf.h_addrtype) {
#ifdef AF_INET6
			case AF_INET6:
				inet_ntop(result_buf.h_addrtype, result_buf.h_addr, asc_str, INET6_ADDRSTRLEN);
				break;
#endif
			case AF_INET:
				inet_ntop(result_buf.h_addrtype, result_buf.h_addr, asc_str, INET_ADDRSTRLEN);
				break;
			default:
				break;
		}
	} else {
		//bzero(sin, sizeof(sin));
		bzero(sin, sizeof(struct sockaddr_in));
		memcpy(&sin->sin_addr.s_addr, result_buf.h_addr, sizeof(sin->sin_addr.s_addr));
	}

	sip->addrtype = result_buf.h_addrtype;
	int i;
	char **pptr = result_buf.h_addr_list;
	for (i = 0; pptr[i] != NULL && i < IP_GROUP_NUM; ++i) {
		memcpy(&(sip->ip[i]), pptr[i], sizeof(sin->sin_addr.s_addr));
	}

	return 0;
}

int lib_getportbyserv(const char *name, const char *proto, int *port)
{
	if ( (name == NULL || strlen(name) == 0) || (proto == NULL || strlen(proto) == 0)) {
		return -1;
	}

	struct servent result_buf;
	struct servent **result;
	char buf[8192];

	bzero(&result, sizeof(result));

	if (lib_getservbyname_r(name, proto, &result_buf, buf, 8192, result) != 0) {
		return -1;
	}

	*port = result_buf.s_port;
	return result_buf.s_port;
}

int lib_getrecvqueuesize(int sockfd)
{
	int val = 0;
	char buf[65535];
	int flags = MSG_DONTWAIT | MSG_PEEK;
	while((val = recv(sockfd, buf, 65535, flags)) == -1 && errno == EINTR);
	return val;
}
