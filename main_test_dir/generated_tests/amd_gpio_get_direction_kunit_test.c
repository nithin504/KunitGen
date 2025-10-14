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

static int amd_gpio_get_direction(struct gpio_chip *gc, unsigned offset)
{
	unsigned long flags;
	u32 pin_reg;
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + offset * 4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);

	if (pin_reg & BIT(OUTPUT_ENABLE_OFF))
		return GPIO_LINE_DIRECTION_OUT;

	return GPIO_LINE_DIRECTION_IN;
}

static void test_amd_gpio_get_direction_output(struct kunit *test)
{
	struct gpio_chip gc;
	unsigned int offset = 0;
	u32 *reg;

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	reg = (u32 *)(test_mmio_buffer + offset * 4);
	*reg = BIT(OUTPUT_ENABLE_OFF);

	int ret = amd_gpio_get_direction(&gc, offset);
	KUNIT_EXPECT_EQ(test, ret, GPIO_LINE_DIRECTION_OUT);
}

static void test_amd_gpio_get_direction_input(struct kunit *test)
{
	struct gpio_chip gc;
	unsigned int offset = 1;
	u32 *reg;

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	reg = (u32 *)(test_mmio_buffer + offset * 4);
	*reg = 0;

	int ret = amd_gpio_get_direction(&gc, offset);
	KUNIT_EXPECT_EQ(test, ret, GPIO_LINE_DIRECTION_IN);
}

static void test_amd_gpio_get_direction_output_enable_clear(struct kunit *test)
{
	struct gpio_chip gc;
	unsigned int offset = 2;
	u32 *reg;

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	reg = (u32 *)(test_mmio_buffer + offset * 4);
	*reg = ~BIT(OUTPUT_ENABLE_OFF);

	int ret = amd_gpio_get_direction(&gc, offset);
	KUNIT_EXPECT_EQ(test, ret, GPIO_LINE_DIRECTION_IN);
}

static void test_amd_gpio_get_direction_multiple_bits_set(struct kunit *test)
{
	struct gpio_chip gc;
	unsigned int offset = 3;
	u32 *reg;

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	reg = (u32 *)(test_mmio_buffer + offset * 4);
	*reg = BIT(OUTPUT_ENABLE_OFF) | BIT(31) | BIT(15);

	int ret = amd_gpio_get_direction(&gc, offset);
	KUNIT_EXPECT_EQ(test, ret, GPIO_LINE_DIRECTION_OUT);
}

static void test_amd_gpio_get_direction_max_offset(struct kunit *test)
{
	struct gpio_chip gc;
	unsigned int offset = (sizeof(test_mmio_buffer) / 4) - 1;
	u32 *reg;

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	reg = (u32 *)(test_mmio_buffer + offset * 4);
	*reg = BIT(OUTPUT_ENABLE_OFF);

	int ret = amd_gpio_get_direction(&gc, offset);
	KUNIT_EXPECT_EQ(test, ret, GPIO_LINE_DIRECTION_OUT);
}

static struct kunit_case amd_gpio_get_direction_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_get_direction_output),
	KUNIT_CASE(test_amd_gpio_get_direction_input),
	KUNIT_CASE(test_amd_gpio_get_direction_output_enable_clear),
	KUNIT_CASE(test_amd_gpio_get_direction_multiple_bits_set),
	KUNIT_CASE(test_amd_gpio_get_direction_max_offset),
	{}
};

static struct kunit_suite amd_gpio_get_direction_test_suite = {
	.name = "amd_gpio_get_direction_test",
	.test_cases = amd_gpio_get_direction_test_cases,
};

kunit_test_suite(amd_gpio_get_direction_test_suite);