
// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/spinlock.h>
#include <linux/io.h>

#define PIN_STS_OFF 28

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
};

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

static char test_mmio_buffer[4096];
static struct amd_gpio mock_gpio_dev;
static struct gpio_chip mock_gc;

static void test_amd_gpio_get_value_pin_set(struct kunit *test)
{
	int ret;
	u32 test_val = BIT(PIN_STS_OFF);
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(test_val, mock_gpio_dev.base + 0 * 4);

	ret = amd_gpio_get_value(&mock_gc, 0);
	KUNIT_EXPECT_EQ(test, ret, 1);
}

static void test_amd_gpio_get_value_pin_clear(struct kunit *test)
{
	int ret;
	u32 test_val = 0;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(test_val, mock_gpio_dev.base + 0 * 4);

	ret = amd_gpio_get_value(&mock_gc, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_get_value_multiple_offsets(struct kunit *test)
{
	int ret;
	u32 test_val = BIT(PIN_STS_OFF);
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(test_val, mock_gpio_dev.base + 1 * 4);
	writel(0, mock_gpio_dev.base + 2 * 4);

	ret = amd_gpio_get_value(&mock_gc, 1);
	KUNIT_EXPECT_EQ(test, ret, 1);
	
	ret = amd_gpio_get_value(&mock_gc, 2);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_get_value_other_bits_set(struct kunit *test)
{
	int ret;
	u32 test_val = BIT(PIN_STS_OFF) | BIT(0) | BIT(15) | BIT(31);
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(test_val, mock_gpio_dev.base + 0 * 4);

	ret = amd_gpio_get_value(&mock_gc, 0);
	KUNIT_EXPECT_EQ(test, ret, 1);
}

static void test_amd_gpio_get_value_all_bits_clear(struct kunit *test)
{
	int ret;
	u32 test_val = 0;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(test_val, mock_gpio_dev.base + 0 * 4);

	ret = amd_gpio_get_value(&mock_gc, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_get_value_all_bits_set(struct kunit *test)
{
	int ret;
	u32 test_val = ~0;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(test_val, mock_gpio_dev.base + 0 * 4);

	ret = amd_gpio_get_value(&mock_gc, 0);
	KUNIT_EXPECT_EQ(test, ret, 1);
}

static void test_amd_gpio_get_value_max_offset(struct kunit *test)
{
	int ret;
	unsigned max_offset = (sizeof(test_mmio_buffer) / 4) - 1;
	u32 test_val = BIT(PIN_STS_OFF);
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(test_val, mock_gpio_dev.base + max_offset * 4);

	ret = amd_gpio_get_value(&mock_gc, max_offset);
	KUNIT_EXPECT_EQ(test, ret, 1);
}

static struct kunit_case amd_gpio_get_value_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_get_value_pin_set),
	KUNIT_CASE(test_amd_gpio_get_value_pin_clear),
	KUNIT_CASE(test_amd_gpio_get_value_multiple_offsets),
	KUNIT_CASE(test_amd_gpio_get_value_other_bits_set),
	KUNIT_CASE(test_amd_gpio_get_value_all_bits_clear),
	KUNIT_CASE(test_amd_gpio_get_value_all_bits_set),
	KUNIT_CASE(test_amd_gpio_get_value_max_offset),
	{}
};

static struct kunit_suite amd_gpio_get_value_test_suite = {
	.name = "amd_gpio_get_value_test",
	.test_cases = amd_gpio_get_value_test_cases,
};

kunit_test_suite(amd_gpio_get_value_test_suite);