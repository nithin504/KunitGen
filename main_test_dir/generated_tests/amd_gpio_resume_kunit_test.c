// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include "pinctrl-amd.c"

#define MOCK_BASE_ADDR 0x1000
#define PIN_IRQ_PENDING 0x1

static struct amd_gpio mock_gpio_dev;
static struct device mock_dev;
static char mmio_buffer[4096];

static bool should_save_ret = true;

bool amd_gpio_should_save(struct amd_gpio *gpio_dev, unsigned int pin)
{
	return should_save_ret;
}

static void test_amd_gpio_resume_success(struct kunit *test)
{
	struct pinctrl_pin_desc pins[] = {
		{ .number = 0 },
		{ .number = 1 },
	};
	struct pinctrl_desc desc = {
		.npins = ARRAY_SIZE(pins),
		.pins = pins,
	};
	struct amd_pinctrl pctrl = {
		.desc = &desc,
	};

	mock_gpio_dev.pctrl = &pctrl;
	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.saved_regs[0] = 0x0;
	mock_gpio_dev.saved_regs[1] = 0x0;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	writel(0x1, mock_gpio_dev.base + 0 * 4);
	writel(0x0, mock_gpio_dev.base + 1 * 4);

	dev_set_drvdata(&mock_dev, &mock_gpio_dev);

	should_save_ret = true;
	int ret = amd_gpio_resume(&mock_dev);

	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, mock_gpio_dev.saved_regs[0] & PIN_IRQ_PENDING, 0x1);
	KUNIT_EXPECT_EQ(test, mock_gpio_dev.saved_regs[1] & PIN_IRQ_PENDING, 0x0);
}

static void test_amd_gpio_resume_no_save(struct kunit *test)
{
	struct pinctrl_pin_desc pins[] = {
		{ .number = 0 },
	};
	struct pinctrl_desc desc = {
		.npins = ARRAY_SIZE(pins),
		.pins = pins,
	};
	struct amd_pinctrl pctrl = {
		.desc = &desc,
	};

	mock_gpio_dev.pctrl = &pctrl;
	mock_gpio_dev.base = mmio_buffer;
	mock_gpio_dev.saved_regs[0] = 0x5;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	writel(0x1, mock_gpio_dev.base + 0 * 4);

	dev_set_drvdata(&mock_dev, &mock_gpio_dev);

	should_save_ret = false;
	int ret = amd_gpio_resume(&mock_dev);

	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, mock_gpio_dev.saved_regs[0], 0x5);
}

static void test_amd_gpio_resume_empty_pins(struct kunit *test)
{
	struct pinctrl_desc desc = {
		.npins = 0,
		.pins = NULL,
	};
	struct amd_pinctrl pctrl = {
		.desc = &desc,
	};

	mock_gpio_dev.pctrl = &pctrl;
	dev_set_drvdata(&mock_dev, &mock_gpio_dev);

	int ret = amd_gpio_resume(&mock_dev);

	KUNIT_EXPECT_EQ(test, ret, 0);
}

static struct kunit_case amd_gpio_resume_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_resume_success),
	KUNIT_CASE(test_amd_gpio_resume_no_save),
	KUNIT_CASE(test_amd_gpio_resume_empty_pins),
	{}
};

static struct kunit_suite amd_gpio_resume_test_suite = {
	.name = "amd_gpio_resume_test",
	.test_cases = amd_gpio_resume_test_cases,
};

kunit_test_suite(amd_gpio_resume_test_suite);
