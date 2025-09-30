#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/spinlock.h>
#include <linux/io.h>

static struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
} mock_gpio_dev;

static char test_mmio_buffer[4096];

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

static void test_amd_gpio_direction_input_success(struct kunit *test)
{
	struct gpio_chip gc;
	int ret;

	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	ret = amd_gpio_direction_input(&gc, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_direction_input_max_offset(struct kunit *test)
{
	struct gpio_chip gc;
	int ret;

	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	ret = amd_gpio_direction_input(&gc, 1023);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_direction_input_zero_offset(struct kunit *test)
{
	struct gpio_chip gc;
	int ret;

	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	ret = amd_gpio_direction_input(&gc, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_direction_input_high_offset(struct kunit *test)
{
	struct gpio_chip gc;
	int ret;

	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	ret = amd_gpio_direction_input(&gc, 4095);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static struct kunit_case amd_gpio_direction_input_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_direction_input_success),
	KUNIT_CASE(test_amd_gpio_direction_input_max_offset),
	KUNIT_CASE(test_amd_gpio_direction_input_zero_offset),
	KUNIT_CASE(test_amd_gpio_direction_input_high_offset),
	{}
};

static struct kunit_suite amd_gpio_direction_input_test_suite = {
	.name = "amd_gpio_direction_input_test",
	.test_cases = amd_gpio_direction_input_test_cases,
};

kunit_test_suite(amd_gpio_direction_input_test_suite);