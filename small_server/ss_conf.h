#ifndef __SS_CONF_H__
#define __SS_CONF_H__

#include "lib_conf.h"
#include "ss_log.h"
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
	u_int connect_type;				//0 short connet , !0 long connect
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

/**
 * @brief free configure file sturct
 * @param[in] conf: configure file struct pointer
 * @return SS_CONF_SUCCESS
 */
int ss_conf_close(ss_conf_data_t* conf);

/**
 * @brief read configuration item value in the configuration infomation structure, and examined by range file, with the buffer length limit, value is of type "char *"
 * @param[in] conf: configure struct pointer
 * @param[in] conf_name: the configure item name
 * @param[out] conf_value: the configure item value
 * @param[in] n: the max length to read
 * @param[in] comment: the configure item introduction, in order to write configure
 * @param[in] default_value: the configure item default value, it is NULL as usual, means that don't use default value
 * @return 
 * --SS_CONF_SUCCESS	success
 * --SS_CONF_DEFAULT	failed, use default value
 * --SS_CONF_OVERFLOW	the numberic type overflow
 * --SS_CONF_LOST		didn't find configure item
 * --SS_CONF_CHECKSUCCESS	range file check success
 * --SS_CONF_CHECKFAIL	range file check fail
 * --SS_CONF_SETMULTIPLE configure item repeat
 * --SS_CONF_NULL		input param invalid
 */
int ss_conf_getnstr(const ss_conf_data_t *conf, const char *name, char *value, const size_t n, const char *comment, const char *default_value = NULL);

/**
 * @brief read configuration item value in the configuration infomation structure, and examined by range file, with the buffer length limit, value is of type "int"
 * @param[in] conf: configure struct pointer
 * @param[in] conf_name: the configure item name
 * @param[out] conf_value: the configure item value
 * @param[in] comment: the configure item introduction, in order to write configure
 * @param[in] default_value: the configure item default value, it is NULL as usual, means that don't use default value
 * @return 
 * --SS_CONF_SUCCESS	success
 * --SS_CONF_DEFAULT	failed, use default value
 * --SS_CONF_OVERFLOW	the numberic type overflow
 * --SS_CONF_LOST		didn't find configure item
 * --SS_CONF_CHECKSUCCESS	range file check success
 * --SS_CONF_CHECKFAIL	range file check fail
 * --SS_CONF_SETMULTIPLE configure item repeat
 * --SS_CONF_NULL		input param invalid
 */
int ss_conf_getint(const ss_conf_data_t *conf, const char *name, int *value, const char *comment, const int *default_value = NULL);

/**
 * @brief read configuration item value in the configuration infomation structure, and examined by range file, with the buffer length limit, value is of type "uint"
 * @param[in] conf: configure struct pointer
 * @param[in] conf_name: the configure item name
 * @param[out] conf_value: the configure item value
 * @param[in] comment: the configure item introduction, in order to write configure
 * @param[in] default_value: the configure item default value, it is NULL as usual, means that don't use default value
 * @return 
 * --SS_CONF_SUCCESS	success
 * --SS_CONF_DEFAULT	failed, use default value
 * --SS_CONF_OVERFLOW	the numberic type overflow
 * --SS_CONF_LOST		didn't find configure item
 * --SS_CONF_CHECKSUCCESS	range file check success
 * --SS_CONF_CHECKFAIL	range file check fail
 * --SS_CONF_SETMULTIPLE configure item repeat
 * --SS_CONF_NULL		input param invalid
 */
int ss_conf_getuint(const ss_conf_data_t *conf, const char *name, unsigned int *value, const char *comment, const unsigned int *default_value = NULL);

/**
 * @brief read configuration item value in the configuration infomation structure, and examined by range file, with the buffer length limit, value is of type "long long"
 * @param[in] conf: configure struct pointer
 * @param[in] conf_name: the configure item name
 * @param[out] conf_value: the configure item value
 * @param[in] comment: the configure item introduction, in order to write configure
 * @param[in] default_value: the configure item default value, it is NULL as usual, means that don't use default value
 * @return 
 * --SS_CONF_SUCCESS	success
 * --SS_CONF_DEFAULT	failed, use default value
 * --SS_CONF_OVERFLOW	the numberic type overflow
 * --SS_CONF_LOST		didn't find configure item
 * --SS_CONF_CHECKSUCCESS	range file check success
 * --SS_CONF_CHECKFAIL	range file check fail
 * --SS_CONF_SETMULTIPLE configure item repeat
 * --SS_CONF_NULL		input param invalid
 */
