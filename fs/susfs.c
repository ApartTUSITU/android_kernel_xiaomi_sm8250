#include <linux/version.h>
#include <linux/cred.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/printk.h>
#include <linux/namei.h>
#include <linux/list.h>
#include <linux/init_task.h>
#include <linux/spinlock.h>
#include <linux/seqlock.h>
#include <linux/stat.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/fdtable.h>
#include <linux/statfs.h>
#include <linux/random.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/fsnotify_backend.h>
#include <linux/version.h> // We need check kernel version.
#include <linux/susfs.h>
#include "fuse/fuse_i.h"
#include "mount.h"

extern bool susfs_is_current_ksu_domain(void);

#ifdef CONFIG_KSU_SUSFS_ENABLE_LOG
bool susfs_is_log_enabled __read_mostly = true;
#define SUSFS_LOGI(fmt, ...) if (READ_ONCE(susfs_is_log_enabled)) pr_info("susfs:[%u][%d][%s] " fmt, current_uid().val, current->pid, __func__, ##__VA_ARGS__)
#define SUSFS_LOGE(fmt, ...) if (READ_ONCE(susfs_is_log_enabled)) pr_err("susfs:[%u][%d][%s]" fmt, current_uid().val, current->pid, __func__, ##__VA_ARGS__)
#else
#define SUSFS_LOGI(fmt, ...) 
#define SUSFS_LOGE(fmt, ...) 
#endif

bool susfs_starts_with(const char *str, const char *prefix) {
    while (*prefix) {
        if (*str++ != *prefix++)
            return false;
    }
    return true;
}

#ifndef FUSE_SUPER_MAGIC
#define FUSE_SUPER_MAGIC 0x65735546
#endif

/* sus_path */
#ifdef CONFIG_KSU_SUSFS_SUS_PATH
DEFINE_STATIC_SRCU(susfs_srcu_sus_path_loop);
static DEFINE_SPINLOCK(susfs_spin_lock_sus_path);
static LIST_HEAD(LH_SUS_PATH_LOOP);
const struct qstr susfs_fake_qstr_name = QSTR_INIT("..5.u.S", 7); // used to re-test the dcache lookup, make sure you don't have file named like this!!

void susfs_add_sus_path(void __user **user_info) {
	struct st_susfs_sus_path info = {0};
	struct path path;
	struct inode *inode = NULL;
	struct fuse_inode *fi = NULL;

	if (copy_from_user(&info, (struct st_susfs_sus_path __user*)*user_info, sizeof(info))) {
		info.err = -EFAULT;
		goto out_copy_to_user;
	}

	info.err = kern_path(info.target_pathname, LOOKUP_FOLLOW, &path);
	if (info.err) {
		SUSFS_LOGE("failed opening file '%s'\n", info.target_pathname);
		goto out_copy_to_user;
	}

	inode = d_backing_inode(path.dentry);
	if (!inode || !inode->i_mapping) {
		SUSFS_LOGE("inode || inode->i_mapping is NULL\n");
		info.err = -ENOENT;
		goto out_path_put_path;
	}

	if (inode->i_sb->s_magic == FUSE_SUPER_MAGIC) {
		fi = get_fuse_inode(inode);
		if (!fi || !fi->inode.i_mapping) {
			SUSFS_LOGE("fi || fi->inode.i_mapping is NULL\n");
			info.err = -ENOENT;
			goto out_path_put_path;
		}
		set_bit(AS_FLAGS_SUS_PATH, &fi->inode.i_mapping->flags);
		SUSFS_LOGI("flagged AS_FLAGS_SUS_PATH on pathname: '%s', fi->nodeid: %llu, fi->inode.i_ino: %lu, fi->inode.i_mapping->flags: 0x%lx\n", 
					info.target_pathname, fi->nodeid, fi->inode.i_ino, fi->inode.i_mapping->flags);
		info.err = 0;
		goto out_path_put_path;
	}

	set_bit(AS_FLAGS_SUS_PATH, &inode->i_mapping->flags);
	SUSFS_LOGI("flagged AS_FLAGS_SUS_PATH on pathname: '%s', ino: '%lu', inode->i_mapping->flags: 0x%lx\n",
				info.target_pathname, inode->i_ino, inode->i_mapping->flags);
	info.err = 0;
out_path_put_path:
	path_put(&path);
out_copy_to_user:
	if (copy_to_user(&((struct st_susfs_sus_path __user*)*user_info)->err, &info.err, sizeof(info.err))) {
		info.err = -EFAULT;
	}
	SUSFS_LOGI("CMD_SUSFS_ADD_SUS_PATH -> ret: %d\n", info.err);
}

void susfs_add_sus_path_loop(void __user **user_info) {
	struct st_susfs_sus_path_list *new_list = NULL;
	struct st_susfs_sus_path info = {0};

	if (copy_from_user(&info, (struct st_susfs_sus_path __user*)*user_info, sizeof(info))) {
		info.err = -EFAULT;
		goto out_copy_to_user;
	}

	if (*info.target_pathname == '\0') {
		SUSFS_LOGE("target_pathname cannot be empty\n");
		info.err = -EINVAL;
		goto out_copy_to_user;
	}

	new_list = kmalloc(sizeof(struct st_susfs_sus_path_list), GFP_KERNEL);
	if (!new_list) {
		info.err = -ENOMEM;
		goto out_copy_to_user;
	}
	strncpy(new_list->info.target_pathname, info.target_pathname, SUSFS_MAX_LEN_PATHNAME - 1);
	strncpy(new_list->target_pathname, info.target_pathname, SUSFS_MAX_LEN_PATHNAME - 1);
	INIT_LIST_HEAD(&new_list->list);
	spin_lock(&susfs_spin_lock_sus_path);
	list_add_tail_rcu(&new_list->list, &LH_SUS_PATH_LOOP);
	spin_unlock(&susfs_spin_lock_sus_path);
	SUSFS_LOGI("target_pathname: '%s', is successfully added to LH_SUS_PATH_LOOP\n", new_list->target_pathname);
	info.err = 0;
out_copy_to_user:
	if (copy_to_user(&((struct st_susfs_sus_path __user*)*user_info)->err, &info.err, sizeof(info.err))) {
		info.err = -EFAULT;
	}
	SUSFS_LOGI("CMD_SUSFS_ADD_SUS_PATH_LOOP -> ret: %d\n", info.err);
}

