#include <kunit/test.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

// Mock structures and functions
struct amd_pmf_dev {
	void *dummy;
};

enum auto_mode_type {
	AUTO_QUIET,
	AUTO_BALANCE,
	AUTO_PERFORMANCE,
	AUTO_PERFORMANCE_ON_LAP,
	AUTO_MODE_MAX
};

enum auto_transition_type {
	AUTO_TRANSITION_TO_QUIET,
	AUTO_TRANSITION_TO_PERFORMANCE,
	AUTO_TRANSITION_FROM_QUIET_TO_BALANCE,
	AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE,
	AUTO_TRANSITION_MAX
};

enum stt_temp_index {
	STT_TEMP_APU,
	STT_TEMP_HS2,
	STT_TEMP_MAX
};

#define FAN_INDEX_AUTO 0

struct apmf_auto_mode {
	// Time constants
	u32 balanced_to_quiet;
	u32 balanced_to_perf;
	u32 quiet_to_balanced;
	u32 perf_to_balanced;

	// Power floors
	u32 pfloor_quiet;
	u32 pfloor_balanced;
	u32 pfloor_perf;

	// Power deltas
	u32 pd_balanced_to_quiet;
	u32 pd_balanced_to_perf;
	u32 pd_quiet_to_balanced;
	u32 pd_perf_to_balanced;

	// SPL, SPPT, FPPT, etc.
	u32 spl_quiet;
	u32 sppt_quiet;
	u32 fppt_quiet;
	u32 sppt_apu_only_quiet;
	u32 stt_min_limit_quiet;
	u32 stt_apu_quiet;
	u32 stt_hs2_quiet;

	u32 spl_balanced;
	u32 sppt_balanced;
	u32 fppt_balanced;
	u32 sppt_apu_only_balanced;
	u32 stt_min_limit_balanced;
	u32 stt_apu_balanced;
	u32 stt_hs2_balanced;

	u32 spl_perf;
	u32 sppt_perf;
	u32 fppt_perf;
	u32 sppt_apu_only_perf;
	u32 stt_min_limit_perf;
	u32 stt_apu_perf;
	u32 stt_hs2_perf;

	u32 spl_perf_on_lap;
	u32 sppt_perf_on_lap;
	u32 fppt_perf_on_lap;
	u32 sppt_apu_only_perf_on_lap;
	u32 stt_min_limit_perf_on_lap;
	u32 stt_apu_perf_on_lap;
	u32 stt_hs2_perf_on_lap;

	// Fan IDs
	u32 fan_id_quiet;
	u32 fan_id_balanced;
	u32 fan_id_perf;
};

struct fan_control {
	u32 fan_id;
	bool manual;
};

struct power_table_control {
	u32 spl;
	u32 sppt;
	u32 fppt;
	u32 sppt_apu_only;
	u32 stt_min;
	u32 stt_skin_temp[STT_TEMP_MAX];
};

struct mode_set_config {
	struct fan_control fan_control;
	u32 power_floor;
	struct power_table_control power_control;
};

struct transition_config {
	u32 time_constant;
	u32 power_delta;
	enum auto_mode_type target_mode;
	bool shifting_up;
};

struct config_store {
	enum auto_mode_type current_mode;
	struct mode_set_config mode_set[AUTO_MODE_MAX];
	struct transition_config transition[AUTO_TRANSITION_MAX];
};

static struct config_store config_store;

