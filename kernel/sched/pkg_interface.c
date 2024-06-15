#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/sched/signal.h>
#include <linux/init.h>
#include <linux/pkg_stat.h>
#include "pkg.h"

#define TRACE_UID_MAX 81920

static char *top_package[PACKAGE_TYPES] = {
	"g:     top_package",
	"score: top_package",
	"bcore: top_package",
	"lcore: top_package",
};

static int runtime_traced_uid = 1000;
static int fb_traced_uid = 1000;
static int traced_window = 3;
static int top_package_uid[PACKAGE_TYPES];
static int cur_render_uid;
static int develop_mode = 1;
static struct user_struct *user_zero;
int pause_mode;

static struct proc_dir_entry *package_rootdir;
static struct proc_dir_entry *package_dir;
static struct proc_dir_entry *stat_dir;
static struct proc_dir_entry *migt_dir;
static struct proc_dir_entry *package_fb_showall;
static struct proc_dir_entry *package_fb_traced_show;
static struct proc_dir_entry *package_condition_reset;
static struct proc_dir_entry *package_fb_status_set;
static struct proc_dir_entry *package_showall_entry;
static struct proc_dir_entry *package_tracestat_entry;
static struct proc_dir_entry *package_top_bcore_entry;
static struct proc_dir_entry *package_top_lcore_entry;
static struct proc_dir_entry *package_top_entry;
static struct proc_dir_entry *package_trace_entry;
static struct proc_dir_entry *package_window_size_entry;
static struct proc_dir_entry *package_develop_mode_entry;
static struct proc_dir_entry *package_runtime_entry;
static struct proc_dir_entry *package_trace_window;
static struct proc_dir_entry *exec_buckets_entry;
static struct proc_dir_entry *package_pause_mode_entry;
static struct proc_dir_entry *package_resettraced_window;
static struct runtime_info *package_runtime_info;
__weak DECLARE_BITMAP(package_uid_bit, (TRACE_UID_MAX + 1));
static unsigned int fb_uid;
static int fb_condition;
static int runtime_show_all(struct seq_file *m, void *v)
{
	int scluster_usage = 0;
	int bcluster_usage = 0;
	int lcluster_usage = 0;
	u64 scluster_total_usage = 0;
	u64 bcluster_total_usage = 0;
	u64 lcluster_total_usage = 0;
	int scluster_bias = 0;
	int bcluster_bias = 0;
	int app_run_on_scluster, app_run_on_bcluster, app_run_on_lcluster;
	int top_package_load[PACKAGE_TYPES];
	u64 package_runtime;
	struct user_struct *user;
	kuid_t uid;
	int i, bit;
	int last_uid =
		MIN(*((int *)get_uid_max_value()) + 1, TRACE_UID_MAX - 1);
	int print_user_zero = 0;
	int pprev_index = HISTORY_ITMES + 1;
	int prev_index = HISTORY_ITMES;
	u64 delta_exec =
		jiffies_to_nsecs(get_jiffies_64() -
				 package_runtime_info[pprev_index].update_time);
	package_runtime_info[prev_index].update_time = get_jiffies_64();

	if (develop_mode) {
		pprev_index = HISTORY_ITMES + 1;
		prev_index = HISTORY_ITMES;
		package_runtime_info[prev_index].update_time = get_jiffies_64();
		delta_exec = jiffies_to_nsecs(
			get_jiffies_64() -
			package_runtime_info[pprev_index].update_time);
	}

	if (delta_exec <= 0)
		return 0;

	memset(top_package_load, 0, PACKAGE_TYPES * sizeof(int));
	seq_printf(m, "time info: from [%llu]s to [%llu]s ago\n",
		   (get_jiffies_64() -
		    package_runtime_info[prev_index].update_time) /
			   HZ,
		   (get_jiffies_64() -
		    package_runtime_info[pprev_index].update_time) /
			   HZ);

	for_each_set_bit (bit, package_uid_bit, last_uid) {
		uid.val = bit;
		user = find_user(uid);

		if (bit == last_uid && !print_user_zero) {
			user = user_zero;
			print_user_zero = 1;
			pr_info("-----------non user info -----------------\n");
		}

		if (!user)
			continue;

		package_runtime =
			(user->pkg.sup_cluster_runtime[prev_index] +
			 user->pkg.big_cluster_runtime[prev_index] +
			 user->pkg.little_cluster_runtime[prev_index]) -
			(user->pkg.sup_cluster_runtime[pprev_index] +
			 user->pkg.big_cluster_runtime[pprev_index] +
			 user->pkg.little_cluster_runtime[pprev_index]);

		if (!package_runtime) {
			free_uid(user);
			continue;
		}

		if ((user->pkg.sup_cluster_runtime[prev_index] <
		     user->pkg.sup_cluster_runtime[pprev_index]) ||
		    (user->pkg.big_cluster_runtime[prev_index] <
		     user->pkg.big_cluster_runtime[pprev_index]) ||
		    (user->pkg.little_cluster_runtime[prev_index] <
		     user->pkg.little_cluster_runtime[pprev_index])) {
			free_uid(user);
			continue;
		}

		app_run_on_scluster =
			(int)((user->pkg.sup_cluster_runtime[prev_index] -
			       user->pkg.sup_cluster_runtime[pprev_index]) *
			      10000 / delta_exec);
		if (app_run_on_scluster >= top_package_load[TOP_ON_SCORE]) {
			/*top load package in big cluster*/
			top_package_load[TOP_ON_SCORE] = app_run_on_scluster;
			top_package_uid[TOP_ON_SCORE] = bit;
		}

		if (app_run_on_scluster > 0)
			scluster_usage += app_run_on_scluster;

		app_run_on_bcluster =
			(int)((user->pkg.big_cluster_runtime[prev_index] -
			       user->pkg.big_cluster_runtime[pprev_index]) *
			      10000 / delta_exec);
		if (app_run_on_bcluster >= top_package_load[TOP_ON_BCORE]) {
			/*top load package in big cluster*/
			top_package_load[TOP_ON_BCORE] = app_run_on_bcluster;
			top_package_uid[TOP_ON_BCORE] = bit;
		}

		if (app_run_on_bcluster > 0)
			bcluster_usage += app_run_on_bcluster;

		app_run_on_lcluster =
			(int)((user->pkg.little_cluster_runtime[prev_index] -
			       user->pkg.little_cluster_runtime[pprev_index]) *
			      10000 / delta_exec);
		if (app_run_on_lcluster >= top_package_load[TOP_ON_LCORE]) {
			/*top load package in little cluster*/
			top_package_load[TOP_ON_LCORE] = app_run_on_lcluster;
			top_package_uid[TOP_ON_LCORE] = bit;
		}

		if (app_run_on_lcluster > 0)
			lcluster_usage += app_run_on_lcluster;

		if (app_run_on_scluster + app_run_on_bcluster +
			    app_run_on_lcluster >
		    top_package_load[TOP_ALL]) {
			/*top load package*/
			top_package_load[TOP_ALL] = app_run_on_scluster +
						    app_run_on_bcluster +
						    app_run_on_lcluster;
			top_package_uid[TOP_ALL] = bit;
		}

		bcluster_bias =
			(int)((user->pkg.big_cluster_runtime[prev_index] -
			       user->pkg.big_cluster_runtime[pprev_index]) *
			      10000 / package_runtime);
		if (bcluster_bias < 0)
			bcluster_bias = 0;

		scluster_bias =
			(int)((user->pkg.sup_cluster_runtime[prev_index] -
			       user->pkg.sup_cluster_runtime[pprev_index]) *
			      10000 / package_runtime);
		if (scluster_bias < 0)
			scluster_bias = 0;

		scluster_total_usage +=
			(user->pkg.sup_cluster_runtime[prev_index] -
			 user->pkg.sup_cluster_runtime[pprev_index]);
		bcluster_total_usage +=
			(user->pkg.big_cluster_runtime[prev_index] -
			 user->pkg.big_cluster_runtime[pprev_index]);
		lcluster_total_usage +=
			(user->pkg.little_cluster_runtime[prev_index] -
			 user->pkg.little_cluster_runtime[pprev_index]);

		seq_printf(
			m,
			"%5d: <s>%16llu [%3d.%02d] <b>%16llu [%3d.%02d] <l>: %16llu",
			bit,
			(user->pkg.sup_cluster_runtime[prev_index] -
			 user->pkg.sup_cluster_runtime[pprev_index]),
			app_run_on_scluster / 100, app_run_on_scluster % 100,
			(user->pkg.big_cluster_runtime[prev_index] -
			 user->pkg.big_cluster_runtime[pprev_index]),
			app_run_on_bcluster / 100, app_run_on_bcluster % 100,
			(user->pkg.little_cluster_runtime[prev_index] -
			 user->pkg.little_cluster_runtime[pprev_index]));
		seq_printf(m, " [%3d.%02d]--bias: s-[%3d.%02d] b-[%3d.%02d]\n",
			   scluster_bias / 100, scluster_bias % 100,
			   bcluster_bias / 100, bcluster_bias % 100,
			   app_run_on_lcluster / 100,
			   app_run_on_lcluster % 100);

		free_uid(user);
	}

	seq_printf(
		m,
		"\nusage---sup cluster:[%d.%d] big cluster:[%d.%d] little cluster :[%d.%d]\n",
		scluster_usage / 100, scluster_usage % 100,
		bcluster_usage / 100, bcluster_usage % 100,
		lcluster_usage / 100, lcluster_usage % 100);

	seq_printf(
		m,
		"total_usage---sup cluster:[%llu] big cluster:[%llu] little cluster:[%llu]\n",
		scluster_total_usage, bcluster_total_usage,
		lcluster_total_usage);

	seq_puts(m, "top load package info--- 0 for s/b/l cluster\n");

	for (i = 0; i < PACKAGE_TYPES; i++)
		seq_printf(m, "%s: %d %3d.%02d\n", top_package[i],
			   top_package_uid[i], top_package_load[i] / 100,
			   top_package_load[i] % 100);

	return 0;
}

