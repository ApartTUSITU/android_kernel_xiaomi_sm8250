#define pr_fmt(fmt) "migt-sched: " fmt

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/sched/signal.h>
#include <linux/kthread.h>
#include <linux/sort.h>
#include <linux/cred.h>
#include <linux/pkg_stat.h>
#include <linux/jiffies.h>

#define RENDER_AGING_TIME (3 * HZ)
#define USER_MAXTHREAD (1 << 10)
#define MIGT_THREAD_NAME "migt_thread"
#define BOOST_MAXTIME_LIMIT (60 * HZ)
#define DEFAULT_BOOST_MINTIME 30
#define for_each_user_process(p, user)                                         \
	list_for_each_entry (p, &user->pkg.list, pkg.list)
#define min_value(x, y) (x < y ? x : y)

#define mi_time_after(a, b)                                                    \
	(typecheck(unsigned long, a) && typecheck(unsigned long, b) &&         \
	 ((long)((b) - (a)) < 0))
#define mi_time_before(a, b) mi_time_after(b, a)

int __weak enable_pkg_monitor = 1;
int __weak set_render_as_stask;
int __weak force_stask_to_big;
int __weak stask_candidate_num = 1;
int __weak vip_task_max_num = 15;
int __weak ip_task_max_num = 5;
int __weak vip_task_schedboost;
int __weak need_reset_runtime_everytime = 1;
int __weak force_reset_runtime = 1;
int __weak minor_window_app;
void __weak trigger_ceiling_limit(void)
{
}
int __weak migt_enable(void)
{
	return 0;
}
int __weak vip_task_max_cpu_nr = 7;
int __weak fas_power_mod;

static int boost_task_pid;
static int migt_debug;
static struct ctl_table_header *migt_sched_header;
static DECLARE_WAIT_QUEUE_HEAD(migt_wait_queue);
static atomic_t migt_has_work_todo = ATOMIC_INIT(0);
static atomic_t migt_init_sucess = ATOMIC_INIT(0);
static atomic_t mi_dynamic_vip_num = ATOMIC_INIT(0);
struct task_struct *migt_thread;
//static cpumask_t minor_app_use_cpumask;
static cpumask_t viptask_use_cpumask;

static struct cpumask cluster_affinity_cpumask[CLUSTER_AFFINITY_TYPES];
static struct cpumask cluster_core_mask[CLUSTER_AFFINITY_TYPES];
static int cluster_number;
static int cluster_core_init_suc;
static int cluster_core_init(void);

enum STASK_POLICY {
	CHOOSE_WAKERS,
	CHOOSE_HERAY,
} pick_stask_policy = CHOOSE_HERAY;

static struct rend_info_s {
	seqcount_t seq ____cacheline_aligned_in_smp;
	raw_spinlock_t lock;
	bool active;
	int pid;
	unsigned long last_update_jiffies;
	char comm[TASK_COMM_LEN];
} render_info[RENDER_TYPES];

static struct task_runtime {
	struct task_struct *tsk;
	u64 run_times;
} user_array[USER_MAXTHREAD];
static struct rend_info_s rinfo_back[RENDER_TYPES];

static struct migt_work {
	int uid;
} g_work;

void add_work_to_migt(int data)
{
	if (atomic_read(&migt_has_work_todo) ||
	    !atomic_read(&migt_init_sucess)) {
		if (migt_debug)
			pr_err("%s, %d, %d\n", __func__, __LINE__, data);
		return;
	}

	g_work.uid = data;
	atomic_set(&migt_has_work_todo, 1);
	wake_up_interruptible(&migt_wait_queue);
}

void migt_monitor_hook(int enqueue, int cpu, struct task_struct *p,
		       u64 walltime)
{
	u64 exec_delta;

	if (!enable_pkg_monitor)
		return;

	if (enqueue)
		p->pkg.migt.prev_sum = p->se.sum_exec_runtime;
	else {
		exec_delta = p->se.sum_exec_runtime - p->pkg.migt.prev_sum;
		if (exec_delta > p->pkg.migt.max_exec)
			p->pkg.migt.max_exec = exec_delta;
	}
}

