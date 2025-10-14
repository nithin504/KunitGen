// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/platform_device.h>

// Define necessary constants and macros from the original driver
#define DB_TMR_OUT_MASK 0xFF
#define PULL_DOWN_ENABLE_OFF 1
#define PULL_UP_ENABLE_OFF 0
#define DRV_STRENGTH_SEL_OFF 8
#define DRV_STRENGTH_SEL_MASK 0x3

// Mock structures
struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
	struct platform_device *pdev;
};

// Function under test (copied and made static for direct access)
static int amd_pinconf_get(struct pinctrl_dev *pctldev,
			   unsigned int pin,
			   unsigned long *config)
{
	u32 pin_reg;
	unsigned arg;
	unsigned long flags;
	struct amd_gpio *gpio_dev = pinctrl_dev_get_drvdata(pctldev);
	enum pin_config_param param = pinconf_to_config_param(*config);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + pin * 4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);
	switch (param) {
	case PIN_CONFIG_INPUT_DEBOUNCE:
		arg = pin_reg & DB_TMR_OUT_MASK;
		break;

	case PIN_CONFIG_BIAS_PULL_DOWN:
		arg = (pin_reg >> PULL_DOWN_ENABLE_OFF) & BIT(0);
		break;

	case PIN_CONFIG_BIAS_PULL_UP:
		arg = (pin_reg >> PULL_UP_ENABLE_OFF) & BIT(0);
		break;

	case PIN_CONFIG_DRIVE_STRENGTH:
		arg = (pin_reg >> DRV_STRENGTH_SEL_OFF) & DRV_STRENGTH_SEL_MASK;
		break;

	default:
		dev_dbg(&gpio_dev->pdev->dev, "Invalid config param %04x\n",
			param);
		return -ENOTSUPP;
	}

	*config = pinconf_to_config_packed(param, arg);

	return 0;
}

// Mock data
static struct amd_gpio mock_gpio_dev;
static char mmio_buffer[4096];

// Mock implementation of pinctrl_dev_get_drvdata
void *pinctrl_dev_get_drvdata(struct pinctrl_dev *pctldev)
{
	return &mock_gpio_dev;
}

// Test cases
static void test_amd_pinconf_get_input_debounce(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long config = pinconf_to_config_packed(PIN_CONFIG_INPUT_DEBOUNCE, 0);
	u32 reg_val = 0xAB; // Simulate register value with debounce bits set

	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gpio_dev.pdev = kunit_kzalloc(test, sizeof(*mock_gpio_dev.pdev), GFP_KERNEL);

	writel(reg_val, mock_gpio_dev.base + 0);

	int ret = amd_pinconf_get(&dummy_pctldev, 0, &config);
	KUNIT_EXPECT_EQ(test, ret, 0);

	unsigned long packed_result = pinconf_to_config_packed(PIN_CONFIG_INPUT_DEBOUNCE, reg_val & DB_TMR_OUT_MASK);
	KUNIT_EXPECT_EQ(test, config, packed_result);
}

static void test_amd_pinconf_get_bias_pull_down_enabled(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long config = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_DOWN, 0);
	u32 reg_val = BIT(PULL_DOWN_ENABLE_OFF); // Pull-down enabled

	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gpio_dev.pdev = kunit_kzalloc(test, sizeof(*mock_gpio_dev.pdev), GFP_KERNEL);

	writel(reg_val, mock_gpio_dev.base + 0);

	int ret = amd_pinconf_get(&dummy_pctldev, 0, &config);
	KUNIT_EXPECT_EQ(test, ret, 0);

	unsigned long packed_result = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_DOWN, 1);
	KUNIT_EXPECT_EQ(test, config, packed_result);
}

