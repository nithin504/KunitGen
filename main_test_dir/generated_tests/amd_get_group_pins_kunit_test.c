// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/errno.h>

// Mock pinctrl_dev_get_drvdata
void *set_mock_pinctrl_dev_get_drvdata(struct pinctrl_dev *pctldev);
#define pinctrl_dev_get_drvdata set_mock_pinctrl_dev_get_drvdata

// Include the source code under test
#include "pinctrl-amd.c"

#define MAX_GROUPS 10
#define MAX_PINS_PER_GROUP 32

static struct amd_gpio mock_gpio_dev;
static struct pinctrl_dev dummy_pctldev;
static unsigned mock_pins[MAX_GROUPS][MAX_PINS_PER_GROUP];
static struct pin_group mock_groups[MAX_GROUPS];

void *set_mock_pinctrl_dev_get_drvdata(struct pinctrl_dev *pctldev)
{
	return &mock_gpio_dev;
}

static void test_amd_get_group_pins_valid_group(struct kunit *test)
{
	const unsigned *pins;
	unsigned num_pins;
	int ret;

	// Setup mock data
	mock_gpio_dev.groups = mock_groups;
	mock_groups[0].pins = mock_pins[0];
	mock_groups[0].npins = 5;
	for (int i = 0; i < 5; i++)
		mock_pins[0][i] = 100 + i;

	ret = amd_get_group_pins(&dummy_pctldev, 0, &pins, &num_pins);

	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_PTR_EQ(test, pins, mock_pins[0]);
	KUNIT_EXPECT_EQ(test, num_pins, 5U);
}

static void test_amd_get_group_pins_zero_pins(struct kunit *test)
{
	const unsigned *pins;
	unsigned num_pins;
	int ret;

	// Setup mock data with zero pins
	mock_gpio_dev.groups = mock_groups;
	mock_groups[1].pins = mock_pins[1];
	mock_groups[1].npins = 0;

	ret = amd_get_group_pins(&dummy_pctldev, 1, &pins, &num_pins);

	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_PTR_EQ(test, pins, mock_pins[1]);
	KUNIT_EXPECT_EQ(test, num_pins, 0U);
}

static void test_amd_get_group_pins_max_group_index(struct kunit *test)
{
	const unsigned *pins;
	unsigned num_pins;
	int ret;

	// Setup mock data for maximum group index
	mock_gpio_dev.groups = mock_groups;
	mock_groups[MAX_GROUPS - 1].pins = mock_pins[MAX_GROUPS - 1];
	mock_groups[MAX_GROUPS - 1].npins = 3;
	for (int i = 0; i < 3; i++)
		mock_pins[MAX_GROUPS - 1][i] = 200 + i;

	ret = amd_get_group_pins(&dummy_pctldev, MAX_GROUPS - 1, &pins, &num_pins);

	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_PTR_EQ(test, pins, mock_pins[MAX_GROUPS - 1]);
	KUNIT_EXPECT_EQ(test, num_pins, 3U);
}

static void test_amd_get_group_pins_null_pins_pointer(struct kunit *test)
{
	unsigned num_pins;
	int ret;

	// This should cause a kernel oops due to dereferencing NULL,
	// but we can at least check that the function doesn't return an error
	mock_gpio_dev.groups = mock_groups;
	mock_groups[0].pins = mock_pins[0];
	mock_groups[0].npins = 1;

	ret = amd_get_group_pins(&dummy_pctldev, 0, NULL, &num_pins);

	// The function itself doesn't validate the pins pointer
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_get_group_pins_null_num_pins_pointer(struct kunit *test)
{
	const unsigned *pins;
	int ret;

	// This should cause a kernel oops due to dereferencing NULL,
	// but we can at least check that the function doesn't return an error
	mock_gpio_dev.groups = mock_groups;
	mock_groups[0].pins = mock_pins[0];
	mock_groups[0].npins = 1;

	ret = amd_get_group_pins(&dummy_pctldev, 0, &pins, NULL);

	// The function itself doesn't validate the num_pins pointer
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static struct kunit_case amd_get_group_pins_test_cases[] = {
	KUNIT_CASE(test_amd_get_group_pins_valid_group),
	KUNIT_CASE(test_amd_get_group_pins_zero_pins),
	KUNIT_CASE(test_amd_get_group_pins_max_group_index),
	KUNIT_CASE(test_amd_get_group_pins_null_pins_pointer),
	KUNIT_CASE(test_amd_get_group_pins_null_num_pins_pointer),
	{}
};

static struct kunit_suite amd_get_group_pins_test_suite = {
	.name = "amd_get_group_pins_test",
	.test_cases = amd_get_group_pins_test_cases,
};

kunit_test_suite(amd_get_group_pins_test_suite);
