#ifndef __SS_LOG_H__
#define __SS_LOG_H__

#include <pthread.h>
#include "lib_log.h"

//max userinfo length
#define NOTICE_INFO_MAXLEN 2048
//special and invisiable separating(new line sign LF)
#define SS_SPACE '\10'

enum ss_notice_type {
	SS_LOG_ZERO = 0, //this one is not used, and should be thefirsr
	SS_LOG_LOGID,	//set the logid corresponding command
	SS_LOG_PROCTIME,	//set the process time corresponding command
	SS_LOG_REQIP,	//set the request IP corresponding command
	SS_LOG_REQSVR,	//set the request server name corresponding command
	SS_LOG_SVRNAME,	//set current server name
	SS_LOG_CMDNO,	//set the request command corresponding command
	SS_LOG_ERRNO,	//set the process result corresponding command
	SS_LOG_END		//this one should be the last
};

/**
 * @brief init function
 *		should run before write log and it has nothing to do with lib_log which has no open. it can be run in the beginning of process
 * @param[in] log_file: the name of log file
 * @param[in] log_path: the path of log file
 * @param[in] max_size: the max length of log file
 * @param[in] log_level: the level of log(0, 1, 2, 4, 8, 16)
 * @param[in] to_syslog: interesting events to send to syslog
 * @param[in] spec: swtiches LIB_LOGTTY or LIB_LOGNEWFILE
 * @return 0 success -1 failed
 */
int ss_log_init(const char *log_path = "./log", const char *log_file = "ss_log.", int max_size = 1000, int log_level = 16, int to_syslog = 0, int spec = 0);

/**
 * @brief thead init function.it is for thread that print log. 
 * @param[in] thread_name: thread info
 * @return 0 success, -1 failed
 */
int ss_log_initthread(const char *thread_name);

/*
 * @brief program end, call this function to close log file
 */
void ss_log_close();

/*
 * @brief program end, call this function to close log file
 */
void ss_log_closethread();

/**
 * @brief set thread notice info stack, the max length is NOTICE_INFO_MAXLEN
 * @param[in] key:  key info of  key:value
 * @param[in] fmt:  format of info
 * @return length of truely added info
 */

unsigned int ss_log_pushnotice(const char *key, const char *fmt, ...);

/**
 * @brief set basic notice info, the items should be required
 * @param[in] type: basic information type
 * @param[in] fmt: format info 
 * @return the number of chars been added
 */
unsigned int ss_log_setbasic(ss_notice_type type, const char*fmt, ...);

/**
 * @brief get basic notice info
 * @param[in] type: basic notice info type
 * @return string info
 */
char* ss_log_getbasic(ss_notice_type type);

/**
 * @brief set logid
 * @param[in] logid: logid of network
 * @return logid that set
 */
unsigned int ss_log_setlogid(unsigned int logid);

/**
 * @brief get logid
 * @return logid
 */
unsigned int ss_log_getlogid();

/**
 * @brief clear logid
 * @return logid that clear
 */
unsigned int ss_log_clearlogid();

/**
 * @brief pop the push into info
 * @return string of user put
 */
char* ss_log_popnotice();

/**
 * @brief clear thread log data. after printing log, it will be call
 * @return 0 success , other failed
 */
int ss_log_clearnotice();

/**
 * @brief get the processing time for thread, used in LIB_LOG_DEBUG to print distance last print time
 * @return the process time from last time.
 */
unsigned int ss_log_getussecond();

#define SS_LOG_MONITOR(fmt, arg...) do { \
	lib_writelog(LIB_LOG_WARNING, "---LOG_MONITOR--- [ %clogid:%s %c][ %creqip:%s %c][%s:%d]" fmt, \
			SS_SPACE, ss_log_getbasic(SS_LOG_LOGID), SS_SPACE, SS_SPACE, ss_log_getbasic(SS_LOG_REQIP), SS_SPACE, \
			__FILE__, __LINE__, ## arg); \
}while (0)

#define SS_LOG_FATAL(fmt, arg...) do { \
	lib_writelog(LIB_LOG_WARNING, "---LOG_MONITOR--- [%clogid:%s %c][%creqiq:%s %c][%s:%d]" fmt, \
			SS_SPACE, ss_log_getbasic(SS_LOG_LOGID), SS_SPACE, SS_SPACE, ss_log_getbasic(SS_LOG_REQIP), SS_SPACE, \
			__FILE__, __LINE__, ## arg); \
	lib_writelog(LIB_LOG_FATAL, "[ %clogid:%s %c][ %creqip:%s %c][%s:%d]" fmt,\
			SS_SPACE, ss_log_getbasic(SS_LOG_LOGID), SS_SPACE, SS_SPACE, ss_log_getbasic(SS_LOG_REQIP), SS_SPACE, \
			__FILE__, __LINE__, ## arg); \
} while (0)


#define SS_LOG_WARNING(fmt, arg...) do { \
	lib_writelog(LIB_LOG_WARNING, "[ %clogid:%s %c][ %creqip:%s %c][%s:%d]" fmt, \
			SS_SPACE, ss_log_getbasic(SS_LOG_LOGID), SS_SPACE, SS_SPACE, ss_log_getbasic(SS_LOG_REQIP), SS_SPACE, \
			__FILE__, __LINE__, ## arg); \
} while (0)

#define SS_LOG_NOTICE(fmt, arg...) do { \
	lib_writelog(LIB_LOG_NOTICE, "[ %clogid:%s %c][ %cproctime:%s %c][ %creqip:%s %c][ %creqsvr:%s %c][ %ccmdno:%s %c][ %csvrname:%s %c][ %cerrno:%s %c][ %c%s %c][ %c%s %c][%c" fmt" %c]",  \
			SS_SPACE, ss_log_getbasic(SS_LOG_LOGID), SS_SPACE, SS_SPACE, ss_log_getbasic(SS_LOG_PROCTIME), SS_SPACE, \
			SS_SPACE, ss_log_getbasic(SS_LOG_REQIP), SS_SPACE, SS_SPACE, ss_log_getbasic(SS_LOG_REQSVR), SS_SPACE, \
			SS_SPACE, ss_log_getbasic(SS_LOG_CMDNO), SS_SPACE, SS_SPACE, ss_log_getbasic(SS_LOG_SVRNAME), SS_SPACE, \
			SS_SPACE, ss_log_getbasic(SS_LOG_ERRNO), SS_SPACE, SS_SPACE, "", SS_SPACE, \
			SS_SPACE, ss_log_popnotice(), SS_SPACE, SS_SPACE, ## arg, SS_SPACE); \
	ss_log_clearnotice(); \
} while (0)

#define SS_LOG_TRACE(fmt, arg...) do { \
	lib_writelog(LIB_LOG_TRACE, "[ %clogid:%s %c][%s:%d][time:%uus]" fmt, \
			SS_SPACE, ss_log_getbasic(SS_LOG_LOGID), SS_SPACE, __FILE__, __LINE__, \
			ss_log_getussecond(), ## arg); \
} while (0)


#define SS_LOG_DEBUG(fmt, arg...) do { \
	lib_writelog(LIB_LOG_DEBUG, "[ %clogid:%s %c][%s:%d][time:%uus]" fmt, \
			SS_SPACE, ss_log_getbasic(SS_LOG_LOGID), SS_SPACE, __FILE__, __LINE__, \
			ss_log_getussecond(), ## arg); \
} while (0)




#endif
