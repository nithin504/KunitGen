
// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/io.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/spinlock.h>
#include <linux/errno.h>

#define WAKE_CNTRL_OFF_S0I3 0
#define WAKE_CNTRL_OFF_S3 1
#define WAKE_CNTRL_OFF_S4 2

struct pin_desc;
struct pinctrl_dev;
struct pinctrl_desc;

struct amd_gpio {
	struct pinctrl_dev *pctrl;
	void __iomem *base;
	raw_spinlock_t lock;
};

static struct pin_desc *pin_desc_get(struct pinctrl_dev *pctldev, int pin)
{
	return NULL;
}

static void amd_gpio_irq_init(struct amd_gpio *gpio_dev)
{
	const struct pinctrl_desc *desc = gpio_dev->pctrl->desc;
	unsigned long flags;
	u32 pin_reg, mask;
	int i;

	mask = BIT(WAKE_CNTRL_OFF_S0I3) | BIT(WAKE_CNTRL_OFF_S3) |
		BIT(WAKE_CNTRL_OFF_S4);

	for (i = 0; i < desc->npins; i++) {
		int pin = desc->pins[i].number;
		const struct pin_desc *pd = pin_desc_get(gpio_dev->pctrl, pin);

		if (!pd)
			continue;

		raw_spin_lock_irqsave(&gpio_dev->lock, flags);

		pin_reg = readl(gpio_dev->base + pin * 4);
		pin_reg &= ~mask;
		writel(pin_reg, gpio_dev->base + pin * 4);

		raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);
	}
}

static char test_mmio_buffer[4096];
static struct amd_gpio mock_gpio_dev;
static struct pinctrl_desc mock_pinctrl_desc;
static struct pinctrl_dev mock_pinctrl_dev;
static struct pin_desc mock_pin_descs[4];
static struct pinctrl_pin_desc mock_pins[4];

static void test_amd_gpio_irq_init_normal(struct kunit *test)
{
	int i;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.pctrl = &mock_pinctrl_dev;
	mock_gpio_dev.pctrl->desc = &mock_pinctrl_desc;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	mock_pinctrl_desc.npins = 4;
	mock_pinctrl_desc.pins = mock_pins;
	
	for (i = 0; i < 4; i++) {
		mock_pins[i].number = i;
		writel(0xFFFFFFFF, mock_gpio_dev.base + i * 4);
	}
	
	amd_gpio_irq_init(&mock_gpio_dev);
	
	for (i = 0; i < 4; i++) {
		u32 val = readl(mock_gpio_dev.base + i * 4);
		KUNIT_EXPECT_EQ(test, val & (BIT(WAKE_CNTRL_OFF_S0I3) | 
			BIT(WAKE_CNTRL_OFF_S3) | BIT(WAKE_CNTRL_OFF_S4)), 0);
	}
}

static void test_amd_gpio_irq_init_no_pins(struct kunit *test)
{
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.pctrl = &mock_pinctrl_dev;
	mock_gpio_dev.pctrl->desc = &mock_pinctrl_desc;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	mock_pinctrl_desc.npins = 0;
	mock_pinctrl_desc.pins = NULL;
	
	writel(0xFFFFFFFF, mock_gpio_dev.base);
	
	amd_gpio_irq_init(&mock_gpio_dev);
	
	u32 val = readl(mock_gpio_dev.base);
	KUNIT_EXPECT_EQ(test, val, 0xFFFFFFFF);
}

static void test_amd_gpio_irq_init_some_pins_missing(struct kunit *test)
{
	int i;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.pctrl = &mock_pinctrl_dev;
	mock_gpio_dev.pctrl->desc = &mock_pinctrl_desc;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	mock_pinctrl_desc.npins = 4;
	mock_pinctrl_desc.pins = mock_pins;
	
	for (i = 0; i < 4; i++) {
		mock_pins[i].number = i;
		writel(0xFFFFFFFF, mock_gpio_dev.base + i * 4);
	}
	
	amd_gpio_irq_init(&mock_gpio_dev);
	
	for (i = 0; i < 4; i++) {
		u32 val = readl(mock_gpio_dev.base + i * 4);
		KUNIT_EXPECT_EQ(test, val & (BIT(WAKE_CNTRL_OFF_S0I3) | 
			BIT(WAKE_CNTRL_OFF_S3) | BIT(WAKE_CNTRL_OFF_S4)), 0);
	}
}

static void test_amd_gpio_irq_init_already_cleared(struct kunit *test)
{
	int i;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.pctrl = &mock_pinctrl_dev;
	mock_gpio_dev.pctrl->desc = &mock_pinctrl_desc;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	mock_pinctrl_desc.npins = 4;
	mock_pinctrl_desc.pins = mock_pins;
	
	for (i = 0; i < 4; i++) {
		mock_pins[i].number = i;
		writel(0x0, mock_gpio_dev.base + i * 4);
	}
	
	amd_gpio_irq_init(&mock_gpio_dev);
	
	for (i = 0; i < 4; i++) {
		u32 val = readl(mock_gpio_dev.base + i * 4);
		KUNIT_EXPECT_EQ(test, val & (BIT(WAKE_CNTRL_OFF_S0I3) | 
			BIT(WAKE_CNTRL_OFF_S3) | BIT(WAKE_CNTRL_OFF_S4)), 0);
	}
}

static struct kunit_case amd_gpio_irq_init_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_init_normal),
	KUNIT_CASE(test_amd_gpio_irq_init_no_pins),
	KUNIT_CASE(test_amd_gpio_irq_init_some_pins_missing),
	KUNIT_CASE(test_amd_gpio_irq_init_already_cleared),
	{}
};

static struct kunit_suite amd_gpio_irq_init_test_suite = {
	.name = "amd_gpio_irq_init_test",
	.test_cases = amd_gpio_irq_init_test_cases,
};

kunit_test_suite(amd_gpio_irq_init_test_suite);