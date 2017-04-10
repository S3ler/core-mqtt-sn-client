//
// Created by bele on 08.04.17.
//

#include <cstring>
#include "Client.h"
#include "mqttsn_messages.h"

bool Client::begin() {
    if (socket != nullptr && mqttSnMessageHandler != nullptr && system != nullptr) {
        return mqttSnMessageHandler->begin();
    }
    return false;
}

bool Client::is_gateway_address(device_address *pAddress) {
    return memcmp(&this->gw_address, pAddress, sizeof(device_address)) == 0;
}

void Client::notify_socket_disconnected() {
    this->socket_disconnected = true;
}

void Client::set_await_message(message_type msg_type) {
    this->await_msg_type = msg_type;
}

bool Client::loop() {
    if (!mqttsn_connected) {
        return false;
    }
    if (socket_disconnected) {
        set_await_message(MQTTSN_PINGREQ);
        return false;
    }
    socket->loop();
    if (system->has_beaten()) {
        ping_outstanding = true;
    }
    if (ping_outstanding && await_msg_type == MQTTSN_PINGREQ) {
        mqttSnMessageHandler->send_pingreq(&gw_address);
        set_await_message(MQTTSN_PINGRESP);
    }
    return true;
}

void Client::setCallback(MQTT_CALLBACK_SIGNATURE) {
    this->callback = callback;
}

bool Client::connect(device_address *address, const char *client_id, uint16_t duration) {
    // global verwalten
    uint8_t retries = 5;
    memset(&this->gw_address, 0, sizeof(device_address));
    for (uint8_t tries = 0; tries < retries; tries++) {
        system->set_heartbeat(15 * 1000);
        mqttSnMessageHandler->send_connect(address, client_id, duration);
        this->set_await_message(MQTTSN_CONNACK);
        while (!mqttsn_connected) {
            socket->loop();
            if (mqttsn_connected) {
                memcpy(&this->gw_address, address, sizeof(device_address));
                system->set_heartbeat(60 * 1000);
                return true;
            }
            if (system->has_beaten()) {
                // timeout
                break;
            }
            if (socket_disconnected) {
                return false;
            }
        }
        system->sleep((tries + 1) * 10 * 1000);
    }
    // timeout - take another gateway
    return false;
}

void Client::set_socket(SocketInterface *socket) {
    this->socket = socket;
}


void Client::subscribe(const char *topic, uint8_t qos) {
    while (this->await_msg_type != MQTTSN_PINGREQ) {
        // wait until we have no other messages in flight
        if (!socket->loop()) {
            return;
        }
    }
    mqttSnMessageHandler->send_subscribe(&gw_address, topic, qos);
    this->set_await_message(MQTTSN_SUBACK);
    memset(&registration, 0, sizeof(topic_registration));
    //strcpy(registration.topic_name, topic);
    registration.topic_name = (char *) topic;
    registration.granted_qos = qos;
    await_topic_id = true;
}

void Client::publish(const char *payload, const char *topic, int8_t qos) {
    uint16_t payload_length = strlen(payload);
    uint16_t msg_id = this->increment_and_get_msg_id_counter();
    if (qos == 0) {
        msg_id = 0;
    }
    uint16_t topic_id = registration.topic_id;
    mqttSnMessageHandler->send_publish(&gw_address, (uint8_t *)payload, payload_length, msg_id, topic_id, true, false,qos, false);
}

message_type Client::get_await_message() {
    return this->await_msg_type;
}

void Client::set_mqttsn_connected() {
    this->mqttsn_connected = true;
}

void Client::notify_socket_connected() {
    this->socket_disconnected = false;
}

void Client::set_system(System *system) {
    this->system = system;
}

void Client::set_mqttsn_message_handler(MqttSnMessageHandler *message_handler) {
    this->mqttSnMessageHandler = message_handler;
}

void Client::norify_pingresponse_arrived() {
    system->set_heartbeat(system->get_heartbeat());
    ping_outstanding = false;
    //TODO mention: client observer his timout value and manages connections only by HIS pingreq
    // pingrequeest from the gateway to the client do NOT reset this timer
    // TODO enhancement: both pingrequests and pingreqsponse reset the timer OR better:
    // ALL answers reset the timer
}

uint16_t Client::increment_and_get_msg_id_counter() {
    if (++msg_id_counter == 0) {
        msg_id_counter = 1;
    }
    return msg_id_counter;
}

uint16_t Client::get_await_message_id() {
    return msg_id_counter;
}

void Client::set_await_topic_id(uint16_t topic_id) {
    registration.topic_id = topic_id;
}

void Client::set_granted_qos(int8_t granted_qos) {
    registration.granted_qos = (uint8_t) granted_qos;
}

bool Client::is_mqttsn_connected() {
    return mqttsn_connected;
}

void Client::handle_publish(device_address *address, uint8_t *data, uint16_t data_len, uint16_t topic_id, bool retain,
                          int8_t qos) {
    // call callback :)
    if (topic_id == registration.topic_id && qos == registration.granted_qos) {
        callback(registration.topic_name, data, data_len,retain);
    }
}

