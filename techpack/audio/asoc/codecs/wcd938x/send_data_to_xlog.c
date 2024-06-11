#include "send_data_to_xlog.h"
#include <linux/debugfs.h>
//#include <linux/device.h>

char wcd938x_msg_format[] = "{\"name\":\"mbhc_impedance\",\"audio_event\":{\"mbhc_impedance_left\":\"%d\",\"mbhc_impedance_right\":\"%d\"},\"dgt\":\"null\",\"audio_ext\":\"null\" }";

#define MAX_LEN 512

void send_mbhc_impedance_to_xlog(const unsigned int zl, const unsigned int zr)
{
	int ret = -1;
	pr_info("%s: zl = %d, zr = %d", __func__, zl, zr);
	ret = xlog_wcd938x_send_int(zl, zr);
	if (ret < 0) {
		pr_info("%s: failed", __func__);
	} else {
		pr_info("%s: success", __func__);
	}
}

int xlog_wcd938x_send_int(const unsigned int zl, const unsigned int zr)
{
	int ret = 0;
	char msg[512];
	pr_info("%s: zl = %d, zr = %d", __func__, zl, zr);
	ret = xlog_wcd938x_format_msg_int(msg, zl, zr);
	if (ret < 0) {
		return ret;
	}
#ifdef CONFIG_XLOGCHAR
	xlogchar_kwrite(msg, sizeof(msg));
#endif
	pr_info("%s: send msg: %s", __func__, msg);
	return ret;
}

int xlog_wcd938x_format_msg_int(char *msg, const unsigned int zl, const unsigned int zr)
{
	if (msg == NULL) {
		pr_info("%s: the msg is NULL", __func__);
		return -EINVAL;
	}
	pr_info("%s start", __func__);
	snprintf(msg, MAX_LEN, wcd938x_msg_format, zl, zr);
	pr_info("%s end", __func__);
	return 0;
}
