
static int amd_get_group_pins(struct pinctrl_dev *pctldev,
			      unsigned group,
			      const unsigned **pins,
			      unsigned *num_pins)
{
	struct amd_gpio *gpio_dev = pinctrl_dev_get_drvdata(pctldev);

	*pins = gpio_dev->groups[group].pins;
	*num_pins = gpio_dev->groups[group].npins;
	return 0;
}