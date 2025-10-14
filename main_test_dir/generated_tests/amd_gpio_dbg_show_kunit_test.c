// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/seq_file.h>
#include <linux/gpio/driver.h>
#include <linux/io.h>
#include <linux/spinlock.h>

#define WAKE_INT_MASTER_REG 0x0
#define AMD_GPIO_PINS_BANK0 64
#define AMD_GPIO_PINS_BANK1 64
#define AMD_GPIO_PINS_BANK2 64
#define AMD_GPIO_PINS_BANK3 64

#define INTERRUPT_ENABLE_OFF 16
#define ACTIVE_LEVEL_OFF 17
#define ACTIVE_LEVEL_MASK 0x3
#define ACTIVE_LEVEL_HIGH 0x1
#define ACTIVE_LEVEL_LOW 0x2
#define ACTIVE_LEVEL_BOTH 0x3
#define LEVEL_TRIG_OFF 19
#define INTERRUPT_MASK_OFF 20
#define INTERRUPT_STS_OFF 21
#define WAKE_CNTRL_OFF_S0I3 22
#define WAKE_CNTRL_OFF_S3 23
#define WAKE_CNTRL_OFF_S4 24
#define WAKECNTRL_Z_OFF 25
#define WAKE_STS_OFF 26
#define PULL_UP_ENABLE_OFF 8
#define PULL_DOWN_ENABLE_OFF 9
#define OUTPUT_ENABLE_OFF 0
#define OUTPUT_VALUE_OFF 1
#define PIN_STS_OFF 2
#define DB_CNTRL_OFF 28
#define DB_CNTRl_MASK 0x7
#define DB_TMR_OUT_UNIT_OFF 9
#define DB_TMR_LARGE_OFF 10
#define DB_TMR_OUT_MASK 0xFF
#define DB_TYPE_REMOVE_GLITCH 0x1
#define DB_TYPE_PRESERVE_LOW_GLITCH 0x2

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
	unsigned int hwbank_num;
};

static struct amd_gpio *gpiochip_get_data(struct gpio_chip *gc)
{
	return gc->private;
}

static char mock_mmio[8192];
static struct amd_gpio mock_gpio_dev;
static struct gpio_chip mock_gpio_chip;
static struct seq_file mock_seq_file;

static void setup_mock_gpio(struct kunit *test)
{
	memset(&mock_gpio_dev, 0, sizeof(mock_gpio_dev));
	memset(&mock_gpio_chip, 0, sizeof(mock_gpio_chip));
	memset(&mock_seq_file, 0, sizeof(mock_seq_file));
	memset(mock_mmio, 0, sizeof(mock_mmio));

	mock_gpio_dev.base = mock_mmio;
	mock_gpio_dev.hwbank_num = 4;
	raw_spin_lock_init(&mock_gpio_dev.lock);
	mock_gpio_chip.private = &mock_gpio_dev;
	mock_seq_file.buf = kunit_kzalloc(test, 4096, GFP_KERNEL);
	mock_seq_file.size = 4096;
}

static void test_amd_gpio_dbg_show_basic_output(struct kunit *test)
{
	setup_mock_gpio(test);
	writel(0x12345678, mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, mock_seq_file.count > 0);
}

static void test_amd_gpio_dbg_show_all_banks(struct kunit *test)
{
	setup_mock_gpio(test);
	mock_gpio_dev.hwbank_num = 4;
	writel(0x0, mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, mock_seq_file.count > 0);
}

static void test_amd_gpio_dbg_show_no_banks(struct kunit *test)
{
	setup_mock_gpio(test);
	mock_gpio_dev.hwbank_num = 0;
	writel(0x0, mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, mock_seq_file.count > 0);
}

static void test_amd_gpio_dbg_show_interrupt_enabled_edge_high(struct kunit *test)
{
	setup_mock_gpio(test);
	u32 reg_val = BIT(INTERRUPT_ENABLE_OFF) |
		      (ACTIVE_LEVEL_HIGH << ACTIVE_LEVEL_OFF) |
		      BIT(INTERRUPT_MASK_OFF) |
		      BIT(INTERRUPT_STS_OFF);
	writel(reg_val, mock_gpio_dev.base + 4 * 0);
	writel(0x0, mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, mock_seq_file.count > 0);
}

static void test_amd_gpio_dbg_show_interrupt_enabled_level_low(struct kunit *test)
{
	setup_mock_gpio(test);
	u32 reg_val = BIT(INTERRUPT_ENABLE_OFF) |
		      (ACTIVE_LEVEL_LOW << ACTIVE_LEVEL_OFF) |
		      BIT(LEVEL_TRIG_OFF);
	writel(reg_val, mock_gpio_dev.base + 4 * 1);
	writel(0x0, mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, mock_seq_file.count > 0);
}

static void test_amd_gpio_dbg_show_interrupt_both_edge(struct kunit *test)
{
	setup_mock_gpio(test);
	u32 reg_val = BIT(INTERRUPT_ENABLE_OFF) |
		      (ACTIVE_LEVEL_BOTH << ACTIVE_LEVEL_OFF);
	writel(reg_val, mock_gpio_dev.base + 4 * 2);
	writel(0x0, mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, mock_seq_file.count > 0);
}

static void test_amd_gpio_dbg_show_interrupt_disabled(struct kunit *test)
{
	setup_mock_gpio(test);
	writel(0x0, mock_gpio_dev.base + 4 * 3);
	writel(0x0, mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, mock_seq_file.count > 0);
}

