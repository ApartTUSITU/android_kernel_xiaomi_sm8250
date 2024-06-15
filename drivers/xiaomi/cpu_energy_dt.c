/**
 * Copyright (c) Xiaomi Technologies Co., Ltd. 2019. All rights reserved.
 * File name: cpu_energy_dt.c
 * Descrviption: Migt Energy aware loading driver
 * Author: guchao1@xiaomi.com
 * Version: 1.0
 * Date:  2019/09/25
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) "cpu_energy_init: " fmt

#include <linux/cpufreq.h>
#include <linux/cpumask.h>
#include <linux/cpuset.h>
#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/pm_opp.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/arch_topology.h>

struct ea_state {
	unsigned long frequency;
	unsigned long power;
	unsigned long capacity;
};

struct id_state {
	unsigned long power; /* power consumption in this idle state */
};

static cpumask_var_t cpus_visit;
static DEFINE_PER_CPU(struct ea_state *, cpu_eas);
static DEFINE_PER_CPU(unsigned long, nr_cap_stats);
static DEFINE_PER_CPU(struct id_state *, cpu_ids);
static DEFINE_PER_CPU(unsigned long, nr_ids_stats);
static void finish_eas_workfn(struct work_struct *work);
static DECLARE_WORK(finish_eas_work, finish_eas_workfn);
static DEFINE_MUTEX(eas_loading_mutex);

static unsigned long get_cpu_ori_cap(int cpu)
{
	return (per_cpu(max_freq_scale, cpu) * per_cpu(cpu_scale, cpu)) >>
	       SCHED_CAPACITY_SHIFT;
}

static int init_energy_callback(struct notifier_block *nb, unsigned long val,
				void *data)
{
	unsigned long cap_nstats;
	unsigned long capacity, max_freq, freq;
	struct cpufreq_policy *policy = data;
	struct device *cpu_dev;
	struct ea_state *eas;
	int cpu, i, ret = 0;

	if (val != CPUFREQ_NOTIFY)
		return 0;

	mutex_lock(&eas_loading_mutex);

	/* Do not register twice*/
	for_each_cpu (cpu, policy->cpus) {
		if (per_cpu(nr_cap_stats, cpu) || per_cpu(cpu_eas, cpu) ||
		    per_cpu(nr_ids_stats, cpu) || per_cpu(cpu_ids, cpu)) {
			ret = -EEXIST;
			goto unlock;
		}
	}

	cpu = cpumask_first(policy->cpus);
	cpu_dev = get_cpu_device(cpu);
	if (!cpu_dev) {
		pr_warn("CPU device missing for CPU %d\n", cpu);
		ret = -ENODEV;
		goto unlock;
	}

	capacity = get_cpu_ori_cap(cpu);

	cap_nstats = dev_pm_opp_get_opp_count(cpu_dev);
	if (cap_nstats <= 0)
		pr_err("OPP table is not ready\n");

	eas = kcalloc(cap_nstats, sizeof(struct ea_state), GFP_KERNEL);
	if (!eas) {
		ret = -ENOMEM;
		goto unlock;
	}

	for (i = 0, freq = 0; i < cap_nstats; i++, freq++) {
		ret = of_dev_pm_opp_get_cpu_power(&eas[i].power, &freq, cpu);
		if (unlikely(ret)) {
			kfree(eas);
			pr_err("pd%d: invalid cap. state: %d\n", cpu, ret);
			goto unlock;
		}

		eas[i].frequency = freq;
	}

	max_freq = eas[cap_nstats - 1].frequency;
	if (!max_freq) {
		kfree(eas);
		ret = -EINVAL;
		goto unlock;
	}

	for (i = 0; i < cap_nstats; i++) {
		eas[i].capacity = eas[i].frequency * capacity / max_freq;
		pr_info("C %8lu F %8lu P %8lu\n", eas[i].capacity,
			eas[i].frequency, eas[i].power);
	}

	for_each_cpu (i, policy->cpus) {
		per_cpu(nr_cap_stats, i) = cap_nstats;
		per_cpu(cpu_eas, i) = eas;
	}

	pr_info("Registering EAS of %*pbl\n", cpumask_pr_args(policy->cpus));

	cpumask_andnot(cpus_visit, cpus_visit, policy->cpus);
	if (cpumask_empty(cpus_visit))
		schedule_work(&finish_eas_work);

unlock:
	mutex_unlock(&eas_loading_mutex);

	return ret;
}

static struct notifier_block init_eas_notifier = {
	.notifier_call = init_energy_callback,
};

static void finish_eas_workfn(struct work_struct *work)
{
	cpufreq_unregister_notifier(&init_eas_notifier,
				    CPUFREQ_POLICY_NOTIFIER);
	free_cpumask_var(cpus_visit);
}

static int __init cpu_pcost_init(void)
{
	int ret;

	if (!alloc_cpumask_var(&cpus_visit, GFP_KERNEL))
		return -ENOMEM;

	cpumask_copy(cpus_visit, cpu_possible_mask);

	ret = cpufreq_register_notifier(&init_eas_notifier,
					CPUFREQ_POLICY_NOTIFIER);
	pr_err("%s suceess\n", __func__);

	if (ret)
		free_cpumask_var(cpus_visit);

	return ret;
}
core_initcall(cpu_pcost_init);

/*--------interface-------*/
void cpu_cap_dt(int cpu, unsigned int freq_index, unsigned int *power,
		unsigned int *freq, unsigned int *cap)
{
	if (unlikely(freq_index >= per_cpu(nr_cap_stats, cpu)))
		freq_index = per_cpu(nr_cap_stats, cpu) - 1;

	if (power)
		*power = per_cpu(cpu_eas, cpu)[freq_index].power;
	if (freq)
		*freq = per_cpu(cpu_eas, cpu)[freq_index].frequency;
	if (cap)
		*cap = per_cpu(cpu_eas, cpu)[freq_index].capacity;
}

void cpu_ids_dt(int cpu, unsigned int idle_index, unsigned int *power)
{
	if (power)
		*power = 0;
}

unsigned long get_nr_cap_stats(int cpu)
{
	return per_cpu(nr_cap_stats, cpu);
}

unsigned long get_nr_id_stats(int cpu)
{
	return per_cpu(nr_ids_stats, cpu);
}
