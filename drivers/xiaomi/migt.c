/**
 * Copyright (c) Xiaomi Technologies Co., Ltd. 2019. All rights reserved.
 *
 * File name: migt.c
 * Descrviption: cpufreq boost function.
 * Author: guchao1@xiaomi.com
 * Version: 2.0
 * Date:  2019/01/29
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) "migt: " fmt

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/miscdevice.h>
#include <linux/security.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/uaccess.h>
#include <linux/hrtimer.h>
#include <linux/sched/core_ctl.h>
#include <linux/cred.h>

#define CREATE_TRACE_POINTS
#include "migt_trace.h"

#define MAX_CEILING_USERS 10
#define VIP_REQ_LIMIT 3

struct migt {
	int cpu;
	unsigned int migt_min;
	unsigned int boost_freq;
	unsigned int migt_ceiling_max;
	unsigned int ceiling_freq;
};

enum freq_type { boost_freq, ceiling_freq };

static struct ceiling_user_s {
	int uid[MAX_CEILING_USERS];
	int count;
} ceiling_users;

enum migt_cmd {
	QUEUE_BUFFER = 1,
	DEQUEUE_BUFFER,
	SET_CEILING,
};

static enum BOOST_POLICY {
	NO_MIGT = 0,
	MIGT_1_0,
	MIGT_2_0,
	MIGT_3_0,
} boost_policy = NO_MIGT;

static int ceiling_enable;
static int ceiling_freq_limit_work;
static bool migt_enabled;
static spinlock_t migt_lock;
static cpumask_var_t active_cluster_cpus;
static u32 sys_migt_count;
static u64 last_update_time;
static uid_t last_traced_uid;
static int render_pid = -1;
static int boost_cycle;
static int migt_boost_flag;
static struct hrtimer hrtimer;
static struct hrtimer vtask_timer;
static DEFINE_PER_CPU(struct migt, migt_info);
static struct workqueue_struct *migt_wq;
static struct delayed_work migt_work;
static struct delayed_work ceiling_work;
static struct work_struct irq_boost_work;
static struct delayed_work migt_rem;
static struct delayed_work ceiling_rem;
static unsigned long ceiling_next_update;
unsigned int minor_window_app;
module_param(minor_window_app, uint, 0644);
static unsigned int perf_mod = 1;
module_param(perf_mod, uint, 0644);
static unsigned int first_lunch_fps = 100;
module_param(first_lunch_fps, uint, 0644);
static unsigned int ceiling_slack_ms = MSEC_PER_SEC;
module_param(ceiling_slack_ms, uint, 0644);
static int ceiling_from_user;
module_param(ceiling_from_user, uint, 0644);
static unsigned int high_resolution_enable = 1;
module_param(high_resolution_enable, uint, 0644);
static unsigned int migt_debug;
module_param(migt_debug, uint, 0644);
static unsigned int migt_ms = 50;
module_param(migt_ms, uint, 0644);
static unsigned int migt_thresh = 18;
module_param(migt_thresh, uint, 0644);
static unsigned int itask_detect_interval = 5000;
module_param(itask_detect_interval, uint, 0644);
static unsigned int over_thresh_count;
module_param(over_thresh_count, uint, 0644);
module_param(boost_policy, uint, 0644);
static unsigned int cpu_boost_cycle = 10;
module_param(cpu_boost_cycle, uint, 0644);
static unsigned int render_update = 1;
module_param(render_update, uint, 0644);
int vip_task_schedboost;
int fas_power_mod;
module_param(fas_power_mod, uint, 0644);

#ifdef CONFIG_PACKAGE_RUNTIME_INFO
static u64 migt_monitor_thresh;
static u64 migt_fast_div;
static unsigned int migt_viptask_thresh = 50;
module_param(migt_viptask_thresh, uint, 0644);
static unsigned int fps_mexec_update_policy;
module_param(fps_mexec_update_policy, uint, 0644);
static unsigned int fps_variance_ratio = 10;
module_param(fps_variance_ratio, uint, 0644);
static int tsched_debug;
module_param(tsched_debug, uint, 0644);

static DECLARE_BITMAP(cluster_affinity_uid_mlist, BIT_MAP_SIZE);
static DECLARE_BITMAP(cluster_affinity_uid_llist, BIT_MAP_SIZE);
static DECLARE_BITMAP(cluster_affinity_uid_blist, BIT_MAP_SIZE);

int enable_pkg_monitor = 1;
module_param(enable_pkg_monitor, uint, 0644);
int set_render_as_stask;
module_param(set_render_as_stask, uint, 0644);
int force_stask_to_big;
module_param(force_stask_to_big, uint, 0644);
int stask_candidate_num = 1;
module_param(stask_candidate_num, uint, 0644);
int vip_task_max_num = 15;
module_param(vip_task_max_num, uint, 0644);
int ip_task_max_num = 5;
module_param(ip_task_max_num, uint, 0644);
int need_reset_runtime_everytime;
module_param(need_reset_runtime_everytime, uint, 0644);
int force_reset_runtime;
module_param(force_reset_runtime, uint, 0644);
int glk_disable = 1;
module_param(glk_disable, uint, 0644);
int glk_fbreak_enable;
module_param(glk_fbreak_enable, uint, 0644);
int glk_freq_limit_start;
module_param(glk_freq_limit_start, uint, 0644);
int glk_freq_limit_walt;
module_param(glk_freq_limit_walt, uint, 0644);
unsigned int glk_maxfreq[MAX_CLUSTERS];
module_param_array(glk_maxfreq, uint, NULL, 0644);
unsigned int glk_minfreq[MAX_CLUSTERS];
module_param_array(glk_minfreq, uint, NULL, 0644);
#endif
unsigned int mi_viptask[VIP_REQ_LIMIT];

void __weak add_work_to_migt(int uid)
{
	BUG();
	return;
}

void __weak mi_vip_task_req(int *pid, unsigned int nr, unsigned int times)
{
}
int __weak get_mi_dynamic_vip_num(void);

static int set_mi_vip_task_req(const char *buf, const struct kernel_param *kp)
{
	int i, len, ntokens = 0;
	unsigned int val;
	int num = 0;
	unsigned int times = 0;
	const char *cp = buf;

	while ((cp = strpbrk(cp + 1, ":")))
		ntokens++;

	len = strlen(buf);

	if (!ntokens) {
		if (sscanf(buf, "%u-%u\n", &val, &times) != 2)
			return -EINVAL;

		pr_info("val %d times %d\n", val, times);
		mi_vip_task_req(&val, 1, times);
		return 0;
	}

	cp = buf;
	for (i = 0; i < ntokens; i++) {
		if (kstrtouint(cp, 0, &val))
			return -EINVAL;

		mi_viptask[num++] = val;
		pr_info("arg %d val %d\n", num, val);
		cp = strpbrk(cp + 1, ":");
		cp++;
		if ((cp >= buf + len))
			return 0;

		if (num >= VIP_REQ_LIMIT) {
			cp = strpbrk(cp + 1, "-");
			cp++;
			if ((cp >= buf + len))
				return 0;

			if (kstrtouint(cp, 0, &times))
				return -EINVAL;

			pr_info("arg %d times %d\n", num, times);
			mi_vip_task_req(mi_viptask, num, times);
			return 0;
		}
	}

	if (cp < buf + len) {
		if (sscanf(cp, "%u-%u", &val, &times) != 2)
			return -EINVAL;

		mi_viptask[num++] = val;
		pr_info("arg %d val = %d times = %d\n", num, val, times);
	}

	mi_vip_task_req(mi_viptask, num, times);

	return 0;
}

static int get_mi_viptask(char *buf, const struct kernel_param *kp)
{
	int i, cnt = 0;

	for (i = 0; i < VIP_REQ_LIMIT; i++)
		cnt += snprintf(buf + cnt, PAGE_SIZE - cnt, "%u:%d ",
				mi_viptask[i], get_mi_dynamic_vip_num());

	cnt += snprintf(buf + cnt, PAGE_SIZE - cnt, "\n");
	return cnt;
}

static const struct kernel_param_ops param_ops_mi_viptask = {
	.set = set_mi_vip_task_req,
	.get = get_mi_viptask,
};

module_param_cb(mi_viptask, &param_ops_mi_viptask, NULL, 0644);

static void add_uid_to_list(int uid, enum CLUSTER_AFFINITY type)
{
	if (unlikely(uid >= BIT_MAP_SIZE)) {
		pr_info("Big uid %d warning\n", uid);
		return;
	}
	switch (type) {
	case CAFFINITY_MID_LIST:
		clear_bit(uid, cluster_affinity_uid_llist);
		clear_bit(uid, cluster_affinity_uid_blist);
		set_bit(uid, cluster_affinity_uid_mlist);
		break;
	case CAFFINITY_LITTLE_LIST:
		clear_bit(uid, cluster_affinity_uid_mlist);
		clear_bit(uid, cluster_affinity_uid_blist);
		set_bit(uid, cluster_affinity_uid_llist);
		break;
	case CAFFINITY_BIG_LIST:
		clear_bit(uid, cluster_affinity_uid_llist);
		clear_bit(uid, cluster_affinity_uid_mlist);
		set_bit(uid, cluster_affinity_uid_blist);
		break;
	default:
		clear_bit(uid, cluster_affinity_uid_mlist);
		clear_bit(uid, cluster_affinity_uid_blist);
		clear_bit(uid, cluster_affinity_uid_llist);
		break;
	}
	if (tsched_debug)
		pr_info("Add uid %d to %d list\n", uid, type);
}
static void del_uid_from_list(int uid, enum CLUSTER_AFFINITY type)
{
	if (unlikely(uid >= BIT_MAP_SIZE)) {
		pr_info("Big uid %d warning\n", uid);
		return;
	}
	switch (type) {
	case CAFFINITY_MID_LIST:
		clear_bit(uid, cluster_affinity_uid_mlist);
		break;
	case CAFFINITY_LITTLE_LIST:
		clear_bit(uid, cluster_affinity_uid_llist);
		break;
	case CAFFINITY_BIG_LIST:
		clear_bit(uid, cluster_affinity_uid_blist);
		break;
	default:
		clear_bit(uid, cluster_affinity_uid_blist);
		clear_bit(uid, cluster_affinity_uid_mlist);
		clear_bit(uid, cluster_affinity_uid_llist);
		break;
	}
	if (tsched_debug)
		pr_info("Del uid %d from %d list\n", uid, type);
}
static inline void reset_affinity_list_zero(enum CLUSTER_AFFINITY type)
{
	unsigned int len = BITS_TO_LONGS(BIT_MAP_SIZE) * sizeof(unsigned long);

	switch (type) {
	case CAFFINITY_MID_LIST:
		memset(cluster_affinity_uid_mlist, 0, len);
		break;
	case CAFFINITY_LITTLE_LIST:
		memset(cluster_affinity_uid_llist, 0, len);
		break;
	case CAFFINITY_BIG_LIST:
		memset(cluster_affinity_uid_blist, 0, len);
		break;
	default:
		memset(cluster_affinity_uid_blist, 0, len);
		memset(cluster_affinity_uid_mlist, 0, len);
		memset(cluster_affinity_uid_llist, 0, len);
		break;
	}
}
static int set_clus_affinity_uidlist(const char *buf,
				     const struct kernel_param *kp)
{
	int i, len, ntokens = 0;
	unsigned int val;
	const char *cp = buf;
	bool need_clean = false;
	bool delete = false;
	enum CLUSTER_AFFINITY type;

	len = strlen(kp->name);
	if (strnstr(kp->name, "add_lclus_affinity_uidlist", len))
		type = CAFFINITY_LITTLE_LIST;
	else if (strnstr(kp->name, "add_mclus_affinity_uidlist", len))
		type = CAFFINITY_MID_LIST;
	else if (strnstr(kp->name, "add_bclus_affinity_uidlist", len))
		type = CAFFINITY_BIG_LIST;
	else if (strnstr(kp->name, "reset_clus_affinity_uidlist", len))
		need_clean = true;
	else if (strnstr(kp->name, "del_lclus_affinity_uidlist", len)) {
		delete = true;
		type = CAFFINITY_LITTLE_LIST;
	} else if (strnstr(kp->name, "del_mclus_affinity_uidlist", len)) {
		delete = true;
		type = CAFFINITY_MID_LIST;
	} else if (strnstr(kp->name, "del_bclus_affinity_uidlist", len)) {
		delete = true;
		type = CAFFINITY_BIG_LIST;
	} else {
		return 0;
	}

	while ((cp = strpbrk(cp + 1, ":")))
		ntokens++;

	len = strlen(buf);
	if (!ntokens) {
		if (sscanf(buf, "%u\n", &val) != 1)
			return -EINVAL;

		if (need_clean) {
			reset_affinity_list_zero(val);
			return 0;
		}

		if (delete)
			del_uid_from_list(val, type);
		else
			add_uid_to_list(val, type);

		return 0;
	}
	cp = buf;
	if (delete) {
		for (i = 0; i < ntokens; i++) {
			if (kstrtouint(cp, 0, &val))
				return -EINVAL;
			del_uid_from_list(val, type);
			pr_info("arg %d val %d\n", i, val);
			cp = strpbrk(cp + 1, ":");
			cp++;
			if (cp >= buf + len)
				break;
		}
		if (cp < buf + len) {
			if (kstrtouint(cp, 0, &val))
				return -EINVAL;

			del_uid_from_list(val, type);
			pr_info("arg %d val = %d\n", i, val);
		}
		return 0;
	}
	for (i = 0; i < ntokens; i++) {
		if (kstrtouint(cp, 0, &val))
			return -EINVAL;

		add_uid_to_list(val, type);
		pr_info("arg %d val %d\n", i, val);
		cp = strpbrk(cp + 1, ":");
		cp++;
		if (cp >= buf + len)
			break;
	}
	if (cp < buf + len) {
		if (kstrtouint(cp, 0, &val))
			return -EINVAL;

		add_uid_to_list(val, type);
		pr_info("arg %d val = %d\n", i, val);
	}
	return 0;
}
static int get_clus_affinity_uidlist(char *buf, const struct kernel_param *kp)
{
	int cnt = 0, bit, len;
	enum CLUSTER_AFFINITY type;

	len = strlen(kp->name);
	if (strnstr(kp->name, "show_lclus_affinity_uidlist", len))
		type = CAFFINITY_LITTLE_LIST;
	else if (strnstr(kp->name, "show_mclus_affinity_uidlist", len))
		type = CAFFINITY_MID_LIST;
	else if (strnstr(kp->name, "show_bclus_affinity_uidlist", len))
		type = CAFFINITY_BIG_LIST;
	else {
		cnt += snprintf(buf + cnt, PAGE_SIZE - cnt,
				"need show_[b|l|m]clus_affinity_uidlist\n");
		return cnt;
	}

	if (type == CAFFINITY_BIG_LIST)
		for_each_set_bit (bit, cluster_affinity_uid_blist, BIT_MAP_SIZE)
			cnt += snprintf(buf + cnt, PAGE_SIZE - cnt, "%u ", bit);

	if (type == CAFFINITY_MID_LIST)
		for_each_set_bit (bit, cluster_affinity_uid_mlist, BIT_MAP_SIZE)
			cnt += snprintf(buf + cnt, PAGE_SIZE - cnt, "%u ", bit);

	if (type == CAFFINITY_LITTLE_LIST)
		for_each_set_bit (bit, cluster_affinity_uid_llist, BIT_MAP_SIZE)
			cnt += snprintf(buf + cnt, PAGE_SIZE - cnt, "%u ", bit);

	cnt += snprintf(buf + cnt, PAGE_SIZE - cnt, "\n");
	return cnt;
}
static const struct kernel_param_ops param_ops_clus_affinity_uidlist = {
	.set = set_clus_affinity_uidlist,
	.get = get_clus_affinity_uidlist,
};

module_param_cb(add_lclus_affinity_uidlist, &param_ops_clus_affinity_uidlist,
		NULL, 0644);
module_param_cb(add_mclus_affinity_uidlist, &param_ops_clus_affinity_uidlist,
		NULL, 0644);
module_param_cb(add_bclus_affinity_uidlist, &param_ops_clus_affinity_uidlist,
		NULL, 0644);
module_param_cb(reset_clus_affinity_uidlist, &param_ops_clus_affinity_uidlist,
		NULL, 0644);
module_param_cb(del_lclus_affinity_uidlist, &param_ops_clus_affinity_uidlist,
		NULL, 0644);
module_param_cb(del_mclus_affinity_uidlist, &param_ops_clus_affinity_uidlist,
		NULL, 0644);
module_param_cb(del_bclus_affinity_uidlist, &param_ops_clus_affinity_uidlist,
		NULL, 0644);
module_param_cb(show_lclus_affinity_uidlist, &param_ops_clus_affinity_uidlist,
		NULL, 0644);
module_param_cb(show_mclus_affinity_uidlist, &param_ops_clus_affinity_uidlist,
		NULL, 0644);
module_param_cb(show_bclus_affinity_uidlist, &param_ops_clus_affinity_uidlist,
		NULL, 0644);

enum CLUSTER_AFFINITY mi_uid_type(int uid)
{
	if (unlikely(uid >= BIT_MAP_SIZE))
		return 0;

	if (test_bit(uid, cluster_affinity_uid_mlist))
		return CAFFINITY_MID_LIST;
	else if (test_bit(uid, cluster_affinity_uid_llist))
		return CAFFINITY_LITTLE_LIST;
	else if (test_bit(uid, cluster_affinity_uid_blist))
		return CAFFINITY_BIG_LIST;
	else
		return 0;
}
EXPORT_SYMBOL(mi_uid_type);

static int set_migt_freq(const char *buf, const struct kernel_param *kp)
{
	int i, len, ntokens = 0;
	unsigned int val, cpu;
	const char *cp = buf;
	bool enabled = false;
	enum freq_type type;

	len = strlen(kp->name);
	if (strnstr(kp->name, "migt_freq", len))
		type = boost_freq;
	if (strnstr(kp->name, "migt_ceiling_freq", len))
		type = ceiling_freq;

	while ((cp = strpbrk(cp + 1, " :")))
		ntokens++;

	if (!ntokens) {
		if (sscanf(buf, "%u\n", &val) != 1)
			return -EINVAL;

		for_each_possible_cpu (i) {
			if (type == boost_freq)
				per_cpu(migt_info, i).boost_freq = val;
			else
				per_cpu(migt_info, i).ceiling_freq = val;
		}
		goto check_enable;
	}

	if (!(ntokens % 2))
		return -EINVAL;

	cp = buf;
	for (i = 0; i < ntokens; i += 2) {
		if (sscanf(cp, "%u:%u", &cpu, &val) != 2)
			return -EINVAL;

		if (cpu >= num_possible_cpus())
			return -EINVAL;

		if (type == boost_freq)
			per_cpu(migt_info, cpu).boost_freq = val;
		else
			per_cpu(migt_info, cpu).ceiling_freq = val;

		cp = strchr(cp, ' ');
		cp++;
	}

check_enable:
	for_each_possible_cpu (i) {
		if (per_cpu(migt_info, i).boost_freq) {
			enabled = true;
			break;
		}
	}
	migt_enabled = enabled;
	return 0;
}

static int get_migt_freq(char *buf, const struct kernel_param *kp)
{
	int cnt = 0, cpu, len;
	struct migt *s;
	unsigned int freq = 0;
	enum freq_type type;

	len = strlen(kp->name);
	if (strnstr(kp->name, "migt_freq", len))
		type = boost_freq;
	if (strnstr(kp->name, "migt_ceiling_freq", len))
		type = ceiling_freq;

	for_each_possible_cpu (cpu) {
		s = &per_cpu(migt_info, cpu);
		if (type == boost_freq)
			freq = s->boost_freq;
		else
			freq = s->ceiling_freq;

		cnt += snprintf(buf + cnt, PAGE_SIZE - cnt, "%d:%u ", cpu,
				freq);
	}
	cnt += snprintf(buf + cnt, PAGE_SIZE - cnt, "\n");
	return cnt;
}

static const struct kernel_param_ops param_ops_migt_freq = {
	.set = set_migt_freq,
	.get = get_migt_freq,
};

module_param_cb(migt_freq, &param_ops_migt_freq, NULL, 0644);
module_param_cb(migt_ceiling_freq, &param_ops_migt_freq, NULL, 0644);

/**
 * Used to override the current policy min to boost cpu freq during app launch/
 * package install or other performance scenaries,
 * The cpufreq framework then does the job
 * of enforcing the new policy.
 */
