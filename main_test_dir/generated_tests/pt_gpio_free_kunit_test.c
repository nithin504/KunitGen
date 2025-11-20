#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/io.h>
#include <linux/spinlock.h>

// Mock structure to simulate pt_gpio_chip
struct pt_gpio_chip {
	void __iomem *reg_base;
};

#define PT_SYNC_REG 0x0
#define TEST_PIN_OFFSET 5
#define MMIO_BUFFER_SIZE 4096

static struct pt_gpio_chip test_pt_gpio_chip;
static char mmio_buffer[MMIO_BUFFER_SIZE];
static struct gpio_chip test_gc;

// Stub function for dev_dbg to prevent compilation errors
__printf(3, 4)
static void stub_dev_dbg(struct device *dev, const char *fmt, ...)
{
	/* Do nothing */
}
#define dev_dbg(dev, fmt, ...) stub_dev_dbg(dev, fmt, ##__VA_ARGS__)

// Include the source file containing pt_gpio_free
#include "pt-gpio.c"

static void test_pt_gpio_free_basic(struct kunit *test)
{
	unsigned long flags;
	u32 reg_val_before, reg_val_after;

	// Setup initial register state with the pin bit set
	writel(BIT(TEST_PIN_OFFSET), test_pt_gpio_chip.reg_base + PT_SYNC_REG);

	// Capture value before calling pt_gpio_free
	reg_val_before = readl(test_pt_gpio_chip.reg_base + PT_SYNC_REG);

	// Call the function under test
	pt_gpio_free(&test_gc, TEST_PIN_OFFSET);

	// Check result
	reg_val_after = readl(test_pt_gpio_chip.reg_base + PT_SYNC_REG);
	KUNIT_EXPECT_EQ(test, reg_val_before & BIT(TEST_PIN_OFFSET), BIT(TEST_PIN_OFFSET));
	KUNIT_EXPECT_EQ(test, reg_val_after & BIT(TEST_PIN_OFFSET), 0U);
}

static void test_pt_gpio_free_multiple_pins(struct kunit *test)
{
	const unsigned int pin1 = 0;
	const unsigned int pin2 = 10;
	const unsigned int pin3 = 31; // Assuming 32-bit register
	u32 reg_val;

	// Set multiple bits
	writel(BIT(pin1) | BIT(pin2) | BIT(pin3),
	       test_pt_gpio_chip.reg_base + PT_SYNC_REG);

	// Free one pin
	pt_gpio_free(&test_gc, pin2);

	// Verify only that pin is cleared
	reg_val = readl(test_pt_gpio_chip.reg_base + PT_SYNC_REG);
	KUNIT_EXPECT_NE(test, reg_val & BIT(pin1), 0U);   // Should remain set
	KUNIT_EXPECT_EQ(test, reg_val & BIT(pin2), 0U);   // Should be cleared
	KUNIT_EXPECT_NE(test, reg_val & BIT(pin3), 0U);   // Should remain set
}

static void test_pt_gpio_free_all_bits(struct kunit *test)
{
	const u32 all_bits_set = 0xFFFFFFFF;
	u32 reg_val;

	// Set all bits
	writel(all_bits_set, test_pt_gpio_chip.reg_base + PT_SYNC_REG);

	// Free one specific pin
	pt_gpio_free(&test_gc, TEST_PIN_OFFSET);

	// Verify only that pin is cleared
	reg_val = readl(test_pt_gpio_chip.reg_base + PT_SYNC_REG);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(TEST_PIN_OFFSET), 0U);
	KUNIT_EXPECT_EQ(test, reg_val | BIT(TEST_PIN_OFFSET), all_bits_set);
}

static int pt_gpio_test_init(struct kunit *test)
{
	// Initialize mock hardware register space
	test_pt_gpio_chip.reg_base = (void __iomem *)mmio_buffer;

	// Initialize gpio_chip
	test_gc.parent = NULL; // No need for actual device in this test
	test_gc.bgpio_lock = __RAW_SPIN_LOCK_UNLOCKED(test_gc.bgpio_lock);
	
	// Assign private data
	gpiochip_set_data(&test_gc, &test_pt_gpio_chip);

	return 0;
}

static struct kunit_case pt_gpio_free_test_cases[] = {
	KUNIT_CASE(test_pt_gpio_free_basic),
	KUNIT_CASE(test_pt_gpio_free_multiple_pins),
	KUNIT_CASE(test_pt_gpio_free_all_bits),
	{}
};

static struct kunit_suite pt_gpio_free_test_suite = {
	.name = "pt_gpio_free",
	.init = pt_gpio_test_init,
	.test_cases = pt_gpio_free_test_cases,
};

kunit_test_suite(pt_gpio_free_test_suite);