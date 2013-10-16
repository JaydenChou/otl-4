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

#define CHECK_VALUE(num, min, max) (((num) >= (min) && (num) <= (max)) ? 0 : 1 )
static int check_int_range(const ss_conf_data_t *conf, const char *name, int num)
{
	if (NULL == conf || NULL == name) {
		return SS_CONF_NULL;
	}

	if (NULL == conf->range) {
		SS_LOG_WARNING("no found configure range file, no check range[%s]", name);
		return SS_CONF_CHECKSUCCESS;
	}

	int ret;
	int l;
	int r;
	char range_str[WORD_SIZE];
	ret = lib_getconfstr(conf->range, name, range_str, sizeof(range_str));

	if (-1 == ret) {
		SS_LOG_WARNING("no found [%s] range check item", name);
		return SS_CONF_CHECKSUCCESS;
	}

	if (sscanf(range_str, "range [%d %d]", &l, &r) == 2) {
		if (CHECK_VALUE(num, l, r) == 0) {
			return SS_CONF_CHECKSUCCESS;
		} else {
			SS_LOG_WARNING("int [%s] load error, [%d] overflow range [%d %d]", name, num, l, r);
			return SS_CONF_CHECKFAIL;
		}
	}

	SS_LOG_WARNING("int [%s] load error, [%d] is invalid format", name, num);
	return SS_CONF_CHECKFAIL;

}

int ss_conf_getint(const ss_conf_data_t *conf, const char *name, int *value, const char *comment, const int *default_value)
{

	if (NULL == conf || NULL == name || NULL == value) {
		return SS_CONF_NULL;
	}

	if (conf->build != SS_CONF_READCONF) {
		if (write_comment(conf->conf_file, comment) != SS_CONF_SUCCESS) {
			if (default_value != NULL) {
				fprintf(conf->conf_file, "#[default configure[int], %s : %d]\n%s : %d", name, *default_value, name, *default_value);
				return SS_CONF_DEFAULT;
			} else {
				fprintf(conf->conf_file, "%s : ", name);
			}
			return SS_CONF_SUCCESS;
		}
		return SS_CONF_NULL;
	}

	*value = 0;
	char conf_value_str[WORD_SIZE];

	int ret;

	ret = load_str(conf, name, conf_value_str, sizeof(conf_value_str));
	if (SS_CONF_LOST == ret) {
		if (default_value != NULL) {
			*value = *default_value;
			SS_LOG_WARNING("int [%s] no found, use default value [%d]", name, *default_value);
			return SS_CONF_DEFAULT;
		}
		return SS_CONF_LOST;
	}

	if (is_blank_str(conf_value_str, sizeof(conf_value_str))) {
		SS_LOG_WARNING("int [%s] is empty", name);
		return SS_CONF_CHECKFAIL;
	}

	long num;
	char *endptr;
	errno = 0;
	num = strtol(conf_value_str, &endptr, 10);
	if (errno == ERANGE || num < INT_MIN || num > INT_MAX) {
		SS_LOG_WARNING("int [%s] load error, [%s] overflow", name, conf_value_str);
		return SS_CONF_OVERFLOW;
	}

	if (!is_blank_str(endptr, sizeof(conf_value_str))) {
		SS_LOG_WARNING("int [%s] load error, [%s] is invalid", name, conf_value_str);
		return SS_CONF_CHECKFAIL;
	}
	
	if (check_int_range(conf, name, num) != SS_CONF_CHECKSUCCESS) {
		return SS_CONF_OVERFLOW;
	}

	*value = (int)num;
	SS_LOG_TRACE("get int value [%s : %d]", name, *value);
	return SS_CONF_SUCCESS;


}

