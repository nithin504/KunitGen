// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/property.h>
#include <linux/io.h>
#include <linux/err.h>

struct amd_gpio {
	struct platform_device *pdev;
	void __iomem *iomux_base;
};

struct pinctrl_desc {
	const struct pinmux_ops *pmxops;
};

static struct pinctrl_desc amd_pinctrl_desc = {
	.pmxops = (void *)0x12345678,
};

#define IS_ERR(ptr) unlikely((unsigned long)(ptr) >= (unsigned long)-MAX_ERRNO)
#define PTR_ERR(ptr) ((long)(ptr))
#define ERR_PTR(err) ((void *)(long)(err))

static void *mock_iomux_base = (void *)0xDEADBEEF;

static int mock_device_property_match_string_return = 0;
static bool mock_devm_platform_ioremap_resource_returns_err = false;

int device_property_match_string(struct device *dev, const char *propname, const char *string)
{
	return mock_device_property_match_string_return;
}

void *devm_platform_ioremap_resource(struct platform_device *pdev, unsigned int index)
{
	if (mock_devm_platform_ioremap_resource_returns_err)
		return ERR_PTR(-ENODEV);
	return mock_iomux_base;
}

static void amd_get_iomux_res(struct amd_gpio *gpio_dev)
{
	struct pinctrl_desc *desc = &amd_pinctrl_desc;
	struct device *dev = &gpio_dev->pdev->dev;
	int index;

	index = device_property_match_string(dev, "pinctrl-resource-names", "iomux");
	if (index < 0) {
		dev_dbg(dev, "iomux not supported\n");
		goto out_no_pinmux;
	}

	gpio_dev->iomux_base = devm_platform_ioremap_resource(gpio_dev->pdev, index);
	if (IS_ERR(gpio_dev->iomux_base)) {
		dev_dbg(dev, "iomux not supported %d io resource\n", index);
		goto out_no_pinmux;
	}

	return;

out_no_pinmux:
	desc->pmxops = NULL;
}

static void test_amd_get_iomux_res_property_not_found(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	struct platform_device pdev = {0};
	struct device dev = {0};

	gpio_dev.pdev = &pdev;
	pdev.dev = dev;

	mock_device_property_match_string_return = -1;
	amd_get_iomux_res(&gpio_dev);

	KUNIT_EXPECT_PTR_EQ(test, amd_pinctrl_desc.pmxops, (const struct pinmux_ops *)NULL);
}

static void test_amd_get_iomux_res_ioremap_fails(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	struct platform_device pdev = {0};
	struct device dev = {0};

	gpio_dev.pdev = &pdev;
	pdev.dev = dev;

	mock_device_property_match_string_return = 0;
	mock_devm_platform_ioremap_resource_returns_err = true;
	amd_get_iomux_res(&gpio_dev);

	KUNIT_EXPECT_PTR_EQ(test, amd_pinctrl_desc.pmxops, (const struct pinmux_ops *)NULL);
}

static void test_amd_get_iomux_res_success(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	struct platform_device pdev = {0};
	struct device dev = {0};

	gpio_dev.pdev = &pdev;
	pdev.dev = dev;

	mock_device_property_match_string_return = 0;
	mock_devm_platform_ioremap_resource_returns_err = false;
	amd_get_iomux_res(&gpio_dev);

	KUNIT_EXPECT_PTR_NE(test, amd_pinctrl_desc.pmxops, (const struct pinmux_ops *)NULL);
	KUNIT_EXPECT_PTR_EQ(test, gpio_dev.iomux_base, mock_iomux_base);
}

static struct kunit_case amd_get_iomux_res_test_cases[] = {
	KUNIT_CASE(test_amd_get_iomux_res_property_not_found),
	KUNIT_CASE(test_amd_get_iomux_res_ioremap_fails),
	KUNIT_CASE(test_amd_get_iomux_res_success),
	{}
};

static struct kunit_suite amd_get_iomux_res_test_suite = {
	.name = "amd_get_iomux_res_test",
	.test_cases = amd_get_iomux_res_test_cases,
};

kunit_test_suite(amd_get_iomux_res_test_suite);