// Mock function declarations
static void apmf_get_auto_mode_def(struct amd_pmf_dev *dev, struct apmf_auto_mode *output)
{
	memset(output, 0, sizeof(*output));
	// Fill with test values
	output->balanced_to_quiet = 100;
	output->balanced_to_perf = 200;
	output->quiet_to_balanced = 300;
	output->perf_to_balanced = 400;

	output->pfloor_quiet = 10;
	output->pfloor_balanced = 20;
	output->pfloor_perf = 30;

	output->pd_balanced_to_quiet = 5;
	output->pd_balanced_to_perf = 15;
	output->pd_quiet_to_balanced = 25;
	output->pd_perf_to_balanced = 35;

	output->spl_quiet = 1000;
	output->sppt_quiet = 1100;
	output->fppt_quiet = 1200;
	output->sppt_apu_only_quiet = 1300;
	output->stt_min_limit_quiet = 1400;
	output->stt_apu_quiet = 1500;
	output->stt_hs2_quiet = 1600;

	output->spl_balanced = 2000;
	output->sppt_balanced = 2100;
	output->fppt_balanced = 2200;
	output->sppt_apu_only_balanced = 2300;
	output->stt_min_limit_balanced = 2400;
	output->stt_apu_balanced = 2500;
	output->stt_hs2_balanced = 2600;

	output->spl_perf = 3000;
	output->sppt_perf = 3100;
	output->fppt_perf = 3200;
	output->sppt_apu_only_perf = 3300;
	output->stt_min_limit_perf = 3400;
	output->stt_apu_perf = 3500;
	output->stt_hs2_perf = 3600;

	output->spl_perf_on_lap = 3500;
	output->sppt_perf_on_lap = 3600;
	output->fppt_perf_on_lap = 3700;
	output->sppt_apu_only_perf_on_lap = 3800;
	output->stt_min_limit_perf_on_lap = 3900;
	output->stt_apu_perf_on_lap = 4000;
	output->stt_hs2_perf_on_lap = 4100;

	output->fan_id_quiet = 1;
	output->fan_id_balanced = FAN_INDEX_AUTO;
	output->fan_id_perf = 2;
}

static void amd_pmf_get_power_threshold(void)
{
	// No-op in mock
}

static void amd_pmf_dump_auto_mode_defaults(struct config_store *store)
{
	// No-op in mock
}

// Include the function under test
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

// Test cases
static void test_amd_pmf_load_defaults_time_constants(struct kunit *test)
{
	struct amd_pmf_dev dev = {0};
	struct apmf_auto_mode expected;

	apmf_get_auto_mode_def(&dev, &expected);
	amd_pmf_load_defaults_auto_mode(&dev);

	KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_TO_QUIET].time_constant,
			expected.balanced_to_quiet);
	KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].time_constant,
			expected.balanced_to_perf);
	KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_FROM_QUIET_TO_BALANCE].time_constant,
			expected.quiet_to_balanced);
	KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE].time_constant,
			expected.perf_to_balanced);
}

static void test_amd_pmf_load_defaults_power_floors(struct kunit *test)
{
	struct amd_pmf_dev dev = {0};
	struct apmf_auto_mode expected;

	apmf_get_auto_mode_def(&dev, &expected);
	amd_pmf_load_defaults_auto_mode(&dev);

	KUNIT_EXPECT_EQ(test, config_store.mode_set[AUTO_QUIET].power_floor,
			expected.pfloor_quiet);
	KUNIT_EXPECT_EQ(test, config_store.mode_set[AUTO_BALANCE].power_floor,
			expected.pfloor_balanced);
	KUNIT_EXPECT_EQ(test, config_store.mode_set[AUTO_PERFORMANCE].power_floor,
			expected.pfloor_perf);
	KUNIT_EXPECT_EQ(test, config_store.mode_set[AUTO_PERFORMANCE_ON_LAP].power_floor,
			expected.pfloor_perf);
}

static void test_amd_pmf_load_defaults_power_deltas(struct kunit *test)
{
	struct amd_pmf_dev dev = {0};
	struct apmf_auto_mode expected;

	apmf_get_auto_mode_def(&dev, &expected);
	amd_pmf_load_defaults_auto_mode(&dev);

	KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_TO_QUIET].power_delta,
			expected.pd_balanced_to_quiet);
	KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].power_delta,
			expected.pd_balanced_to_perf);
	KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_FROM_QUIET_TO_BALANCE].power_delta,
			expected.pd_quiet_to_balanced);
	KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE].power_delta,
			expected.pd_perf_to_balanced);
}

