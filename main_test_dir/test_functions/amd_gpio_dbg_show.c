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