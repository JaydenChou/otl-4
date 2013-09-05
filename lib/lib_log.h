#ifndef _LIB_LOG_H
#define _LIB_LOG_H

#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <limits.h>
#include <cstring>
#include <sys/stat.h>
#include "lib_def.h"

#define LOG_FILE_NUM 2048

#define MAX_FILENAME_LEN 1024
#define MAX_SELF_DEF_LOG 8

#define ERROR_NOSPACE	-1
#define ERROR_OPENFILE	-2
#define ERROR_EVENT		-3
#define ERROR_FILEFULL	-4
//Lib log level
#define LIB_LOG_NONE	0
#define LIB_LOG_FATAL	0x01	/** fatal errors */
#define LIB_LOG_WARNING	0x02	/** some exceptions and maybe lead to some potential erros */
#define LIB_LOG_NOTICE	0x04	/** normal informations */
#define LIB_LOG_TRACE	0x08	/** program run trace */
#define LIB_LOG_DEBUG	0x10	/** for program debug */
#define LIB_LOG_ALL		0xff	/** all the kind */

//for lib_logstat_t
#define LIB_LOGTTY	0x02	/** output the log to the tty(stderr) */
#define LIB_LOGNEWFILE 0x08	/** create a new file, make every threads output the log to different files */
#define LIB_LOGSIZESPLIT 0x10 /** in order to split the log. so limit the size of log file */

struct lib_logstat_t {
	int level;			/** the log level 0 - 15 */
	int syslog_level;	/** output the syslog level 0 - 15 */
	int spec;			/** the flag extend, LIB_LOGTTY or LIB_LOGNEWFILE */
};

//for lib_file_t
#define LIB_FILE_TRUNCATE	0x01
#define LIB_FILE_FULL		0x02
struct lib_file_t {
	FILE* fp;			/** file handle */
	int flag;			/** flag LIB_FILE_TRUNCATE or LIB_FILE_FULL */
	int ref_cnt;		/** reference count */
	int max_size;		/** the file max length */
	pthread_mutex_t file_lock; /** write file lock */
	char file_name[MAX_FILENAME_LEN+1];	/** file name */
};

struct lib_log_self_t {
	char name[MAX_SELF_DEF_LOG][PATH_SIZE];	/** custom log file name, the system will append .sdf as suffix */
	char flags[MAX_SELF_DEF_LOG];			/** decide the log should output, 1 true, 0 false; */
	int log_number;							/** the number of custom log file, 0 is defined forbidding output custom log file */
};	

struct lib_log_t {
	char used;		/** 0 unused 1 used */
	lib_file_t *pf;	/** log */
	lib_file_t *pf_wf;	/** log.wf */
	pthread_t tid;	/** thread id */
	int mask;		/**to record the events mask */
	int syslog_mask;	/**output system log events mask */
	int log_spec;	/** LIB_LOGTTY or LIB_LOGNEWFILE */
	lib_file_t *spf[MAX_SELF_DEF_LOG]; /** custom log file handle */
};

/**
 * @brief open log file(log), and initialise log object
 * @param [in] log_path: log file direcotry
 * @param [in] log_procname: the log file prefix. if filename is include "_", it will be truncate the string before "-"
 * @param [in] log_stat: the relevant param about log file
 * @param [in] maxlen: the log file max length(MB)
 * @param [in] self: custom log
 * @return 0 success -1 fail
 * @note call lib_closelog to release resources when exit
 */
extern int lib_openlog(const char* log_path, const char* log_procname, lib_logstat_t *log_stat, int maxlen, lib_log_self_t* self = NULL);

/**
 * @brief output designated log level string to log file
 * @param [in] level: log level
 * @param [in] fmt: the format string
 * @param [in] ...: args list
 * @return 0 success -1 fail
 * @note max length 2048 Bytes
 */
extern int lib_writelog(const int level, const char* fmt, ...) __attribute__ ((format (printf,2,3)));

/**
 * @brief close log file and release resource
 * @param [in] iserr: record the status end -- 0 normal !0 abnormal
 * @return 0 success -1 fail
 */
extern int lib_closelog(int iserr);

/**
 * @brief open log file for the thread
 * @param [in] threadname for print the thread name
 * @param [in] log_stat: the revelant params for log file
 * @param [in] self: custom log
 * @return 0 success -1 fail
 * @note call lib_closelog_r to release resource
 */
extern int lib_openlog_r(const char* threadname, lib_logstat_t* log_stat, lib_log_self_t* self = NULL);

/**
 * @brief close log file for the thread
 * @param [in] iserr, record the status end -- 0 normal !0 abnormal
 * @return 0 success -1 fail
 */
extern int lib_closelog_r(int iserr);


#endif

