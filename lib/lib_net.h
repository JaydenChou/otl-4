#ifndef _LIB_NET_H_
#define _LIB_NET_H_

#include "lib_conf.h"

#define IPACCESS_GRANT_LEN 256
extern char lib_ipaccess_grant[IPACCESS_GRANT_LEN+1];

//pack
extern int lib_socket(int family, int type, int protocol);

//packk
extern int lib_connect(int sockfd, const struct sockaddr *sa, socklen_t addrlen);

//pack
extern int lib_bind(int sockfd, const struct sockaddr *sa, socklen_t addrlen);

extern int lib_listen(int sockfd, int backlog);

/**
 * @brief read the global ip access grant in config struct
 *			only those ips matching the grant string will be allowed
 * @param[in] pd_conf, the config struct
 * @param[out] the global variable lib_ipaccess_grant
 * @return : 0 always
 * @notice:
 *			the name-value pair format:
 *				ip-access-grant : grant-string
 *				grant string as:
 *					*.192.168-160.* : 127.*.*.*; ...
 *			if ip-access-grant does not exist, default string
 *				("127.0.0.1 : 192.168.1.*") will be set
 *			if the function never been called, all ip are allowed
 */
extern int lib_read_ipaccess_grant(lib_conf_data_t *conf);

//pack
extern int lib_accept(int sockfd, struct sockaddr *sa, socklen_t *addrlen);

//pack
extern int lib_getsockname(int sockfd, struct sockaddr *loacladdr, socklen_t *addrlen);

//pack
extern int lib_getpeername(int sockfd, struct sockaddr *peeraddr, socklen_t *addrlen);

//pack
extern int lib_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);

//pack
extern int lib_getsockopt(int sockfd, int level, int optname, void *optval, socklen_t optlen);

