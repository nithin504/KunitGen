// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/io.h>
#include <linux/errno.h>

/* Mock definitions for AMD GPIO registers and constants */
#define WAKE_INT_MASTER_REG 0x100
#define INTERNAL_GPIO0_DEBOUNCE BIT(1)
#define DB_TYPE_REMOVE_GLITCH 0x1
#define DB_CNTRL_OFF 28
#define DB_TMR_OUT_MASK 0xFF
#define DB_TMR_OUT_UNIT_OFF 8
#define DB_TMR_LARGE_OFF 9
#define DB_CNTRl_MASK 0x7
#define BIT(x) (1U << (x))

struct amd_gpio {
	void __iomem *base;
};

static int amd_gpio_set_debounce(struct amd_gpio *gpio_dev, unsigned int offset,
				 unsigned int debounce)
{
	u32 time;
	u32 pin_reg;
	int ret = 0;

	/* Use special handling for Pin0 debounce */
	if (offset == 0) {
		pin_reg = readl(gpio_dev->base + WAKE_INT_MASTER_REG);
		if (pin_reg & INTERNAL_GPIO0_DEBOUNCE)
			debounce = 0;
	}

	pin_reg = readl(gpio_dev->base + offset * 4);

	if (debounce) {
		pin_reg |= DB_TYPE_REMOVE_GLITCH << DB_CNTRL_OFF;
		pin_reg &= ~DB_TMR_OUT_MASK;
		/*
		Debounce	Debounce	Timer	Max
		TmrLarge	TmrOutUnit	Unit	Debounce
							Time
		0	0	61 usec (2 RtcClk)	976 usec
		0	1	244 usec (8 RtcClk)	3.9 msec
		1	0	15.6 msec (512 RtcClk)	250 msec
		1	1	62.5 msec (2048 RtcClk)	1 sec
		*/

		if (debounce < 61) {
			pin_reg |= 1;
			pin_reg &= ~BIT(DB_TMR_OUT_UNIT_OFF);
			pin_reg &= ~BIT(DB_TMR_LARGE_OFF);
		} else if (debounce < 976) {
			time = debounce / 61;
			pin_reg |= time & DB_TMR_OUT_MASK;
			pin_reg &= ~BIT(DB_TMR_OUT_UNIT_OFF);
			pin_reg &= ~BIT(DB_TMR_LARGE_OFF);
		} else if (debounce < 3900) {
			time = debounce / 244;
			pin_reg |= time & DB_TMR_OUT_MASK;
			pin_reg |= BIT(DB_TMR_OUT_UNIT_OFF);
			pin_reg &= ~BIT(DB_TMR_LARGE_OFF);
		} else if (debounce < 250000) {
			time = debounce / 15625;
			pin_reg |= time & DB_TMR_OUT_MASK;
			pin_reg &= ~BIT(DB_TMR_OUT_UNIT_OFF);
			pin_reg |= BIT(DB_TMR_LARGE_OFF);
		} else if (debounce < 1000000) {
			time = debounce / 62500;
			pin_reg |= time & DB_TMR_OUT_MASK;
			pin_reg |= BIT(DB_TMR_OUT_UNIT_OFF);
			pin_reg |= BIT(DB_TMR_LARGE_OFF);
		} else {
			pin_reg &= ~(DB_CNTRl_MASK << DB_CNTRL_OFF);
			ret = -EINVAL;
		}
	} else {
		pin_reg &= ~BIT(DB_TMR_OUT_UNIT_OFF);
		pin_reg &= ~BIT(DB_TMR_LARGE_OFF);
		pin_reg &= ~DB_TMR_OUT_MASK;
		pin_reg &= ~(DB_CNTRl_MASK << DB_CNTRL_OFF);
	}
	writel(pin_reg, gpio_dev->base + offset * 4);

	return ret;
}

/* Test buffer simulating MMIO space */
static char test_mmio_buffer[4096];
static struct amd_gpio mock_gpio_dev;

/* Test cases */

static void test_amd_gpio_set_debounce_zero_debounce(struct kunit *test)
{
	int ret;
	mock_gpio_dev.base = test_mmio_buffer;
	writel(0xFFFFFFFF, mock_gpio_dev.base + 4); /* Initial register value */

	ret = amd_gpio_set_debounce(&mock_gpio_dev, 1, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(mock_gpio_dev.base + 4);
	KUNIT_EXPECT_EQ(test, reg_val & (BIT(DB_TMR_OUT_UNIT_OFF) | BIT(DB_TMR_LARGE_OFF)), 0);
	KUNIT_EXPECT_EQ(test, reg_val & DB_TMR_OUT_MASK, 0);
	KUNIT_EXPECT_EQ(test, (reg_val >> DB_CNTRL_OFF) & DB_CNTRl_MASK, 0);
}

static void test_amd_gpio_set_debounce_less_than_61(struct kunit *test)
{
	int ret;
	mock_gpio_dev.base = test_mmio_buffer;
	writel(0x0, mock_gpio_dev.base + 4);

	ret = amd_gpio_set_debounce(&mock_gpio_dev, 1, 50);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(mock_gpio_dev.base + 4);
	KUNIT_EXPECT_NE(test, reg_val & (DB_TYPE_REMOVE_GLITCH << DB_CNTRL_OFF), 0);
	KUNIT_EXPECT_EQ(test, reg_val & 0xFF, 1);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(DB_TMR_OUT_UNIT_OFF), 0);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(DB_TMR_LARGE_OFF), 0);
}

