
// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/interrupt.h>

static int do_amd_gpio_irq_handler_calls = 0;
static int do_amd_gpio_irq_handler_return_value = 0;

static int do_amd_gpio_irq_handler(int irq, void *dev_id)
{
	do_amd_gpio_irq_handler_calls++;
	return do_amd_gpio_irq_handler_return_value;
}

static void test_amd_gpio_irq_handler_success(struct kunit *test)
{
	do_amd_gpio_irq_handler_calls = 0;
	do_amd_gpio_irq_handler_return_value = IRQ_HANDLED;
	
	irqreturn_t result = amd_gpio_irq_handler(1, NULL);
	
	KUNIT_EXPECT_EQ(test, do_amd_gpio_irq_handler_calls, 1);
	KUNIT_EXPECT_EQ(test, result, IRQ_RETVAL(IRQ_HANDLED));
}

static void test_amd_gpio_irq_handler_none(struct kunit *test)
{
	do_amd_gpio_irq_handler_calls = 0;
	do_amd_gpio_irq_handler_return_value = IRQ_NONE;
	
	irqreturn_t result = amd_gpio_irq_handler(2, NULL);
	
	KUNIT_EXPECT_EQ(test, do_amd_gpio_irq_handler_calls, 1);
	KUNIT_EXPECT_EQ(test, result, IRQ_RETVAL(IRQ_NONE));
}

static void test_amd_gpio_irq_handler_wake(struct kunit *test)
{
	do_amd_gpio_irq_handler_calls = 0;
	do_amd_gpio_irq_handler_return_value = IRQ_WAKE_THREAD;
	
	irqreturn_t result = amd_gpio_irq_handler(3, NULL);
	
	KUNIT_EXPECT_EQ(test, do_amd_gpio_irq_handler_calls, 1);
	KUNIT_EXPECT_EQ(test, result, IRQ_RETVAL(IRQ_WAKE_THREAD));
}

static void test_amd_gpio_irq_handler_negative_irq(struct kunit *test)
{
	do_amd_gpio_irq_handler_calls = 0;
	do_amd_gpio_irq_handler_return_value = IRQ_HANDLED;
	
	irqreturn_t result = amd_gpio_irq_handler(-1, NULL);
	
	KUNIT_EXPECT_EQ(test, do_amd_gpio_irq_handler_calls, 1);
	KUNIT_EXPECT_EQ(test, result, IRQ_RETVAL(IRQ_HANDLED));
}

static void test_amd_gpio_irq_handler_zero_irq(struct kunit *test)
{
	do_amd_gpio_irq_handler_calls = 0;
	do_amd_gpio_irq_handler_return_value = IRQ_HANDLED;
	
	irqreturn_t result = amd_gpio_irq_handler(0, NULL);
	
	KUNIT_EXPECT_EQ(test, do_amd_gpio_irq_handler_calls, 1);
	KUNIT_EXPECT_EQ(test, result, IRQ_RETVAL(IRQ_HANDLED));
}

static void test_amd_gpio_irq_handler_null_dev_id(struct kunit *test)
{
	do_amd_gpio_irq_handler_calls = 0;
	do_amd_gpio_irq_handler_return_value = IRQ_HANDLED;
	
	irqreturn_t result = amd_gpio_irq_handler(4, NULL);
	
	KUNIT_EXPECT_EQ(test, do_amd_gpio_irq_handler_calls, 1);
	KUNIT_EXPECT_EQ(test, result, IRQ_RETVAL(IRQ_HANDLED));
}

static void test_amd_gpio_irq_handler_valid_dev_id(struct kunit *test)
{
	void *dev_id = (void *)0x1234;
	do_amd_gpio_irq_handler_calls = 0;
	do_amd_gpio_irq_handler_return_value = IRQ_HANDLED;
	
	irqreturn_t result = amd_gpio_irq_handler(5, dev_id);
	
	KUNIT_EXPECT_EQ(test, do_amd_gpio_irq_handler_calls, 1);
	KUNIT_EXPECT_EQ(test, result, IRQ_RETVAL(IRQ_HANDLED));
}

static struct kunit_case amd_gpio_irq_handler_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_handler_success),
	KUNIT_CASE(test_amd_gpio_irq_handler_none),
	KUNIT_CASE(test_amd_gpio_irq_handler_wake),
	KUNIT_CASE(test_amd_gpio_irq_handler_negative_irq),
	KUNIT_CASE(test_amd_gpio_irq_handler_zero_irq),
	KUNIT_CASE(test_amd_gpio_irq_handler_null_dev_id),
	KUNIT_CASE(test_amd_gpio_irq_handler_valid_dev_id),
	{}
};

static struct kunit_suite amd_gpio_irq_handler_test_suite = {
	.name = "amd_gpio_irq_handler_test",
	.test_cases = amd_gpio_irq_handler_test_cases,
};

kunit_test_suite(amd_gpio_irq_handler_test_suite);