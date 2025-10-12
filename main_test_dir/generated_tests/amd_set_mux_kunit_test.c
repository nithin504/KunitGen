// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/device.h>

#define NSELECTS 4
#define FUNCTION_INVALID 0xFF
#define FUNCTION_MASK 0x0F

struct amd_gpio;
struct pin_desc;

struct pmx_function {
	const char *groups[NSELECTS];
	unsigned int index;
};

struct pin_group {
	const char *name;
	const unsigned int *pins;
	unsigned int npins;
};

struct amd_gpio {
	void __iomem *iomux_base;
	struct platform_device *pdev;
	struct pin_group *groups;
	struct pinctrl_dev *pctrl;
};

extern struct pmx_function pmx_functions[];
extern struct pin_group pin_groups[];

static struct amd_gpio mock_gpio_dev;
static struct platform_device mock_pdev;
static struct pinctrl_dev mock_pctrl_dev;
static struct device mock_device;
static struct pin_group mock_groups[NSELECTS];
static struct pmx_function mock_pmx_functions[NSELECTS];
static char iomux_memory_region[256];
static const unsigned int mock_pins[] = {0, 1, 2};
static char mux_owner_strings[NSELECTS][32];

void *pinctrl_dev_get_drvdata(struct pinctrl_dev *pctldev)
{
	return &mock_gpio_dev;
}

struct pin_desc *pin_desc_get(struct pinctrl_dev *pctldev, unsigned int pin)
{
	static struct pin_desc pd;
	pd.mux_owner = NULL;
	return &pd;
}

static int amd_set_mux(struct pinctrl_dev *pctrldev, unsigned int function, unsigned int group)
{
	struct amd_gpio *gpio_dev = pinctrl_dev_get_drvdata(pctrldev);
	struct device *dev = &gpio_dev->pdev->dev;
	struct pin_desc *pd;
	int ind, index;

	if (!gpio_dev->iomux_base)
		return -EINVAL;

	for (index = 0; index < NSELECTS; index++) {
		if (strcmp(gpio_dev->groups[group].name,  pmx_functions[function].groups[index]))
			continue;

		if (readb(gpio_dev->iomux_base + pmx_functions[function].index) ==
				FUNCTION_INVALID) {
			dev_err(dev, "IOMUX_GPIO 0x%x not present or supported\n",
				pmx_functions[function].index);
			return -EINVAL;
		}

		writeb(index, gpio_dev->iomux_base + pmx_functions[function].index);

		if (index != (readb(gpio_dev->iomux_base + pmx_functions[function].index) &
					FUNCTION_MASK)) {
			dev_err(dev, "IOMUX_GPIO 0x%x not present or supported\n",
				pmx_functions[function].index);
			return -EINVAL;
		}

		for (ind = 0; ind < gpio_dev->groups[group].npins; ind++) {
			if (strncmp(gpio_dev->groups[group].name, "IMX_F", strlen("IMX_F")))
				continue;

			pd = pin_desc_get(gpio_dev->pctrl, gpio_dev->groups[group].pins[ind]);
			pd->mux_owner = gpio_dev->groups[group].name;
		}
		break;
	}

	return 0;
}

