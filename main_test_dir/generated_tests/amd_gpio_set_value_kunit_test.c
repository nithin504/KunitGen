// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/io.h>
#include <linux/spinlock.h>

#define OUTPUT_VALUE_OFF 16
#define BIT(x) (1U << (x))

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
};

static struct amd_gpio mock_gpio_dev;
static char mmio_buffer[4096];

static struct amd_gpio *gpiochip_get_data(struct gpio_chip *gc)
{
	return &mock_gpio_dev;
}

static u32 readl_mock(void __iomem *addr)
{
	return *((volatile u32 *)addr);
}

static void writel_mock(u32 value, void __iomem *addr)
{
	*((volatile u32 *)addr) = value;
}

#define readl(addr) readl_mock(addr)
#define writel(val, addr) writel_mock(val, addr)

static int amd_gpio_set_value(struct gpio_chip *gc, unsigned int offset, int value)
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

static void test_amd_gpio_set_value_high(struct kunit *test)
{
	struct gpio_chip gc;
	u32 *reg_addr = (u32 *)(mmio_buffer + 4);
	*reg_addr = 0x0;

	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	amd_gpio_set_value(&gc, 1, 1);

	u32 reg_val = *reg_addr;
	KUNIT_EXPECT_NE(test, reg_val & BIT(OUTPUT_VALUE_OFF), 0U);
}

static void test_amd_gpio_set_value_low(struct kunit *test)
{
	struct gpio_chip gc;
	u32 *reg_addr = (u32 *)(mmio_buffer + 8);
	*reg_addr = 0xFFFFFFFF;

	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	amd_gpio_set_value(&gc, 2, 0);

	u32 reg_val = *reg_addr;
	KUNIT_EXPECT_EQ(test, reg_val & BIT(OUTPUT_VALUE_OFF), 0U);
}

static void test_amd_gpio_set_value_no_change_on_same_value_high(struct kunit *test)
{
	struct gpio_chip gc;
	u32 *reg_addr = (u32 *)(mmio_buffer + 12);
	*reg_addr = BIT(OUTPUT_VALUE_OFF);

	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	amd_gpio_set_value(&gc, 3, 1);

	u32 reg_val = *reg_addr;
	KUNIT_EXPECT_EQ(test, reg_val, BIT(OUTPUT_VALUE_OFF));
}

static void test_amd_gpio_set_value_no_change_on_same_value_low(struct kunit *test)
{
	struct gpio_chip gc;
	u32 *reg_addr = (u32 *)(mmio_buffer + 16);
	*reg_addr = 0x0;

	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	amd_gpio_set_value(&gc, 4, 0);

	u32 reg_val = *reg_addr;
	KUNIT_EXPECT_EQ(test, reg_val, 0U);
}

static void test_amd_gpio_set_value_offset_zero(struct kunit *test)
{
	struct gpio_chip gc;
	u32 *reg_addr = (u32 *)mmio_buffer;
	*reg_addr = 0x0;

	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	amd_gpio_set_value(&gc, 0, 1);

	u32 reg_val = *reg_addr;
	KUNIT_EXPECT_NE(test, reg_val & BIT(OUTPUT_VALUE_OFF), 0U);
}

static struct kunit_case amd_gpio_set_value_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_set_value_high),
	KUNIT_CASE(test_amd_gpio_set_value_low),
	KUNIT_CASE(test_amd_gpio_set_value_no_change_on_same_value_high),
	KUNIT_CASE(test_amd_gpio_set_value_no_change_on_same_value_low),
	KUNIT_CASE(test_amd_gpio_set_value_offset_zero),
	{}
};

static struct kunit_suite amd_gpio_set_value_test_suite = {
	.name = "amd_gpio_set_value_test",
	.test_cases = amd_gpio_set_value_test_cases,
};

kunit_test_suite(amd_gpio_set_value_test_suite);
