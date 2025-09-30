```c
// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/io.h>
#include <linux/errno.h>

#define WAKE_INT_MASTER_REG 0x00
#define INTERNAL_GPIO0_DEBOUNCE 0x2
#define DB_TYPE_REMOVE_GLITCH 0x1
#define DB_CNTRL_OFF 28
#define DB_TMR_OUT_MASK 0xFF
#define DB_TMR_OUT_UNIT_OFF 8
#define DB_TMR_LARGE_OFF 9
#define DB_CNTRL_MASK 0x7

struct amd_gpio {
	void __iomem *base;
};

static int amd_gpio_set_debounce(struct amd_gpio *gpio_dev, unsigned int offset,
				 unsigned int debounce)
{
	u32 time;
	u32 pin_reg;
	int ret = 0;

	if (offset == 0) {
		pin_reg = readl(gpio_dev->base + WAKE_INT_MASTER_REG);
		if (pin_reg & INTERNAL_GPIO0_DEBOUNCE)
			debounce = 0;
	}

	pin_reg = readl(gpio_dev->base + offset * 4);

	if (debounce) {
		pin_reg |= DB_TYPE_REMOVE_GLITCH << DB_CNTRL_OFF;
		pin_reg &= ~DB_TMR_OUT_MASK;

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
			pin_reg &= ~(DB_CNTRL_MASK << DB_CNTRL_OFF);
			ret = -EINVAL;
		}
	} else {
		pin_reg &= ~BIT(DB_TMR_OUT_UNIT_OFF);
		pin_reg &= ~BIT(DB_TMR_LARGE_OFF);
		pin_reg &= ~DB_TMR_OUT_MASK;
		pin_reg &= ~(DB_CNTRL_MASK << DB_CNTRL_OFF);
	}
	writel(pin_reg, gpio_dev->base + offset * 4);

	return ret;
}

static struct amd_gpio gpio_dev;
static char test_buffer[8192];

static void test_amd_gpio_set_debounce_offset0_no_internal_debounce(struct kunit *test)
{
	int ret;
	gpio_dev.base = test_buffer;
	writel(0, test_buffer + WAKE_INT_MASTER_REG);
	writel(0, test_buffer + 0 * 4);

	ret = amd_gpio_set_debounce(&gpio_dev, 0, 100);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_set_debounce_offset0_with_internal_debounce(struct kunit *test)
{
	int ret;
	gpio_dev.base = test_buffer;
	writel(INTERNAL_GPIO0_DEBOUNCE, test_buffer + WAKE_INT_MASTER_REG);
	writel(0, test_buffer + 0 * 4);

	ret = amd_gpio_set_debounce(&gpio_dev, 0, 100);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_set_debounce_range_less_than_61(struct kunit *test)
{
	int ret;
	gpio_dev.base = test_buffer;
	writel(0, test_buffer + WAKE_INT_MASTER_REG);
	writel(0, test_buffer + 1 * 4);

	ret = amd_gpio_set_debounce(&gpio_dev, 1, 50);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_set_debounce_range_61_to_975(struct kunit *test)
{
	int ret;
	gpio_dev.base = test_buffer;
	writel(0, test_buffer + WAKE_INT_MASTER_REG);
	writel(0, test_buffer + 2 * 4);

	ret = amd_gpio_set_debounce(&gpio_dev, 2, 100);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_set_debounce_range_976_to_3899(struct kunit *test)
{
	int ret;
	gpio_dev.base = test_buffer;
	writel(0, test_buffer + WAKE_INT_MASTER_REG);
	writel(0, test_buffer + 3 * 4);

	ret = amd_gpio_set_debounce(&gpio_dev, 3, 2000);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_set_debounce_range_3900_to_249999(struct kunit *test)
{
	int ret;
	gpio_dev.base = test_buffer;
	writel(0, test_buffer + WAKE_INT_MASTER_REG);
	writel(0, test_buffer + 4 * 4);

	ret = amd_gpio_set_debounce(&gpio_dev, 4, 10000);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_set_debounce_range_250000_to_999999(struct kunit *test)
{
	int ret;
	gpio_dev.base = test_buffer;
	writel(0, test_buffer + WAKE_INT_MASTER_REG);
	writel(0, test_buffer + 5 * 4);

	ret = amd_gpio_set_debounce(&gpio_dev, 5, 300000);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_set_debounce_invalid_range(struct kunit *test)
{
	int ret;
	gpio_dev.base = test_buffer;
	writel(0, test_buffer + WAKE_INT_MASTER_REG);
	writel(0, test_buffer + 6 * 4);

	ret = amd_gpio_set_debounce(&gpio_dev, 6, 2000000);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_amd_gpio_set_debounce_zero_debounce(struct kunit *test)
{
	int ret;
	gpio_dev.base = test_buffer;
	writel(0, test_buffer + WAKE_INT_MASTER_REG);
	writel(0xFFFFFFFF, test_buffer + 7 * 4);

	ret = amd_gpio_set_debounce(&gpio_dev, 7, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_set_debounce_exact_boundary_values(struct kunit *test)
{
	int ret;
	gpio_dev.base = test_buffer;
	writel(0, test_buffer + WAKE_INT_MASTER_REG);
	writel(0, test_buffer + 8 * 4);

	ret = amd_gpio_set_debounce(&gpio_dev, 8, 61);
	KUNIT_EXPECT_EQ(test, ret, 0);

	writel(0, test_buffer + 9 * 4);
	ret = amd_gpio_set_debounce(&gpio_dev, 9, 976);
	KUNIT_EXPECT_EQ(test, ret, 0);

	writel(0, test_buffer + 10 * 4);
	ret = amd_gpio_set_debounce(&gpio_dev, 10, 3900);
	KUNIT_EXPECT_EQ(test, ret, 0);

	writel(0, test_buffer + 11 * 4);
	ret = amd_gpio_set_debounce(&gpio_dev, 11, 250000);
	KUNIT_EXPECT_EQ(test, ret, 0);

	writel(0, test_buffer + 12 * 4);
	ret = amd_gpio_set_debounce(&gpio_dev, 12, 1000000);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static struct kunit_case amd_gpio_set_debounce_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_set_debounce_offset0_no_internal_debounce),
	KUNIT_CASE(test_amd_gpio_set_debounce_offset0_with_internal_debounce),
	KUNIT_CASE(test_amd_gpio_set_debounce_range_less_than_61),
	KUNIT_CASE(test_amd_gpio_set_debounce_range_61_to_975),
	KUNIT_CASE(test_amd_gpio_set_debounce_range_976_to_3899),
	KUNIT_CASE(test_amd_gpio_set_debounce_range_3900_to_249999),
	KUNIT_CASE(test_amd_gpio_set_debounce_range_250000_to_999999),
	KUNIT_CASE(test_amd_gpio_set_debounce_invalid_range),
	KUNIT_CASE(test_amd_gpio_set_debounce_zero_debounce),
	KUNIT_CASE(test_amd_gpio_set_debounce_exact_boundary_values),
	{}
};

static struct kunit_suite amd_gpio_set_debounce_test_suite = {
	.name = "amd_gpio_set_debounce_test",
	.test_cases = amd_gpio_set_debounce_test_cases,
};

kunit_test_suite(amd_gpio_set_debounce_test_suite);
```