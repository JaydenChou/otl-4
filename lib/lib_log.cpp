#include "lib_log.h"

static pthread_key_t g_log_fd = PTHREAD_KEYS_MAX;
static pthread_mutex_t file_lock = PTHREAD_MUTEX_INITIALIZER;
static lib_file_t g_file_array[LOG_FILE_NUM];

static lib_file_t g_file_stderr = {stderr, 0, 1, 0, PTHREAD_MUTEX_INITIALIZER, "/dev/stderr"};
static lib_log_t g_log_stderr = {1, &g_file_stderr, &g_file_stderr, 0, LIB_LOG_ALL, 0, 0, {NULL}};

static char g_proc_name[MAX_FILENAME_LEN+1] = "";
#define STRING_FATAL	"FATAL:"
#define STRING_WARNING	"WARNING:"
#define STRING_TRACE	"TRACE:"
#define STRING_NOTICE	"NOTICE:"
#define STRING_DEBUG	"DEBUG:"
#define STRING_NULL		"@NULL@"

#define TIME_SIZE 20

static char* lib_ctime(char* time_buf, int time_buf_len)
{
	time_t ttime;
	time(&ttime);
	tm vtm;
	localtime_r(&ttime, &vtm);
	snprintf(time_buf, time_buf_len, "%02d-%02d %02d:%02d:%02d", vtm.tm_mon+1, vtm.tm_mday,
									 vtm.tm_hour, vtm.tm_min, vtm.tm_sec);
	return time_buf;
}

static lib_log_t* lib_alloc_log_unit()
{
	lib_log_t* log_fd;
	log_fd = (lib_log_t*) calloc(1, sizeof(lib_log_t));

	if (NULL == log_fd) {
		return NULL;
	}

	if (pthread_setspecific(g_log_fd, log_fd) != 0) {
		free(log_fd);
		return NULL;
	}

	return log_fd;
}

static lib_log_t* lib_get_log_unit()
{
	return (lib_log_t*) pthread_getspecific(g_log_fd);
}

static void lib_free_log_unit()
{
	lib_log_t* log_fd;
	log_fd = lib_get_log_unit();
	if (log_fd != NULL) {
		pthread_setspecific(g_log_fd, NULL);
		free(log_fd);
	}
}

static FILE* lib_open_file(const char* name, char* mod)
{
	size_t path_len;
	char* path_end;
	char path[MAX_FILENAME_LEN+1];
	FILE* fp;

	fp = fopen((char*)name, mod);
	if (fp != NULL) {
		return fp;
	}

	path_end = strrchr(const_cast<char *>(name), '/');

	if (path_end != NULL) {
		path_len = (path_end > name+MAX_FILENAME_LEN) ? MAX_FILENAME_LEN : (path_end - name);
	} else {
		path_len = strlen(name) > MAX_FILENAME_LEN ? MAX_FILENAME_LEN : strlen(name);
	}

	strncpy(path, name, path_len);
	path[path_len] = '\0';

	mkdir(path, 0700);

	return fopen(path, mod);

}

