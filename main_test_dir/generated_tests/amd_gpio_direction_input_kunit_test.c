// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/io.h>
#include <linux/spinlock.h>

#define OUTPUT_ENABLE_OFF 23
#define BIT(x) (1U << (x))

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
};

static struct amd_gpio *gpiochip_get_data(struct gpio_chip *gc)
{
	return gc->private;
}

static int amd_gpio_direction_input(struct gpio_chip *gc, unsigned offset)
{
	unsigned long flags;
	u32 pin_reg;
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + offset * 4);
	pin_reg &= ~BIT(OUTPUT_ENABLE_OFF);
	writel(pin_reg, gpio_dev->base + offset * 4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);

	return 0;
}

static void test_amd_gpio_direction_input_normal(struct kunit *test)
{
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	char mmio_buffer[4096];
	u32 initial_value = 0xFFFFFFFF;
	u32 expected_value;

	gpio_dev.base = mmio_buffer;
	raw_spin_lock_init(&gpio_dev.lock);
	gc.private = &gpio_dev;

	/* Initialize register with all bits set */
	writel(initial_value, gpio_dev.base + 4);

	/* Perform direction input operation on offset 1 */
	amd_gpio_direction_input(&gc, 1);

	/* Check that OUTPUT_ENABLE bit is cleared */
	expected_value = initial_value & ~BIT(OUTPUT_ENABLE_OFF);
	KUNIT_EXPECT_EQ(test, readl(gpio_dev.base + 4), expected_value);
}

static void test_amd_gpio_direction_input_zero_offset(struct kunit *test)
{
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	char mmio_buffer[4096];
	u32 initial_value = 0x12345678;
	u32 expected_value;

	gpio_dev.base = mmio_buffer;
	raw_spin_lock_init(&gpio_dev.lock);
	gc.private = &gpio_dev;

	/* Initialize register at offset 0 */
	writel(initial_value, gpio_dev.base + 0);

	/* Perform direction input operation on offset 0 */
	amd_gpio_direction_input(&gc, 0);

	/* Check that OUTPUT_ENABLE bit is cleared */
	expected_value = initial_value & ~BIT(OUTPUT_ENABLE_OFF);
	KUNIT_EXPECT_EQ(test, readl(gpio_dev.base + 0), expected_value);
}

static void test_amd_gpio_direction_input_output_bit_already_clear(struct kunit *test)
{
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	char mmio_buffer[4096];
	u32 initial_value = ~(BIT(OUTPUT_ENABLE_OFF)); /* OUTPUT_ENABLE already clear */
	u32 expected_value = initial_value;

	gpio_dev.base = mmio_buffer;
	raw_spin_lock_init(&gpio_dev.lock);
	gc.private = &gpio_dev;

	/* Initialize register */
	writel(initial_value, gpio_dev.base + 8);

	/* Perform direction input operation */
	amd_gpio_direction_input(&gc, 2);

	/* Value should remain unchanged */
	KUNIT_EXPECT_EQ(test, readl(gpio_dev.base + 8), expected_value);
}

static struct kunit_case amd_gpio_direction_input_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_direction_input_normal),
	KUNIT_CASE(test_amd_gpio_direction_input_zero_offset),
	KUNIT_CASE(test_amd_gpio_direction_input_output_bit_already_clear),
	{}
};

static struct kunit_suite amd_gpio_direction_input_test_suite = {
	.name = "amd_gpio_direction_input",
	.test_cases = amd_gpio_direction_input_test_cases,
};

kunit_test_suite(amd_gpio_direction_input_test_suite);
