// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/gpio/driver.h>
#include <linux/spinlock.h>

// Define necessary constants and structures based on pinctrl-amd.c
#define INTERRUPT_ENABLE_OFF 16
#define INTERRUPT_MASK_OFF 20

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
	struct gpio_chip chip;
};

// Include the source file for access to static functions
#include "pinctrl-amd.c"

// Mock data
static char mock_mmio_region[4096];
static struct amd_gpio mock_gpio_dev;
static struct gpio_chip mock_gpio_chip;
static struct irq_data mock_irq_data;

// Helper to initialize test environment
static void setup_mock_gpio(struct kunit *test)
{
	memset(&mock_gpio_dev, 0, sizeof(mock_gpio_dev));
	memset(&mock_gpio_chip, 0, sizeof(mock_gpio_chip));
	memset(&mock_irq_data, 0, sizeof(mock_irq_data));

	mock_gpio_dev.base = (void __iomem *)mock_mmio_region;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gpio_chip.private = &mock_gpio_dev;
	mock_irq_data.chip_data = &mock_gpio_chip;

	// Initialize MMIO region
	memset(mock_mmio_region, 0, sizeof(mock_mmio_region));
}

// Mock implementations
#define irq_data_get_irq_chip_data(d) ((d)->chip_data)
#define gpiochip_get_data(gc) ((gc)->private)
#define irqd_to_hwirq(d) ((d)->hwirq)

static int gpiochip_disable_irq_call_count;
void gpiochip_disable_irq(struct gpio_chip *gc, unsigned int offset)
{
	gpiochip_disable_irq_call_count++;
}

// Test case: normal execution path
static void test_amd_gpio_irq_disable_normal(struct kunit *test)
{
	setup_mock_gpio(test);
	gpiochip_disable_irq_call_count = 0;

	// Setup initial register value
	const u32 initial_value = 0xFFFFFFFF;
	const irq_hw_number_t hwirq = 5;
	writel(initial_value, mock_gpio_dev.base + hwirq * 4);

	// Configure irq_data
	mock_irq_data.hwirq = hwirq;

	// Execute function under test
	amd_gpio_irq_disable(&mock_irq_data);

	// Verify results
	u32 reg_val = readl(mock_gpio_dev.base + hwirq * 4);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(INTERRUPT_ENABLE_OFF), 0U);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(INTERRUPT_MASK_OFF), 0U);
	KUNIT_EXPECT_EQ(test, gpiochip_disable_irq_call_count, 1);
}

// Test case: hwirq at boundary (zero)
static void test_amd_gpio_irq_disable_zero_hwirq(struct kunit *test)
{
	setup_mock_gpio(test);
	gpiochip_disable_irq_call_count = 0;

	const u32 initial_value = 0xAAAAAAAA;
	const irq_hw_number_t hwirq = 0;
	writel(initial_value, mock_gpio_dev.base + hwirq * 4);

	mock_irq_data.hwirq = hwirq;
	amd_gpio_irq_disable(&mock_irq_data);

	u32 reg_val = readl(mock_gpio_dev.base + hwirq * 4);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(INTERRUPT_ENABLE_OFF), 0U);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(INTERRUPT_MASK_OFF), 0U);
	KUNIT_EXPECT_EQ(test, gpiochip_disable_irq_call_count, 1);
}

// Test case: large hwirq index
static void test_amd_gpio_irq_disable_large_hwirq(struct kunit *test)
{
	setup_mock_gpio(test);
	gpiochip_disable_irq_call_count = 0;

	const u32 initial_value = 0x55555555;
	const irq_hw_number_t hwirq = 100;
	writel(initial_value, mock_gpio_dev.base + hwirq * 4);

	mock_irq_data.hwirq = hwirq;
	amd_gpio_irq_disable(&mock_irq_data);

	u32 reg_val = readl(mock_gpio_dev.base + hwirq * 4);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(INTERRUPT_ENABLE_OFF), 0U);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(INTERRUPT_MASK_OFF), 0U);
	KUNIT_EXPECT_EQ(test, gpiochip_disable_irq_call_count, 1);
}

// Test case: already disabled interrupt bits
static void test_amd_gpio_irq_disable_already_disabled(struct kunit *test)
{
	setup_mock_gpio(test);
	gpiochip_disable_irq_call_count = 0;

	const u32 initial_value = ~(BIT(INTERRUPT_ENABLE_OFF) | BIT(INTERRUPT_MASK_OFF));
	const irq_hw_number_t hwirq = 10;
	writel(initial_value, mock_gpio_dev.base + hwirq * 4);

	mock_irq_data.hwirq = hwirq;
	amd_gpio_irq_disable(&mock_irq_data);

	u32 reg_val = readl(mock_gpio_dev.base + hwirq * 4);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(INTERRUPT_ENABLE_OFF), 0U);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(INTERRUPT_MASK_OFF), 0U);
	KUNIT_EXPECT_EQ(test, gpiochip_disable_irq_call_count, 1);
}

static struct kunit_case amd_gpio_irq_disable_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_disable_normal),
	KUNIT_CASE(test_amd_gpio_irq_disable_zero_hwirq),
	KUNIT_CASE(test_amd_gpio_irq_disable_large_hwirq),
	KUNIT_CASE(test_amd_gpio_irq_disable_already_disabled),
	{}
};

static struct kunit_suite amd_gpio_irq_disable_test_suite = {
	.name = "amd_gpio_irq_disable_test",
	.test_cases = amd_gpio_irq_disable_test_cases,
};

kunit_test_suite(amd_gpio_irq_disable_test_suite);