static int check_uint_range(const ss_conf_data_t *conf, const char *name, unsigned num)
{

	if (NULL == conf || NULL == name) {
		return SS_CONF_NULL;
	}

	if (NULL == conf->range) {
		SS_LOG_WARNING("no found configure range file, no check range[%s]", name);
		return SS_CONF_CHECKSUCCESS;
	}

	int ret;
	unsigned int l;
	unsigned int r;
	char range_str[WORD_SIZE];
	ret = lib_getconfstr(conf->range, name, range_str, sizeof(range_str));

	if (-1 == ret) {
		SS_LOG_WARNING("no found [%s] range check item", name);
		return SS_CONF_CHECKSUCCESS;
	}

	if (sscanf(range_str, "range [%u %u]", &l, &r) == 2) {
		if (CHECK_VALUE(num, l, r) == 0) {
			return SS_CONF_CHECKSUCCESS;
		} else {
			SS_LOG_WARNING("int [%s] load error, [%u] overflow range [%u %u]", name, num, l, r);
			return SS_CONF_CHECKFAIL;
		}
	}

	SS_LOG_WARNING("int [%s] load error, [%u] is invalid format", name, num);
	return SS_CONF_CHECKFAIL;

}

int ss_conf_getuint(const ss_conf_data_t* conf, const char *name, unsigned int *value, const char *comment, const unsigned int *default_value)
{


	if (NULL == conf || NULL == name || NULL == value) {
		return SS_CONF_NULL;
	}

	if (conf->build != SS_CONF_READCONF) {
		if (write_comment(conf->conf_file, comment) != SS_CONF_SUCCESS) {
			if (default_value != NULL) {
				fprintf(conf->conf_file, "#[default configure[uint], [%s : %u]\n%s : %u", name, *default_value, name, *default_value);
				return SS_CONF_DEFAULT;
			} else {
				fprintf(conf->conf_file, "%s : ", name);
			}
			return SS_CONF_SUCCESS;
		}
		return SS_CONF_NULL;
	}

	*value = 0;
	char conf_value_str[WORD_SIZE];

	int ret;

	ret = load_str(conf, name, conf_value_str, sizeof(conf_value_str));
	if (SS_CONF_LOST == ret) {
		if (default_value != NULL) {
			*value = *default_value;
			SS_LOG_WARNING("int [%s] no found, use default value [%u]", name, *default_value);
			return SS_CONF_DEFAULT;
		}
		SS_LOG_WARNING("load uint fail, no found[%s]", name);
		return SS_CONF_LOST;
	}

	if (is_blank_str(conf_value_str, sizeof(conf_value_str))) {
		SS_LOG_WARNING("int [%s] is empty", name);
		return SS_CONF_CHECKFAIL;
	}

	unsigned long num;
	char *endptr;
	errno = 0;
	num = strtoul(conf_value_str, &endptr, 10);
	if (errno == ERANGE || num < INT_MIN || num > INT_MAX) {
		SS_LOG_WARNING("int [%s] load error, [%s] overflow", name, conf_value_str);
		return SS_CONF_OVERFLOW;
	}

	if (!is_blank_str(endptr, sizeof(conf_value_str))) {
		SS_LOG_WARNING("int [%s] load error, [%s] is invalid", name, conf_value_str);
		return SS_CONF_CHECKFAIL;
	}
	
	if (check_uint_range(conf, name, num) != SS_CONF_CHECKSUCCESS) {
		return SS_CONF_OVERFLOW;
	}

	*value = (unsigned int)num;
	SS_LOG_TRACE("get int value [%s : %u]", name, *value);
	return SS_CONF_SUCCESS;

}

static int check_int64_range(const ss_conf_data_t *conf, const char *name, long long num)
{

	if (NULL == conf || NULL == name) {
		return SS_CONF_NULL;
	}

	if (NULL == conf->range) {
		SS_LOG_WARNING("no found configure range file, no check range[%s]", name);
		return SS_CONF_CHECKSUCCESS;
	}

	int ret;
	long long l;
	long long r;
	char range_str[WORD_SIZE];
	ret = lib_getconfstr(conf->range, name, range_str, sizeof(range_str));

	if (-1 == ret) {
		SS_LOG_WARNING("no found [%s] range check item", name);
		return SS_CONF_CHECKSUCCESS;
	}

	if (sscanf(range_str, "range [%lld %lld]", &l, &r) == 2) {
		if (CHECK_VALUE(num, l, r) == 0) {
			return SS_CONF_CHECKSUCCESS;
		} else {
			SS_LOG_WARNING("int [%s] load error, [%lld] overflow range [%lld %lld]", name, num, l, r);
			return SS_CONF_CHECKFAIL;
		}
	}

	SS_LOG_WARNING("int [%s] load error, [%lld] is invalid format", name, num);
	return SS_CONF_CHECKFAIL;
}

