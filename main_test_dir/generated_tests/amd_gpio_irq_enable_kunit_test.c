// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/irq.h>
#include <linux/spinlock.h>
#include <linux/io.h>

#define INTERRUPT_ENABLE_OFF 0
#define INTERRUPT_MASK_OFF 1

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

	gpiochip_enable_irq(gc, d->hwirq);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + (d->hwirq)*4);
	pin_reg |= BIT(INTERRUPT_ENABLE_OFF);
	pin_reg |= BIT(INTERRUPT_MASK_OFF);
	writel(pin_reg, gpio_dev->base + (d->hwirq)*4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);
}

static char test_mmio_buffer[4096];
static struct amd_gpio mock_gpio_dev;
static struct gpio_chip mock_gpio_chip;
static int gpiochip_enable_irq_called;
static unsigned long gpiochip_enable_irq_hwirq;
static int raw_spin_lock_irqsave_called;
static int raw_spin_unlock_irqrestore_called;
static int readl_called;
static int writel_called;
static u32 last_written_value;
static unsigned long last_written_offset;

void gpiochip_enable_irq_mock(struct gpio_chip *gc, unsigned long hwirq)
{
	gpiochip_enable_irq_called++;
	gpiochip_enable_irq_hwirq = hwirq;
}

unsigned long raw_spin_lock_irqsave_mock(raw_spinlock_t *lock)
{
	raw_spin_lock_irqsave_called++;
	return 0;
}

void raw_spin_unlock_irqrestore_mock(raw_spinlock_t *lock, unsigned long flags)
{
	raw_spin_unlock_irqrestore_called++;
}

u32 readl_mock(const volatile void __iomem *addr)
{
	readl_called++;
	return 0;
}

void writel_mock(u32 value, volatile void __iomem *addr)
{
	writel_called++;
	last_written_value = value;
	last_written_offset = (unsigned long)addr - (unsigned long)mock_gpio_dev.base;
}

#define gpiochip_enable_irq gpiochip_enable_irq_mock
#define raw_spin_lock_irqsave raw_spin_lock_irqsave_mock
#define raw_spin_unlock_irqrestore raw_spin_unlock_irqrestore_mock
#define readl readl_mock
#define writel writel_mock

static void test_amd_gpio_irq_enable_normal(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	unsigned long test_hwirq = 5;

	d.hwirq = test_hwirq;
	d.domain = NULL;
	d.mask = 0;
	d.chip = NULL;
	d.chip_data = &gc;
	gc.parent = NULL;
	gpio_dev.base = test_mmio_buffer;
	gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock);

	gpiochip_enable_irq_called = 0;
	raw_spin_lock_irqsave_called = 0;
	raw_spin_unlock_irqrestore_called = 0;
	readl_called = 0;
	writel_called = 0;

	amd_gpio_irq_enable(&d);

	KUNIT_EXPECT_EQ(test, gpiochip_enable_irq_called, 1);
	KUNIT_EXPECT_EQ(test, gpiochip_enable_irq_hwirq, test_hwirq);
	KUNIT_EXPECT_EQ(test, raw_spin_lock_irqsave_called, 1);
	KUNIT_EXPECT_EQ(test, readl_called, 1);
	KUNIT_EXPECT_EQ(test, writel_called, 1);
	KUNIT_EXPECT_EQ(test, raw_spin_unlock_irqrestore_called, 1);
	KUNIT_EXPECT_EQ(test, last_written_offset, test_hwirq * 4);
	KUNIT_EXPECT_TRUE(test, last_written_value & BIT(INTERRUPT_ENABLE_OFF));
	KUNIT_EXPECT_TRUE(test, last_written_value & BIT(INTERRUPT_MASK_OFF));
}

