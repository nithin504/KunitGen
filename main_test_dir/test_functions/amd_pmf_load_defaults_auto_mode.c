
static void amd_pmf_load_defaults_auto_mode(struct amd_pmf_dev *dev)
{
	struct apmf_auto_mode output;
	struct power_table_control *pwr_ctrl;
	int i;

	apmf_get_auto_mode_def(dev, &output);
	/* time constant */
	config_store.transition[AUTO_TRANSITION_TO_QUIET].time_constant =
								output.balanced_to_quiet;
	config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].time_constant =
								output.balanced_to_perf;
	config_store.transition[AUTO_TRANSITION_FROM_QUIET_TO_BALANCE].time_constant =
								output.quiet_to_balanced;
	config_store.transition[AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE].time_constant =
								output.perf_to_balanced;

	/* power floor */
	config_store.mode_set[AUTO_QUIET].power_floor = output.pfloor_quiet;
	config_store.mode_set[AUTO_BALANCE].power_floor = output.pfloor_balanced;
	config_store.mode_set[AUTO_PERFORMANCE].power_floor = output.pfloor_perf;
	config_store.mode_set[AUTO_PERFORMANCE_ON_LAP].power_floor = output.pfloor_perf;

	/* Power delta for mode change */
	config_store.transition[AUTO_TRANSITION_TO_QUIET].power_delta =
								output.pd_balanced_to_quiet;
	config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].power_delta =
								output.pd_balanced_to_perf;
	config_store.transition[AUTO_TRANSITION_FROM_QUIET_TO_BALANCE].power_delta =
								output.pd_quiet_to_balanced;
	config_store.transition[AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE].power_delta =
								output.pd_perf_to_balanced;

	/* Power threshold */
	amd_pmf_get_power_threshold();

	/* skin temperature limits */
	pwr_ctrl = &config_store.mode_set[AUTO_QUIET].power_control;
	pwr_ctrl->spl = output.spl_quiet;
	pwr_ctrl->sppt = output.sppt_quiet;
	pwr_ctrl->fppt = output.fppt_quiet;
	pwr_ctrl->sppt_apu_only = output.sppt_apu_only_quiet;
	pwr_ctrl->stt_min = output.stt_min_limit_quiet;
	pwr_ctrl->stt_skin_temp[STT_TEMP_APU] = output.stt_apu_quiet;
	pwr_ctrl->stt_skin_temp[STT_TEMP_HS2] = output.stt_hs2_quiet;

	pwr_ctrl = &config_store.mode_set[AUTO_BALANCE].power_control;
	pwr_ctrl->spl = output.spl_balanced;
	pwr_ctrl->sppt = output.sppt_balanced;
	pwr_ctrl->fppt = output.fppt_balanced;
	pwr_ctrl->sppt_apu_only = output.sppt_apu_only_balanced;
	pwr_ctrl->stt_min = output.stt_min_limit_balanced;
	pwr_ctrl->stt_skin_temp[STT_TEMP_APU] = output.stt_apu_balanced;
	pwr_ctrl->stt_skin_temp[STT_TEMP_HS2] = output.stt_hs2_balanced;

	pwr_ctrl = &config_store.mode_set[AUTO_PERFORMANCE].power_control;
	pwr_ctrl->spl = output.spl_perf;
	pwr_ctrl->sppt = output.sppt_perf;
	pwr_ctrl->fppt = output.fppt_perf;
	pwr_ctrl->sppt_apu_only = output.sppt_apu_only_perf;
	pwr_ctrl->stt_min = output.stt_min_limit_perf;
	pwr_ctrl->stt_skin_temp[STT_TEMP_APU] = output.stt_apu_perf;
	pwr_ctrl->stt_skin_temp[STT_TEMP_HS2] = output.stt_hs2_perf;

	pwr_ctrl = &config_store.mode_set[AUTO_PERFORMANCE_ON_LAP].power_control;
	pwr_ctrl->spl = output.spl_perf_on_lap;
	pwr_ctrl->sppt = output.sppt_perf_on_lap;
	pwr_ctrl->fppt = output.fppt_perf_on_lap;
	pwr_ctrl->sppt_apu_only = output.sppt_apu_only_perf_on_lap;
	pwr_ctrl->stt_min = output.stt_min_limit_perf_on_lap;
	pwr_ctrl->stt_skin_temp[STT_TEMP_APU] = output.stt_apu_perf_on_lap;
	pwr_ctrl->stt_skin_temp[STT_TEMP_HS2] = output.stt_hs2_perf_on_lap;

	/* Fan ID */
	config_store.mode_set[AUTO_QUIET].fan_control.fan_id = output.fan_id_quiet;
	config_store.mode_set[AUTO_BALANCE].fan_control.fan_id = output.fan_id_balanced;
	config_store.mode_set[AUTO_PERFORMANCE].fan_control.fan_id = output.fan_id_perf;
	config_store.mode_set[AUTO_PERFORMANCE_ON_LAP].fan_control.fan_id =
									output.fan_id_perf;

	config_store.transition[AUTO_TRANSITION_TO_QUIET].target_mode = AUTO_QUIET;
	config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].target_mode =
								AUTO_PERFORMANCE;
	config_store.transition[AUTO_TRANSITION_FROM_QUIET_TO_BALANCE].target_mode =
									AUTO_BALANCE;
	config_store.transition[AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE].target_mode =
									AUTO_BALANCE;

	config_store.transition[AUTO_TRANSITION_TO_QUIET].shifting_up = false;
	config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].shifting_up = true;
	config_store.transition[AUTO_TRANSITION_FROM_QUIET_TO_BALANCE].shifting_up = true;
	config_store.transition[AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE].shifting_up =
										false;

	for (i = 0 ; i < AUTO_MODE_MAX ; i++) {
		if (config_store.mode_set[i].fan_control.fan_id == FAN_INDEX_AUTO)
			config_store.mode_set[i].fan_control.manual = false;
		else
			config_store.mode_set[i].fan_control.manual = true;
	}

	/* set to initial default values */
	config_store.current_mode = AUTO_BALANCE;
	dev->socket_power_history_idx = -1;

	amd_pmf_dump_auto_mode_defaults(&config_store);
}