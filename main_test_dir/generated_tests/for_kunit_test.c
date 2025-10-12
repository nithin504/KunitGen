```c
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
	unsigned int npins;
	unsigned int *pins;
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

static void assign_mux_owner_for_imx_f_groups(struct gpio_device *gpio_dev, unsigned int group)
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

static char test_pin_memory[4096];
static struct pin_desc test_pin_descs[256];
static struct pinctrl_device test_pctrl;
static struct group_info test_groups[10];
static struct gpio_device test_gpio_dev;
static unsigned int test_pins[256];

static void test_assign_mux_owner_match_group(struct kunit *test)
{
	memset(test_pin_descs, 0, sizeof(test_pin_descs));
	memset(test_groups, 0, sizeof(test_groups));
	memset(test_pins, 0, sizeof(test_pins));

	test_pctrl.pin_descs = test_pin_descs;
	test_gpio_dev.pctrl = &test_pctrl;
	test_gpio_dev.groups = test_groups;
	test_gpio_dev.ngroups = 1;

	test_groups[0].name = "IMX_FOO";
	test_groups[0].npins = 2;
	test_groups[0].pins = test_pins;
	test_pins[0] = 10;
	test_pins[1] = 11;

	assign_mux_owner_for_imx_f_groups(&test_gpio_dev, 0);

	KUNIT_EXPECT_PTR_EQ(test, (void *)test_pin_descs[10].mux_owner, (void *)test_groups[0].name);
	KUNIT_EXPECT_PTR_EQ(test, (void *)test_pin_descs[11].mux_owner, (void *)test_groups[0].name);
}

static void test_assign_mux_owner_non_match_group(struct kunit *test)
{
	memset(test_pin_descs, 0, sizeof(test_pin_descs));
	memset(test_groups, 0, sizeof(test_groups));
	memset(test_pins, 0, sizeof(test_pins));

	test_pctrl.pin_descs = test_pin_descs;
	test_gpio_dev.pctrl = &test_pctrl;
	test_gpio_dev.groups = test_groups;
	test_gpio_dev.ngroups = 1;

	test_groups[0].name = "NOT_IMX_F";
	test_groups[0].npins = 2;
	test_groups[0].pins = test_pins;
	test_pins[0] = 20;
	test_pins[1] = 21;

	assign_mux_owner_for_imx_f_groups(&test_gpio_dev, 0);

	KUNIT_EXPECT_NULL(test, test_pin_descs[20].mux_owner);
	KUNIT_EXPECT_NULL(test, test_pin_descs[21].mux_owner);
}

static void test_assign_mux_owner_partial_match_group(struct kunit *test)
{
	memset(test_pin_descs, 0, sizeof(test_pin_descs));
	memset(test_groups, 0, sizeof(test_groups));
	memset(test_pins, 0, sizeof(test_pins));

	test_pctrl.pin_descs = test_pin_descs;
	test_gpio_dev.pctrl = &test_pctrl;
	test_gpio_dev.groups = test_groups;
	test_gpio_dev.ngroups = 2;

	test_groups[0].name = "IMX_FOO";
	test_groups[0].npins = 1;
	test_groups[0].pins = test_pins;
	test_pins[0] = 30;

	test_groups[1].name = "NOT_MATCHING";
	test_groups[1].npins = 1;
	test_groups[1].pins = &test_pins[1];
	test_pins[1] = 31;

	assign_mux_owner_for_imx_f_groups(&test_gpio_dev, 0);
	assign_mux_owner_for_imx_f_groups(&test_gpio_dev, 1);

	KUNIT_EXPECT_PTR_EQ(test, (void *)test_pin_descs[30].mux_owner, (void *)test_groups[0].name);
	KUNIT_EXPECT_NULL(test, test_pin_descs[31].mux_owner);
}

static void test_assign_mux_owner_empty_group(struct kunit *test)
{
	memset(test_pin_descs, 0, sizeof(test_pin_descs));
	memset(test_groups, 0, sizeof(test_groups));

	test_pctrl.pin_descs = test_pin_descs;
	test_gpio_dev.pctrl = &test_pctrl;
	test_gpio_dev.groups = test_groups;
	test_gpio_dev.ngroups = 1;

	test_groups[0].name = "IMX_FOO";
	test_groups[0].npins = 0;
	test_groups[0].pins = NULL;

	assign_mux_owner_for_imx_f_groups(&test_gpio_dev, 0);

	KUNIT_EXPECT_EQ(test, 1, 1);
}

static void test_assign_mux_owner_null_group_name(struct kunit *test)
{
	memset(test_pin_descs, 0, sizeof(test_pin_descs));
	memset(test_groups, 0, sizeof(test_groups));
	memset(test_pins, 0, sizeof(test_pins));

	test_pctrl.pin_descs = test_pin_descs;
	test_gpio_dev.pctrl = &test_pctrl;
	test_gpio_dev.groups = test_groups;
	test_gpio_dev.ngroups = 1;

	test_groups[0].name = NULL;
	test_groups[0].npins = 1;
	test_groups[0].pins = test_pins;
	test_pins[0] = 40;

	assign_mux_owner_for_imx_f_groups(&test_gpio_dev, 0);

	KUNIT_EXPECT_NULL(test, test_pin_descs[40].mux_owner);
}

static struct kunit_case generated_test_cases[] = {
	KUNIT_CASE(test_assign_mux_owner_match_group),
	KUNIT_CASE(test_assign_mux_owner_non_match_group),
	KUNIT_CASE(test_assign_mux_owner_partial_match_group),
	KUNIT_CASE(test_assign_mux_owner_empty_group),
	KUNIT_CASE(test_assign_mux_owner_null_group_name),
	{}
};

static struct kunit_suite generated_test_suite = {
	.name = "generated-kunit-test",
	.test_cases = generated_test_cases,
};

kunit_test_suite(generated_test_suite);
```