static lib_file_t* lib_open_file_unit(const char* file_name, int flag, int max_size)
{
	int i;
	lib_file_t* file_ret = NULL;
	pthread_mutex_lock(&file_lock);
	for (i = 0; i < LOG_FILE_NUM; ++i) {
		if (NULL == g_file_array[i].fp) {
			if (NULL == file_ret) {
				file_ret = &g_file_array[i];
			}
			continue;
		}

		if (!strcmp(g_file_array[i].file_name, file_name)) {
			++g_file_array[i].ref_cnt;
			pthread_mutex_unlock(&file_lock);
			return &g_file_array[i];
		}
	}

	if (file_ret == NULL) {
		file_ret->fp = lib_open_file(file_name, "a");
		if (NULL == file_ret->fp) {
			pthread_mutex_unlock(&file_lock);
			return (lib_file_t*)ERROR_OPENFILE;
		}

		pthread_mutex_init(&(file_ret->file_lock), NULL);
		file_ret->flag = flag;
		file_ret->max_size = max_size;
		file_ret->ref_cnt = 1;
		snprintf(file_ret->file_name, MAX_FILENAME_LEN, "%s", file_name);
	}

	pthread_mutex_unlock(&file_lock);
	
	return file_ret;
}
static int lib_check(lib_file_t* file_fd, char* tmp_file, size_t tmp_file_size, int split_file)
{
	int ret = 0;
	int stat_ret = 0;
	struct stat st;
	if (NULL == file_fd || NULL == file_fd->fp) {
		return -1;
	}

	if (0 == file_fd->max_size) {
		return -1;
	}

	if (stderr == file_fd->fp) {
		return -1;
	}

	stat_ret = stat(file_fd->file_name, &st);

	if (-1 == stat_ret) {
		fclose(file_fd->fp);
		file_fd->fp = lib_open_file(file_fd->file_name, "a");
		if (NULL == file_fd->fp) {
			return -1;
		}
	}

	if ((file_fd->flag & LIB_FILE_TRUNCATE) && st.st_size+LOG_BUF_SIZE_EX > file_fd->max_size) {
	   if (!split_file) {
		   char *p = NULL;
		   p = strrchr(const_cast<char *>(file_fd->file_name), '/');
		   if (NULL == p) {
			   snprintf(tmp_file, tmp_file_size, "%s.tmp", file_fd->file_name);
		   } else {
			   *p = 0;
			   snprintf(tmp_file, tmp_file_size, "%s/.%s.tmp", file_fd->file_name, p+1);
			   *p = '/';
		   }
	   } else {
		   time_t tmp_time;
		   struct tm vtm;
		   localtime_r(&tmp_time, &vtm);
		   int retl = snprintf(tmp_file, tmp_file_size, "%s.%04d%02d%02d%02d%02d%02d", vtm.tm_year+1900, vtm.tm_mon+1, vtm.tm_mday, vtm.tm_hour, vtm.tm_min, vtm.tm_sec);
		   int suff_num = 0;
		   struct stat statbuf;
		   while (stat(tmp_file, &statbuf) == 0) {
			   suff_num++;
			   snprintf(tmp_file+retl, tmp_file_size-retl,".%d", suff_num);
		   } 
	   }
	   
	   rename(file_fd->file_name, tmp_file);
	   fclose(file_fd->fp);
	   file_fd->fp = lib_open_file(file_fd->file_name, "a");

	   if (NULL == file_fd->fp) {
		   return -1;
	   }

	   ret = 1;
	} else {
		if (st.st_size >= file_fd->max_size) {
			file_fd->flag & LIB_FILE_FULL;
		}
	}
	return ret;
}
void lib_close_file(lib_file_t* file_fd)
{
	pthread_mutex_lock(&file_lock);
	--file_fd->ref_cnt;
	if (file_fd->ref_cnt <= 0) {
		if (file_fd != NULL) {
			fclose(file_fd->fp);
		}
		memset(file_fd, 0, sizeof(lib_file_t));
	}
	pthread_mutex_unlock(&file_lock);
}

