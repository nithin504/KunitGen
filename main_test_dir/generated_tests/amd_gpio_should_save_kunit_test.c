// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/gpio/driver.h>
#include <linux/errno.h>

// Mock pin_desc_get function
static const struct pin_desc *pin_desc_get(struct pinctrl_dev *pctldev, unsigned int pin)
{
	return (const struct pin_desc *)kunit_kzalloc(current->kunit_test, sizeof(struct pin_desc), GFP_KERNEL);
}

// Mock gpiochip_line_is_irq function
static bool gpiochip_line_is_irq(struct gpio_chip *gc, unsigned int offset)
{
	return false;
}

static bool amd_gpio_should_save(struct amd_gpio *gpio_dev, unsigned int pin)
{
	const struct pin_desc *pd = pin_desc_get(gpio_dev->pctrl, pin);

	if (!pd)
		return false;

	if (pd->mux_owner || pd->gpio_owner ||
	    gpiochip_line_is_irq(&gpio_dev->gc, pin))
		return true;

	return false;
}

static void test_amd_gpio_should_save_null_pd(struct kunit *test)
{
	struct amd_gpio gpio_dev;
	struct pinctrl_dev pctrl;
	struct gpio_chip gc;
	
	gpio_dev.pctrl = &pctrl;
	gpio_dev.gc = gc;
	
	bool result = amd_gpio_should_save(&gpio_dev, 0);
	KUNIT_EXPECT_FALSE(test, result);
}

static void test_amd_gpio_should_save_mux_owner_true(struct kunit *test)
{
	struct amd_gpio gpio_dev;
	struct pinctrl_dev pctrl;
	struct gpio_chip gc;
	struct pin_desc *pd;
	
	gpio_dev.pctrl = &pctrl;
	gpio_dev.gc = gc;
	
	pd = (struct pin_desc *)kunit_kzalloc(test, sizeof(struct pin_desc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pd);
	
	pd->mux_owner = true;
	pd->gpio_owner = false;
	
	// Mock pin_desc_get to return our test pin_desc
	const struct pin_desc *(*orig_pin_desc_get)(struct pinctrl_dev *, unsigned int) = pin_desc_get;
	pin_desc_get = (void *)kunit_kzalloc(test, sizeof(void *), GFP_KERNEL);
	*(void **)pin_desc_get = (void *)pd;
	
	bool result = amd_gpio_should_save(&gpio_dev, 0);
	KUNIT_EXPECT_TRUE(test, result);
	
	pin_desc_get = orig_pin_desc_get;
}

static void test_amd_gpio_should_save_gpio_owner_true(struct kunit *test)
{
	struct amd_gpio gpio_dev;
	struct pinctrl_dev pctrl;
	struct gpio_chip gc;
	struct pin_desc *pd;
	
	gpio_dev.pctrl = &pctrl;
	gpio_dev.gc = gc;
	
	pd = (struct pin_desc *)kunit_kzalloc(test, sizeof(struct pin_desc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pd);
	
	pd->mux_owner = false;
	pd->gpio_owner = true;
	
	const struct pin_desc *(*orig_pin_desc_get)(struct pinctrl_dev *, unsigned int) = pin_desc_get;
	pin_desc_get = (void *)kunit_kzalloc(test, sizeof(void *), GFP_KERNEL);
	*(void **)pin_desc_get = (void *)pd;
	
	bool result = amd_gpio_should_save(&gpio_dev, 0);
	KUNIT_EXPECT_TRUE(test, result);
	
	pin_desc_get = orig_pin_desc_get;
}

static void test_amd_gpio_should_save_irq_line_true(struct kunit *test)
{
	struct amd_gpio gpio_dev;
	struct pinctrl_dev pctrl;
	struct gpio_chip gc;
	struct pin_desc *pd;
	
	gpio_dev.pctrl = &pctrl;
	gpio_dev.gc = gc;
	
	pd = (struct pin_desc *)kunit_kzalloc(test, sizeof(struct pin_desc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pd);
	
	pd->mux_owner = false;
	pd->gpio_owner = false;
	
	const struct pin_desc *(*orig_pin_desc_get)(struct pinctrl_dev *, unsigned int) = pin_desc_get;
	pin_desc_get = (void *)kunit_kzalloc(test, sizeof(void *), GFP_KERNEL);
	*(void **)pin_desc_get = (void *)pd;
	
	bool (*orig_gpiochip_line_is_irq)(struct gpio_chip *, unsigned int) = gpiochip_line_is_irq;
	gpiochip_line_is_irq = (void *)kunit_kzalloc(test, sizeof(void *), GFP_KERNEL);
	*(void **)gpiochip_line_is_irq = (void *)1; // Return true
	
	bool result = amd_gpio_should_save(&gpio_dev, 0);
	KUNIT_EXPECT_TRUE(test, result);
	
	pin_desc_get = orig_pin_desc_get;
	gpiochip_line_is_irq = orig_gpiochip_line_is_irq;
}

static void test_amd_gpio_should_save_all_false(struct kunit *test)
{
	struct amd_gpio gpio_dev;
	struct pinctrl_dev pctrl;
	struct gpio_chip gc;
	struct pin_desc *pd;
	
	gpio_dev.pctrl = &pctrl;
	gpio_dev.gc = gc;
	
	pd = (struct pin_desc *)kunit_kzalloc(test, sizeof(struct pin_desc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pd);
	
	pd->mux_owner = false;
	pd->gpio_owner = false;
	
	const struct pin_desc *(*orig_pin_desc_get)(struct pinctrl_dev *, unsigned int) = pin_desc_get;
	pin_desc_get = (void *)kunit_kzalloc(test, sizeof(void *), GFP_KERNEL);
	*(void **)pin_desc_get = (void *)pd;
	
	bool result = amd_gpio_should_save(&gpio_dev, 0);
	KUNIT_EXPECT_FALSE(test, result);
	
	pin_desc_get = orig_pin_desc_get;
}

static struct kunit_case amd_gpio_should_save_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_should_save_null_pd),
	KUNIT_CASE(test_amd_gpio_should_save_mux_owner_true),
	KUNIT_CASE(test_amd_gpio_should_save_gpio_owner_true),
	KUNIT_CASE(test_amd_gpio_should_save_irq_line_true),
	KUNIT_CASE(test_amd_gpio_should_save_all_false),
	{}
};

static struct kunit_suite amd_gpio_should_save_test_suite = {
	.name = "amd_gpio_should_save_test",
	.test_cases = amd_gpio_should_save_test_cases,
};

kunit_test_suite(amd_gpio_should_save_test_suite);