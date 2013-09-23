#ifndef __SS_CONF_H__
#define __SS_CONF_H__

#include "lib_conf.h"
//define return value type
#define SS_CONF_SUCCESS 0				//get configure success
#define SS_CONF_DEFAULT -1				//not found the item, use default value
#define SS_CONF_LOADFAIL -2				//get configure failed
#define SS_CONF_OVERFLOW -3				//the value is out of range
#define SS_CONF_INVALID_CHARACTER -4	// there is invalid char in the value
#define SS_CONF_LOST -5					//can't find item
#define SS_CONF_CHECKSUCCESS -6			//check format success
#define SS_CONF_CHECKFAIL -7			//check format fail
#define SS_CONF_SETMULTIPLE -8			//the item is exist. another again
#define SS_CONF_NULL -9					//all

//some utils
#define SS_CONF_MAX_CLONE 4
#define SS_CONF_DEFAULT_ITEMNUM 1024
#define SS_CONF_IPMAX 128
#define SS_CONF_IPLENMAX 16
#define SS_CONF_READCONF 0

typedef unsigned int u_int;

typedef struct _ss_conf_data_t {
	lib_conf_data_t *option;		//configure item data
	lib_conf_data_t *range;			//range file configure item, if file isn't exist, the value is null
	char filename[LINE_SIZE];		//the configure file name
	FILE *conf_file;				//the file pointer
	int build;						//0 read conf , !0 create configure file
} ss_conf_data_t;

typedef struct _ss_svr_ip_t {
	u_int num;						//the number of ip on the same computer, to do with clone IP
	char name[SS_CONF_MAX_CLONE][WORD_SIZE];//IP on the one computer split by '/'
} ss_svr_ip_t;

typedef struct _ss_svr_t {			//server can support configure
	char svr_name[WORD_SIZE];		//server name
	u_int port;						//server port number
	u_int read_timeout;				//read time out
	u_int write_timeout;			//write time out
	u_int threadnum;				//thead number
	u_int connet_type;				//0 short connet , !0 long connect
	u_int server_type;				//the pool type, as ss_server configure
	u_int queue_size;				//the length of queue, when pool is cpool , it is valid
	u_int sock_size;				//the length of sock, when pool is cpool, it is valid
} ss_svr_t;

typedef struct _ss_request_svr_t {	//request server configure
	char svr_name[WORD_SIZE];		//server name;
	u_int num;						//the number of IP
	ss_svr_ip_t ip_list[SS_CONF_IPMAX];	//every ip info
	u_int port;						//port number
	u_int read_timeout;				//read timeout
	u_int write_timeout;			//write timeout
	u_int connect_timeout;			//connect timeout
	u_int max_connect;				//max connect number;
	u_int retry;					//the number of retry
	u_int connect_type;				//0 short connect, !0 long connect
	u_int linger;					//if use linger way
} ss_request_svr_t;

/**
 * @brief init configure file struct
 * @param[in] path: file path
 * @param[in] filename: file name
 * @param[in] build: SS_CONF_READCONF read configure others will create new file
 * @param[in[ num: the number of configure item
 * @return NULL fail, ss_conf_data_t pointer
 */
ss_conf_data_t* ss_conf_init(const char* path, const char *filename, const int build = SS_CONF_READCONF, const int num = SS_CONF_DEFAULT_ITEMNUM);


#endif
