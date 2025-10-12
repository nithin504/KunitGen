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
#define OUTPUT_ENABLE_OFF 6
#define OUTPUT_VALUE_OFF 7
#define PIN_STS_OFF 0
#define DB_CNTRL_OFF 28
#define DB_CNTRl_MASK 0x7
#define DB_TMR_OUT_UNIT_OFF 10
#define DB_TMR_LARGE_OFF 9
#define DB_TMR_OUT_MASK 0xFF
#define DB_TYPE_REMOVE_GLITCH 0x1
#define DB_TYPE_PRESERVE_LOW_GLITCH 0x2

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
	unsigned int hwbank_num;
	struct gpio_chip chip;
};

static struct amd_gpio *gpiochip_get_data(struct gpio_chip *gc)
{
	return container_of(gc, struct amd_gpio, chip);
}

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
			/* Illegal bank number, ignore */
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

			db_cntrl = (DB_CNTRl_MASK << DB_CNTRL_OFF) & pin_reg;
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

static char seq_buffer[16384];
static size_t seq_pos;

static ssize_t seq_write(struct file *file, const char __user *buf, size_t len, loff_t *ppos)
{
	return 0;
}

static ssize_t seq_read(struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
	size_t avail = seq_pos - *ppos;
	if (avail > len)
		avail = len;
	if (avail > 0)
		memcpy(buf, seq_buffer + *ppos, avail);
	*ppos += avail;
	return avail;
}

static const struct seq_operations seq_ops = {
	.start = NULL,
	.next = NULL,
	.stop = NULL,
	.show = NULL,
};

static int seq_open(struct inode *inode, struct file *file)
{
	struct seq_file *m;
	int ret = seq_open_private(file, &seq_ops, sizeof(*m));
	if (ret)
		return ret;
	m = file->private_data;
	m->buf = seq_buffer;
	m->size = sizeof(seq_buffer);
	return 0;
}

static const struct file_operations seq_fops = {
	.owner = THIS_MODULE,
	.open = seq_open,
	.read = seq_read,
	.write = seq_write,
	.llseek = seq_lseek,
	.release = seq_release_private,
};

static void test_amd_gpio_dbg_show_basic(struct kunit *test)
{
	struct amd_gpio gpio_dev = {
		.hwbank_num = 1,
		.base = kunit_kzalloc(test, 4096, GFP_KERNEL),
		.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock),
	};
	struct gpio_chip gc = {};
	gpio_dev.chip = gc;
	struct seq_file s = {};
	s.buf = seq_buffer;
	s.size = sizeof(seq_buffer);
	s.op = &seq_ops;

	writel(0xdeadbeef, gpio_dev.base + WAKE_INT_MASTER_REG);
	writel(0x0, gpio_dev.base + 0);

	amd_gpio_dbg_show(&s, &gpio_dev.chip);

	KUNIT_EXPECT_TRUE(test, strlen(seq_buffer) > 0);
}

static void test_amd_gpio_dbg_show_interrupt_enabled(struct kunit *test)
{
	struct amd_gpio gpio_dev = {
		.hwbank_num = 1,
		.base = kunit_kzalloc(test, 4096, GFP_KERNEL),
		.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock),
	};
	struct gpio_chip gc = {};
	gpio_dev.chip = gc;
	struct seq_file s = {};
	s.buf = seq_buffer;
	s.size = sizeof(seq_buffer);
	s.op = &seq_ops;

	writel(0xdeadbeef, gpio_dev.base + WAKE_INT_MASTER_REG);
	writel(BIT(INTERRUPT_ENABLE_OFF), gpio_dev.base + 0);

	amd_gpio_dbg_show(&s, &gpio_dev.chip);

	KUNIT_EXPECT_TRUE(test, strlen(seq_buffer) > 0);
	KUNIT_EXPECT_PTR_NE(test, strstr(seq_buffer, "🔥"), NULL);
}

