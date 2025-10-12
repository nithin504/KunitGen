// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/errno.h>

// Mock data structures
struct pinctrl_dev {};
struct amd_gpio {};

// Function prototypes for mocks
static int mock_amd_get_group_pins_success(struct pinctrl_dev *pctldev,
					   unsigned int group,
					   const unsigned **pins,
					   unsigned *npins);
static int mock_amd_get_group_pins_failure(struct pinctrl_dev *pctldev,
					   unsigned int group,
					   const unsigned **pins,
					   unsigned *npins);
static int mock_amd_pinconf_get_success(struct pinctrl_dev *pctldev,
					unsigned int pin,
					unsigned long *config);
static int mock_amd_pinconf_get_failure(struct pinctrl_dev *pctldev,
					unsigned int pin,
					unsigned long *config);

// Override real functions with mocks
#define amd_get_group_pins mock_amd_get_group_pins_success
#define amd_pinconf_get mock_amd_pinconf_get_success
#include "pinctrl-amd.c"

// Restore original names for actual mocking
#undef amd_get_group_pins
#undef amd_pinconf_get

// Re-define function pointers for dynamic mocking
static typeof(amd_get_group_pins)* test_amd_get_group_pins_fn;
static typeof(amd_pinconf_get)* test_amd_pinconf_get_fn;

static int amd_get_group_pins(struct pinctrl_dev *pctldev,
			      unsigned int group,
			      const unsigned **pins,
			      unsigned *npins)
{
	return test_amd_get_group_pins_fn(pctldev, group, pins, npins);
}

static int amd_pinconf_get(struct pinctrl_dev *pctldev,
			   unsigned int pin,
			   unsigned long *config)
{
	return test_amd_pinconf_get_fn(pctldev, pin, config);
}

// Mock implementations
static unsigned test_pins[] = { 5 };

static int mock_amd_get_group_pins_success(struct pinctrl_dev *pctldev,
					   unsigned int group,
					   const unsigned **pins,
					   unsigned *npins)
{
	*pins = test_pins;
	*npins = 1;
	return 0;
}

static int mock_amd_get_group_pins_failure(struct pinctrl_dev *pctldev,
					   unsigned int group,
					   const unsigned **pins,
					   unsigned *npins)
{
	return -EINVAL;
}

static int mock_amd_pinconf_get_success(struct pinctrl_dev *pctldev,
					unsigned int pin,
					unsigned long *config)
{
	*config = 0x12345678;
	return 0;
}

static int mock_amd_pinconf_get_failure(struct pinctrl_dev *pctldev,
					unsigned int pin,
					unsigned long *config)
{
	return -ENOTSUPP;
}

// Test cases
static void test_amd_pinconf_group_get_success(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	unsigned long config = 0;
	test_amd_get_group_pins_fn = mock_amd_get_group_pins_success;
	test_amd_pinconf_get_fn = mock_amd_pinconf_get_success;

	int ret = amd_pinconf_group_get(&pctldev, 0, &config);

	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, config, 0x12345678UL);
}

static void test_amd_pinconf_group_get_get_group_pins_fail(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	unsigned long config = 0;
	test_amd_get_group_pins_fn = mock_amd_get_group_pins_failure;
	test_amd_pinconf_get_fn = mock_amd_pinconf_get_success;

	int ret = amd_pinconf_group_get(&pctldev, 0, &config);

	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_amd_pinconf_group_get_pinconf_get_fail(struct kunit *test)
{
	struct pinctrl_dev pctldev;
	unsigned long config = 0;
	test_amd_get_group_pins_fn = mock_amd_get_group_pins_success;
	test_amd_pinconf_get_fn = mock_amd_pinconf_get_failure;

	int ret = amd_pinconf_group_get(&pctldev, 0, &config);

	KUNIT_EXPECT_EQ(test, ret, -ENOTSUPP);
}

static struct kunit_case amd_pinconf_group_get_test_cases[] = {
	KUNIT_CASE(test_amd_pinconf_group_get_success),
	KUNIT_CASE(test_amd_pinconf_group_get_get_group_pins_fail),
	KUNIT_CASE(test_amd_pinconf_group_get_pinconf_get_fail),
	{}
};

static struct kunit_suite amd_pinconf_group_get_test_suite = {
	.name = "amd_pinconf_group_get_test",
	.test_cases = amd_pinconf_group_get_test_cases,
};

kunit_test_suite(amd_pinconf_group_get_test_suite);
