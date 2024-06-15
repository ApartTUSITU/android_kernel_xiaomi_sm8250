/**
 * Copyright (c) Xiaomi Technologies Co., Ltd. 2019. All rights reserved.
 * File name: cpu_pcost.c
 * Descrviption: cal cpu power cost
 * Author: guchao1@xiaomi.com
 * Version: 1.0
 * Date:  2019/09/26
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define pr_fmt(fmt) "cpu_pcost: " fmt

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cpu.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/security.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/pkg_stat.h>
#include <linux/cpufreq.h>
#include <linux/cpumask.h>
#include <linux/cpuset.h>
#include <linux/gfp.h>
#include <linux/time.h>
#include <linux/tick.h>
#include "cpu_energy_dt.h"

struct cap_state {
	unsigned int freq;
	unsigned int power;
};

struct id_state {
	unsigned int power;
};

struct collet_work {
	char *buf;
	int len;
	int cpu;
	struct delayed_work wk;
};

struct cluster_devinfo {
	raw_spinlock_t lock;
	struct cpumask cpus;
	struct cap_state *cstate;
	struct collet_work cwk;
	int nr_cap_stats;
	u64 *atime;
	u64 *wtime;
	u64 *pcost;
	u64 last_time;
	unsigned int cur_freq;
	int cap_index;
	u8 has_init;
};

struct cpu_state {
	u64 prev_idle_time;
	u64 prev_wtime;
	u64 *atime;
	u64 wtime;
	int cid;
};

#define COLLET_WORK(work) container_of(work, struct collet_work, wk)

#define POWER_WINDOW_SIZE (3600 * HZ)

static int cluster_number;
static int init_cpu_suc[NR_CPUS];
static int init_clus_suc[MAX_CLUSTER];
static unsigned int collet_period_jif = HZ * 600; /*10 min*/
module_param(collet_period_jif, uint, 0644);
static unsigned int auto_collet = 1;
module_param(auto_collet, uint, 0644);
static unsigned int debug_pcost;
module_param(debug_pcost, uint, 0644);
static unsigned int power_unit = 60 * HZ; /*1 min*/
module_param(power_unit, uint, 0644);
static unsigned int disable_pcost = 1;
module_param(disable_pcost, uint, 0644);
static struct cluster_devinfo cluster_info[MAX_CLUSTER];
static DEFINE_PER_CPU(struct cpu_state, cpu_st);
static struct workqueue_struct *ea_wq;

static u64 get_cpu_atime_us(int cpu, u64 *wtime)
{
	int init_flag = 0;
	u64 cpu_idle_time = 0;
	u64 cpu_wtime = 0;
	struct cpu_state *cpu_data = &per_cpu(cpu_st, cpu);

	*wtime = 0;
	cpu_idle_time = get_cpu_idle_time_us(cpu, &cpu_wtime);

	if (debug_pcost)
		pr_info("cpu %d idle utime %15llu wall time %15llu\n", cpu,
			cpu_idle_time, cpu_wtime);

	cpu_idle_time = cpu_idle_time - cpu_data->prev_idle_time;
	cpu_wtime = cpu_wtime - cpu_data->prev_wtime;

	if (unlikely(!cpu_data->prev_wtime))
		init_flag = 1;

	cpu_data->prev_idle_time += cpu_idle_time;
	cpu_data->prev_wtime += cpu_wtime;

	if (unlikely(init_flag)) {
		*wtime = 0;
		return 0;
	}

	if (cpu_wtime < 0 || cpu_wtime < cpu_idle_time)
		return 0;

	*wtime = cpu_wtime;
	return cpu_wtime - cpu_idle_time;
}

static int freq_index(struct cluster_devinfo *cluster, unsigned int freq)
{
	int index;

	for (index = 0; index < cluster->nr_cap_stats; index++)
		if (cluster->cstate[index].freq >= freq)
			return index;

	return cluster->nr_cap_stats - 1;
}

static void clus_power_update(unsigned int new_freq, int src_cpu)
{
	int index, cpu, nr_cap;
	unsigned long flags;
	u64 now, delta, atime[NR_CPUS], wtime[NR_CPUS];
	struct cluster_devinfo *cluster;
	struct cpu_state *cst;

	cst = &per_cpu(cpu_st, src_cpu);
	if (!init_clus_suc[cst->cid])
		return;

	cluster = &cluster_info[cst->cid];
	for_each_cpu (cpu, &cluster->cpus) {
		atime[cpu] = get_cpu_atime_us(cpu, &wtime[cpu]);
		if (!wtime[cpu])
			continue;

		if (unlikely(debug_pcost))
			pr_info(" %d atime %15llu, wtime %15llu\n", cpu,
				atime[cpu], wtime[cpu]);
	}

	now = ktime_get_ns() >> 10; /* us */
	raw_spin_lock_irqsave(&cluster->lock, flags);
	index = cluster->cap_index;
	nr_cap = cluster->nr_cap_stats;
	for_each_cpu (cpu, &cluster->cpus) {
		if (!wtime[cpu])
			continue;

		per_cpu(cpu_st, cpu).atime[index] += atime[cpu];
		per_cpu(cpu_st, cpu).wtime += wtime[cpu];
		cluster->atime[index] += atime[cpu];
		cluster->wtime[index] += wtime[cpu];
	}

	delta = now - cluster->last_time;
	cluster->wtime[nr_cap] += delta;
	cluster->last_time += delta;
	cluster->cur_freq = new_freq;
	cluster->cap_index = freq_index(cluster, new_freq);
	raw_spin_unlock_irqrestore(&cluster->lock, flags);
	return;
}