int ss_conf_getint64(const ss_conf_data_t *conf, const char *name, long long *value, const char *comment, const long long *default_value)
{

	if (NULL == conf || NULL == name || NULL == value) {
		return SS_CONF_NULL;
	}

	if (conf->build != SS_CONF_READCONF) {
		if (write_comment(conf->conf_file, comment) != SS_CONF_SUCCESS) {
			if (default_value != NULL) {
				fprintf(conf->conf_file, "#[default configure[uint64], [%s : %lld]\n%s : %lld", name, *default_value, name, *default_value);
				return SS_CONF_DEFAULT;
			} else {
				fprintf(conf->conf_file, "%s : ", name);
			}
			return SS_CONF_SUCCESS;
		}
		return SS_CONF_NULL;
	}

	*value = 0;
	char conf_value_str[WORD_SIZE];

	int ret;

	ret = load_str(conf, name, conf_value_str, sizeof(conf_value_str));
	if (SS_CONF_LOST == ret) {
		if (default_value != NULL) {
			*value = *default_value;
			SS_LOG_WARNING("int [%s] no found, use default value [%lld]", name, *default_value);
			return SS_CONF_DEFAULT;
		}
		SS_LOG_WARNING("load uint fail, no found[%s]", name);
		return SS_CONF_LOST;
	}

	if (is_blank_str(conf_value_str, sizeof(conf_value_str))) {
		SS_LOG_WARNING("int [%s] is empty", name);
		return SS_CONF_CHECKFAIL;
	}

	long long num;
	char *endptr;
	errno = 0;
	num = strtoll(conf_value_str, &endptr, 10);
	if (errno == ERANGE || num < INT_MIN || num > INT_MAX) {
		SS_LOG_WARNING("int [%s] load error, [%s] overflow", name, conf_value_str);
		return SS_CONF_OVERFLOW;
	}

	if (!is_blank_str(endptr, sizeof(conf_value_str))) {
		SS_LOG_WARNING("int [%s] load error, [%s] is invalid", name, conf_value_str);
		return SS_CONF_CHECKFAIL;
	}
	
	if (check_int64_range(conf, name, num) != SS_CONF_CHECKSUCCESS) {
		return SS_CONF_OVERFLOW;
	}

	*value = num;
	SS_LOG_TRACE("get int value [%s : %lld]", name, *value);
	return SS_CONF_SUCCESS;

}

static int check_uint64_range(const ss_conf_data_t *conf, const char *name, unsigned long long num)
{

	if (NULL == conf || NULL == name) {
		return SS_CONF_NULL;
	}

	if (NULL == conf->range) {
		SS_LOG_WARNING("no found configure range file, no check range[%s]", name);
		return SS_CONF_CHECKSUCCESS;
	}

	int ret;
	unsigned long long l;
	unsigned long long r;
	char range_str[WORD_SIZE];
	ret = lib_getconfstr(conf->range, name, range_str, sizeof(range_str));

	if (-1 == ret) {
		SS_LOG_WARNING("no found [%s] range check item", name);
		return SS_CONF_CHECKSUCCESS;
	}

	if (sscanf(range_str, "range [%llu %llu]", &l, &r) == 2) {
		if (CHECK_VALUE(num, l, r) == 0) {
			return SS_CONF_CHECKSUCCESS;
		} else {
			SS_LOG_WARNING("int [%s] load error, [%llu] overflow range [%llu %llu]", name, num, l, r);
			return SS_CONF_CHECKFAIL;
		}
	}

	SS_LOG_WARNING("int [%s] load error, [%llu] is invalid format", name, num);
	return SS_CONF_CHECKFAIL;
}

