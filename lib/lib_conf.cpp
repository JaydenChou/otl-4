#include "lib_def.h"
#include "lib_log.h"
#include "lib_conf.h"

void *debug_malloc(size_t size, const char *file, int line, const char *func)
{
	void *p;

	p = malloc(size);
	printf("%s:%d:%s:malloc(%ld): p=0x%lx\n",
			file, line, func, size, (unsigned long)p);
	return p;
}

#define malloc(s) debug_malloc(s, __FILE__, __LINE__, __func__)
#define free(p)  do {                                                   \
	printf("%s:%d:%s:free(0x%lx)\n", __FILE__, __LINE__,            \
			__func__, (unsigned long)p);                                \
	free(p);                                                        \
} while (0)


lib_conf_data_t* lib_initconf(int confnum)
{
	lib_conf_data_t* p_conf = (lib_conf_data_t*) calloc(1, sizeof(lib_conf_data_t));
	if (NULL == p_conf) {
		lib_writelog(LIB_LOG_FATAL,"in lib_conf.cpp: initconf failed");
		return NULL;
	}

	if (confnum < 1024) {
		p_conf->size = 1024;
	} else {
		p_conf->size = confnum;
	}

	p_conf->item = (lib_conf_item_t*) calloc((size_t)p_conf->size, sizeof(lib_conf_item_t));

	if (NULL == p_conf->item) {
		lib_writelog(LOG_WARNING, "in lib_conf.cpp: calloc space for conf->item failed");
	}

	return p_conf;
}

int lib_freeconf(lib_conf_data_t* p_conf)
{
	if (NULL == p_conf) {
		return 1;
	}
	
	if (NULL != p_conf->item) {
		free(p_conf->item);
	}

	free(p_conf);

	return 1;
}

static int parse_line(char *line, char* key, char* value)
{

	if (NULL == line || NULL == key || NULL == value) {
		return -1;
	}
	memset(key, 0, sizeof(key));
	memset(value, 0, sizeof(value));

	int line_len = strlen(line);
	if (line_len <= 0) {
		lib_writelog(LIB_LOG_WARNING, "the line %s length less 0", line);
		return -1;
	}
	if (line[0] == '#') {
		return 1;//code comments
	}
	int i;
	bool prefix_flag = false;
	bool key_done = false;
	bool value_start = false;
	int j = 0;
	for (i = 0; i < line_len; ++i) {
		if (line[i] != SPACE && line[i] != TAB && !prefix_flag) {
			if (line[i] == '\r' || line[i] == '\n') {
				return 2;//blank line
			}
			if (line[i] == ':') {
				return -1;//valid line
			}
			prefix_flag = true;
		}

		if (line[i] != ':' && prefix_flag && !key_done) {
			key[j++] = line[i];
		} 

		if (line[i] == ':' && prefix_flag && !key_done) {
			key[j] = '\0';
			--j;
			for(; key[j] == SPACE || key[j] == TAB; --j) {
				key[j] = '\0';
			}
			key_done = true;
			j = 0;
			++i;
		}

		if (line[i] != SPACE && line[i] != TAB && line[i] !='\r' && line[j] != '\n' && key_done && !value_start) {
			value_start = true;
		}

		if (line[i] != '\r' && line[i] != '\n' && value_start) {
			value[j++] = line[i];
		}
		
	}
	value[j] = '\0';
	--j;
	for(; value[j] == SPACE || value[j] == TAB; --j) {
	   value[j] = '\0';
	}	   

	return 0;

}

