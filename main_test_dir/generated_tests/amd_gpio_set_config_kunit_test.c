#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/errno.h>

static struct amd_gpio {
	struct pinctrl_dev *pctrl;
} mock_gpio_dev;

static struct pinctrl_dev mock_pctldev;

static int amd_pinconf_set(struct pinctrl_dev *pctldev, unsigned int pin,
			   unsigned long *configs, size_t num_configs)
{
	return 0;
}

static int amd_gpio_set_config(struct gpio_chip *gc, unsigned int pin,
			       unsigned long config)
{
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);
	return amd_pinconf_set(gpio_dev->pctrl, pin, &config, 1);
}

static void test_amd_gpio_set_config_success(struct kunit *test)
{
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	int ret;
	
	gpio_dev.pctrl = &mock_pctldev;
	gpiochip_set_data(&gc, &gpio_dev);
	
	ret = amd_gpio_set_config(&gc, 0, 0x1234);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_set_config_null_gpio_chip(struct kunit *test)
{
	int ret;
	
	ret = amd_gpio_set_config(NULL, 0, 0x1234);
	KUNIT_EXPECT_LT(test, ret, 0);
}

static void test_amd_gpio_set_config_null_pinctrl_dev(struct kunit *test)
{
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	int ret;
	
	gpio_dev.pctrl = NULL;
	gpiochip_set_data(&gc, &gpio_dev);
	
	ret = amd_gpio_set_config(&gc, 0, 0x1234);
	KUNIT_EXPECT_LT(test, ret, 0);
}

static void test_amd_gpio_set_config_max_pin(struct kunit *test)
{
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	int ret;
	
	gpio_dev.pctrl = &mock_pctldev;
	gpiochip_set_data(&gc, &gpio_dev);
	
	ret = amd_gpio_set_config(&gc, UINT_MAX, 0x1234);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_set_config_zero_config(struct kunit *test)
{
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	int ret;
	
	gpio_dev.pctrl = &mock_pctldev;
	gpiochip_set_data(&gc, &gpio_dev);
	
	ret = amd_gpio_set_config(&gc, 5, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_set_config_max_config(struct kunit *test)
{
	struct gpio_chip gc;
	struct amd_gpio gpio_dev;
	int ret;
	
	gpio_dev.pctrl = &mock_pctldev;
	gpiochip_set_data(&gc, &gpio_dev);
	
	ret = amd_gpio_set_config(&gc, 10, ULONG_MAX);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static struct kunit_case amd_gpio_set_config_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_set_config_success),
	KUNIT_CASE(test_amd_gpio_set_config_null_gpio_chip),
	KUNIT_CASE(test_amd_gpio_set_config_null_pinctrl_dev),
	KUNIT_CASE(test_amd_gpio_set_config_max_pin),
	KUNIT_CASE(test_amd_gpio_set_config_zero_config),
	KUNIT_CASE(test_amd_gpio_set_config_max_config),
	{}
};

static struct kunit_suite amd_gpio_set_config_test_suite = {
	.name = "amd_gpio_set_config_test",
	.test_cases = amd_gpio_set_config_test_cases,
};

kunit_test_suite(amd_gpio_set_config_test_suite);