static int boost_adjust_notify(struct notifier_block *nb, unsigned long val,
			       void *data)
{
	struct cpufreq_policy *policy = data;
	unsigned int cpu = policy->cpu;
	struct migt *s = &per_cpu(migt_info, cpu);
	unsigned int min_freq = s->migt_min;

	switch (val) {
	case CPUFREQ_ADJUST:
		if (!min_freq)
			break;

		cpufreq_verify_within_limits(policy, min_freq, UINT_MAX);
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block boost_adjust_nb = {
	.notifier_call = boost_adjust_notify,
};

static int ceiling_adjust_notify(struct notifier_block *nb, unsigned long val,
				 void *data)
{
	struct cpufreq_policy *policy = data;
	unsigned int cpu = policy->cpu;
	struct migt *s = &per_cpu(migt_info, cpu);
	unsigned int max_freq = s->migt_ceiling_max;

	switch (val) {
	case CPUFREQ_ADJUST:
		if (!max_freq && (max_freq != UINT_MAX))
			break;

		cpufreq_verify_within_limits(policy, 0, max_freq);
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block ceiling_adjust_nb = {
	.notifier_call = ceiling_adjust_notify,
};

static int freq_update_should_skip(enum cluster_type cluster)
{
	switch (boost_policy) {
	case NO_MIGT:
		break;
	// Boost mid cluster in migt 1.0
	case MIGT_1_0:
		if (cluster == MID_CLUSTER)
			return 0;
		break;
	// Boost mid & big cluster in migt 2.0 && 3.0
	case MIGT_2_0:
	case MIGT_3_0:
		if (cluster >= MID_CLUSTER)
			return 0;
		break;
	default:
		break;
	}
	return 1;
}

static void update_policy_online(void)
{
	unsigned int i;
	const struct cpumask *cluster_cpus;
	struct cpufreq_policy *policy;
	enum cluster_type cluster_index = LITTLE_CLUSTER;

	get_online_cpus();
	for_each_online_cpu (i) {
		cluster_cpus = cpu_coregroup_mask(i);
		cpumask_and(active_cluster_cpus, cluster_cpus, cpu_online_mask);
		if (i != cpumask_first(active_cluster_cpus))
			continue;

		if (freq_update_should_skip(cluster_index)) {
			cluster_index++;
			continue;
		}

		if (migt_debug == 1)
			pr_debug("Updating policy for CPU %d\n", i);

		policy = cpufreq_cpu_get(i);
		if (policy) {
			cpufreq_update_policy(i);
			cpufreq_cpu_put(policy);
		}
		cluster_index++;
	}
	put_online_cpus();
}

static void do_migt_rem(struct work_struct *work)
{
	unsigned int i;
	struct migt *i_migt_info;

	trace_do_migt_rem((unsigned long)work);
	if (--boost_cycle > 0) {
		queue_delayed_work(migt_wq, &migt_rem,
				   msecs_to_jiffies(migt_ms));
		glk_force_maxfreq_break(true);
		return;
	}
	glk_force_maxfreq_break(false);

	if (migt_debug == 1)
		pr_err("migt boost end...%d %llu", __LINE__,
		       ktime_to_us(ktime_get()));

	core_ctl_set_boost(false);
	glk_maxfreq_break(false);

	for_each_possible_cpu (i) {
		i_migt_info = &per_cpu(migt_info, i);
		i_migt_info->migt_min = 0;
		trace_i_migt_info(i, i_migt_info->migt_min,
				  i_migt_info->boost_freq,
				  i_migt_info->migt_ceiling_max,
				  i_migt_info->ceiling_freq);
	}

	// Update policies for all online CPUs
	update_policy_online();
	vip_task_schedboost = 0;
	trace_do_migt_rem_done((unsigned long)work);
}

static void feed_ceiling_rem_work(void)
{
	if (!ceiling_freq_limit_work)
		return;

	if (time_after(jiffies, ceiling_next_update)) {
		ceiling_next_update =
			jiffies + msecs_to_jiffies(ceiling_slack_ms >> 1);
		cancel_delayed_work_sync(&ceiling_rem);
		queue_delayed_work(migt_wq, &ceiling_rem,
				   msecs_to_jiffies(ceiling_slack_ms));
	}
}

static void do_migt(struct work_struct *work)
{
	unsigned int i;
	struct migt *i_migt_info;
	bool work_pending = false;

	trace_do_migt((unsigned long)work);
	work_pending = cancel_delayed_work_sync(&migt_rem);
	if (migt_debug == 1)
		pr_err("migt boost start....%d %llu", __LINE__,
		       ktime_to_us(ktime_get()));

	/**
	 * 1: boost game super task
	 * 2: force all avaliable cores on line
	 * 3: boost mid cluster cpufreq
	 */
	vip_task_schedboost = 1;
	if (!work_pending)
		core_ctl_set_boost(true);
	glk_maxfreq_break(true);

	for_each_possible_cpu (i) {
		i_migt_info = &per_cpu(migt_info, i);
		i_migt_info->migt_min = i_migt_info->boost_freq;
		trace_i_migt_info(i, i_migt_info->migt_min,
				  i_migt_info->boost_freq,
				  i_migt_info->migt_ceiling_max,
				  i_migt_info->ceiling_freq);
	}

	update_policy_online();
	queue_delayed_work(migt_wq, &migt_rem, msecs_to_jiffies(migt_ms));

	boost_cycle = cpu_boost_cycle;
	trace_do_migt_done((unsigned long)work);
}

static enum hrtimer_restart do_boost_work(struct hrtimer *timer)
{
	if (work_pending(&irq_boost_work))
		return HRTIMER_NORESTART;

	queue_work(migt_wq, &irq_boost_work);

	return HRTIMER_NORESTART;
}

static void trigger_migt_in_hrtimer(int timeout_ms)
{
	// If the timer is already running, stop it
	if (hrtimer_active(&hrtimer))
		hrtimer_cancel(&hrtimer);

	hrtimer_start(&hrtimer, ms_to_ktime(timeout_ms), HRTIMER_MODE_REL);
}

static void trigger_migt(void)
{
	int deadline = migt_thresh;
	u64 now;

	if (!migt_enabled || !boost_policy)
		return;

	if (++sys_migt_count > first_lunch_fps && boost_policy >= MIGT_2_0 &&
	    ((sys_migt_count % itask_detect_interval == 0) ||
	     (current->pid != render_pid))) {
		glk_freq_limit_start = 1;
		if (current->pid != render_pid) {
			if (migt_debug)
				pr_info("render pid change from %d to %d\n",
					render_pid, current->pid);
			trace_render_change(render_pid, current->pid);
			if (render_update)
				add_work_to_migt(current_uid().val);
		} else
			add_work_to_migt(current_uid().val);
	}

	if (sys_migt_count <= first_lunch_fps)
		glk_freq_limit_start = 0;

	render_pid = current->pid;
	now = ktime_get_ns();
	if (now - last_update_time >= migt_thresh * NSEC_PER_MSEC)
		over_thresh_count++;

	migt_boost_flag = 0;
	boost_cycle = 0;
	last_update_time = now;
	feed_ceiling_rem_work();

	switch (boost_policy) {
	case NO_MIGT:
		return;

	case MIGT_1_0:
		break;

	case MIGT_2_0:
		return;

	case MIGT_3_0:
		game_load_history_update(sys_migt_count);
		if (unlikely(ceiling_enable))
			queue_delayed_work(migt_wq, &ceiling_work,
					   msecs_to_jiffies(1));
		deadline = (migt_thresh + (migt_thresh >> 1));
		break;
	default:
		pr_err("invalid version number %d\n", boost_policy);
		return;
	}

	// MIGT_1_0 or MIGT_3_0
	if (high_resolution_enable)
		return trigger_migt_in_hrtimer(deadline);

	cancel_delayed_work_sync(&migt_work);
	queue_delayed_work(migt_wq, &migt_work, msecs_to_jiffies(deadline));
}

static void do_ceiling_rem(struct work_struct *work)
{
	unsigned int i;
	struct migt *i_migt_info;

	trace_do_ceiling_rem((unsigned long)work);
	if (!ceiling_freq_limit_work)
		return;

	if (migt_debug == 1)
		pr_err("migt do ceiling_rem....%d %llu", __LINE__,
		       ktime_to_us(ktime_get()));

	for_each_possible_cpu (i) {
		i_migt_info = &per_cpu(migt_info, i);
		i_migt_info->migt_ceiling_max = UINT_MAX;
		trace_i_migt_info(i, i_migt_info->migt_min,
				  i_migt_info->boost_freq,
				  i_migt_info->migt_ceiling_max,
				  i_migt_info->ceiling_freq);
	}

	// Update policies for all online CPUs
	update_policy_online();
	ceiling_freq_limit_work = 0;
	trace_do_ceiling_rem_done((unsigned long)work);
}

static int check_uid_valid(int uid)
{
	int i, users = (ceiling_users.count > MAX_CEILING_USERS) ?
				     MAX_CEILING_USERS :
				     ceiling_users.count;

	for (i = 0; i < users; i++)
		if (uid == ceiling_users.uid[i])
			return 1;
	return 0;
}

static void add_uid_to_ceiling_user(int uid)
{
	int i, users = (ceiling_users.count > MAX_CEILING_USERS) ?
				     MAX_CEILING_USERS :
				     ceiling_users.count;

	if (migt_debug == 2) {
		pr_info("--------ceiling_user list--------\n");
		for (i = 0; i < users; i++)
			pr_info("%d ", ceiling_users.uid[i]);
	}

	for (i = 0; i < users; i++)
		if (uid == ceiling_users.uid[i])
			return;

	if (migt_debug == 2)
		pr_info("\nadd uid %d to ceiling users\n", uid);

	// need to add new ceiling user
	if (users < MAX_CEILING_USERS) {
		ceiling_users.uid[i] = uid;
		ceiling_users.count++;
	} else {
		for (i = 0; i < users - 1; i--)
			ceiling_users.uid[i] = ceiling_users.uid[i + 1];
		ceiling_users.uid[users - 1] = uid;
	}
}

void __weak update_freq_limit(u64 frame_period)
{
	return;
}

static void do_ceiling_limit(struct work_struct *work)
{
	unsigned int i;
	struct migt *i_migt_info;

	trace_do_ceiling_limit((unsigned long)work);
	if (!ceiling_freq_limit_work)
		if (check_uid_valid(last_traced_uid))
			ceiling_freq_limit_work = 1;

	if (!ceiling_freq_limit_work)
		return;

	if (migt_debug == 2)
		pr_err("migt ceiling_limit ...%d %llu, ceiling_freq: %d",
		       __LINE__, ktime_to_us(ktime_get()),
		       i_migt_info->ceiling_freq);

	if (likely(!ceiling_from_user)) {
		update_freq_limit(migt_thresh * NSEC_PER_MSEC);
		return;
	}

	for_each_possible_cpu (i) {
		i_migt_info = &per_cpu(migt_info, i);
		i_migt_info->migt_ceiling_max = i_migt_info->ceiling_freq;
		trace_i_migt_info(i, i_migt_info->migt_min,
				  i_migt_info->boost_freq,
				  i_migt_info->migt_ceiling_max,
				  i_migt_info->ceiling_freq);
	}

	// Update policies for all online CPUs
	update_policy_online();
	trace_do_ceiling_limit_done((unsigned long)work);
}

static int migt_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int migt_release(struct inode *ignored, struct file *file)
{
	return 0;
}

static int migt_mmap(struct file *file, struct vm_area_struct *vma)
{
	return 0;
}

int get_cur_render_uid(void)
{
	return last_traced_uid;
}

int package_runtime_should_stop(void)
{
	return boost_policy == NO_MIGT;
}

static long migt_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	int traced_uid, type;
	int user_cmd = _IOC_NR(cmd);

	traced_uid = from_kuid(&init_user_ns, current_uid());
	//	if (traced_uid == minor_window_app && minor_window_app)
	//		return 0;

	trace_migt_ioctl(user_cmd, traced_uid);

	if (migt_debug == 1)
		pr_err("migt boost comming. %d %d %llu", __LINE__, user_cmd,
		       ktime_to_us(ktime_get()));

	switch (user_cmd) {
	case QUEUE_BUFFER:
		if (traced_uid != last_traced_uid) {
			if (ceiling_freq_limit_work) {
				cancel_delayed_work_sync(&ceiling_rem);
				ceiling_next_update = jiffies;
				queue_delayed_work(migt_wq, &ceiling_rem,
						   msecs_to_jiffies(1));
			}

			for (type = 0; type < RENDER_TYPES; type++)
				reset_render_info(type);
			sys_migt_count = 0;

			if (boost_policy == MIGT_3_0)
				game_load_reset();
			last_traced_uid = traced_uid;
		}

		update_render_info(current, RENDER_QUEUE_THREAD);
		trigger_migt();
		break;

	case DEQUEUE_BUFFER:
		update_render_info(current, RENDER_DEQUEUE_THREAD);
		break;

	case SET_CEILING:
		ceiling_enable = 1;
		add_uid_to_ceiling_user(last_traced_uid);
		break;

	default:
		break;
	}

	trace_migt_ioctl_done(user_cmd, traced_uid);
	return 0;
}

static const struct file_operations migt_fops = {
	.owner = THIS_MODULE,
	.open = migt_open,
	.release = migt_release,
	.mmap = migt_mmap,
	.unlocked_ioctl = migt_ioctl,
};

static struct miscdevice migt_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "migt",
	.fops = &migt_fops,
};

#ifdef CONFIG_PACKAGE_RUNTIME_INFO

static void vtask_boost_in_hrtimer(int timeout_ms)
{
	// If the timer is already running, stop it
	if (hrtimer_active(&vtask_timer))
		hrtimer_cancel(&vtask_timer);

	hrtimer_start(&vtask_timer, ms_to_ktime(timeout_ms), HRTIMER_MODE_REL);
}

static void vtask_boost(struct task_struct *tsk, int index)
{
	int flag = 0;

	if (spin_trylock(&migt_lock)) {
		if (!migt_boost_flag)
			flag = 1;
		migt_boost_flag = 1;
		spin_unlock(&migt_lock);
	}

	if (flag && !work_pending(&irq_boost_work)) {
		vtask_boost_in_hrtimer(0);
#ifdef VTASK_BOOST_DEBUG
		tsk->pkg.migt.boostat[index]++;
#endif
	}
}

static void update_migt_monitor_thresh(int thresh)
{
	migt_fast_div = NUM_MIGT_BUCKETS * MSEC_PER_SEC / thresh;
	migt_monitor_thresh =
		(thresh * NSEC_PER_MSEC * migt_viptask_thresh) / 100;
}

static int get_bucket_index(u64 fps_exec)
{
	u32 index;

	if (!migt_fast_div)
		migt_fast_div = NUM_MIGT_BUCKETS * MSEC_PER_SEC / migt_thresh;

	index = fps_exec * migt_fast_div / NSEC_PER_SEC;
	index %= NUM_MIGT_BUCKETS;
	return index;
}

static void update_task_fps_mexec(struct task_struct *tsk)
{
	switch (fps_mexec_update_policy) {
	case 0:
		tsk->pkg.migt.fps_mexec++;
		break;

	case 1:
		tsk->pkg.migt.fps_mexec += tsk->pkg.migt.fps_exec;
		tsk->pkg.migt.fps_mexec >>= 1;
		break;

	case 2:
	default:
		if (tsk->pkg.migt.fps_exec > tsk->pkg.migt.fps_mexec)
			tsk->pkg.migt.fps_mexec = tsk->pkg.migt.fps_exec;
		break;
	}
}

void migt_hook(struct task_struct *tsk, u64 delta, int cpu)
{
	u32 index, i;
	static int old_migt_thresh;
	u64 time_diff, prev_window, now = ktime_get_ns();
	uid_t tsk_uid = from_kuid(&init_user_ns, task_uid(tsk));

	if (boost_policy < MIGT_2_0)
		return;
	else if (boost_policy == MIGT_3_0)
		game_load_update(tsk, get_scale_exec_time(delta, cpu), cpu);

	if (tsk_uid < 10000 || (tsk_uid != last_traced_uid))
		return;

	if (unlikely(now < last_update_time))
		time_diff = 0;
	else
		time_diff = now - last_update_time;

	if (sys_migt_count != tsk->pkg.migt.migt_count) {
		if (!old_migt_thresh || (old_migt_thresh != migt_thresh)) {
			old_migt_thresh = migt_thresh;
			update_migt_monitor_thresh(migt_thresh);
		}

		if (time_diff < delta) {
			prev_window = delta - time_diff;
			tsk->pkg.migt.fps_exec += prev_window;
			delta -= prev_window;
		} else {
			tsk->pkg.migt.fps_exec += delta;
			delta = 0;
		}

		if (tsk->pkg.migt.fps_exec > migt_monitor_thresh)
			update_task_fps_mexec(tsk);
		index = get_bucket_index(tsk->pkg.migt.fps_exec);

		for (i = 0; i <= index; i++)
			tsk->pkg.migt.bucket[i]++;
		tsk->pkg.migt.fps_exec = 0;
		tsk->pkg.migt.migt_count = sys_migt_count;
	}

	tsk->pkg.migt.fps_exec += delta;
	index = get_bucket_index(tsk->pkg.migt.fps_exec);

	if (((tsk->pkg.migt.bucket[index] * fps_variance_ratio <
	      tsk->pkg.migt.bucket[0])) &&
	    !migt_boost_flag) {
		if (game_ip_task(tsk) &&
		    (tsk->pkg.migt.fps_exec > migt_monitor_thresh))
			//for ip task boost
			vtask_boost(tsk, index);
	}
}

#endif

int migt_enable(void)
{
	return boost_policy;
}

u64 get_migt_thresh(void)
{
	return migt_thresh;
}

static int migt_init(void)
{
	int cpu, ret;
	struct migt *s;

	if (!alloc_cpumask_var(&active_cluster_cpus, GFP_KERNEL | __GFP_ZERO))
		return -ENOMEM;

	hrtimer_init(&hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrtimer.function = do_boost_work;

	hrtimer_init(&vtask_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vtask_timer.function = do_boost_work;

	migt_wq = alloc_workqueue("migt_wq", WQ_HIGHPRI, 0);
	if (!migt_wq) {
		free_cpumask_var(active_cluster_cpus);
		return -EFAULT;
	}

	ret = misc_register(&migt_misc);
	if (unlikely(ret)) {
		free_cpumask_var(active_cluster_cpus);
		flush_workqueue(migt_wq);
		destroy_workqueue(migt_wq);
		return ret;
	}

	INIT_DELAYED_WORK(&migt_work, do_migt);
	INIT_WORK(&irq_boost_work, do_migt);
	INIT_DELAYED_WORK(&migt_rem, do_migt_rem);
	INIT_DELAYED_WORK(&ceiling_work, do_ceiling_limit);
	INIT_DELAYED_WORK(&ceiling_rem, do_ceiling_rem);
	spin_lock_init(&migt_lock);
	for_each_possible_cpu (cpu) {
		s = &per_cpu(migt_info, cpu);
		s->cpu = cpu;
		s->migt_ceiling_max = UINT_MAX;
	}

	pr_info("migt init success\n");
	cpufreq_register_notifier(&boost_adjust_nb, CPUFREQ_POLICY_NOTIFIER);
	cpufreq_register_notifier(&ceiling_adjust_nb, CPUFREQ_POLICY_NOTIFIER);
	return 0;
}
late_initcall(migt_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("migt-driver by David");
