// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/io.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/spinlock.h>

#define WAKE_CNTRL_OFF_S0I3 10
#define WAKE_CNTRL_OFF_S3   11
#define WAKE_CNTRL_OFF_S4   12

struct amd_gpio {
	void __iomem *base;
	struct pinctrl_dev *pctrl;
	raw_spinlock_t lock;
};

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

static const struct pinctrl_pin_desc test_pins[] = {
	{ .number = 0 },
	{ .number = 1 },
	{ .number = 2 },
};

static struct pinctrl_desc test_pinctrl_desc = {
	.pins = test_pins,
	.npins = ARRAY_SIZE(test_pins),
};

static struct pinctrl_dev test_pctrl_dev = {
	.desc = &test_pinctrl_desc,
};

static const struct pin_desc mock_pin_desc = {};

const struct pin_desc *pin_desc_get(struct pinctrl_dev *pctldev, unsigned int pin)
{
	if (pin < test_pinctrl_desc.npins)
		return &mock_pin_desc;
	return NULL;
}

static char mmio_buffer[4096];

static void test_amd_gpio_irq_init_normal(struct kunit *test)
{
	struct amd_gpio gpio_dev = {
		.base = mmio_buffer,
		.pctrl = &test_pctrl_dev,
		.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock),
	};

	memset(mmio_buffer, 0xFF, sizeof(mmio_buffer));

	amd_gpio_irq_init(&gpio_dev);

	for (int i = 0; i < test_pinctrl_desc.npins; i++) {
		u32 reg_val = readl(gpio_dev.base + test_pins[i].number * 4);
		KUNIT_EXPECT_EQ(test, reg_val & BIT(WAKE_CNTRL_OFF_S0I3), 0U);
		KUNIT_EXPECT_EQ(test, reg_val & BIT(WAKE_CNTRL_OFF_S3), 0U);
		KUNIT_EXPECT_EQ(test, reg_val & BIT(WAKE_CNTRL_OFF_S4), 0U);
	}
}

static void test_amd_gpio_irq_init_null_pin_desc(struct kunit *test)
{
	struct amd_gpio gpio_dev = {
		.base = mmio_buffer,
		.pctrl = &test_pctrl_dev,
		.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock),
	};

	const struct pin_desc *(*orig_pin_desc_get)(struct pinctrl_dev *, unsigned int);
	orig_pin_desc_get = pin_desc_get;

	const struct pin_desc *null_pin_desc_get(struct pinctrl_dev *pctldev, unsigned int pin)
	{
		return NULL;
	}
	*(const struct pin_desc *(**)(struct pinctrl_dev *, unsigned int))((void *)&pin_desc_get) = null_pin_desc_get;

	amd_gpio_irq_init(&gpio_dev);

	*(const struct pin_desc *(**)(struct pinctrl_dev *, unsigned int))((void *)&pin_desc_get) = orig_pin_desc_get;
}

static void test_amd_gpio_irq_init_empty_pins(struct kunit *test)
{
	struct pinctrl_desc empty_desc = {
		.pins = NULL,
		.npins = 0,
	};
	struct pinctrl_dev empty_pctrl_dev = {
		.desc = &empty_desc,
	};
	struct amd_gpio gpio_dev = {
		.base = mmio_buffer,
		.pctrl = &empty_pctrl_dev,
		.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock),
	};

	amd_gpio_irq_init(&gpio_dev);
}

static struct kunit_case amd_gpio_irq_init_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_init_normal),
	KUNIT_CASE(test_amd_gpio_irq_init_null_pin_desc),
	KUNIT_CASE(test_amd_gpio_irq_init_empty_pins),
	{}
};

static struct kunit_suite amd_gpio_irq_init_test_suite = {
	.name = "amd_gpio_irq_init_test",
	.test_cases = amd_gpio_irq_init_test_cases,
};

kunit_test_suite(amd_gpio_irq_init_test_suite);
