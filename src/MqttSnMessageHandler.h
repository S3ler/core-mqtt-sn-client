//
// Created by bele on 08.04.17.
//

#ifndef CORE_MQTT_SN_CLIENT_MQTTSNMESSAGEHANDLER_H
#define CORE_MQTT_SN_CLIENT_MQTTSNMESSAGEHANDLER_H

class SocketInterface;
class Client;

#include "SocketInterface.h"
#include "Client.h"

class MqttSnMessageHandler {
private:
    SocketInterface *socket;
    Client* core;

public:
    bool begin();
    void setSocket(SocketInterface *socket);
    void setCore(Client* core);
    void receiveData(device_address *address, uint8_t *bytes);

    void parse_pingreq(device_address *address, uint8_t *bytes);
    void handle_pingreq(device_address *address);
    void send_pingresp(device_address *address);

    void send_connect(device_address *address, const char *client_id, uint16_t duration);

    void parse_connack(device_address *pAddress, uint8_t *bytes);

    void send_disconnect(device_address *pAddress);

    void notify_socket_connected();

    void notify_socket_disconnected();

    void send_pingreq(device_address* address);

    void parse_pingresp(device_address *pAddress, uint8_t *bytes);

    void send_subscribe(device_address *address, const char *topic_name, uint8_t qos);

    void parse_suback(device_address *pAddress, uint8_t *bytes);

    void parse_publish(device_address *address, uint8_t *bytes);

    void send_publish(device_address *address, uint8_t *data, uint8_t data_len, uint16_t msg_id,
                                            uint16_t topic_id, bool short_topic, bool retain, uint8_t qos, bool dup);
};


#endif //CORE_MQTT_SN_CLIENT_MQTTSNMESSAGEHANDLER_H
