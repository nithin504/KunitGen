
		for (ind = 0; ind < gpio_dev->groups[group].npins; ind++) {
			if (strncmp(gpio_dev->groups[group].name, "IMX_F", strlen("IMX_F")))
				continue;

			pd = pin_desc_get(gpio_dev->pctrl, gpio_dev->groups[group].pins[ind]);
			pd->mux_owner = gpio_dev->groups[group].name;
		}