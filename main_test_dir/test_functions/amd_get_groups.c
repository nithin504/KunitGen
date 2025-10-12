
static int amd_get_groups(struct pinctrl_dev *pctrldev, unsigned int selector,
			  const char * const **groups,
			  unsigned int * const num_groups)
{
	struct amd_gpio *gpio_dev = pinctrl_dev_get_drvdata(pctrldev);

	if (!gpio_dev->iomux_base) {
		dev_err(&gpio_dev->pdev->dev, "iomux function %d group not supported\n", selector);
		return -EINVAL;
	}

	*groups = pmx_functions[selector].groups;
	*num_groups = pmx_functions[selector].ngroups;
	return 0;
}