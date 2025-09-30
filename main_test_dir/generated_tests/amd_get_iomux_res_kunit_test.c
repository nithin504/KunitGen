// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/err.h>
#include <linux/io.h>

static struct pinctrl_desc amd_pinctrl_desc;

struct amd_gpio {
	struct platform_device *pdev;
	void __iomem *iomux_base;
};

static int device_property_match_string_mock(struct device *dev, const char *propname, const char *string)
{
	return -1;
}

static void __iomem *devm_platform_ioremap_resource_mock(struct platform_device *pdev, int index)
{
	return ERR_PTR(-ENXIO);
}

#define device_property_match_string device_property_match_string_mock
#define devm_platform_ioremap_resource devm_platform_ioremap_resource_mock

static void amd_get_iomux_res(struct amd_gpio *gpio_dev)
{
	struct pinctrl_desc *desc = &amd_pinctrl_desc;
	struct device *dev = &gpio_dev->pdev->dev;
	int index;

	index = device_property_match_string(dev, "pinctrl-resource-names",  "iomux");
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

static void test_amd_get_iomux_res_device_property_fail(struct kunit *test)
{
	struct amd_gpio gpio_dev;
	struct platform_device pdev;
	struct device dev;
	
	gpio_dev.pdev = &pdev;
	pdev.dev = &dev;
	amd_pinctrl_desc.pmxops = (void *)0x1234;
	
	amd_get_iomux_res(&gpio_dev);
	
	KUNIT_EXPECT_PTR_EQ(test, amd_pinctrl_desc.pmxops, NULL);
}

static int device_property_match_string_success_mock(struct device *dev, const char *propname, const char *string)
{
	return 0;
}

static void __iomem *devm_platform_ioremap_resource_success_mock(struct platform_device *pdev, int index)
{
	return (void __iomem *)0x1000;
}

static void test_amd_get_iomux_res_ioremap_success(struct kunit *test)
{
	struct amd_gpio gpio_dev;
	struct platform_device pdev;
	struct device dev;
	
	gpio_dev.pdev = &pdev;
	pdev.dev = &dev;
	amd_pinctrl_desc.pmxops = (void *)0x1234;
	
	device_property_match_string = device_property_match_string_success_mock;
	devm_platform_ioremap_resource = devm_platform_ioremap_resource_success_mock;
	
	amd_get_iomux_res(&gpio_dev);
	
	KUNIT_EXPECT_PTR_EQ(test, gpio_dev.iomux_base, (void __iomem *)0x1000);
	KUNIT_EXPECT_PTR_EQ(test, amd_pinctrl_desc.pmxops, (void *)0x1234);
	
	device_property_match_string = device_property_match_string_mock;
	devm_platform_ioremap_resource = devm_platform_ioremap_resource_mock;
}

static void __iomem *devm_platform_ioremap_resource_fail_mock(struct platform_device *pdev, int index)
{
	return ERR_PTR(-EINVAL);
}

static void test_amd_get_iomux_res_ioremap_fail(struct kunit *test)
{
	struct amd_gpio gpio_dev;
	struct platform_device pdev;
	struct device dev;
	
	gpio_dev.pdev = &pdev;
	pdev.dev = &dev;
	amd_pinctrl_desc.pmxops = (void *)0x1234;
	
	device_property_match_string = device_property_match_string_success_mock;
	devm_platform_ioremap_resource = devm_platform_ioremap_resource_fail_mock;
	
	amd_get_iomux_res(&gpio_dev);
	
	KUNIT_EXPECT_TRUE(test, IS_ERR(gpio_dev.iomux_base));
	KUNIT_EXPECT_PTR_EQ(test, amd_pinctrl_desc.pmxops, NULL);
	
	device_property_match_string = device_property_match_string_mock;
	devm_platform_ioremap_resource = devm_platform_ioremap_resource_mock;
}

static struct kunit_case amd_get_iomux_res_test_cases[] = {
	KUNIT_CASE(test_amd_get_iomux_res_device_property_fail),
	KUNIT_CASE(test_amd_get_iomux_res_ioremap_success),
	KUNIT_CASE(test_amd_get_iomux_res_ioremap_fail),
	{}
};

static struct kunit_suite amd_get_iomux_res_test_suite = {
	.name = "amd_get_iomux_res_test",
	.test_cases = amd_get_iomux_res_test_cases,
};

kunit_test_suite(amd_get_iomux_res_test_suite);