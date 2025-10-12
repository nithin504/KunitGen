// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/platform_device.h>

// Mock structures and data
struct amd_gpio {
	void __iomem *iomux_base;
	struct platform_device *pdev;
};

struct amd_pmx_func {
	const char * const *groups;
	unsigned int ngroups;
};

#define MAX_FUNCTIONS 10
static struct amd_pmx_func pmx_functions[MAX_FUNCTIONS];

// Include the function under test
static int amd_get_groups(struct pinctrl_dev *pctrldev, unsigned int selector,
			  const char * const **groups,
			  unsigned int * const num_groups)
{
	struct amd_gpio *gpio_dev = pinctrl_dev_get_drvdata(pctrldev);

	if (!gpio_dev->iomux_base) {
		dev_err(&gpio_dev->pdev->dev, "iomux function %d group not supported\n", selector);
		return -EINVAL;
	}

	*groups = pmx_functions[selector].groups;
	*num_groups = pmx_functions[selector].ngroups;
	return 0;
}

// Mock pinctrl_dev_get_drvdata
static void *mock_drvdata;
static void *pinctrl_dev_get_drvdata(struct pinctrl_dev *pctrldev)
{
	return mock_drvdata;
}

// Test cases
static void test_amd_get_groups_iomux_base_null(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	struct amd_gpio gpio_dev = {
		.iomux_base = NULL,
		.pdev = kunit_kzalloc(test, sizeof(*gpio_dev.pdev), GFP_KERNEL)
	};
	const char * const *groups;
	unsigned int num_groups;

	mock_drvdata = &gpio_dev;

	int ret = amd_get_groups(&pctldev, 0, &groups, &num_groups);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_amd_get_groups_success(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	char test_mmio_region[4096];
	struct amd_gpio gpio_dev = {
		.iomux_base = test_mmio_region,
		.pdev = kunit_kzalloc(test, sizeof(*gpio_dev.pdev), GFP_KERNEL)
	};
	const char *group_names[] = {"group1", "group2"};
	const char * const *groups;
	unsigned int num_groups = 0;

	pmx_functions[0].groups = group_names;
	pmx_functions[0].ngroups = 2;

	mock_drvdata = &gpio_dev;

	int ret = amd_get_groups(&pctldev, 0, &groups, &num_groups);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_PTR_EQ(test, groups, group_names);
	KUNIT_EXPECT_EQ(test, num_groups, 2U);
}

static void test_amd_get_groups_different_selector(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	char test_mmio_region[4096];
	struct amd_gpio gpio_dev = {
		.iomux_base = test_mmio_region,
		.pdev = kunit_kzalloc(test, sizeof(*gpio_dev.pdev), GFP_KERNEL)
	};
	const char *group_names[] = {"groupA"};
	const char * const *groups;
	unsigned int num_groups = 0;

	pmx_functions[5].groups = group_names;
	pmx_functions[5].ngroups = 1;

	mock_drvdata = &gpio_dev;

	int ret = amd_get_groups(&pctldev, 5, &groups, &num_groups);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_PTR_EQ(test, groups, group_names);
	KUNIT_EXPECT_EQ(test, num_groups, 1U);
}

static void test_amd_get_groups_zero_groups(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	char test_mmio_region[4096];
	struct amd_gpio gpio_dev = {
		.iomux_base = test_mmio_region,
		.pdev = kunit_kzalloc(test, sizeof(*gpio_dev.pdev), GFP_KERNEL)
	};
	const char * const *groups;
	unsigned int num_groups = 99; // Initialize to non-zero to check if overwritten

	pmx_functions[1].groups = NULL;
	pmx_functions[1].ngroups = 0;

	mock_drvdata = &gpio_dev;

	int ret = amd_get_groups(&pctldev, 1, &groups, &num_groups);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_PTR_EQ(test, groups, (const char * const *)NULL);
	KUNIT_EXPECT_EQ(test, num_groups, 0U);
}

static struct kunit_case amd_get_groups_test_cases[] = {
	KUNIT_CASE(test_amd_get_groups_iomux_base_null),
	KUNIT_CASE(test_amd_get_groups_success),
	KUNIT_CASE(test_amd_get_groups_different_selector),
	KUNIT_CASE(test_amd_get_groups_zero_groups),
	{}
};

static struct kunit_suite amd_get_groups_test_suite = {
	.name = "amd_get_groups_test",
	.test_cases = amd_get_groups_test_cases,
};

kunit_test_suite(amd_get_groups_test_suite);