static void test_amd_gpio_dbg_show_active_high_level(struct kunit *test)
{
	struct amd_gpio gpio_dev = {
		.hwbank_num = 1,
		.base = kunit_kzalloc(test, 4096, GFP_KERNEL),
		.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock),
	};
	struct gpio_chip gc = {};
	gpio_dev.chip = gc;
	struct seq_file s = {};
	s.buf = seq_buffer;
	s.size = sizeof(seq_buffer);
	s.op = &seq_ops;

	writel(0xdeadbeef, gpio_dev.base + WAKE_INT_MASTER_REG);
	u32 reg_val = BIT(INTERRUPT_ENABLE_OFF) |
	              (ACTIVE_LEVEL_HIGH << ACTIVE_LEVEL_OFF) |
	              BIT(LEVEL_TRIG_OFF);
	writel(reg_val, gpio_dev.base + 0);

	amd_gpio_dbg_show(&s, &gpio_dev.chip);

	KUNIT_EXPECT_TRUE(test, strlen(seq_buffer) > 0);
	KUNIT_EXPECT_PTR_NE(test, strstr(seq_buffer, "↑"), NULL);
	KUNIT_EXPECT_PTR_NE(test, strstr(seq_buffer, "level"), NULL);
}

static void test_amd_gpio_dbg_show_edge_triggered_both_levels(struct kunit *test)
{
	struct amd_gpio gpio_dev = {
		.hwbank_num = 1,
		.base = kunit_kzalloc(test, 4096, GFP_KERNEL),
		.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock),
	};
	struct gpio_chip gc = {};
	gpio_dev.chip = gc;
	struct seq_file s = {};
	s.buf = seq_buffer;
	s.size = sizeof(seq_buffer);
	s.op = &seq_ops;

	writel(0xdeadbeef, gpio_dev.base + WAKE_INT_MASTER_REG);
	u32 reg_val = BIT(INTERRUPT_ENABLE_OFF) |
	              (ACTIVE_LEVEL_BOTH << ACTIVE_LEVEL_OFF);
	writel(reg_val, gpio_dev.base + 0);

	amd_gpio_dbg_show(&s, &gpio_dev.chip);

	KUNIT_EXPECT_TRUE(test, strlen(seq_buffer) > 0);
	KUNIT_EXPECT_PTR_NE(test, strstr(seq_buffer, "b"), NULL);
	KUNIT_EXPECT_PTR_NE(test, strstr(seq_buffer, " edge"), NULL);
}

static void test_amd_gpio_dbg_show_pull_up(struct kunit *test)
{
	struct amd_gpio gpio_dev = {
		.hwbank_num = 1,
		.base = kunit_kzalloc(test, 4096, GFP_KERNEL),
		.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock),
	};
	struct gpio_chip gc = {};
	gpio_dev.chip = gc;
	struct seq_file s = {};
	s.buf = seq_buffer;
	s.size = sizeof(seq_buffer);
	s.op = &seq_ops;

	writel(0xdeadbeef, gpio_dev.base + WAKE_INT_MASTER_REG);
	writel(BIT(PULL_UP_ENABLE_OFF), gpio_dev.base + 0);

	amd_gpio_dbg_show(&s, &gpio_dev.chip);

	KUNIT_EXPECT_TRUE(test, strlen(seq_buffer) > 0);
	KUNIT_EXPECT_PTR_NE(test, strstr(seq_buffer, "  ↑ "), NULL);
}

static void test_amd_gpio_dbg_show_output_high(struct kunit *test)
{
	struct amd_gpio gpio_dev = {
		.hwbank_num = 1,
		.base = kunit_kzalloc(test, 4096, GFP_KERNEL),
		.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock),
	};
	struct gpio_chip gc = {};
	gpio_dev.chip = gc;
	struct seq_file s = {};
	s.buf = seq_buffer;
	s.size = sizeof(seq_buffer);
	s.op = &seq_ops;

	writel(0xdeadbeef, gpio_dev.base + WAKE_INT_MASTER_REG);
	u32 reg_val = BIT(OUTPUT_ENABLE_OFF) | BIT(OUTPUT_VALUE_OFF);
	writel(reg_val, gpio_dev.base + 0);

	amd_gpio_dbg_show(&s, &gpio_dev.chip);

	KUNIT_EXPECT_TRUE(test, strlen(seq_buffer) > 0);
	KUNIT_EXPECT_PTR_NE(test, strstr(seq_buffer, "output ↑"), NULL);
}

