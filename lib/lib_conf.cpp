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
/*
int main()
{
	return 0;
}
*/
