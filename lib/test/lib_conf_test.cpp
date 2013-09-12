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
	lib_freeconf(p_conf);
}

TEST(lib_conf, test_readconf)
{
	lib_conf_data_t *p_conf;
	p_conf = lib_initconf(0);

	int ret = lib_readconf("/root/git-project/otl/build/output_lib", "testconf", p_conf);

	//ASSERT_EQ(-1, ret);
	ASSERT_EQ(0,ret);
	int i = 0;
	for(; i < 3; i++) {
		printf("%s : %s\n", p_conf->item[i].name, p_conf->item[i].value);
	}


}


int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