void susfs_run_sus_path_loop(void) {
	struct st_susfs_sus_path_list *cursor = NULL;
	struct path path;
	struct inode *inode;
	struct fuse_inode *fi = NULL;
	int srcu_idx = srcu_read_lock(&susfs_srcu_sus_path_loop);

	list_for_each_entry_rcu(cursor, &LH_SUS_PATH_LOOP, list) {
		if (!kern_path(cursor->target_pathname, 0, &path))
		{
			inode = d_backing_inode(path.dentry);
			if (!inode || !inode->i_mapping) {
				SUSFS_LOGE("inode || inode->i_mapping is NULL\n");
				path_put(&path);
				continue;
			}
			if (inode->i_sb->s_magic == FUSE_SUPER_MAGIC) {
				fi = get_fuse_inode(inode);
				if (!fi || !fi->inode.i_mapping) {
					SUSFS_LOGE("fi || fi->inode.i_mapping is NULL\n");
					path_put(&path);
					continue;
				}
				set_bit(AS_FLAGS_SUS_PATH, &fi->inode.i_mapping->flags);
				SUSFS_LOGI("re-flag AS_FLAGS_SUS_PATH on path '%s', fi->inode.i_ino: '%lu', fi->inode.i_mapping->flags: 0x%lx\n",
						   cursor->target_pathname, fi->inode.i_ino, fi->inode.i_mapping->flags);
			} else {
				set_bit(AS_FLAGS_SUS_PATH, &inode->i_mapping->flags);
				SUSFS_LOGI("re-flag AS_FLAGS_SUS_PATH on path '%s', inode->i_ino: '%lu', inode->i_mapping->flags: 0x%lx\n",
						cursor->target_pathname, inode->i_ino, inode->i_mapping->flags);
			}
			path_put(&path);
		}
	}
	srcu_read_unlock(&susfs_srcu_sus_path_loop, srcu_idx);
}

