#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/io.h>
#include <linux/spinlock.h>

#define PT_SYNC_REG 0x28

struct pt_gpio_chip {
	void __iomem *reg_base;
};

static void pt_gpio_free(struct gpio_chip *gc, unsigned offset);

static char test_mmio_buffer[4096];

static void test_pt_gpio_free_basic(struct kunit *test)
{
	struct gpio_chip gc;
	struct pt_gpio_chip pt_gpio_chip;
	unsigned long flags;
	u32 *sync_reg;
	u32 initial_value = 0xFFFFFFFF;
	u32 expected_value;
	unsigned offset = 5;

	/* Setup */
	memset(test_mmio_buffer, 0, sizeof(test_mmio_buffer));
	pt_gpio_chip.reg_base = test_mmio_buffer;
	sync_reg = (u32 *)(test_mmio_buffer + PT_SYNC_REG);
	*sync_reg = initial_value;
	
	gc.parent = NULL;
	gc.bgpio_lock = __RAW_SPIN_LOCK_UNLOCKED(gc.bgpio_lock);
	
	/* Perform test */
	pt_gpio_free(&gc, offset);
	
	/* Verify */
	expected_value = initial_value & ~BIT(offset);
	KUNIT_EXPECT_EQ(test, *sync_reg, expected_value);
}

static void test_pt_gpio_free_first_pin(struct kunit *test)
{
	struct gpio_chip gc;
	struct pt_gpio_chip pt_gpio_chip;
	u32 *sync_reg;
	u32 initial_value = 0xFFFFFFFF;
	u32 expected_value;
	unsigned offset = 0;

	/* Setup */
	memset(test_mmio_buffer, 0, sizeof(test_mmio_buffer));
	pt_gpio_chip.reg_base = test_mmio_buffer;
	sync_reg = (u32 *)(test_mmio_buffer + PT_SYNC_REG);
	*sync_reg = initial_value;
	
	gc.parent = NULL;
	gc.bgpio_lock = __RAW_SPIN_LOCK_UNLOCKED(gc.bgpio_lock);
	
	/* Perform test */
	pt_gpio_free(&gc, offset);
	
	/* Verify */
	expected_value = initial_value & ~BIT(offset);
	KUNIT_EXPECT_EQ(test, *sync_reg, expected_value);
}

static void test_pt_gpio_free_last_pin(struct kunit *test)
{
	struct gpio_chip gc;
	struct pt_gpio_chip pt_gpio_chip;
	u32 *sync_reg;
	u32 initial_value = 0xFFFFFFFF;
	u32 expected_value;
	unsigned offset = 31;

	/* Setup */
	memset(test_mmio_buffer, 0, sizeof(test_mmio_buffer));
	pt_gpio_chip.reg_base = test_mmio_buffer;
	sync_reg = (u32 *)(test_mmio_buffer + PT_SYNC_REG);
	*sync_reg = initial_value;
	
	gc.parent = NULL;
	gc.bgpio_lock = __RAW_SPIN_LOCK_UNLOCKED(gc.bgpio_lock);
	
	/* Perform test */
	pt_gpio_free(&gc, offset);
	
	/* Verify */
	expected_value = initial_value & ~BIT(offset);
	KUNIT_EXPECT_EQ(test, *sync_reg, expected_value);
}

static void test_pt_gpio_free_multiple_calls(struct kunit *test)
{
	struct gpio_chip gc;
	struct pt_gpio_chip pt_gpio_chip;
	u32 *sync_reg;
	u32 initial_value = 0xFFFFFFFF;
	u32 expected_value;
	unsigned offset1 = 3;
	unsigned offset2 = 7;

	/* Setup */
	memset(test_mmio_buffer, 0, sizeof(test_mmio_buffer));
	pt_gpio_chip.reg_base = test_mmio_buffer;
	sync_reg = (u32 *)(test_mmio_buffer + PT_SYNC_REG);
	*sync_reg = initial_value;
	
	gc.parent = NULL;
	gc.bgpio_lock = __RAW_SPIN_LOCK_UNLOCKED(gc.bgpio_lock);
	
	/* Perform test - free pin 3 */
	pt_gpio_free(&gc, offset1);
	
	/* Verify pin 3 is freed */
	expected_value = initial_value & ~BIT(offset1);
	KUNIT_EXPECT_EQ(test, *sync_reg, expected_value);
	
	/* Perform test - free pin 7 */
	pt_gpio_free(&gc, offset2);
	
	/* Verify both pins are freed */
	expected_value = initial_value & ~BIT(offset1) & ~BIT(offset2);
	KUNIT_EXPECT_EQ(test, *sync_reg, expected_value);
}

static struct kunit_case pt_gpio_free_test_cases[] = {
	KUNIT_CASE(test_pt_gpio_free_basic),
	KUNIT_CASE(test_pt_gpio_free_first_pin),
	KUNIT_CASE(test_pt_gpio_free_last_pin),
	KUNIT_CASE(test_pt_gpio_free_multiple_calls),
	{}
};

static struct kunit_suite pt_gpio_free_test_suite = {
	.name = "pt_gpio_free",
	.test_cases = pt_gpio_free_test_cases,
};

kunit_test_suite(pt_gpio_free_test_suite);

/* Implementation under test (since we can't include the original file due to conflicts) */
static void pt_gpio_free(struct gpio_chip *gc, unsigned offset)
{
	struct pt_gpio_chip *pt_gpio = gpiochip_get_data(gc);
	unsigned long flags;
	u32 using_pins;

	raw_spin_lock_irqsave(&gc->bgpio_lock, flags);

	using_pins = readl(pt_gpio->reg_base + PT_SYNC_REG);
	using_pins &= ~BIT(offset);
	writel(using_pins, pt_gpio->reg_base + PT_SYNC_REG);

	raw_spin_unlock_irqrestore(&gc->bgpio_lock, flags);

	dev_dbg(gc->parent, "pt_gpio_free offset=%x\n", offset);
}