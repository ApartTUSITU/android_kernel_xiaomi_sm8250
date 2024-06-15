/**
 * Copyright (c) Xiaomi Technologies Co., Ltd. 2019. All rights reserved.
 *
 * File name: glk.c
 * Descrviption: Game load tracking
 * Author: guchao1@xiaomi.com
 * Version: 1.0
 * Date:  2019/12/03
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define pr_fmt(fmt) "glk: " fmt

#include <linux/kernel.h>
#include <linux/init.h>
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
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/pkg_stat.h>
#include <linux/syscore_ops.h>
#include <linux/irq_work.h>
#include "sched.h"
#include "walt.h"

#define TARGET_PERIOD(x) (MSEC_PER_SEC / (x))
#define FRAME_LOAD_HISITMES 21
enum FRAME_LOAD_TYPE {
	FREQ_CPU_LOAD,
	FREQ_GAME_LOAD,
	FREQ_STASK_LOAD,
	FREQ_VIP_LOAD,
	FREQ_RENDER_LOAD,
	FREQ_LOAD_ITEMS,
};

struct game_runtime {
	u64 sum_exec_runtime;
	u64 prev_sum_exec_runtime;
};

struct cpu_time {
	u64 prev_idle_time;
	u64 prev_wall_time;
};

struct game_load {
	struct game_runtime load[FREQ_LOAD_ITEMS];
	struct cpu_time time;
	raw_spinlock_t loading_slock;
	int cluster; /* cluster_index*/
};

struct capacity_state {
	unsigned long frequency;
};

struct game_load_items {
	u64 cpu_util[CLUSTER_TYPES];
	u64 frame_start;
	u64 frame_period;
	struct capacity_state cap_states[CLUSTER_TYPES];
};

static bool glk_maxfreq_limit = true;
static bool force_maxfreq_break;
static bool glk_minfreq_limit = true;
static u64 last_update_time;
static int cur_history_items;
static int cluster_number;
static int __read_mostly game_load_has_init;
static int __read_mostly sysctl_game_load_choose_policy;
static int __read_mostly perf_mod;
static int frame_tperiod;
static int old_migt_thresh;
static int game_load_debug;
static unsigned int glk_highspeed_load[MAX_CLUSTERS] = { [0 ... MAX_CLUSTERS -
							  1] = 90 };
static unsigned int glk_lowspeed_load[MAX_CLUSTERS] = { [0 ... MAX_CLUSTERS -
							 1] = 30 };

static unsigned long cpu_util_freq_divisor;
static struct cpumask cluster_cpumask[CLUSTER_TYPES];
static struct proc_dir_entry *game_load_dir;
static struct proc_dir_entry *history_load_entry;
static struct proc_dir_entry *cpu_load_entry;
static struct game_load_items game_load_history[FRAME_LOAD_HISITMES];
static struct ctl_table_header *game_load_header;
static DEFINE_PER_CPU(struct game_load, load_info);
static struct irq_work glk_cpufreq_irq_work;
typedef int (*glk_dointvec_f)(struct ctl_table *table, int write,
			      void __user *buffer, size_t *lenp, loff_t *ppos);

/*Redefined in migt.c*/
__weak int glk_disable = 1;
__weak int glk_freq_limit_walt;
__weak int glk_fbreak_enable;
__weak int glk_freq_limit_start;
__weak unsigned int glk_maxfreq[MAX_CLUSTERS];
__weak unsigned int glk_minfreq[MAX_CLUSTERS];

#define for_each_cluster_cpumask(i, cpumask)                                   \
	for (i = 0, cpumask = &cluster_cpumask[0]; i < cluster_number;         \
	     i++, cpumask = &cluster_cpumask[i])

void __weak glk_update_util(struct rq *rq, unsigned int flags)
{
}

u64 __weak get_migt_thresh(void)
{
	return 100;
}

static inline int game_load_inuse(void)
{
	u64 now = ktime_get_ns();

	return (now - last_update_time < NSEC_PER_SEC);
}

