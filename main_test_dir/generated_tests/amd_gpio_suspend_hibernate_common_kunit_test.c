```c
// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/spinlock.h>

#define PIN_IRQ_PENDING 0x1
#define INTERRUPT_MASK_OFF 0
#define DB_CNTRL_OFF 28
#define DB_CNTRl_MASK 0x7
#define WAKE_SOURCE_SUSPEND 0x2
#define WAKE_SOURCE_HIBERNATE 0x4

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
	struct pinctrl_dev *pctrl;
	u32 *saved_regs;
};

static int amd_gpio_should_save(struct amd_gpio *gpio_dev, int pin)
{
	return 1;
}

static void amd_gpio_set_debounce(struct amd_gpio *gpio_dev, int pin, u32 debounce)
{
}

#define dev_get_drvdata(dev) ((struct amd_gpio *)dev)

static int amd_gpio_suspend_hibernate_common(struct device *dev, bool is_suspend)
{
	struct amd_gpio *gpio_dev = dev_get_drvdata(dev);
	const struct pinctrl_desc *desc = gpio_dev->pctrl->desc;
	unsigned long flags;
	int i;
	u32 wake_mask = is_suspend ? WAKE_SOURCE_SUSPEND : WAKE_SOURCE_HIBERNATE;

	for (i = 0; i < desc->npins; i++) {
		int pin = desc->pins[i].number;

		if (!amd_gpio_should_save(gpio_dev, pin))
			continue;

		raw_spin_lock_irqsave(&gpio_dev->lock, flags);
		gpio_dev->saved_regs[i] = readl(gpio_dev->base + pin * 4) & ~PIN_IRQ_PENDING;

		/* mask any interrupts not intended to be a wake source */
		if (!(gpio_dev->saved_regs[i] & wake_mask)) {
			writel(gpio_dev->saved_regs[i] & ~BIT(INTERRUPT_MASK_OFF),
			       gpio_dev->base + pin * 4);
		}

		/*
		 * debounce enabled over suspend has shown issues with a GPIO
		 * being unable to wake the system, as we're only interested in
		 * the actual wakeup event, clear it.
		 */
		if (gpio_dev->saved_regs[i] & (DB_CNTRl_MASK << DB_CNTRL_OFF)) {
			amd_gpio_set_debounce(gpio_dev, pin, 0);
		}

		raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);
	}

	return 0;
}

static char test_mmio_buffer[4096];
static struct amd_gpio test_gpio_dev;
static struct pinctrl_desc test_pinctrl_desc;
static struct pinctrl_pin_desc test_pins[4];
static u32 saved_regs_array[4];

static void test_amd_gpio_suspend_hibernate_common_suspend_no_wake_no_debounce(struct kunit *test)
{
	struct device dev;
	int i;

	memset(test_mmio_buffer, 0, sizeof(test_mmio_buffer));
	test_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&test_gpio_dev.lock);
	test_gpio_dev.pctrl = &(struct pinctrl_dev){.desc = &test_pinctrl_desc};
	test_gpio_dev.saved_regs = saved_regs_array;

	test_pinctrl_desc.npins = 2;
	test_pinctrl_desc.pins = test_pins;
	test_pins[0].number = 0;
	test_pins[1].number = 1;

	for (i = 0; i < test_pinctrl_desc.npins; i++) {
		writel(0x100, test_gpio_dev.base + test_pins[i].number * 4);
	}

	dev.driver_data = &test_gpio_dev;
	amd_gpio_suspend_hibernate_common(&dev, true);

	for (i = 0; i < test_pinctrl_desc.npins; i++) {
		u32 reg_val = readl(test_gpio_dev.base + test_pins[i].number * 4);
		KUNIT_EXPECT_EQ(test, reg_val & BIT(INTERRUPT_MASK_OFF), 0U);
		KUNIT_EXPECT_EQ(test, saved_regs_array[i], 0x100U);
	}
}

static void test_amd_gpio_suspend_hibernate_common_hibernate_no_wake_no_debounce(struct kunit *test)
{
	struct device dev;
	int i;

	memset(test_mmio_buffer, 0, sizeof(test_mmio_buffer));
	test_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&test_gpio_dev.lock);
	test_gpio_dev.pctrl = &(struct pinctrl_dev){.desc = &test_pinctrl_desc};
	test_gpio_dev.saved_regs = saved_regs_array;

	test_pinctrl_desc.npins = 2;
	test_pinctrl_desc.pins = test_pins;
	test_pins[0].number = 0;
	test_pins[1].number = 1;

	for (i = 0; i < test_pinctrl_desc.npins; i++) {
		writel(0x200, test_gpio_dev.base + test_pins[i].number * 4);
	}

	dev.driver_data = &test_gpio_dev;
	amd_gpio_suspend_hibernate_common(&dev, false);

	for (i = 0; i < test_pinctrl_desc.npins; i++) {
		u32 reg_val = readl(test_gpio_dev.base + test_pins[i].number * 4);
		KUNIT_EXPECT_EQ(test, reg_val & BIT(INTERRUPT_MASK_OFF), 0U);
		KUNIT_EXPECT_EQ(test, saved_regs_array[i], 0x200U);
	}
}

static void test_amd_gpio_suspend_hibernate_common_suspend_with_wake_source(struct kunit *test)
{
	struct device dev;
	int i;

	memset(test_mmio_buffer, 0, sizeof(test_mmio_buffer));
	test_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&test_gpio_dev.lock);
	test_gpio_dev.pctrl = &(struct pinctrl_dev){.desc = &test_pinctrl_desc};
	test_gpio_dev.saved_regs = saved_regs_array;

	test_pinctrl_desc.npins = 1;
	test_pinctrl_desc.pins = test_pins;
	test_pins[0].number = 0;

	writel(WAKE_SOURCE_SUSPEND, test_gpio_dev.base + test_pins[0].number * 4);

	dev.driver_data = &test_gpio_dev;
	amd_gpio_suspend_hibernate_common(&dev, true);

	u32 reg_val = readl(test_gpio_dev.base + test_pins[0].number * 4);
	KUNIT_EXPECT_NE(test, reg_val & BIT(INTERRUPT_MASK_OFF), 0U);
	KUNIT_EXPECT_EQ(test, saved_regs_array[0], WAKE_SOURCE_SUSPEND);
}

static void test_amd_gpio_suspend_hibernate_common_hibernate_with_wake_source(struct kunit *test)
{
	struct device dev;
	int i;

	memset(test_mmio_buffer, 0, sizeof(test_mmio_buffer));
	test_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&test_gpio_dev.lock);
	test_gpio_dev.pctrl = &(struct pinctrl_dev){.desc = &test_pinctrl_desc};
	test_gpio_dev.saved_regs = saved_regs_array;

	test_pinctrl_desc.npins = 1;
	test_pinctrl_desc.pins = test_pins;
	test_pins[0].number = 0;

	writel(WAKE_SOURCE_HIBERNATE, test_gpio_dev.base + test_pins[0].number * 4);

	dev.driver_data = &test_gpio_dev;
	amd_gpio_suspend_hibernate_common(&dev, false);

	u32 reg_val = readl(test_gpio_dev.base + test_pins[0].number * 4);
	KUNIT_EXPECT_NE(test, reg_val & BIT(INTERRUPT_MASK_OFF), 0U);
	KUNIT_EXPECT_EQ(test, saved_regs_array[0], WAKE_SOURCE_HIBERNATE);
}

static void test_amd_gpio_suspend_hibernate_common_with_debounce_clear(struct kunit *test)
{
	struct device dev;
	int i;
	bool debounce_cleared = false;

	// Override the debounce function for testing
	typeof(amd_gpio_set_debounce) *orig_debounce = amd_gpio_set_debounce;
	amd_gpio_set_debounce = (void (*)(struct amd_gpio *, int, u32))((void (*)(void)){ debounce_cleared = true; });

	memset(test_mmio_buffer, 0, sizeof(test_mmio_buffer));
	test_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&test_gpio_dev.lock);
	test_gpio_dev.pctrl = &(struct pinctrl_dev){.desc = &test_pinctrl_desc};
	test_gpio_dev.saved_regs = saved_regs_array;

	test_pinctrl_desc.npins = 1;
	test_pinctrl_desc.pins = test_pins;
	test_pins[0].number = 0;

	writel((DB_CNTRl_MASK << DB_CNTRL_OFF), test_gpio_dev.base + test_pins[0].number * 4);

	dev.driver_data = &test_gpio_dev;
	amd_gpio_suspend_hibernate_common(&dev, true);

	KUNIT_EXPECT_TRUE(test, debounce_cleared);
	KUNIT_EXPECT_EQ(test, saved_regs_array[0], (DB_CNTRl_MASK << DB_CNTRL_OFF));

	amd_gpio_set_debounce = orig_debounce;
}

static void test_amd_gpio_suspend_hibernate_common_irq_pending_bit_cleared(struct kunit *test)
{
	struct device dev;

	memset(test_mmio_buffer, 0, sizeof(test_mmio_buffer));
	test_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&test_gpio_dev.lock);
	test_gpio_dev.pctrl = &(struct pinctrl_dev){.desc = &test_pinctrl_desc};
	test_gpio_dev.saved_regs = saved_regs_array;

	test_pinctrl_desc.npins = 1;
	test_pinctrl_desc.pins = test_pins;
	test_pins[0].number = 0;

	writel(0x101, test_gpio_dev.base + test_pins[0].number * 4); // 0x1 = PIN_IRQ_PENDING

	dev.driver_data = &test_gpio_dev;
	amd_gpio_suspend_hibernate_common(&dev, true);

	KUNIT_EXPECT_EQ(test, saved_regs_array[0], 0x100U);
}

static struct kunit_case amd_gpio_suspend_hibernate_common_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_suspend_no_wake_no_debounce),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_hibernate_no_wake_no_debounce),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_suspend_with_wake_source),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_hibernate_with_wake_source),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_with_debounce_clear),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_irq_pending_bit_cleared),
	{}
};

static struct kunit_suite amd_gpio_suspend_hibernate_common_test_suite = {
	.name = "amd_gpio_suspend_hibernate_common_test",
	.test_cases = amd_gpio_suspend_hibernate_common_test_cases,
};

kunit_test_suite(amd_gpio_suspend_hibernate_common_test_suite);
```