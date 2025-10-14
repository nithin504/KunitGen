// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/spinlock.h>

#define PIN_COUNT 256
#define INTERRUPT_ENABLE_OFF 16
#define INTERRUPT_MASK_OFF 17

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
};

static char mock_mmio_region[PIN_COUNT * 4];

static struct gpio_chip mock_gc;
static struct amd_gpio mock_gpio_dev;
static struct irq_data mock_irq_data;

static struct gpio_chip *mock_irq_data_get_irq_chip_data(struct irq_data *d)
{
	return &mock_gc;
}

static struct amd_gpio *mock_gpiochip_get_data(struct gpio_chip *gc)
{
	return &mock_gpio_dev;
}

static unsigned int mock_irqd_to_hwirq(const struct irq_data *d)
{
	return 0;
}

static int gpiochip_enable_irq_call_count = 0;
static unsigned long gpiochip_enable_irq_hwirq_arg = 0;

static void mock_gpiochip_enable_irq(struct gpio_chip *gc, unsigned int hwirq)
{
	gpiochip_enable_irq_call_count++;
	gpiochip_enable_irq_hwirq_arg = hwirq;
}

#define irq_data_get_irq_chip_data mock_irq_data_get_irq_chip_data
#define gpiochip_get_data mock_gpiochip_get_data
#define irqd_to_hwirq mock_irqd_to_hwirq
#define gpiochip_enable_irq mock_gpiochip_enable_irq

#include "pinctrl-amd.c"

static void test_amd_gpio_irq_enable_normal(struct kunit *test)
{
	u32 expected_value;
	unsigned long flags;
	
	gpiochip_enable_irq_call_count = 0;
	gpiochip_enable_irq_hwirq_arg = 0;
	
	mock_gpio_dev.base = mock_mmio_region;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(0x0, mock_gpio_dev.base);
	
	amd_gpio_irq_enable(&mock_irq_data);
	
	expected_value = BIT(INTERRUPT_ENABLE_OFF) | BIT(INTERRUPT_MASK_OFF);
	KUNIT_EXPECT_EQ(test, readl(mock_gpio_dev.base), expected_value);
	KUNIT_EXPECT_EQ(test, gpiochip_enable_irq_call_count, 1);
	KUNIT_EXPECT_EQ(test, gpiochip_enable_irq_hwirq_arg, 0UL);
}

static void test_amd_gpio_irq_enable_with_existing_bits(struct kunit *test)
{
	u32 initial_value = 0xF0F0F0F0;
	u32 expected_value;
	
	mock_gpio_dev.base = mock_mmio_region;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(initial_value, mock_gpio_dev.base);
	
	amd_gpio_irq_enable(&mock_irq_data);
	
	expected_value = initial_value | BIT(INTERRUPT_ENABLE_OFF) | BIT(INTERRUPT_MASK_OFF);
	KUNIT_EXPECT_EQ(test, readl(mock_gpio_dev.base), expected_value);
}

static void test_amd_gpio_irq_enable_different_hwirq(struct kunit *test)
{
	static unsigned int custom_irqd_to_hwirq(const struct irq_data *d)
	{
		return 5;
	}
	
	unsigned int (*original_irqd_to_hwirq)(const struct irq_data *d) = irqd_to_hwirq;
	void __iomem *pin_address;
	u32 expected_value;
	
	irqd_to_hwirq = custom_irqd_to_hwirq;
	gpiochip_enable_irq_call_count = 0;
	gpiochip_enable_irq_hwirq_arg = 0;
	
	mock_gpio_dev.base = mock_mmio_region;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	pin_address = mock_gpio_dev.base + 5 * 4;
	writel(0x0, pin_address);
	
	amd_gpio_irq_enable(&mock_irq_data);
	
	expected_value = BIT(INTERRUPT_ENABLE_OFF) | BIT(INTERRUPT_MASK_OFF);
	KUNIT_EXPECT_EQ(test, readl(pin_address), expected_value);
	KUNIT_EXPECT_EQ(test, gpiochip_enable_irq_call_count, 1);
	KUNIT_EXPECT_EQ(test, gpiochip_enable_irq_hwirq_arg, 5UL);
	
	irqd_to_hwirq = original_irqd_to_hwirq;
}

static struct kunit_case amd_gpio_irq_enable_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_enable_normal),
	KUNIT_CASE(test_amd_gpio_irq_enable_with_existing_bits),
	KUNIT_CASE(test_amd_gpio_irq_enable_different_hwirq),
	{}
};

static struct kunit_suite amd_gpio_irq_enable_test_suite = {
	.name = "amd_gpio_irq_enable_test",
	.test_cases = amd_gpio_irq_enable_test_cases,
};

kunit_test_suite(amd_gpio_irq_enable_test_suite);
