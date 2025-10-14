// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/irq.h>
#include <linux/spinlock.h>
#include <linux/io.h>

// Define missing constants
#define INTERRUPT_MASK_OFF 16

// Mock structures
struct amd_gpio {
	void *base;
	raw_spinlock_t lock;
};

// Copy the function to test and make it static
static void amd_gpio_irq_mask(struct irq_data *d)
{
	u32 pin_reg;
	unsigned long flags;
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);
	irq_hw_number_t hwirq = irqd_to_hwirq(d);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + hwirq * 4);
	pin_reg &= ~BIT(INTERRUPT_MASK_OFF);
	writel(pin_reg, gpio_dev->base + hwirq * 4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);
}

// Test buffer for MMIO operations
static char test_buffer[4096];

// Test cases
static void test_amd_gpio_irq_mask_normal_operation(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	unsigned long flags;
	u32 initial_value = BIT(INTERRUPT_MASK_OFF) | 0x12345678;
	irq_hw_number_t hwirq = 5;

	// Initialize test structures
	gpio_dev.base = test_buffer;
	raw_spin_lock_init(&gpio_dev.lock);
	
	// Set up mock irq_data
	d.hwirq = hwirq;
	
	// Set initial register value
	writel(initial_value, gpio_dev.base + hwirq * 4);
	
	// Call the function
	amd_gpio_irq_mask(&d);
	
	// Verify the interrupt mask bit was cleared
	u32 result = readl(gpio_dev.base + hwirq * 4);
	KUNIT_EXPECT_EQ(test, result, initial_value & ~BIT(INTERRUPT_MASK_OFF));
}

static void test_amd_gpio_irq_mask_already_cleared(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	unsigned long flags;
	u32 initial_value = 0x12345678; // Interrupt mask already cleared
	irq_hw_number_t hwirq = 10;

	// Initialize test structures
	gpio_dev.base = test_buffer;
	raw_spin_lock_init(&gpio_dev.lock);
	
	// Set up mock irq_data
	d.hwirq = hwirq;
	
	// Set initial register value
	writel(initial_value, gpio_dev.base + hwirq * 4);
	
	// Call the function
	amd_gpio_irq_mask(&d);
	
	// Verify the register value remains unchanged
	u32 result = readl(gpio_dev.base + hwirq * 4);
	KUNIT_EXPECT_EQ(test, result, initial_value);
}

static void test_amd_gpio_irq_mask_zero_hwirq(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	unsigned long flags;
	u32 initial_value = BIT(INTERRUPT_MASK_OFF);
	irq_hw_number_t hwirq = 0;

	// Initialize test structures
	gpio_dev.base = test_buffer;
	raw_spin_lock_init(&gpio_dev.lock);
	
	// Set up mock irq_data
	d.hwirq = hwirq;
	
	// Set initial register value
	writel(initial_value, gpio_dev.base + hwirq * 4);
	
	// Call the function
	amd_gpio_irq_mask(&d);
	
	// Verify the interrupt mask bit was cleared
	u32 result = readl(gpio_dev.base + hwirq * 4);
	KUNIT_EXPECT_EQ(test, result, 0);
}

static void test_amd_gpio_irq_mask_max_hwirq(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	unsigned long flags;
	u32 initial_value = BIT(INTERRUPT_MASK_OFF);
	irq_hw_number_t hwirq = (sizeof(test_buffer) / 4) - 1; // Maximum valid offset

	// Initialize test structures
	gpio_dev.base = test_buffer;
	raw_spin_lock_init(&gpio_dev.lock);
	
	// Set up mock irq_data
	d.hwirq = hwirq;
	
	// Set initial register value
	writel(initial_value, gpio_dev.base + hwirq * 4);
	
	// Call the function
	amd_gpio_irq_mask(&d);
	
	// Verify the interrupt mask bit was cleared
	u32 result = readl(gpio_dev.base + hwirq * 4);
	KUNIT_EXPECT_EQ(test, result, 0);
}

static void test_amd_gpio_irq_mask_all_bits_set(struct kunit *test)
{
	struct irq_data d;
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	unsigned long flags;
	u32 initial_value = 0xFFFFFFFF; // All bits set
	irq_hw_number_t hwirq = 7;

	// Initialize test structures
	gpio_dev.base = test_buffer;
	raw_spin_lock_init(&gpio_dev.lock);
	
	// Set up mock irq_data
	d.hwirq = hwirq;
	
	// Set initial register value
	writel(initial_value, gpio_dev.base + hwirq * 4);
	
	// Call the function
	amd_gpio_irq_mask(&d);
	
	// Verify only the interrupt mask bit was cleared
	u32 result = readl(gpio_dev.base + hwirq * 4);
	KUNIT_EXPECT_EQ(test, result, initial_value & ~BIT(INTERRUPT_MASK_OFF));
}

static struct kunit_case amd_gpio_irq_mask_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_mask_normal_operation),
	KUNIT_CASE(test_amd_gpio_irq_mask_already_cleared),
	KUNIT_CASE(test_amd_gpio_irq_mask_zero_hwirq),
	KUNIT_CASE(test_amd_gpio_irq_mask_max_hwirq),
	KUNIT_CASE(test_amd_gpio_irq_mask_all_bits_set),
	{}
};

static struct kunit_suite amd_gpio_irq_mask_test_suite = {
	.name = "amd_gpio_irq_mask_test",
	.test_cases = amd_gpio_irq_mask_test_cases,
};

kunit_test_suite(amd_gpio_irq_mask_test_suite);