static int lib_readconf_no_dir(const char *full_path, lib_conf_data_t* p_conf, int start)
{
	FILE* fp;
	int ret = 0;
	//open configure file
	fp = fopen(full_path, "r");
	if (NULL == fp) {
		lib_writelog(LIB_LOG_FATAL, "[readconf] open configure file:[%s] fail", full_path);
		return -1;
	}

	//read conf item
	int item_num = start;
	char line[LINE_SIZE];
	char key[WORD_SIZE];
	char value[WORD_SIZE];
	
	while (fgets(line, LINE_SIZE-1, fp) != NULL) {
		if (item_num >= p_conf->size) {
			ret = -1;
			goto end;
		}
		int result = parse_line(line, key, value);

		switch(result) {
			case -1:
				lib_writelog(LIB_LOG_WARNING,"valid line: %s", line);
				break;
			case 1:
				lib_writelog(LIB_LOG_DEBUG,"comments");
				break;
			case 2:
				lib_writelog(LIB_LOG_DEBUG, "blank line");
				break;
			case 0:

				snprintf(p_conf->item[item_num].name, sizeof(p_conf->item[item_num].name), "%s",key);
				snprintf(p_conf->item[item_num].value, sizeof(p_conf->item[item_num].value), "%s",value);
				++item_num;
				break;
			default:
				break;
		}

	}

	p_conf->num = item_num;

end:
	if (NULL != fp) {
		fclose(fp);
	}
	return ret;
}

static int lib_readconf_no_dir(const char *full_path, lib_conf_data_t* p_conf)
{
	return lib_readconf_no_dir(full_path, p_conf, p_conf->num);
}

int lib_readconf(const char* work_path, const char* filename, lib_conf_data_t* p_conf)
{
	if (NULL == work_path || NULL == filename) {
		lib_writelog(LIB_LOG_FATAL, "in lib_conf.cpp: work_path or filename is null pointer");
		return -1;
	}

	char fullname[PATH_SIZE];
	snprintf(fullname, sizeof(fullname), "%s/%s", work_path, filename);
	
	return lib_readconf_no_dir(fullname, p_conf);
}

static bool abs_path(const char* path) 
{
	return path[0] == '/' || !strncmp(path, "~/", 2);
}

static int lib_readconf_include(const char* path, lib_conf_data_t* p_conf) 
{
	int i = 0;
	char fullpath[LINE_SIZE];
	char filename[LINE_SIZE];
	for (; i < p_conf->num; ++i) {
		if (!strcmp("$include", p_conf->item[i].name)) {
			char *value = p_conf->item[i].value;
			int cnt = 0;
			for (; *value != '\0'; ++value) {
				filename[cnt++] = *value;
			}
			filename[cnt] = '\0';


			if (abs_path(value)) {
				return lib_readconf_no_dir(filename, p_conf, p_conf->num);
			} else {
				snprintf(fullpath, sizeof(fullpath), "%s/%s", path, filename);
				return lib_readconf_no_dir(fullpath, p_conf, p_conf->num);
			}
		} else {
			continue;
		}
	}
	return 0;

}


int lib_readconf_ex(const char* work_path, const char* fname, lib_conf_data_t* p_conf)
{
	if (NULL == work_path || NULL == fname) {
		lib_writelog(LIB_LOG_FATAL, "there is NULL pointer in params");
		return -1;
	}
	char fullname[LINE_SIZE];
	snprintf(fullname, sizeof(fullname), "%s/%s", work_path, fname);
	int ret = lib_readconf_no_dir(fullname, p_conf);

	if (ret != 0) {
		return ret;
	}

	return lib_readconf_include(work_path, p_conf);

}

int lib_writeconf(const char* work_path, const char* fname, lib_conf_data_t* p_conf)
{
	if (NULL == work_path || NULL == fname || NULL == p_conf) {
		lib_writelog(LIB_LOG_FATAL, "in lib_writeconf: null params");
		return -1;
	}

	if (p_conf->num > p_conf->size || p_conf->num < 0) {
		lib_writelog(LIB_LOG_WARNING, "in lib_writeconf: p_conf->num is invalid");
		return -1;
	}

	char fullpath[LINE_SIZE];
	char key[WORD_SIZE];
	char value[WORD_SIZE];
	snprintf(fullpath, sizeof(fullpath), "%s/%s", work_path, fname);
	FILE *fp = fopen(fullpath, "w");

	if (NULL == fp) {
		lib_writelog(LIB_LOG_FATAL, "in lib_writelog:  open file [%s] failed", fullpath);
		return -1;
	}

	int i = 0;
	for (; i < p_conf->num; ++i) {
		if (sscanf(p_conf->item[i].name, "%s", key) != 1) {
			continue;
		}
		if (sscanf(p_conf->item[i].value, "%s", value) != 1) {
			continue;
		}
		fprintf(fp,"%s:%s\n", key, value);
	}

	fclose(fp);

	return 0;
}

