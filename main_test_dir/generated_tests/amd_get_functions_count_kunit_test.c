// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>

// Mock data for pmx_functions array
static const char * const pmx_functions[] = {
	"function0",
	"function1",
	"function2",
};

// Include the function under test directly
static int amd_get_functions_count(struct pinctrl_dev *pctldev)
{
	return ARRAY_SIZE(pmx_functions);
}

// Test case: valid pinctrl device
static void test_amd_get_functions_count_valid(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	int count = amd_get_functions_count(&dummy_pctldev);
	KUNIT_EXPECT_EQ(test, count, 3);
}

// Test case: NULL pinctrl device (should still work since it's unused)
static void test_amd_get_functions_count_null_pctldev(struct kunit *test)
{
	int count = amd_get_functions_count(NULL);
	KUNIT_EXPECT_EQ(test, count, 3);
}

static struct kunit_case amd_get_functions_count_test_cases[] = {
	KUNIT_CASE(test_amd_get_functions_count_valid),
	KUNIT_CASE(test_amd_get_functions_count_null_pctldev),
	{}
};

static struct kunit_suite amd_get_functions_count_test_suite = {
	.name = "amd_get_functions_count_test",
	.test_cases = amd_get_functions_count_test_cases,
};

kunit_test_suite(amd_get_functions_count_test_suite);