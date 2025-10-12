// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/gpio/driver.h>
#include <linux/spinlock.h>

#define INTERRUPT_MASK_OFF 20
#define BIT(x) (1U << (x))

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
	struct gpio_chip chip;
};

static struct amd_gpio mock_gpio_dev;
static char mock_mmio_buffer[4096];

static void amd_gpio_irq_mask(struct irq_data *d)
{
	u32 pin_reg;
	unsigned long flags;
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);
	irq_hw_number_t hwirq = irqd_to_hwirq(d);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + hwirq * 4);
	pin_reg &= ~BIT(INTERRUPT_MASK_OFF);
	writel(pin_reg, gpio_dev->base + hwirq * 4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);
}

static void test_amd_gpio_irq_mask_normal_operation(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	unsigned int hwirq = 5;
	u32 initial_value = 0xFFFFFFFF;
	u32 expected_value;

	mock_gpio_dev.base = mock_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	writel(initial_value, mock_gpio_dev.base + hwirq * 4);

	d.hwirq = hwirq;
	d.chip_data = &gc;
	gc.private = &mock_gpio_dev;

	amd_gpio_irq_mask(&d);

	expected_value = initial_value & ~BIT(INTERRUPT_MASK_OFF);
	u32 result = readl(mock_gpio_dev.base + hwirq * 4);
	KUNIT_EXPECT_EQ(test, result, expected_value);
}

static void test_amd_gpio_irq_mask_zero_hwirq(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	unsigned int hwirq = 0;
	u32 initial_value = 0xAAAA5555;
	u32 expected_value;

	mock_gpio_dev.base = mock_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	writel(initial_value, mock_gpio_dev.base + hwirq * 4);

	d.hwirq = hwirq;
	d.chip_data = &gc;
	gc.private = &mock_gpio_dev;

	amd_gpio_irq_mask(&d);

	expected_value = initial_value & ~BIT(INTERRUPT_MASK_OFF);
	u32 result = readl(mock_gpio_dev.base + hwirq * 4);
	KUNIT_EXPECT_EQ(test, result, expected_value);
}

static void test_amd_gpio_irq_mask_already_masked(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	unsigned int hwirq = 3;
	u32 initial_value = ~(BIT(INTERRUPT_MASK_OFF)); 
	u32 expected_value = initial_value; 

	mock_gpio_dev.base = mock_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	writel(initial_value, mock_gpio_dev.base + hwirq * 4);

	d.hwirq = hwirq;
	d.chip_data = &gc;
	gc.private = &mock_gpio_dev;

	amd_gpio_irq_mask(&d);

	u32 result = readl(mock_gpio_dev.base + hwirq * 4);
	KUNIT_EXPECT_EQ(test, result, expected_value);
}

static struct kunit_case amd_gpio_irq_mask_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_mask_normal_operation),
	KUNIT_CASE(test_amd_gpio_irq_mask_zero_hwirq),
	KUNIT_CASE(test_amd_gpio_irq_mask_already_masked),
	{}
};

static struct kunit_suite amd_gpio_irq_mask_test_suite = {
	.name = "amd_gpio_irq_mask_test",
	.test_cases = amd_gpio_irq_mask_test_cases,
};

kunit_test_suite(amd_gpio_irq_mask_test_suite);
