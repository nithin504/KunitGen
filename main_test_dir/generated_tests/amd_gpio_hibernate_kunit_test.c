#include <kunit/test.h>
#include <linux/device.h>

static int amd_gpio_suspend_hibernate_common(struct device *dev, bool hibernate)
{
	return 0;
}

static int amd_gpio_hibernate(struct device *dev)
{
	return amd_gpio_suspend_hibernate_common(dev, false);
}

static void test_amd_gpio_hibernate_null_device(struct kunit *test)
{
	int ret = amd_gpio_hibernate(NULL);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_hibernate_valid_device(struct kunit *test)
{
	struct device dev;
	int ret = amd_gpio_hibernate(&dev);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static struct kunit_case amd_gpio_hibernate_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_hibernate_null_device),
	KUNIT_CASE(test_amd_gpio_hibernate_valid_device),
	{}
};

static struct kunit_suite amd_gpio_hibernate_test_suite = {
	.name = "amd_gpio_hibernate_test",
	.test_cases = amd_gpio_hibernate_test_cases,
};

kunit_test_suite(amd_gpio_hibernate_test_suite);