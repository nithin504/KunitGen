
// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>

static const char * const pmx_functions[] = {
	"test_function1",
	"test_function2",
	"test_function3"
};

static int amd_get_functions_count(struct pinctrl_dev *pctldev)
{
	return ARRAY_SIZE(pmx_functions);
}

static void test_amd_get_functions_count_valid(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	int count;
	
	count = amd_get_functions_count(&pctldev);
	KUNIT_EXPECT_EQ(test, count, ARRAY_SIZE(pmx_functions));
}

static void test_amd_get_functions_count_null_pctldev(struct kunit *test)
{
	int count;
	
	count = amd_get_functions_count(NULL);
	KUNIT_EXPECT_EQ(test, count, ARRAY_SIZE(pmx_functions));
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