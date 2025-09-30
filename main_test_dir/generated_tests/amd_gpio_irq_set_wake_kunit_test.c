
// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/irq.h>
#include <linux/gpio/driver.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#define WAKE_CNTRL_OFF_S0I3 0
#define WAKE_CNTRL_OFF_S3 1

static char str_enable_disable(unsigned int on)
{
	return on ? "enable" : "disable";
}

struct amd_gpio {
	raw_spinlock_t lock;
	void __iomem *base;
	int irq;
	struct platform_device *pdev;
};

static int enable_irq_wake(unsigned int irq) { return 0; }
static int disable_irq_wake(unsigned int irq) { return 0; }

static int amd_gpio_irq_set_wake(struct irq_data *d, unsigned int on)
{
	u32 pin_reg;
	unsigned long flags;
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);
	u32 wake_mask = BIT(WAKE_CNTRL_OFF_S0I3) | BIT(WAKE_CNTRL_OFF_S3);
	int err;

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + (d->hwirq)*4);

	if (on)
		pin_reg |= wake_mask;
	else
		pin_reg &= ~wake_mask;

	writel(pin_reg, gpio_dev->base + (d->hwirq)*4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);

	if (on)
		err = enable_irq_wake(gpio_dev->irq);
	else
		err = disable_irq_wake(gpio_dev->irq);

	if (err)
		dev_err(&gpio_dev->pdev->dev, "failed to %s wake-up interrupt\n",
			str_enable_disable(on));

	return 0;
}

static char test_mmio_buffer[4096];
static struct amd_gpio test_gpio_dev;
static struct platform_device test_pdev;
static struct gpio_chip test_gc;

static void test_amd_gpio_irq_set_wake_enable(struct kunit *test)
{
	struct irq_data d;
	unsigned long hwirq = 0;
	int ret;
	
	test_gpio_dev.base = test_mmio_buffer;
	test_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(test_gpio_dev.lock);
	test_gpio_dev.irq = 1;
	test_gpio_dev.pdev = &test_pdev;
	d.hwirq = hwirq;
	d.domain = NULL;
	d.chip_data = &test_gc;

	ret = amd_gpio_irq_set_wake(&d, 1);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_irq_set_wake_disable(struct kunit *test)
{
	struct irq_data d;
	unsigned long hwirq = 1;
	int ret;
	
	test_gpio_dev.base = test_mmio_buffer;
	test_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(test_gpio_dev.lock);
	test_gpio_dev.irq = 1;
	test_gpio_dev.pdev = &test_pdev;
	d.hwirq = hwirq;
	d.domain = NULL;
	d.chip_data = &test_gc;

	ret = amd_gpio_irq_set_wake(&d, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_irq_set_wake_enable_error(struct kunit *test)
{
	struct irq_data d;
	unsigned long hwirq = 2;
	int ret;
	
	test_gpio_dev.base = test_mmio_buffer;
	test_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(test_gpio_dev.lock);
	test_gpio_dev.irq = 1;
	test_gpio_dev.pdev = &test_pdev;
	d.hwirq = hwirq;
	d.domain = NULL;
	d.chip_data = &test_gc;

	ret = amd_gpio_irq_set_wake(&d, 1);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_irq_set_wake_disable_error(struct kunit *test)
{
	struct irq_data d;
	unsigned long hwirq = 3;
	int ret;
	
	test_gpio_dev.base = test_mmio_buffer;
	test_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(test_gpio_dev.lock);
	test_gpio_dev.irq = 1;
	test_gpio_dev.pdev = &test_pdev;
	d.hwirq = hwirq;
	d.domain = NULL;
	d.chip_data = &test_gc;

	ret = amd_gpio_irq_set_wake(&d, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_irq_set_wake_max_hwirq(struct kunit *test)
{
	struct irq_data d;
	unsigned long hwirq = (sizeof(test_mmio_buffer) / 4) - 1;
	int ret;
	
	test_gpio_dev.base = test_mmio_buffer;
	test_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(test_gpio_dev.lock);
	test_gpio_dev.irq = 1;
	test_gpio_dev.pdev = &test_pdev;
	d.hwirq = hwirq;
	d.domain = NULL;
	d.chip_data = &test_gc;

	ret = amd_gpio_irq_set_wake(&d, 1);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static struct kunit_case amd_gpio_irq_set_wake_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_set_wake_enable),
	KUNIT_CASE(test_amd_gpio_irq_set_wake_disable),
	KUNIT_CASE(test_amd_gpio_irq_set_wake_enable_error),
	KUNIT_CASE(test_amd_gpio_irq_set_wake_disable_error),
	KUNIT_CASE(test_amd_gpio_irq_set_wake_max_hwirq),
	{}
};

static struct kunit_suite amd_gpio_irq_set_wake_test_suite = {
	.name = "amd_gpio_irq_set_wake_test",
	.test_cases = amd_gpio_irq_set_wake_test_cases,
};

kunit_test_suite(amd_gpio_irq_set_wake_test_suite);