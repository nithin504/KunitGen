// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/pinctrl/pinctrl.h>

#define pinctrl_dev_get_drvdata mock_pinctrl_dev_get_drvdata
#define dev_err mock_dev_err

static struct amd_gpio *mock_pinctrl_dev_get_drvdata(struct pinctrl_dev *pctldev)
{
	return (struct amd_gpio *)pctldev;
}

static int mock_dev_err_count;
static void mock_dev_err(const struct device *dev, const char *fmt, ...) { mock_dev_err_count++; }

static const struct {
	const char * const *groups;
	unsigned int ngroups;
} pmx_functions[] = {
	{ (const char * const[]){ "group0", "group1" }, 2 },
	{ (const char * const[]){ "group2" }, 1 },
	{ (const char * const[]){ "group3", "group4", "group5" }, 3 },
	{ NULL, 0 },
	{ (const char * const[]){ "group6" }, 1 },
};

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

static void test_amd_get_groups_success(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	struct amd_gpio gpio_dev;
	struct platform_device pdev;
	const char * const *groups;
	unsigned int num_groups;
	int ret;

	gpio_dev.iomux_base = (void *)0x1000;
	gpio_dev.pdev = &pdev;
	pctldev = (struct pinctrl_dev)&gpio_dev;

	ret = amd_get_groups(&pctldev, 0, &groups, &num_groups);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_PTR_EQ(test, groups, pmx_functions[0].groups);
	KUNIT_EXPECT_EQ(test, num_groups, 2);

	ret = amd_get_groups(&pctldev, 1, &groups, &num_groups);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_PTR_EQ(test, groups, pmx_functions[1].groups);
	KUNIT_EXPECT_EQ(test, num_groups, 1);

	ret = amd_get_groups(&pctldev, 2, &groups, &num_groups);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_PTR_EQ(test, groups, pmx_functions[2].groups);
	KUNIT_EXPECT_EQ(test, num_groups, 3);
}

static void test_amd_get_groups_no_iomux_base(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	struct amd_gpio gpio_dev;
	struct platform_device pdev;
	const char * const *groups;
	unsigned int num_groups;
	int ret;

	gpio_dev.iomux_base = NULL;
	gpio_dev.pdev = &pdev;
	pctldev = (struct pinctrl_dev)&gpio_dev;
	mock_dev_err_count = 0;

	ret = amd_get_groups(&pctldev, 0, &groups, &num_groups);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
	KUNIT_EXPECT_EQ(test, mock_dev_err_count, 1);
}

static void test_amd_get_groups_null_groups(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	struct amd_gpio gpio_dev;
	struct platform_device pdev;
	unsigned int num_groups;
	int ret;

	gpio_dev.iomux_base = (void *)0x1000;
	gpio_dev.pdev = &pdev;
	pctldev = (struct pinctrl_dev)&gpio_dev;

	ret = amd_get_groups(&pctldev, 0, NULL, &num_groups);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_get_groups_null_num_groups(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	struct amd_gpio gpio_dev;
	struct platform_device pdev;
	const char * const *groups;
	int ret;

	gpio_dev.iomux_base = (void *)0x1000;
	gpio_dev.pdev = &pdev;
	pctldev = (struct pinctrl_dev)&gpio_dev;

	ret = amd_get_groups(&pctldev, 0, &groups, NULL);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_get_groups_null_groups_and_num_groups(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	struct amd_gpio gpio_dev;
	struct platform_device pdev;
	int ret;

	gpio_dev.iomux_base = (void *)0x1000;
	gpio_dev.pdev = &pdev;
	pctldev = (struct pinctrl_dev)&gpio_dev;

	ret = amd_get_groups(&pctldev, 0, NULL, NULL);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_get_groups_null_groups_struct(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	struct amd_gpio gpio_dev;
	struct platform_device pdev;
	const char * const *groups;
	unsigned int num_groups;
	int ret;

	gpio_dev.iomux_base = (void *)0x1000;
	gpio_dev.pdev = &pdev;
	pctldev = (struct pinctrl_dev)&gpio_dev;

	ret = amd_get_groups(&pctldev, 3, &groups, &num_groups);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_PTR_EQ(test, groups, NULL);
	KUNIT_EXPECT_EQ(test, num_groups, 0);
}

static void test_amd_get_groups_out_of_bounds_selector(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	struct amd_gpio gpio_dev;
	struct platform_device pdev;
	const char * const *groups;
	unsigned int num_groups;
	int ret;

	gpio_dev.iomux_base = (void *)0x1000;
	gpio_dev.pdev = &pdev;
	pctldev = (struct pinctrl_dev)&gpio_dev;

	ret = amd_get_groups(&pctldev, 100, &groups, &num_groups);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static struct kunit_case amd_get_groups_test_cases[] = {
	KUNIT_CASE(test_amd_get_groups_success),
	KUNIT_CASE(test_amd_get_groups_no_iomux_base),
	KUNIT_CASE(test_amd_get_groups_null_groups),
	KUNIT_CASE(test_amd_get_groups_null_num_groups),
	KUNIT_CASE(test_amd_get_groups_null_groups_and_num_groups),
	KUNIT_CASE(test_amd_get_groups_null_groups_struct),
	KUNIT_CASE(test_amd_get_groups_out_of_bounds_selector),
	{}
};

static struct kunit_suite amd_get_groups_test_suite = {
	.name = "amd_get_groups_test",
	.test_cases = amd_get_groups_test_cases,
};

kunit_test_suite(amd_get_groups_test_suite);