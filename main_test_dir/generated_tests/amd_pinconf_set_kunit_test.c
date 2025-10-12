// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#define PIN_INDEX 0
#define PULL_DOWN_ENABLE_OFF 1
#define PULL_UP_ENABLE_OFF 0
#define DRV_STRENGTH_SEL_OFF 9
#define DRV_STRENGTH_SEL_MASK 0x7

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
	struct platform_device *pdev;
};

static int mock_amd_gpio_set_debounce_called = 0;
static int mock_amd_gpio_set_debounce_retval = 0;
static unsigned int mock_amd_gpio_set_debounce_pin = 0;
static unsigned int mock_amd_gpio_set_debounce_arg = 0;

static int amd_gpio_set_debounce(struct amd_gpio *gpio_dev, unsigned int pin, u32 debounce)
{
	mock_amd_gpio_set_debounce_called++;
	mock_amd_gpio_set_debounce_pin = pin;
	mock_amd_gpio_set_debounce_arg = debounce;
	return mock_amd_gpio_set_debounce_retval;
}

#define pinctrl_dev_get_drvdata mock_pinctrl_dev_get_drvdata
static struct amd_gpio *mock_gpio_dev_ptr;

static void *mock_pinctrl_dev_get_drvdata(struct pinctrl_dev *pctldev)
{
	return mock_gpio_dev_ptr;
}

static char mmio_buffer[4096];
static struct amd_gpio mock_gpio_dev_instance;
static struct platform_device mock_pdev;

static int amd_pinconf_set(struct pinctrl_dev *pctldev, unsigned int pin,
			   unsigned long *configs, unsigned int num_configs)
{
	int i;
	u32 arg;
	int ret = 0;
	u32 pin_reg;
	unsigned long flags;
	enum pin_config_param param;
	struct amd_gpio *gpio_dev = pinctrl_dev_get_drvdata(pctldev);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	for (i = 0; i < num_configs; i++) {
		param = pinconf_to_config_param(configs[i]);
		arg = pinconf_to_config_argument(configs[i]);
		pin_reg = readl(gpio_dev->base + pin * 4);

		switch (param) {
		case PIN_CONFIG_INPUT_DEBOUNCE:
			ret = amd_gpio_set_debounce(gpio_dev, pin, arg);
			goto out_unlock;

		case PIN_CONFIG_BIAS_PULL_DOWN:
			pin_reg &= ~BIT(PULL_DOWN_ENABLE_OFF);
			pin_reg |= (arg & BIT(0)) << PULL_DOWN_ENABLE_OFF;
			break;

		case PIN_CONFIG_BIAS_PULL_UP:
			pin_reg &= ~BIT(PULL_UP_ENABLE_OFF);
			pin_reg |= (arg & BIT(0)) << PULL_UP_ENABLE_OFF;
			break;

		case PIN_CONFIG_DRIVE_STRENGTH:
			pin_reg &= ~(DRV_STRENGTH_SEL_MASK << DRV_STRENGTH_SEL_OFF);
			pin_reg |= (arg & DRV_STRENGTH_SEL_MASK) << DRV_STRENGTH_SEL_OFF;
			break;

		default:
			if (gpio_dev->pdev && gpio_dev->pdev->dev.driver)
				dev_dbg(&gpio_dev->pdev->dev, "Invalid config param %04x\n", param);
			ret = -ENOTSUPP;
		}

		writel(pin_reg, gpio_dev->base + pin * 4);
	}
out_unlock:
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);

	return ret;
}

static void test_amd_pinconf_set_debounce_success(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = {
		pinconf_to_config_packed(PIN_CONFIG_INPUT_DEBOUNCE, 100)
	};

	mock_amd_gpio_set_debounce_called = 0;
	mock_amd_gpio_set_debounce_retval = 0;
	mock_gpio_dev_ptr = &mock_gpio_dev_instance;
	mock_gpio_dev_instance.base = mmio_buffer;
	mock_gpio_dev_instance.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev_instance.lock);
	mock_gpio_dev_instance.pdev = &mock_pdev;

	int ret = amd_pinconf_set(&dummy_pctldev, PIN_INDEX, configs, ARRAY_SIZE(configs));

	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, mock_amd_gpio_set_debounce_called, 1);
	KUNIT_EXPECT_EQ(test, mock_amd_gpio_set_debounce_pin, PIN_INDEX);
	KUNIT_EXPECT_EQ(test, mock_amd_gpio_set_debounce_arg, 100U);
}

