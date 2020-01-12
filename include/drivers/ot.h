#ifndef __OT_H__
#define __OT_H__

#include <zephyr.h>
#include <net/openthread.h>

void init_open_thread(struct openthread_context *_ot_context);
uint8_t *ot_get_eui64(void);

#endif /* __OT_H__ */
