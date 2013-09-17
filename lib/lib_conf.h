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

/*
 * @brief free a configure struct
 * @param [in] the configure data pointer which need to be free.
 * @return 1 always success.
 */
int lib_freeconf(lib_conf_data_t* p_conf);

/*
 * @brief read configure file content to confdata
 * @param [in] work_path: work directory
 * @param [in] filename: configure filename
 * @param [in/out] p_conf: the pointer configure strcut
 * @return 0 success, -1 fail
 */
int lib_readconf(const char* work_path, const char* filename, lib_conf_data_t* p_conf);

/*
 * @brief read the contents of configure to structure, support $include grammar.
 *		it will continue to read the specified configure, if you specify the "conf2",
 *		the program will read the "conf2" file after reading the configure file.
 *		if the conf2 in the presence of conf, so as to master file.
 * @param[in/out] work_path: the configure's path
 * @param[in/out] fname: the configure file name
 * @param[in/out] p_conf: the structure of conf
 * @return 0 success other fail
 */
int lib_readconf_ex(const char* work_path, const char* fname, lib_conf_data_t* p_conf);

/*
 * @brief write the conf struct to file
 * @param[in] work_path: work directory
 * @param[in] fname: file name
 * @param[in] p_conf: conf strcut data
 * @return 0 success -1 failed
 */
int lib_writeconf(const char* work_path, const char* fname, lib_conf_data_t* p_conf);

/*
 * @brief modify the item value in the configure struct
 * @param[in] p_conf: configure struct 
 * @param[in] name: the modified item name
 * @param[in] value:  the value to be set to the item
 * @return 0 success -1 item isn't exist
 */
int lib_modifyconfstr(lib_conf_data_t *p_conf, char *name, char* value);

/*
 * @brief add new item to configure struct
 * @param[in] p_conf: configure strcut
 * @param[in] name: the item's name
 * @param[in] value: the item's value
 * @return 0 success -1 failed
 */
int lib_addconfstr(lib_conf_data_t *p_conf, char *name, char *value);


#endif

