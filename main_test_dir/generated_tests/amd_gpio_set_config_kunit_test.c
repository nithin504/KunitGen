// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/err.h>
#include <linux/slab.h>

// Mock includes and definitions
#define PIN_CONF_PACKED(param, arg) (((param) << 16) | ((arg) & 0xffff))

// Mock structures
struct amd_gpio {
	struct gpio_chip chip;
	struct pinctrl_dev *pctrl;
};

// Function under test (copied and made static for direct access)
static int amd_gpio_set_config(struct gpio_chip *gc, unsigned int pin,
			       unsigned long config)
{
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);

	return amd_pinconf_set(gpio_dev->pctrl, pin, &config, 1);
}

// Mocking amd_pinconf_set to control its behavior during tests
static int mock_amd_pinconf_set_ret = 0;
int amd_pinconf_set(struct pinctrl_dev *pctldev, unsigned int pin,
		    unsigned long *configs, unsigned int num_configs)
{
	return mock_amd_pinconf_set_ret;
}

// Test case: successful configuration
static void test_amd_gpio_set_config_success(struct kunit *test)
{
	struct amd_gpio *gpio_dev;
	struct gpio_chip *gc;
	int ret;

	gpio_dev = kunit_kzalloc(test, sizeof(*gpio_dev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev);

	gc = &gpio_dev->chip;
	gpio_dev->pctrl = (struct pinctrl_dev *)0xdeadbeef; // Dummy non-NULL ptr
	gc->owner = THIS_MODULE;

	mock_amd_pinconf_set_ret = 0;
	ret = amd_gpio_set_config(gc, 5, PIN_CONF_PACKED(PIN_CONFIG_BIAS_DISABLE, 0));

	KUNIT_EXPECT_EQ(test, ret, 0);
}

// Test case: error propagation from amd_pinconf_set
static void test_amd_gpio_set_config_error_propagation(struct kunit *test)
{
	struct amd_gpio *gpio_dev;
	struct gpio_chip *gc;
	int ret;

	gpio_dev = kunit_kzalloc(test, sizeof(*gpio_dev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev);

	gc = &gpio_dev->chip;
	gpio_dev->pctrl = (struct pinctrl_dev *)0xdeadbeef;
	gc->owner = THIS_MODULE;

	mock_amd_pinconf_set_ret = -EINVAL;
	ret = amd_gpio_set_config(gc, 10, PIN_CONF_PACKED(PIN_CONFIG_SLEW_RATE, 1));

	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

// Test case: different pin configurations
static void test_amd_gpio_set_config_various_configs(struct kunit *test)
{
	struct amd_gpio *gpio_dev;
	struct gpio_chip *gc;
	unsigned long configs[] = {
		PIN_CONF_PACKED(PIN_CONFIG_BIAS_PULL_UP, 1),
		PIN_CONF_PACKED(PIN_CONFIG_DRIVE_STRENGTH, 2),
		PIN_CONF_PACKED(PIN_CONFIG_INPUT_DEBOUNCE, 3)
	};
	int i, ret;

	gpio_dev = kunit_kzalloc(test, sizeof(*gpio_dev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gpio_dev);

	gc = &gpio_dev->chip;
	gpio_dev->pctrl = (struct pinctrl_dev *)0xdeadbeef;
	gc->owner = THIS_MODULE;

	for (i = 0; i < ARRAY_SIZE(configs); i++) {
		mock_amd_pinconf_set_ret = 0;
		ret = amd_gpio_set_config(gc, 0, configs[i]);
		KUNIT_EXPECT_EQ(test, ret, 0);
	}
}

static struct kunit_case amd_gpio_set_config_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_set_config_success),
	KUNIT_CASE(test_amd_gpio_set_config_error_propagation),
	KUNIT_CASE(test_amd_gpio_set_config_various_configs),
	{}
};

static struct kunit_suite amd_gpio_set_config_test_suite = {
	.name = "amd_gpio_set_config_test",
	.test_cases = amd_gpio_set_config_test_cases,
};

kunit_test_suite(amd_gpio_set_config_test_suite);
