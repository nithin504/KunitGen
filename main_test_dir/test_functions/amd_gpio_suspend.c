
static int amd_gpio_suspend(struct device *dev)
{
#ifdef CONFIG_SUSPEND
	pinctrl_dev = dev_get_drvdata(dev);
#endif
	return amd_gpio_suspend_hibernate_common(dev, true);
}