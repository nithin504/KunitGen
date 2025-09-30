// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/spinlock.h>
#include <linux/io.h>

#define OUTPUT_ENABLE_OFF 11
#define GPIO_LINE_DIRECTION_OUT 1
#define GPIO_LINE_DIRECTION_IN 0

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
};

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

static char test_mmio_buffer[4096];
static struct amd_gpio mock_gpio_dev;
static struct gpio_chip mock_gpio_chip;

static void test_amd_gpio_get_direction_output(struct kunit *test)
{
	int ret;
	unsigned offset = 0;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	writel(BIT(OUTPUT_ENABLE_OFF), mock_gpio_dev.base + offset * 4);
	
	ret = amd_gpio_get_direction(&mock_gpio_chip, offset);
	KUNIT_EXPECT_EQ(test, ret, GPIO_LINE_DIRECTION_OUT);
}

static void test_amd_gpio_get_direction_input(struct kunit *test)
{
	int ret;
	unsigned offset = 1;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	writel(0x0, mock_gpio_dev.base + offset * 4);
	
	ret = amd_gpio_get_direction(&mock_gpio_chip, offset);
	KUNIT_EXPECT_EQ(test, ret, GPIO_LINE_DIRECTION_IN);
}

static void test_amd_gpio_get_direction_output_with_other_bits(struct kunit *test)
{
	int ret;
	unsigned offset = 2;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	writel(BIT(OUTPUT_ENABLE_OFF) | BIT(0) | BIT(5) | BIT(15), mock_gpio_dev.base + offset * 4);
	
	ret = amd_gpio_get_direction(&mock_gpio_chip, offset);
	KUNIT_EXPECT_EQ(test, ret, GPIO_LINE_DIRECTION_OUT);
}

static void test_amd_gpio_get_direction_input_with_other_bits(struct kunit *test)
{
	int ret;
	unsigned offset = 3;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	writel(BIT(0) | BIT(5) | BIT(15), mock_gpio_dev.base + offset * 4);
	
	ret = amd_gpio_get_direction(&mock_gpio_chip, offset);
	KUNIT_EXPECT_EQ(test, ret, GPIO_LINE_DIRECTION_IN);
}

static void test_amd_gpio_get_direction_max_offset(struct kunit *test)
{
	int ret;
	unsigned offset = (sizeof(test_mmio_buffer) / 4) - 1;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	writel(BIT(OUTPUT_ENABLE_OFF), mock_gpio_dev.base + offset * 4);
	
	ret = amd_gpio_get_direction(&mock_gpio_chip, offset);
	KUNIT_EXPECT_EQ(test, ret, GPIO_LINE_DIRECTION_OUT);
}

static void test_amd_gpio_get_direction_zero_offset(struct kunit *test)
{
	int ret;
	unsigned offset = 0;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	writel(0x0, mock_gpio_dev.base + offset * 4);
	
	ret = amd_gpio_get_direction(&mock_gpio_chip, offset);
	KUNIT_EXPECT_EQ(test, ret, GPIO_LINE_DIRECTION_IN);
}

static void test_amd_gpio_get_direction_all_ones(struct kunit *test)
{
	int ret;
	unsigned offset = 4;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	writel(0xFFFFFFFF, mock_gpio_dev.base + offset * 4);
	
	ret = amd_gpio_get_direction(&mock_gpio_chip, offset);
	KUNIT_EXPECT_EQ(test, ret, GPIO_LINE_DIRECTION_OUT);
}

static void test_amd_gpio_get_direction_all_zeros(struct kunit *test)
{
	int ret;
	unsigned offset = 5;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	writel(0x0, mock_gpio_dev.base + offset * 4);
	
	ret = amd_gpio_get_direction(&mock_gpio_chip, offset);
	KUNIT_EXPECT_EQ(test, ret, GPIO_LINE_DIRECTION_IN);
}

static struct kunit_case amd_gpio_get_direction_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_get_direction_output),
	KUNIT_CASE(test_amd_gpio_get_direction_input),
	KUNIT_CASE(test_amd_gpio_get_direction_output_with_other_bits),
	KUNIT_CASE(test_amd_gpio_get_direction_input_with_other_bits),
	KUNIT_CASE(test_amd_gpio_get_direction_max_offset),
	KUNIT_CASE(test_amd_gpio_get_direction_zero_offset),
	KUNIT_CASE(test_amd_gpio_get_direction_all_ones),
	KUNIT_CASE(test_amd_gpio_get_direction_all_zeros),
	{}
};

static struct kunit_suite amd_gpio_get_direction_test_suite = {
	.name = "amd_gpio_get_direction_test",
	.test_cases = amd_gpio_get_direction_test_cases,
};

kunit_test_suite(amd_gpio_get_direction_test_suite);