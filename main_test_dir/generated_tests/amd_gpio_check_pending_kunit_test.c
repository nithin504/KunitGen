// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/io.h>
#include <linux/pinctrl/pinctrl.h>

#define PIN_IRQ_PENDING 0x1
#define MOCK_BASE_ADDR 0x1000

// Mock structures and variables
struct amd_gpio {
	void __iomem *base;
	struct pinctrl_dev *pctrl;
};

struct pinctrl_desc {
	const struct pinctrl_pin_desc *pins;
	unsigned int npins;
};

struct pinctrl_pin_desc {
	unsigned number;
	const char *name;
	void *drv_data;
};

static struct amd_gpio mock_gpio_dev;
static struct pinctrl_dev mock_pinctrl_dev;
static struct pinctrl_desc mock_pinctrl_desc;
static char mmio_buffer[4096];
static int pm_debug_messages_on;
static int pm_pr_dbg_call_count;

// Mock function implementations
#define pinctrl_dev (&mock_pinctrl_dev)
#define readl(addr) (*(volatile u32 *)(addr))
#define pm_pr_dbg(fmt, ...) do { \
	if (pm_debug_messages_on) \
		pm_pr_dbg_call_count++; \
} while (0)

// Include the function under test
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

// Test cases
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
	
	// Setup mock data
	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.pctrl = &mock_pinctrl_dev;
	mock_pinctrl_dev.desc = &mock_pinctrl_desc;
	
	struct pinctrl_pin_desc pins[] = {
		{.number = 0},
		{.number = 1}
	};
	mock_pinctrl_desc.pins = pins;
	mock_pinctrl_desc.npins = 2;
	
	// No IRQ pending bits set
	*(u32 *)(mmio_buffer + 0 * 4) = 0x0;
	*(u32 *)(mmio_buffer + 1 * 4) = 0x0;
	
	amd_gpio_check_pending();
	
	KUNIT_EXPECT_EQ(test, pm_pr_dbg_call_count, 0);
}

static void test_amd_gpio_check_pending_single_irq_pending(struct kunit *test)
{
	pm_debug_messages_on = 1;
	pm_pr_dbg_call_count = 0;
	
	// Setup mock data
	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.pctrl = &mock_pinctrl_dev;
	mock_pinctrl_dev.desc = &mock_pinctrl_desc;
	
	struct pinctrl_pin_desc pins[] = {
		{.number = 0},
		{.number = 1}
	};
	mock_pinctrl_desc.pins = pins;
	mock_pinctrl_desc.npins = 2;
	
	// One IRQ pending bit set
	*(u32 *)(mmio_buffer + 0 * 4) = 0x0;
	*(u32 *)(mmio_buffer + 1 * 4) = PIN_IRQ_PENDING;
	
	amd_gpio_check_pending();
	
	KUNIT_EXPECT_EQ(test, pm_pr_dbg_call_count, 1);
}

static void test_amd_gpio_check_pending_multiple_irq_pending(struct kunit *test)
{
	pm_debug_messages_on = 1;
	pm_pr_dbg_call_count = 0;
	
	// Setup mock data
	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.pctrl = &mock_pinctrl_dev;
	mock_pinctrl_dev.desc = &mock_pinctrl_desc;
	
	struct pinctrl_pin_desc pins[] = {
		{.number = 0},
		{.number = 1},
		{.number = 2}
	};
	mock_pinctrl_desc.pins = pins;
	mock_pinctrl_desc.npins = 3;
	
	// Multiple IRQ pending bits set
	*(u32 *)(mmio_buffer + 0 * 4) = PIN_IRQ_PENDING;
	*(u32 *)(mmio_buffer + 1 * 4) = 0x0;
	*(u32 *)(mmio_buffer + 2 * 4) = PIN_IRQ_PENDING;
	
	amd_gpio_check_pending();
	
	KUNIT_EXPECT_EQ(test, pm_pr_dbg_call_count, 2);
}

static void test_amd_gpio_check_pending_zero_pins(struct kunit *test)
{
	pm_debug_messages_on = 1;
	pm_pr_dbg_call_count = 0;
	
	// Setup mock data with zero pins
	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.pctrl = &mock_pinctrl_dev;
	mock_pinctrl_dev.desc = &mock_pinctrl_desc;
	mock_pinctrl_desc.pins = NULL;
	mock_pinctrl_desc.npins = 0;
	
	amd_gpio_check_pending();
	
	KUNIT_EXPECT_EQ(test, pm_pr_dbg_call_count, 0);
}

static struct kunit_case amd_gpio_check_pending_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_check_pending_pm_debug_off),
	KUNIT_CASE(test_amd_gpio_check_pending_no_irq_pending),
	KUNIT_CASE(test_amd_gpio_check_pending_single_irq_pending),
	KUNIT_CASE(test_amd_gpio_check_pending_multiple_irq_pending),
	KUNIT_CASE(test_amd_gpio_check_pending_zero_pins),
	{}
};

static struct kunit_suite amd_gpio_check_pending_test_suite = {
	.name = "amd_gpio_check_pending_test",
	.test_cases = amd_gpio_check_pending_test_cases,
};

kunit_test_suite(amd_gpio_check_pending_test_suite);
