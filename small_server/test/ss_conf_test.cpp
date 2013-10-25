#include <gtest/gtest.h>
#include "ss_conf.h"

static const char *path = "./";
static const char *filename = "conf_read";
TEST(ss_conf, test_ss_conf_init)
{
	ss_conf_data_t *conf = NULL;
	conf = ss_conf_init(path, filename);

	ASSERT_TRUE(NULL != conf);

	int ret;
	int num_int;
	const int default_int = 123;

	ret = ss_conf_getint(conf, "int", &num_int, NULL);
	ASSERT_EQ(SS_CONF_SUCCESS, ret);
	ASSERT_EQ(10, num_int);
	ret = ss_conf_getint(conf, "int_default", &num_int, NULL, &default_int);
	ASSERT_EQ(SS_CONF_DEFAULT, ret);
	ASSERT_EQ(123, num_int);


	unsigned int num_uint;
	const unsigned int default_uint = 123;

	ret = ss_conf_getuint(conf, "uint", &num_uint, NULL);
	ASSERT_EQ(SS_CONF_SUCCESS, ret);
	ASSERT_EQ(65534, num_uint);
	ret = ss_conf_getuint(conf, "uint_default", &num_uint, NULL, &default_uint);
	ASSERT_EQ(SS_CONF_DEFAULT, ret);
	ASSERT_EQ(123, num_uint);

	long long  num_int64;
	const long long default_int64 = 123;

	ret = ss_conf_getint64(conf, "int64", &num_int64, NULL);
	ASSERT_EQ(SS_CONF_SUCCESS, ret);
	ASSERT_EQ(123456789, num_int64);
	ret = ss_conf_getint64(conf, "int64_default", &num_int64, NULL, &default_int64);
	ASSERT_EQ(SS_CONF_DEFAULT, ret);
	ASSERT_EQ(123, num_int64);

	unsigned long long  num_uint64;
	const unsigned long long  default_uint64 = 123;

	ret = ss_conf_getuint64(conf, "uint64", &num_uint64, NULL);
	ASSERT_EQ(SS_CONF_SUCCESS, ret);
	ASSERT_EQ(987654321, num_uint64);
	ret = ss_conf_getuint64(conf, "uint64_default", &num_uint64, NULL, &default_uint64);
	ASSERT_EQ(SS_CONF_DEFAULT, ret);
	ASSERT_EQ(123, num_uint64);

	float  num_float;
	const float default_float = 1.23;

	ret = ss_conf_getfloat(conf, "float", &num_float, NULL);
	ASSERT_EQ(SS_CONF_SUCCESS, ret);
	ASSERT_FLOAT_EQ(1234.5, num_float);
	ret = ss_conf_getfloat(conf, "float_default", &num_float, NULL, &default_float);
	ASSERT_EQ(SS_CONF_DEFAULT, ret);
	ASSERT_FLOAT_EQ(1.23, num_float);

	char str[WORD_SIZE];
	const char *default_str = "default_str";
	
	ret = ss_conf_getnstr(conf, "str", str, sizeof(str), NULL);
	ASSERT_EQ(SS_CONF_SUCCESS, ret);
	ASSERT_STREQ("string", str);
	ret = ss_conf_getnstr(conf, "default_str", str, sizeof(str), NULL, default_str);
	ASSERT_EQ(SS_CONF_DEFAULT, ret);
	ASSERT_STREQ("default_str", str);

	ss_svr_t svr_data;
	ret = ss_conf_getsvr(conf, NULL, "test", &svr_data, NULL);
	ASSERT_EQ(SS_CONF_SUCCESS, ret);
	ASSERT_STREQ("test", svr_data.svr_name);
	ASSERT_EQ(80, svr_data.port);
	ASSERT_EQ(2000, svr_data.read_timeout);
	ASSERT_EQ(2000, svr_data.write_timeout);
	ASSERT_EQ(100, svr_data.threadnum);
	ASSERT_EQ(0, svr_data.connect_type);
	ASSERT_EQ(1, svr_data.server_type);
	ASSERT_EQ(100, svr_data.queue_size);
	ASSERT_EQ(100, svr_data.sock_size);

	ss_request_svr_t req_svr;
	ret = ss_conf_getreqsvr(conf, NULL, "request", &req_svr, NULL);
	ASSERT_STREQ("test", req_svr.svr_name);
	ASSERT_EQ(80, req_svr.port);
	ASSERT_EQ(1000, req_svr.read_timeout);
	ASSERT_EQ(1000, req_svr.write_timeout);
	ASSERT_EQ(3, req_svr.num);
	ASSERT_EQ(1000, req_svr.connect_timeout);
	ASSERT_EQ(10, req_svr.max_connect);
	ASSERT_EQ(3, req_svr.retry);
	ASSERT_EQ(0, req_svr.connect_type);
	ASSERT_EQ(0, req_svr.linger);
	ASSERT_EQ(SS_CONF_SUCCESS, ret);
	ASSERT_EQ(2, req_svr.ip_list[0].num);
	ASSERT_STREQ("192.168.0.1", req_svr.ip_list[0].name[0]);
	ASSERT_STREQ("192.168.1.1", req_svr.ip_list[0].name[1]);
	ASSERT_EQ(1, req_svr.ip_list[1].num);
	ASSERT_STREQ("172.0.1.1", req_svr.ip_list[1].name[0]);
	ASSERT_EQ(1, req_svr.ip_list[2].num);
	ASSERT_STREQ("255.255.255.0", req_svr.ip_list[2].name[0]);

	ret = ss_conf_close(conf);
	//ASSERT_TRUE(NULL == conf->option);
	//ASSERT_TRUE(NULL == conf->range);
	ASSERT_TRUE(NULL == conf->conf_file);
	ASSERT_EQ(SS_CONF_SUCCESS, ret);

}
int main(int argc, char *argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
