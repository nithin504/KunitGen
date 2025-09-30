
// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/irq.h>
#include <linux/gpio/driver.h>
#include <linux/spinlock.h>
#include <linux/io.h>

#define INTERRUPT_MASK_OFF 11

struct amd_gpio {
	raw_spinlock_t lock;
	void __iomem *base;
};

static void amd_gpio_irq_unmask(struct irq_data *d)
{
	u32 pin_reg;
	unsigned long flags;
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + (d->hwirq)*4);
	pin_reg |= BIT(INTERRUPT_MASK_OFF);
	writel(pin_reg, gpio_dev->base + (d->hwirq)*4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);
}

static char test_mmio_buffer[4096];
static struct amd_gpio mock_gpio_dev;
static struct gpio_chip mock_gpio_chip;
static struct irq_data mock_irq_data;

static void test_amd_gpio_irq_unmask_normal(struct kunit *test)
{
	unsigned long flags;
	u32 initial_val = 0;
	u32 expected_val;
	
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);
	
	writel(initial_val, mock_gpio_dev.base + (mock_irq_data.hwirq)*4);
	
	amd_gpio_irq_unmask(&mock_irq_data);
	
	expected_val = readl(mock_gpio_dev.base + (mock_irq_data.hwirq)*4);
	KUNIT_EXPECT_EQ(test, expected_val, BIT(INTERRUPT_MASK_OFF));
}

static void test_amd_gpio_irq_unmask_already_set(struct kunit *test)
{
	unsigned long flags;
	u32 initial_val = BIT(INTERRUPT_MASK_OFF);
	u32 expected_val;
	
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);
	
	writel(initial_val, mock_gpio_dev.base + (mock_irq_data.hwirq)*4);
	
	amd_gpio_irq_unmask(&mock_irq_data);
	
	expected_val = readl(mock_gpio_dev.base + (mock_irq_data.hwirq)*4);
	KUNIT_EXPECT_EQ(test, expected_val, BIT(INTERRUPT_MASK_OFF));
}

static void test_amd_gpio_irq_unmask_with_other_bits_set(struct kunit *test)
{
	unsigned long flags;
	u32 initial_val = 0x0000F00F;
	u32 expected_val;
	
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);
	
	writel(initial_val, mock_gpio_dev.base + (mock_irq_data.hwirq)*4);
	
	amd_gpio_irq_unmask(&mock_irq_data);
	
	expected_val = readl(mock_gpio_dev.base + (mock_irq_data.hwirq)*4);
	KUNIT_EXPECT_EQ(test, expected_val, initial_val | BIT(INTERRUPT_MASK_OFF));
}

static void test_amd_gpio_irq_unmask_zero_hwirq(struct kunit *test)
{
	unsigned long flags;
	u32 initial_val = 0;
	u32 expected_val;
	struct irq_data zero_irq_data = mock_irq_data;
	
	zero_irq_data.hwirq = 0;
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);
	
	writel(initial_val, mock_gpio_dev.base);
	
	amd_gpio_irq_unmask(&zero_irq_data);
	
	expected_val = readl(mock_gpio_dev.base);
	KUNIT_EXPECT_EQ(test, expected_val, BIT(INTERRUPT_MASK_OFF));
}

static void test_amd_gpio_irq_unmask_max_hwirq(struct kunit *test)
{
	unsigned long flags;
	u32 initial_val = 0;
	u32 expected_val;
	struct irq_data max_irq_data = mock_irq_data;
	
	max_irq_data.hwirq = (sizeof(test_mmio_buffer) / 4) - 1;
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);
	
	writel(initial_val, mock_gpio_dev.base + max_irq_data.hwirq * 4);
	
	amd_gpio_irq_unmask(&max_irq_data);
	
	expected_val = readl(mock_gpio_dev.base + max_irq_data.hwirq * 4);
	KUNIT_EXPECT_EQ(test, expected_val, BIT(INTERRUPT_MASK_OFF));
}

static struct kunit_case amd_gpio_irq_unmask_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_unmask_normal),
	KUNIT_CASE(test_amd_gpio_irq_unmask_already_set),
	KUNIT_CASE(test_amd_gpio_irq_unmask_with_other_bits_set),
	KUNIT_CASE(test_amd_gpio_irq_unmask_zero_hwirq),
	KUNIT_CASE(test_amd_gpio_irq_unmask_max_hwirq),
	{}
};

static struct kunit_suite amd_gpio_irq_unmask_test_suite = {
	.name = "amd_gpio_irq_unmask_test",
	.test_cases = amd_gpio_irq_unmask_test_cases,
};

kunit_test_suite(amd_gpio_irq_unmask_test_suite);