int ss_conf_getuint64(const ss_conf_data_t *conf, const char *name, unsigned long long *value, const char *comment, const unsigned long long *default_value)
{

	if (NULL == conf || NULL == name || NULL == value) {
		return SS_CONF_NULL;
	}

	if (conf->build != SS_CONF_READCONF) {
		if (write_comment(conf->conf_file, comment) != SS_CONF_SUCCESS) {
			if (default_value != NULL) {
				fprintf(conf->conf_file, "#[default configure[uint64], [%s : %llu]\n%s : %llu", name, *default_value, name, *default_value);
				return SS_CONF_DEFAULT;
			} else {
				fprintf(conf->conf_file, "%s : ", name);
			}
			return SS_CONF_SUCCESS;
		}
		return SS_CONF_NULL;
	}

	*value = 0;
	char conf_value_str[WORD_SIZE];

	int ret;

	ret = load_str(conf, name, conf_value_str, sizeof(conf_value_str));
	if (SS_CONF_LOST == ret) {
		if (default_value != NULL) {
			*value = *default_value;
			SS_LOG_WARNING("int [%s] no found, use default value [%lld]", name, *default_value);
			return SS_CONF_DEFAULT;
		}
		SS_LOG_WARNING("load uint fail, no found[%s]", name);
		return SS_CONF_LOST;
	}

	if (is_blank_str(conf_value_str, sizeof(conf_value_str))) {
		SS_LOG_WARNING("int [%s] is empty", name);
		return SS_CONF_CHECKFAIL;
	}

	unsigned long long num;
	char *endptr;
	errno = 0;
	num = strtoull(conf_value_str, &endptr, 10);
	if (errno == ERANGE || num < INT_MIN || num > INT_MAX) {
		SS_LOG_WARNING("int [%s] load error, [%s] overflow", name, conf_value_str);
		return SS_CONF_OVERFLOW;
	}

	if (!is_blank_str(endptr, sizeof(conf_value_str))) {
		SS_LOG_WARNING("int [%s] load error, [%s] is invalid", name, conf_value_str);
		return SS_CONF_CHECKFAIL;
	}
	
	if (check_uint64_range(conf, name, num) != SS_CONF_CHECKSUCCESS) {
		return SS_CONF_OVERFLOW;
	}

	*value = num;
	SS_LOG_TRACE("get int value [%s : %llu]", name, *value);
	return SS_CONF_SUCCESS;

}

int  ss_conf_getfloat(const ss_conf_data_t *conf, const char *name, float *value, const char *comment, const float *default_value)
{
	if (NULL == conf || NULL == name || NULL == value) {
		return SS_CONF_NULL;
	}

	if (conf->build != SS_CONF_READCONF) {
		if (write_comment(conf->conf_file, comment) != SS_CONF_SUCCESS) {
			if (default_value != NULL) {
				fprintf(conf->conf_file, "#[default configure[float], [%s : %f]\n%s : %f", name, *default_value, name, *default_value);
				return SS_CONF_DEFAULT;
			} else {
				fprintf(conf->conf_file, "%s : ", name);
			}
			return SS_CONF_SUCCESS;
		}
		return SS_CONF_NULL;
	}

	*value = 0;
	char conf_value_str[WORD_SIZE];

	int ret;

	ret = load_str(conf, name, conf_value_str, sizeof(conf_value_str));
	if (SS_CONF_LOST == ret) {
		if (default_value != NULL) {
			*value = *default_value;
			SS_LOG_WARNING("int [%s] no found, use default value [%f]", name, *default_value);
			return SS_CONF_DEFAULT;
		}
		SS_LOG_WARNING("load uint fail, no found[%s]", name);
		return SS_CONF_LOST;
	}

	if (is_blank_str(conf_value_str, sizeof(conf_value_str))) {
		SS_LOG_WARNING("int [%s] is empty", name);
		return SS_CONF_CHECKFAIL;
	}

	float num;
	char *endptr;
	errno = 0;
	num = strtod(conf_value_str, &endptr);
	if (errno == ERANGE || num < INT_MIN || num > INT_MAX) {
		SS_LOG_WARNING("int [%s] load error, [%s] overflow", name, conf_value_str);
		return SS_CONF_OVERFLOW;
	}

	if (!is_blank_str(endptr, sizeof(conf_value_str))) {
		SS_LOG_WARNING("int [%s] load error, [%s] is invalid", name, conf_value_str);
		return SS_CONF_CHECKFAIL;
	}
	
	if (check_uint64_range(conf, name, num) != SS_CONF_CHECKSUCCESS) {
		return SS_CONF_OVERFLOW;
	}

	*value = num;
	SS_LOG_TRACE("get int value [%s : %f]", name, *value);
	return SS_CONF_SUCCESS;

}


