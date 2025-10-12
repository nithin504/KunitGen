// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/device.h>
#include <linux/platform_device.h>

// Mocking amd_gpio_suspend_hibernate_common to control its behavior during tests
static bool mock_hibernate_common_called;
static struct device *mock_hibernate_dev;
static bool mock_hibernate_state;
static int mock_hibernate_return_value;

int amd_gpio_suspend_hibernate_common(struct device *dev, bool is_suspend)
{
	mock_hibernate_common_called = true;
	mock_hibernate_dev = dev;
	mock_hibernate_state = is_suspend;
	return mock_hibernate_return_value;
}

// Include the source code under test
#ifdef CONFIG_SUSPEND
static struct pinctrl_dev *pinctrl_dev;
#endif

static int amd_gpio_suspend(struct device *dev)
{
#ifdef CONFIG_SUSPEND
	pinctrl_dev = dev_get_drvdata(dev);
#endif
	return amd_gpio_suspend_hibernate_common(dev, true);
}

// --- Test Cases ---

static void test_amd_gpio_suspend_success(struct kunit *test)
{
	struct platform_device *pdev = kunit_kzalloc(test, sizeof(*pdev), GFP_KERNEL);
	struct device *dev = &pdev->dev;
	mock_hibernate_return_value = 0;
	mock_hibernate_common_called = false;

	int ret = amd_gpio_suspend(dev);

	KUNIT_EXPECT_TRUE(test, mock_hibernate_common_called);
	KUNIT_EXPECT_PTR_EQ(test, mock_hibernate_dev, dev);
	KUNIT_EXPECT_TRUE(test, mock_hibernate_state);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_suspend_error_path(struct kunit *test)
{
	struct platform_device *pdev = kunit_kzalloc(test, sizeof(*pdev), GFP_KERNEL);
	struct device *dev = &pdev->dev;
	mock_hibernate_return_value = -EIO;
	mock_hibernate_common_called = false;

	int ret = amd_gpio_suspend(dev);

	KUNIT_EXPECT_TRUE(test, mock_hibernate_common_called);
	KUNIT_EXPECT_PTR_EQ(test, mock_hibernate_dev, dev);
	KUNIT_EXPECT_TRUE(test, mock_hibernate_state);
	KUNIT_EXPECT_EQ(test, ret, -EIO);
}

#ifdef CONFIG_SUSPEND
static void test_amd_gpio_suspend_pdata_assignment(struct kunit *test)
{
	struct platform_device *pdev = kunit_kzalloc(test, sizeof(*pdev), GFP_KERNEL);
	struct device *dev = &pdev->dev;
	struct pinctrl_dev dummy_pctldev;
	dev_set_drvdata(dev, &dummy_pctldev);
	mock_hibernate_return_value = 0;
	mock_hibernate_common_called = false;
	pinctrl_dev = NULL; // Reset global

	int ret = amd_gpio_suspend(dev);

	KUNIT_EXPECT_TRUE(test, mock_hibernate_common_called);
	KUNIT_EXPECT_PTR_EQ(test, mock_hibernate_dev, dev);
	KUNIT_EXPECT_TRUE(test, mock_hibernate_state);
	KUNIT_EXPECT_PTR_EQ(test, pinctrl_dev, &dummy_pctldev);
	KUNIT_EXPECT_EQ(test, ret, 0);
}
#endif

static struct kunit_case amd_gpio_suspend_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_suspend_success),
	KUNIT_CASE(test_amd_gpio_suspend_error_path),
#ifdef CONFIG_SUSPEND
	KUNIT_CASE(test_amd_gpio_suspend_pdata_assignment),
#endif
	{}
};

static struct kunit_suite amd_gpio_suspend_test_suite = {
	.name = "amd_gpio_suspend_test",
	.test_cases = amd_gpio_suspend_test_cases,
};

kunit_test_suite(amd_gpio_suspend_test_suite);
