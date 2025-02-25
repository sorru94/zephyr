/*
 * Copyright (c) 2023 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

// #include <zephyr/debug/thread_analyzer.h>

#define DIE_TEMP_ALIAS(i) DT_ALIAS(_CONCAT(die_temp, i))
#define DIE_TEMPERATURE_SENSOR(i, _)                                                               \
	IF_ENABLED(DT_NODE_EXISTS(DIE_TEMP_ALIAS(i)), (DEVICE_DT_GET(DIE_TEMP_ALIAS(i)),))

/* support up to 16 cpu die temperature sensors */
static const struct device *const sensors[] = {LISTIFY(16, DIE_TEMPERATURE_SENSOR, ())};

static int print_die_temperature(const struct device *dev)
{
	struct sensor_value val;
	int rc;

	/* fetch sensor samples */
	rc = sensor_sample_fetch(dev);
	if (rc) {
		printk("Failed to fetch sample (%d)\n", rc);
		return rc;
	}

	rc = sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &val);
	if (rc) {
		printk("Failed to get data (%d)\n", rc);
		return rc;
	}

	printk("CPU Die temperature[%s]: %.1f °C\n", dev->name, sensor_value_to_double(&val));
	return 0;
}

// static void thread_print_cb(struct thread_analyzer_info *info)
// {
// 	printk("Thread id: %s\n", info->name);
// 	printk("  Thread CPU utilization: %u.\n", info->utilization);
// 	printk("  Total CPU cycles used: %llu.\n", info->usage.total_cycles);
// 	printk("  Current Frame: %llu; Longest Frame: %llu; Average Frame: %llu\n",
// 		info->usage.current_cycles, info->usage.peak_cycles, info->usage.average_cycles);
// }

void cpu_usage_timer_handler(struct k_timer *dummy)
{
	static uint64_t prev_total_cycles = 0U; // Non idle cycles
	static uint64_t prev_idle_cycles = 0U; // idle cycles
	static uint64_t prev_execution_cycles = 0U; // Sum of idle + non idle cycles

	k_thread_runtime_stats_t stats;
	k_thread_runtime_stats_cpu_get(0, &stats);
	printk("CPU 0 stats.\n");
	printk("    CPU idle:      %15llu\n", stats.idle_cycles);
	printk("    CPU current:   %15llu\n", stats.current_cycles);
	printk("    CPU peak:      %15llu\n", stats.peak_cycles);
	printk("    CPU average:   %15llu\n", stats.average_cycles);
	printk("    CPU execution: %15llu\n", stats.execution_cycles);
	printk("    CPU total:     %15llu\n", stats.total_cycles);

	printk("    CPU recent total:      %15llu\n", stats.total_cycles - prev_total_cycles);
	printk("    CPU recent idle:       %15llu\n", stats.idle_cycles - prev_idle_cycles);
	printk("    CPU recent execution:  %15llu\n", stats.execution_cycles - prev_execution_cycles);

	double tot_cpu_usage = 100.0f * stats.total_cycles / stats.execution_cycles;
	double cpu_usage = 100.0f * (stats.total_cycles - prev_total_cycles) / (stats.execution_cycles - prev_execution_cycles);
	printk("    CPU usage since startup: %.2lf\n", tot_cpu_usage);
	printk("    CPU usage: %.2lf\n", cpu_usage);

	prev_total_cycles = stats.total_cycles;
	prev_idle_cycles = stats.idle_cycles;
	prev_execution_cycles = stats.execution_cycles;
}

K_TIMER_DEFINE(cpu_usage_timer, cpu_usage_timer_handler, NULL);

int main(void)
{
	int rc;

	for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
		if (!device_is_ready(sensors[i])) {
			printk("sensor: device %s not ready.\n", sensors[i]->name);
			return 0;
		}
	}

	k_timer_start(&cpu_usage_timer, K_SECONDS(5), K_SECONDS(5));

	while (1) {
		for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
			rc = print_die_temperature(sensors[i]);
			if (rc < 0) {
				return 0;
			}
		}
		// thread_analyzer_run(thread_print_cb, 0);
		// k_thread_runtime_stats_t stats;
		// k_thread_runtime_stats_cpu_get(0, &stats);
		// printk("CPU 0 stats.\n");
		// printk("    CPU idle:      %15llu.\n", stats.idle_cycles);
		// printk("    CPU current:   %15llu.\n", stats.current_cycles);
		// printk("    CPU peak:      %15llu.\n", stats.peak_cycles);
		// printk("    CPU average:   %15llu.\n", stats.average_cycles);
		// printk("    CPU execution: %15llu.\n", stats.execution_cycles);
		// printk("    CPU total:     %15llu.\n", stats.total_cycles);
		k_msleep(5000);
	}
	return 0;
}