static void test_amd_gpio_dbg_show_wake_controls(struct kunit *test)
{
	setup_mock_gpio(test);
	u32 reg_val = BIT(WAKE_CNTRL_OFF_S0I3) |
		      BIT(WAKE_CNTRL_OFF_S3) |
		      BIT(WAKE_CNTRL_OFF_S4) |
		      BIT(WAKECNTRL_Z_OFF) |
		      BIT(WAKE_STS_OFF);
	writel(reg_val, mock_gpio_dev.base + 4 * 4);
	writel(0x0, mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, mock_seq_file.count > 0);
}

static void test_amd_gpio_dbg_show_pull_up_down_none(struct kunit *test)
{
	setup_mock_gpio(test);
	writel(0x0, mock_gpio_dev.base + 4 * 5);
	writel(0x0, mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, mock_seq_file.count > 0);
}

static void test_amd_gpio_dbg_show_pull_up(struct kunit *test)
{
	setup_mock_gpio(test);
	writel(BIT(PULL_UP_ENABLE_OFF), mock_gpio_dev.base + 4 * 6);
	writel(0x0, mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, mock_seq_file.count > 0);
}

static void test_amd_gpio_dbg_show_pull_down(struct kunit *test)
{
	setup_mock_gpio(test);
	writel(BIT(PULL_DOWN_ENABLE_OFF), mock_gpio_dev.base + 4 * 7);
	writel(0x0, mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, mock_seq_file.count > 0);
}

static void test_amd_gpio_dbg_show_output_high(struct kunit *test)
{
	setup_mock_gpio(test);
	u32 reg_val = BIT(OUTPUT_ENABLE_OFF) | BIT(OUTPUT_VALUE_OFF);
	writel(reg_val, mock_gpio_dev.base + 4 * 8);
	writel(0x0, mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, mock_seq_file.count > 0);
}

static void test_amd_gpio_dbg_show_input_high(struct kunit *test)
{
	setup_mock_gpio(test);
	u32 reg_val = BIT(PIN_STS_OFF);
	writel(reg_val, mock_gpio_dev.base + 4 * 9);
	writel(0x0, mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, mock_seq_file.count > 0);
}

static void test_amd_gpio_dbg_show_debounce_remove_glitch_large_unit_us(struct kunit *test)
{
	setup_mock_gpio(test);
	u32 reg_val = (DB_TYPE_REMOVE_GLITCH << DB_CNTRL_OFF) |
		      BIT(DB_TMR_LARGE_OFF) |
		      BIT(DB_TMR_OUT_UNIT_OFF) |
		      (10 & DB_TMR_OUT_MASK);
	writel(reg_val, mock_gpio_dev.base + 4 * 10);
	writel(0x0, mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, mock_seq_file.count > 0);
}

static void test_amd_gpio_dbg_show_debounce_preserve_low_glitch_small_unit_ms(struct kunit *test)
{
	setup_mock_gpio(test);
	u32 reg_val = (DB_TYPE_PRESERVE_LOW_GLITCH << DB_CNTRL_OFF) |
		      (20 & DB_TMR_OUT_MASK);
	writel(reg_val, mock_gpio_dev.base + 4 * 11);
	writel(0x0, mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, mock_seq_file.count > 0);
}

static void test_amd_gpio_dbg_show_debounce_other_type(struct kunit *test)
{
	setup_mock_gpio(test);
	u32 reg_val = (0x3 << DB_CNTRL_OFF) |
		      BIT(DB_TMR_LARGE_OFF) |
		      (30 & DB_TMR_OUT_MASK);
	writel(reg_val, mock_gpio_dev.base + 4 * 12);
	writel(0x0, mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, mock_seq_file.count > 0);
}

static void test_amd_gpio_dbg_show_no_debounce(struct kunit *test)
{
	setup_mock_gpio(test);
	writel(0x0, mock_gpio_dev.base + 4 * 13);
	writel(0x0, mock_gpio_dev.base + WAKE_INT_MASTER_REG);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, mock_seq_file.count > 0);
}

static struct kunit_case amd_gpio_dbg_show_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_dbg_show_basic_output),
	KUNIT_CASE(test_amd_gpio_dbg_show_all_banks),
	KUNIT_CASE(test_amd_gpio_dbg_show_no_banks),
	KUNIT_CASE(test_amd_gpio_dbg_show_interrupt_enabled_edge_high),
	KUNIT_CASE(test_amd_gpio_dbg_show_interrupt_enabled_level_low),
	KUNIT_CASE(test_amd_gpio_dbg_show_interrupt_both_edge),
	KUNIT_CASE(test_amd_gpio_dbg_show_interrupt_disabled),
	KUNIT_CASE(test_amd_gpio_dbg_show_wake_controls),
	KUNIT_CASE(test_amd_gpio_dbg_show_pull_up_down_none),
	KUNIT_CASE(test_amd_gpio_dbg_show_pull_up),
	KUNIT_CASE(test_amd_gpio_dbg_show_pull_down),
	KUNIT_CASE(test_amd_gpio_dbg_show_output_high),
	KUNIT_CASE(test_amd_gpio_dbg_show_input_high),
	KUNIT_CASE(test_amd_gpio_dbg_show_debounce_remove_glitch_large_unit_us),
	KUNIT_CASE(test_amd_gpio_dbg_show_debounce_preserve_low_glitch_small_unit_ms),
	KUNIT_CASE(test_amd_gpio_dbg_show_debounce_other_type),
	KUNIT_CASE(test_amd_gpio_dbg_show_no_debounce),
	{}
};

static struct kunit_suite amd_gpio_dbg_show_test_suite = {
	.name = "amd_gpio_dbg_show_test",
	.test_cases = amd_gpio_dbg_show_test_cases,
};

kunit_test_suite(amd_gpio_dbg_show_test_suite);