int lib_openlog_ex(lib_log_t *log_fd, const char* file_name, int mask, int flag, int maxlen, lib_log_self_t* self)
{
	char tmp_file_name[MAX_FILENAME_LEN];
	log_fd->mask = mask;
	g_log_stderr.mask = mask;
	snprintf(tmp_file_name, MAX_FILENAME_LEN, "%slog", file_name);
	log_fd->pf = lib_open_file_unit(tmp_file_name, flag, maxlen);
	if (NULL == log_fd->pf) {
		return ERROR_NOSPACE;
	} else if ((lib_file_t*)ERROR_OPENFILE == log_fd->pf) {
		return ERROR_OPENFILE;
	}

	snprintf(tmp_file_name, MAX_FILENAME_LEN, "%slog.wf", file_name);
	log_fd->pf_wf = lib_open_file_unit(tmp_file_name, flag, maxlen);
	if (NULL == log_fd->pf) {
		return ERROR_NOSPACE;
	} else if ((lib_file_t*)ERROR_OPENFILE == log_fd->pf) {
		return ERROR_OPENFILE;
	}

	if (self != NULL) {
		for (int i = 0; i < self->log_number; ++i) {
			if (strlen(self->name[i]) != 0 && self->flags[i]) {
				snprintf(tmp_file_name, MAX_FILENAME_LEN, "%s%s.sdf.log", file_name, self->name[i]);
				log_fd->spf[i] = lib_open_file_unit(tmp_file_name, flag, maxlen);
				if (NULL == log_fd->spf[i] || (lib_file_t*)ERROR_OPENFILE == log_fd->spf[i]) {
					for (int j = i-1; j >= 0; --j) {
						if (self->flags[i]) {
							lib_close_file(log_fd->spf[j]);
						}
					}
					lib_close_file(log_fd->pf);
					lib_close_file(log_fd->pf_wf);
					return (NULL == log_fd->spf[i]) ? ERROR_NOSPACE : ERROR_OPENFILE;
				}
			}
		}
	}

	return 0;
}

static int lib_openlog_self(const lib_log_self_t* self)
{
	if (NULL != self) {


		if (self->log_number > MAX_SELF_DEF_LOG || self->log_number < 0) {
			fprintf(stderr, "in lib_log.cpp: self define log_number error! log_numer = %d\n", self->log_number);
			fprintf(stderr, "in lib_log.cpp:open self log error\n");
			return -1;
		}

		for (int i = 0; i < self->log_number; ++i) {
			if (strlen(self->name[i]) == 0 && self->flags[i] == 1) {
				fprintf(stderr, "int lib_log.cpp: self define log[%d] error!\n", i);
				fprintf(stderr, "int lib_log.cpp: open self define log[i] error!\n", i);
				return -1;
			}
		}
	}

	return 0;
}

/*
 * @brief make the buff to the real log file 
 * @param [in] file_fd : file fd
 * @param [in] buff : buffe whici to be write
 * @return -1 fail 0 success
 */
int lib_vwritelog_buff(lib_file_t* file_fd, char *buff, const int split_file)
{
	int check_flag = 0;
	char tmp_filename[MAX_FILENAME_LEN]; /* the temp filename, for log to call back */
	if (NULL == file_fd) {
		return -1;
	}

	pthread_mutex_lock(&file_fd->file_lock);
	check_flag = lib_check(file_fd, tmp_filename, sizeof(tmp_filename), split_file);
	if (check_flag > 0) {
		fprintf(file_fd->fp, "%s\n", buff);
		fflush(file_fd->fp);
	}
	pthread_mutex_unlock(&file_fd->file_lock);
	
	if (1 == check_flag && 0 == split_file) {
		remove(tmp_filename);
	}

	return 0;	
}

