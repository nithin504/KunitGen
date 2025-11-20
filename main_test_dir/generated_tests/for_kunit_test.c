#include <kunit/test.h>

#define AUTO_MODE_MAX 10
#define FAN_INDEX_AUTO 0xFF

struct fan_control {
	int fan_id;
	bool manual;
};

struct mode_config {
	struct fan_control fan_control;
};

struct config_store {
	struct mode_config mode_set[AUTO_MODE_MAX];
};

static struct config_store config_store;

static void test_fan_control_manual_flag(struct kunit *test)
{
	int i;

	/* Initialize all entries with default values */
	for (i = 0; i < AUTO_MODE_MAX; i++) {
		config_store.mode_set[i].fan_control.fan_id = i;
		config_store.mode_set[i].fan_control.manual = false;
	}

	/* Set one entry to FAN_INDEX_AUTO */
	config_store.mode_set[5].fan_control.fan_id = FAN_INDEX_AUTO;

	/* Run the code under test */
	for (i = 0 ; i < AUTO_MODE_MAX ; i++) {
		if (config_store.mode_set[i].fan_control.fan_id == FAN_INDEX_AUTO)
			config_store.mode_set[i].fan_control.manual = false;
		else
			config_store.mode_set[i].fan_control.manual = true;
	}

	/* Verify results */
	for (i = 0; i < AUTO_MODE_MAX; i++) {
		if (i == 5) {
			KUNIT_EXPECT_FALSE(test, config_store.mode_set[i].fan_control.manual);
		} else {
			KUNIT_EXPECT_TRUE(test, config_store.mode_set[i].fan_control.manual);
		}
	}
}

static void test_fan_control_all_auto(struct kunit *test)
{
	int i;

	/* Initialize all entries to FAN_INDEX_AUTO */
	for (i = 0; i < AUTO_MODE_MAX; i++) {
		config_store.mode_set[i].fan_control.fan_id = FAN_INDEX_AUTO;
		config_store.mode_set[i].fan_control.manual = true;
	}

	/* Run the code under test */
	for (i = 0 ; i < AUTO_MODE_MAX ; i++) {
		if (config_store.mode_set[i].fan_control.fan_id == FAN_INDEX_AUTO)
			config_store.mode_set[i].fan_control.manual = false;
		else
			config_store.mode_set[i].fan_control.manual = true;
	}

	/* Verify all are set to auto (manual = false) */
	for (i = 0; i < AUTO_MODE_MAX; i++) {
		KUNIT_EXPECT_FALSE(test, config_store.mode_set[i].fan_control.manual);
	}
}

static void test_fan_control_none_auto(struct kunit *test)
{
	int i;

	/* Initialize all entries to non-AUTO values */
	for (i = 0; i < AUTO_MODE_MAX; i++) {
		config_store.mode_set[i].fan_control.fan_id = i;
		config_store.mode_set[i].fan_control.manual = false;
	}

	/* Run the code under test */
	for (i = 0 ; i < AUTO_MODE_MAX ; i++) {
		if (config_store.mode_set[i].fan_control.fan_id == FAN_INDEX_AUTO)
			config_store.mode_set[i].fan_control.manual = false;
		else
			config_store.mode_set[i].fan_control.manual = true;
	}

	/* Verify all are set to manual (manual = true) */
	for (i = 0; i < AUTO_MODE_MAX; i++) {
		KUNIT_EXPECT_TRUE(test, config_store.mode_set[i].fan_control.manual);
	}
}

static struct kunit_case fan_control_test_cases[] = {
	KUNIT_CASE(test_fan_control_manual_flag),
	KUNIT_CASE(test_fan_control_all_auto),
	KUNIT_CASE(test_fan_control_none_auto),
	{}
};

static struct kunit_suite fan_control_test_suite = {
	.name = "fan_control_manual_flag_test",
	.test_cases = fan_control_test_cases,
};

kunit_test_suite(fan_control_test_suite);