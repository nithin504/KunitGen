#include <kunit/test.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/acpi.h>
#include <linux/gpio/driver.h>

#define PT_DIRECTION_REG   0x00
#define PT_INPUTDATA_REG   0x04
#define PT_OUTPUTDATA_REG  0x08
#define PT_CLOCKRATE_REG   0x0C
#define PT_SYNC_REG        0x28

struct pt_gpio_chip {
	struct gpio_chip gc;
	void __iomem *reg_base;
};

static int pt_gpio_request(struct gpio_chip *gc, unsigned offset)
{
	return 0;
}

static void pt_gpio_free(struct gpio_chip *gc, unsigned offset)
{
}

static struct platform_device *create_mock_platform_device(struct kunit *test)
{
	struct platform_device *pdev;

	pdev = kunit_kzalloc(test, sizeof(*pdev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pdev);

	device_initialize(&pdev->dev);
	pdev->dev.release = kunit_kfree_release;

	return pdev;
}

static void setup_mock_mmio(struct kunit *test, struct pt_gpio_chip *pt_gpio, size_t size)
{
	pt_gpio->reg_base = kunit_kzalloc(test, size, GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pt_gpio->reg_base);
}

static void test_pt_gpio_probe_no_acpi_companion(struct kunit *test)
{
	struct platform_device *pdev = create_mock_platform_device(test);
	int ret;

	ACPI_COMPANION_SET(&pdev->dev, NULL);

	ret = pt_gpio_probe(pdev);
	KUNIT_EXPECT_EQ(test, ret, -ENODEV);
}

static void test_pt_gpio_probe_kzalloc_failure(struct kunit *test)
{
	struct platform_device *pdev = create_mock_platform_device(test);
	int ret;

	ACPI_COMPANION_SET(&pdev->dev, (void *)1);

	ret = pt_gpio_probe(pdev);
	KUNIT_EXPECT_EQ(test, ret, -ENOMEM);
}

static void test_pt_gpio_probe_success(struct kunit *test)
{
	struct platform_device *pdev = create_mock_platform_device(test);
	struct pt_gpio_chip *pt_gpio;
	struct resource res = {
		.start = 0x1000,
		.end = 0x1000 + 0x100 - 1,
		.flags = IORESOURCE_MEM,
	};
	int ret;

	ACPI_COMPANION_SET(&pdev->dev, (void *)1);

	pdev->resource = &res;
	pdev->num_resources = 1;

	pt_gpio = kunit_kzalloc(test, sizeof(*pt_gpio), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pt_gpio);

	setup_mock_mmio(test, pt_gpio, 0x100);

	platform_set_drvdata(pdev, pt_gpio);

	ret = pt_gpio_probe(pdev);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static struct kunit_case pt_gpio_test_cases[] = {
	KUNIT_CASE(test_pt_gpio_probe_no_acpi_companion),
	KUNIT_CASE(test_pt_gpio_probe_kzalloc_failure),
	KUNIT_CASE(test_pt_gpio_probe_success),
	{}
};

static struct kunit_suite pt_gpio_test_suite = {
	.name = "pt_gpio",
	.test_cases = pt_gpio_test_cases,
};

kunit_test_suite(pt_gpio_test_suite);