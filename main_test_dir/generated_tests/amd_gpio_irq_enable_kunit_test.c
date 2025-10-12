// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/spinlock.h>

#define PIN_COUNT 256
#define INTERRUPT_ENABLE_OFF 16
#define INTERRUPT_MASK_OFF 17

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
};

static char mock_mmio_region[PIN_COUNT * 4];

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

static void test_amd_gpio_irq_enable_normal(struct kunit *test)
{
	struct amd_gpio *gpio_dev;
	struct gpio_chip *gc;
	struct irq_data d;
	u32 reg_val;

	gpio_dev = kunit_kzalloc(test, sizeof(*gpio_dev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev);

	gpio_dev->base = mock_mmio_region;
	gpio_dev->lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev->lock);

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);
	gc->private = gpio_dev;

	d.hwirq = 5;
	d.chip_data = gc;

	memset(mock_mmio_region, 0, sizeof(mock_mmio_region));

	amd_gpio_irq_enable(&d);

	reg_val = readl(gpio_dev->base + d.hwirq * 4);
	KUNIT_EXPECT_TRUE(test, !!(reg_val & BIT(INTERRUPT_ENABLE_OFF)));
	KUNIT_EXPECT_TRUE(test, !!(reg_val & BIT(INTERRUPT_MASK_OFF)));
}

static void test_amd_gpio_irq_enable_first_pin(struct kunit *test)
{
	struct amd_gpio *gpio_dev;
	struct gpio_chip *gc;
	struct irq_data d;
	u32 reg_val;

	gpio_dev = kunit_kzalloc(test, sizeof(*gpio_dev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev);

	gpio_dev->base = mock_mmio_region;
	gpio_dev->lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev->lock);

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);
	gc->private = gpio_dev;

	d.hwirq = 0;
	d.chip_data = gc;

	memset(mock_mmio_region, 0, sizeof(mock_mmio_region));

	amd_gpio_irq_enable(&d);

	reg_val = readl(gpio_dev->base + d.hwirq * 4);
	KUNIT_EXPECT_TRUE(test, !!(reg_val & BIT(INTERRUPT_ENABLE_OFF)));
	KUNIT_EXPECT_TRUE(test, !!(reg_val & BIT(INTERRUPT_MASK_OFF)));
}

static void test_amd_gpio_irq_enable_last_pin(struct kunit *test)
{
	struct amd_gpio *gpio_dev;
	struct gpio_chip *gc;
	struct irq_data d;
	u32 reg_val;

	gpio_dev = kunit_kzalloc(test, sizeof(*gpio_dev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev);

	gpio_dev->base = mock_mmio_region;
	gpio_dev->lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev->lock);

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);
	gc->private = gpio_dev;

	d.hwirq = PIN_COUNT - 1;
	d.chip_data = gc;

	memset(mock_mmio_region, 0, sizeof(mock_mmio_region));

	amd_gpio_irq_enable(&d);

	reg_val = readl(gpio_dev->base + d.hwirq * 4);
	KUNIT_EXPECT_TRUE(test, !!(reg_val & BIT(INTERRUPT_ENABLE_OFF)));
	KUNIT_EXPECT_TRUE(test, !!(reg_val & BIT(INTERRUPT_MASK_OFF)));
}

static void test_amd_gpio_irq_enable_preserve_other_bits(struct kunit *test)
{
	struct amd_gpio *gpio_dev;
	struct gpio_chip *gc;
	struct irq_data d;
	u32 reg_val;

	gpio_dev = kunit_kzalloc(test, sizeof(*gpio_dev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev);

	gpio_dev->base = mock_mmio_region;
	gpio_dev->lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev->lock);

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);
	gc->private = gpio_dev;

	d.hwirq = 10;
	d.chip_data = gc;

	memset(mock_mmio_region, 0, sizeof(mock_mmio_region));
	writel(0xF0F0F0F0, gpio_dev->base + d.hwirq * 4);

	amd_gpio_irq_enable(&d);

	reg_val = readl(gpio_dev->base + d.hwirq * 4);
	KUNIT_EXPECT_TRUE(test, !!(reg_val & BIT(INTERRUPT_ENABLE_OFF)));
	KUNIT_EXPECT_TRUE(test, !!(reg_val & BIT(INTERRUPT_MASK_OFF)));
	KUNIT_EXPECT_NE(test, reg_val, 0xF0F0F0F0);
}

static struct kunit_case amd_gpio_irq_enable_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_enable_normal),
	KUNIT_CASE(test_amd_gpio_irq_enable_first_pin),
	KUNIT_CASE(test_amd_gpio_irq_enable_last_pin),
	KUNIT_CASE(test_amd_gpio_irq_enable_preserve_other_bits),
	{}
};

static struct kunit_suite amd_gpio_irq_enable_test_suite = {
	.name = "amd_gpio_irq_enable_test",
	.test_cases = amd_gpio_irq_enable_test_cases,
};

kunit_test_suite(amd_gpio_irq_enable_test_suite);
