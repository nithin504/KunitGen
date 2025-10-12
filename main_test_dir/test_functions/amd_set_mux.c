
static int amd_set_mux(struct pinctrl_dev *pctrldev, unsigned int function, unsigned int group)
{
	struct amd_gpio *gpio_dev = pinctrl_dev_get_drvdata(pctrldev);
	struct device *dev = &gpio_dev->pdev->dev;
	struct pin_desc *pd;
	int ind, index;

	if (!gpio_dev->iomux_base)
		return -EINVAL;

	for (index = 0; index < NSELECTS; index++) {
		if (strcmp(gpio_dev->groups[group].name,  pmx_functions[function].groups[index]))
			continue;

		if (readb(gpio_dev->iomux_base + pmx_functions[function].index) ==
				FUNCTION_INVALID) {
			dev_err(dev, "IOMUX_GPIO 0x%x not present or supported\n",
				pmx_functions[function].index);
			return -EINVAL;
		}

		writeb(index, gpio_dev->iomux_base + pmx_functions[function].index);

		if (index != (readb(gpio_dev->iomux_base + pmx_functions[function].index) &
					FUNCTION_MASK)) {
			dev_err(dev, "IOMUX_GPIO 0x%x not present or supported\n",
				pmx_functions[function].index);
			return -EINVAL;
		}

		for (ind = 0; ind < gpio_dev->groups[group].npins; ind++) {
			if (strncmp(gpio_dev->groups[group].name, "IMX_F", strlen("IMX_F")))
				continue;

			pd = pin_desc_get(gpio_dev->pctrl, gpio_dev->groups[group].pins[ind]);
			pd->mux_owner = gpio_dev->groups[group].name;
		}
		break;
	}

	return 0;
}