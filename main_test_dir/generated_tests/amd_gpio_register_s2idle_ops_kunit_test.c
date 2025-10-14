// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/platform_device.h>
#include <linux/errno.h>

static inline void amd_gpio_register_s2idle_ops(void) {}

static void test_amd_gpio_register_s2idle_ops_basic(struct kunit *test)
{
	amd_gpio_register_s2idle_ops();
	KUNIT_EXPECT_TRUE(test, true);
}

static struct kunit_case amd_gpio_register_s2idle_ops_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_register_s2idle_ops_basic),
	{}
};

static struct kunit_suite amd_gpio_register_s2idle_ops_test_suite = {
	.name = "amd_gpio_register_s2idle_ops_test",
	.test_cases = amd_gpio_register_s2idle_ops_test_cases,
};

kunit_test_suite(amd_gpio_register_s2idle_ops_test_suite);