
	if (!gpio_dev->iomux_base) {
		dev_err(&gpio_dev->pdev->dev, "iomux function %d group not supported\n", selector);
		return -EINVAL;
	}