static inline void set_render_flag(struct task_struct *tsk,
				   enum RENDER_TYPE type)
{
	tsk->pkg.migt.flag |= 1 << (GAME_QRENDER_TASK + type);
}

static inline void clean_render_flag(struct task_struct *tsk,
				     enum RENDER_TYPE type)
{
	tsk->pkg.migt.flag &= ~(1 << (GAME_QRENDER_TASK + type));
}

static void get_render_info(struct rend_info_s *rinfo)
{
	int type;
	unsigned int seq;

	for (type = 0; type < RENDER_TYPES; type++) {
		rinfo[type].active = 0;
		do {
			seq = read_seqcount_begin(&render_info[type].seq);

			memcpy(rinfo[type].comm, render_info[type].comm,
			       TASK_COMM_LEN);
			rinfo[type].active = render_info[type].active;

		} while (read_seqcount_retry(&render_info[type].seq, seq));
	}
}

static bool is_render_sibling(struct task_struct *tsk,
			      struct rend_info_s *rinfo)
{
	int type, len;

	for (type = 0; type < RENDER_TYPES; type++) {
		if (!rinfo[type].active)
			continue;

		len = strlen(rinfo[type].comm);
		if (!strncmp(rinfo[type].comm, tsk->comm, len))
			return true;
	}

	return false;
}

static int pick_stask(struct task_runtime *array, int num)
{
	unsigned long long max_exec = 0;
	int i, max = 0;

	for (i = 0; i < num; i++) {
		if (array[i].tsk->pkg.migt.fps_mexec > max_exec) {
			max_exec = array[i].tsk->pkg.migt.fps_mexec;
			max = i;
		}
	}

	return max;
}

static void set_viptask_flag(struct task_struct *p, int pri, int super_id)
{
	if (pri == -1) {
		if (!set_render_as_stask)
			p->pkg.migt.flag |= 1 << GAME_VIP_TASK;
		else
			p->pkg.migt.flag |= 1 << GAME_SUPER_TASK;
		return;
	} else if (pri == super_id) {
		p->pkg.migt.flag |= 1 << GAME_SUPER_TASK;
		if (migt_debug)
			pr_info("set %s pri %d as super\n", p->comm, pri);
		return;
	} else if (pri < stask_candidate_num + vip_task_max_num) {
		if (migt_debug)
			pr_info("set %s pri %d as vip\n", p->comm, pri);
		p->pkg.migt.flag |= 1 << GAME_VIP_TASK;
		return;
	} else if (pri <
		   stask_candidate_num + vip_task_max_num + ip_task_max_num)
		p->pkg.migt.flag |= 1 << GAME_IP_TASK;
}

static void clear_viptask_flag(struct task_struct *p)
{
	if (is_render_thread(p))
		return;

	p->pkg.migt.flag &= MASK_CLE_GTASK;
}

static int cmp(const void *a, const void *b)
{
	struct task_runtime *p1 = (struct task_runtime *)a;
	struct task_runtime *p2 = (struct task_runtime *)b;

	if (p1->run_times > p2->run_times)
		return -EPERM;

	if (p1->run_times < p2->run_times)
		return 1;

	return 0;
}

