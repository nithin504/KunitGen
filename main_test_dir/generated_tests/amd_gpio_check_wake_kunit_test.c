// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/io.h>

// Mock function declaration to simulate do_amd_gpio_irq_handler behavior
static bool do_amd_gpio_irq_handler(int irq, void *dev_id);

// Include the function under test
static bool __maybe_unused amd_gpio_check_wake(void *dev_id)
{
	return do_amd_gpio_irq_handler(-1, dev_id);
}

// Global variables to control/mock behavior
static bool mock_do_amd_gpio_irq_handler_return = false;
static int mock_do_amd_gpio_irq_handler_call_count = 0;
static void *last_dev_id_passed = NULL;

// Mock implementation of do_amd_gpio_irq_handler
static bool do_amd_gpio_irq_handler(int irq, void *dev_id)
{
	mock_do_amd_gpio_irq_handler_call_count++;
	last_dev_id_passed = dev_id;
	return mock_do_amd_gpio_irq_handler_return;
}

// Test case: amd_gpio_check_wake returns true when do_amd_gpio_irq_handler returns true
static void test_amd_gpio_check_wake_returns_true(struct kunit *test)
{
	mock_do_amd_gpio_irq_handler_return = true;
	mock_do_amd_gpio_irq_handler_call_count = 0;
	last_dev_id_passed = NULL;

	void *test_dev_id = (void *)0xDEADBEEF;
	bool result = amd_gpio_check_wake(test_dev_id);

	KUNIT_EXPECT_TRUE(test, result);
	KUNIT_EXPECT_EQ(test, mock_do_amd_gpio_irq_handler_call_count, 1);
	KUNIT_EXPECT_PTR_EQ(test, last_dev_id_passed, test_dev_id);
}

// Test case: amd_gpio_check_wake returns false when do_amd_gpio_irq_handler returns false
static void test_amd_gpio_check_wake_returns_false(struct kunit *test)
{
	mock_do_amd_gpio_irq_handler_return = false;
	mock_do_amd_gpio_irq_handler_call_count = 0;
	last_dev_id_passed = NULL;

	void *test_dev_id = (void *)0xCAFEBABE;
	bool result = amd_gpio_check_wake(test_dev_id);

	KUNIT_EXPECT_FALSE(test, result);
	KUNIT_EXPECT_EQ(test, mock_do_amd_gpio_irq_handler_call_count, 1);
	KUNIT_EXPECT_PTR_EQ(test, last_dev_id_passed, test_dev_id);
}

// Test case: amd_gpio_check_wake passes NULL dev_id correctly
static void test_amd_gpio_check_wake_null_dev_id(struct kunit *test)
{
	mock_do_amd_gpio_irq_handler_return = false;
	mock_do_amd_gpio_irq_handler_call_count = 0;
	last_dev_id_passed = NULL;

	bool result = amd_gpio_check_wake(NULL);

	KUNIT_EXPECT_FALSE(test, result);
	KUNIT_EXPECT_EQ(test, mock_do_amd_gpio_irq_handler_call_count, 1);
	KUNIT_EXPECT_NULL(test, last_dev_id_passed);
}

static struct kunit_case generated_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_check_wake_returns_true),
	KUNIT_CASE(test_amd_gpio_check_wake_returns_false),
	KUNIT_CASE(test_amd_gpio_check_wake_null_dev_id),
	{}
};

static struct kunit_suite generated_test_suite = {
	.name = "amd_gpio_check_wake-test",
	.test_cases = generated_test_cases,
};

kunit_test_suite(generated_test_suite);
