/* echo-client.c - Networking echo client */

/*
 * Copyright (c) 2017 Intel Corporation.
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The echo-client application is acting as a client that is run in Zephyr OS,
 * and echo-server is run in the host acting as a server. The client will send
 * either unicast or multicast packets to the server which will reply the packet
 * back to the originator.
 *
 * In this sample application we create four threads that start to send data.
 * This might not be what you want to do in your app so caveat emptor.
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <errno.h>
#include <stdio.h>

#include <net/socket.h>
#include <net/tls_credentials.h>

#include <net/net_mgmt.h>
#include <net/net_event.h>
#include <net/net_conn_mgr.h>

#include "common.h"
#include "ca_certificate.h"
#include <net/openthread.h>
#include <openthread/thread.h>
#include <openthread/link.h>
#include <net/coap.h>
#include <drivers/coap_client.h>

#define APP_BANNER "Coap client"

#define EVENT_MASK (NET_EVENT_L4_CONNECTED | \
                    NET_EVENT_L4_DISCONNECTED)

static int nfds;

static bool connected;
K_SEM_DEFINE(run_app, 0, 1);

static struct net_mgmt_event_callback mgmt_cb;
static struct openthread_context *ot_context;
typedef void (*ot_config_disconn_cb)(void);
ot_config_disconn_cb disconn_cb;

static otDeviceRole role = OT_DEVICE_ROLE_DISABLED;

void get_ot_state(int role);

static void event_handler(struct net_mgmt_event_callback *cb,
                          u32_t mgmt_event, struct net_if *iface)
{
    if ((mgmt_event & EVENT_MASK) != mgmt_event)
    {
        return;
    }

    if (mgmt_event == NET_EVENT_L4_CONNECTED)
    {
        LOG_INF("Network connected");

        connected = true;
        k_sem_give(&run_app);

        return;
    }

    if (mgmt_event == NET_EVENT_L4_DISCONNECTED)
    {
        LOG_INF("Network disconnected");

        connected = false;
        k_sem_reset(&run_app);

        return;
    }
}

static void init_app(void)
{
    LOG_INF(APP_BANNER);

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
    int err = tls_credential_add(CA_CERTIFICATE_TAG,
                                 TLS_CREDENTIAL_CA_CERTIFICATE,
                                 ca_certificate,
                                 sizeof(ca_certificate));
    if (err < 0)
    {
        LOG_ERR("Failed to register public certificate: %d", err);
    }
#endif

#if defined(CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED)
    err = tls_credential_add(PSK_TAG,
                             TLS_CREDENTIAL_PSK,
                             psk,
                             sizeof(psk));
    if (err < 0)
    {
        LOG_ERR("Failed to register PSK: %d", err);
    }
    err = tls_credential_add(PSK_TAG,
                             TLS_CREDENTIAL_PSK_ID,
                             psk_id,
                             sizeof(psk_id) - 1);
    if (err < 0)
    {
        LOG_ERR("Failed to register PSK ID: %d", err);
    }
#endif

    if (IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER))
    {
        net_mgmt_init_event_callback(&mgmt_cb,
                                     event_handler, EVENT_MASK);
        net_mgmt_add_event_callback(&mgmt_cb);

        net_conn_mgr_resend_status();
    }

    init_vlan();
}

static void update_role(void)
{
    role = otThreadGetDeviceRole(ot_context->instance);
    LOG_DBG("OT role: %u", role);

    /* Device connected */
    if (role != OT_DEVICE_ROLE_DISABLED && role != OT_DEVICE_ROLE_DETACHED)
        return;

    /* Device disconnected */
    LOG_WRN("OT disconnected!");
    if (disconn_cb != NULL)
        disconn_cb();
}

static void change_cb(otChangedFlags aFlags, void *aContext)
{
    /* Ignore if no role change */
    if ((aFlags & OT_CHANGED_THREAD_ROLE) == false)
        return;

    LOG_DBG("OT Role changed");
    update_role();
}

