#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/io.h>
#include <linux/spinlock.h>

#define PT_SYNC_REG 0x28

struct pt_gpio_chip {
	void __iomem *reg_base;
};

static char test_mmio_buffer[4096];

static int pt_gpio_request(struct gpio_chip *gc, unsigned offset)
{
	struct pt_gpio_chip *pt_gpio = gpiochip_get_data(gc);
	unsigned long flags;
	u32 using_pins;

	raw_spin_lock_irqsave(&gc->bgpio_lock, flags);

	using_pins = readl(pt_gpio->reg_base + PT_SYNC_REG);
	if (using_pins & BIT(offset)) {
		raw_spin_unlock_irqrestore(&gc->bgpio_lock, flags);
		return -EINVAL;
	}

	writel(using_pins | BIT(offset), pt_gpio->reg_base + PT_SYNC_REG);

	raw_spin_unlock_irqrestore(&gc->bgpio_lock, flags);

	return 0;
}

static void test_pt_gpio_request_success(struct kunit *test)
{
	struct gpio_chip mock_gc = {};
	struct pt_gpio_chip mock_pt_gpio = {};
	unsigned long flags;
	int ret;

	mock_gc.parent = NULL;
	mock_gc.bgpio_lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gc.bgpio_lock);
	mock_pt_gpio.reg_base = test_mmio_buffer;
	memset(test_mmio_buffer, 0, sizeof(test_mmio_buffer));

	gpiochip_set_data(&mock_gc, &mock_pt_gpio);

	writel(0x0, mock_pt_gpio.reg_base + PT_SYNC_REG);

	ret = pt_gpio_request(&mock_gc, 5);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(mock_pt_gpio.reg_base + PT_SYNC_REG);
	KUNIT_EXPECT_EQ(test, reg_val, BIT(5));
}

static void test_pt_gpio_request_pin_already_used(struct kunit *test)
{
	struct gpio_chip mock_gc = {};
	struct pt_gpio_chip mock_pt_gpio = {};
	int ret;

	mock_gc.parent = NULL;
	mock_gc.bgpio_lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gc.bgpio_lock);
	mock_pt_gpio.reg_base = test_mmio_buffer;
	memset(test_mmio_buffer, 0, sizeof(test_mmio_buffer));

	gpiochip_set_data(&mock_gc, &mock_pt_gpio);

	writel(BIT(3), mock_pt_gpio.reg_base + PT_SYNC_REG);

	ret = pt_gpio_request(&mock_gc, 3);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_pt_gpio_request_different_pin(struct kunit *test)
{
	struct gpio_chip mock_gc = {};
	struct pt_gpio_chip mock_pt_gpio = {};
	int ret;

	mock_gc.parent = NULL;
	mock_gc.bgpio_lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gc.bgpio_lock);
	mock_pt_gpio.reg_base = test_mmio_buffer;
	memset(test_mmio_buffer, 0, sizeof(test_mmio_buffer));

	gpiochip_set_data(&mock_gc, &mock_pt_gpio);

	writel(BIT(2), mock_pt_gpio.reg_base + PT_SYNC_REG);

	ret = pt_gpio_request(&mock_gc, 7);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(mock_pt_gpio.reg_base + PT_SYNC_REG);
	KUNIT_EXPECT_EQ(test, reg_val, BIT(2) | BIT(7));
}

static struct kunit_case pt_gpio_request_test_cases[] = {
	KUNIT_CASE(test_pt_gpio_request_success),
	KUNIT_CASE(test_pt_gpio_request_pin_already_used),
	KUNIT_CASE(test_pt_gpio_request_different_pin),
	{}
};

static struct kunit_suite pt_gpio_request_test_suite = {
	.name = "pt_gpio_request",
	.test_cases = pt_gpio_request_test_cases,
};

kunit_test_suite(pt_gpio_request_test_suite);