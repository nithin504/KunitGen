```c
#include <kunit/test.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/io.h>

struct amd_gpio {
	void __iomem *iomux_base;
	struct platform_device *pdev;
};

static int test_selector = 0;

static int amd_gpio_mock_function_select(struct amd_gpio *gpio_dev, unsigned int offset, int selector)
{
	if (!gpio_dev->iomux_base) {
		dev_err(&gpio_dev->pdev->dev, "iomux function %d group not supported\n", selector);
		return -EINVAL;
	}
	return 0;
}

static void test_function_select_iomux_base_null(struct kunit *test)
{
	struct amd_gpio gpio_dev = {
		.iomux_base = NULL,
		.pdev = kunit_kzalloc(test, sizeof(*gpio_dev.pdev), GFP_KERNEL)
	};
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev.pdev);

	int ret = amd_gpio_mock_function_select(&gpio_dev, 0, test_selector);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_function_select_iomux_base_valid(struct kunit *test)
{
	char iomux_mem[4096];
	struct amd_gpio gpio_dev = {
		.iomux_base = iomux_mem,
		.pdev = kunit_kzalloc(test, sizeof(*gpio_dev.pdev), GFP_KERNEL)
	};
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev.pdev);

	int ret = amd_gpio_mock_function_select(&gpio_dev, 0, test_selector);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static struct kunit_case generated_test_cases[] = {
	KUNIT_CASE(test_function_select_iomux_base_null),
	KUNIT_CASE(test_function_select_iomux_base_valid),
	{}
};

static struct kunit_suite generated_test_suite = {
	.name = "generated-kunit-test",
	.test_cases = generated_test_cases,
};

kunit_test_suite(generated_test_suite);
```