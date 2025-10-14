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

static void __iomem *mock_base_addr(void)
{
	return mock_mmio_buffer;
}

static struct amd_gpio *gpiochip_get_data(struct gpio_chip *gc)
{
	return &mock_gpio_dev;
}

static u32 readl_mock(const volatile void __iomem *addr)
{
	ptrdiff_t offset = (uintptr_t)addr - (uintptr_t)mock_mmio_buffer;
	if (offset >= 0 && offset < sizeof(mock_mmio_buffer))
		return *(volatile u32 *)(mock_mmio_buffer + offset);
	return 0;
}

static void writel_mock(u32 value, volatile void __iomem *addr)
{
	ptrdiff_t offset = (uintptr_t)addr - (uintptr_t)mock_mmio_buffer;
	if (offset >= 0 && offset < sizeof(mock_mmio_buffer))
		*(volatile u32 *)(mock_mmio_buffer + offset) = value;
}

#define readl(addr) readl_mock(addr)
#define writel(val, addr) writel_mock(val, addr)

static int amd_gpio_direction_output(struct gpio_chip *gc, unsigned offset,
				     int value)
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
	u32 expected_reg_val;

	mock_gpio_dev.base = mock_base_addr();
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	// Initialize register to known value
	*((u32 *)(mock_mmio_buffer + offset * 4)) = 0x0;

	amd_gpio_direction_output(&gc, offset, value);

	expected_reg_val = BIT(OUTPUT_ENABLE_OFF) | BIT(OUTPUT_VALUE_OFF);
	KUNIT_EXPECT_EQ(test, readl(mock_gpio_dev.base + offset * 4), expected_reg_val);
}

static void test_amd_gpio_direction_output_value_low(struct kunit *test)
{
	struct gpio_chip gc;
	unsigned offset = 2;
	int value = 0;
	u32 expected_reg_val;

	mock_gpio_dev.base = mock_base_addr();
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	// Initialize register to known value with bit set
	*((u32 *)(mock_mmio_buffer + offset * 4)) = BIT(OUTPUT_VALUE_OFF);

	amd_gpio_direction_output(&gc, offset, value);

	expected_reg_val = BIT(OUTPUT_ENABLE_OFF);
	KUNIT_EXPECT_EQ(test, readl(mock_gpio_dev.base + offset * 4), expected_reg_val);
}

static void test_amd_gpio_direction_output_offset_zero(struct kunit *test)
{
	struct gpio_chip gc;
	unsigned offset = 0;
	int value = 1;
	u32 expected_reg_val;

	mock_gpio_dev.base = mock_base_addr();
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	*((u32 *)(mock_mmio_buffer + offset * 4)) = 0x0;

	amd_gpio_direction_output(&gc, offset, value);

	expected_reg_val = BIT(OUTPUT_ENABLE_OFF) | BIT(OUTPUT_VALUE_OFF);
	KUNIT_EXPECT_EQ(test, readl(mock_gpio_dev.base + offset * 4), expected_reg_val);
}

static void test_amd_gpio_direction_output_preserve_other_bits(struct kunit *test)
{
	struct gpio_chip gc;
	unsigned offset = 3;
	int value = 1;
	u32 initial_reg_val = 0x12345678;
	u32 expected_reg_val;

	mock_gpio_dev.base = mock_base_addr();
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	*((u32 *)(mock_mmio_buffer + offset * 4)) = initial_reg_val;

	amd_gpio_direction_output(&gc, offset, value);

	expected_reg_val = initial_reg_val | BIT(OUTPUT_ENABLE_OFF) | BIT(OUTPUT_VALUE_OFF);
	KUNIT_EXPECT_EQ(test, readl(mock_gpio_dev.base + offset * 4), expected_reg_val);
}

static struct kunit_case amd_gpio_direction_output_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_direction_output_value_high),
	KUNIT_CASE(test_amd_gpio_direction_output_value_low),
	KUNIT_CASE(test_amd_gpio_direction_output_offset_zero),
	KUNIT_CASE(test_amd_gpio_direction_output_preserve_other_bits),
	{}
};

static struct kunit_suite amd_gpio_direction_output_test_suite = {
	.name = "amd_gpio_direction_output_test",
	.test_cases = amd_gpio_direction_output_test_cases,
};

kunit_test_suite(amd_gpio_direction_output_test_suite);