int glk_enable(void)
{
	if (glk_disable)
		return 0;

	// enable in migt 3.0
	return (game_load_inuse() && (migt_enable() > 2));
}

static u64 game_runtime_update(struct game_runtime *load, u64 delta,
			       bool new_frame)
{
	load->sum_exec_runtime += delta;
	if (new_frame) {
		load->prev_sum_exec_runtime = load->sum_exec_runtime;
		load->sum_exec_runtime = 0;
	}

	return load->prev_sum_exec_runtime;
}

void game_load_update(struct task_struct *task, u64 delta, int cpu)
{
	int i, flag[FREQ_LOAD_ITEMS];
	unsigned long rflag;

	if (unlikely(!game_load_has_init))
		return;

	flag[FREQ_CPU_LOAD] = 1;
	flag[FREQ_GAME_LOAD] = game_task(task) ? 1 : 0;
	flag[FREQ_VIP_LOAD] = game_vip_task(task) ? 1 : 0;
	flag[FREQ_STASK_LOAD] = game_super_task(task) ? 1 : 0;
	flag[FREQ_RENDER_LOAD] = is_render_thread(task);

	raw_spin_lock_irqsave(&per_cpu(load_info, cpu).loading_slock, rflag);
	for (i = 0; i < FREQ_LOAD_ITEMS; i++) {
		if (!flag[i])
			continue;

		game_runtime_update(&per_cpu(load_info, cpu).load[i], delta, 0);
	}
	raw_spin_unlock_irqrestore(&per_cpu(load_info, cpu).loading_slock,
				   rflag);
}

static void game_load_history_items_reset(int history_items)
{
	int cluster;
	struct game_load_items *game_load;

	if ((history_items >= FRAME_LOAD_HISITMES) || history_items < 0)
		return;

	game_load = &game_load_history[history_items];
	for (cluster = 0; cluster < cluster_number; cluster++)
		game_load->cpu_util[cluster] = 0;

	game_load->frame_start = 0;
	game_load->frame_period = 0;
}

void glk_irq_work(struct irq_work *irq_work)
{
	struct sched_cluster *cluster;
	struct rq *rq;
	int cpu;
	u64 wc;
	int level = 0;

	for_each_cpu (cpu, cpu_possible_mask) {
		if (level == 0)
			raw_spin_lock(&cpu_rq(cpu)->lock);
		else
			raw_spin_lock_nested(&cpu_rq(cpu)->lock, level);
		level++;
	}

	wc = sched_ktime_clock();
	for_each_sched_cluster(cluster)
	{
		raw_spin_lock(&cluster->load_lock);
		for_each_cpu (cpu, &cluster->cpus) {
			rq = cpu_rq(cpu);
			if (rq->curr)
				update_task_ravg(rq->curr, rq, TASK_UPDATE, wc,
						 0);
		}
		raw_spin_unlock(&cluster->load_lock);
	}

	for_each_sched_cluster(cluster)
	{
		cpumask_t cluster_online_cpus;
		unsigned int num_cpus, i = 1;

		cpumask_and(&cluster_online_cpus, &cluster->cpus,

			    cpu_online_mask);
		num_cpus = cpumask_weight(&cluster_online_cpus);
		for_each_cpu (cpu, &cluster_online_cpus) {
			int flag = SCHED_CPUFREQ_GLK;

			rq = cpu_rq(cpu);
			if (i == num_cpus)
				glk_update_util(cpu_rq(cpu), flag);
			else
				glk_update_util(cpu_rq(cpu),
						flag | SCHED_CPUFREQ_CONTINUE);
			i++;
		}
	}

	for_each_cpu (cpu, cpu_possible_mask)
		raw_spin_unlock(&cpu_rq(cpu)->lock);
}

void glk_force_maxfreq_break(bool val)
{
	force_maxfreq_break = val;
}

void glk_maxfreq_break(bool val)
{
	if (likely(glk_fbreak_enable))
		glk_maxfreq_limit = !val;
}