static void mark_viptask(int uid)
{
	int j, i = 0;
	int super_id = 0;
	struct task_struct *tsk;
	kuid_t kuid;
	struct user_struct *user;

	kuid.val = uid;
	user = find_user(kuid);
	if (!user)
		return;

	get_render_info(rinfo_back);
	read_lock(&user->pkg.lock);
	if (list_empty(&user->pkg.list)) {
		read_unlock(&user->pkg.lock);
		free_uid(user);
		return;
	}

	for_each_user_process(tsk, user)
	{
		if (!tsk)
			break;

		BUG_ON(task_uid(tsk).val != user->uid.val);
		if (need_reset_runtime_everytime || force_reset_runtime) {
			get_task_struct(tsk);
			for (j = 0; j < NUM_MIGT_BUCKETS; j++) {
				tsk->pkg.migt.bucket[j] = 0;
				tsk->pkg.migt.fps_mexec = 0;
#ifdef VTASK_BOOST_DEBUG
				tsk->pkg.migt.boostat[j] = 0;
#endif
			}
			j = 0;
			put_task_struct(tsk);
		}

		if (is_render_thread(tsk) ||
		    is_render_sibling(tsk, rinfo_back)) {
			set_viptask_flag(tsk, -1, 0);
			continue;
		}

		if (!tsk->pkg.migt.wake_render) {
			clear_viptask_flag(tsk);
			continue;
		}

		get_task_struct(tsk);
		user_array[i].tsk = tsk;
		user_array[i].run_times = tsk->pkg.migt.wake_render;
		if (++i >= USER_MAXTHREAD) {
			pr_err("too many VIP task\n");
			i--;
			break;
		}
	}

	read_unlock(&user->pkg.lock);
	force_reset_runtime = 0;

	if (i > 0) {
		sort(user_array, i, sizeof(struct task_runtime), cmp, NULL);

		if (pick_stask_policy == CHOOSE_HERAY)
			super_id = pick_stask(
				user_array, min_value(stask_candidate_num, i));

		for (j = 0; j < i; j++) {
			if (!user_array[j].tsk) {
				pr_err("%d: task is NULL\n", j);
				continue;
			}

			if (migt_debug && j < 20)
				pr_err("%3d:comm %16s pid %6d key:%llu load %llu sup:%d\n",
				       j, user_array[j].tsk->comm,
				       user_array[j].tsk->pid,
				       user_array[j].run_times,
				       user_array[j].tsk->pkg.migt.fps_mexec,
				       super_id);

			clear_viptask_flag(user_array[j].tsk);
			set_viptask_flag(user_array[j].tsk, j, super_id);
			put_task_struct(user_array[j].tsk);
			user_array[j].tsk = NULL;
			user_array[j].run_times = 0;
		}
	}

	free_uid(user);
}

void migt_monitor_init(struct task_struct *p)
{
	int i;

	p->pkg.migt.migt_count = 0;
	p->pkg.migt.prev_sum = 0;
	p->pkg.migt.max_exec = 0;
	p->pkg.migt.fps_exec = 0;
	p->pkg.migt.fps_mexec = 0;
	p->pkg.migt.flag = MIGT_NORMAL_TASK;
	p->pkg.migt.run_times = 0;
	p->pkg.migt.wake_render = 0;
	p->pkg.migt.boost_end = 0;
	cpumask_copy(&p->pkg.migt.cpus_allowed, cpu_possible_mask);
	for (i = 0; i < NUM_MIGT_BUCKETS; i++) {
		p->pkg.migt.bucket[i] = 0;
#ifdef VTASK_BOOST_DEBUG
		p->pkg.migt.boostat[i] = 0;
#endif
	}
}

static int migt_hardwork(void *data)
{
	struct migt_work *work = (struct migt_work *)data;

	while (1) {
		/*wait data*/
		wait_event_interruptible(migt_wait_queue,
					 atomic_read(&migt_has_work_todo));

		if (work->uid) {
			trigger_ceiling_limit();
			mark_viptask(work->uid);
			work->uid = 0;
		}
		atomic_set(&migt_has_work_todo, 0);
	}
	return 0;
}

static inline void set_mi_vip_task(struct task_struct *p, unsigned int jiff)
{
	if (p) {
		p->pkg.migt.boost_end = jiffies + jiff;
		if (!(p->pkg.migt.flag & MASK_MI_VTASK))
			atomic_inc(&mi_dynamic_vip_num);

		p->pkg.migt.flag |= MASK_MI_VTASK;
	}
}

