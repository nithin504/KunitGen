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
static char mock_mmio_buffer[4096];

static void *mock_gpiochip_get_data(struct gpio_chip *gc)
{
	return &mock_gpio_dev;
}

#define gpiochip_get_data mock_gpiochip_get_data
#include "pinctrl-amd.c"

static int test_amd_gpio_direction_output_value_high(struct kunit *test)
{
	struct gpio_chip gc;
	u32 reg_val;
	unsigned offset = 1;
	int value = 1;
	int ret;

	mock_gpio_dev.base = mock_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	writel(0x0, mock_gpio_dev.base + offset * 4);

	ret = amd_gpio_direction_output(&gc, offset, value);

	reg_val = readl(mock_gpio_dev.base + offset * 4);

	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_NE(test, reg_val & BIT(OUTPUT_ENABLE_OFF), 0U);
	KUNIT_EXPECT_NE(test, reg_val & BIT(OUTPUT_VALUE_OFF), 0U);

	return 0;
}

static int test_amd_gpio_direction_output_value_low(struct kunit *test)
{
	struct gpio_chip gc;
	u32 reg_val;
	unsigned offset = 2;
	int value = 0;
	int ret;

	mock_gpio_dev.base = mock_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	writel(0xFFFFFFFF, mock_gpio_dev.base + offset * 4);

	ret = amd_gpio_direction_output(&gc, offset, value);

	reg_val = readl(mock_gpio_dev.base + offset * 4);

	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_NE(test, reg_val & BIT(OUTPUT_ENABLE_OFF), 0U);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(OUTPUT_VALUE_OFF), 0U);

	return 0;
}

static int test_amd_gpio_direction_output_offset_zero(struct kunit *test)
{
	struct gpio_chip gc;
	u32 reg_val;
	unsigned offset = 0;
	int value = 1;
	int ret;

	mock_gpio_dev.base = mock_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	writel(0x0, mock_gpio_dev.base + offset * 4);

	ret = amd_gpio_direction_output(&gc, offset, value);

	reg_val = readl(mock_gpio_dev.base + offset * 4);

	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_NE(test, reg_val & BIT(OUTPUT_ENABLE_OFF), 0U);
	KUNIT_EXPECT_NE(test, reg_val & BIT(OUTPUT_VALUE_OFF), 0U);

	return 0;
}

static struct kunit_case amd_gpio_direction_output_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_direction_output_value_high),
	KUNIT_CASE(test_amd_gpio_direction_output_value_low),
	KUNIT_CASE(test_amd_gpio_direction_output_offset_zero),
	{}
};

static struct kunit_suite amd_gpio_direction_output_test_suite = {
	.name = "amd_gpio_direction_output_test",
	.test_cases = amd_gpio_direction_output_test_cases,
};

kunit_test_suite(amd_gpio_direction_output_test_suite);
