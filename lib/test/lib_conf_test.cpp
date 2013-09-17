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

	int ret = lib_readconf("/root/git-project/otl/output", "testconf", p_conf);
	ASSERT_EQ(-1, ret);
	
	
	FILE* fp = fopen("testconf", "w");
	fprintf(fp, "#       \n");
	fprintf(fp, "        \n");
	fprintf(fp, "  ::  \n");
	fprintf(fp, "name : value\n");
	fprintf(fp, "     name1:value1\n");
	fprintf(fp, "   name2       :         value2\n");
	fprintf(fp, "name3:value3\n");
	fprintf(fp, "name4:::::::::value4\n");
	fprintf(fp, "       name5                ::value5\n");
	fprintf(fp, "name6:   value6\n");
	fprintf(fp, "name7:1234567890\n");
	fprintf(fp, "name8:7890\n");
	fprintf(fp, "name9:78902   \n");
	fprintf(fp, "  name10  :  789034   \n");
	fclose(fp);
	
	ret = lib_readconf("/root/git-project/otl/output", "testconf", p_conf);
	ASSERT_EQ(0,ret);
	
	int i = 0;
	EXPECT_STREQ("name",p_conf->item[i].name);
	EXPECT_STREQ("value",p_conf->item[i].value);
	EXPECT_STREQ("name1",p_conf->item[++i].name);
	EXPECT_STREQ("value1",p_conf->item[i].value);
	EXPECT_STREQ("name2",p_conf->item[++i].name);
	EXPECT_STREQ("value2",p_conf->item[i].value);
	EXPECT_STREQ("name3",p_conf->item[++i].name);
	EXPECT_STREQ("value3",p_conf->item[i].value);
	EXPECT_STREQ("name4",p_conf->item[++i].name);
	EXPECT_STREQ("::::::::value4",p_conf->item[i].value);
	EXPECT_STREQ("name5",p_conf->item[++i].name);
	EXPECT_STREQ(":value5",p_conf->item[i].value);
	EXPECT_STREQ("name6",p_conf->item[++i].name);
	EXPECT_STREQ("value6",p_conf->item[i].value);
	EXPECT_STREQ("name7",p_conf->item[++i].name);
	EXPECT_STREQ("1234567890",p_conf->item[i].value);
	EXPECT_STREQ("name8",p_conf->item[++i].name);
	EXPECT_STREQ("7890",p_conf->item[i].value);
	EXPECT_STREQ("name9",p_conf->item[++i].name);
	EXPECT_STREQ("78902",p_conf->item[i].value);
	EXPECT_STREQ("name10",p_conf->item[++i].name);
	EXPECT_STREQ("789034",p_conf->item[i].value);

	remove("/root/git-project/otl/output/testconf");

}


TEST(lib_conf, test_lib_readconf_ex_nodiff)
{
	lib_conf_data_t *p_conf;
	p_conf = lib_initconf(0);

	FILE* fp = fopen("testconf1", "w");
	fprintf(fp,"$include : testconf2\n");
	fprintf(fp," key : value \n");
	fclose(fp);

	fp = fopen("testconf2", "w");
	fprintf(fp, "key2 : value2\n");
	fclose(fp);

	int ret = lib_readconf_ex("/root/git-project/otl/output", "testconf1", p_conf);
	EXPECT_EQ(0, ret);
	EXPECT_EQ(3, p_conf->num);
	EXPECT_STREQ("$include", p_conf->item[0].name);
	EXPECT_STREQ("testconf2", p_conf->item[0].value);
	EXPECT_STREQ("key", p_conf->item[1].name);
	EXPECT_STREQ("value", p_conf->item[1].value);
	EXPECT_STREQ("key2", p_conf->item[2].name);
	EXPECT_STREQ("value2", p_conf->item[2].value);

	lib_freeconf(p_conf);
	p_conf = NULL;

	remove("/root/git-project/otl/output/testconf1");
	remove("/root/git-project/otl/output/testconf2");

}

TEST(lib_conf, test_lib_readconf_ex_diff)
{
	lib_conf_data_t *p_conf;
	p_conf = lib_initconf(0);
	
	FILE* fp = fopen("testconf3", "w");
	fprintf(fp, "key1:value1\n");
	fprintf(fp, "key2:value2\n");
	fprintf(fp, "$include:testconf4\n");
	fprintf(fp, "key3:value3\n");
	fclose(fp);
	
	fp = fopen("testconf4", "w");
	fprintf(fp, "key1:value2\n");
	fclose(fp);
	
	int ret = lib_readconf_ex("/root/git-project/otl/output/", "testconf3", p_conf);
	
	EXPECT_EQ(0, ret);
	
	EXPECT_EQ(5, p_conf->num);
	EXPECT_STREQ("testconf4", p_conf->item[2].value);
	EXPECT_STREQ("value1", p_conf->item[0].value);
	
	//lib_freeconf(p_conf);
	//p_conf = NULL;

	remove("/root/git-project/otl/output/testconf3");
	remove("/root/git-project/otl/output/testconf4");
	
}

TEST(lib_conf, test_lib_writeconf)
{
	lib_conf_data_t *p_conf;

	p_conf = lib_initconf(0);
	FILE *fp = fopen("testconf5", "w");
	fprintf(fp, "key1:value1\n");
	fclose(fp);
	
	int ret = lib_readconf(".", "testconf5", p_conf);

	ret = lib_writeconf(".", "testconf6", p_conf);

	fp = fopen("testconf6", "r");
	EXPECT_TRUE(NULL != fp);
	char buf[LINE_SIZE];
	fgets(buf, LINE_SIZE, fp);

	EXPECT_STREQ("key1:value1\n", buf);

	remove("./testconf5");
	remove("./testconf6");

	EXPECT_EQ(0, ret);


}

TEST(lib_conf, test_lib_modifyconfstr)
{

	lib_conf_data_t *p_conf;

	p_conf = lib_initconf(0);
	FILE *fp = fopen("testconf7", "w");
	fprintf(fp, "key1:value1\n");
	fclose(fp);
	
	int ret = lib_readconf(".", "testconf7", p_conf);

	EXPECT_EQ(0, ret);

	ret = lib_modifyconfstr(p_conf, "key1", "value2");
	EXPECT_EQ(0, ret);
	EXPECT_STREQ("value2", p_conf->item[0].value);

	remove("./testconf7");

}

int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
