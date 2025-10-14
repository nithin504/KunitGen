// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/gpio/driver.h>
#include <linux/irq.h>
#include <linux/irqdesc.h>

// Mock structures and functions
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
static bool gpiochip_line_is_irq_return_value = false;

const struct pin_desc *pin_desc_get(struct pinctrl_dev *pctldev, unsigned int pin)
{
	if (pin == 999) // Special case to simulate NULL return
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

	result = amd_gpio_should_save(&gpio_dev, 999); // Triggers NULL pd
	KUNIT_EXPECT_FALSE(test, result);
}

static void test_amd_gpio_should_save_no_owners_no_irq(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	bool result;

	memset(&mock_pd, 0, sizeof(mock_pd));
	gpiochip_line_is_irq_return_value = false;

	result = amd_gpio_should_save(&gpio_dev, 0);
	KUNIT_EXPECT_FALSE(test, result);
}

static void test_amd_gpio_should_save_mux_owner_set(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	bool result;

	memset(&mock_pd, 0, sizeof(mock_pd));
	mock_pd.mux_owner = (void *)0x1; // Non-NULL
	gpiochip_line_is_irq_return_value = false;

	result = amd_gpio_should_save(&gpio_dev, 0);
	KUNIT_EXPECT_TRUE(test, result);
}

static void test_amd_gpio_should_save_gpio_owner_set(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	bool result;

	memset(&mock_pd, 0, sizeof(mock_pd));
	mock_pd.gpio_owner = (void *)0x1; // Non-NULL
	gpiochip_line_is_irq_return_value = false;

	result = amd_gpio_should_save(&gpio_dev, 0);
	KUNIT_EXPECT_TRUE(test, result);
}

static void test_amd_gpio_should_save_line_is_irq_true(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	bool result;

	memset(&mock_pd, 0, sizeof(mock_pd));
	gpiochip_line_is_irq_return_value = true;

	result = amd_gpio_should_save(&gpio_dev, 0);
	KUNIT_EXPECT_TRUE(test, result);
}

static void test_amd_gpio_should_save_all_conditions_true(struct kunit *test)
{
	struct amd_gpio gpio_dev = {0};
	bool result;

	memset(&mock_pd, 0, sizeof(mock_pd));
	mock_pd.mux_owner = (void *)0x1;
	mock_pd.gpio_owner = (void *)0x1;
	gpiochip_line_is_irq_return_value = true;

	result = amd_gpio_should_save(&gpio_dev, 0);
	KUNIT_EXPECT_TRUE(test, result);
}

static struct kunit_case amd_gpio_should_save_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_should_save_null_pin_desc),
	KUNIT_CASE(test_amd_gpio_should_save_no_owners_no_irq),
	KUNIT_CASE(test_amd_gpio_should_save_mux_owner_set),
	KUNIT_CASE(test_amd_gpio_should_save_gpio_owner_set),
	KUNIT_CASE(test_amd_gpio_should_save_line_is_irq_true),
	KUNIT_CASE(test_amd_gpio_should_save_all_conditions_true),
	{}
};

static struct kunit_suite amd_gpio_should_save_test_suite = {
	.name = "amd_gpio_should_save",
	.test_cases = amd_gpio_should_save_test_cases,
};

kunit_test_suite(amd_gpio_should_save_test_suite);
