#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/spinlock.h>
#include <linux/io.h>

#define OUTPUT_VALUE_OFF 6

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
};

static int amd_gpio_set_value(struct gpio_chip *gc, unsigned int offset,
			      int value)
{
	u32 pin_reg;
	unsigned long flags;
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + offset * 4);
	if (value)
		pin_reg |= BIT(OUTPUT_VALUE_OFF);
	else
		pin_reg &= ~BIT(OUTPUT_VALUE_OFF);
	writel(pin_reg, gpio_dev->base + offset * 4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);

	return 0;
}

static char test_mmio_buffer[4096];
static struct amd_gpio mock_gpio_dev;
static struct gpio_chip mock_gc;

static void test_amd_gpio_set_value_high(struct kunit *test)
{
	unsigned int offset = 0;
	int value = 1;
	int ret;
	
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);
	writel(0x0, mock_gpio_dev.base + offset * 4);
	
	ret = amd_gpio_set_value(&mock_gc, offset, value);
	
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, readl(mock_gpio_dev.base + offset * 4) & BIT(OUTPUT_VALUE_OFF), BIT(OUTPUT_VALUE_OFF));
}

static void test_amd_gpio_set_value_low(struct kunit *test)
{
	unsigned int offset = 1;
	int value = 0;
	int ret;
	
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);
	writel(0xFFFFFFFF, mock_gpio_dev.base + offset * 4);
	
	ret = amd_gpio_set_value(&mock_gc, offset, value);
	
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, readl(mock_gpio_dev.base + offset * 4) & BIT(OUTPUT_VALUE_OFF), 0);
}

static void test_amd_gpio_set_value_high_from_low(struct kunit *test)
{
	unsigned int offset = 2;
	int value = 1;
	int ret;
	
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);
	writel(0x0, mock_gpio_dev.base + offset * 4);
	
	ret = amd_gpio_set_value(&mock_gc, offset, value);
	
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, readl(mock_gpio_dev.base + offset * 4) & BIT(OUTPUT_VALUE_OFF), BIT(OUTPUT_VALUE_OFF));
}

static void test_amd_gpio_set_value_low_from_high(struct kunit *test)
{
	unsigned int offset = 3;
	int value = 0;
	int ret;
	
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);
	writel(BIT(OUTPUT_VALUE_OFF), mock_gpio_dev.base + offset * 4);
	
	ret = amd_gpio_set_value(&mock_gc, offset, value);
	
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, readl(mock_gpio_dev.base + offset * 4) & BIT(OUTPUT_VALUE_OFF), 0);
}

static void test_amd_gpio_set_value_max_offset(struct kunit *test)
{
	unsigned int offset = (sizeof(test_mmio_buffer) / 4) - 1;
	int value = 1;
	int ret;
	
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);
	writel(0x0, mock_gpio_dev.base + offset * 4);
	
	ret = amd_gpio_set_value(&mock_gc, offset, value);
	
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, readl(mock_gpio_dev.base + offset * 4) & BIT(OUTPUT_VALUE_OFF), BIT(OUTPUT_VALUE_OFF));
}

static void test_amd_gpio_set_value_preserves_other_bits(struct kunit *test)
{
	unsigned int offset = 4;
	int value = 1;
	u32 initial_value = 0xAAAAAAAA;
	int ret;
	
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);
	writel(initial_value, mock_gpio_dev.base + offset * 4);
	
	ret = amd_gpio_set_value(&mock_gc, offset, value);
	
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, readl(mock_gpio_dev.base + offset * 4) & BIT(OUTPUT_VALUE_OFF), BIT(OUTPUT_VALUE_OFF));
	KUNIT_EXPECT_EQ(test, readl(mock_gpio_dev.base + offset * 4) & ~BIT(OUTPUT_VALUE_OFF), initial_value & ~BIT(OUTPUT_VALUE_OFF));
}

static struct kunit_case amd_gpio_set_value_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_set_value_high),
	KUNIT_CASE(test_amd_gpio_set_value_low),
	KUNIT_CASE(test_amd_gpio_set_value_high_from_low),
	KUNIT_CASE(test_amd_gpio_set_value_low_from_high),
	KUNIT_CASE(test_amd_gpio_set_value_max_offset),
	KUNIT_CASE(test_amd_gpio_set_value_preserves_other_bits),
	{}
};

static struct kunit_suite amd_gpio_set_value_test_suite = {
	.name = "amd_gpio_set_value_test",
	.test_cases = amd_gpio_set_value_test_cases,
};

kunit_test_suite(amd_gpio_set_value_test_suite);