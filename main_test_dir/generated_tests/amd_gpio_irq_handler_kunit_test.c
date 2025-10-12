// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqreturn.h>

// Mock implementation of do_amd_gpio_irq_handler
static int mock_do_amd_gpio_irq_handler_result = 0;
static int mock_do_amd_gpio_irq_handler_call_count = 0;

static int do_amd_gpio_irq_handler(int irq, void *dev_id)
{
	mock_do_amd_gpio_irq_handler_call_count++;
	return mock_do_amd_gpio_irq_handler_result;
}

// Function under test
static irqreturn_t amd_gpio_irq_handler(int irq, void *dev_id)
{
	return IRQ_RETVAL(do_amd_gpio_irq_handler(irq, dev_id));
}

// Test case: amd_gpio_irq_handler returns IRQ_HANDLED when do_amd_gpio_irq_handler returns non-zero
static void test_amd_gpio_irq_handler_handled(struct kunit *test)
{
	mock_do_amd_gpio_irq_handler_result = 1;
	mock_do_amd_gpio_irq_handler_call_count = 0;

	irqreturn_t ret = amd_gpio_irq_handler(42, NULL);

	KUNIT_EXPECT_EQ(test, ret, IRQ_HANDLED);
	KUNIT_EXPECT_EQ(test, mock_do_amd_gpio_irq_handler_call_count, 1);
}

// Test case: amd_gpio_irq_handler returns IRQ_NONE when do_amd_gpio_irq_handler returns zero
static void test_amd_gpio_irq_handler_not_handled(struct kunit *test)
{
	mock_do_amd_gpio_irq_handler_result = 0;
	mock_do_amd_gpio_irq_handler_call_count = 0;

	irqreturn_t ret = amd_gpio_irq_handler(42, NULL);

	KUNIT_EXPECT_EQ(test, ret, IRQ_NONE);
	KUNIT_EXPECT_EQ(test, mock_do_amd_gpio_irq_handler_call_count, 1);
}

// Test case: amd_gpio_irq_handler handles negative irq values correctly
static void test_amd_gpio_irq_handler_negative_irq(struct kunit *test)
{
	mock_do_amd_gpio_irq_handler_result = 1;
	mock_do_amd_gpio_irq_handler_call_count = 0;

	irqreturn_t ret = amd_gpio_irq_handler(-1, NULL);

	KUNIT_EXPECT_EQ(test, ret, IRQ_HANDLED);
	KUNIT_EXPECT_EQ(test, mock_do_amd_gpio_irq_handler_call_count, 1);
}

// Test case: amd_gpio_irq_handler handles NULL dev_id correctly
static void test_amd_gpio_irq_handler_null_dev_id(struct kunit *test)
{
	mock_do_amd_gpio_irq_handler_result = 0;
	mock_do_amd_gpio_irq_handler_call_count = 0;

	irqreturn_t ret = amd_gpio_irq_handler(0, NULL);

	KUNIT_EXPECT_EQ(test, ret, IRQ_NONE);
	KUNIT_EXPECT_EQ(test, mock_do_amd_gpio_irq_handler_call_count, 1);
}

// Test case: amd_gpio_irq_handler handles maximum integer irq value
static void test_amd_gpio_irq_handler_max_irq(struct kunit *test)
{
	mock_do_amd_gpio_irq_handler_result = 1;
	mock_do_amd_gpio_irq_handler_call_count = 0;

	irqreturn_t ret = amd_gpio_irq_handler(INT_MAX, (void *)0xDEADBEEF);

	KUNIT_EXPECT_EQ(test, ret, IRQ_HANDLED);
	KUNIT_EXPECT_EQ(test, mock_do_amd_gpio_irq_handler_call_count, 1);
}

static struct kunit_case amd_gpio_irq_handler_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_handler_handled),
	KUNIT_CASE(test_amd_gpio_irq_handler_not_handled),
	KUNIT_CASE(test_amd_gpio_irq_handler_negative_irq),
	KUNIT_CASE(test_amd_gpio_irq_handler_null_dev_id),
	KUNIT_CASE(test_amd_gpio_irq_handler_max_irq),
	{}
};

static struct kunit_suite amd_gpio_irq_handler_test_suite = {
	.name = "amd_gpio_irq_handler_test",
	.test_cases = amd_gpio_irq_handler_test_cases,
};

kunit_test_suite(amd_gpio_irq_handler_test_suite);