static void test_amd_set_mux_null_iomux_base(struct kunit *test)
{
	mock_gpio_dev.iomux_base = NULL;
	int ret = amd_set_mux(&mock_pctrl_dev, 0, 0);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_amd_set_mux_group_name_mismatch_all_indices(struct kunit *test)
{
	mock_gpio_dev.iomux_base = iomux_memory_region;
	memset(iomux_memory_region, 0, sizeof(iomux_memory_region));

	strcpy(mock_groups[0].name, "GRP_A");
	strcpy(mock_pmx_functions[0].groups[0], "GRP_B");
	strcpy(mock_pmx_functions[0].groups[1], "GRP_C");
	strcpy(mock_pmx_functions[0].groups[2], "GRP_D");
	strcpy(mock_pmx_functions[0].groups[3], "GRP_E");

	int ret = amd_set_mux(&mock_pctrl_dev, 0, 0);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_amd_set_mux_function_invalid_at_index(struct kunit *test)
{
	mock_gpio_dev.iomux_base = iomux_memory_region;
	memset(iomux_memory_region, 0, sizeof(iomux_memory_region));

	strcpy(mock_groups[0].name, "GRP_MATCH");
	strcpy(mock_pmx_functions[0].groups[0], "GRP_MATCH");
	mock_pmx_functions[0].index = 0x10;
	writeb(FUNCTION_INVALID, iomux_memory_region + 0x10);

	int ret = amd_set_mux(&mock_pctrl_dev, 0, 0);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_amd_set_mux_write_readback_mismatch(struct kunit *test)
{
	mock_gpio_dev.iomux_base = iomux_memory_region;
	memset(iomux_memory_region, 0, sizeof(iomux_memory_region));

	strcpy(mock_groups[0].name, "GRP_MATCH");
	strcpy(mock_pmx_functions[0].groups[0], "GRP_MATCH");
	mock_pmx_functions[0].index = 0x10;
	writeb(0xF0, iomux_memory_region + 0x10);

	int ret = amd_set_mux(&mock_pctrl_dev, 0, 0);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_amd_set_mux_success_no_imx_prefix(struct kunit *test)
{
	mock_gpio_dev.iomux_base = iomux_memory_region;
	memset(iomux_memory_region, 0, sizeof(iomux_memory_region));

	strcpy(mock_groups[0].name, "GRP_MATCH");
	strcpy(mock_pmx_functions[0].groups[0], "GRP_MATCH");
	mock_pmx_functions[0].index = 0x10;
	writeb(0x00, iomux_memory_region + 0x10);

	int ret = amd_set_mux(&mock_pctrl_dev, 0, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, readb(iomux_memory_region + 0x10), 0x00);
}

static void test_amd_set_mux_success_with_imx_prefix(struct kunit *test)
{
	mock_gpio_dev.iomux_base = iomux_memory_region;
	memset(iomux_memory_region, 0, sizeof(iomux_memory_region));
	mock_gpio_dev.pctrl = &mock_pctrl_dev;

	strcpy(mock_groups[0].name, "IMX_F_MATCH");
	strcpy(mock_pmx_functions[0].groups[0], "IMX_F_MATCH");
	mock_pmx_functions[0].index = 0x10;
	writeb(0x00, iomux_memory_region + 0x10);
	mock_groups[0].pins = mock_pins;
	mock_groups[0].npins = ARRAY_SIZE(mock_pins);

	int ret = amd_set_mux(&mock_pctrl_dev, 0, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, readb(iomux_memory_region + 0x10), 0x00);
}

static struct kunit_case amd_set_mux_test_cases[] = {
	KUNIT_CASE(test_amd_set_mux_null_iomux_base),
	KUNIT_CASE(test_amd_set_mux_group_name_mismatch_all_indices),
	KUNIT_CASE(test_amd_set_mux_function_invalid_at_index),
	KUNIT_CASE(test_amd_set_mux_write_readback_mismatch),
	KUNIT_CASE(test_amd_set_mux_success_no_imx_prefix),
	KUNIT_CASE(test_amd_set_mux_success_with_imx_prefix),
	{}
};

static struct kunit_suite amd_set_mux_test_suite = {
	.name = "amd_set_mux_test",
	.test_cases = amd_set_mux_test_cases,
};

kunit_test_suite(amd_set_mux_test_suite);

struct pmx_function pmx_functions[NSELECTS] = {
	{ .groups = { "GRP_A", "GRP_B", "GRP_C", "GRP_D" }, .index = 0x10 },
	{ .groups = { "GRP_E", "GRP_F", "GRP_G", "GRP_H" }, .index = 0x14 },
	{ .groups = { "GRP_I", "GRP_J", "GRP_K", "GRP_L" }, .index = 0x18 },
	{ .groups = { "GRP_M", "GRP_N", "GRP_O", "GRP_P" }, .index = 0x1C },
};

struct pin_group pin_groups[NSELECTS] = {
	{ .name = "GRP_A", .pins = mock_pins, .npins = ARRAY_SIZE(mock_pins) },
	{ .name = "GRP_B", .pins = mock_pins, .npins = ARRAY_SIZE(mock_pins) },
	{ .name = "GRP_C", .pins = mock_pins, .npins = ARRAY_SIZE(mock_pins) },
	{ .name = "GRP_D", .pins = mock_pins, .npins = ARRAY_SIZE(mock_pins) },
};