static void test_amd_pinconf_get_bias_pull_down_disabled(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long config = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_DOWN, 0);
	u32 reg_val = 0; // Pull-down disabled

	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gpio_dev.pdev = kunit_kzalloc(test, sizeof(*mock_gpio_dev.pdev), GFP_KERNEL);

	writel(reg_val, mock_gpio_dev.base + 0);

	int ret = amd_pinconf_get(&dummy_pctldev, 0, &config);
	KUNIT_EXPECT_EQ(test, ret, 0);

	unsigned long packed_result = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_DOWN, 0);
	KUNIT_EXPECT_EQ(test, config, packed_result);
}

static void test_amd_pinconf_get_bias_pull_up_enabled(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long config = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_UP, 0);
	u32 reg_val = BIT(PULL_UP_ENABLE_OFF); // Pull-up enabled

	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gpio_dev.pdev = kunit_kzalloc(test, sizeof(*mock_gpio_dev.pdev), GFP_KERNEL);

	writel(reg_val, mock_gpio_dev.base + 0);

	int ret = amd_pinconf_get(&dummy_pctldev, 0, &config);
	KUNIT_EXPECT_EQ(test, ret, 0);

	unsigned long packed_result = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_UP, 1);
	KUNIT_EXPECT_EQ(test, config, packed_result);
}

static void test_amd_pinconf_get_bias_pull_up_disabled(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long config = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_UP, 0);
	u32 reg_val = 0; // Pull-up disabled

	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gpio_dev.pdev = kunit_kzalloc(test, sizeof(*mock_gpio_dev.pdev), GFP_KERNEL);

	writel(reg_val, mock_gpio_dev.base + 0);

	int ret = amd_pinconf_get(&dummy_pctldev, 0, &config);
	KUNIT_EXPECT_EQ(test, ret, 0);

	unsigned long packed_result = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_UP, 0);
	KUNIT_EXPECT_EQ(test, config, packed_result);
}

static void test_amd_pinconf_get_drive_strength(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long config = pinconf_to_config_packed(PIN_CONFIG_DRIVE_STRENGTH, 0);
	u32 reg_val = (0x2 << DRV_STRENGTH_SEL_OFF); // Drive strength = 2

	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gpio_dev.pdev = kunit_kzalloc(test, sizeof(*mock_gpio_dev.pdev), GFP_KERNEL);

	writel(reg_val, mock_gpio_dev.base + 0);

	int ret = amd_pinconf_get(&dummy_pctldev, 0, &config);
	KUNIT_EXPECT_EQ(test, ret, 0);

	unsigned long packed_result = pinconf_to_config_packed(PIN_CONFIG_DRIVE_STRENGTH, 2);
	KUNIT_EXPECT_EQ(test, config, packed_result);
}

static void test_amd_pinconf_get_invalid_param(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long config = pinconf_to_config_packed((enum pin_config_param)0xFFFF, 0);

	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gpio_dev.pdev = kunit_kzalloc(test, sizeof(*mock_gpio_dev.pdev), GFP_KERNEL);

	int ret = amd_pinconf_get(&dummy_pctldev, 0, &config);
	KUNIT_EXPECT_EQ(test, ret, -ENOTSUPP);
}

static struct kunit_case amd_pinconf_get_test_cases[] = {
	KUNIT_CASE(test_amd_pinconf_get_input_debounce),
	KUNIT_CASE(test_amd_pinconf_get_bias_pull_down_enabled),
	KUNIT_CASE(test_amd_pinconf_get_bias_pull_down_disabled),
	KUNIT_CASE(test_amd_pinconf_get_bias_pull_up_enabled),
	KUNIT_CASE(test_amd_pinconf_get_bias_pull_up_disabled),
	KUNIT_CASE(test_amd_pinconf_get_drive_strength),
	KUNIT_CASE(test_amd_pinconf_get_invalid_param),
	{}
};

static struct kunit_suite amd_pinconf_get_test_suite = {
	.name = "amd_pinconf_get_test",
	.test_cases = amd_pinconf_get_test_cases,
};

kunit_test_suite(amd_pinconf_get_test_suite);
