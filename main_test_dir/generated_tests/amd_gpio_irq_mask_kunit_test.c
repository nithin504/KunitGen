#include <kunit/test.h>
#include <linux/irq.h>
#include <linux/gpio/driver.h>
#include <linux/spinlock.h>
#include <linux/io.h>

#define INTERRUPT_MASK_OFF 16

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
};

static void amd_gpio_irq_mask(struct irq_data *d)
{
	u32 pin_reg;
	unsigned long flags;
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + (d->hwirq)*4);
	pin_reg &= ~BIT(INTERRUPT_MASK_OFF);
	writel(pin_reg, gpio_dev->base + (d->hwirq)*4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);
}

static char test_mmio_buffer[4096];
static struct amd_gpio mock_gpio_dev;
static struct gpio_chip mock_gpio_chip;
static struct irq_data mock_irq_data;

static void test_amd_gpio_irq_mask_normal(struct kunit *test)
{
	unsigned long flags;
	u32 initial_value = BIT(INTERRUPT_MASK_OFF);
	
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);
	
	writel(initial_value, mock_gpio_dev.base + 0 * 4);
	
	mock_irq_data.hwirq = 0;
	amd_gpio_irq_mask(&mock_irq_data);
	
	u32 result = readl(mock_gpio_dev.base + 0 * 4);
	KUNIT_EXPECT_EQ(test, result, 0);
}

static void test_amd_gpio_irq_mask_already_unmasked(struct kunit *test)
{
	unsigned long flags;
	u32 initial_value = 0;
	
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);
	
	writel(initial_value, mock_gpio_dev.base + 1 * 4);
	
	mock_irq_data.hwirq = 1;
	amd_gpio_irq_mask(&mock_irq_data);
	
	u32 result = readl(mock_gpio_dev.base + 1 * 4);
	KUNIT_EXPECT_EQ(test, result, 0);
}

static void test_amd_gpio_irq_mask_multiple_bits(struct kunit *test)
{
	unsigned long flags;
	u32 initial_value = BIT(INTERRUPT_MASK_OFF) | BIT(0) | BIT(31);
	
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);
	
	writel(initial_value, mock_gpio_dev.base + 2 * 4);
	
	mock_irq_data.hwirq = 2;
	amd_gpio_irq_mask(&mock_irq_data);
	
	u32 result = readl(mock_gpio_dev.base + 2 * 4);
	KUNIT_EXPECT_EQ(test, result, (BIT(0) | BIT(31)));
}

static void test_amd_gpio_irq_mask_different_hwirq(struct kunit *test)
{
	unsigned long flags;
	u32 initial_value = BIT(INTERRUPT_MASK_OFF);
	
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);
	
	writel(initial_value, mock_gpio_dev.base + 5 * 4);
	
	mock_irq_data.hwirq = 5;
	amd_gpio_irq_mask(&mock_irq_data);
	
	u32 result = readl(mock_gpio_dev.base + 5 * 4);
	KUNIT_EXPECT_EQ(test, result, 0);
	
	u32 other_pin = readl(mock_gpio_dev.base + 0 * 4);
	KUNIT_EXPECT_EQ(test, other_pin, 0);
}

static struct kunit_case amd_gpio_irq_mask_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_mask_normal),
	KUNIT_CASE(test_amd_gpio_irq_mask_already_unmasked),
	KUNIT_CASE(test_amd_gpio_irq_mask_multiple_bits),
	KUNIT_CASE(test_amd_gpio_irq_mask_different_hwirq),
	{}
};

static struct kunit_suite amd_gpio_irq_mask_test_suite = {
	.name = "amd_gpio_irq_mask_test",
	.test_cases = amd_gpio_irq_mask_test_cases,
};

kunit_test_suite(amd_gpio_irq_mask_test_suite);