// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/errno.h>

static int amd_get_group_pins(struct pinctrl_dev *pctldev, unsigned group,
			      const unsigned **pins, unsigned *npins)
{
	return -ENOTSUPP;
}

static int amd_pinconf_set(struct pinctrl_dev *pctldev, unsigned pin,
			   unsigned long *configs, unsigned num_configs)
{
	return -ENOTSUPP;
}

static int amd_pinconf_group_set(struct pinctrl_dev *pctldev,
				unsigned group, unsigned long *configs,
				unsigned num_configs)
{
	const unsigned *pins;
	unsigned npins;
	int i, ret;

	ret = amd_get_group_pins(pctldev, group, &pins, &npins);
	if (ret)
		return ret;
	for (i = 0; i < npins; i++) {
		if (amd_pinconf_set(pctldev, pins[i], configs, num_configs))
			return -ENOTSUPP;
	}
	return 0;
}

static void test_amd_pinconf_group_set_success(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	unsigned long configs[1] = {0};
	unsigned group = 0;
	int ret;

	ret = amd_pinconf_group_set(&pctldev, group, configs, 1);
	KUNIT_EXPECT_EQ(test, ret, -ENOTSUPP);
}

static void test_amd_pinconf_group_set_amd_get_group_pins_fail(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	unsigned long configs[1] = {0};
	unsigned group = 0;
	int ret;

	ret = amd_pinconf_group_set(&pctldev, group, configs, 1);
	KUNIT_EXPECT_EQ(test, ret, -ENOTSUPP);
}

static void test_amd_pinconf_group_set_amd_pinconf_set_fail(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	unsigned long configs[1] = {0};
	unsigned group = 0;
	int ret;

	ret = amd_pinconf_group_set(&pctldev, group, configs, 1);
	KUNIT_EXPECT_EQ(test, ret, -ENOTSUPP);
}

static void test_amd_pinconf_group_set_zero_configs(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	unsigned group = 0;
	int ret;

	ret = amd_pinconf_group_set(&pctldev, group, NULL, 0);
	KUNIT_EXPECT_EQ(test, ret, -ENOTSUPP);
}

static void test_amd_pinconf_group_set_null_configs(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	unsigned group = 0;
	int ret;

	ret = amd_pinconf_group_set(&pctldev, group, NULL, 1);
	KUNIT_EXPECT_EQ(test, ret, -ENOTSUPP);
}

static struct kunit_case amd_pinconf_group_set_test_cases[] = {
	KUNIT_CASE(test_amd_pinconf_group_set_success),
	KUNIT_CASE(test_amd_pinconf_group_set_amd_get_group_pins_fail),
	KUNIT_CASE(test_amd_pinconf_group_set_amd_pinconf_set_fail),
	KUNIT_CASE(test_amd_pinconf_group_set_zero_configs),
	KUNIT_CASE(test_amd_pinconf_group_set_null_configs),
	{}
};

static struct kunit_suite amd_pinconf_group_set_test_suite = {
	.name = "amd_pinconf_group_set_test",
	.test_cases = amd_pinconf_group_set_test_cases,
};

kunit_test_suite(amd_pinconf_group_set_test_suite);