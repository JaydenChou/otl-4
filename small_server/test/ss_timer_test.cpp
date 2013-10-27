#include <gtest/gtest.h>
#include "ss_timer.h"

TEST(ss_timer, timer_init)
{
	pss_timer_t timer = (pss_timer_t) malloc(sizeof(ss_timer_t));
	ASSERT_TRUE(NULL != timer);
	
	int ret = ss_timer_init(timer);
	ASSERT_EQ(0, ret);
	ASSERT_EQ(0, timer->timer_cur);
}

TEST(ss_timer, timer_reset)
{
	pss_timer_t timer = (pss_timer_t) malloc(sizeof(ss_timer_t));
	ASSERT_TRUE(NULL != timer);
	ss_timer_init(timer);
	timer->timer_cur = 10;
	
	int ret = ss_timer_reset(timer);
	ASSERT_EQ(0, ret);
	ASSERT_EQ(0, timer->timer_cur);
}


TEST(ss_timer, ss_timer_settask_and_endtask_and_gettask)
{

	pss_timer_t timer = (pss_timer_t) malloc(sizeof(ss_timer_t));
	ASSERT_TRUE(NULL != timer);
	ss_timer_init(timer);

	int ret = ss_timer_settask(timer, "test");
	ASSERT_EQ(0, ret);
	ASSERT_EQ(1, timer->timer_cur);
	ASSERT_STREQ("test", timer->timername[0]);

	sleep(1);
	ret = ss_timer_gettask(timer, "test");

	ASSERT_LT(0, ret);
	
	char value[20];
	ss_timer_reset(timer);
	ss_timer_settask(timer, "test");
	sleep(1);
	ss_timer_gettaskstring(timer, "test", value, sizeof(value));
	ASSERT_EQ(20, sizeof(value));
	puts(value);

	ss_timer_reset(timer);
	ss_timer_settask(timer, "test");
	sleep(1);
	char *p = ss_timer_gettaskformat(timer);
	puts(p);

}

int main(int argc, char *argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