int ss_conf_getsvr(const ss_conf_data_t *conf, const char *product_name, const char *module_name, ss_svr_t *value, const char *comment)
{

	if (NULL == conf) {
		return SS_CONF_NULL;
	}

	if (NULL == value && SS_CONF_READCONF == conf->build) {
		return SS_CONF_NULL;
	}

	if (NULL == module_name) {
		SS_LOG_WARNING("[ss_conf_getsvr] module name is NULL");
		return SS_CONF_NULL;
	}

	if (SS_CONF_READCONF != conf->build) {
		if (write_comment(conf->conf_file,"") != SS_CONF_SUCCESS) {
			return SS_CONF_NULL;
		}
		if (write_comment(conf->conf_file, comment) != SS_CONF_SUCCESS) {
			return SS_CONF_NULL;
		}
	}
	char conf_name[WORD_SIZE];
	char svr_name[WORD_SIZE];
	int ret = SS_CONF_SUCCESS;
	int item_ret;
	unsigned int tmp;
	char str_tmp[WORD_SIZE];

	//merge product name and module name
	if (NULL != product_name) {
		snprintf(conf_name, sizeof(conf_name), "%s_%s", product_name, module_name);
	} else {
		snprintf(conf_name, sizeof(conf_name), "%s", module_name);
	}
	//to be use show server name
	snprintf(svr_name, sizeof(svr_name), "_svr_%s_name", conf_name);
	item_ret = ss_conf_getnstr(conf, svr_name, str_tmp, sizeof(str_tmp), "server name");
	if (item_ret != SS_CONF_SUCCESS) {
		ret = item_ret;
	}
	if (value != NULL && SS_CONF_READCONF == conf->build) {
		snprintf(value->svr_name, sizeof(value->svr_name), "%s", str_tmp);
	}

	//get port
	snprintf(svr_name, sizeof(svr_name), "_svr_%s_port", conf_name);
	item_ret = ss_conf_getuint(conf, svr_name, &tmp, "started sever port");
	if (item_ret != SS_CONF_SUCCESS) {
		ret = item_ret;
	}
	if (value != NULL && SS_CONF_READCONF == conf->build) {
		value->port = tmp;
	}

	//get read timeout
	snprintf(svr_name, sizeof(svr_name), "_svr_%s_readtimeout", conf_name);
	item_ret = ss_conf_getuint(conf, svr_name, &tmp, "read timeout");
	if (item_ret != SS_CONF_SUCCESS) {
		ret = item_ret;
	}

	if (value != NULL && SS_CONF_READCONF == conf->build) {
		value->read_timeout = tmp;
	}

	//get write timeout
	snprintf(svr_name, sizeof(svr_name), "_svr_%s_wirtetimeout", conf_name);
	item_ret = ss_conf_getuint(conf, svr_name, &tmp, "write timeout");
	if (item_ret != SS_CONF_SUCCESS) {
		ret = item_ret;
	}
	if (value != NULL && SS_CONF_READCONF == conf->build) {
		value->write_timeout = tmp;
	}

	//get thread num
	snprintf(svr_name, sizeof(svr_name), "_svr_%s_threadnum", conf_name);
	item_ret = ss_conf_getuint(conf, svr_name, &tmp, "started thread num");
	if (item_ret != SS_CONF_SUCCESS) {
		ret = item_ret;
	}
	if (value != NULL && SS_CONF_READCONF == conf->build) {
		value->threadnum = tmp;
	}

	//get connect type
	snprintf(svr_name, sizeof(svr_name), "_svr_%s_connecttype",conf_name);
	item_ret = ss_conf_getuint(conf, svr_name, &tmp, "connecttype");
	if (item_ret != SS_CONF_SUCCESS) {
		ret = item_ret;
	}
	if (value != NULL && SS_CONF_READCONF == conf->build) {
		value->connect_type = tmp;
	}

	//get pool type
	snprintf(svr_name, sizeof(svr_name), "_svr_%s_servertype", conf_name);
	item_ret = ss_conf_getuint(conf, svr_name, &tmp, "server type");
	if (item_ret != SS_CONF_SUCCESS) {
		ret = item_ret;
	}
	if (value != NULL && SS_CONF_READCONF == conf->build) {
		value->server_type = tmp;
	}

	//get cpool size default = 100
	unsigned int temp_default = 100;
	snprintf(svr_name, sizeof(svr_name), "_svr_%s_queuesize", conf_name);
	item_ret = ss_conf_getuint(conf, svr_name, &tmp, "queue size", &temp_default);
	if (item_ret != SS_CONF_SUCCESS) {
		ret = item_ret;
	}
	if (value != NULL && SS_CONF_READCONF == conf->build) {
		value->queue_size = tmp;
	}

	//get sock size default = 500
	temp_default = 500;
	snprintf(svr_name, sizeof(svr_name), "_svr_%s_socksize", conf_name);
	item_ret = ss_conf_getuint(conf, svr_name, &tmp, "sock size", &temp_default);
	if (item_ret != SS_CONF_SUCCESS) {
		ret = item_ret;
	}
	if (value != NULL && SS_CONF_READCONF == conf->build) {
		value->sock_size = tmp;
	}

	//for beauty
	if(SS_CONF_READCONF != conf->build) {
		if (fputc('\n',conf->conf_file) == EOF) {
			return SS_CONF_NULL;
		}
	}
		
	return SS_CONF_SUCCESS;
}