static void test_amd_pinconf_set_debounce_failure(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = {
		pinconf_to_config_packed(PIN_CONFIG_INPUT_DEBOUNCE, 200)
	};

	mock_amd_gpio_set_debounce_called = 0;
	mock_amd_gpio_set_debounce_retval = -EINVAL;
	mock_gpio_dev_ptr = &mock_gpio_dev_instance;
	mock_gpio_dev_instance.base = mmio_buffer;
	mock_gpio_dev_instance.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev_instance.lock);
	mock_gpio_dev_instance.pdev = &mock_pdev;

	int ret = amd_pinconf_set(&dummy_pctldev, PIN_INDEX, configs, ARRAY_SIZE(configs));

	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
	KUNIT_EXPECT_EQ(test, mock_amd_gpio_set_debounce_called, 1);
	KUNIT_EXPECT_EQ(test, mock_amd_gpio_set_debounce_pin, PIN_INDEX);
	KUNIT_EXPECT_EQ(test, mock_amd_gpio_set_debounce_arg, 200U);
}

static void test_amd_pinconf_set_pull_down_enable(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = {
		pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_DOWN, 1)
	};

	mock_gpio_dev_ptr = &mock_gpio_dev_instance;
	mock_gpio_dev_instance.base = mmio_buffer;
	mock_gpio_dev_instance.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev_instance.lock);
	mock_gpio_dev_instance.pdev = NULL;

	writel(0xFFFFFFFF, mock_gpio_dev_instance.base + PIN_INDEX * 4);

	int ret = amd_pinconf_set(&dummy_pctldev, PIN_INDEX, configs, ARRAY_SIZE(configs));

	KUNIT_EXPECT_EQ(test, ret, 0);
	u32 reg_val = readl(mock_gpio_dev_instance.base + PIN_INDEX * 4);
	KUNIT_EXPECT_NE(test, reg_val & BIT(PULL_DOWN_ENABLE_OFF), 0U);
}

static void test_amd_pinconf_set_pull_down_disable(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = {
		pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_DOWN, 0)
	};

	mock_gpio_dev_ptr = &mock_gpio_dev_instance;
	mock_gpio_dev_instance.base = mmio_buffer;
	mock_gpio_dev_instance.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev_instance.lock);
	mock_gpio_dev_instance.pdev = NULL;

	writel(0xFFFFFFFF, mock_gpio_dev_instance.base + PIN_INDEX * 4);

	int ret = amd_pinconf_set(&dummy_pctldev, PIN_INDEX, configs, ARRAY_SIZE(configs));

	KUNIT_EXPECT_EQ(test, ret, 0);
	u32 reg_val = readl(mock_gpio_dev_instance.base + PIN_INDEX * 4);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(PULL_DOWN_ENABLE_OFF), 0U);
}

static void test_amd_pinconf_set_pull_up_enable(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = {
		pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_UP, 1)
	};

	mock_gpio_dev_ptr = &mock_gpio_dev_instance;
	mock_gpio_dev_instance.base = mmio_buffer;
	mock_gpio_dev_instance.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev_instance.lock);
	mock_gpio_dev_instance.pdev = NULL;

	writel(0x0, mock_gpio_dev_instance.base + PIN_INDEX * 4);

	int ret = amd_pinconf_set(&dummy_pctldev, PIN_INDEX, configs, ARRAY_SIZE(configs));

	KUNIT_EXPECT_EQ(test, ret, 0);
	u32 reg_val = readl(mock_gpio_dev_instance.base + PIN_INDEX * 4);
	KUNIT_EXPECT_NE(test, reg_val & BIT(PULL_UP_ENABLE_OFF), 0U);
}

static void test_amd_pinconf_set_pull_up_disable(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = {
		pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_UP, 0)
	};

	mock_gpio_dev_ptr = &mock_gpio_dev_instance;
	mock_gpio_dev_instance.base = mmio_buffer;
	mock_gpio_dev_instance.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev_instance.lock);
	mock_gpio_dev_instance.pdev = NULL;

	writel(0xFFFFFFFF, mock_gpio_dev_instance.base + PIN_INDEX * 4);

	int ret = amd_pinconf_set(&dummy_pctldev, PIN_INDEX, configs, ARRAY_SIZE(configs));

	KUNIT_EXPECT_EQ(test, ret, 0);
	u32 reg_val = readl(mock_gpio_dev_instance.base + PIN_INDEX * 4);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(PULL_UP_ENABLE_OFF), 0U);
}