static void test_amd_gpio_dbg_show_debounce_preserve_low_glitch(struct kunit *test)
{
	struct amd_gpio gpio_dev = {
		.hwbank_num = 1,
		.base = kunit_kzalloc(test, 4096, GFP_KERNEL),
		.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock),
	};
	struct gpio_chip gc = {};
	gpio_dev.chip = gc;
	struct seq_file s = {};
	s.buf = seq_buffer;
	s.size = sizeof(seq_buffer);
	s.op = &seq_ops;

	writel(0xdeadbeef, gpio_dev.base + WAKE_INT_MASTER_REG);
	u32 reg_val = (DB_TYPE_PRESERVE_LOW_GLITCH << DB_CNTRL_OFF) |
	              BIT(DB_TMR_OUT_UNIT_OFF) |
	              (10 & DB_TMR_OUT_MASK);
	writel(reg_val, gpio_dev.base + 0);

	amd_gpio_dbg_show(&s, &gpio_dev.chip);

	KUNIT_EXPECT_TRUE(test, strlen(seq_buffer) > 0);
	KUNIT_EXPECT_PTR_NE(test, strstr(seq_buffer, "↓ (🕑 002440us)"), NULL);
}

static void test_amd_gpio_dbg_show_multiple_banks(struct kunit *test)
{
	struct amd_gpio gpio_dev = {
		.hwbank_num = 2,
		.base = kunit_kzalloc(test, 8192, GFP_KERNEL),
		.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock),
	};
	struct gpio_chip gc = {};
	gpio_dev.chip = gc;
	struct seq_file s = {};
	s.buf = seq_buffer;
	s.size = sizeof(seq_buffer);
	s.op = &seq_ops;

	writel(0xdeadbeef, gpio_dev.base + WAKE_INT_MASTER_REG);
	writel(0x0, gpio_dev.base + 0);
	writel(0x0, gpio_dev.base + 64 * 4);

	amd_gpio_dbg_show(&s, &gpio_dev.chip);

	KUNIT_EXPECT_TRUE(test, strlen(seq_buffer) > 0);
	KUNIT_EXPECT_PTR_NE(test, strstr(seq_buffer, "GPIO bank0"), NULL);
	KUNIT_EXPECT_PTR_NE(test, strstr(seq_buffer, "GPIO bank1"), NULL);
}

static void test_amd_gpio_dbg_show_invalid_bank_ignored(struct kunit *test)
{
	struct amd_gpio gpio_dev = {
		.hwbank_num = 5,
		.base = kunit_kzalloc(test, 16384, GFP_KERNEL),
		.lock = __RAW_SPIN_LOCK_UNLOCKED(gpio_dev.lock),
	};
	struct gpio_chip gc = {};
	gpio_dev.chip = gc;
	struct seq_file s = {};
	s.buf = seq_buffer;
	s.size = sizeof(seq_buffer);
	s.op = &seq_ops;

	writel(0xdeadbeef, gpio_dev.base + WAKE_INT_MASTER_REG);
	writel(0x0, gpio_dev.base + 0);

	amd_gpio_dbg_show(&s, &gpio_dev.chip);

	KUNIT_EXPECT_TRUE(test, strlen(seq_buffer) > 0);
	KUNIT_EXPECT_PTR_NE(test, strstr(seq_buffer, "GPIO bank0"), NULL);
	KUNIT_EXPECT_PTR_NE(test, strstr(seq_buffer, "GPIO bank1"), NULL);
	KUNIT_EXPECT_PTR_NE(test, strstr(seq_buffer, "GPIO bank2"), NULL);
	KUNIT_EXPECT_PTR_NE(test, strstr(seq_buffer, "GPIO bank3"), NULL);
	KUNIT_EXPECT_PTR_EQ(test, strstr(seq_buffer, "GPIO bank4"), NULL);
}

static struct kunit_case amd_gpio_dbg_show_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_dbg_show_basic),
	KUNIT_CASE(test_amd_gpio_dbg_show_interrupt_enabled),
	KUNIT_CASE(test_amd_gpio_dbg_show_active_high_level),
	KUNIT_CASE(test_amd_gpio_dbg_show_edge_triggered_both_levels),
	KUNIT_CASE(test_amd_gpio_dbg_show_pull_up),
	KUNIT_CASE(test_amd_gpio_dbg_show_output_high),
	KUNIT_CASE(test_amd_gpio_dbg_show_debounce_preserve_low_glitch),
	KUNIT_CASE(test_amd_gpio_dbg_show_multiple_banks),
	KUNIT_CASE(test_amd_gpio_dbg_show_invalid_bank_ignored),
	{}
};

static struct kunit_suite amd_gpio_dbg_show_test_suite = {
	.name = "amd_gpio_dbg_show_test",
	.test_cases = amd_gpio_dbg_show_test_cases,
};

kunit_test_suite(amd_gpio_dbg_show_test_suite);
