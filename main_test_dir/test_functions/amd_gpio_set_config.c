
static int amd_gpio_set_config(struct gpio_chip *gc, unsigned int pin,
			       unsigned long config)
{
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);

	return amd_pinconf_set(gpio_dev->pctrl, pin, &config, 1);
}