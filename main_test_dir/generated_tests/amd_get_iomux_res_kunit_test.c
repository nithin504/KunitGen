```c
// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/property.h>
#include <linux/io.h>

struct amd_gpio {
	struct platform_device *pdev;
	void __iomem *iomux_base;
};

struct pinctrl_desc {
	const struct pinctrl_pin_desc *pins;
	unsigned int npins;
	const char *name;
	const struct pinctrl_ops *pctlops;
	const struct pinmux_ops *pmxops;
	const struct pinconf_ops *confops;
	struct module *owner;
};

extern struct pinctrl_desc amd_pinctrl_desc;

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

static const struct property_entry iomux_prop_present[] = {
	PROPERTY_ENTRY_STRING_ARRAY("pinctrl-resource-names", "iomux"),
	{}
};

static const struct property_entry iomux_prop_absent[] = {
	PROPERTY_ENTRY_STRING_ARRAY("pinctrl-resource-names", "other"),
	{}
};

static void test_amd_get_iomux_res_property_not_found(struct kunit *test)
{
	struct platform_device *pdev = kunit_kzalloc(test, sizeof(*pdev), GFP_KERNEL);
	struct amd_gpio gpio_dev = { .pdev = pdev };
	struct pinctrl_desc *desc = &amd_pinctrl_desc;

	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pdev);

	device_property_add_properties(&pdev->dev, iomux_prop_absent);

	amd_get_iomux_res(&gpio_dev);

	KUNIT_EXPECT_PTR_EQ(test, desc->pmxops, (const struct pinmux_ops *)NULL);
}

static void test_amd_get_iomux_res_ioremap_failure(struct kunit *test)
{
	struct platform_device *pdev = kunit_kzalloc(test, sizeof(*pdev), GFP_KERNEL);
	struct amd_gpio gpio_dev = { .pdev = pdev };
	struct pinctrl_desc *desc = &amd_pinctrl_desc;

	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pdev);

	device_property_add_properties(&pdev->dev, iomux_prop_present);

	amd_get_iomux_res(&gpio_dev);

	KUNIT_EXPECT_PTR_EQ(test, desc->pmxops, (const struct pinmux_ops *)NULL);
}

static void test_amd_get_iomux_res_success(struct kunit *test)
{
	struct platform_device *pdev = kunit_kzalloc(test, sizeof(*pdev), GFP_KERNEL);
	struct amd_gpio gpio_dev = { .pdev = pdev };
	struct pinctrl_desc *desc = &amd_pinctrl_desc;
	struct resource res = {
		.start = 0x1000,
		.end = 0x1FFF,
		.flags = IORESOURCE_MEM,
		.name = "iomux"
	};

	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pdev);

	device_property_add_properties(&pdev->dev, iomux_prop_present);
	pdev->num_resources = 1;
	pdev->resource = &res;

	desc->pmxops = (const struct pinmux_ops *)0xDEADBEEF;

	amd_get_iomux_res(&gpio_dev);

	KUNIT_EXPECT_NE(test, gpio_dev.iomux_base, (void __iomem *)NULL);
	KUNIT_EXPECT_PTR_NE(test, desc->pmxops, (const struct pinmux_ops *)NULL);
}

static struct kunit_case amd_get_iomux_res_test_cases[] = {
	KUNIT_CASE(test_amd_get_iomux_res_property_not_found),
	KUNIT_CASE(test_amd_get_iomux_res_ioremap_failure),
	KUNIT_CASE(test_amd_get_iomux_res_success),
	{}
};

static struct kunit_suite amd_get_iomux_res_test_suite = {
	.name = "amd_get_iomux_res_test",
	.test_cases = amd_get_iomux_res_test_cases,
};

kunit_test_suite(amd_get_iomux_res_test_suite);
```