static void test_amd_pmf_load_defaults_skin_temp_limits_quiet(struct kunit *test)
{
	struct amd_pmf_dev dev = {0};
	struct apmf_auto_mode expected;

	apmf_get_auto_mode_def(&dev, &expected);
	amd_pmf_load_defaults_auto_mode(&dev);

	struct power_table_control *pwr_ctrl = &config_store.mode_set[AUTO_QUIET].power_control;
	KUNIT_EXPECT_EQ(test, pwr_ctrl->spl, expected.spl_quiet);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->sppt, expected.sppt_quiet);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->fppt, expected.fppt_quiet);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->sppt_apu_only, expected.sppt_apu_only_quiet);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->stt_min, expected.stt_min_limit_quiet);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->stt_skin_temp[STT_TEMP_APU], expected.stt_apu_quiet);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->stt_skin_temp[STT_TEMP_HS2], expected.stt_hs2_quiet);
}

static void test_amd_pmf_load_defaults_skin_temp_limits_balance(struct kunit *test)
{
	struct amd_pmf_dev dev = {0};
	struct apmf_auto_mode expected;

	apmf_get_auto_mode_def(&dev, &expected);
	amd_pmf_load_defaults_auto_mode(&dev);

	struct power_table_control *pwr_ctrl = &config_store.mode_set[AUTO_BALANCE].power_control;
	KUNIT_EXPECT_EQ(test, pwr_ctrl->spl, expected.spl_balanced);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->sppt, expected.sppt_balanced);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->fppt, expected.fppt_balanced);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->sppt_apu_only, expected.sppt_apu_only_balanced);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->stt_min, expected.stt_min_limit_balanced);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->stt_skin_temp[STT_TEMP_APU], expected.stt_apu_balanced);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->stt_skin_temp[STT_TEMP_HS2], expected.stt_hs2_balanced);
}

static void test_amd_pmf_load_defaults_skin_temp_limits_performance(struct kunit *test)
{
	struct amd_pmf_dev dev = {0};
	struct apmf_auto_mode expected;

	apmf_get_auto_mode_def(&dev, &expected);
	amd_pmf_load_defaults_auto_mode(&dev);

	struct power_table_control *pwr_ctrl = &config_store.mode_set[AUTO_PERFORMANCE].power_control;
	KUNIT_EXPECT_EQ(test, pwr_ctrl->spl, expected.spl_perf);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->sppt, expected.sppt_perf);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->fppt, expected.fppt_perf);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->sppt_apu_only, expected.sppt_apu_only_perf);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->stt_min, expected.stt_min_limit_perf);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->stt_skin_temp[STT_TEMP_APU], expected.stt_apu_perf);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->stt_skin_temp[STT_TEMP_HS2], expected.stt_hs2_perf);
}

static void test_amd_pmf_load_defaults_skin_temp_limits_performance_on_lap(struct kunit *test)
{
	struct amd_pmf_dev dev = {0};
	struct apmf_auto_mode expected;

	apmf_get_auto_mode_def(&dev, &expected);
	amd_pmf_load_defaults_auto_mode(&dev);

	struct power_table_control *pwr_ctrl = &config_store.mode_set[AUTO_PERFORMANCE_ON_LAP].power_control;
	KUNIT_EXPECT_EQ(test, pwr_ctrl->spl, expected.spl_perf_on_lap);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->sppt, expected.sppt_perf_on_lap);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->fppt, expected.fppt_perf_on_lap);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->sppt_apu_only, expected.sppt_apu_only_perf_on_lap);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->stt_min, expected.stt_min_limit_perf_on_lap);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->stt_skin_temp[STT_TEMP_APU], expected.stt_apu_perf_on_lap);
	KUNIT_EXPECT_EQ(test, pwr_ctrl->stt_skin_temp[STT_TEMP_HS2], expected.stt_hs2_perf_on_lap);
}

static void test_amd_pmf_load_defaults_fan_ids(struct kunit *test)
{
	struct amd_pmf_dev dev = {0};
	struct apmf_auto_mode expected;

	apmf_get_auto_mode_def(&dev, &expected);
	amd_pmf_load_defaults_auto_mode(&dev);

	KUNIT_EXPECT_EQ(test, config_store.mode_set[AUTO_QUIET].fan_control.fan_id,
			expected.fan_id_quiet);
	KUNIT_EXPECT_EQ(test, config_store.mode_set[AUTO_BALANCE].fan_control.fan_id,
			expected.fan_id_balanced);
	KUNIT_EXPECT_EQ(test, config_store.mode_set[AUTO_PERFORMANCE].fan_control.fan_id,
			expected.fan_id_perf);
	KUNIT_EXPECT_EQ(test, config_store.mode_set[AUTO_PERFORMANCE_ON_LAP].fan_control.fan_id,
			expected.fan_id_perf);
}