static int cpufreq_notifier_trans(struct notifier_block *nb, unsigned long val,
				  void *data)
{
	struct cpufreq_freqs *freq = (struct cpufreq_freqs *)data;
	unsigned int cpu = freq->cpu, new_freq = freq->new;

	if (likely(disable_pcost))
		return NOTIFY_OK;

	if (val != CPUFREQ_POSTCHANGE)
		return NOTIFY_DONE;

	clus_power_update(new_freq, cpu);
	return NOTIFY_OK;
}

static struct notifier_block notifier_trans_block = {
	.notifier_call = cpufreq_notifier_trans
};

static void clus_clear_table(struct cluster_devinfo *clus,
			     unsigned int new_freq)
{
	unsigned int nr_cap = clus->nr_cap_stats;
	unsigned long flags;
	u64 now;
	int cpu;

	now = ktime_get_ns() >> 10;
	raw_spin_lock_irqsave(&clus->lock, flags);
	memset(clus->atime, 0, nr_cap * sizeof(u64));
	memset(clus->wtime, 0, (nr_cap + 1) * sizeof(u64));
	memset(clus->pcost, 0, (nr_cap + 1) * sizeof(u64));

	clus->cur_freq = new_freq;
	clus->cap_index = freq_index(clus, new_freq);
	clus->last_time = now;

	for_each_cpu (cpu, &clus->cpus) {
		if (init_cpu_suc[cpu]) {
			memset(per_cpu(cpu_st, cpu).atime, 0,
			       nr_cap * sizeof(u64));
			per_cpu(cpu_st, cpu).prev_idle_time = 0;
			per_cpu(cpu_st, cpu).wtime = 0;
			per_cpu(cpu_st, cpu).prev_wtime = 0;
		}
	}
	raw_spin_unlock_irqrestore(&clus->lock, flags);

	cpu = cpumask_first(&clus->cpus);
	if (cpu < nr_cpu_ids)
		clus_power_update(new_freq, cpu);
}

static ssize_t read_clus_power(int src_cpu, char *buf)
{
	int i, cid = per_cpu(cpu_st, src_cpu).cid;
	struct cluster_devinfo *clus = &cluster_info[cid];
	struct cpufreq_policy *policy;
	int nr_cap = clus->nr_cap_stats;
	unsigned int cpu, cur_freq, len = 0;
	u64 w_time, *wtime, *pcost, *atime, fcost;

	if (!init_clus_suc[cid])
		return snprintf(buf, sizeof(buf), "clus init failed %d\n", cid);

	get_online_cpus();
	policy = cpufreq_cpu_get(src_cpu);
	if (policy) {
		cur_freq = policy->cur;
		cpufreq_cpu_put(policy);
	}
	put_online_cpus();

	if (unlikely(!policy))
		return snprintf(buf, sizeof(buf), "get policy failed %d\n",
				cid);

	if (debug_pcost)
		pr_info("\n-----------\n");

	clus_power_update(cur_freq, src_cpu);
	for_each_cpu (cpu, &clus->cpus) {
		w_time = per_cpu(cpu_st, cpu).wtime;
		atime = per_cpu(cpu_st, cpu).atime;
		if (unlikely(!atime)) {
			pr_err("active time is NULL\n");
			return len;
		}

		for (i = 0; i < nr_cap; i++) {
			if (!debug_pcost)
				break;

			pr_info("%d a %15llu\n, w %15llu\n", i, atime[i],
				w_time);
		}

		len += snprintf(buf + len, PAGE_SIZE - len, "%d: %llu\n", cpu,
				w_time);
	}

	w_time = clus->wtime[nr_cap];
	wtime = clus->wtime;
	atime = clus->atime;
	pcost = clus->pcost;
	pcost[nr_cap] = 0;
	for (i = 0; i < nr_cap; i++) {
		fcost = div64_u64(atime[i] * clus->cstate[i].power, w_time);
		pcost[i] = fcost;
		pcost[nr_cap] += fcost;

		if (debug_pcost)
			pr_info("%2d p: %15llu a: %15llu w: %15llu ap %15llu\n",
				i, clus->cstate[i].power, atime[i], wtime[i],
				pcost[i]);

		len += snprintf(
			buf + len, PAGE_SIZE - len,
			"p %15llu a %15llu w %15llu ap %15llu rp %15llu %d\n",
			clus->cstate[i].power, atime[i], wtime[i], pcost[i],
			div64_u64(atime[i] * clus->cstate[i].power, power_unit),
			(int)div64_u64(atime[i] * 100, wtime[i]));
	}

	if (debug_pcost)
		pr_info("%d: %llu\n", cid, pcost[clus->nr_cap_stats]);

	len += snprintf(buf + len, PAGE_SIZE - len, "cid %d: %llu\n", cid,
			pcost[clus->nr_cap_stats]);
	return len;
}

