```c
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

static struct amd_gpio mock_gpio_dev;
static char mock_mmio_region[4096];

static void *__wrap_gpiochip_get_data(struct gpio_chip *gc)
{
	return &mock_gpio_dev;
}

#define gpiochip_get_data __wrap_gpiochip_get_data

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
	unsigned offset = 5;
	u32 initial_value = 0xFFFFFFFF;
	u32 expected_value;

	mock_gpio_dev.base = mock_mmio_region;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	writel(initial_value, mock_gpio_dev.base + offset * 4);

	int ret = amd_gpio_direction_input(&gc, offset);
	KUNIT_EXPECT_EQ(test, ret, 0);

	expected_value = initial_value & ~BIT(OUTPUT_ENABLE_OFF);
	u32 result = readl(mock_gpio_dev.base + offset * 4);
	KUNIT_EXPECT_EQ(test, result, expected_value);
}

static void test_amd_gpio_direction_input_zero_offset(struct kunit *test)
{
	struct gpio_chip gc;
	unsigned offset = 0;
	u32 initial_value = 0x12345678;
	u32 expected_value;

	mock_gpio_dev.base = mock_mmio_region;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	writel(initial_value, mock_gpio_dev.base + offset * 4);

	int ret = amd_gpio_direction_input(&gc, offset);
	KUNIT_EXPECT_EQ(test, ret, 0);

	expected_value = initial_value & ~BIT(OUTPUT_ENABLE_OFF);
	u32 result = readl(mock_gpio_dev.base + offset * 4);
	KUNIT_EXPECT_EQ(test, result, expected_value);
}

static void test_amd_gpio_direction_input_output_bit_already_clear(struct kunit *test)
{
	struct gpio_chip gc;
	unsigned offset = 3;
	u32 initial_value = 0x12345600;
	u32 expected_value = initial_value;

	mock_gpio_dev.base = mock_mmio_region;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	writel(initial_value, mock_gpio_dev.base + offset * 4);

	int ret = amd_gpio_direction_input(&gc, offset);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 result = readl(mock_gpio_dev.base + offset * 4);
	KUNIT_EXPECT_EQ(test, result, expected_value);
}

static struct kunit_case amd_gpio_direction_input_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_direction_input_normal),
	KUNIT_CASE(test_amd_gpio_direction_input_zero_offset),
	KUNIT_CASE(test_amd_gpio_direction_input_output_bit_already_clear),
	{}
};

static struct kunit_suite amd_gpio_direction_input_test_suite = {
	.name = "amd_gpio_direction_input_test",
	.test_cases = amd_gpio_direction_input_test_cases,
};

kunit_test_suite(amd_gpio_direction_input_test_suite);
```