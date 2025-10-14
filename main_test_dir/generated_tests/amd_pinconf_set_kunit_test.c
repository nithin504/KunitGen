// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/platform_device.h>

// Mock pinctrl_dev_get_drvdata
void *set_mock_pinctrl_dev_get_drvdata(struct pinctrl_dev *pctldev);
#define pinctrl_dev_get_drvdata set_mock_pinctrl_dev_get_drvdata

// Mock dev_dbg to avoid redefinition warnings
#define dev_dbg(dev, fmt, ...) do { } while (0)

// Mock amd_gpio_set_debounce
int amd_gpio_set_debounce(struct amd_gpio *gpio_dev, unsigned int pin, u32 debounce);
#define amd_gpio_set_debounce(...) amd_gpio_set_debounce_mock(__VA_ARGS__)

#include "pinctrl-amd.c"

#define TEST_PIN_INDEX 0
static char test_mmio_buffer[4096];
static struct amd_gpio mock_gpio_dev;
static int amd_gpio_set_debounce_called = 0;
static int amd_gpio_set_debounce_ret = 0;

void *set_mock_pinctrl_dev_get_drvdata(struct pinctrl_dev *pctldev)
{
	return &mock_gpio_dev;
}

int amd_gpio_set_debounce_mock(struct amd_gpio *gpio_dev, unsigned int pin, u32 debounce)
{
	amd_gpio_set_debounce_called++;
	return amd_gpio_set_debounce_ret;
}

// Test debounce case (calls amd_gpio_set_debounce)
static void test_amd_pinconf_set_debounce(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = {
		pinconf_to_config_packed(PIN_CONFIG_INPUT_DEBOUNCE, 0x5)
	};

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	amd_gpio_set_debounce_called = 0;
	amd_gpio_set_debounce_ret = 0;

	writel(0x0, mock_gpio_dev.base + TEST_PIN_INDEX * 4);

	int ret = amd_pinconf_set(&dummy_pctldev, TEST_PIN_INDEX, configs, ARRAY_SIZE(configs));
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, amd_gpio_set_debounce_called, 1);
}

// Test debounce case with error return
static void test_amd_pinconf_set_debounce_error(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = {
		pinconf_to_config_packed(PIN_CONFIG_INPUT_DEBOUNCE, 0x5)
	};

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	amd_gpio_set_debounce_called = 0;
	amd_gpio_set_debounce_ret = -EINVAL;

	writel(0x0, mock_gpio_dev.base + TEST_PIN_INDEX * 4);

	int ret = amd_pinconf_set(&dummy_pctldev, TEST_PIN_INDEX, configs, ARRAY_SIZE(configs));
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
	KUNIT_EXPECT_EQ(test, amd_gpio_set_debounce_called, 1);
}

// Test pull-down case with arg=1
static void test_amd_pinconf_set_pull_down_enable(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = {
		pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_DOWN, 1)
	};

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(0xFFFFFFFF, mock_gpio_dev.base + TEST_PIN_INDEX * 4);

	int ret = amd_pinconf_set(&dummy_pctldev, TEST_PIN_INDEX, configs, ARRAY_SIZE(configs));
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 val = readl(mock_gpio_dev.base + TEST_PIN_INDEX * 4);
	KUNIT_EXPECT_EQ(test, val & BIT(PULL_DOWN_ENABLE_OFF), BIT(PULL_DOWN_ENABLE_OFF));
}

// Test pull-down case with arg=0
static void test_amd_pinconf_set_pull_down_disable(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = {
		pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_DOWN, 0)
	};

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(0xFFFFFFFF, mock_gpio_dev.base + TEST_PIN_INDEX * 4);

	int ret = amd_pinconf_set(&dummy_pctldev, TEST_PIN_INDEX, configs, ARRAY_SIZE(configs));
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 val = readl(mock_gpio_dev.base + TEST_PIN_INDEX * 4);
	KUNIT_EXPECT_EQ(test, val & BIT(PULL_DOWN_ENABLE_OFF), 0);
}

// Test pull-up case with arg=1
static void test_amd_pinconf_set_pull_up_enable(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = {
		pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_UP, 1)
	};

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(0xFFFFFFFF, mock_gpio_dev.base + TEST_PIN_INDEX * 4);

	int ret = amd_pinconf_set(&dummy_pctldev, TEST_PIN_INDEX, configs, ARRAY_SIZE(configs));
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 val = readl(mock_gpio_dev.base + TEST_PIN_INDEX * 4);
	KUNIT_EXPECT_EQ(test, val & BIT(PULL_UP_ENABLE_OFF), BIT(PULL_UP_ENABLE_OFF));
}

// Test pull-up case with arg=0
static void test_amd_pinconf_set_pull_up_disable(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = {
		pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_UP, 0)
	};

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(0xFFFFFFFF, mock_gpio_dev.base + TEST_PIN_INDEX * 4);

	int ret = amd_pinconf_set(&dummy_pctldev, TEST_PIN_INDEX, configs, ARRAY_SIZE(configs));
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 val = readl(mock_gpio_dev.base + TEST_PIN_INDEX * 4);
	KUNIT_EXPECT_EQ(test, val & BIT(PULL_UP_ENABLE_OFF), 0);
}

// Test drive strength case with valid argument
static void test_amd_pinconf_set_drive_strength(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = {
		pinconf_to_config_packed(PIN_CONFIG_DRIVE_STRENGTH, 0x3)
	};

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(0x0, mock_gpio_dev.base + TEST_PIN_INDEX * 4);

	int ret = amd_pinconf_set(&dummy_pctldev, TEST_PIN_INDEX, configs, ARRAY_SIZE(configs));
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 val = readl(mock_gpio_dev.base + TEST_PIN_INDEX * 4);
	KUNIT_EXPECT_EQ(test, (val >> DRV_STRENGTH_SEL_OFF) & DRV_STRENGTH_SEL_MASK, 0x3);
}

