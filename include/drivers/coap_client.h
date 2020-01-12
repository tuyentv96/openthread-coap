#ifndef __COAP_CLIENT_H__
#define __COAP_CLIENT_H__

/**
 * @brief Representation of a CoAP message.
 */
struct coap_meassage
{
    u8_t method;   /* Coap method */
    u8_t *payload; /* Coap payload */
};

int start_coap_client(void);
int send_simple_coap_msgs_and_wait_for_reply(void);
int coap_send(u8_t method, u8_t *payload);

#endif /* __COAP_CLIENT_H__ */
