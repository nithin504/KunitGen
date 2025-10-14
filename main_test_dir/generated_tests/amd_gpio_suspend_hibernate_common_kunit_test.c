// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/spinlock.h>

// Mock definitions
#define PIN_IRQ_PENDING 0x100
#define INTERRUPT_MASK_OFF 2
#define WAKE_SOURCE_SUSPEND 0x1
#define WAKE_SOURCE_HIBERNATE 0x2
#define DB_CNTRl_MASK 0x7
#define DB_CNTRL_OFF 28

// Forward declarations
struct amd_gpio;
static int amd_gpio_should_save(struct amd_gpio *gpio_dev, int pin);
static void amd_gpio_set_debounce(struct amd_gpio *gpio_dev, int pin, int value);

#include "pinctrl-amd.c"

// Mock implementations
static int should_save_ret = 1;
static int set_debounce_called = 0;
static int last_pin_passed = -1;
static int last_debounce_value = -1;
static u32 *mock_mmio_base;
static int mock_mmio_size = 8192;

static int amd_gpio_should_save(struct amd_gpio *gpio_dev, int pin)
{
	return should_save_ret;
}

static void amd_gpio_set_debounce(struct amd_gpio *gpio_dev, int pin, int value)
{
	set_debounce_called++;
	last_pin_passed = pin;
	last_debounce_value = value;
}

// Test setup helpers
static void setup_mock_gpio_device(struct kunit *test, struct amd_gpio *gpio_dev, int npins)
{
	gpio_dev->base = kunit_kzalloc(test, mock_mmio_size, GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev->base);
	mock_mmio_base = (u32 *)gpio_dev->base;

	gpio_dev->pctrl = kunit_kzalloc(test, sizeof(*(gpio_dev->pctrl)), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev->pctrl);

	gpio_dev->pctrl->desc = kunit_kzalloc(test, sizeof(*(gpio_dev->pctrl->desc)), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev->pctrl->desc);

	gpio_dev->pctrl->desc->pins = kunit_kzalloc(test,
						    sizeof(*(gpio_dev->pctrl->desc->pins)) * npins,
						    GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev->pctrl->desc->pins);

	gpio_dev->pctrl->desc->npins = npins;
	gpio_dev->saved_regs = kunit_kzalloc(test, sizeof(u32) * npins, GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev->saved_regs);

	raw_spin_lock_init(&gpio_dev->lock);

	// Initialize pins
	for (int i = 0; i < npins; i++) {
		gpio_dev->pctrl->desc->pins[i].number = i;
		mock_mmio_base[i] = 0x0; // Initial register value
	}
}

