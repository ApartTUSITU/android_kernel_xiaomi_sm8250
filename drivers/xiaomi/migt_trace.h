#undef TRACE_SYSTEM
#define TRACE_SYSTEM migt
#if !defined(_MIGT_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _MIGT_TRACE_H
#include <linux/tracepoint.h>

TRACE_EVENT(migt_ioctl, TP_PROTO(int user_cmd, int trace_id),
	    TP_ARGS(user_cmd, trace_id),
	    TP_STRUCT__entry(__field(int, user_cmd) __field(int, trace_id)),
	    TP_fast_assign(__entry->user_cmd = user_cmd;
			   __entry->trace_id = trace_id;),
	    TP_printk("cmd=%d trace_id=%d", __entry->user_cmd,
		      __entry->trace_id));
TRACE_EVENT(migt_ioctl_done, TP_PROTO(int user_cmd, int trace_id),
	    TP_ARGS(user_cmd, trace_id),
	    TP_STRUCT__entry(__field(int, user_cmd) __field(int, trace_id)),
	    TP_fast_assign(__entry->user_cmd = user_cmd;
			   __entry->trace_id = trace_id;),
	    TP_printk("cmd=%d trace_id=%d", __entry->user_cmd,
		      __entry->trace_id));

TRACE_EVENT(do_migt, TP_PROTO(unsigned long work), TP_ARGS(work),
	    TP_STRUCT__entry(__field(unsigned long, work)),
	    TP_fast_assign(__entry->work = work;),
	    TP_printk("struct work_struct *work = 0x%x", __entry->work));
TRACE_EVENT(do_migt_done, TP_PROTO(unsigned long work), TP_ARGS(work),
	    TP_STRUCT__entry(__field(unsigned long, work)),
	    TP_fast_assign(__entry->work = work;),
	    TP_printk("struct work_struct *work = 0x%x", __entry->work));

TRACE_EVENT(do_migt_rem, TP_PROTO(unsigned long work), TP_ARGS(work),
	    TP_STRUCT__entry(__field(unsigned long, work)),
	    TP_fast_assign(__entry->work = work;),
	    TP_printk("struct work_struct *work = 0x%x", __entry->work));
TRACE_EVENT(do_migt_rem_done, TP_PROTO(unsigned long work), TP_ARGS(work),
	    TP_STRUCT__entry(__field(unsigned long, work)),
	    TP_fast_assign(__entry->work = work;),
	    TP_printk("struct work_struct *work = 0x%x", __entry->work));

TRACE_EVENT(do_ceiling_limit, TP_PROTO(unsigned long work), TP_ARGS(work),
	    TP_STRUCT__entry(__field(unsigned long, work)),
	    TP_fast_assign(__entry->work = work;),
	    TP_printk("struct work_struct *work = 0x%x", __entry->work));
TRACE_EVENT(do_ceiling_limit_done, TP_PROTO(unsigned long work), TP_ARGS(work),
	    TP_STRUCT__entry(__field(unsigned long, work)),
	    TP_fast_assign(__entry->work = work;),
	    TP_printk("struct work_struct *work = 0x%x", __entry->work));

TRACE_EVENT(do_ceiling_rem, TP_PROTO(unsigned long work), TP_ARGS(work),
	    TP_STRUCT__entry(__field(unsigned long, work)),
	    TP_fast_assign(__entry->work = work;),
	    TP_printk("struct work_struct *work = 0x%x", __entry->work));
TRACE_EVENT(do_ceiling_rem_done, TP_PROTO(unsigned long work), TP_ARGS(work),
	    TP_STRUCT__entry(__field(unsigned long, work)),
	    TP_fast_assign(__entry->work = work;),
	    TP_printk("struct work_struct *work = 0x%x", __entry->work));

TRACE_EVENT(boost_adjust_notify,
	    TP_PROTO(unsigned long nb_ptr, unsigned long val,
		     unsigned long data_ptr),
	    TP_ARGS(nb_ptr, val, data_ptr),
	    TP_STRUCT__entry(__field(unsigned long, nb_ptr)
				     __field(unsigned long, val)
					     __field(unsigned long, data_ptr)),
	    TP_fast_assign(__entry->nb_ptr = nb_ptr; __entry->val = val;
			   __entry->data_ptr = data_ptr;),
	    TP_printk("nb=0x%x, val=%lu, data=0x%x", __entry->nb_ptr,
		      __entry->val, __entry->data_ptr));

