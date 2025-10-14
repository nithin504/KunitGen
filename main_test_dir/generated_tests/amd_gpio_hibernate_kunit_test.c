// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/device.h>

// Mocked function declaration to simulate the common suspend/hibernate logic
static int amd_gpio_suspend_hibernate_common_called = 0;
static bool last_hibernate_arg = true;

static int amd_gpio_suspend_hibernate_common(struct device *dev, bool hibernating)
{
	amd_gpio_suspend_hibernate_common_called++;
	last_hibernate_arg = hibernating;
	return 0; // Simulate success
}

// Include the function under test
static int amd_gpio_hibernate(struct device *dev)
{
	return amd_gpio_suspend_hibernate_common(dev, false);
}

// --- Test Cases ---

static void test_amd_gpio_hibernate_success(struct kunit *test)
{
	struct device dev;
	int ret;

	amd_gpio_suspend_hibernate_common_called = 0;
	last_hibernate_arg = true;

	ret = amd_gpio_hibernate(&dev);

	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, amd_gpio_suspend_hibernate_common_called, 1);
	KUNIT_EXPECT_FALSE(test, last_hibernate_arg);
}

static void test_amd_gpio_hibernate_null_device(struct kunit *test)
{
	int ret;

	amd_gpio_suspend_hibernate_common_called = 0;
	last_hibernate_arg = true;

	ret = amd_gpio_hibernate(NULL);

	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, amd_gpio_suspend_hibernate_common_called, 1);
	KUNIT_EXPECT_FALSE(test, last_hibernate_arg);
}

// --- Test Suite Definition ---

static struct kunit_case generated_kunit_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_hibernate_success),
	KUNIT_CASE(test_amd_gpio_hibernate_null_device),
	{}
};

static struct kunit_suite generated_kunit_test_suite = {
	.name = "amd_gpio_hibernate_test",
	.test_cases = generated_kunit_test_cases,
};

kunit_test_suite(generated_kunit_test_suite);
