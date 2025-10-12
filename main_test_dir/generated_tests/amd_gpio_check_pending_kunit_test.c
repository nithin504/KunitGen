// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/io.h>
#include <linux/pinctrl/pinctrl.h>

#define PIN_IRQ_PENDING 0x1
#define MOCK_BASE_ADDR 0x1000

struct amd_gpio {
	void __iomem *base;
	struct pinctrl_dev *pctrl;
};

static struct amd_gpio mock_gpio_dev;
static struct pinctrl_desc mock_pinctrl_desc;
static struct pinctrl_dev mock_pinctrl_dev;
static char mmio_buffer[4096];
static int pm_debug_messages_on;
static int pm_pr_dbg_call_count;

void pm_pr_dbg(const char *fmt, ...) {
	pm_pr_dbg_call_count++;
}

#define pinctrl_dev (&mock_pinctrl_dev)

#include "pinctrl-amd.c"

static void test_amd_gpio_check_pending_pm_debug_off(struct kunit *test)
{
	pm_debug_messages_on = 0;
	pm_pr_dbg_call_count = 0;

	amd_gpio_check_pending();

	KUNIT_EXPECT_EQ(test, pm_pr_dbg_call_count, 0);
}

static void test_amd_gpio_check_pending_no_irq_pending(struct kunit *test)
{
	pm_debug_messages_on = 1;
	pm_pr_dbg_call_count = 0;
	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.pctrl = &mock_pinctrl_dev;
	mock_pinctrl_dev.desc = &mock_pinctrl_desc;

	static struct pinctrl_pin_desc pins[] = {
		{ .number = 0 },
		{ .number = 1 },
	};
	mock_pinctrl_desc.pins = pins;
	mock_pinctrl_desc.npins = 2;

	memset(mmio_buffer, 0, sizeof(mmio_buffer));

	amd_gpio_check_pending();

	KUNIT_EXPECT_EQ(test, pm_pr_dbg_call_count, 0);
}

static void test_amd_gpio_check_pending_single_irq_pending(struct kunit *test)
{
	pm_debug_messages_on = 1;
	pm_pr_dbg_call_count = 0;
	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.pctrl = &mock_pinctrl_dev;
	mock_pinctrl_dev.desc = &mock_pinctrl_desc;

	static struct pinctrl_pin_desc pins[] = {
		{ .number = 0 },
		{ .number = 1 },
	};
	mock_pinctrl_desc.pins = pins;
	mock_pinctrl_desc.npins = 2;

	memset(mmio_buffer, 0, sizeof(mmio_buffer));
	writel(PIN_IRQ_PENDING, mock_gpio_dev.base + 0 * 4);

	amd_gpio_check_pending();

	KUNIT_EXPECT_EQ(test, pm_pr_dbg_call_count, 1);
}

static void test_amd_gpio_check_pending_multiple_irq_pending(struct kunit *test)
{
	pm_debug_messages_on = 1;
	pm_pr_dbg_call_count = 0;
	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.pctrl = &mock_pinctrl_dev;
	mock_pinctrl_dev.desc = &mock_pinctrl_desc;

	static struct pinctrl_pin_desc pins[] = {
		{ .number = 0 },
		{ .number = 1 },
		{ .number = 2 },
	};
	mock_pinctrl_desc.pins = pins;
	mock_pinctrl_desc.npins = 3;

	memset(mmio_buffer, 0, sizeof(mmio_buffer));
	writel(PIN_IRQ_PENDING, mock_gpio_dev.base + 0 * 4);
	writel(0, mock_gpio_dev.base + 1 * 4);
	writel(PIN_IRQ_PENDING, mock_gpio_dev.base + 2 * 4);

	amd_gpio_check_pending();

	KUNIT_EXPECT_EQ(test, pm_pr_dbg_call_count, 2);
}

static void test_amd_gpio_check_pending_all_irq_pending(struct kunit *test)
{
	pm_debug_messages_on = 1;
	pm_pr_dbg_call_count = 0;
	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.pctrl = &mock_pinctrl_dev;
	mock_pinctrl_dev.desc = &mock_pinctrl_desc;

	static struct pinctrl_pin_desc pins[] = {
		{ .number = 0 },
		{ .number = 1 },
	};
	mock_pinctrl_desc.pins = pins;
	mock_pinctrl_desc.npins = 2;

	memset(mmio_buffer, 0, sizeof(mmio_buffer));
	writel(PIN_IRQ_PENDING, mock_gpio_dev.base + 0 * 4);
	writel(PIN_IRQ_PENDING, mock_gpio_dev.base + 1 * 4);

	amd_gpio_check_pending();

	KUNIT_EXPECT_EQ(test, pm_pr_dbg_call_count, 2);
}

static struct kunit_case amd_gpio_check_pending_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_check_pending_pm_debug_off),
	KUNIT_CASE(test_amd_gpio_check_pending_no_irq_pending),
	KUNIT_CASE(test_amd_gpio_check_pending_single_irq_pending),
	KUNIT_CASE(test_amd_gpio_check_pending_multiple_irq_pending),
	KUNIT_CASE(test_amd_gpio_check_pending_all_irq_pending),
	{}
};

static struct kunit_suite amd_gpio_check_pending_test_suite = {
	.name = "amd_gpio_check_pending_test",
	.test_cases = amd_gpio_check_pending_test_cases,
};

kunit_test_suite(amd_gpio_check_pending_test_suite);
