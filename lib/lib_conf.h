#ifndef __LIB_CONF_H__
#define __LIB_CONF_H__

#include "lib_def.h"

typedef struct lib_conf_item {
	char name[WORD_SIZE];
	char value[WORD_SIZE];
} lib_conf_item_t;

typedef struct lib_conf_data {
	int num;
	int size;
	lib_conf_item_t *item;
} lib_conf_data_t;

/*
 * @brief init a configure struct
 * @param [in] confnum, the number of configure num, if it less than 1024, it will be set 1024.
 * @return NULL fail, or a lib_conf_data pointer
 */
lib_conf_data_t* lib_initconf(int confnum);
#endif
