#ifndef SCHED_PKG_H
#define SCHED_PKG_H

#define TRACE_UID_MAX 81920
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

enum package_type {
	TOP_ALL = 0,
	TOP_ON_SCORE,
	TOP_ON_BCORE,
	TOP_ON_LCORE,
	PACKAGE_TYPES,
};

struct runtime_info {
	u64 update_time;
};

struct runtime_info *get_package_runtime_info(void);
extern int pause_mode;

#define get_interface(value_name)                                              \
	void *get_##value_name(void)                                           \
	{                                                                      \
		return &value_name;                                            \
	}

#define declare_interface(value_name) void *get_##value_name(void);

declare_interface(curr_runtime_items);
declare_interface(uid_max_value);
declare_interface(runtime_window_size);
declare_interface(user_zero);

void create_runtime_proc(void);
void create_game_load_proc(void *rootdir);
void delete_runtimeproc(void);
void delete_game_load_proc(void);
#endif
