// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/gpio/driver.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/device.h>

#define WAKE_CNTRL_OFF_S0I3 10
#define WAKE_CNTRL_OFF_S3   11

struct amd_gpio {
	void __iomem *base;
	spinlock_t lock;
	int irq;
	struct platform_device *pdev;
};

static int mock_enable_irq_wake_ret = 0;
static int mock_disable_irq_wake_ret = 0;
static int enable_irq_wake_call_count = 0;
static int disable_irq_wake_call_count = 0;
static int dev_err_call_count = 0;

static char mmio_buffer[4096];

static inline struct gpio_chip *irq_data_get_irq_chip_data(struct irq_data *d)
{
	return (struct gpio_chip *)d->chip_data;
}

static const char *str_enable_disable(unsigned int on)
{
	return on ? "enable" : "disable";
}

static void pm_pr_dbg(const char *fmt, ...) {}

static int enable_irq_wake(unsigned int irq)
{
	enable_irq_wake_call_count++;
	return mock_enable_irq_wake_ret;
}

static int disable_irq_wake(unsigned int irq)
{
	disable_irq_wake_call_count++;
	return mock_disable_irq_wake_ret;
}

static void dev_err(const struct device *dev, const char *fmt, ...)
{
	dev_err_call_count++;
}

#include "pinctrl-amd.c"

static int test_amd_gpio_irq_set_wake_enable_success(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	unsigned long flags;
	u32 expected_reg_val;
	irq_hw_number_t hwirq = 5;

	gc.private = &gpio_dev;
	d.chip_data = &gc;
	d.hwirq = hwirq;

	gpio_dev.base = mmio_buffer;
	gpio_dev.irq = 42;
	gpio_dev.pdev = kunit_kzalloc(test, sizeof(*gpio_dev.pdev), GFP_KERNEL);
	spin_lock_init(&gpio_dev.lock);

	mock_enable_irq_wake_ret = 0;
	enable_irq_wake_call_count = 0;
	dev_err_call_count = 0;

	writel(0x0, gpio_dev.base + hwirq * 4);

	int ret = amd_gpio_irq_set_wake(&d, 1);

	expected_reg_val = BIT(WAKE_CNTRL_OFF_S0I3) | BIT(WAKE_CNTRL_OFF_S3);
	KUNIT_EXPECT_EQ(test, readl(gpio_dev.base + hwirq * 4), expected_reg_val);
	KUNIT_EXPECT_EQ(test, enable_irq_wake_call_count, 1);
	KUNIT_EXPECT_EQ(test, dev_err_call_count, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);

	return 0;
}

static int test_amd_gpio_irq_set_wake_disable_success(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	unsigned long flags;
	u32 reg_val, expected_reg_val;
	irq_hw_number_t hwirq = 7;

	gc.private = &gpio_dev;
	d.chip_data = &gc;
	d.hwirq = hwirq;

	gpio_dev.base = mmio_buffer;
	gpio_dev.irq = 44;
	gpio_dev.pdev = kunit_kzalloc(test, sizeof(*gpio_dev.pdev), GFP_KERNEL);
	spin_lock_init(&gpio_dev.lock);

	mock_disable_irq_wake_ret = 0;
	disable_irq_wake_call_count = 0;
	dev_err_call_count = 0;

	reg_val = BIT(WAKE_CNTRL_OFF_S0I3) | BIT(WAKE_CNTRL_OFF_S3);
	writel(reg_val, gpio_dev.base + hwirq * 4);

	int ret = amd_gpio_irq_set_wake(&d, 0);

	expected_reg_val = 0;
	KUNIT_EXPECT_EQ(test, readl(gpio_dev.base + hwirq * 4), expected_reg_val);
	KUNIT_EXPECT_EQ(test, disable_irq_wake_call_count, 1);
	KUNIT_EXPECT_EQ(test, dev_err_call_count, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);

	return 0;
}

static int test_amd_gpio_irq_set_wake_enable_fail(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	unsigned long flags;
	u32 expected_reg_val;
	irq_hw_number_t hwirq = 9;

	gc.private = &gpio_dev;
	d.chip_data = &gc;
	d.hwirq = hwirq;

	gpio_dev.base = mmio_buffer;
	gpio_dev.irq = 46;
	gpio_dev.pdev = kunit_kzalloc(test, sizeof(*gpio_dev.pdev), GFP_KERNEL);
	spin_lock_init(&gpio_dev.lock);

	mock_enable_irq_wake_ret = -EINVAL;
	enable_irq_wake_call_count = 0;
	dev_err_call_count = 0;

	writel(0x0, gpio_dev.base + hwirq * 4);

	int ret = amd_gpio_irq_set_wake(&d, 1);

	expected_reg_val = BIT(WAKE_CNTRL_OFF_S0I3) | BIT(WAKE_CNTRL_OFF_S3);
	KUNIT_EXPECT_EQ(test, readl(gpio_dev.base + hwirq * 4), expected_reg_val);
	KUNIT_EXPECT_EQ(test, enable_irq_wake_call_count, 1);
	KUNIT_EXPECT_EQ(test, dev_err_call_count, 1);
	KUNIT_EXPECT_EQ(test, ret, 0);

	return 0;
}

static int test_amd_gpio_irq_set_wake_disable_fail(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	unsigned long flags;
	u32 reg_val, expected_reg_val;
	irq_hw_number_t hwirq = 11;

	gc.private = &gpio_dev;
	d.chip_data = &gc;
	d.hwirq = hwirq;

	gpio_dev.base = mmio_buffer;
	gpio_dev.irq = 48;
	gpio_dev.pdev = kunit_kzalloc(test, sizeof(*gpio_dev.pdev), GFP_KERNEL);
	spin_lock_init(&gpio_dev.lock);

	mock_disable_irq_wake_ret = -EINVAL;
	disable_irq_wake_call_count = 0;
	dev_err_call_count = 0;

	reg_val = BIT(WAKE_CNTRL_OFF_S0I3) | BIT(WAKE_CNTRL_OFF_S3);
	writel(reg_val, gpio_dev.base + hwirq * 4);

	int ret = amd_gpio_irq_set_wake(&d, 0);

	expected_reg_val = 0;
	KUNIT_EXPECT_EQ(test, readl(gpio_dev.base + hwirq * 4), expected_reg_val);
	KUNIT_EXPECT_EQ(test, disable_irq_wake_call_count, 1);
	KUNIT_EXPECT_EQ(test, dev_err_call_count, 1);
	KUNIT_EXPECT_EQ(test, ret, 0);

	return 0;
}

static struct kunit_case amd_gpio_irq_set_wake_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_set_wake_enable_success),
	KUNIT_CASE(test_amd_gpio_irq_set_wake_disable_success),
	KUNIT_CASE(test_amd_gpio_irq_set_wake_enable_fail),
	KUNIT_CASE(test_amd_gpio_irq_set_wake_disable_fail),
	{}
};

static struct kunit_suite amd_gpio_irq_set_wake_test_suite = {
	.name = "amd_gpio_irq_set_wake_test",
	.test_cases = amd_gpio_irq_set_wake_test_cases,
};

kunit_test_suite(amd_gpio_irq_set_wake_test_suite);
