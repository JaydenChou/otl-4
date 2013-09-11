#include "gtest/gtest.h"
#include "lib_log.h"
TEST(lib_log, open_log)
{
	const char *log_path = "/root/git-project/otl/build/output_lib/test_log_path";
	lib_logstat_t log_stat;
	log_stat.level = 0x1;
	log_stat.syslog_level = 0x1;
	log_stat.spec = LIB_LOGTTY;

	const char *proc_name = "test";
	int maxsize = 1024*1024;

	lib_openlog(log_path, proc_name, &log_stat, maxsize);
//	ASSERT_STREQ("test_log_path", g_log_path);

}
int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
