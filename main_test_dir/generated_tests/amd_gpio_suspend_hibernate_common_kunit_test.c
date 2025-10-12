// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/spinlock.h>

#define PIN_IRQ_PENDING 0x1
#define INTERRUPT_MASK_OFF 2
#define DB_CNTRl_MASK 0x7
#define DB_CNTRL_OFF 28
#define WAKE_SOURCE_SUSPEND 0x10
#define WAKE_SOURCE_HIBERNATE 0x20

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

static void amd_gpio_set_debounce(struct amd_gpio *gpio_dev, int pin, int debounce)
{
	// Mock implementation
}

#define dev_get_drvdata(dev) ((struct amd_gpio *)dev)

#include "pinctrl-amd.c"

#define TEST_PIN_COUNT 4
#define TEST_BUFFER_SIZE 4096

static struct amd_gpio test_gpio_dev;
static struct pinctrl_desc test_pinctrl_desc;
static struct pinctrl_pin_desc test_pins[TEST_PIN_COUNT];
static u32 test_saved_regs[TEST_PIN_COUNT];
static char test_mmio_buffer[TEST_BUFFER_SIZE];

static void setup_test_environment(struct kunit *test)
{
	memset(&test_gpio_dev, 0, sizeof(test_gpio_dev));
	memset(&test_pinctrl_desc, 0, sizeof(test_pinctrl_desc));
	memset(test_pins, 0, sizeof(test_pins));
	memset(test_saved_regs, 0, sizeof(test_saved_regs));
	memset(test_mmio_buffer, 0, sizeof(test_mmio_buffer));

	test_gpio_dev.base = test_mmio_buffer;
	raw_spin_lock_init(&test_gpio_dev.lock);
	test_gpio_dev.saved_regs = test_saved_regs;

	test_pinctrl_desc.npins = TEST_PIN_COUNT;
	test_pinctrl_desc.pins = test_pins;

	for (int i = 0; i < TEST_PIN_COUNT; i++) {
		test_pins[i].number = i * 4;
	}

	static struct pinctrl_dev pctrl_dev;
	pctrl_dev.desc = &test_pinctrl_desc;
	test_gpio_dev.pctrl = &pctrl_dev;
}

static void test_amd_gpio_suspend_hibernate_common_suspend_no_wake_mask_no_debounce(struct kunit *test)
{
	setup_test_environment(test);

	// Setup registers without wake mask and debounce bits
	for (int i = 0; i < TEST_PIN_COUNT; i++) {
		writel(0x0, test_gpio_dev.base + test_pins[i].number * 4);
	}

	struct device dev;
	int ret = amd_gpio_suspend_hibernate_common(&dev, true);

	KUNIT_EXPECT_EQ(test, ret, 0);

	// Check that interrupt mask bit is cleared
	for (int i = 0; i < TEST_PIN_COUNT; i++) {
		u32 reg_val = readl(test_gpio_dev.base + test_pins[i].number * 4);
		KUNIT_EXPECT_EQ(test, reg_val & BIT(INTERRUPT_MASK_OFF), 0U);
	}
}

static void test_amd_gpio_suspend_hibernate_common_hibernate_with_wake_mask_no_debounce(struct kunit *test)
{
	setup_test_environment(test);

	// Setup registers with hibernate wake mask but no debounce
	for (int i = 0; i < TEST_PIN_COUNT; i++) {
		writel(WAKE_SOURCE_HIBERNATE, test_gpio_dev.base + test_pins[i].number * 4);
	}

	struct device dev;
	int ret = amd_gpio_suspend_hibernate_common(&dev, false);

	KUNIT_EXPECT_EQ(test, ret, 0);

	// Check that registers remain unchanged since they are wake sources
	for (int i = 0; i < TEST_PIN_COUNT; i++) {
		u32 reg_val = readl(test_gpio_dev.base + test_pins[i].number * 4);
		KUNIT_EXPECT_EQ(test, reg_val, WAKE_SOURCE_HIBERNATE);
	}
}

static void test_amd_gpio_suspend_hibernate_common_suspend_with_debounce_clear(struct kunit *test)
{
	setup_test_environment(test);

	// Setup registers with debounce enabled
	for (int i = 0; i < TEST_PIN_COUNT; i++) {
		writel(DB_CNTRl_MASK << DB_CNTRL_OFF, test_gpio_dev.base + test_pins[i].number * 4);
	}

	struct device dev;
	int ret = amd_gpio_suspend_hibernate_common(&dev, true);

	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_suspend_hibernate_common_hibernate_with_both_wake_and_debounce(struct kunit *test)
{
	setup_test_environment(test);

	// Setup registers with both wake source and debounce
	for (int i = 0; i < TEST_PIN_COUNT; i++) {
		writel(WAKE_SOURCE_HIBERNATE | (DB_CNTRl_MASK << DB_CNTRL_OFF),
		       test_gpio_dev.base + test_pins[i].number * 4);
	}

	struct device dev;
	int ret = amd_gpio_suspend_hibernate_common(&dev, false);

	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_suspend_hibernate_common_zero_pins(struct kunit *test)
{
	setup_test_environment(test);
	test_pinctrl_desc.npins = 0;

	struct device dev;
	int ret = amd_gpio_suspend_hibernate_common(&dev, true);

	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_suspend_hibernate_common_pin_not_saved(struct kunit *test)
{
	setup_test_environment(test);

	// Override the should_save function to skip first pin
	typeof(amd_gpio_should_save) *orig_should_save = amd_gpio_should_save;
	amd_gpio_should_save = [](struct amd_gpio *gpio_dev, int pin) -> int {
		return pin != 0;
	};

	// Setup registers
	for (int i = 0; i < TEST_PIN_COUNT; i++) {
		writel(0x0, test_gpio_dev.base + test_pins[i].number * 4);
	}

	struct device dev;
	int ret = amd_gpio_suspend_hibernate_common(&dev, true);

	KUNIT_EXPECT_EQ(test, ret, 0);

	// First pin should remain unchanged, others should have interrupt masked
	u32 reg_val = readl(test_gpio_dev.base + test_pins[0].number * 4);
	KUNIT_EXPECT_EQ(test, reg_val, 0U);

	for (int i = 1; i < TEST_PIN_COUNT; i++) {
		reg_val = readl(test_gpio_dev.base + test_pins[i].number * 4);
		KUNIT_EXPECT_EQ(test, reg_val & BIT(INTERRUPT_MASK_OFF), 0U);
	}

	// Restore original function
	amd_gpio_should_save = orig_should_save;
}

static struct kunit_case amd_gpio_suspend_hibernate_common_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_suspend_no_wake_mask_no_debounce),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_hibernate_with_wake_mask_no_debounce),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_suspend_with_debounce_clear),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_hibernate_with_both_wake_and_debounce),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_zero_pins),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_pin_not_saved),
	{}
};

static struct kunit_suite amd_gpio_suspend_hibernate_common_test_suite = {
	.name = "amd_gpio_suspend_hibernate_common_test",
	.test_cases = amd_gpio_suspend_hibernate_common_test_cases,
};

kunit_test_suite(amd_gpio_suspend_hibernate_common_test_suite);