static void test_amd_pmf_load_defaults_target_modes(struct kunit *test)
{
	struct amd_pmf_dev dev = {0};

	amd_pmf_load_defaults_auto_mode(&dev);

	KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_TO_QUIET].target_mode,
			AUTO_QUIET);
	KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].target_mode,
			AUTO_PERFORMANCE);
	KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_FROM_QUIET_TO_BALANCE].target_mode,
			AUTO_BALANCE);
	KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE].target_mode,
			AUTO_BALANCE);
}

static void test_amd_pmf_load_defaults_shifting_directions(struct kunit *test)
{
	struct amd_pmf_dev dev = {0};

	amd_pmf_load_defaults_auto_mode(&dev);

	KUNIT_EXPECT_FALSE(test, config_store.transition[AUTO_TRANSITION_TO_QUIET].shifting_up);
	KUNIT_EXPECT_TRUE(test, config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].shifting_up);
	KUNIT_EXPECT_TRUE(test, config_store.transition[AUTO_TRANSITION_FROM_QUIET_TO_BALANCE].shifting_up);
	KUNIT_EXPECT_FALSE(test, config_store.transition[AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE].shifting_up);
}

static void test_amd_pmf_load_defaults_manual_fan_control(struct kunit *test)
{
	struct amd_pmf_dev dev = {0};
	struct apmf_auto_mode expected;

	apmf_get_auto_mode_def(&dev, &expected);
	amd_pmf_load_defaults_auto_mode(&dev);

	// Fan ID 1 -> manual = true
	KUNIT_EXPECT_TRUE(test, config_store.mode_set[AUTO_QUIET].fan_control.manual);
	// Fan ID FAN_INDEX_AUTO -> manual = false
	KUNIT_EXPECT_FALSE(test, config_store.mode_set[AUTO_BALANCE].fan_control.manual);
	// Fan ID 2 -> manual = true
	KUNIT_EXPECT_TRUE(test, config_store.mode_set[AUTO_PERFORMANCE].fan_control.manual);
	KUNIT_EXPECT_TRUE(test, config_store.mode_set[AUTO_PERFORMANCE_ON_LAP].fan_control.manual);
}

static void test_amd_pmf_load_defaults_initial_values(struct kunit *test)
{
	struct amd_pmf_dev dev = {0};

	amd_pmf_load_defaults_auto_mode(&dev);

	KUNIT_EXPECT_EQ(test, config_store.current_mode, AUTO_BALANCE);
	KUNIT_EXPECT_EQ(test, dev.socket_power_history_idx, -1);
}

static struct kunit_case amd_pmf_load_defaults_test_cases[] = {
	KUNIT_CASE(test_amd_pmf_load_defaults_time_constants),
	KUNIT_CASE(test_amd_pmf_load_defaults_power_floors),
	KUNIT_CASE(test_amd_pmf_load_defaults_power_deltas),
	KUNIT_CASE(test_amd_pmf_load_defaults_skin_temp_limits_quiet),
	KUNIT_CASE(test_amd_pmf_load_defaults_skin_temp_limits_balance),
	KUNIT_CASE(test_amd_pmf_load_defaults_skin_temp_limits_performance),
	KUNIT_CASE(test_amd_pmf_load_defaults_skin_temp_limits_performance_on_lap),
	KUNIT_CASE(test_amd_pmf_load_defaults_fan_ids),
	KUNIT_CASE(test_amd_pmf_load_defaults_target_modes),
	KUNIT_CASE(test_amd_pmf_load_defaults_shifting_directions),
	KUNIT_CASE(test_amd_pmf_load_defaults_manual_fan_control),
	KUNIT_CASE(test_amd_pmf_load_defaults_initial_values),
	{}
};

static struct kunit_suite amd_pmf_load_defaults_test_suite = {
	.name = "amd_pmf_load_defaults_test",
	.test_cases = amd_pmf_load_defaults_test_cases,
};

kunit_test_suite(amd_pmf_load_defaults_test_suite);

MODULE_LICENSE("GPL");