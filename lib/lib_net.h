#ifndef _LIB_NET_H_
#define _LIB_NET_H_

#define IPACCESS_GRANT_LEN 256
extern char lib_ipaccess_grant[IPACCESS_GRANT_LEN+1];

//pack
extern int lib_socket(int family, int type, int protocol);


#endif