static void test_amd_pinconf_set_drive_strength(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = {
		pinconf_to_config_packed(PIN_CONFIG_DRIVE_STRENGTH, 5)
	};

	mock_gpio_dev_ptr = &mock_gpio_dev_instance;
	mock_gpio_dev_instance.base = mmio_buffer;
	mock_gpio_dev_instance.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev_instance.lock);
	mock_gpio_dev_instance.pdev = NULL;

	writel(0x0, mock_gpio_dev_instance.base + PIN_INDEX * 4);

	int ret = amd_pinconf_set(&dummy_pctldev, PIN_INDEX, configs, ARRAY_SIZE(configs));

	KUNIT_EXPECT_EQ(test, ret, 0);
	u32 reg_val = readl(mock_gpio_dev_instance.base + PIN_INDEX * 4);
	KUNIT_EXPECT_EQ(test, (reg_val >> DRV_STRENGTH_SEL_OFF) & DRV_STRENGTH_SEL_MASK, 5U);
}

static void test_amd_pinconf_set_invalid_param(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = {
		pinconf_to_config_packed((enum pin_config_param)0xFFFF, 0)
	};

	mock_gpio_dev_ptr = &mock_gpio_dev_instance;
	mock_gpio_dev_instance.base = mmio_buffer;
	mock_gpio_dev_instance.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev_instance.lock);
	mock_gpio_dev_instance.pdev = &mock_pdev;

	int ret = amd_pinconf_set(&dummy_pctldev, PIN_INDEX, configs, ARRAY_SIZE(configs));

	KUNIT_EXPECT_EQ(test, ret, -ENOTSUPP);
}

static void test_amd_pinconf_set_multiple_configs(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long configs[] = {
		pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_DOWN, 1),
		pinconf_to_config_packed(PIN_CONFIG_DRIVE_STRENGTH, 3)
	};

	mock_gpio_dev_ptr = &mock_gpio_dev_instance;
	mock_gpio_dev_instance.base = mmio_buffer;
	mock_gpio_dev_instance.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev_instance.lock);
	mock_gpio_dev_instance.pdev = NULL;

	writel(0x0, mock_gpio_dev_instance.base + PIN_INDEX * 4);

	int ret = amd_pinconf_set(&dummy_pctldev, PIN_INDEX, configs, ARRAY_SIZE(configs));

	KUNIT_EXPECT_EQ(test, ret, 0);
	u32 reg_val = readl(mock_gpio_dev_instance.base + PIN_INDEX * 4);
	KUNIT_EXPECT_NE(test, reg_val & BIT(PULL_DOWN_ENABLE_OFF), 0U);
	KUNIT_EXPECT_EQ(test, (reg_val >> DRV_STRENGTH_SEL_OFF) & DRV_STRENGTH_SEL_MASK, 3U);
}

static void test_amd_pinconf_set_empty_configs(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long *configs = NULL;

	mock_gpio_dev_ptr = &mock_gpio_dev_instance;
	mock_gpio_dev_instance.base = mmio_buffer;
	mock_gpio_dev_instance.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev_instance.lock);
	mock_gpio_dev_instance.pdev = NULL;

	int ret = amd_pinconf_set(&dummy_pctldev, PIN_INDEX, configs, 0);

	KUNIT_EXPECT_EQ(test, ret, 0);
}

static struct kunit_case amd_pinconf_set_test_cases[] = {
	KUNIT_CASE(test_amd_pinconf_set_debounce_success),
	KUNIT_CASE(test_amd_pinconf_set_debounce_failure),
	KUNIT_CASE(test_amd_pinconf_set_pull_down_enable),
	KUNIT_CASE(test_amd_pinconf_set_pull_down_disable),
	KUNIT_CASE(test_amd_pinconf_set_pull_up_enable),
	KUNIT_CASE(test_amd_pinconf_set_pull_up_disable),
	KUNIT_CASE(test_amd_pinconf_set_drive_strength),
	KUNIT_CASE(test_amd_pinconf_set_invalid_param),
	KUNIT_CASE(test_amd_pinconf_set_multiple_configs),
	KUNIT_CASE(test_amd_pinconf_set_empty_configs),
	{}
};

static struct kunit_suite amd_pinconf_set_test_suite = {
	.name = "amd_pinconf_set_test",
	.test_cases = amd_pinconf_set_test_cases,
};

kunit_test_suite(amd_pinconf_set_test_suite);
