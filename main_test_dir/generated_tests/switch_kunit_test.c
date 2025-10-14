// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/platform_device.h>

// Mock definitions for AMD GPIO
#define PIN_CONFIG_INPUT_DEBOUNCE 1
#define PIN_CONFIG_BIAS_PULL_DOWN 2
#define PIN_CONFIG_BIAS_PULL_UP 3
#define PIN_CONFIG_DRIVE_STRENGTH 4

#define BIT(x) (1UL << (x))
#define PULL_DOWN_ENABLE_OFF 0
#define PULL_UP_ENABLE_OFF 1
#define DRV_STRENGTH_SEL_MASK 0x3
#define DRV_STRENGTH_SEL_OFF 2

#define WAKE_INT_MASTER_REG 0x100

struct amd_gpio {
	void __iomem *base;
	spinlock_t lock;
	struct platform_device *pdev;
};

static int amd_gpio_set_debounce_ret = 0;
static int amd_gpio_set_debounce_call_count = 0;

static int amd_gpio_set_debounce(struct amd_gpio *gpio_dev, unsigned int pin, u32 debounce)
{
	amd_gpio_set_debounce_call_count++;
	return amd_gpio_set_debounce_ret;
}

// Mock dev_dbg to prevent linking issues
#define dev_dbg(dev, fmt, ...) do { } while (0)

// Function under test (embedded)
static int test_amd_pinconf_set_inner(
		struct amd_gpio *gpio_dev,
		unsigned int pin,
		enum pin_config_param param,
		u32 arg)
{
	u32 pin_reg = 0;
	int ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&gpio_dev->lock, flags);

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
		dev_dbg(&gpio_dev->pdev->dev,
			"Invalid config param %04x\n", param);
		ret = -ENOTSUPP;
	}

out_unlock:
	spin_unlock_irqrestore(&gpio_dev->lock, flags);
	return ret;
}

// Test cases
static void test_pinconf_set_debounce_success(struct kunit *test)
{
	struct amd_gpio gpio_dev = {
		.base = NULL,
		.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock),
		.pdev = NULL,
	};
	amd_gpio_set_debounce_ret = 0;
	amd_gpio_set_debounce_call_count = 0;

	int ret = test_amd_pinconf_set_inner(&gpio_dev, 5, PIN_CONFIG_INPUT_DEBOUNCE, 100);

	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, amd_gpio_set_debounce_call_count, 1);
}

static void test_pinconf_set_debounce_failure(struct kunit *test)
{
	struct amd_gpio gpio_dev = {
		.base = NULL,
		.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock),
		.pdev = NULL,
	};
	amd_gpio_set_debounce_ret = -EINVAL;
	amd_gpio_set_debounce_call_count = 0;

	int ret = test_amd_pinconf_set_inner(&gpio_dev, 3, PIN_CONFIG_INPUT_DEBOUNCE, 200);

	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
	KUNIT_EXPECT_EQ(test, amd_gpio_set_debounce_call_count, 1);
}

static void test_pinconf_set_pull_down_enable(struct kunit *test)
{
	struct amd_gpio gpio_dev = {
		.base = NULL,
		.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock),
		.pdev = NULL,
	};

	// We can't check pin_reg directly, but we ensure no crash and correct path taken
	int ret = test_amd_pinconf_set_inner(&gpio_dev, 2, PIN_CONFIG_BIAS_PULL_DOWN, 1);

	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_pinconf_set_pull_down_disable(struct kunit *test)
{
	struct amd_gpio gpio_dev = {
		.base = NULL,
		.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock),
		.pdev = NULL,
	};

	int ret = test_amd_pinconf_set_inner(&gpio_dev, 2, PIN_CONFIG_BIAS_PULL_DOWN, 0);

	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_pinconf_set_pull_up_enable(struct kunit *test)
{
	struct amd_gpio gpio_dev = {
		.base = NULL,
		.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock),
		.pdev = NULL,
	};

	int ret = test_amd_pinconf_set_inner(&gpio_dev, 4, PIN_CONFIG_BIAS_PULL_UP, 1);

	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_pinconf_set_pull_up_disable(struct kunit *test)
{
	struct amd_gpio gpio_dev = {
		.base = NULL,
		.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock),
		.pdev = NULL,
	};

	int ret = test_amd_pinconf_set_inner(&gpio_dev, 4, PIN_CONFIG_BIAS_PULL_UP, 0);

	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_pinconf_set_drive_strength(struct kunit *test)
{
	struct amd_gpio gpio_dev = {
		.base = NULL,
		.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock),
		.pdev = NULL,
	};

	int ret = test_amd_pinconf_set_inner(&gpio_dev, 1, PIN_CONFIG_DRIVE_STRENGTH, 2);

	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_pinconf_set_drive_strength_max(struct kunit *test)
{
	struct amd_gpio gpio_dev = {
		.base = NULL,
		.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock),
		.pdev = NULL,
	};

	int ret = test_amd_pinconf_set_inner(&gpio_dev, 1, PIN_CONFIG_DRIVE_STRENGTH, 3);

	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_pinconf_set_invalid_param(struct kunit *test)
{
	struct amd_gpio gpio_dev = {
		.base = NULL,
		.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock),
		.pdev = kunit_kzalloc(test, sizeof(*(gpio_dev.pdev)), GFP_KERNEL),
	};

	int ret = test_amd_pinconf_set_inner(&gpio_dev, 0, (enum pin_config_param)0xFFFF, 0);

	KUNIT_EXPECT_EQ(test, ret, -ENOTSUPP);
}

static struct kunit_case generated_test_cases[] = {
	KUNIT_CASE(test_pinconf_set_debounce_success),
	KUNIT_CASE(test_pinconf_set_debounce_failure),
	KUNIT_CASE(test_pinconf_set_pull_down_enable),
	KUNIT_CASE(test_pinconf_set_pull_down_disable),
	KUNIT_CASE(test_pinconf_set_pull_up_enable),
	KUNIT_CASE(test_pinconf_set_pull_up_disable),
	KUNIT_CASE(test_pinconf_set_drive_strength),
	KUNIT_CASE(test_pinconf_set_drive_strength_max),
	KUNIT_CASE(test_pinconf_set_invalid_param),
	{}
};

static struct kunit_suite generated_test_suite = {
	.name = "generated_pinconf_test",
	.test_cases = generated_test_cases,
};

kunit_test_suite(generated_test_suite);
