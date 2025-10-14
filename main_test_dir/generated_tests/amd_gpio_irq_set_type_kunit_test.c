// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/irq.h>
#include <linux/gpio/driver.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/platform_device.h>

// Mock definitions for AMD GPIO driver constants
#define LEVEL_TRIG_OFF          22
#define ACTIVE_LEVEL_OFF        20
#define ACTIVE_LEVEL_MASK       0x3
#define ACTIVE_HIGH             0x1
#define ACTIVE_LOW              0x0
#define BOTH_EDGES              0x2
#define LEVEL_TRIGGER           0x1
#define CLR_INTR_STAT           0x1
#define INTERRUPT_STS_OFF       19
#define INTERRUPT_ENABLE_OFF    16
#define INTERRUPT_MASK_OFF      17

// Mock structures
struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
	struct platform_device *pdev;
};

// Function under test
static int amd_gpio_irq_set_type(struct irq_data *d, unsigned int type)
{
	int ret = 0;
	u32 pin_reg, pin_reg_irq_en, mask;
	unsigned long flags;
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);
	irq_hw_number_t hwirq = irqd_to_hwirq(d);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + hwirq * 4);

	switch (type & IRQ_TYPE_SENSE_MASK) {
	case IRQ_TYPE_EDGE_RISING:
		pin_reg &= ~BIT(LEVEL_TRIG_OFF);
		pin_reg &= ~(ACTIVE_LEVEL_MASK << ACTIVE_LEVEL_OFF);
		pin_reg |= ACTIVE_HIGH << ACTIVE_LEVEL_OFF;
		irq_set_handler_locked(d, handle_edge_irq);
		break;

	case IRQ_TYPE_EDGE_FALLING:
		pin_reg &= ~BIT(LEVEL_TRIG_OFF);
		pin_reg &= ~(ACTIVE_LEVEL_MASK << ACTIVE_LEVEL_OFF);
		pin_reg |= ACTIVE_LOW << ACTIVE_LEVEL_OFF;
		irq_set_handler_locked(d, handle_edge_irq);
		break;

	case IRQ_TYPE_EDGE_BOTH:
		pin_reg &= ~BIT(LEVEL_TRIG_OFF);
		pin_reg &= ~(ACTIVE_LEVEL_MASK << ACTIVE_LEVEL_OFF);
		pin_reg |= BOTH_EDGES << ACTIVE_LEVEL_OFF;
		irq_set_handler_locked(d, handle_edge_irq);
		break;

	case IRQ_TYPE_LEVEL_HIGH:
		pin_reg |= LEVEL_TRIGGER << LEVEL_TRIG_OFF;
		pin_reg &= ~(ACTIVE_LEVEL_MASK << ACTIVE_LEVEL_OFF);
		pin_reg |= ACTIVE_HIGH << ACTIVE_LEVEL_OFF;
		irq_set_handler_locked(d, handle_level_irq);
		break;

	case IRQ_TYPE_LEVEL_LOW:
		pin_reg |= LEVEL_TRIGGER << LEVEL_TRIG_OFF;
		pin_reg &= ~(ACTIVE_LEVEL_MASK << ACTIVE_LEVEL_OFF);
		pin_reg |= ACTIVE_LOW << ACTIVE_LEVEL_OFF;
		irq_set_handler_locked(d, handle_level_irq);
		break;

	case IRQ_TYPE_NONE:
		break;

	default:
		dev_err(&gpio_dev->pdev->dev, "Invalid type value\n");
		ret = -EINVAL;
	}

	pin_reg |= CLR_INTR_STAT << INTERRUPT_STS_OFF;
	mask = BIT(INTERRUPT_ENABLE_OFF);
	pin_reg_irq_en = pin_reg;
	pin_reg_irq_en |= mask;
	pin_reg_irq_en &= ~BIT(INTERRUPT_MASK_OFF);
	writel(pin_reg_irq_en, gpio_dev->base + hwirq * 4);
	while ((readl(gpio_dev->base + hwirq * 4) & mask) != mask)
		continue;
	writel(pin_reg, gpio_dev->base + hwirq * 4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);

	return ret;
}

