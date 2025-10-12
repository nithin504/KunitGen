```c
// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/io.h>
#include <linux/spinlock.h>

#define OUTPUT_ENABLE_OFF 22
#define OUTPUT_VALUE_OFF 16

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

static void test_amd_gpio_direction_output_value_high(struct kunit *test)
{
	struct gpio_chip gc;
	unsigned offset = 1;
	int value = 1;
	u32 reg_val;

	mock_gpio_dev.base = mock_mmio_region;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(0x0, mock_gpio_dev.base + offset * 4);

	amd_gpio_direction_output(&gc, offset, value);

	reg_val = readl(mock_gpio_dev.base + offset * 4);
	KUNIT_EXPECT_TRUE(test, !!(reg_val & BIT(OUTPUT_ENABLE_OFF)));
	KUNIT_EXPECT_TRUE(test, !!(reg_val & BIT(OUTPUT_VALUE_OFF)));
}

static void test_amd_gpio_direction_output_value_low(struct kunit *test)
{
	struct gpio_chip gc;
	unsigned offset = 2;
	int value = 0;
	u32 reg_val;

	mock_gpio_dev.base = mock_mmio_region;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(0xFFFFFFFF, mock_gpio_dev.base + offset * 4);

	amd_gpio_direction_output(&gc, offset, value);

	reg_val = readl(mock_gpio_dev.base + offset * 4);
	KUNIT_EXPECT_TRUE(test, !!(reg_val & BIT(OUTPUT_ENABLE_OFF)));
	KUNIT_EXPECT_TRUE(test, !(reg_val & BIT(OUTPUT_VALUE_OFF)));
}

static void test_amd_gpio_direction_output_offset_zero(struct kunit *test)
{
	struct gpio_chip gc;
	unsigned offset = 0;
	int value = 1;
	u32 reg_val;

	mock_gpio_dev.base = mock_mmio_region;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(0x0, mock_gpio_dev.base + offset * 4);

	amd_gpio_direction_output(&gc, offset, value);

	reg_val = readl(mock_gpio_dev.base + offset * 4);
	KUNIT_EXPECT_TRUE(test, !!(reg_val & BIT(OUTPUT_ENABLE_OFF)));
	KUNIT_EXPECT_TRUE(test, !!(reg_val & BIT(OUTPUT_VALUE_OFF)));
}

static void test_amd_gpio_direction_output_large_offset(struct kunit *test)
{
	struct gpio_chip gc;
	unsigned offset = 100;
	int value = 0;
	u32 reg_val;

	mock_gpio_dev.base = mock_mmio_region;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(0xFFFFFFFF, mock_gpio_dev.base + offset * 4);

	amd_gpio_direction_output(&gc, offset, value);

	reg_val = readl(mock_gpio_dev.base + offset * 4);
	KUNIT_EXPECT_TRUE(test, !!(reg_val & BIT(OUTPUT_ENABLE_OFF)));
	KUNIT_EXPECT_TRUE(test, !(reg_val & BIT(OUTPUT_VALUE_OFF)));
}

static struct kunit_case amd_gpio_direction_output_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_direction_output_value_high),
	KUNIT_CASE(test_amd_gpio_direction_output_value_low),
	KUNIT_CASE(test_amd_gpio_direction_output_offset_zero),
	KUNIT_CASE(test_amd_gpio_direction_output_large_offset),
	{}
};

static struct kunit_suite amd_gpio_direction_output_test_suite = {
	.name = "amd_gpio_direction_output_test",
	.test_cases = amd_gpio_direction_output_test_cases,
};

kunit_test_suite(amd_gpio_direction_output_test_suite);
```