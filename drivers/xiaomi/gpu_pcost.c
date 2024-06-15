/**
 * Copyright (c) Xiaomi Technologies Co., Ltd. 2019. All rights reserved.
 * File name: gpu_pcost.c
 * Descrviption: cal gpu power cost
 * Author: guchao1@xiaomi.com
 * Version: 1.0
 * Date:  2019/10/10
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) "gpu_energy: " fmt
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cpu.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/security.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/devfreq.h>
#include <linux/gfp.h>
#include <linux/time.h>
#include <linux/tick.h>
#include <linux/msm_adreno_devfreq.h>

struct ea_state {
	unsigned int freq;
	u64 active_time;
	u64 wall_time;
};

struct gpu_devinfo {
	struct notifier_block nb;
	struct devfreq *this;
	struct ea_state estat[MSM_ADRENO_MAX_PWRLEVELS];
	int nr_eas;
	u64 wall_time;
	unsigned int cur_freq;
	struct {
		u64 busy;
		u64 wall;
	} t;
};

static int init_gpu_suc;
static struct gpu_devinfo gdev_zero;
static struct gpu_devinfo *gdev;
static unsigned int disable_pcost = 1;
module_param(disable_pcost, uint, 0644);

static int gpufreq_notifier_trans(struct notifier_block *nb, unsigned long var,
				  void *ptr);
static void gpu_ea_create_file(struct devfreq *devfreq);
static void gpu_ea_remove_file(struct devfreq *devfreq);

int gpu_ea_start(struct devfreq *devf)
{
	int i, ret, nr_eas;

	if (unlikely(!devf || !devf->profile))
		return -ENODEV;

	nr_eas = devf->profile->max_state;
	if ((devf->flag & DF_GPU) || !nr_eas)
		return -EINVAL;

	gpu_ea_create_file(devf);
	devf->ea_private = kcalloc(1, sizeof(struct gpu_devinfo), GFP_NOWAIT);

	if (!devf->ea_private)
		return -ENOMEM;

	gdev = (struct gpu_devinfo *)devf->ea_private;
	memset(gdev->estat, 0, sizeof(struct ea_state));

	for (i = 0; i < nr_eas; i++) {
		gdev->estat[i].freq = devf->profile->freq_table[i];
		gdev->estat[i].active_time = 0;
		gdev->estat[i].wall_time = 0;
	}

	gdev->nr_eas = nr_eas;
	gdev->wall_time = 0;
	gdev->cur_freq = 0;
	gdev->t.busy = 0;
	gdev->t.wall = 0;
	gdev->this = devf;
	gdev->nb.notifier_call = gpufreq_notifier_trans;
	ret = devfreq_register_notifier(devf, &gdev->nb,
					DEVFREQ_TRANSITION_NOTIFIER);
	if (unlikely(ret < 0)) {
		if (likely(devf->ea_private))
			kfree(devf->ea_private);

		devf->ea_private = NULL;
		devf->flag &= ~DF_NORMAL;
		init_gpu_suc = 0;
		gdev = &gdev_zero;
		return ret;
	}

	devf->flag |= DF_GPU;
	init_gpu_suc = 1;

	return 0;
}

/**
 * get_freq_level() - Lookup freq_table for the frequency
 */
static int get_freq_level(struct gpu_devinfo *devf, unsigned long freq)
{
	int lev;

	for (lev = 0; lev < devf->nr_eas; lev++)
		if (freq == devf->estat[lev].freq)
			return lev;

	return -EINVAL;
}

int gpu_ea_update_stats(struct devfreq *devf, u64 busy, u64 wall)
{
	if (unlikely(!devf || !(devf->flag & DF_GPU) || !init_gpu_suc))
		return -EINVAL;
	gdev->t.busy += busy;
	gdev->t.wall += wall;
	return 0;
}

int gpu_ea_stop(struct devfreq *devf)
{
	if (unlikely(!devf || (devf->flag != DF_GPU)))
		return -EINVAL;

	init_gpu_suc = 0;
	devfreq_unregister_notifier(devf, &gdev->nb,
				    DEVFREQ_TRANSITION_NOTIFIER);
	gpu_ea_remove_file(devf);
	gdev = &gdev_zero;
	if (likely(devf->ea_private))
		kfree(devf->ea_private);

	devf->ea_private = NULL;
	devf->flag &= ~DF_NORMAL;

	return 0;
}

static int gpufreq_notifier_trans(struct notifier_block *nb, unsigned long val,
				  void *ptr)
{
	int lev;
	struct gpu_devinfo *g_dev = container_of(nb, struct gpu_devinfo, nb);
	struct devfreq *devfreq = g_dev->this;
	struct devfreq_freqs *freqs = (struct devfreq_freqs *)ptr;

	if (likely(disable_pcost))
		return NOTIFY_DONE;

	if (unlikely(!devfreq || !(devfreq->flag & DF_GPU) || !init_gpu_suc))
		return NOTIFY_DONE;

	if (val != DEVFREQ_POSTCHANGE)
		return NOTIFY_DONE;

	lev = get_freq_level(g_dev, freqs->old);
	if (likely(lev > 0)) {
		if (likely(freqs->old == g_dev->cur_freq)) {
			g_dev->estat[lev].active_time += g_dev->t.busy;
			g_dev->estat[lev].wall_time += g_dev->t.wall;
		}
	}

	g_dev->wall_time += gdev->t.wall;
	g_dev->t.busy = 0;
	g_dev->t.wall = 0;
	g_dev->cur_freq = freqs->new;

	return NOTIFY_DONE;
}

static ssize_t gpu_ea_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	int i;

	if (!init_gpu_suc)
		return count;

	if (gdev && !gdev->nr_eas)
		return count;

	for (i = 0; i < gdev->nr_eas; i++) {
		gdev->estat[i].active_time = 0;
		gdev->estat[i].wall_time = 0;
	}
	gdev->wall_time = 0;

	return count;
}

static ssize_t gpu_ea_show(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	int i, nr_eas, len = 0;
	u64 wall_time, tmp, power = 0;

	if (!init_gpu_suc)
		return len;

	if (gdev && !gdev->nr_eas)
		return len;

	wall_time = gdev->wall_time;
	nr_eas = gdev->nr_eas;

	for (i = 0; i < nr_eas; i++) {
		tmp = gdev->estat[i].active_time * gdev->estat[i].freq;
		tmp = div64_u64(tmp, gdev->estat[i].wall_time);

		len += snprintf(buf + len, PAGE_SIZE - len,
				"f %8d at %16llu wt %16llu\n",
				gdev->estat[i].freq, gdev->estat[i].active_time,
				gdev->estat[i].wall_time);

		power += tmp;
	}

	power = div64_u64(power, wall_time);

	len += snprintf(buf + len, PAGE_SIZE - len, "%llu\n", power);

	return len;
}

static DEVICE_ATTR(gpu_ea, 0444, gpu_ea_show, gpu_ea_store);

static const struct device_attribute *gpu_ea_attr_list[] = { &dev_attr_gpu_ea,
							     NULL };

static void gpu_ea_create_file(struct devfreq *devf)
{
	int i;

	for (i = 0; gpu_ea_attr_list[i] != NULL; i++)
		device_create_file(&devf->dev, gpu_ea_attr_list[i]);
}

static void gpu_ea_remove_file(struct devfreq *devf)
{
	int i;

	for (i = 0; gpu_ea_attr_list[i] != NULL; i++)
		device_remove_file(&devf->dev, gpu_ea_attr_list[i]);
}
