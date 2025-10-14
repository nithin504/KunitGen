// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/spinlock.h>
#include <linux/io.h>

static struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
} mock_gpio_dev;

static char test_mmio_buffer[4096];

static int amd_gpio_set_value(struct gpio_chip *gc, unsigned int offset,
			      int value)
{
	u32 pin_reg;
	unsigned long flags;
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + offset * 4);
	if (value)
		pin_reg |= BIT(OUTPUT_VALUE_OFF);
	else
		pin_reg &= ~BIT(OUTPUT_VALUE_OFF);
	writel(pin_reg, gpio_dev->base + offset * 4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);

	return 0;
}

static void test_amd_gpio_set_value_high(struct kunit *test)
{
	struct gpio_chip gc;
	u32 initial_value = 0x0;
	unsigned int offset = 1;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	writel(initial_value, mock_gpio_dev.base + offset * 4);
	
	int ret = amd_gpio_set_value(&gc, offset, 1);
	
	u32 result = readl(mock_gpio_dev.base + offset * 4);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, result & BIT(OUTPUT_VALUE_OFF), BIT(OUTPUT_VALUE_OFF));
}

static void test_amd_gpio_set_value_low(struct kunit *test)
{
	struct gpio_chip gc;
	u32 initial_value = 0xFFFFFFFF;
	unsigned int offset = 2;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	writel(initial_value, mock_gpio_dev.base + offset * 4);
	
	int ret = amd_gpio_set_value(&gc, offset, 0);
	
	u32 result = readl(mock_gpio_dev.base + offset * 4);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, result & BIT(OUTPUT_VALUE_OFF), 0);
}

static void test_amd_gpio_set_value_zero_offset(struct kunit *test)
{
	struct gpio_chip gc;
	u32 initial_value = 0x0;
	unsigned int offset = 0;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	writel(initial_value, mock_gpio_dev.base + offset * 4);
	
	int ret = amd_gpio_set_value(&gc, offset, 1);
	
	u32 result = readl(mock_gpio_dev.base + offset * 4);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, result & BIT(OUTPUT_VALUE_OFF), BIT(OUTPUT_VALUE_OFF));
}

static void test_amd_gpio_set_value_max_offset(struct kunit *test)
{
	struct gpio_chip gc;
	u32 initial_value = 0x0;
	unsigned int offset = (sizeof(test_mmio_buffer) / 4) - 1;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	writel(initial_value, mock_gpio_dev.base + offset * 4);
	
	int ret = amd_gpio_set_value(&gc, offset, 1);
	
	u32 result = readl(mock_gpio_dev.base + offset * 4);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, result & BIT(OUTPUT_VALUE_OFF), BIT(OUTPUT_VALUE_OFF));
}

static void test_amd_gpio_set_value_already_set(struct kunit *test)
{
	struct gpio_chip gc;
	u32 initial_value = BIT(OUTPUT_VALUE_OFF);
	unsigned int offset = 3;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	writel(initial_value, mock_gpio_dev.base + offset * 4);
	
	int ret = amd_gpio_set_value(&gc, offset, 1);
	
	u32 result = readl(mock_gpio_dev.base + offset * 4);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, result & BIT(OUTPUT_VALUE_OFF), BIT(OUTPUT_VALUE_OFF));
}

static void test_amd_gpio_set_value_already_clear(struct kunit *test)
{
	struct gpio_chip gc;
	u32 initial_value = 0x0;
	unsigned int offset = 4;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	writel(initial_value, mock_gpio_dev.base + offset * 4);
	
	int ret = amd_gpio_set_value(&gc, offset, 0);
	
	u32 result = readl(mock_gpio_dev.base + offset * 4);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, result & BIT(OUTPUT_VALUE_OFF), 0);
}

static struct kunit_case amd_gpio_set_value_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_set_value_high),
	KUNIT_CASE(test_amd_gpio_set_value_low),
	KUNIT_CASE(test_amd_gpio_set_value_zero_offset),
	KUNIT_CASE(test_amd_gpio_set_value_max_offset),
	KUNIT_CASE(test_amd_gpio_set_value_already_set),
	KUNIT_CASE(test_amd_gpio_set_value_already_clear),
	{}
};

static struct kunit_suite amd_gpio_set_value_test_suite = {
	.name = "amd_gpio_set_value_test",
	.test_cases = amd_gpio_set_value_test_cases,
};

kunit_test_suite(amd_gpio_set_value_test_suite);