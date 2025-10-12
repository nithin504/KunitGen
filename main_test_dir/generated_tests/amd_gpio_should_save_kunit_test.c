```c
// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/gpio/driver.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>

// Mock structures
struct amd_gpio {
	struct gpio_chip gc;
	struct pinctrl_dev *pctrl;
};

struct pin_desc {
	struct device_node *mux_owner;
	struct gpio_chip *gpio_owner;
};

// Function under test
static bool amd_gpio_should_save(struct amd_gpio *gpio_dev, unsigned int pin)
{
	const struct pin_desc *pd = pin_desc_get(gpio_dev->pctrl, pin);

	if (!pd)
		return false;

	/*
	 * Only restore the pin if it is actually in use by the kernel (or
	 * by userspace).
	 */
	if (pd->mux_owner || pd->gpio_owner ||
	    gpiochip_line_is_irq(&gpio_dev->gc, pin))
		return true;

	return false;
}

// Mock implementations
static struct pin_desc mock_pd;
static bool gpiochip_line_is_irq_return_value;

const struct pin_desc *pin_desc_get(struct pinctrl_dev *pctldev, unsigned int pin)
{
	if (pin == 0xFFFFFFFF)
		return NULL;
	return &mock_pd;
}

bool gpiochip_line_is_irq(struct gpio_chip *gc, unsigned int offset)
{
	return gpiochip_line_is_irq_return_value;
}

// Test cases
static void test_amd_gpio_should_save_null_pin_desc(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	bool result;

	result = amd_gpio_should_save(&gpio_dev, 0xFFFFFFFF);
	KUNIT_EXPECT_FALSE(test, result);
}

static void test_amd_gpio_should_save_no_owners_no_irq(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	bool result;

	mock_pd.mux_owner = NULL;
	mock_pd.gpio_owner = NULL;
	gpiochip_line_is_irq_return_value = false;

	result = amd_gpio_should_save(&gpio_dev, 0);
	KUNIT_EXPECT_FALSE(test, result);
}

static void test_amd_gpio_should_save_mux_owner_set(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	bool result;
	struct device_node dummy_node;

	mock_pd.mux_owner = &dummy_node;
	mock_pd.gpio_owner = NULL;
	gpiochip_line_is_irq_return_value = false;

	result = amd_gpio_should_save(&gpio_dev, 0);
	KUNIT_EXPECT_TRUE(test, result);
}

static void test_amd_gpio_should_save_gpio_owner_set(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	bool result;
	struct gpio_chip dummy_gc;

	mock_pd.mux_owner = NULL;
	mock_pd.gpio_owner = &dummy_gc;
	gpiochip_line_is_irq_return_value = false;

	result = amd_gpio_should_save(&gpio_dev, 0);
	KUNIT_EXPECT_TRUE(test, result);
}

static void test_amd_gpio_should_save_irq_line_true(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	bool result;

	mock_pd.mux_owner = NULL;
	mock_pd.gpio_owner = NULL;
	gpiochip_line_is_irq_return_value = true;

	result = amd_gpio_should_save(&gpio_dev, 0);
	KUNIT_EXPECT_TRUE(test, result);
}

static void test_amd_gpio_should_save_all_conditions_true(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	bool result;
	struct device_node dummy_node;
	struct gpio_chip dummy_gc;

	mock_pd.mux_owner = &dummy_node;
	mock_pd.gpio_owner = &dummy_gc;
	gpiochip_line_is_irq_return_value = true;

	result = amd_gpio_should_save(&gpio_dev, 0);
	KUNIT_EXPECT_TRUE(test, result);
}

static struct kunit_case amd_gpio_should_save_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_should_save_null_pin_desc),
	KUNIT_CASE(test_amd_gpio_should_save_no_owners_no_irq),
	KUNIT_CASE(test_amd_gpio_should_save_mux_owner_set),
	KUNIT_CASE(test_amd_gpio_should_save_gpio_owner_set),
	KUNIT_CASE(test_amd_gpio_should_save_irq_line_true),
	KUNIT_CASE(test_amd_gpio_should_save_all_conditions_true),
	{}
};

static struct kunit_suite amd_gpio_should_save_test_suite = {
	.name = "amd_gpio_should_save_test",
	.test_cases = amd_gpio_should_save_test_cases,
};

kunit_test_suite(amd_gpio_should_save_test_suite);
```