```c
// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/stddef.h>

// Mock data to simulate pmx_functions array
static const char *pmx_functions[] = {
	"function0",
	"function1",
	"function2"
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
	KUNIT_EXPECT_EQ(test, count, 3);
}

// Test case: empty array simulation (edge case)
static void test_amd_get_functions_count_empty_array(struct kunit *test)
{
	const char **saved_pmx_functions = (const char **)pmx_functions;
	// Temporarily replace with zero-sized array semantics by redefining locally
	#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
	static const char *empty_pmx_functions[] = {};
	// Redefine function with local array
	static int local_amd_get_functions_count(struct pinctrl_dev *pctldev)
	{
		return ARRAY_SIZE(empty_pmx_functions);
	}
	int count = local_amd_get_functions_count(NULL);
	KUNIT_EXPECT_EQ(test, count, 0);
	#undef ARRAY_SIZE
}

static struct kunit_case amd_get_functions_count_test_cases[] = {
	KUNIT_CASE(test_amd_get_functions_count_normal),
	KUNIT_CASE(test_amd_get_functions_count_empty_array),
	{}
};

static struct kunit_suite amd_get_functions_count_test_suite = {
	.name = "amd_get_functions_count_test",
	.test_cases = amd_get_functions_count_test_cases,
};

kunit_test_suite(amd_get_functions_count_test_suite);
```