static inline bool is_i_uid_not_allowed(uid_t i_uid) {
	return likely(current_uid().val != i_uid);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
bool susfs_is_inode_sus_path(struct mnt_idmap* idmap, struct inode *inode)
#else
bool susfs_is_inode_sus_path(struct inode *inode)
#endif
{
	struct fuse_inode *fi = NULL;
	if (!susfs_is_current_proc_umounted_app()) {
		return false;
	}
	if (!inode->i_mapping) {
		SUSFS_LOGE("inode->i_mapping is NULL\n");
		return false;
	}
	if (inode->i_sb->s_magic == FUSE_SUPER_MAGIC) {
		fi = get_fuse_inode(inode);
		if (!fi || !fi->inode.i_mapping) {
			SUSFS_LOGE("fi || fi->inode.i_mapping is NULL\n");
			return false;
		}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
		if (unlikely(test_bit(AS_FLAGS_SUS_PATH, &fi->inode.i_mapping->flags) &&
			is_i_uid_not_allowed(i_uid_into_vfsuid(idmap, &fi->inode).val)))
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
		if (unlikely(test_bit(AS_FLAGS_SUS_PATH, &fi->inode.i_mapping->flags) &&
			is_i_uid_not_allowed(i_uid_into_mnt(i_user_ns(&fi->inode), &fi->inode).val)))
#else
		if (unlikely(test_bit(AS_FLAGS_SUS_PATH, &fi->inode.i_mapping->flags) &&
			is_i_uid_not_allowed(fi->inode.i_uid.val)))
#endif
		{
			SUSFS_LOGI("hiding path with ino '%lu'\n", inode->i_ino);
			return true;
		}
		return false;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	if (unlikely(test_bit(AS_FLAGS_SUS_PATH, &inode->i_mapping->flags) &&
		is_i_uid_not_allowed(i_uid_into_vfsuid(idmap, inode).val)))
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	if (unlikely(test_bit(AS_FLAGS_SUS_PATH, &inode->i_mapping->flags) &&
		is_i_uid_not_allowed(i_uid_into_mnt(i_user_ns(inode), inode).val)))
#else
	if (unlikely(test_bit(AS_FLAGS_SUS_PATH, &inode->i_mapping->flags) &&
		is_i_uid_not_allowed(inode->i_uid.val)))
#endif
	{
		SUSFS_LOGI("hiding path with ino '%lu'\n", inode->i_ino);
		return true;
	}
	return false;
}
#endif // #ifdef CONFIG_KSU_SUSFS_SUS_PATH

/* sus_mount */
#ifdef CONFIG_KSU_SUSFS_SUS_MOUNT
// - Default to false now so zygisk can pick up the sus mounts without the need to turn it off manually in post-fs-data stage
//   otherwise user needs to turn it on in post-fs-data stage and turn it off in boot-completed stage
bool susfs_hide_sus_mnts_for_non_su_procs = false;

void susfs_set_hide_sus_mnts_for_non_su_procs(void __user **user_info) {
	struct st_susfs_hide_sus_mnts_for_non_su_procs info = {0};

	if (copy_from_user(&info, (struct st_susfs_hide_sus_mnts_for_non_su_procs __user*)*user_info, sizeof(info))) {
		info.err = -EFAULT;
		goto out_copy_to_user;
	}

	WRITE_ONCE(susfs_hide_sus_mnts_for_non_su_procs, info.enabled);
	SUSFS_LOGI("susfs_hide_sus_mnts_for_non_su_procs: %d\n", info.enabled);
	info.err = 0;
out_copy_to_user:
	if (copy_to_user(&((struct st_susfs_hide_sus_mnts_for_non_su_procs __user*)*user_info)->err, &info.err, sizeof(info.err))) {
		info.err = -EFAULT;
	}
	SUSFS_LOGI("CMD_SUSFS_HIDE_SUS_MNTS_FOR_NON_SU_PROCS -> ret: %d\n", info.err);
}
#endif // #ifdef CONFIG_KSU_SUSFS_SUS_MOUNT

/* sus_kstat */
#ifdef CONFIG_KSU_SUSFS_SUS_KSTAT
static DEFINE_SPINLOCK(susfs_spin_lock_sus_kstat);
static DEFINE_HASHTABLE(SUS_KSTAT_HLIST, 10);
static int susfs_mark_inode_sus_kstat(char *target_pathname, struct st_susfs_sus_kstat_hlist *new_entry) {
	struct path path;
	struct inode *inode = NULL;
	struct fuse_inode *fi = NULL;
	int err = 0;

	err = kern_path(target_pathname, 0, &path);
	if (err) {
		SUSFS_LOGE("failed opening file '%s'\n", target_pathname);
		return err;
	}

	inode = d_backing_inode(path.dentry);
	if (!inode || !inode->i_mapping) {
		SUSFS_LOGE("inode || inode->i_mapping is NULL\n");
		err = -ENOENT;
		goto out_path_put_path;
	}

	if (inode->i_sb->s_magic == FUSE_SUPER_MAGIC) {
		fi = get_fuse_inode(inode);
		if (!fi || !fi->inode.i_mapping) {
			SUSFS_LOGE("fi || fi->inode.i_mapping is NULL\n");
			err = -ENOENT;
			goto out_path_put_path;
		}
		set_bit(AS_FLAGS_SUS_KSTAT, &fi->inode.i_mapping->flags);
		new_entry->is_fuse = true;
		new_entry->target_dev = fi->inode.i_sb->s_dev;
		SUSFS_LOGI("flagged AS_FLAGS_SUS_KSTAT on pathname: '%s', is_fuse: %d, fi->inode.i_sb->s_dev: %u, fi->nodeid: %llu, fi->inode.i_ino: %lu, fi->inode.i_mapping->flags: 0x%lx\n",
					target_pathname, new_entry->is_fuse, fi->inode.i_sb->s_dev, fi->nodeid, fi->inode.i_ino, fi->inode.i_mapping->flags);
		err = 0;
		goto out_path_put_path;
	}

	set_bit(AS_FLAGS_SUS_KSTAT, &inode->i_mapping->flags);
	new_entry->is_fuse = false;
	new_entry->target_dev = inode->i_sb->s_dev;
	SUSFS_LOGI("flagged AS_FLAGS_SUS_KSTAT on pathname: '%s', is_fuse: %d, inode->i_sb->s_dev: %u,  inode->i_ino: %lu, inode->i_mapping->flags: 0x%lx\n",
				target_pathname, new_entry->is_fuse, inode->i_sb->s_dev, inode->i_ino, inode->i_mapping->flags);

out_path_put_path:
	path_put(&path);
	return 0;
}

void susfs_add_sus_kstat(void __user **user_info) {
	struct st_susfs_sus_kstat info = {0};
	struct st_susfs_sus_kstat_hlist *new_entry, *tmp_entry;

	if (copy_from_user(&info, (struct st_susfs_sus_kstat __user*)*user_info, sizeof(info))) {
		info.err = -EFAULT;
		goto out_copy_to_user;
	}

	if (*info.target_pathname == '\0') {
		info.err = -EINVAL;
		goto out_copy_to_user;
	}

	new_entry = kmalloc(sizeof(struct st_susfs_sus_kstat_hlist), GFP_KERNEL);
	if (!new_entry) {
		info.err = -ENOMEM;
		goto out_copy_to_user;
	}

	// If it is added statically, check for duplicated entry, and remove it first if so
	if (info.is_statically) {
		spin_lock(&susfs_spin_lock_sus_kstat);
		hash_for_each_possible(SUS_KSTAT_HLIST, tmp_entry, node, info.target_ino) {
			if (!strcmp(tmp_entry->info.target_pathname, info.target_pathname)) {
				memcpy(&new_entry->info, &tmp_entry->info, sizeof(tmp_entry->info));
				new_entry->target_ino = info.target_ino;
				new_entry->info.target_ino = info.target_ino;
				hash_del_rcu(&tmp_entry->node);
				spin_unlock(&susfs_spin_lock_sus_kstat);
				synchronize_rcu();
				kfree(tmp_entry);
				goto out_add_new_entry;
			}
		}
		spin_unlock(&susfs_spin_lock_sus_kstat);
	}

out_add_new_entry:
#if defined(__ARCH_WANT_STAT64) || defined(__ARCH_WANT_COMPAT_STAT64)
#ifdef CONFIG_MIPS
	info.spoofed_dev = new_decode_dev(info.spoofed_dev);
#else
	info.spoofed_dev = huge_decode_dev(info.spoofed_dev);
#endif /* CONFIG_MIPS */
#else
	info.spoofed_dev = old_decode_dev(info.spoofed_dev);
#endif /* defined(__ARCH_WANT_STAT64) || defined(__ARCH_WANT_COMPAT_STAT64) */

	new_entry->target_ino = info.target_ino;
	memcpy(&new_entry->info, &info, sizeof(info));

	info.err = susfs_mark_inode_sus_kstat(new_entry->info.target_pathname, new_entry);
	if (info.err) {
		kfree(new_entry);
		goto out_copy_to_user;
	}

	spin_lock(&susfs_spin_lock_sus_kstat);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	SUSFS_LOGI("is_fuse: %d, is_statically: '%d', target_ino: '%lu', target_pathname: '%s', spoofed_ino: '%lu', spoofed_dev: '%lu', spoofed_nlink: '%u', spoofed_size: '%llu', spoofed_atime_tv_sec: '%ld', spoofed_mtime_tv_sec: '%ld', spoofed_ctime_tv_sec: '%ld', spoofed_atime_tv_nsec: '%ld', spoofed_mtime_tv_nsec: '%ld', spoofed_ctime_tv_nsec: '%ld', spoofed_blksize: '%lu', spoofed_blocks: '%llu', is successfully added to SUS_KSTAT_HLIST\n",
			new_entry->is_fuse,
			new_entry->info.is_statically, new_entry->info.target_ino, new_entry->info.target_pathname,
			new_entry->info.spoofed_ino, new_entry->info.spoofed_dev,
			new_entry->info.spoofed_nlink, new_entry->info.spoofed_size,
			new_entry->info.spoofed_atime_tv_sec, new_entry->info.spoofed_mtime_tv_sec, new_entry->info.spoofed_ctime_tv_sec,
			new_entry->info.spoofed_atime_tv_nsec, new_entry->info.spoofed_mtime_tv_nsec, new_entry->info.spoofed_ctime_tv_nsec,
			new_entry->info.spoofed_blksize, new_entry->info.spoofed_blocks);
#else
	SUSFS_LOGI("is_fuse: %d, is_statically: '%d', target_ino: '%lu', target_pathname: '%s', spoofed_ino: '%lu', spoofed_dev: '%lu', spoofed_nlink: '%u', spoofed_size: '%u', spoofed_atime_tv_sec: '%ld', spoofed_mtime_tv_sec: '%ld', spoofed_ctime_tv_sec: '%ld', spoofed_atime_tv_nsec: '%ld', spoofed_mtime_tv_nsec: '%ld', spoofed_ctime_tv_nsec: '%ld', spoofed_blksize: '%lu', spoofed_blocks: '%llu', is successfully added to SUS_KSTAT_HLIST\n",
			new_entry->is_fuse,
			new_entry->info.is_statically, new_entry->info.target_ino, new_entry->info.target_pathname,
			new_entry->info.spoofed_ino, new_entry->info.spoofed_dev,
			new_entry->info.spoofed_nlink, new_entry->info.spoofed_size,
			new_entry->info.spoofed_atime_tv_sec, new_entry->info.spoofed_mtime_tv_sec, new_entry->info.spoofed_ctime_tv_sec,
			new_entry->info.spoofed_atime_tv_nsec, new_entry->info.spoofed_mtime_tv_nsec, new_entry->info.spoofed_ctime_tv_nsec,
			new_entry->info.spoofed_blksize, new_entry->info.spoofed_blocks);
#endif
	hash_add_rcu(SUS_KSTAT_HLIST, &new_entry->node, info.target_ino);
	spin_unlock(&susfs_spin_lock_sus_kstat);
	info.err = 0;
out_copy_to_user:
	if (copy_to_user(&((struct st_susfs_sus_kstat __user*)*user_info)->err, &info.err, sizeof(info.err))) {
		info.err = -EFAULT;
	}
	if (!info.is_statically) {
		SUSFS_LOGI("CMD_SUSFS_ADD_SUS_KSTAT -> ret: %d\n", info.err);
	} else {
		SUSFS_LOGI("CMD_SUSFS_ADD_SUS_KSTAT_STATICALLY -> ret: %d\n", info.err);
	}
}

void susfs_update_sus_kstat(void __user **user_info) {
	struct st_susfs_sus_kstat info = {0};
	struct st_susfs_sus_kstat_hlist *new_entry, *tmp_entry;

	if (copy_from_user(&info, (struct st_susfs_sus_kstat __user*)*user_info, sizeof(info))) {
		info.err = -EFAULT;
		goto out_copy_to_user;
	}

	new_entry = kmalloc(sizeof(struct st_susfs_sus_kstat_hlist), GFP_KERNEL);
	if (!new_entry) {
		info.err = -ENOMEM;
		goto out_copy_to_user;
	}

	spin_lock(&susfs_spin_lock_sus_kstat);
	hash_for_each_possible(SUS_KSTAT_HLIST, tmp_entry, node, info.target_ino) {
		if (!strcmp(tmp_entry->info.target_pathname, info.target_pathname)) {
			memcpy(&new_entry->info, &tmp_entry->info, sizeof(tmp_entry->info));
			new_entry->target_ino = info.target_ino;
			new_entry->info.target_ino = info.target_ino;
			hash_del_rcu(&tmp_entry->node);
			spin_unlock(&susfs_spin_lock_sus_kstat);
			synchronize_rcu();
			kfree(tmp_entry);
			goto out_add_new_entry;
		}
	}
	spin_unlock(&susfs_spin_lock_sus_kstat);
	info.err = -ENOENT;
	goto out_copy_to_user;

out_add_new_entry:
	info.err = susfs_mark_inode_sus_kstat(new_entry->info.target_pathname, new_entry);
	if (info.err) {
		kfree(new_entry);

	goto out_copy_to_user;
	}
	SUSFS_LOGI("updating target_ino from '%lu' to '%lu' for pathname: '%s' in SUS_KSTAT_HLIST\n",
					new_entry->info.target_ino, info.target_ino, info.target_pathname);
	spin_lock(&susfs_spin_lock_sus_kstat);
	hash_add_rcu(SUS_KSTAT_HLIST, &new_entry->node, info.target_ino);
	spin_unlock(&susfs_spin_lock_sus_kstat);
	info.err = 0;
out_copy_to_user:
	if (copy_to_user(&((struct st_susfs_sus_kstat __user*)*user_info)->err, &info.err, sizeof(info.err))) {
		info.err = -EFAULT;
	}
	SUSFS_LOGI("CMD_SUSFS_UPDATE_SUS_KSTAT -> ret: %d\n", info.err);
}

void susfs_generic_fillattr_spoofer(struct inode *inode, struct kstat *stat)
{
	struct st_susfs_sus_kstat_hlist *entry = NULL;
	struct fuse_inode *fi = NULL;
	unsigned long target_ino = 0;
	dev_t target_dev = 0;
	bool is_fuse = false;

	if (inode->i_sb->s_magic == FUSE_SUPER_MAGIC) {
		fi = get_fuse_inode(inode);
		if (!fi || !fi->inode.i_mapping) {
			SUSFS_LOGE("fi || fi->inode.i_mapping is NULL\n");
			return;
		}
		if (!test_bit(AS_FLAGS_SUS_KSTAT, &fi->inode.i_mapping->flags) ||
			!susfs_is_current_proc_umounted_app())
			return;
		target_ino = fi->inode.i_ino;
		target_dev = fi->inode.i_sb->s_dev;
		is_fuse = true;
		goto out_spoof_kstat;
	}

	if (!inode->i_mapping) {
		SUSFS_LOGE("inode->i_mapping is NULL\n");
		return;
	}

	if (!test_bit(AS_FLAGS_SUS_KSTAT, &inode->i_mapping->flags) ||
	    !susfs_is_current_proc_umounted_app())
		return;

	target_ino = inode->i_ino;
	target_dev = inode->i_sb->s_dev;

out_spoof_kstat:
	rcu_read_lock();
	hash_for_each_possible_rcu(SUS_KSTAT_HLIST, entry, node, target_ino) {
		if (entry->target_ino == target_ino &&
			entry->target_dev == target_dev &&
			entry->is_fuse == is_fuse)
		{
			SUSFS_LOGI("spoofing kstat for path: %s, target_ino: %lu, target_dev: %u\n",
					entry->info.target_pathname, target_ino, target_dev);
			if (entry->info.flags & KSTAT_SPOOF_INO)
				stat->ino = entry->info.spoofed_ino;
			if (entry->info.flags & KSTAT_SPOOF_DEV)
				stat->dev = entry->info.spoofed_dev;
			if (entry->info.flags & KSTAT_SPOOF_NLINK)
				stat->nlink = entry->info.spoofed_nlink;
			if (entry->info.flags & KSTAT_SPOOF_SIZE)
				stat->size = entry->info.spoofed_size;
			if (entry->info.flags & KSTAT_SPOOF_ATIME_TV_SEC)
				stat->atime.tv_sec = entry->info.spoofed_atime_tv_sec;
			if (entry->info.flags & KSTAT_SPOOF_ATIME_TV_NSEC)
				stat->atime.tv_nsec = entry->info.spoofed_atime_tv_nsec;
			if (entry->info.flags & KSTAT_SPOOF_MTIME_TV_SEC)
				stat->mtime.tv_sec = entry->info.spoofed_mtime_tv_sec;
			if (entry->info.flags & KSTAT_SPOOF_MTIME_TV_NSEC)
				stat->mtime.tv_nsec = entry->info.spoofed_mtime_tv_nsec;
			if (entry->info.flags & KSTAT_SPOOF_CTIME_TV_SEC)
				stat->ctime.tv_sec = entry->info.spoofed_ctime_tv_sec;
			if (entry->info.flags & KSTAT_SPOOF_CTIME_TV_NSEC)
				stat->ctime.tv_nsec = entry->info.spoofed_ctime_tv_nsec;
			if (entry->info.flags & KSTAT_SPOOF_BLKSIZE)
				stat->blksize = entry->info.spoofed_blksize;
			if (entry->info.flags & KSTAT_SPOOF_BLOCKS)
				stat->blocks = entry->info.spoofed_blocks;
			rcu_read_unlock();
			return;
		}
	}
	rcu_read_unlock();
}

void susfs_show_map_vma_spoofer(struct inode *inode, dev_t *out_dev, unsigned long *out_ino) {
	struct st_susfs_sus_kstat_hlist *entry = NULL;
	struct fuse_inode *fi = NULL;
	unsigned long target_ino = 0;
	dev_t target_dev = 0;
	bool is_fuse = false;

	if (inode->i_sb->s_magic == FUSE_SUPER_MAGIC) {
		fi = get_fuse_inode(inode);
		if (!fi || !fi->inode.i_mapping) {
			SUSFS_LOGE("fi || fi->inode.i_mapping is NULL\n");
			return;
		}
		if (!test_bit(AS_FLAGS_SUS_KSTAT, &fi->inode.i_mapping->flags) ||
			!susfs_is_current_proc_umounted_app())
			return;
		target_ino = fi->inode.i_ino;
		target_dev = fi->inode.i_sb->s_dev;
		is_fuse = true;
		goto out_spoof_kstat;
	}

	if (!inode->i_mapping) {
		SUSFS_LOGE("inode->i_mapping is NULL\n");
		return;
	}

	if (!test_bit(AS_FLAGS_SUS_KSTAT, &inode->i_mapping->flags) ||
		!susfs_is_current_proc_umounted_app())
		return;

	target_ino = inode->i_ino;
	target_dev = inode->i_sb->s_dev;

out_spoof_kstat:
	rcu_read_lock();
	hash_for_each_possible_rcu(SUS_KSTAT_HLIST, entry, node, target_ino) {
		if (entry->target_ino == target_ino &&
			entry->target_dev == target_dev &&
			entry->is_fuse == is_fuse)
		{
			SUSFS_LOGI("spoofing kstat for target_ino: %lu, target_dev: %u\n", target_ino, target_dev);
			*out_dev = entry->info.spoofed_dev;
			*out_ino = entry->info.spoofed_ino;
			rcu_read_unlock();
			return;
		}
	}
	rcu_read_unlock();
}
#endif // #ifdef CONFIG_KSU_SUSFS_SUS_KSTAT

/* spoof_uname */
#ifdef CONFIG_KSU_SUSFS_SPOOF_UNAME
static struct st_susfs_uname my_uname = {0};
static bool is_susfs_uname_set = false;
static DEFINE_SEQLOCK(susfs_uname_seqlock);

void susfs_set_uname(void __user **user_info) {
	struct st_susfs_uname info = {0};

	if (copy_from_user(&info, (struct st_susfs_uname __user*)*user_info, sizeof(info))) {
		info.err = -EFAULT;
		goto out_copy_to_user;
	}

	if (*info.release == '\0' || *info.version == '\0') {
		info.err = -EFAULT;
		goto out_copy_to_user;
	}

	write_seqlock(&susfs_uname_seqlock);
	if (!strcmp(info.release, "default")) {
		strscpy(my_uname.release, utsname()->release, __NEW_UTS_LEN);
	} else {
		strncpy(my_uname.release, info.release, __NEW_UTS_LEN);
	}
	if (!strcmp(info.version, "default")) {
		strscpy(my_uname.version, utsname()->version, __NEW_UTS_LEN);
	} else {
		strncpy(my_uname.version, info.version, __NEW_UTS_LEN);
	}
	is_susfs_uname_set = true;
	write_sequnlock(&susfs_uname_seqlock);
	SUSFS_LOGI("set spoofed release: '%s', version: '%s'\n",
				my_uname.release, my_uname.version);

	info.err = 0;
out_copy_to_user:
	if (copy_to_user(&((struct st_susfs_uname __user*)*user_info)->err, &info.err, sizeof(info.err))) {
		info.err = -EFAULT;
	}
	SUSFS_LOGI("CMD_SUSFS_SET_UNAME -> ret: %d\n", info.err);
}

void susfs_spoof_uname(struct new_utsname* tmp) {
	unsigned seq;

	do {
		seq = read_seqbegin(&susfs_uname_seqlock);
		if (is_susfs_uname_set) {
			strncpy(tmp->release, my_uname.release, __NEW_UTS_LEN);
			strncpy(tmp->version, my_uname.version, __NEW_UTS_LEN);
		}
	} while (read_seqretry(&susfs_uname_seqlock, seq));
}
#endif // #ifdef CONFIG_KSU_SUSFS_SPOOF_UNAME

/* enable_log */
#ifdef CONFIG_KSU_SUSFS_ENABLE_LOG
void susfs_enable_log(void __user **user_info) {
	struct st_susfs_log info = {0};

	if (copy_from_user(&info, (struct st_susfs_log __user*)*user_info, sizeof(info))) {
		info.err = -EFAULT;
		goto out_copy_to_user;
	}

	WRITE_ONCE(susfs_is_log_enabled, info.enabled);

	if (info.enabled) {
		pr_info("susfs: enable logging to kernel");
	} else {
		pr_info("susfs: disable logging to kernel");
	}
	info.err = 0;
out_copy_to_user:
	if (copy_to_user(&((struct st_susfs_log __user*)*user_info)->err, &info.err, sizeof(info.err))) {
		info.err = -EFAULT;
	}
	SUSFS_LOGI("CMD_SUSFS_ENABLE_LOG -> ret: %d\n", info.err);
}
#endif // #ifdef CONFIG_KSU_SUSFS_ENABLE_LOG

/* spoof_cmdline_or_bootconfig */
#ifdef CONFIG_KSU_SUSFS_SPOOF_CMDLINE_OR_BOOTCONFIG
static char *fake_cmdline_or_bootconfig = NULL;
static bool susfs_is_fake_cmdline_or_bootconfig_set = false;
static DEFINE_SEQLOCK(susfs_fake_cmdline_or_bootconfig_seqlock);

void susfs_set_cmdline_or_bootconfig(void __user **user_info) {
	struct st_susfs_spoof_cmdline_or_bootconfig *info = (struct st_susfs_spoof_cmdline_or_bootconfig *)kzalloc(sizeof(struct st_susfs_spoof_cmdline_or_bootconfig), GFP_KERNEL);
	
	if (!info) {
		info->err = -ENOMEM;
		goto out_copy_to_user;
	}

	if (copy_from_user(info, (struct st_susfs_spoof_cmdline_or_bootconfig __user*)*user_info, sizeof(struct st_susfs_spoof_cmdline_or_bootconfig))) {
		info->err = -EFAULT;
		goto out_copy_to_user;
	}

	if (*info->fake_cmdline_or_bootconfig == '\0') {
		info->err = -EINVAL;
		goto out_copy_to_user;
	}

	if (!fake_cmdline_or_bootconfig) {
		fake_cmdline_or_bootconfig = (char *)kzalloc(SUSFS_FAKE_CMDLINE_OR_BOOTCONFIG_SIZE, GFP_KERNEL);
		if (!fake_cmdline_or_bootconfig) {
			info->err = -ENOMEM;
			goto out_copy_to_user;
		}
	}

	write_seqlock(&susfs_fake_cmdline_or_bootconfig_seqlock);
	strncpy(fake_cmdline_or_bootconfig,
			info->fake_cmdline_or_bootconfig,
			SUSFS_FAKE_CMDLINE_OR_BOOTCONFIG_SIZE-1);
	susfs_is_fake_cmdline_or_bootconfig_set = true;
	write_sequnlock(&susfs_fake_cmdline_or_bootconfig_seqlock);

	SUSFS_LOGI("fake_cmdline_or_bootconfig is set\n");
	info->err = 0;

out_copy_to_user:
	if (copy_to_user(&((struct st_susfs_spoof_cmdline_or_bootconfig __user*)*user_info)->err, &info->err, sizeof(info->err))) {
		info->err = -EFAULT;
	}
	SUSFS_LOGI("CMD_SUSFS_SET_CMDLINE_OR_BOOTCONFIG -> ret: %d\n", info->err);
	if (info) {
		kfree(info);
	}
}

int susfs_spoof_cmdline_or_bootconfig(struct seq_file *m) {
	unsigned seq;
	int err = -EINVAL;

	do {
		seq = read_seqbegin(&susfs_fake_cmdline_or_bootconfig_seqlock);
		if (susfs_is_fake_cmdline_or_bootconfig_set) {
			seq_puts(m, fake_cmdline_or_bootconfig);
			err = 0;
		}
	} while (read_seqretry(&susfs_fake_cmdline_or_bootconfig_seqlock, seq));

	return err;
}
#endif

/* open_redirect */
#ifdef CONFIG_KSU_SUSFS_OPEN_REDIRECT
static DEFINE_SPINLOCK(susfs_spin_lock_open_redirect);
static DEFINE_HASHTABLE(OPEN_REDIRECT_HLIST, 10);

static int susfs_update_open_redirect_inode(struct st_susfs_open_redirect_hlist *new_entry) {
	struct path path;
	struct inode *inode;
	int err = 0;

	err = kern_path(new_entry->target_pathname, LOOKUP_FOLLOW, &path);
	if (err) {
		SUSFS_LOGE("failed opening file '%s'\n", new_entry->target_pathname);
		return err;
	}

	inode = d_backing_inode(path.dentry);
	if (!inode || !inode->i_mapping) {
		SUSFS_LOGE("inode || inode->i_mapping is NULL\n");
		err = -ENOENT;
		goto out_path_put_target;
	}

	set_bit(AS_FLAGS_OPEN_REDIRECT, &inode->i_mapping->flags);

out_path_put_target:
	path_put(&path);
	return err;
}

void susfs_add_open_redirect(void __user **user_info) {
	struct st_susfs_open_redirect info = {0};
	struct st_susfs_open_redirect_hlist *new_entry;

	if (copy_from_user(&info, (struct st_susfs_open_redirect __user*)*user_info, sizeof(info))) {
		info.err = -EFAULT;
		goto out_copy_to_user;
	}

	new_entry = kmalloc(sizeof(struct st_susfs_open_redirect_hlist), GFP_KERNEL);
	if (!new_entry) {
		info.err = -ENOMEM;
		goto out_copy_to_user;
	}

	new_entry->target_ino = info.target_ino;
	strncpy(new_entry->target_pathname, info.target_pathname, SUSFS_MAX_LEN_PATHNAME-1);
	strncpy(new_entry->redirected_pathname, info.redirected_pathname, SUSFS_MAX_LEN_PATHNAME-1);
	if (susfs_update_open_redirect_inode(new_entry)) {
		SUSFS_LOGE("failed adding path '%s' to OPEN_REDIRECT_HLIST\n", new_entry->target_pathname);
		kfree(new_entry);
		info.err = -EINVAL;
		goto out_copy_to_user;
	}

	spin_lock(&susfs_spin_lock_open_redirect);
	hash_add(OPEN_REDIRECT_HLIST, &new_entry->node, info.target_ino);
	spin_unlock(&susfs_spin_lock_open_redirect);
	SUSFS_LOGI("target_ino: '%lu', target_pathname: '%s' redirected_pathname: '%s', is successfully added to OPEN_REDIRECT_HLIST\n",
			new_entry->target_ino, new_entry->target_pathname, new_entry->redirected_pathname);
	info.err = 0;
out_copy_to_user:
	if (copy_to_user(&((struct st_susfs_open_redirect __user*)*user_info)->err, &info.err, sizeof(info.err))) {
		info.err = -EFAULT;
	}
	SUSFS_LOGI("CMD_SUSFS_ADD_OPEN_REDIRECT -> ret: %d\n", info.err);
}

struct filename* susfs_get_redirected_path(unsigned long ino) {
	struct st_susfs_open_redirect_hlist *entry;

	hash_for_each_possible(OPEN_REDIRECT_HLIST, entry, node, ino) {
		if (entry->target_ino == ino) {
			SUSFS_LOGI("Redirect for ino: %lu\n", ino);
			return getname_kernel(entry->redirected_pathname);
		}
	}
	return ERR_PTR(-ENOENT);
}
#endif // #ifdef CONFIG_KSU_SUSFS_OPEN_REDIRECT

/* sus_map */
#ifdef CONFIG_KSU_SUSFS_SUS_MAP
void susfs_add_sus_map(void __user **user_info) {
	struct st_susfs_sus_map info = {0};
	struct path path;
	struct inode *inode = NULL;

	if (copy_from_user(&info, (struct st_susfs_sus_map __user*)*user_info, sizeof(info))) {
		info.err = -EFAULT;
		goto out_copy_to_user;
	}

	info.err = kern_path(info.target_pathname, LOOKUP_FOLLOW, &path);
	if (info.err) {
		SUSFS_LOGE("failed opening file '%s'\n", info.target_pathname);
		goto out_copy_to_user;
	}

	inode = d_backing_inode(path.dentry);
	if (!inode || !inode->i_mapping) {
		SUSFS_LOGE("inode || inode->i_mapping is NULL\n");
		info.err = -ENOENT;
		goto out_path_put_path;
	}
	set_bit(AS_FLAGS_SUS_MAP, &inode->i_mapping->flags);
	SUSFS_LOGI("pathname: '%s', is flagged as AS_FLAGS_SUS_MAP\n", info.target_pathname);
	info.err = 0;
out_path_put_path:
	path_put(&path);
out_copy_to_user:
	if (copy_to_user(&((struct st_susfs_sus_map __user*)*user_info)->err, &info.err, sizeof(info.err))) {
		info.err = -EFAULT;
	}
	SUSFS_LOGI("CMD_SUSFS_ADD_SUS_MAP -> ret: %d\n", info.err);
}
#endif // #ifdef CONFIG_KSU_SUSFS_SUS_MAP

/* susfs avc log spoofing */
extern bool susfs_is_avc_log_spoofing_enabled;

void susfs_set_avc_log_spoofing(void __user **user_info) {
	struct st_susfs_avc_log_spoofing info = {0};

	if (copy_from_user(&info, (struct st_susfs_avc_log_spoofing __user*)*user_info, sizeof(info))) {
		info.err = -EFAULT;
		goto out_copy_to_user;
	}

	WRITE_ONCE(susfs_is_avc_log_spoofing_enabled, info.enabled);
	SUSFS_LOGI("susfs_is_avc_log_spoofing_enabled: %d\n", info.enabled);
	info.err = 0;
out_copy_to_user:
	if (copy_to_user(&((struct st_susfs_avc_log_spoofing __user*)*user_info)->err, &info.err, sizeof(info.err))) {
		info.err = -EFAULT;
	}
	SUSFS_LOGI("CMD_SUSFS_ENABLE_AVC_LOG_SPOOFING -> ret: %d\n", info.err);
}

/* get susfs enabled features */
static int copy_config_to_buf(const char *config_string, char *buf_ptr, size_t *copied_size, size_t bufsize) {
	size_t tmp_size = strlen(config_string);

	*copied_size += tmp_size;
	if (*copied_size >= bufsize) {
		SUSFS_LOGE("bufsize is not big enough to hold the string.\n");
		return -EINVAL;
	}
	strncpy(buf_ptr, config_string, tmp_size);
	return 0;
}

void susfs_get_enabled_features(void __user **user_info) {
	struct st_susfs_enabled_features *info = (struct st_susfs_enabled_features *)kzalloc(sizeof(struct st_susfs_enabled_features), GFP_KERNEL);
	char *buf_ptr = NULL;
	size_t copied_size = 0;

	if (!info) {
		info->err = -ENOMEM;
		goto out_copy_to_user;
	}

	if (copy_from_user(info, (struct st_susfs_enabled_features __user*)*user_info, sizeof(struct st_susfs_enabled_features))) {
		info->err = -EFAULT;
		goto out_copy_to_user;
	}

	buf_ptr = info->enabled_features;

#ifdef CONFIG_KSU_SUSFS_SUS_PATH
	info->err = copy_config_to_buf("CONFIG_KSU_SUSFS_SUS_PATH\n", buf_ptr, &copied_size, SUSFS_ENABLED_FEATURES_SIZE);
	if (info->err) goto out_copy_to_user;
	buf_ptr = info->enabled_features + copied_size;
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_MOUNT
	info->err = copy_config_to_buf("CONFIG_KSU_SUSFS_SUS_MOUNT\n", buf_ptr, &copied_size, SUSFS_ENABLED_FEATURES_SIZE);
	if (info->err) goto out_copy_to_user;
	buf_ptr = info->enabled_features + copied_size;
#endif // #ifdef CONFIG_KSU_SUSFS_SUS_MOUNT
#ifdef CONFIG_KSU_SUSFS_SUS_KSTAT
	info->err = copy_config_to_buf("CONFIG_KSU_SUSFS_SUS_KSTAT\n", buf_ptr, &copied_size, SUSFS_ENABLED_FEATURES_SIZE);
	if (info->err) goto out_copy_to_user;
	buf_ptr = info->enabled_features + copied_size;
#endif
#ifdef CONFIG_KSU_SUSFS_SPOOF_UNAME
	info->err = copy_config_to_buf("CONFIG_KSU_SUSFS_SPOOF_UNAME\n", buf_ptr, &copied_size, SUSFS_ENABLED_FEATURES_SIZE);
	if (info->err) goto out_copy_to_user;
	buf_ptr = info->enabled_features + copied_size;
#endif
#ifdef CONFIG_KSU_SUSFS_ENABLE_LOG
	info->err = copy_config_to_buf("CONFIG_KSU_SUSFS_ENABLE_LOG\n", buf_ptr, &copied_size, SUSFS_ENABLED_FEATURES_SIZE);
	if (info->err) goto out_copy_to_user;
	buf_ptr = info->enabled_features + copied_size;
#endif
#ifdef CONFIG_KSU_SUSFS_HIDE_KSU_SUSFS_SYMBOLS
	info->err = copy_config_to_buf("CONFIG_KSU_SUSFS_HIDE_KSU_SUSFS_SYMBOLS\n", buf_ptr, &copied_size, SUSFS_ENABLED_FEATURES_SIZE);
	if (info->err) goto out_copy_to_user;
	buf_ptr = info->enabled_features + copied_size;
#endif
#ifdef CONFIG_KSU_SUSFS_SPOOF_CMDLINE_OR_BOOTCONFIG
	info->err = copy_config_to_buf("CONFIG_KSU_SUSFS_SPOOF_CMDLINE_OR_BOOTCONFIG\n", buf_ptr, &copied_size, SUSFS_ENABLED_FEATURES_SIZE);
	if (info->err) goto out_copy_to_user;
	buf_ptr = info->enabled_features + copied_size;
#endif
#ifdef CONFIG_KSU_SUSFS_OPEN_REDIRECT
	info->err = copy_config_to_buf("CONFIG_KSU_SUSFS_OPEN_REDIRECT\n", buf_ptr, &copied_size, SUSFS_ENABLED_FEATURES_SIZE);
	if (info->err) goto out_copy_to_user;
	buf_ptr = info->enabled_features + copied_size;
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_MAP
	info->err = copy_config_to_buf("CONFIG_KSU_SUSFS_SUS_MAP\n", buf_ptr, &copied_size, SUSFS_ENABLED_FEATURES_SIZE);
	if (info->err) goto out_copy_to_user;
	buf_ptr = info->enabled_features + copied_size;
#endif

	info->err = 0;
out_copy_to_user:
	if (copy_to_user((struct st_susfs_enabled_features __user*)*user_info, info, sizeof(struct st_susfs_enabled_features))) {
		info->err = -EFAULT;
	}
	SUSFS_LOGI("CMD_SUSFS_SHOW_ENABLED_FEATURES -> ret: %d\n", info->err);
	if (info) {
		kfree(info);
	}
}

/* show_variant */
void susfs_show_variant(void __user **user_info) {
	struct st_susfs_variant info = {0};

	if (copy_from_user(&info, (struct st_susfs_variant __user*)*user_info, sizeof(info))) {
		info.err = -EFAULT;
		goto out_copy_to_user;
	}

	strncpy(info.susfs_variant, SUSFS_VARIANT, SUSFS_MAX_VARIANT_BUFSIZE-1);
	info.err = 0;
out_copy_to_user:
	if (copy_to_user((struct st_susfs_variant __user*)*user_info, &info, sizeof(info))) {
		info.err = -EFAULT;
	}
	SUSFS_LOGI("CMD_SUSFS_SHOW_VARIANT -> ret: %d\n", info.err);
}

/* show version */
void susfs_show_version(void __user **user_info) {
	struct st_susfs_version info = {0};

	if (copy_from_user(&info, (struct st_susfs_version __user*)*user_info, sizeof(info))) {
		info.err = -EFAULT;
		goto out_copy_to_user;
	}

	strncpy(info.susfs_version, SUSFS_VERSION, SUSFS_MAX_VERSION_BUFSIZE-1);
	info.err = 0;
out_copy_to_user:
	if (copy_to_user((struct st_susfs_version __user*)*user_info, &info, sizeof(info))) {
		info.err = -EFAULT;
	}
	SUSFS_LOGI("CMD_SUSFS_SHOW_VERSION -> ret: %d\n", info.err);
}

/* kthread for checking if /sdcard/Android is accessible via fsnoitfy */
/* code is straightly borrowed from KernelSU's pkg_observer.c */
#define SDCARD_ANDROID_PATH "/data/media/0/Android"
extern void setup_selinux(const char *domain, struct cred *cred);
extern bool susfs_is_current_ksu_domain(void);
bool susfs_is_sdcard_android_data_decrypted __read_mostly = false;

struct watch_dir {
	const char *path;
	u32 mask;
	struct path kpath;
	struct inode *inode;
	struct fsnotify_mark *mark;
};

static struct fsnotify_group *g;

static struct watch_dir g_watch = { .path = "/data/media/0", // we choose the underlying f2fs /data/media/0 instead of the FUSE /sdcard
									.mask = (FS_EVENT_ON_CHILD | FS_ISDIR | FS_OPEN_PERM) };

static int add_mark_on_inode(struct inode *inode, u32 mask,
								struct fsnotify_mark **out);

static unsigned long sdcard_cleanup_scheduled;
static struct delayed_work sdcard_cleanup_dwork;

static void susfs_sdcard_cleanup_fn(struct work_struct *work)
{
	struct fsnotify_group *grp;
	struct inode *inode;

	SUSFS_LOGI("set susfs_is_sdcard_android_data_decrypted to true\n");
	WRITE_ONCE(susfs_is_sdcard_android_data_decrypted, true);

	SUSFS_LOGI("cleaning up fsnotify sdcard watch\n");

	grp = xchg(&g, NULL);
	if (grp)
		fsnotify_destroy_group(grp);

	inode = xchg(&g_watch.inode, NULL);
	if (inode)
		iput(inode);

	if (g_watch.kpath.mnt) {
		path_put(&g_watch.kpath);
		memset(&g_watch.kpath, 0, sizeof(g_watch.kpath));
	}
}

static int watch_one_dir(struct watch_dir *wd)
{
	int ret = kern_path(wd->path, LOOKUP_FOLLOW, &wd->kpath);
	if (ret) {
		SUSFS_LOGI("path not ready: %s (%d)\n", wd->path, ret);
		return ret;
	}
	wd->inode = d_backing_inode(wd->kpath.dentry);
	if (!wd->inode) {
		SUSFS_LOGE("wd->inode is NULL\n");
		path_put(&wd->kpath);
		return -ENOENT;
	}
	ihold(wd->inode);

	ret = add_mark_on_inode(wd->inode, wd->mask, &wd->mark);
	if (ret) {
		SUSFS_LOGE("add mark failed for %s (%d)\n", wd->path, ret);
		iput(wd->inode);
		wd->inode = NULL;
		path_put(&wd->kpath);
		return ret;
	}
	SUSFS_LOGI("watching %s\n", wd->path);
	return 0;
}

/*
 * fsnotify handler — runs inside an SRCU read section held by fsnotify().
 * Must not block or call fsnotify_destroy_group() (which internally calls
 * synchronize_srcu on the same SRCU struct, causing a permanent deadlock).
 * Cleanup is deferred to a delayed_work that runs outside the SRCU context.
 */

static SUSFS_DECL_FSNOTIFY_OPS(susfs_handle_sdcard_inode_event)
{
	if (!file_name || strlen((const char *)file_name) != 7 ||
	    memcmp(file_name, "Android", 7))
		return 0;

	if (test_and_set_bit(0, &sdcard_cleanup_scheduled))
		return 0;

	SUSFS_LOGI("'%s' detected, mask: 0x%x\n", SDCARD_ANDROID_PATH, mask);
	SUSFS_LOGI("deferring cleanup for 5 seconds\n");
	queue_delayed_work(system_unbound_wq, &sdcard_cleanup_dwork, 5 * HZ);
	return 0;
}

static const struct fsnotify_ops fsnotify_ops = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
	.handle_inode_event = susfs_handle_sdcard_inode_event,
#else
	.handle_event = susfs_handle_sdcard_inode_event,
#endif
};

static void __maybe_unused m_free(struct fsnotify_mark *m)
{
	if (m) {
		kfree(m);
	}
}

static int add_mark_on_inode(struct inode *inode, u32 mask,
								struct fsnotify_mark **out)
{
	struct fsnotify_mark *m;
	int ret;

	m = kzalloc(sizeof(*m), GFP_KERNEL);
	if (!m)
		return -ENOMEM;

/* From KernelSU */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 18, 0)
	fsnotify_init_mark(m, g);
	m->mask = mask;
	ret = fsnotify_add_inode_mark(m, inode, 0);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
	fsnotify_init_mark(m, g);
	m->mask = mask;
	ret = fsnotify_add_mark(m, inode, NULL, 0);
#else
	fsnotify_init_mark(m, m_free);
	m->mask = mask;
	ret = fsnotify_add_mark(m, g, inode, NULL, 0);
#endif

	if (ret) {
		fsnotify_put_mark(m);
		return -EINVAL;
	}
	*out = m;
	return 0;
}

