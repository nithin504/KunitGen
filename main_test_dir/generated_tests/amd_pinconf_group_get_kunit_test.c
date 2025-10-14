// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/errno.h>

// Mocked dependencies
static int mock_amd_get_group_pins_ret = 0;
static const unsigned *mock_pins_ptr = NULL;
static unsigned mock_npins_val = 0;
static int mock_amd_pinconf_get_ret = 0;

int amd_get_group_pins(struct pinctrl_dev *pctldev, unsigned int group,
                       const unsigned **pins, unsigned *npins)
{
	if (mock_amd_get_group_pins_ret)
		return mock_amd_get_group_pins_ret;

	*pins = mock_pins_ptr;
	*npins = mock_npins_val;
	return 0;
}

int amd_pinconf_get(struct pinctrl_dev *pctldev, unsigned int pin,
                    unsigned long *config)
{
	return mock_amd_pinconf_get_ret;
}

// Include the function under test
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

// Test cases
static void test_amd_pinconf_group_get_success(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	unsigned long config = 0;
	unsigned pins[] = { 10 };
	mock_amd_get_group_pins_ret = 0;
	mock_pins_ptr = pins;
	mock_npins_val = 1;
	mock_amd_pinconf_get_ret = 0;

	int ret = amd_pinconf_group_get(&pctldev, 0, &config);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_pinconf_group_get_get_group_pins_fail(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	unsigned long config = 0;
	mock_amd_get_group_pins_ret = -EINVAL;
	mock_pins_ptr = NULL;
	mock_npins_val = 0;

	int ret = amd_pinconf_group_get(&pctldev, 0, &config);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_amd_pinconf_group_get_pinconf_get_fail(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	unsigned long config = 0;
	unsigned pins[] = { 10 };
	mock_amd_get_group_pins_ret = 0;
	mock_pins_ptr = pins;
	mock_npins_val = 1;
	mock_amd_pinconf_get_ret = -ENOTSUPP;

	int ret = amd_pinconf_group_get(&pctldev, 0, &config);
	KUNIT_EXPECT_EQ(test, ret, -ENOTSUPP);
}

static struct kunit_case amd_pinconf_group_get_test_cases[] = {
	KUNIT_CASE(test_amd_pinconf_group_get_success),
	KUNIT_CASE(test_amd_pinconf_group_get_get_group_pins_fail),
	KUNIT_CASE(test_amd_pinconf_group_get_pinconf_get_fail),
	{}
};

static struct kunit_suite amd_pinconf_group_get_test_suite = {
	.name = "amd_pinconf_group_get_test",
	.test_cases = amd_pinconf_group_get_test_cases,
};

kunit_test_suite(amd_pinconf_group_get_test_suite);
