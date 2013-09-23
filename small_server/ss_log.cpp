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

	log_stat.level = log_level;
	log_stat.syslog_level = to_syslog;
	log_stat.spec = spec;


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

int ss_log_initthread(const char *thread_name)
{
	if (NULL == thread_name) {
		return -1;
	}

	if (lib_openlog_r(thread_name, &log_stat) !=0) {
		fprintf(stderr, "thread open log failed,"
				"error[%m] file[%s] line[%d]", __FILE__, __LINE__);
		return -1;
	}

	return notice_specific_init();
}


void ss_log_close()
{
	lib_closelog(0);
}

void ss_log_closethread()
{
	lib_closelog_r(0);
}

static int my_strncat(char *dst, const char *src, size_t n)
{
	char *d, *end;
	d = dst;
	end = dst+n-1;

	for (; d < end && *d != '\0'; ++d);

	for (; d < end; d++, src++) {
		if (!(*d=*src)) {
			return 0;
		}
	}

	return -1;
}

static int my_strlen(char *str, size_t len)
{
	if (NULL == str) {
		return 0;
	}

	char *d = str;
	int curlen = 0;
	char *end = str+len-1;

	for(; curlen < len && *d != '\0' && d < end; ++d, ++curlen);
	return curlen;
}

unsigned int ss_log_pushnotice(const char *key, const char *fmt, ...)
{
	if (NULL == fmt || NULL == key) {
		return -1;
	}

	char buff[NOTICE_INFO_MAXLEN];
	char buffvalue[NOTICE_INFO_MAXLEN];
	pnotice_info_t pnotice;

	size_t infolen = 0;

	//push key info to buffer
	buff[0] = '\0';
	buffvalue[0] = '\0';
	my_strncat(buff, key, sizeof(buff));
	if (-1 == my_strncat(buff, ":", sizeof(buff))) {
		return 0;
	}
	buff[NOTICE_INFO_MAXLEN-1] = '\0';

	//push value info to buffer
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffvalue, sizeof(buffvalue), fmt, args);
	va_end(args);

	if (-1 == my_strncat(buff, buffvalue, sizeof(buff))) {
		return 0;
	}

	if (-1 == my_strncat(buff, " ", sizeof(buff))) {
		return 0;
	}

	buff[NOTICE_INFO_MAXLEN-1] = '\0';

	pnotice = static_cast<pnotice_info_t>(pthread_getspecific(g_notice_key));
	if (NULL == pnotice) {
		return 0;
	}

	pnotice->extra_notice_str[pnotice->extra_cur_len] = '\0';
	if (my_strncat(&pnotice->extra_notice_str[pnotice->extra_cur_len], buff, sizeof(pnotice->extra_notice_str) - pnotice->extra_cur_len) != -1) {
		infolen = my_strlen(buff, NOTICE_INFO_MAXLEN);
		pnotice->extra_cur_len += infolen;
	} else {
		pnotice->extra_notice_str[pnotice->extra_cur_len] = '\0';
	}

	return infolen;
}

unsigned int ss_log_setbasic(ss_notice_type type, const char*fmt, ...)
{
	if (type <= SS_LOG_ZERO || type >= SS_LOG_END) {
		return 0;
	}

	if (NULL == fmt) {
		return 0;
	}

	char buff[NOTICE_INFO_MAXLEN];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buff, sizeof(buff), fmt, args);
	buff[NOTICE_INFO_MAXLEN-1] = '\0';
	va_end(args);

	pnotice_info_t pnotice;
	pnotice = static_cast<pnotice_info_t>(pthread_getspecific(g_notice_key));
	if (NULL == pnotice) {
		return 0;
	}

	size_t ret;

	switch(type) {
		case SS_LOG_LOGID:
			strncpy(pnotice->logid_str, buff, sizeof(pnotice->logid_str)-1);
			pnotice->logid_str[sizeof(pnotice->logid_str)-1] = '\0';
			ret = strlen(pnotice->logid_str);
			break;
		case SS_LOG_REQIP:
			strncpy(pnotice->requestip_str, buff, sizeof(pnotice->requestip_str)-1);
			pnotice->requestip_str[sizeof(pnotice->requestip_str)-1] = '\0';
			ret = strlen(pnotice->requestip_str);
			break;
		case SS_LOG_PROCTIME:
			strncpy(pnotice->processtime_str, buff, sizeof(pnotice->processtime_str)-1);
			pnotice->processtime_str[sizeof(pnotice->processtime_str)-1] = '\0';
			ret = strlen(pnotice->processtime_str);
			break;
		case SS_LOG_REQSVR:
			strncpy(pnotice->requestserver_str, buff, sizeof(pnotice->requestserver_str)-1);
			pnotice->requestserver_str[sizeof(pnotice->requestserver_str)-1] = '\0';
			ret = strlen(pnotice->requestserver_str);
			break;
		case SS_LOG_ERRNO:
			strncpy(pnotice->errno_str, buff, sizeof(pnotice->errno_str)-1);
			pnotice->errno_str[sizeof(pnotice->errno_str)-1] = '\0';
			ret = strlen(pnotice->errno_str);
			break;
		case SS_LOG_CMDNO:
			strncpy(pnotice->cmd_str, buff, sizeof(pnotice->cmd_str)-1);
			pnotice->cmd_str[sizeof(pnotice->cmd_str)-1] = '\0';
			ret = strlen(pnotice->cmd_str);
			break;
		case SS_LOG_SVRNAME:
			strncpy(pnotice->svrname_str, buff, sizeof(pnotice->svrname_str)-1);
			pnotice->svrname_str[sizeof(pnotice->svrname_str)-1] = '\0';
			ret = strlen(pnotice->svrname_str);
			break;
		default:
			ret = 0;
			break;
	}

	return ret;

}

