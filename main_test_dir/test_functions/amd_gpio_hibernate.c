
static int amd_gpio_hibernate(struct device *dev)
{
	return amd_gpio_suspend_hibernate_common(dev, false);
}