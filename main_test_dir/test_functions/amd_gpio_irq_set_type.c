
static int amd_gpio_irq_set_type(struct irq_data *d, unsigned int type)
{
	int ret = 0;
	u32 pin_reg, pin_reg_irq_en, mask;
	unsigned long flags;
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + (d->hwirq)*4);

	switch (type & IRQ_TYPE_SENSE_MASK) {
	case IRQ_TYPE_EDGE_RISING:
		pin_reg &= ~BIT(LEVEL_TRIG_OFF);
		pin_reg &= ~(ACTIVE_LEVEL_MASK << ACTIVE_LEVEL_OFF);
		pin_reg |= ACTIVE_HIGH << ACTIVE_LEVEL_OFF;
		irq_set_handler_locked(d, handle_edge_irq);
		break;

	case IRQ_TYPE_EDGE_FALLING:
		pin_reg &= ~BIT(LEVEL_TRIG_OFF);
		pin_reg &= ~(ACTIVE_LEVEL_MASK << ACTIVE_LEVEL_OFF);
		pin_reg |= ACTIVE_LOW << ACTIVE_LEVEL_OFF;
		irq_set_handler_locked(d, handle_edge_irq);
		break;

	case IRQ_TYPE_EDGE_BOTH:
		pin_reg &= ~BIT(LEVEL_TRIG_OFF);
		pin_reg &= ~(ACTIVE_LEVEL_MASK << ACTIVE_LEVEL_OFF);
		pin_reg |= BOTH_EDGES << ACTIVE_LEVEL_OFF;
		irq_set_handler_locked(d, handle_edge_irq);
		break;

	case IRQ_TYPE_LEVEL_HIGH:
		pin_reg |= LEVEL_TRIGGER << LEVEL_TRIG_OFF;
		pin_reg &= ~(ACTIVE_LEVEL_MASK << ACTIVE_LEVEL_OFF);
		pin_reg |= ACTIVE_HIGH << ACTIVE_LEVEL_OFF;
		irq_set_handler_locked(d, handle_level_irq);
		break;

	case IRQ_TYPE_LEVEL_LOW:
		pin_reg |= LEVEL_TRIGGER << LEVEL_TRIG_OFF;
		pin_reg &= ~(ACTIVE_LEVEL_MASK << ACTIVE_LEVEL_OFF);
		pin_reg |= ACTIVE_LOW << ACTIVE_LEVEL_OFF;
		irq_set_handler_locked(d, handle_level_irq);
		break;

	case IRQ_TYPE_NONE:
		break;

	default:
		dev_err(&gpio_dev->pdev->dev, "Invalid type value\n");
		ret = -EINVAL;
	}

	pin_reg |= CLR_INTR_STAT << INTERRUPT_STS_OFF;
	/*
	 * If WAKE_INT_MASTER_REG.MaskStsEn is set, a software write to the
	 * debounce registers of any GPIO will block wake/interrupt status
	 * generation for *all* GPIOs for a length of time that depends on
	 * WAKE_INT_MASTER_REG.MaskStsLength[11:0].  During this period the
	 * INTERRUPT_ENABLE bit will read as 0.
	 *
	 * We temporarily enable irq for the GPIO whose configuration is
	 * changing, and then wait for it to read back as 1 to know when
	 * debounce has settled and then disable the irq again.
	 * We do this polling with the spinlock held to ensure other GPIO
	 * access routines do not read an incorrect value for the irq enable
	 * bit of other GPIOs.  We keep the GPIO masked while polling to avoid
	 * spurious irqs, and disable the irq again after polling.
	 */
	mask = BIT(INTERRUPT_ENABLE_OFF);
	pin_reg_irq_en = pin_reg;
	pin_reg_irq_en |= mask;
	pin_reg_irq_en &= ~BIT(INTERRUPT_MASK_OFF);
	writel(pin_reg_irq_en, gpio_dev->base + (d->hwirq)*4);
	while ((readl(gpio_dev->base + (d->hwirq)*4) & mask) != mask)
		continue;
	writel(pin_reg, gpio_dev->base + (d->hwirq)*4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);

	return ret;
}