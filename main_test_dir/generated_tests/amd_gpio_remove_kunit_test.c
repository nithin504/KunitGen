```c
// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/platform_device.h>
#include <linux/gpio/driver.h>
#include <linux/acpi.h>

struct amd_gpio {
	struct gpio_chip gc;
};

static void amd_gpio_unregister_s2idle_ops(void)
{
}

static bool gpiochip_remove_called;
static bool acpi_unregister_wakeup_handler_called;
static bool amd_gpio_unregister_s2idle_ops_called;

#define gpiochip_remove(gc) do { gpiochip_remove_called = true; } while (0)
#define acpi_unregister_wakeup_handler(a, b) do { acpi_unregister_wakeup_handler_called = true; } while (0)
#define amd_gpio_unregister_s2idle_ops() do { amd_gpio_unregister_s2idle_ops_called = true; } while (0)

#include "pinctrl-amd.c"

static void test_amd_gpio_remove_normal(struct kunit *test)
{
	struct platform_device pdev;
	struct amd_gpio gpio_dev;

	gpiochip_remove_called = false;
	acpi_unregister_wakeup_handler_called = false;
	amd_gpio_unregister_s2idle_ops_called = false;

	platform_set_drvdata(&pdev, &gpio_dev);

	amd_gpio_remove(&pdev);

	KUNIT_EXPECT_TRUE(test, gpiochip_remove_called);
	KUNIT_EXPECT_TRUE(test, acpi_unregister_wakeup_handler_called);
	KUNIT_EXPECT_TRUE(test, amd_gpio_unregister_s2idle_ops_called);
}

static struct kunit_case amd_gpio_remove_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_remove_normal),
	{}
};

static struct kunit_suite amd_gpio_remove_test_suite = {
	.name = "amd_gpio_remove_test",
	.test_cases = amd_gpio_remove_test_cases,
};

kunit_test_suite(amd_gpio_remove_test_suite);
```