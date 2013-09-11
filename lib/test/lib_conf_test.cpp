#include "gtest/gtest.h"

#include "lib_conf.h"

TEST(lib_conf, test_initconf)
{

	lib_conf_data_t *p_conf;
	p_conf = lib_initconf(100);
	EXPECT_TRUE(NULL != p_conf);
	EXPECT_EQ(1024, p_conf->size);
	EXPECT_EQ(0, p_conf->num);
	EXPECT_FALSE(NULL == p_conf->item);
}


int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
