#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/sched/signal.h>

#include "pkg.h"

static int curr_runtime_items;
static int uid_max_value;
static int runtime_window_size = 300;
static int runtime_enable;
static int package_runtime_disable;
static u64 sys_update_time = 0xffffffff;
static struct work_struct package_runtime_roll;
static enum cluster_type cpu_cluster_map[NR_CPUS];
static struct user_struct user_zero = {
	.__count = REFCOUNT_INIT(1),
};

DECLARE_BITMAP(package_uid_bit, (TRACE_UID_MAX + 1));

static struct runtime_info package_runtime_info[HISTORY_WINDOWS + 1];

void init_package_runtime_info(struct user_struct *user);
void package_runtime_hook(u64 now);

get_interface(curr_runtime_items);
get_interface(uid_max_value);
get_interface(runtime_window_size);
get_interface(user_zero);

int __weak package_runtime_should_stop(void)
{
	return 0;
}

static void map_cpu_cluster(void)
{
	unsigned int i, j;
	const struct cpumask *cluster_cpus;
	enum cluster_type clustertype = LITTLE_CLUSTER;

	for_each_online_cpu (i) {
		cluster_cpus = cpu_coregroup_mask(i);
		if (i != cpumask_first(cluster_cpus))
			continue;
		for_each_cpu (j, cluster_cpus)
			cpu_cluster_map[j] = clustertype;
		clustertype++;
	}
}

enum cluster_type get_cluster_type(int cpu)
{
	if (!cpu_cluster_map[cpu])
		map_cpu_cluster();

	return cpu_cluster_map[cpu] < CLUSTER_TYPES ? cpu_cluster_map[cpu] : 0;
}

static void package_runtime_roll_wk(struct work_struct *work)
{
	struct user_struct *user;
	kuid_t uid;
	int bit, i;

	for_each_set_bit (bit, package_uid_bit,
			  MIN(uid_max_value + 1, TRACE_UID_MAX - 1)) {
		uid.val = bit;
		user = find_user(uid);

		if (user) {
			user->pkg.sup_cluster_runtime[curr_runtime_items] =
				user->pkg.sup_cluster_runtime[HISTORY_ITMES];
			user->pkg.little_cluster_runtime[curr_runtime_items] =
				user->pkg.little_cluster_runtime[HISTORY_ITMES];
			user->pkg.big_cluster_runtime[curr_runtime_items] =
				user->pkg.big_cluster_runtime[HISTORY_ITMES];
			user->pkg.little_cluster_runtime[curr_runtime_items] =
				user->pkg.little_cluster_runtime[HISTORY_ITMES];
			for (i = 0; i < MAX_CLUSTER; i++) {
				user->pkg.back_runtime[curr_runtime_items][i] =
					user->pkg.back_runtime[HISTORY_ITMES][i];
				user->pkg.front_runtime[curr_runtime_items][i] =
					user->pkg.front_runtime[HISTORY_ITMES]
							       [i];
			}
			free_uid(user);
		} else {
			user_zero.pkg.sup_cluster_runtime[curr_runtime_items] =
				user_zero.pkg.sup_cluster_runtime[HISTORY_ITMES];
			user_zero.pkg.big_cluster_runtime[curr_runtime_items] =
				user_zero.pkg.big_cluster_runtime[HISTORY_ITMES];
			user_zero.pkg
				.little_cluster_runtime[curr_runtime_items] =
				user_zero.pkg
					.little_cluster_runtime[HISTORY_ITMES];
			for (i = 0; i < MAX_CLUSTER; i++) {
				user_zero.pkg
					.back_runtime[curr_runtime_items][i] =
					user_zero.pkg
						.back_runtime[HISTORY_ITMES][i];
				user_zero.pkg
					.front_runtime[curr_runtime_items][i] =
					user_zero.pkg
						.front_runtime[HISTORY_ITMES][i];
			}
		}

		if (refcount_read(&user->__count) == 1) {
			clear_bit(user->uid.val, package_uid_bit);
			free_uid(user);
		}
	}

	package_runtime_info[curr_runtime_items].update_time = sys_update_time;
	sys_update_time = get_jiffies_64();
	++curr_runtime_items;
	curr_runtime_items %= HISTORY_ITMES;
}

static int __init pkg_init(void)
{
	int i;

	printk(KERN_ERR "in %s\n", __func__);
	create_runtime_proc();
	for (i = 0; i < HISTORY_WINDOWS; i++)
		package_runtime_info[i].update_time = 0;
	init_package_runtime_info(&user_zero);
	INIT_WORK(&package_runtime_roll, package_runtime_roll_wk);
	sys_update_time = get_jiffies_64();
	package_runtime_info[HISTORY_ITMES].update_time = sys_update_time;
	package_runtime_info[HISTORY_ITMES + 1].update_time = sys_update_time;
	package_runtime_info[HISTORY_ITMES + 2].update_time = sys_update_time;
	runtime_enable = 1;
	return 0;
}

static void __exit pkg_exit(void)
{
	printk(KERN_ERR "in %s\n", __func__);
	delete_runtimeproc();
}

static struct user_struct *task_user(struct task_struct *tsk)
{
	struct user_struct *user;

	rcu_read_lock();
	user = task_cred_xxx(tsk, user);
	user = get_uid(user);
	rcu_read_unlock();

	if (likely(user))
		return user;

