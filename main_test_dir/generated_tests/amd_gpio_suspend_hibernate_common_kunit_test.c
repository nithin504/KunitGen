
// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pm.h>

#define WAKE_SOURCE_SUSPEND 0x1
#define WAKE_SOURCE_HIBERNATE 0x2
#define PIN_IRQ_PENDING 0x80000000
#define INTERRUPT_MASK_OFF 28
#define DB_CNTRL_OFF 28
#define DB_CNTRL_MASK 0x7

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
	struct pinctrl_dev *pctrl;
	u32 *saved_regs;
};

struct pinctrl_desc {
	unsigned int npins;
	struct pinctrl_pin_desc *pins;
};

static bool amd_gpio_should_save(struct amd_gpio *gpio_dev, int pin)
{
	return true;
}

static int amd_gpio_set_debounce(struct amd_gpio *gpio_dev, int pin, unsigned int debounce)
{
	return 0;
}

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

		if (!(gpio_dev->saved_regs[i] & wake_mask)) {
			writel(gpio_dev->saved_regs[i] & ~BIT(INTERRUPT_MASK_OFF),
			       gpio_dev->base + pin * 4);
			pm_pr_dbg("Disabling GPIO #%d interrupt for %s.\n",
				  pin, is_suspend ? "suspend" : "hibernate");
		}

		if (gpio_dev->saved_regs[i] & (DB_CNTRL_MASK << DB_CNTRL_OFF)) {
			amd_gpio_set_debounce(gpio_dev, pin, 0);
			pm_pr_dbg("Clearing debounce for GPIO #%d during %s.\n",
				  pin, is_suspend ? "suspend" : "hibernate");
		}

		raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);
	}

	return 0;
}

static char test_mmio_buffer[4096];
static struct amd_gpio mock_gpio_dev;
static struct pinctrl_desc mock_desc;
static struct pinctrl_pin_desc mock_pins[3];
static u32 saved_regs[3];
static struct device mock_dev;

static void test_amd_gpio_suspend_hibernate_common_no_pins(struct kunit *test)
{
	mock_desc.npins = 0;
	mock_gpio_dev.pctrl = (struct pinctrl_dev *)&mock_desc;
	mock_gpio_dev.saved_regs = saved_regs;
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	int ret = amd_gpio_suspend_hibernate_common(&mock_dev, true);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_suspend_hibernate_common_suspend(struct kunit *test)
{
	mock_desc.npins = 2;
	mock_desc.pins = mock_pins;
	mock_pins[0].number = 0;
	mock_pins[1].number = 1;
	mock_gpio_dev.pctrl = (struct pinctrl_dev *)&mock_desc;
	mock_gpio_dev.saved_regs = saved_regs;
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	writel(WAKE_SOURCE_SUSPEND, test_mmio_buffer + 0);
	writel(0, test_mmio_buffer + 4);

	int ret = amd_gpio_suspend_hibernate_common(&mock_dev, true);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_suspend_hibernate_common_hibernate(struct kunit *test)
{
	mock_desc.npins = 2;
	mock_desc.pins = mock_pins;
	mock_pins[0].number = 0;
	mock_pins[1].number = 1;
	mock_gpio_dev.pctrl = (struct pinctrl_dev *)&mock_desc;
	mock_gpio_dev.saved_regs = saved_regs;
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	writel(WAKE_SOURCE_HIBERNATE, test_mmio_buffer + 0);
	writel(0, test_mmio_buffer + 4);

	int ret = amd_gpio_suspend_hibernate_common(&mock_dev, false);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_suspend_hibernate_common_mask_interrupt(struct kunit *test)
{
	mock_desc.npins = 1;
	mock_desc.pins = mock_pins;
	mock_pins[0].number = 0;
	mock_gpio_dev.pctrl = (struct pinctrl_dev *)&mock_desc;
	mock_gpio_dev.saved_regs = saved_regs;
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	writel(0, test_mmio_buffer + 0);

	int ret = amd_gpio_suspend_hibernate_common(&mock_dev, true);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_suspend_hibernate_common_clear_debounce(struct kunit *test)
{
	mock_desc.npins = 1;
	mock_desc.pins = mock_pins;
	mock_pins[0].number = 0;
	mock_gpio_dev.pctrl = (struct pinctrl_dev *)&mock_desc;
	mock_gpio_dev.saved_regs = saved_regs;
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	writel(DB_CNTRL_MASK << DB_CNTRL_OFF, test_mmio_buffer + 0);

	int ret = amd_gpio_suspend_hibernate_common(&mock_dev, true);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_suspend_hibernate_common_skip_pin(struct kunit *test)
{
	bool (*orig_amd_gpio_should_save)(struct amd_gpio *, int) = amd_gpio_should_save;
	
	mock_desc.npins = 1;
	mock_desc.pins = mock_pins;
	mock_pins[0].number = 0;
	mock_gpio_dev.pctrl = (struct pinctrl_dev *)&mock_desc;
	mock_gpio_dev.saved_regs = saved_regs;
	mock_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	amd_gpio_should_save = NULL;

	int ret = amd_gpio_suspend_hibernate_common(&mock_dev, true);
	KUNIT_EXPECT_EQ(test, ret, 0);

	amd_gpio_should_save = orig_amd_gpio_should_save;
}

static struct kunit_case amd_gpio_suspend_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_no_pins),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_suspend),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_hibernate),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_mask_interrupt),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_clear_debounce),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_skip_pin),
	{}
};

static struct kunit_suite amd_gpio_suspend_test_suite = {
	.name = "amd_gpio_suspend_test",
	.test_cases = amd_gpio_suspend_test_cases,
};

kunit_test_suite(amd_gpio_suspend_test_suite);