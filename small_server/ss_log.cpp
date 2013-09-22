#include "ss_log.h"
#include <sys/time.h>

static lib_logstat_t log_stat;		//the need variable of lib log, contains some info of log pointer
static pthread_key_t g_time_key;	//debug timer key
static pthread_key_t g_notice_key;	//notice stack key

static pthread_once_t g_sslog_once = PTHREAD_ONCE_INIT;

#define SS_LOG_INT_LEN 128
#define SS_LOG_IP_LEN 128
#define SS_LOG_SERVNAME_LEN 128
#define SS_LOG_CMD_WITHEXP_LEN 128

typedef struct _notice_info_t {
	unsigned int extra_cur_len;					//the length of log file which rd registration problem
	char extra_notice_str[NOTICE_INFO_MAXLEN];	//the save string for rd registration problem
	char logid_str[SS_LOG_INT_LEN];				//logid string
	char processtime_str[SS_LOG_INT_LEN];		//the process time
	char requestip_str[SS_LOG_IP_LEN];			//the request ip address
	char requestserver_str[SS_LOG_SERVNAME_LEN];	//the name of request server
	char svrname_str[SS_LOG_SERVNAME_LEN];		//the name of request server
	char errno_str[SS_LOG_CMD_WITHEXP_LEN];		//the error number
	char cmd_str[SS_LOG_CMD_WITHEXP_LEN];		//command number
	unsigned int logid;							//logid
} notice_info_t, *pnotice_info_t;

/*
 * @brief delete sslog alloc memory info about time, if it is exist
 */
static void ss_delete_logtimemem(void *pvoid)
{
	struct timeval *plasttime;
	plasttime = static_cast<timeval *>(pvoid);
	if (NULL != plasttime) {
		free(plasttime);
	}
}

/*
 * @brief delete sslog allo memory info about notice, if it is exist
 */
static void ss_delete_lognoticemem(void *pvoid)
{
	pnotice_info_t pnotice;
	pnotice = static_cast<pnotice_info_t>(pvoid);
	if (NULL != pnotice) {
		free(pnotice);
	}
}

static void gen_key()
{
	pthread_key_create(&g_time_key, ss_delete_logtimemem);
	pthread_key_create(&g_notice_key, ss_delete_lognoticemem);
}

static int notice_specific_init()
{
	pnotice_info_t pnotice;
	pnotice = static_cast<pnotice_info_t>(pthread_getspecific(g_notice_key));
	if (NULL == pnotice) {
		pnotice = (pnotice_info_t)calloc(sizeof(notice_info_t), 1);
		if (NULL == pnotice) {
			return -1;
		}
		pnotice->extra_cur_len = 0;
	}

	pthread_setspecific(g_notice_key, (const void*)pnotice);

	struct timeval *plasttime;
	plasttime = static_cast<struct timeval*>(pthread_getspecific(g_time_key));
	if (NULL == plasttime) {
		plasttime = (struct timeval*)calloc(sizeof(struct timeval), 1);
		if (NULL == plasttime) {
			free(pnotice);
			return -1;
		}
	}
	gettimeofday(plasttime, NULL);
	pthread_setspecific(g_time_key, (const void*)plasttime);

	return 0;
}

int ss_log_init(const char *log_path, const char *log_file, int max_size, int log_level, int to_syslog, int spec)
{
	char real_path[256];
	char real_file[256];

	//init unique key
	pthread_once(&g_sslog_once, gen_key);

	if (NULL == log_path) {
		fprintf(stderr, "log_path parameter error.\n");
		return -1;
	}

	if (NULL == log_file) {
		fprintf(stderr, "log_file parameter error.\n");
		return -1;
	}

	//set the default file path and file 
	if ('\0' == log_path) {
		snprintf(real_path, sizeof(real_path), "%s", "./log");
	} else {
		snprintf(real_path, sizeof(real_path), "%s", log_path);
	}

	if ('\0' == log_file) {
		snprintf(real_file, sizeof(real_file), "%s", "ss_log.");
	} else {
		snprintf(real_file, sizeof(real_file), "%s", log_file);
	}

	//call lib to open log file
	
	if (0 != lib_openlog(real_path, real_file, &log_stat, max_size)) {
		fprintf(stderr, "open log[path=%s file=%s level=%d size=%d] failed, file[%s] line[%d] error[%m]\n", real_path, real_file, log_level, max_size, __FILE__, __LINE__);
		return -1;
	}

	return notice_specific_init();
}
