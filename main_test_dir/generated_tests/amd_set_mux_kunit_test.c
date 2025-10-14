// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#define NSELECTS 4
#define FUNCTION_INVALID 0xFF
#define FUNCTION_MASK 0x0F

struct amd_gpio {
	void __iomem *iomux_base;
	struct platform_device *pdev;
	struct pinctrl_dev *pctrl;
	struct {
		const char *name;
		const unsigned int *pins;
		unsigned int npins;
	} groups[4];
};

struct pin_desc {
	const char *mux_owner;
};

struct pmx_function {
	const char *groups[NSELECTS];
	unsigned int index;
};

static struct pmx_function pmx_functions[] = {
	{
		.groups = {"group0_match", "group1_nomatch", "group2_nomatch", "group3_nomatch"},
		.index = 0x10,
	},
	{
		.groups = {"group0_nomatch", "group1_match", "group2_nomatch", "group3_nomatch"},
		.index = 0x20,
	}
};

static const unsigned int dummy_pins[] = {0, 1, 2};
static struct pin_desc pin_descriptors[3];

static struct pin_desc *pin_desc_get(struct pinctrl_dev *pctldev, unsigned int pin)
{
	if (pin >= ARRAY_SIZE(pin_descriptors))
		return ERR_PTR(-EINVAL);
	return &pin_descriptors[pin];
}

static struct amd_gpio *mock_gpio_dev;
static char iomux_memory_region[0x100];

static void *mock_pinctrl_dev_get_drvdata(struct pinctrl_dev *pctrldev)
{
	return mock_gpio_dev;
}

#define pinctrl_dev_get_drvdata mock_pinctrl_dev_get_drvdata
#define readb(addr) (*(volatile u8 *)(addr))
#define writeb(val, addr) (*(volatile u8 *)(addr) = (val))

#include "pinctrl-amd.c"

static int test_init(struct kunit *test)
{
	mock_gpio_dev = kunit_kzalloc(test, sizeof(*mock_gpio_dev), GFP_KERNEL);
	if (!mock_gpio_dev)
		return -ENOMEM;

	mock_gpio_dev->iomux_base = iomux_memory_region;
	mock_gpio_dev->pdev = kunit_kzalloc(test, sizeof(*(mock_gpio_dev->pdev)), GFP_KERNEL);
	if (!mock_gpio_dev->pdev)
		return -ENOMEM;

	mock_gpio_dev->pctrl = (struct pinctrl_dev *)0xDEADBEEF;

	mock_gpio_dev->groups[0].name = "group0_match";
	mock_gpio_dev->groups[0].pins = dummy_pins;
	mock_gpio_dev->groups[0].npins = ARRAY_SIZE(dummy_pins);

	mock_gpio_dev->groups[1].name = "group1_match";
	mock_gpio_dev->groups[1].pins = dummy_pins;
	mock_gpio_dev->groups[1].npins = ARRAY_SIZE(dummy_pins);

	mock_gpio_dev->groups[2].name = "IMX_F_group";
	mock_gpio_dev->groups[2].pins = dummy_pins;
	mock_gpio_dev->groups[2].npins = ARRAY_SIZE(dummy_pins);

	memset(iomux_memory_region, 0, sizeof(iomux_memory_region));
	memset(pin_descriptors, 0, sizeof(pin_descriptors));

	return 0;
}

static void test_amd_set_mux_null_iomux_base(struct kunit *test)
{
	struct pinctrl_dev pctrldev;
	mock_gpio_dev->iomux_base = NULL;
	int ret = amd_set_mux(&pctrldev, 0, 0);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_amd_set_mux_no_group_match(struct kunit *test)
{
	struct pinctrl_dev pctrldev;
	mock_gpio_dev->iomux_base = iomux_memory_region;
	writeb(0x0, mock_gpio_dev->iomux_base + pmx_functions[0].index);
	int ret = amd_set_mux(&pctrldev, 0, 1); // group1 vs function0.group0_match
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_set_mux_function_invalid(struct kunit *test)
{
	struct pinctrl_dev pctrldev;
	writeb(FUNCTION_INVALID, mock_gpio_dev->iomux_base + pmx_functions[0].index);
	int ret = amd_set_mux(&pctrldev, 0, 0);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_amd_set_mux_write_readback_fail(struct kunit *test)
{
	struct pinctrl_dev pctrldev;
	writeb(0x0, mock_gpio_dev->iomux_base + pmx_functions[0].index);
	int ret = amd_set_mux(&pctrldev, 0, 0);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_amd_set_mux_success_with_imx_prefix(struct kunit *test)
{
	struct pinctrl_dev pctrldev;
	writeb(0x0, mock_gpio_dev->iomux_base + pmx_functions[1].index);
	int ret = amd_set_mux(&pctrldev, 1, 2); // matches IMX_F_group
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_STR_EQ(test, pin_descriptors[0].mux_owner, "IMX_F_group");
	KUNIT_EXPECT_STR_EQ(test, pin_descriptors[1].mux_owner, "IMX_F_group");
	KUNIT_EXPECT_STR_EQ(test, pin_descriptors[2].mux_owner, "IMX_F_group");
}

static void test_amd_set_mux_success_without_imx_prefix(struct kunit *test)
{
	struct pinctrl_dev pctrldev;
	writeb(0x0, mock_gpio_dev->iomux_base + pmx_functions[0].index);
	int ret = amd_set_mux(&pctrldev, 0, 0); // group0_match does not start with IMX_F
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_PTR_EQ(test, (void *)pin_descriptors[0].mux_owner, (void *)NULL);
	KUNIT_EXPECT_PTR_EQ(test, (void *)pin_descriptors[1].mux_owner, (void *)NULL);
	KUNIT_EXPECT_PTR_EQ(test, (void *)pin_descriptors[2].mux_owner, (void *)NULL);
}

static struct kunit_case amd_set_mux_test_cases[] = {
	KUNIT_CASE(test_amd_set_mux_null_iomux_base),
	KUNIT_CASE(test_amd_set_mux_no_group_match),
	KUNIT_CASE(test_amd_set_mux_function_invalid),
	KUNIT_CASE(test_amd_set_mux_write_readback_fail),
	KUNIT_CASE(test_amd_set_mux_success_with_imx_prefix),
	KUNIT_CASE(test_amd_set_mux_success_without_imx_prefix),
	{}
};

static struct kunit_suite amd_set_mux_test_suite = {
	.name = "amd_set_mux_test",
	.init = test_init,
	.test_cases = amd_set_mux_test_cases,
};

kunit_test_suite(amd_set_mux_test_suite);
