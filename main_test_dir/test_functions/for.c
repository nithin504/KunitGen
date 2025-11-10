
	for (i = 0 ; i < AUTO_MODE_MAX ; i++) {
		if (config_store.mode_set[i].fan_control.fan_id == FAN_INDEX_AUTO)
			config_store.mode_set[i].fan_control.manual = false;
		else
			config_store.mode_set[i].fan_control.manual = true;
	}