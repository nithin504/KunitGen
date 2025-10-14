// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/gpio/driver.h>
#include <linux/spinlock.h>

#define INTERRUPT_MASK_OFF 18

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
};

static void amd_gpio_irq_unmask(struct irq_data *d)
{
	u32 pin_reg;
	unsigned long flags;
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);
	irq_hw_number_t hwirq = irqd_to_hwirq(d);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + hwirq * 4);
	pin_reg |= BIT(INTERRUPT_MASK_OFF);
	writel(pin_reg, gpio_dev->base + hwirq * 4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);
}

static void test_amd_gpio_irq_unmask_normal(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	char mmio_buffer[4096] __aligned(4);
	u32 initial_value = 0x12345678;
	irq_hw_number_t hwirq = 5;

	gpio_dev.base = mmio_buffer;
	raw_spin_lock_init(&gpio_dev.lock);

	d.hwirq = hwirq;
	gc.private = &gpio_dev;
	d.chip_data = &gc;

	/* Initialize register content */
	writel(initial_value, gpio_dev.base + hwirq * 4);

	amd_gpio_irq_unmask(&d);

	u32 result = readl(gpio_dev.base + hwirq * 4);
	KUNIT_EXPECT_EQ(test, result, initial_value | BIT(INTERRUPT_MASK_OFF));
}

static void test_amd_gpio_irq_unmask_zero_offset(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	char mmio_buffer[4096] __aligned(4);
	u32 initial_value = 0xABCDEF00;
	irq_hw_number_t hwirq = 0;

	gpio_dev.base = mmio_buffer;
	raw_spin_lock_init(&gpio_dev.lock);

	d.hwirq = hwirq;
	gc.private = &gpio_dev;
	d.chip_data = &gc;

	/* Initialize register content at offset 0 */
	writel(initial_value, gpio_dev.base);

	amd_gpio_irq_unmask(&d);

	u32 result = readl(gpio_dev.base);
	KUNIT_EXPECT_EQ(test, result, initial_value | BIT(INTERRUPT_MASK_OFF));
}

static void test_amd_gpio_irq_unmask_large_offset(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	char mmio_buffer[8192] __aligned(4);
	u32 initial_value = 0xF0F0F0F0;
	irq_hw_number_t hwirq = 255; /* Large but within bounds */

	gpio_dev.base = mmio_buffer;
	raw_spin_lock_init(&gpio_dev.lock);

	d.hwirq = hwirq;
	gc.private = &gpio_dev;
	d.chip_data = &gc;

	/* Initialize register content at large offset */
	writel(initial_value, gpio_dev.base + hwirq * 4);

	amd_gpio_irq_unmask(&d);

	u32 result = readl(gpio_dev.base + hwirq * 4);
	KUNIT_EXPECT_EQ(test, result, initial_value | BIT(INTERRUPT_MASK_OFF));
}

static void test_amd_gpio_irq_unmask_bit_already_set(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	char mmio_buffer[4096] __aligned(4);
	u32 initial_value = 0x12345678 | BIT(INTERRUPT_MASK_OFF);
	irq_hw_number_t hwirq = 10;

	gpio_dev.base = mmio_buffer;
	raw_spin_lock_init(&gpio_dev.lock);

	d.hwirq = hwirq;
	gc.private = &gpio_dev;
	d.chip_data = &gc;

	/* Initialize register with bit already set */
	writel(initial_value, gpio_dev.base + hwirq * 4);

	amd_gpio_irq_unmask(&d);

	u32 result = readl(gpio_dev.base + hwirq * 4);
	KUNIT_EXPECT_EQ(test, result, initial_value);
}

static struct kunit_case amd_gpio_irq_unmask_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_unmask_normal),
	KUNIT_CASE(test_amd_gpio_irq_unmask_zero_offset),
	KUNIT_CASE(test_amd_gpio_irq_unmask_large_offset),
	KUNIT_CASE(test_amd_gpio_irq_unmask_bit_already_set),
	{}
};

static struct kunit_suite amd_gpio_irq_unmask_test_suite = {
	.name = "amd_gpio_irq_unmask_test",
	.test_cases = amd_gpio_irq_unmask_test_cases,
};

kunit_test_suite(amd_gpio_irq_unmask_test_suite);
