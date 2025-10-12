
static int amd_gpio_suspend_hibernate_common(struct device *dev, bool is_suspend)
{
	struct amd_gpio *gpio_dev = dev_get_drvdata(dev);
	const struct pinctrl_desc *desc = gpio_dev->pctrl->desc;
	unsigned long flags;
	int i;
	u32 wake_mask = is_suspend ? WAKE_SOURCE_SUSPEND : WAKE_SOURCE_HIBERNATE;

	for (i = 0; i < desc->npins; i++) {
		int pin = desc->pins[i].number;

		if (!amd_gpio_should_save(gpio_dev, pin))
			continue;

		raw_spin_lock_irqsave(&gpio_dev->lock, flags);
		gpio_dev->saved_regs[i] = readl(gpio_dev->base + pin * 4) & ~PIN_IRQ_PENDING;

		/* mask any interrupts not intended to be a wake source */
		if (!(gpio_dev->saved_regs[i] & wake_mask)) {
			writel(gpio_dev->saved_regs[i] & ~BIT(INTERRUPT_MASK_OFF),
			       gpio_dev->base + pin * 4);
			pm_pr_dbg("Disabling GPIO #%d interrupt for %s.\n",
				  pin, is_suspend ? "suspend" : "hibernate");
		}

		/*
		 * debounce enabled over suspend has shown issues with a GPIO
		 * being unable to wake the system, as we're only interested in
		 * the actual wakeup event, clear it.
		 */
		if (gpio_dev->saved_regs[i] & (DB_CNTRl_MASK << DB_CNTRL_OFF)) {
			amd_gpio_set_debounce(gpio_dev, pin, 0);
			pm_pr_dbg("Clearing debounce for GPIO #%d during %s.\n",
				  pin, is_suspend ? "suspend" : "hibernate");
		}

		raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);
	}

	return 0;
}