static inline void clean_mi_vip_task(struct task_struct *p)
{
	unsigned long boost_end = p->pkg.migt.boost_end;

	if (mi_time_after(jiffies, boost_end)) {
		if (migt_debug)
			pr_info("clean vip flag %d, %s, time %lu %lu to %lu\n",
				p->pid, p->comm, jiffies, boost_end,
				p->pkg.migt.boost_end);
		atomic_dec(&mi_dynamic_vip_num);
		p->pkg.migt.flag &= (~MASK_MI_VTASK);
	}
}

static int proc_boost_mi_task(struct ctl_table *table, int write,
			      void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret = proc_dointvec(table, write, buffer, lenp, ppos);
	struct task_struct *target;

	rcu_read_lock();
	target = find_task_by_vpid(boost_task_pid);
	if (unlikely(!target)) {
		rcu_read_unlock();
		pr_err("Invalid input %d, no such process\n", boost_task_pid);
		return 0;
	}

	get_task_struct(target);
	pr_info("%d, %s set as mi vip task from %u", boost_task_pid,
		target->comm, jiffies);
	rcu_read_unlock();
	set_mi_vip_task(target, HZ);
	put_task_struct(target);
	boost_task_pid = -1;
	return ret;
}

void mi_vip_task_req(int *pid, unsigned int nr, unsigned int jiff)
{
	int i, task_pid;
	struct task_struct *target;

#define MAX_MI_VIP_REQ 5
	if (nr > MAX_MI_VIP_REQ) {
		pr_err("req too many vip tasks\n");
		return;
	}

	if (!pid)
		return;

	jiff = min_value(BOOST_MAXTIME_LIMIT, jiff);
	if (unlikely(!jiff))
		jiff = DEFAULT_BOOST_MINTIME;

	for (i = 0; i < nr; i++) {
		rcu_read_lock();
		task_pid = pid[i];
		target = find_task_by_vpid(task_pid);

		if (unlikely(!target)) {
			rcu_read_unlock();
			pr_err("Invalid input %d, no such process\n", task_pid);
			continue;
		}

		get_task_struct(target);
		pr_info("%d, %s set as mi vip task from %u to %u", task_pid,
			target->comm, jiffies, jiffies + jiff);
		rcu_read_unlock();
		set_mi_vip_task(target, jiff);
		put_task_struct(target);
	}
}

int get_mi_dynamic_vip_num(void)
{
	return atomic_read(&mi_dynamic_vip_num);
}

int proc_viptask_cpus_set(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int cpu, ret = proc_dointvec(table, write, buffer, lenp, ppos);

	cpumask_clear(&viptask_use_cpumask);
	for_each_possible_cpu (cpu) {
		if (cpu >= vip_task_max_cpu_nr)
			break;

		cpumask_set_cpu(cpu, &viptask_use_cpumask);
		pr_info("vip task use max cpu nr %d\n", cpu);
	}

	return ret;
}

