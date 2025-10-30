
static int pt_gpio_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct pt_gpio_chip *pt_gpio;
	int ret = 0;

	if (!ACPI_COMPANION(dev)) {
		dev_err(dev, "PT GPIO device node not found\n");
		return -ENODEV;
	}

	pt_gpio = devm_kzalloc(dev, sizeof(struct pt_gpio_chip), GFP_KERNEL);
	if (!pt_gpio)
		return -ENOMEM;

	pt_gpio->reg_base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(pt_gpio->reg_base)) {
		dev_err(dev, "Failed to map MMIO resource for PT GPIO.\n");
		return PTR_ERR(pt_gpio->reg_base);
	}

	ret = bgpio_init(&pt_gpio->gc, dev, 4,
			 pt_gpio->reg_base + PT_INPUTDATA_REG,
			 pt_gpio->reg_base + PT_OUTPUTDATA_REG, NULL,
			 pt_gpio->reg_base + PT_DIRECTION_REG, NULL,
			 BGPIOF_READ_OUTPUT_REG_SET);
	if (ret) {
		dev_err(dev, "bgpio_init failed\n");
		return ret;
	}

	pt_gpio->gc.owner            = THIS_MODULE;
	pt_gpio->gc.request          = pt_gpio_request;
	pt_gpio->gc.free             = pt_gpio_free;
	pt_gpio->gc.ngpio            = (uintptr_t)device_get_match_data(dev);

	ret = devm_gpiochip_add_data(dev, &pt_gpio->gc, pt_gpio);
	if (ret) {
		dev_err(dev, "Failed to register GPIO lib\n");
		return ret;
	}

	platform_set_drvdata(pdev, pt_gpio);

	/* initialize register setting */
	writel(0, pt_gpio->reg_base + PT_SYNC_REG);
	writel(0, pt_gpio->reg_base + PT_CLOCKRATE_REG);

	dev_dbg(dev, "PT GPIO driver loaded\n");
	return ret;
}