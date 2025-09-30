
static void amd_gpio_remove(struct platform_device *pdev)
{
	struct amd_gpio *gpio_dev;

	gpio_dev = platform_get_drvdata(pdev);

	gpiochip_remove(&gpio_dev->gc);
	acpi_unregister_wakeup_handler(amd_gpio_check_wake, gpio_dev);
	amd_gpio_unregister_s2idle_ops();
}