void glk_minfreq_break(bool val)
{
	if (likely(glk_fbreak_enable))
		glk_minfreq_limit = !val;
}

unsigned int glk_freq_limit(struct cpufreq_policy *policy,
			    unsigned int *target_freq)
{
	unsigned int soft_minfreq, soft_maxfreq;
	int cid;

	if ((!glk_freq_limit_walt) || unlikely(!glk_freq_limit_start))
		return *target_freq;

	cid = per_cpu(load_info, policy->cpu).cluster;
	soft_maxfreq = (glk_maxfreq_limit && !force_maxfreq_break) ?
				     glk_maxfreq[cid] :
				     policy->max;
	soft_minfreq = glk_minfreq_limit ? glk_minfreq[cid] : policy->min;

	soft_minfreq = max(policy->min, soft_minfreq);
	if (!soft_maxfreq)
		soft_maxfreq = policy->max;

	*target_freq = min(soft_maxfreq, max(*target_freq, soft_minfreq));
	return *target_freq;
}

unsigned long glk_cal_freq(struct cpufreq_policy *policy, unsigned long util,
			   unsigned long max)
{
	unsigned int cid, prev_items, cur_items = cur_history_items;
	u64 glk_load, game_load, frame_period = 0;
	struct game_load_items *prev_game_load;
	unsigned int target_freq;
	unsigned int soft_minfreq, soft_maxfreq;

	if (!glk_enable())
		return 0;

	if (unlikely(cur_items == 0))
		prev_items = FRAME_LOAD_HISITMES - 1;
	else
		prev_items = cur_items - 1;

	prev_game_load = &game_load_history[prev_items];
	frame_period = prev_game_load->frame_period / NSEC_PER_MSEC;
	cid = per_cpu(load_info, policy->cpu).cluster;
	game_load = prev_game_load->cpu_util[cid];
	glk_load = game_load * 100 / capacity_orig_of(policy->cpu);

	soft_minfreq = glk_maxfreq_limit ? glk_maxfreq[cid] : policy->max;
	soft_maxfreq = glk_minfreq_limit ? glk_minfreq[cid] : policy->min;
	if (unlikely((soft_maxfreq < soft_minfreq) || !frame_period))
		return 0;

	if (!glk_minfreq[cid])
		glk_minfreq[cid] = policy->min;

	if (unlikely(frame_period > (get_migt_thresh() << 1))) {
		if (policy->cur >= soft_maxfreq && perf_mod)
			return policy->max;

		return soft_maxfreq;
	}

	if (glk_load > glk_highspeed_load[cid])
		return soft_maxfreq;

	if (glk_load < glk_lowspeed_load[cid])
		return soft_minfreq;

	target_freq = (glk_load * (soft_maxfreq + (soft_maxfreq >> 3)) / 100);
	target_freq = max(soft_minfreq, min(soft_maxfreq, target_freq));

	if (unlikely(game_load_debug))
		pr_info("glk_load %d, target_freq %d\n", glk_load, target_freq);

	return target_freq;
}

static u64 update_glk_load(struct cpumask *mask, int index, bool roll)
{
	int i, cpu, policy = sysctl_game_load_choose_policy;
	u64 total_load[FREQ_LOAD_ITEMS], cpu_load[FREQ_LOAD_ITEMS];
	u64 target_load = 0;
	struct game_load_items *cur_frame_load;
	struct game_runtime *cpu_runtime;

	cur_frame_load = &game_load_history[index];
	memset(total_load, 0, sizeof(u64) * FREQ_LOAD_ITEMS);

	for_each_cpu (cpu, mask) {
		for (i = 0; i < FREQ_LOAD_ITEMS; i++) {
			cpu_runtime = &per_cpu(load_info, cpu).load[i];
			cpu_load[i] = game_runtime_update(cpu_runtime, 0, roll);

			if (unlikely(i == FREQ_CPU_LOAD)) {
				if (cpu_load[i] >= total_load[i])
					total_load[i] = cpu_load[i];
			} else
				total_load[i] += cpu_load[i];
		}
	}

	switch (policy) {
	case FREQ_CPU_LOAD:
	case FREQ_GAME_LOAD:
	case FREQ_STASK_LOAD:
	case FREQ_VIP_LOAD:
	case FREQ_RENDER_LOAD:
		target_load = total_load[policy];
		break;

	default:
		target_load = total_load[FREQ_STASK_LOAD];
		break;
	}

	return target_load;
}

