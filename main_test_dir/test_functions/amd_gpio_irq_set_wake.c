
static int amd_gpio_irq_set_wake(struct irq_data *d, unsigned int on)
{
	u32 pin_reg;
	unsigned long flags;
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);
	u32 wake_mask = BIT(WAKE_CNTRL_OFF_S0I3) | BIT(WAKE_CNTRL_OFF_S3);
	irq_hw_number_t hwirq = irqd_to_hwirq(d);
	int err;

	pm_pr_dbg("Setting wake for GPIO %lu to %s\n",
		   hwirq, str_enable_disable(on));

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + hwirq * 4);

	if (on)
		pin_reg |= wake_mask;
	else
		pin_reg &= ~wake_mask;

	writel(pin_reg, gpio_dev->base + hwirq * 4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);

	if (on)
		err = enable_irq_wake(gpio_dev->irq);
	else
		err = disable_irq_wake(gpio_dev->irq);

	if (err)
		dev_err(&gpio_dev->pdev->dev, "failed to %s wake-up interrupt\n",
			str_enable_disable(on));

	return 0;
}