static int runtime_open_all(struct inode *inode, struct file *file)
{
	return single_open(file, runtime_show_all, NULL);
}

static const struct file_operations package_show_all_fops = {
	.open = runtime_open_all,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int package_runtime_show(struct seq_file *m, void *v)
{
	struct task_struct *p;
	struct user_struct *user;
	kuid_t uid;
	struct list_head *l;
	struct package_runtime_info *pkg;
	u64 scluster_total_usage = 0;
	u64 bcluster_total_usage = 0;
	u64 lcluster_total_usage = 0;
	int app_run_on_scluster, app_run_on_bcluster, app_run_on_lcluster;
	int scluster_bias, bcluster_bias;
	u64 app_run_stime, app_run_btime, app_run_ltime;
	int scluster_usage = 0;
	int bcluster_usage = 0;
	int lcluster_usage = 0;
	int traced_uid = *((int *)m->private);
	int pprev_index = HISTORY_ITMES + 1;
	int prev_index = HISTORY_ITMES;
	u64 delta_exec =
		jiffies_to_nsecs(get_jiffies_64() -
				 package_runtime_info[pprev_index].update_time);
	package_runtime_info[prev_index].update_time = get_jiffies_64();

	if (develop_mode) {
		pprev_index = HISTORY_ITMES + 1;
		prev_index = HISTORY_ITMES;
		package_runtime_info[prev_index].update_time = get_jiffies_64();
		delta_exec = jiffies_to_nsecs(
			get_jiffies_64() -
			package_runtime_info[pprev_index].update_time);
	}

	seq_printf(m, "traced uid %d\n", traced_uid);
	seq_printf(m, "time info: from [%llu]s to [%llu]s ago\n",
		   (get_jiffies_64() -
		    package_runtime_info[prev_index].update_time) /
			   HZ,
		   (get_jiffies_64() -
		    package_runtime_info[pprev_index].update_time) /
			   HZ);

	rcu_read_lock();
	uid.val = traced_uid;
	user = find_user(uid);

	list_for_each (l, &(user->pkg.list)) {
		pkg = container_of(l, struct package_runtime_info, list);
		p = container_of(pkg, struct task_struct, pkg);

		app_run_stime = p->pkg.sup_cluster_runtime[prev_index] -
				p->pkg.sup_cluster_runtime[pprev_index];
		app_run_btime = p->pkg.big_cluster_runtime[prev_index] -
				p->pkg.big_cluster_runtime[pprev_index];
		app_run_ltime = p->pkg.little_cluster_runtime[prev_index] -
				p->pkg.little_cluster_runtime[pprev_index];

		if (!(app_run_stime + app_run_btime + app_run_ltime))
			continue;

		if ((p->pkg.sup_cluster_runtime[prev_index] <
		     p->pkg.sup_cluster_runtime[pprev_index]) ||
		    (p->pkg.big_cluster_runtime[prev_index] <
		     p->pkg.big_cluster_runtime[pprev_index]) ||
		    (p->pkg.little_cluster_runtime[prev_index] <
		     p->pkg.little_cluster_runtime[pprev_index])) {
			continue;
		}

		app_run_on_scluster = (int)(app_run_stime * 10000 / delta_exec);
		if (app_run_on_scluster > 0)
			scluster_usage += app_run_on_scluster;
		else
			app_run_on_scluster = 0;

		app_run_on_bcluster = (int)(app_run_btime * 10000 / delta_exec);
		if (app_run_on_bcluster > 0)
			bcluster_usage += app_run_on_bcluster;
		else
			app_run_on_bcluster = 0;

		app_run_on_lcluster = (int)(app_run_ltime * 10000 / delta_exec);
		if (app_run_on_lcluster > 0)
			lcluster_usage += app_run_on_lcluster;
		else
			app_run_on_lcluster = 0;

		scluster_bias =
			(int)(app_run_stime * 10000 /
			      (app_run_stime + app_run_btime + app_run_ltime));
		if (scluster_bias < 0)
			scluster_bias = 0;
		bcluster_bias =
			(int)(app_run_btime * 10000 /
			      (app_run_stime + app_run_btime + app_run_ltime));
		if (bcluster_bias < 0)
			bcluster_bias = 0;

		scluster_total_usage += app_run_stime;
		bcluster_total_usage += app_run_btime;
		lcluster_total_usage += app_run_ltime;
		seq_printf(m,
			   "%5d,%16s,s:%16llu [%3d.%02d],b:%16llu [%3d.%02d]",
			   p->pid, p->comm, app_run_stime,
			   app_run_on_scluster / 100, app_run_on_scluster % 100,
			   app_run_btime, app_run_on_bcluster / 100,
			   app_run_on_bcluster % 100);
		seq_printf(m,
			   "l:%16llu [%3d.%02d],| s:[%3d.%02d], b:[%3d.%02d]\n",
			   app_run_ltime, app_run_on_lcluster / 100,
			   app_run_on_lcluster % 100, scluster_bias / 100,
			   scluster_bias % 100, bcluster_bias / 100,
			   bcluster_bias % 100);
	}

	seq_printf(m, "usage total sup:[%d.%d] big:[%d.%d] little:[%d.%d]\n",
		   scluster_usage / 100, scluster_usage % 100,
		   bcluster_usage / 100, bcluster_usage % 100,
		   lcluster_usage / 100, lcluster_usage % 100);
	seq_printf(
		m,
		"\ntotal_usage---sup cluster:[%llu] big cluster:[%llu] little cluster:[%llu]\n",
		scluster_total_usage, bcluster_total_usage,
		lcluster_total_usage);

	free_uid(user);
	rcu_read_unlock();
	return 0;
}

static int package_runtime_open(struct inode *inode, struct file *file)
{
	struct seq_file *m;
	int ret = single_open(file, package_runtime_show, NULL);

	if (!ret) {
		m = file->private_data;
		m->private = &runtime_traced_uid;
	}
	return ret;
}

static ssize_t trace_package_write(struct file *filp, const char __user *buf,
				   size_t count, loff_t *f_ops)
{
	unsigned char buffer[32] = { 0 };
	int value = 0;

	if (copy_from_user(buffer, buf, (count > 32 ? 32 : count)))
		return count;

	if (!kstrtoint(buffer, 10, &value)) {
		pr_err("got user info: %d....\n", value);
		runtime_traced_uid = value;
	}

	return count;
}

static const struct file_operations package_trace_fops = {
	.open = package_runtime_open,
	.read = seq_read,
	.write = trace_package_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int fb_show_all(struct seq_file *m, void *v)
{
	struct user_struct *user;
	kuid_t uid;
	int print_user_zero = 0, bit, i;
	int last_uid =
		MIN(*((int *)get_uid_max_value()) + 1, TRACE_UID_MAX - 1);
	u64 front_usage = 0, all_front_usage = 0;
	u64 back_usage = 0, all_back_usage = 0;
	u64 fbindex = HISTORY_ITMES + 2;
	int pprev_index = HISTORY_ITMES + 1;
	int prev_index = HISTORY_ITMES;

	seq_printf(
		m, "time delta: [%llu]s, UNIT is ms!!\n",
		(get_jiffies_64() - package_runtime_info[fbindex].update_time) /
			HZ);

	for_each_set_bit (bit, package_uid_bit, last_uid) {
		uid.val = bit;
		user = find_user(uid);

		if (bit == last_uid && !print_user_zero) {
			user = user_zero;
			print_user_zero = 1;
			pr_info("-----------non user info -----------------\n");
		}

		if (!user)
			continue;

		front_usage = 0;
		back_usage = 0;
		for (i = 0; i < MAX_CLUSTER; i++) {
			front_usage +=
				(user->pkg.front_runtime[prev_index][i] -
				 user->pkg.front_runtime[pprev_index][i]);

			back_usage += (user->pkg.back_runtime[prev_index][i] -
				       user->pkg.back_runtime[pprev_index][i]);
		}
		if (!(front_usage + back_usage)) {
			free_uid(user);
			continue;
		}

		all_front_usage += front_usage;
		all_back_usage += back_usage;

		seq_printf(m, "%5d: <ALL>%16llu <FRONT>%16llu <BACK>%16llu\n",
			   bit, (front_usage + back_usage) / NSEC_PER_MSEC,
			   front_usage / NSEC_PER_MSEC,
			   back_usage / NSEC_PER_MSEC);

		seq_printf(m, "<FRONT>:<l>%16llu, <b>%16llu, <s>%16llu\n",
			   (user->pkg.front_runtime[prev_index][0] -
			    user->pkg.front_runtime[pprev_index][0]) /
				   NSEC_PER_MSEC,
			   (user->pkg.front_runtime[prev_index][1] -
			    user->pkg.front_runtime[pprev_index][1]) /
				   NSEC_PER_MSEC,
			   (user->pkg.front_runtime[prev_index][2] -
			    user->pkg.front_runtime[pprev_index][2]) /
				   NSEC_PER_MSEC);

		seq_printf(m, "<BACK>:<l>%16llu, <b>%16llu, <s>%16llu\n",
			   (user->pkg.back_runtime[prev_index][0] -
			    user->pkg.back_runtime[pprev_index][0]) /
				   NSEC_PER_MSEC,
			   (user->pkg.back_runtime[prev_index][1] -
			    user->pkg.back_runtime[pprev_index][1]) /
				   NSEC_PER_MSEC,
			   (user->pkg.back_runtime[prev_index][2] -
			    user->pkg.back_runtime[pprev_index][2]) /
				   NSEC_PER_MSEC);
		free_uid(user);
	}

	seq_printf(m,
		   "total_usage---\n<ALL>%16llu <FRONT>%16llu <BACK>%16llu\n",
		   (all_front_usage + all_back_usage) / NSEC_PER_MSEC,
		   all_front_usage / NSEC_PER_MSEC,
		   all_back_usage / NSEC_PER_MSEC);

	return 0;
}

static int fb_open_all(struct inode *inode, struct file *file)
{
	return single_open(file, fb_show_all, NULL);
}

static const struct file_operations package_fb_fops = {
	.open = fb_open_all,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int fb_show_uid(struct seq_file *m, void *v)
{
	int traced_uid = *((int *)m->private), i;
	struct task_struct *p, *tsk;
	int fbindex = HISTORY_ITMES + 2;
	int pprev_index = HISTORY_ITMES + 1;
	int prev_index = HISTORY_ITMES;
	u64 front_usage = 0, all_front_usage = 0;
	u64 back_usage = 0, all_back_usage = 0;

	seq_printf(
		m, "time delta:[%llu]s, UNIT is ms\n",
		(get_jiffies_64() - package_runtime_info[fbindex].update_time) /
			HZ);

	rcu_read_lock();
	for_each_process (tsk) {
		if (task_uid(tsk).val != traced_uid)
			continue;

		for_each_thread (tsk, p) {
			front_usage = 0;
			back_usage = 0;
			for (i = 0; i < MAX_CLUSTER; i++) {
				front_usage +=
					(p->pkg.front_runtime[prev_index][i] -
					 p->pkg.front_runtime[pprev_index][i]);
				back_usage +=
					(p->pkg.back_runtime[prev_index][i] -
					 p->pkg.back_runtime[pprev_index][i]);
			}

			if (!front_usage && !back_usage)
				continue;

			all_front_usage += front_usage;
			all_back_usage += back_usage;

			seq_printf(
				m,
				"%5d,%16s: <ALL>%16llu <FRONT>%16llu <BACK>%16llu\n",
				p->pid, p->comm,
				(front_usage + back_usage) / NSEC_PER_MSEC,
				front_usage / NSEC_PER_MSEC,
				back_usage / NSEC_PER_MSEC);
			seq_printf(m,
				   "<FRONT>: <l>%16llu, <b>%16llu, <s>%16llu\n",
				   (p->pkg.front_runtime[prev_index][0] -
				    p->pkg.front_runtime[pprev_index][0]) /
					   NSEC_PER_MSEC,
				   (p->pkg.front_runtime[prev_index][1] -
				    p->pkg.front_runtime[pprev_index][1]) /
					   NSEC_PER_MSEC,
				   (p->pkg.front_runtime[prev_index][2] -
				    p->pkg.front_runtime[pprev_index][2]) /
					   NSEC_PER_MSEC);
			seq_printf(m,
				   "<BACK>: <l>%16llu, <b>%16llu, <s>%16llu\n",
				   (p->pkg.back_runtime[prev_index][0] -
				    p->pkg.back_runtime[pprev_index][0]) /
					   NSEC_PER_MSEC,
				   (p->pkg.back_runtime[prev_index][1] -
				    p->pkg.back_runtime[pprev_index][1]) /
					   NSEC_PER_MSEC,
				   (p->pkg.back_runtime[prev_index][2] -
				    p->pkg.back_runtime[pprev_index][2]) /
					   NSEC_PER_MSEC);
		}
	}

	seq_printf(m, "%5d: <ALL>%16llu <FRONT>%16llu <BACK>%16llu\n",
		   traced_uid,
		   (all_front_usage + all_back_usage) / NSEC_PER_MSEC,
		   all_front_usage / NSEC_PER_MSEC,
		   all_back_usage / NSEC_PER_MSEC);

	rcu_read_unlock();
	return 0;
}

static int fb_open_uid(struct inode *inode, struct file *file)
{
	struct seq_file *m;
	int ret = single_open(file, fb_show_uid, NULL);

	if (!ret) {
		m = file->private_data;
		m->private = &fb_traced_uid;
	}
	return ret;
}

static ssize_t fb_uid_write(struct file *filp, const char __user *buf,
			    size_t count, loff_t *f_ops)
{
	unsigned char buffer[32] = { 0 };
	int value = 0;

	if (copy_from_user(buffer, buf, (count > 32 ? 32 : count)))
		return count;

	if (!kstrtoint(buffer, 10, &value)) {
		pr_err("got user info: %d....\n", value);
		fb_traced_uid = value;
	}

	return count;
}

static const struct file_operations package_fb_uid_fops = {
	.open = fb_open_uid,
	.read = seq_read,
	.write = fb_uid_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static void reset_fb(int window_idx)
{
	struct user_struct *user;
	int uid_max_value = *((int *)get_uid_max_value());
	int bit, i;
	kuid_t uid;
	int fbindex = HISTORY_ITMES + 2;
	int curr_runtime_items = *((int *)get_curr_runtime_items);

	if (window_idx < 0 || window_idx >= HISTORY_ITMES)
		window_idx = (curr_runtime_items + 2) % HISTORY_ITMES;
	else
		window_idx = (HISTORY_ITMES + curr_runtime_items - window_idx) %
			     HISTORY_ITMES;

	if (develop_mode)
		window_idx = HISTORY_ITMES;

	for_each_set_bit (bit, package_uid_bit,
			  MIN(uid_max_value + 1, TRACE_UID_MAX - 1)) {
		uid.val = bit;
		user = find_user(uid);

		if (user) {
			for (i = 0; i < MAX_CLUSTER; i++) {
				user->pkg.front_runtime[HISTORY_ITMES + 1][i] =
					user->pkg.front_runtime[HISTORY_ITMES]
							       [i];
				user->pkg.back_runtime[HISTORY_ITMES + 1][i] =
					user->pkg.back_runtime[HISTORY_ITMES][i];
			}
		}
		free_uid(user);
	}

	for (i = 0; i < MAX_CLUSTER; i++) {
		user->pkg.front_runtime[HISTORY_ITMES + 1][i] =
			user->pkg.front_runtime[HISTORY_ITMES][i];
		user->pkg.back_runtime[HISTORY_ITMES + 1][i] =
			user->pkg.back_runtime[HISTORY_ITMES][i];
	}

	package_runtime_info[fbindex].update_time = get_jiffies_64();
}

static int pkg_condition_reset(struct seq_file *m, void *v)
{
	reset_fb(traced_window);
	seq_puts(m, "font&back stats reset success\n");
	return 0;
}

static int pkg_cond_reset_open(struct inode *inode, struct file *file)
{
	return single_open(file, pkg_condition_reset, NULL);
}

static const struct file_operations pkg_cond_reset_fops = {
	.open = pkg_cond_reset_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int fb_status_show(struct seq_file *m, void *v)
{
	seq_printf(m, "lastest input: %d:%d\n", fb_uid, fb_condition);
	return 0;
}

static int fb_status_open(struct inode *inode, struct file *file)
{
	return single_open(file, fb_status_show, NULL);
}

static ssize_t fb_status_set(struct file *filp, const char __user *buf,
			     size_t count, loff_t *f_ops)
{
	kuid_t uid;
	struct user_struct *user;
	unsigned char buffer[32] = { 0 };

	if (copy_from_user(buffer, buf, (count > 32 ? 32 : count)))
		return count;

	if (sscanf(buffer, "%d:%d", &fb_uid, &fb_condition) != 2)
		return count;

	pr_info("uid is %d, condition is %d\n", fb_uid, fb_condition);

	uid.val = fb_uid;

	user = find_user(uid);
	if (user == NULL) {
		pr_info("uid is invalid\n");
		return 0;
	}
	user->pkg.edt = fb_condition;
	free_uid(user);

	return count;
}

static const struct file_operations pkg_fb_set_fops = {
	.open = fb_status_open,
	.read = seq_read,
	.write = fb_status_set,
	.llseek = seq_lseek,
	.release = single_release,
};

static int top_package_on_bcore_open(struct inode *inode, struct file *file)
{
	struct seq_file *m;
	int ret = single_open(file, package_runtime_show, NULL);

	if (!ret) {
		m = file->private_data;
		m->private = &top_package_uid[TOP_ON_BCORE];
	}
	return ret;
}

static const struct file_operations package_top_bcore_fops = {
	.open = top_package_on_bcore_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int top_package_on_lcore_open(struct inode *inode, struct file *file)
{
	struct seq_file *m;
	int ret = single_open(file, package_runtime_show, NULL);

	if (!ret) {
		m = file->private_data;
		m->private = &top_package_uid[TOP_ON_LCORE];
	}
	return ret;
}

static const struct file_operations package_top_lcore_fops = {
	.open = top_package_on_lcore_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int top_package_open(struct inode *inode, struct file *file)
{
	struct seq_file *m;
	int ret = single_open(file, package_runtime_show, NULL);

	if (!ret) {
		m = file->private_data;
		m->private = &top_package_uid[TOP_ALL];
	}
	return ret;
}

static const struct file_operations package_top_fops = {
	.open = top_package_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int package_trace_stat(struct seq_file *m, void *v)
{
	seq_printf(m, "traced uid  is %d\n", runtime_traced_uid);
	seq_printf(m, "traced max uid is %d\n", *((int *)get_uid_max_value()));
	return 0;
}

static int package_tracestat_open(struct inode *inode, struct file *file)
{
	return single_open(file, package_trace_stat, NULL);
}

static const struct file_operations package_tracestat_fops = {
	.open = package_tracestat_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static void set_traced_window(int window_idx)
{
	struct user_struct *user;
	struct task_struct *tsk, *ptsk;
	int curr_runtime_items = *((int *)get_curr_runtime_items());
	int uid_max_value = *((int *)get_uid_max_value());
	kuid_t uid;
	int bit;

	if (window_idx < 0 || window_idx >= HISTORY_ITMES)
		window_idx = (curr_runtime_items + 2) % HISTORY_ITMES;
	else
		window_idx = (HISTORY_ITMES + curr_runtime_items - window_idx) %
			     HISTORY_ITMES;

	if (develop_mode)
		window_idx = HISTORY_ITMES;

	for_each_set_bit (bit, package_uid_bit,
			  MIN(uid_max_value + 1, TRACE_UID_MAX - 1)) {
		uid.val = bit;
		user = find_user(uid);

		if (user) {
			user->pkg.sup_cluster_runtime[HISTORY_ITMES + 1] =
				user->pkg.sup_cluster_runtime[window_idx];
			user->pkg.big_cluster_runtime[HISTORY_ITMES + 1] =
				user->pkg.big_cluster_runtime[window_idx];
			user->pkg.little_cluster_runtime[HISTORY_ITMES + 1] =
				user->pkg.little_cluster_runtime[window_idx];
			free_uid(user);
		}

		if (refcount_read(&user->__count) == 1) {
			clear_bit(user->uid.val, package_uid_bit);
			free_uid(user);
		}
	}

	user_zero->pkg.sup_cluster_runtime[HISTORY_ITMES + 1] =
		user_zero->pkg.sup_cluster_runtime[window_idx];
	user_zero->pkg.big_cluster_runtime[HISTORY_ITMES + 1] =
		user_zero->pkg.big_cluster_runtime[window_idx];
	user_zero->pkg.little_cluster_runtime[HISTORY_ITMES + 1] =
		user_zero->pkg.little_cluster_runtime[window_idx];

	package_runtime_info[HISTORY_ITMES + 1].update_time = get_jiffies_64();

	rcu_read_lock();
	for_each_process (ptsk) {
		for_each_thread (ptsk, tsk) {
			tsk->pkg.sup_cluster_runtime[HISTORY_ITMES + 1] =
				tsk->pkg.sup_cluster_runtime[window_idx];
			tsk->pkg.big_cluster_runtime[HISTORY_ITMES + 1] =
				tsk->pkg.big_cluster_runtime[window_idx];
			tsk->pkg.little_cluster_runtime[HISTORY_ITMES + 1] =
				tsk->pkg.little_cluster_runtime[window_idx];
			/*reset migt sched data*/
			migt_monitor_init(tsk);
		}
	}
	rcu_read_unlock();
}

static int reset_traced_window_show(struct seq_file *m, void *v)
{
	set_traced_window(traced_window);
	seq_puts(m, "reset traced window show success!\n");
	return 0;
}

static int package_reset_traced_window(struct inode *inode, struct file *file)
{
	return single_open(file, reset_traced_window_show, NULL);
}

static const struct file_operations package_reset_tracedwindow_fops = {
	.open = package_reset_traced_window,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int traced_window_show(struct seq_file *m, void *v)
{
	seq_printf(m, "traced window is last %d\n", traced_window);
	return 0;
}

static int package_open_traced_window(struct inode *inode, struct file *file)
{
	return single_open(file, traced_window_show, NULL);
}

static ssize_t traced_window_write(struct file *filp, const char __user *buf,
				   size_t count, loff_t *f_ops)
{
	int window_idx = 0;
	unsigned char buffer[32] = { 0 };

	if (copy_from_user(buffer, buf, (count > 32 ? 32 : count)))
		return count;

	if (!kstrtoint(buffer, 10, &window_idx))
		traced_window = window_idx;

	pr_err("traced window %d\n", window_idx);
	set_traced_window(traced_window);

	return count;
}

static const struct file_operations package_tracedwindow_fops = {
	.open = package_open_traced_window,
	.read = seq_read,
	.write = traced_window_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int window_size_show(struct seq_file *m, void *v)
{
	seq_printf(m, "window size is %d\n",
		   *((int *)get_runtime_window_size()));
	return 0;
}

static int package_open_window_size(struct inode *inode, struct file *file)
{
	return single_open(file, window_size_show, NULL);
}

static const struct file_operations package_windowsize_fops = {
	.open = package_open_window_size,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int develop_mode_show(struct seq_file *m, void *v)
{
	seq_printf(m, "develop mode %s\n", develop_mode ? "open" : "close");
	return 0;
}

static int develop_mode_open(struct inode *inode, struct file *file)
{
	return single_open(file, develop_mode_show, NULL);
}

static ssize_t develop_mode_set(struct file *filp, const char __user *buf,
				size_t count, loff_t *f_ops)
{
	unsigned char buffer[32] = { 0 };
	int value = 0;

	if (copy_from_user(buffer, buf, (count > 32 ? 32 : count)))
		return count;

	if (!kstrtoint(buffer, 10, &value))
		develop_mode = value;

	return count;
}

static const struct file_operations develop_mode_fops = {
	.open = develop_mode_open,
	.read = seq_read,
	.write = develop_mode_set,
	.llseek = seq_lseek,
	.release = single_release,
};

static int pause_mode_show(struct seq_file *m, void *v)
{
	seq_printf(m, "pause mode %s\n", pause_mode ? "yes" : "no");
	return 0;
}

static int pause_mode_open(struct inode *inode, struct file *file)
{
	return single_open(file, pause_mode_show, NULL);
}

static ssize_t pause_mode_set(struct file *filp, const char __user *buf,
			      size_t count, loff_t *f_ops)
{
	unsigned char buffer[32] = { 0 };
	int value = 0;

	if (copy_from_user(buffer, buf, (count > 32 ? 32 : count)))
		return count;

	if (!kstrtoint(buffer, 10, &value))
		pause_mode = value;

	return count;
}

static const struct file_operations pause_mode_fops = {
	.open = pause_mode_open,
	.read = seq_read,
	.write = pause_mode_set,
	.llseek = seq_lseek,
	.release = single_release,
};

static int exec_buckets_show(struct seq_file *m, void *v)
{
	struct task_struct *p, *tsk;
	int i, flag, flag2, flag3;
	int traced_uid = *((int *)m->private);

	rcu_read_lock();
	for_each_process (tsk) {
		if (task_uid(tsk).val != traced_uid)
			continue;
		for_each_thread (tsk, p) {
			flag = 0;
			flag2 = 0;
			flag3 = 0;

			if (p->pkg.migt.flag & MINOR_TASK)
				flag3 = 1;
			if (!cpumask_test_cpu(4, &p->cpus_allowed))
				flag = 1;
			if (!cpumask_test_cpu(4, &p->pkg.migt.cpus_allowed))
				flag2 = 1;
			seq_printf(m, "%5d,%16s buckests:{", p->pid, p->comm);

			for (i = 0; i < NUM_MIGT_BUCKETS; i++)
				seq_printf(m, " %8d", p->pkg.migt.bucket[i]);

			seq_printf(
				m, "  } %s %d %d %d\n",
				(game_super_task(p) ?
					       "sup" :
					       (is_render_thread(p) ?
							"ren" :
							(game_vip_task(p) ?
								 "vip" :
								 (game_ip_task(p) ?
									  "imp" :
									  "nor")))),
				flag, flag2, flag3);
		}
	}
	rcu_read_unlock();
	return 0;
}

static int exec_buckets_open(struct inode *inode, struct file *file)
{
	struct seq_file *m;
	int ret = single_open(file, exec_buckets_show, NULL);

	if (!ret) {
		cur_render_uid = get_cur_render_uid();
		m = file->private_data;
		m->private = &runtime_traced_uid;
		if (cur_render_uid >= 0)
			m->private = &cur_render_uid;
	}
	return ret;
}

static const struct file_operations exec_buckets_fops = {
	.open = exec_buckets_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

void create_runtime_proc(void)
{
	pause_mode = 0;
	user_zero = (struct user_struct *)get_user_zero();
	package_rootdir = proc_mkdir("package", NULL);
	if (!package_rootdir)
		return;

	/*pkg runtime info in pkg dir */
	package_dir = proc_mkdir("pkg", package_rootdir);
	if (!package_dir)
		return;

	package_showall_entry = proc_create("show_all", 0664, package_dir,
					    &package_show_all_fops);
	package_top_bcore_entry =
		proc_create("top_package_on_bcore", 0664, package_dir,
			    &package_top_bcore_fops);
	package_top_lcore_entry =
		proc_create("top_package_on_lcore", 0664, package_dir,
			    &package_top_lcore_fops);
	package_top_entry = proc_create("top_package", 0664, package_dir,
					&package_top_fops);
	package_trace_entry = proc_create("show_traced_package", 0664,
					  package_dir, &package_trace_fops);
	package_fb_showall = proc_create("show_fb_showall", 0666, package_dir,
					 &package_fb_fops);
	package_fb_traced_show = proc_create("show_fb_uid_show", 0666,
					     package_dir, &package_fb_uid_fops);
	/*pkg runtime stat*/
	stat_dir = proc_mkdir("stat", package_rootdir);
	if (!stat_dir)
		return;

	package_tracestat_entry = proc_create("show_tracestat", 0664, stat_dir,
					      &package_tracestat_fops);
	package_trace_window = proc_create("show_traced_window", 0664, stat_dir,
					   &package_tracedwindow_fops);
	package_window_size_entry = proc_create(
		"show_windowsize", 0664, stat_dir, &package_windowsize_fops);
	package_develop_mode_entry =
		proc_create("develop_mode", 0664, stat_dir, &develop_mode_fops);
	package_resettraced_window =
		proc_create("reset_traced_window", 0664, stat_dir,
			    &package_reset_tracedwindow_fops);
	package_condition_reset = proc_create("pkg_condition_reset", 0666,
					      stat_dir, &pkg_cond_reset_fops);
	package_fb_status_set =
		proc_create("pkg_fb_set", 0666, stat_dir, &pkg_fb_set_fops);
	package_pause_mode_entry =
		proc_create("pause_mode", 0664, stat_dir, &pause_mode_fops);
	package_runtime_info = get_package_runtime_info();
	/*migt sched info*/
	migt_dir = proc_mkdir("migt", package_rootdir);
	if (!migt_dir)
		return;

	exec_buckets_entry = proc_create("show_exec_buckets", 0664, migt_dir,
					 &exec_buckets_fops);
	create_game_load_proc(migt_dir);
}

void delete_runtimeproc(void)
{
	if (!package_rootdir)
		return;

	if (!package_dir)
		goto remove_rootdir;

	if (package_showall_entry)
		proc_remove(package_showall_entry);
	if (package_tracestat_entry)
		proc_remove(package_tracestat_entry);
	if (package_top_bcore_entry)
		proc_remove(package_top_bcore_entry);
	if (package_top_lcore_entry)
		proc_remove(package_top_lcore_entry);
	if (package_top_entry)
		proc_remove(package_top_entry);
	if (package_fb_showall)
		proc_remove(package_fb_showall);
	if (package_fb_traced_show)
		proc_remove(package_fb_traced_show);
	proc_remove(package_dir);

	if (!stat_dir)
		return;

	if (package_trace_entry)
		proc_remove(package_trace_entry);
	if (package_trace_window)
		proc_remove(package_trace_window);
	if (package_window_size_entry)
		proc_remove(package_window_size_entry);
	if (package_develop_mode_entry)
		proc_remove(package_develop_mode_entry);
	if (package_runtime_entry)
		proc_remove(package_runtime_entry);
	if (package_pause_mode_entry)
		proc_remove(package_pause_mode_entry);
	if (package_resettraced_window)
		proc_remove(package_resettraced_window);
	if (package_condition_reset)
		proc_remove(package_condition_reset);
	if (package_fb_status_set)
		proc_remove(package_fb_status_set);
	proc_remove(stat_dir);

	if (!migt_dir)
		return;

	delete_game_load_proc();
	if (exec_buckets_entry)
		proc_remove(exec_buckets_entry);
	proc_remove(migt_dir);

remove_rootdir:
	proc_remove(package_rootdir);
}