static void test_amd_gpio_set_debounce_less_than_976(struct kunit *test)
{
	int ret;
	mock_gpio_dev.base = test_mmio_buffer;
	writel(0x0, mock_gpio_dev.base + 4);

	ret = amd_gpio_set_debounce(&mock_gpio_dev, 1, 300);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(mock_gpio_dev.base + 4);
	u32 time = 300 / 61;
	KUNIT_EXPECT_EQ(test, reg_val & 0xFF, time);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(DB_TMR_OUT_UNIT_OFF), 0);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(DB_TMR_LARGE_OFF), 0);
}

static void test_amd_gpio_set_debounce_less_than_3900(struct kunit *test)
{
	int ret;
	mock_gpio_dev.base = test_mmio_buffer;
	writel(0x0, mock_gpio_dev.base + 4);

	ret = amd_gpio_set_debounce(&mock_gpio_dev, 1, 2000);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(mock_gpio_dev.base + 4);
	u32 time = 2000 / 244;
	KUNIT_EXPECT_EQ(test, reg_val & 0xFF, time);
	KUNIT_EXPECT_NE(test, reg_val & BIT(DB_TMR_OUT_UNIT_OFF), 0);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(DB_TMR_LARGE_OFF), 0);
}

static void test_amd_gpio_set_debounce_less_than_250000(struct kunit *test)
{
	int ret;
	mock_gpio_dev.base = test_mmio_buffer;
	writel(0x0, mock_gpio_dev.base + 4);

	ret = amd_gpio_set_debounce(&mock_gpio_dev, 1, 100000);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(mock_gpio_dev.base + 4);
	u32 time = 100000 / 15625;
	KUNIT_EXPECT_EQ(test, reg_val & 0xFF, time);
	KUNIT_EXPECT_EQ(test, reg_val & BIT(DB_TMR_OUT_UNIT_OFF), 0);
	KUNIT_EXPECT_NE(test, reg_val & BIT(DB_TMR_LARGE_OFF), 0);
}

static void test_amd_gpio_set_debounce_less_than_1000000(struct kunit *test)
{
	int ret;
	mock_gpio_dev.base = test_mmio_buffer;
	writel(0x0, mock_gpio_dev.base + 4);

	ret = amd_gpio_set_debounce(&mock_gpio_dev, 1, 500000);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(mock_gpio_dev.base + 4);
	u32 time = 500000 / 62500;
	KUNIT_EXPECT_EQ(test, reg_val & 0xFF, time);
	KUNIT_EXPECT_NE(test, reg_val & BIT(DB_TMR_OUT_UNIT_OFF), 0);
	KUNIT_EXPECT_NE(test, reg_val & BIT(DB_TMR_LARGE_OFF), 0);
}

static void test_amd_gpio_set_debounce_exceeds_max(struct kunit *test)
{
	int ret;
	mock_gpio_dev.base = test_mmio_buffer;
	writel(0xFFFFFFFF, mock_gpio_dev.base + 4);

	ret = amd_gpio_set_debounce(&mock_gpio_dev, 1, 2000000);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);

	u32 reg_val = readl(mock_gpio_dev.base + 4);
	KUNIT_EXPECT_EQ(test, (reg_val >> DB_CNTRL_OFF) & DB_CNTRl_MASK, 0);
}

static void test_amd_gpio_set_debounce_offset_zero_no_internal_debounce(struct kunit *test)
{
	int ret;
	mock_gpio_dev.base = test_mmio_buffer;
	writel(0x0, mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	writel(0x0, mock_gpio_dev.base + 0);

	ret = amd_gpio_set_debounce(&mock_gpio_dev, 0, 100);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(mock_gpio_dev.base + 0);
	u32 time = 100 / 61;
	KUNIT_EXPECT_EQ(test, reg_val & 0xFF, time);
}

static void test_amd_gpio_set_debounce_offset_zero_with_internal_debounce(struct kunit *test)
{
	int ret;
	mock_gpio_dev.base = test_mmio_buffer;
	writel(INTERNAL_GPIO0_DEBOUNCE, mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	writel(0xFFFFFFFF, mock_gpio_dev.base + 0);

	ret = amd_gpio_set_debounce(&mock_gpio_dev, 0, 100);
	KUNIT_EXPECT_EQ(test, ret, 0);

	u32 reg_val = readl(mock_gpio_dev.base + 0);
	KUNIT_EXPECT_EQ(test, reg_val & (BIT(DB_TMR_OUT_UNIT_OFF) | BIT(DB_TMR_LARGE_OFF)), 0);
	KUNIT_EXPECT_EQ(test, reg_val & DB_TMR_OUT_MASK, 0);
	KUNIT_EXPECT_EQ(test, (reg_val >> DB_CNTRL_OFF) & DB_CNTRl_MASK, 0);
}

static struct kunit_case amd_gpio_debounce_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_set_debounce_zero_debounce),
	KUNIT_CASE(test_amd_gpio_set_debounce_less_than_61),
	KUNIT_CASE(test_amd_gpio_set_debounce_less_than_976),
	KUNIT_CASE(test_amd_gpio_set_debounce_less_than_3900),
	KUNIT_CASE(test_amd_gpio_set_debounce_less_than_250000),
	KUNIT_CASE(test_amd_gpio_set_debounce_less_than_1000000),
	KUNIT_CASE(test_amd_gpio_set_debounce_exceeds_max),
	KUNIT_CASE(test_amd_gpio_set_debounce_offset_zero_no_internal_debounce),
	KUNIT_CASE(test_amd_gpio_set_debounce_offset_zero_with_internal_debounce),
	{}
};

static struct kunit_suite amd_gpio_debounce_test_suite = {
	.name = "amd_gpio_debounce_test",
	.test_cases = amd_gpio_debounce_test_cases,
};

kunit_test_suite(amd_gpio_debounce_test_suite);
