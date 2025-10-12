// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/io.h>
#include <linux/spinlock.h>

#define PIN_STS_OFF 0

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
};

static struct amd_gpio mock_gpio_dev;
static char mock_mmio_buffer[4096];

static struct amd_gpio *gpiochip_get_data(struct gpio_chip *gc)
{
	return &mock_gpio_dev;
}

static u32 readl_mock(void __iomem *addr)
{
	return *((volatile u32 *)addr);
}

#define readl(addr) readl_mock(addr)

static int amd_gpio_get_value(struct gpio_chip *gc, unsigned offset)
{
	u32 pin_reg;
	unsigned long flags;
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + offset * 4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);

	return !!(pin_reg & BIT(PIN_STS_OFF));
}

static void test_amd_gpio_get_value_pin_low(struct kunit *test)
{
	struct gpio_chip gc;
	u32 *reg_addr = (u32 *)(mock_mmio_buffer + 4 * 1);
	*reg_addr = 0x0;

	mock_gpio_dev.base = mock_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	int ret = amd_gpio_get_value(&gc, 1);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_get_value_pin_high(struct kunit *test)
{
	struct gpio_chip gc;
	u32 *reg_addr = (u32 *)(mock_mmio_buffer + 4 * 2);
	*reg_addr = BIT(PIN_STS_OFF);

	mock_gpio_dev.base = mock_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	int ret = amd_gpio_get_value(&gc, 2);
	KUNIT_EXPECT_EQ(test, ret, 1);
}

static void test_amd_gpio_get_value_offset_zero(struct kunit *test)
{
	struct gpio_chip gc;
	u32 *reg_addr = (u32 *)mock_mmio_buffer;
	*reg_addr = BIT(PIN_STS_OFF);

	mock_gpio_dev.base = mock_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	int ret = amd_gpio_get_value(&gc, 0);
	KUNIT_EXPECT_EQ(test, ret, 1);
}

static void test_amd_gpio_get_value_bit_not_set(struct kunit *test)
{
	struct gpio_chip gc;
	u32 *reg_addr = (u32 *)(mock_mmio_buffer + 4 * 3);
	*reg_addr = 0xFFFFFFFF & ~BIT(PIN_STS_OFF);

	mock_gpio_dev.base = mock_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	int ret = amd_gpio_get_value(&gc, 3);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_get_value_with_other_bits_set(struct kunit *test)
{
	struct gpio_chip gc;
	u32 *reg_addr = (u32 *)(mock_mmio_buffer + 4 * 4);
	*reg_addr = BIT(PIN_STS_OFF) | 0xCAFEBABE;

	mock_gpio_dev.base = mock_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	int ret = amd_gpio_get_value(&gc, 4);
	KUNIT_EXPECT_EQ(test, ret, 1);
}

static struct kunit_case amd_gpio_get_value_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_get_value_pin_low),
	KUNIT_CASE(test_amd_gpio_get_value_pin_high),
	KUNIT_CASE(test_amd_gpio_get_value_offset_zero),
	KUNIT_CASE(test_amd_gpio_get_value_bit_not_set),
	KUNIT_CASE(test_amd_gpio_get_value_with_other_bits_set),
	{}
};

static struct kunit_suite amd_gpio_get_value_test_suite = {
	.name = "amd_gpio_get_value_test",
	.test_cases = amd_gpio_get_value_test_cases,
};

kunit_test_suite(amd_gpio_get_value_test_suite);
