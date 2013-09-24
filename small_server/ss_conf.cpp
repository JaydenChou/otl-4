#include "ss_conf.h"
#include <sys/stat.h>

static bool file_exist(const char *path, const char *name)
{
	if (NULL == path || NULL == name) {
		return false;
	}

	struct stat statbuff;
	char fullpath[PATH_SIZE];
	snprintf(fullpath, sizeof(fullpath), "%s/%s", path, name);

	if (stat(fullpath, &statbuff) == -1) {
		return false;
	}

	if(!S_ISREG(statbuff.st_mode)) {
		return false;
	}

	return true;
}

static lib_conf_data_t* get_rangefile(const char *path, const char *filename)
{
	if (NULL == path || NULL == filename) {
		return NULL;
	}

	if (!file_exist(path, filename)) {
		return NULL;
	}

	lib_conf_data_t *range = NULL;

	range = lib_initconf(SS_CONF_DEFAULT_ITEMNUM);

	if (NULL != range) {
		if (lib_readconf_ex(path, filename, range) == -1) {
			lib_freeconf(range);
			SS_LOG_WARNING("can't load file[%s]", filename);
			range = NULL;
		}
	} else {
		SS_LOG_WARNING("couldn't open[%s]", filename);
	}

	return range;
}

static int write_comment(FILE *fp, const char *comment)
{
	if (NULL == fp) {
		return SS_CONF_NULL;
	}

	fputc('\n',fp);
	if (NULL == comment) {
		return SS_CONF_SUCCESS;
	}

	char *p = NULL;
	size_t n = 0;
	while(*comment) {
		while (*comment == '\n') {
			fputc('\n', fp);
		}

		if (*comment == '\0') {
			break;
		}

		fputc('#', fp);
		p = strchr(const_cast<char*>(comment), '\n');
		if (NULL == p) {
			break;
		}

		n = (size_t)(p-comment);
		fwrite(comment, sizeof(char), n, fp);

		comment = p;
	}

	fputs(comment, fp);
	fputc('\n', fp);
	fflush(fp);

	return SS_CONF_SUCCESS;
}

static int load_str(const ss_conf_data_t *conf, const char *name, char *value, size_t n)
{
	if (NULL == conf || NULL == name || NULL == value) {
		return SS_CONF_NULL;
	}

	if (NULL == conf->option) {
		return SS_CONF_NULL;
	}

	int ret = SS_CONF_LOST;

	value[0] = '\0';

	for (int i = 0; i < conf->option->num; ++i) {
		if (!strncmp(conf->option->item[i].name, name, sizeof(conf->option->item[i].name))) {
			if (ret == SS_CONF_SUCCESS) {
				SS_LOG_WARNING("more than one item name is [%s], use the value [%s]", name, value);
				return SS_CONF_SETMULTIPLE;
			}
			snprintf(value, n, "%s", conf->option->item[i].value);
			ret = SS_CONF_SUCCESS;
		}
	}

	return ret;
}

int ss_conf_getnstr(const ss_conf_data_t *conf, const char *conf_name, char *conf_value, const size_t n, const char *comment, const char *default_value)
{
	if (NULL == conf || NULL == conf_name || NULL == conf_value || 0 == n) {
		return SS_CONF_NULL;
	}

	if (conf->build != SS_CONF_READCONF) {
		if (write_comment(conf->conf_file, comment) == SS_CONF_SUCCESS) {
			if (default_value != NULL) {
				fprintf(conf->conf_file, "#[default configure(string), %s : %s]\n%s : %s", conf_name, conf_value, conf_name, conf_value);
			} else {
				fprintf(conf->conf_file, "%s : ", conf_name);
			}
			return SS_CONF_SUCCESS;
		}
		return SS_CONF_NULL;
	}

	int ret;

	ret = load_str(conf, conf_name, conf_value, n);

	if (ret == SS_CONF_LOST) {
		if (default_value != NULL) {
			snprintf(conf_value, n, "%s", default_value);
			SS_LOG_WARNING("load string [%s] fail, use default value [%s]", conf_name, default_value);
			return SS_CONF_DEFAULT;
		}
		SS_LOG_WARNING("load string fail, not found[%s]", conf_name);
		return SS_CONF_LOST;
	}
	/*

	if (check_str(conf, conf_name, conf_value) != SS_CONF_CHECKSUCCESS) {
		return SS_CONF_CHECKFAIL;
	}
	*/

	SS_LOG_TRACE("get string value [%s : %s]", conf_name, conf_value);

	return ret;

}
ss_conf_data_t* ss_conf_init(const char* path, const char *filename, const int build, const int num)
{
	ss_conf_data_t* conf = NULL;
	size_t ret;
	char range_file[PATH_SIZE];
	if (NULL == path || NULL == filename) {
		return NULL;
	}

	conf = (ss_conf_data_t*) calloc(1, sizeof(ss_conf_data_t));
	if (NULL == conf) {
		return NULL;
	}

	conf->conf_file = NULL;

	if (build != SS_CONF_READCONF) {
		ret = snprintf(conf->filename, sizeof(conf->filename),"%s/%s", path, filename);
		if (ret >= sizeof(conf->filename)) {
			SS_LOG_FATAL("[ss_conf_init] path:%s filename:%s too long", path, filename);
			free(conf);
			return NULL;
		}

		conf->conf_file = fopen(conf->filename, "w");
		if (NULL == conf->conf_file) {
			SS_LOG_FATAL("[ss_conf_init] %s write open error", conf->filename);
			free(conf);
			return NULL;
		}

	} else {
		ret = snprintf(conf->filename, sizeof(conf->filename), "%s/%s", path, filename);
		if (ret >= sizeof(conf->filename)) {
			SS_LOG_FATAL("[ss_conf_init] path:%s filename:%s too long", path, filename);
			free(conf);
			return NULL;
		}
	}

	ret = snprintf(range_file, sizeof(range_file), "%s.range", conf->filename);

	if (ret >= sizeof(range_file)) {
		SS_LOG_FATAL("[ss_conf_init] path:%s filename:%s too long", path, filename);
		free(conf);
		return NULL;
	}

	conf->option = lib_initconf(num);

	if (NULL == conf->option) {
		if (conf->conf_file != NULL) {
			fclose(conf->conf_file);
		}
		free(conf);
		return NULL;
	}

	conf->build = build;
	conf->range = NULL;

	if (build != SS_CONF_READCONF) {
		return conf;
	}

	conf->range = get_rangefile(path, range_file);

	if (lib_readconf_ex(path, filename, conf->option) == -1) {
		lib_freeconf(conf->option);
		if (conf->range != NULL) {
			lib_freeconf(conf->range);
		}
		if (conf->conf_file != NULL) {
			fclose(conf->conf_file);
		}
		free(conf);
		return NULL;
	}

	return conf;
}

int ss_conf_close(ss_conf_data_t *conf)
{
	if (NULL != conf) {
		if (NULL != conf->option) {
			lib_freeconf(conf->option);
			conf->option = NULL;
		}

		if (NULL != conf->range) {
			lib_freeconf(conf->range);
			conf->range = NULL;
		}

		if (NULL != conf->conf_file) {
			fclose(conf->conf_file);
			conf->conf_file = NULL;
		}

		free(conf);

	}

	return SS_CONF_SUCCESS;
}
