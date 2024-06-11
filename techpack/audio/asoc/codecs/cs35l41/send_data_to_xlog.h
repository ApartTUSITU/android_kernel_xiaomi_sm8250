#ifndef SEND_DATA_TO_XLOG
#define SEND_DATA_TO_XLOG

#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/ratelimit.h>
#include <asm/current.h>
#include <asm/div64.h>
#include <linux/kernel.h>
#include <linux/atomic.h>
#include <linux/poll.h>

#ifdef CONFIG_XLOGCHAR
extern ssize_t xlogchar_kwrite(const char __user *buf, size_t count);
#endif

void send_DC_data_to_xlog(char *reason);
int xlog_send_int(char *reason);
int xlog_format_msg_int (char *msg, char *reason);

#endif

