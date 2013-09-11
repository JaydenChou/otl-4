#include "gtest/gtest.h"
#include "lib_log.h"

TEST(lib_log, test_log_fun)
{
	const char *log_path = "/root/git-project/otl/build/output_lib/test_log_path";

	const char *proc_name = "test";
	int maxsize = 1024*1024;

	const char *log_name = "testlog";
	const char *log_name_wf = "testlog.wf";
	int ret = lib_openlog(log_path, proc_name, NULL, maxsize);
	ASSERT_EQ(0, ret);
	char *full_path = (char *) malloc(sizeof(char)*(strlen(log_path)+15));
	memset(full_path, 0, sizeof(full_path));
	strncpy(full_path, log_path, strlen(log_path));
	strncpy(full_path+strlen(log_path),"/", 1);
	strncpy(full_path+strlen(log_path)+1,log_name, sizeof(log_name));
	strncpy(full_path+strlen(full_path),"log", 3);

	ASSERT_STREQ("/root/git-project/otl/build/output_lib/test_log_path/testlog", full_path);

	struct stat buff_stat;
	ret = stat(full_path, &buff_stat);
	ASSERT_LT(0, buff_stat.st_size);
	ASSERT_EQ(0, ret);

	ret = lib_writelog(LIB_LOG_FATAL,"%s-%s","haha", "heihei");
	ASSERT_EQ(0, ret);

	ret = lib_closelog(false);
	ASSERT_EQ(0, ret);

}

TEST(lib_log, test_writelog)
{
	int ret = lib_writelog(LIB_LOG_FATAL,"the log content will be output stderr\n");
	ASSERT_EQ(0, ret);
}
int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