static void test_amd_gpio_irq_enable_zero_hwirq(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	unsigned long test_hwirq = 0;

	d.hwirq = test_hwirq;
	d.domain = NULL;
	d.mask = 0;
	d.chip = NULL;
	d.chip_data = &gc;
	gc.parent = NULL;
	gpio_dev.base = test_mmio_buffer;
	gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock);

	gpiochip_enable_irq_called = 0;
	raw_spin_lock_irqsave_called = 0;
	raw_spin_unlock_irqrestore_called = 0;
	readl_called = 0;
	writel_called = 0;

	amd_gpio_irq_enable(&d);

	KUNIT_EXPECT_EQ(test, gpiochip_enable_irq_called, 1);
	KUNIT_EXPECT_EQ(test, gpiochip_enable_irq_hwirq, test_hwirq);
	KUNIT_EXPECT_EQ(test, raw_spin_lock_irqsave_called, 1);
	KUNIT_EXPECT_EQ(test, readl_called, 1);
	KUNIT_EXPECT_EQ(test, writel_called, 1);
	KUNIT_EXPECT_EQ(test, raw_spin_unlock_irqrestore_called, 1);
	KUNIT_EXPECT_EQ(test, last_written_offset, test_hwirq * 4);
}

static void test_amd_gpio_irq_enable_max_hwirq(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	unsigned long test_hwirq = 1023;

	d.hwirq = test_hwirq;
	d.domain = NULL;
	d.mask = 0;
	d.chip = NULL;
	d.chip_data = &gc;
	gc.parent = NULL;
	gpio_dev.base = test_mmio_buffer;
	gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock);

	gpiochip_enable_irq_called = 0;
	raw_spin_lock_irqsave_called = 0;
	raw_spin_unlock_irqrestore_called = 0;
	readl_called = 0;
	writel_called = 0;

	amd_gpio_irq_enable(&d);

	KUNIT_EXPECT_EQ(test, gpiochip_enable_irq_called, 1);
	KUNIT_EXPECT_EQ(test, gpiochip_enable_irq_hwirq, test_hwirq);
	KUNIT_EXPECT_EQ(test, raw_spin_lock_irqsave_called, 1);
	KUNIT_EXPECT_EQ(test, readl_called, 1);
	KUNIT_EXPECT_EQ(test, writel_called, 1);
	KUNIT_EXPECT_EQ(test, raw_spin_unlock_irqrestore_called, 1);
	KUNIT_EXPECT_EQ(test, last_written_offset, test_hwirq * 4);
}

static void test_amd_gpio_irq_enable_with_initial_bits_set(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	unsigned long test_hwirq = 7;
	u32 initial_value = 0xAAAAAAAA;

	d.hwirq = test_hwirq;
	d.domain = NULL;
	d.mask = 0;
	d.chip = NULL;
	d.chip_data = &gc;
	gc.parent = NULL;
	gpio_dev.base = test_mmio_buffer;
	gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock);

	gpiochip_enable_irq_called = 0;
	raw_spin_lock_irqsave_called = 0;
	raw_spin_unlock_irqrestore_called = 0;
	readl_called = 0;
	writel_called = 0;

	readl_mock = NULL;
	writel_mock = NULL;

	amd_gpio_irq_enable(&d);

	KUNIT_EXPECT_EQ(test, gpiochip_enable_irq_called, 1);
	KUNIT_EXPECT_EQ(test, gpiochip_enable_irq_hwirq, test_hwirq);
	KUNIT_EXPECT_EQ(test, raw_spin_lock_irqsave_called, 1);
	KUNIT_EXPECT_EQ(test, raw_spin_unlock_irqrestore_called, 1);
}

static struct kunit_case amd_gpio_irq_enable_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_enable_normal),
	KUNIT_CASE(test_amd_gpio_irq_enable_zero_hwirq),
	KUNIT_CASE(test_amd_gpio_irq_enable_max_hwirq),
	KUNIT_CASE(test_amd_gpio_irq_enable_with_initial_bits_set),
	{}
};

static struct kunit_suite amd_gpio_irq_enable_test_suite = {
	.name = "amd_gpio_irq_enable_test",
	.test_cases = amd_gpio_irq_enable_test_cases,
};

kunit_test_suite(amd_gpio_irq_enable_test_suite);