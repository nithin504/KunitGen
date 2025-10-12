static void amd_gpio_check_pending(void)
{
	struct amd_gpio *gpio_dev = pinctrl_dev;
	const struct pinctrl_desc *desc = gpio_dev->pctrl->desc;
	int i;

	if (!pm_debug_messages_on)
		return;

	for (i = 0; i < desc->npins; i++) {
		int pin = desc->pins[i].number;
		u32 tmp;

		tmp = readl(gpio_dev->base + pin * 4);
		if (tmp & PIN_IRQ_PENDING)
			pm_pr_dbg("%s: GPIO %d is active: 0x%x.\n", __func__, pin, tmp);
	}
}