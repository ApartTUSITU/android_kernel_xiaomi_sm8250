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

void send_mbhc_impedance_to_xlog(const unsigned int zl, const unsigned int zr);
int xlog_wcd938x_send_int(const unsigned int zl, const unsigned int zr);
int xlog_wcd938x_format_msg_int(char *msg, const unsigned int zl, const unsigned int zr);

#endif

