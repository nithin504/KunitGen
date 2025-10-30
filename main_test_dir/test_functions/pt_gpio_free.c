
static void pt_gpio_free(struct gpio_chip *gc, unsigned offset)
{
	struct pt_gpio_chip *pt_gpio = gpiochip_get_data(gc);
	unsigned long flags;
	u32 using_pins;

	raw_spin_lock_irqsave(&gc->bgpio_lock, flags);

	using_pins = readl(pt_gpio->reg_base + PT_SYNC_REG);
	using_pins &= ~BIT(offset);
	writel(using_pins, pt_gpio->reg_base + PT_SYNC_REG);

	raw_spin_unlock_irqrestore(&gc->bgpio_lock, flags);

	dev_dbg(gc->parent, "pt_gpio_free offset=%x\n", offset);
}