int lib_modifyconfstr(lib_conf_data_t *p_conf, char *name, char *value)
{
	if (NULL == p_conf || NULL == name || NULL == value) {
		lib_writelog(LIB_LOG_FATAL, "in lib_modifyconfstr: null param");
		return -1;
	}

	if (strlen(value) > WORD_SIZE) {
		lib_writelog(LIB_LOG_WARNING, "the value is invalid");
		return -1;
	}

	int i = 0;
	for (; i < p_conf->num; ++i) {
		if (!strcmp(p_conf->item[i].name, name)) {
			snprintf(p_conf->item[i].value, sizeof(p_conf->item[i].value), "%s", value);
			return 0;
		}
	}

	return -1;
}

enum {
	TYPE_INT,
	TYPE_UINT,
	TYPE_INT64,
	TYPE_UINT64,
	TYPE_STRING,
	TYPE_FLOAT,
};

static int lib_addconf(lib_conf_data_t *p_conf, const char *name, void *value, int type)
{
	if (NULL == p_conf || NULL == name || NULL == value) {
		lib_writelog(LIB_LOG_FATAL, "in lib_addconf: null param");
		return -1;
	}

	if (p_conf->num > p_conf->size || p_conf->num < 0) {
		lib_writelog(LIB_LOG_FATAL, "configure struct item num invalid");
		return -1;
	}

	if (strlen(name) > WORD_SIZE || strlen((char *)value) > WORD_SIZE) {
		lib_writelog(LIB_LOG_FATAL, "the name or value to large");
		return -1;
	}

	int i = 0;
	for (; i < p_conf->num; ++i) {
		if (!strcmp(p_conf->item[i].name, name)) {
			return 0;
		}
	}

	
	strncpy(p_conf->item[p_conf->num].name, name, WORD_SIZE);
	
	switch(type) {
		case TYPE_STRING:
			strncpy(p_conf->item[p_conf->num].value, (char *)value, WORD_SIZE);
			break;
		case TYPE_INT:
			snprintf(p_conf->item[p_conf->num].value, sizeof(p_conf->item[p_conf->num].value), "%d", *((int *)value));
			break;
		case TYPE_FLOAT:
			snprintf(p_conf->item[p_conf->num].value, sizeof(p_conf->item[p_conf->num].value), "%f", *((float *)value));
			break;
		case TYPE_UINT:
			snprintf(p_conf->item[p_conf->num].value, sizeof(p_conf->item[p_conf->num].value), "%u", *((unsigned int*)value));
			break;
		case TYPE_INT64:
			snprintf(p_conf->item[p_conf->num].value, sizeof(p_conf->item[p_conf->num].value), "%lld", *((long long*)value));
			break;
		case TYPE_UINT64:
			snprintf(p_conf->item[p_conf->num].value, sizeof(p_conf->item[p_conf->num].value), "%llu", *((unsigned long long*)value));
			break;
		default:
			break;
	}

	++p_conf->num;

	return 0;
}

int lib_addconfstr(lib_conf_data_t *p_conf, const char *name, char *value)
{
	return lib_addconf(p_conf, name, value, TYPE_STRING);
}

int lib_getconfstr(lib_conf_data_t* p_conf, const char *name, char *value, const size_t size)
{
	if (NULL == p_conf || NULL == name || NULL == value) {
		lib_writelog(LIB_LOG_FATAL, "in lib_getconstr: param is null");
		return -1;
	}

	if (size <= 0 || size > WORD_SIZE) {
		lib_writelog(LIB_LOG_FATAL, "the param size is invalid");
		return -1;
	}

	for (int i = 0; i < p_conf->num; ++i) {
		if (!strcmp(p_conf->item[i].name, name)) {
			strncpy(value, p_conf->item[i].value, size);
			return 0;
		}
	}

	return -1;
}

