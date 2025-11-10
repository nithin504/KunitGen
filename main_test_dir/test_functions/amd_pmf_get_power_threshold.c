
static void amd_pmf_get_power_threshold(void)
{
	config_store.transition[AUTO_TRANSITION_TO_QUIET].power_threshold =
				config_store.mode_set[AUTO_BALANCE].power_floor -
				config_store.transition[AUTO_TRANSITION_TO_QUIET].power_delta;

	config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].power_threshold =
				config_store.mode_set[AUTO_BALANCE].power_floor -
				config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].power_delta;

	config_store.transition[AUTO_TRANSITION_FROM_QUIET_TO_BALANCE].power_threshold =
			config_store.mode_set[AUTO_QUIET].power_floor -
			config_store.transition[AUTO_TRANSITION_FROM_QUIET_TO_BALANCE].power_delta;

	config_store.transition[AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE].power_threshold =
		config_store.mode_set[AUTO_PERFORMANCE].power_floor -
		config_store.transition[AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE].power_delta;

#ifdef CONFIG_AMD_PMF_DEBUG
	pr_debug("[AUTO MODE TO_QUIET] pt: %u mW pf: %u mW pd: %u mW\n",
		 config_store.transition[AUTO_TRANSITION_TO_QUIET].power_threshold,
		 config_store.mode_set[AUTO_BALANCE].power_floor,
		 config_store.transition[AUTO_TRANSITION_TO_QUIET].power_delta);

	pr_debug("[AUTO MODE TO_PERFORMANCE] pt: %u mW pf: %u mW pd: %u mW\n",
		 config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].power_threshold,
		 config_store.mode_set[AUTO_BALANCE].power_floor,
		 config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].power_delta);

	pr_debug("[AUTO MODE QUIET_TO_BALANCE] pt: %u mW pf: %u mW pd: %u mW\n",
		 config_store.transition[AUTO_TRANSITION_FROM_QUIET_TO_BALANCE]
		 .power_threshold,
		 config_store.mode_set[AUTO_QUIET].power_floor,
		 config_store.transition[AUTO_TRANSITION_FROM_QUIET_TO_BALANCE].power_delta);

	pr_debug("[AUTO MODE PERFORMANCE_TO_BALANCE] pt: %u mW pf: %u mW pd: %u mW\n",
		 config_store.transition[AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE]
		 .power_threshold,
		 config_store.mode_set[AUTO_PERFORMANCE].power_floor,
		 config_store.transition[AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE].power_delta);
#endif
}