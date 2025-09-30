
// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/io.h>

#define INTERRUPT_ENABLE_OFF 0
#define INTERRUPT_MASK_OFF 1

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

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + (d->hwirq)*4);
	pin_reg &= ~BIT(INTERRUPT_ENABLE_OFF);
	pin_reg &= ~BIT(INTERRUPT_MASK_OFF);
	writel(pin_reg, gpio_dev->base + (d->hwirq)*4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);

	gpiochip_disable_irq(gc, d->hwirq);
}

static char test_mmio_buffer[4096];
static struct amd_gpio mock_gpio_dev;
static int gpiochip_disable_irq_called;
static unsigned long gpiochip_disable_irq_hwirq;

void gpiochip_disable_irq_mock(struct gpio_chip *gc, unsigned int offset)
{
	gpiochip_disable_irq_called++;
	gpiochip_disable_irq_hwirq = offset;
}
#define gpiochip_disable_irq gpiochip_disable_irq_mock

static void test_amd_gpio_irq_disable_normal(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	unsigned long hwirq = 5;
	
	d.hwirq = hwirq;
	gpio_dev.base = test_mmio_buffer;
	gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock);
	
	writel(0xFFFFFFFF, gpio_dev.base + hwirq * 4);
	gpiochip_disable_irq_called = 0;
	gpiochip_disable_irq_hwirq = 0;
	
	amd_gpio_irq_disable(&d);
	
	u32 final_val = readl(gpio_dev.base + hwirq * 4);
	KUNIT_EXPECT_EQ(test, final_val & BIT(INTERRUPT_ENABLE_OFF), 0);
	KUNIT_EXPECT_EQ(test, final_val & BIT(INTERRUPT_MASK_OFF), 0);
	KUNIT_EXPECT_EQ(test, gpiochip_disable_irq_called, 1);
	KUNIT_EXPECT_EQ(test, gpiochip_disable_irq_hwirq, hwirq);
}

static void test_amd_gpio_irq_disable_already_disabled(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	unsigned long hwirq = 3;
	
	d.hwirq = hwirq;
	gpio_dev.base = test_mmio_buffer;
	gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock);
	
	writel(0x0, gpio_dev.base + hwirq * 4);
	gpiochip_disable_irq_called = 0;
	gpiochip_disable_irq_hwirq = 0;
	
	amd_gpio_irq_disable(&d);
	
	u32 final_val = readl(gpio_dev.base + hwirq * 4);
	KUNIT_EXPECT_EQ(test, final_val & BIT(INTERRUPT_ENABLE_OFF), 0);
	KUNIT_EXPECT_EQ(test, final_val & BIT(INTERRUPT_MASK_OFF), 0);
	KUNIT_EXPECT_EQ(test, gpiochip_disable_irq_called, 1);
	KUNIT_EXPECT_EQ(test, gpiochip_disable_irq_hwirq, hwirq);
}

static void test_amd_gpio_irq_disable_hwirq_zero(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	unsigned long hwirq = 0;
	
	d.hwirq = hwirq;
	gpio_dev.base = test_mmio_buffer;
	gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock);
	
	writel(0xFFFFFFFF, gpio_dev.base + hwirq * 4);
	gpiochip_disable_irq_called = 0;
	gpiochip_disable_irq_hwirq = 0;
	
	amd_gpio_irq_disable(&d);
	
	u32 final_val = readl(gpio_dev.base + hwirq * 4);
	KUNIT_EXPECT_EQ(test, final_val & BIT(INTERRUPT_ENABLE_OFF), 0);
	KUNIT_EXPECT_EQ(test, final_val & BIT(INTERRUPT_MASK_OFF), 0);
	KUNIT_EXPECT_EQ(test, gpiochip_disable_irq_called, 1);
	KUNIT_EXPECT_EQ(test, gpiochip_disable_irq_hwirq, hwirq);
}

static void test_amd_gpio_irq_disable_max_hwirq(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	unsigned long hwirq = (sizeof(test_mmio_buffer) / 4) - 1;
	
	d.hwirq = hwirq;
	gpio_dev.base = test_mmio_buffer;
	gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock);
	
	writel(0xFFFFFFFF, gpio_dev.base + hwirq * 4);
	gpiochip_disable_irq_called = 0;
	gpiochip_disable_irq_hwirq = 0;
	
	amd_gpio_irq_disable(&d);
	
	u32 final_val = readl(gpio_dev.base + hwirq * 4);
	KUNIT_EXPECT_EQ(test, final_val & BIT(INTERRUPT_ENABLE_OFF), 0);
	KUNIT_EXPECT_EQ(test, final_val & BIT(INTERRUPT_MASK_OFF), 0);
	KUNIT_EXPECT_EQ(test, gpiochip_disable_irq_called, 1);
	KUNIT_EXPECT_EQ(test, gpiochip_disable_irq_hwirq, hwirq);
}

static struct kunit_case amd_gpio_irq_disable_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_disable_normal),
	KUNIT_CASE(test_amd_gpio_irq_disable_already_disabled),
	KUNIT_CASE(test_amd_gpio_irq_disable_hwirq_zero),
	KUNIT_CASE(test_amd_gpio_irq_disable_max_hwirq),
	{}
};

static struct kunit_suite amd_gpio_irq_disable_test_suite = {
	.name = "amd_gpio_irq_disable_test",
	.test_cases = amd_gpio_irq_disable_test_cases,
};

kunit_test_suite(amd_gpio_irq_disable_test_suite);