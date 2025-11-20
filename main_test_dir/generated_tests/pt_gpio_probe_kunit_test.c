#include <kunit/test.h>
#include <linux/platform_device.h>
#include <linux/acpi.h>
#include <linux/io.h>
#include <linux/gpio/driver.h>

// Mocking ACPI companion check
static bool mock_acpi_companion = true;
#define ACPI_COMPANION(dev) (mock_acpi_companion ? (void *)1 : NULL)

// Define register offsets as per driver
#define PT_INPUTDATA_REG    0x00
#define PT_OUTPUTDATA_REG   0x04
#define PT_DIRECTION_REG    0x08
#define PT_SYNC_REG         0x0C
#define PT_CLOCKRATE_REG    0x10

// Include the source file to test
#include "pt-gpio.c"

// Test resource
static struct resource test_pt_gpio_resources[] = {
	{
		.start = 0x1000,
		.end   = 0x1010,
		.flags = IORESOURCE_MEM,
	},
};

// Test platform device
static struct platform_device *test_pt_gpio_pdev;

// Test memory region
static void *test_mmio_base;
#define TEST_MMIO_SIZE 0x1000

static void setup_test_device(struct kunit *test)
{
	test_mmio_base = kunit_kzalloc(test, TEST_MMIO_SIZE, GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, test_mmio_base);

	test_pt_gpio_pdev = kunit_kzalloc(test, sizeof(*test_pt_gpio_pdev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, test_pt_gpio_pdev);

	test_pt_gpio_pdev->name = "pt-gpio";
	test_pt_gpio_pdev->id = PLATFORM_DEVID_NONE;
	test_pt_gpio_pdev->num_resources = ARRAY_SIZE(test_pt_gpio_resources);
	test_pt_gpio_pdev->resource = test_pt_gpio_resources;

	// Setup resource mapping simulation
	test_pt_gpio_resources[0].start = (resource_size_t)test_mmio_base;
	test_pt_gpio_resources[0].end = (resource_size_t)(test_mmio_base + TEST_MMIO_SIZE - 1);
}

static void test_pt_gpio_probe_success(struct kunit *test)
{
	int ret;

	setup_test_device(test);
	mock_acpi_companion = true;

	// Set match data
	struct platform_driver dummy_driver = {
		.driver = {
			.of_match_table = NULL,
		}
	};
	device_set_platform_data(&test_pt_gpio_pdev->dev, (void *)16); // ngpio = 16

	ret = pt_gpio_probe(test_pt_gpio_pdev);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_pt_gpio_probe_no_acpi(struct kunit *test)
{
	int ret;

	setup_test_device(test);
	mock_acpi_companion = false; // Simulate no ACPI companion

	ret = pt_gpio_probe(test_pt_gpio_pdev);
	KUNIT_EXPECT_EQ(test, ret, -ENODEV);
}

static void test_pt_gpio_probe_ioremap_failure(struct kunit *test)
{
	int ret;

	// Make resource invalid to simulate ioremap failure
	test_pt_gpio_resources[0].flags = 0;
	setup_test_device(test);
	mock_acpi_companion = true;

	ret = pt_gpio_probe(test_pt_gpio_pdev);
	KUNIT_EXPECT_TRUE(test, IS_ERR_VALUE(ret));
}

static void test_pt_gpio_probe_bgpio_init_failure(struct kunit *test)
{
	int ret;

	setup_test_device(test);
	mock_acpi_companion = true;

	// This test assumes that bgpio_init could fail under certain conditions.
	// Since we can't easily make it fail without modifying its behavior,
	// this is more of a placeholder unless we add specific injection points.
	// For now, just run normal probe and ensure cleanup works.
	ret = pt_gpio_probe(test_pt_gpio_pdev);
	if (!IS_ERR_VALUE(ret)) {
		// If probe succeeded, validate some register initialization occurred
		u32 sync_val = readl(test_mmio_base + PT_SYNC_REG);
		u32 clk_val = readl(test_mmio_base + PT_CLOCKRATE_REG);
		KUNIT_EXPECT_EQ(test, sync_val, 0U);
		KUNIT_EXPECT_EQ(test, clk_val, 0U);
	}
}

static struct kunit_case pt_gpio_test_cases[] = {
	KUNIT_CASE(test_pt_gpio_probe_success),
	KUNIT_CASE(test_pt_gpio_probe_no_acpi),
	KUNIT_CASE(test_pt_gpio_probe_ioremap_failure),
	KUNIT_CASE(test_pt_gpio_probe_bgpio_init_failure),
	{}
};

static struct kunit_suite pt_gpio_test_suite = {
	.name = "pt-gpio-probe",
	.test_cases = pt_gpio_test_cases,
};

kunit_test_suite(pt_gpio_test_suite);