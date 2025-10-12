```c
// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/io.h>
#include "pinctrl-amd.c"

static bool mock_do_amd_gpio_irq_handler_return = false;
static int mock_do_amd_gpio_irq_handler_call_count = 0;
static void *mock_do_amd_gpio_irq_handler_last_dev_id = NULL;

bool do_amd_gpio_irq_handler(int irq, void *dev_id)
{
	mock_do_amd_gpio_irq_handler_call_count++;
	mock_do_amd_gpio_irq_handler_last_dev_id = dev_id;
	return mock_do_amd_gpio_irq_handler_return;
}

static void test_amd_gpio_check_wake_returns_true_when_handler_returns_true(struct kunit *test)
{
	mock_do_amd_gpio_irq_handler_return = true;
	mock_do_amd_gpio_irq_handler_call_count = 0;
	mock_do_amd_gpio_irq_handler_last_dev_id = NULL;

	void *test_dev_id = (void *)0xDEADBEEF;
	bool result = amd_gpio_check_wake(test_dev_id);

	KUNIT_EXPECT_TRUE(test, result);
	KUNIT_EXPECT_EQ(test, mock_do_amd_gpio_irq_handler_call_count, 1);
	KUNIT_EXPECT_PTR_EQ(test, mock_do_amd_gpio_irq_handler_last_dev_id, test_dev_id);
}

static void test_amd_gpio_check_wake_returns_false_when_handler_returns_false(struct kunit *test)
{
	mock_do_amd_gpio_irq_handler_return = false;
	mock_do_amd_gpio_irq_handler_call_count = 0;
	mock_do_amd_gpio_irq_handler_last_dev_id = NULL;

	void *test_dev_id = (void *)0xCAFEBABE;
	bool result = amd_gpio_check_wake(test_dev_id);

	KUNIT_EXPECT_FALSE(test, result);
	KUNIT_EXPECT_EQ(test, mock_do_amd_gpio_irq_handler_call_count, 1);
	KUNIT_EXPECT_PTR_EQ(test, mock_do_amd_gpio_irq_handler_last_dev_id, test_dev_id);
}

static void test_amd_gpio_check_wake_with_null_dev_id(struct kunit *test)
{
	mock_do_amd_gpio_irq_handler_return = true;
	mock_do_amd_gpio_irq_handler_call_count = 0;
	mock_do_amd_gpio_irq_handler_last_dev_id = NULL;

	bool result = amd_gpio_check_wake(NULL);

	KUNIT_EXPECT_TRUE(test, result);
	KUNIT_EXPECT_EQ(test, mock_do_amd_gpio_irq_handler_call_count, 1);
	KUNIT_EXPECT_NULL(test, mock_do_amd_gpio_irq_handler_last_dev_id);
}

static struct kunit_case generated_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_check_wake_returns_true_when_handler_returns_true),
	KUNIT_CASE(test_amd_gpio_check_wake_returns_false_when_handler_returns_false),
	KUNIT_CASE(test_amd_gpio_check_wake_with_null_dev_id),
	{}
};

static struct kunit_suite generated_test_suite = {
	.name = "amd_gpio_check_wake_test",
	.test_cases = generated_test_cases,
};

kunit_test_suite(generated_test_suite);
```