// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/gpio/driver.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/spinlock.h>

// Mock structures and functions
struct amd_gpio {
	struct gpio_chip gc;
	struct pinctrl_dev *pctrl;
	void __iomem *base;
	raw_spinlock_t lock;
	struct platform_device *pdev;
};

struct pin_desc {
	struct device_node *mux_owner;
	struct gpio_chip *gpio_owner;
};

static struct pin_desc *pin_desc_get(struct pinctrl_dev *pctldev, unsigned int pin)
{
	// This will be replaced by our mocks
	return NULL;
}

static bool gpiochip_line_is_irq(struct gpio_chip *gc, unsigned int offset)
{
	// This will be replaced by our mocks
	return false;
}

// Include the function under test
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

// Mock data
static struct amd_gpio mock_gpio_dev;
static struct pin_desc mock_pin_desc;

// Mock implementations
static struct pin_desc *(*orig_pin_desc_get)(struct pinctrl_dev *, unsigned int);
static bool (*orig_gpiochip_line_is_irq)(struct gpio_chip *, unsigned int);

static struct pin_desc *mock_pin_desc_get(struct pinctrl_dev *pctldev, unsigned int pin)
{
	if (pin == 0)
		return &mock_pin_desc;
	else if (pin == 1)
		return NULL; // Simulate pin_desc_get returning NULL
	return NULL;
}

static bool mock_gpiochip_line_is_irq_true(struct gpio_chip *gc, unsigned int offset)
{
	return true;
}

static bool mock_gpiochip_line_is_irq_false(struct gpio_chip *gc, unsigned int offset)
{
	return false;
}

// Test cases
static void test_amd_gpio_should_save_null_pd(struct kunit *test)
{
	bool result;

	// Setup: Make pin_desc_get return NULL
	orig_pin_desc_get = pin_desc_get;
	*(void **)&pin_desc_get = (void *)mock_pin_desc_get;

	result = amd_gpio_should_save(&mock_gpio_dev, 1);
	KUNIT_EXPECT_FALSE(test, result);
}

static void test_amd_gpio_should_save_no_owner_no_irq(struct kunit *test)
{
	bool result;

	// Setup
	orig_pin_desc_get = pin_desc_get;
	*(void **)&pin_desc_get = (void *)mock_pin_desc_get;
	orig_gpiochip_line_is_irq = gpiochip_line_is_irq;
	*(void **)&gpiochip_line_is_irq = (void *)mock_gpiochip_line_is_irq_false;

	mock_pin_desc.mux_owner = NULL;
	mock_pin_desc.gpio_owner = NULL;

	result = amd_gpio_should_save(&mock_gpio_dev, 0);
	KUNIT_EXPECT_FALSE(test, result);
}

static void test_amd_gpio_should_save_mux_owner_set(struct kunit *test)
{
	bool result;

	// Setup
	orig_pin_desc_get = pin_desc_get;
	*(void **)&pin_desc_get = (void *)mock_pin_desc_get;
	orig_gpiochip_line_is_irq = gpiochip_line_is_irq;
	*(void **)&gpiochip_line_is_irq = (void *)mock_gpiochip_line_is_irq_false;

	mock_pin_desc.mux_owner = (struct device_node *)0x1; // Non-NULL
	mock_pin_desc.gpio_owner = NULL;

	result = amd_gpio_should_save(&mock_gpio_dev, 0);
	KUNIT_EXPECT_TRUE(test, result);
}

static void test_amd_gpio_should_save_gpio_owner_set(struct kunit *test)
{
	bool result;

	// Setup
	orig_pin_desc_get = pin_desc_get;
	*(void **)&pin_desc_get = (void *)mock_pin_desc_get;
	orig_gpiochip_line_is_irq = gpiochip_line_is_irq;
	*(void **)&gpiochip_line_is_irq = (void *)mock_gpiochip_line_is_irq_false;

	mock_pin_desc.mux_owner = NULL;
	mock_pin_desc.gpio_owner = (struct gpio_chip *)0x1; // Non-NULL

	result = amd_gpio_should_save(&mock_gpio_dev, 0);
	KUNIT_EXPECT_TRUE(test, result);
}

static void test_amd_gpio_should_save_line_is_irq_true(struct kunit *test)
{
	bool result;

	// Setup
	orig_pin_desc_get = pin_desc_get;
	*(void **)&pin_desc_get = (void *)mock_pin_desc_get;
	orig_gpiochip_line_is_irq = gpiochip_line_is_irq;
	*(void **)&gpiochip_line_is_irq = (void *)mock_gpiochip_line_is_irq_true;

	mock_pin_desc.mux_owner = NULL;
	mock_pin_desc.gpio_owner = NULL;

	result = amd_gpio_should_save(&mock_gpio_dev, 0);
	KUNIT_EXPECT_TRUE(test, result);
}

static void test_amd_gpio_should_save_all_conditions_true(struct kunit *test)
{
	bool result;

	// Setup
	orig_pin_desc_get = pin_desc_get;
	*(void **)&pin_desc_get = (void *)mock_pin_desc_get;
	orig_gpiochip_line_is_irq = gpiochip_line_is_irq;
	*(void **)&gpiochip_line_is_irq = (void *)mock_gpiochip_line_is_irq_true;

	mock_pin_desc.mux_owner = (struct device_node *)0x1;
	mock_pin_desc.gpio_owner = (struct gpio_chip *)0x1;

	result = amd_gpio_should_save(&mock_gpio_dev, 0);
	KUNIT_EXPECT_TRUE(test, result);
}

static struct kunit_case amd_gpio_should_save_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_should_save_null_pd),
	KUNIT_CASE(test_amd_gpio_should_save_no_owner_no_irq),
	KUNIT_CASE(test_amd_gpio_should_save_mux_owner_set),
	KUNIT_CASE(test_amd_gpio_should_save_gpio_owner_set),
	KUNIT_CASE(test_amd_gpio_should_save_line_is_irq_true),
	KUNIT_CASE(test_amd_gpio_should_save_all_conditions_true),
	{}
};

static struct kunit_suite amd_gpio_should_save_test_suite = {
	.name = "amd_gpio_should_save_test",
	.test_cases = amd_gpio_should_save_test_cases,
};

kunit_test_suite(amd_gpio_should_save_test_suite);
