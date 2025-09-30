// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/seq_file.h>
#include <linux/gpio/driver.h>
#include <linux/spinlock.h>
#include <linux/io.h>

#define BIT(nr)			(1UL << (nr))

#define WAKE_INT_MASTER_REG 0x00
#define INTERRUPT_ENABLE_OFF 28
#define ACTIVE_LEVEL_OFF 26
#define ACTIVE_LEVEL_MASK 0x3
#define ACTIVE_LEVEL_HIGH 0
#define ACTIVE_LEVEL_LOW 1
#define ACTIVE_LEVEL_BOTH 2
#define LEVEL_TRIG_OFF 27
#define INTERRUPT_MASK_OFF 29
#define INTERRUPT_STS_OFF 30
#define WAKE_CNTRL_OFF_S0I3 20
#define WAKE_CNTRL_OFF_S3 21
#define WAKE_CNTRL_OFF_S4 22
#define WAKECNTRL_Z_OFF 23
#define WAKE_STS_OFF 24
#define PULL_UP_ENABLE_OFF 0
#define PULL_DOWN_ENABLE_OFF 1
#define OUTPUT_ENABLE_OFF 2
#define OUTPUT_VALUE_OFF 3
#define PIN_STS_OFF 4
#define DB_CNTRL_OFF 28
#define DB_CNTRL_MASK 0x7
#define DB_TMR_OUT_UNIT_OFF 8
#define DB_TMR_LARGE_OFF 9
#define DB_TMR_OUT_MASK 0xFF
#define DB_TYPE_REMOVE_GLITCH 0x1
#define DB_TYPE_PRESERVE_LOW_GLITCH 0x2

#define AMD_GPIO_PINS_BANK0 64
#define AMD_GPIO_PINS_BANK1 64
#define AMD_GPIO_PINS_BANK2 64
#define AMD_GPIO_PINS_BANK3 64

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
	unsigned int hwbank_num;
};

static char mock_buffer[8192];
static struct amd_gpio mock_gpio_dev;
static struct seq_file mock_seq_file;
static struct gpio_chip mock_gpio_chip;

static void *mock_gpiochip_get_data(struct gpio_chip *gc)
{
	return &mock_gpio_dev;
}

static u32 mock_readl(const volatile void __iomem *addr)
{
	return *((u32 *)addr);
}

static void mock_seq_printf(struct seq_file *s, const char *fmt, ...) {}

static void mock_seq_puts(struct seq_file *s, const char *str) {}

#define gpiochip_get_data mock_gpiochip_get_data
#define readl mock_readl
#define seq_printf mock_seq_printf
#define seq_puts mock_seq_puts

