// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/io.h>
#include <linux/spinlock.h>

#define OUTPUT_ENABLE_OFF 22

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

static void test_amd_gpio_direction_input_valid(struct kunit *test)
{
	struct gpio_chip gc;
	struct amd_gpio *gpio_dev;
	char __iomem *base;
	u32 reg_val;

	base = kunit_kzalloc(test, 4096, GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, base);

	gpio_dev = kunit_kzalloc(test, sizeof(*gpio_dev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev);

	gpio_dev->base = base;
	raw_spin_lock_init(&gpio_dev->lock);
	gc.private = gpio_dev;

	/* Set initial register value with OUTPUT_ENABLE bit set */
	writel(0xFFFFFFFF, gpio_dev->base);

	amd_gpio_direction_input(&gc, 0);

	reg_val = readl(gpio_dev->base);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(OUTPUT_ENABLE_OFF), 0U);
}

static void test_amd_gpio_direction_input_offset_nonzero(struct kunit *test)
{
	struct gpio_chip gc;
	struct amd_gpio *gpio_dev;
	char __iomem *base;
	u32 reg_val;

	base = kunit_kzalloc(test, 4096, GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, base);

	gpio_dev = kunit_kzalloc(test, sizeof(*gpio_dev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev);

	gpio_dev->base = base;
	raw_spin_lock_init(&gpio_dev->lock);
	gc.private = gpio_dev;

	/* Initialize second pin register with OUTPUT_ENABLE bit set */
	writel(0xFFFFFFFF, gpio_dev->base + 4);

	amd_gpio_direction_input(&gc, 1);

	reg_val = readl(gpio_dev->base + 4);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(OUTPUT_ENABLE_OFF), 0U);
}

static void test_amd_gpio_direction_input_already_disabled(struct kunit *test)
{
	struct gpio_chip gc;
	struct amd_gpio *gpio_dev;
	char __iomem *base;
	u32 reg_val;

	base = kunit_kzalloc(test, 4096, GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, base);

	gpio_dev = kunit_kzalloc(test, sizeof(*gpio_dev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev);

	gpio_dev->base = base;
	raw_spin_lock_init(&gpio_dev->lock);
	gc.private = gpio_dev;

	/* Set initial register value with OUTPUT_ENABLE bit cleared */
	writel(0x0, gpio_dev->base);

	amd_gpio_direction_input(&gc, 0);

	reg_val = readl(gpio_dev->base);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(OUTPUT_ENABLE_OFF), 0U);
}

static struct kunit_case amd_gpio_direction_input_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_direction_input_valid),
	KUNIT_CASE(test_amd_gpio_direction_input_offset_nonzero),
	KUNIT_CASE(test_amd_gpio_direction_input_already_disabled),
	{}
};

static struct kunit_suite amd_gpio_direction_input_test_suite = {
	.name = "amd_gpio_direction_input_test",
	.test_cases = amd_gpio_direction_input_test_cases,
};

kunit_test_suite(amd_gpio_direction_input_test_suite);
