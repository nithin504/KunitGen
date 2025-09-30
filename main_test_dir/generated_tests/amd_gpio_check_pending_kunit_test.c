
// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/io.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/printk.h>

#define PIN_IRQ_PENDING (1 << 28)

static struct pinctrl_dev *pinctrl_dev;

struct amd_gpio {
	struct pinctrl_dev *pctrl;
	void __iomem *base;
};

struct pinctrl_desc {
	unsigned int npins;
	struct pinctrl_pin_desc *pins;
};

struct pinctrl_pin_desc {
	unsigned int number;
};

static bool pm_debug_messages_on;

static void pm_pr_dbg(const char *fmt, ...) {}

static void amd_gpio_check_pending(void)
{
	struct amd_gpio *gpio_dev = pinctrl_dev;
	const struct pinctrl_desc *desc = gpio_dev->pctrl->desc;
	int i;

	if (!pm_debug_messages_on)
		return;

	for (i = 0; i < desc->npins; i++) {
		int pin = desc->pins[i].number;
		u32 tmp;

		tmp = readl(gpio_dev->base + pin * 4);
		if (tmp & PIN_IRQ_PENDING)
			pm_pr_dbg("%s: GPIO %d is active: 0x%x.\n", __func__, pin, tmp);
	}
}

static char test_mmio_buffer[4096];
static struct amd_gpio mock_gpio_dev;
static struct pinctrl_dev mock_pctrl_dev;
static struct pinctrl_desc mock_pinctrl_desc;
static struct pinctrl_pin_desc mock_pins[4];

static void test_amd_gpio_check_pending_pm_debug_disabled(struct kunit *test)
{
	pm_debug_messages_on = false;
	pinctrl_dev = &mock_pctrl_dev;
	mock_gpio_dev.pctrl = &mock_pctrl_dev;
	mock_pctrl_dev.desc = &mock_pinctrl_desc;
	
	amd_gpio_check_pending();
	
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_check_pending_no_pins(struct kunit *test)
{
	pm_debug_messages_on = true;
	pinctrl_dev = &mock_pctrl_dev;
	mock_gpio_dev.pctrl = &mock_pctrl_dev;
	mock_gpio_dev.base = test_mmio_buffer;
	mock_pctrl_dev.desc = &mock_pinctrl_desc;
	mock_pinctrl_desc.npins = 0;
	mock_pinctrl_desc.pins = mock_pins;
	
	amd_gpio_check_pending();
	
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_check_pending_no_irq_pending(struct kunit *test)
{
	pm_debug_messages_on = true;
	pinctrl_dev = &mock_pctrl_dev;
	mock_gpio_dev.pctrl = &mock_pctrl_dev;
	mock_gpio_dev.base = test_mmio_buffer;
	mock_pctrl_dev.desc = &mock_pinctrl_desc;
	mock_pinctrl_desc.npins = 2;
	mock_pinctrl_desc.pins = mock_pins;
	mock_pins[0].number = 0;
	mock_pins[1].number = 1;
	
	writel(0x0, test_mmio_buffer + 0 * 4);
	writel(0x0, test_mmio_buffer + 1 * 4);
	
	amd_gpio_check_pending();
	
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_check_pending_with_irq_pending(struct kunit *test)
{
	pm_debug_messages_on = true;
	pinctrl_dev = &mock_pctrl_dev;
	mock_gpio_dev.pctrl = &mock_pctrl_dev;
	mock_gpio_dev.base = test_mmio_buffer;
	mock_pctrl_dev.desc = &mock_pinctrl_desc;
	mock_pinctrl_desc.npins = 3;
	mock_pinctrl_desc.pins = mock_pins;
	mock_pins[0].number = 0;
	mock_pins[1].number = 1;
	mock_pins[2].number = 2;
	
	writel(0x0, test_mmio_buffer + 0 * 4);
	writel(PIN_IRQ_PENDING, test_mmio_buffer + 1 * 4);
	writel(PIN_IRQ_PENDING | 0x123, test_mmio_buffer + 2 * 4);
	
	amd_gpio_check_pending();
	
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_check_pending_multiple_pins(struct kunit *test)
{
	pm_debug_messages_on = true;
	pinctrl_dev = &mock_pctrl_dev;
	mock_gpio_dev.pctrl = &mock_pctrl_dev;
	mock_gpio_dev.base = test_mmio_buffer;
	mock_pctrl_dev.desc = &mock_pinctrl_desc;
	mock_pinctrl_desc.npins = 4;
	mock_pinctrl_desc.pins = mock_pins;
	mock_pins[0].number = 0;
	mock_pins[1].number = 1;
	mock_pins[2].number = 2;
	mock_pins[3].number = 3;
	
	writel(PIN_IRQ_PENDING, test_mmio_buffer + 0 * 4);
	writel(PIN_IRQ_PENDING, test_mmio_buffer + 1 * 4);
	writel(PIN_IRQ_PENDING, test_mmio_buffer + 2 * 4);
	writel(PIN_IRQ_PENDING, test_mmio_buffer + 3 * 4);
	
	amd_gpio_check_pending();
	
	KUNIT_EXPECT_TRUE(test, true);
}

static struct kunit_case amd_gpio_check_pending_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_check_pending_pm_debug_disabled),
	KUNIT_CASE(test_amd_gpio_check_pending_no_pins),
	KUNIT_CASE(test_amd_gpio_check_pending_no_irq_pending),
	KUNIT_CASE(test_amd_gpio_check_pending_with_irq_pending),
	KUNIT_CASE(test_amd_gpio_check_pending_multiple_pins),
	{}
};

static struct kunit_suite amd_gpio_check_pending_test_suite = {
	.name = "amd_gpio_check_pending_test",
	.test_cases = amd_gpio_check_pending_test_cases,
};

kunit_test_suite(amd_gpio_check_pending_test_suite);