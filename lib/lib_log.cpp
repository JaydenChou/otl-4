#include "lib_log.h"

static pthread_key_t g_log_fd = PTHREAD_KEYS_MAX;
static pthread_mutex_t file_lock = PTHREAD_MUTEX_INITIALIZER;
static lib_file_t g_file_array[LOG_FILE_NUM];
#define STRING_FATAL	"FATAL:"
#define STRING_WARING	"WARING:"
#define STRING_TRACE	"TRACE:"
#define STRING_NOTICE	"NOTICE:"
#define STRING_DEBUG	"DEBUG:"
#define STRING_NULL		"@NULL@"

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
