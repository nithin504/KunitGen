// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/spinlock.h>

// Mock definitions and includes
#define PIN_IRQ_PENDING 0x100
static int mock_should_save_result = 1;

static int amd_gpio_should_save(struct amd_gpio *gpio_dev, int pin)
{
	return mock_should_save_result;
}

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
	struct pinctrl_dev *pctrl;
	u32 *saved_regs;
};

#include "pinctrl-amd.c" // Include the source under test

// Begin test implementation
#define MOCK_BASE_ADDR 0x10000
#define NUM_MOCK_PINS 4
static char mock_mmio_region[0x10000];
static u32 mock_saved_regs[NUM_MOCK_PINS];
static struct pinctrl_pin_desc mock_pins[NUM_MOCK_PINS];
static struct pinctrl_desc mock_pinctrl_desc = {
	.npins = NUM_MOCK_PINS,
	.pins = mock_pins,
};
static struct amd_gpio mock_gpio_dev = {
	.base = mock_mmio_region,
	.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock),
	.saved_regs = mock_saved_regs,
	.pctrl = &(struct pinctrl_dev){ .desc = &mock_pinctrl_desc },
};
static struct platform_device mock_pdev;
static struct device mock_dev = {
	.platform_data = &mock_pdev,
};

static void setup_mock_pins(void)
{
	for (int i = 0; i < NUM_MOCK_PINS; i++) {
		mock_pins[i].number = i * 4;
		mock_saved_regs[i] = 0x55;
		writel(0x1FF, mock_gpio_dev.base + mock_pins[i].number * 4);
	}
}

static void test_amd_gpio_resume_normal_operation(struct kunit *test)
{
	struct device *dev = &mock_dev;
	mock_should_save_result = 1;
	setup_mock_pins();

	dev_set_drvdata(dev, &mock_gpio_dev);
	int ret = amd_gpio_resume(dev);

	KUNIT_EXPECT_EQ(test, ret, 0);
	for (int i = 0; i < NUM_MOCK_PINS; i++) {
		u32 reg_val = readl(mock_gpio_dev.base + mock_pins[i].number * 4);
		KUNIT_EXPECT_EQ(test, reg_val, 0x155);
	}
}

static void test_amd_gpio_resume_skip_save(struct kunit *test)
{
	struct device *dev = &mock_dev;
	mock_should_save_result = 0;
	setup_mock_pins();

	dev_set_drvdata(dev, &mock_gpio_dev);
	int ret = amd_gpio_resume(dev);

	KUNIT_EXPECT_EQ(test, ret, 0);
	for (int i = 0; i < NUM_MOCK_PINS; i++) {
		u32 reg_val = readl(mock_gpio_dev.base + mock_pins[i].number * 4);
		KUNIT_EXPECT_EQ(test, reg_val, 0x1FF);
	}
}

static void test_amd_gpio_resume_mixed_save(struct kunit *test)
{
	struct device *dev = &mock_dev;
	mock_should_save_result = 1;
	setup_mock_pins();
	mock_should_save_result = 0;
	mock_pins[1].number = 16;
	writel(0x1FF, mock_gpio_dev.base + mock_pins[1].number * 4);
	mock_should_save_result = 1;

	dev_set_drvdata(dev, &mock_gpio_dev);
	int ret = amd_gpio_resume(dev);

	KUNIT_EXPECT_EQ(test, ret, 0);
	u32 reg_val0 = readl(mock_gpio_dev.base + mock_pins[0].number * 4);
	KUNIT_EXPECT_EQ(test, reg_val0, 0x155);
	u32 reg_val1 = readl(mock_gpio_dev.base + mock_pins[1].number * 4);
	KUNIT_EXPECT_EQ(test, reg_val1, 0x1FF);
}

static void test_amd_gpio_resume_no_pins(struct kunit *test)
{
	struct device *dev = &mock_dev;
	struct pinctrl_desc empty_desc = { .npins = 0 };
	mock_gpio_dev.pctrl->desc = &empty_desc;

	dev_set_drvdata(dev, &mock_gpio_dev);
	int ret = amd_gpio_resume(dev);

	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_resume_null_base(struct kunit *test)
{
	struct device *dev = &mock_dev;
	mock_gpio_dev.base = NULL;
	setup_mock_pins();

	dev_set_drvdata(dev, &mock_gpio_dev);
	int ret = amd_gpio_resume(dev);

	KUNIT_EXPECT_EQ(test, ret, 0);
}

static struct kunit_case amd_gpio_resume_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_resume_normal_operation),
	KUNIT_CASE(test_amd_gpio_resume_skip_save),
	KUNIT_CASE(test_amd_gpio_resume_mixed_save),
	KUNIT_CASE(test_amd_gpio_resume_no_pins),
	KUNIT_CASE(test_amd_gpio_resume_null_base),
	{}
};

static struct kunit_suite amd_gpio_resume_test_suite = {
	.name = "amd_gpio_resume_test",
	.test_cases = amd_gpio_resume_test_cases,
};

kunit_test_suite(amd_gpio_resume_test_suite);
