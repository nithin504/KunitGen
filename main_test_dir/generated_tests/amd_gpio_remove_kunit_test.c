// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/platform_device.h>
#include <linux/gpio/driver.h>
#include <linux/acpi.h>

// Mock structures
struct amd_gpio {
	struct gpio_chip gc;
};

// Function under test (copied and made static for direct access)
static void amd_gpio_remove(struct platform_device *pdev)
{
	struct amd_gpio *gpio_dev;

	gpio_dev = platform_get_drvdata(pdev);

	gpiochip_remove(&gpio_dev->gc);
	acpi_unregister_wakeup_handler(amd_gpio_check_wake, gpio_dev);
	amd_gpio_unregister_s2idle_ops();
}

// Mock implementations
static int gpiochip_remove_call_count;
static void *gpiochip_remove_arg;
static void gpiochip_remove_mock(struct gpio_chip *gc)
{
	gpiochip_remove_call_count++;
	gpiochip_remove_arg = gc;
}

static int acpi_unregister_wakeup_handler_call_count;
static void *acpi_unregister_wakeup_handler_fn_arg;
static void *acpi_unregister_wakeup_handler_dev_arg;
static void acpi_unregister_wakeup_handler_mock(void (*fn)(void *), void *dev)
{
	acpi_unregister_wakeup_handler_call_count++;
	acpi_unregister_wakeup_handler_fn_arg = fn;
	acpi_unregister_wakeup_handler_dev_arg = dev;
}

static int amd_gpio_unregister_s2idle_ops_call_count;
static void amd_gpio_unregister_s2idle_ops_mock(void)
{
	amd_gpio_unregister_s2idle_ops_call_count++;
}

// Redefine calls to use mocks
#define gpiochip_remove(gc) gpiochip_remove_mock(gc)
#define acpi_unregister_wakeup_handler(fn, dev) acpi_unregister_wakeup_handler_mock(fn, dev)
#define amd_gpio_unregister_s2idle_ops() amd_gpio_unregister_s2idle_ops_mock()

// Include the source file after defining mocks
#include "pinctrl-amd.c"

// Test case: normal execution path
static void test_amd_gpio_remove_normal(struct kunit *test)
{
	struct platform_device *pdev;
	struct amd_gpio *gpio_dev;

	gpiochip_remove_call_count = 0;
	acpi_unregister_wakeup_handler_call_count = 0;
	amd_gpio_unregister_s2idle_ops_call_count = 0;

	pdev = kunit_kzalloc(test, sizeof(*pdev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pdev);

	gpio_dev = kunit_kzalloc(test, sizeof(*gpio_dev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev);

	platform_set_drvdata(pdev, gpio_dev);

	amd_gpio_remove(pdev);

	KUNIT_EXPECT_EQ(test, gpiochip_remove_call_count, 1);
	KUNIT_EXPECT_PTR_EQ(test, gpiochip_remove_arg, &gpio_dev->gc);

	KUNIT_EXPECT_EQ(test, acpi_unregister_wakeup_handler_call_count, 1);
	KUNIT_EXPECT_PTR_EQ(test, acpi_unregister_wakeup_handler_fn_arg, amd_gpio_check_wake);
	KUNIT_EXPECT_PTR_EQ(test, acpi_unregister_wakeup_handler_dev_arg, gpio_dev);

	KUNIT_EXPECT_EQ(test, amd_gpio_unregister_s2idle_ops_call_count, 1);
}

// Test case: platform device with NULL drvdata
static void test_amd_gpio_remove_null_drvdata(struct kunit *test)
{
	struct platform_device *pdev;

	gpiochip_remove_call_count = 0;
	acpi_unregister_wakeup_handler_call_count = 0;
	amd_gpio_unregister_s2idle_ops_call_count = 0;

	pdev = kunit_kzalloc(test, sizeof(*pdev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pdev);

	platform_set_drvdata(pdev, NULL);

	amd_gpio_remove(pdev);

	KUNIT_EXPECT_EQ(test, gpiochip_remove_call_count, 0);
	KUNIT_EXPECT_EQ(test, acpi_unregister_wakeup_handler_call_count, 0);
	KUNIT_EXPECT_EQ(test, amd_gpio_unregister_s2idle_ops_call_count, 1);
}

static struct kunit_case amd_gpio_remove_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_remove_normal),
	KUNIT_CASE(test_amd_gpio_remove_null_drvdata),
	{}
};

static struct kunit_suite amd_gpio_remove_test_suite = {
	.name = "amd_gpio_remove_test",
	.test_cases = amd_gpio_remove_test_cases,
};

kunit_test_suite(amd_gpio_remove_test_suite);
