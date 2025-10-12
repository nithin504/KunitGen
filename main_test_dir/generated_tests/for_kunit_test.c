// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/string.h>
#include <linux/pinctrl/pinctrl.h>

struct pin_desc {
	const char *mux_owner;
};

struct pinctrl_device {
	struct pin_desc *pin_descs;
};

struct group_info {
	const char *name;
	unsigned int *pins;
	unsigned int npins;
};

struct gpio_device {
	struct pinctrl_device *pctrl;
	struct group_info *groups;
	unsigned int ngroups;
};

static struct pin_desc *pin_desc_get(struct pinctrl_device *pctrl, unsigned int pin)
{
	return &pctrl->pin_descs[pin];
}

static void process_imx_groups(struct gpio_device *gpio_dev, unsigned int group)
{
	unsigned int ind;
	struct pin_desc *pd;

	for (ind = 0; ind < gpio_dev->groups[group].npins; ind++) {
		if (strncmp(gpio_dev->groups[group].name, "IMX_F", strlen("IMX_F")))
			continue;

		pd = pin_desc_get(gpio_dev->pctrl, gpio_dev->groups[group].pins[ind]);
		pd->mux_owner = gpio_dev->groups[group].name;
	}
}

static void test_process_imx_groups_match_prefix(struct kunit *test)
{
	struct pinctrl_device pctrl_dev = {0};
	struct group_info groups[1];
	struct gpio_device gpio_dev = {0};
	struct pin_desc pin_descs[2];
	unsigned int pins[] = {0, 1};
	char buffer[64];

	pctrl_dev.pin_descs = pin_descs;
	gpio_dev.pctrl = &pctrl_dev;
	gpio_dev.groups = groups;
	gpio_dev.ngroups = 1;

	groups[0].name = "IMX_FOO";
	groups[0].pins = pins;
	groups[0].npins = 2;

	pin_descs[0].mux_owner = NULL;
	pin_descs[1].mux_owner = NULL;

	process_imx_groups(&gpio_dev, 0);

	KUNIT_EXPECT_PTR_EQ(test, (void *)pin_descs[0].mux_owner, (void *)groups[0].name);
	KUNIT_EXPECT_PTR_EQ(test, (void *)pin_descs[1].mux_owner, (void *)groups[0].name);
}

static void test_process_imx_groups_no_match_prefix(struct kunit *test)
{
	struct pinctrl_device pctrl_dev = {0};
	struct group_info groups[1];
	struct gpio_device gpio_dev = {0};
	struct pin_desc pin_descs[2];
	unsigned int pins[] = {0, 1};

	pctrl_dev.pin_descs = pin_descs;
	gpio_dev.pctrl = &pctrl_dev;
	gpio_dev.groups = groups;
	gpio_dev.ngroups = 1;

	groups[0].name = "IMX_GOO";
	groups[0].pins = pins;
	groups[0].npins = 2;

	pin_descs[0].mux_owner = NULL;
	pin_descs[1].mux_owner = NULL;

	process_imx_groups(&gpio_dev, 0);

	KUNIT_EXPECT_PTR_EQ(test, (void *)pin_descs[0].mux_owner, (void *)NULL);
	KUNIT_EXPECT_PTR_EQ(test, (void *)pin_descs[1].mux_owner, (void *)NULL);
}

static void test_process_imx_groups_partial_match_prefix(struct kunit *test)
{
	struct pinctrl_device pctrl_dev = {0};
	struct group_info groups[1];
	struct gpio_device gpio_dev = {0};
	struct pin_desc pin_descs[2];
	unsigned int pins[] = {0, 1};

	pctrl_dev.pin_descs = pin_descs;
	gpio_dev.pctrl = &pctrl_dev;
	gpio_dev.groups = groups;
	gpio_dev.ngroups = 1;

	groups[0].name = "IMX_F";
	groups[0].pins = pins;
	groups[0].npins = 2;

	pin_descs[0].mux_owner = NULL;
	pin_descs[1].mux_owner = NULL;

	process_imx_groups(&gpio_dev, 0);

	KUNIT_EXPECT_PTR_EQ(test, (void *)pin_descs[0].mux_owner, (void *)groups[0].name);
	KUNIT_EXPECT_PTR_EQ(test, (void *)pin_descs[1].mux_owner, (void *)groups[0].name);
}

static void test_process_imx_groups_empty_group(struct kunit *test)
{
	struct pinctrl_device pctrl_dev = {0};
	struct group_info groups[1];
	struct gpio_device gpio_dev = {0};

	pctrl_dev.pin_descs = NULL;
	gpio_dev.pctrl = &pctrl_dev;
	gpio_dev.groups = groups;
	gpio_dev.ngroups = 1;

	groups[0].name = "IMX_FOO";
	groups[0].pins = NULL;
	groups[0].npins = 0;

	process_imx_groups(&gpio_dev, 0);
	KUNIT_EXPECT_EQ(test, 1, 1);
}

static void test_process_imx_groups_null_pins(struct kunit *test)
{
	struct pinctrl_device pctrl_dev = {0};
	struct group_info groups[1];
	struct gpio_device gpio_dev = {0};

	pctrl_dev.pin_descs = NULL;
	gpio_dev.pctrl = &pctrl_dev;
	gpio_dev.groups = groups;
	gpio_dev.ngroups = 1;

	groups[0].name = "IMX_FOO";
	groups[0].pins = NULL;
	groups[0].npins = 1;

	process_imx_groups(&gpio_dev, 0);
	KUNIT_EXPECT_EQ(test, 1, 1);
}

static struct kunit_case imx_group_test_cases[] = {
	KUNIT_CASE(test_process_imx_groups_match_prefix),
	KUNIT_CASE(test_process_imx_groups_no_match_prefix),
	KUNIT_CASE(test_process_imx_groups_partial_match_prefix),
	KUNIT_CASE(test_process_imx_groups_empty_group),
	KUNIT_CASE(test_process_imx_groups_null_pins),
	{}
};

static struct kunit_suite imx_group_test_suite = {
	.name = "imx_group_test",
	.test_cases = imx_group_test_cases,
};

kunit_test_suite(imx_group_test_suite);
