// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/io.h>

// Mock structure definitions
struct amd_gpio {
	int ngroups;
};

// Function under test (copied directly)
static int amd_get_groups_count(struct pinctrl_dev *pctldev)
{
	struct amd_gpio *gpio_dev = pinctrl_dev_get_drvdata(pctldev);

	return gpio_dev->ngroups;
}

// Mocking pinctrl_dev_get_drvdata
static void *mock_drvdata;

void *pinctrl_dev_get_drvdata(struct pinctrl_dev *pctldev)
{
	return mock_drvdata;
}

// Test case: normal operation
static void test_amd_get_groups_count_normal(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	struct amd_gpio gpio_dev = {
		.ngroups = 5,
	};

	mock_drvdata = &gpio_dev;

	int ret = amd_get_groups_count(&dummy_pctldev);
	KUNIT_EXPECT_EQ(test, ret, 5);
}

// Test case: zero groups
static void test_amd_get_groups_count_zero_groups(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	struct amd_gpio gpio_dev = {
		.ngroups = 0,
	};

	mock_drvdata = &gpio_dev;

	int ret = amd_get_groups_count(&dummy_pctldev);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

// Test case: large number of groups
static void test_amd_get_groups_count_large_number(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	struct amd_gpio gpio_dev = {
		.ngroups = 10000,
	};

	mock_drvdata = &gpio_dev;

	int ret = amd_get_groups_count(&dummy_pctldev);
	KUNIT_EXPECT_EQ(test, ret, 10000);
}

// Test case: negative group count (invalid but possible in struct)
static void test_amd_get_groups_count_negative_groups(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	struct amd_gpio gpio_dev = {
		.ngroups = -1,
	};

	mock_drvdata = &gpio_dev;

	int ret = amd_get_groups_count(&dummy_pctldev);
	KUNIT_EXPECT_EQ(test, ret, -1);
}

static struct kunit_case amd_get_groups_count_test_cases[] = {
	KUNIT_CASE(test_amd_get_groups_count_normal),
	KUNIT_CASE(test_amd_get_groups_count_zero_groups),
	KUNIT_CASE(test_amd_get_groups_count_large_number),
	KUNIT_CASE(test_amd_get_groups_count_negative_groups),
	{}
};

static struct kunit_suite amd_get_groups_count_test_suite = {
	.name = "amd_get_groups_count_test",
	.test_cases = amd_get_groups_count_test_cases,
};

kunit_test_suite(amd_get_groups_count_test_suite);
