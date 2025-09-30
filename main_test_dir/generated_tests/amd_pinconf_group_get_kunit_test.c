
// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/errno.h>

static int amd_get_group_pins(struct pinctrl_dev *pctldev, unsigned int group,
			      const unsigned **pins, unsigned *npins)
{
	return -ENOTSUPP;
}

static int amd_pinconf_get(struct pinctrl_dev *pctldev, unsigned int pin,
			   unsigned long *config)
{
	return -ENOTSUPP;
}

static int amd_pinconf_group_get(struct pinctrl_dev *pctldev,
				 unsigned int group,
				 unsigned long *config)
{
	const unsigned *pins;
	unsigned npins;
	int ret;

	ret = amd_get_group_pins(pctldev, group, &pins, &npins);
	if (ret)
		return ret;

	if (amd_pinconf_get(pctldev, pins[0], config))
		return -ENOTSUPP;

	return 0;
}

static void test_amd_pinconf_group_get_success(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	unsigned long config;
	int ret;

	amd_get_group_pins = amd_get_group_pins_mock_success;
	amd_pinconf_get = amd_pinconf_get_mock_success;

	ret = amd_pinconf_group_get(&pctldev, 0, &config);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_pinconf_group_get_get_group_pins_fails(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	unsigned long config;
	int ret;

	amd_get_group_pins = amd_get_group_pins_mock_fail;

	ret = amd_pinconf_group_get(&pctldev, 0, &config);
	KUNIT_EXPECT_EQ(test, ret, -ENOTSUPP);
}

static void test_amd_pinconf_group_get_pinconf_get_fails(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	unsigned long config;
	int ret;

	amd_get_group_pins = amd_get_group_pins_mock_success;
	amd_pinconf_get = amd_pinconf_get_mock_fail;

	ret = amd_pinconf_group_get(&pctldev, 0, &config);
	KUNIT_EXPECT_EQ(test, ret, -ENOTSUPP);
}

static void test_amd_pinconf_group_get_null_config(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	int ret;

	amd_get_group_pins = amd_get_group_pins_mock_success;

	ret = amd_pinconf_group_get(&pctldev, 0, NULL);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static int amd_get_group_pins_mock_success(struct pinctrl_dev *pctldev,
					  unsigned int group,
					  const unsigned **pins,
					  unsigned *npins)
{
	static const unsigned mock_pins[] = {0};
	*pins = mock_pins;
	*npins = 1;
	return 0;
}

static int amd_get_group_pins_mock_fail(struct pinctrl_dev *pctldev,
					unsigned int group,
					const unsigned **pins,
					unsigned *npins)
{
	return -ENOTSUPP;
}

static int amd_pinconf_get_mock_success(struct pinctrl_dev *pctldev,
					unsigned int pin,
					unsigned long *config)
{
	if (config)
		*config = 0;
	return 0;
}

static int amd_pinconf_get_mock_fail(struct pinctrl_dev *pctldev,
				     unsigned int pin,
				     unsigned long *config)
{
	return -ENOTSUPP;
}

static struct kunit_case amd_pinconf_group_get_test_cases[] = {
	KUNIT_CASE(test_amd_pinconf_group_get_success),
	KUNIT_CASE(test_amd_pinconf_group_get_get_group_pins_fails),
	KUNIT_CASE(test_amd_pinconf_group_get_pinconf_get_fails),
	KUNIT_CASE(test_amd_pinconf_group_get_null_config),
	{}
};

static struct kunit_suite amd_pinconf_group_get_test_suite = {
	.name = "amd_pinconf_group_get_test",
	.test_cases = amd_pinconf_group_get_test_cases,
};

kunit_test_suite(amd_pinconf_group_get_test_suite);