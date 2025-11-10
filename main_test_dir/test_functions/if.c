				if (config_store.current_mode !=
				    config_store.transition[j].target_mode) {
					config_store.current_mode =
							config_store.transition[j].target_mode;
					dev_dbg(dev->dev, "[AUTO_MODE] moving to mode:%s\n",
						state_as_str(config_store.current_mode));
					amd_pmf_set_automode(dev, config_store.current_mode, NULL);
				}