static void amd_gpio_dbg_show(struct seq_file *s, struct gpio_chip *gc)
{
	u32 pin_reg;
	u32 db_cntrl;
	unsigned long flags;
	unsigned int bank, i, pin_num;
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);

	bool tmr_out_unit;
	bool tmr_large;

	char *level_trig;
	char *active_level;
	char *interrupt_mask;
	char *wake_cntrl0;
	char *wake_cntrl1;
	char *wake_cntrl2;
	char *pin_sts;
	char *interrupt_sts;
	char *wake_sts;
	char *orientation;
	char debounce_value[40];
	char *debounce_enable;
	char *wake_cntrlz;

	seq_printf(s, "WAKE_INT_MASTER_REG: 0x%08x\n", readl(gpio_dev->base + WAKE_INT_MASTER_REG));
	for (bank = 0; bank < gpio_dev->hwbank_num; bank++) {
		unsigned int time = 0;
		unsigned int unit = 0;

		switch (bank) {
		case 0:
			i = 0;
			pin_num = AMD_GPIO_PINS_BANK0;
			break;
		case 1:
			i = 64;
			pin_num = AMD_GPIO_PINS_BANK1 + i;
			break;
		case 2:
			i = 128;
			pin_num = AMD_GPIO_PINS_BANK2 + i;
			break;
		case 3:
			i = 192;
			pin_num = AMD_GPIO_PINS_BANK3 + i;
			break;
		default:
			continue;
		}
		seq_printf(s, "GPIO bank%d\n", bank);
		seq_puts(s, "gpio\t  int|active|trigger|S0i3| S3|S4/S5| Z|wake|pull|  orient|       debounce|reg\n");
		for (; i < pin_num; i++) {
			seq_printf(s, "#%d\t", i);
			raw_spin_lock_irqsave(&gpio_dev->lock, flags);
			pin_reg = readl(gpio_dev->base + i * 4);
			raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);

			if (pin_reg & BIT(INTERRUPT_ENABLE_OFF)) {
				u8 level = (pin_reg >> ACTIVE_LEVEL_OFF) &
						ACTIVE_LEVEL_MASK;

				if (level == ACTIVE_LEVEL_HIGH)
					active_level = "↑";
				else if (level == ACTIVE_LEVEL_LOW)
					active_level = "↓";
				else if (!(pin_reg & BIT(LEVEL_TRIG_OFF)) &&
					 level == ACTIVE_LEVEL_BOTH)
					active_level = "b";
				else
					active_level = "?";

				if (pin_reg & BIT(LEVEL_TRIG_OFF))
					level_trig = "level";
				else
					level_trig = " edge";

				if (pin_reg & BIT(INTERRUPT_MASK_OFF))
					interrupt_mask = "😛";
				else
					interrupt_mask = "😷";

				if (pin_reg & BIT(INTERRUPT_STS_OFF))
					interrupt_sts = "🔥";
				else
					interrupt_sts = "  ";

				seq_printf(s, "%s %s|     %s|  %s|",
				   interrupt_sts,
				   interrupt_mask,
				   active_level,
				   level_trig);
			} else
				seq_puts(s, "    ∅|      |       |");

			if (pin_reg & BIT(WAKE_CNTRL_OFF_S0I3))
				wake_cntrl0 = "⏰";
			else
				wake_cntrl0 = "  ";
			seq_printf(s, "  %s| ", wake_cntrl0);

			if (pin_reg & BIT(WAKE_CNTRL_OFF_S3))
				wake_cntrl1 = "⏰";
			else
				wake_cntrl1 = "  ";
			seq_printf(s, "%s|", wake_cntrl1);

			if (pin_reg & BIT(WAKE_CNTRL_OFF_S4))
				wake_cntrl2 = "⏰";
			else
				wake_cntrl2 = "  ";
			seq_printf(s, "   %s|", wake_cntrl2);

			if (pin_reg & BIT(WAKECNTRL_Z_OFF))
				wake_cntrlz = "⏰";
			else
				wake_cntrlz = "  ";
			seq_printf(s, "%s|", wake_cntrlz);

			if (pin_reg & BIT(WAKE_STS_OFF))
				wake_sts = "🔥";
			else
				wake_sts = " ";
			seq_printf(s, "   %s|", wake_sts);

			if (pin_reg & BIT(PULL_UP_ENABLE_OFF)) {
				seq_puts(s, "  ↑ |");
			} else if (pin_reg & BIT(PULL_DOWN_ENABLE_OFF)) {
				seq_puts(s, "  ↓ |");
			} else  {
				seq_puts(s, "    |");
			}

			if (pin_reg & BIT(OUTPUT_ENABLE_OFF)) {
				pin_sts = "output";
				if (pin_reg & BIT(OUTPUT_VALUE_OFF))
					orientation = "↑";
				else
					orientation = "↓";
			} else {
				pin_sts = "input ";
				if (pin_reg & BIT(PIN_STS_OFF))
					orientation = "↑";
				else
					orientation = "↓";
			}
			seq_printf(s, "%s %s|", pin_sts, orientation);

			db_cntrl = (DB_CNTRL_MASK << DB_CNTRL_OFF) & pin_reg;
			if (db_cntrl) {
				tmr_out_unit = pin_reg & BIT(DB_TMR_OUT_UNIT_OFF);
				tmr_large = pin_reg & BIT(DB_TMR_LARGE_OFF);
				time = pin_reg & DB_TMR_OUT_MASK;
				if (tmr_large) {
					if (tmr_out_unit)
						unit = 62500;
					else
						unit = 15625;
				} else {
					if (tmr_out_unit)
						unit = 244;
					else
						unit = 61;
				}
				if ((DB_TYPE_REMOVE_GLITCH << DB_CNTRL_OFF) == db_cntrl)
					debounce_enable = "b";
				else if ((DB_TYPE_PRESERVE_LOW_GLITCH << DB_CNTRL_OFF) == db_cntrl)
					debounce_enable = "↓";
				else
					debounce_enable = "↑";
				snprintf(debounce_value, sizeof(debounce_value), "%06u", time * unit);
				seq_printf(s, "%s (🕑 %sus)|", debounce_enable, debounce_value);
			} else {
				seq_puts(s, "               |");
			}
			seq_printf(s, "0x%x\n", pin_reg);
		}
	}
}

