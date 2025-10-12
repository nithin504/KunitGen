
static irqreturn_t amd_gpio_irq_handler(int irq, void *dev_id)
{
	return IRQ_RETVAL(do_amd_gpio_irq_handler(irq, dev_id));
}