int ss_conf_getint64(const ss_conf_data_t *conf, const char *name, long long *value, const char *comment, const long long *default_value = NULL);

/**
 * @brief read configuration item value in the configuration infomation structure, and examined by range file, with the buffer length limit, value is of type "unsigned long long"
 * @param[in] conf: configure struct pointer
 * @param[in] conf_name: the configure item name
 * @param[out] conf_value: the configure item value
 * @param[in] comment: the configure item introduction, in order to write configure
 * @param[in] default_value: the configure item default value, it is NULL as usual, means that don't use default value
 * @return 
 * --SS_CONF_SUCCESS	success
 * --SS_CONF_DEFAULT	failed, use default value
 * --SS_CONF_OVERFLOW	the numberic type overflow
 * --SS_CONF_LOST		didn't find configure item
 * --SS_CONF_CHECKSUCCESS	range file check success
 * --SS_CONF_CHECKFAIL	range file check fail
 * --SS_CONF_SETMULTIPLE configure item repeat
 * --SS_CONF_NULL		input param invalid
 */
int ss_conf_getuint64(const ss_conf_data_t *conf, const char *name, unsigned long long *value, const char *comment, const unsigned long long *default_value = NULL);

/**
 * @brief read configuration item value in the configuration infomation structure, and examined by range file, with the buffer length limit, value is of type "float"
 * @param[in] conf: configure struct pointer
 * @param[in] conf_name: the configure item name
 * @param[out] conf_value: the configure item value
 * @param[in] comment: the configure item introduction, in order to write configure
 * @param[in] default_value: the configure item default value, it is NULL as usual, means that don't use default value
 * @return 
 * --SS_CONF_SUCCESS	success
 * --SS_CONF_DEFAULT	failed, use default value
 * --SS_CONF_OVERFLOW	the numberic type overflow
 * --SS_CONF_LOST		didn't find configure item
 * --SS_CONF_CHECKSUCCESS	range file check success
 * --SS_CONF_CHECKFAIL	range file check fail
 * --SS_CONF_SETMULTIPLE configure item repeat
 * --SS_CONF_NULL		input param invalid
 */
int ss_conf_getfloat(const ss_conf_data_t *conf, const char *name, float *value, const char *comment, const float *default_value = NULL);

/**
 * @brief read configuration item value in the configuration infomation structure, and examined by range file, with the buffer length limit, value is of type "ss_svr_t"
 * @param[in] conf: configure struct pointer
 * @param[in] product_name: product name, it can be NULL
 * @param[in] module_name: the module name, it can not be NULL
 * @param[out] conf_value: the configure item value
 * @param[in] comment: the configure item introduction, in order to write configure
 * @return 
 * --SS_CONF_SUCCESS	success
 * --SS_CONF_DEFAULT	failed, use default value
 * --SS_CONF_OVERFLOW	the numberic type overflow
 * --SS_CONF_LOST		didn't find configure item
 * --SS_CONF_CHECKSUCCESS	range file check success
 * --SS_CONF_CHECKFAIL	range file check fail
 * --SS_CONF_SETMULTIPLE configure item repeat
 * --SS_CONF_NULL		input param invalid
 */
int ss_conf_getsvr(const ss_conf_data_t *conf, const char *product_name, const char *module_name, ss_svr_t *value, const char *comment);

/**
 * @brief read configuration item value in the configuration infomation structure, and examined by range file, with the buffer length limit, value is of type "ss_svr_t"
 * @param[in] conf: configure struct pointer
 * @param[in] product_name: product name, it can be NULL
 * @param[in] module_name: the module name, it can not be NULL
 * @param[out] conf_value: the configure item value
 * @param[in] comment: the configure item introduction, in order to write configure
 * @return 
 * --SS_CONF_SUCCESS	success
 * --SS_CONF_DEFAULT	failed, use default value
 * --SS_CONF_OVERFLOW	the numberic type overflow
 * --SS_CONF_LOST		didn't find configure item
 * --SS_CONF_CHECKSUCCESS	range file check success
 * --SS_CONF_CHECKFAIL	range file check fail
 * --SS_CONF_SETMULTIPLE configure item repeat
 * --SS_CONF_NULL		input param invalid
 */