	return &user_zero;
}

static void update_task_runtime_info(struct task_struct *tsk, u64 delta,
				     int cpu)
{
	enum cluster_type clustertype = get_cluster_type(cpu);
	struct user_struct *user;
	int curr_items = (curr_runtime_items % HISTORY_ITMES);

	if (!runtime_enable || pause_mode)
		return;

	if (is_idle_task(tsk))
		return;

	user = task_user(tsk);
	/* No processes for this user,assign it as user_zero */
	if (!user)
		user = &user_zero;
	switch (user->pkg.edt) {
	case FRONT:
		tsk->pkg.front_runtime[HISTORY_ITMES][clustertype] += delta;
		tsk->pkg.front_runtime[curr_items][clustertype] =
			tsk->pkg.front_runtime[HISTORY_ITMES][clustertype];
		user->pkg.front_runtime[HISTORY_ITMES][clustertype] += delta;

		break;
	case BACK:
		tsk->pkg.back_runtime[HISTORY_ITMES][clustertype] += delta;
		tsk->pkg.back_runtime[curr_items][clustertype] =
			tsk->pkg.back_runtime[HISTORY_ITMES][clustertype];
		user->pkg.back_runtime[HISTORY_ITMES][clustertype] += delta;
		break;
	default:
		break;
	}

	switch (clustertype) {
	case BIG_CLUSTER:
		tsk->pkg.sup_cluster_runtime[HISTORY_ITMES] += delta;
		user->pkg.sup_cluster_runtime[HISTORY_ITMES] += delta;
		break;

	case MID_CLUSTER:
		tsk->pkg.big_cluster_runtime[HISTORY_ITMES] += delta;
		user->pkg.big_cluster_runtime[HISTORY_ITMES] += delta;
		break;

	case LITTLE_CLUSTER:
		tsk->pkg.little_cluster_runtime[HISTORY_ITMES] += delta;
		user->pkg.little_cluster_runtime[HISTORY_ITMES] += delta;
		break;
	default:
		break;
	}

	tsk->pkg.sup_cluster_runtime[curr_items] =
		tsk->pkg.sup_cluster_runtime[HISTORY_ITMES];
	tsk->pkg.big_cluster_runtime[curr_items] =
		tsk->pkg.big_cluster_runtime[HISTORY_ITMES];
	tsk->pkg.little_cluster_runtime[curr_items] =
		tsk->pkg.little_cluster_runtime[HISTORY_ITMES];

	if (likely(user != &user_zero))
		free_uid(user);

	migt_hook(tsk, delta, cpu);
}

void update_pkg_load(struct task_struct *tsk, int cpu, int flag, u64 wallclock,
		     u64 delta)
{
	update_task_runtime_info(tsk, delta, cpu);
}
EXPORT_SYMBOL(update_pkg_load);

bool pkg_enable(void)
{
	return !pause_mode;
}
EXPORT_SYMBOL(pkg_enable);

void package_runtime_monitor(u64 now) /*should run in timer interrupt*/
{
	package_runtime_disable = package_runtime_should_stop();
	if (!runtime_enable || pause_mode)
		return;

	if ((now - sys_update_time) < runtime_window_size * HZ)
		return;

	queue_work_on(0, system_long_wq, &package_runtime_roll);
}
EXPORT_SYMBOL(package_runtime_monitor);

void init_task_runtime_info(struct task_struct *tsk)
{
	int i, j;

	if (tsk) {
		for (i = 0; i < HISTORY_WINDOWS; i++) {
			tsk->pkg.sup_cluster_runtime[i] =
				tsk->pkg.little_cluster_runtime[i] =
					tsk->pkg.big_cluster_runtime[i] = 0;
			for (j = 0; j < MAX_CLUSTER; j++) {
				tsk->pkg.front_runtime[i][j] =
					tsk->pkg.back_runtime[i][j] = 0;
			}
		}
#ifdef CONFIG_MILLET
		tsk->pkg.millet_freeze_flag = 0;
#endif
		migt_monitor_init(tsk);
	}
}
EXPORT_SYMBOL(init_task_runtime_info);

void init_package_runtime_info(struct user_struct *user)
{
	int i, j;

	if (!user)
		return;

	for (i = 0; i < HISTORY_WINDOWS; i++) {
		user->pkg.sup_cluster_runtime[i] =
			user->pkg.little_cluster_runtime[i] =
				user->pkg.big_cluster_runtime[i] = 0;
		for (j = 0; j < MAX_CLUSTER; j++) {
			user->pkg.front_runtime[i][j] =
				user->pkg.back_runtime[i][j] = 0;
		}
	}
	user->pkg.edt = BACK;

	if (user->uid.val > TRACE_UID_MAX - 1)
		set_bit(TRACE_UID_MAX - 1, package_uid_bit);
	else
		set_bit(user->uid.val, package_uid_bit);

	if (user->uid.val > uid_max_value)
		uid_max_value = user->uid.val;
	rwlock_init(&user->pkg.lock);
	INIT_LIST_HEAD(&user->pkg.list);
	refcount_inc(&user->__count);
}
EXPORT_SYMBOL(init_package_runtime_info);

struct runtime_info *get_package_runtime_info(void)
{
	return package_runtime_info;
}

module_init(pkg_init);
module_exit(pkg_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("pkg runtime info calc by David");
