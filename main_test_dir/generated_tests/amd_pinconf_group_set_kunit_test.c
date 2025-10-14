// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/errno.h>

// Mock structures and functions
struct amd_gpio {
	// Minimal mock structure
};

static int mock_amd_get_group_pins_return = 0;
static int mock_amd_pinconf_set_return = 0;
static int mock_amd_pinconf_set_call_count = 0;
static unsigned mock_pins_data[] = { 10, 20, 30 };
static const unsigned *mock_pins_ptr = mock_pins_data;
static unsigned mock_npins = 3;

// Function prototypes
static int amd_get_group_pins(struct pinctrl_dev *pctldev, unsigned group,
			      const unsigned **pins, unsigned *npins);
static int amd_pinconf_set(struct pinctrl_dev *pctldev, unsigned pin,
			   unsigned long *configs, unsigned num_configs);

// Include the function under test
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

// Mock implementations
static int amd_get_group_pins(struct pinctrl_dev *pctldev, unsigned group,
			      const unsigned **pins, unsigned *npins)
{
	if (mock_amd_get_group_pins_return)
		return mock_amd_get_group_pins_return;
	*pins = mock_pins_ptr;
	*npins = mock_npins;
	return 0;
}

static int amd_pinconf_set(struct pinctrl_dev *pctldev, unsigned pin,
			   unsigned long *configs, unsigned num_configs)
{
	mock_amd_pinconf_set_call_count++;
	return mock_amd_pinconf_set_return;
}

// Test cases
static void test_amd_pinconf_group_set_success(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = { 0x12345678 };
	mock_amd_get_group_pins_return = 0;
	mock_amd_pinconf_set_return = 0;
	mock_amd_pinconf_set_call_count = 0;

	int ret = amd_pinconf_group_set(&dummy_pctldev, 0, configs, 1);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, mock_amd_pinconf_set_call_count, 3);
}

static void test_amd_pinconf_group_set_get_group_pins_fail(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = { 0x12345678 };
	mock_amd_get_group_pins_return = -EINVAL;
	mock_amd_pinconf_set_call_count = 0;

	int ret = amd_pinconf_group_set(&dummy_pctldev, 0, configs, 1);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
	KUNIT_EXPECT_EQ(test, mock_amd_pinconf_set_call_count, 0);
}

static void test_amd_pinconf_group_set_pinconf_set_fail(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = { 0x12345678 };
	mock_amd_get_group_pins_return = 0;
	mock_amd_pinconf_set_return = -ENOTSUPP;
	mock_amd_pinconf_set_call_count = 0;

	int ret = amd_pinconf_group_set(&dummy_pctldev, 0, configs, 1);
	KUNIT_EXPECT_EQ(test, ret, -ENOTSUPP);
	KUNIT_EXPECT_EQ(test, mock_amd_pinconf_set_call_count, 1);
}

static void test_amd_pinconf_group_set_empty_group(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = { 0x12345678 };
	mock_amd_get_group_pins_return = 0;
	mock_npins = 0;
	mock_amd_pinconf_set_call_count = 0;

	int ret = amd_pinconf_group_set(&dummy_pctldev, 0, configs, 1);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, mock_amd_pinconf_set_call_count, 0);
}

static void test_amd_pinconf_group_set_single_pin(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = { 0x12345678 };
	mock_amd_get_group_pins_return = 0;
	mock_pins_data[0] = 42;
	mock_npins = 1;
	mock_amd_pinconf_set_return = 0;
	mock_amd_pinconf_set_call_count = 0;

	int ret = amd_pinconf_group_set(&dummy_pctldev, 0, configs, 1);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, mock_amd_pinconf_set_call_count, 1);
}

static struct kunit_case amd_pinconf_group_set_test_cases[] = {
	KUNIT_CASE(test_amd_pinconf_group_set_success),
	KUNIT_CASE(test_amd_pinconf_group_set_get_group_pins_fail),
	KUNIT_CASE(test_amd_pinconf_group_set_pinconf_set_fail),
	KUNIT_CASE(test_amd_pinconf_group_set_empty_group),
	KUNIT_CASE(test_amd_pinconf_group_set_single_pin),
	{}
};

static struct kunit_suite amd_pinconf_group_set_test_suite = {
	.name = "amd_pinconf_group_set_test",
	.test_cases = amd_pinconf_group_set_test_cases,
};

kunit_test_suite(amd_pinconf_group_set_test_suite);
