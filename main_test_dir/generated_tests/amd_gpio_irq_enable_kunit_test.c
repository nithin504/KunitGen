```c
// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/gpio/driver.h>
#include <linux/spinlock.h>

#define INTERRUPT_ENABLE_OFF 16
#define INTERRUPT_MASK_OFF 17

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
};

static void amd_gpio_irq_enable(struct irq_data *d)
{
	u32 pin_reg;
	unsigned long flags;
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);
	irq_hw_number_t hwirq = irqd_to_hwirq(d);

	gpiochip_enable_irq(gc, hwirq);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + hwirq * 4);
	pin_reg |= BIT(INTERRUPT_ENABLE_OFF);
	pin_reg |= BIT(INTERRUPT_MASK_OFF);
	writel(pin_reg, gpio_dev->base + hwirq * 4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);
}

static char mmio_buffer[4096];
static struct amd_gpio mock_gpio_dev;
static struct gpio_chip mock_gc;
static struct irq_data mock_irq_data;

static void test_amd_gpio_irq_enable_normal(struct kunit *test)
{
	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_irq_data.hwirq = 0;
	mock_irq_data.chip_data = &mock_gc;
	mock_gc.private = &mock_gpio_dev;

	writel(0x0, mock_gpio_dev.base);

	amd_gpio_irq_enable(&mock_irq_data);

	u32 reg_val = readl(mock_gpio_dev.base);
	KUNIT_EXPECT_TRUE(test, !!(reg_val & BIT(INTERRUPT_ENABLE_OFF)));
	KUNIT_EXPECT_TRUE(test, !!(reg_val & BIT(INTERRUPT_MASK_OFF)));
}

static void test_amd_gpio_irq_enable_nonzero_initial_value(struct kunit *test)
{
	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_irq_data.hwirq = 1;
	mock_irq_data.chip_data = &mock_gc;
	mock_gc.private = &mock_gpio_dev;

	writel(0xFFFFFFFF & ~(BIT(INTERRUPT_ENABLE_OFF) | BIT(INTERRUPT_MASK_OFF)), mock_gpio_dev.base + 4);

	amd_gpio_irq_enable(&mock_irq_data);

	u32 reg_val = readl(mock_gpio_dev.base + 4);
	KUNIT_EXPECT_TRUE(test, !!(reg_val & BIT(INTERRUPT_ENABLE_OFF)));
	KUNIT_EXPECT_TRUE(test, !!(reg_val & BIT(INTERRUPT_MASK_OFF)));
	KUNIT_EXPECT_TRUE(test, !(reg_val & ~((BIT(INTERRUPT_ENABLE_OFF) | BIT(INTERRUPT_MASK_OFF)) | 0xFFFFFFFC)));
}

static struct kunit_case amd_gpio_irq_enable_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_enable_normal),
	KUNIT_CASE(test_amd_gpio_irq_enable_nonzero_initial_value),
	{}
};

static struct kunit_suite amd_gpio_irq_enable_test_suite = {
	.name = "amd_gpio_irq_enable_test",
	.test_cases = amd_gpio_irq_enable_test_cases,
};

kunit_test_suite(amd_gpio_irq_enable_test_suite);
```