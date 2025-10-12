```c
// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/gpio/driver.h>
#include <linux/spinlock.h>

#define INTERRUPT_MASK_OFF 25
#define BIT(x) (1U << (x))

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

static char mmio_buffer[4096];
static struct amd_gpio mock_gpio_dev;

static void test_amd_gpio_irq_unmask_normal(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	unsigned long hwirq = 2;
	u32 expected_value;

	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	gc.private = &mock_gpio_dev;
	d.chip_data = &gc;
	d.hwirq = hwirq;

	writel(0x0, mock_gpio_dev.base + hwirq * 4);

	amd_gpio_irq_unmask(&d);

	expected_value = BIT(INTERRUPT_MASK_OFF);
	KUNIT_EXPECT_EQ(test, readl(mock_gpio_dev.base + hwirq * 4), expected_value);
}

static void test_amd_gpio_irq_unmask_existing_bits(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	unsigned long hwirq = 1;
	u32 initial_value = 0x12345678;
	u32 expected_value;

	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	gc.private = &mock_gpio_dev;
	d.chip_data = &gc;
	d.hwirq = hwirq;

	writel(initial_value, mock_gpio_dev.base + hwirq * 4);

	amd_gpio_irq_unmask(&d);

	expected_value = initial_value | BIT(INTERRUPT_MASK_OFF);
	KUNIT_EXPECT_EQ(test, readl(mock_gpio_dev.base + hwirq * 4), expected_value);
}

static void test_amd_gpio_irq_unmask_zero_hwirq(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	unsigned long hwirq = 0;
	u32 expected_value;

	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	gc.private = &mock_gpio_dev;
	d.chip_data = &gc;
	d.hwirq = hwirq;

	writel(0x0, mock_gpio_dev.base + hwirq * 4);

	amd_gpio_irq_unmask(&d);

	expected_value = BIT(INTERRUPT_MASK_OFF);
	KUNIT_EXPECT_EQ(test, readl(mock_gpio_dev.base + hwirq * 4), expected_value);
}

static void test_amd_gpio_irq_unmask_large_hwirq(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	unsigned long hwirq = 100;
	u32 expected_value;

	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	gc.private = &mock_gpio_dev;
	d.chip_data = &gc;
	d.hwirq = hwirq;

	writel(0x0, mock_gpio_dev.base + hwirq * 4);

	amd_gpio_irq_unmask(&d);

	expected_value = BIT(INTERRUPT_MASK_OFF);
	KUNIT_EXPECT_EQ(test, readl(mock_gpio_dev.base + hwirq * 4), expected_value);
}

static struct kunit_case amd_gpio_irq_unmask_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_unmask_normal),
	KUNIT_CASE(test_amd_gpio_irq_unmask_existing_bits),
	KUNIT_CASE(test_amd_gpio_irq_unmask_zero_hwirq),
	KUNIT_CASE(test_amd_gpio_irq_unmask_large_hwirq),
	{}
};

static struct kunit_suite amd_gpio_irq_unmask_test_suite = {
	.name = "amd_gpio_irq_unmask_test",
	.test_cases = amd_gpio_irq_unmask_test_cases,
};

kunit_test_suite(amd_gpio_irq_unmask_test_suite);
```