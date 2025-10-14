// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/device.h>
#include <linux/platform_device.h>

// Mocking amd_gpio_suspend_hibernate_common to control its behavior during testing
static bool mock_hibernate_common_called;
static bool mock_hibernate_return_value;
static struct device *mock_hibernate_dev_passed;
static bool mock_hibernate_second_arg;

int amd_gpio_suspend_hibernate_common(struct device *dev, bool val)
{
	mock_hibernate_common_called = true;
	mock_hibernate_dev_passed = dev;
	mock_hibernate_second_arg = val;
	return mock_hibernate_return_value ? 0 : -1;
}

// Include the function under test
#ifdef CONFIG_SUSPEND
static struct pinctrl_dev *pinctrl_dev; // Simulate global variable if needed
#endif

static int amd_gpio_suspend(struct device *dev)
{
#ifdef CONFIG_SUSPEND
	pinctrl_dev = dev_get_drvdata(dev);
#endif
	return amd_gpio_suspend_hibernate_common(dev, true);
}

// --- Begin Tests ---

static void test_amd_gpio_suspend_calls_hibernate_with_true(struct kunit *test)
{
	struct platform_device *pdev = kunit_kzalloc(test, sizeof(*pdev), GFP_KERNEL);
	struct device *dev = &pdev->dev;
	mock_hibernate_common_called = false;
	mock_hibernate_return_value = true;

	int ret = amd_gpio_suspend(dev);

	KUNIT_EXPECT_TRUE(test, mock_hibernate_common_called);
	KUNIT_EXPECT_PTR_EQ(test, mock_hibernate_dev_passed, dev);
	KUNIT_EXPECT_TRUE(test, mock_hibernate_second_arg);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_suspend_returns_error_when_hibernate_fails(struct kunit *test)
{
	struct platform_device *pdev = kunit_kzalloc(test, sizeof(*pdev), GFP_KERNEL);
	struct device *dev = &pdev->dev;
	mock_hibernate_common_called = false;
	mock_hibernate_return_value = false;

	int ret = amd_gpio_suspend(dev);

	KUNIT_EXPECT_TRUE(test, mock_hibernate_common_called);
	KUNIT_EXPECT_PTR_EQ(test, mock_hibernate_dev_passed, dev);
	KUNIT_EXPECT_TRUE(test, mock_hibernate_second_arg);
	KUNIT_EXPECT_EQ(test, ret, -1);
}

// --- End Tests ---

static struct kunit_case generated_kunit_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_suspend_calls_hibernate_with_true),
	KUNIT_CASE(test_amd_gpio_suspend_returns_error_when_hibernate_fails),
	{}
};

static struct kunit_suite generated_kunit_test_suite = {
	.name = "amd_gpio_suspend-test",
	.test_cases = generated_kunit_test_cases,
};

kunit_test_suite(generated_kunit_test_suite);