static int get_target_period(int migt_thresh)
{
	if (likely(old_migt_thresh == migt_thresh) && frame_tperiod)
		return frame_tperiod;

	old_migt_thresh = migt_thresh;
	switch (migt_thresh) {
	case 11 ... 16:
		frame_tperiod = TARGET_PERIOD(90);
		break;
	case 17 ... 24:
		frame_tperiod = TARGET_PERIOD(60);
		break;
	case 25 ... 33:
		frame_tperiod = TARGET_PERIOD(40);
		break;
	case 34 ... 100:
		frame_tperiod = TARGET_PERIOD(30);
		break;
	default:
		frame_tperiod = TARGET_PERIOD(30);
		break;
	};

	return frame_tperiod;
}

void game_load_history_update(u64 tick)
{
	int cpu_util, migt_thresh, index = 0;
	struct game_load_items *cur_frame_load;
	struct cpumask *mask;

	u64 target_load, now = ktime_get_ns();

	if (unlikely(!game_load_has_init))
		return;

	last_update_time = now;
	migt_thresh = get_migt_thresh();
	cur_frame_load = &game_load_history[cur_history_items];
	if (likely(cur_frame_load->frame_start)) {
		cur_frame_load->frame_period =
			now - cur_frame_load->frame_start;

		cpu_util_freq_divisor = get_target_period(migt_thresh) << 10;

		if (!cpu_util_freq_divisor)
			return;
	} else {
		/*first frame start*/
		cur_frame_load->frame_start = now;
		return;
	}

	for_each_cluster_cpumask(index, mask)
	{
		if (index >= cluster_number)
			break;

		target_load = update_glk_load(mask, cur_history_items, true);
		cpu_util = div64_u64(target_load, cpu_util_freq_divisor);
		cur_frame_load->cpu_util[index] = cpu_util;
	}

	/*new game period*/
	cur_history_items++;
	cur_history_items %= FRAME_LOAD_HISITMES;
	game_load_history_items_reset(cur_history_items);
	game_load_history[cur_history_items].frame_start = now;
	irq_work_queue(&glk_cpufreq_irq_work);
}

void game_load_reset(void)
{
	int i, j, cpu, index = 0;
	unsigned long flag;
	struct cpumask *mask;
	struct game_runtime *cpu_runtime;

	for (i = 0; i < FRAME_LOAD_HISITMES; i++) {
		game_load_history[i].frame_start = 0;
		game_load_history[i].frame_period = 0;
		for (j = 0; j < cluster_number; j++)
			game_load_history[i].cpu_util[j] = 0;
	}
	cur_history_items = 0;

	for_each_cluster_cpumask(index, mask)
	{
		if (index >= cluster_number)
			break;

		for_each_cpu (cpu, mask) {
			raw_spin_lock_irqsave(
				&per_cpu(load_info, cpu).loading_slock, flag);
			for (i = 0; i < FREQ_LOAD_ITEMS; i++) {
				cpu_runtime = &per_cpu(load_info, cpu).load[i];
				cpu_runtime->sum_exec_runtime = 0;
				cpu_runtime->prev_sum_exec_runtime = 0;
			}
			raw_spin_unlock_irqrestore(
				&per_cpu(load_info, cpu).loading_slock, flag);
		}
	}
}

/* proc file for debug*/
static inline void dump_his_load_info(int index, struct seq_file *m, void *v)
{
	char cluster_name[4] = "lmb";
	int j, i = index % FRAME_LOAD_HISITMES;

	if (!game_load_history[i].frame_period)
		return;

	seq_printf(m, "period %llu:\n", game_load_history[i].frame_period);
	for (j = 0; j < cluster_number; j++)
		seq_printf(m, "%c: %llu %lu\t\n", cluster_name[j],
			   game_load_history[i].cpu_util[j],
			   game_load_history[i].cap_states[j].frequency);
	seq_puts(m, "\n");
}