// Test helper macros
#define TEST_HWIRQ 0
#define REG_SIZE 4

// Test cases
static void test_amd_gpio_irq_set_type_edge_rising(struct kunit *test)
{
	struct amd_gpio gpio_dev;
	char mmio_buffer[REG_SIZE * 2] = {0};
	struct gpio_chip gc = {};
	struct irq_data d = {};
	int ret;

	gpio_dev.base = (void __iomem *)mmio_buffer;
	raw_spin_lock_init(&gpio_dev.lock);
	gpio_dev.pdev = kunit_kzalloc(test, sizeof(*gpio_dev.pdev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev.pdev);

	gc.private = &gpio_dev;
	d.chip_data = &gc;
	d.hwirq = TEST_HWIRQ;

	// Initial register value
	writel(0xFFFFFFFF, gpio_dev.base + TEST_HWIRQ * REG_SIZE);

	ret = amd_gpio_irq_set_type(&d, IRQ_TYPE_EDGE_RISING);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(gpio_dev.base + TEST_HWIRQ * REG_SIZE);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(LEVEL_TRIG_OFF), 0U);
	KUNIT_EXPECT_EQ(test, (reg_val >> ACTIVE_LEVEL_OFF) & ACTIVE_LEVEL_MASK, ACTIVE_HIGH);
}

static void test_amd_gpio_irq_set_type_edge_falling(struct kunit *test)
{
	struct amd_gpio gpio_dev;
	char mmio_buffer[REG_SIZE * 2] = {0};
	struct gpio_chip gc = {};
	struct irq_data d = {};
	int ret;

	gpio_dev.base = (void __iomem *)mmio_buffer;
	raw_spin_lock_init(&gpio_dev.lock);
	gpio_dev.pdev = kunit_kzalloc(test, sizeof(*gpio_dev.pdev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev.pdev);

	gc.private = &gpio_dev;
	d.chip_data = &gc;
	d.hwirq = TEST_HWIRQ;

	writel(0xFFFFFFFF, gpio_dev.base + TEST_HWIRQ * REG_SIZE);

	ret = amd_gpio_irq_set_type(&d, IRQ_TYPE_EDGE_FALLING);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(gpio_dev.base + TEST_HWIRQ * REG_SIZE);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(LEVEL_TRIG_OFF), 0U);
	KUNIT_EXPECT_EQ(test, (reg_val >> ACTIVE_LEVEL_OFF) & ACTIVE_LEVEL_MASK, ACTIVE_LOW);
}

static void test_amd_gpio_irq_set_type_edge_both(struct kunit *test)
{
	struct amd_gpio gpio_dev;
	char mmio_buffer[REG_SIZE * 2] = {0};
	struct gpio_chip gc = {};
	struct irq_data d = {};
	int ret;

	gpio_dev.base = (void __iomem *)mmio_buffer;
	raw_spin_lock_init(&gpio_dev.lock);
	gpio_dev.pdev = kunit_kzalloc(test, sizeof(*gpio_dev.pdev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev.pdev);

	gc.private = &gpio_dev;
	d.chip_data = &gc;
	d.hwirq = TEST_HWIRQ;

	writel(0xFFFFFFFF, gpio_dev.base + TEST_HWIRQ * REG_SIZE);

	ret = amd_gpio_irq_set_type(&d, IRQ_TYPE_EDGE_BOTH);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(gpio_dev.base + TEST_HWIRQ * REG_SIZE);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(LEVEL_TRIG_OFF), 0U);
	KUNIT_EXPECT_EQ(test, (reg_val >> ACTIVE_LEVEL_OFF) & ACTIVE_LEVEL_MASK, BOTH_EDGES);
}