static struct ctl_table migt_table[] = {
	{
		.procname = "boost_pid",
		.data = &boost_task_pid,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_boost_mi_task,
	},
	{
		.procname = "viptask_maxcpu_nr",
		.data = &vip_task_max_cpu_nr,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_viptask_cpus_set,
	},
	{
		.procname = "enable_pkg_monitor",
		.data = &enable_pkg_monitor,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{
		.procname = "migt_sched_debug",
		.data = &migt_debug,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{
		.procname = "render_stask",
		.data = &set_render_as_stask,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{
		.procname = "force_stask_tob",
		.data = &force_stask_to_big,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{
		.procname = "stask_candidate_num",
		.data = &stask_candidate_num,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{
		.procname = "vip_task_max_num",
		.data = &vip_task_max_num,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{
		.procname = "ip_task_max_num",
		.data = &ip_task_max_num,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{}
};

static struct ctl_table migt_ctl_root[] = { {
						    .procname = "migt",
						    .mode = 0555,
						    .child = migt_table,
					    },
					    {} };

static int __init migt_sched_init(void)
{
	int type, ret = -1;
	int cpu = 0;

	/*run thread*/
	migt_thread = kthread_run(migt_hardwork, &g_work, MIGT_THREAD_NAME);
	if (IS_ERR_OR_NULL(migt_thread)) {
		pr_err("migt_thread %s: error create thread!\n", __func__);
		return ret;
	}

	pr_info("migt_sched %s: inited!\n", __func__);
	atomic_set(&migt_init_sucess, 1);
	WARN_ON(migt_sched_header);
	migt_sched_header = register_sysctl_table(migt_ctl_root);
	cluster_core_init();

	for (type = 0; type < RENDER_TYPES; type++) {
		seqcount_init(&render_info[type].seq);
		raw_spin_lock_init(&render_info[type].lock);
		reset_render_info(type);
	}

	for_each_possible_cpu (cpu) {
		if (cpu > vip_task_max_cpu_nr)
			continue;
		else
			cpumask_set_cpu(cpu, &viptask_use_cpumask);
	}

	return 0;
}

static void __exit migt_sched_exit(void)
{
	if (!IS_ERR_OR_NULL(migt_thread))
		kthread_stop(migt_thread);

	unregister_sysctl_table(migt_sched_header);
	migt_sched_header = NULL;
}

int game_task(struct task_struct *p)
{
	if (!migt_enable())
		return 0;

	return (from_kuid(&init_user_ns, task_uid(p)) == g_work.uid);
}

int game_ip_task(struct task_struct *p)
{
	if (!migt_enable())
		return 0;

	return (p->pkg.migt.flag & MASK_ITASK);
}

int __game_vip_task(struct task_struct *p)
{
	if (!migt_enable())
		return 0;

	return (p->pkg.migt.flag & MASK_VTASK);
}

int game_super_task(struct task_struct *p)
{
	if (!migt_enable())
		return 0;

	if (vip_task_schedboost || force_stask_to_big)
		return (p->pkg.migt.flag & MASK_STASK);

	return 0;
}

int game_vip_task(struct task_struct *p)
{
	unsigned long boost_end = p->pkg.migt.boost_end;

	if (!migt_enable()) {
		if (mi_time_before(jiffies, boost_end)) {
			if (migt_debug)
				pr_info("%d %s is mi vip task %d\n", p->pid,
					p->comm,
					p->pkg.migt.flag & MASK_MI_VTASK);

			return p->pkg.migt.flag & MASK_MI_VTASK;
		}

		else if (p->pkg.migt.flag & MASK_MI_VTASK)
			clean_mi_vip_task(p);
	}

	return __game_vip_task(p);
}

static int is_minor_window_app(int uid)
{
	if (!minor_window_app)
		return 0;

	return uid == minor_window_app;
}

int minor_window_task(struct task_struct *p)
{
	int uid = from_kuid(&init_user_ns, task_uid(p));

	return is_minor_window_app(uid) || mi_uid_type(uid);
}

bool get_minor_window_cpumask(struct task_struct *p, cpumask_t *cpumask)
{
	int uid = from_kuid(&init_user_ns, task_uid(p));
	enum CLUSTER_AFFINITY clus_affinity;

	if (unlikely(!cpumask || !cluster_core_init_suc))
		return false;

	if (is_minor_window_app(uid)) {
		cpumask_copy(cpumask, cpu_possible_mask);
		return true;
	}

	clus_affinity = mi_uid_type(uid);
	if (!clus_affinity)
		return false;

	cpumask_copy(cpumask, &cluster_affinity_cpumask[clus_affinity]);

	return true;
}

static int cluster_core_init(void)
{
	struct cpumask cpus = *cpu_possible_mask;
	struct cpumask tcpus;
	enum CLUSTER_AFFINITY clus_affinity = CAFFINITY_LITTLE_LIST;
	const struct cpumask *cluster_cpus;
	int i, cpu, cluster = 0;

	pr_info("come into %s\n", __func__);
	memset(&tcpus, 0, sizeof(struct cpumask));

	for_each_cpu (i, &cpus) {
		if (cluster >= MAX_CLUSTER)
			break;
		cluster_cpus = cpu_coregroup_mask(i);
		if (i != cpumask_first(cluster_cpus))
			continue;
		cpumask_copy(&cluster_core_mask[cluster], cluster_cpus);
		cpumask_or(&tcpus, &tcpus, cluster_cpus);
		cpumask_andnot(&cpus, &cpus, cluster_cpus);
		if (clus_affinity < CLUSTER_AFFINITY_TYPES) {
			pr_info("clus_affnity %d ", clus_affinity);
			cpumask_copy(&cluster_affinity_cpumask[clus_affinity],
				     &tcpus);
			for_each_cpu (cpu,
				      &cluster_affinity_cpumask[clus_affinity])
				pr_info("-%d \t", cpu);

			pr_info("\n");
		}
		clus_affinity++;
		cluster++;
	}

	cpumask_andnot(&cluster_affinity_cpumask[CAFFINITY_BIG_LIST],
		       &cluster_affinity_cpumask[CAFFINITY_BIG_LIST],
		       &cluster_affinity_cpumask[CAFFINITY_LITTLE_LIST]);

	cluster_number = cluster;
	cluster_core_init_suc = 1;
	return 0;
}

void reset_render_info(enum RENDER_TYPE type)
{
	int pid;
	struct task_struct *tsk;

	raw_spin_lock(&render_info[type].lock);
	write_seqcount_begin(&render_info[type].seq);
	render_info[type].active = false;
	render_info[type].last_update_jiffies = 0;
	pid = render_info[type].pid;
	render_info[type].pid = -1;
	memset(render_info[type].comm, 0, TASK_COMM_LEN);
	write_seqcount_end(&render_info[type].seq);
	raw_spin_unlock(&render_info[type].lock);

	if (pid > 0) {
		rcu_read_lock();
		tsk = find_task_by_vpid(pid);
		if (tsk)
			clean_render_flag(tsk, type);
		rcu_read_unlock();
	}
}

void update_render_info(struct task_struct *tsk, enum RENDER_TYPE type)
{
	int len, pid;
	struct task_struct *old_render;

	raw_spin_lock(&render_info[type].lock);
	write_seqcount_begin(&render_info[type].seq);
	if (likely(render_info[type].pid == tsk->pid)) {
		write_seqcount_end(&render_info[type].seq);
		raw_spin_unlock(&render_info[type].lock);
		return;
	}

	render_info[type].active = true;
	set_render_flag(tsk, type);
	render_info[type].active = 1;
	render_info[type].last_update_jiffies = jiffies;
	pid = render_info[type].pid;
	render_info[type].pid = tsk->pid;
	memset(render_info[type].comm, 0, TASK_COMM_LEN);
	len = strlen(tsk->comm);
	strscpy(render_info[type].comm, tsk->comm, len);
	write_seqcount_end(&render_info[type].seq);
	raw_spin_unlock(&render_info[type].lock);

	if (pid > 0) {
		rcu_read_lock();
		old_render = find_task_by_vpid(pid);
		if (old_render)
			clean_render_flag(old_render, type);
		rcu_read_unlock();
	}
}

int is_render_thread(struct task_struct *tsk)
{
	return tsk->pkg.migt.flag & MASK_RTASK;
}

int fas_power_bias(struct task_struct *tsk)
{
	if (!migt_enable())
		return 0;

	if (game_task(tsk))
		return 0;

	return !!fas_power_mod;
}

module_init(migt_sched_init);
module_exit(migt_sched_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("vip-task detected by David");