static int history_load_show(struct seq_file *m, void *v)
{
	int i, index = 0;

	index = cur_history_items;

	for (i = index + 1; i < FRAME_LOAD_HISITMES; i++)
		dump_his_load_info(i, m, v);

	for (i = 0; i <= index; i++)
		dump_his_load_info(i, m, v);

	//dump_cap_buckets();
	return 0;
}

static int history_load_open(struct inode *inode, struct file *file)
{
	return single_open(file, history_load_show, NULL);
}

static const struct file_operations history_load_fops = {
	.open = history_load_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int cpu_load_show(struct seq_file *m, void *v)
{
	int i, cpu, index;
	int period, prev_history_items;
	unsigned long game_load;
	struct game_runtime *cpu_runtime;
	struct cpumask *mask;

	if (!cur_history_items)
		prev_history_items = FRAME_LOAD_HISITMES - 1;
	else
		prev_history_items = cur_history_items - 1;
	period = game_load_history[prev_history_items].frame_period;

	seq_printf(m, "Period :%llu\n", period);
	for_each_cluster_cpumask(index, mask)
	{
		if (index >= cluster_number)
			break;

		seq_printf(m, "---cluster %d---\n", index);
		for_each_cpu (cpu, mask) {
			for (i = 0; i < FREQ_LOAD_ITEMS; i++) {
				cpu_runtime = &per_cpu(load_info, cpu).load[i];
				if (!cpu_util_freq_divisor)
					cpu_util_freq_divisor = 1;

				game_load = div64_u64(
					cpu_runtime->prev_sum_exec_runtime,
					cpu_util_freq_divisor);

				seq_printf(m, "%9d \t", game_load);
			}
		}
		seq_puts(m, "\n");
	}

	return 0;
}

static int cpu_load_open(struct inode *inode, struct file *file)
{
	return single_open(file, cpu_load_show, NULL);
}

static const struct file_operations cpu_load_fops = {
	.open = cpu_load_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

void create_game_load_proc(void *rootdir)
{
	struct proc_dir_entry *pkg_rootdir = NULL;

	if (rootdir)
		pkg_rootdir = (struct proc_dir_entry *)rootdir;
	else
		return;

	game_load_dir = proc_mkdir("glk", pkg_rootdir);
	if (!game_load_dir)
		return;

	history_load_entry = proc_create("game_history_load", 0664,
					 game_load_dir, &history_load_fops);
	cpu_load_entry = proc_create("game_cpu_load", 0664, game_load_dir,
				     &cpu_load_fops);
}

void delete_game_load_proc(void)
{
	if (!game_load_dir)
		return;

	if (cpu_load_entry)
		proc_remove(cpu_load_entry);
	if (history_load_entry)
		proc_remove(history_load_entry);
	proc_remove(game_load_dir);
}

/* sysctl interface */
int proc_glk_dointvec(struct ctl_table *table, int write, void __user *buffer,
		      size_t *lenp, loff_t *ppos, glk_dointvec_f f)
{
	int ret;
	unsigned int *data = (unsigned int *)table->data;
	unsigned int *old_val;
	static DEFINE_MUTEX(mutex);

	mutex_lock(&mutex);

	if (table->maxlen != (sizeof(int) * MAX_CLUSTERS))
		table->maxlen = sizeof(int) * MAX_CLUSTERS;

	if (!f)
		f = proc_dointvec;

	if (!write) {
		ret = f(table, write, buffer, lenp, ppos);
		goto unlock_mutex;
	}

	/*
	 * Cache the old values so that they can be restored
	 * if either the write fails (for example out of range values)
	 */
	old_val = kzalloc(table->maxlen, GFP_KERNEL);
	if (!old_val) {
		ret = -ENOMEM;
		goto unlock_mutex;
	}

	memcpy(old_val, data, table->maxlen);
	ret = f(table, write, buffer, lenp, ppos);

	if (ret) {
		memcpy(data, old_val, table->maxlen);
		goto free_old_val;
	}

free_old_val:
	kfree(old_val);
unlock_mutex:
	mutex_unlock(&mutex);

	return ret;
}

static int proc_glk_dotload(struct ctl_table *table, int write,
			    void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int i, ret = proc_glk_dointvec(table, write, buffer, lenp, ppos, NULL);

	if (!game_load_debug)
		return ret;

	for (i = 0; i < MAX_CLUSTERS; i++)
		pr_info("cid %d minload %d maxload %d\n", i,
			glk_lowspeed_load[i], glk_highspeed_load[i]);
	return ret;
}

static int proc_glk_dofreq(struct ctl_table *table, int write,
			   void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int i, ret = proc_glk_dointvec(table, write, buffer, lenp, ppos, NULL);

	if (!game_load_debug)
		return ret;

	for (i = 0; i < MAX_CLUSTERS; i++)
		pr_info("cid %d minfreq %d maxfreq %d\n", i, glk_minfreq[i],
			glk_minfreq[i]);
	return ret;
}

static struct ctl_table game_load_table[] = {
	{
		.procname = "game_load_choose_policy",
		.data = &sysctl_game_load_choose_policy,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{
		.procname = "glk_disable",
		.data = &glk_disable,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{
		.procname = "glk_freq_limit_walt",
		.data = &glk_freq_limit_walt,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{
		.procname = "game_hispeed_load",
		.data = &glk_highspeed_load,
		.maxlen = sizeof(int) * MAX_CLUSTERS,
		.mode = 0644,
		.proc_handler = proc_glk_dotload,
	},
	{
		.procname = "game_lowspeed_load",
		.data = &glk_lowspeed_load,
		.maxlen = sizeof(int) * MAX_CLUSTERS,
		.mode = 0644,
		.proc_handler = proc_glk_dotload,
	},
	{
		.procname = "game_minfreq_limit",
		.data = &glk_minfreq,
		.maxlen = sizeof(int) * MAX_CLUSTERS,
		.mode = 0644,
		.proc_handler = proc_glk_dofreq,
	},
	{
		.procname = "freq_break_enable",
		.data = &glk_fbreak_enable,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{
		.procname = "game_maxfreq_limit",
		.data = &glk_maxfreq,
		.maxlen = sizeof(int) * MAX_CLUSTERS,
		.mode = 0644,
		.proc_handler = proc_glk_dofreq,
	},
	{
		.procname = "perf_mod",
		.data = &perf_mod,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{
		.procname = "load_debug",
		.data = &game_load_debug,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{}
};

static struct ctl_table game_ctl_root[] = { {
						    .procname = "glk",
						    .mode = 0555,
						    .child = game_load_table,
					    },
					    {} };

int game_load_init(void)
{
	struct cpumask cpus = *cpu_possible_mask;
	const struct cpumask *cluster_cpus;
	int i, cpu, cluster = 0;

	for_each_cpu (i, &cpus) {
		if (cluster >= MAX_CLUSTERS)
			break;

		cluster_cpus = cpu_coregroup_mask(i);
		if (i != cpumask_first(cluster_cpus))
			continue;

		for_each_cpu (cpu, cluster_cpus) {
			per_cpu(load_info, cpu).cluster = cluster;
			raw_spin_lock_init(
				&per_cpu(load_info, cpu).loading_slock);
		}

		cpumask_copy(&cluster_cpumask[cluster++], cluster_cpus);
		cpumask_andnot(&cpus, &cpus, cluster_cpus);
	}

	cluster_number = cluster;

	WARN_ON(game_load_header);
	init_irq_work(&glk_cpufreq_irq_work, glk_irq_work);
	game_load_header = register_sysctl_table(game_ctl_root);
	game_load_has_init = 1;
	pr_err("game load init success\n");

	return 0;
}

late_initcall(game_load_init);