int lib_vwritelog_ex(lib_log_t* log_fd, int event, const char *fmt, va_list args)
{
	size_t pos = 0;
	char buff[LOG_BUF_SIZE_EX];
	char now[TIME_SIZE];
	lib_file_t* file_fd;

	buff[0] = '\0';
	file_fd = log_fd->pf;

	if (log_fd->mask < event) {
		return ERROR_EVENT;
	}

	switch(event) {
		case LIB_LOG_START:
			break;
		case LIB_LOG_WFSTART:
			file_fd = log_fd->pf_wf;
			break;
		case LIB_LOG_NONE:
			break;
		case LIB_LOG_FATAL:
			memcpy(&buff[pos], STRING_FATAL, sizeof(STRING_FATAL));
			file_fd = log_fd->pf_wf;
			break;
		case LIB_LOG_WARNING:
			memcpy(&buff[pos], STRING_WARNING, sizeof(STRING_WARNING));
			file_fd = log_fd->pf_wf;
			break;
		case LIB_LOG_NOTICE:
			memcpy(&buff[pos], STRING_NOTICE, sizeof(STRING_NOTICE));
			break;
		case LIB_LOG_TRACE:
			memcpy(&buff[pos], STRING_TRACE, sizeof(STRING_TRACE));
			break;
		case LIB_LOG_DEBUG:
			memcpy(&buff[pos], STRING_DEBUG, sizeof(STRING_DEBUG));
			break;
		default:
			break;
	}

	if (file_fd->flag & LIB_FILE_FULL) {
		return ERROR_FILEFULL;
	}
	lib_ctime(now, sizeof(now));
	pos += strlen(&buff[pos]);
	pos += snprintf(&buff[pos], LOG_BUF_SIZE_EX-pos,"%s: %s * %lu", now, g_proc_name, log_fd->tid);
	vsnprintf(&buff[pos], LOG_BUF_SIZE_EX-pos,fmt, args);

	if (log_fd->syslog_mask & event) {
		int priority;

		switch (event) {
			case LIB_LOG_FATAL:
				priority = LOG_ERR;
				break;
			case LIB_LOG_WARNING:
				priority = LOG_WARNING;
				break;
			case LIB_LOG_NOTICE:
				priority = LOG_INFO;
				break;
			case LIB_LOG_TRACE:
			case LIB_LOG_DEBUG:
				priority = LOG_DEBUG;
				break;
			default:
				priority = LOG_NOTICE;
				break;
		}
		syslog(priority, "%s\n", buff);
	}
	return lib_vwritelog_buff(file_fd, buff, (log_fd->log_spec & LIB_LOGSIZESPLIT));
}
int lib_vwritelog_self(lib_log_t* log_fd, int event, const char *fmt, va_list args)
{
	size_t pos = 0;
	char buff[LOG_BUF_SIZE_EX];
	char now[TIME_SIZE];
	lib_file_t *file_fd;

	file_fd = log_fd->spf[event];

	if (file_fd->flag & LIB_FILE_FULL) {
		return ERROR_FILEFULL;
	}

	lib_ctime(now, sizeof(now));
	pos += snprintf(&buff[pos], LOG_BUF_SIZE_EX - pos, "SDF_LOG: LEVEL:%d %s: %s * %lu", event, now, g_proc_name, log_fd->tid);
	vsnprintf(&buff[pos], LOG_BUF_SIZE_EX-pos, fmt, args);

	return lib_vwritelog_buff(file_fd, buff, (log_fd->log_spec & LIB_LOGSIZESPLIT));
}
static int lib_vwritelog_ex(lib_log_t* log_fd, int event, const char* fmt, ...)
{
	int ret = 0;
	va_list args;
	va_start(args, fmt);
	ret = lib_vwritelog_ex(log_fd, event, fmt, args);
	va_end(args);

	return ret;
}
static int lib_closelog_fd(lib_log_t* log_fd)
{
	if (NULL == log_fd) {
		return -1;
	}

	if (log_fd->pf != NULL) {
		lib_close_file(log_fd->pf);
	}

	if (log_fd->pf_wf != NULL) {
		lib_close_file(log_fd->pf_wf);
	}

	for (int i = 0; i < MAX_SELF_DEF_LOG; ++i) {
		if (log_fd->spf[i] != NULL) {
			lib_close_file(log_fd->spf[i]);
		}
	}

	return 0;
}

static void log_fd_init()
{
	pthread_key_create(&g_log_fd, NULL);
}
int lib_openlog(const char* log_path, const char* log_procname, lib_logstat_t *log_stat, int maxlen, lib_log_self_t* self)
{
	return 0;
}
int lib_openlog_r(const char* threadname, lib_logstat_t* log_stat, lib_log_self_t* self)
{
	return 0;
}
int lib_closelog_r(int iserr)
{
	return 0;
}

int main()
{
	printf("hello world");
	return 0;
}