int lib_getconfint(lib_conf_data_t* p_conf, const char *name, int *value)
{
	if (NULL == p_conf || NULL == name) {
		lib_writelog(LIB_LOG_FATAL, "in lib_getconfint: param is null");
		return -1;
	}

	char tstr[WORD_SIZE];
	char *p = NULL;
	int ret;
	int num;
	lib_getconfstr(p_conf, name, tstr, sizeof(tstr));


	if (strlen(tstr) <= 0) {
		lib_writelog(LIB_LOG_WARNING, "the value of %s is null", name);
		return -1;
	}

	if (tstr[0] == '0' && tstr[1] == 0) {
		*value = 0;
		return 0;
	}

	if (tstr[0] != '0') {
		p = tstr;
		ret = sscanf(p, "%d", &num);
	} else if (tstr[1] != 'x') {
		p = tstr+1;
		ret = sscanf(p, "%o", &num);
	} else {
		p = tstr+2;
		ret = sscanf(p, "%x", &num);
	}

	if (ret <= 0) {
		return -1;
	}

	*value = num;

	return 0;

}

int lib_modifyconfint(lib_conf_data_t* p_conf, const char *name, int value)
{
	if (NULL == p_conf || NULL == name) {
		lib_writelog(LIB_LOG_FATAL, "in lib_modifyconfint: the param is null");
		return -1;
	}

	for (int i = 0; i < p_conf->num; ++i) {
		if (!strcmp(p_conf->item[i].name, name)) {
				snprintf(p_conf->item[i].value, sizeof(p_conf->item[i].value), "%d", value);
				return 0;
		}
	}
				
	return -1;			
}

int lib_addconfint(lib_conf_data_t* p_conf, const char* name, int value)
{
	return lib_addconf(p_conf, name, &value, TYPE_INT);
}

int lib_getconffloat(lib_conf_data_t* p_conf, const char* name, float *value)
{
	if (NULL == p_conf || NULL == name) {
		lib_writelog(LIB_LOG_FATAL, "in lib_getconffloat: param is null");
		return -1;
	}

	char tstr[WORD_SIZE];
	int ret;

	ret = lib_getconfstr(p_conf, name, tstr, sizeof(tstr));
	
	if (ret < 0) {
		return -1;
	}

	*value = (float) atof(tstr);

	return 0;
}

int lib_modifyconffloat(lib_conf_data_t* p_conf, const char*name, float value) 
{
	if (NULL == p_conf || NULL == name) {
		lib_writelog(LIB_LOG_FATAL, "in lib_modifyconffloat: param is null");
		return -1;
	}

	for (int i = 0; i < p_conf->num; ++i) {
		if (!strcmp(p_conf->item[i].name, name)) {
			snprintf(p_conf->item[i].value, sizeof(p_conf->item[i].value), "%f", value);
			return 0;
		}
	}

	return -1;
}

int lib_addconffloat(lib_conf_data_t* p_conf, const char *name, float value)
{
	return lib_addconf(p_conf, name, &value, TYPE_FLOAT);
}

int lib_getconfuint(lib_conf_data_t* p_conf, const char *name, unsigned int *value)
{
	if (NULL == p_conf || NULL == name) {
		lib_writelog(LIB_LOG_FATAL, "in lib_getconfuint: param is null");
		return -1;
	}
	char tstr[WORD_SIZE];
	char *p = NULL;
	int ret;
	int num;

	lib_getconfstr(p_conf, name, tstr, sizeof(tstr));

	if (tstr[0] == '0' && tstr[1] == 0) {
		*value = 0;
		return 0;
	}

	if (tstr[0] != '0') {
		ret = sscanf(tstr, "%u", &num);
	} else if (tstr[1] != 'x') {
		p = tstr+1;
		ret = sscanf(p, "%o", &num);
	} else {
		p = tstr+2;
		ret = sscanf(p, "%x", &num);
	}

	if (ret <= 0) {
		return -1;
	}

	*value = num;
	
	return 0;
}

