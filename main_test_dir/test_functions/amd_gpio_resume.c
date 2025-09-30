
static int amd_gpio_resume(struct device *dev)
{
	struct amd_gpio *gpio_dev = dev_get_drvdata(dev);
	const struct pinctrl_desc *desc = gpio_dev->pctrl->desc;
	unsigned long flags;
	int i;

	for (i = 0; i < desc->npins; i++) {
		int pin = desc->pins[i].number;

		if (!amd_gpio_should_save(gpio_dev, pin))
			continue;

		raw_spin_lock_irqsave(&gpio_dev->lock, flags);
		gpio_dev->saved_regs[i] |= readl(gpio_dev->base + pin * 4) & PIN_IRQ_PENDING;
		writel(gpio_dev->saved_regs[i], gpio_dev->base + pin * 4);
		raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);
	}

	return 0;
}