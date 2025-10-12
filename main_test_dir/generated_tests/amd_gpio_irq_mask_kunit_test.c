```c
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
};

static struct amd_gpio mock_gpio_dev;
static char mock_mmio_buffer[4096];

static struct gpio_chip mock_gc = {
	.parent = NULL,
};

static struct irq_data mock_irq_data;

static void *mock_irq_data_get_irq_chip_data(struct irq_data *d)
{
	return &mock_gc;
}

static struct amd_gpio *mock_gpiochip_get_data(struct gpio_chip *gc)
{
	return &mock_gpio_dev;
}

static irq_hw_number_t mock_irqd_to_hwirq(struct irq_data *d)
{
	return 0;
}

#define irq_data_get_irq_chip_data mock_irq_data_get_irq_chip_data
#define gpiochip_get_data mock_gpiochip_get_data
#define irqd_to_hwirq mock_irqd_to_hwirq

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

static void test_amd_gpio_irq_mask_normal(struct kunit *test)
{
	u32 *reg = (u32 *)(mock_mmio_buffer);
	*reg = BIT(INTERRUPT_MASK_OFF) | 0x123;

	mock_gpio_dev.base = mock_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	amd_gpio_irq_mask(&mock_irq_data);

	u32 val = readl(mock_gpio_dev.base);
	KUNIT_EXPECT_EQ(test, val, 0x123U);
}

static void test_amd_gpio_irq_mask_already_cleared(struct kunit *test)
{
	u32 *reg = (u32 *)(mock_mmio_buffer);
	*reg = 0x123;

	mock_gpio_dev.base = mock_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	amd_gpio_irq_mask(&mock_irq_data);

	u32 val = readl(mock_gpio_dev.base);
	KUNIT_EXPECT_EQ(test, val, 0x123U);
}

static void test_amd_gpio_irq_mask_other_bits_set(struct kunit *test)
{
	u32 *reg = (u32 *)(mock_mmio_buffer);
	*reg = BIT(INTERRUPT_MASK_OFF) | BIT(0) | BIT(5) | BIT(10);

	mock_gpio_dev.base = mock_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	amd_gpio_irq_mask(&mock_irq_data);

	u32 val = readl(mock_gpio_dev.base);
	KUNIT_EXPECT_EQ(test, val, (u32)(BIT(0) | BIT(5) | BIT(10)));
}

static struct kunit_case amd_gpio_irq_mask_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_mask_normal),
	KUNIT_CASE(test_amd_gpio_irq_mask_already_cleared),
	KUNIT_CASE(test_amd_gpio_irq_mask_other_bits_set),
	{}
};

static struct kunit_suite amd_gpio_irq_mask_test_suite = {
	.name = "amd_gpio_irq_mask_test",
	.test_cases = amd_gpio_irq_mask_test_cases,
};

kunit_test_suite(amd_gpio_irq_mask_test_suite);
```