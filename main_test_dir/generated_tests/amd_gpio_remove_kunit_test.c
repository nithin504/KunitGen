// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/platform_device.h>
#include <linux/gpio/driver.h>
#include <linux/acpi.h>

static void amd_gpio_remove(struct platform_device *pdev)
{
	struct amd_gpio *gpio_dev;

	gpio_dev = platform_get_drvdata(pdev);

	gpiochip_remove(&gpio_dev->gc);
	acpi_unregister_wakeup_handler(amd_gpio_check_wake, gpio_dev);
	amd_gpio_unregister_s2idle_ops();
}

static int amd_gpio_check_wake_mock(struct acpi_gpe_handler_data *data)
{
	return 0;
}

static void amd_gpio_unregister_s2idle_ops_mock(void)
{
}

static void test_amd_gpio_remove_normal_case(struct kunit *test)
{
	struct platform_device pdev;
	struct amd_gpio gpio_dev;
	struct gpio_chip gc;

	gpio_dev.gc = gc;
	
	platform_set_drvdata(&pdev, &gpio_dev);
	
	amd_gpio_check_wake = amd_gpio_check_wake_mock;
	amd_gpio_unregister_s2idle_ops = amd_gpio_unregister_s2idle_ops_mock;
	
	amd_gpio_remove(&pdev);
	
	KUNIT_EXPECT_PTR_EQ(test, platform_get_drvdata(&pdev), &gpio_dev);
}

static void test_amd_gpio_remove_null_pdev(struct kunit *test)
{
	amd_gpio_check_wake = amd_gpio_check_wake_mock;
	amd_gpio_unregister_s2idle_ops = amd_gpio_unregister_s2idle_ops_mock;
	
	amd_gpio_remove(NULL);
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_remove_null_drvdata(struct kunit *test)
{
	struct platform_device pdev;
	
	platform_set_drvdata(&pdev, NULL);
	amd_gpio_check_wake = amd_gpio_check_wake_mock;
	amd_gpio_unregister_s2idle_ops = amd_gpio_unregister_s2idle_ops_mock;
	
	amd_gpio_remove(&pdev);
	KUNIT_EXPECT_TRUE(test, true);
}

static struct kunit_case amd_gpio_remove_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_remove_normal_case),
	KUNIT_CASE(test_amd_gpio_remove_null_pdev),
	KUNIT_CASE(test_amd_gpio_remove_null_drvdata),
	{}
};

static struct kunit_suite amd_gpio_remove_test_suite = {
	.name = "amd_gpio_remove_test",
	.test_cases = amd_gpio_remove_test_cases,
};

kunit_test_suite(amd_gpio_remove_test_suite);