int lib_modifyconfuint(lib_conf_data_t* p_conf, const char* name, unsigned int value)
{
	if (NULL == p_conf || NULL == name) {
		lib_writelog(LIB_LOG_FATAL, "in lib_modifyconfuint: param is null");
		return -1;
	}

	for (int i = 0; i < p_conf->num; ++i) {
		if (!strcmp(p_conf->item[i].name, name)) {
			snprintf(p_conf->item[i].value, sizeof(p_conf->item[i].value), "%u", value);
			return 0;
		}
	}

	return -1;
}

int lib_addconfuint(lib_conf_data_t* p_conf, const char* name, unsigned int value)
{
	return lib_addconf(p_conf, name, &value, TYPE_UINT);
}

int lib_getconfint64(lib_conf_data_t* p_conf, const char* name, long long *value)
{
	if (NULL == p_conf || NULL == name) {
		lib_writelog(LIB_LOG_FATAL, "in lib_getconfint64:param is null");
		return -1;
	}

	int ret;
	long long num;
	char tstr[WORD_SIZE];
	char *p = NULL;

	ret = lib_getconfstr(p_conf, name, tstr, sizeof(tstr));

	if (ret != 0) {
		return -1;
	}

	if (tstr[0] == '0' && tstr[1] == 0) {
		*value = 0;
		return 0;
	}

	if (tstr[0] != '0') {
		ret = sscanf(tstr, "%lld", &num);
	} else if (tstr[1] != 'x') {
		p = tstr+1;
		ret = sscanf(p, "%llo", &num);
	} else {
		p = tstr+2;
		ret = sscanf(p, "%llx", &num);
	}

	if (ret <= 0) {
		return -1;
	}

	*value = num;

	return 0;
}

int lib_modifyconfint64(lib_conf_data_t* p_conf, const char* name, long long value)
{
	if (NULL == p_conf || NULL == name) {
		lib_writelog(LIB_LOG_FATAL, "in lib_modifyconfint64:param is null");
		return -1;
	}

	for (int i = 0; i < p_conf->num; ++i) {
		if (!strcmp(p_conf->item[i].name, name)) {
			snprintf(p_conf->item[i].value, sizeof(p_conf->item[i].value), "%lld", value);
			return 0;
		}
	}

	return -1;
}

int lib_addconfint64(lib_conf_data_t* p_conf, const char* name, long long value)
{
	return lib_addconf(p_conf, name, &value, TYPE_INT64);
}

int lib_getconfuint64(lib_conf_data_t* p_conf, const char* name, unsigned long long *value)
{
	if (NULL == p_conf || NULL == name) {
		lib_writelog(LIB_LOG_FATAL, "in lib_getconfuint64: param is null");
		return -1;
	}

	char tstr[WORD_SIZE];
	char *p = NULL;
	int ret;
	unsigned long long num;

	ret = lib_getconfstr(p_conf, name, tstr, sizeof(tstr));

	if (ret < 0) {
		return -1;
	}

	if (tstr[0] == '0' && tstr[1] == 0) {
		*value = 0;
		return 0;
	}

	if (tstr[0] != '0') {
		ret = sscanf(tstr, "%llu", &num);
	} else if (tstr[1] != 'x') {
		p = tstr+1;
		ret = sscanf(p, "%llo", &num);
	} else {
		p = tstr+2;
		ret = sscanf(p, "%llx", &num);
	}

	if (ret <= 0) {
		return -1;
	}

	*value = num;

	return 0;
}

int lib_modifyconfuint64(lib_conf_data_t* p_conf, const char* name, unsigned long long value) 
{
	if (NULL == p_conf || NULL == name) {
		lib_writelog(LIB_LOG_FATAL, "in lib_modifyconfuint64:param is null");
		return -1;
	}

	for (int i = 0; i < p_conf->num; ++i) {
		if (!strcmp(p_conf->item[i].name, name)) {
			snprintf(p_conf->item[i].value, sizeof(p_conf->item[i].value), "%llu", value);
			return 0;
		}
	}

	return -1;
}

int lib_addconfuint64(lib_conf_data_t* p_conf, const char* name, unsigned long long value)
{
	return lib_addconf(p_conf, name, &value, TYPE_UINT64);
}
/*
int main()
{
	return 0;
}
*/
