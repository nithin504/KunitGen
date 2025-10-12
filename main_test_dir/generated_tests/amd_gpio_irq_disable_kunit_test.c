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

static char mock_mmio_region[4096];
static struct amd_gpio mock_gpio_dev;
static struct gpio_chip mock_gpio_chip;

static void test_amd_gpio_irq_disable_normal(struct kunit *test)
{
	struct irq_data d;
	u32 initial_value = 0xFFFFFFFF;
	u32 expected_value;
	irq_hw_number_t hwirq = 5;

	d.hwirq = hwirq;
	d.chip_data = &mock_gpio_chip;

	mock_gpio_dev.base = mock_mmio_region;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gpio_chip.private = &mock_gpio_dev;

	writel(initial_value, mock_gpio_dev.base + hwirq * 4);

	amd_gpio_irq_disable(&d);

	expected_value = initial_value & ~BIT(INTERRUPT_ENABLE_OFF) & ~BIT(INTERRUPT_MASK_OFF);
	KUNIT_EXPECT_EQ(test, readl(mock_gpio_dev.base + hwirq * 4), expected_value);
}

static struct kunit_case generated_kunit_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_disable_normal),
	{}
};

static struct kunit_suite generated_kunit_test_suite = {
	.name = "generated-kunit-test",
	.test_cases = generated_kunit_test_cases,
};

kunit_test_suite(generated_kunit_test_suite);
```