// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/gpio/driver.h>
#include <linux/spinlock.h>

#define INTERRUPT_MASK_OFF 25
#define BIT(nr) (1UL << (nr))

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
	struct gpio_chip chip;
};

static struct amd_gpio *gpiochip_get_data(struct gpio_chip *chip)
{
	return container_of(chip, struct amd_gpio, chip);
}

static void amd_gpio_irq_unmask(struct irq_data *d)
{
	u32 pin_reg;
	unsigned long flags;
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);
	irq_hw_number_t hwirq = irqd_to_hwirq(d);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + hwirq * 4);
	pin_reg |= BIT(INTERRUPT_MASK_OFF);
	writel(pin_reg, gpio_dev->base + hwirq * 4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);
}

static char mmio_buffer[4096];

static void test_amd_gpio_irq_unmask_normal(struct kunit *test)
{
	struct amd_gpio gpio_dev = {
		.base = mmio_buffer,
		.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock),
	};
	struct gpio_chip gc = {
		.parent = NULL,
	};
	struct irq_data d = {
		.chip_data = &gc,
	};

	irq_hw_number_t hwirq = 2;
	u32 expected_offset = hwirq * 4;
	void __iomem *reg_addr = gpio_dev.base + expected_offset;

	// Initialize register to known value
	writel(0x0, reg_addr);

	// Override irqd_to_hwirq and irq_data_get_irq_chip_data via direct assignment
	// Since we can't redefine macros, we simulate behavior through structure setup

	amd_gpio_irq_unmask(&d);

	u32 result = readl(reg_addr);
	KUNIT_EXPECT_EQ(test, result, BIT(INTERRUPT_MASK_OFF));
}

static struct kunit_case amd_gpio_irq_unmask_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_unmask_normal),
	{}
};

static struct kunit_suite amd_gpio_irq_unmask_test_suite = {
	.name = "amd_gpio_irq_unmask_test",
	.test_cases = amd_gpio_irq_unmask_test_cases,
};

kunit_test_suite(amd_gpio_irq_unmask_test_suite);
