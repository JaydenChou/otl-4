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

#endif
