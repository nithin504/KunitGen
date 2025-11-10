
void amd_pmf_trans_automode(struct amd_pmf_dev *dev, int socket_power, ktime_t time_elapsed_ms)
{
	int avg_power = 0;
	bool update = false;
	int i, j;

	/* Get the average moving average computed by auto mode algorithm */
	avg_power = amd_pmf_get_moving_avg(dev, socket_power);

	for (i = 0; i < AUTO_TRANSITION_MAX; i++) {
		if ((config_store.transition[i].shifting_up && avg_power >=
		     config_store.transition[i].power_threshold) ||
		    (!config_store.transition[i].shifting_up && avg_power <=
		     config_store.transition[i].power_threshold)) {
			if (config_store.transition[i].timer <
			    config_store.transition[i].time_constant)
				config_store.transition[i].timer += time_elapsed_ms;
		} else {
			config_store.transition[i].timer = 0;
		}

		if (config_store.transition[i].timer >=
		    config_store.transition[i].time_constant &&
		    !config_store.transition[i].applied) {
			config_store.transition[i].applied = true;
			update = true;
		} else if (config_store.transition[i].timer <=
			   config_store.transition[i].time_constant &&
			   config_store.transition[i].applied) {
			config_store.transition[i].applied = false;
			update = true;
		}

#ifdef CONFIG_AMD_PMF_DEBUG
		dev_dbg(dev->dev, "[AUTO MODE] average_power : %d mW mode: %s\n", avg_power,
			state_as_str(config_store.current_mode));

		dev_dbg(dev->dev, "[AUTO MODE] time: %lld ms timer: %u ms tc: %u ms\n",
			time_elapsed_ms, config_store.transition[i].timer,
			config_store.transition[i].time_constant);

		dev_dbg(dev->dev, "[AUTO MODE] shiftup: %u pt: %u mW pf: %u mW pd: %u mW\n",
			config_store.transition[i].shifting_up,
			config_store.transition[i].power_threshold,
			config_store.mode_set[i].power_floor,
			config_store.transition[i].power_delta);
#endif
	}

	dev_dbg(dev->dev, "[AUTO_MODE] avg power: %u mW mode: %s\n", avg_power,
		state_as_str(config_store.current_mode));

#ifdef CONFIG_AMD_PMF_DEBUG
	dev_dbg(dev->dev, "[AUTO MODE] priority1: %u priority2: %u priority3: %u priority4: %u\n",
		config_store.transition[0].applied,
		config_store.transition[1].applied,
		config_store.transition[2].applied,
		config_store.transition[3].applied);
#endif

	if (update) {
		for (j = 0; j < AUTO_TRANSITION_MAX; j++) {
			/* Apply the mode with highest priority indentified */
			if (config_store.transition[j].applied) {
				if (config_store.current_mode !=
				    config_store.transition[j].target_mode) {
					config_store.current_mode =
							config_store.transition[j].target_mode;
					dev_dbg(dev->dev, "[AUTO_MODE] moving to mode:%s\n",
						state_as_str(config_store.current_mode));
					amd_pmf_set_automode(dev, config_store.current_mode, NULL);
				}
				break;
			}
		}
	}
}