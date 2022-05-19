/* drivers/gpu/arm/.../platform/gpu_dvfs_handler.c
 *
 * Copyright 2011 by S.LSI. Samsung Electronics Inc.
 * San#24, Nongseo-Dong, Giheung-Gu, Yongin, Korea
 *
 * Samsung SoC Mali-T Series DVFS driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software FoundatIon.
 */

/**
 * @file gpu_dvfs_handler.c
 * DVFS
 */

#include <mali_kbase.h>

#include "mali_kbase_platform.h"
#include "gpu_control.h"
#include "gpu_dvfs_handler.h"
#include "gpu_dvfs_governor.h"

extern struct kbase_device *pkbdev;

#ifdef CONFIG_MALI_DVFS
int kbase_platform_dvfs_event(struct kbase_device *kbdev, u32 utilisation)
{
	struct exynos_context *platform;
	char *env[2] = {"FEATURE=GPUI", NULL};
#ifdef CONFIG_MALI_SEC_G3D_PERF_STABLE_PMQOS
	unsigned long util_mul_freq;
#endif

	platform = (struct exynos_context *) kbdev->platform_context;

	DVFS_ASSERT(platform);

	if(platform->fault_count >= 5 && platform->bigdata_uevent_is_sent == false)
	{
		platform->bigdata_uevent_is_sent = true;
		kobject_uevent_env(&kbdev->dev->kobj, KOBJ_CHANGE, env);
	}

	mutex_lock(&platform->gpu_dvfs_handler_lock);
	if (gpu_control_is_power_on(kbdev)) {
		int clk = 0;
		gpu_dvfs_calculate_env_data(kbdev);
		clk = gpu_dvfs_decide_next_freq(kbdev, platform->env_data.utilization);
#ifdef CONFIG_MALI_SEC_G3D_PERF_STABLE_PMQOS
		util_mul_freq = (unsigned long)clk * platform->env_data.utilization;
#ifdef CONFIG_MALI_SEC_CL_BOOST
		if (!kbdev->pm.backend.metrics.is_full_compute_util && util_mul_freq > STAY_COUNT_NO_PEAK_MODE_UTIL_MUL_FREQ) {
#else
		if (util_mul_freq > STAY_COUNT_NO_PEAK_MODE_UTIL_MUL_FREQ) {
#endif
			platform->stay_count_no_peak_mode++;
			if (platform->stay_count_no_peak_mode >= STAY_COUNT_NO_PEAK_MODE_PERIOD)
				platform->stay_count_no_peak_mode = STAY_COUNT_NO_PEAK_MODE_PERIOD;
			GPU_LOG(DVFS_DEBUG, DUMMY, 0u, 0u, "Peak mode count enable start %llu, %d, %d, %d\n", util_mul_freq, platform->env_data.utilization, clk, platform->stay_count_no_peak_mode);
		} else {
			platform->stay_count_no_peak_mode = 0;
			GPU_LOG(DVFS_DEBUG, DUMMY, 0u, 0u, "Peak mode disable %llu, %d, %d, %d\n", util_mul_freq, platform->env_data.utilization, clk, platform->stay_count_no_peak_mode);
		}
#endif
		gpu_set_target_clk_vol(clk, true, false);
	}
	mutex_unlock(&platform->gpu_dvfs_handler_lock);

	GPU_LOG(DVFS_DEBUG, DUMMY, 0u, 0u, "dvfs hanlder is called\n");

	return 0;
}

int gpu_dvfs_handler_init(struct kbase_device *kbdev)
{
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;

	DVFS_ASSERT(platform);

	if (!platform->dvfs_status)
		platform->dvfs_status = true;


#ifdef CONFIG_MALI_PM_QOS
	gpu_pm_qos_command(platform, GPU_CONTROL_PM_QOS_INIT);
#endif /* CONFIG_MALI_PM_QOS */

	gpu_set_target_clk_vol(platform->table[platform->step].clock, false, false);

	GPU_LOG(DVFS_INFO, DUMMY, 0u, 0u, "dvfs handler initialized\n");
	return 0;
}

int gpu_dvfs_handler_deinit(struct kbase_device *kbdev)
{
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;

	DVFS_ASSERT(platform);

	if (platform->dvfs_status)
		platform->dvfs_status = false;

#ifdef CONFIG_MALI_PM_QOS
	gpu_pm_qos_command(platform, GPU_CONTROL_PM_QOS_DEINIT);
#endif /* CONFIG_MALI_PM_QOS */


	GPU_LOG(DVFS_INFO, DUMMY, 0u, 0u, "dvfs handler de-initialized\n");
	return 0;
}
#else
#define gpu_dvfs_event_proc(q) do { } while (0)
int kbase_platform_dvfs_event(struct kbase_device *kbdev, u32 utilisation)
{
	return 0;
}
#endif /* CONFIG_MALI_DVFS */