//pack -1 error 0 timeout fd# on success
extern int lib_select(int maxfd, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

// get localhost bytes order
// return 1 == Big-endian , 0 == Little-endian -1 error
extern int lib_gethostbyteorder();

#define LIB_GET 1
#define LIB_SET 0
/**
 * @breif operate the receive buff of socket
 * @param[in] sockfd : the sock id
 * @param[in/out] optval: the address which store the buffer size
 * @param[in] mode : operate mod. LIB_GET or LIB_SET
 * @return : as same as getsockopt and sockopt. pack them
 */
int lib_opsockopt_rcvbuff(int sockfd, void *optval, char mode);

/**
 * @breif operate the receive buff of socket
 * @param[in] sockfd : the sock id
 * @param[in/out] optval: the address which store the buffer size
 * @param[in] mode : operate mod. LIB_GET or LIB_SET
 * @return : as same as getsockopt and sockopt. pack them
 */
int lib_opsockopt_sndbuff(int sockfd, void *optval, char mode);

#define LIB_OPEN 1
#define LIB_CLOSE 0
/**
 * @brief Operate the switch SO_REUSEADDR. use this function you could find the state<OPEN/CLOSE> of this switch
 * @param[in] sockfd : the socket id
 * @param[in] optval : the address which store the operation. the relevant value is OPEN/CLOSE
 * @param[in] mode : operate mode LIB_GET or LIB_SET
 */
extern int lib_opsockopt_ruesaddr(int sockfd, int *optval, char mode);
#if SO_REUSEPORT
extern int lib_opsockopt_ruseport(int sockfd, int *optval, char mode);
#endif

/**
 * @brief operate the switch SO_KEEPALIVE, Use this function you could find the state<OPEN/CLOSE> of this switch
 * @param[in] sockfd : the socket id
 * @param[in] optval : the address which store the operation. the relevant value is OPEN/CLOSE
 * @param[in] mode : operate mode LIB_GET or LIB_SET
 */
extern int lib_opsockopt_keepalive(int sockfd, int *op, char mode);

/**
 * @brief operate the switch SO_RCVTIMEO. use this function you could find the current value of this option
 * @param[in] sockfd : the socket id
 * @param[in] tv : the time struct which store the time
 * @param[in] mode : operate mod LIB_GET or LIB_SET
 */
extern int lib_opsockopt_rcvtimeo(int sockfd, struct timeval *tv, char mode);

/**
 * @brief operate the switch SO_RSNDIMEO. use this function you could find the current value of this option
 * @param[in] sockfd : the socket id
 * @param[in] tv : the time struct which store the time
 * @param[in] mode : operate mod LIB_GET or LIB_SET
 */
extern int lib_opsockopt_sndtimeo(int sockfd, struct timeval *tv, char mode);

/**
 * @breif fill data struct with IPv4 <struct sockaddr_in>
 */
extern int lib_fillsa4(struct sockaddr_in *sin, char *ip, int port);

/**
 * @brief start a server on local machine at port number with ANY address queue.
 * @param[in] port : the listen port
 * @param[in] queue: the connet queue size (listen backlog param
 * @return -1 failed or fd# 
 */
extern int lib_tcplisten(int port, int queue);

/**
 * @brief a time-out version of connect() for millisecond
 *			while exit on error, close sockfd,
 * @param[in] sockfd, saptr, socklen as the same as connect
 * @param[in] msec: the over time, in second;
 */

extern int lib_connecto_sclose_ms(int sockfd, const struct sockaddr *saptr, socklen_t socklen, int msecs);

/**
 * @brief a time-out version of connect() for millisecond
 * @param[in] sockfd, saptr, socklen as the same as connect
 * @param[in] msec: the over time, in second;
 */

extern int lib_connecto_ms_ex(int sockfd, const struct sockaddr *saptr, socklen_t socklen, int msecs);

/**
 * @brief time-out version for millisecond
 *			build up a TCP connection to host at port. this is used at client part ,return the socket fd on success, or -1 on error 
 * @param[in] host : the target host
 * @param[in] port : the target port
 * @param[in] secs : the over time
 * @return fd# on success, -1 failed
 */
extern int lib_tcpconnecto_ms(char *host, int port, int msecs);

/**
 * @brief start a unix domain server on a path
 * @param[in] path : the target path
 * @param[in] queue : listen() backlog-param
 * @return fd# on success, -1 - failed
 */
extern int lib_tcpdomainlisten(char *path, int queue);


/**
 * @brief start a unix domain server on a path
 * @param[in] path : the target path
 * @return fd# on success, -1 - failed
 */
extern int lib_tcpdomainconnect(char *path);

//pack
int lib_recv(int sockfd, void *buf, size_t len, int flags);

//pack
int lib_send(int sockfd, void *buf, size_t len, int falgs);

/**
 * @brief function equal lib_sreadale(), timeout milliseconds
 * @param fd socket
 * @param mseconds
 * @return -1 error 0 timeout >0 can read
 */
extern int lib_sreadable_ms(int fd, int mseconds);

/**
 * @brief function equal lib_sreadale_tv(), timeout milliseconds
 * @param fd socket
 * @param mseconds
 * @return -1 error 0 timeout >0 can read
 */
extern int lib_sreadable_tv(int fd, struct timeval *tv);

/**
 * @brief 
 * @param[in] fd : socket fd
 * @param[in] msecond : timeout
 * @return -1 error 0 timeout  > 0 can write
 */
extern int lib_swriteable_ms(int fd, struct timeval *tv);

/**
 * @brief 
 * @param[in] fd : read socket
 * @param[in] ptrl : data buffer
 * @param[in] nbytes : the size to read
 * @param[in] msecs : timeout 
 * @return -1 failed or the size of read bytes
 */
extern int lib_sreado_ms_ex(int sockfd, void *ptrl, size_t nbytes, int msecs);

extern int lib_reado_tv(int sockfd, void *ptrl, size_t nbytes, struct timeval *tv);

/**
 * @brief 
 * @param[in] fd : read socket
 * @param[in] ptrl : data buffer
 * @param[in] nbytes : the size to read
 * @param[in] msecs : timeout 
 * @return -1 failed or the size of read bytes
 */
extern int lib_reado_ms(int sockfd, void *ptrl, size_t nbytes, int msecs);

//pack
extern int lib_scloes(int sockfd);

/**
 * @brief 
 * @param[in] fd : read socket
 * @param[in] ptrl : data buffer
 * @param[in] nbytes : the size to write
 * @param[in] msecs : timeout 
 * @return -1 failed or the size of write bytes
 */
extern int lib_swriteo_ms_ex(int sockfd, void *ptrl, size_t nbytes, int msecs);

//pack
extern int lib_shutdown(int fd, int howto);

//pack
extern int lib_gethostbyname_r(const char *hostname, struct hostent *result_buf, char *buf, int buflen, struct hostent **result, int *h_err);

//pack
extern int lib_getservbyname_r(const char *name, const char *proto, struct servent *result_buf, char *buf, size_t buflen, struct servent **result);

//pack
extern int lib_getservbyport_r(int port, const char *proto, struct servent *result_buf, char *buf, size_t buflen, struct servent **result);

//pack
extern int lib_gethostbyname_r(const char *name, struct hostent *result_buf, char *buf, int buflen, struct hostent **result, int *h_err);

#define ASCII 0
#define NET 1
/**
 * @brief get ip address of one host return the h_addr item
 * @param[in] name : host name
 * @param[in] sin : ip
 * @param[in] mode : ASCII or NET
 * @param[in] asc_str
 * @return 0 success , -1 failed -2 call error
 */
extern int lib_gethostipbyname_r(const char *name, struct sockaddr_in *sin, int mode, char *asc_str);

#define IP_GROUP_NUM 10

typedef struct {
	int addrtype;
	int ip[IP_GROUP_NUM];
} sip_group;


/**
 * @brief get ip address group of one host. return the h_addr item.
 */
extern int lib_gethostallipbyname_r(const char *name, struct sockaddr_in *sin, int mode, char *asc_str, sip_group *sip);

/**
 * @brief get port by serv
 * @return on success return the port, or return -1
 */
extern int lib_getportbyserv(const char *name, const char *proto, int *port);

/**
 * @brief get recv queue size
 */
extern int lib_getrecvqueuesize(int sockfd);
#endif
