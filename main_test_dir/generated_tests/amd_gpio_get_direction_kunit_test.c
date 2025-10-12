// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/io.h>
#include <linux/spinlock.h>

#define OUTPUT_ENABLE_OFF 23

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
};

static struct amd_gpio gpio_dev;
static char mmio_buffer[4096];

static struct amd_gpio *gpiochip_get_data(struct gpio_chip *gc)
{
	return &gpio_dev;
}

static u32 readl_mock(void __iomem *addr)
{
	return *((u32 *)addr);
}

#define readl(addr) readl_mock(addr)

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

static void test_amd_gpio_get_direction_output_enabled(struct kunit *test)
{
	struct gpio_chip gc;
	u32 *reg_addr = (u32 *)(mmio_buffer + 4);
	*reg_addr = BIT(OUTPUT_ENABLE_OFF);

	gpio_dev.base = mmio_buffer;
	gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock);

	int direction = amd_gpio_get_direction(&gc, 1);
	KUNIT_EXPECT_EQ(test, direction, GPIO_LINE_DIRECTION_OUT);
}

static void test_amd_gpio_get_direction_input_disabled(struct kunit *test)
{
	struct gpio_chip gc;
	u32 *reg_addr = (u32 *)(mmio_buffer + 8);
	*reg_addr = 0;

	gpio_dev.base = mmio_buffer;
	gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock);

	int direction = amd_gpio_get_direction(&gc, 2);
	KUNIT_EXPECT_EQ(test, direction, GPIO_LINE_DIRECTION_IN);
}

static void test_amd_gpio_get_direction_zero_offset(struct kunit *test)
{
	struct gpio_chip gc;
	u32 *reg_addr = (u32 *)mmio_buffer;
	*reg_addr = BIT(OUTPUT_ENABLE_OFF);

	gpio_dev.base = mmio_buffer;
	gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock);

	int direction = amd_gpio_get_direction(&gc, 0);
	KUNIT_EXPECT_EQ(test, direction, GPIO_LINE_DIRECTION_OUT);
}

static void test_amd_gpio_get_direction_unaligned_bit_clear(struct kunit *test)
{
	struct gpio_chip gc;
	u32 *reg_addr = (u32 *)(mmio_buffer + 12);
	*reg_addr = ~BIT(OUTPUT_ENABLE_OFF);

	gpio_dev.base = mmio_buffer;
	gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock);

	int direction = amd_gpio_get_direction(&gc, 3);
	KUNIT_EXPECT_EQ(test, direction, GPIO_LINE_DIRECTION_IN);
}

static void test_amd_gpio_get_direction_unaligned_bit_set(struct kunit *test)
{
	struct gpio_chip gc;
	u32 *reg_addr = (u32 *)(mmio_buffer + 16);
	*reg_addr = BIT(OUTPUT_ENABLE_OFF) | 0x12345678;

	gpio_dev.base = mmio_buffer;
	gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock);

	int direction = amd_gpio_get_direction(&gc, 4);
	KUNIT_EXPECT_EQ(test, direction, GPIO_LINE_DIRECTION_OUT);
}

static struct kunit_case amd_gpio_get_direction_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_get_direction_output_enabled),
	KUNIT_CASE(test_amd_gpio_get_direction_input_disabled),
	KUNIT_CASE(test_amd_gpio_get_direction_zero_offset),
	KUNIT_CASE(test_amd_gpio_get_direction_unaligned_bit_clear),
	KUNIT_CASE(test_amd_gpio_get_direction_unaligned_bit_set),
	{}
};

static struct kunit_suite amd_gpio_get_direction_test_suite = {
	.name = "amd_gpio_get_direction_test",
	.test_cases = amd_gpio_get_direction_test_cases,
};

kunit_test_suite(amd_gpio_get_direction_test_suite);
