#include "ss_conf.h"
#include <sys/stat.h>
#include <regex.h>
#define SS_CONF_MAIL_REGEX "^[_.0-9a-zA-Z-]*@([_0-9a-zA-Z-]*.)*[_0-9a-zA-Z_]{2,5}$"
 #define SS_CONF_IP_REGEX   "([0-9]|[0-9][0-9]|1[0-9][0-9]|2[0-4]"\
	     "[0-9]|25[0-5])\\.([0-9]|[0-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\\."\
 "([0-9]|[0-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\\.([0-9]|[0-9][0-9]"\
 "|1[0-9][0-9]|2[0-4][0-9]|25[0-5])$"

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

static bool is_blank_str(char *str, size_t n)
{
	if (str == NULL) {
		return true;
	}

	for (int i = 0; i < n && str[i] != '\0'; ++i) {
		if (str[i] != SPACE || str[i] != TAB || str[i] != '\0') {
			return false;
		}
	}
	
	return true;
}

static int check_str_regex(char *str, const char *regex_str)
{
	regex_t re;
	regmatch_t pmatch[2];
	if (NULL == str) {
		return SS_CONF_CHECKFAIL;
	}

	//if regex_str is null, we think it needn't to be check
	if (NULL == regex_str) {
		return SS_CONF_CHECKSUCCESS;
	}

	regcomp(&re, regex_str, REG_EXTENDED);

	if (regexec(&re, str, 2, pmatch, REG_EXTENDED) == 0) {
		if (pmatch[0].rm_so == 0 && str[pmatch[0].rm_eo] == '\0') {
			regfree(&re);
			return SS_CONF_CHECKSUCCESS;
		}
	}

	regfree(&re);
	return SS_CONF_CHECKFAIL;
}

static int check_str_ipv4(char *str)
{
	return check_str_regex(str, SS_CONF_IP_REGEX);
}

static int check_str_mail(char *str)
{
	return check_str_regex(str, SS_CONF_MAIL_REGEX);
}

static int get_regex_str(char *str, char *regex_str, size_t n)
{
	if (NULL == str || NULL == regex_str) {
	   return SS_CONF_NULL;
	}
	
	char *l = NULL;
	char *r = NULL;
	l = strchr(str, '[');
	if (NULL == l) {
		return SS_CONF_NULL;
	}
    r = strchr(str,']');
	if (NULL == r) {
		return SS_CONF_NULL;
	}
	if (r > l + n) {
		return SS_CONF_NULL;
	}

	strncpy(regex_str, l, r-l-1);
	regex_str[r-l-1] = '\0';
	return SS_CONF_SUCCESS;	
}

static int check_str(const ss_conf_data_t *conf, const char *name, char *value)
{

	if (NULL == conf || NULL == name || NULL == value) {
		return SS_CONF_NULL;
	}

	//if range file isn't exist, also check success
	if (conf->range == NULL) {
		return SS_CONF_CHECKSUCCESS;
	}

	//get the value string, if not found return check_success
	int ret;
	char range_str[WORD_SIZE];
	char range_check_str[WORD_SIZE];

	ret = lib_getconfnstr(conf->range, name, range_str, sizeof(range_str));

	if (ret == -1) {
		return SS_CONF_CHECKSUCCESS;
	}

	if (is_blank_str(range_str, sizeof(range_str))) {
		return SS_CONF_CHECKSUCCESS;
	}

	range_check_str[0] = '\0';

	if (sscanf(range_str, "%s", range_check_str) != 1) {
		SS_LOG_WARNING("string [%s : %s] range file [%s] format is invalid", name, value, range_str);
		return SS_CONF_CHECKFAIL;
	}

	if (!strncasecmp(range_check_str, "ip", 2)) {

		ret = check_str_ipv4(value);
		if (ret != SS_CONF_CHECKSUCCESS) {
			SS_LOG_WARNING("[%s : %s] is invalid ip format", name, value);
		}
		
		return ret;
	} else if (!strncasecmp(range_check_str, "mail", 4)) {
		ret = check_str_mail(value);
		if (ret != SS_CONF_CHECKSUCCESS) {
			SS_LOG_WARNING("[%s : %s] is in valid mail format", name, value);
		}

		return ret;
	} else if (!strncasecmp(range_check_str, "regex", 5)) {//other regex check
		if (get_regex_str(range_str, range_check_str, sizeof(range_check_str) == SS_CONF_CHECKSUCCESS)) {
			ret = check_str_regex(value, range_check_str);
			if (ret != SS_CONF_CHECKSUCCESS) {
				SS_LOG_WARNING("check [%s : %s] with regex [%s] error", name, value, range_check_str);
			}
			return ret;
		}
	}

	SS_LOG_WARNING("check [%s : %s] range file [%s] format invalid", name, value, range_str);
	return SS_CONF_CHECKFAIL;
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

	if (check_str(conf, conf_name, conf_value) != SS_CONF_CHECKSUCCESS) {
		return SS_CONF_CHECKFAIL;
	}

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
