#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/errno.h>

// Mock structure for pt_gpio_chip
struct pt_gpio_chip {
	void __iomem *reg_base;
};

#define PT_SYNC_REG 0x0
#define TEST_PIN_OFFSET 5

static char test_mmio_buffer[4096];
static struct pt_gpio_chip mock_pt_gpio;
static struct gpio_chip mock_gc;

// Stub for dev_dbg and dev_warn to prevent compilation issues
#define dev_dbg(dev, fmt, ...) printk(fmt, ##__VA_ARGS__)
#define dev_warn(dev, fmt, ...) printk(fmt, ##__VA_ARGS__)

// Helper to get private data from gpio_chip
static inline void *gpiochip_get_data(const struct gpio_chip *gc)
{
	return gc->private;
}

// Include the function under test
static int pt_gpio_request(struct gpio_chip *gc, unsigned offset)
{
	struct pt_gpio_chip *pt_gpio = gpiochip_get_data(gc);
	unsigned long flags;
	u32 using_pins;

	dev_dbg(gc->parent, "pt_gpio_request offset=%x\n", offset);

	raw_spin_lock_irqsave(&gc->bgpio_lock, flags);

	using_pins = readl(pt_gpio->reg_base + PT_SYNC_REG);
	if (using_pins & BIT(offset)) {
		dev_warn(gc->parent, "PT GPIO pin %x reconfigured\n",
			 offset);
		raw_spin_unlock_irqrestore(&gc->bgpio_lock, flags);
		return -EINVAL;
	}

	writel(using_pins | BIT(offset), pt_gpio->reg_base + PT_SYNC_REG);

	raw_spin_unlock_irqrestore(&gc->bgpio_lock, flags);

	return 0;
}

static void test_pt_gpio_request_success(struct kunit *test)
{
	int ret;
	u32 reg_val;

	// Initialize mock structures
	mock_pt_gpio.reg_base = test_mmio_buffer;
	mock_gc.private = &mock_pt_gpio;
	mock_gc.bgpio_lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gc.bgpio_lock);

	// Clear register
	writel(0x0, mock_pt_gpio.reg_base + PT_SYNC_REG);

	// Request GPIO pin
	ret = pt_gpio_request(&mock_gc, TEST_PIN_OFFSET);

	// Verify success
	KUNIT_EXPECT_EQ(test, ret, 0);

	// Check if bit is set
	reg_val = readl(mock_pt_gpio.reg_base + PT_SYNC_REG);
	KUNIT_EXPECT_TRUE(test, !!(reg_val & BIT(TEST_PIN_OFFSET)));
}

static void test_pt_gpio_request_pin_already_used(struct kunit *test)
{
	int ret;

	// Initialize mock structures
	mock_pt_gpio.reg_base = test_mmio_buffer;
	mock_gc.private = &mock_pt_gpio;
	mock_gc.bgpio_lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gc.bgpio_lock);

	// Pre-set the pin as used
	writel(BIT(TEST_PIN_OFFSET), mock_pt_gpio.reg_base + PT_SYNC_REG);

	// Try to request same pin again
	ret = pt_gpio_request(&mock_gc, TEST_PIN_OFFSET);

	// Should return error
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_pt_gpio_request_different_pins(struct kunit *test)
{
	int ret;
	u32 reg_val;

	// Initialize mock structures
	mock_pt_gpio.reg_base = test_mmio_buffer;
	mock_gc.private = &mock_pt_gpio;
	mock_gc.bgpio_lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gc.bgpio_lock);

	// Clear register
	writel(0x0, mock_pt_gpio.reg_base + PT_SYNC_REG);

	// Request two different pins
	ret = pt_gpio_request(&mock_gc, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = pt_gpio_request(&mock_gc, 10);
	KUNIT_EXPECT_EQ(test, ret, 0);

	// Check both bits are set
	reg_val = readl(mock_pt_gpio.reg_base + PT_SYNC_REG);
	KUNIT_EXPECT_TRUE(test, !!(reg_val & BIT(0)));
	KUNIT_EXPECT_TRUE(test, !!(reg_val & BIT(10)));
}

static struct kunit_case pt_gpio_request_test_cases[] = {
	KUNIT_CASE(test_pt_gpio_request_success),
	KUNIT_CASE(test_pt_gpio_request_pin_already_used),
	KUNIT_CASE(test_pt_gpio_request_different_pins),
	{}
};

static struct kunit_suite pt_gpio_request_test_suite = {
	.name = "pt_gpio_request_test",
	.test_cases = pt_gpio_request_test_cases,
};

kunit_test_suite(pt_gpio_request_test_suite);