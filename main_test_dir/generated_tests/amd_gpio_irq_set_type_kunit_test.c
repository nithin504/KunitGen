// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include "pinctrl-amd.c"

#define TEST_PIN_INDEX 0
#define MOCK_BASE_ADDR 0x1000

static char test_mmio_buffer[4096];
static struct amd_gpio mock_gpio_dev;
static struct gpio_chip mock_gc;
static struct irq_data mock_irq_data;

static void setup_mock_gpio_dev(void)
{
	memset(&mock_gpio_dev, 0, sizeof(mock_gpio_dev));
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(0x0, mock_gpio_dev.base + TEST_PIN_INDEX * 4);
}

static void setup_mock_irq_data(void)
{
	memset(&mock_irq_data, 0, sizeof(mock_irq_data));
	mock_irq_data.hwirq = TEST_PIN_INDEX;
}

static void test_amd_gpio_irq_set_type_edge_rising(struct kunit *test)
{
	int ret;
	setup_mock_gpio_dev();
	setup_mock_irq_data();

	mock_gc.private = &mock_gpio_dev;
	mock_irq_data.chip_data = &mock_gc;

	ret = amd_gpio_irq_set_type(&mock_irq_data, IRQ_TYPE_EDGE_RISING);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(mock_gpio_dev.base + TEST_PIN_INDEX * 4);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(LEVEL_TRIG_OFF), 0);
	KUNIT_EXPECT_EQ(test, (reg_val >> ACTIVE_LEVEL_OFF) & ACTIVE_LEVEL_MASK, ACTIVE_HIGH);
}

static void test_amd_gpio_irq_set_type_edge_falling(struct kunit *test)
{
	int ret;
	setup_mock_gpio_dev();
	setup_mock_irq_data();

	mock_gc.private = &mock_gpio_dev;
	mock_irq_data.chip_data = &mock_gc;

	ret = amd_gpio_irq_set_type(&mock_irq_data, IRQ_TYPE_EDGE_FALLING);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(mock_gpio_dev.base + TEST_PIN_INDEX * 4);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(LEVEL_TRIG_OFF), 0);
	KUNIT_EXPECT_EQ(test, (reg_val >> ACTIVE_LEVEL_OFF) & ACTIVE_LEVEL_MASK, ACTIVE_LOW);
}

static void test_amd_gpio_irq_set_type_edge_both(struct kunit *test)
{
	int ret;
	setup_mock_gpio_dev();
	setup_mock_irq_data();

	mock_gc.private = &mock_gpio_dev;
	mock_irq_data.chip_data = &mock_gc;

	ret = amd_gpio_irq_set_type(&mock_irq_data, IRQ_TYPE_EDGE_BOTH);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(mock_gpio_dev.base + TEST_PIN_INDEX * 4);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(LEVEL_TRIG_OFF), 0);
	KUNIT_EXPECT_EQ(test, (reg_val >> ACTIVE_LEVEL_OFF) & ACTIVE_LEVEL_MASK, BOTH_EDGES);
}

static void test_amd_gpio_irq_set_type_level_high(struct kunit *test)
{
	int ret;
	setup_mock_gpio_dev();
	setup_mock_irq_data();

	mock_gc.private = &mock_gpio_dev;
	mock_irq_data.chip_data = &mock_gc;

	ret = amd_gpio_irq_set_type(&mock_irq_data, IRQ_TYPE_LEVEL_HIGH);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(mock_gpio_dev.base + TEST_PIN_INDEX * 4);
	KUNIT_EXPECT_NE(test, reg_val & BIT(LEVEL_TRIG_OFF), 0);
	KUNIT_EXPECT_EQ(test, (reg_val >> ACTIVE_LEVEL_OFF) & ACTIVE_LEVEL_MASK, ACTIVE_HIGH);
}