int ss_conf_getreqsvr(const ss_conf_data_t *conf, const char *product_name, const char *module_name, ss_request_svr_t *value, const char *comment);

/**
 * @brief read configuration item value in the configuration infomation structure, and examined by range file, with the buffer length limit, value is of type "char *"
 * @param[in] path: configure file path
 * @param[in] filename: configure file name
 * @param[in] conf_name: the name of get
 * @param[out] conf_value: the configure item value
 * @param[in] n: the read max size
 * @param[in] default_value: when it doesn't find out! replace with default_value. if it is NULL. it means doesn't no using.
 * @return 
 * --SS_CONF_SUCCESS	success
 * --SS_CONF_DEFAULT	failed, use default value
 * --SS_CONF_OVERFLOW	the numberic type overflow
 * --SS_CONF_LOST		didn't find configure item
 * --SS_CONF_CHECKSUCCESS	range file check success
 * --SS_CONF_CHECKFAIL	range file check fail
 * --SS_CONF_SETMULTIPLE configure item repeat
 * --SS_CONF_NULL		input param invalid
 */
int ss_conf_getonenstr(const char *path, const char *filename, const char *conf_name, char *value, const size_t n, const char *default_value = NULL);

/**
 * @brief read configuration item value in the configuration infomation structure, and examined by range file, with the buffer length limit, value is of type "int"
 * @param[in] path: configure file path
 * @param[in] filename: configure file name
 * @param[in] conf_name: the name of get
 * @param[out] conf_value: the configure item value
 * @param[in] default_value: when it doesn't find out! replace with default_value. if it is NULL. it means doesn't no using.
 * @return 
 * --SS_CONF_SUCCESS	success
 * --SS_CONF_DEFAULT	failed, use default value
 * --SS_CONF_OVERFLOW	the numberic type overflow
 * --SS_CONF_LOST		didn't find configure item
 * --SS_CONF_CHECKSUCCESS	range file check success
 * --SS_CONF_CHECKFAIL	range file check fail
 * --SS_CONF_SETMULTIPLE configure item repeat
 * --SS_CONF_NULL		input param invalid
 */
int ss_conf_getoneint(const char *path, const char *filename, const char *conf_name, int *value, const int *default_value = NULL);

/**
 * @brief read configuration item value in the configuration infomation structure, and examined by range file, with the buffer length limit, value is of type "uint"
 * @param[in] path: configure file path
 * @param[in] filename: configure file name
 * @param[in] conf_name: the name of get
 * @param[out] conf_value: the configure item value
 * @param[in] default_value: when it doesn't find out! replace with default_value. if it is NULL. it means doesn't no using.
 * @return 
 * --SS_CONF_SUCCESS	success
 * --SS_CONF_DEFAULT	failed, use default value
 * --SS_CONF_OVERFLOW	the numberic type overflow
 * --SS_CONF_LOST		didn't find configure item
 * --SS_CONF_CHECKSUCCESS	range file check success
 * --SS_CONF_CHECKFAIL	range file check fail
 * --SS_CONF_SETMULTIPLE configure item repeat
 * --SS_CONF_NULL		input param invalid
 */
int ss_conf_getoneuint(const char *path, const char *filename, const char *conf_name, unsigned int *value, const unsigned int *default_value = NULL);

/**
 * @brief read configuration item value in the configuration infomation structure, and examined by range file, with the buffer length limit, value is of type "int 64"
 * @param[in] path: configure file path
 * @param[in] filename: configure file name
 * @param[in] conf_name: the name of get
 * @param[out] conf_value: the configure item value
 * @param[in] default_value: when it doesn't find out! replace with default_value. if it is NULL. it means doesn't no using.
 * @return 
 * --SS_CONF_SUCCESS	success
 * --SS_CONF_DEFAULT	failed, use default value
 * --SS_CONF_OVERFLOW	the numberic type overflow
 * --SS_CONF_LOST		didn't find configure item
 * --SS_CONF_CHECKSUCCESS	range file check success
 * --SS_CONF_CHECKFAIL	range file check fail
 * --SS_CONF_SETMULTIPLE configure item repeat
 * --SS_CONF_NULL		input param invalid
 */
