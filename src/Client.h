//
// Created by bele on 08.04.17.
//

#ifndef CORE_MQTT_SN_CLIENT_CORE_H
#define CORE_MQTT_SN_CLIENT_CORE_H

class MqttSnMessageHandler;

#include "global_defines.h"
#include "SocketInterface.h"
#include "System.h"
#include "mqttsn_messages.h"

struct topic_registration{
    char* topic_name;
    uint16_t topic_id;
    uint8_t granted_qos;
};

#ifdef ESP8266
#include <functional>
#define MQTT_CALLBACK_SIGNATURE std::function<void(char*, uint8_t*, uint16_t)> callback
#else
#define MQTT_CALLBACK_SIGNATURE void (*callback)(char*, uint8_t*, uint16_t, bool retain)
#endif

class Client {
private:
    SocketInterface *socket;
    MqttSnMessageHandler *mqttSnMessageHandler;
    System *system;
    device_address gw_address;
    bool socket_disconnected = false;
    bool mqttsn_connected = false;
    message_type await_msg_type;
    bool ping_outstanding = false;
    MQTT_CALLBACK_SIGNATURE;
    //TODO supports only one subscribtion topic at the moment
    topic_registration registration;
    uint16_t msg_id_counter = 1;
public:
    bool await_topic_id = false;

    virtual bool begin();

    bool is_gateway_address(device_address *pAddress);

    void notify_socket_disconnected();

    void setCallback(MQTT_CALLBACK_SIGNATURE);

    bool connect(device_address *address, const char *client_id, uint16_t duration);

    bool loop();

    void set_await_message(message_type msg_type);

    void set_socket(SocketInterface *socket);

    void subscribe(const char *topic, uint8_t qos);

    void publish(const char *payload, const char *topic, int8_t qos);

    message_type get_await_message();

    void set_mqttsn_connected();

    void notify_socket_connected();

    void set_system(System *system);

    void set_mqttsn_message_handler(MqttSnMessageHandler *message_handler);

    void norify_pingresponse_arrived();

    uint16_t increment_and_get_msg_id_counter();

    uint16_t get_await_message_id();

    void set_await_topic_id(uint16_t topic_id);

    void set_granted_qos(int8_t granted_qos);

    bool is_mqttsn_connected();

    void
    handle_publish(device_address *address, uint8_t data[], uint16_t data_len, uint16_t topic_id, bool retain, int8_t qos);
};


#endif //CORE_MQTT_SN_CLIENT_CORE_H
