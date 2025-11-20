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
#define MMIO_BUF_SIZE 4096

static struct pt_gpio_chip test_pt_gpio;
static char mmio_buffer[MMIO_BUF_SIZE];

// Helper function to initialize the mock chip
static void setup_mock_pt_gpio(struct kunit *test)
{
	test_pt_gpio.reg_base = (void __iomem *)mmio_buffer;
	memset(mmio_buffer, 0, MMIO_BUF_SIZE);
}

// Test freeing a pin clears the correct bit
static void test_pt_gpio_free_clears_bit(struct kunit *test)
{
	struct gpio_chip gc = { .bgpio_lock = __RAW_SPIN_LOCK_UNLOCKED(gc.bgpio_lock) };
	u32 reg_val_before, reg_val_after;

	setup_mock_pt_gpio(test);
	gc.parent = NULL; // Not used in this test
	
	// Set up initial register state with the pin bit set
	writel(BIT(TEST_PIN_OFFSET), test_pt_gpio.reg_base + PT_SYNC_REG);

	reg_val_before = readl(test_pt_gpio.reg_base + PT_SYNC_REG);
	KUNIT_EXPECT_NE(test, reg_val_before & BIT(TEST_PIN_OFFSET), 0U);

	// Call the function under test
	pt_gpio_free(&gc, TEST_PIN_OFFSET);

	reg_val_after = readl(test_pt_gpio.reg_base + PT_SYNC_REG);
	KUNIT_EXPECT_EQ(test, reg_val_after & BIT(TEST_PIN_OFFSET), 0U);
}

// Test freeing multiple pins maintains other bits
static void test_pt_gpio_free_preserves_other_bits(struct kunit *test)
{
	struct gpio_chip gc = { .bgpio_lock = __RAW_SPIN_LOCK_UNLOCKED(gc.bgpio_lock) };
	u32 reg_val_before, reg_val_after;
	const u32 initial_pins = BIT(1) | BIT(3) | BIT(TEST_PIN_OFFSET) | BIT(7);

	setup_mock_pt_gpio(test);
	gc.parent = NULL;

	// Set up initial register state with several pins set
	writel(initial_pins, test_pt_gpio.reg_base + PT_SYNC_REG);

	reg_val_before = readl(test_pt_gpio.reg_base + PT_SYNC_REG);
	KUNIT_EXPECT_EQ(test, reg_val_before, initial_pins);

	// Free only one pin
	pt_gpio_free(&gc, TEST_PIN_OFFSET);

	reg_val_after = readl(test_pt_gpio.reg_base + PT_SYNC_REG);
	// Check that our target pin is cleared but others remain
	KUNIT_EXPECT_EQ(test, reg_val_after & BIT(TEST_PIN_OFFSET), 0U);
	KUNIT_EXPECT_EQ(test, reg_val_after & (BIT(1) | BIT(3) | BIT(7)), 
				    initial_pins & (BIT(1) | BIT(3) | BIT(7)));
}

// Test freeing already cleared pin has no effect
static void test_pt_gpio_free_already_cleared(struct kunit *test)
{
	struct gpio_chip gc = { .bgpio_lock = __RAW_SPIN_LOCK_UNLOCKED(gc.bgpio_lock) };
	u32 reg_val_before, reg_val_after;

	setup_mock_pt_gpio(test);
	gc.parent = NULL;

	// Ensure register starts with the pin bit clear
	writel(0, test_pt_gpio.reg_base + PT_SYNC_REG);

	reg_val_before = readl(test_pt_gpio.reg_base + PT_SYNC_REG);
	KUNIT_EXPECT_EQ(test, reg_val_before & BIT(TEST_PIN_OFFSET), 0U);

	pt_gpio_free(&gc, TEST_PIN_OFFSET);

	reg_val_after = readl(test_pt_gpio.reg_base + PT_SYNC_REG);
	KUNIT_EXPECT_EQ(test, reg_val_after, 0U);
}

static struct kunit_case pt_gpio_free_test_cases[] = {
	KUNIT_CASE(test_pt_gpio_free_clears_bit),
	KUNIT_CASE(test_pt_gpio_free_preserves_other_bits),
	KUNIT_CASE(test_pt_gpio_free_already_cleared),
	{}
};

static struct kunit_suite pt_gpio_free_test_suite = {
	.name = "pt_gpio_free",
	.test_cases = pt_gpio_free_test_cases,
};

kunit_test_suite(pt_gpio_free_test_suite);