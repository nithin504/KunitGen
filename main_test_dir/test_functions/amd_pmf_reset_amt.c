
int amd_pmf_reset_amt(struct amd_pmf_dev *dev)
{
	/*
	 * OEM BIOS implementation guide says that if the auto mode is enabled
	 * the platform_profile registration shall be done by the OEM driver.
	 * There could be cases where both static slider and auto mode BIOS
	 * functions are enabled, in that case enable static slider updates
	 * only if it advertised as supported.
	 */

	if (is_apmf_func_supported(dev, APMF_FUNC_STATIC_SLIDER_GRANULAR)) {
		dev_dbg(dev->dev, "resetting AMT thermals\n");
		amd_pmf_set_sps_power_limits(dev);
	}
	return 0;
}