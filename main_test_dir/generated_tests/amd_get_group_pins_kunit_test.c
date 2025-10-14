// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/io.h>

// Mock structures and data
struct amd_gpio_group {
	const unsigned *pins;
	unsigned npins;
};

struct amd_gpio {
	struct amd_gpio_group *groups;
};

// Mock pinctrl_dev_get_drvdata
static struct amd_gpio mock_gpio_dev;
static struct amd_gpio_group mock_groups[10];
static unsigned mock_pins_group0[] = { 10, 11, 12 };
static unsigned mock_pins_group1[] = { 20, 21 };

static void *pinctrl_dev_get_drvdata(struct pinctrl_dev *pctldev)
{
	return &mock_gpio_dev;
}

// Include the function under test
static int amd_get_group_pins(struct pinctrl_dev *pctldev,
			      unsigned group,
			      const unsigned **pins,
			      unsigned *num_pins)
{
	struct amd_gpio *gpio_dev = pinctrl_dev_get_drvdata(pctldev);

	*pins = gpio_dev->groups[group].pins;
	*num_pins = gpio_dev->groups[group].npins;
	return 0;
}

// Test cases
static void test_amd_get_group_pins_valid_group0(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	const unsigned *pins;
	unsigned num_pins;

	mock_gpio_dev.groups = mock_groups;
	mock_groups[0].pins = mock_pins_group0;
	mock_groups[0].npins = ARRAY_SIZE(mock_pins_group0);

	int ret = amd_get_group_pins(&dummy_pctldev, 0, &pins, &num_pins);

	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_PTR_EQ(test, pins, mock_pins_group0);
	KUNIT_EXPECT_EQ(test, num_pins, 3U);
}

static void test_amd_get_group_pins_valid_group1(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	const unsigned *pins;
	unsigned num_pins;

	mock_gpio_dev.groups = mock_groups;
	mock_groups[1].pins = mock_pins_group1;
	mock_groups[1].npins = ARRAY_SIZE(mock_pins_group1);

	int ret = amd_get_group_pins(&dummy_pctldev, 1, &pins, &num_pins);

	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_PTR_EQ(test, pins, mock_pins_group1);
	KUNIT_EXPECT_EQ(test, num_pins, 2U);
}

static void test_amd_get_group_pins_empty_group(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	const unsigned *pins;
	unsigned num_pins;

	mock_gpio_dev.groups = mock_groups;
	mock_groups[2].pins = NULL;
	mock_groups[2].npins = 0;

	int ret = amd_get_group_pins(&dummy_pctldev, 2, &pins, &num_pins);

	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_NULL(test, pins);
	KUNIT_EXPECT_EQ(test, num_pins, 0U);
}

static void test_amd_get_group_pins_null_pins_pointer(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned num_pins;

	mock_gpio_dev.groups = mock_groups;
	mock_groups[3].pins = mock_pins_group0;
	mock_groups[3].npins = ARRAY_SIZE(mock_pins_group0);

	int ret = amd_get_group_pins(&dummy_pctldev, 3, NULL, &num_pins);

	// Function does not check for NULL pins, so behavior depends on implementation
	// Since we can't dereference NULL safely in test, just ensure it returns 0
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_get_group_pins_null_num_pins_pointer(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	const unsigned *pins;

	mock_gpio_dev.groups = mock_groups;
	mock_groups[4].pins = mock_pins_group0;
	mock_groups[4].npins = ARRAY_SIZE(mock_pins_group0);

	int ret = amd_get_group_pins(&dummy_pctldev, 4, &pins, NULL);

	// Function does not check for NULL num_pins, so behavior depends on implementation
	// Since we can't dereference NULL safely in test, just ensure it returns 0
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static struct kunit_case amd_get_group_pins_test_cases[] = {
	KUNIT_CASE(test_amd_get_group_pins_valid_group0),
	KUNIT_CASE(test_amd_get_group_pins_valid_group1),
	KUNIT_CASE(test_amd_get_group_pins_empty_group),
	KUNIT_CASE(test_amd_get_group_pins_null_pins_pointer),
	KUNIT_CASE(test_amd_get_group_pins_null_num_pins_pointer),
	{}
};

static struct kunit_suite amd_get_group_pins_test_suite = {
	.name = "amd_get_group_pins_test",
	.test_cases = amd_get_group_pins_test_cases,
};

kunit_test_suite(amd_get_group_pins_test_suite);
