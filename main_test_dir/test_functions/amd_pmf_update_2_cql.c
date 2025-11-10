
void amd_pmf_update_2_cql(struct amd_pmf_dev *dev, bool is_cql_event)
{
	int mode = config_store.current_mode;

	config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].target_mode =
				   is_cql_event ? AUTO_PERFORMANCE_ON_LAP : AUTO_PERFORMANCE;

	if ((mode == AUTO_PERFORMANCE || mode == AUTO_PERFORMANCE_ON_LAP) &&
	    mode != config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].target_mode) {
		mode = config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].target_mode;
		amd_pmf_set_automode(dev, mode, NULL);
	}
	dev_dbg(dev->dev, "updated CQL thermals\n");
}