// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/errno.h>

// Mock data structures
struct amd_gpio {
    void *base;
};

static struct amd_gpio mock_gpio_dev;

// Function prototypes for mocks
int mock_amd_get_group_pins(struct pinctrl_dev *pctldev, unsigned group,
                            const unsigned **pins, unsigned *npins);
int mock_amd_pinconf_set(struct pinctrl_dev *pctldev, unsigned pin,
                         unsigned long *configs, unsigned num_configs);

// Redefine external calls with mocks
#define amd_get_group_pins mock_amd_get_group_pins
#define amd_pinconf_set mock_amd_pinconf_set
#include "pinctrl-amd.c"

// Global variables to control mock behavior
static int mock_get_group_pins_ret = 0;
static int mock_pinconf_set_ret = 0;
static int mock_pinconf_set_call_count = 0;
static unsigned mock_npins = 0;
static const unsigned *mock_pins_ptr = NULL;

// Mock implementations
int mock_amd_get_group_pins(struct pinctrl_dev *pctldev, unsigned group,
                            const unsigned **pins, unsigned *npins)
{
    if (mock_get_group_pins_ret)
        return mock_get_group_pins_ret;

    *npins = mock_npins;
    *pins = mock_pins_ptr;
    return 0;
}

int mock_amd_pinconf_set(struct pinctrl_dev *pctldev, unsigned pin,
                         unsigned long *configs, unsigned num_configs)
{
    mock_pinconf_set_call_count++;
    return mock_pinconf_set_ret;
}

// Test case: Normal operation with single pin
static void test_amd_pinconf_group_set_single_pin(struct kunit *test)
{
    struct pinctrl_dev dummy_pctldev;
    unsigned long configs[] = { 0x12345678 };
    unsigned pins[] = { 5 };

    mock_get_group_pins_ret = 0;
    mock_pinconf_set_ret = 0;
    mock_pinconf_set_call_count = 0;
    mock_npins = 1;
    mock_pins_ptr = pins;

    int ret = amd_pinconf_group_set(&dummy_pctldev, 0, configs, 1);

    KUNIT_EXPECT_EQ(test, ret, 0);
    KUNIT_EXPECT_EQ(test, mock_pinconf_set_call_count, 1);
}

// Test case: Normal operation with multiple pins
static void test_amd_pinconf_group_set_multiple_pins(struct kunit *test)
{
    struct pinctrl_dev dummy_pctldev;
    unsigned long configs[] = { 0x12345678 };
    unsigned pins[] = { 10, 20, 30 };

    mock_get_group_pins_ret = 0;
    mock_pinconf_set_ret = 0;
    mock_pinconf_set_call_count = 0;
    mock_npins = 3;
    mock_pins_ptr = pins;

    int ret = amd_pinconf_group_set(&dummy_pctldev, 0, configs, 1);

    KUNIT_EXPECT_EQ(test, ret, 0);
    KUNIT_EXPECT_EQ(test, mock_pinconf_set_call_count, 3);
}

// Test case: amd_get_group_pins fails
static void test_amd_pinconf_group_set_get_group_pins_fail(struct kunit *test)
{
    struct pinctrl_dev dummy_pctldev;
    unsigned long configs[] = { 0x12345678 };

    mock_get_group_pins_ret = -EINVAL;

    int ret = amd_pinconf_group_set(&dummy_pctldev, 0, configs, 1);

    KUNIT_EXPECT_EQ(test, ret, -EINVAL);
    KUNIT_EXPECT_EQ(test, mock_pinconf_set_call_count, 0);
}

// Test case: amd_pinconf_set fails for first pin
static void test_amd_pinconf_group_set_pinconf_set_fail_first(struct kunit *test)
{
    struct pinctrl_dev dummy_pctldev;
    unsigned long configs[] = { 0x12345678 };
    unsigned pins[] = { 5, 10 };

    mock_get_group_pins_ret = 0;
    mock_pinconf_set_ret = -ENOTSUPP;
    mock_pinconf_set_call_count = 0;
    mock_npins = 2;
    mock_pins_ptr = pins;

    int ret = amd_pinconf_group_set(&dummy_pctldev, 0, configs, 1);

    KUNIT_EXPECT_EQ(test, ret, -ENOTSUPP);
    KUNIT_EXPECT_EQ(test, mock_pinconf_set_call_count, 1);
}

// Test case: amd_pinconf_set fails for second pin
static void test_amd_pinconf_group_set_pinconf_set_fail_second(struct kunit *test)
{
    struct pinctrl_dev dummy_pctldev;
    unsigned long configs[] = { 0x12345678 };
    unsigned pins[] = { 5, 10 };

    mock_get_group_pins_ret = 0;
    mock_pinconf_set_ret = 0;
    mock_pinconf_set_call_count = 0;
    mock_npins = 2;
    mock_pins_ptr = pins;

    // Override mock behavior after first call
    static int call_counter = 0;
    mock_pinconf_set_ret = (++call_counter == 2) ? -ENOTSUPP : 0;

    int ret = amd_pinconf_group_set(&dummy_pctldev, 0, configs, 1);

    KUNIT_EXPECT_EQ(test, ret, -ENOTSUPP);
    KUNIT_EXPECT_EQ(test, mock_pinconf_set_call_count, 2);
}

// Test case: Zero pins in group
static void test_amd_pinconf_group_set_zero_pins(struct kunit *test)
{
    struct pinctrl_dev dummy_pctldev;
    unsigned long configs[] = { 0x12345678 };

    mock_get_group_pins_ret = 0;
    mock_pinconf_set_call_count = 0;
    mock_npins = 0;
    mock_pins_ptr = NULL;

    int ret = amd_pinconf_group_set(&dummy_pctldev, 0, configs, 1);

    KUNIT_EXPECT_EQ(test, ret, 0);
    KUNIT_EXPECT_EQ(test, mock_pinconf_set_call_count, 0);
}

// Test case: Zero configs
static void test_amd_pinconf_group_set_zero_configs(struct kunit *test)
{
    struct pinctrl_dev dummy_pctldev;
    unsigned pins[] = { 5 };

    mock_get_group_pins_ret = 0;
    mock_pinconf_set_ret = 0;
    mock_pinconf_set_call_count = 0;
    mock_npins = 1;
    mock_pins_ptr = pins;

    int ret = amd_pinconf_group_set(&dummy_pctldev, 0, NULL, 0);

    KUNIT_EXPECT_EQ(test, ret, 0);
    KUNIT_EXPECT_EQ(test, mock_pinconf_set_call_count, 1);
}

static struct kunit_case amd_pinconf_group_set_test_cases[] = {
    KUNIT_CASE(test_amd_pinconf_group_set_single_pin),
    KUNIT_CASE(test_amd_pinconf_group_set_multiple_pins),
    KUNIT_CASE(test_amd_pinconf_group_set_get_group_pins_fail),
    KUNIT_CASE(test_amd_pinconf_group_set_pinconf_set_fail_first),
    KUNIT_CASE(test_amd_pinconf_group_set_pinconf_set_fail_second),
    KUNIT_CASE(test_amd_pinconf_group_set_zero_pins),
    KUNIT_CASE(test_amd_pinconf_group_set_zero_configs),
    {}
};

static struct kunit_suite amd_pinconf_group_set_test_suite = {
    .name = "amd_pinconf_group_set_test",
    .test_cases = amd_pinconf_group_set_test_cases,
};

kunit_test_suite(amd_pinconf_group_set_test_suite);