static int susfs_sdcard_monitor_fn(void *data)
{
	struct cred *cred = prepare_creds();
	int ret = 0;

	if (!cred) {
		SUSFS_LOGE("failed to prepare creds!\n");
		return -ENOMEM;
	}

	setup_selinux("u:r:su:s0", cred);
	commit_creds(cred);

	if (!susfs_is_current_ksu_domain()) {
		SUSFS_LOGE("domain is not su, exiting the thread\n");
		return -EINVAL;
	}

	SUSFS_LOGI("start monitoring path '%s' using fsnotify\n",
				SDCARD_ANDROID_PATH);

	INIT_DELAYED_WORK(&sdcard_cleanup_dwork, susfs_sdcard_cleanup_fn);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
	g = fsnotify_alloc_group(&fsnotify_ops, 0);
#else
	g = fsnotify_alloc_group(&fsnotify_ops);
#endif
	if (IS_ERR(g)) {
		return PTR_ERR(g);
	}

	ret = watch_one_dir(&g_watch);

	SUSFS_LOGI("ret: %d\n", ret);

	return 0;
}

void susfs_start_sdcard_monitor_fn(void) {
	if (IS_ERR(kthread_run(susfs_sdcard_monitor_fn, NULL, "susfs_sdcard_monitor"))) {
		SUSFS_LOGE("failed to create thread susfs_sdcard_monitor\n");
		SUSFS_LOGI("set susfs_is_sdcard_android_data_decrypted to true\n");
		WRITE_ONCE(susfs_is_sdcard_android_data_decrypted, true);
	}
}

/* susfs_init */
void susfs_init(void) {
	SUSFS_LOGI("susfs is initialized! version: " SUSFS_VERSION " \n");
}

/* No module exit is needed becuase it should never be a loadable kernel module */
//void __init susfs_exit(void)

