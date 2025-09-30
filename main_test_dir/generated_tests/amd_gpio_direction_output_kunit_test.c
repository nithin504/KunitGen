// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/spinlock.h>
#include <linux/io.h>

#define OUTPUT_ENABLE_OFF 11
#define OUTPUT_VALUE_OFF 9

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
};

static int amd_gpio_direction_output(struct gpio_chip *gc, unsigned offset, int value)
{
	u32 pin_reg;
	unsigned long flags;
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + offset * 4);
	pin_reg |= BIT(OUTPUT_ENABLE_OFF);
	if (value)
		pin_reg |= BIT(OUTPUT_VALUE_OFF);
	else
		pin_reg &= ~BIT(OUTPUT_VALUE_OFF);
	writel(pin_reg, gpio_dev->base + offset * 4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);

	return 0;
}

static char test_mmio_buffer[4096];
static struct amd_gpio test_gpio_dev;
static struct gpio_chip test_gc;

static void test_amd_gpio_direction_output_value_high(struct kunit *test)
{
	unsigned long flags;
	u32 initial_reg = 0;
	u32 expected_reg;
	
	test_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&test_gpio_dev.lock);
	writel(initial_reg, test_gpio_dev.base + 0 * 4);

	int ret = amd_gpio_direction_output(&test_gc, 0, 1);
	
	KUNIT_EXPECT_EQ(test, ret, 0);
	
	raw_spin_lock_irqsave(&test_gpio_dev.lock, flags);
	u32 result_reg = readl(test_gpio_dev.base + 0 * 4);
	raw_spin_unlock_irqrestore(&test_gpio_dev.lock, flags);
	
	expected_reg = initial_reg | BIT(OUTPUT_ENABLE_OFF) | BIT(OUTPUT_VALUE_OFF);
	KUNIT_EXPECT_EQ(test, result_reg, expected_reg);
}

static void test_amd_gpio_direction_output_value_low(struct kunit *test)
{
	unsigned long flags;
	u32 initial_reg = BIT(OUTPUT_VALUE_OFF);
	u32 expected_reg;
	
	test_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&test_gpio_dev.lock);
	writel(initial_reg, test_gpio_dev.base + 1 * 4);

	int ret = amd_gpio_direction_output(&test_gc, 1, 0);
	
	KUNIT_EXPECT_EQ(test, ret, 0);
	
	raw_spin_lock_irqsave(&test_gpio_dev.lock, flags);
	u32 result_reg = readl(test_gpio_dev.base + 1 * 4);
	raw_spin_unlock_irqrestore(&test_gpio_dev.lock, flags);
	
	expected_reg = (initial_reg | BIT(OUTPUT_ENABLE_OFF)) & ~BIT(OUTPUT_VALUE_OFF);
	KUNIT_EXPECT_EQ(test, result_reg, expected_reg);
}

static void test_amd_gpio_direction_output_already_configured(struct kunit *test)
{
	unsigned long flags;
	u32 initial_reg = BIT(OUTPUT_ENABLE_OFF) | BIT(OUTPUT_VALUE_OFF);
	
	test_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&test_gpio_dev.lock);
	writel(initial_reg, test_gpio_dev.base + 2 * 4);

	int ret = amd_gpio_direction_output(&test_gc, 2, 1);
	
	KUNIT_EXPECT_EQ(test, ret, 0);
	
	raw_spin_lock_irqsave(&test_gpio_dev.lock, flags);
	u32 result_reg = readl(test_gpio_dev.base + 2 * 4);
	raw_spin_unlock_irqrestore(&test_gpio_dev.lock, flags);
	
	KUNIT_EXPECT_EQ(test, result_reg, initial_reg);
}

static void test_amd_gpio_direction_output_different_offset(struct kunit *test)
{
	unsigned long flags;
	u32 initial_reg = 0;
	
	test_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&test_gpio_dev.lock);
	writel(initial_reg, test_gpio_dev.base + 10 * 4);

	int ret = amd_gpio_direction_output(&test_gc, 10, 1);
	
	KUNIT_EXPECT_EQ(test, ret, 0);
	
	raw_spin_lock_irqsave(&test_gpio_dev.lock, flags);
	u32 result_reg = readl(test_gpio_dev.base + 10 * 4);
	raw_spin_unlock_irqrestore(&test_gpio_dev.lock, flags);
	
	u32 expected_reg = BIT(OUTPUT_ENABLE_OFF) | BIT(OUTPUT_VALUE_OFF);
	KUNIT_EXPECT_EQ(test, result_reg, expected_reg);
}

static void test_amd_gpio_direction_output_max_offset(struct kunit *test)
{
	unsigned long flags;
	unsigned max_offset = (sizeof(test_mmio_buffer) / 4) - 1;
	u32 initial_reg = 0;
	
	test_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&test_gpio_dev.lock);
	writel(initial_reg, test_gpio_dev.base + max_offset * 4);

	int ret = amd_gpio_direction_output(&test_gc, max_offset, 0);
	
	KUNIT_EXPECT_EQ(test, ret, 0);
	
	raw_spin_lock_irqsave(&test_gpio_dev.lock, flags);
	u32 result_reg = readl(test_gpio_dev.base + max_offset * 4);
	raw_spin_unlock_irqrestore(&test_gpio_dev.lock, flags);
	
	u32 expected_reg = BIT(OUTPUT_ENABLE_OFF);
	KUNIT_EXPECT_EQ(test, result_reg, expected_reg);
}

static struct kunit_case amd_gpio_direction_output_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_direction_output_value_high),
	KUNIT_CASE(test_amd_gpio_direction_output_value_low),
	KUNIT_CASE(test_amd_gpio_direction_output_already_configured),
	KUNIT_CASE(test_amd_gpio_direction_output_different_offset),
	KUNIT_CASE(test_amd_gpio_direction_output_max_offset),
	{}
};

static struct kunit_suite amd_gpio_direction_output_test_suite = {
	.name = "amd_gpio_direction_output_test",
	.test_cases = amd_gpio_direction_output_test_cases,
};

kunit_test_suite(amd_gpio_direction_output_test_suite);