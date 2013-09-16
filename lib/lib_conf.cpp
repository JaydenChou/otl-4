#include "lib_def.h"
#include "lib_log.h"
#include "lib_conf.h"

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
		p_conf->size = 1024;
	}

	p_conf->item = (lib_conf_item_t*) calloc(1, sizeof(lib_conf_item_t));

	if (NULL == p_conf->item) {
		lib_writelog(LOG_WARNING, "in lib_conf.cpp: calloc space for conf->item failed");
	}

	return p_conf;
}

int lib_freeconf(lib_conf_data_t* p_conf)
{
	if (NULL != p_conf) {
		if (NULL != p_conf->item) {
			free(p_conf->item);
		}
		free(p_conf);
	}

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
				snprintf(p_conf->item[item_num].name, WORD_SIZE, "%s",key);
				snprintf(p_conf->item[item_num].value, WORD_SIZE, "%s",value);
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
	return lib_readconf_no_dir(full_path, p_conf, 0);
}

int lib_readconf(const char* work_path, const char* filename, lib_conf_data_t* p_conf)
{
	if (NULL == work_path || NULL == filename) {
		lib_writelog(LIB_LOG_FATAL, "in lib_conf.cpp: work_path or filename is null pointer");
		return -1;
	}

	char fullname[PATH_SIZE];
	snprintf(fullname, PATH_SIZE, "%s/%s", work_path, filename);
	
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
	for (; i < p_conf->num; ++i) {
		if (!strcmp("$include", p_conf->item[i].name)) {
			char *value = p_conf->item[i].value;
			if (abs_path(value)) {
				return lib_readconf_no_dir(value, p_conf, p_conf->num);
			} else {
				snprintf(fullpath, LINE_SIZE, "%s/%s", path, value);
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
	snprintf(fullname, LINE_SIZE, "%s/%s", work_path, fname);
	int ret = lib_readconf_no_dir(fullname, p_conf);
	if (ret != 0) {
		return ret;
	}

	return lib_readconf_include(work_path, p_conf);

}


/*
int main()
{
	return 0;
}
*/