static void test_amd_gpio_irq_set_type_level_high(struct kunit *test)
{
	struct amd_gpio gpio_dev;
	char mmio_buffer[REG_SIZE * 2] = {0};
	struct gpio_chip gc = {};
	struct irq_data d = {};
	int ret;

	gpio_dev.base = (void __iomem *)mmio_buffer;
	raw_spin_lock_init(&gpio_dev.lock);
	gpio_dev.pdev = kunit_kzalloc(test, sizeof(*gpio_dev.pdev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev.pdev);

	gc.private = &gpio_dev;
	d.chip_data = &gc;
	d.hwirq = TEST_HWIRQ;

	writel(0x0, gpio_dev.base + TEST_HWIRQ * REG_SIZE);

	ret = amd_gpio_irq_set_type(&d, IRQ_TYPE_LEVEL_HIGH);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(gpio_dev.base + TEST_HWIRQ * REG_SIZE);
	KUNIT_EXPECT_NE(test, reg_val & BIT(LEVEL_TRIG_OFF), 0U);
	KUNIT_EXPECT_EQ(test, (reg_val >> ACTIVE_LEVEL_OFF) & ACTIVE_LEVEL_MASK, ACTIVE_HIGH);
}

static void test_amd_gpio_irq_set_type_level_low(struct kunit *test)
{
	struct amd_gpio gpio_dev;
	char mmio_buffer[REG_SIZE * 2] = {0};
	struct gpio_chip gc = {};
	struct irq_data d = {};
	int ret;

	gpio_dev.base = (void __iomem *)mmio_buffer;
	raw_spin_lock_init(&gpio_dev.lock);
	gpio_dev.pdev = kunit_kzalloc(test, sizeof(*gpio_dev.pdev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev.pdev);

	gc.private = &gpio_dev;
	d.chip_data = &gc;
	d.hwirq = TEST_HWIRQ;

	writel(0x0, gpio_dev.base + TEST_HWIRQ * REG_SIZE);

	ret = amd_gpio_irq_set_type(&d, IRQ_TYPE_LEVEL_LOW);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(gpio_dev.base + TEST_HWIRQ * REG_SIZE);
	KUNIT_EXPECT_NE(test, reg_val & BIT(LEVEL_TRIG_OFF), 0U);
	KUNIT_EXPECT_EQ(test, (reg_val >> ACTIVE_LEVEL_OFF) & ACTIVE_LEVEL_MASK, ACTIVE_LOW);
}

static void test_amd_gpio_irq_set_type_none(struct kunit *test)
{
	struct amd_gpio gpio_dev;
	char mmio_buffer[REG_SIZE * 2] = {0};
	struct gpio_chip gc = {};
	struct irq_data d = {};
	int ret;

	gpio_dev.base = (void __iomem *)mmio_buffer;
	raw_spin_lock_init(&gpio_dev.lock);
	gpio_dev.pdev = kunit_kzalloc(test, sizeof(*gpio_dev.pdev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev.pdev);

	gc.private = &gpio_dev;
	d.chip_data = &gc;
	d.hwirq = TEST_HWIRQ;

	writel(0x12345678, gpio_dev.base + TEST_HWIRQ * REG_SIZE);

	ret = amd_gpio_irq_set_type(&d, IRQ_TYPE_NONE);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(gpio_dev.base + TEST_HWIRQ * REG_SIZE);
	KUNIT_EXPECT_EQ(test, reg_val, 0x12345678U | (CLR_INTR_STAT << INTERRUPT_STS_OFF));
}

static void test_amd_gpio_irq_set_type_invalid(struct kunit *test)
{
	struct amd_gpio gpio_dev;
	char mmio_buffer[REG_SIZE * 2] = {0};
	struct gpio_chip gc = {};
	struct irq_data d = {};
	int ret;

	gpio_dev.base = (void __iomem *)mmio_buffer;
	raw_spin_lock_init(&gpio_dev.lock);
	gpio_dev.pdev = kunit_kzalloc(test, sizeof(*gpio_dev.pdev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev.pdev);

	gc.private = &gpio_dev;
	d.chip_data = &gc;
	d.hwirq = TEST_HWIRQ;

	writel(0x0, gpio_dev.base + TEST_HWIRQ * REG_SIZE);

	ret = amd_gpio_irq_set_type(&d, 0xFF); // Invalid type
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_amd_gpio_irq_set_type_clear_interrupt_status_bit(struct kunit *test)
{
	struct amd_gpio gpio_dev;
	char mmio_buffer[REG_SIZE * 2] = {0};
	struct gpio_chip gc = {};
	struct irq_data d = {};
	int ret;

	gpio_dev.base = (void __iomem *)mmio_buffer;
	raw_spin_lock_init(&gpio_dev.lock);
	gpio_dev.pdev = kunit_kzalloc(test, sizeof(*gpio_dev.pdev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev.pdev);

	gc.private = &gpio_dev;
	d.chip_data = &gc;
	d.hwirq = TEST_HWIRQ;

	writel(0x0, gpio_dev.base + TEST_HWIRQ * REG_SIZE);

	ret = amd_gpio_irq_set_type(&d, IRQ_TYPE_EDGE_RISING);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(gpio_dev.base + TEST_HWIRQ * REG_SIZE);
	KUNIT_EXPECT_NE(test, reg_val & (CLR_INTR_STAT << INTERRUPT_STS_OFF), 0U);
}

static void test_amd_gpio_irq_set_type_polling_enable_disable(struct kunit *test)
{
	struct amd_gpio gpio_dev;
	char mmio_buffer[REG_SIZE * 2] = {0};
	struct gpio_chip gc = {};
	struct irq_data d = {};
	int ret;

	gpio_dev.base = (void __iomem *)mmio_buffer;
	raw_spin_lock_init(&gpio_dev.lock);
	gpio_dev.pdev = kunit_kzalloc(test, sizeof(*gpio_dev.pdev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev.pdev);

	gc.private = &gpio_dev;
	d.chip_data = &gc;
	d.hwirq = TEST_HWIRQ;

	// Simulate polling behavior by pre-setting the expected final value
	writel(0x0, gpio_dev.base + TEST_HWIRQ * REG_SIZE);

	ret = amd_gpio_irq_set_type(&d, IRQ_TYPE_EDGE_RISING);
	KUNIT_EXPECT_EQ(test, ret, 0);

	// Final register value should have interrupt disabled but status cleared
	u32 reg_val = readl(gpio_dev.base + TEST_HWIRQ * REG_SIZE);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(INTERRUPT_MASK_OFF), BIT(INTERRUPT_MASK_OFF));
	KUNIT_EXPECT_NE(test, reg_val & (CLR_INTR_STAT << INTERRUPT_STS_OFF), 0U);
}

static struct kunit_case amd_gpio_irq_set_type_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_set_type_edge_rising),
	KUNIT_CASE(test_amd_gpio_irq_set_type_edge_falling),
	KUNIT_CASE(test_amd_gpio_irq_set_type_edge_both),
	KUNIT_CASE(test_amd_gpio_irq_set_type_level_high),
	KUNIT_CASE(test_amd_gpio_irq_set_type_level_low),
	KUNIT_CASE(test_amd_gpio_irq_set_type_none),
	KUNIT_CASE(test_amd_gpio_irq_set_type_invalid),
	KUNIT_CASE(test_amd_gpio_irq_set_type_clear_interrupt_status_bit),
	KUNIT_CASE(test_amd_gpio_irq_set_type_polling_enable_disable),
	{}
};

static struct kunit_suite amd_gpio_irq_set_type_test_suite = {
	.name = "amd_gpio_irq_set_type_test",
	.test_cases = amd_gpio_irq_set_type_test_cases,
};

kunit_test_suite(amd_gpio_irq_set_type_test_suite);