TRACE_EVENT(boost_adjust_notify_done,
	    TP_PROTO(unsigned long nb_ptr, unsigned long val,
		     unsigned long data_ptr),
	    TP_ARGS(nb_ptr, val, data_ptr),
	    TP_STRUCT__entry(__field(unsigned long, nb_ptr)
				     __field(unsigned long, val)
					     __field(unsigned long, data_ptr)),
	    TP_fast_assign(__entry->nb_ptr = nb_ptr; __entry->val = val;
			   __entry->data_ptr = data_ptr;),
	    TP_printk("nb=0x%x, val=%lu, data=0x%x", __entry->nb_ptr,
		      __entry->val, __entry->data_ptr));

TRACE_EVENT(ceiling_adjust_notify,
	    TP_PROTO(unsigned long nb_ptr, unsigned long val,
		     unsigned long data_ptr),
	    TP_ARGS(nb_ptr, val, data_ptr),
	    TP_STRUCT__entry(__field(unsigned long, nb_ptr)
				     __field(unsigned long, val)
					     __field(unsigned long, data_ptr)),
	    TP_fast_assign(__entry->nb_ptr = nb_ptr; __entry->val = val;
			   __entry->data_ptr = data_ptr;),
	    TP_printk("nb=0x%x, val=%lu, data=0x%x", __entry->nb_ptr,
		      __entry->val, __entry->data_ptr));

TRACE_EVENT(ceiling_adjust_notify_done,
	    TP_PROTO(unsigned long nb_ptr, unsigned long val,
		     unsigned long data_ptr),
	    TP_ARGS(nb_ptr, val, data_ptr),
	    TP_STRUCT__entry(__field(unsigned long, nb_ptr)
				     __field(unsigned long, val)
					     __field(unsigned long, data_ptr)),
	    TP_fast_assign(__entry->nb_ptr = nb_ptr; __entry->val = val;
			   __entry->data_ptr = data_ptr;),
	    TP_printk("nb=0x%x, val=%lu, data=0x%x", __entry->nb_ptr,
		      __entry->val, __entry->data_ptr));

TRACE_EVENT(render_change, TP_PROTO(int old_pid, int new_pid),
	    TP_ARGS(old_pid, new_pid),
	    TP_STRUCT__entry(__field(int, old_pid) __field(int, new_pid)),
	    TP_fast_assign(__entry->old_pid = old_pid;
			   __entry->new_pid = new_pid;),
	    TP_printk("render pid changes, old:%d -> new:%d", __entry->old_pid,
		      __entry->new_pid));

TRACE_EVENT(cpu_freq_modify,
	    TP_PROTO(unsigned int cpu, unsigned int min, unsigned int max),
	    TP_ARGS(cpu, min, max),
	    TP_STRUCT__entry(__field(unsigned int, cpu)
				     __field(unsigned int, min)
					     __field(unsigned int, max)),
	    TP_fast_assign(__entry->cpu = cpu; __entry->min = min;
			   __entry->max = max;),
	    TP_printk("set cpu %lu freq : min=%lukHz, max=%lukHz", __entry->cpu,
		      __entry->min, __entry->max));

TRACE_EVENT(
	i_migt_info,
	TP_PROTO(int cpu, unsigned int migt_min, unsigned int boost_freq,
		 unsigned int migt_ceiling_max, unsigned int ceiling_freq),
	TP_ARGS(cpu, migt_min, boost_freq, migt_ceiling_max, ceiling_freq),
	TP_STRUCT__entry(__field(int, cpu) __field(unsigned int, migt_min)
				 __field(unsigned int, boost_freq)
					 __field(unsigned int, migt_ceiling_max)
						 __field(unsigned int,
							 ceiling_freq)),
	TP_fast_assign(__entry->cpu = cpu; __entry->migt_min = migt_min;
		       __entry->boost_freq = boost_freq;
		       __entry->migt_ceiling_max = migt_ceiling_max;
		       __entry->ceiling_freq = ceiling_freq;),
	TP_printk(
		"migt value in cpu %d modified: migt_min=0x%x, boost_freq=0x%x, migt_ceiling_max=0x%x, ceiling_freq=0x%x",
		__entry->cpu, __entry->migt_min, __entry->boost_freq,
		__entry->migt_ceiling_max, __entry->ceiling_freq));

#endif /* _MIGT_TRACE_H */

#undef TRACE_INCLUDE_PATH
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_PATH .
#define TRACE_INCLUDE_FILE migt_trace
#include <trace/define_trace.h>
