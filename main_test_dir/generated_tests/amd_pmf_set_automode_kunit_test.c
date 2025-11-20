#include <kunit/test.h>
#include <linux/module.h>
#include <linux/kernel.h>

// Assuming necessary headers and declarations for amd_pmf_dev, auto_mode_mode_config, etc.
// These would normally come from the driver's header files.
struct amd_pmf_dev;
struct auto_mode_mode_config;
struct power_table_control;

// Mocks for external functions used inside amd_pmf_set_automode
static int send_cmd_call_count = 0;
static int last_cmd_sent = 0;
static int last_arg_sent = 0;

static bool apmf_func_supported_ret = false;
static int apmf_update_fan_idx_call_count = 0;

// Mock implementation of amd_pmf_send_cmd
void amd_pmf_send_cmd(struct amd_pmf_dev *dev, int command, bool flag, int arg, void *ptr)
{
	send_cmd_call_count++;
	last_cmd_sent = command;
	last_arg_sent = arg;
}

// Mock implementation of is_apmf_func_supported
bool is_apmf_func_supported(struct amd_pmf_dev *dev, int func)
{
	return apmf_func_supported_ret;
}

// Mock implementation of apmf_update_fan_idx
void apmf_update_fan_idx(struct amd_pmf_dev *dev, int manual, int fan_id)
{
	apmf_update_fan_idx_call_count++;
}

// Mock implementation of fixp_q88_fromint
int fixp_q88_fromint(int val)
{
	return val << 8; // Simplified Q8.8 fixed point conversion
}

// Constants for commands
#define SET_SPL 1
#define SET_FPPT 2
#define SET_SPPT 3
#define SET_SPPT_APU_ONLY 4
#define SET_STT_MIN_LIMIT 5
#define SET_STT_LIMIT_APU 6
#define SET_STT_LIMIT_HS2 7
#define APMF_FUNC_SET_FAN_IDX 100

// Enums for STT temp indices
enum {
	STT_TEMP_APU,
	STT_TEMP_HS2,
};

// Structures as per usage in function
struct power_table_control {
	int spl;
	int fppt;
	int sppt;
	int sppt_apu_only;
	int stt_min;
	int stt_skin_temp[2]; // For APU and HS2
};

struct fan_control {
	int manual;
	int fan_id;
};

struct mode_config {
	struct power_table_control power_control;
	struct fan_control fan_control;
};

struct config_store_type {
	struct mode_config mode_set[10]; // Assume max 10 modes
} config_store;

// The function under test
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

// Test cases
static void test_amd_pmf_set_automode_commands_sent(struct kunit *test)
{
	struct amd_pmf_dev dev;
	int idx = 0;
	struct auto_mode_mode_config table;

	config_store.mode_set[idx].power_control.spl = 100;
	config_store.mode_set[idx].power_control.fppt = 200;
	config_store.mode_set[idx].power_control.sppt = 300;
	config_store.mode_set[idx].power_control.sppt_apu_only = 400;
	config_store.mode_set[idx].power_control.stt_min = 500;
	config_store.mode_set[idx].power_control.stt_skin_temp[STT_TEMP_APU] = 60;
	config_store.mode_set[idx].power_control.stt_skin_temp[STT_TEMP_HS2] = 70;

	send_cmd_call_count = 0;

	amd_pmf_set_automode(&dev, idx, &table);

	KUNIT_EXPECT_EQ(test, send_cmd_call_count, 7);
}

static void test_amd_pmf_set_automode_spl_command(struct kunit *test)
{
	struct amd_pmf_dev dev;
	int idx = 0;
	struct auto_mode_mode_config table;

	config_store.mode_set[idx].power_control.spl = 123;
	config_store.mode_set[idx].power_control.fppt = 0;
	config_store.mode_set[idx].power_control.sppt = 0;
	config_store.mode_set[idx].power_control.sppt_apu_only = 0;
	config_store.mode_set[idx].power_control.stt_min = 0;
	config_store.mode_set[idx].power_control.stt_skin_temp[STT_TEMP_APU] = 0;
	config_store.mode_set[idx].power_control.stt_skin_temp[STT_TEMP_HS2] = 0;

	last_cmd_sent = 0;
	last_arg_sent = 0;

	amd_pmf_set_automode(&dev, idx, &table);

	KUNIT_EXPECT_EQ(test, last_cmd_sent, SET_SPL);
	KUNIT_EXPECT_EQ(test, last_arg_sent, 123);
}

static void test_amd_pmf_set_automode_stt_limit_apu_fixed_point(struct kunit *test)
{
	struct amd_pmf_dev dev;
	int idx = 0;
	struct auto_mode_mode_config table;

	config_store.mode_set[idx].power_control.spl = 0;
	config_store.mode_set[idx].power_control.fppt = 0;
	config_store.mode_set[idx].power_control.sppt = 0;
	config_store.mode_set[idx].power_control.sppt_apu_only = 0;
	config_store.mode_set[idx].power_control.stt_min = 0;
	config_store.mode_set[idx].power_control.stt_skin_temp[STT_TEMP_APU] = 25; // Should become 25 << 8 = 6400
	config_store.mode_set[idx].power_control.stt_skin_temp[STT_TEMP_HS2] = 0;

	last_cmd_sent = 0;
	last_arg_sent = 0;

	amd_pmf_set_automode(&dev, idx, &table);

	KUNIT_EXPECT_EQ(test, last_cmd_sent, SET_STT_LIMIT_APU);
	KUNIT_EXPECT_EQ(test, last_arg_sent, 6400); // 25 in Q8.8 format
}

static void test_amd_pmf_set_automode_fan_not_supported(struct kunit *test)
{
	struct amd_pmf_dev dev;
	int idx = 0;
	struct auto_mode_mode_config table;

	apmf_func_supported_ret = false;
	apmf_update_fan_idx_call_count = 0;

	config_store.mode_set[idx].fan_control.manual = 1;
	config_store.mode_set[idx].fan_control.fan_id = 2;

	amd_pmf_set_automode(&dev, idx, &table);

	KUNIT_EXPECT_EQ(test, apmf_update_fan_idx_call_count, 0);
}

static void test_amd_pmf_set_automode_fan_supported(struct kunit *test)
{
	struct amd_pmf_dev dev;
	int idx = 0;
	struct auto_mode_mode_config table;

	apmf_func_supported_ret = true;
	apmf_update_fan_idx_call_count = 0;

	config_store.mode_set[idx].fan_control.manual = 1;
	config_store.mode_set[idx].fan_control.fan_id = 2;

	amd_pmf_set_automode(&dev, idx, &table);

	KUNIT_EXPECT_EQ(test, apmf_update_fan_idx_call_count, 1);
}

static struct kunit_case amd_pmf_set_automode_test_cases[] = {
	KUNIT_CASE(test_amd_pmf_set_automode_commands_sent),
	KUNIT_CASE(test_amd_pmf_set_automode_spl_command),
	KUNIT_CASE(test_amd_pmf_set_automode_stt_limit_apu_fixed_point),
	KUNIT_CASE(test_amd_pmf_set_automode_fan_not_supported),
	KUNIT_CASE(test_amd_pmf_set_automode_fan_supported),
	{}
};

static struct kunit_suite amd_pmf_set_automode_test_suite = {
	.name = "amd_pmf_set_automode_test",
	.test_cases = amd_pmf_set_automode_test_cases,
};

kunit_test_suite(amd_pmf_set_automode_test_suite);