static void test_amd_gpio_irq_set_type_level_low(struct kunit *test)
{
	int ret;
	setup_mock_gpio_dev();
	setup_mock_irq_data();

	mock_gc.private = &mock_gpio_dev;
	mock_irq_data.chip_data = &mock_gc;

	ret = amd_gpio_irq_set_type(&mock_irq_data, IRQ_TYPE_LEVEL_LOW);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(mock_gpio_dev.base + TEST_PIN_INDEX * 4);
	KUNIT_EXPECT_NE(test, reg_val & BIT(LEVEL_TRIG_OFF), 0);
	KUNIT_EXPECT_EQ(test, (reg_val >> ACTIVE_LEVEL_OFF) & ACTIVE_LEVEL_MASK, ACTIVE_LOW);
}

static void test_amd_gpio_irq_set_type_none(struct kunit *test)
{
	int ret;
	setup_mock_gpio_dev();
	setup_mock_irq_data();

	mock_gc.private = &mock_gpio_dev;
	mock_irq_data.chip_data = &mock_gc;

	ret = amd_gpio_irq_set_type(&mock_irq_data, IRQ_TYPE_NONE);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_irq_set_type_invalid(struct kunit *test)
{
	int ret;
	setup_mock_gpio_dev();
	setup_mock_irq_data();

	mock_gc.private = &mock_gpio_dev;
	mock_irq_data.chip_data = &mock_gc;

	ret = amd_gpio_irq_set_type(&mock_irq_data, 0xFF);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_amd_gpio_irq_set_type_clear_interrupt_status(struct kunit *test)
{
	int ret;
	setup_mock_gpio_dev();
	setup_mock_irq_data();

	mock_gc.private = &mock_gpio_dev;
	mock_irq_data.chip_data = &mock_gc;

	ret = amd_gpio_irq_set_type(&mock_irq_data, IRQ_TYPE_EDGE_RISING);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(mock_gpio_dev.base + TEST_PIN_INDEX * 4);
	KUNIT_EXPECT_EQ(test, (reg_val >> INTERRUPT_STS_OFF) & 0x1, CLR_INTR_STAT);
}

static void test_amd_gpio_irq_set_type_polling_enable_disable(struct kunit *test)
{
	int ret;
	setup_mock_gpio_dev();
	setup_mock_irq_data();

	mock_gc.private = &mock_gpio_dev;
	mock_irq_data.chip_data = &mock_gc;

	// Simulate hardware that immediately reflects the written value
	writel(0x0, mock_gpio_dev.base + TEST_PIN_INDEX * 4);

	ret = amd_gpio_irq_set_type(&mock_irq_data, IRQ_TYPE_EDGE_RISING);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(mock_gpio_dev.base + TEST_PIN_INDEX * 4);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(INTERRUPT_ENABLE_OFF), 0);
	KUNIT_EXPECT_NE(test, reg_val & BIT(INTERRUPT_MASK_OFF), 0);
}

static struct kunit_case amd_gpio_irq_set_type_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_set_type_edge_rising),
	KUNIT_CASE(test_amd_gpio_irq_set_type_edge_falling),
	KUNIT_CASE(test_amd_gpio_irq_set_type_edge_both),
	KUNIT_CASE(test_amd_gpio_irq_set_type_level_high),
	KUNIT_CASE(test_amd_gpio_irq_set_type_level_low),
	KUNIT_CASE(test_amd_gpio_irq_set_type_none),
	KUNIT_CASE(test_amd_gpio_irq_set_type_invalid),
	KUNIT_CASE(test_amd_gpio_irq_set_type_clear_interrupt_status),
	KUNIT_CASE(test_amd_gpio_irq_set_type_polling_enable_disable),
	{}
};

static struct kunit_suite amd_gpio_irq_set_type_test_suite = {
	.name = "amd_gpio_irq_set_type_test",
	.test_cases = amd_gpio_irq_set_type_test_cases,
};

kunit_test_suite(amd_gpio_irq_set_type_test_suite);
