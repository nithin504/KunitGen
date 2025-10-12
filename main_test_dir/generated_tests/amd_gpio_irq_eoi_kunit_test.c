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
	struct gpio_chip chip;
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

static void test_amd_gpio_irq_eoi_sets_eoi_bit(struct kunit *test)
{
	struct amd_gpio *gpio_dev;
	struct gpio_chip *gc;
	struct irq_data irq_data;
	char __iomem *mmio_base;
	u32 reg_value;

	gpio_dev = kunit_kzalloc(test, sizeof(*gpio_dev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev);

	mmio_base = kunit_kzalloc(test, 4096, GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, mmio_base);

	gpio_dev->base = mmio_base;
	raw_spin_lock_init(&gpio_dev->lock);

	gc = &gpio_dev->chip;
	gc->private = gpio_dev;

	irq_data.chip_data = gc;

	writel(0x0, gpio_dev->base + WAKE_INT_MASTER_REG);

	amd_gpio_irq_eoi(&irq_data);

	reg_value = readl(gpio_dev->base + WAKE_INT_MASTER_REG);
	KUNIT_EXPECT_EQ(test, reg_value & EOI_MASK, EOI_MASK);
}

static void test_amd_gpio_irq_eoi_preserves_other_bits(struct kunit *test)
{
	struct amd_gpio *gpio_dev;
	struct gpio_chip *gc;
	struct irq_data irq_data;
	char __iomem *mmio_base;
	u32 reg_value, expected_value;

	gpio_dev = kunit_kzalloc(test, sizeof(*gpio_dev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev);

	mmio_base = kunit_kzalloc(test, 4096, GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, mmio_base);

	gpio_dev->base = mmio_base;
	raw_spin_lock_init(&gpio_dev->lock);

	gc = &gpio_dev->chip;
	gc->private = gpio_dev;

	irq_data.chip_data = gc;

	writel(0xDEADBEEF, gpio_dev->base + WAKE_INT_MASTER_REG);

	amd_gpio_irq_eoi(&irq_data);

	reg_value = readl(gpio_dev->base + WAKE_INT_MASTER_REG);
	expected_value = 0xDEADBEEF | EOI_MASK;
	KUNIT_EXPECT_EQ(test, reg_value, expected_value);
}

static struct kunit_case amd_gpio_irq_eoi_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_eoi_sets_eoi_bit),
	KUNIT_CASE(test_amd_gpio_irq_eoi_preserves_other_bits),
	{}
};

static struct kunit_suite amd_gpio_irq_eoi_test_suite = {
	.name = "amd_gpio_irq_eoi_test",
	.test_cases = amd_gpio_irq_eoi_test_cases,
};

kunit_test_suite(amd_gpio_irq_eoi_test_suite);