void ot_state_changed_app_handler(uint32_t flags, void *context)
{
    struct openthread_context *ot_context = context;

    LOG_INF("State changed! Flags: 0x%08" PRIx32 " Current role: %d",
            flags, otThreadGetDeviceRole(ot_context->instance));
    get_ot_state(otThreadGetDeviceRole(ot_context->instance));

    if (flags & OT_CHANGED_IP6_ADDRESS_REMOVED)
    {
        LOG_DBG("Ipv6 address removed");
        rm_ipv6_addr_from_zephyr(ot_context);
    }

    if (flags & OT_CHANGED_IP6_ADDRESS_ADDED)
    {
        LOG_DBG("Ipv6 address added");
        add_ipv6_addr_to_zephyr(ot_context);
    }

    if (flags & OT_CHANGED_IP6_MULTICAST_UNSUBSCRIBED)
    {
        LOG_DBG("Ipv6 multicast address removed");
        rm_ipv6_maddr_from_zephyr(ot_context);
    }

    if (flags & OT_CHANGED_IP6_MULTICAST_SUBSCRIBED)
    {
        LOG_DBG("Ipv6 multicast address added");
        add_ipv6_maddr_to_zephyr(ot_context);
    }
}

int ot_config_init()
{
    struct net_if *iface;
    int state;

    LOG_DBG("Initializing OpenThread handler");
    iface = net_if_get_default();
    if (iface == NULL)
    {
        LOG_ERR("Failed to get net interface");
        return -1;
    }

    ot_context = net_if_l2_data(iface);
    if (ot_context == NULL)
    {
        LOG_ERR("Failed to get OT context");
        return -1;
    }

    otSetStateChangedCallback(ot_context->instance,
                              &ot_state_changed_app_handler, ot_context);

    // state = otThreadGetDeviceRole(ot_context);

    return 0;
}

void get_ot_state(int role)
{
    switch (role)
    {
    case OT_DEVICE_ROLE_CHILD:
        LOG_INF("STATE: OT_DEVICE_ROLE_CHILD");
        break;
    case OT_DEVICE_ROLE_ROUTER:
        LOG_INF("STATE: OT_DEVICE_ROLE_ROUTER");
        break;
    case OT_DEVICE_ROLE_LEADER:
        LOG_INF("STATE: OT_DEVICE_ROLE_LEADER");
        break;
    case OT_DEVICE_ROLE_DISABLED:
        LOG_INF("STATE: OT_DEVICE_ROLE_DISABLED");
        break;
    case OT_DEVICE_ROLE_DETACHED:
        LOG_INF("STATE: OT_DEVICE_ROLE_DETACHED");
        break;
    default:
        LOG_INF("STATE: default");
        break;
    }
}

int ot_config_start(void)
{
    int rc;

    rc = otIp6SetEnabled(ot_context->instance, true);
    if (rc)
    {
        LOG_ERR("Failed to enable IPv6 communication. (err %d)", rc);
        return rc;
    }

    rc = otThreadSetEnabled(ot_context->instance, true);
    if (rc)
        LOG_ERR("Failed to start Thread protocol. (err %d)", rc);

    return rc;
}

int ot_config_stop(void)
{
    int rc;

    /* Disable OpenThread service */
    LOG_DBG("Stopping OpenThread service");

    rc = otThreadSetEnabled(ot_context->instance, false);
    if (rc)
    {
        LOG_ERR("Failed to stop Thread protocol. (err %d)", rc);
        return rc;
    }

    rc = otIp6SetEnabled(ot_context->instance, false);
    if (rc)
        LOG_ERR("Failed to disable IPv6 communication. (err %d)", rc);

    return rc;
}

void start_app(void)
{
    int ret;
    int r;

    init_app();
    ot_config_init();
    ot_get_eui64();

    LOG_DBG("Start CoAP-client sample");
    r = start_coap_client();
    if (r < 0)
    {
        LOG_INF("start_coap_client failed %d", r);
    }
    // ot_config_stop();
    // ot_config_start();
    // get_ot_state();

    while (true)
    {
        /* GET, PUT, POST, DELETE */
        r = send_simple_coap_msgs_and_wait_for_reply();
        if (r < 0)
        {
            LOG_INF("send_simple_coap_msgs_and_wait_for_reply failed %d", r);
        }

        k_sleep(5000);
    }
}
