
		switch (param) {
		case PIN_CONFIG_INPUT_DEBOUNCE:
			ret = amd_gpio_set_debounce(gpio_dev, pin, arg);
			goto out_unlock;

		case PIN_CONFIG_BIAS_PULL_DOWN:
			pin_reg &= ~BIT(PULL_DOWN_ENABLE_OFF);
			pin_reg |= (arg & BIT(0)) << PULL_DOWN_ENABLE_OFF;
			break;

		case PIN_CONFIG_BIAS_PULL_UP:
			pin_reg &= ~BIT(PULL_UP_ENABLE_OFF);
			pin_reg |= (arg & BIT(0)) << PULL_UP_ENABLE_OFF;
			break;

		case PIN_CONFIG_DRIVE_STRENGTH:
			pin_reg &= ~(DRV_STRENGTH_SEL_MASK
					<< DRV_STRENGTH_SEL_OFF);
			pin_reg |= (arg & DRV_STRENGTH_SEL_MASK)
					<< DRV_STRENGTH_SEL_OFF;
			break;

		default:
			dev_dbg(&gpio_dev->pdev->dev,
				"Invalid config param %04x\n", param);
			ret = -ENOTSUPP;
		}