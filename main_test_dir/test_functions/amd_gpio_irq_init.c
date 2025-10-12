
static void amd_gpio_irq_init(struct amd_gpio *gpio_dev)
{
	const struct pinctrl_desc *desc = gpio_dev->pctrl->desc;
	unsigned long flags;
	u32 pin_reg, mask;
	int i;

	mask = BIT(WAKE_CNTRL_OFF_S0I3) | BIT(WAKE_CNTRL_OFF_S3) |
		BIT(WAKE_CNTRL_OFF_S4);

	for (i = 0; i < desc->npins; i++) {
		int pin = desc->pins[i].number;
		const struct pin_desc *pd = pin_desc_get(gpio_dev->pctrl, pin);

		if (!pd)
			continue;

		raw_spin_lock_irqsave(&gpio_dev->lock, flags);

		pin_reg = readl(gpio_dev->base + pin * 4);
		pin_reg &= ~mask;
		writel(pin_reg, gpio_dev->base + pin * 4);

		raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);
	}
}