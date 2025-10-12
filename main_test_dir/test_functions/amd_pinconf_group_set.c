
static int amd_pinconf_group_set(struct pinctrl_dev *pctldev,
				unsigned group, unsigned long *configs,
				unsigned num_configs)
{
	const unsigned *pins;
	unsigned npins;
	int i, ret;

	ret = amd_get_group_pins(pctldev, group, &pins, &npins);
	if (ret)
		return ret;
	for (i = 0; i < npins; i++) {
		if (amd_pinconf_set(pctldev, pins[i], configs, num_configs))
			return -ENOTSUPP;
	}
	return 0;
}