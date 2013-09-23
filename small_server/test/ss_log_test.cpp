#include <gtest/gtest.h>
#include "ss_log.h"

TEST(ss_log, test_init)
{
	int ret = ss_log_init();
	ss_log_close();
	EXPECT_EQ(0, ret);
	struct stat statbuf;
	stat("./log/sslog", &statbuf);
	ret = S_ISREG(statbuf.st_mode);
	EXPECT_EQ(1, ret);
	stat("./log/sslog.wf", &statbuf);
	ret = S_ISREG(statbuf.st_mode);
	EXPECT_EQ(1, ret);

	ret = ss_log_init("./log","test");
	ss_log_close();
	EXPECT_EQ(0, ret);
	stat("./log/testlog", &statbuf);
	ret = S_ISREG(statbuf.st_mode);
	EXPECT_EQ(1, ret);
	stat("./log/testlog.wf", &statbuf);
	ret = S_ISREG(statbuf.st_mode);
	EXPECT_EQ(1, ret);

	ret = ss_log_initthread("testthread");
	ss_log_closethread();
	EXPECT_EQ(0, ret);


	ret = ss_log_init("./log", "test1", 1000, 16, 0, LIB_LOGNEWFILE);
	ss_log_close();
	EXPECT_EQ(0, ret);
	ret = ss_log_initthread("testthread");
	ss_log_closethread();
	EXPECT_EQ(0, ret);

	system("rm -rf log");

}

TEST(ss_log, test_basic)
{
	unsigned int ret;
	ret = ss_log_setbasic(SS_LOG_LOGID,"123");
	EXPECT_EQ(3, ret);
	char *p = NULL;
	p = ss_log_getbasic(SS_LOG_LOGID);
	EXPECT_STREQ("123", p);

	ret = ss_log_setbasic(SS_LOG_PROCTIME, "321456");
	EXPECT_EQ(6, ret);
	p = ss_log_getbasic(SS_LOG_PROCTIME);
	EXPECT_STREQ("321456", p);

	ret = ss_log_setbasic(SS_LOG_REQIP,"192.168.1.1");
	EXPECT_EQ(11, ret);
	p = ss_log_getbasic(SS_LOG_REQIP);
	EXPECT_STREQ("192.168.1.1", p);

	ret = ss_log_setbasic(SS_LOG_REQSVR,"post");
	EXPECT_EQ(4, ret);
	p = ss_log_getbasic(SS_LOG_REQSVR);
	EXPECT_STREQ("post", p);

	ret = ss_log_setbasic(SS_LOG_ERRNO,"0");
	EXPECT_EQ(1, ret);
	p = ss_log_getbasic(SS_LOG_ERRNO);
	EXPECT_STREQ("0", p);

	ret = ss_log_setbasic(SS_LOG_CMDNO,"rm");
	EXPECT_EQ(2, ret);
	p = ss_log_getbasic(SS_LOG_CMDNO);
	EXPECT_STREQ("rm", p);

	ret = ss_log_setbasic(SS_LOG_SVRNAME,"smtp");
	EXPECT_EQ(4, ret);
	p = ss_log_getbasic(SS_LOG_SVRNAME);
	EXPECT_STREQ("smtp", p);

	ret = ss_log_setlogid(100);
	EXPECT_EQ(100, ret);
	ret = ss_log_getlogid();
	EXPECT_EQ(100, ret);

	ret = ss_log_clearlogid();
	EXPECT_EQ(0, ret);

	ret = ss_log_pushnotice("key", "value");
	EXPECT_EQ(10, ret);
	p = ss_log_popnotice();
	EXPECT_STREQ("key:value ", p);
	ret = ss_log_clearnotice();
	EXPECT_EQ(0, ret);

}

TEST(ss_log, test_print_log)
{
	ss_log_init();
	ss_log_setbasic(SS_LOG_LOGID, "1");
	ss_log_setbasic(SS_LOG_PROCTIME, "20130923");
	ss_log_setbasic(SS_LOG_REQIP, "192.168.1.1");
	ss_log_setbasic(SS_LOG_REQSVR, "post");
	ss_log_setbasic(SS_LOG_ERRNO, "0");
	ss_log_setbasic(SS_LOG_CMDNO, "0");
	ss_log_setbasic(SS_LOG_SVRNAME, "test");

	SS_LOG_MONITOR("hahaha %d", 1);
	SS_LOG_FATAL("fatal %d", 2);
	SS_LOG_WARNING("warning %d", 3);
	SS_LOG_NOTICE("notice %d", 4);
	SS_LOG_TRACE("Trace %d", 5);
	SS_LOG_DEBUG("debug %d", 6);

	system("rm -rf log");

}

int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
