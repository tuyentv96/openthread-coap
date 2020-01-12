#include <logging/log.h>
LOG_MODULE_REGISTER(driver_ot, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <openthread/thread.h>

#include "drivers/ot.h"

static struct openthread_context *ot_context;

void init_open_thread(struct openthread_context *_ot_context)
{
    ot_context = _ot_context;
}

uint8_t *ot_get_eui64(void)
{
    uint8_t aIeeeEui64[8];
    int i;

    otPlatRadioGetIeeeEui64(ot_context, aIeeeEui64);
    LOG_DBG("EUI64: %x", aIeeeEui64);
    // for (i = 0; i < 8; i++)
    // {
    //     LOG_INF("%x", aIeeeEui64[i]);
    // }

    return aIeeeEui64;
}