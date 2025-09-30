```c
// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/irq.h>
#include <linux/gpio/driver.h>
#include <linux/spinlock.h>
#include <linux/io.h>

#define WAKE_INT_MASTER_REG 0
#define EOI_MASK 0x1

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
};

static void amd_gpio_irq_eoi(struct irq_data *d)
{
	u32 reg;
	unsigned long flags;
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	reg = readl(gpio_dev->base + WAKE_INT_MASTER_REG);
	reg |= EOI_MASK;
	writel(reg, gpio_dev->base + WAKE_INT_MASTER_REG);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);
}

static char test_mmio_buffer[4096];
static struct amd_gpio mock_gpio_dev;
static struct gpio_chip mock_gpio_chip;
static struct irq_data mock_irq_data;

static void test_amd_gpio_irq_eoi_normal(struct kunit *test)
{
	unsigned long flags;
	u32 initial_reg = 0x0;
	
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);
	
	writel(initial_reg, mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	
	mock_irq_data.chip_data = &mock_gpio_chip;
	
	amd_gpio_irq_eoi(&mock_irq_data);
	
	u32 final_reg = readl(mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	KUNIT_EXPECT_EQ(test, final_reg, initial_reg | EOI_MASK);
}

static void test_amd_gpio_irq_eoi_already_set(struct kunit *test)
{
	unsigned long flags;
	u32 initial_reg = EOI_MASK;
	
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);
	
	writel(initial_reg, mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	
	mock_irq_data.chip_data = &mock_gpio_chip;
	
	amd_gpio_irq_eoi(&mock_irq_data);
	
	u32 final_reg = readl(mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	KUNIT_EXPECT_EQ(test, final_reg, initial_reg);
}

static void test_amd_gpio_irq_eoi_other_bits_set(struct kunit *test)
{
	unsigned long flags;
	u32 initial_reg = 0xFFFFFFFE;
	
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);
	
	writel(initial_reg, mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	
	mock_irq_data.chip_data = &mock_gpio_chip;
	
	amd_gpio_irq_eoi(&mock_irq_data);
	
	u32 final_reg = readl(mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	KUNIT_EXPECT_EQ(test, final_reg, initial_reg | EOI_MASK);
}

static struct kunit_case amd_gpio_irq_eoi_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_eoi_normal),
	KUNIT_CASE(test_amd_gpio_irq_eoi_already_set),
	KUNIT_CASE(test_amd_gpio_irq_eoi_other_bits_set),
	{}
};

static struct kunit_suite amd_gpio_irq_eoi_test_suite = {
	.name = "amd_gpio_irq_eoi_test",
	.test_cases = amd_gpio_irq_eoi_test_cases,
};

kunit_test_suite(amd_gpio_irq_eoi_test_suite);
```