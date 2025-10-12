// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/irq.h>
#include <linux/gpio/driver.h>
#include <linux/io.h>
#include <linux/spinlock.h>

#define INTERRUPT_ENABLE_OFF 16
#define INTERRUPT_MASK_OFF 17

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
};

static void amd_gpio_irq_disable(struct irq_data *d)
{
	u32 pin_reg;
	unsigned long flags;
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);
	irq_hw_number_t hwirq = irqd_to_hwirq(d);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + hwirq * 4);
	pin_reg &= ~BIT(INTERRUPT_ENABLE_OFF);
	pin_reg &= ~BIT(INTERRUPT_MASK_OFF);
	writel(pin_reg, gpio_dev->base + hwirq * 4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);

	gpiochip_disable_irq(gc, hwirq);
}

static char mmio_buffer[4096];
static struct amd_gpio mock_gpio_dev;
static struct gpio_chip mock_gc;
static struct irq_data mock_irq_data;

static void setup_mock_irq_data(struct kunit *test, irq_hw_number_t hwirq)
{
	mock_irq_data.hwirq = hwirq;
	mock_irq_data.chip_data = &mock_gc;
}

static void test_amd_gpio_irq_disable_normal_operation(struct kunit *test)
{
	const irq_hw_number_t hwirq = 5;
	u32 initial_value = 0xFFFFFFFF;
	u32 expected_value;

	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gc.private = &mock_gpio_dev;

	setup_mock_irq_data(test, hwirq);

	writel(initial_value, mock_gpio_dev.base + hwirq * 4);

	amd_gpio_irq_disable(&mock_irq_data);

	expected_value = initial_value & ~BIT(INTERRUPT_ENABLE_OFF) & ~BIT(INTERRUPT_MASK_OFF);
	u32 result = readl(mock_gpio_dev.base + hwirq * 4);
	KUNIT_EXPECT_EQ(test, result, expected_value);
}

static void test_amd_gpio_irq_disable_zero_hwirq(struct kunit *test)
{
	const irq_hw_number_t hwirq = 0;
	u32 initial_value = 0x12345678;
	u32 expected_value;

	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gc.private = &mock_gpio_dev;

	setup_mock_irq_data(test, hwirq);

	writel(initial_value, mock_gpio_dev.base + hwirq * 4);

	amd_gpio_irq_disable(&mock_irq_data);

	expected_value = initial_value & ~BIT(INTERRUPT_ENABLE_OFF) & ~BIT(INTERRUPT_MASK_OFF);
	u32 result = readl(mock_gpio_dev.base + hwirq * 4);
	KUNIT_EXPECT_EQ(test, result, expected_value);
}

static void test_amd_gpio_irq_disable_clear_bits_only(struct kunit *test)
{
	const irq_hw_number_t hwirq = 10;
	u32 initial_value = BIT(INTERRUPT_ENABLE_OFF) | BIT(INTERRUPT_MASK_OFF);
	u32 expected_value = 0;

	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gc.private = &mock_gpio_dev;

	setup_mock_irq_data(test, hwirq);

	writel(initial_value, mock_gpio_dev.base + hwirq * 4);

	amd_gpio_irq_disable(&mock_irq_data);

	u32 result = readl(mock_gpio_dev.base + hwirq * 4);
	KUNIT_EXPECT_EQ(test, result, expected_value);
}

static void test_amd_gpio_irq_disable_other_bits_preserved(struct kunit *test)
{
	const irq_hw_number_t hwirq = 3;
	u32 other_bits = 0x12340000;
	u32 initial_value = other_bits | BIT(INTERRUPT_ENABLE_OFF) | BIT(INTERRUPT_MASK_OFF);
	u32 expected_value = other_bits;

	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gc.private = &mock_gpio_dev;

	setup_mock_irq_data(test, hwirq);

	writel(initial_value, mock_gpio_dev.base + hwirq * 4);

	amd_gpio_irq_disable(&mock_irq_data);

	u32 result = readl(mock_gpio_dev.base + hwirq * 4);
	KUNIT_EXPECT_EQ(test, result, expected_value);
}

static struct kunit_case amd_gpio_irq_disable_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_disable_normal_operation),
	KUNIT_CASE(test_amd_gpio_irq_disable_zero_hwirq),
	KUNIT_CASE(test_amd_gpio_irq_disable_clear_bits_only),
	KUNIT_CASE(test_amd_gpio_irq_disable_other_bits_preserved),
	{}
};

static struct kunit_suite amd_gpio_irq_disable_test_suite = {
	.name = "amd_gpio_irq_disable_test",
	.test_cases = amd_gpio_irq_disable_test_cases,
};

kunit_test_suite(amd_gpio_irq_disable_test_suite);