/*
 * @brief parse ip list, eg: 127.0.0.1 172.18.120.154/172.18.120.155
 * @param[in] svr_name: server name, for log
 * @param[in] src: ip string
 * @param[out] svr_ips: ss_svr_ipt, ss_svr_ip's ip list
 * @param[out] num: ip list num
 */
int parse_iplist(const char *svr_name, const char *src, ss_svr_ip_t svr_ips[], unsigned int *num)
{
	if (NULL == src || NULL == num) {
		return SS_CONF_NULL;
	}

	*num = 0;
	int i = 0;
	int j;
	int l;
	int r;
	while(*src) {
		if (i >= SS_CONF_IPMAX) {
			SS_LOG_WARNING("[%s] ip too much, must <= %u", svr_name, SS_CONF_IPMAX);
		return SS_CONF_CHECKFAIL;
		}

		l = strspn(src, " \t");
		src += l;
		j = 0;
		svr_ips[i].num = 0;
		while(*src) {
			r = strcspn(src, " \t/");
			if (r >= SS_CONF_IPLENMAX) {
				SS_LOG_WARNING("[%s] ip %*s too long", svr_name, r, src);
				return SS_CONF_CHECKFAIL;
			}
			strncpy(svr_ips[i].name[j], src, r);
			svr_ips[i].name[j][r] = '\0';
			if (check_str_ipv4(svr_ips[i].name[j]) != SS_CONF_CHECKSUCCESS) {
				return SS_CONF_CHECKFAIL;
			}
			src += r;
			++j;
			if (*src != '/') break;
			++src;
		}
		++i;
		svr_ips[i].num = j;
	}

	*num = i;
	return SS_CONF_SUCCESS;
		
}