static struct device *create_mock_device(struct kunit *test, struct amd_gpio *gpio_dev)
{
	struct device *dev = kunit_kzalloc(test, sizeof(*dev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, dev);
	dev_set_drvdata(dev, gpio_dev);
	return dev;
}

// Test cases
static void test_amd_gpio_suspend_hibernate_common_suspend_no_pins(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	struct device *dev;

	setup_mock_gpio_device(test, &gpio_dev, 0);
	dev = create_mock_device(test, &gpio_dev);

	int ret = amd_gpio_suspend_hibernate_common(dev, true);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_suspend_hibernate_common_hibernate_no_pins(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	struct device *dev;

	setup_mock_gpio_device(test, &gpio_dev, 0);
	dev = create_mock_device(test, &gpio_dev);

	int ret = amd_gpio_suspend_hibernate_common(dev, false);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_suspend_hibernate_common_suspend_all_pins_should_not_save(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	struct device *dev;

	should_save_ret = 0; // No pins should be saved
	setup_mock_gpio_device(test, &gpio_dev, 5);
	dev = create_mock_device(test, &gpio_dev);

	int ret = amd_gpio_suspend_hibernate_common(dev, true);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, set_debounce_called, 0);
}

static void test_amd_gpio_suspend_hibernate_common_hibernate_all_pins_should_not_save(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	struct device *dev;

	should_save_ret = 0; // No pins should be saved
	setup_mock_gpio_device(test, &gpio_dev, 5);
	dev = create_mock_device(test, &gpio_dev);

	int ret = amd_gpio_suspend_hibernate_common(dev, false);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, set_debounce_called, 0);
}

static void test_amd_gpio_suspend_hibernate_common_suspend_pin_not_wake_source_no_debounce(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	struct device *dev;
	int pin_index = 2;

	should_save_ret = 1;
	setup_mock_gpio_device(test, &gpio_dev, 5);
	dev = create_mock_device(test, &gpio_dev);

	// Configure pin to not be a wake source and no debounce
	mock_mmio_base[pin_index] = 0x0;

	int ret = amd_gpio_suspend_hibernate_common(dev, true);
	KUNIT_EXPECT_EQ(test, ret, 0);

	// Check that interrupt mask bit is cleared
	u32 reg_val = mock_mmio_base[pin_index];
	KUNIT_EXPECT_EQ(test, (reg_val & BIT(INTERRUPT_MASK_OFF)), 0U);

	// Check that debounce was not cleared
	KUNIT_EXPECT_EQ(test, set_debounce_called, 0);
}

static void test_amd_gpio_suspend_hibernate_common_hibernate_pin_not_wake_source_no_debounce(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	struct device *dev;
	int pin_index = 3;

	should_save_ret = 1;
	setup_mock_gpio_device(test, &gpio_dev, 5);
	dev = create_mock_device(test, &gpio_dev);

	// Configure pin to not be a wake source and no debounce
	mock_mmio_base[pin_index] = 0x0;

	int ret = amd_gpio_suspend_hibernate_common(dev, false);
	KUNIT_EXPECT_EQ(test, ret, 0);

	// Check that interrupt mask bit is cleared
	u32 reg_val = mock_mmio_base[pin_index];
	KUNIT_EXPECT_EQ(test, (reg_val & BIT(INTERRUPT_MASK_OFF)), 0U);

	// Check that debounce was not cleared
	KUNIT_EXPECT_EQ(test, set_debounce_called, 0);
}

static void test_amd_gpio_suspend_hibernate_common_suspend_pin_is_wake_source_no_debounce(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	struct device *dev;
	int pin_index = 1;

	should_save_ret = 1;
	setup_mock_gpio_device(test, &gpio_dev, 5);
	dev = create_mock_device(test, &gpio_dev);

	// Configure pin to be a wake source and no debounce
	mock_mmio_base[pin_index] = WAKE_SOURCE_SUSPEND;

	int ret = amd_gpio_suspend_hibernate_common(dev, true);
	KUNIT_EXPECT_EQ(test, ret, 0);

	// Check that register value remains unchanged
	u32 reg_val = mock_mmio_base[pin_index];
	KUNIT_EXPECT_EQ(test, reg_val, WAKE_SOURCE_SUSPEND);

	// Check that debounce was not cleared
	KUNIT_EXPECT_EQ(test, set_debounce_called, 0);
}

static void test_amd_gpio_suspend_hibernate_common_hibernate_pin_is_wake_source_no_debounce(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	struct device *dev;
	int pin_index = 4;

	should_save_ret = 1;
	setup_mock_gpio_device(test, &gpio_dev, 5);
	dev = create_mock_device(test, &gpio_dev);

	// Configure pin to be a wake source and no debounce
	mock_mmio_base[pin_index] = WAKE_SOURCE_HIBERNATE;

	int ret = amd_gpio_suspend_hibernate_common(dev, false);
	KUNIT_EXPECT_EQ(test, ret, 0);

	// Check that register value remains unchanged
	u32 reg_val = mock_mmio_base[pin_index];
	KUNIT_EXPECT_EQ(test, reg_val, WAKE_SOURCE_HIBERNATE);

	// Check that debounce was not cleared
	KUNIT_EXPECT_EQ(test, set_debounce_called, 0);
}

static void test_amd_gpio_suspend_hibernate_common_suspend_pin_not_wake_source_has_debounce(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	struct device *dev;
	int pin_index = 0;

	should_save_ret = 1;
	set_debounce_called = 0;
	last_pin_passed = -1;
	last_debounce_value = -1;

	setup_mock_gpio_device(test, &gpio_dev, 5);
	dev = create_mock_device(test, &gpio_dev);

	// Configure pin to not be a wake source but has debounce
	mock_mmio_base[pin_index] = (DB_CNTRl_MASK << DB_CNTRL_OFF);

	int ret = amd_gpio_suspend_hibernate_common(dev, true);
	KUNIT_EXPECT_EQ(test, ret, 0);

	// Check that interrupt mask bit is cleared
	u32 reg_val = mock_mmio_base[pin_index];
	KUNIT_EXPECT_EQ(test, (reg_val & BIT(INTERRUPT_MASK_OFF)), 0U);

	// Check that debounce was cleared
	KUNIT_EXPECT_EQ(test, set_debounce_called, 1);
	KUNIT_EXPECT_EQ(test, last_pin_passed, pin_index);
	KUNIT_EXPECT_EQ(test, last_debounce_value, 0);
}

static void test_amd_gpio_suspend_hibernate_common_hibernate_pin_not_wake_source_has_debounce(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	struct device *dev;
	int pin_index = 2;

	should_save_ret = 1;
	set_debounce_called = 0;
	last_pin_passed = -1;
	last_debounce_value = -1;

	setup_mock_gpio_device(test, &gpio_dev, 5);
	dev = create_mock_device(test, &gpio_dev);

	// Configure pin to not be a wake source but has debounce
	mock_mmio_base[pin_index] = (DB_CNTRl_MASK << DB_CNTRL_OFF);

	int ret = amd_gpio_suspend_hibernate_common(dev, false);
	KUNIT_EXPECT_EQ(test, ret, 0);

	// Check that interrupt mask bit is cleared
	u32 reg_val = mock_mmio_base[pin_index];
	KUNIT_EXPECT_EQ(test, (reg_val & BIT(INTERRUPT_MASK_OFF)), 0U);

	// Check that debounce was cleared
	KUNIT_EXPECT_EQ(test, set_debounce_called, 1);
	KUNIT_EXPECT_EQ(test, last_pin_passed, pin_index);
	KUNIT_EXPECT_EQ(test, last_debounce_value, 0);
}

static void test_amd_gpio_suspend_hibernate_common_suspend_pin_is_wake_source_has_debounce(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	struct device *dev;
	int pin_index = 3;

	should_save_ret = 1;
	set_debounce_called = 0;
	last_pin_passed = -1;
	last_debounce_value = -1;

	setup_mock_gpio_device(test, &gpio_dev, 5);
	dev = create_mock_device(test, &gpio_dev);

	// Configure pin to be a wake source and has debounce
	mock_mmio_base[pin_index] = WAKE_SOURCE_SUSPEND | (DB_CNTRl_MASK << DB_CNTRL_OFF);

	int ret = amd_gpio_suspend_hibernate_common(dev, true);
	KUNIT_EXPECT_EQ(test, ret, 0);

	// Check that register value remains unchanged
	u32 reg_val = mock_mmio_base[pin_index];
	KUNIT_EXPECT_EQ(test, reg_val, WAKE_SOURCE_SUSPEND | (DB_CNTRl_MASK << DB_CNTRL_OFF));

	// Check that debounce was cleared
	KUNIT_EXPECT_EQ(test, set_debounce_called, 1);
	KUNIT_EXPECT_EQ(test, last_pin_passed, pin_index);
	KUNIT_EXPECT_EQ(test, last_debounce_value, 0);
}

static void test_amd_gpio_suspend_hibernate_common_hibernate_pin_is_wake_source_has_debounce(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	struct device *dev;
	int pin_index = 1;

	should_save_ret = 1;
	set_debounce_called = 0;
	last_pin_passed = -1;
	last_debounce_value = -1;

	setup_mock_gpio_device(test, &gpio_dev, 5);
	dev = create_mock_device(test, &gpio_dev);

	// Configure pin to be a wake source and has debounce
	mock_mmio_base[pin_index] = WAKE_SOURCE_HIBERNATE | (DB_CNTRl_MASK << DB_CNTRL_OFF);

	int ret = amd_gpio_suspend_hibernate_common(dev, false);
	KUNIT_EXPECT_EQ(test, ret, 0);

	// Check that register value remains unchanged
	u32 reg_val = mock_mmio_base[pin_index];
	KUNIT_EXPECT_EQ(test, reg_val, WAKE_SOURCE_HIBERNATE | (DB_CNTRl_MASK << DB_CNTRL_OFF));

	// Check that debounce was cleared
	KUNIT_EXPECT_EQ(test, set_debounce_called, 1);
	KUNIT_EXPECT_EQ(test, last_pin_passed, pin_index);
	KUNIT_EXPECT_EQ(test, last_debounce_value, 0);
}

static void test_amd_gpio_suspend_hibernate_common_multiple_pins_mixed_conditions(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	struct device *dev;

	should_save_ret = 1;
	set_debounce_called = 0;

	setup_mock_gpio_device(test, &gpio_dev, 4);
	dev = create_mock_device(test, &gpio_dev);

	// Pin 0: Not wake source, no debounce
	mock_mmio_base[0] = 0x0;
	// Pin 1: Wake source, no debounce
	mock_mmio_base[1] = WAKE_SOURCE_SUSPEND;
	// Pin 2: Not wake source, has debounce
	mock_mmio_base[2] = (DB_CNTRl_MASK << DB_CNTRL_OFF);
	// Pin 3: Wake source, has debounce
	mock_mmio_base[3] = WAKE_SOURCE_HIBERNATE | (DB_CNTRl_MASK << DB_CNTRL_OFF);

	int ret = amd_gpio_suspend_hibernate_common(dev, false);
	KUNIT_EXPECT_EQ(test, ret, 0);

	// Check pin 0: interrupt masked
	KUNIT_EXPECT_EQ(test, (mock_mmio_base[0] & BIT(INTERRUPT_MASK_OFF)), 0U);
	// Check pin 1: unchanged
	KUNIT_EXPECT_EQ(test, mock_mmio_base[1], WAKE_SOURCE_SUSPEND);
	// Check pin 2: interrupt masked, debounce cleared
	KUNIT_EXPECT_EQ(test, (mock_mmio_base[2] & BIT(INTERRUPT_MASK_OFF)), 0U);
	KUNIT_EXPECT_EQ(test, set_debounce_called, 1);
	// Check pin 3: unchanged, debounce cleared
	KUNIT_EXPECT_EQ(test, mock_mmio_base[3], WAKE_SOURCE_HIBERNATE | (DB_CNTRl_MASK << DB_CNTRL_OFF));
	KUNIT_EXPECT_EQ(test, set_debounce_called, 2);
}

static struct kunit_case amd_gpio_suspend_hibernate_common_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_suspend_no_pins),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_hibernate_no_pins),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_suspend_all_pins_should_not_save),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_hibernate_all_pins_should_not_save),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_suspend_pin_not_wake_source_no_debounce),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_hibernate_pin_not_wake_source_no_debounce),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_suspend_pin_is_wake_source_no_debounce),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_hibernate_pin_is_wake_source_no_debounce),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_suspend_pin_not_wake_source_has_debounce),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_hibernate_pin_not_wake_source_has_debounce),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_suspend_pin_is_wake_source_has_debounce),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_hibernate_pin_is_wake_source_has_debounce),
	KUNIT_CASE(test_amd_gpio_suspend_hibernate_common_multiple_pins_mixed_conditions),
	{}
};

static struct kunit_suite amd_gpio_suspend_hibernate_common_test_suite = {
	.name = "amd_gpio_suspend_hibernate_common_test",
	.test_cases = amd_gpio_suspend_hibernate_common_test_cases,
};

kunit_test_suite(amd_gpio_suspend_hibernate_common_test_suite);