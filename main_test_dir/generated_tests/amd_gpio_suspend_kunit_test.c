// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/device.h>
#include <linux/pinctrl/pinctrl.h>

static struct pinctrl_dev *pinctrl_dev;

static int amd_gpio_suspend_hibernate_common(struct device *dev, bool suspend)
{
	return 0;
}

static int amd_gpio_suspend(struct device *dev)
{
#ifdef CONFIG_SUSPEND
	pinctrl_dev = dev_get_drvdata(dev);
#endif
	return amd_gpio_suspend_hibernate_common(dev, true);
}

static void test_amd_gpio_suspend_with_config_suspend(struct kunit *test)
{
	struct device dev;
	struct pinctrl_dev pctldev;
	
	dev_set_drvdata(&dev, &pctldev);
	
	int ret = amd_gpio_suspend(&dev);
	
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_PTR_EQ(test, pinctrl_dev, &pctldev);
}

static void test_amd_gpio_suspend_without_config_suspend(struct kunit *test)
{
	struct device dev;
	struct pinctrl_dev pctldev;
	
	dev_set_drvdata(&dev, &pctldev);
	
	int ret = amd_gpio_suspend(&dev);
	
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_suspend_null_device(struct kunit *test)
{
	int ret = amd_gpio_suspend(NULL);
	
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_suspend_null_drvdata(struct kunit *test)
{
	struct device dev;
	
	dev_set_drvdata(&dev, NULL);
	
	int ret = amd_gpio_suspend(&dev);
	
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static struct kunit_case amd_gpio_suspend_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_suspend_with_config_suspend),
	KUNIT_CASE(test_amd_gpio_suspend_without_config_suspend),
	KUNIT_CASE(test_amd_gpio_suspend_null_device),
	KUNIT_CASE(test_amd_gpio_suspend_null_drvdata),
	{}
};

static struct kunit_suite amd_gpio_suspend_test_suite = {
	.name = "amd_gpio_suspend_test",
	.test_cases = amd_gpio_suspend_test_cases,
};

kunit_test_suite(amd_gpio_suspend_test_suite);