// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/io.h>

/* Mock structures */
struct amd_gpio {
	int ngroups;
};

/* Mock function */
static struct amd_gpio mock_gpio_dev;

static inline void *pinctrl_dev_get_drvdata(struct pinctrl_dev *pctldev)
{
	return &mock_gpio_dev;
}

/* Function under test (copied and made static) */
static int amd_get_groups_count(struct pinctrl_dev *pctldev)
{
	struct amd_gpio *gpio_dev = pinctrl_dev_get_drvdata(pctldev);

	return gpio_dev->ngroups;
}

/* Test cases */
static void test_amd_get_groups_count_valid(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	mock_gpio_dev.ngroups = 5;

	int ret = amd_get_groups_count(&dummy_pctldev);
	KUNIT_EXPECT_EQ(test, ret, 5);
}

static void test_amd_get_groups_count_zero(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	mock_gpio_dev.ngroups = 0;

	int ret = amd_get_groups_count(&dummy_pctldev);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_get_groups_count_negative(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	mock_gpio_dev.ngroups = -3;

	int ret = amd_get_groups_count(&dummy_pctldev);
	KUNIT_EXPECT_EQ(test, ret, -3);
}

static struct kunit_case amd_get_groups_count_test_cases[] = {
	KUNIT_CASE(test_amd_get_groups_count_valid),
	KUNIT_CASE(test_amd_get_groups_count_zero),
	KUNIT_CASE(test_amd_get_groups_count_negative),
	{}
};

static struct kunit_suite amd_get_groups_count_test_suite = {
	.name = "amd_get_groups_count_test",
	.test_cases = amd_get_groups_count_test_cases,
};

kunit_test_suite(amd_get_groups_count_test_suite);
