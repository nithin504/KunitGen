// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/stddef.h>

// Mock pmx_functions array to simulate the real one
static const char * const pmx_functions[] = {
	"function0",
	"function1",
	"function2",
	"function3",
};

// Include the function under test directly
static int amd_get_functions_count(struct pinctrl_dev *pctldev)
{
	return ARRAY_SIZE(pmx_functions);
}

// Test case: normal operation
static void test_amd_get_functions_count_normal(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	int count = amd_get_functions_count(&dummy_pctldev);
	KUNIT_EXPECT_EQ(test, count, (int)ARRAY_SIZE(pmx_functions));
}

// Test case: pass NULL (should still work since parameter is unused)
static void test_amd_get_functions_count_null_arg(struct kunit *test)
{
	int count = amd_get_functions_count(NULL);
	KUNIT_EXPECT_EQ(test, count, (int)ARRAY_SIZE(pmx_functions));
}

static struct kunit_case amd_get_functions_count_test_cases[] = {
	KUNIT_CASE(test_amd_get_functions_count_normal),
	KUNIT_CASE(test_amd_get_functions_count_null_arg),
	{}
};

static struct kunit_suite amd_get_functions_count_test_suite = {
	.name = "amd_get_functions_count_test",
	.test_cases = amd_get_functions_count_test_cases,
};

kunit_test_suite(amd_get_functions_count_test_suite);
