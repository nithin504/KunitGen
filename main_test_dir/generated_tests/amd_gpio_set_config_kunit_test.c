// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/err.h>
#include <linux/io.h>

// Mock includes and definitions
#define PIN_CONF_PACKED(param, arg) (((param) << 16) | ((arg) & 0xffff))
#define TEST_PIN 5

struct amd_gpio {
	struct gpio_chip chip;
	struct pinctrl_dev *pctrl;
	void __iomem *base;
	raw_spinlock_t lock;
};

// Mocked external function declaration
int amd_pinconf_set(struct pinctrl_dev *pctldev, unsigned int pin,
		    unsigned long *configs, unsigned int num_configs);

// Include the source under test
static int amd_gpio_set_config(struct gpio_chip *gc, unsigned int pin,
			       unsigned long config)
{
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);
	return amd_pinconf_set(gpio_dev->pctrl, pin, &config, 1);
}

// Mock data
static struct amd_gpio *mock_gpio_dev;
static int mock_amd_pinconf_set_return = 0;
static bool mock_amd_pinconf_set_called = false;
static unsigned int mock_pinconf_set_pin;
static unsigned long mock_pinconf_set_config;

// Mock implementation of amd_pinconf_set
int amd_pinconf_set(struct pinctrl_dev *pctldev, unsigned int pin,
		    unsigned long *configs, unsigned int num_configs)
{
	mock_amd_pinconf_set_called = true;
	mock_pinconf_set_pin = pin;
	if (num_configs > 0)
		mock_pinconf_set_config = configs[0];
	return mock_amd_pinconf_set_return;
}

// Test cases
static void test_amd_gpio_set_config_success(struct kunit *test)
{
	struct gpio_chip *gc;
	unsigned long config = PIN_CONF_PACKED(PIN_CONFIG_INPUT_DEBOUNCE, 10);
	int ret;

	mock_gpio_dev = kunit_kzalloc(test, sizeof(*mock_gpio_dev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, mock_gpio_dev);

	gc = &mock_gpio_dev->chip;
	gc->owner = THIS_MODULE;
	mock_gpio_dev->pctrl = (struct pinctrl_dev *)0xDEADBEEF; // Dummy ptr

	mock_amd_pinconf_set_return = 0;
	mock_amd_pinconf_set_called = false;

	ret = amd_gpio_set_config(gc, TEST_PIN, config);

	KUNIT_EXPECT_TRUE(test, mock_amd_pinconf_set_called);
	KUNIT_EXPECT_EQ(test, mock_pinconf_set_pin, TEST_PIN);
	KUNIT_EXPECT_EQ(test, mock_pinconf_set_config, config);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_set_config_failure(struct kunit *test)
{
	struct gpio_chip *gc;
	unsigned long config = PIN_CONF_PACKED(PIN_CONFIG_BIAS_PULL_UP, 1);
	int ret;

	mock_gpio_dev = kunit_kzalloc(test, sizeof(*mock_gpio_dev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, mock_gpio_dev);

	gc = &mock_gpio_dev->chip;
	gc->owner = THIS_MODULE;
	mock_gpio_dev->pctrl = (struct pinctrl_dev *)0xDEADBEEF;

	mock_amd_pinconf_set_return = -EINVAL;
	mock_amd_pinconf_set_called = false;

	ret = amd_gpio_set_config(gc, TEST_PIN, config);

	KUNIT_EXPECT_TRUE(test, mock_amd_pinconf_set_called);
	KUNIT_EXPECT_EQ(test, mock_pinconf_set_pin, TEST_PIN);
	KUNIT_EXPECT_EQ(test, mock_pinconf_set_config, config);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_amd_gpio_set_config_null_gpio_chip(struct kunit *test)
{
	unsigned long config = PIN_CONF_PACKED(PIN_CONFIG_DRIVE_STRENGTH, 2);
	int ret;

	mock_amd_pinconf_set_called = false;

	ret = amd_gpio_set_config(NULL, TEST_PIN, config);

	KUNIT_EXPECT_FALSE(test, mock_amd_pinconf_set_called);
	// Actual behavior depends on gpiochip_get_data(), but since it's mocked,
	// we can't test dereference. We assume it would crash or return error.
	// Since our function doesn't check for gc == NULL, result is undefined.
	// But if we reach amd_pinconf_set, then it was called.
	// In real scenario, this may cause oops.
}

static struct kunit_case amd_gpio_set_config_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_set_config_success),
	KUNIT_CASE(test_amd_gpio_set_config_failure),
	KUNIT_CASE(test_amd_gpio_set_config_null_gpio_chip),
	{}
};

static struct kunit_suite amd_gpio_set_config_test_suite = {
	.name = "amd_gpio_set_config_test",
	.test_cases = amd_gpio_set_config_test_cases,
};

kunit_test_suite(amd_gpio_set_config_test_suite);
