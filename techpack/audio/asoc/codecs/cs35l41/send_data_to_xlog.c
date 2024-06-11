#include "send_data_to_xlog.h"
#include <linux/debugfs.h>

char msg_format[] = "{\"name\":\"dc_detection\",\"audio_event\":{\"dc_reason\":\"%s\"},\"dgt\":\"null\",\"audio_ext\":\"null\" }";

#define MAX_LEN 512

void send_DC_data_to_xlog(char *reason)
{
	int ret = -1;
	pr_info("%s:  reason: %s", __func__, reason);
	ret = xlog_send_int(reason);
	if (ret < 0) {
		pr_info("%s: failed", __func__);
	} else {
		pr_info("%s: success", __func__);
	}
}

int xlog_send_int(char *reason)
{
	int ret = 0;
	char msg[512];
	pr_info("%s: reason: %s", __func__, reason);
	ret = xlog_format_msg_int(msg, reason);
	if (ret < 0) {
		return ret;
	}
#ifdef CONFIG_XLOGCHAR
	xlogchar_kwrite(msg, sizeof(msg));
#endif
	pr_info("%s: send msg: %s", __func__, msg);
	return ret;
}

int xlog_format_msg_int (char *msg, char *reason)
{
	if (msg == NULL) {
		pr_info("%s: the msg is NULL", __func__);
		return -EINVAL;
	}
	pr_info("%s start", __func__);
	snprintf(msg, MAX_LEN, msg_format, reason);
	pr_info("%s end", __func__);
	return 0;
}
