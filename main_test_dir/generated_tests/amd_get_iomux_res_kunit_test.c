// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/property.h>
#include <linux/err.h>

// Mocking headers and definitions
#define PTR_ERR_OR_ZERO(ptr) (!IS_ERR(ptr) ? 0 : PTR_ERR(ptr))

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

// Function under test
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

// --- Begin Mocks ---
static const char * const test_pinctrl_names[] = { "other", "iomux" };
static int mock_device_property_match_string_return = 1;
static bool mock_devm_platform_ioremap_resource_fail = false;
static void __iomem *mock_iomux_base = (void __iomem *)0xDEADBEEF;

int device_property_match_string(const struct device *dev, const char *propname, const char *string)
{
	return mock_device_property_match_string_return;
}

void __iomem *devm_platform_ioremap_resource(struct platform_device *pdev, unsigned int index)
{
	if (mock_devm_platform_ioremap_resource_fail)
		return ERR_PTR(-ENOMEM);
	return mock_iomux_base;
}

// Global state for test verification
static struct pinctrl_desc amd_pinctrl_desc = {
	.pmxops = (const struct pinmux_ops *)0x12345678,
};

// --- End Mocks ---

// Test Cases
static void test_amd_get_iomux_res_success(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	struct platform_device pdev = {0};
	gpio_dev.pdev = &pdev;

	mock_device_property_match_string_return = 1; // Found at index 1
	mock_devm_platform_ioremap_resource_fail = false;

	amd_get_iomux_res(&gpio_dev);

	KUNIT_EXPECT_PTR_EQ(test, gpio_dev.iomux_base, mock_iomux_base);
	KUNIT_EXPECT_PTR_NE(test, (void *)amd_pinctrl_desc.pmxops, (void *)NULL);
}

static void test_amd_get_iomux_res_property_not_found(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	struct platform_device pdev = {0};
	gpio_dev.pdev = &pdev;

	mock_device_property_match_string_return = -EINVAL; // Not found
	mock_devm_platform_ioremap_resource_fail = false;

	amd_get_iomux_res(&gpio_dev);

	KUNIT_EXPECT_PTR_EQ(test, gpio_dev.iomux_base, (void __iomem *)NULL);
	KUNIT_EXPECT_PTR_EQ(test, (void *)amd_pinctrl_desc.pmxops, (void *)NULL);
}

static void test_amd_get_iomux_res_ioremap_fail(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	struct platform_device pdev = {0};
	gpio_dev.pdev = &pdev;

	mock_device_property_match_string_return = 0; // Found at index 0
	mock_devm_platform_ioremap_resource_fail = true;

	amd_get_iomux_res(&gpio_dev);

	KUNIT_EXPECT_TRUE(test, IS_ERR(gpio_dev.iomux_base));
	KUNIT_EXPECT_PTR_EQ(test, (void *)amd_pinctrl_desc.pmxops, (void *)NULL);
}

static struct kunit_case amd_get_iomux_res_test_cases[] = {
	KUNIT_CASE(test_amd_get_iomux_res_success),
	KUNIT_CASE(test_amd_get_iomux_res_property_not_found),
	KUNIT_CASE(test_amd_get_iomux_res_ioremap_fail),
	{}
};

static struct kunit_suite amd_get_iomux_res_test_suite = {
	.name = "amd_get_iomux_res_test",
	.test_cases = amd_get_iomux_res_test_cases,
};

kunit_test_suite(amd_get_iomux_res_test_suite);
