// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/gpio/driver.h>
#include <linux/spinlock.h>

// Define required constants and structures
#define WAKE_CNTRL_OFF_S0I3 0
#define WAKE_CNTRL_OFF_S3   1

struct amd_gpio {
	void __iomem *base;
	spinlock_t lock;
	int irq;
	struct platform_device *pdev;
};

// Function under test
static int amd_gpio_irq_set_wake(struct irq_data *d, unsigned int on)
{
	u32 pin_reg;
	unsigned long flags;
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);
	u32 wake_mask = BIT(WAKE_CNTRL_OFF_S0I3) | BIT(WAKE_CNTRL_OFF_S3);
	irq_hw_number_t hwirq = irqd_to_hwirq(d);
	int err;

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + hwirq * 4);

	if (on)
		pin_reg |= wake_mask;
	else
		pin_reg &= ~wake_mask;

	writel(pin_reg, gpio_dev->base + hwirq * 4);
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

// Mock data
static struct amd_gpio mock_gpio_dev;
static char mock_mmio_buffer[4096];

// Mock implementations
static int mock_enable_irq_wake_result = 0;
static int mock_disable_irq_wake_result = 0;

static int mock_enable_irq_wake(int irq)
{
	return mock_enable_irq_wake_result;
}

static int mock_disable_irq_wake(int irq)
{
	return mock_disable_irq_wake_result;
}

#define enable_irq_wake mock_enable_irq_wake
#define disable_irq_wake mock_disable_irq_wake

// Helper to create fake irq_data
static struct irq_data *create_mock_irq_data(unsigned long hwirq)
{
	static struct irq_data d;
	d.hwirq = hwirq;
	return &d;
}

// Test cases
static void test_amd_gpio_irq_set_wake_on_success(struct kunit *test)
{
	struct irq_data *d = create_mock_irq_data(1);
	mock_gpio_dev.base = mock_mmio_buffer;
	mock_gpio_dev.irq = 42;
	mock_gpio_dev.pdev = kunit_kzalloc(test, sizeof(*mock_gpio_dev.pdev), GFP_KERNEL);
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_enable_irq_wake_result = 0;

	// Initialize register content
	writel(0x0, mock_gpio_dev.base + 4);

	// Setup gpio_chip and link to irq_data
	static struct gpio_chip gc;
	gc.private = &mock_gpio_dev;
	d.chip = &gc;

	int ret = amd_gpio_irq_set_wake(d, 1);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(mock_gpio_dev.base + 4);
	KUNIT_EXPECT_NE(test, reg_val & (BIT(WAKE_CNTRL_OFF_S0I3) | BIT(WAKE_CNTRL_OFF_S3)), 0U);
}

static void test_amd_gpio_irq_set_wake_off_success(struct kunit *test)
{
	struct irq_data *d = create_mock_irq_data(2);
	mock_gpio_dev.base = mock_mmio_buffer;
	mock_gpio_dev.irq = 42;
	mock_gpio_dev.pdev = kunit_kzalloc(test, sizeof(*mock_gpio_dev.pdev), GFP_KERNEL);
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_disable_irq_wake_result = 0;

	// Pre-set register with wake bits enabled
	writel(BIT(WAKE_CNTRL_OFF_S0I3) | BIT(WAKE_CNTRL_OFF_S3), mock_gpio_dev.base + 8);

	// Setup gpio_chip and link to irq_data
	static struct gpio_chip gc;
	gc.private = &mock_gpio_dev;
	d.chip = &gc;

	int ret = amd_gpio_irq_set_wake(d, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(mock_gpio_dev.base + 8);
	KUNIT_EXPECT_EQ(test, reg_val & (BIT(WAKE_CNTRL_OFF_S0I3) | BIT(WAKE_CNTRL_OFF_S3)), 0U);
}

static void test_amd_gpio_irq_set_wake_enable_irq_wake_error(struct kunit *test)
{
	struct irq_data *d = create_mock_irq_data(3);
	mock_gpio_dev.base = mock_mmio_buffer;
	mock_gpio_dev.irq = 42;
	mock_gpio_dev.pdev = kunit_kzalloc(test, sizeof(*mock_gpio_dev.pdev), GFP_KERNEL);
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_enable_irq_wake_result = -EINVAL;

	// Initialize register content
	writel(0x0, mock_gpio_dev.base + 12);

	// Setup gpio_chip and link to irq_data
	static struct gpio_chip gc;
	gc.private = &mock_gpio_dev;
	d.chip = &gc;

	int ret = amd_gpio_irq_set_wake(d, 1);
	KUNIT_EXPECT_EQ(test, ret, 0); // Should still return 0 despite internal error
}

static void test_amd_gpio_irq_set_wake_disable_irq_wake_error(struct kunit *test)
{
	struct irq_data *d = create_mock_irq_data(4);
	mock_gpio_dev.base = mock_mmio_buffer;
	mock_gpio_dev.irq = 42;
	mock_gpio_dev.pdev = kunit_kzalloc(test, sizeof(*mock_gpio_dev.pdev), GFP_KERNEL);
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_disable_irq_wake_result = -EINVAL;

	// Pre-set register with wake bits enabled
	writel(BIT(WAKE_CNTRL_OFF_S0I3) | BIT(WAKE_CNTRL_OFF_S3), mock_gpio_dev.base + 16);

	// Setup gpio_chip and link to irq_data
	static struct gpio_chip gc;
	gc.private = &mock_gpio_dev;
	d.chip = &gc;

	int ret = amd_gpio_irq_set_wake(d, 0);
	KUNIT_EXPECT_EQ(test, ret, 0); // Should still return 0 despite internal error
}

static void test_amd_gpio_irq_set_wake_null_pdata(struct kunit *test)
{
	struct irq_data *d = create_mock_irq_data(5);
	mock_gpio_dev.base = mock_mmio_buffer;
	mock_gpio_dev.irq = 42;
	mock_gpio_dev.pdev = NULL; // Null device
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_enable_irq_wake_result = -ENOMEM;

	// Initialize register content
	writel(0x0, mock_gpio_dev.base + 20);

	// Setup gpio_chip and link to irq_data
	static struct gpio_chip gc;
	gc.private = &mock_gpio_dev;
	d.chip = &gc;

	int ret = amd_gpio_irq_set_wake(d, 1);
	KUNIT_EXPECT_EQ(test, ret, 0); // Should still return 0 even with null pdev
}

static struct kunit_case amd_gpio_irq_set_wake_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_set_wake_on_success),
	KUNIT_CASE(test_amd_gpio_irq_set_wake_off_success),
	KUNIT_CASE(test_amd_gpio_irq_set_wake_enable_irq_wake_error),
	KUNIT_CASE(test_amd_gpio_irq_set_wake_disable_irq_wake_error),
	KUNIT_CASE(test_amd_gpio_irq_set_wake_null_pdata),
	{}
};

static struct kunit_suite amd_gpio_irq_set_wake_test_suite = {
	.name = "amd_gpio_irq_set_wake_test",
	.test_cases = amd_gpio_irq_set_wake_test_cases,
};

kunit_test_suite(amd_gpio_irq_set_wake_test_suite);