char* ss_log_getbasic(ss_notice_type type)
{
	if (type <= SS_LOG_ZERO || type >= SS_LOG_END) {
		return "";
	}

	pnotice_info_t pnotice;
	pnotice = static_cast<pnotice_info_t>(pthread_getspecific(g_notice_key));
	if (NULL == pnotice) {
		return "";
	}

	switch(type) {
		case SS_LOG_LOGID:
			return pnotice->logid_str;
		case SS_LOG_PROCTIME:
			return pnotice->processtime_str;
		case SS_LOG_REQIP:
			return pnotice->requestip_str;
		case SS_LOG_REQSVR:
			return pnotice->requestserver_str;
		case SS_LOG_ERRNO:
			return pnotice->errno_str;
		case SS_LOG_CMDNO:
			return pnotice->cmd_str;
		case SS_LOG_SVRNAME:
			return pnotice->svrname_str;
		default:
			return "";
	}

	return "";
}

unsigned int ss_log_setlogid(unsigned int logid)
{
	pnotice_info_t pnotice;
	pnotice = static_cast<pnotice_info_t>(pthread_getspecific(g_notice_key));
	if (NULL == pnotice) {
		return 0;
	}

	pnotice->logid = logid;
	
	return logid;
}

unsigned int ss_log_getlogid()
{
	pnotice_info_t pnotice;
	pnotice = static_cast<pnotice_info_t>(pthread_getspecific(g_notice_key));

	if (NULL == pnotice) {
		return 0;
	}

	return pnotice->logid;
}

unsigned int ss_log_clearlogid()
{
	return ss_log_setlogid(0);
}

char* ss_log_popnotice()
{
	pnotice_info_t pnotice;
	pnotice = static_cast<pnotice_info_t>(pthread_getspecific(g_notice_key));

	if (NULL == pnotice) {
		return "";
	}

	if (pnotice->extra_cur_len >= NOTICE_INFO_MAXLEN) {
		pnotice->extra_cur_len = NOTICE_INFO_MAXLEN - 1;
	}

	pnotice->extra_notice_str[pnotice->extra_cur_len] = '\0';
	pnotice->extra_cur_len = 0;

	return pnotice->extra_notice_str;
}


int ss_log_clearnotice()
{
	pnotice_info_t pnotice;
	pnotice = static_cast<pnotice_info_t>(pthread_getspecific(g_notice_key));

	if (pnotice == NULL) {
		return -1;
	}

	pnotice->logid_str[0] = '\0';
	pnotice->processtime_str[0] = '\0';
	pnotice->requestip_str[0] = '\0';
	pnotice->requestserver_str[0] = '\0';
	pnotice->errno_str[0] = '\0';
	pnotice->cmd_str[0] = '\0';
	pnotice->svrname_str[0] = '\0';

	return 0;
}

unsigned int ss_log_getussecond()
{
	struct timeval curtime;
	struct timeval *plasttime;
	long totaltimes;
	long totaltimeus;

	plasttime = static_cast<struct timeval*>(pthread_getspecific(g_time_key));

	if (NULL == plasttime) {
		return 0;
	}

	gettimeofday(&curtime, NULL);

	totaltimes = curtime.tv_sec - plasttime->tv_sec;
	totaltimeus = curtime.tv_usec - plasttime->tv_usec;

	*plasttime = curtime;

	pthread_setspecific(g_time_key,(const void*)plasttime);
	return totaltimes*1000000+totaltimeus;
}
