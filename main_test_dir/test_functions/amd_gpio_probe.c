
static int amd_gpio_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *res;
	struct amd_gpio *gpio_dev;
	struct gpio_irq_chip *girq;

	gpio_dev = devm_kzalloc(&pdev->dev,
				sizeof(struct amd_gpio), GFP_KERNEL);
	if (!gpio_dev)
		return -ENOMEM;

	raw_spin_lock_init(&gpio_dev->lock);

	gpio_dev->base = devm_platform_get_and_ioremap_resource(pdev, 0, &res);
	if (IS_ERR(gpio_dev->base)) {
		dev_err(&pdev->dev, "Failed to get gpio io resource.\n");
		return PTR_ERR(gpio_dev->base);
	}

	gpio_dev->irq = platform_get_irq(pdev, 0);
	if (gpio_dev->irq < 0)
		return gpio_dev->irq;

#ifdef CONFIG_SUSPEND
	gpio_dev->saved_regs = devm_kcalloc(&pdev->dev, amd_pinctrl_desc.npins,
					    sizeof(*gpio_dev->saved_regs),
					    GFP_KERNEL);
	if (!gpio_dev->saved_regs)
		return -ENOMEM;
#endif

	gpio_dev->pdev = pdev;
	gpio_dev->gc.get_direction	= amd_gpio_get_direction;
	gpio_dev->gc.direction_input	= amd_gpio_direction_input;
	gpio_dev->gc.direction_output	= amd_gpio_direction_output;
	gpio_dev->gc.get			= amd_gpio_get_value;
	gpio_dev->gc.set			= amd_gpio_set_value;
	gpio_dev->gc.set_config		= amd_gpio_set_config;
	gpio_dev->gc.dbg_show		= amd_gpio_dbg_show;

	gpio_dev->gc.base		= -1;
	gpio_dev->gc.label			= pdev->name;
	gpio_dev->gc.owner			= THIS_MODULE;
	gpio_dev->gc.parent			= &pdev->dev;
	gpio_dev->gc.ngpio			= resource_size(res) / 4;

	gpio_dev->hwbank_num = gpio_dev->gc.ngpio / 64;
	gpio_dev->groups = kerncz_groups;
	gpio_dev->ngroups = ARRAY_SIZE(kerncz_groups);

	amd_pinctrl_desc.name = dev_name(&pdev->dev);
	amd_get_iomux_res(gpio_dev);
	gpio_dev->pctrl = devm_pinctrl_register(&pdev->dev, &amd_pinctrl_desc,
						gpio_dev);
	if (IS_ERR(gpio_dev->pctrl)) {
		dev_err(&pdev->dev, "Couldn't register pinctrl driver\n");
		return PTR_ERR(gpio_dev->pctrl);
	}

	/* Disable and mask interrupts */
	amd_gpio_irq_init(gpio_dev);

	girq = &gpio_dev->gc.irq;
	gpio_irq_chip_set_chip(girq, &amd_gpio_irqchip);
	/* This will let us handle the parent IRQ in the driver */
	girq->parent_handler = NULL;
	girq->num_parents = 0;
	girq->parents = NULL;
	girq->default_type = IRQ_TYPE_NONE;
	girq->handler = handle_simple_irq;

	ret = gpiochip_add_data(&gpio_dev->gc, gpio_dev);
	if (ret)
		return ret;

	ret = gpiochip_add_pin_range(&gpio_dev->gc, dev_name(&pdev->dev),
				0, 0, gpio_dev->gc.ngpio);
	if (ret) {
		dev_err(&pdev->dev, "Failed to add pin range\n");
		goto out2;
	}

	ret = devm_request_irq(&pdev->dev, gpio_dev->irq, amd_gpio_irq_handler,
			       IRQF_SHARED | IRQF_COND_ONESHOT, KBUILD_MODNAME, gpio_dev);
	if (ret)
		goto out2;

	platform_set_drvdata(pdev, gpio_dev);
	acpi_register_wakeup_handler(gpio_dev->irq, amd_gpio_check_wake, gpio_dev);
	amd_gpio_register_s2idle_ops();

	dev_dbg(&pdev->dev, "amd gpio driver loaded\n");
	return ret;

out2:
	gpiochip_remove(&gpio_dev->gc);

	return ret;
}