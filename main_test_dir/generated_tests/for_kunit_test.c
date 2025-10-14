// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/string.h>
#include <linux/pinctrl/pinctrl.h>

// Mock structures and functions
struct pin_desc {
	const char *mux_owner;
};

struct pinctrl_device {
	// Minimal mock
};

struct pin_group {
	const char *name;
	unsigned int *pins;
	unsigned int npins;
};

struct gpio_device {
	struct pinctrl_device *pctrl;
	struct pin_group *groups;
	unsigned int ngroups;
};

// Function pointer for pin_desc_get
static struct pin_desc *(*mock_pin_desc_get)(struct pinctrl_device *, unsigned int);
struct pin_desc *pin_desc_get(struct pinctrl_device *pctrl, unsigned int pin)
{
	if (mock_pin_desc_get)
		return mock_pin_desc_get(pctrl, pin);
	return NULL;
}

// Inline the provided code snippet as a function
static void assign_mux_owner_for_imx_f_groups(struct gpio_device *gpio_dev, unsigned int group)
{
	unsigned int ind;
	struct pin_desc *pd;

	for (ind = 0; ind < gpio_dev->groups[group].npins; ind++) {
		if (strncmp(gpio_dev->groups[group].name, "IMX_F", strlen("IMX_F")))
			continue;

		pd = pin_desc_get(gpio_dev->pctrl, gpio_dev->groups[group].pins[ind]);
		if (pd)
			pd->mux_owner = gpio_dev->groups[group].name;
	}
}

// --- Test Cases ---

static struct pin_desc test_pin_desc;
static int mock_pin_desc_get_call_count;

static struct pin_desc *test_pin_desc_get(struct pinctrl_device *pctrl, unsigned int pin)
{
	mock_pin_desc_get_call_count++;
	return &test_pin_desc;
}

static void test_assign_mux_owner_group_matches_imx_f(struct kunit *test)
{
	struct gpio_device gpio_dev;
	struct pin_group group;
	unsigned int pins[] = { 10, 20 };
	const char *group_name = "IMX_FOO";

	group.name = group_name;
	group.pins = pins;
	group.npins = ARRAY_SIZE(pins);

	gpio_dev.groups = &group;
	gpio_dev.ngroups = 1;
	gpio_dev.pctrl = NULL; // Not used in mock

	mock_pin_desc_get = test_pin_desc_get;
	mock_pin_desc_get_call_count = 0;
	test_pin_desc.mux_owner = NULL;

	assign_mux_owner_for_imx_f_groups(&gpio_dev, 0);

	KUNIT_EXPECT_EQ(test, mock_pin_desc_get_call_count, 2);
	KUNIT_EXPECT_PTR_EQ(test, (void *)test_pin_desc.mux_owner, (void *)group_name);
}

static void test_assign_mux_owner_group_does_not_match(struct kunit *test)
{
	struct gpio_device gpio_dev;
	struct pin_group group;
	unsigned int pins[] = { 10, 20 };
	const char *group_name = "GPIO_GROUP";

	group.name = group_name;
	group.pins = pins;
	group.npins = ARRAY_SIZE(pins);

	gpio_dev.groups = &group;
	gpio_dev.ngroups = 1;
	gpio_dev.pctrl = NULL;

	mock_pin_desc_get = test_pin_desc_get;
	mock_pin_desc_get_call_count = 0;
	test_pin_desc.mux_owner = NULL;

	assign_mux_owner_for_imx_f_groups(&gpio_dev, 0);

	KUNIT_EXPECT_EQ(test, mock_pin_desc_get_call_count, 0);
	KUNIT_EXPECT_NULL(test, (void *)test_pin_desc.mux_owner);
}

static void test_assign_mux_owner_empty_group(struct kunit *test)
{
	struct gpio_device gpio_dev;
	struct pin_group group;
	const char *group_name = "IMX_FOO";

	group.name = group_name;
	group.pins = NULL;
	group.npins = 0;

	gpio_dev.groups = &group;
	gpio_dev.ngroups = 1;
	gpio_dev.pctrl = NULL;

	mock_pin_desc_get = test_pin_desc_get;
	mock_pin_desc_get_call_count = 0;

	assign_mux_owner_for_imx_f_groups(&gpio_dev, 0);

	KUNIT_EXPECT_EQ(test, mock_pin_desc_get_call_count, 0);
}

static void test_assign_mux_owner_null_pin_desc(struct kunit *test)
{
	struct gpio_device gpio_dev;
	struct pin_group group;
	unsigned int pins[] = { 10 };

	group.name = "IMX_FOO";
	group.pins = pins;
	group.npins = ARRAY_SIZE(pins);

	gpio_dev.groups = &group;
	gpio_dev.ngroups = 1;
	gpio_dev.pctrl = NULL;

	// Simulate pin_desc_get returning NULL
	mock_pin_desc_get = NULL;
	test_pin_desc.mux_owner = NULL;

	assign_mux_owner_for_imx_f_groups(&gpio_dev, 0);

	// Should not crash and mux_owner should remain unchanged
	KUNIT_EXPECT_NULL(test, (void *)test_pin_desc.mux_owner);
}

static struct kunit_case generated_test_cases[] = {
	KUNIT_CASE(test_assign_mux_owner_group_matches_imx_f),
	KUNIT_CASE(test_assign_mux_owner_group_does_not_match),
	KUNIT_CASE(test_assign_mux_owner_empty_group),
	KUNIT_CASE(test_assign_mux_owner_null_pin_desc),
	{}
};

static struct kunit_suite generated_test_suite = {
	.name = "generated-kunit-test",
	.test_cases = generated_test_cases,
};

kunit_test_suite(generated_test_suite);
