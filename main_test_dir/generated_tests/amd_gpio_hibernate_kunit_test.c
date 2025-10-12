// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/device.h>
#include <linux/err.h>

// Mocked function to simulate amd_gpio_suspend_hibernate_common behavior
static int mock_amd_gpio_suspend_hibernate_common_call_count = 0;
static bool mock_amd_gpio_suspend_hibernate_should_fail = false;
static int mock_amd_gpio_suspend_hibernate_return_value = 0;

static int amd_gpio_suspend_hibernate_common(struct device *dev, bool is_suspend)
{
	mock_amd_gpio_suspend_hibernate_common_call_count++;
	if (mock_amd_gpio_suspend_hibernate_should_fail)
		return mock_amd_gpio_suspend_hibernate_return_value;
	return 0;
}

// Function under test (copied and made static)
static int amd_gpio_hibernate(struct device *dev)
{
	return amd_gpio_suspend_hibernate_common(dev, false);
}

// Test case: successful hibernation
static void test_amd_gpio_hibernate_success(struct kunit *test)
{
	struct device dummy_dev;
	mock_amd_gpio_suspend_hibernate_common_call_count = 0;
	mock_amd_gpio_suspend_hibernate_should_fail = false;

	int ret = amd_gpio_hibernate(&dummy_dev);

	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, mock_amd_gpio_suspend_hibernate_common_call_count, 1);
}

// Test case: hibernation failure
static void test_amd_gpio_hibernate_failure(struct kunit *test)
{
	struct device dummy_dev;
	mock_amd_gpio_suspend_hibernate_common_call_count = 0;
	mock_amd_gpio_suspend_hibernate_should_fail = true;
	mock_amd_gpio_suspend_hibernate_return_value = -EIO;

	int ret = amd_gpio_hibernate(&dummy_dev);

	KUNIT_EXPECT_EQ(test, ret, -EIO);
	KUNIT_EXPECT_EQ(test, mock_amd_gpio_suspend_hibernate_common_call_count, 1);
}

// Test case: NULL device pointer
static void test_amd_gpio_hibernate_null_device(struct kunit *test)
{
	mock_amd_gpio_suspend_hibernate_common_call_count = 0;
	mock_amd_gpio_suspend_hibernate_should_fail = false;

	int ret = amd_gpio_hibernate(NULL);

	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, mock_amd_gpio_suspend_hibernate_common_call_count, 1);
}

static struct kunit_case amd_gpio_hibernate_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_hibernate_success),
	KUNIT_CASE(test_amd_gpio_hibernate_failure),
	KUNIT_CASE(test_amd_gpio_hibernate_null_device),
	{}
};

static struct kunit_suite amd_gpio_hibernate_test_suite = {
	.name = "amd_gpio_hibernate_test",
	.test_cases = amd_gpio_hibernate_test_cases,
};

kunit_test_suite(amd_gpio_hibernate_test_suite);
