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

// Include the function under test
static irqreturn_t amd_gpio_irq_handler(int irq, void *dev_id)
{
	return IRQ_RETVAL(do_amd_gpio_irq_handler(irq, dev_id));
}

// Test when do_amd_gpio_irq_handler returns 0 (IRQ_NONE)
static void test_amd_gpio_irq_handler_returns_none(struct kunit *test)
{
	mock_do_amd_gpio_irq_handler_result = 0;
	mock_do_amd_gpio_irq_handler_call_count = 0;

	irqreturn_t result = amd_gpio_irq_handler(10, NULL);

	KUNIT_EXPECT_EQ(test, result, IRQ_NONE);
	KUNIT_EXPECT_EQ(test, mock_do_amd_gpio_irq_handler_call_count, 1);
}

// Test when do_amd_gpio_irq_handler returns 1 (IRQ_HANDLED)
static void test_amd_gpio_irq_handler_returns_handled(struct kunit *test)
{
	mock_do_amd_gpio_irq_handler_result = 1;
	mock_do_amd_gpio_irq_handler_call_count = 0;

	irqreturn_t result = amd_gpio_irq_handler(20, (void *)0xDEADBEEF);

	KUNIT_EXPECT_EQ(test, result, IRQ_HANDLED);
	KUNIT_EXPECT_EQ(test, mock_do_amd_gpio_irq_handler_call_count, 1);
}

// Test with different irq values
static void test_amd_gpio_irq_handler_various_irq_values(struct kunit *test)
{
	int irqs[] = {0, 1, -1, INT_MAX, INT_MIN};
	for (int i = 0; i < ARRAY_SIZE(irqs); i++) {
		mock_do_amd_gpio_irq_handler_result = (i % 2); // Alternate between 0 and 1
		mock_do_amd_gpio_irq_handler_call_count = 0;

		irqreturn_t result = amd_gpio_irq_handler(irqs[i], (void *)(unsigned long)irqs[i]);

		KUNIT_EXPECT_EQ(test, result, (irqreturn_t)(mock_do_amd_gpio_irq_handler_result ? IRQ_HANDLED : IRQ_NONE));
		KUNIT_EXPECT_EQ(test, mock_do_amd_gpio_irq_handler_call_count, 1);
	}
}

// Test with various dev_id pointers
static void test_amd_gpio_irq_handler_various_dev_ids(struct kunit *test)
{
	void *dev_ids[] = {NULL, (void *)0x1, (void *)0xFFFFFFFF, (void *)0xDEADBEEF};
	
	for (int i = 0; i < ARRAY_SIZE(dev_ids); i++) {
		mock_do_amd_gpio_irq_handler_result = 1;
		mock_do_amd_gpio_irq_handler_call_count = 0;

		irqreturn_t result = amd_gpio_irq_handler(0, dev_ids[i]);

		KUNIT_EXPECT_EQ(test, result, IRQ_HANDLED);
		KUNIT_EXPECT_EQ(test, mock_do_amd_gpio_irq_handler_call_count, 1);
	}
}

static struct kunit_case amd_gpio_irq_handler_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_handler_returns_none),
	KUNIT_CASE(test_amd_gpio_irq_handler_returns_handled),
	KUNIT_CASE(test_amd_gpio_irq_handler_various_irq_values),
	KUNIT_CASE(test_amd_gpio_irq_handler_various_dev_ids),
	{}
};

static struct kunit_suite amd_gpio_irq_handler_test_suite = {
	.name = "amd_gpio_irq_handler_test",
	.test_cases = amd_gpio_irq_handler_test_cases,
};

kunit_test_suite(amd_gpio_irq_handler_test_suite);
