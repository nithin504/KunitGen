#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#define DB_TMR_OUT_MASK 0xFF
#define PULL_DOWN_ENABLE_OFF 0
#define PULL_UP_ENABLE_OFF 1
#define DRV_STRENGTH_SEL_OFF 2
#define DRV_STRENGTH_SEL_MASK 0x3

static struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
	struct platform_device *pdev;
} mock_gpio_dev;

static char test_mmio_buffer[4096];

void *pinctrl_dev_get_drvdata_mock(struct pinctrl_dev *pctldev)
{
	return &mock_gpio_dev;
}

#define pinctrl_dev_get_drvdata pinctrl_dev_get_drvdata_mock

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
	pin_reg = readl(gpio_dev->base + pin*4);
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

static void test_amd_pinconf_get_debounce(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long config = pinconf_to_config_packed(PIN_CONFIG_INPUT_DEBOUNCE, 0);
	unsigned int pin = 0;

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(0xAB, mock_gpio_dev.base + pin * 4);

	int ret = amd_pinconf_get(&dummy_pctldev, pin, &config);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, pinconf_to_config_param(config), PIN_CONFIG_INPUT_DEBOUNCE);
	KUNIT_EXPECT_EQ(test, pinconf_to_config_argument(config), 0xAB);
}

static void test_amd_pinconf_get_pull_down_enabled(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long config = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_DOWN, 0);
	unsigned int pin = 1;

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(BIT(PULL_DOWN_ENABLE_OFF), mock_gpio_dev.base + pin * 4);

	int ret = amd_pinconf_get(&dummy_pctldev, pin, &config);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, pinconf_to_config_param(config), PIN_CONFIG_BIAS_PULL_DOWN);
	KUNIT_EXPECT_EQ(test, pinconf_to_config_argument(config), 1);
}

static void test_amd_pinconf_get_pull_down_disabled(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long config = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_DOWN, 0);
	unsigned int pin = 2;

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(0x0, mock_gpio_dev.base + pin * 4);

	int ret = amd_pinconf_get(&dummy_pctldev, pin, &config);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, pinconf_to_config_param(config), PIN_CONFIG_BIAS_PULL_DOWN);
	KUNIT_EXPECT_EQ(test, pinconf_to_config_argument(config), 0);
}

static void test_amd_pinconf_get_pull_up_enabled(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long config = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_UP, 0);
	unsigned int pin = 3;

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(BIT(PULL_UP_ENABLE_OFF), mock_gpio_dev.base + pin * 4);

	int ret = amd_pinconf_get(&dummy_pctldev, pin, &config);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, pinconf_to_config_param(config), PIN_CONFIG_BIAS_PULL_UP);
	KUNIT_EXPECT_EQ(test, pinconf_to_config_argument(config), 1);
}

static void test_amd_pinconf_get_pull_up_disabled(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long config = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_UP, 0);
	unsigned int pin = 4;

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(0x0, mock_gpio_dev.base + pin * 4);

	int ret = amd_pinconf_get(&dummy_pctldev, pin, &config);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, pinconf_to_config_param(config), PIN_CONFIG_BIAS_PULL_UP);
	KUNIT_EXPECT_EQ(test, pinconf_to_config_argument(config), 0);
}

static void test_amd_pinconf_get_drive_strength(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long config = pinconf_to_config_packed(PIN_CONFIG_DRIVE_STRENGTH, 0);
	unsigned int pin = 5;
	u32 drive_val = 0x3;

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	writel(drive_val << DRV_STRENGTH_SEL_OFF, mock_gpio_dev.base + pin * 4);

	int ret = amd_pinconf_get(&dummy_pctldev, pin, &config);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, pinconf_to_config_param(config), PIN_CONFIG_DRIVE_STRENGTH);
	KUNIT_EXPECT_EQ(test, pinconf_to_config_argument(config), drive_val);
}

static void test_amd_pinconf_get_invalid_param(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long config = pinconf_to_config_packed((enum pin_config_param)0xFFFF, 0);
	unsigned int pin = 6;
	struct platform_device pdev = {};

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gpio_dev.pdev = &pdev;
	writel(0x0, mock_gpio_dev.base + pin * 4);

	int ret = amd_pinconf_get(&dummy_pctldev, pin, &config);
	KUNIT_EXPECT_EQ(test, ret, -ENOTSUPP);
}

static void test_amd_pinconf_get_pin_out_of_bounds(struct kunit *test)
{
	struct pinctrl_dev dummy_pctldev;
	unsigned long config = pinconf_to_config_packed(PIN_CONFIG_INPUT_DEBOUNCE, 0);
	unsigned int pin = 1000;

	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	int ret = amd_pinconf_get(&dummy_pctldev, pin, &config);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static struct kunit_case amd_pinconf_get_test_cases[] = {
	KUNIT_CASE(test_amd_pinconf_get_debounce),
	KUNIT_CASE(test_amd_pinconf_get_pull_down_enabled),
	KUNIT_CASE(test_amd_pinconf_get_pull_down_disabled),
	KUNIT_CASE(test_amd_pinconf_get_pull_up_enabled),
	KUNIT_CASE(test_amd_pinconf_get_pull_up_disabled),
	KUNIT_CASE(test_amd_pinconf_get_drive_strength),
	KUNIT_CASE(test_amd_pinconf_get_invalid_param),
	KUNIT_CASE(test_amd_pinconf_get_pin_out_of_bounds),
	{}
};

static struct kunit_suite amd_pinconf_get_test_suite = {
	.name = "amd_pinconf_get_test",
	.test_cases = amd_pinconf_get_test_cases,
};

kunit_test_suite(amd_pinconf_get_test_suite);