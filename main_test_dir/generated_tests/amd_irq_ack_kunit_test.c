```c
// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/irq.h>

static void amd_irq_ack(struct irq_data *d)
{
	/*
	 * based on HW design,there is no need to ack HW
	 * before handle current irq. But this routine is
	 * necessary for handle_edge_irq
	*/
}

static void test_amd_irq_ack_null_irq_data(struct kunit *test)
{
	amd_irq_ack(NULL);
	KUNIT_EXPECT_EQ(test, 0, 0);
}

static void test_amd_irq_ack_valid_irq_data(struct kunit *test)
{
	struct irq_data d;
	amd_irq_ack(&d);
	KUNIT_EXPECT_EQ(test, 0, 0);
}

static struct kunit_case amd_irq_ack_test_cases[] = {
	KUNIT_CASE(test_amd_irq_ack_null_irq_data),
	KUNIT_CASE(test_amd_irq_ack_valid_irq_data),
	{}
};

static struct kunit_suite amd_irq_ack_test_suite = {
	.name = "amd_irq_ack_test",
	.test_cases = amd_irq_ack_test_cases,
};

kunit_test_suite(amd_irq_ack_test_suite);
```