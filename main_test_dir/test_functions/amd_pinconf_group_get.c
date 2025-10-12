
static int amd_pinconf_group_get(struct pinctrl_dev *pctldev,
				unsigned int group,
				unsigned long *config)
{
	const unsigned *pins;
	unsigned npins;
	int ret;

	ret = amd_get_group_pins(pctldev, group, &pins, &npins);
	if (ret)
		return ret;

	if (amd_pinconf_get(pctldev, pins[0], config))
			return -ENOTSUPP;

	return 0;
}