
static bool do_amd_gpio_irq_handler(int irq, void *dev_id)
{
	struct amd_gpio *gpio_dev = dev_id;
	struct gpio_chip *gc = &gpio_dev->gc;
	unsigned int i, irqnr;
	unsigned long flags;
	u32 __iomem *regs;
	bool ret = false;
	u32  regval;
	u64 status, mask;

	/* Read the wake status */
	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	status = readl(gpio_dev->base + WAKE_INT_STATUS_REG1);
	status <<= 32;
	status |= readl(gpio_dev->base + WAKE_INT_STATUS_REG0);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);

	/* Bit 0-45 contain the relevant status bits */
	status &= (1ULL << 46) - 1;
	regs = gpio_dev->base;
	for (mask = 1, irqnr = 0; status; mask <<= 1, regs += 4, irqnr += 4) {
		if (!(status & mask))
			continue;
		status &= ~mask;

		/* Each status bit covers four pins */
		for (i = 0; i < 4; i++) {
			regval = readl(regs + i);

			if (regval & PIN_IRQ_PENDING)
				pm_pr_dbg("GPIO %d is active: 0x%x",
					  irqnr + i, regval);

			/* caused wake on resume context for shared IRQ */
			if (irq < 0 && (regval & BIT(WAKE_STS_OFF)))
				return true;

			if (!(regval & PIN_IRQ_PENDING) ||
			    !(regval & BIT(INTERRUPT_MASK_OFF)))
				continue;
			generic_handle_domain_irq_safe(gc->irq.domain, irqnr + i);

			/* Clear interrupt.
			 * We must read the pin register again, in case the
			 * value was changed while executing
			 * generic_handle_domain_irq() above.
			 * If the line is not an irq, disable it in order to
			 * avoid a system hang caused by an interrupt storm.
			 */
			raw_spin_lock_irqsave(&gpio_dev->lock, flags);
			regval = readl(regs + i);
			if (!gpiochip_line_is_irq(gc, irqnr + i)) {
				regval &= ~BIT(INTERRUPT_MASK_OFF);
				dev_dbg(&gpio_dev->pdev->dev,
					"Disabling spurious GPIO IRQ %d\n",
					irqnr + i);
			} else {
				ret = true;
			}
			writel(regval, regs + i);
			raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);
		}
	}
	/* did not cause wake on resume context for shared IRQ */
	if (irq < 0)
		return false;

	/* Signal EOI to the GPIO unit */
	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	regval = readl(gpio_dev->base + WAKE_INT_MASTER_REG);
	regval |= EOI_MASK;
	writel(regval, gpio_dev->base + WAKE_INT_MASTER_REG);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);

	return ret;
}