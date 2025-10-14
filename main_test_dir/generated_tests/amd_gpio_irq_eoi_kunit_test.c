// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/gpio/driver.h>
#include <linux/spinlock.h>

#define WAKE_INT_MASTER_REG 0x100
#define EOI_MASK 0x1

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
};

static void amd_gpio_irq_eoi(struct irq_data *d)
{
	u32 reg;
	unsigned long flags;
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	reg = readl(gpio_dev->base + WAKE_INT_MASTER_REG);
	reg |= EOI_MASK;
	writel(reg, gpio_dev->base + WAKE_INT_MASTER_REG);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);
}

static void *mock_irq_data_get_irq_chip_data(struct irq_data *d)
{
	return d->chip_data;
}

static void *mock_gpiochip_get_data(struct gpio_chip *gc)
{
	return gc->private;
}

#define irq_data_get_irq_chip_data mock_irq_data_get_irq_chip_data
#define gpiochip_get_data mock_gpiochip_get_data

static char mmio_buffer[4096];

static void test_amd_gpio_irq_eoi_normal(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	u32 *reg_addr = (u32 *)(mmio_buffer + WAKE_INT_MASTER_REG);

	gpio_dev.base = mmio_buffer;
	gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock);
	gc.private = &gpio_dev;
	d.chip_data = &gc;

	*reg_addr = 0x0;

	amd_gpio_irq_eoi(&d);

	KUNIT_EXPECT_EQ(test, *reg_addr & EOI_MASK, EOI_MASK);
}

static void test_amd_gpio_irq_eoi_preserve_other_bits(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	u32 *reg_addr = (u32 *)(mmio_buffer + WAKE_INT_MASTER_REG);

	gpio_dev.base = mmio_buffer;
	gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock);
	gc.private = &gpio_dev;
	d.chip_data = &gc;

	*reg_addr = 0xF0;

	amd_gpio_irq_eoi(&d);

	KUNIT_EXPECT_EQ(test, *reg_addr, 0xF1U);
}

static struct kunit_case amd_gpio_irq_eoi_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_eoi_normal),
	KUNIT_CASE(test_amd_gpio_irq_eoi_preserve_other_bits),
	{}
};

static struct kunit_suite amd_gpio_irq_eoi_test_suite = {
	.name = "amd_gpio_irq_eoi_test",
	.test_cases = amd_gpio_irq_eoi_test_cases,
};

kunit_test_suite(amd_gpio_irq_eoi_test_suite);