// Test drive strength case with masked argument
static void test_amd_pinconf_set_drive_strength_masked(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = {
		pinconf_to_config_packed(PIN_CONFIG_DRIVE_STRENGTH, 0xFF)
	};

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(0x0, mock_gpio_dev.base + TEST_PIN_INDEX * 4);

	int ret = amd_pinconf_set(&dummy_pctldev, TEST_PIN_INDEX, configs, ARRAY_SIZE(configs));
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 val = readl(mock_gpio_dev.base + TEST_PIN_INDEX * 4);
	KUNIT_EXPECT_EQ(test, (val >> DRV_STRENGTH_SEL_OFF) & DRV_STRENGTH_SEL_MASK, DRV_STRENGTH_SEL_MASK);
}

// Test invalid param case
static void test_amd_pinconf_set_invalid_param(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = {
		pinconf_to_config_packed((enum pin_config_param)0xFFFF, 0)
	};

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gpio_dev.pdev = NULL;

	int ret = amd_pinconf_set(&dummy_pctldev, TEST_PIN_INDEX, configs, ARRAY_SIZE(configs));
	KUNIT_EXPECT_EQ(test, ret, -ENOTSUPP);
}

// Test multiple configs processing
static void test_amd_pinconf_set_multiple_configs(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = {
		pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_DOWN, 1),
		pinconf_to_config_packed(PIN_CONFIG_DRIVE_STRENGTH, 0x2),
		pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_UP, 1)
	};

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(0x0, mock_gpio_dev.base + TEST_PIN_INDEX * 4);

	int ret = amd_pinconf_set(&dummy_pctldev, TEST_PIN_INDEX, configs, ARRAY_SIZE(configs));
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 val = readl(mock_gpio_dev.base + TEST_PIN_INDEX * 4);
	KUNIT_EXPECT_EQ(test, val & BIT(PULL_DOWN_ENABLE_OFF), BIT(PULL_DOWN_ENABLE_OFF));
	KUNIT_EXPECT_EQ(test, val & BIT(PULL_UP_ENABLE_OFF), BIT(PULL_UP_ENABLE_OFF));
	KUNIT_EXPECT_EQ(test, (val >> DRV_STRENGTH_SEL_OFF) & DRV_STRENGTH_SEL_MASK, 0x2);
}

// Test debounce in multiple configs (should break early)
static void test_amd_pinconf_set_debounce_multiple_configs(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = {
		pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_DOWN, 1),
		pinconf_to_config_packed(PIN_CONFIG_INPUT_DEBOUNCE, 0x5),
		pinconf_to_config_packed(PIN_CONFIG_DRIVE_STRENGTH, 0x2)
	};

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	amd_gpio_set_debounce_called = 0;
	amd_gpio_set_debounce_ret = 0;

	writel(0x0, mock_gpio_dev.base + TEST_PIN_INDEX * 4);

	int ret = amd_pinconf_set(&dummy_pctldev, TEST_PIN_INDEX, configs, ARRAY_SIZE(configs));
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, amd_gpio_set_debounce_called, 1);

	// Verify that only the first config was processed (debounce breaks the loop)
	u32 val = readl(mock_gpio_dev.base + TEST_PIN_INDEX * 4);
	KUNIT_EXPECT_EQ(test, val & BIT(PULL_DOWN_ENABLE_OFF), BIT(PULL_DOWN_ENABLE_OFF));
}

// Test zero configs
static void test_amd_pinconf_set_zero_configs(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = {};

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	int ret = amd_pinconf_set(&dummy_pctldev, TEST_PIN_INDEX, configs, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

// Test high pin number (edge case)
static void test_amd_pinconf_set_high_pin(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = {
		pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_DOWN, 1)
	};

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(0xFFFFFFFF, mock_gpio_dev.base + 1000 * 4);

	int ret = amd_pinconf_set(&dummy_pctldev, 1000, configs, ARRAY_SIZE(configs));
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 val = readl(mock_gpio_dev.base + 1000 * 4);
	KUNIT_EXPECT_EQ(test, val & BIT(PULL_DOWN_ENABLE_OFF), BIT(PULL_DOWN_ENABLE_OFF));
}

static struct kunit_case amd_pinconf_set_test_cases[] = {
	KUNIT_CASE(test_amd_pinconf_set_debounce),
	KUNIT_CASE(test_amd_pinconf_set_debounce_error),
	KUNIT_CASE(test_amd_pinconf_set_pull_down_enable),
	KUNIT_CASE(test_amd_pinconf_set_pull_down_disable),
	KUNIT_CASE(test_amd_pinconf_set_pull_up_enable),
	KUNIT_CASE(test_amd_pinconf_set_pull_up_disable),
	KUNIT_CASE(test_amd_pinconf_set_drive_strength),
	KUNIT_CASE(test_amd_pinconf_set_drive_strength_masked),
	KUNIT_CASE(test_amd_pinconf_set_invalid_param),
	KUNIT_CASE(test_amd_pinconf_set_multiple_configs),
	KUNIT_CASE(test_amd_pinconf_set_debounce_multiple_configs),
	KUNIT_CASE(test_amd_pinconf_set_zero_configs),
	KUNIT_CASE(test_amd_pinconf_set_high_pin),
	{}
};

static struct kunit_suite amd_pinconf_set_test_suite = {
	.name = "amd_pinconf_set_test",
	.test_cases = amd_pinconf_set_test_cases,
};

kunit_test_suite(amd_pinconf_set_test_suite);