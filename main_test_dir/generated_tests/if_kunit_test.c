// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/io.h>

struct amd_gpio {
	void __iomem *iomux_base;
	struct platform_device *pdev;
};

static int test_selector = 0;

static int mock_dev_err_called = 0;
#define dev_err(dev, fmt, ...) do { \
	mock_dev_err_called++; \
} while (0)

static int test_function_with_iomux_check(struct amd_gpio *gpio_dev, int selector)
{
	if (!gpio_dev->iomux_base) {
		dev_err(&gpio_dev->pdev->dev, "iomux function %d group not supported\n", selector);
		return -EINVAL;
	}
	return 0;
}

static void test_function_iomux_base_null(struct kunit *test)
{
	struct amd_gpio gpio_dev = {
		.iomux_base = NULL,
		.pdev = kunit_kzalloc(test, sizeof(*gpio_dev.pdev), GFP_KERNEL)
	};
	mock_dev_err_called = 0;
	int ret = test_function_with_iomux_check(&gpio_dev, test_selector);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
	KUNIT_EXPECT_EQ(test, mock_dev_err_called, 1);
}

static void test_function_iomux_base_valid(struct kunit *test)
{
	char dummy_base[4096];
	struct amd_gpio gpio_dev = {
		.iomux_base = dummy_base,
		.pdev = kunit_kzalloc(test, sizeof(*gpio_dev.pdev), GFP_KERNEL)
	};
	mock_dev_err_called = 0;
	int ret = test_function_with_iomux_check(&gpio_dev, test_selector);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, mock_dev_err_called, 0);
}

static struct kunit_case generated_test_cases[] = {
	KUNIT_CASE(test_function_iomux_base_null),
	KUNIT_CASE(test_function_iomux_base_valid),
	{}
};

static struct kunit_suite generated_test_suite = {
	.name = "generated-iomux-check-test",
	.test_cases = generated_test_cases,
};

kunit_test_suite(generated_test_suite);