int ss_conf_getoneint64(const char *path, const char *filename, const char *conf_name, long long *value, const long long *default_value = NULL);

/**
 * @brief read configuration item value in the configuration infomation structure, and examined by range file, with the buffer length limit, value is of type "unsigned long long"
 * @param[in] path: configure file path
 * @param[in] filename: configure file name
 * @param[in] conf_name: the name of get
 * @param[out] conf_value: the configure item value
 * @param[in] default_value: when it doesn't find out! replace with default_value. if it is NULL. it means doesn't no using.
 * @return 
 * --SS_CONF_SUCCESS	success
 * --SS_CONF_DEFAULT	failed, use default value
 * --SS_CONF_OVERFLOW	the numberic type overflow
 * --SS_CONF_LOST		didn't find configure item
 * --SS_CONF_CHECKSUCCESS	range file check success
 * --SS_CONF_CHECKFAIL	range file check fail
 * --SS_CONF_SETMULTIPLE configure item repeat
 * --SS_CONF_NULL		input param invalid
 */
int ss_conf_getoneuint64(const char *path, const char *filename, const char *conf_name, unsigned long long *value, const unsigned long long *default_value = NULL);

/**
 * @brief read configuration item value in the configuration infomation structure, and examined by range file, with the buffer length limit, value is of type "float"
 * @param[in] path: configure file path
 * @param[in] filename: configure file name
 * @param[in] conf_name: the name of get
 * @param[out] conf_value: the configure item value
 * @param[in] default_value: when it doesn't find out! replace with default_value. if it is NULL. it means doesn't no using.
 * @return 
 * --SS_CONF_SUCCESS	success
 * --SS_CONF_DEFAULT	failed, use default value
 * --SS_CONF_OVERFLOW	the numberic type overflow
 * --SS_CONF_LOST		didn't find configure item
 * --SS_CONF_CHECKSUCCESS	range file check success
 * --SS_CONF_CHECKFAIL	range file check fail
 * --SS_CONF_SETMULTIPLE configure item repeat
 * --SS_CONF_NULL		input param invalid
 */
int ss_conf_getonefloat(const char *path, const char *filename, const char *conf_name, float *value, const float *default_value = NULL);

/**
 * @brief read configuration item value in the configuration infomation structure, and examined by range file, with the buffer length limit, value is of type "ss_svr_t"
 * @param[in] path: configure file path
 * @param[in] filename: configure file name
 * @param[in] conf_name: the name of get
 * @param[out] conf_value: the configure item value
 * @param[in] default_value: when it doesn't find out! replace with default_value. if it is NULL. it means doesn't no using.
 * @return 
 * --SS_CONF_SUCCESS	success
 * --SS_CONF_DEFAULT	failed, use default value
 * --SS_CONF_OVERFLOW	the numberic type overflow
 * --SS_CONF_LOST		didn't find configure item
 * --SS_CONF_CHECKSUCCESS	range file check success
 * --SS_CONF_CHECKFAIL	range file check fail
 * --SS_CONF_SETMULTIPLE configure item repeat
 * --SS_CONF_NULL		input param invalid
 */
int ss_conf_getonesvr(const char *path, const char *filename, const char *product_name, const char *module_name, ss_svr_t *value);

/**
 * @brief read configuration item value in the configuration infomation structure, and examined by range file, with the buffer length limit, value is of type "ss_request_svr_t"
 * @param[in] path: configure file path
 * @param[in] filename: configure file name
 * @param[in] conf_name: the name of get
 * @param[out] conf_value: the configure item value
 * @param[in] default_value: when it doesn't find out! replace with default_value. if it is NULL. it means doesn't no using.
 * @return 
 * --SS_CONF_SUCCESS	success
 * --SS_CONF_DEFAULT	failed, use default value
 * --SS_CONF_OVERFLOW	the numberic type overflow
 * --SS_CONF_LOST		didn't find configure item
 * --SS_CONF_CHECKSUCCESS	range file check success
 * --SS_CONF_CHECKFAIL	range file check fail
 * --SS_CONF_SETMULTIPLE configure item repeat
 * --SS_CONF_NULL		input param invalid
 */
int ss_conf_getonereqsvr(const char *path, const char *filename, const char *product_name, const char *module_name, ss_request_svr_t *value);

#endif
