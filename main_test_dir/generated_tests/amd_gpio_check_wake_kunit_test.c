
// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/errno.h>

static bool do_amd_gpio_irq_handler(int irq, void *dev_id)
{
	return false;
}

static bool __maybe_unused amd_gpio_check_wake(void *dev_id)
{
	return do_amd_gpio_irq_handler(-1, dev_id);
}

static void test_amd_gpio_check_wake_null_dev_id(struct kunit *test)
{
	bool ret = amd_gpio_check_wake(NULL);
	KUNIT_EXPECT_EQ(test, ret, false);
}

static void test_amd_gpio_check_wake_valid_dev_id(struct kunit *test)
{
	void *dev_id = (void *)0x1234;
	bool ret = amd_gpio_check_wake(dev_id);
	KUNIT_EXPECT_EQ(test, ret, false);
}

static struct kunit_case amd_gpio_check_wake_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_check_wake_null_dev_id),
	KUNIT_CASE(test_amd_gpio_check_wake_valid_dev_id),
	{}
};

static struct kunit_suite amd_gpio_check_wake_test_suite = {
	.name = "amd_gpio_check_wake_test",
	.test_cases = amd_gpio_check_wake_test_cases,
};

kunit_test_suite(amd_gpio_check_wake_test_suite);