
static void amd_pmf_set_automode(struct amd_pmf_dev *dev, int idx,
				 struct auto_mode_mode_config *table)
{
	struct power_table_control *pwr_ctrl = &config_store.mode_set[idx].power_control;

	amd_pmf_send_cmd(dev, SET_SPL, false, pwr_ctrl->spl, NULL);
	amd_pmf_send_cmd(dev, SET_FPPT, false, pwr_ctrl->fppt, NULL);
	amd_pmf_send_cmd(dev, SET_SPPT, false, pwr_ctrl->sppt, NULL);
	amd_pmf_send_cmd(dev, SET_SPPT_APU_ONLY, false, pwr_ctrl->sppt_apu_only, NULL);
	amd_pmf_send_cmd(dev, SET_STT_MIN_LIMIT, false, pwr_ctrl->stt_min, NULL);
	amd_pmf_send_cmd(dev, SET_STT_LIMIT_APU, false,
			 fixp_q88_fromint(pwr_ctrl->stt_skin_temp[STT_TEMP_APU]), NULL);
	amd_pmf_send_cmd(dev, SET_STT_LIMIT_HS2, false,
			 fixp_q88_fromint(pwr_ctrl->stt_skin_temp[STT_TEMP_HS2]), NULL);

	if (is_apmf_func_supported(dev, APMF_FUNC_SET_FAN_IDX))
		apmf_update_fan_idx(dev, config_store.mode_set[idx].fan_control.manual,
				    config_store.mode_set[idx].fan_control.fan_id);
}