int ss_conf_getreqsvr(const ss_conf_data_t *conf, const char *product_name, const char *module_name, ss_request_svr_t *value, const char *comment)
{
	if (NULL == conf) {
		return SS_CONF_NULL;
	}

	if (NULL == value && SS_CONF_READCONF == conf->build) {
		return SS_CONF_NULL;
	}

	if (NULL == module_name) {
		return SS_CONF_NULL;
	}

	if (SS_CONF_READCONF != conf->build) {
		if (write_comment(conf->conf_file,"") != SS_CONF_SUCCESS) {
			return SS_CONF_NULL;
		}

		if (write_comment(conf->conf_file, comment) != SS_CONF_SUCCESS) {
			return SS_CONF_NULL;
		}
	}

	int ret = SS_CONF_SUCCESS;
	char conf_name[WORD_SIZE];
	if (NULL == product_name) {
		snprintf(conf_name, sizeof(conf_name), "%s", module_name);
	} else {
		snprintf(conf_name, sizeof(conf_name), "%s_%s", product_name, module_name);
	}
	char svr_name[WORD_SIZE];

	//the name of server
	int item_ret;
	unsigned int tmp;
	char str_tmp[WORD_SIZE];

	snprintf(svr_name, sizeof(svr_name), "_svr_%s_name", conf_name);
	item_ret = ss_conf_getnstr(conf, svr_name, str_tmp, sizeof(str_tmp), "request server name");
	if (item_ret != SS_CONF_SUCCESS) {
		ret = item_ret;
	}
	if (value != NULL && SS_CONF_READCONF == conf->build) {
		snprintf(value->svr_name, sizeof(value->svr_name), "%s", str_tmp);
	}

	//get ip_list
	snprintf(svr_name, sizeof(svr_name), "_svr_%s_ip", conf_name);
	item_ret = ss_conf_getnstr(conf, svr_name, str_tmp, sizeof(str_tmp), "request ip list");
	if (item_ret != SS_CONF_SUCCESS) {
		item_ret = ret;
	} else {
		item_ret = parse_iplist(svr_name, str_tmp,value->ip_list, &value->num);
		if (item_ret != SS_CONF_SUCCESS) {
			ret = item_ret;
		}
	}

	//get port
	snprintf(svr_name, sizeof(svr_name), "_svr_%s_port", conf_name);
	item_ret = ss_conf_getuint(conf, svr_name, &tmp, "server port");
	if (item_ret != SS_CONF_SUCCESS) {
		ret = item_ret;
	}
	if (value != NULL && SS_CONF_READCONF == conf->build) {
		value->port = tmp;
	}

	//get read timeout
	snprintf(svr_name, sizeof(svr_name), "_svr_%s_readtimeout", conf_name);
	item_ret = ss_conf_getuint(conf, svr_name, &tmp, "server read timeout");
	if (item_ret != SS_CONF_SUCCESS) {
		ret = item_ret;
	}
	if (value != NULL && SS_CONF_READCONF == conf->build) {
		value->read_timeout = tmp;
	}

	//get write timeout
	snprintf(svr_name, sizeof(svr_name), "_svr_%s_writetimeout", conf_name);
	item_ret = ss_conf_getuint(conf, svr_name, &tmp, "server write  timeout");
	if (item_ret != SS_CONF_SUCCESS) {
		ret = item_ret;
	}

	if (value != NULL && SS_CONF_READCONF == conf->build) {
		value->write_timeout = tmp;
	}

	//get connect timeout
	snprintf(svr_name, sizeof(svr_name), "_svr_%s_connecttimeout", conf_name);
	item_ret = ss_conf_getuint(conf, svr_name, &tmp, "connect timeout");
	if (item_ret != SS_CONF_SUCCESS) {
		ret = item_ret;
	}
	if (value != NULL && SS_CONF_READCONF == conf->build) {
		value->connect_timeout = tmp;
	}

	//get max connect num
	snprintf(svr_name, sizeof(svr_name), "_svr_%s_maxconnect", conf_name);
	item_ret = ss_conf_getuint(conf, svr_name, &tmp, "max connect");
	if (item_ret != SS_CONF_SUCCESS) {
		ret = item_ret;
	}
	if (value != NULL && SS_CONF_READCONF == conf->build) {
		value->max_connect = tmp;
	}

	//get retry time
	snprintf(svr_name, sizeof(svr_name), "_svr_%s_retry", conf_name);
	item_ret = ss_conf_getuint(conf, svr_name, &tmp, "retry time");
	if (item_ret != SS_CONF_SUCCESS) {
		ret = item_ret;
	}
	if (value != NULL && SS_CONF_READCONF == conf->build) {
		value->retry = tmp;
	}

	//get connect type
	snprintf(svr_name, sizeof(svr_name), "_svr_%s_connecttype", conf_name);
	item_ret = ss_conf_getuint(conf, svr_name, &tmp, "connect type");
	if (item_ret = SS_CONF_SUCCESS) {
		ret = item_ret;
	}
	if (value != NULL && SS_CONF_READCONF == conf->build) {
		value->connect_type = tmp;
	}

	//get linger
	snprintf(svr_name, sizeof(svr_name), "_svr_%s_linger", conf_name);
	item_ret = ss_conf_getuint(conf, svr_name, &tmp, "linger");
	if (item_ret == SS_CONF_SUCCESS) {
		ret = item_ret;
	}
	if (value != NULL && SS_CONF_READCONF == conf->build) {
		value->linger = tmp;
	}

	if (SS_CONF_READCONF != conf->build) {
		if (fputc('\n', conf->conf_file) == EOF) {
			return SS_CONF_NULL;
		}
	}

	return ret;
}
