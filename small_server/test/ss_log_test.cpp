#include <gtest/gtest.h>
#include "ss_log.h"

TEST(ss_log, test_init)
{
	int ret = 0;
	EXPECT_EQ(0, ret);
}

int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
