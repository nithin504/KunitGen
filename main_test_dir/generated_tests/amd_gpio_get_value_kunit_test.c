// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/io.h>
#include <linux/spinlock.h>

#define PIN_STS_OFF 0

// Mocked AMD GPIO structure
struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
};

// Mocked gpiochip_get_data
static inline void *gpiochip_get_data(struct gpio_chip *gc)
{
	return gc->private;
}

// Mocked readl
static u32 mock_readl_value = 0;
static u32 __mock_readl(void __iomem *addr)
{
	return mock_readl_value;
}
#define readl __mock_readl

// Include the function under test
static int amd_gpio_get_value(struct gpio_chip *gc, unsigned offset)
{
	u32 pin_reg;
	unsigned long flags;
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + offset * 4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);

	return !!(pin_reg & BIT(PIN_STS_OFF));
}

// Test cases
static void test_amd_gpio_get_value_pin_low(struct kunit *test)
{
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	char mmio_buffer[4096];

	gc.private = &gpio_dev;
	gpio_dev.base = mmio_buffer;
	gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock);
	mock_readl_value = 0x0;

	int ret = amd_gpio_get_value(&gc, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_get_value_pin_high(struct kunit *test)
{
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	char mmio_buffer[4096];

	gc.private = &gpio_dev;
	gpio_dev.base = mmio_buffer;
	gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock);
	mock_readl_value = BIT(PIN_STS_OFF);

	int ret = amd_gpio_get_value(&gc, 0);
	KUNIT_EXPECT_EQ(test, ret, 1);
}

static void test_amd_gpio_get_value_offset_nonzero(struct kunit *test)
{
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	char mmio_buffer[4096];

	gc.private = &gpio_dev;
	gpio_dev.base = mmio_buffer;
	gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock);
	mock_readl_value = BIT(PIN_STS_OFF);

	int ret = amd_gpio_get_value(&gc, 5);
	KUNIT_EXPECT_EQ(test, ret, 1);
}

static void test_amd_gpio_get_value_bit_not_set_but_other_bits(struct kunit *test)
{
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	char mmio_buffer[4096];

	gc.private = &gpio_dev;
	gpio_dev.base = mmio_buffer;
	gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock);
	mock_readl_value = 0xFFFFFFFE;

	int ret = amd_gpio_get_value(&gc, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_get_value_bit_set_among_others(struct kunit *test)
{
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	char mmio_buffer[4096];

	gc.private = &gpio_dev;
	gpio_dev.base = mmio_buffer;
	gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock);
	mock_readl_value = 0xFFFFFFFF;

	int ret = amd_gpio_get_value(&gc, 0);
	KUNIT_EXPECT_EQ(test, ret, 1);
}

static struct kunit_case amd_gpio_get_value_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_get_value_pin_low),
	KUNIT_CASE(test_amd_gpio_get_value_pin_high),
	KUNIT_CASE(test_amd_gpio_get_value_offset_nonzero),
	KUNIT_CASE(test_amd_gpio_get_value_bit_not_set_but_other_bits),
	KUNIT_CASE(test_amd_gpio_get_value_bit_set_among_others),
	{}
};

static struct kunit_suite amd_gpio_get_value_test_suite = {
	.name = "amd_gpio_get_value_test",
	.test_cases = amd_gpio_get_value_test_cases,
};

kunit_test_suite(amd_gpio_get_value_test_suite);