static ssize_t show_clus_power(struct cpufreq_policy *policy, char *buf)
{
	return read_clus_power(policy->cpu, buf);
}

static ssize_t store_clus_power(struct cpufreq_policy *policy, const char *buf,
				size_t count)
{
	int cid = per_cpu(cpu_st, policy->cpu).cid;
	struct cluster_devinfo *clus = &cluster_info[cid];

	if (init_clus_suc[cid])
		clus_clear_table(clus, policy->cur);
	else
		return count;

	if (auto_collet) {
		clus->cwk.cpu = policy->cpu;
		clus->cwk.len = 0;
		queue_delayed_work(ea_wq, &clus->cwk.wk, collet_period_jif);
	}

	return count;
}

static ssize_t show_collet_power(struct cpufreq_policy *policy, char *buf)
{
	int cid = per_cpu(cpu_st, policy->cpu).cid;
	struct cluster_devinfo *clus = &cluster_info[cid];
	struct collet_work *cwk = &clus->cwk;

	if (!cwk->len)
		return 0;

	memcpy(buf, cwk->buf, cwk->len);

	return cwk->len;
}

static ssize_t show_ea_stat(struct cpufreq_policy *policy, char *buf)
{
	int i, cid = per_cpu(cpu_st, policy->cpu).cid;
	struct cluster_devinfo *clus = &cluster_info[cid];
	int nr_cap = clus->nr_cap_stats;
	int len = 0;

	if (!init_clus_suc[cid])
		return snprintf(buf, sizeof(buf), "clus init failed %d\n", cid);

	for (i = 0; i < nr_cap; i++) {
		if (len >= PAGE_SIZE)
			return len;

		if (debug_pcost)
			pr_info("%d: buf %lu P %8lu, F %8lu  len %d\n", i, buf,
				clus->cstate[i].power, clus->cstate[i].freq,
				len);
		len += snprintf(buf + len, PAGE_SIZE - len, "%2d %8lu %8lu\n",
				i, clus->cstate[i].power, clus->cstate[i].freq);
		if (debug_pcost)
			pr_info("%d: buf %lu P %8lu, F %8lu  len %d\n", i,
				buf + len, clus->cstate[i].power,
				clus->cstate[i].freq, len);
	}

	len += snprintf(buf + len, PAGE_SIZE - len, "\n");

	return len;
}

cpufreq_freq_attr_rw(clus_power);
cpufreq_freq_attr_ro(ea_stat);
cpufreq_freq_attr_ro(collet_power);

static struct attribute *default_attrs[] = { &clus_power.attr, &ea_stat.attr,
					     &collet_power.attr, NULL };

static const struct attribute_group eas_attr_group = { .attrs = default_attrs,
						       .name = "cpu_pcost" };

static void do_collet(struct work_struct *work)
{
	struct delayed_work *dwk =
		container_of(work, struct delayed_work, work);
	struct collet_work *cwk = COLLET_WORK(dwk);

	cwk->len = read_clus_power(cwk->cpu, cwk->buf);
}

void create_cpu_pcost_entry(struct cpufreq_policy *policy)
{
	int rc;

	rc = sysfs_create_group(&policy->kobj, &eas_attr_group);
	if (!rc)
		pr_err("create cpu pcost entry failed\n");
}

static int alloc_percpu_time(int cpu, int nr_cap)
{
	if (!nr_cap)
		return -EPERM;

	per_cpu(cpu_st, cpu).atime = kcalloc(nr_cap, sizeof(u64), GFP_NOWAIT);

	if (unlikely(!per_cpu(cpu_st, cpu).atime)) {
		pr_err("alloc percpu active time err\n");
		return -ENOMEM;
	}

	init_cpu_suc[cpu] = 1;
	pr_info("cpu %d alloc percpu active %lu\n", cpu,
		per_cpu(cpu_st, cpu).atime);

	return 0;
}