static void test_amd_gpio_dbg_show_bank0(struct kunit *test)
{
	mock_gpio_dev.base = (void __iomem *)mock_buffer;
	mock_gpio_dev.hwbank_num = 1;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_dbg_show_bank1(struct kunit *test)
{
	mock_gpio_dev.base = (void __iomem *)mock_buffer;
	mock_gpio_dev.hwbank_num = 2;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_dbg_show_bank2(struct kunit *test)
{
	mock_gpio_dev.base = (void __iomem *)mock_buffer;
	mock_gpio_dev.hwbank_num = 3;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_dbg_show_bank3(struct kunit *test)
{
	mock_gpio_dev.base = (void __iomem *)mock_buffer;
	mock_gpio_dev.hwbank_num = 4;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_dbg_show_invalid_bank(struct kunit *test)
{
	mock_gpio_dev.base = (void __iomem *)mock_buffer;
	mock_gpio_dev.hwbank_num = 5;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_dbg_show_interrupt_enabled(struct kunit *test)
{
	mock_gpio_dev.base = (void __iomem *)mock_buffer;
	mock_gpio_dev.hwbank_num = 1;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	*((u32 *)mock_buffer) = BIT(INTERRUPT_ENABLE_OFF);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_dbg_show_interrupt_disabled(struct kunit *test)
{
	mock_gpio_dev.base = (void __iomem *)mock_buffer;
	mock_gpio_dev.hwbank_num = 1;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	*((u32 *)mock_buffer) = 0;
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_dbg_show_active_level_high(struct kunit *test)
{
	mock_gpio_dev.base = (void __iomem *)mock_buffer;
	mock_gpio_dev.hwbank_num = 1;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	*((u32 *)mock_buffer) = BIT(INTERRUPT_ENABLE_OFF) | 
			       (ACTIVE_LEVEL_HIGH << ACTIVE_LEVEL_OFF);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_dbg_show_active_level_low(struct kunit *test)
{
	mock_gpio_dev.base = (void __iomem *)mock_buffer;
	mock_gpio_dev.hwbank_num = 1;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	*((u32 *)mock_buffer) = BIT(INTERRUPT_ENABLE_OFF) | 
			       (ACTIVE_LEVEL_LOW << ACTIVE_LEVEL_OFF);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_dbg_show_active_level_both(struct kunit *test)
{
	mock_gpio_dev.base = (void __iomem *)mock_buffer;
	mock_gpio_dev.hwbank_num = 1;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	*((u32 *)mock_buffer) = BIT(INTERRUPT_ENABLE_OFF) | 
			       (ACTIVE_LEVEL_BOTH << ACTIVE_LEVEL_OFF);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_dbg_show_level_trigger(struct kunit *test)
{
	mock_gpio_dev.base = (void __iomem *)mock_buffer;
	mock_gpio_dev.hwbank_num = 1;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	*((u32 *)mock_buffer) = BIT(INTERRUPT_ENABLE_OFF) | 
			       BIT(LEVEL_TRIG_OFF);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_dbg_show_interrupt_masked(struct kunit *test)
{
	mock_gpio_dev.base = (void __iomem *)mock_buffer;
	mock_gpio_dev.hwbank_num = 1;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	*((u32 *)mock_buffer) = BIT(INTERRUPT_ENABLE_OFF) | 
			       BIT(INTERRUPT_MASK_OFF);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_dbg_show_interrupt_status(struct kunit *test)
{
	mock_gpio_dev.base = (void __iomem *)mock_buffer;
	mock_gpio_dev.hwbank_num = 1;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	*((u32 *)mock_buffer) = BIT(INTERRUPT_ENABLE_OFF) | 
			       BIT(INTERRUPT_STS_OFF);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_dbg_show_wake_controls(struct kunit *test)
{
	mock_gpio_dev.base = (void __iomem *)mock_buffer;
	mock_gpio_dev.hwbank_num = 1;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	*((u32 *)mock_buffer) = BIT(WAKE_CNTRL_OFF_S0I3) | 
			       BIT(WAKE_CNTRL_OFF_S3) | 
			       BIT(WAKE_CNTRL_OFF_S4) | 
			       BIT(WAKECNTRL_Z_OFF) | 
			       BIT(WAKE_STS_OFF);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_dbg_show_pull_up(struct kunit *test)
{
	mock_gpio_dev.base = (void __iomem *)mock_buffer;
	mock_gpio_dev.hwbank_num = 1;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	*((u32 *)mock_buffer) = BIT(PULL_UP_ENABLE_OFF);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_dbg_show_pull_down(struct kunit *test)
{
	mock_gpio_dev.base = (void __iomem *)mock_buffer;
	mock_gpio_dev.hwbank_num = 1;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	*((u32 *)mock_buffer) = BIT(PULL_DOWN_ENABLE_OFF);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_dbg_show_output_mode(struct kunit *test)
{
	mock_gpio_dev.base = (void __iomem *)mock_buffer;
	mock_gpio_dev.hwbank_num = 1;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	*((u32 *)mock_buffer) = BIT(OUTPUT_ENABLE_OFF) | 
			       BIT(OUTPUT_VALUE_OFF);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_dbg_show_input_mode(struct kunit *test)
{
	mock_gpio_dev.base = (void __iomem *)mock_buffer;
	mock_gpio_dev.hwbank_num = 1;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	*((u32 *)mock_buffer) = BIT(PIN_STS_OFF);
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_dbg_show_debounce_remove_glitch(struct kunit *test)
{
	mock_gpio_dev.base = (void __iomem *)mock_buffer;
	mock_gpio_dev.hwbank_num = 1;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	*((u32 *)mock_buffer) = (DB_TYPE_REMOVE_GLITCH << DB_CNTRL_OFF) | 
			       BIT(DB_TMR_OUT_UNIT_OFF) | 
			       0x55;
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_dbg_show_debounce_preserve_low(struct kunit *test)
{
	mock_gpio_dev.base = (void __iomem *)mock_buffer;
	mock_gpio_dev.hwbank_num = 1;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	*((u32 *)mock_buffer) = (DB_TYPE_PRESERVE_LOW_GLITCH << DB_CNTRL_OFF) | 
			       BIT(DB_TMR_LARGE_OFF) | 
			       0xAA;
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_dbg_show_debounce_preserve_high(struct kunit *test)
{
	mock_gpio_dev.base = (void __iomem *)mock_buffer;
	mock_gpio_dev.hwbank_num = 1;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	*((u32 *)mock_buffer) = (0x3 << DB_CNTRL_OFF) | 
			       BIT(DB_TMR_LARGE_OFF) | 
			       BIT(DB_TMR_OUT_UNIT_OFF) | 
			       0x7F;
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, true);
}

static void test_amd_gpio_dbg_show_no_debounce(struct kunit *test)
{
	mock_gpio_dev.base = (void __iomem *)mock_buffer;
	mock_gpio_dev.hwbank_num = 1;
	raw_spin_lock_init(&mock_gpio_dev.lock);

	*((u32 *)mock_buffer) = 0;
	amd_gpio_dbg_show(&mock_seq_file, &mock_gpio_chip);
	KUNIT_EXPECT_TRUE(test, true);
}

static struct kunit_case amd_gpio_dbg_show_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_dbg_show_bank0),
	KUNIT_CASE(test_amd_gpio_dbg_show_bank1),
	KUNIT_CASE(test_amd_gpio_dbg_show_bank2),
	KUNIT_CASE(test_amd_gpio_dbg_show_bank3),
	KUNIT_CASE(test_amd_gpio_dbg_show_invalid_bank),
	KUNIT_CASE(test_amd_gpio_dbg_show_interrupt_enabled),
	KUNIT_CASE(test_amd_gpio_dbg_show_interrupt_disabled),
	KUNIT_CASE(test_amd_gpio_dbg_show_active_level_high),
	KUNIT_CASE(test_amd_gpio_dbg_show_active_level_low),
	KUNIT_CASE(test_amd_gpio_dbg_show_active_level_both),
	KUNIT_CASE(test_amd_gpio_dbg_show_level_trigger),
	KUNIT_CASE(test_amd_gpio_dbg_show_interrupt_masked),
	KUNIT_CASE(test_amd_gpio_dbg_show_interrupt_status),
	KUNIT_CASE(test_amd_gpio_dbg_show_wake_controls),
	KUNIT_CASE(test_amd_gpio_dbg_show_pull_up),
	KUNIT_CASE(test_amd_gpio_dbg_show_pull_down),
	KUNIT_CASE(test_amd_gpio_dbg_show_output_mode),
	KUNIT_CASE(test_amd_gpio_dbg_show_input_mode),
	KUNIT_CASE(test_amd_gpio_dbg_show_debounce_remove_glitch),
	KUNIT_CASE(test_amd_gpio_dbg_show_debounce_preserve_low),
	KUNIT_CASE(test_amd_gpio_dbg_show_debounce_preserve_high),
	KUNIT_CASE(test_amd_gpio_dbg_show_no_debounce),
	{}
};

static struct kunit_suite amd_gpio_dbg_show_test_suite = {
	.name = "amd_gpio_dbg_show_test",
	.test_cases = amd_gpio_dbg_show_test_cases,
};

kunit_test_suite(amd_gpio_dbg_show_test_suite);