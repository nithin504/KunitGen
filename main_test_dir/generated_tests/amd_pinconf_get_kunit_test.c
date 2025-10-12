// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#define DB_TMR_OUT_MASK 0xFF
#define PULL_DOWN_ENABLE_OFF 1
#define PULL_UP_ENABLE_OFF 2
#define DRV_STRENGTH_SEL_OFF 3
#define DRV_STRENGTH_SEL_MASK 0x3

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
	struct platform_device *pdev;
};

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
		dev_dbg(&gpio_dev->pdev->dev, "Invalid config param %04x\n", param);
		return -ENOTSUPP;
	}

	*config = pinconf_to_config_packed(param, arg);

	return 0;
}

static struct amd_gpio mock_gpio_dev;
static char test_mmio_buffer[4096];

static void *mock_pinctrl_dev_get_drvdata(struct pinctrl_dev *pctldev)
{
	return &mock_gpio_dev;
}

#define pinctrl_dev_get_drvdata mock_pinctrl_dev_get_drvdata

static void test_amd_pinconf_get_input_debounce(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long config = pinconf_to_config_packed(PIN_CONFIG_INPUT_DEBOUNCE, 0);
	u32 reg_val = 0xABCD1234;
	unsigned long expected_arg = reg_val & DB_TMR_OUT_MASK;

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(reg_val, mock_gpio_dev.base);

	int ret = amd_pinconf_get(&dummy_pctldev, 0, &config);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, pinconf_to_config_argument(config), expected_arg);
}

static void test_amd_pinconf_get_bias_pull_down_enabled(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long config = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_DOWN, 0);
	u32 reg_val = BIT(PULL_DOWN_ENABLE_OFF);
	unsigned long expected_arg = 1;

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(reg_val, mock_gpio_dev.base);

	int ret = amd_pinconf_get(&dummy_pctldev, 0, &config);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, pinconf_to_config_argument(config), expected_arg);
}

static void test_amd_pinconf_get_bias_pull_down_disabled(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long config = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_DOWN, 0);
	u32 reg_val = 0;
	unsigned long expected_arg = 0;

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(reg_val, mock_gpio_dev.base);

	int ret = amd_pinconf_get(&dummy_pctldev, 0, &config);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, pinconf_to_config_argument(config), expected_arg);
}

static void test_amd_pinconf_get_bias_pull_up_enabled(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long config = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_UP, 0);
	u32 reg_val = BIT(PULL_UP_ENABLE_OFF);
	unsigned long expected_arg = 1;

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(reg_val, mock_gpio_dev.base);

	int ret = amd_pinconf_get(&dummy_pctldev, 0, &config);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, pinconf_to_config_argument(config), expected_arg);
}

static void test_amd_pinconf_get_bias_pull_up_disabled(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long config = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_UP, 0);
	u32 reg_val = 0;
	unsigned long expected_arg = 0;

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(reg_val, mock_gpio_dev.base);

	int ret = amd_pinconf_get(&dummy_pctldev, 0, &config);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, pinconf_to_config_argument(config), expected_arg);
}

static void test_amd_pinconf_get_drive_strength(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long config = pinconf_to_config_packed(PIN_CONFIG_DRIVE_STRENGTH, 0);
	u32 reg_val = (0x2 << DRV_STRENGTH_SEL_OFF);
	unsigned long expected_arg = 0x2;

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(reg_val, mock_gpio_dev.base);

	int ret = amd_pinconf_get(&dummy_pctldev, 0, &config);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, pinconf_to_config_argument(config), expected_arg);
}

static void test_amd_pinconf_get_invalid_param(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long config = pinconf_to_config_packed((enum pin_config_param)0xFFFF, 0);

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gpio_dev.pdev = kunit_kzalloc(test, sizeof(*mock_gpio_dev.pdev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, mock_gpio_dev.pdev);

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