static int enery_probe(int cluster)
{
	struct cap_state *cstate;
	int index, cpu, nr_cap, cap_len;
	u64 *wtime;

	cpu = cpumask_first(&cluster_info[cluster].cpus);
	nr_cap = get_nr_cap_stats(cpu);
	cap_len = sizeof(struct cap_state) + sizeof(u64);

	if (cluster_info[cluster].nr_cap_stats != nr_cap) {
		kfree(cluster_info[cluster].cstate);

		cluster_info[cluster].cstate =
			kcalloc(nr_cap, cap_len, GFP_NOWAIT);
	}

	if (!cluster_info[cluster].cstate)
		return -ENOMEM;

	wtime = kcalloc((nr_cap + 1) * 2, sizeof(u64), GFP_NOWAIT);
	if (!wtime) {
		kfree(cluster_info[cluster].cstate);
		return -ENOMEM;
	}

	cluster_info[cluster].wtime = wtime;
	cluster_info[cluster].pcost = (u64 *)(wtime + nr_cap + 1);

	cluster_info[cluster].cwk.buf = (char *)get_zeroed_page(GFP_KERNEL);
	if (!cluster_info[cluster].cwk.buf) {
		kfree(cluster_info[cluster].cstate);
		kfree(cluster_info[cluster].wtime);
		return -ENOMEM;
	}

	INIT_DELAYED_WORK(&cluster_info[cluster].cwk.wk, do_collet);
	cluster_info[cluster].has_init = 1;

	cluster_info[cluster].nr_cap_stats = nr_cap;
	cstate = cluster_info[cluster].cstate;
	cluster_info[cluster].atime = (u64 *)(cstate + nr_cap);

	for (index = 0; index < nr_cap; index++) {
		cpu_cap_dt(cpu, index, &cstate[index].power,
			   &cstate[index].freq, NULL);
		pr_info("%2d, P: %8lu, F:%8lu\n", index, cstate[index].power,
			cstate[index].freq);
	}

	for_each_cpu (cpu, &cluster_info[cluster].cpus) {
		if (alloc_percpu_time(cpu, nr_cap))
			return -ENOMEM;
	}

	cluster_info[cluster].last_time = ktime_get_ns() >> 10;
	return 0;
}

static void kfree_clus_mem(struct cluster_devinfo *cluster)
{
	kfree(cluster->cstate);
	kfree(cluster->wtime);
	free_page((unsigned long)cluster->cwk.buf);
}

static int cpu_pcost_init(void)
{
	struct cpumask cpus = *cpu_possible_mask;
	const struct cpumask *cluster_cpus;
	int i, cpu, cluster = 0;

	pr_info("come into %s\n", __func__);
	for_each_cpu (i, &cpus) {
		if (cluster >= MAX_CLUSTER)
			break;

		cluster_cpus = cpu_coregroup_mask(i);
		if (i != cpumask_first(cluster_cpus))
			continue;

		for_each_cpu (cpu, cluster_cpus) {
			per_cpu(cpu_st, cpu).cid = cluster;
			per_cpu(cpu_st, cpu).prev_idle_time = 0;
			per_cpu(cpu_st, cpu).prev_wtime = 0;
		}

		cpumask_copy(&cluster_info[cluster].cpus, cluster_cpus);
		cpumask_andnot(&cpus, &cpus, cluster_cpus);
		cluster_info[cluster].nr_cap_stats = 0;
		cluster_info[cluster].cap_index = 0;
		cluster_info[cluster].wtime = 0;
		cluster_info[cluster].cur_freq = 0;
		raw_spin_lock_init(&cluster_info[cluster].lock);
		if (enery_probe(cluster))
			goto alloc_err;

		init_clus_suc[cluster] = 1;
		cluster++;
	}

	cluster_number = cluster;
	cpufreq_register_notifier(&notifier_trans_block,
				  CPUFREQ_TRANSITION_NOTIFIER);

	ea_wq = alloc_workqueue("ea_wq", WQ_HIGHPRI, 0);
	if (!ea_wq)
		goto alloc_err;

	pr_info("%s suc\n", __func__);
	return 0;

alloc_err:
	for (cpu = nr_cpu_ids - 1; cpu >= 0; cpu--) {
		if (init_cpu_suc[cpu]) {
			init_cpu_suc[cpu] = 0;
			kfree(per_cpu(cpu_st, cpu).atime);
		}
	}

	for (cluster = MAX_CLUSTER - 1; cluster >= 0; cluster--) {
		init_clus_suc[cluster] = 0;
		if (cluster_info[cluster].has_init) {
			cluster_info[cluster].has_init = 0;
			kfree_clus_mem(&cluster_info[cluster]);
		}
	}

	return -ENOMEM;
}

late_initcall(cpu_pcost_init);
